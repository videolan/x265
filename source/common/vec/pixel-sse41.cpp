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

/* this file instantiates SSE4.1 versions of the pixel primitives */

#include "primitives.h"
#include <assert.h>
#include <xmmintrin.h> // SSE
#include <smmintrin.h> // SSE4.1

using namespace x265;

#if defined(_MSC_VER)
#pragma warning(disable: 4799) // MMX warning EMMS
#endif

#if defined(__INTEL_COMPILER) || defined(__GCC__)
#define HAVE_MMX 1
#elif defined(_MSC_VER) && defined(X86_64)
#define HAVE_MMX 0
#else
#define HAVE_MMX 1
#endif

namespace {
#if !HIGH_BIT_DEPTH
#if HAVE_MMX
template<int ly>
int sad_4(pixel * fenc, intptr_t fencstride, pixel * fref, intptr_t frefstride)
{
    assert((ly % 4) == 0);

    __m64 sum0 = _mm_setzero_si64();

    __m64 T00, T01, T02, T03;
    __m64 T10, T11, T12, T13;
    __m64 T20, T21, T22, T23;

    if ((ly % 16) == 0)
    {
        for (int i = 0; i < ly; i += 16)
        {
            T00 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 0) * fencstride));
            T01 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 1) * fencstride));
            T02 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 2) * fencstride));
            T03 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 3) * fencstride));

            T10 = _mm_cvtsi32_si64(*(int*)(fref + (i + 0) * frefstride));
            T11 = _mm_cvtsi32_si64(*(int*)(fref + (i + 1) * frefstride));
            T12 = _mm_cvtsi32_si64(*(int*)(fref + (i + 2) * frefstride));
            T13 = _mm_cvtsi32_si64(*(int*)(fref + (i + 3) * frefstride));

            T20 = _mm_sad_pu8(T00, T10);
            T21 = _mm_sad_pu8(T01, T11);
            T22 = _mm_sad_pu8(T02, T12);
            T23 = _mm_sad_pu8(T03, T13);

            sum0 = _mm_add_pi16(sum0, T20);
            sum0 = _mm_add_pi16(sum0, T21);
            sum0 = _mm_add_pi16(sum0, T22);
            sum0 = _mm_add_pi16(sum0, T23);

            T00 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 4) * fencstride));
            T01 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 5) * fencstride));
            T02 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 6) * fencstride));
            T03 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 7) * fencstride));

            T10 = _mm_cvtsi32_si64(*(int*)(fref + (i + 4) * frefstride));
            T11 = _mm_cvtsi32_si64(*(int*)(fref + (i + 5) * frefstride));
            T12 = _mm_cvtsi32_si64(*(int*)(fref + (i + 6) * frefstride));
            T13 = _mm_cvtsi32_si64(*(int*)(fref + (i + 7) * frefstride));

            T20 = _mm_sad_pu8(T00, T10);
            T21 = _mm_sad_pu8(T01, T11);
            T22 = _mm_sad_pu8(T02, T12);
            T23 = _mm_sad_pu8(T03, T13);

            sum0 = _mm_add_pi16(sum0, T20);
            sum0 = _mm_add_pi16(sum0, T21);
            sum0 = _mm_add_pi16(sum0, T22);
            sum0 = _mm_add_pi16(sum0, T23);

            T00 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 8) * fencstride));
            T01 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 9) * fencstride));
            T02 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 10) * fencstride));
            T03 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 11) * fencstride));

            T10 = _mm_cvtsi32_si64(*(int*)(fref + (i + 8) * frefstride));
            T11 = _mm_cvtsi32_si64(*(int*)(fref + (i + 9) * frefstride));
            T12 = _mm_cvtsi32_si64(*(int*)(fref + (i + 10) * frefstride));
            T13 = _mm_cvtsi32_si64(*(int*)(fref + (i + 11) * frefstride));

            T20 = _mm_sad_pu8(T00, T10);
            T21 = _mm_sad_pu8(T01, T11);
            T22 = _mm_sad_pu8(T02, T12);
            T23 = _mm_sad_pu8(T03, T13);

            sum0 = _mm_add_pi16(sum0, T20);
            sum0 = _mm_add_pi16(sum0, T21);
            sum0 = _mm_add_pi16(sum0, T22);
            sum0 = _mm_add_pi16(sum0, T23);

            T00 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 12) * fencstride));
            T01 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 13) * fencstride));
            T02 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 14) * fencstride));
            T03 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 15) * fencstride));

            T10 = _mm_cvtsi32_si64(*(int*)(fref + (i + 12) * frefstride));
            T11 = _mm_cvtsi32_si64(*(int*)(fref + (i + 13) * frefstride));
            T12 = _mm_cvtsi32_si64(*(int*)(fref + (i + 14) * frefstride));
            T13 = _mm_cvtsi32_si64(*(int*)(fref + (i + 15) * frefstride));

            T20 = _mm_sad_pu8(T00, T10);
            T21 = _mm_sad_pu8(T01, T11);
            T22 = _mm_sad_pu8(T02, T12);
            T23 = _mm_sad_pu8(T03, T13);

            sum0 = _mm_add_pi16(sum0, T20);
            sum0 = _mm_add_pi16(sum0, T21);
            sum0 = _mm_add_pi16(sum0, T22);
            sum0 = _mm_add_pi16(sum0, T23);
        }
    }
    else if ((ly % 8) == 0)
    {
        for (int i = 0; i < ly; i += 8)
        {
            T00 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 0) * fencstride));
            T01 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 1) * fencstride));
            T02 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 2) * fencstride));
            T03 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 3) * fencstride));

            T10 = _mm_cvtsi32_si64(*(int*)(fref + (i + 0) * frefstride));
            T11 = _mm_cvtsi32_si64(*(int*)(fref + (i + 1) * frefstride));
            T12 = _mm_cvtsi32_si64(*(int*)(fref + (i + 2) * frefstride));
            T13 = _mm_cvtsi32_si64(*(int*)(fref + (i + 3) * frefstride));

            T20 = _mm_sad_pu8(T00, T10);
            T21 = _mm_sad_pu8(T01, T11);
            T22 = _mm_sad_pu8(T02, T12);
            T23 = _mm_sad_pu8(T03, T13);

            sum0 = _mm_add_pi16(sum0, T20);
            sum0 = _mm_add_pi16(sum0, T21);
            sum0 = _mm_add_pi16(sum0, T22);
            sum0 = _mm_add_pi16(sum0, T23);

            T00 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 4) * fencstride));
            T01 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 5) * fencstride));
            T02 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 6) * fencstride));
            T03 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 7) * fencstride));

            T10 = _mm_cvtsi32_si64(*(int*)(fref + (i + 4) * frefstride));
            T11 = _mm_cvtsi32_si64(*(int*)(fref + (i + 5) * frefstride));
            T12 = _mm_cvtsi32_si64(*(int*)(fref + (i + 6) * frefstride));
            T13 = _mm_cvtsi32_si64(*(int*)(fref + (i + 7) * frefstride));

            T20 = _mm_sad_pu8(T00, T10);
            T21 = _mm_sad_pu8(T01, T11);
            T22 = _mm_sad_pu8(T02, T12);
            T23 = _mm_sad_pu8(T03, T13);

            sum0 = _mm_add_pi16(sum0, T20);
            sum0 = _mm_add_pi16(sum0, T21);
            sum0 = _mm_add_pi16(sum0, T22);
            sum0 = _mm_add_pi16(sum0, T23);
        }
    }
    else
    {
        for (int i = 0; i < ly; i += 4)
        {
            T00 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 0) * fencstride));
            T01 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 1) * fencstride));
            T02 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 2) * fencstride));
            T03 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 3) * fencstride));

            T10 = _mm_cvtsi32_si64(*(int*)(fref + (i + 0) * frefstride));
            T11 = _mm_cvtsi32_si64(*(int*)(fref + (i + 1) * frefstride));
            T12 = _mm_cvtsi32_si64(*(int*)(fref + (i + 2) * frefstride));
            T13 = _mm_cvtsi32_si64(*(int*)(fref + (i + 3) * frefstride));

            T20 = _mm_sad_pu8(T00, T10);
            T21 = _mm_sad_pu8(T01, T11);
            T22 = _mm_sad_pu8(T02, T12);
            T23 = _mm_sad_pu8(T03, T13);

            sum0 = _mm_add_pi16(sum0, T20);
            sum0 = _mm_add_pi16(sum0, T21);
            sum0 = _mm_add_pi16(sum0, T22);
            sum0 = _mm_add_pi16(sum0, T23);
        }
    }
    // 8 * 255 -> 11 bits x 8 -> 14 bits
    int sum = _m_to_int(sum0);
    return sum;
}

#else /* if HAVE_MMX */

template<int ly>
int sad_4(pixel * fenc, intptr_t fencstride, pixel * fref, intptr_t frefstride)
{
    assert((ly % 4) == 0);
    __m128i sum0 = _mm_setzero_si128();
    __m128i sum1 = _mm_setzero_si128();
    __m128i T00, T01, T02, T03;
    __m128i T10, T11, T12, T13;
    __m128i T20 = _mm_setzero_si128();

    if (ly == 4)
    {
        T00 = _mm_loadl_epi64((__m128i*)(fenc + (0) * fencstride));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (1) * fencstride));
        T01 = _mm_unpacklo_epi32(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (2) * fencstride));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (3) * fencstride));
        T03 = _mm_unpacklo_epi32(T02, T03);
        T03 = _mm_unpacklo_epi64(T01, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref + (0) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref + (1) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref + (3) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        T13 = _mm_unpacklo_epi64(T11, T13);

        T20 = _mm_sad_epu8(T03, T13);
        sum0 = _mm_add_epi32(sum0, T20);
    }
    else if (ly == 8)
    {
        T00 = _mm_loadl_epi64((__m128i*)(fenc + (0) * fencstride));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (1) * fencstride));
        T01 = _mm_unpacklo_epi32(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (2) * fencstride));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (3) * fencstride));
        T03 = _mm_unpacklo_epi32(T02, T03);
        T03 = _mm_unpacklo_epi64(T01, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref + (0) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref + (1) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref + (3) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        T13 = _mm_unpacklo_epi64(T11, T13);

        T20 = _mm_sad_epu8(T03, T13);
        sum0 = _mm_add_epi32(sum0, T20);

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (4) * fencstride));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (5) * fencstride));
        T01 = _mm_unpacklo_epi32(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (6) * fencstride));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (7) * fencstride));
        T03 = _mm_unpacklo_epi32(T02, T03);
        T03 = _mm_unpacklo_epi64(T01, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref + (5) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref + (7) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        T13 = _mm_unpacklo_epi64(T11, T13);

        T20 = _mm_sad_epu8(T03, T13);
        sum0 = _mm_add_epi32(sum0, T20);
    }
    else if (ly == 16)
    {
        T00 = _mm_loadl_epi64((__m128i*)(fenc + (0) * fencstride));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (1) * fencstride));
        T01 = _mm_unpacklo_epi32(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (2) * fencstride));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (3) * fencstride));
        T03 = _mm_unpacklo_epi32(T02, T03);
        T03 = _mm_unpacklo_epi64(T01, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref + (0) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref + (1) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref + (3) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        T13 = _mm_unpacklo_epi64(T11, T13);

        T20 = _mm_sad_epu8(T03, T13);
        sum0 = _mm_add_epi32(sum0, T20);

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (4) * fencstride));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (5) * fencstride));
        T01 = _mm_unpacklo_epi32(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (6) * fencstride));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (7) * fencstride));
        T03 = _mm_unpacklo_epi32(T02, T03);
        T03 = _mm_unpacklo_epi64(T01, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref + (5) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref + (7) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        T13 = _mm_unpacklo_epi64(T11, T13);

        T20 = _mm_sad_epu8(T03, T13);
        sum0 = _mm_add_epi32(sum0, T20);

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (8) * fencstride));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (9) * fencstride));
        T01 = _mm_unpacklo_epi32(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (10) * fencstride));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (11) * fencstride));
        T03 = _mm_unpacklo_epi32(T02, T03);
        T03 = _mm_unpacklo_epi64(T01, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref + (8) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref + (9) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref + (10) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref + (11) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        T13 = _mm_unpacklo_epi64(T11, T13);

        T20 = _mm_sad_epu8(T03, T13);
        sum0 = _mm_add_epi32(sum0, T20);

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (12) * fencstride));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (13) * fencstride));
        T01 = _mm_unpacklo_epi32(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (14) * fencstride));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (15) * fencstride));
        T03 = _mm_unpacklo_epi32(T02, T03);
        T03 = _mm_unpacklo_epi64(T01, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref + (12) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref + (13) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref + (14) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref + (15) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        T13 = _mm_unpacklo_epi64(T11, T13);

        T20 = _mm_sad_epu8(T03, T13);
        sum0 = _mm_add_epi32(sum0, T20);
    }
    else if ((ly % 8) == 0)
    {
        for (int i = 0; i < ly; i += 8)
        {
            T00 = _mm_loadl_epi64((__m128i*)(fenc + (i + 0) * fencstride));
            T01 = _mm_loadl_epi64((__m128i*)(fenc + (i + 1) * fencstride));
            T01 = _mm_unpacklo_epi32(T00, T01);
            T02 = _mm_loadl_epi64((__m128i*)(fenc + (i + 2) * fencstride));
            T03 = _mm_loadl_epi64((__m128i*)(fenc + (i + 3) * fencstride));
            T03 = _mm_unpacklo_epi32(T02, T03);
            T03 = _mm_unpacklo_epi64(T01, T03);

            T10 = _mm_loadl_epi64((__m128i*)(fref + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            T13 = _mm_unpacklo_epi64(T11, T13);

            T20 = _mm_sad_epu8(T03, T13);
            sum0 = _mm_add_epi32(sum0, T20);

            T00 = _mm_loadl_epi64((__m128i*)(fenc + (i + 4) * fencstride));
            T01 = _mm_loadl_epi64((__m128i*)(fenc + (i + 5) * fencstride));
            T01 = _mm_unpacklo_epi32(T00, T01);
            T02 = _mm_loadl_epi64((__m128i*)(fenc + (i + 6) * fencstride));
            T03 = _mm_loadl_epi64((__m128i*)(fenc + (i + 7) * fencstride));
            T03 = _mm_unpacklo_epi32(T02, T03);
            T03 = _mm_unpacklo_epi64(T01, T03);

            T10 = _mm_loadl_epi64((__m128i*)(fref + (i + 4) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref + (i + 5) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref + (i + 6) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref + (i + 7) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            T13 = _mm_unpacklo_epi64(T11, T13);

            T20 = _mm_sad_epu8(T03, T13);
            sum0 = _mm_add_epi32(sum0, T20);
        }
    }
    else
    {
        for (int i = 0; i < ly; i += 4)
        {
            T00 = _mm_loadl_epi64((__m128i*)(fenc + (i + 0) * fencstride));
            T01 = _mm_loadl_epi64((__m128i*)(fenc + (i + 1) * fencstride));
            T01 = _mm_unpacklo_epi32(T00, T01);
            T02 = _mm_loadl_epi64((__m128i*)(fenc + (i + 2) * fencstride));
            T03 = _mm_loadl_epi64((__m128i*)(fenc + (i + 3) * fencstride));
            T03 = _mm_unpacklo_epi32(T02, T03);
            T03 = _mm_unpacklo_epi64(T01, T03);

            T10 = _mm_loadl_epi64((__m128i*)(fref + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            T13 = _mm_unpacklo_epi64(T11, T13);

            T20 = _mm_sad_epu8(T03, T13);
            sum0 = _mm_add_epi32(sum0, T20);
        }
    }

    sum1 = _mm_shuffle_epi32(sum0, 2);
    sum0 = _mm_add_epi32(sum0, sum1);
    return _mm_cvtsi128_si32(sum0);
}

#endif /* if HAVE_MMX */

#if HAVE_MMX
template<int ly>
int sad_8(pixel * fenc, intptr_t fencstride, pixel * fref, intptr_t frefstride)
{
    assert((ly % 4) == 0);

    __m64 sum0 = _mm_setzero_si64();

    __m64 T00, T01, T02, T03;
    __m64 T10, T11, T12, T13;
    __m64 T20, T21, T22, T23;

    if ((ly % 16) == 0)
    {
        for (int i = 0; i < ly; i += 16)
        {
            T00 = (*(__m64*)(fenc + (i + 0) * fencstride));
            T01 = (*(__m64*)(fenc + (i + 1) * fencstride));
            T02 = (*(__m64*)(fenc + (i + 2) * fencstride));
            T03 = (*(__m64*)(fenc + (i + 3) * fencstride));

            T10 = (*(__m64*)(fref + (i + 0) * frefstride));
            T11 = (*(__m64*)(fref + (i + 1) * frefstride));
            T12 = (*(__m64*)(fref + (i + 2) * frefstride));
            T13 = (*(__m64*)(fref + (i + 3) * frefstride));

            T20 = _mm_sad_pu8(T00, T10);
            T21 = _mm_sad_pu8(T01, T11);
            T22 = _mm_sad_pu8(T02, T12);
            T23 = _mm_sad_pu8(T03, T13);

            sum0 = _mm_add_pi16(sum0, T20);
            sum0 = _mm_add_pi16(sum0, T21);
            sum0 = _mm_add_pi16(sum0, T22);
            sum0 = _mm_add_pi16(sum0, T23);

            T00 = (*(__m64*)(fenc + (i + 4) * fencstride));
            T01 = (*(__m64*)(fenc + (i + 5) * fencstride));
            T02 = (*(__m64*)(fenc + (i + 6) * fencstride));
            T03 = (*(__m64*)(fenc + (i + 7) * fencstride));

            T10 = (*(__m64*)(fref + (i + 4) * frefstride));
            T11 = (*(__m64*)(fref + (i + 5) * frefstride));
            T12 = (*(__m64*)(fref + (i + 6) * frefstride));
            T13 = (*(__m64*)(fref + (i + 7) * frefstride));

            T20 = _mm_sad_pu8(T00, T10);
            T21 = _mm_sad_pu8(T01, T11);
            T22 = _mm_sad_pu8(T02, T12);
            T23 = _mm_sad_pu8(T03, T13);

            sum0 = _mm_add_pi16(sum0, T20);
            sum0 = _mm_add_pi16(sum0, T21);
            sum0 = _mm_add_pi16(sum0, T22);
            sum0 = _mm_add_pi16(sum0, T23);

            T00 = (*(__m64*)(fenc + (i + 8) * fencstride));
            T01 = (*(__m64*)(fenc + (i + 9) * fencstride));
            T02 = (*(__m64*)(fenc + (i + 10) * fencstride));
            T03 = (*(__m64*)(fenc + (i + 11) * fencstride));

            T10 = (*(__m64*)(fref + (i + 8) * frefstride));
            T11 = (*(__m64*)(fref + (i + 9) * frefstride));
            T12 = (*(__m64*)(fref + (i + 10) * frefstride));
            T13 = (*(__m64*)(fref + (i + 11) * frefstride));

            T20 = _mm_sad_pu8(T00, T10);
            T21 = _mm_sad_pu8(T01, T11);
            T22 = _mm_sad_pu8(T02, T12);
            T23 = _mm_sad_pu8(T03, T13);

            sum0 = _mm_add_pi16(sum0, T20);
            sum0 = _mm_add_pi16(sum0, T21);
            sum0 = _mm_add_pi16(sum0, T22);
            sum0 = _mm_add_pi16(sum0, T23);

            T00 = (*(__m64*)(fenc + (i + 12) * fencstride));
            T01 = (*(__m64*)(fenc + (i + 13) * fencstride));
            T02 = (*(__m64*)(fenc + (i + 14) * fencstride));
            T03 = (*(__m64*)(fenc + (i + 15) * fencstride));

            T10 = (*(__m64*)(fref + (i + 12) * frefstride));
            T11 = (*(__m64*)(fref + (i + 13) * frefstride));
            T12 = (*(__m64*)(fref + (i + 14) * frefstride));
            T13 = (*(__m64*)(fref + (i + 15) * frefstride));

            T20 = _mm_sad_pu8(T00, T10);
            T21 = _mm_sad_pu8(T01, T11);
            T22 = _mm_sad_pu8(T02, T12);
            T23 = _mm_sad_pu8(T03, T13);

            sum0 = _mm_add_pi16(sum0, T20);
            sum0 = _mm_add_pi16(sum0, T21);
            sum0 = _mm_add_pi16(sum0, T22);
            sum0 = _mm_add_pi16(sum0, T23);
        }
    }
    else if ((ly % 8) == 0)
    {
        for (int i = 0; i < ly; i += 8)
        {
            T00 = (*(__m64*)(fenc + (i + 0) * fencstride));
            T01 = (*(__m64*)(fenc + (i + 1) * fencstride));
            T02 = (*(__m64*)(fenc + (i + 2) * fencstride));
            T03 = (*(__m64*)(fenc + (i + 3) * fencstride));

            T10 = (*(__m64*)(fref + (i + 0) * frefstride));
            T11 = (*(__m64*)(fref + (i + 1) * frefstride));
            T12 = (*(__m64*)(fref + (i + 2) * frefstride));
            T13 = (*(__m64*)(fref + (i + 3) * frefstride));

            T20 = _mm_sad_pu8(T00, T10);
            T21 = _mm_sad_pu8(T01, T11);
            T22 = _mm_sad_pu8(T02, T12);
            T23 = _mm_sad_pu8(T03, T13);

            sum0 = _mm_add_pi16(sum0, T20);
            sum0 = _mm_add_pi16(sum0, T21);
            sum0 = _mm_add_pi16(sum0, T22);
            sum0 = _mm_add_pi16(sum0, T23);

            T00 = (*(__m64*)(fenc + (i + 4) * fencstride));
            T01 = (*(__m64*)(fenc + (i + 5) * fencstride));
            T02 = (*(__m64*)(fenc + (i + 6) * fencstride));
            T03 = (*(__m64*)(fenc + (i + 7) * fencstride));

            T10 = (*(__m64*)(fref + (i + 4) * frefstride));
            T11 = (*(__m64*)(fref + (i + 5) * frefstride));
            T12 = (*(__m64*)(fref + (i + 6) * frefstride));
            T13 = (*(__m64*)(fref + (i + 7) * frefstride));

            T20 = _mm_sad_pu8(T00, T10);
            T21 = _mm_sad_pu8(T01, T11);
            T22 = _mm_sad_pu8(T02, T12);
            T23 = _mm_sad_pu8(T03, T13);

            sum0 = _mm_add_pi16(sum0, T20);
            sum0 = _mm_add_pi16(sum0, T21);
            sum0 = _mm_add_pi16(sum0, T22);
            sum0 = _mm_add_pi16(sum0, T23);
        }
    }
    else
    {
        for (int i = 0; i < ly; i += 4)
        {
            T00 = (*(__m64*)(fenc + (i + 0) * fencstride));
            T01 = (*(__m64*)(fenc + (i + 1) * fencstride));
            T02 = (*(__m64*)(fenc + (i + 2) * fencstride));
            T03 = (*(__m64*)(fenc + (i + 3) * fencstride));

            T10 = (*(__m64*)(fref + (i + 0) * frefstride));
            T11 = (*(__m64*)(fref + (i + 1) * frefstride));
            T12 = (*(__m64*)(fref + (i + 2) * frefstride));
            T13 = (*(__m64*)(fref + (i + 3) * frefstride));

            T20 = _mm_sad_pu8(T00, T10);
            T21 = _mm_sad_pu8(T01, T11);
            T22 = _mm_sad_pu8(T02, T12);
            T23 = _mm_sad_pu8(T03, T13);

            sum0 = _mm_add_pi16(sum0, T20);
            sum0 = _mm_add_pi16(sum0, T21);
            sum0 = _mm_add_pi16(sum0, T22);
            sum0 = _mm_add_pi16(sum0, T23);
        }
    }
    // 8 * 255 -> 11 bits x 8 -> 14 bits
    int sum = _m_to_int(sum0);
    return sum;
}

#else /* if HAVE_MMX */

template<int ly>
int sad_8(pixel * fenc, intptr_t fencstride, pixel * fref, intptr_t frefstride)
{
    assert((ly % 4) == 0);
    __m128i sum0 = _mm_setzero_si128();
    __m128i sum1 = _mm_setzero_si128();
    __m128i T00, T01, T02, T03;
    __m128i T10, T11, T12, T13;
    __m128i T20, T21;

    if (ly == 4)
    {
        T00 = _mm_loadl_epi64((__m128i*)(fenc + (0) * fencstride));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (1) * fencstride));
        T01 = _mm_unpacklo_epi64(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (2) * fencstride));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (3) * fencstride));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref + (0) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref + (1) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);
        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi32(sum0, T20);
        sum1 = _mm_add_epi32(sum1, T21);
    }
    else if (ly == 8)
    {
        T00 = _mm_loadl_epi64((__m128i*)(fenc + (0) * fencstride));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (1) * fencstride));
        T01 = _mm_unpacklo_epi64(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (2) * fencstride));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (3) * fencstride));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref + (0) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref + (1) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);
        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi32(sum0, T20);
        sum1 = _mm_add_epi32(sum1, T21);

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (4) * fencstride));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (5) * fencstride));
        T01 = _mm_unpacklo_epi64(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (6) * fencstride));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (7) * fencstride));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref + (5) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref + (7) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);
        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi32(sum0, T20);
        sum1 = _mm_add_epi32(sum1, T21);
    }
    else if (ly == 16)
    {
        T00 = _mm_loadl_epi64((__m128i*)(fenc + (0) * fencstride));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (1) * fencstride));
        T01 = _mm_unpacklo_epi64(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (2) * fencstride));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (3) * fencstride));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref + (0) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref + (1) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);
        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi32(sum0, T20);
        sum1 = _mm_add_epi32(sum1, T21);

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (4) * fencstride));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (5) * fencstride));
        T01 = _mm_unpacklo_epi64(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (6) * fencstride));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (7) * fencstride));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref + (5) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref + (7) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);
        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi32(sum0, T20);
        sum1 = _mm_add_epi32(sum1, T21);

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (8) * fencstride));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (9) * fencstride));
        T01 = _mm_unpacklo_epi64(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (10) * fencstride));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (11) * fencstride));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref + (8) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref + (9) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref + (10) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref + (11) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);
        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi32(sum0, T20);
        sum1 = _mm_add_epi32(sum1, T21);

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (12) * fencstride));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (13) * fencstride));
        T01 = _mm_unpacklo_epi64(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (14) * fencstride));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (15) * fencstride));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref + (12) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref + (13) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref + (14) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref + (15) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);
        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi32(sum0, T20);
        sum1 = _mm_add_epi32(sum1, T21);
    }
    else if ((ly % 8) == 0)
    {
        for (int i = 0; i < ly; i += 8)
        {
            T00 = _mm_loadl_epi64((__m128i*)(fenc + (i + 0) * fencstride));
            T01 = _mm_loadl_epi64((__m128i*)(fenc + (i + 1) * fencstride));
            T01 = _mm_unpacklo_epi64(T00, T01);
            T02 = _mm_loadl_epi64((__m128i*)(fenc + (i + 2) * fencstride));
            T03 = _mm_loadl_epi64((__m128i*)(fenc + (i + 3) * fencstride));
            T03 = _mm_unpacklo_epi64(T02, T03);

            T10 = _mm_loadl_epi64((__m128i*)(fref + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi64(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi64(T12, T13);
            T20 = _mm_sad_epu8(T01, T11);
            T21 = _mm_sad_epu8(T03, T13);

            sum0 = _mm_add_epi32(sum0, T20);
            sum1 = _mm_add_epi32(sum1, T21);

            T00 = _mm_loadl_epi64((__m128i*)(fenc + (i + 4) * fencstride));
            T01 = _mm_loadl_epi64((__m128i*)(fenc + (i + 5) * fencstride));
            T01 = _mm_unpacklo_epi64(T00, T01);
            T02 = _mm_loadl_epi64((__m128i*)(fenc + (i + 6) * fencstride));
            T03 = _mm_loadl_epi64((__m128i*)(fenc + (i + 7) * fencstride));
            T03 = _mm_unpacklo_epi64(T02, T03);

            T10 = _mm_loadl_epi64((__m128i*)(fref + (i + 4) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref + (i + 5) * frefstride));
            T11 = _mm_unpacklo_epi64(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref + (i + 6) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref + (i + 7) * frefstride));
            T13 = _mm_unpacklo_epi64(T12, T13);
            T20 = _mm_sad_epu8(T01, T11);
            T21 = _mm_sad_epu8(T03, T13);

            sum0 = _mm_add_epi32(sum0, T20);
            sum1 = _mm_add_epi32(sum1, T21);
        }
    }
    else
    {
        for (int i = 0; i < ly; i += 4)
        {
            T00 = _mm_loadl_epi64((__m128i*)(fenc + (i + 0) * fencstride));
            T01 = _mm_loadl_epi64((__m128i*)(fenc + (i + 1) * fencstride));
            T01 = _mm_unpacklo_epi64(T00, T01);
            T02 = _mm_loadl_epi64((__m128i*)(fenc + (i + 2) * fencstride));
            T03 = _mm_loadl_epi64((__m128i*)(fenc + (i + 3) * fencstride));
            T03 = _mm_unpacklo_epi64(T02, T03);

            T10 = _mm_loadl_epi64((__m128i*)(fref + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi64(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi64(T12, T13);
            T20 = _mm_sad_epu8(T01, T11);
            T21 = _mm_sad_epu8(T03, T13);

            sum0 = _mm_add_epi32(sum0, T20);
            sum1 = _mm_add_epi32(sum1, T21);
        }
    }
    // [0 x 0 x]
    sum0 = _mm_add_epi32(sum0, sum1);
    sum1 = _mm_shuffle_epi32(sum0, 2);
    sum0 = _mm_add_epi32(sum0, sum1);
    return _mm_cvtsi128_si32(sum0);
}

#endif /* if HAVE_MMX */

template<int ly>
int sad_12(pixel *fenc, intptr_t fencstride, pixel *fref, intptr_t frefstride)
{
    assert((ly % 4) == 0);
    __m128i sum0 = _mm_setzero_si128();
    __m128i sum1 = _mm_setzero_si128();

    __m128i mask = _mm_set_epi32(0x00000000, 0xffffffff, 0xffffffff, 0xffffffff);

    if (ly == 4)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T00 = _mm_and_si128(T00, mask);
        T01 = _mm_load_si128((__m128i*)(fenc + fencstride));
        T01 = _mm_and_si128(T01, mask);
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * fencstride));
        T02 = _mm_and_si128(T02, mask);
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * fencstride));
        T03 = _mm_and_si128(T03, mask);

        T10 = _mm_load_si128((__m128i*)(fref));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_load_si128((__m128i*)(fref + frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_load_si128((__m128i*)(fref + (2) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_load_si128((__m128i*)(fref + (3) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20,T22);
    }
    else if (ly == 8)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T00 = _mm_and_si128(T00, mask);
        T01 = _mm_load_si128((__m128i*)(fenc + fencstride));
        T01 = _mm_and_si128(T01, mask);
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * fencstride));
        T02 = _mm_and_si128(T02, mask);
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * fencstride));
        T03 = _mm_and_si128(T03, mask);

        T10 = _mm_load_si128((__m128i*)(fref));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_load_si128((__m128i*)(fref + frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_load_si128((__m128i*)(fref + (2) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_load_si128((__m128i*)(fref + (3) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20,T22);

        T00 = _mm_load_si128((__m128i*)(fenc + (4) * fencstride));
        T00 = _mm_and_si128(T00, mask);
        T01 = _mm_load_si128((__m128i*)(fenc + (5) * fencstride));
        T01 = _mm_and_si128(T01, mask);
        T02 = _mm_load_si128((__m128i*)(fenc + (6) * fencstride));
        T02 = _mm_and_si128(T02, mask);
        T03 = _mm_load_si128((__m128i*)(fenc + (7) * fencstride));
        T03 = _mm_and_si128(T03, mask);

        T10 = _mm_load_si128((__m128i*)(fref + (4) * frefstride));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_load_si128((__m128i*)(fref + (5) * frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_load_si128((__m128i*)(fref + (6) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_load_si128((__m128i*)(fref + (7) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);
    }
    else if (ly == 16)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T00 = _mm_and_si128(T00, mask);
        T01 = _mm_load_si128((__m128i*)(fenc + fencstride));
        T01 = _mm_and_si128(T01, mask);
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * fencstride));
        T02 = _mm_and_si128(T02, mask);
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * fencstride));
        T03 = _mm_and_si128(T03, mask);

        T10 = _mm_load_si128((__m128i*)(fref));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_load_si128((__m128i*)(fref + frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_load_si128((__m128i*)(fref + (2) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_load_si128((__m128i*)(fref + (3) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20,T22);

        T00 = _mm_load_si128((__m128i*)(fenc + (4) * fencstride));
        T00 = _mm_and_si128(T00, mask);
        T01 = _mm_load_si128((__m128i*)(fenc + (5) * fencstride));
        T01 = _mm_and_si128(T01, mask);
        T02 = _mm_load_si128((__m128i*)(fenc + (6) * fencstride));
        T02 = _mm_and_si128(T02, mask);
        T03 = _mm_load_si128((__m128i*)(fenc + (7) * fencstride));
        T03 = _mm_and_si128(T03, mask);

        T10 = _mm_load_si128((__m128i*)(fref + (4) * frefstride));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_load_si128((__m128i*)(fref + (5) * frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_load_si128((__m128i*)(fref + (6) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_load_si128((__m128i*)(fref + (7) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (8) * fencstride));
        T00 = _mm_and_si128(T00, mask);
        T01 = _mm_load_si128((__m128i*)(fenc + (9) * fencstride));
        T01 = _mm_and_si128(T01, mask);
        T02 = _mm_load_si128((__m128i*)(fenc + (10) * fencstride));
        T02 = _mm_and_si128(T02, mask);
        T03 = _mm_load_si128((__m128i*)(fenc + (11) * fencstride));
        T03 = _mm_and_si128(T03, mask);

        T10 = _mm_load_si128((__m128i*)(fref + (8) * frefstride));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_load_si128((__m128i*)(fref + (9) * frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_load_si128((__m128i*)(fref + (10) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_load_si128((__m128i*)(fref + (11) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (12) * fencstride));
        T00 = _mm_and_si128(T00, mask);
        T01 = _mm_load_si128((__m128i*)(fenc + (13) * fencstride));
        T01 = _mm_and_si128(T01, mask);
        T02 = _mm_load_si128((__m128i*)(fenc + (14) * fencstride));
        T02 = _mm_and_si128(T02, mask);
        T03 = _mm_load_si128((__m128i*)(fenc + (15) * fencstride));
        T03 = _mm_and_si128(T03, mask);

        T10 = _mm_load_si128((__m128i*)(fref + (12) * frefstride));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_load_si128((__m128i*)(fref + (13) * frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_load_si128((__m128i*)(fref + (14) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_load_si128((__m128i*)(fref + (15) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);
    }
    else if ((ly % 8) == 0)
    {
        for (int i = 0; i < ly; i += 8)
        {
            __m128i T00, T01, T02, T03;
            __m128i T10, T11, T12, T13;
            __m128i T20, T21, T22, T23;

            T00 = _mm_load_si128((__m128i*)(fenc + (i) * fencstride));
            T00 = _mm_and_si128(T00, mask);
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 1) * fencstride));
            T01 = _mm_and_si128(T01, mask);
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 2) * fencstride));
            T02 = _mm_and_si128(T02, mask);
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 3) * fencstride));
            T03 = _mm_and_si128(T03, mask);

            T10 = _mm_load_si128((__m128i*)(fref + (i) * frefstride));
            T10 = _mm_and_si128(T10, mask);
            T11 = _mm_load_si128((__m128i*)(fref + (i + 1) * frefstride));
            T11 = _mm_and_si128(T11, mask);
            T12 = _mm_load_si128((__m128i*)(fref + (i + 2) * frefstride));
            T12 = _mm_and_si128(T12, mask);
            T13 = _mm_load_si128((__m128i*)(fref + (i + 3) * frefstride));
            T13 = _mm_and_si128(T13, mask);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            sum0 = _mm_add_epi16(sum0, T20);
            sum0 = _mm_add_epi16(sum0, T21);
            sum0 = _mm_add_epi16(sum0, T22);
            sum0 = _mm_add_epi16(sum0, T23);

            T00 = _mm_load_si128((__m128i*)(fenc + (i + 4) * fencstride));
            T00 = _mm_and_si128(T00, mask);
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 5) * fencstride));
            T01 = _mm_and_si128(T01, mask);
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 6) * fencstride));
            T02 = _mm_and_si128(T02, mask);
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 7) * fencstride));
            T03 = _mm_and_si128(T03, mask);

            T10 = _mm_load_si128((__m128i*)(fref + (i + 4) * frefstride));
            T10 = _mm_and_si128(T10, mask);
            T11 = _mm_load_si128((__m128i*)(fref + (i + 5) * frefstride));
            T11 = _mm_and_si128(T11, mask);
            T12 = _mm_load_si128((__m128i*)(fref + (i + 6) * frefstride));
            T12 = _mm_and_si128(T12, mask);
            T13 = _mm_load_si128((__m128i*)(fref + (i + 7) * frefstride));
            T13 = _mm_and_si128(T13, mask);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            sum0 = _mm_add_epi16(sum0, T20);
            sum0 = _mm_add_epi16(sum0, T21);
            sum0 = _mm_add_epi16(sum0, T22);
            sum0 = _mm_add_epi16(sum0, T23);
        }
    }
    else
    {
        for (int i = 0; i < ly; i += 4)
        {
            __m128i T00, T01, T02, T03;
            __m128i T10, T11, T12, T13;
            __m128i T20, T21, T22, T23;

            T00 = _mm_load_si128((__m128i*)(fenc + (i) * fencstride));
            T00 = _mm_and_si128(T00, mask);
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 1) * fencstride));
            T01 = _mm_and_si128(T01, mask);
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 2) * fencstride));
            T02 = _mm_and_si128(T02, mask);
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 3) * fencstride));
            T03 = _mm_and_si128(T03, mask);

            T10 = _mm_load_si128((__m128i*)(fref + (i) * frefstride));
            T10 = _mm_and_si128(T10, mask);
            T11 = _mm_load_si128((__m128i*)(fref + (i + 1) * frefstride));
            T11 = _mm_and_si128(T11, mask);
            T12 = _mm_load_si128((__m128i*)(fref + (i + 2) * frefstride));
            T12 = _mm_and_si128(T12, mask);
            T13 = _mm_load_si128((__m128i*)(fref + (i + 3) * frefstride));
            T13 = _mm_and_si128(T13, mask);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            sum0 = _mm_add_epi16(sum0, T20);
            sum0 = _mm_add_epi16(sum0, T21);
            sum0 = _mm_add_epi16(sum0, T22);
            sum0 = _mm_add_epi16(sum0, T23);
        }
    }
    sum1 = _mm_shuffle_epi32(sum0, 2);
    sum0 = _mm_add_epi32(sum0, sum1);

    return _mm_cvtsi128_si32(sum0);
}

