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

using namespace x265;

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
    , m_bReferenced(false)
    , m_sps(NULL)
    , m_pps(NULL)
    , m_pic(NULL)
    , m_colFromL0Flag(1)
    , m_colRefIdx(0)
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

    for (int numCount = 0; numCount < MAX_NUM_REF; numCount++)
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
}

void TComSlice::initSlice()
{
    m_numRefIdx[0] = 0;
    m_numRefIdx[1] = 0;

    m_colFromL0Flag = 1;

    m_colRefIdx = 0;
    m_bCheckLDC = false;
    m_sliceQpDeltaCb = 0;
    m_sliceQpDeltaCr = 0;
    m_maxNumMergeCand = MRG_MAX_NUM_CANDS;

    m_bFinalized = false;

    m_cabacInitFlag = false;
    m_numEntryPointOffsets = 0;
    m_enableTMVPFlag = true;
    m_ssim = 0;
    m_ssimCnt = 0;
    m_numWPRefs = 0;
}

bool TComSlice::getRapPicFlag()
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
void  TComSlice::allocSubstreamSizes(UInt numSubstreams)
{
    delete[] m_substreamSizes;
    m_substreamSizes = new UInt[numSubstreams > 0 ? numSubstreams - 1 : 0];
}

TComPic* TComSlice::xGetRefPic(PicList& picList, int poc)
{
    TComPic *iterPic = picList.first();
    TComPic* pic = NULL;

    while (iterPic)
    {
        pic = iterPic;
        if (pic->getPOC() == poc)
        {
            break;
        }
        iterPic = iterPic->m_next;
    }

    return pic;
}

TComPic* TComSlice::xGetLongTermRefPic(PicList& picList, int poc, bool pocHasMsb)
{
    TComPic* iterPic = picList.first();
    TComPic* pic = iterPic;
    TComPic* stPic = pic;

    int pocCycle = 1 << getSPS()->getBitsForPOC();

    if (!pocHasMsb)
    {
        poc = poc % pocCycle;
    }

    while (iterPic)
    {
        pic = iterPic;
        if (pic && pic->getPOC() != this->getPOC() && pic->getSlice()->isReferenced())
        {
            int picPoc = pic->getPOC();
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

        iterPic = iterPic->m_next;
    }

    return stPic;
}

void TComSlice::setRefPOCList()
{
    for (int dir = 0; dir < 2; dir++)
    {
        for (int numRefIdx = 0; numRefIdx < m_numRefIdx[dir]; numRefIdx++)
        {
            m_refPOCList[dir][numRefIdx] = m_refPicList[dir][numRefIdx]->getPOC();
        }
    }
}

void TComSlice::setRefPicList(PicList& picList, bool checkNumPocTotalCurr)
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
    int i;

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
    int numPocTotalCurr = numPocStCurr0 + numPocStCurr1 + numPocLtCurr;
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

    int cIdx = 0;
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

    for (int rIdx = 0; rIdx < m_numRefIdx[0]; rIdx++)
    {
        cIdx = rIdx % numPocTotalCurr;
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
        for (int rIdx = 0; rIdx < m_numRefIdx[1]; rIdx++)
        {
            cIdx = rIdx % numPocTotalCurr;
            assert(cIdx >= 0 && cIdx < numPocTotalCurr);
            m_refPicList[1][rIdx] = rpsCurrList1[cIdx];
            m_bIsUsedAsLongTerm[1][rIdx] = (cIdx >= numPocStCurr0 + numPocStCurr1);
        }
    }
}

