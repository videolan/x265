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

#include "TLibCommon/TypeDef.h"

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

#define NTAPS_LUMA        8 ///< Number of taps for luma
#define NTAPS_CHROMA      4 ///< Number of taps for chroma
#define IF_INTERNAL_PREC 14 ///< Number of bits for internal precision
#define IF_FILTER_PREC    6 ///< Log2 of sum of filter taps
#define IF_INTERNAL_OFFS (1 << (IF_INTERNAL_PREC - 1)) ///< Offset used internally

template<int N>
void filterVertical_pel_pel(int bitDepth, Pel *src, int srcStride, Pel *dst, int dstStride, int width, int height, short const *coeff);
template<int N>
void filterVertical_short_pel(int bitDepth, short *src, int srcStride, Pel *dst, int dstStride, int width, int height, short const *coeff);

template<int N>
void filterHorizontal_pel_pel(int bitDepth, Pel *src, int srcStride, Pel *dst, int dstStride, int width, int height, short const *coeff);
template<int N>
void filterHorizontal_pel_short(int bitDepth, Pel *src, int srcStride, short *dst, int dstStride, int width, int height, short const *coeff);

void filterCopy(Pel *src, int srcStride, Pel *dst, int dstStride, int width, int height);
void filterConvertShortToPel(int bitDepth, short *src, int srcStride, Pel *dst, int dstStride, int width, int height);
void filterConvertPelToShort(int bitDepth, Pel *src, int srcStride, short *dst, int dstStride, int width, int height);

#endif // ifndef X265_INTERPOLATIONFILTER_H
