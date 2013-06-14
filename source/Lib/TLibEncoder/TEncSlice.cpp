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
#include "common.h"
#include "threading.h"
#include "wavefront.h"

#include <math.h>

using namespace x265;

//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TEncSlice::TEncSlice()
{
    m_apcPicYuvPred = NULL;
    m_apcPicYuvResi = NULL;
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
}

Void TEncSlice::init(TEncTop* pcEncTop)
{
    m_pcCfg             = pcEncTop;
    m_pcListPic         = pcEncTop->getListPic();
    m_pcGOPEncoder      = pcEncTop->getGOPEncoder();
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
 \param iTimeOffset   POC offset for hierarchical structure
 \param iDepth        temporal layer depth
 \param pSPS          SPS associated with the slice
 \param pPPS          PPS associated with the slice
 */
TComSlice* TEncSlice::initEncSlice(TComPic* pcPic, x265::EncodeFrame *pcEncodeFrame, Int pocLast, Int pocCurr, Int iGOPid, TComSPS* pSPS, TComPPS *pPPS)
{
    Double dQP;
    Double dLambda;

    TComSlice* pcSlice = pcPic->getSlice(0);
    pcSlice->setSPS(pSPS);
    pcSlice->setPPS(pPPS);
    pcSlice->setSliceBits(0);
    pcSlice->setPic(pcPic);
    pcSlice->initSlice();
    pcSlice->setPicOutputFlag(true);
    pcSlice->setPOC(pocCurr);

    // depth computation based on GOP size
    Int depth;
    {
        Int poc = pcSlice->getPOC() % m_pcCfg->getGOPSize();
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

    pcSlice->setSliceType(eSliceType);

    // ------------------------------------------------------------------------------------------------------------------
    // Non-referenced frame marking
    // ------------------------------------------------------------------------------------------------------------------

    if (pocLast == 0)
    {
        pcSlice->setTemporalLayerNonReferenceFlag(false);
    }
    else
    {
        pcSlice->setTemporalLayerNonReferenceFlag(!m_pcCfg->getGOPEntry(iGOPid).m_refPic);
    }
    pcSlice->setReferenced(true);

    // ------------------------------------------------------------------------------------------------------------------
    // QP setting
    // ------------------------------------------------------------------------------------------------------------------

    dQP = m_pcCfg->getQP();
    if (eSliceType != I_SLICE)
    {
        if (!((dQP == -pcSlice->getSPS()->getQpBDOffsetY()) && (pcSlice->getSPS()->getUseLossless())))
        {
            dQP += m_pcCfg->getGOPEntry(iGOPid).m_QPOffset;
        }
    }

    // modify QP
    Int* pdQPs = m_pcCfg->getdQPs();
    if (pdQPs)
    {
        dQP += pdQPs[pcSlice->getPOC()];
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
    if (!m_pcCfg->getUseHADME() && pcSlice->getSliceType() != I_SLICE)
    {
        dLambda *= 0.95;
    }

    iQP = max(-pSPS->getQpBDOffsetY(), min(MAX_QP, (Int)floor(dQP + 0.5)));

    if (pcSlice->getSliceType() != I_SLICE)
    {
        dLambda *= m_pcCfg->getLambdaModifier(m_pcCfg->getGOPEntry(iGOPid).m_temporalId);
    }

    // for RDO
    // in RdCost there is only one lambda because the luma and chroma bits are not separated, instead we weight the distortion of chroma.
    Double weight = 1.0;
    Int qpc;
    Int chromaQPOffset;

    chromaQPOffset = pcSlice->getPPS()->getChromaCbQpOffset() + pcSlice->getSliceQpDeltaCb();
    qpc = Clip3(0, 57, iQP + chromaQPOffset);
    weight = pow(2.0, (iQP - g_aucChromaScale[qpc]) / 3.0); // takes into account of the chroma qp mapping and chroma qp Offset
    pcEncodeFrame->setCbDistortionWeight(weight);

    chromaQPOffset = pcSlice->getPPS()->getChromaCrQpOffset() + pcSlice->getSliceQpDeltaCr();
    qpc = Clip3(0, 57, iQP + chromaQPOffset);
    weight = pow(2.0, (iQP - g_aucChromaScale[qpc]) / 3.0); // takes into account of the chroma qp mapping and chroma qp Offset
    pcEncodeFrame->setCrDistortionWeight(weight);

    // for RDOQ
    pcEncodeFrame->setLambda(dLambda, dLambda / weight);

    // For SAO
    pcSlice->setLambda(dLambda, dLambda / weight);

#if HB_LAMBDA_FOR_LDC
    // restore original slice type
    eSliceType = (pocLast == 0 || pocCurr % m_pcCfg->getIntraPeriod() == 0 || m_pcGOPEncoder->getGOPSize() == 0) ? I_SLICE : eSliceType;

    pcSlice->setSliceType(eSliceType);
#endif

    if (m_pcCfg->getUseRecalculateQPAccordingToLambda())
    {
        dQP = xGetQPValueAccordingToLambda(dLambda);
        iQP = max(-pSPS->getQpBDOffsetY(), min(MAX_QP, (Int)floor(dQP + 0.5)));
    }

    pcSlice->setSliceQp(iQP);
    pcSlice->setSliceQpBase(iQP);
    pcSlice->setSliceQpDelta(0);
    pcSlice->setSliceQpDeltaCb(0);
    pcSlice->setSliceQpDeltaCr(0);
    pcSlice->setNumRefIdx(REF_PIC_LIST_0, m_pcCfg->getGOPEntry(iGOPid).m_numRefPicsActive);
    pcSlice->setNumRefIdx(REF_PIC_LIST_1, m_pcCfg->getGOPEntry(iGOPid).m_numRefPicsActive);

    if (m_pcCfg->getDeblockingFilterMetric())
    {
        pcSlice->setDeblockingFilterOverrideFlag(true);
        pcSlice->setDeblockingFilterDisable(false);
        pcSlice->setDeblockingFilterBetaOffsetDiv2(0);
        pcSlice->setDeblockingFilterTcOffsetDiv2(0);
    }
    else if (pcSlice->getPPS()->getDeblockingFilterControlPresentFlag())
    {
        pcSlice->getPPS()->setDeblockingFilterOverrideEnabledFlag(!m_pcCfg->getLoopFilterOffsetInPPS());
        pcSlice->setDeblockingFilterOverrideFlag(!m_pcCfg->getLoopFilterOffsetInPPS());
        pcSlice->getPPS()->setPicDisableDeblockingFilterFlag(m_pcCfg->getLoopFilterDisable());
        pcSlice->setDeblockingFilterDisable(m_pcCfg->getLoopFilterDisable());
        if (!pcSlice->getDeblockingFilterDisable())
        {
            if (!m_pcCfg->getLoopFilterOffsetInPPS() && eSliceType != I_SLICE)
            {
                pcSlice->getPPS()->setDeblockingFilterBetaOffsetDiv2(m_pcCfg->getGOPEntry(iGOPid).m_betaOffsetDiv2 + m_pcCfg->getLoopFilterBetaOffset());
                pcSlice->getPPS()->setDeblockingFilterTcOffsetDiv2(m_pcCfg->getGOPEntry(iGOPid).m_tcOffsetDiv2 + m_pcCfg->getLoopFilterTcOffset());
                pcSlice->setDeblockingFilterBetaOffsetDiv2(m_pcCfg->getGOPEntry(iGOPid).m_betaOffsetDiv2 + m_pcCfg->getLoopFilterBetaOffset());
                pcSlice->setDeblockingFilterTcOffsetDiv2(m_pcCfg->getGOPEntry(iGOPid).m_tcOffsetDiv2 + m_pcCfg->getLoopFilterTcOffset());
            }
            else
            {
                pcSlice->getPPS()->setDeblockingFilterBetaOffsetDiv2(m_pcCfg->getLoopFilterBetaOffset());
                pcSlice->getPPS()->setDeblockingFilterTcOffsetDiv2(m_pcCfg->getLoopFilterTcOffset());
                pcSlice->setDeblockingFilterBetaOffsetDiv2(m_pcCfg->getLoopFilterBetaOffset());
                pcSlice->setDeblockingFilterTcOffsetDiv2(m_pcCfg->getLoopFilterTcOffset());
            }
        }
    }
    else
    {
        pcSlice->setDeblockingFilterOverrideFlag(false);
        pcSlice->setDeblockingFilterDisable(false);
        pcSlice->setDeblockingFilterBetaOffsetDiv2(0);
        pcSlice->setDeblockingFilterTcOffsetDiv2(0);
    }

    pcSlice->setDepth(depth);

    pcPic->setTLayer(m_pcCfg->getGOPEntry(iGOPid).m_temporalId);
    if (eSliceType == I_SLICE)
    {
        pcPic->setTLayer(0);
    }
    pcSlice->setTLayer(pcPic->getTLayer());

    assert(m_apcPicYuvPred);
    assert(m_apcPicYuvResi);

    pcPic->setPicYuvPred(m_apcPicYuvPred);
    pcPic->setPicYuvResi(m_apcPicYuvResi);
    pcSlice->setMaxNumMergeCand(m_pcCfg->getMaxNumMergeCand());
    xStoreWPparam(pPPS->getUseWP(), pPPS->getWPBiPred());
    return pcSlice;
}

Void TEncSlice::resetQP(TComPic* pic, EncodeFrame *pcEncodeFrame, Int sliceQP, Double lambda)
{
    TComSlice* slice = pic->getSlice(0);

    // store lambda
    slice->setSliceQp(sliceQP);
    slice->setSliceQpBase(sliceQP);

    // for RDO
    // in RdCost there is only one lambda because the luma and chroma bits are not separated, instead we weight the distortion of chroma.
    Double weight;
    Int qpc;
    Int chromaQPOffset;

    chromaQPOffset = slice->getPPS()->getChromaCbQpOffset() + slice->getSliceQpDeltaCb();
    qpc = Clip3(0, 57, sliceQP + chromaQPOffset);
    weight = pow(2.0, (sliceQP - g_aucChromaScale[qpc]) / 3.0); // takes into account of the chroma qp mapping and chroma qp Offset
    pcEncodeFrame->setCbDistortionWeight(weight);

    chromaQPOffset = slice->getPPS()->getChromaCrQpOffset() + slice->getSliceQpDeltaCr();
    qpc = Clip3(0, 57, sliceQP + chromaQPOffset);
    weight = pow(2.0, (sliceQP - g_aucChromaScale[qpc]) / 3.0); // takes into account of the chroma qp mapping and chroma qp Offset
    pcEncodeFrame->setCrDistortionWeight(weight);

    // for RDOQ
    pcEncodeFrame->setLambda(lambda, lambda / weight);

    // For SAO
    slice->setLambda(lambda, lambda / weight);
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

Void TEncSlice::setSearchRange(TComSlice* pcSlice, EncodeFrame *pcEncodeframe)
{
    Int iCurrPOC = pcSlice->getPOC();
    Int iGOPSize = m_pcCfg->getGOPSize();
    Int iOffset = (iGOPSize >> 1);
    Int iMaxSR = m_pcCfg->getSearchRange();
    Int iNumPredDir = pcSlice->isInterP() ? 1 : 2;

    for (Int iDir = 0; iDir <= iNumPredDir; iDir++)
    {
        RefPicList  e = (iDir ? REF_PIC_LIST_1 : REF_PIC_LIST_0);
        for (Int iRefIdx = 0; iRefIdx < pcSlice->getNumRefIdx(e); iRefIdx++)
        {
            Int iRefPOC = pcSlice->getRefPic(e, iRefIdx)->getPOC();
            Int iNewSR = Clip3(8, iMaxSR, (iMaxSR * ADAPT_SR_SCALE * abs(iCurrPOC - iRefPOC) + iOffset) / iGOPSize);

            pcEncodeframe->setAdaptiveSearchRange(iDir, iRefIdx, iNewSR);
        }
    }
}

/** \param rpcPic   picture class
 */
#if CU_STAT_LOGFILE
int cntInter[4], cntIntra[4], cntSplit[4],  totalCU;
int cuInterDistribution[4][4], cuIntraDistribution[4][3], cntIntraNxN;
int cntSkipCu[4], cntTotalCu[4];
extern FILE * fp, * fp1;
#endif
Void TEncSlice::compressSlice(TComPic* rpcPic, EncodeFrame* pcEncodeFrame)
{
    PPAScopeEvent(TEncSlice_compressSlice);

#if CU_STAT_LOGFILE

    for (int i = 0; i < 4; i++)
    {
        cntInter[i] = 0;
        cntIntra[i] = 0;
        cntSplit[i] = 0;
        cntSkipCu[i] = 0;
        cntTotalCu[i] = 0;
        for (int j = 0; j < 4; j++)
        {
            if (j < 3)
            {
                cuIntraDistribution[i][j] = 0;
            }
            cuInterDistribution[i][j] = 0;
        }
    }

    totalCU = 0;
    cntIntraNxN = 0;
#endif // if LOGGING

    rpcPic->getSlice(getSliceIdx())->setSliceSegmentBits(0);
    TComSlice* pcSlice = rpcPic->getSlice(getSliceIdx());
    xDetermineStartAndBoundingCUAddr(rpcPic, false);

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
        // TODO: fix this option
        assert(0);
#if 0
        m_pcTrQuant->clearSliceARLCnt();
        if (pcSlice->getSliceType() != I_SLICE)
        {
            Int qpBase = pcSlice->getSliceQpBase();
            pcSlice->setSliceQp(qpBase + m_pcTrQuant->getQpDelta(qpBase));
        }
#endif
    }

    pcEncodeFrame->Encode(rpcPic, pcSlice);

    if (m_pcCfg->getWaveFrontsynchro())
    {
        pcSlice->setNextSlice(true);
    }

    xRestoreWPparam(pcSlice);
#if CU_STAT_LOGFILE
    if (pcSlice->getSliceType() == P_SLICE)
    {
        fprintf(fp, " FRAME  - P FRAME \n");
        fprintf(fp, "64x64 : Inter [%.2f %%  (2Nx2N %.2f %%,  Nx2N %.2f %% , 2NxN %.2f %%, AMP %.2f %%)] Intra [%.2f %%(DC %.2f %% Planar %.2f %% Ang %.2f%% )] Split[%.2f] %% Skipped[%.2f]%% \n", (double)(cntInter[0] * 100) / cntTotalCu[0], (double)(cuInterDistribution[0][0] * 100) / cntInter[0],  (double)(cuInterDistribution[0][2] * 100) / cntInter[0], (double)(cuInterDistribution[0][1] * 100) / cntInter[0], (double)(cuInterDistribution[0][3] * 100) / cntInter[0], (double)(cntIntra[0] * 100) / cntTotalCu[0], (double)(cuIntraDistribution[0][0] * 100) / cntIntra[0], (double)(cuIntraDistribution[0][1] * 100) / cntIntra[0], (double)(cuIntraDistribution[0][2] * 100) / cntIntra[0], (double)(cntSplit[0] * 100) / cntTotalCu[0], (double)(cntSkipCu[0] * 100) / cntTotalCu[0]);
        fprintf(fp, "32x32 : Inter [%.2f %% (2Nx2N %.2f %%,  Nx2N %.2f %%, 2NxN %.2f %%, AMP %.2f %%)] Intra [%.2f %%(DC %.2f %% Planar %.2f %% Ang %.2f %% )] Split[%.2f] %% Skipped[%.2f] %%\n", (double)(cntInter[1] * 100) / cntTotalCu[1], (double)(cuInterDistribution[1][0] * 100) / cntInter[1],  (double)(cuInterDistribution[1][2] * 100) / cntInter[1], (double)(cuInterDistribution[1][1] * 100) / cntInter[1], (double)(cuInterDistribution[1][3] * 100) / cntInter[1], (double)(cntIntra[1] * 100) / cntTotalCu[1], (double)(cuIntraDistribution[1][0] * 100) / cntIntra[1], (double)(cuIntraDistribution[1][1] * 100) / cntIntra[1], (double)(cuIntraDistribution[1][2] * 100) / cntIntra[1], (double)(cntSplit[1] * 100) / cntTotalCu[1], (double)(cntSkipCu[1] * 100) / cntTotalCu[1]);
        fprintf(fp, "16x16 : Inter [%.2f %% (2Nx2N %.2f %%,  Nx2N %.2f %%, 2NxN %.2f %%, AMP %.2f %%)] Intra [%.2f %%(DC %.2f %% Planar %.2f %% Ang %.2f %% )] Split[%.2f] %% Skipped[%.2f]%% \n", (double)(cntInter[2] * 100) / cntTotalCu[2], (double)(cuInterDistribution[2][0] * 100) / cntInter[2],  (double)(cuInterDistribution[2][2] * 100) / cntInter[2], (double)(cuInterDistribution[2][1] * 100) / cntInter[2], (double)(cuInterDistribution[2][3] * 100) / cntInter[2], (double)(cntIntra[2] * 100) / cntTotalCu[2], (double)(cuIntraDistribution[2][0] * 100) / cntIntra[2], (double)(cuIntraDistribution[2][1] * 100) / cntIntra[2], (double)(cuIntraDistribution[2][2] * 100) / cntIntra[2], (double)(cntSplit[2] * 100) / cntTotalCu[2], (double)(cntSkipCu[2] * 100) / cntTotalCu[2]);
        fprintf(fp, "8x8 : Inter [%.2f %% (2Nx2N %.2f %%,  Nx2N %.2f %%, 2NxN %.2f %%, AMP %.2f %%)] Intra [%.2f %%(DC %.2f  %% Planar %.2f %% Ang %.2f %%) NxN[%.2f] %% ] Skipped[%.2f] %% \n \n", (double)(cntInter[3] * 100) / cntTotalCu[3], (double)(cuInterDistribution[3][0] * 100) / cntInter[3],  (double)(cuInterDistribution[3][2] * 100) / cntInter[3], (double)(cuInterDistribution[3][1] * 100) / cntInter[3], (double)(cuInterDistribution[3][3] * 100) / cntInter[3], (double)((cntIntra[3]) * 100) / cntTotalCu[3], (double)(cuIntraDistribution[3][0] * 100) / cntIntra[3], (double)(cuIntraDistribution[3][1] * 100) / cntIntra[3], (double)(cuIntraDistribution[3][2] * 100) / cntIntra[3], (double)(cntIntraNxN * 100) / cntIntra[3], (double)(cntSkipCu[3] * 100) / cntTotalCu[3]);
    }

    else
    {
        fprintf(fp, " FRAME - I FRAME \n");
        fprintf(fp, "64x64 : Intra [%.2f %%] Skipped [%.2f %%]\n", (double)(cntIntra[0] * 100) / cntTotalCu[0], (double)(cntSkipCu[0] * 100) / cntTotalCu[0]);
        fprintf(fp, "32x32 : Intra [%.2f %%] Skipped [%.2f %%] \n", (double)(cntIntra[1] * 100) / cntTotalCu[1], (double)(cntSkipCu[1] * 100) / cntTotalCu[1]);
        fprintf(fp, "16x16 : Intra [%.2f %%] Skipped [%.2f %%]\n", (double)(cntIntra[2] * 100) / cntTotalCu[2], (double)(cntSkipCu[2] * 100) / cntTotalCu[2]);
        fprintf(fp, "8x8   : Intra [%.2f %%] Skipped [%.2f %%]\n", (double)(cntIntra[3] * 100) / cntTotalCu[3], (double)(cntSkipCu[3] * 100) / cntTotalCu[3]);
    }

#endif // if LOGGING
}

/**
 \param  rpcPic        picture class
 \retval rpcBitstream  bitstream class
 */
Void TEncSlice::encodeSlice(TComPic*& rpcPic, TComOutputBitstream* pcSubstreams, EncodeFrame* pcEncodeFrame)
{
    PPAScopeEvent(TEncSlice_encodeSlice);
    UInt       uiCUAddr;
    UInt       uiStartCUAddr;
    UInt       uiBoundingCUAddr;
    TComSlice* pcSlice = rpcPic->getSlice(getSliceIdx());

    // choose entropy coder
    TEncEntropy *pcEntropyCoder = pcEncodeFrame->getEntropyEncoder(0);
    TEncSbac *pcSbacCoder = pcEncodeFrame->getSingletonSbac();

    pcEncodeFrame->resetEncoder();
    pcEncodeFrame->getCuEncoder(0)->setBitCounter(NULL);
    pcEntropyCoder->setEntropyCoder(pcSbacCoder, pcSlice);

    uiStartCUAddr = 0;
    uiBoundingCUAddr = pcSlice->getSliceCurEndCUAddr();

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

    const Bool bWaveFrontsynchro = m_pcCfg->getWaveFrontsynchro();
    const UInt uiHeightInLCUs = rpcPic->getPicSym()->getFrameHeightInCU();
    const Int  iNumSubstreams = (bWaveFrontsynchro ? uiHeightInLCUs : 1);
    UInt uiBitsOriginallyInSubstreams = 0;

    for (Int iSubstrmIdx = 0; iSubstrmIdx < iNumSubstreams; iSubstrmIdx++)
    {
        pcEncodeFrame->getBufferSBac(iSubstrmIdx)->loadContexts(pcSbacCoder); //init. state
        uiBitsOriginallyInSubstreams += pcSubstreams[iSubstrmIdx].getNumberOfWrittenBits();
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

        pcEntropyCoder->setBitstream(&pcSubstreams[uiSubStrm]);

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
                pcEncodeFrame->getSbacCoder(uiSubStrm)->loadContexts(pcEncodeFrame->getBufferSBac(uiLin - 1));
            }
        }
        pcSbacCoder->load(pcEncodeFrame->getSbacCoder(uiSubStrm)); //this load is used to simplify the code (avoid to change all the call to m_pcSbacCoder)

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
                    pcEntropyCoder->m_pcEntropyCoderIf->codeSaoMerge(mergeLeft);
                }
                else
                {
                    mergeLeft = 0;
                }
                if (mergeLeft == 0)
                {
                    if (allowMergeUp)
                    {
                        pcEntropyCoder->m_pcEntropyCoderIf->codeSaoMerge(mergeUp);
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
                                pcEntropyCoder->encodeSaoOffset(&saoParam->saoLcuParam[compIdx][addr], compIdx);
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
        pcEncodeFrame->getCuEncoder(0)->set_pcEntropyCoder(pcEntropyCoder);
        pcEncodeFrame->getCuEncoder(0)->encodeCU(pcCU);

#if ENC_DEC_TRACE
        g_bJustDoIt = g_bEncDecTraceDisable;
#endif
        pcEncodeFrame->getSbacCoder(uiSubStrm)->load(pcSbacCoder); //load back status of the entropy coder after encoding the LCU into relevant bitstream entropy coder

        // Store probabilities of second LCU in line into buffer
        if ((iNumSubstreams > 1) && (uiCol == 1) && bWaveFrontsynchro)
        {
            pcEncodeFrame->getBufferSBac(uiLin)->loadContexts(pcEncodeFrame->getSbacCoder(uiSubStrm));
        }
    }

    if (m_pcCfg->getUseAdaptQpSelect())
    {
        assert(0);
#if 0
        m_pcTrQuant->storeSliceQpNext(pcSlice);
#endif
    }
    if (pcSlice->getPPS()->getCabacInitPresentFlag())
    {
        pcEntropyCoder->determineCabacInitIdx();
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
