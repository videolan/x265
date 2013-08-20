/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
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
 * For more information, contact us at licensing@multicorewareinc.com.
 *****************************************************************************/

#include "TLibCommon/TComPic.h"
#include "TLibCommon/TComSlice.h"
#include "TLibEncoder/TEncCfg.h"

#include "PPA/ppa.h"
#include "dpb.h"
#include "frameencoder.h"

using namespace x265;

DPB::~DPB()
{
    while (!m_picList.empty())
    {
        TComPic* pic = m_picList.popFront();
        pic->destroy();
        delete pic;
    }
}

void DPB::recycleUnreferenced(TComList<TComPic*> freeList)
{
    // move unreferenced pictures from picList to freeList for recycle
    TComSlice::sortPicList(m_picList);
    TComList<TComPic*>::iterator iterPic = m_picList.begin();
    while (iterPic != m_picList.end())
    {
        TComPic *pic = *(iterPic++);
        if (pic->getSlice()->isReferenced() == false)
        {
            pic->getPicYuvRec()->clearReferences();
            pic->getPicYuvRec()->clearExtendedFlag();
            
            // iterator is invalidated by remove, restart scan
            m_picList.remove(pic);
            iterPic = m_picList.begin();

            freeList.pushBack(pic);
        }
    }
}

