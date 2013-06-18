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
    : m_iPPSId(-1)
    , m_iPOC(0)
    , m_iLastIDR(0)
    , m_eNalUnitType(NAL_UNIT_CODED_SLICE_IDR_W_RADL)
    , m_eSliceType(I_SLICE)
    , m_iSliceQp(0)
    , m_dependentSliceSegmentFlag(false)
    , m_iSliceQpBase(0)
    , m_deblockingFilterDisable(false)
    , m_deblockingFilterOverrideFlag(false)
    , m_deblockingFilterBetaOffsetDiv2(0)
    , m_deblockingFilterTcOffsetDiv2(0)
    , m_bCheckLDC(false)
    , m_iSliceQpDelta(0)
    , m_iSliceQpDeltaCb(0)
    , m_iSliceQpDeltaCr(0)
    , m_iDepth(0)
    , m_bReferenced(false)
    , m_pcSPS(NULL)
    , m_pcPPS(NULL)
    , m_pcPic(NULL)
    , m_colFromL0Flag(1)
    , m_colRefIdx(0)
    , m_dLambdaLuma(0.0)
    , m_dLambdaChroma(0.0)
    , m_uiTLayer(0)
    , m_bTLayerSwitchingFlag(false)
    , m_sliceCurEndCUAddr(0)
    , m_nextSlice(false)
    , m_sliceBits(0)
    , m_sliceSegmentBits(0)
    , m_bFinalized(false)
    , m_uiTileOffstForMultES(0)
    , m_puiSubstreamSizes(NULL)
    , m_cabacInitFlag(false)
    , m_bLMvdL1Zero(false)
    , m_numEntryPointOffsets(0)
    , m_temporalLayerNonReferenceFlag(false)
    , m_enableTMVPFlag(true)
{
    m_aiNumRefIdx[0] = m_aiNumRefIdx[1] = 0;

    initEqualRef();

    for (Int idx = 0; idx < MAX_NUM_REF; idx++)
    {
        m_list1IdxToList0Idx[idx] = -1;
    }

    for (Int iNumCount = 0; iNumCount < MAX_NUM_REF; iNumCount++)
    {
        m_apcRefPicList[0][iNumCount] = NULL;
        m_apcRefPicList[1][iNumCount] = NULL;
        m_aiRefPOCList[0][iNumCount] = 0;
        m_aiRefPOCList[1][iNumCount] = 0;
    }

    resetWpScaling();
    initWpAcDcParam();
    m_saoEnabledFlag = false;
}

TComSlice::~TComSlice()
{
    delete[] m_puiSubstreamSizes;
    m_puiSubstreamSizes = NULL;
}

