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
using namespace std;

Y4MInput::Y4MInput(const char *filename)
{
    ifs.open(filename, ios::binary | ios::in);
    if (!ifs.fail())
        parseHeader();
    buf = new char[3 * width * height / 2];
}

Y4MInput::~Y4MInput()
{
    ifs.close();
    if (buf) delete[] buf;
}

void Y4MInput::parseHeader()
{
    int t_width = 0;
    int t_height = 0;
    int t_rateNumerator = 0;
    int t_rateDenominator = 0;

    while (ifs)
    {
        // Skip Y4MPEG string
        char byte = ifs.get();
        while (!ifs.eof() && (byte != ' ') && (byte != '\n'))
            byte = ifs.get();

        while (byte == ' ' && ifs)
        {
            // read parameter identifier
            switch (ifs.get())
            {
            case 'W':
                t_width = 0;
                while (ifs)
                {
                    byte = ifs.get();

                    if (byte == ' ' || byte == '\n')
                    {
                        break;
                    }
                    else
                    {
                        t_width = t_width * 10 + (byte - '0');
                    }
                }
                break;

            case 'H':
                t_height = 0;
                while (ifs)
                {
                    byte = ifs.get();
                    if (byte == ' ' || byte == '\n')
                    {
                        break;
                    }
                    else
                    {
                        t_height = t_height * 10 + (byte - '0');
                    }
                }
                break;

            case 'F':
                t_rateNumerator = 0;
                t_rateDenominator = 0;
                while (ifs)
                {
                    byte = ifs.get();
                    if (byte == '.')
                    {
                        t_rateDenominator = 1;
                        while (ifs)
                        {
                            byte = ifs.get();
                            if (byte == ' ' || byte == '\n')
                            {
                                break;
                            }
                            else
                            {
                                t_rateNumerator = t_rateNumerator * 10 + (byte - '0');
                                t_rateDenominator = t_rateDenominator * 10;
                            }
                        }

                        break;
                    }
                    else if (byte == ':')
                    {
                        while (ifs)
                        {
                            byte = ifs.get();
                            if (byte == ' ' || byte == '\n')
                            {
                                break;
                            }
                            else
                                t_rateDenominator = t_rateDenominator * 10 + (byte - '0');
                        }

                        break;
                    }
                    else
                    {
                        t_rateNumerator = t_rateNumerator * 10 + (byte - '0');
                    }
                }
                break;

            default:
                while (ifs)
                {
                    // consume this unsupported configuration word
                    byte = ifs.get();
                    if (byte == ' ' || byte == '\n')
                        break;
                }
                break;
            }
        }

        if (byte == '\n')
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

static const char header[] = "FRAME";

int Y4MInput::guessFrameCount()
{
    long cur = ifs.tellg();
    ifs.seekg (0, ios::end);
    long size = ifs.tellg();
    ifs.seekg (cur, ios::beg);

    return (int) ((size - cur) / ((width * height * 3 / 2) + strlen(header) + 1));
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
    char hbuf[sizeof(header)];
    ifs.read(hbuf, strlen(header));
    if (!ifs || strncmp(hbuf, header, strlen(header)))
    {
        fprintf(stderr, "Y4M frame header missing\n");
        return false;
    }

    /* consume bytes up to line feed */
    char byte = ifs.get();
    while (byte != '\n' && !ifs)
        byte = ifs.get();

    const size_t count = width * height * 3 / 2;

    pic.planes[0] = buf;

    pic.planes[1] = buf + width * height;

    pic.planes[2] = buf + width * height + ((width * height) >> 2);

    pic.stride[0] = width;

    pic.stride[1] = pic.stride[2] = pic.stride[0] >> 1;

    ifs.read(buf, count);
    PPAStopCpuEventFunc(read_yuv);

    return ifs.good();
}
