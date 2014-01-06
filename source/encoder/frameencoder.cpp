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

#include "TLibEncoder/NALwrite.h"
#include "encoder.h"
#include "frameencoder.h"
#include "cturow.h"
#include "common.h"

#include <math.h>

using namespace x265;

enum SCALING_LIST_PARAMETER
{
    SCALING_LIST_OFF,
    SCALING_LIST_DEFAULT,
};

FrameEncoder::FrameEncoder()
    : WaveFront(NULL)
    , m_threadActive(true)
    , m_rows(NULL)
    , m_top(NULL)
    , m_cfg(NULL)
    , m_pic(NULL)
{
    for (int i = 0; i < MAX_NAL_UNITS; i++)
    {
        m_nalList[i] = NULL;
    }

    m_nalCount = 0;
    m_totalTime = 0;
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

    // flush condition, release queued NALs
    for (int i = 0; i < m_nalCount; i++)
    {
        NALUnitEBSP *nalu = m_nalList[i];
        free(nalu->m_nalUnitData);
        X265_FREE(nalu);
    }

    if (m_rows)
    {
        for (int i = 0; i < m_numRows; ++i)
        {
            m_rows[i].destroy();
        }

        delete[] m_rows;
    }

    m_frameFilter.destroy();
    // wait for worker thread to exit
    stop();
}

void FrameEncoder::init(Encoder *top, int numRows)
{
    m_top = top;
    m_cfg = top;
    m_numRows = numRows;
    m_filterRowDelay = (m_cfg->param.saoLcuBasedOptimization && m_cfg->param.saoLcuBoundary) ?
        2 : (m_cfg->param.bEnableSAO || m_cfg->param.bEnableLoopFilter ? 1 : 0);

    m_rows = new CTURow[m_numRows];
    for (int i = 0; i < m_numRows; ++i)
    {
        m_rows[i].create(top);
        for (int list = 0; list <= 1; list++)
        {
            for (int ref = 0; ref <= MAX_NUM_REF; ref++)
            {
                m_rows[i].m_search.m_mref[list][ref] = &m_mref[list][ref];
            }
        }
    }

    // NOTE: 2 times of numRows because both Encoder and Filter in same queue
    if (!WaveFront::init(m_numRows * 2))
    {
        assert(!"Unable to initialize job queue.");
        m_pool = NULL;
    }

    m_frameFilter.init(top, numRows, getRDGoOnSbacCoder(0));

    // initialize SPS
    top->initSPS(&m_sps);

    // initialize PPS
    m_pps.setSPS(&m_sps);

    top->initPPS(&m_pps);

    m_sps.setNumLongTermRefPicSPS(0);
    if (m_cfg->getPictureTimingSEIEnabled() || m_cfg->getDecodingUnitInfoSEIEnabled())
    {
        m_sps.setHrdParameters(m_cfg->param.frameRate, 0, m_cfg->param.rc.bitrate, m_cfg->param.bframes > 0);
    }
    if (m_cfg->getBufferingPeriodSEIEnabled() || m_cfg->getPictureTimingSEIEnabled() || m_cfg->getDecodingUnitInfoSEIEnabled())
    {
        m_sps.getVuiParameters()->setHrdParametersPresentFlag(true);
    }

    m_sps.setTMVPFlagsPresent(true);

    // set default slice level flag to the same as SPS level flag
    if (m_cfg->getUseScalingListId() == SCALING_LIST_OFF)
    {
        for (int i = 0; i < m_numRows; i++)
        {
            m_rows[i].m_trQuant.setFlatScalingList();
            m_rows[i].m_trQuant.setUseScalingList(false);
        }

        m_sps.setScalingListPresentFlag(false);
        m_pps.setScalingListPresentFlag(false);
    }
    else if (m_cfg->getUseScalingListId() == SCALING_LIST_DEFAULT)
    {
        for (int i = 0; i < m_numRows; i++)
        {
            m_rows[i].m_trQuant.setScalingList(m_top->getScalingList());
            m_rows[i].m_trQuant.setUseScalingList(true);
        }

        m_sps.setScalingListPresentFlag(false);
        m_pps.setScalingListPresentFlag(false);
    }
    else
    {
        printf("error : ScalingList == %d not supported\n", m_top->getUseScalingListId());
        assert(0);
    }

    start();
}

