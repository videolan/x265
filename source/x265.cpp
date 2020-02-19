/*****************************************************************************
 * Copyright (C) 2013-2020 MulticoreWare, Inc
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

#include "x265.h"
#include "x265cli.h"

#include "input/input.h"
#include "output/output.h"
#include "output/reconplay.h"
#include "svt.h"

#if HAVE_VLD
/* Visual Leak Detector */
#include <vld.h>
#endif

#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#include <string>
#include <ostream>
#include <fstream>
#include <queue>

using namespace X265_NS;

/* Ctrl-C handler */
static volatile sig_atomic_t b_ctrl_c /* = 0 */;
static void sigint_handler(int)
{
    b_ctrl_c = 1;
}
#define START_CODE 0x00000001
#define START_CODE_BYTES 4

#ifdef _WIN32
/* Copy of x264 code, which allows for Unicode characters in the command line.
 * Retrieve command line arguments as UTF-8. */
static int get_argv_utf8(int *argc_ptr, char ***argv_ptr)
{
    int ret = 0;
    wchar_t **argv_utf16 = CommandLineToArgvW(GetCommandLineW(), argc_ptr);
    if (argv_utf16)
    {
        int argc = *argc_ptr;
        int offset = (argc + 1) * sizeof(char*);
        int size = offset;

        for (int i = 0; i < argc; i++)
            size += WideCharToMultiByte(CP_UTF8, 0, argv_utf16[i], -1, NULL, 0, NULL, NULL);

        char **argv = *argv_ptr = (char**)malloc(size);
        if (argv)
        {
            for (int i = 0; i < argc; i++)
            {
                argv[i] = (char*)argv + offset;
                offset += WideCharToMultiByte(CP_UTF8, 0, argv_utf16[i], -1, argv[i], size - offset, NULL, NULL);
            }
            argv[argc] = NULL;
            ret = 1;
        }
        LocalFree(argv_utf16);
    }
    return ret;
}
#endif

/* Parse the RPU file and extract the RPU corresponding to the current picture 
 * and fill the rpu field of the input picture */
static int rpuParser(x265_picture * pic, FILE * ptr)
{
    uint8_t byteVal;
    uint32_t code = 0;
    int bytesRead = 0;
    pic->rpu.payloadSize = 0;

    if (!pic->pts)
    {
        while (bytesRead++ < 4 && fread(&byteVal, sizeof(uint8_t), 1, ptr))
            code = (code << 8) | byteVal;
      
        if (code != START_CODE)
        {
            x265_log(NULL, X265_LOG_ERROR, "Invalid Dolby Vision RPU startcode in POC %d\n", pic->pts);
            return 1;
        }
    } 

    bytesRead = 0;
    while (fread(&byteVal, sizeof(uint8_t), 1, ptr))
    {
        code = (code << 8) | byteVal;
        if (bytesRead++ < 3)
            continue;
        if (bytesRead >= 1024)
        {
            x265_log(NULL, X265_LOG_ERROR, "Invalid Dolby Vision RPU size in POC %d\n", pic->pts);
            return 1;
        }
        
        if (code != START_CODE)
            pic->rpu.payload[pic->rpu.payloadSize++] = (code >> (3 * 8)) & 0xFF;
        else
            return 0;       
    }

    int ShiftBytes = START_CODE_BYTES - (bytesRead - pic->rpu.payloadSize);
    int bytesLeft = bytesRead - pic->rpu.payloadSize;
    code = (code << ShiftBytes * 8);
    for (int i = 0; i < bytesLeft; i++)
    {
        pic->rpu.payload[pic->rpu.payloadSize++] = (code >> (3 * 8)) & 0xFF;
        code = (code << 8);
    }
    if (!pic->rpu.payloadSize)
        x265_log(NULL, X265_LOG_WARNING, "Dolby Vision RPU not found for POC %d\n", pic->pts);
    return 0;
}


/* CLI return codes:
 *
 * 0 - encode successful
 * 1 - unable to parse command line
 * 2 - unable to open encoder
 * 3 - unable to generate stream headers
 * 4 - encoder abort */

