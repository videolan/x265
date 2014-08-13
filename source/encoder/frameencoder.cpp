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

#include "PPA/ppa.h"
#include "wavefront.h"
#include "param.h"

#include "encoder.h"
#include "frameencoder.h"
#include "cturow.h"
#include "common.h"
#include "slicetype.h"
#include "nal.h"

namespace x265 {
void weightAnalyse(Slice& slice, x265_param& param);

FrameEncoder::FrameEncoder()
    : WaveFront(NULL)
    , m_threadActive(true)
    , m_rows(NULL)
    , m_top(NULL)
    , m_param(NULL)
    , m_frame(NULL)
{
    m_totalTime = 0;
    m_bAllRowsStop = false;
    m_vbvResetTriggerRow = -1;
    m_outStreams = NULL;
    m_substreamSizes = NULL;
    memset(&m_frameStats, 0, sizeof(m_frameStats));
    memset(&m_rce, 0, sizeof(RateControlEntry));
}

void FrameEncoder::destroy()
{
    JobProvider::flush();  // ensure no worker threads are using this frame

    m_threadActive = false;
    m_enable.trigger();

    delete[] m_rows;

    if (m_param->bEmitHRDSEI)
    {
        delete m_rce.picTimingSEI;
        delete m_rce.hrdTiming;
    }

    delete[] m_outStreams;
    X265_FREE(m_substreamSizes);
    m_frameFilter.destroy();

    // wait for worker thread to exit
    stop();
}

bool FrameEncoder::init(Encoder *top, int numRows, int numCols)
{
    m_top = top;
    m_param = top->m_param;
    m_numRows = numRows;
    m_numCols = numCols;
    m_filterRowDelay = (m_param->bEnableSAO && m_param->saoLcuBasedOptimization && m_param->saoLcuBoundary) ?
                        2 : (m_param->bEnableSAO || m_param->bEnableLoopFilter ? 1 : 0);
    m_filterRowDelayCus = m_filterRowDelay * numCols;

    m_rows = new CTURow[m_numRows];
    bool ok = !!m_numRows;

    int range  = m_param->searchRange; /* fpel search */
        range += 1;                    /* diamond search range check lag */
        range += 2;                    /* subpel refine */
        range += NTAPS_LUMA / 2;       /* subpel filter half-length */
    m_refLagRows = 1 + ((range + g_maxCUSize - 1) / g_maxCUSize);

    // NOTE: 2 times of numRows because both Encoder and Filter in same queue
    if (!WaveFront::init(m_numRows * 2))
    {
        x265_log(m_param, X265_LOG_ERROR, "unable to initialize wavefront queue\n");
        m_pool = NULL;
    }

    m_tld.init(*top);
    m_frameFilter.init(top, this, numRows, &m_rows[0].m_entropyCoder);

    // initialize HRD parameters of SPS
    if (m_param->bEmitHRDSEI)
    {
        m_rce.picTimingSEI = new SEIPictureTiming;
        m_rce.hrdTiming = new HRDTiming;

        ok &= m_rce.picTimingSEI && m_rce.hrdTiming;
    }

    memset(&m_frameStats, 0, sizeof(m_frameStats));
    memset(&m_nr, 0, sizeof(m_nr));
    m_nr.bNoiseReduction = !!m_param->noiseReduction;

    start();
    return ok;
}

void FrameEncoder::startCompressFrame(Frame* pic)
{
    m_frame = pic;
    pic->m_picSym->m_slice->m_mref = m_mref;
    m_enable.trigger();
}

void FrameEncoder::threadMain()
{
    // worker thread routine for FrameEncoder
    do
    {
        m_enable.wait(); // Encoder::encode() triggers this event
        if (m_threadActive)
        {
            compressFrame();
            m_done.trigger(); // FrameEncoder::getEncodedPicture() blocks for this event
        }
    }
    while (m_threadActive);
}

void FrameEncoder::compressFrame()
{
    PPAScopeEvent(FrameEncoder_compressFrame);
    int64_t startCompressTime = x265_mdate();
    Slice* slice = m_frame->m_picSym->m_slice;

    /* Emit access unit delimiter unless this is the first frame and the user is
     * not repeating headers (since AUD is supposed to be the first NAL in the access
     * unit) */
    if (m_param->bEnableAccessUnitDelimiters && (m_frame->getPOC() || m_param->bRepeatHeaders))
    {
        m_bs.resetBits();
        m_entropyCoder.setBitstream(&m_bs);
        m_entropyCoder.codeAUD(slice);
        m_bs.writeByteAlignment();
        m_nalList.serialize(NAL_UNIT_ACCESS_UNIT_DELIMITER, m_bs);
    }
    if (m_frame->m_lowres.bKeyframe && m_param->bRepeatHeaders)
        m_top->getStreamHeaders(m_nalList, m_entropyCoder, m_bs);

    // Weighted Prediction parameters estimation.
    bool bUseWeightP = slice->m_sliceType == P_SLICE && slice->m_pps->bUseWeightPred;
    bool bUseWeightB = slice->m_sliceType == B_SLICE && slice->m_pps->bUseWeightedBiPred;
    if (bUseWeightP || bUseWeightB)
        weightAnalyse(*slice, *m_param);
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
            m_mref[l][ref].init(slice->m_refPicList[l][ref]->getPicYuvRec(), w);
        }
    }

    uint32_t numSubstreams = m_param->bEnableWavefront ? m_frame->getPicSym()->getFrameHeightInCU() : 1;
    if (!m_outStreams)
    {
        m_outStreams = new Bitstream[numSubstreams];
        m_substreamSizes = X265_MALLOC(uint32_t, numSubstreams);
        for (uint32_t i = 0; i < numSubstreams; i++)
            m_rows[i].m_rowEntropyCoder.setBitstream(&m_outStreams[i]);
    }
    else
        for (uint32_t i = 0; i < numSubstreams; i++)
            m_outStreams[i].resetBits();

    /* Get the QP for this frame from rate control. This call may block until
     * frames ahead of it in encode order have called rateControlEnd() */
    int qp = m_top->m_rateControl->rateControlStart(m_frame, &m_rce, m_top);
    m_rce.newQp = qp;

    int qpCb = Clip3(0, MAX_MAX_QP, qp + slice->m_pps->chromaCbQpOffset);
    m_frameFilter.m_sao.lumaLambda = x265_lambda2_tab[qp];
    m_frameFilter.m_sao.chromaLambda = x265_lambda2_tab[qpCb]; // Use Cb QP for SAO chroma
    switch (slice->m_sliceType)
    {
    case I_SLICE:
        m_frameFilter.m_sao.depth = 0;
        break;
    case P_SLICE:
        m_frameFilter.m_sao.depth = 1;
        break;
    case B_SLICE:
        m_frameFilter.m_sao.depth = 2 + !IS_REFERENCED(slice);
        break;
    }
    m_frameFilter.start(m_frame);

    // Clip slice QP to 0-51 spec range before encoding
    qp = Clip3(-QP_BD_OFFSET, MAX_QP, qp);
    slice->m_sliceQp = qp;

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

        // The recovery point SEI message assists a decoder in determining when the decoding
        // process will produce acceptable pictures for display after the decoder initiates
        // random access. The m_recoveryPocCnt is in units of POC(picture order count) which
        // means pictures encoded after the CRA but precede it in display order(leading) are
        // implicitly discarded after a random access seek regardless of the value of
        // m_recoveryPocCnt. Our encoder does not use references prior to the most recent CRA,
        // so all pictures following the CRA in POC order are guaranteed to be displayable,
        // so m_recoveryPocCnt is always 0.
        SEIRecoveryPoint sei_recovery_point;
        sei_recovery_point.m_recoveryPocCnt = 0;
        sei_recovery_point.m_exactMatchingFlag = true;
        sei_recovery_point.m_brokenLinkFlag = false;

        m_bs.resetBits();
        sei_recovery_point.write(m_bs, *slice->m_sps);
        m_bs.writeByteAlignment();

        m_nalList.serialize(NAL_UNIT_PREFIX_SEI, m_bs);
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
            sei->m_auCpbRemovalDelay = X265_MIN(X265_MAX(1, m_rce.encodeOrder - m_top->m_lastBPSEI), (1 << hrd->cpbRemovalDelayLength));
            sei->m_picDpbOutputDelay = slice->m_sps->numReorderPics + poc - m_rce.encodeOrder;
        }

        m_bs.resetBits();
        sei->write(m_bs, *slice->m_sps);
        m_bs.writeByteAlignment();
        m_nalList.serialize(NAL_UNIT_PREFIX_SEI, m_bs);
    }

    // Analyze CTU rows, most of the hard work is done here
    // frame is compressed in a wave-front pattern if WPP is enabled. Loop filter runs as a
    // wave-front behind the CU compression and reconstruction
    compressCTURows();

    if (m_param->rc.bStatWrite)
    {
        // accumulate intra,inter,skip cu count per frame for 2 pass
        for (int i = 0; i < m_numRows; i++)
        {
            m_frameStats.cuCount_i += m_rows[i].m_iCuCnt;
            m_frameStats.cuCount_p += m_rows[i].m_pCuCnt;
            m_frameStats.cuCount_skip += m_rows[i].m_skipCuCnt;
        }
        double totalCuCount = m_frameStats.cuCount_i + m_frameStats.cuCount_p + m_frameStats.cuCount_skip;
        m_frameStats.cuCount_i /= totalCuCount;
        m_frameStats.cuCount_p /= totalCuCount;
        m_frameStats.cuCount_skip /= totalCuCount;
    }
    if (slice->m_sps->bUseSAO && !m_param->saoLcuBasedOptimization)
    {
        /* frame based SAO */
        m_frameFilter.m_sao.SAOProcess(m_frame->getPicSym()->getSaoParam());
        m_frameFilter.m_sao.endSaoEnc();
        restoreLFDisabledOrigYuv(m_frame);

        // Extend border after whole-frame SAO is finished
        for (int row = 0; row < m_numRows; row++)
            m_frameFilter.processRowPost(row);
    }

    m_bs.resetBits();
    m_entropyCoder.resetEntropy(slice);
    m_entropyCoder.setBitstream(&m_bs);
    m_entropyCoder.codeSliceHeader(slice);

    // re-encode each row of CUs for the final time (TODO: get rid of this second pass)
    encodeSlice();

    // serialize each row, record final lengths in slice header
    uint32_t maxStreamSize = m_nalList.serializeSubstreams(m_substreamSizes, numSubstreams, m_outStreams);

    // complete the slice header by writing WPP row-starts
    m_entropyCoder.setBitstream(&m_bs);
    if (slice->m_pps->bEntropyCodingSyncEnabled)
        m_entropyCoder.codeSliceHeaderWPPEntryPoints(slice, m_substreamSizes, maxStreamSize);
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

    m_elapsedCompressTime = (double)(x265_mdate() - startCompressTime) / 1000000;
    /* rateControlEnd may also block for earlier frames to call rateControlUpdateStats */
    if (m_top->m_rateControl->rateControlEnd(m_frame, m_accessUnitBits, &m_rce, &m_frameStats) < 0)
        m_top->m_aborted = true;

    noiseReductionUpdate();

    // Decrement referenced frame reference counts, allow them to be recycled
    for (int l = 0; l < numPredDir; l++)
    {
        for (int ref = 0; ref < slice->m_numRefIdx[l]; ref++)
        {
            Frame *refpic = slice->m_refPicList[l][ref];
            ATOMIC_DEC(&refpic->m_countRefEncoders);
        }
    }
}

