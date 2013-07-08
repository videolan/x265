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
#include <smmintrin.h>

extern char g_aucConvertToBit[];
extern unsigned char IntraFilterType[][35];

using namespace x265;

#define PRED_INTRA_ANG4_START   \
    Vec8s row11, row12, row21, row22, row31, row32, row41, row42;   \
    Vec16uc tmp16_1, tmp16_2;   \
    Vec2uq tmp2uq;  \
    Vec8s v_deltaFract, v_deltaPos(0), thirty2(32), thirty1(31), v_ipAngle(0);  \
    bool modeHor = (dirMode < 18);                                              


#define PRED_INTRA_ANG4_END \
    v_deltaFract = v_deltaPos & thirty1;    \
    row11 = ((thirty2 - v_deltaFract) * row11 + (v_deltaFract * row12) + 16) >> 5;  \
    /*row2*/    \
    v_deltaPos += v_ipAngle;    \
    v_deltaFract = v_deltaPos & thirty1;    \
    row21 = ((thirty2 - v_deltaFract) * row21 + (v_deltaFract * row22) + 16) >> 5;  \
    /*row3*/    \
    v_deltaPos += v_ipAngle;    \
    v_deltaFract = v_deltaPos & thirty1;    \
    row31 = ((thirty2 - v_deltaFract) * row31 + (v_deltaFract * row32) + 16) >> 5;  \
    /*row4*/    \
    v_deltaPos += v_ipAngle;    \
    v_deltaFract = v_deltaPos & thirty1;    \
    row41 = ((thirty2 - v_deltaFract) * row41 + (v_deltaFract * row42) + 16) >> 5;  \
    /* Flip the block */    \
    if (modeHor)    \
    {   \
        Vec8s tmp1, tmp2, tmp3, tmp4;   \
        tmp1 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>(row11, row31); \
        tmp2 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>(row21, row41); \
        tmp3 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>(tmp1, tmp2);   \
        tmp4 = blend8s<4, 12, 5, 13, 6, 14, 7, 15>(tmp1, tmp2); \
        tmp16_1 = compress_unsafe(tmp3, tmp3);  \
        store_partial(const_int(4), pDst, tmp16_1); \
        tmp2uq = reinterpret_i(tmp16_1);    \
        tmp2uq >>= 32;  \
        store_partial(const_int(4), pDst + dstStride, tmp2uq);  \
        tmp16_1 = compress_unsafe(tmp4, tmp4);  \
        store_partial(const_int(4), pDst + (2 * dstStride), tmp16_1);   \
        tmp2uq = reinterpret_i(tmp16_1);    \
        tmp2uq >>= 32;  \
        store_partial(const_int(4), pDst + (3 * dstStride), tmp2uq);    \
    }   \
    else    \
    {   \
        store_partial(const_int(4), pDst, compress_unsafe(row11, row11));   \
        store_partial(const_int(4), pDst + (dstStride), compress_unsafe(row21, row21)); \
        store_partial(const_int(4), pDst + (2 * dstStride), compress_unsafe(row31, row31)); \
        store_partial(const_int(4), pDst + (3 * dstStride), compress_unsafe(row41, row41)); \
    }

#define PRED_INTRA_ANG8_START   \
    /* Map the mode index to main prediction direction and angle*/    \
    bool modeHor       = (dirMode < 18);    \
    bool modeVer       = !modeHor;  \
    int intraPredAngle = modeVer ? (int)dirMode - VER_IDX : modeHor ? -((int)dirMode - HOR_IDX) : 0;    \
    int absAng         = abs(intraPredAngle);   \
    int signAng        = intraPredAngle < 0 ? -1 : 1;   \
    /* Set bitshifts and scale the angle parameter to block size*/  \
    int angTable[9]    = { 0,    2,    5,   9,  13,  17,  21,  26,  32 };   \
    absAng             = angTable[absAng];  \
    intraPredAngle     = signAng * absAng;  \
    if (modeHor)         /* Near horizontal modes*/   \
    {\
        Vec16uc tmp;    \
        Vec8s row11, row12; \
        Vec16uc row1, row2, row3, row4, tmp16_1, tmp16_2;   \
        Vec8s v_deltaFract, v_deltaPos, thirty2(32), thirty1(31), v_ipAngle;    \
        Vec8s tmp1, tmp2;   \
        v_deltaPos = 0; \
        v_ipAngle = intraPredAngle; \

#define PRED_INTRA_ANG8_MIDDLE   \
        /* Flip the block */    \
        tmp16_1 = blend16uc<0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23>(row1, row2);    \
        tmp16_2 = blend16uc<8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31>(row1, row2);  \
        row1 = tmp16_1; \
        row2 = tmp16_2; \
        tmp16_1 = blend16uc<0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23>(row3, row4);    \
        tmp16_2 = blend16uc<8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31>(row3, row4);  \
        row3 = tmp16_1; \
        row4 = tmp16_2; \
        tmp16_1 = blend16uc<0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23>(row1, row2);    \
        tmp16_2 = blend16uc<8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31>(row1, row2);  \
        row1 = tmp16_1; \
        row2 = tmp16_2; \
        tmp16_1 = blend16uc<0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23>(row3, row4);    \
        tmp16_2 = blend16uc<8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31>(row3, row4);  \
        row3 = tmp16_1; \
        row4 = tmp16_2; \
        tmp16_1 = blend4i<0, 4, 1, 5>((Vec4i)row1, (Vec4i)row3);    \
        tmp16_2 = blend4i<2, 6, 3, 7>((Vec4i)row1, (Vec4i)row3);    \
        row1 = tmp16_1; \
        row3 = tmp16_2; \
        tmp16_1 = blend4i<0, 4, 1, 5>((Vec4i)row2, (Vec4i)row4);    \
        tmp16_2 = blend4i<2, 6, 3, 7>((Vec4i)row2, (Vec4i)row4);    \
        row2 = tmp16_1; \
        row4 = tmp16_2; \
        store_partial(const_int(8), pDst, row1);   /*row1*/   \
        store_partial(const_int(8), pDst + (2 * dstStride), row3);   /*row3*/   \
        store_partial(const_int(8), pDst + (4 * dstStride), row2);   /*row5*/   \
        store_partial(const_int(8), pDst + (6 * dstStride), row4);   /*row7*/   \
        row1 = blend2q<1, 3>((Vec2q)row1, (Vec2q)row1); \
        store_partial(const_int(8), pDst + (1 * dstStride), row1);   /*row2*/   \
        row1 = blend2q<1, 3>((Vec2q)row3, (Vec2q)row3); \
        store_partial(const_int(8), pDst + (3 * dstStride), row1);   /*row4*/   \
        row1 = blend2q<1, 3>((Vec2q)row2, (Vec2q)row2);    \
        store_partial(const_int(8), pDst + (5 * dstStride), row1);   /*row6*/   \
        row1 = blend2q<1, 3>((Vec2q)row4, (Vec2q)row4); \
        store_partial(const_int(8), pDst + (7 * dstStride), row1);   /*row8*/   \
    }   \
    else                         /* Vertical modes*/    \
    {\
        Vec8s row11, row12; \
        Vec8s v_deltaFract, v_deltaPos, thirty2(32), thirty1(31), v_ipAngle;    \
        Vec16uc tmp;    \
        Vec8s tmp1, tmp2;   \
        v_deltaPos = 0; \
        v_ipAngle = intraPredAngle; \


namespace {
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

__m128i v_multiL, v_multiH, v_multiH2, v_multiH3, v_multiH4, v_multiH5, v_multiH6, v_multiH7;
__m128i v_multi_2Row;

/* When compiled with /arch:AVX, this code is not safe to run on non-AVX CPUs and
 * thus we cannot use static initialization.  This routine is only called if the
 * detected CPU can support this SIMD architecture. */
static void initFileStaticVars()
{
    v_multiL = _mm_setr_epi16(1, 2, 3, 4, 5, 6, 7, 8);
    v_multiH = _mm_setr_epi16(9, 10, 11, 12, 13, 14, 15, 16);
    v_multiH2 = _mm_setr_epi16(17, 18, 19, 20, 21, 22, 23, 24);
    v_multiH3 = _mm_setr_epi16(25, 26, 27, 28, 29, 30, 31, 32);
    v_multiH4 = _mm_setr_epi16(33, 34, 35, 36, 37, 38, 39, 40);
    v_multiH5 = _mm_setr_epi16(41, 42, 43, 44, 45, 46, 47, 48);
    v_multiH6 = _mm_setr_epi16(49, 50, 51, 52, 53, 54, 55, 56);
    v_multiH7 = _mm_setr_epi16(57, 58, 59, 60, 61, 62, 63, 64);
    v_multi_2Row = _mm_setr_epi16(1, 2, 3, 4, 1, 2, 3, 4);
}

static inline
void predDCFiltering(pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int width, int /*height*/)
{
    pixel* pDst = dst;
    int y;
    pixel pixDC = *pDst;
    int pixDCx3 = pixDC * 3 + 2;

    // boundary pixels processing
    pDst[0] = (pixel)((src[-srcStride] + src[-1] + 2 * pixDC + 2) >> 2);

    Vec8us im1(pixDCx3);
    Vec8us im2, im3;
#if HIGH_BIT_DEPTH
    switch (width)
    {
    case 4:
        im2 = load_partial(const_int(8), &src[1 - srcStride]);
        im2 = (im1 + im2) >> const_int(2);
        store_partial(const_int(8), &dst[1], im2);
        break;

    case 8:
        im2.load(&src[1 - srcStride]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&dst[1]);
        break;

    case 16:
        im2.load(&src[1 - srcStride]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&dst[1]);

        im2.load(&src[1 - srcStride + 8]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&dst[1 + 8]);
        break;

    case 32:
        im2.load(&src[1 - srcStride]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&dst[1]);

        im2.load(&src[1 - srcStride + 8]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&pDst[1 + 8]);

        im2.load(&src[1 - srcStride + 16]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&pDst[1 + 16]);

        im2.load(&src[1 - srcStride + 24]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&pDst[1 + 24]);
        break;

    //case 64:
    default:
        im2.load(&src[1 - srcStride]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&pDst[1]);

        im2.load(&src[1 - srcStride + 8]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&pDst[1 + 8]);

        im2.load(&src[1 - srcStride + 16]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&pDst[1 + 16]);

        im2.load(&src[1 - srcStride + 24]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&pDst[1 + 24]);

        im2.load(&src[1 - srcStride + 32]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&pDst[1 + 32]);

        im2.load(&src[1 - srcStride + 40]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&pDst[1 + 40]);

        im2.load(&src[1 - srcStride + 48]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&pDst[1 + 48]);

        im2.load(&src[1 - srcStride + 56]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&pDst[1 + 56]);
        break;
    }

#else /* if HIGH_BIT_DEPTH */
    Vec16uc pix;
    switch (width)
    {
    case 4:
        pix = load_partial(const_int(4), &src[1 - srcStride]);
        im2 = extend_low(pix);
        im2 = (im1 + im2) >> const_int(2);
        pix = compress(im2, im2);
        store_partial(const_int(4), &dst[1], pix);
        break;

    case 8:
        pix = load_partial(const_int(8), &src[1 - srcStride]);
        im2 = extend_low(pix);
        im2 = (im1 + im2) >> const_int(2);
        pix = compress(im2, im2);
        store_partial(const_int(8), &dst[1], pix);
        break;

    case 16:
        pix.load(&src[1 - srcStride]);
        im2 = extend_low(pix);
        im3 = extend_high(pix);
        im2 = (im1 + im2) >> const_int(2);
        im3 = (im1 + im3) >> const_int(2);
        pix = compress(im2, im3);
        pix.store(&dst[1]);
        break;

    case 32:
        pix.load(&src[1 - srcStride]);
        im2 = extend_low(pix);
        im3 = extend_high(pix);
        im2 = (im1 + im2) >> const_int(2);
        im3 = (im1 + im3) >> const_int(2);
        pix = compress(im2, im3);
        pix.store(&dst[1]);

        pix.load(&src[1 - srcStride + 16]);
        im2 = extend_low(pix);
        im3 = extend_high(pix);
        im2 = (im1 + im2) >> const_int(2);
        im3 = (im1 + im3) >> const_int(2);
        pix = compress(im2, im3);
        pix.store(&dst[1 + 16]);
        break;

    //case 64:
    default:
        pix.load(&src[1 - srcStride]);
        im2 = extend_low(pix);
        im3 = extend_high(pix);
        im2 = (im1 + im2) >> const_int(2);
        im3 = (im1 + im3) >> const_int(2);
        pix = compress(im2, im3);
        pix.store(&dst[1]);

        pix.load(&src[1 - srcStride + 16]);
        im2 = extend_low(pix);
        im3 = extend_high(pix);
        im2 = (im1 + im2) >> const_int(2);
        im3 = (im1 + im3) >> const_int(2);
        pix = compress(im2, im3);
        pix.store(&dst[1 + 16]);

        pix.load(&src[1 - srcStride + 32]);
        im2 = extend_low(pix);
        im3 = extend_high(pix);
        im2 = (im1 + im2) >> const_int(2);
        im3 = (im1 + im3) >> const_int(2);
        pix = compress(im2, im3);
        pix.store(&dst[1 + 32]);

        pix.load(&src[1 - srcStride + 48]);
        im2 = extend_low(pix);
        im3 = extend_high(pix);
        im2 = (im1 + im2) >> const_int(2);
        im3 = (im1 + im3) >> const_int(2);
        pix = compress(im2, im3);
        pix.store(&dst[1 + 48]);
        break;
    }

#endif /* if HIGH_BIT_DEPTH */

    for (y = 1; y < width; y++)
    {
        dst[dstStride] = (pixel)((src[srcStride - 1] + pixDCx3) >> 2);
        src += srcStride;
        dst += dstStride;
    }
}

void predIntraDC(pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int width, int filter)
{
    int sum = 0;
    int logSize = g_aucConvertToBit[width] + 2;
    pixel *srcAbove = &src[-srcStride];
    pixel *srcLeft = &src[-1];

    for (int iInd = 0; iInd < width; iInd++)
    {
        sum += *srcLeft;
        srcLeft += srcStride;
    }

#if HIGH_BIT_DEPTH
    Vec8s sumAbove(0);
    Vec8s m0;

    switch (width)
    {
    case 4:
        sumAbove = load_partial(const_int(8), srcAbove);
        break;
    case 8:
        m0.load(srcAbove);
        sumAbove = m0;
        break;
    case 16:
        m0.load(srcAbove);
        sumAbove  = m0;
        m0.load(srcAbove + 8);
        sumAbove += m0;
        break;
    case 32:
        m0.load(srcAbove);
        sumAbove  = m0;
        m0.load(srcAbove + 8);
        sumAbove += m0;
        m0.load(srcAbove + 16);
        sumAbove += m0;
        m0.load(srcAbove + 24);
        sumAbove += m0;
        break;
        //case 64:
    default:
        // CHECK_ME: the max support bit_depth is 13-bits
        m0.load(srcAbove);
        sumAbove  = m0;
        m0.load(srcAbove + 8);
        sumAbove += m0;
        m0.load(srcAbove + 16);
        sumAbove += m0;
        m0.load(srcAbove + 24);
        sumAbove += m0;
        m0.load(srcAbove + 32);
        sumAbove += m0;
        m0.load(srcAbove + 40);
        sumAbove += m0;
        m0.load(srcAbove + 48);
        sumAbove += m0;
        m0.load(srcAbove + 56);
        sumAbove += m0;
        break;
    }

    sum += horizontal_add_x(sumAbove);

    logSize += 1;
    pixel dcVal = (sum + (1 << (logSize - 1))) >> logSize;
    Vec8us dcValN(dcVal);
    int k;

    pixel *dst1 = dst;
    switch (width)
    {
    case 4:
        store_partial(const_int(8), dst1, dcValN);
        dst1 += dstStride;
        store_partial(const_int(8), dst1, dcValN);
        dst1 += dstStride;
        store_partial(const_int(8), dst1, dcValN);
        dst1 += dstStride;
        store_partial(const_int(8), dst1, dcValN);
        dst1 += dstStride;
        break;

    case 8:
        dcValN.store(dst1);
        dst1 += dstStride;
        dcValN.store(dst1);
        dst1 += dstStride;
        dcValN.store(dst1);
        dst1 += dstStride;
        dcValN.store(dst1);
        dst1 += dstStride;
        dcValN.store(dst1);
        dst1 += dstStride;
        dcValN.store(dst1);
        dst1 += dstStride;
        dcValN.store(dst1);
        dst1 += dstStride;
        dcValN.store(dst1);
        dst1 += dstStride;
        break;

    case 16:
        for (k = 0; k < 16; k += 2)
        {
            dcValN.store(dst1);
            dcValN.store(dst1 + 8);
            dst1 += dstStride;
            dcValN.store(dst1);
            dcValN.store(dst1 + 8);
            dst1 += dstStride;
        }

        break;

    case 32:
        for (k = 0; k < 32; k++)
        {
            dcValN.store(dst1);
            dcValN.store(dst1 +  8);
            dcValN.store(dst1 + 16);
            dcValN.store(dst1 + 24);
            dst1 += dstStride;
        }

        break;

    //case 64:
    default:
        for (k = 0; k < 64; k++)
        {
            dcValN.store(dst1);
            dcValN.store(dst1 +  8);
            dcValN.store(dst1 + 16);
            dcValN.store(dst1 + 24);
            dcValN.store(dst1 + 32);
            dcValN.store(dst1 + 40);
            dcValN.store(dst1 + 48);
            dcValN.store(dst1 + 56);
            dst1 += dstStride;
        }

        break;
    }

    if (filter)
    {
        predDCFiltering(src, srcStride, dst, dstStride, width, width);
    }
#else // if !HIGH_BIT_DEPTH

    {
        Vec16uc pix;
        Vec8us  im;
        Vec4ui  im1, im2;

        switch (width)
        {
        case 4:
            pix.fromUint32(*(uint32_t*)srcAbove);
            sum += horizontal_add(extend_low(pix));
            break;
        case 8:
#if X86_64
            pix.fromUint64(*(uint64_t*)srcAbove);
#else
            pix.load_partial(8, srcAbove);
#endif
            sum += horizontal_add(extend_low(pix));
            break;
        case 16:
            pix.load(srcAbove);
            sum += horizontal_add_x(pix);
            break;
        case 32:
            pix.load(srcAbove);
            im1 = (Vec4ui)(pix.sad(_mm_setzero_si128()));
            pix.load(srcAbove + 16);
            im1 += (Vec4ui)(pix.sad(_mm_setzero_si128()));
            im1 += (Vec4ui)((Vec128b)im1 >> const_int(64));
            sum += toInt32(im1);
            break;
        //case 64:
        default:
            pix.load(srcAbove);
            im1 = (Vec4ui)(pix.sad(_mm_setzero_si128()));
            pix.load(srcAbove + 16);
            im1 += (Vec4ui)(pix.sad(_mm_setzero_si128()));
            pix.load(srcAbove + 32);
            im1 += (Vec4ui)(pix.sad(_mm_setzero_si128()));
            pix.load(srcAbove + 48);
            im1 += (Vec4ui)(pix.sad(_mm_setzero_si128()));
            im1 += (Vec4ui)((Vec128b)im1 >> const_int(64));
            sum += toInt32(im1);
            break;
        }
    }

    logSize += 1;
    pixel dcVal = (sum + (1 << (logSize - 1))) >> logSize;
    Vec16uc dcValN(dcVal);
    int k;

    pixel *dst1 = dst;
    switch (width)
    {
    case 4:
        store_partial(const_int(4), dst1, dcValN);
        dst1 += dstStride;
        store_partial(const_int(4), dst1, dcValN);
        dst1 += dstStride;
        store_partial(const_int(4), dst1, dcValN);
        dst1 += dstStride;
        store_partial(const_int(4), dst1, dcValN);
        break;

    case 8:
        store_partial(const_int(8), dst1, dcValN);
        dst1 += dstStride;
        store_partial(const_int(8), dst1, dcValN);
        dst1 += dstStride;
        store_partial(const_int(8), dst1, dcValN);
        dst1 += dstStride;
        store_partial(const_int(8), dst1, dcValN);
        dst1 += dstStride;
        store_partial(const_int(8), dst1, dcValN);
        dst1 += dstStride;
        store_partial(const_int(8), dst1, dcValN);
        dst1 += dstStride;
        store_partial(const_int(8), dst1, dcValN);
        dst1 += dstStride;
        store_partial(const_int(8), dst1, dcValN);
        break;

    case 16:
        for (k = 0; k < 16; k += 4)
        {
            store_partial(const_int(16), dst1, dcValN);
            dst1 += dstStride;
            store_partial(const_int(16), dst1, dcValN);
            dst1 += dstStride;
            store_partial(const_int(16), dst1, dcValN);
            dst1 += dstStride;
            store_partial(const_int(16), dst1, dcValN);
            dst1 += dstStride;
        }

        break;

    case 32:
        for (k = 0; k < 32; k += 2)
        {
            store_partial(const_int(16), dst1,      dcValN);
            store_partial(const_int(16), dst1 + 16, dcValN);
            dst1 += dstStride;
            store_partial(const_int(16), dst1,      dcValN);
            store_partial(const_int(16), dst1 + 16, dcValN);
            dst1 += dstStride;
        }

        break;

    case 64:
        for (k = 0; k < 64; k++)
        {
            store_partial(const_int(16), dst1,      dcValN);
            store_partial(const_int(16), dst1 + 16, dcValN);
            store_partial(const_int(16), dst1 + 32, dcValN);
            store_partial(const_int(16), dst1 + 48, dcValN);
            dst1 += dstStride;
        }

        break;
    }

    if (filter)
    {
        predDCFiltering(src, srcStride, dst, dstStride, width, width);
    }
#endif // if HIGH_BIT_DEPTH
}

#if HIGH_BIT_DEPTH
// CHECK_ME: I am not sure the v_rightColumnN will be overflow when input as 12bpp
void predIntraPlanar4(pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride)
{
    int k, bottomLeft, topRight;
    // NOTE: I use 16-bits is enough here, because we have least than 13-bits as input, and shift left by 2, it is 15-bits
    int16_t leftColumn[4];

    // Get left and above reference column and row
    Vec8s v_topRow = (Vec8s)load_partial(const_int(8), &src[-srcStride]); // topRow

    for (k = 0; k < 4; k++)
    {
        leftColumn[k] = src[k * srcStride - 1];
    }

    Vec8s v_leftColumn = (Vec8s)load_partial(const_int(8), leftColumn);   // leftColumn

    // Prepare intermediate variables used in interpolation
    bottomLeft = src[4 * srcStride - 1];
    topRight   = src[4 - srcStride];

    Vec8s v_bottomLeft(bottomLeft);
    Vec8s v_topRight(topRight);

    Vec8s v_bottomRow = v_bottomLeft - v_topRow;
    Vec8s v_rightColumn = v_topRight - v_leftColumn;

    v_topRow = v_topRow << const_int(2);
    v_leftColumn = v_leftColumn << const_int(2);

    // Generate prediction signal
    Vec8s v_horPred4 = v_leftColumn + Vec8s(4);
    const Vec8s v_multi(1, 2, 3, 4, 5, 6, 7, 8);
    Vec8s v_horPred, v_rightColumnN;
    Vec8s v_im4;
    Vec16uc v_im5;

    // line0
    v_horPred = broadcast(const_int(0), v_horPred4);
    v_rightColumnN = broadcast(const_int(0), v_rightColumn) * v_multi;
    v_horPred = v_horPred + v_rightColumnN;
    v_topRow = v_topRow + v_bottomRow;
    // CHECK_ME: the HM don't clip the pixel, so I assume there is biggest 12+3=15(bits)
    v_im4 = (Vec8s)(v_horPred + v_topRow) >> const_int(3);
    store_partial(const_int(8), &dst[0 * dstStride], v_im4);

    // line1
    v_horPred = broadcast(const_int(1), v_horPred4);
    v_rightColumnN = broadcast(const_int(1), v_rightColumn) * v_multi;
    v_horPred = v_horPred + v_rightColumnN;
    v_topRow = v_topRow + v_bottomRow;
    v_im4 = (Vec8s)(v_horPred + v_topRow) >> const_int(3);
    store_partial(const_int(8), &dst[1 * dstStride], v_im4);

    // line2
    v_horPred = broadcast(const_int(2), v_horPred4);
    v_rightColumnN = broadcast(const_int(2), v_rightColumn) * v_multi;
    v_horPred = v_horPred + v_rightColumnN;
    v_topRow = v_topRow + v_bottomRow;
    v_im4 = (Vec8s)(v_horPred + v_topRow) >> const_int(3);
    store_partial(const_int(8), &dst[2 * dstStride], v_im4);

    // line3
    v_horPred = broadcast(const_int(3), v_horPred4);
    v_rightColumnN = broadcast(const_int(3), v_rightColumn) * v_multi;
    v_horPred = v_horPred + v_rightColumnN;
    v_topRow = v_topRow + v_bottomRow;
    v_im4 = (Vec8s)(v_horPred + v_topRow) >> const_int(3);
    store_partial(const_int(8), &dst[3 * dstStride], v_im4);
}

#else /* if HIGH_BIT_DEPTH */
void predIntraPlanar4(pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride)
{
    int k;
    pixel bottomLeft, topRight;

    // Get left and above reference column and row
    Vec16uc im0 = (Vec16uc)load_partial(const_int(4), &src[-srcStride]); // topRow
    Vec8s v_topRow = extend_low(im0);

    int16_t leftColumn[4];

    for (k = 0; k < 4; k++)
    {
        leftColumn[k] = src[k * srcStride - 1];
    }

    Vec8s v_leftColumn = (Vec8s)load_partial(const_int(8), (void*)leftColumn);   // leftColumn

    // Prepare intermediate variables used in interpolation
    bottomLeft = src[4 * srcStride - 1];
    topRight   = src[4 - srcStride];

    Vec8s v_bottomLeft(bottomLeft);
    Vec8s v_topRight(topRight);

    Vec8s v_bottomRow = v_bottomLeft - v_topRow;
    Vec8s v_rightColumn = v_topRight - v_leftColumn;

    v_topRow = v_topRow << const_int(2);
    v_leftColumn = v_leftColumn << const_int(2);

    Vec8s v_horPred4 = v_leftColumn + Vec8s(4);
    const Vec8s v_multi(1, 2, 3, 4, 5, 6, 7, 8);
    Vec8s v_horPred, v_rightColumnN;
    Vec8s v_im4;
    Vec16uc v_im5;

#define COMP_PRED_PLANAR4_ROW(X) { \
        v_horPred = broadcast(const_int((X)), v_horPred4); \
        v_rightColumnN = broadcast(const_int((X)), v_rightColumn) * v_multi; \
        v_horPred = v_horPred + v_rightColumnN; \
        v_topRow = v_topRow + v_bottomRow; \
        v_im4 = (Vec8s)(v_horPred + v_topRow) >> const_int(3); \
        v_im5 = compress_unsafe(v_im4, v_im4); \
        store_partial(const_int(4), &dst[(X)*dstStride], v_im5); \
}

    COMP_PRED_PLANAR4_ROW(0)
    COMP_PRED_PLANAR4_ROW(1)
    COMP_PRED_PLANAR4_ROW(2)
    COMP_PRED_PLANAR4_ROW(3)

#undef COMP_PRED_PLANAR4_ROW
}

#if INSTRSET >= 5
void predIntraPlanar4_sse4(pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride)
{
    pixel bottomLeft, topRight;

    // Get left and above reference column and row
    __m128i im0 = _mm_cvtsi32_si128(*(uint32_t*)&src[-srcStride]); // topRow
    __m128i v_topRow = _mm_unpacklo_epi8(im0, _mm_setzero_si128());

    v_topRow = _mm_shuffle_epi32(v_topRow, 0x44);

    // Prepare intermediate variables used in interpolation
    bottomLeft = src[4 * srcStride - 1];
    topRight   = src[4 - srcStride];

    __m128i v_bottomLeft = _mm_set1_epi16(bottomLeft);
    __m128i v_bottomRow   = _mm_sub_epi16(v_bottomLeft, v_topRow);

    v_topRow = _mm_slli_epi16(v_topRow, 2);

    __m128i v_horPred, v_rightColumnN;
    __m128i v_im4;
    __m128i v_im5;
    __m128i _tmp0, _tmp1;

    __m128i v_bottomRowL = _mm_unpacklo_epi64(v_bottomRow, _mm_setzero_si128());
    v_topRow = _mm_sub_epi16(v_topRow, v_bottomRowL);
    v_bottomRow = _mm_slli_epi16(v_bottomRow, 1);

#define COMP_PRED_PLANAR_2ROW(Y) { \
        _tmp0 = _mm_cvtsi32_si128((src[((Y)) * srcStride - 1] << 2) + 4); \
        _tmp0 = _mm_shufflelo_epi16(_tmp0, 0); \
        _tmp1 = _mm_cvtsi32_si128((src[((Y)+1) * srcStride - 1] << 2) + 4); \
        _tmp1 = _mm_shufflelo_epi16(_tmp1, 0); \
        v_horPred = _mm_unpacklo_epi64(_tmp0, _tmp1); \
        _tmp0 = _mm_cvtsi32_si128(topRight - src[((Y)) * srcStride - 1]); \
        _tmp0 = _mm_shufflelo_epi16(_tmp0, 0); \
        _tmp1 = _mm_cvtsi32_si128(topRight - src[((Y)+1) * srcStride - 1]); \
        _tmp1 = _mm_shufflelo_epi16(_tmp1, 0); \
        v_rightColumnN = _mm_unpacklo_epi64(_tmp0, _tmp1); \
        v_rightColumnN = _mm_mullo_epi16(v_rightColumnN, v_multi_2Row); \
        v_horPred = _mm_add_epi16(v_horPred, v_rightColumnN); \
        v_topRow = _mm_add_epi16(v_topRow, v_bottomRow); \
        v_im4 = _mm_srai_epi16(_mm_add_epi16(v_horPred, v_topRow), 3); \
        v_im5 = _mm_packus_epi16(v_im4, v_im4); \
        *(uint32_t*)&dst[(Y)*dstStride] = _mm_cvtsi128_si32(v_im5); \
        *(uint32_t*)&dst[((Y)+1) * dstStride] = _mm_cvtsi128_si32(_mm_shuffle_epi32(v_im5, 0x55));; \
}

    COMP_PRED_PLANAR_2ROW(0)
    COMP_PRED_PLANAR_2ROW(2)

#undef COMP_PRED_PLANAR4_ROW
}

#endif // INSTRSET >= 5

#endif /* if HIGH_BIT_DEPTH */

#if HIGH_BIT_DEPTH

#define COMP_PRED_PLANAR_ROW(X) { \
        v_horPred = permute8s<X, X, X, X, X, X, X, X>(v_horPred4); \
        v_rightColumnN = permute8s<X, X, X, X, X, X, X, X>(v_rightColumn) * v_multi; \
        v_horPred = v_horPred + v_rightColumnN; \
        v_topRow = v_topRow + v_bottomRow; \
        v_im4 = (Vec8s)(v_horPred + v_topRow) >> (3 + shift); \
        store_partial(const_int(16), &dst[X * dstStride], v_im4); \
}

void predIntraPlanar8(pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride)
{
    int k, bottomLeft, topRight;

    int16_t leftColumn[8];

    // Get left and above reference column and row
    Vec8s v_topRow = (Vec8s)load_partial(const_int(16), &src[-srcStride]); // topRow

    for (k = 0; k < 8; k++)
    {
        leftColumn[k] = src[k * srcStride - 1];
    }

    Vec8s v_leftColumn = (Vec8s)load_partial(const_int(16), leftColumn);   // leftColumn

    // Prepare intermediate variables used in interpolation
    bottomLeft = src[8 * srcStride - 1];
    topRight   = src[8 - srcStride];

    Vec8s v_bottomLeft(bottomLeft);
    Vec8s v_topRight(topRight);

    Vec8s v_bottomRow = v_bottomLeft - v_topRow;
    Vec8s v_rightColumn = v_topRight - v_leftColumn;

    int shift = g_aucConvertToBit[8];          // Using value corresponding to width = 8
    v_topRow = v_topRow << (2 + shift);
    v_leftColumn = v_leftColumn << (2 + shift);

    // Generate prediction signal
    Vec8s v_horPred4 = v_leftColumn + Vec8s(8);
    const Vec8s v_multi(1, 2, 3, 4, 5, 6, 7, 8);
    Vec8s v_horPred, v_rightColumnN;
    Vec8s v_im4;
    Vec16uc v_im5;

    COMP_PRED_PLANAR_ROW(0);     // row 0
    COMP_PRED_PLANAR_ROW(1);
    COMP_PRED_PLANAR_ROW(2);
    COMP_PRED_PLANAR_ROW(3);
    COMP_PRED_PLANAR_ROW(4);
    COMP_PRED_PLANAR_ROW(5);
    COMP_PRED_PLANAR_ROW(6);
    COMP_PRED_PLANAR_ROW(7);     // row 7
}

#undef COMP_PRED_PLANAR_ROW
#else /* if HIGH_BIT_DEPTH */

#define COMP_PRED_PLANAR_ROW(X) { \
        v_horPred = permute8s<X, X, X, X, X, X, X, X>(v_horPred4); \
        v_rightColumnN = permute8s<X, X, X, X, X, X, X, X>(v_rightColumn) * v_multi; \
        v_horPred = v_horPred + v_rightColumnN; \
        v_topRow = v_topRow + v_bottomRow; \
        v_im4 = (Vec8s)(v_horPred + v_topRow) >> (3 + shift); \
        v_im5 = compress(v_im4, v_im4); \
        store_partial(const_int(8), &dst[X * dstStride], v_im5); \
}

void predIntraPlanar8(pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride)
{
    int k;
    pixel bottomLeft, topRight;
    int16_t leftColumn[8];

    // Get left and above reference column and row
    Vec16uc im0 = (Vec16uc)load_partial(const_int(8), &src[-srcStride]); // topRow
    Vec8s v_topRow = extend_low(im0);

    for (k = 0; k < 8; k++)
    {
        leftColumn[k] = src[k * srcStride - 1];
    }

    Vec8s v_leftColumn;
    v_leftColumn.load(leftColumn);   // leftColumn

    // Prepare intermediate variables used in interpolation
    bottomLeft = src[8 * srcStride - 1];
    topRight   = src[8 - srcStride];

    Vec8s v_bottomLeft(bottomLeft);
    Vec8s v_topRight(topRight);

    Vec8s v_bottomRow = v_bottomLeft - v_topRow;
    Vec8s v_rightColumn = v_topRight - v_leftColumn;

    int shift = g_aucConvertToBit[8];         // Using value corresponding to width = 8
    v_topRow = v_topRow << (2 + shift);
    v_leftColumn = v_leftColumn << (2 + shift);

    Vec8s v_horPred4 = v_leftColumn + Vec8s(8);
    const Vec8s v_multi(1, 2, 3, 4, 5, 6, 7, 8);
    Vec8s v_horPred, v_rightColumnN;
    Vec8s v_im4;
    Vec16uc v_im5;

    COMP_PRED_PLANAR_ROW(0);     // row 0
    COMP_PRED_PLANAR_ROW(1);
    COMP_PRED_PLANAR_ROW(2);
    COMP_PRED_PLANAR_ROW(3);
    COMP_PRED_PLANAR_ROW(4);
    COMP_PRED_PLANAR_ROW(5);
    COMP_PRED_PLANAR_ROW(6);
    COMP_PRED_PLANAR_ROW(7);     // row 7
}

