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
    setUseRectInter(param->enableRectInter);

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
    setProfile(m_profile);
    setLevel(m_levelTier, m_level);

    setGOPSize(m_iGOPSize);
    setGopList(m_GOPList);
    setExtraRPSs(m_extraRPSs);
    for (int i = 0; i < MAX_TLAYER; i++)
    {
        setNumReorderPics(m_numReorderPics[i], i);
        setMaxDecPicBuffering(m_maxDecPicBuffering[i], i);
        setLambdaModifier(i, 1.0);
    }

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
    setMinSpatialSegmentationIdc(0);
    setAspectRatioIdc(0);
    setSarWidth(0);
    setSarHeight(0);
    setOverscanInfoPresentFlag(0);
    setOverscanAppropriateFlag(0);
    setVideoSignalTypePresentFlag(0);
    setVideoFormat(5);
    setVideoFullRangeFlag(0);
    setColourDescriptionPresentFlag(0);
    setColourPrimaries(2);
    setTransferCharacteristics(2);
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
#include "input/input.h"
#include "output/output.h"
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
    x265::ThreadPool *threadPool;
    fstream bitstreamFile;

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
    bool do_help = false;
    int cpuid = 0;
    int threadcount = 0;
    const char *inputfn = NULL, *reconfn = NULL, *bitstreamfn = NULL;