void FrameEncoder::encodeSlice()
{
    Slice* slice = m_frame->m_picSym->m_slice;
    const uint32_t widthInLCUs = m_frame->getPicSym()->getFrameWidthInCU();
    const uint32_t lastCUAddr = (slice->m_endCUAddr + m_frame->getNumPartInCU() - 1) / m_frame->getNumPartInCU();
    const int numSubstreams = m_param->bEnableWavefront ? m_frame->getPicSym()->getFrameHeightInCU() : 1;
    SAOParam *saoParam = slice->m_pic->getPicSym()->getSaoParam();

    for (uint32_t cuAddr = 0; cuAddr < lastCUAddr; cuAddr++)
    {
        uint32_t col = cuAddr % widthInLCUs;
        uint32_t lin = cuAddr / widthInLCUs;
        uint32_t subStrm = lin % numSubstreams;
        TComDataCU* cu = m_frame->getCU(cuAddr);

        m_entropyCoder.setBitstream(&m_outStreams[subStrm]);

        // Synchronize cabac probabilities with upper-right LCU if it's available and we're at the start of a line.
        if (m_param->bEnableWavefront && !col && lin)
            m_rows[subStrm].m_rowEntropyCoder.loadContexts(m_rows[lin - 1].m_bufferEntropyCoder);

        // this load is used to simplify the code (avoid to change all the call to m_entropyCoder)
        m_entropyCoder.load(m_rows[subStrm].m_rowEntropyCoder);

        if (slice->m_sps->bUseSAO)
        {
            if (saoParam->bSaoFlag[0] || saoParam->bSaoFlag[1])
            {
                int mergeLeft = saoParam->saoLcuParam[0][cuAddr].mergeLeftFlag && col;
                int mergeUp = saoParam->saoLcuParam[0][cuAddr].mergeUpFlag && lin;
                if (col)
                    m_entropyCoder.codeSaoMerge(mergeLeft);
                if (lin && !mergeLeft)
                    m_entropyCoder.codeSaoMerge(mergeUp);
                if (!mergeLeft && !mergeUp)
                {
                    if (saoParam->bSaoFlag[0])
                        m_entropyCoder.codeSaoOffset(&saoParam->saoLcuParam[0][cuAddr], 0);
                    if (saoParam->bSaoFlag[1])
                    {
                        m_entropyCoder.codeSaoOffset(&saoParam->saoLcuParam[1][cuAddr], 1);
                        m_entropyCoder.codeSaoOffset(&saoParam->saoLcuParam[2][cuAddr], 2);
                    }
                }
            }
            else
            {
                for (int i = 0; i < 3; i++)
                    saoParam->saoLcuParam[i][cuAddr].reset();
            }
        }

        // final coding (bitstream generation) for this CU
        m_tld.m_cuCoder.m_entropyCoder = &m_entropyCoder;
        m_tld.m_cuCoder.encodeCU(cu);

        // load back status of the entropy coder after encoding the LCU into relevant bitstream entropy coder
        m_rows[subStrm].m_rowEntropyCoder.load(m_entropyCoder);

        // Store probabilities of second LCU in line into buffer
        if (col == 1 && m_param->bEnableWavefront)
            m_rows[lin].m_bufferEntropyCoder.loadContexts(m_rows[subStrm].m_rowEntropyCoder);

        // Collect Frame Stats for 2 pass
        m_frameStats.mvBits += cu->m_mvBits;
        m_frameStats.coeffBits += cu->m_coeffBits;
        m_frameStats.miscBits += cu->m_totalBits - (cu->m_mvBits + cu->m_coeffBits);
    }

    // flush lines
    for (int i = 0; i < numSubstreams; i++)
    {
        m_rows[i].m_rowEntropyCoder.codeTerminatingBit(1);
        m_rows[i].m_rowEntropyCoder.codeSliceFinish();
        m_outStreams[i].writeByteAlignment();
    }
}