#undef COMP_PRED_PLANAR_ROW

#if INSTRSET >= 5
void predIntraPlanar8_sse4(pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride)
{
    pixel bottomLeft, topRight;

    // Get left and above reference column and row
    __m128i im0 = _mm_loadl_epi64((__m128i*)&src[0 - srcStride]); // topRow
    __m128i v_topRow = _mm_unpacklo_epi8(im0, _mm_setzero_si128());

    __m128i v_leftColumn = _mm_setzero_si128();

    v_leftColumn = _mm_insert_epi8(v_leftColumn, src[0 * srcStride - 1], 0);
    v_leftColumn = _mm_insert_epi8(v_leftColumn, src[1 * srcStride - 1], 1);
    v_leftColumn = _mm_insert_epi8(v_leftColumn, src[2 * srcStride - 1], 2);
    v_leftColumn = _mm_insert_epi8(v_leftColumn, src[3 * srcStride - 1], 3);
    v_leftColumn = _mm_insert_epi8(v_leftColumn, src[4 * srcStride - 1], 4);
    v_leftColumn = _mm_insert_epi8(v_leftColumn, src[5 * srcStride - 1], 5);
    v_leftColumn = _mm_insert_epi8(v_leftColumn, src[6 * srcStride - 1], 6);
    v_leftColumn = _mm_insert_epi8(v_leftColumn, src[7 * srcStride - 1], 7);
    v_leftColumn = _mm_unpacklo_epi8(v_leftColumn, _mm_setzero_si128());

    // Prepare intermediate variables used in interpolation
    bottomLeft = src[8 * srcStride - 1];
    topRight   = src[8 - srcStride];

    __m128i v_bottomLeft = _mm_set1_epi16(bottomLeft);
    __m128i v_topRight   = _mm_set1_epi16(topRight);

    __m128i v_bottomRow   = _mm_sub_epi16(v_bottomLeft, v_topRow);
    __m128i v_rightColumn = _mm_sub_epi16(v_topRight, v_leftColumn);

    v_topRow = _mm_slli_epi16(v_topRow, 3);
    v_leftColumn = _mm_slli_epi16(v_leftColumn, 3);

    __m128i v_horPred4 = _mm_add_epi16(v_leftColumn, _mm_set1_epi16(8));
    __m128i v_horPred, v_rightColumnN;
    __m128i v_im4;
    __m128i v_im5;

#define COMP_PRED_PLANAR_ROW(Y) { \
        if ((Y) < 4) { \
            v_horPred = _mm_shufflelo_epi16(v_horPred4, ((Y) & 3) * 0x55); \
            v_horPred = _mm_unpacklo_epi64(v_horPred, v_horPred); \
            v_rightColumnN = _mm_shufflelo_epi16(v_rightColumn, ((Y) & 3) * 0x55); \
            v_rightColumnN = _mm_unpacklo_epi64(v_rightColumnN, v_rightColumnN); \
        } \
        else { \
            v_horPred = _mm_shufflehi_epi16(v_horPred4, ((Y) & 3) * 0x55); \
            v_horPred = _mm_unpackhi_epi64(v_horPred, v_horPred); \
            v_rightColumnN = _mm_shufflehi_epi16(v_rightColumn, ((Y) & 3) * 0x55); \
            v_rightColumnN = _mm_unpackhi_epi64(v_rightColumnN, v_rightColumnN); \
        } \
        v_rightColumnN = _mm_mullo_epi16(v_rightColumnN, v_multiL); \
        v_horPred = _mm_add_epi16(v_horPred, v_rightColumnN); \
        v_topRow = _mm_add_epi16(v_topRow, v_bottomRow); \
        v_im4 = _mm_srai_epi16(_mm_add_epi16(v_horPred, v_topRow), 4); \
        v_im5 = _mm_packus_epi16(v_im4, v_im4); \
        _mm_storel_epi64((__m128i*)&dst[(Y)*dstStride], v_im5); \
}

    COMP_PRED_PLANAR_ROW(0)
    COMP_PRED_PLANAR_ROW(1)
    COMP_PRED_PLANAR_ROW(2)
    COMP_PRED_PLANAR_ROW(3)
    COMP_PRED_PLANAR_ROW(4)
    COMP_PRED_PLANAR_ROW(5)
    COMP_PRED_PLANAR_ROW(6)
    COMP_PRED_PLANAR_ROW(7)

#undef COMP_PRED_PLANAR_ROW
}

#endif // INSTRSET >= 5

#endif /* if HIGH_BIT_DEPTH */

#if HIGH_BIT_DEPTH
#define COMP_PRED_PLANAR_ROW(X) { \
        v_horPred_lo = permute8s<X, X, X, X, X, X, X, X>(v_horPred4); \
        v_horPred_hi = v_horPred_lo; \
        v_rightColumnN_lo = permute8s<X, X, X, X, X, X, X, X>(v_rightColumn); \
        v_rightColumnN_hi = v_rightColumnN_lo; \
        v_rightColumnN_lo *= v_multi_lo; \
        v_rightColumnN_hi *= v_multi_hi; \
        v_horPred_lo = v_horPred_lo + v_rightColumnN_lo; \
        v_horPred_hi = v_horPred_hi + v_rightColumnN_hi; \
        v_topRow_lo = v_topRow_lo + v_bottomRow_lo; \
        v_topRow_hi = v_topRow_hi + v_bottomRow_hi; \
        v_im4_lo = (Vec8s)(v_horPred_lo + v_topRow_lo) >> (3 + shift); \
        v_im4_hi = (Vec8s)(v_horPred_hi + v_topRow_hi) >> (3 + shift); \
        v_im4_lo.store(&dst[X * dstStride]); \
        v_im4_hi.store(&dst[X * dstStride + 8]); \
}

void predIntraPlanar16(pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride)
{
    int k;
    pixel bottomLeft, topRight;
    int16_t leftColumn[16];

    // Get left and above reference column and row
    Vec8s v_topRow_lo, v_topRow_hi;

    v_topRow_lo.load(&src[-srcStride]);
    v_topRow_hi.load(&src[-srcStride + 8]);

    for (k = 0; k < 16; k++)
    {
        leftColumn[k] = src[k * srcStride - 1];
    }

    Vec8s v_leftColumn;
    v_leftColumn.load(leftColumn);   // leftColumn

    // Prepare intermediate variables used in interpolation
    bottomLeft = src[16 * srcStride - 1];
    topRight   = src[16 - srcStride];

    Vec8s v_bottomLeft(bottomLeft);
    Vec8s v_topRight(topRight);

    Vec8s v_bottomRow_lo = v_bottomLeft - v_topRow_lo;
    Vec8s v_bottomRow_hi = v_bottomLeft - v_topRow_hi;
    Vec8s v_rightColumn = v_topRight - v_leftColumn;

    int shift = g_aucConvertToBit[16];         // Using value corresponding to width = 8
    v_topRow_lo = v_topRow_lo << (2 + shift);
    v_topRow_hi = v_topRow_hi << (2 + shift);
    v_leftColumn = v_leftColumn << (2 + shift);

    Vec8s v_horPred4 = v_leftColumn + Vec8s(16);
    const Vec8s v_multi_lo(1, 2, 3, 4, 5, 6, 7, 8);
    const Vec8s v_multi_hi(9, 10, 11, 12, 13, 14, 15, 16);
    Vec8s v_horPred_lo, v_horPred_hi, v_rightColumnN_lo, v_rightColumnN_hi;
    Vec8s v_im4_lo, v_im4_hi;
    Vec16uc v_im5;

    COMP_PRED_PLANAR_ROW(0);     // row 0
    COMP_PRED_PLANAR_ROW(1);
    COMP_PRED_PLANAR_ROW(2);
    COMP_PRED_PLANAR_ROW(3);
    COMP_PRED_PLANAR_ROW(4);
    COMP_PRED_PLANAR_ROW(5);
    COMP_PRED_PLANAR_ROW(6);
    COMP_PRED_PLANAR_ROW(7);     // row 7

    v_leftColumn.load(leftColumn + 8);   // leftColumn lower 8 rows
    v_rightColumn = v_topRight - v_leftColumn;
    v_leftColumn = v_leftColumn << (2 + shift);
    v_horPred4 = v_leftColumn + Vec8s(16);

    COMP_PRED_PLANAR_ROW(8);     // row 0
    COMP_PRED_PLANAR_ROW(9);
    COMP_PRED_PLANAR_ROW(10);
    COMP_PRED_PLANAR_ROW(11);
    COMP_PRED_PLANAR_ROW(12);
    COMP_PRED_PLANAR_ROW(13);
    COMP_PRED_PLANAR_ROW(14);
    COMP_PRED_PLANAR_ROW(15);
}

#undef COMP_PRED_PLANAR_ROW

#else /* if HIGH_BIT_DEPTH */
#define COMP_PRED_PLANAR_ROW(X) { \
        v_horPred_lo = permute8s<X, X, X, X, X, X, X, X>(v_horPred4); \
        v_horPred_hi = v_horPred_lo; \
        v_rightColumnN_lo = permute8s<X, X, X, X, X, X, X, X>(v_rightColumn); \
        v_rightColumnN_hi = v_rightColumnN_lo; \
        v_rightColumnN_lo *= v_multi_lo; \
        v_rightColumnN_hi *= v_multi_hi; \
        v_horPred_lo = v_horPred_lo + v_rightColumnN_lo; \
        v_horPred_hi = v_horPred_hi + v_rightColumnN_hi; \
        v_topRow_lo = v_topRow_lo + v_bottomRow_lo; \
        v_topRow_hi = v_topRow_hi + v_bottomRow_hi; \
        v_im4_lo = (Vec8s)(v_horPred_lo + v_topRow_lo) >> (3 + shift); \
        v_im4_hi = (Vec8s)(v_horPred_hi + v_topRow_hi) >> (3 + shift); \
        v_im5 = compress(v_im4_lo, v_im4_hi); \
        store_partial(const_int(16), &dst[X * dstStride], v_im5); \
}

void predIntraPlanar16(pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride)
{
    int k;
    pixel bottomLeft, topRight;
    int16_t leftColumn[16];

    // Get left and above reference column and row
    Vec16uc im0 = (Vec16uc)load_partial(const_int(16), &src[-srcStride]); // topRow
    Vec8s v_topRow_lo = extend_low(im0);
    Vec8s v_topRow_hi = extend_high(im0);

    for (k = 0; k < 16; k++)
    {
        leftColumn[k] = src[k * srcStride - 1];
    }

    Vec8s v_leftColumn;
    v_leftColumn.load(leftColumn);   // leftColumn

    // Prepare intermediate variables used in interpolation
    bottomLeft = src[16 * srcStride - 1];
    topRight   = src[16 - srcStride];

    Vec8s v_bottomLeft(bottomLeft);
    Vec8s v_topRight(topRight);

    Vec8s v_bottomRow_lo = v_bottomLeft - v_topRow_lo;
    Vec8s v_bottomRow_hi = v_bottomLeft - v_topRow_hi;
    Vec8s v_rightColumn = v_topRight - v_leftColumn;

    int shift = g_aucConvertToBit[16];         // Using value corresponding to width = 8
    v_topRow_lo = v_topRow_lo << (2 + shift);
    v_topRow_hi = v_topRow_hi << (2 + shift);
    v_leftColumn = v_leftColumn << (2 + shift);

    Vec8s v_horPred4 = v_leftColumn + Vec8s(16);
    const Vec8s v_multi_lo(1, 2, 3, 4, 5, 6, 7, 8);
    const Vec8s v_multi_hi(9, 10, 11, 12, 13, 14, 15, 16);
    Vec8s v_horPred_lo, v_horPred_hi, v_rightColumnN_lo, v_rightColumnN_hi;
    Vec8s v_im4_lo, v_im4_hi;
    Vec16uc v_im5;

    COMP_PRED_PLANAR_ROW(0);     // row 0
    COMP_PRED_PLANAR_ROW(1);
    COMP_PRED_PLANAR_ROW(2);
    COMP_PRED_PLANAR_ROW(3);
    COMP_PRED_PLANAR_ROW(4);
    COMP_PRED_PLANAR_ROW(5);
    COMP_PRED_PLANAR_ROW(6);
    COMP_PRED_PLANAR_ROW(7);     // row 7

    v_leftColumn.load(leftColumn + 8);   // leftColumn lower 8 rows
    v_rightColumn = v_topRight - v_leftColumn;
    v_leftColumn = v_leftColumn << (2 + shift);
    v_horPred4 = v_leftColumn + Vec8s(16);

    COMP_PRED_PLANAR_ROW(8);     // row 0
    COMP_PRED_PLANAR_ROW(9);
    COMP_PRED_PLANAR_ROW(10);
    COMP_PRED_PLANAR_ROW(11);
    COMP_PRED_PLANAR_ROW(12);
    COMP_PRED_PLANAR_ROW(13);
    COMP_PRED_PLANAR_ROW(14);
    COMP_PRED_PLANAR_ROW(15);
}

#undef COMP_PRED_PLANAR_ROW

#if INSTRSET >= 5
void predIntraPlanar16_sse4(pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride)
{
    pixel bottomLeft, topRight;
    __m128i v_topRow[2];
    __m128i v_bottomRow[2];

    // Get left and above reference column and row
    __m128i im0 = _mm_loadu_si128((__m128i*)&src[0 - srcStride]); // topRow

    v_topRow[0] = _mm_unpacklo_epi8(im0, _mm_setzero_si128());
    v_topRow[1] = _mm_unpackhi_epi8(im0, _mm_setzero_si128());

    // Prepare intermediate variables used in interpolation
    bottomLeft = src[16 * srcStride - 1];
    topRight   = src[16 - srcStride];

    __m128i v_bottomLeft = _mm_set1_epi16(bottomLeft);

    v_bottomRow[0] = _mm_sub_epi16(v_bottomLeft, v_topRow[0]);
    v_bottomRow[1] = _mm_sub_epi16(v_bottomLeft, v_topRow[1]);

    v_topRow[0] = _mm_slli_epi16(v_topRow[0], 4);
    v_topRow[1] = _mm_slli_epi16(v_topRow[1], 4);

    __m128i v_horPred, v_horPredN[2], v_rightColumnN[2];
    __m128i v_im4L, v_im4H;
    __m128i v_im5;

#define COMP_PRED_PLANAR_ROW(Y) { \
        v_horPred = _mm_cvtsi32_si128((src[(Y)*srcStride - 1] << 4) + 16); \
        v_horPred = _mm_shufflelo_epi16(v_horPred, 0); \
        v_horPred = _mm_shuffle_epi32(v_horPred, 0); \
        __m128i _tmp = _mm_cvtsi32_si128(topRight - src[(Y)*srcStride - 1]); \
        _tmp = _mm_shufflelo_epi16(_tmp, 0); \
        _tmp = _mm_shuffle_epi32(_tmp, 0); \
        v_rightColumnN[0] = _mm_mullo_epi16(_tmp, v_multiL); \
        v_rightColumnN[1] = _mm_mullo_epi16(_tmp, v_multiH); \
        v_horPredN[0] = _mm_add_epi16(v_horPred, v_rightColumnN[0]); \
        v_horPredN[1] = _mm_add_epi16(v_horPred, v_rightColumnN[1]); \
        v_topRow[0] = _mm_add_epi16(v_topRow[0], v_bottomRow[0]); \
        v_topRow[1] = _mm_add_epi16(v_topRow[1], v_bottomRow[1]); \
        v_im4L = _mm_srai_epi16(_mm_add_epi16(v_horPredN[0], v_topRow[0]), 5); \
        v_im4H = _mm_srai_epi16(_mm_add_epi16(v_horPredN[1], v_topRow[1]), 5); \
        v_im5 = _mm_packus_epi16(v_im4L, v_im4H); \
        _mm_storeu_si128((__m128i*)&dst[(Y)*dstStride], v_im5); \
}

    COMP_PRED_PLANAR_ROW(0)
    COMP_PRED_PLANAR_ROW(1)
    COMP_PRED_PLANAR_ROW(2)
    COMP_PRED_PLANAR_ROW(3)
    COMP_PRED_PLANAR_ROW(4)
    COMP_PRED_PLANAR_ROW(5)
    COMP_PRED_PLANAR_ROW(6)
    COMP_PRED_PLANAR_ROW(7)
    COMP_PRED_PLANAR_ROW(8)
    COMP_PRED_PLANAR_ROW(9)
    COMP_PRED_PLANAR_ROW(10)
    COMP_PRED_PLANAR_ROW(11)
    COMP_PRED_PLANAR_ROW(12)
    COMP_PRED_PLANAR_ROW(13)
    COMP_PRED_PLANAR_ROW(14)
    COMP_PRED_PLANAR_ROW(15)

#undef COMP_PRED_PLANAR_ROW
}

#endif // INSTRSET >= 5

#if INSTRSET >= 5
void predIntraPlanar32_sse4(pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride)
{
    pixel bottomLeft, topRight;
    __m128i v_topRow[4];
    __m128i v_bottomRow[4];

    // Get left and above reference column and row
    __m128i im0 = _mm_loadu_si128((__m128i*)&src[0 - srcStride]); // topRow
    __m128i im1 = _mm_loadu_si128((__m128i*)&src[16 - srcStride]); // topRow

    v_topRow[0] = _mm_unpacklo_epi8(im0, _mm_setzero_si128());
    v_topRow[1] = _mm_unpackhi_epi8(im0, _mm_setzero_si128());
    v_topRow[2] = _mm_unpacklo_epi8(im1, _mm_setzero_si128());
    v_topRow[3] = _mm_unpackhi_epi8(im1, _mm_setzero_si128());

    // Prepare intermediate variables used in interpolation
    bottomLeft = src[32 * srcStride - 1];
    topRight   = src[32 - srcStride];

    __m128i v_bottomLeft = _mm_set1_epi16(bottomLeft);

    v_bottomRow[0] = _mm_sub_epi16(v_bottomLeft, v_topRow[0]);
    v_bottomRow[1] = _mm_sub_epi16(v_bottomLeft, v_topRow[1]);
    v_bottomRow[2] = _mm_sub_epi16(v_bottomLeft, v_topRow[2]);
    v_bottomRow[3] = _mm_sub_epi16(v_bottomLeft, v_topRow[3]);

    v_topRow[0] = _mm_slli_epi16(v_topRow[0], 5);
    v_topRow[1] = _mm_slli_epi16(v_topRow[1], 5);
    v_topRow[2] = _mm_slli_epi16(v_topRow[2], 5);
    v_topRow[3] = _mm_slli_epi16(v_topRow[3], 5);

    __m128i v_horPred, v_horPredN[4], v_rightColumnN[4];
    __m128i v_im4[4];
    __m128i v_im5[2];

#define COMP_PRED_PLANAR_ROW(Y) { \
        v_horPred = _mm_cvtsi32_si128((src[(Y)*srcStride - 1] << 5) + 32); \
        v_horPred = _mm_shufflelo_epi16(v_horPred, 0); \
        v_horPred = _mm_shuffle_epi32(v_horPred, 0); \
        __m128i _tmp = _mm_cvtsi32_si128(topRight - src[(Y)*srcStride - 1]); \
        _tmp = _mm_shufflelo_epi16(_tmp, 0); \
        _tmp = _mm_shuffle_epi32(_tmp, 0); \
        v_rightColumnN[0] = _mm_mullo_epi16(_tmp, v_multiL); \
        v_rightColumnN[1] = _mm_mullo_epi16(_tmp, v_multiH); \
        v_rightColumnN[2] = _mm_mullo_epi16(_tmp, v_multiH2); \
        v_rightColumnN[3] = _mm_mullo_epi16(_tmp, v_multiH3); \
        v_horPredN[0] = _mm_add_epi16(v_horPred, v_rightColumnN[0]); \
        v_horPredN[1] = _mm_add_epi16(v_horPred, v_rightColumnN[1]); \
        v_horPredN[2] = _mm_add_epi16(v_horPred, v_rightColumnN[2]); \
        v_horPredN[3] = _mm_add_epi16(v_horPred, v_rightColumnN[3]); \
        v_topRow[0] = _mm_add_epi16(v_topRow[0], v_bottomRow[0]); \
        v_topRow[1] = _mm_add_epi16(v_topRow[1], v_bottomRow[1]); \
        v_topRow[2] = _mm_add_epi16(v_topRow[2], v_bottomRow[2]); \
        v_topRow[3] = _mm_add_epi16(v_topRow[3], v_bottomRow[3]); \
        v_im4[0] = _mm_srai_epi16(_mm_add_epi16(v_horPredN[0], v_topRow[0]), 6); \
        v_im4[1] = _mm_srai_epi16(_mm_add_epi16(v_horPredN[1], v_topRow[1]), 6); \
        v_im4[2] = _mm_srai_epi16(_mm_add_epi16(v_horPredN[2], v_topRow[2]), 6); \
        v_im4[3] = _mm_srai_epi16(_mm_add_epi16(v_horPredN[3], v_topRow[3]), 6); \
        v_im5[0] = _mm_packus_epi16(v_im4[0], v_im4[1]); \
        v_im5[1] = _mm_packus_epi16(v_im4[2], v_im4[3]); \
        _mm_storeu_si128((__m128i*)&dst[(Y)*dstStride], v_im5[0]); \
        _mm_storeu_si128((__m128i*)&dst[(Y)*dstStride + 16], v_im5[1]); \
}

    int i;
    for (i = 0; i < 32; i += 2)
    {
        COMP_PRED_PLANAR_ROW(i + 0);
        COMP_PRED_PLANAR_ROW(i + 1);
    }

#undef COMP_PRED_PLANAR_ROW
}

#endif // INSTRSET >= 5

#if INSTRSET >= 5
void predIntraPlanar64_sse4(pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride)
{
    pixel bottomLeft, topRight;
    __m128i v_topRow[8];
    __m128i v_bottomRow[8];

    // Get left and above reference column and row
    __m128i im0 = _mm_loadu_si128((__m128i*)&src[0 - srcStride]); // topRow
    __m128i im1 = _mm_loadu_si128((__m128i*)&src[16 - srcStride]); // topRow
    __m128i im2 = _mm_loadu_si128((__m128i*)&src[32 - srcStride]); // topRow
    __m128i im3 = _mm_loadu_si128((__m128i*)&src[48 - srcStride]); // topRow

    v_topRow[0] = _mm_unpacklo_epi8(im0, _mm_setzero_si128());
    v_topRow[1] = _mm_unpackhi_epi8(im0, _mm_setzero_si128());
    v_topRow[2] = _mm_unpacklo_epi8(im1, _mm_setzero_si128());
    v_topRow[3] = _mm_unpackhi_epi8(im1, _mm_setzero_si128());
    v_topRow[4] = _mm_unpacklo_epi8(im2, _mm_setzero_si128());
    v_topRow[5] = _mm_unpackhi_epi8(im2, _mm_setzero_si128());
    v_topRow[6] = _mm_unpacklo_epi8(im3, _mm_setzero_si128());
    v_topRow[7] = _mm_unpackhi_epi8(im3, _mm_setzero_si128());

    // Prepare intermediate variables used in interpolation
    bottomLeft = src[64 * srcStride - 1];
    topRight   = src[64 - srcStride];

    __m128i v_bottomLeft = _mm_set1_epi16(bottomLeft);

    v_bottomRow[0] = _mm_sub_epi16(v_bottomLeft, v_topRow[0]);
    v_bottomRow[1] = _mm_sub_epi16(v_bottomLeft, v_topRow[1]);
    v_bottomRow[2] = _mm_sub_epi16(v_bottomLeft, v_topRow[2]);
    v_bottomRow[3] = _mm_sub_epi16(v_bottomLeft, v_topRow[3]);
    v_bottomRow[4] = _mm_sub_epi16(v_bottomLeft, v_topRow[4]);
    v_bottomRow[5] = _mm_sub_epi16(v_bottomLeft, v_topRow[5]);
    v_bottomRow[6] = _mm_sub_epi16(v_bottomLeft, v_topRow[6]);
    v_bottomRow[7] = _mm_sub_epi16(v_bottomLeft, v_topRow[7]);

    v_topRow[0] = _mm_slli_epi16(v_topRow[0], 6);
    v_topRow[1] = _mm_slli_epi16(v_topRow[1], 6);
    v_topRow[2] = _mm_slli_epi16(v_topRow[2], 6);
    v_topRow[3] = _mm_slli_epi16(v_topRow[3], 6);
    v_topRow[4] = _mm_slli_epi16(v_topRow[4], 6);
    v_topRow[5] = _mm_slli_epi16(v_topRow[5], 6);
    v_topRow[6] = _mm_slli_epi16(v_topRow[6], 6);
    v_topRow[7] = _mm_slli_epi16(v_topRow[7], 6);

    __m128i v_horPred, v_horPredN[8], v_rightColumnN[8];
    __m128i v_im4[8];
    __m128i v_im5[4];

#define COMP_PRED_PLANAR_ROW(Y) { \
        v_horPred = _mm_cvtsi32_si128((src[(Y)*srcStride - 1] << 6) + 64); \
        v_horPred = _mm_shufflelo_epi16(v_horPred, 0); \
        v_horPred = _mm_shuffle_epi32(v_horPred, 0); \
        __m128i _tmp = _mm_cvtsi32_si128(topRight - src[(Y)*srcStride - 1]); \
        _tmp = _mm_shufflelo_epi16(_tmp, 0); \
        _tmp = _mm_shuffle_epi32(_tmp, 0); \
        v_rightColumnN[0] = _mm_mullo_epi16(_tmp, v_multiL); \
        v_rightColumnN[1] = _mm_mullo_epi16(_tmp, v_multiH); \
        v_rightColumnN[2] = _mm_mullo_epi16(_tmp, v_multiH2); \
        v_rightColumnN[3] = _mm_mullo_epi16(_tmp, v_multiH3); \
        v_rightColumnN[4] = _mm_mullo_epi16(_tmp, v_multiH4); \
        v_rightColumnN[5] = _mm_mullo_epi16(_tmp, v_multiH5); \
        v_rightColumnN[6] = _mm_mullo_epi16(_tmp, v_multiH6); \
        v_rightColumnN[7] = _mm_mullo_epi16(_tmp, v_multiH7); \
        v_horPredN[0] = _mm_add_epi16(v_horPred, v_rightColumnN[0]); \
        v_horPredN[1] = _mm_add_epi16(v_horPred, v_rightColumnN[1]); \
        v_horPredN[2] = _mm_add_epi16(v_horPred, v_rightColumnN[2]); \
        v_horPredN[3] = _mm_add_epi16(v_horPred, v_rightColumnN[3]); \
        v_horPredN[4] = _mm_add_epi16(v_horPred, v_rightColumnN[4]); \
        v_horPredN[5] = _mm_add_epi16(v_horPred, v_rightColumnN[5]); \
        v_horPredN[6] = _mm_add_epi16(v_horPred, v_rightColumnN[6]); \
        v_horPredN[7] = _mm_add_epi16(v_horPred, v_rightColumnN[7]); \
        v_topRow[0] = _mm_add_epi16(v_topRow[0], v_bottomRow[0]); \
        v_topRow[1] = _mm_add_epi16(v_topRow[1], v_bottomRow[1]); \
        v_topRow[2] = _mm_add_epi16(v_topRow[2], v_bottomRow[2]); \
        v_topRow[3] = _mm_add_epi16(v_topRow[3], v_bottomRow[3]); \
        v_topRow[4] = _mm_add_epi16(v_topRow[4], v_bottomRow[4]); \
        v_topRow[5] = _mm_add_epi16(v_topRow[5], v_bottomRow[5]); \
        v_topRow[6] = _mm_add_epi16(v_topRow[6], v_bottomRow[6]); \
        v_topRow[7] = _mm_add_epi16(v_topRow[7], v_bottomRow[7]); \
        v_im4[0] = _mm_srai_epi16(_mm_add_epi16(v_horPredN[0], v_topRow[0]), 7); \
        v_im4[1] = _mm_srai_epi16(_mm_add_epi16(v_horPredN[1], v_topRow[1]), 7); \
        v_im4[2] = _mm_srai_epi16(_mm_add_epi16(v_horPredN[2], v_topRow[2]), 7); \
        v_im4[3] = _mm_srai_epi16(_mm_add_epi16(v_horPredN[3], v_topRow[3]), 7); \
        v_im4[4] = _mm_srai_epi16(_mm_add_epi16(v_horPredN[4], v_topRow[4]), 7); \
        v_im4[5] = _mm_srai_epi16(_mm_add_epi16(v_horPredN[5], v_topRow[5]), 7); \
        v_im4[6] = _mm_srai_epi16(_mm_add_epi16(v_horPredN[6], v_topRow[6]), 7); \
        v_im4[7] = _mm_srai_epi16(_mm_add_epi16(v_horPredN[7], v_topRow[7]), 7); \
        v_im5[0] = _mm_packus_epi16(v_im4[0], v_im4[1]); \
        v_im5[1] = _mm_packus_epi16(v_im4[2], v_im4[3]); \
        v_im5[2] = _mm_packus_epi16(v_im4[4], v_im4[5]); \
        v_im5[3] = _mm_packus_epi16(v_im4[6], v_im4[7]); \
        _mm_storeu_si128((__m128i*)&dst[(Y)*dstStride], v_im5[0]); \
        _mm_storeu_si128((__m128i*)&dst[(Y)*dstStride + 16], v_im5[1]); \
        _mm_storeu_si128((__m128i*)&dst[(Y)*dstStride + 32], v_im5[2]); \
        _mm_storeu_si128((__m128i*)&dst[(Y)*dstStride + 48], v_im5[3]); \
}

    int i;
    for (i = 0; i < 64; i++)
    {
        COMP_PRED_PLANAR_ROW(i + 0);
        //COMP_PRED_PLANAR_ROW(i+1);
    }

#undef COMP_PRED_PLANAR_ROW
}

#endif // INSTRSET >= 5

#endif /* if HIGH_BIT_DEPTH */

typedef void predIntraPlanar_t (pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride);
predIntraPlanar_t *intraPlanarN[] =
{
#if !HIGH_BIT_DEPTH && INSTRSET >= 5
    predIntraPlanar4_sse4,
    predIntraPlanar8_sse4,
    predIntraPlanar16_sse4,
    predIntraPlanar32_sse4,
    predIntraPlanar64_sse4,
#else
    predIntraPlanar4,
    predIntraPlanar8,
    predIntraPlanar16,
#endif
};

void predIntraPlanar(pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int width)
{
    //assert(width == height);

    int nLog2Size = g_aucConvertToBit[width] + 2;

#if (!HIGH_BIT_DEPTH) && (INSTRSET >= 5)
    intraPlanarN[nLog2Size - 2](src, srcStride, dst, dstStride);
    return;
#else
    int k, l, bottomLeft, topRight;
    int horPred;
    // OPT_ME: when width is 64, the shift1D is 8, then the dynamic range is [-65280, 65280], so we have to use 32 bits here
    int32_t leftColumn[MAX_CU_SIZE + 1], topRow[MAX_CU_SIZE + 1];
    // CHECK_ME: dynamic range is 9 bits or 15 bits(I assume max input bit_depth is 14 bits)
    int16_t bottomRow[MAX_CU_SIZE], rightColumn[MAX_CU_SIZE];
    int blkSize = width;
    int offset2D = width;
    int shift1D = nLog2Size;
    int shift2D = shift1D + 1;

    if (width < 32)
    {
        intraPlanarN[nLog2Size - 2](src, srcStride, dst, dstStride);
        return;
    }

    // Get left and above reference column and row
    for (k = 0; k < blkSize + 1; k++)
    {
        topRow[k] = src[k - srcStride];
        leftColumn[k] = src[k * srcStride - 1];
    }

    // Prepare intermediate variables used in interpolation
    bottomLeft = leftColumn[blkSize];
    topRight   = topRow[blkSize];
    for (k = 0; k < blkSize; k++)
    {
        bottomRow[k]   = bottomLeft - topRow[k];
        rightColumn[k] = topRight   - leftColumn[k];
        topRow[k]      <<= shift1D;
        leftColumn[k]  <<= shift1D;
    }

    // Generate prediction signal
    for (k = 0; k < blkSize; k++)
    {
        horPred = leftColumn[k] + offset2D;
        for (l = 0; l < blkSize; l++)
        {
            horPred += rightColumn[k];
            topRow[l] += bottomRow[l];
            dst[k * dstStride + l] = ((horPred + topRow[l]) >> shift2D);
        }
    }

#endif /* if (!HIGH_BIT_DEPTH) && (INSTRSET >= 5) */
}

