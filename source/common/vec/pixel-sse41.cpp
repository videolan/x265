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

#include "TLibCommon/TComRom.h"
#include "primitives.h"
#include <assert.h>
#include <xmmintrin.h> // SSE
#include <smmintrin.h> // SSE4.1

using namespace x265;

namespace {
#if !HIGH_BIT_DEPTH
template<int ly>
// will only be instanced with ly == 16
int sad_12(pixel *fenc, intptr_t fencstride, pixel *fref, intptr_t frefstride)
{
    assert(ly == 16);
    __m128i sum0 = _mm_setzero_si128();
    __m128i sum1 = _mm_setzero_si128();
    __m128i T00, T01, T02, T03;
    __m128i T10, T11, T12, T13;
    __m128i T20, T21, T22, T23;

#define MASK _mm_set_epi32(0x00000000, 0xffffffff, 0xffffffff, 0xffffffff)

#define PROCESS_12x4(BASE) \
    T00 = _mm_load_si128((__m128i*)(fenc + (BASE + 0) * fencstride)); \
    T00 = _mm_and_si128(T00, MASK); \
    T01 = _mm_load_si128((__m128i*)(fenc + (BASE + 1) * fencstride)); \
    T01 = _mm_and_si128(T01, MASK); \
    T02 = _mm_load_si128((__m128i*)(fenc + (BASE + 2) * fencstride)); \
    T02 = _mm_and_si128(T02, MASK); \
    T03 = _mm_load_si128((__m128i*)(fenc + (BASE + 3) * fencstride)); \
    T03 = _mm_and_si128(T03, MASK); \
    T10 = _mm_loadu_si128((__m128i*)(fref + (BASE + 0) * frefstride)); \
    T10 = _mm_and_si128(T10, MASK); \
    T11 = _mm_loadu_si128((__m128i*)(fref + (BASE + 1) * frefstride)); \
    T11 = _mm_and_si128(T11, MASK); \
    T12 = _mm_loadu_si128((__m128i*)(fref + (BASE + 2) * frefstride)); \
    T12 = _mm_and_si128(T12, MASK); \
    T13 = _mm_loadu_si128((__m128i*)(fref + (BASE + 3) * frefstride)); \
    T13 = _mm_and_si128(T13, MASK); \
    T20 = _mm_sad_epu8(T00, T10); \
    T21 = _mm_sad_epu8(T01, T11); \
    T22 = _mm_sad_epu8(T02, T12); \
    T23 = _mm_sad_epu8(T03, T13); \
    sum0 = _mm_add_epi16(sum0, T20); \
    sum0 = _mm_add_epi16(sum0, T21); \
    sum0 = _mm_add_epi16(sum0, T22); \
    sum0 = _mm_add_epi16(sum0, T23)

    PROCESS_12x4(0);
    PROCESS_12x4(4);
    PROCESS_12x4(8);
    PROCESS_12x4(12);

    sum1 = _mm_shuffle_epi32(sum0, 2);
    sum0 = _mm_add_epi32(sum0, sum1);

    return _mm_cvtsi128_si32(sum0);
}

template<int ly>
// always instanced for 32 rows
int sad_24(pixel *fenc, intptr_t fencstride, pixel *fref, intptr_t frefstride)
{
    __m128i sum0 = _mm_setzero_si128();
    __m128i sum1 = _mm_setzero_si128();
    __m128i T00, T01, T02, T03;
    __m128i T10, T11, T12, T13;
    __m128i T20, T21, T22, T23;

#define PROCESS_24x4(BASE) \
    T00 = _mm_load_si128((__m128i*)(fenc + (BASE + 0) * fencstride)); \
    T01 = _mm_load_si128((__m128i*)(fenc + (BASE + 1) * fencstride)); \
    T02 = _mm_load_si128((__m128i*)(fenc + (BASE + 2) * fencstride)); \
    T03 = _mm_load_si128((__m128i*)(fenc + (BASE + 3) * fencstride)); \
    T10 = _mm_loadu_si128((__m128i*)(fref + (BASE + 0) * frefstride)); \
    T11 = _mm_loadu_si128((__m128i*)(fref + (BASE + 1) * frefstride)); \
    T12 = _mm_loadu_si128((__m128i*)(fref + (BASE + 2) * frefstride)); \
    T13 = _mm_loadu_si128((__m128i*)(fref + (BASE + 3) * frefstride)); \
    T20 = _mm_sad_epu8(T00, T10); \
    T21 = _mm_sad_epu8(T01, T11); \
    T22 = _mm_sad_epu8(T02, T12); \
    T23 = _mm_sad_epu8(T03, T13); \
    sum0 = _mm_add_epi32(sum0, T20); \
    sum0 = _mm_add_epi32(sum0, T21); \
    sum0 = _mm_add_epi32(sum0, T22); \
    sum0 = _mm_add_epi32(sum0, T23); \
    T00 = _mm_loadl_epi64((__m128i*)(fenc + 16 + ((BASE + 0) * fencstride))); \
    T01 = _mm_loadl_epi64((__m128i*)(fenc + 16 + ((BASE + 1) * fencstride))); \
    T01 = _mm_unpacklo_epi64(T00, T01); \
    T02 = _mm_loadl_epi64((__m128i*)(fenc + 16 + ((BASE + 2) * fencstride))); \
    T03 = _mm_loadl_epi64((__m128i*)(fenc + 16 + ((BASE + 3) * fencstride))); \
    T03 = _mm_unpacklo_epi64(T02, T03); \
    T10 = _mm_loadl_epi64((__m128i*)(fref + 16 + ((BASE + 0) * frefstride))); \
    T11 = _mm_loadl_epi64((__m128i*)(fref + 16 + ((BASE + 1) * frefstride))); \
    T11 = _mm_unpacklo_epi64(T10, T11); \
    T12 = _mm_loadl_epi64((__m128i*)(fref + 16 + ((BASE + 2) * frefstride))); \
    T13 = _mm_loadl_epi64((__m128i*)(fref + 16 + ((BASE + 3) * frefstride))); \
    T13 = _mm_unpacklo_epi64(T12, T13); \
    T20 = _mm_setzero_si128(); \
    T21 = _mm_setzero_si128(); \
    T20 = _mm_sad_epu8(T01, T11); \
    T21 = _mm_sad_epu8(T03, T13); \
    sum0 = _mm_add_epi32(sum0, T20); \
    sum0 = _mm_add_epi32(sum0, T21);

    for (int i = 0; i < ly; i += 8)
    {
        PROCESS_24x4(i);
        PROCESS_24x4(i + 4);
    }

    sum1 = _mm_shuffle_epi32(sum0, 2);
    sum0 = _mm_add_epi32(sum0, sum1);

    return _mm_cvtsi128_si32(sum0);
}

template<int ly>
// ly will be 8, 16, 24, or 32
int sad_32(pixel * fenc, intptr_t fencstride, pixel * fref, intptr_t frefstride)
{
    __m128i sum0 = _mm_setzero_si128();
    __m128i sum1 = _mm_setzero_si128();
    __m128i T00, T01, T02, T03;
    __m128i T10, T11, T12, T13;
    __m128i T20, T21, T22, T23;

#define PROCESS_32x4(BASE) \
    T00 = _mm_load_si128((__m128i*)(fenc + (BASE + 0) * fencstride)); \
    T01 = _mm_load_si128((__m128i*)(fenc + (BASE + 1) * fencstride)); \
    T02 = _mm_load_si128((__m128i*)(fenc + (BASE + 2) * fencstride)); \
    T03 = _mm_load_si128((__m128i*)(fenc + (BASE + 3) * fencstride)); \
    T10 = _mm_loadu_si128((__m128i*)(fref + (BASE + 0) * frefstride)); \
    T11 = _mm_loadu_si128((__m128i*)(fref + (BASE + 1) * frefstride)); \
    T12 = _mm_loadu_si128((__m128i*)(fref + (BASE + 2) * frefstride)); \
    T13 = _mm_loadu_si128((__m128i*)(fref + (BASE + 3) * frefstride)); \
    T20 = _mm_sad_epu8(T00, T10); \
    T21 = _mm_sad_epu8(T01, T11); \
    T22 = _mm_sad_epu8(T02, T12); \
    T23 = _mm_sad_epu8(T03, T13); \
    sum0 = _mm_add_epi32(sum0, T20); \
    sum0 = _mm_add_epi32(sum0, T21); \
    sum0 = _mm_add_epi32(sum0, T22); \
    sum0 = _mm_add_epi32(sum0, T23); \
    T00 = _mm_load_si128((__m128i*)(fenc + 16 + (BASE + 0) * fencstride)); \
    T01 = _mm_load_si128((__m128i*)(fenc + 16 + (BASE + 1) * fencstride)); \
    T02 = _mm_load_si128((__m128i*)(fenc + 16 + (BASE + 2) * fencstride)); \
    T03 = _mm_load_si128((__m128i*)(fenc + 16 + (BASE + 3) * fencstride)); \
    T10 = _mm_loadu_si128((__m128i*)(fref + 16 + (BASE + 0) * frefstride)); \
    T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (BASE + 1) * frefstride)); \
    T12 = _mm_loadu_si128((__m128i*)(fref + 16 + (BASE + 2) * frefstride)); \
    T13 = _mm_loadu_si128((__m128i*)(fref + 16 + (BASE + 3) * frefstride)); \
    T20 = _mm_sad_epu8(T00, T10); \
    T21 = _mm_sad_epu8(T01, T11); \
    T22 = _mm_sad_epu8(T02, T12); \
    T23 = _mm_sad_epu8(T03, T13); \
    sum0 = _mm_add_epi32(sum0, T20); \
    sum0 = _mm_add_epi32(sum0, T21); \
    sum0 = _mm_add_epi32(sum0, T22); \
    sum0 = _mm_add_epi32(sum0, T23);

    for (int i = 0; i < ly; i += 8)
    {
        PROCESS_32x4(i);
        PROCESS_32x4(i + 4);
    }

    sum1 = _mm_shuffle_epi32(sum0, 2);
    sum0 = _mm_add_epi32(sum0, sum1);

    return _mm_cvtsi128_si32(sum0);
}

template<int ly>
int sad_48(pixel * fenc, intptr_t fencstride, pixel * fref, intptr_t frefstride)
{
    __m128i sum0 = _mm_setzero_si128();
    __m128i sum1 = _mm_setzero_si128();

    /* for ly = 64 */
    for (int i = 0; i < ly; i += 8)
    {
        __m128i T00, T01, T02;
        __m128i T10, T11, T12;
        __m128i T20, T21, T22;

        T00 = _mm_load_si128((__m128i*)(fenc + (i + 0) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 0) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 0) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (i + 0) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (i + 0) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi32(sum0, T20);
        sum0 = _mm_add_epi32(sum0, T21);
        sum0 = _mm_add_epi32(sum0, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + (i + 1) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 1) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 1) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (i + 1) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (i + 1) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi32(sum0, T20);
        sum0 = _mm_add_epi32(sum0, T21);
        sum0 = _mm_add_epi32(sum0, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + (i + 2) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 2) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 2) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (i + 2) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (i + 2) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (i + 2) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi32(sum0, T20);
        sum0 = _mm_add_epi32(sum0, T21);
        sum0 = _mm_add_epi32(sum0, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + (i + 3) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 3) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 3) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (i + 3) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (i + 3) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi32(sum0, T20);
        sum0 = _mm_add_epi32(sum0, T21);
        sum0 = _mm_add_epi32(sum0, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + (i + 4) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 4) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 4) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (i + 4) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (i + 4) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi32(sum0, T20);
        sum0 = _mm_add_epi32(sum0, T21);
        sum0 = _mm_add_epi32(sum0, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + (i + 5) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 5) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 5) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (i + 5) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (i + 5) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi32(sum0, T20);
        sum0 = _mm_add_epi32(sum0, T21);
        sum0 = _mm_add_epi32(sum0, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + (i + 6) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 6) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 6) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (i + 6) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (i + 6) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (i + 6) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi32(sum0, T20);
        sum0 = _mm_add_epi32(sum0, T21);
        sum0 = _mm_add_epi32(sum0, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + (i + 7) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 7) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 7) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (i + 7) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (i + 7) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi32(sum0, T20);
        sum0 = _mm_add_epi32(sum0, T21);
        sum0 = _mm_add_epi32(sum0, T22);
    }

    sum1 = _mm_shuffle_epi32(sum0, 2);
    sum0 = _mm_add_epi32(sum0, sum1);
    return _mm_cvtsi128_si32(sum0);
}

