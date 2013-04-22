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
#include <stdio.h>
#include <string.h>

using namespace x265;

#define Y4M_FRAME_MAGIC 5 // "FRAME"

Y4MInput::Y4MInput(const char *filename)
{
    fp = fopen(filename, "rb");
    if (fp)
        parseHeader();
    buf = new uint8_t[3 * width * height / 2];
}

Y4MInput::~Y4MInput()
{
    if (fp) fclose(fp);
    if (buf) delete[] buf;
}

#if _MSC_VER
#pragma warning(disable: 4127)
#endif
void Y4MInput::parseHeader()
{
    char source[5];
    int t_width = 0;
    int t_height = 0;
    int t_rateNumerator = 0;
    int t_rateDenominator = 0;

    while (1)
    {
        source[0] = 0x0;

        while ((source[0] != 0x20) && (source[0] != 0x0a))
        {
            if (fread(&source[0], 1, 1, fp) == 0)
            {
                break;
            }
        }

        if (source[0] == 0x00)
        {
            break;
        }

        while (source[0] == 0x20)
        {
            // read parameter identifier
            fread(&source[1], 1, 1, fp);
            if (source[1] == 'W')
            {
                t_width = 0;
                while (true)
                {
                    fread(&source[0], 1, 1, fp);

                    if (source[0] == 0x20 || source[0] == 0x0a)
                    {
                        break;
                    }
                    else
                    {
                        t_width = t_width * 10 + (source[0] - '0');
                    }
                }

                continue;
            }

            if (source[1] == 'H')
            {
                t_height = 0;
                while (true)
                {
                    fread(&source[0], 1, 1, fp);
                    if (source[0] == 0x20 || source[0] == 0x0a)
                    {
                        break;
                    }
                    else
                    {
                        t_height = t_height * 10 + (source[0] - '0');
                    }
                }

                continue;
            }

            if (source[1] == 'F')
            {
                t_rateNumerator = 0;
                t_rateDenominator = 0;
                while (true)
                {
                    fread(&source[0], 1, 1, fp);
                    if (source[0] == '.')
                    {
                        t_rateDenominator = 1;
                        while (true)
                        {
                            fread(&source[0], 1, 1, fp);
                            if (source[0] == 0x20 || source[0] == 0x10)
                            {
                                break;
                            }
                            else
                            {
                                t_rateNumerator = t_rateNumerator * 10 + (source[0] - '0');
                                t_rateDenominator = t_rateDenominator * 10;
                            }
                        }

                        break;
                    }
                    else if (source[0] == ':')
                    {
                        while (true)
                        {
                            fread(&source[0], 1, 1, fp);
                            if (source[0] == 0x20 || source[0] == 0x0a)
                            {
                                break;
                            }
                            else
                                t_rateDenominator = t_rateDenominator * 10 + (source[0] - '0');
                        }

                        break;
                    }
                    else
                    {
                        t_rateNumerator = t_rateNumerator * 10 + (source[0] - '0');
                    }
                }

                continue;
            }

            break;
        }

        if (source[0] == 0x0a)
        {
            break;
        }
    }

//TODO need to include code for parsing colourspace from file.

    width = t_width;
    height = t_height;
    rateNum = t_rateNumerator;
    rateDenom = t_rateDenominator;
}

int  Y4MInput::guessFrameCount() const
{
    /* TODO: Get file size, subtract file header, divide by (framesize+frameheader) */
    return 0;
}

void Y4MInput::skipFrames(int numFrames)
{
    x265_picture pic;

    for (int i = 0; i < numFrames; i++)
    {
        readPicture(pic);
    }
}

bool Y4MInput::readPicture(x265_picture& pic)
{
    PPAStartCpuEventFunc(read_yuv);

    /* strip off the FRAME header */
    char header[Y4M_FRAME_MAGIC];

    if (fread(&header, 1, sizeof(header), fp) < sizeof(header))
        return false;
    if (!strncmp(header, "FRAME", Y4M_FRAME_MAGIC))
    {
        fprintf(stderr, "Y4M frame header missing\n");
        return false;
    }

    /* consume bytes up to line feed */
    char byte;
    do
    {
        if (fread(&byte, 1, 1, fp) == 0)
        {
            fprintf(stderr, "Y4M frame header incomplete\n");
            return false;
        }
    }
    while (byte != '\n');

    const size_t count = width * height * 3 / 2;

    pic.planes[0] = buf;

    pic.planes[1] = buf + (width * height);

    pic.planes[2] = buf + ((width * height) + ((width >> 1) * (height >> 1)));

    pic.stride[0] = width;

    pic.stride[1] = pic.stride[2] = pic.stride[0] >> 1;

    size_t bytes = fread(buf, 1, count, fp);
    PPAStopCpuEventFunc(read_yuv);

    return bytes == count;
}
