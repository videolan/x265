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

/* this file instantiates SSE3 versions of the pixel primitives */

#include "primitives.h"
#include <assert.h>
#include <xmmintrin.h> // SSE
#include <pmmintrin.h> // SSE3

using namespace x265;

void convert16to32_shl(int *dst, short *org, intptr_t stride, int shift, int size)
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

void convert16to16_shl(short *dst, short *org, int width, int height, intptr_t stride, int shift)
{
    int i, j;

    if (width == 4)
    {
        for (i = 0; i < height; i += 2)
        {
            __m128i T00, T01;

            T00 = _mm_loadl_epi64((__m128i*)&org[(i) * stride]);
            T01 = _mm_loadl_epi64((__m128i*)&org[(i + 1) * stride]);
            T00 = _mm_unpacklo_epi64(T00, T01);
            T00 = _mm_slli_epi16(T00, shift);
            _mm_storeu_si128((__m128i*)&dst[i * 4], T00);
        }
    }
    else
    {
        for (i = 0; i < height; i++)
        {
            for (j = 0; j < width; j += 8)
            {
                __m128i T00;

                T00 = _mm_loadu_si128((__m128i*)&org[i * stride + j]);
                T00 = _mm_slli_epi16(T00, shift);
                _mm_storeu_si128((__m128i*)&dst[i * width + j], T00);
            }
        }
    }
}

#if !HIGH_BIT_DEPTH
void transpose4(pixel* dst, pixel* src, intptr_t stride)
{
    __m128i T00, T01, T02, T03;

    T00 = _mm_cvtsi32_si128(*(int*)&src[0 * stride]);   // [03 02 01 00]
    T01 = _mm_cvtsi32_si128(*(int*)&src[1 * stride]);   // [13 12 11 10]
    T02 = _mm_cvtsi32_si128(*(int*)&src[2 * stride]);   // [23 22 21 20]
    T03 = _mm_cvtsi32_si128(*(int*)&src[3 * stride]);   // [33 32 31 30]

    T00 = _mm_unpacklo_epi8(T00, T01);
    T01 = _mm_unpacklo_epi8(T02, T03);

    T00 = _mm_unpacklo_epi16(T00, T01);

    _mm_store_si128((__m128i*)dst, T00);
}

#define TRANSPOSE_8X8(in0, in1, in2, in3, in4, in5, in6, in7, out0, out1, out2, out3) \
    {                                                      \
        const __m128i tr0_0 = _mm_unpacklo_epi8(in0, in1); \
        const __m128i tr0_1 = _mm_unpacklo_epi8(in2, in3); \
        const __m128i tr0_2 = _mm_unpacklo_epi8(in4, in5); \
        const __m128i tr0_3 = _mm_unpacklo_epi8(in6, in7); \
        const __m128i tr1_0 = _mm_unpacklo_epi16(tr0_0, tr0_1); \
        const __m128i tr1_1 = _mm_unpackhi_epi16(tr0_0, tr0_1); \
        const __m128i tr1_2 = _mm_unpacklo_epi16(tr0_2, tr0_3); \
        const __m128i tr1_3 = _mm_unpackhi_epi16(tr0_2, tr0_3); \
        out0 = _mm_unpacklo_epi32(tr1_0, tr1_2); \
        out1 = _mm_unpackhi_epi32(tr1_0, tr1_2); \
        out2 = _mm_unpacklo_epi32(tr1_1, tr1_3); \
        out3 = _mm_unpackhi_epi32(tr1_1, tr1_3); \
    }

