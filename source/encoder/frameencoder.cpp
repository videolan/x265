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
void weightAnalyse(TComSlice& slice, x265_param& param);

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
    memset(&m_frameStats, 0, sizeof(m_frameStats));
    memset(&m_rce, 0, sizeof(RateControlEntry));
}

void FrameEncoder::setThreadPool(ThreadPool *p)
{
    m_pool = p;
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
    m_filterRowDelay = (m_param->saoLcuBasedOptimization && m_param->saoLcuBoundary) ?
        2 : (m_param->bEnableSAO || m_param->bEnableLoopFilter ? 1 : 0);
    m_filterRowDelayCus = m_filterRowDelay * numCols;

    m_rows = new CTURow[m_numRows];
    bool ok = !!m_numRows;

    int range = m_param->searchRange; /* fpel search */
    range    += 1;                    /* diamond search range check lag */
    range    += 2;                    /* subpel refine */
    range    += NTAPS_LUMA / 2;       /* subpel filter half-length */
    m_refLagRows = 1 + ((range + g_maxCUSize - 1) / g_maxCUSize);

    // NOTE: 2 times of numRows because both Encoder and Filter in same queue
    if (!WaveFront::init(m_numRows * 2))
    {
        x265_log(m_param, X265_LOG_ERROR, "unable to initialize wavefront queue\n");
        m_pool = NULL;
    }

    m_tld.init(*top);
    m_frameFilter.init(top, this, numRows, &m_rows[0].m_sbacCoder);

    // initialize SPS
    top->initSPS(&m_sps);

    // initialize PPS
    m_pps.setSPS(&m_sps);

    top->initPPS(&m_pps);

    m_sps.setNumLongTermRefPicSPS(0);

    // initialize HRD parameters of SPS
    if (m_param->bEmitHRDSEI)
    {
        m_rce.picTimingSEI = new SEIPictureTiming;
        m_rce.hrdTiming = new HRDTiming;

        ok &= m_rce.picTimingSEI && m_rce.hrdTiming;
        if (ok)
            m_top->m_rateControl->initHRD(&m_sps);
    }


    m_sps.setTMVPFlagsPresent(true);

    // set default slice level flag to the same as SPS level flag
    if (m_top->m_useScalingListId == SCALING_LIST_OFF)
    {
        m_sps.setScalingListPresentFlag(false);
        m_pps.setScalingListPresentFlag(false);
    }
    else if (m_top->m_useScalingListId == SCALING_LIST_DEFAULT)
    {
        m_sps.setScalingListPresentFlag(false);
        m_pps.setScalingListPresentFlag(false);
    }
    else
    {
        x265_log(m_param, X265_LOG_ERROR, "ScalingList == %d not supported\n", m_top->m_useScalingListId);
        ok = false;
    }

    memset(&m_frameStats, 0, sizeof(m_frameStats));
    memset(m_nr.offsetDenoise, 0, sizeof(m_nr.offsetDenoise[0][0]) * 8 * 1024);
    memset(m_nr.residualSumBuf, 0, sizeof(m_nr.residualSumBuf[0][0][0]) * 4 * 8 * 1024);
    memset(m_nr.countBuf, 0, sizeof(m_nr.countBuf[0][0]) * 4 * 8);

    m_nr.offset = m_nr.offsetDenoise;
    m_nr.residualSum = m_nr.residualSumBuf[0];
    m_nr.count = m_nr.countBuf[0];
    m_nr.bNoiseReduction = !!m_param->noiseReduction;

    start();
    return ok;
}

/****************************************************************************
 * DCT-domain noise reduction / adaptive deadzone
 * from libavcodec
 ****************************************************************************/

