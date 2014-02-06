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
#include <assert.h>
#include <xmmintrin.h> // SSE
#include <pmmintrin.h> // SSE3
#include <tmmintrin.h> // SSSE3

using namespace x265;

namespace {
#if !HIGH_BIT_DEPTH
const int angAP[17][64] =
{
    {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64
    },
    {
        0, 1, 2, 3, 4, 4, 5, 6, 7, 8, 8, 9, 10, 11, 12, 13, 13, 14, 15, 16, 17, 17, 18, 19, 20, 21, 21, 22, 23, 24, 25, 26, 26, 27, 28, 29, 30, 30, 31, 32, 33, 34, 34, 35, 36, 37, 38, 39, 39, 40, 41, 42, 43, 43, 44, 45, 46, 47, 47, 48, 49, 50, 51, 52
    },
    {
        0, 1, 1, 2, 3, 3, 4, 5, 5, 6, 7, 7, 8, 9, 9, 10, 11, 11, 12, 13, 13, 14, 15, 15, 16, 17, 17, 18, 19, 19, 20, 21, 21, 22, 22, 23, 24, 24, 25, 26, 26, 27, 28, 28, 29, 30, 30, 31, 32, 32, 33, 34, 34, 35, 36, 36, 37, 38, 38, 39, 40, 40, 41, 42
    },
    {
        0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 23, 24, 24, 25, 26, 26, 27, 27, 28, 28, 29, 29, 30, 30, 31, 31, 32, 32, 33, 34
    },
    {
        0, 0, 1, 1, 2, 2, 2, 3, 3, 4, 4, 4, 5, 5, 6, 6, 6, 7, 7, 8, 8, 8, 9, 9, 10, 10, 10, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 15, 16, 16, 17, 17, 17, 18, 18, 19, 19, 19, 20, 20, 21, 21, 21, 22, 22, 23, 23, 23, 24, 24, 25, 25, 26
    },
    {
        0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 9, 9, 9, 9, 10, 10, 10, 10, 11, 11, 11, 12, 12, 12, 12, 13, 13, 13, 14, 14, 14, 14, 15, 15, 15, 16, 16, 16, 16, 17, 17, 17, 18
    },
    {
        0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 10
    },
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4
    },
    { // 0th virtual index; never used; just to help indexing
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4
    },
    {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4
    },
    {
        -1, -1, -1, -1, -1, -1, -2, -2, -2, -2, -2, -2, -3, -3, -3, -3, -3, -3, -3, -4, -4, -4, -4, -4, -4, -5, -5, -5, -5, -5, -5, -5, -6, -6, -6, -6, -6, -6, -7, -7, -7, -7, -7, -7, -8, -8, -8, -8, -8, -8, -8, -9, -9, -9, -9, -9, -9, -10, -10, -10, -10, -10, -10, -10
    },
    {
        -1, -1, -1, -2, -2, -2, -2, -3, -3, -3, -4, -4, -4, -4, -5, -5, -5, -6, -6, -6, -6, -7, -7, -7, -8, -8, -8, -8, -9, -9, -9, -9, -10, -10, -10, -11, -11, -11, -11, -12, -12, -12, -13, -13, -13, -13, -14, -14, -14, -15, -15, -15, -15, -16, -16, -16, -17, -17, -17, -17, -18, -18, -18, -18
    },
    {
        -1, -1, -2, -2, -3, -3, -3, -4, -4, -5, -5, -5, -6, -6, -7, -7, -7, -8, -8, -9, -9, -9, -10, -10, -11, -11, -11, -12, -12, -13, -13, -13, -14, -14, -15, -15, -16, -16, -16, -17, -17, -18, -18, -18, -19, -19, -20, -20, -20, -21, -21, -22, -22, -22, -23, -23, -24, -24, -24, -25, -25, -26, -26, -26
    },
    {
        -1, -2, -2, -3, -3, -4, -4, -5, -5, -6, -6, -7, -7, -8, -8, -9, -10, -10, -11, -11, -12, -12, -13, -13, -14, -14, -15, -15, -16, -16, -17, -17, -18, -19, -19, -20, -20, -21, -21, -22, -22, -23, -23, -24, -24, -25, -25, -26, -27, -27, -28, -28, -29, -29, -30, -30, -31, -31, -32, -32, -33, -33, -34, -34
    },
    {
        -1, -2, -2, -3, -4, -4, -5, -6, -6, -7, -8, -8, -9, -10, -10, -11, -12, -12, -13, -14, -14, -15, -16, -16, -17, -18, -18, -19, -20, -20, -21, -21, -22, -23, -23, -24, -25, -25, -26, -27, -27, -28, -29, -29, -30, -31, -31, -32, -33, -33, -34, -35, -35, -36, -37, -37, -38, -39, -39, -40, -41, -41, -42, -42
    },
    {
        -1, -2, -3, -4, -5, -5, -6, -7, -8, -9, -9, -10, -11, -12, -13, -13, -14, -15, -16, -17, -18, -18, -19, -20, -21, -22, -22, -23, -24, -25, -26, -26, -27, -28, -29, -30, -31, -31, -32, -33, -34, -35, -35, -36, -37, -38, -39, -39, -40, -41, -42, -43, -44, -44, -45, -46, -47, -48, -48, -49, -50, -51, -52, -52
    },
    {
        -1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, -14, -15, -16, -17, -18, -19, -20, -21, -22, -23, -24, -25, -26, -27, -28, -29, -30, -31, -32, -33, -34, -35, -36, -37, -38, -39, -40, -41, -42, -43, -44, -45, -46, -47, -48, -49, -50, -51, -52, -53, -54, -55, -56, -57, -58, -59, -60, -61, -62, -63, -64
    }
};

