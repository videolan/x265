/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Mandar Gurav <mandar@multicorewareinc.com>
 *          Mahesh Pittala <mahesh@multicorewareinc.com>
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

// Vector class versions of pixel comparison performance primitives
/* intrinsics for when pixel type is uint8_t */

#if defined(_MSC_VER)
#undef ALWAYSINLINE
#define ALWAYSINLINE  __forceinline
#pragma warning(disable: 4100) // unused formal parameters
#endif
#pragma warning(disable: 4799) // MMX warning , EMMS

template<int ly>
int sad_4(pixel * fenc, intptr_t fencstride, pixel * fref, intptr_t frefstride)
{
    Vec16uc m1, n1;

    Vec4i sum(0);
    Vec8us sad(0);
    int max_iterators = (ly >> 4) << 4;
    int row;

    for (row = 0; row < max_iterators; row += 16)
    {
        for (int i = 0; i < 16; i++)
        {
            m1.fromUint32(*(uint32_t*)fenc);
            n1.fromUint32(*(uint32_t*)fref);
            sad.addSumAbsDiff(m1, n1);

            fenc += fencstride;
            fref += frefstride;
        }

        sum += extend_low(sad);
        sad = 0;
    }

    while (row++ < ly)
    {
        m1.fromUint32(*(uint32_t*)fenc);
        n1.fromUint32(*(uint32_t*)fref);
        sad.addSumAbsDiff(m1, n1);

        fenc += fencstride;
        fref += frefstride;
    }

    sum += extend_low(sad);
    return horizontal_add(sum);
}

#ifdef X86_64
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

#else /* ifdef X86_64 */

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

#endif /* ifdef X86_64 */

template<int ly>
int sad_12(pixel * fenc, intptr_t fencstride, pixel * fref, intptr_t frefstride)
{
    Vec16uc m1, n1;

    Vec4i sum(0);
    Vec8us sad(0);
    int max_iterators = (ly >> 4) << 4;
    int row;

    for (row = 0; row < max_iterators; row += 16)
    {
        for (int i = 0; i < 16; i++)
        {
            m1.load_a(fenc);
            m1.cutoff(12);
            n1.load(fref);
            n1.cutoff(12);
            sad.addSumAbsDiff(m1, n1);

            fenc += fencstride;
            fref += frefstride;
        }

        sum += extend_low(sad) + extend_high(sad);
        sad = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(fenc);
        m1.cutoff(12);
        n1.load(fref);
        n1.cutoff(12);
        sad.addSumAbsDiff(m1, n1);

        fenc += fencstride;
        fref += frefstride;
    }

    sum += extend_low(sad) + extend_high(sad);
    return horizontal_add(sum);
}

template<int size>
ALWAYSINLINE void unrollFunc_16(pixel *fenc, intptr_t fencstride, pixel *fref, intptr_t frefstride, Vec8us& sad)
{
    unrollFunc_16<1>(fenc, fencstride, fref, frefstride, sad);
    unrollFunc_16<size - 1>(fenc + fencstride, fencstride, fref + frefstride, frefstride, sad);
}

template<>
ALWAYSINLINE void unrollFunc_16<1>(pixel *fenc, intptr_t, pixel *fref, intptr_t, Vec8us& sad)
{
    Vec16uc m1, n1;

    m1.load_a(fenc);
    n1.load(fref);
    sad.addSumAbsDiff(m1, n1);
}

template<int ly>
int sad_16(pixel * fenc, intptr_t fencstride, pixel * fref, intptr_t frefstride)
{
    Vec4i sum(0);
    Vec8us sad(0);
    int max_iterators = (ly >> 4) << 4;
    int row = 0;

    if (ly < 16)
    {
        unrollFunc_16<ly>(fenc, fencstride, fref, frefstride, sad);
        sum += extend_low(sad) + extend_high(sad);
        return horizontal_add(sum);
    }
    for (row = 0; row < max_iterators; row += 16)
    {
        unrollFunc_16<16>(fenc, fencstride, fref, frefstride, sad);
        sum += extend_low(sad) + extend_high(sad);
        sad = 0;
        fenc += fencstride * 16;
        fref += frefstride * 16;
    }

    if (ly & 8)
    {
        unrollFunc_16<8>(fenc, fencstride, fref, frefstride, sad);
        sum += extend_low(sad) + extend_high(sad);
        return horizontal_add(sum);
    }
    return horizontal_add(sum);
}

template<int ly>
int sad_24(pixel *fenc, intptr_t fencstride, pixel *fref, intptr_t frefstride)
{
    Vec16uc m1, n1;

    Vec4i sum(0);
    Vec8us sad(0);
    int max_iterators = (ly >> 4) << 4;
    int row;

    for (row = 0; row < max_iterators; row += 16)
    {
        for (int i = 0; i < 16; i++)
        {
            m1.load_a(fenc);
            n1.load(fref);
            sad.addSumAbsDiff(m1, n1);

            m1.load_a(fenc + 16);
            m1.cutoff(8);
            n1.load(fref + 16);
            n1.cutoff(8);
            sad.addSumAbsDiff(m1, n1);

            fenc += fencstride;
            fref += frefstride;
        }

        sum += extend_low(sad) + extend_high(sad);
        sad = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(fenc);
        n1.load(fref);
        sad.addSumAbsDiff(m1, n1);

        m1.load_a(fenc + 16);
        m1.cutoff(8);
        n1.load(fref + 16);
        n1.cutoff(8);
        sad.addSumAbsDiff(m1, n1);

        fenc += fencstride;
        fref += frefstride;
    }

    sum += extend_low(sad) + extend_high(sad);
    return horizontal_add(sum);
}

template<int size>
ALWAYSINLINE void unrollFunc_32(pixel *fenc, intptr_t fencstride, pixel *fref, intptr_t frefstride, Vec8us& sad)
{
    unrollFunc_32<1>(fenc, fencstride, fref, frefstride, sad);
    unrollFunc_32<size - 1>(fenc + fencstride, fencstride, fref + frefstride, frefstride, sad);
}

template<>
ALWAYSINLINE void unrollFunc_32<1>(pixel *fenc, intptr_t, pixel *fref, intptr_t, Vec8us& sad)
{
    Vec16uc m1, n1;

    m1.load_a(fenc);
    n1.load(fref);
    sad.addSumAbsDiff(m1, n1);

    m1.load_a(fenc + 16);
    n1.load(fref + 16);
    sad.addSumAbsDiff(m1, n1);
}

template<int ly>
int sad_32(pixel * fenc, intptr_t fencstride, pixel * fref, intptr_t frefstride)
{
    Vec4i sum(0);
    Vec8us sad;
    int max_iterators = (ly >> 2) << 2;
    int row;
    if (ly == 4)
    {
        sad = 0;
        unrollFunc_32<4>(fenc, fencstride, fref, frefstride, sad);
        sum += extend_low(sad) + extend_high(sad);
        return horizontal_add(sum);
    }
    for (row = 0; row < max_iterators; row += 4)
    {
        sad = 0;
        unrollFunc_32<4>(fenc, fencstride, fref, frefstride, sad);
        sum += extend_low(sad) + extend_high(sad);
        fenc += fencstride * 4;
        fref += frefstride * 4;
    }

    return horizontal_add(sum);
}

