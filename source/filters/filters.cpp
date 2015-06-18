/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Selvakumar Nithiyaruban <selvakumar@multicorewareinc.com>
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

#include "filters.h"
#include "common.h"

using namespace X265_NS;

/* The dithering algorithm is based on Sierra-2-4A error diffusion. */
void ditherPlane(pixel *dst, int dstStride, uint16_t *src, int srcStride,
                 int width, int height, int16_t *errors, int bitDepth)
{
    const int lShift = 16 - bitDepth;
    const int rShift = 16 - bitDepth + 2;
    const int half = (1 << (16 - bitDepth + 1));
    const int pixelMax = (1 << bitDepth) - 1;

    memset(errors, 0, (width + 1) * sizeof(int16_t));
    int pitch = 1;
    for (int y = 0; y < height; y++, src += srcStride, dst += dstStride)
    {
        int16_t err = 0;
        for (int x = 0; x < width; x++)
        {
            err = err * 2 + errors[x] + errors[x + 1];
            dst[x * pitch] = (pixel)x265_clip3(0, pixelMax, ((src[x * 1] << 2) + err + half) >> rShift);
            errors[x] = err = src[x * pitch] - (dst[x * pitch] << lShift);
        }
    }
}

void ditherImage(x265_picture& picIn, int picWidth, int picHeight, int16_t *errorBuf, int bitDepth)
{
    /* This portion of code is from readFrame in x264. */
    for (int i = 0; i < x265_cli_csps[picIn.colorSpace].planes; i++)
    {
        if ((picIn.bitDepth & 7) && (picIn.bitDepth != 16))
        {
            /* upconvert non 16bit high depth planes to 16bit */
            uint16_t *plane = (uint16_t*)picIn.planes[i];
            uint32_t pixelCount = x265_picturePlaneSize(picIn.colorSpace, picWidth, picHeight, i);
            int lShift = 16 - picIn.bitDepth;

            /* This loop assumes width is equal to stride which
               happens to be true for file reader outputs */
            for (uint32_t j = 0; j < pixelCount; j++)
            {
                plane[j] = plane[j] << lShift;
            }
        }
    }

    for (int i = 0; i < x265_cli_csps[picIn.colorSpace].planes; i++)
    {
        int height = (int)(picHeight >> x265_cli_csps[picIn.colorSpace].height[i]);
        int width = (int)(picWidth >> x265_cli_csps[picIn.colorSpace].width[i]);

        ditherPlane(((pixel*)picIn.planes[i]), picIn.stride[i] / sizeof(pixel), ((uint16_t*)picIn.planes[i]),
                    picIn.stride[i] / 2, width, height, errorBuf, bitDepth);
    }
}