int main(int argc, char **argv)
{
#if HAVE_VLD
    // This uses Microsoft's proprietary WCHAR type, but this only builds on Windows to start with
    VLDSetReportOptions(VLD_OPT_REPORT_TO_DEBUGGER | VLD_OPT_REPORT_TO_FILE, L"x265_leaks.txt");
#endif
    PROFILE_INIT();
    THREAD_NAME("API", 0);

    GetConsoleTitle(orgConsoleTitle, CONSOLE_TITLE_SIZE);
    SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED);
#if _WIN32
    char** orgArgv = argv;
    get_argv_utf8(&argc, &argv);
#endif

    ReconPlay* reconPlay = NULL;
    CLIOptions cliopt;

    if (cliopt.parse(argc, argv))
    {
        cliopt.destroy();
        if (cliopt.api)
            cliopt.api->param_free(cliopt.param);
        exit(1);
    }

    x265_param* param = cliopt.param;
    const x265_api* api = cliopt.api;
#if ENABLE_LIBVMAF
    x265_vmaf_data* vmafdata = cliopt.vmafData;
#endif
    /* This allows muxers to modify bitstream format */
    cliopt.output->setParam(param);

    if (cliopt.reconPlayCmd)
        reconPlay = new ReconPlay(cliopt.reconPlayCmd, *param);

    if (cliopt.zoneFile)
    {
        if (!cliopt.parseZoneFile())
        {
            x265_log(NULL, X265_LOG_ERROR, "Unable to parse zonefile\n");
            fclose(cliopt.zoneFile);
            cliopt.zoneFile = NULL;
        }
    }

    /* note: we could try to acquire a different libx265 API here based on
     * the profile found during option parsing, but it must be done before
     * opening an encoder */

    x265_encoder *encoder = api->encoder_open(param);
    if (!encoder)
    {
        x265_log(param, X265_LOG_ERROR, "failed to open encoder\n");
        cliopt.destroy();
        api->param_free(param);
        api->cleanup();
        exit(2);
    }

    /* get the encoder parameters post-initialization */
    api->encoder_parameters(encoder, param);

     /* Control-C handler */
    if (signal(SIGINT, sigint_handler) == SIG_ERR)
        x265_log(param, X265_LOG_ERROR, "Unable to register CTRL+C handler: %s\n", strerror(errno));

    x265_picture pic_orig, pic_out;
    x265_picture *pic_in = &pic_orig;
    /* Allocate recon picture if analysis save/load is enabled */
    std::priority_queue<int64_t>* pts_queue = cliopt.output->needPTS() ? new std::priority_queue<int64_t>() : NULL;
    x265_picture *pic_recon = (cliopt.recon || param->analysisSave || param->analysisLoad || pts_queue || reconPlay || param->csvLogLevel) ? &pic_out : NULL;
    uint32_t inFrameCount = 0;
    uint32_t outFrameCount = 0;
    x265_nal *p_nal;
    x265_stats stats;
    uint32_t nal;
    int16_t *errorBuf = NULL;
    bool bDolbyVisionRPU = false;
    uint8_t *rpuPayload = NULL;
    int ret = 0;
    int inputPicNum = 1;
    x265_picture picField1, picField2;

    if (!param->bRepeatHeaders && !param->bEnableSvtHevc)
    {
        if (api->encoder_headers(encoder, &p_nal, &nal) < 0)
        {
            x265_log(param, X265_LOG_ERROR, "Failure generating stream headers\n");
            ret = 3;
            goto fail;
        }
        else
            cliopt.totalbytes += cliopt.output->writeHeaders(p_nal, nal);
    }

    if (param->bField && param->interlaceMode)
    {
        api->picture_init(param, &picField1);
        api->picture_init(param, &picField2);
        // return back the original height of input
        param->sourceHeight *= 2;
        api->picture_init(param, pic_in);
    }
    else
        api->picture_init(param, pic_in);

    if (param->dolbyProfile && cliopt.dolbyVisionRpu)
    {
        rpuPayload = X265_MALLOC(uint8_t, 1024);
        pic_in->rpu.payload = rpuPayload;
        if (pic_in->rpu.payload)
            bDolbyVisionRPU = true;
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
        pic_orig.poc = (param->bField && param->interlaceMode) ? inFrameCount * 2 : inFrameCount;
        if (cliopt.qpfile)
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
            if (pic_in->bitDepth > param->internalBitDepth && cliopt.bDither)
            {
                x265_dither_image(pic_in, cliopt.input->getWidth(), cliopt.input->getHeight(), errorBuf, param->internalBitDepth);
                pic_in->bitDepth = param->internalBitDepth;
            }
            /* Overwrite PTS */
            pic_in->pts = pic_in->poc;

            // convert to field
            if (param->bField && param->interlaceMode)
            {
                int height = pic_in->height >> 1;
                
                int static bCreated = 0;
                if (bCreated == 0)
                {
                    bCreated = 1;
                    inputPicNum = 2;
                    picField1.fieldNum = 1;
                    picField2.fieldNum = 2;

                    picField1.bitDepth = picField2.bitDepth = pic_in->bitDepth;
                    picField1.colorSpace = picField2.colorSpace = pic_in->colorSpace;
                    picField1.height = picField2.height = pic_in->height >> 1;
                    picField1.framesize = picField2.framesize = pic_in->framesize >> 1;

                    size_t fieldFrameSize = (size_t)pic_in->framesize >> 1;
                    char* field1Buf = X265_MALLOC(char, fieldFrameSize);
                    char* field2Buf = X265_MALLOC(char, fieldFrameSize);
  
                    int stride = picField1.stride[0] = picField2.stride[0] = pic_in->stride[0];
                    uint64_t framesize = stride * (height >> x265_cli_csps[pic_in->colorSpace].height[0]);
                    picField1.planes[0] = field1Buf;
                    picField2.planes[0] = field2Buf;
                    for (int i = 1; i < x265_cli_csps[pic_in->colorSpace].planes; i++)
                    {
                        picField1.planes[i] = field1Buf + framesize;
                        picField2.planes[i] = field2Buf + framesize;

                        stride = picField1.stride[i] = picField2.stride[i] = pic_in->stride[i];
                        framesize += (stride * (height >> x265_cli_csps[pic_in->colorSpace].height[i]));
                    }
                    assert(framesize  == picField1.framesize);
                }

                picField1.pts = picField1.poc = pic_in->poc;
                picField2.pts = picField2.poc = pic_in->poc + 1;

                picField1.userSEI = picField2.userSEI = pic_in->userSEI;

                //if (pic_in->userData)
                //{
                //    // Have to handle userData here
                //}

                if (pic_in->framesize)
                {
                    for (int i = 0; i < x265_cli_csps[pic_in->colorSpace].planes; i++)
                    {
                        char* srcP1 = (char*)pic_in->planes[i];
                        char* srcP2 = (char*)pic_in->planes[i] + pic_in->stride[i];
                        char* p1 = (char*)picField1.planes[i];
                        char* p2 = (char*)picField2.planes[i];

                        int stride = picField1.stride[i];

                        for (int y = 0; y < (height >> x265_cli_csps[pic_in->colorSpace].height[i]); y++)
                        {
                            memcpy(p1, srcP1, stride);
                            memcpy(p2, srcP2, stride);
                            srcP1 += 2*stride;
                            srcP2 += 2*stride;
                            p1 += stride;
                            p2 += stride;
                        }
                    }
                }
            }

            if (bDolbyVisionRPU)
            {
                if (param->bField && param->interlaceMode)
                {
                    if (rpuParser(&picField1, cliopt.dolbyVisionRpu) > 0)
                        goto fail;
                    if (rpuParser(&picField2, cliopt.dolbyVisionRpu) > 0)
                        goto fail;
                }
                else
                {
                    if (rpuParser(pic_in, cliopt.dolbyVisionRpu) > 0)
                        goto fail;
                }
            }
        }
                
        for (int inputNum = 0; inputNum < inputPicNum; inputNum++)
        {  
            x265_picture *picInput = NULL;
            if (inputPicNum == 2)
                picInput = pic_in ? (inputNum ? &picField2 : &picField1) : NULL;
            else
                picInput = pic_in;

            int numEncoded = api->encoder_encode( encoder, &p_nal, &nal, picInput, pic_recon );
            if( numEncoded < 0 )
            {
                b_ctrl_c = 1;
                ret = 4;
                break;
            }

            if (reconPlay && numEncoded)
                reconPlay->writePicture(*pic_recon);

            outFrameCount += numEncoded;

            if (numEncoded && pic_recon && cliopt.recon)
                cliopt.recon->writePicture(pic_out);
            if (nal)
            {
                cliopt.totalbytes += cliopt.output->writeFrame(p_nal, nal, pic_out);
                if (pts_queue)
                {
                    pts_queue->push(-pic_out.pts);
                    if (pts_queue->size() > 2)
                        pts_queue->pop();
                }
            }
            cliopt.printStatus( outFrameCount );
        }
    }

    /* Flush the encoder */
    while (!b_ctrl_c)
    {
        int numEncoded = api->encoder_encode(encoder, &p_nal, &nal, NULL, pic_recon);
        if (numEncoded < 0)
        {
            ret = 4;
            break;
        }

        if (reconPlay && numEncoded)
            reconPlay->writePicture(*pic_recon);

        outFrameCount += numEncoded;
        if (numEncoded && pic_recon && cliopt.recon)
            cliopt.recon->writePicture(pic_out);
        if (nal)
        {
            cliopt.totalbytes += cliopt.output->writeFrame(p_nal, nal, pic_out);
            if (pts_queue)
            {
                pts_queue->push(-pic_out.pts);
                if (pts_queue->size() > 2)
                    pts_queue->pop();
            }
        }

        cliopt.printStatus(outFrameCount);

        if (!numEncoded)
            break;
    }
  
    if (bDolbyVisionRPU)
    {
        if(fgetc(cliopt.dolbyVisionRpu) != EOF)
            x265_log(NULL, X265_LOG_WARNING, "Dolby Vision RPU count is greater than frame count\n");
        x265_log(NULL, X265_LOG_INFO, "VES muxing with Dolby Vision RPU file successful\n");
    }

    /* clear progress report */
    if (cliopt.bProgress)
        fprintf(stderr, "%*s\r", 80, " ");

