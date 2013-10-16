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

#if HAVE_VLD
/* Visual Leak Detector */
#include <vld.h>
#endif
#include "PPA/ppa.h"

#if _WIN32
#include <sys/types.h>
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif
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

static int64_t x265_mdate(void)
{
#if _WIN32
    struct timeb tb;
    ftime(&tb);
    return ((int64_t)tb.time * 1000 + (int64_t)tb.millitm) * 1000;
#else
    struct timeval tv_date;
    gettimeofday(&tv_date, NULL);
    return (int64_t)tv_date.tv_sec * 1000000 + (int64_t)tv_date.tv_usec;
#endif
}

using namespace x265;

static const char short_options[] = "o:f:F:r:i:b:s:q:m:hV";
static struct option long_options[] =
{
#define HELP(message)
#define OPT(longname, var, argreq, flag, helptext) { longname, argreq, NULL, flag },
#define STROPT OPT
#include "x265opts.h"
    { 0, 0, 0, 0 }
#undef OPT
#undef STROPT
#undef HELP
};

#if CU_STAT_LOGFILE
FILE* fp = NULL;
FILE * fp1 = NULL;
#endif

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
    FILE *csvfp;
    int bProgress;
    int cli_log_level;

    uint32_t frameSkip;                 ///< number of frames to skip from the beginning
    uint32_t framesToBeEncoded;         ///< number of frames to encode

    uint32_t essentialBytes;            ///< total essential NAL bytes written to bitstream
    uint32_t totalBytes;                ///< total bytes written to bitstream

    int64_t i_start;
    int64_t i_previous;

    CLIOptions()
    {
        input = NULL;
        recon = NULL;
        csvfp = NULL;
        framesToBeEncoded = frameSkip = 0;
        essentialBytes = 0;
        totalBytes = 0;
        bProgress = true;
        i_start = x265_mdate();
        i_previous = 0;
        cli_log_level = X265_LOG_INFO;
    }

    void destroy()
    {
        if (input)
            input->release();
        input = NULL;
        if (recon)
            recon->release();
        recon = NULL;
        if (csvfp)
            fclose(csvfp);
        csvfp = NULL;
    }

    void rateStatsAccum(NalUnitType nalUnitType, uint32_t annexBsize)
    {
        switch (nalUnitType)
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
            essentialBytes += annexBsize;
            break;
        default:
            break;
        }

        totalBytes += annexBsize;
    }

    void writeNALs(const x265_nal_t* nal, int nalcount)
    {
        PPAScopeEvent(bitstream_write);
        for (int i = 0; i < nalcount; i++)
        {
            bitstreamFile.write((const char*)nal->p_payload, nal->i_payload);
            rateStatsAccum((NalUnitType)nal->i_type, nal->i_payload);
            nal++;
        }
    }

    /* in microseconds */
    static const int UPDATE_INTERVAL = 250000;

    void printStatus(int i_frame, x265_param_t *param)
    {
        char buf[200];
        int64_t i_time = x265_mdate();

        if (!bProgress || (i_previous && i_time - i_previous < UPDATE_INTERVAL))
            return;
        int64_t i_elapsed = i_time - i_start;
        double fps = i_elapsed > 0 ? i_frame * 1000000. / i_elapsed : 0;
        if (framesToBeEncoded && i_frame)
        {
            float bitrate = 0.008f * totalBytes / ((float)i_frame / param->frameRate);
            int eta = (int)(i_elapsed * (framesToBeEncoded - i_frame) / ((int64_t)i_frame * 1000000));
            sprintf(buf, "x265 [%.1f%%] %d/%d frames, %.2f fps, %.2f kb/s, eta %d:%02d:%02d",
                    100. * i_frame / framesToBeEncoded, i_frame, framesToBeEncoded, fps, bitrate,
                    eta / 3600, (eta / 60) % 60, eta % 60);
        }
        else
        {
            double bitrate = (double)totalBytes * 8 / ((double)1000 * param->frameRate);
            sprintf(buf, "x265 %d frames: %.2f fps, %.2f kb/s", i_frame, fps, bitrate);
        }
        fprintf(stderr, "%s  \r", buf + 5);
        SetConsoleTitle(buf);
        fflush(stderr); // needed in windows
        i_previous = i_time;
    }

    void log(int i_level, const char *fmt, ...)
    {
        if (i_level > cli_log_level)
            return;
        const char *s_level;
        switch (i_level)
        {
        case X265_LOG_ERROR:
            s_level = "error";
            break;
        case X265_LOG_WARNING:
            s_level = "warning";
            break;
        case X265_LOG_INFO:
            s_level = "info";
            break;
        case X265_LOG_DEBUG:
            s_level = "debug";
            break;
        default:
            s_level = "unknown";
            break;
        }

        fprintf(stderr, "x265 [%s]: ", s_level);
        va_list arg;
        va_start(arg, fmt);
        vfprintf(stderr, fmt, arg);
        va_end(arg);
    }

    void print_version(x265_param_t *param)
    {
        fprintf(stderr, "x265 [info]: HEVC encoder version %s\n", x265_version_str);
        fprintf(stderr, "x265 [info]: build info %s\n", x265_build_info_str);
        x265_setup_primitives(param, 0);
    }

    void do_help(x265_param_t *param)
    {
        x265_param_default(param);
        print_version(param);
        int inputBitDepth = 8, outputBitDepth = param->internalBitDepth;
#define H0 printf
#define OPT(value) (value ? "Enable" : "Disable")
        H0("\nSyntax: x265 [options] infile [-o] outfile\n");
        H0("    infile can be YUV or Y4M\n");
        H0("    outfile is raw HEVC bitstream\n");
        H0("Standalone Executable Options:\n");
        H0("-h/--h                           Show this help text\n");
        H0("   --cpuid                       Limit SIMD arch 0:auto 1:None 2:SSE2 .. 8:AVX2 \n \t\t\t\t Default :0\n");
        H0("   --threads                     Number of threads for thread pool (0: detect CPU core count \n \t\t\t\t Default : %d\n", param->poolNumThreads);
        H0("-F/--frame-threads               Number of concurrently encoded frames \n \t\t\t\t Default : %d\n", param->frameNumThreads);
        H0("   --log                         Logging level 0:ERROR 1:WARNING 2:INFO 3:DEBUG -1:NONE \n \t\t\t\t Default : %d\n", param->logLevel);
        H0("   --csv                         Comma separated value log file, appends one line per run\n");
        H0("   --no-progress                 Disable progress reports \n \t\t\t\t Default : Enable\n");
        H0("-o/--output                      Bitstream output file name\n");
        H0("Input Options:\n");
        H0("   --input                       Raw YUV or Y4M input file name\n");
        H0("   --input-depth                 Bit-depth of input file (YUV only) \n \t\t\t\t Default : %d\n",inputBitDepth);
        H0("   --input-res                   Source picture size [w x h], auto-detect if Y4M\n");
        H0("   --fps                         Frame rate, auto-detect if Y4M \n \t\t\t\t Default : 0\n");
        H0("   --frame-skip                  Number of frames to skip at start of input file \n \t\t\t\t Default : 0\n");
        H0("-f/--frames                      Number of frames to be encoded (0 implies all)\n");
        H0("Reconstructed video options (debugging):\n");
        H0("-r/--recon                       Reconstructed image YUV or Y4M output file name\n");
        H0("   --recon-depth                 Bit-depth of output file \n \t\t\t\t Default : %d\n", outputBitDepth);
        H0("Quad-Tree analysis:\n");
        H0("   --no-wpp                      Disable Wavefront Parallel Processing\n");
        H0("   --wpp                         Enable Wavefront Parallel Processing \n \t\t\t\t Default : %s\n",OPT(param->bEnableWavefront));
        H0("-s/--ctu                         Maximum CU size (default: 64x64) \n \t\t\t\t Default : %d\n", param->maxCUSize);
        H0("   --tu-intra-depth              Max TU recursive depth for intra CUs \n \t\t\t\t Default : %d\n", param->tuQTMaxIntraDepth);
        H0("   --tu-inter-depth              Max TU recursive depth for inter CUs \n \t\t\t\t Default : %d\n", param->tuQTMaxInterDepth);
        H0("Temporal / motion search options:\n");
        H0("   --me                          Motion search method 0:dia 1:hex 2:umh 3:star 4:full \n \t\t\t\t Default : %d\n", param->searchMethod);
        H0("-m/--subme                       Amount of subpel refinement to perform (0:least .. 7:most) \n \t\t\t\t Default : %d \n", param->subpelRefine);
        H0("   --merange                     Motion search range \n \t\t\t\t Default : %d\n", param->searchRange);
        H0("   --no-rect                     Disable rectangular motion partitions Nx2N and 2NxN\n");
        H0("   --rect                        Enable rectangular motion partitions Nx2N and 2NxN \n \t\t\t\t Default : %s\n",OPT(param->bEnableRectInter));
        H0("   --no-amp                      Disable asymmetric motion partitions\n");
        H0("   --amp                         Enable asymmetric motion partitions, requires --rect \n \t\t\t\t Default : %s\n",OPT(param->bEnableAMP));
        H0("   --max-merge                   Maximum number of merge candidates \n \t\t\t\t Default : %d\n", param->maxNumMergeCand);
        H0("   --no-early-skip               Disable early SKIP detection\n");
        H0("   --early-skip                  Enable early SKIP detection \n \t\t\t\t Default : %s\n", OPT(param->bEnableEarlySkip));
        H0("   --no-fast-cbf                 Disable Cbf fast mode\n");
        H0("   --fast-cbf                    Enable Cbf fast mode \n \t\t\t\t Default : %s\n", OPT(param->bEnableCbfFastMode));
        H0("Spatial / intra options:\n");
        H0("   --rdpenalty                   penalty for 32x32 intra TU in non-I slices. 0:disabled 1:RD-penalty 2:maximum \n \t\t\t\t Default : %d\n", param->rdPenalty);
        H0("   --no-tskip                    Disable intra transform skipping\n");
        H0("   --tskip                       Enable intra transform skipping \n \t\t\t\t Default : %s\n", OPT(param->bEnableTransformSkip));
        H0("   --no-tskip-fast               Disable fast intra transform skipping\n");
        H0("   --tskip-fast                  Enable fast intra transform skipping \n \t\t\t\t Default : %s\n", OPT(param->bEnableTSkipFast));
        H0("   --no-strong-intra-smoothing   Disable strong intra smoothing for 32x32 blocks\n");
        H0("   --strong-intra-smoothing      Enable strong intra smoothing for 32x32 blocks \n \t\t\t\t Default : %s\n", OPT(param->bEnableStrongIntraSmoothing));
        H0("   --no-constrained-intra        Disable constrained intra prediction (use only intra coded reference pixels)\n");
        H0("   --constrained-intra           Constrained intra prediction (use only intra coded reference pixels) \n \t\t\t\t Default : %s\n", OPT(param->bEnableConstrainedIntra));
        H0("Slice decision options:\n");
        H0("   --refresh                     Intra refresh type - 0:none, 1:CDR, 2:IDR (default: CDR) \n \t\t\t\t Default : %d\n", param->decodingRefreshType);
        H0("   -i/--keyint                   Max intra period in frames \n \t\t\t\t Default : %d\n", param->keyframeMax);
        H0("   --rc-lookahead                Number of frames for frame-type lookahead (determines encoder latency) \n \t\t\t\t Default : %d\n", param->lookaheadDepth);
        H0("   --bframes                     Maximum number of consecutive b-frames (now it only enables B GOP structure) \n \t\t\t\t Default : %d\n", param->bframes);
        H0("   --bframe-bias                 Bias towards B frame decisions \n \t\t\t\t Default : %d\n", param->bFrameBias);
        H0("   --b-adapt                     0 - none, 1 - fast, 2 - full (trellis) adaptive B frame scheduling \n \t\t\t\t Default : %d\n", param->bFrameAdaptive);
        H0("   --ref                         max number of L0 references to be allowed  (1 .. 16) \n \t\t\t\t Default : %d\n", param->maxNumReferences);
        H0("   --no-weightp                  Disable weighted prediction in P slices\n");
        H0("-w/--weightp                     Enable weighted prediction in P slices \n \t\t\t\t Default : %s\n", OPT(param->bEnableWeightedPred));
        H0("QP, rate control and rate distortion options:\n");
        H0("   --bitrate                     Target bitrate (kbps), implies ABR \n \t\t\t\t Default : %d\n", param->rc.bitrate);
        H0("-q/--qp                          Base QP for CQP mode \n \t\t\t\t Default : %d\n", param->rc.qp);
        H0("   --cbqpoffs                    Chroma Cb QP Offset \n \t\t\t\t Default : %d\n", param->cbQpOffset);
        H0("   --crqpoffs                    Chroma Cr QP Offset \n \t\t\t\t Default : %d\n", param->crQpOffset);
        H0("   --rd                          Level of RD in mode decision 0:least....2:full RDO \n \t\t\t\t Default : %d\n", param->bRDLevel);
        H0("   --no-signhide                 Disable hide sign bit of one coeff per TU (rdo)");
        H0("   --signhide                    Hide sign bit of one coeff per TU (rdo) \n \t\t\t\t Default : %s\n", OPT(param->bEnableSignHiding));
        H0("Loop filter:\n");
        H0("   --no-lft                      Disable Loop Filter\n");
        H0("   --lft                         Enable Loop Filter \n \t\t\t\t Default : %s\n", OPT(param->bEnableLoopFilter));
        H0("Sample Adaptive Offset loop filter:\n");
        H0("   --no-sao                      Disable Sample Adaptive Offset\n");
        H0("   --sao                         Enable Sample Adaptive Offset \n \t\t\t\t Default : %s\n", OPT(param->bEnableSAO));
        H0("   --sao-lcu-bounds              0: right/bottom boundary areas skipped  1: non-deblocked pixels are used \n \t\t\t\t Default : %d\n", param->saoLcuBoundary);
        H0("   --sao-lcu-opt                 0: SAO picture-based optimization, 1: SAO LCU-based optimization \n \t\t\t\t Default : %d\n", param->saoLcuBasedOptimization);
        H0("Quality reporting metrics:\n");
        H0("   --no-ssim                     Disable reporting SSIM metric scores\n");
        H0("   --ssim                        Enable reporting SSIM metric scores \n \t\t\t\t Default : %s\n", OPT(param->bEnableSsim));
        H0("   --no-psnr                     Disable reporting PSNR metric scores\n");
        H0("   --psnr                        Enable reporting PSNR metric scores \n \t\t\t\t Default : %s\n", OPT(param->bEnablePsnr));
        H0("SEI options:\n");
        H0("   --hash                        Decoded Picture Hash SEI 0: disabled, 1: MD5, 2: CRC, 3: Checksum \n \t\t\t\t Default : %d\n", param->decodedPictureHashSEI);
#undef OPT
#undef H0
        exit(0);
    }

    bool parse(int argc, char **argv, x265_param_t* param)
    {
        int berror = 0;
        int help = 0;
        int cpuid = 0;
        uint32_t inputBitDepth = 8;
        uint32_t outputBitDepth = 8;
        const char *inputfn = NULL;
        const char *reconfn = NULL;
        const char *bitstreamfn = NULL;
        const char *csvfn = NULL;
        const char *inputRes = NULL;

        x265_param_default(param);

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
                do_help(param);
                break;

            case 'V':
                print_version(param);
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
                            log(X265_LOG_WARNING, "internal error: short option '%c' has no long option\n", c);
                        return true;
                    }
                }
                if (long_options_index < 0)
                {
                    log(X265_LOG_WARNING, "short option '%c' unrecognized\n", c);
                    return true;
                }
