/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
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

#include "x265.h"
#include "common.h"
#include "encoder.h"
#include "TLibEncoder/TEncSlice.h"
#include "TLibCommon/AccessUnit.h"

#include <stdio.h>
#include <string.h>

struct x265_t : public x265::Encoder
{

};

using namespace x265;

#if _MSC_VER
#pragma warning(disable: 4800) // 'int' : forcing value to bool 'true' or 'false' (performance warning)
#pragma warning(disable: 4100) // unused formal parameters (temporary)
#endif

void Encoder::configure(x265_param_t *param)
{
    setFrameRate(param->iFrameRate);
    setSourceWidth(param->iSourceWidth);
    setSourceHeight(param->iSourceHeight);
    setIntraPeriod(param->iIntraPeriod);
    setQP(param->iQP);
    setUseAMP(param->enableAMP);
    setUseAMPRefine(param->enableAMPRefine);

    //====== Loop/Deblock Filter ========
    setLoopFilterDisable(param->bLoopFilterDisable);
    setLoopFilterOffsetInPPS(param->loopFilterOffsetInPPS);
    setLoopFilterBetaOffset(param->loopFilterBetaOffsetDiv2);
    setLoopFilterTcOffset(param->loopFilterTcOffsetDiv2);
    setDeblockingFilterControlPresent(param->DeblockingFilterControlPresent);
    setDeblockingFilterMetric(param->DeblockingFilterMetric);

    //====== Motion search ========
    setSearchMethod(param->searchMethod);
    setSearchRange(param->iSearchRange);
    setBipredSearchRange(param->bipredSearchRange);

    //====== Quality control ========
    setMaxCuDQPDepth(param->iMaxCuDQPDepth);
    setChromaCbQpOffset(param->cbQpOffset);
    setChromaCrQpOffset(param->crQpOffset);
    setUseAdaptQpSelect(param->bUseAdaptQpSelect);

    assert(g_bitDepthY);
    int lowestQP = -6 * (g_bitDepthY - 8); // XXX: check
    if ((param->iQP == lowestQP) && param->useLossless)
    {
        param->bUseAdaptiveQP = 0;
    }
    setUseAdaptiveQP(param->bUseAdaptiveQP);
    setQPAdaptationRange(param->iQPAdaptationRange);

    //====== Coding Tools ========
    setUseLossless(param->useLossless);
    setUseRDOQ(param->useRDOQ);
    setUseRDOQTS(param->useRDOQTS);
    setRDpenalty(param->rdPenalty);
    setQuadtreeTULog2MaxSize(param->uiQuadtreeTULog2MaxSize);
    setQuadtreeTULog2MinSize(param->uiQuadtreeTULog2MinSize);
    setQuadtreeTUMaxDepthInter(param->uiQuadtreeTUMaxDepthInter);
    setQuadtreeTUMaxDepthIntra(param->uiQuadtreeTUMaxDepthIntra);
    setUseFastDecisionForMerge(param->useFastDecisionForMerge);
    setUseCbfFastMode(param->bUseCbfFastMode);
    setUseEarlySkipDetection(param->useEarlySkipDetection);
    setUseTransformSkip(param->useTransformSkip);
    setUseTransformSkipFast(param->useTransformSkipFast);
    setUseConstrainedIntraPred(param->bUseConstrainedIntraPred);
    setPCMLog2MinSize(param->uiPCMLog2MinSize);
    setUsePCM(param->usePCM);
    setPCMLog2MaxSize(param->pcmLog2MaxSize);
    setMaxNumMergeCand(param->maxNumMergeCand);
    setUseSAO(param->bUseSAO);
    setMaxNumOffsetsPerPic(param->maxNumOffsetsPerPic);
    setSaoLcuBoundary(param->saoLcuBoundary);
    setSaoLcuBasedOptimization(param->saoLcuBasedOptimization);
    setPCMInputBitDepthFlag(param->bPCMInputBitDepthFlag);
    setPCMFilterDisableFlag(param->bPCMFilterDisableFlag);

    //====== Parallel Merge Estimation ========
    setLog2ParallelMergeLevelMinus2(param->log2ParallelMergeLevel - 2);

    //====== Weighted Prediction ========
    setUseWP(param->useWeightedPred);
    setWPBiPred(param->useWeightedBiPred);

    setWaveFrontSynchro(param->iWaveFrontSynchro);

    setTMVPModeId(param->TMVPModeId);
    setSignHideFlag(param->signHideFlag);

    setUseRateCtrl(param->RCEnableRateControl);
    setTargetBitrate(param->RCTargetBitrate);
    setKeepHierBit(param->RCKeepHierarchicalBit);
    setLCULevelRC(param->RCLCULevelRC);
    setUseLCUSeparateModel(param->RCUseLCUSeparateModel);
    setInitialQP(param->RCInitialQP);
    setForceIntraQP(param->RCForceIntraQP);

    setTransquantBypassEnableFlag(param->TransquantBypassEnableFlag);
    setCUTransquantBypassFlagValue(param->CUTransquantBypassFlagValue);
    setUseStrongIntraSmoothing(param->useStrongIntraSmoothing);


    //====== Settings derived from user configuration ======
    setFramesToBeEncoded(m_framesToBeEncoded);
    setFrameSkip(m_FrameSkip);
    setProfile(m_profile);
    setLevel(m_levelTier, m_level);

    setGOPSize(m_iGOPSize);
    setGopList(m_GOPList);
    setExtraRPSs(m_extraRPSs);
    for (int i = 0; i < MAX_TLAYER; i++)
    {
        setNumReorderPics(m_numReorderPics[i], i);
        setMaxDecPicBuffering(m_maxDecPicBuffering[i], i);
        setLambdaModifier(i, m_adLambdaModifier[i]);
    }
    setMinSpatialSegmentationIdc(m_minSpatialSegmentationIdc);

    TComVPS vps;
    vps.setMaxTLayers(m_maxTempLayer);
    vps.setTemporalNestingFlag(m_maxTempLayer > 0);
    vps.setMaxLayers(1);
    for (Int i = 0; i < MAX_TLAYER; i++)
    {
        vps.setNumReorderPics(m_numReorderPics[i], i);
        vps.setMaxDecPicBuffering(m_maxDecPicBuffering[i], i);
    }
    setVPS(&vps);
    setMaxTempLayer(m_maxTempLayer);


    //====== HM Settings not exposed for configuration ======
    setConformanceWindow(0, 0, 0, 0);
    int nullpad[2] = { 0, 0 };
    setPad(nullpad);
    setProgressiveSourceFlag(1);
    setInterlacedSourceFlag(0);
    setNonPackedConstraintFlag(0);
    setFrameOnlyConstraintFlag(0);
    setDecodingRefreshType(0);
    setUseASR(0);
    setUseHADME(1);
    setdQPs(NULL);
    setDecodedPictureHashSEIEnabled(0);
    setRecoveryPointSEIEnabled(0);
    setBufferingPeriodSEIEnabled(0);
    setPictureTimingSEIEnabled(0);
    setDisplayOrientationSEIAngle(0);
    setTemporalLevel0IndexSEIEnabled(0);
    setGradualDecodingRefreshInfoEnabled(0);
    setDecodingUnitInfoSEIEnabled(0);
    setSOPDescriptionSEIEnabled(0);
    setScalableNestingSEIEnabled(0);
    setUseScalingListId(0);
    setScalingListFile(NULL);
    setUseRecalculateQPAccordingToLambda(0);
    setActiveParameterSetsSEIEnabled(0);
    setVuiParametersPresentFlag(0);
    setAspectRatioIdc(0);
    setSarWidth(0);
    setSarHeight(0);
    setOverscanInfoPresentFlag(0);
    setOverscanAppropriateFlag(0);
    setVideoSignalTypePresentFlag(0);
    setVideoFormat(0);
    setVideoFullRangeFlag(0);
    setColourDescriptionPresentFlag(0);
    setColourPrimaries(0);
    setTransferCharacteristics(0);
    setMatrixCoefficients(2);
    setChromaLocInfoPresentFlag(0);
    setChromaSampleLocTypeTopField(0);
    setChromaSampleLocTypeBottomField(0);
    setNeutralChromaIndicationFlag(0);
    setDefaultDisplayWindow(0, 0, 0, 0);
    setFrameFieldInfoPresentFlag(0);
    setPocProportionalToTimingFlag(0);
    setNumTicksPocDiffOneMinus1(0);
    setBitstreamRestrictionFlag(0);
    setMotionVectorsOverPicBoundariesFlag(0);
    setMaxBytesPerPicDenom(2);
    setMaxBitsPerMinCuDenom(1);
    setLog2MaxMvLengthHorizontal(15);
    setLog2MaxMvLengthVertical(15);
}