#if 0
    ("help", do_help, false, "this help text")
    ("cpuid",                 cpuid,               0, "SIMD architecture. 2:MMX2 .. 8:AVX2 (default:0-auto)")
    ("threads",               threadcount,         0, "Number of threads for thread pool (default:CPU HT core count)")
    // File, I/O and source parameters
    ("InputFile,i",           inputfn,                 "", "Original YUV input file name")
    ("BitstreamFile,b",       bitstreamfn,     "hevc.bin", "Bitstream output file name")
    ("ReconFile,o",           reconfn,                 "", "Reconstructed YUV output file name")
    ("InputBitDepth",         cliopt->inputBitDepth,     8, "Bit-depth of input file")
    ("OutputBitDepth",        cliopt->outputBitDepth,    0, "Bit-depth of output file (default:InternalBitDepth)")
    ("FrameSkip,-fs",         cliopt->frameSkip,         0u, "Number of frames to skip at start of input YUV")
    ("FramesToBeEncoded,f",   cliopt->framesToBeEncoded, 0, "Number of frames to be encoded (default=all)")


    ("SourceWidth,-wdt",      param->iSourceWidth,      0, "Source picture width")
    ("SourceHeight,-hgt",     param->iSourceHeight,     0, "Source picture height")
    ("FrameRate,-fr",         param->iFrameRate,        0, "Frame rate")
    ("InternalBitDepth",      param->internalBitDepth,  0, "Bit-depth the codec operates at. (default:InputBitDepth)"
    "If different to InputBitDepth, source data will be converted")

    ("MaxCUSize,s",             param->uiMaxCUSize,              64u, "Maximum CU size")
    ("MaxPartitionDepth,h",     param->uiMaxCUDepth,              4u, "CU depth")

    ("QuadtreeTULog2MaxSize",   param->uiQuadtreeTULog2MaxSize,   6u, "Maximum TU size in logarithm base 2")
    ("QuadtreeTULog2MinSize",   param->uiQuadtreeTULog2MinSize,   2u, "Minimum TU size in logarithm base 2")

    ("QuadtreeTUMaxDepthIntra", param->uiQuadtreeTUMaxDepthIntra, 1u, "Depth of TU tree for intra CUs")
    ("QuadtreeTUMaxDepthInter", param->uiQuadtreeTUMaxDepthInter, 2u, "Depth of TU tree for inter CUs")

    // Coding structure parameters
    ("IntraPeriod,-ip",         param->iIntraPeriod,              -1, "Intra period in frames, (-1: only first frame)")

    // motion options
    ("SearchMethod,-me",        param->searchMethod,               3, "0:DIA 1:HEX 2:UMH 3:HM 4:ORIG")
    ("SearchRange,-sr",         param->iSearchRange,              96, "Motion search range")
    ("BipredSearchRange",       param->bipredSearchRange,          4, "Motion search range for bipred refinement")

    /* Quantization parameters */
    ("MaxCuDQPDepth,-dqd",   param->iMaxCuDQPDepth,    0u, "max depth for a minimum CuDQP")

    ("CbQpOffset,-cbqpofs",  param->cbQpOffset,        0, "Chroma Cb QP Offset")
    ("CrQpOffset,-crqpofs",  param->crQpOffset,        0, "Chroma Cr QP Offset")

    ("AdaptiveQpSelection,-aqps",     param->bUseAdaptQpSelect,        0, "AdaptiveQpSelection")

    ("AdaptiveQP,-aq",                param->bUseAdaptiveQP,           0, "QP adaptation based on a psycho-visual model")
    ("MaxQPAdaptationRange,-aqr",     param->iQPAdaptationRange,       6, "QP adaptation range")
    ("RDOQ",                          param->useRDOQ,                  1)
    ("RDOQTS",                        param->useRDOQTS,                1)
    ("RDpenalty",                     param->rdPenalty,                0,  "RD-penalty for 32x32 TU for intra in non-intra slices. 0:disbaled  1:RD-penalty  2:maximum RD-penalty")

    // Deblocking filter parameters
    ("LoopFilterDisable",              param->bLoopFilterDisable,             0)
    ("LoopFilterOffsetInPPS",          param->loopFilterOffsetInPPS,          0)
    ("LoopFilterBetaOffset_div2",      param->loopFilterBetaOffsetDiv2,       0)
    ("LoopFilterTcOffset_div2",        param->loopFilterTcOffsetDiv2,         0)
    ("DeblockingFilterControlPresent", param->DeblockingFilterControlPresent, 0)
    ("DeblockingFilterMetric",         param->DeblockingFilterMetric,         0)

    // Coding tools
    ("AMP",                      param->enableAMP,                 1,  "Enable asymmetric motion partitions")
    ("RectInter",                param->enableRectInter,           1,  "Enable rectangular motion partitions Nx2N and 2NxN, disabling also disables AMP")
    ("TransformSkip",            param->useTransformSkip,          0,  "Intra transform skipping")
    ("TransformSkipFast",        param->useTransformSkipFast,      0,  "Fast intra transform skipping")
    ("SAO",                      param->bUseSAO,                   1,  "Enable Sample Adaptive Offset")
    ("MaxNumOffsetsPerPic",      param->maxNumOffsetsPerPic,    2048,  "Max number of SAO offset per picture (Default: 2048)")
    ("SAOLcuBoundary",           param->saoLcuBoundary,            0,  "0: right/bottom LCU boundary areas skipped from SAO parameter estimation, 1: non-deblocked pixels are used for those areas")
    ("SAOLcuBasedOptimization",  param->saoLcuBasedOptimization,   1,  "0: SAO picture-based optimization, 1: SAO LCU-based optimization ")

    ("ConstrainedIntraPred",     param->bUseConstrainedIntraPred,  0, "Constrained Intra Prediction")

    ("PCMEnabledFlag",           param->usePCM,                    0)
    ("PCMLog2MaxSize",           param->pcmLog2MaxSize,            5u)
    ("PCMLog2MinSize",           param->uiPCMLog2MinSize,          3u)
    ("PCMInputBitDepthFlag",     param->bPCMInputBitDepthFlag,     1)
    ("PCMFilterDisableFlag",     param->bPCMFilterDisableFlag,     0)

    ("LosslessCuEnabled",        param->useLossless,               0)

    ("WeightedPredP,-wpP",       param->useWeightedPred,               0,          "Use weighted prediction in P slices")
    ("WeightedPredB,-wpB",       param->useWeightedBiPred,             0,          "Use weighted (bidirectional) prediction in B slices")
    ("Log2ParallelMergeLevel",   param->log2ParallelMergeLevel,       2u,          "Parallel merge estimation region")
    ("WaveFrontSynchro",         param->iWaveFrontSynchro,             0,          "0: no synchro; 1 synchro with TR; 2 TRR etc")
    ("SignHideFlag,-SBH",        param->signHideFlag, 1)
    ("MaxNumMergeCand",          param->maxNumMergeCand,               5u,         "Maximum number of merge candidates")

    ("TMVPMode", param->TMVPModeId, 1, "TMVP mode 0: TMVP disable for all slices. 1: TMVP enable for all slices (default) 2: TMVP enable for certain slices only")
    ("FDM", param->useFastDecisionForMerge, 1, "Fast decision for Merge RD Cost")
    ("CFM", param->bUseCbfFastMode, 0, "Cbf fast mode setting")
    ("ESD", param->useEarlySkipDetection, 0, "Early SKIP detection setting")

    ("RateControl",         param->RCEnableRateControl,       0, "Rate control: enable rate control")
    ("TargetBitrate",       param->RCTargetBitrate,           0, "Rate control: target bitrate")
    ("KeepHierarchicalBit", param->RCKeepHierarchicalBit,     0, "Rate control: keep hierarchical bit allocation in rate control algorithm")
    ("LCULevelRateControl", param->RCLCULevelRC,              1, "Rate control: true: LCU level RC; false: picture level RC")
    ("RCLCUSeparateModel",  param->RCUseLCUSeparateModel,     1, "Rate control: use LCU level separate R-lambda model")
    ("InitialQP",           param->RCInitialQP,               0, "Rate control: initial QP")
    ("RCForceIntraQP",      param->RCForceIntraQP,            0, "Rate control: force intra QP to be equal to initial QP")

    ("TransquantBypassEnableFlag",     param->TransquantBypassEnableFlag,         0, "transquant_bypass_enable_flag indicator in PPS")
    ("CUTransquantBypassFlagValue",    param->CUTransquantBypassFlagValue,        0, "Fixed cu_transquant_bypass_flag value, when transquant_bypass_enable_flag is enabled")
    ("StrongIntraSmoothing,-sis",      param->useStrongIntraSmoothing,            1, "Enable strong intra smoothing for 32x32 blocks")
    ;