#define GETAP(X, Y) angAP[8 - (X)][(Y)]

// 16x16
#define PREDANG_CALCROW_VER(X) \
    LOADROW(row11L, row11H, GETAP(lookIdx, X)); \
    LOADROW(row12L, row12H, GETAP(lookIdx, X) + 1); \
    CALCROW(row11L, row11H, row11L, row11H, row12L, row12H); \
    itmp = _mm_packus_epi16(row11L, row11H); \
    _mm_storeu_si128((__m128i*)(dst + ((X)*dstStride)), itmp);

#define PREDANG_CALCROW_HOR(X, rowx) \
    LOADROW(row11L, row11H, GETAP(lookIdx, (X))); \
    LOADROW(row12L, row12H, GETAP(lookIdx, (X)) + 1); \
    CALCROW(row11L, row11H, row11L, row11H, row12L, row12H); \
    rowx = _mm_packus_epi16(row11L, row11H);

#define LOADROW(ROWL, ROWH, X) \
    itmp = _mm_loadu_si128((__m128i const*)(refMain + 1 + (X))); \
    ROWL = _mm_unpacklo_epi8(itmp, _mm_setzero_si128()); \
    ROWH = _mm_unpackhi_epi8(itmp, _mm_setzero_si128());

#define CALCROW(RESL, RESH, ROW1L, ROW1H, ROW2L, ROW2H) \
    v_deltaPos = _mm_add_epi16(v_deltaPos, v_ipAngle); \
    v_deltaFract = _mm_and_si128(v_deltaPos, thirty1); \
    it1 = _mm_sub_epi16(thirty2, v_deltaFract); \
    it2 = _mm_mullo_epi16(it1, ROW1L); \
    it3 = _mm_mullo_epi16(v_deltaFract, ROW2L); \
    it2 = _mm_add_epi16(it2, it3); \
    i16 = _mm_set1_epi16(16); \
    it2 = _mm_add_epi16(it2, i16); \
    RESL = _mm_srai_epi16(it2, 5); \
    it2 = _mm_mullo_epi16(it1, ROW1H); \
    it3 = _mm_mullo_epi16(v_deltaFract, ROW2H); \
    it2 = _mm_add_epi16(it2, it3); \
    it2 = _mm_add_epi16(it2, i16); \
    RESH = _mm_srai_epi16(it2, 5);

