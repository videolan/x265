/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Deepthi Devaki <deepthidevaki@multicorewareinc.com>,
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

/* Interpolation Filter functions */

#include "InterpolationFilter.h"
#include "TLibCommon/TypeDef.h"
#include <string.h>

#if _MSC_VER
#pragma warning(disable: 4127) // conditional expression is constant, typical for templated functions
#pragma warning(disable: 4100) // unreferenced formal parameter
#endif

#define IF_INTERNAL_PREC 14 ///< Number of bits for internal precision
#define IF_FILTER_PREC    6 ///< Log2 of sum of filter taps
#define IF_INTERNAL_OFFS (1 << (IF_INTERNAL_PREC - 1)) ///< Offset used internally

template<int N>
void filterVertical_short_pel(int bitDepth, short *src, int srcStride, Pel *dst, int dstStride, int width, int height, short const *coeff)
{
    int cStride = srcStride;

    src -= (N / 2 - 1) * cStride;

    int offset;
    short maxVal;
    int headRoom = IF_INTERNAL_PREC - bitDepth;
    int shift = IF_FILTER_PREC;

    shift += headRoom;
    offset = 1 << (shift - 1);
    offset +=  IF_INTERNAL_OFFS << IF_FILTER_PREC;
    maxVal = (1 << bitDepth) - 1;

    int row, col;
    for (row = 0; row < height; row++)
    {
        for (col = 0; col < width; col++)
        {
            int sum;

            sum  = src[col + 0 * cStride] * coeff[0];
            sum += src[col + 1 * cStride] * coeff[1];
            if (N >= 4)
            {
                sum += src[col + 2 * cStride] * coeff[2];
                sum += src[col + 3 * cStride] * coeff[3];
            }
            if (N >= 6)
            {
                sum += src[col + 4 * cStride] * coeff[4];
                sum += src[col + 5 * cStride] * coeff[5];
            }
            if (N == 8)
            {
                sum += src[col + 6 * cStride] * coeff[6];
                sum += src[col + 7 * cStride] * coeff[7];
            }

            short val = (short)((sum + offset) >> shift);

            val = (val < 0) ? 0 : val;
            val = (val > maxVal) ? maxVal : val;

            dst[col] = (Pel)val;
        }

        src += srcStride;
        dst += dstStride;
    }
}

template<int N>
void filterHorizontal_pel_pel(int bitDepth, Pel *src, int srcStride, Pel *dst, int dstStride, int width, int height, short const *coeff)
{
    int cStride = 1;

    src -= (N / 2 - 1) * cStride;

    int offset;
    short maxVal;
    int headRoom = IF_INTERNAL_PREC - bitDepth;
    offset =  (1 << (headRoom - 1));
    maxVal = (1 << bitDepth) - 1;

    int row, col;
    for (row = 0; row < height; row++)
    {
        for (col = 0; col < width; col++)
        {
            int sum;

            sum  = src[col + 0 * cStride] * coeff[0];
            sum += src[col + 1 * cStride] * coeff[1];
            if (N >= 4)
            {
                sum += src[col + 2 * cStride] * coeff[2];
                sum += src[col + 3 * cStride] * coeff[3];
            }
            if (N >= 6)
            {
                sum += src[col + 4 * cStride] * coeff[4];
                sum += src[col + 5 * cStride] * coeff[5];
            }
            if (N == 8)
            {
                sum += src[col + 6 * cStride] * coeff[6];
                sum += src[col + 7 * cStride] * coeff[7];
            }

            short val = (short)((sum + offset) >> headRoom);

            if (val < 0) val = 0;
            if (val > maxVal) val = maxVal;
            dst[col] = (Pel)val;
        }

        src += srcStride;
        dst += dstStride;
    }
}

template<int N>
void filterHorizontal_pel_short(int bitDepth, Pel *src, int srcStride, short *dst, int dstStride, int width, int height, short const *coeff)
{
    int cStride = 1;

    src -= (N / 2 - 1) * cStride;

    int offset;
    int headRoom = IF_INTERNAL_PREC - bitDepth;
    int shift = IF_FILTER_PREC;

    shift -= headRoom;
    offset = -IF_INTERNAL_OFFS << shift;

    int row, col;
    for (row = 0; row < height; row++)
    {
        for (col = 0; col < width; col++)
        {
            int sum;

            sum  = src[col + 0 * cStride] * coeff[0];
            sum += src[col + 1 * cStride] * coeff[1];
            if (N >= 4)
            {
                sum += src[col + 2 * cStride] * coeff[2];
                sum += src[col + 3 * cStride] * coeff[3];
            }
            if (N >= 6)
            {
                sum += src[col + 4 * cStride] * coeff[4];
                sum += src[col + 5 * cStride] * coeff[5];
            }
            if (N == 8)
            {
                sum += src[col + 6 * cStride] * coeff[6];
                sum += src[col + 7 * cStride] * coeff[7];
            }

            short val = (short)((sum + offset) >> shift);
            dst[col] = val;
        }

        src += srcStride;
        dst += dstStride;
    }
}

