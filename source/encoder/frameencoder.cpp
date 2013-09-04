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
#include "TLibEncoder/NALwrite.h"
#include "frameencoder.h"
#include "cturow.h"

#include <math.h>

using namespace x265;

#if CU_STAT_LOGFILE
int cntInter[4], cntIntra[4], cntSplit[4],  totalCU;
int cuInterDistribution[4][4], cuIntraDistribution[4][3], cntIntraNxN;
int cntSkipCu[4], cntTotalCu[4];
extern FILE * fp, * fp1;
#endif

enum SCALING_LIST_PARAMETER
{
    SCALING_LIST_OFF,
    SCALING_LIST_DEFAULT,
};

FrameEncoder::FrameEncoder()
    : WaveFront(NULL)
    , m_threadActive(true)
    , m_top(NULL)
    , m_cfg(NULL)
    , m_frameFilter(NULL)
    , m_pic(NULL)
    , m_rows(NULL)
{}

void FrameEncoder::setThreadPool(ThreadPool *p)
{
    m_pool = p;
    m_frameFilter.setThreadPool(p);
}

void FrameEncoder::destroy()
{
    JobProvider::flush();  // ensure no worker threads are using this frame

    m_threadActive = false;
    m_enable.trigger();

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

void FrameEncoder::init(TEncTop *top, int numRows)
{
    m_top = top;
    m_cfg = top;
    m_numRows = numRows;
    row_delay = (m_cfg->param.saoLcuBasedOptimization && m_cfg->param.saoLcuBoundary) ? 2 : 1;

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

    m_frameFilter.init(top, numRows, getEntropyCoder(0), getRDGoOnSbacCoder(0));

    // initialize SPS
    top->xInitSPS(&m_sps);

    // initialize PPS
    m_pps.setSPS(&m_sps);

    top->xInitPPS(&m_pps);

    m_sps.setNumLongTermRefPicSPS(0);
    if (m_cfg->getPictureTimingSEIEnabled() || m_cfg->getDecodingUnitInfoSEIEnabled())
    {
        m_sps.getVuiParameters()->getHrdParameters()->setNumDU(0);
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
        setFlatScalingList();
        setUseScalingList(false);
        m_sps.setScalingListPresentFlag(false);
        m_pps.setScalingListPresentFlag(false);
    }
    else if (m_cfg->getUseScalingListId() == SCALING_LIST_DEFAULT)
    {
        m_sps.setScalingListPresentFlag(false);
        m_pps.setScalingListPresentFlag(false);
        setScalingList(m_top->getScalingList());
        setUseScalingList(true);
    }
    else
    {
        printf("error : ScalingList == %d not supported\n", m_top->getUseScalingListId());
        assert(0);
    }
    start();
}

int FrameEncoder::getStreamHeaders(AccessUnit& accessUnit)
{
    TEncEntropy* entropyCoder = getEntropyCoder(0);

    entropyCoder->setEntropyCoder(&m_cavlcCoder, NULL);

    /* headers for start of bitstream */
    OutputNALUnit nalu(NAL_UNIT_VPS);
    entropyCoder->setBitstream(&nalu.m_Bitstream);
    entropyCoder->encodeVPS(m_cfg->getVPS());
    writeRBSPTrailingBits(nalu.m_Bitstream);
    accessUnit.push_back(new NALUnitEBSP(nalu));

    nalu = NALUnit(NAL_UNIT_SPS);
    entropyCoder->setBitstream(&nalu.m_Bitstream);
    entropyCoder->encodeSPS(&m_sps);
    writeRBSPTrailingBits(nalu.m_Bitstream);
    accessUnit.push_back(new NALUnitEBSP(nalu));

    nalu = NALUnit(NAL_UNIT_PPS);
    entropyCoder->setBitstream(&nalu.m_Bitstream);
    entropyCoder->encodePPS(&m_pps);
    writeRBSPTrailingBits(nalu.m_Bitstream);
    accessUnit.push_back(new NALUnitEBSP(nalu));

    if (m_cfg->getActiveParameterSetsSEIEnabled())
    {
        SEIActiveParameterSets sei;
        sei.activeVPSId = m_cfg->getVPS()->getVPSId();
        sei.m_fullRandomAccessFlag = false;
        sei.m_noParamSetUpdateFlag = false;
        sei.numSpsIdsMinus1 = 0;
        sei.activeSeqParamSetId.resize(sei.numSpsIdsMinus1 + 1);
        sei.activeSeqParamSetId[0] = m_sps.getSPSId();

        entropyCoder->setBitstream(&nalu.m_Bitstream);
        m_seiWriter.writeSEImessage(nalu.m_Bitstream, sei, &m_sps);
        writeRBSPTrailingBits(nalu.m_Bitstream);
        accessUnit.push_back(new NALUnitEBSP(nalu));
    }

    if (m_cfg->getDisplayOrientationSEIAngle())
    {
        SEIDisplayOrientation sei;
        sei.cancelFlag = false;
        sei.horFlip = false;
        sei.verFlip = false;
        sei.anticlockwiseRotation = m_cfg->getDisplayOrientationSEIAngle();

        nalu = NALUnit(NAL_UNIT_PREFIX_SEI);
        entropyCoder->setBitstream(&nalu.m_Bitstream);
        m_seiWriter.writeSEImessage(nalu.m_Bitstream, sei, &m_sps);
        writeRBSPTrailingBits(nalu.m_Bitstream);
        accessUnit.push_back(new NALUnitEBSP(nalu));
    }
    return 0;
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
    SliceType sliceType = IS_X265_TYPE_B(type)? B_SLICE : ((type == X265_TYPE_P) ? P_SLICE: I_SLICE);
    slice->setSliceType(sliceType);
    slice->setReferenced(true);
    slice->setScalingList(m_top->getScalingList());
    slice->getScalingList()->setUseTransformSkip(m_pps.getUseTransformSkip());

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

    // depth computation based on GOP size
    int depth = 0;
    int poc = slice->getPOC() % m_cfg->getGOPSizeMin();
    if (poc)
    {
        int step = m_cfg->getGOPSizeMin();
        for (int i = step >> 1; i >= 1; i >>= 1)
        {
            for (int j = i; j < m_cfg->getGOPSizeMin(); j += step)
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

    double qpdouble;
    double lambda;
    qpdouble = m_cfg->param.rc.qp;

    // ------------------------------------------------------------------------------------------------------------------
    // Lambda computation
    // ------------------------------------------------------------------------------------------------------------------

    int qp = X265_MAX(-m_sps.getQpBDOffsetY(), X265_MIN(MAX_QP, (int)floor(qpdouble + 0.5)));
    if (slice->getSliceType() == I_SLICE)
    {
        lambda = X265_MAX(1,x265_lambda2_tab_I[qp]);
    }
    else
    {
        qp += 2;
        lambda = X265_MAX(1,x265_lambda2_non_I[qp]);
    }

    // for RDO
    // in RdCost there is only one lambda because the luma and chroma bits are not separated,
    // instead we weight the distortion of chroma.
    double weight = 1.0;
    int qpc;
    int chromaQPOffset;

    chromaQPOffset = slice->getPPS()->getChromaCbQpOffset() + slice->getSliceQpDeltaCb();
    qpc = Clip3(0, 57, qp + chromaQPOffset);
    weight = pow(2.0, (qp - g_chromaScale[qpc]) / 3.0); // takes into account of the chroma qp mapping and chroma qp Offset
    setCbDistortionWeight(weight);

    chromaQPOffset = slice->getPPS()->getChromaCrQpOffset() + slice->getSliceQpDeltaCr();
    qpc = Clip3(0, 57, qp + chromaQPOffset);
    weight = pow(2.0, (qp - g_chromaScale[qpc]) / 3.0); // takes into account of the chroma qp mapping and chroma qp Offset
    setCrDistortionWeight(weight);

    // for RDOQ
    setQPLambda(qp, lambda, lambda / weight, slice->getDepth());

    // For SAO
    slice->setLambda(lambda, lambda / weight);

    slice->setSliceQp(qp);
    slice->setSliceQpBase(qp);
    slice->setSliceQpDelta(0);
    slice->setSliceQpDeltaCb(0);
    slice->setSliceQpDeltaCr(0);
}

void FrameEncoder::compressFrame()
{
    PPAScopeEvent(FrameEncoder_compressFrame);

    TEncEntropy* entropyCoder = getEntropyCoder(0);
    TComSlice*   slice        = m_pic->getSlice();

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
            RefPicList e = (dir ? REF_PIC_LIST_1 : REF_PIC_LIST_0);
            for (int refIdx = 0; refIdx < slice->getNumRefIdx(e); refIdx++)
            {
                int refPOC = slice->getRefPic(e, refIdx)->getPOC();
                int newSR = Clip3(8, maxSR, (maxSR * ADAPT_SR_SCALE * abs(pocCurr - refPOC) + 4) >> 3);
                setAdaptiveSearchRange(dir, refIdx, newSR);
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
        RefPicList list = (l ? REF_PIC_LIST_1 : REF_PIC_LIST_0);
        wpScalingParam *w = NULL;
        for (int ref = 0; ref < slice->getNumRefIdx(list); ref++)
        {
            TComPicYuv *recon = slice->getRefPic(list, ref)->getPicYuvRec();
            if ((slice->isInterP() && slice->getPPS()->getUseWP()))
                w = slice->m_weightPredTable[list][ref];
            slice->m_mref[list][ref] = recon->generateMotionReference(w);
        }
    }

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

    if (m_sps.getUseSAO())
    {
        m_pic->createNonDBFilterInfo(slice->getSliceCurEndCUAddr(), 0);
    }

    // Analyze CTU rows, most of the hard work is done here
    // frame is compressed in a wave-front pattern if WPP is enabled. Loop filter runs as a
    // wave-front behind the CU compression and reconstruction
    compressCTURows();

#if CU_STAT_LOGFILE
    if (slice->getSliceType() != I_SLICE)
    {
        fprintf(fp, " FRAME  - Inter FRAME \n");
        fprintf(fp, "64x64 : Inter [%.2f %%  (2Nx2N %.2f %%,  Nx2N %.2f %% , 2NxN %.2f %%, AMP %.2f %%)] Intra [%.2f %%(DC %.2f %% Planar %.2f %% Ang %.2f%% )] Split[%.2f] %% Skipped[%.2f]%% \n", (double)(cntInter[0] * 100) / cntTotalCu[0], (double)(cuInterDistribution[0][0] * 100) / cntInter[0],  (double)(cuInterDistribution[0][2] * 100) / cntInter[0], (double)(cuInterDistribution[0][1] * 100) / cntInter[0], (double)(cuInterDistribution[0][3] * 100) / cntInter[0], (double)(cntIntra[0] * 100) / cntTotalCu[0], (double)(cuIntraDistribution[0][0] * 100) / cntIntra[0], (double)(cuIntraDistribution[0][1] * 100) / cntIntra[0], (double)(cuIntraDistribution[0][2] * 100) / cntIntra[0], (double)(cntSplit[0] * 100) / cntTotalCu[0], (double)(cntSkipCu[0] * 100) / cntTotalCu[0]);
        fprintf(fp, "32x32 : Inter [%.2f %% (2Nx2N %.2f %%,  Nx2N %.2f %%, 2NxN %.2f %%, AMP %.2f %%)] Intra [%.2f %%(DC %.2f %% Planar %.2f %% Ang %.2f %% )] Split[%.2f] %% Skipped[%.2f] %%\n", (double)(cntInter[1] * 100) / cntTotalCu[1], (double)(cuInterDistribution[1][0] * 100) / cntInter[1],  (double)(cuInterDistribution[1][2] * 100) / cntInter[1], (double)(cuInterDistribution[1][1] * 100) / cntInter[1], (double)(cuInterDistribution[1][3] * 100) / cntInter[1], (double)(cntIntra[1] * 100) / cntTotalCu[1], (double)(cuIntraDistribution[1][0] * 100) / cntIntra[1], (double)(cuIntraDistribution[1][1] * 100) / cntIntra[1], (double)(cuIntraDistribution[1][2] * 100) / cntIntra[1], (double)(cntSplit[1] * 100) / cntTotalCu[1], (double)(cntSkipCu[1] * 100) / cntTotalCu[1]);
        fprintf(fp, "16x16 : Inter [%.2f %% (2Nx2N %.2f %%,  Nx2N %.2f %%, 2NxN %.2f %%, AMP %.2f %%)] Intra [%.2f %%(DC %.2f %% Planar %.2f %% Ang %.2f %% )] Split[%.2f] %% Skipped[%.2f]%% \n", (double)(cntInter[2] * 100) / cntTotalCu[2], (double)(cuInterDistribution[2][0] * 100) / cntInter[2],  (double)(cuInterDistribution[2][2] * 100) / cntInter[2], (double)(cuInterDistribution[2][1] * 100) / cntInter[2], (double)(cuInterDistribution[2][3] * 100) / cntInter[2], (double)(cntIntra[2] * 100) / cntTotalCu[2], (double)(cuIntraDistribution[2][0] * 100) / cntIntra[2], (double)(cuIntraDistribution[2][1] * 100) / cntIntra[2], (double)(cuIntraDistribution[2][2] * 100) / cntIntra[2], (double)(cntSplit[2] * 100) / cntTotalCu[2], (double)(cntSkipCu[2] * 100) / cntTotalCu[2]);
        fprintf(fp, "8x8 : Inter [%.2f %% (2Nx2N %.2f %%,  Nx2N %.2f %%, 2NxN %.2f %%, AMP %.2f %%)] Intra [%.2f %%(DC %.2f  %% Planar %.2f %% Ang %.2f %%) NxN[%.2f] %% ] Skipped[%.2f] %% \n \n", (double)(cntInter[3] * 100) / cntTotalCu[3], (double)(cuInterDistribution[3][0] * 100) / cntInter[3],  (double)(cuInterDistribution[3][2] * 100) / cntInter[3], (double)(cuInterDistribution[3][1] * 100) / cntInter[3], (double)(cuInterDistribution[3][3] * 100) / cntInter[3], (double)((cntIntra[3]) * 100) / cntTotalCu[3], (double)(cuIntraDistribution[3][0] * 100) / cntIntra[3], (double)(cuIntraDistribution[3][1] * 100) / cntIntra[3], (double)(cuIntraDistribution[3][2] * 100) / cntIntra[3], (double)(cntIntraNxN * 100) / cntIntra[3], (double)(cntSkipCu[3] * 100) / cntTotalCu[3]);
    }
    else
    {
        fprintf(fp, " FRAME - Intra FRAME \n");
        fprintf(fp, "64x64 : Intra [%.2f %%] Skipped [%.2f %%]\n", (double)(cntIntra[0] * 100) / cntTotalCu[0], (double)(cntSkipCu[0] * 100) / cntTotalCu[0]);
        fprintf(fp, "32x32 : Intra [%.2f %%] Skipped [%.2f %%] \n", (double)(cntIntra[1] * 100) / cntTotalCu[1], (double)(cntSkipCu[1] * 100) / cntTotalCu[1]);
        fprintf(fp, "16x16 : Intra [%.2f %%] Skipped [%.2f %%]\n", (double)(cntIntra[2] * 100) / cntTotalCu[2], (double)(cntSkipCu[2] * 100) / cntTotalCu[2]);
        fprintf(fp, "8x8   : Intra [%.2f %%] Skipped [%.2f %%]\n", (double)(cntIntra[3] * 100) / cntTotalCu[3], (double)(cntSkipCu[3] * 100) / cntTotalCu[3]);
    }
#endif // if LOGGING

    // wait for loop filter completion
    if (m_cfg->param.bEnableLoopFilter)
    {
        m_frameFilter.wait();
        m_frameFilter.dequeue();
    }

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

            m_seiWriter.writeSEImessage(nalu.m_Bitstream, seiGradualDecodingRefreshInfo, slice->getSPS());
            writeRBSPTrailingBits(nalu.m_Bitstream);
            m_accessUnit.push_back(new NALUnitEBSP(nalu));
        }
        // Recovery point SEI
        OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI);

        SEIRecoveryPoint sei_recovery_point;
        sei_recovery_point.m_recoveryPocCnt    = 0;
        sei_recovery_point.m_exactMatchingFlag = (slice->getPOC() == 0) ? (true) : (false);
        sei_recovery_point.m_brokenLinkFlag    = false;

        m_seiWriter.writeSEImessage(nalu.m_Bitstream, sei_recovery_point, slice->getSPS());
        writeRBSPTrailingBits(nalu.m_Bitstream);
        m_accessUnit.push_back(new NALUnitEBSP(nalu));
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

    entropyCoder->setEntropyCoder(&m_cavlcCoder, slice);
    entropyCoder->resetEntropy();

    /* start slice NALunit */
    OutputNALUnit nalu(slice->getNalUnitType(), 0);
    bool sliceSegment = !slice->isNextSlice();
    entropyCoder->setBitstream(&nalu.m_Bitstream);
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

    m_sbacCoder.init((TEncBinIf*)&m_binCoderCABAC);
    entropyCoder->setEntropyCoder(&m_sbacCoder, slice);
    entropyCoder->resetEntropy();
    resetEntropy(slice);

    if (slice->isNextSlice())
    {
        // set entropy coder for writing
        m_sbacCoder.init((TEncBinIf*)&m_binCoderCABAC);
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
            entropyCoder->setBitstream(&nalu.m_Bitstream);
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
        // The final bitstream is either nalu.m_Bitstream or pcBitstreamRedirect;
        UInt* substreamSizes = slice->getSubstreamSizes();
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
        entropyCoder->setEntropyCoder(&m_cavlcCoder, slice);
        entropyCoder->setBitstream(&nalu.m_Bitstream);
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
    nalu.m_Bitstream.writeByteAlignment(); // Slice header byte-alignment

    // Perform bitstream concatenation
    if (bitstreamRedirect->getNumberOfWrittenBits() > 0)
    {
        nalu.m_Bitstream.addSubstream(bitstreamRedirect);
    }
    entropyCoder->setBitstream(&nalu.m_Bitstream);
    bitstreamRedirect->clear();

    m_accessUnit.push_back(new NALUnitEBSP(nalu));

    if (m_sps.getUseSAO())
    {
        m_frameFilter.end();
        m_pic->destroyNonDBFilterInfo();
    }
    m_pic->compressMotion();

    /* Decrement referenced frame reference counts, allow them to be recycled */
    for (int l = 0; l < numPredDir; l++)
    {
        RefPicList list = (l ? REF_PIC_LIST_1 : REF_PIC_LIST_0);
        for (int ref = 0; ref < slice->getNumRefIdx(list); ref++)
        {
            TComPic *refpic = slice->getRefPic(list, ref);
            ATOMIC_DEC(&refpic->m_countRefEncoders);
        }
    }

    delete[] outStreams;
    delete bitstreamRedirect;
}

void FrameEncoder::encodeSlice(TComOutputBitstream* substreams)
{
    PPAScopeEvent(FrameEncoder_encodeSlice);

    // choose entropy coder
    TEncEntropy *entropyCoder = getEntropyCoder(0);
    TComSlice* slice = m_pic->getSlice();

    resetEncoder();
    getCuEncoder(0)->setBitCounter(NULL);
    entropyCoder->setEntropyCoder(&m_sbacCoder, slice);

    UInt cuAddr;
    UInt startCUAddr = 0;
    UInt boundingCUAddr = slice->getSliceCurEndCUAddr();

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
    const UInt heightInLCUs = m_pic->getPicSym()->getFrameHeightInCU();
    const int  numSubstreams = (bWaveFrontsynchro ? heightInLCUs : 1);
    UInt bitsOriginallyInSubstreams = 0;

    for (int substrmIdx = 0; substrmIdx < numSubstreams; substrmIdx++)
    {
        getBufferSBac(substrmIdx)->loadContexts(&m_sbacCoder); //init. state
        bitsOriginallyInSubstreams += substreams[substrmIdx].getNumberOfWrittenBits();
    }

    UInt widthInLCUs = m_pic->getPicSym()->getFrameWidthInCU();
    UInt col = 0, lin = 0, subStrm = 0;
    cuAddr = (startCUAddr / m_pic->getNumPartInCU()); /* for tiles, startCUAddr is NOT the real raster scan address, it is actually
                                                       an encoding order index, so we need to convert the index (startCUAddr)
                                                       into the real raster scan address (cuAddr) via the CUOrderMap */
    UInt encCUOrder;
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
            UInt widthInCU = m_pic->getFrameWidthInCU();
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
    UInt numberOfCUsInFrame = m_pic->getNumCUsInFrame();
    UInt boundingCUAddrSlice = numberOfCUsInFrame * m_pic->getNumPartInCU();

    // WPP: if a slice does not start at the beginning of a CTB row, it must end within the same CTB row
    m_pic->getSlice()->setSliceCurEndCUAddr(boundingCUAddrSlice);
}

void FrameEncoder::compressCTURows()
{
    // reset entropy coders
    m_sbacCoder.init(&m_binCoderCABAC);
    for (int i = 0; i < this->m_numRows; i++)
    {
        m_rows[i].init();
        m_rows[i].m_entropyCoder.setEntropyCoder(&m_sbacCoder, m_pic->getSlice());
        m_rows[i].m_entropyCoder.resetEntropy();

        m_rows[i].m_rdSbacCoders[0][CI_CURR_BEST]->load(&m_sbacCoder);

        m_pic->m_complete_enc[i] = 0;
    }

    m_referenceRowsAvailable = 0;

    if (m_pool && m_cfg->param.bEnableWavefront)
    {
        WaveFront::clearEnabledRowMask();
        WaveFront::enqueue();

        m_frameFilter.start(m_pic);

        TComSlice* slice = m_pic->getSlice();
        int numPredDir = slice->isInterP() ? 1 : slice->isInterB() ? 2 : 0;
        for (int row = 0; row < m_numRows; row++)
        {
            UInt min = m_numRows;
            for (int l = 0; l < numPredDir; l++)
            {
                RefPicList list = (l ? REF_PIC_LIST_1 : REF_PIC_LIST_0);
                for (int ref = 0; ref < slice->getNumRefIdx(list); ref++)
                {
                    TComPic *refpic = slice->getRefPic(list, ref);
                    while ((refpic->m_reconRowCount != (UInt)m_numRows) && (refpic->m_reconRowCount < (UInt)row + 2))
                    {
                        refpic->m_reconRowWait.wait();
                    }

                    min = X265_MIN(min, refpic->m_reconRowCount);
                }
            }

            m_referenceRowsAvailable = min;

            WaveFront::enableRow(row);
        }

        WaveFront::enqueueRow(0);

        m_completionEvent.wait();

        WaveFront::dequeue();
    }
    else
    {
        for (int i = 0; i < this->m_numRows; i++)
        {
            processRow(i);
        }

        m_frameFilter.start(m_pic);

        if (m_cfg->param.bEnableLoopFilter)
        {
            for (int i = 0; i < this->m_numRows; i++)
            {
                m_frameFilter.processRow(i);
            }
        }
    }
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

        codeRow.m_search.m_referenceRowsAvailable = m_referenceRowsAvailable;
        TEncSbac *bufSbac = (m_cfg->param.bEnableWavefront && col == 0 && row > 0) ? &m_rows[row - 1].m_bufferSbacCoder : NULL;
        codeRow.processCU(cu, m_pic->getSlice(), bufSbac, m_cfg->param.bEnableWavefront && col == 1);

        // TODO: Keep atomic running totals for rate control?
        // cu->m_totalBits;
        // cu->m_totalCost;
        // cu->m_totalDistortion;

        // Completed CU processing
        m_pic->m_complete_enc[row]++;

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

    // Active Loopfilter
    if (row >= row_delay)
    {
        // NOTE: my version, it need check active flag
        m_frameFilter.enqueueRow(row - row_delay);
    }

    // this row of CTUs has been encoded
    if (row == m_numRows - 1)
    {
        // NOTE: I ignore (row-1) because enqueueRow is replace operator
        m_frameFilter.enqueueRow(row);
        m_completionEvent.trigger();
    }
}

TComPic *FrameEncoder::getEncodedPicture(AccessUnit& accessUnit)
{
    if (m_pic)
    {
        /* TODO: frame parallelism - block here until worker thread completes */
        m_done.wait();

        TComPic *ret = m_pic;
        m_pic = NULL;

        // move NALs from member variable list to end of user's container
        accessUnit.splice(accessUnit.end(), m_accessUnit);
        return ret;
    }

    return NULL;
}
