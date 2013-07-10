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

#include "primitives.h"
#include <cstring>
#include <assert.h>
#include "TLibCommon/TComPrediction.h"

#if _MSC_VER
#pragma warning(disable: 4127) // conditional expression is constant, typical for templated functions
#pragma warning(disable: 4100) // unreferenced formal parameter
#endif

namespace {
template<int N>
void filterVertical_s_p(int bitDepth, short *src, int srcStride, pixel *dst, int dstStride, int width, int height, short const *coeff)
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

            dst[col] = (pixel)val;
        }

        src += srcStride;
        dst += dstStride;
    }
}

template<int N>
void filterHorizontal_p_p(int bitDepth, pixel *src, int srcStride, pixel *dst, int dstStride, int width, int height, short const *coeff)
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

            short val = (short)(sum + offset) >> headRoom;

            if (val < 0) val = 0;
            if (val > maxVal) val = maxVal;
            dst[col] = (pixel)val;
        }

        src += srcStride;
        dst += dstStride;
    }
}

template<int N>
void filterVertical_s_s(int /* bitDepth */, short *src, int srcStride, short *dst, int dstStride, int width, int height, short const *coeff)
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

    int cStride =  srcStride;
    src -= (N / 2 - 1) * cStride;
    int shift = IF_FILTER_PREC;
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

            short val = (short)((sum) >> shift);
            dst[col] = val;
        }

        src += srcStride;
        dst += dstStride;
    }
}

template<int N>
void filterVertical_p_s(int bitDepth, pixel *src, int srcStride, short *dst, int dstStride, int width, int height, short const *coeff)
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

    int Stride =  srcStride;
    src -= (N / 2 - 1) * Stride;
    Int offset;
    Int headRoom = IF_INTERNAL_PREC - bitDepth;
    Int shift = IF_FILTER_PREC;

    shift -=  headRoom;
    offset = -IF_INTERNAL_OFFS << shift;

    int row, col;
    for (row = 0; row < height; row++)
    {
        for (col = 0; col < width; col++)
        {
            int sum;

            sum  = src[col + 0 * Stride] * c[0];
            sum += src[col + 1 * Stride] * c[1];
            if (N >= 4)
            {
                sum += src[col + 2 * Stride] * c[2];
                sum += src[col + 3 * Stride] * c[3];
            }
            if (N >= 6)
            {
                sum += src[col + 4 * Stride] * c[4];
                sum += src[col + 5 * Stride] * c[5];
            }
            if (N == 8)
            {
                sum += src[col + 6 * Stride] * c[6];
                sum += src[col + 7 * Stride] * c[7];
            }

            short val = (short)((sum + offset) >> shift);
            dst[col] = val;
        }

        src += srcStride;
        dst += dstStride;
    }
}

template<int N>
void filterHorizontal_p_s(int bitDepth, pixel *src, int srcStride, short *dst, int dstStride, int width, int height, short const *coeff)
{
    int Stride = 1;

    src -= (N / 2 - 1) * Stride;

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

            sum  = src[col + 0 * Stride] * coeff[0];
            sum += src[col + 1 * Stride] * coeff[1];
            if (N >= 4)
            {
                sum += src[col + 2 * Stride] * coeff[2];
                sum += src[col + 3 * Stride] * coeff[3];
            }
            if (N >= 6)
            {
                sum += src[col + 4 * Stride] * coeff[4];
                sum += src[col + 5 * Stride] * coeff[5];
            }
            if (N == 8)
            {
                sum += src[col + 6 * Stride] * coeff[6];
                sum += src[col + 7 * Stride] * coeff[7];
            }

            short val = (short)(sum + offset) >> shift;
            dst[col] = val;
        }

        src += srcStride;
        dst += dstStride;
    }
}

void filterConvertShortToPel(int bitDepth, short *src, int srcStride, pixel *dst, int dstStride, int width, int height)
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
            dst[col] = (pixel)val;
        }

        src += srcStride;
        dst += dstStride;
    }
}

void filterConvertPelToShort(int bitDepth, pixel *src, int srcStride, short *dst, int dstStride, int width, int height)
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
void filterVertical_p_p(int bitDepth, pixel *src, int srcStride, pixel *dst, int dstStride, int width, int height, short const *coeff)
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

    int Stride = srcStride;
    src -= (N / 2 - 1) * Stride;

    int offset;
    short maxVal;
    //int headRoom = IF_INTERNAL_PREC - bitDepth;
    int shift = IF_FILTER_PREC;

    //shift += headRoom;
    offset = 1 << (shift - 1);
    //offset += IF_INTERNAL_OFFS << IF_FILTER_PREC;
    maxVal = (1 << bitDepth) - 1;

    int row, col;

    for (row = 0; row < height; row++)
    {
        for (col = 0; col < width; col++)
        {
            int sum;

            sum  = src[col + 0 * Stride] * c[0];
            sum += src[col + 1 * Stride] * c[1];
            if (N >= 4)
            {
                sum += src[col + 2 * Stride] * c[2];
                sum += src[col + 3 * Stride] * c[3];
            }
            if (N >= 6)
            {
                sum += src[col + 4 * Stride] * c[4];
                sum += src[col + 5 * Stride] * c[5];
            }
            if (N == 8)
            {
                sum += src[col + 6 * Stride] * c[6];
                sum += src[col + 7 * Stride] * c[7];
            }

            short val = (short)((sum + offset) >> shift);
            val = (val < 0) ? 0 : val;
            val = (val > maxVal) ? maxVal : val;

            dst[col] = (pixel)val;
        }

        src += srcStride;
        dst += dstStride;
    }
}

