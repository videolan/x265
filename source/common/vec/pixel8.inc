/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Mandar Gurav <mandar@multicorewareinc.com>
 *          Mahesh Pittala <mahesh@multicorewareinc.com>
 *          Praveen Kumar Tiwari <praveen@multicorewareinc.com>
 *          Nabajit Deka <nabajit@multicorewareinc.com>
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

/* intrinsics for when pixel type is uint8_t */

#if defined(_MSC_VER)
#pragma warning(disable: 4100) // unused formal parameters
#endif

#if INSTRSET >= X265_CPU_LEVEL_SSE41
template<int ly>
int sad_48(pixel * fenc, intptr_t fencstride, pixel * fref, intptr_t frefstride)
{
    assert((ly % 4) == 0);

    __m128i sum0 = _mm_setzero_si128();
    __m128i sum1 = _mm_setzero_si128();

    if (ly == 4)
    {
        __m128i T00, T01, T02;
        __m128i T10, T11, T12;
        __m128i T20, T21, T22;

        T00 = _mm_load_si128((__m128i*)(fenc));           /*Loding 48 8-bit integer from fenc to local variables*/
        T01 = _mm_load_si128((__m128i*)(fenc + 16));
        T02 = _mm_load_si128((__m128i*)(fenc + 32));

        T10 = _mm_loadu_si128((__m128i*)(fref));          /*Loding 48 8-bit integer from fref to local variables*/
        T11 = _mm_loadu_si128((__m128i*)(fref + 16));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + (1) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (1) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (1) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (1) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (1) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + (2) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (2) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (2) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (2) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (2) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (2) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + (3) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (3) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (3) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (3) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (3) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
    }
    else if (ly == 8)
    {
        __m128i T00, T01, T02;
        __m128i T10, T11, T12;
        __m128i T20, T21, T22;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T01 = _mm_load_si128((__m128i*)(fenc + 16));
        T02 = _mm_load_si128((__m128i*)(fenc + 32));

        T10 = _mm_loadu_si128((__m128i*)(fref));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + (1) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (1) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (1) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (1) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (1) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + (2) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (2) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (2) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (2) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (2) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (2) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + (3) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (3) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (3) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (3) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (3) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + (4) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (4) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (4) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (4) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (4) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + (5) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (5) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (5) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (5) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (5) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + (6) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (6) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (6) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (6) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (6) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (6) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + (7) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (7) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (7) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (7) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (7) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
    }
    else if (ly == 16)
    {
        __m128i T00, T01, T02;
        __m128i T10, T11, T12;
        __m128i T20, T21, T22;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T01 = _mm_load_si128((__m128i*)(fenc + 16));
        T02 = _mm_load_si128((__m128i*)(fenc + 32));

        T10 = _mm_loadu_si128((__m128i*)(fref));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + (1) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (1) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (1) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (1) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (1) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + (2) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (2) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (2) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (2) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (2) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (2) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + (3) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (3) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (3) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (3) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (3) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + (4) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (4) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (4) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (4) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (4) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + (5) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (5) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (5) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (5) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (5) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + (6) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (6) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (6) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (6) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (6) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (6) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + (7) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (7) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (7) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (7) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (7) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + (8) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (8) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (8) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (8) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (8) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + (9) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (9) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (9) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (9) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (9) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + (10) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (10) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (10) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (10) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (10) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (10) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + (11) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (11) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (11) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (11) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (11) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + (12) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (12) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (12) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (12) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (12) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + (13) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (13) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (13) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (13) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (13) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + (14) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (14) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (14) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (14) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (14) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (14) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + (15) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (15) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (15) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (15) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (15) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
    }
    else if ((ly % 8) == 0)
    {
        /* for ly = 24, 32, 48, 64 */
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
    }
    else
    {
        for (int i = 0; i < ly; i += 4)
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

            sum0 = _mm_add_epi16(sum0, T20);
            sum0 = _mm_add_epi16(sum0, T21);
            sum0 = _mm_add_epi16(sum0, T22);

            T00 = _mm_load_si128((__m128i*)(fenc + (i + 1) * fencstride));
            T01 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 1) * fencstride));
            T02 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 1) * fencstride));

            T10 = _mm_loadu_si128((__m128i*)(fref + (i + 1) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (i + 1) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (i + 1) * frefstride));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);

            sum0 = _mm_add_epi16(sum0, T20);
            sum0 = _mm_add_epi16(sum0, T21);
            sum0 = _mm_add_epi16(sum0, T22);

            T00 = _mm_load_si128((__m128i*)(fenc + (i + 2) * fencstride));
            T01 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 2) * fencstride));
            T02 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 2) * fencstride));

            T10 = _mm_loadu_si128((__m128i*)(fref + (i + 2) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (i + 2) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (i + 2) * frefstride));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);

            sum0 = _mm_add_epi16(sum0, T20);
            sum0 = _mm_add_epi16(sum0, T21);
            sum0 = _mm_add_epi16(sum0, T22);

            T00 = _mm_load_si128((__m128i*)(fenc + (i + 3) * fencstride));
            T01 = _mm_load_si128((__m128i*)(fenc + 16 + (i + 3) * fencstride));
            T02 = _mm_load_si128((__m128i*)(fenc + 32 + (i + 3) * fencstride));

            T10 = _mm_loadu_si128((__m128i*)(fref + (i + 3) * frefstride));
            T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (i + 3) * frefstride));
            T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (i + 3) * frefstride));

            T20 = _mm_sad_epu8(T00, T10);
            T21 = _mm_sad_epu8(T01, T11);
            T22 = _mm_sad_epu8(T02, T12);

            sum0 = _mm_add_epi16(sum0, T20);
            sum0 = _mm_add_epi16(sum0, T21);
            sum0 = _mm_add_epi16(sum0, T22);
        }
    }
    sum1 = _mm_shuffle_epi32(sum0, 2);
    sum0 = _mm_add_epi32(sum0, sum1);
    return _mm_cvtsi128_si32(sum0);
}