template<int ly>
// ly will be 16, 32, 48, or 64
int sad_64(pixel * fenc, intptr_t fencstride, pixel * fref, intptr_t frefstride)
{
    __m128i sum0 = _mm_setzero_si128();
    __m128i sum1 = _mm_setzero_si128();

    for (int i = 0; i < ly; i += 8)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;

        T00 = _mm_load_si128((__m128i*)(fenc + (i + 0) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 0) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 0) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (i + 0) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (i + 0) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (i + 0) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48 + (i + 0) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi32(sum0, T20);
        sum0 = _mm_add_epi32(sum0, T21);
        sum0 = _mm_add_epi32(sum0, T22);
        sum0 = _mm_add_epi32(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (i + 1) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 1) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 1) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (i + 1) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (i + 1) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (i + 1) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48 + (i + 1) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi32(sum0, T20);
        sum0 = _mm_add_epi32(sum0, T21);
        sum0 = _mm_add_epi32(sum0, T22);
        sum0 = _mm_add_epi32(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (i + 2) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 2) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 2) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (i + 2) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (i + 2) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (i + 2) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48 + (i + 2) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi32(sum0, T20);
        sum0 = _mm_add_epi32(sum0, T21);
        sum0 = _mm_add_epi32(sum0, T22);
        sum0 = _mm_add_epi32(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (i + 3) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 3) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 3) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (i + 3) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (i + 3) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (i + 3) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (i + 3) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi32(sum0, T20);
        sum0 = _mm_add_epi32(sum0, T21);
        sum0 = _mm_add_epi32(sum0, T22);
        sum0 = _mm_add_epi32(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (i + 4) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 4) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 4) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (i + 4) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (i + 4) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (i + 4) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48 + (i + 4) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi32(sum0, T20);
        sum0 = _mm_add_epi32(sum0, T21);
        sum0 = _mm_add_epi32(sum0, T22);
        sum0 = _mm_add_epi32(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (i + 5) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 5) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 5) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (i + 5) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (i + 5) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (i + 5) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48 + (i + 5) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi32(sum0, T20);
        sum0 = _mm_add_epi32(sum0, T21);
        sum0 = _mm_add_epi32(sum0, T22);
        sum0 = _mm_add_epi32(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (i + 6) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 6) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 6) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (i + 6) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (i + 6) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (i + 6) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48 + (i + 6) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi32(sum0, T20);
        sum0 = _mm_add_epi32(sum0, T21);
        sum0 = _mm_add_epi32(sum0, T22);
        sum0 = _mm_add_epi32(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (i + 7) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 7) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 7) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (i + 7) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (i + 7) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (i + 7) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (i + 7) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi32(sum0, T20);
        sum0 = _mm_add_epi32(sum0, T21);
        sum0 = _mm_add_epi32(sum0, T22);
        sum0 = _mm_add_epi32(sum0, T23);
    }

    sum1 = _mm_shuffle_epi32(sum0, 2);
    sum0 = _mm_add_epi32(sum0, sum1);
    return _mm_cvtsi128_si32(sum0);
}

template<int ly>
void sad_x3_12(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, intptr_t frefstride, int32_t *res)
{
    assert(ly == 16);
    res[0] = res[1] = res[2] = 0;
    __m128i T00, T01, T02, T03;
    __m128i T10, T11, T12, T13;
    __m128i T20, T21, T22, T23;
    __m128i sum0, sum1;

#ifndef MASK
#define MASK _mm_set_epi32(0x0, 0xffffffff, 0xffffffff, 0xffffffff)
#endif

#define PROCESS_12x4x3(BASE) \
    T00 = _mm_load_si128((__m128i*)(fenc + (BASE + 0) * FENC_STRIDE)); \
    T00 = _mm_and_si128(T00, MASK); \
    T01 = _mm_load_si128((__m128i*)(fenc + (BASE + 1) * FENC_STRIDE)); \
    T01 = _mm_and_si128(T01, MASK); \
    T02 = _mm_load_si128((__m128i*)(fenc + (BASE + 2) * FENC_STRIDE)); \
    T02 = _mm_and_si128(T02, MASK); \
    T03 = _mm_load_si128((__m128i*)(fenc + (BASE + 3) * FENC_STRIDE)); \
    T03 = _mm_and_si128(T03, MASK); \
    T10 = _mm_loadu_si128((__m128i*)(fref1 + (BASE + 0) * frefstride)); \
    T10 = _mm_and_si128(T10, MASK); \
    T11 = _mm_loadu_si128((__m128i*)(fref1 + (BASE + 1) * frefstride)); \
    T11 = _mm_and_si128(T11, MASK); \
    T12 = _mm_loadu_si128((__m128i*)(fref1 + (BASE + 2) * frefstride)); \
    T12 = _mm_and_si128(T12, MASK); \
    T13 = _mm_loadu_si128((__m128i*)(fref1 + (BASE + 3) * frefstride)); \
    T13 = _mm_and_si128(T13, MASK); \
    T20 = _mm_sad_epu8(T00, T10); \
    T21 = _mm_sad_epu8(T01, T11); \
    T22 = _mm_sad_epu8(T02, T12); \
    T23 = _mm_sad_epu8(T03, T13); \
    T20 = _mm_add_epi16(T20, T21); \
    T22 = _mm_add_epi16(T22, T23); \
    sum0 = _mm_add_epi16(T20, T22); \
    sum1 = _mm_shuffle_epi32(sum0, 2); \
    sum0 = _mm_add_epi32(sum0, sum1); \
    res[0] += _mm_cvtsi128_si32(sum0); \
    T10 = _mm_loadu_si128((__m128i*)(fref2 + (BASE + 0) * frefstride)); \
    T10 = _mm_and_si128(T10, MASK); \
    T11 = _mm_loadu_si128((__m128i*)(fref2 + (BASE + 1) * frefstride)); \
    T11 = _mm_and_si128(T11, MASK); \
    T12 = _mm_loadu_si128((__m128i*)(fref2 + (BASE + 2) * frefstride)); \
    T12 = _mm_and_si128(T12, MASK); \
    T13 = _mm_loadu_si128((__m128i*)(fref2 + (BASE + 3) * frefstride)); \
    T13 = _mm_and_si128(T13, MASK); \
    T20 = _mm_sad_epu8(T00, T10); \
    T21 = _mm_sad_epu8(T01, T11); \
    T22 = _mm_sad_epu8(T02, T12); \
    T23 = _mm_sad_epu8(T03, T13); \
    T20 = _mm_add_epi16(T20, T21); \
    T22 = _mm_add_epi16(T22, T23); \
    sum0 = _mm_add_epi16(T20, T22); \
    sum1 = _mm_shuffle_epi32(sum0, 2); \
    sum0 = _mm_add_epi32(sum0, sum1); \
    res[1] += _mm_cvtsi128_si32(sum0); \
    T10 = _mm_loadu_si128((__m128i*)(fref3 + (BASE + 0) * frefstride)); \
    T10 = _mm_and_si128(T10, MASK); \
    T11 = _mm_loadu_si128((__m128i*)(fref3 + (BASE + 1) * frefstride)); \
    T11 = _mm_and_si128(T11, MASK); \
    T12 = _mm_loadu_si128((__m128i*)(fref3 + (BASE + 2) * frefstride)); \
    T12 = _mm_and_si128(T12, MASK); \
    T13 = _mm_loadu_si128((__m128i*)(fref3 + (BASE + 3) * frefstride)); \
    T13 = _mm_and_si128(T13, MASK); \
    T20 = _mm_sad_epu8(T00, T10); \
    T21 = _mm_sad_epu8(T01, T11); \
    T22 = _mm_sad_epu8(T02, T12); \
    T23 = _mm_sad_epu8(T03, T13); \
    T20 = _mm_add_epi16(T20, T21); \
    T22 = _mm_add_epi16(T22, T23); \
    sum0 = _mm_add_epi16(T20, T22); \
    sum1 = _mm_shuffle_epi32(sum0, 2); \
    sum0 = _mm_add_epi32(sum0, sum1); \
    res[2] += _mm_cvtsi128_si32(sum0)

    PROCESS_12x4x3(0);
    PROCESS_12x4x3(4);
    PROCESS_12x4x3(8);
    PROCESS_12x4x3(12);
}

