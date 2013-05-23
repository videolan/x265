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

#if _MSC_VER
#pragma warning(disable: 4505) // : 'writeAnnexB' : unreferenced local function has been removed
#endif

#include "TLibEncoder/AnnexBwrite.h"
#include "input/input.h"
#include "output/output.h"
#include "threadpool.h"
#include "primitives.h"
#include "common.h"
#include "PPA/ppa.h"
#include "x265.h"

#if HAVE_VLD
/* Visual Leak Detector */
#include <vld.h>
#endif

#include <getopt.h>
#include <list>
#include <ostream>
#include <fstream>

using namespace x265;
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
        // do_help();
        return true;

    x265::SetupPrimitives(cpuid);
    cliopt->threadPool = x265::ThreadPool::AllocThreadPool(threadcount);

    /* parse the width, height, frame rate from the y4m files if it is not given in the configuration file */
    cliopt->input = x265::Input::Open(inputfn);
    if (!cliopt->input || cliopt->input->isFail())
    {
        printf("x265: unable to open input file\n");
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
            printf("x265: unable to write reconstruction file\n");
            cliopt->recon->release();
            cliopt->recon = 0;
        }
    }

#if !HIGH_BIT_DEPTH
    if (cliopt->inputBitDepth != 8 || cliopt->outputBitDepth != 8 || param->internalBitDepth != 8)
    {
        printf("x265: not compiled for bit depths greater than 8\n");
        return true;
    }
#endif

    cliopt->bitstreamFile.open(bitstreamfn, fstream::binary | fstream::out);
    if (!cliopt->bitstreamFile)
    {
        fprintf(stderr, "x265: failed to open bitstream file <%s> for writing\n", bitstreamfn);
        return true;
    }

    printf("Frame index                  : %u - %d (%d frames)\n", 
        cliopt->frameSkip, cliopt->frameSkip + cliopt->framesToBeEncoded - 1, cliopt->framesToBeEncoded);
    printf("Bitstream File               : %s\n", bitstreamfn);
    //printf("GOP size                     : %d\n", m_iGOPSize);

    return false;
}

int main(int argc, char **argv)
{
#if HAVE_VLD
    VLDSetReportOptions(VLD_OPT_REPORT_TO_DEBUGGER, NULL);
#endif
    PPA_INIT();

    x265_param_t param;
    CLIOptions   cliopt;

#define XSTR(x) STR(x)
#define STR(x) #x
    // TODO: perhaps this should only be printed when asked (x265 --version)?
    // TODO: needs proper logging file handle with log levels, etc
    fprintf(stdout, "x265: HEVC encoder version %s\n", XSTR(X265_VERSION));
    fprintf(stdout, "x265: build info ");
    fprintf(stdout, NVM_ONOS);
    fprintf(stdout, NVM_COMPILEDBY);
    fprintf(stdout, NVM_BITS);
#if HIGH_BIT_DEPTH
    fprintf(stdout, "16bpp");
#else
    fprintf(stdout, "8bpp");
#endif
    fprintf(stdout, "\n");

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

    x265_picture_t pic_orig, pic_recon;
    x265_picture_t *pic_in = &pic_orig;
    x265_picture_t *pic_out = cliopt.recon ? &pic_recon : 0;
    x265_nal_t *p_nal;
    int nal;

    // main encoder loop
    uint32_t frameCount = 0;
    while (pic_in)
    {
        // read input YUV file
        if (frameCount < cliopt.framesToBeEncoded && cliopt.input->readPicture(pic_orig))
            frameCount++;
        else
            pic_in = NULL;

        int iNumEncoded = x265_encoder_encode(encoder, &p_nal, &nal, pic_in, pic_out);
        if (iNumEncoded && pic_out)
            cliopt.recon->writePicture(pic_recon);

        /*
        if (nal)
        {
            PPAStartCpuEventFunc(bitstream_write);
            const AccessUnit &au = *(iterBitstream++);
            const vector<UInt>& stats = writeAnnexB(cliopt.bitstreamFile, au);
            cliopt.rateStatsAccum(au, stats);
            PPAStopCpuEventFunc(bitstream_write);
        } */
    }

    /* Flush the encoder */
    while (x265_encoder_encode(encoder, &p_nal, &nal, NULL, pic_out))
    {
        if (pic_out)
            cliopt.recon->writePicture(pic_recon);

        /*
        if (nal)
        {
            PPAStartCpuEventFunc(bitstream_write);
            const AccessUnit &au = *(iterBitstream++);
            const vector<UInt>& stats = writeAnnexB(cliopt.bitstreamFile, au);
            cliopt.rateStatsAccum(au, stats);
            PPAStopCpuEventFunc(bitstream_write);
        } */
    }

    x265_encoder_close(encoder);
    cliopt.bitstreamFile.close();

    double vidtime = (double)frameCount / param.iFrameRate;
    printf("Bytes written to file: %u (%.3f kbps)\n", cliopt.totalBytes, 0.008 * cliopt.totalBytes / vidtime);

#if HAVE_VLD
    assert(VLDReportLeaks() == 0);
#endif
    return 0;
}
