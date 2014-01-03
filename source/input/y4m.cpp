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
#include <io.h>
#include <fcntl.h>
#if defined(_MSC_VER)
#pragma warning(disable: 4996) // POSIX setmode and fileno deprecated
#endif
#endif

using namespace x265;
using namespace std;

Y4MInput::Y4MInput(const char *filename, uint32_t /*inputBitDepth*/)
{
    for (uint32_t i = 0; i < QUEUE_SIZE; i++)
    {
        plane[i][2] = plane[i][1] = plane[i][0] = NULL;
    }

    head = tail = 0;
    colorSpace = X265_CSP_I420;

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
            for (uint32_t i = 0; i < QUEUE_SIZE; i++)
            {
                pictureAlloc(i);
            }
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

    for (uint32_t i = 0; i < QUEUE_SIZE; i++)
    {
        for (int j = 0; j < x265_cli_csps[colorSpace].planes; j++)
        {
            delete[] plane[i][j];
        }
    }
}

void Y4MInput::pictureAlloc(int queueindex)
{
    for (int i = 0; i < x265_cli_csps[colorSpace].planes; i++)
    {
        plane_size[i] = (uint32_t)((width >> x265_cli_csps[colorSpace].width[i]) * (height >> x265_cli_csps[colorSpace].height[i]));
        plane[queueindex][i] = new char[plane_size[i]];
        plane_stride[i] = (uint32_t)(width >> x265_cli_csps[colorSpace].width[i]);

        if (plane[queueindex][i] == NULL)
        {
            x265_log(NULL, X265_LOG_ERROR, "y4m: buffer allocation failure, aborting");
            threadActive = false;
            return;
        }
    }
}

bool Y4MInput::parseHeader()
{
    if (!ifs)
        return false;

    width = 0;
    height = 0;
    rateNum = 0;
    rateDenom = 0;
    colorSpace = X265_CSP_I420;
    int csp = 0;

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

            case 'C':
                while (!ifs->eof())
                {
                    c = ifs->get();

                    if (c == ' ' || c == '\n')
                    {
                        break;
                    }
                    else
                    {
                        csp = csp * 10 + (c - '0');
                    }
                }

                colorSpace = (csp == 444) ? X265_CSP_I444 : (csp == 422) ? X265_CSP_I422 : X265_CSP_I420;
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
        (rateNum / rateDenom) < 1 || (rateNum / rateDenom) > MAX_FRAME_RATE ||
        colorSpace <= X265_CSP_I400 || colorSpace >= X265_CSP_COUNT)
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

    int frameSize = 0;
    for (int i = 0; i < x265_cli_csps[colorSpace].planes; i++)
    {
        frameSize += (uint32_t)((width >> x265_cli_csps[colorSpace].width[i]) * (height >> x265_cli_csps[colorSpace].height[i]));
    }

    return (int)((size - cur) / (frameSize + strlen(header) + 1));
}

void Y4MInput::skipFrames(uint32_t numFrames)
{
    if (ifs && numFrames)
    {
        size_t frameSize = strlen(header) + 1;

        for (int i = 0; i < x265_cli_csps[colorSpace].planes; i++)
            frameSize += (size_t)((width >> x265_cli_csps[colorSpace].width[i]) * (height >> x265_cli_csps[colorSpace].height[i]));

        for (uint32_t i = 0; i < numFrames; i++)
            ifs->ignore(frameSize);
    }
}

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

    for (int i = 0; i < x265_cli_csps[colorSpace].planes; i++)
    {
        pic.planes[i] = plane[head][i];
        pic.stride[i] = plane_stride[i];
    }

    head = (head + 1) % QUEUE_SIZE;
    notFull.trigger();

    PPAStopCpuEventFunc(read_yuv);
    return true;
}

void Y4MInput::startReader()
{
    if (threadActive)
        start();
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

    while ((tail + 1) % QUEUE_SIZE == head)
    {
        notFull.wait();
        if (!threadActive)
            return false;
    }

    for (int i = 0; i < x265_cli_csps[colorSpace].planes; i++)
    {
        ifs->read(plane[tail][i], plane_size[i]);
    }

    frameStat[tail] = !ifs->fail();
    tail = (tail + 1) % QUEUE_SIZE;
    notEmpty.trigger();
    return !ifs->fail();
}

void Y4MInput::release()
{
    threadActive = false;
    notFull.trigger();
    stop();
    delete this;
}