template<int ly>
int sad_16(pixel * fenc, intptr_t fencstride, pixel * fref, intptr_t frefstride)
{
    assert((ly % 4) == 0);

    __m128i sum0 = _mm_setzero_si128();
    __m128i sum1 = _mm_setzero_si128();
    __m128i T00, T01, T02, T03;
    __m128i T10, T11, T12, T13;
    __m128i T20, T21, T22, T23;

    if (ly == 4)
    {
        T00 = _mm_load_si128((__m128i*)(fenc + (0) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + (1) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + (1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);
    }
    else if (ly == 8)
    {
        T00 = _mm_load_si128((__m128i*)(fenc + (0) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + (1) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + (1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (4) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + (5) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + (6) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + (7) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);
    }
    else if (ly == 16)
    {
        T00 = _mm_load_si128((__m128i*)(fenc + (0) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + (1) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + (1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (4) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + (5) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + (6) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + (7) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (8) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + (9) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + (10) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + (11) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (12) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + (13) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + (14) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + (15) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);
    }
    else if ((ly % 8) == 0)
    {
        for (int i = 0; i < ly; i += 8)
        {
            T00 = _mm_load_si128((__m128i*)(fenc + (i + 0) * fencstride));
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 1) * fencstride));
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 2) * fencstride));
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 3) * fencstride));

            T10 = _mm_loadu_si128((__m128i*)(fref + (i + 0) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref + (i + 3) * frefstride));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            sum0 = _mm_add_epi16(sum0, T20);
            sum0 = _mm_add_epi16(sum0, T21);
            sum0 = _mm_add_epi16(sum0, T22);
            sum0 = _mm_add_epi16(sum0, T23);

            T00 = _mm_load_si128((__m128i*)(fenc + (i + 4) * fencstride));
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 5) * fencstride));
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 6) * fencstride));
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 7) * fencstride));

            T10 = _mm_loadu_si128((__m128i*)(fref + (i + 4) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref + (i + 5) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref + (i + 6) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref + (i + 7) * frefstride));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            sum0 = _mm_add_epi16(sum0, T20);
            sum0 = _mm_add_epi16(sum0, T21);
            sum0 = _mm_add_epi16(sum0, T22);
            sum0 = _mm_add_epi16(sum0, T23);
        }
    }
    else
    {
        for (int i = 0; i < ly; i += 4)
        {
            T00 = _mm_load_si128((__m128i*)(fenc + (i + 0) * fencstride));
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 1) * fencstride));
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 2) * fencstride));
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 3) * fencstride));

            T10 = _mm_loadu_si128((__m128i*)(fref + (i + 0) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref + (i + 3) * frefstride));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            sum0 = _mm_add_epi16(sum0, T20);
            sum0 = _mm_add_epi16(sum0, T21);
            sum0 = _mm_add_epi16(sum0, T22);
            sum0 = _mm_add_epi16(sum0, T23);
        }
    }

    sum1 = _mm_shuffle_epi32(sum0, 2);
    sum0 = _mm_add_epi32(sum0, sum1);

    return _mm_cvtsi128_si32(sum0);
}

template<int ly>
int sad_24(pixel *fenc, intptr_t fencstride, pixel *fref, intptr_t frefstride)
{
    assert((ly % 4) == 0);
    __m128i sum0 = _mm_setzero_si128();
    __m128i sum1 = _mm_setzero_si128();

    if (ly == 4)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T01 = _mm_load_si128((__m128i*)(fenc + fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * fencstride));

        T10 = _mm_load_si128((__m128i*)(fref));
        T11 = _mm_load_si128((__m128i*)(fref + frefstride));
        T12 = _mm_load_si128((__m128i*)(fref + (2) * frefstride));
        T13 = _mm_load_si128((__m128i*)(fref + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        T00 = _mm_loadl_epi64((__m128i*)(fenc + 16));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (fencstride + 16)));
        T01 = _mm_unpacklo_epi64(T00, T01);

        T02 = _mm_loadl_epi64((__m128i*)(fenc + ((2) * fencstride) + 16));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + ((3) * fencstride) + 16));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref + ((0) * frefstride) + 16));
        T11 = _mm_loadl_epi64((__m128i*)(fref + ((1) * frefstride) + 16));
        T11 = _mm_unpacklo_epi64(T10, T11);

        T12 = _mm_loadl_epi64((__m128i*)(fref + ((2) * frefstride) + 16));
        T13 = _mm_loadl_epi64((__m128i*)(fref + ((3) * frefstride) + 16));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_setzero_si128();
        T21 = _mm_setzero_si128();

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi32(sum0, T20);
        sum0 = _mm_add_epi32(sum0, T21);
    }
    else if (ly == 8)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T01 = _mm_load_si128((__m128i*)(fenc + fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * fencstride));

        T10 = _mm_load_si128((__m128i*)(fref));
        T11 = _mm_load_si128((__m128i*)(fref + frefstride));
        T12 = _mm_load_si128((__m128i*)(fref + (2) * frefstride));
        T13 = _mm_load_si128((__m128i*)(fref + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        T00 = _mm_loadl_epi64((__m128i*)(fenc + 16));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (fencstride + 16)));
        T01 = _mm_unpacklo_epi64(T00, T01);

        T02 = _mm_loadl_epi64((__m128i*)(fenc + ((2) * fencstride) + 16));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + ((3) * fencstride) + 16));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref + ((0) * frefstride) + 16));
        T11 = _mm_loadl_epi64((__m128i*)(fref + ((1) * frefstride) + 16));
        T11 = _mm_unpacklo_epi64(T10, T11);

        T12 = _mm_loadl_epi64((__m128i*)(fref + ((2) * frefstride) + 16));
        T13 = _mm_loadl_epi64((__m128i*)(fref + ((3) * frefstride) + 16));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_setzero_si128();
        T21 = _mm_setzero_si128();

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi32(sum0, T20);
        sum0 = _mm_add_epi32(sum0, T21);

        T00 = _mm_load_si128((__m128i*)(fenc + (4) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + (5) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + (6) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + (7) * fencstride));

        T10 = _mm_load_si128((__m128i*)(fref + (4) * frefstride));
        T11 = _mm_load_si128((__m128i*)(fref + (5) * frefstride));
        T12 = _mm_load_si128((__m128i*)(fref + (6) * frefstride));
        T13 = _mm_load_si128((__m128i*)(fref + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);

        T00 = _mm_loadl_epi64((__m128i*)(fenc + ((4) * fencstride) + 16));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + ((5) * fencstride) + 16));
        T01 = _mm_unpacklo_epi64(T00, T01);

        T02 = _mm_loadl_epi64((__m128i*)(fenc + ((6) * fencstride) + 16));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + ((7) * fencstride) + 16));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref + ((4) * frefstride) + 16));
        T11 = _mm_loadl_epi64((__m128i*)(fref + ((5) * frefstride) + 16));
        T11 = _mm_unpacklo_epi64(T10, T11);

        T12 = _mm_loadl_epi64((__m128i*)(fref + ((6) * frefstride) + 16));
        T13 = _mm_loadl_epi64((__m128i*)(fref + ((7) * frefstride) + 16));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_setzero_si128();
        T21 = _mm_setzero_si128();

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi32(sum0, T20);
        sum0 = _mm_add_epi32(sum0, T21);
    }
    else if (ly == 16)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T01 = _mm_load_si128((__m128i*)(fenc + fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * fencstride));

        T10 = _mm_load_si128((__m128i*)(fref));
        T11 = _mm_load_si128((__m128i*)(fref + frefstride));
        T12 = _mm_load_si128((__m128i*)(fref + (2) * frefstride));
        T13 = _mm_load_si128((__m128i*)(fref + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        T00 = _mm_loadl_epi64((__m128i*)(fenc + 16));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + fencstride + 16));
        T01 = _mm_unpacklo_epi64(T00, T01);

        T02 = _mm_loadl_epi64((__m128i*)(fenc + ((2) * fencstride) + 16));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + ((3) * fencstride) + 16));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref + 16));
        T11 = _mm_loadl_epi64((__m128i*)(fref + (frefstride + 16)));
        T11 = _mm_unpacklo_epi64(T10, T11);

        T12 = _mm_loadl_epi64((__m128i*)(fref + ((2) * frefstride) + 16));
        T13 = _mm_loadl_epi64((__m128i*)(fref + ((3) * frefstride) + 16));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_setzero_si128();
        T21 = _mm_setzero_si128();

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi32(sum0, T20);
        sum0 = _mm_add_epi32(sum0, T21);

        T00 = _mm_load_si128((__m128i*)(fenc + (4) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + (5) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + (6) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + (7) * fencstride));

        T10 = _mm_load_si128((__m128i*)(fref + (4) * frefstride));
        T11 = _mm_load_si128((__m128i*)(fref + (5) * frefstride));
        T12 = _mm_load_si128((__m128i*)(fref + (6) * frefstride));
        T13 = _mm_load_si128((__m128i*)(fref + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);

        T00 = _mm_loadl_epi64((__m128i*)(fenc + ((4) * fencstride) + 16));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + ((5) * fencstride) + 16));
        T01 = _mm_unpacklo_epi64(T00, T01);

        T02 = _mm_loadl_epi64((__m128i*)(fenc + ((6) * fencstride) + 16));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + ((7) * fencstride) + 16));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref + ((4) * frefstride) + 16));
        T11 = _mm_loadl_epi64((__m128i*)(fref + ((5) * frefstride) + 16));
        T11 = _mm_unpacklo_epi64(T10, T11);

        T12 = _mm_loadl_epi64((__m128i*)(fref + ((6) * frefstride) + 16));
        T13 = _mm_loadl_epi64((__m128i*)(fref + ((7) * frefstride) + 16));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_setzero_si128();
        T21 = _mm_setzero_si128();

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi32(sum0, T20);
        sum0 = _mm_add_epi32(sum0, T21);

        T00 = _mm_load_si128((__m128i*)(fenc + (8) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + (9) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + (10) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + (11) * fencstride));

        T10 = _mm_load_si128((__m128i*)(fref + (8) * frefstride));
        T11 = _mm_load_si128((__m128i*)(fref + (9) * frefstride));
        T12 = _mm_load_si128((__m128i*)(fref + (10) * frefstride));
        T13 = _mm_load_si128((__m128i*)(fref + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);

        T00 = _mm_loadl_epi64((__m128i*)(fenc + ((8) * fencstride) + 16));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + ((9) * fencstride) + 16));
        T01 = _mm_unpacklo_epi64(T00, T01);

        T02 = _mm_loadl_epi64((__m128i*)(fenc + ((10) * fencstride) + 16));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + ((11) * fencstride) + 16));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref + ((8) * frefstride) + 16));
        T11 = _mm_loadl_epi64((__m128i*)(fref + ((9) * frefstride) + 16));
        T11 = _mm_unpacklo_epi64(T10, T11);

        T12 = _mm_loadl_epi64((__m128i*)(fref + ((10) * frefstride) + 16));
        T13 = _mm_loadl_epi64((__m128i*)(fref + ((11) * frefstride) + 16));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_setzero_si128();
        T21 = _mm_setzero_si128();

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi32(sum0, T20);
        sum0 = _mm_add_epi32(sum0, T21);

        T00 = _mm_load_si128((__m128i*)(fenc + (12) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + (13) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + (14) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + (15) * fencstride));

        T10 = _mm_load_si128((__m128i*)(fref + (12) * frefstride));
        T11 = _mm_load_si128((__m128i*)(fref + (13) * frefstride));
        T12 = _mm_load_si128((__m128i*)(fref + (14) * frefstride));
        T13 = _mm_load_si128((__m128i*)(fref + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);

        T00 = _mm_loadl_epi64((__m128i*)(fenc + ((12) * fencstride) + 16));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + ((13) * fencstride) + 16));
        T01 = _mm_unpacklo_epi64(T00, T01);

        T02 = _mm_loadl_epi64((__m128i*)(fenc + ((14) * fencstride) + 16));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + ((15) * fencstride) + 16));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref + ((12) * frefstride) + 16));
        T11 = _mm_loadl_epi64((__m128i*)(fref + ((13) * frefstride) + 16));
        T11 = _mm_unpacklo_epi64(T10, T11);

        T12 = _mm_loadl_epi64((__m128i*)(fref + ((14) * frefstride) + 16));
        T13 = _mm_loadl_epi64((__m128i*)(fref + ((15) * frefstride) + 16));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_setzero_si128();
        T21 = _mm_setzero_si128();

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi32(sum0, T20);
        sum0 = _mm_add_epi32(sum0, T21);
    }
    else if ((ly % 8) == 0)
    {
        for (int i = 0; i < ly; i += 8)
        {
            __m128i T00, T01, T02, T03;
            __m128i T10, T11, T12, T13;
            __m128i T20, T21, T22, T23;

            T00 = _mm_load_si128((__m128i*)(fenc + (i) * fencstride));
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 1) * fencstride));
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 2) * fencstride));
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 3) * fencstride));

            T10 = _mm_load_si128((__m128i*)(fref + (i) * frefstride));
            T11 = _mm_load_si128((__m128i*)(fref + (i + 1) * frefstride));
            T12 = _mm_load_si128((__m128i*)(fref + (i + 2) * frefstride));
            T13 = _mm_load_si128((__m128i*)(fref + (i + 3) * frefstride));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            sum0 = _mm_add_epi32(sum0, T20);
            sum0 = _mm_add_epi32(sum0, T21);
            sum0 = _mm_add_epi32(sum0, T22);
            sum0 = _mm_add_epi32(sum0, T23);

            T00 = _mm_loadl_epi64((__m128i*)(fenc + ((i) * fencstride) + 16));
            T01 = _mm_loadl_epi64((__m128i*)(fenc + ((i + 1) * fencstride) + 16));
            T01 = _mm_unpacklo_epi64(T00, T01);

            T02 = _mm_loadl_epi64((__m128i*)(fenc + ((i + 2) * fencstride) + 16));
            T03 = _mm_loadl_epi64((__m128i*)(fenc + ((i + 3) * fencstride) + 16));
            T03 = _mm_unpacklo_epi64(T02, T03);

            T10 = _mm_loadl_epi64((__m128i*)(fref + ((i) * frefstride) + 16));
            T11 = _mm_loadl_epi64((__m128i*)(fref + ((i + 1) * frefstride) + 16));
            T11 = _mm_unpacklo_epi64(T10, T11);

            T12 = _mm_loadl_epi64((__m128i*)(fref + ((i + 2) * frefstride) + 16));
            T13 = _mm_loadl_epi64((__m128i*)(fref + ((i + 3) * frefstride) + 16));
            T13 = _mm_unpacklo_epi64(T12, T13);

            T20 = _mm_setzero_si128();
            T21 = _mm_setzero_si128();

            T20 = _mm_sad_epu8(T01, T11);
            T21 = _mm_sad_epu8(T03, T13);

            sum0 = _mm_add_epi32(sum0, T20);
            sum0 = _mm_add_epi32(sum0, T21);

            T00 = _mm_load_si128((__m128i*)(fenc + (i + 4) * fencstride));
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 5) * fencstride));
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 6) * fencstride));
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 7) * fencstride));

            T10 = _mm_load_si128((__m128i*)(fref + (i + 4) * frefstride));
            T11 = _mm_load_si128((__m128i*)(fref + (i + 5) * frefstride));
            T12 = _mm_load_si128((__m128i*)(fref + (i + 6) * frefstride));
            T13 = _mm_load_si128((__m128i*)(fref + (i + 7) * frefstride));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            sum0 = _mm_add_epi32(sum0, T20);
            sum0 = _mm_add_epi32(sum0, T21);
            sum0 = _mm_add_epi32(sum0, T22);
            sum0 = _mm_add_epi32(sum0, T23);

            T00 = _mm_loadl_epi64((__m128i*)(fenc + ((i + 4) * fencstride) + 16));
            T01 = _mm_loadl_epi64((__m128i*)(fenc + ((i + 5) * fencstride) + 16));
            T01 = _mm_unpacklo_epi64(T00, T01);

            T02 = _mm_loadl_epi64((__m128i*)(fenc + ((i + 6) * fencstride) + 16));
            T03 = _mm_loadl_epi64((__m128i*)(fenc + ((i + 7) * fencstride) + 16));
            T03 = _mm_unpacklo_epi64(T02, T03);

            T10 = _mm_loadl_epi64((__m128i*)(fref + ((i + 4) * frefstride) + 16));
            T11 = _mm_loadl_epi64((__m128i*)(fref + ((i + 5) * frefstride) + 16));
            T11 = _mm_unpacklo_epi64(T10, T11);

            T12 = _mm_loadl_epi64((__m128i*)(fref + ((i + 6) * frefstride) + 16));
            T13 = _mm_loadl_epi64((__m128i*)(fref + ((i + 7) * frefstride) + 16));
            T13 = _mm_unpacklo_epi64(T12, T13);

            T20 = _mm_setzero_si128();
            T21 = _mm_setzero_si128();

            T20 = _mm_sad_epu8(T01, T11);
            T21 = _mm_sad_epu8(T03, T13);

            sum0 = _mm_add_epi32(sum0, T20);
            sum0 = _mm_add_epi32(sum0, T21);
        }
    }
    else
    {
        for (int i = 0; i < ly; i += 4)
        {
            __m128i T00, T01, T02, T03;
            __m128i T10, T11, T12, T13;
            __m128i T20, T21, T22, T23;

            T00 = _mm_load_si128((__m128i*)(fenc + (i) * fencstride));
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 1) * fencstride));
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 2) * fencstride));
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 3) * fencstride));

            T10 = _mm_load_si128((__m128i*)(fref + (i) * frefstride));
            T11 = _mm_load_si128((__m128i*)(fref + (i + 1) * frefstride));
            T12 = _mm_load_si128((__m128i*)(fref + (i + 2) * frefstride));
            T13 = _mm_load_si128((__m128i*)(fref + (i + 3) * frefstride));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            sum0 = _mm_add_epi16(sum0, T20);
            sum0 = _mm_add_epi16(sum0, T21);
            sum0 = _mm_add_epi16(sum0, T22);
            sum0 = _mm_add_epi16(sum0, T23);

            T00 = _mm_loadl_epi64((__m128i*)(fenc + ((i) * fencstride) + 16));
            T01 = _mm_loadl_epi64((__m128i*)(fenc + ((i + 1) * fencstride) + 16));
            T01 = _mm_unpacklo_epi64(T00, T01);

            T02 = _mm_loadl_epi64((__m128i*)(fenc + ((i + 2) * fencstride) + 16));
            T03 = _mm_loadl_epi64((__m128i*)(fenc + ((i + 3) * fencstride) + 16));
            T03 = _mm_unpacklo_epi64(T02, T03);

            T10 = _mm_loadl_epi64((__m128i*)(fref + ((i) * frefstride) + 16));
            T11 = _mm_loadl_epi64((__m128i*)(fref + ((i + 1) * frefstride) + 16));
            T11 = _mm_unpacklo_epi64(T10, T11);

            T12 = _mm_loadl_epi64((__m128i*)(fref + ((i + 2) * frefstride) + 16));
            T13 = _mm_loadl_epi64((__m128i*)(fref + ((i + 3) * frefstride) + 16));
            T13 = _mm_unpacklo_epi64(T12, T13);

            T20 = _mm_setzero_si128();
            T21 = _mm_setzero_si128();

            T20 = _mm_sad_epu8(T01, T11);
            T21 = _mm_sad_epu8(T03, T13);

            sum0 = _mm_add_epi32(sum0, T20);
            sum0 = _mm_add_epi32(sum0, T21);
        }
    }
    sum1 = _mm_shuffle_epi32(sum0, 2);
    sum0 = _mm_add_epi32(sum0, sum1);

    return _mm_cvtsi128_si32(sum0);
}

template<int ly>
int sad_32(pixel * fenc, intptr_t fencstride, pixel * fref, intptr_t frefstride)
{
    assert((ly % 4) == 0);

    __m128i sum0 = _mm_setzero_si128();
    __m128i sum1 = _mm_setzero_si128();

    if (ly == 4)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T01 = _mm_load_si128((__m128i*)(fenc + fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref));
        T11 = _mm_loadu_si128((__m128i*)(fref + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        T00 = _mm_load_si128((__m128i*)(fenc + 16));
        T01 = _mm_load_si128((__m128i*)(fenc + fencstride + 16));
        T02 = _mm_load_si128((__m128i*)(fenc + ((2) * fencstride) + 16));
        T03 = _mm_load_si128((__m128i*)(fenc + ((3) * fencstride) + 16));

        T10 = _mm_loadu_si128((__m128i*)(fref + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref + frefstride + 16));
        T12 = _mm_loadu_si128((__m128i*)(fref + ((2) * frefstride) + 16));
        T13 = _mm_loadu_si128((__m128i*)(fref + ((3) * frefstride) + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi32(sum0, T20);
        sum0 = _mm_add_epi32(sum0, T21);
        sum0 = _mm_add_epi32(sum0, T22);
        sum0 = _mm_add_epi32(sum0, T23);
    }
    else if (ly == 8)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T01 = _mm_load_si128((__m128i*)(fenc + fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref));
        T11 = _mm_loadu_si128((__m128i*)(fref + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        T00 = _mm_load_si128((__m128i*)(fenc + 16));
        T01 = _mm_load_si128((__m128i*)(fenc + fencstride + 16));
        T02 = _mm_load_si128((__m128i*)(fenc + ((2) * fencstride) + 16));
        T03 = _mm_load_si128((__m128i*)(fenc + ((3) * fencstride) + 16));

        T10 = _mm_loadu_si128((__m128i*)(fref + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref + frefstride + 16));
        T12 = _mm_loadu_si128((__m128i*)(fref + ((2) * frefstride) + 16));
        T13 = _mm_loadu_si128((__m128i*)(fref + ((3) * frefstride) + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi32(sum0, T20);
        sum0 = _mm_add_epi32(sum0, T21);
        sum0 = _mm_add_epi32(sum0, T22);
        sum0 = _mm_add_epi32(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (4) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + (5) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + (6) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + (7) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi32(sum0, T20);
        sum0 = _mm_add_epi32(sum0, T21);
        sum0 = _mm_add_epi32(sum0, T22);
        sum0 = _mm_add_epi32(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + ((4) * fencstride) + 16));
        T01 = _mm_load_si128((__m128i*)(fenc + ((5) * fencstride) + 16));
        T02 = _mm_load_si128((__m128i*)(fenc + ((6) * fencstride) + 16));
        T03 = _mm_load_si128((__m128i*)(fenc + ((7) * fencstride) + 16));

        T10 = _mm_loadu_si128((__m128i*)(fref + ((4) * frefstride) + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref + ((5) * frefstride) + 16));
        T12 = _mm_loadu_si128((__m128i*)(fref + ((6) * frefstride) + 16));
        T13 = _mm_loadu_si128((__m128i*)(fref + ((7) * frefstride) + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi32(sum0, T20);
        sum0 = _mm_add_epi32(sum0, T21);
        sum0 = _mm_add_epi32(sum0, T22);
        sum0 = _mm_add_epi32(sum0, T23);
    }
    else if (ly == 16)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T01 = _mm_load_si128((__m128i*)(fenc + fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref));
        T11 = _mm_loadu_si128((__m128i*)(fref + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        T00 = _mm_load_si128((__m128i*)(fenc + 16));
        T01 = _mm_load_si128((__m128i*)(fenc + fencstride + 16));
        T02 = _mm_load_si128((__m128i*)(fenc + ((2) * fencstride) + 16));
        T03 = _mm_load_si128((__m128i*)(fenc + ((3) * fencstride) + 16));

        T10 = _mm_loadu_si128((__m128i*)(fref + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref + frefstride + 16));
        T12 = _mm_loadu_si128((__m128i*)(fref + ((2) * frefstride) + 16));
        T13 = _mm_loadu_si128((__m128i*)(fref + ((3) * frefstride) + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi32(sum0, T20);
        sum0 = _mm_add_epi32(sum0, T21);
        sum0 = _mm_add_epi32(sum0, T22);
        sum0 = _mm_add_epi32(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (4) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + (5) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + (6) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + (7) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi32(sum0, T20);
        sum0 = _mm_add_epi32(sum0, T21);
        sum0 = _mm_add_epi32(sum0, T22);
        sum0 = _mm_add_epi32(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + ((4) * fencstride) + 16));
        T01 = _mm_load_si128((__m128i*)(fenc + ((5) * fencstride) + 16));
        T02 = _mm_load_si128((__m128i*)(fenc + ((6) * fencstride) + 16));
        T03 = _mm_load_si128((__m128i*)(fenc + ((7) * fencstride) + 16));

        T10 = _mm_loadu_si128((__m128i*)(fref + ((4) * frefstride) + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref + ((5) * frefstride) + 16));
        T12 = _mm_loadu_si128((__m128i*)(fref + ((6) * frefstride) + 16));
        T13 = _mm_loadu_si128((__m128i*)(fref + ((7) * frefstride) + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi32(sum0, T20);
        sum0 = _mm_add_epi32(sum0, T21);
        sum0 = _mm_add_epi32(sum0, T22);
        sum0 = _mm_add_epi32(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (8) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + (9) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + (10) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + (11) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi32(sum0, T20);
        sum0 = _mm_add_epi32(sum0, T21);
        sum0 = _mm_add_epi32(sum0, T22);
        sum0 = _mm_add_epi32(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + ((8) * fencstride) + 16));
        T01 = _mm_load_si128((__m128i*)(fenc + ((9) * fencstride) + 16));
        T02 = _mm_load_si128((__m128i*)(fenc + ((10) * fencstride) + 16));
        T03 = _mm_load_si128((__m128i*)(fenc + ((11) * fencstride) + 16));

        T10 = _mm_loadu_si128((__m128i*)(fref + ((8) * frefstride) + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref + ((9) * frefstride) + 16));
        T12 = _mm_loadu_si128((__m128i*)(fref + ((10) * frefstride) + 16));
        T13 = _mm_loadu_si128((__m128i*)(fref + ((11) * frefstride) + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);
        T00 = _mm_load_si128((__m128i*)(fenc + (12) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + (13) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + (14) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + (15) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi32(sum0, T20);
        sum0 = _mm_add_epi32(sum0, T21);
        sum0 = _mm_add_epi32(sum0, T22);
        sum0 = _mm_add_epi32(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + ((12) * fencstride) + 16));
        T01 = _mm_load_si128((__m128i*)(fenc + ((13) * fencstride) + 16));
        T02 = _mm_load_si128((__m128i*)(fenc + ((14) * fencstride) + 16));
        T03 = _mm_load_si128((__m128i*)(fenc + ((15) * fencstride) + 16));

        T10 = _mm_loadu_si128((__m128i*)(fref + ((12) * frefstride) + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref + ((13) * frefstride) + 16));
        T12 = _mm_loadu_si128((__m128i*)(fref + ((14) * frefstride) + 16));
        T13 = _mm_loadu_si128((__m128i*)(fref + ((15) * frefstride) + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi32(sum0, T20);
        sum0 = _mm_add_epi32(sum0, T21);
        sum0 = _mm_add_epi32(sum0, T22);
        sum0 = _mm_add_epi32(sum0, T23);
    }
    else if ((ly % 8) == 0)
    {
        for (int i = 0; i < ly; i += 8)
        {
            __m128i T00, T01, T02, T03;
            __m128i T10, T11, T12, T13;
            __m128i T20, T21, T22, T23;

            T00 = _mm_load_si128((__m128i*)(fenc + (i) * fencstride));
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 1) * fencstride));
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 2) * fencstride));
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 3) * fencstride));

            T10 = _mm_loadu_si128((__m128i*)(fref + (i) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref + (i + 3) * frefstride));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            sum0 = _mm_add_epi32(sum0, T20);
            sum0 = _mm_add_epi32(sum0, T21);
            sum0 = _mm_add_epi32(sum0, T22);
            sum0 = _mm_add_epi32(sum0, T23);

            T00 = _mm_load_si128((__m128i*)(fenc + ((i) * fencstride) + 16));
            T01 = _mm_load_si128((__m128i*)(fenc + ((i + 1) * fencstride) + 16));
            T02 = _mm_load_si128((__m128i*)(fenc + ((i + 2) * fencstride) + 16));
            T03 = _mm_load_si128((__m128i*)(fenc + ((i + 3) * fencstride) + 16));

            T10 = _mm_loadu_si128((__m128i*)(fref + ((i) * frefstride) + 16));
            T11 = _mm_loadu_si128((__m128i*)(fref + ((i + 1) * frefstride) + 16));
            T12 = _mm_loadu_si128((__m128i*)(fref + ((i + 2) * frefstride) + 16));
            T13 = _mm_loadu_si128((__m128i*)(fref + ((i + 3) * frefstride) + 16));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            sum0 = _mm_add_epi32(sum0, T20);
            sum0 = _mm_add_epi32(sum0, T21);
            sum0 = _mm_add_epi32(sum0, T22);
            sum0 = _mm_add_epi32(sum0, T23);

            T00 = _mm_load_si128((__m128i*)(fenc + (i + 4) * fencstride));
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 5) * fencstride));
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 6) * fencstride));
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 7) * fencstride));

            T10 = _mm_loadu_si128((__m128i*)(fref + (i + 4) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref + (i + 5) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref + (i + 6) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref + (i + 7) * frefstride));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            sum0 = _mm_add_epi32(sum0, T20);
            sum0 = _mm_add_epi32(sum0, T21);
            sum0 = _mm_add_epi32(sum0, T22);
            sum0 = _mm_add_epi32(sum0, T23);

            T00 = _mm_load_si128((__m128i*)(fenc + ((i + 4) * fencstride) + 16));
            T01 = _mm_load_si128((__m128i*)(fenc + ((i + 5) * fencstride) + 16));
            T02 = _mm_load_si128((__m128i*)(fenc + ((i + 6) * fencstride) + 16));
            T03 = _mm_load_si128((__m128i*)(fenc + ((i + 7) * fencstride) + 16));

            T10 = _mm_loadu_si128((__m128i*)(fref + ((i + 4) * frefstride) + 16));
            T11 = _mm_loadu_si128((__m128i*)(fref + ((i + 5) * frefstride) + 16));
            T12 = _mm_loadu_si128((__m128i*)(fref + ((i + 6) * frefstride) + 16));
            T13 = _mm_loadu_si128((__m128i*)(fref + ((i + 7) * frefstride) + 16));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            sum0 = _mm_add_epi32(sum0, T20);
            sum0 = _mm_add_epi32(sum0, T21);
            sum0 = _mm_add_epi32(sum0, T22);
            sum0 = _mm_add_epi32(sum0, T23);
        }
    }
    else if ((ly % 4) == 0)
    {
        for (int i = 0; i < ly; i += 4)
        {
            __m128i T00, T01, T02, T03;
            __m128i T10, T11, T12, T13;
            __m128i T20, T21, T22, T23;

            T00 = _mm_load_si128((__m128i*)(fenc + (i) * fencstride));
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 1) * fencstride));
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 2) * fencstride));
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 3) * fencstride));

            T10 = _mm_loadu_si128((__m128i*)(fref + (i) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref + (i + 3) * frefstride));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            sum0 = _mm_add_epi32(sum0, T20);
            sum0 = _mm_add_epi32(sum0, T21);
            sum0 = _mm_add_epi32(sum0, T22);
            sum0 = _mm_add_epi32(sum0, T23);

            T00 = _mm_load_si128((__m128i*)(fenc + ((i) * fencstride) + 16));
            T01 = _mm_load_si128((__m128i*)(fenc + ((i + 1) * fencstride) + 16));
            T02 = _mm_load_si128((__m128i*)(fenc + ((i + 2) * fencstride) + 16));
            T03 = _mm_load_si128((__m128i*)(fenc + ((i + 3) * fencstride) + 16));

            T10 = _mm_loadu_si128((__m128i*)(fref + ((i + 0) * frefstride) + 16));
            T11 = _mm_loadu_si128((__m128i*)(fref + ((i + 1) * frefstride) + 16));
            T12 = _mm_loadu_si128((__m128i*)(fref + ((i + 2) * frefstride) + 16));
            T13 = _mm_loadu_si128((__m128i*)(fref + ((i + 3) * frefstride) + 16));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            sum0 = _mm_add_epi32(sum0, T20);
            sum0 = _mm_add_epi32(sum0, T21);
            sum0 = _mm_add_epi32(sum0, T22);
            sum0 = _mm_add_epi32(sum0, T23);
        }
    }

    sum1 = _mm_shuffle_epi32(sum0, 2);
    sum0 = _mm_add_epi32(sum0, sum1);

    return _mm_cvtsi128_si32(sum0);
}