template<int size>
ALWAYSINLINE void unrollFunc_48(pixel *fenc, intptr_t fencstride, pixel *fref, intptr_t frefstride, Vec8us *sad)
{
    unrollFunc_48<1>(fenc, fencstride, fref, frefstride, sad);
    unrollFunc_48<size - 1>(fenc + fencstride, fencstride, fref + frefstride, frefstride, sad);
}

template<>
ALWAYSINLINE void unrollFunc_48<1>(pixel *fenc, intptr_t, pixel *fref, intptr_t, Vec8us *sad)
{
    Vec16uc m1, n1;

    m1.load_a(fenc);
    n1.load(fref);
    sad[0].addSumAbsDiff(m1, n1);

    m1.load_a(fenc + 16);
    n1.load(fref + 16);
    sad[0].addSumAbsDiff(m1, n1);

    m1.load_a(fenc + 32);
    n1.load(fref + 32);
    sad[0].addSumAbsDiff(m1, n1);
}

template<int ly>
int sad_48(pixel * fenc, intptr_t fencstride, pixel * fref, intptr_t frefstride)
{
    Vec4i sum(0);
    Vec8us sad(0);
    int max_iterators = (ly >> 3) << 3;
    int row;
    if (ly == 4)
    {
        unrollFunc_48<4>(fenc, fencstride, fref, frefstride, &sad);
        sum += extend_low(sad) + extend_high(sad);
        return horizontal_add(sum);
    }
    for (row = 0; row < max_iterators; row += 8)
    {
        unrollFunc_48<8>(fenc, fencstride, fref, frefstride, &sad);
        sum += extend_low(sad) + extend_high(sad);
        sad = 0;
        fenc += fencstride * 8;
        fref += frefstride * 8;
    }

    if (ly & 4)
    {
        unrollFunc_48<4>(fenc, fencstride, fref, frefstride, &sad);
        sum += extend_low(sad) + extend_high(sad);
    }
    return horizontal_add(sum);
}

template<int size>
ALWAYSINLINE void unrollFunc_64(pixel *fenc, intptr_t fencstride, pixel *fref, intptr_t frefstride, Vec8us& sad)
{
    unrollFunc_64<1>(fenc, fencstride, fref, frefstride, sad);
    unrollFunc_64<size - 1>(fenc + fencstride, fencstride, fref + frefstride, frefstride, sad);
}

template<>
ALWAYSINLINE void unrollFunc_64<1>(pixel *fenc, intptr_t, pixel *fref, intptr_t, Vec8us& sad)
{
    Vec16uc m1, n1;

    m1.load_a(fenc);
    n1.load(fref);
    sad.addSumAbsDiff(m1, n1);

    m1.load_a(fenc + 16);
    n1.load(fref + 16);
    sad.addSumAbsDiff(m1, n1);

    m1.load_a(fenc + 32);
    n1.load(fref + 32);
    sad.addSumAbsDiff(m1, n1);

    m1.load_a(fenc + 48);
    n1.load(fref + 48);
    sad.addSumAbsDiff(m1, n1);
}

template<int ly>
int sad_64(pixel * fenc, intptr_t fencstride, pixel * fref, intptr_t frefstride)
{
    Vec4i sum(0);
    Vec8us sad;
    int max_iterators = (ly >> 2) << 2;
    int row;
    for (row = 0; row < max_iterators; row += 4)
    {
        sad = 0;
        unrollFunc_64<4>(fenc, fencstride, fref, frefstride, sad);
        sum += extend_low(sad) + extend_high(sad);
        fenc += fencstride * 4;
        fref += frefstride * 4;
    }

    return horizontal_add(sum);
}

template<int ly>
void sad_x3_4(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, intptr_t frefstride, int *res)
{
    Vec16uc m1, n1, n2, n3;

    Vec4i sum1(0), sum2(0), sum3(0);
    Vec8us sad1(0), sad2(0), sad3(0);
    int max_iterators = (ly >> 4) << 4;
    int row;

    for (row = 0; row < max_iterators; row += 16)
    {
        for (int i = 0; i < 16; i++)
        {
            m1.fromUint32(*(uint32_t*)fenc);
            n1.fromUint32(*(uint32_t*)fref1);
            n2.fromUint32(*(uint32_t*)fref2);
            n3.fromUint32(*(uint32_t*)fref3);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            fenc += FENC_STRIDE;
            fref1 += frefstride;
            fref2 += frefstride;
            fref3 += frefstride;
        }

        sum1 += extend_low(sad1);
        sum2 += extend_low(sad2);
        sum3 += extend_low(sad3);
        sad1 = 0;
        sad2 = 0;
        sad3 = 0;
    }

    while (row++ < ly)
    {
        m1.fromUint32(*(uint32_t*)fenc);
        n1.fromUint32(*(uint32_t*)fref1);
        n2.fromUint32(*(uint32_t*)fref2);
        n3.fromUint32(*(uint32_t*)fref3);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);

        fenc += FENC_STRIDE;
        fref1 += frefstride;
        fref2 += frefstride;
        fref3 += frefstride;
    }

    sum1 += extend_low(sad1);
    sum2 += extend_low(sad2);
    sum3 += extend_low(sad3);

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
}

/* For performance - This function assumes that the *last load* can access 16 elements. */
template<int ly>
void sad_x3_8(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, intptr_t frefstride, int *res)
{
    Vec16uc m1, n1, n2, n3;

    Vec4i sum1(0), sum2(0), sum3(0);
    Vec8us sad1(0), sad2(0), sad3(0);
    int max_iterators = (ly >> 4) << 4;
    int row;

    for (row = 0; row < max_iterators; row += 16)
    {
        for (int i = 0; i < 16; i++)
        {
            m1.load_a(fenc);
            n1.load(fref1);
            n2.load(fref2);
            n3.load(fref3);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            fenc += FENC_STRIDE;
            fref1 += frefstride;
            fref2 += frefstride;
            fref3 += frefstride;
        }

        sum1 += extend_low(sad1);
        sum2 += extend_low(sad2);
        sum3 += extend_low(sad3);
        sad1 = 0;
        sad2 = 0;
        sad3 = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(fenc);
        n1.load(fref1);
        n2.load(fref2);
        n3.load(fref3);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);

        fenc += FENC_STRIDE;
        fref1 += frefstride;
        fref2 += frefstride;
        fref3 += frefstride;
    }

    sum1 += extend_low(sad1);
    sum2 += extend_low(sad2);
    sum3 += extend_low(sad3);

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
}

/* For performance - This function assumes that the *last load* can access 16 elements. */
template<int ly>
void sad_x3_12(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, intptr_t frefstride, int *res)
{
    Vec16uc m1, n1, n2, n3;

    Vec4i sum1(0), sum2(0), sum3(0);
    Vec8us sad1(0), sad2(0), sad3(0);
    int max_iterators = (ly >> 4) << 4;
    int row;

    for (row = 0; row < max_iterators; row += 16)
    {
        for (int i = 0; i < 16; i++)
        {
            m1.load_a(fenc);
            m1.cutoff(12);
            n1.load(fref1);
            n1.cutoff(12);
            n2.load(fref2);
            n2.cutoff(12);
            n3.load(fref3);
            n3.cutoff(12);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            fenc += FENC_STRIDE;
            fref1 += frefstride;
            fref2 += frefstride;
            fref3 += frefstride;
        }

        sum1 += extend_low(sad1) + extend_high(sad1);
        sum2 += extend_low(sad2) + extend_high(sad2);
        sum3 += extend_low(sad3) + extend_high(sad3);
        sad1 = 0;
        sad2 = 0;
        sad3 = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(fenc);
        m1.cutoff(12);
        n1.load(fref1);
        n1.cutoff(12);
        n2.load(fref2);
        n2.cutoff(12);
        n3.load(fref3);
        n3.cutoff(12);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);

        fenc += FENC_STRIDE;
        fref1 += frefstride;
        fref2 += frefstride;
        fref3 += frefstride;
    }

    sum1 += extend_low(sad1) + extend_high(sad1);
    sum2 += extend_low(sad2) + extend_high(sad2);
    sum3 += extend_low(sad3) + extend_high(sad3);

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
}

