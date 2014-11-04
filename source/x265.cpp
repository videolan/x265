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
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#if _MSC_VER
#pragma warning(disable: 4127) // conditional expression is constant, yes I know
#endif

#include "input/input.h"
#include "output/output.h"
#include "filters/filters.h"
#include "common.h"
#include "param.h"
#include "cpu.h"
#include "x265.h"

#if HAVE_VLD
/* Visual Leak Detector */
#include <vld.h>
#endif
#include "PPA/ppa.h"

#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>

#include <string>
#include <ostream>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#else
#define GetConsoleTitle(t, n)
#define SetConsoleTitle(t)
#endif

using namespace x265;

static const char short_options[] = "o:p:f:F:r:I:i:b:s:t:q:m:hwV?";
static const struct option long_options[] =
{
    { "help",                 no_argument, NULL, 'h' },
    { "version",              no_argument, NULL, 'V' },
    { "asm",            required_argument, NULL, 0 },
    { "no-asm",               no_argument, NULL, 0 },
    { "threads",        required_argument, NULL, 0 },
    { "preset",         required_argument, NULL, 'p' },
    { "tune",           required_argument, NULL, 't' },
    { "frame-threads",  required_argument, NULL, 'F' },
    { "no-pmode",             no_argument, NULL, 0 },
    { "pmode",                no_argument, NULL, 0 },
    { "no-pme",               no_argument, NULL, 0 },
    { "pme",                  no_argument, NULL, 0 },
    { "log-level",      required_argument, NULL, 0 },
    { "profile",        required_argument, NULL, 0 },
    { "level-idc",      required_argument, NULL, 0 },
    { "high-tier",            no_argument, NULL, 0 },
    { "no-high-tier",         no_argument, NULL, 0 },
    { "csv",            required_argument, NULL, 0 },
    { "no-cu-stats",          no_argument, NULL, 0 },
    { "cu-stats",             no_argument, NULL, 0 },
    { "y4m",                  no_argument, NULL, 0 },
    { "no-progress",          no_argument, NULL, 0 },
    { "output",         required_argument, NULL, 'o' },
    { "input",          required_argument, NULL, 0 },
    { "input-depth",    required_argument, NULL, 0 },
    { "input-res",      required_argument, NULL, 0 },
    { "input-csp",      required_argument, NULL, 0 },
    { "interlace",      required_argument, NULL, 0 },
    { "no-interlace",         no_argument, NULL, 0 },
    { "fps",            required_argument, NULL, 0 },
    { "seek",           required_argument, NULL, 0 },
    { "frame-skip",     required_argument, NULL, 0 },
    { "frames",         required_argument, NULL, 'f' },
    { "recon",          required_argument, NULL, 'r' },
    { "recon-depth",    required_argument, NULL, 0 },
    { "no-wpp",               no_argument, NULL, 0 },
    { "wpp",                  no_argument, NULL, 0 },
    { "ctu",            required_argument, NULL, 's' },
    { "tu-intra-depth", required_argument, NULL, 0 },
    { "tu-inter-depth", required_argument, NULL, 0 },
    { "me",             required_argument, NULL, 0 },
    { "subme",          required_argument, NULL, 'm' },
    { "merange",        required_argument, NULL, 0 },
    { "max-merge",      required_argument, NULL, 0 },
    { "no-temporal-mvp",      no_argument, NULL, 0 },
    { "temporal-mvp",         no_argument, NULL, 0 },
    { "rdpenalty",      required_argument, NULL, 0 },
    { "no-rect",              no_argument, NULL, 0 },
    { "rect",                 no_argument, NULL, 0 },
    { "no-amp",               no_argument, NULL, 0 },
    { "amp",                  no_argument, NULL, 0 },
    { "no-early-skip",        no_argument, NULL, 0 },
    { "early-skip",           no_argument, NULL, 0 },
    { "no-fast-cbf",          no_argument, NULL, 0 },
    { "fast-cbf",             no_argument, NULL, 0 },
    { "no-tskip",             no_argument, NULL, 0 },
    { "tskip",                no_argument, NULL, 0 },
    { "no-tskip-fast",        no_argument, NULL, 0 },
    { "tskip-fast",           no_argument, NULL, 0 },
    { "cu-lossless",          no_argument, NULL, 0 },
    { "no-cu-lossless",       no_argument, NULL, 0 },
    { "no-constrained-intra", no_argument, NULL, 0 },
    { "constrained-intra",    no_argument, NULL, 0 },
    { "fast-intra",           no_argument, NULL, 0 },
    { "no-fast-intra",        no_argument, NULL, 0 },
    { "no-open-gop",          no_argument, NULL, 0 },
    { "open-gop",             no_argument, NULL, 0 },
    { "keyint",         required_argument, NULL, 'I' },
    { "min-keyint",     required_argument, NULL, 'i' },
    { "scenecut",       required_argument, NULL, 0 },
    { "no-scenecut",          no_argument, NULL, 0 },
    { "rc-lookahead",   required_argument, NULL, 0 },
    { "bframes",        required_argument, NULL, 'b' },
    { "bframe-bias",    required_argument, NULL, 0 },
    { "b-adapt",        required_argument, NULL, 0 },
    { "no-b-adapt",           no_argument, NULL, 0 },
    { "no-b-pyramid",         no_argument, NULL, 0 },
    { "b-pyramid",            no_argument, NULL, 0 },
    { "ref",            required_argument, NULL, 0 },
    { "no-weightp",           no_argument, NULL, 0 },
    { "weightp",              no_argument, NULL, 'w' },
    { "no-weightb",           no_argument, NULL, 0 },
    { "weightb",              no_argument, NULL, 0 },
    { "crf",            required_argument, NULL, 0 },
    { "crf-max",        required_argument, NULL, 0 },
    { "crf-min",        required_argument, NULL, 0 },
    { "vbv-maxrate",    required_argument, NULL, 0 },
    { "vbv-bufsize",    required_argument, NULL, 0 },
    { "vbv-init",       required_argument, NULL, 0 },
    { "bitrate",        required_argument, NULL, 0 },
    { "qp",             required_argument, NULL, 'q' },
    { "aq-mode",        required_argument, NULL, 0 },
    { "aq-strength",    required_argument, NULL, 0 },
    { "ipratio",        required_argument, NULL, 0 },
    { "pbratio",        required_argument, NULL, 0 },
    { "cbqpoffs",       required_argument, NULL, 0 },
    { "crqpoffs",       required_argument, NULL, 0 },
    { "rd",             required_argument, NULL, 0 },
    { "psy-rd",         required_argument, NULL, 0 },
    { "psy-rdoq",       required_argument, NULL, 0 },
    { "scaling-list",   required_argument, NULL, 0 },
    { "lossless",             no_argument, NULL, 0 },
    { "no-lossless",          no_argument, NULL, 0 },
    { "no-signhide",          no_argument, NULL, 0 },
    { "signhide",             no_argument, NULL, 0 },
    { "no-lft",               no_argument, NULL, 0 }, /* DEPRECATED */
    { "lft",                  no_argument, NULL, 0 }, /* DEPRECATED */
    { "no-deblock",           no_argument, NULL, 0 },
    { "deblock",        required_argument, NULL, 0 },
    { "no-sao",               no_argument, NULL, 0 },
    { "sao",                  no_argument, NULL, 0 },
    { "no-sao-non-deblock",   no_argument, NULL, 0 },
    { "sao-non-deblock",      no_argument, NULL, 0 },
    { "no-ssim",              no_argument, NULL, 0 },
    { "ssim",                 no_argument, NULL, 0 },
    { "no-psnr",              no_argument, NULL, 0 },
    { "psnr",                 no_argument, NULL, 0 },
    { "hash",           required_argument, NULL, 0 },
    { "no-strong-intra-smoothing", no_argument, NULL, 0 },
    { "strong-intra-smoothing",    no_argument, NULL, 0 },
    { "no-cutree",                 no_argument, NULL, 0 },
    { "cutree",                    no_argument, NULL, 0 },
    { "no-hrd",               no_argument, NULL, 0 },
    { "hrd",                  no_argument, NULL, 0 },
    { "sar",            required_argument, NULL, 0 },
    { "overscan",       required_argument, NULL, 0 },
    { "videoformat",    required_argument, NULL, 0 },
    { "range",          required_argument, NULL, 0 },
    { "colorprim",      required_argument, NULL, 0 },
    { "transfer",       required_argument, NULL, 0 },
    { "colormatrix",    required_argument, NULL, 0 },
    { "chromaloc",      required_argument, NULL, 0 },
    { "crop-rect",      required_argument, NULL, 0 },
    { "no-dither",            no_argument, NULL, 0 },
    { "dither",               no_argument, NULL, 0 },
    { "no-repeat-headers",    no_argument, NULL, 0 },
    { "repeat-headers",       no_argument, NULL, 0 },
    { "aud",                  no_argument, NULL, 0 },
    { "no-aud",               no_argument, NULL, 0 },
    { "info",                 no_argument, NULL, 0 },
    { "no-info",              no_argument, NULL, 0 },
    { "qpfile",         required_argument, NULL, 0 },
    { "lambda-file",    required_argument, NULL, 0 },
    { "b-intra",              no_argument, NULL, 0 },
    { "no-b-intra",           no_argument, NULL, 0 },
    { "nr",             required_argument, NULL, 0 },
    { "stats",          required_argument, NULL, 0 },
    { "pass",           required_argument, NULL, 0 },
    { "slow-firstpass",       no_argument, NULL, 0 },
    { "no-slow-firstpass",    no_argument, NULL, 0 },
    { "analysis-mode",  required_argument, NULL, 0 },
    { "analysis-file",  required_argument, NULL, 0 },
    { 0, 0, 0, 0 }
};