#endif /* if INSTRSET >= X265_CPU_LEVEL_SSE41 */

#if INSTRSET >= X265_CPU_LEVEL_SSE41
template<int ly>
int sad_64(pixel * fenc, intptr_t fencstride, pixel * fref, intptr_t frefstride)
{
    assert((ly % 4) == 0);

    __m128i sum0 = _mm_setzero_si128();
    __m128i sum1 = _mm_setzero_si128();

    if (ly == 4)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;

        T00 = _mm_load_si128((__m128i*)(fenc));              /*Loding 64 8-bit integer from fenc to local variables*/
        T01 = _mm_load_si128((__m128i*)(fenc + 16));
        T02 = _mm_load_si128((__m128i*)(fenc + 32));
        T03 = _mm_load_si128((__m128i*)(fenc + 48));

        T10 = _mm_loadu_si128((__m128i*)(fref));              /*Loding 64 8-bit integer from fref to local variables*/
        T11 = _mm_loadu_si128((__m128i*)(fref + 16));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (1) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (1) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (1) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (1) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (1) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (1) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48 + (1) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (2) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (2) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (2) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (2) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (2) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (2) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48 + (2) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (3) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (3) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (3) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (3) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (3) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (3) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (3) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48 + (3) * frefstride));

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
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;

        T00 = _mm_load_si128((__m128i*)(fenc));
        T01 = _mm_load_si128((__m128i*)(fenc + 16));
        T02 = _mm_load_si128((__m128i*)(fenc + 32));
        T03 = _mm_load_si128((__m128i*)(fenc + 48));

        T10 = _mm_loadu_si128((__m128i*)(fref));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (1) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (1) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (1) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (1) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (1) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (1) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48 + (1) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (2) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (2) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (2) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (2) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (2) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (2) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48 + (2) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (3) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (3) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (3) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (3) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (3) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (3) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (3) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (4) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (4) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (4) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (4) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (4) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (4) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48 + (4) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (5) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (5) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (5) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (5) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (5) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (5) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48 + (5) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (6) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (6) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (6) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (6) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (6) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (6) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48 + (6) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (7) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (7) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (7) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (7) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (7) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (7) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (7) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48 + (7) * frefstride));

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
        T01 = _mm_load_si128((__m128i*)(fenc + 16));
        T02 = _mm_load_si128((__m128i*)(fenc + 32));
        T03 = _mm_load_si128((__m128i*)(fenc + 48));

        T10 = _mm_loadu_si128((__m128i*)(fref));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (1) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (1) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (1) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (1) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (1) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (1) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (1) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48 + (1) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (2) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (2) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (2) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (2) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (2) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (2) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48 + (2) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (3) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (3) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (3) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (3) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (3) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (3) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (3) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (4) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (4) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (4) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (4) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (4) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (4) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48 + (4) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (5) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (5) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (5) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (5) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (5) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (5) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48 + (5) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (6) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (6) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (6) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (6) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (6) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (6) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48 + (6) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (7) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (7) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (7) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (7) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (7) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (7) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (7) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (8) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (8) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (8) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (8) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (8) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (8) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48 + (8) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (9) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (9) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (9) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (9) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (9) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (9) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48 + (9) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (10) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (10) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (10) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (10) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (10) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (10) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48 + (10) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (11) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (11) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (11) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (11) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (11) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (11) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (11) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (12) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (12) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (12) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (12) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (12) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (12) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48 + (12) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (13) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (13) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (13) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (13) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (13) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (13) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48 + (13) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (14) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (14) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (14) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (14) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (14) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (14) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48 + (14) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        sum0 = _mm_add_epi16(sum0, T20);
        sum0 = _mm_add_epi16(sum0, T21);
        sum0 = _mm_add_epi16(sum0, T22);
        sum0 = _mm_add_epi16(sum0, T23);

        T00 = _mm_load_si128((__m128i*)(fenc + (15) * fencstride));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (15) * fencstride));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (15) * fencstride));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (15) * fencstride));

        T10 = _mm_loadu_si128((__m128i*)(fref + (15) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref + 16 + (15) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref + 32 + (15) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref + 48 + (15) * frefstride));

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
    }
    else
    {
        for (int i = 0; i < ly; i += 4)
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
        }
    }
    sum1 = _mm_shuffle_epi32(sum0, 2);
    sum0 = _mm_add_epi32(sum0, sum1);
    return _mm_cvtsi128_si32(sum0);
}