template<int ly>
void sad_x3_24(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, intptr_t frefstride, int32_t *res)
{
    res[0] = res[1] = res[2] = 0;
    __m128i T00, T01, T02, T03, T04, T05;
    __m128i T10, T11, T12, T13, T14, T15;
    __m128i T20, T21, T22, T23;
    __m128i T30, T31;
    __m128i sum0, sum1;

#define PROCESS_24x4x3(BASE) \
    T00 = _mm_load_si128((__m128i*)(fenc + (BASE)*FENC_STRIDE)); \
    T01 = _mm_load_si128((__m128i*)(fenc + (BASE + 1) * FENC_STRIDE)); \
    T02 = _mm_load_si128((__m128i*)(fenc + (BASE + 2) * FENC_STRIDE)); \
    T03 = _mm_load_si128((__m128i*)(fenc + (BASE + 3) * FENC_STRIDE)); \
    T10 = _mm_loadl_epi64((__m128i*)(fenc + (BASE)*FENC_STRIDE + 16)); \
    T11 = _mm_loadl_epi64((__m128i*)(fenc + (BASE + 1) * FENC_STRIDE + 16)); \
    T04 = _mm_unpacklo_epi64(T10, T11); \
    T12 = _mm_loadl_epi64((__m128i*)(fenc + (BASE + 2) * FENC_STRIDE + 16)); \
    T13 = _mm_loadl_epi64((__m128i*)(fenc + (BASE + 3) * FENC_STRIDE + 16)); \
    T05 = _mm_unpacklo_epi64(T12, T13); \
    T10 = _mm_loadu_si128((__m128i*)(fref1 + (BASE)*frefstride)); \
    T11 = _mm_loadu_si128((__m128i*)(fref1 + (BASE + 1) * frefstride)); \
    T12 = _mm_loadu_si128((__m128i*)(fref1 + (BASE + 2) * frefstride)); \
    T13 = _mm_loadu_si128((__m128i*)(fref1 + (BASE + 3) * frefstride)); \
    T20 = _mm_loadl_epi64((__m128i*)(fref1 + (BASE)*frefstride + 16)); \
    T21 = _mm_loadl_epi64((__m128i*)(fref1 + (BASE + 1) * frefstride + 16)); \
    T14 = _mm_unpacklo_epi64(T20, T21); \
    T22 = _mm_loadl_epi64((__m128i*)(fref1 + (BASE + 2) * frefstride + 16)); \
    T23 = _mm_loadl_epi64((__m128i*)(fref1 + (BASE + 3) * frefstride + 16)); \
    T15 = _mm_unpacklo_epi64(T22, T23); \
    T20 = _mm_sad_epu8(T00, T10); \
    T21 = _mm_sad_epu8(T01, T11); \
    T22 = _mm_sad_epu8(T02, T12); \
    T23 = _mm_sad_epu8(T03, T13); \
    T30 = _mm_sad_epu8(T04, T14); \
    T31 = _mm_sad_epu8(T05, T15); \
    T20 = _mm_add_epi16(T20, T21); \
    T22 = _mm_add_epi16(T22, T23); \
    sum0 = _mm_add_epi16(T20, T22); \
    sum1 = _mm_add_epi16(T30, T31); \
    sum0 = _mm_add_epi16(sum0, sum1); \
    sum1 = _mm_shuffle_epi32(sum0, 2); \
    sum0 = _mm_add_epi32(sum0, sum1); \
    res[0] += _mm_cvtsi128_si32(sum0); \
    T10 = _mm_loadu_si128((__m128i*)(fref2 + (BASE)*frefstride)); \
    T11 = _mm_loadu_si128((__m128i*)(fref2 + (BASE + 1) * frefstride)); \
    T12 = _mm_loadu_si128((__m128i*)(fref2 + (BASE + 2) * frefstride)); \
    T13 = _mm_loadu_si128((__m128i*)(fref2 + (BASE + 3) * frefstride)); \
    T20 = _mm_loadl_epi64((__m128i*)(fref2 + (BASE)*frefstride + 16)); \
    T21 = _mm_loadl_epi64((__m128i*)(fref2 + (BASE + 1) * frefstride + 16)); \
    T14 = _mm_unpacklo_epi64(T20, T21); \
    T22 = _mm_loadl_epi64((__m128i*)(fref2 + (BASE + 2) * frefstride + 16)); \
    T23 = _mm_loadl_epi64((__m128i*)(fref2 + (BASE + 3) * frefstride + 16)); \
    T15 = _mm_unpacklo_epi64(T22, T23); \
    T20 = _mm_sad_epu8(T00, T10); \
    T21 = _mm_sad_epu8(T01, T11); \
    T22 = _mm_sad_epu8(T02, T12); \
    T23 = _mm_sad_epu8(T03, T13); \
    T30 = _mm_sad_epu8(T04, T14); \
    T31 = _mm_sad_epu8(T05, T15); \
    T20 = _mm_add_epi16(T20, T21); \
    T22 = _mm_add_epi16(T22, T23); \
    sum0 = _mm_add_epi16(T20, T22); \
    sum1 = _mm_add_epi16(T30, T31); \
    sum0 = _mm_add_epi16(sum0, sum1); \
    sum1 = _mm_shuffle_epi32(sum0, 2); \
    sum0 = _mm_add_epi32(sum0, sum1); \
    res[1] += _mm_cvtsi128_si32(sum0); \
    T10 = _mm_loadu_si128((__m128i*)(fref3 + (BASE)*frefstride)); \
    T11 = _mm_loadu_si128((__m128i*)(fref3 + (BASE + 1) * frefstride)); \
    T12 = _mm_loadu_si128((__m128i*)(fref3 + (BASE + 2) * frefstride)); \
    T13 = _mm_loadu_si128((__m128i*)(fref3 + (BASE + 3) * frefstride)); \
    T20 = _mm_loadl_epi64((__m128i*)(fref3 + (BASE)*frefstride + 16)); \
    T21 = _mm_loadl_epi64((__m128i*)(fref3 + (BASE + 1) * frefstride + 16)); \
    T14 = _mm_unpacklo_epi64(T20, T21); \
    T22 = _mm_loadl_epi64((__m128i*)(fref3 + (BASE + 2) * frefstride + 16)); \
    T23 = _mm_loadl_epi64((__m128i*)(fref3 + (BASE + 3) * frefstride + 16)); \
    T15 = _mm_unpacklo_epi64(T22, T23); \
    T20 = _mm_sad_epu8(T00, T10); \
    T21 = _mm_sad_epu8(T01, T11); \
    T22 = _mm_sad_epu8(T02, T12); \
    T23 = _mm_sad_epu8(T03, T13); \
    T30 = _mm_sad_epu8(T04, T14); \
    T31 = _mm_sad_epu8(T05, T15); \
    T20 = _mm_add_epi16(T20, T21); \
    T22 = _mm_add_epi16(T22, T23); \
    sum0 = _mm_add_epi16(T20, T22); \
    sum1 = _mm_add_epi16(T30, T31); \
    sum0 = _mm_add_epi16(sum0, sum1); \
    sum1 = _mm_shuffle_epi32(sum0, 2); \
    sum0 = _mm_add_epi32(sum0, sum1); \
    res[2] += _mm_cvtsi128_si32(sum0);

    for (int i = 0; i < ly; i += 8)
    {
        PROCESS_24x4x3(i);
        PROCESS_24x4x3(i + 4);
    }
}

template<int ly>
// ly will be 8, 16, 24, or 32
void sad_x3_32(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, intptr_t frefstride, int32_t *res)
{
    res[0] = res[1] = res[2] = 0;
    __m128i T00, T01, T02, T03, T04, T05, T06, T07;
    __m128i T10, T11, T12, T13, T14, T15, T16, T17;
    __m128i T20, T21, T22, T23, T24, T25, T26, T27;
    __m128i sum0, sum1;

#define PROCESS_32x4x3(BASE) \
    T00 = _mm_load_si128((__m128i*)(fenc + (BASE)*FENC_STRIDE)); \
    T01 = _mm_load_si128((__m128i*)(fenc + (BASE + 1) * FENC_STRIDE)); \
    T02 = _mm_load_si128((__m128i*)(fenc + (BASE + 2) * FENC_STRIDE)); \
    T03 = _mm_load_si128((__m128i*)(fenc + (BASE + 3) * FENC_STRIDE)); \
    T04 = _mm_load_si128((__m128i*)(fenc + (BASE)*FENC_STRIDE + 16)); \
    T05 = _mm_load_si128((__m128i*)(fenc + (BASE + 1) * FENC_STRIDE + 16)); \
    T06 = _mm_load_si128((__m128i*)(fenc + (BASE + 2) * FENC_STRIDE + 16)); \
    T07 = _mm_load_si128((__m128i*)(fenc + (BASE + 3) * FENC_STRIDE + 16)); \
    T10 = _mm_loadu_si128((__m128i*)(fref1 + (BASE)*frefstride)); \
    T11 = _mm_loadu_si128((__m128i*)(fref1 + (BASE + 1) * frefstride)); \
    T12 = _mm_loadu_si128((__m128i*)(fref1 + (BASE + 2) * frefstride)); \
    T13 = _mm_loadu_si128((__m128i*)(fref1 + (BASE + 3) * frefstride)); \
    T14 = _mm_loadu_si128((__m128i*)(fref1 + (BASE)*frefstride + 16)); \
    T15 = _mm_loadu_si128((__m128i*)(fref1 + (BASE + 1) * frefstride + 16)); \
    T16 = _mm_loadu_si128((__m128i*)(fref1 + (BASE + 2) * frefstride + 16)); \
    T17 = _mm_loadu_si128((__m128i*)(fref1 + (BASE + 3) * frefstride + 16)); \
    T20 = _mm_sad_epu8(T00, T10); \
    T21 = _mm_sad_epu8(T01, T11); \
    T22 = _mm_sad_epu8(T02, T12); \
    T23 = _mm_sad_epu8(T03, T13); \
    T24 = _mm_sad_epu8(T04, T14); \
    T25 = _mm_sad_epu8(T05, T15); \
    T26 = _mm_sad_epu8(T06, T16); \
    T27 = _mm_sad_epu8(T07, T17); \
    T20 = _mm_add_epi16(T20, T21); \
    T22 = _mm_add_epi16(T22, T23); \
    T24 = _mm_add_epi16(T24, T25); \
    T26 = _mm_add_epi16(T26, T27); \
    sum0 = _mm_add_epi16(T20, T22); \
    sum0 = _mm_add_epi16(sum0, T24); \
    sum0 = _mm_add_epi16(sum0, T26); \
    sum1 = _mm_shuffle_epi32(sum0, 2); \
    sum0 = _mm_add_epi32(sum0, sum1); \
    res[0] += _mm_cvtsi128_si32(sum0); \
    T10 = _mm_loadu_si128((__m128i*)(fref2 + (BASE)*frefstride)); \
    T11 = _mm_loadu_si128((__m128i*)(fref2 + (BASE + 1) * frefstride)); \
    T12 = _mm_loadu_si128((__m128i*)(fref2 + (BASE + 2) * frefstride)); \
    T13 = _mm_loadu_si128((__m128i*)(fref2 + (BASE + 3) * frefstride)); \
    T14 = _mm_loadu_si128((__m128i*)(fref2 + (BASE)*frefstride + 16)); \
    T15 = _mm_loadu_si128((__m128i*)(fref2 + (BASE + 1) * frefstride + 16)); \
    T16 = _mm_loadu_si128((__m128i*)(fref2 + (BASE + 2) * frefstride + 16)); \
    T17 = _mm_loadu_si128((__m128i*)(fref2 + (BASE + 3) * frefstride + 16)); \
    T20 = _mm_sad_epu8(T00, T10); \
    T21 = _mm_sad_epu8(T01, T11); \
    T22 = _mm_sad_epu8(T02, T12); \
    T23 = _mm_sad_epu8(T03, T13); \
    T24 = _mm_sad_epu8(T04, T14); \
    T25 = _mm_sad_epu8(T05, T15); \
    T26 = _mm_sad_epu8(T06, T16); \
    T27 = _mm_sad_epu8(T07, T17); \
    T20 = _mm_add_epi16(T20, T21); \
    T22 = _mm_add_epi16(T22, T23); \
    T24 = _mm_add_epi16(T24, T25); \
    T26 = _mm_add_epi16(T26, T27); \
    sum0 = _mm_add_epi16(T20, T22); \
    sum0 = _mm_add_epi16(sum0, T24); \
    sum0 = _mm_add_epi16(sum0, T26); \
    sum1 = _mm_shuffle_epi32(sum0, 2); \
    sum0 = _mm_add_epi32(sum0, sum1); \
    res[1] += _mm_cvtsi128_si32(sum0); \
    T10 = _mm_loadu_si128((__m128i*)(fref3 + (BASE)*frefstride)); \
    T11 = _mm_loadu_si128((__m128i*)(fref3 + (BASE + 1) * frefstride)); \
    T12 = _mm_loadu_si128((__m128i*)(fref3 + (BASE + 2) * frefstride)); \
    T13 = _mm_loadu_si128((__m128i*)(fref3 + (BASE + 3) * frefstride)); \
    T14 = _mm_loadu_si128((__m128i*)(fref3 + (BASE)*frefstride + 16)); \
    T15 = _mm_loadu_si128((__m128i*)(fref3 + (BASE + 1) * frefstride + 16)); \
    T16 = _mm_loadu_si128((__m128i*)(fref3 + (BASE + 2) * frefstride + 16)); \
    T17 = _mm_loadu_si128((__m128i*)(fref3 + (BASE + 3) * frefstride + 16)); \
    T20 = _mm_sad_epu8(T00, T10); \
    T21 = _mm_sad_epu8(T01, T11); \
    T22 = _mm_sad_epu8(T02, T12); \
    T23 = _mm_sad_epu8(T03, T13); \
    T24 = _mm_sad_epu8(T04, T14); \
    T25 = _mm_sad_epu8(T05, T15); \
    T26 = _mm_sad_epu8(T06, T16); \
    T27 = _mm_sad_epu8(T07, T17); \
    T20 = _mm_add_epi16(T20, T21); \
    T22 = _mm_add_epi16(T22, T23); \
    T24 = _mm_add_epi16(T24, T25); \
    T26 = _mm_add_epi16(T26, T27); \
    sum0 = _mm_add_epi16(T20, T22); \
    sum0 = _mm_add_epi16(sum0, T24); \
    sum0 = _mm_add_epi16(sum0, T26); \
    sum1 = _mm_shuffle_epi32(sum0, 2); \
    sum0 = _mm_add_epi32(sum0, sum1); \
    res[2] += _mm_cvtsi128_si32(sum0);

    for (int i = 0; i < ly; i += 8)
    {
        PROCESS_32x4x3(i);
        PROCESS_32x4x3(i + 4);
    }
}

template<int ly>
void sad_x3_48(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, intptr_t frefstride, int32_t *res)
{
    __m128i sum0 = _mm_setzero_si128();
    __m128i sum1 = _mm_setzero_si128();
    __m128i sum2 = _mm_setzero_si128();
    __m128i sum3;

    for (int i = 0; i < ly; i += 8)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;

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
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 0) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 1) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 0) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 1) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 7) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 7) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);
    }

    sum3 = _mm_shuffle_epi32(sum0, 2);
    sum0 = _mm_add_epi32(sum0, sum3);
    res[0] = _mm_cvtsi128_si32(sum0);       /*Extracting sad value for reference frame 1*/

    sum3 = _mm_shuffle_epi32(sum1, 2);
    sum1 = _mm_add_epi32(sum1, sum3);
    res[1] = _mm_cvtsi128_si32(sum1);       /*Extracting sad value for reference frame 2*/

    sum3 = _mm_shuffle_epi32(sum2, 2);
    sum2 = _mm_add_epi32(sum2, sum3);
    res[2] = _mm_cvtsi128_si32(sum2);       /*Extracting sad value for reference frame 3*/
}