/* Ctrl-C handler */
static volatile sig_atomic_t b_ctrl_c /* = 0 */;
static void sigint_handler(int)
{
    b_ctrl_c = 1;
}

struct CLIOptions
{
    Input*  input;
    Output* recon;
    std::fstream bitstreamFile;
    bool bProgress;
    bool bForceY4m;
    bool bDither;

    uint32_t seek;              // number of frames to skip from the beginning
    uint32_t framesToBeEncoded; // number of frames to encode
    uint64_t totalbytes;
    size_t   analysisRecordSize; // number of bytes read from or dumped into file
    int      analysisHeaderSize;

    int64_t startTime;
    int64_t prevUpdateTime;
    float   frameRate;
    FILE*   qpfile;
    FILE*   analysisFile;

    /* in microseconds */
    static const int UPDATE_INTERVAL = 250000;

    CLIOptions()
    {
        input = NULL;
        recon = NULL;
        framesToBeEncoded = seek = 0;
        totalbytes = 0;
        bProgress = true;
        bForceY4m = false;
        startTime = x265_mdate();
        prevUpdateTime = 0;
        bDither = false;
        qpfile = NULL;
        analysisFile = NULL;
        analysisRecordSize = 0;
        analysisHeaderSize = 0;
    }

    void destroy();
    void writeNALs(const x265_nal* nal, uint32_t nalcount);
    void printStatus(uint32_t frameNum, x265_param *param);
    void printVersion(x265_param *param);
    void showHelp(x265_param *param);
    bool parse(int argc, char **argv, x265_param* param);
    bool parseQPFile(x265_picture &pic_org);
    void readAnalysisFile(x265_picture* pic, x265_param*);
    void writeAnalysisFile(x265_picture* pic, x265_param*);
    bool validateFanout(x265_param*);
};

void CLIOptions::destroy()
{
    if (input)
        input->release();
    input = NULL;
    if (recon)
        recon->release();
    recon = NULL;
    if (qpfile)
        fclose(qpfile);
    qpfile = NULL;
    if (analysisFile)
        fclose(analysisFile);
    analysisFile = NULL;
}

void CLIOptions::writeNALs(const x265_nal* nal, uint32_t nalcount)
{
    PPAScopeEvent(bitstream_write);
    for (uint32_t i = 0; i < nalcount; i++)
    {
        bitstreamFile.write((const char*)nal->payload, nal->sizeBytes);
        totalbytes += nal->sizeBytes;
        nal++;
    }
}

void CLIOptions::printStatus(uint32_t frameNum, x265_param *param)
{
    char buf[200];
    int64_t time = x265_mdate();

    if (!bProgress || !frameNum || (prevUpdateTime && time - prevUpdateTime < UPDATE_INTERVAL))
        return;
    int64_t elapsed = time - startTime;
    double fps = elapsed > 0 ? frameNum * 1000000. / elapsed : 0;
    float bitrate = 0.008f * totalbytes * (param->fpsNum / param->fpsDenom) / ((float)frameNum);
    if (framesToBeEncoded)
    {
        int eta = (int)(elapsed * (framesToBeEncoded - frameNum) / ((int64_t)frameNum * 1000000));
        sprintf(buf, "x265 [%.1f%%] %d/%d frames, %.2f fps, %.2f kb/s, eta %d:%02d:%02d",
                100. * frameNum / framesToBeEncoded, frameNum, framesToBeEncoded, fps, bitrate,
                eta / 3600, (eta / 60) % 60, eta % 60);
    }
    else
    {
        sprintf(buf, "x265 %d frames: %.2f fps, %.2f kb/s", frameNum, fps, bitrate);
    }
    fprintf(stderr, "%s  \r", buf + 5);
    SetConsoleTitle(buf);
    fflush(stderr); // needed in windows
    prevUpdateTime = time;
}

void CLIOptions::printVersion(x265_param *param)
{
    x265_log(param, X265_LOG_INFO, "HEVC encoder version %s\n", x265_version_str);
    x265_log(param, X265_LOG_INFO, "build info %s\n", x265_build_info_str);
}

