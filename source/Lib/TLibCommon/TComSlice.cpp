/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2013, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/** \file     TComSlice.cpp
    \brief    slice header and SPS class
*/

#include "CommonDef.h"
#include "TComSlice.h"
#include "TComPic.h"
#include "TLibEncoder/TEncSbac.h"
#include "threadpool.h"

//! \ingroup TLibCommon
//! \{

TComSlice::TComSlice()
    : m_ppsId(-1)
    , m_poc(0)
    , m_lastIDR(0)
    , m_nalUnitType(NAL_UNIT_CODED_SLICE_IDR_W_RADL)
    , m_sliceType(I_SLICE)
    , m_sliceQp(0)
    , m_dependentSliceSegmentFlag(false)
    , m_sliceQpBase(0)
    , m_deblockingFilterDisable(false)
    , m_deblockingFilterOverrideFlag(false)
    , m_deblockingFilterBetaOffsetDiv2(0)
    , m_deblockingFilterTcOffsetDiv2(0)
    , m_bCheckLDC(false)
    , m_sliceQpDelta(0)
    , m_sliceQpDeltaCb(0)
    , m_sliceQpDeltaCr(0)
    , m_depth(0)
    , m_bReferenced(false)
    , m_sps(NULL)
    , m_pps(NULL)
    , m_pic(NULL)
    , m_colFromL0Flag(1)
    , m_colRefIdx(0)
    , m_lumaLambda(0.0)
    , m_chromaLambda(0.0)
    , m_sliceCurEndCUAddr(0)
    , m_nextSlice(false)
    , m_sliceBits(0)
    , m_sliceSegmentBits(0)
    , m_bFinalized(false)
    , m_tileOffstForMultES(0)
    , m_substreamSizes(NULL)
    , m_cabacInitFlag(false)
    , m_bLMvdL1Zero(false)
    , m_numEntryPointOffsets(0)
    , m_temporalLayerNonReferenceFlag(false)
    , m_enableTMVPFlag(true)
{
    m_numRefIdx[0] = m_numRefIdx[1] = 0;

    initEqualRef();

    for (Int idx = 0; idx < MAX_NUM_REF; idx++)
    {
        m_list1IdxToList0Idx[idx] = -1;
    }

    for (Int numCount = 0; numCount < MAX_NUM_REF; numCount++)
    {
        m_refPicList[0][numCount] = NULL;
        m_refPicList[1][numCount] = NULL;
        m_refPOCList[0][numCount] = 0;
        m_refPOCList[1][numCount] = 0;
    }

    resetWpScaling();
    initWpAcDcParam();
    m_saoEnabledFlag = false;
}

TComSlice::~TComSlice()
{
    delete[] m_substreamSizes;
    m_substreamSizes = NULL;
}

Void TComSlice::initSlice()
{
    m_numRefIdx[0] = 0;
    m_numRefIdx[1] = 0;

    m_colFromL0Flag = 1;

    m_colRefIdx = 0;
    initEqualRef();
    m_bCheckLDC = false;
    m_sliceQpDeltaCb = 0;
    m_sliceQpDeltaCr = 0;
    m_maxNumMergeCand = MRG_MAX_NUM_CANDS;

    m_bFinalized = false;

    m_tileByteLocation.clear();
    m_cabacInitFlag = false;
    m_numEntryPointOffsets = 0;
    m_enableTMVPFlag = true;
}

Bool TComSlice::getRapPicFlag()
{
    return getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL
           || getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_N_LP
           || getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_N_LP
           || getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_RADL
           || getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_LP
           || getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA;
}

/**
 - allocate table to contain substream sizes to be written to the slice header.
 .
 \param uiNumSubstreams Number of substreams -- the allocation will be this value - 1.
 */
Void  TComSlice::allocSubstreamSizes(UInt numSubstreams)
{
    delete[] m_substreamSizes;
    m_substreamSizes = new UInt[numSubstreams > 0 ? numSubstreams - 1 : 0];
}

Void  TComSlice::sortPicList(TComList<TComPic*>& picList)
{
    TComPic* picExtract;
    TComPic* picInsert;

    TComList<TComPic*>::iterator iterPicExtract;
    TComList<TComPic*>::iterator iterPicExtract_1;
    TComList<TComPic*>::iterator iterPicInsert;

    for (Int i = 1; i < (Int)(picList.size()); i++)
    {
        iterPicExtract = picList.begin();
        for (Int j = 0; j < i; j++)
        {
            iterPicExtract++;
        }

        picExtract = *(iterPicExtract);
        iterPicInsert = picList.begin();
        while (iterPicInsert != iterPicExtract)
        {
            picInsert = *(iterPicInsert);
            if (picInsert->getPOC() >= picExtract->getPOC())
            {
                break;
            }

            iterPicInsert++;
        }

        iterPicExtract_1 = iterPicExtract;
        iterPicExtract_1++;

        //  swap iterPicExtract and iterPicInsert, iterPicExtract = curr. / iterPicInsert = insertion position
        picList.insert(iterPicInsert, iterPicExtract, iterPicExtract_1);
        picList.erase(iterPicExtract);
    }
}

TComPic* TComSlice::xGetRefPic(TComList<TComPic*>& picList, Int poc)
{
    TComList<TComPic*>::iterator iterPic = picList.begin();
    TComPic* pic = NULL;

    while (iterPic != picList.end())
    {
        pic = *iterPic;
        if (pic->getPOC() == poc)
        {
            break;
        }
        iterPic++;
    }

    return pic;
}

TComPic* TComSlice::xGetLongTermRefPic(TComList<TComPic*>& picList, Int poc, Bool pocHasMsb)
{
    TComList<TComPic*>::iterator  iterPic = picList.begin();
    TComPic* pic = *(iterPic);
    TComPic* stPic = pic;

    Int pocCycle = 1 << getSPS()->getBitsForPOC();
    if (!pocHasMsb)
    {
        poc = poc % pocCycle;
    }

    while (iterPic != picList.end())
    {
        pic = *(iterPic);
        if (pic && pic->getPOC() != this->getPOC() && pic->getSlice()->isReferenced())
        {
            Int picPoc = pic->getPOC();
            if (!pocHasMsb)
            {
                picPoc = picPoc % pocCycle;
            }

            if (poc == picPoc)
            {
                if (pic->getIsLongTerm())
                {
                    return pic;
                }
                else
                {
                    stPic = pic;
                }
                break;
            }
        }

        iterPic++;
    }

    return stPic;
}

Void TComSlice::setRefPOCList()
{
    for (Int dir = 0; dir < 2; dir++)
    {
        for (Int numRefIdx = 0; numRefIdx < m_numRefIdx[dir]; numRefIdx++)
        {
            m_refPOCList[dir][numRefIdx] = m_refPicList[dir][numRefIdx]->getPOC();
        }
    }
}

Void TComSlice::setList1IdxToList0Idx()
{
    Int idxL0, idxL1;

    for (idxL1 = 0; idxL1 < getNumRefIdx(REF_PIC_LIST_1); idxL1++)
    {
        m_list1IdxToList0Idx[idxL1] = -1;
        for (idxL0 = 0; idxL0 < getNumRefIdx(REF_PIC_LIST_0); idxL0++)
        {
            if (m_refPicList[REF_PIC_LIST_0][idxL0]->getPOC() == m_refPicList[REF_PIC_LIST_1][idxL1]->getPOC())
            {
                m_list1IdxToList0Idx[idxL1] = idxL0;
                break;
            }
        }
    }
}

