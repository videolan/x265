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
#include "frameencoder.h"

#include <math.h>

using namespace x265;

//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TEncSlice::TEncSlice()
{}

TEncSlice::~TEncSlice()
{}

Void TEncSlice::create(Int, Int, UInt, UInt, UChar)
{}

Void TEncSlice::destroy()
{}

Void TEncSlice::init(TEncTop* top)
{
    m_cfg = top;
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
TComSlice* TEncSlice::initEncSlice(TComPic* pic, x265::FrameEncoder *frameEncoder, Bool bForceISlice, Int pocLast, Int pocCurr,
                                   Int gopID, TComSPS* sps, TComPPS *pps)
{
    Double qpdouble;
    Double lambda;
    TComSlice* slice = pic->getSlice();

    slice->setSPS(sps);
    slice->setPPS(pps);
    slice->setSliceBits(0);
    slice->setPic(pic);
    slice->initSlice();
    slice->setPicOutputFlag(true);
    slice->setPOC(pocCurr);

    // depth computation based on GOP size
    Int depth;
    {
        Int poc = slice->getPOC() % m_cfg->getGOPSize();
        if (poc == 0)
        {
            depth = 0;
        }
        else
        {
            Int step = m_cfg->getGOPSize();
            depth    = 0;
            for (Int i = step >> 1; i >= 1; i >>= 1)
            {
                for (Int j = i; j < m_cfg->getGOPSize(); j += step)
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
    SliceType sliceType = (pocLast == 0 || pocCurr % m_cfg->param.keyframeInterval == 0 || bForceISlice) ? I_SLICE : B_SLICE;
    slice->setSliceType(sliceType);

    // ------------------------------------------------------------------------------------------------------------------
    // Non-referenced frame marking
    // ------------------------------------------------------------------------------------------------------------------

    if (pocLast == 0)
    {
        slice->setTemporalLayerNonReferenceFlag(false);
    }
    else
    {
        slice->setTemporalLayerNonReferenceFlag(!m_cfg->getGOPEntry(gopID).m_refPic);
    }
    slice->setReferenced(true);

    // ------------------------------------------------------------------------------------------------------------------
    // QP setting
    // ------------------------------------------------------------------------------------------------------------------

    qpdouble = m_cfg->param.qp;
    if (sliceType != I_SLICE)
    {
        if (!((qpdouble == -slice->getSPS()->getQpBDOffsetY()) && (slice->getSPS()->getUseLossless())))
        {
            qpdouble += m_cfg->getGOPEntry(gopID).m_QPOffset;
        }
    }

    // modify QP
    Int* pdQPs = m_cfg->getdQPs();
    if (pdQPs)
    {
        qpdouble += pdQPs[slice->getPOC()];
    }

    // ------------------------------------------------------------------------------------------------------------------
    // Lambda computation
    // ------------------------------------------------------------------------------------------------------------------

    Int qp;

    // compute lambda value
    Int    NumberBFrames = (m_cfg->getGOPSize() - 1);
    Int    SHIFT_QP = 12;
    Double lambda_scale = 1.0 - Clip3(0.0, 0.5, 0.05 * (Double)NumberBFrames);
#if FULL_NBIT
    Int    bitdepth_luma_qp_scale = 6 * (X265_DEPTH - 8);
    Double qp_temp_orig = (Double)dQP - SHIFT_QP;
#else
    Int    bitdepth_luma_qp_scale = 0;
#endif
    Double qp_temp = (Double)qpdouble + bitdepth_luma_qp_scale - SHIFT_QP;

    // Case #1: I or P-slices (key-frame)
    Double qpFactor = m_cfg->getGOPEntry(gopID).m_QPFactor;
    if (sliceType == I_SLICE)
    {
        qpFactor = 0.57 * lambda_scale;
    }
    lambda = qpFactor * pow(2.0, qp_temp / 3.0);

    if (depth > 0)
    {
#if FULL_NBIT
        lambda *= Clip3(2.00, 4.00, (qp_temp_orig / 6.0));
#else
        lambda *= Clip3(2.00, 4.00, (qp_temp / 6.0));
#endif
    }

    qp = max(-sps->getQpBDOffsetY(), min(MAX_QP, (Int)floor(qpdouble + 0.5)));

    if (slice->getSliceType() != I_SLICE)
    {
        lambda *= m_cfg->getLambdaModifier(0); // temporal layer 0
    }

    // for RDO
    // in RdCost there is only one lambda because the luma and chroma bits are not separated, instead we weight the distortion of chroma.
    Double weight = 1.0;
    Int qpc;
    Int chromaQPOffset;

    chromaQPOffset = slice->getPPS()->getChromaCbQpOffset() + slice->getSliceQpDeltaCb();
    qpc = Clip3(0, 57, qp + chromaQPOffset);
    weight = pow(2.0, (qp - g_chromaScale[qpc]) / 3.0); // takes into account of the chroma qp mapping and chroma qp Offset
    frameEncoder->setCbDistortionWeight(weight);

    chromaQPOffset = slice->getPPS()->getChromaCrQpOffset() + slice->getSliceQpDeltaCr();
    qpc = Clip3(0, 57, qp + chromaQPOffset);
    weight = pow(2.0, (qp - g_chromaScale[qpc]) / 3.0); // takes into account of the chroma qp mapping and chroma qp Offset
    frameEncoder->setCrDistortionWeight(weight);

    // for RDOQ
    frameEncoder->setQPLambda(qp, lambda, lambda / weight);

    // For SAO
    slice->setLambda(lambda, lambda / weight);

#if HB_LAMBDA_FOR_LDC
    // restore original slice type
    sliceType = (pocLast == 0 || pocCurr % m_cfg->param.keyframeInterval == 0 || bForceISlice) ? I_SLICE : sliceType;

    slice->setSliceType(sliceType);
#endif

    if (m_cfg->getUseRecalculateQPAccordingToLambda())
    {
        qpdouble = xGetQPValueAccordingToLambda(lambda);
        qp = max(-sps->getQpBDOffsetY(), min(MAX_QP, (Int)floor(qpdouble + 0.5)));
    }

    slice->setSliceQp(qp);
    slice->setSliceQpBase(qp);
    slice->setSliceQpDelta(0);
    slice->setSliceQpDeltaCb(0);
    slice->setSliceQpDeltaCr(0);
    slice->setNumRefIdx(REF_PIC_LIST_0, m_cfg->getGOPEntry(gopID).m_numRefPicsActive);
    slice->setNumRefIdx(REF_PIC_LIST_1, m_cfg->getGOPEntry(gopID).m_numRefPicsActive);

    if (slice->getPPS()->getDeblockingFilterControlPresentFlag())
    {
        slice->getPPS()->setDeblockingFilterOverrideEnabledFlag(!m_cfg->getLoopFilterOffsetInPPS());
        slice->setDeblockingFilterOverrideFlag(!m_cfg->getLoopFilterOffsetInPPS());
        slice->getPPS()->setPicDisableDeblockingFilterFlag(m_cfg->getLoopFilterDisable());
        slice->setDeblockingFilterDisable(m_cfg->getLoopFilterDisable());
        if (!slice->getDeblockingFilterDisable())
        {
            if (!m_cfg->getLoopFilterOffsetInPPS() && sliceType != I_SLICE)
            {
                slice->getPPS()->setDeblockingFilterBetaOffsetDiv2(m_cfg->getGOPEntry(gopID).m_betaOffsetDiv2 + m_cfg->getLoopFilterBetaOffset());
                slice->getPPS()->setDeblockingFilterTcOffsetDiv2(m_cfg->getGOPEntry(gopID).m_tcOffsetDiv2 + m_cfg->getLoopFilterTcOffset());
                slice->setDeblockingFilterBetaOffsetDiv2(m_cfg->getGOPEntry(gopID).m_betaOffsetDiv2 + m_cfg->getLoopFilterBetaOffset());
                slice->setDeblockingFilterTcOffsetDiv2(m_cfg->getGOPEntry(gopID).m_tcOffsetDiv2 + m_cfg->getLoopFilterTcOffset());
            }
            else
            {
                slice->getPPS()->setDeblockingFilterBetaOffsetDiv2(m_cfg->getLoopFilterBetaOffset());
                slice->getPPS()->setDeblockingFilterTcOffsetDiv2(m_cfg->getLoopFilterTcOffset());
                slice->setDeblockingFilterBetaOffsetDiv2(m_cfg->getLoopFilterBetaOffset());
                slice->setDeblockingFilterTcOffsetDiv2(m_cfg->getLoopFilterTcOffset());
            }
        }
    }
    else
    {
        slice->setDeblockingFilterOverrideFlag(false);
        slice->setDeblockingFilterDisable(false);
        slice->setDeblockingFilterBetaOffsetDiv2(0);
        slice->setDeblockingFilterTcOffsetDiv2(0);
    }

    slice->setDepth(depth);
    slice->setMaxNumMergeCand(m_cfg->param.maxNumMergeCand);
    xStoreWPparam(pps->getUseWP(), pps->getWPBiPred());
    return slice;
}

Void TEncSlice::resetQP(TComPic* pic, FrameEncoder *frameEncoder, Int sliceQP, Double lambda)
{
    TComSlice* slice = pic->getSlice();

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
    weight = pow(2.0, (sliceQP - g_chromaScale[qpc]) / 3.0); // takes into account of the chroma qp mapping and chroma qp Offset
    frameEncoder->setCbDistortionWeight(weight);

    chromaQPOffset = slice->getPPS()->getChromaCrQpOffset() + slice->getSliceQpDeltaCr();
    qpc = Clip3(0, 57, sliceQP + chromaQPOffset);
    weight = pow(2.0, (sliceQP - g_chromaScale[qpc]) / 3.0); // takes into account of the chroma qp mapping and chroma qp Offset
    frameEncoder->setCrDistortionWeight(weight);

    // for RDOQ
    frameEncoder->setQPLambda(sliceQP, lambda, lambda / weight);

    // For SAO
    slice->setLambda(lambda, lambda / weight);
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

Void TEncSlice::setSearchRange(TComSlice* slice, FrameEncoder *frameEncoder)
{
    Int currPOC = slice->getPOC();
    Int gopSize = m_cfg->getGOPSize();
    Int offset = (gopSize >> 1);
    Int maxSR = m_cfg->param.searchRange;
    Int numPredDir = slice->isInterP() ? 1 : 2;

    for (Int dir = 0; dir <= numPredDir; dir++)
    {
        RefPicList  e = (dir ? REF_PIC_LIST_1 : REF_PIC_LIST_0);
        for (Int refIdx = 0; refIdx < slice->getNumRefIdx(e); refIdx++)
        {
            Int refPOC = slice->getRefPic(e, refIdx)->getPOC();
            Int newSR = Clip3(8, maxSR, (maxSR * ADAPT_SR_SCALE * abs(currPOC - refPOC) + offset) / gopSize);

            frameEncoder->setAdaptiveSearchRange(dir, refIdx, newSR);
        }
    }
}

#if CU_STAT_LOGFILE
int cntInter[4], cntIntra[4], cntSplit[4],  totalCU;
int cuInterDistribution[4][4], cuIntraDistribution[4][3], cntIntraNxN;
int cntSkipCu[4], cntTotalCu[4];
extern FILE * fp, * fp1;
#endif
Void TEncSlice::compressSlice(TComPic* pic, FrameEncoder* frameEncoder)
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
#endif // if CU_STAT_LOGFILE

    TComSlice* slice = pic->getSlice();
    slice->setSliceSegmentBits(0);
    xDetermineStartAndBoundingCUAddr(pic, false);

    //------------------------------------------------------------------------------
    //  Weighted Prediction parameters estimation.
    //------------------------------------------------------------------------------
    // calculate AC/DC values for current picture
    if (slice->getPPS()->getUseWP() || slice->getPPS()->getWPBiPred())
    {
        xCalcACDCParamSlice(slice);
    }

    Bool wpexplicit = (slice->getSliceType() == P_SLICE && slice->getPPS()->getUseWP()) ||
        (slice->getSliceType() == B_SLICE && slice->getPPS()->getWPBiPred());

    if (wpexplicit)
    {
        //------------------------------------------------------------------------------
        //  Weighted Prediction implemented at Slice level. SliceMode=2 is not supported yet.
        //------------------------------------------------------------------------------
        xEstimateWPParamSlice(slice);
        slice->initWpScaling();

        // check WP on/off
        xCheckWPEnable(slice);
    }

    Int numPredDir = slice->isInterP() ? 1 : 2;

    if ((slice->getSliceType() == P_SLICE && slice->getPPS()->getUseWP()))
    {
        for (Int refList = 0; refList < numPredDir; refList++)
        {
            RefPicList  picList = (refList ? REF_PIC_LIST_1 : REF_PIC_LIST_0);
            for (Int refIdxTemp = 0; refIdxTemp < slice->getNumRefIdx(picList); refIdxTemp++)
            {
                //Generate weighted motionreference
                wpScalingParam *w = &(slice->m_weightPredTable[picList][refIdxTemp][0]);
                slice->m_mref[picList][refIdxTemp] = slice->getRefPic(picList, refIdxTemp)->getPicYuvRec()->generateMotionReference(x265::ThreadPool::getThreadPool(), w);
            }
        }
    }
    else
    {
        for (Int refList = 0; refList < numPredDir; refList++)
        {
            RefPicList  picList = (refList ? REF_PIC_LIST_1 : REF_PIC_LIST_0);
            for (Int refIdxTemp = 0; refIdxTemp < slice->getNumRefIdx(picList); refIdxTemp++)
            {
                slice->m_mref[picList][refIdxTemp] = slice->getRefPic(picList, refIdxTemp)->getPicYuvRec()->generateMotionReference(x265::ThreadPool::getThreadPool(), NULL);
            }
        }
    }

    frameEncoder->encode(pic, slice);

    if (m_cfg->param.bEnableWavefront)
    {
        slice->setNextSlice(true);
    }

    xRestoreWPparam(slice);

#if CU_STAT_LOGFILE
    if (slice->getSliceType() == P_SLICE)
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
 \param  outPic        picture class
 \retval rpcBitstream  bitstream class
 */
Void TEncSlice::encodeSlice(TComPic* pic, TComOutputBitstream* substreams, FrameEncoder* frameEncoder)
{
    PPAScopeEvent(TEncSlice_encodeSlice);
    UInt       cuAddr;
    UInt       startCUAddr;
    UInt       boundingCUAddr;
    TComSlice* slice = pic->getSlice();

    // choose entropy coder
    TEncEntropy *entropyCoder = frameEncoder->getEntropyCoder(0);
    TEncSbac *sbacCoder = frameEncoder->getSingletonSbac();

    frameEncoder->resetEncoder();
    frameEncoder->getCuEncoder(0)->setBitCounter(NULL);
    entropyCoder->setEntropyCoder(sbacCoder, slice);

    startCUAddr = 0;
    boundingCUAddr = slice->getSliceCurEndCUAddr();

    // Appropriate substream bitstream is switched later.
    // for every CU
#if ENC_DEC_TRACE
    g_bJustDoIt = g_bEncDecTraceEnable;
#endif
    DTRACE_CABAC_VL(g_nSymbolCounter++);
    DTRACE_CABAC_T("\tPOC: ");
    DTRACE_CABAC_V(pic->getPOC());
    DTRACE_CABAC_T("\n");
#if ENC_DEC_TRACE
    g_bJustDoIt = g_bEncDecTraceDisable;
#endif

    const Bool bWaveFrontsynchro = m_cfg->param.bEnableWavefront;
    const UInt heightInLCUs = pic->getPicSym()->getFrameHeightInCU();
    const Int  numSubstreams = (bWaveFrontsynchro ? heightInLCUs : 1);
    UInt bitsOriginallyInSubstreams = 0;

    for (Int substrmIdx = 0; substrmIdx < numSubstreams; substrmIdx++)
    {
        frameEncoder->getBufferSBac(substrmIdx)->loadContexts(sbacCoder); //init. state
        bitsOriginallyInSubstreams += substreams[substrmIdx].getNumberOfWrittenBits();
    }

    UInt widthInLCUs  = pic->getPicSym()->getFrameWidthInCU();
    UInt col = 0, lin = 0, subStrm = 0;
    cuAddr = (startCUAddr / pic->getNumPartInCU()); /* for tiles, startCUAddr is NOT the real raster scan address, it is actually
                                                       an encoding order index, so we need to convert the index (startCUAddr)
                                                       into the real raster scan address (cuAddr) via the CUOrderMap */
    UInt encCUOrder;
    for (encCUOrder = startCUAddr / pic->getNumPartInCU();
         encCUOrder < (boundingCUAddr + pic->getNumPartInCU() - 1) / pic->getNumPartInCU();
         cuAddr = (++encCUOrder))
    {
        col     = cuAddr % widthInLCUs;
        lin     = cuAddr / widthInLCUs;
        subStrm = lin % numSubstreams;

        entropyCoder->setBitstream(&substreams[subStrm]);

        // Synchronize cabac probabilities with upper-right LCU if it's available and we're at the start of a line.
        if ((numSubstreams > 1) && (col == 0) && bWaveFrontsynchro)
        {
            // We'll sync if the TR is available.
            TComDataCU *cuUp = pic->getCU(cuAddr)->getCUAbove();
            UInt widthInCU = pic->getFrameWidthInCU();
            TComDataCU *cuTr = NULL;

            // CHECK_ME: here can be optimize a little, do it later
            if (cuUp && ((cuAddr % widthInCU + 1) < widthInCU))
            {
                cuTr = pic->getCU(cuAddr - widthInCU + 1);
            }
            if (true /*bEnforceSliceRestriction*/ && ((cuTr == NULL) || (cuTr->getSlice() == NULL)))
            {
                // TR not available.
            }
            else
            {
                // TR is available, we use it.
                frameEncoder->getSbacCoder(subStrm)->loadContexts(frameEncoder->getBufferSBac(lin - 1));
            }
        }
        sbacCoder->load(frameEncoder->getSbacCoder(subStrm)); //this load is used to simplify the code (avoid to change all the call to m_pcSbacCoder)

        TComDataCU* cu = pic->getCU(cuAddr);
        if (slice->getSPS()->getUseSAO() && (slice->getSaoEnabledFlag() || slice->getSaoEnabledFlagChroma()))
        {
            SAOParam *saoParam = slice->getPic()->getPicSym()->getSaoParam();
            Int numCuInWidth     = saoParam->numCuInWidth;
            Int cuAddrInSlice    = cuAddr;
            Int rx = cuAddr % numCuInWidth;
            Int ry = cuAddr / numCuInWidth;
            Int allowMergeLeft = 1;
            Int allowMergeUp   = 1;
            Int addr = cu->getAddr();
            allowMergeLeft = (rx > 0) && (cuAddrInSlice != 0);
            allowMergeUp = (ry > 0) && (cuAddrInSlice >= 0);
            if (saoParam->bSaoFlag[0] || saoParam->bSaoFlag[1])
            {
                Int mergeLeft = saoParam->saoLcuParam[0][addr].mergeLeftFlag;
                Int mergeUp = saoParam->saoLcuParam[0][addr].mergeUpFlag;
                if (allowMergeLeft)
                {
                    entropyCoder->m_pcEntropyCoderIf->codeSaoMerge(mergeLeft);
                }
                else
                {
                    mergeLeft = 0;
                }
                if (mergeLeft == 0)
                {
                    if (allowMergeUp)
                    {
                        entropyCoder->m_pcEntropyCoderIf->codeSaoMerge(mergeUp);
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
                                entropyCoder->encodeSaoOffset(&saoParam->saoLcuParam[compIdx][addr], compIdx);
                            }
                        }
                    }
                }
            }
        }
        else if (slice->getSPS()->getUseSAO())
        {
            Int addr = cu->getAddr();
            SAOParam *saoParam = slice->getPic()->getPicSym()->getSaoParam();
            for (Int cIdx = 0; cIdx < 3; cIdx++)
            {
                SaoLcuParam *saoLcuParam = &(saoParam->saoLcuParam[cIdx][addr]);
                if (((cIdx == 0) && !slice->getSaoEnabledFlag()) || ((cIdx == 1 || cIdx == 2) && !slice->getSaoEnabledFlagChroma()))
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
        frameEncoder->getCuEncoder(0)->setEntropyCoder(entropyCoder);
        frameEncoder->getCuEncoder(0)->encodeCU(cu);

#if ENC_DEC_TRACE
        g_bJustDoIt = g_bEncDecTraceDisable;
#endif

        // load back status of the entropy coder after encoding the LCU into relevant bitstream entropy coder
        frameEncoder->getSbacCoder(subStrm)->load(sbacCoder);

        // Store probabilities of second LCU in line into buffer
        if ((numSubstreams > 1) && (col == 1) && bWaveFrontsynchro)
        {
            frameEncoder->getBufferSBac(lin)->loadContexts(frameEncoder->getSbacCoder(subStrm));
        }
    }

    if (slice->getPPS()->getCabacInitPresentFlag())
    {
        entropyCoder->determineCabacInitIdx();
    }
}

/** Determines the starting and bounding LCU address of current slice / dependent slice
 * \param bEncodeSlice Identifies if the calling function is compressSlice() [false] or encodeSlice() [true]
 * \returns Updates uiStartCUAddr, uiBoundingCUAddr with appropriate LCU address
 */
Void TEncSlice::xDetermineStartAndBoundingCUAddr(TComPic* pic, Bool bEncodeSlice)
{
    TComSlice* slice = pic->getSlice();
    UInt numberOfCUsInFrame = pic->getNumCUsInFrame();
    UInt boundingCUAddrSlice = numberOfCUsInFrame * pic->getNumPartInCU();

    // WPP: if a slice does not start at the beginning of a CTB row, it must end within the same CTB row
    slice->setSliceCurEndCUAddr(boundingCUAddrSlice);

    if (!bEncodeSlice)
    {
        // For fixed number of LCU within an entropy and reconstruction slice we already know whether we will encounter
        // end of entropy and/or reconstruction slice first. Set the flags accordingly.
        slice->setNextSlice(false);
    }
}

Double TEncSlice::xGetQPValueAccordingToLambda(Double lambda)
{
    return 4.2005 * log(lambda) + 13.7122;
}

//! \}
