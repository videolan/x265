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

/* this file instantiates AVX2 versions of the pixel primitives */

#include "primitives.h"

#ifdef __GNUC__
#include <x86intrin.h>                 // x86intrin.h includes header files for whatever instruction
                                       // sets are specified on the compiler command line, such as:
                                       // xopintrin.h, fma4intrin.h
#else
#include <immintrin.h>                 // MS version of immintrin.h covers AVX, AVX2 and FMA3
#endif // __GNUC__

using namespace x265;

namespace {
#if !HIGH_BIT_DEPTH
#define PROCESS_32x4(BASE) \
    T00 = _mm256_load_si256((__m256i*)(fenc + (BASE + 0) * fencstride)); \
    T01 = _mm256_load_si256((__m256i*)(fenc + (BASE + 1) * fencstride)); \
    T02 = _mm256_load_si256((__m256i*)(fenc + (BASE + 2) * fencstride)); \
    T03 = _mm256_load_si256((__m256i*)(fenc + (BASE + 3) * fencstride)); \
    T10 = _mm256_loadu_si256((__m256i*)(fref + (BASE + 0) * frefstride)); \
    T11 = _mm256_loadu_si256((__m256i*)(fref + (BASE + 1) * frefstride)); \
    T12 = _mm256_loadu_si256((__m256i*)(fref + (BASE + 2) * frefstride)); \
    T13 = _mm256_loadu_si256((__m256i*)(fref + (BASE + 3) * frefstride)); \
    T20 = _mm256_sad_epu8(T00, T10); \
    T21 = _mm256_sad_epu8(T01, T11); \
    T22 = _mm256_sad_epu8(T02, T12); \
    T23 = _mm256_sad_epu8(T03, T13); \
    sum0 = _mm256_add_epi32(sum0, T20); \
    sum0 = _mm256_add_epi32(sum0, T21); \
    sum0 = _mm256_add_epi32(sum0, T22); \
    sum0 = _mm256_add_epi32(sum0, T23)

template<int ly>
int sad_avx2_32(pixel * fenc, intptr_t fencstride, pixel * fref, intptr_t frefstride)
{
    __m256i sum0 = _mm256_setzero_si256();
    __m256i sum1 = _mm256_setzero_si256();
    __m256i T00, T01, T02, T03;
    __m256i T10, T11, T12, T13;
    __m256i T20, T21, T22, T23;

    for (int i = 0; i < ly; i += 8)
    {
        PROCESS_32x4(i);
        PROCESS_32x4(i + 4);
    }

    sum1 = _mm256_shuffle_epi32(sum0, 2);
    sum0 = _mm256_add_epi32(sum0, sum1);
    __m128i tmpsum0 = _mm256_extracti128_si256(sum0, 0);
    __m128i tmpsum1 = _mm256_extracti128_si256(sum0, 1);
    tmpsum0 = _mm_add_epi32(tmpsum0, tmpsum1);
    return _mm_cvtsi128_si32(tmpsum0);
}

#define PROCESS_64x4(BASE) \
    T00 = _mm256_load_si256((__m256i*)(fenc + (BASE + 0) * fencstride)); \
    T01 = _mm256_load_si256((__m256i*)(fenc + (BASE + 1) * fencstride)); \
    T02 = _mm256_load_si256((__m256i*)(fenc + (BASE + 2) * fencstride)); \
    T03 = _mm256_load_si256((__m256i*)(fenc + (BASE + 3) * fencstride)); \
    T10 = _mm256_loadu_si256((__m256i*)(fref + (BASE + 0) * frefstride)); \
    T11 = _mm256_loadu_si256((__m256i*)(fref + (BASE + 1) * frefstride)); \
    T12 = _mm256_loadu_si256((__m256i*)(fref + (BASE + 2) * frefstride)); \
    T13 = _mm256_loadu_si256((__m256i*)(fref + (BASE + 3) * frefstride)); \
    T20 = _mm256_sad_epu8(T00, T10); \
    T21 = _mm256_sad_epu8(T01, T11); \
    T22 = _mm256_sad_epu8(T02, T12); \
    T23 = _mm256_sad_epu8(T03, T13); \
    sum0 = _mm256_add_epi32(sum0, T20); \
    sum0 = _mm256_add_epi32(sum0, T21); \
    sum0 = _mm256_add_epi32(sum0, T22); \
    sum0 = _mm256_add_epi32(sum0, T23); \
    T00 = _mm256_load_si256((__m256i*)(fenc + 32 + (BASE + 0) * fencstride)); \
    T01 = _mm256_load_si256((__m256i*)(fenc + 32 + (BASE + 1) * fencstride)); \
    T02 = _mm256_load_si256((__m256i*)(fenc + 32 + (BASE + 2) * fencstride)); \
    T03 = _mm256_load_si256((__m256i*)(fenc + 32 + (BASE + 3) * fencstride)); \
    T10 = _mm256_loadu_si256((__m256i*)(fref + 32 + (BASE + 0) * frefstride)); \
    T11 = _mm256_loadu_si256((__m256i*)(fref + 32 + (BASE + 1) * frefstride)); \
    T12 = _mm256_loadu_si256((__m256i*)(fref + 32 + (BASE + 2) * frefstride)); \
    T13 = _mm256_loadu_si256((__m256i*)(fref + 32 + (BASE + 3) * frefstride)); \
    T20 = _mm256_sad_epu8(T00, T10); \
    T21 = _mm256_sad_epu8(T01, T11); \
    T22 = _mm256_sad_epu8(T02, T12); \
    T23 = _mm256_sad_epu8(T03, T13); \
    sum0 = _mm256_add_epi32(sum0, T20); \
    sum0 = _mm256_add_epi32(sum0, T21); \
    sum0 = _mm256_add_epi32(sum0, T22); \
    sum0 = _mm256_add_epi32(sum0, T23)

template<int ly>
int sad_avx2_64(pixel * fenc, intptr_t fencstride, pixel * fref, intptr_t frefstride)
{
    __m256i sum0 = _mm256_setzero_si256();
    __m256i sum1 = _mm256_setzero_si256();
    __m256i T00, T01, T02, T03;
    __m256i T10, T11, T12, T13;
    __m256i T20, T21, T22, T23;

    for (int i = 0; i < ly; i += 8)
    {
        PROCESS_64x4(i);
        PROCESS_64x4(i + 4);
    }

    sum1 = _mm256_shuffle_epi32(sum0, 2);
    sum0 = _mm256_add_epi32(sum0, sum1);
    __m128i tmpsum0 = _mm256_extracti128_si256(sum0, 0);
    __m128i tmpsum1 = _mm256_extracti128_si256(sum0, 1);
    tmpsum0 = _mm_add_epi32(tmpsum0, tmpsum1);
    return _mm_cvtsi128_si32(tmpsum0);
}

#define PROCESS_X3_32x4(BASE) \
    T00 = _mm256_load_si256((__m256i*)(fenc + (BASE + 0) * FENC_STRIDE)); \
    T01 = _mm256_load_si256((__m256i*)(fenc + (BASE + 1) * FENC_STRIDE)); \
    T02 = _mm256_load_si256((__m256i*)(fenc + (BASE + 2) * FENC_STRIDE)); \
    T03 = _mm256_load_si256((__m256i*)(fenc + (BASE + 3) * FENC_STRIDE)); \
    T10 = _mm256_loadu_si256((__m256i*)(fref1 + (BASE + 0) * frefstride)); \
    T11 = _mm256_loadu_si256((__m256i*)(fref1 + (BASE + 1) * frefstride)); \
    T12 = _mm256_loadu_si256((__m256i*)(fref1 + (BASE + 2) * frefstride)); \
    T13 = _mm256_loadu_si256((__m256i*)(fref1 + (BASE + 3) * frefstride)); \
    T20 = _mm256_sad_epu8(T00, T10); \
    T21 = _mm256_sad_epu8(T01, T11); \
    T22 = _mm256_sad_epu8(T02, T12); \
    T23 = _mm256_sad_epu8(T03, T13); \
    T20 = _mm256_add_epi32(T20, T21); \
    T20 = _mm256_add_epi32(T20, T22); \
    T20 = _mm256_add_epi32(T20, T23); \
    sum0 = _mm256_add_epi32(T20, sum0); \
    T10 = _mm256_loadu_si256((__m256i*)(fref2 + (BASE + 0) * frefstride)); \
    T11 = _mm256_loadu_si256((__m256i*)(fref2 + (BASE + 1) * frefstride)); \
    T12 = _mm256_loadu_si256((__m256i*)(fref2 + (BASE + 2) * frefstride)); \
    T13 = _mm256_loadu_si256((__m256i*)(fref2 + (BASE + 3) * frefstride)); \
    T20 = _mm256_sad_epu8(T00, T10); \
    T21 = _mm256_sad_epu8(T01, T11); \
    T22 = _mm256_sad_epu8(T02, T12); \
    T23 = _mm256_sad_epu8(T03, T13); \
    T20 = _mm256_add_epi32(T20, T21); \
    T20 = _mm256_add_epi32(T20, T22); \
    T20 = _mm256_add_epi32(T20, T23); \
    sum1 = _mm256_add_epi32(T20, sum1); \
    T10 = _mm256_loadu_si256((__m256i*)(fref3 + (BASE + 0) * frefstride)); \
    T11 = _mm256_loadu_si256((__m256i*)(fref3 + (BASE + 1) * frefstride)); \
    T12 = _mm256_loadu_si256((__m256i*)(fref3 + (BASE + 2) * frefstride)); \
    T13 = _mm256_loadu_si256((__m256i*)(fref3 + (BASE + 3) * frefstride)); \
    T20 = _mm256_sad_epu8(T00, T10); \
    T21 = _mm256_sad_epu8(T01, T11); \
    T22 = _mm256_sad_epu8(T02, T12); \
    T23 = _mm256_sad_epu8(T03, T13); \
    T20 = _mm256_add_epi32(T20, T21); \
    T20 = _mm256_add_epi32(T20, T22); \
    T20 = _mm256_add_epi32(T20, T23); \
    sum2 = _mm256_add_epi32(T20, sum2)

template<int ly>
void sad_avx2_x3_32(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, intptr_t frefstride, int *res)
{
    __m256i sum0 = _mm256_setzero_si256();
    __m256i sum1 = _mm256_setzero_si256();
    __m256i sum2 = _mm256_setzero_si256();
    __m256i sum3;
    __m256i T00, T01, T02, T03;
    __m256i T10, T11, T12, T13;
    __m256i T20, T21, T22, T23;

    for (int i = 0; i < ly; i += 8)
    {
        PROCESS_X3_32x4(i);
        PROCESS_X3_32x4(i + 4);
    }

    sum3 = _mm256_shuffle_epi32(sum0, 2);
    sum0 = _mm256_add_epi32(sum0, sum3);
    __m128i tmpsum0 = _mm256_extracti128_si256(sum0, 0);
    __m128i tmpsum1 = _mm256_extracti128_si256(sum0, 1);
    tmpsum0 = _mm_add_epi32(tmpsum0, tmpsum1);
    res[0] = _mm_cvtsi128_si32(tmpsum0);

    sum3 = _mm256_shuffle_epi32(sum1, 2);
    sum1 = _mm256_add_epi32(sum1, sum3);
    tmpsum0 = _mm256_extracti128_si256(sum1, 0);
    tmpsum1 = _mm256_extracti128_si256(sum1, 1);
    tmpsum0 = _mm_add_epi32(tmpsum0, tmpsum1);
    res[1] = _mm_cvtsi128_si32(tmpsum0);

    sum3 = _mm256_shuffle_epi32(sum2, 2);
    sum2 = _mm256_add_epi32(sum2, sum3);
    tmpsum0 = _mm256_extracti128_si256(sum2, 0);
    tmpsum1 = _mm256_extracti128_si256(sum2, 1);
    tmpsum0 = _mm_add_epi32(tmpsum0, tmpsum1);
    res[2] = _mm_cvtsi128_si32(tmpsum0);
}

#define PROCESS_X3_64x4(BASE) \
    T00 = _mm256_load_si256((__m256i*)(fenc + (BASE + 0) * FENC_STRIDE)); \
    T01 = _mm256_load_si256((__m256i*)(fenc + (BASE + 1) * FENC_STRIDE)); \
    T02 = _mm256_load_si256((__m256i*)(fenc + (BASE + 2) * FENC_STRIDE)); \
    T03 = _mm256_load_si256((__m256i*)(fenc + (BASE + 3) * FENC_STRIDE)); \
    T10 = _mm256_loadu_si256((__m256i*)(fref1 + (BASE + 0) * frefstride)); \
    T11 = _mm256_loadu_si256((__m256i*)(fref1 + (BASE + 1) * frefstride)); \
    T12 = _mm256_loadu_si256((__m256i*)(fref1 + (BASE + 2) * frefstride)); \
    T13 = _mm256_loadu_si256((__m256i*)(fref1 + (BASE + 3) * frefstride)); \
    T20 = _mm256_sad_epu8(T00, T10); \
    T21 = _mm256_sad_epu8(T01, T11); \
    T22 = _mm256_sad_epu8(T02, T12); \
    T23 = _mm256_sad_epu8(T03, T13); \
    T20 = _mm256_add_epi32(T20, T21); \
    T20 = _mm256_add_epi32(T20, T22); \
    T20 = _mm256_add_epi32(T20, T23); \
    sum0 = _mm256_add_epi32(T20, sum0); \
    T10 = _mm256_loadu_si256((__m256i*)(fref2 + (BASE + 0) * frefstride)); \
    T11 = _mm256_loadu_si256((__m256i*)(fref2 + (BASE + 1) * frefstride)); \
    T12 = _mm256_loadu_si256((__m256i*)(fref2 + (BASE + 2) * frefstride)); \
    T13 = _mm256_loadu_si256((__m256i*)(fref2 + (BASE + 3) * frefstride)); \
    T20 = _mm256_sad_epu8(T00, T10); \
    T21 = _mm256_sad_epu8(T01, T11); \
    T22 = _mm256_sad_epu8(T02, T12); \
    T23 = _mm256_sad_epu8(T03, T13); \
    T20 = _mm256_add_epi32(T20, T21); \
    T20 = _mm256_add_epi32(T20, T22); \
    T20 = _mm256_add_epi32(T20, T23); \
    sum1 = _mm256_add_epi32(T20, sum1); \
    T10 = _mm256_loadu_si256((__m256i*)(fref3 + (BASE + 0) * frefstride)); \
    T11 = _mm256_loadu_si256((__m256i*)(fref3 + (BASE + 1) * frefstride)); \
    T12 = _mm256_loadu_si256((__m256i*)(fref3 + (BASE + 2) * frefstride)); \
    T13 = _mm256_loadu_si256((__m256i*)(fref3 + (BASE + 3) * frefstride)); \
    T20 = _mm256_sad_epu8(T00, T10); \
    T21 = _mm256_sad_epu8(T01, T11); \
    T22 = _mm256_sad_epu8(T02, T12); \
    T23 = _mm256_sad_epu8(T03, T13); \
    T20 = _mm256_add_epi32(T20, T21); \
    T20 = _mm256_add_epi32(T20, T22); \
    T20 = _mm256_add_epi32(T20, T23); \
    sum2 = _mm256_add_epi32(T20, sum2); \
    T00 = _mm256_load_si256((__m256i*)(fenc + 32 + (BASE + 0) * FENC_STRIDE)); \
    T01 = _mm256_load_si256((__m256i*)(fenc + 32 + (BASE + 1) * FENC_STRIDE)); \
    T02 = _mm256_load_si256((__m256i*)(fenc + 32 + (BASE + 2) * FENC_STRIDE)); \
    T03 = _mm256_load_si256((__m256i*)(fenc + 32 + (BASE + 3) * FENC_STRIDE)); \
    T10 = _mm256_loadu_si256((__m256i*)(fref1 + 32 + (BASE + 0) * frefstride)); \
    T11 = _mm256_loadu_si256((__m256i*)(fref1 + 32 + (BASE + 1) * frefstride)); \
    T12 = _mm256_loadu_si256((__m256i*)(fref1 + 32 + (BASE + 2) * frefstride)); \
    T13 = _mm256_loadu_si256((__m256i*)(fref1 + 32 + (BASE + 3) * frefstride)); \
    T20 = _mm256_sad_epu8(T00, T10); \
    T21 = _mm256_sad_epu8(T01, T11); \
    T22 = _mm256_sad_epu8(T02, T12); \
    T23 = _mm256_sad_epu8(T03, T13); \
    T20 = _mm256_add_epi32(T20, T21); \
    T20 = _mm256_add_epi32(T20, T22); \
    T20 = _mm256_add_epi32(T20, T23); \
    sum0 = _mm256_add_epi32(T20, sum0); \
    T10 = _mm256_loadu_si256((__m256i*)(fref2 + 32 + (BASE + 0) * frefstride)); \
    T11 = _mm256_loadu_si256((__m256i*)(fref2 + 32 + (BASE + 1) * frefstride)); \
    T12 = _mm256_loadu_si256((__m256i*)(fref2 + 32 + (BASE + 2) * frefstride)); \
    T13 = _mm256_loadu_si256((__m256i*)(fref2 + 32 + (BASE + 3) * frefstride)); \
    T20 = _mm256_sad_epu8(T00, T10); \
    T21 = _mm256_sad_epu8(T01, T11); \
    T22 = _mm256_sad_epu8(T02, T12); \
    T23 = _mm256_sad_epu8(T03, T13); \
    T20 = _mm256_add_epi32(T20, T21); \
    T20 = _mm256_add_epi32(T20, T22); \
    T20 = _mm256_add_epi32(T20, T23); \
    sum1 = _mm256_add_epi32(T20, sum1); \
    T10 = _mm256_loadu_si256((__m256i*)(fref3 + 32 + (BASE + 0) * frefstride)); \
    T11 = _mm256_loadu_si256((__m256i*)(fref3 + 32 + (BASE + 1) * frefstride)); \
    T12 = _mm256_loadu_si256((__m256i*)(fref3 + 32 + (BASE + 2) * frefstride)); \
    T13 = _mm256_loadu_si256((__m256i*)(fref3 + 32 + (BASE + 3) * frefstride)); \
    T20 = _mm256_sad_epu8(T00, T10); \
    T21 = _mm256_sad_epu8(T01, T11); \
    T22 = _mm256_sad_epu8(T02, T12); \
    T23 = _mm256_sad_epu8(T03, T13); \
    T20 = _mm256_add_epi32(T20, T21); \
    T20 = _mm256_add_epi32(T20, T22); \
    T20 = _mm256_add_epi32(T20, T23); \
    sum2 = _mm256_add_epi32(T20, sum2)

template<int ly>
void sad_avx2_x3_64(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, intptr_t frefstride, int *res)
{
    __m256i sum0 = _mm256_setzero_si256();
    __m256i sum1 = _mm256_setzero_si256();
    __m256i sum2 = _mm256_setzero_si256();
    __m256i sum3;
    __m256i T00, T01, T02, T03;
    __m256i T10, T11, T12, T13;
    __m256i T20, T21, T22, T23;

    for (int i = 0; i < ly; i += 8)
    {
        PROCESS_X3_64x4(i);
        PROCESS_X3_64x4(i + 4);
    }

    sum3 = _mm256_shuffle_epi32(sum0, 2);
    sum0 = _mm256_add_epi32(sum0, sum3);
    __m128i tmpsum0 = _mm256_extracti128_si256(sum0, 0);
    __m128i tmpsum1 = _mm256_extracti128_si256(sum0, 1);
    tmpsum0 = _mm_add_epi32(tmpsum0, tmpsum1);
    res[0] = _mm_cvtsi128_si32(tmpsum0);

    sum3 = _mm256_shuffle_epi32(sum1, 2);
    sum1 = _mm256_add_epi32(sum1, sum3);
    tmpsum0 = _mm256_extracti128_si256(sum1, 0);
    tmpsum1 = _mm256_extracti128_si256(sum1, 1);
    tmpsum0 = _mm_add_epi32(tmpsum0, tmpsum1);
    res[1] = _mm_cvtsi128_si32(tmpsum0);

    sum3 = _mm256_shuffle_epi32(sum2, 2);
    sum2 = _mm256_add_epi32(sum2, sum3);
    tmpsum0 = _mm256_extracti128_si256(sum2, 0);
    tmpsum1 = _mm256_extracti128_si256(sum2, 1);
    tmpsum0 = _mm_add_epi32(tmpsum0, tmpsum1);
    res[2] = _mm_cvtsi128_si32(tmpsum0);
}


#define PROCESS_X4_32x4(BASE) \
    T00 = _mm256_load_si256((__m256i*)(fenc + (BASE + 0) * FENC_STRIDE)); \
    T01 = _mm256_load_si256((__m256i*)(fenc + (BASE + 1) * FENC_STRIDE)); \
    T02 = _mm256_load_si256((__m256i*)(fenc + (BASE + 2) * FENC_STRIDE)); \
    T03 = _mm256_load_si256((__m256i*)(fenc + (BASE + 3) * FENC_STRIDE)); \
    T10 = _mm256_loadu_si256((__m256i*)(fref1 + (BASE + 0) * frefstride)); \
    T11 = _mm256_loadu_si256((__m256i*)(fref1 + (BASE + 1) * frefstride)); \
    T12 = _mm256_loadu_si256((__m256i*)(fref1 + (BASE + 2) * frefstride)); \
    T13 = _mm256_loadu_si256((__m256i*)(fref1 + (BASE + 3) * frefstride)); \
    T20 = _mm256_sad_epu8(T00, T10); \
    T21 = _mm256_sad_epu8(T01, T11); \
    T22 = _mm256_sad_epu8(T02, T12); \
    T23 = _mm256_sad_epu8(T03, T13); \
    T20 = _mm256_add_epi32(T20, T21); \
    T20 = _mm256_add_epi32(T20, T22); \
    T20 = _mm256_add_epi32(T20, T23); \
    sum0 = _mm256_add_epi32(T20, sum0); \
    T10 = _mm256_loadu_si256((__m256i*)(fref2 + (BASE + 0) * frefstride)); \
    T11 = _mm256_loadu_si256((__m256i*)(fref2 + (BASE + 1) * frefstride)); \
    T12 = _mm256_loadu_si256((__m256i*)(fref2 + (BASE + 2) * frefstride)); \
    T13 = _mm256_loadu_si256((__m256i*)(fref2 + (BASE + 3) * frefstride)); \
    T20 = _mm256_sad_epu8(T00, T10); \
    T21 = _mm256_sad_epu8(T01, T11); \
    T22 = _mm256_sad_epu8(T02, T12); \
    T23 = _mm256_sad_epu8(T03, T13); \
    T20 = _mm256_add_epi32(T20, T21); \
    T20 = _mm256_add_epi32(T20, T22); \
    T20 = _mm256_add_epi32(T20, T23); \
    sum1 = _mm256_add_epi32(T20, sum1); \
    T10 = _mm256_loadu_si256((__m256i*)(fref3 + (BASE + 0) * frefstride)); \
    T11 = _mm256_loadu_si256((__m256i*)(fref3 + (BASE + 1) * frefstride)); \
    T12 = _mm256_loadu_si256((__m256i*)(fref3 + (BASE + 2) * frefstride)); \
    T13 = _mm256_loadu_si256((__m256i*)(fref3 + (BASE + 3) * frefstride)); \
    T20 = _mm256_sad_epu8(T00, T10); \
    T21 = _mm256_sad_epu8(T01, T11); \
    T22 = _mm256_sad_epu8(T02, T12); \
    T23 = _mm256_sad_epu8(T03, T13); \
    T20 = _mm256_add_epi32(T20, T21); \
    T20 = _mm256_add_epi32(T20, T22); \
    T20 = _mm256_add_epi32(T20, T23); \
    sum2 = _mm256_add_epi32(T20, sum2); \
    T10 = _mm256_loadu_si256((__m256i*)(fref4 + (BASE + 0) * frefstride)); \
    T11 = _mm256_loadu_si256((__m256i*)(fref4 + (BASE + 1) * frefstride)); \
    T12 = _mm256_loadu_si256((__m256i*)(fref4 + (BASE + 2) * frefstride)); \
    T13 = _mm256_loadu_si256((__m256i*)(fref4 + (BASE + 3) * frefstride)); \
    T20 = _mm256_sad_epu8(T00, T10); \
    T21 = _mm256_sad_epu8(T01, T11); \
    T22 = _mm256_sad_epu8(T02, T12); \
    T23 = _mm256_sad_epu8(T03, T13); \
    T20 = _mm256_add_epi32(T20, T21); \
    T20 = _mm256_add_epi32(T20, T22); \
    T20 = _mm256_add_epi32(T20, T23); \
    sum3 = _mm256_add_epi32(T20, sum3)

template<int ly>
void sad_avx2_x4_32(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3,pixel *fref4, intptr_t frefstride, int *res)
{
    __m256i sum0 = _mm256_setzero_si256();
    __m256i sum1 = _mm256_setzero_si256();
    __m256i sum2 = _mm256_setzero_si256();
    __m256i sum3 = _mm256_setzero_si256();
    __m256i sum4;
    __m256i T00, T01, T02, T03;
    __m256i T10, T11, T12, T13;
    __m256i T20, T21, T22, T23;

    for (int i = 0; i < ly; i += 8)
    {
        PROCESS_X4_32x4(i);
        PROCESS_X4_32x4(i + 4);
    }

    sum4 = _mm256_shuffle_epi32(sum0, 2);
    sum0 = _mm256_add_epi32(sum0, sum4);
    __m128i tmpsum0 = _mm256_extracti128_si256(sum0, 0);
    __m128i tmpsum1 = _mm256_extracti128_si256(sum0, 1);
    tmpsum0 = _mm_add_epi32(tmpsum0, tmpsum1);
    res[0] = _mm_cvtsi128_si32(tmpsum0);

    sum4 = _mm256_shuffle_epi32(sum1, 2);
    sum1 = _mm256_add_epi32(sum1, sum4);
    tmpsum0 = _mm256_extracti128_si256(sum1, 0);
    tmpsum1 = _mm256_extracti128_si256(sum1, 1);
    tmpsum0 = _mm_add_epi32(tmpsum0, tmpsum1);
    res[1] = _mm_cvtsi128_si32(tmpsum0);

    sum4 = _mm256_shuffle_epi32(sum2, 2);
    sum2 = _mm256_add_epi32(sum2, sum4);
    tmpsum0 = _mm256_extracti128_si256(sum2, 0);
    tmpsum1 = _mm256_extracti128_si256(sum2, 1);
    tmpsum0 = _mm_add_epi32(tmpsum0, tmpsum1);
    res[2] = _mm_cvtsi128_si32(tmpsum0);

    sum4 = _mm256_shuffle_epi32(sum3, 2);
    sum3 = _mm256_add_epi32(sum3, sum4);
    tmpsum0 = _mm256_extracti128_si256(sum3, 0);
    tmpsum1 = _mm256_extracti128_si256(sum3, 1);
    tmpsum0 = _mm_add_epi32(tmpsum0, tmpsum1);
    res[3] = _mm_cvtsi128_si32(tmpsum0);
}

#define PROCESS_X4_64x4(BASE) \
    T00 = _mm256_load_si256((__m256i*)(fenc + (BASE + 0) * FENC_STRIDE)); \
    T01 = _mm256_load_si256((__m256i*)(fenc + (BASE + 1) * FENC_STRIDE)); \
    T02 = _mm256_load_si256((__m256i*)(fenc + (BASE + 2) * FENC_STRIDE)); \
    T03 = _mm256_load_si256((__m256i*)(fenc + (BASE + 3) * FENC_STRIDE)); \
    T10 = _mm256_loadu_si256((__m256i*)(fref1 + (BASE + 0) * frefstride)); \
    T11 = _mm256_loadu_si256((__m256i*)(fref1 + (BASE + 1) * frefstride)); \
    T12 = _mm256_loadu_si256((__m256i*)(fref1 + (BASE + 2) * frefstride)); \
    T13 = _mm256_loadu_si256((__m256i*)(fref1 + (BASE + 3) * frefstride)); \
    T20 = _mm256_sad_epu8(T00, T10); \
    T21 = _mm256_sad_epu8(T01, T11); \
    T22 = _mm256_sad_epu8(T02, T12); \
    T23 = _mm256_sad_epu8(T03, T13); \
    T20 = _mm256_add_epi32(T20, T21); \
    T20 = _mm256_add_epi32(T20, T22); \
    T20 = _mm256_add_epi32(T20, T23); \
    sum0 = _mm256_add_epi32(T20, sum0); \
    T10 = _mm256_loadu_si256((__m256i*)(fref2 + (BASE + 0) * frefstride)); \
    T11 = _mm256_loadu_si256((__m256i*)(fref2 + (BASE + 1) * frefstride)); \
    T12 = _mm256_loadu_si256((__m256i*)(fref2 + (BASE + 2) * frefstride)); \
    T13 = _mm256_loadu_si256((__m256i*)(fref2 + (BASE + 3) * frefstride)); \
    T20 = _mm256_sad_epu8(T00, T10); \
    T21 = _mm256_sad_epu8(T01, T11); \
    T22 = _mm256_sad_epu8(T02, T12); \
    T23 = _mm256_sad_epu8(T03, T13); \
    T20 = _mm256_add_epi32(T20, T21); \
    T20 = _mm256_add_epi32(T20, T22); \
    T20 = _mm256_add_epi32(T20, T23); \
    sum1 = _mm256_add_epi32(T20, sum1); \
    T10 = _mm256_loadu_si256((__m256i*)(fref3 + (BASE + 0) * frefstride)); \
    T11 = _mm256_loadu_si256((__m256i*)(fref3 + (BASE + 1) * frefstride)); \
    T12 = _mm256_loadu_si256((__m256i*)(fref3 + (BASE + 2) * frefstride)); \
    T13 = _mm256_loadu_si256((__m256i*)(fref3 + (BASE + 3) * frefstride)); \
    T20 = _mm256_sad_epu8(T00, T10); \
    T21 = _mm256_sad_epu8(T01, T11); \
    T22 = _mm256_sad_epu8(T02, T12); \
    T23 = _mm256_sad_epu8(T03, T13); \
    T20 = _mm256_add_epi32(T20, T21); \
    T20 = _mm256_add_epi32(T20, T22); \
    T20 = _mm256_add_epi32(T20, T23); \
    sum2 = _mm256_add_epi32(T20, sum2); \
    T10 = _mm256_loadu_si256((__m256i*)(fref4 + (BASE + 0) * frefstride)); \
    T11 = _mm256_loadu_si256((__m256i*)(fref4 + (BASE + 1) * frefstride)); \
    T12 = _mm256_loadu_si256((__m256i*)(fref4 + (BASE + 2) * frefstride)); \
    T13 = _mm256_loadu_si256((__m256i*)(fref4 + (BASE + 3) * frefstride)); \
    T20 = _mm256_sad_epu8(T00, T10); \
    T21 = _mm256_sad_epu8(T01, T11); \
    T22 = _mm256_sad_epu8(T02, T12); \
    T23 = _mm256_sad_epu8(T03, T13); \
    T20 = _mm256_add_epi32(T20, T21); \
    T20 = _mm256_add_epi32(T20, T22); \
    T20 = _mm256_add_epi32(T20, T23); \
    sum3 = _mm256_add_epi32(T20, sum3); \
    T00 = _mm256_load_si256((__m256i*)(fenc + 32 + (BASE + 0) * FENC_STRIDE)); \
    T01 = _mm256_load_si256((__m256i*)(fenc + 32 + (BASE + 1) * FENC_STRIDE)); \
    T02 = _mm256_load_si256((__m256i*)(fenc + 32 + (BASE + 2) * FENC_STRIDE)); \
    T03 = _mm256_load_si256((__m256i*)(fenc + 32 + (BASE + 3) * FENC_STRIDE)); \
    T10 = _mm256_loadu_si256((__m256i*)(fref1 + 32 + (BASE + 0) * frefstride)); \
    T11 = _mm256_loadu_si256((__m256i*)(fref1 + 32 + (BASE + 1) * frefstride)); \
    T12 = _mm256_loadu_si256((__m256i*)(fref1 + 32 + (BASE + 2) * frefstride)); \
    T13 = _mm256_loadu_si256((__m256i*)(fref1 + 32 + (BASE + 3) * frefstride)); \
    T20 = _mm256_sad_epu8(T00, T10); \
    T21 = _mm256_sad_epu8(T01, T11); \
    T22 = _mm256_sad_epu8(T02, T12); \
    T23 = _mm256_sad_epu8(T03, T13); \
    T20 = _mm256_add_epi32(T20, T21); \
    T20 = _mm256_add_epi32(T20, T22); \
    T20 = _mm256_add_epi32(T20, T23); \
    sum0 = _mm256_add_epi32(T20, sum0); \
    T10 = _mm256_loadu_si256((__m256i*)(fref2 + 32 + (BASE + 0) * frefstride)); \
    T11 = _mm256_loadu_si256((__m256i*)(fref2 + 32 + (BASE + 1) * frefstride)); \
    T12 = _mm256_loadu_si256((__m256i*)(fref2 + 32 + (BASE + 2) * frefstride)); \
    T13 = _mm256_loadu_si256((__m256i*)(fref2 + 32 + (BASE + 3) * frefstride)); \
    T20 = _mm256_sad_epu8(T00, T10); \
    T21 = _mm256_sad_epu8(T01, T11); \
    T22 = _mm256_sad_epu8(T02, T12); \
    T23 = _mm256_sad_epu8(T03, T13); \
    T20 = _mm256_add_epi32(T20, T21); \
    T20 = _mm256_add_epi32(T20, T22); \
    T20 = _mm256_add_epi32(T20, T23); \
    sum1 = _mm256_add_epi32(T20, sum1); \
    T10 = _mm256_loadu_si256((__m256i*)(fref3 + 32 + (BASE + 0) * frefstride)); \
    T11 = _mm256_loadu_si256((__m256i*)(fref3 + 32 + (BASE + 1) * frefstride)); \
    T12 = _mm256_loadu_si256((__m256i*)(fref3 + 32 + (BASE + 2) * frefstride)); \
    T13 = _mm256_loadu_si256((__m256i*)(fref3 + 32 + (BASE + 3) * frefstride)); \
    T20 = _mm256_sad_epu8(T00, T10); \
    T21 = _mm256_sad_epu8(T01, T11); \
    T22 = _mm256_sad_epu8(T02, T12); \
    T23 = _mm256_sad_epu8(T03, T13); \
    T20 = _mm256_add_epi32(T20, T21); \
    T20 = _mm256_add_epi32(T20, T22); \
    T20 = _mm256_add_epi32(T20, T23); \
    sum2 = _mm256_add_epi32(T20, sum2); \
    T10 = _mm256_loadu_si256((__m256i*)(fref4 + 32 + (BASE + 0) * frefstride)); \
    T11 = _mm256_loadu_si256((__m256i*)(fref4 + 32 + (BASE + 1) * frefstride)); \
    T12 = _mm256_loadu_si256((__m256i*)(fref4 + 32 + (BASE + 2) * frefstride)); \
    T13 = _mm256_loadu_si256((__m256i*)(fref4 + 32 + (BASE + 3) * frefstride)); \
    T20 = _mm256_sad_epu8(T00, T10); \
    T21 = _mm256_sad_epu8(T01, T11); \
    T22 = _mm256_sad_epu8(T02, T12); \
    T23 = _mm256_sad_epu8(T03, T13); \
    T20 = _mm256_add_epi32(T20, T21); \
    T20 = _mm256_add_epi32(T20, T22); \
    T20 = _mm256_add_epi32(T20, T23); \
    sum3 = _mm256_add_epi32(T20, sum3)

template<int ly>
void sad_avx2_x4_64(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3,pixel *fref4, intptr_t frefstride, int *res)
{
    __m256i sum0 = _mm256_setzero_si256();
    __m256i sum1 = _mm256_setzero_si256();
    __m256i sum2 = _mm256_setzero_si256();
    __m256i sum3 = _mm256_setzero_si256();
    __m256i sum4;
    __m256i T00, T01, T02, T03;
    __m256i T10, T11, T12, T13;
    __m256i T20, T21, T22, T23;

    for (int i = 0; i < ly; i += 8)
    {
        PROCESS_X4_64x4(i);
        PROCESS_X4_64x4(i + 4);
    }

    sum4 = _mm256_shuffle_epi32(sum0, 2);
    sum0 = _mm256_add_epi32(sum0, sum4);
    __m128i tmpsum0 = _mm256_extracti128_si256(sum0, 0);
    __m128i tmpsum1 = _mm256_extracti128_si256(sum0, 1);
    tmpsum0 = _mm_add_epi32(tmpsum0, tmpsum1);
    res[0] = _mm_cvtsi128_si32(tmpsum0);

    sum4 = _mm256_shuffle_epi32(sum1, 2);
    sum1 = _mm256_add_epi32(sum1, sum4);
    tmpsum0 = _mm256_extracti128_si256(sum1, 0);
    tmpsum1 = _mm256_extracti128_si256(sum1, 1);
    tmpsum0 = _mm_add_epi32(tmpsum0, tmpsum1);
    res[1] = _mm_cvtsi128_si32(tmpsum0);

    sum4 = _mm256_shuffle_epi32(sum2, 2);
    sum2 = _mm256_add_epi32(sum2, sum4);
    tmpsum0 = _mm256_extracti128_si256(sum2, 0);
    tmpsum1 = _mm256_extracti128_si256(sum2, 1);
    tmpsum0 = _mm_add_epi32(tmpsum0, tmpsum1);
    res[2] = _mm_cvtsi128_si32(tmpsum0);

    sum4 = _mm256_shuffle_epi32(sum3, 2);
    sum3 = _mm256_add_epi32(sum3, sum4);
    tmpsum0 = _mm256_extracti128_si256(sum3, 0);
    tmpsum1 = _mm256_extracti128_si256(sum3, 1);
    tmpsum0 = _mm_add_epi32(tmpsum0, tmpsum1);
    res[3] = _mm_cvtsi128_si32(tmpsum0);
}

#endif // if !HIGH_BIT_DEPTH
}

namespace x265 {
void Setup_Vec_PixelPrimitives_avx2(EncoderPrimitives &p)
{
    p.sad[0] = p.sad[0];
#define SET_SADS(W, H) \
    p.sad[LUMA_ ## W ## x ## H] = sad_avx2_ ## W<H>; \
    p.sad_x3[LUMA_ ## W ## x ## H] = sad_avx2_x3_ ## W<H>; \
    p.sad_x4[LUMA_ ## W ## x ## H] = sad_avx2_x4_ ## W<H>; \

#if !HIGH_BIT_DEPTH
#if (defined(__GNUC__) || defined(__INTEL_COMPILER)) || defined(__clang__)
    SET_SADS(32, 8);
    SET_SADS(32, 16);
    SET_SADS(32, 24);
    SET_SADS(32, 32);
    SET_SADS(32, 64);
    SET_SADS(64, 16);
    SET_SADS(64, 32);
    SET_SADS(64, 48);
    SET_SADS(64, 64);
#endif
#endif // if !HIGH_BIT_DEPTH
}
}