Void TComSlice::setRefPicList(TComList<TComPic*>& picList, Bool checkNumPocTotalCurr)
{
    if (!checkNumPocTotalCurr)
    {
        if (m_sliceType == I_SLICE)
        {
            ::memset(m_refPicList, 0, sizeof(m_refPicList));
            ::memset(m_numRefIdx,  0, sizeof(m_numRefIdx));

            return;
        }

        m_numRefIdx[0] = getNumRefIdx(REF_PIC_LIST_0);
        m_numRefIdx[1] = getNumRefIdx(REF_PIC_LIST_1);
    }

    TComPic* refPic = NULL;
    TComPic* refPicSetStCurr0[16];
    TComPic* refPicSetStCurr1[16];
    TComPic* refPicSetLtCurr[16];
    UInt numPocStCurr0 = 0;
    UInt numPocStCurr1 = 0;
    UInt numPocLtCurr = 0;
    Int i;

    for (i = 0; i < m_rps->getNumberOfNegativePictures(); i++)
    {
        if (m_rps->getUsed(i))
        {
            refPic = xGetRefPic(picList, getPOC() + m_rps->getDeltaPOC(i));
            refPic->setIsLongTerm(0);
            refPicSetStCurr0[numPocStCurr0] = refPic;
            numPocStCurr0++;
            refPic->setCheckLTMSBPresent(false);
        }
    }

    for (; i < m_rps->getNumberOfNegativePictures() + m_rps->getNumberOfPositivePictures(); i++)
    {
        if (m_rps->getUsed(i))
        {
            refPic = xGetRefPic(picList, getPOC() + m_rps->getDeltaPOC(i));
            refPic->setIsLongTerm(0);
            refPicSetStCurr1[numPocStCurr1] = refPic;
            numPocStCurr1++;
            refPic->setCheckLTMSBPresent(false);
        }
    }

    for (i = m_rps->getNumberOfNegativePictures() + m_rps->getNumberOfPositivePictures() + m_rps->getNumberOfLongtermPictures() - 1;
        i > m_rps->getNumberOfNegativePictures() + m_rps->getNumberOfPositivePictures() - 1; i--)
    {
        if (m_rps->getUsed(i))
        {
            refPic = xGetLongTermRefPic(picList, m_rps->getPOC(i), m_rps->getCheckLTMSBPresent(i));
            refPic->setIsLongTerm(1);
            refPicSetLtCurr[numPocLtCurr] = refPic;
            numPocLtCurr++;
        }
        if (refPic == NULL)
        {
            refPic = xGetLongTermRefPic(picList, m_rps->getPOC(i), m_rps->getCheckLTMSBPresent(i));
        }
        refPic->setCheckLTMSBPresent(m_rps->getCheckLTMSBPresent(i));
    }

    // ref_pic_list_init
    TComPic* rpsCurrList0[MAX_NUM_REF + 1];
    TComPic* rpsCurrList1[MAX_NUM_REF + 1];
    Int numPocTotalCurr = numPocStCurr0 + numPocStCurr1 + numPocLtCurr;
    if (checkNumPocTotalCurr)
    {
        // The variable NumPocTotalCurr is derived as specified in subclause 7.4.7.2. It is a requirement of bitstream conformance
        // that the following applies to the value of NumPocTotalCurr:
        // - If the current picture is a BLA or CRA picture, the value of NumPocTotalCurr shall be equal to 0.
        // - Otherwise, when the current picture contains a P or B slice, the value of NumPocTotalCurr shall not be equal to 0.
        if (getRapPicFlag())
        {
            assert(numPocTotalCurr == 0);
        }

        if (m_sliceType == I_SLICE)
        {
            ::memset(m_refPicList, 0, sizeof(m_refPicList));
            ::memset(m_numRefIdx,   0, sizeof(m_numRefIdx));

            return;
        }

        assert(numPocTotalCurr >= 0);

        m_numRefIdx[0] = getNumRefIdx(REF_PIC_LIST_0);
        m_numRefIdx[1] = getNumRefIdx(REF_PIC_LIST_1);
    }

    Int cIdx = 0;
    for (i = 0; i < numPocStCurr0; i++, cIdx++)
    {
        rpsCurrList0[cIdx] = refPicSetStCurr0[i];
    }

    for (i = 0; i < numPocStCurr1; i++, cIdx++)
    {
        rpsCurrList0[cIdx] = refPicSetStCurr1[i];
    }

    for (i = 0; i < numPocLtCurr; i++, cIdx++)
    {
        rpsCurrList0[cIdx] = refPicSetLtCurr[i];
    }

    assert(cIdx == numPocTotalCurr);

    if (m_sliceType == B_SLICE)
    {
        cIdx = 0;
        for (i = 0; i < numPocStCurr1; i++, cIdx++)
        {
            rpsCurrList1[cIdx] = refPicSetStCurr1[i];
        }

        for (i = 0; i < numPocStCurr0; i++, cIdx++)
        {
            rpsCurrList1[cIdx] = refPicSetStCurr0[i];
        }

        for (i = 0; i < numPocLtCurr; i++, cIdx++)
        {
            rpsCurrList1[cIdx] = refPicSetLtCurr[i];
        }

        assert(cIdx == numPocTotalCurr);
    }

    ::memset(m_bIsUsedAsLongTerm, 0, sizeof(m_bIsUsedAsLongTerm));

    for (Int rIdx = 0; rIdx < m_numRefIdx[0]; rIdx++)
    {
        cIdx = m_refPicListModification.getRefPicListModificationFlagL0() ? m_refPicListModification.getRefPicSetIdxL0(rIdx) : rIdx % numPocTotalCurr;
        assert(cIdx >= 0 && cIdx < numPocTotalCurr);
        m_refPicList[0][rIdx] = rpsCurrList0[cIdx];
        m_bIsUsedAsLongTerm[0][rIdx] = (cIdx >= numPocStCurr0 + numPocStCurr1);
    }

    if (m_sliceType != B_SLICE)
    {
        m_numRefIdx[1] = 0;
        ::memset(m_refPicList[1], 0, sizeof(m_refPicList[1]));
    }
    else
    {
        for (Int rIdx = 0; rIdx < m_numRefIdx[1]; rIdx++)
        {
            cIdx = m_refPicListModification.getRefPicListModificationFlagL1() ? m_refPicListModification.getRefPicSetIdxL1(rIdx) : rIdx % numPocTotalCurr;
            assert(cIdx >= 0 && cIdx < numPocTotalCurr);
            m_refPicList[1][rIdx] = rpsCurrList1[cIdx];
            m_bIsUsedAsLongTerm[1][rIdx] = (cIdx >= numPocStCurr0 + numPocStCurr1);
        }
    }
}

