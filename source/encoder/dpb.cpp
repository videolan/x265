/*****************************************************************************
 * Copyright (C) 2013-2020 MulticoreWare, Inc
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Min Chen <chenm003@163.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *
 * This program is also available under a commercial proprietary license.
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#include "common.h"
#include "frame.h"
#include "framedata.h"
#include "picyuv.h"
#include "slice.h"

#include "dpb.h"

using namespace X265_NS;

DPB::~DPB()
{
    while (!m_freeList.empty())
    {
        Frame* curFrame = m_freeList.popFront();
        curFrame->destroy();
        delete curFrame;
    }

    while (!m_picList.empty())
    {
        Frame* curFrame = m_picList.popFront();
        curFrame->destroy();
        delete curFrame;
    }

    while (m_frameDataFreeList)
    {
        FrameData* next = m_frameDataFreeList->m_freeListNext;
        m_frameDataFreeList->destroy();

        m_frameDataFreeList->m_reconPic->destroy();
        delete m_frameDataFreeList->m_reconPic;

        delete m_frameDataFreeList;
        m_frameDataFreeList = next;
    }
}

// move unreferenced pictures from picList to freeList for recycle
void DPB::recycleUnreferenced()
{
    Frame *iterFrame = m_picList.first();

    while (iterFrame)
    {
        Frame *curFrame = iterFrame;
        iterFrame = iterFrame->m_next;
        bool isMCSTFReferenced = false;

        if (curFrame->m_param->bEnableTemporalFilter)
            isMCSTFReferenced =!!(curFrame->m_refPicCnt[1]);

        if (!curFrame->m_encData->m_bHasReferences && !curFrame->m_countRefEncoders && !isMCSTFReferenced)
        {
            curFrame->m_bChromaExtended = false;

            if (curFrame->m_param->bEnableTemporalFilter)
                *curFrame->m_isSubSampled = false;

            // Reset column counter
            X265_CHECK(curFrame->m_reconRowFlag != NULL, "curFrame->m_reconRowFlag check failure");
            X265_CHECK(curFrame->m_reconColCount != NULL, "curFrame->m_reconColCount check failure");
            X265_CHECK(curFrame->m_numRows > 0, "curFrame->m_numRows check failure");

            for(int32_t row = 0; row < curFrame->m_numRows; row++)
            {
                curFrame->m_reconRowFlag[row].set(0);
                curFrame->m_reconColCount[row].set(0);
            }

            // iterator is invalidated by remove, restart scan
            m_picList.remove(*curFrame);
            iterFrame = m_picList.first();

            m_freeList.pushBack(*curFrame);
            curFrame->m_encData->m_freeListNext = m_frameDataFreeList;
            m_frameDataFreeList = curFrame->m_encData;
            for (int i = 0; i < INTEGRAL_PLANE_NUM; i++)
            {
                if (curFrame->m_encData->m_meBuffer[i] != NULL)
                {
                    X265_FREE(curFrame->m_encData->m_meBuffer[i]);
                    curFrame->m_encData->m_meBuffer[i] = NULL;
                }
            }
            if (curFrame->m_ctuInfo != NULL)
            {
                uint32_t widthInCU = (curFrame->m_param->sourceWidth + curFrame->m_param->maxCUSize - 1) >> curFrame->m_param->maxLog2CUSize;
                uint32_t heightInCU = (curFrame->m_param->sourceHeight + curFrame->m_param->maxCUSize - 1) >> curFrame->m_param->maxLog2CUSize;
                uint32_t numCUsInFrame = widthInCU * heightInCU;
                for (uint32_t i = 0; i < numCUsInFrame; i++)
                {
                    X265_FREE((*curFrame->m_ctuInfo + i)->ctuInfo);
                    (*curFrame->m_ctuInfo + i)->ctuInfo = NULL;
                }
                X265_FREE(*curFrame->m_ctuInfo);
                *(curFrame->m_ctuInfo) = NULL;
                X265_FREE(curFrame->m_ctuInfo);
                curFrame->m_ctuInfo = NULL;
                X265_FREE(curFrame->m_prevCtuInfoChange);
                curFrame->m_prevCtuInfoChange = NULL;
            }
            curFrame->m_encData = NULL;
            curFrame->m_reconPic = NULL;
        }
    }
}

void DPB::prepareEncode(Frame *newFrame)
{
    Slice* slice = newFrame->m_encData->m_slice;
    slice->m_poc = newFrame->m_poc;
    slice->m_fieldNum = newFrame->m_fieldNum;

    int pocCurr = slice->m_poc;
    int type = newFrame->m_lowres.sliceType;
    bool bIsKeyFrame = newFrame->m_lowres.bKeyframe;
    slice->m_nalUnitType = getNalUnitType(pocCurr, bIsKeyFrame);
    if (slice->m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_W_RADL || slice->m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_N_LP)
        m_lastIDR = pocCurr;
    slice->m_lastIDR = m_lastIDR;
    slice->m_sliceType = IS_X265_TYPE_B(type) ? B_SLICE : (type == X265_TYPE_P) ? P_SLICE : I_SLICE;

    if (type == X265_TYPE_B)
    {
        newFrame->m_encData->m_bHasReferences = false;

        newFrame->m_tempLayer = (newFrame->m_param->bEnableTemporalSubLayers && !m_bTemporalSublayer) ? 1 : newFrame->m_tempLayer;
        // Adjust NAL type for unreferenced B frames (change from _R "referenced"
        // to _N "non-referenced" NAL unit type)
        switch (slice->m_nalUnitType)
        {
        case NAL_UNIT_CODED_SLICE_TRAIL_R:
            slice->m_nalUnitType = newFrame->m_param->bEnableTemporalSubLayers ? NAL_UNIT_CODED_SLICE_TSA_N : NAL_UNIT_CODED_SLICE_TRAIL_N;
            break;
        case NAL_UNIT_CODED_SLICE_RADL_R:
            slice->m_nalUnitType = NAL_UNIT_CODED_SLICE_RADL_N;
            break;
        case NAL_UNIT_CODED_SLICE_RASL_R:
            slice->m_nalUnitType = NAL_UNIT_CODED_SLICE_RASL_N;
            break;
        default:
            break;
        }
    }
    else
    {
        /* m_bHasReferences starts out as true for non-B pictures, and is set to false
         * once no more pictures reference it */
        newFrame->m_encData->m_bHasReferences = true;
    }

    m_picList.pushFront(*newFrame);

    if (m_bTemporalSublayer && getTemporalLayerNonReferenceFlag())
    {
        switch (slice->m_nalUnitType)
        {
        case NAL_UNIT_CODED_SLICE_TRAIL_R:
            slice->m_nalUnitType =  NAL_UNIT_CODED_SLICE_TRAIL_N;
            break;
        case NAL_UNIT_CODED_SLICE_RADL_R:
            slice->m_nalUnitType = NAL_UNIT_CODED_SLICE_RADL_N;
            break;
        case NAL_UNIT_CODED_SLICE_RASL_R:
            slice->m_nalUnitType = NAL_UNIT_CODED_SLICE_RASL_N;
            break;
        default:
            break;
        }
    }
    // Do decoding refresh marking if any
    decodingRefreshMarking(pocCurr, slice->m_nalUnitType);

    computeRPS(pocCurr, newFrame->m_tempLayer, slice->isIRAP(), &slice->m_rps, slice->m_sps->maxDecPicBuffering[newFrame->m_tempLayer]);
    bool isTSAPic = ((slice->m_nalUnitType == 2) || (slice->m_nalUnitType == 3)) ? true : false;
    // Mark pictures in m_piclist as unreferenced if they are not included in RPS
    applyReferencePictureSet(&slice->m_rps, pocCurr, newFrame->m_tempLayer, isTSAPic);


    if (m_bTemporalSublayer && newFrame->m_tempLayer > 0
        && !(slice->m_nalUnitType == NAL_UNIT_CODED_SLICE_RADL_N     // Check if not a leading picture
            || slice->m_nalUnitType == NAL_UNIT_CODED_SLICE_RADL_R
            || slice->m_nalUnitType == NAL_UNIT_CODED_SLICE_RASL_N
            || slice->m_nalUnitType == NAL_UNIT_CODED_SLICE_RASL_R)
        )
    {
        if (isTemporalLayerSwitchingPoint(pocCurr, newFrame->m_tempLayer) || (slice->m_sps->maxTempSubLayers == 1))
        {
            if (getTemporalLayerNonReferenceFlag())
            {
                slice->m_nalUnitType = NAL_UNIT_CODED_SLICE_TSA_N;
            }
            else
            {
                slice->m_nalUnitType = NAL_UNIT_CODED_SLICE_TSA_R;
            }
        }
        else if (isStepwiseTemporalLayerSwitchingPoint(&slice->m_rps, pocCurr, newFrame->m_tempLayer))
        {
            bool isSTSA = true;
            int id = newFrame->m_gopOffset % x265_gop_ra_length[newFrame->m_gopId];
            for (int ii = id; (ii < x265_gop_ra_length[newFrame->m_gopId] && isSTSA == true); ii++)
            {
                int tempIdRef = x265_gop_ra[newFrame->m_gopId][ii].layer;
                if (tempIdRef == newFrame->m_tempLayer)
                {
                    for (int jj = 0; jj < slice->m_rps.numberOfPositivePictures + slice->m_rps.numberOfNegativePictures; jj++)
                    {
                        if (slice->m_rps.bUsed[jj])
                        {
                            int refPoc = x265_gop_ra[newFrame->m_gopId][ii].poc_offset + slice->m_rps.deltaPOC[jj];
                            int kk = 0;
                            for (kk = 0; kk < x265_gop_ra_length[newFrame->m_gopId]; kk++)
                            {
                                if (x265_gop_ra[newFrame->m_gopId][kk].poc_offset == refPoc)
                                {
                                    break;
                                }
                            }
                            if (x265_gop_ra[newFrame->m_gopId][kk].layer >= newFrame->m_tempLayer)
                            {
                                isSTSA = false;
                                break;
                            }
                        }
                    }
                }
            }
            if (isSTSA == true)
            {
                if (getTemporalLayerNonReferenceFlag())
                {
                    slice->m_nalUnitType = NAL_UNIT_CODED_SLICE_STSA_N;
                }
                else
                {
                    slice->m_nalUnitType = NAL_UNIT_CODED_SLICE_STSA_R;
                }
            }
        }
    }

    if (slice->m_sliceType != I_SLICE)
        slice->m_numRefIdx[0] = x265_clip3(1, newFrame->m_param->maxNumReferences, slice->m_rps.numberOfNegativePictures);
    else
        slice->m_numRefIdx[0] = X265_MIN(newFrame->m_param->maxNumReferences, slice->m_rps.numberOfNegativePictures); // Ensuring L0 contains just the -ve POC
    slice->m_numRefIdx[1] = X265_MIN(newFrame->m_param->bBPyramid ? 2 : 1, slice->m_rps.numberOfPositivePictures);
    slice->setRefPicList(m_picList);

    X265_CHECK(slice->m_sliceType != B_SLICE || slice->m_numRefIdx[1], "B slice without L1 references (non-fatal)\n");

    if (slice->m_sliceType == B_SLICE)
    {
        /* TODO: the lookahead should be able to tell which reference picture
         * had the least motion residual.  We should be able to use that here to
         * select a colocation reference list and index */
        slice->m_colFromL0Flag = false;
        slice->m_colRefIdx = 0;
        slice->m_bCheckLDC = false;
    }
    else
    {
        slice->m_bCheckLDC = true;
        slice->m_colFromL0Flag = true;
        slice->m_colRefIdx = 0;
    }

    // Disable Loopfilter in bound area, because we will do slice-parallelism in future
    slice->m_sLFaseFlag = (newFrame->m_param->maxSlices > 1) ? false : ((SLFASE_CONSTANT & (1 << (pocCurr % 31))) > 0);

    /* Increment reference count of all motion-referenced frames to prevent them
     * from being recycled. These counts are decremented at the end of
     * compressFrame() */
    int numPredDir = slice->isInterP() ? 1 : slice->isInterB() ? 2 : 0;
    for (int l = 0; l < numPredDir; l++)
    {
        for (int ref = 0; ref < slice->m_numRefIdx[l]; ref++)
        {
            Frame *refpic = slice->m_refFrameList[l][ref];
            ATOMIC_INC(&refpic->m_countRefEncoders);
        }
    }
}