void transpose8(pixel* dst, pixel* src, intptr_t stride)
{
    __m128i T00, T01, T02, T03, T04, T05, T06, T07;

    T00 = _mm_loadl_epi64((__m128i*)&src[0 * stride]);   // [07 06 05 04 03 02 01 00]
    T01 = _mm_loadl_epi64((__m128i*)&src[1 * stride]);   // [17 16 15 14 13 12 11 10]
    T02 = _mm_loadl_epi64((__m128i*)&src[2 * stride]);   // [27 26 25 24 23 22 21 20]
    T03 = _mm_loadl_epi64((__m128i*)&src[3 * stride]);   // [37 36 35 34 33 32 31 30]
    T04 = _mm_loadl_epi64((__m128i*)&src[4 * stride]);   // [47 46 45 44 43 42 41 40]
    T05 = _mm_loadl_epi64((__m128i*)&src[5 * stride]);   // [57 56 55 54 53 52 51 50]
    T06 = _mm_loadl_epi64((__m128i*)&src[6 * stride]);   // [67 66 65 64 63 62 61 60]
    T07 = _mm_loadl_epi64((__m128i*)&src[7 * stride]);   // [77 76 75 74 73 72 71 70]

    TRANSPOSE_8X8(T00, T01, T02, T03, T04, T05, T06, T07, T00, T01, T02, T03);

    _mm_store_si128((__m128i*)&dst[0 * 8], T00);
    _mm_store_si128((__m128i*)&dst[2 * 8], T01);
    _mm_store_si128((__m128i*)&dst[4 * 8], T02);
    _mm_store_si128((__m128i*)&dst[6 * 8], T03);
}