#if HIGH_BIT_DEPTH
void xPredIntraAng4x4(int bitDepth, pixel* pDst, int dstStride, int width, int dirMode, pixel *refLeft, pixel *refAbove)
{
    int blkSize        = width;

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
        for (int k = -1; k > blkSize * intraPredAngle >> 5; k--)
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

    // bfilter will always be true for blocksize 4
    if (intraPredAngle == 0)  // Exactly hotizontal/vertical angles
    {
        if (modeHor)
        {
            Vec8s v_temp;
            Vec8s v_side_0; // refSide[0] value in a vector
            v_temp.load((void*)refSide);
            v_side_0 = broadcast(const_int(0), (Vec8s)v_temp);

            Vec8s v_side;
            v_side.load(refSide + 1);

            Vec8s v_main;
            v_main = load_partial(const_int(8), (void*)(refMain + 1));

            Vec8s tmp1, tmp2;
            tmp1 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>(v_main, v_main);
            tmp2 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>(tmp1, tmp1);
            tmp1 = blend8s<4, 12, 5, 13, 6, 14, 7, 15>(tmp1, tmp1);

            Vec8s row0;
            v_side -= v_side_0;
            v_side = v_side >> 1;
            row0 = tmp2 + v_side;
            row0 = min(max(0, row0), (1 << bitDepth) - 1);

            store_partial(const_int(8), pDst, row0);                //row0
            store_partial(const_int(8), pDst + (2 * dstStride), tmp1); //row2

            tmp2 = blend8s<4, 12, 5, 13, 6, 14, 7, 15>(tmp2, tmp2);
            tmp1 = blend8s<4, 12, 5, 13, 6, 14, 7, 15>(tmp1, tmp1);

            store_partial(const_int(8), pDst + (3 * dstStride), tmp1); //row3
            store_partial(const_int(8), pDst + (dstStride), tmp2);    //row1
        }
        else
        {
            Vec16uc v_main;
            v_main = load_partial(const_int(8), refMain + 1);
            store_partial(const_int(8), pDst, v_main);
            store_partial(const_int(8), pDst + dstStride, v_main);
            store_partial(const_int(8), pDst + (2 * dstStride), v_main);
            store_partial(const_int(8), pDst + (3 * dstStride), v_main);

            for (int k = 0; k < 4; k++)
            {
                pDst[k * dstStride] = (pixel)Clip3((short)0, (short)((1 << bitDepth) - 1), static_cast<short>((pDst[k * dstStride]) + ((refSide[k + 1] - refSide[0]) >> 1)));
            }
        }
    }
    else if (intraPredAngle == -32)
    {
        Vec8s tmp;
        tmp = load_partial(const_int(8), refMain);        //-1,0,1,2
        store_partial(const_int(8), pDst, tmp);
        tmp = load_partial(const_int(8), refMain - 1);     //-2,-1,0,1
        store_partial(const_int(8), pDst + dstStride, tmp);
        tmp = load_partial(const_int(8), refMain - 2);
        store_partial(const_int(8), pDst + 2 * dstStride, tmp);
        tmp = load_partial(const_int(8), refMain - 3);
        store_partial(const_int(8), pDst + 3 * dstStride, tmp);
        return;
    }
    else if (intraPredAngle == 32)
    {
        Vec8s tmp;
        tmp = load_partial(const_int(8), refMain + 2);        //-1,0,1,2
        store_partial(const_int(8), pDst, tmp);
        tmp = load_partial(const_int(8), refMain + 3);     //-2,-1,0,1
        store_partial(const_int(8), pDst + dstStride, tmp);
        tmp = load_partial(const_int(8), refMain + 4);
        store_partial(const_int(8), pDst + 2 * dstStride, tmp);
        tmp = load_partial(const_int(8), refMain + 5);
        store_partial(const_int(8), pDst + 3 * dstStride, tmp);
        return;
    }
    else
    {
        Vec8s row11, row12, row21, row22, row31, row32, row41, row42;
        Vec8s v_deltaFract, v_deltaPos, thirty2(32), thirty1(31), v_ipAngle;

        row11 = (Vec8s)load_partial(const_int(8), refMain + 1 + GETAP(lookIdx, 0));
        row12 = (Vec8s)load_partial(const_int(8), refMain + 1 + GETAP(lookIdx, 0) + 1);

        row21 = (Vec8s)load_partial(const_int(8), refMain + 1 + GETAP(lookIdx, 1));
        row22 = (Vec8s)load_partial(const_int(8), refMain + 1 + GETAP(lookIdx, 1) + 1);

        row31 = (Vec8s)load_partial(const_int(8), refMain + 1 + GETAP(lookIdx, 2));
        row32 = (Vec8s)load_partial(const_int(8), refMain + 1 + GETAP(lookIdx, 2) + 1);

        row41 = (Vec8s)load_partial(const_int(8), refMain + 1 + GETAP(lookIdx, 3));
        row42 = (Vec8s)load_partial(const_int(8), refMain + 1 + GETAP(lookIdx, 3) + 1);

        v_deltaPos = v_ipAngle = intraPredAngle;

        //row1
        v_deltaFract = v_deltaPos & thirty1;
        row11 = ((thirty2 - v_deltaFract) * row11 + (v_deltaFract * row12) + 16) >> 5;

        //row2
        v_deltaPos += v_ipAngle;
        v_deltaFract = v_deltaPos & thirty1;
        row21 = ((thirty2 - v_deltaFract) * row21 + (v_deltaFract * row22) + 16) >> 5;

        //row3
        v_deltaPos += v_ipAngle;
        v_deltaFract = v_deltaPos & thirty1;
        row31 = ((thirty2 - v_deltaFract) * row31 + (v_deltaFract * row32) + 16) >> 5;

        //row4
        v_deltaPos += v_ipAngle;
        v_deltaFract = v_deltaPos & thirty1;
        row41 = ((thirty2 - v_deltaFract) * row41 + (v_deltaFract * row42) + 16) >> 5;

        // Flip the block

        if (modeHor)
        {
            Vec8s tmp1, tmp2, tmp3, tmp4;

            tmp1 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>(row11, row31);
            tmp2 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>(row21, row41);

            tmp3 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>(tmp1, tmp2);
            tmp4 = blend8s<4, 12, 5, 13, 6, 14, 7, 15>(tmp1, tmp2);

            //tmp16_1 = compress(tmp3, tmp3);
            store_partial(const_int(8), pDst, tmp3);

            store_partial(const_int(8), pDst + (2 * dstStride), tmp4);  //row2

            tmp3 = blend2q<1, 3>((Vec2q)tmp3, (Vec2q)tmp3);
            tmp4 = blend2q<1, 3>((Vec2q)tmp4, (Vec2q)tmp4);

            store_partial(const_int(8), pDst + (3 * dstStride), tmp4);   //row3
            store_partial(const_int(8), pDst + (dstStride), tmp3);       //row1
        }
        else
        {
            store_partial(const_int(8), pDst, row11);
            store_partial(const_int(8), pDst + (dstStride), row21);
            store_partial(const_int(8), pDst + (2 * dstStride), row31);
            store_partial(const_int(8), pDst + (3 * dstStride), row41);
        }
    }
}

#else /* if HIGH_BIT_DEPTH */

void PredIntraAng4_32(pixel* pDst, int dstStride, pixel *refMain, int /*dirMode*/)
{
    Vec16uc tmp16_1, tmp16_2;
    tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain + 2);
    store_partial(const_int(4), pDst, tmp16_1);
    tmp16_2 = (Vec16uc)load_partial(const_int(8), refMain + 3);
    store_partial(const_int(4), pDst + dstStride, tmp16_2);
    tmp16_2 = (Vec16uc)load_partial(const_int(8), refMain + 4);
    store_partial(const_int(4), pDst + 2 * dstStride, tmp16_2);
    tmp16_2 = (Vec16uc)load_partial(const_int(8), refMain + 5);
    store_partial(const_int(4), pDst + 3 * dstStride, tmp16_2);
}

void PredIntraAng4_26(pixel* pDst, int dstStride, pixel *refMain, int dirMode)
{
    PRED_INTRA_ANG4_START

    tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain + 1); 
    row11 = extend_low(tmp16_1);    //offsets(0,1,2,3)

    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 8;
    tmp16_2 = reinterpret_i(tmp2uq);
    row12 = extend_low(tmp16_2);    //offsets(1,2,3,4)

    row21 = row12;

    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 16;
    tmp16_2 = reinterpret_i(tmp2uq);
    row22 = extend_low(tmp16_2);    //offsets(2,3,4,5)

    row31 = row22;
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 24;
    tmp16_2 = reinterpret_i(tmp2uq);
    row32 = extend_low(tmp16_2);    //offsets(3,4,5,6)

    row41 = row32;
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 32;
    tmp16_2 = reinterpret_i(tmp2uq);
    row42 = extend_low(tmp16_2);    //offsets(4,5,6,7)

    v_deltaPos = v_ipAngle = 26;
    
    PRED_INTRA_ANG4_END
}

void PredIntraAng4_21(pixel* pDst, int dstStride, pixel *refMain, int dirMode) 
{
    PRED_INTRA_ANG4_START

    tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain + 1);

    row11 = extend_low(tmp16_1);    //offsets(0,1,2,3)

    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 8;
    tmp16_2 = reinterpret_i(tmp2uq);
    row12 = extend_low(tmp16_2);    //offsets(1,2,3,4)

    row21 = row12;

    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 16;
    tmp16_2 = reinterpret_i(tmp2uq);
    row22 = extend_low(tmp16_2);    //offsets(2,3,4,5)

    row31 = row21;
    row32 = row22;

    row41 = row22;
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 24;
    tmp16_2 = reinterpret_i(tmp2uq);
    row42 = extend_low(tmp16_2);    //offsets(3,4,5,6)

    v_deltaPos = v_ipAngle = 21;

    PRED_INTRA_ANG4_END
}

void PredIntraAng4_17(pixel* pDst, int dstStride, pixel *refMain, int dirMode) 
{
    PRED_INTRA_ANG4_START

    tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain + 1);

    row11 = extend_low(tmp16_1);    //offsets(0,1,2,3)

    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 8;
    tmp16_2 = reinterpret_i(tmp2uq);
    row12 = extend_low(tmp16_2);    //offsets(1,2,3,4)

    row21 = row12;

    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 16;
    tmp16_2 = reinterpret_i(tmp2uq);
    row22 = extend_low(tmp16_2);    //offsets(2,3,4,5)

    row31 = row21;
    row32 = row22;

    row41 = row22;
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 24;
    tmp16_2 = reinterpret_i(tmp2uq);
    row42 = extend_low(tmp16_2);    //offsets(3,4,5,6)

    v_deltaPos = v_ipAngle = 17;

    PRED_INTRA_ANG4_END
}

void PredIntraAng4_13(pixel* pDst, int dstStride, pixel *refMain, int dirMode) 
{
    PRED_INTRA_ANG4_START

    tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain + 1);
    row11 = extend_low(tmp16_1);    //offsets(0,1,2,3)

    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 8;
    tmp16_2 = reinterpret_i(tmp2uq);
    row12 = extend_low(tmp16_2);    //offsets(1,2,3,4)

    row21 = row11;                  //offsets(0,1,2,3)
    row22 = row12;
    row31 = row12;                  //offsets(1,2,3,4)

    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 16;
    tmp16_2 = reinterpret_i(tmp2uq);
    row32 = extend_low(tmp16_2);    //offsets(2,3,4,5)

    row41 = row31;                  //offsets(1,2,3,4)
    row42 = row32;

    v_deltaPos = v_ipAngle = 13;

    PRED_INTRA_ANG4_END
}

void PredIntraAng4_9(pixel* pDst, int dstStride, pixel *refMain, int dirMode) 
{
    PRED_INTRA_ANG4_START

    tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain + 1);
    row11 = extend_low(tmp16_1);    //offsets(0,1,2,3)
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 8;
    tmp16_2 = reinterpret_i(tmp2uq);
    row12 = extend_low(tmp16_2);    //offsets(1,2,3,4)
    row21 = row11;                  //offsets(0,1,2,3)
    row22 = row12;
    row31 = row11;
    row32 = row12;
    row41 = row12;
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 16;
    tmp16_2 = reinterpret_i(tmp2uq);
    row42 = extend_low(tmp16_2);

    v_deltaPos = v_ipAngle = 9;

    PRED_INTRA_ANG4_END
}

void PredIntraAng4_5(pixel* pDst, int dstStride, pixel *refMain, int dirMode) 
{
    PRED_INTRA_ANG4_START

    tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain + 1);
    row11 = extend_low(tmp16_1);    //offsets(0,1,2,3)
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 8;
    tmp16_2 = reinterpret_i(tmp2uq);
    row12 = extend_low(tmp16_2);    //offsets(1,2,3,4)
    row21 = row11;                  //offsets(0,1,2,3)
    row22 = row12;
    row31 = row11;
    row32 = row12;
    row41 = row11;
    row42 = row12;

    v_deltaPos = v_ipAngle = 5;

    PRED_INTRA_ANG4_END
}

void PredIntraAng4_2(pixel* pDst, int dstStride, pixel *refMain, int dirMode) 
{
    PRED_INTRA_ANG4_START

    tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain + 1);
    row11 = extend_low(tmp16_1);    //offsets(0,1,2,3)
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 8;
    tmp16_2 = reinterpret_i(tmp2uq);
    row12 = extend_low(tmp16_2);    //offsets(1,2,3,4)
    row21 = row11;                  //offsets(0,1,2,3)
    row22 = row12;
    row31 = row11;
    row32 = row12;
    row41 = row11;
    row42 = row12;

    v_deltaPos = v_ipAngle = 2;

    PRED_INTRA_ANG4_END
}

void PredIntraAng4_m_2(pixel* pDst, int dstStride, pixel *refMain, int dirMode) 
{
   PRED_INTRA_ANG4_START

    tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain);
    row11 = extend_low(tmp16_1);    //offsets(0,1,2,3)
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 8;
    tmp16_2 = reinterpret_i(tmp2uq);
    row12 = extend_low(tmp16_2);    //offsets(1,2,3,4)
    row21 = row11;                  //offsets(0,1,2,3)
    row22 = row12;
    row31 = row11;
    row32 = row12;
    row41 = row11;
    row42 = row12;

    v_deltaPos = v_ipAngle = -2;

    PRED_INTRA_ANG4_END
}

void PredIntraAng4_m_5(pixel* pDst, int dstStride, pixel *refMain, int dirMode) 
{
    PRED_INTRA_ANG4_START

    tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain);
    row11 = extend_low(tmp16_1);    //offsets(0,1,2,3)
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 8;
    tmp16_2 = reinterpret_i(tmp2uq);
    row12 = extend_low(tmp16_2);    //offsets(1,2,3,4)
    row21 = row11;                  //offsets(0,1,2,3)
    row22 = row12;
    row31 = row11;
    row32 = row12;
    row41 = row11;
    row42 = row12;

    v_deltaPos = v_ipAngle = -5;

    PRED_INTRA_ANG4_END
}

void PredIntraAng4_m_9(pixel* pDst, int dstStride, pixel *refMain, int dirMode) 
{
    PRED_INTRA_ANG4_START

    tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain - 1);
    row41 = extend_low(tmp16_1);    //offsets(-2,-1,0,1)
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 8;
    tmp16_2 = reinterpret_i(tmp2uq);
    row42 = extend_low(tmp16_2);    //offsets(-1,0,1,2)

    row11 = row42;
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 16;
    tmp16_2 = reinterpret_i(tmp2uq);
    row12 = extend_low(tmp16_2);    //offsets(-1,0,1,2)

    row21 = row42;                  //offsets(0,1,2,3)
    row22 = row12;
    row31 = row42;
    row32 = row12;

    v_deltaPos = v_ipAngle = -9;

    PRED_INTRA_ANG4_END
}

void PredIntraAng4_m_13(pixel* pDst, int dstStride, pixel *refMain, int dirMode) 
{
    PRED_INTRA_ANG4_START

    tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain - 1);
    row41 = extend_low(tmp16_1);    //offsets(-2,-1,0,1)
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 8;
    tmp16_2 = reinterpret_i(tmp2uq);
    row42 = extend_low(tmp16_2);    //offsets(-1,0,1,2)

    row11 = row42;
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 16;
    tmp16_2 = reinterpret_i(tmp2uq);
    row12 = extend_low(tmp16_2);    //offsets(-1,0,1,2)

    row21 = row42;                  //offsets(0,1,2,3)
    row22 = row12;
    row31 = row41;
    row32 = row42;

    v_deltaPos = v_ipAngle = -13;

    PRED_INTRA_ANG4_END
}

void PredIntraAng4_m_17(pixel* pDst, int dstStride, pixel *refMain, int dirMode) 
{
    PRED_INTRA_ANG4_START

    tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain - 2);
    row41 = extend_low(tmp16_1);    //offsets(-3,-2,-1,0)
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 8;
    tmp16_2 = reinterpret_i(tmp2uq);
    row42 = extend_low(tmp16_2);    //offsets(-2,-1,0,1)

    row31 = row42;                  //offsets(-2,-1,0,1)
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 16;
    tmp16_2 = reinterpret_i(tmp2uq);
    row32 = extend_low(tmp16_2);    //offsets(-1,0,1,2)

    row21 = row31;                  //offsets(-2,-1,0,1)
    row22 = row32;

    row11 = row32;
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 24;
    tmp16_2 = reinterpret_i(tmp2uq);
    row12 = extend_low(tmp16_2);    //offsets(-1,0,1,2)

    v_deltaPos = v_ipAngle = -17;
    
    PRED_INTRA_ANG4_END
}

void PredIntraAng4_m_21(pixel* pDst, int dstStride, pixel *refMain, int dirMode) 
{
    PRED_INTRA_ANG4_START

    tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain - 2);
    row41 = extend_low(tmp16_1);    //offsets(-3,-2,-1,0)
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 8;
    tmp16_2 = reinterpret_i(tmp2uq);
    row42 = extend_low(tmp16_2);    //offsets(-2,-1,0,1)

    row31 = row42;                  //offsets(-2,-1,0,1)
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 16;
    tmp16_2 = reinterpret_i(tmp2uq);
    row32 = extend_low(tmp16_2);    //offsets(-1,0,1,2)

    row21 = row31;                  //offsets(-2,-1,0,1)
    row22 = row32;

    row11 = row32;
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 24;
    tmp16_2 = reinterpret_i(tmp2uq);
    row12 = extend_low(tmp16_2);    //offsets(-1,0,1,2)

    v_deltaPos = v_ipAngle = -21;

    PRED_INTRA_ANG4_END
}

void PredIntraAng4_m_26(pixel* pDst, int dstStride, pixel *refMain, int dirMode) 
{
    PRED_INTRA_ANG4_START

    tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain - 3);
    row41 = extend_low(tmp16_1);    //offsets(-4,-3,-2,-1)
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 8;
    tmp16_2 = reinterpret_i(tmp2uq);
    row42 = extend_low(tmp16_2);    //offsets(-3,-2,-1,0)

    row31 = row42;                  //offsets(-3,-2,-1,0)
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 16;
    tmp16_2 = reinterpret_i(tmp2uq);
    row32 = extend_low(tmp16_2);    //offsets(-2,-1,0,1)

    row21 = row32;                  //offsets(-2,-1,0,1)
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 24;
    tmp16_2 = reinterpret_i(tmp2uq);
    row22 = extend_low(tmp16_2);    //offsets(-1,0,1,2)

    row11 = row22;                  //offsets(-1,0,1,2)
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 32;
    tmp16_2 = reinterpret_i(tmp2uq);
    row12 = extend_low(tmp16_2);    //offsets(0,1,2,3)

    v_deltaPos = v_ipAngle = -26;

    PRED_INTRA_ANG4_END
}

void PredIntraAng4_m_32(pixel* pDst, int dstStride, pixel *refMain, int /*dirMode*/) 
{
    Vec16uc tmp16_1, tmp16_2;
    tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain);    //-1,0,1,2
    store_partial(const_int(4), pDst, tmp16_1);
    tmp16_2 = (Vec16uc)load_partial(const_int(8), refMain - 1); //-2,-1,0,1
    store_partial(const_int(4), pDst + dstStride, tmp16_2);
    tmp16_2 = (Vec16uc)load_partial(const_int(8), refMain - 2);
    store_partial(const_int(4), pDst + 2 * dstStride, tmp16_2);
    tmp16_2 = (Vec16uc)load_partial(const_int(8), refMain - 3);
    store_partial(const_int(4), pDst + 3 * dstStride, tmp16_2);    
}

typedef void (*PredIntraAng4x4_table)(pixel* pDst, int dstStride, pixel *refMain, int dirMode);
PredIntraAng4x4_table PredIntraAng4[] = {
    /* PredIntraAng4_0 is replaced with PredIntraAng4_2. For PredIntraAng4_0 we are going through default path in the xPredIntraAng4x4 because we cannot afford to pass large number arguments for this function. */
    PredIntraAng4_32,
    PredIntraAng4_26,
    PredIntraAng4_21,
    PredIntraAng4_17,
    PredIntraAng4_13,
    PredIntraAng4_9,
    PredIntraAng4_5,
    PredIntraAng4_2,
    PredIntraAng4_2,    //Intentionally wrong! It should be "PredIntraAng4_0" here. 
    PredIntraAng4_m_2,
    PredIntraAng4_m_5,
    PredIntraAng4_m_9,
    PredIntraAng4_m_13,
    PredIntraAng4_m_17,
    PredIntraAng4_m_21,
    PredIntraAng4_m_26,
    PredIntraAng4_m_32,
    PredIntraAng4_m_26,
    PredIntraAng4_m_21,
    PredIntraAng4_m_17,
    PredIntraAng4_m_13,
    PredIntraAng4_m_9,
    PredIntraAng4_m_5,
    PredIntraAng4_m_2,
    PredIntraAng4_2,    //Intentionally wrong! It should be "PredIntraAng4_0" here. 
    PredIntraAng4_2,
    PredIntraAng4_5,
    PredIntraAng4_9,
    PredIntraAng4_13,
    PredIntraAng4_17,
    PredIntraAng4_21,
    PredIntraAng4_26,
    PredIntraAng4_32
};
void xPredIntraAng4x4(int /*bitDepth*/, pixel* pDst, int dstStride, int width, int dirMode, pixel *refLeft, pixel *refAbove, bool bFilter = true)
{
    assert(dirMode > 1); //no planar and dc
    static const int mode_to_angle_table[] = {32, 26, 21, 17, 13, 9, 5, 2, 0, -2, -5, -9, -13, -17, -21, -26, -32, -26, -21, -17, -13, -9, -5, -2, 0, 2, 5, 9, 13, 17, 21, 26, 32};
    static const int mode_to_invAng_table[] = {256, 315, 390, 482, 630, 910, 1638, 4096, 0, 4096, 1638, 910, 630, 482, 390, 315, 256, 315, 390, 482, 630, 910, 1638, 4096, 0, 4096, 1638, 910, 630, 482, 390, 315, 256};
    int intraPredAngle = mode_to_angle_table[dirMode-2];
    int invAngle       = mode_to_invAng_table[dirMode-2];

    bool modeHor       = (dirMode < 18);
    bool modeVer       = !modeHor;

    // Do angular predictions
    pixel* refMain;
    pixel* refSide;

    // Initialise the Main and Left reference array.
    if (intraPredAngle < 0)
    {
        int blkSize = width;
        refMain = (modeVer ? refAbove : refLeft);     // + (blkSize - 1);
        refSide = (modeVer ? refLeft : refAbove);     // + (blkSize - 1);

        // Extend the Main reference to the left.
        int invAngleSum    = 128;     // rounding for (shift by 8)
        for (int k = -1; k > blkSize * intraPredAngle >> 5; k--)
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

    // bfilter will always be true for exactly vertical/horizontal modes
    if (intraPredAngle == 0)  // Exactly hotizontal/vertical angles
    {
        if (modeHor)
        {
            Vec16uc v_main;
            v_main = load_partial(const_int(4), (void*)(refMain + 1));

            Vec16uc tmp16;
            tmp16 = blend16c<0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23>(v_main, v_main);
            tmp16 = blend16c<0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23>(tmp16, tmp16);
            Vec2uq tmp;

            if (bFilter)
            {
                Vec16uc v_temp;
                Vec8s v_side_0; // refSide[0] value in a vector
                v_temp = load_partial(const_int(8), (void*)refSide);
                v_side_0 = broadcast(const_int(0), (Vec8s)v_temp);
                v_side_0 = v_side_0 & 0x00FF;

                //shift v_side by 1 element (1 byte)
                tmp = reinterpret_i(v_temp);
                tmp = tmp >> 8;
                v_temp = reinterpret_i(tmp);
                Vec8s v_side = extend_low(v_temp);

                Vec8s row0 = extend_low(tmp16);
                v_side -= v_side_0;
                v_side = v_side >> 1;
                row0 += v_side;
                row0 = min(max(0, row0), 255);
                Vec16uc v_res(compress_unsafe(row0, 0));
                store_partial(const_int(4), pDst, v_res);
            }
            else
            {
                store_partial(const_int(4), pDst, tmp16);
            }

            tmp = (Vec2uq)tmp16;
            tmp >>= 32;
            store_partial(const_int(4), pDst + dstStride, tmp);

            tmp = blend2q<1, 3>(reinterpret_i(tmp16), reinterpret_i(tmp16));
            store_partial(const_int(4), pDst + (2 * dstStride), tmp);

            tmp >>= 32;
            store_partial(const_int(4), pDst + (3 * dstStride), tmp);
        }
        else
        {
            Vec16uc v_main;
            v_main = load_partial(const_int(4), refMain + 1);
            store_partial(const_int(4), pDst, v_main);
            store_partial(const_int(4), pDst + dstStride, v_main);
            store_partial(const_int(4), pDst + (2 * dstStride), v_main);
            store_partial(const_int(4), pDst + (3 * dstStride), v_main);
            if (bFilter)
            {
                for (int k = 0; k < 4; k++)
                {
                    pDst[k * dstStride] = (pixel)Clip3((short)0, (short)((1 << 8) - 1), static_cast<short>((pDst[k * dstStride]) + ((refSide[k + 1] - refSide[0]) >> 1)));
                }
            }
        }
    }
    else
    {
        PredIntraAng4[dirMode-2](pDst, dstStride, refMain, dirMode);
    }
}

#endif /* if HIGH_BIT_DEPTH */

#if HIGH_BIT_DEPTH
#else
#define PREDANG_CALCROW_VER(X) { \
        LOADROW(row11, GETAP(lookIdx, X)); \
        LOADROW(row12, GETAP(lookIdx, X) + 1); \
        CALCROW(row11, row11, row12); \
        store_partial(const_int(8), pDst + (X * dstStride), compress(row11, row11)); \
}

#define PREDANG_CALCROW_HOR(X, rowx) { \
        LOADROW(row11, GETAP(lookIdx, X)); \
        LOADROW(row12, GETAP(lookIdx, X) + 1); \
        CALCROW(rowx, row11, row12); \
}

// ROW is a Vec8s variable, X is the index in of data to be loaded
#define LOADROW(ROW, X) { \
        tmp = load_partial(const_int(8), refMain + 1 + X); \
        ROW = extend_low(tmp); \
}

#define CALCROW(RES, ROW1, ROW2) { \
        v_deltaPos += v_ipAngle; \
        v_deltaFract = v_deltaPos & thirty1; \
        RES = ((thirty2 - v_deltaFract) * ROW1 + (v_deltaFract * ROW2) + 16) >> 5; \
}

void PredIntraAng8_32(pixel* pDst, int dstStride, pixel *refMain, int /*dirMode*/) 
{
    Vec8s tmp;
    tmp = load_partial(const_int(8), refMain + 2);        //-1,0,1,2
    store_partial(const_int(8), pDst, tmp);
    tmp = load_partial(const_int(8), refMain + 3);     //-2,-1,0,1
    store_partial(const_int(8), pDst + dstStride, tmp);
    tmp = load_partial(const_int(8), refMain + 4);
    store_partial(const_int(8), pDst + 2 * dstStride, tmp);
    tmp = load_partial(const_int(8), refMain + 5);
    store_partial(const_int(8), pDst + 3 * dstStride, tmp);
    tmp = load_partial(const_int(8), refMain + 6);
    store_partial(const_int(8), pDst + 4 * dstStride, tmp);
    tmp = load_partial(const_int(8), refMain + 7);
    store_partial(const_int(8), pDst + 5 * dstStride, tmp);
    tmp = load_partial(const_int(8), refMain + 8);
    store_partial(const_int(8), pDst + 6 * dstStride, tmp);
    tmp = load_partial(const_int(8), refMain + 9);
    store_partial(const_int(8), pDst + 7 * dstStride, tmp);
}

void PredIntraAng8_26(pixel* pDst, int dstStride, pixel *refMain, int dirMode) 
{
    // Map the mode index to main prediction direction and angle
    bool modeHor       = (dirMode < 18);
    bool modeVer       = !modeHor;
    int intraPredAngle = modeVer ? (int)dirMode - VER_IDX : modeHor ? -((int)dirMode - HOR_IDX) : 0;
    int lookIdx = intraPredAngle;
    int absAng         = abs(intraPredAngle);
    int signAng        = intraPredAngle < 0 ? -1 : 1;

    // Set bitshifts and scale the angle parameter to block size
    int angTable[9]    = { 0,    2,    5,   9,  13,  17,  21,  26,  32 };
    absAng             = angTable[absAng];
    intraPredAngle     = signAng * absAng;

    if (modeHor)         // Near horizontal modes
    {
        Vec16uc tmp;
        Vec8s row11, row12;
        Vec16uc row1, row2, row3, row4, tmp16_1, tmp16_2;
        Vec8s v_deltaFract, v_deltaPos, thirty2(32), thirty1(31), v_ipAngle;
        Vec8s tmp1, tmp2;
        v_deltaPos = 0;
        v_ipAngle = intraPredAngle;

        PREDANG_CALCROW_HOR(0, tmp1);
        PREDANG_CALCROW_HOR(1, tmp2);
        row1 = compress(tmp1, tmp2);
        PREDANG_CALCROW_HOR(2, tmp1);
        PREDANG_CALCROW_HOR(3, tmp2);
        row2 = compress(tmp1, tmp2);
        PREDANG_CALCROW_HOR(4, tmp1);
        PREDANG_CALCROW_HOR(5, tmp2);
        row3 = compress(tmp1, tmp2);
        PREDANG_CALCROW_HOR(6, tmp1);
        PREDANG_CALCROW_HOR(7, tmp2);
        row4 = compress(tmp1, tmp2);

        PRED_INTRA_ANG8_MIDDLE

        PREDANG_CALCROW_VER(0);
        PREDANG_CALCROW_VER(1);
        PREDANG_CALCROW_VER(2);
        PREDANG_CALCROW_VER(3);
        PREDANG_CALCROW_VER(4);
        PREDANG_CALCROW_VER(5);
        PREDANG_CALCROW_VER(6);
        PREDANG_CALCROW_VER(7);
    }
}

void PredIntraAng8_5(pixel* pDst, int dstStride, pixel *refMain, int dirMode) 
{
        PRED_INTRA_ANG8_START

        LOADROW(row11, 0);
        LOADROW(row12, 1);
        CALCROW(tmp1, row11, row12);
        CALCROW(tmp2, row11, row12);
        row1 = compress(tmp1, tmp2);
        CALCROW(tmp1, row11, row12);
        CALCROW(tmp2, row11, row12);
        row2 = compress(tmp1, tmp2);
        CALCROW(tmp1, row11, row12);
        CALCROW(tmp2, row11, row12);
        row3 = compress(tmp1, tmp2);
        row11 = row12;
        LOADROW(row12, 2);
        CALCROW(tmp1, row11, row12);
        CALCROW(tmp2, row11, row12);
        row4 = compress(tmp1, tmp2);

        PRED_INTRA_ANG8_MIDDLE

        LOADROW(row11, 0);
        LOADROW(row12, 1);
        CALCROW(tmp1, row11, row12);
        CALCROW(tmp2, row11, row12);
        store_partial(const_int(8), pDst, compress(tmp1, tmp1));
        store_partial(const_int(8), pDst + dstStride, compress(tmp2, tmp2));
        CALCROW(tmp1, row11, row12);
        CALCROW(tmp2, row11, row12);
        store_partial(const_int(8), pDst + (2 * dstStride), compress(tmp1, tmp1));
        store_partial(const_int(8), pDst + (3 * dstStride), compress(tmp2, tmp2));
        CALCROW(tmp1, row11, row12);
        CALCROW(tmp2, row11, row12);
        store_partial(const_int(8), pDst + (4 * dstStride), compress(tmp1, tmp1));
        store_partial(const_int(8), pDst + (5 * dstStride), compress(tmp2, tmp2));
        row11 = row12;
        LOADROW(row12, 2);
        CALCROW(tmp1, row11, row12);
        CALCROW(tmp2, row11, row12);
        store_partial(const_int(8), pDst + (6 * dstStride), compress(tmp1, tmp1));
        store_partial(const_int(8), pDst + (7 * dstStride), compress(tmp2, tmp2));
    }
}

void PredIntraAng8_2(pixel* pDst, int dstStride, pixel *refMain, int dirMode) 
{
        PRED_INTRA_ANG8_START

        LOADROW(row11, 0);
        LOADROW(row12, 1);
        CALCROW(tmp1, row11, row12);
        CALCROW(tmp2, row11, row12);
        row1 = compress(tmp1, tmp2);
        CALCROW(tmp1, row11, row12);
        CALCROW(tmp2, row11, row12);
        row2 = compress(tmp1, tmp2);
        CALCROW(tmp1, row11, row12);
        CALCROW(tmp2, row11, row12);
        row3 = compress(tmp1, tmp2);
        CALCROW(tmp1, row11, row12);
        CALCROW(tmp2, row11, row12);
        row4 = compress(tmp1, tmp2);

        PRED_INTRA_ANG8_MIDDLE

        LOADROW(row11, 0);
        LOADROW(row12, 1);
        CALCROW(tmp1, row11, row12);
        CALCROW(tmp2, row11, row12);
        store_partial(const_int(8), pDst, compress(tmp1, tmp1));
        store_partial(const_int(8), pDst + dstStride, compress(tmp2, tmp2));
        CALCROW(tmp1, row11, row12);
        CALCROW(tmp2, row11, row12);
        store_partial(const_int(8), pDst + (2 * dstStride), compress(tmp1, tmp1));
        store_partial(const_int(8), pDst + (3 * dstStride), compress(tmp2, tmp2));
        CALCROW(tmp1, row11, row12);
        CALCROW(tmp2, row11, row12);
        store_partial(const_int(8), pDst + (4 * dstStride), compress(tmp1, tmp1));
        store_partial(const_int(8), pDst + (5 * dstStride), compress(tmp2, tmp2));
        CALCROW(tmp1, row11, row12);
        CALCROW(tmp2, row11, row12);
        store_partial(const_int(8), pDst + (6 * dstStride), compress(tmp1, tmp1));
        store_partial(const_int(8), pDst + (7 * dstStride), compress(tmp2, tmp2));
    }
}

void PredIntraAng8_m_2(pixel* pDst, int dstStride, pixel *refMain, int dirMode) 
{
        PRED_INTRA_ANG8_START

        LOADROW(row11, -1);
        LOADROW(row12, 0);
        CALCROW(tmp1, row11, row12);
        CALCROW(tmp2, row11, row12);
        row1 = compress(tmp1, tmp2);
        CALCROW(tmp1, row11, row12);
        CALCROW(tmp2, row11, row12);
        row2 = compress(tmp1, tmp2);
        CALCROW(tmp1, row11, row12);
        CALCROW(tmp2, row11, row12);
        row3 = compress(tmp1, tmp2);
        CALCROW(tmp1, row11, row12);
        CALCROW(tmp2, row11, row12);
        row4 = compress(tmp1, tmp2);

        PRED_INTRA_ANG8_MIDDLE

        LOADROW(row11, -1);
        LOADROW(row12, 0);
        CALCROW(tmp1, row11, row12);
        CALCROW(tmp2, row11, row12);
        store_partial(const_int(8), pDst, compress(tmp1, tmp1));
        store_partial(const_int(8), pDst + (dstStride), compress(tmp2, tmp2));
        CALCROW(tmp1, row11, row12);
        CALCROW(tmp2, row11, row12);
        store_partial(const_int(8), pDst + (2 * dstStride), compress(tmp1, tmp1));
        store_partial(const_int(8), pDst + (3 * dstStride), compress(tmp2, tmp2));
        CALCROW(tmp1, row11, row12);
        CALCROW(tmp2, row11, row12);
        store_partial(const_int(8), pDst + (4 * dstStride), compress(tmp1, tmp1));
        store_partial(const_int(8), pDst + (5 * dstStride), compress(tmp2, tmp2));
        CALCROW(tmp1, row11, row12);
        CALCROW(tmp2, row11, row12);
        store_partial(const_int(8), pDst + (6 * dstStride), compress(tmp1, tmp1));
        store_partial(const_int(8), pDst + (7 * dstStride), compress(tmp2, tmp2));
    }
}