extern "C"
x265_t *x265_encoder_open(x265_param_t *param)
{
    if (x265_check_params(param))
        return NULL;

    x265_t *encoder = new x265_t;
    if (encoder)
    {
        encoder->configure(param);
        encoder->create();
        encoder->init();
    }

    return encoder;
}

extern "C"
int x265_encoder_encode(x265_t *encoder, x265_nal_t **pp_nal, int *pi_nal, x265_picture_t *pic_in)
{
    return 0;
}

extern "C"
void x265_encoder_close(x265_t *encoder)
{
    // delete used buffers in encoder class
    encoder->deletePicBuffer();
    encoder->destroy();
    delete encoder;
}

/*======= Everything below here will become the new x265main.cpp ==========*/

#include "TLibEncoder/AnnexBwrite.h"
#include <getopt.h>
#include <list>
#include <ostream>
#include <fstream>
#include "PPA/ppa.h"

using namespace std;

struct CLIOptions
{
    x265::Input*  input;
    x265::Output* recon;
    char* outputFileName;               ///< output bit-stream file
    x265::ThreadPool *threadPool;

    uint32_t inputBitDepth;             ///< bit-depth of input file
    uint32_t outputBitDepth;            ///< bit-depth of output file
    uint32_t frameSkip;                 ///< number of skipped frames from the beginning
    uint32_t framesToBeEncoded;         ///< number of encoded frames
    uint32_t essentialBytes;
    uint32_t totalBytes;

