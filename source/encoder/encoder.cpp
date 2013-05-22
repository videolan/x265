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

struct x265_t : public x265::Encoder {};

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

    //====== Motion search ========
    setSearchMethod(param->searchMethod);
    setSearchRange(param->iSearchRange);
    setBipredSearchRange(param->bipredSearchRange);

    //====== Quality control ========
    setMaxCuDQPDepth(param->iMaxCuDQPDepth);
    setChromaCbQpOffset(param->cbQpOffset);
    setChromaCrQpOffset(param->crQpOffset);
    setUseAdaptQpSelect(param->bUseAdaptQpSelect);

    setUseAdaptiveQP(param->bUseAdaptiveQP);
    setQPAdaptationRange(param->iQPAdaptationRange);

    //====== Coding Tools ========
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
    setMaxNumMergeCand(param->maxNumMergeCand);
    setUseSAO(param->bUseSAO);
    setMaxNumOffsetsPerPic(param->maxNumOffsetsPerPic);
    setSaoLcuBoundary(param->saoLcuBoundary);
    setSaoLcuBasedOptimization(param->saoLcuBasedOptimization);

    //====== Parallel Merge Estimation ========
    setLog2ParallelMergeLevelMinus2(param->log2ParallelMergeLevel - 2);

    //====== Weighted Prediction ========
    setUseWP(param->useWeightedPred);
    setWPBiPred(param->useWeightedBiPred);

    setWaveFrontSynchro(param->iWaveFrontSynchro);

    setTMVPModeId(param->TMVPModeId);
    setSignHideFlag(param->signHideFlag);

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

    setUsePCM(0);
    setPCMLog2MinSize(3);
    setPCMLog2MaxSize(5);
    setPCMInputBitDepthFlag(1);
    setPCMFilterDisableFlag(0);

    setUseRateCtrl(0);
    setTargetBitrate(0);
    setKeepHierBit(0);
    setLCULevelRC(0);
    setUseLCUSeparateModel(0);
    setInitialQP(0);
    setForceIntraQP(0);

    setUseLossless(0); // x264 configures this via --qp=0

    setLoopFilterDisable(0);
    setLoopFilterOffsetInPPS(0);
    setLoopFilterBetaOffset(0);
    setLoopFilterTcOffset(0);
    setDeblockingFilterControlPresent(0);
    setDeblockingFilterMetric(0);

    setTransquantBypassEnableFlag(0);
    setCUTransquantBypassFlagValue(0);
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

#define OPT(longname, var, argreq, flag, helptext)

static const char short_options[] = "i:b:o:f:s:d:";
OPT("help",            help,                            no_argument,   0, "Show help text")
OPT("cpuid",           cpuid,                     required_argument,   0, "SIMD architecture. 2:MMX2 .. 8:AVX2 (default:0-auto)")
OPT("threads",         threadcount,               required_argument,   0, "Number of threads for thread pool (default:CPU HT core count)")
OPT("InputFile",       inputfn,                   required_argument, 'i', "Raw YUV or Y4M input file name")
OPT("BitstreamFile",   bitstreamfn,               required_argument, 'b', "Bitstream output file name")
OPT("ReconFile",       reconfn,                   required_argument, 'o', "Reconstructed YUV output file name")

OPT("InputBitDepth",   cliopt->inputBitDepth,     required_argument,   0, "Bit-depth of input file (default: 8)")
OPT("OutputBitDepth",  cliopt->outputBitDepth,    required_argument,   0, "Bit-depth of output file (default:InternalBitDepth)")
OPT("FrameSkip",       cliopt->frameSkip,         required_argument,   0, "Number of frames to skip at start of input YUV")
OPT("frames",          cliopt->framesToBeEncoded, required_argument, 'f', "Number of frames to be encoded (default=all)")

OPT("wpp",             param->iWaveFrontSynchro,        no_argument,   0, "0:no synchro 1:synchro with TR 2:TRR etc")
OPT("width",           param->iSourceWidth,       required_argument, 'w', "Source picture width")
OPT("height",          param->iSourceHeight,      required_argument, 'h', "Source picture height")
OPT("rate",            param->iFrameRate,         required_argument, 'r', "Frame rate")
OPT("depth",           param->internalBitDepth,   required_argument,   0, "Bit-depth the codec operates at. (default:InputBitDepth)"
                                                                          "If different to InputBitDepth, source data will be converted")
OPT("ctu",             param->uiMaxCUSize,        required_argument, 's', "Maximum CU size (default: 64x64)")
OPT("pdepth",          param->uiMaxCUDepth,       required_argument, 'd', "CU partition depth (default: 4)")