template<int ly>
void sad_x3_16(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, intptr_t frefstride, int *res)
{
    Vec16uc m1, n1, n2, n3;

    Vec4i sum1(0), sum2(0), sum3(0);
    Vec8us sad1(0), sad2(0), sad3(0);
    int max_iterators = (ly >> 4) << 4;
    int row;

    for (row = 0; row < max_iterators; row += 16)
    {
        for (int i = 0; i < 16; i++)
        {
            m1.load_a(fenc);
            n1.load(fref1);
            n2.load(fref2);
            n3.load(fref3);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            fenc += FENC_STRIDE;
            fref1 += frefstride;
            fref2 += frefstride;
            fref3 += frefstride;
        }

        sum1 += extend_low(sad1) + extend_high(sad1);
        sum2 += extend_low(sad2) + extend_high(sad2);
        sum3 += extend_low(sad3) + extend_high(sad3);
        sad1 = 0;
        sad2 = 0;
        sad3 = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(fenc);
        n1.load(fref1);
        n2.load(fref2);
        n3.load(fref3);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);

        fenc += FENC_STRIDE;
        fref1 += frefstride;
        fref2 += frefstride;
        fref3 += frefstride;
    }

    sum1 += extend_low(sad1) + extend_high(sad1);
    sum2 += extend_low(sad2) + extend_high(sad2);
    sum3 += extend_low(sad3) + extend_high(sad3);

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
}

template<int ly>
void sad_x3_24(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, intptr_t frefstride, int *res)
{
    Vec16uc m1, n1, n2, n3;

    Vec4i sum1(0), sum2(0), sum3(0);
    Vec8us sad1(0), sad2(0), sad3(0);
    int max_iterators = (ly >> 4) << 4;
    int row;

    for (row = 0; row < max_iterators; row += 16)
    {
        for (int i = 0; i < 16; i++)
        {
            m1.load_a(fenc);
            n1.load(fref1);
            n2.load(fref2);
            n3.load(fref3);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            m1.load_a(fenc + 16);
            m1.cutoff(8);
            n1.load(fref1 + 16);
            n1.cutoff(8);
            n2.load(fref2 + 16);
            n2.cutoff(8);
            n3.load(fref3 + 16);
            n3.cutoff(8);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            fenc += FENC_STRIDE;
            fref1 += frefstride;
            fref2 += frefstride;
            fref3 += frefstride;
        }

        sum1 += extend_low(sad1) + extend_high(sad1);
        sum2 += extend_low(sad2) + extend_high(sad2);
        sum3 += extend_low(sad3) + extend_high(sad3);
        sad1 = 0;
        sad2 = 0;
        sad3 = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(fenc);
        n1.load(fref1);
        n2.load(fref2);
        n3.load(fref3);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);

        m1.load_a(fenc + 16);
        m1.cutoff(8);
        n1.load(fref1 + 16);
        n1.cutoff(8);
        n2.load(fref2 + 16);
        n2.cutoff(8);
        n3.load(fref3 + 16);
        n3.cutoff(8);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);

        fenc += FENC_STRIDE;
        fref1 += frefstride;
        fref2 += frefstride;
        fref3 += frefstride;
    }

    sum1 += extend_low(sad1) + extend_high(sad1);
    sum2 += extend_low(sad2) + extend_high(sad2);
    sum3 += extend_low(sad3) + extend_high(sad3);

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
}

template<int ly>
void sad_x3_32(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, intptr_t frefstride, int *res)
{
    Vec16uc m1, n1, n2, n3;

    Vec4i sum1(0), sum2(0), sum3(0);
    Vec8us sad1(0), sad2(0), sad3(0);
    int max_iterators = (ly >> 3) << 3;
    int row;

    for (row = 0; row < max_iterators; row += 8)
    {
        for (int i = 0; i < 8; i++)
        {
            m1.load_a(fenc);
            n1.load(fref1);
            n2.load(fref2);
            n3.load(fref3);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            m1.load_a(fenc + 16);
            n1.load(fref1 + 16);
            n2.load(fref2 + 16);
            n3.load(fref3 + 16);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            fenc += FENC_STRIDE;
            fref1 += frefstride;
            fref2 += frefstride;
            fref3 += frefstride;
        }

        sum1 += extend_low(sad1) + extend_high(sad1);
        sum2 += extend_low(sad2) + extend_high(sad2);
        sum3 += extend_low(sad3) + extend_high(sad3);
        sad1 = 0;
        sad2 = 0;
        sad3 = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(fenc);
        n1.load(fref1);
        n2.load(fref2);
        n3.load(fref3);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);

        m1.load_a(fenc + 16);
        n1.load(fref1 + 16);
        n2.load(fref2 + 16);
        n3.load(fref3 + 16);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);

        fenc += FENC_STRIDE;
        fref1 += frefstride;
        fref2 += frefstride;
        fref3 += frefstride;
    }

    sum1 += extend_low(sad1) + extend_high(sad1);
    sum2 += extend_low(sad2) + extend_high(sad2);
    sum3 += extend_low(sad3) + extend_high(sad3);

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
}

template<int ly>
void sad_x3_48(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, intptr_t frefstride, int *res)
{
    Vec16uc m1, n1, n2, n3;

    Vec4i sum1(0), sum2(0), sum3(0);
    Vec8us sad1(0), sad2(0), sad3(0);
    int max_iterators = (ly >> 3) << 3;
    int row;

    for (row = 0; row < max_iterators; row += 8)
    {
        for (int i = 0; i < 8; i++)
        {
            m1.load_a(fenc);
            n1.load(fref1);
            n2.load(fref2);
            n3.load(fref3);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            m1.load_a(fenc + 16);
            n1.load(fref1 + 16);
            n2.load(fref2 + 16);
            n3.load(fref3 + 16);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            m1.load_a(fenc + 32);
            n1.load(fref1 + 32);
            n2.load(fref2 + 32);
            n3.load(fref3 + 32);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            fenc += FENC_STRIDE;
            fref1 += frefstride;
            fref2 += frefstride;
            fref3 += frefstride;
        }

        sum1 += extend_low(sad1) + extend_high(sad1);
        sum2 += extend_low(sad2) + extend_high(sad2);
        sum3 += extend_low(sad3) + extend_high(sad3);
        sad1 = 0;
        sad2 = 0;
        sad3 = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(fenc);
        n1.load(fref1);
        n2.load(fref2);
        n3.load(fref3);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);

        m1.load_a(fenc + 16);
        n1.load(fref1 + 16);
        n2.load(fref2 + 16);
        n3.load(fref3 + 16);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);

        m1.load_a(fenc + 32);
        n1.load(fref1 + 32);
        n2.load(fref2 + 32);
        n3.load(fref3 + 32);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);

        fenc += FENC_STRIDE;
        fref1 += frefstride;
        fref2 += frefstride;
        fref3 += frefstride;
    }

    sum1 += extend_low(sad1) + extend_high(sad1);
    sum2 += extend_low(sad2) + extend_high(sad2);
    sum3 += extend_low(sad3) + extend_high(sad3);

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
}

