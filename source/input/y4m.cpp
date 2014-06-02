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
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#include "y4m.h"
#include "PPA/ppa.h"
#include "common.h"

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

static const char header[] = "FRAME";

Y4MInput::Y4MInput(InputFileInfo& info)
{
    for (uint32_t i = 0; i < QUEUE_SIZE; i++)
    {
        plane[i][2] = plane[i][1] = plane[i][0] = NULL;
        frameStat[i] = false;
    }

    head.set(0);
    tail.set(0);

    threadActive = false;
    colorSpace = info.csp;
    sarWidth = info.sarWidth;
    sarHeight = info.sarHeight;
    width = info.width;
    height = info.height;
    rateNum = info.fpsNum;
    rateDenom = info.fpsDenom;
    depth = info.depth;

    ifs = NULL;
    if (!strcmp(info.filename, "-"))
    {
        ifs = &cin;
#if _WIN32
        setmode(fileno(stdin), O_BINARY);
#endif
    }
    else
        ifs = new ifstream(info.filename, ios::binary | ios::in);

    uint32_t bytesPerPixel = 1;
    size_t frameSize = strlen(header) + 1;
    if (ifs && ifs->good() && parseHeader())
    {
        bytesPerPixel = depth > 8 ? 2 : 1;
        for (int i = 0; i < x265_cli_csps[colorSpace].planes; i++)
        {
            plane_stride[i] = (uint32_t)(width >> x265_cli_csps[colorSpace].width[i]) * bytesPerPixel;
            plane_size[i] =   (uint32_t)(plane_stride[i] * (height >> x265_cli_csps[colorSpace].height[i]));
            frameSize += plane_size[i];
        }
        threadActive = true;
        for (uint32_t q = 0; q < QUEUE_SIZE && threadActive; q++)
        {
            for (int i = 0; i < x265_cli_csps[colorSpace].planes; i++)
            {
                plane[q][i] = X265_MALLOC(char, plane_size[i]);
                if (!plane[q][i])
                {
                    x265_log(NULL, X265_LOG_ERROR, "y4m: buffer allocation failure, aborting");
                    threadActive = false;
                    break;
                }
            }
        }
    }
    if (!threadActive)
    {
        if (ifs && ifs != &cin)
            delete ifs;
        ifs = NULL;
        return;
    }

    info.width = width;
    info.height = height;
    info.sarHeight = sarHeight;
    info.sarWidth = sarWidth;
    info.fpsNum = rateNum;
    info.fpsDenom = rateDenom;
    info.csp = colorSpace;
    info.depth = depth;
    info.frameCount = -1;

    /* try to estimate frame count, if this is not stdin */
    if (ifs != &cin)
    {
        istream::pos_type cur = ifs->tellg();

#if defined(_MSC_VER) && _MSC_VER < 1700
        /* Older MSVC versions cannot handle 64bit file sizes properly, so go native */
        HANDLE hFile = CreateFileA(info.filename, GENERIC_READ,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            LARGE_INTEGER size;
            if (GetFileSizeEx(hFile, &size))
                info.frameCount = (int)((size.QuadPart - (int64_t)cur) / frameSize);
            CloseHandle(hFile);
        }
#else // if defined(_MSC_VER) && _MSC_VER < 1700
        if (cur >= 0)
        {
            ifs->seekg(0, ios::end);
            istream::pos_type size = ifs->tellg();
            ifs->seekg(cur, ios::beg);
            if (size > 0)
                info.frameCount = (int)((size - cur) / frameSize);
        }
#endif // if defined(_MSC_VER) && _MSC_VER < 1700
    }

    if (info.skipFrames)
    {
#if X86_64
        ifs->seekg((uint64_t)frameSize * info.skipFrames, ios::cur);
#else
        for (int i = 0; i < info.skipFrames; i++)
        {
            ifs->ignore(frameSize);
        }
#endif
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
            x265_free(plane[i][j]);
        }
    }
}

