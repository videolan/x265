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
inline void predDCFiltering(pixel* above, pixel* left, pixel* dst, intptr_t dstStride, int width)
{
    int y;
    pixel pixDC = *dst;
    int pixDCx3 = pixDC * 3 + 2;

    // boundary pixels processing
    dst[0] = (pixel)((above[0] + left[0] + 2 * pixDC + 2) >> 2);

    __m128i im1 = _mm_set1_epi16(pixDCx3);
    __m128i im2, im3;

    __m128i pix;
    switch (width)
    {
    case 4:
        pix = _mm_cvtsi32_si128(*(uint32_t*)&above[1]);
        im2 = _mm_unpacklo_epi8(pix, _mm_setzero_si128());
        im2 = _mm_srai_epi16(_mm_add_epi16(im1, im2), 2);
        pix = _mm_packus_epi16(im2, im2);
        *(uint32_t*)&dst[1] = _mm_cvtsi128_si32(pix);
        break;

    case 8:
        pix = _mm_loadl_epi64((__m128i*)&above[1]);
        im2 = _mm_unpacklo_epi8(pix, _mm_setzero_si128());
        im2 = _mm_srai_epi16(_mm_add_epi16(im1, im2), 2);
        pix = _mm_packus_epi16(im2, im2);
        _mm_storel_epi64((__m128i*)&dst[1], pix);
        break;

    case 16:
        pix = _mm_loadu_si128((__m128i*)&above[1]);
        im2 = _mm_unpacklo_epi8(pix, _mm_setzero_si128());
        im3 = _mm_unpackhi_epi8(pix, _mm_setzero_si128());
        im2 = _mm_srai_epi16(_mm_add_epi16(im1, im2), 2);
        im3 = _mm_srai_epi16(_mm_add_epi16(im1, im3), 2);
        pix = _mm_packus_epi16(im2, im3);
        _mm_storeu_si128((__m128i*)&dst[1], pix);
        break;

    case 32:
        pix = _mm_loadu_si128((__m128i*)&above[1]);
        im2 = _mm_unpacklo_epi8(pix, _mm_setzero_si128());
        im3 = _mm_unpackhi_epi8(pix, _mm_setzero_si128());
        im2 = _mm_srai_epi16(_mm_add_epi16(im1, im2), 2);
        im3 = _mm_srai_epi16(_mm_add_epi16(im1, im3), 2);
        pix = _mm_packus_epi16(im2, im3);
        _mm_storeu_si128((__m128i*)&dst[1], pix);

        pix = _mm_loadu_si128((__m128i*)&above[1 + 16]);
        im2 = _mm_unpacklo_epi8(pix, _mm_setzero_si128());
        im3 = _mm_unpackhi_epi8(pix, _mm_setzero_si128());
        im2 = _mm_srai_epi16(_mm_add_epi16(im1, im2), 2);
        im3 = _mm_srai_epi16(_mm_add_epi16(im1, im3), 2);
        pix = _mm_packus_epi16(im2, im3);
        _mm_storeu_si128((__m128i*)&dst[1 + 16], pix);
        break;
    }

    for (y = 1; y < width; y++)
    {
        dst[dstStride] = (pixel)((left[y] + pixDCx3) >> 2);
        dst += dstStride;
    }
}

void intra_pred_dc(pixel* above, pixel* left, pixel* dst, intptr_t dstStride, int width, int filter)
{
    int sum;
    int logSize = g_convertToBit[width] + 2;

    __m128i pixL, pixT, temp;

    switch (width)
    {
    case 4:
        pixL = _mm_cvtsi32_si128(*(uint32_t*)left);
        pixT = _mm_cvtsi32_si128(*(uint32_t*)above);
        pixL = _mm_unpacklo_epi8(pixL, _mm_setzero_si128());
        pixT = _mm_unpacklo_epi8(pixT, _mm_setzero_si128());
        temp = _mm_add_epi16(pixL, pixT);
        sum  = _mm_cvtsi128_si32(_mm_hadd_epi16(_mm_hadd_epi16(temp, temp), temp));
        break;
    case 8:
#if X86_64
        pixL = _mm_cvtsi64_si128(*(uint64_t*)left);
        pixT = _mm_cvtsi64_si128(*(uint64_t*)above);
#else
        pixL = _mm_loadu_si128((__m128i*)left);
        pixT = _mm_loadu_si128((__m128i*)above);
#endif
        pixL = _mm_unpacklo_epi8(pixL, _mm_setzero_si128());
        pixT = _mm_unpacklo_epi8(pixT, _mm_setzero_si128());
        temp = _mm_add_epi16(pixL, pixT);
        sum  = _mm_cvtsi128_si32(_mm_hadd_epi16(_mm_hadd_epi16(_mm_hadd_epi16(temp, temp), temp), temp));
        break;
    case 16:
        pixL = _mm_loadu_si128((__m128i*)left);
        pixT = _mm_loadu_si128((__m128i*)above);
        temp = _mm_sad_epu8(pixL, _mm_setzero_si128());
        temp = _mm_add_epi16(temp, _mm_sad_epu8(pixT, _mm_setzero_si128()));
        sum = _mm_cvtsi128_si32(_mm_add_epi32(_mm_shuffle_epi32(temp, 2), temp));
        break;

    default:
    case 32:
        pixL = _mm_loadu_si128((__m128i*)left);
        temp = _mm_sad_epu8(pixL, _mm_setzero_si128());
        pixL = _mm_loadu_si128((__m128i*)(left + 16));
        temp = _mm_add_epi16(temp, _mm_sad_epu8(pixL, _mm_setzero_si128()));

        pixT = _mm_loadu_si128((__m128i*)above);
        temp = _mm_add_epi16(temp, _mm_sad_epu8(pixT, _mm_setzero_si128()));
        pixT = _mm_loadu_si128((__m128i*)(above + 16));
        temp = _mm_add_epi16(temp, _mm_sad_epu8(pixT, _mm_setzero_si128()));
        sum = _mm_cvtsi128_si32(_mm_add_epi32(_mm_shuffle_epi32(temp, 2), temp));
        break;

    }

    logSize += 1;
    pixel dcVal = (sum + (1 << (logSize - 1))) >> logSize;
    __m128i dcValN = _mm_set1_epi8(dcVal);

    pixel *dst1 = dst;
    switch (width)
    {
    case 4:
        *(uint32_t*)dst1 = _mm_cvtsi128_si32(dcValN);
        dst1 += dstStride;
        *(uint32_t*)dst1 = _mm_cvtsi128_si32(dcValN);
        dst1 += dstStride;
        *(uint32_t*)dst1 = _mm_cvtsi128_si32(dcValN);
        dst1 += dstStride;
        *(uint32_t*)dst1 = _mm_cvtsi128_si32(dcValN);
        break;

    case 8:
        _mm_storel_epi64((__m128i*)dst1, dcValN);
        dst1 += dstStride;
        _mm_storel_epi64((__m128i*)dst1, dcValN);
        dst1 += dstStride;
        _mm_storel_epi64((__m128i*)dst1, dcValN);
        dst1 += dstStride;
        _mm_storel_epi64((__m128i*)dst1, dcValN);
        dst1 += dstStride;
        _mm_storel_epi64((__m128i*)dst1, dcValN);
        dst1 += dstStride;
        _mm_storel_epi64((__m128i*)dst1, dcValN);
        dst1 += dstStride;
        _mm_storel_epi64((__m128i*)dst1, dcValN);
        dst1 += dstStride;
        _mm_storel_epi64((__m128i*)dst1, dcValN);
        break;

    case 16:
        for (int k = 0; k < 16; k += 4)
        {
            _mm_storeu_si128((__m128i*)dst1, dcValN);
            dst1 += dstStride;
            _mm_storeu_si128((__m128i*)dst1, dcValN);
            dst1 += dstStride;
            _mm_storeu_si128((__m128i*)dst1, dcValN);
            dst1 += dstStride;
            _mm_storeu_si128((__m128i*)dst1, dcValN);
            dst1 += dstStride;
        }
        break;

    case 32:
        for (int k = 0; k < 32; k += 2)
        {
            _mm_storeu_si128((__m128i*)dst1, dcValN);
            _mm_storeu_si128((__m128i*)(dst1 + 16), dcValN);
            dst1 += dstStride;
            _mm_storeu_si128((__m128i*)dst1, dcValN);
            _mm_storeu_si128((__m128i*)(dst1 + 16), dcValN);
            dst1 += dstStride;
        }
        break;
    }

    if (filter)
    {
        predDCFiltering(above, left, dst, dstStride, width);
    }
}

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

#define BROADCAST16(a, d, x) { \
    const __m128i mask = _mm_set1_epi16( (((d) * 2) | ((d) * 2 + 1) << 8) ); \
    (x) = _mm_shuffle_epi8((a), mask); \
}