template<int ly>
void sad_x3_64(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, intptr_t frefstride, int *res)
{
    Vec16uc m1, n1, n2, n3;

    Vec4i sum1(0), sum2(0), sum3(0);
    Vec8us sad1(0), sad2(0), sad3(0);
    int row;

    for (row = 0; row < ly; row += 4)
    {
        for (int i = 0; i < 4; i++)
        {
            m1.load_a(fenc);
            n1.load(fref1);
            n2.load(fref2);
            n3.load(fref3);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            m1.load_a(fenc + 16);
            n1.load(fref1 + 16);
            n2.load(fref2 + 16);
            n3.load(fref3 + 16);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            m1.load_a(fenc + 32);
            n1.load(fref1 + 32);
            n2.load(fref2 + 32);
            n3.load(fref3 + 32);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            m1.load_a(fenc + 48);
            n1.load(fref1 + 48);
            n2.load(fref2 + 48);
            n3.load(fref3 + 48);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            fenc += FENC_STRIDE;
            fref1 += frefstride;
            fref2 += frefstride;
            fref3 += frefstride;
        }

        sum1 += extend_low(sad1) + extend_high(sad1);
        sum2 += extend_low(sad2) + extend_high(sad2);
        sum3 += extend_low(sad3) + extend_high(sad3);
        sad1 = 0;
        sad2 = 0;
        sad3 = 0;
    }

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
}

template<int ly>
void sad_x4_4(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, pixel *fref4, intptr_t frefstride, int *res)
{
    Vec16uc m1, n1, n2, n3, n4;

    Vec4i sum1(0), sum2(0), sum3(0), sum4(0);
    Vec8us sad1(0), sad2(0), sad3(0), sad4(0);
    int max_iterators = (ly >> 4) << 4;
    int row;

    for (row = 0; row < max_iterators; row += 16)
    {
        for (int i = 0; i < 16; i++)
        {
            m1.fromUint32(*(uint32_t*)fenc);
            n1.fromUint32(*(uint32_t*)fref1);
            n2.fromUint32(*(uint32_t*)fref2);
            n3.fromUint32(*(uint32_t*)fref3);
            n4.fromUint32(*(uint32_t*)fref4);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            fenc += FENC_STRIDE;
            fref1 += frefstride;
            fref2 += frefstride;
            fref3 += frefstride;
            fref4 += frefstride;
        }

        sum1 += extend_low(sad1);
        sum2 += extend_low(sad2);
        sum3 += extend_low(sad3);
        sum4 += extend_low(sad4);
        sad1 = 0;
        sad2 = 0;
        sad3 = 0;
        sad4 = 0;
    }

    while (row++ < ly)
    {
        m1.fromUint32(*(uint32_t*)fenc);
        n1.fromUint32(*(uint32_t*)fref1);
        n2.fromUint32(*(uint32_t*)fref2);
        n3.fromUint32(*(uint32_t*)fref3);
        n4.fromUint32(*(uint32_t*)fref4);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);
        sad4.addSumAbsDiff(m1, n4);

        fenc += FENC_STRIDE;
        fref1 += frefstride;
        fref2 += frefstride;
        fref3 += frefstride;
        fref4 += frefstride;
    }

    sum1 += extend_low(sad1);
    sum2 += extend_low(sad2);
    sum3 += extend_low(sad3);
    sum4 += extend_low(sad4);

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
    res[3] = horizontal_add(sum4);
}

template<int ly>
void sad_x4_8(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, pixel *fref4, intptr_t frefstride, int *res)
{
    Vec16uc m1, n1, n2, n3, n4;

    Vec4i sum1(0), sum2(0), sum3(0), sum4(0);
    Vec8us sad1(0), sad2(0), sad3(0), sad4(0);
    int max_iterators = (ly >> 4) << 4;
    int row;

    for (row = 0; row < max_iterators; row += 16)
    {
        for (int i = 0; i < 16; i++)
        {
            m1.load_a(fenc);
            n1.load(fref1);
            n2.load(fref2);
            n3.load(fref3);
            n4.load(fref4);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            fenc += FENC_STRIDE;
            fref1 += frefstride;
            fref2 += frefstride;
            fref3 += frefstride;
            fref4 += frefstride;
        }

        sum1 += extend_low(sad1);
        sum2 += extend_low(sad2);
        sum3 += extend_low(sad3);
        sum4 += extend_low(sad4);
        sad1 = 0;
        sad2 = 0;
        sad3 = 0;
        sad4 = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(fenc);
        n1.load(fref1);
        n2.load(fref2);
        n3.load(fref3);
        n4.load(fref4);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);
        sad4.addSumAbsDiff(m1, n4);

        fenc += FENC_STRIDE;
        fref1 += frefstride;
        fref2 += frefstride;
        fref3 += frefstride;
        fref4 += frefstride;
    }

    sum1 += extend_low(sad1);
    sum2 += extend_low(sad2);
    sum3 += extend_low(sad3);
    sum4 += extend_low(sad4);

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
    res[3] = horizontal_add(sum4);
}

/* For performance - This function assumes that the *last load* can access 16 elements. */
template<int ly>
void sad_x4_12(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, pixel *fref4, intptr_t frefstride, int *res)
{
    Vec16uc m1, n1, n2, n3, n4;

    Vec4i sum1(0), sum2(0), sum3(0), sum4(0);
    Vec8us sad1(0), sad2(0), sad3(0), sad4(0);
    int max_iterators = (ly >> 4) << 4;
    int row;

    for (row = 0; row < max_iterators; row += 16)
    {
        for (int i = 0; i < 16; i++)
        {
            m1.load_a(fenc);
            m1.cutoff(12);
            n1.load(fref1);
            n1.cutoff(12);
            n2.load(fref2);
            n2.cutoff(12);
            n3.load(fref3);
            n3.cutoff(12);
            n4.load(fref4);
            n4.cutoff(12);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            fenc += FENC_STRIDE;
            fref1 += frefstride;
            fref2 += frefstride;
            fref3 += frefstride;
            fref4 += frefstride;
        }

        sum1 += extend_low(sad1) + extend_high(sad1);
        sum2 += extend_low(sad2) + extend_high(sad2);
        sum3 += extend_low(sad3) + extend_high(sad3);
        sum4 += extend_low(sad4) + extend_high(sad4);
        sad1 = 0;
        sad2 = 0;
        sad3 = 0;
        sad4 = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(fenc);
        m1.cutoff(12);
        n1.load(fref1);
        n1.cutoff(12);
        n2.load(fref2);
        n2.cutoff(12);
        n3.load(fref3);
        n3.cutoff(12);
        n4.load(fref4);
        n4.cutoff(12);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);
        sad4.addSumAbsDiff(m1, n4);

        fenc += FENC_STRIDE;
        fref1 += frefstride;
        fref2 += frefstride;
        fref3 += frefstride;
        fref4 += frefstride;
    }

    sum1 += extend_low(sad1) + extend_high(sad1);
    sum2 += extend_low(sad2) + extend_high(sad2);
    sum3 += extend_low(sad3) + extend_high(sad3);
    sum4 += extend_low(sad4) + extend_high(sad4);

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
    res[3] = horizontal_add(sum4);
}