void DPB::prepareEncode(TComPic *pic, FrameEncoder *frameEncoder)
{
    PPAScopeEvent(DPB_prepareEncode);

    int gopIdx = pic->m_lowres.gopIdx;
    int pocCurr = pic->getSlice()->getPOC();

    Bool forceIntra = m_cfg->param.keyframeMax == 1 || (pocCurr % m_cfg->param.keyframeMax == 0) || pocCurr == 0;
    m_picList.pushBack(pic);
    frameEncoder->initSlice(pic, forceIntra, gopIdx);

    TComSlice* slice = pic->getSlice();
    if (getNalUnitType(pocCurr, m_lastIDR) == NAL_UNIT_CODED_SLICE_IDR_W_RADL ||
        getNalUnitType(pocCurr, m_lastIDR) == NAL_UNIT_CODED_SLICE_IDR_N_LP)
    {
        m_lastIDR = pocCurr;
    }
    slice->setLastIDR(m_lastIDR);

    slice->setNumRefIdx(REF_PIC_LIST_0, m_cfg->getGOPEntry(gopIdx).m_numRefPicsActive);
    slice->setNumRefIdx(REF_PIC_LIST_1, m_cfg->getGOPEntry(gopIdx).m_numRefPicsActive);

    if (slice->getSliceType() == B_SLICE && m_cfg->getGOPEntry(gopIdx).m_sliceType == 'P')
    {
        slice->setSliceType(P_SLICE);
    }
    if (pocCurr == 0)
    {
        slice->setTemporalLayerNonReferenceFlag(false);
    }
    else
    {
        // m_refPic is true if this frame is used as a motion reference
        slice->setTemporalLayerNonReferenceFlag(!m_cfg->getGOPEntry(gopIdx).m_refPic);
    }

    // Set the nal unit type
    slice->setNalUnitType(getNalUnitType(pocCurr, m_lastIDR));

    // If the slice is un-referenced, change from _R "referenced" to _N "non-referenced" NAL unit type
    if (slice->getTemporalLayerNonReferenceFlag())
    {
        if (slice->getNalUnitType() == NAL_UNIT_CODED_SLICE_TRAIL_R)
        {
            slice->setNalUnitType(NAL_UNIT_CODED_SLICE_TRAIL_N);
        }
        if (slice->getNalUnitType() == NAL_UNIT_CODED_SLICE_RADL_R)
        {
            slice->setNalUnitType(NAL_UNIT_CODED_SLICE_RADL_N);
        }
        if (slice->getNalUnitType() == NAL_UNIT_CODED_SLICE_RASL_R)
        {
            slice->setNalUnitType(NAL_UNIT_CODED_SLICE_RASL_N);
        }
    }

    // Do decoding refresh marking if any
    slice->decodingRefreshMarking(m_pocCRA, m_bRefreshPending, m_picList);
    selectReferencePictureSet(slice, frameEncoder, pocCurr, gopIdx);
    slice->getRPS()->setNumberOfLongtermPictures(0);

    if ((slice->checkThatAllRefPicsAreAvailable(m_picList, slice->getRPS(), false) != 0) || (slice->isIRAP()))
    {
        slice->createExplicitReferencePictureSetFromReference(m_picList, slice->getRPS(), slice->isIRAP());
    }
    slice->applyReferencePictureSet(m_picList, slice->getRPS());

    arrangeLongtermPicturesInRPS(slice, frameEncoder);
    TComRefPicListModification* refPicListModification = slice->getRefPicListModification();
    refPicListModification->setRefPicListModificationFlagL0(false);
    refPicListModification->setRefPicListModificationFlagL1(false);
    slice->setNumRefIdx(REF_PIC_LIST_0, min(m_cfg->getGOPEntry(gopIdx).m_numRefPicsActive, slice->getRPS()->getNumberOfPictures()));
    slice->setNumRefIdx(REF_PIC_LIST_1, min(m_cfg->getGOPEntry(gopIdx).m_numRefPicsActive, slice->getRPS()->getNumberOfPictures()));

    // TODO: Is this safe for frame parallelism?
    slice->setRefPicList(m_picList);

    // Slice type refinement
    if ((slice->getSliceType() == B_SLICE) && (slice->getNumRefIdx(REF_PIC_LIST_1) == 0))
    {
        slice->setSliceType(P_SLICE);
    }

    if (slice->getSliceType() == B_SLICE)
    {
        // TODO: figure out what this is doing, replace with something more accurate
        //       what is setColFromL0Flag() for?

        // select colDir
        UInt colDir = 1;
        int closeLeft = 1, closeRight = -1;
        for (int i = 0; i < m_cfg->getGOPEntry(gopIdx).m_numRefPics; i++)
        {
            int ref = m_cfg->getGOPEntry(gopIdx).m_referencePics[i];
            if (ref > 0 && (ref < closeRight || closeRight == -1))
            {
                closeRight = ref;
            }
            else if (ref < 0 && (ref > closeLeft || closeLeft == 1))
            {
                closeLeft = ref;
            }
        }

        if (closeRight > -1)
        {
            closeRight = closeRight + m_cfg->getGOPEntry(gopIdx).m_POC - 1;
        }
        if (closeLeft < 1)
        {
            closeLeft = closeLeft + m_cfg->getGOPEntry(gopIdx).m_POC - 1;
            while (closeLeft < 0)
            {
                closeLeft += m_cfg->getGOPSize();
            }
        }
        int leftQP = 0, rightQP = 0;
        for (int i = 0; i < m_cfg->getGOPSize(); i++)
        {
            if (m_cfg->getGOPEntry(i).m_POC == (closeLeft % m_cfg->getGOPSize()) + 1)
            {
                leftQP = m_cfg->getGOPEntry(i).m_QPOffset;
            }
            if (m_cfg->getGOPEntry(i).m_POC == (closeRight % m_cfg->getGOPSize()) + 1)
            {
                rightQP = m_cfg->getGOPEntry(i).m_QPOffset;
            }
        }

        if (closeRight > -1 && rightQP < leftQP)
        {
            colDir = 0;
        }
        slice->setColFromL0Flag(1 - colDir);

        Bool bLowDelay = true;
        int curPOC = slice->getPOC();
        int refIdx = 0;

        for (refIdx = 0; refIdx < slice->getNumRefIdx(REF_PIC_LIST_0) && bLowDelay; refIdx++)
        {
            if (slice->getRefPic(REF_PIC_LIST_0, refIdx)->getPOC() > curPOC)
            {
                bLowDelay = false;
            }
        }

        for (refIdx = 0; refIdx < slice->getNumRefIdx(REF_PIC_LIST_1) && bLowDelay; refIdx++)
        {
            if (slice->getRefPic(REF_PIC_LIST_1, refIdx)->getPOC() > curPOC)
            {
                bLowDelay = false;
            }
        }

        slice->setCheckLDC(bLowDelay);
    }
    else
    {
        slice->setCheckLDC(true);
    }

    slice->setRefPOCList();
    slice->setList1IdxToList0Idx();
    slice->setEnableTMVPFlag(1);

    Bool bGPBcheck = false;
    if (slice->getSliceType() == B_SLICE)
    {
        if (slice->getNumRefIdx(REF_PIC_LIST_0) == slice->getNumRefIdx(REF_PIC_LIST_1))
        {
            bGPBcheck = true;
            for (int i = 0; i < slice->getNumRefIdx(REF_PIC_LIST_1); i++)
            {
                if (slice->getRefPOC(REF_PIC_LIST_1, i) != slice->getRefPOC(REF_PIC_LIST_0, i))
                {
                    bGPBcheck = false;
                    break;
                }
            }
        }
    }

    slice->setMvdL1ZeroFlag(bGPBcheck);
    slice->setNextSlice(false);
}