OPT("constrained-intra", param->bUseConstrainedIntraPred,      no_argument, 0, "Constrained intra prediction (use only intra coded reference pixels)")
OPT("TULog2MaxSize",   param->uiQuadtreeTULog2MaxSize,   required_argument, 0, "Maximum TU size in logarithm base 2")
OPT("TULog2MinSize",   param->uiQuadtreeTULog2MinSize,   required_argument, 0, "Minimum TU size in logarithm base 2")
OPT("TUMaxDepthIntra", param->uiQuadtreeTUMaxDepthIntra, required_argument, 0, "Depth of TU tree for intra CUs")
OPT("TUMaxDepthInter", param->uiQuadtreeTUMaxDepthInter, required_argument, 0, "Depth of TU tree for inter CUs")
OPT("keyint",          param->iIntraPeriod,              required_argument, 0, "Intra period in frames, (-1: only first frame)")
OPT("me",              param->searchMethod,              required_argument, 0, "0:dia 1:hex 2:umh 3:star 4:hm-orig")
OPT("merange",         param->iSearchRange,              required_argument, 0, "Motion search range (default: 96)")
OPT("bpredrange",      param->bipredSearchRange,         required_argument, 0, "Motion search range for bipred refinement (default:4)")
OPT("MaxCuDQPDepth",   param->iMaxCuDQPDepth,            required_argument, 0, "Max depth for a minimum CU dQP")
OPT("cbqpoffs",        param->cbQpOffset,                required_argument, 0, "Chroma Cb QP Offset")
OPT("crqpoffs",        param->crQpOffset,                required_argument, 0, "Chroma Cr QP Offset")
OPT("aqselect",        param->bUseAdaptQpSelect,               no_argument, 0, "Adaptive QP selection")
OPT("aq",              param->bUseAdaptiveQP,                  no_argument, 0, "QP adaptation based on a psycho-visual model")
OPT("aqrange",         param->iQPAdaptationRange,        required_argument, 0, "QP adaptation range")
OPT("rdoq",            param->useRDOQ,                         no_argument, 0, "Use RDO quantization")
OPT("rdoqts",          param->useRDOQTS,                       no_argument, 0, "Use RDO quantization with transform skip")
OPT("rdpenalty",       param->rdPenalty,                 required_argument, 0, "RD-penalty for 32x32 TU for intra in non-intra slices. 0:disabled  1:RD-penalty  2:maximum RD-penalty")
OPT("amp",             param->enableAMP,                       no_argument, 0, "Enable asymmetric motion partitions")
OPT("rect",            param->enableRectInter,                 no_argument, 0, "Enable rectangular motion partitions Nx2N and 2NxN, disabling also disables AMP")
OPT("tskip",           param->useTransformSkip,                no_argument, 0, "Intra transform skipping")
OPT("tskip-fast",      param->useTransformSkipFast,            no_argument, 0, "Fast intra transform skipping")
OPT("sao",             param->bUseSAO,                         no_argument, 0, "Enable Sample Adaptive Offset")
OPT("max-sao-offsets", param->maxNumOffsetsPerPic,       required_argument, 0, "Max number of SAO offset per picture (Default: 2048)")
OPT("SAOLcuBoundary",  param->saoLcuBoundary,                  no_argument, 0, "0: right/bottom LCU boundary areas skipped from SAO parameter estimation, 1: non-deblocked pixels are used for those areas")
OPT("sao-lcu-opt",     param->saoLcuBasedOptimization,         no_argument, 0, "0: SAO picture-based optimization, 1: SAO LCU-based optimization ")
OPT("weightp",         param->useWeightedPred,                 no_argument, 0, "Use weighted prediction in P slices")
OPT("weightbp",        param->useWeightedBiPred,               no_argument, 0, "Use weighted (bidirectional) prediction in B slices")
OPT("merge-level",     param->log2ParallelMergeLevel,    required_argument, 0, "Parallel merge estimation region")
OPT("hidesign",        param->signHideFlag,                    no_argument, 0, "Hide sign bit of one coeff per TU (rdo)")
OPT("MaxNumMergeCand", param->maxNumMergeCand,           required_argument, 0, "Maximum number of merge candidates")
OPT("tmvp",            param->TMVPModeId,                required_argument, 0, "TMVP mode 0: TMVP disable for all slices. 1: TMVP enable for all slices (default) 2: TMVP enable for certain slices only")
OPT("fdm",             param->useFastDecisionForMerge,         no_argument, 0, "Fast decision for Merge RD Cost")
OPT("fast-cbf",        param->bUseCbfFastMode,                 no_argument, 0, "Cbf fast mode setting")
OPT("early-skip",      param->useEarlySkipDetection,           no_argument, 0, "Early SKIP detection setting")
OPT("strong-intra-smoothing", param->useStrongIntraSmoothing,  no_argument, 0, "Enable strong intra smoothing for 32x32 blocks")

bool parse(int argc, char **argv, x265_param_t* param, CLIOptions* cliopt)
{
    int help = 0;
    int cpuid = 0;
    int threadcount = 0;
    const char *inputfn = NULL, *reconfn = NULL, *bitstreamfn = NULL;

    if (argc <= 1 || help)
        return true;

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
