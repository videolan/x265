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

#ifndef X265_INTERPOLATIONFILTER_H
#define X265_INTERPOLATIONFILTER_H

typedef short Pel;

const short m_lumaFilter[4][8] =
{
    {  0, 0,   0, 64,  0,   0, 0,  0 },
    { -1, 4, -10, 58, 17,  -5, 1,  0 },
    { -1, 4, -11, 40, 40, -11, 4, -1 },
    {  0, 1,  -5, 17, 58, -10, 4, -1 }
};

const short m_chromaFilter[8][4] =
{
    {  0, 64,  0,  0 },
    { -2, 58, 10, -2 },
    { -4, 54, 16, -2 },
    { -6, 46, 28, -4 },
    { -4, 36, 36, -4 },
    { -4, 28, 46, -6 },
    { -2, 16, 54, -4 },
    { -2, 10, 58, -2 }
};

#define IF_INTERNAL_PREC 14 ///< Number of bits for internal precision
#define IF_FILTER_PREC    6 ///< Log2 of sum of filter taps
#define IF_INTERNAL_OFFS (1 << (IF_INTERNAL_PREC - 1)) ///< Offset used internally

template<int N>
void filterVertical_pel_pel(int bitDepth, Pel *src, int srcStride, Pel *dst, int dstStride, int width, int height, short const *coeff)
{
    short c[8];

    // int N=8;

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

    for (row = 0; row < height; row++)
    {
        for (col = 0; col < width; col++)
        {
            int sum;

            sum  = (((short)src[col + 0 * cStride] << headRoom) - (short)IF_INTERNAL_OFFS) * c[0];
            sum += (((short)src[col + 1 * cStride] << headRoom) - (short)IF_INTERNAL_OFFS) * c[1];
            if (N >= 4)
            {
                sum += (((short)src[col + 2 * cStride] << headRoom) - (short)IF_INTERNAL_OFFS) * c[2];
                sum += (((short)src[col + 3 * cStride] << headRoom) - (short)IF_INTERNAL_OFFS) * c[3];
            }
            if (N >= 6)
            {
                sum += (((short)src[col + 4 * cStride] << headRoom) - (short)IF_INTERNAL_OFFS) * c[4];
                sum += (((short)src[col + 5 * cStride] << headRoom) - (short)IF_INTERNAL_OFFS) * c[5];
            }
            if (N == 8)
            {
                sum += (((short)src[col + 6 * cStride] << headRoom) - (short)IF_INTERNAL_OFFS) * c[6];
                sum += (((short)src[col + 7 * cStride] << headRoom) - (short)IF_INTERNAL_OFFS) * c[7];
            }

            short val = (short)((sum + offset) >> shift);

            val = (val < 0) ? 0 : val;
            val = (val > maxVal) ? maxVal : val;

            dst[col] = val;
        }

        src += srcStride;
        dst += dstStride;
    }
}

template<int N>
void filterVertical_short_pel(int bitDepth, short *src, int srcStride, Pel *dst, int dstStride, int width, int height, short const *coeff)
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
    offset +=  IF_INTERNAL_OFFS << IF_FILTER_PREC;
    maxVal = (1 << bitDepth) - 1;

    int row, col;
    for (row = 0; row < height; row++)
    {
        for (col = 0; col < width; col++)
        {
            int sum;

            sum  = src[col + 0 * cStride] * c[0];
            sum += src[col + 1 * cStride] * c[1];
            if (N >= 4)
            {
                sum += src[col + 2 * cStride] * c[2];
                sum += src[col + 3 * cStride] * c[3];
            }
            if (N >= 6)
            {
                sum += src[col + 4 * cStride] * c[4];
                sum += src[col + 5 * cStride] * c[5];
            }
            if (N == 8)
            {
                sum += src[col + 6 * cStride] * c[6];
                sum += src[col + 7 * cStride] * c[7];
            }

            short val = (sum + offset) >> shift;

            val = (val < 0) ? 0 : val;
            val = (val > maxVal) ? maxVal : val;

            dst[col] = val;
        }

        src += srcStride;
        dst += dstStride;
    }
}