void filterCopy(Pel *src, int srcStride, Pel *dst, int dstStride, int width, int height)
{
    int row;

    for (row = 0; row < height; row++)
    {
        memcpy(dst, src, sizeof(Pel) * width);

        src += srcStride;
        dst += dstStride;
    }
}

void filterConvertShortToPel(int bitDepth, short *src, int srcStride, Pel *dst, int dstStride, int width, int height)
{
    int shift = IF_INTERNAL_PREC - bitDepth;
    short offset = IF_INTERNAL_OFFS;

    offset += shift ? (1 << (shift - 1)) : 0;
    short maxVal = (1 << bitDepth) - 1;
    short minVal = 0;
    int row, col;
    for (row = 0; row < height; row++)
    {
        for (col = 0; col < width; col++)
        {
            short val = src[col];
            val = (val + offset) >> shift;
            if (val < minVal) val = minVal;
            if (val > maxVal) val = maxVal;
            dst[col] = (Pel)val;
        }

        src += srcStride;
        dst += dstStride;
    }
}

void filterConvertPelToShort(int bitDepth, Pel *src, int srcStride, short *dst, int dstStride, int width, int height)
{
    int shift = IF_INTERNAL_PREC - bitDepth;
    int row, col;

    for (row = 0; row < height; row++)
    {
        for (col = 0; col < width; col++)
        {
            short val = src[col] << shift;
            dst[col] = val - (short)IF_INTERNAL_OFFS;
        }

        src += srcStride;
        dst += dstStride;
    }
}

template<int N>
void filterVertical_pel_pel(int bitDepth, Pel *src, int srcStride, Pel *dst, int dstStride, int width, int height, short const *coeff)
{
    short c[8];

    c[0] = coeff[0];
    c[1] = coeff[1];
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

    int cStride = srcStride;
    src -= (N / 2 - 1) * cStride;

    int offset;
    short maxVal;
    int headRoom = IF_INTERNAL_PREC - bitDepth;
    int shift = IF_FILTER_PREC;

    shift += headRoom;
    offset = 1 << (shift - 1);
    offset += IF_INTERNAL_OFFS << IF_FILTER_PREC;
    maxVal = (1 << bitDepth) - 1;

    int row, col;

    int sumCoeffs = c[0] + c[1] + c[2] + c[3];
    if (N == 8)
        sumCoeffs += c[4] + c[5] + c[6] + c[7];

    for (row = 0; row < height; row++)
    {
        for (col = 0; col < width; col++)
        {
            int sum;

            sum  = (((short)src[col + 0 * cStride] << headRoom)) * c[0];
            sum += (((short)src[col + 1 * cStride] << headRoom)) * c[1];
            if (N >= 4)
            {
                sum += (((short)src[col + 2 * cStride] << headRoom)) * c[2];
                sum += (((short)src[col + 3 * cStride] << headRoom)) * c[3];
            }
            if (N >= 6)
            {
                sum += (((short)src[col + 4 * cStride] << headRoom)) * c[4];
                sum += (((short)src[col + 5 * cStride] << headRoom)) * c[5];
            }
            if (N == 8)
            {
                sum += (((short)src[col + 6 * cStride] << headRoom)) * c[6];
                sum += (((short)src[col + 7 * cStride] << headRoom)) * c[7];
            }

            sum -= sumCoeffs * IF_INTERNAL_OFFS;
            short val = (short)((sum + offset) >> shift);

            val = (val < 0) ? 0 : val;
            val = (val > maxVal) ? maxVal : val;

            dst[col] = (Pel)val;
        }

        src += srcStride;
        dst += dstStride;
    }
}

template
void filterVertical_pel_pel<8>(int bitDepth, Pel *src, int srcStride, Pel *dst, int dstStride, int width, int height, short const *coeff);
template
void filterVertical_short_pel<8>(int bitDepth, short *src, int srcStride, Pel *dst, int dstStride, int width, int height, short const *coeff);
template
void filterHorizontal_pel_pel<8>(int bitDepth, Pel *src, int srcStride, Pel *dst, int dstStride, int width, int height, short const *coeff);
template
void filterHorizontal_pel_short<8>(int bitDepth, Pel *src, int srcStride, short *dst, int dstStride, int width, int height, short const *coeff);

template
void filterVertical_pel_pel<4>(int bitDepth, Pel *src, int srcStride, Pel *dst, int dstStride, int width, int height, short const *coeff);
template
void filterVertical_short_pel<4>(int bitDepth, short *src, int srcStride, Pel *dst, int dstStride, int width, int height, short const *coeff);
template
void filterHorizontal_pel_pel<4>(int bitDepth, Pel *src, int srcStride, Pel *dst, int dstStride, int width, int height, short const *coeff);
template
void filterHorizontal_pel_short<4>(int bitDepth, Pel *src, int srcStride, short *dst, int dstStride, int width, int height, short const *coeff);

#if _MSC_VER
#pragma warning(default: 4127) // conditional expression is constant, typical for templated functions
#pragma warning(default: 4100)
#endif