void filterVertical_short_pel_multiplane(int bitDepth, short *src, int srcStride, pixel *dstE, pixel *dstI, pixel *dstP, int dstStride, int block_width, int block_height)
{
    filterVertical_s_p<8>(bitDepth, src, srcStride, dstI, dstStride, block_width, block_height, TComPrediction::m_lumaFilter[2]);
    filterVertical_s_p<8>(bitDepth, src, srcStride, dstE, dstStride, block_width, block_height, TComPrediction::m_lumaFilter[1]);
    filterVertical_s_p<8>(bitDepth, src, srcStride, dstP, dstStride, block_width, block_height, TComPrediction::m_lumaFilter[3]);
}

void extendPicCompBorder(pixel* piTxt, int stride, int width, int height, int marginX, int marginY)
{
    int   x, y;
    pixel*  pi;

    pi = piTxt;
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < marginX; x++)
        {
            pi[-marginX + x] = pi[0];
            pi[width + x] = pi[width - 1];
        }

        pi += stride;
    }

    pi -= (stride + marginX);
    for (y = 0; y < marginY; y++)
    {
        ::memcpy(pi + (y + 1) * stride, pi, sizeof(pixel) * (width + (marginX << 1)));
    }

    pi -= ((height - 1) * stride);
    for (y = 0; y < marginY; y++)
    {
        ::memcpy(pi - (y + 1) * stride, pi, sizeof(pixel) * (width + (marginX << 1)));
    }
}

void filterVerticalMultiplaneExtend(int bitDepth, short *src, int srcStride, pixel *dstE, pixel *dstI, pixel *dstP, int dstStride, int block_width, int block_height, int marginX, int marginY)
{
    filterVertical_s_p<8>(bitDepth, src, srcStride, dstI, dstStride, block_width, block_height, TComPrediction::m_lumaFilter[2]);
    filterVertical_s_p<8>(bitDepth, src, srcStride, dstE, dstStride, block_width, block_height, TComPrediction::m_lumaFilter[1]);
    filterVertical_s_p<8>(bitDepth, src, srcStride, dstP, dstStride, block_width, block_height, TComPrediction::m_lumaFilter[3]);
    extendPicCompBorder(dstE, dstStride, block_width, block_height, marginX, marginY);
    extendPicCompBorder(dstI, dstStride, block_width, block_height, marginX, marginY);
    extendPicCompBorder(dstP, dstStride, block_width, block_height, marginX, marginY);
}

void filterHorizontalMultiplaneExtend(int bitDepth, pixel *src, int srcStride, short *midF, short* midA, short* midB, short* midC, int midStride, pixel *pDstA, pixel *pDstB, pixel *pDstC, int pDstStride, int block_width, int block_height, int marginX, int marginY)
{
    filterConvertPelToShort(bitDepth, src, srcStride, midF, midStride, block_width, block_height);
    filterHorizontal_p_s<8>(bitDepth, src, srcStride, midB, midStride, block_width, block_height, TComPrediction::m_lumaFilter[2]);
    filterHorizontal_p_s<8>(bitDepth, src, srcStride, midA, midStride, block_width, block_height, TComPrediction::m_lumaFilter[1]);
    filterHorizontal_p_s<8>(bitDepth, src, srcStride, midC, midStride, block_width, block_height, TComPrediction::m_lumaFilter[3]);
    filterConvertShortToPel(bitDepth, midA, midStride, pDstA, pDstStride, block_width, block_height);
    filterConvertShortToPel(bitDepth, midB, midStride, pDstB, pDstStride, block_width, block_height);
    filterConvertShortToPel(bitDepth, midC, midStride, pDstC, pDstStride, block_width, block_height);

    extendPicCompBorder(pDstA, pDstStride, block_width, block_height, marginX, marginY);
    extendPicCompBorder(pDstB, pDstStride, block_width, block_height, marginX, marginY);
    extendPicCompBorder(pDstC, pDstStride, block_width, block_height, marginX, marginY);
}
}

#if _MSC_VER
#pragma warning(default: 4127) // conditional expression is constant, typical for templated functions
#pragma warning(default: 4100)
#endif

namespace x265 {
// x265 private namespace

void Setup_C_IPFilterPrimitives(EncoderPrimitives& p)
{
    p.ipfilter_pp[FILTER_H_P_P_8] = filterHorizontal_p_p<8>;
    p.ipfilter_ps[FILTER_H_P_S_8] = filterHorizontal_p_s<8>;
    p.ipfilter_ps[FILTER_V_P_S_8] = filterVertical_p_s<8>;
    p.ipfilter_sp[FILTER_V_S_P_8] = filterVertical_s_p<8>;
    p.ipfilter_pp[FILTER_H_P_P_4] = filterHorizontal_p_p<4>;
    p.ipfilter_ps[FILTER_H_P_S_4] = filterHorizontal_p_s<4>;
    p.ipfilter_ps[FILTER_V_P_S_4] = filterVertical_p_s<4>;
    p.ipfilter_sp[FILTER_V_S_P_4] = filterVertical_s_p<4>;
    p.ipfilter_pp[FILTER_V_P_P_8] = filterVertical_p_p<8>;
    p.ipfilter_pp[FILTER_V_P_P_4] = filterVertical_p_p<4>;
    p.ipfilter_ss[FILTER_V_S_S_8] = filterVertical_s_s<8>;
    p.ipfilter_ss[FILTER_V_S_S_4] = filterVertical_s_s<4>;

    p.ipfilter_p2s = filterConvertPelToShort;
    p.ipfilter_s2p = filterConvertShortToPel;

    p.filterVmulti = filterVerticalMultiplaneExtend;
    p.filterHmulti = filterHorizontalMultiplaneExtend;
}
}