void DPB::computeRPS(int curPoc, int tempId, bool isRAP, RPS * rps, unsigned int maxDecPicBuffer)
{
    unsigned int poci = 0, numNeg = 0, numPos = 0;

    Frame* iterPic = m_picList.first();

    while (iterPic && (poci < maxDecPicBuffer - 1))
    {
        if ((iterPic->m_poc != curPoc) && iterPic->m_encData->m_bHasReferences)
        {
            if ((!m_bTemporalSublayer || (iterPic->m_tempLayer <= tempId)) && ((m_lastIDR >= curPoc) || (m_lastIDR <= iterPic->m_poc)))
            {
                    rps->poc[poci] = iterPic->m_poc;
                    rps->deltaPOC[poci] = rps->poc[poci] - curPoc;
                    (rps->deltaPOC[poci] < 0) ? numNeg++ : numPos++;
                    rps->bUsed[poci] = !isRAP;
                    poci++;
            }
        }
        iterPic = iterPic->m_next;
    }

    rps->numberOfPictures = poci;
    rps->numberOfPositivePictures = numPos;
    rps->numberOfNegativePictures = numNeg;

    rps->sortDeltaPOC();
}

bool DPB::getTemporalLayerNonReferenceFlag()
{
    Frame* curFrame = m_picList.first();
    if (curFrame->m_encData->m_bHasReferences)
    {
        curFrame->m_sameLayerRefPic = true;
        return false;
    }
    else
        return true;
}