Int TComSlice::getNumRpsCurrTempList()
{
    Int numRpsCurrTempList = 0;

    if (m_sliceType == I_SLICE)
    {
        return 0;
    }
    for (UInt i = 0; i < m_rps->getNumberOfNegativePictures() + m_rps->getNumberOfPositivePictures() + m_rps->getNumberOfLongtermPictures(); i++)
    {
        if (m_rps->getUsed(i))
        {
            numRpsCurrTempList++;
        }
    }

    return numRpsCurrTempList;
}

Void TComSlice::initEqualRef()
{
    for (Int dir = 0; dir < 2; dir++)
    {
        for (Int refIdx1 = 0; refIdx1 < MAX_NUM_REF; refIdx1++)
        {
            for (Int refIdx2 = refIdx1; refIdx2 < MAX_NUM_REF; refIdx2++)
            {
                m_bEqualRef[dir][refIdx1][refIdx2] = m_bEqualRef[dir][refIdx2][refIdx1] = (refIdx1 == refIdx2 ? true : false);
            }
        }
    }
}

Void TComSlice::checkCRA(TComReferencePictureSet *rps, Int& pocCRA, Bool& prevRAPisBLA)
{
    for (Int i = 0; i < rps->getNumberOfNegativePictures() + rps->getNumberOfPositivePictures(); i++)
    {
        if (pocCRA < MAX_UINT && getPOC() > pocCRA)
        {
            assert(getPOC() + rps->getDeltaPOC(i) >= pocCRA);
        }
    }

    for (Int i = rps->getNumberOfNegativePictures() + rps->getNumberOfPositivePictures(); i < rps->getNumberOfPictures(); i++)
    {
        if (pocCRA < MAX_UINT && getPOC() > pocCRA)
        {
            if (!rps->getCheckLTMSBPresent(i))
            {
                //assert(xGetLongTermRefPic(picList, rps->getPOC(i), false)->getPOC() >= pocCRA);
            }
            else
            {
                assert(rps->getPOC(i) >= pocCRA);
            }
        }
    }

    if (getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL || getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_N_LP) // IDR picture found
    {
        pocCRA = getPOC();
        prevRAPisBLA = false;
    }
    else if (getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA) // CRA picture found
    {
        pocCRA = getPOC();
        prevRAPisBLA = false;
    }
    else if (getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_LP
             || getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_RADL
             || getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_N_LP) // BLA picture found
    {
        pocCRA = getPOC();
        prevRAPisBLA = true;
    }
}

Void TComSlice::copySliceInfo(TComSlice *src)
{
    assert(src != NULL);

    Int i, j, k;

    m_poc                 = src->m_poc;
    m_nalUnitType         = src->m_nalUnitType;
    m_sliceType           = src->m_sliceType;
    m_sliceQp             = src->m_sliceQp;
    m_sliceQpBase         = src->m_sliceQpBase;
    m_deblockingFilterDisable   = src->m_deblockingFilterDisable;
    m_deblockingFilterOverrideFlag = src->m_deblockingFilterOverrideFlag;
    m_deblockingFilterBetaOffsetDiv2 = src->m_deblockingFilterBetaOffsetDiv2;
    m_deblockingFilterTcOffsetDiv2 = src->m_deblockingFilterTcOffsetDiv2;

    for (i = 0; i < 2; i++)
    {
        m_numRefIdx[i] = src->m_numRefIdx[i];
    }

    for (i = 0; i < MAX_NUM_REF; i++)
    {
        m_list1IdxToList0Idx[i] = src->m_list1IdxToList0Idx[i];
    }

    m_bCheckLDC           = src->m_bCheckLDC;
    m_sliceQpDelta        = src->m_sliceQpDelta;
    m_sliceQpDeltaCb      = src->m_sliceQpDeltaCb;
    m_sliceQpDeltaCr      = src->m_sliceQpDeltaCr;
    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < MAX_NUM_REF; j++)
        {
            m_refPicList[i][j] = src->m_refPicList[i][j];
            m_refPOCList[i][j] = src->m_refPOCList[i][j];
        }
    }

    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < MAX_NUM_REF + 1; j++)
        {
            m_bIsUsedAsLongTerm[i][j] = src->m_bIsUsedAsLongTerm[i][j];
        }
    }

    m_depth = src->m_depth;

    // referenced slice
    m_bReferenced = src->m_bReferenced;

    // access channel
    m_sps = src->m_sps;
    m_pps = src->m_pps;
    m_rps = src->m_rps;
    m_lastIDR = src->m_lastIDR;

    m_pic = src->m_pic;

    m_colFromL0Flag = src->m_colFromL0Flag;
    m_colRefIdx = src->m_colRefIdx;
    m_lumaLambda = src->m_lumaLambda;
    m_chromaLambda = src->m_chromaLambda;
    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < MAX_NUM_REF; j++)
        {
            for (k = 0; k < MAX_NUM_REF; k++)
            {
                m_bEqualRef[i][j][k] = src->m_bEqualRef[i][j][k];
            }
        }
    }

    m_sliceCurEndCUAddr = src->m_sliceCurEndCUAddr;
    m_nextSlice = src->m_nextSlice;
    for (Int e = 0; e < 2; e++)
    {
        for (Int n = 0; n < MAX_NUM_REF; n++)
        {
            memcpy(m_weightPredTable[e][n], src->m_weightPredTable[e][n], sizeof(wpScalingParam) * 3);
        }
    }

    m_saoEnabledFlag = src->m_saoEnabledFlag;
    m_saoEnabledFlagChroma = src->m_saoEnabledFlagChroma;
    m_cabacInitFlag = src->m_cabacInitFlag;
    m_numEntryPointOffsets  = src->m_numEntryPointOffsets;

    m_bLMvdL1Zero = src->m_bLMvdL1Zero;
    m_enableTMVPFlag = src->m_enableTMVPFlag;
    m_maxNumMergeCand = src->m_maxNumMergeCand;
}

