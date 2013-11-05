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

#include "y4m.h"
#include "PPA/ppa.h"
#include "common.h"
#include <stdio.h>
#include <string.h>
#include <iostream>

#if _WIN32
#include "io.h"
#include "fcntl.h"
#if defined(_MSC_VER)
#pragma warning(disable: 4996) // POSIX setmode and fileno deprecated
#endif
#endif

using namespace x265;
using namespace std;

Y4MInput::Y4MInput(const char *filename)
{

#if defined ENABLE_THREAD
    for (uint32_t i = 0; i < QUEUE_SIZE; i++)
        buf[i] = NULL;
#else
    buf = NULL;
#endif

    ifs = NULL;
    if (!strcmp(filename, "-"))
    {
        ifs = &cin;
#if _WIN32
        setmode(fileno(stdin), O_BINARY);
#endif
    }
    else
        ifs = new ifstream(filename, ios::binary | ios::in);

    threadActive = false;
    if (ifs && !ifs->fail())
    {
        if (parseHeader())
        {
            threadActive = true;
#if defined(ENABLE_THREAD)
            head = 0;
            tail = 0;
            for (uint32_t i = 0; i < QUEUE_SIZE; i++)
            {
                buf[i] = new char[3 * width * height / 2];
                if (buf[i] == NULL)
                {
                    x265_log(NULL, X265_LOG_ERROR, "y4m: buffer allocation failure, aborting\n");
                    threadActive = false;
                }
            }
#else // if defined(ENABLE_THREAD)
            buf = new char[3 * width * height / 2];
#endif // if defined(ENABLE_THREAD)
        }
    }
    if (!threadActive && ifs && ifs != &cin)
    {
        delete ifs;
        ifs = NULL;
    }
}

Y4MInput::~Y4MInput()
{
    if (ifs && ifs != &cin)
        delete ifs;
#if defined(ENABLE_THREAD)
    for (uint32_t i = 0; i < QUEUE_SIZE; i++)
    {
        delete[] buf[i];
    }

#else
    delete[] buf;
#endif
}

bool Y4MInput::parseHeader()
{
    if (!ifs)
        return false;

    width = 0;
    height = 0;
    rateNum = 0;
    rateDenom = 0;

    while (!ifs->eof())
    {
        // Skip Y4MPEG string
        int c = ifs->get();
        while (!ifs->eof() && (c != ' ') && (c != '\n'))
        {
            c = ifs->get();
        }

        while (c == ' ' && !ifs->eof())
        {
            // read parameter identifier
            switch (ifs->get())
            {
            case 'W':
                width = 0;
                while (!ifs->eof())
                {
                    c = ifs->get();

                    if (c == ' ' || c == '\n')
                    {
                        break;
                    }
                    else
                    {
                        width = width * 10 + (c - '0');
                    }
                }

                break;

            case 'H':
                height = 0;
                while (!ifs->eof())
                {
                    c = ifs->get();
                    if (c == ' ' || c == '\n')
                    {
                        break;
                    }
                    else
                    {
                        height = height * 10 + (c - '0');
                    }
                }

                break;

            case 'F':
                rateNum = 0;
                rateDenom = 0;
                while (!ifs->eof())
                {
                    c = ifs->get();
                    if (c == '.')
                    {
                        rateDenom = 1;
                        while (!ifs->eof())
                        {
                            c = ifs->get();
                            if (c == ' ' || c == '\n')
                            {
                                break;
                            }
                            else
                            {
                                rateNum = rateNum * 10 + (c - '0');
                                rateDenom = rateDenom * 10;
                            }
                        }

                        break;
                    }
                    else if (c == ':')
                    {
                        while (!ifs->eof())
                        {
                            c = ifs->get();
                            if (c == ' ' || c == '\n')
                            {
                                break;
                            }
                            else
                                rateDenom = rateDenom * 10 + (c - '0');
                        }

                        break;
                    }
                    else
                    {
                        rateNum = rateNum * 10 + (c - '0');
                    }
                }

                break;

            default:
                while (!ifs->eof())
                {
                    // consume this unsupported configuration word
                    c = ifs->get();
                    if (c == ' ' || c == '\n')
                        break;
                }

                break;
            }
        }

        if (c == '\n')
        {
            break;
        }
    }

    if (width < MIN_FRAME_WIDTH || width > MAX_FRAME_WIDTH ||
        height < MIN_FRAME_HEIGHT || width > MAX_FRAME_HEIGHT ||
        (rateNum / rateDenom) < 1 || (rateNum / rateDenom) > MAX_FRAME_RATE)
        return false;

    return true;
}