int FrameEncoder::getStreamHeaders(NALUnitEBSP **nalunits)
{
    TEncEntropy* entropyCoder = getEntropyCoder(0);

    entropyCoder->setEntropyCoder(&m_sbacCoder, NULL);
    int count = 0;

    /* headers for start of bitstream */
    OutputNALUnit nalu(NAL_UNIT_VPS);
    entropyCoder->setBitstream(&nalu.m_bitstream);
    entropyCoder->encodeVPS(m_cfg->getVPS());
    writeRBSPTrailingBits(nalu.m_bitstream);
    CHECKED_MALLOC(nalunits[count], NALUnitEBSP, 1);
    nalunits[count]->init(nalu);
    count++;

    nalu = NALUnit(NAL_UNIT_SPS);
    entropyCoder->setBitstream(&nalu.m_bitstream);
    entropyCoder->encodeSPS(&m_sps);
    writeRBSPTrailingBits(nalu.m_bitstream);
    CHECKED_MALLOC(nalunits[count], NALUnitEBSP, 1);
    nalunits[count]->init(nalu);
    count++;

    nalu = NALUnit(NAL_UNIT_PPS);
    entropyCoder->setBitstream(&nalu.m_bitstream);
    entropyCoder->encodePPS(&m_pps);
    writeRBSPTrailingBits(nalu.m_bitstream);
    CHECKED_MALLOC(nalunits[count], NALUnitEBSP, 1);
    nalunits[count]->init(nalu);
    count++;

    if (m_cfg->getActiveParameterSetsSEIEnabled())
    {
        SEIActiveParameterSets sei;
        sei.activeVPSId = m_cfg->getVPS()->getVPSId();
        sei.m_fullRandomAccessFlag = false;
        sei.m_noParamSetUpdateFlag = false;
        sei.numSpsIdsMinus1 = 0;
        sei.activeSeqParamSetId = m_sps.getSPSId();

        entropyCoder->setBitstream(&nalu.m_bitstream);
        m_seiWriter.writeSEImessage(nalu.m_bitstream, sei, &m_sps);
        writeRBSPTrailingBits(nalu.m_bitstream);
        CHECKED_MALLOC(nalunits[count], NALUnitEBSP, 1);
        nalunits[count]->init(nalu);
        count++;
    }

    if (m_cfg->getDisplayOrientationSEIAngle())
    {
        SEIDisplayOrientation sei;
        sei.cancelFlag = false;
        sei.horFlip = false;
        sei.verFlip = false;
        sei.anticlockwiseRotation = m_cfg->getDisplayOrientationSEIAngle();

        nalu = NALUnit(NAL_UNIT_PREFIX_SEI);
        entropyCoder->setBitstream(&nalu.m_bitstream);
        m_seiWriter.writeSEImessage(nalu.m_bitstream, sei, &m_sps);
        writeRBSPTrailingBits(nalu.m_bitstream);
        CHECKED_MALLOC(nalunits[count], NALUnitEBSP, 1);
        nalunits[count]->init(nalu);
    }
    return 0;

fail:
    return -1;
}

