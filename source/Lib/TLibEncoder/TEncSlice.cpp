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

/** \file     TEncSlice.cpp
    \brief    slice encoder class
*/

#include "TEncTop.h"
#include "TEncSlice.h"
#include "PPA/ppa.h"
#include <math.h>

//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TEncSlice::TEncSlice()
{
    m_apcPicYuvPred = NULL;
    m_apcPicYuvResi = NULL;
    m_pcBufferSbacCoders = NULL;
    m_pcBufferBinCoderCABACs = NULL;
    m_pcBufferLowLatSbacCoders = NULL;
    m_pcBufferLowLatBinCoderCABACs = NULL;
}

TEncSlice::~TEncSlice()
{
    for (std::vector<TEncSbac*>::iterator i = CTXMem.begin(); i != CTXMem.end(); i++)
    {
        delete (*i);
    }
}

Void TEncSlice::initCtxMem(UInt i)
{
    for (std::vector<TEncSbac*>::iterator j = CTXMem.begin(); j != CTXMem.end(); j++)
    {
        delete (*j);
    }

    CTXMem.clear();
    CTXMem.resize(i);
}

Void TEncSlice::create(Int iWidth, Int iHeight, UInt iMaxCUWidth, UInt iMaxCUHeight, UChar uhTotalDepth)
{
    // create prediction picture
    if (m_apcPicYuvPred == NULL)
    {
        m_apcPicYuvPred  = new TComPicYuv;
        m_apcPicYuvPred->create(iWidth, iHeight, iMaxCUWidth, iMaxCUHeight, uhTotalDepth);
    }

    // create residual picture
    if (m_apcPicYuvResi == NULL)
    {
        m_apcPicYuvResi  = new TComPicYuv;
        m_apcPicYuvResi->create(iWidth, iHeight, iMaxCUWidth, iMaxCUHeight, uhTotalDepth);
    }
}

Void TEncSlice::destroy()
{
    // destroy prediction picture
    if (m_apcPicYuvPred)
    {
        m_apcPicYuvPred->destroy();
        delete m_apcPicYuvPred;
        m_apcPicYuvPred  = NULL;
    }

    // destroy residual picture
    if (m_apcPicYuvResi)
    {
        m_apcPicYuvResi->destroy();
        delete m_apcPicYuvResi;
        m_apcPicYuvResi  = NULL;
    }

    if (m_pcBufferSbacCoders)
    {
        delete[] m_pcBufferSbacCoders;
    }
    if (m_pcBufferBinCoderCABACs)
    {
        delete[] m_pcBufferBinCoderCABACs;
    }
    if (m_pcBufferLowLatSbacCoders)
        delete[] m_pcBufferLowLatSbacCoders;
    if (m_pcBufferLowLatBinCoderCABACs)
        delete[] m_pcBufferLowLatBinCoderCABACs;
}

Void TEncSlice::init(TEncTop* pcEncTop)
{
    m_pcCfg             = pcEncTop;
    m_pcListPic         = pcEncTop->getListPic();

    m_pcGOPEncoder      = pcEncTop->getGOPEncoder();
    m_pcCuEncoder       = pcEncTop->getCuEncoder();
    m_pcPredSearch      = pcEncTop->getPredSearch();

    m_pcEntropyCoder    = pcEncTop->getEntropyCoder();
    m_pcCavlcCoder      = pcEncTop->getCavlcCoder();
    m_pcSbacCoder       = pcEncTop->getSbacCoder();
    m_pcBinCABAC        = pcEncTop->getBinCABAC();
    m_pcTrQuant         = pcEncTop->getTrQuant();

    m_pcBitCounter      = pcEncTop->getBitCounter();
    m_pcRdCost          = pcEncTop->getRdCost();
    m_pppcRDSbacCoder   = pcEncTop->getRDSbacCoder();
    m_pcRDGoOnSbacCoder = pcEncTop->getRDGoOnSbacCoder();

    m_pcRateCtrl        = pcEncTop->getRateCtrl();
}

/**
 - non-referenced frame marking
 - QP computation based on temporal structure
 - lambda computation based on QP
 - set temporal layer ID and the parameter sets
 .
 \param pcPic         picture class
 \param pocLast       POC of last picture
 \param pocCurr       current POC
 \param iNumPicRcvd   number of received pictures
 \param iTimeOffset   POC offset for hierarchical structure
 \param iDepth        temporal layer depth
 \param rpcSlice      slice header class
 \param pSPS          SPS associated with the slice
 \param pPPS          PPS associated with the slice
 */