#endif
    if (argc == 1 || do_help)
    {
        /* argc == 1: no options have been specified */
        return false;
    }

    x265::SetupPrimitives(cpuid);
    cliopt->threadPool = x265::ThreadPool::AllocThreadPool(threadcount);

    /* parse the width, height, frame rate from the y4m files if it is not given in the configuration file */
    cliopt->input = x265::Input::Open(inputfn);
    if (!cliopt->input || cliopt->input->isFail())
    {
        printf("Unable to open source file\n");
        return true;
    }
    if (cliopt->input->getWidth())
    {
        param->iSourceWidth = cliopt->input->getWidth();
        param->iSourceHeight = cliopt->input->getHeight();
        param->iFrameRate = (int)cliopt->input->getRate();
        cliopt->inputBitDepth = 8;
    }
    else
    {
        cliopt->input->setDimensions(param->iSourceWidth, param->iSourceHeight);
        cliopt->input->setBitDepth(cliopt->inputBitDepth);
    }

    /* rules for input, output and internal bitdepths as per help text */
    if (!param->internalBitDepth) { param->internalBitDepth = cliopt->inputBitDepth; }
    if (!cliopt->outputBitDepth) { cliopt->outputBitDepth = param->internalBitDepth; }

    uint32_t numRemainingFrames = (uint32_t)cliopt->input->guessFrameCount();

    if (cliopt->frameSkip)
    {
        cliopt->input->skipFrames(cliopt->frameSkip);
    }

    cliopt->framesToBeEncoded = cliopt->framesToBeEncoded ? min(cliopt->framesToBeEncoded, numRemainingFrames) : numRemainingFrames;

    printf("Input File                   : %s (%d total frames)\n", inputfn, numRemainingFrames);

    if (reconfn)
    {
        printf("Reconstruction File          : %s\n", reconfn);
        cliopt->recon = x265::Output::Open(reconfn, param->iSourceWidth, param->iSourceHeight, cliopt->outputBitDepth, param->iFrameRate);
        if (cliopt->recon->isFail())
        {
            printf("Unable to write reconstruction file\n");
            cliopt->recon->release();
            cliopt->recon = 0;
        }
    }