template<int ly>
void sad_x4_16(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, pixel *fref4, intptr_t frefstride, int *res)
{
    Vec16uc m1, n1, n2, n3, n4;

    Vec4i sum1(0), sum2(0), sum3(0), sum4(0);
    Vec8us sad1(0), sad2(0), sad3(0), sad4(0);
    int max_iterators = (ly >> 4) << 4;
    int row;

    for (row = 0; row < max_iterators; row += 16)
    {
        for (int i = 0; i < 16; i++)
        {
            m1.load_a(fenc);
            n1.load(fref1);
            n2.load(fref2);
            n3.load(fref3);
            n4.load(fref4);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            fenc += FENC_STRIDE;
            fref1 += frefstride;
            fref2 += frefstride;
            fref3 += frefstride;
            fref4 += frefstride;
        }

        sum1 += extend_low(sad1) + extend_high(sad1);
        sum2 += extend_low(sad2) + extend_high(sad2);
        sum3 += extend_low(sad3) + extend_high(sad3);
        sum4 += extend_low(sad4) + extend_high(sad4);
        sad1 = 0;
        sad2 = 0;
        sad3 = 0;
        sad4 = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(fenc);
        n1.load(fref1);
        n2.load(fref2);
        n3.load(fref3);
        n4.load(fref4);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);
        sad4.addSumAbsDiff(m1, n4);

        fenc += FENC_STRIDE;
        fref1 += frefstride;
        fref2 += frefstride;
        fref3 += frefstride;
        fref4 += frefstride;
    }

    sum1 += extend_low(sad1) + extend_high(sad1);
    sum2 += extend_low(sad2) + extend_high(sad2);
    sum3 += extend_low(sad3) + extend_high(sad3);
    sum4 += extend_low(sad4) + extend_high(sad4);

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
    res[3] = horizontal_add(sum4);
}

template<int ly>
void sad_x4_24(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, pixel *fref4, intptr_t frefstride, int *res)
{
    Vec16uc m1, n1, n2, n3, n4;

    Vec4i sum1(0), sum2(0), sum3(0), sum4(0);
    Vec8us sad1(0), sad2(0), sad3(0), sad4(0);
    int max_iterators = (ly >> 4) << 4;
    int row;

    for (row = 0; row < max_iterators; row += 16)
    {
        for (int i = 0; i < 16; i++)
        {
            m1.load_a(fenc);
            n1.load(fref1);
            n2.load(fref2);
            n3.load(fref3);
            n4.load(fref4);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            m1.load_a(fenc + 16);
            m1.cutoff(8);
            n1.load(fref1 + 16);
            n1.cutoff(8);
            n2.load(fref2 + 16);
            n2.cutoff(8);
            n3.load(fref3 + 16);
            n3.cutoff(8);
            n4.load(fref4 + 16);
            n4.cutoff(8);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            fenc += FENC_STRIDE;
            fref1 += frefstride;
            fref2 += frefstride;
            fref3 += frefstride;
            fref4 += frefstride;
        }

        sum1 += extend_low(sad1) + extend_high(sad1);
        sum2 += extend_low(sad2) + extend_high(sad2);
        sum3 += extend_low(sad3) + extend_high(sad3);
        sum4 += extend_low(sad4) + extend_high(sad4);
        sad1 = 0;
        sad2 = 0;
        sad3 = 0;
        sad4 = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(fenc);
        n1.load(fref1);
        n2.load(fref2);
        n3.load(fref3);
        n4.load(fref4);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);
        sad4.addSumAbsDiff(m1, n4);

        m1.load_a(fenc + 16);
        m1.cutoff(8);
        n1.load(fref1 + 16);
        n1.cutoff(8);
        n2.load(fref2 + 16);
        n2.cutoff(8);
        n3.load(fref3 + 16);
        n3.cutoff(8);
        n4.load(fref4 + 16);
        n4.cutoff(8);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);
        sad4.addSumAbsDiff(m1, n4);

        fenc += FENC_STRIDE;
        fref1 += frefstride;
        fref2 += frefstride;
        fref3 += frefstride;
        fref4 += frefstride;
    }

    sum1 += extend_low(sad1) + extend_high(sad1);
    sum2 += extend_low(sad2) + extend_high(sad2);
    sum3 += extend_low(sad3) + extend_high(sad3);
    sum4 += extend_low(sad4) + extend_high(sad4);

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
    res[3] = horizontal_add(sum4);
}

template<int ly>
void sad_x4_32(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, pixel *fref4, intptr_t frefstride, int *res)
{
    Vec16uc m1, n1, n2, n3, n4;

    Vec4i sum1(0), sum2(0), sum3(0), sum4(0);
    Vec8us sad1(0), sad2(0), sad3(0), sad4(0);
    int max_iterators = (ly >> 3) << 3;
    int row;

    for (row = 0; row < max_iterators; row += 8)
    {
        for (int i = 0; i < 8; i++)
        {
            m1.load_a(fenc);
            n1.load(fref1);
            n2.load(fref2);
            n3.load(fref3);
            n4.load(fref4);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            m1.load_a(fenc + 16);
            n1.load(fref1 + 16);
            n2.load(fref2 + 16);
            n3.load(fref3 + 16);
            n4.load(fref4 + 16);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            fenc += FENC_STRIDE;
            fref1 += frefstride;
            fref2 += frefstride;
            fref3 += frefstride;
            fref4 += frefstride;
        }

        sum1 += extend_low(sad1) + extend_high(sad1);
        sum2 += extend_low(sad2) + extend_high(sad2);
        sum3 += extend_low(sad3) + extend_high(sad3);
        sum4 += extend_low(sad4) + extend_high(sad4);
        sad1 = 0;
        sad2 = 0;
        sad3 = 0;
        sad4 = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(fenc);
        n1.load(fref1);
        n2.load(fref2);
        n3.load(fref3);
        n4.load(fref4);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);
        sad4.addSumAbsDiff(m1, n4);

        m1.load_a(fenc + 16);
        n1.load(fref1 + 16);
        n2.load(fref2 + 16);
        n3.load(fref3 + 16);
        n4.load(fref4 + 16);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);
        sad4.addSumAbsDiff(m1, n4);

        fenc += FENC_STRIDE;
        fref1 += frefstride;
        fref2 += frefstride;
        fref3 += frefstride;
        fref4 += frefstride;
    }

    sum1 += extend_low(sad1) + extend_high(sad1);
    sum2 += extend_low(sad2) + extend_high(sad2);
    sum3 += extend_low(sad3) + extend_high(sad3);
    sum4 += extend_low(sad4) + extend_high(sad4);

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
    res[3] = horizontal_add(sum4);
}