/** Function for applying picture marking based on the Reference Picture Set in pReferencePictureSet.
*/
Int TComSlice::checkThatAllRefPicsAreAvailable(TComList<TComPic*>& picList, TComReferencePictureSet *rps, Bool printErrors, Int pocRandomAccess)
{
    TComPic* outPic;
    Int i, isAvailable;
    Int atLeastOneLost = 0;
    Int atLeastOneRemoved = 0;
    Int iPocLost = 0;

    // loop through all long-term pictures in the Reference Picture Set
    // to see if the picture should be kept as reference picture
    for (i = rps->getNumberOfNegativePictures() + rps->getNumberOfPositivePictures(); i < rps->getNumberOfPictures(); i++)
    {
        isAvailable = 0;
        // loop through all pictures in the reference picture buffer
        TComList<TComPic*>::iterator iterPic = picList.begin();
        while (iterPic != picList.end())
        {
            outPic = *(iterPic++);
            if (rps->getCheckLTMSBPresent(i) == true)
            {
                if (outPic->getIsLongTerm() && (outPic->getPicSym()->getSlice()->getPOC()) == rps->getPOC(i) && outPic->getSlice()->isReferenced())
                {
                    isAvailable = 1;
                }
            }
            else
            {
                if (outPic->getIsLongTerm() && (outPic->getPicSym()->getSlice()->getPOC() % (1 << outPic->getPicSym()->getSlice()->getSPS()->getBitsForPOC())) == rps->getPOC(i) % (1 << outPic->getPicSym()->getSlice()->getSPS()->getBitsForPOC()) && outPic->getSlice()->isReferenced())
                {
                    isAvailable = 1;
                }
            }
        }

        // if there was no such long-term check the short terms
        if (!isAvailable)
        {
            iterPic = picList.begin();
            while (iterPic != picList.end())
            {
                outPic = *(iterPic++);

                Int pocCycle = 1 << outPic->getPicSym()->getSlice()->getSPS()->getBitsForPOC();
                Int curPoc = outPic->getPicSym()->getSlice()->getPOC();
                Int refPoc = rps->getPOC(i);
                if (!rps->getCheckLTMSBPresent(i))
                {
                    curPoc = curPoc % pocCycle;
                    refPoc = refPoc % pocCycle;
                }

                if (outPic->getSlice()->isReferenced() && curPoc == refPoc)
                {
                    isAvailable = 1;
                    outPic->setIsLongTerm(1);
                    break;
                }
            }
        }
        // report that a picture is lost if it is in the Reference Picture Set
        // but not available as reference picture
        if (isAvailable == 0)
        {
            if (this->getPOC() + rps->getDeltaPOC(i) >= pocRandomAccess)
            {
                if (!rps->getUsed(i))
                {
                    if (printErrors)
                    {
                        printf("\nLong-term reference picture with POC = %3d seems to have been removed or not correctly decoded.", this->getPOC() + rps->getDeltaPOC(i));
                    }
                    atLeastOneRemoved = 1;
                }
                else
                {
                    if (printErrors)
                    {
                        printf("\nLong-term reference picture with POC = %3d is lost or not correctly decoded!", this->getPOC() + rps->getDeltaPOC(i));
                    }
                    atLeastOneLost = 1;
                    iPocLost = this->getPOC() + rps->getDeltaPOC(i);
                }
            }
        }
    }

    // loop through all short-term pictures in the Reference Picture Set
    // to see if the picture should be kept as reference picture
    for (i = 0; i < rps->getNumberOfNegativePictures() + rps->getNumberOfPositivePictures(); i++)
    {
        isAvailable = 0;
        // loop through all pictures in the reference picture buffer
        TComList<TComPic*>::iterator iterPic = picList.begin();
        while (iterPic != picList.end())
        {
            outPic = *(iterPic++);

            if (!outPic->getIsLongTerm() && outPic->getPicSym()->getSlice()->getPOC() == this->getPOC() + rps->getDeltaPOC(i) &&
                outPic->getSlice()->isReferenced())
            {
                isAvailable = 1;
            }
        }

        // report that a picture is lost if it is in the Reference Picture Set
        // but not available as reference picture
        if (isAvailable == 0)
        {
            if (this->getPOC() + rps->getDeltaPOC(i) >= pocRandomAccess)
            {
                if (!rps->getUsed(i))
                {
                    if (printErrors)
                    {
                        printf("\nShort-term reference picture with POC = %3d seems to have been removed or not correctly decoded.",
                               this->getPOC() + rps->getDeltaPOC(i));
                    }
                    atLeastOneRemoved = 1;
                }
                else
                {
                    if (printErrors)
                    {
                        printf("\nShort-term reference picture with POC = %3d is lost or not correctly decoded!", this->getPOC() + rps->getDeltaPOC(i));
                    }
                    atLeastOneLost = 1;
                    iPocLost = this->getPOC() + rps->getDeltaPOC(i);
                }
            }
        }
    }

    if (atLeastOneLost)
    {
        return iPocLost + 1;
    }
    if (atLeastOneRemoved)
    {
        return -2;
    }
    else
    {
        return 0;
    }
}

/** Function for constructing an explicit Reference Picture Set out of the available pictures in a referenced Reference Picture Set
*/
Void TComSlice::createExplicitReferencePictureSetFromReference(TComList<TComPic*>& picList, TComReferencePictureSet *rps, Bool isRAP)
{
    TComPic* outPic;
    Int i, j;
    Int k = 0;
    Int nrOfNegativePictures = 0;
    Int nrOfPositivePictures = 0;
    TComReferencePictureSet* refRPS = this->getLocalRPS();

    // loop through all pictures in the Reference Picture Set
    for (i = 0; i < rps->getNumberOfPictures(); i++)
    {
        j = 0;
        // loop through all pictures in the reference picture buffer
        TComList<TComPic*>::iterator iterPic = picList.begin();
        while (iterPic != picList.end())
        {
            j++;
            outPic = *(iterPic++);

            if (outPic->getPicSym()->getSlice()->getPOC() == this->getPOC() + rps->getDeltaPOC(i) && outPic->getSlice()->isReferenced())
            {
                // This picture exists as a reference picture
                // and should be added to the explicit Reference Picture Set
                refRPS->setDeltaPOC(k, rps->getDeltaPOC(i));
                refRPS->setUsed(k, rps->getUsed(i) && (!isRAP));
                if (refRPS->getDeltaPOC(k) < 0)
                {
                    nrOfNegativePictures++;
                }
                else
                {
                    nrOfPositivePictures++;
                }
                k++;
            }
        }
    }

    refRPS->setNumberOfNegativePictures(nrOfNegativePictures);
    refRPS->setNumberOfPositivePictures(nrOfPositivePictures);
    refRPS->setNumberOfPictures(nrOfNegativePictures + nrOfPositivePictures);
    // This is a simplistic inter rps example. A smarter encoder will look for a better reference RPS to do the
    // inter RPS prediction with.  Here we just use the reference used by pReferencePictureSet.
    // If pReferencePictureSet is not inter_RPS_predicted, then inter_RPS_prediction is for the current RPS also disabled.
    if (!rps->getInterRPSPrediction())
    {
        refRPS->setInterRPSPrediction(false);
        refRPS->setNumRefIdc(0);
    }
    else
    {
        Int rIdx =  this->getRPSidx() - rps->getDeltaRIdxMinus1() - 1;
        Int deltaRPS = rps->getDeltaRPS();
        const TComReferencePictureSet *refRPSOther = this->getSPS()->getRPSList()->getReferencePictureSet(rIdx);
        Int refPics = refRPSOther->getNumberOfPictures();
        Int newIdc = 0;
        for (i = 0; i <= refPics; i++)
        {
            Int deltaPOC = ((i != refPics) ? refRPSOther->getDeltaPOC(i) : 0); // check if the reference abs POC is >= 0
            Int refIdc = 0;
            for (j = 0; j < refRPS->getNumberOfPictures(); j++) // loop through the  pictures in the new RPS
            {
                if ((deltaPOC + deltaRPS) == refRPS->getDeltaPOC(j))
                {
                    if (refRPS->getUsed(j))
                    {
                        refIdc = 1;
                    }
                    else
                    {
                        refIdc = 2;
                    }
                }
            }

            refRPS->setRefIdc(i, refIdc);
            newIdc++;
        }

        refRPS->setInterRPSPrediction(true);
        refRPS->setNumRefIdc(newIdc);
        refRPS->setDeltaRPS(deltaRPS);
        refRPS->setDeltaRIdxMinus1(rps->getDeltaRIdxMinus1() + this->getSPS()->getRPSList()->getNumberOfReferencePictureSets() - this->getRPSidx());
    }

    this->setRPS(refRPS);
    this->setRPSidx(-1);
}