    CLIOptions()
    {
        input = NULL;
        recon = NULL;
        outputFileName = NULL;
        threadPool = NULL;
        inputBitDepth = outputBitDepth = 8;
        framesToBeEncoded = frameSkip = 0;
        essentialBytes = 0;
        totalBytes = 0;
    }

    void destroy()
    {
        if (threadPool)
            threadPool->Release();
        threadPool = NULL;
        if (input)
            input->release();
        input = NULL;
        if (recon)
            recon->release();
        recon = NULL;
    }

    void rateStatsAccum(const AccessUnit &au, const vector<UInt>& annexBsizes)
    {
        AccessUnit::const_iterator it_au = au.begin();

        vector<UInt>::const_iterator it_stats = annexBsizes.begin();

        for (; it_au != au.end(); it_au++, it_stats++)
        {
            switch ((*it_au)->m_nalUnitType)
            {
            case NAL_UNIT_CODED_SLICE_TRAIL_R:
            case NAL_UNIT_CODED_SLICE_TRAIL_N:
            case NAL_UNIT_CODED_SLICE_TLA_R:
            case NAL_UNIT_CODED_SLICE_TSA_N:
            case NAL_UNIT_CODED_SLICE_STSA_R:
            case NAL_UNIT_CODED_SLICE_STSA_N:
            case NAL_UNIT_CODED_SLICE_BLA_W_LP:
            case NAL_UNIT_CODED_SLICE_BLA_W_RADL:
            case NAL_UNIT_CODED_SLICE_BLA_N_LP:
            case NAL_UNIT_CODED_SLICE_IDR_W_RADL:
            case NAL_UNIT_CODED_SLICE_IDR_N_LP:
            case NAL_UNIT_CODED_SLICE_CRA:
            case NAL_UNIT_CODED_SLICE_RADL_N:
            case NAL_UNIT_CODED_SLICE_RADL_R:
            case NAL_UNIT_CODED_SLICE_RASL_N:
            case NAL_UNIT_CODED_SLICE_RASL_R:
            case NAL_UNIT_VPS:
            case NAL_UNIT_SPS:
            case NAL_UNIT_PPS:
                essentialBytes += *it_stats;
                break;
            default:
                break;
            }

            totalBytes += *it_stats;
        }
    }
};

bool parse(int argc, char **argv, x265_param_t* param, CLIOptions* cliopt)
{
    return true;
}

