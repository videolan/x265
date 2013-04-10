/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors:
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

#include "primitives.h"
#include "butterfly.h"
#include <algorithm>

/** clip a, such that minVal <= a <= maxVal */
template<typename Type>
///< general min/max clip
inline Type Clip3(Type minVal, Type maxVal, Type a) { return std::min<Type>(std::max<Type>(minVal, a), maxVal);}

namespace {
// anonymous file-static namespace

void CDECL inversedst(pixel *tmp, pixel *block, int shift)  // input tmp, output block
{
    int i, c[4];
    int rnd_factor = 1 << (shift - 1);

    for (i = 0; i < 4; i++)
    {
        // Intermediate Variables
        c[0] = tmp[i] + tmp[8 + i];
        c[1] = tmp[8 + i] + tmp[12 + i];
        c[2] = tmp[i] - tmp[12 + i];
        c[3] = 74 * tmp[4 + i];

        block[4 * i + 0] = (pixel)Clip3(-32768, 32767, (29 * c[0] + 55 * c[1]     + c[3]               + rnd_factor) >> shift);
        block[4 * i + 1] = (pixel)Clip3(-32768, 32767, (55 * c[2] - 29 * c[1]     + c[3]               + rnd_factor) >> shift);
        block[4 * i + 2] = (pixel)Clip3(-32768, 32767, (74 * (tmp[i] - tmp[8 + i]  + tmp[12 + i])      + rnd_factor) >> shift);
        block[4 * i + 3] = (pixel)Clip3(-32768, 32767, (55 * c[0] + 29 * c[2]     - c[3]               + rnd_factor) >> shift);
    }
}

void CDECL filter_8_nonvertical(pixel *coeff,
                                pixel *src,
                                int    srcStride,
                                pixel *dst,
                                int    dstStride,
                                int    block_width,
                                int    block_height,
                                short  maxVal,
                                int    shift,
                                int    offset,
                                bool   isLast)
{
    int row, col;
    short c[8];

    c[0] = coeff[0];
    c[1] = coeff[1];
    c[2] = coeff[2];
    c[3] = coeff[3];
    c[4] = coeff[4];
    c[5] = coeff[5];
    c[6] = coeff[6];
    c[7] = coeff[7];
    for (row = 0; row < block_height; row++)
    {
        for (col = 0; col < block_width; col++)
        {
            int sum;
            sum  = src[col + 0] * c[0];
            sum += src[col + 1] * c[1];
            sum += src[col + 2] * c[2];
            sum += src[col + 3] * c[3];
            sum += src[col + 4] * c[4];
            sum += src[col + 5] * c[5];
            sum += src[col + 6] * c[6];
            sum += src[col + 7] * c[7];
            short val = (short)(sum + offset) >> shift;
            if (isLast)
            {
                val = (val < 0) ? 0 : val;
                val = (val > maxVal) ? maxVal : val;
            }

            dst[col] = val;
        }

        src += srcStride;
        dst += dstStride;
    }
}

void CDECL PartialButterfly16(pixel *src, pixel *dst, int shift, int line)
{
    int j, k;
    int E[8], O[8];
    int EE[4], EO[4];
    int EEE[2], EEO[2];
    int add = 1 << (shift - 1);

    for (j = 0; j < line; j++)
    {
        /* E and O */
        for (k = 0; k < 8; k++)
        {
            E[k] = src[k] + src[15 - k];
            O[k] = src[k] - src[15 - k];
        }

        /* EE and EO */
        for (k = 0; k < 4; k++)
        {
            EE[k] = E[k] + E[7 - k];
            EO[k] = E[k] - E[7 - k];
        }

        /* EEE and EEO */
        EEE[0] = EE[0] + EE[3];
        EEO[0] = EE[0] - EE[3];
        EEE[1] = EE[1] + EE[2];
        EEO[1] = EE[1] - EE[2];

        dst[0] = (pixel)(g_aiT16[0][0] * EEE[0] + g_aiT16[0][1] * EEE[1] + add) >> shift;
        dst[8 * line] = (pixel)(g_aiT16[8][0] * EEE[0] + g_aiT16[8][1] * EEE[1] + add) >> shift;
        dst[4 * line] = (pixel)(g_aiT16[4][0] * EEO[0] + g_aiT16[4][1] * EEO[1] + add) >> shift;
        dst[12 * line] = (pixel)(g_aiT16[12][0] * EEO[0] + g_aiT16[12][1] * EEO[1] + add) >> shift;

        for (k = 2; k < 16; k += 4)
        {
            dst[k *
                line] =
                (pixel)(g_aiT16[k][0] * EO[0] + g_aiT16[k][1] * EO[1] + g_aiT16[k][2] * EO[2] + g_aiT16[k][3] * EO[3] +
                        add) >> shift;
        }

        for (k = 1; k < 16; k += 2)
        {
            dst[k * line] = (pixel)(g_aiT16[k][0] * O[0] + g_aiT16[k][1] * O[1] + g_aiT16[k][2] * O[2] + g_aiT16[k][3] * O[3] +
                                    g_aiT16[k][4] * O[4] + g_aiT16[k][5] * O[5] + g_aiT16[k][6] * O[6] + g_aiT16[k][7] * O[7] +
                                    add) >> shift;
        }

        src += 16;
        dst++;
    }
}
}

namespace x265 {
// x265 private namespace

void Setup_C_MacroblockPrimitives(EncoderPrimitives& p)
{
    p.inversedst = inversedst;
    p.filter_8_nonvertical = filter_8_nonvertical;
    p.partial_butterfly[BUTTERFLY_16] = PartialButterfly16;
}
}