template<int N>
void filterHorizontal_pel_pel(int bitDepth, Pel *src, int srcStride, Pel *dst, int dstStride, int width, int height, short const *coeff)
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

    int cStride = 1;
    src -= (N / 2 - 1) * cStride;

    int offset;
    short maxVal;
    int headRoom = IF_INTERNAL_PREC - bitDepth;
    int shift = IF_FILTER_PREC;

    shift -= headRoom;
    offset = -IF_INTERNAL_OFFS << shift;
    maxVal = (1 << bitDepth) - 1;
    short offsetPost = IF_INTERNAL_OFFS;
    offsetPost += headRoom ? (1 << (headRoom - 1)) : 0;

    int row, col;
    for (row = 0; row < height; row++)
    {
        for (col = 0; col < width; col++)
        {
            int sum;

            sum  = src[col + 0 * cStride] * c[0];
            sum += src[col + 1 * cStride] * c[1];
            if (N >= 4)
            {
                sum += src[col + 2 * cStride] * c[2];
                sum += src[col + 3 * cStride] * c[3];
            }
            if (N >= 6)
            {
                sum += src[col + 4 * cStride] * c[4];
                sum += src[col + 5 * cStride] * c[5];
            }
            if (N == 8)
            {
                sum += src[col + 6 * cStride] * c[6];
                sum += src[col + 7 * cStride] * c[7];
            }

            short val = (sum + offset) >> shift;

            val = (val + offsetPost) >> headRoom;
            if (val < 0) val = 0;
            if (val > maxVal) val = maxVal;
            dst[col] = val;
        }

        src += srcStride;
        dst += dstStride;
    }
}

template<int N>
void filterHorizontal_pel_short(int bitDepth, Pel *src, int srcStride, short *dst, int dstStride, int width, int height, short const *coeff)
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

    int cStride = 1;
    src -= (N / 2 - 1) * cStride;

    int offset;
    short maxVal;
    int headRoom = IF_INTERNAL_PREC - bitDepth;
    int shift = IF_FILTER_PREC;

    shift -= headRoom;
    offset = -IF_INTERNAL_OFFS << shift;
    maxVal = 0;

    int row, col;
    for (row = 0; row < height; row++)
    {
        for (col = 0; col < width; col++)
        {
            int sum;

            sum  = src[col + 0 * cStride] * c[0];
            sum += src[col + 1 * cStride] * c[1];
            if (N >= 4)
            {
                sum += src[col + 2 * cStride] * c[2];
                sum += src[col + 3 * cStride] * c[3];
            }
            if (N >= 6)
            {
                sum += src[col + 4 * cStride] * c[4];
                sum += src[col + 5 * cStride] * c[5];
            }
            if (N == 8)
            {
                sum += src[col + 6 * cStride] * c[6];
                sum += src[col + 7 * cStride] * c[7];
            }

            short val = (sum + offset) >> shift;
            dst[col] = val;
        }

        src += srcStride;
        dst += dstStride;
    }
}

void filterCopy(Pel *src, int srcStride, Pel *dst, int dstStride, int width, int height)
{
    int row, col;

    for (row = 0; row < height; row++)
    {
        for (col = 0; col < width; col++)
        {
            dst[col] = src[col];
        }

        src += srcStride;
        dst += dstStride;
    }
}

void filterConvert(int bitDepth, short *src, int srcStride, Pel *dst, int dstStride, int width, int height)
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
            dst[col] = val;
        }

        src += srcStride;
        dst += dstStride;
    }
}

void filterConvertPS(int bitDepth, Pel *src, int srcStride, short *dst, int dstStride, int width, int height)
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

#endif // ifndef X265_INTERPOLATIONFILTER_H