template<int ly>
void sad_x4_48(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, pixel *fref4, intptr_t frefstride, int *res)
{
    Vec16uc m1, n1, n2, n3, n4;

    Vec4i sum1(0), sum2(0), sum3(0), sum4(0);
    Vec8us sad1(0), sad2(0), sad3(0), sad4(0);
    int max_iterators = (ly >> 3) << 3;
    int row;

    for (row = 0; row < max_iterators; row += 8)
    {
        for (int i = 0; i < 8; i++)
        {
            m1.load_a(fenc);
            n1.load(fref1);
            n2.load(fref2);
            n3.load(fref3);
            n4.load(fref4);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            m1.load_a(fenc + 16);
            n1.load(fref1 + 16);
            n2.load(fref2 + 16);
            n3.load(fref3 + 16);
            n4.load(fref4 + 16);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            m1.load_a(fenc + 32);
            n1.load(fref1 + 32);
            n2.load(fref2 + 32);
            n3.load(fref3 + 32);
            n4.load(fref4 + 32);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            fenc += FENC_STRIDE;
            fref1 += frefstride;
            fref2 += frefstride;
            fref3 += frefstride;
            fref4 += frefstride;
        }

        sum1 += extend_low(sad1) + extend_high(sad1);
        sum2 += extend_low(sad2) + extend_high(sad2);
        sum3 += extend_low(sad3) + extend_high(sad3);
        sum4 += extend_low(sad4) + extend_high(sad4);
        sad1 = 0;
        sad2 = 0;
        sad3 = 0;
        sad4 = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(fenc);
        n1.load(fref1);
        n2.load(fref2);
        n3.load(fref3);
        n4.load(fref4);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);
        sad4.addSumAbsDiff(m1, n4);

        m1.load_a(fenc + 16);
        n1.load(fref1 + 16);
        n2.load(fref2 + 16);
        n3.load(fref3 + 16);
        n4.load(fref4 + 16);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);
        sad4.addSumAbsDiff(m1, n4);

        m1.load_a(fenc + 32);
        n1.load(fref1 + 32);
        n2.load(fref2 + 32);
        n3.load(fref3 + 32);
        n4.load(fref4 + 32);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);
        sad4.addSumAbsDiff(m1, n4);

        fenc += FENC_STRIDE;
        fref1 += frefstride;
        fref2 += frefstride;
        fref3 += frefstride;
        fref4 += frefstride;
    }

    sum1 += extend_low(sad1) + extend_high(sad1);
    sum2 += extend_low(sad2) + extend_high(sad2);
    sum3 += extend_low(sad3) + extend_high(sad3);
    sum4 += extend_low(sad4) + extend_high(sad4);

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
    res[3] = horizontal_add(sum4);
}

template<int ly>
void sad_x4_64(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, pixel *fref4, intptr_t frefstride, int *res)
{
    Vec16uc m1, n1, n2, n3, n4;

    Vec4i sum1(0), sum2(0), sum3(0), sum4(0);
    Vec8us sad1(0), sad2(0), sad3(0), sad4(0);
    int row;

    for (row = 0; row < ly; row += 4)
    {
        for (int i = 0; i < 4; i++)
        {
            m1.load_a(fenc);
            n1.load(fref1);
            n2.load(fref2);
            n3.load(fref3);
            n4.load(fref4);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            m1.load_a(fenc + 16);
            n1.load(fref1 + 16);
            n2.load(fref2 + 16);
            n3.load(fref3 + 16);
            n4.load(fref4 + 16);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            m1.load_a(fenc + 32);
            n1.load(fref1 + 32);
            n2.load(fref2 + 32);
            n3.load(fref3 + 32);
            n4.load(fref4 + 32);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            m1.load_a(fenc + 48);
            n1.load(fref1 + 48);
            n2.load(fref2 + 48);
            n3.load(fref3 + 48);
            n4.load(fref4 + 48);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            fenc += FENC_STRIDE;
            fref1 += frefstride;
            fref2 += frefstride;
            fref3 += frefstride;
            fref4 += frefstride;
        }

        sum1 += extend_low(sad1) + extend_high(sad1);
        sum2 += extend_low(sad2) + extend_high(sad2);
        sum3 += extend_low(sad3) + extend_high(sad3);
        sum4 += extend_low(sad4) + extend_high(sad4);
        sad1 = 0;
        sad2 = 0;
        sad3 = 0;
        sad4 = 0;
    }

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
    res[3] = horizontal_add(sum4);
}

void getResidual4(pixel *fenc, pixel *pred, short *resi, int stride)
{
    for (int y = 0; y < 4; y++)
    {
        Vec16uc f;
        f.fromUint32(*(uint32_t*)fenc);
        Vec16uc p;
        p.fromUint32(*(uint32_t*)pred);
        Vec8s r = extend_low(f) - extend_low(p);
        store_partial(const_int(8), resi, r);

        fenc += stride;
        pred += stride;
        resi += stride;
    }
}

void getResidual8(pixel *fenc, pixel *pred, short *resi, int stride)
{
    for (int y = 0; y < 8; y++)
    {
        Vec16uc f;
        f.load(fenc);
        Vec16uc p;
        p.load(pred);
        Vec8s r = extend_low(f) - extend_low(p);
        r.store(resi);

        fenc += stride;
        pred += stride;
        resi += stride;
    }
}

void getResidual16(pixel *fenc, pixel *pred, short *resi, int stride)
{
    Vec16uc f, p;
    Vec8s r;

    for (int y = 0; y < 16; y++)
    {
        f.load_a(fenc);
        p.load_a(pred);
        r = extend_low(f) - extend_low(p);
        r.store(resi);
        r = extend_high(f) - extend_high(p);
        r.store(resi + 8);

        fenc += stride;
        pred += stride;
        resi += stride;
    }
}

void getResidual32(pixel *fenc, pixel *pred, short *resi, int stride)
{
    Vec16uc f, p;
    Vec8s r;

    for (int y = 0; y < 32; y++)
    {
        f.load_a(fenc);
        p.load_a(pred);
        r = extend_low(f) - extend_low(p);
        r.store(resi);
        r = extend_high(f) - extend_high(p);
        r.store(resi + 8);

        f.load_a(fenc + 16);
        p.load_a(pred + 16);
        r = extend_low(f) - extend_low(p);
        r.store(resi + 16);
        r = extend_high(f) - extend_high(p);
        r.store(resi + 24);

        fenc += stride;
        pred += stride;
        resi += stride;
    }
}

void getResidual64(pixel *fenc, pixel *pred, short *resi, int stride)
{
    Vec16uc f, p;
    Vec8s r;

    for (int y = 0; y < 64; y++)
    {
        f.load_a(fenc);
        p.load_a(pred);
        r = extend_low(f) - extend_low(p);
        r.store(resi);
        r = extend_high(f) - extend_high(p);
        r.store(resi + 8);

        f.load_a(fenc + 16);
        p.load_a(pred + 16);
        r = extend_low(f) - extend_low(p);
        r.store(resi + 16);
        r = extend_high(f) - extend_high(p);
        r.store(resi + 24);

        f.load_a(fenc + 32);
        p.load_a(pred + 32);
        r = extend_low(f) - extend_low(p);
        r.store(resi + 32);
        r = extend_high(f) - extend_high(p);
        r.store(resi + 40);

        f.load_a(fenc + 48);
        p.load_a(pred + 48);
        r = extend_low(f) - extend_low(p);
        r.store(resi + 48);
        r = extend_high(f) - extend_high(p);
        r.store(resi + 56);

        fenc += stride;
        pred += stride;
        resi += stride;
    }
}