#define BLND2_16(R1, R2) \
    itmp1 = _mm_unpacklo_epi8(R1, R2); \
    itmp2 = _mm_unpackhi_epi8(R1, R2); \
    R1 = itmp1; \
    R2 = itmp2;

#define MB4(R1, R2, R3, R4) \
    BLND2_16(R1, R2) \
    BLND2_16(R3, R4) \
    itmp1 = _mm_unpacklo_epi16(R1, R3); \
    itmp2 = _mm_unpackhi_epi16(R1, R3); \
    R1 = itmp1; \
    R3 = itmp2; \
    itmp1 = _mm_unpacklo_epi16(R2, R4); \
    itmp2 = _mm_unpackhi_epi16(R2, R4); \
    R2 = itmp1; \
    R4 = itmp2;

#define BLND2_4(R1, R2) \
    itmp1 = _mm_unpacklo_epi32(R1, R2); \
    itmp2 = _mm_unpackhi_epi32(R1, R2); \
    R1 = itmp1; \
    R2 = itmp2;

#define BLND2_2(R1, R2) \
    itmp1 = _mm_unpacklo_epi64(R1, R2); \
    itmp2 = _mm_unpackhi_epi64(R1, R2); \
    _mm_storeu_si128((__m128i*)dst, itmp1); \
    dst += dstStride; \
    _mm_storeu_si128((__m128i*)dst, itmp2); \
    dst += dstStride;

#define CALC_BLND_8ROWS(R1, R2, R3, R4, R5, R6, R7, R8, X) \
    PREDANG_CALCROW_HOR(0 + X, R1) \
    PREDANG_CALCROW_HOR(1 + X, R2) \
    PREDANG_CALCROW_HOR(2 + X, R3) \
    PREDANG_CALCROW_HOR(3 + X, R4) \
    PREDANG_CALCROW_HOR(4 + X, R5) \
    PREDANG_CALCROW_HOR(5 + X, R6) \
    PREDANG_CALCROW_HOR(6 + X, R7) \
    PREDANG_CALCROW_HOR(7 + X, R8) \
    MB4(R1, R2, R3, R4) \
    MB4(R5, R6, R7, R8) \
    BLND2_4(R1, R5); \
    BLND2_4(R2, R6); \
    BLND2_4(R3, R7); \
    BLND2_4(R4, R8);

