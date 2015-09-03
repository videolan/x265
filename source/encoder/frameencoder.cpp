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
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#include "common.h"
#include "frame.h"
#include "framedata.h"
#include "wavefront.h"
#include "param.h"

#include "encoder.h"
#include "frameencoder.h"
#include "common.h"
#include "slicetype.h"
#include "nal.h"

namespace X265_NS {
void weightAnalyse(Slice& slice, Frame& frame, x265_param& param);

FrameEncoder::FrameEncoder()
{
    m_prevOutputTime = x265_mdate();
    m_isFrameEncoder = true;
    m_threadActive = true;
    m_slicetypeWaitTime = 0;
    m_activeWorkerCount = 0;
    m_completionCount = 0;
    m_bAllRowsStop = false;
    m_vbvResetTriggerRow = -1;
    m_outStreams = NULL;
    m_substreamSizes = NULL;
    m_nr = NULL;
    m_tld = NULL;
    m_rows = NULL;
    m_top = NULL;
    m_param = NULL;
    m_frame = NULL;
    m_cuGeoms = NULL;
    m_ctuGeomMap = NULL;
    m_localTldIdx = 0;
    memset(&m_rce, 0, sizeof(RateControlEntry));
}

void FrameEncoder::destroy()
{
    if (m_pool)
    {
        if (!m_jpId)
        {
            int numTLD = m_pool->m_numWorkers;
            if (!m_param->bEnableWavefront)
                numTLD += m_pool->m_numProviders;
            for (int i = 0; i < numTLD; i++)
                m_tld[i].destroy();
            delete [] m_tld;
        }
    }
    else
    {
        m_tld->destroy();
        delete m_tld;
    }

    delete[] m_rows;
    delete[] m_outStreams;
    X265_FREE(m_cuGeoms);
    X265_FREE(m_ctuGeomMap);
    X265_FREE(m_substreamSizes);
    X265_FREE(m_nr);

    m_frameFilter.destroy();

    if (m_param->bEmitHRDSEI || !!m_param->interlaceMode)
    {
        delete m_rce.picTimingSEI;
        delete m_rce.hrdTiming;
    }
}

bool FrameEncoder::init(Encoder *top, int numRows, int numCols)
{
    m_top = top;
    m_param = top->m_param;
    m_numRows = numRows;
    m_numCols = numCols;
    m_filterRowDelay = (m_param->bEnableSAO && m_param->bSaoNonDeblocked) ?
                        2 : (m_param->bEnableSAO || m_param->bEnableLoopFilter ? 1 : 0);
    m_filterRowDelayCus = m_filterRowDelay * numCols;
    m_rows = new CTURow[m_numRows];
    bool ok = !!m_numRows;

    /* determine full motion search range */
    int range  = m_param->searchRange;       /* fpel search */
    range += !!(m_param->searchMethod < 2);  /* diamond/hex range check lag */
    range += NTAPS_LUMA / 2;                 /* subpel filter half-length */
    range += 2 + MotionEstimate::hpelIterationCount(m_param->subpelRefine) / 2; /* subpel refine steps */
    m_refLagRows = 1 + ((range + g_maxCUSize - 1) / g_maxCUSize);

    // NOTE: 2 times of numRows because both Encoder and Filter in same queue
    if (!WaveFront::init(m_numRows * 2))
    {
        x265_log(m_param, X265_LOG_ERROR, "unable to initialize wavefront queue\n");
        m_pool = NULL;
    }

    m_frameFilter.init(top, this, numRows);

    // initialize HRD parameters of SPS
    if (m_param->bEmitHRDSEI || !!m_param->interlaceMode)
    {
        m_rce.picTimingSEI = new SEIPictureTiming;
        m_rce.hrdTiming = new HRDTiming;

        ok &= m_rce.picTimingSEI && m_rce.hrdTiming;
    }

    if (m_param->noiseReductionIntra || m_param->noiseReductionInter || m_param->rc.vbvBufferSize)
        m_nr = X265_MALLOC(NoiseReduction, 1);
    if (m_nr)
        memset(m_nr, 0, sizeof(NoiseReduction));
    else
        m_param->noiseReductionIntra = m_param->noiseReductionInter = 0;

    return ok;
}

/* Generate a complete list of unique geom sets for the current picture dimensions */
bool FrameEncoder::initializeGeoms()
{
    /* Geoms only vary between CTUs in the presence of picture edges */
    int maxCUSize = m_param->maxCUSize;
    int minCUSize = m_param->minCUSize;
    int heightRem = m_param->sourceHeight & (maxCUSize - 1);
    int widthRem = m_param->sourceWidth & (maxCUSize - 1);
    int allocGeoms = 1; // body
    if (heightRem && widthRem)
        allocGeoms = 4; // body, right, bottom, corner
    else if (heightRem || widthRem)
        allocGeoms = 2; // body, right or bottom

    m_ctuGeomMap = X265_MALLOC(uint32_t, m_numRows * m_numCols);
    m_cuGeoms = X265_MALLOC(CUGeom, allocGeoms * CUGeom::MAX_GEOMS);
    if (!m_cuGeoms || !m_ctuGeomMap)
        return false;

    // body
    CUData::calcCTUGeoms(maxCUSize, maxCUSize, maxCUSize, minCUSize, m_cuGeoms);
    memset(m_ctuGeomMap, 0, sizeof(uint32_t) * m_numRows * m_numCols);
    if (allocGeoms == 1)
        return true;

    int countGeoms = 1;
    if (widthRem)
    {
        // right
        CUData::calcCTUGeoms(widthRem, maxCUSize, maxCUSize, minCUSize, m_cuGeoms + countGeoms * CUGeom::MAX_GEOMS);
        for (uint32_t i = 0; i < m_numRows; i++)
        {
            uint32_t ctuAddr = m_numCols * (i + 1) - 1;
            m_ctuGeomMap[ctuAddr] = countGeoms * CUGeom::MAX_GEOMS;
        }
        countGeoms++;
    }
    if (heightRem)
    {
        // bottom
        CUData::calcCTUGeoms(maxCUSize, heightRem, maxCUSize, minCUSize, m_cuGeoms + countGeoms * CUGeom::MAX_GEOMS);
        for (uint32_t i = 0; i < m_numCols; i++)
        {
            uint32_t ctuAddr = m_numCols * (m_numRows - 1) + i;
            m_ctuGeomMap[ctuAddr] = countGeoms * CUGeom::MAX_GEOMS;
        }
        countGeoms++;

        if (widthRem)
        {
            // corner
            CUData::calcCTUGeoms(widthRem, heightRem, maxCUSize, minCUSize, m_cuGeoms + countGeoms * CUGeom::MAX_GEOMS);

            uint32_t ctuAddr = m_numCols * m_numRows - 1;
            m_ctuGeomMap[ctuAddr] = countGeoms * CUGeom::MAX_GEOMS;
            countGeoms++;
        }
        X265_CHECK(countGeoms == allocGeoms, "geometry match check failure\n");
    }

    return true;
}

bool FrameEncoder::startCompressFrame(Frame* curFrame)
{
    m_slicetypeWaitTime = x265_mdate() - m_prevOutputTime;
    m_frame = curFrame;
    m_param = curFrame->m_param;
    m_sliceType = curFrame->m_lowres.sliceType;
    curFrame->m_encData->m_frameEncoderID = m_jpId;
    curFrame->m_encData->m_jobProvider = this;
    curFrame->m_encData->m_slice->m_mref = m_mref;

    if (!m_cuGeoms)
    {
        if (!initializeGeoms())
            return false;
    }

    m_enable.trigger();
    return true;
}

void FrameEncoder::threadMain()
{
    THREAD_NAME("Frame", m_jpId);

    if (m_pool)
    {
        m_pool->setCurrentThreadAffinity();

        /* the first FE on each NUMA node is responsible for allocating thread
         * local data for all worker threads in that pool. If WPP is disabled, then
         * each FE also needs a TLD instance */
        if (!m_jpId)
        {
            int numTLD = m_pool->m_numWorkers;
            if (!m_param->bEnableWavefront)
                numTLD += m_pool->m_numProviders;

            m_tld = new ThreadLocalData[numTLD];
            for (int i = 0; i < numTLD; i++)
            {
                m_tld[i].analysis.initSearch(*m_param, m_top->m_scalingList);
                m_tld[i].analysis.create(m_tld);
            }

            for (int i = 0; i < m_pool->m_numProviders; i++)
            {
                if (m_pool->m_jpTable[i]->m_isFrameEncoder) /* ugh; over-allocation and other issues here */
                {
                    FrameEncoder *peer = dynamic_cast<FrameEncoder*>(m_pool->m_jpTable[i]);
                    peer->m_tld = m_tld;
                }
            }
        }

        if (m_param->bEnableWavefront)
            m_localTldIdx = -1; // cause exception if used
        else
            m_localTldIdx = m_pool->m_numWorkers + m_jpId;
    }
    else
    {
        m_tld = new ThreadLocalData;
        m_tld->analysis.initSearch(*m_param, m_top->m_scalingList);
        m_tld->analysis.create(NULL);
        m_localTldIdx = 0;
    }

    m_done.trigger();     /* signal that thread is initialized */ 
    m_enable.wait();      /* Encoder::encode() triggers this event */

    while (m_threadActive)
    {
        compressFrame();
        m_done.trigger(); /* FrameEncoder::getEncodedPicture() blocks for this event */
        m_enable.wait();
    }
}

void FrameEncoder::WeightAnalysis::processTasks(int /* workerThreadId */)
{
    Frame* frame = master.m_frame;
    weightAnalyse(*frame->m_encData->m_slice, *frame, *master.m_param);
}

void FrameEncoder::compressFrame()
{
    ProfileScopeEvent(frameThread);

    m_startCompressTime = x265_mdate();
    m_totalActiveWorkerCount = 0;
    m_activeWorkerCountSamples = 0;
    m_totalWorkerElapsedTime = 0;
    m_totalNoWorkerTime = 0;
    m_countRowBlocks = 0;
    m_allRowsAvailableTime = 0;
    m_stallStartTime = 0;

    m_completionCount = 0;
    m_bAllRowsStop = false;
    m_vbvResetTriggerRow = -1;

    m_SSDY = m_SSDU = m_SSDV = 0;
    m_ssim = 0;
    m_ssimCnt = 0;
    memset(&(m_frame->m_encData->m_frameStats), 0, sizeof(m_frame->m_encData->m_frameStats));

    /* Emit access unit delimiter unless this is the first frame and the user is
     * not repeating headers (since AUD is supposed to be the first NAL in the access
     * unit) */
    Slice* slice = m_frame->m_encData->m_slice;
    if (m_param->bEnableAccessUnitDelimiters && (m_frame->m_poc || m_param->bRepeatHeaders))
    {
        m_bs.resetBits();
        m_entropyCoder.setBitstream(&m_bs);
        m_entropyCoder.codeAUD(*slice);
        m_bs.writeByteAlignment();
        m_nalList.serialize(NAL_UNIT_ACCESS_UNIT_DELIMITER, m_bs);
    }
    if (m_frame->m_lowres.bKeyframe && m_param->bRepeatHeaders)
        m_top->getStreamHeaders(m_nalList, m_entropyCoder, m_bs);

    // Weighted Prediction parameters estimation.
    bool bUseWeightP = slice->m_sliceType == P_SLICE && slice->m_pps->bUseWeightPred;
    bool bUseWeightB = slice->m_sliceType == B_SLICE && slice->m_pps->bUseWeightedBiPred;
    if (bUseWeightP || bUseWeightB)
    {
#if DETAILED_CU_STATS
        m_cuStats.countWeightAnalyze++;
        ScopedElapsedTime time(m_cuStats.weightAnalyzeTime);
#endif
        WeightAnalysis wa(*this);
        if (m_pool && wa.tryBondPeers(*this, 1))
            /* use an idle worker for weight analysis */
            wa.waitForExit();
        else
            weightAnalyse(*slice, *m_frame, *m_param);
    }
    else
        slice->disableWeights();

    // Generate motion references
    int numPredDir = slice->isInterP() ? 1 : slice->isInterB() ? 2 : 0;
    for (int l = 0; l < numPredDir; l++)
    {
        for (int ref = 0; ref < slice->m_numRefIdx[l]; ref++)
        {
            WeightParam *w = NULL;
            if ((bUseWeightP || bUseWeightB) && slice->m_weightPredTable[l][ref][0].bPresentFlag)
                w = slice->m_weightPredTable[l][ref];
            slice->m_refReconPicList[l][ref] = slice->m_refFrameList[l][ref]->m_reconPic;
            m_mref[l][ref].init(slice->m_refReconPicList[l][ref], w, *m_param);
        }
    }

    int numTLD;
    if (m_pool)
        numTLD = m_param->bEnableWavefront ? m_pool->m_numWorkers : m_pool->m_numWorkers + m_pool->m_numProviders;
    else
        numTLD = 1;

    /* Get the QP for this frame from rate control. This call may block until
     * frames ahead of it in encode order have called rateControlEnd() */
    int qp = m_top->m_rateControl->rateControlStart(m_frame, &m_rce, m_top);
    m_rce.newQp = qp;

    if (m_nr)
    {
        if (qp > QP_MAX_SPEC && m_frame->m_param->rc.vbvBufferSize)
        {
            for (int i = 0; i < numTLD; i++)
            {
                m_tld[i].analysis.m_quant.m_frameNr[m_jpId].offset = m_top->m_offsetEmergency[qp - QP_MAX_SPEC - 1];
                m_tld[i].analysis.m_quant.m_frameNr[m_jpId].residualSum = m_top->m_residualSumEmergency;
                m_tld[i].analysis.m_quant.m_frameNr[m_jpId].count = m_top->m_countEmergency;
            }
        }
        else
        {
            if (m_param->noiseReductionIntra || m_param->noiseReductionInter)
            {
                for (int i = 0; i < numTLD; i++)
                {
                    m_tld[i].analysis.m_quant.m_frameNr[m_jpId].offset = m_tld[i].analysis.m_quant.m_frameNr[m_jpId].nrOffsetDenoise;
                    m_tld[i].analysis.m_quant.m_frameNr[m_jpId].residualSum = m_tld[i].analysis.m_quant.m_frameNr[m_jpId].nrResidualSum;
                    m_tld[i].analysis.m_quant.m_frameNr[m_jpId].count = m_tld[i].analysis.m_quant.m_frameNr[m_jpId].nrCount;
                }
            }
            else
            {
                for (int i = 0; i < numTLD; i++)
                    m_tld[i].analysis.m_quant.m_frameNr[m_jpId].offset = NULL;
            }
        }
    }

    /* Clip slice QP to 0-51 spec range before encoding */
    slice->m_sliceQp = x265_clip3(-QP_BD_OFFSET, QP_MAX_SPEC, qp);

    m_initSliceContext.resetEntropy(*slice);

    m_frameFilter.start(m_frame, m_initSliceContext, qp);

    /* ensure all rows are blocked prior to initializing row CTU counters */
    WaveFront::clearEnabledRowMask();

    /* reset entropy coders */
    m_entropyCoder.load(m_initSliceContext);
    for (uint32_t i = 0; i < m_numRows; i++)
        m_rows[i].init(m_initSliceContext);

    uint32_t numSubstreams = m_param->bEnableWavefront ? slice->m_sps->numCuInHeight : 1;
    if (!m_outStreams)
    {
        m_outStreams = new Bitstream[numSubstreams];
        m_substreamSizes = X265_MALLOC(uint32_t, numSubstreams);
        if (!m_param->bEnableSAO)
            for (uint32_t i = 0; i < numSubstreams; i++)
                m_rows[i].rowGoOnCoder.setBitstream(&m_outStreams[i]);
    }
    else
        for (uint32_t i = 0; i < numSubstreams; i++)
            m_outStreams[i].resetBits();

    int prevBPSEI = m_rce.encodeOrder ? m_top->m_lastBPSEI : 0;

    if (m_frame->m_lowres.bKeyframe)
    {
        if (m_param->bEmitHRDSEI)
        {
            SEIBufferingPeriod* bpSei = &m_top->m_rateControl->m_bufPeriodSEI;

            // since the temporal layer HRD is not ready, we assumed it is fixed
            bpSei->m_auCpbRemovalDelayDelta = 1;
            bpSei->m_cpbDelayOffset = 0;
            bpSei->m_dpbDelayOffset = 0;

            // hrdFullness() calculates the initial CPB removal delay and offset
            m_top->m_rateControl->hrdFullness(bpSei);

            m_bs.resetBits();
            bpSei->write(m_bs, *slice->m_sps);
            m_bs.writeByteAlignment();

            m_nalList.serialize(NAL_UNIT_PREFIX_SEI, m_bs);

            m_top->m_lastBPSEI = m_rce.encodeOrder;
        }
    }

    if (m_param->bEmitHRDSEI || !!m_param->interlaceMode)
    {
        SEIPictureTiming *sei = m_rce.picTimingSEI;
        const VUI *vui = &slice->m_sps->vuiParameters;
        const HRDInfo *hrd = &vui->hrdParameters;
        int poc = slice->m_poc;

        if (vui->frameFieldInfoPresentFlag)
        {
            if (m_param->interlaceMode == 2)
                sei->m_picStruct = (poc & 1) ? 1 /* top */ : 2 /* bottom */;
            else if (m_param->interlaceMode == 1)
                sei->m_picStruct = (poc & 1) ? 2 /* bottom */ : 1 /* top */;
            else
                sei->m_picStruct = 0;
            sei->m_sourceScanType = 0;
            sei->m_duplicateFlag = false;
        }

        if (vui->hrdParametersPresentFlag)
        {
            // The m_aucpbremoval delay specifies how many clock ticks the
            // access unit associated with the picture timing SEI message has to
            // wait after removal of the access unit with the most recent
            // buffering period SEI message
            sei->m_auCpbRemovalDelay = X265_MIN(X265_MAX(1, m_rce.encodeOrder - prevBPSEI), (1 << hrd->cpbRemovalDelayLength));
            sei->m_picDpbOutputDelay = slice->m_sps->numReorderPics + poc - m_rce.encodeOrder;
        }

        m_bs.resetBits();
        sei->write(m_bs, *slice->m_sps);
        m_bs.writeByteAlignment();
        m_nalList.serialize(NAL_UNIT_PREFIX_SEI, m_bs);
    }

    /* CQP and CRF (without capped VBV) doesn't use mid-frame statistics to 
     * tune RateControl parameters for other frames.
     * Hence, for these modes, update m_startEndOrder and unlock RC for previous threads waiting in
     * RateControlEnd here, after the slice contexts are initialized. For the rest - ABR
     * and VBV, unlock only after rateControlUpdateStats of this frame is called */
    if (m_param->rc.rateControlMode != X265_RC_ABR && !m_top->m_rateControl->m_isVbv)
    {
        m_top->m_rateControl->m_startEndOrder.incr();

        if (m_rce.encodeOrder < m_param->frameNumThreads - 1)
            m_top->m_rateControl->m_startEndOrder.incr(); // faked rateControlEnd calls for negative frames
    }

    /* Analyze CTU rows, most of the hard work is done here.  Frame is
     * compressed in a wave-front pattern if WPP is enabled. Row based loop
     * filters runs behind the CTU compression and reconstruction */

    m_rows[0].active = true;
    if (m_param->bEnableWavefront)
    {
        for (uint32_t row = 0; row < m_numRows; row++)
        {
            // block until all reference frames have reconstructed the rows we need
            for (int l = 0; l < numPredDir; l++)
            {
                for (int ref = 0; ref < slice->m_numRefIdx[l]; ref++)
                {
                    Frame *refpic = slice->m_refFrameList[l][ref];

                    uint32_t reconRowCount = refpic->m_reconRowCount.get();
                    while ((reconRowCount != m_numRows) && (reconRowCount < row + m_refLagRows))
                        reconRowCount = refpic->m_reconRowCount.waitForChange(reconRowCount);

                    if ((bUseWeightP || bUseWeightB) && m_mref[l][ref].isWeighted)
                        m_mref[l][ref].applyWeight(row + m_refLagRows, m_numRows);
                }
            }

            enableRowEncoder(row); /* clear external dependency for this row */
            if (!row)
            {
                m_row0WaitTime = x265_mdate();
                enqueueRowEncoder(0); /* clear internal dependency, start wavefront */
            }
            tryWakeOne();
        }

        m_allRowsAvailableTime = x265_mdate();
        tryWakeOne(); /* ensure one thread is active or help-wanted flag is set prior to blocking */
        static const int block_ms = 250;
        while (m_completionEvent.timedWait(block_ms))
            tryWakeOne();
    }
    else
    {
        for (uint32_t i = 0; i < m_numRows + m_filterRowDelay; i++)
        {
            // compress
            if (i < m_numRows)
            {
                // block until all reference frames have reconstructed the rows we need
                for (int l = 0; l < numPredDir; l++)
                {
                    int list = l;
                    for (int ref = 0; ref < slice->m_numRefIdx[list]; ref++)
                    {
                        Frame *refpic = slice->m_refFrameList[list][ref];

                        uint32_t reconRowCount = refpic->m_reconRowCount.get();
                        while ((reconRowCount != m_numRows) && (reconRowCount < i + m_refLagRows))
                            reconRowCount = refpic->m_reconRowCount.waitForChange(reconRowCount);

                        if ((bUseWeightP || bUseWeightB) && m_mref[l][ref].isWeighted)
                            m_mref[list][ref].applyWeight(i + m_refLagRows, m_numRows);
                    }
                }

                if (!i)
                    m_row0WaitTime = x265_mdate();
                else if (i == m_numRows - 1)
                    m_allRowsAvailableTime = x265_mdate();
                processRowEncoder(i, m_tld[m_localTldIdx]);
            }

            // filter
            if (i >= m_filterRowDelay)
                m_frameFilter.processRow(i - m_filterRowDelay);
        }
    }

    if (m_param->rc.bStatWrite)
    {
        int totalI = 0, totalP = 0, totalSkip = 0;

        // accumulate intra,inter,skip cu count per frame for 2 pass
        for (uint32_t i = 0; i < m_numRows; i++)
        {
            m_frame->m_encData->m_frameStats.mvBits    += m_rows[i].rowStats.mvBits;
            m_frame->m_encData->m_frameStats.coeffBits += m_rows[i].rowStats.coeffBits;
            m_frame->m_encData->m_frameStats.miscBits  += m_rows[i].rowStats.miscBits;
            totalI                                     += m_rows[i].rowStats.intra8x8Cnt;
            totalP                                     += m_rows[i].rowStats.inter8x8Cnt;
            totalSkip                                  += m_rows[i].rowStats.skip8x8Cnt;
        }
        int totalCuCount = totalI + totalP + totalSkip;
        m_frame->m_encData->m_frameStats.percent8x8Intra = (double)totalI / totalCuCount;
        m_frame->m_encData->m_frameStats.percent8x8Inter = (double)totalP / totalCuCount;
        m_frame->m_encData->m_frameStats.percent8x8Skip  = (double)totalSkip / totalCuCount;
    }
    for (uint32_t i = 0; i < m_numRows; i++)
    {
        m_frame->m_encData->m_frameStats.cntIntraNxN      += m_rows[i].rowStats.cntIntraNxN;
        m_frame->m_encData->m_frameStats.totalCu          += m_rows[i].rowStats.totalCu;
        m_frame->m_encData->m_frameStats.totalCtu         += m_rows[i].rowStats.totalCtu;
        m_frame->m_encData->m_frameStats.lumaDistortion   += m_rows[i].rowStats.lumaDistortion;
        m_frame->m_encData->m_frameStats.chromaDistortion += m_rows[i].rowStats.chromaDistortion;
        m_frame->m_encData->m_frameStats.psyEnergy        += m_rows[i].rowStats.psyEnergy;
        m_frame->m_encData->m_frameStats.resEnergy        += m_rows[i].rowStats.resEnergy;
        for (uint32_t depth = 0; depth <= g_maxCUDepth; depth++)
        {
            m_frame->m_encData->m_frameStats.cntSkipCu[depth] += m_rows[i].rowStats.cntSkipCu[depth];
            m_frame->m_encData->m_frameStats.cntMergeCu[depth] += m_rows[i].rowStats.cntMergeCu[depth];
            for (int m = 0; m < INTER_MODES; m++)
                m_frame->m_encData->m_frameStats.cuInterDistribution[depth][m] += m_rows[i].rowStats.cuInterDistribution[depth][m];
            for (int n = 0; n < INTRA_MODES; n++)
                m_frame->m_encData->m_frameStats.cuIntraDistribution[depth][n] += m_rows[i].rowStats.cuIntraDistribution[depth][n];
        }
    }
    m_frame->m_encData->m_frameStats.avgLumaDistortion   = (double)(m_frame->m_encData->m_frameStats.lumaDistortion) / m_frame->m_encData->m_frameStats.totalCtu;
    m_frame->m_encData->m_frameStats.avgChromaDistortion = (double)(m_frame->m_encData->m_frameStats.chromaDistortion) / m_frame->m_encData->m_frameStats.totalCtu;
    m_frame->m_encData->m_frameStats.avgPsyEnergy        = (double)(m_frame->m_encData->m_frameStats.psyEnergy) / m_frame->m_encData->m_frameStats.totalCtu;
    m_frame->m_encData->m_frameStats.avgResEnergy        = (double)(m_frame->m_encData->m_frameStats.resEnergy) / m_frame->m_encData->m_frameStats.totalCtu;
    m_frame->m_encData->m_frameStats.percentIntraNxN     = (double)(m_frame->m_encData->m_frameStats.cntIntraNxN * 100) / m_frame->m_encData->m_frameStats.totalCu;
    for (uint32_t depth = 0; depth <= g_maxCUDepth; depth++)
    {
        m_frame->m_encData->m_frameStats.percentSkipCu[depth]  = (double)(m_frame->m_encData->m_frameStats.cntSkipCu[depth] * 100) / m_frame->m_encData->m_frameStats.totalCu;
        m_frame->m_encData->m_frameStats.percentMergeCu[depth] = (double)(m_frame->m_encData->m_frameStats.cntMergeCu[depth] * 100) / m_frame->m_encData->m_frameStats.totalCu;
        for (int n = 0; n < INTRA_MODES; n++)
            m_frame->m_encData->m_frameStats.percentIntraDistribution[depth][n] = (double)(m_frame->m_encData->m_frameStats.cuIntraDistribution[depth][n] * 100) / m_frame->m_encData->m_frameStats.totalCu;
        uint64_t cuInterRectCnt = 0; // sum of Nx2N, 2NxN counts
        cuInterRectCnt += m_frame->m_encData->m_frameStats.cuInterDistribution[depth][1] + m_frame->m_encData->m_frameStats.cuInterDistribution[depth][2];
        m_frame->m_encData->m_frameStats.percentInterDistribution[depth][0] = (double)(m_frame->m_encData->m_frameStats.cuInterDistribution[depth][0] * 100) / m_frame->m_encData->m_frameStats.totalCu;
        m_frame->m_encData->m_frameStats.percentInterDistribution[depth][1] = (double)(cuInterRectCnt * 100) / m_frame->m_encData->m_frameStats.totalCu;
        m_frame->m_encData->m_frameStats.percentInterDistribution[depth][2] = (double)(m_frame->m_encData->m_frameStats.cuInterDistribution[depth][3] * 100) / m_frame->m_encData->m_frameStats.totalCu;
    }

    m_bs.resetBits();
    m_entropyCoder.load(m_initSliceContext);
    m_entropyCoder.setBitstream(&m_bs);
    m_entropyCoder.codeSliceHeader(*slice, *m_frame->m_encData);

    // finish encode of each CTU row, only required when SAO is enabled
    if (m_param->bEnableSAO)
        encodeSlice();

    // serialize each row, record final lengths in slice header
    uint32_t maxStreamSize = m_nalList.serializeSubstreams(m_substreamSizes, numSubstreams, m_outStreams);

    // complete the slice header by writing WPP row-starts
    m_entropyCoder.setBitstream(&m_bs);
    if (slice->m_pps->bEntropyCodingSyncEnabled)
        m_entropyCoder.codeSliceHeaderWPPEntryPoints(*slice, m_substreamSizes, maxStreamSize);
    m_bs.writeByteAlignment();

    m_nalList.serialize(slice->m_nalUnitType, m_bs);

    if (m_param->decodedPictureHashSEI)
    {
        if (m_param->decodedPictureHashSEI == 1)
        {
            m_seiReconPictureDigest.m_method = SEIDecodedPictureHash::MD5;
            for (int i = 0; i < 3; i++)
                MD5Final(&m_state[i], m_seiReconPictureDigest.m_digest[i]);
        }
        else if (m_param->decodedPictureHashSEI == 2)
        {
            m_seiReconPictureDigest.m_method = SEIDecodedPictureHash::CRC;
            for (int i = 0; i < 3; i++)
                crcFinish(m_crc[i], m_seiReconPictureDigest.m_digest[i]);
        }
        else if (m_param->decodedPictureHashSEI == 3)
        {
            m_seiReconPictureDigest.m_method = SEIDecodedPictureHash::CHECKSUM;
            for (int i = 0; i < 3; i++)
                checksumFinish(m_checksum[i], m_seiReconPictureDigest.m_digest[i]);
        }

        m_bs.resetBits();
        m_seiReconPictureDigest.write(m_bs, *slice->m_sps);
        m_bs.writeByteAlignment();

        m_nalList.serialize(NAL_UNIT_SUFFIX_SEI, m_bs);
    }

    uint64_t bytes = 0;
    for (uint32_t i = 0; i < m_nalList.m_numNal; i++)
    {
        int type = m_nalList.m_nal[i].type;

        // exclude SEI
        if (type != NAL_UNIT_PREFIX_SEI && type != NAL_UNIT_SUFFIX_SEI)
        {
            bytes += m_nalList.m_nal[i].sizeBytes;
            // and exclude start code prefix
            bytes -= (!i || type == NAL_UNIT_SPS || type == NAL_UNIT_PPS) ? 4 : 3;
        }
    }
    m_accessUnitBits = bytes << 3;

    m_endCompressTime = x265_mdate();

    /* rateControlEnd may also block for earlier frames to call rateControlUpdateStats */
    if (m_top->m_rateControl->rateControlEnd(m_frame, m_accessUnitBits, &m_rce) < 0)
        m_top->m_aborted = true;

    /* Decrement referenced frame reference counts, allow them to be recycled */
    for (int l = 0; l < numPredDir; l++)
    {
        for (int ref = 0; ref < slice->m_numRefIdx[l]; ref++)
        {
            Frame *refpic = slice->m_refFrameList[l][ref];
            ATOMIC_DEC(&refpic->m_countRefEncoders);
        }
    }

    if (m_nr)
    {
        bool nrEnabled = (m_rce.newQp < QP_MAX_SPEC || !m_param->rc.vbvBufferSize) && (m_param->noiseReductionIntra || m_param->noiseReductionInter);

        if (nrEnabled)
        {
            /* Accumulate NR statistics from all worker threads */
            for (int i = 0; i < numTLD; i++)
            {
                NoiseReduction* nr = &m_tld[i].analysis.m_quant.m_frameNr[m_jpId];
                for (int cat = 0; cat < MAX_NUM_TR_CATEGORIES; cat++)
                {
                    for (int coeff = 0; coeff < MAX_NUM_TR_COEFFS; coeff++)
                        m_nr->nrResidualSum[cat][coeff] += nr->nrResidualSum[cat][coeff];

                    m_nr->nrCount[cat] += nr->nrCount[cat];
                }
            }

            noiseReductionUpdate();

            /* Copy updated NR coefficients back to all worker threads */
            for (int i = 0; i < numTLD; i++)
            {
                NoiseReduction* nr = &m_tld[i].analysis.m_quant.m_frameNr[m_jpId];
                memcpy(nr->nrOffsetDenoise, m_nr->nrOffsetDenoise, sizeof(uint16_t)* MAX_NUM_TR_CATEGORIES * MAX_NUM_TR_COEFFS);
                memset(nr->nrCount, 0, sizeof(uint32_t)* MAX_NUM_TR_CATEGORIES);
                memset(nr->nrResidualSum, 0, sizeof(uint32_t)* MAX_NUM_TR_CATEGORIES * MAX_NUM_TR_COEFFS);
            }
        }
    }

#if DETAILED_CU_STATS
    /* Accumulate CU statistics from each worker thread, we could report
     * per-frame stats here, but currently we do not. */
    for (int i = 0; i < numTLD; i++)
        m_cuStats.accumulate(m_tld[i].analysis.m_stats[m_jpId]);
#endif

    m_endFrameTime = x265_mdate();
}

void FrameEncoder::encodeSlice()
{
    Slice* slice = m_frame->m_encData->m_slice;
    const uint32_t widthInLCUs = slice->m_sps->numCuInWidth;
    const uint32_t lastCUAddr = (slice->m_endCUAddr + NUM_4x4_PARTITIONS - 1) / NUM_4x4_PARTITIONS;
    const uint32_t numSubstreams = m_param->bEnableWavefront ? slice->m_sps->numCuInHeight : 1;

    SAOParam* saoParam = slice->m_sps->bUseSAO ? m_frame->m_encData->m_saoParam : NULL;
    for (uint32_t cuAddr = 0; cuAddr < lastCUAddr; cuAddr++)
    {
        uint32_t col = cuAddr % widthInLCUs;
        uint32_t lin = cuAddr / widthInLCUs;
        uint32_t subStrm = lin % numSubstreams;
        CUData* ctu = m_frame->m_encData->getPicCTU(cuAddr);

        m_entropyCoder.setBitstream(&m_outStreams[subStrm]);

        // Synchronize cabac probabilities with upper-right CTU if it's available and we're at the start of a line.
        if (m_param->bEnableWavefront && !col && lin)
        {
            m_entropyCoder.copyState(m_initSliceContext);
            m_entropyCoder.loadContexts(m_rows[lin - 1].bufferedEntropy);
        }

        if (saoParam)
        {
            if (saoParam->bSaoFlag[0] || saoParam->bSaoFlag[1])
            {
                int mergeLeft = col && saoParam->ctuParam[0][cuAddr].mergeMode == SAO_MERGE_LEFT;
                int mergeUp = lin && saoParam->ctuParam[0][cuAddr].mergeMode == SAO_MERGE_UP;
                if (col)
                    m_entropyCoder.codeSaoMerge(mergeLeft);
                if (lin && !mergeLeft)
                    m_entropyCoder.codeSaoMerge(mergeUp);
                if (!mergeLeft && !mergeUp)
                {
                    if (saoParam->bSaoFlag[0])
                        m_entropyCoder.codeSaoOffset(saoParam->ctuParam[0][cuAddr], 0);
                    if (saoParam->bSaoFlag[1])
                    {
                        m_entropyCoder.codeSaoOffset(saoParam->ctuParam[1][cuAddr], 1);
                        m_entropyCoder.codeSaoOffset(saoParam->ctuParam[2][cuAddr], 2);
                    }
                }
            }
            else
            {
                for (int i = 0; i < 3; i++)
                    saoParam->ctuParam[i][cuAddr].reset();
            }
        }

        // final coding (bitstream generation) for this CU
        m_entropyCoder.encodeCTU(*ctu, m_cuGeoms[m_ctuGeomMap[cuAddr]]);

        if (m_param->bEnableWavefront)
        {
            if (col == 1)
                // Store probabilities of second CTU in line into buffer
                m_rows[lin].bufferedEntropy.loadContexts(m_entropyCoder);

            if (col == widthInLCUs - 1)
                m_entropyCoder.finishSlice();
        }
    }
    if (!m_param->bEnableWavefront)
        m_entropyCoder.finishSlice();
}

void FrameEncoder::processRow(int row, int threadId)
{
    int64_t startTime = x265_mdate();
    if (ATOMIC_INC(&m_activeWorkerCount) == 1 && m_stallStartTime)
        m_totalNoWorkerTime += x265_mdate() - m_stallStartTime;

    const uint32_t realRow = row >> 1;
    const uint32_t typeNum = row & 1;

    if (!typeNum)
        processRowEncoder(realRow, m_tld[threadId]);
    else
    {
        m_frameFilter.processRow(realRow);

        // NOTE: Active next row
        if (realRow != m_numRows - 1)
            enqueueRowFilter(realRow + 1);
    }

    if (ATOMIC_DEC(&m_activeWorkerCount) == 0)
        m_stallStartTime = x265_mdate();

    m_totalWorkerElapsedTime += x265_mdate() - startTime; // not thread safe, but good enough
}

// Called by worker threads
void FrameEncoder::processRowEncoder(int intRow, ThreadLocalData& tld)
{
    uint32_t row = (uint32_t)intRow;
    CTURow& curRow = m_rows[row];

    tld.analysis.m_param = m_param;
    if (m_param->bEnableWavefront)
    {
        ScopedLock self(curRow.lock);
        if (!curRow.active)
            /* VBV restart is in progress, exit out */
            return;
        if (curRow.busy)
        {
            /* On multi-socket Windows servers, we have seen problems with
             * ATOMIC_CAS which resulted in multiple worker threads processing
             * the same CU row, which often resulted in bad pointer accesses. We
             * believe the problem is fixed, but are leaving this check in place
             * to prevent crashes in case it is not */
            x265_log(m_param, X265_LOG_WARNING,
                     "internal error - simultaneous row access detected. Please report HW to x265-devel@videolan.org\n");
            return;
        }
        curRow.busy = true;
    }

    /* When WPP is enabled, every row has its own row coder instance. Otherwise
     * they share row 0 */
    Entropy& rowCoder = m_param->bEnableWavefront ? m_rows[row].rowGoOnCoder : m_rows[0].rowGoOnCoder;
    FrameData& curEncData = *m_frame->m_encData;
    Slice *slice = curEncData.m_slice;

    const uint32_t numCols = m_numCols;
    const uint32_t lineStartCUAddr = row * numCols;
    bool bIsVbv = m_param->rc.vbvBufferSize > 0 && m_param->rc.vbvMaxBitrate > 0;

    uint32_t maxBlockCols = (m_frame->m_fencPic->m_picWidth + (16 - 1)) / 16;
    uint32_t maxBlockRows = (m_frame->m_fencPic->m_picHeight + (16 - 1)) / 16;
    uint32_t noOfBlocks = g_maxCUSize / 16;

    while (curRow.completed < numCols)
    {
        ProfileScopeEvent(encodeCTU);

        uint32_t col = curRow.completed;
        const uint32_t cuAddr = lineStartCUAddr + col;
        CUData* ctu = curEncData.getPicCTU(cuAddr);
        ctu->initCTU(*m_frame, cuAddr, slice->m_sliceQp);

        if (bIsVbv)
        {
            if (!row)
            {
                curEncData.m_rowStat[row].diagQp = curEncData.m_avgQpRc;
                curEncData.m_rowStat[row].diagQpScale = x265_qp2qScale(curEncData.m_avgQpRc);
            }

            FrameData::RCStatCU& cuStat = curEncData.m_cuStat[cuAddr];
            if (row >= col && row && m_vbvResetTriggerRow != intRow)
                cuStat.baseQp = curEncData.m_cuStat[cuAddr - numCols + 1].baseQp;
            else
                cuStat.baseQp = curEncData.m_rowStat[row].diagQp;

            /* TODO: use defines from slicetype.h for lowres block size */
            uint32_t block_y = (ctu->m_cuPelY >> g_maxLog2CUSize) * noOfBlocks;
            uint32_t block_x = (ctu->m_cuPelX >> g_maxLog2CUSize) * noOfBlocks;
            
            cuStat.vbvCost = 0;
            cuStat.intraVbvCost = 0;
            for (uint32_t h = 0; h < noOfBlocks && block_y < maxBlockRows; h++, block_y++)
            {
                uint32_t idx = block_x + (block_y * maxBlockCols);

                for (uint32_t w = 0; w < noOfBlocks && (block_x + w) < maxBlockCols; w++, idx++)
                {
                    cuStat.vbvCost += m_frame->m_lowres.lowresCostForRc[idx] & LOWRES_COST_MASK;
                    cuStat.intraVbvCost += m_frame->m_lowres.intraCost[idx];
                }
            }
        }
        else
            curEncData.m_cuStat[cuAddr].baseQp = curEncData.m_avgQpRc;

        if (m_param->bEnableWavefront && !col && row)
        {
            // Load SBAC coder context from previous row and initialize row state.
            rowCoder.copyState(m_initSliceContext);
            rowCoder.loadContexts(m_rows[row - 1].bufferedEntropy);
        }

        // Does all the CU analysis, returns best top level mode decision
        Mode& best = tld.analysis.compressCTU(*ctu, *m_frame, m_cuGeoms[m_ctuGeomMap[cuAddr]], rowCoder);

        // take a sample of the current active worker count
        ATOMIC_ADD(&m_totalActiveWorkerCount, m_activeWorkerCount);
        ATOMIC_INC(&m_activeWorkerCountSamples);

        /* advance top-level row coder to include the context of this CTU.
         * if SAO is disabled, rowCoder writes the final CTU bitstream */
        rowCoder.encodeCTU(*ctu, m_cuGeoms[m_ctuGeomMap[cuAddr]]);

        if (m_param->bEnableWavefront && col == 1)
            // Save CABAC state for next row
            curRow.bufferedEntropy.loadContexts(rowCoder);

        // Completed CU processing
        curRow.completed++;

        FrameStats frameLog;
        curEncData.m_rowStat[row].sumQpAq += collectCTUStatistics(*ctu, &frameLog);

        // copy no. of intra, inter Cu cnt per row into frame stats for 2 pass
        if (m_param->rc.bStatWrite)
        {
            curRow.rowStats.mvBits    += best.mvBits;
            curRow.rowStats.coeffBits += best.coeffBits;
            curRow.rowStats.miscBits  += best.totalBits - (best.mvBits + best.coeffBits);

            for (uint32_t depth = 0; depth <= g_maxCUDepth; depth++)
            {
                /* 1 << shift == number of 8x8 blocks at current depth */
                int shift = 2 * (g_maxCUDepth - depth);
                int cuSize = g_maxCUSize >> depth;

                if (cuSize == 8)
                    curRow.rowStats.intra8x8Cnt += (int)(frameLog.cntIntra[depth] + frameLog.cntIntraNxN);
                else
                    curRow.rowStats.intra8x8Cnt += (int)(frameLog.cntIntra[depth] << shift);

                curRow.rowStats.inter8x8Cnt += (int)(frameLog.cntInter[depth] << shift);
                curRow.rowStats.skip8x8Cnt += (int)((frameLog.cntSkipCu[depth] + frameLog.cntMergeCu[depth]) << shift);
            }
        }
        curRow.rowStats.totalCtu++;
        curRow.rowStats.lumaDistortion   += best.lumaDistortion;
        curRow.rowStats.chromaDistortion += best.chromaDistortion;
        curRow.rowStats.psyEnergy        += best.psyEnergy;
        curRow.rowStats.resEnergy        += best.resEnergy;
        curRow.rowStats.cntIntraNxN      += frameLog.cntIntraNxN;
        curRow.rowStats.totalCu          += frameLog.totalCu;
        for (uint32_t depth = 0; depth <= g_maxCUDepth; depth++)
        {
            curRow.rowStats.cntSkipCu[depth] += frameLog.cntSkipCu[depth];
            curRow.rowStats.cntMergeCu[depth] += frameLog.cntMergeCu[depth];
            for (int m = 0; m < INTER_MODES; m++)
                curRow.rowStats.cuInterDistribution[depth][m] += frameLog.cuInterDistribution[depth][m];
            for (int n = 0; n < INTRA_MODES; n++)
                curRow.rowStats.cuIntraDistribution[depth][n] += frameLog.cuIntraDistribution[depth][n];
        }

        curEncData.m_cuStat[cuAddr].totalBits = best.totalBits;
        x265_emms();

        if (bIsVbv)
        {
            // Update encoded bits, satdCost, baseQP for each CU
            curEncData.m_rowStat[row].diagSatd      += curEncData.m_cuStat[cuAddr].vbvCost;
            curEncData.m_rowStat[row].diagIntraSatd += curEncData.m_cuStat[cuAddr].intraVbvCost;
            curEncData.m_rowStat[row].encodedBits   += curEncData.m_cuStat[cuAddr].totalBits;
            curEncData.m_rowStat[row].sumQpRc       += curEncData.m_cuStat[cuAddr].baseQp;
            curEncData.m_rowStat[row].numEncodedCUs = cuAddr;

            // If current block is at row diagonal checkpoint, call vbv ratecontrol.

            if (row == col && row)
            {
                double qpBase = curEncData.m_cuStat[cuAddr].baseQp;
                int reEncode = m_top->m_rateControl->rowDiagonalVbvRateControl(m_frame, row, &m_rce, qpBase);
                qpBase = x265_clip3((double)QP_MIN, (double)QP_MAX_MAX, qpBase);
                curEncData.m_rowStat[row].diagQp = qpBase;
                curEncData.m_rowStat[row].diagQpScale =  x265_qp2qScale(qpBase);

                if (reEncode < 0)
                {
                    x265_log(m_param, X265_LOG_DEBUG, "POC %d row %d - encode restart required for VBV, to %.2f from %.2f\n",
                             m_frame->m_poc, row, qpBase, curEncData.m_cuStat[cuAddr].baseQp);

                    // prevent the WaveFront::findJob() method from providing new jobs
                    m_vbvResetTriggerRow = row;
                    m_bAllRowsStop = true;

                    for (uint32_t r = m_numRows - 1; r >= row; r--)
                    {
                        CTURow& stopRow = m_rows[r];

                        if (r != row)
                        {
                            /* if row was active (ready to be run) clear active bit and bitmap bit for this row */
                            stopRow.lock.acquire();
                            while (stopRow.active)
                            {
                                if (dequeueRow(r * 2))
                                    stopRow.active = false;
                                else
                                {
                                    /* we must release the row lock to allow the thread to exit */
                                    stopRow.lock.release();
                                    GIVE_UP_TIME();
                                    stopRow.lock.acquire();
                                }
                            }
                            stopRow.lock.release();

                            bool bRowBusy = true;
                            do
                            {
                                stopRow.lock.acquire();
                                bRowBusy = stopRow.busy;
                                stopRow.lock.release();

                                if (bRowBusy)
                                {
                                    GIVE_UP_TIME();
                                }
                            }
                            while (bRowBusy);
                        }

                        m_outStreams[r].resetBits();
                        stopRow.completed = 0;
                        memset(&stopRow.rowStats, 0, sizeof(stopRow.rowStats));
                        curEncData.m_rowStat[r].numEncodedCUs = 0;
                        curEncData.m_rowStat[r].encodedBits = 0;
                        curEncData.m_rowStat[r].diagSatd = 0;
                        curEncData.m_rowStat[r].diagIntraSatd = 0;
                        curEncData.m_rowStat[r].sumQpRc = 0;
                        curEncData.m_rowStat[r].sumQpAq = 0;
                    }

                    m_bAllRowsStop = false;
                }
            }
        }

        /* SAO parameter estimation using non-deblocked pixels for CTU bottom and right boundary areas */
        if (m_param->bEnableSAO && m_param->bSaoNonDeblocked)
            m_frameFilter.m_sao.calcSaoStatsCu_BeforeDblk(m_frame, col, row);

        if (m_param->bEnableWavefront && curRow.completed >= 2 && row < m_numRows - 1 &&
            (!m_bAllRowsStop || intRow + 1 < m_vbvResetTriggerRow))
        {
            /* activate next row */
            ScopedLock below(m_rows[row + 1].lock);
            if (m_rows[row + 1].active == false &&
                m_rows[row + 1].completed + 2 <= curRow.completed)
            {
                m_rows[row + 1].active = true;
                enqueueRowEncoder(row + 1);
                tryWakeOne(); /* wake up a sleeping thread or set the help wanted flag */
            }
        }

        ScopedLock self(curRow.lock);
        if ((m_bAllRowsStop && intRow > m_vbvResetTriggerRow) ||
            (row > 0 && curRow.completed < numCols - 1 && m_rows[row - 1].completed < m_rows[row].completed + 2))
        {
            curRow.active = false;
            curRow.busy = false;
            ATOMIC_INC(&m_countRowBlocks);
            return;
        }
    }

    /** this row of CTUs has been compressed **/

    /* If encoding with ABR, update update bits and complexity in rate control
     * after a number of rows so the next frame's rateControlStart has more
     * accurate data for estimation. At the start of the encode we update stats
     * after half the frame is encoded, but after this initial period we update
     * after refLagRows (the number of rows reference frames must have completed
     * before referencees may begin encoding) */
    uint32_t rowCount = 0;
    if (m_param->rc.rateControlMode == X265_RC_ABR || bIsVbv)
    {
        if ((uint32_t)m_rce.encodeOrder <= 2 * (m_param->fpsNum / m_param->fpsDenom))
            rowCount = X265_MIN((m_numRows + 1) / 2, m_numRows - 1);
        else
            rowCount = X265_MIN(m_refLagRows, m_numRows - 1);
        if (row == rowCount)
        {
            m_rce.rowTotalBits = 0;
            if (bIsVbv)
                for (uint32_t i = 0; i < rowCount; i++)
                    m_rce.rowTotalBits += curEncData.m_rowStat[i].encodedBits;
            else
                for (uint32_t cuAddr = 0; cuAddr < rowCount * numCols; cuAddr++)
                    m_rce.rowTotalBits += curEncData.m_cuStat[cuAddr].totalBits;

            m_top->m_rateControl->rateControlUpdateStats(&m_rce);
        }
    }

    /* flush row bitstream (if WPP and no SAO) or flush frame if no WPP and no SAO */
    if (!m_param->bEnableSAO && (m_param->bEnableWavefront || row == m_numRows - 1))
        rowCoder.finishSlice();

    if (m_param->bEnableWavefront)
    {
        /* trigger row-wise loop filters */
        if (row >= m_filterRowDelay)
        {
            enableRowFilter(row - m_filterRowDelay);

            /* NOTE: Activate filter if first row (row 0) */
            if (row == m_filterRowDelay)
                enqueueRowFilter(0);
            tryWakeOne();
        }
        if (row == m_numRows - 1)
        {
            for (uint32_t i = m_numRows - m_filterRowDelay; i < m_numRows; i++)
                enableRowFilter(i);
            tryWakeOne();
        }
    }

    tld.analysis.m_param = NULL;
    curRow.busy = false;

    if (ATOMIC_INC(&m_completionCount) == 2 * (int)m_numRows)
        m_completionEvent.trigger();
}

/* collect statistics about CU coding decisions, return total QP */
int FrameEncoder::collectCTUStatistics(const CUData& ctu, FrameStats* log)
{
    int totQP = 0;
    if (ctu.m_slice->m_sliceType == I_SLICE)
    {
        uint32_t depth = 0;
        for (uint32_t absPartIdx = 0; absPartIdx < ctu.m_numPartitions; absPartIdx += ctu.m_numPartitions >> (depth * 2))
        {
            depth = ctu.m_cuDepth[absPartIdx];

            log->totalCu++;
            log->cntIntra[depth]++;
            totQP += ctu.m_qp[absPartIdx] * (ctu.m_numPartitions >> (depth * 2));

            if (ctu.m_predMode[absPartIdx] == MODE_NONE)
            {
                log->totalCu--;
                log->cntIntra[depth]--;
            }
            else if (ctu.m_partSize[absPartIdx] != SIZE_2Nx2N)
            {
                /* TODO: log intra modes at absPartIdx +0 to +3 */
                X265_CHECK(ctu.m_log2CUSize[absPartIdx] == 3 && ctu.m_slice->m_sps->quadtreeTULog2MinSize < 3, "Intra NxN found at improbable depth\n");
                log->cntIntraNxN++;
                log->cntIntra[depth]--;
            }
            else if (ctu.m_lumaIntraDir[absPartIdx] > 1)
                log->cuIntraDistribution[depth][ANGULAR_MODE_ID]++;
            else
                log->cuIntraDistribution[depth][ctu.m_lumaIntraDir[absPartIdx]]++;
        }
    }
    else
    {
        uint32_t depth = 0;
        for (uint32_t absPartIdx = 0; absPartIdx < ctu.m_numPartitions; absPartIdx += ctu.m_numPartitions >> (depth * 2))
        {
            depth = ctu.m_cuDepth[absPartIdx];

            log->totalCu++;
            totQP += ctu.m_qp[absPartIdx] * (ctu.m_numPartitions >> (depth * 2));

            if (ctu.m_predMode[absPartIdx] == MODE_NONE)
                log->totalCu--;
            else if (ctu.isSkipped(absPartIdx))
            {
                if (ctu.m_mergeFlag[0])
                    log->cntMergeCu[depth]++;
                else
                    log->cntSkipCu[depth]++;
            }
            else if (ctu.isInter(absPartIdx))
            {
                log->cntInter[depth]++;

                if (ctu.m_partSize[absPartIdx] < AMP_ID)
                    log->cuInterDistribution[depth][ctu.m_partSize[absPartIdx]]++;
                else
                    log->cuInterDistribution[depth][AMP_ID]++;
            }
            else if (ctu.isIntra(absPartIdx))
            {
                log->cntIntra[depth]++;

                if (ctu.m_partSize[absPartIdx] != SIZE_2Nx2N)
                {
                    X265_CHECK(ctu.m_log2CUSize[absPartIdx] == 3 && ctu.m_slice->m_sps->quadtreeTULog2MinSize < 3, "Intra NxN found at improbable depth\n");
                    log->cntIntraNxN++;
                    log->cntIntra[depth]--;
                    /* TODO: log intra modes at absPartIdx +0 to +3 */
                }
                else if (ctu.m_lumaIntraDir[absPartIdx] > 1)
                    log->cuIntraDistribution[depth][ANGULAR_MODE_ID]++;
                else
                    log->cuIntraDistribution[depth][ctu.m_lumaIntraDir[absPartIdx]]++;
            }
        }
    }

    return totQP;
}

/* DCT-domain noise reduction / adaptive deadzone from libavcodec */
void FrameEncoder::noiseReductionUpdate()
{
    static const uint32_t maxBlocksPerTrSize[4] = {1 << 18, 1 << 16, 1 << 14, 1 << 12};

    for (int cat = 0; cat < MAX_NUM_TR_CATEGORIES; cat++)
    {
        int trSize = cat & 3;
        int coefCount = 1 << ((trSize + 2) * 2);

        if (m_nr->nrCount[cat] > maxBlocksPerTrSize[trSize])
        {
            for (int i = 0; i < coefCount; i++)
                m_nr->nrResidualSum[cat][i] >>= 1;
            m_nr->nrCount[cat] >>= 1;
        }

        int nrStrength = cat < 8 ? m_param->noiseReductionIntra : m_param->noiseReductionInter;
        uint64_t scaledCount = (uint64_t)nrStrength * m_nr->nrCount[cat];

        for (int i = 0; i < coefCount; i++)
        {
            uint64_t value = scaledCount + m_nr->nrResidualSum[cat][i] / 2;
            uint64_t denom = m_nr->nrResidualSum[cat][i] + 1;
            m_nr->nrOffsetDenoise[cat][i] = (uint16_t)(value / denom);
        }

        // Don't denoise DC coefficients
        m_nr->nrOffsetDenoise[cat][0] = 0;
    }
}

Frame *FrameEncoder::getEncodedPicture(NALList& output)
{
    if (m_frame)
    {
        /* block here until worker thread completes */
        m_done.wait();

        Frame *ret = m_frame;
        m_frame = NULL;
        output.takeContents(m_nalList);
        m_prevOutputTime = x265_mdate();
        return ret;
    }

    return NULL;
}
}