// This is a function that determines what Reference Picture Set to use for a
// specific slice (with POC = POCCurr)
void DPB::selectReferencePictureSet(TComSlice* slice, FrameEncoder *frameEncoder, int curPOC, int gopID)
{
    slice->setRPSidx(gopID);
    UInt intraPeriod = m_cfg->param.keyframeMax;
    int gopSize = m_cfg->getGOPSize();

    for (int extraNum = gopSize; extraNum < m_cfg->getExtraRPSs() + gopSize; extraNum++)
    {
        if (intraPeriod > 0 && m_cfg->param.decodingRefreshType > 0)
        {
            int POCIndex = curPOC % intraPeriod;
            if (POCIndex == 0)
            {
                POCIndex = intraPeriod;
            }
            if (POCIndex == m_cfg->getGOPEntry(extraNum).m_POC)
            {
                slice->setRPSidx(extraNum);
            }
        }
        else
        {
            if (curPOC == m_cfg->getGOPEntry(extraNum).m_POC)
            {
                slice->setRPSidx(extraNum);
            }
        }
    }

    slice->setRPS(frameEncoder->m_sps.getRPSList()->getReferencePictureSet(slice->getRPSidx()));
    slice->getRPS()->setNumberOfPictures(slice->getRPS()->getNumberOfNegativePictures() + slice->getRPS()->getNumberOfPositivePictures());
}

int DPB::getReferencePictureSetIdxForSOP(int curPOC, int gopID)
{
    int rpsIdx = gopID;
    int gopSize = m_cfg->getGOPSize();
    UInt intraPeriod = m_cfg->param.keyframeMax;

    for (int extraNum = gopSize; extraNum < m_cfg->getExtraRPSs() + gopSize; extraNum++)
    {
        if (intraPeriod > 0 && m_cfg->param.decodingRefreshType > 0)
        {
            int POCIndex = curPOC % intraPeriod;
            if (POCIndex == 0)
            {
                POCIndex = intraPeriod;
            }
            if (POCIndex == m_cfg->getGOPEntry(extraNum).m_POC)
            {
                rpsIdx = extraNum;
            }
        }
        else
        {
            if (curPOC == m_cfg->getGOPEntry(extraNum).m_POC)
            {
                rpsIdx = extraNum;
            }
        }
    }

    return rpsIdx;
}

/** Function for deciding the nal_unit_type.
 * \param pocCurr POC of the current picture
 * \returns the nal unit type of the picture
 * This function checks the configuration and returns the appropriate nal_unit_type for the picture.
 */
NalUnitType DPB::getNalUnitType(int curPOC, int lastIDR)
{
    if (curPOC == 0)
    {
        return NAL_UNIT_CODED_SLICE_IDR_W_RADL;
    }
    if (curPOC % m_cfg->param.keyframeMax == 0)
    {
        if (m_cfg->param.decodingRefreshType == 1)
        {
            return NAL_UNIT_CODED_SLICE_CRA;
        }
        else if (m_cfg->param.decodingRefreshType == 2)
        {
            return NAL_UNIT_CODED_SLICE_IDR_W_RADL;
        }
    }
    if (m_pocCRA > 0)
    {
        if (curPOC < m_pocCRA)
        {
            // All leading pictures are being marked as TFD pictures here since current encoder uses all
            // reference pictures while encoding leading pictures. An encoder can ensure that a leading
            // picture can be still decodable when random accessing to a CRA/CRANT/BLA/BLANT picture by
            // controlling the reference pictures used for encoding that leading picture. Such a leading
            // picture need not be marked as a TFD picture.
            return NAL_UNIT_CODED_SLICE_RASL_R;
        }
    }
    if (lastIDR > 0)
    {
        if (curPOC < lastIDR)
        {
            return NAL_UNIT_CODED_SLICE_RADL_R;
        }
    }
    return NAL_UNIT_CODED_SLICE_TRAIL_R;
}

static inline int getLSB(int poc, int maxLSB)
{
    if (poc >= 0)
    {
        return poc % maxLSB;
    }
    else
    {
        return (maxLSB - ((-poc) % maxLSB)) % maxLSB;
    }
}