template<int ly>
// ly will be 16, 32, 48, or 64
void sad_x3_64(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, intptr_t frefstride, int32_t *res)
{
    __m128i sum0 = _mm_setzero_si128();
    __m128i sum1 = _mm_setzero_si128();
    __m128i sum2 = _mm_setzero_si128();
    __m128i sum3;

    for (int i = 0; i < ly; i += 8)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;

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
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 0) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 1) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 0) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 1) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 48 + (i + 0) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 48 + (i + 1) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 48 + (i + 2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (i + 3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 7) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 7) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 48 + (i + 4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 48 + (i + 5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 48 + (i + 6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (i + 7) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);
    }

    sum3 = _mm_shuffle_epi32(sum0, 2);
    sum0 = _mm_add_epi32(sum0, sum3);
    res[0] = _mm_cvtsi128_si32(sum0);       /*Extracting sad value for reference frame 1*/

    sum3 = _mm_shuffle_epi32(sum1, 2);
    sum1 = _mm_add_epi32(sum1, sum3);
    res[1] = _mm_cvtsi128_si32(sum1);       /*Extracting sad value for reference frame 2*/

    sum3 = _mm_shuffle_epi32(sum2, 2);
    sum2 = _mm_add_epi32(sum2, sum3);
    res[2] = _mm_cvtsi128_si32(sum2);       /*Extracting sad value for reference frame 3*/
}

template<int ly>
void sad_x4_12(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, pixel *fref4, intptr_t frefstride, int32_t *res)
{
    assert(ly == 16);
    res[0] = res[1] = res[2] = res[3] = 0;
    __m128i T00, T01, T02, T03;
    __m128i T10, T11, T12, T13;
    __m128i T20, T21, T22, T23;
    __m128i sum0, sum1;

#ifndef MASK
#define MASK _mm_set_epi32(0x0, 0xffffffff, 0xffffffff, 0xffffffff)
#endif

#define PROCESS_12x4x4(BASE) \
    T00 = _mm_load_si128((__m128i*)(fenc + (BASE)*FENC_STRIDE)); \
    T00 = _mm_and_si128(T00, MASK); \
    T01 = _mm_load_si128((__m128i*)(fenc + (BASE + 1) * FENC_STRIDE)); \
    T01 = _mm_and_si128(T01, MASK); \
    T02 = _mm_load_si128((__m128i*)(fenc + (BASE + 2) * FENC_STRIDE)); \
    T02 = _mm_and_si128(T02, MASK); \
    T03 = _mm_load_si128((__m128i*)(fenc + (BASE + 3) * FENC_STRIDE)); \
    T03 = _mm_and_si128(T03, MASK); \
    T10 = _mm_loadu_si128((__m128i*)(fref1 + (BASE + 0) * frefstride)); \
    T10 = _mm_and_si128(T10, MASK); \
    T11 = _mm_loadu_si128((__m128i*)(fref1 + (BASE + 1) * frefstride)); \
    T11 = _mm_and_si128(T11, MASK); \
    T12 = _mm_loadu_si128((__m128i*)(fref1 + (BASE + 2) * frefstride)); \
    T12 = _mm_and_si128(T12, MASK); \
    T13 = _mm_loadu_si128((__m128i*)(fref1 + (BASE + 3) * frefstride)); \
    T13 = _mm_and_si128(T13, MASK); \
    T20 = _mm_sad_epu8(T00, T10); \
    T21 = _mm_sad_epu8(T01, T11); \
    T22 = _mm_sad_epu8(T02, T12); \
    T23 = _mm_sad_epu8(T03, T13); \
    T20 = _mm_add_epi16(T20, T21); \
    T22 = _mm_add_epi16(T22, T23); \
    sum0 = _mm_add_epi16(T20, T22); \
    sum1 = _mm_shuffle_epi32(sum0, 2); \
    sum0 = _mm_add_epi32(sum0, sum1); \
    res[0] += _mm_cvtsi128_si32(sum0); \
    T10 = _mm_loadu_si128((__m128i*)(fref2 + (BASE)*frefstride)); \
    T10 = _mm_and_si128(T10, MASK); \
    T11 = _mm_loadu_si128((__m128i*)(fref2 + (BASE + 1) * frefstride)); \
    T11 = _mm_and_si128(T11, MASK); \
    T12 = _mm_loadu_si128((__m128i*)(fref2 + (BASE + 2) * frefstride)); \
    T12 = _mm_and_si128(T12, MASK); \
    T13 = _mm_loadu_si128((__m128i*)(fref2 + (BASE + 3) * frefstride)); \
    T13 = _mm_and_si128(T13, MASK); \
    T20 = _mm_sad_epu8(T00, T10); \
    T21 = _mm_sad_epu8(T01, T11); \
    T22 = _mm_sad_epu8(T02, T12); \
    T23 = _mm_sad_epu8(T03, T13); \
    T20 = _mm_add_epi16(T20, T21); \
    T22 = _mm_add_epi16(T22, T23); \
    sum0 = _mm_add_epi16(T20, T22); \
    sum1 = _mm_shuffle_epi32(sum0, 2); \
    sum0 = _mm_add_epi32(sum0, sum1); \
    res[1] += _mm_cvtsi128_si32(sum0); \
    T10 = _mm_loadu_si128((__m128i*)(fref3 + (BASE)*frefstride)); \
    T10 = _mm_and_si128(T10, MASK); \
    T11 = _mm_loadu_si128((__m128i*)(fref3 + (BASE + 1) * frefstride)); \
    T11 = _mm_and_si128(T11, MASK); \
    T12 = _mm_loadu_si128((__m128i*)(fref3 + (BASE + 2) * frefstride)); \
    T12 = _mm_and_si128(T12, MASK); \
    T13 = _mm_loadu_si128((__m128i*)(fref3 + (BASE + 3) * frefstride)); \
    T13 = _mm_and_si128(T13, MASK); \
    T20 = _mm_sad_epu8(T00, T10); \
    T21 = _mm_sad_epu8(T01, T11); \
    T22 = _mm_sad_epu8(T02, T12); \
    T23 = _mm_sad_epu8(T03, T13); \
    T20 = _mm_add_epi16(T20, T21); \
    T22 = _mm_add_epi16(T22, T23); \
    sum0 = _mm_add_epi16(T20, T22); \
    sum1 = _mm_shuffle_epi32(sum0, 2); \
    sum0 = _mm_add_epi32(sum0, sum1); \
    res[2] += _mm_cvtsi128_si32(sum0); \
    T10 = _mm_loadu_si128((__m128i*)(fref4 + (BASE)*frefstride)); \
    T10 = _mm_and_si128(T10, MASK); \
    T11 = _mm_loadu_si128((__m128i*)(fref4 + (BASE + 1) * frefstride)); \
    T11 = _mm_and_si128(T11, MASK); \
    T12 = _mm_loadu_si128((__m128i*)(fref4 + (BASE + 2) * frefstride)); \
    T12 = _mm_and_si128(T12, MASK); \
    T13 = _mm_loadu_si128((__m128i*)(fref4 + (BASE + 3) * frefstride)); \
    T13 = _mm_and_si128(T13, MASK); \
    T20 = _mm_sad_epu8(T00, T10); \
    T21 = _mm_sad_epu8(T01, T11); \
    T22 = _mm_sad_epu8(T02, T12); \
    T23 = _mm_sad_epu8(T03, T13); \
    T20 = _mm_add_epi16(T20, T21); \
    T22 = _mm_add_epi16(T22, T23); \
    sum0 = _mm_add_epi16(T20, T22); \
    sum1 = _mm_shuffle_epi32(sum0, 2); \
    sum0 = _mm_add_epi32(sum0, sum1); \
    res[3] += _mm_cvtsi128_si32(sum0);

    PROCESS_12x4x4(0);
    PROCESS_12x4x4(4);
    PROCESS_12x4x4(8);
    PROCESS_12x4x4(12);
}