void CLIOptions::showHelp(x265_param *param)
{
    x265_param_default(param);
    printVersion(param);

#define H0 printf
#define OPT(value) (value ? "enabled" : "disabled")
    H0("\nSyntax: x265 [options] infile [-o] outfile\n");
    H0("    infile can be YUV or Y4M\n");
    H0("    outfile is raw HEVC bitstream\n");
    H0("\nExecutable Options:\n");
    H0("-h/--help                        Show this help text and exit\n");
    H0("-V/--version                     Show version info and exit\n");
    H0("\nOutput Options:\n");
    H0("-o/--output <filename>           Bitstream output file name\n");
    H0("   --log-level <string>          Logging level: none error warning info debug full. Default %s\n", logLevelNames[param->logLevel + 1]);
    H0("   --no-progress                 Disable CLI progress reports\n");
    H0("   --[no-]cu-stats               Enable logging stats about distribution of cu across all modes. Default %s\n",OPT(param->bLogCuStats));
    H0("   --csv <filename>              Comma separated log file, log level >= 3 frame log, else one line per run\n");
    H0("\nInput Options:\n");
    H0("   --input <filename>            Raw YUV or Y4M input file name. `-` for stdin\n");
    H0("   --y4m                         Force parsing of input stream as YUV4MPEG2 regardless of file extension\n");
    H0("   --fps <float|rational>        Source frame rate (float or num/denom), auto-detected if Y4M\n");
    H0("   --input-res WxH               Source picture size [w x h], auto-detected if Y4M\n");
    H0("   --input-depth <integer>       Bit-depth of input file. Default 8\n");
    H0("   --input-csp <string>          Source color space: i420, i444 or i422, auto-detected if Y4M. Default: i420\n");
    H0("-f/--frames <integer>            Maximum number of frames to encode. Default all\n");
    H0("   --seek <integer>              First frame to encode\n");
    H0("   --[no-]interlace <bff|tff>    Indicate input pictures are interlace fields in temporal order. Default progressive\n");
    H0("   --dither                      Enable dither if downscaling to 8 bit pixels. Default disabled\n");
    H0("\nQuality reporting metrics:\n");
    H0("   --[no-]ssim                   Enable reporting SSIM metric scores. Default %s\n", OPT(param->bEnableSsim));
    H0("   --[no-]psnr                   Enable reporting PSNR metric scores. Default %s\n", OPT(param->bEnablePsnr));
    H0("\nProfile, Level, Tier:\n");
    H0("   --profile <string>            Enforce an encode profile: main, main10, mainstillpicture\n");
    H0("   --level-idc <integer|float>   Force a minumum required decoder level (as '5.0' or '50')\n");
    H0("   --[no-]high-tier              If a decoder level is specified, this modifier selects High tier of that level\n");
    H0("\nThreading, performance:\n");
    H0("   --threads <integer>           Number of threads for thread pool (0: detect CPU core count, default)\n");
    H0("-F/--frame-threads <integer>     Number of concurrently encoded frames. 0: auto-determined by core count\n");
    H0("   --[no-]wpp                    Enable Wavefront Parallel Processing. Default %s\n", OPT(param->bEnableWavefront));
    H0("   --[no-]pmode                  Parallel mode analysis. Default %s\n", OPT(param->bDistributeModeAnalysis));
    H0("   --[no-]pme                    Parallel motion estimation. Default %s\n", OPT(param->bDistributeMotionEstimation));
    H0("   --[no-]asm <bool|int|string>  Override CPU detection. Default: auto\n");
    H0("\nPresets:\n");
    H0("-p/--preset <string>             Trade off performance for compression efficiency. Default medium\n");
    H0("                                 ultrafast, superfast, veryfast, faster, fast, medium, slow, slower, veryslow, or placebo\n");
    H0("-t/--tune <string>               Tune the settings for a particular type of source or situation:\n");
    H0("                                 psnr, ssim, zerolatency, or fastdecode\n");
    H0("\nQuad-Tree size and depth:\n");
    H0("-s/--ctu <64|32|16>              Maximum CU size (default: 64x64). Default %d\n", param->maxCUSize);
    H0("   --tu-intra-depth <integer>    Max TU recursive depth for intra CUs. Default %d\n", param->tuQTMaxIntraDepth);
    H0("   --tu-inter-depth <integer>    Max TU recursive depth for inter CUs. Default %d\n", param->tuQTMaxInterDepth);
    H0("   --[no-]rect                   Enable rectangular motion partitions Nx2N and 2NxN. Default %s\n", OPT(param->bEnableRectInter));
    H0("   --[no-]amp                    Enable asymmetric motion partitions, requires --rect. Default %s\n", OPT(param->bEnableAMP));
    H0("\nAnalysis:\n");
    H0("   --rd <0..6>                   Level of RD in mode decision 0:least....6:full RDO. Default %d\n", param->rdLevel);
    H0("   --psy-rd <0..2.0>             Strength of psycho-visual rate distortion optimization, 0 to disable. Default %f\n", param->psyRd);
    H0("   --psy-rdoq <0..50.0>          Strength of psycho-visual optimization in quantization, 0 to disable. Default %f\n", param->psyRdoq);
    H0("   --nr <integer>                An integer value in range of 100 to 1000, which denotes strength of noise reduction. Default disabled\n");
    H0("   --[no-]tskip-fast             Enable fast intra transform skipping. Default %s\n", OPT(param->bEnableTSkipFast));
    H0("   --[no-]early-skip             Enable early SKIP detection. Default %s\n", OPT(param->bEnableEarlySkip));
    H0("   --[no-]fast-cbf               Enable early outs based on whether residual is coded. Default %s\n", OPT(param->bEnableCbfFastMode));
    H0("\nCoding tools:\n");
    H0("-w/--[no-]weightp                Enable weighted prediction in P slices. Default %s\n", OPT(param->bEnableWeightedPred));
    H0("   --[no-]weightb                Enable weighted prediction in B slices. Default %s\n", OPT(param->bEnableWeightedBiPred));
    H0("   --[no-]cu-lossless            Consider lossless mode in CU RDO decisions. Default %s\n", OPT(param->bCULossless));
    H0("   --[no-]signhide               Hide sign bit of one coeff per TU (rdo). Default %s\n", OPT(param->bEnableSignHiding));
    H0("   --[no-]tskip                  Enable intra 4x4 transform skipping. Default %s\n", OPT(param->bEnableTransformSkip));
    H0("\nTemporal / motion search options:\n");
    H0("   --me <string>                 Motion search method dia hex umh star full. Default %d\n", param->searchMethod);
    H0("-m/--subme <integer>             Amount of subpel refinement to perform (0:least .. 7:most). Default %d \n", param->subpelRefine);
    H0("   --merange <integer>           Motion search range. Default %d\n", param->searchRange);
    H0("   --max-merge <1..5>            Maximum number of merge candidates. Default %d\n", param->maxNumMergeCand);
    H0("   --[no-]temporal-mvp           Enable temporal MV predictors. Default %s\n", OPT(param->bEnableTemporalMvp));
    H0("\nSpatial / intra options:\n");
    H0("   --[no-]strong-intra-smoothing Enable strong intra smoothing for 32x32 blocks. Default %s\n", OPT(param->bEnableStrongIntraSmoothing));
    H0("   --[no-]constrained-intra      Constrained intra prediction (use only intra coded reference pixels) Default %s\n", OPT(param->bEnableConstrainedIntra));
    H0("   --[no-]b-intra                Enable intra in B frames in veryslow presets. Default %s\n", OPT(param->bIntraInBFrames));
    H0("   --[no-]fast-intra             Enable faster search method for angular intra predictions. Default %s\n", OPT(param->bEnableFastIntra));
    H0("   --rdpenalty <0..2>            penalty for 32x32 intra TU in non-I slices. 0:disabled 1:RD-penalty 2:maximum. Default %d\n", param->rdPenalty);
    H0("\nSlice decision options:\n");
    H0("   --[no-]open-gop               Enable open-GOP, allows I slices to be non-IDR. Default %s\n", OPT(param->bOpenGOP));
    H0("-I/--keyint <integer>            Max IDR period in frames. -1 for infinite-gop. Default %d\n", param->keyframeMax);
    H0("-i/--min-keyint <integer>        Scenecuts closer together than this are coded as I, not IDR. Default: auto\n");
    H0("   --no-scenecut                 Disable adaptive I-frame decision\n");
    H0("   --scenecut <integer>          How aggressively to insert extra I-frames. Default %d\n", param->scenecutThreshold);
    H0("   --rc-lookahead <integer>      Number of frames for frame-type lookahead (determines encoder latency) Default %d\n", param->lookaheadDepth);
    H0("   --bframes <integer>           Maximum number of consecutive b-frames (now it only enables B GOP structure) Default %d\n", param->bframes);
    H0("   --bframe-bias <integer>       Bias towards B frame decisions. Default %d\n", param->bFrameBias);
    H0("   --b-adapt <0..2>              0 - none, 1 - fast, 2 - full (trellis) adaptive B frame scheduling. Default %d\n", param->bFrameAdaptive);
    H0("   --[no-]b-pyramid              Use B-frames as references. Default %s\n", OPT(param->bBPyramid));
    H0("   --ref <integer>               max number of L0 references to be allowed (1 .. 16) Default %d\n", param->maxNumReferences);
    H0("   --qpfile <string>             Force frametypes and QPs for some or all frames\n");
    H0("                                 Format of each line: framenumber frametype QP\n");
    H0("                                 QP is optional (none lets x265 choose). Frametypes: I,i,P,B,b.\n");
    H0("                                 QPs are restricted by qpmin/qpmax.\n");
    H0("\nRate control, Quantization:\n");
    H0("   --bitrate <integer>           Target bitrate (kbps) for ABR (implied). Default %d\n", param->rc.bitrate);
    H0("-q/--qp <integer>                QP for P slices in CQP mode (implied). --ipratio and --pbration determine other slice QPs\n");
    H0("   --crf <float>                 Quality-based VBR (0-51). Default %f\n", param->rc.rfConstant);
    H0("   --[no-]lossless               Enable lossless: bypass transform, quant and loop filters globally. Default %s\n", OPT(param->bLossless));
    H0("   --crf-max <float>             With CRF+VBV, limit RF to this value. Default %f\n", param->rc.rfConstantMax);
    H0("                                 May cause VBV underflows!\n");
    H0("   --crf-min <float>             With CRF+VBV, limit RF to this value. Default %f\n", param->rc.rfConstantMin);
    H0("                                 this specifies a minimum rate factor value for encode!\n");
    H0("   --vbv-maxrate <integer>       Max local bitrate (kbit/s). Default %d\n", param->rc.vbvMaxBitrate);
    H0("   --vbv-bufsize <integer>       Set size of the VBV buffer (kbit). Default %d\n", param->rc.vbvBufferSize);
    H0("   --vbv-init <float>            Initial VBV buffer occupancy (fraction of bufsize or in kbits). Default %f\n", param->rc.vbvBufferInit);
    H0("   --aq-mode <integer>           Mode for Adaptive Quantization - 0:none 1:uniform AQ 2:auto variance. Default %d\n", param->rc.aqMode);
    H0("   --aq-strength <float>         Reduces blocking and blurring in flat and textured areas.(0 to 3.0). Default %f\n", param->rc.aqStrength);
    H0("   --[no-]cutree                 Enable cutree for Adaptive Quantization. Default %s\n", OPT(param->rc.cuTree));
    H0("   --ipratio <float>             QP factor between I and P. Default %f\n", param->rc.ipFactor);
    H0("   --pbratio <float>             QP factor between P and B. Default %f\n", param->rc.pbFactor);
    H0("   --cbqpoffs <integer>          Chroma Cb QP Offset. Default %d\n", param->cbQpOffset);
    H0("   --crqpoffs <integer>          Chroma Cr QP Offset. Default %d\n", param->crQpOffset);
    H0("   --stats                       Filename for stats file in multipass pass rate control. Default x265_2pass.log\n");
    H0("   --pass                        Multi pass rate control.\n"
       "                                   - 1 : First pass, creates stats file\n"
       "                                   - 2 : Last pass, does not overwrite stats file\n"
       "                                   - 3 : Nth pass, overwrites stats file\n");
    H0("   --[no-]slow-firstpass         Enable a slow first pass in a multipass rate control mode. Default %s\n", OPT(param->rc.bEnableSlowFirstPass));
    H0("   --analysis-mode <string|int>  save - Dump analysis info into file, load - Load analysis buffers from the file. Default %d\n", param->analysisMode);
    H0("   --analysis-file <filename>    Specify file name used for either dumping or reading analysis data.\n");
    H0("   --scaling-list <string>       Specify a file containing HM style quant scaling lists or 'default' or 'off'. Default: off\n");
    H0("   --lambda-file <string>        Specify a file containing replacement values for the lambda tables\n");
    H0("                                 MAX_MAX_QP+1 floats for lambda table, then again for lambda2 table\n");
    H0("                                 Blank lines and lines starting with hash(#) are ignored\n");
    H0("                                 Comma is considered to be white-space\n");
    H0("\nLoop filters (deblock and SAO):\n");
    H0("   --[no-]deblock                Enable Deblocking Loop Filter, optionally specify tC:Beta offsets Default %s\n", OPT(param->bEnableLoopFilter));
    H0("   --[no-]sao                    Enable Sample Adaptive Offset. Default %s\n", OPT(param->bEnableSAO));
    H0("   --[no-]sao-non-deblock        Use non-deblocked pixels, else right/bottom boundary areas skipped. Default %s\n", OPT(param->bSaoNonDeblocked));
    H0("\nVUI options:\n");
    H0("   --sar <width:height|int>      Sample Aspect Ratio, the ratio of width to height of an individual pixel.\n");
    H0("                                 Choose from 0=undef, 1=1:1(\"square\"), 2=12:11, 3=10:11, 4=16:11,\n");
    H0("                                 5=40:33, 6=24:11, 7=20:11, 8=32:11, 9=80:33, 10=18:11, 11=15:11,\n");
    H0("                                 12=64:33, 13=160:99, 14=4:3, 15=3:2, 16=2:1 or custom ratio of <int:int>. Default %d\n", param->vui.aspectRatioIdc);
    H0("   --crop-rect <string>          Add 'left,top,right,bottom' to the bitstream-level cropping rectangle\n");
    H0("   --overscan <string>           Specify whether it is appropriate for decoder to show cropped region: undef, show or crop. Default undef\n");
    H0("   --videoformat <string>        Specify video format from undef, component, pal, ntsc, secam, mac. Default undef\n");
    H0("   --range <string>              Specify black level and range of luma and chroma signals as full or limited Default limited\n");
    H0("   --colorprim <string>          Specify color primaries from undef, bt709, bt470m, bt470bg, smpte170m,\n");
    H0("                                 smpte240m, film, bt2020. Default undef\n");
    H0("   --transfer <string>           Specify transfer characteristics from undef, bt709, bt470m, bt470bg, smpte170m,\n");
    H0("                                 smpte240m, linear, log100, log316, iec61966-2-4, bt1361e, iec61966-2-1,\n");
    H0("                                 bt2020-10, bt2020-12. Default undef\n");
    H0("   --colormatrix <string>        Specify color matrix setting from undef, bt709, fcc, bt470bg, smpte170m,\n");
    H0("                                 smpte240m, GBR, YCgCo, bt2020nc, bt2020c. Default undef\n");
    H0("   --chromaloc <integer>         Specify chroma sample location (0 to 5). Default of %d\n", param->vui.chromaSampleLocTypeTopField);
    H0("\nBitstream options:\n");
    H0("   --[no-]info                   Emit SEI identifying encoder and parameters. Default %s\n", OPT(param->bEmitInfoSEI));
    H0("   --[no-]aud                    Emit access unit delimiters at the start of each access unit. Default %s\n", OPT(param->bEnableAccessUnitDelimiters));
    H0("   --[no-]hrd                    Enable HRD parameters signalling. Default %s\n", OPT(param->bEmitHRDSEI));
    H0("   --[no-]repeat-headers         Emit SPS and PPS headers at each keyframe. Default %s\n", OPT(param->bRepeatHeaders));
    H0("   --hash <integer>              Decoded Picture Hash SEI 0: disabled, 1: MD5, 2: CRC, 3: Checksum. Default %d\n", param->decodedPictureHashSEI);
    H0("\nReconstructed video options (debugging):\n");
    H0("-r/--recon <filename>            Reconstructed raw image YUV or Y4M output file name\n");
    H0("   --recon-depth <integer>       Bit-depth of reconstructed raw image file. Defaults to input bit depth, or 8 if Y4M\n");
#undef OPT
#undef H0
    printf("\n\nFull documentation may be found at http://x265.readthedocs.org/en/default/cli.html\n");
    exit(0);
}