void FrameEncoder::compressCTURows()
{
    PPAScopeEvent(FrameEncoder_compressRows);
    Slice* slice = m_frame->m_picSym->m_slice;

    // reset entropy coders
    m_entropyCoder.resetEntropy(slice);
    for (int i = 0; i < m_numRows; i++)
    {
        m_rows[i].init(slice);
        m_rows[i].m_rdEntropyCoders[0][CI_CURR_BEST].load(m_entropyCoder);
        m_rows[i].m_rowEntropyCoder.load(m_entropyCoder);
        m_rows[i].m_completed = 0;
        m_rows[i].m_busy = false;
    }

    m_bAllRowsStop = false;
    m_vbvResetTriggerRow = -1;

    m_SSDY = m_SSDU = m_SSDV = 0;
    m_ssim = 0;
    m_ssimCnt = 0;
    memset(&m_frameStats, 0, sizeof(m_frameStats));

    bool bUseWeightP = slice->m_pps->bUseWeightPred && slice->m_sliceType == P_SLICE;
    bool bUseWeightB = slice->m_pps->bUseWeightedBiPred && slice->m_sliceType == B_SLICE;
    int numPredDir = slice->isInterP() ? 1 : slice->isInterB() ? 2 : 0;

    m_rows[0].m_active = true;
    if (m_pool && m_param->bEnableWavefront)
    {
        WaveFront::clearEnabledRowMask();
        WaveFront::enqueue();

        for (int row = 0; row < m_numRows; row++)
        {
            // block until all reference frames have reconstructed the rows we need
            for (int l = 0; l < numPredDir; l++)
            {
                for (int ref = 0; ref < slice->m_numRefIdx[l]; ref++)
                {
                    Frame *refpic = slice->m_refPicList[l][ref];

                    int reconRowCount = refpic->m_reconRowCount.get();
                    while ((reconRowCount != m_numRows) && (reconRowCount < row + m_refLagRows))
                        reconRowCount = refpic->m_reconRowCount.waitForChange(reconRowCount);

                    if ((bUseWeightP || bUseWeightB) && m_mref[l][ref].isWeighted)
                        m_mref[l][ref].applyWeight(row + m_refLagRows, m_numRows);
                }
            }

            enableRowEncoder(row);
            if (row == 0)
                enqueueRowEncoder(0);
            else
                m_pool->pokeIdleThread();
        }

        m_completionEvent.wait();

        WaveFront::dequeue();
    }
    else
    {
        for (int i = 0; i < this->m_numRows + m_filterRowDelay; i++)
        {
            // Encode
            if (i < m_numRows)
            {
                // block until all reference frames have reconstructed the rows we need
                for (int l = 0; l < numPredDir; l++)
                {
                    int list = l;
                    for (int ref = 0; ref < slice->m_numRefIdx[list]; ref++)
                    {
                        Frame *refpic = slice->m_refPicList[list][ref];

                        int reconRowCount = refpic->m_reconRowCount.get();
                        while ((reconRowCount != m_numRows) && (reconRowCount < i + m_refLagRows))
                            reconRowCount = refpic->m_reconRowCount.waitForChange(reconRowCount);

                        if ((bUseWeightP || bUseWeightB) && m_mref[l][ref].isWeighted)
                            m_mref[list][ref].applyWeight(i + m_refLagRows, m_numRows);
                    }
                }

                processRow(i * 2 + 0, -1);
            }

            // Filter
            if (i >= m_filterRowDelay)
                processRow((i - m_filterRowDelay) * 2 + 1, -1);
        }
    }
    m_frameTime = (double)m_totalTime / 1000000;
    m_totalTime = 0;
}