inline void transpose16_dummy(pixel* dst, intptr_t dststride, pixel* src, intptr_t srcstride)
{
    __m128i T00, T01, T02, T03, T04, T05, T06, T07;

    T00 = _mm_loadl_epi64((__m128i*)&src[0 * srcstride]);
    T01 = _mm_loadl_epi64((__m128i*)&src[1 * srcstride]);
    T02 = _mm_loadl_epi64((__m128i*)&src[2 * srcstride]);
    T03 = _mm_loadl_epi64((__m128i*)&src[3 * srcstride]);
    T04 = _mm_loadl_epi64((__m128i*)&src[4 * srcstride]);
    T05 = _mm_loadl_epi64((__m128i*)&src[5 * srcstride]);
    T06 = _mm_loadl_epi64((__m128i*)&src[6 * srcstride]);
    T07 = _mm_loadl_epi64((__m128i*)&src[7 * srcstride]);
    TRANSPOSE_8X8(T00, T01, T02, T03, T04, T05, T06, T07, T00, T01, T02, T03);
    _mm_storel_epi64((__m128i*)&dst[0 * dststride], T00);
    _mm_storeh_pi((__m64*)&dst[1 * dststride], _mm_castsi128_ps(T00));
    _mm_storel_epi64((__m128i*)&dst[2 * dststride], T01);
    _mm_storeh_pi((__m64*)&dst[3 * dststride], _mm_castsi128_ps(T01));
    _mm_storel_epi64((__m128i*)&dst[4 * dststride], T02);
    _mm_storeh_pi((__m64*)&dst[5 * dststride], _mm_castsi128_ps(T02));
    _mm_storel_epi64((__m128i*)&dst[6 * dststride], T03);
    _mm_storeh_pi((__m64*)&dst[7 * dststride], _mm_castsi128_ps(T03));

    T00 = _mm_loadl_epi64((__m128i*)&src[0 * srcstride + 8]);
    T01 = _mm_loadl_epi64((__m128i*)&src[1 * srcstride + 8]);
    T02 = _mm_loadl_epi64((__m128i*)&src[2 * srcstride + 8]);
    T03 = _mm_loadl_epi64((__m128i*)&src[3 * srcstride + 8]);
    T04 = _mm_loadl_epi64((__m128i*)&src[4 * srcstride + 8]);
    T05 = _mm_loadl_epi64((__m128i*)&src[5 * srcstride + 8]);
    T06 = _mm_loadl_epi64((__m128i*)&src[6 * srcstride + 8]);
    T07 = _mm_loadl_epi64((__m128i*)&src[7 * srcstride + 8]);
    TRANSPOSE_8X8(T00, T01, T02, T03, T04, T05, T06, T07, T00, T01, T02, T03);
    _mm_storel_epi64((__m128i*)&dst[8 * dststride], T00);
    _mm_storeh_pi((__m64*)&dst[9 * dststride], _mm_castsi128_ps(T00));
    _mm_storel_epi64((__m128i*)&dst[10 * dststride], T01);
    _mm_storeh_pi((__m64*)&dst[11 * dststride], _mm_castsi128_ps(T01));
    _mm_storel_epi64((__m128i*)&dst[12 * dststride], T02);
    _mm_storeh_pi((__m64*)&dst[13 * dststride], _mm_castsi128_ps(T02));
    _mm_storel_epi64((__m128i*)&dst[14 * dststride], T03);
    _mm_storeh_pi((__m64*)&dst[15 * dststride], _mm_castsi128_ps(T03));

    T00 = _mm_loadl_epi64((__m128i*)&src[8 * srcstride]);
    T01 = _mm_loadl_epi64((__m128i*)&src[9 * srcstride]);
    T02 = _mm_loadl_epi64((__m128i*)&src[10 * srcstride]);
    T03 = _mm_loadl_epi64((__m128i*)&src[11 * srcstride]);
    T04 = _mm_loadl_epi64((__m128i*)&src[12 * srcstride]);
    T05 = _mm_loadl_epi64((__m128i*)&src[13 * srcstride]);
    T06 = _mm_loadl_epi64((__m128i*)&src[14 * srcstride]);
    T07 = _mm_loadl_epi64((__m128i*)&src[15 * srcstride]);
    TRANSPOSE_8X8(T00, T01, T02, T03, T04, T05, T06, T07, T00, T01, T02, T03);
    _mm_storel_epi64((__m128i*)&dst[0 * dststride + 8], T00);
    _mm_storeh_pi((__m64*)&dst[1 * dststride + 8], _mm_castsi128_ps(T00));
    _mm_storel_epi64((__m128i*)&dst[2 * dststride + 8], T01);
    _mm_storeh_pi((__m64*)&dst[3 * dststride + 8], _mm_castsi128_ps(T01));
    _mm_storel_epi64((__m128i*)&dst[4 * dststride + 8], T02);
    _mm_storeh_pi((__m64*)&dst[5 * dststride + 8], _mm_castsi128_ps(T02));
    _mm_storel_epi64((__m128i*)&dst[6 * dststride + 8], T03);
    _mm_storeh_pi((__m64*)&dst[7 * dststride + 8], _mm_castsi128_ps(T03));

    T00 = _mm_loadl_epi64((__m128i*)&src[8 * srcstride + 8]);
    T01 = _mm_loadl_epi64((__m128i*)&src[9 * srcstride + 8]);
    T02 = _mm_loadl_epi64((__m128i*)&src[10 * srcstride + 8]);
    T03 = _mm_loadl_epi64((__m128i*)&src[11 * srcstride + 8]);
    T04 = _mm_loadl_epi64((__m128i*)&src[12 * srcstride + 8]);
    T05 = _mm_loadl_epi64((__m128i*)&src[13 * srcstride + 8]);
    T06 = _mm_loadl_epi64((__m128i*)&src[14 * srcstride + 8]);
    T07 = _mm_loadl_epi64((__m128i*)&src[15 * srcstride + 8]);
    TRANSPOSE_8X8(T00, T01, T02, T03, T04, T05, T06, T07, T00, T01, T02, T03);
    _mm_storel_epi64((__m128i*)&dst[8 * dststride + 8], T00);
    _mm_storeh_pi((__m64*)&dst[9 * dststride + 8], _mm_castsi128_ps(T00));
    _mm_storel_epi64((__m128i*)&dst[10 * dststride + 8], T01);
    _mm_storeh_pi((__m64*)&dst[11 * dststride + 8], _mm_castsi128_ps(T01));
    _mm_storel_epi64((__m128i*)&dst[12 * dststride + 8], T02);
    _mm_storeh_pi((__m64*)&dst[13 * dststride + 8], _mm_castsi128_ps(T02));
    _mm_storel_epi64((__m128i*)&dst[14 * dststride + 8], T03);
    _mm_storeh_pi((__m64*)&dst[15 * dststride + 8], _mm_castsi128_ps(T03));
}

void transpose16(pixel* dst, pixel* src, intptr_t srcstride)
{
    transpose16_dummy(dst, 16, src, srcstride);
}

