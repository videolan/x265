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

template<bool isFirst, bool isLast>
void CDECL filter_Horizontal_8(const short *coeff,
                               pixel *      src,
                               int          srcStride,
                               pixel *      dst,
                               int          dstStride,
                               int          block_width,
                               int          block_height,
                               int          bitDepth)
{
    int row, col;
    short *src_s = (short*)src;
    short *dst_s = (short*)dst;

    src_s -= (8 / 2 - 1);     // Here N = 8 and cStride = 1
    int offset;
    short maxVal;
    int headRoom = IF_INTERNAL_PREC - bitDepth;
    int shift = IF_FILTER_PREC;
    if (isLast)
    {
        shift += (isFirst) ? 0 : headRoom;
        offset = 1 << (shift - 1);
        offset += (isFirst) ? 0 : IF_INTERNAL_OFFS << IF_FILTER_PREC;
        maxVal = (1 << bitDepth) - 1;
    }
    else
    {
        shift -= (isFirst) ? headRoom : 0;
        offset = (isFirst) ? -IF_INTERNAL_OFFS << shift : 0;
        maxVal = 0;
    }

    Vec8s vec_c, vec_src1;
    vec_c.load(coeff);
    int sum = 0;
    for (row = 0; row < block_height; row++)
    {
        for (col = 0; col < block_width; col++)
        {
            vec_src1.load(src_s + col);
            vec_src1 = vec_src1 * vec_c;            // Assuming that there is no overflow
            sum = horizontal_add_x(vec_src1);
            short val = (short)(sum + offset) >> shift;
            if (isLast)
            {
                val = (val < 0) ? 0 : val;
                val = (val > maxVal) ? maxVal : val;
            }

            dst_s[col] = val;
        }

        src_s += srcStride;
        dst_s += dstStride;
    }
}

/*
    Vertical Filter using 16-bit operations. It requires that input contains only 8 bit values. Very good performance but with less precision.
    This assumes isFirst=isLast=1.
*/
template<int N>
void CDECL filter_Vertical_16bit(const short *coeff, pixel *src, int srcStride, pixel *dst, int dststride, int block_width,
                                 int block_height, int bitDepth)
{
    int row, col;

    int cStride = srcStride;
    pixel *src_short = src - (N / 2 - 1) * srcStride;

    int offset;
    short maxVal;
    int shift = IF_FILTER_PREC;

    offset = 1 << (shift - 1);
    maxVal = (1 << bitDepth) - 1;

    Vec8s c0(coeff[0]), c1(coeff[1]), c2(coeff[2]), c3(coeff[3]), c4(coeff[4]), c5(coeff[5]), c6(coeff[6]), c7(coeff[7]);    //vectors for coeff[i]
    for (row = 0; row < block_height; row++)
    {
        for (col = 0; col < block_width - 7; col += 8)
        {
            Vec8s row0, row1, row2, row3, row4, row5, row6, row7, sum;

            row0.load(&src_short[col]);
            row1.load(&src_short[col + cStride]);

            row2.load(&src_short[col + 2 * cStride]);
            row3.load(&src_short[col + 3 * cStride]);

            row0 = row0 * c0;
            row1 = row1 * c1;

            sum = add_saturated(row0, row1);

            row2 = row2 * c2;
            row3 = row3 * c3;

            sum = add_saturated(sum, add_saturated(row2, row3));

            if (N == 8)
            {
                row4.load(&src_short[col + 4 * cStride]);
                row5.load(&src_short[col + 5 * cStride]);

                row6.load(&src_short[col + 6 * cStride]);
                row7.load(&src_short[col + 7 * cStride]);

                row4 = row4 * c4;
                row5 = row5 * c5;

                sum = add_saturated(sum, add_saturated(row4, row5));

                row6 = row6 * c6;
                row7 = row7 * c7;

                sum = add_saturated(sum, add_saturated(row6, row7));
            }

            sum = add_saturated(sum, offset) >> shift;

            sum = max(sum, 0);
            Vec8s maxVal_v(maxVal);
            sum = min(sum, maxVal_v);

            sum.store(dst + col);
        }

        //Handle the case when block_width is not a multiple of 8
        if (col < block_width)
        {
            Vec8s row0, row1, row2, row3, row4, row5, row6, row7, sum;

            row0.load(&src_short[col]);
            row1.load(&src_short[col + cStride]);

            row2.load(&src_short[col + 2 * cStride]);
            row3.load(&src_short[col + 3 * cStride]);

            row0 = row0 * c0;
            row1 = row1 * c1;

            sum = add_saturated(row0, row1);

            row2 = row2 * c2;
            row3 = row3 * c3;

            sum = add_saturated(sum, add_saturated(row2, row3));

            if (N == 8)
            {
                row4.load(&src_short[col + 4 * cStride]);
                row5.load(&src_short[col + 5 * cStride]);

                row6.load(&src_short[col + 6 * cStride]);
                row7.load(&src_short[col + 7 * cStride]);

                row4 = row4 * c4;
                row5 = row5 * c5;

                sum = add_saturated(sum, add_saturated(row4, row5));

                row6 = row6 * c6;
                row7 = row7 * c7;

                sum = add_saturated(sum, add_saturated(row6, row7));
            }

            sum = add_saturated(sum, offset) >> shift;

            sum = max(sum, 0);
            Vec8s maxVal_v(maxVal);
            sum = min(sum, maxVal_v);

            sum.store_partial(block_width - col, dst + col);
        }

        src_short += srcStride;
        dst += dststride;
    }
}