template<int ly>
void sad_x4_24(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, pixel *fref4, intptr_t frefstride, int32_t *res)
{
    res[0] = res[1] = res[2] = res[3] = 0;
    __m128i T00, T01, T02, T03, T04, T05;
    __m128i T10, T11, T12, T13, T14, T15;
    __m128i T20, T21, T22, T23;
    __m128i T30, T31;
    __m128i sum0, sum1;

#define PROCESS_24x4x4(BASE) \
    T00 = _mm_load_si128((__m128i*)(fenc + (BASE)*FENC_STRIDE)); \
    T01 = _mm_load_si128((__m128i*)(fenc + (BASE + 1) * FENC_STRIDE)); \
    T02 = _mm_load_si128((__m128i*)(fenc + (BASE + 2) * FENC_STRIDE)); \
    T03 = _mm_load_si128((__m128i*)(fenc + (BASE + 3) * FENC_STRIDE)); \
    T10 = _mm_loadl_epi64((__m128i*)(fenc + (BASE)*FENC_STRIDE + 16)); \
    T11 = _mm_loadl_epi64((__m128i*)(fenc + (BASE + 1) * FENC_STRIDE + 16)); \
    T04 = _mm_unpacklo_epi64(T10, T11); \
    T12 = _mm_loadl_epi64((__m128i*)(fenc + (BASE + 2) * FENC_STRIDE + 16)); \
    T13 = _mm_loadl_epi64((__m128i*)(fenc + (BASE + 3) * FENC_STRIDE + 16)); \
    T05 = _mm_unpacklo_epi64(T12, T13); \
    T10 = _mm_loadu_si128((__m128i*)(fref1 + (BASE)*frefstride)); \
    T11 = _mm_loadu_si128((__m128i*)(fref1 + (BASE + 1) * frefstride)); \
    T12 = _mm_loadu_si128((__m128i*)(fref1 + (BASE + 2) * frefstride)); \
    T13 = _mm_loadu_si128((__m128i*)(fref1 + (BASE + 3) * frefstride)); \
    T20 = _mm_loadl_epi64((__m128i*)(fref1 + (BASE)*frefstride + 16)); \
    T21 = _mm_loadl_epi64((__m128i*)(fref1 + (BASE + 1) * frefstride + 16)); \
    T14 = _mm_unpacklo_epi64(T20, T21); \
    T22 = _mm_loadl_epi64((__m128i*)(fref1 + (BASE + 2) * frefstride + 16)); \
    T23 = _mm_loadl_epi64((__m128i*)(fref1 + (BASE + 3) * frefstride + 16)); \
    T15 = _mm_unpacklo_epi64(T22, T23); \
    T20 = _mm_sad_epu8(T00, T10); \
    T21 = _mm_sad_epu8(T01, T11); \
    T22 = _mm_sad_epu8(T02, T12); \
    T23 = _mm_sad_epu8(T03, T13); \
    T30 = _mm_sad_epu8(T04, T14); \
    T31 = _mm_sad_epu8(T05, T15); \
    T20 = _mm_add_epi16(T20, T21); \
    T22 = _mm_add_epi16(T22, T23); \
    sum0 = _mm_add_epi16(T20, T22); \
    sum1 = _mm_add_epi16(T30, T31); \
    sum0 = _mm_add_epi16(sum0, sum1); \
    sum1 = _mm_shuffle_epi32(sum0, 2); \
    sum0 = _mm_add_epi32(sum0, sum1); \
    res[0] += _mm_cvtsi128_si32(sum0); \
    T10 = _mm_loadu_si128((__m128i*)(fref2 + (BASE)*frefstride)); \
    T11 = _mm_loadu_si128((__m128i*)(fref2 + (BASE + 1) * frefstride)); \
    T12 = _mm_loadu_si128((__m128i*)(fref2 + (BASE + 2) * frefstride)); \
    T13 = _mm_loadu_si128((__m128i*)(fref2 + (BASE + 3) * frefstride)); \
    T20 = _mm_loadl_epi64((__m128i*)(fref2 + (BASE)*frefstride + 16)); \
    T21 = _mm_loadl_epi64((__m128i*)(fref2 + (BASE + 1) * frefstride + 16)); \
    T14 = _mm_unpacklo_epi64(T20, T21); \
    T22 = _mm_loadl_epi64((__m128i*)(fref2 + (BASE + 2) * frefstride + 16)); \
    T23 = _mm_loadl_epi64((__m128i*)(fref2 + (BASE + 3) * frefstride + 16)); \
    T15 = _mm_unpacklo_epi64(T22, T23); \
    T20 = _mm_sad_epu8(T00, T10); \
    T21 = _mm_sad_epu8(T01, T11); \
    T22 = _mm_sad_epu8(T02, T12); \
    T23 = _mm_sad_epu8(T03, T13); \
    T30 = _mm_sad_epu8(T04, T14); \
    T31 = _mm_sad_epu8(T05, T15); \
    T20 = _mm_add_epi16(T20, T21); \
    T22 = _mm_add_epi16(T22, T23); \
    sum0 = _mm_add_epi16(T20, T22); \
    sum1 = _mm_add_epi16(T30, T31); \
    sum0 = _mm_add_epi16(sum0, sum1); \
    sum1 = _mm_shuffle_epi32(sum0, 2); \
    sum0 = _mm_add_epi32(sum0, sum1); \
    res[1] += _mm_cvtsi128_si32(sum0); \
    T10 = _mm_loadu_si128((__m128i*)(fref3 + (BASE)*frefstride)); \
    T11 = _mm_loadu_si128((__m128i*)(fref3 + (BASE + 1) * frefstride)); \
    T12 = _mm_loadu_si128((__m128i*)(fref3 + (BASE + 2) * frefstride)); \
    T13 = _mm_loadu_si128((__m128i*)(fref3 + (BASE + 3) * frefstride)); \
    T20 = _mm_loadl_epi64((__m128i*)(fref3 + (BASE)*frefstride + 16)); \
    T21 = _mm_loadl_epi64((__m128i*)(fref3 + (BASE + 1) * frefstride + 16)); \
    T14 = _mm_unpacklo_epi64(T20, T21); \
    T22 = _mm_loadl_epi64((__m128i*)(fref3 + (BASE + 2) * frefstride + 16)); \
    T23 = _mm_loadl_epi64((__m128i*)(fref3 + (BASE + 3) * frefstride + 16)); \
    T15 = _mm_unpacklo_epi64(T22, T23); \
    T20 = _mm_sad_epu8(T00, T10); \
    T21 = _mm_sad_epu8(T01, T11); \
    T22 = _mm_sad_epu8(T02, T12); \
    T23 = _mm_sad_epu8(T03, T13); \
    T30 = _mm_sad_epu8(T04, T14); \
    T31 = _mm_sad_epu8(T05, T15); \
    T20 = _mm_add_epi16(T20, T21); \
    T22 = _mm_add_epi16(T22, T23); \
    sum0 = _mm_add_epi16(T20, T22); \
    sum1 = _mm_add_epi16(T30, T31); \
    sum0 = _mm_add_epi16(sum0, sum1); \
    sum1 = _mm_shuffle_epi32(sum0, 2); \
    sum0 = _mm_add_epi32(sum0, sum1); \
    res[2] += _mm_cvtsi128_si32(sum0); \
    T10 = _mm_loadu_si128((__m128i*)(fref4 + (BASE)*frefstride)); \
    T11 = _mm_loadu_si128((__m128i*)(fref4 + (BASE + 1) * frefstride)); \
    T12 = _mm_loadu_si128((__m128i*)(fref4 + (BASE + 2) * frefstride)); \
    T13 = _mm_loadu_si128((__m128i*)(fref4 + (BASE + 3) * frefstride)); \
    T20 = _mm_loadl_epi64((__m128i*)(fref4 + (BASE)*frefstride + 16)); \
    T21 = _mm_loadl_epi64((__m128i*)(fref4 + (BASE + 1) * frefstride + 16)); \
    T14 = _mm_unpacklo_epi64(T20, T21); \
    T22 = _mm_loadl_epi64((__m128i*)(fref4 + (BASE + 2) * frefstride + 16)); \
    T23 = _mm_loadl_epi64((__m128i*)(fref4 + (BASE + 3) * frefstride + 16)); \
    T15 = _mm_unpacklo_epi64(T22, T23); \
    T20 = _mm_sad_epu8(T00, T10); \
    T21 = _mm_sad_epu8(T01, T11); \
    T22 = _mm_sad_epu8(T02, T12); \
    T23 = _mm_sad_epu8(T03, T13); \
    T30 = _mm_sad_epu8(T04, T14); \
    T31 = _mm_sad_epu8(T05, T15); \
    T20 = _mm_add_epi16(T20, T21); \
    T22 = _mm_add_epi16(T22, T23); \
    sum0 = _mm_add_epi16(T20, T22); \
    sum1 = _mm_add_epi16(T30, T31); \
    sum0 = _mm_add_epi16(sum0, sum1); \
    sum1 = _mm_shuffle_epi32(sum0, 2); \
    sum0 = _mm_add_epi32(sum0, sum1); \
    res[3] += _mm_cvtsi128_si32(sum0)

    for (int i = 0; i < ly; i += 8)
    {
        PROCESS_24x4x4(i);
        PROCESS_24x4x4(i + 4);
    }
}

template<int ly>
// ly will be 8, 16, 24, or 32
void sad_x4_32(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, pixel *fref4, intptr_t frefstride, int32_t *res)
{
    res[0] = res[1] = res[2] = res[3] = 0;
    __m128i T00, T01, T02, T03, T04, T05, T06, T07;
    __m128i T10, T11, T12, T13, T14, T15, T16, T17;
    __m128i T20, T21, T22, T23, T24, T25, T26, T27;
    __m128i sum0, sum1;

#define PROCESS_32x4x4(BASE) \
    T00 = _mm_load_si128((__m128i*)(fenc + (BASE)*FENC_STRIDE)); \
    T01 = _mm_load_si128((__m128i*)(fenc + (BASE + 1) * FENC_STRIDE)); \
    T02 = _mm_load_si128((__m128i*)(fenc + (BASE + 2) * FENC_STRIDE)); \
    T03 = _mm_load_si128((__m128i*)(fenc + (BASE + 3) * FENC_STRIDE)); \
    T04 = _mm_load_si128((__m128i*)(fenc + (BASE)*FENC_STRIDE + 16)); \
    T05 = _mm_load_si128((__m128i*)(fenc + (BASE + 1) * FENC_STRIDE + 16)); \
    T06 = _mm_load_si128((__m128i*)(fenc + (BASE + 2) * FENC_STRIDE + 16)); \
    T07 = _mm_load_si128((__m128i*)(fenc + (BASE + 3) * FENC_STRIDE + 16)); \
    T10 = _mm_loadu_si128((__m128i*)(fref1 + (BASE)*frefstride)); \
    T11 = _mm_loadu_si128((__m128i*)(fref1 + (BASE + 1) * frefstride)); \
    T12 = _mm_loadu_si128((__m128i*)(fref1 + (BASE + 2) * frefstride)); \
    T13 = _mm_loadu_si128((__m128i*)(fref1 + (BASE + 3) * frefstride)); \
    T14 = _mm_loadu_si128((__m128i*)(fref1 + (BASE)*frefstride + 16)); \
    T15 = _mm_loadu_si128((__m128i*)(fref1 + (BASE + 1) * frefstride + 16)); \
    T16 = _mm_loadu_si128((__m128i*)(fref1 + (BASE + 2) * frefstride + 16)); \
    T17 = _mm_loadu_si128((__m128i*)(fref1 + (BASE + 3) * frefstride + 16)); \
    T20 = _mm_sad_epu8(T00, T10); \
    T21 = _mm_sad_epu8(T01, T11); \
    T22 = _mm_sad_epu8(T02, T12); \
    T23 = _mm_sad_epu8(T03, T13); \
    T24 = _mm_sad_epu8(T04, T14); \
    T25 = _mm_sad_epu8(T05, T15); \
    T26 = _mm_sad_epu8(T06, T16); \
    T27 = _mm_sad_epu8(T07, T17); \
    T20 = _mm_add_epi16(T20, T21); \
    T22 = _mm_add_epi16(T22, T23); \
    T24 = _mm_add_epi16(T24, T25); \
    T26 = _mm_add_epi16(T26, T27); \
    sum0 = _mm_add_epi16(T20, T22); \
    sum0 = _mm_add_epi16(sum0, T24); \
    sum0 = _mm_add_epi16(sum0, T26); \
    sum1 = _mm_shuffle_epi32(sum0, 2); \
    sum0 = _mm_add_epi32(sum0, sum1); \
    res[0] += _mm_cvtsi128_si32(sum0); \
    T10 = _mm_loadu_si128((__m128i*)(fref2 + (BASE)*frefstride)); \
    T11 = _mm_loadu_si128((__m128i*)(fref2 + (BASE + 1) * frefstride)); \
    T12 = _mm_loadu_si128((__m128i*)(fref2 + (BASE + 2) * frefstride)); \
    T13 = _mm_loadu_si128((__m128i*)(fref2 + (BASE + 3) * frefstride)); \
    T14 = _mm_loadu_si128((__m128i*)(fref2 + (BASE)*frefstride + 16)); \
    T15 = _mm_loadu_si128((__m128i*)(fref2 + (BASE + 1) * frefstride + 16)); \
    T16 = _mm_loadu_si128((__m128i*)(fref2 + (BASE + 2) * frefstride + 16)); \
    T17 = _mm_loadu_si128((__m128i*)(fref2 + (BASE + 3) * frefstride + 16)); \
    T20 = _mm_sad_epu8(T00, T10); \
    T21 = _mm_sad_epu8(T01, T11); \
    T22 = _mm_sad_epu8(T02, T12); \
    T23 = _mm_sad_epu8(T03, T13); \
    T24 = _mm_sad_epu8(T04, T14); \
    T25 = _mm_sad_epu8(T05, T15); \
    T26 = _mm_sad_epu8(T06, T16); \
    T27 = _mm_sad_epu8(T07, T17); \
    T20 = _mm_add_epi16(T20, T21); \
    T22 = _mm_add_epi16(T22, T23); \
    T24 = _mm_add_epi16(T24, T25); \
    T26 = _mm_add_epi16(T26, T27); \
    sum0 = _mm_add_epi16(T20, T22); \
    sum0 = _mm_add_epi16(sum0, T24); \
    sum0 = _mm_add_epi16(sum0, T26); \
    sum1 = _mm_shuffle_epi32(sum0, 2); \
    sum0 = _mm_add_epi32(sum0, sum1); \
    res[1] += _mm_cvtsi128_si32(sum0); \
    T10 = _mm_loadu_si128((__m128i*)(fref3 + (BASE)*frefstride)); \
    T11 = _mm_loadu_si128((__m128i*)(fref3 + (BASE + 1) * frefstride)); \
    T12 = _mm_loadu_si128((__m128i*)(fref3 + (BASE + 2) * frefstride)); \
    T13 = _mm_loadu_si128((__m128i*)(fref3 + (BASE + 3) * frefstride)); \
    T14 = _mm_loadu_si128((__m128i*)(fref3 + (BASE)*frefstride + 16)); \
    T15 = _mm_loadu_si128((__m128i*)(fref3 + (BASE + 1) * frefstride + 16)); \
    T16 = _mm_loadu_si128((__m128i*)(fref3 + (BASE + 2) * frefstride + 16)); \
    T17 = _mm_loadu_si128((__m128i*)(fref3 + (BASE + 3) * frefstride + 16)); \
    T20 = _mm_sad_epu8(T00, T10); \
    T21 = _mm_sad_epu8(T01, T11); \
    T22 = _mm_sad_epu8(T02, T12); \
    T23 = _mm_sad_epu8(T03, T13); \
    T24 = _mm_sad_epu8(T04, T14); \
    T25 = _mm_sad_epu8(T05, T15); \
    T26 = _mm_sad_epu8(T06, T16); \
    T27 = _mm_sad_epu8(T07, T17); \
    T20 = _mm_add_epi16(T20, T21); \
    T22 = _mm_add_epi16(T22, T23); \
    T24 = _mm_add_epi16(T24, T25); \
    T26 = _mm_add_epi16(T26, T27); \
    sum0 = _mm_add_epi16(T20, T22); \
    sum0 = _mm_add_epi16(sum0, T24); \
    sum0 = _mm_add_epi16(sum0, T26); \
    sum1 = _mm_shuffle_epi32(sum0, 2); \
    sum0 = _mm_add_epi32(sum0, sum1); \
    res[2] += _mm_cvtsi128_si32(sum0); \
    T10 = _mm_loadu_si128((__m128i*)(fref4 + (BASE)*frefstride)); \
    T11 = _mm_loadu_si128((__m128i*)(fref4 + (BASE + 1) * frefstride)); \
    T12 = _mm_loadu_si128((__m128i*)(fref4 + (BASE + 2) * frefstride)); \
    T13 = _mm_loadu_si128((__m128i*)(fref4 + (BASE + 3) * frefstride)); \
    T14 = _mm_loadu_si128((__m128i*)(fref4 + (BASE)*frefstride + 16)); \
    T15 = _mm_loadu_si128((__m128i*)(fref4 + (BASE + 1) * frefstride + 16)); \
    T16 = _mm_loadu_si128((__m128i*)(fref4 + (BASE + 2) * frefstride + 16)); \
    T17 = _mm_loadu_si128((__m128i*)(fref4 + (BASE + 3) * frefstride + 16)); \
    T20 = _mm_sad_epu8(T00, T10); \
    T21 = _mm_sad_epu8(T01, T11); \
    T22 = _mm_sad_epu8(T02, T12); \
    T23 = _mm_sad_epu8(T03, T13); \
    T24 = _mm_sad_epu8(T04, T14); \
    T25 = _mm_sad_epu8(T05, T15); \
    T26 = _mm_sad_epu8(T06, T16); \
    T27 = _mm_sad_epu8(T07, T17); \
    T20 = _mm_add_epi16(T20, T21); \
    T22 = _mm_add_epi16(T22, T23); \
    T24 = _mm_add_epi16(T24, T25); \
    T26 = _mm_add_epi16(T26, T27); \
    sum0 = _mm_add_epi16(T20, T22); \
    sum0 = _mm_add_epi16(sum0, T24); \
    sum0 = _mm_add_epi16(sum0, T26); \
    sum1 = _mm_shuffle_epi32(sum0, 2); \
    sum0 = _mm_add_epi32(sum0, sum1); \
    res[3] += _mm_cvtsi128_si32(sum0)

    for (int i = 0; i < ly; i += 8)
    {
        PROCESS_32x4x4(i);
        PROCESS_32x4x4(i + 4);
    }
}