/* Marking reference pictures when an IDR/CRA is encountered. */
void DPB::decodingRefreshMarking(int pocCurr, NalUnitType nalUnitType)
{
    if (nalUnitType == NAL_UNIT_CODED_SLICE_IDR_W_RADL || nalUnitType == NAL_UNIT_CODED_SLICE_IDR_N_LP)
    {
        /* If the nal_unit_type is IDR, all pictures in the reference picture
         * list are marked as "unused for reference" */
        Frame* iterFrame = m_picList.first();
        while (iterFrame)
        {
            if (iterFrame->m_poc != pocCurr)
                iterFrame->m_encData->m_bHasReferences = false;
            iterFrame = iterFrame->m_next;
        }
    }
    else // CRA or No DR
    {
        if (m_bRefreshPending && pocCurr > m_pocCRA)
        {
            /* If the bRefreshPending flag is true (a deferred decoding refresh
             * is pending) and the current temporal reference is greater than
             * the temporal reference of the latest CRA picture (pocCRA), mark
             * all reference pictures except the latest CRA picture as "unused
             * for reference" and set the bRefreshPending flag to false */
            Frame* iterFrame = m_picList.first();
            while (iterFrame)
            {
                if (iterFrame->m_poc != pocCurr && iterFrame->m_poc != m_pocCRA)
                    iterFrame->m_encData->m_bHasReferences = false;
                iterFrame = iterFrame->m_next;
            }

            m_bRefreshPending = false;
        }
        if (nalUnitType == NAL_UNIT_CODED_SLICE_CRA)
        {
            /* If the nal_unit_type is CRA, set the bRefreshPending flag to true
             * and pocCRA to the temporal reference of the current picture */
            m_bRefreshPending = true;
            m_pocCRA = pocCurr;
        }
    }

    /* Note that the current picture is already placed in the reference list and
     * its marking is not changed.  If the current picture has a nal_ref_idc
     * that is not 0, it will remain marked as "used for reference" */
}

