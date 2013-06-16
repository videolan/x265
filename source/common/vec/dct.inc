/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Mandar Gurav <mandar@multicorewareinc.com>
 *          Deepthi Devaki Akkoorath <deepthidevaki@multicorewareinc.com>
 *          Mahesh Pittala <mahesh@multicorewareinc.com>
 *          Rajesh Paulraj <rajesh@multicorewareinc.com>
 *          Min Chen <min.chen@multicorewareinc.com>
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

// Vector class versions of macroblock performance primitives

#include "primitives.h"
#include "TLibCommon/TypeDef.h"    // TCoeff, Int, UInt

#include <assert.h>
#include <string.h>
#include <smmintrin.h>
#include <algorithm>

namespace {

/* Used for filter */
#define IF_INTERNAL_PREC 14 ///< Number of bits for internal precision
#define IF_FILTER_PREC    6 ///< Log2 of sum of filter taps
#define IF_INTERNAL_OFFS (1 << (IF_INTERNAL_PREC - 1)) ///< Offset used internally

#if defined(_MSC_VER) && _MSC_VER == 1500 && X86_64
#define VC9_X64 1
#if INSTRSET > 4
#pragma message ("VC9 x64 detected, avoiding SSE4 butterfly intrinsics")
#endif
#endif

void CDECL inversedst(short *tmp, short *block, int shift)  // input tmp, output block
{
    int rnd_factor = 1 << (shift - 1);

    Vec8s tmp0, tmp1;

    tmp0.load_a(tmp);
    tmp1.load_a(tmp + 8);

    Vec4i c0 = extend_low(tmp0);
    Vec4i c1 = extend_high(tmp0);
    Vec4i c2 = extend_low(tmp1);
    Vec4i c3 = extend_high(tmp1);

    Vec4i c0_total = c0 + c2;
    Vec4i c1_total = c2 + c3;
    Vec4i c2_total = c0 - c3;
    Vec4i c3_total = 74 * c1;

    Vec4i c4 = (c0 - c2 + c3);

    Vec4i c0_final = (29 * c0_total + 55 * c1_total + c3_total + rnd_factor) >> shift;
    Vec4i c1_final = (55 * c2_total - 29 * c1_total + c3_total + rnd_factor) >> shift;
    Vec4i c2_final = (74 * c4 + rnd_factor) >> shift;
    Vec4i c3_final = (55 * c0_total + 29 * c2_total - c3_total + rnd_factor) >> shift;

    Vec8s half0 = compress_saturated(c0_final, c1_final);
    Vec8s half1 = compress_saturated(c2_final, c3_final);
    blend8s<0, 4, 8, 12, 1, 5, 9, 13>(half0, half1).store_a(block);
    blend8s<2, 6, 10, 14, 3, 7, 11, 15>(half0, half1).store_a(block + 8);
}

void CDECL partialButterfly16(short *src, short *dst, int shift, int line)
{
    int j;
    int add = 1 << (shift - 1);

    Vec4i g_aiT_zero_row(64, 64, 0, 0);
    Vec4i g_aiT_four_row(83, 36, 0, 0);
    Vec4i g_aiT_eight_row(64, -64, 0, 0);
    Vec4i g_aiT_twelve_row(36, -83, 0, 0);

    Vec4i g_aiT_two_row(89, 75, 50, 18);
    Vec4i g_aiT_six_row(75, -18, -89, -50);
    Vec4i g_aiT_ten_row(50, -89, 18, 75);
    Vec4i g_aiT_fourteen_row(18, -50, 75, -89);

    Vec4i g_aiT_one_row_first_half(90, 87, 80, 70);
    Vec4i g_aiT_one_row_second_half(57, 43, 25,  9);
    Vec4i g_aiT_three_row_first_half(87, 57,  9, -43);
    Vec4i g_aiT_three_row_second_half(-80, -90, -70, -25);
    Vec4i g_aiT_five_row_first_half(80,  9, -70, -87);
    Vec4i g_aiT_five_row_second_half(-25, 57, 90, 43);
    Vec4i g_aiT_seven_row_first_half(70, -43, -87,  9);
    Vec4i g_aiT_seven_row_second_half(90, 25, -80, -57);
    Vec4i g_aiT_nine_row_first_half(57, -80, -25, 90);
    Vec4i g_aiT_nine_row_second_half(-9, -87, 43, 70);
    Vec4i g_aiT_eleven_row_first_half(43, -90, 57, 25);
    Vec4i g_aiT_eleven_row_second_half(-87, 70,  9, -80);
    Vec4i g_aiT_thirteen_row_first_half(25, -70, 90, -80);
    Vec4i g_aiT_thirteen_row_second_half(43,  9, -57, 87);
    Vec4i g_aiT_fifteen_row_first_half(9, -25, 43, -57);
    Vec4i g_aiT_fifteen_row_second_half(70, -80, 87, -90);

    for (j = 0; j < line; j++)
    {
        Vec8s tmp1, tmp2;
        tmp1.load(src);
        Vec4i tmp1_first_half = extend_low(tmp1);
        Vec4i tmp1_second_half = extend_high(tmp1);

        tmp2.load(src + 8);
        Vec4i tmp2_first_half_tmp = extend_low(tmp2);
        Vec4i tmp2_second_half_tmp = extend_high(tmp2);
        Vec4i tmp2_first_half = permute4i<3, 2, 1, 0>(tmp2_second_half_tmp);
        Vec4i tmp2_second_half = permute4i<3, 2, 1, 0>(tmp2_first_half_tmp);

        Vec4i E_first_half = tmp1_first_half + tmp2_first_half;
        Vec4i E_second_half_tmp = tmp1_second_half + tmp2_second_half;
        Vec4i O_first_half = tmp1_first_half - tmp2_first_half;
        Vec4i O_second_half = tmp1_second_half - tmp2_second_half;

        Vec4i E_second_half = permute4i<3, 2, 1, 0>(E_second_half_tmp);

        Vec4i EE = E_first_half + E_second_half;
        Vec4i EO = E_first_half - E_second_half;

        Vec4i EE_first_half = permute4i<0, 1, -1, -1>(EE);
        Vec4i EE_second_half = permute4i<3, 2, -1, -1>(EE);

        Vec4i EEE = EE_first_half + EE_second_half;
        Vec4i EEO = EE_first_half - EE_second_half;

        Vec4i dst_tmp0 = g_aiT_zero_row * EEE;
        Vec4i dst_tmp4 = g_aiT_four_row * EEO;
        Vec4i dst_tmp8 = g_aiT_eight_row * EEE;
        Vec4i dst_tmp12 = g_aiT_twelve_row * EEO;

        int dst_zero = horizontal_add(dst_tmp0);
        int dst_four = horizontal_add(dst_tmp4);
        int dst_eight = horizontal_add(dst_tmp8);
        int dst_twelve = horizontal_add(dst_tmp12);

        Vec4i dst_0_8_4_12(dst_zero, dst_eight, dst_four, dst_twelve);

        Vec4i dst_result = dst_0_8_4_12 + add;
        Vec4i dst_shift_result = dst_result >> shift;

        dst[0] = dst_shift_result[0];
        dst[8 * line] = dst_shift_result[1];
        dst[4 * line] = dst_shift_result[2];
        dst[12 * line] = dst_shift_result[3];

        Vec4i dst_tmp2 = g_aiT_two_row * EO;
        Vec4i dst_tmp6 = g_aiT_six_row * EO;
        Vec4i dst_tmp10 = g_aiT_ten_row * EO;
        Vec4i dst_tmp14 = g_aiT_fourteen_row * EO;

        int dst_two = horizontal_add(dst_tmp2);
        int dst_six = horizontal_add(dst_tmp6);
        int dst_ten = horizontal_add(dst_tmp10);
        int dst_fourteen = horizontal_add(dst_tmp14);

        Vec4i dst_2_6_10_14(dst_two, dst_six, dst_ten, dst_fourteen);
        dst_2_6_10_14 = dst_2_6_10_14 + add;
        dst_2_6_10_14 = dst_2_6_10_14 >> shift;

        dst[2 * line] = dst_2_6_10_14[0];
        dst[6 * line] = dst_2_6_10_14[1];
        dst[10 * line] = dst_2_6_10_14[2];
        dst[14 * line] = dst_2_6_10_14[3];

        Vec4i dst_tmp1_first_half = g_aiT_one_row_first_half * O_first_half;
        Vec4i dst_tmp1_second_half = g_aiT_one_row_second_half * O_second_half;
        Vec4i dst_tmp3_first_half = g_aiT_three_row_first_half * O_first_half;
        Vec4i dst_tmp3_second_half = g_aiT_three_row_second_half * O_second_half;
        Vec4i dst_tmp5_first_half = g_aiT_five_row_first_half * O_first_half;
        Vec4i dst_tmp5_second_half = g_aiT_five_row_second_half * O_second_half;
        Vec4i dst_tmp7_first_half = g_aiT_seven_row_first_half * O_first_half;
        Vec4i dst_tmp7_second_half = g_aiT_seven_row_second_half * O_second_half;
        Vec4i dst_tmp9_first_half = g_aiT_nine_row_first_half * O_first_half;
        Vec4i dst_tmp9_second_half = g_aiT_nine_row_second_half * O_second_half;
        Vec4i dst_tmp11_first_half = g_aiT_eleven_row_first_half * O_first_half;
        Vec4i dst_tmp11_second_half = g_aiT_eleven_row_second_half * O_second_half;
        Vec4i dst_tmp13_first_half = g_aiT_thirteen_row_first_half * O_first_half;
        Vec4i dst_tmp13_second_half = g_aiT_thirteen_row_second_half * O_second_half;
        Vec4i dst_tmp15_first_half = g_aiT_fifteen_row_first_half * O_first_half;
        Vec4i dst_tmp15_second_half = g_aiT_fifteen_row_second_half * O_second_half;

        int dst_one = horizontal_add(dst_tmp1_first_half) + horizontal_add(dst_tmp1_second_half);
        int dst_three = horizontal_add(dst_tmp3_first_half) + horizontal_add(dst_tmp3_second_half);
        int dst_five = horizontal_add(dst_tmp5_first_half) + horizontal_add(dst_tmp5_second_half);
        int dst_seven = horizontal_add(dst_tmp7_first_half) + horizontal_add(dst_tmp7_second_half);
        int dst_nine = horizontal_add(dst_tmp9_first_half) + horizontal_add(dst_tmp9_second_half);
        int dst_eleven = horizontal_add(dst_tmp11_first_half) + horizontal_add(dst_tmp11_second_half);
        int dst_thirteen = horizontal_add(dst_tmp13_first_half) + horizontal_add(dst_tmp13_second_half);
        int dst_fifteen = horizontal_add(dst_tmp15_first_half) + horizontal_add(dst_tmp15_second_half);

        Vec4i dst_1_3_5_7(dst_one, dst_three, dst_five, dst_seven);
        dst_1_3_5_7 = dst_1_3_5_7 + add;
        dst_1_3_5_7 = dst_1_3_5_7 >> shift;

        Vec4i dst_9_11_13_15(dst_nine, dst_eleven, dst_thirteen, dst_fifteen);
        dst_9_11_13_15 = dst_9_11_13_15 + add;
        dst_9_11_13_15 = dst_9_11_13_15 >> shift;

        dst[1 * line] = dst_1_3_5_7[0];
        dst[3 * line] = dst_1_3_5_7[1];
        dst[5 * line] = dst_1_3_5_7[2];
        dst[7 * line] = dst_1_3_5_7[3];
        dst[9 * line] = dst_9_11_13_15[0];
        dst[11 * line] = dst_9_11_13_15[1];
        dst[13 * line] = dst_9_11_13_15[2];
        dst[15 * line] = dst_9_11_13_15[3];

        src += 16;
        dst++;
    }
}

#if INSTRSET <= 4 || defined(VC9_X64) //partialButterfly8 vector code

void CDECL partialButterfly8(short *src, short *dst, int shift, int line)
{
    int j;
    int add = 1 << (shift - 1);

    Vec4i g_aiT8_zero_row(64, 64, 0, 0);
    Vec4i g_aiT8_four_row(64, -64, 0, 0);
    Vec4i g_aiT8_two_row(83, 36, 0, 0);
    Vec4i g_aiT8_six_row(36, -83, 0, 0);

    Vec4i g_aiT8_one_row(89, 75, 50, 18);
    Vec4i g_aiT8_three_row(75, -18, -89, -50);
    Vec4i g_aiT8_five_row(50, -89, 18, 75);
    Vec4i g_aiT8_seven_row(18, -50, 75, -89);

    for (j = 0; j < line; j++)
    {
        Vec8s tmp;
        tmp.load(src);

        Vec4i E_first_half = extend_low(tmp);
        Vec4i E_second_half = extend_high(tmp);
        E_second_half = permute4i<3, 2, 1, 0>(E_second_half);

        Vec4i E = E_first_half + E_second_half;
        Vec4i O = E_first_half - E_second_half;

        Vec4i EE_first_half = permute4i<0, 1, -1, -1>(E);
        Vec4i EE_second_half = permute4i<3, 2, -1, -1>(E);
        Vec4i EE = EE_first_half + EE_second_half;
        Vec4i EO = EE_first_half - EE_second_half;

        int dst0 = ((horizontal_add(g_aiT8_zero_row * EE)) + add) >> shift;
        int dst4 = ((horizontal_add(g_aiT8_four_row * EE)) + add) >> shift;
        int dst2 = ((horizontal_add(g_aiT8_two_row * EO)) + add) >> shift;
        int dst6 = ((horizontal_add(g_aiT8_six_row * EO)) + add) >> shift;

        dst[0] = dst0;
        dst[4 * line] = dst4;
        dst[2 * line] = dst2;
        dst[6 * line] = dst6;

        int dst1 = ((horizontal_add(g_aiT8_one_row * O)) + add) >> shift;
        int dst3 = ((horizontal_add(g_aiT8_three_row * O)) + add) >> shift;
        int dst5 = ((horizontal_add(g_aiT8_five_row * O)) + add) >> shift;
        int dst7 = ((horizontal_add(g_aiT8_seven_row * O)) + add) >> shift;

        dst[line] = dst1;
        dst[3 * line] = dst3;
        dst[5 * line] = dst5;
        dst[7 * line] = dst7;

        src += 8;
        dst++;
    }
}

#else //partialButterfly8 intrinsic code

void CDECL partialButterfly8(short *src, short *dst, int shift, int /* line */)
{
    int add = 1 << (shift - 1);
    __m128i c32_add   = _mm_set1_epi32(add);

    __m128i c32_89_75_50_18 = _mm_set_epi32(18, 50, 75, 89);
    __m128i c32_75_n18_n89_n50 = _mm_set_epi32(-50, -89, -18, 75);
    __m128i c32_50_n89_18_75 = _mm_set_epi32(75, 18, -89, 50);
    __m128i c32_18_n50_75_n89 = _mm_set_epi32(-89, 75, -50, 18);

    __m128i src_tmp0 = _mm_load_si128((const __m128i*)src);
    __m128i T20 = _mm_srai_epi32(_mm_unpacklo_epi16(src_tmp0, src_tmp0), 16);
    __m128i T21 = _mm_srai_epi32(_mm_unpackhi_epi16(src_tmp0, src_tmp0), 16);

    T21 = _mm_shuffle_epi32(T21, 0x1b);

    __m128i E = _mm_add_epi32(T20, T21);
    __m128i O = _mm_sub_epi32(T20, T21);

    int EE0_tmp = _mm_extract_epi32(E, 0);
    int EE1_tmp = _mm_extract_epi32(E, 1);
    int EE2_tmp = _mm_extract_epi32(E, 2);
    int EE3_tmp = _mm_extract_epi32(E, 3);

    int EE0 = EE0_tmp + EE3_tmp;
    int EE1 = EE1_tmp + EE2_tmp;
    int EO0 = EE0_tmp - EE3_tmp;
    int EO1 = EE1_tmp - EE2_tmp;

    int dst0_tmp1 = (EE0 << 6);
    int dst0_tmp2 = (EE1 << 6);

    int dst0 = dst0_tmp1 + dst0_tmp2;
    int dst32 = dst0_tmp1 - dst0_tmp2;
    int dst16 = 83 * EO0 + 36 * EO1;
    int dst48 = 36 * EO0 - 83 * EO1;

    __m128i c32_89_75_50_18_O = _mm_mullo_epi32(c32_89_75_50_18, O);
    __m128i c32_75_n18_n89_n50_O = _mm_mullo_epi32(c32_75_n18_n89_n50, O);
    __m128i c32_50_n89_18_75_O = _mm_mullo_epi32(c32_50_n89_18_75, O);
    __m128i c32_18_n50_75_n89_O = _mm_mullo_epi32(c32_18_n50_75_n89, O);

    c32_89_75_50_18_O = _mm_add_epi32(c32_89_75_50_18_O, _mm_srli_si128(c32_89_75_50_18_O, 8));
    c32_89_75_50_18_O = _mm_add_epi32(c32_89_75_50_18_O, _mm_srli_si128(c32_89_75_50_18_O, 4));
    int dst8 = _mm_cvtsi128_si32(c32_89_75_50_18_O);

    c32_75_n18_n89_n50_O = _mm_add_epi32(c32_75_n18_n89_n50_O, _mm_srli_si128(c32_75_n18_n89_n50_O, 8));
    c32_75_n18_n89_n50_O = _mm_add_epi32(c32_75_n18_n89_n50_O, _mm_srli_si128(c32_75_n18_n89_n50_O, 4));
    int dst24 = _mm_cvtsi128_si32(c32_75_n18_n89_n50_O);

    c32_50_n89_18_75_O = _mm_add_epi32(c32_50_n89_18_75_O, _mm_srli_si128(c32_50_n89_18_75_O, 8));
    c32_50_n89_18_75_O = _mm_add_epi32(c32_50_n89_18_75_O, _mm_srli_si128(c32_50_n89_18_75_O, 4));
    int dst40 = _mm_cvtsi128_si32(c32_50_n89_18_75_O);

    c32_18_n50_75_n89_O = _mm_add_epi32(c32_18_n50_75_n89_O, _mm_srli_si128(c32_18_n50_75_n89_O, 8));
    c32_18_n50_75_n89_O  = _mm_add_epi32(c32_18_n50_75_n89_O, _mm_srli_si128(c32_18_n50_75_n89_O, 4));
    int dst56 = _mm_cvtsi128_si32(c32_18_n50_75_n89_O);

    src += 8;

    src_tmp0 = _mm_load_si128((const __m128i*)src);
    T20 = _mm_srai_epi32(_mm_unpacklo_epi16(src_tmp0, src_tmp0), 16);
    T21 = _mm_srai_epi32(_mm_unpackhi_epi16(src_tmp0, src_tmp0), 16);
    T21 = _mm_shuffle_epi32(T21, 0x1b);

    E = _mm_add_epi32(T20, T21);
    O = _mm_sub_epi32(T20, T21);

    EE0_tmp = _mm_extract_epi32(E, 0);
    EE1_tmp = _mm_extract_epi32(E, 1);
    EE2_tmp = _mm_extract_epi32(E, 2);
    EE3_tmp = _mm_extract_epi32(E, 3);

    EE0 = EE0_tmp + EE3_tmp;
    EE1 = EE1_tmp + EE2_tmp;
    EO0 = EE0_tmp - EE3_tmp;
    EO1 = EE1_tmp - EE2_tmp;

    dst0_tmp1 = (EE0 << 6);
    dst0_tmp2 = (EE1 << 6);

    int dst1 =  dst0_tmp1 + dst0_tmp2;
    int dst33 = dst0_tmp1 - dst0_tmp2;
    int dst17 = 83 * EO0 + 36 * EO1;
    int dst49 = 36 * EO0 - 83 * EO1;

    c32_89_75_50_18_O = _mm_mullo_epi32(c32_89_75_50_18, O);
    c32_75_n18_n89_n50_O = _mm_mullo_epi32(c32_75_n18_n89_n50, O);
    c32_50_n89_18_75_O = _mm_mullo_epi32(c32_50_n89_18_75, O);
    c32_18_n50_75_n89_O = _mm_mullo_epi32(c32_18_n50_75_n89, O);

    c32_89_75_50_18_O = _mm_add_epi32(c32_89_75_50_18_O, _mm_srli_si128(c32_89_75_50_18_O, 8));
    c32_89_75_50_18_O = _mm_add_epi32(c32_89_75_50_18_O, _mm_srli_si128(c32_89_75_50_18_O, 4));
    int dst9 = _mm_cvtsi128_si32(c32_89_75_50_18_O);

    c32_75_n18_n89_n50_O = _mm_add_epi32(c32_75_n18_n89_n50_O, _mm_srli_si128(c32_75_n18_n89_n50_O, 8));
    c32_75_n18_n89_n50_O = _mm_add_epi32(c32_75_n18_n89_n50_O, _mm_srli_si128(c32_75_n18_n89_n50_O, 4));
    int dst25 = _mm_cvtsi128_si32(c32_75_n18_n89_n50_O);

    c32_50_n89_18_75_O = _mm_add_epi32(c32_50_n89_18_75_O, _mm_srli_si128(c32_50_n89_18_75_O, 8));
    c32_50_n89_18_75_O = _mm_add_epi32(c32_50_n89_18_75_O, _mm_srli_si128(c32_50_n89_18_75_O, 4));
    int dst41 = _mm_cvtsi128_si32(c32_50_n89_18_75_O);

    c32_18_n50_75_n89_O = _mm_add_epi32(c32_18_n50_75_n89_O, _mm_srli_si128(c32_18_n50_75_n89_O, 8));
    c32_18_n50_75_n89_O  = _mm_add_epi32(c32_18_n50_75_n89_O, _mm_srli_si128(c32_18_n50_75_n89_O, 4));
    int dst57 = _mm_cvtsi128_si32(c32_18_n50_75_n89_O);

    src += 8;

    src_tmp0 = _mm_load_si128((const __m128i*)src);
    T20 = _mm_srai_epi32(_mm_unpacklo_epi16(src_tmp0, src_tmp0), 16);
    T21 = _mm_srai_epi32(_mm_unpackhi_epi16(src_tmp0, src_tmp0), 16);
    T21 = _mm_shuffle_epi32(T21, 0x1b);

    E = _mm_add_epi32(T20, T21);
    O = _mm_sub_epi32(T20, T21);

    EE0_tmp = _mm_extract_epi32(E, 0);
    EE1_tmp = _mm_extract_epi32(E, 1);
    EE2_tmp = _mm_extract_epi32(E, 2);
    EE3_tmp = _mm_extract_epi32(E, 3);

    EE0 = EE0_tmp + EE3_tmp;
    EE1 = EE1_tmp + EE2_tmp;
    EO0 = EE0_tmp - EE3_tmp;
    EO1 = EE1_tmp - EE2_tmp;

    dst0_tmp1 = (EE0 << 6);
    dst0_tmp2 = (EE1 << 6);

    int dst2 =  dst0_tmp1 + dst0_tmp2;
    int dst34 = dst0_tmp1 - dst0_tmp2;
    int dst18 = 83 * EO0 + 36 * EO1;
    int dst50 = 36 * EO0 - 83 * EO1;

    c32_89_75_50_18_O = _mm_mullo_epi32(c32_89_75_50_18, O);
    c32_75_n18_n89_n50_O = _mm_mullo_epi32(c32_75_n18_n89_n50, O);
    c32_50_n89_18_75_O = _mm_mullo_epi32(c32_50_n89_18_75, O);
    c32_18_n50_75_n89_O = _mm_mullo_epi32(c32_18_n50_75_n89, O);

    c32_89_75_50_18_O = _mm_add_epi32(c32_89_75_50_18_O, _mm_srli_si128(c32_89_75_50_18_O, 8));
    c32_89_75_50_18_O = _mm_add_epi32(c32_89_75_50_18_O, _mm_srli_si128(c32_89_75_50_18_O, 4));
    int dst10 = _mm_cvtsi128_si32(c32_89_75_50_18_O);

    c32_75_n18_n89_n50_O = _mm_add_epi32(c32_75_n18_n89_n50_O, _mm_srli_si128(c32_75_n18_n89_n50_O, 8));
    c32_75_n18_n89_n50_O = _mm_add_epi32(c32_75_n18_n89_n50_O, _mm_srli_si128(c32_75_n18_n89_n50_O, 4));
    int dst26 = _mm_cvtsi128_si32(c32_75_n18_n89_n50_O);

    c32_50_n89_18_75_O = _mm_add_epi32(c32_50_n89_18_75_O, _mm_srli_si128(c32_50_n89_18_75_O, 8));
    c32_50_n89_18_75_O = _mm_add_epi32(c32_50_n89_18_75_O, _mm_srli_si128(c32_50_n89_18_75_O, 4));
    int dst42 = _mm_cvtsi128_si32(c32_50_n89_18_75_O);

    c32_18_n50_75_n89_O = _mm_add_epi32(c32_18_n50_75_n89_O, _mm_srli_si128(c32_18_n50_75_n89_O, 8));
    c32_18_n50_75_n89_O  = _mm_add_epi32(c32_18_n50_75_n89_O, _mm_srli_si128(c32_18_n50_75_n89_O, 4));
    int dst58 = _mm_cvtsi128_si32(c32_18_n50_75_n89_O);

    src += 8;

    src_tmp0 = _mm_load_si128((const __m128i*)src);
    T20 = _mm_srai_epi32(_mm_unpacklo_epi16(src_tmp0, src_tmp0), 16);
    T21 = _mm_srai_epi32(_mm_unpackhi_epi16(src_tmp0, src_tmp0), 16);
    T21 = _mm_shuffle_epi32(T21, 0x1b);

    E = _mm_add_epi32(T20, T21);
    O = _mm_sub_epi32(T20, T21);

    EE0_tmp = _mm_extract_epi32(E, 0);
    EE1_tmp = _mm_extract_epi32(E, 1);
    EE2_tmp = _mm_extract_epi32(E, 2);
    EE3_tmp = _mm_extract_epi32(E, 3);

    EE0 = EE0_tmp + EE3_tmp;
    EE1 = EE1_tmp + EE2_tmp;
    EO0 = EE0_tmp - EE3_tmp;
    EO1 = EE1_tmp - EE2_tmp;

    dst0_tmp1 = (EE0 << 6);
    dst0_tmp2 = (EE1 << 6);

    int dst3 =  dst0_tmp1 + dst0_tmp2;
    int dst35 = dst0_tmp1 - dst0_tmp2;
    int dst19 = 83 * EO0 + 36 * EO1;
    int dst51 = 36 * EO0 - 83 * EO1;

    c32_89_75_50_18_O = _mm_mullo_epi32(c32_89_75_50_18, O);
    c32_75_n18_n89_n50_O = _mm_mullo_epi32(c32_75_n18_n89_n50, O);
    c32_50_n89_18_75_O = _mm_mullo_epi32(c32_50_n89_18_75, O);
    c32_18_n50_75_n89_O = _mm_mullo_epi32(c32_18_n50_75_n89, O);

    c32_89_75_50_18_O = _mm_add_epi32(c32_89_75_50_18_O, _mm_srli_si128(c32_89_75_50_18_O, 8));
    c32_89_75_50_18_O = _mm_add_epi32(c32_89_75_50_18_O, _mm_srli_si128(c32_89_75_50_18_O, 4));
    int dst11 = _mm_cvtsi128_si32(c32_89_75_50_18_O);

    c32_75_n18_n89_n50_O = _mm_add_epi32(c32_75_n18_n89_n50_O, _mm_srli_si128(c32_75_n18_n89_n50_O, 8));
    c32_75_n18_n89_n50_O = _mm_add_epi32(c32_75_n18_n89_n50_O, _mm_srli_si128(c32_75_n18_n89_n50_O, 4));
    int dst27 = _mm_cvtsi128_si32(c32_75_n18_n89_n50_O);

    c32_50_n89_18_75_O = _mm_add_epi32(c32_50_n89_18_75_O, _mm_srli_si128(c32_50_n89_18_75_O, 8));
    c32_50_n89_18_75_O = _mm_add_epi32(c32_50_n89_18_75_O, _mm_srli_si128(c32_50_n89_18_75_O, 4));
    int dst43 = _mm_cvtsi128_si32(c32_50_n89_18_75_O);

    c32_18_n50_75_n89_O = _mm_add_epi32(c32_18_n50_75_n89_O, _mm_srli_si128(c32_18_n50_75_n89_O, 8));
    c32_18_n50_75_n89_O  = _mm_add_epi32(c32_18_n50_75_n89_O, _mm_srli_si128(c32_18_n50_75_n89_O, 4));
    int dst59 = _mm_cvtsi128_si32(c32_18_n50_75_n89_O);

    src += 8;

    src_tmp0 = _mm_load_si128((const __m128i*)src);
    T20 = _mm_srai_epi32(_mm_unpacklo_epi16(src_tmp0, src_tmp0), 16);
    T21 = _mm_srai_epi32(_mm_unpackhi_epi16(src_tmp0, src_tmp0), 16);
    T21 = _mm_shuffle_epi32(T21, 0x1b);

    E = _mm_add_epi32(T20, T21);
    O = _mm_sub_epi32(T20, T21);

    EE0_tmp = _mm_extract_epi32(E, 0);
    EE1_tmp = _mm_extract_epi32(E, 1);
    EE2_tmp = _mm_extract_epi32(E, 2);
    EE3_tmp = _mm_extract_epi32(E, 3);

    EE0 = EE0_tmp + EE3_tmp;
    EE1 = EE1_tmp + EE2_tmp;
    EO0 = EE0_tmp - EE3_tmp;
    EO1 = EE1_tmp - EE2_tmp;

    dst0_tmp1 = (EE0 << 6);
    dst0_tmp2 = (EE1 << 6);

    int dst4 =  dst0_tmp1 + dst0_tmp2;
    int dst36 = dst0_tmp1 - dst0_tmp2;
    int dst20 = 83 * EO0 + 36 * EO1;
    int dst52 = 36 * EO0 - 83 * EO1;

    c32_89_75_50_18_O = _mm_mullo_epi32(c32_89_75_50_18, O);
    c32_75_n18_n89_n50_O = _mm_mullo_epi32(c32_75_n18_n89_n50, O);
    c32_50_n89_18_75_O = _mm_mullo_epi32(c32_50_n89_18_75, O);
    c32_18_n50_75_n89_O = _mm_mullo_epi32(c32_18_n50_75_n89, O);

    c32_89_75_50_18_O = _mm_add_epi32(c32_89_75_50_18_O, _mm_srli_si128(c32_89_75_50_18_O, 8));
    c32_89_75_50_18_O = _mm_add_epi32(c32_89_75_50_18_O, _mm_srli_si128(c32_89_75_50_18_O, 4));
    int dst12 = _mm_cvtsi128_si32(c32_89_75_50_18_O);

    c32_75_n18_n89_n50_O = _mm_add_epi32(c32_75_n18_n89_n50_O, _mm_srli_si128(c32_75_n18_n89_n50_O, 8));
    c32_75_n18_n89_n50_O = _mm_add_epi32(c32_75_n18_n89_n50_O, _mm_srli_si128(c32_75_n18_n89_n50_O, 4));
    int dst28 = _mm_cvtsi128_si32(c32_75_n18_n89_n50_O);

    c32_50_n89_18_75_O = _mm_add_epi32(c32_50_n89_18_75_O, _mm_srli_si128(c32_50_n89_18_75_O, 8));
    c32_50_n89_18_75_O = _mm_add_epi32(c32_50_n89_18_75_O, _mm_srli_si128(c32_50_n89_18_75_O, 4));
    int dst44 = _mm_cvtsi128_si32(c32_50_n89_18_75_O);

    c32_18_n50_75_n89_O = _mm_add_epi32(c32_18_n50_75_n89_O, _mm_srli_si128(c32_18_n50_75_n89_O, 8));
    c32_18_n50_75_n89_O  = _mm_add_epi32(c32_18_n50_75_n89_O, _mm_srli_si128(c32_18_n50_75_n89_O, 4));
    int dst60 = _mm_cvtsi128_si32(c32_18_n50_75_n89_O);

    src += 8;

    src_tmp0 = _mm_load_si128((const __m128i*)src);
    T20 = _mm_srai_epi32(_mm_unpacklo_epi16(src_tmp0, src_tmp0), 16);
    T21 = _mm_srai_epi32(_mm_unpackhi_epi16(src_tmp0, src_tmp0), 16);
    T21 = _mm_shuffle_epi32(T21, 0x1b);

    E = _mm_add_epi32(T20, T21);
    O = _mm_sub_epi32(T20, T21);

    EE0_tmp = _mm_extract_epi32(E, 0);
    EE1_tmp = _mm_extract_epi32(E, 1);
    EE2_tmp = _mm_extract_epi32(E, 2);
    EE3_tmp = _mm_extract_epi32(E, 3);

    EE0 = EE0_tmp + EE3_tmp;
    EE1 = EE1_tmp + EE2_tmp;
    EO0 = EE0_tmp - EE3_tmp;
    EO1 = EE1_tmp - EE2_tmp;

    dst0_tmp1 = (EE0 << 6);
    dst0_tmp2 = (EE1 << 6);

    int dst5 =  dst0_tmp1 + dst0_tmp2;
    int dst37 = dst0_tmp1 - dst0_tmp2;
    int dst21 = 83 * EO0 + 36 * EO1;
    int dst53 = 36 * EO0 - 83 * EO1;

    c32_89_75_50_18_O = _mm_mullo_epi32(c32_89_75_50_18, O);
    c32_75_n18_n89_n50_O = _mm_mullo_epi32(c32_75_n18_n89_n50, O);
    c32_50_n89_18_75_O = _mm_mullo_epi32(c32_50_n89_18_75, O);
    c32_18_n50_75_n89_O = _mm_mullo_epi32(c32_18_n50_75_n89, O);

    c32_89_75_50_18_O = _mm_add_epi32(c32_89_75_50_18_O, _mm_srli_si128(c32_89_75_50_18_O, 8));
    c32_89_75_50_18_O = _mm_add_epi32(c32_89_75_50_18_O, _mm_srli_si128(c32_89_75_50_18_O, 4));
    int dst13 = _mm_cvtsi128_si32(c32_89_75_50_18_O);

    c32_75_n18_n89_n50_O = _mm_add_epi32(c32_75_n18_n89_n50_O, _mm_srli_si128(c32_75_n18_n89_n50_O, 8));
    c32_75_n18_n89_n50_O = _mm_add_epi32(c32_75_n18_n89_n50_O, _mm_srli_si128(c32_75_n18_n89_n50_O, 4));
    int dst29 = _mm_cvtsi128_si32(c32_75_n18_n89_n50_O);

    c32_50_n89_18_75_O = _mm_add_epi32(c32_50_n89_18_75_O, _mm_srli_si128(c32_50_n89_18_75_O, 8));
    c32_50_n89_18_75_O = _mm_add_epi32(c32_50_n89_18_75_O, _mm_srli_si128(c32_50_n89_18_75_O, 4));
    int dst45 = _mm_cvtsi128_si32(c32_50_n89_18_75_O);

    c32_18_n50_75_n89_O = _mm_add_epi32(c32_18_n50_75_n89_O, _mm_srli_si128(c32_18_n50_75_n89_O, 8));
    c32_18_n50_75_n89_O  = _mm_add_epi32(c32_18_n50_75_n89_O, _mm_srli_si128(c32_18_n50_75_n89_O, 4));
    int dst61 = _mm_cvtsi128_si32(c32_18_n50_75_n89_O);

    src += 8;

    src_tmp0 = _mm_load_si128((const __m128i*)src);
    T20 = _mm_srai_epi32(_mm_unpacklo_epi16(src_tmp0, src_tmp0), 16);
    T21 = _mm_srai_epi32(_mm_unpackhi_epi16(src_tmp0, src_tmp0), 16);
    T21 = _mm_shuffle_epi32(T21, 0x1b);

    E = _mm_add_epi32(T20, T21);
    O = _mm_sub_epi32(T20, T21);

    EE0_tmp = _mm_extract_epi32(E, 0);
    EE1_tmp = _mm_extract_epi32(E, 1);
    EE2_tmp = _mm_extract_epi32(E, 2);
    EE3_tmp = _mm_extract_epi32(E, 3);

    EE0 = EE0_tmp + EE3_tmp;
    EE1 = EE1_tmp + EE2_tmp;
    EO0 = EE0_tmp - EE3_tmp;
    EO1 = EE1_tmp - EE2_tmp;

    dst0_tmp1 = (EE0 << 6);
    dst0_tmp2 = (EE1 << 6);

    int dst6 =  dst0_tmp1 + dst0_tmp2;
    int dst38 = dst0_tmp1 - dst0_tmp2;
    int dst22 = 83 * EO0 + 36 * EO1;
    int dst54 = 36 * EO0 - 83 * EO1;

    c32_89_75_50_18_O = _mm_mullo_epi32(c32_89_75_50_18, O);
    c32_75_n18_n89_n50_O = _mm_mullo_epi32(c32_75_n18_n89_n50, O);
    c32_50_n89_18_75_O = _mm_mullo_epi32(c32_50_n89_18_75, O);
    c32_18_n50_75_n89_O = _mm_mullo_epi32(c32_18_n50_75_n89, O);

    c32_89_75_50_18_O = _mm_add_epi32(c32_89_75_50_18_O, _mm_srli_si128(c32_89_75_50_18_O, 8));
    c32_89_75_50_18_O = _mm_add_epi32(c32_89_75_50_18_O, _mm_srli_si128(c32_89_75_50_18_O, 4));
    int dst14 = _mm_cvtsi128_si32(c32_89_75_50_18_O);

    c32_75_n18_n89_n50_O = _mm_add_epi32(c32_75_n18_n89_n50_O, _mm_srli_si128(c32_75_n18_n89_n50_O, 8));
    c32_75_n18_n89_n50_O = _mm_add_epi32(c32_75_n18_n89_n50_O, _mm_srli_si128(c32_75_n18_n89_n50_O, 4));
    int dst30 = _mm_cvtsi128_si32(c32_75_n18_n89_n50_O);

    c32_50_n89_18_75_O = _mm_add_epi32(c32_50_n89_18_75_O, _mm_srli_si128(c32_50_n89_18_75_O, 8));
    c32_50_n89_18_75_O = _mm_add_epi32(c32_50_n89_18_75_O, _mm_srli_si128(c32_50_n89_18_75_O, 4));
    int dst46 = _mm_cvtsi128_si32(c32_50_n89_18_75_O);

    c32_18_n50_75_n89_O = _mm_add_epi32(c32_18_n50_75_n89_O, _mm_srli_si128(c32_18_n50_75_n89_O, 8));
    c32_18_n50_75_n89_O  = _mm_add_epi32(c32_18_n50_75_n89_O, _mm_srli_si128(c32_18_n50_75_n89_O, 4));
    int dst62 = _mm_cvtsi128_si32(c32_18_n50_75_n89_O);

    src += 8;

    src_tmp0 = _mm_load_si128((const __m128i*)src);
    T20 = _mm_srai_epi32(_mm_unpacklo_epi16(src_tmp0, src_tmp0), 16);
    T21 = _mm_srai_epi32(_mm_unpackhi_epi16(src_tmp0, src_tmp0), 16);
    T21 = _mm_shuffle_epi32(T21, 0x1b);

    E = _mm_add_epi32(T20, T21);
    O = _mm_sub_epi32(T20, T21);

    EE0_tmp = _mm_extract_epi32(E, 0);
    EE1_tmp = _mm_extract_epi32(E, 1);
    EE2_tmp = _mm_extract_epi32(E, 2);
    EE3_tmp = _mm_extract_epi32(E, 3);

    EE0 = EE0_tmp + EE3_tmp;
    EE1 = EE1_tmp + EE2_tmp;
    EO0 = EE0_tmp - EE3_tmp;
    EO1 = EE1_tmp - EE2_tmp;

    dst0_tmp1 = (EE0 << 6);
    dst0_tmp2 = (EE1 << 6);

    int dst7 =  dst0_tmp1 + dst0_tmp2;
    int dst39 = dst0_tmp1 - dst0_tmp2;
    int dst23 = 83 * EO0 + 36 * EO1;
    int dst55 = 36 * EO0 - 83 * EO1;

    c32_89_75_50_18_O = _mm_mullo_epi32(c32_89_75_50_18, O);
    c32_75_n18_n89_n50_O = _mm_mullo_epi32(c32_75_n18_n89_n50, O);
    c32_50_n89_18_75_O = _mm_mullo_epi32(c32_50_n89_18_75, O);
    c32_18_n50_75_n89_O = _mm_mullo_epi32(c32_18_n50_75_n89, O);

    c32_89_75_50_18_O = _mm_add_epi32(c32_89_75_50_18_O, _mm_srli_si128(c32_89_75_50_18_O, 8));
    c32_89_75_50_18_O = _mm_add_epi32(c32_89_75_50_18_O, _mm_srli_si128(c32_89_75_50_18_O, 4));
    int dst15 = _mm_cvtsi128_si32(c32_89_75_50_18_O);

    c32_75_n18_n89_n50_O = _mm_add_epi32(c32_75_n18_n89_n50_O, _mm_srli_si128(c32_75_n18_n89_n50_O, 8));
    c32_75_n18_n89_n50_O = _mm_add_epi32(c32_75_n18_n89_n50_O, _mm_srli_si128(c32_75_n18_n89_n50_O, 4));
    int dst31 = _mm_cvtsi128_si32(c32_75_n18_n89_n50_O);

    c32_50_n89_18_75_O = _mm_add_epi32(c32_50_n89_18_75_O, _mm_srli_si128(c32_50_n89_18_75_O, 8));
    c32_50_n89_18_75_O = _mm_add_epi32(c32_50_n89_18_75_O, _mm_srli_si128(c32_50_n89_18_75_O, 4));
    int dst47 = _mm_cvtsi128_si32(c32_50_n89_18_75_O);

    c32_18_n50_75_n89_O = _mm_add_epi32(c32_18_n50_75_n89_O, _mm_srli_si128(c32_18_n50_75_n89_O, 8));
    c32_18_n50_75_n89_O  = _mm_add_epi32(c32_18_n50_75_n89_O, _mm_srli_si128(c32_18_n50_75_n89_O, 4));
    int dst63 = _mm_cvtsi128_si32(c32_18_n50_75_n89_O);

    __m128i dst_0_1_2_3 = _mm_set_epi32(dst3, dst2, dst1, dst0);
    dst_0_1_2_3 = _mm_add_epi32(dst_0_1_2_3, c32_add);
    dst_0_1_2_3 = _mm_srai_epi32(dst_0_1_2_3, shift);

    __m128i dst_4_5_6_7 = _mm_set_epi32(dst7, dst6, dst5, dst4);
    dst_4_5_6_7 = _mm_add_epi32(dst_4_5_6_7, c32_add);
    dst_4_5_6_7 = _mm_srai_epi32(dst_4_5_6_7, shift);

    dst_0_1_2_3 = _mm_slli_epi32(dst_0_1_2_3, 16);
    dst_4_5_6_7  = _mm_slli_epi32(dst_4_5_6_7, 16);
    dst_0_1_2_3 = _mm_srai_epi32(dst_0_1_2_3, 16);
    dst_4_5_6_7 = _mm_srai_epi32(dst_4_5_6_7, 16);

    __m128i dst_0_7 = _mm_packs_epi32(dst_0_1_2_3, dst_4_5_6_7);
    _mm_store_si128((__m128i*)dst, dst_0_7);

    __m128i dst32_33_34_35 = _mm_set_epi32(dst35, dst34, dst33, dst32);
    dst32_33_34_35 = _mm_add_epi32(dst32_33_34_35, c32_add);
    dst32_33_34_35 = _mm_srai_epi32(dst32_33_34_35, shift);

    __m128i dst36_37_38_39 = _mm_set_epi32(dst39, dst38, dst37, dst36);
    dst36_37_38_39 = _mm_add_epi32(dst36_37_38_39, c32_add);
    dst36_37_38_39 = _mm_srai_epi32(dst36_37_38_39, shift);

    dst32_33_34_35 = _mm_slli_epi32(dst32_33_34_35, 16);
    dst36_37_38_39  = _mm_slli_epi32(dst36_37_38_39, 16);
    dst32_33_34_35 = _mm_srai_epi32(dst32_33_34_35, 16);
    dst36_37_38_39 = _mm_srai_epi32(dst36_37_38_39, 16);

    __m128i dst_32_39 = _mm_packs_epi32(dst32_33_34_35, dst36_37_38_39);
    _mm_store_si128((__m128i*)(dst + 32), dst_32_39);

    __m128i dst16_17_18_19 = _mm_set_epi32(dst19, dst18, dst17, dst16);
    dst16_17_18_19 = _mm_add_epi32(dst16_17_18_19, c32_add);
    dst16_17_18_19 = _mm_srai_epi32(dst16_17_18_19, shift);

    __m128i dst20_21_22_23 = _mm_set_epi32(dst23, dst22, dst21, dst20);
    dst20_21_22_23 = _mm_add_epi32(dst20_21_22_23, c32_add);
    dst20_21_22_23 = _mm_srai_epi32(dst20_21_22_23, shift);

    dst16_17_18_19 = _mm_slli_epi32(dst16_17_18_19, 16);
    dst20_21_22_23  = _mm_slli_epi32(dst20_21_22_23, 16);
    dst16_17_18_19 = _mm_srai_epi32(dst16_17_18_19, 16);
    dst20_21_22_23 = _mm_srai_epi32(dst20_21_22_23, 16);

    __m128i dst_16_23 = _mm_packs_epi32(dst16_17_18_19, dst20_21_22_23);
    _mm_store_si128((__m128i*)(dst + 16), dst_16_23);

    __m128i dst48_49_50_51 = _mm_set_epi32(dst51, dst50, dst49, dst48);
    dst48_49_50_51 = _mm_add_epi32(dst48_49_50_51, c32_add);
    dst48_49_50_51 = _mm_srai_epi32(dst48_49_50_51, shift);

    __m128i dst52_53_54_55 = _mm_set_epi32(dst55, dst54, dst53, dst52);
    dst52_53_54_55 = _mm_add_epi32(dst52_53_54_55, c32_add);
    dst52_53_54_55 = _mm_srai_epi32(dst52_53_54_55, shift);

    dst48_49_50_51 = _mm_slli_epi32(dst48_49_50_51, 16);
    dst52_53_54_55  = _mm_slli_epi32(dst52_53_54_55, 16);
    dst48_49_50_51 = _mm_srai_epi32(dst48_49_50_51, 16);
    dst52_53_54_55 = _mm_srai_epi32(dst52_53_54_55, 16);

    __m128i dst_48_55 = _mm_packs_epi32(dst48_49_50_51, dst52_53_54_55);
    _mm_store_si128((__m128i*)(dst + 48),  dst_48_55);

    __m128i dst_8_9_10_11 = _mm_set_epi32(dst11, dst10, dst9, dst8);
    dst_8_9_10_11 = _mm_add_epi32(dst_8_9_10_11, c32_add);
    dst_8_9_10_11 = _mm_srai_epi32(dst_8_9_10_11, shift);

    __m128i dst_12_13_14_15 = _mm_set_epi32(dst15, dst14, dst13, dst12);
    dst_12_13_14_15 = _mm_add_epi32(dst_12_13_14_15, c32_add);
    dst_12_13_14_15 = _mm_srai_epi32(dst_12_13_14_15, shift);

    dst_8_9_10_11 = _mm_slli_epi32(dst_8_9_10_11, 16);
    dst_12_13_14_15  = _mm_slli_epi32(dst_12_13_14_15, 16);
    dst_8_9_10_11 = _mm_srai_epi32(dst_8_9_10_11, 16);
    dst_12_13_14_15 = _mm_srai_epi32(dst_12_13_14_15, 16);

    __m128i dst_8_15 = _mm_packs_epi32(dst_8_9_10_11, dst_12_13_14_15);
    _mm_store_si128((__m128i*)(dst + 8), dst_8_15);

    __m128i dst24_25_26_27 = _mm_set_epi32(dst27, dst26, dst25, dst24);
    dst24_25_26_27 = _mm_add_epi32(dst24_25_26_27, c32_add);
    dst24_25_26_27 = _mm_srai_epi32(dst24_25_26_27, shift);

    __m128i dst28_29_30_31 = _mm_set_epi32(dst31, dst30, dst29, dst28);
    dst28_29_30_31 = _mm_add_epi32(dst28_29_30_31, c32_add);
    dst28_29_30_31 = _mm_srai_epi32(dst28_29_30_31, shift);

    dst24_25_26_27 = _mm_slli_epi32(dst24_25_26_27, 16);
    dst28_29_30_31  = _mm_slli_epi32(dst28_29_30_31, 16);
    dst24_25_26_27 = _mm_srai_epi32(dst24_25_26_27, 16);
    dst28_29_30_31 = _mm_srai_epi32(dst28_29_30_31, 16);

    __m128i dst_24_31 = _mm_packs_epi32(dst24_25_26_27, dst28_29_30_31);
    _mm_store_si128((__m128i*)(dst + 24), dst_24_31);

    __m128i dst40_41_42_43 = _mm_set_epi32(dst43, dst42, dst41, dst40);
    dst40_41_42_43 = _mm_add_epi32(dst40_41_42_43, c32_add);
    dst40_41_42_43  = _mm_srai_epi32(dst40_41_42_43, shift);

    __m128i dst44_45_46_47 = _mm_set_epi32(dst47, dst46, dst45, dst44);
    dst44_45_46_47 = _mm_add_epi32(dst44_45_46_47, c32_add);
    dst44_45_46_47  = _mm_srai_epi32(dst44_45_46_47, shift);

    dst40_41_42_43 = _mm_slli_epi32(dst40_41_42_43, 16);
    dst44_45_46_47  = _mm_slli_epi32(dst44_45_46_47, 16);
    dst40_41_42_43 = _mm_srai_epi32(dst40_41_42_43, 16);
    dst44_45_46_47 = _mm_srai_epi32(dst44_45_46_47, 16);

    __m128i dst_40_47 = _mm_packs_epi32(dst40_41_42_43, dst44_45_46_47);
    _mm_store_si128((__m128i*)(dst + 40), dst_40_47);

    __m128i dst56_57_58_59 = _mm_set_epi32(dst59, dst58, dst57, dst56);
    dst56_57_58_59 = _mm_add_epi32(dst56_57_58_59, c32_add);
    dst56_57_58_59  = _mm_srai_epi32(dst56_57_58_59, shift);

    __m128i dst60_61_62_63 = _mm_set_epi32(dst63, dst62, dst61, dst60);
    dst60_61_62_63 = _mm_add_epi32(dst60_61_62_63, c32_add);
    dst60_61_62_63  = _mm_srai_epi32(dst60_61_62_63, shift);

    dst56_57_58_59 = _mm_slli_epi32(dst56_57_58_59, 16);
    dst60_61_62_63  = _mm_slli_epi32(dst60_61_62_63, 16);
    dst56_57_58_59 = _mm_srai_epi32(dst56_57_58_59, 16);
    dst60_61_62_63 = _mm_srai_epi32(dst60_61_62_63, 16);

    __m128i dst_56_63 = _mm_packs_epi32(dst56_57_58_59, dst60_61_62_63);
    _mm_store_si128((__m128i*)(dst + 56),  dst_56_63);
}

#endif  //partialButterfly8 intrinsic code

#if (INSTRSET > 4) && !defined(VC9_X64)
// Do not allow VC9 x64 to compile this version of the primitive

void CDECL partialButterfly32(short *src, short *dst, int nshift, int line)
{
    int add = 1 << (nshift - 1);
    __m128i c32_add   = _mm_set1_epi32(add);

    __m128i c32_89_75_50_18 = _mm_set_epi32(18, 50, 75, 89);  //for the first loop
    __m128i c32_75_n18_n89_n50 = _mm_set_epi32(-50, -89, -18, 75);
    __m128i c32_50_n89_18_75 = _mm_set_epi32(75, 18, -89, 50);
    __m128i c32_18_n50_75_n89 = _mm_set_epi32(-89, 75, -50, 18);

    __m128i c32_90_87_80_70 = _mm_set_epi32(70, 80, 87, 90);  //for the second loop
    __m128i c32_57_43_25_9 = _mm_set_epi32(9, 25, 43, 57);
    __m128i c32_87_57_9_n43 = _mm_set_epi32(-43, 9, 57, 87);
    __m128i c32_n80_n90_n70_n25 = _mm_set_epi32(-25, -70, -90, -80);
    __m128i c32_80_9_n70_n87 = _mm_set_epi32(-87, -70, 9, 80);
    __m128i c32_n25_57_90_43 = _mm_set_epi32(43, 90, 57, -25);
    __m128i c32_9_n87_n43_70 = _mm_set_epi32(9, -87, -43, 70);
    __m128i c32_90_25_n80_n57 = _mm_set_epi32(-57, -80, 25, 90);
    __m128i c32_57_n80_n25_90 = _mm_set_epi32(90, -25, -80, 57);
    __m128i c32_n9_n87_43_70 = _mm_set_epi32(70, 43, -87, -9);
    __m128i c32_43_n90_57_25 = _mm_set_epi32(25, 57, -90, 43);
    __m128i c32_n87_70_9_n80 = _mm_set_epi32(-80, 9, 70, -87);
    __m128i c32_25_n70_90_n80 = _mm_set_epi32(-80, 90, -70, 25);
    __m128i c32_43_9_n57_87 = _mm_set_epi32(87, -57, 9, 43);
    __m128i c32_9_n25_43_n57 = _mm_set_epi32(-57, 43, -25, 9);
    __m128i c32_70_n80_87_n90 = _mm_set_epi32(-90, 87, -80, 70);

    __m128i c32_90_90_88_85 = _mm_set_epi32(85, 88, 90, 90); //for the third loop
    __m128i c32_82_78_73_67 = _mm_set_epi32(67, 73, 78, 82);
    __m128i c32_61_54_46_38 = _mm_set_epi32(38, 46, 54, 61);
    __m128i c32_4_13_22_31  = _mm_set_epi32(4, 13, 22, 31);

    __m128i c32_90_82_67_46   = _mm_set_epi32(46, 67, 82, 90);
    __m128i c32_22_n4_n31_n54 = _mm_set_epi32(-54, -31, -4, 22);
    __m128i c32_n73_n85_n90_n88 = _mm_set_epi32(-88, -90, -85, -73);
    __m128i c32_n78_n61_n38_n13 = _mm_set_epi32(-13, -38, -61, -78);

    __m128i c32_88_67_31_n13 = _mm_set_epi32(-13, 31, 67, 88);
    __m128i c32_n54_n82_n90_n78 = _mm_set_epi32(-78, -90, -82, -54);
    __m128i c32_73_38_n4_n46 = _mm_set_epi32(73, 38, -4, -46);
    __m128i c32_22_61_85_90 = _mm_set_epi32(22, 61, 85, 90);

    __m128i c32_n67_n13_46_85 = _mm_set_epi32(-67, -13, 46, 85);
    __m128i c32_38_n22_n73_n90 = _mm_set_epi32(38, -22, -73, -90);
    __m128i c32_n4_54_88_82 = _mm_set_epi32(-4, 54, 88, 82);
    __m128i c32_n31_n78_n90_n61 = _mm_set_epi32(-31, -78, -90, -61);

    __m128i c32_n90_n54_22_82 = _mm_set_epi32(-90, -54, 22, 82);
    __m128i c32_85_78_13_n61 = _mm_set_epi32(85, 78, 13, -61);
    __m128i c32_n67_n90_n46_31 = _mm_set_epi32(-67, -90, -46, 31);
    __m128i c32_38_88_73_4 = _mm_set_epi32(38, 88, 73, 4);

    __m128i c32_n73_n82_n4_78 = _mm_set_epi32(-73, -82, -4, 78);
    __m128i c32_n22_67_85_13 = _mm_set_epi32(-22, 67, 85, 13);
    __m128i c32_90_31_n61_n88 = _mm_set_epi32(90, 31, -61, -88);
    __m128i c32_n46_n90_n38_54 = _mm_set_epi32(-46, -90, -38, 54);

    __m128i c32_n22_n90_n31_73 = _mm_set_epi32(-22, -90, -31, 73);
    __m128i c32_n90_n38_67_78 = _mm_set_epi32(-90, -38, 67, 78);
    __m128i c32_n46_61_82_n13 = _mm_set_epi32(-46, 61, 82, -13);
    __m128i c32_54_85_n4_n88 = _mm_set_epi32(54, 85, -4, -88);

    __m128i c32_38_n78_n54_67 = _mm_set_epi32(38, -78, -54, 67);
    __m128i c32_4_n90_n22_85 = _mm_set_epi32(4, -90, -22, 85);
    __m128i c32_n31_n88_13_90 = _mm_set_epi32(-31, -88, 13, 90);
    __m128i c32_n61_n73_46_82 = _mm_set_epi32(-61, -73, 46, 82);

    __m128i c32_82_n46_n73_61 = _mm_set_epi32(82, -46, -73, 61);
    __m128i c32_90_n13_n88_31 = _mm_set_epi32(90, -13, -88, 31);
    __m128i c32_85_22_n90_n4 = _mm_set_epi32(85, 22, -90, -4);
    __m128i c32_67_54_n78_n38 = _mm_set_epi32(67, 54, -78, -38);

    __m128i c32_88_n4_n85_54 = _mm_set_epi32(88, -4, -85, 54);
    __m128i c32_13_82_n61_n46 = _mm_set_epi32(13, 82, -61, -46);
    __m128i c32_n78_67_38_n90 = _mm_set_epi32(-78, 67, 38, -90);
    __m128i c32_n73_n31_90_n22 = _mm_set_epi32(-73, -31, 90, -22);

    __m128i c32_54_38_n90_46 = _mm_set_epi32(54, 38, -90, 46);
    __m128i c32_n88_61_31_n90 = _mm_set_epi32(-88, 61, 31, -90);
    __m128i c32_13_n85_67_22 = _mm_set_epi32(13, -85, 67, 22);
    __m128i c32_73_n82_4_78 = _mm_set_epi32(78, 4, -82, 73);

    __m128i c32_n4_73_n88_38 = _mm_set_epi32(-4, 73, -88, 38);
    __m128i c32_n31_n46_90_n67 = _mm_set_epi32(-31, -46, 90, -67);
    __m128i c32_61_13_n78_85 = _mm_set_epi32(61, 13, -78, 85);
    __m128i c32_n82_22_54_n90 = _mm_set_epi32(-82, 22, 54, -90);

    __m128i c32_n61_90_n78_31 = _mm_set_epi32(-61, 90, -78, 31);
    __m128i c32_82_n88_54_4 = _mm_set_epi32(82, -88, 54, 4);
    __m128i c32_n90_73_n22_n38 = _mm_set_epi32(-90, 73, -22, -38);
    __m128i c32_85_n46_n13_67 = _mm_set_epi32(85, -46, -13, 67);

    __m128i c32_n90_85_n61_22 = _mm_set_epi32(-90, 85, -61, 22);
    __m128i c32_46_n4_n38_73 = _mm_set_epi32(46, -4, -38, 73);
    __m128i c32_54_n82_90_n78 = _mm_set_epi32(54, -82, 90, -78);
    __m128i c32_n88_67_n31_n13 = _mm_set_epi32(-88, 67, -31, -13);

    __m128i c32_n78_61_n38_13 = _mm_set_epi32(-78, 61, -38, 13);
    __m128i c32_88_n90_85_n73 = _mm_set_epi32(-73, 85, -90, 88);
    __m128i c32_22_4_n31_54 = _mm_set_epi32(22, 4, -31, 54);
    __m128i c32_90_n82_67_n46 = _mm_set_epi32(90, -82, 67, -46);

    __m128i c32_n31_22_n13_4 = _mm_set_epi32(-31, 22, -13, 4);
    __m128i c32_n61_54_n46_38 = _mm_set_epi32(-61, 54, -46, 38);
    __m128i c32_n82_78_n73_67 = _mm_set_epi32(-82, 78, -73, 67);
    __m128i c32_n90_90_n88_85 = _mm_set_epi32(-90, 90, -88, 85);

    // to avoid VC9-x64 compiler bugs
    int tmp;

    for (int j = 0; j < line; j++)
    {
        __m128i src_tmp0 = _mm_load_si128((const __m128i*)src);
        __m128i T20 = _mm_srai_epi32(_mm_unpacklo_epi16(src_tmp0, src_tmp0), 16);
        __m128i T21 = _mm_srai_epi32(_mm_unpackhi_epi16(src_tmp0, src_tmp0), 16);

        src_tmp0 = _mm_load_si128((const __m128i*)(src + 8));
        __m128i T22 = _mm_srai_epi32(_mm_unpacklo_epi16(src_tmp0, src_tmp0), 16);
        __m128i T23 = _mm_srai_epi32(_mm_unpackhi_epi16(src_tmp0, src_tmp0), 16);

        src_tmp0 = _mm_load_si128((const __m128i*)(src + 16));
        __m128i T33 = _mm_srai_epi32(_mm_unpacklo_epi16(src_tmp0, src_tmp0), 16);
        T33 = _mm_shuffle_epi32(T33, 0x1b);
        __m128i T32 = _mm_srai_epi32(_mm_unpackhi_epi16(src_tmp0, src_tmp0), 16);
        T32 = _mm_shuffle_epi32(T32, 0x1b);

        src_tmp0 = _mm_load_si128((const __m128i*)(src + 24));
        __m128i T31 = _mm_srai_epi32(_mm_unpacklo_epi16(src_tmp0, src_tmp0), 16);
        T31 = _mm_shuffle_epi32(T31, 0x1b);
        __m128i T30 = _mm_srai_epi32(_mm_unpackhi_epi16(src_tmp0, src_tmp0), 16);
        T30 = _mm_shuffle_epi32(T30, 0x1b);

        __m128i E_0_3 = _mm_add_epi32(T20, T30);
        __m128i E_4_7 = _mm_add_epi32(T21, T31);
        __m128i E_8_11 = _mm_add_epi32(T22, T32);
        __m128i E_12_15 = _mm_add_epi32(T23, T33);

        __m128i E_8_11_rev = _mm_shuffle_epi32(E_8_11, 0x1b);
        __m128i E_12_15_rev = _mm_shuffle_epi32(E_12_15, 0x1b);

        __m128i O_0_3 = _mm_sub_epi32(T20, T30);
        __m128i O_4_7 = _mm_sub_epi32(T21, T31);
        __m128i O_8_11 = _mm_sub_epi32(T22, T32);
        __m128i O_12_15 = _mm_sub_epi32(T23, T33);

        __m128i EE_0_3 = _mm_add_epi32(E_0_3, E_12_15_rev);
        __m128i EE_4_7 = _mm_add_epi32(E_4_7, E_8_11_rev);

        __m128i EO_0_3 = _mm_sub_epi32(E_0_3, E_12_15_rev);
        __m128i EO_4_7 = _mm_sub_epi32(E_4_7, E_8_11_rev);

        __m128i EE_7_4 = _mm_shuffle_epi32(EE_4_7, 0x1b);

        __m128i EEE = _mm_add_epi32(EE_0_3,  EE_7_4);
        __m128i EEO = _mm_sub_epi32(EE_0_3,  EE_7_4);

        __m128i EEE_rev = _mm_shuffle_epi32(EEE, 0x1b);

        __m128i EEEE = _mm_add_epi32(EEE, EEE_rev);
        __m128i EEEO = _mm_sub_epi32(EEE, EEE_rev);

        int EEEE0 = _mm_extract_epi32(EEEE, 0);
        int EEEE1 = _mm_extract_epi32(EEEE, 1);

        int EEEO0 = _mm_extract_epi32(EEEO, 0);
        int EEEO1 = _mm_extract_epi32(EEEO, 1);

        dst[0] = (64 * EEEE0 + 64 * EEEE1 + add) >> nshift;
        dst[16 * line] = (64 * EEEE0 + (-64) * EEEE1 + add) >> nshift;
        dst[8 * line] = (83 * EEEO0 + 36 * EEEO1 + add) >> nshift;
        dst[24 * line] = (36 * EEEO0 + (-83) * EEEO1 + add) >> nshift;

        __m128i dst_tmp0 = _mm_mullo_epi32(c32_89_75_50_18, EEO);
        __m128i dst_tmp1 = _mm_mullo_epi32(c32_75_n18_n89_n50, EEO);
        __m128i dst_tmp2 = _mm_mullo_epi32(c32_50_n89_18_75, EEO);
        __m128i dst_tmp3 = _mm_mullo_epi32(c32_18_n50_75_n89, EEO);

        dst_tmp0 = _mm_add_epi32(dst_tmp0, _mm_srli_si128(dst_tmp0, 8));
        dst_tmp0 = _mm_add_epi32(dst_tmp0, _mm_srli_si128(dst_tmp0, 4));
        int dst_4xline = _mm_cvtsi128_si32(dst_tmp0);

        dst_tmp1 = _mm_add_epi32(dst_tmp1, _mm_srli_si128(dst_tmp1, 8));
        dst_tmp1 = _mm_add_epi32(dst_tmp1, _mm_srli_si128(dst_tmp1, 4));
        int dst_12xline = _mm_cvtsi128_si32(dst_tmp1);

        dst_tmp2 = _mm_add_epi32(dst_tmp2, _mm_srli_si128(dst_tmp2, 8));
        dst_tmp2 = _mm_add_epi32(dst_tmp2, _mm_srli_si128(dst_tmp2, 4));
        int dst_20xline = _mm_cvtsi128_si32(dst_tmp2);

        dst_tmp3 = _mm_add_epi32(dst_tmp3, _mm_srli_si128(dst_tmp3, 8));
        dst_tmp3 = _mm_add_epi32(dst_tmp3, _mm_srli_si128(dst_tmp3, 4));
        int dst_28xline = _mm_cvtsi128_si32(dst_tmp3);

        __m128i dst_4_12_20_28 = _mm_set_epi32(dst_28xline, dst_20xline, dst_12xline, dst_4xline);
        dst_4_12_20_28  = _mm_srai_epi32(_mm_add_epi32(c32_add, dst_4_12_20_28), nshift);

        tmp = _mm_extract_epi32(dst_4_12_20_28, 0);
        dst[4 * line] = (short)tmp;
        tmp = _mm_extract_epi32(dst_4_12_20_28, 1);
        dst[12 * line] = (short)tmp;
        tmp = _mm_extract_epi32(dst_4_12_20_28, 2);
        dst[20 * line] = (short)tmp;
        tmp = _mm_extract_epi32(dst_4_12_20_28, 3);
        dst[28 * line] = (short)tmp;

        __m128i dst_tmp4 = _mm_mullo_epi32(c32_90_87_80_70, EO_0_3);
        __m128i dst_tmp5 = _mm_mullo_epi32(c32_57_43_25_9, EO_4_7);

        dst_tmp4 = _mm_add_epi32(dst_tmp4, _mm_srli_si128(dst_tmp4, 8));
        dst_tmp4 = _mm_add_epi32(dst_tmp4, _mm_srli_si128(dst_tmp4, 4));
        int dst_2xline_lo = _mm_cvtsi128_si32(dst_tmp4);

        dst_tmp5 = _mm_add_epi32(dst_tmp5, _mm_srli_si128(dst_tmp5, 8));
        dst_tmp5 = _mm_add_epi32(dst_tmp5, _mm_srli_si128(dst_tmp5, 4));
        int dst_12xline_hi = _mm_cvtsi128_si32(dst_tmp5);

        int dst_2xline =  dst_2xline_lo + dst_12xline_hi;

        __m128i dst_tmp6 = _mm_mullo_epi32(c32_87_57_9_n43, EO_0_3);
        __m128i dst_tmp7 = _mm_mullo_epi32(c32_n80_n90_n70_n25, EO_4_7);

        dst_tmp6 = _mm_add_epi32(dst_tmp6, _mm_srli_si128(dst_tmp6, 8));
        dst_tmp6 = _mm_add_epi32(dst_tmp6, _mm_srli_si128(dst_tmp6, 4));
        int dst_6xline_lo = _mm_cvtsi128_si32(dst_tmp6);

        dst_tmp7 = _mm_add_epi32(dst_tmp7, _mm_srli_si128(dst_tmp7, 8));
        dst_tmp7 = _mm_add_epi32(dst_tmp7, _mm_srli_si128(dst_tmp7, 4));
        int dst_6xline_hi = _mm_cvtsi128_si32(dst_tmp7);

        int dst_6xline =  dst_6xline_lo + dst_6xline_hi;

        __m128i dst_tmp8 = _mm_mullo_epi32(c32_80_9_n70_n87, EO_0_3);
        __m128i dst_tmp9 = _mm_mullo_epi32(c32_n25_57_90_43, EO_4_7);

        dst_tmp8 = _mm_add_epi32(dst_tmp8, _mm_srli_si128(dst_tmp8, 8));
        dst_tmp8 = _mm_add_epi32(dst_tmp8, _mm_srli_si128(dst_tmp8, 4));
        int dst_10xline_lo = _mm_cvtsi128_si32(dst_tmp8);

        dst_tmp9 = _mm_add_epi32(dst_tmp9, _mm_srli_si128(dst_tmp9, 8));
        dst_tmp9 = _mm_add_epi32(dst_tmp9, _mm_srli_si128(dst_tmp9, 4));
        int dst_10xline_hi = _mm_cvtsi128_si32(dst_tmp9);

        int dst_10xline =  dst_10xline_lo + dst_10xline_hi;

        __m128i dst_tmp10 = _mm_mullo_epi32(c32_9_n87_n43_70, EO_0_3);
        __m128i dst_tmp11 = _mm_mullo_epi32(c32_90_25_n80_n57, EO_4_7);

        dst_tmp10 = _mm_add_epi32(dst_tmp10, _mm_srli_si128(dst_tmp10, 8));
        dst_tmp10 = _mm_add_epi32(dst_tmp10, _mm_srli_si128(dst_tmp10, 4));
        int dst_14xline_lo = _mm_cvtsi128_si32(dst_tmp10);

        dst_tmp11 = _mm_add_epi32(dst_tmp11, _mm_srli_si128(dst_tmp11, 8));
        dst_tmp11 = _mm_add_epi32(dst_tmp11, _mm_srli_si128(dst_tmp11, 4));
        int dst_14xline_hi = _mm_cvtsi128_si32(dst_tmp11);

        int dst_14xline =  dst_14xline_lo + dst_14xline_hi;

        __m128i dst_2_6_10_14 = _mm_set_epi32(dst_14xline, dst_10xline, dst_6xline, dst_2xline);
        dst_2_6_10_14  = _mm_srai_epi32(_mm_add_epi32(c32_add, dst_2_6_10_14), nshift);

        tmp = _mm_extract_epi32(dst_2_6_10_14, 0);
        dst[2 * line] = (short)tmp;
        tmp = _mm_extract_epi32(dst_2_6_10_14, 1);
        dst[6 * line] = (short)tmp;
        tmp = _mm_extract_epi32(dst_2_6_10_14, 2);
        dst[10 * line] = (short)tmp;
        tmp = _mm_extract_epi32(dst_2_6_10_14, 3);
        dst[14 * line] = (short)tmp;

        __m128i dst_tmp12 = _mm_mullo_epi32(c32_57_n80_n25_90, EO_0_3);
        __m128i dst_tmp13 = _mm_mullo_epi32(c32_n9_n87_43_70, EO_4_7);

        dst_tmp12 = _mm_add_epi32(dst_tmp12, _mm_srli_si128(dst_tmp12, 8));
        dst_tmp12 = _mm_add_epi32(dst_tmp12, _mm_srli_si128(dst_tmp12, 4));
        int dst_18xline_lo = _mm_cvtsi128_si32(dst_tmp12);

        dst_tmp13 = _mm_add_epi32(dst_tmp13, _mm_srli_si128(dst_tmp13, 8));
        dst_tmp13 = _mm_add_epi32(dst_tmp13, _mm_srli_si128(dst_tmp13, 4));
        int dst_18xline_hi = _mm_cvtsi128_si32(dst_tmp13);

        int dst_18xline =  dst_18xline_lo + dst_18xline_hi;

        __m128i dst_tmp14 = _mm_mullo_epi32(c32_43_n90_57_25, EO_0_3);
        __m128i dst_tmp15 = _mm_mullo_epi32(c32_n87_70_9_n80, EO_4_7);

        dst_tmp14 = _mm_add_epi32(dst_tmp14, _mm_srli_si128(dst_tmp14, 8));
        dst_tmp14 = _mm_add_epi32(dst_tmp14, _mm_srli_si128(dst_tmp14, 4));
        int dst_22xline_lo = _mm_cvtsi128_si32(dst_tmp14);

        dst_tmp15 = _mm_add_epi32(dst_tmp15, _mm_srli_si128(dst_tmp15, 8));
        dst_tmp15 = _mm_add_epi32(dst_tmp15, _mm_srli_si128(dst_tmp15, 4));
        int dst_22xline_hi = _mm_cvtsi128_si32(dst_tmp15);

        int dst_22xline =  dst_22xline_lo + dst_22xline_hi;

        __m128i dst_tmp16 = _mm_mullo_epi32(c32_25_n70_90_n80, EO_0_3);
        __m128i dst_tmp17 = _mm_mullo_epi32(c32_43_9_n57_87, EO_4_7);

        dst_tmp16 = _mm_add_epi32(dst_tmp16, _mm_srli_si128(dst_tmp16, 8));
        dst_tmp16 = _mm_add_epi32(dst_tmp16, _mm_srli_si128(dst_tmp16, 4));
        int dst_26xline_lo = _mm_cvtsi128_si32(dst_tmp16);

        dst_tmp17 = _mm_add_epi32(dst_tmp17, _mm_srli_si128(dst_tmp17, 8));
        dst_tmp17 = _mm_add_epi32(dst_tmp17, _mm_srli_si128(dst_tmp17, 4));
        int dst_26xline_hi = _mm_cvtsi128_si32(dst_tmp17);

        int dst_26xline =  dst_26xline_lo + dst_26xline_hi;

        __m128i dst_tmp18 = _mm_mullo_epi32(c32_9_n25_43_n57, EO_0_3);
        __m128i dst_tmp19 = _mm_mullo_epi32(c32_70_n80_87_n90, EO_4_7);

        dst_tmp18 = _mm_add_epi32(dst_tmp18, _mm_srli_si128(dst_tmp18, 8));
        dst_tmp18 = _mm_add_epi32(dst_tmp18, _mm_srli_si128(dst_tmp18, 4));
        int dst_30xline_lo = _mm_cvtsi128_si32(dst_tmp18);

        dst_tmp19 = _mm_add_epi32(dst_tmp19, _mm_srli_si128(dst_tmp19, 8));
        dst_tmp19 = _mm_add_epi32(dst_tmp19, _mm_srli_si128(dst_tmp19, 4));
        int dst_30xline_hi = _mm_cvtsi128_si32(dst_tmp19);

        int dst_30xline =  dst_30xline_lo + dst_30xline_hi;

        __m128i dst_18_22_26_30 = _mm_set_epi32(dst_30xline, dst_26xline, dst_22xline, dst_18xline);
        dst_18_22_26_30  = _mm_srai_epi32(_mm_add_epi32(c32_add, dst_18_22_26_30), nshift);

        tmp = _mm_extract_epi32(dst_18_22_26_30, 0);
        dst[18 * line] = (short)tmp;
        tmp = _mm_extract_epi32(dst_18_22_26_30, 1);
        dst[22 * line] = (short)tmp;
        tmp = _mm_extract_epi32(dst_18_22_26_30, 2);
        dst[26 * line] = (short)tmp;
        tmp = _mm_extract_epi32(dst_18_22_26_30, 3);
        dst[30 * line] = (short)tmp;

        __m128i dst_tmp20 = _mm_mullo_epi32(c32_90_90_88_85, O_0_3);
        __m128i dst_tmp21 = _mm_mullo_epi32(c32_82_78_73_67, O_4_7);
        __m128i dst_tmp22 = _mm_mullo_epi32(c32_61_54_46_38, O_8_11);
        __m128i dst_tmp23 = _mm_mullo_epi32(c32_4_13_22_31, O_12_15);

        dst_tmp20 = _mm_add_epi32(dst_tmp20, _mm_srli_si128(dst_tmp20, 8));
        dst_tmp20 = _mm_add_epi32(dst_tmp20, _mm_srli_si128(dst_tmp20, 4));
        int dst_1xline_lo1 = _mm_cvtsi128_si32(dst_tmp20);

        dst_tmp21 = _mm_add_epi32(dst_tmp21, _mm_srli_si128(dst_tmp21, 8));
        dst_tmp21 = _mm_add_epi32(dst_tmp21, _mm_srli_si128(dst_tmp21, 4));
        int dst_1xline_lo2 = _mm_cvtsi128_si32(dst_tmp21);

        dst_tmp22 = _mm_add_epi32(dst_tmp22, _mm_srli_si128(dst_tmp22, 8));
        dst_tmp22 = _mm_add_epi32(dst_tmp22, _mm_srli_si128(dst_tmp22, 4));
        int dst_1xline_hi1 = _mm_cvtsi128_si32(dst_tmp22);

        dst_tmp23 = _mm_add_epi32(dst_tmp23, _mm_srli_si128(dst_tmp23, 8));
        dst_tmp23 = _mm_add_epi32(dst_tmp23, _mm_srli_si128(dst_tmp23, 4));
        int dst_1xline_hi2 = _mm_cvtsi128_si32(dst_tmp23);

        int dst_1xline =  dst_1xline_lo1 + dst_1xline_lo2 + dst_1xline_hi1 + dst_1xline_hi2;

        __m128i dst_tmp24 = _mm_mullo_epi32(c32_90_82_67_46, O_0_3);
        __m128i dst_tmp25 = _mm_mullo_epi32(c32_22_n4_n31_n54, O_4_7);
        __m128i dst_tmp26 = _mm_mullo_epi32(c32_n73_n85_n90_n88, O_8_11);
        __m128i dst_tmp27 = _mm_mullo_epi32(c32_n78_n61_n38_n13, O_12_15);

        dst_tmp24 = _mm_add_epi32(dst_tmp24, _mm_srli_si128(dst_tmp24, 8));
        dst_tmp24 = _mm_add_epi32(dst_tmp24, _mm_srli_si128(dst_tmp24, 4));
        int dst_3xline_lo1 = _mm_cvtsi128_si32(dst_tmp24);

        dst_tmp25 = _mm_add_epi32(dst_tmp25, _mm_srli_si128(dst_tmp25, 8));
        dst_tmp25 = _mm_add_epi32(dst_tmp25, _mm_srli_si128(dst_tmp25, 4));
        int dst_3xline_lo2 = _mm_cvtsi128_si32(dst_tmp25);

        dst_tmp26 = _mm_add_epi32(dst_tmp26, _mm_srli_si128(dst_tmp26, 8));
        dst_tmp26 = _mm_add_epi32(dst_tmp26, _mm_srli_si128(dst_tmp26, 4));
        int dst_3xline_hi1 = _mm_cvtsi128_si32(dst_tmp26);

        dst_tmp27 = _mm_add_epi32(dst_tmp27, _mm_srli_si128(dst_tmp27, 8));
        dst_tmp27 = _mm_add_epi32(dst_tmp27, _mm_srli_si128(dst_tmp27, 4));
        int dst_3xline_hi2 = _mm_cvtsi128_si32(dst_tmp27);

        int dst_3xline =  dst_3xline_lo1 + dst_3xline_lo2 + dst_3xline_hi1 + dst_3xline_hi2;

        __m128i dst_tmp28 = _mm_mullo_epi32(c32_88_67_31_n13, O_0_3);
        __m128i dst_tmp29 = _mm_mullo_epi32(c32_n54_n82_n90_n78, O_4_7);
        __m128i dst_tmp30 = _mm_mullo_epi32(c32_73_38_n4_n46, O_8_11);
        __m128i dst_tmp31 = _mm_mullo_epi32(c32_22_61_85_90, O_12_15);

        dst_tmp28 = _mm_add_epi32(dst_tmp28, _mm_srli_si128(dst_tmp28, 8));
        dst_tmp28 = _mm_add_epi32(dst_tmp28, _mm_srli_si128(dst_tmp28, 4));
        int dst_5xline_lo1 = _mm_cvtsi128_si32(dst_tmp28);

        dst_tmp29 = _mm_add_epi32(dst_tmp29, _mm_srli_si128(dst_tmp29, 8));
        dst_tmp29 = _mm_add_epi32(dst_tmp29, _mm_srli_si128(dst_tmp29, 4));
        int dst_5xline_lo2 = _mm_cvtsi128_si32(dst_tmp29);

        dst_tmp30 = _mm_add_epi32(dst_tmp30, _mm_srli_si128(dst_tmp30, 8));
        dst_tmp30 = _mm_add_epi32(dst_tmp30, _mm_srli_si128(dst_tmp30, 4));
        int dst_5xline_hi1 = _mm_cvtsi128_si32(dst_tmp30);

        dst_tmp31 = _mm_add_epi32(dst_tmp31, _mm_srli_si128(dst_tmp31, 8));
        dst_tmp31 = _mm_add_epi32(dst_tmp31, _mm_srli_si128(dst_tmp31, 4));
        int dst_5xline_hi2 = _mm_cvtsi128_si32(dst_tmp31);

        int dst_5xline =  dst_5xline_lo1 + dst_5xline_lo2 + dst_5xline_hi1 + dst_5xline_hi2;

        __m128i dst_tmp32 = _mm_mullo_epi32(c32_n67_n13_46_85, O_0_3);
        __m128i dst_tmp33 = _mm_mullo_epi32(c32_38_n22_n73_n90, O_4_7);
        __m128i dst_tmp34 = _mm_mullo_epi32(c32_n4_54_88_82, O_8_11);
        __m128i dst_tmp35 = _mm_mullo_epi32(c32_n31_n78_n90_n61, O_12_15);

        dst_tmp32 = _mm_add_epi32(dst_tmp32, _mm_srli_si128(dst_tmp32, 8));
        dst_tmp32 = _mm_add_epi32(dst_tmp32, _mm_srli_si128(dst_tmp32, 4));
        int dst_7xline_lo1 = _mm_cvtsi128_si32(dst_tmp32);

        dst_tmp33 = _mm_add_epi32(dst_tmp33, _mm_srli_si128(dst_tmp33, 8));
        dst_tmp33 = _mm_add_epi32(dst_tmp33, _mm_srli_si128(dst_tmp33, 4));
        int dst_7xline_lo2 = _mm_cvtsi128_si32(dst_tmp33);

        dst_tmp34 = _mm_add_epi32(dst_tmp34, _mm_srli_si128(dst_tmp34, 8));
        dst_tmp34 = _mm_add_epi32(dst_tmp34, _mm_srli_si128(dst_tmp34, 4));
        int dst_7xline_hi1 = _mm_cvtsi128_si32(dst_tmp34);

        dst_tmp35 = _mm_add_epi32(dst_tmp35, _mm_srli_si128(dst_tmp35, 8));
        dst_tmp35 = _mm_add_epi32(dst_tmp35, _mm_srli_si128(dst_tmp35, 4));
        int dst_7xline_hi2 = _mm_cvtsi128_si32(dst_tmp35);

        int dst_7xline =  dst_7xline_lo1 + dst_7xline_lo2 + dst_7xline_hi1 + dst_7xline_hi2;

        __m128i dst_1_3_5_7 = _mm_set_epi32(dst_7xline, dst_5xline, dst_3xline, dst_1xline);
        dst_1_3_5_7  = _mm_srai_epi32(_mm_add_epi32(c32_add, dst_1_3_5_7), nshift);

        tmp = _mm_extract_epi32(dst_1_3_5_7, 0);
        dst[1 * line] = (short)tmp;
        tmp = _mm_extract_epi32(dst_1_3_5_7, 1);
        dst[3 * line] = (short)tmp;
        tmp = _mm_extract_epi32(dst_1_3_5_7, 2);
        dst[5 * line] = (short)tmp;
        tmp = _mm_extract_epi32(dst_1_3_5_7, 3);
        dst[7 * line] = (short)tmp;

        __m128i dst_tmp36 = _mm_mullo_epi32(c32_n90_n54_22_82, O_0_3);
        __m128i dst_tmp37 = _mm_mullo_epi32(c32_85_78_13_n61, O_4_7);
        __m128i dst_tmp38 = _mm_mullo_epi32(c32_n67_n90_n46_31, O_8_11);
        __m128i dst_tmp39 = _mm_mullo_epi32(c32_38_88_73_4, O_12_15);

        dst_tmp36 = _mm_add_epi32(dst_tmp36, _mm_srli_si128(dst_tmp36, 8));
        dst_tmp36 = _mm_add_epi32(dst_tmp36, _mm_srli_si128(dst_tmp36, 4));
        int dst_9xline_lo1 = _mm_cvtsi128_si32(dst_tmp36);

        dst_tmp37 = _mm_add_epi32(dst_tmp37, _mm_srli_si128(dst_tmp37, 8));
        dst_tmp37 = _mm_add_epi32(dst_tmp37, _mm_srli_si128(dst_tmp37, 4));
        int dst_9xline_lo2 = _mm_cvtsi128_si32(dst_tmp37);

        dst_tmp38 = _mm_add_epi32(dst_tmp38, _mm_srli_si128(dst_tmp38, 8));
        dst_tmp38 = _mm_add_epi32(dst_tmp38, _mm_srli_si128(dst_tmp38, 4));
        int dst_9xline_hi1 = _mm_cvtsi128_si32(dst_tmp38);

        dst_tmp39 = _mm_add_epi32(dst_tmp39, _mm_srli_si128(dst_tmp39, 8));
        dst_tmp39 = _mm_add_epi32(dst_tmp39, _mm_srli_si128(dst_tmp39, 4));
        int dst_9xline_hi2 = _mm_cvtsi128_si32(dst_tmp39);

        int dst_9xline =  dst_9xline_lo1 + dst_9xline_lo2 + dst_9xline_hi1 + dst_9xline_hi2;

        __m128i dst_tmp40 = _mm_mullo_epi32(c32_n73_n82_n4_78, O_0_3);
        __m128i dst_tmp41 = _mm_mullo_epi32(c32_n22_67_85_13, O_4_7);
        __m128i dst_tmp42 = _mm_mullo_epi32(c32_90_31_n61_n88, O_8_11);
        __m128i dst_tmp43 = _mm_mullo_epi32(c32_n46_n90_n38_54, O_12_15);

        dst_tmp40 = _mm_add_epi32(dst_tmp40, _mm_srli_si128(dst_tmp40, 8));
        dst_tmp40 = _mm_add_epi32(dst_tmp40, _mm_srli_si128(dst_tmp40, 4));
        int dst_11xline_lo1 = _mm_cvtsi128_si32(dst_tmp40);

        dst_tmp41 = _mm_add_epi32(dst_tmp41, _mm_srli_si128(dst_tmp41, 8));
        dst_tmp41 = _mm_add_epi32(dst_tmp41, _mm_srli_si128(dst_tmp41, 4));
        int dst_11xline_lo2 = _mm_cvtsi128_si32(dst_tmp41);

        dst_tmp42 = _mm_add_epi32(dst_tmp42, _mm_srli_si128(dst_tmp42, 8));
        dst_tmp42 = _mm_add_epi32(dst_tmp42, _mm_srli_si128(dst_tmp42, 4));
        int dst_11xline_hi1 = _mm_cvtsi128_si32(dst_tmp42);

        dst_tmp43 = _mm_add_epi32(dst_tmp43, _mm_srli_si128(dst_tmp43, 8));
        dst_tmp43 = _mm_add_epi32(dst_tmp43, _mm_srli_si128(dst_tmp43, 4));
        int dst_11xline_hi2 = _mm_cvtsi128_si32(dst_tmp43);

        int dst_11xline =  dst_11xline_lo1 + dst_11xline_lo2 + dst_11xline_hi1 + dst_11xline_hi2;

        __m128i dst_tmp44 = _mm_mullo_epi32(c32_n22_n90_n31_73, O_0_3);
        __m128i dst_tmp45 = _mm_mullo_epi32(c32_n90_n38_67_78, O_4_7);
        __m128i dst_tmp46 = _mm_mullo_epi32(c32_n46_61_82_n13, O_8_11);
        __m128i dst_tmp47 = _mm_mullo_epi32(c32_54_85_n4_n88, O_12_15);

        dst_tmp44 = _mm_add_epi32(dst_tmp44, _mm_srli_si128(dst_tmp44, 8));
        dst_tmp44 = _mm_add_epi32(dst_tmp44, _mm_srli_si128(dst_tmp44, 4));
        int dst_13xline_lo1 = _mm_cvtsi128_si32(dst_tmp44);

        dst_tmp45 = _mm_add_epi32(dst_tmp45, _mm_srli_si128(dst_tmp45, 8));
        dst_tmp45 = _mm_add_epi32(dst_tmp45, _mm_srli_si128(dst_tmp45, 4));
        int dst_13xline_lo2 = _mm_cvtsi128_si32(dst_tmp45);

        dst_tmp46 = _mm_add_epi32(dst_tmp46, _mm_srli_si128(dst_tmp46, 8));
        dst_tmp46 = _mm_add_epi32(dst_tmp46, _mm_srli_si128(dst_tmp46, 4));
        int dst_13xline_hi1 = _mm_cvtsi128_si32(dst_tmp46);

        dst_tmp47 = _mm_add_epi32(dst_tmp47, _mm_srli_si128(dst_tmp47, 8));
        dst_tmp47 = _mm_add_epi32(dst_tmp47, _mm_srli_si128(dst_tmp47, 4));
        int dst_13xline_hi2 = _mm_cvtsi128_si32(dst_tmp47);

        int dst_13xline =  dst_13xline_lo1 + dst_13xline_lo2 + dst_13xline_hi1 + dst_13xline_hi2;
        __m128i dst_tmp48 = _mm_mullo_epi32(c32_38_n78_n54_67, O_0_3);
        __m128i dst_tmp49 = _mm_mullo_epi32(c32_4_n90_n22_85, O_4_7);
        __m128i dst_tmp50 = _mm_mullo_epi32(c32_n31_n88_13_90, O_8_11);
        __m128i dst_tmp51 = _mm_mullo_epi32(c32_n61_n73_46_82, O_12_15);

        dst_tmp48 = _mm_add_epi32(dst_tmp48, _mm_srli_si128(dst_tmp48, 8));
        dst_tmp48 = _mm_add_epi32(dst_tmp48, _mm_srli_si128(dst_tmp48, 4));
        int dst_15xline_lo1 = _mm_cvtsi128_si32(dst_tmp48);

        dst_tmp49 = _mm_add_epi32(dst_tmp49, _mm_srli_si128(dst_tmp49, 8));
        dst_tmp49 = _mm_add_epi32(dst_tmp49, _mm_srli_si128(dst_tmp49, 4));
        int dst_15xline_lo2 = _mm_cvtsi128_si32(dst_tmp49);

        dst_tmp50 = _mm_add_epi32(dst_tmp50, _mm_srli_si128(dst_tmp50, 8));
        dst_tmp50 = _mm_add_epi32(dst_tmp50, _mm_srli_si128(dst_tmp50, 4));
        int dst_15xline_hi1 = _mm_cvtsi128_si32(dst_tmp50);

        dst_tmp51 = _mm_add_epi32(dst_tmp51, _mm_srli_si128(dst_tmp51, 8));
        dst_tmp51 = _mm_add_epi32(dst_tmp51, _mm_srli_si128(dst_tmp51, 4));
        int dst_15xline_hi2 = _mm_cvtsi128_si32(dst_tmp51);

        int dst_15xline =  dst_15xline_lo1 + dst_15xline_lo2 + dst_15xline_hi1 + dst_15xline_hi2;

        __m128i dst_9_11_13_15 = _mm_set_epi32(dst_15xline, dst_13xline, dst_11xline, dst_9xline);
        dst_9_11_13_15  = _mm_srai_epi32(_mm_add_epi32(c32_add, dst_9_11_13_15), nshift);

        tmp = _mm_extract_epi32(dst_9_11_13_15, 0);
        dst[9 * line] = (short)tmp;
        tmp = _mm_extract_epi32(dst_9_11_13_15, 1);
        dst[11 * line] = (short)tmp;
        tmp = _mm_extract_epi32(dst_9_11_13_15, 2);
        dst[13 * line] = (short)tmp;
        tmp = _mm_extract_epi32(dst_9_11_13_15, 3);
        dst[15 * line] = (short)tmp;

        __m128i dst_tmp52 = _mm_mullo_epi32(c32_82_n46_n73_61, O_0_3);
        __m128i dst_tmp53 = _mm_mullo_epi32(c32_90_n13_n88_31, O_4_7);
        __m128i dst_tmp54 = _mm_mullo_epi32(c32_85_22_n90_n4, O_8_11);
        __m128i dst_tmp55 = _mm_mullo_epi32(c32_67_54_n78_n38, O_12_15);

        dst_tmp52 = _mm_add_epi32(dst_tmp52, _mm_srli_si128(dst_tmp52, 8));
        dst_tmp52 = _mm_add_epi32(dst_tmp52, _mm_srli_si128(dst_tmp52, 4));
        int dst_17xline_lo1 = _mm_cvtsi128_si32(dst_tmp52);

        dst_tmp53 = _mm_add_epi32(dst_tmp53, _mm_srli_si128(dst_tmp53, 8));
        dst_tmp53 = _mm_add_epi32(dst_tmp53, _mm_srli_si128(dst_tmp53, 4));
        int dst_17xline_lo2 = _mm_cvtsi128_si32(dst_tmp53);

        dst_tmp54 = _mm_add_epi32(dst_tmp54, _mm_srli_si128(dst_tmp54, 8));
        dst_tmp54 = _mm_add_epi32(dst_tmp54, _mm_srli_si128(dst_tmp54, 4));
        int dst_17xline_hi1 = _mm_cvtsi128_si32(dst_tmp54);

        dst_tmp55 = _mm_add_epi32(dst_tmp55, _mm_srli_si128(dst_tmp55, 8));
        dst_tmp55 = _mm_add_epi32(dst_tmp55, _mm_srli_si128(dst_tmp55, 4));
        int dst_17xline_hi2 = _mm_cvtsi128_si32(dst_tmp55);

        int dst_17xline =  dst_17xline_lo1 + dst_17xline_lo2 + dst_17xline_hi1 + dst_17xline_hi2;

        __m128i dst_tmp56 = _mm_mullo_epi32(c32_88_n4_n85_54, O_0_3);
        __m128i dst_tmp57 = _mm_mullo_epi32(c32_13_82_n61_n46, O_4_7);
        __m128i dst_tmp58 = _mm_mullo_epi32(c32_n78_67_38_n90, O_8_11);
        __m128i dst_tmp59 = _mm_mullo_epi32(c32_n73_n31_90_n22, O_12_15);

        dst_tmp56 = _mm_add_epi32(dst_tmp56, _mm_srli_si128(dst_tmp56, 8));
        dst_tmp56 = _mm_add_epi32(dst_tmp56, _mm_srli_si128(dst_tmp56, 4));
        int dst_19xline_lo1 = _mm_cvtsi128_si32(dst_tmp56);

        dst_tmp57 = _mm_add_epi32(dst_tmp57, _mm_srli_si128(dst_tmp57, 8));
        dst_tmp57 = _mm_add_epi32(dst_tmp57, _mm_srli_si128(dst_tmp57, 4));
        int dst_19xline_lo2 = _mm_cvtsi128_si32(dst_tmp57);

        dst_tmp58 = _mm_add_epi32(dst_tmp58, _mm_srli_si128(dst_tmp58, 8));
        dst_tmp58 = _mm_add_epi32(dst_tmp58, _mm_srli_si128(dst_tmp58, 4));
        int dst_19xline_hi1 = _mm_cvtsi128_si32(dst_tmp58);

        dst_tmp59 = _mm_add_epi32(dst_tmp59, _mm_srli_si128(dst_tmp59, 8));
        dst_tmp59 = _mm_add_epi32(dst_tmp59, _mm_srli_si128(dst_tmp59, 4));
        int dst_19xline_hi2 = _mm_cvtsi128_si32(dst_tmp59);

        int dst_19xline =  dst_19xline_lo1 + dst_19xline_lo2 + dst_19xline_hi1 + dst_19xline_hi2;

        __m128i dst_tmp60 = _mm_mullo_epi32(c32_54_38_n90_46, O_0_3);
        __m128i dst_tmp61 = _mm_mullo_epi32(c32_n88_61_31_n90, O_4_7);
        __m128i dst_tmp62 = _mm_mullo_epi32(c32_13_n85_67_22, O_8_11);
        __m128i dst_tmp63 = _mm_mullo_epi32(c32_73_n82_4_78, O_12_15);

        dst_tmp60 = _mm_add_epi32(dst_tmp60, _mm_srli_si128(dst_tmp60, 8));
        dst_tmp60 = _mm_add_epi32(dst_tmp60, _mm_srli_si128(dst_tmp60, 4));
        int dst_21xline_lo1 = _mm_cvtsi128_si32(dst_tmp60);

        dst_tmp61 = _mm_add_epi32(dst_tmp61, _mm_srli_si128(dst_tmp61, 8));
        dst_tmp61 = _mm_add_epi32(dst_tmp61, _mm_srli_si128(dst_tmp61, 4));
        int dst_21xline_lo2 = _mm_cvtsi128_si32(dst_tmp61);

        dst_tmp62 = _mm_add_epi32(dst_tmp62, _mm_srli_si128(dst_tmp62, 8));
        dst_tmp62 = _mm_add_epi32(dst_tmp62, _mm_srli_si128(dst_tmp62, 4));
        int dst_21xline_hi1 = _mm_cvtsi128_si32(dst_tmp62);

        dst_tmp63 = _mm_add_epi32(dst_tmp63, _mm_srli_si128(dst_tmp63, 8));
        dst_tmp63 = _mm_add_epi32(dst_tmp63, _mm_srli_si128(dst_tmp63, 4));
        int dst_21xline_hi2 = _mm_cvtsi128_si32(dst_tmp63);

        int dst_21xline =  dst_21xline_lo1 + dst_21xline_lo2 + dst_21xline_hi1 + dst_21xline_hi2;

        __m128i dst_tmp64 = _mm_mullo_epi32(c32_n4_73_n88_38, O_0_3);
        __m128i dst_tmp65 = _mm_mullo_epi32(c32_n31_n46_90_n67, O_4_7);
        __m128i dst_tmp66 = _mm_mullo_epi32(c32_61_13_n78_85, O_8_11);
        __m128i dst_tmp67 = _mm_mullo_epi32(c32_n82_22_54_n90, O_12_15);

        dst_tmp64 = _mm_add_epi32(dst_tmp64, _mm_srli_si128(dst_tmp64, 8));
        dst_tmp64 = _mm_add_epi32(dst_tmp64, _mm_srli_si128(dst_tmp64, 4));
        int dst_23xline_lo1 = _mm_cvtsi128_si32(dst_tmp64);

        dst_tmp65 = _mm_add_epi32(dst_tmp65, _mm_srli_si128(dst_tmp65, 8));
        dst_tmp65 = _mm_add_epi32(dst_tmp65, _mm_srli_si128(dst_tmp65, 4));
        int dst_23xline_lo2 = _mm_cvtsi128_si32(dst_tmp65);

        dst_tmp66 = _mm_add_epi32(dst_tmp66, _mm_srli_si128(dst_tmp66, 8));
        dst_tmp66 = _mm_add_epi32(dst_tmp66, _mm_srli_si128(dst_tmp66, 4));
        int dst_23xline_hi1 = _mm_cvtsi128_si32(dst_tmp66);

        dst_tmp67 = _mm_add_epi32(dst_tmp67, _mm_srli_si128(dst_tmp67, 8));
        dst_tmp67 = _mm_add_epi32(dst_tmp67, _mm_srli_si128(dst_tmp67, 4));
        int dst_23xline_hi2 = _mm_cvtsi128_si32(dst_tmp67);

        int dst_23xline =  dst_23xline_lo1 + dst_23xline_lo2 + dst_23xline_hi1 + dst_23xline_hi2;

        __m128i dst_17_19_21_23 = _mm_set_epi32(dst_23xline, dst_21xline, dst_19xline, dst_17xline);
        dst_17_19_21_23  = _mm_srai_epi32(_mm_add_epi32(c32_add, dst_17_19_21_23), nshift);

        tmp = _mm_extract_epi32(dst_17_19_21_23, 0);
        dst[17 * line] = (short)tmp;
        tmp = _mm_extract_epi32(dst_17_19_21_23, 1);
        dst[19 * line] = (short)tmp;
        tmp = _mm_extract_epi32(dst_17_19_21_23, 2);
        dst[21 * line] = (short)tmp;
        tmp = _mm_extract_epi32(dst_17_19_21_23, 3);
        dst[23 * line] = (short)tmp;

        __m128i dst_tmp68 = _mm_mullo_epi32(c32_n61_90_n78_31, O_0_3);
        __m128i dst_tmp69 = _mm_mullo_epi32(c32_82_n88_54_4, O_4_7);
        __m128i dst_tmp70 = _mm_mullo_epi32(c32_n90_73_n22_n38, O_8_11);
        __m128i dst_tmp71 = _mm_mullo_epi32(c32_85_n46_n13_67, O_12_15);

        dst_tmp68 = _mm_add_epi32(dst_tmp68, _mm_srli_si128(dst_tmp68, 8));
        dst_tmp68 = _mm_add_epi32(dst_tmp68, _mm_srli_si128(dst_tmp68, 4));
        int dst_25xline_lo1 = _mm_cvtsi128_si32(dst_tmp68);

        dst_tmp69 = _mm_add_epi32(dst_tmp69, _mm_srli_si128(dst_tmp69, 8));
        dst_tmp69 = _mm_add_epi32(dst_tmp69, _mm_srli_si128(dst_tmp69, 4));
        int dst_25xline_lo2 = _mm_cvtsi128_si32(dst_tmp69);

        dst_tmp70 = _mm_add_epi32(dst_tmp70, _mm_srli_si128(dst_tmp70, 8));
        dst_tmp70 = _mm_add_epi32(dst_tmp70, _mm_srli_si128(dst_tmp70, 4));
        int dst_25xline_hi1 = _mm_cvtsi128_si32(dst_tmp70);

        dst_tmp71 = _mm_add_epi32(dst_tmp71, _mm_srli_si128(dst_tmp71, 8));
        dst_tmp71 = _mm_add_epi32(dst_tmp71, _mm_srli_si128(dst_tmp71, 4));
        int dst_25xline_hi2 = _mm_cvtsi128_si32(dst_tmp71);

        int dst_25xline =  dst_25xline_lo1 + dst_25xline_lo2 + dst_25xline_hi1 + dst_25xline_hi2;

        __m128i dst_tmp72 = _mm_mullo_epi32(c32_n90_85_n61_22, O_0_3);
        __m128i dst_tmp73 = _mm_mullo_epi32(c32_46_n4_n38_73, O_4_7);
        __m128i dst_tmp74 = _mm_mullo_epi32(c32_54_n82_90_n78, O_8_11);
        __m128i dst_tmp75 = _mm_mullo_epi32(c32_n88_67_n31_n13, O_12_15);

        dst_tmp72 = _mm_add_epi32(dst_tmp72, _mm_srli_si128(dst_tmp72, 8));
        dst_tmp72 = _mm_add_epi32(dst_tmp72, _mm_srli_si128(dst_tmp72, 4));
        int dst_27xline_lo1 = _mm_cvtsi128_si32(dst_tmp72);

        dst_tmp73 = _mm_add_epi32(dst_tmp73, _mm_srli_si128(dst_tmp73, 8));
        dst_tmp73 = _mm_add_epi32(dst_tmp73, _mm_srli_si128(dst_tmp73, 4));
        int dst_27xline_lo2 = _mm_cvtsi128_si32(dst_tmp73);

        dst_tmp74 = _mm_add_epi32(dst_tmp74, _mm_srli_si128(dst_tmp74, 8));
        dst_tmp74 = _mm_add_epi32(dst_tmp74, _mm_srli_si128(dst_tmp74, 4));
        int dst_27xline_hi1 = _mm_cvtsi128_si32(dst_tmp74);

        dst_tmp75 = _mm_add_epi32(dst_tmp75, _mm_srli_si128(dst_tmp75, 8));
        dst_tmp75 = _mm_add_epi32(dst_tmp75, _mm_srli_si128(dst_tmp75, 4));
        int dst_27xline_hi2 = _mm_cvtsi128_si32(dst_tmp75);

        int dst_27xline =  dst_27xline_lo1 + dst_27xline_lo2 + dst_27xline_hi1 + dst_27xline_hi2;

        __m128i dst_tmp76 = _mm_mullo_epi32(c32_n78_61_n38_13, O_0_3);
        __m128i dst_tmp77 = _mm_mullo_epi32(c32_88_n90_85_n73, O_4_7);
        __m128i dst_tmp78 = _mm_mullo_epi32(c32_22_4_n31_54, O_8_11);
        __m128i dst_tmp79 = _mm_mullo_epi32(c32_90_n82_67_n46, O_12_15);

        dst_tmp76 = _mm_add_epi32(dst_tmp76, _mm_srli_si128(dst_tmp76, 8));
        dst_tmp76 = _mm_add_epi32(dst_tmp76, _mm_srli_si128(dst_tmp76, 4));
        int dst_29xline_lo1 = _mm_cvtsi128_si32(dst_tmp76);

        dst_tmp77 = _mm_add_epi32(dst_tmp77, _mm_srli_si128(dst_tmp77, 8));
        dst_tmp77 = _mm_add_epi32(dst_tmp77, _mm_srli_si128(dst_tmp77, 4));
        int dst_29xline_lo2 = _mm_cvtsi128_si32(dst_tmp77);

        dst_tmp78 = _mm_add_epi32(dst_tmp78, _mm_srli_si128(dst_tmp78, 8));
        dst_tmp78 = _mm_add_epi32(dst_tmp78, _mm_srli_si128(dst_tmp78, 4));
        int dst_29xline_hi1 = _mm_cvtsi128_si32(dst_tmp78);

        dst_tmp79 = _mm_add_epi32(dst_tmp79, _mm_srli_si128(dst_tmp79, 8));
        dst_tmp79 = _mm_add_epi32(dst_tmp79, _mm_srli_si128(dst_tmp79, 4));
        int dst_29xline_hi2 = _mm_cvtsi128_si32(dst_tmp79);

        int dst_29xline =  dst_29xline_lo1 + dst_29xline_lo2 + dst_29xline_hi1 + dst_29xline_hi2;

        __m128i dst_tmp80 = _mm_mullo_epi32(c32_n31_22_n13_4, O_0_3);
        __m128i dst_tmp81 = _mm_mullo_epi32(c32_n61_54_n46_38, O_4_7);
        __m128i dst_tmp82 = _mm_mullo_epi32(c32_n82_78_n73_67, O_8_11);
        __m128i dst_tmp83 = _mm_mullo_epi32(c32_n90_90_n88_85, O_12_15);

        dst_tmp80 = _mm_add_epi32(dst_tmp80, _mm_srli_si128(dst_tmp80, 8));
        dst_tmp80 = _mm_add_epi32(dst_tmp80, _mm_srli_si128(dst_tmp80, 4));
        int dst_31xline_lo1 = _mm_cvtsi128_si32(dst_tmp80);

        dst_tmp81 = _mm_add_epi32(dst_tmp81, _mm_srli_si128(dst_tmp81, 8));
        dst_tmp81 = _mm_add_epi32(dst_tmp81, _mm_srli_si128(dst_tmp81, 4));
        int dst_31xline_lo2 = _mm_cvtsi128_si32(dst_tmp81);

        dst_tmp82 = _mm_add_epi32(dst_tmp82, _mm_srli_si128(dst_tmp82, 8));
        dst_tmp82 = _mm_add_epi32(dst_tmp82, _mm_srli_si128(dst_tmp82, 4));
        int dst_31xline_hi1 = _mm_cvtsi128_si32(dst_tmp82);

        dst_tmp83 = _mm_add_epi32(dst_tmp83, _mm_srli_si128(dst_tmp83, 8));
        dst_tmp83 = _mm_add_epi32(dst_tmp83, _mm_srli_si128(dst_tmp83, 4));
        int dst_31xline_hi2 = _mm_cvtsi128_si32(dst_tmp83);

        int dst_31xline =  dst_31xline_lo1 + dst_31xline_lo2 + dst_31xline_hi1 + dst_31xline_hi2;

        __m128i dst_25_27_29_31 = _mm_set_epi32(dst_31xline, dst_29xline, dst_27xline, dst_25xline);
        dst_25_27_29_31  = _mm_srai_epi32(_mm_add_epi32(c32_add, dst_25_27_29_31), nshift);

        tmp = _mm_extract_epi32(dst_25_27_29_31, 0);
        dst[25 * line] = (short)tmp;
        tmp = _mm_extract_epi32(dst_25_27_29_31, 1);
        dst[27 * line] = (short)tmp;
        tmp = _mm_extract_epi32(dst_25_27_29_31, 2);
        dst[29 * line] = (short)tmp;
        tmp = _mm_extract_epi32(dst_25_27_29_31, 3);
        dst[31 * line] = (short)tmp;

        src += 32;
        dst++;
    }
}

#else // INSTRSET <= 4

void CDECL partialButterfly32(short *src, short *dst, int shift, int line)
{
    int j;
    int add = 1 << (shift - 1);

    Vec4i g_aiT_zero_row_first_two(64, 64, 0, 0);
    Vec4i g_aiT_eight_row_first_two(83, 36, 0, 0);
    Vec4i g_aiT_sixten_row_first_two(64, -64, 0, 0);
    Vec4i g_aiT_twentyfour_row_first_two(36, -83, 0, 0);

    Vec4i g_aiT_four_row_first_four(89, 75, 50, 18);
    Vec4i g_aiT_twelve_row_first_four(75, -18, -89, -50);
    Vec4i g_aiT_twenty_row_first_four(50, -89, 18, 75);
    Vec4i g_aiT_twentyeight_row_first_four(18, -50, 75, -89);

    Vec4i g_aiT_two_row_first_four(90, 87, 80, 70);
    Vec4i g_aiT_two_row_second_four(57, 43, 25,  9);
    Vec4i g_aiT_six_row_first_four(87, 57,  9, -43);
    Vec4i g_aiT_six_row_second_four(-80, -90, -70, -25);
    Vec4i g_aiT_ten_row_first_four(80,  9, -70, -87);
    Vec4i g_aiT_ten_row_second_four(-25, 57, 90, 43);
    Vec4i g_aiT_fourteen_row_first_four(70, -43, -87,  9);
    Vec4i g_aiT_fourteen_row_second_four(90, 25, -80, -57);
    Vec4i g_aiT_eighteen_row_first_four(57, -80, -25, 90);
    Vec4i g_aiT_eighteen_row_second_four(-9, -87, 43, 70);
    Vec4i g_aiT_twentytwo_row_first_four(43, -90, 57, 25);
    Vec4i g_aiT_twentytwo_row_second_four(-87, 70,  9, -80);
    Vec4i g_aiT_twentysix_row_first_four(25, -70, 90, -80);
    Vec4i g_aiT_twentysix_row_second_four(43,  9, -57, 87);
    Vec4i g_aiT_thirty_row_first_four(9, -25, 43, -57);
    Vec4i g_aiT_thirty_row_second_four(70, -80, 87, -90);

    Vec4i g_aiT_one_row_first_four(90, 90, 88, 85);
    Vec4i g_aiT_one_row_second_four(82, 78, 73, 67);
    Vec4i g_aiT_one_row_third_four(61, 54, 46, 38);
    Vec4i g_aiT_one_row_fourth_four(31, 22, 13,  4);

    Vec4i g_aiT_three_row_first_four(90, 82, 67, 46);
    Vec4i g_aiT_three_row_second_four(22, -4, -31, -54);
    Vec4i g_aiT_three_row_third_four(-73, -85, -90, -88);
    Vec4i g_aiT_three_row_fourth_four(-78, -61, -38, -13);

    Vec4i g_aiT_five_row_first_four(88, 67, 31, -13);
    Vec4i g_aiT_five_row_second_four(-54, -82, -90, -78);
    Vec4i g_aiT_five_row_third_four(-46, -4, 38, 73);
    Vec4i g_aiT_five_row_fourth_four(90, 85, 61, 22);

    Vec4i g_aiT_seven_row_first_four(85, 46, -13, -67);
    Vec4i g_aiT_seven_row_second_four(-90, -73, -22, 38);
    Vec4i g_aiT_seven_row_third_four(82, 88, 54, -4);
    Vec4i g_aiT_seven_row_fourth_four(-61, -90, -78, -31);

    Vec4i g_aiT_nine_row_first_four(82, 22, -54, -90);
    Vec4i g_aiT_nine_row_second_four(-61, 13, 78, 85);
    Vec4i g_aiT_nine_row_third_four(31, -46, -90, -67);
    Vec4i g_aiT_nine_row_fourth_four(4, 73, 88, 38);

    Vec4i g_aiT_eleven_row_first_four(78, -4, -82, -73);
    Vec4i g_aiT_eleven_row_second_four(13, 85, 67, -22);
    Vec4i g_aiT_eleven_row_third_four(-88, -61, 31, 90);
    Vec4i g_aiT_eleven_row_fourth_four(54, -38, -90, -46);

    Vec4i g_aiT_thirteen_row_first_four(73, -31, -90, -22);
    Vec4i g_aiT_thirteen_row_second_four(78, 67, -38, -90);
    Vec4i g_aiT_thirteen_row_third_four(-13, 82, 61, -46);
    Vec4i g_aiT_thirteen_row_fourth_four(-88, -4, 85, 54);

    Vec4i g_aiT_fifteen_row_first_four(67, -54, -78, 38);
    Vec4i g_aiT_fifteen_row_second_four(85, -22, -90,  4);
    Vec4i g_aiT_fifteen_row_third_four(90, 13, -88, -31);
    Vec4i g_aiT_fifteen_row_fourth_four(82, 46, -73, -61);

    Vec4i g_aiT_seventeen_row_first_four(61, -73, -46, 82);
    Vec4i g_aiT_seventeen_row_second_four(31, -88, -13, 90);
    Vec4i g_aiT_seventeen_row_third_four(-4, -90, 22, 85);
    Vec4i g_aiT_seventeen_row_fourth_four(-38, -78, 54, 67);

    Vec4i g_aiT_nineteen_row_first_four(54, -85, -4, 88);
    Vec4i g_aiT_nineteen_row_second_four(-46, -61, 82, 13);
    Vec4i g_aiT_nineteen_row_third_four(-90, 38, 67, -78);
    Vec4i g_aiT_nineteen_row_fourth_four(-22, 90, -31, -73);

    Vec4i g_aiT_twentyone_row_first_four(46, -90, 38, 54);
    Vec4i g_aiT_twentyone_row_second_four(-90, 31, 61, -88);
    Vec4i g_aiT_twentyone_row_third_four(22, 67, -85, 13);
    Vec4i g_aiT_twentyone_row_fourth_four(73, -82,  4, 78);

    Vec4i g_aiT_twentythree_row_first_four(38, -88, 73, -4);
    Vec4i g_aiT_twentythree_row_second_four(-67, 90, -46, -31);
    Vec4i g_aiT_twentythree_row_third_four(85, -78, 13, 61);
    Vec4i g_aiT_twentythree_row_fourth_four(-90, 54, 22, -82);

    Vec4i g_aiT_twentyfive_row_first_four(31, -78, 90, -61);
    Vec4i g_aiT_twentyfive_row_second_four(4, 54, -88, 82);
    Vec4i g_aiT_twentyfive_row_third_four(-38, -22, 73, -90);
    Vec4i g_aiT_twentyfive_row_fourth_four(67, -13, -46, 85);

    Vec4i g_aiT_twentyseven_row_first_four(22, -61, 85, -90);
    Vec4i g_aiT_twentyseven_row_second_four(73, -38, -4, 46);
    Vec4i g_aiT_twentyseven_row_third_four(-78, 90, -82, 54);
    Vec4i g_aiT_twentyseven_row_fourth_four(-13, -31, 67, -88);

    Vec4i g_aiT_twentynine_row_first_four(13, -38, 61, -78);
    Vec4i g_aiT_twentynine_row_second_four(88, -90, 85, -73);
    Vec4i g_aiT_twentynine_row_third_four(54, -31,  4, 22);
    Vec4i g_aiT_twentynine_row_fourth_four(-46, 67, -82, 90);

    Vec4i g_aiT_thirtyone_row_first_four(4, -13, 22, -31);
    Vec4i g_aiT_thirtyone_row_second_four(38, -46, 54, -61);
    Vec4i g_aiT_thirtyone_row_third_four(67, -73, 78, -82);
    Vec4i g_aiT_thirtyone_row_fourth_four(85, -88, 90, -90);

    for (j = 0; j < line; j++)
    {
        Vec8s tmp1, tmp2, tmp3, tmp4;

        tmp1.load(src);
        Vec4i tmp1_first_half = extend_low(tmp1);
        Vec4i tmp1_second_half = extend_high(tmp1);

        tmp2.load(src + 8);
        Vec4i tmp2_first_half = extend_low(tmp2);
        Vec4i tmp2_second_half = extend_high(tmp2);

        tmp3.load(src + 16);
        Vec4i tmp3_first_half_tmp = extend_low(tmp3);
        Vec4i tmp3_second_half_tmp = extend_high(tmp3);
        Vec4i tmp3_first_half = permute4i<3, 2, 1, 0>(tmp3_first_half_tmp);
        Vec4i tmp3_second_half = permute4i<3, 2, 1, 0>(tmp3_second_half_tmp);

        tmp4.load(src + 24);
        Vec4i tmp4_first_half_tmp = extend_low(tmp4);
        Vec4i tmp4_second_half_tmp = extend_high(tmp4);
        Vec4i tmp4_first_half = permute4i<3, 2, 1, 0>(tmp4_first_half_tmp);
        Vec4i tmp4_second_half = permute4i<3, 2, 1, 0>(tmp4_second_half_tmp);

        Vec4i E_first_four =  tmp1_first_half + tmp4_second_half;
        Vec4i E_second_four = tmp1_second_half + tmp4_first_half;
        Vec4i E_third_four = tmp2_first_half + tmp3_second_half;
        Vec4i E_last_four = tmp2_second_half + tmp3_first_half;

        Vec4i O_first_four =  tmp1_first_half - tmp4_second_half;
        Vec4i O_second_four = tmp1_second_half - tmp4_first_half;
        Vec4i O_third_four = tmp2_first_half - tmp3_second_half;
        Vec4i O_last_four = tmp2_second_half - tmp3_first_half;

        Vec4i E_last_four_rev = permute4i<3, 2, 1, 0>(E_last_four);
        Vec4i E_third_four_rev = permute4i<3, 2, 1, 0>(E_third_four);

        Vec4i EE_first_four = E_first_four + E_last_four_rev;
        Vec4i EE_last_four = E_second_four + E_third_four_rev;
        Vec4i EO_first_four = E_first_four - E_last_four_rev;
        Vec4i EO_last_four = E_second_four - E_third_four_rev;

        Vec4i EE_last_four_rev = permute4i<3, 2, 1, 0>(EE_last_four);

        Vec4i EEE = EE_first_four + EE_last_four_rev;
        Vec4i EEO = EE_first_four - EE_last_four_rev;

        Vec4i EEEE_first_half = permute4i<0, 1, -1, -1>(EEE);
        Vec4i EEEE_second_half = permute4i<3, 2, -1, -1>(EEE);
        Vec4i EEEE = EEEE_first_half + EEEE_second_half;
        Vec4i EEEO = EEEE_first_half - EEEE_second_half;

        int dst0_hresult = (horizontal_add(g_aiT_zero_row_first_two * EEEE) + add) >> shift;
        int dst8_hresult = (horizontal_add(g_aiT_eight_row_first_two * EEEO) + add) >> shift;
        int dst16_hresult = (horizontal_add(g_aiT_sixten_row_first_two * EEEE) + add) >> shift;
        int dst24_hresult = (horizontal_add(g_aiT_twentyfour_row_first_two * EEEO) + add) >> shift;

        dst[0] = dst0_hresult;
        dst[8 * line] = dst8_hresult;
        dst[16 * line] = dst16_hresult;
        dst[24 * line] = dst24_hresult;

        int dst4_hresult = (horizontal_add(g_aiT_four_row_first_four * EEO) + add) >> shift;
        int dst12_hresult = (horizontal_add(g_aiT_twelve_row_first_four * EEO) + add) >> shift;
        int dst20_hresult = (horizontal_add(g_aiT_twenty_row_first_four * EEO) + add) >> shift;
        int dst28_hresult = (horizontal_add(g_aiT_twentyeight_row_first_four * EEO) + add) >> shift;

        dst[4 * line] = dst4_hresult;
        dst[12 * line] = dst12_hresult;
        dst[20 * line] = dst20_hresult;
        dst[28 * line] = dst28_hresult;

        int dst2_hresult =
            (horizontal_add((g_aiT_two_row_first_four *
                             EO_first_four) + (g_aiT_two_row_second_four * EO_last_four)) + add) >> shift;
        int dst6_hresult =
            (horizontal_add((g_aiT_six_row_first_four *
                             EO_first_four) + (g_aiT_six_row_second_four * EO_last_four)) + add) >> shift;
        int dst10_hresult =
            (horizontal_add((g_aiT_ten_row_first_four *
                             EO_first_four) + (g_aiT_ten_row_second_four * EO_last_four)) + add) >> shift;
        int dst14_hresult =
            (horizontal_add((g_aiT_fourteen_row_first_four *
                             EO_first_four) + (g_aiT_fourteen_row_second_four * EO_last_four)) + add) >> shift;
        int dst18_hresult =
            (horizontal_add((g_aiT_eighteen_row_first_four *
                             EO_first_four) + (g_aiT_eighteen_row_second_four * EO_last_four)) + add) >> shift;
        int dst22_hresult =
            (horizontal_add((g_aiT_twentytwo_row_first_four *
                             EO_first_four) + (g_aiT_twentytwo_row_second_four * EO_last_four)) + add) >> shift;
        int dst26_hresult =
            (horizontal_add((g_aiT_twentysix_row_first_four *
                             EO_first_four) + (g_aiT_twentysix_row_second_four * EO_last_four)) + add) >> shift;
        int dst30_hresult =
            (horizontal_add((g_aiT_thirty_row_first_four *
                             EO_first_four) + (g_aiT_thirty_row_second_four * EO_last_four)) + add) >> shift;

        dst[2 * line] = dst2_hresult;
        dst[6 * line] = dst6_hresult;
        dst[10 * line] = dst10_hresult;
        dst[14 * line] = dst14_hresult;
        dst[18 * line] = dst18_hresult;
        dst[22 * line] = dst22_hresult;
        dst[26 * line] = dst26_hresult;
        dst[30 * line] = dst30_hresult;

        Vec4i dst1_temp = (g_aiT_one_row_first_four * O_first_four) + (g_aiT_one_row_second_four * O_second_four) +
            (g_aiT_one_row_third_four * O_third_four) + (g_aiT_one_row_fourth_four * O_last_four);
        Vec4i dst3_temp = (g_aiT_three_row_first_four * O_first_four) + (g_aiT_three_row_second_four * O_second_four) +
            (g_aiT_three_row_third_four * O_third_four) + (g_aiT_three_row_fourth_four * O_last_four);
        Vec4i dst5_temp = (g_aiT_five_row_first_four * O_first_four) + (g_aiT_five_row_second_four * O_second_four) +
            (g_aiT_five_row_third_four * O_third_four) + (g_aiT_five_row_fourth_four * O_last_four);
        Vec4i dst7_temp = (g_aiT_seven_row_first_four * O_first_four) + (g_aiT_seven_row_second_four * O_second_four) +
            (g_aiT_seven_row_third_four * O_third_four) + (g_aiT_seven_row_fourth_four * O_last_four);
        Vec4i dst9_temp = (g_aiT_nine_row_first_four * O_first_four) + (g_aiT_nine_row_second_four * O_second_four) +
            (g_aiT_nine_row_third_four * O_third_four) + (g_aiT_nine_row_fourth_four * O_last_four);
        Vec4i dst11_temp = (g_aiT_eleven_row_first_four * O_first_four) + (g_aiT_eleven_row_second_four * O_second_four) +
            (g_aiT_eleven_row_third_four * O_third_four) + (g_aiT_eleven_row_fourth_four * O_last_four);
        Vec4i dst13_temp = (g_aiT_thirteen_row_first_four * O_first_four) + (g_aiT_thirteen_row_second_four * O_second_four) +
            (g_aiT_thirteen_row_third_four * O_third_four) + (g_aiT_thirteen_row_fourth_four * O_last_four);
        Vec4i dst15_temp = (g_aiT_fifteen_row_first_four * O_first_four) + (g_aiT_fifteen_row_second_four * O_second_four) +
            (g_aiT_fifteen_row_third_four * O_third_four) + (g_aiT_fifteen_row_fourth_four * O_last_four);
        Vec4i dst17_temp = (g_aiT_seventeen_row_first_four * O_first_four) + (g_aiT_seventeen_row_second_four * O_second_four) +
            (g_aiT_seventeen_row_third_four * O_third_four) + (g_aiT_seventeen_row_fourth_four * O_last_four);
        Vec4i dst19_temp = (g_aiT_nineteen_row_first_four * O_first_four) + (g_aiT_nineteen_row_second_four * O_second_four) +
            (g_aiT_nineteen_row_third_four * O_third_four) + (g_aiT_nineteen_row_fourth_four * O_last_four);
        Vec4i dst21_temp = (g_aiT_twentyone_row_first_four * O_first_four) + (g_aiT_twentyone_row_second_four * O_second_four) +
            (g_aiT_twentyone_row_third_four * O_third_four) + (g_aiT_twentyone_row_fourth_four * O_last_four);
        Vec4i dst23_temp =
            (g_aiT_twentythree_row_first_four * O_first_four) + (g_aiT_twentythree_row_second_four * O_second_four) +
            (g_aiT_twentythree_row_third_four * O_third_four) + (g_aiT_twentythree_row_fourth_four * O_last_four);
        Vec4i dst25_temp =
            (g_aiT_twentyfive_row_first_four * O_first_four) + (g_aiT_twentyfive_row_second_four * O_second_four) +
            (g_aiT_twentyfive_row_third_four * O_third_four) + (g_aiT_twentyfive_row_fourth_four * O_last_four);
        Vec4i dst27_temp =
            (g_aiT_twentyseven_row_first_four * O_first_four) + (g_aiT_twentyseven_row_second_four * O_second_four) +
            (g_aiT_twentyseven_row_third_four * O_third_four) + (g_aiT_twentyseven_row_fourth_four * O_last_four);
        Vec4i dst29_temp =
            (g_aiT_twentynine_row_first_four * O_first_four) + (g_aiT_twentynine_row_second_four * O_second_four) +
            (g_aiT_twentynine_row_third_four * O_third_four) + (g_aiT_twentynine_row_fourth_four * O_last_four);
        Vec4i dst31_temp = (g_aiT_thirtyone_row_first_four * O_first_four) + (g_aiT_thirtyone_row_second_four * O_second_four) +
            (g_aiT_thirtyone_row_third_four * O_third_four) + (g_aiT_thirtyone_row_fourth_four * O_last_four);

        dst[1 * line] = (horizontal_add(dst1_temp) + add) >> shift;
        dst[3 * line] = (horizontal_add(dst3_temp) + add) >> shift;
        dst[5 * line] = (horizontal_add(dst5_temp) + add) >> shift;
        dst[7 * line] = (horizontal_add(dst7_temp) + add) >> shift;
        dst[9 * line] = (horizontal_add(dst9_temp) + add) >> shift;
        dst[11 * line] = (horizontal_add(dst11_temp) + add) >> shift;
        dst[13 * line] = (horizontal_add(dst13_temp) + add) >> shift;
        dst[15 * line] = (horizontal_add(dst15_temp) + add) >> shift;
        dst[17 * line] = (horizontal_add(dst17_temp) + add) >> shift;
        dst[19 * line] = (horizontal_add(dst19_temp) + add) >> shift;
        dst[21 * line] = (horizontal_add(dst21_temp) + add) >> shift;
        dst[23 * line] = (horizontal_add(dst23_temp) + add) >> shift;
        dst[25 * line] = (horizontal_add(dst25_temp) + add) >> shift;
        dst[27 * line] = (horizontal_add(dst27_temp) + add) >> shift;
        dst[29 * line] = (horizontal_add(dst29_temp) + add) >> shift;
        dst[31 * line] = (horizontal_add(dst31_temp) + add) >> shift;

        src += 32;
        dst++;
    }
}

#endif  //partialButterfly32 vector code

void CDECL partialButterfly4(short *src, short *dst, int nshift, int /* line */)
{
    // Const
    __m128i c_1         = _mm_set1_epi32(1);
    __m128i c16_64_64   = _mm_set1_epi32(0x00400040);
    __m128i c16_n64_64  = _mm_set1_epi32(0xFFC00040);
    __m128i c16_36_83   = _mm_set1_epi32(0x00240053);
    __m128i c16_n83_36  = _mm_set1_epi32(0xFFAD0024);
    __m128i c32_128     = _mm_set1_epi32(128);
    __m128i c32_64      = _mm_set1_epi32(64);
    __m128i c32_83_36   = _mm_set_epi32(36, 83, 36, 83);
    __m128i c32_64_n64   = _mm_set_epi32(-64, 64, -64, 64);
    __m128i c32_36_n83   = _mm_set_epi32(-83, 36, -83, 36);

    __m128i T20  = _mm_loadl_epi64((const __m128i*)(src + 0)); // [03 02 01 00]
    __m128i T21  = _mm_loadl_epi64((const __m128i*)(src + 4)); // [13 12 11 10]
    __m128i T22  = _mm_loadl_epi64((const __m128i*)(src + 8)); // [23 22 21 20]
    __m128i T23  = _mm_loadl_epi64((const __m128i*)(src + 12)); // [33 32 31 30]

    // DCT1
    __m128i T30  = _mm_unpacklo_epi32(T20, T21);        // [13 12 03 02 11 10 01 00]
    __m128i T31  = _mm_unpacklo_epi32(T22, T23);        // [33 32 23 22 31 30 21 20]
    __m128i T32  = _mm_shufflehi_epi16(T30, 0xB1);      // [12 13 02 03 11 10 01 00]
    __m128i T33  = _mm_shufflehi_epi16(T31, 0xB1);      // [32 33 22 23 31 30 21 20]
    __m128i T40  = _mm_unpacklo_epi64(T32, T33);        // [31 30 21 20 11 10 01 00]
    __m128i T41  = _mm_unpackhi_epi64(T32, T33);        // [32 33 22 23 12 13 02 03]
    __m128i T50  = _mm_add_epi16(T40, T41);             // [1+2 0+3]
    __m128i T51  = _mm_sub_epi16(T40, T41);             // [1-2 0-3]
    __m128i T60  = _mm_madd_epi16(c16_64_64,  T50);     // [ 64*s12 + 64*s03] = [03 02 01 00]
    __m128i T61  = _mm_madd_epi16(c16_36_83,  T51);     // [ 36*d12 + 83*d03] = [13 12 11 10]
    __m128i T62  = _mm_madd_epi16(c16_n64_64, T50);     // [-64*s12 + 64*s03] = [23 22 21 20]
    __m128i T63  = _mm_madd_epi16(c16_n83_36, T51);     // [-83*d12 + 36*d03] = [33 32 31 30]
    __m128i T70  = _mm_srai_epi32(_mm_add_epi32(c_1, T60), nshift);  // [03 02 01 00]
    __m128i T71  = _mm_srai_epi32(_mm_add_epi32(c_1, T61), nshift);  // [13 12 11 10]
    __m128i T72  = _mm_srai_epi32(_mm_add_epi32(c_1, T62), nshift);  // [23 22 21 20]
    __m128i T73  = _mm_srai_epi32(_mm_add_epi32(c_1, T63), nshift);  // [33 32 31 30]

    // DCT2
    nshift = 2 + 6;

    __m128i c32_temp1 = _mm_slli_epi32(T70, 16);
    c32_temp1 = _mm_srai_epi32(c32_temp1, 16);

    __m128i c32_temp2 = _mm_slli_epi32(T71, 16);
    c32_temp2 = _mm_srai_epi32(c32_temp2, 16);

    __m128i c32_temp3 = _mm_slli_epi32(T72, 16);
    c32_temp3 = _mm_srai_epi32(c32_temp3, 16);

    __m128i c32_temp4 = _mm_slli_epi32(T73, 16);
    c32_temp4 = _mm_srai_epi32(c32_temp4, 16);

    __m128i Coeff1_0101 = _mm_unpacklo_epi64(c32_temp1, c32_temp2);
    __m128i Coeff1_2323 = _mm_unpackhi_epi64(c32_temp1, c32_temp2);
    Coeff1_2323 = _mm_shuffle_epi32(Coeff1_2323, 0xb1);
    __m128i Coeff2_0101 = _mm_unpacklo_epi64(c32_temp3, c32_temp4);
    __m128i Coeff2_2323 = _mm_unpackhi_epi64(c32_temp3, c32_temp4);
    Coeff2_2323 = _mm_shuffle_epi32(Coeff2_2323, 0xb1);

    __m128i E0123 = _mm_add_epi32(Coeff1_0101, Coeff1_2323);
    __m128i O0123 = _mm_sub_epi32(Coeff1_0101, Coeff1_2323);
    __m128i E4567 = _mm_add_epi32(Coeff2_0101, Coeff2_2323);
    __m128i O4567 = _mm_sub_epi32(Coeff2_0101, Coeff2_2323);

    //Co-effs 0-3
    __m128i E_0_3 = _mm_mullo_epi32(E0123, c32_64);   //  [ E0*64 E1*64 E2*64 E3*64]
    __m128i E_4_7 = _mm_mullo_epi32(E4567, c32_64);   //  [ E4*64 E5*64 E6*64 E7*64]
    __m128i Coeff_0_3 = _mm_hadd_epi32(E_0_3, E_4_7);
    Coeff_0_3 = _mm_add_epi32(Coeff_0_3, c32_128);
    Coeff_0_3 = _mm_srai_epi32(Coeff_0_3, nshift);

    //Co-effs 4-7
    __m128i O_0_3 = _mm_mullo_epi32(O0123, c32_83_36); // [O0*36 O1*83 O2*36 O3*83]
    __m128i O_4_7 = _mm_mullo_epi32(O4567, c32_83_36); // [O4*36 O5*83 O6*36 O7*83]
    __m128i Coeff_4_7 = _mm_hadd_epi32(O_0_3, O_4_7);
    Coeff_4_7 = _mm_add_epi32(Coeff_4_7, c32_128);
    Coeff_4_7 = _mm_srai_epi32(Coeff_4_7, nshift);

    Coeff_0_3 = _mm_slli_epi32(Coeff_0_3, 16);
    Coeff_4_7   = _mm_slli_epi32(Coeff_4_7, 16);
    Coeff_0_3 = _mm_srai_epi32(Coeff_0_3, 16);
    Coeff_4_7  = _mm_srai_epi32(Coeff_4_7, 16);

    // Store back the DCT results
    __m128i Coeff_0_7 = _mm_packs_epi32(Coeff_0_3, Coeff_4_7);  //Coeffs 0-7
    _mm_store_si128((__m128i*)dst, Coeff_0_7);

    //Co-effs 8-11
    __m128i E_8_11  = _mm_mullo_epi32(E0123, c32_64_n64); // [ E0*-64 E1*64 E2*-64 E3*64]
    __m128i E_12_15 = _mm_mullo_epi32(E4567, c32_64_n64); // [ E4*-64 E5*64 E6*-64 E7*64]
    __m128i Coeff_8_11 = _mm_hadd_epi32(E_8_11, E_12_15);
    Coeff_8_11 = _mm_add_epi32(Coeff_8_11, c32_128);
    Coeff_8_11 = _mm_srai_epi32(Coeff_8_11, nshift);

    //Co-effs 12-15
    __m128i O_8_11  = _mm_mullo_epi32(O0123, c32_36_n83); // [O0*-83 O1*36 O2*-83 O3*36]
    __m128i O_12_15 = _mm_mullo_epi32(O4567, c32_36_n83); // [O4*-83 O5*36 O6*-83 O7*36]
    __m128i Coeff_12_15 = _mm_hadd_epi32(O_8_11, O_12_15);
    Coeff_12_15 = _mm_add_epi32(Coeff_12_15, c32_128);
    Coeff_12_15 = _mm_srai_epi32(Coeff_12_15, nshift);

    Coeff_8_11 = _mm_slli_epi32(Coeff_8_11, 16);
    Coeff_12_15   = _mm_slli_epi32(Coeff_12_15, 16);
    Coeff_8_11 = _mm_srai_epi32(Coeff_8_11, 16);
    Coeff_12_15  = _mm_srai_epi32(Coeff_12_15, 16);

    // Store back the DCT results
    __m128i Coeff_8_15 = _mm_packs_epi32(Coeff_8_11, Coeff_12_15);   //Coeffs 8-15
    _mm_store_si128((__m128i*)(dst + 8), Coeff_8_15);
}

#if 0 // partialButterflyInverse4 vector code

void CDECL partialButterflyInverse4(short *src, short *dst, int shift, int line)
{
    int j;
    int add = 1 << (shift - 1);

    for (j = 0; j < line; j++)
    {
        int src_line = src[line];
        int src_line3 = src[3 * line];
        int src_line2_shift = (src[2 * line] << 6);
        int src_zero_shift = (src[0] << 6);

        int O_first_value = 83 * src_line + 36 * src_line3;
        int O_second_value = 36 * src_line - 83 * src_line3;
        int E_first_value = src_zero_shift + src_line2_shift;
        int E_second_value = src_zero_shift - src_line2_shift;

        int first_value = E_first_value + O_first_value;
        int second_value = E_second_value + O_second_value;
        int third_value = E_second_value - O_second_value;
        int fourth_value = E_first_value - O_first_value;

        Vec4i dst_third(first_value, second_value, third_value, fourth_value);
        dst_third = (dst_third + add) >> shift;
        Vec4i all_zero(0);

        Vec8s final_value = compress_saturated(dst_third, all_zero);

        final_value.store_partial(4, dst);

        src++;
        dst += 4;
    }
}

#endif  // partialButterflyInverse4 vector code

#if 1 // partialButterflyInverse4 intrinsic code

void CDECL partialButterflyInverse4(short *src, short *dst, int shift, int line)
{
    int j;
    int add = 1 << (shift - 1);
    __m128i c_add = _mm_set1_epi32(add);

    for (j = 0; j < (line / 2); j++)
    {
        int src_line = src[line];
        int src_line3 = src[3 * line];
        int src_line2_shift = (src[2 * line] << 6);
        int src_zero_shift = (src[0] << 6);

        int O_first_value = 83 * src_line + 36 * src_line3;
        int O_second_value = 36 * src_line - 83 * src_line3;
        int E_first_value = src_zero_shift + src_line2_shift;
        int E_second_value = src_zero_shift - src_line2_shift;

        int first_value = E_first_value + O_first_value;
        int second_value = E_second_value + O_second_value;
        int third_value = E_second_value - O_second_value;
        int fourth_value = E_first_value - O_first_value;

        __m128i sum_diff_value = _mm_set_epi32(fourth_value, third_value, second_value, first_value);
        __m128i dst_third = _mm_srai_epi32(_mm_add_epi32(c_add, sum_diff_value), shift);

        src++;

        src_line = src[line];
        src_line3 = src[3 * line];
        src_line2_shift = (src[2 * line] << 6);
        src_zero_shift = (src[0] << 6);

        O_first_value = 83 * src_line + 36 * src_line3;
        O_second_value = 36 * src_line - 83 * src_line3;
        E_first_value = src_zero_shift + src_line2_shift;
        E_second_value = src_zero_shift - src_line2_shift;

        first_value = E_first_value + O_first_value;
        second_value = E_second_value + O_second_value;
        third_value = E_second_value - O_second_value;
        fourth_value = E_first_value - O_first_value;

        sum_diff_value = _mm_set_epi32(fourth_value, third_value, second_value, first_value);
        __m128i dst_third1 = _mm_srai_epi32(_mm_add_epi32(c_add, sum_diff_value), shift);

        __m128i dst_tmp_final = _mm_packs_epi32(dst_third, dst_third1);
        _mm_store_si128((__m128i*)(dst), dst_tmp_final);

        src++;
        dst += 8;
    }
}

#endif  // partialButterflyInverse4 intrinsic code

#if 0 // partialButterflyInverse8 vector code

void CDECL partialButterflyInverse8(short *src, short *dst, int shift, int line)
{
    int j;
    int E[4];
    int EE[2], EO[2];
    int add = 1 << (shift - 1);

    Vec4i coeff_const1(89, 75, 50, 18);
    Vec4i coeff_const2(75, -18, -89, -50);
    Vec4i coeff_const3(50, -89, 18, 75);
    Vec4i coeff_const4(18, -50, 75, -89);

    for (j = 0; j < line; j++)
    {
        int src_line = src[line];
        int src_line3 = src[3 * line];
        int src_line5 = src[5 * line];
        int src_line7 = src[7 * line];

        Vec4i tmp1 = coeff_const1 * src_line;
        Vec4i tmp2 = coeff_const2 * src_line3;
        Vec4i tmp3 = coeff_const3 * src_line5;
        Vec4i tmp4 = coeff_const4 * src_line7;

        Vec4i O_vec = tmp1 + tmp2 + tmp3 + tmp4;

        int EO_first = src[0] << 6;
        int EO_second = src[(line << 2)] << 6;

        int sub = (line << 1);
        sub = src[sub];

        EO[0] = 83 * sub + 36 * src[6 * line];
        EO[1] = 36 * sub  + (-83) * src[6 * line];

        EE[0] = EO_first + EO_second;
        EE[1] = EO_first - EO_second;

        E[0] = EE[0] + EO[0];
        E[3] = EE[0] - EO[0];
        E[1] = EE[1] + EO[1];
        E[2] = EE[1] - EO[1];

        Vec4i E_vec;
        E_vec.load(E);

        Vec4i E_O_sum;
        E_O_sum = E_vec + O_vec;
        E_O_sum = E_O_sum + add;
        E_O_sum = E_O_sum >> shift;

        Vec4i E_O_diff;
        E_O_diff = E_vec - O_vec;
        E_O_diff = E_O_diff + add;
        E_O_diff = E_O_diff >> shift;
        E_O_diff = permute4i<3, 2, 1, 0>(E_O_diff);

        Vec8s final_value = compress_saturated(E_O_sum, E_O_diff);
        final_value.store(dst);

        src++;
        dst += 8;
    }
}

#endif  // partialButterflyInverse8 vector code

#if 1 // partialButterflyInverse8 vector code

void CDECL partialButterflyInverse8(short *src, short *dst, int shift, int line)
{
    int j;
    int E[4];
    int EE[2], EO[2];
    int add = 1 << (shift - 1);

    __m128i c_add = _mm_set1_epi32(add);
    __m128i c32_89_75_50_18 = _mm_set_epi32(18, 50, 75, 89);
    __m128i c32_75_n18_n89_n50 = _mm_set_epi32(-50, -89, -18, 75);
    __m128i c32_50_n89_18_75 = _mm_set_epi32(75, 18, -89, 50);
    __m128i c32_18_n50_75_n89 = _mm_set_epi32(-89, 75, -50, 18);

    for (j = 0; j < line; j++)
    {
        int src_line = src[line];
        int src_3xline = src[3 * line];
        int src_5xline = src[5 * line];
        int src_7xline = src[7 * line];

        __m128i c_src_line = _mm_set1_epi32(src_line);
        __m128i c_src_3xline = _mm_set1_epi32(src_3xline);
        __m128i c_src_5xline = _mm_set1_epi32(src_5xline);
        __m128i c_src_7xline = _mm_set1_epi32(src_7xline);

        __m128i c32_89_75_50_18_src_line = _mm_mullo_epi32(c32_89_75_50_18, c_src_line);
        __m128i c32_75_n18_n89_n50_src_3xline = _mm_mullo_epi32(c32_75_n18_n89_n50, c_src_3xline);
        __m128i c32_50_n89_18_75_src_5xline = _mm_mullo_epi32(c32_50_n89_18_75, c_src_5xline);
        __m128i c32_18_n50_75_n89_src_7xline = _mm_mullo_epi32(c32_18_n50_75_n89, c_src_7xline);

        __m128i O = _mm_add_epi32(c32_89_75_50_18_src_line, c32_75_n18_n89_n50_src_3xline);
        O = _mm_add_epi32(O, c32_50_n89_18_75_src_5xline);
        O = _mm_add_epi32(O, c32_18_n50_75_n89_src_7xline);
        __m128i O_rev = _mm_shuffle_epi32(O, 0x1b);

        int EO_first = src[0] << 6;
        int EO_second = src[(line << 2)] << 6;

        int sub = (line << 1);
        sub = src[sub];

        EO[0] = 83 * sub + 36 * src[6 * line];
        EO[1] = 36 * sub  + (-83) * src[6 * line];

        EE[0] = EO_first + EO_second;
        EE[1] = EO_first - EO_second;

        E[0] = EE[0] + EO[0];
        E[3] = EE[0] - EO[0];
        E[1] = EE[1] + EO[1];
        E[2] = EE[1] - EO[1];

        __m128i E0123 = _mm_set_epi32(E[3], E[2], E[1], E[0]);
        __m128i E0123_rev = _mm_shuffle_epi32(E0123, 0x1b);

        __m128i EO_sum = _mm_add_epi32(O, E0123);
        __m128i EO_rev_sub = _mm_sub_epi32(E0123_rev, O_rev);

        __m128i dst_tmp1 = _mm_srai_epi32(_mm_add_epi32(c_add, EO_sum), shift);
        __m128i dst_tmp2 = _mm_srai_epi32(_mm_add_epi32(c_add, EO_rev_sub), shift);

        __m128i dst_tmp_final = _mm_packs_epi32(dst_tmp1, dst_tmp2);
        _mm_store_si128((__m128i*)(dst), dst_tmp_final);

        src++;
        dst += 8;
    }
}

#endif  // partialButterflyInverse8 intrinsic code

#if 0 // partialButterflyInverse16 vector code

void CDECL partialButterflyInverse16(short *src, short *dst, int shift, int line)
{
    int j;
    int add = 1 << (shift - 1);

    Vec4i coeff_const1(89, 75, 50, 18);
    Vec4i coeff_const2(75, -18, -89, -50);
    Vec4i coeff_const3(50, -89, 18, 75);
    Vec4i coeff_const4(18, -50, 75, -89);

    Vec4i coeff_const5(90, 87, 80, 70);
    Vec4i coeff_const6(87, 57, 9, -43);
    Vec4i coeff_const7(80, 9, -70, -87);
    Vec4i coeff_const8(70, -43, -87, 9);
    Vec4i coeff_const9(57, -80, -25, 90);
    Vec4i coeff_const10(43, -90, 57, 25);
    Vec4i coeff_const11(25, -70, 90, -80);
    Vec4i coeff_const12(9, -25, 43, -57);

    Vec4i coeff_const13(57, 43, 25, 9);
    Vec4i coeff_const14(-80, -90, -70, -25);
    Vec4i coeff_const15(-25, 57, 90, 43);
    Vec4i coeff_const16(90, 25, -80, -57);
    Vec4i coeff_const17(-9, -87, 43, 70);
    Vec4i coeff_const18(-87, 70, 9, -80);
    Vec4i coeff_const19(43, 9, -57, 87);
    Vec4i coeff_const20(70, -80, 87, -90);

    for (j = 0; j < line; j++)
    {
        int src_line = src[line];
        int src_line3 = src[3 * line];
        int src_line5 = src[5 * line];
        int src_line7 = src[7 * line];
        int src_line9 = src[9 * line];
        int src_line11 = src[11 * line];
        int src_line13 = src[13 * line];
        int src_line15 = src[15 * line];

        int src_line2 = src[2 * line];
        int src_line6 = src[6 * line];
        int src_line10 = src[10 * line];
        int src_line14 = src[14 * line];

        Vec4i O_tmp1 = coeff_const5 * src_line;
        Vec4i O_tmp2 = coeff_const6 * src_line3;
        Vec4i O_tmp3 = coeff_const7 * src_line5;
        Vec4i O_tmp4 = coeff_const8 * src_line7;
        Vec4i O_tmp5 = coeff_const9 * src_line9;
        Vec4i O_tmp6 = coeff_const10 * src_line11;
        Vec4i O_tmp7 = coeff_const11 * src_line13;
        Vec4i O_tmp8 = coeff_const12 * src_line15;

        Vec4i O_tmp9 = coeff_const13 * src_line;
        Vec4i O_tmp10 = coeff_const14 * src_line3;
        Vec4i O_tmp11 = coeff_const15 * src_line5;
        Vec4i O_tmp12 = coeff_const16 * src_line7;
        Vec4i O_tmp13 = coeff_const17 * src_line9;
        Vec4i O_tmp14 = coeff_const18 * src_line11;
        Vec4i O_tmp15 = coeff_const19 * src_line13;
        Vec4i O_tmp16 = coeff_const20 * src_line15;

        Vec4i O_first_half = O_tmp1 + O_tmp2 + O_tmp3 + O_tmp4 + O_tmp5 + O_tmp6 + O_tmp7 + O_tmp8;
        Vec4i O_second_half = O_tmp9 + O_tmp10 + O_tmp11 + O_tmp12 + O_tmp13 + O_tmp14 + O_tmp15 + O_tmp16;

        Vec4i tmp1 = coeff_const1 * src_line2;
        Vec4i tmp2 = coeff_const2 * src_line6;
        Vec4i tmp3 = coeff_const3 * src_line10;
        Vec4i tmp4 = coeff_const4 * src_line14;

        Vec4i EO = tmp1 + tmp2 + tmp3 + tmp4;

        int src_zero = (src[0] << 6);
        int src_eight = (src[line << 3] << 6);

        int EEO_zero = 83 * src[4 * line] + 36 * src[12 * line];
        int EEE_zero = src_zero + src_eight;
        int EEO_one  = 36 * src[4 * line] + -83 * src[12 * line];
        int EEE_one  = src_zero - src_eight;

        int EE_zero = EEE_zero + EEO_zero;
        int EE_one = EEE_one + EEO_one;
        int EE_two = EEE_one - EEO_one;
        int EE_three = EEE_zero - EEO_zero;

        Vec4i EE(EE_zero, EE_one, EE_two, EE_three);
        Vec4i E_first_half = EE + EO;
        Vec4i E_second_half = EE - EO;
        E_second_half = permute4i<3, 2, 1, 0>(E_second_half);

        Vec4i first_four_min_value;
        Vec4i second_four_min_value;
        Vec4i dst_third_first_four = (E_first_half + O_first_half + add);
        dst_third_first_four = dst_third_first_four >> shift;
        Vec4i dst_third_second_four = (E_second_half + O_second_half + add);
        dst_third_second_four = dst_third_second_four >> shift;

        Vec8s first_eight_final_value = compress_saturated(dst_third_first_four, dst_third_second_four);
        first_eight_final_value.store(dst);

        Vec4i dst_third_third_four = E_second_half - O_second_half;
        dst_third_third_four = (dst_third_third_four + add);
        dst_third_third_four = dst_third_third_four  >> shift;
        dst_third_third_four = permute4i<3, 2, 1, 0>(dst_third_third_four);

        Vec4i dst_third_four_four = E_first_half - O_first_half;
        dst_third_four_four = (dst_third_four_four + add);
        dst_third_four_four = dst_third_four_four >> shift;
        dst_third_four_four = permute4i<3, 2, 1, 0>(dst_third_four_four);

        Vec8s second_eight_final_value = compress_saturated(dst_third_third_four, dst_third_four_four);
        second_eight_final_value.store(dst + 8);

        src++;
        dst += 16;
    }
}

#endif  // partialButterflyInverse16 vector code

#if 1 // partialButterflyInverse16 intrinsic code

void CDECL partialButterflyInverse16(short *src, short *dst, int shift, int line)
{
    int j;

    int add = 1 << (shift - 1);
    __m128i c_add = _mm_set1_epi32(add);

    __m128i c32_90_87_80_70 = _mm_set_epi32(70, 80, 87, 90);
    __m128i c32_87_57_9_n43 = _mm_set_epi32(-43, 9, 57, 87);
    __m128i c32_80_9_n70_n87 = _mm_set_epi32(-87, -70, 9, 80);
    __m128i c32_70_n43_n87_9 = _mm_set_epi32(9, -87, -43, 70);
    __m128i c32_57_n80_n25_90 = _mm_set_epi32(90, -25, -80, 57);
    __m128i c32_43_n90_57_25 = _mm_set_epi32(25, 57, -90, 43);
    __m128i c32_25_n70_90_n80 = _mm_set_epi32(-80, 90, -70, 25);
    __m128i c32_9_n25_43_n57 = _mm_set_epi32(-57, 43, -25, 9);

    __m128i c32_57_43_25_9 = _mm_set_epi32(9, 25, 43, 57);
    __m128i c32_n80_n90_n70_n25 = _mm_set_epi32(-25, -70, -90, -80);
    __m128i c32_n25_57_90_43 = _mm_set_epi32(43, 90, 57, -25);
    __m128i c32_90_25_n80_n57 = _mm_set_epi32(-57, -80, 25, 90);
    __m128i c32_n9_n87_43_70 = _mm_set_epi32(70, 43, -87, -9);
    __m128i c32_n87_70_9_n80 = _mm_set_epi32(-80, 9, 70, -87);
    __m128i c32_43_9_n57_87 = _mm_set_epi32(87, -57, 9, 43);
    __m128i c32_n90_87_n80_70 = _mm_set_epi32(-90, 87, -80, 70);

    __m128i c32_89_75_50_18 = _mm_set_epi32(18, 50, 75, 89);
    __m128i c32_75_n18_n89_n50 = _mm_set_epi32(-50, -89, -18, 75);
    __m128i c32_50_n89_18_75 = _mm_set_epi32(75, 18, -89, 50);
    __m128i c32_18_n50_75_n89 = _mm_set_epi32(-89, 75, -50, 18);

    for (j = 0; j < line; j++)
    {
        int src_line = src[line];
        int src_3xline = src[3 * line];
        int src_5xline = src[5 * line];
        int src_7xline = src[7 * line];
        int src_9xline = src[9 * line];
        int src_11xline = src[11 * line];
        int src_13xline = src[13 * line];
        int src_15xline = src[15 * line];

        int src_2xline = src[2 * line];
        int src_6xline = src[6 * line];
        int src_10xline = src[10 * line];
        int src_14xline = src[14 * line];

        __m128i c_src_line = _mm_set1_epi32(src_line);
        __m128i c_src_3xline = _mm_set1_epi32(src_3xline);
        __m128i c_src_5xline = _mm_set1_epi32(src_5xline);
        __m128i c_src_7xline = _mm_set1_epi32(src_7xline);
        __m128i c_src_9xline = _mm_set1_epi32(src_9xline);
        __m128i c_src_11xline = _mm_set1_epi32(src_11xline);
        __m128i c_src_13xline = _mm_set1_epi32(src_13xline);
        __m128i c_src_15xline = _mm_set1_epi32(src_15xline);

        __m128i c_src_2xline = _mm_set1_epi32(src_2xline);
        __m128i c_src_6xline = _mm_set1_epi32(src_6xline);
        __m128i c_src_10xline = _mm_set1_epi32(src_10xline);
        __m128i c_src_14xline = _mm_set1_epi32(src_14xline);

        __m128i c32_90_87_80_70_src_line1 = _mm_mullo_epi32(c32_90_87_80_70, c_src_line);
        __m128i c32_57_43_25_9_src_line2 = _mm_mullo_epi32(c32_57_43_25_9, c_src_line);

        __m128i c32_87_57_9_n43_src_3xline1 = _mm_mullo_epi32(c32_87_57_9_n43, c_src_3xline);
        __m128i c32_n80_n90_n70_n25_src_3xline2 = _mm_mullo_epi32(c32_n80_n90_n70_n25, c_src_3xline);

        __m128i c32_80_9_n70_n87_src_5xline1 = _mm_mullo_epi32(c32_80_9_n70_n87, c_src_5xline);
        __m128i c32_n25_57_90_43_src_5xline2 = _mm_mullo_epi32(c32_n25_57_90_43, c_src_5xline);

        __m128i c32_70_n43_n87_9_src_7xline1 = _mm_mullo_epi32(c32_70_n43_n87_9, c_src_7xline);
        __m128i c32_90_25_n80_n57_src_7xline2 = _mm_mullo_epi32(c32_90_25_n80_n57, c_src_7xline);

        __m128i c32_57_n80_n25_90_src_9xline1 = _mm_mullo_epi32(c32_57_n80_n25_90, c_src_9xline);
        __m128i c32_n9_n87_43_70_src_9xline2 = _mm_mullo_epi32(c32_n9_n87_43_70, c_src_9xline);

        __m128i c32_43_n90_57_25_src_11xline1 = _mm_mullo_epi32(c32_43_n90_57_25, c_src_11xline);
        __m128i c32_n87_70_9_n80_src_11xline2 = _mm_mullo_epi32(c32_n87_70_9_n80, c_src_11xline);

        __m128i c32_25_n70_90_n80_src_13xline1 = _mm_mullo_epi32(c32_25_n70_90_n80, c_src_13xline);
        __m128i c32_43_9_n57_87_src_13xline2 = _mm_mullo_epi32(c32_43_9_n57_87, c_src_13xline);

        __m128i c32_9_n25_43_n57_src_15xline1 = _mm_mullo_epi32(c32_9_n25_43_n57, c_src_15xline);
        __m128i c32_n90_87_n80_70_src_15xline2 = _mm_mullo_epi32(c32_n90_87_n80_70, c_src_15xline);

        __m128i O_first_half = _mm_add_epi32(c32_90_87_80_70_src_line1, c32_87_57_9_n43_src_3xline1);
        O_first_half = _mm_add_epi32(O_first_half, c32_80_9_n70_n87_src_5xline1);
        O_first_half = _mm_add_epi32(O_first_half, c32_70_n43_n87_9_src_7xline1);
        O_first_half = _mm_add_epi32(O_first_half, c32_57_n80_n25_90_src_9xline1);
        O_first_half = _mm_add_epi32(O_first_half, c32_43_n90_57_25_src_11xline1);
        O_first_half = _mm_add_epi32(O_first_half, c32_25_n70_90_n80_src_13xline1);
        O_first_half = _mm_add_epi32(O_first_half, c32_9_n25_43_n57_src_15xline1);
        __m128i O_first_half_rev = _mm_shuffle_epi32(O_first_half, 0x1b);

        __m128i O_second_half = _mm_add_epi32(c32_57_43_25_9_src_line2, c32_n80_n90_n70_n25_src_3xline2);
        O_second_half = _mm_add_epi32(O_second_half, c32_n25_57_90_43_src_5xline2);
        O_second_half = _mm_add_epi32(O_second_half, c32_90_25_n80_n57_src_7xline2);
        O_second_half = _mm_add_epi32(O_second_half, c32_n9_n87_43_70_src_9xline2);
        O_second_half = _mm_add_epi32(O_second_half, c32_n87_70_9_n80_src_11xline2);
        O_second_half = _mm_add_epi32(O_second_half, c32_43_9_n57_87_src_13xline2);
        O_second_half = _mm_add_epi32(O_second_half, c32_n90_87_n80_70_src_15xline2);
        __m128i O_second_half_rev = _mm_shuffle_epi32(O_second_half, 0x1b);

        __m128i c32_89_75_50_18_src_2xline = _mm_mullo_epi32(c32_89_75_50_18, c_src_2xline);
        __m128i c32_75_n18_n89_n50_src_6xline = _mm_mullo_epi32(c32_75_n18_n89_n50, c_src_6xline);
        __m128i c32_50_n89_18_75_src_10xline = _mm_mullo_epi32(c32_50_n89_18_75, c_src_10xline);
        __m128i c32_18_n50_75_n89_src_14xline = _mm_mullo_epi32(c32_18_n50_75_n89, c_src_14xline);

        __m128i EO = _mm_add_epi32(c32_89_75_50_18_src_2xline, c32_75_n18_n89_n50_src_6xline);
        EO = _mm_add_epi32(EO, c32_50_n89_18_75_src_10xline);
        EO = _mm_add_epi32(EO, c32_18_n50_75_n89_src_14xline);
        __m128i EO_rev = _mm_shuffle_epi32(EO, 0x1b);

        int src_zero = (src[0] << 6);
        int src_eight = (src[line << 3] << 6);

        int EEO_zero = 83 * src[4 * line] + 36 * src[12 * line];
        int EEE_zero = src_zero + src_eight;
        int EEO_one  = 36 * src[4 * line] + -83 * src[12 * line];
        int EEE_one  = src_zero - src_eight;

        int EE_zero = EEE_zero + EEO_zero;
        int EE_one = EEE_one + EEO_one;
        int EE_two = EEE_one - EEO_one;
        int EE_three = EEE_zero - EEO_zero;

        __m128i EE = _mm_set_epi32(EE_three, EE_two, EE_one, EE_zero);
        __m128i EE_rev = _mm_shuffle_epi32(EE, 0x1b);

        __m128i E_first_half = _mm_add_epi32(EE, EO);
        __m128i E_second_half = _mm_sub_epi32(EE_rev, EO_rev);

        __m128i EO_sum1 = _mm_add_epi32(E_first_half, O_first_half);
        __m128i EO_sum2 = _mm_add_epi32(E_second_half, O_second_half);

        __m128i dst_tmp1 = _mm_srai_epi32(_mm_add_epi32(c_add, EO_sum1), shift);
        __m128i dst_tmp2 = _mm_srai_epi32(_mm_add_epi32(c_add, EO_sum2), shift);

        __m128i dst_tmp_final1 = _mm_packs_epi32(dst_tmp1, dst_tmp2);
        _mm_store_si128((__m128i*)(dst), dst_tmp_final1);

        __m128i E_first_half_rev = _mm_shuffle_epi32(E_first_half, 0x1b);
        __m128i E_second_half_rev = _mm_shuffle_epi32(E_second_half, 0x1b);

        __m128i EO_sub1 = _mm_sub_epi32(E_second_half_rev, O_second_half_rev);
        __m128i EO_sub2 = _mm_sub_epi32(E_first_half_rev, O_first_half_rev);

        __m128i dst_tmp3 = _mm_srai_epi32(_mm_add_epi32(c_add, EO_sub1), shift);
        __m128i dst_tmp4 = _mm_srai_epi32(_mm_add_epi32(c_add, EO_sub2), shift);

        __m128i dst_tmp_final2 = _mm_packs_epi32(dst_tmp3, dst_tmp4);
        _mm_store_si128((__m128i*)(dst + 8), dst_tmp_final2);

        src++;
        dst += 16;
    }
}

#endif  // partialButterflyInverse16 intrinsic code

void CDECL partialButterflyInverse32(short *src, short *dst, int shift, int line)
{
    int j;

    int add = 1 << (shift - 1);

    for (j = 0; j < line; j++)
    {
        int O_zero = 90 * src[line] + 90 * src[3 * line] + 88 * src[5 * line] + 85 * src[7 * line] +
            82 * src[9 * line] + 78 * src[11 * line] + 73 * src[13 * line] + 67 * src[15 * line] +
            61 * src[17 * line] + 54 * src[19 * line] + 46 * src[21 * line] + 38 * src[23 * line] +
            31 * src[25 * line] + 22 * src[27 * line] + 13 * src[29 * line] + 4 * src[31 * line];

        int O_one = 90 * src[line] + 82 * src[3 * line] + 67 * src[5 * line] + 46 * src[7 * line] +
            22 * src[9 * line] + (-4) * src[11 * line] + (-31) * src[13 * line] + (-54) * src[15 * line] +
            (-73) * src[17 * line] + (-85) * src[19 * line] + (-90) * src[21 * line] + (-88) * src[23 * line] +
            (-78) * src[25 * line] + (-61) * src[27 * line] + (-38) * src[29 * line] + (-13) * src[31 * line];

        int O_two = 88 * src[line] + 67 * src[3 * line] + 31 * src[5 * line] + (-13) * src[7 * line] +
            (-54) * src[9 * line] + (-82) * src[11 * line] + (-90) * src[13 * line] + (-78) * src[15 * line] +
            (-46) * src[17 * line] + (-4) * src[19 * line] + (38) * src[21 * line] + (73) * src[23 * line] +
            (90) * src[25 * line] + (85) * src[27 * line] + (61) * src[29 * line] + (22) * src[31 * line];

        int O_three = 85 * src[line] + 46 * src[3 * line] + (-13) * src[5 * line] + (-67) * src[7 * line] +
            (-90) * src[9 * line] + (-73) * src[11 * line] + (-22) * src[13 * line] + (38) * src[15 * line] +
            (82) * src[17 * line] + (88) * src[19 * line] + (54) * src[21 * line] + (-4) * src[23 * line] +
            (-61) * src[25 * line] + (-90) * src[27 * line] + (-78) * src[29 * line] + (-31) * src[31 * line];

        int O_four = 82 * src[line] + 22 * src[3 * line] + (-54) * src[5 * line] + (-90) * src[7 * line] +
            (-61) * src[9 * line] + (13) * src[11 * line] + (78) * src[13 * line] + (85) * src[15 * line] +
            (31) * src[17 * line] + (-46) * src[19 * line] + (-90) * src[21 * line] + (-67) * src[23 * line] +
            (4) * src[25 * line] + (73) * src[27 * line] + (88) * src[29 * line] + (38) * src[31 * line];

        int O_five = 78 * src[line] + (-4) * src[3 * line] + (-82) * src[5 * line] + (-73) * src[7 * line] +
            (13) * src[9 * line] + (85) * src[11 * line] + (67) * src[13 * line] + (-22) * src[15 * line] +
            (-88) * src[17 * line] + (-61) * src[19 * line] + (31) * src[21 * line] + (90) * src[23 * line] +
            (54) * src[25 * line] + (-38) * src[27 * line] + (-90) * src[29 * line] + (-46) * src[31 * line];

        int O_six = 73 * src[line] + (-31) * src[3 * line] + (-90) * src[5 * line] + (-22) * src[7 * line] +
            (78) * src[9 * line] + (67) * src[11 * line] + (-38) * src[13 * line] + (-90) * src[15 * line] +
            (-13) * src[17 * line] + (82) * src[19 * line] + (61) * src[21 * line] + (-46) * src[23 * line] +
            (-88) * src[25 * line] + (-4) * src[27 * line] + (85) * src[29 * line] + (54) * src[31 * line];

        int O_seven = 67 * src[line] + (-54) * src[3 * line] + (-78) * src[5 * line] + (38) * src[7 * line] +
            (85) * src[9 * line] + (-22) * src[11 * line] + (-90) * src[13 * line] + (4) * src[15 * line] +
            (90) * src[17 * line] + (13) * src[19 * line] + (-88) * src[21 * line] + (-31) * src[23 * line] +
            (82) * src[25 * line] + (46) * src[27 * line] + (-73) * src[29 * line] + (-61) * src[31 * line];

        int O_eight = 61 * src[line] + (-73) * src[3 * line] + (-46) * src[5 * line] + (82) * src[7 * line] +
            (31) * src[9 * line] + (-88) * src[11 * line] + (-13) * src[13 * line] + (90) * src[15 * line] +
            (-4) * src[17 * line] + (-90) * src[19 * line] + (22) * src[21 * line] + (85) * src[23 * line] +
            (-38) * src[25 * line] + (-78) * src[27 * line] + (54) * src[29 * line] + (67) * src[31 * line];

        int O_nine = 54 * src[line] + (-85) * src[3 * line] + (-4) * src[5 * line] + (88) * src[7 * line] +
            (-46) * src[9 * line] + (-61) * src[11 * line] + (82) * src[13 * line] + (13) * src[15 * line] +
            (-90) * src[17 * line] + (38) * src[19 * line] + (67) * src[21 * line] + (-78) * src[23 * line] +
            (-22) * src[25 * line] + (90) * src[27 * line] + (-31) * src[29 * line] + (-73) * src[31 * line];

        int O_ten = 46 * src[line] + (-90) * src[3 * line] + (38) * src[5 * line] + (54) * src[7 * line] +
            (-90) * src[9 * line] + (31) * src[11 * line] + (61) * src[13 * line] + (-88) * src[15 * line] +
            (22) * src[17 * line] + (67) * src[19 * line] + (-85) * src[21 * line] + (13) * src[23 * line] +
            (73) * src[25 * line] + (-82) * src[27 * line] + (4) * src[29 * line] + (78) * src[31 * line];

        int O_eleven = 38 * src[line] + (-88) * src[3 * line] + (73) * src[5 * line] + (-4) * src[7 * line] +
            (-67) * src[9 * line] + (90) * src[11 * line] + (-46) * src[13 * line] + (-31) * src[15 * line] +
            (85) * src[17 * line] + (-78) * src[19 * line] + (13) * src[21 * line] + (61) * src[23 * line] +
            (-90) * src[25 * line] + (54) * src[27 * line] + (22) * src[29 * line] + (-82) * src[31 * line];

        int O_twelve = 31 * src[line] + (-78) * src[3 * line] + (90) * src[5 * line] + (-61) * src[7 * line] +
            (4) * src[9 * line] + (54) * src[11 * line] + (-88) * src[13 * line] + (82) * src[15 * line] +
            (-38) * src[17 * line] + (-22) * src[19 * line] + (73) * src[21 * line] + (-90) * src[23 * line] +
            (67) * src[25 * line] + (-13) * src[27 * line] + (-46) * src[29 * line] + (85) * src[31 * line];

        int O_thirteen = 22 * src[line] + (-61) * src[3 * line] + (85) * src[5 * line] + (-90) * src[7 * line] +
            (73) * src[9 * line] + (-38) * src[11 * line] + (-4) * src[13 * line] + (46) * src[15 * line] +
            (-78) * src[17 * line] + (90) * src[19 * line] + (-82) * src[21 * line] + (54) * src[23 * line] +
            (-13) * src[25 * line] + (-31) * src[27 * line] + (67) * src[29 * line] + (-88) * src[31 * line];

        int O_fourteen = 13 * src[line] + (-38) * src[3 * line] + (61) * src[5 * line] + (-78) * src[7 * line] +
            (88) * src[9 * line] + (-90) * src[11 * line] + (85) * src[13 * line] + (-73) * src[15 * line] +
            (54) * src[17 * line] + (-31) * src[19 * line] + (4) * src[21 * line] + (22) * src[23 * line] +
            (-46) * src[25 * line] + (67) * src[27 * line] + (-82) * src[29 * line] + (90) * src[31 * line];

        int O_fifteen = 4 * src[line] + (-13) * src[3 * line] + (22) * src[5 * line] + (-31) * src[7 * line] +
            (38) * src[9 * line] + (-46) * src[11 * line] + (54) * src[13 * line] + (-61) * src[15 * line] +
            (67) * src[17 * line] + (-73) * src[19 * line] + (78) * src[21 * line] + (-82) * src[23 * line] +
            (85) * src[25 * line] + (-88) * src[27 * line] + (90) * src[29 * line] + (-90) * src[31 * line];

        Vec4i O_first_four(O_zero, O_one, O_two, O_three);
        Vec4i O_second_four(O_four, O_five, O_six, O_seven);
        Vec4i O_third_four(O_eight, O_nine, O_ten, O_eleven);
        Vec4i O_four_four(O_twelve, O_thirteen, O_fourteen, O_fifteen);

        int EO_zero = 90 * src[2 * line] + 87 * src[6 * line] + 80 * src[10 * line] + 70 * src[14 * line] +
            57 * src[18 * line] + 43 * src[22 * line] + 25 * src[26 * line] + 9 * src[30 * line];

        int EO_one = 87 * src[2 * line] + 57 * src[6 * line] + 9 * src[10 * line] + (-43) * src[14 * line] +
            (-80) * src[18 * line] + (-90) * src[22 * line] + (-70) * src[26 * line] + (-25) * src[30 * line];

        int EO_two = 80 * src[2 * line] + 9 * src[6 * line] + (-70) * src[10 * line] + (-87) * src[14 * line] +
            (-25) * src[18 * line] + (57) * src[22 * line] + (90) * src[26 * line] + (43) * src[30 * line];

        int EO_three = 70 * src[2 * line] + (-43) * src[6 * line] + (-87) * src[10 * line] + (9) * src[14 * line] +
            (90) * src[18 * line] + (25) * src[22 * line] + (-80) * src[26 * line] + (-57) * src[30 * line];

        int EO_four = 57 * src[2 * line]  + (-80) * src[6 * line] + (-25) * src[10 * line] + (90) * src[14 * line] +
            (-9) * src[18 * line] + (-87) * src[22 * line] + (43) * src[26 * line] + (70) * src[30 * line];

        int EO_five = 43 * src[2 * line]  + (-90) * src[6 * line] + (57) * src[10 * line] + (25) * src[14 * line] +
            (-87) * src[18 * line] + (70) * src[22 * line] + (9) * src[26 * line] + (-80) * src[30 * line];

        int EO_six = 25 * src[2 * line]  + (-70) * src[6 * line] + (90) * src[10 * line] + (-80) * src[14 * line] +
            (43) * src[18 * line] + (9) * src[22 * line] + (-57) * src[26 * line] + (87) * src[30 * line];

        int EO_seven = 9 * src[2 * line]  + (-25) * src[6 * line] + (43) * src[10 * line] + (-57) * src[14 * line] +
            (70) * src[18 * line] + (-80) * src[22 * line] + (87) * src[26 * line] + (-90) * src[30 * line];

        Vec4i EO_first_half(EO_zero, EO_one, EO_two, EO_three);
        Vec4i EO_second_half(EO_four, EO_five, EO_six, EO_seven);

        int EEO_zero = 89 * src[4 * line] + 75 * src[12 * line] + 50 * src[20 * line] + 18 * src[28 * line];
        int EEO_one = 75 * src[4 * line] + (-18) * src[12 * line] + (-89) * src[20 * line] + (-50) * src[28 * line];
        int EEO_two = 50 * src[4 * line] + (-89) * src[12 * line] + 18 * src[20 * line] + 75 * src[28 * line];
        int EEO_three = 18 * src[4 * line] + (-50) * src[12 * line] + 75 * src[20 * line] + (-89) * src[28 * line];

        Vec4i EEO(EEO_zero, EEO_one, EEO_two, EEO_three);

        int EEEO_zero = 83 * src[8 * line] + 36 * src[24 * line];
        int EEEO_one = 36 * src[8 * line] + (-83) * src[24 * line];
        int EEEE_zero = 64 * src[0] + 64 * src[16 * line];
        int  EEEE_one = 64 * src[0] + (-64) * src[16 * line];

        int EEE_zero = EEEE_zero + EEEO_zero;
        int EEE_three = EEEE_zero - EEEO_zero;
        int EEE_one = EEEE_one + EEEO_one;
        int EEE_two = EEEE_one - EEEO_one;

        Vec4i EEE(EEE_zero, EEE_one, EEE_two, EEE_three);
        Vec4i EE_first_half = EEE + EEO;
        Vec4i EE_second_half = EEE - EEO;
        EE_second_half = permute4i<3, 2, 1, 0>(EE_second_half);

        Vec4i E_first_four = EE_first_half + EO_first_half;
        Vec4i E_second_four = EE_second_half + EO_second_half;
        Vec4i E_third_four = EE_second_half - EO_second_half;
        E_third_four = permute4i<3, 2, 1, 0>(E_third_four);
        Vec4i E_four_four = EE_first_half - EO_first_half;
        E_four_four = permute4i<3, 2, 1, 0>(E_four_four);

        Vec4i dst_third_first_four =  (E_first_four + O_first_four + add) >> shift;
        Vec4i dst_third_second_four =  (E_second_four + O_second_four + add) >> shift;
        Vec4i dst_third_third_four =  (E_third_four + O_third_four + add) >> shift;
        Vec4i dst_third_four_four =  (E_four_four + O_four_four + add) >> shift;

        Vec8s half0 = compress_saturated(dst_third_first_four, dst_third_second_four);
        Vec8s half1 = compress_saturated(dst_third_third_four, dst_third_four_four);
        half0.store(dst);
        half1.store(dst + 8);

        Vec4i dst_third_five_four =  (E_four_four - O_four_four + add) >> shift;
        dst_third_five_four = permute4i<3, 2, 1, 0>(dst_third_five_four);
        Vec4i dst_third_six_four =  (E_third_four - O_third_four + add) >> shift;
        dst_third_six_four = permute4i<3, 2, 1, 0>(dst_third_six_four);
        Vec4i dst_third_seven_four =  (E_second_four - O_second_four + add) >> shift;
        dst_third_seven_four = permute4i<3, 2, 1, 0>(dst_third_seven_four);
        Vec4i dst_third_eight_four =  (E_first_four - O_first_four + add) >> shift;
        dst_third_eight_four = permute4i<3, 2, 1, 0>(dst_third_eight_four);

        Vec8s half2 = compress_saturated(dst_third_five_four, dst_third_six_four);
        Vec8s half3 = compress_saturated(dst_third_seven_four, dst_third_eight_four);
        half2.store(dst + 16);
        half3.store(dst + 24);

        src++;
        dst += 32;
    }
}

void CDECL xDeQuant(int bitDepth, const int* pSrc, int* pDes, int iWidth, int iHeight, int iPer, int iRem, bool useScalingList, unsigned int uiLog2TrSize, int *piDequantCoefOrig)
{
    const int* piQCoef = pSrc;
    int* piCoef = pDes;

    int g_invQuantScales[6] = { 40, 45, 51, 57, 64, 72 };

    if (iWidth > 32)
    {
        iWidth  = 32;
        iHeight = 32;
    }

    int iShift, iAdd;

    int iTransformShift = 15 - bitDepth - uiLog2TrSize;

    iShift = 6 - iTransformShift;

    if (useScalingList)
    {
        iShift += 4;
        int *piDequantCoef = piDequantCoefOrig;

        if (iShift > iPer)
        {
            iAdd = 1 << (iShift - iPer - 1);
            Vec4i IAdd(iAdd);

            for (int n = 0; n < iWidth * iHeight; n = n + 8)
            {
                Vec4i qCoef1, qCoef2, deQuantCoef1, deQuantCoef2;

                qCoef1.load(piQCoef + n);
                qCoef2.load(piQCoef + n + 4);

                deQuantCoef1.load(piDequantCoef + n);
                deQuantCoef2.load(piDequantCoef + n + 4);

                Vec8s qCoef12 = compress_saturated(qCoef1, qCoef2);

                qCoef1 = extend_low(qCoef12);
                qCoef2 = extend_high(qCoef12);

                qCoef1 =  (qCoef1 *  deQuantCoef1 + IAdd) >> (iShift - iPer);
                qCoef2 =  (qCoef2 *  deQuantCoef2 + IAdd) >> (iShift - iPer);

                qCoef12 = compress_saturated(qCoef1, qCoef2);

                qCoef1 = extend_low(qCoef12);
                qCoef1.store(piCoef + n);
                qCoef2 = extend_high(qCoef12);
                qCoef2.store(piCoef + n + 4);
            }
        }
        else
        {
            for (int n = 0; n < iWidth * iHeight; n = n + 8)
            {
                Vec4i qCoef1, qCoef2, deQuantCoef1, deQuantCoef2;

                qCoef1.load(piQCoef + n);
                qCoef2.load(piQCoef + n + 4);

                deQuantCoef1.load(piDequantCoef + n);
                deQuantCoef2.load(piDequantCoef + n + 4);

                Vec8s qCoef12 = compress_saturated(qCoef1, qCoef2);

                qCoef1 = extend_low(qCoef12);
                qCoef2 = extend_high(qCoef12);

                qCoef1 = qCoef1 * deQuantCoef1;
                qCoef2 = qCoef2 * deQuantCoef2;

                qCoef12 = compress_saturated(qCoef1, qCoef2);

                qCoef1 = extend_low(qCoef12);
                qCoef2 = extend_high(qCoef12);

                qCoef1 = qCoef1 << (iPer - iShift);
                qCoef2 = qCoef2 << (iPer - iShift);

                qCoef12 = compress_saturated(qCoef1, qCoef2);

                qCoef1 = extend_low(qCoef12);
                qCoef1.store(piCoef + n);
                qCoef2 = extend_high(qCoef12);
                qCoef2.store(piCoef + n + 4);
            }
        }
    }
    else
    {
        iAdd = 1 << (iShift - 1);
        int scale = g_invQuantScales[iRem] << iPer;

        Vec4i Scale(scale);
        Vec4i IAdd(iAdd);

        for (int n = 0; n < iWidth * iHeight; n = n + 8)
        {
            Vec4i qCoef1, qCoef2;
            qCoef1.load(piQCoef + n);
            qCoef2.load(piQCoef + n + 4);

            Vec8s qCoef12 = compress_saturated(qCoef1, qCoef2);

            qCoef1 = extend_low(qCoef12);
            qCoef2 = extend_high(qCoef12);

            qCoef1 = (qCoef1 * Scale + IAdd) >> iShift;
            qCoef2 = (qCoef2 * Scale + IAdd) >> iShift;

            qCoef12 = compress_saturated(qCoef1, qCoef2);

            qCoef1 = extend_low(qCoef12);
            qCoef1.store(piCoef + n);
            qCoef2 = extend_high(qCoef12);
            qCoef2.store(piCoef + n + 4);
        }
    }
}

#if INSTRSET < 5
void xIDST4(short *pSrc, short *pDst, intptr_t stride)
{
    const int shift_1st = 7;
    const int shift_2nd = 12;
    ALIGN_VAR_32(Short, tmp[4 * 4]);
    ALIGN_VAR_32(Short, tmp2[4 * 4]);

    inversedst(pSrc, tmp, shift_1st);
    inversedst(tmp, tmp2, shift_2nd);
    for(int i=0; i<4; i++)
    {
        memcpy(&pDst[i * stride], &tmp2[i * 4], 4 * sizeof(short));
    }
}
#endif // INSTRSET < 5

#if INSTRSET >= 5
ALIGN_VAR_32(static const short, tab_idst_4x4[8][8] )=
{
    {   29, +84, 29,  +84,  29, +84,  29, +84 },
    {  +74, +55, +74, +55, +74, +55, +74, +55 },
    {   55, -29,  55, -29,  55, -29,  55, -29 },
    {  +74, -84, +74, -84, +74, -84, +74, -84 },
    {   74, -74,  74, -74,  74, -74,  74, -74 },
    {    0, +74,   0, +74,   0, +74,   0, +74 },
    {   84, +55,  84, +55,  84, +55,  84, +55 },
    {  -74, -29, -74, -29, -74, -29, -74, -29 }
};

void xIDST4(short *pSrc, short *pDst, intptr_t stride)
{
    __m128i m128iAdd, S0, S8, m128iTmp1, m128iTmp2, m128iAC, m128iBD, m128iA, m128iD;
    m128iAdd  = _mm_set1_epi32( 64 );

    S0  = _mm_load_si128   ( (__m128i*)( pSrc      ) );
    S8  = _mm_load_si128   ( (__m128i*)( pSrc + 8  ) );

    m128iAC  = _mm_unpacklo_epi16( S0 , S8 );
    m128iBD  = _mm_unpackhi_epi16( S0 , S8 );

    m128iTmp1 = _mm_madd_epi16( m128iAC, _mm_load_si128( (__m128i*)( tab_idst_4x4[0] ) ) );
    m128iTmp2 = _mm_madd_epi16( m128iBD, _mm_load_si128( (__m128i*)( tab_idst_4x4[1] ) ) );
    S0   = _mm_add_epi32( m128iTmp1, m128iTmp2 );
    S0   = _mm_add_epi32( S0, m128iAdd );
    S0   = _mm_srai_epi32( S0, 7  );

    m128iTmp1 = _mm_madd_epi16( m128iAC, _mm_load_si128( (__m128i*)( tab_idst_4x4[2] ) ) );
    m128iTmp2 = _mm_madd_epi16( m128iBD, _mm_load_si128( (__m128i*)( tab_idst_4x4[3] ) ) );
    S8   = _mm_add_epi32( m128iTmp1, m128iTmp2 );
    S8   = _mm_add_epi32( S8, m128iAdd );
    S8   = _mm_srai_epi32( S8, 7  );

    m128iA = _mm_packs_epi32( S0, S8 );

    m128iTmp1 = _mm_madd_epi16( m128iAC, _mm_load_si128( (__m128i*)( tab_idst_4x4[4] ) ) );
    m128iTmp2 = _mm_madd_epi16( m128iBD, _mm_load_si128( (__m128i*)( tab_idst_4x4[5] ) ) );
    S0  = _mm_add_epi32( m128iTmp1, m128iTmp2 );
    S0  = _mm_add_epi32( S0, m128iAdd );
    S0  = _mm_srai_epi32( S0, 7  );

    m128iTmp1 = _mm_madd_epi16( m128iAC, _mm_load_si128( (__m128i*)( tab_idst_4x4[6] ) ) );
    m128iTmp2 = _mm_madd_epi16( m128iBD, _mm_load_si128( (__m128i*)( tab_idst_4x4[7] ) ) );
    S8  = _mm_add_epi32( m128iTmp1, m128iTmp2 );
    S8  = _mm_add_epi32( S8, m128iAdd );
    S8  = _mm_srai_epi32( S8, 7  );

    m128iD = _mm_packs_epi32( S0, S8 );

    S0 =_mm_unpacklo_epi16(  m128iA, m128iD );
    S8 =_mm_unpackhi_epi16(  m128iA, m128iD );

    m128iA =_mm_unpacklo_epi16(  S0, S8 );
    m128iD =_mm_unpackhi_epi16(  S0, S8 );

    /*   ###################    */
    m128iAdd  = _mm_set1_epi32( 2048 );

    m128iAC  = _mm_unpacklo_epi16( m128iA , m128iD );
    m128iBD  = _mm_unpackhi_epi16( m128iA , m128iD );

    m128iTmp1 = _mm_madd_epi16( m128iAC, _mm_load_si128( (__m128i*)( tab_idst_4x4[0] ) ) );
    m128iTmp2 = _mm_madd_epi16( m128iBD, _mm_load_si128( (__m128i*)( tab_idst_4x4[1] ) ) );
    S0   = _mm_add_epi32( m128iTmp1, m128iTmp2 );
    S0   = _mm_add_epi32( S0, m128iAdd );
    S0   = _mm_srai_epi32( S0, 12  );

    m128iTmp1 = _mm_madd_epi16( m128iAC, _mm_load_si128( (__m128i*)( tab_idst_4x4[2] ) ) );
    m128iTmp2 = _mm_madd_epi16( m128iBD, _mm_load_si128( (__m128i*)( tab_idst_4x4[3] ) ) );
    S8   = _mm_add_epi32( m128iTmp1, m128iTmp2 );
    S8   = _mm_add_epi32( S8, m128iAdd );
    S8   = _mm_srai_epi32( S8, 12  );

    m128iA = _mm_packs_epi32( S0, S8 );

    m128iTmp1 = _mm_madd_epi16( m128iAC, _mm_load_si128( (__m128i*)( tab_idst_4x4[4] ) ) );
    m128iTmp2 = _mm_madd_epi16( m128iBD, _mm_load_si128( (__m128i*)( tab_idst_4x4[5] ) ) );
    S0  = _mm_add_epi32( m128iTmp1, m128iTmp2 );
    S0  = _mm_add_epi32( S0, m128iAdd );
    S0  = _mm_srai_epi32( S0, 12  );

    m128iTmp1 = _mm_madd_epi16( m128iAC, _mm_load_si128( (__m128i*)( tab_idst_4x4[6] ) ) );
    m128iTmp2 = _mm_madd_epi16( m128iBD, _mm_load_si128( (__m128i*)( tab_idst_4x4[7] ) ) );
    S8  = _mm_add_epi32( m128iTmp1, m128iTmp2 );
    S8  = _mm_add_epi32( S8, m128iAdd );
    S8  = _mm_srai_epi32( S8, 12  );

    m128iD = _mm_packs_epi32( S0, S8 );

    m128iTmp1 = _mm_unpacklo_epi16(m128iA, m128iD);   // [32 30 22 20 12 10 02 00]
    m128iTmp2 = _mm_unpackhi_epi16(m128iA, m128iD);   // [33 31 23 21 13 11 03 01]
    m128iAC   = _mm_unpacklo_epi16(m128iTmp1, m128iTmp2);
    m128iBD   = _mm_unpackhi_epi16(m128iTmp1, m128iTmp2);

    _mm_storel_epi64( (__m128i*)&pDst[0 * stride], m128iAC );
    _mm_storeh_pi   ( (__m64*  )&pDst[1 * stride], _mm_castsi128_ps(m128iAC));
    _mm_storel_epi64( (__m128i*)&pDst[2 * stride], m128iBD );
    _mm_storeh_pi   ( (__m64*  )&pDst[3 * stride], _mm_castsi128_ps(m128iBD));
}
#endif // INSTRSET >= 5

ALIGN_VAR_32(static const short, tab_dct_4x4[][8]) =
{
    { 64, 64, 64, 64, 64, 64, 64, 64},
    { 83, 36, 83, 36, 83, 36, 83, 36},
    { 64,-64, 64,-64, 64,-64, 64,-64},
    { 36,-83, 36,-83, 36,-83, 36,-83},
};
void xDCT4(short *pSrc, short *pDst, intptr_t)
{
    // Const
    __m128i c_1         = _mm_set1_epi32(1);
    __m128i c_128       = _mm_set1_epi32(128);

    __m128i T20, T21;
    __m128i T30, T31, T32, T33;
    __m128i T40, T41, T50, T51, T60, T61, T62, T63, T70, T71, T72, T73;
    __m128i T50_, T51_;

    T20  = _mm_loadu_si128((__m128i *)&pSrc[0 * 4]); // [13 12 11 10 03 02 01 00]
    T21  = _mm_loadu_si128((__m128i *)&pSrc[2 * 4]); // [33 32 31 30 23 22 21 20]

    // DCT1
    T30  = _mm_shuffle_epi32(T20, 0xD8);        // [13 12 03 02 11 10 01 00]
    T31  = _mm_shuffle_epi32(T21, 0xD8);        // [33 32 23 22 31 30 21 20]
    T32  = _mm_shufflehi_epi16(T30, 0xB1);      // [12 13 02 03 11 10 01 00]
    T33  = _mm_shufflehi_epi16(T31, 0xB1);      // [32 33 22 23 31 30 21 20]

    T40  = _mm_unpacklo_epi64(T32, T33);        // [31 30 21 20 11 10 01 00]
    T41  = _mm_unpackhi_epi64(T32, T33);        // [32 33 22 23 12 13 02 03]
    T50  = _mm_add_epi16(T40, T41);             // [1+2 0+3]
    T51  = _mm_sub_epi16(T40, T41);             // [1-2 0-3]
    T60  = _mm_madd_epi16(T50, _mm_load_si128((__m128i*)tab_dct_4x4[0])); // [ 64*s12 + 64*s03] = [03 02 01 00]
    T61  = _mm_madd_epi16(T51, _mm_load_si128((__m128i*)tab_dct_4x4[1])); // [ 36*d12 + 83*d03] = [13 12 11 10]
    T62  = _mm_madd_epi16(T50, _mm_load_si128((__m128i*)tab_dct_4x4[2])); // [-64*s12 + 64*s03] = [23 22 21 20]
    T63  = _mm_madd_epi16(T51, _mm_load_si128((__m128i*)tab_dct_4x4[3])); // [-83*d12 + 36*d03] = [33 32 31 30]
    T70  = _mm_srai_epi32(_mm_add_epi32(T60, c_1), 1);  // [30 20 10 00]
    T71  = _mm_srai_epi32(_mm_add_epi32(T61, c_1), 1);  // [31 21 11 01]
    T72  = _mm_srai_epi32(_mm_add_epi32(T62, c_1), 1);  // [32 22 12 02]
    T73  = _mm_srai_epi32(_mm_add_epi32(T63, c_1), 1);  // [33 23 13 03]

    // Transpose
    T20  = _mm_packs_epi32(T70, T71);       // [13 12 11 10 03 02 01 00]
    T21  = _mm_packs_epi32(T72, T73);       // [33 32 31 30 23 22 21 20]

    T30  = _mm_shuffle_epi32(T20, 0xD8);        // [13 12 03 02 11 10 01 00]
    T31  = _mm_shuffle_epi32(T21, 0xD8);        // [33 32 23 22 31 30 21 20]
    T32  = _mm_shufflehi_epi16(T30, 0xB1);      // [12 13 02 03 11 10 01 00]
    T33  = _mm_shufflehi_epi16(T31, 0xB1);      // [32 33 22 23 31 30 21 20]

    T40  = _mm_unpacklo_epi64(T32, T33);        // [31 30 21 20 11 10 01 00]
    T41  = _mm_unpackhi_epi64(T32, T33);        // [32 33 22 23 12 13 02 03]

    T50_ = _mm_madd_epi16(T40, _mm_load_si128((__m128i*)tab_dct_4x4[0]));
    T51_ = _mm_madd_epi16(T41, _mm_load_si128((__m128i*)tab_dct_4x4[0]));
    T60  = _mm_add_epi32(T50_, T51_);
    T50_ = _mm_madd_epi16(T40, _mm_load_si128((__m128i*)tab_dct_4x4[1]));
    T51_ = _mm_madd_epi16(T41, _mm_load_si128((__m128i*)tab_dct_4x4[1]));
    T61  = _mm_sub_epi32(T50_, T51_);
    T50_ = _mm_madd_epi16(T40, _mm_load_si128((__m128i*)tab_dct_4x4[2]));
    T51_ = _mm_madd_epi16(T41, _mm_load_si128((__m128i*)tab_dct_4x4[2]));
    T62  = _mm_add_epi32(T50_, T51_);
    T50_ = _mm_madd_epi16(T40, _mm_load_si128((__m128i*)tab_dct_4x4[3]));
    T51_ = _mm_madd_epi16(T41, _mm_load_si128((__m128i*)tab_dct_4x4[3]));
    T63  = _mm_sub_epi32(T50_, T51_);
    
    T70  = _mm_srai_epi32(_mm_add_epi32(T60, c_128), 8);  // [30 20 10 00]
    T71  = _mm_srai_epi32(_mm_add_epi32(T61, c_128), 8);  // [31 21 11 01]
    T72  = _mm_srai_epi32(_mm_add_epi32(T62, c_128), 8);  // [32 22 12 02]
    T73  = _mm_srai_epi32(_mm_add_epi32(T63, c_128), 8);  // [33 23 13 03]

    T20  = _mm_packs_epi32(T70, T71);       // [13 12 11 10 03 02 01 00]
    T21  = _mm_packs_epi32(T72, T73);       // [33 32 31 30 23 22 21 20]

    _mm_storeu_si128((__m128i*)&pDst[0 * 4], T20);
    _mm_storeu_si128((__m128i*)&pDst[2 * 4], T21);
}

ALIGN_VAR_32(static const short, tab_idct_4x4[4][8] )=
{
    { 64,  64, 64,  64, 64,  64, 64,  64 },
    { 64, -64, 64, -64, 64, -64, 64, -64 },
    { 83,  36, 83,  36, 83,  36, 83,  36 },
    { 36, -83, 36, -83, 36, -83, 36, -83 },
};
void xIDCT4(short *pSrc, short *pDst, intptr_t stride)
{
    __m128i S0, S8, m128iAdd, m128Tmp1, m128Tmp2, E1, E2, O1, O2, m128iA, m128iD;
    S0   = _mm_load_si128( (__m128i*)( pSrc     ) );
    S8   = _mm_load_si128( (__m128i*)( pSrc + 8 ) );
    m128iAdd  = _mm_set1_epi32( 64 );

    m128Tmp1 = _mm_unpacklo_epi16(  S0, S8 );
    E1 = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_4x4[0] ) ) );
    E1 = _mm_add_epi32( E1, m128iAdd );

    E2 = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_4x4[1] ) ) );
    E2 = _mm_add_epi32( E2, m128iAdd );


    m128Tmp1 = _mm_unpackhi_epi16(  S0, S8 );
    O1 = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_4x4[2] ) ) );
    O2 = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_4x4[3] ) ) );

    m128iA  = _mm_add_epi32( E1, O1 );
    m128iA  = _mm_srai_epi32( m128iA, 7  );        // Sum = Sum >> iShiftNum
    m128Tmp1 = _mm_add_epi32( E2, O2 );
    m128Tmp1 = _mm_srai_epi32( m128Tmp1, 7  );       // Sum = Sum >> iShiftNum
    m128iA = _mm_packs_epi32( m128iA, m128Tmp1);

    m128iD = _mm_sub_epi32( E2, O2 );
    m128iD = _mm_srai_epi32( m128iD, 7  );         // Sum = Sum >> iShiftNum

    m128Tmp1 = _mm_sub_epi32( E1, O1 );
    m128Tmp1 = _mm_srai_epi32( m128Tmp1, 7  );       // Sum = Sum >> iShiftNum

    m128iD = _mm_packs_epi32( m128iD, m128Tmp1 );

    S0 =_mm_unpacklo_epi16(  m128iA, m128iD );
    S8 =_mm_unpackhi_epi16(  m128iA, m128iD );

    m128iA =_mm_unpacklo_epi16(  S0, S8 );
    m128iD =_mm_unpackhi_epi16(  S0, S8 );

    /*  ##########################  */


    m128iAdd  = _mm_set1_epi32( 2048 );
    m128Tmp1 = _mm_unpacklo_epi16(  m128iA, m128iD );
    E1 = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_4x4[0] ) ) );
    E1 = _mm_add_epi32( E1, m128iAdd );

    E2 = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_4x4[1] ) ) );
    E2 = _mm_add_epi32( E2, m128iAdd );


    m128Tmp1 = _mm_unpackhi_epi16(  m128iA, m128iD );
    O1 = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_4x4[2] ) ) );
    O2 = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_4x4[3] ) ) );

    m128iA   = _mm_add_epi32( E1, O1 );
    m128iA   = _mm_srai_epi32( m128iA, 12  );
    m128Tmp1 = _mm_add_epi32( E2, O2 );
    m128Tmp1 = _mm_srai_epi32( m128Tmp1, 12  );
    m128iA   = _mm_packs_epi32( m128iA, m128Tmp1);

    m128iD = _mm_sub_epi32( E2, O2 );
    m128iD = _mm_srai_epi32( m128iD, 12  );

    m128Tmp1 = _mm_sub_epi32( E1, O1 );
    m128Tmp1 = _mm_srai_epi32( m128Tmp1, 12  );

    m128iD = _mm_packs_epi32( m128iD, m128Tmp1 );

    m128Tmp1 = _mm_unpacklo_epi16(m128iA, m128iD);   // [32 30 22 20 12 10 02 00]
    m128Tmp2 = _mm_unpackhi_epi16(m128iA, m128iD);   // [33 31 23 21 13 11 03 01]
    m128iA   = _mm_unpacklo_epi16(m128Tmp1, m128Tmp2);
    m128iD   = _mm_unpackhi_epi16(m128Tmp1, m128Tmp2);

    _mm_storel_epi64( (__m128i*)&pDst[0 * stride], m128iA );
    _mm_storeh_pi   ( (__m64*  )&pDst[1 * stride], _mm_castsi128_ps(m128iA));
    _mm_storel_epi64( (__m128i*)&pDst[2 * stride], m128iD );
    _mm_storeh_pi   ( (__m64*  )&pDst[3 * stride], _mm_castsi128_ps(m128iD));
}

#if INSTRSET < 4
void xDCT8(short *pSrc, short *pDst, intptr_t)
{
    const int shift_1st = 2;
    const int shift_2nd = 9;
    ALIGN_VAR_32(Short, tmp[8 * 8]);

    partialButterfly8(pSrc, tmp, shift_1st, 8);
    partialButterfly8(tmp, pDst, shift_2nd, 8);
}
#endif // INSTRSET < 4

#if INSTRSET >= 4
ALIGN_VAR_32(static const short, tab_dct_8x8[][8])=
{
    { 0x0100, 0x0F0E, 0x0706, 0x0908, 0x0302, 0x0D0C, 0x0504, 0x0B0A},

    { 64, 64, 64, 64, 64, 64, 64, 64},
    { 64,-64, 64,-64, 64,-64, 64,-64},
    { 83, 36, 83, 36, 83, 36, 83, 36},
    { 36,-83, 36,-83, 36,-83, 36,-83},
    { 89, 18, 75, 50, 89, 18, 75, 50},
    { 75,-50,-18,-89, 75,-50,-18,-89},
    { 50, 75,-89, 18, 50, 75,-89, 18},
    { 18,-89,-50, 75, 18,-89,-50, 75},

    { 83, 83,-83,-83, 36, 36,-36,-36},
    { 36, 36,-36,-36,-83,-83, 83, 83},
    { 89,-89, 18,-18, 75,-75, 50,-50},
    { 75,-75,-50, 50,-18, 18,-89, 89},
    { 50,-50, 75,-75,-89, 89, 18,-18},
    { 18,-18,-89, 89,-50, 50, 75,-75},
};
void xDCT8(int16_t *pSrc, int16_t *pDst, intptr_t nStride)
{
    // Const
    __m128i c_2     = _mm_set1_epi32(2);
    __m128i c_256   = _mm_set1_epi32(256);

    // DCT1
    __m128i T00, T01, T02, T03, T04, T05, T06, T07;
    __m128i T10, T11, T12, T13, T14, T15, T16, T17;
    __m128i T20, T21, T22, T23, T24, T25, T26, T27;
    __m128i T30, T31, T32, T33;
    __m128i T40, T41, T42, T43, T44, T45, T46, T47;
    __m128i T50, T51, T52, T53, T54, T55, T56, T57;
    __m128i T60, T61;

    T00 = _mm_load_si128((__m128i*)&pSrc[0 * 8]);   // [07 06 05 04 03 02 01 00]
    T01 = _mm_load_si128((__m128i*)&pSrc[1 * 8]);   // [17 16 15 14 13 12 11 10]
    T02 = _mm_load_si128((__m128i*)&pSrc[2 * 8]);   // [27 26 25 24 23 22 21 20]
    T03 = _mm_load_si128((__m128i*)&pSrc[3 * 8]);   // [37 36 35 34 33 32 31 30]
    T04 = _mm_load_si128((__m128i*)&pSrc[4 * 8]);   // [47 46 45 44 43 42 41 40]
    T05 = _mm_load_si128((__m128i*)&pSrc[5 * 8]);   // [57 56 55 54 53 52 51 50]
    T06 = _mm_load_si128((__m128i*)&pSrc[6 * 8]);   // [67 66 65 64 63 62 61 60]
    T07 = _mm_load_si128((__m128i*)&pSrc[7 * 8]);   // [77 76 75 74 73 72 71 70]

    T10 = _mm_shuffle_epi8(T00, _mm_load_si128((__m128i*)tab_dct_8x8[0]));  // [05 02 06 01 04 03 07 00]
    T11 = _mm_shuffle_epi8(T01, _mm_load_si128((__m128i*)tab_dct_8x8[0]));
    T12 = _mm_shuffle_epi8(T02, _mm_load_si128((__m128i*)tab_dct_8x8[0]));
    T13 = _mm_shuffle_epi8(T03, _mm_load_si128((__m128i*)tab_dct_8x8[0]));
    T14 = _mm_shuffle_epi8(T04, _mm_load_si128((__m128i*)tab_dct_8x8[0]));
    T15 = _mm_shuffle_epi8(T05, _mm_load_si128((__m128i*)tab_dct_8x8[0]));
    T16 = _mm_shuffle_epi8(T06, _mm_load_si128((__m128i*)tab_dct_8x8[0]));
    T17 = _mm_shuffle_epi8(T07, _mm_load_si128((__m128i*)tab_dct_8x8[0]));

    T20 = _mm_hadd_epi16(T10, T11);     // [s25_1 s16_1 s34_1 s07_1 s25_0 s16_0 s34_0 s07_0]
    T21 = _mm_hadd_epi16(T12, T13);     // [s25_3 s16_3 s34_3 s07_3 s25_2 s16_2 s34_2 s07_2]
    T22 = _mm_hadd_epi16(T14, T15);     // [s25_5 s16_5 s34_5 s07_5 s25_4 s16_4 s34_4 s07_4]
    T23 = _mm_hadd_epi16(T16, T17);     // [s25_7 s16_7 s34_7 s07_7 s25_6 s16_6 s34_6 s07_6]

    T24 = _mm_hsub_epi16(T10, T11);     // [d25_1 d16_1 d34_1 d07_1 d25_0 d16_0 d34_0 d07_0]
    T25 = _mm_hsub_epi16(T12, T13);     // [d25_3 d16_3 d34_3 d07_3 d25_2 d16_2 d34_2 d07_2]
    T26 = _mm_hsub_epi16(T14, T15);     // [d25_5 d16_5 d34_5 d07_5 d25_4 d16_4 d34_4 d07_4]
    T27 = _mm_hsub_epi16(T16, T17);     // [d25_7 d16_7 d34_7 d07_7 d25_6 d16_6 d34_6 d07_6]

    T30 = _mm_hadd_epi16(T20, T21);     // [EE1_3 EE0_3 EE1_2 EE0_2 EE1_1 EE0_1 EE1_0 EE0_0]
    T31 = _mm_hadd_epi16(T22, T23);     // [EE1_7 EE0_7 EE1_6 EE0_6 EE1_5 EE0_5 EE1_4 EE0_4]
    T32 = _mm_hsub_epi16(T20, T21);     // [EO1_3 EO0_3 EO1_2 EO0_2 EO1_1 EO0_1 EO1_0 EO0_0]
    T33 = _mm_hsub_epi16(T22, T23);     // [EO1_7 EO0_7 EO1_6 EO0_6 EO1_5 EO0_5 EO1_4 EO0_4]

    T40 = _mm_madd_epi16(T30, _mm_load_si128((__m128i*)tab_dct_8x8[1]));
    T41 = _mm_madd_epi16(T31, _mm_load_si128((__m128i*)tab_dct_8x8[1]));
    T40 = _mm_srai_epi32(_mm_add_epi32(T40, c_2), 2);
    T41 = _mm_srai_epi32(_mm_add_epi32(T41, c_2), 2);
    T50 = _mm_packs_epi32(T40, T41);

    T42 = _mm_madd_epi16(T30, _mm_load_si128((__m128i*)tab_dct_8x8[2]));
    T43 = _mm_madd_epi16(T31, _mm_load_si128((__m128i*)tab_dct_8x8[2]));
    T42 = _mm_srai_epi32(_mm_add_epi32(T42, c_2), 2);
    T43 = _mm_srai_epi32(_mm_add_epi32(T43, c_2), 2);
    T54 = _mm_packs_epi32(T42, T43);

    T44 = _mm_madd_epi16(T32, _mm_load_si128((__m128i*)tab_dct_8x8[3]));
    T45 = _mm_madd_epi16(T33, _mm_load_si128((__m128i*)tab_dct_8x8[3]));
    T44 = _mm_srai_epi32(_mm_add_epi32(T44, c_2), 2);
    T45 = _mm_srai_epi32(_mm_add_epi32(T45, c_2), 2);
    T52 = _mm_packs_epi32(T44, T45);

    T46 = _mm_madd_epi16(T32, _mm_load_si128((__m128i*)tab_dct_8x8[4]));
    T47 = _mm_madd_epi16(T33, _mm_load_si128((__m128i*)tab_dct_8x8[4]));
    T46 = _mm_srai_epi32(_mm_add_epi32(T46, c_2), 2);
    T47 = _mm_srai_epi32(_mm_add_epi32(T47, c_2), 2);
    T56 = _mm_packs_epi32(T46, T47);

    T40 = _mm_madd_epi16(T24, _mm_load_si128((__m128i*)tab_dct_8x8[5]));
    T41 = _mm_madd_epi16(T25, _mm_load_si128((__m128i*)tab_dct_8x8[5]));
    T42 = _mm_madd_epi16(T26, _mm_load_si128((__m128i*)tab_dct_8x8[5]));
    T43 = _mm_madd_epi16(T27, _mm_load_si128((__m128i*)tab_dct_8x8[5]));
    T40 = _mm_hadd_epi32(T40, T41);
    T42 = _mm_hadd_epi32(T42, T43);
    T40 = _mm_srai_epi32(_mm_add_epi32(T40, c_2), 2);
    T42 = _mm_srai_epi32(_mm_add_epi32(T42, c_2), 2);
    T51 = _mm_packs_epi32(T40, T42);

    T40 = _mm_madd_epi16(T24, _mm_load_si128((__m128i*)tab_dct_8x8[6]));
    T41 = _mm_madd_epi16(T25, _mm_load_si128((__m128i*)tab_dct_8x8[6]));
    T42 = _mm_madd_epi16(T26, _mm_load_si128((__m128i*)tab_dct_8x8[6]));
    T43 = _mm_madd_epi16(T27, _mm_load_si128((__m128i*)tab_dct_8x8[6]));
    T40 = _mm_hadd_epi32(T40, T41);
    T42 = _mm_hadd_epi32(T42, T43);
    T40 = _mm_srai_epi32(_mm_add_epi32(T40, c_2), 2);
    T42 = _mm_srai_epi32(_mm_add_epi32(T42, c_2), 2);
    T53 = _mm_packs_epi32(T40, T42);

    T40 = _mm_madd_epi16(T24, _mm_load_si128((__m128i*)tab_dct_8x8[7]));
    T41 = _mm_madd_epi16(T25, _mm_load_si128((__m128i*)tab_dct_8x8[7]));
    T42 = _mm_madd_epi16(T26, _mm_load_si128((__m128i*)tab_dct_8x8[7]));
    T43 = _mm_madd_epi16(T27, _mm_load_si128((__m128i*)tab_dct_8x8[7]));
    T40 = _mm_hadd_epi32(T40, T41);
    T42 = _mm_hadd_epi32(T42, T43);
    T40 = _mm_srai_epi32(_mm_add_epi32(T40, c_2), 2);
    T42 = _mm_srai_epi32(_mm_add_epi32(T42, c_2), 2);
    T55 = _mm_packs_epi32(T40, T42);

    T40 = _mm_madd_epi16(T24, _mm_load_si128((__m128i*)tab_dct_8x8[8]));
    T41 = _mm_madd_epi16(T25, _mm_load_si128((__m128i*)tab_dct_8x8[8]));
    T42 = _mm_madd_epi16(T26, _mm_load_si128((__m128i*)tab_dct_8x8[8]));
    T43 = _mm_madd_epi16(T27, _mm_load_si128((__m128i*)tab_dct_8x8[8]));
    T40 = _mm_hadd_epi32(T40, T41);
    T42 = _mm_hadd_epi32(T42, T43);
    T40 = _mm_srai_epi32(_mm_add_epi32(T40, c_2), 2);
    T42 = _mm_srai_epi32(_mm_add_epi32(T42, c_2), 2);
    T57 = _mm_packs_epi32(T40, T42);

    T10 = _mm_shuffle_epi8(T50, _mm_load_si128((__m128i*)tab_dct_8x8[0]));  // [05 02 06 01 04 03 07 00]
    T11 = _mm_shuffle_epi8(T51, _mm_load_si128((__m128i*)tab_dct_8x8[0]));
    T12 = _mm_shuffle_epi8(T52, _mm_load_si128((__m128i*)tab_dct_8x8[0]));
    T13 = _mm_shuffle_epi8(T53, _mm_load_si128((__m128i*)tab_dct_8x8[0]));
    T14 = _mm_shuffle_epi8(T54, _mm_load_si128((__m128i*)tab_dct_8x8[0]));
    T15 = _mm_shuffle_epi8(T55, _mm_load_si128((__m128i*)tab_dct_8x8[0]));
    T16 = _mm_shuffle_epi8(T56, _mm_load_si128((__m128i*)tab_dct_8x8[0]));
    T17 = _mm_shuffle_epi8(T57, _mm_load_si128((__m128i*)tab_dct_8x8[0]));

    // DCT2
    T20 = _mm_madd_epi16(T10, _mm_load_si128((__m128i*)tab_dct_8x8[1]));    // [64*s25_0 64*s16_0 64*s34_0 64*s07_0]
    T21 = _mm_madd_epi16(T11, _mm_load_si128((__m128i*)tab_dct_8x8[1]));    // [64*s25_1 64*s16_1 64*s34_1 64*s07_1]
    T22 = _mm_madd_epi16(T12, _mm_load_si128((__m128i*)tab_dct_8x8[1]));    // [64*s25_2 64*s16_2 64*s34_2 64*s07_2]
    T23 = _mm_madd_epi16(T13, _mm_load_si128((__m128i*)tab_dct_8x8[1]));    // [64*s25_3 64*s16_3 64*s34_3 64*s07_3]
    T24 = _mm_madd_epi16(T14, _mm_load_si128((__m128i*)tab_dct_8x8[1]));    // [64*s25_4 64*s16_4 64*s34_4 64*s07_4]
    T25 = _mm_madd_epi16(T15, _mm_load_si128((__m128i*)tab_dct_8x8[1]));    // [64*s25_5 64*s16_5 64*s34_5 64*s07_5]
    T26 = _mm_madd_epi16(T16, _mm_load_si128((__m128i*)tab_dct_8x8[1]));    // [64*s25_6 64*s16_6 64*s34_6 64*s07_6]
    T27 = _mm_madd_epi16(T17, _mm_load_si128((__m128i*)tab_dct_8x8[1]));    // [64*s25_7 64*s16_7 64*s34_7 64*s07_7]

    T30 = _mm_hadd_epi32(T20, T21); // [64*(s16+s25)_1 64*(s07+s34)_1 64*(s16+s25)_0 64*(s07+s34)_0]
    T31 = _mm_hadd_epi32(T22, T23); // [64*(s16+s25)_3 64*(s07+s34)_3 64*(s16+s25)_2 64*(s07+s34)_2]
    T32 = _mm_hadd_epi32(T24, T25); // [64*(s16+s25)_5 64*(s07+s34)_5 64*(s16+s25)_4 64*(s07+s34)_4]
    T33 = _mm_hadd_epi32(T26, T27); // [64*(s16+s25)_7 64*(s07+s34)_7 64*(s16+s25)_6 64*(s07+s34)_6]

    T40 = _mm_hadd_epi32(T30, T31); // [64*((s07+s34)+(s16+s25))_3 64*((s07+s34)+(s16+s25))_2 64*((s07+s34)+(s16+s25))_1 64*((s07+s34)+(s16+s25))_0]
    T41 = _mm_hadd_epi32(T32, T33); // [64*((s07+s34)+(s16+s25))_7 64*((s07+s34)+(s16+s25))_6 64*((s07+s34)+(s16+s25))_5 64*((s07+s34)+(s16+s25))_4]
    T42 = _mm_hsub_epi32(T30, T31); // [64*((s07+s34)-(s16+s25))_3 64*((s07+s34)-(s16+s25))_2 64*((s07+s34)-(s16+s25))_1 64*((s07+s34)-(s16+s25))_0]
    T43 = _mm_hsub_epi32(T32, T33); // [64*((s07+s34)-(s16+s25))_7 64*((s07+s34)-(s16+s25))_6 64*((s07+s34)-(s16+s25))_5 64*((s07+s34)-(s16+s25))_4]

    T50 = _mm_srai_epi32(_mm_add_epi32(T40, c_256), 9);
    T51 = _mm_srai_epi32(_mm_add_epi32(T41, c_256), 9);
    T52 = _mm_srai_epi32(_mm_add_epi32(T42, c_256), 9);
    T53 = _mm_srai_epi32(_mm_add_epi32(T43, c_256), 9);

    T60 = _mm_packs_epi32(T50, T51);
    T61 = _mm_packs_epi32(T52, T53);
    _mm_store_si128((__m128i*)&pDst[0*nStride], T60);
    _mm_store_si128((__m128i*)&pDst[4*nStride], T61);

#define MAKE_ODD(tab, dst) \
    T20 = _mm_madd_epi16(T10, _mm_load_si128((__m128i*)tab_dct_8x8[(tab)])); \
    T21 = _mm_madd_epi16(T11, _mm_load_si128((__m128i*)tab_dct_8x8[(tab)])); \
    T22 = _mm_madd_epi16(T12, _mm_load_si128((__m128i*)tab_dct_8x8[(tab)])); \
    T23 = _mm_madd_epi16(T13, _mm_load_si128((__m128i*)tab_dct_8x8[(tab)])); \
    T24 = _mm_madd_epi16(T14, _mm_load_si128((__m128i*)tab_dct_8x8[(tab)])); \
    T25 = _mm_madd_epi16(T15, _mm_load_si128((__m128i*)tab_dct_8x8[(tab)])); \
    T26 = _mm_madd_epi16(T16, _mm_load_si128((__m128i*)tab_dct_8x8[(tab)])); \
    T27 = _mm_madd_epi16(T17, _mm_load_si128((__m128i*)tab_dct_8x8[(tab)])); \
    T30 = _mm_hadd_epi32(T20, T21); \
    T31 = _mm_hadd_epi32(T22, T23); \
    T32 = _mm_hadd_epi32(T24, T25); \
    T33 = _mm_hadd_epi32(T26, T27); \
    T40 = _mm_hadd_epi32(T30, T31); \
    T41 = _mm_hadd_epi32(T32, T33); \
    T50 = _mm_srai_epi32(_mm_add_epi32(T40, c_256), 9); \
    T51 = _mm_srai_epi32(_mm_add_epi32(T41, c_256), 9); \
    T60 = _mm_packs_epi32(T50, T51); \
    _mm_store_si128((__m128i*)&pDst[(dst)*nStride], T60); \

    MAKE_ODD( 9, 2);
    MAKE_ODD(10, 6);
    MAKE_ODD(11, 1);
    MAKE_ODD(12, 3);
    MAKE_ODD(13, 5);
    MAKE_ODD(14, 7);
#undef MAKE_ODD
}
#endif // INSTRSET >= 4

#if INSTRSET < 4
void xDCT16(short *pSrc, short *pDst, intptr_t)
{
    const int shift_1st = 3;
    const int shift_2nd = 10;
    ALIGN_VAR_32(Short, tmp[16 * 16]);

    partialButterfly16(pSrc, tmp, shift_1st, 16);
    partialButterfly16(tmp, pDst, shift_2nd, 16);
}
#endif // INSTRSET < 4

#if INSTRSET >= 4
ALIGN_VAR_32(static const short, tab_dct_16x16_0[][8])=
{
    { 0x0F0E, 0x0D0C, 0x0B0A, 0x0908, 0x0706, 0x0504, 0x0302, 0x0100},  // 0
    { 0x0100, 0x0F0E, 0x0706, 0x0908, 0x0302, 0x0D0C, 0x0504, 0x0B0A},  // 1
    { 0x0100, 0x0706, 0x0302, 0x0504, 0x0F0E, 0x0908, 0x0D0C, 0x0B0A},  // 2
    { 0x0F0E, 0x0908, 0x0D0C, 0x0B0A, 0x0100, 0x0706, 0x0302, 0x0504},  // 3
};

ALIGN_VAR_32(static const short, tab_dct_16x16_1[][8])=
{
    { 90, 87, 80, 70, 57, 43, 25,  9},  //  0
    { 87, 57,  9,-43,-80,-90,-70,-25},  //  1
    { 80,  9,-70,-87,-25, 57, 90, 43},  //  2
    { 70,-43,-87,  9, 90, 25,-80,-57},  //  3
    { 57,-80,-25, 90,- 9,-87, 43, 70},  //  4
    { 43,-90, 57, 25,-87, 70,  9,-80},  //  5
    { 25,-70, 90,-80, 43,  9,-57, 87},  //  6
    {  9,-25, 43,-57, 70,-80, 87,-90},  //  7
    { 83, 83,-83,-83, 36, 36,-36,-36},  //  8
    { 36, 36,-36,-36,-83,-83, 83, 83},  //  9
    { 89, 89, 18, 18, 75, 75, 50, 50},  // 10
    { 75, 75,-50,-50,-18,-18,-89,-89},  // 11
    { 50, 50, 75, 75,-89,-89, 18, 18},  // 12
    { 18, 18,-89,-89,-50,-50, 75, 75},  // 13

#define MAKE_COEF(a0, a1, a2, a3, a4, a5, a6, a7) \
    { (a0), -(a0), (a3), -(a3), (a1), -(a1), (a2), -(a2)}, \
    { (a7), -(a7), (a4), -(a4), (a6), -(a6), (a5), -(a5)},

    MAKE_COEF(90, 87, 80, 70, 57, 43, 25,  9)
    MAKE_COEF(87, 57,  9,-43,-80,-90,-70,-25)
    MAKE_COEF(80,  9,-70,-87,-25, 57, 90, 43)
    MAKE_COEF(70,-43,-87,  9, 90, 25,-80,-57)
    MAKE_COEF(57,-80,-25, 90, -9,-87, 43, 70)
    MAKE_COEF(43,-90, 57, 25,-87, 70,  9,-80)
    MAKE_COEF(25,-70, 90,-80, 43,  9,-57, 87)
    MAKE_COEF( 9,-25, 43,-57, 70,-80, 87,-90)
#undef MAKE_COEF
};

void xDCT16(short *pSrc, short *pDst, intptr_t nStride)
{
    // Const
    __m128i c_4     = _mm_set1_epi32(4);
    __m128i c_512   = _mm_set1_epi32(512);

    int i;

    ALIGN_VAR_32(short, tmp[16*16]);

    __m128i T00A, T01A, T02A, T03A, T04A, T05A, T06A, T07A;
    __m128i T00B, T01B, T02B, T03B, T04B, T05B, T06B, T07B;
    __m128i T10, T11, T12, T13, T14, T15, T16, T17;
    __m128i T20, T21, T22, T23, T24, T25, T26, T27;
    __m128i T30, T31, T32, T33, T34, T35, T36, T37;
    __m128i T40, T41, T42, T43, T44, T45, T46, T47;
    __m128i T50, T51, T52, T53;
    __m128i T60, T61, T62, T63, T64, T65, T66, T67;
    __m128i T70;

    // DCT1
    for( i=0; i<16; i+=8 )
    {
        T00A = _mm_load_si128((__m128i*)&pSrc[(i+0)*16+0]);    // [07 06 05 04 03 02 01 00]
        T00B = _mm_load_si128((__m128i*)&pSrc[(i+0)*16+8]);    // [0F 0E 0D 0C 0B 0A 09 08]
        T01A = _mm_load_si128((__m128i*)&pSrc[(i+1)*16+0]);    // [17 16 15 14 13 12 11 10]
        T01B = _mm_load_si128((__m128i*)&pSrc[(i+1)*16+8]);    // [1F 1E 1D 1C 1B 1A 19 18]
        T02A = _mm_load_si128((__m128i*)&pSrc[(i+2)*16+0]);    // [27 26 25 24 23 22 21 20]
        T02B = _mm_load_si128((__m128i*)&pSrc[(i+2)*16+8]);    // [2F 2E 2D 2C 2B 2A 29 28]
        T03A = _mm_load_si128((__m128i*)&pSrc[(i+3)*16+0]);    // [37 36 35 34 33 32 31 30]
        T03B = _mm_load_si128((__m128i*)&pSrc[(i+3)*16+8]);    // [3F 3E 3D 3C 3B 3A 39 38]
        T04A = _mm_load_si128((__m128i*)&pSrc[(i+4)*16+0]);    // [47 46 45 44 43 42 41 40]
        T04B = _mm_load_si128((__m128i*)&pSrc[(i+4)*16+8]);    // [4F 4E 4D 4C 4B 4A 49 48]
        T05A = _mm_load_si128((__m128i*)&pSrc[(i+5)*16+0]);    // [57 56 55 54 53 52 51 50]
        T05B = _mm_load_si128((__m128i*)&pSrc[(i+5)*16+8]);    // [5F 5E 5D 5C 5B 5A 59 58]
        T06A = _mm_load_si128((__m128i*)&pSrc[(i+6)*16+0]);    // [67 66 65 64 63 62 61 60]
        T06B = _mm_load_si128((__m128i*)&pSrc[(i+6)*16+8]);    // [6F 6E 6D 6C 6B 6A 69 68]
        T07A = _mm_load_si128((__m128i*)&pSrc[(i+7)*16+0]);    // [77 76 75 74 73 72 71 70]
        T07B = _mm_load_si128((__m128i*)&pSrc[(i+7)*16+8]);    // [7F 7E 7D 7C 7B 7A 79 78]

        T00B = _mm_shuffle_epi8(T00B, _mm_load_si128((__m128i*)tab_dct_16x16_0[0]));
        T01B = _mm_shuffle_epi8(T01B, _mm_load_si128((__m128i*)tab_dct_16x16_0[0]));
        T02B = _mm_shuffle_epi8(T02B, _mm_load_si128((__m128i*)tab_dct_16x16_0[0]));
        T03B = _mm_shuffle_epi8(T03B, _mm_load_si128((__m128i*)tab_dct_16x16_0[0]));
        T04B = _mm_shuffle_epi8(T04B, _mm_load_si128((__m128i*)tab_dct_16x16_0[0]));
        T05B = _mm_shuffle_epi8(T05B, _mm_load_si128((__m128i*)tab_dct_16x16_0[0]));
        T06B = _mm_shuffle_epi8(T06B, _mm_load_si128((__m128i*)tab_dct_16x16_0[0]));
        T07B = _mm_shuffle_epi8(T07B, _mm_load_si128((__m128i*)tab_dct_16x16_0[0]));

        T10  = _mm_add_epi16(T00A, T00B);
        T11  = _mm_add_epi16(T01A, T01B);
        T12  = _mm_add_epi16(T02A, T02B);
        T13  = _mm_add_epi16(T03A, T03B);
        T14  = _mm_add_epi16(T04A, T04B);
        T15  = _mm_add_epi16(T05A, T05B);
        T16  = _mm_add_epi16(T06A, T06B);
        T17  = _mm_add_epi16(T07A, T07B);

        T20  = _mm_sub_epi16(T00A, T00B);
        T21  = _mm_sub_epi16(T01A, T01B);
        T22  = _mm_sub_epi16(T02A, T02B);
        T23  = _mm_sub_epi16(T03A, T03B);
        T24  = _mm_sub_epi16(T04A, T04B);
        T25  = _mm_sub_epi16(T05A, T05B);
        T26  = _mm_sub_epi16(T06A, T06B);
        T27  = _mm_sub_epi16(T07A, T07B);

        T30  = _mm_shuffle_epi8(T10, _mm_load_si128((__m128i*)tab_dct_16x16_0[1]));
        T31  = _mm_shuffle_epi8(T11, _mm_load_si128((__m128i*)tab_dct_16x16_0[1]));
        T32  = _mm_shuffle_epi8(T12, _mm_load_si128((__m128i*)tab_dct_16x16_0[1]));
        T33  = _mm_shuffle_epi8(T13, _mm_load_si128((__m128i*)tab_dct_16x16_0[1]));
        T34  = _mm_shuffle_epi8(T14, _mm_load_si128((__m128i*)tab_dct_16x16_0[1]));
        T35  = _mm_shuffle_epi8(T15, _mm_load_si128((__m128i*)tab_dct_16x16_0[1]));
        T36  = _mm_shuffle_epi8(T16, _mm_load_si128((__m128i*)tab_dct_16x16_0[1]));
        T37  = _mm_shuffle_epi8(T17, _mm_load_si128((__m128i*)tab_dct_16x16_0[1]));

        T40  = _mm_hadd_epi16(T30, T31);
        T41  = _mm_hadd_epi16(T32, T33);
        T42  = _mm_hadd_epi16(T34, T35);
        T43  = _mm_hadd_epi16(T36, T37);
        T44  = _mm_hsub_epi16(T30, T31);
        T45  = _mm_hsub_epi16(T32, T33);
        T46  = _mm_hsub_epi16(T34, T35);
        T47  = _mm_hsub_epi16(T36, T37);

        T50  = _mm_hadd_epi16(T40, T41);
        T51  = _mm_hadd_epi16(T42, T43);
        T52  = _mm_hsub_epi16(T40, T41);
        T53  = _mm_hsub_epi16(T42, T43);

        T60  = _mm_madd_epi16(T50, _mm_load_si128((__m128i*)tab_dct_8x8[ 1]));
        T61  = _mm_madd_epi16(T51, _mm_load_si128((__m128i*)tab_dct_8x8[ 1]));
        T60  = _mm_srai_epi32(_mm_add_epi32(T60, c_4), 3);
        T61  = _mm_srai_epi32(_mm_add_epi32(T61, c_4), 3);
        T70  = _mm_packs_epi32(T60, T61);
        _mm_store_si128((__m128i*)&tmp[ 0*16+i], T70);

        T60  = _mm_madd_epi16(T50, _mm_load_si128((__m128i*)tab_dct_8x8[ 2]));
        T61  = _mm_madd_epi16(T51, _mm_load_si128((__m128i*)tab_dct_8x8[ 2]));
        T60  = _mm_srai_epi32(_mm_add_epi32(T60, c_4), 3);
        T61  = _mm_srai_epi32(_mm_add_epi32(T61, c_4), 3);
        T70  = _mm_packs_epi32(T60, T61);
        _mm_store_si128((__m128i*)&tmp[ 8*16+i], T70);

        T60  = _mm_madd_epi16(T52, _mm_load_si128((__m128i*)tab_dct_8x8[ 3]));
        T61  = _mm_madd_epi16(T53, _mm_load_si128((__m128i*)tab_dct_8x8[ 3]));
        T60  = _mm_srai_epi32(_mm_add_epi32(T60, c_4), 3);
        T61  = _mm_srai_epi32(_mm_add_epi32(T61, c_4), 3);
        T70  = _mm_packs_epi32(T60, T61);
        _mm_store_si128((__m128i*)&tmp[ 4*16+i], T70);

        T60  = _mm_madd_epi16(T52, _mm_load_si128((__m128i*)tab_dct_8x8[ 4]));
        T61  = _mm_madd_epi16(T53, _mm_load_si128((__m128i*)tab_dct_8x8[ 4]));
        T60  = _mm_srai_epi32(_mm_add_epi32(T60, c_4), 3);
        T61  = _mm_srai_epi32(_mm_add_epi32(T61, c_4), 3);
        T70  = _mm_packs_epi32(T60, T61);
        _mm_store_si128((__m128i*)&tmp[12*16+i], T70);

        T60  = _mm_madd_epi16(T44, _mm_load_si128((__m128i*)tab_dct_8x8[ 5]));
        T61  = _mm_madd_epi16(T45, _mm_load_si128((__m128i*)tab_dct_8x8[ 5]));
        T62  = _mm_madd_epi16(T46, _mm_load_si128((__m128i*)tab_dct_8x8[ 5]));
        T63  = _mm_madd_epi16(T47, _mm_load_si128((__m128i*)tab_dct_8x8[ 5]));
        T60  = _mm_hadd_epi32(T60, T61);
        T61  = _mm_hadd_epi32(T62, T63);
        T60  = _mm_srai_epi32(_mm_add_epi32(T60, c_4), 3);
        T61  = _mm_srai_epi32(_mm_add_epi32(T61, c_4), 3);
        T70  = _mm_packs_epi32(T60, T61);
        _mm_store_si128((__m128i*)&tmp[ 2*16+i], T70);

        T60  = _mm_madd_epi16(T44, _mm_load_si128((__m128i*)tab_dct_8x8[ 6]));
        T61  = _mm_madd_epi16(T45, _mm_load_si128((__m128i*)tab_dct_8x8[ 6]));
        T62  = _mm_madd_epi16(T46, _mm_load_si128((__m128i*)tab_dct_8x8[ 6]));
        T63  = _mm_madd_epi16(T47, _mm_load_si128((__m128i*)tab_dct_8x8[ 6]));
        T60  = _mm_hadd_epi32(T60, T61);
        T61  = _mm_hadd_epi32(T62, T63);
        T60  = _mm_srai_epi32(_mm_add_epi32(T60, c_4), 3);
        T61  = _mm_srai_epi32(_mm_add_epi32(T61, c_4), 3);
        T70  = _mm_packs_epi32(T60, T61);
        _mm_store_si128((__m128i*)&tmp[ 6*16+i], T70);

        T60  = _mm_madd_epi16(T44, _mm_load_si128((__m128i*)tab_dct_8x8[ 7]));
        T61  = _mm_madd_epi16(T45, _mm_load_si128((__m128i*)tab_dct_8x8[ 7]));
        T62  = _mm_madd_epi16(T46, _mm_load_si128((__m128i*)tab_dct_8x8[ 7]));
        T63  = _mm_madd_epi16(T47, _mm_load_si128((__m128i*)tab_dct_8x8[ 7]));
        T60  = _mm_hadd_epi32(T60, T61);
        T61  = _mm_hadd_epi32(T62, T63);
        T60  = _mm_srai_epi32(_mm_add_epi32(T60, c_4), 3);
        T61  = _mm_srai_epi32(_mm_add_epi32(T61, c_4), 3);
        T70  = _mm_packs_epi32(T60, T61);
        _mm_store_si128((__m128i*)&tmp[10*16+i], T70);

        T60  = _mm_madd_epi16(T44, _mm_load_si128((__m128i*)tab_dct_8x8[ 8]));
        T61  = _mm_madd_epi16(T45, _mm_load_si128((__m128i*)tab_dct_8x8[ 8]));
        T62  = _mm_madd_epi16(T46, _mm_load_si128((__m128i*)tab_dct_8x8[ 8]));
        T63  = _mm_madd_epi16(T47, _mm_load_si128((__m128i*)tab_dct_8x8[ 8]));
        T60  = _mm_hadd_epi32(T60, T61);
        T61  = _mm_hadd_epi32(T62, T63);
        T60  = _mm_srai_epi32(_mm_add_epi32(T60, c_4), 3);
        T61  = _mm_srai_epi32(_mm_add_epi32(T61, c_4), 3);
        T70  = _mm_packs_epi32(T60, T61);
        _mm_store_si128((__m128i*)&tmp[14*16+i], T70);

#define MAKE_ODD(tab, dst) \
        T60  = _mm_madd_epi16(T20, _mm_load_si128((__m128i*)tab_dct_16x16_1[(tab)])); \
        T61  = _mm_madd_epi16(T21, _mm_load_si128((__m128i*)tab_dct_16x16_1[(tab)])); \
        T62  = _mm_madd_epi16(T22, _mm_load_si128((__m128i*)tab_dct_16x16_1[(tab)])); \
        T63  = _mm_madd_epi16(T23, _mm_load_si128((__m128i*)tab_dct_16x16_1[(tab)])); \
        T64  = _mm_madd_epi16(T24, _mm_load_si128((__m128i*)tab_dct_16x16_1[(tab)])); \
        T65  = _mm_madd_epi16(T25, _mm_load_si128((__m128i*)tab_dct_16x16_1[(tab)])); \
        T66  = _mm_madd_epi16(T26, _mm_load_si128((__m128i*)tab_dct_16x16_1[(tab)])); \
        T67  = _mm_madd_epi16(T27, _mm_load_si128((__m128i*)tab_dct_16x16_1[(tab)])); \
        T60  = _mm_hadd_epi32(T60, T61); \
        T61  = _mm_hadd_epi32(T62, T63); \
        T62  = _mm_hadd_epi32(T64, T65); \
        T63  = _mm_hadd_epi32(T66, T67); \
        T60  = _mm_hadd_epi32(T60, T61); \
        T61  = _mm_hadd_epi32(T62, T63); \
        T60  = _mm_srai_epi32(_mm_add_epi32(T60, c_4), 3); \
        T61  = _mm_srai_epi32(_mm_add_epi32(T61, c_4), 3); \
        T70  = _mm_packs_epi32(T60, T61); \
        _mm_store_si128((__m128i*)&tmp[(dst)*16+i], T70);

        MAKE_ODD( 0, 1);
        MAKE_ODD( 1, 3);
        MAKE_ODD( 2, 5);
        MAKE_ODD( 3, 7);
        MAKE_ODD( 4, 9);
        MAKE_ODD( 5,11);
        MAKE_ODD( 6,13);
        MAKE_ODD( 7,15);
#undef MAKE_ODD
    }

    // DCT2
    for( i=0; i<16; i+=4 )
    {
        T00A = _mm_load_si128((__m128i*)&tmp[(i+0)*16+0]);    // [07 06 05 04 03 02 01 00]
        T00B = _mm_load_si128((__m128i*)&tmp[(i+0)*16+8]);    // [0F 0E 0D 0C 0B 0A 09 08]
        T01A = _mm_load_si128((__m128i*)&tmp[(i+1)*16+0]);    // [17 16 15 14 13 12 11 10]
        T01B = _mm_load_si128((__m128i*)&tmp[(i+1)*16+8]);    // [1F 1E 1D 1C 1B 1A 19 18]
        T02A = _mm_load_si128((__m128i*)&tmp[(i+2)*16+0]);    // [27 26 25 24 23 22 21 20]
        T02B = _mm_load_si128((__m128i*)&tmp[(i+2)*16+8]);    // [2F 2E 2D 2C 2B 2A 29 28]
        T03A = _mm_load_si128((__m128i*)&tmp[(i+3)*16+0]);    // [37 36 35 34 33 32 31 30]
        T03B = _mm_load_si128((__m128i*)&tmp[(i+3)*16+8]);    // [3F 3E 3D 3C 3B 3A 39 38]

        T00A = _mm_shuffle_epi8(T00A, _mm_load_si128((__m128i*)tab_dct_16x16_0[2]));
        T00B = _mm_shuffle_epi8(T00B, _mm_load_si128((__m128i*)tab_dct_16x16_0[3]));
        T01A = _mm_shuffle_epi8(T01A, _mm_load_si128((__m128i*)tab_dct_16x16_0[2]));
        T01B = _mm_shuffle_epi8(T01B, _mm_load_si128((__m128i*)tab_dct_16x16_0[3]));
        T02A = _mm_shuffle_epi8(T02A, _mm_load_si128((__m128i*)tab_dct_16x16_0[2]));
        T02B = _mm_shuffle_epi8(T02B, _mm_load_si128((__m128i*)tab_dct_16x16_0[3]));
        T03A = _mm_shuffle_epi8(T03A, _mm_load_si128((__m128i*)tab_dct_16x16_0[2]));
        T03B = _mm_shuffle_epi8(T03B, _mm_load_si128((__m128i*)tab_dct_16x16_0[3]));

        T10  = _mm_unpacklo_epi16(T00A, T00B);
        T11  = _mm_unpackhi_epi16(T00A, T00B);
        T12  = _mm_unpacklo_epi16(T01A, T01B);
        T13  = _mm_unpackhi_epi16(T01A, T01B);
        T14  = _mm_unpacklo_epi16(T02A, T02B);
        T15  = _mm_unpackhi_epi16(T02A, T02B);
        T16  = _mm_unpacklo_epi16(T03A, T03B);
        T17  = _mm_unpackhi_epi16(T03A, T03B);

        T20  = _mm_madd_epi16(T10, _mm_load_si128((__m128i*)tab_dct_8x8[1]));
        T21  = _mm_madd_epi16(T11, _mm_load_si128((__m128i*)tab_dct_8x8[1]));
        T22  = _mm_madd_epi16(T12, _mm_load_si128((__m128i*)tab_dct_8x8[1]));
        T23  = _mm_madd_epi16(T13, _mm_load_si128((__m128i*)tab_dct_8x8[1]));
        T24  = _mm_madd_epi16(T14, _mm_load_si128((__m128i*)tab_dct_8x8[1]));
        T25  = _mm_madd_epi16(T15, _mm_load_si128((__m128i*)tab_dct_8x8[1]));
        T26  = _mm_madd_epi16(T16, _mm_load_si128((__m128i*)tab_dct_8x8[1]));
        T27  = _mm_madd_epi16(T17, _mm_load_si128((__m128i*)tab_dct_8x8[1]));

        T30  = _mm_add_epi32(T20, T21);
        T31  = _mm_add_epi32(T22, T23);
        T32  = _mm_add_epi32(T24, T25);
        T33  = _mm_add_epi32(T26, T27);

        T30  = _mm_hadd_epi32(T30, T31);
        T31  = _mm_hadd_epi32(T32, T33);

        T40  = _mm_hadd_epi32(T30, T31);
        T41  = _mm_hsub_epi32(T30, T31);
        T40  = _mm_srai_epi32(_mm_add_epi32(T40, c_512), 10);
        T41  = _mm_srai_epi32(_mm_add_epi32(T41, c_512), 10);
        T60  = _mm_packs_epi32(T40, T40);
        T61  = _mm_packs_epi32(T41, T41);
        _mm_storel_epi64((__m128i*)&pDst[ 0*nStride+i], T60);
        _mm_storel_epi64((__m128i*)&pDst[ 8*nStride+i], T61);

        T20  = _mm_madd_epi16(T10, _mm_load_si128((__m128i*)tab_dct_16x16_1[ 8]));
        T21  = _mm_madd_epi16(T11, _mm_load_si128((__m128i*)tab_dct_16x16_1[ 8]));
        T22  = _mm_madd_epi16(T12, _mm_load_si128((__m128i*)tab_dct_16x16_1[ 8]));
        T23  = _mm_madd_epi16(T13, _mm_load_si128((__m128i*)tab_dct_16x16_1[ 8]));
        T24  = _mm_madd_epi16(T14, _mm_load_si128((__m128i*)tab_dct_16x16_1[ 8]));
        T25  = _mm_madd_epi16(T15, _mm_load_si128((__m128i*)tab_dct_16x16_1[ 8]));
        T26  = _mm_madd_epi16(T16, _mm_load_si128((__m128i*)tab_dct_16x16_1[ 8]));
        T27  = _mm_madd_epi16(T17, _mm_load_si128((__m128i*)tab_dct_16x16_1[ 8]));

        T30  = _mm_add_epi32(T20, T21);
        T31  = _mm_add_epi32(T22, T23);
        T32  = _mm_add_epi32(T24, T25);
        T33  = _mm_add_epi32(T26, T27);

        T30  = _mm_hadd_epi32(T30, T31);
        T31  = _mm_hadd_epi32(T32, T33);

        T40  = _mm_hadd_epi32(T30, T31);
        T40  = _mm_srai_epi32(_mm_add_epi32(T40, c_512), 10);
        T60  = _mm_packs_epi32(T40, T40);
        _mm_storel_epi64((__m128i*)&pDst[ 4*nStride+i], T60);

        T20  = _mm_madd_epi16(T10, _mm_load_si128((__m128i*)tab_dct_16x16_1[ 9]));
        T21  = _mm_madd_epi16(T11, _mm_load_si128((__m128i*)tab_dct_16x16_1[ 9]));
        T22  = _mm_madd_epi16(T12, _mm_load_si128((__m128i*)tab_dct_16x16_1[ 9]));
        T23  = _mm_madd_epi16(T13, _mm_load_si128((__m128i*)tab_dct_16x16_1[ 9]));
        T24  = _mm_madd_epi16(T14, _mm_load_si128((__m128i*)tab_dct_16x16_1[ 9]));
        T25  = _mm_madd_epi16(T15, _mm_load_si128((__m128i*)tab_dct_16x16_1[ 9]));
        T26  = _mm_madd_epi16(T16, _mm_load_si128((__m128i*)tab_dct_16x16_1[ 9]));
        T27  = _mm_madd_epi16(T17, _mm_load_si128((__m128i*)tab_dct_16x16_1[ 9]));

        T30  = _mm_add_epi32(T20, T21);
        T31  = _mm_add_epi32(T22, T23);
        T32  = _mm_add_epi32(T24, T25);
        T33  = _mm_add_epi32(T26, T27);

        T30  = _mm_hadd_epi32(T30, T31);
        T31  = _mm_hadd_epi32(T32, T33);

        T40  = _mm_hadd_epi32(T30, T31);
        T40  = _mm_srai_epi32(_mm_add_epi32(T40, c_512), 10);
        T60  = _mm_packs_epi32(T40, T40);
        _mm_storel_epi64((__m128i*)&pDst[12*nStride+i], T60);

        T20  = _mm_madd_epi16(T10, _mm_load_si128((__m128i*)tab_dct_16x16_1[10]));
        T21  = _mm_madd_epi16(T11, _mm_load_si128((__m128i*)tab_dct_16x16_1[10]));
        T22  = _mm_madd_epi16(T12, _mm_load_si128((__m128i*)tab_dct_16x16_1[10]));
        T23  = _mm_madd_epi16(T13, _mm_load_si128((__m128i*)tab_dct_16x16_1[10]));
        T24  = _mm_madd_epi16(T14, _mm_load_si128((__m128i*)tab_dct_16x16_1[10]));
        T25  = _mm_madd_epi16(T15, _mm_load_si128((__m128i*)tab_dct_16x16_1[10]));
        T26  = _mm_madd_epi16(T16, _mm_load_si128((__m128i*)tab_dct_16x16_1[10]));
        T27  = _mm_madd_epi16(T17, _mm_load_si128((__m128i*)tab_dct_16x16_1[10]));

        T30  = _mm_sub_epi32(T20, T21);
        T31  = _mm_sub_epi32(T22, T23);
        T32  = _mm_sub_epi32(T24, T25);
        T33  = _mm_sub_epi32(T26, T27);

        T30  = _mm_hadd_epi32(T30, T31);
        T31  = _mm_hadd_epi32(T32, T33);

        T40  = _mm_hadd_epi32(T30, T31);
        T40  = _mm_srai_epi32(_mm_add_epi32(T40, c_512), 10);
        T60  = _mm_packs_epi32(T40, T40);
        _mm_storel_epi64((__m128i*)&pDst[ 2*nStride+i], T60);

        T20  = _mm_madd_epi16(T10, _mm_load_si128((__m128i*)tab_dct_16x16_1[11]));
        T21  = _mm_madd_epi16(T11, _mm_load_si128((__m128i*)tab_dct_16x16_1[11]));
        T22  = _mm_madd_epi16(T12, _mm_load_si128((__m128i*)tab_dct_16x16_1[11]));
        T23  = _mm_madd_epi16(T13, _mm_load_si128((__m128i*)tab_dct_16x16_1[11]));
        T24  = _mm_madd_epi16(T14, _mm_load_si128((__m128i*)tab_dct_16x16_1[11]));
        T25  = _mm_madd_epi16(T15, _mm_load_si128((__m128i*)tab_dct_16x16_1[11]));
        T26  = _mm_madd_epi16(T16, _mm_load_si128((__m128i*)tab_dct_16x16_1[11]));
        T27  = _mm_madd_epi16(T17, _mm_load_si128((__m128i*)tab_dct_16x16_1[11]));

        T30  = _mm_sub_epi32(T20, T21); 
        T31  = _mm_sub_epi32(T22, T23);
        T32  = _mm_sub_epi32(T24, T25);
        T33  = _mm_sub_epi32(T26, T27);

        T30  = _mm_hadd_epi32(T30, T31);
        T31  = _mm_hadd_epi32(T32, T33);

        T40  = _mm_hadd_epi32(T30, T31);
        T40  = _mm_srai_epi32(_mm_add_epi32(T40, c_512), 10);
        T60  = _mm_packs_epi32(T40, T40);
        _mm_storel_epi64((__m128i*)&pDst[ 6*nStride+i], T60);

        T20  = _mm_madd_epi16(T10, _mm_load_si128((__m128i*)tab_dct_16x16_1[12]));
        T21  = _mm_madd_epi16(T11, _mm_load_si128((__m128i*)tab_dct_16x16_1[12]));
        T22  = _mm_madd_epi16(T12, _mm_load_si128((__m128i*)tab_dct_16x16_1[12]));
        T23  = _mm_madd_epi16(T13, _mm_load_si128((__m128i*)tab_dct_16x16_1[12]));
        T24  = _mm_madd_epi16(T14, _mm_load_si128((__m128i*)tab_dct_16x16_1[12]));
        T25  = _mm_madd_epi16(T15, _mm_load_si128((__m128i*)tab_dct_16x16_1[12]));
        T26  = _mm_madd_epi16(T16, _mm_load_si128((__m128i*)tab_dct_16x16_1[12]));
        T27  = _mm_madd_epi16(T17, _mm_load_si128((__m128i*)tab_dct_16x16_1[12]));

        T30  = _mm_sub_epi32(T20, T21); 
        T31  = _mm_sub_epi32(T22, T23);
        T32  = _mm_sub_epi32(T24, T25);
        T33  = _mm_sub_epi32(T26, T27);

        T30  = _mm_hadd_epi32(T30, T31);
        T31  = _mm_hadd_epi32(T32, T33);

        T40  = _mm_hadd_epi32(T30, T31);
        T40  = _mm_srai_epi32(_mm_add_epi32(T40, c_512), 10);
        T60  = _mm_packs_epi32(T40, T40);
        _mm_storel_epi64((__m128i*)&pDst[10*nStride+i], T60);

        T20  = _mm_madd_epi16(T10, _mm_load_si128((__m128i*)tab_dct_16x16_1[13]));
        T21  = _mm_madd_epi16(T11, _mm_load_si128((__m128i*)tab_dct_16x16_1[13]));
        T22  = _mm_madd_epi16(T12, _mm_load_si128((__m128i*)tab_dct_16x16_1[13]));
        T23  = _mm_madd_epi16(T13, _mm_load_si128((__m128i*)tab_dct_16x16_1[13]));
        T24  = _mm_madd_epi16(T14, _mm_load_si128((__m128i*)tab_dct_16x16_1[13]));
        T25  = _mm_madd_epi16(T15, _mm_load_si128((__m128i*)tab_dct_16x16_1[13]));
        T26  = _mm_madd_epi16(T16, _mm_load_si128((__m128i*)tab_dct_16x16_1[13]));
        T27  = _mm_madd_epi16(T17, _mm_load_si128((__m128i*)tab_dct_16x16_1[13]));

        T30  = _mm_sub_epi32(T20, T21); 
        T31  = _mm_sub_epi32(T22, T23);
        T32  = _mm_sub_epi32(T24, T25);
        T33  = _mm_sub_epi32(T26, T27);

        T30  = _mm_hadd_epi32(T30, T31);
        T31  = _mm_hadd_epi32(T32, T33);

        T40  = _mm_hadd_epi32(T30, T31);
        T40  = _mm_srai_epi32(_mm_add_epi32(T40, c_512), 10);
        T60  = _mm_packs_epi32(T40, T40);
        _mm_storel_epi64((__m128i*)&pDst[14*nStride+i], T60);


#define MAKE_ODD(tab, dst) \
        T20  = _mm_madd_epi16(T10, _mm_load_si128((__m128i*)tab_dct_16x16_1[(tab)  ])); /* [*O2_0 *O1_0 *O3_0 *O0_0] */ \
        T21  = _mm_madd_epi16(T11, _mm_load_si128((__m128i*)tab_dct_16x16_1[(tab)+1])); /* [*O5_0 *O6_0 *O4_0 *O7_0] */ \
        T22  = _mm_madd_epi16(T12, _mm_load_si128((__m128i*)tab_dct_16x16_1[(tab)  ])); \
        T23  = _mm_madd_epi16(T13, _mm_load_si128((__m128i*)tab_dct_16x16_1[(tab)+1])); \
        T24  = _mm_madd_epi16(T14, _mm_load_si128((__m128i*)tab_dct_16x16_1[(tab)  ])); \
        T25  = _mm_madd_epi16(T15, _mm_load_si128((__m128i*)tab_dct_16x16_1[(tab)+1])); \
        T26  = _mm_madd_epi16(T16, _mm_load_si128((__m128i*)tab_dct_16x16_1[(tab)  ])); \
        T27  = _mm_madd_epi16(T17, _mm_load_si128((__m128i*)tab_dct_16x16_1[(tab)+1])); \
        \
        T30  = _mm_add_epi32(T20, T21); \
        T31  = _mm_add_epi32(T22, T23); \
        T32  = _mm_add_epi32(T24, T25); \
        T33  = _mm_add_epi32(T26, T27); \
        \
        T30  = _mm_hadd_epi32(T30, T31); \
        T31  = _mm_hadd_epi32(T32, T33); \
        \
        T40  = _mm_hadd_epi32(T30, T31); \
        T40  = _mm_srai_epi32(_mm_add_epi32(T40, c_512), 10); \
        T60  = _mm_packs_epi32(T40, T40); \
        _mm_storel_epi64((__m128i*)&pDst[(dst)*nStride+i], T60);

        MAKE_ODD(14,  1);
        MAKE_ODD(16,  3);
        MAKE_ODD(18,  5);
        MAKE_ODD(20,  7);
        MAKE_ODD(22,  9);
        MAKE_ODD(24, 11);
        MAKE_ODD(26, 13);
        MAKE_ODD(28, 15);
#undef MAKE_ODD
    }
}
#endif // INSTRSET >= 4

ALIGN_VAR_32(static const short, tab_idct_8x8[12][8] )=
{
    {  89,  75,  89,  75, 89,  75, 89,  75 },
    {  50,  18,  50,  18, 50,  18, 50,  18 },
    {  75, -18,  75, -18, 75, -18, 75, -18 },
    { -89, -50, -89, -50,-89, -50,-89, -50 },
    {  50, -89,  50, -89, 50, -89, 50, -89 },
    {  18,  75,  18,  75, 18,  75, 18,  75 },
    {  18, -50,  18, -50, 18, -50, 18, -50 },
    {  75, -89,  75, -89, 75, -89, 75, -89 },
    {  64,  64,  64,  64, 64,  64, 64,  64 },
    {  64, -64,  64, -64, 64, -64, 64, -64 },
    {  83,  36,  83,  36, 83,  36, 83,  36 },
    {  36, -83,  36, -83, 36, -83, 36, -83 }
};
void xIDCT8(short *pSrc, short *pDst, intptr_t stride)
{
    __m128i m128iS0, m128iS1, m128iS2, m128iS3, m128iS4, m128iS5, m128iS6, m128iS7, m128iAdd, m128Tmp0,     m128Tmp1,m128Tmp2, m128Tmp3, E0h, E1h, E2h, E3h, E0l, E1l, E2l, E3l, O0h, O1h, O2h, O3h, O0l, O1l, O2l, O3l, EE0l, EE1l, E00l, E01l, EE0h, EE1h, E00h, E01h;
    m128iAdd  = _mm_set1_epi32( 64 );

    m128iS1   = _mm_load_si128( (__m128i*)( pSrc + 8   ) );
    m128iS3   = _mm_load_si128( (__m128i*)( pSrc + 24 ) );
    m128Tmp0 = _mm_unpacklo_epi16(  m128iS1, m128iS3 );
    E1l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_8x8[0] ) ) );
    m128Tmp1 = _mm_unpackhi_epi16(  m128iS1, m128iS3 );
    E1h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_8x8[0] ) ) );
    m128iS5   = _mm_load_si128( (__m128i*)( pSrc + 40   ) );
    m128iS7   = _mm_load_si128( (__m128i*)( pSrc + 56 ) );
    m128Tmp2 =  _mm_unpacklo_epi16(  m128iS5, m128iS7 );
    E2l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_8x8[1] ) ) );
    m128Tmp3 = _mm_unpackhi_epi16(  m128iS5, m128iS7 );
    E2h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_8x8[1] ) ) );
    O0l = _mm_add_epi32(E1l, E2l);
    O0h = _mm_add_epi32(E1h, E2h);

    E1l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_8x8[2] ) ) );
    E1h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_8x8[2] ) ) );
    E2l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_8x8[3] ) ) );
    E2h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_8x8[3] ) ) );

    O1l = _mm_add_epi32(E1l, E2l);
    O1h = _mm_add_epi32(E1h, E2h);

    E1l =  _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_8x8[4] ) ) );
    E1h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_8x8[4] ) ) );
    E2l =  _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_8x8[5] ) ) );
    E2h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_8x8[5] ) ) );
    O2l = _mm_add_epi32(E1l, E2l);
    O2h = _mm_add_epi32(E1h, E2h);

    E1l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_8x8[6] ) ) );
    E1h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_8x8[6] ) ) );
    E2l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_8x8[7] ) ) );
    E2h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_8x8[7] ) ) );
    O3h = _mm_add_epi32(E1h, E2h);
    O3l = _mm_add_epi32(E1l, E2l);

    /*    -------     */

    m128iS0   = _mm_load_si128( (__m128i*)( pSrc + 0   ) );
    m128iS4   = _mm_load_si128( (__m128i*)( pSrc + 32   ) );
    m128Tmp0 = _mm_unpacklo_epi16(  m128iS0, m128iS4 );
    EE0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_8x8[8] ) ) );
    m128Tmp1 = _mm_unpackhi_epi16(  m128iS0, m128iS4 );
    EE0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_8x8[8] ) ) );

    EE1l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_8x8[9] ) ) );
    EE1h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_8x8[9] ) ) );


    /*    -------     */

    m128iS2   = _mm_load_si128( (__m128i*)( pSrc  +16) );
    m128iS6   = _mm_load_si128( (__m128i*)( pSrc + 48   ) );
    m128Tmp0 = _mm_unpacklo_epi16(  m128iS2, m128iS6 );
    E00l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_8x8[10] ) ) );
    m128Tmp1 = _mm_unpackhi_epi16(  m128iS2, m128iS6 );
    E00h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_8x8[10] ) ) );
    E01l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_8x8[11] ) ) );
    E01h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_8x8[11] ) ) );
    E0l = _mm_add_epi32(EE0l , E00l);
    E0l = _mm_add_epi32(E0l, m128iAdd);
    E0h = _mm_add_epi32(EE0h , E00h);
    E0h = _mm_add_epi32(E0h, m128iAdd);
    E3l = _mm_sub_epi32(EE0l , E00l);
    E3l = _mm_add_epi32(E3l , m128iAdd);
    E3h = _mm_sub_epi32(EE0h , E00h);
    E3h = _mm_add_epi32(E3h , m128iAdd);

    E1l = _mm_add_epi32(EE1l , E01l);
    E1l = _mm_add_epi32(E1l , m128iAdd);
    E1h = _mm_add_epi32(EE1h , E01h);
    E1h = _mm_add_epi32(E1h , m128iAdd);
    E2l = _mm_sub_epi32(EE1l , E01l);
    E2l = _mm_add_epi32(E2l , m128iAdd);
    E2h = _mm_sub_epi32(EE1h , E01h);
    E2h = _mm_add_epi32(E2h , m128iAdd);
    m128iS0 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E0l, O0l),7), _mm_srai_epi32(_mm_add_epi32(E0h, O0h), 7));
    m128iS1 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E1l, O1l),7), _mm_srai_epi32(_mm_add_epi32(E1h, O1h), 7));
    m128iS2 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E2l, O2l),7), _mm_srai_epi32(_mm_add_epi32(E2h, O2h), 7));
    m128iS3 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E3l, O3l),7), _mm_srai_epi32(_mm_add_epi32(E3h, O3h), 7));
    m128iS4 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E3l, O3l),7), _mm_srai_epi32(_mm_sub_epi32(E3h, O3h), 7));
    m128iS5 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E2l, O2l),7), _mm_srai_epi32(_mm_sub_epi32(E2h, O2h), 7));
    m128iS6 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E1l, O1l),7), _mm_srai_epi32(_mm_sub_epi32(E1h, O1h), 7));
    m128iS7 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E0l, O0l),7), _mm_srai_epi32(_mm_sub_epi32(E0h, O0h), 7));
    /*  Invers matrix   */

    E0l = _mm_unpacklo_epi16(m128iS0, m128iS4);
    E1l = _mm_unpacklo_epi16(m128iS1, m128iS5);
    E2l = _mm_unpacklo_epi16(m128iS2, m128iS6);
    E3l = _mm_unpacklo_epi16(m128iS3, m128iS7);
    O0l = _mm_unpackhi_epi16(m128iS0, m128iS4);
    O1l = _mm_unpackhi_epi16(m128iS1, m128iS5);
    O2l = _mm_unpackhi_epi16(m128iS2, m128iS6);
    O3l = _mm_unpackhi_epi16(m128iS3, m128iS7);
    m128Tmp0 = _mm_unpacklo_epi16(E0l, E2l);
    m128Tmp1 = _mm_unpacklo_epi16(E1l, E3l);
    m128iS0  = _mm_unpacklo_epi16(m128Tmp0, m128Tmp1);
    m128iS1  = _mm_unpackhi_epi16(m128Tmp0, m128Tmp1);
    m128Tmp2 = _mm_unpackhi_epi16(E0l, E2l);
    m128Tmp3 = _mm_unpackhi_epi16(E1l, E3l);
    m128iS2 = _mm_unpacklo_epi16(m128Tmp2, m128Tmp3);
    m128iS3 = _mm_unpackhi_epi16(m128Tmp2, m128Tmp3);
    m128Tmp0 = _mm_unpacklo_epi16(O0l, O2l);
    m128Tmp1 = _mm_unpacklo_epi16(O1l, O3l);
    m128iS4  = _mm_unpacklo_epi16(m128Tmp0, m128Tmp1);
    m128iS5  = _mm_unpackhi_epi16(m128Tmp0, m128Tmp1);
    m128Tmp2 = _mm_unpackhi_epi16(O0l, O2l);
    m128Tmp3 = _mm_unpackhi_epi16(O1l, O3l);
    m128iS6 = _mm_unpacklo_epi16(m128Tmp2, m128Tmp3);
    m128iS7 = _mm_unpackhi_epi16(m128Tmp2, m128Tmp3);

    m128iAdd  = _mm_set1_epi32( 2048 );


    m128Tmp0 = _mm_unpacklo_epi16(  m128iS1, m128iS3 );
    E1l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_8x8[0] ) ) );
    m128Tmp1 = _mm_unpackhi_epi16(  m128iS1, m128iS3 );
    E1h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_8x8[0] ) ) );
    m128Tmp2 =  _mm_unpacklo_epi16(  m128iS5, m128iS7 );
    E2l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_8x8[1] ) ) );
    m128Tmp3 = _mm_unpackhi_epi16(  m128iS5, m128iS7 );
    E2h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_8x8[1] ) ) );
    O0l = _mm_add_epi32(E1l, E2l);
    O0h = _mm_add_epi32(E1h, E2h);
    E1l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_8x8[2] ) ) );
    E1h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_8x8[2] ) ) );
    E2l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_8x8[3] ) ) );
    E2h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_8x8[3] ) ) );
    O1l = _mm_add_epi32(E1l, E2l);
    O1h = _mm_add_epi32(E1h, E2h);
    E1l =  _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_8x8[4] ) ) );
    E1h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_8x8[4] ) ) );
    E2l =  _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_8x8[5] ) ) );
    E2h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_8x8[5] ) ) );
    O2l = _mm_add_epi32(E1l, E2l);
    O2h = _mm_add_epi32(E1h, E2h);
    E1l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_8x8[6] ) ) );
    E1h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_8x8[6] ) ) );
    E2l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_8x8[7] ) ) );
    E2h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_8x8[7] ) ) );
    O3h = _mm_add_epi32(E1h, E2h);
    O3l = _mm_add_epi32(E1l, E2l);

    m128Tmp0 = _mm_unpacklo_epi16(  m128iS0, m128iS4 );
    EE0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_8x8[8] ) ) );
    m128Tmp1 = _mm_unpackhi_epi16(  m128iS0, m128iS4 );
    EE0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_8x8[8] ) ) );
    EE1l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_8x8[9] ) ) );
    EE1h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_8x8[9] ) ) );

    m128Tmp0 = _mm_unpacklo_epi16(  m128iS2, m128iS6 );
    E00l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_8x8[10] ) ) );
    m128Tmp1 = _mm_unpackhi_epi16(  m128iS2, m128iS6 );
    E00h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_8x8[10] ) ) );
    E01l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_8x8[11] ) ) );
    E01h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_8x8[11] ) ) );
    E0l = _mm_add_epi32(EE0l , E00l);
    E0l = _mm_add_epi32(E0l, m128iAdd);
    E0h = _mm_add_epi32(EE0h , E00h);
    E0h = _mm_add_epi32(E0h, m128iAdd);
    E3l = _mm_sub_epi32(EE0l , E00l);
    E3l = _mm_add_epi32(E3l , m128iAdd);
    E3h = _mm_sub_epi32(EE0h , E00h);
    E3h = _mm_add_epi32(E3h , m128iAdd);
    E1l = _mm_add_epi32(EE1l , E01l);
    E1l = _mm_add_epi32(E1l , m128iAdd);
    E1h = _mm_add_epi32(EE1h , E01h);
    E1h = _mm_add_epi32(E1h , m128iAdd);
    E2l = _mm_sub_epi32(EE1l , E01l);
    E2l = _mm_add_epi32(E2l , m128iAdd);
    E2h = _mm_sub_epi32(EE1h , E01h);
    E2h = _mm_add_epi32(E2h , m128iAdd);

    m128iS0 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E0l, O0l),12), _mm_srai_epi32(_mm_add_epi32(E0h, O0h), 12));
    m128iS1 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E1l, O1l),12), _mm_srai_epi32(_mm_add_epi32(E1h, O1h), 12));
    m128iS2 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E2l, O2l),12), _mm_srai_epi32(_mm_add_epi32(E2h, O2h), 12));
    m128iS3 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E3l, O3l),12), _mm_srai_epi32(_mm_add_epi32(E3h, O3h), 12));
    m128iS4 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E3l, O3l),12), _mm_srai_epi32(_mm_sub_epi32(E3h, O3h), 12));
    m128iS5 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E2l, O2l),12), _mm_srai_epi32(_mm_sub_epi32(E2h, O2h), 12));
    m128iS6 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E1l, O1l),12), _mm_srai_epi32(_mm_sub_epi32(E1h, O1h), 12));
    m128iS7 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E0l, O0l),12), _mm_srai_epi32(_mm_sub_epi32(E0h, O0h), 12));

    // [07 06 05 04 03 02 01 00]
    // [17 16 15 14 13 12 11 10]
    // [27 26 25 24 23 22 21 20]
    // [37 36 35 34 33 32 31 30]
    // [47 46 45 44 43 42 41 40]
    // [57 56 55 54 53 52 51 50]
    // [67 66 65 64 63 62 61 60]
    // [77 76 75 74 73 72 71 70]

    __m128i T00 = _mm_unpacklo_epi16(m128iS0, m128iS1);     // [13 03 12 02 11 01 10 00]
    __m128i T01 = _mm_unpackhi_epi16(m128iS0, m128iS1);     // [17 07 16 06 15 05 14 04]
    __m128i T02 = _mm_unpacklo_epi16(m128iS2, m128iS3);     // [33 23 32 22 31 21 30 20]
    __m128i T03 = _mm_unpackhi_epi16(m128iS2, m128iS3);     // [37 27 36 26 35 25 34 24]
    __m128i T04 = _mm_unpacklo_epi16(m128iS4, m128iS5);     // [53 43 52 42 51 41 50 40]
    __m128i T05 = _mm_unpackhi_epi16(m128iS4, m128iS5);     // [57 47 56 46 55 45 54 44]
    __m128i T06 = _mm_unpacklo_epi16(m128iS6, m128iS7);     // [73 63 72 62 71 61 70 60]
    __m128i T07 = _mm_unpackhi_epi16(m128iS6, m128iS7);     // [77 67 76 66 75 65 74 64]

    __m128i T10, T11;
    T10 = _mm_unpacklo_epi32(T00, T02);                                     // [31 21 11 01 30 20 10 00]
    T11 = _mm_unpackhi_epi32(T00, T02);                                     // [33 23 13 03 32 22 12 02]
    _mm_storel_epi64( (__m128i*)&pDst[0 * stride +  0], T10 );                   // [30 20 10 00]
    _mm_storeh_pi   ( (__m64*  )&pDst[1 * stride +  0], _mm_castsi128_ps(T10));  // [31 21 11 01]
    _mm_storel_epi64( (__m128i*)&pDst[2 * stride +  0], T11 );                   // [32 22 12 02]
    _mm_storeh_pi   ( (__m64*  )&pDst[3 * stride +  0], _mm_castsi128_ps(T11));  // [33 23 13 03]

    T10 = _mm_unpacklo_epi32(T04, T06);                                     // [71 61 51 41 70 60 50 40]
    T11 = _mm_unpackhi_epi32(T04, T06);                                     // [73 63 53 43 72 62 52 42]
    _mm_storel_epi64( (__m128i*)&pDst[0 * stride +  4], T10 );
    _mm_storeh_pi   ( (__m64*  )&pDst[1 * stride +  4], _mm_castsi128_ps(T10));
    _mm_storel_epi64( (__m128i*)&pDst[2 * stride +  4], T11 );
    _mm_storeh_pi   ( (__m64*  )&pDst[3 * stride +  4], _mm_castsi128_ps(T11));

    T10 = _mm_unpacklo_epi32(T01, T03);                                     // [35 25 15 05 34 24 14 04]
    T11 = _mm_unpackhi_epi32(T01, T03);                                     // [37 27 17 07 36 26 16 06]
    _mm_storel_epi64( (__m128i*)&pDst[4 * stride +  0], T10 );
    _mm_storeh_pi   ( (__m64*  )&pDst[5 * stride +  0], _mm_castsi128_ps(T10));
    _mm_storel_epi64( (__m128i*)&pDst[6 * stride +  0], T11 );
    _mm_storeh_pi   ( (__m64*  )&pDst[7 * stride +  0], _mm_castsi128_ps(T11));

    T10 = _mm_unpacklo_epi32(T05, T07);                                     // [75 65 55 45 74 64 54 44]
    T11 = _mm_unpackhi_epi32(T05, T07);                                     // [77 67 57 47 76 56 46 36]
    _mm_storel_epi64( (__m128i*)&pDst[4 * stride +  4], T10 );
    _mm_storeh_pi   ( (__m64*  )&pDst[5 * stride +  4], _mm_castsi128_ps(T10));
    _mm_storel_epi64( (__m128i*)&pDst[6 * stride +  4], T11 );
    _mm_storeh_pi   ( (__m64*  )&pDst[7 * stride +  4], _mm_castsi128_ps(T11));
}

ALIGN_VAR_32(static const int16_t, tab_idct_16x16_1[4][8][8] )=
{
    {/*1-3*/ /*2-6*/
        { 90,  87,  90,  87,  90,  87,  90,  87 },
        { 87,  57,  87,  57,  87,  57,  87,  57 },
        { 80,   9,  80,   9,  80,   9,  80,   9 },
        { 70, -43,  70, -43,  70, -43,  70, -43 },
        { 57, -80,  57, -80,  57, -80,  57, -80 },
        { 43, -90,  43, -90,  43, -90,  43, -90 },
        { 25, -70,  25, -70,  25, -70,  25, -70 },
        { 9,  -25,   9, -25,   9, -25,   9, -25 },
    },{ /*5-7*/ /*10-14*/
        {  80,  70,  80,  70,  80,  70,  80,  70 },
        {   9, -43,   9, -43,   9, -43,   9, -43 },
        { -70, -87, -70, -87, -70, -87, -70, -87 },
        { -87,   9, -87,   9, -87,   9, -87,   9 },
        { -25,  90, -25,  90, -25,  90, -25,  90 },
        {  57,  25,  57,  25,  57,  25,  57,  25 },
        {  90, -80,  90, -80,  90, -80,  90, -80 },
        {  43, -57,  43, -57,  43, -57,  43, -57 },
    },{ /*9-11*/ /*18-22*/
        {  57,  43,  57,  43,  57,  43,  57,  43 },
        { -80, -90, -80, -90, -80, -90, -80, -90 },
        { -25,  57, -25,  57, -25,  57, -25,  57 },
        {  90,  25,  90,  25,  90,  25,  90,  25 },
        {  -9,  -87, -9,  -87, -9,  -87, -9, -87 },
        { -87,  70, -87,  70, -87,  70, -87,  70 },
        {  43,   9,  43,   9,  43,   9,  43,   9 },
        {  70, -80,  70, -80,  70, -80,  70, -80 },
    },{/*13-15*/ /*  26-30   */
        {  25,   9,  25,   9,  25,   9,  25,   9 },
        { -70, -25, -70, -25, -70, -25, -70, -25 },
        {  90,  43,  90,  43,  90,  43,  90,  43 },
        { -80, -57, -80, -57, -80, -57, -80, -57 },
        {  43,  70,  43,  70,  43,  70,  43,  70 },
        {  9,  -80,   9, -80,   9, -80,   9, -80 },
        { -57,  87, -57,  87, -57,  87, -57,  87 },
        {  87, -90,  87, -90,  87, -90,  87, -90 },
    }
};

ALIGN_VAR_32(static const int16_t, tab_idct_16x16_2[2][4][8] )=
{
    { /*2-6*/ /*4-12*/
        { 89,  75,  89,  75, 89,  75, 89,  75 },
        { 75, -18,  75, -18, 75, -18, 75, -18 },
        { 50, -89,  50, -89, 50, -89, 50, -89 },
        { 18, -50,  18, -50, 18, -50, 18, -50 },
    },{ /*10-14*/  /*20-28*/
        {  50,  18,  50,  18,  50,  18,  50,  18 },
        { -89, -50, -89, -50, -89, -50, -89, -50 },
        {  18,  75,  18,  75,  18,  75,  18,  75 },
        {  75, -89,  75, -89,  75, -89,  75, -89 },
    }
};

ALIGN_VAR_32(static const int16_t, tab_idct_16x16_3[2][2][8] )=
{
    {/*4-12*/ /*8-24*/
        {  83,  36,  83,  36,  83,  36,  83,  36 },
        {  36, -83,  36, -83,  36, -83,  36, -83 },
    },{ /*0-8*/  /*0-16*/
        { 64,  64, 64,  64, 64,  64, 64,  64 },
        { 64, -64, 64, -64, 64, -64, 64, -64 },
    }
};

void xIDCT16(short *pSrc, short *pDst, intptr_t stride)
{
    int i;
    int j;
    int32_t shift;
    __m128i m128iS0, m128iS1, m128iS2, m128iS3, m128iS4, m128iS5, m128iS6, m128iS7, m128iS8, m128iS9, m128iS10, m128iS11, m128iS12, m128iS13, m128iS14, m128iS15 ,  m128iAdd, m128Tmp0,     m128Tmp1,m128Tmp2, m128Tmp3, m128Tmp4, m128Tmp5,m128Tmp6, m128Tmp7, E0h, E1h, E2h, E3h, E0l, E1l, E2l, E3l, O0h, O1h, O2h, O3h, O4h, O5h, O6h, O7h,O0l, O1l, O2l, O3l, O4l, O5l, O6l, O7l,EE0l, EE1l, EE2l, EE3l, E00l, E01l, EE0h, EE1h, EE2h, EE3h,E00h, E01h;
    __m128i E4l, E5l, E6l, E7l;
    __m128i E4h, E5h, E6h, E7h;
    m128iS0   = _mm_load_si128( (__m128i*)( pSrc       ) );
    m128iS1   = _mm_load_si128( (__m128i*)( pSrc +  16 ) );
    m128iS2   = _mm_load_si128( (__m128i*)( pSrc +  32 ) );
    m128iS3   = _mm_load_si128( (__m128i*)( pSrc +  48 ) );
    m128iS4   = _mm_loadu_si128((__m128i*)( pSrc +  64 ) );
    m128iS5   = _mm_load_si128( (__m128i*)( pSrc +  80 ) );
    m128iS6   = _mm_load_si128( (__m128i*)( pSrc +  96 ) );
    m128iS7   = _mm_load_si128( (__m128i*)( pSrc + 112 ) );
    m128iS8   = _mm_load_si128( (__m128i*)( pSrc + 128 ) );
    m128iS9   = _mm_load_si128( (__m128i*)( pSrc + 144 ) );
    m128iS10  = _mm_load_si128( (__m128i*)( pSrc + 160 ) );
    m128iS11  = _mm_load_si128( (__m128i*)( pSrc + 176 ) );
    m128iS12  = _mm_loadu_si128((__m128i*)( pSrc + 192 ) );
    m128iS13  = _mm_load_si128( (__m128i*)( pSrc + 208 ) );
    m128iS14  = _mm_load_si128( (__m128i*)( pSrc + 224 ) );
    m128iS15  = _mm_load_si128( (__m128i*)( pSrc + 240 ) );
    shift = 7;
    m128iAdd  = _mm_set1_epi32( 64 );

    for(j=0; j< 2; j++) {
        for(i=0; i < 16; i+=8) {

            m128Tmp0 = _mm_unpacklo_epi16(  m128iS1, m128iS3 );
            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][0] ) ) );
            m128Tmp1 = _mm_unpackhi_epi16(  m128iS1, m128iS3 );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][0] ) ) );


            m128Tmp2 =  _mm_unpacklo_epi16(  m128iS5, m128iS7 );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][0] ) ) );
            m128Tmp3 = _mm_unpackhi_epi16(  m128iS5, m128iS7 );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][0] ) ) );


            m128Tmp4 =  _mm_unpacklo_epi16(  m128iS9, m128iS11 );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][0] ) ) );
            m128Tmp5 = _mm_unpackhi_epi16(  m128iS9, m128iS11 );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][0] ) ) );


            m128Tmp6 =  _mm_unpacklo_epi16(  m128iS13, m128iS15 );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][0] ) ) );
            m128Tmp7 = _mm_unpackhi_epi16(  m128iS13, m128iS15 );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][0] ) ) );

            O0l = _mm_add_epi32(E0l, E1l);
            O0l = _mm_add_epi32(O0l, E2l);
            O0l = _mm_add_epi32(O0l, E3l);

            O0h = _mm_add_epi32(E0h, E1h);
            O0h = _mm_add_epi32(O0h, E2h);
            O0h = _mm_add_epi32(O0h, E3h);

            /* Compute O1*/
            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][1] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][1] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][1] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][1] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][1] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][1] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][1] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][1] ) ) );
            O1l = _mm_add_epi32(E0l, E1l);
            O1l = _mm_add_epi32(O1l, E2l);
            O1l = _mm_add_epi32(O1l, E3l);
            O1h = _mm_add_epi32(E0h, E1h);
            O1h = _mm_add_epi32(O1h, E2h);
            O1h = _mm_add_epi32(O1h, E3h);

            /* Compute O2*/
            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][2] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][2] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][2] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][2] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][2] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][2] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][2] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][2] ) ) );
            O2l = _mm_add_epi32(E0l, E1l);
            O2l = _mm_add_epi32(O2l, E2l);
            O2l = _mm_add_epi32(O2l, E3l);

            O2h = _mm_add_epi32(E0h, E1h);
            O2h = _mm_add_epi32(O2h, E2h);
            O2h = _mm_add_epi32(O2h, E3h);

            /* Compute O3*/
            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][3] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][3] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][3] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][3] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][3] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][3] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][3] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][3] ) ) );

            O3l = _mm_add_epi32(E0l, E1l);
            O3l = _mm_add_epi32(O3l, E2l);
            O3l = _mm_add_epi32(O3l, E3l);

            O3h = _mm_add_epi32(E0h, E1h);
            O3h = _mm_add_epi32(O3h, E2h);
            O3h = _mm_add_epi32(O3h, E3h);

            /* Compute O4*/

            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][4] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][4] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][4] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][4] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][4] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][4] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][4] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][4] ) ) );

            O4l = _mm_add_epi32(E0l, E1l);
            O4l = _mm_add_epi32(O4l, E2l);
            O4l = _mm_add_epi32(O4l, E3l);

            O4h = _mm_add_epi32(E0h, E1h);
            O4h = _mm_add_epi32(O4h, E2h);
            O4h = _mm_add_epi32(O4h, E3h);

            /* Compute O5*/
            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][5] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][5] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][5] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][5] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][5] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][5] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][5] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][5] ) ) );

            O5l = _mm_add_epi32(E0l, E1l);
            O5l = _mm_add_epi32(O5l, E2l);
            O5l = _mm_add_epi32(O5l, E3l);

            O5h = _mm_add_epi32(E0h, E1h);
            O5h = _mm_add_epi32(O5h, E2h);
            O5h = _mm_add_epi32(O5h, E3h);

            /* Compute O6*/

            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][6] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][6] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][6] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][6] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][6] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][6] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][6] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][6] ) ) );

            O6l = _mm_add_epi32(E0l, E1l);
            O6l = _mm_add_epi32(O6l, E2l);
            O6l = _mm_add_epi32(O6l, E3l);

            O6h = _mm_add_epi32(E0h, E1h);
            O6h = _mm_add_epi32(O6h, E2h);
            O6h = _mm_add_epi32(O6h, E3h);

            /* Compute O7*/

            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][7] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][7] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][7] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][7] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][7] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][7] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][7] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][7] ) ) );

            O7l = _mm_add_epi32(E0l, E1l);
            O7l = _mm_add_epi32(O7l, E2l);
            O7l = _mm_add_epi32(O7l, E3l);

            O7h = _mm_add_epi32(E0h, E1h);
            O7h = _mm_add_epi32(O7h, E2h);
            O7h = _mm_add_epi32(O7h, E3h);

            /*  Compute E0  */

            m128Tmp0 = _mm_unpacklo_epi16(  m128iS2, m128iS6 );
            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[0][0] ) ) );
            m128Tmp1 = _mm_unpackhi_epi16(  m128iS2, m128iS6 );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[0][0] ) ) );


            m128Tmp2 =  _mm_unpacklo_epi16(  m128iS10, m128iS14 );
            E0l = _mm_add_epi32(E0l, _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[1][0] ) ) ));
            m128Tmp3 = _mm_unpackhi_epi16(  m128iS10, m128iS14 );
            E0h = _mm_add_epi32(E0h, _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[1][0] ) ) ));

            /*  Compute E1  */
            E1l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[0][1] ) ));
            E1h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[0][1] ) ) );
            E1l = _mm_add_epi32(E1l,_mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[1][1] ) ) ));
            E1h = _mm_add_epi32(E1h,_mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[1][1] ) ) ));


            /*  Compute E2  */
            E2l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[0][2] ) ) );
            E2h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[0][2] ) ) );
            E2l = _mm_add_epi32(E2l,_mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[1][2] ) ) ));
            E2h = _mm_add_epi32(E2h,_mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[1][2] ) ) ));
            /*  Compute E3  */
            E3l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[0][3] ) ) );
            E3h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[0][3] ) ) );
            E3l = _mm_add_epi32(E3l,_mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[1][3] ) ) ));
            E3h = _mm_add_epi32(E3h,_mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[1][3] ) ) ));

            /*  Compute EE0 and EEE */

            m128Tmp0 = _mm_unpacklo_epi16(  m128iS4, m128iS12 );
            E00l =  _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_3[0][0] ) ) );
            m128Tmp1 = _mm_unpackhi_epi16(  m128iS4, m128iS12 );
            E00h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_3[0][0] ) ) );

            m128Tmp2 =  _mm_unpacklo_epi16(  m128iS0, m128iS8 );
            EE0l =  _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_3[1][0] ) ) );
            m128Tmp3 = _mm_unpackhi_epi16(  m128iS0, m128iS8 );
            EE0h =  _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_3[1][0] ) ) );


            E01l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_3[0][1] ) ) );
            E01h  = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_3[0][1] ) ) );

            EE1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_3[1][1] ) ) );
            EE1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_3[1][1] ) ) );

            /*  Compute EE    */
            EE2l = _mm_sub_epi32(EE1l,E01l);
            EE3l = _mm_sub_epi32(EE0l,E00l);
            EE2h = _mm_sub_epi32(EE1h,E01h);
            EE3h = _mm_sub_epi32(EE0h,E00h);

            EE0l = _mm_add_epi32(EE0l,E00l);
            EE1l = _mm_add_epi32(EE1l,E01l);
            EE0h = _mm_add_epi32(EE0h,E00h);
            EE1h = _mm_add_epi32(EE1h,E01h);

            /*      Compute E       */

            E4l = _mm_sub_epi32(EE3l,E3l);
            E4l = _mm_add_epi32(E4l, m128iAdd);

            E5l = _mm_sub_epi32(EE2l,E2l);
            E5l = _mm_add_epi32(E5l, m128iAdd);

            E6l = _mm_sub_epi32(EE1l,E1l);
            E6l = _mm_add_epi32(E6l, m128iAdd);

            E7l = _mm_sub_epi32(EE0l,E0l);
            E7l = _mm_add_epi32(E7l, m128iAdd);

            E4h = _mm_sub_epi32(EE3h,E3h);
            E4h = _mm_add_epi32(E4h, m128iAdd);

            E5h = _mm_sub_epi32(EE2h,E2h);
            E5h = _mm_add_epi32(E5h, m128iAdd);

            E6h = _mm_sub_epi32(EE1h,E1h);
            E6h = _mm_add_epi32(E6h, m128iAdd);

            E7h = _mm_sub_epi32(EE0h,E0h);
            E7h = _mm_add_epi32(E7h, m128iAdd);

            E0l = _mm_add_epi32(EE0l,E0l);
            E0l = _mm_add_epi32(E0l, m128iAdd);

            E1l = _mm_add_epi32(EE1l,E1l);
            E1l = _mm_add_epi32(E1l, m128iAdd);

            E2l = _mm_add_epi32(EE2l,E2l);
            E2l = _mm_add_epi32(E2l, m128iAdd);

            E3l = _mm_add_epi32(EE3l,E3l);
            E3l = _mm_add_epi32(E3l, m128iAdd);

            E0h = _mm_add_epi32(EE0h,E0h);
            E0h = _mm_add_epi32(E0h, m128iAdd);

            E1h = _mm_add_epi32(EE1h,E1h);
            E1h = _mm_add_epi32(E1h, m128iAdd);

            E2h = _mm_add_epi32(EE2h,E2h);
            E2h = _mm_add_epi32(E2h, m128iAdd);

            E3h = _mm_add_epi32(EE3h,E3h);
            E3h = _mm_add_epi32(E3h, m128iAdd);

            m128iS0 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E0l, O0l),shift), _mm_srai_epi32(_mm_add_epi32(E0h, O0h), shift));
            m128iS1 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E1l, O1l),shift), _mm_srai_epi32(_mm_add_epi32(E1h, O1h), shift));
            m128iS2 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E2l, O2l),shift), _mm_srai_epi32(_mm_add_epi32(E2h, O2h), shift));
            m128iS3 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E3l, O3l),shift), _mm_srai_epi32(_mm_add_epi32(E3h, O3h), shift));

            m128iS4 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E4l, O4l),shift), _mm_srai_epi32(_mm_add_epi32(E4h, O4h), shift));
            m128iS5 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E5l, O5l),shift), _mm_srai_epi32(_mm_add_epi32(E5h, O5h), shift));
            m128iS6 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E6l, O6l),shift), _mm_srai_epi32(_mm_add_epi32(E6h, O6h), shift));
            m128iS7 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E7l, O7l),shift), _mm_srai_epi32(_mm_add_epi32(E7h, O7h), shift));

            m128iS15 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E0l, O0l),shift), _mm_srai_epi32(_mm_sub_epi32(E0h, O0h), shift));
            m128iS14 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E1l, O1l),shift), _mm_srai_epi32(_mm_sub_epi32(E1h, O1h), shift));
            m128iS13 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E2l, O2l),shift), _mm_srai_epi32(_mm_sub_epi32(E2h, O2h), shift));
            m128iS12 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E3l, O3l),shift), _mm_srai_epi32(_mm_sub_epi32(E3h, O3h), shift));

            m128iS11 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E4l, O4l),shift), _mm_srai_epi32(_mm_sub_epi32(E4h, O4h), shift));
            m128iS10 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E5l, O5l),shift), _mm_srai_epi32(_mm_sub_epi32(E5h, O5h), shift));
            m128iS9 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E6l, O6l),shift), _mm_srai_epi32(_mm_sub_epi32(E6h, O6h), shift));
            m128iS8 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E7l, O7l),shift), _mm_srai_epi32(_mm_sub_epi32(E7h, O7h), shift));


            if(!j){
                /*      Inverse the matrix      */
                E0l = _mm_unpacklo_epi16(m128iS0, m128iS8);
                E1l = _mm_unpacklo_epi16(m128iS1, m128iS9);
                E2l = _mm_unpacklo_epi16(m128iS2, m128iS10);
                E3l = _mm_unpacklo_epi16(m128iS3, m128iS11);
                E4l = _mm_unpacklo_epi16(m128iS4, m128iS12);
                E5l = _mm_unpacklo_epi16(m128iS5, m128iS13);
                E6l = _mm_unpacklo_epi16(m128iS6, m128iS14);
                E7l = _mm_unpacklo_epi16(m128iS7, m128iS15);

                O0l = _mm_unpackhi_epi16(m128iS0, m128iS8);
                O1l = _mm_unpackhi_epi16(m128iS1, m128iS9);
                O2l = _mm_unpackhi_epi16(m128iS2, m128iS10);
                O3l = _mm_unpackhi_epi16(m128iS3, m128iS11);
                O4l = _mm_unpackhi_epi16(m128iS4, m128iS12);
                O5l = _mm_unpackhi_epi16(m128iS5, m128iS13);
                O6l = _mm_unpackhi_epi16(m128iS6, m128iS14);
                O7l = _mm_unpackhi_epi16(m128iS7, m128iS15);


                m128Tmp0 = _mm_unpacklo_epi16(E0l, E4l);
                m128Tmp1 = _mm_unpacklo_epi16(E1l, E5l);
                m128Tmp2 = _mm_unpacklo_epi16(E2l, E6l);
                m128Tmp3 = _mm_unpacklo_epi16(E3l, E7l);

                m128Tmp4 = _mm_unpacklo_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpacklo_epi16(m128Tmp1, m128Tmp3);
                m128iS0  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS1  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                m128Tmp4 = _mm_unpackhi_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpackhi_epi16(m128Tmp1, m128Tmp3);
                m128iS2  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS3  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                m128Tmp0 = _mm_unpackhi_epi16(E0l, E4l);
                m128Tmp1 = _mm_unpackhi_epi16(E1l, E5l);
                m128Tmp2 = _mm_unpackhi_epi16(E2l, E6l);
                m128Tmp3 = _mm_unpackhi_epi16(E3l, E7l);

                m128Tmp4 = _mm_unpacklo_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpacklo_epi16(m128Tmp1, m128Tmp3);
                m128iS4  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS5  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                m128Tmp4 = _mm_unpackhi_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpackhi_epi16(m128Tmp1, m128Tmp3);
                m128iS6  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS7  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                m128Tmp0 = _mm_unpacklo_epi16(O0l, O4l);
                m128Tmp1 = _mm_unpacklo_epi16(O1l, O5l);
                m128Tmp2 = _mm_unpacklo_epi16(O2l, O6l);
                m128Tmp3 = _mm_unpacklo_epi16(O3l, O7l);

                m128Tmp4 = _mm_unpacklo_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpacklo_epi16(m128Tmp1, m128Tmp3);
                m128iS8  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS9  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                m128Tmp4 = _mm_unpackhi_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpackhi_epi16(m128Tmp1, m128Tmp3);
                m128iS10  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS11  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                m128Tmp0 = _mm_unpackhi_epi16(O0l, O4l);
                m128Tmp1 = _mm_unpackhi_epi16(O1l, O5l);
                m128Tmp2 = _mm_unpackhi_epi16(O2l, O6l);
                m128Tmp3 = _mm_unpackhi_epi16(O3l, O7l);

                m128Tmp4 = _mm_unpacklo_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpacklo_epi16(m128Tmp1, m128Tmp3);
                m128iS12  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS13  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                m128Tmp4 = _mm_unpackhi_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpackhi_epi16(m128Tmp1, m128Tmp3);
                m128iS14  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS15  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                /*  */
                _mm_store_si128((__m128i*)( pSrc +       i ), m128iS0);
                _mm_store_si128((__m128i*)( pSrc +  16 + i ), m128iS1);
                _mm_store_si128((__m128i*)( pSrc +  32 + i ), m128iS2);
                _mm_store_si128((__m128i*)( pSrc +  48 + i ), m128iS3);
                _mm_store_si128((__m128i*)( pSrc +  64 + i ), m128iS4);
                _mm_store_si128((__m128i*)( pSrc +  80 + i ), m128iS5);
                _mm_store_si128((__m128i*)( pSrc +  96 + i ), m128iS6);
                _mm_store_si128((__m128i*)( pSrc + 112 + i ), m128iS7);
                _mm_store_si128((__m128i*)( pSrc + 128 + i ), m128iS8);
                _mm_store_si128((__m128i*)( pSrc + 144 + i ), m128iS9);
                _mm_store_si128((__m128i*)( pSrc + 160 + i ), m128iS10);
                _mm_store_si128((__m128i*)( pSrc + 176 + i ), m128iS11);
                _mm_store_si128((__m128i*)( pSrc + 192 + i ), m128iS12);
                _mm_store_si128((__m128i*)( pSrc + 208 + i ), m128iS13);
                _mm_store_si128((__m128i*)( pSrc + 224 + i ), m128iS14);
                _mm_store_si128((__m128i*)( pSrc + 240 + i ), m128iS15);

                if(!i) {
                    m128iS0   = _mm_load_si128( (__m128i*)( pSrc +   8) );
                    m128iS1   = _mm_load_si128( (__m128i*)( pSrc +  24) );
                    m128iS2   = _mm_load_si128( (__m128i*)( pSrc +  40) );
                    m128iS3   = _mm_load_si128( (__m128i*)( pSrc +  56) );
                    m128iS4   = _mm_loadu_si128((__m128i*)( pSrc +  72) );
                    m128iS5   = _mm_load_si128( (__m128i*)( pSrc +  88) );
                    m128iS6   = _mm_load_si128( (__m128i*)( pSrc + 104) );
                    m128iS7   = _mm_load_si128( (__m128i*)( pSrc + 120) );
                    m128iS8   = _mm_load_si128( (__m128i*)( pSrc + 136) );
                    m128iS9   = _mm_load_si128( (__m128i*)( pSrc + 152) );
                    m128iS10  = _mm_load_si128( (__m128i*)( pSrc + 168) );
                    m128iS11  = _mm_load_si128( (__m128i*)( pSrc + 184) );
                    m128iS12  = _mm_loadu_si128((__m128i*)( pSrc + 200) );
                    m128iS13  = _mm_load_si128( (__m128i*)( pSrc + 216) );
                    m128iS14  = _mm_load_si128( (__m128i*)( pSrc + 232) );
                    m128iS15  = _mm_load_si128( (__m128i*)( pSrc + 248) );
                } else {
                    m128iS0   = _mm_load_si128( (__m128i*)( pSrc      ) );
                    m128iS1   = _mm_load_si128( (__m128i*)( pSrc +  32) );
                    m128iS2   = _mm_load_si128( (__m128i*)( pSrc +  64) );
                    m128iS3   = _mm_load_si128( (__m128i*)( pSrc +  96) );
                    m128iS4   = _mm_loadu_si128((__m128i*)( pSrc + 128) );
                    m128iS5   = _mm_load_si128( (__m128i*)( pSrc + 160) );
                    m128iS6   = _mm_load_si128( (__m128i*)( pSrc + 192) );
                    m128iS7   = _mm_load_si128( (__m128i*)( pSrc + 224) );
                    m128iS8   = _mm_load_si128( (__m128i*)( pSrc +   8) );
                    m128iS9   = _mm_load_si128( (__m128i*)( pSrc +  32 +8) );
                    m128iS10  = _mm_load_si128( (__m128i*)( pSrc +  64 +8) );
                    m128iS11  = _mm_load_si128( (__m128i*)( pSrc +  96 +8) );
                    m128iS12  = _mm_loadu_si128((__m128i*)( pSrc + 128 +8) );
                    m128iS13  = _mm_load_si128( (__m128i*)( pSrc + 160 +8) );
                    m128iS14  = _mm_load_si128( (__m128i*)( pSrc + 192 +8) );
                    m128iS15  = _mm_load_si128( (__m128i*)( pSrc + 224 +8) );
                    shift = 12;
                    m128iAdd  = _mm_set1_epi32( 2048 );
                }

            } else {
                __m128i T00, T01, T02, T03;
                __m128i T10, T11;

#define STORE_4x8(_COL, A, B, C, D) \
                T00 = _mm_unpacklo_epi16((A), (B)); \
                T01 = _mm_unpackhi_epi16((A), (B)); \
                T02 = _mm_unpacklo_epi16((C), (D)); \
                T03 = _mm_unpackhi_epi16((C), (D)); \
                T10 = _mm_unpacklo_epi32(T00, T02); \
                T11 = _mm_unpackhi_epi32(T00, T02); \
                _mm_storel_epi64( (__m128i*)&pDst[(i+0)*stride + (_COL)], T10 ); \
                _mm_storeh_pi   ( (__m64*  )&pDst[(i+1)*stride + (_COL)], _mm_castsi128_ps(T10)); \
                _mm_storel_epi64( (__m128i*)&pDst[(i+2)*stride + (_COL)], T11 ); \
                _mm_storeh_pi   ( (__m64*  )&pDst[(i+3)*stride + (_COL)], _mm_castsi128_ps(T11)); \
                T10 = _mm_unpacklo_epi32(T01, T03); \
                T11 = _mm_unpackhi_epi32(T01, T03); \
                _mm_storel_epi64( (__m128i*)&pDst[(i+4)*stride + (_COL)], T10 ); \
                _mm_storeh_pi   ( (__m64*  )&pDst[(i+5)*stride + (_COL)], _mm_castsi128_ps(T10)); \
                _mm_storel_epi64( (__m128i*)&pDst[(i+6)*stride + (_COL)], T11 ); \
                _mm_storeh_pi   ( (__m64*  )&pDst[(i+7)*stride + (_COL)], _mm_castsi128_ps(T11)); \


                STORE_4x8( 0, m128iS0,  m128iS1,  m128iS2,  m128iS3);
                STORE_4x8( 4, m128iS4,  m128iS5,  m128iS6,  m128iS7);
                STORE_4x8( 8, m128iS8,  m128iS9,  m128iS10, m128iS11);
                STORE_4x8(12, m128iS12, m128iS13, m128iS14, m128iS15);
#undef STORE_4x8

                if(!i){
                    m128iS0   = _mm_load_si128( (__m128i*) ( pSrc +  16));
                    m128iS1   = _mm_load_si128( (__m128i*) ( pSrc +  48));
                    m128iS2   = _mm_load_si128( (__m128i*) ( pSrc +  80));
                    m128iS3   = _mm_loadu_si128( (__m128i*)( pSrc + 112));
                    m128iS4   = _mm_load_si128( (__m128i*) ( pSrc + 144));
                    m128iS5   = _mm_load_si128( (__m128i*) ( pSrc + 176));
                    m128iS6   = _mm_load_si128( (__m128i*) ( pSrc + 208));
                    m128iS7   = _mm_load_si128( (__m128i*) ( pSrc + 240));
                    m128iS8   = _mm_load_si128( (__m128i*) ( pSrc +  24));
                    m128iS9   = _mm_load_si128( (__m128i*) ( pSrc +  56));
                    m128iS10  = _mm_load_si128( (__m128i*) ( pSrc +  88));
                    m128iS11  = _mm_loadu_si128( (__m128i*)( pSrc + 120));
                    m128iS12  = _mm_load_si128( (__m128i*) ( pSrc + 152));
                    m128iS13  = _mm_load_si128( (__m128i*) ( pSrc + 184));
                    m128iS14  = _mm_load_si128( (__m128i*) ( pSrc + 216));
                    m128iS15  = _mm_load_si128( (__m128i*) ( pSrc + 248));
                }
            }
        }
    }
}

ALIGN_VAR_32(static const short, tab_idct_32x32[8][16][8] )=
{
    { /*   1-3     */
        { 90,  90, 90,  90, 90,  90, 90,  90 },
        { 90,  82, 90,  82, 90,  82, 90,  82 },
        { 88,  67, 88,  67, 88,  67, 88,  67 },
        { 85,  46, 85,  46, 85,  46, 85,  46 },
        { 82,  22, 82,  22, 82,  22, 82,  22 },
        { 78,  -4, 78,  -4, 78,  -4, 78,  -4 },
        { 73, -31, 73, -31, 73, -31, 73, -31 },
        { 67, -54, 67, -54, 67, -54, 67, -54 },
        { 61, -73, 61, -73, 61, -73, 61, -73 },
        { 54, -85, 54, -85, 54, -85, 54, -85 },
        { 46, -90, 46, -90, 46, -90, 46, -90 },
        { 38, -88, 38, -88, 38, -88, 38, -88 },
        { 31, -78, 31, -78, 31, -78, 31, -78 },
        { 22, -61, 22, -61, 22, -61, 22, -61 },
        { 13, -38, 13, -38, 13, -38, 13, -38 },
        { 4,  -13,  4, -13,  4, -13,  4, -13 },
    },{/*  5-7 */
        {  88,  85,  88,  85,  88,  85,  88,  85 },
        {  67,  46,  67,  46,  67,  46,  67,  46 },
        {  31, -13,  31, -13,  31, -13,  31, -13 },
        { -13, -67, -13, -67, -13, -67, -13, -67 },
        { -54, -90, -54, -90, -54, -90, -54, -90 },
        { -82, -73, -82, -73, -82, -73, -82, -73 },
        { -90, -22, -90, -22, -90, -22, -90, -22 },
        { -78,  38, -78,  38, -78,  38, -78,  38 },
        { -46,  82, -46,  82, -46,  82, -46,  82 },
        {  -4,  88,  -4,  88,  -4,  88,  -4,  88 },
        {  38,  54,  38,  54,  38,  54,  38,  54 },
        {  73,  -4,  73,  -4,  73,  -4,  73,  -4 },
        {  90, -61,  90, -61,  90, -61,  90, -61 },
        {  85, -90,  85, -90,  85, -90,  85, -90 },
        {  61, -78,  61, -78,  61, -78,  61, -78 },
        {  22, -31,  22, -31,  22, -31,  22, -31 },
    },{/*  9-11   */
        {  82,  78,  82,  78,  82,  78,  82,  78 },
        {  22,  -4,  22,  -4,  22,  -4,  22,  -4 },
        { -54, -82, -54, -82, -54, -82, -54, -82 },
        { -90, -73, -90, -73, -90, -73, -90, -73 },
        { -61,  13, -61,  13, -61,  13, -61,  13 },
        {  13,  85,  13,  85,  13,  85,  13,  85 },
        {  78,  67,  78,  67,  78,  67,  78,  67 },
        {  85, -22,  85, -22,  85, -22,  85, -22 },
        {  31, -88,  31, -88,  31, -88,  31, -88 },
        { -46, -61, -46, -61, -46, -61, -46, -61 },
        { -90,  31, -90,  31, -90,  31, -90,  31 },
        { -67,  90, -67,  90, -67,  90, -67,  90 },
        {   4,  54,   4,  54,   4,  54,   4,  54 },
        {  73, -38,  73, -38,  73, -38,  73, -38 },
        {  88, -90,  88, -90,  88, -90,  88, -90 },
        {  38, -46,  38, -46,  38, -46,  38, -46 },
    },{/*  13-15   */
        {  73,  67,  73,  67,  73,  67,  73,  67 },
        { -31, -54, -31, -54, -31, -54, -31, -54 },
        { -90, -78, -90, -78, -90, -78, -90, -78 },
        { -22,  38, -22,  38, -22,  38, -22,  38 },
        {  78,  85,  78,  85,  78,  85,  78,  85 },
        {  67, -22,  67, -22,  67, -22,  67, -22 },
        { -38, -90, -38, -90, -38, -90, -38, -90 },
        { -90,   4, -90,   4, -90,   4, -90,   4 },
        { -13,  90, -13,  90, -13,  90, -13,  90 },
        {  82,  13,  82,  13,  82,  13,  82,  13 },
        {  61, -88,  61, -88,  61, -88,  61, -88 },
        { -46, -31, -46, -31, -46, -31, -46, -31 },
        { -88,  82, -88,  82, -88,  82, -88,  82 },
        { -4,   46, -4,   46, -4,   46, -4,   46 },
        {  85, -73,  85, -73,  85, -73,  85, -73 },
        {  54, -61,  54, -61,  54, -61,  54, -61 },
    },{/*  17-19   */
        {  61,  54,  61,  54,  61,  54,  61,  54 },
        { -73, -85, -73, -85, -73, -85, -73, -85 },
        { -46,  -4, -46,  -4, -46,  -4, -46,  -4 },
        {  82,  88,  82,  88,  82,  88,  82,  88 },
        {  31, -46,  31, -46,  31, -46,  31, -46 },
        { -88, -61, -88, -61, -88, -61, -88, -61 },
        { -13,  82, -13,  82, -13,  82, -13,  82 },
        {  90,  13,  90,  13,  90,  13,  90,  13 },
        { -4, -90,  -4, -90,  -4, -90,  -4, -90 },
        { -90,  38, -90,  38, -90,  38, -90,  38 },
        {  22,  67,  22,  67,  22,  67,  22,  67 },
        {  85, -78,  85, -78,  85, -78,  85, -78 },
        { -38, -22, -38, -22, -38, -22, -38, -22 },
        { -78,  90, -78,  90, -78,  90, -78,  90 },
        {  54, -31,  54, -31,  54, -31,  54, -31 },
        {  67, -73,  67, -73,  67, -73,  67, -73 },
    },{ /*  21-23   */
        {  46,  38,  46,  38,  46,  38,  46,  38 },
        { -90, -88, -90, -88, -90, -88, -90, -88 },
        {  38,  73,  38,  73,  38,  73,  38,  73 },
        {  54,  -4,  54,  -4,  54,  -4,  54,  -4 },
        { -90, -67, -90, -67, -90, -67, -90, -67 },
        {  31,  90,  31,  90,  31,  90,  31,  90 },
        {  61, -46,  61, -46,  61, -46,  61, -46 },
        { -88, -31, -88, -31, -88, -31, -88, -31 },
        {  22,  85,  22,  85,  22,  85,  22,  85 },
        {  67, -78,  67, -78,  67, -78,  67, -78 },
        { -85,  13, -85,  13, -85,  13, -85,  13 },
        {  13,  61,  13,  61,  13,  61,  13,  61 },
        {  73, -90,  73, -90,  73, -90,  73, -90 },
        { -82,  54, -82,  54, -82,  54, -82,  54 },
        {   4,  22,   4,  22,   4,  22,   4,  22 },
        {  78, -82,  78, -82,  78, -82,  78, -82 },
    },{ /*  25-27   */
        {  31,  22,  31,  22,  31,  22,  31,  22 },
        { -78, -61, -78, -61, -78, -61, -78, -61 },
        {  90,  85,  90,  85,  90,  85,  90,  85 },
        { -61, -90, -61, -90, -61, -90, -61, -90 },
        {   4,  73,   4,  73,   4,  73,   4,  73 },
        {  54, -38,  54, -38,  54, -38,  54, -38 },
        { -88,  -4, -88,  -4, -88,  -4, -88,  -4 },
        {  82,  46,  82,  46,  82,  46,  82,  46 },
        { -38, -78, -38, -78, -38, -78, -38, -78 },
        { -22,  90, -22,  90, -22,  90, -22,  90 },
        {  73, -82,  73, -82,  73, -82,  73, -82 },
        { -90,  54, -90,  54, -90,  54, -90,  54 },
        {  67, -13,  67, -13,  67, -13,  67, -13 },
        { -13, -31, -13, -31, -13, -31, -13, -31 },
        { -46,  67, -46,  67, -46,  67, -46,  67 },
        {  85, -88,  85, -88,  85, -88,  85, -88 },
    },{/*  29-31   */
        {  13,   4,  13,   4,  13,   4,  13,   4 },
        { -38, -13, -38, -13, -38, -13, -38, -13 },
        {  61,  22,  61,  22,  61,  22,  61,  22 },
        { -78, -31, -78, -31, -78, -31, -78, -31 },
        {  88,  38,  88,  38,  88,  38,  88,  38 },
        { -90, -46, -90, -46, -90, -46, -90, -46 },
        {  85,  54,  85,  54,  85,  54,  85,  54 },
        { -73, -61, -73, -61, -73, -61, -73, -61 },
        {  54,  67,  54,  67,  54,  67,  54,  67 },
        { -31, -73, -31, -73, -31, -73, -31, -73 },
        {   4,  78,   4,  78,   4,  78,   4,  78 },
        {  22, -82,  22, -82,  22, -82,  22, -82 },
        { -46,  85, -46,  85, -46,  85, -46,  85 },
        {  67, -88,  67, -88,  67, -88,  67, -88 },
        { -82,  90, -82,  90, -82,  90, -82,  90 },
        {  90, -90,  90, -90,  90, -90,  90, -90 },
    }
};

void xIDCT32(short *pSrc, short *pDst, intptr_t stride)
{
    int i,j;
    int shift;

    __m128i m128iS0, m128iS1, m128iS2, m128iS3, m128iS4, m128iS5, m128iS6, m128iS7, m128iS8, m128iS9, m128iS10, m128iS11, m128iS12, m128iS13, m128iS14, m128iS15 ,  m128iAdd, m128Tmp0,     m128Tmp1,m128Tmp2, m128Tmp3, m128Tmp4, m128Tmp5,m128Tmp6, m128Tmp7, E0h, E1h, E2h, E3h, E0l, E1l, E2l, E3l, O0h, O1h, O2h, O3h, O4h, O5h, O6h, O7h,O0l, O1l, O2l, O3l, O4l, O5l, O6l, O7l,EE0l, EE1l, EE2l, EE3l, E00l, E01l, EE0h, EE1h, EE2h, EE3h,E00h, E01h;
    __m128i E4l, E5l, E6l, E7l, E8l, E9l, E10l, E11l, E12l, E13l, E14l, E15l;
    __m128i E4h, E5h, E6h, E7h, E8h, E9h, E10h, E11h, E12h, E13h, E14h, E15h, EEE0l, EEE1l, EEE0h, EEE1h;
    __m128i m128iS16, m128iS17, m128iS18, m128iS19, m128iS20, m128iS21, m128iS22, m128iS23, m128iS24, m128iS25, m128iS26, m128iS27, m128iS28, m128iS29, m128iS30, m128iS31, m128Tmp8, m128Tmp9, m128Tmp10, m128Tmp11, m128Tmp12, m128Tmp13, m128Tmp14, m128Tmp15, O8h, O9h, O10h, O11h, O12h, O13h, O14h, O15h,O8l, O9l, O10l, O11l, O12l, O13l, O14l, O15l, E02l, E02h, E03l, E03h, EE7l, EE6l, EE5l, EE4l, EE7h, EE6h, EE5h, EE4h;
    m128iS0   = _mm_load_si128( (__m128i*)( pSrc       ) );
    m128iS1   = _mm_load_si128( (__m128i*)( pSrc +  32 ) );
    m128iS2   = _mm_load_si128( (__m128i*)( pSrc +  64 ) );
    m128iS3   = _mm_load_si128( (__m128i*)( pSrc +  96 ) );
    m128iS4   = _mm_loadu_si128((__m128i*)( pSrc + 128 ) );
    m128iS5   = _mm_load_si128( (__m128i*)( pSrc + 160 ) );
    m128iS6   = _mm_load_si128( (__m128i*)( pSrc + 192 ) );
    m128iS7   = _mm_load_si128( (__m128i*)( pSrc + 224 ) );
    m128iS8   = _mm_load_si128( (__m128i*)( pSrc + 256 ) );
    m128iS9   = _mm_load_si128( (__m128i*)( pSrc + 288 ) );
    m128iS10  = _mm_load_si128( (__m128i*)( pSrc + 320 ) );
    m128iS11  = _mm_load_si128( (__m128i*)( pSrc + 352 ) );
    m128iS12  = _mm_loadu_si128((__m128i*)( pSrc + 384 ) );
    m128iS13  = _mm_load_si128( (__m128i*)( pSrc + 416 ) );
    m128iS14  = _mm_load_si128( (__m128i*)( pSrc + 448 ) );
    m128iS15  = _mm_load_si128( (__m128i*)( pSrc + 480 ) );
    m128iS16  = _mm_load_si128( (__m128i*)( pSrc + 512 ) );
    m128iS17  = _mm_load_si128( (__m128i*)( pSrc + 544 ) );
    m128iS18  = _mm_load_si128( (__m128i*)( pSrc + 576 ) );
    m128iS19  = _mm_load_si128( (__m128i*)( pSrc + 608 ) );
    m128iS20  = _mm_load_si128( (__m128i*)( pSrc + 640 ) );
    m128iS21  = _mm_load_si128( (__m128i*)( pSrc + 672 ) );
    m128iS22  = _mm_load_si128( (__m128i*)( pSrc + 704 ) );
    m128iS23  = _mm_load_si128( (__m128i*)( pSrc + 736 ) );
    m128iS24  = _mm_load_si128( (__m128i*)( pSrc + 768 ) );
    m128iS25  = _mm_load_si128( (__m128i*)( pSrc + 800 ) );
    m128iS26  = _mm_load_si128( (__m128i*)( pSrc + 832 ) );
    m128iS27  = _mm_load_si128( (__m128i*)( pSrc + 864 ) );
    m128iS28  = _mm_load_si128( (__m128i*)( pSrc + 896 ) );
    m128iS29  = _mm_load_si128( (__m128i*)( pSrc + 928 ) );
    m128iS30  = _mm_load_si128( (__m128i*)( pSrc + 960 ) );
    m128iS31  = _mm_load_si128( (__m128i*)( pSrc + 992 ) );

    shift = 7;
    m128iAdd  = _mm_set1_epi32( 64 );

    for(j=0; j< 2; j++) {
        for(i=0; i < 32; i+=8) {
            m128Tmp0 = _mm_unpacklo_epi16(  m128iS1, m128iS3 );
            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][0] ) ) );
            m128Tmp1 = _mm_unpackhi_epi16(  m128iS1, m128iS3 );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][0] ) ) );


            m128Tmp2 =  _mm_unpacklo_epi16(  m128iS5, m128iS7 );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][0] ) ) );
            m128Tmp3 = _mm_unpackhi_epi16(  m128iS5, m128iS7 );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][0] ) ) );


            m128Tmp4 =  _mm_unpacklo_epi16(  m128iS9, m128iS11 );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][0] ) ) );
            m128Tmp5 = _mm_unpackhi_epi16(  m128iS9, m128iS11 );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][0] ) ) );


            m128Tmp6 =  _mm_unpacklo_epi16(  m128iS13, m128iS15 );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][0] ) ) );
            m128Tmp7 = _mm_unpackhi_epi16(  m128iS13, m128iS15 );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][0] ) ) );

            m128Tmp8 =  _mm_unpacklo_epi16(  m128iS17, m128iS19 );
            E4l = _mm_madd_epi16( m128Tmp8, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][0] ) ) );
            m128Tmp9 = _mm_unpackhi_epi16(  m128iS17, m128iS19 );
            E4h = _mm_madd_epi16( m128Tmp9, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][0] ) ) );

            m128Tmp10 =  _mm_unpacklo_epi16(  m128iS21, m128iS23 );
            E5l = _mm_madd_epi16( m128Tmp10, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][0] ) ) );
            m128Tmp11 = _mm_unpackhi_epi16(  m128iS21, m128iS23 );
            E5h = _mm_madd_epi16( m128Tmp11, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][0] ) ) );

            m128Tmp12 =  _mm_unpacklo_epi16(  m128iS25, m128iS27 );
            E6l = _mm_madd_epi16( m128Tmp12, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][0] ) ) );
            m128Tmp13 = _mm_unpackhi_epi16(  m128iS25, m128iS27 );
            E6h = _mm_madd_epi16( m128Tmp13, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][0] ) ) );

            m128Tmp14 =  _mm_unpacklo_epi16(  m128iS29, m128iS31 );
            E7l = _mm_madd_epi16( m128Tmp14, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][0] ) ) );
            m128Tmp15 = _mm_unpackhi_epi16(  m128iS29, m128iS31 );
            E7h = _mm_madd_epi16( m128Tmp15, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][0] ) ) );


            O0l = _mm_add_epi32(E0l, E1l);
            O0l = _mm_add_epi32(O0l, E2l);
            O0l = _mm_add_epi32(O0l, E3l);
            O0l = _mm_add_epi32(O0l, E4l);
            O0l = _mm_add_epi32(O0l, E5l);
            O0l = _mm_add_epi32(O0l, E6l);
            O0l = _mm_add_epi32(O0l, E7l);


            O0h = _mm_add_epi32(E0h, E1h);
            O0h = _mm_add_epi32(O0h, E2h);
            O0h = _mm_add_epi32(O0h, E3h);
            O0h = _mm_add_epi32(O0h, E4h);
            O0h = _mm_add_epi32(O0h, E5h);
            O0h = _mm_add_epi32(O0h, E6h);
            O0h = _mm_add_epi32(O0h, E7h);


            /* Compute O1*/
            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][1] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][1] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][1] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][1] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][1] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][1] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][1] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][1] ) ) );

            E4l = _mm_madd_epi16( m128Tmp8, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][1] ) ) );
            E4h = _mm_madd_epi16( m128Tmp9, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][1] ) ) );
            E5l = _mm_madd_epi16( m128Tmp10, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][1] ) ) );
            E5h = _mm_madd_epi16( m128Tmp11, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][1] ) ) );
            E6l = _mm_madd_epi16( m128Tmp12, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][1] ) ) );
            E6h = _mm_madd_epi16( m128Tmp13, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][1] ) ) );
            E7l = _mm_madd_epi16( m128Tmp14, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][1] ) ) );
            E7h = _mm_madd_epi16( m128Tmp15, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][1] ) ) );




            O1l = _mm_add_epi32(E0l, E1l);
            O1l = _mm_add_epi32(O1l, E2l);
            O1l = _mm_add_epi32(O1l, E3l);
            O1l = _mm_add_epi32(O1l, E4l);
            O1l = _mm_add_epi32(O1l, E5l);
            O1l = _mm_add_epi32(O1l, E6l);
            O1l = _mm_add_epi32(O1l, E7l);

            O1h = _mm_add_epi32(E0h, E1h);
            O1h = _mm_add_epi32(O1h, E2h);
            O1h = _mm_add_epi32(O1h, E3h);
            O1h = _mm_add_epi32(O1h, E4h);
            O1h = _mm_add_epi32(O1h, E5h);
            O1h = _mm_add_epi32(O1h, E6h);
            O1h = _mm_add_epi32(O1h, E7h);
            /* Compute O2*/
            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][2] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][2] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][2] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][2] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][2] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][2] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][2] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][2] ) ) );

            E4l = _mm_madd_epi16( m128Tmp8, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][2] ) ) );
            E4h = _mm_madd_epi16( m128Tmp9, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][2] ) ) );
            E5l = _mm_madd_epi16( m128Tmp10, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][2] ) ) );
            E5h = _mm_madd_epi16( m128Tmp11, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][2] ) ) );
            E6l = _mm_madd_epi16( m128Tmp12, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][2] ) ) );
            E6h = _mm_madd_epi16( m128Tmp13, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][2] ) ) );
            E7l = _mm_madd_epi16( m128Tmp14, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][2] ) ) );
            E7h = _mm_madd_epi16( m128Tmp15, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][2] ) ) );


            O2l = _mm_add_epi32(E0l, E1l);
            O2l = _mm_add_epi32(O2l, E2l);
            O2l = _mm_add_epi32(O2l, E3l);
            O2l = _mm_add_epi32(O2l, E4l);
            O2l = _mm_add_epi32(O2l, E5l);
            O2l = _mm_add_epi32(O2l, E6l);
            O2l = _mm_add_epi32(O2l, E7l);

            O2h = _mm_add_epi32(E0h, E1h);
            O2h = _mm_add_epi32(O2h, E2h);
            O2h = _mm_add_epi32(O2h, E3h);
            O2h = _mm_add_epi32(O2h, E4h);
            O2h = _mm_add_epi32(O2h, E5h);
            O2h = _mm_add_epi32(O2h, E6h);
            O2h = _mm_add_epi32(O2h, E7h);
            /* Compute O3*/
            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][3] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][3] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][3] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][3] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][3] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][3] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][3] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][3] ) ) );


            E4l = _mm_madd_epi16( m128Tmp8, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][3] ) ) );
            E4h = _mm_madd_epi16( m128Tmp9, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][3] ) ) );
            E5l = _mm_madd_epi16( m128Tmp10, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][3] ) ) );
            E5h = _mm_madd_epi16( m128Tmp11, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][3] ) ) );
            E6l = _mm_madd_epi16( m128Tmp12, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][3] ) ) );
            E6h = _mm_madd_epi16( m128Tmp13, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][3] ) ) );
            E7l = _mm_madd_epi16( m128Tmp14, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][3] ) ) );
            E7h = _mm_madd_epi16( m128Tmp15, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][3] ) ) );


            O3l = _mm_add_epi32(E0l, E1l);
            O3l = _mm_add_epi32(O3l, E2l);
            O3l = _mm_add_epi32(O3l, E3l);
            O3l = _mm_add_epi32(O3l, E4l);
            O3l = _mm_add_epi32(O3l, E5l);
            O3l = _mm_add_epi32(O3l, E6l);
            O3l = _mm_add_epi32(O3l, E7l);

            O3h = _mm_add_epi32(E0h, E1h);
            O3h = _mm_add_epi32(O3h, E2h);
            O3h = _mm_add_epi32(O3h, E3h);
            O3h = _mm_add_epi32(O3h, E4h);
            O3h = _mm_add_epi32(O3h, E5h);
            O3h = _mm_add_epi32(O3h, E6h);
            O3h = _mm_add_epi32(O3h, E7h);
            /* Compute O4*/

            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][4] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][4] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][4] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][4] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][4] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][4] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][4] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][4] ) ) );


            E4l = _mm_madd_epi16( m128Tmp8, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][4] ) ) );
            E4h = _mm_madd_epi16( m128Tmp9, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][4] ) ) );
            E5l = _mm_madd_epi16( m128Tmp10, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][4] ) ) );
            E5h = _mm_madd_epi16( m128Tmp11, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][4] ) ) );
            E6l = _mm_madd_epi16( m128Tmp12, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][4] ) ) );
            E6h = _mm_madd_epi16( m128Tmp13, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][4] ) ) );
            E7l = _mm_madd_epi16( m128Tmp14, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][4] ) ) );
            E7h = _mm_madd_epi16( m128Tmp15, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][4] ) ) );


            O4l = _mm_add_epi32(E0l, E1l);
            O4l = _mm_add_epi32(O4l, E2l);
            O4l = _mm_add_epi32(O4l, E3l);
            O4l = _mm_add_epi32(O4l, E4l);
            O4l = _mm_add_epi32(O4l, E5l);
            O4l = _mm_add_epi32(O4l, E6l);
            O4l = _mm_add_epi32(O4l, E7l);

            O4h = _mm_add_epi32(E0h, E1h);
            O4h = _mm_add_epi32(O4h, E2h);
            O4h = _mm_add_epi32(O4h, E3h);
            O4h = _mm_add_epi32(O4h, E4h);
            O4h = _mm_add_epi32(O4h, E5h);
            O4h = _mm_add_epi32(O4h, E6h);
            O4h = _mm_add_epi32(O4h, E7h);


            /* Compute O5*/
            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][5] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][5] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][5] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][5] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][5] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][5] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][5] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][5] ) ) );


            E4l = _mm_madd_epi16( m128Tmp8, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][5] ) ) );
            E4h = _mm_madd_epi16( m128Tmp9, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][5] ) ) );
            E5l = _mm_madd_epi16( m128Tmp10, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][5] ) ) );
            E5h = _mm_madd_epi16( m128Tmp11, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][5] ) ) );
            E6l = _mm_madd_epi16( m128Tmp12, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][5] ) ) );
            E6h = _mm_madd_epi16( m128Tmp13, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][5] ) ) );
            E7l = _mm_madd_epi16( m128Tmp14, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][5] ) ) );
            E7h = _mm_madd_epi16( m128Tmp15, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][5] ) ) );


            O5l = _mm_add_epi32(E0l, E1l);
            O5l = _mm_add_epi32(O5l, E2l);
            O5l = _mm_add_epi32(O5l, E3l);
            O5l = _mm_add_epi32(O5l, E4l);
            O5l = _mm_add_epi32(O5l, E5l);
            O5l = _mm_add_epi32(O5l, E6l);
            O5l = _mm_add_epi32(O5l, E7l);

            O5h = _mm_add_epi32(E0h, E1h);
            O5h = _mm_add_epi32(O5h, E2h);
            O5h = _mm_add_epi32(O5h, E3h);
            O5h = _mm_add_epi32(O5h, E4h);
            O5h = _mm_add_epi32(O5h, E5h);
            O5h = _mm_add_epi32(O5h, E6h);
            O5h = _mm_add_epi32(O5h, E7h);

            /* Compute O6*/

            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][6] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][6] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][6] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][6] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][6] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][6] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][6] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][6] ) ) );


            E4l = _mm_madd_epi16( m128Tmp8, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][6] ) ) );
            E4h = _mm_madd_epi16( m128Tmp9, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][6] ) ) );
            E5l = _mm_madd_epi16( m128Tmp10, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][6] ) ) );
            E5h = _mm_madd_epi16( m128Tmp11, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][6] ) ) );
            E6l = _mm_madd_epi16( m128Tmp12, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][6] ) ) );
            E6h = _mm_madd_epi16( m128Tmp13, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][6] ) ) );
            E7l = _mm_madd_epi16( m128Tmp14, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][6] ) ) );
            E7h = _mm_madd_epi16( m128Tmp15, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][6] ) ) );


            O6l = _mm_add_epi32(E0l, E1l);
            O6l = _mm_add_epi32(O6l, E2l);
            O6l = _mm_add_epi32(O6l, E3l);
            O6l = _mm_add_epi32(O6l, E4l);
            O6l = _mm_add_epi32(O6l, E5l);
            O6l = _mm_add_epi32(O6l, E6l);
            O6l = _mm_add_epi32(O6l, E7l);

            O6h = _mm_add_epi32(E0h, E1h);
            O6h = _mm_add_epi32(O6h, E2h);
            O6h = _mm_add_epi32(O6h, E3h);
            O6h = _mm_add_epi32(O6h, E4h);
            O6h = _mm_add_epi32(O6h, E5h);
            O6h = _mm_add_epi32(O6h, E6h);
            O6h = _mm_add_epi32(O6h, E7h);

            /* Compute O7*/

            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][7] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][7] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][7] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][7] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][7] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][7] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][7] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][7] ) ) );


            E4l = _mm_madd_epi16( m128Tmp8, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][7] ) ) );
            E4h = _mm_madd_epi16( m128Tmp9, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][7] ) ) );
            E5l = _mm_madd_epi16( m128Tmp10, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][7] ) ) );
            E5h = _mm_madd_epi16( m128Tmp11, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][7] ) ) );
            E6l = _mm_madd_epi16( m128Tmp12, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][7] ) ) );
            E6h = _mm_madd_epi16( m128Tmp13, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][7] ) ) );
            E7l = _mm_madd_epi16( m128Tmp14, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][7] ) ) );
            E7h = _mm_madd_epi16( m128Tmp15, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][7] ) ) );


            O7l = _mm_add_epi32(E0l, E1l);
            O7l = _mm_add_epi32(O7l, E2l);
            O7l = _mm_add_epi32(O7l, E3l);
            O7l = _mm_add_epi32(O7l, E4l);
            O7l = _mm_add_epi32(O7l, E5l);
            O7l = _mm_add_epi32(O7l, E6l);
            O7l = _mm_add_epi32(O7l, E7l);

            O7h = _mm_add_epi32(E0h, E1h);
            O7h = _mm_add_epi32(O7h, E2h);
            O7h = _mm_add_epi32(O7h, E3h);
            O7h = _mm_add_epi32(O7h, E4h);
            O7h = _mm_add_epi32(O7h, E5h);
            O7h = _mm_add_epi32(O7h, E6h);
            O7h = _mm_add_epi32(O7h, E7h);

            /* Compute O8*/

            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][8] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][8] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][8] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][8] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][8] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][8] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][8] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][8] ) ) );


            E4l = _mm_madd_epi16( m128Tmp8, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][8] ) ) );
            E4h = _mm_madd_epi16( m128Tmp9, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][8] ) ) );
            E5l = _mm_madd_epi16( m128Tmp10, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][8] ) ) );
            E5h = _mm_madd_epi16( m128Tmp11, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][8] ) ) );
            E6l = _mm_madd_epi16( m128Tmp12, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][8] ) ) );
            E6h = _mm_madd_epi16( m128Tmp13, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][8] ) ) );
            E7l = _mm_madd_epi16( m128Tmp14, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][8] ) ) );
            E7h = _mm_madd_epi16( m128Tmp15, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][8] ) ) );


            O8l = _mm_add_epi32(E0l, E1l);
            O8l = _mm_add_epi32(O8l, E2l);
            O8l = _mm_add_epi32(O8l, E3l);
            O8l = _mm_add_epi32(O8l, E4l);
            O8l = _mm_add_epi32(O8l, E5l);
            O8l = _mm_add_epi32(O8l, E6l);
            O8l = _mm_add_epi32(O8l, E7l);

            O8h = _mm_add_epi32(E0h, E1h);
            O8h = _mm_add_epi32(O8h, E2h);
            O8h = _mm_add_epi32(O8h, E3h);
            O8h = _mm_add_epi32(O8h, E4h);
            O8h = _mm_add_epi32(O8h, E5h);
            O8h = _mm_add_epi32(O8h, E6h);
            O8h = _mm_add_epi32(O8h, E7h);


            /* Compute O9*/

            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][9] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][9] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][9] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][9] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][9] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][9] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][9] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][9] ) ) );


            E4l = _mm_madd_epi16( m128Tmp8, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][9] ) ) );
            E4h = _mm_madd_epi16( m128Tmp9, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][9] ) ) );
            E5l = _mm_madd_epi16( m128Tmp10, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][9] ) ) );
            E5h = _mm_madd_epi16( m128Tmp11, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][9] ) ) );
            E6l = _mm_madd_epi16( m128Tmp12, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][9] ) ) );
            E6h = _mm_madd_epi16( m128Tmp13, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][9] ) ) );
            E7l = _mm_madd_epi16( m128Tmp14, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][9] ) ) );
            E7h = _mm_madd_epi16( m128Tmp15, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][9] ) ) );


            O9l = _mm_add_epi32(E0l, E1l);
            O9l = _mm_add_epi32(O9l, E2l);
            O9l = _mm_add_epi32(O9l, E3l);
            O9l = _mm_add_epi32(O9l, E4l);
            O9l = _mm_add_epi32(O9l, E5l);
            O9l = _mm_add_epi32(O9l, E6l);
            O9l = _mm_add_epi32(O9l, E7l);

            O9h = _mm_add_epi32(E0h, E1h);
            O9h = _mm_add_epi32(O9h, E2h);
            O9h = _mm_add_epi32(O9h, E3h);
            O9h = _mm_add_epi32(O9h, E4h);
            O9h = _mm_add_epi32(O9h, E5h);
            O9h = _mm_add_epi32(O9h, E6h);
            O9h = _mm_add_epi32(O9h, E7h);


            /* Compute 10*/

            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][10] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][10] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][10] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][10] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][10] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][10] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][10] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][10] ) ) );


            E4l = _mm_madd_epi16( m128Tmp8, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][10] ) ) );
            E4h = _mm_madd_epi16( m128Tmp9, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][10] ) ) );
            E5l = _mm_madd_epi16( m128Tmp10, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][10] ) ) );
            E5h = _mm_madd_epi16( m128Tmp11, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][10] ) ) );
            E6l = _mm_madd_epi16( m128Tmp12, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][10] ) ) );
            E6h = _mm_madd_epi16( m128Tmp13, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][10] ) ) );
            E7l = _mm_madd_epi16( m128Tmp14, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][10] ) ) );
            E7h = _mm_madd_epi16( m128Tmp15, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][10] ) ) );


            O10l = _mm_add_epi32(E0l, E1l);
            O10l = _mm_add_epi32(O10l, E2l);
            O10l = _mm_add_epi32(O10l, E3l);
            O10l = _mm_add_epi32(O10l, E4l);
            O10l = _mm_add_epi32(O10l, E5l);
            O10l = _mm_add_epi32(O10l, E6l);
            O10l = _mm_add_epi32(O10l, E7l);

            O10h = _mm_add_epi32(E0h, E1h);
            O10h = _mm_add_epi32(O10h, E2h);
            O10h = _mm_add_epi32(O10h, E3h);
            O10h = _mm_add_epi32(O10h, E4h);
            O10h = _mm_add_epi32(O10h, E5h);
            O10h = _mm_add_epi32(O10h, E6h);
            O10h = _mm_add_epi32(O10h, E7h);




            /* Compute 11*/

            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][11] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][11] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][11] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][11] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][11] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][11] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][11] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][11] ) ) );


            E4l = _mm_madd_epi16( m128Tmp8, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][11] ) ) );
            E4h = _mm_madd_epi16( m128Tmp9, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][11] ) ) );
            E5l = _mm_madd_epi16( m128Tmp10, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][11] ) ) );
            E5h = _mm_madd_epi16( m128Tmp11, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][11] ) ) );
            E6l = _mm_madd_epi16( m128Tmp12, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][11] ) ) );
            E6h = _mm_madd_epi16( m128Tmp13, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][11] ) ) );
            E7l = _mm_madd_epi16( m128Tmp14, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][11] ) ) );
            E7h = _mm_madd_epi16( m128Tmp15, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][11] ) ) );


            O11l = _mm_add_epi32(E0l, E1l);
            O11l = _mm_add_epi32(O11l, E2l);
            O11l = _mm_add_epi32(O11l, E3l);
            O11l = _mm_add_epi32(O11l, E4l);
            O11l = _mm_add_epi32(O11l, E5l);
            O11l = _mm_add_epi32(O11l, E6l);
            O11l = _mm_add_epi32(O11l, E7l);

            O11h = _mm_add_epi32(E0h, E1h);
            O11h = _mm_add_epi32(O11h, E2h);
            O11h = _mm_add_epi32(O11h, E3h);
            O11h = _mm_add_epi32(O11h, E4h);
            O11h = _mm_add_epi32(O11h, E5h);
            O11h = _mm_add_epi32(O11h, E6h);
            O11h = _mm_add_epi32(O11h, E7h);



            /* Compute 12*/

            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][12] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][12] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][12] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][12] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][12] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][12] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][12] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][12] ) ) );


            E4l = _mm_madd_epi16( m128Tmp8, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][12] ) ) );
            E4h = _mm_madd_epi16( m128Tmp9, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][12] ) ) );
            E5l = _mm_madd_epi16( m128Tmp10, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][12] ) ) );
            E5h = _mm_madd_epi16( m128Tmp11, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][12] ) ) );
            E6l = _mm_madd_epi16( m128Tmp12, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][12] ) ) );
            E6h = _mm_madd_epi16( m128Tmp13, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][12] ) ) );
            E7l = _mm_madd_epi16( m128Tmp14, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][12] ) ) );
            E7h = _mm_madd_epi16( m128Tmp15, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][12] ) ) );


            O12l = _mm_add_epi32(E0l, E1l);
            O12l = _mm_add_epi32(O12l, E2l);
            O12l = _mm_add_epi32(O12l, E3l);
            O12l = _mm_add_epi32(O12l, E4l);
            O12l = _mm_add_epi32(O12l, E5l);
            O12l = _mm_add_epi32(O12l, E6l);
            O12l = _mm_add_epi32(O12l, E7l);

            O12h = _mm_add_epi32(E0h, E1h);
            O12h = _mm_add_epi32(O12h, E2h);
            O12h = _mm_add_epi32(O12h, E3h);
            O12h = _mm_add_epi32(O12h, E4h);
            O12h = _mm_add_epi32(O12h, E5h);
            O12h = _mm_add_epi32(O12h, E6h);
            O12h = _mm_add_epi32(O12h, E7h);



            /* Compute 13*/

            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][13] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][13] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][13] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][13] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][13] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][13] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][13] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][13] ) ) );


            E4l = _mm_madd_epi16( m128Tmp8, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][13] ) ) );
            E4h = _mm_madd_epi16( m128Tmp9, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][13] ) ) );
            E5l = _mm_madd_epi16( m128Tmp10, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][13] ) ) );
            E5h = _mm_madd_epi16( m128Tmp11, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][13] ) ) );
            E6l = _mm_madd_epi16( m128Tmp12, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][13] ) ) );
            E6h = _mm_madd_epi16( m128Tmp13, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][13] ) ) );
            E7l = _mm_madd_epi16( m128Tmp14, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][13] ) ) );
            E7h = _mm_madd_epi16( m128Tmp15, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][13] ) ) );


            O13l = _mm_add_epi32(E0l, E1l);
            O13l = _mm_add_epi32(O13l, E2l);
            O13l = _mm_add_epi32(O13l, E3l);
            O13l = _mm_add_epi32(O13l, E4l);
            O13l = _mm_add_epi32(O13l, E5l);
            O13l = _mm_add_epi32(O13l, E6l);
            O13l = _mm_add_epi32(O13l, E7l);

            O13h = _mm_add_epi32(E0h, E1h);
            O13h = _mm_add_epi32(O13h, E2h);
            O13h = _mm_add_epi32(O13h, E3h);
            O13h = _mm_add_epi32(O13h, E4h);
            O13h = _mm_add_epi32(O13h, E5h);
            O13h = _mm_add_epi32(O13h, E6h);
            O13h = _mm_add_epi32(O13h, E7h);


            /* Compute O14  */

            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][14] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][14] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][14] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][14] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][14] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][14] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][14] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][14] ) ) );


            E4l = _mm_madd_epi16( m128Tmp8, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][14] ) ) );
            E4h = _mm_madd_epi16( m128Tmp9, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][14] ) ) );
            E5l = _mm_madd_epi16( m128Tmp10, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][14] ) ) );
            E5h = _mm_madd_epi16( m128Tmp11, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][14] ) ) );
            E6l = _mm_madd_epi16( m128Tmp12, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][14] ) ) );
            E6h = _mm_madd_epi16( m128Tmp13, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][14] ) ) );
            E7l = _mm_madd_epi16( m128Tmp14, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][14] ) ) );
            E7h = _mm_madd_epi16( m128Tmp15, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][14] ) ) );


            O14l = _mm_add_epi32(E0l, E1l);
            O14l = _mm_add_epi32(O14l, E2l);
            O14l = _mm_add_epi32(O14l, E3l);
            O14l = _mm_add_epi32(O14l, E4l);
            O14l = _mm_add_epi32(O14l, E5l);
            O14l = _mm_add_epi32(O14l, E6l);
            O14l = _mm_add_epi32(O14l, E7l);

            O14h = _mm_add_epi32(E0h, E1h);
            O14h = _mm_add_epi32(O14h, E2h);
            O14h = _mm_add_epi32(O14h, E3h);
            O14h = _mm_add_epi32(O14h, E4h);
            O14h = _mm_add_epi32(O14h, E5h);
            O14h = _mm_add_epi32(O14h, E6h);
            O14h = _mm_add_epi32(O14h, E7h);



            /* Compute O15*/

            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][15] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][15] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][15] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][15] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][15] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][15] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][15] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][15] ) ) );


            E4l = _mm_madd_epi16( m128Tmp8, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][15] ) ) );
            E4h = _mm_madd_epi16( m128Tmp9, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][15] ) ) );
            E5l = _mm_madd_epi16( m128Tmp10, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][15] ) ) );
            E5h = _mm_madd_epi16( m128Tmp11, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][15] ) ) );
            E6l = _mm_madd_epi16( m128Tmp12, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][15] ) ) );
            E6h = _mm_madd_epi16( m128Tmp13, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][15] ) ) );
            E7l = _mm_madd_epi16( m128Tmp14, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][15] ) ) );
            E7h = _mm_madd_epi16( m128Tmp15, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][15] ) ) );


            O15l = _mm_add_epi32(E0l, E1l);
            O15l = _mm_add_epi32(O15l, E2l);
            O15l = _mm_add_epi32(O15l, E3l);
            O15l = _mm_add_epi32(O15l, E4l);
            O15l = _mm_add_epi32(O15l, E5l);
            O15l = _mm_add_epi32(O15l, E6l);
            O15l = _mm_add_epi32(O15l, E7l);

            O15h = _mm_add_epi32(E0h, E1h);
            O15h = _mm_add_epi32(O15h, E2h);
            O15h = _mm_add_epi32(O15h, E3h);
            O15h = _mm_add_epi32(O15h, E4h);
            O15h = _mm_add_epi32(O15h, E5h);
            O15h = _mm_add_epi32(O15h, E6h);
            O15h = _mm_add_epi32(O15h, E7h);
            /*  Compute E0  */

            m128Tmp0 = _mm_unpacklo_epi16(  m128iS2, m128iS6 );
            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][0] ) ) );
            m128Tmp1 = _mm_unpackhi_epi16(  m128iS2, m128iS6 );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][0] ) ) );


            m128Tmp2 =  _mm_unpacklo_epi16(  m128iS10, m128iS14 );
            E0l = _mm_add_epi32(E0l, _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][0] ) ) ));
            m128Tmp3 = _mm_unpackhi_epi16(  m128iS10, m128iS14 );
            E0h = _mm_add_epi32(E0h, _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][0] ) ) ));

            m128Tmp4 =  _mm_unpacklo_epi16(  m128iS18, m128iS22 );
            E0l = _mm_add_epi32(E0l, _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][0] ) ) ));
            m128Tmp5 = _mm_unpackhi_epi16(  m128iS18, m128iS22 );
            E0h = _mm_add_epi32(E0h, _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][0] ) ) ));


            m128Tmp6 =  _mm_unpacklo_epi16(  m128iS26, m128iS30 );
            E0l = _mm_add_epi32(E0l, _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][0] ) ) ));
            m128Tmp7 = _mm_unpackhi_epi16(  m128iS26, m128iS30 );
            E0h = _mm_add_epi32(E0h, _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][0] ) ) ));

            /*  Compute E1  */
            E1l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][1] ) ));
            E1h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][1] ) ) );
            E1l = _mm_add_epi32(E1l,_mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][1] ) ) ));
            E1h = _mm_add_epi32(E1h,_mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][1] ) ) ));
            E1l = _mm_add_epi32(E1l,_mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][1] ) ) ));
            E1h = _mm_add_epi32(E1h,_mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][1] ) ) ));
            E1l = _mm_add_epi32(E1l,_mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][1] ) ) ));
            E1h = _mm_add_epi32(E1h,_mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][1] ) ) ));

            /*  Compute E2  */
            E2l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][2] ) ) );
            E2h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][2] ) ) );
            E2l = _mm_add_epi32(E2l,_mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][2] ) ) ));
            E2h = _mm_add_epi32(E2h,_mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][2] ) ) ));
            E2l = _mm_add_epi32(E2l,_mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][2] ) ) ));
            E2h = _mm_add_epi32(E2h,_mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][2] ) ) ));
            E2l = _mm_add_epi32(E2l,_mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][2] ) ) ));
            E2h = _mm_add_epi32(E2h,_mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][2] ) ) ));


            /*  Compute E3  */
            E3l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][3] ) ) );
            E3h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][3] ) ) );
            E3l = _mm_add_epi32(E3l,_mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][3] ) ) ));
            E3h = _mm_add_epi32(E3h,_mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][3] ) ) ));
            E3l = _mm_add_epi32(E3l,_mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][3] ) ) ));
            E3h = _mm_add_epi32(E3h,_mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][3] ) ) ));
            E3l = _mm_add_epi32(E3l,_mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][3] ) ) ));
            E3h = _mm_add_epi32(E3h,_mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][3] ) ) ));

            /*  Compute E4  */
            E4l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][4] ) ) );
            E4h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][4] ) ) );
            E4l = _mm_add_epi32(E4l,_mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][4] ) ) ));
            E4h = _mm_add_epi32(E4h,_mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][4] ) ) ));
            E4l = _mm_add_epi32(E4l,_mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][4] ) ) ));
            E4h = _mm_add_epi32(E4h,_mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][4] ) ) ));
            E4l = _mm_add_epi32(E4l,_mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][4] ) ) ));
            E4h = _mm_add_epi32(E4h,_mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][4] ) ) ));


            /*  Compute E3  */
            E5l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][5] ) ) );
            E5h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][5] ) ) );
            E5l = _mm_add_epi32(E5l,_mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][5] ) ) ));
            E5h = _mm_add_epi32(E5h,_mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][5] ) ) ));
            E5l = _mm_add_epi32(E5l,_mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][5] ) ) ));
            E5h = _mm_add_epi32(E5h,_mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][5] ) ) ));
            E5l = _mm_add_epi32(E5l,_mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][5] ) ) ));
            E5h = _mm_add_epi32(E5h,_mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][5] ) ) ));


            /*  Compute E6  */
            E6l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][6] ) ) );
            E6h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][6] ) ) );
            E6l = _mm_add_epi32(E6l,_mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][6] ) ) ));
            E6h = _mm_add_epi32(E6h,_mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][6] ) ) ));
            E6l = _mm_add_epi32(E6l,_mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][6] ) ) ));
            E6h = _mm_add_epi32(E6h,_mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][6] ) ) ));
            E6l = _mm_add_epi32(E6l,_mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][6] ) ) ));
            E6h = _mm_add_epi32(E6h,_mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][6] ) ) ));

            /*  Compute E7  */
            E7l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][7] ) ) );
            E7h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][7] ) ) );
            E7l = _mm_add_epi32(E7l,_mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][7] ) ) ));
            E7h = _mm_add_epi32(E7h,_mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][7] ) ) ));
            E7l = _mm_add_epi32(E7l,_mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][7] ) ) ));
            E7h = _mm_add_epi32(E7h,_mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][7] ) ) ));
            E7l = _mm_add_epi32(E7l,_mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][7] ) ) ));
            E7h = _mm_add_epi32(E7h,_mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][7] ) ) ));


            /*  Compute EE0 and EEE */


            m128Tmp0 = _mm_unpacklo_epi16(  m128iS4, m128iS12 );
            E00l =  _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[0][0] ) ) );
            m128Tmp1 = _mm_unpackhi_epi16(  m128iS4, m128iS12 );
            E00h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[0][0] ) ) );

            m128Tmp2 = _mm_unpacklo_epi16(  m128iS20, m128iS28 );
            E00l =  _mm_add_epi32(E00l, _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[1][0] ) ) ));
            m128Tmp3 = _mm_unpackhi_epi16(  m128iS20, m128iS28 );
            E00h = _mm_add_epi32(E00h,_mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[1][0] ) ) ));



            E01l =  _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[0][1] ) ) );
            E01h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[0][1] ) ) );
            E01l =  _mm_add_epi32(E01l, _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[1][1] ) ) ));
            E01h = _mm_add_epi32(E01h,_mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[1][1] ) ) ));

            E02l =  _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[0][2] ) ) );
            E02h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[0][2] ) ) );
            E02l =  _mm_add_epi32(E02l, _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[1][2] ) ) ));
            E02h = _mm_add_epi32(E02h,_mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[1][2] ) ) ));

            E03l =  _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[0][3] ) ) );
            E03h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[0][3] ) ) );
            E03l =  _mm_add_epi32(E03l, _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[1][3] ) ) ));
            E03h = _mm_add_epi32(E03h,_mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[1][3] ) ) ));

            /*  Compute EE0 and EEE */


            m128Tmp0 = _mm_unpacklo_epi16(  m128iS8, m128iS24 );
            EE0l =  _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_3[0][0] ) ) );
            m128Tmp1 = _mm_unpackhi_epi16(  m128iS8, m128iS24 );
            EE0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_3[0][0] ) ) );

            m128Tmp2 =  _mm_unpacklo_epi16(  m128iS0, m128iS16 );
            EEE0l =  _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_3[1][0] ) ) );
            m128Tmp3 = _mm_unpackhi_epi16(  m128iS0, m128iS16 );
            EEE0h =  _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_3[1][0] ) ) );


            EE1l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_3[0][1] ) ) );
            EE1h  = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_3[0][1] ) ) );

            EEE1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_3[1][1] ) ) );
            EEE1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_3[1][1] ) ) );

            /*  Compute EE    */

            EE2l = _mm_sub_epi32(EEE1l,EE1l);
            EE3l = _mm_sub_epi32(EEE0l,EE0l);
            EE2h = _mm_sub_epi32(EEE1h,EE1h);
            EE3h = _mm_sub_epi32(EEE0h,EE0h);

            EE0l = _mm_add_epi32(EEE0l,EE0l);
            EE1l = _mm_add_epi32(EEE1l,EE1l);
            EE0h = _mm_add_epi32(EEE0h,EE0h);
            EE1h = _mm_add_epi32(EEE1h,EE1h);
            /**/

            EE7l = _mm_sub_epi32(EE0l, E00l);
            EE6l = _mm_sub_epi32(EE1l, E01l);
            EE5l = _mm_sub_epi32(EE2l, E02l);
            EE4l = _mm_sub_epi32(EE3l, E03l);

            EE7h = _mm_sub_epi32(EE0h, E00h);
            EE6h = _mm_sub_epi32(EE1h, E01h);
            EE5h = _mm_sub_epi32(EE2h, E02h);
            EE4h = _mm_sub_epi32(EE3h, E03h);


            EE0l = _mm_add_epi32(EE0l, E00l);
            EE1l = _mm_add_epi32(EE1l, E01l);
            EE2l = _mm_add_epi32(EE2l, E02l);
            EE3l = _mm_add_epi32(EE3l, E03l);

            EE0h = _mm_add_epi32(EE0h, E00h);
            EE1h = _mm_add_epi32(EE1h, E01h);
            EE2h = _mm_add_epi32(EE2h, E02h);
            EE3h = _mm_add_epi32(EE3h, E03h);
            /*      Compute E       */

            E15l = _mm_sub_epi32(EE0l,E0l);
            E15l = _mm_add_epi32(E15l, m128iAdd);
            E14l = _mm_sub_epi32(EE1l,E1l);
            E14l = _mm_add_epi32(E14l, m128iAdd);
            E13l = _mm_sub_epi32(EE2l,E2l);
            E13l = _mm_add_epi32(E13l, m128iAdd);
            E12l = _mm_sub_epi32(EE3l,E3l);
            E12l = _mm_add_epi32(E12l, m128iAdd);
            E11l = _mm_sub_epi32(EE4l,E4l);
            E11l = _mm_add_epi32(E11l, m128iAdd);
            E10l = _mm_sub_epi32(EE5l,E5l);
            E10l = _mm_add_epi32(E10l, m128iAdd);
            E9l = _mm_sub_epi32(EE6l,E6l);
            E9l = _mm_add_epi32(E9l, m128iAdd);
            E8l = _mm_sub_epi32(EE7l,E7l);
            E8l = _mm_add_epi32(E8l, m128iAdd);

            E0l = _mm_add_epi32(EE0l,E0l);
            E0l = _mm_add_epi32(E0l, m128iAdd);
            E1l = _mm_add_epi32(EE1l,E1l);
            E1l = _mm_add_epi32(E1l, m128iAdd);
            E2l = _mm_add_epi32(EE2l,E2l);
            E2l = _mm_add_epi32(E2l, m128iAdd);
            E3l = _mm_add_epi32(EE3l,E3l);
            E3l = _mm_add_epi32(E3l, m128iAdd);
            E4l = _mm_add_epi32(EE4l,E4l);
            E4l = _mm_add_epi32(E4l, m128iAdd);
            E5l = _mm_add_epi32(EE5l,E5l);
            E5l = _mm_add_epi32(E5l, m128iAdd);
            E6l = _mm_add_epi32(EE6l,E6l);
            E6l = _mm_add_epi32(E6l, m128iAdd);
            E7l = _mm_add_epi32(EE7l,E7l);
            E7l = _mm_add_epi32(E7l, m128iAdd);


            E15h = _mm_sub_epi32(EE0h,E0h);
            E15h = _mm_add_epi32(E15h, m128iAdd);
            E14h = _mm_sub_epi32(EE1h,E1h);
            E14h = _mm_add_epi32(E14h, m128iAdd);
            E13h = _mm_sub_epi32(EE2h,E2h);
            E13h = _mm_add_epi32(E13h, m128iAdd);
            E12h = _mm_sub_epi32(EE3h,E3h);
            E12h = _mm_add_epi32(E12h, m128iAdd);
            E11h = _mm_sub_epi32(EE4h,E4h);
            E11h = _mm_add_epi32(E11h, m128iAdd);
            E10h = _mm_sub_epi32(EE5h,E5h);
            E10h = _mm_add_epi32(E10h, m128iAdd);
            E9h = _mm_sub_epi32(EE6h,E6h);
            E9h = _mm_add_epi32(E9h, m128iAdd);
            E8h = _mm_sub_epi32(EE7h,E7h);
            E8h = _mm_add_epi32(E8h, m128iAdd);

            E0h = _mm_add_epi32(EE0h,E0h);
            E0h = _mm_add_epi32(E0h, m128iAdd);
            E1h = _mm_add_epi32(EE1h,E1h);
            E1h = _mm_add_epi32(E1h, m128iAdd);
            E2h = _mm_add_epi32(EE2h,E2h);
            E2h = _mm_add_epi32(E2h, m128iAdd);
            E3h = _mm_add_epi32(EE3h,E3h);
            E3h = _mm_add_epi32(E3h, m128iAdd);
            E4h = _mm_add_epi32(EE4h,E4h);
            E4h = _mm_add_epi32(E4h, m128iAdd);
            E5h = _mm_add_epi32(EE5h,E5h);
            E5h = _mm_add_epi32(E5h, m128iAdd);
            E6h = _mm_add_epi32(EE6h,E6h);
            E6h = _mm_add_epi32(E6h, m128iAdd);
            E7h = _mm_add_epi32(EE7h,E7h);
            E7h = _mm_add_epi32(E7h, m128iAdd);


            m128iS0 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E0l, O0l),shift), _mm_srai_epi32(_mm_add_epi32(E0h, O0h), shift));
            m128iS1 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E1l, O1l),shift), _mm_srai_epi32(_mm_add_epi32(E1h, O1h), shift));
            m128iS2 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E2l, O2l),shift), _mm_srai_epi32(_mm_add_epi32(E2h, O2h), shift));
            m128iS3 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E3l, O3l),shift), _mm_srai_epi32(_mm_add_epi32(E3h, O3h), shift));
            m128iS4 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E4l, O4l),shift), _mm_srai_epi32(_mm_add_epi32(E4h, O4h), shift));
            m128iS5 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E5l, O5l),shift), _mm_srai_epi32(_mm_add_epi32(E5h, O5h), shift));
            m128iS6 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E6l, O6l),shift), _mm_srai_epi32(_mm_add_epi32(E6h, O6h), shift));
            m128iS7 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E7l, O7l),shift), _mm_srai_epi32(_mm_add_epi32(E7h, O7h), shift));
            m128iS8 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E8l, O8l),shift), _mm_srai_epi32(_mm_add_epi32(E8h, O8h), shift));
            m128iS9 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E9l, O9l),shift), _mm_srai_epi32(_mm_add_epi32(E9h, O9h), shift));
            m128iS10 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E10l, O10l),shift), _mm_srai_epi32(_mm_add_epi32(E10h, O10h), shift));
            m128iS11 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E11l, O11l),shift), _mm_srai_epi32(_mm_add_epi32(E11h, O11h), shift));
            m128iS12 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E12l, O12l),shift), _mm_srai_epi32(_mm_add_epi32(E12h, O12h), shift));
            m128iS13 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E13l, O13l),shift), _mm_srai_epi32(_mm_add_epi32(E13h, O13h), shift));
            m128iS14 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E14l, O14l),shift), _mm_srai_epi32(_mm_add_epi32(E14h, O14h), shift));
            m128iS15 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E15l, O15l),shift), _mm_srai_epi32(_mm_add_epi32(E15h, O15h), shift));

            m128iS31 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E0l, O0l),shift), _mm_srai_epi32(_mm_sub_epi32(E0h, O0h), shift));
            m128iS30 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E1l, O1l),shift), _mm_srai_epi32(_mm_sub_epi32(E1h, O1h), shift));
            m128iS29 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E2l, O2l),shift), _mm_srai_epi32(_mm_sub_epi32(E2h, O2h), shift));
            m128iS28 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E3l, O3l),shift), _mm_srai_epi32(_mm_sub_epi32(E3h, O3h), shift));
            m128iS27 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E4l, O4l),shift), _mm_srai_epi32(_mm_sub_epi32(E4h, O4h), shift));
            m128iS26 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E5l, O5l),shift), _mm_srai_epi32(_mm_sub_epi32(E5h, O5h), shift));
            m128iS25 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E6l, O6l),shift), _mm_srai_epi32(_mm_sub_epi32(E6h, O6h), shift));
            m128iS24 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E7l, O7l),shift), _mm_srai_epi32(_mm_sub_epi32(E7h, O7h), shift));
            m128iS23 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E8l, O8l),shift), _mm_srai_epi32(_mm_sub_epi32(E8h, O8h), shift));
            m128iS22 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E9l, O9l),shift), _mm_srai_epi32(_mm_sub_epi32(E9h, O9h), shift));
            m128iS21 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E10l, O10l),shift), _mm_srai_epi32(_mm_sub_epi32(E10h, O10h), shift));
            m128iS20 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E11l, O11l),shift), _mm_srai_epi32(_mm_sub_epi32(E11h, O11h), shift));
            m128iS19 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E12l, O12l),shift), _mm_srai_epi32(_mm_sub_epi32(E12h, O12h), shift));
            m128iS18 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E13l, O13l),shift), _mm_srai_epi32(_mm_sub_epi32(E13h, O13h), shift));
            m128iS17 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E14l, O14l),shift), _mm_srai_epi32(_mm_sub_epi32(E14h, O14h), shift));
            m128iS16 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E15l, O15l),shift), _mm_srai_epi32(_mm_sub_epi32(E15h, O15h), shift));

            if(!j){
                /*      Inverse the matrix      */
                E0l = _mm_unpacklo_epi16(m128iS0, m128iS16);
                E1l = _mm_unpacklo_epi16(m128iS1, m128iS17);
                E2l = _mm_unpacklo_epi16(m128iS2, m128iS18);
                E3l = _mm_unpacklo_epi16(m128iS3, m128iS19);
                E4l = _mm_unpacklo_epi16(m128iS4, m128iS20);
                E5l = _mm_unpacklo_epi16(m128iS5, m128iS21);
                E6l = _mm_unpacklo_epi16(m128iS6, m128iS22);
                E7l = _mm_unpacklo_epi16(m128iS7, m128iS23);
                E8l = _mm_unpacklo_epi16(m128iS8, m128iS24);
                E9l = _mm_unpacklo_epi16(m128iS9, m128iS25);
                E10l = _mm_unpacklo_epi16(m128iS10, m128iS26);
                E11l = _mm_unpacklo_epi16(m128iS11, m128iS27);
                E12l = _mm_unpacklo_epi16(m128iS12, m128iS28);
                E13l = _mm_unpacklo_epi16(m128iS13, m128iS29);
                E14l = _mm_unpacklo_epi16(m128iS14, m128iS30);
                E15l = _mm_unpacklo_epi16(m128iS15, m128iS31);

                O0l = _mm_unpackhi_epi16(m128iS0, m128iS16);
                O1l = _mm_unpackhi_epi16(m128iS1, m128iS17);
                O2l = _mm_unpackhi_epi16(m128iS2, m128iS18);
                O3l = _mm_unpackhi_epi16(m128iS3, m128iS19);
                O4l = _mm_unpackhi_epi16(m128iS4, m128iS20);
                O5l = _mm_unpackhi_epi16(m128iS5, m128iS21);
                O6l = _mm_unpackhi_epi16(m128iS6, m128iS22);
                O7l = _mm_unpackhi_epi16(m128iS7, m128iS23);
                O8l = _mm_unpackhi_epi16(m128iS8, m128iS24);
                O9l = _mm_unpackhi_epi16(m128iS9, m128iS25);
                O10l = _mm_unpackhi_epi16(m128iS10, m128iS26);
                O11l = _mm_unpackhi_epi16(m128iS11, m128iS27);
                O12l = _mm_unpackhi_epi16(m128iS12, m128iS28);
                O13l = _mm_unpackhi_epi16(m128iS13, m128iS29);
                O14l = _mm_unpackhi_epi16(m128iS14, m128iS30);
                O15l = _mm_unpackhi_epi16(m128iS15, m128iS31);

                E0h  = _mm_unpacklo_epi16(E0l, E8l);
                E1h  = _mm_unpacklo_epi16(E1l, E9l);
                E2h = _mm_unpacklo_epi16(E2l, E10l);
                E3h  = _mm_unpacklo_epi16(E3l, E11l);
                E4h  = _mm_unpacklo_epi16(E4l, E12l);
                E5h  = _mm_unpacklo_epi16(E5l, E13l);
                E6h  = _mm_unpacklo_epi16(E6l, E14l);
                E7h  = _mm_unpacklo_epi16(E7l, E15l);

                E8h = _mm_unpackhi_epi16(E0l, E8l);
                E9h = _mm_unpackhi_epi16(E1l, E9l);
                E10h = _mm_unpackhi_epi16(E2l, E10l);
                E11h = _mm_unpackhi_epi16(E3l, E11l);
                E12h = _mm_unpackhi_epi16(E4l, E12l);
                E13h = _mm_unpackhi_epi16(E5l, E13l);
                E14h = _mm_unpackhi_epi16(E6l, E14l);
                E15h = _mm_unpackhi_epi16(E7l, E15l);


                m128Tmp0 = _mm_unpacklo_epi16(E0h, E4h);
                m128Tmp1 = _mm_unpacklo_epi16(E1h, E5h);
                m128Tmp2 = _mm_unpacklo_epi16(E2h, E6h);
                m128Tmp3 = _mm_unpacklo_epi16(E3h, E7h);

                m128Tmp4 = _mm_unpacklo_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpacklo_epi16(m128Tmp1, m128Tmp3);
                m128iS0  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS1  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                m128Tmp4 = _mm_unpackhi_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpackhi_epi16(m128Tmp1, m128Tmp3);
                m128iS2  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS3  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                m128Tmp0 = _mm_unpackhi_epi16(E0h, E4h);
                m128Tmp1 = _mm_unpackhi_epi16(E1h, E5h);
                m128Tmp2 = _mm_unpackhi_epi16(E2h, E6h);
                m128Tmp3 = _mm_unpackhi_epi16(E3h, E7h);

                m128Tmp4 = _mm_unpacklo_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpacklo_epi16(m128Tmp1, m128Tmp3);
                m128iS4  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS5  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                m128Tmp4 = _mm_unpackhi_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpackhi_epi16(m128Tmp1, m128Tmp3);
                m128iS6  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS7  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                m128Tmp0 = _mm_unpacklo_epi16(E8h, E12h);
                m128Tmp1 = _mm_unpacklo_epi16(E9h, E13h);
                m128Tmp2 = _mm_unpacklo_epi16(E10h, E14h);
                m128Tmp3 = _mm_unpacklo_epi16(E11h, E15h);

                m128Tmp4 = _mm_unpacklo_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpacklo_epi16(m128Tmp1, m128Tmp3);
                m128iS8  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS9  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                m128Tmp4 = _mm_unpackhi_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpackhi_epi16(m128Tmp1, m128Tmp3);
                m128iS10  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS11  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                m128Tmp0 = _mm_unpackhi_epi16(E8h, E12h);
                m128Tmp1 = _mm_unpackhi_epi16(E9h, E13h);
                m128Tmp2 = _mm_unpackhi_epi16(E10h, E14h);
                m128Tmp3 = _mm_unpackhi_epi16(E11h, E15h);

                m128Tmp4 = _mm_unpacklo_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpacklo_epi16(m128Tmp1, m128Tmp3);
                m128iS12  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS13  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                m128Tmp4 = _mm_unpackhi_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpackhi_epi16(m128Tmp1, m128Tmp3);
                m128iS14  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS15  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                /*  */
                E0h  = _mm_unpacklo_epi16(O0l, O8l);
                E1h  = _mm_unpacklo_epi16(O1l, O9l);
                E2h = _mm_unpacklo_epi16(O2l, O10l);
                E3h  = _mm_unpacklo_epi16(O3l, O11l);
                E4h  = _mm_unpacklo_epi16(O4l, O12l);
                E5h  = _mm_unpacklo_epi16(O5l, O13l);
                E6h  = _mm_unpacklo_epi16(O6l, O14l);
                E7h  = _mm_unpacklo_epi16(O7l, O15l);

                E8h = _mm_unpackhi_epi16(O0l, O8l);
                E9h = _mm_unpackhi_epi16(O1l, O9l);
                E10h = _mm_unpackhi_epi16(O2l, O10l);
                E11h = _mm_unpackhi_epi16(O3l, O11l);
                E12h = _mm_unpackhi_epi16(O4l, O12l);
                E13h = _mm_unpackhi_epi16(O5l, O13l);
                E14h = _mm_unpackhi_epi16(O6l, O14l);
                E15h = _mm_unpackhi_epi16(O7l, O15l);

                m128Tmp0 = _mm_unpacklo_epi16(E0h, E4h);
                m128Tmp1 = _mm_unpacklo_epi16(E1h, E5h);
                m128Tmp2 = _mm_unpacklo_epi16(E2h, E6h);
                m128Tmp3 = _mm_unpacklo_epi16(E3h, E7h);

                m128Tmp4 = _mm_unpacklo_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpacklo_epi16(m128Tmp1, m128Tmp3);
                m128iS16  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS17  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                m128Tmp4 = _mm_unpackhi_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpackhi_epi16(m128Tmp1, m128Tmp3);
                m128iS18  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS19  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                m128Tmp0 = _mm_unpackhi_epi16(E0h, E4h);
                m128Tmp1 = _mm_unpackhi_epi16(E1h, E5h);
                m128Tmp2 = _mm_unpackhi_epi16(E2h, E6h);
                m128Tmp3 = _mm_unpackhi_epi16(E3h, E7h);

                m128Tmp4 = _mm_unpacklo_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpacklo_epi16(m128Tmp1, m128Tmp3);
                m128iS20  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS21  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                m128Tmp4 = _mm_unpackhi_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpackhi_epi16(m128Tmp1, m128Tmp3);
                m128iS22  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS23  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                m128Tmp0 = _mm_unpacklo_epi16(E8h, E12h);
                m128Tmp1 = _mm_unpacklo_epi16(E9h, E13h);
                m128Tmp2 = _mm_unpacklo_epi16(E10h, E14h);
                m128Tmp3 = _mm_unpacklo_epi16(E11h, E15h);

                m128Tmp4 = _mm_unpacklo_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpacklo_epi16(m128Tmp1, m128Tmp3);
                m128iS24  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS25  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                m128Tmp4 = _mm_unpackhi_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpackhi_epi16(m128Tmp1, m128Tmp3);
                m128iS26  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS27  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                m128Tmp0 = _mm_unpackhi_epi16(E8h, E12h);
                m128Tmp1 = _mm_unpackhi_epi16(E9h, E13h);
                m128Tmp2 = _mm_unpackhi_epi16(E10h, E14h);
                m128Tmp3 = _mm_unpackhi_epi16(E11h, E15h);

                m128Tmp4 = _mm_unpacklo_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpacklo_epi16(m128Tmp1, m128Tmp3);
                m128iS28  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS29  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                m128Tmp4 = _mm_unpackhi_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpackhi_epi16(m128Tmp1, m128Tmp3);
                m128iS30  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS31  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);
                /*  */
                _mm_store_si128((__m128i*)( pSrc +      i ), m128iS0);
                _mm_store_si128((__m128i*)( pSrc + 32 + i ), m128iS1);
                _mm_store_si128((__m128i*)( pSrc + 64 + i ), m128iS2);
                _mm_store_si128((__m128i*)( pSrc + 96 + i ), m128iS3);
                _mm_store_si128((__m128i*)( pSrc + 128 + i ), m128iS4);
                _mm_store_si128((__m128i*)( pSrc + 160 + i ), m128iS5);
                _mm_store_si128((__m128i*)( pSrc + 192 + i ), m128iS6);
                _mm_store_si128((__m128i*)( pSrc + 224 + i), m128iS7);
                _mm_store_si128((__m128i*)( pSrc + 256 + i), m128iS8);
                _mm_store_si128((__m128i*)( pSrc + 288 + i ), m128iS9);
                _mm_store_si128((__m128i*)( pSrc + 320 + i), m128iS10);
                _mm_store_si128((__m128i*)( pSrc + 352 + i), m128iS11);
                _mm_store_si128((__m128i*)( pSrc + 384 + i ), m128iS12);
                _mm_store_si128((__m128i*)( pSrc + 416 + i), m128iS13);
                _mm_store_si128((__m128i*)( pSrc + 448 + i), m128iS14);
                _mm_store_si128((__m128i*)( pSrc + 480 + i ), m128iS15);
                _mm_store_si128((__m128i*)( pSrc + 512 +i ), m128iS16);
                _mm_store_si128((__m128i*)( pSrc + 544 + i ), m128iS17);
                _mm_store_si128((__m128i*)( pSrc + 576 + i ), m128iS18);
                _mm_store_si128((__m128i*)( pSrc + 608 + i ), m128iS19);
                _mm_store_si128((__m128i*)( pSrc + 640 + i ), m128iS20);
                _mm_store_si128((__m128i*)( pSrc + 672 + i ), m128iS21);
                _mm_store_si128((__m128i*)( pSrc + 704 + i ), m128iS22);
                _mm_store_si128((__m128i*)( pSrc + 736 + i), m128iS23);
                _mm_store_si128((__m128i*)( pSrc + 768 + i), m128iS24);
                _mm_store_si128((__m128i*)( pSrc + 800 + i ), m128iS25);
                _mm_store_si128((__m128i*)( pSrc + 832 + i), m128iS26);
                _mm_store_si128((__m128i*)( pSrc + 864 + i), m128iS27);
                _mm_store_si128((__m128i*)( pSrc + 896 + i ), m128iS28);
                _mm_store_si128((__m128i*)( pSrc + 928 + i), m128iS29);
                _mm_store_si128((__m128i*)( pSrc + 960 + i), m128iS30);
                _mm_store_si128((__m128i*)( pSrc + 992+ i ), m128iS31);

                if(i <= 16 ) {
                    int k = i+8;
                    m128iS0   = _mm_load_si128( (__m128i*)( pSrc + k ) );
                    m128iS1   = _mm_load_si128( (__m128i*)( pSrc + 32 + k) );
                    m128iS2   = _mm_load_si128( (__m128i*)( pSrc + 64 + k) );
                    m128iS3   = _mm_load_si128( (__m128i*)( pSrc + 96 + k) );
                    m128iS4   = _mm_load_si128( (__m128i*)( pSrc + 128 + k ) );
                    m128iS5   = _mm_load_si128( (__m128i*)( pSrc + 160 + k) );
                    m128iS6   = _mm_load_si128( (__m128i*)( pSrc + 192 +k) );
                    m128iS7   = _mm_load_si128( (__m128i*)( pSrc + 224 +k ) );
                    m128iS8   = _mm_load_si128( (__m128i*)( pSrc + 256 +k ) );
                    m128iS9   = _mm_load_si128( (__m128i*)( pSrc + 288 +k ));
                    m128iS10   = _mm_load_si128( (__m128i*)( pSrc + 320 + k ) );
                    m128iS11   = _mm_load_si128( (__m128i*)( pSrc + 352 + k));
                    m128iS12   = _mm_load_si128( (__m128i*)( pSrc + 384 +k ) );
                    m128iS13   = _mm_load_si128( (__m128i*)( pSrc + 416 + k) );
                    m128iS14   = _mm_load_si128( (__m128i*)( pSrc + 448 + k) );
                    m128iS15   = _mm_load_si128( (__m128i*)( pSrc + 480 + k) );

                    m128iS16   = _mm_load_si128( (__m128i*)( pSrc + 512 + k) );
                    m128iS17   = _mm_load_si128( (__m128i*)( pSrc + 544 + k) );
                    m128iS18   = _mm_load_si128( (__m128i*)( pSrc + 576 + k) );
                    m128iS19   = _mm_load_si128( (__m128i*)( pSrc + 608 + k) );
                    m128iS20   = _mm_load_si128( (__m128i*)( pSrc + 640 + k ) );
                    m128iS21   = _mm_load_si128( (__m128i*)( pSrc + 672 + k) );
                    m128iS22   = _mm_load_si128( (__m128i*)( pSrc + 704 + k) );
                    m128iS23   = _mm_load_si128( (__m128i*)( pSrc + 736 + k ) );
                    m128iS24   = _mm_load_si128( (__m128i*)( pSrc + 768 + k ) );
                    m128iS25   = _mm_load_si128( (__m128i*)( pSrc + 800 + k ));
                    m128iS26   = _mm_load_si128( (__m128i*)( pSrc + 832 + k ) );
                    m128iS27   = _mm_load_si128( (__m128i*)( pSrc + 864 + k));
                    m128iS28   = _mm_load_si128( (__m128i*)( pSrc + 896 + k ) );
                    m128iS29   = _mm_load_si128( (__m128i*)( pSrc + 928 + k) );
                    m128iS30   = _mm_load_si128( (__m128i*)( pSrc + 960 + k) );
                    m128iS31   = _mm_load_si128( (__m128i*)( pSrc + 992 + k) );
                } else {
                    m128iS0   = _mm_load_si128( (__m128i*)( pSrc) );
                    m128iS1   = _mm_load_si128( (__m128i*)( pSrc + 128) );
                    m128iS2   = _mm_load_si128( (__m128i*)( pSrc + 256 ) );
                    m128iS3   = _mm_load_si128( (__m128i*)( pSrc + 384 ) );
                    m128iS4   = _mm_loadu_si128((__m128i*)( pSrc  + 512 ) );
                    m128iS5   = _mm_load_si128( (__m128i*)( pSrc + 640 ) );
                    m128iS6   = _mm_load_si128( (__m128i*)( pSrc  + 768) );
                    m128iS7   = _mm_load_si128( (__m128i*)( pSrc + 896) );
                    m128iS8   = _mm_load_si128( (__m128i*)( pSrc + 8) );
                    m128iS9   = _mm_load_si128( (__m128i*)( pSrc + 128 +8));
                    m128iS10  = _mm_load_si128( (__m128i*)( pSrc + 256  +8 ) );
                    m128iS11  = _mm_load_si128( (__m128i*)( pSrc + 384 +8));
                    m128iS12  = _mm_loadu_si128((__m128i*)( pSrc + 512 +8) );
                    m128iS13  = _mm_load_si128( (__m128i*)( pSrc + 640 +8) );
                    m128iS14  = _mm_load_si128( (__m128i*)( pSrc + 768 +8) );
                    m128iS15  = _mm_load_si128( (__m128i*)( pSrc + 896 +8) );
                    m128iS16  = _mm_load_si128( (__m128i*)( pSrc + 16) );
                    m128iS17  = _mm_load_si128( (__m128i*)( pSrc + 128 +16));
                    m128iS18  = _mm_load_si128( (__m128i*)( pSrc + 256  +16 ) );
                    m128iS19  = _mm_load_si128( (__m128i*)( pSrc + 384 +16));
                    m128iS20  = _mm_loadu_si128((__m128i*)( pSrc + 512 +16) );
                    m128iS21  = _mm_load_si128( (__m128i*)( pSrc + 640 +16) );
                    m128iS22  = _mm_load_si128( (__m128i*)( pSrc + 768 +16) );
                    m128iS23  = _mm_load_si128( (__m128i*)( pSrc + 896 +16) );
                    m128iS24  = _mm_load_si128( (__m128i*)( pSrc + 24) );
                    m128iS25  = _mm_load_si128( (__m128i*)( pSrc + 128 +24));
                    m128iS26  = _mm_load_si128( (__m128i*)( pSrc + 256  +24 ) );
                    m128iS27  = _mm_load_si128( (__m128i*)( pSrc + 384 +24));
                    m128iS28  = _mm_loadu_si128((__m128i*)( pSrc + 512 +24) );
                    m128iS29  = _mm_load_si128( (__m128i*)( pSrc + 640 +24) );
                    m128iS30  = _mm_load_si128( (__m128i*)( pSrc + 768 +24) );
                    m128iS31  = _mm_load_si128( (__m128i*)( pSrc + 896 +24) );
                    shift = 12;
                    m128iAdd  = _mm_set1_epi32( 2048 );
                }

            } else {
                __m128i T00, T01, T02, T03;
                __m128i T10, T11;

#define STORE_4x8(_COL, A, B, C, D) \
                T00 = _mm_unpacklo_epi16((A), (B)); \
                T01 = _mm_unpackhi_epi16((A), (B)); \
                T02 = _mm_unpacklo_epi16((C), (D)); \
                T03 = _mm_unpackhi_epi16((C), (D)); \
                T10 = _mm_unpacklo_epi32(T00, T02); \
                T11 = _mm_unpackhi_epi32(T00, T02); \
                _mm_storel_epi64( (__m128i*)&pDst[(i+0)*stride + (_COL)], T10 ); \
                _mm_storeh_pi   ( (__m64*  )&pDst[(i+1)*stride + (_COL)], _mm_castsi128_ps(T10)); \
                _mm_storel_epi64( (__m128i*)&pDst[(i+2)*stride + (_COL)], T11 ); \
                _mm_storeh_pi   ( (__m64*  )&pDst[(i+3)*stride + (_COL)], _mm_castsi128_ps(T11)); \
                T10 = _mm_unpacklo_epi32(T01, T03); \
                T11 = _mm_unpackhi_epi32(T01, T03); \
                _mm_storel_epi64( (__m128i*)&pDst[(i+4)*stride + (_COL)], T10 ); \
                _mm_storeh_pi   ( (__m64*  )&pDst[(i+5)*stride + (_COL)], _mm_castsi128_ps(T10)); \
                _mm_storel_epi64( (__m128i*)&pDst[(i+6)*stride + (_COL)], T11 ); \
                _mm_storeh_pi   ( (__m64*  )&pDst[(i+7)*stride + (_COL)], _mm_castsi128_ps(T11)); \


                STORE_4x8( 0, m128iS0,  m128iS1,  m128iS2,  m128iS3);
                STORE_4x8( 4, m128iS4,  m128iS5,  m128iS6,  m128iS7);
                STORE_4x8( 8, m128iS8,  m128iS9,  m128iS10, m128iS11);
                STORE_4x8(12, m128iS12, m128iS13, m128iS14, m128iS15);
                STORE_4x8(16, m128iS16, m128iS17, m128iS18, m128iS19);
                STORE_4x8(20, m128iS20, m128iS21, m128iS22, m128iS23);
                STORE_4x8(24, m128iS24, m128iS25, m128iS26, m128iS27);
                STORE_4x8(28, m128iS28, m128iS29, m128iS30, m128iS31);
#undef STORE_4x8

                if(i<=16){
                    int k = (i+8)*4;
                    m128iS0   = _mm_load_si128( (__m128i*)( pSrc + k) );
                    m128iS1   = _mm_load_si128( (__m128i*)( pSrc + 128 + k) );
                    m128iS2   = _mm_load_si128( (__m128i*)( pSrc + 256 + k ) );
                    m128iS3   = _mm_load_si128( (__m128i*)( pSrc + 384 + k ) );
                    m128iS4   = _mm_loadu_si128((__m128i*)( pSrc + 512 + k ) );
                    m128iS5   = _mm_load_si128( (__m128i*)( pSrc + 640 + k ) );
                    m128iS6   = _mm_load_si128( (__m128i*)( pSrc + 768 + k) );
                    m128iS7   = _mm_load_si128( (__m128i*)( pSrc + 896 + k) );
                    m128iS8   = _mm_load_si128( (__m128i*)( pSrc + 8 + k) );
                    m128iS9   = _mm_load_si128( (__m128i*)( pSrc + 128 +8 + k));
                    m128iS10  = _mm_load_si128( (__m128i*)( pSrc + 256  +8  + k) );
                    m128iS11  = _mm_load_si128( (__m128i*)( pSrc + 384 +8 + k));
                    m128iS12  = _mm_loadu_si128((__m128i*)( pSrc + 512 +8 + k) );
                    m128iS13  = _mm_load_si128( (__m128i*)( pSrc + 640 +8 + k) );
                    m128iS14  = _mm_load_si128( (__m128i*)( pSrc + 768 +8 + k) );
                    m128iS15  = _mm_load_si128( (__m128i*)( pSrc + 896 +8 + k) );
                    m128iS16  = _mm_load_si128( (__m128i*)( pSrc + 16 + k) );
                    m128iS17  = _mm_load_si128( (__m128i*)( pSrc + 128 +16 + k));
                    m128iS18  = _mm_load_si128( (__m128i*)( pSrc + 256  +16 + k) );
                    m128iS19  = _mm_load_si128( (__m128i*)( pSrc + 384 +16 + k));
                    m128iS20  = _mm_loadu_si128((__m128i*)( pSrc + 512 +16 + k) );
                    m128iS21  = _mm_load_si128( (__m128i*)( pSrc + 640 +16 + k) );
                    m128iS22  = _mm_load_si128( (__m128i*)( pSrc + 768 +16 + k) );
                    m128iS23  = _mm_load_si128( (__m128i*)( pSrc + 896 +16 + k) );
                    m128iS24  = _mm_load_si128( (__m128i*)( pSrc + 24 + k) );
                    m128iS25  = _mm_load_si128( (__m128i*)( pSrc + 128 +24 + k));
                    m128iS26  = _mm_load_si128( (__m128i*)( pSrc + 256  +24  + k) );
                    m128iS27  = _mm_load_si128( (__m128i*)( pSrc + 384 +24 + k));
                    m128iS28  = _mm_loadu_si128((__m128i*)( pSrc + 512 +24 + k) );
                    m128iS29  = _mm_load_si128( (__m128i*)( pSrc + 640 +24 + k) );
                    m128iS30  = _mm_load_si128( (__m128i*)( pSrc + 768 +24 + k) );
                    m128iS31  = _mm_load_si128( (__m128i*)( pSrc + 896 +24 + k) );
                }
            }
        }
    }
}
}

#include "utils.h"

namespace x265 {
// private x265 namespace

void NAME(Setup_Vec_DCTPrimitives)(EncoderPrimitives &p)
{
    p.inversedst = inversedst;

    p.partial_butterfly[BUTTERFLY_4] = partialButterfly4;
    p.partial_butterfly[BUTTERFLY_8] = partialButterfly8;
    p.partial_butterfly[BUTTERFLY_16] = partialButterfly16;
    p.partial_butterfly[BUTTERFLY_32] = partialButterfly32;
    p.partial_butterfly[BUTTERFLY_INVERSE_4] = partialButterflyInverse4;
    p.partial_butterfly[BUTTERFLY_INVERSE_8] = partialButterflyInverse8;
    p.partial_butterfly[BUTTERFLY_INVERSE_16] = partialButterflyInverse16;
    p.partial_butterfly[BUTTERFLY_INVERSE_32] = partialButterflyInverse32;

    p.deQuant = xDeQuant;
    p.dct[DCT_4x4] = xDCT4;
    p.dct[DCT_8x8] = xDCT8;
    p.dct[DCT_16x16] = xDCT16;
    p.dct[IDST_4x4] = xIDST4;
    p.dct[IDCT_4x4] = xIDCT4;
    p.dct[IDCT_8x8] = xIDCT8;
    p.dct[IDCT_16x16] = xIDCT16;
    p.dct[IDCT_32x32] = xIDCT32;
}
}