void FrameEncoder::processRow(int row, int threadId)
{
    const int realRow = row >> 1;
    const int typeNum = row & 1;

    ThreadLocalData& tld = threadId >= 0 ? m_top->m_threadLocalData[threadId] : m_tld;

    if (!typeNum)
        processRowEncoder(realRow, tld);
    else
    {
        processRowFilter(realRow, tld);

        // NOTE: Active next row
        if (realRow != m_numRows - 1)
            enqueueRowFilter(realRow + 1);
        else
            m_completionEvent.trigger();
    }
}

// Called by worker threads
void FrameEncoder::processRowEncoder(int row, ThreadLocalData& tld)
{
    PPAScopeEvent(Thread_ProcessRow);

    CTURow& codeRow = m_rows[m_param->bEnableWavefront ? row : 0];
    CTURow& curRow  = m_rows[row];
    {
        ScopedLock self(curRow.m_lock);
        if (!curRow.m_active)
            /* VBV restart is in progress, exit out */
            return;
        if (curRow.m_busy)
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
        curRow.m_busy = true;
    }

    // setup thread-local data
    TComPicYuv* fenc = m_frame->getPicYuvOrg();
    tld.m_cuCoder.m_quant.m_nr = &m_nr;
    tld.m_cuCoder.m_me.setSourcePlane(fenc->getLumaAddr(), fenc->getStride());
    tld.m_cuCoder.m_log = &tld.m_cuCoder.m_sliceTypeLog[m_frame->m_picSym->m_slice->m_sliceType];
    setLambda(m_frame->m_picSym->m_slice->m_sliceQp, tld);

    int64_t startTime = x265_mdate();
    assert(m_frame->getPicSym()->getFrameWidthInCU() == m_numCols);
    const uint32_t numCols = m_numCols;
    const uint32_t lineStartCUAddr = row * numCols;
    bool bIsVbv = m_param->rc.vbvBufferSize > 0 && m_param->rc.vbvMaxBitrate > 0;

    while (curRow.m_completed < numCols)
    {
        int col = curRow.m_completed;
        const uint32_t cuAddr = lineStartCUAddr + col;
        TComDataCU* cu = m_frame->getCU(cuAddr);
        cu->initCU(m_frame, cuAddr);
        cu->setQPSubParts(m_frame->m_picSym->m_slice->m_sliceQp, 0, 0);

        if (bIsVbv)
        {
            if (!row)
            {
                m_frame->m_rowDiagQp[row] = m_frame->m_avgQpRc;
                m_frame->m_rowDiagQScale[row] = x265_qp2qScale(m_frame->m_avgQpRc);
            }

            if (row >= col && row && m_vbvResetTriggerRow != row)
                cu->m_baseQp = m_frame->getCU(cuAddr - numCols + 1)->m_baseQp;
            else
                cu->m_baseQp = m_frame->m_rowDiagQp[row];
        }
        else
            cu->m_baseQp = m_frame->m_avgQpRc;

        if (m_param->rc.aqMode || bIsVbv)
        {
            int qp = calcQpForCu(cuAddr, cu->m_baseQp);
            qp = Clip3(-QP_BD_OFFSET, MAX_QP, qp);
            setLambda(qp, tld);
            cu->setQPSubParts(char(qp), 0, 0);
            if (m_param->rc.aqMode)
                m_frame->m_qpaAq[row] += qp;
        }

        Entropy *bufSbac = (m_param->bEnableWavefront && col == 0 && row > 0) ? &m_rows[row - 1].m_bufferEntropyCoder : NULL;
        codeRow.processCU(cu, bufSbac, tld, m_param->bEnableWavefront && col == 1);
        // Completed CU processing
        curRow.m_completed++;

        // copy no. of intra, inter Cu cnt per row into frame stats for 2 pass
        if (m_param->rc.bStatWrite)
        {
            double scale = (double)(1 << (g_maxCUSize / 16));
            for (uint32_t part = 0; part < g_maxCUDepth ; part++, scale /= 4)
            {
                curRow.m_iCuCnt += scale * tld.m_cuCoder.m_log->qTreeIntraCnt[part];
                curRow.m_pCuCnt += scale * tld.m_cuCoder.m_log->qTreeInterCnt[part];
                curRow.m_skipCuCnt += scale * tld.m_cuCoder.m_log->qTreeSkipCnt[part];

                //clear the row cu data from thread local object
                tld.m_cuCoder.m_log->qTreeIntraCnt[part] = tld.m_cuCoder.m_log->qTreeInterCnt[part] = tld.m_cuCoder.m_log->qTreeSkipCnt[part] = 0;
            }
        }

        if (bIsVbv)
        {
            // Update encoded bits, satdCost, baseQP for each CU
            m_frame->m_rowDiagSatd[row] += m_frame->m_cuCostsForVbv[cuAddr];
            m_frame->m_rowDiagIntraSatd[row] += m_frame->m_intraCuCostsForVbv[cuAddr];
            m_frame->m_rowEncodedBits[row] += cu->m_totalBits;
            m_frame->m_numEncodedCusPerRow[row] = cuAddr;
            m_frame->m_qpaRc[row] += cu->m_baseQp;

            // If current block is at row diagonal checkpoint, call vbv ratecontrol.

            if (row == col && row)
            {
                double qpBase = cu->m_baseQp;
                int reEncode = m_top->m_rateControl->rowDiagonalVbvRateControl(m_frame, row, &m_rce, qpBase);
                qpBase = Clip3((double)MIN_QP, (double)MAX_MAX_QP, qpBase);
                m_frame->m_rowDiagQp[row] = qpBase;
                m_frame->m_rowDiagQScale[row] =  x265_qp2qScale(qpBase);

                if (reEncode < 0)
                {
                    x265_log(m_param, X265_LOG_DEBUG, "POC %d row %d - encode restart required for VBV, to %.2f from %.2f\n",
                             m_frame->getPOC(), row, qpBase, cu->m_baseQp);

                    // prevent the WaveFront::findJob() method from providing new jobs
                    m_vbvResetTriggerRow = row;
                    m_bAllRowsStop = true;

                    for (int r = m_numRows - 1; r >= row; r--)
                    {
                        CTURow& stopRow = m_rows[r];

                        if (r != row)
                        {
                            /* if row was active (ready to be run) clear active bit and bitmap bit for this row */
                            stopRow.m_lock.acquire();
                            while (stopRow.m_active)
                            {
                                if (dequeueRow(r * 2))
                                    stopRow.m_active = false;
                                else
                                    GIVE_UP_TIME();
                            }

                            stopRow.m_lock.release();

                            bool bRowBusy = true;
                            do
                            {
                                stopRow.m_lock.acquire();
                                bRowBusy = stopRow.m_busy;
                                stopRow.m_lock.release();

                                if (bRowBusy)
                                {
                                    GIVE_UP_TIME();
                                }
                            }
                            while (bRowBusy);
                        }

                        stopRow.m_completed = 0;
                        stopRow.m_iCuCnt = stopRow.m_pCuCnt = stopRow.m_skipCuCnt = 0;
                        if (m_frame->m_qpaAq)
                            m_frame->m_qpaAq[r] = 0;
                        m_frame->m_qpaRc[r] = 0;
                        m_frame->m_rowEncodedBits[r] = 0;
                        m_frame->m_numEncodedCusPerRow[r] = 0;
                        m_frame->m_rowDiagSatd[r] = 0;
                        m_frame->m_rowDiagIntraSatd[r] = 0;
                    }

                    m_bAllRowsStop = false;
                }
            }
        }

        // NOTE: do CU level Filter
        if (m_param->bEnableSAO && m_param->saoLcuBasedOptimization && m_param->saoLcuBoundary)
            // SAO parameter estimation using non-deblocked pixels for LCU bottom and right boundary areas
            m_frameFilter.m_sao.calcSaoStatsCu_BeforeDblk(m_frame, col, row);

        // NOTE: active next row
        if (curRow.m_completed >= 2 && row < m_numRows - 1)
        {
            ScopedLock below(m_rows[row + 1].m_lock);
            if (m_rows[row + 1].m_active == false &&
                m_rows[row + 1].m_completed + 2 <= curRow.m_completed &&
                (!m_bAllRowsStop || row + 1 < m_vbvResetTriggerRow))
            {
                m_rows[row + 1].m_active = true;
                enqueueRowEncoder(row + 1);
            }
        }

        ScopedLock self(curRow.m_lock);
        if ((m_bAllRowsStop && row > m_vbvResetTriggerRow) ||
            (row > 0 && curRow.m_completed < numCols - 1 && m_rows[row - 1].m_completed < m_rows[row].m_completed + 2))
        {
            curRow.m_active = false;
            curRow.m_busy = false;
            m_totalTime += x265_mdate() - startTime;
            return;
        }
    }

    /* *this row of CTUs has been encoded* */

    /* If encoding with ABR, update update bits and complexity in rate control
     * after a number of rows so the next frame's rateControlStart has more
     * accurate data for estimation. At the start of the encode we update stats
     * after half the frame is encoded, but after this initial period we update
     * after refLagRows (the number of rows reference frames must have completed
     * before referencees may begin encoding) */
    int rowCount = 0;
    if (m_param->rc.rateControlMode == X265_RC_ABR)
    {
        if ((uint32_t)m_rce.encodeOrder <= 2 * (m_param->fpsNum / m_param->fpsDenom))
            rowCount = m_numRows/2;
        else
            rowCount = m_refLagRows;
    }
    if (row == rowCount)
    {
        int64_t bits = 0;
        for (uint32_t col = 0; col < rowCount * numCols; col++)
        {
            TComDataCU* cu = m_frame->getCU(col);
            bits += cu->m_totalBits;
        }

        m_rce.rowTotalBits = bits;
        m_top->m_rateControl->rateControlUpdateStats(&m_rce);
    }

    // trigger row-wise loop filters
    if (row >= m_filterRowDelay)
    {
        enableRowFilter(row - m_filterRowDelay);

        // NOTE: Active Filter to first row (row 0)
        if (row == m_filterRowDelay)
            enqueueRowFilter(0);
    }
    if (row == m_numRows - 1)
    {
        for (int i = m_numRows - m_filterRowDelay; i < m_numRows; i++)
            enableRowFilter(i);
    }

    m_totalTime += x265_mdate() - startTime;
    curRow.m_busy = false;
}