#if HAVE_MMX
template<int ly>
void sad_x3_4(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, intptr_t frefstride, int *res)
{
    assert((ly % 4) == 0);

    if (ly == 4)
    {
        __m64 sum0, sum1, sum2;
        __m64 T10, T11, T12, T13;
        __m64 T00, T01, T02, T03;
        __m64 T20, T21;

        T00 = _mm_cvtsi32_si64(*(int*)(fenc));
        T01 = _mm_cvtsi32_si64(*(int*)(fenc + FENC_STRIDE));
        T00 = _mm_unpacklo_pi8(T00, T01);
        T02 = _mm_cvtsi32_si64(*(int*)(fenc + (FENC_STRIDE << 1)));
        T03 = _mm_cvtsi32_si64(*(int*)(fenc + 3 * FENC_STRIDE));
        T02 = _mm_unpacklo_pi8(T02, T03);

        T10 = _mm_cvtsi32_si64(*(int*)(fref1));
        T11 = _mm_cvtsi32_si64(*(int*)(fref1 + frefstride));
        T10 = _mm_unpacklo_pi8(T10, T11);
        T12 = _mm_cvtsi32_si64(*(int*)(fref1 + 2 * frefstride));
        T13 = _mm_cvtsi32_si64(*(int*)(fref1 + 3 * frefstride));
        T12 = _mm_unpacklo_pi8(T12, T13);

        T20 = _mm_sad_pu8(T00, T10);
        T21 = _mm_sad_pu8(T02, T12);

        sum0 = _mm_add_pi16(T20, T21);

        T10 = _mm_cvtsi32_si64(*(int*)(fref2));
        T11 = _mm_cvtsi32_si64(*(int*)(fref2 + frefstride));
        T10 = _mm_unpacklo_pi8(T10, T11);
        T12 = _mm_cvtsi32_si64(*(int*)(fref2 + 2 * frefstride));
        T13 = _mm_cvtsi32_si64(*(int*)(fref2 + 3 * frefstride));
        T12 = _mm_unpacklo_pi8(T12, T13);

        T20 = _mm_sad_pu8(T00, T10);
        T21 = _mm_sad_pu8(T02, T12);

        sum1 = _mm_add_pi16(T20, T21);

        T10 = _mm_cvtsi32_si64(*(int*)(fref3));
        T11 = _mm_cvtsi32_si64(*(int*)(fref3 + frefstride));
        T10 = _mm_unpacklo_pi8(T10, T11);
        T12 = _mm_cvtsi32_si64(*(int*)(fref3 + 2 * frefstride));
        T13 = _mm_cvtsi32_si64(*(int*)(fref3 + 3 * frefstride));
        T12 = _mm_unpacklo_pi8(T12, T13);

        T20 = _mm_sad_pu8(T00, T10);
        T21 = _mm_sad_pu8(T02, T12);

        sum2 = _mm_add_pi16(T20, T21);

        res[0] = _m_to_int(sum0);
        res[1] = _m_to_int(sum1);
        res[2] = _m_to_int(sum2);
    }
    else if (ly == 8)
    {
        __m128i sum0, sum1, sum2;

        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i R00, R01, R02, R03;
        __m128i T20;

        T00 = _mm_loadl_epi64((__m128i*)(fenc));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + FENC_STRIDE));
        T01 = _mm_unpacklo_epi32(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (3) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi32(T02, T03);
        R00 = _mm_unpacklo_epi64(T01, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + 2 * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + 3 * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R01 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref2));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + 2 * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + 3 * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R02 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref3));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + 2 * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + 3 * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R03 = _mm_unpacklo_epi64(T11, T13);

        T20 = _mm_sad_epu8(R00, R01);
        sum0 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));

        T20 = _mm_sad_epu8(R00, R02);
        sum1 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));

        T20 = _mm_sad_epu8(R00, R03);
        sum2 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (4) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (5) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi32(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (6) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (7) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi32(T02, T03);
        R00 = _mm_unpacklo_epi64(T01, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + 4 * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + 5 * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + 6 * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + 7 * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R01 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + 4 * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + 5 * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + 6 * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + 7 * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R02 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + 4 * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + 5 * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + 6 * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + 7 * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R03 = _mm_unpacklo_epi64(T11, T13);

        T20 = _mm_sad_epu8(R00, R01);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum0 = _mm_add_epi32(sum0, T20);

        T20 = _mm_sad_epu8(R00, R02);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum1 = _mm_add_epi32(sum1, T20);

        T20 = _mm_sad_epu8(R00, R03);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum2 = _mm_add_epi32(sum2, T20);

        res[0] = _mm_cvtsi128_si32(sum0);
        res[1] = _mm_cvtsi128_si32(sum1);
        res[2] = _mm_cvtsi128_si32(sum2);
    }
    else if (ly == 16)
    {
        __m128i sum0, sum1, sum2;

        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i R00, R01, R02, R03;
        __m128i T20;

        T00 = _mm_loadl_epi64((__m128i*)(fenc));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + FENC_STRIDE));
        T01 = _mm_unpacklo_epi32(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (3) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi32(T02, T03);
        R00 = _mm_unpacklo_epi64(T01, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (3) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R01 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref2));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (3) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R02 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref3));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (3) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R03 = _mm_unpacklo_epi64(T11, T13);

        T20 = _mm_sad_epu8(R00, R01);
        sum0 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));

        T20 = _mm_sad_epu8(R00, R02);
        sum1 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));

        T20 = _mm_sad_epu8(R00, R03);
        sum2 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (4) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (5) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi32(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (6) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (7) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi32(T02, T03);
        R00 = _mm_unpacklo_epi64(T01, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (5) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (7) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R01 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (5) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (7) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R02 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (5) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (7) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R03 = _mm_unpacklo_epi64(T11, T13);

        T20 = _mm_sad_epu8(R00, R01);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum0 = _mm_add_epi32(sum0, T20);

        T20 = _mm_sad_epu8(R00, R02);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum1 = _mm_add_epi32(sum1, T20);

        T20 = _mm_sad_epu8(R00, R03);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum2 = _mm_add_epi32(sum2, T20);

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (8) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (9) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi32(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (10) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (11) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi32(T02, T03);
        R00 = _mm_unpacklo_epi64(T01, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (8) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (9) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (10) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (11) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R01 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (8) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (9) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (10) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (11) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R02 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (8) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (9) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (10) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (11) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R03 = _mm_unpacklo_epi64(T11, T13);

        T20 = _mm_sad_epu8(R00, R01);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum0 = _mm_add_epi32(sum0, T20);

        T20 = _mm_sad_epu8(R00, R02);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum1 = _mm_add_epi32(sum1, T20);

        T20 = _mm_sad_epu8(R00, R03);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum2 = _mm_add_epi32(sum2, T20);

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (12) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (13) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi32(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (14) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (15) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi32(T02, T03);
        R00 = _mm_unpacklo_epi64(T01, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (12) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (13) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (14) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (15) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R01 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (12) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (13) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (14) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (15) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R02 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (12) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (13) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (14) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (15) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R03 = _mm_unpacklo_epi64(T11, T13);

        T20 = _mm_sad_epu8(R00, R01);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum0 = _mm_add_epi32(sum0, T20);

        T20 = _mm_sad_epu8(R00, R02);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum1 = _mm_add_epi32(sum1, T20);

        T20 = _mm_sad_epu8(R00, R03);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum2 = _mm_add_epi32(sum2, T20);

        res[0] = _mm_cvtsi128_si32(sum0);
        res[1] = _mm_cvtsi128_si32(sum1);
        res[2] = _mm_cvtsi128_si32(sum2);
    }
    else if ((ly % 8) == 0)
    {
        __m128i sum0 = _mm_setzero_si128();
        __m128i sum1 = _mm_setzero_si128();
        __m128i sum2 = _mm_setzero_si128();

        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i R00, R01, R02, R03;
        __m128i T20;

        for (int i = 0; i < ly; i += 8)
        {
            T00 = _mm_loadl_epi64((__m128i*)(fenc + (i + 0) * FENC_STRIDE));
            T01 = _mm_loadl_epi64((__m128i*)(fenc + (i + 1) * FENC_STRIDE));
            T01 = _mm_unpacklo_epi32(T00, T01);
            T02 = _mm_loadl_epi64((__m128i*)(fenc + (i + 2) * FENC_STRIDE));
            T03 = _mm_loadl_epi64((__m128i*)(fenc + (i + 3) * FENC_STRIDE));
            T03 = _mm_unpacklo_epi32(T02, T03);
            R00 = _mm_unpacklo_epi64(T01, T03);

            T10 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            R01 = _mm_unpacklo_epi64(T11, T13);

            T10 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            R02 = _mm_unpacklo_epi64(T11, T13);

            T10 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            R03 = _mm_unpacklo_epi64(T11, T13);

            T20 = _mm_sad_epu8(R00, R01);
            T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
            sum0 = _mm_add_epi32(sum0, T20);

            T20 = _mm_sad_epu8(R00, R02);
            T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
            sum1 = _mm_add_epi32(sum1, T20);

            T20 = _mm_sad_epu8(R00, R03);
            T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
            sum2 = _mm_add_epi32(sum2, T20);

            T00 = _mm_loadl_epi64((__m128i*)(fenc + (i + 4) * FENC_STRIDE));
            T01 = _mm_loadl_epi64((__m128i*)(fenc + (i + 5) * FENC_STRIDE));
            T01 = _mm_unpacklo_epi32(T00, T01);
            T02 = _mm_loadl_epi64((__m128i*)(fenc + (i + 6) * FENC_STRIDE));
            T03 = _mm_loadl_epi64((__m128i*)(fenc + (i + 7) * FENC_STRIDE));
            T03 = _mm_unpacklo_epi32(T02, T03);
            R00 = _mm_unpacklo_epi64(T01, T03);

            T10 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 4) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 5) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 6) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 7) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            R01 = _mm_unpacklo_epi64(T11, T13);

            T10 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 4) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 5) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 6) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 7) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            R02 = _mm_unpacklo_epi64(T11, T13);

            T10 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 4) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 5) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 6) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 7) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            R03 = _mm_unpacklo_epi64(T11, T13);

            T20 = _mm_sad_epu8(R00, R01);
            T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
            sum0 = _mm_add_epi32(sum0, T20);

            T20 = _mm_sad_epu8(R00, R02);
            T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
            sum1 = _mm_add_epi32(sum1, T20);

            T20 = _mm_sad_epu8(R00, R03);
            T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
            sum2 = _mm_add_epi32(sum2, T20);
        }

        res[0] = _mm_cvtsi128_si32(sum0);
        res[1] = _mm_cvtsi128_si32(sum1);
        res[2] = _mm_cvtsi128_si32(sum2);
    }
    else
    {
        __m64 sum0 = _mm_setzero_si64();
        __m64 sum1 = _mm_setzero_si64();
        __m64 sum2 = _mm_setzero_si64();

        __m64 T10, T11, T12, T13;
        __m64 T00, T01, T02, T03;
        __m64 T20, T21;

        for (int i = 0; i < ly; i += 4)
        {
            int frefstrideZero = (i + 0) * frefstride;
            int frefstrideOne = (i + 1) * frefstride;
            int frefstrideTwo = (i + 2) * frefstride;
            int frefstrideThree = (i + 3) * frefstride;

            T00 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 0) * FENC_STRIDE));
            T01 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 1) * FENC_STRIDE));
            T00 = _mm_unpacklo_pi8(T00, T01);
            T02 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 2) * FENC_STRIDE));
            T03 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 3) * FENC_STRIDE));
            T02 = _mm_unpacklo_pi8(T02, T03);

            T10 = _mm_cvtsi32_si64(*(int*)(fref1 + frefstrideZero));
            T11 = _mm_cvtsi32_si64(*(int*)(fref1 + frefstrideOne));
            T10 = _mm_unpacklo_pi8(T10, T11);
            T12 = _mm_cvtsi32_si64(*(int*)(fref1 + frefstrideTwo));
            T13 = _mm_cvtsi32_si64(*(int*)(fref1 + frefstrideThree));
            T12 = _mm_unpacklo_pi8(T12, T13);

            T20 = _mm_sad_pu8(T00, T10);
            T21 = _mm_sad_pu8(T02, T12);

            sum0 = _mm_add_pi16(sum0, T20);
            sum0 = _mm_add_pi16(sum0, T21);

            T10 = _mm_cvtsi32_si64(*(int*)(fref2 + frefstrideZero));
            T11 = _mm_cvtsi32_si64(*(int*)(fref2 + frefstrideOne));
            T10 = _mm_unpacklo_pi8(T10, T11);
            T12 = _mm_cvtsi32_si64(*(int*)(fref2 + frefstrideTwo));
            T13 = _mm_cvtsi32_si64(*(int*)(fref2 + frefstrideThree));
            T12 = _mm_unpacklo_pi8(T12, T13);

            T20 = _mm_sad_pu8(T00, T10);
            T21 = _mm_sad_pu8(T02, T12);

            sum1 = _mm_add_pi16(sum1, T20);
            sum1 = _mm_add_pi16(sum1, T21);

            T10 = _mm_cvtsi32_si64(*(int*)(fref3 + frefstrideZero));
            T11 = _mm_cvtsi32_si64(*(int*)(fref3 + frefstrideOne));
            T10 = _mm_unpacklo_pi8(T10, T11);
            T12 = _mm_cvtsi32_si64(*(int*)(fref3 + frefstrideTwo));
            T13 = _mm_cvtsi32_si64(*(int*)(fref3 + frefstrideThree));
            T12 = _mm_unpacklo_pi8(T12, T13);

            T20 = _mm_sad_pu8(T00, T10);
            T21 = _mm_sad_pu8(T02, T12);

            sum2 = _mm_add_pi16(sum2, T20);
            sum2 = _mm_add_pi16(sum2, T21);
        }

        res[0] = _m_to_int(sum0);
        res[1] = _m_to_int(sum1);
        res[2] = _m_to_int(sum2);
    }
}

#else /* if HAVE_MMX */

template<int ly>
void sad_x3_4(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, intptr_t frefstride, int *res)
{
    assert((ly % 4) == 0);
    __m128i sum0 = _mm_setzero_si128();
    __m128i sum1 = _mm_setzero_si128();
    __m128i sum2 = _mm_setzero_si128();

    __m128i T00, T01, T02, T03;
    __m128i T10, T11, T12, T13;
    __m128i R00, R01, R02, R03;
    __m128i T20;

    if (ly == 4)
    {
        T00 = _mm_loadl_epi64((__m128i*)(fenc + (0) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (1) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi32(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (3) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi32(T02, T03);
        R00 = _mm_unpacklo_epi64(T01, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (0) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (1) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (3) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R01 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (0) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (1) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (3) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R02 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (0) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (1) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (3) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R03 = _mm_unpacklo_epi64(T11, T13);

        T20 = _mm_sad_epu8(R00, R01);
        sum0 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));

        T20 = _mm_sad_epu8(R00, R02);
        sum1 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));

        T20 = _mm_sad_epu8(R00, R03);
        sum2 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
    }
    else if (ly == 8)
    {
        T00 = _mm_loadl_epi64((__m128i*)(fenc + (0) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (1) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi32(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (3) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi32(T02, T03);
        R00 = _mm_unpacklo_epi64(T01, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (0) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (1) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (3) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R01 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (0) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (1) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (3) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R02 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (0) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (1) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (3) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R03 = _mm_unpacklo_epi64(T11, T13);

        T20 = _mm_sad_epu8(R00, R01);
        sum0 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));

        T20 = _mm_sad_epu8(R00, R02);
        sum1 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));

        T20 = _mm_sad_epu8(R00, R03);
        sum2 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (4) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (5) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi32(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (6) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (7) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi32(T02, T03);
        R00 = _mm_unpacklo_epi64(T01, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (5) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (7) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R01 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (5) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (7) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R02 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (5) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (7) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R03 = _mm_unpacklo_epi64(T11, T13);

        T20 = _mm_sad_epu8(R00, R01);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum0 = _mm_add_epi32(sum0, T20);

        T20 = _mm_sad_epu8(R00, R02);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum1 = _mm_add_epi32(sum1, T20);

        T20 = _mm_sad_epu8(R00, R03);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum2 = _mm_add_epi32(sum2, T20);
    }
    else if (ly == 16)
    {
        T00 = _mm_loadl_epi64((__m128i*)(fenc + (0) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (1) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi32(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (3) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi32(T02, T03);
        R00 = _mm_unpacklo_epi64(T01, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (0) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (1) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (3) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R01 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (0) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (1) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (3) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R02 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (0) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (1) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (3) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R03 = _mm_unpacklo_epi64(T11, T13);

        T20 = _mm_sad_epu8(R00, R01);
        sum0 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));

        T20 = _mm_sad_epu8(R00, R02);
        sum1 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));

        T20 = _mm_sad_epu8(R00, R03);
        sum2 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (4) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (5) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi32(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (6) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (7) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi32(T02, T03);
        R00 = _mm_unpacklo_epi64(T01, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (5) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (7) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R01 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (5) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (7) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R02 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (5) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (7) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R03 = _mm_unpacklo_epi64(T11, T13);

        T20 = _mm_sad_epu8(R00, R01);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum0 = _mm_add_epi32(sum0, T20);

        T20 = _mm_sad_epu8(R00, R02);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum1 = _mm_add_epi32(sum1, T20);

        T20 = _mm_sad_epu8(R00, R03);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum2 = _mm_add_epi32(sum2, T20);

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (8) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (9) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi32(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (10) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (11) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi32(T02, T03);
        R00 = _mm_unpacklo_epi64(T01, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (8) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (9) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (10) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (11) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R01 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (8) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (9) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (10) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (11) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R02 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (8) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (9) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (10) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (11) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R03 = _mm_unpacklo_epi64(T11, T13);

        T20 = _mm_sad_epu8(R00, R01);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum0 = _mm_add_epi32(sum0, T20);

        T20 = _mm_sad_epu8(R00, R02);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum1 = _mm_add_epi32(sum1, T20);

        T20 = _mm_sad_epu8(R00, R03);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum2 = _mm_add_epi32(sum2, T20);

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (12) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (13) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi32(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (14) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (15) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi32(T02, T03);
        R00 = _mm_unpacklo_epi64(T01, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (12) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (13) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (14) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (15) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R01 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (12) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (13) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (14) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (15) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R02 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (12) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (13) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (14) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (15) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R03 = _mm_unpacklo_epi64(T11, T13);

        T20 = _mm_sad_epu8(R00, R01);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum0 = _mm_add_epi32(sum0, T20);

        T20 = _mm_sad_epu8(R00, R02);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum1 = _mm_add_epi32(sum1, T20);

        T20 = _mm_sad_epu8(R00, R03);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum2 = _mm_add_epi32(sum2, T20);
    }
    else if ((ly % 8) == 0)
    {
        for (int i = 0; i < ly; i += 8)
        {
            T00 = _mm_loadl_epi64((__m128i*)(fenc + (i + 0) * FENC_STRIDE));
            T01 = _mm_loadl_epi64((__m128i*)(fenc + (i + 1) * FENC_STRIDE));
            T01 = _mm_unpacklo_epi32(T00, T01);
            T02 = _mm_loadl_epi64((__m128i*)(fenc + (i + 2) * FENC_STRIDE));
            T03 = _mm_loadl_epi64((__m128i*)(fenc + (i + 3) * FENC_STRIDE));
            T03 = _mm_unpacklo_epi32(T02, T03);
            R00 = _mm_unpacklo_epi64(T01, T03);

            T10 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            R01 = _mm_unpacklo_epi64(T11, T13);

            T10 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            R02 = _mm_unpacklo_epi64(T11, T13);

            T10 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            R03 = _mm_unpacklo_epi64(T11, T13);

            T20 = _mm_sad_epu8(R00, R01);
            T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
            sum0 = _mm_add_epi32(sum0, T20);

            T20 = _mm_sad_epu8(R00, R02);
            T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
            sum1 = _mm_add_epi32(sum1, T20);

            T20 = _mm_sad_epu8(R00, R03);
            T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
            sum2 = _mm_add_epi32(sum2, T20);

            T00 = _mm_loadl_epi64((__m128i*)(fenc + (i + 4) * FENC_STRIDE));
            T01 = _mm_loadl_epi64((__m128i*)(fenc + (i + 5) * FENC_STRIDE));
            T01 = _mm_unpacklo_epi32(T00, T01);
            T02 = _mm_loadl_epi64((__m128i*)(fenc + (i + 6) * FENC_STRIDE));
            T03 = _mm_loadl_epi64((__m128i*)(fenc + (i + 7) * FENC_STRIDE));
            T03 = _mm_unpacklo_epi32(T02, T03);
            R00 = _mm_unpacklo_epi64(T01, T03);

            T10 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 4) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 5) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 6) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 7) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            R01 = _mm_unpacklo_epi64(T11, T13);

            T10 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 4) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 5) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 6) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 7) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            R02 = _mm_unpacklo_epi64(T11, T13);

            T10 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 4) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 5) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 6) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 7) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            R03 = _mm_unpacklo_epi64(T11, T13);

            T20 = _mm_sad_epu8(R00, R01);
            T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
            sum0 = _mm_add_epi32(sum0, T20);

            T20 = _mm_sad_epu8(R00, R02);
            T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
            sum1 = _mm_add_epi32(sum1, T20);

            T20 = _mm_sad_epu8(R00, R03);
            T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
            sum2 = _mm_add_epi32(sum2, T20);
        }
    }
    else
    {
        for (int i = 0; i < ly; i += 4)
        {
            T00 = _mm_loadl_epi64((__m128i*)(fenc + (i + 0) * FENC_STRIDE));
            T01 = _mm_loadl_epi64((__m128i*)(fenc + (i + 1) * FENC_STRIDE));
            T01 = _mm_unpacklo_epi32(T00, T01);
            T02 = _mm_loadl_epi64((__m128i*)(fenc + (i + 2) * FENC_STRIDE));
            T03 = _mm_loadl_epi64((__m128i*)(fenc + (i + 3) * FENC_STRIDE));
            T03 = _mm_unpacklo_epi32(T02, T03);
            R00 = _mm_unpacklo_epi64(T01, T03);

            T10 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            R01 = _mm_unpacklo_epi64(T11, T13);

            T10 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            R02 = _mm_unpacklo_epi64(T11, T13);

            T10 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            R03 = _mm_unpacklo_epi64(T11, T13);

            T20 = _mm_sad_epu8(R00, R01);
            T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
            sum0 = _mm_add_epi32(sum0, T20);

            T20 = _mm_sad_epu8(R00, R02);
            T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
            sum1 = _mm_add_epi32(sum1, T20);

            T20 = _mm_sad_epu8(R00, R03);
            T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
            sum2 = _mm_add_epi32(sum2, T20);
        }
    }

    res[0] = _mm_cvtsi128_si32(sum0);
    res[1] = _mm_cvtsi128_si32(sum1);
    res[2] = _mm_cvtsi128_si32(sum2);
}

#endif /* if HAVE_MMX */

#if HAVE_MMX
template<int ly>
void sad_x3_8(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, intptr_t frefstride, int *res)
{
    assert((ly % 4) == 0);

    if (ly == 4)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21;

        __m128i sum0;

        // fenc = fenc + (0) * FENC_STRIDE, fenc + FENC_STRIDE = fenc + (1) * FENC_STRIDE
        T00 = _mm_loadl_epi64((__m128i*)(fenc));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + FENC_STRIDE));
        T01 = _mm_unpacklo_epi64(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (3) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[0] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref2));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[1] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref3));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[2] = _mm_cvtsi128_si32(sum0);
    }
    else if (ly == 8)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21;

        __m128i sum0;

        T00 = _mm_loadl_epi64((__m128i*)(fenc));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + FENC_STRIDE));
        T01 = _mm_unpacklo_epi64(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (3) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[0] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref2));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[1] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref3));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[2] = _mm_cvtsi128_si32(sum0);

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (4) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (5) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi64(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (6) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (7) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (5) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (7) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[0] = res[0] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (5) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (7) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[1] = res[1] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (5) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (7) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[2] = res[2] + _mm_cvtsi128_si32(sum0);
    }
    else if (ly == 16)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21;

        __m128i sum0;

        T00 = _mm_loadl_epi64((__m128i*)(fenc));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + FENC_STRIDE));
        T01 = _mm_unpacklo_epi64(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (3) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[0] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref2));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[1] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref3));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[2] = _mm_cvtsi128_si32(sum0);

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (4) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (5) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi64(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (6) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (7) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (5) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (7) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[0] = res[0] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (5) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (7) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[1] = res[1] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (5) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (7) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[2] = res[2] + _mm_cvtsi128_si32(sum0);

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (8) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (9) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi64(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (10) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (11) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (8) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (9) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (10) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (11) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[0] = res[0] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (8) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (9) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (10) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (11) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[1] = res[1] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (8) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (9) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (10) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (11) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[2] = res[2] + _mm_cvtsi128_si32(sum0);

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (12) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (13) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi64(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (14) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (15) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (12) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (13) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (14) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (15) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[0] = res[0] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (12) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (13) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (14) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (15) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[1] = res[1] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (12) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (13) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (14) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (15) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[2] = res[2] + _mm_cvtsi128_si32(sum0);
    }
    else if ((ly % 8) == 0)
    {
        __m64 sum0 = _mm_setzero_si64();
        __m64 sum1 = _mm_setzero_si64();
        __m64 sum2 = _mm_setzero_si64();

        __m64 T00, T01, T02, T03, T04, T05, T06, T07;
        __m64 T10, T11, T12, T13, T14, T15, T16, T17;
        __m64 T20, T21, T22, T23, T24, T25, T26, T27;

        for (int i = 0; i < ly; i += 8)
        {
            T00 = (*(__m64*)(fenc + (i + 0) * FENC_STRIDE));
            T01 = (*(__m64*)(fenc + (i + 1) * FENC_STRIDE));
            T02 = (*(__m64*)(fenc + (i + 2) * FENC_STRIDE));
            T03 = (*(__m64*)(fenc + (i + 3) * FENC_STRIDE));
            T04 = (*(__m64*)(fenc + (i + 4) * FENC_STRIDE));
            T05 = (*(__m64*)(fenc + (i + 5) * FENC_STRIDE));
            T06 = (*(__m64*)(fenc + (i + 6) * FENC_STRIDE));
            T07 = (*(__m64*)(fenc + (i + 7) * FENC_STRIDE));

            T10 = (*(__m64*)(fref1 + (i + 0) * frefstride));
            T11 = (*(__m64*)(fref1 + (i + 1) * frefstride));
            T12 = (*(__m64*)(fref1 + (i + 2) * frefstride));
            T13 = (*(__m64*)(fref1 + (i + 3) * frefstride));
            T14 = (*(__m64*)(fref1 + (i + 4) * frefstride));
            T15 = (*(__m64*)(fref1 + (i + 5) * frefstride));
            T16 = (*(__m64*)(fref1 + (i + 6) * frefstride));
            T17 = (*(__m64*)(fref1 + (i + 7) * frefstride));

            T20 = _mm_sad_pu8(T00, T10);
            T21 = _mm_sad_pu8(T01, T11);
            T22 = _mm_sad_pu8(T02, T12);
            T23 = _mm_sad_pu8(T03, T13);
            T24 = _mm_sad_pu8(T04, T14);
            T25 = _mm_sad_pu8(T05, T15);
            T26 = _mm_sad_pu8(T06, T16);
            T27 = _mm_sad_pu8(T07, T17);

            sum0 = _mm_add_pi16(sum0, T20);
            sum0 = _mm_add_pi16(sum0, T21);
            sum0 = _mm_add_pi16(sum0, T22);
            sum0 = _mm_add_pi16(sum0, T23);
            sum0 = _mm_add_pi16(sum0, T24);
            sum0 = _mm_add_pi16(sum0, T25);
            sum0 = _mm_add_pi16(sum0, T26);
            sum0 = _mm_add_pi16(sum0, T27);

            T10 = (*(__m64*)(fref2 + (i + 0) * frefstride));
            T11 = (*(__m64*)(fref2 + (i + 1) * frefstride));
            T12 = (*(__m64*)(fref2 + (i + 2) * frefstride));
            T13 = (*(__m64*)(fref2 + (i + 3) * frefstride));
            T14 = (*(__m64*)(fref2 + (i + 4) * frefstride));
            T15 = (*(__m64*)(fref2 + (i + 5) * frefstride));
            T16 = (*(__m64*)(fref2 + (i + 6) * frefstride));
            T17 = (*(__m64*)(fref2 + (i + 7) * frefstride));

            T20 = _mm_sad_pu8(T00, T10);
            T21 = _mm_sad_pu8(T01, T11);
            T22 = _mm_sad_pu8(T02, T12);
            T23 = _mm_sad_pu8(T03, T13);
            T24 = _mm_sad_pu8(T04, T14);
            T25 = _mm_sad_pu8(T05, T15);
            T26 = _mm_sad_pu8(T06, T16);
            T27 = _mm_sad_pu8(T07, T17);

            sum1 = _mm_add_pi16(sum1, T20);
            sum1 = _mm_add_pi16(sum1, T21);
            sum1 = _mm_add_pi16(sum1, T22);
            sum1 = _mm_add_pi16(sum1, T23);
            sum1 = _mm_add_pi16(sum1, T24);
            sum1 = _mm_add_pi16(sum1, T25);
            sum1 = _mm_add_pi16(sum1, T26);
            sum1 = _mm_add_pi16(sum1, T27);

            T10 = (*(__m64*)(fref3 + (i + 0) * frefstride));
            T11 = (*(__m64*)(fref3 + (i + 1) * frefstride));
            T12 = (*(__m64*)(fref3 + (i + 2) * frefstride));
            T13 = (*(__m64*)(fref3 + (i + 3) * frefstride));
            T14 = (*(__m64*)(fref3 + (i + 4) * frefstride));
            T15 = (*(__m64*)(fref3 + (i + 5) * frefstride));
            T16 = (*(__m64*)(fref3 + (i + 6) * frefstride));
            T17 = (*(__m64*)(fref3 + (i + 7) * frefstride));

            T20 = _mm_sad_pu8(T00, T10);
            T21 = _mm_sad_pu8(T01, T11);
            T22 = _mm_sad_pu8(T02, T12);
            T23 = _mm_sad_pu8(T03, T13);
            T24 = _mm_sad_pu8(T04, T14);
            T25 = _mm_sad_pu8(T05, T15);
            T26 = _mm_sad_pu8(T06, T16);
            T27 = _mm_sad_pu8(T07, T17);

            sum2 = _mm_add_pi16(sum2, T20);
            sum2 = _mm_add_pi16(sum2, T21);
            sum2 = _mm_add_pi16(sum2, T22);
            sum2 = _mm_add_pi16(sum2, T23);
            sum2 = _mm_add_pi16(sum2, T24);
            sum2 = _mm_add_pi16(sum2, T25);
            sum2 = _mm_add_pi16(sum2, T26);
            sum2 = _mm_add_pi16(sum2, T27);
        }

        res[0] = _m_to_int(sum0);
        res[1] = _m_to_int(sum1);
        res[2] = _m_to_int(sum2);
    }
    else
    {
        __m64 sum0 = _mm_setzero_si64();
        __m64 sum1 = _mm_setzero_si64();
        __m64 sum2 = _mm_setzero_si64();

        __m64 T00, T01, T02, T03;
        __m64 T10, T11, T12, T13;
        __m64 T20, T21, T22, T23;

        for (int i = 0; i < ly; i += 4)
        {
            T00 = (*(__m64*)(fenc + (i + 0) * FENC_STRIDE));
            T01 = (*(__m64*)(fenc + (i + 1) * FENC_STRIDE));
            T02 = (*(__m64*)(fenc + (i + 2) * FENC_STRIDE));
            T03 = (*(__m64*)(fenc + (i + 3) * FENC_STRIDE));

            T10 = (*(__m64*)(fref1 + (i + 0) * frefstride));
            T11 = (*(__m64*)(fref1 + (i + 1) * frefstride));
            T12 = (*(__m64*)(fref1 + (i + 2) * frefstride));
            T13 = (*(__m64*)(fref1 + (i + 3) * frefstride));

            T20 = _mm_sad_pu8(T00, T10);
            T21 = _mm_sad_pu8(T01, T11);
            T22 = _mm_sad_pu8(T02, T12);
            T23 = _mm_sad_pu8(T03, T13);

            sum0 = _mm_add_pi16(sum0, T20);
            sum0 = _mm_add_pi16(sum0, T21);
            sum0 = _mm_add_pi16(sum0, T22);
            sum0 = _mm_add_pi16(sum0, T23);

            T10 = (*(__m64*)(fref2 + (i + 0) * frefstride));
            T11 = (*(__m64*)(fref2 + (i + 1) * frefstride));
            T12 = (*(__m64*)(fref2 + (i + 2) * frefstride));
            T13 = (*(__m64*)(fref2 + (i + 3) * frefstride));

            T20 = _mm_sad_pu8(T00, T10);
            T21 = _mm_sad_pu8(T01, T11);
            T22 = _mm_sad_pu8(T02, T12);
            T23 = _mm_sad_pu8(T03, T13);

            sum1 = _mm_add_pi16(sum1, T20);
            sum1 = _mm_add_pi16(sum1, T21);
            sum1 = _mm_add_pi16(sum1, T22);
            sum1 = _mm_add_pi16(sum1, T23);

            T10 = (*(__m64*)(fref3 + (i + 0) * frefstride));
            T11 = (*(__m64*)(fref3 + (i + 1) * frefstride));
            T12 = (*(__m64*)(fref3 + (i + 2) * frefstride));
            T13 = (*(__m64*)(fref3 + (i + 3) * frefstride));

            T20 = _mm_sad_pu8(T00, T10);
            T21 = _mm_sad_pu8(T01, T11);
            T22 = _mm_sad_pu8(T02, T12);
            T23 = _mm_sad_pu8(T03, T13);

            sum2 = _mm_add_pi16(sum2, T20);
            sum2 = _mm_add_pi16(sum2, T21);
            sum2 = _mm_add_pi16(sum2, T22);
            sum2 = _mm_add_pi16(sum2, T23);
        }

        res[0] = _m_to_int(sum0);
        res[1] = _m_to_int(sum1);
        res[2] = _m_to_int(sum2);
    }
}

#else /* if HAVE_MMX */

template<int ly>
void sad_x3_8(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, intptr_t frefstride, int *res)
{
    assert((ly % 4) == 0);

    if (ly == 4)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21;
        __m128i sum0;

        T00 = _mm_loadl_epi64((__m128i*)(fenc));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + FENC_STRIDE));
        T01 = _mm_unpacklo_epi64(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (3) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[0] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref2));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[1] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref3));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[2] = _mm_cvtsi128_si32(sum0);
    }
    else if (ly == 8)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21;
        __m128i sum0;

        T00 = _mm_loadl_epi64((__m128i*)(fenc));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + FENC_STRIDE));
        T01 = _mm_unpacklo_epi64(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (3) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[0] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref2));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[1] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref3));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[2] = _mm_cvtsi128_si32(sum0);

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (4) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (5) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi64(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (6) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (7) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (5) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (7) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[0] = res[0] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (5) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (7) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[1] = res[1] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (5) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (7) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[2] = res[2] + _mm_cvtsi128_si32(sum0);
    }
    else if (ly == 16)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21;
        __m128i sum0;

        T00 = _mm_loadl_epi64((__m128i*)(fenc));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + FENC_STRIDE));
        T01 = _mm_unpacklo_epi64(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (3) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[0] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref2));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[1] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref3));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[2] = _mm_cvtsi128_si32(sum0);

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (4) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (5) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi64(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (6) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (7) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (5) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (7) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[0] = res[0] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (5) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (7) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[1] = res[1] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (5) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (7) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[2] = res[2] + _mm_cvtsi128_si32(sum0);

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (8) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (9) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi64(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (10) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (11) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (8) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (9) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (10) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (11) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[0] = res[0] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (8) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (9) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (10) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (11) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[1] = res[1] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (8) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (9) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (10) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (11) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[2] = res[2] + _mm_cvtsi128_si32(sum0);

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (12) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (13) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi64(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (14) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (15) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (12) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (13) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (14) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (15) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[0] = res[0] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (12) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (13) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (14) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (15) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[1] = res[1] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (12) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (13) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (14) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (15) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[2] = res[2] + _mm_cvtsi128_si32(sum0);
    }
    else if ((ly % 8) == 0)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21;
        __m128i sum0 = _mm_setzero_si128();

        res[0] = res[1] = res[2] = 0;
        for (int i = 0; i < ly; i += 8)
        {
            T00 = _mm_loadl_epi64((__m128i*)(fenc + (i + 0) * FENC_STRIDE));
            T01 = _mm_loadl_epi64((__m128i*)(fenc + (i + 1) * FENC_STRIDE));
            T01 = _mm_unpacklo_epi64(T00, T01);
            T02 = _mm_loadl_epi64((__m128i*)(fenc + (i + 2) * FENC_STRIDE));
            T03 = _mm_loadl_epi64((__m128i*)(fenc + (i + 3) * FENC_STRIDE));
            T03 = _mm_unpacklo_epi64(T02, T03);

            T10 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi64(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi64(T12, T13);

            T20 = _mm_sad_epu8(T01, T11);
            T21 = _mm_sad_epu8(T03, T13);
            T21 = _mm_add_epi32(T20, T21);
            sum0 = _mm_shuffle_epi32(T21, 2);
            sum0 = _mm_add_epi32(sum0, T21);
            res[0] = res[0] + _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi64(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi64(T12, T13);

            T20 = _mm_sad_epu8(T01, T11);
            T21 = _mm_sad_epu8(T03, T13);
            T21 = _mm_add_epi32(T20, T21);
            sum0 = _mm_shuffle_epi32(T21, 2);
            sum0 = _mm_add_epi32(sum0, T21);
            res[1] = res[1] + _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi64(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi64(T12, T13);

            T20 = _mm_sad_epu8(T01, T11);
            T21 = _mm_sad_epu8(T03, T13);
            T21 = _mm_add_epi32(T20, T21);
            sum0 = _mm_shuffle_epi32(T21, 2);
            sum0 = _mm_add_epi32(sum0, T21);
            res[2] = res[2] + _mm_cvtsi128_si32(sum0);

            T00 = _mm_loadl_epi64((__m128i*)(fenc + (i + 4) * FENC_STRIDE));
            T01 = _mm_loadl_epi64((__m128i*)(fenc + (i + 5) * FENC_STRIDE));
            T01 = _mm_unpacklo_epi64(T00, T01);
            T02 = _mm_loadl_epi64((__m128i*)(fenc + (i + 6) * FENC_STRIDE));
            T03 = _mm_loadl_epi64((__m128i*)(fenc + (i + 7) * FENC_STRIDE));
            T03 = _mm_unpacklo_epi64(T02, T03);

            T10 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 4) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 5) * frefstride));
            T11 = _mm_unpacklo_epi64(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 6) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 7) * frefstride));
            T13 = _mm_unpacklo_epi64(T12, T13);

            T20 = _mm_sad_epu8(T01, T11);
            T21 = _mm_sad_epu8(T03, T13);
            T21 = _mm_add_epi32(T20, T21);
            sum0 = _mm_shuffle_epi32(T21, 2);
            sum0 = _mm_add_epi32(sum0, T21);
            res[0] = res[0] + _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 4) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 5) * frefstride));
            T11 = _mm_unpacklo_epi64(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 6) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 7) * frefstride));
            T13 = _mm_unpacklo_epi64(T12, T13);

            T20 = _mm_sad_epu8(T01, T11);
            T21 = _mm_sad_epu8(T03, T13);
            T21 = _mm_add_epi32(T20, T21);
            sum0 = _mm_shuffle_epi32(T21, 2);
            sum0 = _mm_add_epi32(sum0, T21);
            res[1] = res[1] + _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 4) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 5) * frefstride));
            T11 = _mm_unpacklo_epi64(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 6) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 7) * frefstride));
            T13 = _mm_unpacklo_epi64(T12, T13);

            T20 = _mm_sad_epu8(T01, T11);
            T21 = _mm_sad_epu8(T03, T13);
            T21 = _mm_add_epi32(T20, T21);
            sum0 = _mm_shuffle_epi32(T21, 2);
            sum0 = _mm_add_epi32(sum0, T21);
            res[2] = res[2] + _mm_cvtsi128_si32(sum0);
        }
    }
    else
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21;
        __m128i sum0 = _mm_setzero_si128();

        res[0] = res[1] = res[2] = 0;
        for (int i = 0; i < ly; i += 4)
        {
            T00 = _mm_loadl_epi64((__m128i*)(fenc + (i + 0) * FENC_STRIDE));
            T01 = _mm_loadl_epi64((__m128i*)(fenc + (i + 1) * FENC_STRIDE));
            T01 = _mm_unpacklo_epi64(T00, T01);
            T02 = _mm_loadl_epi64((__m128i*)(fenc + (i + 2) * FENC_STRIDE));
            T03 = _mm_loadl_epi64((__m128i*)(fenc + (i + 3) * FENC_STRIDE));
            T03 = _mm_unpacklo_epi64(T02, T03);

            T10 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi64(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi64(T12, T13);

            T20 = _mm_sad_epu8(T01, T11);
            T21 = _mm_sad_epu8(T03, T13);
            T21 = _mm_add_epi32(T20, T21);
            sum0 = _mm_shuffle_epi32(T21, 2);
            sum0 = _mm_add_epi32(sum0, T21);
            res[0] = res[0] + _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi64(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi64(T12, T13);

            T20 = _mm_sad_epu8(T01, T11);
            T21 = _mm_sad_epu8(T03, T13);
            T21 = _mm_add_epi32(T20, T21);
            sum0 = _mm_shuffle_epi32(T21, 2);
            sum0 = _mm_add_epi32(sum0, T21);
            res[1] = res[1] + _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi64(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi64(T12, T13);

            T20 = _mm_sad_epu8(T01, T11);
            T21 = _mm_sad_epu8(T03, T13);
            T21 = _mm_add_epi32(T20, T21);
            sum0 = _mm_shuffle_epi32(T21, 2);
            sum0 = _mm_add_epi32(sum0, T21);
            res[2] = res[2] + _mm_cvtsi128_si32(sum0);
        }
    }
}

#endif /* if HAVE_MMX */

/* For performance - This function assumes that the *last load* can access 16 elements. */

template<int ly>
void sad_x3_12(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, intptr_t frefstride, int *res)
{
    assert((ly % 4) == 0);

    __m128i mask = _mm_set_epi32(0x0, 0xffffffff, 0xffffffff, 0xffffffff);

    if(ly == 4)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;
        __m128i sum0, sum1;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T00 = _mm_and_si128(T00, mask);
        T01 = _mm_load_si128((__m128i*)(fenc + FENC_STRIDE));
        T01 = _mm_and_si128(T01, mask);
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * FENC_STRIDE));
        T02 = _mm_and_si128(T02, mask);
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * FENC_STRIDE));
        T03 = _mm_and_si128(T03, mask);

        T10 = _mm_loadu_si128((__m128i*)(fref1));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref1 + frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (2) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (3) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref2 + frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (2) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (3) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref3 + frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (2) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (3) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] = _mm_cvtsi128_si32(sum0);
    }
    if (ly == 8)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;
        __m128i sum0, sum1;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T00 = _mm_and_si128(T00, mask);
        T01 = _mm_load_si128((__m128i*)(fenc + FENC_STRIDE));
        T01 = _mm_and_si128(T01, mask);
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * FENC_STRIDE));
        T02 = _mm_and_si128(T02, mask);
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * FENC_STRIDE));
        T03 = _mm_and_si128(T03, mask);

        T10 = _mm_loadu_si128((__m128i*)(fref1));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref1 + frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (2) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (3) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref2 + frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (2) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (3) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref3 + frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (2) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (3) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] = _mm_cvtsi128_si32(sum0);

        T00 = _mm_load_si128((__m128i*)(fenc + (4) * FENC_STRIDE));
        T00 = _mm_and_si128(T00, mask);
        T01 = _mm_load_si128((__m128i*)(fenc + (5) * FENC_STRIDE));
        T01 = _mm_and_si128(T01, mask);
        T02 = _mm_load_si128((__m128i*)(fenc + (6) * FENC_STRIDE));
        T02 = _mm_and_si128(T02, mask);
        T03 = _mm_load_si128((__m128i*)(fenc + (7) * FENC_STRIDE));
        T03 = _mm_and_si128(T03, mask);

        T10 = _mm_loadu_si128((__m128i*)(fref1 + (4) * frefstride));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref1 + (5) * frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (6) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (7) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + (4) * frefstride));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref2 + (5) * frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (6) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (7) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + (4) * frefstride));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref3 + (5) * frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (6) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (7) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] += _mm_cvtsi128_si32(sum0);
    }
    if (ly == 16)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;
        __m128i sum0, sum1;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T00 = _mm_and_si128(T00, mask);
        T01 = _mm_load_si128((__m128i*)(fenc + FENC_STRIDE));
        T01 = _mm_and_si128(T01, mask);
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * FENC_STRIDE));
        T02 = _mm_and_si128(T02, mask);
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * FENC_STRIDE));
        T03 = _mm_and_si128(T03, mask);

        T10 = _mm_loadu_si128((__m128i*)(fref1));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref1 + frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (2) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (3) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref2 + frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (2) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (3) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref3 + frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (2) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (3) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] = _mm_cvtsi128_si32(sum0);

        T00 = _mm_load_si128((__m128i*)(fenc + (4) * FENC_STRIDE));
        T00 = _mm_and_si128(T00, mask);
        T01 = _mm_load_si128((__m128i*)(fenc + (5) * FENC_STRIDE));
        T01 = _mm_and_si128(T01, mask);
        T02 = _mm_load_si128((__m128i*)(fenc + (6) * FENC_STRIDE));
        T02 = _mm_and_si128(T02, mask);
        T03 = _mm_load_si128((__m128i*)(fenc + (7) * FENC_STRIDE));
        T03 = _mm_and_si128(T03, mask);

        T10 = _mm_loadu_si128((__m128i*)(fref1 + (4) * frefstride));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref1 + (5) * frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (6) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (7) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + (4) * frefstride));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref2 + (5) * frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (6) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (7) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + (4) * frefstride));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref3 + (5) * frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (6) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (7) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] += _mm_cvtsi128_si32(sum0);

        T00 = _mm_load_si128((__m128i*)(fenc + (8) * FENC_STRIDE));
        T00 = _mm_and_si128(T00, mask);
        T01 = _mm_load_si128((__m128i*)(fenc + (9) * FENC_STRIDE));
        T01 = _mm_and_si128(T01, mask);
        T02 = _mm_load_si128((__m128i*)(fenc + (10) * FENC_STRIDE));
        T02 = _mm_and_si128(T02, mask);
        T03 = _mm_load_si128((__m128i*)(fenc + (11) * FENC_STRIDE));
        T03 = _mm_and_si128(T03, mask);

        T10 = _mm_loadu_si128((__m128i*)(fref1 + (8) * frefstride));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref1 + (9) * frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (10) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (11) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + (8) * frefstride));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref2 + (9) * frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (10) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (11) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + (8) * frefstride));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref3 + (9) * frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (10) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (11) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] += _mm_cvtsi128_si32(sum0);

        T00 = _mm_load_si128((__m128i*)(fenc + (12) * FENC_STRIDE));
        T00 = _mm_and_si128(T00, mask);
        T01 = _mm_load_si128((__m128i*)(fenc + (13) * FENC_STRIDE));
        T01 = _mm_and_si128(T01, mask);
        T02 = _mm_load_si128((__m128i*)(fenc + (14) * FENC_STRIDE));
        T02 = _mm_and_si128(T02, mask);
        T03 = _mm_load_si128((__m128i*)(fenc + (15) * FENC_STRIDE));
        T03 = _mm_and_si128(T03, mask);

        T10 = _mm_loadu_si128((__m128i*)(fref1 + (12) * frefstride));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref1 + (13) * frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (14) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (15) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + (12) * frefstride));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref2 + (13) * frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (14) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (15) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + (12) * frefstride));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref3 + (13) * frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (14) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (15) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] += _mm_cvtsi128_si32(sum0);
    }
    if ((ly % 8) == 0)
    {
        res[0] = res[1] = res[2] = 0;
        for(int i = 0; i < ly; i += 8)
        {
            __m128i T00, T01, T02, T03;
            __m128i T10, T11, T12, T13;
            __m128i T20, T21, T22, T23;
            __m128i sum0, sum1;

            T00 = _mm_load_si128((__m128i*)(fenc + (i) * FENC_STRIDE));
            T00 = _mm_and_si128(T00, mask);
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 1) * FENC_STRIDE));
            T01 = _mm_and_si128(T01, mask);
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 2) * FENC_STRIDE));
            T02 = _mm_and_si128(T02, mask);
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 3) * FENC_STRIDE));
            T03 = _mm_and_si128(T03, mask);

            T10 = _mm_loadu_si128((__m128i*)(fref1 + (i) * frefstride));
            T10 = _mm_and_si128(T10, mask);
            T11 = _mm_loadu_si128((__m128i*)(fref1 + (i + 1) * frefstride));
            T11 = _mm_and_si128(T11, mask);
            T12 = _mm_loadu_si128((__m128i*)(fref1 + (i + 2) * frefstride));
            T12 = _mm_and_si128(T12, mask);
            T13 = _mm_loadu_si128((__m128i*)(fref1 + (i + 3) * frefstride));
            T13 = _mm_and_si128(T13, mask);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22); 

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[0] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref2 + (i) * frefstride));
            T10 = _mm_and_si128(T10, mask);
            T11 = _mm_loadu_si128((__m128i*)(fref2 + (i + 1) * frefstride));
            T11 = _mm_and_si128(T11, mask);
            T12 = _mm_loadu_si128((__m128i*)(fref2 + (i + 2) * frefstride));
            T12 = _mm_and_si128(T12, mask);
            T13 = _mm_loadu_si128((__m128i*)(fref2 + (i + 3) * frefstride));
            T13 = _mm_and_si128(T13, mask);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22); 

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[1] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref3 + (i) * frefstride));
            T10 = _mm_and_si128(T10, mask);
            T11 = _mm_loadu_si128((__m128i*)(fref3 + (i + 1) * frefstride));
            T11 = _mm_and_si128(T11, mask);
            T12 = _mm_loadu_si128((__m128i*)(fref3 + (i + 2) * frefstride));
            T12 = _mm_and_si128(T12, mask);
            T13 = _mm_loadu_si128((__m128i*)(fref3 + (i + 3) * frefstride));
            T13 = _mm_and_si128(T13, mask);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22); 

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[2] += _mm_cvtsi128_si32(sum0);

            T00 = _mm_load_si128((__m128i*)(fenc + (i + 4) * FENC_STRIDE));
            T00 = _mm_and_si128(T00, mask);
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 5) * FENC_STRIDE));
            T01 = _mm_and_si128(T01, mask);
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 6) * FENC_STRIDE));
            T02 = _mm_and_si128(T02, mask);
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 7) * FENC_STRIDE));
            T03 = _mm_and_si128(T03, mask);

            T10 = _mm_loadu_si128((__m128i*)(fref1 + (i + 4) * frefstride));
            T10 = _mm_and_si128(T10, mask);
            T11 = _mm_loadu_si128((__m128i*)(fref1 + (i + 5) * frefstride));
            T11 = _mm_and_si128(T11, mask);
            T12 = _mm_loadu_si128((__m128i*)(fref1 + (i + 6) * frefstride));
            T12 = _mm_and_si128(T12, mask);
            T13 = _mm_loadu_si128((__m128i*)(fref1 + (i + 7) * frefstride));
            T13 = _mm_and_si128(T13, mask);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22); 

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[0] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref2 + (i + 4) * frefstride));
            T10 = _mm_and_si128(T10, mask);
            T11 = _mm_loadu_si128((__m128i*)(fref2 + (i + 5) * frefstride));
            T11 = _mm_and_si128(T11, mask);
            T12 = _mm_loadu_si128((__m128i*)(fref2 + (i + 6) * frefstride));
            T12 = _mm_and_si128(T12, mask);
            T13 = _mm_loadu_si128((__m128i*)(fref2 + (i + 7) * frefstride));
            T13 = _mm_and_si128(T13, mask);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22); 

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[1] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref3 + (i + 4) * frefstride));
            T10 = _mm_and_si128(T10, mask);
            T11 = _mm_loadu_si128((__m128i*)(fref3 + (i + 5) * frefstride));
            T11 = _mm_and_si128(T11, mask);
            T12 = _mm_loadu_si128((__m128i*)(fref3 + (i + 6) * frefstride));
            T12 = _mm_and_si128(T12, mask);
            T13 = _mm_loadu_si128((__m128i*)(fref3 + (i + 7) * frefstride));
            T13 = _mm_and_si128(T13, mask);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22); 

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[2] += _mm_cvtsi128_si32(sum0);
        }
    }
    else
    {
        res[0] = res[1] = res[2] = 0;
        for(int i = 0; i < ly; i += 4)
        {
            __m128i T00, T01, T02, T03;
            __m128i T10, T11, T12, T13;
            __m128i T20, T21, T22, T23;
            __m128i sum0, sum1;

            T00 = _mm_load_si128((__m128i*)(fenc + (i) * FENC_STRIDE));
            T00 = _mm_and_si128(T00, mask);
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 1) * FENC_STRIDE));
            T01 = _mm_and_si128(T01, mask);
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 2) * FENC_STRIDE));
            T02 = _mm_and_si128(T02, mask);
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 3) * FENC_STRIDE));
            T03 = _mm_and_si128(T03, mask);

            T10 = _mm_loadu_si128((__m128i*)(fref1 + (i) * frefstride));
            T10 = _mm_and_si128(T10, mask);
            T11 = _mm_loadu_si128((__m128i*)(fref1 + (i + 1) * frefstride));
            T11 = _mm_and_si128(T11, mask);
            T12 = _mm_loadu_si128((__m128i*)(fref1 + (i + 2) * frefstride));
            T12 = _mm_and_si128(T12, mask);
            T13 = _mm_loadu_si128((__m128i*)(fref1 + (i + 3) * frefstride));
            T13 = _mm_and_si128(T13, mask);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22); 

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[0] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref2 + (i) * frefstride));
            T10 = _mm_and_si128(T10, mask);
            T11 = _mm_loadu_si128((__m128i*)(fref2 + (i + 1) * frefstride));
            T11 = _mm_and_si128(T11, mask);
            T12 = _mm_loadu_si128((__m128i*)(fref2 + (i + 2) * frefstride));
            T12 = _mm_and_si128(T12, mask);
            T13 = _mm_loadu_si128((__m128i*)(fref2 + (i + 3) * frefstride));
            T13 = _mm_and_si128(T13, mask);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22); 

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[1] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref3 + (i) * frefstride));
            T10 = _mm_and_si128(T10, mask);
            T11 = _mm_loadu_si128((__m128i*)(fref3 + (i + 1) * frefstride));
            T11 = _mm_and_si128(T11, mask);
            T12 = _mm_loadu_si128((__m128i*)(fref3 + (i + 2) * frefstride));
            T12 = _mm_and_si128(T12, mask);
            T13 = _mm_loadu_si128((__m128i*)(fref3 + (i + 3) * frefstride));
            T13 = _mm_and_si128(T13, mask);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22); 

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[2] += _mm_cvtsi128_si32(sum0);
        }
    }
}