Void TEncSlice::initEncSlice(TComPic* pcPic, Int pocLast, Int pocCurr, Int iNumPicRcvd, Int iGOPid, TComSlice*& rpcSlice, TComSPS* pSPS, TComPPS *pPPS)
{
    Double dQP;
    Double dLambda;

    rpcSlice = pcPic->getSlice(0);
    rpcSlice->setSPS(pSPS);
    rpcSlice->setPPS(pPPS);
    rpcSlice->setSliceBits(0);
    rpcSlice->setPic(pcPic);
    rpcSlice->initSlice();
    rpcSlice->setPicOutputFlag(true);
    rpcSlice->setPOC(pocCurr);

    // depth computation based on GOP size
    Int depth;
    {
        Int poc = rpcSlice->getPOC() % m_pcCfg->getGOPSize();
        if (poc == 0)
        {
            depth = 0;
        }
        else
        {
            Int step = m_pcCfg->getGOPSize();
            depth    = 0;
            for (Int i = step >> 1; i >= 1; i >>= 1)
            {
                for (Int j = i; j < m_pcCfg->getGOPSize(); j += step)
                {
                    if (j == poc)
                    {
                        i = 0;
                        break;
                    }
                }

                step >>= 1;
                depth++;
            }
        }
    }

    // slice type
    SliceType eSliceType;

    eSliceType = B_SLICE;
    eSliceType = (pocLast == 0 || pocCurr % m_pcCfg->getIntraPeriod() == 0 || m_pcGOPEncoder->getGOPSize() == 0) ? I_SLICE : eSliceType;

    rpcSlice->setSliceType(eSliceType);

    // ------------------------------------------------------------------------------------------------------------------
    // Non-referenced frame marking
    // ------------------------------------------------------------------------------------------------------------------

    if (pocLast == 0)
    {
        rpcSlice->setTemporalLayerNonReferenceFlag(false);
    }
    else
    {
        rpcSlice->setTemporalLayerNonReferenceFlag(!m_pcCfg->getGOPEntry(iGOPid).m_refPic);
    }
    rpcSlice->setReferenced(true);

    // ------------------------------------------------------------------------------------------------------------------
    // QP setting
    // ------------------------------------------------------------------------------------------------------------------

    dQP = m_pcCfg->getQP();
    if (eSliceType != I_SLICE)
    {
        if (!((dQP == -rpcSlice->getSPS()->getQpBDOffsetY()) && (rpcSlice->getSPS()->getUseLossless())))
        {
            dQP += m_pcCfg->getGOPEntry(iGOPid).m_QPOffset;
        }
    }

    // modify QP
    Int* pdQPs = m_pcCfg->getdQPs();
    if (pdQPs)
    {
        dQP += pdQPs[rpcSlice->getPOC()];
    }
    // ------------------------------------------------------------------------------------------------------------------
    // Lambda computation
    // ------------------------------------------------------------------------------------------------------------------

    Int iQP;

    // compute lambda value
    Int    NumberBFrames = (m_pcCfg->getGOPSize() - 1);
    Int    SHIFT_QP = 12;
    Double dLambda_scale = 1.0 - Clip3(0.0, 0.5, 0.05 * (Double)NumberBFrames);
#if FULL_NBIT
    Int    bitdepth_luma_qp_scale = 6 * (g_bitDepth - 8);
#else
    Int    bitdepth_luma_qp_scale = 0;
#endif
    Double qp_temp = (Double)dQP + bitdepth_luma_qp_scale - SHIFT_QP;
#if FULL_NBIT
    Double qp_temp_orig = (Double)dQP - SHIFT_QP;
#endif
    // Case #1: I or P-slices (key-frame)
    Double dQPFactor = m_pcCfg->getGOPEntry(iGOPid).m_QPFactor;
    if (eSliceType == I_SLICE)
    {
        dQPFactor = 0.57 * dLambda_scale;
    }
    dLambda = dQPFactor * pow(2.0, qp_temp / 3.0);

    if (depth > 0)
    {
#if FULL_NBIT
        dLambda *= Clip3(2.00, 4.00, (qp_temp_orig / 6.0)); // (j == B_SLICE && p_cur_frm->layer != 0 )
#else
        dLambda *= Clip3(2.00, 4.00, (qp_temp / 6.0)); // (j == B_SLICE && p_cur_frm->layer != 0 )
#endif
    }

    // if hadamard is used in ME process
    if (!m_pcCfg->getUseHADME() && rpcSlice->getSliceType() != I_SLICE)
    {
        dLambda *= 0.95;
    }

    iQP = max(-pSPS->getQpBDOffsetY(), min(MAX_QP, (Int)floor(dQP + 0.5)));

    if (rpcSlice->getSliceType() != I_SLICE)
    {
        dLambda *= m_pcCfg->getLambdaModifier(m_pcCfg->getGOPEntry(iGOPid).m_temporalId);
    }

    // store lambda
    m_pcRdCost->setLambda(dLambda);

    // for RDO
    // in RdCost there is only one lambda because the luma and chroma bits are not separated, instead we weight the distortion of chroma.
    Double weight = 1.0;
    Int qpc;
    Int chromaQPOffset;

    chromaQPOffset = rpcSlice->getPPS()->getChromaCbQpOffset() + rpcSlice->getSliceQpDeltaCb();
    qpc = Clip3(0, 57, iQP + chromaQPOffset);
    weight = pow(2.0, (iQP - g_aucChromaScale[qpc]) / 3.0); // takes into account of the chroma qp mapping and chroma qp Offset
    m_pcRdCost->setCbDistortionWeight(weight);

    chromaQPOffset = rpcSlice->getPPS()->getChromaCrQpOffset() + rpcSlice->getSliceQpDeltaCr();
    qpc = Clip3(0, 57, iQP + chromaQPOffset);
    weight = pow(2.0, (iQP - g_aucChromaScale[qpc]) / 3.0); // takes into account of the chroma qp mapping and chroma qp Offset
    m_pcRdCost->setCrDistortionWeight(weight);

#if RDOQ_CHROMA_LAMBDA
    // for RDOQ
    m_pcTrQuant->setLambda(dLambda, dLambda / weight);
#else
    m_pcTrQuant->setLambda(dLambda);
#endif

#if SAO_CHROMA_LAMBDA
    // For SAO
    rpcSlice->setLambda(dLambda, dLambda / weight);
#else
    rpcSlice->setLambda(dLambda);
#endif

#if HB_LAMBDA_FOR_LDC
    // restore original slice type
    eSliceType = (pocLast == 0 || pocCurr % m_pcCfg->getIntraPeriod() == 0 || m_pcGOPEncoder->getGOPSize() == 0) ? I_SLICE : eSliceType;

    rpcSlice->setSliceType(eSliceType);
#endif

    if (m_pcCfg->getUseRecalculateQPAccordingToLambda())
    {
        dQP = xGetQPValueAccordingToLambda(dLambda);
        iQP = max(-pSPS->getQpBDOffsetY(), min(MAX_QP, (Int)floor(dQP + 0.5)));
    }

    rpcSlice->setSliceQp(iQP);
    rpcSlice->setSliceQpBase(iQP);
    rpcSlice->setSliceQpDelta(0);
    rpcSlice->setSliceQpDeltaCb(0);
    rpcSlice->setSliceQpDeltaCr(0);
    rpcSlice->setNumRefIdx(REF_PIC_LIST_0, m_pcCfg->getGOPEntry(iGOPid).m_numRefPicsActive);
    rpcSlice->setNumRefIdx(REF_PIC_LIST_1, m_pcCfg->getGOPEntry(iGOPid).m_numRefPicsActive);

    if (m_pcCfg->getDeblockingFilterMetric())
    {
        rpcSlice->setDeblockingFilterOverrideFlag(true);
        rpcSlice->setDeblockingFilterDisable(false);
        rpcSlice->setDeblockingFilterBetaOffsetDiv2(0);
        rpcSlice->setDeblockingFilterTcOffsetDiv2(0);
    }
    else if (rpcSlice->getPPS()->getDeblockingFilterControlPresentFlag())
    {
        rpcSlice->getPPS()->setDeblockingFilterOverrideEnabledFlag(!m_pcCfg->getLoopFilterOffsetInPPS());
        rpcSlice->setDeblockingFilterOverrideFlag(!m_pcCfg->getLoopFilterOffsetInPPS());
        rpcSlice->getPPS()->setPicDisableDeblockingFilterFlag(m_pcCfg->getLoopFilterDisable());
        rpcSlice->setDeblockingFilterDisable(m_pcCfg->getLoopFilterDisable());
        if (!rpcSlice->getDeblockingFilterDisable())
        {
            if (!m_pcCfg->getLoopFilterOffsetInPPS() && eSliceType != I_SLICE)
            {
                rpcSlice->getPPS()->setDeblockingFilterBetaOffsetDiv2(m_pcCfg->getGOPEntry(iGOPid).m_betaOffsetDiv2 + m_pcCfg->getLoopFilterBetaOffset());
                rpcSlice->getPPS()->setDeblockingFilterTcOffsetDiv2(m_pcCfg->getGOPEntry(iGOPid).m_tcOffsetDiv2 + m_pcCfg->getLoopFilterTcOffset());
                rpcSlice->setDeblockingFilterBetaOffsetDiv2(m_pcCfg->getGOPEntry(iGOPid).m_betaOffsetDiv2 + m_pcCfg->getLoopFilterBetaOffset());
                rpcSlice->setDeblockingFilterTcOffsetDiv2(m_pcCfg->getGOPEntry(iGOPid).m_tcOffsetDiv2 + m_pcCfg->getLoopFilterTcOffset());
            }
            else
            {
                rpcSlice->getPPS()->setDeblockingFilterBetaOffsetDiv2(m_pcCfg->getLoopFilterBetaOffset());
                rpcSlice->getPPS()->setDeblockingFilterTcOffsetDiv2(m_pcCfg->getLoopFilterTcOffset());
                rpcSlice->setDeblockingFilterBetaOffsetDiv2(m_pcCfg->getLoopFilterBetaOffset());
                rpcSlice->setDeblockingFilterTcOffsetDiv2(m_pcCfg->getLoopFilterTcOffset());
            }
        }
    }
    else
    {
        rpcSlice->setDeblockingFilterOverrideFlag(false);
        rpcSlice->setDeblockingFilterDisable(false);
        rpcSlice->setDeblockingFilterBetaOffsetDiv2(0);
        rpcSlice->setDeblockingFilterTcOffsetDiv2(0);
    }

    rpcSlice->setDepth(depth);

    pcPic->setTLayer(m_pcCfg->getGOPEntry(iGOPid).m_temporalId);
    if (eSliceType == I_SLICE)
    {
        pcPic->setTLayer(0);
    }
    rpcSlice->setTLayer(pcPic->getTLayer());

    assert(m_apcPicYuvPred);
    assert(m_apcPicYuvResi);

    pcPic->setPicYuvPred(m_apcPicYuvPred);
    pcPic->setPicYuvResi(m_apcPicYuvResi);
    rpcSlice->setMaxNumMergeCand(m_pcCfg->getMaxNumMergeCand());
    xStoreWPparam(pPPS->getUseWP(), pPPS->getWPBiPred());
}

