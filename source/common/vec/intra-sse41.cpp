/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Min Chen <chenm003@163.com>
 *          Deepthi Devaki <deepthidevaki@multicorewareinc.com>
 *          Steve Borho <steve@borho.org>
 *          ShinYee Chung <shinyee@multicorewareinc.com>
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
#include <smmintrin.h>

using namespace x265;

namespace {
#if !HIGH_BIT_DEPTH
// TODO: Remove unused table and merge here
ALIGN_VAR_32(static const unsigned char, tab_angle_2[][16]) =
{
    { 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 },       //  0
};

ALIGN_VAR_32(static const char, tab_angle_1[][16]) =
{
#define MAKE_COEF8(a) \
    { 32 - (a), (a), 32 - (a), (a), 32 - (a), (a), 32 - (a), (a), 32 - (a), (a), 32 - (a), (a), 32 - (a), (a), 32 - (a), (a) \
    },

    MAKE_COEF8(0)
    MAKE_COEF8(1)
    MAKE_COEF8(2)
    MAKE_COEF8(3)
    MAKE_COEF8(4)
    MAKE_COEF8(5)
    MAKE_COEF8(6)
    MAKE_COEF8(7)
    MAKE_COEF8(8)
    MAKE_COEF8(9)
    MAKE_COEF8(10)
    MAKE_COEF8(11)
    MAKE_COEF8(12)
    MAKE_COEF8(13)
    MAKE_COEF8(14)
    MAKE_COEF8(15)
    MAKE_COEF8(16)
    MAKE_COEF8(17)
    MAKE_COEF8(18)
    MAKE_COEF8(19)
    MAKE_COEF8(20)
    MAKE_COEF8(21)
    MAKE_COEF8(22)
    MAKE_COEF8(23)
    MAKE_COEF8(24)
    MAKE_COEF8(25)
    MAKE_COEF8(26)
    MAKE_COEF8(27)
    MAKE_COEF8(28)
    MAKE_COEF8(29)
    MAKE_COEF8(30)
    MAKE_COEF8(31)

#undef MAKE_COEF8
};

#if !defined(__clang__)
// See doc/intra/T32.TXT for algorithm details
void predIntraAngs32(pixel *dst0, pixel *above0, pixel *left0, pixel *above1, pixel *left1, bool /*bLuma*/)
{
#define N   (32)
    pixel(*dstN)[N * N] = (pixel(*)[N * N])dst0;
    const __m128i c_16 = _mm_set1_epi16(16);
    __m128i T00A, T00B;
    __m128i T01A, T01B;
    __m128i T20A, T20B;
    __m128i T30A, T30B;

    T00A = _mm_loadu_si128((__m128i*)(left0 +   1));
    T00B = _mm_loadu_si128((__m128i*)(left0 +  17));
    T01A = _mm_loadu_si128((__m128i*)(above0 +  1));
    T01B = _mm_loadu_si128((__m128i*)(above0 + 17));

    for (int i = 0; i < N; i++)
    {
        _mm_store_si128((__m128i*)&dstN[8][i * N], T00A);
        _mm_store_si128((__m128i*)&dstN[8][i * N + 16], T00B);
        _mm_store_si128((__m128i*)&dstN[24][i * N], T01A);
        _mm_store_si128((__m128i*)&dstN[24][i * N + 16], T01B);
    }

#define HALF(m, n, o, a, b, t, d) { \
        __m128i T10, T12; \
        T10 = _mm_maddubs_epi16(T0 ## a, _mm_load_si128((__m128i*)tab_angle_1[(t)])); \
        T12 = _mm_maddubs_epi16(T0 ## b, _mm_load_si128((__m128i*)tab_angle_1[(t)])); \
        T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5); \
        T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5); \
        d   = _mm_unpacklo_epi16(T10, T12); \
        T10 = _mm_unpackhi_epi16(T10, T12); \
        d = _mm_packus_epi16(d, T10); \
        _mm_store_si128((__m128i*)&dstN[(m) - 2][(n) * N + (o) * 16], d); \
}

#define LINE(m, n, a, b, t) { \
        HALF(m, n, 0, a ## A, b ## A, t, T20A) \
        HALF(m, n, 1, a ## B, b ## B, t, T20B) \
}

#define COPY(m, n) { \
        _mm_store_si128((__m128i*)&dstN[(m) - 2][(n) * N + 0 * 16], T20A); \
        _mm_store_si128((__m128i*)&dstN[(m) - 2][(n) * N + 1 * 16], T20B); \
}

#define AVGS(m, n) { \
        _mm_store_si128((__m128i*)&dstN[(m) - 2][(n) * N + 0 * 16], _mm_avg_epu8(T00A, T01A)); \
        _mm_store_si128((__m128i*)&dstN[(m) - 2][(n) * N + 1 * 16], _mm_avg_epu8(T00B, T01B)); \
}

    // ***********************************************************************
    T00A = _mm_loadu_si128((__m128i*)(left1 +  1));
    T00B = _mm_loadu_si128((__m128i*)(left1 + 17));
    T01A = _mm_loadu_si128((__m128i*)(left1 +  2));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 18));

    _mm_store_si128((__m128i*)&dstN[2 - 2][0 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][0 * N + 1 * 16], T01B);
    _mm_store_si128((__m128i*)&dstN[9 - 2][15 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[9 - 2][15 * N + 1 * 16], T01B);
    AVGS(9,  7);
    LINE(9,  0, 0, 1,  2);
    LINE(9,  1, 0, 1,  4);
    LINE(9,  2, 0, 1,  6);
    LINE(9,  3, 0, 1,  8);
    LINE(9,  4, 0, 1, 10);
    COPY(8,  1);
    LINE(9,  5, 0, 1, 12);
    LINE(9,  6, 0, 1, 14);
    LINE(9,  8, 0, 1, 18);
    COPY(7,  1);
    LINE(9,  9, 0, 1, 20);
    COPY(8,  3);
    LINE(9, 10, 0, 1, 22);
    LINE(9, 11, 0, 1, 24);
    LINE(9, 12, 0, 1, 26);
    COPY(3,  0);
    COPY(6,  1);
    LINE(9, 13, 0, 1, 28);
    LINE(9, 14, 0, 1, 30);
    COPY(8,  5);
    LINE(4,  0, 0, 1, 21);
    LINE(5,  0, 0, 1, 17);
    LINE(6,  0, 0, 1, 13);
    LINE(7,  0, 0, 1,  9);
    LINE(7,  2, 0, 1, 27);
    LINE(8,  0, 0, 1,  5);
    LINE(8,  2, 0, 1, 15);
    LINE(8,  4, 0, 1, 25);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 +  3));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 19));

    _mm_store_si128((__m128i*)&dstN[2 - 2][1 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][1 * N + 1 * 16], T01B);
    _mm_store_si128((__m128i*)&dstN[9 - 2][31 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[9 - 2][31 * N + 1 * 16], T01B);
    AVGS(9, 23);
    LINE(9, 16, 0, 1,  2);
    COPY(5,  1);
    LINE(9, 17, 0, 1,  4);
    COPY(7,  3);
    LINE(9, 18, 0, 1,  6);
    LINE(9, 19, 0, 1,  8);
    COPY(8,  7);
    LINE(9, 20, 0, 1, 10);
    COPY(4,  1);
    LINE(9, 21, 0, 1, 12);
    LINE(9, 22, 0, 1, 14);
    LINE(9, 24, 0, 1, 18);
    COPY(8,  9);
    LINE(9, 25, 0, 1, 20);
    COPY(3,  1);
    COPY(6,  3);
    LINE(9, 26, 0, 1, 22);
    COPY(7,  5);
    LINE(9, 27, 0, 1, 24);
    LINE(9, 28, 0, 1, 26);
    LINE(9, 29, 0, 1, 28);
    COPY(8, 11);
    LINE(9, 30, 0, 1, 30);
    LINE(8,  6, 0, 1,  3);
    LINE(8,  8, 0, 1, 13);
    COPY(7,  4);
    LINE(8, 10, 0, 1, 23);
    LINE(7,  6, 0, 1, 31);
    COPY(4,  2);
    LINE(6,  2, 0, 1,  7);
    LINE(5,  2, 0, 1, 19);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 +  4));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 20));

    _mm_store_si128((__m128i*)&dstN[2 - 2][2 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][2 * N + 1 * 16], T01B);
    AVGS(8, 15);
    LINE(8, 12, 0, 1,  1);
    COPY(6,  4);
    LINE(8, 13, 0, 1,  6);
    LINE(8, 14, 0, 1, 11);
    LINE(8, 16, 0, 1, 21);
    COPY(5,  4);
    LINE(8, 17, 0, 1, 26);
    COPY(7,  9);
    LINE(8, 18, 0, 1, 31);
    LINE(4,  3, 0, 1, 20);
    LINE(5,  3, 0, 1,  4);
    LINE(6,  5, 0, 1, 14);
    COPY(3,  2);
    LINE(6,  6, 0, 1, 27);
    LINE(7,  7, 0, 1,  8);
    LINE(7,  8, 0, 1, 17);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 +  5));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 21));

    _mm_store_si128((__m128i*)&dstN[2 - 2][3 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][3 * N + 1 * 16], T01B);
    LINE(8, 19, 0, 1,  4);
    LINE(8, 20, 0, 1,  9);
    COPY(4,  4);
    LINE(8, 21, 0, 1, 14);
    LINE(8, 22, 0, 1, 19);
    LINE(8, 23, 0, 1, 24);
    LINE(8, 24, 0, 1, 29);
    LINE(7, 10, 0, 1,  3);
    LINE(7, 11, 0, 1, 12);
    LINE(7, 12, 0, 1, 21);
    COPY(6,  8);
    LINE(7, 13, 0, 1, 30);
    COPY(4,  5);
    LINE(6,  7, 0, 1,  8);
    COPY(3,  3);
    LINE(5,  5, 0, 1,  6);
    LINE(5,  6, 0, 1, 23);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 +  6));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 22));

    _mm_store_si128((__m128i*)&dstN[2 - 2][4 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][4 * N + 1 * 16], T01B);
    _mm_store_si128((__m128i*)&dstN[8 - 2][31 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[8 - 2][31 * N + 1 * 16], T01B);
    AVGS(7, 15);
    LINE(8, 25, 0, 1,  2);
    COPY(3,  4);
    LINE(8, 26, 0, 1,  7);
    COPY(7, 14);
    LINE(8, 27, 0, 1, 12);
    LINE(8, 28, 0, 1, 17);
    LINE(8, 29, 0, 1, 22);
    LINE(8, 30, 0, 1, 27);
    LINE(7, 16, 0, 1, 25);
    COPY(5,  8);
    LINE(6,  9, 0, 1,  2);
    LINE(6, 10, 0, 1, 15);
    LINE(6, 11, 0, 1, 28);
    COPY(3,  5);
    LINE(5,  7, 0, 1,  8);
    LINE(4,  6, 0, 1, 19);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 +  7));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 23));

    _mm_store_si128((__m128i*)&dstN[2 - 2][5 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][5 * N + 1 * 16], T01B);
    LINE(4,  7, 0, 1,  8);
    LINE(5,  9, 0, 1, 10);
    LINE(5, 10, 0, 1, 27);
    LINE(6, 12, 0, 1,  9);
    LINE(6, 13, 0, 1, 22);
    COPY(3,  6);
    LINE(7, 17, 0, 1,  2);
    LINE(7, 18, 0, 1, 11);
    LINE(7, 19, 0, 1, 20);
    LINE(7, 20, 0, 1, 29);
    COPY(4,  8);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 +  8));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 24));

    _mm_store_si128((__m128i*)&dstN[2 - 2][6 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][6 * N + 1 * 16], T01B);
    AVGS(6, 15);
    AVGS(3,  7);
    LINE(4,  9, 0, 1, 18);
    LINE(5, 11, 0, 1, 12);
    LINE(6, 14, 0, 1,  3);
    LINE(6, 16, 0, 1, 29);
    COPY(5, 12);
    LINE(7, 21, 0, 1,  6);
    LINE(7, 22, 0, 1, 15);
    LINE(7, 23, 0, 1, 24);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 +  9));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 25));

    _mm_store_si128((__m128i*)&dstN[2 - 2][7 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][7 * N + 1 * 16], T01B);
    LINE(7, 24, 0, 1,  1);
    LINE(7, 25, 0, 1, 10);
    COPY(6, 17);
    COPY(3,  8);
    LINE(7, 26, 0, 1, 19);
    LINE(7, 27, 0, 1, 28);
    COPY(4, 11);
    LINE(6, 18, 0, 1, 23);
    LINE(5, 13, 0, 1, 14);
    LINE(5, 14, 0, 1, 31);
    LINE(4, 10, 0, 1,  7);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 + 10));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 26));

    _mm_store_si128((__m128i*)&dstN[2 - 2][8 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][8 * N + 1 * 16], T01B);
    _mm_store_si128((__m128i*)&dstN[7 - 2][31 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[7 - 2][31 * N + 1 * 16], T01B);
    LINE(7, 28, 0, 1,  5);
    LINE(7, 29, 0, 1, 14);
    LINE(7, 30, 0, 1, 23);
    LINE(6, 19, 0, 1,  4);
    COPY(3,  9);
    LINE(6, 20, 0, 1, 17);
    COPY(4, 12);
    LINE(6, 21, 0, 1, 30);
    COPY(3, 10);
    AVGS(5, 15);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 + 11));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 27));

    _mm_store_si128((__m128i*)&dstN[2 - 2][9 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][9 * N + 1 * 16], T01B);
    LINE(4, 13, 0, 1,  6);
    LINE(4, 14, 0, 1, 27);
    LINE(5, 16, 0, 1,  1);
    LINE(5, 17, 0, 1, 18);
    LINE(6, 22, 0, 1, 11);
    LINE(6, 23, 0, 1, 24);
    COPY(3, 11);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 + 12));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 28));

    _mm_store_si128((__m128i*)&dstN[2 - 2][10 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][10 * N + 1 * 16], T01B);
    AVGS(4, 15);
    LINE(5, 18, 0, 1,  3);
    LINE(5, 19, 0, 1, 20);
    LINE(6, 24, 0, 1,  5);
    LINE(6, 25, 0, 1, 18);
    COPY(3, 12);
    LINE(6, 26, 0, 1, 31);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 + 13));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 29));

    _mm_store_si128((__m128i*)&dstN[2 - 2][11 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][11 * N + 1 * 16], T01B);
    LINE(5, 20, 0, 1,  5);
    COPY(4, 16);
    LINE(5, 21, 0, 1, 22);
    LINE(6, 27, 0, 1, 12);
    COPY(3, 13);
    LINE(6, 28, 0, 1, 25);
    LINE(4, 17, 0, 1, 26);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 + 14));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 30));

    _mm_store_si128((__m128i*)&dstN[2 - 2][12 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][12 * N + 1 * 16], T01B);
    _mm_store_si128((__m128i*)&dstN[3 - 2][15 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[3 - 2][15 * N + 1 * 16], T01B);
    _mm_store_si128((__m128i*)&dstN[6 - 2][31 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[6 - 2][31 * N + 1 * 16], T01B);
    LINE(3, 14, 0, 1,  6);
    COPY(6, 29);
    LINE(4, 18, 0, 1, 15);
    LINE(5, 22, 0, 1,  7);
    LINE(5, 23, 0, 1, 24);
    LINE(6, 30, 0, 1, 19);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 + 15));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 31));

    _mm_store_si128((__m128i*)&dstN[2 - 2][13 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][13 * N + 1 * 16], T01B);
    LINE(3, 16, 0, 1, 26);
    COPY(5, 25);
    LINE(4, 19, 0, 1,  4);
    LINE(4, 20, 0, 1, 25);
    LINE(5, 24, 0, 1,  9);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 + 16));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 32));

    _mm_store_si128((__m128i*)&dstN[2 - 2][14 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][14 * N + 1 * 16], T01B);
    LINE(3, 17, 0, 1, 20);
    LINE(4, 21, 0, 1, 14);
    LINE(5, 26, 0, 1, 11);
    LINE(5, 27, 0, 1, 28);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 + 17));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 33));

    _mm_store_si128((__m128i*)&dstN[2 - 2][15 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][15 * N + 1 * 16], T01B);
    LINE(3, 18, 0, 1, 14);
    LINE(4, 22, 0, 1,  3);
    LINE(4, 23, 0, 1, 24);
    LINE(5, 28, 0, 1, 13);
    LINE(5, 29, 0, 1, 30);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 + 18));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 34));

    _mm_store_si128((__m128i*)&dstN[2 - 2][16 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][16 * N + 1 * 16], T01B);
    _mm_store_si128((__m128i*)&dstN[5 - 2][31 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[5 - 2][31 * N + 1 * 16], T01B);
    LINE(3, 19, 0, 1,  8);
    LINE(4, 24, 0, 1, 13);
    LINE(5, 30, 0, 1, 15);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 + 19));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 35));

    _mm_store_si128((__m128i*)&dstN[2 - 2][17 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][17 * N + 1 * 16], T01B);
    LINE(3, 20, 0, 1,  2);
    COPY(4, 25);
    LINE(3, 21, 0, 1, 28);
    LINE(4, 26, 0, 1, 23);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 + 20));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 36));

    _mm_store_si128((__m128i*)&dstN[2 - 2][18 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][18 * N + 1 * 16], T01B);
    LINE(3, 22, 0, 1, 22);
    LINE(4, 27, 0, 1, 12);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 + 21));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 37));

    _mm_store_si128((__m128i*)&dstN[2 - 2][19 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][19 * N + 1 * 16], T01B);
    AVGS(3, 23);
    LINE(4, 28, 0, 1,  1);
    LINE(4, 29, 0, 1, 22);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 + 22));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 38));

    _mm_store_si128((__m128i*)&dstN[2 - 2][20 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][20 * N + 1 * 16], T01B);
    _mm_store_si128((__m128i*)&dstN[4 - 2][31 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[4 - 2][31 * N + 1 * 16], T01B);
    LINE(3, 24, 0, 1, 10);
    LINE(4, 30, 0, 1, 11);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 + 23));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 39));

    _mm_store_si128((__m128i*)&dstN[2 - 2][21 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][21 * N + 1 * 16], T01B);
    LINE(3, 25, 0, 1,  4);
    LINE(3, 26, 0, 1, 30);

    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 + 24));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 40));

    _mm_store_si128((__m128i*)&dstN[2 - 2][22 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][22 * N + 1 * 16], T01B);
    LINE(3, 27, 0, 1, 24);

    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 + 25));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 41));

    _mm_store_si128((__m128i*)&dstN[2 - 2][23 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][23 * N + 1 * 16], T01B);
    LINE(3, 28, 0, 1, 18);

    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 + 26));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 42));

    _mm_store_si128((__m128i*)&dstN[2 - 2][24 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][24 * N + 1 * 16], T01B);
    LINE(3, 29, 0, 1, 12);

    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 + 27));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 43));

    _mm_store_si128((__m128i*)&dstN[2 - 2][25 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][25 * N + 1 * 16], T01B);
    _mm_store_si128((__m128i*)&dstN[3 - 2][31 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[3 - 2][31 * N + 1 * 16], T01B);
    LINE(3, 30, 0, 1,  6);

    T01A = _mm_loadu_si128((__m128i*)(left1 + 28));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 44));
    _mm_store_si128((__m128i*)&dstN[2 - 2][26 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][26 * N + 1 * 16], T01B);

    T01A = _mm_loadu_si128((__m128i*)(left1 + 29));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 45));
    _mm_store_si128((__m128i*)&dstN[2 - 2][27 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][27 * N + 1 * 16], T01B);

    T01A = _mm_loadu_si128((__m128i*)(left1 + 30));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 46));
    _mm_store_si128((__m128i*)&dstN[2 - 2][28 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][28 * N + 1 * 16], T01B);

    T01A = _mm_loadu_si128((__m128i*)(left1 + 31));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 47));
    _mm_store_si128((__m128i*)&dstN[2 - 2][29 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][29 * N + 1 * 16], T01B);

    T01A = _mm_loadu_si128((__m128i*)(left1 + 32));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 48));
    _mm_store_si128((__m128i*)&dstN[2 - 2][30 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][30 * N + 1 * 16], T01B);

    T01A = _mm_loadu_si128((__m128i*)(left1 + 33));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 49));
    _mm_store_si128((__m128i*)&dstN[2 - 2][31 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][31 * N + 1 * 16], T01B);

    // ***********************************************************************
    T00A = _mm_loadu_si128((__m128i*)(left1 +  0));
    T00B = _mm_loadu_si128((__m128i*)(left1 + 16));
    T01A = _mm_loadu_si128((__m128i*)(left1 +  1));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 17));

    _mm_store_si128((__m128i*)&dstN[11 - 2][15 * N + 0 * 16], T00A);
    _mm_store_si128((__m128i*)&dstN[11 - 2][15 * N + 1 * 16], T00B);
    LINE(11,  0, 0, 1, 30);
    LINE(11,  1, 0, 1, 28);
    LINE(11,  2, 0, 1, 26);
    LINE(11,  3, 0, 1, 24);
    LINE(11,  4, 0, 1, 22);
    COPY(12,  1);
    LINE(11,  5, 0, 1, 20);
    LINE(11,  6, 0, 1, 18);
    AVGS(11,  7);
    LINE(11,  8, 0, 1, 14);
    COPY(13,  1);
    LINE(11,  9, 0, 1, 12);
    COPY(12,  3);
    LINE(11, 10, 0, 1, 10);
    LINE(11, 11, 0, 1,  8);
    LINE(11, 12, 0, 1,  6);
    COPY(14,  1);
    COPY(17,  0);
    LINE(11, 13, 0, 1,  4);
    LINE(11, 14, 0, 1,  2);
    COPY(12,  5);
    LINE(12,  0, 0, 1, 27);
    LINE(12,  2, 0, 1, 17);
    LINE(12,  4, 0, 1,  7);
    LINE(13,  0, 0, 1, 23);
    LINE(13,  2, 0, 1,  5);
    LINE(14,  0, 0, 1, 19);
    LINE(15,  0, 0, 1, 15);
    LINE(16,  0, 0, 1, 11);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T30A = _mm_slli_si128(T00A, 1);
    T30B = T00B;
    T00A = _mm_insert_epi8(T30A, above1[16], 0);

    _mm_store_si128((__m128i*)&dstN[11 - 2][31 * N + 0 * 16], T00A);
    _mm_store_si128((__m128i*)&dstN[11 - 2][31 * N + 1 * 16], T00B);
    LINE(11, 16, 0, 1, 30);
    LINE(11, 17, 0, 1, 28);
    LINE(11, 18, 0, 1, 26);
    LINE(11, 19, 0, 1, 24);
    LINE(11, 20, 0, 1, 22);
    LINE(11, 21, 0, 1, 20);
    LINE(11, 22, 0, 1, 18);
    AVGS(11, 23);
    LINE(11, 24, 0, 1, 14);
    LINE(11, 25, 0, 1, 12);
    LINE(11, 26, 0, 1, 10);
    LINE(11, 27, 0, 1,  8);
    LINE(11, 28, 0, 1,  6);
    LINE(11, 29, 0, 1,  4);
    LINE(11, 30, 0, 1,  2);

    T00A = _mm_insert_epi8(T30A, above1[6], 0);

    LINE(12,  6, 0, 1, 29);
    LINE(12,  7, 0, 1, 24);
    LINE(12,  8, 0, 1, 19);
    LINE(12,  9, 0, 1, 14);
    LINE(12, 10, 0, 1,  9);
    LINE(12, 11, 0, 1,  4);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[13], 0);

    LINE(12, 12, 0, 1, 31);
    LINE(12, 13, 0, 1, 26);
    LINE(12, 14, 0, 1, 21);
    AVGS(12, 15);
    LINE(12, 16, 0, 1, 11);
    LINE(12, 17, 0, 1,  6);
    LINE(12, 18, 0, 1,  1);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[19], 0);

    LINE(12, 19, 0, 1, 28);
    LINE(12, 20, 0, 1, 23);
    LINE(12, 21, 0, 1, 18);
    LINE(12, 22, 0, 1, 13);
    LINE(12, 23, 0, 1,  8);
    LINE(12, 24, 0, 1,  3);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[26], 0);

    _mm_store_si128((__m128i*)&dstN[12 - 2][31 * N + 0 * 16], T00A);
    _mm_store_si128((__m128i*)&dstN[12 - 2][31 * N + 1 * 16], T00B);
    LINE(12, 25, 0, 1, 30);
    LINE(12, 26, 0, 1, 25);
    LINE(12, 27, 0, 1, 20);
    LINE(12, 28, 0, 1, 15);
    LINE(12, 29, 0, 1, 10);
    LINE(12, 30, 0, 1,  5);

    // ***********************************************************************
    T00A = _mm_insert_epi8(T30A, above1[4], 0);
    T00B = T30B;
    T01A = _mm_loadu_si128((__m128i*)(left1 +  0));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 16));

    LINE(13,  3, 0, 1, 28);
    LINE(13,  4, 0, 1, 19);
    LINE(13,  5, 0, 1, 10);
    LINE(13,  6, 0, 1,  1);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[7], 0);

    LINE(13,  7, 0, 1, 24);
    LINE(13,  8, 0, 1, 15);
    LINE(13,  9, 0, 1,  6);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[11], 0);

    LINE(13, 10, 0, 1, 29);
    LINE(13, 11, 0, 1, 20);
    LINE(13, 12, 0, 1, 11);
    LINE(13, 13, 0, 1,  2);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[14], 0);

    LINE(13, 14, 0, 1, 25);
    AVGS(13, 15);
    LINE(13, 16, 0, 1,  7);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[18], 0);

    LINE(13, 17, 0, 1, 30);
    LINE(13, 18, 0, 1, 21);
    LINE(13, 19, 0, 1, 12);
    LINE(13, 20, 0, 1,  3);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[21], 0);

    LINE(13, 21, 0, 1, 26);
    LINE(13, 22, 0, 1, 17);
    LINE(13, 23, 0, 1,  8);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[25], 0);

    LINE(13, 24, 0, 1, 31);
    LINE(13, 25, 0, 1, 22);
    LINE(13, 26, 0, 1, 13);
    LINE(13, 27, 0, 1,  4);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[28], 0);

    _mm_store_si128((__m128i*)&dstN[13 - 2][31 * N + 0 * 16], T00A);
    _mm_store_si128((__m128i*)&dstN[13 - 2][31 * N + 1 * 16], T00B);
    LINE(13, 28, 0, 1, 27);
    LINE(13, 29, 0, 1, 18);
    LINE(13, 30, 0, 1,  9);

    // ***********************************************************************
    T00A = _mm_insert_epi8(T30A, above1[2], 0);
    T00B = T30B;
    T01A = _mm_loadu_si128((__m128i*)(left1 +  0));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 16));
    T30A = T00A;
    T30B = T00B;

    LINE(14,  2, 0, 1, 25);
    LINE(14,  3, 0, 1, 12);
    LINE(15,  1, 0, 1, 30);
    LINE(15,  2, 0, 1, 13);
    LINE(16,  1, 0, 1, 22);
    LINE(16,  2, 0, 1,  1);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[5], 0);

    LINE(14,  4, 0, 1, 31);
    LINE(14,  5, 0, 1, 18);
    LINE(14,  6, 0, 1,  5);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[7], 0);

    LINE(14,  7, 0, 1, 24);
    LINE(14,  8, 0, 1, 11);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[10], 0);

    LINE(14,  9, 0, 1, 30);
    LINE(14, 10, 0, 1, 17);
    LINE(14, 11, 0, 1,  4);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[12], 0);

    LINE(14, 12, 0, 1, 23);
    LINE(14, 13, 0, 1, 10);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[15], 0);

    LINE(14, 14, 0, 1, 29);
    LINE(14, 15, 0, 1, 16);
    LINE(14, 16, 0, 1,  3);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[17], 0);

    LINE(14, 17, 0, 1, 22);
    LINE(14, 18, 0, 1,  9);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[20], 0);

    LINE(14, 19, 0, 1, 28);
    LINE(14, 20, 0, 1, 15);
    LINE(14, 21, 0, 1,  2);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[22], 0);

    LINE(14, 22, 0, 1, 21);
    LINE(14, 23, 0, 1,  8);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[25], 0);

    LINE(14, 24, 0, 1, 27);
    LINE(14, 25, 0, 1, 14);
    LINE(14, 26, 0, 1,  1);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[27], 0);

    LINE(14, 27, 0, 1, 20);
    LINE(14, 28, 0, 1,  7);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[30], 0);

    _mm_store_si128((__m128i*)&dstN[14 - 2][31 * N + 0 * 16], T00A);
    _mm_store_si128((__m128i*)&dstN[14 - 2][31 * N + 1 * 16], T00B);
    LINE(14, 29, 0, 1, 26);
    LINE(14, 30, 0, 1, 13);

    // ***********************************************************************
    T01A = T30A;
    T01B = T30B;
    T00B = _mm_alignr_epi8(T30B, T30A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T30A, 1), above1[4], 0);

    LINE(15,  3, 0, 1, 28);
    LINE(15,  4, 0, 1, 11);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[6], 0);

    LINE(15,  5, 0, 1, 26);
    LINE(15,  6, 0, 1,  9);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[8], 0);

    LINE(15,  7, 0, 1, 24);
    LINE(15,  8, 0, 1,  7);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[9], 0);

    LINE(15,  9, 0, 1, 22);
    LINE(15, 10, 0, 1,  5);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[11], 0);

    LINE(15, 11, 0, 1, 20);
    LINE(15, 12, 0, 1,  3);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[13], 0);

    LINE(15, 13, 0, 1, 18);
    LINE(15, 14, 0, 1,  1);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[15], 0);

    AVGS(15, 15);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[17], 0);

    LINE(15, 16, 0, 1, 31);
    LINE(15, 17, 0, 1, 14);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[19], 0);

    LINE(15, 18, 0, 1, 29);
    LINE(15, 19, 0, 1, 12);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[21], 0);

    LINE(15, 20, 0, 1, 27);
    LINE(15, 21, 0, 1, 10);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[23], 0);

    LINE(15, 22, 0, 1, 25);
    LINE(15, 23, 0, 1,  8);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[24], 0);

    LINE(15, 24, 0, 1, 23);
    LINE(15, 25, 0, 1,  6);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[26], 0);

    LINE(15, 26, 0, 1, 21);
    LINE(15, 27, 0, 1,  4);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[28], 0);

    LINE(15, 28, 0, 1, 19);
    LINE(15, 29, 0, 1,  2);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[30], 0);

    _mm_store_si128((__m128i*)&dstN[15 - 2][31 * N + 0 * 16], T00A);
    _mm_store_si128((__m128i*)&dstN[15 - 2][31 * N + 1 * 16], T00B);
    LINE(15, 30, 0, 1, 17);

    // ***********************************************************************
    T01A = T30A;
    T01B = T30B;
    T00B = _mm_alignr_epi8(T30B, T30A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T30A, 1), above1[3], 0);

    LINE(16,  3, 0, 1, 12);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[5], 0);

    LINE(16,  4, 0, 1, 23);
    LINE(16,  5, 0, 1,  2);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[6], 0);

    LINE(16,  6, 0, 1, 13);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[8], 0);

    LINE(16,  7, 0, 1, 24);
    LINE(16,  8, 0, 1,  3);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[9], 0);

    LINE(16,  9, 0, 1, 14);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[11], 0);

    LINE(16, 10, 0, 1, 25);
    LINE(16, 11, 0, 1,  4);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[12], 0);

    LINE(16, 12, 0, 1, 15);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[14], 0);

    LINE(16, 13, 0, 1, 26);
    LINE(16, 14, 0, 1,  5);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[15], 0);

    LINE(16, 15, 0, 1, 16);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[17], 0);

    LINE(16, 16, 0, 1, 27);
    LINE(16, 17, 0, 1,  6);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[18], 0);

    LINE(16, 18, 0, 1, 17);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[20], 0);

    LINE(16, 19, 0, 1, 28);
    LINE(16, 20, 0, 1,  7);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[21], 0);

    LINE(16, 21, 0, 1, 18);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[23], 0);

    LINE(16, 22, 0, 1, 29);
    LINE(16, 23, 0, 1,  8);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[24], 0);

    LINE(16, 24, 0, 1, 19);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[26], 0);

    LINE(16, 25, 0, 1, 30);
    LINE(16, 26, 0, 1,  9);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[27], 0);

    LINE(16, 27, 0, 1, 20);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[29], 0);

    LINE(16, 28, 0, 1, 31);
    LINE(16, 29, 0, 1, 10);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[30], 0);

    _mm_store_si128((__m128i*)&dstN[16 - 2][31 * N + 0 * 16], T00A);
    _mm_store_si128((__m128i*)&dstN[16 - 2][31 * N + 1 * 16], T00B);
    LINE(16, 30, 0, 1, 21);

    // ***********************************************************************
    T00A = _mm_insert_epi8(T30A, above1[1], 0);
    T00B = T30B;
    T01A = _mm_loadu_si128((__m128i*)(left1 +  0));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 16));

    LINE(17,  1, 0, 1, 12);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[2], 0);

    LINE(17,  2, 0, 1, 18);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[4], 0);

    LINE(17,  3, 0, 1, 24);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[5], 0);

    LINE(17,  4, 0, 1, 30);
    LINE(17,  5, 0, 1,  4);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[6], 0);

    LINE(17,  6, 0, 1, 10);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[7], 0);

    AVGS(17,  7);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[9], 0);

    LINE(17,  8, 0, 1, 22);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[10], 0);

    LINE(17,  9, 0, 1, 28);
    LINE(17, 10, 0, 1,  2);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[11], 0);

    LINE(17, 11, 0, 1,  8);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[12], 0);

    LINE(17, 12, 0, 1, 14);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[14], 0);

    LINE(17, 13, 0, 1, 20);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[15], 0);

    _mm_store_si128((__m128i*)&dstN[17 - 2][15 * N + 0 * 16], T00A);
    _mm_store_si128((__m128i*)&dstN[17 - 2][15 * N + 1 * 16], T00B);
    LINE(17, 14, 0, 1, 26);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[16], 0);

    LINE(17, 16, 0, 1,  6);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[17], 0);

    LINE(17, 17, 0, 1, 12);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[18], 0);

    LINE(17, 18, 0, 1, 18);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[20], 0);

    LINE(17, 19, 0, 1, 24);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[21], 0);

    LINE(17, 20, 0, 1, 30);
    LINE(17, 21, 0, 1,  4);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[22], 0);

    LINE(17, 22, 0, 1, 10);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[23], 0);

    AVGS(17, 23);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[25], 0);

    LINE(17, 24, 0, 1, 22);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[26], 0);

    LINE(17, 25, 0, 1, 28);
    LINE(17, 26, 0, 1,  2);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[27], 0);

    LINE(17, 27, 0, 1,  8);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[28], 0);

    LINE(17, 28, 0, 1, 14);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[30], 0);

    LINE(17, 29, 0, 1, 20);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[31], 0);

    _mm_store_si128((__m128i*)&dstN[17 - 2][31 * N + 0 * 16], T00A);
    _mm_store_si128((__m128i*)&dstN[17 - 2][31 * N + 1 * 16], T00B);
    LINE(17, 30, 0, 1, 26);

    // ***********************************************************************
    T00A = _mm_loadu_si128((__m128i*)(above1 +  0));
    T00B = _mm_loadu_si128((__m128i*)(above1 + 16));
    T01A = _mm_loadu_si128((__m128i*)(left1  +  1));
    T01A = _mm_shuffle_epi8(T01A, _mm_load_si128((__m128i*)tab_angle_2[0]));

    for (int i = 0; i < N / 2; i++)
    {
        _mm_store_si128((__m128i*)&dstN[18 - 2][i * N + 0 * 16], T00A);
        _mm_store_si128((__m128i*)&dstN[18 - 2][i * N + 1 * 16], T00B);
        T00B = _mm_alignr_epi8(T00B, T00A, 15);
        T00A = _mm_alignr_epi8(T00A, T01A, 15);
        T01A = _mm_slli_si128(T01A, 1);
    }

    T01A = _mm_loadu_si128((__m128i*)(left1  + 17));
    T01A = _mm_shuffle_epi8(T01A, _mm_load_si128((__m128i*)tab_angle_2[0]));

    _mm_store_si128((__m128i*)&dstN[18 - 2][16 * N + 0 * 16], T00A);
    _mm_store_si128((__m128i*)&dstN[18 - 2][16 * N + 1 * 16], T00B);
    for (int i = 17; i < N; i++)
    {
        T00B = _mm_alignr_epi8(T00B, T00A, 15);
        T00A = _mm_alignr_epi8(T00A, T01A, 15);
        T01A = _mm_slli_si128(T01A, 1);
        _mm_store_si128((__m128i*)&dstN[18 - 2][i * N + 0 * 16], T00A);
        _mm_store_si128((__m128i*)&dstN[18 - 2][i * N + 1 * 16], T00B);
    }

    // ***********************************************************************
    T00A = _mm_loadu_si128((__m128i*)(above1 +  1));
    T00B = _mm_loadu_si128((__m128i*)(above1 + 17));
    T01A = _mm_loadu_si128((__m128i*)(above1 +  2));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 18));

    _mm_store_si128((__m128i*)&dstN[34 - 2][0 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][0 * N + 1 * 16], T01B);
    _mm_store_si128((__m128i*)&dstN[27 - 2][15 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[27 - 2][15 * N + 1 * 16], T01B);
    AVGS(27,  7);
    LINE(27,  0, 0, 1,  2);
    LINE(27,  1, 0, 1,  4);
    LINE(27,  2, 0, 1,  6);
    LINE(27,  3, 0, 1,  8);
    LINE(27,  4, 0, 1, 10);
    COPY(28,  1);
    LINE(27,  5, 0, 1, 12);
    LINE(27,  6, 0, 1, 14);
    LINE(27,  8, 0, 1, 18);
    COPY(29,  1);
    LINE(27,  9, 0, 1, 20);
    COPY(28,  3);
    LINE(27, 10, 0, 1, 22);
    LINE(27, 11, 0, 1, 24);
    LINE(27, 12, 0, 1, 26);
    COPY(33,  0);
    COPY(30,  1);
    LINE(27, 13, 0, 1, 28);
    LINE(27, 14, 0, 1, 30);
    COPY(28,  5);
    LINE(32,  0, 0, 1, 21);
    LINE(31,  0, 0, 1, 17);
    LINE(30,  0, 0, 1, 13);
    LINE(29,  0, 0, 1,  9);
    LINE(29,  2, 0, 1, 27);
    LINE(28,  0, 0, 1,  5);
    LINE(28,  2, 0, 1, 15);
    LINE(28,  4, 0, 1, 25);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(above1 +  3));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 19));

    _mm_store_si128((__m128i*)&dstN[34 - 2][1 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][1 * N + 1 * 16], T01B);
    _mm_store_si128((__m128i*)&dstN[27 - 2][31 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[27 - 2][31 * N + 1 * 16], T01B);
    AVGS(27, 23);
    LINE(27, 16, 0, 1,  2);
    COPY(31,  1);
    LINE(27, 17, 0, 1,  4);
    COPY(29,  3);
    LINE(27, 18, 0, 1,  6);
    LINE(27, 19, 0, 1,  8);
    COPY(28,  7);
    LINE(27, 20, 0, 1, 10);
    COPY(32,  1);
    LINE(27, 21, 0, 1, 12);
    LINE(27, 22, 0, 1, 14);
    LINE(27, 24, 0, 1, 18);
    COPY(28,  9);
    LINE(27, 25, 0, 1, 20);
    COPY(33,  1);
    COPY(30,  3);
    LINE(27, 26, 0, 1, 22);
    COPY(29,  5);
    LINE(27, 27, 0, 1, 24);
    LINE(27, 28, 0, 1, 26);
    LINE(27, 29, 0, 1, 28);
    COPY(28, 11);
    LINE(27, 30, 0, 1, 30);
    LINE(28,  6, 0, 1,  3);
    LINE(28,  8, 0, 1, 13);
    COPY(29,  4);
    LINE(28, 10, 0, 1, 23);
    LINE(29,  6, 0, 1, 31);
    COPY(32,  2);
    LINE(30,  2, 0, 1,  7);
    LINE(31,  2, 0, 1, 19);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(above1 +  4));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 20));

    _mm_store_si128((__m128i*)&dstN[34 - 2][2 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][2 * N + 1 * 16], T01B);
    AVGS(28, 15);
    LINE(28, 12, 0, 1,  1);
    COPY(30,  4);
    LINE(28, 13, 0, 1,  6);
    LINE(28, 14, 0, 1, 11);
    LINE(28, 16, 0, 1, 21);
    COPY(31,  4);
    LINE(28, 17, 0, 1, 26);
    COPY(29,  9);
    LINE(28, 18, 0, 1, 31);
    LINE(32,  3, 0, 1, 20);
    LINE(31,  3, 0, 1,  4);
    LINE(30,  5, 0, 1, 14);
    COPY(33,  2);
    LINE(30,  6, 0, 1, 27);
    LINE(29,  7, 0, 1,  8);
    LINE(29,  8, 0, 1, 17);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(above1 +  5));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 21));

    _mm_store_si128((__m128i*)&dstN[34 - 2][3 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][3 * N + 1 * 16], T01B);
    LINE(28, 19, 0, 1,  4);
    LINE(28, 20, 0, 1,  9);
    COPY(32,  4);
    LINE(28, 21, 0, 1, 14);
    LINE(28, 22, 0, 1, 19);
    LINE(28, 23, 0, 1, 24);
    LINE(28, 24, 0, 1, 29);
    LINE(29, 10, 0, 1,  3);
    LINE(29, 11, 0, 1, 12);
    LINE(29, 12, 0, 1, 21);
    COPY(30,  8);
    LINE(29, 13, 0, 1, 30);
    COPY(32,  5);
    LINE(30,  7, 0, 1,  8);
    COPY(33,  3);
    LINE(31,  5, 0, 1,  6);
    LINE(31,  6, 0, 1, 23);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(above1 +  6));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 22));

    _mm_store_si128((__m128i*)&dstN[34 - 2][4 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][4 * N + 1 * 16], T01B);
    _mm_store_si128((__m128i*)&dstN[28 - 2][31 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[28 - 2][31 * N + 1 * 16], T01B);
    AVGS(29, 15);
    LINE(28, 25, 0, 1,  2);
    COPY(33,  4);
    LINE(28, 26, 0, 1,  7);
    COPY(29, 14);
    LINE(28, 27, 0, 1, 12);
    LINE(28, 28, 0, 1, 17);
    LINE(28, 29, 0, 1, 22);
    LINE(28, 30, 0, 1, 27);
    LINE(29, 16, 0, 1, 25);
    COPY(31,  8);
    LINE(30,  9, 0, 1,  2);
    LINE(30, 10, 0, 1, 15);
    LINE(30, 11, 0, 1, 28);
    COPY(33,  5);
    LINE(31,  7, 0, 1,  8);
    LINE(32,  6, 0, 1, 19);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(above1 +  7));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 23));

    _mm_store_si128((__m128i*)&dstN[34 - 2][5 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][5 * N + 1 * 16], T01B);
    LINE(32,  7, 0, 1,  8);
    LINE(31,  9, 0, 1, 10);
    LINE(31, 10, 0, 1, 27);
    LINE(30, 12, 0, 1,  9);
    LINE(30, 13, 0, 1, 22);
    COPY(33,  6);
    LINE(29, 17, 0, 1,  2);
    LINE(29, 18, 0, 1, 11);
    LINE(29, 19, 0, 1, 20);
    LINE(29, 20, 0, 1, 29);
    COPY(32,  8);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(above1 +  8));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 24));

    _mm_store_si128((__m128i*)&dstN[34 - 2][6 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][6 * N + 1 * 16], T01B);
    AVGS(30, 15);
    AVGS(33,  7);
    LINE(32,  9, 0, 1, 18);
    LINE(31, 11, 0, 1, 12);
    LINE(30, 14, 0, 1,  3);
    LINE(30, 16, 0, 1, 29);
    COPY(31, 12);
    LINE(29, 21, 0, 1,  6);
    LINE(29, 22, 0, 1, 15);
    LINE(29, 23, 0, 1, 24);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(above1 +  9));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 25));

    _mm_store_si128((__m128i*)&dstN[34 - 2][7 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][7 * N + 1 * 16], T01B);
    LINE(29, 24, 0, 1,  1);
    LINE(29, 25, 0, 1, 10);
    COPY(30, 17);
    COPY(33,  8);
    LINE(29, 26, 0, 1, 19);
    LINE(29, 27, 0, 1, 28);
    COPY(32, 11);
    LINE(30, 18, 0, 1, 23);
    LINE(31, 13, 0, 1, 14);
    LINE(31, 14, 0, 1, 31);
    LINE(32, 10, 0, 1,  7);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(above1 + 10));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 26));

    _mm_store_si128((__m128i*)&dstN[34 - 2][8 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][8 * N + 1 * 16], T01B);
    _mm_store_si128((__m128i*)&dstN[29 - 2][31 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[29 - 2][31 * N + 1 * 16], T01B);
    LINE(29, 28, 0, 1,  5);
    LINE(29, 29, 0, 1, 14);
    LINE(29, 30, 0, 1, 23);
    LINE(30, 19, 0, 1,  4);
    COPY(33,  9);
    LINE(30, 20, 0, 1, 17);
    COPY(32, 12);
    LINE(30, 21, 0, 1, 30);
    COPY(33, 10);
    AVGS(31, 15);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(above1 + 11));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 27));

    _mm_store_si128((__m128i*)&dstN[34 - 2][9 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][9 * N + 1 * 16], T01B);
    LINE(32, 13, 0, 1,  6);
    LINE(32, 14, 0, 1, 27);
    LINE(31, 16, 0, 1,  1);
    LINE(31, 17, 0, 1, 18);
    LINE(30, 22, 0, 1, 11);
    LINE(30, 23, 0, 1, 24);
    COPY(33, 11);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(above1 + 12));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 28));

    _mm_store_si128((__m128i*)&dstN[34 - 2][10 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][10 * N + 1 * 16], T01B);
    AVGS(32, 15);
    LINE(31, 18, 0, 1,  3);
    LINE(31, 19, 0, 1, 20);
    LINE(30, 24, 0, 1,  5);
    LINE(30, 25, 0, 1, 18);
    COPY(33, 12);
    LINE(30, 26, 0, 1, 31);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(above1 + 13));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 29));

    _mm_store_si128((__m128i*)&dstN[34 - 2][11 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][11 * N + 1 * 16], T01B);
    LINE(31, 20, 0, 1,  5);
    COPY(32, 16);
    LINE(31, 21, 0, 1, 22);
    LINE(30, 27, 0, 1, 12);
    COPY(33, 13);
    LINE(30, 28, 0, 1, 25);
    LINE(32, 17, 0, 1, 26);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(above1 + 14));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 30));

    _mm_store_si128((__m128i*)&dstN[34 - 2][12 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][12 * N + 1 * 16], T01B);
    _mm_store_si128((__m128i*)&dstN[33 - 2][15 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[33 - 2][15 * N + 1 * 16], T01B);
    _mm_store_si128((__m128i*)&dstN[30 - 2][31 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[30 - 2][31 * N + 1 * 16], T01B);
    LINE(33, 14, 0, 1,  6);
    COPY(30, 29);
    LINE(32, 18, 0, 1, 15);
    LINE(31, 22, 0, 1,  7);
    LINE(31, 23, 0, 1, 24);
    LINE(30, 30, 0, 1, 19);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(above1 + 15));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 31));

    _mm_store_si128((__m128i*)&dstN[34 - 2][13 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][13 * N + 1 * 16], T01B);
    LINE(33, 16, 0, 1, 26);
    COPY(31, 25);
    LINE(32, 19, 0, 1,  4);
    LINE(32, 20, 0, 1, 25);
    LINE(31, 24, 0, 1,  9);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(above1 + 16));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 32));

    _mm_store_si128((__m128i*)&dstN[34 - 2][14 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][14 * N + 1 * 16], T01B);
    LINE(33, 17, 0, 1, 20);
    LINE(32, 21, 0, 1, 14);
    LINE(31, 26, 0, 1, 11);
    LINE(31, 27, 0, 1, 28);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(above1 + 17));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 33));

    _mm_store_si128((__m128i*)&dstN[34 - 2][15 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][15 * N + 1 * 16], T01B);
    LINE(33, 18, 0, 1, 14);
    LINE(32, 22, 0, 1,  3);
    LINE(32, 23, 0, 1, 24);
    LINE(31, 28, 0, 1, 13);
    LINE(31, 29, 0, 1, 30);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(above1 + 18));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 34));

    _mm_store_si128((__m128i*)&dstN[34 - 2][16 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][16 * N + 1 * 16], T01B);
    _mm_store_si128((__m128i*)&dstN[31 - 2][31 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[31 - 2][31 * N + 1 * 16], T01B);
    LINE(33, 19, 0, 1,  8);
    LINE(32, 24, 0, 1, 13);
    LINE(31, 30, 0, 1, 15);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(above1 + 19));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 35));

    _mm_store_si128((__m128i*)&dstN[34 - 2][17 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][17 * N + 1 * 16], T01B);
    LINE(33, 20, 0, 1,  2);
    COPY(32, 25);
    LINE(33, 21, 0, 1, 28);
    LINE(32, 26, 0, 1, 23);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(above1 + 20));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 36));

    _mm_store_si128((__m128i*)&dstN[34 - 2][18 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][18 * N + 1 * 16], T01B);
    LINE(33, 22, 0, 1, 22);
    LINE(32, 27, 0, 1, 12);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(above1 + 21));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 37));

    _mm_store_si128((__m128i*)&dstN[34 - 2][19 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][19 * N + 1 * 16], T01B);
    AVGS(33, 23);
    LINE(32, 28, 0, 1,  1);
    LINE(32, 29, 0, 1, 22);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(above1 + 22));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 38));

    _mm_store_si128((__m128i*)&dstN[34 - 2][20 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][20 * N + 1 * 16], T01B);
    _mm_store_si128((__m128i*)&dstN[32 - 2][31 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[32 - 2][31 * N + 1 * 16], T01B);
    LINE(33, 24, 0, 1, 10);
    LINE(32, 30, 0, 1, 11);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(above1 + 23));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 39));

    _mm_store_si128((__m128i*)&dstN[34 - 2][21 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][21 * N + 1 * 16], T01B);
    LINE(33, 25, 0, 1,  4);
    LINE(33, 26, 0, 1, 30);

    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(above1 + 24));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 40));

    _mm_store_si128((__m128i*)&dstN[34 - 2][22 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][22 * N + 1 * 16], T01B);
    LINE(33, 27, 0, 1, 24);

    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(above1 + 25));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 41));

    _mm_store_si128((__m128i*)&dstN[34 - 2][23 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][23 * N + 1 * 16], T01B);
    LINE(33, 28, 0, 1, 18);

    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(above1 + 26));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 42));

    _mm_store_si128((__m128i*)&dstN[34 - 2][24 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][24 * N + 1 * 16], T01B);
    LINE(33, 29, 0, 1, 12);

    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(above1 + 27));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 43));

    _mm_store_si128((__m128i*)&dstN[34 - 2][25 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][25 * N + 1 * 16], T01B);
    _mm_store_si128((__m128i*)&dstN[33 - 2][31 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[33 - 2][31 * N + 1 * 16], T01B);
    LINE(33, 30, 0, 1,  6);

    T01A = _mm_loadu_si128((__m128i*)(above1 + 28));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 44));
    _mm_store_si128((__m128i*)&dstN[34 - 2][26 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][26 * N + 1 * 16], T01B);

    T01A = _mm_loadu_si128((__m128i*)(above1 + 29));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 45));
    _mm_store_si128((__m128i*)&dstN[34 - 2][27 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][27 * N + 1 * 16], T01B);

    T01A = _mm_loadu_si128((__m128i*)(above1 + 30));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 46));
    _mm_store_si128((__m128i*)&dstN[34 - 2][28 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][28 * N + 1 * 16], T01B);

    T01A = _mm_loadu_si128((__m128i*)(above1 + 31));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 47));
    _mm_store_si128((__m128i*)&dstN[34 - 2][29 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][29 * N + 1 * 16], T01B);

    T01A = _mm_loadu_si128((__m128i*)(above1 + 32));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 48));
    _mm_store_si128((__m128i*)&dstN[34 - 2][30 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][30 * N + 1 * 16], T01B);

    T01A = _mm_loadu_si128((__m128i*)(above1 + 33));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 49));
    _mm_store_si128((__m128i*)&dstN[34 - 2][31 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][31 * N + 1 * 16], T01B);

    // ***********************************************************************
    T00A = _mm_loadu_si128((__m128i*)(above1 +  0));
    T00B = _mm_loadu_si128((__m128i*)(above1 + 16));
    T01A = _mm_loadu_si128((__m128i*)(above1 +  1));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 17));

    _mm_store_si128((__m128i*)&dstN[25 - 2][15 * N + 0 * 16], T00A);
    _mm_store_si128((__m128i*)&dstN[25 - 2][15 * N + 1 * 16], T00B);
    LINE(25,  0, 0, 1, 30);
    LINE(25,  1, 0, 1, 28);
    LINE(25,  2, 0, 1, 26);
    LINE(25,  3, 0, 1, 24);
    LINE(25,  4, 0, 1, 22);
    COPY(24,  1);
    LINE(25,  5, 0, 1, 20);
    LINE(25,  6, 0, 1, 18);
    AVGS(25,  7);
    LINE(25,  8, 0, 1, 14);
    COPY(23,  1);
    LINE(25,  9, 0, 1, 12);
    COPY(24,  3);
    LINE(25, 10, 0, 1, 10);
    LINE(25, 11, 0, 1,  8);
    LINE(25, 12, 0, 1,  6);
    COPY(22,  1);
    COPY(19,  0);
    LINE(25, 13, 0, 1,  4);
    LINE(25, 14, 0, 1,  2);
    COPY(24,  5);
    LINE(24,  0, 0, 1, 27);
    LINE(24,  2, 0, 1, 17);
    LINE(24,  4, 0, 1,  7);
    LINE(23,  0, 0, 1, 23);
    LINE(23,  2, 0, 1,  5);
    LINE(22,  0, 0, 1, 19);
    LINE(21,  0, 0, 1, 15);
    LINE(20,  0, 0, 1, 11);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T30A = _mm_slli_si128(T00A, 1);
    T30B = T00B;
    T00A = _mm_insert_epi8(T30A, left1[16], 0);

    _mm_store_si128((__m128i*)&dstN[25 - 2][31 * N + 0 * 16], T00A);
    _mm_store_si128((__m128i*)&dstN[25 - 2][31 * N + 1 * 16], T00B);
    LINE(25, 16, 0, 1, 30);
    LINE(25, 17, 0, 1, 28);
    LINE(25, 18, 0, 1, 26);
    LINE(25, 19, 0, 1, 24);
    LINE(25, 20, 0, 1, 22);
    LINE(25, 21, 0, 1, 20);
    LINE(25, 22, 0, 1, 18);
    AVGS(25, 23);
    LINE(25, 24, 0, 1, 14);
    LINE(25, 25, 0, 1, 12);
    LINE(25, 26, 0, 1, 10);
    LINE(25, 27, 0, 1,  8);
    LINE(25, 28, 0, 1,  6);
    LINE(25, 29, 0, 1,  4);
    LINE(25, 30, 0, 1,  2);

    T00A = _mm_insert_epi8(T30A, left1[6], 0);

    LINE(24,  6, 0, 1, 29);
    LINE(24,  7, 0, 1, 24);
    LINE(24,  8, 0, 1, 19);
    LINE(24,  9, 0, 1, 14);
    LINE(24, 10, 0, 1,  9);
    LINE(24, 11, 0, 1,  4);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[13], 0);

    LINE(24, 12, 0, 1, 31);
    LINE(24, 13, 0, 1, 26);
    LINE(24, 14, 0, 1, 21);
    AVGS(24, 15);
    LINE(24, 16, 0, 1, 11);
    LINE(24, 17, 0, 1,  6);
    LINE(24, 18, 0, 1,  1);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[19], 0);

    LINE(24, 19, 0, 1, 28);
    LINE(24, 20, 0, 1, 23);
    LINE(24, 21, 0, 1, 18);
    LINE(24, 22, 0, 1, 13);
    LINE(24, 23, 0, 1,  8);
    LINE(24, 24, 0, 1,  3);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[26], 0);

    _mm_store_si128((__m128i*)&dstN[24 - 2][31 * N + 0 * 16], T00A);
    _mm_store_si128((__m128i*)&dstN[24 - 2][31 * N + 1 * 16], T00B);
    LINE(24, 25, 0, 1, 30);
    LINE(24, 26, 0, 1, 25);
    LINE(24, 27, 0, 1, 20);
    LINE(24, 28, 0, 1, 15);
    LINE(24, 29, 0, 1, 10);
    LINE(24, 30, 0, 1,  5);

    // ***********************************************************************
    T00A = _mm_insert_epi8(T30A, left1[4], 0);
    T00B = T30B;
    T01A = _mm_loadu_si128((__m128i*)(above1 +  0));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 16));

    LINE(23,  3, 0, 1, 28);
    LINE(23,  4, 0, 1, 19);
    LINE(23,  5, 0, 1, 10);
    LINE(23,  6, 0, 1,  1);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[7], 0);

    LINE(23,  7, 0, 1, 24);
    LINE(23,  8, 0, 1, 15);
    LINE(23,  9, 0, 1,  6);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[11], 0);

    LINE(23, 10, 0, 1, 29);
    LINE(23, 11, 0, 1, 20);
    LINE(23, 12, 0, 1, 11);
    LINE(23, 13, 0, 1,  2);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[14], 0);

    LINE(23, 14, 0, 1, 25);
    AVGS(23, 15);
    LINE(23, 16, 0, 1,  7);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[18], 0);

    LINE(23, 17, 0, 1, 30);
    LINE(23, 18, 0, 1, 21);
    LINE(23, 19, 0, 1, 12);
    LINE(23, 20, 0, 1,  3);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[21], 0);

    LINE(23, 21, 0, 1, 26);
    LINE(23, 22, 0, 1, 17);
    LINE(23, 23, 0, 1,  8);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[25], 0);

    LINE(23, 24, 0, 1, 31);
    LINE(23, 25, 0, 1, 22);
    LINE(23, 26, 0, 1, 13);
    LINE(23, 27, 0, 1,  4);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[28], 0);

    _mm_store_si128((__m128i*)&dstN[23 - 2][31 * N + 0 * 16], T00A);
    _mm_store_si128((__m128i*)&dstN[23 - 2][31 * N + 1 * 16], T00B);
    LINE(23, 28, 0, 1, 27);
    LINE(23, 29, 0, 1, 18);
    LINE(23, 30, 0, 1,  9);

    // ***********************************************************************
    T00A = _mm_insert_epi8(T30A, left1[2], 0);
    T00B = T30B;
    T01A = _mm_loadu_si128((__m128i*)(above1 +  0));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 16));
    T30A = T00A;
    T30B = T00B;

    LINE(22,  2, 0, 1, 25);
    LINE(22,  3, 0, 1, 12);
    LINE(21,  1, 0, 1, 30);
    LINE(21,  2, 0, 1, 13);
    LINE(20,  1, 0, 1, 22);
    LINE(20,  2, 0, 1,  1);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[5], 0);

    LINE(22,  4, 0, 1, 31);
    LINE(22,  5, 0, 1, 18);
    LINE(22,  6, 0, 1,  5);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[7], 0);

    LINE(22,  7, 0, 1, 24);
    LINE(22,  8, 0, 1, 11);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[10], 0);

    LINE(22,  9, 0, 1, 30);
    LINE(22, 10, 0, 1, 17);
    LINE(22, 11, 0, 1,  4);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[12], 0);

    LINE(22, 12, 0, 1, 23);
    LINE(22, 13, 0, 1, 10);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[15], 0);

    LINE(22, 14, 0, 1, 29);
    LINE(22, 15, 0, 1, 16);
    LINE(22, 16, 0, 1,  3);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[17], 0);

    LINE(22, 17, 0, 1, 22);
    LINE(22, 18, 0, 1,  9);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[20], 0);

    LINE(22, 19, 0, 1, 28);
    LINE(22, 20, 0, 1, 15);
    LINE(22, 21, 0, 1,  2);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[22], 0);

    LINE(22, 22, 0, 1, 21);
    LINE(22, 23, 0, 1,  8);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[25], 0);

    LINE(22, 24, 0, 1, 27);
    LINE(22, 25, 0, 1, 14);
    LINE(22, 26, 0, 1,  1);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[27], 0);

    LINE(22, 27, 0, 1, 20);
    LINE(22, 28, 0, 1,  7);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[30], 0);

    _mm_store_si128((__m128i*)&dstN[22 - 2][31 * N + 0 * 16], T00A);
    _mm_store_si128((__m128i*)&dstN[22 - 2][31 * N + 1 * 16], T00B);
    LINE(22, 29, 0, 1, 26);
    LINE(22, 30, 0, 1, 13);

    // ***********************************************************************
    T01A = T30A;
    T01B = T30B;
    T00B = _mm_alignr_epi8(T30B, T30A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T30A, 1), left1[4], 0);

    LINE(21,  3, 0, 1, 28);
    LINE(21,  4, 0, 1, 11);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[6], 0);

    LINE(21,  5, 0, 1, 26);
    LINE(21,  6, 0, 1,  9);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[8], 0);

    LINE(21,  7, 0, 1, 24);
    LINE(21,  8, 0, 1,  7);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[9], 0);

    LINE(21,  9, 0, 1, 22);
    LINE(21, 10, 0, 1,  5);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[11], 0);

    LINE(21, 11, 0, 1, 20);
    LINE(21, 12, 0, 1,  3);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[13], 0);

    LINE(21, 13, 0, 1, 18);
    LINE(21, 14, 0, 1,  1);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[15], 0);

    AVGS(21, 15);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[17], 0);

    LINE(21, 16, 0, 1, 31);
    LINE(21, 17, 0, 1, 14);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[19], 0);

    LINE(21, 18, 0, 1, 29);
    LINE(21, 19, 0, 1, 12);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[21], 0);

    LINE(21, 20, 0, 1, 27);
    LINE(21, 21, 0, 1, 10);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[23], 0);

    LINE(21, 22, 0, 1, 25);
    LINE(21, 23, 0, 1,  8);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[24], 0);

    LINE(21, 24, 0, 1, 23);
    LINE(21, 25, 0, 1,  6);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[26], 0);

    LINE(21, 26, 0, 1, 21);
    LINE(21, 27, 0, 1,  4);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[28], 0);

    LINE(21, 28, 0, 1, 19);
    LINE(21, 29, 0, 1,  2);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[30], 0);

    _mm_store_si128((__m128i*)&dstN[21 - 2][31 * N + 0 * 16], T00A);
    _mm_store_si128((__m128i*)&dstN[21 - 2][31 * N + 1 * 16], T00B);
    LINE(21, 30, 0, 1, 17);

    // ***********************************************************************
    T01A = T30A;
    T01B = T30B;
    T00B = _mm_alignr_epi8(T30B, T30A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T30A, 1), left1[3], 0);

    LINE(20,  3, 0, 1, 12);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[5], 0);

    LINE(20,  4, 0, 1, 23);
    LINE(20,  5, 0, 1,  2);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[6], 0);

    LINE(20,  6, 0, 1, 13);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[8], 0);

    LINE(20,  7, 0, 1, 24);
    LINE(20,  8, 0, 1,  3);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[9], 0);

    LINE(20,  9, 0, 1, 14);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[11], 0);

    LINE(20, 10, 0, 1, 25);
    LINE(20, 11, 0, 1,  4);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[12], 0);

    LINE(20, 12, 0, 1, 15);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[14], 0);

    LINE(20, 13, 0, 1, 26);
    LINE(20, 14, 0, 1,  5);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[15], 0);

    LINE(20, 15, 0, 1, 16);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[17], 0);

    LINE(20, 16, 0, 1, 27);
    LINE(20, 17, 0, 1,  6);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[18], 0);

    LINE(20, 18, 0, 1, 17);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[20], 0);

    LINE(20, 19, 0, 1, 28);
    LINE(20, 20, 0, 1,  7);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[21], 0);

    LINE(20, 21, 0, 1, 18);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[23], 0);

    LINE(20, 22, 0, 1, 29);
    LINE(20, 23, 0, 1,  8);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[24], 0);

    LINE(20, 24, 0, 1, 19);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[26], 0);

    LINE(20, 25, 0, 1, 30);
    LINE(20, 26, 0, 1,  9);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[27], 0);

    LINE(20, 27, 0, 1, 20);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[29], 0);

    LINE(20, 28, 0, 1, 31);
    LINE(20, 29, 0, 1, 10);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[30], 0);

    _mm_store_si128((__m128i*)&dstN[20 - 2][31 * N + 0 * 16], T00A);
    _mm_store_si128((__m128i*)&dstN[20 - 2][31 * N + 1 * 16], T00B);
    LINE(20, 30, 0, 1, 21);

    // ***********************************************************************
    T00A = _mm_insert_epi8(T30A, left1[1], 0);
    T00B = T30B;
    T01A = _mm_loadu_si128((__m128i*)(above1 +  0));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 16));

    LINE(19,  1, 0, 1, 12);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[2], 0);

    LINE(19,  2, 0, 1, 18);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[4], 0);

    LINE(19,  3, 0, 1, 24);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[5], 0);

    LINE(19,  4, 0, 1, 30);
    LINE(19,  5, 0, 1,  4);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[6], 0);

    LINE(19,  6, 0, 1, 10);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[7], 0);

    AVGS(19,  7);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[9], 0);

    LINE(19,  8, 0, 1, 22);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[10], 0);

    LINE(19,  9, 0, 1, 28);
    LINE(19, 10, 0, 1,  2);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[11], 0);

    LINE(19, 11, 0, 1,  8);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[12], 0);

    LINE(19, 12, 0, 1, 14);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[14], 0);

    LINE(19, 13, 0, 1, 20);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[15], 0);

    _mm_store_si128((__m128i*)&dstN[19 - 2][15 * N + 0 * 16], T00A);
    _mm_store_si128((__m128i*)&dstN[19 - 2][15 * N + 1 * 16], T00B);
    LINE(19, 14, 0, 1, 26);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[16], 0);

    LINE(19, 16, 0, 1,  6);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[17], 0);

    LINE(19, 17, 0, 1, 12);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[18], 0);

    LINE(19, 18, 0, 1, 18);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[20], 0);

    LINE(19, 19, 0, 1, 24);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[21], 0);

    LINE(19, 20, 0, 1, 30);
    LINE(19, 21, 0, 1,  4);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[22], 0);

    LINE(19, 22, 0, 1, 10);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[23], 0);

    AVGS(19, 23);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[25], 0);

    LINE(19, 24, 0, 1, 22);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[26], 0);

    LINE(19, 25, 0, 1, 28);
    LINE(19, 26, 0, 1,  2);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[27], 0);

    LINE(19, 27, 0, 1,  8);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[28], 0);

    LINE(19, 28, 0, 1, 14);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[30], 0);

    LINE(19, 29, 0, 1, 20);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[31], 0);

    _mm_store_si128((__m128i*)&dstN[19 - 2][31 * N + 0 * 16], T00A);
    _mm_store_si128((__m128i*)&dstN[19 - 2][31 * N + 1 * 16], T00B);
    LINE(19, 30, 0, 1, 26);

#undef COPY
#undef LINE
#undef HALF
#undef N
}

#endif // if !defined(__clang__)
#endif // if !HIGH_BIT_DEPTH
}

namespace x265 {
void Setup_Vec_IPredPrimitives_sse41(EncoderPrimitives& p)
{
#if !HIGH_BIT_DEPTH && !defined(__clang__) && (defined(__GNUC__) || defined(__INTEL_COMPILER) || (defined(_MSC_VER) && (_MSC_VER == 1500)))

    p.intra_pred_allangs[BLOCK_32x32] = predIntraAngs32;

#endif
}
}