bool CLIOptions::parse(int argc, char **argv, x265_param* param)
{
    bool bError = 0;
    int help = 0;
    int inputBitDepth = 8;
    int reconFileBitDepth = 0;
    const char *inputfn = NULL;
    const char *reconfn = NULL;
    const char *bitstreamfn = NULL;
    const char *preset = NULL;
    const char *tune = NULL;
    const char *profile = NULL;
    const char *analysisfn = "x265_analysis.dat";

    if (argc <= 1)
    {
        x265_log(NULL, X265_LOG_ERROR, "No input file. Run x265 --help for a list of options.\n");
        return true;
    }

    /* Presets are applied before all other options. */
    for (optind = 0;; )
    {
        int c = getopt_long(argc, argv, short_options, long_options, NULL);
        if (c == -1)
            break;
        if (c == 'p')
            preset = optarg;
        if (c == 't')
            tune = optarg;
        else if (c == '?')
            showHelp(param);
    }

    if (x265_param_default_preset(param, preset, tune) < 0)
    {
        x265_log(NULL, X265_LOG_ERROR, "preset or tune unrecognized\n");
        return true;
    }

    for (optind = 0;; )
    {
        int long_options_index = -1;
        int c = getopt_long(argc, argv, short_options, long_options, &long_options_index);
        if (c == -1)
        {
            break;
        }

        switch (c)
        {
        case 'h':
            showHelp(param);
            break;

        case 'V':
            printVersion(param);
            x265_setup_primitives(param, -1);
            exit(0);

        default:
            if (long_options_index < 0 && c > 0)
            {
                for (size_t i = 0; i < sizeof(long_options) / sizeof(long_options[0]); i++)
                {
                    if (long_options[i].val == c)
                    {
                        long_options_index = (int)i;
                        break;
                    }
                }

                if (long_options_index < 0)
                {
                    /* getopt_long might have already printed an error message */
                    if (c != 63)
                        x265_log(NULL, X265_LOG_WARNING, "internal error: short option '%c' has no long option\n", c);
                    return true;
                }
            }
            if (long_options_index < 0)
            {
                x265_log(NULL, X265_LOG_WARNING, "short option '%c' unrecognized\n", c);
                return true;
            }
#define OPT(longname) \
    else if (!strcmp(long_options[long_options_index].name, longname))
#define OPT2(name1, name2) \
    else if (!strcmp(long_options[long_options_index].name, name1) || \
             !strcmp(long_options[long_options_index].name, name2))

            if (0) ;
            OPT2("frame-skip", "seek") this->seek = (uint32_t)x265_atoi(optarg, bError);
            OPT("frames") this->framesToBeEncoded = (uint32_t)x265_atoi(optarg, bError);
            OPT("no-progress") this->bProgress = false;
            OPT("output") bitstreamfn = optarg;
            OPT("input") inputfn = optarg;
            OPT("recon") reconfn = optarg;
            OPT("input-depth") inputBitDepth = (uint32_t)x265_atoi(optarg, bError);
            OPT("dither") this->bDither = true;
            OPT("recon-depth") reconFileBitDepth = (uint32_t)x265_atoi(optarg, bError);
            OPT("y4m") this->bForceY4m = true;
            OPT("profile") profile = optarg; /* handled last */
            OPT("preset") /* handled above */;
            OPT("tune")   /* handled above */;
            OPT("analysis-file") analysisfn = optarg;
            OPT("qpfile")
            {
                this->qpfile = fopen(optarg, "rb");
                if (!this->qpfile)
                {
                    x265_log(param, X265_LOG_ERROR, "%s qpfile not found or error in opening qp file \n", optarg);
                    return false;
                }
            }
            else
                bError |= !!x265_param_parse(param, long_options[long_options_index].name, optarg);

            if (bError)
            {
                const char *name = long_options_index > 0 ? long_options[long_options_index].name : argv[optind - 2];
                x265_log(NULL, X265_LOG_ERROR, "invalid argument: %s = %s\n", name, optarg);
                return true;
            }
#undef OPT
        }
    }

    if (optind < argc && !inputfn)
        inputfn = argv[optind++];
    if (optind < argc && !bitstreamfn)
        bitstreamfn = argv[optind++];
    if (optind < argc)
    {
        x265_log(param, X265_LOG_WARNING, "extra unused command arguments given <%s>\n", argv[optind]);
        return true;
    }

    if (argc <= 1 || help)
        showHelp(param);

    if (inputfn == NULL || bitstreamfn == NULL)
    {
        x265_log(param, X265_LOG_ERROR, "input or output file not specified, try -V for help\n");
        return true;
    }

#if HIGH_BIT_DEPTH
    if (param->internalBitDepth != 10)
    {
        x265_log(param, X265_LOG_ERROR, "Only bit depths of 10 are supported in this build\n");
        return true;
    }
#else
    if (param->internalBitDepth != 8)
    {
        x265_log(param, X265_LOG_ERROR, "Only bit depths of 8 are supported in this build\n");
        return true;
    }
#endif // if HIGH_BIT_DEPTH

    InputFileInfo info;
    info.filename = inputfn;
    info.depth = inputBitDepth;
    info.csp = param->internalCsp;
    info.width = param->sourceWidth;
    info.height = param->sourceHeight;
    info.fpsNum = param->fpsNum;
    info.fpsDenom = param->fpsDenom;
    info.sarWidth = param->vui.sarWidth;
    info.sarHeight = param->vui.sarHeight;
    info.skipFrames = seek;
    info.frameCount = 0;
    getParamAspectRatio(param, info.sarWidth, info.sarHeight);

    this->input = Input::open(info, this->bForceY4m);
    if (!this->input || this->input->isFail())
    {
        x265_log(param, X265_LOG_ERROR, "unable to open input file <%s>\n", inputfn);
        return true;
    }

    if (info.depth < 8 || info.depth > 16)
    {
        x265_log(param, X265_LOG_ERROR, "Input bit depth (%d) must be between 8 and 16\n", inputBitDepth);
        return true;
    }

    /* Unconditionally accept height/width/csp from file info */
    param->sourceWidth = info.width;
    param->sourceHeight = info.height;
    param->internalCsp = info.csp;

    /* Accept fps and sar from file info if not specified by user */
    if (param->fpsDenom == 0 || param->fpsNum == 0)
    {
        param->fpsDenom = info.fpsDenom;
        param->fpsNum = info.fpsNum;
    }
    if (!param->vui.aspectRatioIdc && info.sarWidth && info.sarHeight)
        setParamAspectRatio(param, info.sarWidth, info.sarHeight);
    if (this->framesToBeEncoded == 0 && info.frameCount > (int)seek)
        this->framesToBeEncoded = info.frameCount - seek;
    param->totalFrames = this->framesToBeEncoded;

    if (x265_param_apply_profile(param, profile))
        return true;

    if (param->logLevel >= X265_LOG_INFO)
    {
        char buf[128];
        int p = sprintf(buf, "%dx%d fps %d/%d %sp%d", param->sourceWidth, param->sourceHeight,
                        param->fpsNum, param->fpsDenom, x265_source_csp_names[param->internalCsp], info.depth);

        int width, height;
        getParamAspectRatio(param, width, height);
        if (width && height)
            p += sprintf(buf + p, " sar %d:%d", width, height);

        if (framesToBeEncoded <= 0 || info.frameCount <= 0)
            strcpy(buf + p, " unknown frame count");
        else
            sprintf(buf + p, " frames %u - %d of %d", this->seek, this->seek + this->framesToBeEncoded - 1, info.frameCount);

        fprintf(stderr, "%s  [info]: %s\n", input->getName(), buf);
    }

    this->input->startReader();

    if (reconfn)
    {
        if (reconFileBitDepth == 0)
            reconFileBitDepth = param->internalBitDepth;
        this->recon = Output::open(reconfn, param->sourceWidth, param->sourceHeight, reconFileBitDepth,
                                   param->fpsNum, param->fpsDenom, param->internalCsp);
        if (this->recon->isFail())
        {
            x265_log(param, X265_LOG_WARNING, "unable to write reconstruction file\n");
            this->recon->release();
            this->recon = 0;
        }
        else
            fprintf(stderr, "%s  [info]: reconstructed images %dx%d fps %d/%d %s\n", this->recon->getName(),
                    param->sourceWidth, param->sourceHeight, param->fpsNum, param->fpsDenom,
                    x265_source_csp_names[param->internalCsp]);
    }

    this->bitstreamFile.open(bitstreamfn, std::fstream::binary | std::fstream::out);
    if (!this->bitstreamFile)
    {
        x265_log(NULL, X265_LOG_ERROR, "failed to open bitstream file <%s> for writing\n", bitstreamfn);
        return true;
    }

    if (param->analysisMode)
    {
        const char *mode = param->analysisMode == X265_ANALYSIS_SAVE ? "wb" : "rb";
        this->analysisFile = fopen(analysisfn, mode);
        if (!this->analysisFile)
        {
            x265_log(NULL, X265_LOG_ERROR, "failed to open analysis file %s\n", analysisfn);
            return true;
        }
    }

    return false;
}