void transpose32(pixel* dst, pixel* src, intptr_t srcstride)
{
    assert(dst != src);

    transpose16_dummy(dst,                32, src,                      srcstride);
    transpose16_dummy(dst + 16 * 32,      32, src + 16,                 srcstride);
    transpose16_dummy(dst + 16 * 32 + 16, 32, src + 16 * srcstride + 16, srcstride);
    transpose16_dummy(dst + 16,           32, src + 16 * srcstride,      srcstride);
}

void blockfil_s_4(short *dst, intptr_t dstride, short val)
{
    __m128i T00;

    T00 = _mm_cvtsi32_si128(val);
    T00 = _mm_shufflelo_epi16(T00, 0);

    _mm_storel_epi64((__m128i*)&dst[0 * dstride], T00);
    _mm_storel_epi64((__m128i*)&dst[1 * dstride], T00);
    _mm_storel_epi64((__m128i*)&dst[2 * dstride], T00);
    _mm_storel_epi64((__m128i*)&dst[3 * dstride], T00);
}

void blockfil_s_8(short *dst, intptr_t dstride, short val)
{
    __m128i T00;

    T00 = _mm_cvtsi32_si128(val);
    T00 = _mm_shufflelo_epi16(T00, 0);
    T00 = _mm_shuffle_epi32(T00, 0);

    _mm_storeu_si128((__m128i*)&dst[0 * dstride], T00);
    _mm_storeu_si128((__m128i*)&dst[1 * dstride], T00);
    _mm_storeu_si128((__m128i*)&dst[2 * dstride], T00);
    _mm_storeu_si128((__m128i*)&dst[3 * dstride], T00);
    _mm_storeu_si128((__m128i*)&dst[4 * dstride], T00);
    _mm_storeu_si128((__m128i*)&dst[5 * dstride], T00);
    _mm_storeu_si128((__m128i*)&dst[6 * dstride], T00);
    _mm_storeu_si128((__m128i*)&dst[7 * dstride], T00);
}

void blockfil_s_16(short *dst, intptr_t dstride, short val)
{
    __m128i T00;

    T00 = _mm_cvtsi32_si128(val);
    T00 = _mm_shufflelo_epi16(T00, 0);
    T00 = _mm_shuffle_epi32(T00, 0);

    for (int i = 0; i < 16; i += 4)
    {
        _mm_storeu_si128((__m128i*)&dst[(i + 0) * dstride], T00);
        _mm_storeu_si128((__m128i*)&dst[(i + 0) * dstride + 8], T00);
        _mm_storeu_si128((__m128i*)&dst[(i + 1) * dstride], T00);
        _mm_storeu_si128((__m128i*)&dst[(i + 1) * dstride + 8], T00);
        _mm_storeu_si128((__m128i*)&dst[(i + 2) * dstride], T00);
        _mm_storeu_si128((__m128i*)&dst[(i + 2) * dstride + 8], T00);
        _mm_storeu_si128((__m128i*)&dst[(i + 3) * dstride], T00);
        _mm_storeu_si128((__m128i*)&dst[(i + 3) * dstride + 8], T00);
    }
}

void blockfil_s_32(short *dst, intptr_t dstride, short val)
{
    __m128i T00;

    T00 = _mm_cvtsi32_si128(val);
    T00 = _mm_shufflelo_epi16(T00, 0);
    T00 = _mm_shuffle_epi32(T00, 0);

    for (int i = 0; i < 32; i += 2)
    {
        _mm_storeu_si128((__m128i*)&dst[(i + 0) * dstride], T00);
        _mm_storeu_si128((__m128i*)&dst[(i + 0) * dstride +  8], T00);
        _mm_storeu_si128((__m128i*)&dst[(i + 0) * dstride + 16], T00);
        _mm_storeu_si128((__m128i*)&dst[(i + 0) * dstride + 24], T00);
        _mm_storeu_si128((__m128i*)&dst[(i + 1) * dstride], T00);
        _mm_storeu_si128((__m128i*)&dst[(i + 1) * dstride +  8], T00);
        _mm_storeu_si128((__m128i*)&dst[(i + 1) * dstride + 16], T00);
        _mm_storeu_si128((__m128i*)&dst[(i + 1) * dstride + 24], T00);
    }
}
#endif

#define INSTRSET 3
#include "vectorclass.h"
#define ARCH sse3
#include "pixel.inc"
