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
#include <stdio.h>

using namespace x265;

Y4MInput::Y4MInput(const char *filename)
{
    fp = fopen(filename, "rb");
    allocBuf();

    if (fp)
        parseHeader();
}

Y4MInput::~Y4MInput()
{
    if (fp) fclose(fp);
}

#if _MSC_VER
#pragma warning(disable: 4127)
#endif
void Y4MInput::parseHeader()
{
    Char source[5];
    Int t_width = 0;
    Int t_height = 0;
    Int t_rateNumerator = 0;
    Int t_rateDenominator = 0;
    size_t b_error;

    while (1)
    {
        source[0] = 0x0;

        while ((source[0] != 0x20) && (source[0] != 0x0a))
        {
            b_error = fread(&source[0], 1, 1, fp);
            //headerLength++;
            if (b_error)
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
            //  read parameter identifier

            fread(&source[1], 1, 1, fp);
            //headerLength++;
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
    return 0;
}

void Y4MInput::skipFrames(int numFrames)
{
    if (!numFrames)
        return;

    //TODO : Assuming fileBitDepthY = fileBitDepthC

    const std::streamoff framesize = 1 * width * height * 3 / 2;
    const std::streamoff offset = framesize * numFrames;

    /* attempt to seek */
    if (!!fseek(fp, offset, SEEK_CUR))
        return; /* success */

    //TODO : clear file pointer
    //yuv_handler->m_cHandle.clear();

    /* fall back to consuming the input */
    Char buf[512];
    const UInt offset_mod_bufsize = offset % sizeof(buf);
    for (std::streamoff i = 0; i < offset - offset_mod_bufsize; i += sizeof(buf))
    {
        fread(buf, sizeof(buf), 1, fp);
    }

    fread(buf, offset_mod_bufsize, 1, fp);
}

static bool readPlane(Pel* dst, FILE* fp, int width, int height)
{
    Int read_len = width;

    UChar *buf = new UChar[read_len];

    for (Int y = 0; y < height; y++)
    {
        fread(reinterpret_cast<Char*>(buf), read_len, 1, fp);
        if (feof(fp))
        {
            delete[] buf;
            return false;
        }

        for (Int x = 0; x < width; x++)
        {
            dst[y * width + x] = buf[x];
        }
    }

    delete[] buf;
    return true;
}

bool Y4MInput::readPicture(Picture& pic)
{
    /*stripe off the FRAME header */
    char byte[1];
    Int cur_pointer = ftell(fp);

    cur_pointer += Y4M_FRAME_MAGIC;
    fseek(fp, cur_pointer, SEEK_SET);

    while (1)
    {
        byte[0] = 0;
        fread(byte, sizeof(char), 1, fp);
        if (isEof() || isFail())
        {
            break;
        }

        while (*byte != 0x0a)
        {
            fread(byte, sizeof(char), 1, fp);
        }

        break;
    }

    pic.planes[0] = buf;

    pic.planes[1] = buf + (width * height);

    pic.planes[2] = buf + ((width * height) + ((width >> 1) * (height >> 1)));

    //TODO : need to cahnge stride based on conformance  mode

    pic.stride[0] = width + (LumaMarginX << 1);

    pic.stride[1] = pic.stride[2] = pic.stride[0] >> 1;

    if (!readPlane((Pel*)pic.planes[0], fp, width, height))
        return false;

    if (!readPlane((Pel*)pic.planes[1], fp, width >> 1, height >> 1))
        return false;

    if (!readPlane((Pel*)pic.planes[2], fp, width >> 1, height >> 1))
        return false;

    return true;
}

void Y4MInput::allocBuf()
{
    buf = new Pel[2 * width * height];
}
