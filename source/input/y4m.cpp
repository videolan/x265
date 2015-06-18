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

using namespace X265_NS;
using namespace std;

static const char header[] = "FRAME";

Y4MInput::Y4MInput(InputFileInfo& info)
{
    for (int i = 0; i < QUEUE_SIZE; i++)
        buf[i] = NULL;

    threadActive = false;
    colorSpace = info.csp;
    sarWidth = info.sarWidth;
    sarHeight = info.sarHeight;
    width = info.width;
    height = info.height;
    rateNum = info.fpsNum;
    rateDenom = info.fpsDenom;
    depth = info.depth;
    framesize = 0;

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

    if (ifs && ifs->good() && parseHeader())
    {
        int pixelbytes = depth > 8 ? 2 : 1;
        for (int i = 0; i < x265_cli_csps[colorSpace].planes; i++)
        {
            int stride = (width >> x265_cli_csps[colorSpace].width[i]) * pixelbytes;
            framesize += (stride * (height >> x265_cli_csps[colorSpace].height[i]));
        }

        threadActive = true;
        for (int q = 0; q < QUEUE_SIZE; q++)
        {
            buf[q] = X265_MALLOC(char, framesize);
            if (!buf[q])
            {
                x265_log(NULL, X265_LOG_ERROR, "y4m: buffer allocation failure, aborting");
                threadActive = false;
                break;
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

    size_t estFrameSize = framesize + strlen(header) + 1; /* assume basic FRAME\n headers */

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
                info.frameCount = (int)((size.QuadPart - (int64_t)cur) / estFrameSize);
            CloseHandle(hFile);
        }
#else // if defined(_MSC_VER) && _MSC_VER < 1700
        if (cur >= 0)
        {
            ifs->seekg(0, ios::end);
            istream::pos_type size = ifs->tellg();
            ifs->seekg(cur, ios::beg);
            if (size > 0)
                info.frameCount = (int)((size - cur) / estFrameSize);
        }
#endif // if defined(_MSC_VER) && _MSC_VER < 1700
    }

    if (info.skipFrames)
    {
#if X86_64
        ifs->seekg((uint64_t)estFrameSize * info.skipFrames, ios::cur);
#else
        for (int i = 0; i < info.skipFrames; i++)
            ifs->ignore(estFrameSize);
#endif
    }
}

Y4MInput::~Y4MInput()
{
    if (ifs && ifs != &cin)
        delete ifs;

    for (int i = 0; i < QUEUE_SIZE; i++)
        X265_FREE(buf[i]);
}

void Y4MInput::release()
{
    threadActive = false;
    readCount.poke();
    stop();
    delete this;
}

bool Y4MInput::parseHeader()
{
    if (!ifs)
        return false;

    int csp = 0;
    int d = 0;

    while (ifs->good())
    {
        // Skip Y4MPEG string
        int c = ifs->get();
        while (ifs->good() && (c != ' ') && (c != '\n'))
            c = ifs->get();

        while (c == ' ' && ifs->good())
        {
            // read parameter identifier
            switch (ifs->get())
            {
            case 'W':
                width = 0;
                while (ifs->good())
                {
                    c = ifs->get();

                    if (c == ' ' || c == '\n')
                        break;
                    else
                        width = width * 10 + (c - '0');
                }
                break;

            case 'H':
                height = 0;
                while (ifs->good())
                {
                    c = ifs->get();
                    if (c == ' ' || c == '\n')
                        break;
                    else
                        height = height * 10 + (c - '0');
                }
                break;

            case 'F':
                rateNum = 0;
                rateDenom = 0;
                while (ifs->good())
                {
                    c = ifs->get();
                    if (c == '.')
                    {
                        rateDenom = 1;
                        while (ifs->good())
                        {
                            c = ifs->get();
                            if (c == ' ' || c == '\n')
                                break;
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
                        while (ifs->good())
                        {
                            c = ifs->get();
                            if (c == ' ' || c == '\n')
                                break;
                            else
                                rateDenom = rateDenom * 10 + (c - '0');
                        }
                        break;
                    }
                    else
                        rateNum = rateNum * 10 + (c - '0');
                }
                break;

            case 'A':
                sarWidth = 0;
                sarHeight = 0;
                while (ifs->good())
                {
                    c = ifs->get();
                    if (c == ':')
                    {
                        while (ifs->good())
                        {
                            c = ifs->get();
                            if (c == ' ' || c == '\n')
                                break;
                            else
                                sarHeight = sarHeight * 10 + (c - '0');
                        }
                        break;
                    }
                    else
                        sarWidth = sarWidth * 10 + (c - '0');
                }
                break;

            case 'C':
                csp = 0;
                d = 0;
                while (ifs->good())
                {
                    c = ifs->get();

                    if (c <= '9' && c >= '0')
                        csp = csp * 10 + (c - '0');
                    else if (c == 'p')
                    {
                        // example: C420p16
                        while (ifs->good())
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
                while (ifs->good())
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
            break;
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
    THREAD_NAME("Y4MRead", 0);
    do
    {
        if (!populateFrameQueue())
            break;
    }
    while (threadActive);

    threadActive = false;
    writeCount.poke();
}

bool Y4MInput::populateFrameQueue()
{
    if (!ifs || ifs->fail())
        return false;

    /* strip off the FRAME header */
    char hbuf[sizeof(header)];

    ifs->read(hbuf, strlen(header));
    if (ifs->eof())
        return false;

    if (!ifs->good() || memcmp(hbuf, header, strlen(header)))
    {
        x265_log(NULL, X265_LOG_ERROR, "y4m: frame header missing\n");
        return false;
    }

    /* consume bytes up to line feed */
    int c = ifs->get();
    while (c != '\n' && ifs->good())
        c = ifs->get();

    /* wait for room in the ring buffer */
    int written = writeCount.get();
    int read = readCount.get();
    while (written - read > QUEUE_SIZE - 2)
    {
        read = readCount.waitForChange(read);
        if (!threadActive)
            return false;
    }

    ProfileScopeEvent(frameRead);
    ifs->read(buf[written % QUEUE_SIZE], framesize);
    if (ifs->good())
    {
        writeCount.incr();
        return true;
    }
    else
        return false;
}

bool Y4MInput::readPicture(x265_picture& pic)
{
    int read = readCount.get();
    int written = writeCount.get();

#if ENABLE_THREADING

    /* only wait if the read thread is still active */
    while (threadActive && read == written)
        written = writeCount.waitForChange(written);

#else

    populateFrameQueue();

#endif // if ENABLE_THREADING

    if (read < written)
    {
        int pixelbytes = depth > 8 ? 2 : 1;
        pic.bitDepth = depth;
        pic.colorSpace = colorSpace;
        pic.stride[0] = width * pixelbytes;
        pic.stride[1] = pic.stride[0] >> x265_cli_csps[colorSpace].width[1];
        pic.stride[2] = pic.stride[0] >> x265_cli_csps[colorSpace].width[2];
        pic.planes[0] = buf[read % QUEUE_SIZE];
        pic.planes[1] = (char*)pic.planes[0] + pic.stride[0] * height;
        pic.planes[2] = (char*)pic.planes[1] + pic.stride[1] * (height >> x265_cli_csps[colorSpace].height[1]);
        readCount.incr();
        return true;
    }
    else
        return false;
}