bool CLIOptions::validateFanout(x265_param *param)
{
#define CMP_OPT_FANOUT(opt, param_val)\
    {\
        bErr = 0;\
        p = strstr(paramBuf, opt "=");\
        if (p && sscanf(p, opt "=%d" , &i) && param_val != i)\
            bErr = 1;\
        else if (param_val && strstr(paramBuf, "no-"opt))\
            bErr = 1;\
        if (bErr)\
        {\
            x265_log(param, X265_LOG_ERROR, "different " opt " setting than given in analysis file (%d vs %d)\n", param_val, i);\
            X265_FREE(paramBuf);\
            return false;\
        }\
    }

    char *p = NULL, *paramBuf;
    int i, j;
    uint32_t k , l;
    bool bErr = false;

    paramBuf = X265_MALLOC(char, MAXPARAMSIZE);
    if (!paramBuf)
        return false;

    fread(paramBuf, 1, MAXPARAMSIZE, this->analysisFile);

    /* check whether fanout options are compatible */
    if (strncmp(paramBuf, "#options:", 9))
    {
        x265_log(param, X265_LOG_ERROR, "options list in analysis file is not valid\n");
        X265_FREE(paramBuf);
        return false;
    }

    char* buf = strchr(paramBuf, '\n');
    if (!buf)
    {
        x265_log(param, X265_LOG_ERROR, "Malformed analysis file\n");
        X265_FREE(paramBuf);
        return false;
    }
    *buf = '\0';
    fseek(this->analysisFile, (int)strlen(paramBuf) + 1, SEEK_SET);

    if (sscanf(paramBuf, "#options: %dx%d", &i, &j) != 2)
    {
        x265_log(param, X265_LOG_ERROR, "Resolution specified in analysis file is not valid\n");
        X265_FREE(paramBuf);
        return false;
    }
    if ((p = strstr(paramBuf, " fps=")) == 0 || sscanf(p, " fps=%u/%u", &k, &l) != 2)
    {
        x265_log(param, X265_LOG_ERROR, "fps specified in analysis file is not valid\n");
        X265_FREE(paramBuf);
        return false;
    }
    if (k != param->fpsNum || l != param->fpsDenom)
    {
        x265_log(param, X265_LOG_ERROR, "fps mismatch than given in analysis file (%u/%u vs %u/%u)\n",
            param->fpsNum, param->fpsDenom, k, l);
        X265_FREE(paramBuf);
        return false;
    }

    CMP_OPT_FANOUT("bitdepth", param->internalBitDepth);
    CMP_OPT_FANOUT("weightp", param->bEnableWeightedPred);
    CMP_OPT_FANOUT("bframes", param->bframes);
    CMP_OPT_FANOUT("b-pyramid", param->bBPyramid);
    CMP_OPT_FANOUT("b-adapt", param->bFrameAdaptive);
    CMP_OPT_FANOUT("open-gop", param->bOpenGOP);
    CMP_OPT_FANOUT("keyint", param->keyframeMax);
    CMP_OPT_FANOUT("min-keyint", param->keyframeMin);
    CMP_OPT_FANOUT("scenecut", param->scenecutThreshold);
    CMP_OPT_FANOUT("ctu", (int)param->maxCUSize);
    CMP_OPT_FANOUT("ref", param->maxNumReferences);
    CMP_OPT_FANOUT("rc-lookahead", param->lookaheadDepth);

#undef CMP_OPT_FANOUT

    X265_FREE(paramBuf);
    return true;
}