Void TComSlice::initSlice()
{
    m_aiNumRefIdx[0]      = 0;
    m_aiNumRefIdx[1]      = 0;

    m_colFromL0Flag = 1;

    m_colRefIdx = 0;
    initEqualRef();
    m_bCheckLDC = false;
    m_iSliceQpDeltaCb = 0;
    m_iSliceQpDeltaCr = 0;
    m_maxNumMergeCand = MRG_MAX_NUM_CANDS;

    m_bFinalized = false;

    m_tileByteLocation.clear();
    m_cabacInitFlag        = false;
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
Void  TComSlice::allocSubstreamSizes(UInt uiNumSubstreams)
{
    delete[] m_puiSubstreamSizes;
    m_puiSubstreamSizes = new UInt[uiNumSubstreams > 0 ? uiNumSubstreams - 1 : 0];
}

Void  TComSlice::sortPicList(TComList<TComPic*>& rcListPic)
{
    TComPic*    pcPicExtract;
    TComPic*    pcPicInsert;

    TComList<TComPic*>::iterator    iterPicExtract;
    TComList<TComPic*>::iterator    iterPicExtract_1;
    TComList<TComPic*>::iterator    iterPicInsert;

    for (Int i = 1; i < (Int)(rcListPic.size()); i++)
    {
        iterPicExtract = rcListPic.begin();
        for (Int j = 0; j < i; j++)
        {
            iterPicExtract++;
        }

        pcPicExtract = *(iterPicExtract);
        iterPicInsert = rcListPic.begin();
        while (iterPicInsert != iterPicExtract)
        {
            pcPicInsert = *(iterPicInsert);
            if (pcPicInsert->getPOC() >= pcPicExtract->getPOC())
            {
                break;
            }

            iterPicInsert++;
        }

        iterPicExtract_1 = iterPicExtract;
        iterPicExtract_1++;

        //  swap iterPicExtract and iterPicInsert, iterPicExtract = curr. / iterPicInsert = insertion position
        rcListPic.insert(iterPicInsert, iterPicExtract, iterPicExtract_1);
        rcListPic.erase(iterPicExtract);
    }
}

TComPic* TComSlice::xGetRefPic(TComList<TComPic*>& rcListPic,
                               Int                 poc)
{
    TComList<TComPic*>::iterator  iterPic = rcListPic.begin();
    TComPic*                      pcPic = *(iterPic);
    while (iterPic != rcListPic.end())
    {
        if (pcPic->getPOC() == poc)
        {
            break;
        }
        iterPic++;
        pcPic = *(iterPic);
    }

    return pcPic;
}

TComPic* TComSlice::xGetLongTermRefPic(TComList<TComPic*>& rcListPic, Int poc, Bool pocHasMsb)
{
    TComList<TComPic*>::iterator  iterPic = rcListPic.begin();
    TComPic*                      pcPic = *(iterPic);
    TComPic*                      pcStPic = pcPic;

    Int pocCycle = 1 << getSPS()->getBitsForPOC();
    if (!pocHasMsb)
    {
        poc = poc % pocCycle;
    }

    while (iterPic != rcListPic.end())
    {
        pcPic = *(iterPic);
        if (pcPic && pcPic->getPOC() != this->getPOC() && pcPic->getSlice()->isReferenced())
        {
            Int picPoc = pcPic->getPOC();
            if (!pocHasMsb)
            {
                picPoc = picPoc % pocCycle;
            }

            if (poc == picPoc)
            {
                if (pcPic->getIsLongTerm())
                {
                    return pcPic;
                }
                else
                {
                    pcStPic = pcPic;
                }
                break;
            }
        }

        iterPic++;
    }

    return pcStPic;
}

Void TComSlice::setRefPOCList()
{
    for (Int iDir = 0; iDir < 2; iDir++)
    {
        for (Int iNumRefIdx = 0; iNumRefIdx < m_aiNumRefIdx[iDir]; iNumRefIdx++)
        {
            m_aiRefPOCList[iDir][iNumRefIdx] = m_apcRefPicList[iDir][iNumRefIdx]->getPOC();
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
            if (m_apcRefPicList[REF_PIC_LIST_0][idxL0]->getPOC() == m_apcRefPicList[REF_PIC_LIST_1][idxL1]->getPOC())
            {
                m_list1IdxToList0Idx[idxL1] = idxL0;
                break;
            }
        }
    }
}

Void TComSlice::setRefPicList(TComList<TComPic*>& rcListPic, Bool checkNumPocTotalCurr)
{
    if (!checkNumPocTotalCurr)
    {
        if (m_eSliceType == I_SLICE)
        {
            ::memset(m_apcRefPicList, 0, sizeof(m_apcRefPicList));
            ::memset(m_aiNumRefIdx,   0, sizeof(m_aiNumRefIdx));

            return;
        }

        m_aiNumRefIdx[0] = getNumRefIdx(REF_PIC_LIST_0);
        m_aiNumRefIdx[1] = getNumRefIdx(REF_PIC_LIST_1);
    }

    TComPic*  pcRefPic = NULL;
    TComPic*  RefPicSetStCurr0[16];
    TComPic*  RefPicSetStCurr1[16];
    TComPic*  RefPicSetLtCurr[16];
    UInt NumPocStCurr0 = 0;
    UInt NumPocStCurr1 = 0;
    UInt NumPocLtCurr = 0;
    Int i;

    for (i = 0; i < m_pcRPS->getNumberOfNegativePictures(); i++)
    {
        if (m_pcRPS->getUsed(i))
        {
            pcRefPic = xGetRefPic(rcListPic, getPOC() + m_pcRPS->getDeltaPOC(i));
            pcRefPic->setIsLongTerm(0);
            pcRefPic->getPicYuvRec()->extendPicBorder(x265::ThreadPool::GetThreadPool());
            RefPicSetStCurr0[NumPocStCurr0] = pcRefPic;
            NumPocStCurr0++;
            pcRefPic->setCheckLTMSBPresent(false);
        }
    }

    for (; i < m_pcRPS->getNumberOfNegativePictures() + m_pcRPS->getNumberOfPositivePictures(); i++)
    {
        if (m_pcRPS->getUsed(i))
        {
            pcRefPic = xGetRefPic(rcListPic, getPOC() + m_pcRPS->getDeltaPOC(i));
            pcRefPic->setIsLongTerm(0);
            pcRefPic->getPicYuvRec()->extendPicBorder(x265::ThreadPool::GetThreadPool());
            RefPicSetStCurr1[NumPocStCurr1] = pcRefPic;
            NumPocStCurr1++;
            pcRefPic->setCheckLTMSBPresent(false);
        }
    }

    for (i = m_pcRPS->getNumberOfNegativePictures() + m_pcRPS->getNumberOfPositivePictures() + m_pcRPS->getNumberOfLongtermPictures() - 1; i > m_pcRPS->getNumberOfNegativePictures() + m_pcRPS->getNumberOfPositivePictures() - 1; i--)
    {
        if (m_pcRPS->getUsed(i))
        {
            pcRefPic = xGetLongTermRefPic(rcListPic, m_pcRPS->getPOC(i), m_pcRPS->getCheckLTMSBPresent(i));
            pcRefPic->setIsLongTerm(1);
            pcRefPic->getPicYuvRec()->extendPicBorder(x265::ThreadPool::GetThreadPool());
            RefPicSetLtCurr[NumPocLtCurr] = pcRefPic;
            NumPocLtCurr++;
        }
        if (pcRefPic == NULL)
        {
            pcRefPic = xGetLongTermRefPic(rcListPic, m_pcRPS->getPOC(i), m_pcRPS->getCheckLTMSBPresent(i));
        }
        pcRefPic->setCheckLTMSBPresent(m_pcRPS->getCheckLTMSBPresent(i));
    }

    // ref_pic_list_init
    TComPic*  rpsCurrList0[MAX_NUM_REF + 1];
    TComPic*  rpsCurrList1[MAX_NUM_REF + 1];
    Int numPocTotalCurr = NumPocStCurr0 + NumPocStCurr1 + NumPocLtCurr;
    if (checkNumPocTotalCurr)
    {
        // The variable NumPocTotalCurr is derived as specified in subclause 7.4.7.2. It is a requirement of bitstream conformance that the following applies to the value of NumPocTotalCurr:
        // - If the current picture is a BLA or CRA picture, the value of NumPocTotalCurr shall be equal to 0.
        // - Otherwise, when the current picture contains a P or B slice, the value of NumPocTotalCurr shall not be equal to 0.
        if (getRapPicFlag())
        {
            assert(numPocTotalCurr == 0);
        }

        if (m_eSliceType == I_SLICE)
        {
            ::memset(m_apcRefPicList, 0, sizeof(m_apcRefPicList));
            ::memset(m_aiNumRefIdx,   0, sizeof(m_aiNumRefIdx));

            return;
        }

        assert(numPocTotalCurr >= 0);

        m_aiNumRefIdx[0] = getNumRefIdx(REF_PIC_LIST_0);
        m_aiNumRefIdx[1] = getNumRefIdx(REF_PIC_LIST_1);
    }

    Int cIdx = 0;
    for (i = 0; i < NumPocStCurr0; i++, cIdx++)
    {
        rpsCurrList0[cIdx] = RefPicSetStCurr0[i];
    }

    for (i = 0; i < NumPocStCurr1; i++, cIdx++)
    {
        rpsCurrList0[cIdx] = RefPicSetStCurr1[i];
    }

    for (i = 0; i < NumPocLtCurr; i++, cIdx++)
    {
        rpsCurrList0[cIdx] = RefPicSetLtCurr[i];
    }
    assert(cIdx == numPocTotalCurr);

    if (m_eSliceType == B_SLICE)
    {
        cIdx = 0;
        for (i = 0; i < NumPocStCurr1; i++, cIdx++)
        {
            rpsCurrList1[cIdx] = RefPicSetStCurr1[i];
        }

        for (i = 0; i < NumPocStCurr0; i++, cIdx++)
        {
            rpsCurrList1[cIdx] = RefPicSetStCurr0[i];
        }

        for (i = 0; i < NumPocLtCurr; i++, cIdx++)
        {
            rpsCurrList1[cIdx] = RefPicSetLtCurr[i];
        }
        assert(cIdx == numPocTotalCurr);
    }

    ::memset(m_bIsUsedAsLongTerm, 0, sizeof(m_bIsUsedAsLongTerm));

    for (Int rIdx = 0; rIdx < m_aiNumRefIdx[0]; rIdx ++)
    {
        cIdx = m_RefPicListModification.getRefPicListModificationFlagL0() ? m_RefPicListModification.getRefPicSetIdxL0(rIdx) : rIdx % numPocTotalCurr;
        assert(cIdx >= 0 && cIdx < numPocTotalCurr);
        m_apcRefPicList[0][rIdx] = rpsCurrList0[ cIdx ];
        m_bIsUsedAsLongTerm[0][rIdx] = ( cIdx >= NumPocStCurr0 + NumPocStCurr1 );
    }

    if (m_eSliceType != B_SLICE)
    {
        m_aiNumRefIdx[1] = 0;
        ::memset(m_apcRefPicList[1], 0, sizeof(m_apcRefPicList[1]));
    }
    else
    {
        for (Int rIdx = 0; rIdx < m_aiNumRefIdx[1]; rIdx ++)
        {
            cIdx = m_RefPicListModification.getRefPicListModificationFlagL1() ? m_RefPicListModification.getRefPicSetIdxL1(rIdx) : rIdx % numPocTotalCurr;
            assert(cIdx >= 0 && cIdx < numPocTotalCurr);
            m_apcRefPicList[1][rIdx] = rpsCurrList1[ cIdx ];
            m_bIsUsedAsLongTerm[1][rIdx] = ( cIdx >= NumPocStCurr0 + NumPocStCurr1 );
        }
    }
}

Int TComSlice::getNumRpsCurrTempList()
{
    Int numRpsCurrTempList = 0;

    if (m_eSliceType == I_SLICE)
    {
        return 0;
    }
    for (UInt i = 0; i < m_pcRPS->getNumberOfNegativePictures() + m_pcRPS->getNumberOfPositivePictures() + m_pcRPS->getNumberOfLongtermPictures(); i++)
    {
        if (m_pcRPS->getUsed(i))
        {
            numRpsCurrTempList++;
        }
    }

    return numRpsCurrTempList;
}

Void TComSlice::initEqualRef()
{
    for (Int iDir = 0; iDir < 2; iDir++)
    {
        for (Int iRefIdx1 = 0; iRefIdx1 < MAX_NUM_REF; iRefIdx1++)
        {
            for (Int iRefIdx2 = iRefIdx1; iRefIdx2 < MAX_NUM_REF; iRefIdx2++)
            {
                m_abEqualRef[iDir][iRefIdx1][iRefIdx2] = m_abEqualRef[iDir][iRefIdx2][iRefIdx1] = (iRefIdx1 == iRefIdx2 ? true : false);
            }
        }
    }
}

Void TComSlice::checkCRA(TComReferencePictureSet *pReferencePictureSet, Int& pocCRA, Bool& prevRAPisBLA, TComList<TComPic *>& rcListPic)
{
    for (Int i = 0; i < pReferencePictureSet->getNumberOfNegativePictures() + pReferencePictureSet->getNumberOfPositivePictures(); i++)
    {
        if (pocCRA < MAX_UINT && getPOC() > pocCRA)
        {
            assert(getPOC() + pReferencePictureSet->getDeltaPOC(i) >= pocCRA);
        }
    }

    for (Int i = pReferencePictureSet->getNumberOfNegativePictures() + pReferencePictureSet->getNumberOfPositivePictures(); i < pReferencePictureSet->getNumberOfPictures(); i++)
    {
        if (pocCRA < MAX_UINT && getPOC() > pocCRA)
        {
            if (!pReferencePictureSet->getCheckLTMSBPresent(i))
            {
                assert(xGetLongTermRefPic(rcListPic, pReferencePictureSet->getPOC(i), false)->getPOC() >= pocCRA);
            }
            else
            {
                assert(pReferencePictureSet->getPOC(i) >= pocCRA);
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

/** Function for marking the reference pictures when an IDR/CRA/CRANT/BLA/BLANT is encountered.
 * \param pocCRA POC of the CRA/CRANT/BLA/BLANT picture
 * \param bRefreshPending flag indicating if a deferred decoding refresh is pending
 * \param rcListPic reference to the reference picture list
 * This function marks the reference pictures as "unused for reference" in the following conditions.
 * If the nal_unit_type is IDR/BLA/BLANT, all pictures in the reference picture list
 * are marked as "unused for reference"
 *    If the nal_unit_type is BLA/BLANT, set the pocCRA to the temporal reference of the current picture.
 * Otherwise
 *    If the bRefreshPending flag is true (a deferred decoding refresh is pending) and the current
 *    temporal reference is greater than the temporal reference of the latest CRA/CRANT/BLA/BLANT picture (pocCRA),
 *    mark all reference pictures except the latest CRA/CRANT/BLA/BLANT picture as "unused for reference" and set
 *    the bRefreshPending flag to false.
 *    If the nal_unit_type is CRA/CRANT, set the bRefreshPending flag to true and pocCRA to the temporal
 *    reference of the current picture.
 * Note that the current picture is already placed in the reference list and its marking is not changed.
 * If the current picture has a nal_ref_idc that is not 0, it will remain marked as "used for reference".
 */
Void TComSlice::decodingRefreshMarking(Int& pocCRA, Bool& bRefreshPending, TComList<TComPic*>& rcListPic)
{
    TComPic*                 rpcPic;
    Int pocCurr = getPOC();

    if (getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_LP
        || getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_RADL
        || getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_N_LP
        || getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL
        || getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_N_LP) // IDR or BLA picture
    {
        // mark all pictures as not used for reference
        TComList<TComPic*>::iterator        iterPic       = rcListPic.begin();
        while (iterPic != rcListPic.end())
        {
            rpcPic = *(iterPic);
            if (rpcPic->getPOC() != pocCurr) rpcPic->getSlice()->setReferenced(false);
            iterPic++;
        }

        if (getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_LP
            || getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_RADL
            || getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_N_LP)
        {
            pocCRA = pocCurr;
        }
    }
    else // CRA or No DR
    {
        if (bRefreshPending == true && pocCurr > pocCRA) // CRA reference marking pending
        {
            TComList<TComPic*>::iterator        iterPic       = rcListPic.begin();
            while (iterPic != rcListPic.end())
            {
                rpcPic = *(iterPic);
                if (rpcPic->getPOC() != pocCurr && rpcPic->getPOC() != pocCRA)
                {
                    rpcPic->getSlice()->setReferenced(false);
                }
                iterPic++;
            }

            bRefreshPending = false;
        }
        if (getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA) // CRA picture found
        {
            bRefreshPending = true;
            pocCRA = pocCurr;
        }
    }
}

Void TComSlice::copySliceInfo(TComSlice *pSrc)
{
    assert(pSrc != NULL);

    Int i, j, k;

    m_iPOC                 = pSrc->m_iPOC;
    m_eNalUnitType         = pSrc->m_eNalUnitType;
    m_eSliceType           = pSrc->m_eSliceType;
    m_iSliceQp             = pSrc->m_iSliceQp;
    m_iSliceQpBase         = pSrc->m_iSliceQpBase;
    m_deblockingFilterDisable   = pSrc->m_deblockingFilterDisable;
    m_deblockingFilterOverrideFlag = pSrc->m_deblockingFilterOverrideFlag;
    m_deblockingFilterBetaOffsetDiv2 = pSrc->m_deblockingFilterBetaOffsetDiv2;
    m_deblockingFilterTcOffsetDiv2 = pSrc->m_deblockingFilterTcOffsetDiv2;

    for (i = 0; i < 2; i++)
    {
        m_aiNumRefIdx[i]     = pSrc->m_aiNumRefIdx[i];
    }

    for (i = 0; i < MAX_NUM_REF; i++)
    {
        m_list1IdxToList0Idx[i] = pSrc->m_list1IdxToList0Idx[i];
    }

    m_bCheckLDC             = pSrc->m_bCheckLDC;
    m_iSliceQpDelta        = pSrc->m_iSliceQpDelta;
    m_iSliceQpDeltaCb      = pSrc->m_iSliceQpDeltaCb;
    m_iSliceQpDeltaCr      = pSrc->m_iSliceQpDeltaCr;
    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < MAX_NUM_REF; j++)
        {
            m_apcRefPicList[i][j]  = pSrc->m_apcRefPicList[i][j];
            m_aiRefPOCList[i][j]   = pSrc->m_aiRefPOCList[i][j];
        }
    }

    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < MAX_NUM_REF + 1; j++)
        {
            m_bIsUsedAsLongTerm[i][j] = pSrc->m_bIsUsedAsLongTerm[i][j];
        }
    }

    m_iDepth               = pSrc->m_iDepth;

    // referenced slice
    m_bReferenced          = pSrc->m_bReferenced;

    // access channel
    m_pcSPS                = pSrc->m_pcSPS;
    m_pcPPS                = pSrc->m_pcPPS;
    m_pcRPS                = pSrc->m_pcRPS;
    m_iLastIDR             = pSrc->m_iLastIDR;

    m_pcPic                = pSrc->m_pcPic;

    m_colFromL0Flag        = pSrc->m_colFromL0Flag;
    m_colRefIdx            = pSrc->m_colRefIdx;
    m_dLambdaLuma          = pSrc->m_dLambdaLuma;
    m_dLambdaChroma        = pSrc->m_dLambdaChroma;
    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < MAX_NUM_REF; j++)
        {
            for (k = 0; k < MAX_NUM_REF; k++)
            {
                m_abEqualRef[i][j][k] = pSrc->m_abEqualRef[i][j][k];
            }
        }
    }

    m_uiTLayer                      = pSrc->m_uiTLayer;
    m_bTLayerSwitchingFlag          = pSrc->m_bTLayerSwitchingFlag;

    m_sliceCurEndCUAddr            = pSrc->m_sliceCurEndCUAddr;
    m_nextSlice                    = pSrc->m_nextSlice;
    for (Int e = 0; e < 2; e++)
    {
        for (Int n = 0; n < MAX_NUM_REF; n++)
        {
            memcpy(m_weightPredTable[e][n], pSrc->m_weightPredTable[e][n], sizeof(wpScalingParam) * 3);
        }
    }

    m_saoEnabledFlag = pSrc->m_saoEnabledFlag;
    m_saoEnabledFlagChroma = pSrc->m_saoEnabledFlagChroma;
    m_cabacInitFlag                = pSrc->m_cabacInitFlag;
    m_numEntryPointOffsets  = pSrc->m_numEntryPointOffsets;

    m_bLMvdL1Zero = pSrc->m_bLMvdL1Zero;
    m_enableTMVPFlag                = pSrc->m_enableTMVPFlag;
    m_maxNumMergeCand               = pSrc->m_maxNumMergeCand;
}