template<int ly>
void sad_x3_16(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, intptr_t frefstride, int *res)
{
    assert((ly % 4) == 0);

    if (ly == 4)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;
        __m128i sum0, sum1;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T01 = _mm_load_si128((__m128i*)(fenc + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] = _mm_cvtsi128_si32(sum0);
    }
    else if (ly == 8)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;
        __m128i sum0, sum1;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T01 = _mm_load_si128((__m128i*)(fenc + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] = _mm_cvtsi128_si32(sum0);

        T00 = _mm_load_si128((__m128i*)(fenc + (4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + (5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (7) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] = res[0] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] = res[1] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] = res[2] + _mm_cvtsi128_si32(sum0);
    }
    else if (ly == 16)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;
        __m128i sum0, sum1;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T01 = _mm_load_si128((__m128i*)(fenc + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] = _mm_cvtsi128_si32(sum0);

        T00 = _mm_load_si128((__m128i*)(fenc + (4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + (5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (7) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] = res[0] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] = res[1] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] = res[2] + _mm_cvtsi128_si32(sum0);

        T00 = _mm_load_si128((__m128i*)(fenc + (8) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + (9) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (10) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (11) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] = res[0] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] = res[1] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] = res[2] + _mm_cvtsi128_si32(sum0);

        T00 = _mm_load_si128((__m128i*)(fenc + (12) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + (13) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (14) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (15) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] = res[0] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] = res[1] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] = res[2] + _mm_cvtsi128_si32(sum0);
    }
    else if ((ly % 8) == 0)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;
        __m128i sum0, sum1;

        res[0] = res[1] = res[2] = 0;
        for (int i = 0; i < ly; i += 8)
        {
            T00 = _mm_load_si128((__m128i*)(fenc + (i + 0) * FENC_STRIDE));
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 1) * FENC_STRIDE));
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 2) * FENC_STRIDE));
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 3) * FENC_STRIDE));

            T10 = _mm_loadu_si128((__m128i*)(fref1 + (i + 0) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref1 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref1 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref1 + (i + 3) * frefstride));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[0] = res[0] + _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref2 + (i + 0) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref2 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref2 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref2 + (i + 3) * frefstride));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[1] = res[1] + _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref3 + (i + 0) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref3 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref3 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref3 + (i + 3) * frefstride));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[2] = res[2] + _mm_cvtsi128_si32(sum0);

            T00 = _mm_load_si128((__m128i*)(fenc + (i + 4) * FENC_STRIDE));
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 5) * FENC_STRIDE));
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 6) * FENC_STRIDE));
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 7) * FENC_STRIDE));

            T10 = _mm_loadu_si128((__m128i*)(fref1 + (i + 4) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref1 + (i + 5) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref1 + (i + 6) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref1 + (i + 7) * frefstride));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[0] = res[0] + _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref2 + (i + 4) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref2 + (i + 5) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref2 + (i + 6) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref2 + (i + 7) * frefstride));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[1] = res[1] + _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref3 + (i + 4) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref3 + (i + 5) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref3 + (i + 6) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref3 + (i + 7) * frefstride));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[2] = res[2] + _mm_cvtsi128_si32(sum0);
        }
    }
    else
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;
        __m128i sum0, sum1;

        res[0] = res[1] = res[2] = 0;
        for (int i = 0; i < ly; i += 4)
        {
            T00 = _mm_load_si128((__m128i*)(fenc + (i + 0) * FENC_STRIDE));
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 1) * FENC_STRIDE));
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 2) * FENC_STRIDE));
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 3) * FENC_STRIDE));

            T10 = _mm_loadu_si128((__m128i*)(fref1 + (i + 0) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref1 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref1 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref1 + (i + 3) * frefstride));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[0] = res[0] + _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref2 + (i + 0) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref2 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref2 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref2 + (i + 3) * frefstride));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[1] = res[1] + _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref3 + (i + 0) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref3 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref3 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref3 + (i + 3) * frefstride));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[2] = res[2] + _mm_cvtsi128_si32(sum0);
        }
    }
}

template<int ly>
void sad_x3_24(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, intptr_t frefstride, int *res)
{
    assert((ly % 4) == 0);

    if (ly == 4)
    {
        __m128i T00, T01, T02, T03, T04, T05;
        __m128i T10, T11, T12, T13, T14, T15;
        __m128i T20, T21, T22, T23;
        __m128i T30, T31;

        __m128i sum0, sum1;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T01 = _mm_load_si128((__m128i*)(fenc + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * FENC_STRIDE));

        T10 = _mm_loadl_epi64((__m128i*)(fenc + 16));
        T11 = _mm_loadl_epi64((__m128i*)(fenc + (FENC_STRIDE + 16)));
        T04 = _mm_unpacklo_epi64(T10, T11);

        T12 = _mm_loadl_epi64((__m128i*)(fenc + ((2) * FENC_STRIDE) + 16));
        T13 = _mm_loadl_epi64((__m128i*)(fenc + ((3) * FENC_STRIDE) + 16));
        T05 = _mm_unpacklo_epi64(T12, T13);

        T10 = _mm_loadu_si128((__m128i*)(fref1));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (3) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref1 + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref1 + (frefstride + 16)));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref1 + ((2) * frefstride) + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref1 + ((3) * frefstride) + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (3) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref2 + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref2 + (frefstride + 16)));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref2 + ((2) * frefstride) + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref2 + ((3) * frefstride) + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (3) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref3 + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref3 + (frefstride + 16)));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref3 + ((2) * frefstride) + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref3 + ((3) * frefstride) + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] = _mm_cvtsi128_si32(sum0);
    }
    else if (ly == 8)
    {
        __m128i T00, T01, T02, T03, T04, T05;
        __m128i T10, T11, T12, T13, T14, T15;
        __m128i T20, T21, T22, T23;
        __m128i T30, T31;

        __m128i sum0, sum1;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T01 = _mm_load_si128((__m128i*)(fenc + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * FENC_STRIDE));

        T10 = _mm_loadl_epi64((__m128i*)(fenc + 16));
        T11 = _mm_loadl_epi64((__m128i*)(fenc + (FENC_STRIDE + 16)));
        T04 = _mm_unpacklo_epi64(T10, T11);

        T12 = _mm_loadl_epi64((__m128i*)(fenc + ((2) * FENC_STRIDE) + 16));
        T13 = _mm_loadl_epi64((__m128i*)(fenc + ((3) * FENC_STRIDE) + 16));
        T05 = _mm_unpacklo_epi64(T12, T13);

        T10 = _mm_loadu_si128((__m128i*)(fref1));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (3) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref1 + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref1 + (frefstride + 16)));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref1 + ((2) * frefstride) + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref1 + ((3) * frefstride) + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (3) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref2 + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref2 + (frefstride + 16)));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref2 + ((2) * frefstride) + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref2 + ((3) * frefstride) + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (3) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref3 + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref3 + (frefstride + 16)));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref3 + ((2) * frefstride) + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref3 + ((3) * frefstride) + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] = _mm_cvtsi128_si32(sum0);

        T00 = _mm_load_si128((__m128i*)(fenc + (4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + (5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (7) * FENC_STRIDE));

        T10 = _mm_loadl_epi64((__m128i*)(fenc + (4) * FENC_STRIDE + 16));
        T11 = _mm_loadl_epi64((__m128i*)(fenc + (5) * FENC_STRIDE + 16));
        T04 = _mm_unpacklo_epi64(T10, T11);

        T12 = _mm_loadl_epi64((__m128i*)(fenc + (6) * FENC_STRIDE + 16));
        T13 = _mm_loadl_epi64((__m128i*)(fenc + (7) * FENC_STRIDE + 16));
        T05 = _mm_unpacklo_epi64(T12, T13);

        T10 = _mm_loadu_si128((__m128i*)(fref1 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (7) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref1 + (4) * frefstride + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref1 + (5) * frefstride + 16));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref1 + (6) * frefstride + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref1 + (7) * frefstride + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (7) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref2 + (4) * frefstride + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref2 + (5) * frefstride + 16));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref2 + (6) * frefstride + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref2 + (7) * frefstride + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (7) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref3 + (4) * frefstride + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref3 + (5) * frefstride + 16));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref3 + (6) * frefstride + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref3 + (7) * frefstride + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] += _mm_cvtsi128_si32(sum0);
    }
    else if (ly == 16)
    {
        __m128i T00, T01, T02, T03, T04, T05;
        __m128i T10, T11, T12, T13, T14, T15;
        __m128i T20, T21, T22, T23;
        __m128i T30, T31;

        __m128i sum0, sum1;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T01 = _mm_load_si128((__m128i*)(fenc + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * FENC_STRIDE));

        T10 = _mm_loadl_epi64((__m128i*)(fenc + 16));
        T11 = _mm_loadl_epi64((__m128i*)(fenc + (FENC_STRIDE + 16)));
        T04 = _mm_unpacklo_epi64(T10, T11);

        T12 = _mm_loadl_epi64((__m128i*)(fenc + ((2) * FENC_STRIDE) + 16));
        T13 = _mm_loadl_epi64((__m128i*)(fenc + ((3) * FENC_STRIDE) + 16));
        T05 = _mm_unpacklo_epi64(T12, T13);

        T10 = _mm_loadu_si128((__m128i*)(fref1));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (3) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref1 + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref1 + (frefstride + 16)));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref1 + ((2) * frefstride) + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref1 + ((3) * frefstride) + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (3) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref2 + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref2 + (frefstride + 16)));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref2 + ((2) * frefstride) + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref2 + ((3) * frefstride) + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (3) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref3 + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref3 + (frefstride + 16)));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref3 + ((2) * frefstride) + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref3 + ((3) * frefstride) + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] = _mm_cvtsi128_si32(sum0);

        T00 = _mm_load_si128((__m128i*)(fenc + (4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + (5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (7) * FENC_STRIDE));

        T10 = _mm_loadl_epi64((__m128i*)(fenc + (4) * FENC_STRIDE + 16));
        T11 = _mm_loadl_epi64((__m128i*)(fenc + (5) * FENC_STRIDE + 16));
        T04 = _mm_unpacklo_epi64(T10, T11);

        T12 = _mm_loadl_epi64((__m128i*)(fenc + (6) * FENC_STRIDE + 16));
        T13 = _mm_loadl_epi64((__m128i*)(fenc + (7) * FENC_STRIDE + 16));
        T05 = _mm_unpacklo_epi64(T12, T13);

        T10 = _mm_loadu_si128((__m128i*)(fref1 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (7) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref1 + (4) * frefstride + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref1 + (5) * frefstride + 16));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref1 + (6) * frefstride + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref1 + (7) * frefstride + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (7) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref2 + (4) * frefstride + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref2 + (5) * frefstride + 16));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref2 + (6) * frefstride + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref2 + (7) * frefstride + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (7) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref3 + (4) * frefstride + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref3 + (5) * frefstride + 16));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref3 + (6) * frefstride + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref3 + (7) * frefstride + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] += _mm_cvtsi128_si32(sum0);

        T00 = _mm_load_si128((__m128i*)(fenc + (8) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + (9) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (10) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (11) * FENC_STRIDE));

        T10 = _mm_loadl_epi64((__m128i*)(fenc + (8) * FENC_STRIDE + 16));
        T11 = _mm_loadl_epi64((__m128i*)(fenc + (9) * FENC_STRIDE + 16));
        T04 = _mm_unpacklo_epi64(T10, T11);

        T12 = _mm_loadl_epi64((__m128i*)(fenc + (10) * FENC_STRIDE + 16));
        T13 = _mm_loadl_epi64((__m128i*)(fenc + (11) * FENC_STRIDE + 16));
        T05 = _mm_unpacklo_epi64(T12, T13);

        T10 = _mm_loadu_si128((__m128i*)(fref1 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (11) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref1 + (8) * frefstride + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref1 + (9) * frefstride + 16));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref1 + (10) * frefstride + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref1 + (11) * frefstride + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (11) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref2 + (8) * frefstride + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref2 + (9) * frefstride + 16));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref2 + (10) * frefstride + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref2 + (11) * frefstride + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (11) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref3 + (8) * frefstride + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref3 + (9) * frefstride + 16));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref3 + (10) * frefstride + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref3 + (11) * frefstride + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] += _mm_cvtsi128_si32(sum0);

        T00 = _mm_load_si128((__m128i*)(fenc + (12) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + (13) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (14) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (15) * FENC_STRIDE));

        T10 = _mm_loadl_epi64((__m128i*)(fenc + (12) * FENC_STRIDE + 16));
        T11 = _mm_loadl_epi64((__m128i*)(fenc + (13) * FENC_STRIDE + 16));
        T04 = _mm_unpacklo_epi64(T10, T11);

        T12 = _mm_loadl_epi64((__m128i*)(fenc + (14) * FENC_STRIDE + 16));
        T13 = _mm_loadl_epi64((__m128i*)(fenc + (15) * FENC_STRIDE + 16));
        T05 = _mm_unpacklo_epi64(T12, T13);

        T10 = _mm_loadu_si128((__m128i*)(fref1 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (15) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref1 + (12) * frefstride + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref1 + (13) * frefstride + 16));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref1 + (14) * frefstride + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref1 + (15) * frefstride + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (15) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref2 + (12) * frefstride + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref2 + (13) * frefstride + 16));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref2 + (14) * frefstride + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref2 + (15) * frefstride + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (15) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref3 + (12) * frefstride + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref3 + (13) * frefstride + 16));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref3 + (14) * frefstride + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref3 + (15) * frefstride + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] += _mm_cvtsi128_si32(sum0);
    }
    else if ((ly % 8) == 0)
    {
        res[0] = res[1] = res[2] = 0;
        for(int i = 0; i < ly; i += 8)
        {
            __m128i T00, T01, T02, T03, T04, T05;
            __m128i T10, T11, T12, T13, T14, T15;
            __m128i T20, T21, T22, T23;
            __m128i T30, T31;

            __m128i sum0, sum1;

            T00 = _mm_load_si128((__m128i*)(fenc + (i) * FENC_STRIDE));
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 1) * FENC_STRIDE));
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 2) * FENC_STRIDE));
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 3) * FENC_STRIDE));

            T10 = _mm_loadl_epi64((__m128i*)(fenc + (i) * FENC_STRIDE + 16));
            T11 = _mm_loadl_epi64((__m128i*)(fenc + (i + 1) * FENC_STRIDE + 16));
            T04 = _mm_unpacklo_epi64(T10, T11);

            T12 = _mm_loadl_epi64((__m128i*)(fenc + (i + 2) * FENC_STRIDE + 16));
            T13 = _mm_loadl_epi64((__m128i*)(fenc + (i + 3) * FENC_STRIDE + 16));
            T05 = _mm_unpacklo_epi64(T12, T13);

            T10 = _mm_loadu_si128((__m128i*)(fref1 + (i) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref1 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref1 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref1 + (i + 3) * frefstride));

            T20 = _mm_loadl_epi64((__m128i*)(fref1 + (i) * frefstride + 16));
            T21 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 1) * frefstride + 16));
            T14 = _mm_unpacklo_epi64(T20, T21);

            T22 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 2) * frefstride + 16));
            T23 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 3) * frefstride + 16));
            T15 = _mm_unpacklo_epi64(T22, T23);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);
            T30 = _mm_sad_epu8(T04, T14);
            T31 = _mm_sad_epu8(T05, T15);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);
            sum1 = _mm_add_epi16(T30, T31);
            sum0 = _mm_add_epi16(sum0, sum1);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[0] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref2 + (i) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref2 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref2 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref2 + (i + 3) * frefstride));

            T20 = _mm_loadl_epi64((__m128i*)(fref2 + (i) * frefstride + 16));
            T21 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 1) * frefstride + 16));
            T14 = _mm_unpacklo_epi64(T20, T21);

            T22 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 2) * frefstride + 16));
            T23 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 3) * frefstride + 16));
            T15 = _mm_unpacklo_epi64(T22, T23);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);
            T30 = _mm_sad_epu8(T04, T14);
            T31 = _mm_sad_epu8(T05, T15);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);
            sum1 = _mm_add_epi16(T30, T31);
            sum0 = _mm_add_epi16(sum0, sum1);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[1] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref3 + (i) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref3 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref3 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref3 + (i + 3) * frefstride));

            T20 = _mm_loadl_epi64((__m128i*)(fref3 + (i) * frefstride + 16));
            T21 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 1) * frefstride + 16));
            T14 = _mm_unpacklo_epi64(T20, T21);

            T22 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 2) * frefstride + 16));
            T23 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 3) * frefstride + 16));
            T15 = _mm_unpacklo_epi64(T22, T23);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);
            T30 = _mm_sad_epu8(T04, T14);
            T31 = _mm_sad_epu8(T05, T15);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);
            sum1 = _mm_add_epi16(T30, T31);
            sum0 = _mm_add_epi16(sum0, sum1);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[2] += _mm_cvtsi128_si32(sum0);

            T00 = _mm_load_si128((__m128i*)(fenc + (i + 4) * FENC_STRIDE));
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 5) * FENC_STRIDE));
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 6) * FENC_STRIDE));
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 7) * FENC_STRIDE));

            T10 = _mm_loadl_epi64((__m128i*)(fenc + (i + 4) * FENC_STRIDE + 16));
            T11 = _mm_loadl_epi64((__m128i*)(fenc + (i + 5) * FENC_STRIDE + 16));
            T04 = _mm_unpacklo_epi64(T10, T11);

            T12 = _mm_loadl_epi64((__m128i*)(fenc + (i + 6) * FENC_STRIDE + 16));
            T13 = _mm_loadl_epi64((__m128i*)(fenc + (i + 7) * FENC_STRIDE + 16));
            T05 = _mm_unpacklo_epi64(T12, T13);

            T10 = _mm_loadu_si128((__m128i*)(fref1 + (i + 4) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref1 + (i + 5) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref1 + (i + 6) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref1 + (i + 7) * frefstride));

            T20 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 4) * frefstride + 16));
            T21 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 5) * frefstride + 16));
            T14 = _mm_unpacklo_epi64(T20, T21);

            T22 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 6) * frefstride + 16));
            T23 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 7) * frefstride + 16));
            T15 = _mm_unpacklo_epi64(T22, T23);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);
            T30 = _mm_sad_epu8(T04, T14);
            T31 = _mm_sad_epu8(T05, T15);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);
            sum1 = _mm_add_epi16(T30, T31);
            sum0 = _mm_add_epi16(sum0, sum1);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[0] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref2 + (i + 4) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref2 + (i + 5) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref2 + (i + 6) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref2 + (i + 7) * frefstride));

            T20 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 4) * frefstride + 16));
            T21 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 5) * frefstride + 16));
            T14 = _mm_unpacklo_epi64(T20, T21);

            T22 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 6) * frefstride + 16));
            T23 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 7) * frefstride + 16));
            T15 = _mm_unpacklo_epi64(T22, T23);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);
            T30 = _mm_sad_epu8(T04, T14);
            T31 = _mm_sad_epu8(T05, T15);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);
            sum1 = _mm_add_epi16(T30, T31);
            sum0 = _mm_add_epi16(sum0, sum1);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[1] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref3 + (i + 4) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref3 + (i + 5) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref3 + (i + 6) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref3 + (i + 7) * frefstride));

            T20 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 4) * frefstride + 16));
            T21 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 5) * frefstride + 16));
            T14 = _mm_unpacklo_epi64(T20, T21);

            T22 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 6) * frefstride + 16));
            T23 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 7) * frefstride + 16));
            T15 = _mm_unpacklo_epi64(T22, T23);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);
            T30 = _mm_sad_epu8(T04, T14);
            T31 = _mm_sad_epu8(T05, T15);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);
            sum1 = _mm_add_epi16(T30, T31);
            sum0 = _mm_add_epi16(sum0, sum1);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[2] += _mm_cvtsi128_si32(sum0);
        }
    }
    else
    {
        res[0] = res[1] = res[2] = 0;
        for(int i = 0; i < ly; i += 4)
        {
            __m128i T00, T01, T02, T03, T04, T05;
            __m128i T10, T11, T12, T13, T14, T15;
            __m128i T20, T21, T22, T23;
            __m128i T30, T31;

            __m128i sum0, sum1;

            T00 = _mm_load_si128((__m128i*)(fenc + (i) * FENC_STRIDE));
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 1) * FENC_STRIDE));
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 2) * FENC_STRIDE));
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 3) * FENC_STRIDE));

            T10 = _mm_loadl_epi64((__m128i*)(fenc + (i) * FENC_STRIDE + 16));
            T11 = _mm_loadl_epi64((__m128i*)(fenc + (i + 1) * FENC_STRIDE + 16));
            T04 = _mm_unpacklo_epi64(T10, T11);

            T12 = _mm_loadl_epi64((__m128i*)(fenc + (i + 2) * FENC_STRIDE + 16));
            T13 = _mm_loadl_epi64((__m128i*)(fenc + (i + 3) * FENC_STRIDE + 16));
            T05 = _mm_unpacklo_epi64(T12, T13);

            T10 = _mm_loadu_si128((__m128i*)(fref1 + (i) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref1 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref1 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref1 + (i + 3) * frefstride));

            T20 = _mm_loadl_epi64((__m128i*)(fref1 + (i) * frefstride + 16));
            T21 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 1) * frefstride + 16));
            T14 = _mm_unpacklo_epi64(T20, T21);

            T22 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 2) * frefstride + 16));
            T23 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 3) * frefstride + 16));
            T15 = _mm_unpacklo_epi64(T22, T23);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);
            T30 = _mm_sad_epu8(T04, T14);
            T31 = _mm_sad_epu8(T05, T15);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);
            sum1 = _mm_add_epi16(T30, T31);
            sum0 = _mm_add_epi16(sum0, sum1);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[0] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref2 + (i) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref2 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref2 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref2 + (i + 3) * frefstride));

            T20 = _mm_loadl_epi64((__m128i*)(fref2 + (i) * frefstride + 16));
            T21 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 1) * frefstride + 16));
            T14 = _mm_unpacklo_epi64(T20, T21);

            T22 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 2) * frefstride + 16));
            T23 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 3) * frefstride + 16));
            T15 = _mm_unpacklo_epi64(T22, T23);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);
            T30 = _mm_sad_epu8(T04, T14);
            T31 = _mm_sad_epu8(T05, T15);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);
            sum1 = _mm_add_epi16(T30, T31);
            sum0 = _mm_add_epi16(sum0, sum1);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[1] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref3 + (i) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref3 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref3 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref3 + (i + 3) * frefstride));

            T20 = _mm_loadl_epi64((__m128i*)(fref3 + (i) * frefstride + 16));
            T21 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 1) * frefstride + 16));
            T14 = _mm_unpacklo_epi64(T20, T21);

            T22 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 2) * frefstride + 16));
            T23 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 3) * frefstride + 16));
            T15 = _mm_unpacklo_epi64(T22, T23);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);
            T30 = _mm_sad_epu8(T04, T14);
            T31 = _mm_sad_epu8(T05, T15);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);
            sum1 = _mm_add_epi16(T30, T31);
            sum0 = _mm_add_epi16(sum0, sum1);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[2] += _mm_cvtsi128_si32(sum0);
        }
    }
}