/** Function for applying picture marking based on the Reference Picture Set */
void DPB::applyReferencePictureSet(RPS *rps, int curPoc, int tempId, bool isTSAPicture)
{
    // loop through all pictures in the reference picture buffer
    Frame* iterFrame = m_picList.first();
    while (iterFrame)
    {
        if (iterFrame->m_poc != curPoc && iterFrame->m_encData->m_bHasReferences)
        {
            // loop through all pictures in the Reference Picture Set
            // to see if the picture should be kept as reference picture
            bool referenced = false;
            for (int i = 0; i < rps->numberOfPositivePictures + rps->numberOfNegativePictures; i++)
            {
                if (iterFrame->m_poc == curPoc + rps->deltaPOC[i])
                {
                    referenced = true;
                    break;
                }
            }
            if (!referenced)
                iterFrame->m_encData->m_bHasReferences = false;

            if (m_bTemporalSublayer)
            {
                //check that pictures of higher temporal layers are not used
                assert(referenced == 0 || iterFrame->m_encData->m_bHasReferences == false || iterFrame->m_tempLayer <= tempId);

                //check that pictures of higher or equal temporal layer are not in the RPS if the current picture is a TSA picture
                if (isTSAPicture)
                {
                    assert(referenced == 0 || iterFrame->m_tempLayer < tempId);
                }
                //check that pictures marked as temporal layer non-reference pictures are not used for reference
                if (iterFrame->m_tempLayer == tempId)
                {
                    assert(referenced == 0 || iterFrame->m_sameLayerRefPic == true);
                }
            }
        }
        iterFrame = iterFrame->m_next;
    }
}