Int TComSlice::m_prevPOC = 0;

/** Function for setting the slice's temporal layer ID and corresponding temporal_layer_switching_point_flag.
 * \param uiTLayer Temporal layer ID of the current slice
 * The decoder calls this function to set temporal_layer_switching_point_flag for each temporal layer based on
 * the SPS's temporal_id_nesting_flag and the parsed PPS.  Then, current slice's temporal layer ID and
 * temporal_layer_switching_point_flag is set accordingly.
 */
Void TComSlice::setTLayerInfo(UInt uiTLayer)
{
    m_uiTLayer = uiTLayer;
}

/** Function for checking if this is a switching-point
*/
Bool TComSlice::isTemporalLayerSwitchingPoint(TComList<TComPic*>& rcListPic)
{
    TComPic* rpcPic;

    // loop through all pictures in the reference picture buffer
    TComList<TComPic*>::iterator iterPic = rcListPic.begin();
    while (iterPic != rcListPic.end())
    {
        rpcPic = *(iterPic++);
        if (rpcPic->getSlice()->isReferenced() && rpcPic->getPOC() != getPOC())
        {
            if (rpcPic->getTLayer() >= getTLayer())
            {
                return false;
            }
        }
    }

    return true;
}

/** Function for checking if this is a STSA candidate
 */