Void TEncSlice::resetQP(TComPic* pic, Int sliceQP, Double lambda)
{
    TComSlice* slice = pic->getSlice(0);

    // store lambda
    slice->setSliceQp(sliceQP);
    slice->setSliceQpBase(sliceQP);
    m_pcRdCost->setLambda(lambda);
    // for RDO
    // in RdCost there is only one lambda because the luma and chroma bits are not separated, instead we weight the distortion of chroma.
    Double weight;
    Int qpc;
    Int chromaQPOffset;

    chromaQPOffset = slice->getPPS()->getChromaCbQpOffset() + slice->getSliceQpDeltaCb();
    qpc = Clip3(0, 57, sliceQP + chromaQPOffset);
    weight = pow(2.0, (sliceQP - g_aucChromaScale[qpc]) / 3.0); // takes into account of the chroma qp mapping and chroma qp Offset
    m_pcRdCost->setCbDistortionWeight(weight);

    chromaQPOffset = slice->getPPS()->getChromaCrQpOffset() + slice->getSliceQpDeltaCr();
    qpc = Clip3(0, 57, sliceQP + chromaQPOffset);
    weight = pow(2.0, (sliceQP - g_aucChromaScale[qpc]) / 3.0); // takes into account of the chroma qp mapping and chroma qp Offset
    m_pcRdCost->setCrDistortionWeight(weight);

#if RDOQ_CHROMA_LAMBDA
    // for RDOQ
    m_pcTrQuant->setLambda(lambda, lambda / weight);
#else
    m_pcTrQuant->setLambda(lambda);
#endif

#if SAO_CHROMA_LAMBDA
    // For SAO
    slice->setLambda(lambda, lambda / weight);
#else
    slice->setLambda(lambda);
#endif
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

Void TEncSlice::setSearchRange(TComSlice* pcSlice)
{
    Int iCurrPOC = pcSlice->getPOC();
    Int iRefPOC;
    Int iGOPSize = m_pcCfg->getGOPSize();
    Int iOffset = (iGOPSize >> 1);
    Int iMaxSR = m_pcCfg->getSearchRange();
    Int iNumPredDir = pcSlice->isInterP() ? 1 : 2;

    for (Int iDir = 0; iDir <= iNumPredDir; iDir++)
    {
        //RefPicList e = (RefPicList)iDir;
        RefPicList  e = (iDir ? REF_PIC_LIST_1 : REF_PIC_LIST_0);
        for (Int iRefIdx = 0; iRefIdx < pcSlice->getNumRefIdx(e); iRefIdx++)
        {
            iRefPOC = pcSlice->getRefPic(e, iRefIdx)->getPOC();
            Int iNewSR = Clip3(8, iMaxSR, (iMaxSR * ADAPT_SR_SCALE * abs(iCurrPOC - iRefPOC) + iOffset) / iGOPSize);
            m_pcPredSearch->setAdaptiveSearchRange(iDir, iRefIdx, iNewSR);
        }
    }
}

/** \param rpcPic   picture class
 */
Void TEncSlice::compressSlice(TComPic* rpcPic)
{
    UInt  uiCUAddr;

    PPAScopeEvent(TEncSlice_compressSlice);

    rpcPic->getSlice(getSliceIdx())->setSliceSegmentBits(0);
    TEncBinCABAC* pppcRDSbacCoder = NULL;
    TComSlice* pcSlice            = rpcPic->getSlice(getSliceIdx());
    xDetermineStartAndBoundingCUAddr(rpcPic, false);

    // initialize cost values
    m_uiPicTotalBits  = 0;
    m_dPicRdCost      = 0;
    m_uiPicDist       = 0;

    // set entropy coder
    m_pcSbacCoder->init(m_pcBinCABAC);
    m_pcEntropyCoder->setEntropyCoder(m_pcSbacCoder, pcSlice);
    m_pcEntropyCoder->resetEntropy();
    m_pppcRDSbacCoder[0][CI_CURR_BEST]->load(m_pcSbacCoder);
    pppcRDSbacCoder = (TEncBinCABAC*)m_pppcRDSbacCoder[0][CI_CURR_BEST]->getEncBinIf();
    pppcRDSbacCoder->setBinCountingEnableFlag(false);
    pppcRDSbacCoder->setBinsCoded(0);

    //------------------------------------------------------------------------------
    //  Weighted Prediction parameters estimation.
    //------------------------------------------------------------------------------
    // calculate AC/DC values for current picture
    if (pcSlice->getPPS()->getUseWP() || pcSlice->getPPS()->getWPBiPred())
    {
        xCalcACDCParamSlice(pcSlice);
    }

    Bool bWp_explicit = (pcSlice->getSliceType() == P_SLICE && pcSlice->getPPS()->getUseWP()) || (pcSlice->getSliceType() == B_SLICE && pcSlice->getPPS()->getWPBiPred());

    if (bWp_explicit)
    {
        //------------------------------------------------------------------------------
        //  Weighted Prediction implemented at Slice level. SliceMode=2 is not supported yet.
        //------------------------------------------------------------------------------
        xEstimateWPParamSlice(pcSlice);
        pcSlice->initWpScaling();

        // check WP on/off
        xCheckWPEnable(pcSlice);
    }

    if (m_pcCfg->getUseAdaptQpSelect())
    {
        m_pcTrQuant->clearSliceARLCnt();
        if (pcSlice->getSliceType() != I_SLICE)
        {
            Int qpBase = pcSlice->getSliceQpBase();
            pcSlice->setSliceQp(qpBase + m_pcTrQuant->getQpDelta(qpBase));
        }
    }
    TEncTop* pcEncTop = (TEncTop*)m_pcCfg;
    TEncSbac**** ppppcRDSbacCoders    = pcEncTop->getRDSbacCoders();
    TComBitCounter* pcBitCounters     = pcEncTop->getBitCounters();
    const Bool bWaveFrontsynchro = m_pcCfg->getWaveFrontsynchro();
    const UInt uiWidthInLCUs  = rpcPic->getPicSym()->getFrameWidthInCU();
    const UInt uiHeightInLCUs = rpcPic->getPicSym()->getFrameHeightInCU();
    const Int  iNumSubstreams = (bWaveFrontsynchro ? uiHeightInLCUs : 1);

    // CHECK_ME: there seems WPP always start at every LCU line
    //assert( iNumSubstreams == pcSlice->getPPS()->getNumSubstreams() );

    delete[] m_pcBufferSbacCoders;
    delete[] m_pcBufferBinCoderCABACs;
    m_pcBufferSbacCoders     = new TEncSbac[1];
    m_pcBufferBinCoderCABACs = new TEncBinCABAC[1];
    m_pcBufferSbacCoders[0].init(&m_pcBufferBinCoderCABACs[0]);
    m_pcBufferSbacCoders[0].load(m_pppcRDSbacCoder[0][CI_CURR_BEST]); //init. state

    for (UInt ui = 0; ui < iNumSubstreams; ui++) //init all sbac coders for RD optimization
    {
        ppppcRDSbacCoders[ui][0][CI_CURR_BEST]->load(m_pppcRDSbacCoder[0][CI_CURR_BEST]);
    }

    delete[] m_pcBufferLowLatSbacCoders;
    delete[] m_pcBufferLowLatBinCoderCABACs;
    m_pcBufferLowLatSbacCoders     = new TEncSbac[1];
    m_pcBufferLowLatBinCoderCABACs = new TEncBinCABAC[1];
    m_pcBufferLowLatSbacCoders[0].init(&m_pcBufferLowLatBinCoderCABACs[0]);
    m_pcBufferLowLatSbacCoders[0].load(m_pppcRDSbacCoder[0][CI_CURR_BEST]); //init. state

    UInt uiCol = 0, uiLin = 0, uiSubStrm = 0;
    const UInt uiTotalCUs = rpcPic->getNumCUsInFrame();
    // CHECK_ME: in here, uiCol running uiWidthInLCUs times since "m_uiNumCUsInFrame = m_uiWidthInCU * m_uiHeightInCU;"
    assert((uiTotalCUs % uiWidthInLCUs) == 0);
    assert((uiTotalCUs / uiWidthInLCUs) == uiHeightInLCUs);

    // for every CU in slice
    for(uiLin = 0; uiLin < uiHeightInLCUs; uiLin++)
    {
        const UInt uiCurLineCUAddr = uiLin * uiWidthInLCUs;
        for(uiCol = 0; uiCol < uiWidthInLCUs; uiCol++)
        {
            uiCUAddr = uiCurLineCUAddr + uiCol;

            // initialize CU encoder
            TComDataCU* pcCU = rpcPic->getCU(uiCUAddr);
            pcCU->initCU(rpcPic, uiCUAddr);

            // inherit from TR if necessary, select substream to use.
            uiSubStrm = uiLin % iNumSubstreams;
            if ((iNumSubstreams > 1) && (uiCol == 0) && bWaveFrontsynchro)
            {
                // We'll sync if the TR is available.
                TComDataCU *pcCUUp = pcCU->getCUAbove();
                TComDataCU *pcCUTR = NULL;
                // CHECK_ME: we are here only (uiCol == 0), why HM use this complex statement to check
                if (pcCUUp/* && ((uiCUAddr % uiWidthInLCUs + 1) < uiWidthInLCUs)*/)
                {
                    pcCUTR = rpcPic->getCU(uiCUAddr - uiWidthInLCUs + 1);
                }
                if ((pcCUTR == NULL) || (pcCUTR->getSlice() == NULL))
                {
                    // TR not available.
                }
                else
                {
                    // TR is available, we use it.
                    ppppcRDSbacCoders[uiSubStrm][0][CI_CURR_BEST]->loadContexts(&m_pcBufferSbacCoders[0]);
                }
            }
            m_pppcRDSbacCoder[0][CI_CURR_BEST]->load(ppppcRDSbacCoders[uiSubStrm][0][CI_CURR_BEST]); //this load is used to simplify the code

            // set go-on entropy coder
            m_pcEntropyCoder->setEntropyCoder(m_pcRDGoOnSbacCoder, pcSlice);
            m_pcEntropyCoder->setBitstream(&pcBitCounters[uiSubStrm]);

            ((TEncBinCABAC*)m_pcRDGoOnSbacCoder->getEncBinIf())->setBinCountingEnableFlag(true);

            Double oldLambda = m_pcRdCost->getLambda();
            if (m_pcCfg->getUseRateCtrl())
            {
                Int estQP        = pcSlice->getSliceQp();
                Double estLambda = -1.0;
                Double bpp       = -1.0;

                if (rpcPic->getSlice(0)->getSliceType() == I_SLICE || !m_pcCfg->getLCULevelRC())
                {
                    estQP = pcSlice->getSliceQp();
                }
                else
                {
                    bpp       = m_pcRateCtrl->getRCPic()->getLCUTargetBpp();
                    estLambda = m_pcRateCtrl->getRCPic()->getLCUEstLambda(bpp);
                    estQP     = m_pcRateCtrl->getRCPic()->getLCUEstQP(estLambda, pcSlice->getSliceQp());
                    estQP     = Clip3(-pcSlice->getSPS()->getQpBDOffsetY(), MAX_QP, estQP);

                    m_pcRdCost->setLambda(estLambda);
                }

                m_pcRateCtrl->setRCQP(estQP);
                pcCU->getSlice()->setSliceQpBase(estQP);
            }

            // run CU encoder
            m_pcCuEncoder->compressCU(pcCU);

            if (m_pcCfg->getUseRateCtrl())
            {
                UInt SAD    = m_pcCuEncoder->getLCUPredictionSAD();
                Int height  = min(pcSlice->getSPS()->getMaxCUHeight(), pcSlice->getSPS()->getPicHeightInLumaSamples() - uiCUAddr / rpcPic->getFrameWidthInCU() * pcSlice->getSPS()->getMaxCUHeight());
                Int width   = min(pcSlice->getSPS()->getMaxCUWidth(), pcSlice->getSPS()->getPicWidthInLumaSamples() - uiCUAddr % rpcPic->getFrameWidthInCU() * pcSlice->getSPS()->getMaxCUWidth());
                Double MAD = (Double)SAD / (Double)(height * width);
                MAD = MAD * MAD;
                (m_pcRateCtrl->getRCPic()->getLCU(uiCUAddr)).m_MAD = MAD;

                Int actualQP        = g_RCInvalidQPValue;
                Double actualLambda = m_pcRdCost->getLambda();
                Int actualBits      = pcCU->getTotalBits();
                Int numberOfEffectivePixels    = 0;
                for (Int idx = 0; idx < rpcPic->getNumPartInCU(); idx++)
                {
                    if (pcCU->getPredictionMode(idx) != MODE_NONE && (!pcCU->isSkipped(idx)))
                    {
                        numberOfEffectivePixels = numberOfEffectivePixels + 16;
                        break;
                    }
                }

                if (numberOfEffectivePixels == 0)
                {
                    actualQP = g_RCInvalidQPValue;
                }
                else
                {
                    actualQP = pcCU->getQP(0);
                }
                m_pcRdCost->setLambda(oldLambda);

                m_pcRateCtrl->getRCPic()->updateAfterLCU(m_pcRateCtrl->getRCPic()->getLCUCoded(), actualBits, actualQP, actualLambda, m_pcCfg->getLCULevelRC());
            }

            // restore entropy coder to an initial stage
            m_pcEntropyCoder->setEntropyCoder(m_pppcRDSbacCoder[0][CI_CURR_BEST], pcSlice);
            m_pcEntropyCoder->setBitstream(&pcBitCounters[uiSubStrm]);
            m_pcCuEncoder->setBitCounter(&pcBitCounters[uiSubStrm]);
            m_pcBitCounter = &pcBitCounters[uiSubStrm];
            pppcRDSbacCoder->setBinCountingEnableFlag(true);
            m_pcBitCounter->resetBits();
            pppcRDSbacCoder->setBinsCoded(0);
            m_pcCuEncoder->encodeCU(pcCU);

            pppcRDSbacCoder->setBinCountingEnableFlag(false);
            ppppcRDSbacCoders[uiSubStrm][0][CI_CURR_BEST]->load(m_pppcRDSbacCoder[0][CI_CURR_BEST]);

            //Store probabilties of second LCU in line into buffer
            if ((uiCol == 1) && (iNumSubstreams > 1) && m_pcCfg->getWaveFrontsynchro())
            {
                m_pcBufferSbacCoders[0].loadContexts(ppppcRDSbacCoders[uiSubStrm][0][CI_CURR_BEST]);
            }

            m_uiPicTotalBits += pcCU->getTotalBits();
            m_dPicRdCost     += pcCU->getTotalCost();
            m_uiPicDist      += pcCU->getTotalDistortion();
        } // end of for(uiCol...
    } // end of for(uiLin...

    if (iNumSubstreams > 1)
    {
        pcSlice->setNextSlice(true);
    }
    xRestoreWPparam(pcSlice);
}

/**
 \param  rpcPic        picture class
 \retval rpcBitstream  bitstream class
 */
Void TEncSlice::encodeSlice(TComPic*& rpcPic, TComOutputBitstream* pcSubstreams)
{
    UInt       uiCUAddr;
    UInt       uiStartCUAddr;
    UInt       uiBoundingCUAddr;
    TComSlice* pcSlice = rpcPic->getSlice(getSliceIdx());

    uiStartCUAddr = 0;
    uiBoundingCUAddr = pcSlice->getSliceCurEndCUAddr();
    // choose entropy coder
    {
        m_pcSbacCoder->init((TEncBinIf*)m_pcBinCABAC);
        m_pcEntropyCoder->setEntropyCoder(m_pcSbacCoder, pcSlice);
    }

    m_pcCuEncoder->setBitCounter(NULL);
    m_pcBitCounter = NULL;
    // Appropriate substream bitstream is switched later.
    // for every CU
#if ENC_DEC_TRACE
    g_bJustDoIt = g_bEncDecTraceEnable;
#endif
    DTRACE_CABAC_VL(g_nSymbolCounter++);
    DTRACE_CABAC_T("\tPOC: ");
    DTRACE_CABAC_V(rpcPic->getPOC());
    DTRACE_CABAC_T("\n");
#if ENC_DEC_TRACE
    g_bJustDoIt = g_bEncDecTraceDisable;
#endif

    TEncTop* pcEncTop = (TEncTop*)m_pcCfg;
    TEncSbac* pcSbacCoders = pcEncTop->getSbacCoders(); //coder for each substream
    const Bool bWaveFrontsynchro = m_pcCfg->getWaveFrontsynchro();
    const UInt uiHeightInLCUs = rpcPic->getPicSym()->getFrameHeightInCU();
    const Int  iNumSubstreams = (bWaveFrontsynchro ? uiHeightInLCUs : 1);
    UInt uiBitsOriginallyInSubstreams = 0;
    {
        m_pcBufferSbacCoders[0].load(m_pcSbacCoder); //init. state

        for (Int iSubstrmIdx = 0; iSubstrmIdx < iNumSubstreams; iSubstrmIdx++)
        {
            uiBitsOriginallyInSubstreams += pcSubstreams[iSubstrmIdx].getNumberOfWrittenBits();
        }

        m_pcBufferLowLatSbacCoders[0].load(m_pcSbacCoder); //init. state
    }

    UInt uiWidthInLCUs  = rpcPic->getPicSym()->getFrameWidthInCU();
    UInt uiCol = 0, uiLin = 0, uiSubStrm = 0;
    uiCUAddr = (uiStartCUAddr / rpcPic->getNumPartInCU()); /* for tiles, uiStartCUAddr is NOT the real raster scan address, it is actually
                                                              an encoding order index, so we need to convert the index (uiStartCUAddr)
                                                              into the real raster scan address (uiCUAddr) via the CUOrderMap */
    UInt uiEncCUOrder;
    for (uiEncCUOrder = uiStartCUAddr / rpcPic->getNumPartInCU();
         uiEncCUOrder < (uiBoundingCUAddr + rpcPic->getNumPartInCU() - 1) / rpcPic->getNumPartInCU();
         uiCUAddr = (++uiEncCUOrder))
    {
        //UInt uiSliceStartLCU = pcSlice->getSliceCurStartCUAddr();
        uiCol     = uiCUAddr % uiWidthInLCUs;
        uiLin     = uiCUAddr / uiWidthInLCUs;
        uiSubStrm = uiLin % iNumSubstreams;

        m_pcEntropyCoder->setBitstream(&pcSubstreams[uiSubStrm]);
        // Synchronize cabac probabilities with upper-right LCU if it's available and we're at the start of a line.
        if ((iNumSubstreams > 1) && (uiCol == 0) && bWaveFrontsynchro)
        {
            // We'll sync if the TR is available.
            TComDataCU *pcCUUp = rpcPic->getCU(uiCUAddr)->getCUAbove();
            UInt uiWidthInCU = rpcPic->getFrameWidthInCU();
            UInt uiMaxParts = 1 << (pcSlice->getSPS()->getMaxCUDepth() << 1);
            TComDataCU *pcCUTR = NULL;
            // CHECK_ME: here can br optimize a little, do it later
            if (pcCUUp && ((uiCUAddr % uiWidthInCU + 1) < uiWidthInCU))
            {
                pcCUTR = rpcPic->getCU(uiCUAddr - uiWidthInCU + 1);
            }
            if (true /*bEnforceSliceRestriction*/ && ((pcCUTR == NULL) || (pcCUTR->getSlice() == NULL)))
            {
                // TR not available.
            }
            else
            {
                // TR is available, we use it.
                pcSbacCoders[uiSubStrm].loadContexts(&m_pcBufferSbacCoders[0]);
            }
        }
        m_pcSbacCoder->load(&pcSbacCoders[uiSubStrm]); //this load is used to simplify the code (avoid to change all the call to m_pcSbacCoder)

        TComDataCU* pcCU = rpcPic->getCU(uiCUAddr);
        if (pcSlice->getSPS()->getUseSAO() && (pcSlice->getSaoEnabledFlag() || pcSlice->getSaoEnabledFlagChroma()))
        {
            SAOParam *saoParam = pcSlice->getPic()->getPicSym()->getSaoParam();
            Int iNumCuInWidth     = saoParam->numCuInWidth;
            Int iCUAddrInSlice    = uiCUAddr;
            Int iCUAddrUpInSlice  = iCUAddrInSlice - iNumCuInWidth;
            Int rx = uiCUAddr % iNumCuInWidth;
            Int ry = uiCUAddr / iNumCuInWidth;
            Int allowMergeLeft = 1;
            Int allowMergeUp   = 1;
            Int addr = pcCU->getAddr();
            allowMergeLeft = (rx > 0) && (iCUAddrInSlice != 0);
            allowMergeUp = (ry > 0) && (iCUAddrUpInSlice >= 0);
            if (saoParam->bSaoFlag[0] || saoParam->bSaoFlag[1])
            {
                Int mergeLeft = saoParam->saoLcuParam[0][addr].mergeLeftFlag;
                Int mergeUp = saoParam->saoLcuParam[0][addr].mergeUpFlag;
                if (allowMergeLeft)
                {
                    m_pcEntropyCoder->m_pcEntropyCoderIf->codeSaoMerge(mergeLeft);
                }
                else
                {
                    mergeLeft = 0;
                }
                if (mergeLeft == 0)
                {
                    if (allowMergeUp)
                    {
                        m_pcEntropyCoder->m_pcEntropyCoderIf->codeSaoMerge(mergeUp);
                    }
                    else
                    {
                        mergeUp = 0;
                    }
                    if (mergeUp == 0)
                    {
                        for (Int compIdx = 0; compIdx < 3; compIdx++)
                        {
                            if ((compIdx == 0 && saoParam->bSaoFlag[0]) || (compIdx > 0 && saoParam->bSaoFlag[1]))
                            {
                                m_pcEntropyCoder->encodeSaoOffset(&saoParam->saoLcuParam[compIdx][addr], compIdx);
                            }
                        }
                    }
                }
            }
        }
        else if (pcSlice->getSPS()->getUseSAO())
        {
            Int addr = pcCU->getAddr();
            SAOParam *saoParam = pcSlice->getPic()->getPicSym()->getSaoParam();
            for (Int cIdx = 0; cIdx < 3; cIdx++)
            {
                SaoLcuParam *saoLcuParam = &(saoParam->saoLcuParam[cIdx][addr]);
                if (((cIdx == 0) && !pcSlice->getSaoEnabledFlag()) || ((cIdx == 1 || cIdx == 2) && !pcSlice->getSaoEnabledFlagChroma()))
                {
                    saoLcuParam->mergeUpFlag   = 0;
                    saoLcuParam->mergeLeftFlag = 0;
                    saoLcuParam->subTypeIdx    = 0;
                    saoLcuParam->typeIdx       = -1;
                    saoLcuParam->offset[0]     = 0;
                    saoLcuParam->offset[1]     = 0;
                    saoLcuParam->offset[2]     = 0;
                    saoLcuParam->offset[3]     = 0;
                }
            }
        }
#if ENC_DEC_TRACE
        g_bJustDoIt = g_bEncDecTraceEnable;
#endif
        m_pcCuEncoder->encodeCU(pcCU);
#if ENC_DEC_TRACE
        g_bJustDoIt = g_bEncDecTraceDisable;
#endif
        pcSbacCoders[uiSubStrm].load(m_pcSbacCoder); //load back status of the entropy coder after encoding the LCU into relevant bitstream entropy coder

        //Store probabilities of second LCU in line into buffer
        if ((iNumSubstreams > 1) && (uiCol == 1) && bWaveFrontsynchro)
        {
            m_pcBufferSbacCoders[0].loadContexts(&pcSbacCoders[uiSubStrm]);
        }
    }

    if (m_pcCfg->getUseAdaptQpSelect())
    {
        m_pcTrQuant->storeSliceQpNext(pcSlice);
    }
    if (pcSlice->getPPS()->getCabacInitPresentFlag())
    {
        m_pcEntropyCoder->determineCabacInitIdx();
    }
}

/** Determines the starting and bounding LCU address of current slice / dependent slice
 * \param bEncodeSlice Identifies if the calling function is compressSlice() [false] or encodeSlice() [true]
 * \returns Updates uiStartCUAddr, uiBoundingCUAddr with appropriate LCU address
 */
Void TEncSlice::xDetermineStartAndBoundingCUAddr(TComPic* rpcPic, Bool bEncodeSlice)
{
    TComSlice* pcSlice = rpcPic->getSlice(getSliceIdx());
    UInt uiBoundingCUAddrSlice;

    UInt uiNumberOfCUsInFrame = rpcPic->getNumCUsInFrame();

    uiBoundingCUAddrSlice = uiNumberOfCUsInFrame * rpcPic->getNumPartInCU();

    // WPP: if a slice does not start at the beginning of a CTB row, it must end within the same CTB row
    pcSlice->setSliceCurEndCUAddr(uiBoundingCUAddrSlice);

    if (!bEncodeSlice)
    {
        // For fixed number of LCU within an entropy and reconstruction slice we already know whether we will encounter end of entropy and/or reconstruction slice
        // first. Set the flags accordingly.
        pcSlice->setNextSlice(false);
    }
}

Double TEncSlice::xGetQPValueAccordingToLambda(Double lambda)
{
    return 4.2005 * log(lambda) + 13.7122;
}

//! \}