template<int ly>
void sad_x3_32(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, intptr_t frefstride, int *res)
{
    assert((ly % 4) == 0);

    if (ly == 4)
    {
        __m128i T00, T01, T02, T03, T04, T05, T06, T07;
        __m128i T10, T11, T12, T13, T14, T15, T16, T17;
        __m128i T20, T21, T22, T23, T24, T25, T26, T27;
        __m128i sum0, sum1;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T01 = _mm_load_si128((__m128i*)(fenc + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * FENC_STRIDE));

        T04 = _mm_load_si128((__m128i*)(fenc + 16));
        T05 = _mm_load_si128((__m128i*)(fenc + FENC_STRIDE + 16));
        T06 = _mm_load_si128((__m128i*)(fenc + (2) * FENC_STRIDE + 16));
        T07 = _mm_load_si128((__m128i*)(fenc + (3) * FENC_STRIDE + 16));

        T10 = _mm_loadu_si128((__m128i*)(fref1));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (3) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref1 + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref1 + frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref1 + (2) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref1 + (3) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (3) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref2 + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref2 + frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref2 + (2) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref2 + (3) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (3) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref3 + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref3 + frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref3 + (2) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref3 + (3) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] = _mm_cvtsi128_si32(sum0);
    }
    else if (ly == 8)
    {
        __m128i T00, T01, T02, T03, T04, T05, T06, T07;
        __m128i T10, T11, T12, T13, T14, T15, T16, T17;
        __m128i T20, T21, T22, T23, T24, T25, T26, T27;
        __m128i sum0, sum1;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T01 = _mm_load_si128((__m128i*)(fenc + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * FENC_STRIDE));

        T04 = _mm_load_si128((__m128i*)(fenc + 16));
        T05 = _mm_load_si128((__m128i*)(fenc + FENC_STRIDE + 16));
        T06 = _mm_load_si128((__m128i*)(fenc + (2) * FENC_STRIDE + 16));
        T07 = _mm_load_si128((__m128i*)(fenc + (3) * FENC_STRIDE + 16));

        T10 = _mm_loadu_si128((__m128i*)(fref1));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (3) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref1 + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref1 + frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref1 + (2) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref1 + (3) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (3) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref2 + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref2 + frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref2 + (2) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref2 + (3) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (3) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref3 + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref3 + frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref3 + (2) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref3 + (3) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] = _mm_cvtsi128_si32(sum0);

        T00 = _mm_load_si128((__m128i*)(fenc + (4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + (5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (7) * FENC_STRIDE));

        T04 = _mm_load_si128((__m128i*)(fenc + (4) * FENC_STRIDE + 16));
        T05 = _mm_load_si128((__m128i*)(fenc + (5) * FENC_STRIDE + 16));
        T06 = _mm_load_si128((__m128i*)(fenc + (6) * FENC_STRIDE + 16));
        T07 = _mm_load_si128((__m128i*)(fenc + (7) * FENC_STRIDE + 16));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (7) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref1 + (4) * frefstride + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref1 + (5) * frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref1 + (6) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref1 + (7) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (7) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref2 + (4) * frefstride + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref2 + (5) * frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref2 + (6) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref2 + (7) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (7) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref3 + (4) * frefstride + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref3 + (5) * frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref3 + (6) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref3 + (7) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] += _mm_cvtsi128_si32(sum0);
    }
    else if (ly ==16)
    {
        __m128i T00, T01, T02, T03, T04, T05, T06, T07;
        __m128i T10, T11, T12, T13, T14, T15, T16, T17;
        __m128i T20, T21, T22, T23, T24, T25, T26, T27;
        __m128i sum0, sum1;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T01 = _mm_load_si128((__m128i*)(fenc + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * FENC_STRIDE));

        T04 = _mm_load_si128((__m128i*)(fenc + 16));
        T05 = _mm_load_si128((__m128i*)(fenc + FENC_STRIDE + 16));
        T06 = _mm_load_si128((__m128i*)(fenc + (2) * FENC_STRIDE + 16));
        T07 = _mm_load_si128((__m128i*)(fenc + (3) * FENC_STRIDE + 16));

        T10 = _mm_loadu_si128((__m128i*)(fref1));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (3) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref1 + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref1 + frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref1 + (2) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref1 + (3) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (3) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref2 + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref2 + frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref2 + (2) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref2 + (3) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (3) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref3 + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref3 + frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref3 + (2) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref3 + (3) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] = _mm_cvtsi128_si32(sum0);

        T00 = _mm_load_si128((__m128i*)(fenc + (4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + (5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (7) * FENC_STRIDE));

        T04 = _mm_load_si128((__m128i*)(fenc + (4) * FENC_STRIDE + 16));
        T05 = _mm_load_si128((__m128i*)(fenc + (5) * FENC_STRIDE + 16));
        T06 = _mm_load_si128((__m128i*)(fenc + (6) * FENC_STRIDE + 16));
        T07 = _mm_load_si128((__m128i*)(fenc + (7) * FENC_STRIDE + 16));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (7) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref1 + (4) * frefstride + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref1 + (5) * frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref1 + (6) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref1 + (7) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (7) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref2 + (4) * frefstride + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref2 + (5) * frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref2 + (6) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref2 + (7) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (7) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref3 + (4) * frefstride + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref3 + (5) * frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref3 + (6) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref3 + (7) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] += _mm_cvtsi128_si32(sum0);

        T00 = _mm_load_si128((__m128i*)(fenc + (8) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + (9) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (10) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (11) * FENC_STRIDE));

        T04 = _mm_load_si128((__m128i*)(fenc + (8) * FENC_STRIDE + 16));
        T05 = _mm_load_si128((__m128i*)(fenc + (9) * FENC_STRIDE + 16));
        T06 = _mm_load_si128((__m128i*)(fenc + (10) * FENC_STRIDE + 16));
        T07 = _mm_load_si128((__m128i*)(fenc + (11) * FENC_STRIDE + 16));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (11) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref1 + (8) * frefstride + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref1 + (9) * frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref1 + (10) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref1 + (11) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (11) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref2 + (8) * frefstride + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref2 + (9) * frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref2 + (10) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref2 + (11) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (11) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref3 + (8) * frefstride + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref3 + (9) * frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref3 + (10) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref3 + (11) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] += _mm_cvtsi128_si32(sum0);

        T00 = _mm_load_si128((__m128i*)(fenc + (12) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + (13) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (14) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (15) * FENC_STRIDE));

        T04 = _mm_load_si128((__m128i*)(fenc + (12) * FENC_STRIDE + 16));
        T05 = _mm_load_si128((__m128i*)(fenc + (13) * FENC_STRIDE + 16));
        T06 = _mm_load_si128((__m128i*)(fenc + (14) * FENC_STRIDE + 16));
        T07 = _mm_load_si128((__m128i*)(fenc + (15) * FENC_STRIDE + 16));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (15) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref1 + (12) * frefstride + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref1 + (13) * frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref1 + (14) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref1 + (15) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (15) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref2 + (12) * frefstride + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref2 + (13) * frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref2 + (14) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref2 + (15) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (15) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref3 + (12) * frefstride + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref3 + (13) * frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref3 + (14) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref3 + (15) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] += _mm_cvtsi128_si32(sum0);
    }
    else if ((ly % 8) == 0)
    {
        res[0] = res[1] = res[2] = 0;
        for(int i = 0; i < ly; i += 8)
        {
            __m128i T00, T01, T02, T03, T04, T05, T06, T07;
            __m128i T10, T11, T12, T13, T14, T15, T16, T17;
            __m128i T20, T21, T22, T23, T24, T25, T26, T27;
            __m128i sum0, sum1;

            T00 = _mm_load_si128((__m128i*)(fenc + (i) * FENC_STRIDE));
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 1) * FENC_STRIDE));
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 2) * FENC_STRIDE));
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 3) * FENC_STRIDE));

            T04 = _mm_load_si128((__m128i*)(fenc + (i) * FENC_STRIDE + 16));
            T05 = _mm_load_si128((__m128i*)(fenc + (i + 1) * FENC_STRIDE + 16));
            T06 = _mm_load_si128((__m128i*)(fenc + (i + 2) * FENC_STRIDE + 16));
            T07 = _mm_load_si128((__m128i*)(fenc + (i + 3) * FENC_STRIDE + 16));

            T10 = _mm_loadu_si128((__m128i*)(fref1 + (i) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref1 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref1 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref1 + (i + 3) * frefstride));

            T14 = _mm_loadu_si128((__m128i*)(fref1 + (i) * frefstride + 16));
            T15 = _mm_loadu_si128((__m128i*)(fref1 + (i + 1) * frefstride + 16));
            T16 = _mm_loadu_si128((__m128i*)(fref1 + (i + 2) * frefstride + 16));
            T17 = _mm_loadu_si128((__m128i*)(fref1 + (i + 3) * frefstride + 16));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T24 = _mm_sad_epu8(T04, T14);
            T25 = _mm_sad_epu8(T05, T15);
            T26 = _mm_sad_epu8(T06, T16);
            T27 = _mm_sad_epu8(T07, T17);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);

            T24 = _mm_add_epi16(T24, T25);
            T26 = _mm_add_epi16(T26, T27);
        
            sum0 = _mm_add_epi16(T20, T22);
            sum0 = _mm_add_epi16(sum0, T24);
            sum0 = _mm_add_epi16(sum0, T26);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[0] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref2 + (i) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref2 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref2 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref2 + (i + 3) * frefstride));

            T14 = _mm_loadu_si128((__m128i*)(fref2 + (i) * frefstride + 16));
            T15 = _mm_loadu_si128((__m128i*)(fref2 + (i + 1) * frefstride + 16));
            T16 = _mm_loadu_si128((__m128i*)(fref2 + (i + 2) * frefstride + 16));
            T17 = _mm_loadu_si128((__m128i*)(fref2 + (i + 3) * frefstride + 16));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T24 = _mm_sad_epu8(T04, T14);
            T25 = _mm_sad_epu8(T05, T15);
            T26 = _mm_sad_epu8(T06, T16);
            T27 = _mm_sad_epu8(T07, T17);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);

            T24 = _mm_add_epi16(T24, T25);
            T26 = _mm_add_epi16(T26, T27);
        
            sum0 = _mm_add_epi16(T20, T22);
            sum0 = _mm_add_epi16(sum0, T24);
            sum0 = _mm_add_epi16(sum0, T26);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[1] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref3 + (i) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref3 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref3 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref3 + (i + 3) * frefstride));

            T14 = _mm_loadu_si128((__m128i*)(fref3 + (i) * frefstride + 16));
            T15 = _mm_loadu_si128((__m128i*)(fref3 + (i + 1) * frefstride + 16));
            T16 = _mm_loadu_si128((__m128i*)(fref3 + (i + 2) * frefstride + 16));
            T17 = _mm_loadu_si128((__m128i*)(fref3 + (i + 3) * frefstride + 16));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T24 = _mm_sad_epu8(T04, T14);
            T25 = _mm_sad_epu8(T05, T15);
            T26 = _mm_sad_epu8(T06, T16);
            T27 = _mm_sad_epu8(T07, T17);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);

            T24 = _mm_add_epi16(T24, T25);
            T26 = _mm_add_epi16(T26, T27);
        
            sum0 = _mm_add_epi16(T20, T22);
            sum0 = _mm_add_epi16(sum0, T24);
            sum0 = _mm_add_epi16(sum0, T26);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[2] += _mm_cvtsi128_si32(sum0);

            T00 = _mm_load_si128((__m128i*)(fenc + (i + 4) * FENC_STRIDE));
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 5) * FENC_STRIDE));
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 6) * FENC_STRIDE));
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 7) * FENC_STRIDE));

            T04 = _mm_load_si128((__m128i*)(fenc + (i + 4) * FENC_STRIDE + 16));
            T05 = _mm_load_si128((__m128i*)(fenc + (i + 5) * FENC_STRIDE + 16));
            T06 = _mm_load_si128((__m128i*)(fenc + (i + 6) * FENC_STRIDE + 16));
            T07 = _mm_load_si128((__m128i*)(fenc + (i + 7) * FENC_STRIDE + 16));

            T10 = _mm_loadu_si128((__m128i*)(fref1 + (i + 4) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref1 + (i + 5) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref1 + (i + 6) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref1 + (i + 7) * frefstride));

            T14 = _mm_loadu_si128((__m128i*)(fref1 + (i + 4) * frefstride + 16));
            T15 = _mm_loadu_si128((__m128i*)(fref1 + (i + 5) * frefstride + 16));
            T16 = _mm_loadu_si128((__m128i*)(fref1 + (i + 6) * frefstride + 16));
            T17 = _mm_loadu_si128((__m128i*)(fref1 + (i + 7) * frefstride + 16));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T24 = _mm_sad_epu8(T04, T14);
            T25 = _mm_sad_epu8(T05, T15);
            T26 = _mm_sad_epu8(T06, T16);
            T27 = _mm_sad_epu8(T07, T17);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);

            T24 = _mm_add_epi16(T24, T25);
            T26 = _mm_add_epi16(T26, T27);
        
            sum0 = _mm_add_epi16(T20, T22);
            sum0 = _mm_add_epi16(sum0, T24);
            sum0 = _mm_add_epi16(sum0, T26);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[0] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref2 + (i + 4) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref2 + (i + 5) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref2 + (i + 6) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref2 + (i + 7) * frefstride));

            T14 = _mm_loadu_si128((__m128i*)(fref2 + (i + 4) * frefstride + 16));
            T15 = _mm_loadu_si128((__m128i*)(fref2 + (i + 5) * frefstride + 16));
            T16 = _mm_loadu_si128((__m128i*)(fref2 + (i + 6) * frefstride + 16));
            T17 = _mm_loadu_si128((__m128i*)(fref2 + (i + 7) * frefstride + 16));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T24 = _mm_sad_epu8(T04, T14);
            T25 = _mm_sad_epu8(T05, T15);
            T26 = _mm_sad_epu8(T06, T16);
            T27 = _mm_sad_epu8(T07, T17);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);

            T24 = _mm_add_epi16(T24, T25);
            T26 = _mm_add_epi16(T26, T27);
        
            sum0 = _mm_add_epi16(T20, T22);
            sum0 = _mm_add_epi16(sum0, T24);
            sum0 = _mm_add_epi16(sum0, T26);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[1] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref3 + (i + 4) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref3 + (i + 5) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref3 + (i + 6) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref3 + (i + 7) * frefstride));

            T14 = _mm_loadu_si128((__m128i*)(fref3 + (i + 4) * frefstride + 16));
            T15 = _mm_loadu_si128((__m128i*)(fref3 + (i + 5) * frefstride + 16));
            T16 = _mm_loadu_si128((__m128i*)(fref3 + (i + 6) * frefstride + 16));
            T17 = _mm_loadu_si128((__m128i*)(fref3 + (i + 7) * frefstride + 16));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T24 = _mm_sad_epu8(T04, T14);
            T25 = _mm_sad_epu8(T05, T15);
            T26 = _mm_sad_epu8(T06, T16);
            T27 = _mm_sad_epu8(T07, T17);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);

            T24 = _mm_add_epi16(T24, T25);
            T26 = _mm_add_epi16(T26, T27);
        
            sum0 = _mm_add_epi16(T20, T22);
            sum0 = _mm_add_epi16(sum0, T24);
            sum0 = _mm_add_epi16(sum0, T26);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[2] += _mm_cvtsi128_si32(sum0);
        }
    }
    else
    {
        res[0] = res[1] = res[2] = 0;
        for(int i = 0; i < ly; i += 4)
        {
            __m128i T00, T01, T02, T03, T04, T05, T06, T07;
            __m128i T10, T11, T12, T13, T14, T15, T16, T17;
            __m128i T20, T21, T22, T23, T24, T25, T26, T27;
            __m128i sum0, sum1;

            T00 = _mm_load_si128((__m128i*)(fenc + (i) * FENC_STRIDE));
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 1) * FENC_STRIDE));
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 2) * FENC_STRIDE));
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 3) * FENC_STRIDE));

            T04 = _mm_load_si128((__m128i*)(fenc + (i) * FENC_STRIDE + 16));
            T05 = _mm_load_si128((__m128i*)(fenc + (i + 1) * FENC_STRIDE + 16));
            T06 = _mm_load_si128((__m128i*)(fenc + (i + 2) * FENC_STRIDE + 16));
            T07 = _mm_load_si128((__m128i*)(fenc + (i + 3) * FENC_STRIDE + 16));

            T10 = _mm_loadu_si128((__m128i*)(fref1 + (i) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref1 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref1 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref1 + (i + 3) * frefstride));

            T14 = _mm_loadu_si128((__m128i*)(fref1 + (i) * frefstride + 16));
            T15 = _mm_loadu_si128((__m128i*)(fref1 + (i + 1) * frefstride + 16));
            T16 = _mm_loadu_si128((__m128i*)(fref1 + (i + 2) * frefstride + 16));
            T17 = _mm_loadu_si128((__m128i*)(fref1 + (i + 3) * frefstride + 16));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T24 = _mm_sad_epu8(T04, T14);
            T25 = _mm_sad_epu8(T05, T15);
            T26 = _mm_sad_epu8(T06, T16);
            T27 = _mm_sad_epu8(T07, T17);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);

            T24 = _mm_add_epi16(T24, T25);
            T26 = _mm_add_epi16(T26, T27);
        
            sum0 = _mm_add_epi16(T20, T22);
            sum0 = _mm_add_epi16(sum0, T24);
            sum0 = _mm_add_epi16(sum0, T26);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[0] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref2 + (i) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref2 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref2 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref2 + (i + 3) * frefstride));

            T14 = _mm_loadu_si128((__m128i*)(fref2 + (i) * frefstride + 16));
            T15 = _mm_loadu_si128((__m128i*)(fref2 + (i + 1) * frefstride + 16));
            T16 = _mm_loadu_si128((__m128i*)(fref2 + (i + 2) * frefstride + 16));
            T17 = _mm_loadu_si128((__m128i*)(fref2 + (i + 3) * frefstride + 16));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T24 = _mm_sad_epu8(T04, T14);
            T25 = _mm_sad_epu8(T05, T15);
            T26 = _mm_sad_epu8(T06, T16);
            T27 = _mm_sad_epu8(T07, T17);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);

            T24 = _mm_add_epi16(T24, T25);
            T26 = _mm_add_epi16(T26, T27);
        
            sum0 = _mm_add_epi16(T20, T22);
            sum0 = _mm_add_epi16(sum0, T24);
            sum0 = _mm_add_epi16(sum0, T26);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[1] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref3 + (i) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref3 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref3 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref3 + (i + 3) * frefstride));

            T14 = _mm_loadu_si128((__m128i*)(fref3 + (i) * frefstride + 16));
            T15 = _mm_loadu_si128((__m128i*)(fref3 + (i + 1) * frefstride + 16));
            T16 = _mm_loadu_si128((__m128i*)(fref3 + (i + 2) * frefstride + 16));
            T17 = _mm_loadu_si128((__m128i*)(fref3 + (i + 3) * frefstride + 16));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T24 = _mm_sad_epu8(T04, T14);
            T25 = _mm_sad_epu8(T05, T15);
            T26 = _mm_sad_epu8(T06, T16);
            T27 = _mm_sad_epu8(T07, T17);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);

            T24 = _mm_add_epi16(T24, T25);
            T26 = _mm_add_epi16(T26, T27);
        
            sum0 = _mm_add_epi16(T20, T22);
            sum0 = _mm_add_epi16(sum0, T24);
            sum0 = _mm_add_epi16(sum0, T26);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[2] += _mm_cvtsi128_si32(sum0);
        }
    }
}
#if HAVE_MMX
template<int ly>
void sad_x4_4(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, pixel *fref4, intptr_t frefstride, int *res)
{
    assert((ly % 4) == 0);

    if (ly == 4)
    {
        __m64 sum0, sum1, sum2, sum3;
        __m64 T10, T11, T12, T13;
        __m64 T00, T01, T02, T03;
        __m64 T20, T21;

        T00 = _mm_cvtsi32_si64(*(int*)(fenc));
        T01 = _mm_cvtsi32_si64(*(int*)(fenc + FENC_STRIDE));
        T00 = _mm_unpacklo_pi8(T00, T01);
        T02 = _mm_cvtsi32_si64(*(int*)(fenc + (FENC_STRIDE << 1)));
        T03 = _mm_cvtsi32_si64(*(int*)(fenc + 3 * FENC_STRIDE));
        T02 = _mm_unpacklo_pi8(T02, T03);

        T10 = _mm_cvtsi32_si64(*(int*)(fref1));
        T11 = _mm_cvtsi32_si64(*(int*)(fref1 + frefstride));
        T10 = _mm_unpacklo_pi8(T10, T11);
        T12 = _mm_cvtsi32_si64(*(int*)(fref1 + 2 * frefstride));
        T13 = _mm_cvtsi32_si64(*(int*)(fref1 + 3 * frefstride));
        T12 = _mm_unpacklo_pi8(T12, T13);

        T20 = _mm_sad_pu8(T00, T10);
        T21 = _mm_sad_pu8(T02, T12);

        sum0 = _mm_add_pi16(T20, T21);

        T10 = _mm_cvtsi32_si64(*(int*)(fref2));
        T11 = _mm_cvtsi32_si64(*(int*)(fref2 + frefstride));
        T10 = _mm_unpacklo_pi8(T10, T11);
        T12 = _mm_cvtsi32_si64(*(int*)(fref2 + 2 * frefstride));
        T13 = _mm_cvtsi32_si64(*(int*)(fref2 + 3 * frefstride));
        T12 = _mm_unpacklo_pi8(T12, T13);

        T20 = _mm_sad_pu8(T00, T10);
        T21 = _mm_sad_pu8(T02, T12);

        sum1 = _mm_add_pi16(T20, T21);

        T10 = _mm_cvtsi32_si64(*(int*)(fref3));
        T11 = _mm_cvtsi32_si64(*(int*)(fref3 + frefstride));
        T10 = _mm_unpacklo_pi8(T10, T11);
        T12 = _mm_cvtsi32_si64(*(int*)(fref3 + 2 * frefstride));
        T13 = _mm_cvtsi32_si64(*(int*)(fref3 + 3 * frefstride));
        T12 = _mm_unpacklo_pi8(T12, T13);

        T20 = _mm_sad_pu8(T00, T10);
        T21 = _mm_sad_pu8(T02, T12);

        sum2 = _mm_add_pi16(T20, T21);

        T10 = _mm_cvtsi32_si64(*(int*)(fref4));
        T11 = _mm_cvtsi32_si64(*(int*)(fref4 + frefstride));
        T10 = _mm_unpacklo_pi8(T10, T11);
        T12 = _mm_cvtsi32_si64(*(int*)(fref4 + 2 * frefstride));
        T13 = _mm_cvtsi32_si64(*(int*)(fref4 + 3 * frefstride));
        T12 = _mm_unpacklo_pi8(T12, T13);

        T20 = _mm_sad_pu8(T00, T10);
        T21 = _mm_sad_pu8(T02, T12);

        sum3 = _mm_add_pi16(T20, T21);

        res[0] = _m_to_int(sum0);
        res[1] = _m_to_int(sum1);
        res[2] = _m_to_int(sum2);
        res[3] = _m_to_int(sum3);
    }
    else if (ly == 8)
    {
        __m128i sum0, sum1, sum2, sum3;

        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i R00, R01, R02, R03, R04;
        __m128i T20;

        T00 = _mm_loadl_epi64((__m128i*)(fenc));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + FENC_STRIDE));
        T01 = _mm_unpacklo_epi32(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (3) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi32(T02, T03);
        R00 = _mm_unpacklo_epi64(T01, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + 2 * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + 3 * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R01 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref2));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + 2 * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + 3 * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R02 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref3));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + 2 * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + 3 * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R03 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref4));
        T11 = _mm_loadl_epi64((__m128i*)(fref4 + frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref4 + 2 * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref4 + 3 * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R04 = _mm_unpacklo_epi64(T11, T13);

        T20 = _mm_sad_epu8(R00, R01);
        sum0 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));

        T20 = _mm_sad_epu8(R00, R02);
        sum1 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));

        T20 = _mm_sad_epu8(R00, R03);
        sum2 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));

        T20 = _mm_sad_epu8(R00, R04);
        sum3 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (4) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (5) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi32(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (6) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (7) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi32(T02, T03);
        R00 = _mm_unpacklo_epi64(T01, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + 4 * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + 5 * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + 6 * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + 7 * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R01 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + 4 * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + 5 * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + 6 * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + 7 * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R02 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + 4 * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + 5 * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + 6 * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + 7 * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R03 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref4 + 4 * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref4 + 5 * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref4 + 6 * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref4 + 7 * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R04 = _mm_unpacklo_epi64(T11, T13);

        T20 = _mm_sad_epu8(R00, R01);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum0 = _mm_add_epi32(sum0, T20);

        T20 = _mm_sad_epu8(R00, R02);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum1 = _mm_add_epi32(sum1, T20);

        T20 = _mm_sad_epu8(R00, R03);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum2 = _mm_add_epi32(sum2, T20);

        T20 = _mm_sad_epu8(R00, R04);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum3 = _mm_add_epi32(sum3, T20);

        res[0] = _mm_cvtsi128_si32(sum0);
        res[1] = _mm_cvtsi128_si32(sum1);
        res[2] = _mm_cvtsi128_si32(sum2);
        res[3] = _mm_cvtsi128_si32(sum3);
    }
    else if (ly == 16)
    {
        __m128i sum0, sum1, sum2, sum3;

        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i R00, R01, R02, R03, R04;
        __m128i T20;

        T00 = _mm_loadl_epi64((__m128i*)(fenc));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + FENC_STRIDE));
        T01 = _mm_unpacklo_epi32(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (3) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi32(T02, T03);
        R00 = _mm_unpacklo_epi64(T01, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (3) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R01 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref2));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (3) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R02 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref3));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (3) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R03 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref4));
        T11 = _mm_loadl_epi64((__m128i*)(fref4 + frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref4 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref4 + (3) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R04 = _mm_unpacklo_epi64(T11, T13);

        T20 = _mm_sad_epu8(R00, R01);
        sum0 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));

        T20 = _mm_sad_epu8(R00, R02);
        sum1 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));

        T20 = _mm_sad_epu8(R00, R03);
        sum2 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));

        T20 = _mm_sad_epu8(R00, R04);
        sum3 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (4) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (5) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi32(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (6) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (7) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi32(T02, T03);
        R00 = _mm_unpacklo_epi64(T01, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (5) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (7) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R01 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (5) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (7) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R02 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (5) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (7) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R03 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref4 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref4 + (5) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref4 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref4 + (7) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R04 = _mm_unpacklo_epi64(T11, T13);

        T20 = _mm_sad_epu8(R00, R01);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum0 = _mm_add_epi32(sum0, T20);

        T20 = _mm_sad_epu8(R00, R02);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum1 = _mm_add_epi32(sum1, T20);

        T20 = _mm_sad_epu8(R00, R03);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum2 = _mm_add_epi32(sum2, T20);

        T20 = _mm_sad_epu8(R00, R04);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum3 = _mm_add_epi32(sum3, T20);

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (8) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (9) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi32(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (10) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (11) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi32(T02, T03);
        R00 = _mm_unpacklo_epi64(T01, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (8) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (9) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (10) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (11) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R01 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (8) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (9) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (10) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (11) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R02 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (8) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (9) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (10) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (11) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R03 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref4 + (8) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref4 + (9) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref4 + (10) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref4 + (11) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R04 = _mm_unpacklo_epi64(T11, T13);

        T20 = _mm_sad_epu8(R00, R01);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum0 = _mm_add_epi32(sum0, T20);

        T20 = _mm_sad_epu8(R00, R02);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum1 = _mm_add_epi32(sum1, T20);

        T20 = _mm_sad_epu8(R00, R03);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum2 = _mm_add_epi32(sum2, T20);

        T20 = _mm_sad_epu8(R00, R04);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum3 = _mm_add_epi32(sum3, T20);

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (12) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (13) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi32(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (14) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (15) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi32(T02, T03);
        R00 = _mm_unpacklo_epi64(T01, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (12) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (13) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (14) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (15) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R01 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (12) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (13) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (14) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (15) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R02 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (12) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (13) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (14) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (15) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R03 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref4 + (12) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref4 + (13) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref4 + (14) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref4 + (15) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R04 = _mm_unpacklo_epi64(T11, T13);

        T20 = _mm_sad_epu8(R00, R01);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum0 = _mm_add_epi32(sum0, T20);

        T20 = _mm_sad_epu8(R00, R02);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum1 = _mm_add_epi32(sum1, T20);

        T20 = _mm_sad_epu8(R00, R03);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum2 = _mm_add_epi32(sum2, T20);

        T20 = _mm_sad_epu8(R00, R04);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum3 = _mm_add_epi32(sum3, T20);

        res[0] = _mm_cvtsi128_si32(sum0);
        res[1] = _mm_cvtsi128_si32(sum1);
        res[2] = _mm_cvtsi128_si32(sum2);
        res[3] = _mm_cvtsi128_si32(sum3);
    }
    else if ((ly % 8) == 0)
    {
        __m128i sum0 = _mm_setzero_si128();
        __m128i sum1 = _mm_setzero_si128();
        __m128i sum2 = _mm_setzero_si128();
        __m128i sum3 = _mm_setzero_si128();

        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i R00, R01, R02, R03, R04;
        __m128i T20;

        for (int i = 0; i < ly; i += 8)
        {
            T00 = _mm_loadl_epi64((__m128i*)(fenc + (i + 0) * FENC_STRIDE));
            T01 = _mm_loadl_epi64((__m128i*)(fenc + (i + 1) * FENC_STRIDE));
            T01 = _mm_unpacklo_epi32(T00, T01);
            T02 = _mm_loadl_epi64((__m128i*)(fenc + (i + 2) * FENC_STRIDE));
            T03 = _mm_loadl_epi64((__m128i*)(fenc + (i + 3) * FENC_STRIDE));
            T03 = _mm_unpacklo_epi32(T02, T03);
            R00 = _mm_unpacklo_epi64(T01, T03);

            T10 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            R01 = _mm_unpacklo_epi64(T11, T13);

            T10 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            R02 = _mm_unpacklo_epi64(T11, T13);

            T10 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            R03 = _mm_unpacklo_epi64(T11, T13);

            T10 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            R04 = _mm_unpacklo_epi64(T11, T13);

            T20 = _mm_sad_epu8(R00, R01);
            T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
            sum0 = _mm_add_epi32(sum0, T20);

            T20 = _mm_sad_epu8(R00, R02);
            T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
            sum1 = _mm_add_epi32(sum1, T20);

            T20 = _mm_sad_epu8(R00, R03);
            T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
            sum2 = _mm_add_epi32(sum2, T20);

            T20 = _mm_sad_epu8(R00, R04);
            T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
            sum3 = _mm_add_epi32(sum3, T20);

            T00 = _mm_loadl_epi64((__m128i*)(fenc + (i + 4) * FENC_STRIDE));
            T01 = _mm_loadl_epi64((__m128i*)(fenc + (i + 5) * FENC_STRIDE));
            T01 = _mm_unpacklo_epi32(T00, T01);
            T02 = _mm_loadl_epi64((__m128i*)(fenc + (i + 6) * FENC_STRIDE));
            T03 = _mm_loadl_epi64((__m128i*)(fenc + (i + 7) * FENC_STRIDE));
            T03 = _mm_unpacklo_epi32(T02, T03);
            R00 = _mm_unpacklo_epi64(T01, T03);

            T10 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 4) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 5) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 6) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 7) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            R01 = _mm_unpacklo_epi64(T11, T13);

            T10 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 4) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 5) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 6) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 7) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            R02 = _mm_unpacklo_epi64(T11, T13);

            T10 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 4) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 5) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 6) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 7) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            R03 = _mm_unpacklo_epi64(T11, T13);

            T10 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 4) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 5) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 6) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 7) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            R04 = _mm_unpacklo_epi64(T11, T13);

            T20 = _mm_sad_epu8(R00, R01);
            T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
            sum0 = _mm_add_epi32(sum0, T20);

            T20 = _mm_sad_epu8(R00, R02);
            T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
            sum1 = _mm_add_epi32(sum1, T20);

            T20 = _mm_sad_epu8(R00, R03);
            T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
            sum2 = _mm_add_epi32(sum2, T20);

            T20 = _mm_sad_epu8(R00, R04);
            T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
            sum3 = _mm_add_epi32(sum3, T20);
        }

        res[0] = _mm_cvtsi128_si32(sum0);
        res[1] = _mm_cvtsi128_si32(sum1);
        res[2] = _mm_cvtsi128_si32(sum2);
        res[3] = _mm_cvtsi128_si32(sum3);
    }
    else
    {
        __m64 sum0 = _mm_setzero_si64();
        __m64 sum1 = _mm_setzero_si64();
        __m64 sum2 = _mm_setzero_si64();
        __m64 sum3 = _mm_setzero_si64();

        __m64 T10, T11, T12, T13;
        __m64 T00, T01, T02, T03;
        __m64 T20, T21;

        for (int i = 0; i < ly; i += 4)
        {
            int frefstrideZero = (i + 0) * frefstride;
            int frefstrideOne = (i + 1) * frefstride;
            int frefstrideTwo = (i + 2) * frefstride;
            int frefstrideThree = (i + 3) * frefstride;

            T00 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 0) * FENC_STRIDE));
            T01 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 1) * FENC_STRIDE));
            T00 = _mm_unpacklo_pi8(T00, T01);
            T02 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 2) * FENC_STRIDE));
            T03 = _mm_cvtsi32_si64(*(int*)(fenc + (i + 3) * FENC_STRIDE));
            T02 = _mm_unpacklo_pi8(T02, T03);

            T10 = _mm_cvtsi32_si64(*(int*)(fref1 + frefstrideZero));
            T11 = _mm_cvtsi32_si64(*(int*)(fref1 + frefstrideOne));
            T10 = _mm_unpacklo_pi8(T10, T11);
            T12 = _mm_cvtsi32_si64(*(int*)(fref1 + frefstrideTwo));
            T13 = _mm_cvtsi32_si64(*(int*)(fref1 + frefstrideThree));
            T12 = _mm_unpacklo_pi8(T12, T13);

            T20 = _mm_sad_pu8(T00, T10);
            T21 = _mm_sad_pu8(T02, T12);

            sum0 = _mm_add_pi16(sum0, T20);
            sum0 = _mm_add_pi16(sum0, T21);

            T10 = _mm_cvtsi32_si64(*(int*)(fref2 + frefstrideZero));
            T11 = _mm_cvtsi32_si64(*(int*)(fref2 + frefstrideOne));
            T10 = _mm_unpacklo_pi8(T10, T11);
            T12 = _mm_cvtsi32_si64(*(int*)(fref2 + frefstrideTwo));
            T13 = _mm_cvtsi32_si64(*(int*)(fref2 + frefstrideThree));
            T12 = _mm_unpacklo_pi8(T12, T13);

            T20 = _mm_sad_pu8(T00, T10);
            T21 = _mm_sad_pu8(T02, T12);

            sum1 = _mm_add_pi16(sum1, T20);
            sum1 = _mm_add_pi16(sum1, T21);

            T10 = _mm_cvtsi32_si64(*(int*)(fref3 + frefstrideZero));
            T11 = _mm_cvtsi32_si64(*(int*)(fref3 + frefstrideOne));
            T10 = _mm_unpacklo_pi8(T10, T11);
            T12 = _mm_cvtsi32_si64(*(int*)(fref3 + frefstrideTwo));
            T13 = _mm_cvtsi32_si64(*(int*)(fref3 + frefstrideThree));
            T12 = _mm_unpacklo_pi8(T12, T13);

            T20 = _mm_sad_pu8(T00, T10);
            T21 = _mm_sad_pu8(T02, T12);

            sum2 = _mm_add_pi16(sum2, T20);
            sum2 = _mm_add_pi16(sum2, T21);

            T10 = _mm_cvtsi32_si64(*(int*)(fref4 + frefstrideZero));
            T11 = _mm_cvtsi32_si64(*(int*)(fref4 + frefstrideOne));
            T10 = _mm_unpacklo_pi8(T10, T11);
            T12 = _mm_cvtsi32_si64(*(int*)(fref4 + frefstrideTwo));
            T13 = _mm_cvtsi32_si64(*(int*)(fref4 + frefstrideThree));
            T12 = _mm_unpacklo_pi8(T12, T13);

            T20 = _mm_sad_pu8(T00, T10);
            T21 = _mm_sad_pu8(T02, T12);

            sum3 = _mm_add_pi16(sum3, T20);
            sum3 = _mm_add_pi16(sum3, T21);
        }

        res[0] = _m_to_int(sum0);
        res[1] = _m_to_int(sum1);
        res[2] = _m_to_int(sum2);
        res[3] = _m_to_int(sum3);
    }
}

#else /* if HAVE_MMX */