void FrameEncoder::noiseReductionUpdate()
{
    if (!m_nr.bNoiseReduction)
        return;

    m_nr.offset = m_nr.offsetDenoise;
    m_nr.residualSum = m_nr.residualSumBuf[0];
    m_nr.count = m_nr.countBuf[0];

    int transformSize[4] = {16, 64, 256, 1024};
    uint32_t blockCount[4] = {1 << 18, 1 << 16, 1 << 14, 1 << 12};

    int isCspI444 = (m_param->internalCsp == X265_CSP_I444) ? 1 : 0;
    for (int cat = 0; cat < 7 + isCspI444; cat++)
    {
        int index = cat % 4;
        int size = transformSize[index];

        if (m_nr.count[cat] > blockCount[index])
        {
            for (int i = 0; i < size; i++)
                m_nr.residualSum[cat][i] >>= 1;
            m_nr.count[cat] >>= 1;
        }

        for (int i = 0; i < size; i++)
            m_nr.offset[cat][i] =
                (uint16_t)(((uint64_t)m_param->noiseReduction * m_nr.count[cat]
                 + m_nr.residualSum[cat][i] / 2)
              / ((uint64_t)m_nr.residualSum[cat][i] + 1));

        // Don't denoise DC coefficients
        m_nr.offset[cat][0] = 0;
    }
}

void FrameEncoder::getStreamHeaders(NALList& list, Bitstream& bs)
{
    /* headers for start of bitstream */
    bs.resetBits();
    m_sbacCoder.setBitstream(&bs);
    m_sbacCoder.codeVPS(&m_top->m_vps, &m_top->m_ptl);
    bs.writeByteAlignment();
    list.serialize(NAL_UNIT_VPS, bs);

    bs.resetBits();
    m_sbacCoder.codeSPS(&m_sps, m_top->getScalingList(), &m_top->m_ptl);
    bs.writeByteAlignment();
    list.serialize(NAL_UNIT_SPS, bs);

    bs.resetBits();
    m_sbacCoder.codePPS(&m_pps, m_top->getScalingList());
    bs.writeByteAlignment();
    list.serialize(NAL_UNIT_PPS, bs);

    if (m_param->bEmitInfoSEI)
    {
        char *opts = x265_param2string(m_param);
        if (opts)
        {
            char *buffer = X265_MALLOC(char, strlen(opts) + strlen(x265_version_str) +
                                            strlen(x265_build_info_str) + 200);
            if (buffer)
            {
                sprintf(buffer, "x265 (build %d) - %s:%s - H.265/HEVC codec - "
                        "Copyright 2013-2014 (c) Multicoreware Inc - "
                        "http://x265.org - options: %s",
                        X265_BUILD, x265_version_str, x265_build_info_str, opts);
                
                bs.resetBits();
                SEIuserDataUnregistered idsei;
                idsei.m_userData = (uint8_t*)buffer;
                idsei.m_userDataLength = (uint32_t)strlen(buffer);
                idsei.write(bs, m_sps);
                bs.writeByteAlignment();
                list.serialize(NAL_UNIT_PREFIX_SEI, bs);

                X265_FREE(buffer);
            }

            X265_FREE(opts);
        }
    }

    if (m_param->bEmitHRDSEI)
    {
        SEIActiveParameterSets sei;
        sei.m_activeVPSId = m_top->m_vps.getVPSId();
        sei.m_fullRandomAccessFlag = false;
        sei.m_noParamSetUpdateFlag = false;
        sei.m_numSpsIdsMinus1 = 0;
        sei.m_activeSeqParamSetId = m_sps.getSPSId();

        bs.resetBits();
        sei.write(bs, m_sps);
        list.serialize(NAL_UNIT_PREFIX_SEI, bs);
    }
}