#if !HIGH_BIT_DEPTH
    if (cliopt->inputBitDepth != 8 || cliopt->outputBitDepth != 8 || param->internalBitDepth != 8)
    {
        printf("x265 not compiled for bit depths greater than 8\n");
        return true;
    }
#endif

    printf("Bitstream File               : %s\n", bitstreamfn);
    printf("Frame index                  : %u - %d (%d frames)\n", cliopt->frameSkip, cliopt->frameSkip + cliopt->framesToBeEncoded - 1, cliopt->framesToBeEncoded);

    //    printf("GOP size                     : %d\n", m_iGOPSize);

    cliopt->bitstreamFile.open(bitstreamfn, fstream::binary | fstream::out);
    if (!cliopt->bitstreamFile)
    {
        fprintf(stderr, "failed to open bitstream file <%s> for writing\n", bitstreamfn);
        return true;
    }

    return false;
}

void new_main(int argc, char **argv)
{
    x265_param_t param;
    CLIOptions   cliopt;

    x265_param_default(&param);

    if (parse(argc, argv, &param, &cliopt))
        exit(1);

    x265_set_globals(&param, cliopt.inputBitDepth);

    if (x265_check_params(&param))
        exit(1);

    x265_print_params(&param);

    x265_t *encoder = x265_encoder_open(&param);
    if (!encoder)
    {
        fprintf(stderr, "x265: failed to open encoder\n");
        exit(1);
    }

    TComList<TComPicYuv *> cListPicYuvRec; ///< list of reconstructed YUV files
    list<AccessUnit> outputAccessUnits;    ///< list of access units to write out, populated by the encoder5_t

    // main encoder loop
    uint32_t iFrameRcvd = 0;
    bool bEos = false;
    while (!bEos)
    {
        TComPicYuv *pcPicYuvRec = NULL;
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
        bool nopic = false;
        if (cliopt.input->readPicture(pic))
        {
            iFrameRcvd++;
            bEos = (iFrameRcvd == cliopt.framesToBeEncoded);
        }
        else
        {
            nopic = true;
            bEos = true;
        }

        Int iNumEncoded = encoder->encode(bEos, nopic ? 0 : &pic, cListPicYuvRec, outputAccessUnits);

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

            for (i = 0; i < iNumEncoded; i++)
            {
                if (cliopt.recon)
                {
                    x265_picture_t rpic;
                    TComPicYuv  *recpic  = *(iterPicYuvRec++);
                    rpic.planes[0] = recpic->getLumaAddr(); rpic.stride[0] = recpic->getStride();
                    rpic.planes[1] = recpic->getCbAddr();   rpic.stride[1] = recpic->getCStride();
                    rpic.planes[2] = recpic->getCrAddr();   rpic.stride[2] = recpic->getCStride();
                    rpic.bitDepth = sizeof(Pel)*8;
                    cliopt.recon->writePicture(rpic);
                }

                const AccessUnit &au = *(iterBitstream++);
                const vector<UInt>& stats = writeAnnexB(cliopt.bitstreamFile, au);
                cliopt.rateStatsAccum(au, stats);
            }
            outputAccessUnits.clear();
            PPAStopCpuEventFunc(bitstream_write);
        }
    }

    encoder->printSummary();
    cliopt.bitstreamFile.close();

    double time = (double)iFrameRcvd / param.iFrameRate;
    printf("Bytes written to file: %u (%.3f kbps)\n", cliopt.totalBytes, 0.008 * cliopt.totalBytes / time);

    x265_encoder_close(encoder);

    TComList<TComPicYuv *>::iterator iterPicYuvRec = cListPicYuvRec.begin();
    size_t iSize = cListPicYuvRec.size();
    for (size_t i = 0; i < iSize; i++)
    {
        TComPicYuv *recpic = *(iterPicYuvRec++);
        recpic->destroy();
        delete recpic;
    }
}