void PredIntraAng8_m_5(pixel* pDst, int dstStride, pixel *refMain, int dirMode) 
{
        PRED_INTRA_ANG8_START

        LOADROW(row11, -1);
        LOADROW(row12, 0);
        CALCROW(tmp1, row11, row12);
        CALCROW(tmp2, row11, row12);
        row1 = compress(tmp1, tmp2);
        CALCROW(tmp1, row11, row12);
        CALCROW(tmp2, row11, row12);
        row2 = compress(tmp1, tmp2);
        CALCROW(tmp1, row11, row12);
        CALCROW(tmp2, row11, row12);
        row3 = compress(tmp1, tmp2);
        row12 = row11;
        LOADROW(row11, -2);
        CALCROW(tmp1, row11, row12);
        CALCROW(tmp2, row11, row12);
        row4 = compress(tmp1, tmp2);

        PRED_INTRA_ANG8_MIDDLE

        LOADROW(row11, -1);
        LOADROW(row12, 0);
        CALCROW(tmp1, row11, row12);
        CALCROW(tmp2, row11, row12);
        store_partial(const_int(8), pDst, compress(tmp1, tmp1));
        store_partial(const_int(8), pDst + (dstStride), compress(tmp2, tmp2));
        CALCROW(tmp1, row11, row12);
        CALCROW(tmp2, row11, row12);
        store_partial(const_int(8), pDst + (2 * dstStride), compress(tmp1, tmp1));
        store_partial(const_int(8), pDst + (3 * dstStride), compress(tmp2, tmp2));
        CALCROW(tmp1, row11, row12);
        CALCROW(tmp2, row11, row12);
        store_partial(const_int(8), pDst + (4 * dstStride), compress(tmp1, tmp1));
        store_partial(const_int(8), pDst + (5 * dstStride), compress(tmp2, tmp2));
        row12 = row11;
        LOADROW(row11, -2);
        CALCROW(tmp1, row11, row12);
        CALCROW(tmp2, row11, row12);
        store_partial(const_int(8), pDst + (6 * dstStride), compress(tmp1, tmp1));
        store_partial(const_int(8), pDst + (7 * dstStride), compress(tmp2, tmp2));
    }
}     

void PredIntraAng8_m_32(pixel* pDst, int dstStride, pixel *refMain, int /*dirMode*/) 
{
    Vec16uc tmp;
    tmp = load_partial(const_int(8), refMain);        //-1,0,1,2
    store_partial(const_int(8), pDst, tmp);
    tmp = load_partial(const_int(8), refMain - 1);     //-2,-1,0,1
    store_partial(const_int(8), pDst + dstStride, tmp);
    tmp = load_partial(const_int(8), refMain - 2);
    store_partial(const_int(8), pDst + 2 * dstStride, tmp);
    tmp = load_partial(const_int(8), refMain - 3);
    store_partial(const_int(8), pDst + 3 * dstStride, tmp);
    tmp = load_partial(const_int(8), refMain - 4);
    store_partial(const_int(8), pDst + 4 * dstStride, tmp);
    tmp = load_partial(const_int(8), refMain - 5);
    store_partial(const_int(8), pDst + 5 * dstStride, tmp);
    tmp = load_partial(const_int(8), refMain - 6);
    store_partial(const_int(8), pDst + 6 * dstStride, tmp);
    tmp = load_partial(const_int(8), refMain - 7);
    store_partial(const_int(8), pDst + 7 * dstStride, tmp);
}

typedef void (*PredIntraAng8x8_table)(pixel* pDst, int dstStride, pixel *refMain, int dirMode);
PredIntraAng8x8_table PredIntraAng8[] = {
    /* 
    PredIntraAng8_0 is replaced with PredIntraAng8_2. For PredIntraAng8_0 we are going through default path in the xPredIntraAng8x8 because we cannot afford to pass large number arguments for this function. 
    Path for PredIntraAng8_21, PredIntraAng8_m_21, PredIntraAng8_17, PredIntraAng8_m_17, PredIntraAng8_13, PredIntraAng8_m_13, PredIntraAng8_9, PredIntraAng8_m_9 is same as PredIntraAng8_26.
    */
    PredIntraAng8_32,
    PredIntraAng8_26,
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_21" here.      
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_17" here.
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_13" here.
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_9" here.
    PredIntraAng8_5,    
    PredIntraAng8_2,
    PredIntraAng8_2,        //Intentionally wrong! It should be "PredIntraAng8_0" here.
    PredIntraAng8_m_2,
    PredIntraAng8_m_5,
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_m_9" here.
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_m_13" here.
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_m_17" here.
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_m_21" here.
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_m_26" here.
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_m_32" here.
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_m_26" here.
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_m_21" here.
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_m_17" here.
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_m_13" here.
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_m_9" here.
    PredIntraAng8_m_5,
    PredIntraAng8_m_2,
    PredIntraAng8_2,        //Intentionally wrong! It should be "PredIntraAng8_0" here.
    PredIntraAng8_2,
    PredIntraAng8_5,
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_9" here.
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_13" here.
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_17" here.
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_21" here.
    PredIntraAng8_26,
    PredIntraAng8_32
};


void xPredIntraAng8x8(int bitDepth, pixel* pDst, int dstStride, int width, int dirMode, pixel *refLeft, pixel *refAbove, bool bFilter = true)
{
    int k;
    int blkSize        = width;

    assert(dirMode > 1); //no planar and dc
    static const int mode_to_angle_table[] = {32, 26, 21, 17, 13, 9, 5, 2, 0, -2, -5, -9, -13, -17, -21, -26, -32, -26, -21, -17, -13, -9, -5, -2, 0, 2, 5, 9, 13, 17, 21, 26, 32};
    static const int mode_to_invAng_table[] = {256, 315, 390, 482, 630, 910, 1638, 4096, 0, 4096, 1638, 910, 630, 482, 390, 315, 256, 315, 390, 482, 630, 910, 1638, 4096, 0, 4096, 1638, 910, 630, 482, 390, 315, 256};
    int intraPredAngle = mode_to_angle_table[dirMode-2];
    int invAngle       = mode_to_invAng_table[dirMode-2];
    bool modeHor       = (dirMode < 18);
    bool modeVer       = !modeHor;
    pixel* refMain;
    pixel* refSide;

    // Initialise the Main and Left reference array.
    if (intraPredAngle < 0)
    {
        refMain = (modeVer ? refAbove : refLeft);     // + (blkSize - 1);
        refSide = (modeVer ? refLeft : refAbove);     // + (blkSize - 1);

        // Extend the Main reference to the left.
        int invAngleSum    = 128;     // rounding for (shift by 8)
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
            Vec16uc v_temp;
            Vec16uc tmp1;

            v_temp.load(refMain + 1);
            Vec8s v_main;
            v_main = extend_low(v_temp);

            if (bFilter)
            {
                Vec8s v_side_0(refSide[0]); // refSide[0] value in a vector
                Vec16uc v_temp16;
                v_temp16.load(refSide + 1);
                Vec8s v_side;
                v_side = extend_low(v_temp16);

                Vec8s row0;
                row0 = permute8s<0, 0, 0, 0, 0, 0, 0, 0>(v_main);
                v_side -= v_side_0;
                v_side = v_side >> 1;
                row0 = row0 + v_side;
                row0 = min(max(0, row0), (1 << bitDepth) - 1);

                tmp1 = compress(row0, row0);
                store_partial(const_int(8), pDst, tmp1);            //row0
            }
            else
            {
                tmp1 = permute16uc<0, 0, 0, 0, 0, 0, 0, 0, -256, -256, -256, -256, -256, -256, -256, -256>(v_temp);
                store_partial(const_int(8), pDst, tmp1); //row0
            }
            tmp1 = permute16uc<1, 1, 1, 1, 1, 1, 1, 1, -256, -256, -256, -256, -256, -256, -256, -256>(v_temp);
            store_partial(const_int(8), pDst + (1 * dstStride), tmp1); //row1

            tmp1 = permute16uc<2, 2, 2, 2, 2, 2, 2, 2, -256, -256, -256, -256, -256, -256, -256, -256>(v_temp);
            store_partial(const_int(8), pDst + (2 * dstStride), tmp1); //row2

            tmp1 = permute16uc<3, 3, 3, 3, 3, 3, 3, 3, -256, -256, -256, -256, -256, -256, -256, -256>(v_temp);
            store_partial(const_int(8), pDst + (3 * dstStride), tmp1); //row3

            tmp1 = permute16uc<4, 4, 4, 4, 4, 4, 4, 4, -256, -256, -256, -256, -256, -256, -256, -256>(v_temp);
            store_partial(const_int(8), pDst + (4 * dstStride), tmp1); //row4

            tmp1 = permute16uc<5, 5, 5, 5, 5, 5, 5, 5, -256, -256, -256, -256, -256, -256, -256, -256>(v_temp);
            store_partial(const_int(8), pDst + (5 * dstStride), tmp1); //row5

            tmp1 = permute16uc<6, 6, 6, 6, 6, 6, 6, 6, -256, -256, -256, -256, -256, -256, -256, -256>(v_temp);
            store_partial(const_int(8), pDst + (6 * dstStride), tmp1); //row6

            tmp1 = permute16uc<7, 7, 7, 7, 7, 7, 7, 7, -256, -256, -256, -256, -256, -256, -256, -256>(v_temp);
            store_partial(const_int(8), pDst + (7 * dstStride), tmp1); //row7
        }
        else
        {
            Vec16uc v_main;
            v_main = load_partial(const_int(8), refMain + 1);
            store_partial(const_int(8), pDst, v_main);
            store_partial(const_int(8), pDst + dstStride, v_main);
            store_partial(const_int(8), pDst + (2 * dstStride), v_main);
            store_partial(const_int(8), pDst + (3 * dstStride), v_main);
            store_partial(const_int(8), pDst + (4 * dstStride), v_main);
            store_partial(const_int(8), pDst + (5 * dstStride), v_main);
            store_partial(const_int(8), pDst + (6 * dstStride), v_main);
            store_partial(const_int(8), pDst + (7 * dstStride), v_main);

            if (bFilter)
            {
                Vec16uc v_temp;
                Vec8s v_side_0(refSide[0]); // refSide[0] value in a vector

                v_temp.load(refSide + 1);
                Vec8s v_side;
                v_side = extend_low(v_temp);

                v_temp.load(refMain + 1);
                Vec8s row0;
                row0 = permute16uc<0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1>(v_temp);
                v_side -= v_side_0;
                v_side = v_side >> 1;
                row0 = row0 + v_side;
                row0 = min(max(0, row0), (1 << bitDepth) - 1);

                pDst[0 * dstStride] = row0[0];
                pDst[1 * dstStride] = row0[1];
                pDst[2 * dstStride] = row0[2];
                pDst[3 * dstStride] = row0[3];
                pDst[4 * dstStride] = row0[4];
                pDst[5 * dstStride] = row0[5];
                pDst[6 * dstStride] = row0[6];
                pDst[7 * dstStride] = row0[7];
            }
        }
    }
    else
    {
        PredIntraAng8[dirMode-2](pDst, dstStride, refMain, dirMode);
    }    
}

#undef PREDANG_CALCROW_VER
#undef PREDANG_CALCROW_HOR
#undef LOADROW
#undef CALCROW
#endif /* if HIGH_BIT_DEPTH */

//16x16
#if HIGH_BIT_DEPTH
#else
#define PREDANG_CALCROW_VER(X) { \
        LOADROW(row11L, row11H, GETAP(lookIdx, X)); \
        LOADROW(row12L, row12H, GETAP(lookIdx, X) + 1); \
        CALCROW(row11L, row11H, row11L, row11H, row12L, row12H); \
        /*compress(row11L, row11H).store(pDst + ((X)*dstStride));*/ \
        itmp = _mm_packus_epi16(row11L, row11H); \
        _mm_storeu_si128((__m128i*)(pDst + ((X)*dstStride)), itmp); \
}

#define PREDANG_CALCROW_HOR(X, rowx) { \
        LOADROW(row11L, row11H, GETAP(lookIdx, (X))); \
        LOADROW(row12L, row12H, GETAP(lookIdx, (X)) + 1); \
        CALCROW(row11L, row11H, row11L, row11H, row12L, row12H); \
        /*rowx = compress(row11L, row11H);*/  \
        rowx = _mm_packus_epi16(row11L, row11H); \
}

// ROWL/H is a Vec8s variable, X is the index in of data to be loaded
#define LOADROW(ROWL, ROWH, X) { \
        /*tmp.load(refMain + 1 + (X)); */ \
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 1 + (X))); \
        /* ROWL = extend_low(tmp);*/  \
        ROWL = _mm_unpacklo_epi8(itmp, _mm_setzero_si128()); \
        /*ROWH = extend_high(tmp);*/  \
        ROWH = _mm_unpackhi_epi8(itmp, _mm_setzero_si128()); \
}

#define CALCROW(RESL, RESH, ROW1L, ROW1H, ROW2L, ROW2H) { \
        /*v_deltaPos += v_ipAngle; \
        v_deltaFract = v_deltaPos & thirty1;*/ \
        v_deltaPos = _mm_add_epi16(v_deltaPos, v_ipAngle); \
        v_deltaFract = _mm_and_si128(v_deltaPos, thirty1); \
        /*RESL = ((thirty2 - v_deltaFract) * ROW1L + (v_deltaFract * ROW2L) + 16) >> 5; \
        RESH = ((thirty2 - v_deltaFract) * ROW1H + (v_deltaFract * ROW2H) + 16) >> 5;*/ \
        it1 = _mm_sub_epi16(thirty2, v_deltaFract); \
        it2 = _mm_mullo_epi16(it1, ROW1L); \
        it3 = _mm_mullo_epi16(v_deltaFract, ROW2L); \
        it2 = _mm_add_epi16(it2, it3); \
        i16 = _mm_set1_epi16(16); \
        it2 = _mm_add_epi16(it2, i16); \
        RESL = _mm_srai_epi16(it2, 5); \
        \
        it2 = _mm_mullo_epi16(it1, ROW1H); \
        it3 = _mm_mullo_epi16(v_deltaFract, ROW2H); \
        it2 = _mm_add_epi16(it2, it3); \
        it2 = _mm_add_epi16(it2, i16); \
        RESH = _mm_srai_epi16(it2, 5); \
}

#define  BLND2_16(R1, R2) { \
        /*tmp1 = blend16uc<0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23>(R1, R2); */ \
        itmp1 = _mm_unpacklo_epi8(R1, R2); \
        /*tmp2 = blend16uc<8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31>(R1, R2);*/ \
        itmp2 = _mm_unpackhi_epi8(R1, R2); \
        R1 = itmp1; \
        R2 = itmp2; \
}

#define MB4(R1, R2, R3, R4) { \
        BLND2_16(R1, R2) \
        BLND2_16(R3, R4) \
        /*tmp1 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>((Vec8s)R1, (Vec8s)R3);*/  \
        itmp1 = _mm_unpacklo_epi16(R1, R3); \
        /* tmp2 = blend8s<4, 12, 5, 13, 6, 14, 7, 15>((Vec8s)R1, (Vec8s)R3);*/ \
        itmp2 = _mm_unpackhi_epi16(R1, R3); \
        R1 = itmp1; \
        R3 = itmp2; \
        /*R1 = tmp1; \
        R3 = tmp2;*/ \
        /*tmp1 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>((Vec8s)R2, (Vec8s)R4); \
        tmp2 = blend8s<4, 12, 5, 13, 6, 14, 7, 15>((Vec8s)R2, (Vec8s)R4);*/ \
        itmp1 = _mm_unpacklo_epi16(R2, R4); \
        itmp2 = _mm_unpackhi_epi16(R2, R4); \
        R2 = itmp1; \
        R4 = itmp2; \
        /*R2 = tmp1; \
        R4 = tmp2;*/ \
}

#define BLND2_4(R1, R2) { \
        /* tmp1 = blend4i<0, 4, 1, 5>((Vec4i)R1, (Vec4i)R2); \
        tmp2 = blend4i<2, 6, 3, 7>((Vec4i)R1, (Vec4i)R2); */ \
        itmp1 = _mm_unpacklo_epi32(R1, R2); \
        itmp2 = _mm_unpackhi_epi32(R1, R2); \
        R1 = itmp1; \
        R2 = itmp2; \
        /*R1 = tmp1; \
        R2 = tmp2; */\
}

#define BLND2_2(R1, R2) { \
        /*tmp1 = blend2q<0, 2>((Vec2q)R1, (Vec2q)R2); \
        tmp2 = blend2q<1, 3>((Vec2q)R1, (Vec2q)R2);*/ \
        itmp1 = _mm_unpacklo_epi64(R1, R2); \
        itmp2 = _mm_unpackhi_epi64(R1, R2); \
        /*tmp1.store(pDst); */ \
        _mm_storeu_si128((__m128i*)pDst, itmp1); \
        pDst += dstStride; \
        /*tmp2.store(pDst);*/ \
        _mm_storeu_si128((__m128i*)pDst, itmp2); \
        pDst += dstStride; \
}

#define CALC_BLND_8ROWS(R1, R2, R3, R4, R5, R6, R7, R8, X) { \
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
        BLND2_4(R4, R8); \
}