void CLIOptions::readAnalysisFile(x265_picture* pic, x265_param* p)
{
    int poc, width, height;
    uint32_t numPart, numCU;
    fread(&width, sizeof(int), 1, this->analysisFile);
    fread(&height, sizeof(int), 1, this->analysisFile);
    fread(&poc, sizeof(int), 1, this->analysisFile);
    fread(&pic->sliceType, sizeof(int), 1, this->analysisFile);
    fread(&numCU, sizeof(int), 1, this->analysisFile);
    fread(&numPart, sizeof(int), 1, this->analysisFile);

    if (poc != pic->poc || width != p->sourceWidth || height != p->sourceHeight)
    {
        x265_log(NULL, X265_LOG_WARNING, "Error in reading intra-inter data.\n");
        x265_free_analysis_data(pic);
        return;
    }

    fread(pic->analysisData.intraData->depth,
        sizeof(uint8_t), pic->analysisData.numPartitions * pic->analysisData.numCUsInFrame, this->analysisFile);
    fread(pic->analysisData.intraData->modes,
        sizeof(uint8_t), pic->analysisData.numPartitions * pic->analysisData.numCUsInFrame, this->analysisFile);
    fread(pic->analysisData.intraData->partSizes,
        sizeof(char), pic->analysisData.numPartitions * pic->analysisData.numCUsInFrame, this->analysisFile);
    fread(pic->analysisData.intraData->poc,
        sizeof(int), pic->analysisData.numCUsInFrame, this->analysisFile);
    fread(pic->analysisData.intraData->cuAddr,
        sizeof(uint32_t), pic->analysisData.numCUsInFrame, this->analysisFile);
    fread(pic->analysisData.interData, sizeof(x265_inter_data), pic->analysisData.numCUsInFrame * (X265_MAX_PRED_MODE_PER_CU), this->analysisFile);
}

