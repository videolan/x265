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
 * For more information, contact us at licensing@multicorewareinc.com
 *****************************************************************************/

#include "primitives.h"
#include <xmmintrin.h> // SSE
#include <pmmintrin.h> // SSE3
#include <tmmintrin.h> // SSSE3

using namespace x265;

namespace {
void convert16to32_shl(int32_t *dst, int16_t *org, intptr_t stride, int shift, int size)
{
    int i, j;

    for (i = 0; i < size; i++)
    {
        for (j = 0; j < size; j += 4)
        {
            __m128i im16;
            __m128i im32;

            im16 = _mm_loadl_epi64((__m128i*)&org[i * stride + j]);
            im32 = _mm_srai_epi32(_mm_unpacklo_epi16(im16, im16), 16);
            im32 = _mm_slli_epi32(im32, shift);
            _mm_storeu_si128((__m128i*)dst, im32);

            dst += 4;
        }
    }
}

#if !HIGH_BIT_DEPTH
void scale2D_64to32(pixel *dst, pixel *src, intptr_t stride)
{
    int i;
    const __m128i c8_1 = _mm_set1_epi32(0x01010101);
    const __m128i c16_2 = _mm_set1_epi32(0x00020002);

    for (i = 0; i < 64; i += 2)
    {
        __m128i T00 = _mm_loadu_si128((__m128i*)&src[(i + 0) *  stride +  0]);
        __m128i T01 = _mm_loadu_si128((__m128i*)&src[(i + 0) *  stride + 16]);
        __m128i T02 = _mm_loadu_si128((__m128i*)&src[(i + 0) *  stride + 32]);
        __m128i T03 = _mm_loadu_si128((__m128i*)&src[(i + 0) *  stride + 48]);
        __m128i T10 = _mm_loadu_si128((__m128i*)&src[(i + 1) *  stride +  0]);
        __m128i T11 = _mm_loadu_si128((__m128i*)&src[(i + 1) *  stride + 16]);
        __m128i T12 = _mm_loadu_si128((__m128i*)&src[(i + 1) *  stride + 32]);
        __m128i T13 = _mm_loadu_si128((__m128i*)&src[(i + 1) *  stride + 48]);

        __m128i S00 = _mm_maddubs_epi16(T00, c8_1);
        __m128i S01 = _mm_maddubs_epi16(T01, c8_1);
        __m128i S02 = _mm_maddubs_epi16(T02, c8_1);
        __m128i S03 = _mm_maddubs_epi16(T03, c8_1);
        __m128i S10 = _mm_maddubs_epi16(T10, c8_1);
        __m128i S11 = _mm_maddubs_epi16(T11, c8_1);
        __m128i S12 = _mm_maddubs_epi16(T12, c8_1);
        __m128i S13 = _mm_maddubs_epi16(T13, c8_1);

        __m128i S20 = _mm_srli_epi16(_mm_add_epi16(_mm_add_epi16(S00, S10), c16_2), 2);
        __m128i S21 = _mm_srli_epi16(_mm_add_epi16(_mm_add_epi16(S01, S11), c16_2), 2);
        __m128i S22 = _mm_srli_epi16(_mm_add_epi16(_mm_add_epi16(S02, S12), c16_2), 2);
        __m128i S23 = _mm_srli_epi16(_mm_add_epi16(_mm_add_epi16(S03, S13), c16_2), 2);

        _mm_storeu_si128((__m128i*)&dst[(i >> 1) * 32 +  0], _mm_packus_epi16(S20, S21));
        _mm_storeu_si128((__m128i*)&dst[(i >> 1) * 32 + 16], _mm_packus_epi16(S22, S23));
    }
}
#endif // if !HIGH_BIT_DEPTH
}

namespace x265 {
void Setup_Vec_PixelPrimitives_ssse3(EncoderPrimitives &p)
{
    p.cvt16to32_shl = convert16to32_shl;

#if !HIGH_BIT_DEPTH
    p.scale2D_64to32 = scale2D_64to32;
#endif
}
}
