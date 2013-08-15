/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Chung Shin Yee <shinyee@multicorewareinc.com>
 *          Min Chen <chenm003@163.com>
 *          Steve Borho <steve@borho.org>
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

#include "PPA/ppa.h"
#include "wavefront.h"

#include "TLibEncoder/TEncTop.h"
#include "frameencoder.h"
#include "cturow.h"

using namespace x265;

#if CU_STAT_LOGFILE
int cntInter[4], cntIntra[4], cntSplit[4],  totalCU;
int cuInterDistribution[4][4], cuIntraDistribution[4][3], cntIntraNxN;
int cntSkipCu[4], cntTotalCu[4];
extern FILE * fp, * fp1;
#endif

FrameEncoder::FrameEncoder(ThreadPool* pool)
    : WaveFront(pool)
    , m_frameFilter(pool)
    , m_cfg(NULL)
    , m_pic(NULL)
    , m_rows(NULL)
{}

void FrameEncoder::destroy()
{
    JobProvider::flush();  // ensure no worker threads are using this frame

    if (m_rows)
    {
        for (int i = 0; i < m_numRows; ++i)
        {
            m_rows[i].destroy();
        }

        delete[] m_rows;
    }

    if (m_cfg->param.bEnableSAO)
    {
        m_sao.destroy();
        m_sao.destroyEncBuffer();
    }
    m_frameFilter.destroy();
}

void FrameEncoder::init(TEncTop *top, int numRows)
{
    m_cfg = top;
    m_numRows = numRows;
    m_sliceEncoder.m_cfg = m_cfg;

    if (top->param.bEnableSAO)
    {
        m_sao.setSaoLcuBoundary(top->param.saoLcuBoundary);
        m_sao.setSaoLcuBasedOptimization(top->param.saoLcuBasedOptimization);
        m_sao.setMaxNumOffsetsPerPic(top->getMaxNumOffsetsPerPic());
        m_sao.create(top->param.sourceWidth, top->param.sourceHeight, g_maxCUWidth, g_maxCUHeight);
        m_sao.createEncBuffer();
    }
    m_frameFilter.init(top, numRows);

    m_rows = new CTURow[m_numRows];
    for (int i = 0; i < m_numRows; ++i)
    {
        m_rows[i].create(top);
    }

    if (!WaveFront::init(m_numRows))
    {
        assert(!"Unable to initialize job queue.");
        m_pool = NULL;
    }
}