#define OPT(longname) \
    else if (!strcmp(long_options[long_options_index].name, longname))

            if (0);
                OPT("frames") this->framesToBeEncoded = (uint32_t)atoi(optarg);
                OPT("no-progress") this->bProgress = false;
                OPT("frame-skip") this->frameSkip = (uint32_t)atoi(optarg);
                OPT("csv") csvfn = optarg;
                OPT("output") bitstreamfn = optarg;
                OPT("input") inputfn = optarg;
                OPT("recon") reconfn = optarg;
                OPT("input-depth") inputBitDepth = (uint32_t)atoi(optarg);
                OPT("recon-depth") outputBitDepth = (uint32_t)atoi(optarg);
                OPT("input-res") inputRes = optarg;
            else
                berror |= x265_param_parse(param, long_options[long_options_index].name, optarg);

            if (berror)
            {
                const char *name = long_options_index > 0 ? long_options[long_options_index].name : argv[optind-2];
                log(X265_LOG_ERROR, "invalid argument: %s = %s\n", name, optarg);
                return true;
            }
#undef OPT
            }
        }

        cli_log_level = param->logLevel;
        if (optind < argc && !inputfn)
            inputfn = argv[optind++];
        if (optind < argc && !bitstreamfn)
            bitstreamfn = argv[optind++];
        if (optind < argc)
        {
            log(X265_LOG_WARNING, "extra unused command arguments given <%s>\n", argv[optind]);
            return true;
        }

        if (argc <= 1 || help)
            do_help(param);

        if (inputfn == NULL || bitstreamfn == NULL)
        {
            log(X265_LOG_ERROR, "input or output file not specified, try -V for help\n");
            return true;
        }
        this->input = Input::open(inputfn);
        if (!this->input || this->input->isFail())
        {
            log(X265_LOG_ERROR, "unable to open input file <%s>\n", inputfn);
            return true;
        }
        if (this->input->getWidth())
        {
            /* parse the width, height, frame rate from the y4m file */
            param->sourceWidth = this->input->getWidth();
            param->sourceHeight = this->input->getHeight();
            param->frameRate = (int)this->input->getRate();
            inputBitDepth = 8;
        }
        else if (inputRes)
        {
            sscanf(inputRes, "%dx%d", &param->sourceWidth, &param->sourceHeight);
            this->input->setDimensions(param->sourceWidth, param->sourceHeight);
            this->input->setBitDepth(inputBitDepth);
        }
        else if (param->sourceHeight <= 0 || param->sourceWidth <= 0 || param->frameRate <= 0)
        {
            log(X265_LOG_ERROR, "YUV input requires source width, height, and rate to be specified\n");
            return true;
        }
        else
        {
            this->input->setDimensions(param->sourceWidth, param->sourceHeight);
            this->input->setBitDepth(inputBitDepth);
        }

        /* rules for input, output and internal bitdepths as per help text */
        if (!param->internalBitDepth) { param->internalBitDepth = inputBitDepth; }
        if (!outputBitDepth) { outputBitDepth = param->internalBitDepth; }

        uint32_t numRemainingFrames = (uint32_t) this->input->guessFrameCount();

        if (this->frameSkip)
        {
            this->input->skipFrames(this->frameSkip);
        }

        this->framesToBeEncoded = this->framesToBeEncoded ? X265_MIN(this->framesToBeEncoded, numRemainingFrames) : numRemainingFrames;

        if (this->cli_log_level >= X265_LOG_INFO)
        {
            fprintf(stderr, "%s  [info]: %dx%d %dHz, frames %u - %d of %d\n", input->getName(),
                    param->sourceWidth, param->sourceHeight, param->frameRate,
                    this->frameSkip, this->frameSkip + this->framesToBeEncoded - 1, numRemainingFrames);
        }

        if (reconfn)
        {
            this->recon = Output::open(reconfn, param->sourceWidth, param->sourceHeight, outputBitDepth, param->frameRate);
            if (this->recon->isFail())
            {
                log(X265_LOG_WARNING, "unable to write reconstruction file\n");
                this->recon->release();
                this->recon = 0;
            }
        }

