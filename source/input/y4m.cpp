/*****************************************************************************
 * Copyright (C) 2013-2017 MulticoreWare, Inc
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
#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE
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
static const char header[] = {'F','R','A','M','E'};
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
        ifs = stdin;
#if _WIN32
        setmode(fileno(stdin), O_BINARY);
#endif
    }
    else
        ifs = x265_fopen(info.filename, "rb");
    if (ifs && !ferror(ifs) && parseHeader())
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
        if (ifs && ifs != stdin)
            fclose(ifs);
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
    size_t estFrameSize = framesize + sizeof(header) + 1; /* assume basic FRAME\n headers */
    /* try to estimate frame count, if this is not stdin */
    if (ifs != stdin)
    {
        int64_t cur = ftello(ifs);
        if (cur >= 0)
        {
            fseeko(ifs, 0, SEEK_END);
            int64_t size = ftello(ifs);
            fseeko(ifs, cur, SEEK_SET);
            if (size > 0)
                info.frameCount = (int)((size - cur) / estFrameSize);
        }
    }
    if (info.skipFrames)
    {
        if (ifs != stdin)
            fseeko(ifs, (int64_t)estFrameSize * info.skipFrames, SEEK_CUR);
        else
            for (int i = 0; i < info.skipFrames; i++)
                if (fread(buf[0], estFrameSize - framesize, 1, ifs) + fread(buf[0], framesize, 1, ifs) != 2)
                    break;
    }
}
Y4MInput::~Y4MInput()
{
    if (ifs && ifs != stdin)
        fclose(ifs);
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
    int c;
    while ((c = fgetc(ifs)) != EOF)
    {
        // Skip Y4MPEG string
        while ((c != EOF) && (c != ' ') && (c != '\n'))
            c = fgetc(ifs);
        while (c == ' ')
        {
            // read parameter identifier
            switch (fgetc(ifs))
            {
            case 'W':
                width = 0;
                while ((c = fgetc(ifs)) != EOF)
                {
                    if (c == ' ' || c == '\n')
                        break;
                    else
                        width = width * 10 + (c - '0');
                }
                break;
            case 'H':
                height = 0;
                while ((c = fgetc(ifs)) != EOF)
                {
                    if (c == ' ' || c == '\n')
                        break;
                    else
                        height = height * 10 + (c - '0');
                }
                break;

            case 'F':
                rateNum = 0;
                rateDenom = 0;
                while ((c = fgetc(ifs)) != EOF)
                {
                    if (c == '.')
                    {
                        rateDenom = 1;
                        while ((c = fgetc(ifs)) != EOF)
                        {
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
                        while ((c = fgetc(ifs)) != EOF)
                        {
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
                while ((c = fgetc(ifs)) != EOF)
                {
                    if (c == ':')
                    {
                        while ((c = fgetc(ifs)) != EOF)
                        {
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
                while ((c = fgetc(ifs)) != EOF)
                {
                    if (c <= 'o' && c >= '0')
                        csp = csp * 10 + (c - '0');
                    else if (c == 'p')
                    {
                        // example: C420p16
                        while ((c = fgetc(ifs)) != EOF)
                        {
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

                if (csp / 100 == ('m'-'0')*1000 + ('o'-'0')*100 + ('n'-'0')*10 + ('o'-'0'))
                {
                    colorSpace = X265_CSP_I400;
                    d = csp % 100;
                }
                else if (csp / 10 == ('m'-'0')*1000 + ('o'-'0')*100 + ('n'-'0')*10 + ('o'-'0'))
                {
                    colorSpace = X265_CSP_I400;
                    d = csp % 10;
                }
                else if (csp == ('m'-'0')*1000 + ('o'-'0')*100 + ('n'-'0')*10 + ('o'-'0'))
                {
                    colorSpace = X265_CSP_I400;
                    d = 8;
                }
                else
                    colorSpace = (csp == 444) ? X265_CSP_I444 : (csp == 422) ? X265_CSP_I422 : X265_CSP_I420;

                if (d >= 8 && d <= 16)
                    depth = d;
                break;
            default:
                while ((c = fgetc(ifs)) != EOF)
                {
                    // consume this unsupported configuration word
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
        colorSpace < X265_CSP_I400 || colorSpace >= X265_CSP_COUNT)
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
    if (!ifs || ferror(ifs))
        return false;
    /* strip off the FRAME\n header */
    char hbuf[sizeof(header) + 1];
    if (fread(hbuf, sizeof(hbuf), 1, ifs) != 1 || memcmp(hbuf, header, sizeof(header)))
    {
        if (!feof(ifs))
            x265_log(NULL, X265_LOG_ERROR, "y4m: frame header missing\n");
        return false;
    }
    /* consume bytes up to line feed */
    int c = hbuf[sizeof(header)];
    while (c != '\n')
        if ((c = fgetc(ifs)) == EOF)
            break;
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
    if (fread(buf[written % QUEUE_SIZE], framesize, 1, ifs) == 1)
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
        pic.framesize = framesize;
        pic.height = height;
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