Bool TComSlice::isStepwiseTemporalLayerSwitchingPointCandidate(TComList<TComPic*>& rcListPic)
{
    TComPic* rpcPic;

    TComList<TComPic*>::iterator iterPic = rcListPic.begin();
    while (iterPic != rcListPic.end())
    {
        rpcPic = *(iterPic++);
        if (rpcPic->getSlice()->isReferenced() &&  (rpcPic->getUsedByCurr() == true) && rpcPic->getPOC() != getPOC())
        {
            if (rpcPic->getTLayer() >= getTLayer())
            {
                return false;
            }
        }
    }

    return true;
}

/** Function for applying picture marking based on the Reference Picture Set in pReferencePictureSet.
*/
Void TComSlice::applyReferencePictureSet(TComList<TComPic*>& rcListPic, TComReferencePictureSet *pReferencePictureSet)
{
    TComPic* rpcPic;
    Int i, isReference;

    // loop through all pictures in the reference picture buffer
    TComList<TComPic*>::iterator iterPic = rcListPic.begin();
    while (iterPic != rcListPic.end())
    {
        rpcPic = *(iterPic++);

        if (!rpcPic->getSlice()->isReferenced())
        {
            continue;
        }

        isReference = 0;
        // loop through all pictures in the Reference Picture Set
        // to see if the picture should be kept as reference picture
        for (i = 0; i < pReferencePictureSet->getNumberOfPositivePictures() + pReferencePictureSet->getNumberOfNegativePictures(); i++)
        {
            if (!rpcPic->getIsLongTerm() && rpcPic->getPicSym()->getSlice()->getPOC() == this->getPOC() + pReferencePictureSet->getDeltaPOC(i))
            {
                isReference = 1;
                rpcPic->setUsedByCurr(pReferencePictureSet->getUsed(i));
                rpcPic->setIsLongTerm(0);
            }
        }

        for (; i < pReferencePictureSet->getNumberOfPictures(); i++)
        {
            if (pReferencePictureSet->getCheckLTMSBPresent(i) == true)
            {
                if (rpcPic->getIsLongTerm() && (rpcPic->getPicSym()->getSlice()->getPOC()) == pReferencePictureSet->getPOC(i))
                {
                    isReference = 1;
                    rpcPic->setUsedByCurr(pReferencePictureSet->getUsed(i));
                }
            }
            else
            {
                if (rpcPic->getIsLongTerm() && (rpcPic->getPicSym()->getSlice()->getPOC() % (1 << rpcPic->getPicSym()->getSlice()->getSPS()->getBitsForPOC())) == pReferencePictureSet->getPOC(i) % (1 << rpcPic->getPicSym()->getSlice()->getSPS()->getBitsForPOC()))
                {
                    isReference = 1;
                    rpcPic->setUsedByCurr(pReferencePictureSet->getUsed(i));
                }
            }
        }

        // mark the picture as "unused for reference" if it is not in
        // the Reference Picture Set
        if (rpcPic->getPicSym()->getSlice()->getPOC() != this->getPOC() && isReference == 0)
        {
            rpcPic->getSlice()->setReferenced(false);
            rpcPic->setUsedByCurr(0);
            rpcPic->setIsLongTerm(0);
        }
        //check that pictures of higher temporal layers are not used
        assert(rpcPic->getSlice(0)->isReferenced() == 0 || rpcPic->getUsedByCurr() == 0 || rpcPic->getTLayer() <= this->getTLayer());
        //check that pictures of higher or equal temporal layer are not in the RPS if the current picture is a TSA picture
        if (this->getNalUnitType() == NAL_UNIT_CODED_SLICE_TLA_R || this->getNalUnitType() == NAL_UNIT_CODED_SLICE_TSA_N)
        {
            assert(rpcPic->getSlice(0)->isReferenced() == 0 || rpcPic->getTLayer() < this->getTLayer());
        }
        //check that pictures marked as temporal layer non-reference pictures are not used for reference
        if (rpcPic->getPicSym()->getSlice()->getPOC() != this->getPOC() && rpcPic->getTLayer() == this->getTLayer())
        {
            assert(rpcPic->getSlice(0)->isReferenced() == 0 || rpcPic->getUsedByCurr() == 0 || rpcPic->getSlice(0)->getTemporalLayerNonReferenceFlag() == false);
        }
    }
}