void intra_pred_planar4_sse4(pixel* above, pixel* left, pixel* dst, intptr_t dstStride)
{
    pixel bottomLeft, topRight;

    // Get left and above reference column and row
    __m128i im0 = _mm_cvtsi32_si128(*(int32_t*)above); // topRow
    __m128i v_topRow = _mm_cvtepu8_epi16(im0);

    v_topRow = _mm_shuffle_epi32(v_topRow, 0x44);

    // Prepare intermediate variables used in interpolation
    bottomLeft = left[4];
    topRight   = above[4];

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
        _tmp0 = _mm_cvtsi32_si128((left[(Y)] << 2) + 4); \
        _tmp0 = _mm_shufflelo_epi16(_tmp0, 0); \
        _tmp1 = _mm_cvtsi32_si128((left[((Y)+1)] << 2) + 4); \
        _tmp1 = _mm_shufflelo_epi16(_tmp1, 0); \
        v_horPred = _mm_unpacklo_epi64(_tmp0, _tmp1); \
        _tmp0 = _mm_cvtsi32_si128(topRight - left[(Y)]); \
        _tmp0 = _mm_shufflelo_epi16(_tmp0, 0); \
        _tmp1 = _mm_cvtsi32_si128(topRight - left[((Y)+1)]); \
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

void intra_pred_planar8_sse4(pixel* above, pixel* left, pixel* dst, intptr_t dstStride)
{
    pixel bottomLeft, topRight;

    // Get left and above reference column and row
    __m128i v_topRow = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)above)); // topRow

    __m128i v_leftColumn = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)left));

    // Prepare intermediate variables used in interpolation
    bottomLeft = left[8];
    topRight   = above[8];

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
        BROADCAST16(v_horPred4, (Y), v_horPred); \
        BROADCAST16(v_rightColumn, (Y), v_rightColumnN); \
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