void FrameEncoder::initSlice(TComPic* pic, Bool bForceISlice, Int gopID, TComSPS* sps, TComPPS *pps)
{
    m_pic = pic;
    TComSlice* slice = pic->getSlice();
    slice->setSPS(sps);
    slice->setPPS(pps);
    slice->setSliceBits(0);
    slice->setPic(pic);
    slice->initSlice();
    slice->setPicOutputFlag(true);

    // slice type
    SliceType sliceType = bForceISlice ? I_SLICE : B_SLICE;
    slice->setSliceType(sliceType);
    slice->setReferenced(true);

    /* TODO: lookahead and DPB modeling should give us these values */
    slice->setNumRefIdx(REF_PIC_LIST_0, m_cfg->getGOPEntry(gopID).m_numRefPicsActive);
    slice->setNumRefIdx(REF_PIC_LIST_1, m_cfg->getGOPEntry(gopID).m_numRefPicsActive);

    if (slice->getPPS()->getDeblockingFilterControlPresentFlag())
    {
        slice->getPPS()->setDeblockingFilterOverrideEnabledFlag(!m_cfg->getLoopFilterOffsetInPPS());
        slice->setDeblockingFilterOverrideFlag(!m_cfg->getLoopFilterOffsetInPPS());
        slice->getPPS()->setPicDisableDeblockingFilterFlag(!m_cfg->param.bEnableLoopFilter);
        slice->setDeblockingFilterDisable(!m_cfg->param.bEnableLoopFilter);
        if (!slice->getDeblockingFilterDisable())
        {
            slice->getPPS()->setDeblockingFilterBetaOffsetDiv2(m_cfg->getLoopFilterBetaOffset());
            slice->getPPS()->setDeblockingFilterTcOffsetDiv2(m_cfg->getLoopFilterTcOffset());
            slice->setDeblockingFilterBetaOffsetDiv2(m_cfg->getLoopFilterBetaOffset());
            slice->setDeblockingFilterTcOffsetDiv2(m_cfg->getLoopFilterTcOffset());
        }
    }
    else
    {
        slice->setDeblockingFilterOverrideFlag(false);
        slice->setDeblockingFilterDisable(false);
        slice->setDeblockingFilterBetaOffsetDiv2(0);
        slice->setDeblockingFilterTcOffsetDiv2(0);
    }
    m_wp.xStoreWPparam(pps->getUseWP(), pps->getWPBiPred());

    // depth computation based on GOP size
    Int depth = 0;
    Int poc = slice->getPOC() % m_cfg->getGOPSize();
    if (poc)
    {
        Int step = m_cfg->getGOPSize();
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

    slice->setDepth(depth);
    slice->setMaxNumMergeCand(m_cfg->param.maxNumMergeCand);

    // ------------------------------------------------------------------------------------------------------------------
    // QP setting
    // ------------------------------------------------------------------------------------------------------------------

    Double qpdouble;
    Double lambda;
    qpdouble = m_cfg->param.qp;
    if (sliceType != I_SLICE)
    {
        if (!((qpdouble == -slice->getSPS()->getQpBDOffsetY()) && (slice->getSPS()->getUseLossless())))
        {
            qpdouble += m_cfg->getGOPEntry(gopID).m_QPOffset;
        }
    }

    // TODO: Remove dQP?
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
    // in RdCost there is only one lambda because the luma and chroma bits are not separated,
    // instead we weight the distortion of chroma.
    Double weight = 1.0;
    Int qpc;
    Int chromaQPOffset;

    chromaQPOffset = slice->getPPS()->getChromaCbQpOffset() + slice->getSliceQpDeltaCb();
    qpc = Clip3(0, 57, qp + chromaQPOffset);
    weight = pow(2.0, (qp - g_chromaScale[qpc]) / 3.0); // takes into account of the chroma qp mapping and chroma qp Offset
    setCbDistortionWeight(weight);

    chromaQPOffset = slice->getPPS()->getChromaCrQpOffset() + slice->getSliceQpDeltaCr();
    qpc = Clip3(0, 57, qp + chromaQPOffset);
    weight = pow(2.0, (qp - g_chromaScale[qpc]) / 3.0); // takes into account of the chroma qp mapping and chroma qp Offset
    setCrDistortionWeight(weight);

    // for RDOQ
    setQPLambda(qp, lambda, lambda / weight);

    // For SAO
    slice->setLambda(lambda, lambda / weight);

    slice->setSliceQp(qp);
    slice->setSliceQpBase(qp);
    slice->setSliceQpDelta(0);
    slice->setSliceQpDeltaCb(0);
    slice->setSliceQpDeltaCr(0);
}

Void FrameEncoder::compressSlice(TComPic* pic)
{
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

    m_sliceEncoder.xDetermineStartAndBoundingCUAddr(pic, false);

    //------------------------------------------------------------------------------
    //  Weighted Prediction parameters estimation.
    //------------------------------------------------------------------------------
    // calculate AC/DC values for current picture
    if (slice->getPPS()->getUseWP() || slice->getPPS()->getWPBiPred())
    {
        m_wp.xCalcACDCParamSlice(slice);
    }

    bool wpexplicit = (slice->getSliceType() == P_SLICE && slice->getPPS()->getUseWP()) ||
                      (slice->getSliceType() == B_SLICE && slice->getPPS()->getWPBiPred());

    if (wpexplicit)
    {
        //------------------------------------------------------------------------------
        //  Weighted Prediction implemented at Slice level. SliceMode=2 is not supported yet.
        //------------------------------------------------------------------------------
        m_wp.xEstimateWPParamSlice(slice);
        slice->initWpScaling();

        // check WP on/off
        m_wp.xCheckWPEnable(slice);
    }

    // Generate motion references
    Int numPredDir = slice->isInterP() ? 1 : 2;
    for (Int l = 0; l < numPredDir; l++)
    {
        RefPicList list = (l ? REF_PIC_LIST_1 : REF_PIC_LIST_0);
        wpScalingParam *w = NULL;
        for (Int ref = 0; ref < slice->getNumRefIdx(list); ref++)
        {
            TComPicYuv *recon = slice->getRefPic(list, ref)->getPicYuvRec();
            if ((slice->isInterP() && slice->getPPS()->getUseWP()))
                w = slice->m_weightPredTable[list][ref];
            slice->m_mref[list][ref] = recon->generateMotionReference(m_pool, w);
        }
    }

    // reset entropy coders
    m_sbacCoder.init(&m_binCoderCABAC);
    for (int i = 0; i < this->m_numRows; i++)
    {
        m_rows[i].init();
        m_rows[i].m_entropyCoder.setEntropyCoder(&m_sbacCoder, pic->getSlice());
        m_rows[i].m_entropyCoder.resetEntropy();
        m_rows[i].m_rdSbacCoders[0][CI_CURR_BEST]->load(&m_sbacCoder);
        m_pic->m_complete_enc[i] = 0;
    }

    m_frameFilter.start(pic);

    if (!m_pool || !m_cfg->param.bEnableWavefront)
    {
        for (int i = 0; i < this->m_numRows; i++)
        {
            processRow(i);
        }

        if (m_cfg->param.bEnableLoopFilter)
        {
            for (int i = 0; i < this->m_numRows; i++)
            {
                m_frameFilter.processRow(i);
            }
        }
    }
    else
    {
        WaveFront::enqueue();

        // Enqueue first row, then block until worker threads complete the frame
        WaveFront::enqueueRow(0);

        m_completionEvent.wait();

        WaveFront::dequeue();
    }

    if (m_cfg->param.bEnableWavefront)
    {
        slice->setNextSlice(true);
    }

    m_wp.xRestoreWPparam(slice);

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

void FrameEncoder::processRow(int row)
{
    PPAScopeEvent(Thread_ProcessRow);

    // Called by worker threads
    CTURow& curRow  = m_rows[row];
    CTURow& codeRow = m_rows[m_cfg->param.bEnableWavefront ? row : 0];

    const uint32_t numCols = m_pic->getPicSym()->getFrameWidthInCU();
    const uint32_t lineStartCUAddr = row * numCols;
    for (UInt col = m_pic->m_complete_enc[row]; col < numCols; col++)
    {
        const uint32_t cuAddr = lineStartCUAddr + col;
        TComDataCU* cu = m_pic->getCU(cuAddr);
        cu->initCU(m_pic, cuAddr);

        codeRow.m_entropyCoder.setEntropyCoder(&m_sbacCoder, m_pic->getSlice());
        codeRow.m_entropyCoder.resetEntropy();

        TEncSbac *bufSbac = (m_cfg->param.bEnableWavefront && col == 0 && row > 0) ? &m_rows[row - 1].m_bufferSbacCoder : NULL;
        codeRow.processCU(cu, m_pic->getSlice(), bufSbac, m_cfg->param.bEnableWavefront && col == 1);

        // TODO: Keep atomic running totals for rate control?
        // cu->m_totalBits;
        // cu->m_totalCost;
        // cu->m_totalDistortion;

        // Completed CU processing
        m_pic->m_complete_enc[row]++;

        // Active Loopfilter
        if (row > 0)
        {
            // NOTE: my version, it need check active flag
            m_frameFilter.enqueueRow(row - 1);
        }

        if (m_pic->m_complete_enc[row] >= 2 && row < m_numRows - 1)
        {
            ScopedLock below(m_rows[row + 1].m_lock);
            if (m_rows[row + 1].m_active == false &&
                m_pic->m_complete_enc[row + 1] + 2 <= m_pic->m_complete_enc[row])
            {
                m_rows[row + 1].m_active = true;
                WaveFront::enqueueRow(row + 1);
            }
        }

        ScopedLock self(curRow.m_lock);
        if (row > 0 && m_pic->m_complete_enc[row] < numCols - 1 && m_pic->m_complete_enc[row - 1] < m_pic->m_complete_enc[row] + 2)
        {
            curRow.m_active = false;
            return;
        }
        if (m_cfg->param.bEnableWavefront && checkHigherPriorityRow(row))
        {
            curRow.m_active = false;
            return;
        }
    }

    // this row of CTUs has been encoded
    if (row == m_numRows - 1)
    {
        m_completionEvent.trigger();
    }
}