template<int ly>
void sad_x4_4(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, pixel *fref4, intptr_t frefstride, int *res)
{
    assert((ly % 4) == 0);
    __m128i sum0 = _mm_setzero_si128();
    __m128i sum1 = _mm_setzero_si128();
    __m128i sum2 = _mm_setzero_si128();
    __m128i sum3 = _mm_setzero_si128();

    __m128i T00, T01, T02, T03;
    __m128i T10, T11, T12, T13;
    __m128i R00, R01, R02, R03, R04;
    __m128i T20;

    if (ly == 4)
    {
        T00 = _mm_loadl_epi64((__m128i*)(fenc + (0) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (1) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi32(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (3) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi32(T02, T03);
        R00 = _mm_unpacklo_epi64(T01, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (0) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (1) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (3) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R01 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (0) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (1) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (3) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R02 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (0) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (1) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (3) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R03 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref4 + (0) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref4 + (1) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref4 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref4 + (3) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R04 = _mm_unpacklo_epi64(T11, T13);

        T20 = _mm_sad_epu8(R00, R01);
        sum0 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));

        T20 = _mm_sad_epu8(R00, R02);
        sum1 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));

        T20 = _mm_sad_epu8(R00, R03);
        sum2 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));

        T20 = _mm_sad_epu8(R00, R04);
        sum3 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
    }
    else if (ly == 8)
    {
        T00 = _mm_loadl_epi64((__m128i*)(fenc + (0) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (1) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi32(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (3) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi32(T02, T03);
        R00 = _mm_unpacklo_epi64(T01, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (0) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (1) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (3) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R01 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (0) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (1) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (3) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R02 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (0) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (1) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (3) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R03 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref4 + (0) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref4 + (1) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref4 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref4 + (3) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R04 = _mm_unpacklo_epi64(T11, T13);

        T20 = _mm_sad_epu8(R00, R01);
        sum0 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));

        T20 = _mm_sad_epu8(R00, R02);
        sum1 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));

        T20 = _mm_sad_epu8(R00, R03);
        sum2 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));

        T20 = _mm_sad_epu8(R00, R04);
        sum3 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (4) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (5) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi32(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (6) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (7) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi32(T02, T03);
        R00 = _mm_unpacklo_epi64(T01, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (5) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (7) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R01 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (5) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (7) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R02 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (5) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (7) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R03 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref4 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref4 + (5) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref4 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref4 + (7) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R04 = _mm_unpacklo_epi64(T11, T13);

        T20 = _mm_sad_epu8(R00, R01);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum0 = _mm_add_epi32(sum0, T20);

        T20 = _mm_sad_epu8(R00, R02);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum1 = _mm_add_epi32(sum1, T20);

        T20 = _mm_sad_epu8(R00, R03);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum2 = _mm_add_epi32(sum2, T20);

        T20 = _mm_sad_epu8(R00, R04);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum3 = _mm_add_epi32(sum3, T20);
    }
    else if (ly == 16)
    {
        T00 = _mm_loadl_epi64((__m128i*)(fenc + (0) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (1) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi32(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (3) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi32(T02, T03);
        R00 = _mm_unpacklo_epi64(T01, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (0) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (1) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (3) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R01 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (0) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (1) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (3) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R02 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (0) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (1) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (3) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R03 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref4 + (0) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref4 + (1) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref4 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref4 + (3) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R04 = _mm_unpacklo_epi64(T11, T13);

        T20 = _mm_sad_epu8(R00, R01);
        sum0 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));

        T20 = _mm_sad_epu8(R00, R02);
        sum1 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));

        T20 = _mm_sad_epu8(R00, R03);
        sum2 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));

        T20 = _mm_sad_epu8(R00, R04);
        sum3 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (4) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (5) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi32(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (6) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (7) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi32(T02, T03);
        R00 = _mm_unpacklo_epi64(T01, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (5) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (7) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R01 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (5) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (7) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R02 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (5) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (7) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R03 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref4 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref4 + (5) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref4 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref4 + (7) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R04 = _mm_unpacklo_epi64(T11, T13);

        T20 = _mm_sad_epu8(R00, R01);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum0 = _mm_add_epi32(sum0, T20);

        T20 = _mm_sad_epu8(R00, R02);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum1 = _mm_add_epi32(sum1, T20);

        T20 = _mm_sad_epu8(R00, R03);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum2 = _mm_add_epi32(sum2, T20);

        T20 = _mm_sad_epu8(R00, R04);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum3 = _mm_add_epi32(sum3, T20);

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (8) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (9) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi32(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (10) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (11) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi32(T02, T03);
        R00 = _mm_unpacklo_epi64(T01, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (8) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (9) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (10) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (11) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R01 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (8) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (9) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (10) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (11) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R02 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (8) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (9) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (10) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (11) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R03 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref4 + (8) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref4 + (9) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref4 + (10) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref4 + (11) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R04 = _mm_unpacklo_epi64(T11, T13);

        T20 = _mm_sad_epu8(R00, R01);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum0 = _mm_add_epi32(sum0, T20);

        T20 = _mm_sad_epu8(R00, R02);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum1 = _mm_add_epi32(sum1, T20);

        T20 = _mm_sad_epu8(R00, R03);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum2 = _mm_add_epi32(sum2, T20);

        T20 = _mm_sad_epu8(R00, R04);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum3 = _mm_add_epi32(sum3, T20);

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (12) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (13) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi32(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (14) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (15) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi32(T02, T03);
        R00 = _mm_unpacklo_epi64(T01, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (12) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (13) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (14) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (15) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R01 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (12) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (13) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (14) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (15) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R02 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (12) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (13) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (14) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (15) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R03 = _mm_unpacklo_epi64(T11, T13);

        T10 = _mm_loadl_epi64((__m128i*)(fref4 + (12) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref4 + (13) * frefstride));
        T11 = _mm_unpacklo_epi32(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref4 + (14) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref4 + (15) * frefstride));
        T13 = _mm_unpacklo_epi32(T12, T13);
        R04 = _mm_unpacklo_epi64(T11, T13);

        T20 = _mm_sad_epu8(R00, R01);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum0 = _mm_add_epi32(sum0, T20);

        T20 = _mm_sad_epu8(R00, R02);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum1 = _mm_add_epi32(sum1, T20);

        T20 = _mm_sad_epu8(R00, R03);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum2 = _mm_add_epi32(sum2, T20);

        T20 = _mm_sad_epu8(R00, R04);
        T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
        sum3 = _mm_add_epi32(sum3, T20);
    }
    else if ((ly % 8) == 0)
    {
        for (int i = 0; i < ly; i += 8)
        {
            T00 = _mm_loadl_epi64((__m128i*)(fenc + (i + 0) * FENC_STRIDE));
            T01 = _mm_loadl_epi64((__m128i*)(fenc + (i + 1) * FENC_STRIDE));
            T01 = _mm_unpacklo_epi32(T00, T01);
            T02 = _mm_loadl_epi64((__m128i*)(fenc + (i + 2) * FENC_STRIDE));
            T03 = _mm_loadl_epi64((__m128i*)(fenc + (i + 3) * FENC_STRIDE));
            T03 = _mm_unpacklo_epi32(T02, T03);
            R00 = _mm_unpacklo_epi64(T01, T03);

            T10 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            R01 = _mm_unpacklo_epi64(T11, T13);

            T10 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            R02 = _mm_unpacklo_epi64(T11, T13);

            T10 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            R03 = _mm_unpacklo_epi64(T11, T13);

            T10 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            R04 = _mm_unpacklo_epi64(T11, T13);

            T20 = _mm_sad_epu8(R00, R01);
            T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
            sum0 = _mm_add_epi32(sum0, T20);

            T20 = _mm_sad_epu8(R00, R02);
            T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
            sum1 = _mm_add_epi32(sum1, T20);

            T20 = _mm_sad_epu8(R00, R03);
            T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
            sum2 = _mm_add_epi32(sum2, T20);

            T20 = _mm_sad_epu8(R00, R04);
            T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
            sum3 = _mm_add_epi32(sum3, T20);

            T00 = _mm_loadl_epi64((__m128i*)(fenc + (i + 4) * FENC_STRIDE));
            T01 = _mm_loadl_epi64((__m128i*)(fenc + (i + 5) * FENC_STRIDE));
            T01 = _mm_unpacklo_epi32(T00, T01);
            T02 = _mm_loadl_epi64((__m128i*)(fenc + (i + 6) * FENC_STRIDE));
            T03 = _mm_loadl_epi64((__m128i*)(fenc + (i + 7) * FENC_STRIDE));
            T03 = _mm_unpacklo_epi32(T02, T03);
            R00 = _mm_unpacklo_epi64(T01, T03);

            T10 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 4) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 5) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 6) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 7) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            R01 = _mm_unpacklo_epi64(T11, T13);

            T10 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 4) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 5) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 6) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 7) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            R02 = _mm_unpacklo_epi64(T11, T13);

            T10 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 4) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 5) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 6) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 7) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            R03 = _mm_unpacklo_epi64(T11, T13);

            T10 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 4) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 5) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 6) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 7) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            R04 = _mm_unpacklo_epi64(T11, T13);

            T20 = _mm_sad_epu8(R00, R01);
            T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
            sum0 = _mm_add_epi32(sum0, T20);

            T20 = _mm_sad_epu8(R00, R02);
            T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
            sum1 = _mm_add_epi32(sum1, T20);

            T20 = _mm_sad_epu8(R00, R03);
            T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
            sum2 = _mm_add_epi32(sum2, T20);

            T20 = _mm_sad_epu8(R00, R04);
            T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
            sum3 = _mm_add_epi32(sum3, T20);
        }
    }
    else
    {
        for (int i = 0; i < ly; i += 4)
        {
            T00 = _mm_loadl_epi64((__m128i*)(fenc + (i + 0) * FENC_STRIDE));
            T01 = _mm_loadl_epi64((__m128i*)(fenc + (i + 1) * FENC_STRIDE));
            T01 = _mm_unpacklo_epi32(T00, T01);
            T02 = _mm_loadl_epi64((__m128i*)(fenc + (i + 2) * FENC_STRIDE));
            T03 = _mm_loadl_epi64((__m128i*)(fenc + (i + 3) * FENC_STRIDE));
            T03 = _mm_unpacklo_epi32(T02, T03);
            R00 = _mm_unpacklo_epi64(T01, T03);

            T10 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            R01 = _mm_unpacklo_epi64(T11, T13);

            T10 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            R02 = _mm_unpacklo_epi64(T11, T13);

            T10 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            R03 = _mm_unpacklo_epi64(T11, T13);

            T10 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi32(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi32(T12, T13);
            R04 = _mm_unpacklo_epi64(T11, T13);

            T20 = _mm_sad_epu8(R00, R01);
            T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
            sum0 = _mm_add_epi32(sum0, T20);

            T20 = _mm_sad_epu8(R00, R02);
            T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
            sum1 = _mm_add_epi32(sum1, T20);

            T20 = _mm_sad_epu8(R00, R03);
            T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
            sum2 = _mm_add_epi32(sum2, T20);

            T20 = _mm_sad_epu8(R00, R04);
            T20 = _mm_add_epi32(T20, _mm_shuffle_epi32(T20, 2));
            sum3 = _mm_add_epi32(sum3, T20);
        }
    }
    res[0] = _mm_cvtsi128_si32(sum0);
    res[1] = _mm_cvtsi128_si32(sum1);
    res[2] = _mm_cvtsi128_si32(sum2);
    res[3] = _mm_cvtsi128_si32(sum3);
}
#endif /* if HAVE_MMX */

#if HAVE_MMX
template<int ly>
void sad_x4_8(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, pixel *fref4, intptr_t frefstride, int *res)
{
    assert((ly % 4) == 0);

    if (ly == 4)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21;
        __m128i sum0;

        T00 = _mm_loadl_epi64((__m128i*)(fenc));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + FENC_STRIDE));
        T01 = _mm_unpacklo_epi64(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (3) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[0] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref2));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[1] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref3));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[2] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref4));
        T11 = _mm_loadl_epi64((__m128i*)(fref4 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref4 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref4 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[3] = _mm_cvtsi128_si32(sum0);
    }
    else if (ly == 8)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21;
        __m128i sum0;

        T00 = _mm_loadl_epi64((__m128i*)(fenc));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + FENC_STRIDE));
        T01 = _mm_unpacklo_epi64(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (3) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[0] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref2));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[1] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref3));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[2] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref4));
        T11 = _mm_loadl_epi64((__m128i*)(fref4 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref4 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref4 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[3] = _mm_cvtsi128_si32(sum0);

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (4) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (5) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi64(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (6) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (7) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (5) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (7) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[0] = res[0] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (5) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (7) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[1] = res[1] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (5) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (7) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[2] = res[2] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref4 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref4 + (5) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref4 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref4 + (7) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[3] = res[3] + _mm_cvtsi128_si32(sum0);
    }
    else if (ly == 16)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21;
        __m128i sum0;

        T00 = _mm_loadl_epi64((__m128i*)(fenc));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + FENC_STRIDE));
        T01 = _mm_unpacklo_epi64(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (3) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[0] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref2));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[1] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref3));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[2] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref4));
        T11 = _mm_loadl_epi64((__m128i*)(fref4 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref4 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref4 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[3] = _mm_cvtsi128_si32(sum0);

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (4) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (5) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi64(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (6) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (7) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (5) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (7) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[0] = res[0] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (5) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (7) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[1] = res[1] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (5) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (7) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[2] = res[2] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref4 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref4 + (5) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref4 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref4 + (7) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[3] = res[3] + _mm_cvtsi128_si32(sum0);

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (8) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (9) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi64(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (10) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (11) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (8) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (9) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (10) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (11) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[0] = res[0] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (8) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (9) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (10) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (11) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[1] = res[1] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (8) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (9) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (10) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (11) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[2] = res[2] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref4 + (8) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref4 + (9) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref4 + (10) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref4 + (11) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[3] = res[3] + _mm_cvtsi128_si32(sum0);

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (12) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (13) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi64(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (14) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (15) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (12) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (13) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (14) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (15) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[0] = res[0] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (12) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (13) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (14) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (15) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[1] = res[1] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (12) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (13) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (14) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (15) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[2] = res[2] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadl_epi64((__m128i*)(fref4 + (12) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref4 + (13) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref4 + (14) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref4 + (15) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_shuffle_epi32(T21, 2);
        sum0 = _mm_add_epi32(sum0, T21);
        res[3] = res[3] + _mm_cvtsi128_si32(sum0);
    }
    else if ((ly % 8) == 0)
    {
        __m64 sum0 = _mm_setzero_si64();
        __m64 sum1 = _mm_setzero_si64();
        __m64 sum2 = _mm_setzero_si64();
        __m64 sum3 = _mm_setzero_si64();

        __m64 T00, T01, T02, T03, T04, T05, T06, T07;
        __m64 T10, T11, T12, T13, T14, T15, T16, T17;
        __m64 T20, T21, T22, T23, T24, T25, T26, T27;

        for (int i = 0; i < ly; i += 8)
        {
            T00 = (*(__m64*)(fenc + (i + 0) * FENC_STRIDE));
            T01 = (*(__m64*)(fenc + (i + 1) * FENC_STRIDE));
            T02 = (*(__m64*)(fenc + (i + 2) * FENC_STRIDE));
            T03 = (*(__m64*)(fenc + (i + 3) * FENC_STRIDE));
            T04 = (*(__m64*)(fenc + (i + 4) * FENC_STRIDE));
            T05 = (*(__m64*)(fenc + (i + 5) * FENC_STRIDE));
            T06 = (*(__m64*)(fenc + (i + 6) * FENC_STRIDE));
            T07 = (*(__m64*)(fenc + (i + 7) * FENC_STRIDE));

            T10 = (*(__m64*)(fref1 + (i + 0) * frefstride));
            T11 = (*(__m64*)(fref1 + (i + 1) * frefstride));
            T12 = (*(__m64*)(fref1 + (i + 2) * frefstride));
            T13 = (*(__m64*)(fref1 + (i + 3) * frefstride));
            T14 = (*(__m64*)(fref1 + (i + 4) * frefstride));
            T15 = (*(__m64*)(fref1 + (i + 5) * frefstride));
            T16 = (*(__m64*)(fref1 + (i + 6) * frefstride));
            T17 = (*(__m64*)(fref1 + (i + 7) * frefstride));

            T20 = _mm_sad_pu8(T00, T10);
            T21 = _mm_sad_pu8(T01, T11);
            T22 = _mm_sad_pu8(T02, T12);
            T23 = _mm_sad_pu8(T03, T13);
            T24 = _mm_sad_pu8(T04, T14);
            T25 = _mm_sad_pu8(T05, T15);
            T26 = _mm_sad_pu8(T06, T16);
            T27 = _mm_sad_pu8(T07, T17);

            sum0 = _mm_add_pi16(sum0, T20);
            sum0 = _mm_add_pi16(sum0, T21);
            sum0 = _mm_add_pi16(sum0, T22);
            sum0 = _mm_add_pi16(sum0, T23);
            sum0 = _mm_add_pi16(sum0, T24);
            sum0 = _mm_add_pi16(sum0, T25);
            sum0 = _mm_add_pi16(sum0, T26);
            sum0 = _mm_add_pi16(sum0, T27);

            T10 = (*(__m64*)(fref2 + (i + 0) * frefstride));
            T11 = (*(__m64*)(fref2 + (i + 1) * frefstride));
            T12 = (*(__m64*)(fref2 + (i + 2) * frefstride));
            T13 = (*(__m64*)(fref2 + (i + 3) * frefstride));
            T14 = (*(__m64*)(fref2 + (i + 4) * frefstride));
            T15 = (*(__m64*)(fref2 + (i + 5) * frefstride));
            T16 = (*(__m64*)(fref2 + (i + 6) * frefstride));
            T17 = (*(__m64*)(fref2 + (i + 7) * frefstride));

            T20 = _mm_sad_pu8(T00, T10);
            T21 = _mm_sad_pu8(T01, T11);
            T22 = _mm_sad_pu8(T02, T12);
            T23 = _mm_sad_pu8(T03, T13);
            T24 = _mm_sad_pu8(T04, T14);
            T25 = _mm_sad_pu8(T05, T15);
            T26 = _mm_sad_pu8(T06, T16);
            T27 = _mm_sad_pu8(T07, T17);

            sum1 = _mm_add_pi16(sum1, T20);
            sum1 = _mm_add_pi16(sum1, T21);
            sum1 = _mm_add_pi16(sum1, T22);
            sum1 = _mm_add_pi16(sum1, T23);
            sum1 = _mm_add_pi16(sum1, T24);
            sum1 = _mm_add_pi16(sum1, T25);
            sum1 = _mm_add_pi16(sum1, T26);
            sum1 = _mm_add_pi16(sum1, T27);

            T10 = (*(__m64*)(fref3 + (i + 0) * frefstride));
            T11 = (*(__m64*)(fref3 + (i + 1) * frefstride));
            T12 = (*(__m64*)(fref3 + (i + 2) * frefstride));
            T13 = (*(__m64*)(fref3 + (i + 3) * frefstride));
            T14 = (*(__m64*)(fref3 + (i + 4) * frefstride));
            T15 = (*(__m64*)(fref3 + (i + 5) * frefstride));
            T16 = (*(__m64*)(fref3 + (i + 6) * frefstride));
            T17 = (*(__m64*)(fref3 + (i + 7) * frefstride));

            T20 = _mm_sad_pu8(T00, T10);
            T21 = _mm_sad_pu8(T01, T11);
            T22 = _mm_sad_pu8(T02, T12);
            T23 = _mm_sad_pu8(T03, T13);
            T24 = _mm_sad_pu8(T04, T14);
            T25 = _mm_sad_pu8(T05, T15);
            T26 = _mm_sad_pu8(T06, T16);
            T27 = _mm_sad_pu8(T07, T17);

            sum2 = _mm_add_pi16(sum2, T20);
            sum2 = _mm_add_pi16(sum2, T21);
            sum2 = _mm_add_pi16(sum2, T22);
            sum2 = _mm_add_pi16(sum2, T23);
            sum2 = _mm_add_pi16(sum2, T24);
            sum2 = _mm_add_pi16(sum2, T25);
            sum2 = _mm_add_pi16(sum2, T26);
            sum2 = _mm_add_pi16(sum2, T27);

            T10 = (*(__m64*)(fref4 + (i + 0) * frefstride));
            T11 = (*(__m64*)(fref4 + (i + 1) * frefstride));
            T12 = (*(__m64*)(fref4 + (i + 2) * frefstride));
            T13 = (*(__m64*)(fref4 + (i + 3) * frefstride));
            T14 = (*(__m64*)(fref4 + (i + 4) * frefstride));
            T15 = (*(__m64*)(fref4 + (i + 5) * frefstride));
            T16 = (*(__m64*)(fref4 + (i + 6) * frefstride));
            T17 = (*(__m64*)(fref4 + (i + 7) * frefstride));

            T20 = _mm_sad_pu8(T00, T10);
            T21 = _mm_sad_pu8(T01, T11);
            T22 = _mm_sad_pu8(T02, T12);
            T23 = _mm_sad_pu8(T03, T13);
            T24 = _mm_sad_pu8(T04, T14);
            T25 = _mm_sad_pu8(T05, T15);
            T26 = _mm_sad_pu8(T06, T16);
            T27 = _mm_sad_pu8(T07, T17);

            sum3 = _mm_add_pi16(sum3, T20);
            sum3 = _mm_add_pi16(sum3, T21);
            sum3 = _mm_add_pi16(sum3, T22);
            sum3 = _mm_add_pi16(sum3, T23);
            sum3 = _mm_add_pi16(sum3, T24);
            sum3 = _mm_add_pi16(sum3, T25);
            sum3 = _mm_add_pi16(sum3, T26);
            sum3 = _mm_add_pi16(sum3, T27);
        }

        res[0] = _m_to_int(sum0);
        res[1] = _m_to_int(sum1);
        res[2] = _m_to_int(sum2);
        res[3] = _m_to_int(sum3);
    }
    else
    {
        __m64 sum0 = _mm_setzero_si64();
        __m64 sum1 = _mm_setzero_si64();
        __m64 sum2 = _mm_setzero_si64();
        __m64 sum3 = _mm_setzero_si64();

        __m64 T00, T01, T02, T03;
        __m64 T10, T11, T12, T13;
        __m64 T20, T21, T22, T23;

        for (int i = 0; i < ly; i += 4)
        {
            T00 = (*(__m64*)(fenc + (i + 0) * FENC_STRIDE));
            T01 = (*(__m64*)(fenc + (i + 1) * FENC_STRIDE));
            T02 = (*(__m64*)(fenc + (i + 2) * FENC_STRIDE));
            T03 = (*(__m64*)(fenc + (i + 3) * FENC_STRIDE));

            T10 = (*(__m64*)(fref1 + (i + 0) * frefstride));
            T11 = (*(__m64*)(fref1 + (i + 1) * frefstride));
            T12 = (*(__m64*)(fref1 + (i + 2) * frefstride));
            T13 = (*(__m64*)(fref1 + (i + 3) * frefstride));

            T20 = _mm_sad_pu8(T00, T10);
            T21 = _mm_sad_pu8(T01, T11);
            T22 = _mm_sad_pu8(T02, T12);
            T23 = _mm_sad_pu8(T03, T13);

            sum0 = _mm_add_pi16(sum0, T20);
            sum0 = _mm_add_pi16(sum0, T21);
            sum0 = _mm_add_pi16(sum0, T22);
            sum0 = _mm_add_pi16(sum0, T23);

            T10 = (*(__m64*)(fref2 + (i + 0) * frefstride));
            T11 = (*(__m64*)(fref2 + (i + 1) * frefstride));
            T12 = (*(__m64*)(fref2 + (i + 2) * frefstride));
            T13 = (*(__m64*)(fref2 + (i + 3) * frefstride));

            T20 = _mm_sad_pu8(T00, T10);
            T21 = _mm_sad_pu8(T01, T11);
            T22 = _mm_sad_pu8(T02, T12);
            T23 = _mm_sad_pu8(T03, T13);

            sum1 = _mm_add_pi16(sum1, T20);
            sum1 = _mm_add_pi16(sum1, T21);
            sum1 = _mm_add_pi16(sum1, T22);
            sum1 = _mm_add_pi16(sum1, T23);

            T10 = (*(__m64*)(fref3 + (i + 0) * frefstride));
            T11 = (*(__m64*)(fref3 + (i + 1) * frefstride));
            T12 = (*(__m64*)(fref3 + (i + 2) * frefstride));
            T13 = (*(__m64*)(fref3 + (i + 3) * frefstride));

            T20 = _mm_sad_pu8(T00, T10);
            T21 = _mm_sad_pu8(T01, T11);
            T22 = _mm_sad_pu8(T02, T12);
            T23 = _mm_sad_pu8(T03, T13);

            sum2 = _mm_add_pi16(sum2, T20);
            sum2 = _mm_add_pi16(sum2, T21);
            sum2 = _mm_add_pi16(sum2, T22);
            sum2 = _mm_add_pi16(sum2, T23);

            T10 = (*(__m64*)(fref4 + (i + 0) * frefstride));
            T11 = (*(__m64*)(fref4 + (i + 1) * frefstride));
            T12 = (*(__m64*)(fref4 + (i + 2) * frefstride));
            T13 = (*(__m64*)(fref4 + (i + 3) * frefstride));

            T20 = _mm_sad_pu8(T00, T10);
            T21 = _mm_sad_pu8(T01, T11);
            T22 = _mm_sad_pu8(T02, T12);
            T23 = _mm_sad_pu8(T03, T13);

            sum3 = _mm_add_pi16(sum3, T20);
            sum3 = _mm_add_pi16(sum3, T21);
            sum3 = _mm_add_pi16(sum3, T22);
            sum3 = _mm_add_pi16(sum3, T23);
        }

        res[0] = _m_to_int(sum0);
        res[1] = _m_to_int(sum1);
        res[2] = _m_to_int(sum2);
        res[3] = _m_to_int(sum3);
    }
}

#else /* if HAVE_MMX */

template<int ly>
void sad_x4_8(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, pixel *fref4, intptr_t frefstride, int *res)
{
    assert((ly % 4) == 0);

    if (ly == 4)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21;

        __m128i sum0, sum1, sum2, sum3;

        T00 = _mm_loadl_epi64((__m128i*)(fenc));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + FENC_STRIDE));
        T01 = _mm_unpacklo_epi64(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (3) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));

        T10 = _mm_loadl_epi64((__m128i*)(fref2));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum1 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));

        T10 = _mm_loadl_epi64((__m128i*)(fref3));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum2 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));

        T10 = _mm_loadl_epi64((__m128i*)(fref4));
        T11 = _mm_loadl_epi64((__m128i*)(fref4 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref4 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref4 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum3 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));

        res[0] = _mm_cvtsi128_si32(sum0);
        res[1] = _mm_cvtsi128_si32(sum1);
        res[2] = _mm_cvtsi128_si32(sum2);
        res[3] = _mm_cvtsi128_si32(sum3);
    }
    else if (ly == 8)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21;
        __m128i sum0, sum1, sum2, sum3;

        T00 = _mm_loadl_epi64((__m128i*)(fenc));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + FENC_STRIDE));
        T01 = _mm_unpacklo_epi64(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (3) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));

        T10 = _mm_loadl_epi64((__m128i*)(fref2));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum1 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));

        T10 = _mm_loadl_epi64((__m128i*)(fref3));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum2 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));

        T10 = _mm_loadl_epi64((__m128i*)(fref4));
        T11 = _mm_loadl_epi64((__m128i*)(fref4 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref4 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref4 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum3 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (4) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (5) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi64(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (6) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (7) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (5) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (7) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        T21 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));
        sum0 = _mm_add_epi32(sum0, T21);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (5) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (7) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        T21 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));
        sum1 = _mm_add_epi32(sum1, T21);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (5) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (7) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        T21 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));
        sum2 = _mm_add_epi32(sum2, T21);

        T10 = _mm_loadl_epi64((__m128i*)(fref4 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref4 + (5) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref4 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref4 + (7) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        T21 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));
        sum3 = _mm_add_epi32(sum3, T21);

        res[0] = _mm_cvtsi128_si32(sum0);
        res[1] = _mm_cvtsi128_si32(sum1);
        res[2] = _mm_cvtsi128_si32(sum2);
        res[3] = _mm_cvtsi128_si32(sum3);
    }
    else if (ly == 16)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21;
        __m128i sum0, sum1, sum2, sum3;

        T00 = _mm_loadl_epi64((__m128i*)(fenc));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + FENC_STRIDE));
        T01 = _mm_unpacklo_epi64(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (3) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum0 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));

        T10 = _mm_loadl_epi64((__m128i*)(fref2));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum1 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));

        T10 = _mm_loadl_epi64((__m128i*)(fref3));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum2 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));

        T10 = _mm_loadl_epi64((__m128i*)(fref4));
        T11 = _mm_loadl_epi64((__m128i*)(fref4 + frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref4 + (2) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref4 + (3) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        sum3 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (4) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (5) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi64(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (6) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (7) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (5) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (7) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        T21 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));
        sum0 = _mm_add_epi32(sum0, T21);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (5) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (7) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        T21 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));
        sum1 = _mm_add_epi32(sum1, T21);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (5) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (7) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        T21 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));
        sum2 = _mm_add_epi32(sum2, T21);

        T10 = _mm_loadl_epi64((__m128i*)(fref4 + (4) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref4 + (5) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref4 + (6) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref4 + (7) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        T21 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));
        sum3 = _mm_add_epi32(sum3, T21);

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (8) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (9) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi64(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (10) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (11) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (8) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (9) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (10) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (11) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        T21 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));
        sum0 = _mm_add_epi32(sum0, T21);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (8) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (9) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (10) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (11) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        T21 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));
        sum1 = _mm_add_epi32(sum1, T21);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (8) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (9) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (10) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (11) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        T21 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));
        sum2 = _mm_add_epi32(sum2, T21);

        T10 = _mm_loadl_epi64((__m128i*)(fref4 + (8) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref4 + (9) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref4 + (10) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref4 + (11) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        T21 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));
        sum3 = _mm_add_epi32(sum3, T21);

        T00 = _mm_loadl_epi64((__m128i*)(fenc + (12) * FENC_STRIDE));
        T01 = _mm_loadl_epi64((__m128i*)(fenc + (13) * FENC_STRIDE));
        T01 = _mm_unpacklo_epi64(T00, T01);
        T02 = _mm_loadl_epi64((__m128i*)(fenc + (14) * FENC_STRIDE));
        T03 = _mm_loadl_epi64((__m128i*)(fenc + (15) * FENC_STRIDE));
        T03 = _mm_unpacklo_epi64(T02, T03);

        T10 = _mm_loadl_epi64((__m128i*)(fref1 + (12) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref1 + (13) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref1 + (14) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref1 + (15) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        T21 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));
        sum0 = _mm_add_epi32(sum0, T21);

        T10 = _mm_loadl_epi64((__m128i*)(fref2 + (12) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref2 + (13) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref2 + (14) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref2 + (15) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        T21 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));
        sum1 = _mm_add_epi32(sum1, T21);

        T10 = _mm_loadl_epi64((__m128i*)(fref3 + (12) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref3 + (13) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref3 + (14) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref3 + (15) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        T21 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));
        sum2 = _mm_add_epi32(sum2, T21);

        T10 = _mm_loadl_epi64((__m128i*)(fref4 + (12) * frefstride));
        T11 = _mm_loadl_epi64((__m128i*)(fref4 + (13) * frefstride));
        T11 = _mm_unpacklo_epi64(T10, T11);
        T12 = _mm_loadl_epi64((__m128i*)(fref4 + (14) * frefstride));
        T13 = _mm_loadl_epi64((__m128i*)(fref4 + (15) * frefstride));
        T13 = _mm_unpacklo_epi64(T12, T13);

        T20 = _mm_sad_epu8(T01, T11);
        T21 = _mm_sad_epu8(T03, T13);
        T21 = _mm_add_epi32(T20, T21);
        T21 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));
        sum3 = _mm_add_epi32(sum3, T21);

        res[0] = _mm_cvtsi128_si32(sum0);
        res[1] = _mm_cvtsi128_si32(sum1);
        res[2] = _mm_cvtsi128_si32(sum2);
        res[3] = _mm_cvtsi128_si32(sum3);
    }
    else if ((ly % 8) == 0)
    {
        __m128i sum0 = _mm_setzero_si128();
        __m128i sum1 = _mm_setzero_si128();
        __m128i sum2 = _mm_setzero_si128();
        __m128i sum3 = _mm_setzero_si128();

        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21;

        for (int i = 0; i < ly; i += 8)
        {
            T00 = _mm_loadl_epi64((__m128i*)(fenc + (i + 0) * FENC_STRIDE));
            T01 = _mm_loadl_epi64((__m128i*)(fenc + (i + 1) * FENC_STRIDE));
            T01 = _mm_unpacklo_epi64(T00, T01);
            T02 = _mm_loadl_epi64((__m128i*)(fenc + (i + 2) * FENC_STRIDE));
            T03 = _mm_loadl_epi64((__m128i*)(fenc + (i + 3) * FENC_STRIDE));
            T03 = _mm_unpacklo_epi64(T02, T03);

            T10 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi64(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi64(T12, T13);

            T20 = _mm_sad_epu8(T01, T11);
            T21 = _mm_sad_epu8(T03, T13);
            T21 = _mm_add_epi32(T20, T21);
            T21 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));
            sum0 = _mm_add_epi32(sum0, T21);

            T10 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi64(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi64(T12, T13);

            T20 = _mm_sad_epu8(T01, T11);
            T21 = _mm_sad_epu8(T03, T13);
            T21 = _mm_add_epi32(T20, T21);
            T21 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));
            sum1 = _mm_add_epi32(sum1, T21);

            T10 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi64(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi64(T12, T13);

            T20 = _mm_sad_epu8(T01, T11);
            T21 = _mm_sad_epu8(T03, T13);
            T21 = _mm_add_epi32(T20, T21);
            T21 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));
            sum2 = _mm_add_epi32(sum2, T21);

            T10 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi64(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi64(T12, T13);

            T20 = _mm_sad_epu8(T01, T11);
            T21 = _mm_sad_epu8(T03, T13);
            T21 = _mm_add_epi32(T20, T21);
            T21 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));
            sum3 = _mm_add_epi32(sum3, T21);

            T00 = _mm_loadl_epi64((__m128i*)(fenc + (i + 4) * FENC_STRIDE));
            T01 = _mm_loadl_epi64((__m128i*)(fenc + (i + 5) * FENC_STRIDE));
            T01 = _mm_unpacklo_epi64(T00, T01);
            T02 = _mm_loadl_epi64((__m128i*)(fenc + (i + 6) * FENC_STRIDE));
            T03 = _mm_loadl_epi64((__m128i*)(fenc + (i + 7) * FENC_STRIDE));
            T03 = _mm_unpacklo_epi64(T02, T03);

            T10 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 4) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 5) * frefstride));
            T11 = _mm_unpacklo_epi64(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 6) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 7) * frefstride));
            T13 = _mm_unpacklo_epi64(T12, T13);

            T20 = _mm_sad_epu8(T01, T11);
            T21 = _mm_sad_epu8(T03, T13);
            T21 = _mm_add_epi32(T20, T21);
            T21 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));
            sum0 = _mm_add_epi32(sum0, T21);

            T10 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 4) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 5) * frefstride));
            T11 = _mm_unpacklo_epi64(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 6) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 7) * frefstride));
            T13 = _mm_unpacklo_epi64(T12, T13);

            T20 = _mm_sad_epu8(T01, T11);
            T21 = _mm_sad_epu8(T03, T13);
            T21 = _mm_add_epi32(T20, T21);
            T21 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));
            sum1 = _mm_add_epi32(sum1, T21);

            T10 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 4) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 5) * frefstride));
            T11 = _mm_unpacklo_epi64(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 6) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 7) * frefstride));
            T13 = _mm_unpacklo_epi64(T12, T13);

            T20 = _mm_sad_epu8(T01, T11);
            T21 = _mm_sad_epu8(T03, T13);
            T21 = _mm_add_epi32(T20, T21);
            T21 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));
            sum2 = _mm_add_epi32(sum2, T21);

            T10 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 4) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 5) * frefstride));
            T11 = _mm_unpacklo_epi64(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 6) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 7) * frefstride));
            T13 = _mm_unpacklo_epi64(T12, T13);

            T20 = _mm_sad_epu8(T01, T11);
            T21 = _mm_sad_epu8(T03, T13);
            T21 = _mm_add_epi32(T20, T21);
            T21 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));
            sum3 = _mm_add_epi32(sum3, T21);
        }

        res[0] = _mm_cvtsi128_si32(sum0);
        res[1] = _mm_cvtsi128_si32(sum1);
        res[2] = _mm_cvtsi128_si32(sum2);
        res[3] = _mm_cvtsi128_si32(sum3);
    }
    else
    {
        __m128i sum0 = _mm_setzero_si128();
        __m128i sum1 = _mm_setzero_si128();
        __m128i sum2 = _mm_setzero_si128();
        __m128i sum3 = _mm_setzero_si128();

        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21;

        for (int i = 0; i < ly; i += 4)
        {
            T00 = _mm_loadl_epi64((__m128i*)(fenc + (i + 0) * FENC_STRIDE));
            T01 = _mm_loadl_epi64((__m128i*)(fenc + (i + 1) * FENC_STRIDE));
            T01 = _mm_unpacklo_epi64(T00, T01);
            T02 = _mm_loadl_epi64((__m128i*)(fenc + (i + 2) * FENC_STRIDE));
            T03 = _mm_loadl_epi64((__m128i*)(fenc + (i + 3) * FENC_STRIDE));
            T03 = _mm_unpacklo_epi64(T02, T03);

            T10 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi64(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi64(T12, T13);

            T20 = _mm_sad_epu8(T01, T11);
            T21 = _mm_sad_epu8(T03, T13);
            T21 = _mm_add_epi32(T20, T21);
            T21 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));
            sum0 = _mm_add_epi32(sum0, T21);

            T10 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi64(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi64(T12, T13);

            T20 = _mm_sad_epu8(T01, T11);
            T21 = _mm_sad_epu8(T03, T13);
            T21 = _mm_add_epi32(T20, T21);
            T21 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));
            sum1 = _mm_add_epi32(sum1, T21);

            T10 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi64(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi64(T12, T13);

            T20 = _mm_sad_epu8(T01, T11);
            T21 = _mm_sad_epu8(T03, T13);
            T21 = _mm_add_epi32(T20, T21);
            T21 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));
            sum2 = _mm_add_epi32(sum2, T21);

            T10 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 0) * frefstride));
            T11 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 1) * frefstride));
            T11 = _mm_unpacklo_epi64(T10, T11);
            T12 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 2) * frefstride));
            T13 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 3) * frefstride));
            T13 = _mm_unpacklo_epi64(T12, T13);

            T20 = _mm_sad_epu8(T01, T11);
            T21 = _mm_sad_epu8(T03, T13);
            T21 = _mm_add_epi32(T20, T21);
            T21 = _mm_add_epi32(T21, _mm_shuffle_epi32(T21, 2));
            sum3 = _mm_add_epi32(sum3, T21);
        }

        res[0] = _mm_cvtsi128_si32(sum0);
        res[1] = _mm_cvtsi128_si32(sum1);
        res[2] = _mm_cvtsi128_si32(sum2);
        res[3] = _mm_cvtsi128_si32(sum3);
    }
}

#endif /* if HAVE_MMX */

/* For performance - This function assumes that the *last load* can access 16 elements. */