#endif /* if INSTRSET >= X265_CPU_LEVEL_SSE41 */

template<int ly>
void sad_x3_48(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, intptr_t frefstride, int *res)
{
    assert((ly % 4) == 0);

    __m128i sum0 = _mm_setzero_si128();
    __m128i sum1 = _mm_setzero_si128();
    __m128i sum2 = _mm_setzero_si128();
    __m128i sum3;

    if (ly == 4)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;
        /*Loding from offset 0*/
        T00 = _mm_load_si128((__m128i*)(fenc));               /*Loding 48 8-bit integer to Local variable*/
        T01 = _mm_load_si128((__m128i*)(fenc + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1));             /*Loding reference frame 1 to Local variable*/
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

        T10 = _mm_loadu_si128((__m128i*)(fref2));             /*Loding reference frame 2 to Local variable*/
        T11 = _mm_loadu_si128((__m128i*)(fref2 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum1 = _mm_add_epi16(T20, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3));             /*Loding reference frame 3 to Local variable*/
        T11 = _mm_loadu_si128((__m128i*)(fref3 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum2 = _mm_add_epi16(T20, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16));        /*Loding from offset 16*/
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32));        /*Loding from offset 32*/
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);
    }
    else if (ly == 8)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;

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
        sum1 = _mm_add_epi16(T20, T22);

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
        sum2 = _mm_add_epi16(T20, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32));
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16 + (4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (7) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32 + (4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + (5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (7) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);
    }
    else if (ly == 16)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;

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
        sum1 = _mm_add_epi16(T20, T22);

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
        sum2 = _mm_add_epi16(T20, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32));
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16 + (4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (7) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32 + (4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + (5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (7) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16 + (8) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (9) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (10) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (11) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32 + (8) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + (9) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (10) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (11) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16 + (12) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (13) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (14) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (15) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32 + (12) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + (13) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (14) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (15) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);
    }
    else if ((ly % 8) == 0)
    {
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
    }
    else
    {
        for (int i = 0; i < ly; i += 4)
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
        }
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
void sad_x3_64(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, intptr_t frefstride, int *res)
{
    assert((ly % 4) == 0);

    __m128i sum0 = _mm_setzero_si128();
    __m128i sum1 = _mm_setzero_si128();
    __m128i sum2 = _mm_setzero_si128();
    __m128i sum3;

    if (ly == 4)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;
        /*Loding for offset 0*/
        T00 = _mm_load_si128((__m128i*)(fenc));                 /*Loding 64 8-bit integer to Local variable*/
        T01 = _mm_load_si128((__m128i*)(fenc + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1));               /*Loding reference frame 1 to Local variable*/
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

        T10 = _mm_loadu_si128((__m128i*)(fref2));               /*Loding reference frame 2 to Local variable*/
        T11 = _mm_loadu_si128((__m128i*)(fref2 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum1 = _mm_add_epi16(T20, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3));               /*Loding reference frame 3 to Local varibale*/
        T11 = _mm_loadu_si128((__m128i*)(fref3 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum2 = _mm_add_epi16(T20, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16));          /*Loding from offset 16*/
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32));          /*Loding from offset 32*/
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 48));          /*Loding from offset 48*/
        T01 = _mm_load_si128((__m128i*)(fenc + 48 + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 48 + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 48));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 48 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 48));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 48 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 48));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 48 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);
    }
    else if (ly == 8)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;

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
        sum1 = _mm_add_epi16(T20, T22);

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
        sum2 = _mm_add_epi16(T20, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32));
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 48));
        T01 = _mm_load_si128((__m128i*)(fenc + 48 + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 48 + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 48));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 48 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 48));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 48 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 48));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 48 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16 + (4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (7) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32 + (4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + (5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (7) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 48 + (4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 48 + (5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 48 + (6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (7) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);
    }
    else if (ly == 16)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;

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
        sum1 = _mm_add_epi16(T20, T22);

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
        sum2 = _mm_add_epi16(T20, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32));
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 48));
        T01 = _mm_load_si128((__m128i*)(fenc + 48 + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 48 + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 48));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 48 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 48));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 48 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 48));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 48 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16 + (4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (7) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32 + (4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + (5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (7) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 48 + (4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 48 + (5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 48 + (6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (7) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16 + (8) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (9) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (10) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (11) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32 + (8) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + (9) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (10) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (11) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 48 + (8) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 48 + (9) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 48 + (10) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (11) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16 + (12) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (13) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (14) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (15) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32 + (12) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + (13) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (14) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (15) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 48 + (12) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 48 + (13) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 48 + (14) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (15) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);
    }
    else if ((ly % 8) == 0)
    {
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
    }
    else
    {
        for (int i = 0; i < ly; i += 4)
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
        }
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
void sad_x4_48(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, pixel *fref4, intptr_t frefstride, int *res)
{
    assert((ly % 4) == 0);

    __m128i sum0 = _mm_setzero_si128();
    __m128i sum1 = _mm_setzero_si128();
    __m128i sum2 = _mm_setzero_si128();
    __m128i sum3 = _mm_setzero_si128();
    __m128i sum4;
    if (ly == 4)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;
        /*Loding from offset 0 */
        T00 = _mm_load_si128((__m128i*)(fenc));               /*Loding 48 8-bit integer to Local variable*/
        T01 = _mm_load_si128((__m128i*)(fenc + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1));             /*Loding reference frame 1 to Local variable*/
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

        T10 = _mm_loadu_si128((__m128i*)(fref2));             /*Loding reference frame 2 to Local variable*/
        T11 = _mm_loadu_si128((__m128i*)(fref2 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum1 = _mm_add_epi16(T20, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3));             /*Loding reference frame 3 to Local variable*/
        T11 = _mm_loadu_si128((__m128i*)(fref3 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum2 = _mm_add_epi16(T20, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4));         /*Loding reference frame 4 to Local variable*/
        T11 = _mm_loadu_si128((__m128i*)(fref4 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum3 = _mm_add_epi16(T20, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16));        /*Loding from offset 16 */
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32));        /*Loding from offset 32 */
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);
    }
    else if (ly == 8)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;

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
        sum1 = _mm_add_epi16(T20, T22);

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
        sum2 = _mm_add_epi16(T20, T22);

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
        sum3 = _mm_add_epi16(T20, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32));
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16 + (4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (7) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32 + (4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + (5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (7) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);
    }
    else if (ly == 16)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;

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
        sum1 = _mm_add_epi16(T20, T22);

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
        sum2 = _mm_add_epi16(T20, T22);

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
        sum3 = _mm_add_epi16(T20, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32));
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16 + (4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (7) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32 + (4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + (5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (7) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16 + (8) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (9) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (10) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (11) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32 + (8) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + (9) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (10) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (11) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16 + (12) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (13) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (14) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (15) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32 + (12) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + (13) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (14) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (15) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);
    }
    else if ((ly % 8) == 0)
    {
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
    }
    else
    {
        for (int i = 0; i < ly; i += 4)
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
        }
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
void sad_x4_64(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, pixel *fref4, intptr_t frefstride, int *res)
{
    assert((ly % 4) == 0);

    __m128i sum0 = _mm_setzero_si128();
    __m128i sum1 = _mm_setzero_si128();
    __m128i sum2 = _mm_setzero_si128();
    __m128i sum3 = _mm_setzero_si128();
    __m128i sum4;

    if (ly == 4)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;
        /*Loding from offset 0 */
        T00 = _mm_load_si128((__m128i*)(fenc));               /*Loding 64 8-bit integer to Local variable*/
        T01 = _mm_load_si128((__m128i*)(fenc + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1));             /*Loding reference frame 1 to Local variable*/
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

        T10 = _mm_loadu_si128((__m128i*)(fref2));             /*Loding reference frame 2 to Local variable*/
        T11 = _mm_loadu_si128((__m128i*)(fref2 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum1 = _mm_add_epi16(T20, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3));             /*Loding reference frame 3 to Local variable*/
        T11 = _mm_loadu_si128((__m128i*)(fref3 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum2 = _mm_add_epi16(T20, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4));         /*Loding reference frame 4 to Local variable*/
        T11 = _mm_loadu_si128((__m128i*)(fref4 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        sum3 = _mm_add_epi16(T20, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16));        /*Loding from offset 16 */
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32));        /*Loding from offset 32 */
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 48));        /*Loding from offset 48 */
        T01 = _mm_load_si128((__m128i*)(fenc + 48 + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 48 + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 48));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 48 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 48));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 48 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 48));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 48 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 48));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 48 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 48 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 48 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);
    }
    else if (ly == 8)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;

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
        sum1 = _mm_add_epi16(T20, T22);

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
        sum2 = _mm_add_epi16(T20, T22);

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
        sum3 = _mm_add_epi16(T20, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32));
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 48));
        T01 = _mm_load_si128((__m128i*)(fenc + 48 + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 48 + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 48));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 48 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 48));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 48 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 48));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 48 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 48));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 48 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 48 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 48 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16 + (4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (7) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32 + (4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + (5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (7) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 48 + (4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 48 + (5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 48 + (6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (7) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 48 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 48 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 48 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 48 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);
    }
    else if (ly == 16)
    {
        __m128i T00, T01, T02, T03;
        __m128i T10, T11, T12, T13;
        __m128i T20, T21, T22, T23;

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
        sum1 = _mm_add_epi16(T20, T22);

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
        sum2 = _mm_add_epi16(T20, T22);

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
        sum3 = _mm_add_epi16(T20, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 16));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 16 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32));
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 32));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 32 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 48));
        T01 = _mm_load_si128((__m128i*)(fenc + 48 + FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 48 + (2) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (3) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 48));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 48 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 48));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 48 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 48));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 48 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 48));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 48 + frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 48 + (2) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 48 + (3) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16 + (4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (7) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32 + (4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + (5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (7) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 48 + (4) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 48 + (5) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 48 + (6) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (7) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 48 + (4) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 48 + (5) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 48 + (6) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 48 + (7) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16 + (8) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (9) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (10) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (11) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32 + (8) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + (9) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (10) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (11) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 48 + (8) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 48 + (9) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 48 + (10) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (11) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 48 + (8) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 48 + (9) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 48 + (10) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 48 + (11) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

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
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 16 + (12) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 16 + (13) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 16 + (14) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 16 + (15) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 16 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 16 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 16 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 16 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 32 + (12) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 32 + (13) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 32 + (14) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 32 + (15) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 32 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 32 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 32 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 32 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);

        T00 = _mm_load_si128((__m128i*)(fenc + 48 + (12) * FENC_STRIDE));
        T01 = _mm_load_si128((__m128i*)(fenc + 48 + (13) * FENC_STRIDE));
        T02 = _mm_load_si128((__m128i*)(fenc + 48 + (14) * FENC_STRIDE));
        T03 = _mm_load_si128((__m128i*)(fenc + 48 + (15) * FENC_STRIDE));

        T10 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref1 + 48 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum0 = _mm_add_epi32(sum0, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref2 + 48 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum1 = _mm_add_epi32(sum1, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref3 + 48 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum2 = _mm_add_epi32(sum2, T22);

        T10 = _mm_loadu_si128((__m128i*)(fref4 + 48 + (12) * frefstride));
        T11 = _mm_loadu_si128((__m128i*)(fref4 + 48 + (13) * frefstride));
        T12 = _mm_loadu_si128((__m128i*)(fref4 + 48 + (14) * frefstride));
        T13 = _mm_loadu_si128((__m128i*)(fref4 + 48 + (15) * frefstride));

        T20 = _mm_sad_epu8(T00, T10);
        T21 = _mm_sad_epu8(T01, T11);
        T22 = _mm_sad_epu8(T02, T12);
        T23 = _mm_sad_epu8(T03, T13);

        T20 = _mm_add_epi16(T20, T21);
        T22 = _mm_add_epi16(T22, T23);
        T22 = _mm_add_epi16(T20, T22);
        sum3 = _mm_add_epi32(sum3, T22);
    }
    else if ((ly % 8) == 0)
    {
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
    }
    else
    {
        for (int i = 0; i < ly; i += 4)
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
        }
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

template <typename T>
void weightUnidir(T *src, pixel *dst, intptr_t srcStride, intptr_t dstStride, int width, int height, int w0, int round, int shift, int offset)
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
