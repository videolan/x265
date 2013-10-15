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

using namespace x265;
using namespace std;

Y4MInput::Y4MInput(const char *filename)
{
    ifs.open(filename, ios::binary | ios::in);
    threadActive = false;
    if (!ifs.fail())
    {
        if (parseHeader())
        {
            threadActive = true;
#if defined(ENABLE_THREAD)
            for (int i = 0; i < QUEUE_SIZE; i++)
            {
                buf[i] = new char[3 * width * height / 2];
                if ( buf[i] == NULL)
                {
                    x265_log(NULL, X265_LOG_ERROR, "y4m: buffer allocation failure, aborting");
                    threadActive = false;
                }
            }
            head = 0;
            tail = 0;
            start();
#else
            buf = new char[3 * width * height / 2];
#endif
        }
    }
    if (!threadActive)
        ifs.close();
}

Y4MInput::~Y4MInput()
{
    ifs.close();
#if defined(ENABLE_THREAD)
    for (int i = 0; i < QUEUE_SIZE; i++)
    {
        delete[] buf[i];
    }
#else
    delete[] buf;
#endif
}

bool Y4MInput::parseHeader()
{
    int frame_width = 0;
    int frame_height = 0;
    int frame_rateNumerator = 0;
    int frame_rateDenominator = 0;

    while (ifs)
    {
        // Skip Y4MPEG string
        int c = ifs.get();
        while (!ifs.eof() && (c != ' ') && (c != '\n'))
        {
            c = ifs.get();
        }

        while (c == ' ' && ifs)
        {
            // read parameter identifier
            switch (ifs.get())
            {
            case 'W':
                frame_width = 0;
                while (ifs)
                {
                    c = ifs.get();

                    if (c == ' ' || c == '\n')
                    {
                        break;
                    }
                    else
                    {
                        frame_width = frame_width * 10 + (c - '0');
                    }
                }
                break;

            case 'H':
                frame_height = 0;
                while (ifs)
                {
                    c = ifs.get();
                    if (c == ' ' || c == '\n')
                    {
                        break;
                    }
                    else
                    {
                        frame_height = frame_height * 10 + (c - '0');
                    }
                }

                break;

            case 'F':
                frame_rateNumerator = 0;
                frame_rateDenominator = 0;
                while (ifs)
                {
                    c = ifs.get();
                    if (c == '.')
                    {
                        frame_rateDenominator = 1;
                        while (ifs)
                        {
                            c = ifs.get();
                            if (c == ' ' || c == '\n')
                            {
                                break;
                            }
                            else
                            {
                                frame_rateNumerator = frame_rateNumerator * 10 + (c - '0');
                                frame_rateDenominator = frame_rateDenominator * 10;
                            }
                        }

                        break;
                    }
                    else if (c == ':')
                    {
                        while (ifs)
                        {
                            c = ifs.get();
                            if (c == ' ' || c == '\n')
                            {
                                break;
                            }
                            else
                                frame_rateDenominator = frame_rateDenominator * 10 + (c - '0');
                        }

                        break;
                    }
                    else
                    {
                        frame_rateNumerator = frame_rateNumerator * 10 + (c - '0');
                    }
                }

                break;

            default:
                while (ifs)
                {
                    // consume this unsupported configuration word
                    c = ifs.get();
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

//TODO need to include code for parsing colourspace from file.

    if (frame_width < MIN_FRAME_WIDTH || frame_width > MAX_FRAME_WIDTH ||
        frame_height < MIN_FRAME_HEIGHT || frame_width > MAX_FRAME_HEIGHT ||
        (frame_rateNumerator / frame_rateDenominator) < 1 || (frame_rateNumerator / frame_rateDenominator) > MAX_FRAME_RATE)
        return false;

    width = frame_width;
    height = frame_height;
    rateNum = frame_rateNumerator;
    rateDenom = frame_rateDenominator;
    return true;
}

static const char header[] = "FRAME";

int Y4MInput::guessFrameCount()
{
    istream::pos_type cur = ifs.tellg();

    ifs.seekg(0, ios::end);
    istream::pos_type size = ifs.tellg();
    ifs.seekg(cur, ios::beg);

    return (int)((size - cur) / ((width * height * 3 / 2) + strlen(header) + 1));
}

void Y4MInput::skipFrames(int numFrames)
{
    x265_picture_t pic;

    for (int i = 0; i < numFrames; i++)
    {
        readPicture(pic);
    }
}

#if defined(ENABLE_THREAD)
bool Y4MInput::readPicture(x265_picture_t& pic)
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

void Y4MInput::threadMain()
{
    do
    {
        if (!populateFrameQueue())
            break;
    }
    while (threadActive);
}

bool Y4MInput::populateFrameQueue()
{
    /* strip off the FRAME header */
    char hbuf[sizeof(header)];
    ifs.read(hbuf, strlen(header));
    if (!ifs || memcmp(hbuf, header, strlen(header)))
    {
        if (ifs)
            x265_log(NULL, X265_LOG_ERROR, "y4m: frame header missing");
        return false;
    }
    /* consume bytes up to line feed */
    int c = ifs.get();
    while (c != '\n' && !ifs)
    {
        c = ifs.get();
    }
    const size_t count = width * height * 3 / 2;
    while ((tail + 1) % QUEUE_SIZE == head)
    {
        notFull.wait();
        if (!threadActive)
            break;
    }
    ifs.read(buf[tail], count);
    frameStat[tail] = ifs.good();
    if (!frameStat[tail])
        return false;
    tail = (tail + 1) % QUEUE_SIZE;
    notEmpty.trigger();
    return true;
}
#else
bool Y4MInput::readPicture(x265_picture_t& pic)
{
    PPAStartCpuEventFunc(read_yuv);

    /* strip off the FRAME header */
    char hbuf[sizeof(header)];
    ifs.read(hbuf, strlen(header));
    if (!ifs || memcmp(hbuf, header, strlen(header)))
    {
        x265_log(NULL, X265_LOG_ERROR, "y4m: frame header missing");
        return false;
    }

    /* consume bytes up to line feed */
    int c = ifs.get();
    while (c != '\n' && !ifs)
    {
        c = ifs.get();
    }

    const size_t count = width * height * 3 / 2;

    pic.planes[0] = buf;

    pic.planes[1] = buf + width * height;

    pic.planes[2] = buf + width * height + ((width * height) >> 2);

    pic.bitDepth = 8;

    pic.stride[0] = width;

    pic.stride[1] = pic.stride[2] = pic.stride[0] >> 1;

    ifs.read(buf, count);
    PPAStopCpuEventFunc(read_yuv);

    return ifs.good();
}
#endif
void Y4MInput::release()
{
#if defined(ENABLE_THREAD)
    threadActive = false;
    notFull.trigger();
    stop();
#endif
    delete this;
}