template<int ly>
void sad_x4_12(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, pixel *fref4, intptr_t frefstride, int *res)
{
    assert((ly % 4) == 0);

    __m128i mask = _mm_set_epi32(0x0, 0xffffffff, 0xffffffff, 0xffffffff);

    if (ly == 4)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;
        __m128i sum0, sum1;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T00 = _mm_and_si128(T00, mask);
        T01 = _mm_load_si128((__m128i*)(fenc + FENC_STRIDE));
        T01 = _mm_and_si128(T01, mask);
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * FENC_STRIDE));
        T02 = _mm_and_si128(T02, mask);
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * FENC_STRIDE));
        T03 = _mm_and_si128(T03, mask);

        T10 = _mm_loadu_si128((__m128i*)(fref1));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref1 + frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (2) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (3) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref2 + frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (2) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (3) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref3 + frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (2) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (3) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref4));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref4 + frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref4 + (2) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref4 + (3) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[3] = _mm_cvtsi128_si32(sum0);
    }
    else if (ly == 8)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;
        __m128i sum0, sum1;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T00 = _mm_and_si128(T00, mask);
        T01 = _mm_load_si128((__m128i*)(fenc + FENC_STRIDE));
        T01 = _mm_and_si128(T01, mask);
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * FENC_STRIDE));
        T02 = _mm_and_si128(T02, mask);
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * FENC_STRIDE));
        T03 = _mm_and_si128(T03, mask);

        T10 = _mm_loadu_si128((__m128i*)(fref1));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref1 + frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (2) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (3) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref2 + frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (2) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (3) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref3 + frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (2) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (3) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref4));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref4 + frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref4 + (2) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref4 + (3) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[3] = _mm_cvtsi128_si32(sum0);

        T00 = _mm_load_si128((__m128i*)(fenc + (4) * FENC_STRIDE));
        T00 = _mm_and_si128(T00, mask);
        T01 = _mm_load_si128((__m128i*)(fenc + (5) * FENC_STRIDE));
        T01 = _mm_and_si128(T01, mask);
        T02 = _mm_load_si128((__m128i*)(fenc + (6) * FENC_STRIDE));
        T02 = _mm_and_si128(T02, mask);
        T03 = _mm_load_si128((__m128i*)(fenc + (7) * FENC_STRIDE));
        T03 = _mm_and_si128(T03, mask);

        T10 = _mm_loadu_si128((__m128i*)(fref1 + (4) * frefstride));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref1 + (5) * frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (6) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (7) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + (4) * frefstride));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref2 + (5) * frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (6) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (7) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + (4) * frefstride));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref3 + (5) * frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (6) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (7) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + (4) * frefstride));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref4 + (5) * frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref4 + (6) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref4 + (7) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[3] += _mm_cvtsi128_si32(sum0);
    }
    else if (ly == 16)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;
        __m128i sum0, sum1;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T00 = _mm_and_si128(T00, mask);
        T01 = _mm_load_si128((__m128i*)(fenc + FENC_STRIDE));
        T01 = _mm_and_si128(T01, mask);
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * FENC_STRIDE));
        T02 = _mm_and_si128(T02, mask);
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * FENC_STRIDE));
        T03 = _mm_and_si128(T03, mask);

        T10 = _mm_loadu_si128((__m128i*)(fref1));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref1 + frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (2) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (3) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref2 + frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (2) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (3) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref3 + frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (2) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (3) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref4));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref4 + frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref4 + (2) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref4 + (3) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[3] = _mm_cvtsi128_si32(sum0);

        T00 = _mm_load_si128((__m128i*)(fenc + (4) * FENC_STRIDE));
        T00 = _mm_and_si128(T00, mask);
        T01 = _mm_load_si128((__m128i*)(fenc + (5) * FENC_STRIDE));
        T01 = _mm_and_si128(T01, mask);
        T02 = _mm_load_si128((__m128i*)(fenc + (6) * FENC_STRIDE));
        T02 = _mm_and_si128(T02, mask);
        T03 = _mm_load_si128((__m128i*)(fenc + (7) * FENC_STRIDE));
        T03 = _mm_and_si128(T03, mask);

        T10 = _mm_loadu_si128((__m128i*)(fref1 + (4) * frefstride));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref1 + (5) * frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (6) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (7) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + (4) * frefstride));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref2 + (5) * frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (6) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (7) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + (4) * frefstride));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref3 + (5) * frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (6) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (7) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + (4) * frefstride));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref4 + (5) * frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref4 + (6) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref4 + (7) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[3] += _mm_cvtsi128_si32(sum0);

        T00 = _mm_load_si128((__m128i*)(fenc + (8) * FENC_STRIDE));
        T00 = _mm_and_si128(T00, mask);
        T01 = _mm_load_si128((__m128i*)(fenc + (9) * FENC_STRIDE));
        T01 = _mm_and_si128(T01, mask);
        T02 = _mm_load_si128((__m128i*)(fenc + (10) * FENC_STRIDE));
        T02 = _mm_and_si128(T02, mask);
        T03 = _mm_load_si128((__m128i*)(fenc + (11) * FENC_STRIDE));
        T03 = _mm_and_si128(T03, mask);

        T10 = _mm_loadu_si128((__m128i*)(fref1 + (8) * frefstride));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref1 + (9) * frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (10) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (11) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + (8) * frefstride));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref2 + (9) * frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (10) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (11) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + (8) * frefstride));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref3 + (9) * frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (10) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (11) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + (8) * frefstride));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref4 + (9) * frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref4 + (10) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref4 + (11) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[3] += _mm_cvtsi128_si32(sum0);

        T00 = _mm_load_si128((__m128i*)(fenc + (12) * FENC_STRIDE));
        T00 = _mm_and_si128(T00, mask);
        T01 = _mm_load_si128((__m128i*)(fenc + (13) * FENC_STRIDE));
        T01 = _mm_and_si128(T01, mask);
        T02 = _mm_load_si128((__m128i*)(fenc + (14) * FENC_STRIDE));
        T02 = _mm_and_si128(T02, mask);
        T03 = _mm_load_si128((__m128i*)(fenc + (15) * FENC_STRIDE));
        T03 = _mm_and_si128(T03, mask);

        T10 = _mm_loadu_si128((__m128i*)(fref1 + (12) * frefstride));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref1 + (13) * frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (14) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (15) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + (12) * frefstride));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref2 + (13) * frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (14) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (15) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + (12) * frefstride));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref3 + (13) * frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (14) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (15) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + (12) * frefstride));
        T10 = _mm_and_si128(T10, mask);
        T11 = _mm_loadu_si128((__m128i*)(fref4 + (13) * frefstride));
        T11 = _mm_and_si128(T11, mask);
        T12 = _mm_loadu_si128((__m128i*)(fref4 + (14) * frefstride));
        T12 = _mm_and_si128(T12, mask);
        T13 = _mm_loadu_si128((__m128i*)(fref4 + (15) * frefstride));
        T13 = _mm_and_si128(T13, mask);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22); 

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[3] += _mm_cvtsi128_si32(sum0);
    }
    else if ((ly % 8) == 0)
    {
        res[0] = res[1] = res[2] = res[3] = 0;
        for(int i = 0; i < ly; i += 8)
        {
            __m128i T00, T01, T02, T03;
            __m128i T10, T11, T12, T13;
            __m128i T20, T21, T22, T23;
            __m128i sum0, sum1;

            T00 = _mm_load_si128((__m128i*)(fenc + (i) * FENC_STRIDE));
            T00 = _mm_and_si128(T00, mask);
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 1) * FENC_STRIDE));
            T01 = _mm_and_si128(T01, mask);
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 2) * FENC_STRIDE));
            T02 = _mm_and_si128(T02, mask);
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 3) * FENC_STRIDE));
            T03 = _mm_and_si128(T03, mask);

            T10 = _mm_loadu_si128((__m128i*)(fref1 + (i) * frefstride));
            T10 = _mm_and_si128(T10, mask);
            T11 = _mm_loadu_si128((__m128i*)(fref1 + (i + 1) * frefstride));
            T11 = _mm_and_si128(T11, mask);
            T12 = _mm_loadu_si128((__m128i*)(fref1 + (i + 2) * frefstride));
            T12 = _mm_and_si128(T12, mask);
            T13 = _mm_loadu_si128((__m128i*)(fref1 + (i + 3) * frefstride));
            T13 = _mm_and_si128(T13, mask);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22); 

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[0] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref2 + (i) * frefstride));
            T10 = _mm_and_si128(T10, mask);
            T11 = _mm_loadu_si128((__m128i*)(fref2 + (i + 1) * frefstride));
            T11 = _mm_and_si128(T11, mask);
            T12 = _mm_loadu_si128((__m128i*)(fref2 + (i + 2) * frefstride));
            T12 = _mm_and_si128(T12, mask);
            T13 = _mm_loadu_si128((__m128i*)(fref2 + (i + 3) * frefstride));
            T13 = _mm_and_si128(T13, mask);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22); 

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[1] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref3 + (i) * frefstride));
            T10 = _mm_and_si128(T10, mask);
            T11 = _mm_loadu_si128((__m128i*)(fref3 + (i + 1) * frefstride));
            T11 = _mm_and_si128(T11, mask);
            T12 = _mm_loadu_si128((__m128i*)(fref3 + (i + 2) * frefstride));
            T12 = _mm_and_si128(T12, mask);
            T13 = _mm_loadu_si128((__m128i*)(fref3 + (i + 3) * frefstride));
            T13 = _mm_and_si128(T13, mask);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22); 

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[2] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref4 + (i) * frefstride));
            T10 = _mm_and_si128(T10, mask);
            T11 = _mm_loadu_si128((__m128i*)(fref4 + (i + 1) * frefstride));
            T11 = _mm_and_si128(T11, mask);
            T12 = _mm_loadu_si128((__m128i*)(fref4 + (i + 2) * frefstride));
            T12 = _mm_and_si128(T12, mask);
            T13 = _mm_loadu_si128((__m128i*)(fref4 + (i + 3) * frefstride));
            T13 = _mm_and_si128(T13, mask);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22); 

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[3] += _mm_cvtsi128_si32(sum0);

            T00 = _mm_load_si128((__m128i*)(fenc + (i + 4) * FENC_STRIDE));
            T00 = _mm_and_si128(T00, mask);
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 5) * FENC_STRIDE));
            T01 = _mm_and_si128(T01, mask);
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 6) * FENC_STRIDE));
            T02 = _mm_and_si128(T02, mask);
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 7) * FENC_STRIDE));
            T03 = _mm_and_si128(T03, mask);

            T10 = _mm_loadu_si128((__m128i*)(fref1 + (i + 4) * frefstride));
            T10 = _mm_and_si128(T10, mask);
            T11 = _mm_loadu_si128((__m128i*)(fref1 + (i + 5) * frefstride));
            T11 = _mm_and_si128(T11, mask);
            T12 = _mm_loadu_si128((__m128i*)(fref1 + (i + 6) * frefstride));
            T12 = _mm_and_si128(T12, mask);
            T13 = _mm_loadu_si128((__m128i*)(fref1 + (i + 7) * frefstride));
            T13 = _mm_and_si128(T13, mask);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22); 

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[0] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref2 + (i + 4) * frefstride));
            T10 = _mm_and_si128(T10, mask);
            T11 = _mm_loadu_si128((__m128i*)(fref2 + (i + 5) * frefstride));
            T11 = _mm_and_si128(T11, mask);
            T12 = _mm_loadu_si128((__m128i*)(fref2 + (i + 6) * frefstride));
            T12 = _mm_and_si128(T12, mask);
            T13 = _mm_loadu_si128((__m128i*)(fref2 + (i + 7) * frefstride));
            T13 = _mm_and_si128(T13, mask);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22); 

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[1] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref3 + (i + 4) * frefstride));
            T10 = _mm_and_si128(T10, mask);
            T11 = _mm_loadu_si128((__m128i*)(fref3 + (i + 5) * frefstride));
            T11 = _mm_and_si128(T11, mask);
            T12 = _mm_loadu_si128((__m128i*)(fref3 + (i + 6) * frefstride));
            T12 = _mm_and_si128(T12, mask);
            T13 = _mm_loadu_si128((__m128i*)(fref3 + (i + 7) * frefstride));
            T13 = _mm_and_si128(T13, mask);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22); 

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[2] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref4 + (i + 4) * frefstride));
            T10 = _mm_and_si128(T10, mask);
            T11 = _mm_loadu_si128((__m128i*)(fref4 + (i + 5) * frefstride));
            T11 = _mm_and_si128(T11, mask);
            T12 = _mm_loadu_si128((__m128i*)(fref4 + (i + 6) * frefstride));
            T12 = _mm_and_si128(T12, mask);
            T13 = _mm_loadu_si128((__m128i*)(fref4 + (i + 7) * frefstride));
            T13 = _mm_and_si128(T13, mask);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22); 

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[3] += _mm_cvtsi128_si32(sum0);
        }
    }
    else
    {
        res[0] = res[1] = res[2] = res[3] = 0;
        for(int i = 0; i < ly; i += 4)
        {
            __m128i T00, T01, T02, T03;
            __m128i T10, T11, T12, T13;
            __m128i T20, T21, T22, T23;
            __m128i sum0, sum1;

            T00 = _mm_load_si128((__m128i*)(fenc + (i) * FENC_STRIDE));
            T00 = _mm_and_si128(T00, mask);
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 1) * FENC_STRIDE));
            T01 = _mm_and_si128(T01, mask);
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 2) * FENC_STRIDE));
            T02 = _mm_and_si128(T02, mask);
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 3) * FENC_STRIDE));
            T03 = _mm_and_si128(T03, mask);

            T10 = _mm_loadu_si128((__m128i*)(fref1 + (i) * frefstride));
            T10 = _mm_and_si128(T10, mask);
            T11 = _mm_loadu_si128((__m128i*)(fref1 + (i + 1) * frefstride));
            T11 = _mm_and_si128(T11, mask);
            T12 = _mm_loadu_si128((__m128i*)(fref1 + (i + 2) * frefstride));
            T12 = _mm_and_si128(T12, mask);
            T13 = _mm_loadu_si128((__m128i*)(fref1 + (i + 3) * frefstride));
            T13 = _mm_and_si128(T13, mask);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22); 

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[0] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref2 + (i) * frefstride));
            T10 = _mm_and_si128(T10, mask);
            T11 = _mm_loadu_si128((__m128i*)(fref2 + (i + 1) * frefstride));
            T11 = _mm_and_si128(T11, mask);
            T12 = _mm_loadu_si128((__m128i*)(fref2 + (i + 2) * frefstride));
            T12 = _mm_and_si128(T12, mask);
            T13 = _mm_loadu_si128((__m128i*)(fref2 + (i + 3) * frefstride));
            T13 = _mm_and_si128(T13, mask);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22); 

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[1] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref3 + (i) * frefstride));
            T10 = _mm_and_si128(T10, mask);
            T11 = _mm_loadu_si128((__m128i*)(fref3 + (i + 1) * frefstride));
            T11 = _mm_and_si128(T11, mask);
            T12 = _mm_loadu_si128((__m128i*)(fref3 + (i + 2) * frefstride));
            T12 = _mm_and_si128(T12, mask);
            T13 = _mm_loadu_si128((__m128i*)(fref3 + (i + 3) * frefstride));
            T13 = _mm_and_si128(T13, mask);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22); 

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[2] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref4 + (i) * frefstride));
            T10 = _mm_and_si128(T10, mask);
            T11 = _mm_loadu_si128((__m128i*)(fref4 + (i + 1) * frefstride));
            T11 = _mm_and_si128(T11, mask);
            T12 = _mm_loadu_si128((__m128i*)(fref4 + (i + 2) * frefstride));
            T12 = _mm_and_si128(T12, mask);
            T13 = _mm_loadu_si128((__m128i*)(fref4 + (i + 3) * frefstride));
            T13 = _mm_and_si128(T13, mask);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22); 

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[3] += _mm_cvtsi128_si32(sum0);
        }
    }
}

template<int ly>
void sad_x4_16(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, pixel *fref4, intptr_t frefstride, int *res)
{
    assert((ly % 4) == 0);

    if (ly == 4)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;
        __m128i sum0, sum1;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T01 = _mm_load_si128((__m128i*)(fenc + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref4));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[3] = _mm_cvtsi128_si32(sum0);
    }
    else if (ly == 8)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;
        __m128i sum0, sum1;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T01 = _mm_load_si128((__m128i*)(fenc + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref4));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[3] = _mm_cvtsi128_si32(sum0);

        T00 = _mm_load_si128((__m128i*)(fenc + (4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + (5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (7) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] = res[0] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] = res[1] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] = res[2] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[3] = res[3] + _mm_cvtsi128_si32(sum0);
    }
    else if (ly == 16)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;
        __m128i sum0, sum1;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T01 = _mm_load_si128((__m128i*)(fenc + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref4));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[3] = _mm_cvtsi128_si32(sum0);

        T00 = _mm_load_si128((__m128i*)(fenc + (4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + (5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (7) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] = res[0] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] = res[1] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] = res[2] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[3] = res[3] + _mm_cvtsi128_si32(sum0);

        T00 = _mm_load_si128((__m128i*)(fenc + (8) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + (9) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (10) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (11) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] = res[0] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] = res[1] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] = res[2] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[3] = res[3] + _mm_cvtsi128_si32(sum0);

        T00 = _mm_load_si128((__m128i*)(fenc + (12) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + (13) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (14) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (15) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] = res[0] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] = res[1] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] = res[2] + _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[3] = res[3] + _mm_cvtsi128_si32(sum0);
    }
    else if ((ly % 8) == 0)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;
        __m128i sum0, sum1;

        res[0] = res[1] = res[2] = res[3] = 0;
        for (int i = 0; i < ly; i += 8)
        {
            T00 = _mm_load_si128((__m128i*)(fenc + (i + 0) * FENC_STRIDE));
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 1) * FENC_STRIDE));
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 2) * FENC_STRIDE));
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 3) * FENC_STRIDE));

            T10 = _mm_loadu_si128((__m128i*)(fref1 + (i + 0) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref1 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref1 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref1 + (i + 3) * frefstride));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[0] = res[0] + _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref2 + (i + 0) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref2 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref2 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref2 + (i + 3) * frefstride));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[1] = res[1] + _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref3 + (i + 0) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref3 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref3 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref3 + (i + 3) * frefstride));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[2] = res[2] + _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref4 + (i + 0) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref4 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref4 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref4 + (i + 3) * frefstride));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[3] = res[3] + _mm_cvtsi128_si32(sum0);

            T00 = _mm_load_si128((__m128i*)(fenc + (i + 4) * FENC_STRIDE));
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 5) * FENC_STRIDE));
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 6) * FENC_STRIDE));
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 7) * FENC_STRIDE));

            T10 = _mm_loadu_si128((__m128i*)(fref1 + (i + 4) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref1 + (i + 5) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref1 + (i + 6) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref1 + (i + 7) * frefstride));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[0] = res[0] + _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref2 + (i + 4) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref2 + (i + 5) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref2 + (i + 6) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref2 + (i + 7) * frefstride));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[1] = res[1] + _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref3 + (i + 4) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref3 + (i + 5) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref3 + (i + 6) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref3 + (i + 7) * frefstride));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[2] = res[2] + _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref4 + (i + 4) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref4 + (i + 5) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref4 + (i + 6) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref4 + (i + 7) * frefstride));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[3] = res[3] + _mm_cvtsi128_si32(sum0);
        }
    }
    else
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;
        __m128i sum0, sum1;

        res[0] = res[1] = res[2] = res[3] = 0;
        for (int i = 0; i < ly; i += 4)
        {
            T00 = _mm_load_si128((__m128i*)(fenc + (i + 0) * FENC_STRIDE));
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 1) * FENC_STRIDE));
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 2) * FENC_STRIDE));
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 3) * FENC_STRIDE));

            T10 = _mm_loadu_si128((__m128i*)(fref1 + (i + 0) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref1 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref1 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref1 + (i + 3) * frefstride));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[0] = res[0] + _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref2 + (i + 0) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref2 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref2 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref2 + (i + 3) * frefstride));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[1] = res[1] + _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref3 + (i + 0) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref3 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref3 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref3 + (i + 3) * frefstride));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[2] = res[2] + _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref4 + (i + 0) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref4 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref4 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref4 + (i + 3) * frefstride));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[3] = res[3] + _mm_cvtsi128_si32(sum0);
        }
    }
}

template<int ly>
void sad_x4_24(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, pixel *fref4, intptr_t frefstride, int *res)
{
    assert((ly % 4) == 0);

    if (ly == 4)
    {
        __m128i T00, T01, T02, T03, T04, T05;
        __m128i T10, T11, T12, T13, T14, T15;
        __m128i T20, T21, T22, T23;
        __m128i T30, T31;

        __m128i sum0, sum1;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T01 = _mm_load_si128((__m128i*)(fenc + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * FENC_STRIDE));

        T10 = _mm_loadl_epi64((__m128i*)(fenc + 16));
        T11 = _mm_loadl_epi64((__m128i*)(fenc + (FENC_STRIDE + 16)));
        T04 = _mm_unpacklo_epi64(T10, T11);

        T12 = _mm_loadl_epi64((__m128i*)(fenc + ((2) * FENC_STRIDE) + 16));
        T13 = _mm_loadl_epi64((__m128i*)(fenc + ((3) * FENC_STRIDE) + 16));
        T05 = _mm_unpacklo_epi64(T12, T13);

        T10 = _mm_loadu_si128((__m128i*)(fref1));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (3) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref1 + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref1 + (frefstride + 16)));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref1 + ((2) * frefstride) + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref1 + ((3) * frefstride) + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (3) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref2 + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref2 + (frefstride + 16)));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref2 + ((2) * frefstride) + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref2 + ((3) * frefstride) + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (3) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref3 + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref3 + (frefstride + 16)));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref3 + ((2) * frefstride) + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref3 + ((3) * frefstride) + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref4));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + (3) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref4 + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref4 + (frefstride + 16)));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref4 + ((2) * frefstride) + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref4 + ((3) * frefstride) + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[3] = _mm_cvtsi128_si32(sum0);
    }
    else if (ly == 8)
    {
        __m128i T00, T01, T02, T03, T04, T05;
        __m128i T10, T11, T12, T13, T14, T15;
        __m128i T20, T21, T22, T23;
        __m128i T30, T31;

        __m128i sum0, sum1;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T01 = _mm_load_si128((__m128i*)(fenc + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * FENC_STRIDE));

        T10 = _mm_loadl_epi64((__m128i*)(fenc + 16));
        T11 = _mm_loadl_epi64((__m128i*)(fenc + (FENC_STRIDE + 16)));
        T04 = _mm_unpacklo_epi64(T10, T11);

        T12 = _mm_loadl_epi64((__m128i*)(fenc + ((2) * FENC_STRIDE) + 16));
        T13 = _mm_loadl_epi64((__m128i*)(fenc + ((3) * FENC_STRIDE) + 16));
        T05 = _mm_unpacklo_epi64(T12, T13);

        T10 = _mm_loadu_si128((__m128i*)(fref1));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (3) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref1 + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref1 + (frefstride + 16)));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref1 + ((2) * frefstride) + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref1 + ((3) * frefstride) + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (3) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref2 + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref2 + (frefstride + 16)));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref2 + ((2) * frefstride) + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref2 + ((3) * frefstride) + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (3) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref3 + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref3 + (frefstride + 16)));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref3 + ((2) * frefstride) + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref3 + ((3) * frefstride) + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref4));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + (3) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref4 + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref4 + (frefstride + 16)));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref4 + ((2) * frefstride) + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref4 + ((3) * frefstride) + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[3] = _mm_cvtsi128_si32(sum0);

        T00 = _mm_load_si128((__m128i*)(fenc + (4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + (5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (7) * FENC_STRIDE));

        T10 = _mm_loadl_epi64((__m128i*)(fenc + (4) * FENC_STRIDE + 16));
        T11 = _mm_loadl_epi64((__m128i*)(fenc + (5) * FENC_STRIDE + 16));
        T04 = _mm_unpacklo_epi64(T10, T11);

        T12 = _mm_loadl_epi64((__m128i*)(fenc + (6) * FENC_STRIDE + 16));
        T13 = _mm_loadl_epi64((__m128i*)(fenc + (7) * FENC_STRIDE + 16));
        T05 = _mm_unpacklo_epi64(T12, T13);

        T10 = _mm_loadu_si128((__m128i*)(fref1 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (7) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref1 + (4) * frefstride + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref1 + (5) * frefstride + 16));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref1 + (6) * frefstride + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref1 + (7) * frefstride + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (7) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref2 + (4) * frefstride + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref2 + (5) * frefstride + 16));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref2 + (6) * frefstride + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref2 + (7) * frefstride + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (7) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref3 + (4) * frefstride + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref3 + (5) * frefstride + 16));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref3 + (6) * frefstride + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref3 + (7) * frefstride + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + (7) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref4 + (4) * frefstride + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref4 + (5) * frefstride + 16));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref4 + (6) * frefstride + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref4 + (7) * frefstride + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[3] += _mm_cvtsi128_si32(sum0);
    }
    else if (ly == 16)
    {
        __m128i T00, T01, T02, T03, T04, T05;
        __m128i T10, T11, T12, T13, T14, T15;
        __m128i T20, T21, T22, T23;
        __m128i T30, T31;

        __m128i sum0, sum1;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T01 = _mm_load_si128((__m128i*)(fenc + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * FENC_STRIDE));

        T10 = _mm_loadl_epi64((__m128i*)(fenc + 16));
        T11 = _mm_loadl_epi64((__m128i*)(fenc + (FENC_STRIDE + 16)));
        T04 = _mm_unpacklo_epi64(T10, T11);

        T12 = _mm_loadl_epi64((__m128i*)(fenc + ((2) * FENC_STRIDE) + 16));
        T13 = _mm_loadl_epi64((__m128i*)(fenc + ((3) * FENC_STRIDE) + 16));
        T05 = _mm_unpacklo_epi64(T12, T13);

        T10 = _mm_loadu_si128((__m128i*)(fref1));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (3) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref1 + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref1 + (frefstride + 16)));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref1 + ((2) * frefstride) + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref1 + ((3) * frefstride) + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (3) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref2 + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref2 + (frefstride + 16)));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref2 + ((2) * frefstride) + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref2 + ((3) * frefstride) + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (3) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref3 + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref3 + (frefstride + 16)));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref3 + ((2) * frefstride) + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref3 + ((3) * frefstride) + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref4));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + (3) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref4 + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref4 + (frefstride + 16)));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref4 + ((2) * frefstride) + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref4 + ((3) * frefstride) + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[3] = _mm_cvtsi128_si32(sum0);

        T00 = _mm_load_si128((__m128i*)(fenc + (4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + (5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (7) * FENC_STRIDE));

        T10 = _mm_loadl_epi64((__m128i*)(fenc + (4) * FENC_STRIDE + 16));
        T11 = _mm_loadl_epi64((__m128i*)(fenc + (5) * FENC_STRIDE + 16));
        T04 = _mm_unpacklo_epi64(T10, T11);

        T12 = _mm_loadl_epi64((__m128i*)(fenc + (6) * FENC_STRIDE + 16));
        T13 = _mm_loadl_epi64((__m128i*)(fenc + (7) * FENC_STRIDE + 16));
        T05 = _mm_unpacklo_epi64(T12, T13);

        T10 = _mm_loadu_si128((__m128i*)(fref1 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (7) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref1 + (4) * frefstride + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref1 + (5) * frefstride + 16));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref1 + (6) * frefstride + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref1 + (7) * frefstride + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (7) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref2 + (4) * frefstride + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref2 + (5) * frefstride + 16));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref2 + (6) * frefstride + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref2 + (7) * frefstride + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (7) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref3 + (4) * frefstride + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref3 + (5) * frefstride + 16));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref3 + (6) * frefstride + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref3 + (7) * frefstride + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + (7) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref4 + (4) * frefstride + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref4 + (5) * frefstride + 16));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref4 + (6) * frefstride + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref4 + (7) * frefstride + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[3] += _mm_cvtsi128_si32(sum0);

        T00 = _mm_load_si128((__m128i*)(fenc + (8) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + (9) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (10) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (11) * FENC_STRIDE));

        T10 = _mm_loadl_epi64((__m128i*)(fenc + (8) * FENC_STRIDE + 16));
        T11 = _mm_loadl_epi64((__m128i*)(fenc + (9) * FENC_STRIDE + 16));
        T04 = _mm_unpacklo_epi64(T10, T11);

        T12 = _mm_loadl_epi64((__m128i*)(fenc + (10) * FENC_STRIDE + 16));
        T13 = _mm_loadl_epi64((__m128i*)(fenc + (11) * FENC_STRIDE + 16));
        T05 = _mm_unpacklo_epi64(T12, T13);

        T10 = _mm_loadu_si128((__m128i*)(fref1 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (11) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref1 + (8) * frefstride + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref1 + (9) * frefstride + 16));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref1 + (10) * frefstride + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref1 + (11) * frefstride + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (11) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref2 + (8) * frefstride + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref2 + (9) * frefstride + 16));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref2 + (10) * frefstride + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref2 + (11) * frefstride + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (11) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref3 + (8) * frefstride + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref3 + (9) * frefstride + 16));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref3 + (10) * frefstride + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref3 + (11) * frefstride + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + (11) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref4 + (8) * frefstride + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref4 + (9) * frefstride + 16));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref4 + (10) * frefstride + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref4 + (11) * frefstride + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[3] += _mm_cvtsi128_si32(sum0);

        T00 = _mm_load_si128((__m128i*)(fenc + (12) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + (13) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (14) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (15) * FENC_STRIDE));

        T10 = _mm_loadl_epi64((__m128i*)(fenc + (12) * FENC_STRIDE + 16));
        T11 = _mm_loadl_epi64((__m128i*)(fenc + (13) * FENC_STRIDE + 16));
        T04 = _mm_unpacklo_epi64(T10, T11);

        T12 = _mm_loadl_epi64((__m128i*)(fenc + (14) * FENC_STRIDE + 16));
        T13 = _mm_loadl_epi64((__m128i*)(fenc + (15) * FENC_STRIDE + 16));
        T05 = _mm_unpacklo_epi64(T12, T13);

        T10 = _mm_loadu_si128((__m128i*)(fref1 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (15) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref1 + (12) * frefstride + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref1 + (13) * frefstride + 16));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref1 + (14) * frefstride + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref1 + (15) * frefstride + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (15) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref2 + (12) * frefstride + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref2 + (13) * frefstride + 16));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref2 + (14) * frefstride + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref2 + (15) * frefstride + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (15) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref3 + (12) * frefstride + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref3 + (13) * frefstride + 16));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref3 + (14) * frefstride + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref3 + (15) * frefstride + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + (15) * frefstride));

        T20 = _mm_loadl_epi64((__m128i*)(fref4 + (12) * frefstride + 16));
        T21 = _mm_loadl_epi64((__m128i*)(fref4 + (13) * frefstride + 16));
        T14 = _mm_unpacklo_epi64(T20, T21);

        T22 = _mm_loadl_epi64((__m128i*)(fref4 + (14) * frefstride + 16));
        T23 = _mm_loadl_epi64((__m128i*)(fref4 + (15) * frefstride + 16));
        T15 = _mm_unpacklo_epi64(T22, T23);

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);
        T30 = _mm_sad_epu8(T04, T14);
        T31 = _mm_sad_epu8(T05, T15);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum0 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi16(T30, T31);
        sum0 = _mm_add_epi16(sum0, sum1);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[3] += _mm_cvtsi128_si32(sum0);
    }
    else if ((ly % 8) == 0)
    {
        res[0] = res[1] = res[2] = res[3] = 0;
        for(int i = 0; i < ly; i += 8)
        {
            __m128i T00, T01, T02, T03, T04, T05;
            __m128i T10, T11, T12, T13, T14, T15;
            __m128i T20, T21, T22, T23;
            __m128i T30, T31;

            __m128i sum0, sum1;

            T00 = _mm_load_si128((__m128i*)(fenc + (i) * FENC_STRIDE));
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 1) * FENC_STRIDE));
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 2) * FENC_STRIDE));
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 3) * FENC_STRIDE));

            T10 = _mm_loadl_epi64((__m128i*)(fenc + (i) * FENC_STRIDE + 16));
            T11 = _mm_loadl_epi64((__m128i*)(fenc + (i + 1) * FENC_STRIDE + 16));
            T04 = _mm_unpacklo_epi64(T10, T11);

            T12 = _mm_loadl_epi64((__m128i*)(fenc + (i + 2) * FENC_STRIDE + 16));
            T13 = _mm_loadl_epi64((__m128i*)(fenc + (i + 3) * FENC_STRIDE + 16));
            T05 = _mm_unpacklo_epi64(T12, T13);

            T10 = _mm_loadu_si128((__m128i*)(fref1 + (i) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref1 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref1 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref1 + (i + 3) * frefstride));

            T20 = _mm_loadl_epi64((__m128i*)(fref1 + (i) * frefstride + 16));
            T21 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 1) * frefstride + 16));
            T14 = _mm_unpacklo_epi64(T20, T21);

            T22 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 2) * frefstride + 16));
            T23 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 3) * frefstride + 16));
            T15 = _mm_unpacklo_epi64(T22, T23);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);
            T30 = _mm_sad_epu8(T04, T14);
            T31 = _mm_sad_epu8(T05, T15);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);
            sum1 = _mm_add_epi16(T30, T31);
            sum0 = _mm_add_epi16(sum0, sum1);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[0] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref2 + (i) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref2 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref2 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref2 + (i + 3) * frefstride));

            T20 = _mm_loadl_epi64((__m128i*)(fref2 + (i) * frefstride + 16));
            T21 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 1) * frefstride + 16));
            T14 = _mm_unpacklo_epi64(T20, T21);

            T22 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 2) * frefstride + 16));
            T23 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 3) * frefstride + 16));
            T15 = _mm_unpacklo_epi64(T22, T23);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);
            T30 = _mm_sad_epu8(T04, T14);
            T31 = _mm_sad_epu8(T05, T15);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);
            sum1 = _mm_add_epi16(T30, T31);
            sum0 = _mm_add_epi16(sum0, sum1);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[1] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref3 + (i) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref3 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref3 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref3 + (i + 3) * frefstride));

            T20 = _mm_loadl_epi64((__m128i*)(fref3 + (i) * frefstride + 16));
            T21 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 1) * frefstride + 16));
            T14 = _mm_unpacklo_epi64(T20, T21);

            T22 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 2) * frefstride + 16));
            T23 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 3) * frefstride + 16));
            T15 = _mm_unpacklo_epi64(T22, T23);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);
            T30 = _mm_sad_epu8(T04, T14);
            T31 = _mm_sad_epu8(T05, T15);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);
            sum1 = _mm_add_epi16(T30, T31);
            sum0 = _mm_add_epi16(sum0, sum1);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[2] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref4 + (i) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref4 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref4 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref4 + (i + 3) * frefstride));

            T20 = _mm_loadl_epi64((__m128i*)(fref4 + (i) * frefstride + 16));
            T21 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 1) * frefstride + 16));
            T14 = _mm_unpacklo_epi64(T20, T21);

            T22 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 2) * frefstride + 16));
            T23 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 3) * frefstride + 16));
            T15 = _mm_unpacklo_epi64(T22, T23);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);
            T30 = _mm_sad_epu8(T04, T14);
            T31 = _mm_sad_epu8(T05, T15);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);
            sum1 = _mm_add_epi16(T30, T31);
            sum0 = _mm_add_epi16(sum0, sum1);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[3] += _mm_cvtsi128_si32(sum0);

            T00 = _mm_load_si128((__m128i*)(fenc + (i + 4) * FENC_STRIDE));
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 5) * FENC_STRIDE));
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 6) * FENC_STRIDE));
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 7) * FENC_STRIDE));

            T10 = _mm_loadl_epi64((__m128i*)(fenc + (i + 4) * FENC_STRIDE + 16));
            T11 = _mm_loadl_epi64((__m128i*)(fenc + (i + 5) * FENC_STRIDE + 16));
            T04 = _mm_unpacklo_epi64(T10, T11);

            T12 = _mm_loadl_epi64((__m128i*)(fenc + (i + 6) * FENC_STRIDE + 16));
            T13 = _mm_loadl_epi64((__m128i*)(fenc + (i + 7) * FENC_STRIDE + 16));
            T05 = _mm_unpacklo_epi64(T12, T13);

            T10 = _mm_loadu_si128((__m128i*)(fref1 + (i + 4) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref1 + (i + 5) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref1 + (i + 6) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref1 + (i + 7) * frefstride));

            T20 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 4) * frefstride + 16));
            T21 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 5) * frefstride + 16));
            T14 = _mm_unpacklo_epi64(T20, T21);

            T22 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 6) * frefstride + 16));
            T23 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 7) * frefstride + 16));
            T15 = _mm_unpacklo_epi64(T22, T23);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);
            T30 = _mm_sad_epu8(T04, T14);
            T31 = _mm_sad_epu8(T05, T15);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);
            sum1 = _mm_add_epi16(T30, T31);
            sum0 = _mm_add_epi16(sum0, sum1);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[0] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref2 + (i + 4) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref2 + (i + 5) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref2 + (i + 6) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref2 + (i + 7) * frefstride));

            T20 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 4) * frefstride + 16));
            T21 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 5) * frefstride + 16));
            T14 = _mm_unpacklo_epi64(T20, T21);

            T22 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 6) * frefstride + 16));
            T23 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 7) * frefstride + 16));
            T15 = _mm_unpacklo_epi64(T22, T23);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);
            T30 = _mm_sad_epu8(T04, T14);
            T31 = _mm_sad_epu8(T05, T15);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);
            sum1 = _mm_add_epi16(T30, T31);
            sum0 = _mm_add_epi16(sum0, sum1);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[1] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref3 + (i + 4) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref3 + (i + 5) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref3 + (i + 6) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref3 + (i + 7) * frefstride));

            T20 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 4) * frefstride + 16));
            T21 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 5) * frefstride + 16));
            T14 = _mm_unpacklo_epi64(T20, T21);

            T22 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 6) * frefstride + 16));
            T23 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 7) * frefstride + 16));
            T15 = _mm_unpacklo_epi64(T22, T23);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);
            T30 = _mm_sad_epu8(T04, T14);
            T31 = _mm_sad_epu8(T05, T15);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);
            sum1 = _mm_add_epi16(T30, T31);
            sum0 = _mm_add_epi16(sum0, sum1);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[2] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref4 + (i + 4) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref4 + (i + 5) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref4 + (i + 6) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref4 + (i + 7) * frefstride));

            T20 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 4) * frefstride + 16));
            T21 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 5) * frefstride + 16));
            T14 = _mm_unpacklo_epi64(T20, T21);

            T22 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 6) * frefstride + 16));
            T23 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 7) * frefstride + 16));
            T15 = _mm_unpacklo_epi64(T22, T23);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);
            T30 = _mm_sad_epu8(T04, T14);
            T31 = _mm_sad_epu8(T05, T15);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);
            sum1 = _mm_add_epi16(T30, T31);
            sum0 = _mm_add_epi16(sum0, sum1);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[3] += _mm_cvtsi128_si32(sum0);
        }
    }
    else
    {
        res[0] = res[1] = res[2] = res[3] = 0;
        for(int i = 0; i < ly; i += 4)
        {
            __m128i T00, T01, T02, T03, T04, T05;
            __m128i T10, T11, T12, T13, T14, T15;
            __m128i T20, T21, T22, T23;
            __m128i T30, T31;

            __m128i sum0, sum1;

            T00 = _mm_load_si128((__m128i*)(fenc + (i) * FENC_STRIDE));
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 1) * FENC_STRIDE));
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 2) * FENC_STRIDE));
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 3) * FENC_STRIDE));

            T10 = _mm_loadl_epi64((__m128i*)(fenc + (i) * FENC_STRIDE + 16));
            T11 = _mm_loadl_epi64((__m128i*)(fenc + (i + 1) * FENC_STRIDE + 16));
            T04 = _mm_unpacklo_epi64(T10, T11);

            T12 = _mm_loadl_epi64((__m128i*)(fenc + (i + 2) * FENC_STRIDE + 16));
            T13 = _mm_loadl_epi64((__m128i*)(fenc + (i + 3) * FENC_STRIDE + 16));
            T05 = _mm_unpacklo_epi64(T12, T13);

            T10 = _mm_loadu_si128((__m128i*)(fref1 + (i) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref1 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref1 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref1 + (i + 3) * frefstride));

            T20 = _mm_loadl_epi64((__m128i*)(fref1 + (i) * frefstride + 16));
            T21 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 1) * frefstride + 16));
            T14 = _mm_unpacklo_epi64(T20, T21);

            T22 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 2) * frefstride + 16));
            T23 = _mm_loadl_epi64((__m128i*)(fref1 + (i + 3) * frefstride + 16));
            T15 = _mm_unpacklo_epi64(T22, T23);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);
            T30 = _mm_sad_epu8(T04, T14);
            T31 = _mm_sad_epu8(T05, T15);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);
            sum1 = _mm_add_epi16(T30, T31);
            sum0 = _mm_add_epi16(sum0, sum1);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[0] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref2 + (i) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref2 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref2 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref2 + (i + 3) * frefstride));

            T20 = _mm_loadl_epi64((__m128i*)(fref2 + (i) * frefstride + 16));
            T21 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 1) * frefstride + 16));
            T14 = _mm_unpacklo_epi64(T20, T21);

            T22 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 2) * frefstride + 16));
            T23 = _mm_loadl_epi64((__m128i*)(fref2 + (i + 3) * frefstride + 16));
            T15 = _mm_unpacklo_epi64(T22, T23);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);
            T30 = _mm_sad_epu8(T04, T14);
            T31 = _mm_sad_epu8(T05, T15);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);
            sum1 = _mm_add_epi16(T30, T31);
            sum0 = _mm_add_epi16(sum0, sum1);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[1] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref3 + (i) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref3 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref3 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref3 + (i + 3) * frefstride));

            T20 = _mm_loadl_epi64((__m128i*)(fref3 + (i) * frefstride + 16));
            T21 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 1) * frefstride + 16));
            T14 = _mm_unpacklo_epi64(T20, T21);

            T22 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 2) * frefstride + 16));
            T23 = _mm_loadl_epi64((__m128i*)(fref3 + (i + 3) * frefstride + 16));
            T15 = _mm_unpacklo_epi64(T22, T23);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);
            T30 = _mm_sad_epu8(T04, T14);
            T31 = _mm_sad_epu8(T05, T15);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);
            sum1 = _mm_add_epi16(T30, T31);
            sum0 = _mm_add_epi16(sum0, sum1);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[2] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref4 + (i) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref4 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref4 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref4 + (i + 3) * frefstride));

            T20 = _mm_loadl_epi64((__m128i*)(fref4 + (i) * frefstride + 16));
            T21 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 1) * frefstride + 16));
            T14 = _mm_unpacklo_epi64(T20, T21);

            T22 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 2) * frefstride + 16));
            T23 = _mm_loadl_epi64((__m128i*)(fref4 + (i + 3) * frefstride + 16));
            T15 = _mm_unpacklo_epi64(T22, T23);

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);
            T30 = _mm_sad_epu8(T04, T14);
            T31 = _mm_sad_epu8(T05, T15);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);
            sum0 = _mm_add_epi16(T20, T22);
            sum1 = _mm_add_epi16(T30, T31);
            sum0 = _mm_add_epi16(sum0, sum1);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[3] += _mm_cvtsi128_si32(sum0);
        }
    }
}