// Function will arrange the long-term pictures in the decreasing order of poc_lsb_lt,
// and among the pictures with the same lsb, it arranges them in increasing delta_poc_msb_cycle_lt value
void DPB::arrangeLongtermPicturesInRPS(TComSlice *slice, x265::FrameEncoder *frameEncoder)
{
    TComReferencePictureSet *rps = slice->getRPS();

    if (!rps->getNumberOfLongtermPictures())
    {
        return;
    }

    // Arrange long-term reference pictures in the correct order of LSB and MSB,
    // and assign values for pocLSBLT and MSB present flag
    int longtermPicsPoc[MAX_NUM_REF_PICS], longtermPicsLSB[MAX_NUM_REF_PICS], indices[MAX_NUM_REF_PICS];
    int longtermPicsMSB[MAX_NUM_REF_PICS];
    Bool mSBPresentFlag[MAX_NUM_REF_PICS];
    ::memset(longtermPicsPoc, 0, sizeof(longtermPicsPoc));  // Store POC values of LTRP
    ::memset(longtermPicsLSB, 0, sizeof(longtermPicsLSB));  // Store POC LSB values of LTRP
    ::memset(longtermPicsMSB, 0, sizeof(longtermPicsMSB));  // Store POC LSB values of LTRP
    ::memset(indices, 0, sizeof(indices));                  // Indices to aid in tracking sorted LTRPs
    ::memset(mSBPresentFlag, 0, sizeof(mSBPresentFlag));    // Indicate if MSB needs to be present

    // Get the long-term reference pictures
    int offset = rps->getNumberOfNegativePictures() + rps->getNumberOfPositivePictures();
    int i, ctr = 0;
    int maxPicOrderCntLSB = 1 << frameEncoder->m_sps.getBitsForPOC();
    for (i = rps->getNumberOfPictures() - 1; i >= offset; i--, ctr++)
    {
        longtermPicsPoc[ctr] = rps->getPOC(i);                                  // LTRP POC
        longtermPicsLSB[ctr] = getLSB(longtermPicsPoc[ctr], maxPicOrderCntLSB); // LTRP POC LSB
        indices[ctr] = i;
        longtermPicsMSB[ctr] = longtermPicsPoc[ctr] - longtermPicsLSB[ctr];
    }

    int numLongPics = rps->getNumberOfLongtermPictures();
    assert(ctr == numLongPics);

    // Arrange pictures in decreasing order of MSB;
    for (i = 0; i < numLongPics; i++)
    {
        for (int j = 0; j < numLongPics - 1; j++)
        {
            if (longtermPicsMSB[j] < longtermPicsMSB[j + 1])
            {
                std::swap(longtermPicsPoc[j], longtermPicsPoc[j + 1]);
                std::swap(longtermPicsLSB[j], longtermPicsLSB[j + 1]);
                std::swap(longtermPicsMSB[j], longtermPicsMSB[j + 1]);
                std::swap(indices[j], indices[j + 1]);
            }
        }
    }

    for (i = 0; i < numLongPics; i++)
    {
        // Check if MSB present flag should be enabled.
        // Check if the buffer contains any pictures that have the same LSB.
        TComList<TComPic*>::iterator iterPic = m_picList.begin();
        TComPic* pic;
        while (iterPic != m_picList.end())
        {
            pic = *iterPic;
            if ((getLSB(pic->getPOC(), maxPicOrderCntLSB) == longtermPicsLSB[i])   && // Same LSB
                (pic->getSlice()->isReferenced()) &&                                  // Reference picture
                (pic->getPOC() != longtermPicsPoc[i]))                                // Not the LTRP itself
            {
                mSBPresentFlag[i] = true;
                break;
            }
            iterPic++;
        }
    }

    // tempArray for usedByCurr flag
    Bool tempArray[MAX_NUM_REF_PICS];
    ::memset(tempArray, 0, sizeof(tempArray));
    for (i = 0; i < numLongPics; i++)
    {
        tempArray[i] = rps->getUsed(indices[i]) ? true : false;
    }

    // Now write the final values;
    ctr = 0;
    int currMSB = 0, currLSB = 0;
    // currPicPoc = currMSB + currLSB
    currLSB = getLSB(slice->getPOC(), maxPicOrderCntLSB);
    currMSB = slice->getPOC() - currLSB;

    for (i = rps->getNumberOfPictures() - 1; i >= offset; i--, ctr++)
    {
        rps->setPOC(i, longtermPicsPoc[ctr]);
        rps->setDeltaPOC(i, -slice->getPOC() + longtermPicsPoc[ctr]);
        rps->setUsed(i, tempArray[ctr]);
        rps->setPocLSBLT(i, longtermPicsLSB[ctr]);
        rps->setDeltaPocMSBCycleLT(i, (currMSB - (longtermPicsPoc[ctr] - longtermPicsLSB[ctr])) / maxPicOrderCntLSB);
        rps->setDeltaPocMSBPresentFlag(i, mSBPresentFlag[ctr]);

        assert(rps->getDeltaPocMSBCycleLT(i) >= 0); // Non-negative value
    }

    for (i = rps->getNumberOfPictures() - 1, ctr = 1; i >= offset; i--, ctr++)
    {
        for (int j = rps->getNumberOfPictures() - 1 - ctr; j >= offset; j--)
        {
            // Here at the encoder we know that we have set the full POC value for the LTRPs, hence we
            // don't have to check the MSB present flag values for this constraint.
            assert(rps->getPOC(i) != rps->getPOC(j)); // If assert fails, LTRP entry repeated in RPS!!!
        }
    }
}

