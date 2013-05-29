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
#include <assert.h>
#include <string.h>
#include <time.h>
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
    uint32_t outputBitDepth;            ///< bit-depth of output reconstructed images file
    uint32_t frameSkip;                 ///< number of frames to skip from the beginning
    uint32_t framesToBeEncoded;         ///< number of frames to encode

    uint32_t essentialBytes;            ///< total essential NAL bytes written to bitstream
    uint32_t totalBytes;                ///< total bytes written to bitstream

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
};

static void print_version()
{
#define XSTR(x) STR(x)
#define STR(x) #x
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
}

static void do_help()
{
    print_version();
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
#define HELP(message)

    exit(0);
}

static const char short_options[] = "h:o:f:r:i:s:d:q:w:V:";
static struct option long_options[] =
{
#define OPT(longname, var, argreq, flag, helptext) { longname, argreq, NULL, flag },
#define STROPT OPT
#include "x265opts.h"
#undef OPT
#undef STROPT
};

bool parse(int argc, char **argv, x265_param_t* param, CLIOptions* cliopt)
{
    int help = 0;
    int cpuid = 0;
    int threadcount = 0;
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
            }
            if (long_options_index < 0)
                printf("x265: short option '%x' unrecognized\n", c);
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

    if (argc <= 1 || help)
        do_help();

    x265::SetupPrimitives(cpuid);
    cliopt->threadPool = x265::ThreadPool::AllocThreadPool(threadcount);

    if (optind < argc)
        inputfn = argv[optind];
    if (inputfn == NULL)
    {
        printf("x265: no input file specified, try -V for help\n");
        return true;
    }
    cliopt->input = x265::Input::Open(inputfn);
    if (!cliopt->input || cliopt->input->isFail())
    {
        printf("x265: unable to open input file <%s>\n", inputfn);
        return true;
    }
    if (cliopt->input->getWidth())
    {
        /* parse the width, height, frame rate from the y4m file */
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
    assert(param->iSourceHeight && param->iSourceWidth);

    /* rules for input, output and internal bitdepths as per help text */
    if (!param->internalBitDepth) { param->internalBitDepth = cliopt->inputBitDepth; }
    if (!cliopt->outputBitDepth) { cliopt->outputBitDepth = param->internalBitDepth; }

    uint32_t numRemainingFrames = (uint32_t)cliopt->input->guessFrameCount();

    if (cliopt->frameSkip)
    {
        cliopt->input->skipFrames(cliopt->frameSkip);
    }

    cliopt->framesToBeEncoded = cliopt->framesToBeEncoded ? min(cliopt->framesToBeEncoded, numRemainingFrames) : numRemainingFrames;

    printf("x265: Input File                   : %s (%u - %d of %d total frames)\n", inputfn,
        cliopt->frameSkip, cliopt->frameSkip + cliopt->framesToBeEncoded - 1, numRemainingFrames);

    if (reconfn)
    {
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

    // TODO: needs proper logging file handle with log levels, etc
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

    clock_t start = clock();

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
        if (nal)
            cliopt.writeNALs(p_nal, nal);
    }

    /* Flush the encoder */
    while (x265_encoder_encode(encoder, &p_nal, &nal, NULL, pic_out))
    {
        if (pic_out)
            cliopt.recon->writePicture(pic_recon);
        if (nal)
            cliopt.writeNALs(p_nal, nal);
    }

    x265_encoder_close(encoder);
    cliopt.bitstreamFile.close();

    double elapsed = (double)(clock() - start) / CLOCKS_PER_SEC;
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