void FrameEncoder::initSlice(TComPic* pic)
{
    m_pic = pic;
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
    slice->setReferenced(true);
    slice->setScalingList(m_top->getScalingList());
    slice->getScalingList()->setUseTransformSkip(m_pps.getUseTransformSkip());
#if LOG_CU_STATISTICS
    for (int i = 0; i < m_numRows; i++)
    {
        m_rows[i].m_cuCoder.m_log = &m_rows[i].m_cuCoder.m_sliceTypeLog[sliceType];
    }
#endif
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

    slice->setMaxNumMergeCand(m_cfg->param.maxNumMergeCand);
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

void FrameEncoder::setLambda(int qp, int row)
{
    TComSlice* slice = m_pic->getSlice();
    TComPicYuv *fenc = slice->getPic()->getPicYuvOrg();

    double lambda = 0;

    if (m_pic->getSlice()->getSliceType() == I_SLICE)
    {
        lambda = X265_MAX(1, x265_lambda2_tab_I[qp]);
    }
    else
    {
        lambda = X265_MAX(1, x265_lambda2_non_I[qp]);
    }

    // for RDO
    // in RdCost there is only one lambda because the luma and chroma bits are not separated,
    // instead we weight the distortion of chroma.
    int chromaQPOffset = slice->getPPS()->getChromaCbQpOffset() + slice->getSliceQpDeltaCb();
    int qpc = Clip3(0, 70, qp + chromaQPOffset);
    double cbWeight = pow(2.0, (qp - g_chromaScale[qpc])); // takes into account of the chroma qp mapping and chroma qp Offset

    chromaQPOffset = slice->getPPS()->getChromaCrQpOffset() + slice->getSliceQpDeltaCr();
    qpc = Clip3(0, 70, qp + chromaQPOffset);
    double crWeight = pow(2.0, (qp - g_chromaScale[qpc])); // takes into account of the chroma qp mapping and chroma qp Offset
    double chromaLambda = lambda / crWeight;

    m_rows[row].m_search.setQPLambda(qp, lambda, chromaLambda);
    m_rows[row].m_search.m_me.setSourcePlane(fenc->getLumaAddr(), fenc->getStride());
    m_rows[row].m_rdCost.setLambda(lambda);
    m_rows[row].m_rdCost.setCbDistortionWeight(cbWeight);
    m_rows[row].m_rdCost.setCrDistortionWeight(crWeight);
}

void FrameEncoder::compressFrame()
{
    PPAScopeEvent(FrameEncoder_compressFrame);
    int64_t startCompressTime = x265_mdate();
    TEncEntropy* entropyCoder = getEntropyCoder(0);
    TComSlice*   slice        = m_pic->getSlice();
    m_nalCount = 0;

    int qp = slice->getSliceQp();
    double lambda = 0;
    if (slice->getSliceType() == I_SLICE)
    {
        lambda = X265_MAX(1, x265_lambda2_tab_I[qp]);
    }
    else
    {
        lambda = X265_MAX(1, x265_lambda2_non_I[qp]);
    }

    // for RDO
    // in RdCost there is only one lambda because the luma and chroma bits are not separated,
    // instead we weight the distortion of chroma.
    int qpc;
    int chromaQPOffset = slice->getPPS()->getChromaCbQpOffset() + slice->getSliceQpDeltaCb();
    qpc = Clip3(0, 70, qp + chromaQPOffset);
    double cbWeight = pow(2.0, (qp - g_chromaScale[qpc])); // takes into account of the chroma qp mapping and chroma qp Offset
    chromaQPOffset = slice->getPPS()->getChromaCrQpOffset() + slice->getSliceQpDeltaCr();
    qpc = Clip3(0, 70, qp + chromaQPOffset);
    double crWeight = pow(2.0, (qp - g_chromaScale[qpc])); // takes into account of the chroma qp mapping and chroma qp Offset
    double chromaLambda = lambda / crWeight;

    // NOTE: set SAO lambda every Frame
    m_frameFilter.m_sao.lumaLambda = lambda;
    m_frameFilter.m_sao.chromaLambda = chromaLambda;

    TComPicYuv *fenc = slice->getPic()->getPicYuvOrg();
    for (int i = 0; i < m_numRows; i++)
    {
        m_rows[i].m_search.setQPLambda(qp, lambda, chromaLambda);
        m_rows[i].m_search.m_me.setSourcePlane(fenc->getLumaAddr(), fenc->getStride());
        m_rows[i].m_rdCost.setLambda(lambda);
        m_rows[i].m_rdCost.setCbDistortionWeight(cbWeight);
        m_rows[i].m_rdCost.setCrDistortionWeight(crWeight);
    }

    m_frameFilter.m_sao.lumaLambda = lambda;
    m_frameFilter.m_sao.chromaLambda = chromaLambda;
    
    switch (slice->getSliceType())
    {
    case I_SLICE:
        m_frameFilter.m_sao.depth = 0;
        break;
    case P_SLICE:
        m_frameFilter.m_sao.depth = 1;
        break;
    case B_SLICE:
        m_frameFilter.m_sao.depth = 2 + !slice->isReferenced();
        break;
    }
    /* Clip qps back to 0-51 range before encoding */
    if(qp > MAX_QP)
    {
        qp = MAX_QP;
        slice->setSliceQp(qp);
        slice->setSliceQpBase(qp);
    }
    slice->setSliceQpDelta(0);
    slice->setSliceQpDeltaCb(0);
    slice->setSliceQpDeltaCr(0);

    int numSubstreams = m_cfg->param.bEnableWavefront ? m_pic->getPicSym()->getFrameHeightInCU() : 1;
    // TODO: these two items can likely be FrameEncoder member variables to avoid re-allocs
    TComOutputBitstream*  bitstreamRedirect = new TComOutputBitstream;
    TComOutputBitstream*  outStreams = new TComOutputBitstream[numSubstreams];

    if (m_cfg->getUseASR() && !slice->isIntra())
    {
        int pocCurr = slice->getPOC();
        int maxSR = m_cfg->param.searchRange;
        int numPredDir = slice->isInterP() ? 1 : 2;

        for (int dir = 0; dir <= numPredDir; dir++)
        {
            for (int refIdx = 0; refIdx < slice->getNumRefIdx(dir); refIdx++)
            {
                int refPOC = slice->getRefPic(dir, refIdx)->getPOC();
                int newSR = Clip3(8, maxSR, (maxSR * ADAPT_SR_SCALE * abs(pocCurr - refPOC) + 4) >> 3);
                for (int i = 0; i < m_numRows; i++)
                {
                    m_rows[i].m_search.setAdaptiveSearchRange(dir, refIdx, newSR);
                }
            }
        }
    }

    slice->setSliceSegmentBits(0);
    determineSliceBounds();
    slice->setNextSlice(false);

    //------------------------------------------------------------------------------
    //  Weighted Prediction parameters estimation.
    //------------------------------------------------------------------------------
    // calculate AC/DC values for current picture
    m_wp.xStoreWPparam(m_pps.getUseWP(), m_pps.getWPBiPred());
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
    int numPredDir = slice->isInterP() ? 1 : slice->isInterB() ? 2 : 0;
    for (int l = 0; l < numPredDir; l++)
    {
        for (int ref = 0; ref < slice->getNumRefIdx(l); ref++)
        {
            wpScalingParam *w = NULL;
            if ((slice->isInterP() && slice->getPPS()->getUseWP() && slice->m_weightPredTable[l][ref]->bPresentFlag))
            {
                w = slice->m_weightPredTable[l][ref];
                slice->m_numWPRefs++;
            }
            m_mref[l][ref].init(slice->getRefPic(l, ref)->getPicYuvRec(), w);
        }
    }

    // Analyze CTU rows, most of the hard work is done here
    // frame is compressed in a wave-front pattern if WPP is enabled. Loop filter runs as a
    // wave-front behind the CU compression and reconstruction
    compressCTURows();

    if (m_cfg->param.bEnableWavefront)
    {
        slice->setNextSlice(true);
    }

    m_wp.xRestoreWPparam(slice);
    if ((m_cfg->getRecoveryPointSEIEnabled()) && (slice->getSliceType() == I_SLICE))
    {
        if (m_cfg->getGradualDecodingRefreshInfoEnabled() && !slice->getRapPicFlag())
        {
            // Gradual decoding refresh SEI
            OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI);

            SEIGradualDecodingRefreshInfo seiGradualDecodingRefreshInfo;
            seiGradualDecodingRefreshInfo.m_gdrForegroundFlag = true; // Indicating all "foreground"

            m_seiWriter.writeSEImessage(nalu.m_bitstream, seiGradualDecodingRefreshInfo, slice->getSPS());
            writeRBSPTrailingBits(nalu.m_bitstream);
            m_nalList[m_nalCount] = (NALUnitEBSP*)X265_MALLOC(NALUnitEBSP, 1);
            if (m_nalList[m_nalCount])
            {
                m_nalList[m_nalCount]->init(nalu);
                m_nalCount++;
            }
        }
        // Recovery point SEI
        OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI);

        SEIRecoveryPoint sei_recovery_point;
        sei_recovery_point.m_recoveryPocCnt    = 0;
        sei_recovery_point.m_exactMatchingFlag = (slice->getPOC() == 0) ? (true) : (false);
        sei_recovery_point.m_brokenLinkFlag    = false;

        m_seiWriter.writeSEImessage(nalu.m_bitstream, sei_recovery_point, slice->getSPS());
        writeRBSPTrailingBits(nalu.m_bitstream);
        m_nalList[m_nalCount] = (NALUnitEBSP*)X265_MALLOC(NALUnitEBSP, 1);
        if (m_nalList[m_nalCount])
        {
            m_nalList[m_nalCount]->init(nalu);
            m_nalCount++;
        }
    }

    /* use the main bitstream buffer for storing the marshaled picture */
    if (m_sps.getUseSAO())
    {
        SAOParam* saoParam = m_pic->getPicSym()->getSaoParam();

        if (!getSAO()->getSaoLcuBasedOptimization())
        {
            getSAO()->SAOProcess(saoParam);
            getSAO()->endSaoEnc();
            PCMLFDisableProcess(m_pic);

            // Extend border after whole-frame SAO is finished
            for (int row = 0; row < m_numRows; row++)
            {
                m_frameFilter.processRowPost(row);
            }
        }

        slice->setSaoEnabledFlag((saoParam->bSaoFlag[0] == 1) ? true : false);
    }

    entropyCoder->setBitstream(NULL);

    // Reconstruction slice
    slice->setNextSlice(true);
    determineSliceBounds();

    slice->allocSubstreamSizes(numSubstreams);
    for (int i = 0; i < numSubstreams; i++)
    {
        outStreams[i].clear();
    }

    entropyCoder->setEntropyCoder(&m_sbacCoder, slice);
    entropyCoder->resetEntropy();

    /* start slice NALunit */
    bool sliceSegment = !slice->isNextSlice();
    OutputNALUnit nalu(slice->getNalUnitType(), 0);
    entropyCoder->setBitstream(&nalu.m_bitstream);
    entropyCoder->encodeSliceHeader(slice);

    // is it needed?
    if (!sliceSegment)
    {
        bitstreamRedirect->writeAlignOne();
    }
    else
    {
        // We've not completed our slice header info yet, do the alignment later.
    }

    m_sbacCoder.init(&m_binCoderCABAC);
    entropyCoder->setEntropyCoder(&m_sbacCoder, slice);
    entropyCoder->resetEntropy();
    resetEntropy(slice);

    if (slice->isNextSlice())
    {
        // set entropy coder for writing
        m_sbacCoder.init(&m_binCoderCABAC);
        resetEntropy(slice);
        getSbacCoder(0)->load(&m_sbacCoder);

        //ALF is written in substream #0 with CABAC coder #0 (see ALF param encoding below)
        entropyCoder->setEntropyCoder(getSbacCoder(0), slice);
        entropyCoder->resetEntropy();

        // File writing
        if (!sliceSegment)
        {
            entropyCoder->setBitstream(bitstreamRedirect);
        }
        else
        {
            entropyCoder->setBitstream(&nalu.m_bitstream);
        }

        // for now, override the TILES_DECODER setting in order to write substreams.
        entropyCoder->setBitstream(&outStreams[0]);
    }
    slice->setFinalized(true);

    m_sbacCoder.load(getSbacCoder(0));

    slice->setTileOffstForMultES(0);
    encodeSlice(outStreams);

    {
        // Construct the final bitstream by flushing and concatenating substreams.
        // The final bitstream is either nalu.m_bitstream or pcBitstreamRedirect;
        uint32_t* substreamSizes = slice->getSubstreamSizes();
        for (int i = 0; i < numSubstreams; i++)
        {
            // Flush all substreams -- this includes empty ones.
            // Terminating bit and flush.
            entropyCoder->setEntropyCoder(getSbacCoder(i), slice);
            entropyCoder->setBitstream(&outStreams[i]);
            entropyCoder->encodeTerminatingBit(1);
            entropyCoder->encodeSliceFinish();

            outStreams[i].writeByteAlignment(); // Byte-alignment in slice_data() at end of sub-stream

            // Byte alignment is necessary between tiles when tiles are independent.
            if (i + 1 < numSubstreams)
            {
                substreamSizes[i] = outStreams[i].getNumberOfWrittenBits() + (outStreams[i].countStartCodeEmulations() << 3);
            }
        }

        // Complete the slice header info.
        entropyCoder->setEntropyCoder(&m_sbacCoder, slice);
        entropyCoder->setBitstream(&nalu.m_bitstream);
        entropyCoder->encodeTilesWPPEntryPoint(slice);

        // Substreams...
        int nss = m_pps.getEntropyCodingSyncEnabledFlag() ? slice->getNumEntryPointOffsets() + 1 : numSubstreams;
        for (int i = 0; i < nss; i++)
        {
            bitstreamRedirect->addSubstream(&outStreams[i]);
        }
    }

    // If current NALU is the first NALU of slice (containing slice header) and
    // more NALUs exist (due to multiple dependent slices) then buffer it.  If
    // current NALU is the last NALU of slice and a NALU was buffered, then (a)
    // Write current NALU (b) Update an write buffered NALU at appropriate
    // location in NALU list.
    nalu.m_bitstream.writeByteAlignment(); // Slice header byte-alignment

    // Perform bitstream concatenation
    if (bitstreamRedirect->getNumberOfWrittenBits() > 0)
    {
        nalu.m_bitstream.addSubstream(bitstreamRedirect);
    }
    entropyCoder->setBitstream(&nalu.m_bitstream);
    bitstreamRedirect->clear();
    m_nalList[m_nalCount] = (NALUnitEBSP*)X265_MALLOC(NALUnitEBSP, 1);
    if (m_nalList[m_nalCount])
    {
        m_nalList[m_nalCount]->init(nalu);
        m_nalCount++;
    }

    /* write decoded picture hash SEI messages */
    if (m_cfg->param.decodedPictureHashSEI)
    {
        if (m_cfg->param.decodedPictureHashSEI == 1)
        {
            m_seiReconPictureDigest.method = SEIDecodedPictureHash::MD5;
            for (int i = 0; i < 3; i++)
            {
                MD5Final(&(m_pic->m_state[i]), m_seiReconPictureDigest.digest[i]);
            }
        }
        else if (m_cfg->param.decodedPictureHashSEI == 2)
        {
            m_seiReconPictureDigest.method = SEIDecodedPictureHash::CRC;
            for (int i = 0; i < 3; i++)
            {
                crcFinish((m_pic->m_crc[i]), m_seiReconPictureDigest.digest[i]);
            }
        }
        else if (m_cfg->param.decodedPictureHashSEI == 3)
        {
            m_seiReconPictureDigest.method = SEIDecodedPictureHash::CHECKSUM;
            for (int i = 0; i < 3; i++)
            {
                checksumFinish(m_pic->m_checksum[i], m_seiReconPictureDigest.digest[i]);
            }
        }
        OutputNALUnit onalu(NAL_UNIT_SUFFIX_SEI, 0);
        m_seiWriter.writeSEImessage(onalu.m_bitstream, m_seiReconPictureDigest, slice->getSPS());
        writeRBSPTrailingBits(onalu.m_bitstream);

        m_nalList[m_nalCount] = (NALUnitEBSP*)X265_MALLOC(NALUnitEBSP, 1);
        if (m_nalList[m_nalCount])
        {
            m_nalList[m_nalCount]->init(onalu);
            m_nalCount++;
        }
    }

    if (m_sps.getUseSAO())
    {
        m_frameFilter.end();
    }

    // TODO: merge into compress loop, need some time to verify, remove later
    //m_pic->compressMotion();

    /* Decrement referenced frame reference counts, allow them to be recycled */
    for (int l = 0; l < numPredDir; l++)
    {
        for (int ref = 0; ref < slice->getNumRefIdx(l); ref++)
        {
            TComPic *refpic = slice->getRefPic(l, ref);
            ATOMIC_DEC(&refpic->m_countRefEncoders);
        }
    }

    m_pic->m_elapsedCompressTime = (double)(x265_mdate() - startCompressTime) / 1000000;
    delete[] outStreams;
    delete bitstreamRedirect;
}

