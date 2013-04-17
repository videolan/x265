/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Mandar Gurav <mandar@multicorewareinc.com>
 *          Deepthi Devaki Akkoorath <deepthidevaki@multicorewareinc.com>
 *          Mahesh Pittala <mahesh@multicorewareinc.com>
 *          Rajesh Paulraj <rajesh@multicorewareinc.com>
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
#include "Lib/TLibCommon/CommonDef.h"
#include <algorithm>

/* Used for filter */
#define IF_INTERNAL_PREC 14 ///< Number of bits for internal precision
#define IF_FILTER_PREC    6 ///< Log2 of sum of filter taps
#define IF_INTERNAL_OFFS (1 << (IF_INTERNAL_PREC - 1)) ///< Offset used internally

namespace {
// anonymous file-static namespace

void CDECL inversedst(short *tmp, short *block, int shift)  // input tmp, output block
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

        block[4 * i + 0] = (short)Clip3(-32768, 32767, (29 * c[0] + 55 * c[1]     + c[3]               + rnd_factor) >> shift);
        block[4 * i + 1] = (short)Clip3(-32768, 32767, (55 * c[2] - 29 * c[1]     + c[3]               + rnd_factor) >> shift);
        block[4 * i + 2] = (short)Clip3(-32768, 32767, (74 * (tmp[i] - tmp[8 + i]  + tmp[12 + i])      + rnd_factor) >> shift);
        block[4 * i + 3] = (short)Clip3(-32768, 32767, (55 * c[0] + 29 * c[2]     - c[3]               + rnd_factor) >> shift);
    }
}

template<int N, bool isLast>
void CDECL filter_nonvertical(const short *coeff,
                              pixel *      src,
                              int          srcStride,
                              pixel *      dst,
                              int          dstStride,
                              int          block_width,
                              int          block_height,
                              short        maxVal,
                              int          offset,
                              int          shift)
{
    int row, col;
    short c[8];

    c[0] = coeff[0];
    c[1] = coeff[1];
#if _MSC_VER
#pragma warning(disable: 4127) // conditional expression is constant
#endif
    if (N >= 4)
    {
        c[2] = coeff[2];
        c[3] = coeff[3];
    }

    if (N >= 6)
    {
        c[4] = coeff[4];
        c[5] = coeff[5];
    }

    if (N == 8)
    {
        c[6] = coeff[6];
        c[7] = coeff[7];
    }

    for (row = 0; row < block_height; row++)
    {
        for (col = 0; col < block_width; col++)
        {
            short sum;
            sum  = src[col + 0] * c[0];
            sum += src[col + 1] * c[1];
            if (N >= 4)
            {
                sum += src[col + 2] * c[2];
                sum += src[col + 3] * c[3];
            }

            if (N >= 6)
            {
                sum += src[col + 4] * c[4];
                sum += src[col + 5] * c[5];
            }

            if (N == 8)
            {
                sum += src[col + 6] * c[6];
                sum += src[col + 7] * c[7];
            }

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

#if _MSC_VER
#pragma warning(default: 4127) // conditional expression is constant
#endif
}
}

namespace x265 {
// x265 private namespace

void Setup_C_MacroblockPrimitives(EncoderPrimitives& p)
{
    p.inversedst = inversedst;

    p.filter[FILTER_H_4_0] = filter_nonvertical<4, 0>;
    p.filter[FILTER_H_4_1] = filter_nonvertical<4, 1>;

    p.filter[FILTER_H_8_0] = filter_nonvertical<8, 0>;
    p.filter[FILTER_H_8_1] = filter_nonvertical<8, 1>;
}
}