/** Function for applying picture marking based on the Reference Picture Set in pReferencePictureSet.
*/
Int TComSlice::checkThatAllRefPicsAreAvailable(TComList<TComPic*>& rcListPic, TComReferencePictureSet *pReferencePictureSet, Bool printErrors, Int pocRandomAccess)
{
    TComPic* rpcPic;
    Int i, isAvailable;
    Int atLeastOneLost = 0;
    Int atLeastOneRemoved = 0;
    Int iPocLost = 0;

    // loop through all long-term pictures in the Reference Picture Set
    // to see if the picture should be kept as reference picture
    for (i = pReferencePictureSet->getNumberOfNegativePictures() + pReferencePictureSet->getNumberOfPositivePictures(); i < pReferencePictureSet->getNumberOfPictures(); i++)
    {
        isAvailable = 0;
        // loop through all pictures in the reference picture buffer
        TComList<TComPic*>::iterator iterPic = rcListPic.begin();
        while (iterPic != rcListPic.end())
        {
            rpcPic = *(iterPic++);
            if (pReferencePictureSet->getCheckLTMSBPresent(i) == true)
            {
                if (rpcPic->getIsLongTerm() && (rpcPic->getPicSym()->getSlice()->getPOC()) == pReferencePictureSet->getPOC(i) && rpcPic->getSlice()->isReferenced())
                {
                    isAvailable = 1;
                }
            }
            else
            {
                if (rpcPic->getIsLongTerm() && (rpcPic->getPicSym()->getSlice()->getPOC() % (1 << rpcPic->getPicSym()->getSlice()->getSPS()->getBitsForPOC())) == pReferencePictureSet->getPOC(i) % (1 << rpcPic->getPicSym()->getSlice()->getSPS()->getBitsForPOC()) && rpcPic->getSlice()->isReferenced())
                {
                    isAvailable = 1;
                }
            }
        }

        // if there was no such long-term check the short terms
        if (!isAvailable)
        {
            iterPic = rcListPic.begin();
            while (iterPic != rcListPic.end())
            {
                rpcPic = *(iterPic++);

                Int pocCycle = 1 << rpcPic->getPicSym()->getSlice()->getSPS()->getBitsForPOC();
                Int curPoc = rpcPic->getPicSym()->getSlice()->getPOC();
                Int refPoc = pReferencePictureSet->getPOC(i);
                if (!pReferencePictureSet->getCheckLTMSBPresent(i))
                {
                    curPoc = curPoc % pocCycle;
                    refPoc = refPoc % pocCycle;
                }

                if (rpcPic->getSlice()->isReferenced() && curPoc == refPoc)
                {
                    isAvailable = 1;
                    rpcPic->setIsLongTerm(1);
                    break;
                }
            }
        }
        // report that a picture is lost if it is in the Reference Picture Set
        // but not available as reference picture
        if (isAvailable == 0)
        {
            if (this->getPOC() + pReferencePictureSet->getDeltaPOC(i) >= pocRandomAccess)
            {
                if (!pReferencePictureSet->getUsed(i))
                {
                    if (printErrors)
                    {
                        printf("\nLong-term reference picture with POC = %3d seems to have been removed or not correctly decoded.", this->getPOC() + pReferencePictureSet->getDeltaPOC(i));
                    }
                    atLeastOneRemoved = 1;
                }
                else
                {
                    if (printErrors)
                    {
                        printf("\nLong-term reference picture with POC = %3d is lost or not correctly decoded!", this->getPOC() + pReferencePictureSet->getDeltaPOC(i));
                    }
                    atLeastOneLost = 1;
                    iPocLost = this->getPOC() + pReferencePictureSet->getDeltaPOC(i);
                }
            }
        }
    }

    // loop through all short-term pictures in the Reference Picture Set
    // to see if the picture should be kept as reference picture
    for (i = 0; i < pReferencePictureSet->getNumberOfNegativePictures() + pReferencePictureSet->getNumberOfPositivePictures(); i++)
    {
        isAvailable = 0;
        // loop through all pictures in the reference picture buffer
        TComList<TComPic*>::iterator iterPic = rcListPic.begin();
        while (iterPic != rcListPic.end())
        {
            rpcPic = *(iterPic++);

            if (!rpcPic->getIsLongTerm() && rpcPic->getPicSym()->getSlice()->getPOC() == this->getPOC() + pReferencePictureSet->getDeltaPOC(i) && rpcPic->getSlice()->isReferenced())
            {
                isAvailable = 1;
            }
        }

        // report that a picture is lost if it is in the Reference Picture Set
        // but not available as reference picture
        if (isAvailable == 0)
        {
            if (this->getPOC() + pReferencePictureSet->getDeltaPOC(i) >= pocRandomAccess)
            {
                if (!pReferencePictureSet->getUsed(i))
                {
                    if (printErrors)
                    {
                        printf("\nShort-term reference picture with POC = %3d seems to have been removed or not correctly decoded.", this->getPOC() + pReferencePictureSet->getDeltaPOC(i));
                    }
                    atLeastOneRemoved = 1;
                }
                else
                {
                    if (printErrors)
                    {
                        printf("\nShort-term reference picture with POC = %3d is lost or not correctly decoded!", this->getPOC() + pReferencePictureSet->getDeltaPOC(i));
                    }
                    atLeastOneLost = 1;
                    iPocLost = this->getPOC() + pReferencePictureSet->getDeltaPOC(i);
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
Void TComSlice::createExplicitReferencePictureSetFromReference(TComList<TComPic*>& rcListPic, TComReferencePictureSet *pReferencePictureSet, Bool isRAP)
{
    TComPic* rpcPic;
    Int i, j;
    Int k = 0;
    Int nrOfNegativePictures = 0;
    Int nrOfPositivePictures = 0;
    TComReferencePictureSet* pcRPS = this->getLocalRPS();

    // loop through all pictures in the Reference Picture Set
    for (i = 0; i < pReferencePictureSet->getNumberOfPictures(); i++)
    {
        j = 0;
        // loop through all pictures in the reference picture buffer
        TComList<TComPic*>::iterator iterPic = rcListPic.begin();
        while (iterPic != rcListPic.end())
        {
            j++;
            rpcPic = *(iterPic++);

            if (rpcPic->getPicSym()->getSlice()->getPOC() == this->getPOC() + pReferencePictureSet->getDeltaPOC(i) && rpcPic->getSlice()->isReferenced())
            {
                // This picture exists as a reference picture
                // and should be added to the explicit Reference Picture Set
                pcRPS->setDeltaPOC(k, pReferencePictureSet->getDeltaPOC(i));
                pcRPS->setUsed(k, pReferencePictureSet->getUsed(i) && (!isRAP));
                if (pcRPS->getDeltaPOC(k) < 0)
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

    pcRPS->setNumberOfNegativePictures(nrOfNegativePictures);
    pcRPS->setNumberOfPositivePictures(nrOfPositivePictures);
    pcRPS->setNumberOfPictures(nrOfNegativePictures + nrOfPositivePictures);
    // This is a simplistic inter rps example. A smarter encoder will look for a better reference RPS to do the
    // inter RPS prediction with.  Here we just use the reference used by pReferencePictureSet.
    // If pReferencePictureSet is not inter_RPS_predicted, then inter_RPS_prediction is for the current RPS also disabled.
    if (!pReferencePictureSet->getInterRPSPrediction())
    {
        pcRPS->setInterRPSPrediction(false);
        pcRPS->setNumRefIdc(0);
    }
    else
    {
        Int rIdx =  this->getRPSidx() - pReferencePictureSet->getDeltaRIdxMinus1() - 1;
        Int deltaRPS = pReferencePictureSet->getDeltaRPS();
        TComReferencePictureSet* pcRefRPS = this->getSPS()->getRPSList()->getReferencePictureSet(rIdx);
        Int iRefPics = pcRefRPS->getNumberOfPictures();
        Int iNewIdc = 0;
        for (i = 0; i <= iRefPics; i++)
        {
            Int deltaPOC = ((i != iRefPics) ? pcRefRPS->getDeltaPOC(i) : 0); // check if the reference abs POC is >= 0
            Int iRefIdc = 0;
            for (j = 0; j < pcRPS->getNumberOfPictures(); j++) // loop through the  pictures in the new RPS
            {
                if ((deltaPOC + deltaRPS) == pcRPS->getDeltaPOC(j))
                {
                    if (pcRPS->getUsed(j))
                    {
                        iRefIdc = 1;
                    }
                    else
                    {
                        iRefIdc = 2;
                    }
                }
            }

            pcRPS->setRefIdc(i, iRefIdc);
            iNewIdc++;
        }

        pcRPS->setInterRPSPrediction(true);
        pcRPS->setNumRefIdc(iNewIdc);
        pcRPS->setDeltaRPS(deltaRPS);
        pcRPS->setDeltaRIdxMinus1(pReferencePictureSet->getDeltaRIdxMinus1() + this->getSPS()->getRPSList()->getNumberOfReferencePictureSets() - this->getRPSidx());
    }

    this->setRPS(pcRPS);
    this->setRPSidx(-1);
}

/** get AC and DC values for weighted pred
 * \param *wp
 * \returns Void
 */
Void  TComSlice::getWpAcDcParam(wpACDCParam *&wp)
{
    wp = m_weightACDCParam;
}

/** init AC and DC values for weighted pred
 * \returns Void
 */
Void  TComSlice::initWpAcDcParam()
{
    for (Int iComp = 0; iComp < 3; iComp++)
    {
        m_weightACDCParam[iComp].iAC = 0;
        m_weightACDCParam[iComp].iDC = 0;
    }
}

/** get WP tables for weighted pred
 * \param RefPicList
 * \param iRefIdx
 * \param *&wpScalingParam
 * \returns Void
 */
Void  TComSlice::getWpScaling(RefPicList e, Int iRefIdx, wpScalingParam *&wp)
{
    wp = m_weightPredTable[e][iRefIdx];
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
                pwp->bPresentFlag      = false;
                pwp->uiLog2WeightDenom = 0;
                pwp->uiLog2WeightDenom = 0;
                pwp->iWeight           = 1;
                pwp->iOffset           = 0;
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
                    pwp->iWeight = (1 << pwp->uiLog2WeightDenom);
                    pwp->iOffset = 0;
                }

                pwp->w      = pwp->iWeight;
                Int bitDepth = yuv ? g_bitDepthC : g_bitDepthY;
                pwp->o      = pwp->iOffset << (bitDepth - 8);
                pwp->shift  = pwp->uiLog2WeightDenom;
                pwp->round  = (pwp->uiLog2WeightDenom >= 1) ? (1 << (pwp->uiLog2WeightDenom - 1)) : (0);
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
    , m_uiMaxLayers(1)
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
        m_uiMaxDecPicBuffering[i] = 1;
        m_uiMaxLatencyIncrease[i] = 0;
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
    , m_uiMaxTLayers(1)
// Structure
    , m_picWidthInLumaSamples(352)
    , m_picHeightInLumaSamples(288)
    , m_log2MinCodingBlockSize(0)
    , m_log2DiffMaxMinCodingBlockSize(0)
    , m_uiMaxCUWidth(32)
    , m_uiMaxCUHeight(32)
    , m_uiMaxCUDepth(3)
    , m_bLongTermRefsPresent(false)
    , m_uiQuadtreeTULog2MaxSize(0)
    , m_uiQuadtreeTULog2MinSize(0)
    , m_uiQuadtreeTUMaxDepthInter(0)
    , m_uiQuadtreeTUMaxDepthIntra(0)
// Tool list
    , m_usePCM(false)
    , m_pcmLog2MaxSize(5)
    , m_uiPCMLog2MinSize(7)
    , m_bitDepthY(8)
    , m_bitDepthC(8)
    , m_qpBDOffsetY(0)
    , m_qpBDOffsetC(0)
    , m_useLossless(false)
    , m_uiPCMBitDepthLuma(8)
    , m_uiPCMBitDepthChroma(8)
    , m_bPCMFilterDisableFlag(false)
    , m_uiBitsForPOC(8)
    , m_numLongTermRefPicSPS(0)
    , m_uiMaxTrSize(32)
    , m_bUseSAO(false)
    , m_bTemporalIdNestingFlag(false)
    , m_scalingListEnabledFlag(false)
    , m_useStrongIntraSmoothing(false)
    , m_vuiParametersPresentFlag(false)
    , m_vuiParameters()
{
    for (Int i = 0; i < MAX_TLAYER; i++)
    {
        m_uiMaxLatencyIncrease[i] = 0;
        m_uiMaxDecPicBuffering[i] = 1;
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
        hrd->setTickDivisorMinus2(100 - 2);                        //
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
    hrd->setDuCpbSizeScale(6);                                       // in units of 2~( 4 + 4 ) = 1,024 bit

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

const Int TComSPS::m_winUnitX[] = { 1, 2, 2, 1 };
const Int TComSPS::m_winUnitY[] = { 1, 2, 1, 1 };

TComPPS::TComPPS()
    : m_PPSId(0)
    , m_SPSId(0)
    , m_picInitQPMinus26(0)
    , m_useDQP(false)
    , m_bConstrainedIntraPred(false)
    , m_bSliceChromaQpFlag(false)
    , m_pcSPS(NULL)
    , m_uiMaxCuDQPDepth(0)
    , m_uiMinCuDQPSize(0)
    , m_chromaCbQpOffset(0)
    , m_chromaCrQpOffset(0)
    , m_numRefIdxL0DefaultActive(1)
    , m_numRefIdxL1DefaultActive(1)
    , m_TransquantBypassEnableFlag(false)
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
    : m_numberOfPictures(0)
    , m_numberOfNegativePictures(0)
    , m_numberOfPositivePictures(0)
    , m_numberOfLongtermPictures(0)
    , m_interRPSPrediction(0)
    , m_deltaRIdxMinus1(0)
    , m_deltaRPS(0)
    , m_numRefIdc(0)
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

Int TComReferencePictureSet::getUsed(Int bufferNum)
{
    return m_used[bufferNum];
}

Int TComReferencePictureSet::getDeltaPOC(Int bufferNum)
{
    return m_deltaPOC[bufferNum];
}

Int TComReferencePictureSet::getNumberOfPictures()
{
    return m_numberOfPictures;
}

Int TComReferencePictureSet::getPOC(Int bufferNum)
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
Int  TComReferencePictureSet::getRefIdc(Int bufferNum)
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

Int TComRPSList::getNumberOfReferencePictureSets()
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

/** parse syntax infomation
 *  \param pchFile syntax infomation
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