template<int ly>
void sad_x4_32(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, pixel *fref4, intptr_t frefstride, int *res)
{
    assert((ly % 4) == 0);

    if (ly == 4)
    {
        __m128i T00, T01, T02, T03, T04, T05, T06, T07;
        __m128i T10, T11, T12, T13, T14, T15, T16, T17;
        __m128i T20, T21, T22, T23, T24, T25, T26, T27;
        __m128i sum0, sum1;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T01 = _mm_load_si128((__m128i*)(fenc + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * FENC_STRIDE));

        T04 = _mm_load_si128((__m128i*)(fenc + 16));
        T05 = _mm_load_si128((__m128i*)(fenc + FENC_STRIDE + 16));
        T06 = _mm_load_si128((__m128i*)(fenc + (2) * FENC_STRIDE + 16));
        T07 = _mm_load_si128((__m128i*)(fenc + (3) * FENC_STRIDE + 16));

        T10 = _mm_loadu_si128((__m128i*)(fref1));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (3) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref1 + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref1 + frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref1 + (2) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref1 + (3) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (3) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref2 + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref2 + frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref2 + (2) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref2 + (3) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (3) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref3 + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref3 + frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref3 + (2) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref3 + (3) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref4));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + (3) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref4 + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref4 + frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref4 + (2) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref4 + (3) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[3] = _mm_cvtsi128_si32(sum0);
    }
    else if (ly == 8)
    {
        __m128i T00, T01, T02, T03, T04, T05, T06, T07;
        __m128i T10, T11, T12, T13, T14, T15, T16, T17;
        __m128i T20, T21, T22, T23, T24, T25, T26, T27;
        __m128i sum0, sum1;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T01 = _mm_load_si128((__m128i*)(fenc + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * FENC_STRIDE));

        T04 = _mm_load_si128((__m128i*)(fenc + 16));
        T05 = _mm_load_si128((__m128i*)(fenc + FENC_STRIDE + 16));
        T06 = _mm_load_si128((__m128i*)(fenc + (2) * FENC_STRIDE + 16));
        T07 = _mm_load_si128((__m128i*)(fenc + (3) * FENC_STRIDE + 16));

        T10 = _mm_loadu_si128((__m128i*)(fref1));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (3) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref1 + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref1 + frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref1 + (2) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref1 + (3) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (3) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref2 + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref2 + frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref2 + (2) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref2 + (3) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (3) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref3 + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref3 + frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref3 + (2) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref3 + (3) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref4));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + (3) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref4 + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref4 + frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref4 + (2) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref4 + (3) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[3] = _mm_cvtsi128_si32(sum0);

        T00 = _mm_load_si128((__m128i*)(fenc + (4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + (5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (7) * FENC_STRIDE));

        T04 = _mm_load_si128((__m128i*)(fenc + (4) * FENC_STRIDE + 16));
        T05 = _mm_load_si128((__m128i*)(fenc + (5) * FENC_STRIDE + 16));
        T06 = _mm_load_si128((__m128i*)(fenc + (6) * FENC_STRIDE + 16));
        T07 = _mm_load_si128((__m128i*)(fenc + (7) * FENC_STRIDE + 16));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (7) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref1 + (4) * frefstride + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref1 + (5) * frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref1 + (6) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref1 + (7) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (7) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref2 + (4) * frefstride + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref2 + (5) * frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref2 + (6) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref2 + (7) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (7) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref3 + (4) * frefstride + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref3 + (5) * frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref3 + (6) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref3 + (7) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + (7) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref4 + (4) * frefstride + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref4 + (5) * frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref4 + (6) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref4 + (7) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[3] += _mm_cvtsi128_si32(sum0);
    }
    else if (ly ==16)
    {
        __m128i T00, T01, T02, T03, T04, T05, T06, T07;
        __m128i T10, T11, T12, T13, T14, T15, T16, T17;
        __m128i T20, T21, T22, T23, T24, T25, T26, T27;
        __m128i sum0, sum1;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T01 = _mm_load_si128((__m128i*)(fenc + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * FENC_STRIDE));

        T04 = _mm_load_si128((__m128i*)(fenc + 16));
        T05 = _mm_load_si128((__m128i*)(fenc + FENC_STRIDE + 16));
        T06 = _mm_load_si128((__m128i*)(fenc + (2) * FENC_STRIDE + 16));
        T07 = _mm_load_si128((__m128i*)(fenc + (3) * FENC_STRIDE + 16));

        T10 = _mm_loadu_si128((__m128i*)(fref1));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (3) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref1 + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref1 + frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref1 + (2) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref1 + (3) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (3) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref2 + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref2 + frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref2 + (2) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref2 + (3) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (3) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref3 + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref3 + frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref3 + (2) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref3 + (3) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] = _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref4));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + (3) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref4 + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref4 + frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref4 + (2) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref4 + (3) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[3] = _mm_cvtsi128_si32(sum0);

        T00 = _mm_load_si128((__m128i*)(fenc + (4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + (5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (7) * FENC_STRIDE));

        T04 = _mm_load_si128((__m128i*)(fenc + (4) * FENC_STRIDE + 16));
        T05 = _mm_load_si128((__m128i*)(fenc + (5) * FENC_STRIDE + 16));
        T06 = _mm_load_si128((__m128i*)(fenc + (6) * FENC_STRIDE + 16));
        T07 = _mm_load_si128((__m128i*)(fenc + (7) * FENC_STRIDE + 16));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (7) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref1 + (4) * frefstride + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref1 + (5) * frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref1 + (6) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref1 + (7) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (7) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref2 + (4) * frefstride + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref2 + (5) * frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref2 + (6) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref2 + (7) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (7) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref3 + (4) * frefstride + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref3 + (5) * frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref3 + (6) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref3 + (7) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + (7) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref4 + (4) * frefstride + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref4 + (5) * frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref4 + (6) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref4 + (7) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[3] += _mm_cvtsi128_si32(sum0);

        T00 = _mm_load_si128((__m128i*)(fenc + (8) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + (9) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (10) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (11) * FENC_STRIDE));

        T04 = _mm_load_si128((__m128i*)(fenc + (8) * FENC_STRIDE + 16));
        T05 = _mm_load_si128((__m128i*)(fenc + (9) * FENC_STRIDE + 16));
        T06 = _mm_load_si128((__m128i*)(fenc + (10) * FENC_STRIDE + 16));
        T07 = _mm_load_si128((__m128i*)(fenc + (11) * FENC_STRIDE + 16));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (11) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref1 + (8) * frefstride + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref1 + (9) * frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref1 + (10) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref1 + (11) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (11) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref2 + (8) * frefstride + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref2 + (9) * frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref2 + (10) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref2 + (11) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (11) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref3 + (8) * frefstride + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref3 + (9) * frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref3 + (10) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref3 + (11) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + (11) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref4 + (8) * frefstride + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref4 + (9) * frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref4 + (10) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref4 + (11) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[3] += _mm_cvtsi128_si32(sum0);

        T00 = _mm_load_si128((__m128i*)(fenc + (12) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + (13) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (14) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (15) * FENC_STRIDE));

        T04 = _mm_load_si128((__m128i*)(fenc + (12) * FENC_STRIDE + 16));
        T05 = _mm_load_si128((__m128i*)(fenc + (13) * FENC_STRIDE + 16));
        T06 = _mm_load_si128((__m128i*)(fenc + (14) * FENC_STRIDE + 16));
        T07 = _mm_load_si128((__m128i*)(fenc + (15) * FENC_STRIDE + 16));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + (15) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref1 + (12) * frefstride + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref1 + (13) * frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref1 + (14) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref1 + (15) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[0] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (15) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref2 + (12) * frefstride + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref2 + (13) * frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref2 + (14) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref2 + (15) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[1] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (15) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref3 + (12) * frefstride + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref3 + (13) * frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref3 + (14) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref3 + (15) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[2] += _mm_cvtsi128_si32(sum0);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + (15) * frefstride));

        T14 = _mm_loadu_si128((__m128i*)(fref4 + (12) * frefstride + 16));
        T15 = _mm_loadu_si128((__m128i*)(fref4 + (13) * frefstride + 16));
        T16 = _mm_loadu_si128((__m128i*)(fref4 + (14) * frefstride + 16));
        T17 = _mm_loadu_si128((__m128i*)(fref4 + (15) * frefstride + 16));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T24 = _mm_sad_epu8(T04, T14);
        T25 = _mm_sad_epu8(T05, T15);
        T26 = _mm_sad_epu8(T06, T16);
        T27 = _mm_sad_epu8(T07, T17);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);

        T24 = _mm_add_epi16(T24, T25);
        T26 = _mm_add_epi16(T26, T27);
        
        sum0 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi16(sum0, T24);
        sum0 = _mm_add_epi16(sum0, T26);

        sum1 = _mm_shuffle_epi32(sum0, 2);
        sum0 = _mm_add_epi32(sum0, sum1);
        res[3] += _mm_cvtsi128_si32(sum0);
    }
    else if ((ly % 8) == 0)
    {
        res[0] = res[1] = res[2] = res[3] = 0;
        for(int i = 0; i < ly; i += 8)
        {
            __m128i T00, T01, T02, T03, T04, T05, T06, T07;
            __m128i T10, T11, T12, T13, T14, T15, T16, T17;
            __m128i T20, T21, T22, T23, T24, T25, T26, T27;
            __m128i sum0, sum1;

            T00 = _mm_load_si128((__m128i*)(fenc + (i) * FENC_STRIDE));
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 1) * FENC_STRIDE));
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 2) * FENC_STRIDE));
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 3) * FENC_STRIDE));

            T04 = _mm_load_si128((__m128i*)(fenc + (i) * FENC_STRIDE + 16));
            T05 = _mm_load_si128((__m128i*)(fenc + (i + 1) * FENC_STRIDE + 16));
            T06 = _mm_load_si128((__m128i*)(fenc + (i + 2) * FENC_STRIDE + 16));
            T07 = _mm_load_si128((__m128i*)(fenc + (i + 3) * FENC_STRIDE + 16));

            T10 = _mm_loadu_si128((__m128i*)(fref1 + (i) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref1 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref1 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref1 + (i + 3) * frefstride));

            T14 = _mm_loadu_si128((__m128i*)(fref1 + (i) * frefstride + 16));
            T15 = _mm_loadu_si128((__m128i*)(fref1 + (i + 1) * frefstride + 16));
            T16 = _mm_loadu_si128((__m128i*)(fref1 + (i + 2) * frefstride + 16));
            T17 = _mm_loadu_si128((__m128i*)(fref1 + (i + 3) * frefstride + 16));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T24 = _mm_sad_epu8(T04, T14);
            T25 = _mm_sad_epu8(T05, T15);
            T26 = _mm_sad_epu8(T06, T16);
            T27 = _mm_sad_epu8(T07, T17);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);

            T24 = _mm_add_epi16(T24, T25);
            T26 = _mm_add_epi16(T26, T27);
        
            sum0 = _mm_add_epi16(T20, T22);
            sum0 = _mm_add_epi16(sum0, T24);
            sum0 = _mm_add_epi16(sum0, T26);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[0] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref2 + (i) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref2 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref2 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref2 + (i + 3) * frefstride));

            T14 = _mm_loadu_si128((__m128i*)(fref2 + (i) * frefstride + 16));
            T15 = _mm_loadu_si128((__m128i*)(fref2 + (i + 1) * frefstride + 16));
            T16 = _mm_loadu_si128((__m128i*)(fref2 + (i + 2) * frefstride + 16));
            T17 = _mm_loadu_si128((__m128i*)(fref2 + (i + 3) * frefstride + 16));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T24 = _mm_sad_epu8(T04, T14);
            T25 = _mm_sad_epu8(T05, T15);
            T26 = _mm_sad_epu8(T06, T16);
            T27 = _mm_sad_epu8(T07, T17);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);

            T24 = _mm_add_epi16(T24, T25);
            T26 = _mm_add_epi16(T26, T27);
        
            sum0 = _mm_add_epi16(T20, T22);
            sum0 = _mm_add_epi16(sum0, T24);
            sum0 = _mm_add_epi16(sum0, T26);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[1] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref3 + (i) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref3 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref3 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref3 + (i + 3) * frefstride));

            T14 = _mm_loadu_si128((__m128i*)(fref3 + (i) * frefstride + 16));
            T15 = _mm_loadu_si128((__m128i*)(fref3 + (i + 1) * frefstride + 16));
            T16 = _mm_loadu_si128((__m128i*)(fref3 + (i + 2) * frefstride + 16));
            T17 = _mm_loadu_si128((__m128i*)(fref3 + (i + 3) * frefstride + 16));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T24 = _mm_sad_epu8(T04, T14);
            T25 = _mm_sad_epu8(T05, T15);
            T26 = _mm_sad_epu8(T06, T16);
            T27 = _mm_sad_epu8(T07, T17);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);

            T24 = _mm_add_epi16(T24, T25);
            T26 = _mm_add_epi16(T26, T27);
        
            sum0 = _mm_add_epi16(T20, T22);
            sum0 = _mm_add_epi16(sum0, T24);
            sum0 = _mm_add_epi16(sum0, T26);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[2] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref4 + (i) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref4 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref4 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref4 + (i + 3) * frefstride));

            T14 = _mm_loadu_si128((__m128i*)(fref4 + (i) * frefstride + 16));
            T15 = _mm_loadu_si128((__m128i*)(fref4 + (i + 1) * frefstride + 16));
            T16 = _mm_loadu_si128((__m128i*)(fref4 + (i + 2) * frefstride + 16));
            T17 = _mm_loadu_si128((__m128i*)(fref4 + (i + 3) * frefstride + 16));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T24 = _mm_sad_epu8(T04, T14);
            T25 = _mm_sad_epu8(T05, T15);
            T26 = _mm_sad_epu8(T06, T16);
            T27 = _mm_sad_epu8(T07, T17);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);

            T24 = _mm_add_epi16(T24, T25);
            T26 = _mm_add_epi16(T26, T27);
        
            sum0 = _mm_add_epi16(T20, T22);
            sum0 = _mm_add_epi16(sum0, T24);
            sum0 = _mm_add_epi16(sum0, T26);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[3] += _mm_cvtsi128_si32(sum0);

            T00 = _mm_load_si128((__m128i*)(fenc + (i + 4) * FENC_STRIDE));
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 5) * FENC_STRIDE));
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 6) * FENC_STRIDE));
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 7) * FENC_STRIDE));

            T04 = _mm_load_si128((__m128i*)(fenc + (i + 4) * FENC_STRIDE + 16));
            T05 = _mm_load_si128((__m128i*)(fenc + (i + 5) * FENC_STRIDE + 16));
            T06 = _mm_load_si128((__m128i*)(fenc + (i + 6) * FENC_STRIDE + 16));
            T07 = _mm_load_si128((__m128i*)(fenc + (i + 7) * FENC_STRIDE + 16));

            T10 = _mm_loadu_si128((__m128i*)(fref1 + (i + 4) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref1 + (i + 5) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref1 + (i + 6) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref1 + (i + 7) * frefstride));

            T14 = _mm_loadu_si128((__m128i*)(fref1 + (i + 4) * frefstride + 16));
            T15 = _mm_loadu_si128((__m128i*)(fref1 + (i + 5) * frefstride + 16));
            T16 = _mm_loadu_si128((__m128i*)(fref1 + (i + 6) * frefstride + 16));
            T17 = _mm_loadu_si128((__m128i*)(fref1 + (i + 7) * frefstride + 16));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T24 = _mm_sad_epu8(T04, T14);
            T25 = _mm_sad_epu8(T05, T15);
            T26 = _mm_sad_epu8(T06, T16);
            T27 = _mm_sad_epu8(T07, T17);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);

            T24 = _mm_add_epi16(T24, T25);
            T26 = _mm_add_epi16(T26, T27);
        
            sum0 = _mm_add_epi16(T20, T22);
            sum0 = _mm_add_epi16(sum0, T24);
            sum0 = _mm_add_epi16(sum0, T26);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[0] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref2 + (i + 4) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref2 + (i + 5) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref2 + (i + 6) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref2 + (i + 7) * frefstride));

            T14 = _mm_loadu_si128((__m128i*)(fref2 + (i + 4) * frefstride + 16));
            T15 = _mm_loadu_si128((__m128i*)(fref2 + (i + 5) * frefstride + 16));
            T16 = _mm_loadu_si128((__m128i*)(fref2 + (i + 6) * frefstride + 16));
            T17 = _mm_loadu_si128((__m128i*)(fref2 + (i + 7) * frefstride + 16));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T24 = _mm_sad_epu8(T04, T14);
            T25 = _mm_sad_epu8(T05, T15);
            T26 = _mm_sad_epu8(T06, T16);
            T27 = _mm_sad_epu8(T07, T17);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);

            T24 = _mm_add_epi16(T24, T25);
            T26 = _mm_add_epi16(T26, T27);
        
            sum0 = _mm_add_epi16(T20, T22);
            sum0 = _mm_add_epi16(sum0, T24);
            sum0 = _mm_add_epi16(sum0, T26);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[1] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref3 + (i + 4) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref3 + (i + 5) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref3 + (i + 6) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref3 + (i + 7) * frefstride));

            T14 = _mm_loadu_si128((__m128i*)(fref3 + (i + 4) * frefstride + 16));
            T15 = _mm_loadu_si128((__m128i*)(fref3 + (i + 5) * frefstride + 16));
            T16 = _mm_loadu_si128((__m128i*)(fref3 + (i + 6) * frefstride + 16));
            T17 = _mm_loadu_si128((__m128i*)(fref3 + (i + 7) * frefstride + 16));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T24 = _mm_sad_epu8(T04, T14);
            T25 = _mm_sad_epu8(T05, T15);
            T26 = _mm_sad_epu8(T06, T16);
            T27 = _mm_sad_epu8(T07, T17);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);

            T24 = _mm_add_epi16(T24, T25);
            T26 = _mm_add_epi16(T26, T27);
        
            sum0 = _mm_add_epi16(T20, T22);
            sum0 = _mm_add_epi16(sum0, T24);
            sum0 = _mm_add_epi16(sum0, T26);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[2] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref4 + (i + 4) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref4 + (i + 5) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref4 + (i + 6) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref4 + (i + 7) * frefstride));

            T14 = _mm_loadu_si128((__m128i*)(fref4 + (i + 4) * frefstride + 16));
            T15 = _mm_loadu_si128((__m128i*)(fref4 + (i + 5) * frefstride + 16));
            T16 = _mm_loadu_si128((__m128i*)(fref4 + (i + 6) * frefstride + 16));
            T17 = _mm_loadu_si128((__m128i*)(fref4 + (i + 7) * frefstride + 16));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T24 = _mm_sad_epu8(T04, T14);
            T25 = _mm_sad_epu8(T05, T15);
            T26 = _mm_sad_epu8(T06, T16);
            T27 = _mm_sad_epu8(T07, T17);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);

            T24 = _mm_add_epi16(T24, T25);
            T26 = _mm_add_epi16(T26, T27);
        
            sum0 = _mm_add_epi16(T20, T22);
            sum0 = _mm_add_epi16(sum0, T24);
            sum0 = _mm_add_epi16(sum0, T26);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[3] += _mm_cvtsi128_si32(sum0);
        }
    }
    else
    {
        res[0] = res[1] = res[2] = res[3] = 0;
        for(int i = 0; i < ly; i += 4)
        {
            __m128i T00, T01, T02, T03, T04, T05, T06, T07;
            __m128i T10, T11, T12, T13, T14, T15, T16, T17;
            __m128i T20, T21, T22, T23, T24, T25, T26, T27;
            __m128i sum0, sum1;

            T00 = _mm_load_si128((__m128i*)(fenc + (i) * FENC_STRIDE));
            T01 = _mm_load_si128((__m128i*)(fenc + (i + 1) * FENC_STRIDE));
            T02 = _mm_load_si128((__m128i*)(fenc + (i + 2) * FENC_STRIDE));
            T03 = _mm_load_si128((__m128i*)(fenc + (i + 3) * FENC_STRIDE));

            T04 = _mm_load_si128((__m128i*)(fenc + (i) * FENC_STRIDE + 16));
            T05 = _mm_load_si128((__m128i*)(fenc + (i + 1) * FENC_STRIDE + 16));
            T06 = _mm_load_si128((__m128i*)(fenc + (i + 2) * FENC_STRIDE + 16));
            T07 = _mm_load_si128((__m128i*)(fenc + (i + 3) * FENC_STRIDE + 16));

            T10 = _mm_loadu_si128((__m128i*)(fref1 + (i) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref1 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref1 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref1 + (i + 3) * frefstride));

            T14 = _mm_loadu_si128((__m128i*)(fref1 + (i) * frefstride + 16));
            T15 = _mm_loadu_si128((__m128i*)(fref1 + (i + 1) * frefstride + 16));
            T16 = _mm_loadu_si128((__m128i*)(fref1 + (i + 2) * frefstride + 16));
            T17 = _mm_loadu_si128((__m128i*)(fref1 + (i + 3) * frefstride + 16));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T24 = _mm_sad_epu8(T04, T14);
            T25 = _mm_sad_epu8(T05, T15);
            T26 = _mm_sad_epu8(T06, T16);
            T27 = _mm_sad_epu8(T07, T17);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);

            T24 = _mm_add_epi16(T24, T25);
            T26 = _mm_add_epi16(T26, T27);
        
            sum0 = _mm_add_epi16(T20, T22);
            sum0 = _mm_add_epi16(sum0, T24);
            sum0 = _mm_add_epi16(sum0, T26);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[0] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref2 + (i) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref2 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref2 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref2 + (i + 3) * frefstride));

            T14 = _mm_loadu_si128((__m128i*)(fref2 + (i) * frefstride + 16));
            T15 = _mm_loadu_si128((__m128i*)(fref2 + (i + 1) * frefstride + 16));
            T16 = _mm_loadu_si128((__m128i*)(fref2 + (i + 2) * frefstride + 16));
            T17 = _mm_loadu_si128((__m128i*)(fref2 + (i + 3) * frefstride + 16));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T24 = _mm_sad_epu8(T04, T14);
            T25 = _mm_sad_epu8(T05, T15);
            T26 = _mm_sad_epu8(T06, T16);
            T27 = _mm_sad_epu8(T07, T17);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);

            T24 = _mm_add_epi16(T24, T25);
            T26 = _mm_add_epi16(T26, T27);
        
            sum0 = _mm_add_epi16(T20, T22);
            sum0 = _mm_add_epi16(sum0, T24);
            sum0 = _mm_add_epi16(sum0, T26);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[1] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref3 + (i) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref3 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref3 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref3 + (i + 3) * frefstride));

            T14 = _mm_loadu_si128((__m128i*)(fref3 + (i) * frefstride + 16));
            T15 = _mm_loadu_si128((__m128i*)(fref3 + (i + 1) * frefstride + 16));
            T16 = _mm_loadu_si128((__m128i*)(fref3 + (i + 2) * frefstride + 16));
            T17 = _mm_loadu_si128((__m128i*)(fref3 + (i + 3) * frefstride + 16));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T24 = _mm_sad_epu8(T04, T14);
            T25 = _mm_sad_epu8(T05, T15);
            T26 = _mm_sad_epu8(T06, T16);
            T27 = _mm_sad_epu8(T07, T17);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);

            T24 = _mm_add_epi16(T24, T25);
            T26 = _mm_add_epi16(T26, T27);
        
            sum0 = _mm_add_epi16(T20, T22);
            sum0 = _mm_add_epi16(sum0, T24);
            sum0 = _mm_add_epi16(sum0, T26);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[2] += _mm_cvtsi128_si32(sum0);

            T10 = _mm_loadu_si128((__m128i*)(fref4 + (i) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref4 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref4 + (i + 2) * frefstride));
            T13 = _mm_loadu_si128((__m128i*)(fref4 + (i + 3) * frefstride));

            T14 = _mm_loadu_si128((__m128i*)(fref4 + (i) * frefstride + 16));
            T15 = _mm_loadu_si128((__m128i*)(fref4 + (i + 1) * frefstride + 16));
            T16 = _mm_loadu_si128((__m128i*)(fref4 + (i + 2) * frefstride + 16));
            T17 = _mm_loadu_si128((__m128i*)(fref4 + (i + 3) * frefstride + 16));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);
            T23 = _mm_sad_epu8(T03, T13);

            T24 = _mm_sad_epu8(T04, T14);
            T25 = _mm_sad_epu8(T05, T15);
            T26 = _mm_sad_epu8(T06, T16);
            T27 = _mm_sad_epu8(T07, T17);

            T20 = _mm_add_epi16(T20, T21);
            T22 = _mm_add_epi16(T22, T23);

            T24 = _mm_add_epi16(T24, T25);
            T26 = _mm_add_epi16(T26, T27);
        
            sum0 = _mm_add_epi16(T20, T22);
            sum0 = _mm_add_epi16(sum0, T24);
            sum0 = _mm_add_epi16(sum0, T26);

            sum1 = _mm_shuffle_epi32(sum0, 2);
            sum0 = _mm_add_epi32(sum0, sum1);
            res[3] += _mm_cvtsi128_si32(sum0);
        }
    }
}

template<int ly>
int sse_pp8(pixel* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i sum = _mm_setzero_si128();

    for (; rows != 0; rows--)
    {
        __m128i m1 = _mm_loadu_si128((__m128i const*)(fenc));
        m1 = _mm_cvtepu8_epi16(m1);
        __m128i n1 = _mm_loadu_si128((__m128i const*)(fref));
        n1 = _mm_cvtepu8_epi16(n1);

        __m128i diff = _mm_sub_epi16(m1, n1);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        fenc += strideFenc;
        fref += strideFref;
    }

    sum = _mm_hadd_epi32(sum, sum);
    sum = _mm_hadd_epi32(sum, sum);
    return _mm_cvtsi128_si32(sum);
}

template<int ly>
int sse_pp12(pixel* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i sum = _mm_setzero_si128();
    __m128i zero = _mm_setzero_si128();

    for (; rows != 0; rows--)
    {
        __m128i m1 = _mm_loadu_si128((__m128i const*)(fenc));
        __m128i n1 = _mm_loadu_si128((__m128i const*)(fref));

        __m128i m1lo = _mm_cvtepu8_epi16(m1);
        __m128i m1hi = _mm_unpackhi_epi8(m1, zero);

        __m128i n1lo = _mm_cvtepu8_epi16(n1);
        __m128i n1hi = _mm_unpackhi_epi8(n1, zero);

        __m128i difflo = _mm_sub_epi16(m1lo, n1lo);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(difflo, difflo));

        __m128i diffhi = _mm_sub_epi16(m1hi, n1hi);
        __m128i sum_temp = _mm_madd_epi16(diffhi, diffhi);

        sum = _mm_add_epi32(sum, _mm_srli_si128(_mm_slli_si128(sum_temp, 8), 8));

        fenc += strideFenc;
        fref += strideFref;
    }

    sum = _mm_hadd_epi32(sum, sum);
    sum = _mm_hadd_epi32(sum, sum);
    return _mm_cvtsi128_si32(sum);
}

template<int ly>
int sse_pp16(pixel* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i sum = _mm_setzero_si128();
    __m128i zero = _mm_setzero_si128();

    for (; rows != 0; rows--)
    {
        __m128i m1 = _mm_loadu_si128((__m128i const*)(fenc));
        __m128i n1 = _mm_loadu_si128((__m128i const*)(fref));

        __m128i m1lo = _mm_cvtepu8_epi16(m1);
        __m128i m1hi = _mm_unpackhi_epi8(m1, zero);

        __m128i n1lo = _mm_cvtepu8_epi16(n1);
        __m128i n1hi = _mm_unpackhi_epi8(n1, zero);

        __m128i diff = _mm_sub_epi16(m1lo, n1lo);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        diff = _mm_sub_epi16(m1hi, n1hi);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        fenc += strideFenc;
        fref += strideFref;
    }

    sum = _mm_hadd_epi32(sum, sum);
    sum = _mm_hadd_epi32(sum, sum);
    return _mm_cvtsi128_si32(sum);
}

template<int ly>
int sse_pp24(pixel* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i sum = _mm_setzero_si128();
    __m128i zero = _mm_setzero_si128();

    for (; rows != 0; rows--)
    {
        __m128i m1 = _mm_loadu_si128((__m128i const*)(fenc));
        __m128i n1 = _mm_loadu_si128((__m128i const*)(fref));

        __m128i m1lo = _mm_cvtepu8_epi16(m1);
        __m128i m1hi = _mm_unpackhi_epi8(m1, zero);

        __m128i n1lo = _mm_cvtepu8_epi16(n1);
        __m128i n1hi = _mm_unpackhi_epi8(n1, zero);

        __m128i diff = _mm_sub_epi16(m1lo, n1lo);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        diff = _mm_sub_epi16(m1hi, n1hi);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        m1 = _mm_loadu_si128((__m128i const*)(fenc + 16));
        m1 = _mm_cvtepu8_epi16(m1);
        n1 = _mm_loadu_si128((__m128i const*)(fref + 16));
        n1 = _mm_cvtepu8_epi16(n1);

        diff = _mm_sub_epi16(m1, n1);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        fenc += strideFenc;
        fref += strideFref;
    }

    sum = _mm_hadd_epi32(sum, sum);
    sum = _mm_hadd_epi32(sum, sum);
    return _mm_cvtsi128_si32(sum);
}

template<int ly>
int sse_pp32(pixel* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i sum = _mm_setzero_si128();
    __m128i zero = _mm_setzero_si128();

    for (; rows != 0; rows--)
    {
        __m128i m1 = _mm_loadu_si128((__m128i const*)(fenc));
        __m128i n1 = _mm_loadu_si128((__m128i const*)(fref));

        __m128i m1lo = _mm_cvtepu8_epi16(m1);
        __m128i m1hi = _mm_unpackhi_epi8(m1, zero);

        __m128i n1lo = _mm_cvtepu8_epi16(n1);
        __m128i n1hi = _mm_unpackhi_epi8(n1, zero);

        __m128i diff = _mm_sub_epi16(m1lo, n1lo);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        diff = _mm_sub_epi16(m1hi, n1hi);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        m1 = _mm_loadu_si128((__m128i const*)(fenc + 16));
        n1 = _mm_loadu_si128((__m128i const*)(fref + 16));

        m1lo = _mm_cvtepu8_epi16(m1);
        m1hi = _mm_unpackhi_epi8(m1, zero);

        n1lo = _mm_cvtepu8_epi16(n1);
        n1hi = _mm_unpackhi_epi8(n1, zero);

        diff = _mm_sub_epi16(m1lo, n1lo);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        diff = _mm_sub_epi16(m1hi, n1hi);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        fenc += strideFenc;
        fref += strideFref;
    }

    sum = _mm_hadd_epi32(sum, sum);
    sum = _mm_hadd_epi32(sum, sum);
    return _mm_cvtsi128_si32(sum);
}

template<int ly>
int sse_pp48(pixel* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i sum = _mm_setzero_si128();
    __m128i zero = _mm_setzero_si128();

    for (; rows != 0; rows--)
    {
        __m128i m1 = _mm_loadu_si128((__m128i const*)(fenc));
        __m128i n1 = _mm_loadu_si128((__m128i const*)(fref));

        __m128i m1lo = _mm_cvtepu8_epi16(m1);
        __m128i m1hi = _mm_unpackhi_epi8(m1, zero);

        __m128i n1lo = _mm_cvtepu8_epi16(n1);
        __m128i n1hi = _mm_unpackhi_epi8(n1, zero);

        __m128i diff = _mm_sub_epi16(m1lo, n1lo);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        diff = _mm_sub_epi16(m1hi, n1hi);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        m1 = _mm_loadu_si128((__m128i const*)(fenc + 16));
        n1 = _mm_loadu_si128((__m128i const*)(fref + 16));

        m1lo = _mm_cvtepu8_epi16(m1);
        m1hi = _mm_unpackhi_epi8(m1, zero);

        n1lo = _mm_cvtepu8_epi16(n1);
        n1hi = _mm_unpackhi_epi8(n1, zero);

        diff = _mm_sub_epi16(m1lo, n1lo);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        diff = _mm_sub_epi16(m1hi, n1hi);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        m1 = _mm_loadu_si128((__m128i const*)(fenc + 32));
        n1 = _mm_loadu_si128((__m128i const*)(fref + 32));

        m1lo = _mm_cvtepu8_epi16(m1);
        m1hi = _mm_unpackhi_epi8(m1, zero);

        n1lo = _mm_cvtepu8_epi16(n1);
        n1hi = _mm_unpackhi_epi8(n1, zero);

        diff = _mm_sub_epi16(m1lo, n1lo);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        diff = _mm_sub_epi16(m1hi, n1hi);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        fenc += strideFenc;
        fref += strideFref;
    }

    sum = _mm_hadd_epi32(sum, sum);
    sum = _mm_hadd_epi32(sum, sum);
    return _mm_cvtsi128_si32(sum);
}

template<int ly>
int sse_pp64(pixel* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i sum = _mm_setzero_si128();
    __m128i zero = _mm_setzero_si128();

    for (; rows != 0; rows--)
    {
        __m128i m1 = _mm_loadu_si128((__m128i const*)(fenc));
        __m128i n1 = _mm_loadu_si128((__m128i const*)(fref));

        __m128i m1lo = _mm_cvtepu8_epi16(m1);
        __m128i m1hi = _mm_unpackhi_epi8(m1, zero);

        __m128i n1lo = _mm_cvtepu8_epi16(n1);
        __m128i n1hi = _mm_unpackhi_epi8(n1, zero);

        __m128i diff = _mm_sub_epi16(m1lo, n1lo);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        diff = _mm_sub_epi16(m1hi, n1hi);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        m1 = _mm_loadu_si128((__m128i const*)(fenc + 16));
        n1 = _mm_loadu_si128((__m128i const*)(fref + 16));

        m1lo = _mm_cvtepu8_epi16(m1);
        m1hi = _mm_unpackhi_epi8(m1, zero);

        n1lo = _mm_cvtepu8_epi16(n1);
        n1hi = _mm_unpackhi_epi8(n1, zero);

        diff = _mm_sub_epi16(m1lo, n1lo);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        diff = _mm_sub_epi16(m1hi, n1hi);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        m1 = _mm_loadu_si128((__m128i const*)(fenc + 32));
        n1 = _mm_loadu_si128((__m128i const*)(fref + 32));

        m1lo = _mm_cvtepu8_epi16(m1);
        m1hi = _mm_unpackhi_epi8(m1, zero);

        n1lo = _mm_cvtepu8_epi16(n1);
        n1hi = _mm_unpackhi_epi8(n1, zero);

        diff = _mm_sub_epi16(m1lo, n1lo);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        diff = _mm_sub_epi16(m1hi, n1hi);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        m1 = _mm_loadu_si128((__m128i const*)(fenc + 48));
        n1 = _mm_loadu_si128((__m128i const*)(fref + 48));

        m1lo = _mm_cvtepu8_epi16(m1);
        m1hi = _mm_unpackhi_epi8(m1, zero);

        n1lo = _mm_cvtepu8_epi16(n1);
        n1hi = _mm_unpackhi_epi8(n1, zero);

        diff = _mm_sub_epi16(m1lo, n1lo);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        diff = _mm_sub_epi16(m1hi, n1hi);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        fenc += strideFenc;
        fref += strideFref;
    }

    sum = _mm_hadd_epi32(sum, sum);
    sum = _mm_hadd_epi32(sum, sum);
    return _mm_cvtsi128_si32(sum);
}
#endif
}

#define INSTRSET 5
#include "vectorclass.h"
#define ARCH sse41

#include "pixel.inc"