void new_main(int argc, char **argv)
{
    x265_param_t param;
    CLIOptions   cliopt;

    x265_param_default(&param);

    if (parse(argc, argv, &param, &cliopt))
        exit(1);

    fstream bitstreamFile(cliopt.outputFileName, fstream::binary | fstream::out);
    if (!bitstreamFile)
    {
        fprintf(stderr, "failed to open bitstream file <%s> for writing\n", cliopt.outputFileName);
        exit(1);
    }

    TComPicYuv *pcPicYuvRec = NULL;
    TComList<TComPicYuv *> cListPicYuvRec; ///< list of reconstructed YUV files
    list<AccessUnit> outputAccessUnits;    ///< list of access units to write out, populated by the encoder5_t

    x265_t *encoder = x265_encoder_open(&param);
    if (!encoder)
    {
        fprintf(stderr, "x265: failed to open encoder\n");
        exit(1);
    }

    // main encoder loop
    uint32_t iFrameRcvd = 0;
    bool  bEos = false;
    while (!bEos)
    {
        if (cListPicYuvRec.size() == (UInt)encoder->m_iGOPSize)
        {
            pcPicYuvRec = cListPicYuvRec.popFront();
        }
        else
        {
            pcPicYuvRec = new TComPicYuv;

            pcPicYuvRec->create(param.iSourceWidth, param.iSourceHeight, param.uiMaxCUSize, param.uiMaxCUSize, param.uiMaxCUDepth);
        }

        cListPicYuvRec.pushBack(pcPicYuvRec);

        // read input YUV file
        x265_picture_t pic;
        bool flush = false;
        if (cliopt.input->readPicture(pic))
        {
            iFrameRcvd++;
            bEos = (iFrameRcvd == cliopt.framesToBeEncoded);
        }
        else
        {
            flush = true;
            bEos = true;
            encoder->setFramesToBeEncoded(iFrameRcvd);
        }

        PPAStartCpuEventFunc(encode_frame);
        Int iNumEncoded = 0;
        encoder->encode(bEos, flush ? 0 : &pic, cListPicYuvRec, outputAccessUnits, iNumEncoded);
        PPAStopCpuEventFunc(encode_frame);

        // write bitstream to file if necessary
        if (iNumEncoded > 0)
        {
            PPAStartCpuEventFunc(bitstream_write);
            Int i;

            TComList<TComPicYuv *>::iterator iterPicYuvRec = cListPicYuvRec.end();
            list<AccessUnit>::const_iterator iterBitstream = outputAccessUnits.begin();

            for (i = 0; i < iNumEncoded; i++)
            {
                --iterPicYuvRec;
            }

            x265_picture_t pic;
            for (i = 0; i < iNumEncoded; i++)
            {
                if (cliopt.recon)
                {
                    TComPicYuv  *pcPicYuvRec  = *(iterPicYuvRec++);
                    pic.planes[0] = pcPicYuvRec->getLumaAddr(); pic.stride[0] = pcPicYuvRec->getStride();
                    pic.planes[1] = pcPicYuvRec->getCbAddr();   pic.stride[1] = pcPicYuvRec->getCStride();
                    pic.planes[2] = pcPicYuvRec->getCrAddr();   pic.stride[2] = pcPicYuvRec->getCStride();
                    pic.bitDepth = sizeof(Pel)*8;
                    cliopt.recon->writePicture(pic);
                }

                const AccessUnit &au = *(iterBitstream++);
                const vector<UInt>& stats = writeAnnexB(bitstreamFile, au);
                cliopt.rateStatsAccum(au, stats);
            }
            outputAccessUnits.clear();
            PPAStopCpuEventFunc(bitstream_write);
        }
    }

    encoder->printSummary();

    double time = (double)iFrameRcvd / param.iFrameRate;
    printf("Bytes written to file: %u (%.3f kbps)\n", cliopt.totalBytes, 0.008 * cliopt.totalBytes / time);

    x265_encoder_close(encoder);

    TComList<TComPicYuv *>::iterator iterPicYuvRec = cListPicYuvRec.begin();
    size_t iSize = cListPicYuvRec.size();
    for (size_t i = 0; i < iSize; i++)
    {
        TComPicYuv *pcPicYuvRec = *(iterPicYuvRec++);
        pcPicYuvRec->destroy();
        delete pcPicYuvRec;
    }
}