void intra_pred_planar16_sse4(pixel* above, pixel* left, pixel* dst, intptr_t dstStride)
{
    pixel bottomLeft, topRight;
    __m128i v_topRow[2];
    __m128i v_bottomRow[2];

    // Get left and above reference column and row
    __m128i im0 = _mm_loadu_si128((__m128i*)above); // topRow

    v_topRow[0] = _mm_unpacklo_epi8(im0, _mm_setzero_si128());
    v_topRow[1] = _mm_unpackhi_epi8(im0, _mm_setzero_si128());

    // Prepare intermediate variables used in interpolation
    bottomLeft = left[16];
    topRight   = above[16];

    __m128i v_bottomLeft = _mm_set1_epi16(bottomLeft);

    v_bottomRow[0] = _mm_sub_epi16(v_bottomLeft, v_topRow[0]);
    v_bottomRow[1] = _mm_sub_epi16(v_bottomLeft, v_topRow[1]);

    v_topRow[0] = _mm_slli_epi16(v_topRow[0], 4);
    v_topRow[1] = _mm_slli_epi16(v_topRow[1], 4);

    __m128i v_horPred, v_horPredN[2], v_rightColumnN[2];
    __m128i v_im4L, v_im4H;
    __m128i v_im5;

#define COMP_PRED_PLANAR_ROW(Y) { \
        v_horPred = _mm_cvtsi32_si128((left[(Y)] << 4) + 16); \
        v_horPred = _mm_shufflelo_epi16(v_horPred, 0); \
        v_horPred = _mm_shuffle_epi32(v_horPred, 0); \
        __m128i _tmp = _mm_cvtsi32_si128(topRight - left[(Y)]); \
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

void intra_pred_planar32_sse4(pixel* above, pixel* left, pixel* dst, intptr_t dstStride)
{
    pixel bottomLeft, topRight;
    __m128i v_topRow[4];
    __m128i v_bottomRow[4];

    // Get left and above reference column and row
    __m128i im0 = _mm_loadu_si128((__m128i*)&above[0]); // topRow
    __m128i im1 = _mm_loadu_si128((__m128i*)&above[16]); // topRow

    v_topRow[0] = _mm_unpacklo_epi8(im0, _mm_setzero_si128());
    v_topRow[1] = _mm_unpackhi_epi8(im0, _mm_setzero_si128());
    v_topRow[2] = _mm_unpacklo_epi8(im1, _mm_setzero_si128());
    v_topRow[3] = _mm_unpackhi_epi8(im1, _mm_setzero_si128());

    // Prepare intermediate variables used in interpolation
    bottomLeft = left[32];
    topRight   = above[32];

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
        v_horPred = _mm_cvtsi32_si128((left[(Y)] << 5) + 32); \
        v_horPred = _mm_shufflelo_epi16(v_horPred, 0); \
        v_horPred = _mm_shuffle_epi32(v_horPred, 0); \
        __m128i _tmp = _mm_cvtsi32_si128(topRight - left[(Y)]); \
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

void intra_pred_planar64_sse4(pixel* above, pixel* left, pixel* dst, intptr_t dstStride)
{
    pixel bottomLeft, topRight;
    __m128i v_topRow[8];
    __m128i v_bottomRow[8];

    // Get left and above reference column and row
    __m128i im0 = _mm_loadu_si128((__m128i*)&above[0]); // topRow
    __m128i im1 = _mm_loadu_si128((__m128i*)&above[16]); // topRow
    __m128i im2 = _mm_loadu_si128((__m128i*)&above[32]); // topRow
    __m128i im3 = _mm_loadu_si128((__m128i*)&above[48]); // topRow

    v_topRow[0] = _mm_unpacklo_epi8(im0, _mm_setzero_si128());
    v_topRow[1] = _mm_unpackhi_epi8(im0, _mm_setzero_si128());
    v_topRow[2] = _mm_unpacklo_epi8(im1, _mm_setzero_si128());
    v_topRow[3] = _mm_unpackhi_epi8(im1, _mm_setzero_si128());
    v_topRow[4] = _mm_unpacklo_epi8(im2, _mm_setzero_si128());
    v_topRow[5] = _mm_unpackhi_epi8(im2, _mm_setzero_si128());
    v_topRow[6] = _mm_unpacklo_epi8(im3, _mm_setzero_si128());
    v_topRow[7] = _mm_unpackhi_epi8(im3, _mm_setzero_si128());

    // Prepare intermediate variables used in interpolation
    bottomLeft = left[64];
    topRight   = above[64];

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
        v_horPred = _mm_cvtsi32_si128((left[(Y)] << 6) + 64); \
        v_horPred = _mm_shufflelo_epi16(v_horPred, 0); \
        v_horPred = _mm_shuffle_epi32(v_horPred, 0); \
        __m128i _tmp = _mm_cvtsi32_si128(topRight - left[(Y)]); \
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

typedef void intra_pred_planar_t (pixel* above, pixel* left, pixel* dst, intptr_t dstStride);
intra_pred_planar_t *intraPlanarN[] =
{
    intra_pred_planar4_sse4,
    intra_pred_planar8_sse4,
    intra_pred_planar16_sse4,
    intra_pred_planar32_sse4,
    intra_pred_planar64_sse4,
};

void intra_pred_planar(pixel* above, pixel* left, pixel* dst, intptr_t dstStride, int width)
{
    int nLog2Size = g_convertToBit[width] + 2;

    intraPlanarN[nLog2Size - 2](above, left, dst, dstStride);
}

#endif // if !HIGH_BIT_DEPTH

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
    { 0, 10, 11, 13, 1, 0, 10, 11, 3, 2, 0, 10, 5, 4, 2, 0 },    //  8

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
    { 14, 11, 11, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6 },           // 19
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

// See doc/intra/T4.TXT for algorithm details
void predIntraAngs4(pixel *dst, pixel *above0, pixel *left0, pixel *above1, pixel *left1, bool filter)
{
    // avoid warning
    (void)left1;
    (void)above1;

    pixel(*dstN)[4 * 4] = (pixel(*)[4 * 4])dst;

    __m128i T00, T01, T02, T03, T04, T05, T06, T07;
    __m128i T10, T11, T12, T13;
    __m128i T20, T21, T22, T23;
    __m128i T30, T31, T32;
    __m128i R00, R10, R20, R30;
    __m128i R01, R11, R21, R31;

    R00 = _mm_loadu_si128((__m128i*)(left0 + 1));    // [-- -- -- -- -- -- --  -- -08 -07 -06 -05 -04 -03 -02 -01]
    R10 = _mm_srli_si128(R00, 1);                   // [-- -- -- -- -- -- --  --  -- -08 -07 -06 -05 -04 -03 -02]
    R20 = _mm_srli_si128(R00, 2);                   // [-- -- -- -- -- -- --  --  --  -- -08 -07 -06 -05 -04 -03]
    R30 = _mm_srli_si128(R00, 3);                   // [-- -- -- -- -- -- --  --  --  --  -- -08 -07 -06 -05 -04]

    R01 = _mm_loadu_si128((__m128i*)(above0 + 1));   // [-- -- -- -- -- -- --  --  08  07  06  05  04  03  02  01]
    R11 = _mm_srli_si128(R01, 1);                   // [-- -- -- -- -- -- --  --  --  08  07  06  05  04  03  02]
    R21 = _mm_srli_si128(R01, 2);                   // [-- -- -- -- -- -- --  --  --  --  08  07  06  05  04  03]
    R31 = _mm_srli_si128(R01, 3);                   // [-- -- -- -- -- -- --  --  --  --  --  08  07  06  05  04]

    T00 = _mm_unpacklo_epi32(R00, R00);
    T00 = _mm_unpacklo_epi64(T00, T00);
    _mm_store_si128((__m128i*)dstN[8], T00);

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
        dstN[8][0 * 4] = tmp0 & 0xFF;
        dstN[8][1 * 4] = (tmp0 >> 8) & 0xFF;
        dstN[8][2 * 4] = (tmp0 >> 16) & 0xFF;
        dstN[8][3 * 4] = (tmp0 >> 24) & 0xFF;

        tmp0 = _mm_cvtsi128_si32(T01);
        dstN[24][0 * 4] = tmp0 & 0xFF;
        dstN[24][1 * 4] = (tmp0 >> 8) & 0xFF;
        dstN[24][2 * 4] = (tmp0 >> 16) & 0xFF;
        dstN[24][3 * 4] = (tmp0 >> 24) & 0xFF;
    }

    const __m128i c_16 = _mm_set1_epi16(16);

    T00 = _mm_shufflelo_epi16(R10, 0x94);
    T01 = _mm_shufflelo_epi16(R20, 0x94);
    T00 = _mm_unpacklo_epi32(T00, T01);
    _mm_store_si128((__m128i*)dstN[0], T00);

    T00 = _mm_shufflelo_epi16(R11, 0x94);
    T01 = _mm_shufflelo_epi16(R21, 0x94);
    T00 = _mm_unpacklo_epi32(T00, T01);
    _mm_store_si128((__m128i*)dstN[32], T00);

    T00 = _mm_shuffle_epi8(R00, _mm_load_si128((__m128i*)tab_angle_0[0]));
    T01 = _mm_shuffle_epi8(R10, _mm_load_si128((__m128i*)tab_angle_0[0]));
    T02 = _mm_shuffle_epi8(R20, _mm_load_si128((__m128i*)tab_angle_0[0]));
    T03 = _mm_shuffle_epi8(R30, _mm_load_si128((__m128i*)tab_angle_0[0]));
    T04 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[0]));
    T05 = _mm_shuffle_epi8(R11, _mm_load_si128((__m128i*)tab_angle_0[0]));
    T06 = _mm_shuffle_epi8(R21, _mm_load_si128((__m128i*)tab_angle_0[0]));
    T07 = _mm_shuffle_epi8(R31, _mm_load_si128((__m128i*)tab_angle_0[0]));
    T00 = _mm_unpacklo_epi64(T00, T04);
    T01 = _mm_unpacklo_epi64(T01, T05);
    T02 = _mm_unpacklo_epi64(T02, T06);
    T03 = _mm_unpacklo_epi64(T03, T07);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T13 = _mm_maddubs_epi16(T03, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T20 = _mm_unpacklo_epi64(T10, T11);
    T21 = _mm_unpacklo_epi64(T12, T13);
    T20 = _mm_srai_epi16(_mm_add_epi16(T20, c_16), 5);
    T21 = _mm_srai_epi16(_mm_add_epi16(T21, c_16), 5);
    T20 = _mm_packus_epi16(T20, T21);
    _mm_store_si128((__m128i*)dstN[1], T20);
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
    _mm_store_si128((__m128i*)dstN[2], T20);
    T22 = _mm_unpackhi_epi64(T10, T11);
    T23 = _mm_unpackhi_epi64(T12, T13);
    T22 = _mm_srai_epi16(_mm_add_epi16(T22, c_16), 5);
    T23 = _mm_srai_epi16(_mm_add_epi16(T23, c_16), 5);
    T22 = _mm_packus_epi16(T22, T23);
    _mm_store_si128((__m128i*)dstN[30], T22);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T12 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T13 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T20 = _mm_unpacklo_epi64(T10, T11);
    T21 = _mm_unpacklo_epi64(T12, T13);
    T20 = _mm_srai_epi16(_mm_add_epi16(T20, c_16), 5);
    T21 = _mm_srai_epi16(_mm_add_epi16(T21, c_16), 5);
    T20 = _mm_packus_epi16(T20, T21);
    _mm_store_si128((__m128i*)dstN[3], T20);
    T22 = _mm_unpackhi_epi64(T10, T11);
    T23 = _mm_unpackhi_epi64(T12, T13);
    T22 = _mm_srai_epi16(_mm_add_epi16(T22, c_16), 5);
    T23 = _mm_srai_epi16(_mm_add_epi16(T23, c_16), 5);
    T22 = _mm_packus_epi16(T22, T23);
    _mm_store_si128((__m128i*)dstN[29], T22);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T11 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T12 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[7]));
    T13 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T20 = _mm_unpacklo_epi64(T10, T11);
    T21 = _mm_unpacklo_epi64(T12, T13);
    T20 = _mm_srai_epi16(_mm_add_epi16(T20, c_16), 5);
    T21 = _mm_srai_epi16(_mm_add_epi16(T21, c_16), 5);
    T20 = _mm_packus_epi16(T20, T21);
    _mm_store_si128((__m128i*)dstN[4], T20);
    T22 = _mm_unpackhi_epi64(T10, T11);
    T23 = _mm_unpackhi_epi64(T12, T13);
    T22 = _mm_srai_epi16(_mm_add_epi16(T22, c_16), 5);
    T23 = _mm_srai_epi16(_mm_add_epi16(T23, c_16), 5);
    T22 = _mm_packus_epi16(T22, T23);
    _mm_store_si128((__m128i*)dstN[28], T22);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[9]));
    T11 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T12 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T13 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T20 = _mm_unpacklo_epi64(T10, T11);
    T21 = _mm_unpacklo_epi64(T12, T13);
    T20 = _mm_srai_epi16(_mm_add_epi16(T20, c_16), 5);
    T21 = _mm_srai_epi16(_mm_add_epi16(T21, c_16), 5);
    T20 = _mm_packus_epi16(T20, T21);
    _mm_store_si128((__m128i*)dstN[5], T20);
    T22 = _mm_unpackhi_epi64(T10, T11);
    T23 = _mm_unpackhi_epi64(T12, T13);
    T22 = _mm_srai_epi16(_mm_add_epi16(T22, c_16), 5);
    T23 = _mm_srai_epi16(_mm_add_epi16(T23, c_16), 5);
    T22 = _mm_packus_epi16(T22, T23);
    _mm_store_si128((__m128i*)dstN[27], T22);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[5]));
    T11 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T12 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T13 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T20 = _mm_unpacklo_epi64(T10, T11);
    T21 = _mm_unpacklo_epi64(T12, T13);
    T20 = _mm_srai_epi16(_mm_add_epi16(T20, c_16), 5);
    T21 = _mm_srai_epi16(_mm_add_epi16(T21, c_16), 5);
    T20 = _mm_packus_epi16(T20, T21);
    _mm_store_si128((__m128i*)dstN[6], T20);
    T22 = _mm_unpackhi_epi64(T10, T11);
    T23 = _mm_unpackhi_epi64(T12, T13);
    T22 = _mm_srai_epi16(_mm_add_epi16(T22, c_16), 5);
    T23 = _mm_srai_epi16(_mm_add_epi16(T23, c_16), 5);
    T22 = _mm_packus_epi16(T22, T23);
    _mm_store_si128((__m128i*)dstN[26], T22);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T11 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T12 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[6]));
    T13 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T20 = _mm_unpacklo_epi64(T10, T11);
    T21 = _mm_unpacklo_epi64(T12, T13);
    T20 = _mm_srai_epi16(_mm_add_epi16(T20, c_16), 5);
    T21 = _mm_srai_epi16(_mm_add_epi16(T21, c_16), 5);
    T20 = _mm_packus_epi16(T20, T21);
    _mm_store_si128((__m128i*)dstN[7], T20);
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

    T00 = _mm_shuffle_epi8(R00, _mm_load_si128((__m128i*)tab_angle_0[0]));    // [ -- -08 -07 -06 -06 -05 -05 -04 -04 -03 -03 -02 -02 -01 -01  00]
    T04 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[0]));    // [ --  08  07  06  06  05  05  04  04  03  03  02  02  01  01  00]
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
    _mm_store_si128((__m128i*)dstN[9], T20);
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
    T12 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[5]));
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
    T11 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[6]));
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
    T12 = _mm_maddubs_epi16(T30, _mm_load_si128((__m128i*)tab_angle_1[1]));
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
    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[6]));
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