void xPredIntraAng16x16(int bitDepth, pixel* pDst, int dstStride, int width, int dirMode, pixel *refLeft, pixel *refAbove, bool bFilter = true)
{
    int k;
    int blkSize        = width;

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
            Vec16uc v_temp;
            Vec16uc tmp1;
            v_temp.load(refMain + 1);

            if (bFilter)
            {
                Vec8s v_side_0(refSide[0]); // refSide[0] value in a vector
                Vec16uc v_temp16;
                v_temp16.load(refSide + 1);
                Vec8s v_side;
                v_side = extend_low(v_temp16);

                Vec8s row01, row02, ref(refMain[1]);
                v_side -= v_side_0;
                v_side = v_side >> 1;
                row01 = ref + v_side;
                row01 = min(max(0, row01), (1 << bitDepth) - 1);

                v_side = extend_high(v_temp16);
                v_side -= v_side_0;
                v_side = v_side >> 1;
                row02 = ref + v_side;
                row02 = min(max(0, row02), (1 << bitDepth) - 1);

                tmp1 = compress_unsafe(row01, row02);
                tmp1.store(pDst);            //row0
            }
            else
            {
                tmp1 = permute16uc<0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0>(v_temp);
                tmp1.store(pDst); //row0
            }

            tmp1 = permute16uc<1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1>(v_temp);
            tmp1.store(pDst + (1 * dstStride)); //row1

            tmp1 = permute16uc<2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2>(v_temp);
            tmp1.store(pDst + (2 * dstStride)); //row2

            tmp1 = permute16uc<3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3>(v_temp);
            tmp1.store(pDst + (3 * dstStride)); //row3

            tmp1 = permute16uc<4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4>(v_temp);
            tmp1.store(pDst + (4 * dstStride)); //row4

            tmp1 = permute16uc<5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5>(v_temp);
            tmp1.store(pDst + (5 * dstStride)); //row5

            tmp1 = permute16uc<6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6>(v_temp);
            tmp1.store(pDst + (6 * dstStride)); //row6

            tmp1 = permute16uc<7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7>(v_temp);
            tmp1.store(pDst + (7 * dstStride)); //row7

            tmp1 = permute16uc<8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8>(v_temp);
            tmp1.store(pDst + (8 * dstStride)); //row8

            tmp1 = permute16uc<9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9>(v_temp);
            tmp1.store(pDst + (9 * dstStride)); //row9

            tmp1 = permute16uc<10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10>(v_temp);
            tmp1.store(pDst + (10 * dstStride)); //row10

            tmp1 = permute16uc<11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11>(v_temp);
            tmp1.store(pDst + (11 * dstStride)); //row11

            tmp1 = permute16uc<12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12>(v_temp);
            tmp1.store(pDst + (12 * dstStride)); //row12

            tmp1 = permute16uc<13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13>(v_temp);
            tmp1.store(pDst + (13 * dstStride)); //row13

            tmp1 = permute16uc<14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14>(v_temp);
            tmp1.store(pDst + (14 * dstStride)); //row14

            tmp1 = permute16uc<15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15>(v_temp);
            tmp1.store(pDst + (15 * dstStride)); //row15
        }
        else
        {
            Vec16uc v_main;
//            v_main.load(refMain + 1);
            v_main = _mm_loadu_si128((__m128i const*)(refMain + 1));

            _mm_storeu_si128((__m128i*)pDst, v_main);
            _mm_storeu_si128((__m128i*)(pDst + dstStride), v_main);
            _mm_storeu_si128((__m128i*)(pDst + (2 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(pDst + (3 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(pDst + (4 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(pDst + (5 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(pDst + (6 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(pDst + (7 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(pDst + (8 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(pDst + (9 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(pDst + (10 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(pDst + (11 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(pDst + (12 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(pDst + (13 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(pDst + (14 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(pDst + (15 * dstStride)), v_main);

            if (bFilter)
            {
                Vec16uc v_temp;
                Vec8s v_side_0(refSide[0]); // refSide[0] value in a vector

                v_temp.load(refSide + 1);
                Vec8s v_side;
                v_side = extend_low(v_temp);

                Vec8s row0, ref(refMain[1]);
                v_side -= v_side_0;
                v_side = v_side >> 1;
                row0 = ref + v_side;
                row0 = min(max(0, row0), (1 << bitDepth) - 1);

                pDst[0 * dstStride] = row0[0];
                pDst[1 * dstStride] = row0[1];
                pDst[2 * dstStride] = row0[2];
                pDst[3 * dstStride] = row0[3];
                pDst[4 * dstStride] = row0[4];
                pDst[5 * dstStride] = row0[5];
                pDst[6 * dstStride] = row0[6];
                pDst[7 * dstStride] = row0[7];

                v_side = extend_high(v_temp);
                v_side -= v_side_0;
                v_side = v_side >> 1;
                row0 = ref + v_side;
                row0 = min(max(0, row0), (1 << bitDepth) - 1);
                pDst[8 * dstStride] = row0[0];
                pDst[9 * dstStride] = row0[1];
                pDst[10 * dstStride] = row0[2];
                pDst[11 * dstStride] = row0[3];
                pDst[12 * dstStride] = row0[4];
                pDst[13 * dstStride] = row0[5];
                pDst[14 * dstStride] = row0[6];
                pDst[15 * dstStride] = row0[7];
            }
        }
    }
    else if (intraPredAngle == -32)
    {
        Vec16uc v_refSide;
        v_refSide.load(refSide);
        v_refSide = permute16uc<15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0>(v_refSide);
        pixel refMain0 = refMain[0];

        v_refSide.store(refMain - 15);
        refMain[0] = refMain0;

        Vec16uc tmp;
        __m128i itmp;
//        tmp.load(refMain);        //-1,0,1,2
//        tmp.store(pDst);

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);

        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        pDst += dstStride;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        pDst += dstStride;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        pDst += dstStride;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        pDst += dstStride;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        pDst += dstStride;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        pDst += dstStride;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        pDst += dstStride;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        pDst += dstStride;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        pDst += dstStride;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        pDst += dstStride;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        pDst += dstStride;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        pDst += dstStride;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        pDst += dstStride;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        pDst += dstStride;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        pDst += dstStride;
        _mm_storeu_si128((__m128i*)pDst, itmp);

/*
        tmp.load(--refMain);
        pDst += dstStride;
        tmp.store(pDst);
        ... 14 times more
*/
        return;
    }
    else if (intraPredAngle == 32)
    {
        Vec8s tmp;
        __m128i itmp;
        refMain += 2;

//        tmp.load(refMain++);
//        tmp.store(pDst);

        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        _mm_storeu_si128((__m128i*)pDst, itmp);

/*
        tmp.load(refMain++);
        pDst += dstStride;
        tmp.store(pDst);
        ... 14 times more
*/
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        pDst += dstStride;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        pDst += dstStride;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        pDst += dstStride;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        pDst += dstStride;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        pDst += dstStride;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        pDst += dstStride;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        pDst += dstStride;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        pDst += dstStride;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        pDst += dstStride;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        pDst += dstStride;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        pDst += dstStride;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        pDst += dstStride;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        pDst += dstStride;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        pDst += dstStride;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        pDst += dstStride;
        _mm_storeu_si128((__m128i*)pDst, itmp);

        return;
    }
    else
    {
        if (modeHor)
        {
            Vec8s row11L, row12L, row11H, row12H;
            Vec8s v_deltaFract, v_deltaPos, thirty2(32), thirty1(31), v_ipAngle;
            Vec16uc tmp;
            Vec16uc R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, R13, R14, R15, R16;
            Vec16uc tmp1, tmp2;
            v_deltaPos = 0;
            v_ipAngle = intraPredAngle;
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
            Vec8s row11L, row12L, row11H, row12H;
            Vec8s v_deltaFract, v_deltaPos, thirty2(32), thirty1(31), v_ipAngle;
            Vec16uc tmp;
            Vec8s tmp1, tmp2;
            v_deltaPos = 0;
            v_ipAngle = intraPredAngle;
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
#endif /* if HIGH_BIT_DEPTH */

//32x32
#if HIGH_BIT_DEPTH
#else
#define PREDANG_CALCROW_VER(X) { \
        v_deltaPos = _mm_add_epi16(v_deltaPos, v_ipAngle); \
        v_deltaFract = _mm_and_si128(v_deltaPos, thirty1); \
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 1 + (angAP[8 - (lookIdx)][(X)]))); \
        row11L = _mm_unpacklo_epi8(itmp, _mm_setzero_si128()); \
        row11H = _mm_unpackhi_epi8(itmp, _mm_setzero_si128()); \
  \
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 1 + (angAP[8 - (lookIdx)][(X)] + 1))); \
        row12L = _mm_unpacklo_epi8(itmp, _mm_setzero_si128()); \
        row12H = _mm_unpackhi_epi8(itmp, _mm_setzero_si128()); \
  \
        it1 = _mm_sub_epi16(thirty2, v_deltaFract); \
        it2 = _mm_mullo_epi16(it1, row11L); \
        it3 = _mm_mullo_epi16(v_deltaFract, row12L); \
        it2 = _mm_add_epi16(it2, it3); \
        i16 = _mm_set1_epi16(16); \
        it2 = _mm_add_epi16(it2, i16); \
        row11L = _mm_srai_epi16(it2, 5); \
        it2 = _mm_mullo_epi16(it1, row11H); \
        it3 = _mm_mullo_epi16(v_deltaFract, row12H); \
        it2 = _mm_add_epi16(it2, it3); \
        it2 = _mm_add_epi16(it2, i16); \
        row11H = _mm_srai_epi16(it2, 5); \
  \
        itmp = _mm_packus_epi16(row11L, row11H); \
        _mm_storeu_si128((__m128i*)(pDst + ((X)*dstStride)), itmp); \
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 1 + (angAP[8 - (lookIdx)][(X)] + 16))); \
        row11L = _mm_unpacklo_epi8(itmp, _mm_setzero_si128()); \
        row11H = _mm_unpackhi_epi8(itmp, _mm_setzero_si128()); \
  \
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 1 + (angAP[8 - (lookIdx)][(X)] + 17))); \
        row12L = _mm_unpacklo_epi8(itmp, _mm_setzero_si128()); \
        row12H = _mm_unpackhi_epi8(itmp, _mm_setzero_si128()); \
  \
        it1 = _mm_sub_epi16(thirty2, v_deltaFract); \
        it2 = _mm_mullo_epi16(it1, row11L); \
        it3 = _mm_mullo_epi16(v_deltaFract, row12L); \
        it2 = _mm_add_epi16(it2, it3); \
        i16 = _mm_set1_epi16(16); \
        it2 = _mm_add_epi16(it2, i16); \
        row11L = _mm_srai_epi16(it2, 5); \
        it2 = _mm_mullo_epi16(it1, row11H); \
        it3 = _mm_mullo_epi16(v_deltaFract, row12H); \
        it2 = _mm_add_epi16(it2, it3); \
        it2 = _mm_add_epi16(it2, i16); \
        row11H = _mm_srai_epi16(it2, 5); \
  \
        itmp = _mm_packus_epi16(row11L, row11H); \
        _mm_storeu_si128((__m128i*)(pDst + ((X)*dstStride) + 16), itmp); \
}

#define PREDANG_CALCROW_VER_MODE2(X) { \
        v_deltaPos = _mm_add_epi16(v_deltaPos, v_ipAngle); \
        v_deltaFract = _mm_and_si128(v_deltaPos, thirty1); \
        it1 = _mm_sub_epi16(thirty2, v_deltaFract); \
        it2 = _mm_mullo_epi16(it1, row11); \
        it3 = _mm_mullo_epi16(v_deltaFract, row21); \
        it2 = _mm_add_epi16(it2, it3); \
        i16 = _mm_set1_epi16(16); \
        it2 = _mm_add_epi16(it2, i16); \
        res1 = _mm_srai_epi16(it2, 5); \
        it2 = _mm_mullo_epi16(it1, row12); \
        it3 = _mm_mullo_epi16(v_deltaFract, row22); \
        it2 = _mm_add_epi16(it2, it3); \
        it2 = _mm_add_epi16(it2, i16); \
        res2 = _mm_srai_epi16(it2, 5); \
  \
        itmp = _mm_packus_epi16(res1, res2); \
        _mm_storeu_si128((__m128i*)(pDst + ((X)*dstStride)), itmp); \
        it1 = _mm_sub_epi16(thirty2, v_deltaFract); \
        it2 = _mm_mullo_epi16(it1, row13); \
        it3 = _mm_mullo_epi16(v_deltaFract, row23); \
        it2 = _mm_add_epi16(it2, it3); \
        i16 = _mm_set1_epi16(16); \
        it2 = _mm_add_epi16(it2, i16); \
        res1 = _mm_srai_epi16(it2, 5); \
        it2 = _mm_mullo_epi16(it1, row14); \
        it3 = _mm_mullo_epi16(v_deltaFract, row24); \
        it2 = _mm_add_epi16(it2, it3); \
        it2 = _mm_add_epi16(it2, i16); \
        res2 = _mm_srai_epi16(it2, 5); \
  \
        itmp = _mm_packus_epi16(res1, res2); \
        _mm_storeu_si128((__m128i*)(pDst + ((X)*dstStride) + 16), itmp); \
}

#define PREDANG_CALCROW_HOR(X, rowx) { \
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 1 + (angAP[8 - (lookIdx)][((X))]))); \
        row11L = _mm_unpacklo_epi8(itmp, _mm_setzero_si128()); \
        row11H = _mm_unpackhi_epi8(itmp, _mm_setzero_si128()); \
  \
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 1 + (angAP[8 - (lookIdx)][((X))] + 1))); \
        row12L = _mm_unpacklo_epi8(itmp, _mm_setzero_si128()); \
        row12H = _mm_unpackhi_epi8(itmp, _mm_setzero_si128()); \
  \
        v_deltaPos = _mm_add_epi16(v_deltaPos, v_ipAngle); \
        v_deltaFract = _mm_and_si128(v_deltaPos, thirty1); \
        it1 = _mm_sub_epi16(thirty2, v_deltaFract); \
        it2 = _mm_mullo_epi16(it1, row11L); \
        it3 = _mm_mullo_epi16(v_deltaFract, row12L); \
        it2 = _mm_add_epi16(it2, it3); \
        i16 = _mm_set1_epi16(16); \
        it2 = _mm_add_epi16(it2, i16); \
        row11L = _mm_srai_epi16(it2, 5); \
        it2 = _mm_mullo_epi16(it1, row11H); \
        it3 = _mm_mullo_epi16(v_deltaFract, row12H); \
        it2 = _mm_add_epi16(it2, it3); \
        it2 = _mm_add_epi16(it2, i16); \
        row11H = _mm_srai_epi16(it2, 5); \
  \
        rowx = _mm_packus_epi16(row11L, row11H); \
}

#define PREDANG_CALCROW_HOR_MODE2(rowx) { \
        v_deltaPos = _mm_add_epi16(v_deltaPos, v_ipAngle); \
        v_deltaFract = _mm_and_si128(v_deltaPos, thirty1); \
        it1 = _mm_sub_epi16(thirty2, v_deltaFract); \
        it2 = _mm_mullo_epi16(it1, row11L); \
        it3 = _mm_mullo_epi16(v_deltaFract, row12L); \
        it2 = _mm_add_epi16(it2, it3); \
        i16 = _mm_set1_epi16(16); \
        it2 = _mm_add_epi16(it2, i16); \
        res1 = _mm_srai_epi16(it2, 5); \
        it2 = _mm_mullo_epi16(it1, row11H); \
        it3 = _mm_mullo_epi16(v_deltaFract, row12H); \
        it2 = _mm_add_epi16(it2, it3); \
        it2 = _mm_add_epi16(it2, i16); \
        res2 = _mm_srai_epi16(it2, 5); \
  \
        rowx = _mm_packus_epi16(res1, res2); \
}

// ROWL/H is a Vec8s variable, X is the index in of data to be loaded
#define LOADROW(ROWL, ROWH, X) { \
/*        tmp.load(refMain + 1 + (X)); \
        ROWL = extend_low(tmp); \
        ROWH = extend_high(tmp); */\
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 1 + (X))); \
        ROWL = _mm_unpacklo_epi8(itmp, _mm_setzero_si128()); \
        ROWH = _mm_unpackhi_epi8(itmp, _mm_setzero_si128()); \
}

#define BLND2_2(R1, R2) { \
/*        tmp1 = blend2q<0, 2>((Vec2q)R1, (Vec2q)R2); \
        tmp2 = blend2q<1, 3>((Vec2q)R1, (Vec2q)R2); \
        tmp1.store(pDst);   pDst += dstStride; \
        tmp2.store(pDst);   pDst += dstStride; */\
        itmp1 = _mm_unpacklo_epi64(R1, R2); \
        itmp2 = _mm_unpackhi_epi64(R1, R2); \
        _mm_storeu_si128((__m128i*)pDst, itmp1); \
        pDst += dstStride; \
        _mm_storeu_si128((__m128i*)pDst, itmp2); \
        pDst += dstStride; \
}

#define MB8(R1, R2, R3, R4, R5, R6, R7, R8) { \
        itmp1 = _mm_unpacklo_epi8(R1, R2); \
        itmp2 = _mm_unpackhi_epi8(R1, R2); \
        R1 = itmp1; \
        R2 = itmp2; \
        itmp1 = _mm_unpacklo_epi8(R3, R4); \
        itmp2 = _mm_unpackhi_epi8(R3, R4); \
        R3 = itmp1; \
        R4 = itmp2; \
        itmp1 = _mm_unpacklo_epi16(R1, R3); \
        itmp2 = _mm_unpackhi_epi16(R1, R3); \
        R1 = itmp1; \
        R3 = itmp2; \
        itmp1 = _mm_unpacklo_epi16(R2, R4); \
        itmp2 = _mm_unpackhi_epi16(R2, R4); \
        R2 = itmp1; \
        R4 = itmp2; \
        itmp1 = _mm_unpacklo_epi8(R5, R6); \
        itmp2 = _mm_unpackhi_epi8(R5, R6); \
        R5 = itmp1; \
        R6 = itmp2; \
        itmp1 = _mm_unpacklo_epi8(R7, R8); \
        itmp2 = _mm_unpackhi_epi8(R7, R8); \
        R7 = itmp1; \
        R8 = itmp2; \
        itmp1 = _mm_unpacklo_epi16(R5, R7); \
        itmp2 = _mm_unpackhi_epi16(R5, R7); \
        R5 = itmp1; \
        R7 = itmp2; \
        itmp1 = _mm_unpacklo_epi16(R6, R8); \
        itmp2 = _mm_unpackhi_epi16(R6, R8); \
        R6 = itmp1; \
        R8 = itmp2; \
        itmp1 = _mm_unpacklo_epi32(R1, R5); \
        itmp2 = _mm_unpackhi_epi32(R1, R5); \
        R1 = itmp1; \
        R5 = itmp2; \
  \
        itmp1 = _mm_unpacklo_epi32(R2, R6); \
        itmp2 = _mm_unpackhi_epi32(R2, R6); \
        R2 = itmp1; \
        R6 = itmp2; \
  \
        itmp1 = _mm_unpacklo_epi32(R3, R7); \
        itmp2 = _mm_unpackhi_epi32(R3, R7); \
        R3 = itmp1; \
        R7 = itmp2; \
  \
        itmp1 = _mm_unpacklo_epi32(R4, R8); \
        itmp2 = _mm_unpackhi_epi32(R4, R8); \
        R4 = itmp1; \
        R8 = itmp2; \
}

#define CALC_BLND_8ROWS(R1, R2, R3, R4, R5, R6, R7, R8, X) { \
        PREDANG_CALCROW_HOR(0 + X, R1) \
        PREDANG_CALCROW_HOR(1 + X, R2) \
        PREDANG_CALCROW_HOR(2 + X, R3) \
        PREDANG_CALCROW_HOR(3 + X, R4) \
        PREDANG_CALCROW_HOR(4 + X, R5) \
        PREDANG_CALCROW_HOR(5 + X, R6) \
        PREDANG_CALCROW_HOR(6 + X, R7) \
}

#define CALC_BLND_8ROWS_MODE2(R1, R2, R3, R4, R5, R6, R7, R8) { \
        PREDANG_CALCROW_HOR_MODE2(R1) \
        PREDANG_CALCROW_HOR_MODE2(R2) \
        PREDANG_CALCROW_HOR_MODE2(R3) \
        PREDANG_CALCROW_HOR_MODE2(R4) \
        PREDANG_CALCROW_HOR_MODE2(R5) \
        PREDANG_CALCROW_HOR_MODE2(R6) \
        PREDANG_CALCROW_HOR_MODE2(R7) \
}

void xPredIntraAng32x32(int /*bitDepth*/, pixel* pDst, int dstStride, int width, int dirMode, pixel *refLeft, pixel *refAbove)
{
    int k;
    int blkSize        = width;

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
            Vec16uc v_temp, tmp1;

            v_temp.load(refMain + 1);
            /*BROADSTORE16ROWS;*/
            tmp1 = permute16uc<0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0>(v_temp);
            tmp1.store(pDst + (0 * dstStride));
            tmp1.store(pDst + (0 * dstStride) + 16);
            tmp1 = permute16uc<1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1>(v_temp);
            tmp1.store(pDst + (1 * dstStride));
            tmp1.store(pDst + (1 * dstStride) + 16);
            tmp1 = permute16uc<2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2>(v_temp);
            tmp1.store(pDst + (2 * dstStride));
            tmp1.store(pDst + (2 * dstStride) + 16);
            tmp1 = permute16uc<3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3>(v_temp);
            tmp1.store(pDst + (3 * dstStride));
            tmp1.store(pDst + (3 * dstStride) + 16);
            tmp1 = permute16uc<4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4>(v_temp);
            tmp1.store(pDst + (4 * dstStride));
            tmp1.store(pDst + (4 * dstStride) + 16);
            tmp1 = permute16uc<5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5>(v_temp);
            tmp1.store(pDst + (5 * dstStride));
            tmp1.store(pDst + (5 * dstStride) + 16);
            tmp1 = permute16uc<6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6>(v_temp);
            tmp1.store(pDst + (6 * dstStride));
            tmp1.store(pDst + (6 * dstStride) + 16);
            tmp1 = permute16uc<7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7>(v_temp);
            tmp1.store(pDst + (7 * dstStride));
            tmp1.store(pDst + (7 * dstStride) + 16);
            tmp1 = permute16uc<8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8>(v_temp);
            tmp1.store(pDst + (8 * dstStride));
            tmp1.store(pDst + (8 * dstStride) + 16);
            tmp1 = permute16uc<9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9>(v_temp);
            tmp1.store(pDst + (9 * dstStride));
            tmp1.store(pDst + (9 * dstStride) + 16);
            tmp1 = permute16uc<10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10>(v_temp);
            tmp1.store(pDst + (10 * dstStride));
            tmp1.store(pDst + (10 * dstStride) + 16);
            tmp1 = permute16uc<11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11>(v_temp);
            tmp1.store(pDst + (11 * dstStride));
            tmp1.store(pDst + (11 * dstStride) + 16);
            tmp1 = permute16uc<12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12>(v_temp);
            tmp1.store(pDst + (12 * dstStride));
            tmp1.store(pDst + (12 * dstStride) + 16);
            tmp1 = permute16uc<13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13>(v_temp);
            tmp1.store(pDst + (13 * dstStride));
            tmp1.store(pDst + (13 * dstStride) + 16);
            tmp1 = permute16uc<14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14>(v_temp);
            tmp1.store(pDst + (14 * dstStride));
            tmp1.store(pDst + (14 * dstStride) + 16);
            tmp1 = permute16uc<15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15>(v_temp);
            tmp1.store(pDst + (15 * dstStride));
            tmp1.store(pDst + (15 * dstStride) + 16);

            pDst += 16 * dstStride;
            v_temp.load(refMain + 1 + 16);
            /*BROADSTORE16ROWS;*/
            tmp1 = permute16uc<0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0>(v_temp);
            tmp1.store(pDst + (0 * dstStride));
            tmp1.store(pDst + (0 * dstStride) + 16);
            tmp1 = permute16uc<1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1>(v_temp);
            tmp1.store(pDst + (1 * dstStride));
            tmp1.store(pDst + (1 * dstStride) + 16);
            tmp1 = permute16uc<2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2>(v_temp);
            tmp1.store(pDst + (2 * dstStride));
            tmp1.store(pDst + (2 * dstStride) + 16);
            tmp1 = permute16uc<3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3>(v_temp);
            tmp1.store(pDst + (3 * dstStride));
            tmp1.store(pDst + (3 * dstStride) + 16);
            tmp1 = permute16uc<4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4>(v_temp);
            tmp1.store(pDst + (4 * dstStride));
            tmp1.store(pDst + (4 * dstStride) + 16);
            tmp1 = permute16uc<5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5>(v_temp);
            tmp1.store(pDst + (5 * dstStride));
            tmp1.store(pDst + (5 * dstStride) + 16);
            tmp1 = permute16uc<6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6>(v_temp);
            tmp1.store(pDst + (6 * dstStride));
            tmp1.store(pDst + (6 * dstStride) + 16);
            tmp1 = permute16uc<7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7>(v_temp);
            tmp1.store(pDst + (7 * dstStride));
            tmp1.store(pDst + (7 * dstStride) + 16);
            tmp1 = permute16uc<8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8>(v_temp);
            tmp1.store(pDst + (8 * dstStride));
            tmp1.store(pDst + (8 * dstStride) + 16);
            tmp1 = permute16uc<9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9>(v_temp);
            tmp1.store(pDst + (9 * dstStride));
            tmp1.store(pDst + (9 * dstStride) + 16);
            tmp1 = permute16uc<10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10>(v_temp);
            tmp1.store(pDst + (10 * dstStride));
            tmp1.store(pDst + (10 * dstStride) + 16);
            tmp1 = permute16uc<11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11>(v_temp);
            tmp1.store(pDst + (11 * dstStride));
            tmp1.store(pDst + (11 * dstStride) + 16);
            tmp1 = permute16uc<12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12>(v_temp);
            tmp1.store(pDst + (12 * dstStride));
            tmp1.store(pDst + (12 * dstStride) + 16);
            tmp1 = permute16uc<13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13>(v_temp);
            tmp1.store(pDst + (13 * dstStride));
            tmp1.store(pDst + (13 * dstStride) + 16);
            tmp1 = permute16uc<14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14>(v_temp);
            tmp1.store(pDst + (14 * dstStride));
            tmp1.store(pDst + (14 * dstStride) + 16);
            tmp1 = permute16uc<15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15>(v_temp);
            tmp1.store(pDst + (15 * dstStride));
            tmp1.store(pDst + (15 * dstStride) + 16);
        }
        else
        {
            __m128i v_main;
            Pel *dstOriginal = pDst;
//            v_main.load(refMain + 1);
            v_main = _mm_loadu_si128((__m128i const*)(refMain + 1));
//            v_main.store(pDst);
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);

            pDst = dstOriginal + 16;
            v_main = _mm_loadu_si128((__m128i const*)(refMain + 17));
//            v_main.store(pDst);

            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
            pDst += dstStride;
            _mm_storeu_si128((__m128i*)(pDst), v_main);
        }
    }
    else if (intraPredAngle == -32)
    {
        Vec16uc v_refSide;
        pixel refMain0 = refMain[0];

        v_refSide.load(refSide);
        v_refSide = permute16uc<15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0>(v_refSide);
        v_refSide.store(refMain - 15);

        v_refSide.load(refSide + 16);
        v_refSide = permute16uc<15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0>(v_refSide);
        v_refSide.store(refMain - 31);

        refMain[0] = refMain0;

        __m128i itmp;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        _mm_storeu_si128((__m128i*)(pDst + (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 16));
        _mm_storeu_si128((__m128i*)(pDst + 16), itmp);
        pDst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        _mm_storeu_si128((__m128i*)(pDst + (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 16));
        _mm_storeu_si128((__m128i*)(pDst + 16), itmp);
        pDst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        _mm_storeu_si128((__m128i*)(pDst + (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 16));
        _mm_storeu_si128((__m128i*)(pDst + 16), itmp);
        pDst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        _mm_storeu_si128((__m128i*)(pDst + (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 16));
        _mm_storeu_si128((__m128i*)(pDst + 16), itmp);
        pDst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        _mm_storeu_si128((__m128i*)(pDst + (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 16));
        _mm_storeu_si128((__m128i*)(pDst + 16), itmp);
        pDst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        _mm_storeu_si128((__m128i*)(pDst + (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 16));
        _mm_storeu_si128((__m128i*)(pDst + 16), itmp);
        pDst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        _mm_storeu_si128((__m128i*)(pDst + (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 16));
        _mm_storeu_si128((__m128i*)(pDst + 16), itmp);
        pDst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        _mm_storeu_si128((__m128i*)(pDst + (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 16));
        _mm_storeu_si128((__m128i*)(pDst + 16), itmp);
        pDst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        _mm_storeu_si128((__m128i*)(pDst + (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 16));
        _mm_storeu_si128((__m128i*)(pDst + 16), itmp);
        pDst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        _mm_storeu_si128((__m128i*)(pDst + (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 16));
        _mm_storeu_si128((__m128i*)(pDst + 16), itmp);
        pDst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        _mm_storeu_si128((__m128i*)(pDst + (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 16));
        _mm_storeu_si128((__m128i*)(pDst + 16), itmp);
        pDst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        _mm_storeu_si128((__m128i*)(pDst + (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 16));
        _mm_storeu_si128((__m128i*)(pDst + 16), itmp);
        pDst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        _mm_storeu_si128((__m128i*)(pDst + (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 16));
        _mm_storeu_si128((__m128i*)(pDst + 16), itmp);
        pDst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        _mm_storeu_si128((__m128i*)(pDst + (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 16));
        _mm_storeu_si128((__m128i*)(pDst + 16), itmp);
        pDst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        _mm_storeu_si128((__m128i*)(pDst + (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 16));
        _mm_storeu_si128((__m128i*)(pDst + 16), itmp);
        pDst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        _mm_storeu_si128((__m128i*)(pDst + (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 16));
        _mm_storeu_si128((__m128i*)(pDst + 16), itmp);
        pDst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        pDst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        pDst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        pDst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        pDst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        pDst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        pDst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        pDst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        pDst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        pDst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        pDst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        pDst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        pDst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        pDst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        pDst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        pDst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);

        return;
    }
    else if (intraPredAngle == 32)
    {
        __m128i itmp;
        refMain += 2;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        pDst += dstStride;
        refMain++;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        pDst += dstStride;
        refMain++;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        pDst += dstStride;
        refMain++;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        pDst += dstStride;
        refMain++;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        pDst += dstStride;
        refMain++;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        pDst += dstStride;
        refMain++;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        pDst += dstStride;
        refMain++;

        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        pDst += dstStride;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        pDst += dstStride;
        refMain++;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        pDst += dstStride;
        refMain++;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        pDst += dstStride;
        refMain++;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        pDst += dstStride;
        refMain++;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        pDst += dstStride;
        refMain++;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        pDst += dstStride;
        refMain++;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        pDst += dstStride;
        refMain++;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)pDst, itmp);
        pDst += dstStride;
        refMain++;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        refMain++;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        _mm_storeu_si128((__m128i*)(pDst - (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 15));
        _mm_storeu_si128((__m128i*)(pDst + 16), itmp);
        pDst += dstStride;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        refMain++;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        _mm_storeu_si128((__m128i*)(pDst - (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 15));
        _mm_storeu_si128((__m128i*)(pDst + 16), itmp);
        pDst += dstStride;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        refMain++;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        _mm_storeu_si128((__m128i*)(pDst - (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 15));
        _mm_storeu_si128((__m128i*)(pDst + 16), itmp);
        pDst += dstStride;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        refMain++;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        _mm_storeu_si128((__m128i*)(pDst - (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 15));
        _mm_storeu_si128((__m128i*)(pDst + 16), itmp);
        pDst += dstStride;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        refMain++;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        _mm_storeu_si128((__m128i*)(pDst - (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 15));
        _mm_storeu_si128((__m128i*)(pDst + 16), itmp);
        pDst += dstStride;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        refMain++;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        _mm_storeu_si128((__m128i*)(pDst - (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 15));
        _mm_storeu_si128((__m128i*)(pDst + 16), itmp);
        pDst += dstStride;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        refMain++;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        _mm_storeu_si128((__m128i*)(pDst - (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 15));
        _mm_storeu_si128((__m128i*)(pDst + 16), itmp);
        pDst += dstStride;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        refMain++;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        _mm_storeu_si128((__m128i*)(pDst - (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 15));
        _mm_storeu_si128((__m128i*)(pDst + 16), itmp);
        pDst += dstStride;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        refMain++;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        _mm_storeu_si128((__m128i*)(pDst - (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 15));
        _mm_storeu_si128((__m128i*)(pDst + 16), itmp);
        pDst += dstStride;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        refMain++;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        _mm_storeu_si128((__m128i*)(pDst - (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 15));
        _mm_storeu_si128((__m128i*)(pDst + 16), itmp);
        pDst += dstStride;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        refMain++;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        _mm_storeu_si128((__m128i*)(pDst - (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 15));
        _mm_storeu_si128((__m128i*)(pDst + 16), itmp);
        pDst += dstStride;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        refMain++;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        _mm_storeu_si128((__m128i*)(pDst - (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 15));
        _mm_storeu_si128((__m128i*)(pDst + 16), itmp);
        pDst += dstStride;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        refMain++;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        _mm_storeu_si128((__m128i*)(pDst - (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 15));
        _mm_storeu_si128((__m128i*)(pDst + 16), itmp);
        pDst += dstStride;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        refMain++;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        _mm_storeu_si128((__m128i*)(pDst - (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 15));
        _mm_storeu_si128((__m128i*)(pDst + 16), itmp);
        pDst += dstStride;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        refMain++;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        _mm_storeu_si128((__m128i*)(pDst - (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 15));
        _mm_storeu_si128((__m128i*)(pDst + 16), itmp);
        pDst += dstStride;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        refMain++;
        _mm_storeu_si128((__m128i*)pDst, itmp);
        _mm_storeu_si128((__m128i*)(pDst - (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 15));
        _mm_storeu_si128((__m128i*)(pDst + 16), itmp);
        pDst += dstStride;

        return;
    }
    else
    {
        if (modeHor)
        {
            __m128i row11L, row12L, row11H, row12H, res1, res2;
            __m128i v_deltaFract, v_deltaPos, thirty2, thirty1, v_ipAngle;
            __m128i R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, R13, R14, R15, R16;

            Pel * original_pDst = pDst;
            v_deltaPos = _mm_setzero_si128(); //v_deltaPos = 0;
            v_ipAngle = _mm_set1_epi16(intraPredAngle);
            thirty2 = _mm_set1_epi16(32);
            thirty1 = _mm_set1_epi16(31);
            __m128i itmp, itmp1, itmp2, it1, it2, it3, i16;

            switch (intraPredAngle)
            {
            case -2:
                LOADROW(row11L, row11H, -1)
                LOADROW(row12L, row12H,  0)
                R16 = _mm_packus_epi16(row11L, row11H); //R16 = compress(row11L, row11H);

                CALC_BLND_8ROWS_MODE2(R1, R2, R3, R4, R5, R6, R7, R8)
                PREDANG_CALCROW_HOR_MODE2(R8)
                MB8(R1, R2, R3, R4, R5, R6, R7, R8)
                CALC_BLND_8ROWS_MODE2(R9, R10, R11, R12, R13, R14, R15, R16)
                MB8(R9, R10, R11, R12, R13, R14, R15, R16)
                BLND2_2(R1, R9)
                BLND2_2(R5, R13)
                BLND2_2(R3, R11)
                BLND2_2(R7, R15)
                BLND2_2(R2, R10)
                BLND2_2(R6, R14)
                BLND2_2(R4, R12)
                BLND2_2(R8, R16)

                v_deltaPos = _mm_add_epi16(v_deltaPos, v_ipAngle);
                v_deltaFract = _mm_and_si128(v_deltaPos, thirty1);
                row12L = row11L;
                row12H = row11H;
                LOADROW(row11L, row11H, -2)
                R16 = _mm_packus_epi16(row11L, row11H);
                pDst = original_pDst + 16;

                CALC_BLND_8ROWS_MODE2(R1, R2, R3, R4, R5, R6, R7, R8)
                PREDANG_CALCROW_HOR_MODE2(R8)
                MB8(R1, R2, R3, R4, R5, R6, R7, R8)
                CALC_BLND_8ROWS_MODE2(R9, R10, R11, R12, R13, R14, R15, R16)
                MB8(R9, R10, R11, R12, R13, R14, R15, R16)
                BLND2_2(R1, R9)
                BLND2_2(R5, R13)
                BLND2_2(R3, R11)
                BLND2_2(R7, R15)
                BLND2_2(R2, R10)
                BLND2_2(R6, R14)
                BLND2_2(R4, R12)
                BLND2_2(R8, R16)

                pDst = original_pDst + (16 * dstStride);
                refMain += 16;

                v_deltaPos = _mm_setzero_si128();
                v_ipAngle = _mm_set1_epi16(intraPredAngle);
                LOADROW(row11L, row11H, -1)
                LOADROW(row12L, row12H,  0)
                R16 = _mm_packus_epi16(row11L, row11H);

                CALC_BLND_8ROWS_MODE2(R1, R2, R3, R4, R5, R6, R7, R8)
                PREDANG_CALCROW_HOR_MODE2(R8)
                MB8(R1, R2, R3, R4, R5, R6, R7, R8)
                CALC_BLND_8ROWS_MODE2(R9, R10, R11, R12, R13, R14, R15, R16)
                MB8(R9, R10, R11, R12, R13, R14, R15, R16)
                BLND2_2(R1, R9)
                BLND2_2(R5, R13)
                BLND2_2(R3, R11)
                BLND2_2(R7, R15)
                BLND2_2(R2, R10)
                BLND2_2(R6, R14)
                BLND2_2(R4, R12)
                BLND2_2(R8, R16)

                v_deltaPos = _mm_add_epi16(v_deltaPos, v_ipAngle);
                v_deltaFract = _mm_and_si128(v_deltaPos, thirty1);
                row12L = row11L;
                row12H = row11H;
                LOADROW(row11L, row11H, -2)
                R16 = _mm_packus_epi16(row11L, row11H);
                pDst = original_pDst + (16 * dstStride) + 16;

                CALC_BLND_8ROWS_MODE2(R1, R2, R3, R4, R5, R6, R7, R8)
                PREDANG_CALCROW_HOR_MODE2(R8)
                MB8(R1, R2, R3, R4, R5, R6, R7, R8)
                CALC_BLND_8ROWS_MODE2(R9, R10, R11, R12, R13, R14, R15, R16)
                MB8(R9, R10, R11, R12, R13, R14, R15, R16)
                BLND2_2(R1, R9)
                BLND2_2(R5, R13)
                BLND2_2(R3, R11)
                BLND2_2(R7, R15)
                BLND2_2(R2, R10)
                BLND2_2(R6, R14)
                BLND2_2(R4, R12)
                BLND2_2(R8, R16)
                return;

            case  2:
                LOADROW(row11L, row11H, 0)
                LOADROW(row12L, row12H, 1)
                R16 = _mm_packus_epi16(row12L, row12H);

                CALC_BLND_8ROWS_MODE2(R1, R2, R3, R4, R5, R6, R7, R8)
                PREDANG_CALCROW_HOR_MODE2(R8)
                MB8(R1, R2, R3, R4, R5, R6, R7, R8)
                CALC_BLND_8ROWS_MODE2(R9, R10, R11, R12, R13, R14, R15, R16)
                MB8(R9, R10, R11, R12, R13, R14, R15, R16)
                BLND2_2(R1, R9)
                BLND2_2(R5, R13)
                BLND2_2(R3, R11)
                BLND2_2(R7, R15)
                BLND2_2(R2, R10)
                BLND2_2(R6, R14)
                BLND2_2(R4, R12)
                BLND2_2(R8, R16)

                v_deltaPos = _mm_add_epi16(v_deltaPos, v_ipAngle);
                v_deltaFract = _mm_and_si128(v_deltaPos, thirty1);
                row11L = row12L;
                row11H = row12H;
                LOADROW(row12L, row12H, 2)
                R16 = _mm_packus_epi16(row12L, row12H);
                pDst = original_pDst + 16;

                CALC_BLND_8ROWS_MODE2(R1, R2, R3, R4, R5, R6, R7, R8)
                PREDANG_CALCROW_HOR_MODE2(R8)
                MB8(R1, R2, R3, R4, R5, R6, R7, R8)
                CALC_BLND_8ROWS_MODE2(R9, R10, R11, R12, R13, R14, R15, R16)
                MB8(R9, R10, R11, R12, R13, R14, R15, R16)
                BLND2_2(R1, R9)
                BLND2_2(R5, R13)
                BLND2_2(R3, R11)
                BLND2_2(R7, R15)
                BLND2_2(R2, R10)
                BLND2_2(R6, R14)
                BLND2_2(R4, R12)
                BLND2_2(R8, R16)

                pDst = original_pDst + (16 * dstStride);
                refMain += 16;
                v_deltaPos = _mm_setzero_si128();

                v_ipAngle = _mm_set1_epi16(intraPredAngle);
                LOADROW(row11L, row11H, 0)
                LOADROW(row12L, row12H, 1)
                R16 = _mm_packus_epi16(row12L, row12H);

                CALC_BLND_8ROWS_MODE2(R1, R2, R3, R4, R5, R6, R7, R8)
                PREDANG_CALCROW_HOR_MODE2(R8)
                MB8(R1, R2, R3, R4, R5, R6, R7, R8)
                CALC_BLND_8ROWS_MODE2(R9, R10, R11, R12, R13, R14, R15, R16)
                MB8(R9, R10, R11, R12, R13, R14, R15, R16)
                BLND2_2(R1, R9)
                BLND2_2(R5, R13)
                BLND2_2(R3, R11)
                BLND2_2(R7, R15)
                BLND2_2(R2, R10)
                BLND2_2(R6, R14)
                BLND2_2(R4, R12)
                BLND2_2(R8, R16)

                v_deltaPos = _mm_add_epi16(v_deltaPos, v_ipAngle);
                v_deltaFract = _mm_and_si128(v_deltaPos, thirty1);
                row11L = row12L;
                row11H = row12H;
                LOADROW(row12L, row12H, 2)
                R16 = _mm_packus_epi16(row12L, row12H);
                pDst = original_pDst + (16 * dstStride) + 16;

                CALC_BLND_8ROWS_MODE2(R1, R2, R3, R4, R5, R6, R7, R8)
                PREDANG_CALCROW_HOR_MODE2(R8)
                MB8(R1, R2, R3, R4, R5, R6, R7, R8)
                CALC_BLND_8ROWS_MODE2(R9, R10, R11, R12, R13, R14, R15, R16)
                MB8(R9, R10, R11, R12, R13, R14, R15, R16)
                BLND2_2(R1, R9)
                BLND2_2(R5, R13)
                BLND2_2(R3, R11)
                BLND2_2(R7, R15)
                BLND2_2(R2, R10)
                BLND2_2(R6, R14)
                BLND2_2(R4, R12)
                BLND2_2(R8, R16)
                return;
            }

            CALC_BLND_8ROWS(R1, R2, R3, R4, R5, R6, R7, R8, 0)
            PREDANG_CALCROW_HOR(7 + 0, R8)
            MB8(R1, R2, R3, R4, R5, R6, R7, R8)
            CALC_BLND_8ROWS(R9, R10, R11, R12, R13, R14, R15, R16, 8)
            PREDANG_CALCROW_HOR(7 + 8, R16)
            MB8(R9, R10, R11, R12, R13, R14, R15, R16)
            BLND2_2(R1, R9)
            BLND2_2(R5, R13)
            BLND2_2(R3, R11)
            BLND2_2(R7, R15)
            BLND2_2(R2, R10)
            BLND2_2(R6, R14)
            BLND2_2(R4, R12)
            BLND2_2(R8, R16)

            pDst = original_pDst + 16;

            CALC_BLND_8ROWS(R1, R2, R3, R4, R5, R6, R7, R8, 16)
            PREDANG_CALCROW_HOR(7 + 16, R8)
            MB8(R1, R2, R3, R4, R5, R6, R7, R8)
            CALC_BLND_8ROWS(R9, R10, R11, R12, R13, R14, R15, R16, 24)
            R16 = _mm_loadu_si128((__m128i const*)(refMain + 1 + GETAP(lookIdx, 31)));
            MB8(R9, R10, R11, R12, R13, R14, R15, R16)
            BLND2_2(R1, R9)
            BLND2_2(R5, R13)
            BLND2_2(R3, R11)
            BLND2_2(R7, R15)
            BLND2_2(R2, R10)
            BLND2_2(R6, R14)
            BLND2_2(R4, R12)
            BLND2_2(R8, R16)

            pDst = original_pDst + (16 * dstStride);
            refMain += 16;
            v_deltaPos = _mm_setzero_si128();
            v_ipAngle = _mm_set1_epi16(intraPredAngle);

            CALC_BLND_8ROWS(R1, R2, R3, R4, R5, R6, R7, R8, 0)
            PREDANG_CALCROW_HOR(7 + 0, R8)
            MB8(R1, R2, R3, R4, R5, R6, R7, R8)
            CALC_BLND_8ROWS(R9, R10, R11, R12, R13, R14, R15, R16, 8)
            PREDANG_CALCROW_HOR(7 + 8, R16)
            MB8(R9, R10, R11, R12, R13, R14, R15, R16)
            BLND2_2(R1, R9)
            BLND2_2(R5, R13)
            BLND2_2(R3, R11)
            BLND2_2(R7, R15)
            BLND2_2(R2, R10)
            BLND2_2(R6, R14)
            BLND2_2(R4, R12)
            BLND2_2(R8, R16)
            pDst = original_pDst + (16 * dstStride) + 16;

            CALC_BLND_8ROWS(R1, R2, R3, R4, R5, R6, R7, R8, 16)
            PREDANG_CALCROW_HOR(7 + 16, R8)
            MB8(R1, R2, R3, R4, R5, R6, R7, R8)
            CALC_BLND_8ROWS(R9, R10, R11, R12, R13, R14, R15, R16, 24)
            R16 = _mm_loadu_si128((__m128i const*)(refMain + 1 + GETAP(lookIdx, 31)));
            MB8(R9, R10, R11, R12, R13, R14, R15, R16)
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
            __m128i v_deltaFract, v_deltaPos, thirty2, thirty1, v_ipAngle;
            __m128i row11, row12, row13, row14, row21, row22, row23, row24;
            __m128i res1, res2;

            v_deltaPos = _mm_setzero_si128(); //v_deltaPos = 0;
            v_ipAngle = _mm_set1_epi16(intraPredAngle);
            thirty2 = _mm_set1_epi16(32);
            thirty1 = _mm_set1_epi16(31);
            __m128i itmp, it1, it2, it3, i16;

            switch (intraPredAngle)
            {
            case -2:
                LOADROW(row11, row12, -1)
                LOADROW(row21, row22,  0)
                LOADROW(row13, row14, 15)
                LOADROW(row23, row24, 16)
                for (int i = 0; i <= 14; i++)
                {
                    PREDANG_CALCROW_VER_MODE2(i);
                }

                //deltaFract == 0 for 16th row
                v_deltaPos = _mm_add_epi16(v_deltaPos, v_ipAngle);
                v_deltaFract = _mm_and_si128(v_deltaPos, thirty1);
                itmp = _mm_packus_epi16(row11, row12);
                _mm_storeu_si128((__m128i*)(pDst + ((15) * dstStride)), itmp);
                itmp = _mm_packus_epi16(row13, row14);
                _mm_storeu_si128((__m128i*)(pDst + ((15) * dstStride) + 16), itmp);

                row21 = row11;
                row22 = row12;
                row23 = row13;
                row24 = row14;

                LOADROW(row11, row12, -2)
                LOADROW(row13, row14, 14)
                for (int i = 16; i <= 30; i++)
                {
                    PREDANG_CALCROW_VER_MODE2(i);
                }

                itmp = _mm_packus_epi16(row11, row12);
                _mm_storeu_si128((__m128i*)(pDst + ((31) * dstStride)), itmp);
                itmp = _mm_packus_epi16(row13, row14);
                _mm_storeu_si128((__m128i*)(pDst + ((31) * dstStride) + 16), itmp);

                return;

            case  2:

                LOADROW(row11, row12, 0)
                LOADROW(row21, row22, 1)
                LOADROW(row13, row14, 16)
                LOADROW(row23, row24, 17)
                for (int i = 0; i <= 14; i++)
                {
                    PREDANG_CALCROW_VER_MODE2(i);
                }

                //deltaFract == 0 for 16th row

                v_deltaPos = _mm_add_epi16(v_deltaPos, v_ipAngle);
                v_deltaFract = _mm_and_si128(v_deltaPos, thirty1);
                itmp = _mm_packus_epi16(row21, row22);
                _mm_storeu_si128((__m128i*)(pDst + ((15) * dstStride)), itmp);
                itmp = _mm_packus_epi16(row23, row24);
                _mm_storeu_si128((__m128i*)(pDst + ((15) * dstStride) + 16), itmp);

                row11 = row21;
                row12 = row22;
                row13 = row23;
                row14 = row24;

                LOADROW(row21, row22, 2)
                LOADROW(row23, row24, 18)
                for (int i = 16; i <= 30; i++)
                {
                    PREDANG_CALCROW_VER_MODE2(i);
                }

                itmp = _mm_packus_epi16(row21, row22);
                _mm_storeu_si128((__m128i*)(pDst + ((31) * dstStride)), itmp);
                itmp = _mm_packus_epi16(row23, row24);
                _mm_storeu_si128((__m128i*)(pDst + ((31) * dstStride) + 16), itmp);

                return;
            }

            for (int i = 0; i <= 30; i++)
            {
                PREDANG_CALCROW_VER(i);
            }

            itmp = _mm_loadu_si128((__m128i const*)(refMain + 1 + GETAP(lookIdx, 31)));
            _mm_storeu_si128((__m128i*)(pDst + ((31) * dstStride)), itmp);
            itmp = _mm_loadu_si128((__m128i const*)(refMain + 17 + GETAP(lookIdx, 31)));
            _mm_storeu_si128((__m128i*)(pDst + ((31) * dstStride) + 16), itmp);
        }
    }
}

#endif /* if HIGH_BIT_DEPTH */

void xPredIntraAngBufRef(int bitDepth, pixel* pDst, int dstStride, int width, int dirMode, bool bFilter, pixel *refLeft, pixel *refAbove)
{
#if HIGH_BIT_DEPTH
#else
    switch (width)
    {
    case 4:
        xPredIntraAng4x4(bitDepth, pDst, dstStride, width, dirMode, refLeft, refAbove, bFilter);
        return;
    case 8:
        xPredIntraAng8x8(bitDepth, pDst, dstStride, width, dirMode, refLeft, refAbove, bFilter);
        return;
    case 16:
        xPredIntraAng16x16(bitDepth, pDst, dstStride, width, dirMode, refLeft, refAbove, bFilter);
        return;
    case 32:
        xPredIntraAng32x32(bitDepth, pDst, dstStride, width, dirMode, refLeft, refAbove);
        return;
    }

#endif /* if HIGH_BIT_DEPTH */

    int k, l;
    int blkSize        = width;

    // Map the mode index to main prediction direction and angle
    assert(dirMode > 1); //no planar and dc
    bool modeHor       = (dirMode < 18);
    bool modeVer       = !modeHor;
    int intraPredAngle = modeVer ? (int)dirMode - VER_IDX : modeHor ? -((int)dirMode - HOR_IDX) : 0;
    int absAng         = abs(intraPredAngle);
    int signAng        = intraPredAngle < 0 ? -1 : 1;

    // Set bitshifts and scale the angle parameter to block size
    int angTable[9]    = { 0,    2,    5,   9,  13,  17,  21,  26,  32 };
    int invAngTable[9] = { 0, 4096, 1638, 910, 630, 482, 390, 315, 256 }; // (256 * 32) / Angle
    int invAngle       = invAngTable[absAng];
    absAng             = angTable[absAng];
    intraPredAngle     = signAng * absAng;

    // Do angular predictions
    {
        pixel* refMain;
        pixel* refSide;

        // Initialise the Main and Left reference array.
        if (intraPredAngle < 0)
        {
            refMain = (modeVer ? refAbove : refLeft); // + (blkSize - 1);
            refSide = (modeVer ? refLeft : refAbove); // + (blkSize - 1);

            // Extend the Main reference to the left.
            int invAngleSum    = 128; // rounding for (shift by 8)
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

        if (intraPredAngle == 0)
        {
            for (k = 0; k < blkSize; k++)
            {
                for (l = 0; l < blkSize; l++)
                {
                    pDst[k * dstStride + l] = refMain[l + 1];
                }
            }

            if (bFilter)
            {
                for (k = 0; k < blkSize; k++)
                {
                    pDst[k * dstStride] = (pixel)Clip3(0, (1 << bitDepth) - 1, static_cast<short>(pDst[k * dstStride]) + ((refSide[k + 1] - refSide[0]) >> 1));
                }
            }
        }
        else
        {
            int deltaPos = 0;
            int deltaInt;
            int deltaFract;
            int refMainIndex;

            for (k = 0; k < blkSize; k++)
            {
                deltaPos += intraPredAngle;
                deltaInt   = deltaPos >> 5;
                deltaFract = deltaPos & (32 - 1);

                if (deltaFract)
                {
                    // Do linear filtering
                    for (l = 0; l < blkSize; l++)
                    {
                        refMainIndex        = l + deltaInt + 1;
                        pDst[k * dstStride + l] = (pixel)(((32 - deltaFract) * refMain[refMainIndex] + deltaFract * refMain[refMainIndex + 1] + 16) >> 5);
                    }
                }
                else
                {
                    // Just copy the integer samples
                    for (l = 0; l < blkSize; l++)
                    {
                        pDst[k * dstStride + l] = refMain[l + deltaInt + 1];
                    }
                }
            }
        }

        // Flip the block if this is the horizontal mode
        if (modeHor)
        {
            pixel  tmp;
            for (k = 0; k < blkSize - 1; k++)
            {
                for (l = k + 1; l < blkSize; l++)
                {
                    tmp                 = pDst[k * dstStride + l];
                    pDst[k * dstStride + l] = pDst[l * dstStride + k];
                    pDst[l * dstStride + k] = tmp;
                }
            }
        }
    }
}

// TODO: reference code, please optimize it
template<int size>
void xPredIntraAngs(pixel *pDst0, pixel *pAbove0, pixel *pLeft0, pixel *pAbove1, pixel *pLeft1, bool bLuma)
{
    int iMode;

    for( iMode = 2; iMode <= 34; iMode++ )
    {
        pixel *pLeft = (IntraFilterType[(int)g_aucConvertToBit[size]][iMode] ? pLeft1 : pLeft0);
        pixel *pAbove = (IntraFilterType[(int)g_aucConvertToBit[size]][iMode] ? pAbove1 : pAbove0);
        pixel *pDst = pDst0 + (iMode-2) * (size * size);

        xPredIntraAngBufRef(8, pDst, size, size, iMode, bLuma, pLeft, pAbove);

        // Optimize code don't flip buffer
        bool modeHor = (iMode < 18);
        // Flip the block if this is the horizontal mode
        if (modeHor)
        {
            pixel  tmp;
            for (int k = 0; k < size - 1; k++)
            {
                for (int l = k + 1; l < size; l++)
                {
                    tmp                = pDst[k * size + l];
                    pDst[k * size + l] = pDst[l * size + k];
                    pDst[l * size + k] = tmp;
                }
            }
        }
    }
}

ALIGN_VAR_32(static const unsigned char, tab_angle_0[][16]) =
{
    { 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8 },         //  0
    { 15, 0, 0, 1, 2, 3, 4, 5, 7, 0, 0, 9, 10, 11, 12, 13 },    //  1
    { 12, 0, 0, 1, 2, 3, 4, 5, 3, 0, 0, 9, 10, 11, 12, 13 },    //  2
    { 15, 11, 12, 0, 0, 1, 2, 3, 7, 3, 4, 0, 0, 9, 10, 11 },    //  3
    { 13, 12, 11, 8, 8, 1, 2, 3, 5, 4, 3, 0, 0, 9, 10, 11 },    //  4
    { 9, 0, 0, 1, 2, 3, 4, 5, 1, 0, 0, 9, 10, 11, 12, 13 },     //  5
    { 11, 10, 9, 0, 0, 1, 2, 3, 4, 2, 1, 0, 0, 9, 10, 11 },     //  6
    { 15, 12, 11, 10, 9, 0, 0, 1, 7, 4, 3, 2, 1, 0, 0, 9 },     //  7
    { 0, 10, 11, 13, 1, 0, 10, 11, 3, 2, 0, 10, 5, 4, 2, 0},    //  8

    { 1, 2, 2, 3, 3, 4, 4,  5,  5,  6,  6,  7,  7,  8,  8,  9 },    //  9
    { 2, 3, 3, 4, 4, 5, 5,  6,  6,  7,  7,  8,  8,  9,  9, 10 },    // 10
    { 3, 4, 4, 5, 5, 6, 6,  7,  7,  8,  8,  9,  9, 10, 10, 11 },    // 11
    { 4, 5, 5, 6, 6, 7, 7,  8,  8,  9,  9, 10, 10, 11, 11, 12 },    // 12
    { 5, 6, 6, 7, 7, 8, 8,  9,  9, 10, 10, 11, 11, 12, 12, 13 },    // 13
    { 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14 },    // 14
    { 9, 0, 0, 1, 1, 2, 2,  3,  3,  4,  4,  5,  5,  6,  6,  7 },    // 15
    { 2, 0, 0, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14 },    // 16
    { 11, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7 },            // 17
    { 4, 0, 0, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14 },    // 18
    { 14, 11, 11, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6},           // 19
    { 7, 4, 4, 0, 0, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13 },      // 20
    { 13, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7 },            // 21
    { 6, 0, 0, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14 },    // 22
    { 12, 9, 9, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6 },            // 23
    { 5, 2, 2, 0, 0, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13 },      // 24
    { 14, 12, 12, 9, 9, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5 },          // 25
    { 7, 5, 5, 2, 2, 0, 0, 8, 8, 9, 9, 10, 10, 11, 11, 12 },        // 26
    { 11, 9, 9, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6 },            // 27
    { 4, 2, 2, 0, 0, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13 },      // 28
    { 13, 11, 11, 9, 9, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5 },          // 29
    { 6, 4, 4, 2, 2, 0, 0, 8, 8, 9, 9, 10, 10, 11, 11, 12 },        // 30
    { 15, 13, 13, 11, 11, 9, 9, 0, 0, 1, 1, 2, 2, 3, 3, 4 },        // 31
    { 10, 9, 9, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6 },            // 32
    { 3, 2, 2, 0, 0, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13 },      // 33
    { 12, 10, 10, 9, 9, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5 },          // 34
    { 5, 3, 3, 2, 2, 0, 0, 8, 8, 9, 9, 10, 10, 11, 11, 12 },        // 35
    { 13, 12, 12, 10, 10, 9, 9, 0, 0, 1, 1, 2, 2, 3, 3, 4 },        // 36
    { 6, 5, 5, 3, 3, 2, 2, 0, 0, 8, 8, 9, 9, 10, 10, 11 },          // 37
    { 15, 13, 13, 12, 12, 10, 10, 9, 9, 0, 0, 1, 1, 2, 2, 3 },      // 38
    { 0, 7, 6, 5, 4, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },             // 39
    { 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 },       // 40

    { 7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8 },       // 41
};

// TODO: Remove unused table and merge here
ALIGN_VAR_32(static const unsigned char, tab_angle_2[][16]) =
{
    { 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 },       //  0
};

ALIGN_VAR_32(static const char, tab_angle_1[][16]) =
{
#define MAKE_COEF8(a) \
    { 32-(a), (a), 32-(a), (a), 32-(a), (a), 32-(a), (a), 32-(a), (a), 32-(a), (a), 32-(a), (a), 32-(a), (a) },

    MAKE_COEF8(  0 )
    MAKE_COEF8(  1 )
    MAKE_COEF8(  2 )
    MAKE_COEF8(  3 )
    MAKE_COEF8(  4 )
    MAKE_COEF8(  5 )
    MAKE_COEF8(  6 )
    MAKE_COEF8(  7 )
    MAKE_COEF8(  8 )
    MAKE_COEF8(  9 )
    MAKE_COEF8( 10 )
    MAKE_COEF8( 11 )
    MAKE_COEF8( 12 )
    MAKE_COEF8( 13 )
    MAKE_COEF8( 14 )
    MAKE_COEF8( 15 )
    MAKE_COEF8( 16 )
    MAKE_COEF8( 17 )
    MAKE_COEF8( 18 )
    MAKE_COEF8( 19 )
    MAKE_COEF8( 20 )
    MAKE_COEF8( 21 )
    MAKE_COEF8( 22 )
    MAKE_COEF8( 23 )
    MAKE_COEF8( 24 )
    MAKE_COEF8( 25 )
    MAKE_COEF8( 26 )
    MAKE_COEF8( 27 )
    MAKE_COEF8( 28 )
    MAKE_COEF8( 29 )
    MAKE_COEF8( 30 )
    MAKE_COEF8( 31 )

#undef MAKE_COEF8
};

void predIntraAngs4(pixel *dst, pixel *above0, pixel *left0, pixel *above1, pixel *left1, bool filter)
{
    // avoid warning
    (void)left1;
    (void)above1;

    pixel (*dstN)[4*4] = (pixel(*)[4*4])dst;

    __m128i T00, T01, T02, T03, T04, T05, T06, T07;
    __m128i T10, T11, T12, T13;
    __m128i T20, T21, T22, T23;
    __m128i T30, T31, T32;
    __m128i R00, R10, R20, R30;
    __m128i R01, R11, R21, R31;

    R00 = _mm_loadu_si128((__m128i*)(left0+1));    // [-- -- -- -- -- -- --  -- -08 -07 -06 -05 -04 -03 -02 -01]
    R10 = _mm_srli_si128(R00, 1);                   // [-- -- -- -- -- -- --  --  -- -08 -07 -06 -05 -04 -03 -02]
    R20 = _mm_srli_si128(R00, 2);                   // [-- -- -- -- -- -- --  --  --  -- -08 -07 -06 -05 -04 -03]
    R30 = _mm_srli_si128(R00, 3);                   // [-- -- -- -- -- -- --  --  --  --  -- -08 -07 -06 -05 -04]

    R01 = _mm_loadu_si128((__m128i*)(above0+1));   // [-- -- -- -- -- -- --  --  08  07  06  05  04  03  02  01]
    R11 = _mm_srli_si128(R01, 1);                   // [-- -- -- -- -- -- --  --  --  08  07  06  05  04  03  02]
    R21 = _mm_srli_si128(R01, 2);                   // [-- -- -- -- -- -- --  --  --  --  08  07  06  05  04  03]
    R31 = _mm_srli_si128(R01, 3);                   // [-- -- -- -- -- -- --  --  --  --  --  08  07  06  05  04]

    T00 = _mm_unpacklo_epi32(R00, R00);
    T00 = _mm_unpacklo_epi64(T00, T00);
    _mm_store_si128((__m128i*)dstN[ 8], T00);

    T00 = _mm_unpacklo_epi32(R01, R01);
    T00 = _mm_unpacklo_epi64(T00, T00);
    _mm_store_si128((__m128i*)dstN[24], T00);

    if (filter)
    {
        __m128i roundH, roundV;
        __m128i pL = _mm_set1_epi16(left0[1]);
        __m128i pT = _mm_set1_epi16(above0[1]);
        roundH = _mm_set1_epi16(above0[0]);
        roundV = roundH;

        roundH = _mm_srai_epi16(_mm_sub_epi16(_mm_unpacklo_epi8(R01, _mm_setzero_si128()), roundH), 1);
        roundV = _mm_srai_epi16(_mm_sub_epi16(_mm_unpacklo_epi8(R00, _mm_setzero_si128()), roundV), 1);

        T00 = _mm_add_epi16(roundH, pL);
        T00 = _mm_packus_epi16(T00, T00);
        T01 = _mm_add_epi16(roundV, pT);
        T01 = _mm_packus_epi16(T01, T01);

        int tmp0;
        tmp0 = _mm_cvtsi128_si32(T00);
        dstN[ 8][0*4] = tmp0 & 0xFF;
        dstN[ 8][1*4] = (tmp0>>8) & 0xFF;
        dstN[ 8][2*4] = (tmp0>>16) & 0xFF;
        dstN[ 8][3*4] = (tmp0>>24) & 0xFF;

        tmp0 = _mm_cvtsi128_si32(T01);
        dstN[24][0*4] = tmp0 & 0xFF;
        dstN[24][1*4] = (tmp0>>8) & 0xFF;
        dstN[24][2*4] = (tmp0>>16) & 0xFF;
        dstN[24][3*4] = (tmp0>>24) & 0xFF;
    }

    const __m128i c_16 = _mm_set1_epi16(16);

    T00 = _mm_shufflelo_epi16(R10, 0x94);
    T01 = _mm_shufflelo_epi16(R20, 0x94);
    T00 = _mm_unpacklo_epi32(T00, T01);
    _mm_store_si128((__m128i*)dstN[ 0], T00);

    T00 = _mm_shufflelo_epi16(R11, 0x94);
    T01 = _mm_shufflelo_epi16(R21, 0x94);
    T00 = _mm_unpacklo_epi32(T00, T01);
    _mm_store_si128((__m128i*)dstN[32], T00);

    T00 = _mm_shuffle_epi8( R00, _mm_load_si128((__m128i*)tab_angle_0[ 0]));
    T01 = _mm_shuffle_epi8( R10, _mm_load_si128((__m128i*)tab_angle_0[ 0]));
    T02 = _mm_shuffle_epi8( R20, _mm_load_si128((__m128i*)tab_angle_0[ 0]));
    T03 = _mm_shuffle_epi8( R30, _mm_load_si128((__m128i*)tab_angle_0[ 0]));
    T04 = _mm_shuffle_epi8( R01, _mm_load_si128((__m128i*)tab_angle_0[ 0]));
    T05 = _mm_shuffle_epi8( R11, _mm_load_si128((__m128i*)tab_angle_0[ 0]));
    T06 = _mm_shuffle_epi8( R21, _mm_load_si128((__m128i*)tab_angle_0[ 0]));
    T07 = _mm_shuffle_epi8( R31, _mm_load_si128((__m128i*)tab_angle_0[ 0]));
    T00 = _mm_unpacklo_epi64(T00, T04);
    T01 = _mm_unpacklo_epi64(T01, T05);
    T02 = _mm_unpacklo_epi64(T02, T06);
    T03 = _mm_unpacklo_epi64(T03, T07);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T13 = _mm_maddubs_epi16(T03, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T20 = _mm_unpacklo_epi64(T10, T11);
    T21 = _mm_unpacklo_epi64(T12, T13);
    T20 = _mm_srai_epi16(_mm_add_epi16(T20, c_16), 5);
    T21 = _mm_srai_epi16(_mm_add_epi16(T21, c_16), 5);
    T20 = _mm_packus_epi16(T20, T21);
    _mm_store_si128((__m128i*)dstN[ 1], T20);
    T22 = _mm_unpackhi_epi64(T10, T11);
    T23 = _mm_unpackhi_epi64(T12, T13);
    T22 = _mm_srai_epi16(_mm_add_epi16(T22, c_16), 5);
    T23 = _mm_srai_epi16(_mm_add_epi16(T23, c_16), 5);
    T22 = _mm_packus_epi16(T22, T23);
    _mm_store_si128((__m128i*)dstN[31], T22);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[21]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T12 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[31]));
    T13 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T20 = _mm_unpacklo_epi64(T10, T11);
    T21 = _mm_unpacklo_epi64(T12, T13);
    T20 = _mm_srai_epi16(_mm_add_epi16(T20, c_16), 5);
    T21 = _mm_srai_epi16(_mm_add_epi16(T21, c_16), 5);
    T20 = _mm_packus_epi16(T20, T21);
    _mm_store_si128((__m128i*)dstN[ 2], T20);
    T22 = _mm_unpackhi_epi64(T10, T11);
    T23 = _mm_unpackhi_epi64(T12, T13);
    T22 = _mm_srai_epi16(_mm_add_epi16(T22, c_16), 5);
    T23 = _mm_srai_epi16(_mm_add_epi16(T23, c_16), 5);
    T22 = _mm_packus_epi16(T22, T23);
    _mm_store_si128((__m128i*)dstN[30], T22);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T12 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T13 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T20 = _mm_unpacklo_epi64(T10, T11);
    T21 = _mm_unpacklo_epi64(T12, T13);
    T20 = _mm_srai_epi16(_mm_add_epi16(T20, c_16), 5);
    T21 = _mm_srai_epi16(_mm_add_epi16(T21, c_16), 5);
    T20 = _mm_packus_epi16(T20, T21);
    _mm_store_si128((__m128i*)dstN[ 3], T20);
    T22 = _mm_unpackhi_epi64(T10, T11);
    T23 = _mm_unpackhi_epi64(T12, T13);
    T22 = _mm_srai_epi16(_mm_add_epi16(T22, c_16), 5);
    T23 = _mm_srai_epi16(_mm_add_epi16(T23, c_16), 5);
    T22 = _mm_packus_epi16(T22, T23);
    _mm_store_si128((__m128i*)dstN[29], T22);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T11 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T12 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 7]));
    T13 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T20 = _mm_unpacklo_epi64(T10, T11);
    T21 = _mm_unpacklo_epi64(T12, T13);
    T20 = _mm_srai_epi16(_mm_add_epi16(T20, c_16), 5);
    T21 = _mm_srai_epi16(_mm_add_epi16(T21, c_16), 5);
    T20 = _mm_packus_epi16(T20, T21);
    _mm_store_si128((__m128i*)dstN[ 4], T20);
    T22 = _mm_unpackhi_epi64(T10, T11);
    T23 = _mm_unpackhi_epi64(T12, T13);
    T22 = _mm_srai_epi16(_mm_add_epi16(T22, c_16), 5);
    T23 = _mm_srai_epi16(_mm_add_epi16(T23, c_16), 5);
    T22 = _mm_packus_epi16(T22, T23);
    _mm_store_si128((__m128i*)dstN[28], T22);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 9]));
    T11 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T12 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T13 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T20 = _mm_unpacklo_epi64(T10, T11);
    T21 = _mm_unpacklo_epi64(T12, T13);
    T20 = _mm_srai_epi16(_mm_add_epi16(T20, c_16), 5);
    T21 = _mm_srai_epi16(_mm_add_epi16(T21, c_16), 5);
    T20 = _mm_packus_epi16(T20, T21);
    _mm_store_si128((__m128i*)dstN[ 5], T20);
    T22 = _mm_unpackhi_epi64(T10, T11);
    T23 = _mm_unpackhi_epi64(T12, T13);
    T22 = _mm_srai_epi16(_mm_add_epi16(T22, c_16), 5);
    T23 = _mm_srai_epi16(_mm_add_epi16(T23, c_16), 5);
    T22 = _mm_packus_epi16(T22, T23);
    _mm_store_si128((__m128i*)dstN[27], T22);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 5]));
    T11 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T12 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T13 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T20 = _mm_unpacklo_epi64(T10, T11);
    T21 = _mm_unpacklo_epi64(T12, T13);
    T20 = _mm_srai_epi16(_mm_add_epi16(T20, c_16), 5);
    T21 = _mm_srai_epi16(_mm_add_epi16(T21, c_16), 5);
    T20 = _mm_packus_epi16(T20, T21);
    _mm_store_si128((__m128i*)dstN[ 6], T20);
    T22 = _mm_unpackhi_epi64(T10, T11);
    T23 = _mm_unpackhi_epi64(T12, T13);
    T22 = _mm_srai_epi16(_mm_add_epi16(T22, c_16), 5);
    T23 = _mm_srai_epi16(_mm_add_epi16(T23, c_16), 5);
    T22 = _mm_packus_epi16(T22, T23);
    _mm_store_si128((__m128i*)dstN[26], T22);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T11 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T12 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T13 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T20 = _mm_unpacklo_epi64(T10, T11);
    T21 = _mm_unpacklo_epi64(T12, T13);
    T20 = _mm_srai_epi16(_mm_add_epi16(T20, c_16), 5);
    T21 = _mm_srai_epi16(_mm_add_epi16(T21, c_16), 5);
    T20 = _mm_packus_epi16(T20, T21);
    _mm_store_si128((__m128i*)dstN[ 7], T20);
    T22 = _mm_unpackhi_epi64(T10, T11);
    T23 = _mm_unpackhi_epi64(T12, T13);
    T22 = _mm_srai_epi16(_mm_add_epi16(T22, c_16), 5);
    T23 = _mm_srai_epi16(_mm_add_epi16(T23, c_16), 5);
    T22 = _mm_packus_epi16(T22, T23);
    _mm_store_si128((__m128i*)dstN[25], T22);

    R00 = _mm_loadu_si128((__m128i*)(left0));      // [-- -- -- -- -- --  -- -08 -07 -06 -05 -04 -03 -02 -01  00]
    R10 = _mm_srli_si128(R00, 1);                   // [-- -- -- -- -- --  --  -- -08 -07 -06 -05 -04 -03 -02 -01]
    R20 = _mm_srli_si128(R00, 2);                   // [-- -- -- -- -- --  --  --  -- -08 -07 -06 -05 -04 -03 -02]
    R30 = _mm_srli_si128(R00, 3);                   // [-- -- -- -- -- --  --  --  --  -- -08 -07 -06 -05 -04 -03]

    R01 = _mm_loadu_si128((__m128i*)(above0));     // [-- -- -- -- -- -- --   08  07  06  05  04  03  02  01  00]
    R11 = _mm_srli_si128(R01, 1);                   // [-- -- -- -- -- -- --   --  08  07  06  05  04  03  02  01]
    R21 = _mm_srli_si128(R01, 2);                   // [-- -- -- -- -- -- --   --  --  08  07  06  05  04  03  02]
    R31 = _mm_srli_si128(R01, 3);                   // [-- -- -- -- -- -- --   --  --  --  08  07  06  05  04  03]

    T00 = _mm_shuffle_epi8( R00, _mm_load_si128((__m128i*)tab_angle_0[ 0]));    // [ -- -08 -07 -06 -06 -05 -05 -04 -04 -03 -03 -02 -02 -01 -01  00]
    T04 = _mm_shuffle_epi8( R01, _mm_load_si128((__m128i*)tab_angle_0[ 0]));    // [ --  08  07  06  06  05  05  04  04  03  03  02  02  01  01  00]
    T00 = _mm_unpacklo_epi64(T00, T04);     // [ 04  03  03  02  02  01  01  00 -04 -03 -03 -02 -02 -01 -01  00]

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T11 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T12 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T13 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T20 = _mm_unpacklo_epi64(T10, T11);
    T21 = _mm_unpacklo_epi64(T12, T13);
    T20 = _mm_srai_epi16(_mm_add_epi16(T20, c_16), 5);
    T21 = _mm_srai_epi16(_mm_add_epi16(T21, c_16), 5);
    T20 = _mm_packus_epi16(T20, T21);
    _mm_store_si128((__m128i*)dstN[ 9], T20);
    T22 = _mm_unpackhi_epi64(T10, T11);
    T23 = _mm_unpackhi_epi64(T12, T13);
    T22 = _mm_srai_epi16(_mm_add_epi16(T22, c_16), 5);
    T23 = _mm_srai_epi16(_mm_add_epi16(T23, c_16), 5);
    T22 = _mm_packus_epi16(T22, T23);
    _mm_store_si128((__m128i*)dstN[23], T22);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T11 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T12 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T13 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T20 = _mm_unpacklo_epi64(T10, T11);
    T21 = _mm_unpacklo_epi64(T12, T13);
    T20 = _mm_srai_epi16(_mm_add_epi16(T20, c_16), 5);
    T21 = _mm_srai_epi16(_mm_add_epi16(T21, c_16), 5);
    T20 = _mm_packus_epi16(T20, T21);
    _mm_store_si128((__m128i*)dstN[10], T20);
    T22 = _mm_unpackhi_epi64(T10, T11);
    T23 = _mm_unpackhi_epi64(T12, T13);
    T22 = _mm_srai_epi16(_mm_add_epi16(T22, c_16), 5);
    T23 = _mm_srai_epi16(_mm_add_epi16(T23, c_16), 5);
    T22 = _mm_packus_epi16(T22, T23);
    _mm_store_si128((__m128i*)dstN[22], T22);

    T30 = _mm_shuffle_epi8(T00, _mm_load_si128((__m128i*)tab_angle_0[1]));
    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T11 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T12 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 5]));
    T13 = _mm_maddubs_epi16(T30, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T20 = _mm_unpacklo_epi64(T10, T11);
    T21 = _mm_unpacklo_epi64(T12, T13);
    T20 = _mm_srai_epi16(_mm_add_epi16(T20, c_16), 5);
    T21 = _mm_srai_epi16(_mm_add_epi16(T21, c_16), 5);
    T20 = _mm_packus_epi16(T20, T21);
    _mm_store_si128((__m128i*)dstN[11], T20);
    T22 = _mm_unpackhi_epi64(T10, T11);
    T23 = _mm_unpackhi_epi64(T12, T13);
    T22 = _mm_srai_epi16(_mm_add_epi16(T22, c_16), 5);
    T23 = _mm_srai_epi16(_mm_add_epi16(T23, c_16), 5);
    T22 = _mm_packus_epi16(T22, T23);
    _mm_store_si128((__m128i*)dstN[21], T22);

    T30 = _mm_shuffle_epi8(T00, _mm_load_si128((__m128i*)tab_angle_0[2]));
    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T11 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T12 = _mm_maddubs_epi16(T30, _mm_load_si128((__m128i*)tab_angle_1[25]));
    T13 = _mm_maddubs_epi16(T30, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T20 = _mm_unpacklo_epi64(T10, T11);
    T21 = _mm_unpacklo_epi64(T12, T13);
    T20 = _mm_srai_epi16(_mm_add_epi16(T20, c_16), 5);
    T21 = _mm_srai_epi16(_mm_add_epi16(T21, c_16), 5);
    T20 = _mm_packus_epi16(T20, T21);
    _mm_store_si128((__m128i*)dstN[12], T20);
    T22 = _mm_unpackhi_epi64(T10, T11);
    T23 = _mm_unpackhi_epi64(T12, T13);
    T22 = _mm_srai_epi16(_mm_add_epi16(T22, c_16), 5);
    T23 = _mm_srai_epi16(_mm_add_epi16(T23, c_16), 5);
    T22 = _mm_packus_epi16(T22, T23);
    _mm_store_si128((__m128i*)dstN[20], T22);

    T31 = _mm_shuffle_epi8(T00, _mm_load_si128((__m128i*)tab_angle_0[3]));
    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T11 = _mm_maddubs_epi16(T30, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T12 = _mm_maddubs_epi16(T30, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T13 = _mm_maddubs_epi16(T31, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T20 = _mm_unpacklo_epi64(T10, T11);
    T21 = _mm_unpacklo_epi64(T12, T13);
    T20 = _mm_srai_epi16(_mm_add_epi16(T20, c_16), 5);
    T21 = _mm_srai_epi16(_mm_add_epi16(T21, c_16), 5);
    T20 = _mm_packus_epi16(T20, T21);
    _mm_store_si128((__m128i*)dstN[13], T20);
    T22 = _mm_unpackhi_epi64(T10, T11);
    T23 = _mm_unpackhi_epi64(T12, T13);
    T22 = _mm_srai_epi16(_mm_add_epi16(T22, c_16), 5);
    T23 = _mm_srai_epi16(_mm_add_epi16(T23, c_16), 5);
    T22 = _mm_packus_epi16(T22, T23);
    _mm_store_si128((__m128i*)dstN[19], T22);

    T31 = _mm_shuffle_epi8(T00, _mm_load_si128((__m128i*)tab_angle_0[4]));
    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T11 = _mm_maddubs_epi16(T30, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T12 = _mm_maddubs_epi16(T30, _mm_load_si128((__m128i*)tab_angle_1[ 1]));
    T13 = _mm_maddubs_epi16(T31, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T20 = _mm_unpacklo_epi64(T10, T11);
    T21 = _mm_unpacklo_epi64(T12, T13);
    T20 = _mm_srai_epi16(_mm_add_epi16(T20, c_16), 5);
    T21 = _mm_srai_epi16(_mm_add_epi16(T21, c_16), 5);
    T20 = _mm_packus_epi16(T20, T21);
    _mm_store_si128((__m128i*)dstN[14], T20);
    T22 = _mm_unpackhi_epi64(T10, T11);
    T23 = _mm_unpackhi_epi64(T12, T13);
    T22 = _mm_srai_epi16(_mm_add_epi16(T22, c_16), 5);
    T23 = _mm_srai_epi16(_mm_add_epi16(T23, c_16), 5);
    T22 = _mm_packus_epi16(T22, T23);
    _mm_store_si128((__m128i*)dstN[18], T22);

    T30 = _mm_shuffle_epi8(T00, _mm_load_si128((__m128i*)tab_angle_0[5]));
    T31 = _mm_shuffle_epi8(T00, _mm_load_si128((__m128i*)tab_angle_0[6]));
    T32 = _mm_shuffle_epi8(T00, _mm_load_si128((__m128i*)tab_angle_0[7]));
    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T11 = _mm_maddubs_epi16(T30, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T12 = _mm_maddubs_epi16(T31, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T13 = _mm_maddubs_epi16(T32, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T20 = _mm_unpacklo_epi64(T10, T11);
    T21 = _mm_unpacklo_epi64(T12, T13);
    T20 = _mm_srai_epi16(_mm_add_epi16(T20, c_16), 5);
    T21 = _mm_srai_epi16(_mm_add_epi16(T21, c_16), 5);
    T20 = _mm_packus_epi16(T20, T21);
    _mm_store_si128((__m128i*)dstN[15], T20);
    T22 = _mm_unpackhi_epi64(T10, T11);
    T23 = _mm_unpackhi_epi64(T12, T13);
    T22 = _mm_srai_epi16(_mm_add_epi16(T22, c_16), 5);
    T23 = _mm_srai_epi16(_mm_add_epi16(T23, c_16), 5);
    T22 = _mm_packus_epi16(T22, T23);
    _mm_store_si128((__m128i*)dstN[17], T22);

    T30 = _mm_shuffle_epi8(T00, _mm_load_si128((__m128i*)tab_angle_0[8]));
    _mm_store_si128((__m128i*)dstN[16], T30);
}

void predIntraAngs8(pixel *dst, pixel *above0, pixel *left0, pixel *above1, pixel *left1, bool filter)
{
#define N   (8)
    pixel (*dstN)[8*8] = (pixel(*)[8*8])dst;

    __m128i T00, T01, T02, T03;
    __m128i R00, R01;
    __m128i R10, R11;
    __m128i R20, R21;
    __m128i R30, R31;

    R00 = _mm_loadu_si128((__m128i*)(left0+1));    // [-16 -15 -14 -13 -12 -11 -10 -09 -08 -07 -06 -05 -04 -03 -02 -01]
    R10 = _mm_loadu_si128((__m128i*)(above0+1));   // [ 16  15  14  13  12  11  10  09  08  07  06  05  04  03  02  01]

    T00 = _mm_unpacklo_epi64(R00, R00);
    _mm_store_si128((__m128i*)&dstN[ 8][0*N], T00);
    _mm_store_si128((__m128i*)&dstN[ 8][2*N], T00);
    _mm_store_si128((__m128i*)&dstN[ 8][4*N], T00);
    _mm_store_si128((__m128i*)&dstN[ 8][6*N], T00);

    T00 = _mm_unpacklo_epi64(R10, R10);
    _mm_store_si128((__m128i*)&dstN[24][0*N], T00);
    _mm_store_si128((__m128i*)&dstN[24][2*N], T00);
    _mm_store_si128((__m128i*)&dstN[24][4*N], T00);
    _mm_store_si128((__m128i*)&dstN[24][6*N], T00);

    if (filter)
    {
        __m128i roundH, roundV;
        __m128i pL = _mm_set1_epi16(left0[1]);
        __m128i pT = _mm_set1_epi16(above0[1]);
        roundH = _mm_set1_epi16(above0[0]);
        roundV = roundH;

        roundH = _mm_srai_epi16(_mm_sub_epi16(_mm_unpacklo_epi8(R10, _mm_setzero_si128()), roundH), 1);
        roundV = _mm_srai_epi16(_mm_sub_epi16(_mm_unpacklo_epi8(R00, _mm_setzero_si128()), roundV), 1);

        T00 = _mm_add_epi16(roundH, pL);
        T01 = _mm_add_epi16(roundV, pT);
        T00 = _mm_packus_epi16(T00, T01);

        ALIGN_VAR_32(pixel, tmp[2*N]);
        _mm_store_si128((__m128i*)tmp, T00);
        for(int i = 0; i < N; i++)
        {
            dstN[ 8][i*N] = tmp[i  ];
            dstN[24][i*N] = tmp[i+N];
        }
    }

    const __m128i c_16 = _mm_set1_epi16(16);

    R20 = _mm_shuffle_epi8(R00, _mm_load_si128((__m128i*)tab_angle_0[ 0])); // [-09 -08 -08 -07 -07 -06 -06 -05 -05 -04 -04 -03 -03 -02 -02 -01]
    R21 = _mm_shuffle_epi8(R00, _mm_load_si128((__m128i*)tab_angle_0[ 9])); // [-10 -09 -09 -08 -08 -07 -07 -06 -06 -05 -05 -04 -04 -03 -03 -02]

    R30 = _mm_shuffle_epi8(R10, _mm_load_si128((__m128i*)tab_angle_0[ 0]));
    R31 = _mm_shuffle_epi8(R10, _mm_load_si128((__m128i*)tab_angle_0[ 9]));

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[ 1][0*N], T00);
    _mm_store_si128((__m128i*)&dstN[31][0*N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[21]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[21]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[ 2][0*N], T00);
    _mm_store_si128((__m128i*)&dstN[30][0*N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[ 3][0*N], T00);
    _mm_store_si128((__m128i*)&dstN[29][0*N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T01 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T03 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[ 4][0*N], T00);
    _mm_store_si128((__m128i*)&dstN[28][0*N], T01);

    T00 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[ 7]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T02 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[ 7]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[ 4][2*N], T00);
    _mm_store_si128((__m128i*)&dstN[28][2*N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[ 9]));
    T01 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[ 9]));
    T03 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[ 5][0*N], T00);
    _mm_store_si128((__m128i*)&dstN[27][0*N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[ 5][2*N], T00);
    _mm_store_si128((__m128i*)&dstN[27][2*N], T01);

    T00 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T02 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[ 5][4*N], T00);
    _mm_store_si128((__m128i*)&dstN[27][4*N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[ 5]));
    T01 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[ 5]));
    T03 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[ 6][0*N], T00);
    _mm_store_si128((__m128i*)&dstN[26][0*N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T01 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T03 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[ 6][2*N], T00);
    _mm_store_si128((__m128i*)&dstN[26][2*N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[25]));
    T01 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[25]));
    T03 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[ 6][4*N], T00);
    _mm_store_si128((__m128i*)&dstN[26][4*N], T01);

    T00 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[ 3]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T02 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[ 3]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[ 6][6*N], T00);
    _mm_store_si128((__m128i*)&dstN[26][6*N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T01 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T03 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[ 7][0*N], T00);
    _mm_store_si128((__m128i*)&dstN[25][0*N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T01 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T03 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[ 7][2*N], T00);
    _mm_store_si128((__m128i*)&dstN[25][2*N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T01 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T03 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[ 7][4*N], T00);
    _mm_store_si128((__m128i*)&dstN[25][4*N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T01 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[16]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T03 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[16]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[ 7][6*N], T00);
    _mm_store_si128((__m128i*)&dstN[25][6*N], T01);

    R20 = R21;                                                              // [-10 -09 -09 -08 -08 -07 -07 -06 -06 -05 -05 -04 -04 -03 -03 -02]
    R21 = _mm_shuffle_epi8(R00, _mm_load_si128((__m128i*)tab_angle_0[10])); // [-11 -10 -10 -09 -09 -08 -08 -07 -07 -06 -06 -05 -05 -04 -04 -03]
    R30 = R31;
    R31 = _mm_shuffle_epi8(R10, _mm_load_si128((__m128i*)tab_angle_0[10]));

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[31]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[31]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[ 2][2*N], T00);
    _mm_store_si128((__m128i*)&dstN[30][2*N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[ 3][2*N], T00);
    _mm_store_si128((__m128i*)&dstN[29][2*N], T01);

    T00 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[ 1]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T02 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[ 1]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[ 4][4*N], T00);
    _mm_store_si128((__m128i*)&dstN[28][4*N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[31]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[31]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[ 5][6*N], T00);
    _mm_store_si128((__m128i*)&dstN[27][6*N], T01);


    R20 = R21;
    R21 = _mm_shuffle_epi8(R00, _mm_load_si128((__m128i*)tab_angle_0[11]));
    R30 = R31;
    R31 = _mm_shuffle_epi8(R10, _mm_load_si128((__m128i*)tab_angle_0[11]));

    T00 = _mm_loadl_epi64((__m128i*)&dstN[ 4][5*N]);
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T02 = _mm_load_si128((__m128i*)&dstN[28][4*N]);
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T01 = _mm_packus_epi16(T01, T03);
    _mm_storel_epi64((__m128i*)&dstN[ 4][7*N], T01);
    _mm_storeh_pi((__m64*)&dstN[28][7*N], _mm_castsi128_ps(T01));
    T00 = _mm_unpacklo_epi64(T00, T01);
    T01 = _mm_unpackhi_epi64(T02, T01);
    _mm_store_si128((__m128i*)&dstN[ 1][2*N], T00);
    _mm_store_si128((__m128i*)&dstN[31][2*N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T01 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    _mm_storel_epi64((__m128i*)&dstN[ 4][6*N], T00);
    _mm_storeh_pi((__m64*)&dstN[28][6*N], _mm_castsi128_ps(T00));

    T00 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[ 9]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T02 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[ 9]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[ 2][4*N], T00);
    _mm_store_si128((__m128i*)&dstN[30][4*N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[21]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[21]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[ 3][4*N], T00);
    _mm_store_si128((__m128i*)&dstN[29][4*N], T01);


    R20 = R21;
    R21 = _mm_shuffle_epi8(R00, _mm_load_si128((__m128i*)tab_angle_0[12]));
    R30 = R31;
    R31 = _mm_shuffle_epi8(R10, _mm_load_si128((__m128i*)tab_angle_0[12]));

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[ 3][6*N], T00);
    _mm_store_si128((__m128i*)&dstN[29][6*N], T01);

    T00 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T02 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[ 1][4*N], T00);
    _mm_store_si128((__m128i*)&dstN[31][4*N], T01);


    R20 = R21;
    R21 = _mm_shuffle_epi8(R00, _mm_load_si128((__m128i*)tab_angle_0[13]));
    R30 = R31;
    R31 = _mm_shuffle_epi8(R10, _mm_load_si128((__m128i*)tab_angle_0[13]));

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[ 2][6*N], T00);
    _mm_store_si128((__m128i*)&dstN[30][6*N], T01);


    R20 = R21;
    R21 = _mm_shuffle_epi8(R00, _mm_load_si128((__m128i*)tab_angle_0[14]));
    R30 = R31;
    R31 = _mm_shuffle_epi8(R10, _mm_load_si128((__m128i*)tab_angle_0[14]));

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[16]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[16]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[ 1][6*N], T00);
    _mm_store_si128((__m128i*)&dstN[31][6*N], T01);


    // ***********************************************************************
    R00 = _mm_loadu_si128((__m128i*)(left0));      // [-15 -14 -13 -12 -11 -10 -09 -08 -07 -06 -05 -04 -03 -02 -01  00]
    R01 = _mm_unpacklo_epi64(R00, R10);             // [ 08  07  06  05  04  03  02  01 -07 -06 -05 -04 -03 -02 -01  00]
    R10 = _mm_loadu_si128((__m128i*)(above0));     // [ 15  14  13  12  11  10  09  08  07  06  05  04  03  02  01  00]

    R20 = _mm_shuffle_epi8(R00, _mm_load_si128((__m128i*)tab_angle_0[ 0]));
    R30 = _mm_shuffle_epi8(R10, _mm_load_si128((__m128i*)tab_angle_0[ 0]));

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T01 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T03 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[ 9][0*N], T00);
    _mm_store_si128((__m128i*)&dstN[23][0*N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T01 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T03 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[ 9][2*N], T00);
    _mm_store_si128((__m128i*)&dstN[23][2*N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T01 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T03 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[ 9][4*N], T00);
    _mm_store_si128((__m128i*)&dstN[23][4*N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T01 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[16]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T03 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[16]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[ 9][6*N], T00);
    _mm_store_si128((__m128i*)&dstN[23][6*N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T01 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T03 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[10][0*N], T00);
    _mm_store_si128((__m128i*)&dstN[22][0*N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T01 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T03 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[10][2*N], T00);
    _mm_store_si128((__m128i*)&dstN[22][2*N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[ 7]));
    T01 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[ 7]));
    T03 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[10][4*N], T00);
    _mm_store_si128((__m128i*)&dstN[22][4*N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T01 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T03 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[11][0*N], T00);
    _mm_store_si128((__m128i*)&dstN[21][0*N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T01 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T03 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[12][0*N], T00);
//     _mm_storeh_pi((__m64*)&dstN[15][0*N], _mm_castsi128_ps(T00));
    _mm_store_si128((__m128i*)&dstN[20][0*N], T01);
//     _mm_storeh_pi((__m64*)&dstN[17][0*N], _mm_castsi128_ps(T01));


    R21 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[15]));
    R31 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[16]));

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[13][0*N], T00);
    _mm_store_si128((__m128i*)&dstN[19][0*N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[14][0*N], T00);
    _mm_store_si128((__m128i*)&dstN[18][0*N], T01);

    T00 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[25]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T02 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[25]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[12][2*N], T00);
    _mm_store_si128((__m128i*)&dstN[20][2*N], T01);

    T00 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[ 1]));
    T02 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[ 1]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_storel_epi64((__m128i*)&dstN[13][2*N], T00);
    _mm_storeh_pi((__m64*)&dstN[14][2*N], _mm_castsi128_ps(T00));
    _mm_storel_epi64((__m128i*)&dstN[19][2*N], T01);
    _mm_storeh_pi((__m64*)&dstN[18][2*N], _mm_castsi128_ps(T01));


    R21 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[17]));
    R31 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[18]));

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[ 5]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[ 5]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[11][2*N], T00);
    _mm_store_si128((__m128i*)&dstN[21][2*N], T01);

    T00 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T02 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[11][4*N], T00);
    _mm_store_si128((__m128i*)&dstN[21][4*N], T01);

    T00 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[ 1]));
    T01 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[ 1]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    _mm_storel_epi64((__m128i*)&dstN[11][6*N], T00);
    _mm_storeh_pi((__m64*)&dstN[21][6*N], _mm_castsi128_ps(T00));

    R21 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[19]));
    R31 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[20]));

    T00 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T01 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    _mm_storel_epi64((__m128i*)&dstN[11][7*N], T00);
    _mm_storeh_pi((__m64*)&dstN[21][7*N], _mm_castsi128_ps(T00));


    R21 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[21]));
    R31 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[22]));

    T00 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[29]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T02 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[29]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[10][6*N], T00);
    _mm_store_si128((__m128i*)&dstN[22][6*N], T01);

    R21 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[23]));
    R31 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[24]));

    T00 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[31]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T02 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[31]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[12][4*N], T00);
    _mm_store_si128((__m128i*)&dstN[20][4*N], T01);

    R20 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[25]));
    R30 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[26]));

    T00 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[ 5]));
    T01 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T02 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[ 5]));
    T03 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[12][6*N], T00);
    _mm_store_si128((__m128i*)&dstN[20][6*N], T01);

    R20 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[27]));
    R30 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[28]));

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T01 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T03 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_storeu_si128((__m128i*)&dstN[13][3*N], T00);
    _mm_storeu_si128((__m128i*)&dstN[19][3*N], T01);

    R20 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[29]));
    R30 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[30]));

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T01 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[ 9]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T03 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[ 9]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_storeu_si128((__m128i*)&dstN[13][5*N], T00);
    _mm_storeu_si128((__m128i*)&dstN[19][5*N], T01);

    R21 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[31]));
    R31 = _mm_or_si128(_mm_slli_si128(R30, 2), _mm_cvtsi32_si128( (left0[6] << 8) | left0[8] ));

    T00 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T01 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    _mm_storel_epi64((__m128i*)&dstN[13][7*N], T00);
    _mm_storeh_pi((__m64*)&dstN[19][7*N], _mm_castsi128_ps(T00));

    R20 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[32]));
    R30 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[33]));
    R21 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[34]));
    R31 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[35]));

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_storeu_si128((__m128i*)&dstN[14][3*N], T00);
    _mm_storeu_si128((__m128i*)&dstN[18][3*N], T01);

    R20 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[36]));
    R30 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[37]));

    T00 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T01 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T02 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T03 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_storeu_si128((__m128i*)&dstN[14][5*N], T00);
    _mm_storeu_si128((__m128i*)&dstN[18][5*N], T01);

    R21 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[38]));
    R31 = _mm_or_si128(_mm_slli_si128(R30, 2), _mm_cvtsi32_si128( (left0[6] << 8) | left0[8] ));

    T00 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T01 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    _mm_storel_epi64((__m128i*)&dstN[14][7*N], T00);
    _mm_storeh_pi((__m64*)&dstN[18][7*N], _mm_castsi128_ps(T00));


    R00 = _mm_loadl_epi64((__m128i*)&left0[1]);
    R01 = _mm_loadl_epi64((__m128i*)above0);
    R01 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[39]));

    R10 = _mm_loadl_epi64((__m128i*)&above0[1]);
    R11 = _mm_loadl_epi64((__m128i*)left0);
    R11 = _mm_shuffle_epi8(R11, _mm_load_si128((__m128i*)tab_angle_0[39]));

    R00 = _mm_unpacklo_epi64(R00, R01);                                     // [00 01 02 04 05 06 07 00 -8 -7 -6 -5 -4 -3 -2 -1]
    R10 = _mm_unpacklo_epi64(R10, R11);

    R01 = _mm_shuffle_epi8(R00, _mm_load_si128((__m128i*)tab_angle_0[40])); // [01 02 04 05 06 07 00 -8 -7 -6 -5 -4 -3 -2 -1  0]
    R11 = _mm_shuffle_epi8(R10, _mm_load_si128((__m128i*)tab_angle_0[40]));

    R20 = _mm_unpacklo_epi8(R01, R00);
    R30 = _mm_unpacklo_epi8(R11, R10);
    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    R00 = R01;
    R10 = R11;
    R01 = _mm_shuffle_epi8(R00, _mm_load_si128((__m128i*)tab_angle_0[40]));
    R11 = _mm_shuffle_epi8(R10, _mm_load_si128((__m128i*)tab_angle_0[40]));
    R21 = _mm_unpacklo_epi8(R01, R00);
    R31 = _mm_unpacklo_epi8(R11, R10);
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[15][0*N], T00);
    _mm_store_si128((__m128i*)&dstN[17][0*N], T01);

    R00 = R01;
    R10 = R11;
    R01 = _mm_shuffle_epi8(R00, _mm_load_si128((__m128i*)tab_angle_0[40]));
    R11 = _mm_shuffle_epi8(R10, _mm_load_si128((__m128i*)tab_angle_0[40]));
    R20 = _mm_unpacklo_epi8(R01, R00);
    R30 = _mm_unpacklo_epi8(R11, R10);
    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[18]));
    R00 = R01;
    R10 = R11;
    R01 = _mm_shuffle_epi8(R00, _mm_load_si128((__m128i*)tab_angle_0[40]));
    R11 = _mm_shuffle_epi8(R10, _mm_load_si128((__m128i*)tab_angle_0[40]));
    R21 = _mm_unpacklo_epi8(R01, R00);
    R31 = _mm_unpacklo_epi8(R11, R10);
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[15][2*N], T00);
    _mm_store_si128((__m128i*)&dstN[17][2*N], T01);

    R00 = R01;
    R10 = R11;
    R01 = _mm_shuffle_epi8(R00, _mm_load_si128((__m128i*)tab_angle_0[40]));
    R11 = _mm_shuffle_epi8(R10, _mm_load_si128((__m128i*)tab_angle_0[40]));
    R20 = _mm_unpacklo_epi8(R01, R00);
    R30 = _mm_unpacklo_epi8(R11, R10);
    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[30]));
    R21 = _mm_unpacklo_epi8(R01, R00);
    R31 = _mm_unpacklo_epi8(R11, R10);
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[15][4*N], T00);
    _mm_store_si128((__m128i*)&dstN[17][4*N], T01);

    R00 = R01;
    R10 = R11;
    R01 = _mm_shuffle_epi8(R00, _mm_load_si128((__m128i*)tab_angle_0[40]));
    R11 = _mm_shuffle_epi8(R10, _mm_load_si128((__m128i*)tab_angle_0[40]));
    R20 = _mm_unpacklo_epi8(R01, R00);
    R30 = _mm_unpacklo_epi8(R11, R10);
    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[10]));
    R00 = R01;
    R10 = R11;
    R01 = _mm_shuffle_epi8(R00, _mm_load_si128((__m128i*)tab_angle_0[40]));
    R11 = _mm_shuffle_epi8(R10, _mm_load_si128((__m128i*)tab_angle_0[40]));
    R21 = _mm_unpacklo_epi8(R01, R00);
    R31 = _mm_unpacklo_epi8(R11, R10);
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[16]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[16]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[15][6*N], T00);
    _mm_store_si128((__m128i*)&dstN[17][6*N], T01);


    // 8, 7, 6, 5, 4, 3, 2, 1, 0, -1, -2, -3, -4, -5, -6, -7,
    R00 = _mm_loadl_epi64((__m128i*)left1);
    R01 = _mm_loadl_epi64((__m128i*)&above1[1]);

    //R00 = _mm_setr_epi8(0, -1, -2, -3, -4, -5, -6, -7, 99, 99, 99, 99, 99, 99, 99, 99);
    //R01 = _mm_setr_epi8(1, 2, 3, 4, 5, 6, 7, 8, 99, 99, 99, 99, 99, 99, 99, 99);
    R00 = _mm_shuffle_epi8(R00, _mm_load_si128((__m128i*)tab_angle_0[41]));
    R00 = _mm_unpacklo_epi64(R00, R01);

    R01 = _mm_srli_si128(R00, 1);
    T00 = _mm_unpacklo_epi64(R01, R00);
    _mm_store_si128((__m128i*)&dstN[16][6*N], T00);

    R00 = _mm_srli_si128(R00, 2);
    R01 = _mm_srli_si128(R01, 2);
    T00 = _mm_unpacklo_epi64(R01, R00);
    _mm_store_si128((__m128i*)&dstN[16][4*N], T00);

    R00 = _mm_srli_si128(R00, 2);
    R01 = _mm_srli_si128(R01, 2);
    T00 = _mm_unpacklo_epi64(R01, R00);
    _mm_store_si128((__m128i*)&dstN[16][2*N], T00);

    R00 = _mm_srli_si128(R00, 2);
    R01 = _mm_srli_si128(R01, 2);
    T00 = _mm_unpacklo_epi64(R01, R00);
    _mm_store_si128((__m128i*)&dstN[16][0*N], T00);

    T00 = _mm_unpacklo_epi64(_mm_loadl_epi64((__m128i*)&left1[2]), _mm_loadl_epi64((__m128i*)&left1[3]));
    T01 = _mm_unpacklo_epi64(_mm_loadl_epi64((__m128i*)&left1[4]), _mm_loadl_epi64((__m128i*)&left1[5]));
    T02 = _mm_unpacklo_epi64(_mm_loadl_epi64((__m128i*)&left1[6]), _mm_loadl_epi64((__m128i*)&left1[7]));
    T03 = _mm_unpacklo_epi64(_mm_loadl_epi64((__m128i*)&left1[8]), _mm_loadl_epi64((__m128i*)&left1[9]));
    _mm_store_si128((__m128i*)&dstN[ 0][0*N], T00);
    _mm_store_si128((__m128i*)&dstN[ 0][2*N], T01);
    _mm_store_si128((__m128i*)&dstN[ 0][4*N], T02);
    _mm_store_si128((__m128i*)&dstN[ 0][6*N], T03);

    T00 = _mm_unpacklo_epi64(_mm_loadl_epi64((__m128i*)&above1[2]), _mm_loadl_epi64((__m128i*)&above1[3]));
    T01 = _mm_unpacklo_epi64(_mm_loadl_epi64((__m128i*)&above1[4]), _mm_loadl_epi64((__m128i*)&above1[5]));
    T02 = _mm_unpacklo_epi64(_mm_loadl_epi64((__m128i*)&above1[6]), _mm_loadl_epi64((__m128i*)&above1[7]));
    T03 = _mm_unpacklo_epi64(_mm_loadl_epi64((__m128i*)&above1[8]), _mm_loadl_epi64((__m128i*)&above1[9]));
    _mm_store_si128((__m128i*)&dstN[32][0*N], T00);
    _mm_store_si128((__m128i*)&dstN[32][2*N], T01);
    _mm_store_si128((__m128i*)&dstN[32][4*N], T02);
    _mm_store_si128((__m128i*)&dstN[32][6*N], T03);