#if !HIGH_BIT_DEPTH
        if (inputBitDepth != 8 || outputBitDepth != 8 || param->internalBitDepth != 8)
        {
            log(X265_LOG_ERROR, "not compiled for bit depths greater than 8\n");
            return true;
        }
#endif

        this->bitstreamFile.open(bitstreamfn, std::fstream::binary | std::fstream::out);
        if (!this->bitstreamFile)
        {
            log(X265_LOG_ERROR, "failed to open bitstream file <%s> for writing\n", bitstreamfn);
            return true;
        }

        if (csvfn)
        {
            csvfp = fopen(csvfn, "r");
            if (csvfp)
            {
                // file already exists, re-open for append
                fclose(csvfp);
                csvfp = fopen(csvfn, "ab");
            }
            else
            {
                // new CSV file, write header
                csvfp = fopen(csvfn, "wb");
                if (csvfp)
                {
                    fprintf(csvfp, "CLI arguments, date/time, elapsed time, fps, bitrate, global PSNR, version\n");
                }
            }
        }
        x265_setup_primitives(param, cpuid);

        return false;
    }
};

int main(int argc, char **argv)
{
#if HAVE_VLD
    // This uses Microsoft's proprietary WCHAR type, but this only builds on Windows to start with
    VLDSetReportOptions(VLD_OPT_REPORT_TO_DEBUGGER | VLD_OPT_REPORT_TO_FILE, L"x265_leaks.txt");
#endif
    PPA_INIT();

#if CU_STAT_LOGFILE
    fp = fopen("Log_CU_stats.txt", "w");
    fp1 = fopen("LOG_CU_COST.txt", "w");
#endif

    x265_param_t param;
    CLIOptions   cliopt;

    if (cliopt.parse(argc, argv, &param))
    {
        cliopt.destroy();
        exit(1);
    }

    x265_t *encoder = x265_encoder_open(&param);
    if (!encoder)
    {
        cliopt.log(X265_LOG_ERROR, "failed to open encoder\n");
        cliopt.destroy();
        x265_cleanup();
        exit(1);
    }

    /* Control-C handler */
    if (signal(SIGINT, sigint_handler) == SIG_ERR)
        cliopt.log(X265_LOG_ERROR, "Unable to register CTRL+C handler: %s\n", strerror(errno));

    x265_picture_t pic_orig, pic_out;
    x265_picture_t *pic_in = &pic_orig;
    x265_picture_t *pic_recon = cliopt.recon ? &pic_out : NULL;
    x265_nal_t *p_nal;
    x265_stats_t stats;
    int nal;

    if (!x265_encoder_headers(encoder, &p_nal, &nal))
    {
        cliopt.writeNALs(p_nal, nal);
    }

    x265_picture_init(&param, pic_in);

    // main encoder loop
    uint32_t inFrameCount = 0;
    uint32_t outFrameCount = 0;
    while (pic_in && !b_ctrl_c)
    {
        pic_orig.poc = inFrameCount;

        // read input YUV file
        if (inFrameCount < cliopt.framesToBeEncoded && cliopt.input->readPicture(pic_orig))
            inFrameCount++;
        else
            pic_in = NULL;

        int numEncoded = x265_encoder_encode(encoder, &p_nal, &nal, pic_in, pic_recon);
        outFrameCount += numEncoded;
        if (numEncoded && pic_recon)
        {
            cliopt.recon->writePicture(pic_out);
        }

        if (nal)
            cliopt.writeNALs(p_nal, nal);

        // Because x265_encoder_encode() lazily encodes entire GOPs, updates are per-GOP
        cliopt.printStatus(outFrameCount, &param);
    }

    /* Flush the encoder */
    while (!b_ctrl_c)
    {
        int numEncoded = x265_encoder_encode(encoder, &p_nal, &nal, NULL, pic_recon);
        outFrameCount += numEncoded;
        if (numEncoded && pic_recon)
        {
            cliopt.recon->writePicture(pic_out);
        }

        if (nal)
            cliopt.writeNALs(p_nal, nal);

        cliopt.printStatus(outFrameCount, &param);

        if (!numEncoded)
            break;
    }

    /* clear progress report */
    if (cliopt.bProgress)
        fprintf(stderr, "                                                                               \r");

    x265_encoder_get_stats(encoder, &stats);
    x265_encoder_close(encoder, NULL);
    cliopt.bitstreamFile.close();

    if (b_ctrl_c)
        fprintf(stderr, "aborted at input frame %d, output frame %d\n", cliopt.frameSkip + inFrameCount, outFrameCount);

    double elapsed = (double)(x265_mdate() - cliopt.i_start) / 1000000;
    double vidtime = (double)inFrameCount / param.frameRate;
    double bitrate = (0.008f * cliopt.totalBytes) / vidtime;
    printf("\nencoded %d frames in %.2fs (%.2f fps), %.2f kb/s, ",
           outFrameCount, elapsed, outFrameCount / elapsed, bitrate);

    if (param.bEnablePsnr)
        printf("Global PSNR: %.3f\n", stats.globalPsnr);

    if (param.bEnableSsim)
        printf("Global SSIM: %.3f\n", stats.globalSsim);

    x265_cleanup(); /* Free library singletons */

    if (cliopt.csvfp)
    {
        // CLI arguments, date/time, elapsed time, fps, bitrate, global PSNR
        for (int i = 1; i < argc; i++)
        {
            if (i) fputc(' ', cliopt.csvfp);
            fputs(argv[i], cliopt.csvfp);
        }

        time_t now;
        struct tm* timeinfo;
        time(&now);
        timeinfo = localtime(&now);
        char buffer[128];
        strftime(buffer, 128, "%c", timeinfo);
        fprintf(cliopt.csvfp, ", %s, %.2f, %.2f, %.2f, %.2f, %s\n",
            buffer, elapsed, outFrameCount / elapsed, bitrate, stats.globalPsnr, x265_version_str);
    }

    cliopt.destroy();

#if HAVE_VLD
    assert(VLDReportLeaks() == 0);
#endif
#if CU_STAT_LOGFILE
    fclose(fp);
    fclose(fp1);
#endif
    return 0;
}
