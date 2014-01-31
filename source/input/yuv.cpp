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

#include "yuv.h"
#include "PPA/ppa.h"
#include "common.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <iostream>

#define ENABLE_THREADING 1

#if _WIN32
#include <io.h>
#include <fcntl.h>
#if defined(_MSC_VER)
#pragma warning(disable: 4996) // POSIX setmode and fileno deprecated
#endif
#endif

using namespace x265;
using namespace std;

YUVInput::YUVInput(const char *filename, uint32_t inputBitDepth)
{
    for (int i = 0; i < QUEUE_SIZE; i++)
    {
        buf[i] = NULL;
    }

    head = 0;
    tail = 0;
    depth = inputBitDepth;
    pixelbytes = inputBitDepth > 8 ? 2 : 1;
    width = height = framesize = 0;
    threadActive = false;
    if (!strcmp(filename, "-"))
    {
        ifs = &cin;
#if _WIN32
        setmode(fileno(stdin), O_BINARY);
#endif
    }
    else
        ifs = new ifstream(filename, ios::binary | ios::in);

    if (ifs && ifs->good())
        threadActive = true;
    else if (ifs && ifs != &cin)
    {
        delete ifs;
        ifs = NULL;
    }
}

YUVInput::~YUVInput()
{
    if (ifs && ifs != &cin)
        delete ifs;
    for (int i = 0; i < QUEUE_SIZE; i++)
    {
        delete[] buf[i];
    }
}

void YUVInput::release()
{
    threadActive = false;
    notFull.trigger();
    stop();
    delete this;
}

void YUVInput::init()
{
    if (!framesize)
    {
        for (int i = 0; i < x265_cli_csps[colorSpace].planes; i++)
        {
            uint32_t w = width >> x265_cli_csps[colorSpace].width[i];
            uint32_t h = height >> x265_cli_csps[colorSpace].height[i];
            framesize += w * h * pixelbytes;
        }

        for (uint32_t i = 0; i < QUEUE_SIZE; i++)
        {
            buf[i] = new char[framesize];
            if (buf[i] == NULL)
            {
                x265_log(NULL, X265_LOG_ERROR, "yuv: buffer allocation failure, aborting\n");
                threadActive = false;
            }
        }
    }
}

int YUVInput::guessFrameCount()
{
    init();
    if (!ifs || ifs == &cin) return -1;

    ifstream::pos_type cur = ifs->tellg();
    if (cur < 0)
        return -1;

    ifs->seekg(0, ios::end);
    ifstream::pos_type size = ifs->tellg();
    ifs->seekg(cur, ios::beg);
    if (size < 0)
        return -1;

    assert(framesize);
    return (int)((size - cur) / framesize);
}

void YUVInput::skipFrames(uint32_t numFrames)
{
    init();
    if (ifs)
    {
        for (uint32_t i = 0; i < numFrames; i++)
        {
            ifs->ignore(framesize);
        }
    }
}

void YUVInput::startReader()
{
    init();
#if ENABLE_THREADING
    if (ifs && threadActive)
        start();
#endif
}

void YUVInput::threadMain()
{
    while (threadActive)
    {
        if (!populateFrameQueue())
            break;
    }

    threadActive = false;
    notEmpty.trigger();
}

bool YUVInput::populateFrameQueue()
{
    while ((tail + 1) % QUEUE_SIZE == head)
    {
        notFull.wait();
        if (!threadActive)
            return false;
    }

    PPAStartCpuEventFunc(read_yuv);
    ifs->read(buf[tail], framesize);
    frameStat[tail] = !ifs->fail();
    tail = (tail + 1) % QUEUE_SIZE;
    notEmpty.trigger();
    PPAStopCpuEventFunc(read_yuv);

    return !ifs->fail();
}

bool YUVInput::readPicture(x265_picture& pic)
{
#if ENABLE_THREADING
    while (head == tail)
    {
        notEmpty.wait();
        if (!threadActive)
            return false;
    }

#else
    populateFrameQueue();
#endif

    if (!frameStat[head])
        return false;

    pic.bitDepth = depth;
    pic.planes[0] = buf[head];
    pic.planes[1] = (char*)(pic.planes[0]) + width * height * pixelbytes;
    pic.planes[2] = (char*)(pic.planes[1]) + ((width * height * pixelbytes) >> 2);
    pic.stride[0] = width;
    pic.stride[1] = pic.stride[2] = pic.stride[0] >> 1;

    head = (head + 1) % QUEUE_SIZE;
    notFull.trigger();

    return true;
}