static const char header[] = "FRAME";

int Y4MInput::guessFrameCount()
{
    if (!ifs || ifs == &cin) 
        return -1;
    istream::pos_type cur = ifs->tellg();
    if (cur < 0)
        return -1;

    ifs->seekg(0, ios::end);
    istream::pos_type size = ifs->tellg();
    ifs->seekg(cur, ios::beg);
    if (size < 0)
        return -1;

    return (int)((size - cur) / ((width * height * 3 / 2) + strlen(header) + 1));
}

void Y4MInput::skipFrames(uint32_t numFrames)
{
    const size_t count = (width * height * 3 / 2) + strlen(header) + 1;
    if (ifs && numFrames)
    {
        ifs->ignore(count * numFrames);
    }
}

#if defined(ENABLE_THREAD)
bool Y4MInput::readPicture(x265_picture& pic)
{
    PPAStartCpuEventFunc(read_yuv);
    while (head == tail)
    {
        notEmpty.wait();
        if (!threadActive)
            return false;
    }

    if (!frameStat[head])
        return false;

    pic.planes[0] = buf[head];

    pic.planes[1] = buf[head] + width * height;

    pic.planes[2] = buf[head] + width * height + ((width * height) >> 2);

    pic.bitDepth = 8;

    pic.stride[0] = width;

    pic.stride[1] = pic.stride[2] = pic.stride[0] >> 1;
    head = (head + 1) % QUEUE_SIZE;
    notFull.trigger();

    PPAStopCpuEventFunc(read_yuv);
    return true;
}

void Y4MInput::startReader()
{
#if defined(ENABLE_THREAD)
    if (threadActive)
        start();
#endif
}

void Y4MInput::threadMain()
{
    do
    {
        if (!populateFrameQueue())
            break;
    }
    while (threadActive);

    threadActive = false;
    notEmpty.trigger();
}

bool Y4MInput::populateFrameQueue()
{
    /* strip off the FRAME header */
    char hbuf[sizeof(header)];
    if (!ifs)
        return false;

    ifs->read(hbuf, strlen(header));
    if (!ifs || memcmp(hbuf, header, strlen(header)))
    {
        if (ifs)
            x265_log(NULL, X265_LOG_ERROR, "y4m: frame header missing\n");
        return false;
    }
    /* consume bytes up to line feed */
    int c = ifs->get();
    while (c != '\n' && !ifs)
    {
        c = ifs->get();
    }

    const size_t count = width * height * 3 / 2;
    while ((tail + 1) % QUEUE_SIZE == head)
    {
        notFull.wait();
        if (!threadActive)
            return false;
    }

    ifs->read(buf[tail], count);
    frameStat[tail] = !ifs->fail();
    tail = (tail + 1) % QUEUE_SIZE;
    notEmpty.trigger();
    return !ifs->fail();
}

#else // if defined(ENABLE_THREAD)
bool Y4MInput::readPicture(x265_picture& pic)
{
    PPAStartCpuEventFunc(read_yuv);

    /* strip off the FRAME header */
    char hbuf[sizeof(header)];

    if (!ifs)
        return false;

    ifs->read(hbuf, strlen(header));
    if (!ifs || memcmp(hbuf, header, strlen(header)))
    {
        x265_log(NULL, X265_LOG_ERROR, "y4m: frame header missing\n");
        return false;
    }

    /* consume bytes up to line feed */
    int c = ifs->get();
    while (c != '\n' && !ifs)
    {
        c = ifs->get();
    }

    const size_t count = width * height * 3 / 2;

    pic.planes[0] = buf;

    pic.planes[1] = buf + width * height;

    pic.planes[2] = buf + width * height + ((width * height) >> 2);

    pic.bitDepth = 8;

    pic.stride[0] = width;

    pic.stride[1] = pic.stride[2] = pic.stride[0] >> 1;

    ifs->read(buf, count);
    PPAStopCpuEventFunc(read_yuv);

    return !ifs->fail();
}

#endif // if defined(ENABLE_THREAD)
void Y4MInput::release()
{
#if defined(ENABLE_THREAD)
    threadActive = false;
    notFull.trigger();
    stop();
#endif
    delete this;
}