void FrameEncoder::setLambda(int qp, ThreadLocalData &tld)
{
    Slice* slice = m_frame->m_picSym->m_slice;
  
    int qpCb = Clip3(0, MAX_MAX_QP, qp + slice->m_pps->chromaCbQpOffset);
    int qpCr = Clip3(0, MAX_MAX_QP, qp + slice->m_pps->chromaCrQpOffset);
    
    tld.m_cuCoder.setQP(qp, qpCb, qpCr);
}

/* DCT-domain noise reduction / adaptive deadzone from libavcodec */
void FrameEncoder::noiseReductionUpdate()
{
    if (!m_nr.bNoiseReduction)
        return;

    static const uint32_t maxBlocksPerTrSize[4] = {1 << 18, 1 << 16, 1 << 14, 1 << 12};

    for (int cat = 0; cat < 8; cat++)
    {
        int trSize = cat & 3;
        int coefCount = 1 << ((trSize + 2) * 2);

        if (m_nr.count[cat] > maxBlocksPerTrSize[trSize])
        {
            for (int i = 0; i < coefCount; i++)
                m_nr.residualSum[cat][i] >>= 1;
            m_nr.count[cat] >>= 1;
        }

        uint64_t scaledCount = (uint64_t)m_param->noiseReduction * m_nr.count[cat];

        for (int i = 0; i < coefCount; i++)
        {
            uint64_t value = scaledCount + m_nr.residualSum[cat][i] / 2;
            uint64_t denom = m_nr.residualSum[cat][i] + 1;
            m_nr.offsetDenoise[cat][i] = (uint16_t)(value / denom);
        }

        // Don't denoise DC coefficients
        m_nr.offsetDenoise[cat][0] = 0;
    }
}