template<int N, bool isFirst, bool isLast>
void CDECL filter_Vertical(const short *coeff, pixel *src, int srcstride, pixel *dst, int dststride, int block_width,
                           int block_height, int bitdepth)
{
    int row, col;

    short* src_short = (short*)src;
    short* dst_short = (short*)dst;

    int cstride =  srcstride;

    src_short -= (N / 2 - 1) * cstride;

    int offset;
    short maxVal;
    int headroom = IF_INTERNAL_PREC - bitdepth;
    int shift = IF_FILTER_PREC;
    if (isLast)
    {
        shift += (isFirst) ? 0 : headroom;
        offset = 1 << (shift - 1);
        offset += (isFirst) ? 0 : IF_INTERNAL_OFFS << IF_FILTER_PREC;
        maxVal = (1 << bitdepth) - 1;
    }
    else
    {
        shift -= (isFirst) ? headroom : 0;
        offset = (isFirst) ?  -IF_INTERNAL_OFFS << shift : 0;
        maxVal = 0;
    }

    int cm[8][4];
    for (int i = 0; i < N; i++)
    {
        cm[i][0] = coeff[i];
        cm[i][1] = coeff[i];
        cm[i][2] = coeff[i];
        cm[i][3] = coeff[i];
    }

    for (row = 0; row < block_height; row++)
    {
        for (col = 0; col < block_width - 7; col += 8)
        {
            Vec8s row0, row1, row2, row3, row4, row5, row6, row7, sum;
            Vec4i row0_first, row0_last, row1_first, row1_last, sum_first, sum_last;
            Vec4i c0, c1, c2, c3, c4, c5, c6, c7;

            row0.load(&src_short[col]);
            row1.load(&src_short[col + cstride]);

            c0.load(cm[0]);
            c1.load(cm[1]);

            row0_first = extend_low(row0);
            row1_first = extend_low(row1);
            row0_last = extend_high(row0);
            row1_last = extend_high(row1);

            row0_first = row0_first * c0;
            row1_first = row1_first * c1;
            row0_last = row0_last * c0;
            row1_last = row1_last * c1;

            sum_first = row0_first + row1_first;
            sum_last = row0_last + row1_last;

            row2.load(&src_short[col + 2 * cstride]);
            row3.load(&src_short[col + 3 * cstride]);

            c2.load(cm[2]);
            c3.load(cm[3]);

            row0_first = extend_low(row2);
            row0_last = extend_high(row2);
            row0_first = row0_first * c2;
            row0_last = row0_last * c2;
            row1_first = extend_low(row3);
            row1_last = extend_high(row3);
            row1_first = row1_first * c3;
            row1_last = row1_last * c3;
            sum_first += row0_first + row1_first;
            sum_last += row0_last + row1_last;

            if (N == 8)
            {
                row4.load(&src_short[col + 4 * cstride]);
                row5.load(&src_short[col + 5 * cstride]);

                c4.load(cm[4]);
                c5.load(cm[5]);

                row0_first = extend_low(row4);
                row0_last = extend_high(row4);
                row0_first = row0_first * c4;
                row0_last = row0_last * c4;
                row1_first = extend_low(row5);
                row1_last = extend_high(row5);
                row1_first = row1_first * c5;
                row1_last = row1_last * c5;
                sum_first += row0_first + row1_first;
                sum_last += row0_last + row1_last;

                row6.load(&src_short[col + 6 * cstride]);
                row7.load(&src_short[col + 7 * cstride]);

                c6.load(cm[6]);
                c7.load(cm[7]);

                row0_first = extend_low(row6);
                row0_last = extend_high(row6);
                row0_first = row0_first * c6;
                row0_last = row0_last * c6;
                row1_first = extend_low(row7);
                row1_last = extend_high(row7);
                row1_first = row1_first * c7;
                row1_last = row1_last * c7;
                sum_first += row0_first + row1_first;
                sum_last += row0_last + row1_last;
            }

            sum_first = (sum_first + offset)  >> shift;
            sum_last = (sum_last + offset)  >> shift;

            Vec4i zero(0);
            sum = compress(sum_first, sum_last);

            if (isLast)
            {
                sum = max(sum, 0);
                Vec8s maxVal_v(maxVal);
                sum = min(sum, maxVal_v);
            }

            sum.store(dst_short + col);
        }

        //Handle the case when block_width is not multiple of 8
        for (; col < block_width; col += 4)
        {
            Vec8s row0, row1, row2, row3, row4, row5, row6, row7, sum;
            Vec4i row0_first, row0_last, row1_first, row1_last, sum_first, sum_last;
            Vec4i c0, c1, c2, c3, c4, c5, c6, c7;

            row0.load(&src_short[col]);
            row1.load(&src_short[col + cstride]);

            c0.load(cm[0]);
            c1.load(cm[1]);

            row0_first = extend_low(row0);
            row1_first = extend_low(row1);
            row0_first = row0_first * c0;
            row1_first = row1_first * c1;

            sum_first = row0_first + row1_first;

            row2.load(&src_short[col + 2 * cstride]);
            row3.load(&src_short[col + 3 * cstride]);

            c2.load(cm[2]);
            c3.load(cm[3]);

            row0_first = extend_low(row2);
            row0_first = row0_first * c2;
            row1_first = extend_low(row3);
            row1_first = row1_first * c3;
            sum_first += row0_first + row1_first;

            if (N == 8)
            {
                row4.load(&src_short[col + 4 * cstride]);
                row5.load(&src_short[col + 5 * cstride]);

                c4.load(cm[4]);
                c5.load(cm[5]);

                row0_first = extend_low(row4);
                row0_first = row0_first * c4;
                row1_first = extend_low(row5);
                row1_first = row1_first * c5;
                sum_first += row0_first + row1_first;

                row6.load(&src_short[col + 6 * cstride]);
                row7.load(&src_short[col + 7 * cstride]);

                c6.load(cm[6]);
                c7.load(cm[7]);

                row0_first = extend_low(row6);
                row0_first = row0_first * c6;
                row1_first = extend_low(row7);
                row1_first = row1_first * c7;
                sum_first += row0_first + row1_first;
            }

            sum_first = (sum_first + offset)  >> shift;

            Vec4i zero(0);
            sum = compress(sum_first, zero);

            if (isLast)
            {
                sum = max(sum, 0);
                Vec8s maxVal_v(maxVal);
                sum = min(sum, maxVal_v);
            }

            sum.store_partial(block_width - col, dst_short + col);
        }

        src_short += srcstride;
        dst_short += dststride;
    }
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

void Setup_Vec_MacroblockPrimitives(EncoderPrimitives &p)
{
    p.inversedst = inversedst;

    /*p.filter[FILTER_H_4_0_0] = filter_Horizontal_4< 0, 0>;
      p.filter[FILTER_H_4_0_1] = filter_Horizontal_4< 0, 1>;
      p.filter[FILTER_H_4_1_0] = filter_Horizontal_4< 1, 0>;
      p.filter[FILTER_H_4_1_1] = filter_Horizontal_4< 1, 1>;*/

    /* p.filter[FILTER_H_8_0_0] = filter_Horizontal_8<0, 0>;
     p.filter[FILTER_H_8_0_1] = filter_Horizontal_8<0, 1>;
     p.filter[FILTER_H_8_1_0] = filter_Horizontal_8<1, 0>;
     p.filter[FILTER_H_8_1_1] = filter_Horizontal_8<1, 1>;*/

    p.filter[FILTER_V_4_0_0] = filter_Vertical<4, 0, 0>;
    p.filter[FILTER_V_4_0_1] = filter_Vertical<4, 0, 1>;
    p.filter[FILTER_V_4_1_0] = filter_Vertical<4, 1, 0>;
    p.filter[FILTER_V_4_1_1] = filter_Vertical<4, 1, 1>;

    p.filter[FILTER_V_8_0_0] = filter_Vertical<8, 0, 0>;
    p.filter[FILTER_V_8_0_1] = filter_Vertical<8, 0, 1>;
    p.filter[FILTER_V_8_1_0] = filter_Vertical<8, 1, 0>;
    p.filter[FILTER_V_8_1_1] = filter_Vertical<8, 1, 1>;

    p.partial_butterfly[BUTTERFLY_16] = partialButterfly16;
}
