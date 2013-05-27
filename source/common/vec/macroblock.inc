/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Mandar Gurav <mandar@multicorewareinc.com>
 *          Deepthi Devaki Akkoorath <deepthidevaki@multicorewareinc.com>
 *          Mahesh Pittala <mahesh@multicorewareinc.com>
 *          Rajesh Paulraj <rajesh@multicorewareinc.com>
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

/* Used for filter */
#define IF_INTERNAL_PREC 14 ///< Number of bits for internal precision
#define IF_FILTER_PREC    6 ///< Log2 of sum of filter taps
#define IF_INTERNAL_OFFS (1 << (IF_INTERNAL_PREC - 1)) ///< Offset used internally

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

#if INSTRSET <= 4    //partialButterfly32 vector code

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

#if INSTRSET <= 4    //partialButterfly8 vector code

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

#endif  //partialButterfly8 vector code

#if INSTRSET > 4 //partialButterfly8 intrinsic code

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

#if 0   //partialButterfly4 vector code

void CDECL partialButterfly4(short *src, short *dst, int shift, int line)
{
    int j;

    Vec4i g_aiT8_zero_row(64, 64, 0, 0);
    Vec4i g_aiT8_two_row(64, -64, 0, 0);
    Vec4i g_aiT8_one_row(83, 36, 0, 0);
    Vec4i g_aiT8_three_row(36, -83, 0, 0);

    int add = 1 << (shift - 1);

    for (j = 0; j < line; j++)
    {
        // E and O
        int E0 = src[0] + src[3];
        int O0 = src[0] - src[3];
        int E1 = src[1] + src[2];
        int O1 = src[1] - src[2];

        Vec4i E_vec(E0, E1, 0, 0);

        Vec4i O_vec(O0, O1, 0, 0);

        int tmpE1 = horizontal_add(g_aiT8_zero_row * E_vec);
        int tmpO1 = horizontal_add(g_aiT8_one_row * O_vec);
        int tmpE2 = horizontal_add(g_aiT8_two_row * E_vec);
        int tmpO2 = horizontal_add(g_aiT8_three_row * O_vec);

        short dst0 = (short)((tmpE1 + add) >> shift);
        short dst2 = (short)((tmpE2 + add) >> shift);
        short dst_line = (short)((tmpO1 + add) >> shift);
        short dst3 = (short)((tmpO2 + add) >> shift);

        dst[0] = dst0;
        dst[2 * line] = dst2;
        dst[line] = dst_line;
        dst[3 * line] = dst3;

        src += 4;
        dst++;
    }
}

#endif  //partialButterfly4 vector code

#if INSTRSET > 4   //partialButterfly32 intrisic code

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

        dst[4 * line] = (short)_mm_extract_epi32(dst_4_12_20_28, 0);
        dst[12 * line] = (short)_mm_extract_epi32(dst_4_12_20_28, 1);
        dst[20 * line] = (short)_mm_extract_epi32(dst_4_12_20_28, 2);
        dst[28 * line] = (short)_mm_extract_epi32(dst_4_12_20_28, 3);

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

        dst[2 * line] = (short)_mm_extract_epi32(dst_2_6_10_14, 0);
        dst[6 * line] = (short)_mm_extract_epi32(dst_2_6_10_14, 1);
        dst[10 * line] = (short)_mm_extract_epi32(dst_2_6_10_14, 2);
        dst[14 * line] = (short)_mm_extract_epi32(dst_2_6_10_14, 3);

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

        dst[18 * line] = (short)_mm_extract_epi32(dst_18_22_26_30, 0);
        dst[22 * line] = (short)_mm_extract_epi32(dst_18_22_26_30, 1);
        dst[26 * line] = (short)_mm_extract_epi32(dst_18_22_26_30, 2);
        dst[30 * line] = (short)_mm_extract_epi32(dst_18_22_26_30, 3);

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

        dst[1 * line] = (short)_mm_extract_epi32(dst_1_3_5_7, 0);
        dst[3 * line] = (short)_mm_extract_epi32(dst_1_3_5_7, 1);
        dst[5 * line] = (short)_mm_extract_epi32(dst_1_3_5_7, 2);
        dst[7 * line] = (short)_mm_extract_epi32(dst_1_3_5_7, 3);

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

        dst[9 * line] = (short)_mm_extract_epi32(dst_9_11_13_15, 0);
        dst[11 * line] = (short)_mm_extract_epi32(dst_9_11_13_15, 1);
        dst[13 * line] = (short)_mm_extract_epi32(dst_9_11_13_15, 2);
        dst[15 * line] = (short)_mm_extract_epi32(dst_9_11_13_15, 3);

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

        dst[17 * line] = (short)_mm_extract_epi32(dst_17_19_21_23, 0);
        dst[19 * line] = (short)_mm_extract_epi32(dst_17_19_21_23, 1);
        dst[21 * line] = (short)_mm_extract_epi32(dst_17_19_21_23, 2);
        dst[23 * line] = (short)_mm_extract_epi32(dst_17_19_21_23, 3);

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

        dst[25 * line] = (short)_mm_extract_epi32(dst_25_27_29_31, 0);
        dst[27 * line] = (short)_mm_extract_epi32(dst_25_27_29_31, 1);
        dst[29 * line] = (short)_mm_extract_epi32(dst_25_27_29_31, 2);
        dst[31 * line] = (short)_mm_extract_epi32(dst_25_27_29_31, 3);

        src += 32;
        dst++;
    }
}

#endif    // partialButterfly32 intrinsic code

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

void Setup_Vec_MacroblockPrimitives(EncoderPrimitives &p)
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
}
