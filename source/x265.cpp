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
#define XSTR(x) STR(x)
#define STR(x) #x
        fprintf(stderr, "x265 [info]: HEVC encoder version %s\n", XSTR(X265_VERSION));
        fprintf(stderr, "x265 [info]: build info ");
        fprintf(stderr, NVM_ONOS);
        fprintf(stderr, NVM_COMPILEDBY);
        fprintf(stderr, NVM_BITS);
#if HIGH_BIT_DEPTH
        fprintf(stderr, "16bpp");
#else
        fprintf(stderr, "8bpp");
#endif
        fprintf(stderr, "\n");
        x265_setup_primitives(param, 0);
    }

    void do_help(x265_param_t *param)
    {
        x265_param_default(param);
        print_version(param);
        int cpuid = 0, inputBitDepth = 8, outputBitDepth = param->internalBitDepth;
        char help = 'h';

        printf("\nSyntax: x265 [options] infile [-o] outfile\n");
        printf("    infile can be YUV or Y4M\n");
        printf("    outfile is raw HEVC bitstream\n");

#define HELP(message) printf("\n%s\n", message);
#define OPT(longname, var, argreq, flag, helptext) \
    if (flag) printf("-%c/", flag); else printf("   "); \
    printf("--%-20s\t%s\n", longname, helptext); \
    if (argreq == required_argument) printf("\t\t\t\tDefault: %d\n", var);  \
    else if (strncmp(longname, "no-", 3)) printf("\t\t\t\tDefault: %s\n", var ? "Enabled" : "Disabled");
#define STROPT(longname, var, argreq, flag, helptext) \
    if (flag) printf("-%c/", flag); else printf("   "); \
    printf("--%-20s\t%s\n", longname, helptext); 
#include "x265opts.h"
#undef OPT
#undef STROPT
#undef HELP

        exit(0);
    }

    bool parse(int argc, char **argv, x265_param_t* param)
    {
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
#define HELP(message)
#define STROPT(longname, var, argreq, flag, helptext) \
    else if (!strcmp(long_options[long_options_index].name, longname)) \
        (var) = optarg;
#define OPT(longname, var, argreq, flag, helptext) \
    else if (!strcmp(long_options[long_options_index].name, longname)) \
        (var) = (argreq == no_argument) ? (strncmp(longname, "no-", 3) ? 1 : 0) : atoi(optarg);
#include "x265opts.h"
#undef OPT
#undef STROPT
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
            buffer, elapsed, outFrameCount / elapsed, bitrate, stats.globalPsnr, XSTR(X265_VERSION));
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
