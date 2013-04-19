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

#include <assert.h>
#include <sys/stat.h>

#include "input.h"
#include "yuv.h"

using namespace x265;

YUVInput::YUVInput(const char *filename)
{
    fp = fopen(filename, "rb");
    width = height = 0;
}

YUVInput::~YUVInput()
{
    if (fp) fclose(fp);
}

int  YUVInput::guessFrameCount() const
{
    return 0;
}

void YUVInput::skipFrames(int numFrames)
{
    int fileBitDepthY, fileBitDepthC;

    if (!numFrames)
        return;

    //TODO : Assuming fileBitDepthY = fileBitDepthC
    fileBitDepthY = fileBitDepthC = depth;
    const UInt wordsize = (fileBitDepthY > 8 || fileBitDepthC > 8) ? 2 : 1;
    const std::streamoff framesize = wordsize * width * height * 3 / 2;
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

static bool readPlane(Pel* dst, FILE* fp, int width, int height, Bool is16bit)
{
    size_t numread;
    Int read_len = width * (is16bit ? 2 : 1);
    UChar *buf = new UChar[read_len];

    for (Int y = 0; y < height; y++)
    {
        numread = fread(reinterpret_cast<Char*>(buf), 1, read_len, fp);

        if (feof(fp))
        {
            delete[] buf;
            return false;
        }

        if (!is16bit)
        {
            for (Int x = 0; x < width; x++)
            {
                dst[y * width + x] = buf[x];
            }
        }
        else
        {
            for (Int x = 0; x < width; x++)
            {
                dst[y * width + x] = (buf[2 * x + 1] << 8) | buf[2 * x];
            }
        }
    }

    delete[] buf;
    return true;
}

bool YUVInput::readPicture(Picture& pic)
{
    Bool is16bit = depth > 8;

    pic.planes[0] = buf;

    pic.planes[1] = buf + (width * height);

    pic.planes[2] = buf + ((width * height) + ((width >> 1) * (height >> 1)));

    pic.bitDepth = getBitDepth();

    //TODO : need to cahnge stride based on conformance  mode

    pic.stride[0] = width + (LumaMarginX << 1);

    pic.stride[1] = pic.stride[2] = pic.stride[0] >> 1;

    if (!readPlane((Pel*)pic.planes[0], fp, width, height, is16bit))
        return false;

    if (!readPlane((Pel*)pic.planes[1], fp, width >> 1, height >> 1, is16bit))
        return false;

    if (!readPlane((Pel*)pic.planes[2], fp, width >> 1, height >> 1, is16bit))
        return false;

    return true;
}

void YUVInput::allocBuf()
{
    buf = new Pel[2 * width * height];
}
