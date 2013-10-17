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

using namespace x265;
using namespace std;

YUVInput::YUVInput(const char *filename)
{
    ifs.open(filename, ios::binary | ios::in);
    width = height = 0;
    depth = 8;
    threadActive = false;
    if (!ifs.fail())
        threadActive = true;
    else
        ifs.close();
#if defined ENABLE_THREAD
    head = 0;
    tail = 0;
#endif
}

YUVInput::~YUVInput()
{
    ifs.close();
#if defined ENABLE_THREAD
    for (int i = 0; i < QUEUE_SIZE; i++)
    {
        delete[] buf[i];
    }

#else
    delete[] buf;
#endif
}

int YUVInput::guessFrameCount()
{
    ifstream::pos_type cur = ifs.tellg();

    ifs.seekg(0, ios::end);
    ifstream::pos_type size = ifs.tellg();
    ifs.seekg(cur, ios::beg);

    return (int)((size - cur) / (width * height * pixelbytes * 3 / 2));
}

void YUVInput::skipFrames(int numFrames)
{
    ifs.seekg(framesize * numFrames, ios::cur);
}

void YUVInput::setDimensions(int w, int h)
{
    width = w;
    height = h;
    pixelbytes = depth > 8 ? 2 : 1;
    framesize = (width * height * 3 / 2) * pixelbytes;
    if (width < MIN_FRAME_WIDTH || width > MAX_FRAME_WIDTH ||
        height < MIN_FRAME_HEIGHT || height > MAX_FRAME_HEIGHT)
    {
        threadActive = false;
        ifs.close();
    }
    else
    {
#if defined ENABLE_THREAD
        for (int i = 0; i < QUEUE_SIZE; i++)
        {
            buf[i] = new char[framesize];
            if (buf[i] == NULL)
            {
                x265_log(NULL, X265_LOG_ERROR, "yuv: buffer allocation failure, aborting");
                threadActive = false;
            }
        }

        start();
#else // if defined ENABLE_THREAD
        buf = new char[framesize];
#endif // if defined ENABLE_THREAD
    }
}

#if defined ENABLE_THREAD
void YUVInput::threadMain()
{
    do
    {
        if (!populateFrameQueue())
            break;
    }
    while (threadActive);
}

bool YUVInput::populateFrameQueue()
{
    while ((tail + 1) % QUEUE_SIZE == head)
    {
        notFull.wait();
        if (!threadActive)
            break;
    }

    ifs.read(buf[tail], framesize);
    frameStat[tail] = ifs.good();
    if (!frameStat[tail])
        return false;
    tail = (tail + 1) % QUEUE_SIZE;
    notEmpty.trigger();
    return true;
}

bool YUVInput::readPicture(x265_picture_t& pic)
{
    PPAStartCpuEventFunc(read_yuv);
    if (!threadActive)
        return false;
    while (head == tail)
    {
        notEmpty.wait();
    }

    if (!frameStat[head])
        return false;
    pic.planes[0] = buf[head];

    pic.planes[1] = (char*)(pic.planes[0]) + width * height * pixelbytes;

    pic.planes[2] = (char*)(pic.planes[1]) + ((width * height * pixelbytes) >> 2);

    pic.bitDepth = depth;

    pic.stride[0] = width * pixelbytes;

    pic.stride[1] = pic.stride[2] = pic.stride[0] >> 1;

    head = (head + 1) % QUEUE_SIZE;
    notFull.trigger();

    PPAStopCpuEventFunc(read_yuv);

    return true;
}

#else // if defined ENABLE_THREAD

// TODO: only supports 4:2:0 chroma sampling
bool YUVInput::readPicture(x265_picture_t& pic)
{
    PPAStartCpuEventFunc(read_yuv);

    pic.planes[0] = buf;

    pic.planes[1] = (char*)(pic.planes[0]) + width * height * pixelbytes;

    pic.planes[2] = (char*)(pic.planes[1]) + ((width * height * pixelbytes) >> 2);

    pic.bitDepth = depth;

    pic.stride[0] = width * pixelbytes;

    pic.stride[1] = pic.stride[2] = pic.stride[0] >> 1;

    ifs.read(buf, framesize);
    PPAStopCpuEventFunc(read_yuv);

    return ifs.good();
}

#endif // if defined ENABLE_THREAD

void YUVInput::release()
{
#if defined(ENABLE_THREAD)
    threadActive = false;
    notFull.trigger();
    stop();
#endif
    delete this;
}
