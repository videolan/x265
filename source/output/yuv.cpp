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

#include "common.h"
#include "output.h"
#include "yuv.h"

using namespace X265_NS;
using namespace std;

YUVOutput::YUVOutput(const char *filename, int w, int h, uint32_t d, int csp)
    : width(w)
    , height(h)
    , depth(d)
    , colorSpace(csp)
    , frameSize(0)
{
    ofs.open(filename, ios::binary | ios::out);
    buf = new char[width];

    for (int i = 0; i < x265_cli_csps[colorSpace].planes; i++)
        frameSize += (uint32_t)((width >> x265_cli_csps[colorSpace].width[i]) * (height >> x265_cli_csps[colorSpace].height[i]));
}

YUVOutput::~YUVOutput()
{
    ofs.close();
    delete [] buf;
}

bool YUVOutput::writePicture(const x265_picture& pic)
{
    uint64_t fileOffset = pic.poc;
    fileOffset *= frameSize;

    X265_CHECK(pic.colorSpace == colorSpace, "invalid color space\n");
    X265_CHECK(pic.bitDepth == (int)depth, "invalid bit depth\n");

#if HIGH_BIT_DEPTH
    if (depth == 8)
    {
        int shift = pic.bitDepth - 8;
        ofs.seekp((std::streamoff)fileOffset);
        for (int i = 0; i < x265_cli_csps[colorSpace].planes; i++)
        {
            uint16_t *src = (uint16_t*)pic.planes[i];
            for (int h = 0; h < height >> x265_cli_csps[colorSpace].height[i]; h++)
            {
                for (int w = 0; w < width >> x265_cli_csps[colorSpace].width[i]; w++)
                    buf[w] = (char)(src[w] >> shift);

                ofs.write(buf, width >> x265_cli_csps[colorSpace].width[i]);
                src += pic.stride[i] / sizeof(*src);
            }
        }
    }
    else
    {
        ofs.seekp((std::streamoff)(fileOffset * 2));
        for (int i = 0; i < x265_cli_csps[colorSpace].planes; i++)
        {
            uint16_t *src = (uint16_t*)pic.planes[i];
            for (int h = 0; h < height >> x265_cli_csps[colorSpace].height[i]; h++)
            {
                ofs.write((const char*)src, (width * 2) >> x265_cli_csps[colorSpace].width[i]);
                src += pic.stride[i] / sizeof(*src);
            }
        }
    }
#else // if HIGH_BIT_DEPTH
    ofs.seekp((std::streamoff)fileOffset);
    for (int i = 0; i < x265_cli_csps[colorSpace].planes; i++)
    {
        char *src = (char*)pic.planes[i];
        for (int h = 0; h < height >> x265_cli_csps[colorSpace].height[i]; h++)
        {
            ofs.write(src, width >> x265_cli_csps[colorSpace].width[i]);
            src += pic.stride[i] / sizeof(*src);
        }
    }

#endif // if HIGH_BIT_DEPTH

    return true;
}