void calcRecons4(pixel* pPred, short* pResi, pixel* pReco, short* pRecQt, pixel* pRecIPred, int stride, int recstride, int ipredstride)
{
    for (int y = 0; y < 4; y++)
    {
        Vec8s vresi, vpred, vres, vsum;
        Vec16uc tmp;

        tmp = load_partial(const_int(4), pPred);
        vpred = extend_low(tmp);

        vresi = load_partial(const_int(8), pResi);
        vsum = vpred + vresi;

        vsum = min(255, max(vsum, 0));

        store_partial(const_int(8), pRecQt, vsum);

        tmp = compress(vsum, vsum);

        store_partial(const_int(4), pReco, tmp);
        store_partial(const_int(4), pRecIPred, tmp);

        pPred     += stride;
        pResi     += stride;
        pReco     += stride;
        pRecQt    += recstride;
        pRecIPred += ipredstride;
    }
}

void calcRecons8(pixel* pPred, short* pResi, pixel* pReco, short* pRecQt, pixel* pRecIPred, int stride, int recstride, int ipredstride)
{
    for (int y = 0; y < 8; y++)
    {
        Vec8s vresi, vpred, vres, vsum;
        Vec16uc tmp;

        tmp.load(pPred);
        vpred = extend_low(tmp);

        vresi.load(pResi);
        vsum = vpred + vresi;

        vsum = min(255, max(vsum, 0));

        vsum.store(pRecQt);

        tmp = compress(vsum, vsum);

        store_partial(const_int(8), pReco, tmp);
        store_partial(const_int(8), pRecIPred, tmp);

        pPred     += stride;
        pResi     += stride;
        pReco     += stride;
        pRecQt    += recstride;
        pRecIPred += ipredstride;
    }
}

template<int blockSize>
void calcRecons(pixel* pPred, short* pResi, pixel* pReco, short* pRecQt, pixel* pRecIPred, int stride, int recstride, int ipredstride)
{
    for (int y = 0; y < blockSize; y++)
    {
        for (int x = 0; x < blockSize; x += 16)
        {
            Vec8s vresi, vpred, vres, vsum1, vsum2;
            Vec16uc tmp;

            tmp.load(pPred + x);

            vpred = extend_low(tmp);
            vresi.load(pResi + x);
            vsum1 = vpred + vresi;
            vsum1 = min(255, max(vsum1, 0));
            vsum1.store(pRecQt + x);

            vpred = extend_high(tmp);
            vresi.load(pResi + x + 8);
            vsum2 = vpred + vresi;
            vsum2 = min(255, max(vsum2, 0));
            vsum2.store(pRecQt + x + 8);

            tmp = compress(vsum1, vsum2);
            tmp.store(pReco + x);
            tmp.store(pRecIPred + x);
        }

        pPred     += stride;
        pResi     += stride;
        pReco     += stride;
        pRecQt    += recstride;
        pRecIPred += ipredstride;
    }
}

void weightUnidir(short *src, pixel *dst, int srcStride, int dstStride, int width, int height, int w0, int round, int shift, int offset)
{
    int x, y;
    Vec8s tmp;

    Vec4i vw0(w0), vsrc, iofs(IF_INTERNAL_OFFS), ofs(offset), vround(round), vdst;
    for (y = height - 1; y >= 0; y--)
    {
        for (x = 0; x <= width - 4; x += 4)
        {
            tmp  = load_partial(const_int(8), src + x);
            vsrc = extend_low(tmp);
            vdst = ((vw0 * (vsrc + iofs) + vround) >> shift) + ofs;
            store_partial(const_int(4), dst + x, compress_unsafe(compress_saturated(vdst, vdst), 0));
        }

        if (width > x)
        {
            tmp  = load_partial(const_int(4), src + x);
            vsrc = extend_low(tmp);
            vdst = ((vw0 * (vsrc + iofs) + vround) >> shift) + ofs;
            compress_unsafe(compress_saturated(vdst, vdst), 0).store_partial(2, dst + x);
        }
        src += srcStride;
        dst += dstStride;
    }
}

#if INSTRSET >= X265_CPU_LEVEL_AVX2
template<int size>
ALWAYSINLINE void unrollFunc_32_avx2(pixel *fenc, intptr_t fencstride, pixel *fref, intptr_t frefstride, Vec16us& sad)
{
    unrollFunc_32_avx2<1>(fenc, fencstride, fref, frefstride, sad);
    unrollFunc_32_avx2<size - 1>(fenc + fencstride, fencstride, fref + frefstride, frefstride, sad);
}

template<>
ALWAYSINLINE void unrollFunc_32_avx2<1>(pixel *fenc, intptr_t, pixel *fref, intptr_t, Vec16us& sad)
{
    Vec32uc m1, n1;

    m1.load_a(fenc);
    n1.load(fref);
    sad.addSumAbsDiff(m1, n1);
}

template<int ly>
int sad_avx2_32(pixel * fenc, intptr_t fencstride, pixel * fref, intptr_t frefstride)
{
    Vec8i sum(0);
    Vec16us sad(0);
    int max_iterators = (ly >> 4) << 4;
    int row = 0;

    if (ly < 16)
    {
        unrollFunc_32_avx2<ly>(fenc, fencstride, fref, frefstride, sad);
        sum += extend_low(sad) + extend_high(sad);
        return horizontal_add(sum);
    }
    for (row = 0; row < max_iterators; row += 16)
    {
        unrollFunc_32_avx2<16>(fenc, fencstride, fref, frefstride, sad);

        sum += extend_low(sad) + extend_high(sad);
        sad = 0;
        fenc += fencstride * 16;
        fref += frefstride * 16;
    }

    if (ly & 8)
    {
        unrollFunc_32_avx2<8>(fenc, fencstride, fref, frefstride, sad);
        sum += extend_low(sad) + extend_high(sad);
        return horizontal_add(sum);
    }
    return horizontal_add(sum);
}

template<int size>
ALWAYSINLINE void unrollFunc_64_avx2(pixel *fenc, intptr_t fencstride, pixel *fref, intptr_t frefstride, Vec16s& sad)
{
    unrollFunc_64_avx2<1>(fenc, fencstride, fref, frefstride, sad);
    unrollFunc_64_avx2<size - 1>(fenc + fencstride, fencstride, fref + frefstride, frefstride, sad);
}

template<>
ALWAYSINLINE void unrollFunc_64_avx2<1>(pixel *fenc, intptr_t, pixel *fref, intptr_t, Vec16s& sad)
{
    Vec32uc m1, n1;

    m1.load_a(fenc);
    n1.load(fref);
    sad.addSumAbsDiff(m1, n1);

    m1.load_a(fenc + 32);
    n1.load(fref + 32);
    sad.addSumAbsDiff(m1, n1);
}

template<int ly>
int sad_avx2_64(pixel * fenc, intptr_t fencstride, pixel * fref, intptr_t frefstride)
{
    Vec8i sum(0);
    Vec16s sad;
    int max_iterators = (ly >> 2) << 2;
    int row;

    if (ly == 4)
    {
        sad = 0;
        unrollFunc_64_avx2<4>(fenc, fencstride, fref, frefstride, sad);
        sum += extend_low(sad) + extend_high(sad);
        return horizontal_add(sum);
    }
    for (row = 0; row < max_iterators; row += 4)
    {
        sad = 0;
        unrollFunc_64_avx2<4>(fenc, fencstride, fref, frefstride, sad);
        sum += extend_low(sad) + extend_high(sad);
        fenc += fencstride * 4;
        fref += frefstride * 4;
    }

    return horizontal_add(sum);
}

