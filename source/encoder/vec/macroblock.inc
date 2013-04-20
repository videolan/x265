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
                               short *      src,
                               int          srcStride,
                               short *      dst,
                               int          dstStride,
                               int          block_width,
                               int          block_height,
                               int          bitDepth)
{
    int row, col;
    short *src_s = (short*)src;
    short *dst_s = (short*)dst;

    src_s -= (8 / 2 - 1);                                   // Here N = 8 and cStride = 1
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

    Vec4i vec_sum_low, vec_sum_high;
    Vec8s vec_src0, vec_sum, vec_c;
    vec_c.load(coeff);
    Vec8s vec_c0(coeff[0]), vec_c1(coeff[1]), vec_c2(coeff[2]), vec_c3(coeff[3]), vec_c4(coeff[4]), vec_c5(coeff[5]), vec_c6(
        coeff[6]), vec_c7(coeff[7]);
    Vec4i vec_offset(offset);
    Vec8s vec_maxVal(maxVal);
    for (row = 0; row < block_height; row++)
    {
        col = 0;
        for (; col < (block_width - 7); col += 8)                   // Iterations multiple of 8
        {
            vec_src0.load(src_s + col);                         // Load the 8 elements
            vec_src0 = vec_src0 * vec_c0;                       // Multiply by c[0]
            vec_sum_low = extend_low(vec_src0);                 // Convert to integer lower bits
            vec_sum_high = extend_high(vec_src0);               // Convert to integer higher bits

            vec_src0.load(src_s + col + 1);                     // Load the 8 elements
            vec_src0 = vec_src0 * vec_c1;                       // Multiply by c[1]
            vec_sum_low += extend_low(vec_src0);                // Add integer lower bits to sum_low bits
            vec_sum_high += extend_high(vec_src0);              // Add integer higer bits to sum_high bits

            vec_src0.load(src_s + col + 2);                     // Load the 8 elements
            vec_src0 = vec_src0 * vec_c2;                       // Multiply by c[2]
            vec_sum_low += extend_low(vec_src0);                // Add integer lower bits to sum_low bits
            vec_sum_high += extend_high(vec_src0);              // Add integer higer bits to sum_high bits

            vec_src0.load(src_s + col + 3);                     // Load the 8 elements
            vec_src0 = vec_src0 * vec_c3;                       // Multiply by c[2]
            vec_sum_low += extend_low(vec_src0);                // Add integer lower bits to sum_low bits
            vec_sum_high += extend_high(vec_src0);              // Add integer higer bits to sum_high bits

            vec_src0.load(src_s + col + 4);                     // Load the 8 elements
            vec_src0 = vec_src0 * vec_c4;                       // Multiply by c[2]
            vec_sum_low += extend_low(vec_src0);                // Add integer lower bits to sum_low bits
            vec_sum_high += extend_high(vec_src0);              // Add integer higer bits to sum_high bits

            vec_src0.load(src_s + col + 5);                     // Load the 8 elements
            vec_src0 = vec_src0 * vec_c5;                       // Multiply by c[2]
            vec_sum_low += extend_low(vec_src0);                // Add integer lower bits to sum_low bits
            vec_sum_high += extend_high(vec_src0);              // Add integer higer bits to sum_high bits

            vec_src0.load(src_s + col + 6);                     // Load the 8 elements
            vec_src0 = vec_src0 * vec_c6;                       // Multiply by c[2]
            vec_sum_low += extend_low(vec_src0);                // Add integer lower bits to sum_low bits
            vec_sum_high += extend_high(vec_src0);              // Add integer higer bits to sum_high bits

            vec_src0.load(src_s + col + 7);                     // Load the 8 elements
            vec_src0 = vec_src0 * vec_c7;                       // Multiply by c[2]
            vec_sum_low += extend_low(vec_src0);                // Add integer lower bits to sum_low bits
            vec_sum_high += extend_high(vec_src0);              // Add integer higer bits to sum_high bits

            vec_sum_low = (vec_sum_low + vec_offset);           // Add offset(value copied into all integer vector elements) to sum_low
            vec_sum_high = (vec_sum_high + vec_offset);         // Add offset(value copied into all integer vector elements) to sum_high

            vec_sum = compress(vec_sum_low, vec_sum_high);       // Save two integer vectors(Vec4i) to single short vector(Vec8s)
            vec_sum = vec_sum >> shift;                         // This shift must be done after saving integer(two vec4i) data to short(Vec8s)

            if (isLast)
            {
                vec_sum = max(vec_sum, 0);                      // (val < 0) ? 0 : val;
                vec_sum = min(vec_sum, vec_maxVal);             // (val > maxVal) ? maxVal : val;
            }

            vec_sum.store(dst_s + col);                           // Store vector
        }

        for (; col < block_width; col++)                           // Remaining iterations
        {
            vec_src0.load(src_s + col);
            vec_src0 = vec_src0 * vec_c;                        // Assuming that there is no overflow (Everywhere in this function!)
            int sum = horizontal_add(vec_src0);
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
void CDECL filter_Vertical_16bit(const short *coeff, short *src, int srcStride, short *dst, int dststride, int block_width,
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
void CDECL filter_Vertical(const short *coeff, short *src, int srcstride, short *dst, int dststride, int block_width,
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

void Setup_Vec_MacroblockPrimitives(EncoderPrimitives &p)
{
    p.inversedst = inversedst;

    /*p.filter[FILTER_H_4_0_0] = filter_Horizontal_4< 0, 0>;
      p.filter[FILTER_H_4_0_1] = filter_Horizontal_4< 0, 1>;
      p.filter[FILTER_H_4_1_0] = filter_Horizontal_4< 1, 0>;
      p.filter[FILTER_H_4_1_1] = filter_Horizontal_4< 1, 1>;*/

    p.filter[FILTER_H_8_0_0] = filter_Horizontal_8<0, 0>;
    p.filter[FILTER_H_8_0_1] = filter_Horizontal_8<0, 1>;
    p.filter[FILTER_H_8_1_0] = filter_Horizontal_8<1, 0>;
    p.filter[FILTER_H_8_1_1] = filter_Horizontal_8<1, 1>;

    p.filter[FILTER_V_4_0_0] = filter_Vertical<4, 0, 0>;
    p.filter[FILTER_V_4_0_1] = filter_Vertical<4, 0, 1>;
    p.filter[FILTER_V_4_1_0] = filter_Vertical<4, 1, 0>;
    p.filter[FILTER_V_4_1_1] = filter_Vertical<4, 1, 1>;

    p.filter[FILTER_V_8_0_0] = filter_Vertical<8, 0, 0>;
    p.filter[FILTER_V_8_0_1] = filter_Vertical<8, 0, 1>;
    p.filter[FILTER_V_8_1_0] = filter_Vertical<8, 1, 0>;
    p.filter[FILTER_V_8_1_1] = filter_Vertical<8, 1, 1>;

    p.partial_butterfly[BUTTERFLY_16] = partialButterfly16;
    p.partial_butterfly[BUTTERFLY_32] = partialButterfly32;
}
