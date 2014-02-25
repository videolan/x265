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
#pragma warning(disable: 4127) // conditional expression is constant, yes I know
#endif

#include "input/input.h"
#include "output/output.h"
#include "common.h"
#include "x265.h"
#include "param.h"

#if HAVE_VLD
/* Visual Leak Detector */
#include <vld.h>
#endif
#include "PPA/ppa.h"

#include <time.h>

#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
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
    { "cpuid",          required_argument, NULL, 0 },
    { "threads",        required_argument, NULL, 0 },
    { "preset",         required_argument, NULL, 'p' },
    { "tune",           required_argument, NULL, 't' },
    { "frame-threads",  required_argument, NULL, 'F' },
    { "log",            required_argument, NULL, 0 },
    { "csv",            required_argument, NULL, 0 },
    { "y4m",                  no_argument, NULL, 0 },
    { "no-progress",          no_argument, NULL, 0 },
    { "output",         required_argument, NULL, 'o' },
    { "input",          required_argument, NULL, 0 },
    { "input-depth",    required_argument, NULL, 0 },
    { "input-res",      required_argument, NULL, 0 },
    { "input-csp",      required_argument, NULL, 0 },
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
    { "no-constrained-intra", no_argument, NULL, 0 },
    { "constrained-intra",    no_argument, NULL, 0 },
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
    { "no-b-pyramid",         no_argument, NULL, 0 },
    { "b-pyramid",            no_argument, NULL, 0 },
    { "ref",            required_argument, NULL, 0 },
    { "no-weightp",           no_argument, NULL, 0 },
    { "weightp",              no_argument, NULL, 'w' },
    { "crf",            required_argument, NULL, 0 },
    { "vbv-maxrate",    required_argument, NULL, 0 },
    { "vbv-bufsize",    required_argument, NULL, 0 },
    { "vbv-init",       required_argument, NULL, 0 },
    { "bitrate",        required_argument, NULL, 0 },
    { "qp",             required_argument, NULL, 'q' },
    { "aq-mode",        required_argument, NULL, 0 },
    { "aq-strength",    required_argument, NULL, 0 },
    { "cbqpoffs",       required_argument, NULL, 0 },
    { "crqpoffs",       required_argument, NULL, 0 },
    { "rd",             required_argument, NULL, 0 },
    { "no-signhide",          no_argument, NULL, 0 },
    { "signhide",             no_argument, NULL, 0 },
    { "no-lft",               no_argument, NULL, 0 },
    { "lft",                  no_argument, NULL, 0 },
    { "no-sao",               no_argument, NULL, 0 },
    { "sao",                  no_argument, NULL, 0 },
    { "sao-lcu-bounds", required_argument, NULL, 0 },
    { "sao-lcu-opt",    required_argument, NULL, 0 },
    { "no-ssim",              no_argument, NULL, 0 },
    { "ssim",                 no_argument, NULL, 0 },
    { "no-psnr",              no_argument, NULL, 0 },
    { "psnr",                 no_argument, NULL, 0 },
    { "hash",           required_argument, NULL, 0 },
    { "no-strong-intra-smoothing", no_argument, NULL, 0 },
    { "strong-intra-smoothing",    no_argument, NULL, 0 },
    { "no-cutree",                 no_argument, NULL, 0 },
    { "cutree",                    no_argument, NULL, 0 },
    { "vui",                  no_argument, NULL, 0 },
    { "sar",            required_argument, NULL, 0 },
    { "overscan",       required_argument, NULL, 0 },
    { "videoformat",    required_argument, NULL, 0 },
    { "range",          required_argument, NULL, 0 },
    { "colorprim",      required_argument, NULL, 0 },
    { "transfer",       required_argument, NULL, 0 },
    { "colormatrix",    required_argument, NULL, 0 },
    { "chromaloc",      required_argument, NULL, 0 },
    { "fieldseq",             no_argument, NULL, 0 },
    { "no-fieldseq",          no_argument, NULL, 0 },
    { "framefieldinfo",       no_argument, NULL, 0 },
    { "no-framefieldinfo",    no_argument, NULL, 0 },
    { "crop-rect",      required_argument, NULL, 0 },
    { "timinginfo",           no_argument, NULL, 0 },
    { "nal-hrd",              no_argument, NULL, 0 },
    { "bitstreamrestriction", no_argument, NULL, 0 },
    { "subpichrd",            no_argument, NULL, 0 },
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

    uint32_t seek;              // number of frames to skip from the beginning
    uint32_t framesToBeEncoded; // number of frames to encode
    uint64_t totalbytes;

    int64_t startTime;
    int64_t prevUpdateTime;
    float   frameRate;

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
    }

    void destroy();
    void writeNALs(const x265_nal* nal, uint32_t nalcount);
    void printStatus(uint32_t frameNum, x265_param *param);
    void printVersion(x265_param *param);
    void showHelp(x265_param *param);
    bool parse(int argc, char **argv, x265_param* param);
};

