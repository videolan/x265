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

#define INSTRSET 4
#include "vectorclass.h"

using namespace x265;

namespace {
#if !HIGH_BIT_DEPTH 
void scale1D_128to64(pixel *dst, pixel *src, intptr_t /*stride*/)
{
    const __m128i mask = _mm_setr_epi32(0x06040200, 0x0E0C0A08, 0x07050301, 0x0F0D0B09);

    __m128i T00 = _mm_shuffle_epi8(_mm_loadu_si128((__m128i*)&src[0 * 16]), mask);
    __m128i T01 = _mm_shuffle_epi8(_mm_loadu_si128((__m128i*)&src[1 * 16]), mask);
    __m128i T02 = _mm_shuffle_epi8(_mm_loadu_si128((__m128i*)&src[2 * 16]), mask);
    __m128i T03 = _mm_shuffle_epi8(_mm_loadu_si128((__m128i*)&src[3 * 16]), mask);
    __m128i T04 = _mm_shuffle_epi8(_mm_loadu_si128((__m128i*)&src[4 * 16]), mask);
    __m128i T05 = _mm_shuffle_epi8(_mm_loadu_si128((__m128i*)&src[5 * 16]), mask);
    __m128i T06 = _mm_shuffle_epi8(_mm_loadu_si128((__m128i*)&src[6 * 16]), mask);
    __m128i T07 = _mm_shuffle_epi8(_mm_loadu_si128((__m128i*)&src[7 * 16]), mask);

    __m128i T10 = _mm_unpacklo_epi64(T00, T01);
    __m128i T11 = _mm_unpackhi_epi64(T00, T01);
    __m128i T12 = _mm_unpacklo_epi64(T02, T03);
    __m128i T13 = _mm_unpackhi_epi64(T02, T03);
    __m128i T14 = _mm_unpacklo_epi64(T04, T05);
    __m128i T15 = _mm_unpackhi_epi64(T04, T05);
    __m128i T16 = _mm_unpacklo_epi64(T06, T07);
    __m128i T17 = _mm_unpackhi_epi64(T06, T07);

    __m128i T20 = _mm_avg_epu8(T10, T11);
    __m128i T21 = _mm_avg_epu8(T12, T13);
    __m128i T22 = _mm_avg_epu8(T14, T15);
    __m128i T23 = _mm_avg_epu8(T16, T17);

    _mm_storeu_si128((__m128i*)&dst[0], T20);
    _mm_storeu_si128((__m128i*)&dst[16], T21);
    _mm_storeu_si128((__m128i*)&dst[32], T22);
    _mm_storeu_si128((__m128i*)&dst[48], T23);
}

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
#endif
}

namespace x265 {
void Setup_Vec_PixelPrimitives_ssse3(EncoderPrimitives &p)
{
    p.sad[0] = p.sad[0];
#if !HIGH_BIT_DEPTH 
    p.scale1D_128to64 = scale1D_128to64;
    p.scale2D_64to32 = scale2D_64to32;
#endif
}
}