void CLIOptions::writeAnalysisFile(x265_picture* pic, x265_param *p)
{
    uint64_t seekTo = pic->poc * this->analysisRecordSize + this->analysisHeaderSize;
    fseeko(this->analysisFile, seekTo, SEEK_SET);
    fwrite(&p->sourceWidth, sizeof(int), 1, this->analysisFile);
    fwrite(&p->sourceHeight, sizeof(int), 1, this->analysisFile);
    fwrite(&pic->poc, sizeof(int), 1, this->analysisFile);
    fwrite(&pic->sliceType, sizeof(int), 1, this->analysisFile);
    fwrite(&pic->analysisData.numCUsInFrame, sizeof(int), 1, this->analysisFile);
    fwrite(&pic->analysisData.numPartitions, sizeof(int), 1, this->analysisFile);

    fwrite(pic->analysisData.intraData->depth,
        sizeof(uint8_t), pic->analysisData.numPartitions * pic->analysisData.numCUsInFrame, this->analysisFile);
    fwrite(pic->analysisData.intraData->modes,
        sizeof(uint8_t), pic->analysisData.numPartitions * pic->analysisData.numCUsInFrame, this->analysisFile);
    fwrite(pic->analysisData.intraData->partSizes,
        sizeof(char), pic->analysisData.numPartitions * pic->analysisData.numCUsInFrame, this->analysisFile);
    fwrite(pic->analysisData.intraData->poc, sizeof(int), pic->analysisData.numCUsInFrame, this->analysisFile);
    fwrite(pic->analysisData.intraData->cuAddr, sizeof(uint32_t), pic->analysisData.numCUsInFrame, this->analysisFile);
    fwrite(pic->analysisData.interData, sizeof(x265_inter_data), pic->analysisData.numCUsInFrame * X265_MAX_PRED_MODE_PER_CU, this->analysisFile);
}

bool CLIOptions::parseQPFile(x265_picture &pic_org)
{
    int32_t num = -1, qp, ret;
    char type;
    uint32_t filePos;
    pic_org.forceqp = 0;
    pic_org.sliceType = X265_TYPE_AUTO;
    while (num < pic_org.poc)
    {
        filePos = ftell(qpfile);
        qp = -1;
        ret = fscanf(qpfile, "%d %c%*[ \t]%d\n", &num, &type, &qp);

        if (num > pic_org.poc || ret == EOF)
        {
            fseek(qpfile, filePos, SEEK_SET);
            break;
        }
        if (num < pic_org.poc && ret >= 2)
            continue;
        if (ret == 3 && qp >= 0)
            pic_org.forceqp = qp + 1;
        if (type == 'I') pic_org.sliceType = X265_TYPE_IDR;
        else if (type == 'i') pic_org.sliceType = X265_TYPE_I;
        else if (type == 'P') pic_org.sliceType = X265_TYPE_P;
        else if (type == 'B') pic_org.sliceType = X265_TYPE_BREF;
        else if (type == 'b') pic_org.sliceType = X265_TYPE_B;
        else ret = 0;
        if (ret < 2 || qp < -1 || qp > 51)
            return 0;
    }
    return 1;
}