#undef N
}

void predIntraAngs16(pixel *dst0, pixel *above0, pixel *left0, pixel *above1, pixel *left1, bool filter)
{
#define N   (16)
    pixel (*dstN)[N*N] = (pixel(*)[N*N])dst0;

    const __m128i c_16 = _mm_set1_epi16(16);
    __m128i T00, T01, T02, T03;
    __m128i T10, T11, T12, T13, T14;

    T00 = _mm_loadu_si128((__m128i*)(left0+1));
    T01 = _mm_loadu_si128((__m128i*)(above0+1));

    _mm_store_si128((__m128i*)&dstN[ 8][ 0*N], T00);
    _mm_store_si128((__m128i*)&dstN[ 8][ 1*N], T00);
    _mm_store_si128((__m128i*)&dstN[ 8][ 2*N], T00);
    _mm_store_si128((__m128i*)&dstN[ 8][ 3*N], T00);
    _mm_store_si128((__m128i*)&dstN[ 8][ 4*N], T00);
    _mm_store_si128((__m128i*)&dstN[ 8][ 5*N], T00);
    _mm_store_si128((__m128i*)&dstN[ 8][ 6*N], T00);
    _mm_store_si128((__m128i*)&dstN[ 8][ 7*N], T00);
    _mm_store_si128((__m128i*)&dstN[ 8][ 8*N], T00);
    _mm_store_si128((__m128i*)&dstN[ 8][ 9*N], T00);
    _mm_store_si128((__m128i*)&dstN[ 8][10*N], T00);
    _mm_store_si128((__m128i*)&dstN[ 8][11*N], T00);
    _mm_store_si128((__m128i*)&dstN[ 8][12*N], T00);
    _mm_store_si128((__m128i*)&dstN[ 8][13*N], T00);
    _mm_store_si128((__m128i*)&dstN[ 8][14*N], T00);
    _mm_store_si128((__m128i*)&dstN[ 8][15*N], T00);
    _mm_store_si128((__m128i*)&dstN[24][ 0*N], T01);
    _mm_store_si128((__m128i*)&dstN[24][ 1*N], T01);
    _mm_store_si128((__m128i*)&dstN[24][ 2*N], T01);
    _mm_store_si128((__m128i*)&dstN[24][ 3*N], T01);
    _mm_store_si128((__m128i*)&dstN[24][ 4*N], T01);
    _mm_store_si128((__m128i*)&dstN[24][ 5*N], T01);
    _mm_store_si128((__m128i*)&dstN[24][ 6*N], T01);
    _mm_store_si128((__m128i*)&dstN[24][ 7*N], T01);
    _mm_store_si128((__m128i*)&dstN[24][ 8*N], T01);
    _mm_store_si128((__m128i*)&dstN[24][ 9*N], T01);
    _mm_store_si128((__m128i*)&dstN[24][10*N], T01);
    _mm_store_si128((__m128i*)&dstN[24][11*N], T01);
    _mm_store_si128((__m128i*)&dstN[24][12*N], T01);
    _mm_store_si128((__m128i*)&dstN[24][13*N], T01);
    _mm_store_si128((__m128i*)&dstN[24][14*N], T01);
    _mm_store_si128((__m128i*)&dstN[24][15*N], T01);

    if (filter)
    {
        __m128i roundH0, roundH1;
        __m128i roundV0, roundV1;
        __m128i pL = _mm_set1_epi16(left0[1]);
        __m128i pT = _mm_set1_epi16(above0[1]);
        __m128i pLT = _mm_set1_epi16(above0[0]);

        roundH0 = _mm_srai_epi16(_mm_sub_epi16(_mm_unpacklo_epi8(T01, _mm_setzero_si128()), pLT), 1);
        roundV0 = _mm_srai_epi16(_mm_sub_epi16(_mm_unpacklo_epi8(T00, _mm_setzero_si128()), pLT), 1);
        roundH1 = _mm_srai_epi16(_mm_sub_epi16(_mm_unpackhi_epi8(T01, _mm_setzero_si128()), pLT), 1);
        roundV1 = _mm_srai_epi16(_mm_sub_epi16(_mm_unpackhi_epi8(T00, _mm_setzero_si128()), pLT), 1);

        T00 = _mm_add_epi16(roundH0, pL);
        T01 = _mm_add_epi16(roundH1, pL);
        T00 = _mm_packus_epi16(T00, T01);
        T02 = _mm_add_epi16(roundV0, pT);
        T03 = _mm_add_epi16(roundV1, pT);
        T02 = _mm_packus_epi16(T02, T03);

        ALIGN_VAR_32(pixel, tmp0[N]);
        ALIGN_VAR_32(pixel, tmp1[N]);

        _mm_store_si128((__m128i*)tmp0, T00);
        _mm_store_si128((__m128i*)tmp1, T02);

        for(int i = 0; i < N; i++)
        {
            dstN[ 8][i*N] = tmp0[i];
            dstN[24][i*N] = tmp1[i];
        }
    }


    T00 = _mm_loadu_si128((__m128i*)(left0+0));
    T01 = _mm_loadu_si128((__m128i*)(left0+1));
    T02 = _mm_loadu_si128((__m128i*)(left0+2));

    _mm_store_si128((__m128i*)&dstN[ 9][15 * N], T00);
    _mm_store_si128((__m128i*)&dstN[ 7][15 * N], T02);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 7][ 0 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 9][14 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 7][ 1 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 9][13 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 7][ 2 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 9][12 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 7][ 3 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 9][11 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 7][ 4 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 9][10 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 7][ 5 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 9][ 9 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 7][ 6 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 9][ 8 * N], T13);

    T13 = _mm_avg_epu8(T01, T02);
    T14 = _mm_avg_epu8(T01, T00);
    _mm_store_si128((__m128i*)&dstN[ 7][ 7 * N], T13);
    _mm_store_si128((__m128i*)&dstN[ 9][ 7 * N], T14);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 7][ 8 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 9][ 6 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 7][ 9 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 9][ 5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 7][10 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 9][ 4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 7][11 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 9][ 3 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 7][12 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 9][ 2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 7][13 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 9][ 1 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 7][14 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 9][ 0 * N], T13);


    // **********************************************************************
    T00 = _mm_loadu_si128((__m128i*)(above0+0));
    T01 = _mm_loadu_si128((__m128i*)(above0+1));
    T02 = _mm_loadu_si128((__m128i*)(above0+2));

    _mm_store_si128((__m128i*)&dstN[23][15 * N], T00);
    _mm_store_si128((__m128i*)&dstN[25][15 * N], T02);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[25][ 0 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[23][14 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[25][ 1 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[23][13 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[25][ 2 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[23][12 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[25][ 3 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[23][11 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[25][ 4 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[23][10 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[25][ 5 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[23][ 9 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[25][ 6 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[23][ 8 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[16]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[16]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[16]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[25][ 7 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[23][ 7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[25][ 8 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[23][ 6 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[25][ 9 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[23][ 5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[25][10 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[23][ 4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[25][11 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[23][ 3 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[25][12 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[23][ 2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[25][13 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[23][ 1 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[25][14 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[23][ 0 * N], T13);


    // **********************************************************************
    T00 = _mm_loadu_si128((__m128i*)(left1+0));
    T01 = _mm_loadu_si128((__m128i*)(left1+1));

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[10][ 0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[10][ 1 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[10][ 2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[10][ 3 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 7]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 7]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[10][ 4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[10][ 5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[11][ 0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[11][ 1 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 5]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 5]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[11][ 2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[12][ 0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[12][ 1 * N], T13);
    _mm_store_si128((__m128i*)&dstN[15][ 0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[13][ 0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[14][ 0 * N], T13);


    // **********************************************************************
    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[6], 0);
    T03 = T00;

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[29]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[29]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[10][ 6 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[10][ 7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[10][ 8 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[10][ 9 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 9]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 9]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[10][10 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[10][11 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[13], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[31]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[31]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[10][12 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[10][13 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[21]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[21]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[10][14 * N], T13);

    T13 = _mm_avg_epu8(T00, T01);
    _mm_store_si128((__m128i*)&dstN[10][15 * N], T13);


    // **********************************************************************
    T00 = _mm_insert_epi8(T03, above1[4], 0);
    T01 = _mm_loadu_si128((__m128i*)(left1));

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[11][ 3 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[11][ 4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[11][ 5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 1]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 1]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[11][ 6 * N], T13);


    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[ 7], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[11][ 7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[11][ 8 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[11][ 9 * N], T13);


    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[11], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[29]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[29]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[11][10 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[11][11 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[11][12 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[11][13 * N], T13);


    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[14], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[25]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[25]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[11][14 * N], T13);

    T13 = _mm_avg_epu8(T00, T01);
    _mm_store_si128((__m128i*)&dstN[11][15 * N], T13);


    // **********************************************************************
    T00 = _mm_insert_epi8(T03, above1[2], 0);
    T01 = _mm_loadu_si128((__m128i*)(left1));
    T03 = T00;

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[25]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[25]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[12][ 2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[12][ 3 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[13][ 1 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[13][ 2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[14][ 1 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 1]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 1]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[14][ 2 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[ 5], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[31]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[31]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[12][ 4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[12][ 5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 5]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 5]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[12][ 6 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[ 7], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[12][ 7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[12][ 8 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[10], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[12][ 9 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[12][10 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[12][11 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[12], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[12][12 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[12][13 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[15], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[29]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[29]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[12][14 * N], T13);

    T13 = _mm_avg_epu8(T00, T01);
    _mm_store_si128((__m128i*)&dstN[12][15 * N], T13);


    // **********************************************************************
    T01 = T03;
    T00 = _mm_insert_epi8(_mm_slli_si128(T03, 1), above1[4], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[13][ 3 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[13][ 4 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[ 6], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[13][ 5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 9]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 9]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[13][ 6 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[ 8], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[13][ 7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 7]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 7]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[13][ 8 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[ 9], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[13][ 9 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 5]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 5]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[13][10 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[11], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[13][11 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 3]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 3]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[13][12 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[13], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[13][13 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 1]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 1]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[13][14 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[15], 0);

    T13 = _mm_avg_epu8(T00, T01);
    _mm_store_si128((__m128i*)&dstN[13][15 * N], T13);


    // **********************************************************************
    T01 = T03;
    T00 = _mm_insert_epi8(_mm_slli_si128(T03, 1), above1[3], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[14][ 3 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[ 5], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[14][ 4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[14][ 5 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[ 6], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[14][ 6 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[ 8], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[14][ 7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 3]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 3]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[14][ 8 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[ 9], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[14][ 9 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[11], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[25]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[25]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[14][10 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[14][11 * N], T13);


    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[12], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[14][12 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[14], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[14][13 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 5]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 5]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[14][14 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[15], 0);

    T13 = _mm_avg_epu8(T00, T01);
    _mm_store_si128((__m128i*)&dstN[14][15 * N], T13);


    // **********************************************************************
    T00 = _mm_insert_epi8(T03, above1[1], 0);
    T01 = _mm_loadu_si128((__m128i*)left1);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[15][ 1 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[ 2], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[15][ 2 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[ 4], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[15][ 3 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[ 5], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[15][ 4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[15][ 5 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[ 6], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[15][ 6 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[ 7], 0);

    T13 = _mm_avg_epu8(T00, T01);
    _mm_store_si128((__m128i*)&dstN[15][ 7 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[ 9], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[15][ 8 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[10], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[15][ 9 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[15][10 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[11], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[15][11 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[12], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[15][12 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[14], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[15][13 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[15], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[15][14 * N], T13);

    _mm_store_si128((__m128i*)&dstN[15][15 * N], T00);


    // **********************************************************************
    T02 = _mm_loadu_si128((__m128i*)(left1+ 1));
    T03 = _mm_loadu_si128((__m128i*)(left1+17));
    T00 = T02;
    T01 = _mm_alignr_epi8(T03, T02,  1);

    _mm_store_si128((__m128i*)&dstN[ 0][ 0 * N], T01);
    _mm_store_si128((__m128i*)&dstN[ 0][15 * N], T03);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 5]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 5]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 6][ 0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 6][ 1 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 6][ 2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 6][ 3 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[25]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[25]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 6][ 4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 6][ 5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 9]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 9]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 5][ 0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 5][ 1 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 5][ 2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 4][ 0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 4][ 1 * N], T13);
    _mm_store_si128((__m128i*)&dstN[ 1][ 0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 3][ 0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[21]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[21]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 2][ 0 * N], T13);


    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02,  2);

    _mm_store_si128((__m128i*)&dstN[ 0][ 1 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 3]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 3]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 6][ 6 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 6][ 7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 6][ 8 * N], T13);
    _mm_store_si128((__m128i*)&dstN[ 5][ 4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 6][ 9 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 6][10 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 6][11 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 5][ 3 * N], T13);


    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 5][ 5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[31]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[31]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 5][ 6 * N], T13);
    _mm_store_si128((__m128i*)&dstN[ 2][ 2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 7]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 7]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 4][ 2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 4][ 3 * N], T13);
    _mm_store_si128((__m128i*)&dstN[ 1][ 1 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 3][ 1 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 3][ 2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 2][ 1 * N], T13);


    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02,  3);

    _mm_store_si128((__m128i*)&dstN[ 0][ 2 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 1]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 1]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 6][12 * N], T13);
    _mm_store_si128((__m128i*)&dstN[ 4][ 4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 6][13 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 6][14 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[16]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[16]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 6][15 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 5][ 7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 5][ 8 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 5][ 9 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 4][ 5 * N], T13);
    _mm_store_si128((__m128i*)&dstN[ 1][ 2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 4][ 6 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 3][ 3 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[21]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[21]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 3][ 4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 2][ 3 * N], T13);


    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02,  4);

    _mm_store_si128((__m128i*)&dstN[ 0][ 3 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 3]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 3]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 5][10 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 5][11 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[21]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[21]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 5][12 * N], T13);
    _mm_store_si128((__m128i*)&dstN[ 4][ 8 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 5][13 * N], T13);
    _mm_store_si128((__m128i*)&dstN[ 2][ 5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 4][ 7 * N], T13);
    _mm_store_si128((__m128i*)&dstN[ 1][ 3 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 3][ 5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 3][ 6 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 9]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 9]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 2][ 4 * N], T13);

    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02,  5);

    _mm_store_si128((__m128i*)&dstN[ 0][ 4 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 7]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 7]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 5][14 * N], T13);

    T13 = _mm_avg_epu8(T00, T01);
    _mm_store_si128((__m128i*)&dstN[ 5][15 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 4][ 9 * N], T13);
    _mm_store_si128((__m128i*)&dstN[ 1][ 4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 4][10 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 4][11 * N], T13);
    _mm_store_si128((__m128i*)&dstN[ 1][ 5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 3][ 7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[25]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[25]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 3][ 8 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 2][ 6 * N], T13);


    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02,  6);

    _mm_store_si128((__m128i*)&dstN[ 0][ 5 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 9]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 9]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 4][12 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 4][13 * N], T13);
    _mm_store_si128((__m128i*)&dstN[ 1][ 6 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 3][ 9 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 3][10 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 2][ 7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[29]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[29]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 2][ 8 * N], T13);


    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02,  7);

    _mm_store_si128((__m128i*)&dstN[ 0][ 6 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 3]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 3]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 4][14 * N], T13);

    T13 = _mm_avg_epu8(T00, T01);
    _mm_store_si128((__m128i*)&dstN[ 4][15 * N], T13);
    _mm_store_si128((__m128i*)&dstN[ 1][ 7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 3][11 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[29]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[29]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 3][12 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 2][ 9 * N], T13);


    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02,  8);

    _mm_store_si128((__m128i*)&dstN[ 0][ 7 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 3][13 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[31]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[31]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 3][14 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 7]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 7]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 2][10 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 2][11 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 1][ 8 * N], T13);


    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02,  9);

    _mm_store_si128((__m128i*)&dstN[ 0][ 8 * N], T01);

    T13 = _mm_avg_epu8(T00, T01);
    _mm_store_si128((__m128i*)&dstN[ 3][15 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 2][12 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 1][ 9 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 1][10 * N], T13);


    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02, 10);

    _mm_store_si128((__m128i*)&dstN[ 0][ 9 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 2][13 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 2][14 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 1][11 * N], T13);


    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02, 11);

    _mm_store_si128((__m128i*)&dstN[ 0][10 * N], T01);

    T13 = _mm_avg_epu8(T00, T01);
    _mm_store_si128((__m128i*)&dstN[ 2][15 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 1][12 * N], T13);


    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02, 12);

    _mm_store_si128((__m128i*)&dstN[ 0][11 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 1][13 * N], T13);


    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02, 13);

    _mm_store_si128((__m128i*)&dstN[ 0][12 * N], T01);
    _mm_store_si128((__m128i*)&dstN[ 1][15 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[ 1][14 * N], T13);

    T01 = _mm_alignr_epi8(T03, T02, 14);
    _mm_store_si128((__m128i*)&dstN[ 0][13 * N], T01);
    T01 = _mm_alignr_epi8(T03, T02, 15);
    _mm_store_si128((__m128i*)&dstN[ 0][14 * N], T01);



    // **********************************************************************
    T02 = _mm_loadu_si128((__m128i*)(above1+ 1));
    T03 = _mm_loadu_si128((__m128i*)(above1+17));
    T00 = T02;
    T01 = _mm_alignr_epi8(T03, T02,  1);

    _mm_store_si128((__m128i*)&dstN[32][ 0 * N], T01);
    _mm_store_si128((__m128i*)&dstN[32][15 * N], T03);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 5]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 5]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[26][ 0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[26][ 1 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[26][ 2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[26][ 3 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[25]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[25]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[26][ 4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[26][ 5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 9]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 9]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[27][ 0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[27][ 1 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[27][ 2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[28][ 0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[28][ 1 * N], T13);
    _mm_store_si128((__m128i*)&dstN[31][ 0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[29][ 0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[21]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[21]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[30][ 0 * N], T13);


    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02,  2);

    _mm_store_si128((__m128i*)&dstN[32][ 1 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 3]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 3]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[26][ 6 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[26][ 7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[26][ 8 * N], T13);
    _mm_store_si128((__m128i*)&dstN[27][ 4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[26][ 9 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[26][10 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[26][11 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[27][ 3 * N], T13);


    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[27][ 5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[31]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[31]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[27][ 6 * N], T13);
    _mm_store_si128((__m128i*)&dstN[30][ 2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 7]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 7]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[28][ 2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[28][ 3 * N], T13);
    _mm_store_si128((__m128i*)&dstN[31][ 1 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[29][ 1 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[29][ 2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[30][ 1 * N], T13);


    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02,  3);

    _mm_store_si128((__m128i*)&dstN[32][ 2 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 1]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 1]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[26][12 * N], T13);
    _mm_store_si128((__m128i*)&dstN[28][ 4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[26][13 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[26][14 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[16]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[16]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[26][15 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[27][ 7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[27][ 8 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[27][ 9 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[28][ 5 * N], T13);
    _mm_store_si128((__m128i*)&dstN[31][ 2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[28][ 6 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[29][ 3 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[21]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[21]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[29][ 4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[30][ 3 * N], T13);


    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02,  4);

    _mm_store_si128((__m128i*)&dstN[32][ 3 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 3]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 3]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[27][10 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[27][11 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[21]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[21]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[27][12 * N], T13);
    _mm_store_si128((__m128i*)&dstN[28][ 8 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[27][13 * N], T13);
    _mm_store_si128((__m128i*)&dstN[30][ 5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[28][ 7 * N], T13);
    _mm_store_si128((__m128i*)&dstN[31][ 3 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[29][ 5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[29][ 6 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 9]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 9]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[30][ 4 * N], T13);

    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02,  5);

    _mm_store_si128((__m128i*)&dstN[32][ 4 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 7]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 7]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[27][14 * N], T13);

    T13 = _mm_avg_epu8(T00, T01);
    _mm_store_si128((__m128i*)&dstN[27][15 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[28][ 9 * N], T13);
    _mm_store_si128((__m128i*)&dstN[31][ 4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[28][10 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[28][11 * N], T13);
    _mm_store_si128((__m128i*)&dstN[31][ 5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[29][ 7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[25]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[25]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[29][ 8 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[30][ 6 * N], T13);


    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02,  6);

    _mm_store_si128((__m128i*)&dstN[32][ 5 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 9]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 9]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[28][12 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[28][13 * N], T13);
    _mm_store_si128((__m128i*)&dstN[31][ 6 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[29][ 9 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[29][10 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[30][ 7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[29]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[29]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[30][ 8 * N], T13);


    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02,  7);

    _mm_store_si128((__m128i*)&dstN[32][ 6 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 3]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 3]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[28][14 * N], T13);

    T13 = _mm_avg_epu8(T00, T01);
    _mm_store_si128((__m128i*)&dstN[28][15 * N], T13);
    _mm_store_si128((__m128i*)&dstN[31][ 7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[29][11 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[29]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[29]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[29][12 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[30][ 9 * N], T13);


    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02,  8);

    _mm_store_si128((__m128i*)&dstN[32][ 7 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[29][13 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[31]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[31]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[29][14 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 7]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 7]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[30][10 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[30][11 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[31][ 8 * N], T13);


    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02,  9);

    _mm_store_si128((__m128i*)&dstN[32][ 8 * N], T01);

    T13 = _mm_avg_epu8(T00, T01);
    _mm_store_si128((__m128i*)&dstN[29][15 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[30][12 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[31][ 9 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[31][10 * N], T13);


    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02, 10);

    _mm_store_si128((__m128i*)&dstN[32][ 9 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[30][13 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[30][14 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[31][11 * N], T13);


    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02, 11);

    _mm_store_si128((__m128i*)&dstN[32][10 * N], T01);

    T13 = _mm_avg_epu8(T00, T01);
    _mm_store_si128((__m128i*)&dstN[30][15 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[31][12 * N], T13);


    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02, 12);

    _mm_store_si128((__m128i*)&dstN[32][11 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[31][13 * N], T13);


    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02, 13);

    _mm_store_si128((__m128i*)&dstN[32][12 * N], T01);
    _mm_store_si128((__m128i*)&dstN[31][15 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[31][14 * N], T13);

    T01 = _mm_alignr_epi8(T03, T02, 14);
    _mm_store_si128((__m128i*)&dstN[32][13 * N], T01);
    T01 = _mm_alignr_epi8(T03, T02, 15);
    _mm_store_si128((__m128i*)&dstN[32][14 * N], T01);


    // **********************************************************************
    T00 = _mm_loadu_si128((__m128i*)(above1+0));
    T01 = _mm_loadu_si128((__m128i*)(above1+1));

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[22][ 0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[22][ 1 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[22][ 2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[22][ 3 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 7]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 7]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[22][ 4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[22][ 5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[21][ 0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[21][ 1 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 5]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 5]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[21][ 2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[20][ 0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[20][ 1 * N], T13);
    _mm_store_si128((__m128i*)&dstN[17][ 0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[19][ 0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[18][ 0 * N], T13);


    // **********************************************************************
    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[6], 0);
    T03 = T00;

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[29]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[29]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[22][ 6 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[22][ 7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[22][ 8 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[22][ 9 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 9]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 9]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[22][10 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[22][11 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[13], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[31]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[31]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[22][12 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[22][13 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[21]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[21]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[22][14 * N], T13);

    T13 = _mm_avg_epu8(T00, T01);
    _mm_store_si128((__m128i*)&dstN[22][15 * N], T13);


    // **********************************************************************
    T00 = _mm_insert_epi8(T03, left1[4], 0);
    T01 = _mm_loadu_si128((__m128i*)(above1));

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[21][ 3 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[21][ 4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[21][ 5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 1]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 1]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[21][ 6 * N], T13);


    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[ 7], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[21][ 7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[21][ 8 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 6]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[21][ 9 * N], T13);


    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[11], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[29]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[29]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[21][10 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[21][11 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[21][12 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[21][13 * N], T13);


    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[14], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[25]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[25]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[21][14 * N], T13);

    T13 = _mm_avg_epu8(T00, T01);
    _mm_store_si128((__m128i*)&dstN[21][15 * N], T13);


    // **********************************************************************
    T00 = _mm_insert_epi8(T03, left1[2], 0);
    T01 = _mm_loadu_si128((__m128i*)(above1));
    T03 = T00;

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[25]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[25]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[20][ 2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[20][ 3 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[19][ 1 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[19][ 2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[18][ 1 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 1]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 1]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[18][ 2 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[ 5], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[31]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[31]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[20][ 4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[20][ 5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 5]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 5]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[20][ 6 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[ 7], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[20][ 7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[20][ 8 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[10], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[20][ 9 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[20][10 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[20][11 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[12], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[20][12 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[20][13 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[15], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[29]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[29]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[20][14 * N], T13);

    T13 = _mm_avg_epu8(T00, T01);
    _mm_store_si128((__m128i*)&dstN[20][15 * N], T13);


    // **********************************************************************
    T01 = T03;
    T00 = _mm_insert_epi8(_mm_slli_si128(T03, 1), left1[4], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[19][ 3 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[19][ 4 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[ 6], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[19][ 5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 9]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 9]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[19][ 6 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[ 8], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[19][ 7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 7]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 7]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[19][ 8 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[ 9], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[19][ 9 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 5]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 5]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[19][10 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[11], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[19][11 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 3]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 3]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[19][12 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[13], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[19][13 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 1]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 1]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[19][14 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[15], 0);

    T13 = _mm_avg_epu8(T00, T01);
    _mm_store_si128((__m128i*)&dstN[19][15 * N], T13);


    // **********************************************************************
    T01 = T03;
    T00 = _mm_insert_epi8(_mm_slli_si128(T03, 1), left1[3], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[18][ 3 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[ 5], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[18][ 4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[18][ 5 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[ 6], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[18][ 6 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[ 8], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[18][ 7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 3]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 3]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[18][ 8 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[ 9], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[18][ 9 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[11], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[25]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[25]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[18][10 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[18][11 * N], T13);


    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[12], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[18][12 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[14], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[18][13 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 5]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 5]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[18][14 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[15], 0);

    T13 = _mm_avg_epu8(T00, T01);
    _mm_store_si128((__m128i*)&dstN[18][15 * N], T13);


    // **********************************************************************
    T00 = _mm_insert_epi8(T03, left1[1], 0);
    T01 = _mm_loadu_si128((__m128i*)above1);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[17][ 1 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[ 2], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[17][ 2 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[ 4], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[17][ 3 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[ 5], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[17][ 4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 4]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[17][ 5 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[ 6], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[17][ 6 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[ 7], 0);

    T13 = _mm_avg_epu8(T00, T01);
    _mm_store_si128((__m128i*)&dstN[17][ 7 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[ 9], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[17][ 8 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[10], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[17][ 9 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 2]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[17][10 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[11], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[ 8]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[17][11 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[12], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[17][12 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[14], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[17][13 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[15], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[17][14 * N], T13);

    _mm_store_si128((__m128i*)&dstN[17][15 * N], T00);

    T01 = _mm_shuffle_epi8(_mm_loadu_si128((__m128i*)left1), _mm_load_si128((__m128i*)tab_angle_2[0]));
    T02 = _mm_loadu_si128((__m128i*)(above1+1));

    _mm_store_si128((__m128i*)&dstN[16][15 * N], T01);
    T00 = _mm_alignr_epi8(T02, T01,  1);
    _mm_store_si128((__m128i*)&dstN[16][14 * N], T00);
    T00 = _mm_alignr_epi8(T02, T01,  2);
    _mm_store_si128((__m128i*)&dstN[16][13 * N], T00);
    T00 = _mm_alignr_epi8(T02, T01,  3);
    _mm_store_si128((__m128i*)&dstN[16][12 * N], T00);
    T00 = _mm_alignr_epi8(T02, T01,  4);
    _mm_store_si128((__m128i*)&dstN[16][11 * N], T00);
    T00 = _mm_alignr_epi8(T02, T01,  5);
    _mm_store_si128((__m128i*)&dstN[16][10 * N], T00);
    T00 = _mm_alignr_epi8(T02, T01,  6);
    _mm_store_si128((__m128i*)&dstN[16][ 9 * N], T00);
    T00 = _mm_alignr_epi8(T02, T01,  7);
    _mm_store_si128((__m128i*)&dstN[16][ 8 * N], T00);
    T00 = _mm_alignr_epi8(T02, T01,  8);
    _mm_store_si128((__m128i*)&dstN[16][ 7 * N], T00);
    T00 = _mm_alignr_epi8(T02, T01,  9);
    _mm_store_si128((__m128i*)&dstN[16][ 6 * N], T00);
    T00 = _mm_alignr_epi8(T02, T01, 10);
    _mm_store_si128((__m128i*)&dstN[16][ 5 * N], T00);
    T00 = _mm_alignr_epi8(T02, T01, 11);
    _mm_store_si128((__m128i*)&dstN[16][ 4 * N], T00);
    T00 = _mm_alignr_epi8(T02, T01, 12);
    _mm_store_si128((__m128i*)&dstN[16][ 3 * N], T00);
    T00 = _mm_alignr_epi8(T02, T01, 13);
    _mm_store_si128((__m128i*)&dstN[16][ 2 * N], T00);
    T00 = _mm_alignr_epi8(T02, T01, 14);
    _mm_store_si128((__m128i*)&dstN[16][ 1 * N], T00);
    T00 = _mm_alignr_epi8(T02, T01, 15);
    _mm_store_si128((__m128i*)&dstN[16][ 0 * N], T00);

#undef N
}

void predIntraAngs32(pixel *dst0, pixel *above0, pixel *left0, pixel *above1, pixel *left1, bool /*bLuma*/)
{
#define N   (32)
    pixel (*dstN)[N*N] = (pixel(*)[N*N])dst0;
    const __m128i c_16 = _mm_set1_epi16(16);
    __m128i T00A, T00B;
    __m128i T01A, T01B;
    __m128i T20A, T20B;
    __m128i T30A, T30B;

    T00A = _mm_loadu_si128((__m128i*)(left0 +   1));
    T00B = _mm_loadu_si128((__m128i*)(left0 +  17));
    T01A = _mm_loadu_si128((__m128i*)(above0 +  1));
    T01B = _mm_loadu_si128((__m128i*)(above0 + 17));

    for(int i = 0; i < N; i++)
    {
        _mm_store_si128((__m128i*)&dstN[ 8][i * N     ], T00A);
        _mm_store_si128((__m128i*)&dstN[ 8][i * N + 16], T00B);
        _mm_store_si128((__m128i*)&dstN[24][i * N     ], T01A);
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
    _mm_store_si128((__m128i*)&dstN[(m) - 2][ (n) * N + (o) * 16], d); \
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

    _mm_store_si128((__m128i*)&dstN[2 - 2][ 0 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][ 0 * N + 1 * 16], T01B);
    _mm_store_si128((__m128i*)&dstN[9 - 2][15 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[9 - 2][15 * N + 1 * 16], T01B);
    AVGS( 9,  7);
    LINE( 9,  0, 0, 1,  2);
    LINE( 9,  1, 0, 1,  4);
    LINE( 9,  2, 0, 1,  6);
    LINE( 9,  3, 0, 1,  8);
    LINE( 9,  4, 0, 1, 10);
    COPY( 8,  1);
    LINE( 9,  5, 0, 1, 12);
    LINE( 9,  6, 0, 1, 14);
    LINE( 9,  8, 0, 1, 18);
    COPY( 7,  1);
    LINE( 9,  9, 0, 1, 20);
    COPY( 8,  3);
    LINE( 9, 10, 0, 1, 22);
    LINE( 9, 11, 0, 1, 24);
    LINE( 9, 12, 0, 1, 26);
    COPY( 3,  0);
    COPY( 6,  1);
    LINE( 9, 13, 0, 1, 28);
    LINE( 9, 14, 0, 1, 30);
    COPY( 8,  5);
    LINE( 4,  0, 0, 1, 21);
    LINE( 5,  0, 0, 1, 17);
    LINE( 6,  0, 0, 1, 13);
    LINE( 7,  0, 0, 1,  9);
    LINE( 7,  2, 0, 1, 27);
    LINE( 8,  0, 0, 1,  5);
    LINE( 8,  2, 0, 1, 15);
    LINE( 8,  4, 0, 1, 25);


    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 +  3));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 19));

    _mm_store_si128((__m128i*)&dstN[2 - 2][ 1 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][ 1 * N + 1 * 16], T01B);
    _mm_store_si128((__m128i*)&dstN[9 - 2][31 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[9 - 2][31 * N + 1 * 16], T01B);
    AVGS( 9, 23);
    LINE( 9, 16, 0, 1,  2);
    COPY( 5,  1);
    LINE( 9, 17, 0, 1,  4);
    COPY( 7,  3);
    LINE( 9, 18, 0, 1,  6);
    LINE( 9, 19, 0, 1,  8);
    COPY( 8,  7);
    LINE( 9, 20, 0, 1, 10);
    COPY( 4,  1);
    LINE( 9, 21, 0, 1, 12);
    LINE( 9, 22, 0, 1, 14);
    LINE( 9, 24, 0, 1, 18);
    COPY( 8,  9);
    LINE( 9, 25, 0, 1, 20);
    COPY( 3,  1);
    COPY( 6,  3);
    LINE( 9, 26, 0, 1, 22);
    COPY( 7,  5);
    LINE( 9, 27, 0, 1, 24);
    LINE( 9, 28, 0, 1, 26);
    LINE( 9, 29, 0, 1, 28);
    COPY( 8, 11);
    LINE( 9, 30, 0, 1, 30);
    LINE( 8,  6, 0, 1,  3);
    LINE( 8,  8, 0, 1, 13);
    COPY( 7,  4);
    LINE( 8, 10, 0, 1, 23);
    LINE( 7,  6, 0, 1, 31);
    COPY( 4,  2);
    LINE( 6,  2, 0, 1,  7);
    LINE( 5,  2, 0, 1, 19);


    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 +  4));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 20));

    _mm_store_si128((__m128i*)&dstN[2 - 2][ 2 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][ 2 * N + 1 * 16], T01B);
    AVGS( 8, 15);
    LINE( 8, 12, 0, 1,  1);
    COPY( 6,  4);
    LINE( 8, 13, 0, 1,  6);
    LINE( 8, 14, 0, 1, 11);
    LINE( 8, 16, 0, 1, 21);
    COPY( 5,  4);
    LINE( 8, 17, 0, 1, 26);
    COPY( 7,  9);
    LINE( 8, 18, 0, 1, 31);
    LINE( 4,  3, 0, 1, 20);
    LINE( 5,  3, 0, 1,  4);
    LINE( 6,  5, 0, 1, 14);
    COPY( 3,  2);
    LINE( 6,  6, 0, 1, 27);
    LINE( 7,  7, 0, 1,  8);
    LINE( 7,  8, 0, 1, 17);


    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 +  5));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 21));

    _mm_store_si128((__m128i*)&dstN[2 - 2][ 3 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][ 3 * N + 1 * 16], T01B);
    LINE( 8, 19, 0, 1,  4);
    LINE( 8, 20, 0, 1,  9);
    COPY( 4,  4);
    LINE( 8, 21, 0, 1, 14);
    LINE( 8, 22, 0, 1, 19);
    LINE( 8, 23, 0, 1, 24);
    LINE( 8, 24, 0, 1, 29);
    LINE( 7, 10, 0, 1,  3);
    LINE( 7, 11, 0, 1, 12);
    LINE( 7, 12, 0, 1, 21);
    COPY( 6,  8);
    LINE( 7, 13, 0, 1, 30);
    COPY( 4,  5);
    LINE( 6,  7, 0, 1,  8);
    COPY( 3,  3);
    LINE( 5,  5, 0, 1,  6);
    LINE( 5,  6, 0, 1, 23);


    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 +  6));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 22));

    _mm_store_si128((__m128i*)&dstN[2 - 2][ 4 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][ 4 * N + 1 * 16], T01B);
    _mm_store_si128((__m128i*)&dstN[8 - 2][31 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[8 - 2][31 * N + 1 * 16], T01B);
    AVGS( 7, 15);
    LINE( 8, 25, 0, 1,  2);
    COPY( 3,  4);
    LINE( 8, 26, 0, 1,  7);
    COPY( 7, 14);
    LINE( 8, 27, 0, 1, 12);
    LINE( 8, 28, 0, 1, 17);
    LINE( 8, 29, 0, 1, 22);
    LINE( 8, 30, 0, 1, 27);
    LINE( 7, 16, 0, 1, 25);
    COPY( 5,  8);
    LINE( 6,  9, 0, 1,  2);
    LINE( 6, 10, 0, 1, 15);
    LINE( 6, 11, 0, 1, 28);
    COPY( 3,  5);
    LINE( 5,  7, 0, 1,  8);
    LINE( 4,  6, 0, 1, 19);


    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 +  7));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 23));

    _mm_store_si128((__m128i*)&dstN[2 - 2][ 5 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][ 5 * N + 1 * 16], T01B);
    LINE( 4,  7, 0, 1,  8);
    LINE( 5,  9, 0, 1, 10);
    LINE( 5, 10, 0, 1, 27);
    LINE( 6, 12, 0, 1,  9);
    LINE( 6, 13, 0, 1, 22);
    COPY( 3,  6);
    LINE( 7, 17, 0, 1,  2);
    LINE( 7, 18, 0, 1, 11);
    LINE( 7, 19, 0, 1, 20);
    LINE( 7, 20, 0, 1, 29);
    COPY( 4,  8);


    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 +  8));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 24));

    _mm_store_si128((__m128i*)&dstN[2 - 2][ 6 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][ 6 * N + 1 * 16], T01B);
    AVGS( 6, 15);
    AVGS( 3,  7);
    LINE( 4,  9, 0, 1, 18);
    LINE( 5, 11, 0, 1, 12);
    LINE( 6, 14, 0, 1,  3);
    LINE( 6, 16, 0, 1, 29);
    COPY( 5, 12);
    LINE( 7, 21, 0, 1,  6);
    LINE( 7, 22, 0, 1, 15);
    LINE( 7, 23, 0, 1, 24);


    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 +  9));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 25));

    _mm_store_si128((__m128i*)&dstN[2 - 2][ 7 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][ 7 * N + 1 * 16], T01B);
    LINE( 7, 24, 0, 1,  1);
    LINE( 7, 25, 0, 1, 10);
    COPY( 6, 17);
    COPY( 3,  8);
    LINE( 7, 26, 0, 1, 19);
    LINE( 7, 27, 0, 1, 28);
    COPY( 4, 11);
    LINE( 6, 18, 0, 1, 23);
    LINE( 5, 13, 0, 1, 14);
    LINE( 5, 14, 0, 1, 31);
    LINE( 4, 10, 0, 1,  7);


    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 + 10));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 26));

    _mm_store_si128((__m128i*)&dstN[2 - 2][ 8 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][ 8 * N + 1 * 16], T01B);
    _mm_store_si128((__m128i*)&dstN[7 - 2][31 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[7 - 2][31 * N + 1 * 16], T01B);
    LINE( 7, 28, 0, 1,  5);
    LINE( 7, 29, 0, 1, 14);
    LINE( 7, 30, 0, 1, 23);
    LINE( 6, 19, 0, 1,  4);
    COPY( 3,  9);
    LINE( 6, 20, 0, 1, 17);
    COPY( 4, 12);
    LINE( 6, 21, 0, 1, 30);
    COPY( 3, 10);
    AVGS( 5, 15);


    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 + 11));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 27));

    _mm_store_si128((__m128i*)&dstN[2 - 2][ 9 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][ 9 * N + 1 * 16], T01B);
    LINE( 4, 13, 0, 1,  6);
    LINE( 4, 14, 0, 1, 27);
    LINE( 5, 16, 0, 1,  1);
    LINE( 5, 17, 0, 1, 18);
    LINE( 6, 22, 0, 1, 11);
    LINE( 6, 23, 0, 1, 24);
    COPY( 3, 11);


    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 + 12));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 28));

    _mm_store_si128((__m128i*)&dstN[2 - 2][10 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][10 * N + 1 * 16], T01B);
    AVGS( 4, 15);
    LINE( 5, 18, 0, 1,  3);
    LINE( 5, 19, 0, 1, 20);
    LINE( 6, 24, 0, 1,  5);
    LINE( 6, 25, 0, 1, 18);
    COPY( 3, 12);
    LINE( 6, 26, 0, 1, 31);

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 + 13));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 29));

    _mm_store_si128((__m128i*)&dstN[2 - 2][11 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][11 * N + 1 * 16], T01B);
    LINE( 5, 20, 0, 1,  5);
    COPY( 4, 16);
    LINE( 5, 21, 0, 1, 22);
    LINE( 6, 27, 0, 1, 12);
    COPY( 3, 13);
    LINE( 6, 28, 0, 1, 25);
    LINE( 4, 17, 0, 1, 26);


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
    LINE( 3, 14, 0, 1,  6);
    COPY( 6, 29);
    LINE( 4, 18, 0, 1, 15);
    LINE( 5, 22, 0, 1,  7);
    LINE( 5, 23, 0, 1, 24);
    LINE( 6, 30, 0, 1, 19);
    

    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 + 15));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 31));

    _mm_store_si128((__m128i*)&dstN[2 - 2][13 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][13 * N + 1 * 16], T01B);
    LINE( 3, 16, 0, 1, 26);
    COPY( 5, 25);
    LINE( 4, 19, 0, 1,  4);
    LINE( 4, 20, 0, 1, 25);
    LINE( 5, 24, 0, 1,  9);


    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 + 16));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 32));

    _mm_store_si128((__m128i*)&dstN[2 - 2][14 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][14 * N + 1 * 16], T01B);
    LINE( 3, 17, 0, 1, 20);
    LINE( 4, 21, 0, 1, 14);
    LINE( 5, 26, 0, 1, 11);
    LINE( 5, 27, 0, 1, 28);


    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 + 17));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 33));

    _mm_store_si128((__m128i*)&dstN[2 - 2][15 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][15 * N + 1 * 16], T01B);
    LINE( 3, 18, 0, 1, 14);
    LINE( 4, 22, 0, 1,  3);
    LINE( 4, 23, 0, 1, 24);
    LINE( 5, 28, 0, 1, 13);
    LINE( 5, 29, 0, 1, 30);


    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 + 18));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 34));

    _mm_store_si128((__m128i*)&dstN[2 - 2][16 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][16 * N + 1 * 16], T01B);
    _mm_store_si128((__m128i*)&dstN[5 - 2][31 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[5 - 2][31 * N + 1 * 16], T01B);
    LINE( 3, 19, 0, 1,  8);
    LINE( 4, 24, 0, 1, 13);
    LINE( 5, 30, 0, 1, 15);


    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 + 19));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 35));

    _mm_store_si128((__m128i*)&dstN[2 - 2][17 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][17 * N + 1 * 16], T01B);
    LINE( 3, 20, 0, 1,  2);
    COPY( 4, 25);
    LINE( 3, 21, 0, 1, 28);
    LINE( 4, 26, 0, 1, 23);


    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 + 20));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 36));

    _mm_store_si128((__m128i*)&dstN[2 - 2][18 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][18 * N + 1 * 16], T01B);
    LINE( 3, 22, 0, 1, 22);
    LINE( 4, 27, 0, 1, 12);


    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 + 21));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 37));

    _mm_store_si128((__m128i*)&dstN[2 - 2][19 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][19 * N + 1 * 16], T01B);
    AVGS( 3, 23);
    LINE( 4, 28, 0, 1,  1);
    LINE( 4, 29, 0, 1, 22);


    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 + 22));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 38));

    _mm_store_si128((__m128i*)&dstN[2 - 2][20 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][20 * N + 1 * 16], T01B);
    _mm_store_si128((__m128i*)&dstN[4 - 2][31 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[4 - 2][31 * N + 1 * 16], T01B);
    LINE( 3, 24, 0, 1, 10);
    LINE( 4, 30, 0, 1, 11);


    // ***********************************************************************
    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 + 23));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 39));

    _mm_store_si128((__m128i*)&dstN[2 - 2][21 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][21 * N + 1 * 16], T01B);
    LINE( 3, 25, 0, 1,  4);
    LINE( 3, 26, 0, 1, 30);

    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 + 24));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 40));

    _mm_store_si128((__m128i*)&dstN[2 - 2][22 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][22 * N + 1 * 16], T01B);
    LINE( 3, 27, 0, 1, 24);

    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 + 25));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 41));

    _mm_store_si128((__m128i*)&dstN[2 - 2][23 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][23 * N + 1 * 16], T01B);
    LINE( 3, 28, 0, 1, 18);

    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 + 26));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 42));

    _mm_store_si128((__m128i*)&dstN[2 - 2][24 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][24 * N + 1 * 16], T01B);
    LINE( 3, 29, 0, 1, 12);

    T00A = T01A;
    T00B = T01B;
    T01A = _mm_loadu_si128((__m128i*)(left1 + 27));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 43));

    _mm_store_si128((__m128i*)&dstN[2 - 2][25 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[2 - 2][25 * N + 1 * 16], T01B);
    _mm_store_si128((__m128i*)&dstN[3 - 2][31 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[3 - 2][31 * N + 1 * 16], T01B);
    LINE( 3, 30, 0, 1,  6);

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

    T00A = _mm_insert_epi8(T30A, above1[ 6], 0);

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
    T00A = _mm_insert_epi8(T30A, above1[ 4], 0);
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
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[ 7], 0);

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
    T00A = _mm_insert_epi8(T30A, above1[ 2], 0);
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
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[ 5], 0);

    LINE(14,  4, 0, 1, 31);
    LINE(14,  5, 0, 1, 18);
    LINE(14,  6, 0, 1,  5);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[ 7], 0);

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
    T00A = _mm_insert_epi8(_mm_slli_si128(T30A, 1), above1[ 4], 0);

    LINE(15,  3, 0, 1, 28);
    LINE(15,  4, 0, 1, 11);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[ 6], 0);

    LINE(15,  5, 0, 1, 26);
    LINE(15,  6, 0, 1,  9);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[ 8], 0);

    LINE(15,  7, 0, 1, 24);
    LINE(15,  8, 0, 1,  7);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[ 9], 0);

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
    T00A = _mm_insert_epi8(_mm_slli_si128(T30A, 1), above1[ 3], 0);

    LINE(16,  3, 0, 1, 12);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[ 5], 0);

    LINE(16,  4, 0, 1, 23);
    LINE(16,  5, 0, 1,  2);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[ 6], 0);

    LINE(16,  6, 0, 1, 13);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[ 8], 0);

    LINE(16,  7, 0, 1, 24);
    LINE(16,  8, 0, 1,  3);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[ 9], 0);

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
    T00A = _mm_insert_epi8(T30A, above1[ 1], 0);
    T00B = T30B;
    T01A = _mm_loadu_si128((__m128i*)(left1 +  0));
    T01B = _mm_loadu_si128((__m128i*)(left1 + 16));

    LINE(17,  1, 0, 1, 12);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[ 2], 0);

    LINE(17,  2, 0, 1, 18);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[ 4], 0);

    LINE(17,  3, 0, 1, 24);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[ 5], 0);

    LINE(17,  4, 0, 1, 30);
    LINE(17,  5, 0, 1,  4);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[ 6], 0);

    LINE(17,  6, 0, 1, 10);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[ 7], 0);

    AVGS(17,  7);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), above1[ 9], 0);

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

    for(int i = 0; i < N / 2; i++)
    {
        _mm_store_si128((__m128i*)&dstN[18 - 2][ i * N + 0 * 16], T00A);
        _mm_store_si128((__m128i*)&dstN[18 - 2][ i * N + 1 * 16], T00B);
        T00B = _mm_alignr_epi8(T00B, T00A, 15);
        T00A = _mm_alignr_epi8(T00A, T01A, 15);
        T01A = _mm_slli_si128(T01A, 1);
    }

    T01A = _mm_loadu_si128((__m128i*)(left1  + 17));
    T01A = _mm_shuffle_epi8(T01A, _mm_load_si128((__m128i*)tab_angle_2[0]));

    _mm_store_si128((__m128i*)&dstN[18 - 2][16 * N + 0 * 16], T00A);
    _mm_store_si128((__m128i*)&dstN[18 - 2][16 * N + 1 * 16], T00B);
    for(int i = 17; i < N; i++)
    {
        T00B = _mm_alignr_epi8(T00B, T00A, 15);
        T00A = _mm_alignr_epi8(T00A, T01A, 15);
        T01A = _mm_slli_si128(T01A, 1);
        _mm_store_si128((__m128i*)&dstN[18 - 2][ i * N + 0 * 16], T00A);
        _mm_store_si128((__m128i*)&dstN[18 - 2][ i * N + 1 * 16], T00B);
    }

    // ***********************************************************************
    T00A = _mm_loadu_si128((__m128i*)(above1 +  1));
    T00B = _mm_loadu_si128((__m128i*)(above1 + 17));
    T01A = _mm_loadu_si128((__m128i*)(above1 +  2));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 18));

    _mm_store_si128((__m128i*)&dstN[34 - 2][ 0 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][ 0 * N + 1 * 16], T01B);
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

    _mm_store_si128((__m128i*)&dstN[34 - 2][ 1 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][ 1 * N + 1 * 16], T01B);
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

    _mm_store_si128((__m128i*)&dstN[34 - 2][ 2 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][ 2 * N + 1 * 16], T01B);
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

    _mm_store_si128((__m128i*)&dstN[34 - 2][ 3 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][ 3 * N + 1 * 16], T01B);
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

    _mm_store_si128((__m128i*)&dstN[34 - 2][ 4 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][ 4 * N + 1 * 16], T01B);
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

    _mm_store_si128((__m128i*)&dstN[34 - 2][ 5 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][ 5 * N + 1 * 16], T01B);
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

    _mm_store_si128((__m128i*)&dstN[34 - 2][ 6 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][ 6 * N + 1 * 16], T01B);
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

    _mm_store_si128((__m128i*)&dstN[34 - 2][ 7 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][ 7 * N + 1 * 16], T01B);
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

    _mm_store_si128((__m128i*)&dstN[34 - 2][ 8 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][ 8 * N + 1 * 16], T01B);
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

    _mm_store_si128((__m128i*)&dstN[34 - 2][ 9 * N + 0 * 16], T01A);
    _mm_store_si128((__m128i*)&dstN[34 - 2][ 9 * N + 1 * 16], T01B);
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

    T00A = _mm_insert_epi8(T30A, left1[ 6], 0);

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
    T00A = _mm_insert_epi8(T30A, left1[ 4], 0);
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
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[ 7], 0);

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
    T00A = _mm_insert_epi8(T30A, left1[ 2], 0);
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
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[ 5], 0);

    LINE(22,  4, 0, 1, 31);
    LINE(22,  5, 0, 1, 18);
    LINE(22,  6, 0, 1,  5);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[ 7], 0);

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
    T00A = _mm_insert_epi8(_mm_slli_si128(T30A, 1), left1[ 4], 0);

    LINE(21,  3, 0, 1, 28);
    LINE(21,  4, 0, 1, 11);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[ 6], 0);

    LINE(21,  5, 0, 1, 26);
    LINE(21,  6, 0, 1,  9);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[ 8], 0);

    LINE(21,  7, 0, 1, 24);
    LINE(21,  8, 0, 1,  7);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[ 9], 0);

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
    T00A = _mm_insert_epi8(_mm_slli_si128(T30A, 1), left1[ 3], 0);

    LINE(20,  3, 0, 1, 12);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[ 5], 0);

    LINE(20,  4, 0, 1, 23);
    LINE(20,  5, 0, 1,  2);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[ 6], 0);

    LINE(20,  6, 0, 1, 13);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[ 8], 0);

    LINE(20,  7, 0, 1, 24);
    LINE(20,  8, 0, 1,  3);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[ 9], 0);

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
    T00A = _mm_insert_epi8(T30A, left1[ 1], 0);
    T00B = T30B;
    T01A = _mm_loadu_si128((__m128i*)(above1 +  0));
    T01B = _mm_loadu_si128((__m128i*)(above1 + 16));

    LINE(19,  1, 0, 1, 12);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[ 2], 0);

    LINE(19,  2, 0, 1, 18);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[ 4], 0);

    LINE(19,  3, 0, 1, 24);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[ 5], 0);

    LINE(19,  4, 0, 1, 30);
    LINE(19,  5, 0, 1,  4);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[ 6], 0);

    LINE(19,  6, 0, 1, 10);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[ 7], 0);

    AVGS(19,  7);

    T01A = T00A;
    T01B = T00B;
    T00B = _mm_alignr_epi8(T00B, T00A, 15);
    T00A = _mm_insert_epi8(_mm_slli_si128(T00A, 1), left1[ 9], 0);

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

}

