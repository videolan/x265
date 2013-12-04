/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Deepthi Devaki <deepthidevaki@multicorewareinc.com>,
 *          Rajesh Paulraj <rajesh@multicorewareinc.com>
 *          Mandar Gurav <mandar@multicorewareinc.com>
 *          Mahesh Pittala <mahesh@multicorewareinc.com>
 *          Praveen Kumar Tiwari <praveen@multicorewareinc.com>
 *          Nabajit Deka <nabajit@multicorewareinc.com>
 *          Min Chen <chenm003@163.com>
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
#include "TLibCommon/TComRom.h"
#include <xmmintrin.h> // SSE
#include <smmintrin.h> // SSE4.1
#include <assert.h>
#include <string.h>

using namespace x265;

#if !HIGH_BIT_DEPTH
namespace {
template<int N>
void filterVertical_sp(int16_t *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int width, int height, int coeffIdx)
{
    const int16_t *coeff = (N == 8 ? g_lumaFilter[coeffIdx] : g_chromaFilter[coeffIdx]);

    src -= (N / 2 - 1) * srcStride;

    int offset;
    int headRoom = IF_INTERNAL_PREC - X265_DEPTH;
    int shift = IF_FILTER_PREC;
    shift += headRoom;
    offset = 1 << (shift - 1);
    offset +=  IF_INTERNAL_OFFS << IF_FILTER_PREC;

    __m128i coeffTemp = _mm_loadu_si128((__m128i const*)coeff);
    __m128i coeffTempLow = _mm_cvtepi16_epi32(coeffTemp);
    coeffTemp = _mm_srli_si128(coeffTemp, 8);
    __m128i coeffTempHigh = _mm_cvtepi16_epi32(coeffTemp);

    __m128i filterCoeff0 = _mm_shuffle_epi32(coeffTempLow, 0x00);
    __m128i filterCoeff1 = _mm_shuffle_epi32(coeffTempLow, 0x55);
    __m128i filterCoeff2 = _mm_shuffle_epi32(coeffTempLow, 0xAA);
    __m128i filterCoeff3 = _mm_shuffle_epi32(coeffTempLow, 0xFF);

    __m128i filterCoeff4 = _mm_shuffle_epi32(coeffTempHigh, 0x00);
    __m128i filterCoeff5 = _mm_shuffle_epi32(coeffTempHigh, 0x55);
    __m128i filterCoeff6 = _mm_shuffle_epi32(coeffTempHigh, 0xAA);
    __m128i filterCoeff7 = _mm_shuffle_epi32(coeffTempHigh, 0xFF);

    __m128i mask4 = _mm_cvtsi32_si128(0xFFFFFFFF);

    int row, col;

    for (row = 0; row < height; row++)
    {
        col = 0;
        for (; col < (width - 7); col += 8)
        {
            __m128i srcCoeff = _mm_loadu_si128((__m128i const*)(&src[col]));
            __m128i srcCoeffTemp1 = _mm_cvtepi16_epi32(srcCoeff);
            __m128i T00 = _mm_mullo_epi32(srcCoeffTemp1, filterCoeff0);
            srcCoeff = _mm_srli_si128(srcCoeff, 8);
            srcCoeff = _mm_cvtepi16_epi32(srcCoeff);
            __m128i T01 = _mm_mullo_epi32(srcCoeff, filterCoeff0);

            srcCoeff = _mm_loadu_si128((__m128i const*)(&src[col + srcStride]));
            __m128i srcCoeffTemp2 = _mm_cvtepi16_epi32(srcCoeff);
            __m128i T10 = _mm_mullo_epi32(srcCoeffTemp2, filterCoeff1);
            srcCoeff = _mm_srli_si128(srcCoeff, 8);
            srcCoeff = _mm_cvtepi16_epi32(srcCoeff);
            __m128i T11 = _mm_mullo_epi32(srcCoeff, filterCoeff1);

            srcCoeff = _mm_loadu_si128((__m128i const*)(&src[col + 2 * srcStride]));
            __m128i srcCoeffTemp3 = _mm_cvtepi16_epi32(srcCoeff);
            __m128i T20 = _mm_mullo_epi32(srcCoeffTemp3, filterCoeff2);
            srcCoeff = _mm_srli_si128(srcCoeff, 8);
            srcCoeff = _mm_cvtepi16_epi32(srcCoeff);
            __m128i T21 = _mm_mullo_epi32(srcCoeff, filterCoeff2);

            srcCoeff = _mm_loadu_si128((__m128i const*)(&src[col + 3 * srcStride]));
            __m128i srcCoeffTemp4 = _mm_cvtepi16_epi32(srcCoeff);
            __m128i T30 = _mm_mullo_epi32(srcCoeffTemp4, filterCoeff3);
            srcCoeff = _mm_srli_si128(srcCoeff, 8);
            srcCoeff = _mm_cvtepi16_epi32(srcCoeff);
            __m128i T31 = _mm_mullo_epi32(srcCoeff, filterCoeff3);

            __m128i sum01 = _mm_add_epi32(T00, T10);
            __m128i sum23 = _mm_add_epi32(T20, T30);
            __m128i sumlo0123 = _mm_add_epi32(sum01, sum23);

            __m128i sum45 = _mm_add_epi32(T01, T11);
            __m128i sum67 = _mm_add_epi32(T21, T31);
            __m128i sumhi0123 = _mm_add_epi32(sum45, sum67);

            if (N == 8)
            {
                srcCoeff = _mm_loadu_si128((__m128i const*)(&src[col + 4 * srcStride]));
                srcCoeffTemp1 = _mm_cvtepi16_epi32(srcCoeff);
                T00 = _mm_mullo_epi32(srcCoeffTemp1, filterCoeff4);
                srcCoeff = _mm_srli_si128(srcCoeff, 8);
                srcCoeff = _mm_cvtepi16_epi32(srcCoeff);
                T01 = _mm_mullo_epi32(srcCoeff, filterCoeff4);

                srcCoeff = _mm_loadu_si128((__m128i const*)(&src[col + 5 * srcStride]));
                srcCoeffTemp2 = _mm_cvtepi16_epi32(srcCoeff);
                T10 = _mm_mullo_epi32(srcCoeffTemp2, filterCoeff5);
                srcCoeff = _mm_srli_si128(srcCoeff, 8);
                srcCoeff = _mm_cvtepi16_epi32(srcCoeff);
                T11 = _mm_mullo_epi32(srcCoeff, filterCoeff5);

                srcCoeff = _mm_loadu_si128((__m128i const*)(&src[col + 6 * srcStride]));
                srcCoeffTemp3 = _mm_cvtepi16_epi32(srcCoeff);
                T20 = _mm_mullo_epi32(srcCoeffTemp3, filterCoeff6);
                srcCoeff = _mm_srli_si128(srcCoeff, 8);
                srcCoeff = _mm_cvtepi16_epi32(srcCoeff);
                T21 = _mm_mullo_epi32(srcCoeff, filterCoeff6);

                srcCoeff = _mm_loadu_si128((__m128i const*)(&src[col + 7 * srcStride]));
                srcCoeffTemp4 = _mm_cvtepi16_epi32(srcCoeff);
                T30 = _mm_mullo_epi32(srcCoeffTemp4, filterCoeff7);
                srcCoeff = _mm_srli_si128(srcCoeff, 8);
                srcCoeff = _mm_cvtepi16_epi32(srcCoeff);
                T31 = _mm_mullo_epi32(srcCoeff, filterCoeff7);

                sum01 = _mm_add_epi32(T00, T10);
                sum23 = _mm_add_epi32(T20, T30);
                sumlo0123 = _mm_add_epi32(sumlo0123, _mm_add_epi32(sum01, sum23));

                sum45 = _mm_add_epi32(T01, T11);
                sum67 = _mm_add_epi32(T21, T31);
                sumhi0123 = _mm_add_epi32(sumhi0123, _mm_add_epi32(sum45, sum67));
            }
            __m128i sumOffset = _mm_set1_epi32(offset);

            __m128i val1 = _mm_add_epi32(sumlo0123, sumOffset);
            val1 = _mm_srai_epi32(val1, shift);

            __m128i val2 = _mm_add_epi32(sumhi0123, sumOffset);
            val2 = _mm_srai_epi32(val2, shift);

            __m128i val = _mm_packs_epi32(val1, val2);
            __m128i res = _mm_packus_epi16(val, val);
            _mm_storel_epi64((__m128i*)&dst[col], res);
        }

        for (; col < width; col += 4)
        {
            __m128i srcCoeff = _mm_loadl_epi64((__m128i const*)(&src[col]));
            __m128i srcCoeffTemp1 = _mm_cvtepi16_epi32(srcCoeff);
            __m128i T00 = _mm_mullo_epi32(srcCoeffTemp1, filterCoeff0);

            srcCoeff = _mm_loadl_epi64((__m128i const*)(&src[col + srcStride]));
            __m128i srcCoeffTemp2 = _mm_cvtepi16_epi32(srcCoeff);
            __m128i T10 = _mm_mullo_epi32(srcCoeffTemp2, filterCoeff1);

            srcCoeff = _mm_loadl_epi64((__m128i const*)(&src[col + 2 * srcStride]));
            __m128i srcCoeffTemp3 = _mm_cvtepi16_epi32(srcCoeff);
            __m128i T20 = _mm_mullo_epi32(srcCoeffTemp3, filterCoeff2);

            srcCoeff = _mm_loadl_epi64((__m128i const*)(&src[col + 3 * srcStride]));
            __m128i srcCoeffTemp4 = _mm_cvtepi16_epi32(srcCoeff);
            __m128i T30 = _mm_mullo_epi32(srcCoeffTemp4, filterCoeff3);

            __m128i sum01 = _mm_add_epi32(T00, T10);
            __m128i sum23 = _mm_add_epi32(T20, T30);
            __m128i sumlo0123 = _mm_add_epi32(sum01, sum23);

            if (N == 8)
            {
                srcCoeff = _mm_loadl_epi64((__m128i const*)(&src[col + 4 * srcStride]));
                srcCoeffTemp1 = _mm_cvtepi16_epi32(srcCoeff);
                T00 = _mm_mullo_epi32(srcCoeffTemp1, filterCoeff4);

                srcCoeff = _mm_loadl_epi64((__m128i const*)(&src[col + 5 * srcStride]));
                srcCoeffTemp2 = _mm_cvtepi16_epi32(srcCoeff);
                T10 = _mm_mullo_epi32(srcCoeffTemp2, filterCoeff5);

                srcCoeff = _mm_loadl_epi64((__m128i const*)(&src[col + 6 * srcStride]));
                srcCoeffTemp3 = _mm_cvtepi16_epi32(srcCoeff);
                T20 = _mm_mullo_epi32(srcCoeffTemp3, filterCoeff6);

                srcCoeff = _mm_loadl_epi64((__m128i const*)(&src[col + 7 * srcStride]));
                srcCoeffTemp4 = _mm_cvtepi16_epi32(srcCoeff);
                T30 = _mm_mullo_epi32(srcCoeffTemp4, filterCoeff7);

                sum01 = _mm_add_epi32(T00, T10);
                sum23 = _mm_add_epi32(T20, T30);
                sumlo0123 = _mm_add_epi32(sumlo0123, _mm_add_epi32(sum01, sum23));
            }

            __m128i zero16 = _mm_set1_epi16(0);
            __m128i zero32 = _mm_set1_epi32(0);
            __m128i sumOffset = _mm_set1_epi32(offset);

            __m128i val1 = _mm_add_epi32(sumlo0123, sumOffset);
            val1 = _mm_srai_epi32(val1, shift);

            __m128i val = _mm_packs_epi32(val1, zero32);
            __m128i res = _mm_packus_epi16(val, zero16);

            int n = width - col;
            __m128i mask1, mask2, mask3;

            switch (n)   // store either 1, 2, 3 or 4 8-bit results in dst
            {
            case 1: mask1 = _mm_srli_si128(mask4, 3);
                _mm_maskmoveu_si128(res, mask1, (char*)&dst[col]);
                break;

            case 2:  mask2 = _mm_srli_si128(mask4, 2);
                _mm_maskmoveu_si128(res, mask2, (char*)&dst[col]);
                break;

            case 3: mask3 = _mm_srli_si128(mask4, 1);
                _mm_maskmoveu_si128(res, mask3, (char*)&dst[col]);
                break;

            default: _mm_maskmoveu_si128(res, mask4, (char*)&dst[col]);
                break;
            }
        }

        src += srcStride;
        dst += dstStride;
    }
}
}
#endif

namespace x265 {
void Setup_Vec_IPFilterPrimitives_sse41(EncoderPrimitives& p)
{
#if !HIGH_BIT_DEPTH
    p.chroma_vsp = filterVertical_sp<4>;
#endif
}
}