int TComSlice::getNumRpsCurrTempList()
{
    int numRpsCurrTempList = 0;

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

void TComSlice::checkCRA(TComReferencePictureSet *rps, int& pocCRA, bool& prevRAPisBLA)
{
    for (int i = 0; i < rps->getNumberOfNegativePictures() + rps->getNumberOfPositivePictures(); i++)
    {
        if (pocCRA < MAX_UINT && getPOC() > pocCRA)
        {
            assert(getPOC() + rps->getDeltaPOC(i) >= pocCRA);
        }
    }

    for (int i = rps->getNumberOfNegativePictures() + rps->getNumberOfPositivePictures(); i < rps->getNumberOfPictures(); i++)
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

/** get AC and DC values for weighted pred
 * \param *wp
 * \returns void
 */
void TComSlice::getWpAcDcParam(wpACDCParam *&wp)
{
    wp = m_weightACDCParam;
}

/** init AC and DC values for weighted pred
 * \returns void
 */
void TComSlice::initWpAcDcParam()
{
    for (int comp = 0; comp < 3; comp++)
    {
        m_weightACDCParam[comp].ac = 0;
        m_weightACDCParam[comp].dc = 0;
    }
}

/** get WP tables for weighted pred
 * \param RefPicList
 * \param refIdx
 * \param *&wpScalingParam
 * \returns void
 */
void TComSlice::getWpScaling(RefPicList l, int refIdx, wpScalingParam *&wp)
{
    wp = m_weightPredTable[l][refIdx];
}

/** reset Default WP tables settings : no weight.
 * \param wpScalingParam
 * \returns void
 */
void  TComSlice::resetWpScaling()
{
    for (int e = 0; e < 2; e++)
    {
        for (int i = 0; i < MAX_NUM_REF; i++)
        {
            for (int yuv = 0; yuv < 3; yuv++)
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
 * \returns void
 */
void  TComSlice::initWpScaling()
{
    for (int e = 0; e < 2; e++)
    {
        for (int i = 0; i < MAX_NUM_REF; i++)
        {
            for (int yuv = 0; yuv < 3; yuv++)
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
    for (int i = 0; i < MAX_TLAYER; i++)
    {
        m_numReorderPics[i] = 0;
        m_maxDecPicBuffering[i] = 1;
        m_maxLatencyIncrease[i] = 0;
    }
}

TComVPS::~TComVPS()
{
    delete[] m_hrdParameters;
    delete[] m_hrdOpSetIdx;
    delete[] m_cprmsPresentFlag;
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
    for (int i = 0; i < MAX_TLAYER; i++)
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

void  TComSPS::createRPSList(int numRPS)
{
    m_RPSList.destroy();
    m_RPSList.create(numRPS);
}

void TComSPS::setHrdParameters(UInt frameRate, UInt numDU, UInt bitRate, bool randomAccess)
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

    bool rateCnt = (bitRate > 0);
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
    int i, j;
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

void TComReferencePictureSet::setUsed(int bufferNum, bool used)
{
    m_used[bufferNum] = used;
}

void TComReferencePictureSet::setDeltaPOC(int bufferNum, int deltaPOC)
{
    m_deltaPOC[bufferNum] = deltaPOC;
}

int TComReferencePictureSet::getUsed(int bufferNum) const
{
    return m_used[bufferNum];
}

int TComReferencePictureSet::getDeltaPOC(int bufferNum) const
{
    return m_deltaPOC[bufferNum];
}

int TComReferencePictureSet::getNumberOfPictures() const
{
    return m_numberOfPictures;
}

int TComReferencePictureSet::getPOC(int bufferNum) const
{
    return m_POC[bufferNum];
}

void TComReferencePictureSet::setPOC(int bufferNum, int POC)
{
    m_POC[bufferNum] = POC;
}

bool TComReferencePictureSet::getCheckLTMSBPresent(int bufferNum)
{
    return m_bCheckLTMSB[bufferNum];
}

void TComReferencePictureSet::setCheckLTMSBPresent(int bufferNum, bool b)
{
    m_bCheckLTMSB[bufferNum] = b;
}

/** get the reference idc value at uiBufferNum
 * \param uiBufferNum
 * \returns int
 */
int  TComReferencePictureSet::getRefIdc(int bufferNum) const
{
    return m_refIdc[bufferNum];
}

/** Sorts the deltaPOC and Used by current values in the RPS based on the deltaPOC values.
 *  deltaPOC values are sorted with -ve values before the +ve values.  -ve values are in decreasing order.
 *  +ve values are in increasing order.
 * \returns void
 */
void TComReferencePictureSet::sortDeltaPOC()
{
    // sort in increasing order (smallest first)
    for (int j = 1; j < getNumberOfPictures(); j++)
    {
        int deltaPOC = getDeltaPOC(j);
        bool used = getUsed(j);
        for (int k = j - 1; k >= 0; k--)
        {
            int temp = getDeltaPOC(k);
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
    int numNegPics = getNumberOfNegativePictures();
    for (int j = 0, k = numNegPics - 1; j < numNegPics >> 1; j++, k--)
    {
        int deltaPOC = getDeltaPOC(j);
        bool used = getUsed(j);
        setDeltaPOC(j, getDeltaPOC(k));
        setUsed(j, getUsed(k));
        setDeltaPOC(k, deltaPOC);
        setUsed(k, used);
    }
}

/** Prints the deltaPOC and RefIdc (if available) values in the RPS.
 *  A "*" is added to the deltaPOC value if it is Used bu current.
 * \returns void
 */
void TComReferencePictureSet::printDeltaPOC()
{
    printf("DeltaPOC = { ");
    for (int j = 0; j < getNumberOfPictures(); j++)
    {
        printf("%d%s ", getDeltaPOC(j), (getUsed(j) == 1) ? "*" : "");
    }

    if (getInterRPSPrediction())
    {
        printf("}, RefIdc = { ");
        for (int j = 0; j < getNumRefIdc(); j++)
        {
            printf("%d ", getRefIdc(j));
        }
    }
    printf("}\n");
}

TComRPSList::TComRPSList()
    : m_numberOfReferencePictureSets(0)
    , m_referencePictureSets(NULL)
{}

TComRPSList::~TComRPSList()
{}

void TComRPSList::create(int numberOfReferencePictureSets)
{
    m_numberOfReferencePictureSets = numberOfReferencePictureSets;
    m_referencePictureSets = new TComReferencePictureSet[numberOfReferencePictureSets];
}

void TComRPSList::destroy()
{
    delete [] m_referencePictureSets;
    m_numberOfReferencePictureSets = 0;
    m_referencePictureSets = NULL;
}

TComReferencePictureSet* TComRPSList::getReferencePictureSet(int referencePictureSetNum)
{
    return &m_referencePictureSets[referencePictureSetNum];
}

int TComRPSList::getNumberOfReferencePictureSets() const
{
    return m_numberOfReferencePictureSets;
}

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
void TComSlice::setDefaultScalingList()
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
bool TComSlice::checkDefaultScalingList()
{
    UInt defaultCounter = 0;

    for (UInt sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
    {
        for (UInt listId = 0; listId < g_scalingListNum[sizeId]; listId++)
        {
            if (!memcmp(getScalingList()->getScalingListAddress(sizeId, listId), getScalingList()->getScalingListDefaultAddress(sizeId, listId), sizeof(int) * X265_MIN(MAX_MATRIX_COEF_NUM, (int)g_scalingListSize[sizeId])) // check value of matrix
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
void TComScalingList::processRefMatrix(UInt sizeId, UInt listId, UInt refListId)
{
    ::memcpy(getScalingListAddress(sizeId, listId), ((listId == refListId) ? getScalingListDefaultAddress(sizeId, refListId) : getScalingListAddress(sizeId, refListId)), sizeof(int) * X265_MIN(MAX_MATRIX_COEF_NUM, (int)g_scalingListSize[sizeId]));
}

/** parse syntax information
 *  \param pchFile syntax information
 *  \returns false if successful
 */
bool TComScalingList::xParseScalingList(char* pchFile)
{
    FILE *fp;
    char line[1024];
    UInt sizeIdc, listIdc;
    UInt i, size = 0;
    int *src = 0, data;
    char *ret;
    UInt  retval;

    if ((fp = fopen(pchFile, "r")) == (FILE*)NULL)
    {
        printf("can't open file %s :: set Default Matrix\n", pchFile);
        return true;
    }

    for (sizeIdc = 0; sizeIdc < SCALING_LIST_SIZE_NUM; sizeIdc++)
    {
        size = X265_MIN(MAX_MATRIX_COEF_NUM, (int)g_scalingListSize[sizeIdc]);
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
void TComScalingList::init()
{
    for (UInt sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
    {
        for (UInt listId = 0; listId < g_scalingListNum[sizeId]; listId++)
        {
            m_scalingListCoef[sizeId][listId] = new int[X265_MIN(MAX_MATRIX_COEF_NUM, (int)g_scalingListSize[sizeId])];
        }
    }

    m_scalingListCoef[SCALING_LIST_32x32][3] = m_scalingListCoef[SCALING_LIST_32x32][1]; // copy address for 32x32
}

/** destroy quantization matrix array
 */
void TComScalingList::destroy()
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
int* TComScalingList::getScalingListDefaultAddress(UInt sizeId, UInt listId)
{
    int *src = 0;

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
void TComScalingList::processDefaultMarix(UInt sizeId, UInt listId)
{
    ::memcpy(getScalingListAddress(sizeId, listId), getScalingListDefaultAddress(sizeId, listId), sizeof(int) * X265_MIN(MAX_MATRIX_COEF_NUM, (int)g_scalingListSize[sizeId]));
    setScalingListDC(sizeId, listId, SCALING_LIST_DC);
}

/** check DC value of matrix for default matrix signaling
 */
void TComScalingList::checkDcOfMatrix()
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