int main(int argc, char **argv)
{
#if HAVE_VLD
    // This uses Microsoft's proprietary WCHAR type, but this only builds on Windows to start with
    VLDSetReportOptions(VLD_OPT_REPORT_TO_DEBUGGER | VLD_OPT_REPORT_TO_FILE, L"x265_leaks.txt");
#endif
    PPA_INIT();

    x265_param *param = x265_param_alloc();
    CLIOptions cliopt;

    if (cliopt.parse(argc, argv, param))
    {
        cliopt.destroy();
        x265_param_free(param);
        exit(1);
    }

    x265_encoder *encoder = x265_encoder_open(param);
    if (!encoder)
    {
        x265_log(param, X265_LOG_ERROR, "failed to open encoder\n");
        cliopt.destroy();
        x265_param_free(param);
        x265_cleanup();
        exit(1);
    }

    /* get the encoder parameters post-initialization */
    x265_encoder_parameters(encoder, param);

    /* Control-C handler */
    if (signal(SIGINT, sigint_handler) == SIG_ERR)
        x265_log(param, X265_LOG_ERROR, "Unable to register CTRL+C handler: %s\n", strerror(errno));

    x265_picture pic_orig, pic_out;
    x265_picture *pic_in = &pic_orig;
    x265_picture *pic_recon = cliopt.recon ? &pic_out : NULL;
    uint32_t inFrameCount = 0;
    uint32_t outFrameCount = 0;
    x265_nal *p_nal;
    x265_stats stats;
    uint32_t nal;
    int16_t *errorBuf = NULL;

    if (!param->bRepeatHeaders)
    {
        if (x265_encoder_headers(encoder, &p_nal, &nal) < 0)
        {
            x265_log(param, X265_LOG_ERROR, "Failure generating stream headers\n");
            goto fail;
        }
        else
            cliopt.writeNALs(p_nal, nal);
    }

    x265_picture_init(param, pic_in);

    if (param->analysisMode && !pic_recon)
    {
        x265_log(NULL, X265_LOG_ERROR, "Must specify recon with analysis-mode option.\n");
        goto fail;
    }
    if (param->analysisMode)
    {
        if (param->analysisMode == X265_ANALYSIS_SAVE)
        {
            char *p = x265_param2string(param);
            if (!p)
            {
                x265_log(NULL, X265_LOG_ERROR, "analysis: buffer allocation failure, aborting");
                goto fail;
            }
            uint32_t numCU = pic_in->analysisData.numCUsInFrame;
            uint32_t numPart = pic_in->analysisData.numPartitions;

            cliopt.analysisRecordSize = ((sizeof(int) * 4 + sizeof(uint32_t) * 2) + sizeof(x265_inter_data) * numCU * X265_MAX_PRED_MODE_PER_CU +
                    sizeof(uint8_t) * 2 * numPart * numCU + sizeof(char) * numPart * numCU + sizeof(int) * numCU + sizeof(uint32_t) * numCU);

            fprintf(cliopt.analysisFile, "#options: %s\n", p);
            cliopt.analysisHeaderSize = ftell(cliopt.analysisFile);
            X265_FREE(p);
        }
        else
        {
            if (!cliopt.validateFanout(param))
                goto fail;
        }
    }

    if (cliopt.bDither)
    {
        errorBuf = X265_MALLOC(int16_t, param->sourceWidth + 1);
        if (errorBuf)
            memset(errorBuf, 0, (param->sourceWidth + 1) * sizeof(int16_t));
        else
            cliopt.bDither = false;
    }

    // main encoder loop
    while (pic_in && !b_ctrl_c)
    {
        pic_orig.poc = inFrameCount;
        if (cliopt.qpfile && !param->rc.bStatRead)
        {
            if (!cliopt.parseQPFile(pic_orig))
            {
                x265_log(NULL, X265_LOG_ERROR, "can't parse qpfile for frame %d\n", pic_in->poc);
                fclose(cliopt.qpfile);
                cliopt.qpfile = NULL;
            }
        }

        if (cliopt.framesToBeEncoded && inFrameCount >= cliopt.framesToBeEncoded)
            pic_in = NULL;
        else if (cliopt.input->readPicture(pic_orig))
            inFrameCount++;
        else
            pic_in = NULL;

        if (pic_in)
        {
            if (pic_in->bitDepth > X265_DEPTH && cliopt.bDither)
            {
                ditherImage(*pic_in, param->sourceWidth, param->sourceHeight, errorBuf, X265_DEPTH);
                pic_in->bitDepth = X265_DEPTH;
            }
            if (param->analysisMode)
            {
                x265_alloc_analysis_data(pic_in);

                if (param->analysisMode == X265_ANALYSIS_LOAD)
                    cliopt.readAnalysisFile(pic_in, param);
            }
        }

        int numEncoded = x265_encoder_encode(encoder, &p_nal, &nal, pic_in, pic_recon);
        if (numEncoded < 0)
        {
            b_ctrl_c = 1;
            break;
        }
        outFrameCount += numEncoded;
        if (numEncoded && pic_recon)
        {
            cliopt.recon->writePicture(pic_out);
            if (param->analysisMode == X265_ANALYSIS_SAVE)
                cliopt.writeAnalysisFile(pic_recon, param);
            if (param->analysisMode)
                x265_free_analysis_data(pic_recon);
        }

        if (nal)
            cliopt.writeNALs(p_nal, nal);

        // Because x265_encoder_encode() lazily encodes entire GOPs, updates are per-GOP
        cliopt.printStatus(outFrameCount, param);
    }

    /* Flush the encoder */
    while (!b_ctrl_c)
    {
        uint32_t numEncoded = x265_encoder_encode(encoder, &p_nal, &nal, NULL, pic_recon);
        outFrameCount += numEncoded;
        if (numEncoded && pic_recon)
        {
            cliopt.recon->writePicture(pic_out);
            if (param->analysisMode == X265_ANALYSIS_SAVE)
                cliopt.writeAnalysisFile(pic_recon, param);
            if (param->analysisMode)
                x265_free_analysis_data(pic_recon);
        }

        if (nal)
            cliopt.writeNALs(p_nal, nal);

        cliopt.printStatus(outFrameCount, param);

        if (!numEncoded)
            break;
    }

    /* clear progress report */
    if (cliopt.bProgress)
        fprintf(stderr, "%*s\r", 80, " ");

fail:
    x265_encoder_get_stats(encoder, &stats, sizeof(stats));
    if (param->csvfn && !b_ctrl_c)
        x265_encoder_log(encoder, argc, argv);
    x265_encoder_close(encoder);
    cliopt.bitstreamFile.close();

    if (b_ctrl_c)
        fprintf(stderr, "aborted at input frame %d, output frame %d\n",
                cliopt.seek + inFrameCount, stats.encodedPictureCount);

    if (stats.encodedPictureCount)
    {
        printf("\nencoded %d frames in %.2fs (%.2f fps), %.2f kb/s", stats.encodedPictureCount,
               stats.elapsedEncodeTime, stats.encodedPictureCount / stats.elapsedEncodeTime, stats.bitrate);

        if (param->bEnablePsnr)
            printf(", Global PSNR: %.3f", stats.globalPsnr);

        if (param->bEnableSsim)
            printf(", SSIM Mean Y: %.7f (%6.3f dB)", stats.globalSsim, x265_ssim2dB(stats.globalSsim));

        printf("\n");
    }
    else
    {
        printf("\nencoded 0 frames\n");
    }

    x265_cleanup(); /* Free library singletons */

    cliopt.destroy();

    x265_param_free(param);

    X265_FREE(errorBuf);

#if HAVE_VLD
    assert(VLDReportLeaks() == 0);
#endif
    return 0;
}