template<int ly>
void sad_x4_48(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, pixel *fref4, intptr_t frefstride, int32_t *res)
{
    __m128i sum0 = _mm_setzero_si128();
    __m128i sum1 = _mm_setzero_si128();
    __m128i sum2 = _mm_setzero_si128();
    __m128i sum3 = _mm_setzero_si128();
    __m128i sum4;

    for (int i = 0; i < ly; i += 8)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;

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
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 0) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 1) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 0) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 1) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 7) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 7) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);
    }

    sum4 = _mm_shuffle_epi32(sum0, 2);
    sum0 = _mm_add_epi32(sum0, sum4);
    res[0] = _mm_cvtsi128_si32(sum0);     /* Extracting sad value for reference frame 1 */

    sum4 = _mm_shuffle_epi32(sum1, 2);
    sum1 = _mm_add_epi32(sum1, sum4);
    res[1] = _mm_cvtsi128_si32(sum1);     /* Extracting sad value for reference frame 2 */

    sum4 = _mm_shuffle_epi32(sum2, 2);
    sum2 = _mm_add_epi32(sum2, sum4);
    res[2] = _mm_cvtsi128_si32(sum2);     /* Extracting sad value for reference frame 3 */

    sum4 = _mm_shuffle_epi32(sum3, 2);
    sum3 = _mm_add_epi32(sum3, sum4);
    res[3] = _mm_cvtsi128_si32(sum3);     /* Extracting sad value for reference frame 4 */
}