int FrameEncoder::calcQpForCu(uint32_t cuAddr, double baseQp)
{
    x265_emms();
    double qp = baseQp;

    /* clear cuCostsForVbv from when vbv row reset was triggered */
    bool bIsVbv = m_param->rc.vbvBufferSize > 0 && m_param->rc.vbvMaxBitrate > 0;
    if (bIsVbv)
    {
        m_frame->m_cuCostsForVbv[cuAddr] = 0;
        m_frame->m_intraCuCostsForVbv[cuAddr] = 0;
    }

    /* Derive qpOffet for each CU by averaging offsets for all 16x16 blocks in the cu. */
    double qp_offset = 0;
    int maxBlockCols = (m_frame->getPicYuvOrg()->getWidth() + (16 - 1)) / 16;
    int maxBlockRows = (m_frame->getPicYuvOrg()->getHeight() + (16 - 1)) / 16;
    int noOfBlocks = g_maxCUSize / 16;
    int block_y = (cuAddr / m_frame->getPicSym()->getFrameWidthInCU()) * noOfBlocks;
    int block_x = (cuAddr * noOfBlocks) - block_y * m_frame->getPicSym()->getFrameWidthInCU();

    /* Use cuTree offsets if cuTree enabled and frame is referenced, else use AQ offsets */
    bool isReferenced = IS_REFERENCED(m_frame->m_picSym->m_slice);
    double *qpoffs = (isReferenced && m_param->rc.cuTree) ? m_frame->m_lowres.qpCuTreeOffset : m_frame->m_lowres.qpAqOffset;

    int cnt = 0, idx = 0;
    for (int h = 0; h < noOfBlocks && block_y < maxBlockRows; h++, block_y++)
    {
        for (int w = 0; w < noOfBlocks && (block_x + w) < maxBlockCols; w++)
        {
            idx = block_x + w + (block_y * maxBlockCols);
            if (m_param->rc.aqMode)
                qp_offset += qpoffs[idx];
            if (bIsVbv)
            {
                m_frame->m_cuCostsForVbv[cuAddr] += m_frame->m_lowres.lowresCostForRc[idx] & LOWRES_COST_MASK;
                m_frame->m_intraCuCostsForVbv[cuAddr] += m_frame->m_lowres.intraCost[idx];
            }
            cnt++;
        }
    }

    qp_offset /= cnt;
    qp += qp_offset;

    return Clip3(MIN_QP, MAX_MAX_QP, (int)(qp + 0.5));
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
        return ret;
    }

    return NULL;
}
}
