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
#include "PPA/ppa.h"
#include "x265.h"

#if HAVE_VLD
/* Visual Leak Detector */
#include <vld.h>
#endif

#include <signal.h>
#include <getopt.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <string>
#include <time.h>
#include <list>
#include <ostream>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#else
#define GetConsoleTitle(t,n)
#define SetConsoleTitle(t)
#endif

using namespace x265;
using namespace std;

static const char short_options[] = "h:o:f:r:i:s:d:q:w:V:";
static struct option long_options[] =
{
#define HELP(message)
#define OPT(longname, var, argreq, flag, helptext) { longname, argreq, NULL, flag },
#define STROPT OPT
#include "x265opts.h"
#undef OPT
#undef STROPT
#undef HELP
};

/* Ctrl-C handler */
static volatile int b_ctrl_c = 0;
static int          b_exit_on_ctrl_c = 0;
static void sigint_handler( int a )
{
    if( b_exit_on_ctrl_c )
        exit(0);
    b_ctrl_c = 1;
}

struct CLIOptions
{
    x265::Input*  input;
    x265::Output* recon;
    fstream bitstreamFile;
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
        framesToBeEncoded = frameSkip = 0;
        essentialBytes = 0;
        totalBytes = 0;
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
        PPAStartCpuEventFunc(bitstream_write);
        for (int i = 0; i < nalcount; i++)
        {
            bitstreamFile.write((const char*) nal->p_payload, nal->i_payload);
            rateStatsAccum((NalUnitType) nal->i_type, nal->i_payload);
            nal++;
        }
        PPAStopCpuEventFunc(bitstream_write);
    }
    
    /* in microseconds */
    static const int UPDATE_INTERVAL = 250000;

    void printStatus(int i_frame, x265_param_t *param)
    {
        char buf[200];
        int64_t i_time = x265_mdate();
        if( i_previous && i_time - i_previous < UPDATE_INTERVAL )
            return;
        int64_t i_elapsed = i_time - i_start;
        double fps = i_elapsed > 0 ? i_frame * 1000000. / i_elapsed : 0;
        double bitrate = (double) totalBytes * 8 / ( (double) 1000 * param->iFrameRate );
        if( framesToBeEncoded )
        {
            int eta = i_elapsed * (framesToBeEncoded - i_frame) / ((int64_t)i_frame * 1000000);
            sprintf( buf, "x265 [%.1f%%] %d/%d frames, %.2f fps, %.2f kb/s, eta %d:%02d:%02d",
                100. * i_frame / framesToBeEncoded, i_frame, framesToBeEncoded, fps, bitrate,
                eta/3600, (eta/60)%60, eta%60 );
        }
        else
        {
            sprintf( buf, "x265 %d frames: %.2f fps, %.2f kb/s", i_frame, fps, bitrate );
        }
        fprintf( stderr, "%s  \r", buf+5 );
        SetConsoleTitle( buf );
        fflush( stderr ); // needed in windows
        i_previous = i_time;
    }

    void log(int i_level, const char *fmt, ...)
    {
        if( i_level > cli_log_level )
            return;
        std::string s_level;
        switch( i_level )
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
        fprintf( stderr, "x265 [%s]: ", s_level.c_str() );
        va_list arg;
        va_start( arg, fmt );
        vfprintf( stderr, fmt, arg );
        va_end( arg );
    }

    void print_version()
    {
#define XSTR(x) STR(x)
#define STR(x) #x
        printf("x265: HEVC encoder version %s\n", XSTR(X265_VERSION));
        printf("x265: build info ");
        printf(NVM_ONOS);
        printf(NVM_COMPILEDBY);
        printf(NVM_BITS);
#if HIGH_BIT_DEPTH
        printf("16bpp");
#else
        printf("8bpp");
#endif
        printf("\n");
    }

    void do_help()
    {
        print_version();
        printf("Syntax: x265 [options] infile [-o] outfile\n");
        printf("    infile can be YUV or Y4M\n");
        printf("    outfile is raw HEVC stream only\n");
        printf("Options:\n");

#define HELP(message) printf("\n%s\n", message);
#define OPT(longname, var, argreq, flag, helptext)\
        if (flag) printf("-%c/", flag); else printf("   ");\
        printf("--%-20s\t%s\n", longname, helptext);
#define STROPT OPT
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
        const char *inputfn = NULL, *reconfn = NULL, *bitstreamfn = NULL;

        x265_param_default(param);

        for( optind = 0;; )
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
                do_help();
            case 'V':
                print_version();
                exit(0);
            default:
                if (long_options_index < 0 && c > 0)
                {
                    for (size_t i = 0; i < sizeof(long_options)/sizeof(long_options[0]); i++)
                        if (long_options[i].val == c)
                        {
                            long_options_index = (int)i;
                            break;
                        }
                    if (long_options_index < 0)
                    {
                        /* getopt_long already printed an error message */
                        return true;
                    }
                }
                if (long_options_index < 0)
                {
                    log(X265_LOG_WARNING, "short option '%x' unrecognized\n", c);
                    return true;
                }
#define HELP(message)
#define STROPT(longname, var, argreq, flag, helptext)\
                else if (!strcmp(long_options[long_options_index].name, longname))\
                    (var) = optarg;
#define OPT(longname, var, argreq, flag, helptext)\
                else if (!strcmp(long_options[long_options_index].name, longname))\
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
            do_help();

        x265_setup_primitives(param, cpuid);

        if (inputfn == NULL || bitstreamfn == NULL)
        {
            log(X265_LOG_ERROR, "input or output file not specified, try -V for help\n");
            return true;
        }
        this->input = x265::Input::Open(inputfn);
        if (!this->input || this->input->isFail())
        {
            log(X265_LOG_ERROR, "unable to open input file <%s>\n", inputfn);
            return true;
        }
        if (this->input->getWidth())
        {
            /* parse the width, height, frame rate from the y4m file */
            param->iSourceWidth = this->input->getWidth();
            param->iSourceHeight = this->input->getHeight();
            param->iFrameRate = (int)this->input->getRate();
            inputBitDepth = 8;
        }
        else
        {
            this->input->setDimensions(param->iSourceWidth, param->iSourceHeight);
            this->input->setBitDepth(inputBitDepth);
        }
        assert(param->iSourceHeight && param->iSourceWidth);

        /* rules for input, output and internal bitdepths as per help text */
        if (!param->internalBitDepth) { param->internalBitDepth = inputBitDepth; }
        if (!outputBitDepth) { outputBitDepth = param->internalBitDepth; }

        uint32_t numRemainingFrames = (uint32_t)this->input->guessFrameCount();

        if (this->frameSkip)
        {
            this->input->skipFrames(this->frameSkip);
        }

        this->framesToBeEncoded = this->framesToBeEncoded ? min(this->framesToBeEncoded, numRemainingFrames) : numRemainingFrames;

        log(X265_LOG_INFO, "Input File                   : %s (%u - %d of %d total frames)\n", inputfn,
            this->frameSkip, this->frameSkip + this->framesToBeEncoded - 1, numRemainingFrames);

        if (reconfn)
        {
            this->recon = x265::Output::Open(reconfn, param->iSourceWidth, param->iSourceHeight, outputBitDepth, param->iFrameRate);
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

        this->bitstreamFile.open(bitstreamfn, fstream::binary | fstream::out);
        if (!this->bitstreamFile)
        {
            log(X265_LOG_ERROR, "failed to open bitstream file <%s> for writing\n", bitstreamfn);
            return true;
        }

        x265_set_globals(param, inputBitDepth);

        return false;
    }

};