void FrameEncoder::encodeSlice(TComOutputBitstream* substreams)
{
    // choose entropy coder
    TEncEntropy *entropyCoder = getEntropyCoder(0);
    TComSlice* slice = m_pic->getSlice();

    // Initialize slice singletons
    m_sbacCoder.init(&m_binCoderCABAC);
    getCuEncoder(0)->setBitCounter(NULL);
    entropyCoder->setEntropyCoder(&m_sbacCoder, slice);

    uint32_t cuAddr;
    uint32_t startCUAddr = 0;
    uint32_t boundingCUAddr = slice->getSliceCurEndCUAddr();

    // Appropriate substream bitstream is switched later.
    // for every CU
#if ENC_DEC_TRACE
    g_bJustDoIt = g_bEncDecTraceEnable;
#endif
    DTRACE_CABAC_VL(g_nSymbolCounter++);
    DTRACE_CABAC_T("\tPOC: ");
    DTRACE_CABAC_V(m_pic->getPOC());
    DTRACE_CABAC_T("\n");
#if ENC_DEC_TRACE
    g_bJustDoIt = g_bEncDecTraceDisable;
#endif

    const int  bWaveFrontsynchro = m_cfg->param.bEnableWavefront;
    const uint32_t heightInLCUs = m_pic->getPicSym()->getFrameHeightInCU();
    const int  numSubstreams = (bWaveFrontsynchro ? heightInLCUs : 1);
    uint32_t bitsOriginallyInSubstreams = 0;

    for (int substrmIdx = 0; substrmIdx < numSubstreams; substrmIdx++)
    {
        getBufferSBac(substrmIdx)->loadContexts(&m_sbacCoder); //init. state
        bitsOriginallyInSubstreams += substreams[substrmIdx].getNumberOfWrittenBits();
    }

    uint32_t widthInLCUs = m_pic->getPicSym()->getFrameWidthInCU();
    uint32_t col = 0, lin = 0, subStrm = 0;
    cuAddr = (startCUAddr / m_pic->getNumPartInCU()); /* for tiles, startCUAddr is NOT the real raster scan address, it is actually
                                                       an encoding order index, so we need to convert the index (startCUAddr)
                                                       into the real raster scan address (cuAddr) via the CUOrderMap */
    uint32_t encCUOrder;
    for (encCUOrder = startCUAddr / m_pic->getNumPartInCU();
         encCUOrder < (boundingCUAddr + m_pic->getNumPartInCU() - 1) / m_pic->getNumPartInCU();
         cuAddr = ++encCUOrder)
    {
        col     = cuAddr % widthInLCUs;
        lin     = cuAddr / widthInLCUs;
        subStrm = lin % numSubstreams;

        entropyCoder->setBitstream(&substreams[subStrm]);

        // Synchronize cabac probabilities with upper-right LCU if it's available and we're at the start of a line.
        if ((numSubstreams > 1) && (col == 0) && bWaveFrontsynchro)
        {
            // We'll sync if the TR is available.
            TComDataCU *cuUp = m_pic->getCU(cuAddr)->getCUAbove();
            uint32_t widthInCU = m_pic->getFrameWidthInCU();
            TComDataCU *cuTr = NULL;

            // CHECK_ME: here can be optimize a little, do it later
            if (cuUp && ((cuAddr % widthInCU + 1) < widthInCU))
            {
                cuTr = m_pic->getCU(cuAddr - widthInCU + 1);
            }
            if ( /*bEnforceSliceRestriction &&*/ ((cuTr == NULL) || (cuTr->getSlice() == NULL)))
            {
                // TR not available.
            }
            else
            {
                // TR is available, we use it.
                getSbacCoder(subStrm)->loadContexts(getBufferSBac(lin - 1));
            }
        }
        m_sbacCoder.load(getSbacCoder(subStrm)); //this load is used to simplify the code (avoid to change all the call to m_sbacCoder)

        TComDataCU* cu = m_pic->getCU(cuAddr);
        if (slice->getSPS()->getUseSAO() && (slice->getSaoEnabledFlag() || slice->getSaoEnabledFlagChroma()))
        {
            SAOParam *saoParam = slice->getPic()->getPicSym()->getSaoParam();
            int numCuInWidth     = saoParam->numCuInWidth;
            int cuAddrInSlice    = cuAddr;
            int rx = cuAddr % numCuInWidth;
            int ry = cuAddr / numCuInWidth;
            int allowMergeLeft = 1;
            int allowMergeUp   = 1;
            int addr = cu->getAddr();
            allowMergeLeft = (rx > 0) && (cuAddrInSlice != 0);
            allowMergeUp = (ry > 0) && (cuAddrInSlice >= 0);
            if (saoParam->bSaoFlag[0] || saoParam->bSaoFlag[1])
            {
                int mergeLeft = saoParam->saoLcuParam[0][addr].mergeLeftFlag;
                int mergeUp = saoParam->saoLcuParam[0][addr].mergeUpFlag;
                if (allowMergeLeft)
                {
                    entropyCoder->m_entropyCoderIf->codeSaoMerge(mergeLeft);
                }
                else
                {
                    mergeLeft = 0;
                }
                if (mergeLeft == 0)
                {
                    if (allowMergeUp)
                    {
                        entropyCoder->m_entropyCoderIf->codeSaoMerge(mergeUp);
                    }
                    else
                    {
                        mergeUp = 0;
                    }
                    if (mergeUp == 0)
                    {
                        for (int compIdx = 0; compIdx < 3; compIdx++)
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
            int addr = cu->getAddr();
            SAOParam *saoParam = slice->getPic()->getPicSym()->getSaoParam();
            for (int cIdx = 0; cIdx < 3; cIdx++)
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
        getCuEncoder(0)->setEntropyCoder(entropyCoder);
        getCuEncoder(0)->encodeCU(cu);

#if ENC_DEC_TRACE
        g_bJustDoIt = g_bEncDecTraceDisable;
#endif

        // load back status of the entropy coder after encoding the LCU into relevant bitstream entropy coder
        getSbacCoder(subStrm)->load(&m_sbacCoder);

        // Store probabilities of second LCU in line into buffer
        if ((numSubstreams > 1) && (col == 1) && bWaveFrontsynchro)
        {
            getBufferSBac(lin)->loadContexts(getSbacCoder(subStrm));
        }
    }

    if (slice->getPPS()->getCabacInitPresentFlag())
    {
        entropyCoder->determineCabacInitIdx();
    }
}

/** Determines the starting and bounding LCU address of current slice / dependent slice
 * \returns Updates startCUAddr, boundingCUAddr with appropriate LCU address
 */
void FrameEncoder::determineSliceBounds()
{
    uint32_t numberOfCUsInFrame = m_pic->getNumCUsInFrame();
    uint32_t boundingCUAddrSlice = numberOfCUsInFrame * m_pic->getNumPartInCU();

    // WPP: if a slice does not start at the beginning of a CTB row, it must end within the same CTB row
    m_pic->getSlice()->setSliceCurEndCUAddr(boundingCUAddrSlice);
}

void FrameEncoder::compressCTURows()
{
    PPAScopeEvent(FrameEncoder_compressRows);
    TComSlice* slice = m_pic->getSlice();

    // reset entropy coders
    m_sbacCoder.init(&m_binCoderCABAC);
    for (int i = 0; i < this->m_numRows; i++)
    {
        m_rows[i].init(slice);
        m_rows[i].m_entropyCoder.setEntropyCoder(&m_sbacCoder, slice);
        m_rows[i].m_entropyCoder.resetEntropy();

        m_rows[i].m_rdSbacCoders[0][CI_CURR_BEST]->load(&m_sbacCoder);
        m_rows[i].m_completed = 0;
        m_rows[i].m_rdGoOnBinCodersCABAC.m_fracBits = 0;
    }

    uint32_t refLagRows = ((m_cfg->param.searchRange + NTAPS_LUMA / 2 + g_maxCUHeight - 1) / g_maxCUHeight) + 1;
    int numPredDir = slice->isInterP() ? 1 : slice->isInterB() ? 2 : 0;

    m_pic->m_SSDY = 0;
    m_pic->m_SSDU = 0;
    m_pic->m_SSDV = 0;

    m_frameFilter.start(m_pic);

    if (m_pool && m_cfg->param.bEnableWavefront)
    {
        WaveFront::clearEnabledRowMask();
        WaveFront::enqueue();

        for (uint32_t row = 0; row < (uint32_t)m_numRows; row++)
        {
            // block until all reference frames have reconstructed the rows we need
            for (int l = 0; l < numPredDir; l++)
            {
                for (int ref = 0; ref < slice->getNumRefIdx(l); ref++)
                {
                    TComPic *refpic = slice->getRefPic(l, ref);
                    while ((refpic->m_reconRowCount != (uint32_t)m_numRows) && (refpic->m_reconRowCount < row + refLagRows))
                    {
                        refpic->m_reconRowWait.wait();
                    }

                    if (slice->getPPS()->getUseWP() && slice->getSliceType() == P_SLICE && m_mref[l][ref].isWeighted)
                    {
                        m_mref[l][ref].applyWeight(row + refLagRows, m_numRows);
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
                        TComPic *refpic = slice->getRefPic(list, ref);
                        while ((refpic->m_reconRowCount != (uint32_t)m_numRows) && (refpic->m_reconRowCount < i + refLagRows))
                        {
                            refpic->m_reconRowWait.wait();
                        }

                        if (slice->getPPS()->getUseWP() && slice->getSliceType() == P_SLICE && m_mref[l][ref].isWeighted)
                        {
                            m_mref[list][ref].applyWeight(i + refLagRows, m_numRows);
                        }
                    }
                }

                processRow(i * 2 + 0);
            }

            // Filter
            if (i >= m_filterRowDelay)
            {
                processRow((i - m_filterRowDelay) * 2 + 1);
            }
        }
    }
    m_pic->m_frameTime = (double)m_totalTime / 1000000;
    m_totalTime = 0;
}

void FrameEncoder::processRowEncoder(int row)
{
    PPAScopeEvent(Thread_ProcessRow);

    int64_t startTime = x265_mdate();
    // Called by worker threads
    CTURow& curRow  = m_rows[row];
    CTURow& codeRow = m_rows[m_cfg->param.bEnableWavefront ? row : 0];

    const uint32_t numCols = m_pic->getPicSym()->getFrameWidthInCU();
    const uint32_t lineStartCUAddr = row * numCols;
    for (uint32_t col = m_rows[row].m_completed; col < numCols; col++)
    {
        const uint32_t cuAddr = lineStartCUAddr + col;
        TComDataCU* cu = m_pic->getCU(cuAddr);
        cu->initCU(m_pic, cuAddr);

        codeRow.m_entropyCoder.setEntropyCoder(&m_sbacCoder, m_pic->getSlice());
        codeRow.m_entropyCoder.resetEntropy();

        TEncSbac *bufSbac = (m_cfg->param.bEnableWavefront && col == 0 && row > 0) ? &m_rows[row - 1].m_bufferSbacCoder : NULL;
        if (m_cfg->param.rc.aqMode)
        {
            int qp = calcQpForCu(m_pic, cuAddr);
            setLambda(qp, row);
            if (qp > MAX_QP)
                qp = MAX_QP;
            cu->setQP(0, (char)qp);

        }
        codeRow.processCU(cu, m_pic->getSlice(), bufSbac, m_cfg->param.bEnableWavefront && col == 1);

        // TODO: Keep atomic running totals for rate control?
        // cu->m_totalBits;
        // cu->m_totalCost;
        // cu->m_totalDistortion;

        // Completed CU processing
        m_rows[row].m_completed++;

        if (m_rows[row].m_completed >= 2 && row < m_numRows - 1)
        {
            ScopedLock below(m_rows[row + 1].m_lock);
            if (m_rows[row + 1].m_active == false &&
                m_rows[row + 1].m_completed + 2 <= m_rows[row].m_completed)
            {
                m_rows[row + 1].m_active = true;
                enqueueRowEncoder(row + 1);
            }
        }

        ScopedLock self(curRow.m_lock);
        if (row > 0 && m_rows[row].m_completed < numCols - 1 && m_rows[row - 1].m_completed < m_rows[row].m_completed + 2)
        {
            curRow.m_active = false;
            m_totalTime = m_totalTime + (x265_mdate() - startTime);
            return;
        }
    }

    // this row of CTUs has been encoded

    // Run row-wise loop filters
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
    m_totalTime = m_totalTime + (x265_mdate() - startTime);
}

int FrameEncoder::calcQpForCu(TComPic *pic, uint32_t cuAddr)
{
    x265_emms();
    double qp = pic->getSlice()->m_avgQpRc;
    if (m_cfg->param.rc.aqMode)
    {
        /* Derive qpOffet for each CU by averaging offsets for all 16x16 blocks in the cu. */
        double qp_offset = 0;
        int maxBlockCols = (pic->getPicYuvOrg()->getWidth() + (16 - 1)) / 16;
        int maxBlockRows = (pic->getPicYuvOrg()->getHeight() + (16 - 1)) / 16;
        int noOfBlocks = g_maxCUWidth / 16;
        int block_y = (cuAddr / pic->getPicSym()->getFrameWidthInCU()) * noOfBlocks;
        int block_x = (cuAddr * noOfBlocks) - block_y * pic->getPicSym()->getFrameWidthInCU();

        double *qpoffs = (pic->getSlice()->isReferenced() && m_cfg->param.rc.cuTree) ? pic->m_lowres.qpOffset : pic->m_lowres.qpAqOffset;
        int cnt = 0;
        for (int h = 0; h < noOfBlocks && block_y < maxBlockRows; h++, block_y++)
        {
            for (int w = 0; w < noOfBlocks && (block_x + w) < maxBlockCols; w++)
            {
                qp_offset += qpoffs[block_x + w + (block_y * maxBlockCols)];
                cnt++;
            }
        }

        qp_offset /= cnt;
        qp += qp_offset;
    }
    return Clip3(MIN_QP, MAX_MAX_QP, (int)(qp + 0.5));
}

TComPic *FrameEncoder::getEncodedPicture(NALUnitEBSP **nalunits)
{
    if (m_pic)
    {
        /* block here until worker thread completes */
        m_done.wait();

        TComPic *ret = m_pic;
        m_pic = NULL;

        if (nalunits)
        {
            // move NALs from member variable to user's container
            assert(m_nalCount <= MAX_NAL_UNITS);
            ::memcpy(nalunits, m_nalList, sizeof(NALUnitEBSP*) * m_nalCount);
            m_nalCount = 0;
        }
        return ret;
    }

    return NULL;
}