void CLIOptions::destroy()
{
    if (input)
        input->release();
    input = NULL;
    if (recon)
        recon->release();
    recon = NULL;
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
    if (framesToBeEncoded)
    {
        float bitrate = 0.008f * totalbytes / ((float)frameNum / ((float)param->fpsNum / param->fpsDenom));
        int eta = (int)(elapsed * (framesToBeEncoded - frameNum) / ((int64_t)frameNum * 1000000));
        sprintf(buf, "x265 [%.1f%%] %d/%d frames, %.2f fps, %.2f kb/s, eta %d:%02d:%02d",
                100. * frameNum / framesToBeEncoded, frameNum, framesToBeEncoded, fps, bitrate,
                eta / 3600, (eta / 60) % 60, eta % 60);
    }
    else
    {
        float bitrate = (float)totalbytes * 8 / ((float)1000 * param->fpsNum / param->fpsDenom);
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
    x265_setup_primitives(param, -1);
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
    H0("-h/--h                           Show this help text and exit\n");
    H0("-V/--version                     Show version info and exit\n");
    H0("   --cpuid                       Limit SIMD capability bitmap 0:auto 1:None. Default:0\n");
    H0("   --threads                     Number of threads for thread pool (0: detect CPU core count, default)\n");
    H0("-p/--preset                      ultrafast, veryfast, faster, fast, medium, slow, slower, veryslow, or placebo\n");
    H0("-t/--tune                        Tune the settings for a particular type of source or situation: (psnr, ssim, zero-latency)\n");
    H0("-F/--frame-threads               Number of concurrently encoded frames. 0: auto-determined by core count\n");
    H0("   --log                         Logging level 0:ERROR 1:WARNING 2:INFO 3:DEBUG -1:NONE. Default %d\n", param->logLevel);
    H0("   --csv                         Comma separated log file, log level >= 3 frame log, else one line per run\n");
    H0("   --y4m                         Parse input stream as YUV4MPEG2 regardless of file extension\n");
    H0("   --no-progress                 Disable CLI progress reports\n");
    H0("-o/--output                      Bitstream output file name\n");
    H0("\nInput Options:\n");
    H0("   --input                       Raw YUV or Y4M input file name\n");
    H0("   --input-depth                 Bit-depth of input file. Default 8\n");
    H0("   --input-res                   Source picture size [w x h], auto-detected if Y4M\n");
    H0("   --input-csp                   Source color space parameter, auto-detected if Y4M. 1:i420 3:i444. Default: 1\n");
    H0("   --fps                         Source frame rate (float or num/denom), auto-detected if Y4M\n");
    H0("   --seek                        First frame to encode\n");
    H0("-f/--frames                      Maximum number of frames to encode. Default all\n");
    H0("\nQuad-Tree analysis:\n");
    H0("   --[no-]wpp                    Enable Wavefront Parallel Processing. Default %s\n", OPT(param->bEnableWavefront));
    H0("-s/--ctu                         Maximum CU size (default: 64x64). Default %d\n", param->maxCUSize);
    H0("   --tu-intra-depth              Max TU recursive depth for intra CUs. Default %d\n", param->tuQTMaxIntraDepth);
    H0("   --tu-inter-depth              Max TU recursive depth for inter CUs. Default %d\n", param->tuQTMaxInterDepth);
    H0("\nTemporal / motion search options:\n");
    H0("   --me                          Motion search method 0:dia 1:hex 2:umh 3:star 4:full. Default %d\n", param->searchMethod);
    H0("-m/--subme                       Amount of subpel refinement to perform (0:least .. 7:most). Default %d \n", param->subpelRefine);
    H0("   --merange                     Motion search range. Default %d\n", param->searchRange);
    H0("   --[no-]rect                   Enable rectangular motion partitions Nx2N and 2NxN. Default %s\n", OPT(param->bEnableRectInter));
    H0("   --[no-]amp                    Enable asymmetric motion partitions, requires --rect. Default %s\n", OPT(param->bEnableAMP));
    H0("   --max-merge                   Maximum number of merge candidates. Default %d\n", param->maxNumMergeCand);
    H0("   --[no-]early-skip             Enable early SKIP detection. Default %s\n", OPT(param->bEnableEarlySkip));
    H0("   --[no-]fast-cbf               Enable Cbf fast mode. Default %s\n", OPT(param->bEnableCbfFastMode));
    H0("\nSpatial / intra options:\n");
    H0("   --rdpenalty                   penalty for 32x32 intra TU in non-I slices. 0:disabled 1:RD-penalty 2:maximum. Default %d\n", param->rdPenalty);
    H0("   --[no-]tskip                  Enable intra transform skipping. Default %s\n", OPT(param->bEnableTransformSkip));
    H0("   --[no-]tskip-fast             Enable fast intra transform skipping. Default %s\n", OPT(param->bEnableTSkipFast));
    H0("   --[no-]strong-intra-smoothing Enable strong intra smoothing for 32x32 blocks. Default %s\n", OPT(param->bEnableStrongIntraSmoothing));
    H0("   --[no-]constrained-intra      Constrained intra prediction (use only intra coded reference pixels) Default %s\n", OPT(param->bEnableConstrainedIntra));
    H0("\nSlice decision options:\n");
    H0("   --[no-]open-gop               Enable open-GOP, allows I slices to be non-IDR. Default %s\n", OPT(param->bOpenGOP));
    H0("-I/--keyint                      Max IDR period in frames. -1 for infinite-gop. Default %d\n", param->keyframeMax);
    H0("-i/--min-keyint                  Scenecuts closer together than this are coded as I, not IDR. Default: auto\n");
    H0("   --no-scenecut                 Disable adaptive I-frame decision\n");
    H0("   --scenecut                    How aggressively to insert extra I-frames. Default %d\n", param->scenecutThreshold);
    H0("   --rc-lookahead                Number of frames for frame-type lookahead (determines encoder latency) Default %d\n", param->lookaheadDepth);
    H0("   --bframes                     Maximum number of consecutive b-frames (now it only enables B GOP structure) Default %d\n", param->bframes);
    H0("   --bframe-bias                 Bias towards B frame decisions. Default %d\n", param->bFrameBias);
    H0("   --b-adapt                     0 - none, 1 - fast, 2 - full (trellis) adaptive B frame scheduling. Default %d\n", param->bFrameAdaptive);
    H0("   --[no-]b-pyramid              Use B-frames as references. Default %s\n", OPT(param->bBPyramid));
    H0("   --ref                         max number of L0 references to be allowed (1 .. 16) Default %d\n", param->maxNumReferences);
    H0("-w/--[no-]weightp                Enable weighted prediction in P slices. Default %s\n", OPT(param->bEnableWeightedPred));
    H0("\nQP, rate control and rate distortion options:\n");
    H0("   --bitrate                     Target bitrate (kbps), implies ABR. Default %d\n", param->rc.bitrate);
    H0("   --crf                         Quality-based VBR (0-51). Default %f\n", param->rc.rfConstant);
    H0("   --vbv-maxrate                 Max local bitrate (kbit/s). Default %d\n", param->rc.vbvMaxBitrate);
    H0("   --vbv-bufsize                 Set size of the VBV buffer (kbit). Default %d\n", param->rc.vbvBufferSize);
    H0("   --vbv-init                    Initial VBV buffer occupancy. Default %f\n", param->rc.vbvBufferInit);
    H0("-q/--qp                          Base QP for CQP mode. Default %d\n", param->rc.qp);
    H0("   --aq-mode                     Mode for Adaptive Quantization - 0:none 1:aqVariance Default %d\n", param->rc.aqMode);
    H0("   --aq-strength                 Reduces blocking and blurring in flat and textured areas.(0 to 3.0). Default %f\n", param->rc.aqStrength);
    H0("   --[no-]cutree                 Enable cutree for Adaptive Quantization. Default %s\n", OPT(param->rc.cuTree));
    H0("   --cbqpoffs                    Chroma Cb QP Offset. Default %d\n", param->cbQpOffset);
    H0("   --crqpoffs                    Chroma Cr QP Offset. Default %d\n", param->crQpOffset);
    H0("   --rd                          Level of RD in mode decision 0:least....6:full RDO. Default %d\n", param->rdLevel);
    H0("   --[no-]signhide               Hide sign bit of one coeff per TU (rdo). Default %s\n", OPT(param->bEnableSignHiding));
    H0("\nLoop filter:\n");
    H0("   --[no-]lft                    Enable Loop Filter. Default %s\n", OPT(param->bEnableLoopFilter));
    H0("\nSample Adaptive Offset loop filter:\n");
    H0("   --[no-]sao                    Enable Sample Adaptive Offset. Default %s\n", OPT(param->bEnableSAO));
    H0("   --sao-lcu-bounds              0: right/bottom boundary areas skipped  1: non-deblocked pixels are used. Default %d\n", param->saoLcuBoundary);
    H0("   --sao-lcu-opt                 0: SAO picture-based optimization, 1: SAO LCU-based optimization. Default %d\n", param->saoLcuBasedOptimization);
    H0("\nQuality reporting metrics:\n");
    H0("   --[no-]ssim                   Enable reporting SSIM metric scores. Default %s\n", OPT(param->bEnableSsim));
    H0("   --[no-]psnr                   Enable reporting PSNR metric scores. Default %s\n", OPT(param->bEnablePsnr));
    H0("\nReconstructed video options (debugging):\n");
    H0("-r/--recon                       Reconstructed raw image YUV or Y4M output file name\n");
    H0("   --recon-depth                 Bit-depth of reconstructed raw image file. Defaults to input bit depth\n");
    H0("\nSEI options:\n");
    H0("   --hash                        Decoded Picture Hash SEI 0: disabled, 1: MD5, 2: CRC, 3: Checksum. Default %d\n", param->decodedPictureHashSEI);
    H0("\nVUI options:\n");
    H0("   --vui                         Add Video Useability Information with all fields to the SPS. Default %s\n", OPT(param->bEnableVuiParametersPresentFlag));
    H0("   --sar <int:int|int>           Sample Aspect Ratio, the ratio of width to height of an individual pixel.\n");
    H0("                                 Choose from 0=undef, 1=1:1(\"square\"), 2=12:11, 3=10:11, 4=16:11,\n");
    H0("                                 5=40:33, 6=24:11, 7=20:11, 8=32:11, 9=80:33, 10=18:11, 11=15:11,\n");
    H0("                                 12=64:33, 13=160:99, 14=4:3, 15=3:2, 16=2:1 or custom ratio of <int:int>. Default %d\n", param->aspectRatioIdc);
    H0("   --overscan <string>           Specify crop overscan setting from undef, show or crop. Default undef\n");
    H0("   --videoformat <string>        Specify video format from undef, component, pal, ntsc, secam, mac. Default undef\n");
    H0("   --range <string>              Specify black level and range of luma and chroma signals as full or limited Default limited\n");
    H0("   --colorprim <string>          Specify color primaries from undef, bt709, bt470m, bt470bg, smpte170m,\n");
    H0("                                 smpte240m, film, bt2020. Default undef\n");
    H0("   --transfer <string>           Specify transfer characteristics from undef, bt709, bt470m, bt470bg, smpte170m,\n");
    H0("                                 smpte240m, linear, log100, log316, iec61966-2-4, bt1361e, iec61966-2-1,\n");
    H0("                                 bt2020-10, bt2020-12. Default undef\n");
    H0("   --colormatrix <string>        Specify color matrix setting from undef, bt709, fcc, bt470bg, smpte170m,\n");
    H0("                                 smpte240m, GBR, YCgCo, bt2020nc, bt2020c. Default undef\n");
    H0("   --chromaloc <integer>         Specify chroma sample location (0 to 5). Default of %d\n", param->chromaSampleLocTypeTopField);
    H0("   --[no-]fieldseq               Specify that pictures are fields and an SEI timing message\n");
    H0("                                 will be added to every access unit. Default %s\n", OPT(param->bEnableFieldSeqFlag));
    H0("   --[no-]framefieldinfo         Specify that a pic-struct will be added to the SEI timing message. Default %s\n", OPT(param->bEnableFrameFieldInfoPresentFlag));
    H0("   --crop-rect <string>          Add 'left,top,right,bottom' to the bitstream-level cropping rectangle\n");
    H0("   --timinginfo                  Add timing information to the VUI. Defaut %s\n", OPT(param->bEnableVuiTimingInfoPresentFlag));
    H0("   --nal-hrd                     Signal HRD information. Default %s\n", OPT(param->bEnableVuiHrdParametersPresentFlag));
    H0("   --bitstreamrestriction        Add bit stream restriction fields to the VUI. Default %s\n", OPT(param->bEnableBitstreamRestrictionFlag));
    H0("   --subpichrd                   Add sub picture HRD parameters to the HRD. Default %s\n", OPT(param->bEnableSubPicHrdParamsPresentFlag));
#undef OPT
#undef H0
    exit(0);
}

bool CLIOptions::parse(int argc, char **argv, x265_param* param)
{
    int berror = 0;
    int help = 0;
    int cpuid = 0;
    int inputBitDepth = 8;
    int reconFileBitDepth = 0;
    const char *inputfn = NULL;
    const char *reconfn = NULL;
    const char *bitstreamfn = NULL;
    const char *preset = "medium";
    const char *tune = "ssim";

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
        x265_log(NULL, X265_LOG_WARNING, "preset or tune unrecognized\n");
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

            if (0) ;
            OPT("cpuid") cpuid = atoi(optarg);
            OPT("frames") this->framesToBeEncoded = (uint32_t)atoi(optarg);
            OPT("no-progress") this->bProgress = false;
            OPT("seek") this->seek = (uint32_t)atoi(optarg);
            OPT("frame-skip") this->seek = (uint32_t)atoi(optarg);
            OPT("output") bitstreamfn = optarg;
            OPT("input") inputfn = optarg;
            OPT("recon") reconfn = optarg;
            OPT("input-res") berror |= sscanf(optarg, "%dx%d", &param->sourceWidth, &param->sourceHeight) != 2;
            OPT("input-depth") inputBitDepth = (uint32_t)atoi(optarg);
            OPT("recon-depth") reconFileBitDepth = (uint32_t)atoi(optarg);
            OPT("no-scenecut") param->scenecutThreshold = 0; // special handling
            OPT("y4m") bForceY4m = true;
            OPT("preset") /* handled above */;
            OPT("tune")   /* handled above */;
            else
                berror |= x265_param_parse(param, long_options[long_options_index].name, optarg);

            if (berror)
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
    info.sarWidth = param->sarWidth;
    info.sarHeight = param->sarHeight;
    info.skipFrames = seek;
    info.frameCount = 0;
    getParamAspectRatio(param, info.sarWidth, info.sarHeight);

    this->input = Input::open(info, bForceY4m);
    if (!this->input || this->input->isFail())
    {
        x265_log(param, X265_LOG_ERROR, "unable to open input file <%s>\n", inputfn);
        return true;
    }
    if (info.csp != X265_CSP_I420 && info.csp != X265_CSP_I444)
    {
        x265_log(param, X265_LOG_ERROR, "Only i420 and i444 color spaces are supported in this build\n");
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
    if (!param->bEnableAspectRatioIdc && info.sarWidth && info.sarHeight)
        setParamAspectRatio(param, info.sarWidth, info.sarHeight);

    if (this->framesToBeEncoded == 0 && (uint32_t)info.frameCount > seek)
        this->framesToBeEncoded = info.frameCount - seek;
    if (param->logLevel >= X265_LOG_INFO)
    {
        char buf[128];
        int p = sprintf(buf, "%dx%d fps %d/%d %s", param->sourceWidth, param->sourceHeight,
                        param->fpsNum, param->fpsDenom, x265_source_csp_names[param->internalCsp]);

        int width, height;
        getParamAspectRatio(param, width, height);
        if (width && height)
            p += sprintf(buf + p, " sar %d:%d", width, height);

        if (framesToBeEncoded == 0 || info.frameCount == 0)
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
            fprintf(stderr, "%s  [info]: reconstructed images %dx%d %d/%d fps %s\n", this->recon->getName(),
                    param->sourceWidth, param->sourceHeight, param->fpsNum, param->fpsDenom,
                    x265_source_csp_names[param->internalCsp]);
    }

    this->bitstreamFile.open(bitstreamfn, std::fstream::binary | std::fstream::out);
    if (!this->bitstreamFile)
    {
        x265_log(NULL, X265_LOG_ERROR, "failed to open bitstream file <%s> for writing\n", bitstreamfn);
        return true;
    }

    x265_setup_primitives(param, cpuid);
    printVersion(param);
    return false;
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
        exit(1);
    }

    x265_encoder *encoder = x265_encoder_open(param);
    if (!encoder)
    {
        x265_log(param, X265_LOG_ERROR, "failed to open encoder\n");
        cliopt.destroy();
        x265_cleanup();
        exit(1);
    }

    /* Control-C handler */
    if (signal(SIGINT, sigint_handler) == SIG_ERR)
        x265_log(param, X265_LOG_ERROR, "Unable to register CTRL+C handler: %s\n", strerror(errno));

    x265_picture pic_orig, pic_out;
    x265_picture *pic_in = &pic_orig;
    x265_picture *pic_recon = cliopt.recon ? &pic_out : NULL;
    x265_nal *p_nal;
    x265_stats stats;
    uint32_t nal;

    if (!x265_encoder_headers(encoder, &p_nal, &nal))
    {
        cliopt.writeNALs(p_nal, nal);
    }

    x265_picture_init(param, pic_in);

    // main encoder loop
    uint32_t inFrameCount = 0;
    uint32_t outFrameCount = 0;
    while (pic_in && !b_ctrl_c)
    {
        pic_orig.poc = inFrameCount;

        if (cliopt.framesToBeEncoded && inFrameCount >= cliopt.framesToBeEncoded)
            pic_in = NULL;
        else if (cliopt.input->readPicture(pic_orig))
            inFrameCount++;
        else
            pic_in = NULL;

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
            printf(", SSIM Mean Y: %.7f (%6.3f dB)", stats.globalSsim, x265_ssim(stats.globalSsim));

        printf("\n");
    }
    else
    {
        printf("\nencoded 0 frames\n");
    }

    x265_cleanup(); /* Free library singletons */

    cliopt.destroy();

    x265_param_free(param);

#if HAVE_VLD
    assert(VLDReportLeaks() == 0);
#endif
    return 0;
}