#include "utils.h"

namespace x265 {
void NAME(Setup_Vec_IPredPrimitives)(EncoderPrimitives& p)
{
    initFileStaticVars();
    p.getIPredDC = predIntraDC;
    p.getIPredPlanar = predIntraPlanar;
    p.getIPredAng = xPredIntraAngBufRef;

    /* VC10 and VC11 both generate bad Win32 code for these functions.
     * They apparently can't deal with register allocation for intrinsic
     * functions this large */
#if HIGH_BIT_DEPTH || (INSTRSET < 5) || (defined(_MSC_VER) && (_MSC_VER > 1500) && !defined(_WIN64) && !defined(__INTEL_COMPILER))
    p.getIPredAngs[0] = xPredIntraAngs<4>;
    p.getIPredAngs[1] = xPredIntraAngs<8>;
    p.getIPredAngs[2] = xPredIntraAngs<16>;
    p.getIPredAngs[3] = xPredIntraAngs<32>;
    p.getIPredAngs[4] = xPredIntraAngs<64>;
#else
    p.getIPredAngs[0] = predIntraAngs4;
    p.getIPredAngs[1] = predIntraAngs8;
#if X86_64
    p.getIPredAngs[2] = xPredIntraAngs<16>;
    p.getIPredAngs[3] = xPredIntraAngs<32>;
#else
    p.getIPredAngs[2] = predIntraAngs16; // uses PINSERB
    p.getIPredAngs[3] = predIntraAngs32;
#endif
    p.getIPredAngs[4] = xPredIntraAngs<64>;
#endif
}

}