fail:

    delete reconPlay;

    api->encoder_get_stats(encoder, &stats, sizeof(stats));
    if (param->csvfn && !b_ctrl_c)
#if ENABLE_LIBVMAF
        api->vmaf_encoder_log(encoder, argc, argv, param, vmafdata);
#else
        api->encoder_log(encoder, argc, argv);
#endif
    api->encoder_close(encoder);

    int64_t second_largest_pts = 0;
    int64_t largest_pts = 0;
    if (pts_queue && pts_queue->size() >= 2)
    {
        second_largest_pts = -pts_queue->top();
        pts_queue->pop();
        largest_pts = -pts_queue->top();
        pts_queue->pop();
        delete pts_queue;
        pts_queue = NULL;
    }
    cliopt.output->closeFile(largest_pts, second_largest_pts);

    if (b_ctrl_c)
        general_log(param, NULL, X265_LOG_INFO, "aborted at input frame %d, output frame %d\n",
                    cliopt.seek + inFrameCount, stats.encodedPictureCount);

    api->cleanup(); /* Free library singletons */

    cliopt.destroy();

    api->param_free(param);

    X265_FREE(errorBuf);
    X265_FREE(rpuPayload);

    SetConsoleTitle(orgConsoleTitle);
    SetThreadExecutionState(ES_CONTINUOUS);

#if _WIN32
    if (argv != orgArgv)
    {
        free(argv);
        argv = orgArgv;
    }
#endif

#if HAVE_VLD
    assert(VLDReportLeaks() == 0);
#endif

    return ret;
}