bool Y4MInput::parseHeader()
{
    if (!ifs)
        return false;

    int csp = 0;
    int d = 0;

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

            case 'A':
                sarWidth = 0;
                sarHeight = 0;
                while (!ifs->eof())
                {
                    c = ifs->get();
                    if (c == ':')
                    {
                        while (!ifs->eof())
                        {
                            c = ifs->get();
                            if (c == ' ' || c == '\n')
                            {
                                break;
                            }
                            else
                                sarHeight = sarHeight * 10 + (c - '0');
                        }

                        break;
                    }
                    else
                    {
                        sarWidth = sarWidth * 10 + (c - '0');
                    }
                }

                break;

            case 'C':
                csp = 0;
                d = 0;
                while (!ifs->eof())
                {
                    c = ifs->get();

                    if (c <= '9' && c >= '0')
                    {
                        csp = csp * 10 + (c - '0');
                    }
                    else if (c == 'p')
                    {
                        // example: C420p16
                        while (!ifs->eof())
                        {
                            c = ifs->get();

                            if (c <= '9' && c >= '0')
                                d = d * 10 + (c - '0');
                            else
                                break;
                        }
                        break;
                    }
                    else
                        break;
                }

                if (d >= 8 && d <= 16)
                    depth = d;
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
        height < MIN_FRAME_HEIGHT || height > MAX_FRAME_HEIGHT ||
        (rateNum / rateDenom) < 1 || (rateNum / rateDenom) > MAX_FRAME_RATE ||
        colorSpace <= X265_CSP_I400 || colorSpace >= X265_CSP_COUNT)
        return false;

    return true;
}

void Y4MInput::startReader()
{
#if ENABLE_THREADING
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

    /* "open the throttle" at the end, allow reader to consume
     * remaining valid queue entries */
    threadActive = false;
    frameStat[tail.get()] = false;
    tail.set(QUEUE_SIZE);
}

bool Y4MInput::readPicture(x265_picture& pic)
{
    PPAStartCpuEventFunc(read_yuv);
    int curHead = head.get();
    int curTail = tail.get();

#if ENABLE_THREADING

    while (curHead == curTail)
    {
        curTail = tail.waitForChange(curTail);
        if (!threadActive)
            return false;
    }

#else

    populateFrameQueue();

#endif // if ENABLE_THREADING

    if (!frameStat[curHead])
        return false;

    pic.bitDepth = depth;
    pic.colorSpace = colorSpace;
    for (int i = 0; i < x265_cli_csps[colorSpace].planes; i++)
    {
        pic.planes[i] = plane[curHead][i];
        pic.stride[i] = plane_stride[i];
    }

    head.set((curHead + 1) % QUEUE_SIZE);

    PPAStopCpuEventFunc(read_yuv);
    return true;
}

bool Y4MInput::populateFrameQueue()
{
    /* strip off the FRAME header */
    char hbuf[sizeof(header)];

    if (!ifs)
        return false;

    ifs->read(hbuf, strlen(header));
    if (!ifs->good())
        return false;

    if (memcmp(hbuf, header, strlen(header)))
    {
        x265_log(NULL, X265_LOG_ERROR, "y4m: frame header missing\n");
        return false;
    }
    /* consume bytes up to line feed */
    int c = ifs->get();
    while (c != '\n' && ifs->good())
    {
        c = ifs->get();
    }

    int curTail = tail.get();
    int curHead = head.get();
    while ((curTail + 1) % QUEUE_SIZE == curHead)
    {
        curHead = head.waitForChange(curHead);
        if (!threadActive)
            return false;
    }

    for (int i = 0; i < x265_cli_csps[colorSpace].planes; i++)
    {
        ifs->read(plane[curTail][i], plane_size[i]);
    }

    frameStat[curTail] = !ifs->fail();
    tail.set((curTail + 1) % QUEUE_SIZE);
    return !ifs->fail();
}

void Y4MInput::release()
{
    threadActive = false;
    head.set(QUEUE_SIZE); // unblock file reader
    stop();
    delete this;
}
