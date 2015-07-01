/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Peixuan Zhang <zhangpeixuancn@gmail.com>
 *          Chunli Zhang <chunli@multicorewareinc.com>
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

#include "common.h"
#include "reconplay.h"

#include <signal.h>

using namespace X265_NS;

#if _WIN32
#define popen  _popen
#define pclose _pclose
#define pipemode "wb"
#else
#define pipemode "w"
#endif

bool ReconPlay::pipeValid;

#ifndef _WIN32
static void sigpipe_handler(int)
{
    if (ReconPlay::pipeValid)
        general_log(NULL, "exec", X265_LOG_ERROR, "pipe closed\n");
    ReconPlay::pipeValid = false;
}
#endif

ReconPlay::ReconPlay(const char* commandLine, x265_param& param)
{
#ifndef _WIN32
    if (signal(SIGPIPE, sigpipe_handler) == SIG_ERR)
        general_log(&param, "exec", X265_LOG_ERROR, "Unable to register SIGPIPE handler: %s\n", strerror(errno));
#endif

    width = param.sourceWidth;
    height = param.sourceHeight;
    colorSpace = param.internalCsp;

    frameSize = 0;
    for (int i = 0; i < x265_cli_csps[colorSpace].planes; i++)
        frameSize += (uint32_t)((width >> x265_cli_csps[colorSpace].width[i]) * (height >> x265_cli_csps[colorSpace].height[i]));

    for (int i = 0; i < RECON_BUF_SIZE; i++)
    {
        poc[i] = -1;
        CHECKED_MALLOC(frameData[i], pixel, frameSize);
    }

    outputPipe = popen(commandLine, pipemode);
    if (outputPipe)
    {
        const char* csp = (colorSpace >= X265_CSP_I444) ? "444" : (colorSpace >= X265_CSP_I422) ? "422" : "420";
        const char* depth = (param.internalBitDepth == 10) ? "p10" : "";

        fprintf(outputPipe, "YUV4MPEG2 W%d H%d F%d:%d Ip C%s%s\n", width, height, param.fpsNum, param.fpsDenom, csp, depth);

        pipeValid = true;
        threadActive = true;
        start();
        return;
    }
    else
        general_log(&param, "exec", X265_LOG_ERROR, "popen(%s) failed\n", commandLine);

fail:
    threadActive = false;
}

ReconPlay::~ReconPlay()
{
    if (threadActive)
    {
        threadActive = false;
        writeCount.poke();
        stop();
    }

    if (outputPipe) 
        pclose(outputPipe);

    for (int i = 0; i < RECON_BUF_SIZE; i++)
        X265_FREE(frameData[i]);
}

bool ReconPlay::writePicture(const x265_picture& pic)
{
    if (!threadActive || !pipeValid)
        return false;

    int written = writeCount.get();
    int read = readCount.get();
    int currentCursor = pic.poc % RECON_BUF_SIZE;

    /* TODO: it's probably better to drop recon pictures when the ring buffer is
     * backed up on the display app */
    while (written - read > RECON_BUF_SIZE - 2 || poc[currentCursor] != -1)
    {
        read = readCount.waitForChange(read);
        if (!threadActive)
            return false;
    }

    X265_CHECK(pic.colorSpace == colorSpace, "invalid color space\n");
    X265_CHECK(pic.bitDepth == X265_DEPTH,   "invalid bit depth\n");

    pixel* buf = frameData[currentCursor];
    for (int i = 0; i < x265_cli_csps[colorSpace].planes; i++)
    {
        char* src = (char*)pic.planes[i];
        int pwidth = width >> x265_cli_csps[colorSpace].width[i];

        for (int h = 0; h < height >> x265_cli_csps[colorSpace].height[i]; h++)
        {
            memcpy(buf, src, pwidth * sizeof(pixel));
            src += pic.stride[i];
            buf += pwidth;
        }
    }

    poc[currentCursor] = pic.poc;
    writeCount.incr();

    return true;
}

void ReconPlay::threadMain()
{
    THREAD_NAME("ReconPlayOutput", 0);

    do
    {
        /* extract the next output picture in display order and write to pipe */
        if (!outputFrame())
            break;
    }
    while (threadActive);

    threadActive = false;
    readCount.poke();
}

bool ReconPlay::outputFrame()
{
    int written = writeCount.get();
    int read = readCount.get();
    int currentCursor = read % RECON_BUF_SIZE;

    while (poc[currentCursor] != read)
    {
        written = writeCount.waitForChange(written);
        if (!threadActive)
            return false;
    }

    char* buf = (char*)frameData[currentCursor];
    intptr_t remainSize = frameSize * sizeof(pixel);

    fprintf(outputPipe, "FRAME\n");
    while (remainSize > 0)
    {
        intptr_t retCount = (intptr_t)fwrite(buf, sizeof(char), remainSize, outputPipe);

        if (retCount < 0 || !pipeValid)
            /* pipe failure, stop writing and start dropping recon pictures */
            return false;
    
        buf += retCount;
        remainSize -= retCount;
    }

    poc[currentCursor] = -1;
    readCount.incr();
    return true;
}