template<int ly>
void sad_avx2_x3_32(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, intptr_t frefstride, int *res)
{
    Vec32uc m1, n1, n2, n3;

    Vec8i sum1(0), sum2(0), sum3(0);
    Vec16us sad1(0), sad2(0), sad3(0);
    int max_iterators = (ly >> 4) << 4;
    int row;

    for (row = 0; row < max_iterators; row += 16)
    {
        for (int i = 0; i < 16; i++)
        {
            m1.load_a(fenc);
            n1.load(fref1);
            n2.load(fref2);
            n3.load(fref3);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            fenc += FENC_STRIDE;
            fref1 += frefstride;
            fref2 += frefstride;
            fref3 += frefstride;
        }

        sum1 += extend_low(sad1) + extend_high(sad1);
        sum2 += extend_low(sad2) + extend_high(sad2);
        sum3 += extend_low(sad3) + extend_high(sad3);
        sad1 = 0;
        sad2 = 0;
        sad3 = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(fenc);
        n1.load(fref1);
        n2.load(fref2);
        n3.load(fref3);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);

        fenc += FENC_STRIDE;
        fref1 += frefstride;
        fref2 += frefstride;
        fref3 += frefstride;
    }

    sum1 += extend_low(sad1) + extend_high(sad1);
    sum2 += extend_low(sad2) + extend_high(sad2);
    sum3 += extend_low(sad3) + extend_high(sad3);

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
}

template<int ly>
void sad_avx2_x3_64(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, intptr_t frefstride, int *res)
{
    Vec32uc m1, n1, n2, n3;

    Vec8i sum1(0), sum2(0), sum3(0);
    Vec16s sad1(0), sad2(0), sad3(0);
    int max_iterators = (ly >> 3) << 3;
    int row;

    for (row = 0; row < max_iterators; row += 8)
    {
        for (int i = 0; i < 8; i++)
        {
            m1.load_a(fenc);
            n1.load(fref1);
            n2.load(fref2);
            n3.load(fref3);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            m1.load_a(fenc + 32);
            n1.load(fref1 + 32);
            n2.load(fref2 + 32);
            n3.load(fref3 + 32);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            fenc += FENC_STRIDE;
            fref1 += frefstride;
            fref2 += frefstride;
            fref3 += frefstride;
        }

        sum1 += extend_low(sad1) + extend_high(sad1);
        sum2 += extend_low(sad2) + extend_high(sad2);
        sum3 += extend_low(sad3) + extend_high(sad3);
        sad1 = 0;
        sad2 = 0;
        sad3 = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(fenc);
        n1.load(fref1);
        n2.load(fref2);
        n3.load(fref3);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);

        m1.load_a(fenc + 32);
        n1.load(fref1 + 32);
        n2.load(fref2 + 32);
        n3.load(fref3 + 32);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);

        fenc += FENC_STRIDE;
        fref1 += frefstride;
        fref2 += frefstride;
        fref3 += frefstride;
    }

    sum1 += extend_low(sad1) + extend_high(sad1);
    sum2 += extend_low(sad2) + extend_high(sad2);
    sum3 += extend_low(sad3) + extend_high(sad3);

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
}

template<int ly>
void sad_avx2_x4_32(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, pixel *fref4, intptr_t frefstride, int *res)
{
    Vec32uc m1, n1, n2, n3, n4;

    Vec8i sum1(0), sum2(0), sum3(0), sum4(0);
    Vec16s sad1(0), sad2(0), sad3(0), sad4(0);
    int max_iterators = (ly >> 4) << 4;
    int row;

    for (row = 0; row < max_iterators; row += 16)
    {
        for (int i = 0; i < 16; i++)
        {
            m1.load_a(fenc);
            n1.load(fref1);
            n2.load(fref2);
            n3.load(fref3);
            n4.load(fref4);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            fenc += FENC_STRIDE;
            fref1 += frefstride;
            fref2 += frefstride;
            fref3 += frefstride;
            fref4 += frefstride;
        }

        sum1 += extend_low(sad1) + extend_high(sad1);
        sum2 += extend_low(sad2) + extend_high(sad2);
        sum3 += extend_low(sad3) + extend_high(sad3);
        sum4 += extend_low(sad4) + extend_high(sad4);
        sad1 = 0;
        sad2 = 0;
        sad3 = 0;
        sad4 = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(fenc);
        n1.load(fref1);
        n2.load(fref2);
        n3.load(fref3);
        n4.load(fref4);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);
        sad4.addSumAbsDiff(m1, n4);

        fenc += FENC_STRIDE;
        fref1 += frefstride;
        fref2 += frefstride;
        fref3 += frefstride;
        fref4 += frefstride;
    }

    sum1 += extend_low(sad1) + extend_high(sad1);
    sum2 += extend_low(sad2) + extend_high(sad2);
    sum3 += extend_low(sad3) + extend_high(sad3);
    sum4 += extend_low(sad4) + extend_high(sad4);

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
    res[3] = horizontal_add(sum4);
}

template<int ly>
void sad_avx2_x4_64(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, pixel *fref4, intptr_t frefstride, int *res)
{
    Vec32uc m1, n1, n2, n3, n4;

    Vec8i sum1(0), sum2(0), sum3(0), sum4(0);
    Vec16s sad1(0), sad2(0), sad3(0), sad4(0);
    int max_iterators = (ly >> 3) << 3;
    int row;

    for (row = 0; row < max_iterators; row += 8)
    {
        for (int i = 0; i < 8; i++)
        {
            m1.load_a(fenc);
            n1.load(fref1);
            n2.load(fref2);
            n3.load(fref3);
            n4.load(fref4);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            m1.load_a(fenc + 32);
            n1.load(fref1 + 32);
            n2.load(fref2 + 32);
            n3.load(fref3 + 32);
            n4.load(fref4 + 32);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            fenc += FENC_STRIDE;
            fref1 += frefstride;
            fref2 += frefstride;
            fref3 += frefstride;
            fref4 += frefstride;
        }

        sum1 += extend_low(sad1) + extend_high(sad1);
        sum2 += extend_low(sad2) + extend_high(sad2);
        sum3 += extend_low(sad3) + extend_high(sad3);
        sum4 += extend_low(sad4) + extend_high(sad4);
        sad1 = 0;
        sad2 = 0;
        sad3 = 0;
        sad4 = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(fenc);
        n1.load(fref1);
        n2.load(fref2);
        n3.load(fref3);
        n4.load(fref4);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);
        sad4.addSumAbsDiff(m1, n4);

        m1.load_a(fenc + 32);
        n1.load(fref1 + 32);
        n2.load(fref2 + 32);
        n3.load(fref3 + 32);
        n4.load(fref4 + 32);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);
        sad4.addSumAbsDiff(m1, n4);

        fenc += FENC_STRIDE;
        fref1 += frefstride;
        fref2 += frefstride;
        fref3 += frefstride;
        fref4 += frefstride;
    }

    sum1 += extend_low(sad1) + extend_high(sad1);
    sum2 += extend_low(sad2) + extend_high(sad2);
    sum3 += extend_low(sad3) + extend_high(sad3);
    sum4 += extend_low(sad4) + extend_high(sad4);

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
    res[3] = horizontal_add(sum4);
}

#endif /* if INSTRSET >= X265_CPU_LEVEL_AVX2 */

#if INSTRSET >= X265_CPU_LEVEL_SSSE3
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

#endif /* if INSTRSET >= X265_CPU_LEVEL_SSSE3 */