// See doc/intra/T8.TXT for algorithm details
void predIntraAngs8(pixel *dst, pixel *above0, pixel *left0, pixel *above1, pixel *left1, bool filter)
{
#define N   (8)
    pixel(*dstN)[8 * 8] = (pixel(*)[8 * 8])dst;

    __m128i T00, T01, T02, T03;
    __m128i R00, R01;
    __m128i R10, R11;
    __m128i R20, R21;
    __m128i R30, R31;

    R00 = _mm_loadu_si128((__m128i*)(left0 + 1));    // [-16 -15 -14 -13 -12 -11 -10 -09 -08 -07 -06 -05 -04 -03 -02 -01]
    R10 = _mm_loadu_si128((__m128i*)(above0 + 1));   // [ 16  15  14  13  12  11  10  09  08  07  06  05  04  03  02  01]

    T00 = _mm_unpacklo_epi64(R00, R00);
    _mm_store_si128((__m128i*)&dstN[8][0 * N], T00);
    _mm_store_si128((__m128i*)&dstN[8][2 * N], T00);
    _mm_store_si128((__m128i*)&dstN[8][4 * N], T00);
    _mm_store_si128((__m128i*)&dstN[8][6 * N], T00);

    T00 = _mm_unpacklo_epi64(R10, R10);
    _mm_store_si128((__m128i*)&dstN[24][0 * N], T00);
    _mm_store_si128((__m128i*)&dstN[24][2 * N], T00);
    _mm_store_si128((__m128i*)&dstN[24][4 * N], T00);
    _mm_store_si128((__m128i*)&dstN[24][6 * N], T00);

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

        ALIGN_VAR_32(pixel, tmp[2 * N]);
        _mm_store_si128((__m128i*)tmp, T00);
        for (int i = 0; i < N; i++)
        {
            dstN[8][i * N] = tmp[i];
            dstN[24][i * N] = tmp[i + N];
        }
    }

    const __m128i c_16 = _mm_set1_epi16(16);

    R20 = _mm_shuffle_epi8(R00, _mm_load_si128((__m128i*)tab_angle_0[0])); // [-09 -08 -08 -07 -07 -06 -06 -05 -05 -04 -04 -03 -03 -02 -02 -01]
    R21 = _mm_shuffle_epi8(R00, _mm_load_si128((__m128i*)tab_angle_0[9])); // [-10 -09 -09 -08 -08 -07 -07 -06 -06 -05 -05 -04 -04 -03 -03 -02]

    R30 = _mm_shuffle_epi8(R10, _mm_load_si128((__m128i*)tab_angle_0[0]));
    R31 = _mm_shuffle_epi8(R10, _mm_load_si128((__m128i*)tab_angle_0[9]));

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
    _mm_store_si128((__m128i*)&dstN[1][0 * N], T00);
    _mm_store_si128((__m128i*)&dstN[31][0 * N], T01);

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
    _mm_store_si128((__m128i*)&dstN[2][0 * N], T00);
    _mm_store_si128((__m128i*)&dstN[30][0 * N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[3][0 * N], T00);
    _mm_store_si128((__m128i*)&dstN[29][0 * N], T01);

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
    _mm_store_si128((__m128i*)&dstN[4][0 * N], T00);
    _mm_store_si128((__m128i*)&dstN[28][0 * N], T01);

    T00 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[7]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T02 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[7]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[4][2 * N], T00);
    _mm_store_si128((__m128i*)&dstN[28][2 * N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[9]));
    T01 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[9]));
    T03 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[5][0 * N], T00);
    _mm_store_si128((__m128i*)&dstN[27][0 * N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[5][2 * N], T00);
    _mm_store_si128((__m128i*)&dstN[27][2 * N], T01);

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
    _mm_store_si128((__m128i*)&dstN[5][4 * N], T00);
    _mm_store_si128((__m128i*)&dstN[27][4 * N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[5]));
    T01 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[5]));
    T03 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[6][0 * N], T00);
    _mm_store_si128((__m128i*)&dstN[26][0 * N], T01);

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
    _mm_store_si128((__m128i*)&dstN[6][2 * N], T00);
    _mm_store_si128((__m128i*)&dstN[26][2 * N], T01);

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
    _mm_store_si128((__m128i*)&dstN[6][4 * N], T00);
    _mm_store_si128((__m128i*)&dstN[26][4 * N], T01);

    T00 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[3]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T02 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[3]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[6][6 * N], T00);
    _mm_store_si128((__m128i*)&dstN[26][6 * N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T01 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T03 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[7][0 * N], T00);
    _mm_store_si128((__m128i*)&dstN[25][0 * N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[6]));
    T01 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[6]));
    T03 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[7][2 * N], T00);
    _mm_store_si128((__m128i*)&dstN[25][2 * N], T01);

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
    _mm_store_si128((__m128i*)&dstN[7][4 * N], T00);
    _mm_store_si128((__m128i*)&dstN[25][4 * N], T01);

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
    _mm_store_si128((__m128i*)&dstN[7][6 * N], T00);
    _mm_store_si128((__m128i*)&dstN[25][6 * N], T01);

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
    _mm_store_si128((__m128i*)&dstN[2][2 * N], T00);
    _mm_store_si128((__m128i*)&dstN[30][2 * N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[3][2 * N], T00);
    _mm_store_si128((__m128i*)&dstN[29][2 * N], T01);

    T00 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[1]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T02 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[1]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[4][4 * N], T00);
    _mm_store_si128((__m128i*)&dstN[28][4 * N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[31]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[31]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[5][6 * N], T00);
    _mm_store_si128((__m128i*)&dstN[27][6 * N], T01);

    R20 = R21;
    R21 = _mm_shuffle_epi8(R00, _mm_load_si128((__m128i*)tab_angle_0[11]));
    R30 = R31;
    R31 = _mm_shuffle_epi8(R10, _mm_load_si128((__m128i*)tab_angle_0[11]));

    T00 = _mm_loadl_epi64((__m128i*)&dstN[4][5 * N]);
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T02 = _mm_load_si128((__m128i*)&dstN[28][4 * N]);
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T01 = _mm_packus_epi16(T01, T03);
    _mm_storel_epi64((__m128i*)&dstN[4][7 * N], T01);
    _mm_storeh_pi((__m64*)&dstN[28][7 * N], _mm_castsi128_ps(T01));
    T00 = _mm_unpacklo_epi64(T00, T01);
    T01 = _mm_unpackhi_epi64(T02, T01);
    _mm_store_si128((__m128i*)&dstN[1][2 * N], T00);
    _mm_store_si128((__m128i*)&dstN[31][2 * N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T01 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    _mm_storel_epi64((__m128i*)&dstN[4][6 * N], T00);
    _mm_storeh_pi((__m64*)&dstN[28][6 * N], _mm_castsi128_ps(T00));

    T00 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[9]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T02 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[9]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[2][4 * N], T00);
    _mm_store_si128((__m128i*)&dstN[30][4 * N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[21]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[6]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[21]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[6]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[3][4 * N], T00);
    _mm_store_si128((__m128i*)&dstN[29][4 * N], T01);

    R20 = R21;
    R21 = _mm_shuffle_epi8(R00, _mm_load_si128((__m128i*)tab_angle_0[12]));
    R30 = R31;
    R31 = _mm_shuffle_epi8(R10, _mm_load_si128((__m128i*)tab_angle_0[12]));

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[3][6 * N], T00);
    _mm_store_si128((__m128i*)&dstN[29][6 * N], T01);

    T00 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T02 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[1][4 * N], T00);
    _mm_store_si128((__m128i*)&dstN[31][4 * N], T01);

    R20 = R21;
    R21 = _mm_shuffle_epi8(R00, _mm_load_si128((__m128i*)tab_angle_0[13]));
    R30 = R31;
    R31 = _mm_shuffle_epi8(R10, _mm_load_si128((__m128i*)tab_angle_0[13]));

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[2][6 * N], T00);
    _mm_store_si128((__m128i*)&dstN[30][6 * N], T01);

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
    _mm_store_si128((__m128i*)&dstN[1][6 * N], T00);
    _mm_store_si128((__m128i*)&dstN[31][6 * N], T01);

    // ***********************************************************************
    R00 = _mm_loadu_si128((__m128i*)(left0));      // [-15 -14 -13 -12 -11 -10 -09 -08 -07 -06 -05 -04 -03 -02 -01  00]
    R01 = _mm_unpacklo_epi64(R00, R10);             // [ 08  07  06  05  04  03  02  01 -07 -06 -05 -04 -03 -02 -01  00]
    R10 = _mm_loadu_si128((__m128i*)(above0));     // [ 15  14  13  12  11  10  09  08  07  06  05  04  03  02  01  00]

    R20 = _mm_shuffle_epi8(R00, _mm_load_si128((__m128i*)tab_angle_0[0]));
    R30 = _mm_shuffle_epi8(R10, _mm_load_si128((__m128i*)tab_angle_0[0]));

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
    _mm_store_si128((__m128i*)&dstN[9][0 * N], T00);
    _mm_store_si128((__m128i*)&dstN[23][0 * N], T01);

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
    _mm_store_si128((__m128i*)&dstN[9][2 * N], T00);
    _mm_store_si128((__m128i*)&dstN[23][2 * N], T01);

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
    _mm_store_si128((__m128i*)&dstN[9][4 * N], T00);
    _mm_store_si128((__m128i*)&dstN[23][4 * N], T01);

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
    _mm_store_si128((__m128i*)&dstN[9][6 * N], T00);
    _mm_store_si128((__m128i*)&dstN[23][6 * N], T01);

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
    _mm_store_si128((__m128i*)&dstN[10][0 * N], T00);
    _mm_store_si128((__m128i*)&dstN[22][0 * N], T01);

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
    _mm_store_si128((__m128i*)&dstN[10][2 * N], T00);
    _mm_store_si128((__m128i*)&dstN[22][2 * N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[7]));
    T01 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[7]));
    T03 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[10][4 * N], T00);
    _mm_store_si128((__m128i*)&dstN[22][4 * N], T01);

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
    _mm_store_si128((__m128i*)&dstN[11][0 * N], T00);
    _mm_store_si128((__m128i*)&dstN[21][0 * N], T01);

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T01 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[6]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T03 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[6]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[12][0 * N], T00);
//     _mm_storeh_pi((__m64*)&dstN[15][0*N], _mm_castsi128_ps(T00));
    _mm_store_si128((__m128i*)&dstN[20][0 * N], T01);
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
    _mm_store_si128((__m128i*)&dstN[13][0 * N], T00);
    _mm_store_si128((__m128i*)&dstN[19][0 * N], T01);

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
    _mm_store_si128((__m128i*)&dstN[14][0 * N], T00);
    _mm_store_si128((__m128i*)&dstN[18][0 * N], T01);

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
    _mm_store_si128((__m128i*)&dstN[12][2 * N], T00);
    _mm_store_si128((__m128i*)&dstN[20][2 * N], T01);

    T00 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[1]));
    T02 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[1]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_storel_epi64((__m128i*)&dstN[13][2 * N], T00);
    _mm_storeh_pi((__m64*)&dstN[14][2 * N], _mm_castsi128_ps(T00));
    _mm_storel_epi64((__m128i*)&dstN[19][2 * N], T01);
    _mm_storeh_pi((__m64*)&dstN[18][2 * N], _mm_castsi128_ps(T01));

    R21 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[17]));
    R31 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[18]));

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[5]));
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[5]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[11][2 * N], T00);
    _mm_store_si128((__m128i*)&dstN[21][2 * N], T01);

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
    _mm_store_si128((__m128i*)&dstN[11][4 * N], T00);
    _mm_store_si128((__m128i*)&dstN[21][4 * N], T01);

    T00 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[1]));
    T01 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[1]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    _mm_storel_epi64((__m128i*)&dstN[11][6 * N], T00);
    _mm_storeh_pi((__m64*)&dstN[21][6 * N], _mm_castsi128_ps(T00));

    R21 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[19]));
    R31 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[20]));

    T00 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T01 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    _mm_storel_epi64((__m128i*)&dstN[11][7 * N], T00);
    _mm_storeh_pi((__m64*)&dstN[21][7 * N], _mm_castsi128_ps(T00));

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
    _mm_store_si128((__m128i*)&dstN[10][6 * N], T00);
    _mm_store_si128((__m128i*)&dstN[22][6 * N], T01);

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
    _mm_store_si128((__m128i*)&dstN[12][4 * N], T00);
    _mm_store_si128((__m128i*)&dstN[20][4 * N], T01);

    R20 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[25]));
    R30 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[26]));

    T00 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[5]));
    T01 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T02 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[5]));
    T03 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[12][6 * N], T00);
    _mm_store_si128((__m128i*)&dstN[20][6 * N], T01);

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
    _mm_storeu_si128((__m128i*)&dstN[13][3 * N], T00);
    _mm_storeu_si128((__m128i*)&dstN[19][3 * N], T01);

    R20 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[29]));
    R30 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[30]));

    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T01 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[9]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T03 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[9]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_storeu_si128((__m128i*)&dstN[13][5 * N], T00);
    _mm_storeu_si128((__m128i*)&dstN[19][5 * N], T01);

    R21 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[31]));
    R31 = _mm_or_si128(_mm_slli_si128(R30, 2), _mm_cvtsi32_si128((left0[6] << 8) | left0[8]));

    T00 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T01 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    _mm_storel_epi64((__m128i*)&dstN[13][7 * N], T00);
    _mm_storeh_pi((__m64*)&dstN[19][7 * N], _mm_castsi128_ps(T00));

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
    _mm_storeu_si128((__m128i*)&dstN[14][3 * N], T00);
    _mm_storeu_si128((__m128i*)&dstN[18][3 * N], T01);

    R20 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[36]));
    R30 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[37]));

    T00 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T01 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T02 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T03 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_storeu_si128((__m128i*)&dstN[14][5 * N], T00);
    _mm_storeu_si128((__m128i*)&dstN[18][5 * N], T01);

    R21 = _mm_shuffle_epi8(R01, _mm_load_si128((__m128i*)tab_angle_0[38]));
    R31 = _mm_or_si128(_mm_slli_si128(R30, 2), _mm_cvtsi32_si128((left0[6] << 8) | left0[8]));

    T00 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T01 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    _mm_storel_epi64((__m128i*)&dstN[14][7 * N], T00);
    _mm_storeh_pi((__m64*)&dstN[18][7 * N], _mm_castsi128_ps(T00));

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
    T00 = _mm_maddubs_epi16(R20, _mm_load_si128((__m128i*)tab_angle_1[6]));
    T02 = _mm_maddubs_epi16(R30, _mm_load_si128((__m128i*)tab_angle_1[6]));
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
    _mm_store_si128((__m128i*)&dstN[15][0 * N], T00);
    _mm_store_si128((__m128i*)&dstN[17][0 * N], T01);

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
    _mm_store_si128((__m128i*)&dstN[15][2 * N], T00);
    _mm_store_si128((__m128i*)&dstN[17][2 * N], T01);

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
    T01 = _mm_maddubs_epi16(R21, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T03 = _mm_maddubs_epi16(R31, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T00 = _mm_srai_epi16(_mm_add_epi16(T00, c_16), 5);
    T01 = _mm_srai_epi16(_mm_add_epi16(T01, c_16), 5);
    T02 = _mm_srai_epi16(_mm_add_epi16(T02, c_16), 5);
    T03 = _mm_srai_epi16(_mm_add_epi16(T03, c_16), 5);
    T00 = _mm_packus_epi16(T00, T01);
    T01 = _mm_packus_epi16(T02, T03);
    _mm_store_si128((__m128i*)&dstN[15][4 * N], T00);
    _mm_store_si128((__m128i*)&dstN[17][4 * N], T01);

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
    _mm_store_si128((__m128i*)&dstN[15][6 * N], T00);
    _mm_store_si128((__m128i*)&dstN[17][6 * N], T01);

    // 8, 7, 6, 5, 4, 3, 2, 1, 0, -1, -2, -3, -4, -5, -6, -7,
    R00 = _mm_loadl_epi64((__m128i*)left1);
    R01 = _mm_loadl_epi64((__m128i*)&above1[1]);

    //R00 = _mm_setr_epi8(0, -1, -2, -3, -4, -5, -6, -7, 99, 99, 99, 99, 99, 99, 99, 99);
    //R01 = _mm_setr_epi8(1, 2, 3, 4, 5, 6, 7, 8, 99, 99, 99, 99, 99, 99, 99, 99);
    R00 = _mm_shuffle_epi8(R00, _mm_load_si128((__m128i*)tab_angle_0[41]));
    R00 = _mm_unpacklo_epi64(R00, R01);

    R01 = _mm_srli_si128(R00, 1);
    T00 = _mm_unpacklo_epi64(R01, R00);
    _mm_store_si128((__m128i*)&dstN[16][6 * N], T00);

    R00 = _mm_srli_si128(R00, 2);
    R01 = _mm_srli_si128(R01, 2);
    T00 = _mm_unpacklo_epi64(R01, R00);
    _mm_store_si128((__m128i*)&dstN[16][4 * N], T00);

    R00 = _mm_srli_si128(R00, 2);
    R01 = _mm_srli_si128(R01, 2);
    T00 = _mm_unpacklo_epi64(R01, R00);
    _mm_store_si128((__m128i*)&dstN[16][2 * N], T00);

    R00 = _mm_srli_si128(R00, 2);
    R01 = _mm_srli_si128(R01, 2);
    T00 = _mm_unpacklo_epi64(R01, R00);
    _mm_store_si128((__m128i*)&dstN[16][0 * N], T00);

    T00 = _mm_unpacklo_epi64(_mm_loadl_epi64((__m128i*)&left1[2]), _mm_loadl_epi64((__m128i*)&left1[3]));
    T01 = _mm_unpacklo_epi64(_mm_loadl_epi64((__m128i*)&left1[4]), _mm_loadl_epi64((__m128i*)&left1[5]));
    T02 = _mm_unpacklo_epi64(_mm_loadl_epi64((__m128i*)&left1[6]), _mm_loadl_epi64((__m128i*)&left1[7]));
    T03 = _mm_unpacklo_epi64(_mm_loadl_epi64((__m128i*)&left1[8]), _mm_loadl_epi64((__m128i*)&left1[9]));
    _mm_store_si128((__m128i*)&dstN[0][0 * N], T00);
    _mm_store_si128((__m128i*)&dstN[0][2 * N], T01);
    _mm_store_si128((__m128i*)&dstN[0][4 * N], T02);
    _mm_store_si128((__m128i*)&dstN[0][6 * N], T03);

    T00 = _mm_unpacklo_epi64(_mm_loadl_epi64((__m128i*)&above1[2]), _mm_loadl_epi64((__m128i*)&above1[3]));
    T01 = _mm_unpacklo_epi64(_mm_loadl_epi64((__m128i*)&above1[4]), _mm_loadl_epi64((__m128i*)&above1[5]));
    T02 = _mm_unpacklo_epi64(_mm_loadl_epi64((__m128i*)&above1[6]), _mm_loadl_epi64((__m128i*)&above1[7]));
    T03 = _mm_unpacklo_epi64(_mm_loadl_epi64((__m128i*)&above1[8]), _mm_loadl_epi64((__m128i*)&above1[9]));
    _mm_store_si128((__m128i*)&dstN[32][0 * N], T00);
    _mm_store_si128((__m128i*)&dstN[32][2 * N], T01);
    _mm_store_si128((__m128i*)&dstN[32][4 * N], T02);
    _mm_store_si128((__m128i*)&dstN[32][6 * N], T03);

#undef N
}

// See doc/intra/T16.TXT for algorithm details
void predIntraAngs16(pixel *dst0, pixel *above0, pixel *left0, pixel *above1, pixel *left1, bool filter)
{
#define N   (16)
    pixel(*dstN)[N * N] = (pixel(*)[N * N])dst0;

    const __m128i c_16 = _mm_set1_epi16(16);
    __m128i T00, T01, T02, T03;
    __m128i T10, T11, T12, T13, T14;

    T00 = _mm_loadu_si128((__m128i*)(left0 + 1));
    T01 = _mm_loadu_si128((__m128i*)(above0 + 1));

    _mm_store_si128((__m128i*)&dstN[8][0 * N], T00);
    _mm_store_si128((__m128i*)&dstN[8][1 * N], T00);
    _mm_store_si128((__m128i*)&dstN[8][2 * N], T00);
    _mm_store_si128((__m128i*)&dstN[8][3 * N], T00);
    _mm_store_si128((__m128i*)&dstN[8][4 * N], T00);
    _mm_store_si128((__m128i*)&dstN[8][5 * N], T00);
    _mm_store_si128((__m128i*)&dstN[8][6 * N], T00);
    _mm_store_si128((__m128i*)&dstN[8][7 * N], T00);
    _mm_store_si128((__m128i*)&dstN[8][8 * N], T00);
    _mm_store_si128((__m128i*)&dstN[8][9 * N], T00);
    _mm_store_si128((__m128i*)&dstN[8][10 * N], T00);
    _mm_store_si128((__m128i*)&dstN[8][11 * N], T00);
    _mm_store_si128((__m128i*)&dstN[8][12 * N], T00);
    _mm_store_si128((__m128i*)&dstN[8][13 * N], T00);
    _mm_store_si128((__m128i*)&dstN[8][14 * N], T00);
    _mm_store_si128((__m128i*)&dstN[8][15 * N], T00);
    _mm_store_si128((__m128i*)&dstN[24][0 * N], T01);
    _mm_store_si128((__m128i*)&dstN[24][1 * N], T01);
    _mm_store_si128((__m128i*)&dstN[24][2 * N], T01);
    _mm_store_si128((__m128i*)&dstN[24][3 * N], T01);
    _mm_store_si128((__m128i*)&dstN[24][4 * N], T01);
    _mm_store_si128((__m128i*)&dstN[24][5 * N], T01);
    _mm_store_si128((__m128i*)&dstN[24][6 * N], T01);
    _mm_store_si128((__m128i*)&dstN[24][7 * N], T01);
    _mm_store_si128((__m128i*)&dstN[24][8 * N], T01);
    _mm_store_si128((__m128i*)&dstN[24][9 * N], T01);
    _mm_store_si128((__m128i*)&dstN[24][10 * N], T01);
    _mm_store_si128((__m128i*)&dstN[24][11 * N], T01);
    _mm_store_si128((__m128i*)&dstN[24][12 * N], T01);
    _mm_store_si128((__m128i*)&dstN[24][13 * N], T01);
    _mm_store_si128((__m128i*)&dstN[24][14 * N], T01);
    _mm_store_si128((__m128i*)&dstN[24][15 * N], T01);

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

        for (int i = 0; i < N; i++)
        {
            dstN[8][i * N] = tmp0[i];
            dstN[24][i * N] = tmp1[i];
        }
    }

    T00 = _mm_loadu_si128((__m128i*)(left0 + 0));
    T01 = _mm_loadu_si128((__m128i*)(left0 + 1));
    T02 = _mm_loadu_si128((__m128i*)(left0 + 2));

    _mm_store_si128((__m128i*)&dstN[9][15 * N], T00);
    _mm_store_si128((__m128i*)&dstN[7][15 * N], T02);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[7][0 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[9][14 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[7][1 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[9][13 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[6]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[6]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[6]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[7][2 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[9][12 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[7][3 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[9][11 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[7][4 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[9][10 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[7][5 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[9][9 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[7][6 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[9][8 * N], T13);

    T13 = _mm_avg_epu8(T01, T02);
    T14 = _mm_avg_epu8(T01, T00);
    _mm_store_si128((__m128i*)&dstN[7][7 * N], T13);
    _mm_store_si128((__m128i*)&dstN[9][7 * N], T14);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[7][8 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[9][6 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[7][9 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[9][5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[7][10 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[9][4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[7][11 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[9][3 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[7][12 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[9][2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[7][13 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[9][1 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[7][14 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[9][0 * N], T13);

    // **********************************************************************
    T00 = _mm_loadu_si128((__m128i*)(above0 + 0));
    T01 = _mm_loadu_si128((__m128i*)(above0 + 1));
    T02 = _mm_loadu_si128((__m128i*)(above0 + 2));

    _mm_store_si128((__m128i*)&dstN[23][15 * N], T00);
    _mm_store_si128((__m128i*)&dstN[25][15 * N], T02);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[25][0 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[23][14 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[25][1 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[23][13 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[6]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[6]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[6]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[25][2 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[23][12 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[25][3 * N], T13);
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
    _mm_store_si128((__m128i*)&dstN[25][4 * N], T13);
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
    _mm_store_si128((__m128i*)&dstN[25][5 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[23][9 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[25][6 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[23][8 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[16]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[16]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[16]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[25][7 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[23][7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[25][8 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[23][6 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T12 = _mm_maddubs_epi16(T02, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T12 = _mm_srai_epi16(_mm_add_epi16(T12, c_16), 5);
    T13 = _mm_unpacklo_epi16(T11, T12);
    T14 = _mm_unpackhi_epi16(T11, T12);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[25][9 * N], T13);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[23][5 * N], T13);

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
    _mm_store_si128((__m128i*)&dstN[23][4 * N], T13);

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
    _mm_store_si128((__m128i*)&dstN[23][3 * N], T13);

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
    _mm_store_si128((__m128i*)&dstN[23][2 * N], T13);

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
    _mm_store_si128((__m128i*)&dstN[23][1 * N], T13);

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
    _mm_store_si128((__m128i*)&dstN[23][0 * N], T13);

    // **********************************************************************
    T00 = _mm_loadu_si128((__m128i*)(left1 + 0));
    T01 = _mm_loadu_si128((__m128i*)(left1 + 1));

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[10][0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[10][1 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[10][2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[10][3 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[7]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[7]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[10][4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[10][5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[11][0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[11][1 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[5]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[5]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[11][2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[12][0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[6]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[6]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[12][1 * N], T13);
    _mm_store_si128((__m128i*)&dstN[15][0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[13][0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[14][0 * N], T13);

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
    _mm_store_si128((__m128i*)&dstN[10][6 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[10][7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[10][8 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[10][9 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[9]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[9]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[10][10 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[4]));
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
    _mm_store_si128((__m128i*)&dstN[11][3 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[11][4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[11][5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[1]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[1]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[11][6 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[7], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[11][7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[11][8 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[6]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[6]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[11][9 * N], T13);

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

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[2]));
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
    _mm_store_si128((__m128i*)&dstN[12][2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[12][3 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[13][1 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[13][2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[14][1 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[1]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[1]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[14][2 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[5], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[31]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[31]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[12][4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[12][5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[5]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[5]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[12][6 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[7], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[12][7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[12][8 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[10], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[12][9 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[12][10 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[4]));
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
    _mm_store_si128((__m128i*)&dstN[13][3 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[13][4 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[6], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[13][5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[9]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[9]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[13][6 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[8], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[13][7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[7]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[7]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[13][8 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[9], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[13][9 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[5]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[5]));
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

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[3]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[3]));
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

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[1]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[1]));
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
    _mm_store_si128((__m128i*)&dstN[14][3 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[5], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[14][4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[14][5 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[6], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[14][6 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[8], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[14][7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[3]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[3]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[14][8 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[9], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[14][9 * N], T13);

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

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[4]));
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

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[5]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[5]));
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
    _mm_store_si128((__m128i*)&dstN[15][1 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[2], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[15][2 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[4], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[15][3 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[5], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[15][4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[15][5 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[6], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[15][6 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[7], 0);

    T13 = _mm_avg_epu8(T00, T01);
    _mm_store_si128((__m128i*)&dstN[15][7 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[9], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[15][8 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[10], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[15][9 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[15][10 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), above1[11], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[8]));
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
    T02 = _mm_loadu_si128((__m128i*)(left1 + 1));
    T03 = _mm_loadu_si128((__m128i*)(left1 + 17));
    T00 = T02;
    T01 = _mm_alignr_epi8(T03, T02,  1);

    _mm_store_si128((__m128i*)&dstN[0][0 * N], T01);
    _mm_store_si128((__m128i*)&dstN[0][15 * N], T03);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[5]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[5]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[6][0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[6][1 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[6][2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[6][3 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[25]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[25]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[6][4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[6][5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[9]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[9]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[5][0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[5][1 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[5][2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[4][0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[4][1 * N], T13);
    _mm_store_si128((__m128i*)&dstN[1][0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[3][0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[21]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[21]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[2][0 * N], T13);

    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02,  2);

    _mm_store_si128((__m128i*)&dstN[0][1 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[3]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[3]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[6][6 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[6][7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[6][8 * N], T13);
    _mm_store_si128((__m128i*)&dstN[5][4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[6][9 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[6][10 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[6][11 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[5][3 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[5][5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[31]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[31]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[5][6 * N], T13);
    _mm_store_si128((__m128i*)&dstN[2][2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[7]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[7]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[4][2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[4][3 * N], T13);
    _mm_store_si128((__m128i*)&dstN[1][1 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[3][1 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[3][2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[2][1 * N], T13);

    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02,  3);

    _mm_store_si128((__m128i*)&dstN[0][2 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[1]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[1]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[6][12 * N], T13);
    _mm_store_si128((__m128i*)&dstN[4][4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[6]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[6]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[6][13 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[6][14 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[16]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[16]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[6][15 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[5][7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[5][8 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[5][9 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[4][5 * N], T13);
    _mm_store_si128((__m128i*)&dstN[1][2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[4][6 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[3][3 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[21]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[21]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[3][4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[2][3 * N], T13);

    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02,  4);

    _mm_store_si128((__m128i*)&dstN[0][3 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[3]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[3]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[5][10 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[5][11 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[21]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[21]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[5][12 * N], T13);
    _mm_store_si128((__m128i*)&dstN[4][8 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[5][13 * N], T13);
    _mm_store_si128((__m128i*)&dstN[2][5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[4][7 * N], T13);
    _mm_store_si128((__m128i*)&dstN[1][3 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[6]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[6]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[3][5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[3][6 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[9]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[9]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[2][4 * N], T13);

    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02,  5);

    _mm_store_si128((__m128i*)&dstN[0][4 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[7]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[7]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[5][14 * N], T13);

    T13 = _mm_avg_epu8(T00, T01);
    _mm_store_si128((__m128i*)&dstN[5][15 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[4][9 * N], T13);
    _mm_store_si128((__m128i*)&dstN[1][4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[4][10 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[4][11 * N], T13);
    _mm_store_si128((__m128i*)&dstN[1][5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[3][7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[25]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[25]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[3][8 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[2][6 * N], T13);

    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02,  6);

    _mm_store_si128((__m128i*)&dstN[0][5 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[9]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[9]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[4][12 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[4][13 * N], T13);
    _mm_store_si128((__m128i*)&dstN[1][6 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[3][9 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[3][10 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[2][7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[29]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[29]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[2][8 * N], T13);

    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02,  7);

    _mm_store_si128((__m128i*)&dstN[0][6 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[3]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[3]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[4][14 * N], T13);

    T13 = _mm_avg_epu8(T00, T01);
    _mm_store_si128((__m128i*)&dstN[4][15 * N], T13);
    _mm_store_si128((__m128i*)&dstN[1][7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[3][11 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[29]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[29]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[3][12 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[2][9 * N], T13);

    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02,  8);

    _mm_store_si128((__m128i*)&dstN[0][7 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[3][13 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[31]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[31]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[3][14 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[7]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[7]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[2][10 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[2][11 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[1][8 * N], T13);

    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02,  9);

    _mm_store_si128((__m128i*)&dstN[0][8 * N], T01);

    T13 = _mm_avg_epu8(T00, T01);
    _mm_store_si128((__m128i*)&dstN[3][15 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[2][12 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[1][9 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[1][10 * N], T13);

    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02, 10);

    _mm_store_si128((__m128i*)&dstN[0][9 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[6]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[6]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[2][13 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[2][14 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[1][11 * N], T13);

    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02, 11);

    _mm_store_si128((__m128i*)&dstN[0][10 * N], T01);

    T13 = _mm_avg_epu8(T00, T01);
    _mm_store_si128((__m128i*)&dstN[2][15 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[1][12 * N], T13);

    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02, 12);

    _mm_store_si128((__m128i*)&dstN[0][11 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[1][13 * N], T13);

    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02, 13);

    _mm_store_si128((__m128i*)&dstN[0][12 * N], T01);
    _mm_store_si128((__m128i*)&dstN[1][15 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[6]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[6]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[1][14 * N], T13);

    T01 = _mm_alignr_epi8(T03, T02, 14);
    _mm_store_si128((__m128i*)&dstN[0][13 * N], T01);
    T01 = _mm_alignr_epi8(T03, T02, 15);
    _mm_store_si128((__m128i*)&dstN[0][14 * N], T01);

    // **********************************************************************
    T02 = _mm_loadu_si128((__m128i*)(above1 + 1));
    T03 = _mm_loadu_si128((__m128i*)(above1 + 17));
    T00 = T02;
    T01 = _mm_alignr_epi8(T03, T02,  1);

    _mm_store_si128((__m128i*)&dstN[32][0 * N], T01);
    _mm_store_si128((__m128i*)&dstN[32][15 * N], T03);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[5]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[5]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[26][0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[26][1 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[26][2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[26][3 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[25]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[25]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[26][4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[26][5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[9]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[9]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[27][0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[27][1 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[27][2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[28][0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[28][1 * N], T13);
    _mm_store_si128((__m128i*)&dstN[31][0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[29][0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[21]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[21]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[30][0 * N], T13);

    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02,  2);

    _mm_store_si128((__m128i*)&dstN[32][1 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[3]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[3]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[26][6 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[26][7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[26][8 * N], T13);
    _mm_store_si128((__m128i*)&dstN[27][4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[26][9 * N], T13);

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

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[27][3 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[27][5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[31]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[31]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[27][6 * N], T13);
    _mm_store_si128((__m128i*)&dstN[30][2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[7]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[7]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[28][2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[28][3 * N], T13);
    _mm_store_si128((__m128i*)&dstN[31][1 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[29][1 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[29][2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[30][1 * N], T13);

    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02,  3);

    _mm_store_si128((__m128i*)&dstN[32][2 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[1]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[1]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[26][12 * N], T13);
    _mm_store_si128((__m128i*)&dstN[28][4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[6]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[6]));
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

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[27][7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[27][8 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[27][9 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[28][5 * N], T13);
    _mm_store_si128((__m128i*)&dstN[31][2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[28][6 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[29][3 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[21]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[21]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[29][4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[20]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[30][3 * N], T13);

    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02,  4);

    _mm_store_si128((__m128i*)&dstN[32][3 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[3]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[3]));
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
    _mm_store_si128((__m128i*)&dstN[28][8 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[27][13 * N], T13);
    _mm_store_si128((__m128i*)&dstN[30][5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[28][7 * N], T13);
    _mm_store_si128((__m128i*)&dstN[31][3 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[6]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[6]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[29][5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[29][6 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[9]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[9]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[30][4 * N], T13);

    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02,  5);

    _mm_store_si128((__m128i*)&dstN[32][4 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[7]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[7]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[27][14 * N], T13);

    T13 = _mm_avg_epu8(T00, T01);
    _mm_store_si128((__m128i*)&dstN[27][15 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[28][9 * N], T13);
    _mm_store_si128((__m128i*)&dstN[31][4 * N], T13);

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
    _mm_store_si128((__m128i*)&dstN[31][5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[29][7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[25]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[25]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[29][8 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[30][6 * N], T13);

    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02,  6);

    _mm_store_si128((__m128i*)&dstN[32][5 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[9]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[9]));
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
    _mm_store_si128((__m128i*)&dstN[31][6 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[29][9 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[29][10 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[30][7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[29]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[29]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[30][8 * N], T13);

    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02,  7);

    _mm_store_si128((__m128i*)&dstN[32][6 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[3]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[3]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[28][14 * N], T13);

    T13 = _mm_avg_epu8(T00, T01);
    _mm_store_si128((__m128i*)&dstN[28][15 * N], T13);
    _mm_store_si128((__m128i*)&dstN[31][7 * N], T13);

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
    _mm_store_si128((__m128i*)&dstN[30][9 * N], T13);

    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02,  8);

    _mm_store_si128((__m128i*)&dstN[32][7 * N], T01);

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

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[7]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[7]));
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
    _mm_store_si128((__m128i*)&dstN[31][8 * N], T13);

    // **********************************************************************
    T00 = T01;
    T01 = _mm_alignr_epi8(T03, T02,  9);

    _mm_store_si128((__m128i*)&dstN[32][8 * N], T01);

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

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[31][9 * N], T13);

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

    _mm_store_si128((__m128i*)&dstN[32][9 * N], T01);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[6]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[6]));
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

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[6]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[6]));
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
    T00 = _mm_loadu_si128((__m128i*)(above1 + 0));
    T01 = _mm_loadu_si128((__m128i*)(above1 + 1));

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[27]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[22][0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[22][1 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[22][2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[22][3 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[7]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[7]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[22][4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[22][5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[21][0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[21][1 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[5]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[5]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[21][2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[20][0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[6]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[6]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[20][1 * N], T13);
    _mm_store_si128((__m128i*)&dstN[17][0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[19][0 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[18][0 * N], T13);

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
    _mm_store_si128((__m128i*)&dstN[22][6 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[22][7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[22][8 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[22][9 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[9]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[9]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[22][10 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[4]));
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
    _mm_store_si128((__m128i*)&dstN[21][3 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[19]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[21][4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[21][5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[1]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[1]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[21][6 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[7], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[21][7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[15]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[21][8 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[6]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[6]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[21][9 * N], T13);

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

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[2]));
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
    _mm_store_si128((__m128i*)&dstN[20][2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[12]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[20][3 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[19][1 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[19][2 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[18][1 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[1]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[1]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[18][2 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[5], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[31]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[31]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[20][4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[20][5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[5]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[5]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[20][6 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[7], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[20][7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[20][8 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[10], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[20][9 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[17]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[20][10 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[4]));
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
    _mm_store_si128((__m128i*)&dstN[19][3 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[11]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[19][4 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[6], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[26]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[19][5 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[9]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[9]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[19][6 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[8], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[19][7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[7]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[7]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[19][8 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[9], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[19][9 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[5]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[5]));
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

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[3]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[3]));
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

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[1]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[1]));
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
    _mm_store_si128((__m128i*)&dstN[18][3 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[5], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[23]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[18][4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[18][5 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[6], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[13]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[18][6 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[8], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[18][7 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[3]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[3]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[18][8 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[9], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[14]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[18][9 * N], T13);

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

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[4]));
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

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[5]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[5]));
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
    _mm_store_si128((__m128i*)&dstN[17][1 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[2], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[18]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[17][2 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[4], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[24]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[17][3 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[5], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[30]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[17][4 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[4]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[17][5 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[6], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[10]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[17][6 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[7], 0);

    T13 = _mm_avg_epu8(T00, T01);
    _mm_store_si128((__m128i*)&dstN[17][7 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[9], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[22]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[17][8 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[10], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[28]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[17][9 * N], T13);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[2]));
    T10 = _mm_srai_epi16(_mm_add_epi16(T10, c_16), 5);
    T11 = _mm_srai_epi16(_mm_add_epi16(T11, c_16), 5);
    T13 = _mm_unpacklo_epi16(T10, T11);
    T14 = _mm_unpackhi_epi16(T10, T11);
    T13 = _mm_packus_epi16(T13, T14);
    _mm_store_si128((__m128i*)&dstN[17][10 * N], T13);

    T01 = T00;
    T00 = _mm_insert_epi8(_mm_slli_si128(T00, 1), left1[11], 0);

    T10 = _mm_maddubs_epi16(T00, _mm_load_si128((__m128i*)tab_angle_1[8]));
    T11 = _mm_maddubs_epi16(T01, _mm_load_si128((__m128i*)tab_angle_1[8]));
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
    T02 = _mm_loadu_si128((__m128i*)(above1 + 1));

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
    _mm_store_si128((__m128i*)&dstN[16][9 * N], T00);
    T00 = _mm_alignr_epi8(T02, T01,  7);
    _mm_store_si128((__m128i*)&dstN[16][8 * N], T00);
    T00 = _mm_alignr_epi8(T02, T01,  8);
    _mm_store_si128((__m128i*)&dstN[16][7 * N], T00);
    T00 = _mm_alignr_epi8(T02, T01,  9);
    _mm_store_si128((__m128i*)&dstN[16][6 * N], T00);
    T00 = _mm_alignr_epi8(T02, T01, 10);
    _mm_store_si128((__m128i*)&dstN[16][5 * N], T00);
    T00 = _mm_alignr_epi8(T02, T01, 11);
    _mm_store_si128((__m128i*)&dstN[16][4 * N], T00);
    T00 = _mm_alignr_epi8(T02, T01, 12);
    _mm_store_si128((__m128i*)&dstN[16][3 * N], T00);
    T00 = _mm_alignr_epi8(T02, T01, 13);
    _mm_store_si128((__m128i*)&dstN[16][2 * N], T00);
    T00 = _mm_alignr_epi8(T02, T01, 14);
    _mm_store_si128((__m128i*)&dstN[16][1 * N], T00);
    T00 = _mm_alignr_epi8(T02, T01, 15);
    _mm_store_si128((__m128i*)&dstN[16][0 * N], T00);

#undef N
}

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
}

namespace x265 {
void Setup_Vec_IPredPrimitives_sse41(EncoderPrimitives& p)
{
#if HIGH_BIT_DEPTH
    p.intra_pred_planar = p.intra_pred_planar;
#else
    initFileStaticVars();

    p.intra_pred_planar = intra_pred_planar;
    p.intra_pred_dc = intra_pred_dc;

#if defined(__GNUC__) || defined(__INTEL_COMPILER) || (defined(_MSC_VER) && (_MSC_VER == 1500))
    p.intra_pred_allangs[0] = predIntraAngs4;
    p.intra_pred_allangs[1] = predIntraAngs8;
    p.intra_pred_allangs[2] = predIntraAngs16;
    p.intra_pred_allangs[3] = predIntraAngs32;
#elif defined(_MSC_VER) && defined(X86_64)

    /* VC10 and VC11 both generate bad Win32 code for all these functions.
     * They apparently can't deal with register allocation for intrinsic
     * functions this large.  Even Win64 cannot handle 16x16 and 32x32 */
    p.intra_pred_allangs[0] = predIntraAngs4;
    p.intra_pred_allangs[1] = predIntraAngs8;
#endif
#endif /* !HIGH_BIT_DEPTH */
}
}