/** get AC and DC values for weighted pred
 * \param *wp
 * \returns Void
 */
Void TComSlice::getWpAcDcParam(wpACDCParam *&wp)
{
    wp = m_weightACDCParam;
}

/** init AC and DC values for weighted pred
 * \returns Void
 */
Void TComSlice::initWpAcDcParam()
{
    for (Int comp = 0; comp < 3; comp++)
    {
        m_weightACDCParam[comp].ac = 0;
        m_weightACDCParam[comp].dc = 0;
    }
}

/** get WP tables for weighted pred
 * \param RefPicList
 * \param refIdx
 * \param *&wpScalingParam
 * \returns Void
 */
Void TComSlice::getWpScaling(RefPicList l, Int refIdx, wpScalingParam *&wp)
{
    wp = m_weightPredTable[l][refIdx];
}

/** reset Default WP tables settings : no weight.
 * \param wpScalingParam
 * \returns Void
 */
Void  TComSlice::resetWpScaling()
{
    for (Int e = 0; e < 2; e++)
    {
        for (Int i = 0; i < MAX_NUM_REF; i++)
        {
            for (Int yuv = 0; yuv < 3; yuv++)
            {
                wpScalingParam  *pwp = &(m_weightPredTable[e][i][yuv]);
                pwp->bPresentFlag    = false;
                pwp->log2WeightDenom = 0;
                pwp->log2WeightDenom = 0;
                pwp->inputWeight     = 1;
                pwp->inputOffset     = 0;
            }
        }
    }
}

/** init WP table
 * \returns Void
 */