void intraPredAng16x16(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
{
    int k;
    int blkSize        = 16;

    // Map the mode index to main prediction direction and angle
    assert(dirMode > 1); //no planar and dc
    bool modeHor       = (dirMode < 18);
    bool modeVer       = !modeHor;
    int intraPredAngle = modeVer ? (int)dirMode - VER_IDX : modeHor ? -((int)dirMode - HOR_IDX) : 0;
    int lookIdx = intraPredAngle;
    int absAng         = abs(intraPredAngle);
    int signAng        = intraPredAngle < 0 ? -1 : 1;

    // Set bitshifts and scale the angle parameter to block size
    int angTable[9]    = { 0,    2,    5,   9,  13,  17,  21,  26,  32 };
    int invAngTable[9] = { 0, 4096, 1638, 910, 630, 482, 390, 315, 256 }; // (256 * 32) / Angle
    int invAngle       = invAngTable[absAng];
    absAng             = angTable[absAng];
    intraPredAngle     = signAng * absAng;

    // Do angular predictions

    pixel* refMain;
    pixel* refSide;

    // Initialise the Main and Left reference array.
    if (intraPredAngle < 0)
    {
        refMain = (modeVer ? refAbove : refLeft);     // + (blkSize - 1);
        refSide = (modeVer ? refLeft : refAbove);     // + (blkSize - 1);

        // Extend the Main reference to the left.
        int invAngleSum    = 128;     // rounding for (shift by 8)
        if (intraPredAngle != -32)
            for (k = -1; k > blkSize * intraPredAngle >> 5; k--)
            {
                invAngleSum += invAngle;
                refMain[k] = refSide[invAngleSum >> 8];
            }
    }
    else
    {
        refMain = modeVer ? refAbove : refLeft;
        refSide = modeVer ? refLeft  : refAbove;
    }

    // bfilter will always be true for blocksize 8
    if (intraPredAngle == 0)  // Exactly hotizontal/vertical angles
    {
        if (modeHor)
        {
            __m128i v_temp;
            __m128i tmp1;
            v_temp = _mm_loadu_si128((__m128i*)(refMain + 1));

            if (bFilter)
            {
                __m128i v_side_0 = _mm_set1_epi16(refSide[0]); // refSide[0] value in a vector
                __m128i v_temp16;
                v_temp16 = _mm_loadu_si128((__m128i*)(refSide + 1));
                __m128i v_side;
                v_side = _mm_unpacklo_epi8(v_temp16, _mm_setzero_si128());

                __m128i row01, row02, ref;
                ref = _mm_set1_epi16(refMain[1]);
                v_side = _mm_sub_epi16(v_side, v_side_0);
                v_side = _mm_srai_epi16(v_side, 1);
                row01 = _mm_add_epi16(ref, v_side);
                row01 = _mm_min_epi16(_mm_max_epi16(_mm_setzero_si128(), row01), _mm_set1_epi16((1 << X265_DEPTH) - 1));

                v_side = _mm_unpackhi_epi8(v_temp16, _mm_setzero_si128());
                v_side = _mm_sub_epi16(v_side, v_side_0);
                v_side = _mm_srai_epi16(v_side, 1);
                row02 = _mm_add_epi16(ref, v_side);
                row02 = _mm_min_epi16(_mm_max_epi16(_mm_setzero_si128(), row02), _mm_set1_epi16((1 << X265_DEPTH) - 1));

                tmp1 = _mm_packus_epi16(row01, row02);
                _mm_storeu_si128((__m128i*)dst, tmp1);            //row0
            }
            else
            {
                tmp1 = _mm_shuffle_epi8(v_temp, _mm_set1_epi8(0));
                _mm_storeu_si128((__m128i*)dst, tmp1); //row0
            }

            tmp1 = _mm_shuffle_epi8(v_temp, _mm_set1_epi8(1));
            _mm_storeu_si128((__m128i*)(dst + (1 * dstStride)), tmp1); //row1

            tmp1 = _mm_shuffle_epi8(v_temp, _mm_set1_epi8(2));
            _mm_storeu_si128((__m128i*)(dst + (2 * dstStride)), tmp1); //row2

            tmp1 = _mm_shuffle_epi8(v_temp, _mm_set1_epi8(3));
            _mm_storeu_si128((__m128i*)(dst + (3 * dstStride)), tmp1); //row3

            tmp1 = _mm_shuffle_epi8(v_temp, _mm_set1_epi8(4));
            _mm_storeu_si128((__m128i*)(dst + (4 * dstStride)), tmp1); //row4

            tmp1 = _mm_shuffle_epi8(v_temp, _mm_set1_epi8(5));
            _mm_storeu_si128((__m128i*)(dst + (5 * dstStride)), tmp1); //row5

            tmp1 = _mm_shuffle_epi8(v_temp, _mm_set1_epi8(6));
            _mm_storeu_si128((__m128i*)(dst + (6 * dstStride)), tmp1); //row6

            tmp1 = _mm_shuffle_epi8(v_temp, _mm_set1_epi8(7));
            _mm_storeu_si128((__m128i*)(dst + (7 * dstStride)), tmp1); //row7

            tmp1 = _mm_shuffle_epi8(v_temp, _mm_set1_epi8(8));
            _mm_storeu_si128((__m128i*)(dst + (8 * dstStride)), tmp1); //row8

            tmp1 = _mm_shuffle_epi8(v_temp, _mm_set1_epi8(9));
            _mm_storeu_si128((__m128i*)(dst + (9 * dstStride)), tmp1); //row9

            tmp1 = _mm_shuffle_epi8(v_temp, _mm_set1_epi8(10));
            _mm_storeu_si128((__m128i*)(dst + (10 * dstStride)), tmp1); //row10

            tmp1 = _mm_shuffle_epi8(v_temp, _mm_set1_epi8(11));
            _mm_storeu_si128((__m128i*)(dst + (11 * dstStride)), tmp1); //row11

            tmp1 = _mm_shuffle_epi8(v_temp, _mm_set1_epi8(12));
            _mm_storeu_si128((__m128i*)(dst + (12 * dstStride)), tmp1); //row12

            tmp1 = _mm_shuffle_epi8(v_temp, _mm_set1_epi8(13));
            _mm_storeu_si128((__m128i*)(dst + (13 * dstStride)), tmp1); //row13

            tmp1 = _mm_shuffle_epi8(v_temp, _mm_set1_epi8(14));
            _mm_storeu_si128((__m128i*)(dst + (14 * dstStride)), tmp1); //row14

            tmp1 = _mm_shuffle_epi8(v_temp, _mm_set1_epi8(15));
            _mm_storeu_si128((__m128i*)(dst + (15 * dstStride)), tmp1); //row15
        }
        else
        {
            __m128i v_main;
            v_main = _mm_loadu_si128((__m128i const*)(refMain + 1));

            _mm_storeu_si128((__m128i*)dst, v_main);
            _mm_storeu_si128((__m128i*)(dst + dstStride), v_main);
            _mm_storeu_si128((__m128i*)(dst + (2 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(dst + (3 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(dst + (4 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(dst + (5 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(dst + (6 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(dst + (7 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(dst + (8 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(dst + (9 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(dst + (10 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(dst + (11 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(dst + (12 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(dst + (13 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(dst + (14 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(dst + (15 * dstStride)), v_main);

            if (bFilter)
            {
                __m128i v_temp;
                __m128i v_side_0 = _mm_set1_epi16(refSide[0]); // refSide[0] value in a vector

                v_temp = _mm_loadu_si128((__m128i*)(refSide + 1));
                __m128i v_side;
                v_side = _mm_unpacklo_epi8(v_temp, _mm_setzero_si128());

                __m128i row0, ref;
                ref = _mm_set1_epi16(refMain[1]);
                v_side = _mm_sub_epi16(v_side, v_side_0);
                v_side = _mm_srai_epi16(v_side, 1);
                row0 = _mm_add_epi16(ref, v_side);
                row0 = _mm_min_epi16(_mm_max_epi16(_mm_setzero_si128(), row0), _mm_set1_epi16((1 << X265_DEPTH) - 1));

                uint16_t x[8];
                _mm_storeu_si128((__m128i*)x, row0);
                dst[0 * dstStride] = x[0];
                dst[1 * dstStride] = x[1];
                dst[2 * dstStride] = x[2];
                dst[3 * dstStride] = x[3];
                dst[4 * dstStride] = x[4];
                dst[5 * dstStride] = x[5];
                dst[6 * dstStride] = x[6];
                dst[7 * dstStride] = x[7];

                v_side = _mm_unpackhi_epi8(v_temp, _mm_setzero_si128());
                v_side = _mm_sub_epi16(v_side, v_side_0);
                v_side = _mm_srai_epi16(v_side, 1);
                row0 = _mm_add_epi16(ref, v_side);
                row0 = _mm_min_epi16(_mm_max_epi16(_mm_setzero_si128(), row0), _mm_set1_epi16((1 << X265_DEPTH) - 1));

                _mm_storeu_si128((__m128i*)x, row0);
                dst[8 * dstStride] = x[0];
                dst[9 * dstStride] = x[1];
                dst[10 * dstStride] = x[2];
                dst[11 * dstStride] = x[3];
                dst[12 * dstStride] = x[4];
                dst[13 * dstStride] = x[5];
                dst[14 * dstStride] = x[6];
                dst[15 * dstStride] = x[7];
            }
        }
    }
    else if (intraPredAngle == -32)
    {
        __m128i v_refSide;
        v_refSide = _mm_loadu_si128((__m128i*)refSide);
        __m128i temp = _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
        v_refSide = _mm_shuffle_epi8(v_refSide, temp);
        pixel refMain0 = refMain[0];

        _mm_storeu_si128((__m128i*)(refMain - 15), v_refSide);
        refMain[0] = refMain0;

        __m128i itmp;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);

        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);

        return;
    }
    else if (intraPredAngle == 32)
    {
        __m128i itmp;
        refMain += 2;

        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        _mm_storeu_si128((__m128i*)dst, itmp);

        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);

        return;
    }
    else
    {
        if (modeHor)
        {
            __m128i row11L, row12L, row11H, row12H;
            __m128i v_deltaFract;
            __m128i v_deltaPos = _mm_setzero_si128();
            __m128i thirty2 = _mm_set1_epi16(32);
            __m128i thirty1 = _mm_set1_epi16(31);
            __m128i v_ipAngle = _mm_set1_epi16(intraPredAngle);
            __m128i R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, R13, R14, R15, R16;
            __m128i itmp, itmp1, itmp2, it1, it2, it3, i16;
//            MB16;
            CALC_BLND_8ROWS(R1, R2, R3, R4, R5, R6, R7, R8, 0)
            CALC_BLND_8ROWS(R9, R10, R11, R12, R13, R14, R15, R16, 8)
            BLND2_2(R1, R9)
            BLND2_2(R5, R13)
            BLND2_2(R3, R11)
            BLND2_2(R7, R15)
            BLND2_2(R2, R10)
            BLND2_2(R6, R14)
            BLND2_2(R4, R12)
            BLND2_2(R8, R16)
        }
        else
        {
            __m128i row11L, row12L, row11H, row12H;
            __m128i v_deltaFract;
            __m128i v_deltaPos = _mm_setzero_si128();
            __m128i thirty2 = _mm_set1_epi16(32);
            __m128i thirty1 = _mm_set1_epi16(31);
            __m128i v_ipAngle = _mm_set1_epi16(intraPredAngle);
            __m128i itmp, it1, it2, it3, i16;

            PREDANG_CALCROW_VER(0);
            PREDANG_CALCROW_VER(1);
            PREDANG_CALCROW_VER(2);
            PREDANG_CALCROW_VER(3);
            PREDANG_CALCROW_VER(4);
            PREDANG_CALCROW_VER(5);
            PREDANG_CALCROW_VER(6);
            PREDANG_CALCROW_VER(7);
            PREDANG_CALCROW_VER(8);
            PREDANG_CALCROW_VER(9);
            PREDANG_CALCROW_VER(10);
            PREDANG_CALCROW_VER(11);
            PREDANG_CALCROW_VER(12);
            PREDANG_CALCROW_VER(13);
            PREDANG_CALCROW_VER(14);
            PREDANG_CALCROW_VER(15);
        }
    }
}

#undef PREDANG_CALCROW_VER
#undef PREDANG_CALCROW_HOR
#undef LOADROW
#undef CALCROW
#undef BLND2_16
#undef BLND2_2
#undef BLND2_4
#undef MB4
#undef CALC_BLND_8ROWS

#endif // !HIGH_BIT_DEPTH
}

namespace x265 {
void Setup_Vec_IPredPrimitives_ssse3(EncoderPrimitives& p)
{
#if !HIGH_BIT_DEPTH
    for (int i = 2; i < NUM_INTRA_MODE; i++)
    {
        p.intra_pred[BLOCK_16x16][i] = intraPredAng16x16;
    }
#endif
}
}
