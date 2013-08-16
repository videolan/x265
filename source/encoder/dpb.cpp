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
    int gopIdx = pic->m_lowres.gopIdx;
    int pocCurr = pic->getSlice()->getPOC();

    Bool forceIntra = m_cfg->param.keyframeMax == 1 || (pocCurr % m_cfg->param.keyframeMax == 0) || pocCurr == 0;
    frameEncoder->initSlice(pic, forceIntra, gopIdx);

    TComSlice* slice = pic->getSlice();
    if (getNalUnitType(pocCurr, m_lastIDR) == NAL_UNIT_CODED_SLICE_IDR_W_RADL ||
        getNalUnitType(pocCurr, m_lastIDR) == NAL_UNIT_CODED_SLICE_IDR_N_LP)
    {
        m_lastIDR = pocCurr;
    }
    slice->setLastIDR(m_lastIDR);

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