Void  TComSlice::initWpScaling()
{
    for (Int e = 0; e < 2; e++)
    {
        for (Int i = 0; i < MAX_NUM_REF; i++)
        {
            for (Int yuv = 0; yuv < 3; yuv++)
            {
                wpScalingParam  *pwp = &(m_weightPredTable[e][i][yuv]);
                if (!pwp->bPresentFlag)
                {
                    // Inferring values not present :
                    pwp->inputWeight = (1 << pwp->log2WeightDenom);
                    pwp->inputOffset = 0;
                }

                pwp->w = pwp->inputWeight;
                pwp->o      = pwp->inputOffset << (X265_DEPTH - 8);
                pwp->shift  = pwp->log2WeightDenom;
                pwp->round  = (pwp->log2WeightDenom >= 1) ? (1 << (pwp->log2WeightDenom - 1)) : (0);
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Video parameter set (VPS)
// ------------------------------------------------------------------------------------------------
TComVPS::TComVPS()
    : m_VPSId(0)
    , m_uiMaxTLayers(1)
    , m_maxLayers(1)
    , m_bTemporalIdNestingFlag(false)
    , m_numHrdParameters(0)
    , m_maxNuhReservedZeroLayerId(0)
    , m_hrdParameters(NULL)
    , m_hrdOpSetIdx(NULL)
    , m_cprmsPresentFlag(NULL)
{
    for (Int i = 0; i < MAX_TLAYER; i++)
    {
        m_numReorderPics[i] = 0;
        m_maxDecPicBuffering[i] = 1;
        m_maxLatencyIncrease[i] = 0;
    }
}

TComVPS::~TComVPS()
{
    if (m_hrdParameters    != NULL) delete[] m_hrdParameters;
    if (m_hrdOpSetIdx      != NULL) delete[] m_hrdOpSetIdx;
    if (m_cprmsPresentFlag != NULL) delete[] m_cprmsPresentFlag;
}

// ------------------------------------------------------------------------------------------------
// Sequence parameter set (SPS)
// ------------------------------------------------------------------------------------------------

TComSPS::TComSPS()
    : m_SPSId(0)
    , m_VPSId(0)
    , m_chromaFormatIdc(CHROMA_420)
    , m_maxTLayers(1)
// Structure
    , m_picWidthInLumaSamples(352)
    , m_picHeightInLumaSamples(288)
    , m_log2MinCodingBlockSize(0)
    , m_log2DiffMaxMinCodingBlockSize(0)
    , m_maxCUWidth(32)
    , m_maxCUHeight(32)
    , m_maxCUDepth(3)
    , m_bLongTermRefsPresent(false)
    , m_quadtreeTULog2MaxSize(0)
    , m_quadtreeTULog2MinSize(0)
    , m_quadtreeTUMaxDepthInter(0)
    , m_quadtreeTUMaxDepthIntra(0)
// Tool list
    , m_usePCM(false)
    , m_pcmLog2MaxSize(5)
    , m_pcmLog2MinSize(7)
    , m_bitDepthY(8)
    , m_bitDepthC(8)
    , m_qpBDOffsetY(0)
    , m_qpBDOffsetC(0)
    , m_useLossless(false)
    , m_pcmBitDepthLuma(8)
    , m_pcmBitDepthChroma(8)
    , m_bPCMFilterDisableFlag(false)
    , m_bitsForPOC(8)
    , m_numLongTermRefPicSPS(0)
    , m_maxTrSize(32)
    , m_bUseSAO(false)
    , m_bTemporalIdNestingFlag(false)
    , m_scalingListEnabledFlag(false)
    , m_useStrongIntraSmoothing(false)
    , m_vuiParametersPresentFlag(false)
    , m_vuiParameters()
{
    for (Int i = 0; i < MAX_TLAYER; i++)
    {
        m_maxLatencyIncrease[i] = 0;
        m_maxDecPicBuffering[i] = 1;
        m_numReorderPics[i]       = 0;
    }

    m_scalingList = new TComScalingList;
    ::memset(m_ltRefPicPocLsbSps, 0, sizeof(m_ltRefPicPocLsbSps));
    ::memset(m_usedByCurrPicLtSPSFlag, 0, sizeof(m_usedByCurrPicLtSPSFlag));
}

TComSPS::~TComSPS()
{
    delete m_scalingList;
    m_RPSList.destroy();
}

Void  TComSPS::createRPSList(Int numRPS)
{
    m_RPSList.destroy();
    m_RPSList.create(numRPS);
}

Void TComSPS::setHrdParameters(UInt frameRate, UInt numDU, UInt bitRate, Bool randomAccess)
{
    if (!getVuiParametersPresentFlag())
    {
        return;
    }

    TComVUI *vui = getVuiParameters();
    TComHRD *hrd = vui->getHrdParameters();

    TimingInfo *timingInfo = vui->getTimingInfo();
    timingInfo->setTimingInfoPresentFlag(true);
    switch (frameRate)
    {
    case 24:
        timingInfo->setNumUnitsInTick(1125000);
        timingInfo->setTimeScale(27000000);
        break;
    case 25:
        timingInfo->setNumUnitsInTick(1080000);
        timingInfo->setTimeScale(27000000);
        break;
    case 30:
        timingInfo->setNumUnitsInTick(900900);
        timingInfo->setTimeScale(27000000);
        break;
    case 50:
        timingInfo->setNumUnitsInTick(540000);
        timingInfo->setTimeScale(27000000);
        break;
    case 60:
        timingInfo->setNumUnitsInTick(450450);
        timingInfo->setTimeScale(27000000);
        break;
    default:
        timingInfo->setNumUnitsInTick(1001);
        timingInfo->setTimeScale(60000);
        break;
    }

    Bool rateCnt = (bitRate > 0);
    hrd->setNalHrdParametersPresentFlag(rateCnt);
    hrd->setVclHrdParametersPresentFlag(rateCnt);

    hrd->setSubPicCpbParamsPresentFlag((numDU > 1));

    if (hrd->getSubPicCpbParamsPresentFlag())
    {
        hrd->setTickDivisorMinus2(100 - 2);
        hrd->setDuCpbRemovalDelayLengthMinus1(7);                  // 8-bit precision ( plus 1 for last DU in AU )
        hrd->setSubPicCpbParamsInPicTimingSEIFlag(true);
        hrd->setDpbOutputDelayDuLengthMinus1(5 + 7);               // With sub-clock tick factor of 100, at least 7 bits to have the same value as AU dpb delay
    }
    else
    {
        hrd->setSubPicCpbParamsInPicTimingSEIFlag(false);
    }

    hrd->setBitRateScale(4);                                       // in units of 2~( 6 + 4 ) = 1,024 bps
    hrd->setCpbSizeScale(6);                                       // in units of 2~( 4 + 4 ) = 1,024 bit
    hrd->setDuCpbSizeScale(6);                                     // in units of 2~( 4 + 4 ) = 1,024 bit

    hrd->setInitialCpbRemovalDelayLengthMinus1(15);                // assuming 0.5 sec, log2( 90,000 * 0.5 ) = 16-bit
    if (randomAccess)
    {
        hrd->setCpbRemovalDelayLengthMinus1(5);                    // 32 = 2^5 (plus 1)
        hrd->setDpbOutputDelayLengthMinus1(5);                     // 32 + 3 = 2^6
    }
    else
    {
        hrd->setCpbRemovalDelayLengthMinus1(9);                    // max. 2^10
        hrd->setDpbOutputDelayLengthMinus1(9);                     // max. 2^10
    }

/*
   Note: only the case of "vps_max_temporal_layers_minus1 = 0" is supported.
*/
    Int i, j;
    UInt birateValue, cpbSizeValue;
    UInt ducpbSizeValue;
    UInt duBitRateValue = 0;

    for (i = 0; i < MAX_TLAYER; i++)
    {
        hrd->setFixedPicRateFlag(i, 1);
        hrd->setPicDurationInTcMinus1(i, 0);
        hrd->setLowDelayHrdFlag(i, 0);
        hrd->setCpbCntMinus1(i, 0);

        birateValue  = bitRate;
        cpbSizeValue = bitRate;                                 // 1 second
        ducpbSizeValue = bitRate / numDU;
        duBitRateValue = bitRate;
        for (j = 0; j < (hrd->getCpbCntMinus1(i) + 1); j++)
        {
            hrd->setBitRateValueMinus1(i, j, 0, (birateValue  - 1));
            hrd->setCpbSizeValueMinus1(i, j, 0, (cpbSizeValue - 1));
            hrd->setDuCpbSizeValueMinus1(i, j, 0, (ducpbSizeValue - 1));
            hrd->setCbrFlag(i, j, 0, (j == 0));

            hrd->setBitRateValueMinus1(i, j, 1, (birateValue  - 1));
            hrd->setCpbSizeValueMinus1(i, j, 1, (cpbSizeValue - 1));
            hrd->setDuCpbSizeValueMinus1(i, j, 1, (ducpbSizeValue - 1));
            hrd->setDuBitRateValueMinus1(i, j, 1, (duBitRateValue - 1));
            hrd->setCbrFlag(i, j, 1, (j == 0));
        }
    }
}

TComPPS::TComPPS()
    : m_PPSId(0)
    , m_SPSId(0)
    , m_picInitQPMinus26(0)
    , m_useDQP(false)
    , m_bConstrainedIntraPred(false)
    , m_bSliceChromaQpFlag(false)
    , m_sps(NULL)
    , m_maxCuDQPDepth(0)
    , m_minCuDQPSize(0)
    , m_chromaCbQpOffset(0)
    , m_chromaCrQpOffset(0)
    , m_numRefIdxL0DefaultActive(1)
    , m_numRefIdxL1DefaultActive(1)
    , m_transquantBypassEnableFlag(false)
    , m_useTransformSkip(false)
    , m_entropyCodingSyncEnabledFlag(false)
    , m_loopFilterAcrossTilesEnabledFlag(true)
    , m_signHideFlag(0)
    , m_cabacInitPresentFlag(false)
    , m_encCABACTableIdx(I_SLICE)
    , m_sliceHeaderExtensionPresentFlag(false)
    , m_listsModificationPresentFlag(0)
    , m_numExtraSliceHeaderBits(0)
{
    m_scalingList = new TComScalingList;
}

TComPPS::~TComPPS()
{
    delete m_scalingList;
}

TComReferencePictureSet::TComReferencePictureSet()
    : m_deltaRIdxMinus1(0)
    , m_deltaRPS(0)
    , m_numRefIdc(0)
    , m_numberOfPictures(0)
    , m_numberOfNegativePictures(0)
    , m_numberOfPositivePictures(0)
    , m_interRPSPrediction(0)
    , m_numberOfLongtermPictures(0)
{
    ::memset(m_deltaPOC, 0, sizeof(m_deltaPOC));
    ::memset(m_POC, 0, sizeof(m_POC));
    ::memset(m_used, 0, sizeof(m_used));
    ::memset(m_refIdc, 0, sizeof(m_refIdc));
}

TComReferencePictureSet::~TComReferencePictureSet()
{}

Void TComReferencePictureSet::setUsed(Int bufferNum, Bool used)
{
    m_used[bufferNum] = used;
}

Void TComReferencePictureSet::setDeltaPOC(Int bufferNum, Int deltaPOC)
{
    m_deltaPOC[bufferNum] = deltaPOC;
}

Void TComReferencePictureSet::setNumberOfPictures(Int numberOfPictures)
{
    m_numberOfPictures = numberOfPictures;
}

Int TComReferencePictureSet::getUsed(Int bufferNum) const
{
    return m_used[bufferNum];
}

Int TComReferencePictureSet::getDeltaPOC(Int bufferNum) const
{
    return m_deltaPOC[bufferNum];
}

Int TComReferencePictureSet::getNumberOfPictures() const
{
    return m_numberOfPictures;
}

Int TComReferencePictureSet::getPOC(Int bufferNum) const
{
    return m_POC[bufferNum];
}

Void TComReferencePictureSet::setPOC(Int bufferNum, Int POC)
{
    m_POC[bufferNum] = POC;
}

Bool TComReferencePictureSet::getCheckLTMSBPresent(Int bufferNum)
{
    return m_bCheckLTMSB[bufferNum];
}

Void TComReferencePictureSet::setCheckLTMSBPresent(Int bufferNum, Bool b)
{
    m_bCheckLTMSB[bufferNum] = b;
}

/** set the reference idc value at uiBufferNum entry to the value of iRefIdc
 * \param uiBufferNum
 * \param iRefIdc
 * \returns Void
 */
Void TComReferencePictureSet::setRefIdc(Int bufferNum, Int refIdc)
{
    m_refIdc[bufferNum] = refIdc;
}

/** get the reference idc value at uiBufferNum
 * \param uiBufferNum
 * \returns Int
 */
Int  TComReferencePictureSet::getRefIdc(Int bufferNum) const
{
    return m_refIdc[bufferNum];
}

/** Sorts the deltaPOC and Used by current values in the RPS based on the deltaPOC values.
 *  deltaPOC values are sorted with -ve values before the +ve values.  -ve values are in decreasing order.
 *  +ve values are in increasing order.
 * \returns Void
 */
Void TComReferencePictureSet::sortDeltaPOC()
{
    // sort in increasing order (smallest first)
    for (Int j = 1; j < getNumberOfPictures(); j++)
    {
        Int deltaPOC = getDeltaPOC(j);
        Bool used = getUsed(j);
        for (Int k = j - 1; k >= 0; k--)
        {
            Int temp = getDeltaPOC(k);
            if (deltaPOC < temp)
            {
                setDeltaPOC(k + 1, temp);
                setUsed(k + 1, getUsed(k));
                setDeltaPOC(k, deltaPOC);
                setUsed(k, used);
            }
        }
    }

    // flip the negative values to largest first
    Int numNegPics = getNumberOfNegativePictures();
    for (Int j = 0, k = numNegPics - 1; j < numNegPics >> 1; j++, k--)
    {
        Int deltaPOC = getDeltaPOC(j);
        Bool used = getUsed(j);
        setDeltaPOC(j, getDeltaPOC(k));
        setUsed(j, getUsed(k));
        setDeltaPOC(k, deltaPOC);
        setUsed(k, used);
    }
}

/** Prints the deltaPOC and RefIdc (if available) values in the RPS.
 *  A "*" is added to the deltaPOC value if it is Used bu current.
 * \returns Void
 */
Void TComReferencePictureSet::printDeltaPOC()
{
    printf("DeltaPOC = { ");
    for (Int j = 0; j < getNumberOfPictures(); j++)
    {
        printf("%d%s ", getDeltaPOC(j), (getUsed(j) == 1) ? "*" : "");
    }

    if (getInterRPSPrediction())
    {
        printf("}, RefIdc = { ");
        for (Int j = 0; j < getNumRefIdc(); j++)
        {
            printf("%d ", getRefIdc(j));
        }
    }
    printf("}\n");
}

TComRPSList::TComRPSList()
    : m_referencePictureSets(NULL)
{}

TComRPSList::~TComRPSList()
{}

Void TComRPSList::create(Int numberOfReferencePictureSets)
{
    m_numberOfReferencePictureSets = numberOfReferencePictureSets;
    m_referencePictureSets = new TComReferencePictureSet[numberOfReferencePictureSets];
}

Void TComRPSList::destroy()
{
    if (m_referencePictureSets)
    {
        delete [] m_referencePictureSets;
    }
    m_numberOfReferencePictureSets = 0;
    m_referencePictureSets = NULL;
}

TComReferencePictureSet* TComRPSList::getReferencePictureSet(Int referencePictureSetNum)
{
    return &m_referencePictureSets[referencePictureSetNum];
}

Int TComRPSList::getNumberOfReferencePictureSets() const
{
    return m_numberOfReferencePictureSets;
}

Void TComRPSList::setNumberOfReferencePictureSets(Int numberOfReferencePictureSets)
{
    m_numberOfReferencePictureSets = numberOfReferencePictureSets;
}

TComRefPicListModification::TComRefPicListModification()
    : m_bRefPicListModificationFlagL0(false)
    , m_bRefPicListModificationFlagL1(false)
{
    ::memset(m_RefPicSetIdxL0, 0, sizeof(m_RefPicSetIdxL0));
    ::memset(m_RefPicSetIdxL1, 0, sizeof(m_RefPicSetIdxL1));
}

TComRefPicListModification::~TComRefPicListModification()
{}

TComScalingList::TComScalingList()
{
    m_useTransformSkip = false;
    init();
}

TComScalingList::~TComScalingList()
{
    destroy();
}

/** set default quantization matrix to array
*/
Void TComSlice::setDefaultScalingList()
{
    for (UInt sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
    {
        for (UInt listId = 0; listId < g_scalingListNum[sizeId]; listId++)
        {
            getScalingList()->processDefaultMarix(sizeId, listId);
        }
    }
}

/** check if use default quantization matrix
 * \returns true if use default quantization matrix in all size
*/
Bool TComSlice::checkDefaultScalingList()
{
    UInt defaultCounter = 0;

    for (UInt sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
    {
        for (UInt listId = 0; listId < g_scalingListNum[sizeId]; listId++)
        {
            if (!memcmp(getScalingList()->getScalingListAddress(sizeId, listId), getScalingList()->getScalingListDefaultAddress(sizeId, listId), sizeof(Int) * min(MAX_MATRIX_COEF_NUM, (Int)g_scalingListSize[sizeId])) // check value of matrix
                && ((sizeId < SCALING_LIST_16x16) || (getScalingList()->getScalingListDC(sizeId, listId) == 16))) // check DC value
            {
                defaultCounter++;
            }
        }
    }

    return (defaultCounter == (SCALING_LIST_NUM * SCALING_LIST_SIZE_NUM - 4)) ? false : true; // -4 for 32x32
}

/** get scaling matrix from RefMatrixID
 * \param sizeId size index
 * \param Index of input matrix
 * \param Index of reference matrix
 */
Void TComScalingList::processRefMatrix(UInt sizeId, UInt listId, UInt refListId)
{
    ::memcpy(getScalingListAddress(sizeId, listId), ((listId == refListId) ? getScalingListDefaultAddress(sizeId, refListId) : getScalingListAddress(sizeId, refListId)), sizeof(Int) * min(MAX_MATRIX_COEF_NUM, (Int)g_scalingListSize[sizeId]));
}

/** parse syntax information
 *  \param pchFile syntax information
 *  \returns false if successful
 */
Bool TComScalingList::xParseScalingList(Char* pchFile)
{
    FILE *fp;
    Char line[1024];
    UInt sizeIdc, listIdc;
    UInt i, size = 0;
    Int *src = 0, data;
    Char *ret;
    UInt  retval;

    if ((fp = fopen(pchFile, "r")) == (FILE*)NULL)
    {
        printf("can't open file %s :: set Default Matrix\n", pchFile);
        return true;
    }

    for (sizeIdc = 0; sizeIdc < SCALING_LIST_SIZE_NUM; sizeIdc++)
    {
        size = min(MAX_MATRIX_COEF_NUM, (Int)g_scalingListSize[sizeIdc]);
        for (listIdc = 0; listIdc < g_scalingListNum[sizeIdc]; listIdc++)
        {
            src = getScalingListAddress(sizeIdc, listIdc);

            fseek(fp, 0, 0);
            do
            {
                ret = fgets(line, 1024, fp);
                if ((ret == NULL) || (strstr(line, MatrixType[sizeIdc][listIdc]) == NULL && feof(fp)))
                {
                    printf("Error: can't read Matrix :: set Default Matrix\n");
                    return true;
                }
            }
            while (strstr(line, MatrixType[sizeIdc][listIdc]) == NULL);
            for (i = 0; i < size; i++)
            {
                retval = fscanf(fp, "%d,", &data);
                if (retval != 1)
                {
                    printf("Error: can't read Matrix :: set Default Matrix\n");
                    return true;
                }
                src[i] = data;
            }

            //set DC value for default matrix check
            setScalingListDC(sizeIdc, listIdc, src[0]);

            if (sizeIdc > SCALING_LIST_8x8)
            {
                fseek(fp, 0, 0);
                do
                {
                    ret = fgets(line, 1024, fp);
                    if ((ret == NULL) || (strstr(line, MatrixType_DC[sizeIdc][listIdc]) == NULL && feof(fp)))
                    {
                        printf("Error: can't read DC :: set Default Matrix\n");
                        return true;
                    }
                }
                while (strstr(line, MatrixType_DC[sizeIdc][listIdc]) == NULL);
                retval = fscanf(fp, "%d,", &data);
                if (retval != 1)
                {
                    printf("Error: can't read Matrix :: set Default Matrix\n");
                    return true;
                }
                //overwrite DC value when size of matrix is larger than 16x16
                setScalingListDC(sizeIdc, listIdc, data);
            }
        }
    }

    fclose(fp);
    return false;
}

/** initialization process of quantization matrix array
 */
Void TComScalingList::init()
{
    for (UInt sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
    {
        for (UInt listId = 0; listId < g_scalingListNum[sizeId]; listId++)
        {
            m_scalingListCoef[sizeId][listId] = new Int[min(MAX_MATRIX_COEF_NUM, (Int)g_scalingListSize[sizeId])];
        }
    }

    m_scalingListCoef[SCALING_LIST_32x32][3] = m_scalingListCoef[SCALING_LIST_32x32][1]; // copy address for 32x32
}

/** destroy quantization matrix array
 */
Void TComScalingList::destroy()
{
    for (UInt sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
    {
        for (UInt listId = 0; listId < g_scalingListNum[sizeId]; listId++)
        {
            if (m_scalingListCoef[sizeId][listId]) delete [] m_scalingListCoef[sizeId][listId];
        }
    }
}

/** get default address of quantization matrix
 * \param sizeId size index
 * \param listId list index
 * \returns pointer of quantization matrix
 */
Int* TComScalingList::getScalingListDefaultAddress(UInt sizeId, UInt listId)
{
    Int *src = 0;

    switch (sizeId)
    {
    case SCALING_LIST_4x4:
        src = g_quantTSDefault4x4;
        break;
    case SCALING_LIST_8x8:
        src = (listId < 3) ? g_quantIntraDefault8x8 : g_quantInterDefault8x8;
        break;
    case SCALING_LIST_16x16:
        src = (listId < 3) ? g_quantIntraDefault8x8 : g_quantInterDefault8x8;
        break;
    case SCALING_LIST_32x32:
        src = (listId < 1) ? g_quantIntraDefault8x8 : g_quantInterDefault8x8;
        break;
    default:
        assert(0);
        src = NULL;
        break;
    }

    return src;
}

/** process of default matrix
 * \param sizeId size index
 * \param Index of input matrix
 */
Void TComScalingList::processDefaultMarix(UInt sizeId, UInt listId)
{
    ::memcpy(getScalingListAddress(sizeId, listId), getScalingListDefaultAddress(sizeId, listId), sizeof(Int) * min(MAX_MATRIX_COEF_NUM, (Int)g_scalingListSize[sizeId]));
    setScalingListDC(sizeId, listId, SCALING_LIST_DC);
}

/** check DC value of matrix for default matrix signaling
 */
Void TComScalingList::checkDcOfMatrix()
{
    for (UInt sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
    {
        for (UInt listId = 0; listId < g_scalingListNum[sizeId]; listId++)
        {
            //check default matrix?
            if (getScalingListDC(sizeId, listId) == 0)
            {
                processDefaultMarix(sizeId, listId);
            }
        }
    }
}

ParameterSetManager::ParameterSetManager()
    : m_vpsMap(MAX_NUM_VPS)
    , m_spsMap(MAX_NUM_SPS)
    , m_ppsMap(MAX_NUM_PPS)
    , m_activeVPSId(-1)
    , m_activeSPSId(-1)
    , m_activePPSId(-1)
{}

ParameterSetManager::~ParameterSetManager()
{}

//! activate a SPS from a active parameter sets SEI message
//! \returns true, if activation is successful
Bool ParameterSetManager::activateSPSWithSEI(Int spsId)
{
    TComSPS *sps = m_spsMap.getPS(spsId);

    if (sps)
    {
        Int vpsId = sps->getVPSId();
        if (m_vpsMap.getPS(vpsId))
        {
            m_activeVPSId = vpsId;
            m_activeSPSId = spsId;
            return true;
        }
        else
        {
            printf("Warning: tried to activate SPS using an Active parameter sets SEI message. Referenced VPS does not exist.");
        }
    }
    else
    {
        printf("Warning: tried to activate non-existing SPS using an Active parameter sets SEI message.");
    }
    return false;
}

//! activate a PPS and depending on isIDR parameter also SPS and VPS
//! \returns true, if activation is successful
Bool ParameterSetManager::activatePPS(Int ppsId, Bool isIRAP)
{
    TComPPS *pps = m_ppsMap.getPS(ppsId);

    if (pps)
    {
        Int spsId = pps->getSPSId();
        if (!isIRAP && (spsId != m_activeSPSId))
        {
            printf("Warning: tried to activate PPS referring to a inactive SPS at non-IRAP.");
            return false;
        }
        TComSPS *sps = m_spsMap.getPS(spsId);
        if (sps)
        {
            Int vpsId = sps->getVPSId();
            if (!isIRAP && (vpsId != m_activeVPSId))
            {
                printf("Warning: tried to activate PPS referring to a inactive VPS at non-IRAP.");
                return false;
            }
            if (m_vpsMap.getPS(vpsId))
            {
                m_activePPSId = ppsId;
                m_activeVPSId = vpsId;
                m_activeSPSId = spsId;
                return true;
            }
            else
            {
                printf("Warning: tried to activate PPS that refers to a non-existing VPS.");
            }
        }
        else
        {
            printf("Warning: tried to activate a PPS that refers to a non-existing SPS.");
        }
    }
    else
    {
        printf("Warning: tried to activate non-existing PPS.");
    }
    return false;
}

ProfileTierLevel::ProfileTierLevel()
    : m_profileSpace(0)
    , m_tierFlag(false)
    , m_profileIdc(0)
    , m_levelIdc(0)
    , m_progressiveSourceFlag(false)
    , m_interlacedSourceFlag(false)
    , m_nonPackedConstraintFlag(false)
    , m_frameOnlyConstraintFlag(false)
{
    ::memset(m_profileCompatibilityFlag, 0, sizeof(m_profileCompatibilityFlag));
}

TComPTL::TComPTL()
{
    ::memset(m_subLayerProfilePresentFlag, 0, sizeof(m_subLayerProfilePresentFlag));
    ::memset(m_subLayerLevelPresentFlag,   0, sizeof(m_subLayerLevelPresentFlag));
}

//! \}