static inline bool _confirm(bool bflag, const char* message)
{
    if (!bflag)
        return false;

    printf("Error: %s\n", message);
    return true;
}

bool TEncCfg::initializeGOP(x265_param_t *_param)
{
#define CONFIRM(expr, msg) check_failed |= _confirm(expr, msg)
    bool check_failed = false; /* abort if there is a fatal configuration problem */

    if (_param->keyframeMax == 1)
    {
        /* encoder_all_I */
        m_gopList[0] = GOPEntry();
        m_gopList[0].m_QPFactor = 1;
        m_gopList[0].m_POC = 1;
        m_gopList[0].m_numRefPicsActive = 4;
    }
    else if (_param->bframes > 0) // hacky temporary way to select random access
    {
        /* encoder_randomaccess_main */
        int offsets[] = { 1, 2, 3, 4, 4, 3, 4, 4 };
        double factors[] = { 0, 0.442, 0.3536, 0.3536, 0.68 };
        int pocs[] = { 8, 4, 2, 1, 3, 6, 5, 7 };
        int rps[] = { 0, 4, 2, 1, -2, -3, 1, -2 };
        for (int i = 0; i < 8; i++)
        {
            m_gopList[i].m_POC = pocs[i];
            m_gopList[i].m_QPFactor = factors[offsets[i]];
            m_gopList[i].m_QPOffset = offsets[i];
            m_gopList[i].m_deltaRPS = rps[i];
            m_gopList[i].m_sliceType = 'B';
            m_gopList[i].m_numRefPicsActive = i ? 2 : 4;
            m_gopList[i].m_numRefPics = i == 1 ? 3 : 4;
            m_gopList[i].m_interRPSPrediction = i ? 1 : 0;
            m_gopList[i].m_numRefIdc = i == 2 ? 4 : 5;
        }

#define SET4(id, VAR, a, b, c, d) \
    m_gopList[id].VAR[0] = a; m_gopList[id].VAR[1] = b; m_gopList[id].VAR[2] = c; m_gopList[id].VAR[3] = d;
#define SET5(id, VAR, a, b, c, d, e) \
    m_gopList[id].VAR[0] = a; m_gopList[id].VAR[1] = b; m_gopList[id].VAR[2] = c; m_gopList[id].VAR[3] = d; m_gopList[id].VAR[4] = e;

        SET4(0, m_referencePics, -8, -10, -12, -16);
        SET4(1, m_referencePics, -4, -6, 4, 0);
        SET4(2, m_referencePics, -2, -4, 2, 6);
        SET4(3, m_referencePics, -1, 1, 3, 7);
        SET4(4, m_referencePics, -1, -3, 1, 5);
        SET4(5, m_referencePics, -2, -4, -6, 2);
        SET4(6, m_referencePics, -1, -5, 1, 3);
        SET4(7, m_referencePics, -1, -3, -7, 1);
        SET5(1, m_refIdc, 1, 1, 0, 0, 1);
        SET5(2, m_refIdc, 1, 1, 1, 1, 0);
        SET5(3, m_refIdc, 1, 0, 1, 1, 1);
        SET5(4, m_refIdc, 1, 1, 1, 1, 0);
        SET5(5, m_refIdc, 1, 1, 1, 1, 0);
        SET5(6, m_refIdc, 1, 0, 1, 1, 1);
        SET5(7, m_refIdc, 1, 1, 1, 1, 0);
        m_gopSize = 8;
    }
    else
    {
        /* encoder_I_15P */
        int offsets[] = { 3, 2, 3, 1 };
        for (int i = 0; i < 4; i++)
        {
            m_gopList[i].m_POC = i + 1;
            m_gopList[i].m_QPFactor = 0.4624;
            m_gopList[i].m_QPOffset = offsets[i];
            m_gopList[i].m_numRefPicsActive = 1;
            m_gopList[i].m_numRefPics = 1;
            m_gopList[i].m_referencePics[0] = -1;
            m_gopList[i].m_usedByCurrPic[0] = 1;
            m_gopList[i].m_sliceType = 'P';
            if (i > 0)
            {
                m_gopList[i].m_interRPSPrediction = 1;
                m_gopList[i].m_deltaRPS = -1;
                m_gopList[i].m_numRefIdc = 2;
                m_gopList[i].m_refIdc[0] = 0;
                m_gopList[i].m_refIdc[1] = 1;
            }
        }

        m_gopList[3].m_QPFactor = 0.578;
    }

    if (_param->bOpenGOP)
    {
        _param->keyframeMax = MAX_INT;
    }
    else if (_param->keyframeMax > 0)
    {
        m_gopSize = X265_MIN(_param->keyframeMax, m_gopSize);
        m_gopSize = X265_MAX(1, m_gopSize);
        int remain = _param->keyframeMax % m_gopSize;
        if (remain)
        {
            _param->keyframeMax += m_gopSize - remain;
            x265_log(_param, X265_LOG_WARNING, "Keyframe interval must be multiple of %d, forcing --keyint %d\n",
                m_gopSize, _param->keyframeMax);
        }
        if (_param->bframes && _param->keyframeMax < 16)
        {
            _param->keyframeMax = 16;
            x265_log(_param, X265_LOG_WARNING, "Keyframe interval must be at least 16 for B GOP structure\n");
        }
    }
    else if (_param->keyframeMax == 0)
    {
        // default keyframe interval of 1 second
        _param->keyframeMax = _param->frameRate;
        int remain = _param->keyframeMax % m_gopSize;
        if (remain)
            _param->keyframeMax += m_gopSize - remain;
    }
    _param->keyframeMin = _param->keyframeMax;

    if (_param->lookaheadDepth < m_gopSize)
    {
        // this check goes away when we have a real lookahead
        x265_log(_param, X265_LOG_WARNING, "Lookahead depth must be at least one GOP\n");
        _param->lookaheadDepth = m_gopSize;
    }

    bool verifiedGOP = false;
    bool errorGOP = false;
    int checkGOP = 1;
    int numRefs = 1;

    int refList[MAX_NUM_REF_PICS + 1];
    refList[0] = 0;
    bool isOK[MAX_GOP];
    for (Int i = 0; i < MAX_GOP; i++)
    {
        isOK[i] = false;
    }

    int numOK = 0;
    if (_param->bOpenGOP == false)
        CONFIRM(_param->keyframeMax % m_gopSize != 0, "Intra period must be a multiple of GOPSize");

    m_extraRPSs = 0;
    // start looping through frames in coding order until we can verify that the GOP structure is correct.
    while (!verifiedGOP && !errorGOP)
    {
        Int curGOP = (checkGOP - 1) % m_gopSize;
        Int curPOC = ((checkGOP - 1) / m_gopSize) * m_gopSize + m_gopList[curGOP].m_POC;
        if (m_gopList[curGOP].m_POC < 0)
        {
            printf("\nError: found fewer Reference Picture Sets than GOPSize\n");
            errorGOP = true;
        }
        else
        {
            //check that all reference pictures are available, or have a POC < 0 meaning they might be available in the next GOP.
            Bool beforeI = false;
            for (Int i = 0; i < m_gopList[curGOP].m_numRefPics; i++)
            {
                Int absPOC = curPOC + m_gopList[curGOP].m_referencePics[i];
                if (absPOC < 0)
                {
                    beforeI = true;
                }
                else
                {
                    Bool found = false;
                    for (Int j = 0; j < numRefs; j++)
                    {
                        if (refList[j] == absPOC)
                        {
                            found = true;
                            for (Int k = 0; k < m_gopSize; k++)
                            {
                                if (absPOC % m_gopSize == m_gopList[k].m_POC % m_gopSize)
                                {
                                    m_gopList[k].m_refPic = true;
                                    m_gopList[curGOP].m_usedByCurrPic[i] = 1;
                                }
                            }
                        }
                    }

                    if (!found)
                    {
                        printf("\nError: ref pic %d is not available for GOP frame %d\n", m_gopList[curGOP].m_referencePics[i], curGOP + 1);
                        errorGOP = true;
                    }
                }
            }

            if (!beforeI && !errorGOP)
            {
                //all ref frames were present
                if (!isOK[curGOP])
                {
                    numOK++;
                    isOK[curGOP] = true;
                    if (numOK == m_gopSize)
                    {
                        verifiedGOP = true;
                    }
                }
            }
            else
            {
                //create a new GOPEntry for this frame containing all the reference pictures that were available (POC > 0)
                m_gopList[m_gopSize + m_extraRPSs] = m_gopList[curGOP];
                Int newRefs = 0;
                for (Int i = 0; i < m_gopList[curGOP].m_numRefPics; i++)
                {
                    Int absPOC = curPOC + m_gopList[curGOP].m_referencePics[i];
                    if (absPOC >= 0)
                    {
                        m_gopList[m_gopSize + m_extraRPSs].m_referencePics[newRefs] = m_gopList[curGOP].m_referencePics[i];
                        m_gopList[m_gopSize + m_extraRPSs].m_usedByCurrPic[newRefs] = m_gopList[curGOP].m_usedByCurrPic[i];
                        newRefs++;
                    }
                }

                Int numPrefRefs = m_gopList[curGOP].m_numRefPicsActive;

                for (Int offset = -1; offset > -checkGOP; offset--)
                {
                    //step backwards in coding order and include any extra available pictures we might find useful to replace the ones with POC < 0.
                    Int offGOP = (checkGOP - 1 + offset) % m_gopSize;
                    Int offPOC = ((checkGOP - 1 + offset) / m_gopSize) * m_gopSize + m_gopList[offGOP].m_POC;
                    if (offPOC >= 0)
                    {
                        Bool newRef = false;
                        for (Int i = 0; i < numRefs; i++)
                        {
                            if (refList[i] == offPOC)
                            {
                                newRef = true;
                            }
                        }

                        for (Int i = 0; i < newRefs; i++)
                        {
                            if (m_gopList[m_gopSize + m_extraRPSs].m_referencePics[i] == offPOC - curPOC)
                            {
                                newRef = false;
                            }
                        }

                        if (newRef)
                        {
                            Int insertPoint = newRefs;
                            //this picture can be added, find appropriate place in list and insert it.
                            m_gopList[offGOP].m_refPic = true;
                            for (Int j = 0; j < newRefs; j++)
                            {
                                if (m_gopList[m_gopSize + m_extraRPSs].m_referencePics[j] < offPOC - curPOC || m_gopList[m_gopSize + m_extraRPSs].m_referencePics[j] > 0)
                                {
                                    insertPoint = j;
                                    break;
                                }
                            }

                            Int prev = offPOC - curPOC;
                            Int prevUsed = 1;
                            for (Int j = insertPoint; j < newRefs + 1; j++)
                            {
                                Int newPrev = m_gopList[m_gopSize + m_extraRPSs].m_referencePics[j];
                                Int newUsed = m_gopList[m_gopSize + m_extraRPSs].m_usedByCurrPic[j];
                                m_gopList[m_gopSize + m_extraRPSs].m_referencePics[j] = prev;
                                m_gopList[m_gopSize + m_extraRPSs].m_usedByCurrPic[j] = prevUsed;
                                prevUsed = newUsed;
                                prev = newPrev;
                            }

                            newRefs++;
                        }
                    }
                    if (newRefs >= numPrefRefs)
                    {
                        break;
                    }
                }

                m_gopList[m_gopSize + m_extraRPSs].m_numRefPics = newRefs;
                m_gopList[m_gopSize + m_extraRPSs].m_POC = curPOC;
                if (m_extraRPSs == 0)
                {
                    m_gopList[m_gopSize + m_extraRPSs].m_interRPSPrediction = 0;
                    m_gopList[m_gopSize + m_extraRPSs].m_numRefIdc = 0;
                }
                else
                {
                    Int rIdx =  m_gopSize + m_extraRPSs - 1;
                    Int refPOC = m_gopList[rIdx].m_POC;
                    Int refPics = m_gopList[rIdx].m_numRefPics;
                    Int newIdc = 0;
                    for (Int i = 0; i <= refPics; i++)
                    {
                        Int deltaPOC = ((i != refPics) ? m_gopList[rIdx].m_referencePics[i] : 0); // check if the reference abs POC is >= 0
                        Int absPOCref = refPOC + deltaPOC;
                        Int refIdc = 0;
                        for (Int j = 0; j < m_gopList[m_gopSize + m_extraRPSs].m_numRefPics; j++)
                        {
                            if ((absPOCref - curPOC) == m_gopList[m_gopSize + m_extraRPSs].m_referencePics[j])
                            {
                                if (m_gopList[m_gopSize + m_extraRPSs].m_usedByCurrPic[j])
                                {
                                    refIdc = 1;
                                }
                                else
                                {
                                    refIdc = 2;
                                }
                            }
                        }

                        m_gopList[m_gopSize + m_extraRPSs].m_refIdc[newIdc] = refIdc;
                        newIdc++;
                    }

                    m_gopList[m_gopSize + m_extraRPSs].m_interRPSPrediction = 1;
                    m_gopList[m_gopSize + m_extraRPSs].m_numRefIdc = newIdc;
                    m_gopList[m_gopSize + m_extraRPSs].m_deltaRPS = refPOC - m_gopList[m_gopSize + m_extraRPSs].m_POC;
                }
                curGOP = m_gopSize + m_extraRPSs;
                m_extraRPSs++;
            }
            numRefs = 0;
            for (Int i = 0; i < m_gopList[curGOP].m_numRefPics; i++)
            {
                Int absPOC = curPOC + m_gopList[curGOP].m_referencePics[i];
                if (absPOC >= 0)
                {
                    refList[numRefs] = absPOC;
                    numRefs++;
                }
            }

            refList[numRefs] = curPOC;
            numRefs++;
        }
        checkGOP++;
    }

    CONFIRM(errorGOP, "Invalid GOP structure given");
    for (Int i = 0; i < m_gopSize; i++)
    {
        CONFIRM(m_gopList[i].m_sliceType != 'B' && m_gopList[i].m_sliceType != 'P', "Slice type must be equal to B or P");
    }

    for (Int i = 0; i < MAX_TLAYER; i++)
    {
        m_numReorderPics[i] = 0;
        m_maxDecPicBuffering[i] = 1;
    }

    for (Int i = 0; i < m_gopSize; i++)
    {
        if (m_gopList[i].m_numRefPics + 1 > m_maxDecPicBuffering[0])
        {
            m_maxDecPicBuffering[0] = m_gopList[i].m_numRefPics + 1;
        }
        Int highestDecodingNumberWithLowerPOC = 0;
        for (Int j = 0; j < m_gopSize; j++)
        {
            if (m_gopList[j].m_POC <= m_gopList[i].m_POC)
            {
                highestDecodingNumberWithLowerPOC = j;
            }
        }

        Int numReorder = 0;
        for (Int j = 0; j < highestDecodingNumberWithLowerPOC; j++)
        {
            if (m_gopList[j].m_POC > m_gopList[i].m_POC)
            {
                numReorder++;
            }
        }

        if (numReorder > m_numReorderPics[0])
        {
            m_numReorderPics[0] = numReorder;
        }
    }

    for (Int i = 0; i < MAX_TLAYER - 1; i++)
    {
        // a lower layer can not have higher value of m_numReorderPics than a higher layer
        if (m_numReorderPics[i + 1] < m_numReorderPics[i])
        {
            m_numReorderPics[i + 1] = m_numReorderPics[i];
        }
        // the value of num_reorder_pics[ i ] shall be in the range of 0 to max_dec_pic_buffering[ i ] - 1, inclusive
        if (m_numReorderPics[i] > m_maxDecPicBuffering[i] - 1)
        {
            m_maxDecPicBuffering[i] = m_numReorderPics[i] + 1;
        }
        // a lower layer can not have higher value of m_uiMaxDecPicBuffering than a higher layer
        if (m_maxDecPicBuffering[i + 1] < m_maxDecPicBuffering[i])
        {
            m_maxDecPicBuffering[i + 1] = m_maxDecPicBuffering[i];
        }
    }

    // the value of num_reorder_pics[ i ] shall be in the range of 0 to max_dec_pic_buffering[ i ] -  1, inclusive
    if (m_numReorderPics[MAX_TLAYER - 1] > m_maxDecPicBuffering[MAX_TLAYER - 1] - 1)
    {
        m_maxDecPicBuffering[MAX_TLAYER - 1] = m_numReorderPics[MAX_TLAYER - 1] + 1;
    }

    return check_failed;
}