bool DPB::isTemporalLayerSwitchingPoint(int curPoc, int tempId)
{
    // loop through all pictures in the reference picture buffer
    Frame* iterFrame = m_picList.first();
    while (iterFrame)
    {
        if (iterFrame->m_poc != curPoc && iterFrame->m_encData->m_bHasReferences)
        {
            if (iterFrame->m_tempLayer >= tempId)
            {
                return false;
            }
        }
        iterFrame = iterFrame->m_next;
    }
    return true;
}

bool DPB::isStepwiseTemporalLayerSwitchingPoint(RPS *rps, int curPoc, int tempId)
{
    // loop through all pictures in the reference picture buffer
    Frame* iterFrame = m_picList.first();
    while (iterFrame)
    {
        if (iterFrame->m_poc != curPoc && iterFrame->m_encData->m_bHasReferences)
        {
            for (int i = 0; i < rps->numberOfPositivePictures + rps->numberOfNegativePictures; i++)
            {
                if ((iterFrame->m_poc == curPoc + rps->deltaPOC[i]) && rps->bUsed[i])
                {
                    if (iterFrame->m_tempLayer >= tempId)
                    {
                        return false;
                    }
                }
            }
        }
        iterFrame = iterFrame->m_next;
    }
    return true;
}

/* deciding the nal_unit_type */
NalUnitType DPB::getNalUnitType(int curPOC, bool bIsKeyFrame)
{
    if (!curPOC)
        return NAL_UNIT_CODED_SLICE_IDR_N_LP;
    if (bIsKeyFrame)
        return m_bOpenGOP ? NAL_UNIT_CODED_SLICE_CRA : m_bhasLeadingPicture ? NAL_UNIT_CODED_SLICE_IDR_W_RADL : NAL_UNIT_CODED_SLICE_IDR_N_LP;
    if (m_pocCRA && curPOC < m_pocCRA)
        // All leading pictures are being marked as TFD pictures here since
        // current encoder uses all reference pictures while encoding leading
        // pictures. An encoder can ensure that a leading picture can be still
        // decodable when random accessing to a CRA/CRANT/BLA/BLANT picture by
        // controlling the reference pictures used for encoding that leading
        // picture. Such a leading picture need not be marked as a TFD picture.
        return NAL_UNIT_CODED_SLICE_RASL_R;

    if (m_lastIDR && curPOC < m_lastIDR)
        return NAL_UNIT_CODED_SLICE_RADL_R;

    return NAL_UNIT_CODED_SLICE_TRAIL_R;
}