void FrameEncoder::initSlice(Frame* pic)
{
    m_frame = pic;
    TComSlice* slice = pic->getSlice();
    slice->setSPS(&m_sps);
    slice->setPPS(&m_pps);
    slice->setSliceBits(0);
    slice->setPic(pic);
    slice->initSlice();
    slice->setPicOutputFlag(true);
    int type = pic->m_lowres.sliceType;
    SliceType sliceType = IS_X265_TYPE_B(type) ? B_SLICE : ((type == X265_TYPE_P) ? P_SLICE : I_SLICE);
    slice->setSliceType(sliceType);

    if (sliceType != B_SLICE)
        m_isReferenced = true;
    else
    {
        if (pic->m_lowres.sliceType == X265_TYPE_BREF)
            m_isReferenced = true;
        else
            m_isReferenced = false;
    }
    slice->setReferenced(m_isReferenced);

    slice->setScalingList(m_top->getScalingList());
    slice->getScalingList()->setUseTransformSkip(m_pps.getUseTransformSkip());

    if (slice->getPPS()->getDeblockingFilterControlPresentFlag())
    {
        slice->getPPS()->setDeblockingFilterOverrideEnabledFlag(!m_top->m_loopFilterOffsetInPPS);
        slice->setDeblockingFilterOverrideFlag(!m_top->m_loopFilterOffsetInPPS);
        slice->getPPS()->setPicDisableDeblockingFilterFlag(!m_param->bEnableLoopFilter);
        slice->setDeblockingFilterDisable(!m_param->bEnableLoopFilter);
        if (!slice->getDeblockingFilterDisable())
        {
            slice->getPPS()->setDeblockingFilterBetaOffsetDiv2(m_top->m_loopFilterBetaOffset);
            slice->getPPS()->setDeblockingFilterTcOffsetDiv2(m_top->m_loopFilterTcOffset);
            slice->setDeblockingFilterBetaOffsetDiv2(m_top->m_loopFilterBetaOffset);
            slice->setDeblockingFilterTcOffsetDiv2(m_top->m_loopFilterTcOffset);
        }
    }
    else
    {
        slice->setDeblockingFilterOverrideFlag(false);
        slice->setDeblockingFilterDisable(false);
        slice->setDeblockingFilterBetaOffsetDiv2(0);
        slice->setDeblockingFilterTcOffsetDiv2(0);
    }

    slice->setMaxNumMergeCand(m_param->maxNumMergeCand);
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

void FrameEncoder::setLambda(int qp, ThreadLocalData &tld)
{
    TComSlice*  slice = m_frame->getSlice();
  
    int chromaQPOffset = slice->getPPS()->getChromaCbQpOffset() + slice->getSliceQpDeltaCb();
    int qpCb = Clip3(0, MAX_MAX_QP, qp + chromaQPOffset);
    
    chromaQPOffset = slice->getPPS()->getChromaCrQpOffset() + slice->getSliceQpDeltaCr();
    int qpCr = Clip3(0, MAX_MAX_QP, qp + chromaQPOffset);
    
    tld.m_cuCoder.setQP(qp, qpCb, qpCr);
}

void FrameEncoder::compressFrame()
{
    PPAScopeEvent(FrameEncoder_compressFrame);
    int64_t      startCompressTime = x265_mdate();
    TComSlice*   slice             = m_frame->getSlice();
    int          totalCoded        = m_rce.encodeOrder;

    /* Emit access unit delimiter unless this is the first frame and the user is
     * not repeating headers (since AUD is supposed to be the first NAL in the access
     * unit) */
    if (m_param->bEnableAccessUnitDelimiters && (m_frame->getPOC() || m_param->bRepeatHeaders))
    {
        m_bs.resetBits();
        m_sbacCoder.setBitstream(&m_bs);
        m_sbacCoder.codeAUD(slice);
        m_bs.writeByteAlignment();
        m_nalList.serialize(NAL_UNIT_ACCESS_UNIT_DELIMITER, m_bs);
    }
    if (m_frame->m_lowres.bKeyframe)
    {
        if (m_param->bRepeatHeaders)
            getStreamHeaders(m_nalList, m_bs);

        if (m_param->bEmitHRDSEI)
        {
            SEIBufferingPeriod* bpSei = &m_top->m_rateControl->m_bufPeriodSEI;
            bpSei->m_bpSeqParameterSetId = m_sps.getSPSId();
            bpSei->m_rapCpbParamsPresentFlag = 0;

            // for the concatenation, it can be set to one during splicing.
            bpSei->m_concatenationFlag = 0;

            // since the temporal layer HRD is not ready, we assumed it is fixed
            bpSei->m_auCpbRemovalDelayDelta = 1;
            bpSei->m_cpbDelayOffset = 0;
            bpSei->m_dpbDelayOffset = 0;

            // hrdFullness() calculates the initial CPB removal delay and offset
            m_top->m_rateControl->hrdFullness(bpSei);

            m_bs.resetBits();
            bpSei->write(m_bs, m_sps);

            m_nalList.serialize(NAL_UNIT_PREFIX_SEI, m_bs);

            m_top->m_lastBPSEI = totalCoded;
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
        sei_recovery_point.write(m_bs, *slice->getSPS());
        m_bs.writeByteAlignment();

        m_nalList.serialize(NAL_UNIT_PREFIX_SEI, m_bs);
    }

    if (m_param->bEmitHRDSEI || !!m_param->interlaceMode)
    {
        SEIPictureTiming *sei = m_rce.picTimingSEI;
        TComVUI *vui = slice->getSPS()->getVuiParameters();
        TComHRD *hrd = vui->getHrdParameters();
        int poc = slice->getPOC();

        if (vui->getFrameFieldInfoPresentFlag())
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

        if (hrd->getCpbDpbDelaysPresentFlag())
        {
            // The m_aucpbremoval delay specifies how many clock ticks the
            // access unit associated with the picture timing SEI message has to
            // wait after removal of the access unit with the most recent
            // buffering period SEI message
            int cpbDelayLength = hrd->getCpbRemovalDelayLengthMinus1() + 1;
            sei->m_auCpbRemovalDelay = X265_MIN(X265_MAX(1, totalCoded - m_top->m_lastBPSEI), (1 << cpbDelayLength));
            sei->m_picDpbOutputDelay = slice->getSPS()->getNumReorderPics() + poc - totalCoded;
        }

        m_bs.resetBits();
        sei->write(m_bs, m_sps);
        m_nalList.serialize(NAL_UNIT_PREFIX_SEI, m_bs);
    }

    int qp = slice->getSliceQp();

    int chromaQPOffset = slice->getPPS()->getChromaCbQpOffset() + slice->getSliceQpDeltaCb();
    int qpCb = Clip3(0, MAX_MAX_QP, qp + chromaQPOffset);
    
    double lambda = x265_lambda2_tab[qp];
    /* Assuming qpCb and qpCr are the same, since SAO takes only a single chroma lambda. TODO: Check why */
    double chromaLambda = x265_lambda2_tab[qpCb];

    // NOTE: set SAO lambda every Frame
    m_frameFilter.m_sao.lumaLambda = lambda;
    m_frameFilter.m_sao.chromaLambda = chromaLambda;

    // Clip qps back to 0-51 range before encoding
    qp = Clip3(-QP_BD_OFFSET, MAX_QP, qp);
    slice->setSliceQp(qp);
    m_frame->m_avgQpAq = qp;
    slice->setSliceQpDelta(0);
    slice->setSliceQpDeltaCb(0);
    slice->setSliceQpDeltaCr(0);

    switch (slice->getSliceType())
    {
    case I_SLICE:
        m_frameFilter.m_sao.depth = 0;
        break;
    case P_SLICE:
        m_frameFilter.m_sao.depth = 1;
        break;
    case B_SLICE:
        m_frameFilter.m_sao.depth = 2 + !m_isReferenced;
        break;
    }

    slice->setSliceSegmentBits(0);
    slice->setSliceCurEndCUAddr(m_frame->getNumCUsInFrame() * m_frame->getNumPartInCU());

    // Weighted Prediction parameters estimation.
    bool bUseWeightP = slice->getSliceType() == P_SLICE && slice->getPPS()->getUseWP();
    bool bUseWeightB = slice->getSliceType() == B_SLICE && slice->getPPS()->getWPBiPred();
    if (bUseWeightP || bUseWeightB)
        weightAnalyse(*slice, *m_param);
    else
        slice->resetWpScaling();

    // Generate motion references
    int numPredDir = slice->isInterP() ? 1 : slice->isInterB() ? 2 : 0;
    for (int l = 0; l < numPredDir; l++)
    {
        for (int ref = 0; ref < slice->getNumRefIdx(l); ref++)
        {
            wpScalingParam *w = NULL;
            if ((bUseWeightP || bUseWeightB) && slice->m_weightPredTable[l][ref][0].bPresentFlag)
                w = slice->m_weightPredTable[l][ref];
            m_mref[l][ref].init(slice->getRefPic(l, ref)->getPicYuvRec(), w);
        }
    }

    m_bAllRowsStop = false;
    m_vbvResetTriggerRow = -1;

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
    if (m_sps.getUseSAO())
    {
        SAOParam* saoParam = m_frame->getPicSym()->getSaoParam();

        if (!getSAO()->getSaoLcuBasedOptimization())
        {
            /* frame based SAO */
            getSAO()->SAOProcess(saoParam);
            getSAO()->endSaoEnc();
            restoreLFDisabledOrigYuv(m_frame);

            // Extend border after whole-frame SAO is finished
            for (int row = 0; row < m_numRows; row++)
                m_frameFilter.processRowPost(row);
        }

        slice->setSaoEnabledFlag((saoParam->bSaoFlag[0] == 1) ? true : false);
    }

    /* start slice NALunit */
    uint32_t numSubstreams = m_param->bEnableWavefront ? m_frame->getPicSym()->getFrameHeightInCU() : 1;
    if (!m_outStreams)
        m_outStreams = new Bitstream[numSubstreams];
    else
        for (uint32_t i = 0; i < numSubstreams; i++)
            m_outStreams[i].resetBits();
    slice->allocSubstreamSizes(numSubstreams);

    m_bs.resetBits();
    m_sbacCoder.resetEntropy(slice);
    m_sbacCoder.setBitstream(&m_bs);

    if (slice->getSPS()->getUseSAO())
    {
        SAOParam *saoParam = slice->getPic()->getPicSym()->getSaoParam();
        slice->setSaoEnabledFlag(saoParam->bSaoFlag[0]);
        slice->setSaoEnabledFlagChroma(saoParam->bSaoFlag[1]);
    }
    m_sbacCoder.codeSliceHeader(slice);

    // re-encode each row of CUs for the final time (TODO: get rid of this second pass)
    encodeSlice();

    // serialize each row, record final lengths in slice header
    m_nalList.serializeSubstreams(slice->getSubstreamSizes(), numSubstreams, m_outStreams);

    // complete the slice header by writing WPP row-starts
    m_sbacCoder.setBitstream(&m_bs);
    m_sbacCoder.codeTilesWPPEntryPoint(slice);
    m_bs.writeByteAlignment();

    m_nalList.serialize(slice->getNalUnitType(), m_bs);

    if (m_param->decodedPictureHashSEI)
    {
        if (m_param->decodedPictureHashSEI == 1)
        {
            m_seiReconPictureDigest.m_method = SEIDecodedPictureHash::MD5;
            for (int i = 0; i < 3; i++)
            {
                MD5Final(&m_state[i], m_seiReconPictureDigest.m_digest[i]);
            }
        }
        else if (m_param->decodedPictureHashSEI == 2)
        {
            m_seiReconPictureDigest.m_method = SEIDecodedPictureHash::CRC;
            for (int i = 0; i < 3; i++)
            {
                crcFinish(m_crc[i], m_seiReconPictureDigest.m_digest[i]);
            }
        }
        else if (m_param->decodedPictureHashSEI == 3)
        {
            m_seiReconPictureDigest.m_method = SEIDecodedPictureHash::CHECKSUM;
            for (int i = 0; i < 3; i++)
            {
                checksumFinish(m_checksum[i], m_seiReconPictureDigest.m_digest[i]);
            }
        }

        m_bs.resetBits();
        m_seiReconPictureDigest.write(m_bs, *slice->getSPS());
        m_bs.writeByteAlignment();

        m_nalList.serialize(NAL_UNIT_SUFFIX_SEI, m_bs);
    }

    // Decrement referenced frame reference counts, allow them to be recycled
    for (int l = 0; l < numPredDir; l++)
    {
        for (int ref = 0; ref < slice->getNumRefIdx(l); ref++)
        {
            Frame *refpic = slice->getRefPic(l, ref);
            ATOMIC_DEC(&refpic->m_countRefEncoders);
        }
    }

    noiseReductionUpdate();

    m_elapsedCompressTime = (double)(x265_mdate() - startCompressTime) / 1000000;
}

void FrameEncoder::encodeSlice()
{
    TComSlice* slice = m_frame->getSlice();
    const uint32_t widthInLCUs = m_frame->getPicSym()->getFrameWidthInCU();
    const uint32_t lastCUAddr = (slice->getSliceCurEndCUAddr() + m_frame->getNumPartInCU() - 1) / m_frame->getNumPartInCU();
    const int numSubstreams = m_param->bEnableWavefront ? m_frame->getPicSym()->getFrameHeightInCU() : 1;
    SAOParam *saoParam = slice->getPic()->getPicSym()->getSaoParam();

    for (int i = 0; i < numSubstreams; i++)
    {
        m_rows[i].m_rowEntropyCoder.resetEntropy(slice);
        m_rows[i].m_rowEntropyCoder.setBitstream(&m_outStreams[i]);
    }

    for (uint32_t cuAddr = 0; cuAddr < lastCUAddr; cuAddr++)
    {
        uint32_t col = cuAddr % widthInLCUs;
        uint32_t lin = cuAddr / widthInLCUs;
        uint32_t subStrm = lin % numSubstreams;
        TComDataCU* cu = m_frame->getCU(cuAddr);

        m_sbacCoder.setBitstream(&m_outStreams[subStrm]);

        // Synchronize cabac probabilities with upper-right LCU if it's available and we're at the start of a line.
        if (m_param->bEnableWavefront && !col && lin)
            m_rows[subStrm].m_rowEntropyCoder.loadContexts(m_rows[lin - 1].m_bufferSbacCoder);

        // this load is used to simplify the code (avoid to change all the call to m_sbacCoder)
        m_sbacCoder.load(m_rows[subStrm].m_rowEntropyCoder);

        if (slice->getSPS()->getUseSAO())
        {
            if (saoParam->bSaoFlag[0] || saoParam->bSaoFlag[1])
            {
                int mergeLeft = saoParam->saoLcuParam[0][cuAddr].mergeLeftFlag && col;
                int mergeUp = saoParam->saoLcuParam[0][cuAddr].mergeUpFlag && lin;
                if (col)
                    m_sbacCoder.codeSaoMerge(mergeLeft);
                if (lin && !mergeLeft)
                    m_sbacCoder.codeSaoMerge(mergeUp);
                if (!mergeLeft && !mergeUp)
                {
                    if (saoParam->bSaoFlag[0])
                        m_sbacCoder.codeSaoOffset(&saoParam->saoLcuParam[0][cuAddr], 0);
                    if (saoParam->bSaoFlag[1])
                    {
                        m_sbacCoder.codeSaoOffset(&saoParam->saoLcuParam[1][cuAddr], 1);
                        m_sbacCoder.codeSaoOffset(&saoParam->saoLcuParam[2][cuAddr], 2);
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
        m_tld.m_cuCoder.m_sbacCoder = &m_sbacCoder;
        m_tld.m_cuCoder.encodeCU(cu);

        // load back status of the entropy coder after encoding the LCU into relevant bitstream entropy coder
        m_rows[subStrm].m_rowEntropyCoder.load(m_sbacCoder);

        // Store probabilities of second LCU in line into buffer
        if (col == 1 && m_param->bEnableWavefront)
            m_rows[lin].m_bufferSbacCoder.loadContexts(m_rows[subStrm].m_rowEntropyCoder);

        // Collect Frame Stats for 2 pass
        m_frameStats.mvBits += cu->m_mvBits;
        m_frameStats.coeffBits += cu->m_coeffBits;
        m_frameStats.miscBits += cu->m_totalBits - (cu->m_mvBits + cu->m_coeffBits);
    }

    // when frame parallelism is disabled, we can tweak the initial CABAC state of P and B frames
    if (slice->getPPS()->getCabacInitPresentFlag())
        m_sbacCoder.determineCabacInitIdx(slice);

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
    TComSlice* slice = m_frame->getSlice();

    // reset entropy coders
    m_sbacCoder.resetEntropy(slice);
    for (int i = 0; i < this->m_numRows; i++)
    {
        m_rows[i].init(slice);
        m_rows[i].m_rdSbacCoders[0][CI_CURR_BEST].load(m_sbacCoder);
        m_rows[i].m_rdSbacCoders[0][CI_CURR_BEST].zeroFract();
        m_rows[i].m_completed = 0;
        m_rows[i].m_busy = false;
    }

    bool bUseWeightP = slice->getPPS()->getUseWP() && slice->getSliceType() == P_SLICE;
    bool bUseWeightB = slice->getPPS()->getWPBiPred() && slice->getSliceType() == B_SLICE;
    int numPredDir = slice->isInterP() ? 1 : slice->isInterB() ? 2 : 0;

    m_SSDY = m_SSDU = m_SSDV = 0;
    m_ssim = 0;
    m_ssimCnt = 0;
    m_frameFilter.start(m_frame);
    memset(&m_frameStats, 0, sizeof(m_frameStats));

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
                for (int ref = 0; ref < slice->getNumRefIdx(l); ref++)
                {
                    Frame *refpic = slice->getRefPic(l, ref);

                    int reconRowCount = refpic->m_reconRowCount.get();
                    while ((reconRowCount != m_numRows) && (reconRowCount < row + m_refLagRows))
                    {
                        reconRowCount = refpic->m_reconRowCount.waitForChange(reconRowCount);
                    }

                    if ((bUseWeightP || bUseWeightB) && m_mref[l][ref].isWeighted)
                    {
                        m_mref[l][ref].applyWeight(row + m_refLagRows, m_numRows);
                    }
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
            // Encoder
            if (i < m_numRows)
            {
                // block until all reference frames have reconstructed the rows we need
                for (int l = 0; l < numPredDir; l++)
                {
                    int list = l;
                    for (int ref = 0; ref < slice->getNumRefIdx(list); ref++)
                    {
                        Frame *refpic = slice->getRefPic(list, ref);

                        int reconRowCount = refpic->m_reconRowCount.get();
                        while ((reconRowCount != m_numRows) && (reconRowCount < i + m_refLagRows))
                        {
                            reconRowCount = refpic->m_reconRowCount.waitForChange(reconRowCount);
                        }

                        if ((bUseWeightP || bUseWeightB) && m_mref[l][ref].isWeighted)
                        {
                            m_mref[list][ref].applyWeight(i + m_refLagRows, m_numRows);
                        }
                    }
                }

                processRow(i * 2 + 0, -1);
            }

            // Filter
            if (i >= m_filterRowDelay)
            {
                processRow((i - m_filterRowDelay) * 2 + 1, -1);
            }
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
        {
            /* VBV restart is in progress, exit out */
            return;
        }
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
    tld.m_cuCoder.m_trQuant.m_nr = &m_nr;
    tld.m_cuCoder.m_mref = m_mref;
    tld.m_cuCoder.m_me.setSourcePlane(fenc->getLumaAddr(), fenc->getStride());
    tld.m_cuCoder.m_log = &tld.m_cuCoder.m_sliceTypeLog[m_frame->getSlice()->getSliceType()];
    setLambda(m_frame->getSlice()->getSliceQp(), tld);

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
        cu->setQPSubParts(m_frame->getSlice()->getSliceQp(), 0, 0);

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

        SBac *bufSbac = (m_param->bEnableWavefront && col == 0 && row > 0) ? &m_rows[row - 1].m_bufferSbacCoder : NULL;
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
        {
            // SAO parameter estimation using non-deblocked pixels for LCU bottom and right boundary areas
            m_frameFilter.m_sao.calcSaoStatsCu_BeforeDblk(m_frame, col, row);
        }

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

    // this row of CTUs has been encoded

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
        {
            enableRowFilter(i);
        }
    }

    m_totalTime += x265_mdate() - startTime;
    curRow.m_busy = false;
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
    double *qpoffs = (m_isReferenced && m_param->rc.cuTree) ? m_frame->m_lowres.qpCuTreeOffset : m_frame->m_lowres.qpAqOffset;

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