int main(int argc, char **argv)
{
#if HAVE_VLD
    VLDSetReportOptions(VLD_OPT_REPORT_TO_DEBUGGER, NULL);
#endif
    PPA_INIT();

    x265_param_t param;
    CLIOptions   cliopt;

    // TODO: needs proper logging file handle with log levels, etc
    if (cliopt.parse(argc, argv, &param))
        exit(1);

    if (x265_check_params(&param))
        exit(1);

    x265_print_params(&param);

    x265_t *encoder = x265_encoder_open(&param);
    if (!encoder)
    {
        cliopt.log(X265_LOG_ERROR, "failed to open encoder\n");
        exit(1);
    }

    /* Control-C handler */
    signal( SIGINT, sigint_handler );

    x265_picture_t pic_orig, pic_recon;
    x265_picture_t *pic_in = &pic_orig;
    x265_picture_t *pic_out = cliopt.recon ? &pic_recon : 0;
    x265_nal_t *p_nal;
    int nal;

    // main encoder loop
    uint32_t frameCount = 0;
    uint32_t outFrameCount = 0;
    while (pic_in && !b_ctrl_c)
    {
        // read input YUV file
        if (frameCount < cliopt.framesToBeEncoded && cliopt.input->readPicture(pic_orig))
            frameCount++;
        else
            pic_in = NULL;

        int iNumEncoded = x265_encoder_encode(encoder, &p_nal, &nal, pic_in, pic_out);
        if (iNumEncoded && pic_out)
        {
            cliopt.recon->writePicture(pic_recon);
            outFrameCount++;
        }
        if (nal)
            cliopt.writeNALs(p_nal, nal);
        // Because x265_encoder_encode() lazily encodes entire GOPs, updates are per-GOP
        cliopt.printStatus(frameCount, &param);
    }

    /* Flush the encoder */
    while (!b_ctrl_c && x265_encoder_encode(encoder, &p_nal, &nal, NULL, pic_out))
    {
        if (pic_out)
        {
            cliopt.recon->writePicture(pic_recon);
            outFrameCount++;
        }
        if (nal)
            cliopt.writeNALs(p_nal, nal);
        cliopt.printStatus(frameCount, &param);
    }

    /* clear progress report */
    fprintf(stderr, "                                                                               \r");
    fprintf(stderr, "\n");

    x265_encoder_close(encoder);
    cliopt.bitstreamFile.close();

    if (b_ctrl_c)
        fprintf(stderr, "aborted at input frame %d, output frame %d\n", cliopt.frameSkip + frameCount, outFrameCount);

    double elapsed = (double)(x265_mdate() - cliopt.i_start) / 1000000;
    double vidtime = (double)frameCount / param.iFrameRate;
    printf("Bytes written to file: %u (%.3f kbps) in %3.3f sec\n",
        cliopt.totalBytes, 0.008 * cliopt.totalBytes / vidtime, elapsed);

    x265_cleanup(); /* Free library singletons */

    cliopt.destroy();

#if HAVE_VLD
    assert(VLDReportLeaks() == 0);
#endif
    return 0;
}