template<int ly>
void sad_x4_64(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, pixel *fref4, intptr_t frefstride, int32_t *res)
{
    __m128i sum0 = _mm_setzero_si128();
    __m128i sum1 = _mm_setzero_si128();
    __m128i sum2 = _mm_setzero_si128();
    __m128i sum3 = _mm_setzero_si128();
    __m128i sum4;

    for (int i = 0; i < ly; i += 8)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;

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
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 0) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 1) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 0) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 1) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 48 + (i + 0) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 48 + (i + 1) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 48 + (i + 2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (i + 3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 48 + (i + 0) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 48 + (i + 1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 48 + (i + 2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 48 + (i + 3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 7) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 7) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 48 + (i + 4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 48 + (i + 5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 48 + (i + 6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (i + 7) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 48 + (i + 4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 48 + (i + 5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 48 + (i + 6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 48 + (i + 7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);
    }

    sum4 = _mm_shuffle_epi32(sum0, 2);
    sum0 = _mm_add_epi32(sum0, sum4);
    res[0] = _mm_cvtsi128_si32(sum0);     /* Extracting sad value for reference frame 1 */

    sum4 = _mm_shuffle_epi32(sum1, 2);
    sum1 = _mm_add_epi32(sum1, sum4);
    res[1] = _mm_cvtsi128_si32(sum1);     /* Extracting sad value for reference frame 2 */

    sum4 = _mm_shuffle_epi32(sum2, 2);
    sum2 = _mm_add_epi32(sum2, sum4);
    res[2] = _mm_cvtsi128_si32(sum2);     /* Extracting sad value for reference frame 3 */

    sum4 = _mm_shuffle_epi32(sum3, 2);
    sum3 = _mm_add_epi32(sum3, sum4);
    res[3] = _mm_cvtsi128_si32(sum3);     /* Extracting sad value for reference frame 4 */
}

int sse_pp_12x16(pixel* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    __m128i sum = _mm_setzero_si128();
    __m128i zero = _mm_setzero_si128();

    for (int rows = 16; rows != 0; rows--)
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

int sse_pp_24x32(pixel* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    __m128i sum = _mm_setzero_si128();
    __m128i zero = _mm_setzero_si128();

    for (int rows = 32; rows != 0; rows--)
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

int sse_pp_48x64(pixel* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    __m128i sum = _mm_setzero_si128();
    __m128i zero = _mm_setzero_si128();

    for (int rows = 64; rows != 0; rows--)
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
int sse_pp_64(pixel* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
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

void weightUnidir(int16_t *src, pixel *dst, intptr_t srcStride, intptr_t dstStride, int width, int height, int w0, int round, int shift, int offset)
{
    __m128i w00, roundoff, ofs, fs, tmpsrc, tmpdst, tmp, sign;
    int x, y;

    w00 = _mm_set1_epi32(w0);
    ofs = _mm_set1_epi32(IF_INTERNAL_OFFS);
    fs = _mm_set1_epi32(offset);
    roundoff = _mm_set1_epi32(round);
    for (y = height - 1; y >= 0; y--)
    {
        for (x = 0; x <= width - 4; x += 4)
        {
            tmpsrc = _mm_loadl_epi64((__m128i*)(src + x));
            sign = _mm_srai_epi16(tmpsrc, 15);
            tmpsrc = _mm_unpacklo_epi16(tmpsrc, sign);
            tmpdst = _mm_add_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_mullo_epi32(w00, _mm_add_epi32(tmpsrc, ofs)), roundoff), shift), fs);
            *(uint32_t*)(dst + x) = _mm_cvtsi128_si32(_mm_packus_epi16(_mm_packs_epi32(tmpdst, tmpdst), _mm_setzero_si128()));
        }

        if (width > x)
        {
            tmpsrc = _mm_loadl_epi64((__m128i*)(src + x));
            sign = _mm_srai_epi16(tmpsrc, 15);
            tmpsrc = _mm_unpacklo_epi16(tmpsrc, sign);
            tmpdst = _mm_add_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_mullo_epi32(w00, _mm_add_epi32(tmpsrc, ofs)), roundoff), shift), fs);
            tmp = _mm_packus_epi16(_mm_packs_epi32(tmpdst, tmpdst), _mm_setzero_si128());
            union
            {
                int8_t  c[16];
                int16_t s[8];
            } u;

            _mm_storeu_si128((__m128i*)u.c, tmp);
            ((int16_t*)(dst + x))[0] = u.s[0];    //to store only first 16-bit from 128-bit to memory
        }
        src += srcStride;
        dst += dstStride;
    }
}

void weightUnidirPixel(pixel *source, pixel *dest, intptr_t sourceStride, intptr_t destStride, int width, int height, int w0, int arg_round, int shift, int offset)
{
    int x, y;
    __m128i temp;
    __m128i vw0    = _mm_set1_epi32(w0);                // broadcast (32-bit integer) w0 to all elements of vw0
    __m128i ofs    = _mm_set1_epi32(offset);
    __m128i round  = _mm_set1_epi32(arg_round);
    __m128i src, dst, val;

    for (y = height - 1; y >= 0; y--)
    {
        for (x = 0; x <= width - 4; x += 4)
        {
            // The intermediate results would outgrow 16 bits because internal offset is too high
            temp = _mm_cvtsi32_si128(*(uint32_t*)(source + x));
            src = _mm_unpacklo_epi16(_mm_unpacklo_epi8(temp, _mm_setzero_si128()), _mm_setzero_si128());
            val = _mm_slli_epi32(src, (IF_INTERNAL_PREC - X265_DEPTH));
            dst = _mm_add_epi32(_mm_mullo_epi32(vw0, val), round);
            dst =  _mm_sra_epi32(dst, _mm_cvtsi32_si128(shift));
            dst = _mm_add_epi32(dst, ofs);
            *(uint32_t*)(dest + x) = _mm_cvtsi128_si32(_mm_packus_epi16(_mm_packs_epi32(dst, dst), _mm_setzero_si128()));
        }

        if (width > x)
        {
            temp = _mm_cvtsi32_si128(*(uint32_t*)(source + x));
            src = _mm_unpacklo_epi16(_mm_unpacklo_epi8(temp, _mm_setzero_si128()), _mm_setzero_si128());
            val = _mm_slli_epi32(src, (IF_INTERNAL_PREC - X265_DEPTH));
            dst = _mm_add_epi32(_mm_mullo_epi32(vw0, val), round);
            dst =  _mm_sra_epi32(dst, _mm_cvtsi32_si128(shift));
            dst = _mm_add_epi32(dst, ofs);
            temp = _mm_packus_epi16(_mm_packs_epi32(dst, dst), _mm_setzero_si128());

            union
            {
                int8_t  c[16];
                int16_t s[8];
            } u;

            _mm_storeu_si128((__m128i*)u.c, temp);
            ((int16_t*)(dest + x))[0] = u.s[0];
        }
        source += sourceStride;
        dest += destStride;
    }
}

template<int ly>
int sse_sp4(int16_t* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    __m128i sum = _mm_setzero_si128();

    for (int i = 0; i < ly; i++)
    {
        __m128i T00, T01, T02, T03;
        T00 = _mm_loadu_si128((__m128i*)(fenc));
        T01 = _mm_cvtsi32_si128(*(uint32_t*)(fref));
        __m128i sign = _mm_srai_epi16(T00, 15);
        T00 = _mm_unpacklo_epi16(T00, sign);
        T01 = _mm_unpacklo_epi8(T01, _mm_setzero_si128());
        T01 = _mm_unpacklo_epi16(T01, _mm_setzero_si128());
        T02 = _mm_sub_epi32(T00, T01);
        T03 = _mm_mullo_epi32(T02, T02);
        sum = _mm_add_epi32(sum, T03);

        fenc += strideFenc;
        fref += strideFref;
    }

    sum = _mm_hadd_epi32(sum, _mm_setzero_si128());
    sum = _mm_hadd_epi32(sum, _mm_setzero_si128());

    return _mm_cvtsi128_si32(sum);
}

#define SSE_SP8x1 \
    sign = _mm_srai_epi16(T00, 15); \
    T10 = _mm_unpacklo_epi16(T00, sign); \
    T11 = _mm_unpacklo_epi16(T02, _mm_setzero_si128()); \
    T12 = _mm_sub_epi32(T10, T11); \
    T13 = _mm_mullo_epi32(T12, T12); \
    sum0 = _mm_add_epi32(sum0, T13); \
    T10 = _mm_unpackhi_epi16(T00, sign); \
    T11 = _mm_unpackhi_epi16(T02, _mm_setzero_si128()); \
    T12 = _mm_sub_epi32(T10, T11); \
    T13 = _mm_mullo_epi32(T12, T12); \
    sum1 = _mm_add_epi32(sum1, T13)

template<int ly>
int sse_sp8(int16_t* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    __m128i sum0 = _mm_setzero_si128();
    __m128i sum1 = _mm_setzero_si128();

    for (int i = 0; i < ly; i++)
    {
        __m128i T00, T01, T02;
        __m128i T10, T11, T12, T13;
        __m128i sign;

        T00 = _mm_loadu_si128((__m128i*)(fenc));
        T01 = _mm_loadu_si128((__m128i*)(fref));
        T02 = _mm_unpacklo_epi8(T01, _mm_setzero_si128());

        SSE_SP8x1;

        fenc += strideFenc;
        fref += strideFref;
    }

    sum0 = _mm_add_epi32(sum0, sum1);

    sum0 = _mm_hadd_epi32(sum0, _mm_setzero_si128());
    sum0 = _mm_hadd_epi32(sum0, _mm_setzero_si128());

    return _mm_cvtsi128_si32(sum0);
}

template<int ly>
int sse_sp12(int16_t* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    __m128i sum0 = _mm_setzero_si128();

    for (int i = 0; i < ly; i++)
    {
        __m128i T00, T01;
        __m128i T10, T11, T12, T13;
        __m128i sign;
        T00 = _mm_loadu_si128((__m128i*)(fenc));
        T01 = _mm_loadu_si128((__m128i*)(fref));
        T01 = _mm_srli_si128(_mm_slli_si128(T01, 4), 4);    //masking last 4 8-bit integers

        sign = _mm_srai_epi16(T00, 15);
        T10 = _mm_unpacklo_epi16(T00, sign);
        T11 = _mm_unpacklo_epi8(T01, _mm_setzero_si128());
        T11 = _mm_unpacklo_epi16(T11, _mm_setzero_si128());
        T12 = _mm_sub_epi32(T10, T11);
        T13 = _mm_mullo_epi32(T12, T12);
        sum0 = _mm_add_epi32(sum0, T13);

        T10 = _mm_unpackhi_epi16(T00, sign);
        T11 = _mm_unpacklo_epi8(T01, _mm_setzero_si128());
        T11 = _mm_unpackhi_epi16(T11, _mm_setzero_si128());
        T12 = _mm_sub_epi32(T10, T11);
        T13 = _mm_mullo_epi32(T12, T12);
        sum0 = _mm_add_epi32(sum0, T13);

        T00 = _mm_loadu_si128((__m128i*)(fenc + 8));

        T10 = _mm_unpacklo_epi16(T00, sign);
        T11 = _mm_unpackhi_epi8(T01, _mm_setzero_si128());
        T11 = _mm_unpacklo_epi16(T11, _mm_setzero_si128());
        T12 = _mm_sub_epi32(T10, T11);
        T13 = _mm_mullo_epi32(T12, T12);
        sum0 = _mm_add_epi32(sum0, T13);

        fenc += strideFenc;
        fref += strideFref;
    }

    sum0 = _mm_hadd_epi32(sum0, _mm_setzero_si128());
    sum0 = _mm_hadd_epi32(sum0, _mm_setzero_si128());

    return _mm_cvtsi128_si32(sum0);
}

template<int ly>
int sse_sp16(int16_t* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    __m128i sum0 = _mm_setzero_si128();
    __m128i sum1 = _mm_setzero_si128();

    for (int i = 0; i < ly; i++)
    {
        __m128i T00, T01, T02;
        __m128i T10, T11, T12, T13;
        __m128i sign;

        T00 = _mm_loadu_si128((__m128i*)(fenc));
        T01 = _mm_loadu_si128((__m128i*)(fref));
        T02 = _mm_unpacklo_epi8(T01, _mm_setzero_si128());

        SSE_SP8x1;

        T00 = _mm_loadu_si128((__m128i*)(fenc + 8));
        T02 = _mm_unpackhi_epi8(T01, _mm_setzero_si128());

        SSE_SP8x1;

        fenc += strideFenc;
        fref += strideFref;
    }

    sum0 = _mm_add_epi32(sum0, sum1);

    sum0 = _mm_hadd_epi32(sum0, _mm_setzero_si128());
    sum0 = _mm_hadd_epi32(sum0, _mm_setzero_si128());

    return _mm_cvtsi128_si32(sum0);
}

template<int ly>
int sse_sp24(int16_t* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    __m128i sum0 = _mm_setzero_si128();
    __m128i sum1 = _mm_setzero_si128();

    for (int i = 0; i < ly; i++)
    {
        __m128i T00, T01, T02;
        __m128i T10, T11, T12, T13;
        __m128i sign;

        T00 = _mm_loadu_si128((__m128i*)(fenc));
        T01 = _mm_loadu_si128((__m128i*)(fref));
        T02 = _mm_unpacklo_epi8(T01, _mm_setzero_si128());

        SSE_SP8x1;

        T00 = _mm_loadu_si128((__m128i*)(fenc + 8));
        T02 = _mm_unpackhi_epi8(T01, _mm_setzero_si128());

        SSE_SP8x1;

        T00 = _mm_loadu_si128((__m128i*)(fenc + 16));
        T01 = _mm_loadu_si128((__m128i*)(fref + 16));
        T02 = _mm_unpacklo_epi8(T01, _mm_setzero_si128());

        SSE_SP8x1;

        fenc += strideFenc;
        fref += strideFref;
    }

    sum0 = _mm_add_epi32(sum0, sum1);

    sum0 = _mm_hadd_epi32(sum0, _mm_setzero_si128());
    sum0 = _mm_hadd_epi32(sum0, _mm_setzero_si128());

    return _mm_cvtsi128_si32(sum0);
}

template<int ly>
int sse_sp32(int16_t* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    __m128i sum0 = _mm_setzero_si128();
    __m128i sum1 = _mm_setzero_si128();

    for (int i = 0; i < ly; i++)
    {
        __m128i T00, T01, T02;
        __m128i T10, T11, T12, T13;
        __m128i sign;

        T00 = _mm_loadu_si128((__m128i*)(fenc));
        T01 = _mm_loadu_si128((__m128i*)(fref));
        T02 = _mm_unpacklo_epi8(T01, _mm_setzero_si128());

        SSE_SP8x1;

        T00 = _mm_loadu_si128((__m128i*)(fenc + 8));
        T02 = _mm_unpackhi_epi8(T01, _mm_setzero_si128());

        SSE_SP8x1;

        T00 = _mm_loadu_si128((__m128i*)(fenc + 16));
        T01 = _mm_loadu_si128((__m128i*)(fref + 16));
        T02 = _mm_unpacklo_epi8(T01, _mm_setzero_si128());

        SSE_SP8x1;

        T00 = _mm_loadu_si128((__m128i*)(fenc + 24));
        T02 = _mm_unpackhi_epi8(T01, _mm_setzero_si128());

        SSE_SP8x1;

        fenc += strideFenc;
        fref += strideFref;
    }

    sum0 = _mm_add_epi32(sum0, sum1);

    sum0 = _mm_hadd_epi32(sum0, _mm_setzero_si128());
    sum0 = _mm_hadd_epi32(sum0, _mm_setzero_si128());

    return _mm_cvtsi128_si32(sum0);
}

template<int ly>
int sse_sp48(int16_t* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    __m128i sum0 = _mm_setzero_si128();
    __m128i sum1 = _mm_setzero_si128();

    for (int i = 0; i < ly; i++)
    {
        __m128i T00, T01, T02;
        __m128i T10, T11, T12, T13;
        __m128i sign;

        T00 = _mm_loadu_si128((__m128i*)(fenc));
        T01 = _mm_loadu_si128((__m128i*)(fref));
        T02 = _mm_unpacklo_epi8(T01, _mm_setzero_si128());

        SSE_SP8x1;

        T00 = _mm_loadu_si128((__m128i*)(fenc + 8));
        T02 = _mm_unpackhi_epi8(T01, _mm_setzero_si128());

        SSE_SP8x1;

        T00 = _mm_loadu_si128((__m128i*)(fenc + 16));
        T01 = _mm_loadu_si128((__m128i*)(fref + 16));
        T02 = _mm_unpacklo_epi8(T01, _mm_setzero_si128());

        SSE_SP8x1;

        T00 = _mm_loadu_si128((__m128i*)(fenc + 24));
        T02 = _mm_unpackhi_epi8(T01, _mm_setzero_si128());

        SSE_SP8x1;

        T00 = _mm_loadu_si128((__m128i*)(fenc + 32));
        T01 = _mm_loadu_si128((__m128i*)(fref + 32));
        T02 = _mm_unpacklo_epi8(T01, _mm_setzero_si128());

        SSE_SP8x1;

        T00 = _mm_loadu_si128((__m128i*)(fenc + 40));
        T02 = _mm_unpackhi_epi8(T01, _mm_setzero_si128());

        SSE_SP8x1;

        fenc += strideFenc;
        fref += strideFref;
    }

    sum0 = _mm_add_epi32(sum0, sum1);

    sum0 = _mm_hadd_epi32(sum0, _mm_setzero_si128());
    sum0 = _mm_hadd_epi32(sum0, _mm_setzero_si128());

    return _mm_cvtsi128_si32(sum0);
}

template<int ly>
int sse_sp64(int16_t* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    __m128i sum0 = _mm_setzero_si128();
    __m128i sum1 = _mm_setzero_si128();

    for (int i = 0; i < ly; i++)
    {
        __m128i T00, T01, T02;
        __m128i T10, T11, T12, T13;
        __m128i sign;

        T00 = _mm_loadu_si128((__m128i*)(fenc));
        T01 = _mm_loadu_si128((__m128i*)(fref));
        T02 = _mm_unpacklo_epi8(T01, _mm_setzero_si128());

        SSE_SP8x1;

        T00 = _mm_loadu_si128((__m128i*)(fenc + 8));
        T02 = _mm_unpackhi_epi8(T01, _mm_setzero_si128());

        SSE_SP8x1;

        T00 = _mm_loadu_si128((__m128i*)(fenc + 16));
        T01 = _mm_loadu_si128((__m128i*)(fref + 16));
        T02 = _mm_unpacklo_epi8(T01, _mm_setzero_si128());

        SSE_SP8x1;

        T00 = _mm_loadu_si128((__m128i*)(fenc + 24));
        T02 = _mm_unpackhi_epi8(T01, _mm_setzero_si128());

        SSE_SP8x1;

        T00 = _mm_loadu_si128((__m128i*)(fenc + 32));
        T01 = _mm_loadu_si128((__m128i*)(fref + 32));
        T02 = _mm_unpacklo_epi8(T01, _mm_setzero_si128());

        SSE_SP8x1;

        T00 = _mm_loadu_si128((__m128i*)(fenc + 40));
        T02 = _mm_unpackhi_epi8(T01, _mm_setzero_si128());

        SSE_SP8x1;

        T00 = _mm_loadu_si128((__m128i*)(fenc + 48));
        T01 = _mm_loadu_si128((__m128i*)(fref + 48));
        T02 = _mm_unpacklo_epi8(T01, _mm_setzero_si128());

        SSE_SP8x1;

        T00 = _mm_loadu_si128((__m128i*)(fenc + 56));
        T02 = _mm_unpackhi_epi8(T01, _mm_setzero_si128());

        SSE_SP8x1;

        fenc += strideFenc;
        fref += strideFref;
    }

    sum0 = _mm_add_epi32(sum0, sum1);

    sum0 = _mm_hadd_epi32(sum0, _mm_setzero_si128());
    sum0 = _mm_hadd_epi32(sum0, _mm_setzero_si128());

    return _mm_cvtsi128_si32(sum0);
}

#endif /* !HIGH_BIT_DEPTH */

#define PROCESS_SSE_SS4x1(BASE) \
    m1 = _mm_loadu_si128((__m128i const*)(fenc + BASE)); \
    n1 = _mm_loadu_si128((__m128i const*)(fref + BASE)); \
    tmp1 = _mm_cvtepi16_epi32(m1); \
    tmp2 = _mm_cvtepi16_epi32(n1); \
    diff = _mm_sub_epi32(tmp1, tmp2); \
    diff = _mm_mullo_epi32(diff, diff); \
    sum = _mm_add_epi32(sum, diff)

template<int ly>
int sse_ss4(int16_t* fenc, intptr_t strideFenc, int16_t* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i diff;
    __m128i sum  = _mm_setzero_si128();
    __m128i m1, n1, tmp1, tmp2;

    for (; rows != 0; rows--)
    {
        PROCESS_SSE_SS4x1(0);

        fenc += strideFenc;
        fref += strideFref;
    }

    __m128i sum1  = _mm_hadd_epi32(sum, sum);         // horizontally add 4 elements
    return _mm_cvtsi128_si32(_mm_hadd_epi32(sum1, sum1));
}

template<int ly>
int sse_ss8(int16_t* fenc, intptr_t strideFenc, int16_t* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i sum  = _mm_setzero_si128();
    __m128i m1, n1, diff, sign, tmp1, tmp2;

    for (; rows != 0; rows--)
    {
        PROCESS_SSE_SS4x1(0);

        sign = _mm_srai_epi16(m1, 15);
        m1 = _mm_unpackhi_epi16(m1, sign);
        sign = _mm_srai_epi16(n1, 15);
        n1 = _mm_unpackhi_epi16(n1, sign);
        diff = _mm_sub_epi32(m1, n1);
        diff = _mm_mullo_epi32(diff, diff);
        sum = _mm_add_epi32(sum, diff);

        fenc += strideFenc;
        fref += strideFref;
    }

    __m128i sum1  = _mm_hadd_epi32(sum, sum);
    return _mm_cvtsi128_si32(_mm_hadd_epi32(sum1, sum1));
}

template<int ly>
int sse_ss12(int16_t* fenc, intptr_t strideFenc, int16_t* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i sum  = _mm_setzero_si128();
    __m128i m1, n1, diff, sign, tmp1, tmp2;

    for (; rows != 0; rows--)
    {
        PROCESS_SSE_SS4x1(0);

        sign = _mm_srai_epi16(m1, 15);
        m1 = _mm_unpackhi_epi16(m1, sign);
        sign = _mm_srai_epi16(n1, 15);
        n1 = _mm_unpackhi_epi16(n1, sign);
        diff = _mm_sub_epi32(m1, n1);
        diff = _mm_mullo_epi32(diff, diff);
        sum = _mm_add_epi32(sum, diff);

        PROCESS_SSE_SS4x1(8);

        fenc += strideFenc;
        fref += strideFref;
    }

    __m128i sum1  = _mm_hadd_epi32(sum, sum);
    return _mm_cvtsi128_si32(_mm_hadd_epi32(sum1, sum1));
}

template<int ly>
int sse_ss16(int16_t* fenc, intptr_t strideFenc, int16_t* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i sum  = _mm_setzero_si128();
    __m128i m1, n1, diff, sign, tmp1, tmp2;

    for (; rows != 0; rows--)
    {
        for (int i = 0; i < 16; i += 8)
        {
            PROCESS_SSE_SS4x1(i);

            sign = _mm_srai_epi16(m1, 15);
            m1 = _mm_unpackhi_epi16(m1, sign);
            sign = _mm_srai_epi16(n1, 15);
            n1 = _mm_unpackhi_epi16(n1, sign);
            diff = _mm_sub_epi32(m1, n1);
            diff = _mm_mullo_epi32(diff, diff);
            sum = _mm_add_epi32(sum, diff);
        }

        fenc += strideFenc;
        fref += strideFref;
    }

    __m128i sum1  = _mm_hadd_epi32(sum, sum);
    return _mm_cvtsi128_si32(_mm_hadd_epi32(sum1, sum1));
}

template<int ly>
int sse_ss24(int16_t* fenc, intptr_t strideFenc, int16_t* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i sum  = _mm_setzero_si128();
    __m128i m1, n1, diff, sign, tmp1, tmp2;

    for (; rows != 0; rows--)
    {
        for (int i = 0; i < 24; i += 8)
        {
            PROCESS_SSE_SS4x1(i);

            sign = _mm_srai_epi16(m1, 15);
            m1 = _mm_unpackhi_epi16(m1, sign);
            sign = _mm_srai_epi16(n1, 15);
            n1 = _mm_unpackhi_epi16(n1, sign);
            diff = _mm_sub_epi32(m1, n1);
            diff = _mm_mullo_epi32(diff, diff);
            sum = _mm_add_epi32(sum, diff);
        }

        fenc += strideFenc;
        fref += strideFref;
    }

    __m128i sum1  = _mm_hadd_epi32(sum, sum);
    return _mm_cvtsi128_si32(_mm_hadd_epi32(sum1, sum1));
}

template<int ly>
int sse_ss32(int16_t* fenc, intptr_t strideFenc, int16_t* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i sum  = _mm_setzero_si128();
    __m128i m1, n1, diff, sign, tmp1, tmp2;

    for (; rows != 0; rows--)
    {
        for (int i = 0; i < 32; i += 8)
        {
            PROCESS_SSE_SS4x1(i);

            sign = _mm_srai_epi16(m1, 15);
            m1 = _mm_unpackhi_epi16(m1, sign);
            sign = _mm_srai_epi16(n1, 15);
            n1 = _mm_unpackhi_epi16(n1, sign);
            diff = _mm_sub_epi32(m1, n1);
            diff = _mm_mullo_epi32(diff, diff);
            sum = _mm_add_epi32(sum, diff);
        }

        fenc += strideFenc;
        fref += strideFref;
    }

    __m128i sum1  = _mm_hadd_epi32(sum, sum);
    return _mm_cvtsi128_si32(_mm_hadd_epi32(sum1, sum1));
}

template<int ly>
int sse_ss48(int16_t* fenc, intptr_t strideFenc, int16_t* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i sum  = _mm_setzero_si128();
    __m128i m1, n1, diff, sign, tmp1, tmp2;

    for (; rows != 0; rows--)
    {
        for (int i = 0; i < 48; i += 8)
        {
            PROCESS_SSE_SS4x1(i);

            sign = _mm_srai_epi16(m1, 15);
            m1 = _mm_unpackhi_epi16(m1, sign);
            sign = _mm_srai_epi16(n1, 15);
            n1 = _mm_unpackhi_epi16(n1, sign);
            diff = _mm_sub_epi32(m1, n1);
            diff = _mm_mullo_epi32(diff, diff);
            sum = _mm_add_epi32(sum, diff);
        }

        fenc += strideFenc;
        fref += strideFref;
    }

    __m128i sum1  = _mm_hadd_epi32(sum, sum);
    return _mm_cvtsi128_si32(_mm_hadd_epi32(sum1, sum1));
}

template<int ly>
int sse_ss64(int16_t* fenc, intptr_t strideFenc, int16_t* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i sum  = _mm_setzero_si128();
    __m128i m1, n1, diff, sign, tmp1, tmp2;

    for (; rows != 0; rows--)
    {
        for (int i = 0; i < 64; i += 8)
        {
            PROCESS_SSE_SS4x1(i);

            sign = _mm_srai_epi16(m1, 15);
            m1 = _mm_unpackhi_epi16(m1, sign);
            sign = _mm_srai_epi16(n1, 15);
            n1 = _mm_unpackhi_epi16(n1, sign);
            diff = _mm_sub_epi32(m1, n1);
            diff = _mm_mullo_epi32(diff, diff);
            sum = _mm_add_epi32(sum, diff);
        }

        fenc += strideFenc;
        fref += strideFref;
    }

    __m128i sum1  = _mm_hadd_epi32(sum, sum);
    return _mm_cvtsi128_si32(_mm_hadd_epi32(sum1, sum1));
}
}

namespace x265 {
extern void Setup_Vec_Pixel16Primitives_sse41(EncoderPrimitives &p);

#if HIGH_BIT_DEPTH
#define SETUP_PARTITION(W, H) \
    p.sse_sp[LUMA_ ## W ## x ## H] = (pixelcmp_sp_t)sse_ss ## W<H>; \
    p.sse_ss[LUMA_ ## W ## x ## H] = sse_ss ## W < H >
#define SETUP_NONSAD(W, H) \
    p.sse_sp[LUMA_ ## W ## x ## H] = (pixelcmp_sp_t)sse_ss ## W<H>; \
    p.sse_ss[LUMA_ ## W ## x ## H] = sse_ss ## W < H >
#else
#define SETUP_PARTITION(W, H) \
    p.sad[LUMA_ ## W ## x ## H] = sad_ ## W<H>; \
    p.sad_x3[LUMA_ ## W ## x ## H] = sad_x3_ ## W<H>; \
    p.sad_x4[LUMA_ ## W ## x ## H] = sad_x4_ ## W<H>; \
    p.sse_sp[LUMA_ ## W ## x ## H] = sse_sp ## W<H>; \
    p.sse_ss[LUMA_ ## W ## x ## H] = sse_ss ## W < H >
#define SETUP_NONSAD(W, H) \
    p.sse_sp[LUMA_ ## W ## x ## H] = sse_sp ## W<H>; \
    p.sse_ss[LUMA_ ## W ## x ## H] = sse_ss ## W < H >
#endif // if HIGH_BIT_DEPTH

void Setup_Vec_PixelPrimitives_sse41(EncoderPrimitives &p)
{
    /* 2Nx2N, 2NxN, Nx2N, 4Ax3A, 4AxA, 3Ax4A, Ax4A */
    SETUP_PARTITION(64, 64);
    SETUP_PARTITION(64, 32);
    SETUP_PARTITION(32, 64);
    SETUP_PARTITION(64, 16);
    SETUP_PARTITION(64, 48);
    SETUP_NONSAD(16, 64);
    SETUP_PARTITION(48, 64);

    SETUP_PARTITION(32, 32);
    SETUP_PARTITION(32, 16);
    SETUP_NONSAD(16, 32);
    SETUP_PARTITION(32, 8);
    SETUP_PARTITION(32, 24);
    SETUP_NONSAD(8, 32);
    SETUP_PARTITION(24, 32);

    SETUP_NONSAD(16, 16); // 16x16 SAD covered by assembly
    SETUP_NONSAD(16, 8);  // 16x8 SAD covered by assembly
    SETUP_NONSAD(8, 16);  // 8x16 SAD covered by assembly
    SETUP_NONSAD(16, 4);
    SETUP_NONSAD(16, 12);
    SETUP_NONSAD(4, 16); // 4x16 SAD covered by assembly
#if !defined(__clang__)
    SETUP_PARTITION(12, 16);
#endif

    SETUP_NONSAD(8, 8); // 8x8 SAD covered by assembly
    SETUP_NONSAD(8, 4); // 8x4 SAD covered by assembly
    SETUP_NONSAD(4, 8); // 4x8 SAD covered by assembly
    /* 8x8 is too small for AMP partitions */

    SETUP_NONSAD(4, 4); // 4x4 SAD covered by assembly
    /* 4x4 is too small for any sub partitions */

#if HIGH_BIT_DEPTH
    Setup_Vec_Pixel16Primitives_sse41(p);
#else
    // These are the only SSE primitives uncovered by assembly
    p.sse_pp[LUMA_12x16] = sse_pp_12x16;
    p.sse_pp[LUMA_24x32] = sse_pp_24x32;
    p.sse_pp[LUMA_48x64] = sse_pp_48x64;
    p.sse_pp[LUMA_64x64] = sse_pp_64<64>;
    p.sse_pp[LUMA_64x32] = sse_pp_64<32>;
    p.sse_pp[LUMA_64x48] = sse_pp_64<48>;
    p.sse_pp[LUMA_64x16] = sse_pp_64<16>;

    p.weightpUniPixel = weightUnidirPixel;
    p.weightpUni = weightUnidir;
#endif /* !HIGH_BIT_DEPTH */
}
}
