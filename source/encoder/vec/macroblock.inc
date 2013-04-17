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

    tmp0.load(tmp);
    tmp1.load(tmp + 8);

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

    Vec8s half0 = compress(min(max(-32768, c0_final), 32767), min(max(-32768, c1_final), 32767));
    Vec8s half1 = compress(min(max(-32768, c2_final), 32767), min(max(-32768, c3_final), 32767));
    blend8s<0, 4, 8, 12, 1, 5, 9, 13>(half0, half1).store(block);
    blend8s<2, 6, 10, 14, 3, 7, 11, 15>(half0, half1).store(block+8);
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

    Vec8i g_aiT_one_row(90, 87, 80, 70, 57, 43, 25,  9);
    Vec8i g_aiT_three_row(87, 57,  9, -43, -80, -90, -70, -25);
    Vec8i g_aiT_five_row(80,  9, -70, -87, -25, 57, 90, 43);
    Vec8i g_aiT_seven_row(70, -43, -87,  9, 90, 25, -80, -57);
    Vec8i g_aiT_nine_row(57, -80, -25, 90, -9, -87, 43, 70);
    Vec8i g_aiT_eleven_row(43, -90, 57, 25, -87, 70,  9, -80);
    Vec8i g_aiT_thirteen_row(25, -70, 90, -80, 43,  9, -57, 87);
    Vec8i g_aiT_fifteen_row(9, -25, 43, -57, 70, -80, 87, -90);

    for (j = 0; j < line; j++)
    {
        Vec16s tmp1;
        tmp1.load(src);
        Vec8i src_first_half = extend_low(tmp1);
        Vec8i src_second_half_tmp = extend_high(tmp1);
        Vec8i src_second_half = permute8i<15, 14, 13, 12, 11, 10, 9, 8>(src_second_half_tmp);

        Vec8i E = src_first_half + src_second_half;
        Vec8i O = src_first_half - src_second_half;

        Vec4i E_first_half;
        E_first_half = E.get_low();
        Vec4i E_second_half_tmp = E.get_high();
        Vec4i E_second_half = permute4i<7, 6, 5, 4>(E_second_half_tmp);

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

        Vec8i dst_tmp1 = g_aiT_one_row * O;
        Vec8i dst_tmp3 = g_aiT_three_row * O;
        Vec8i dst_tmp5 = g_aiT_five_row * O;
        Vec8i dst_tmp7 = g_aiT_seven_row * O;
        Vec8i dst_tmp9 = g_aiT_nine_row * O;
        Vec8i dst_tmp11 = g_aiT_eleven_row * O;
        Vec8i dst_tmp13 = g_aiT_thirteen_row * O;
        Vec8i dst_tmp15 = g_aiT_fifteen_row * O;

        int dst_one = horizontal_add(dst_tmp1);
        int dst_three = horizontal_add(dst_tmp3);
        int dst_five = horizontal_add(dst_tmp5);
        int dst_seven = horizontal_add(dst_tmp7);
        int dst_nine = horizontal_add(dst_tmp9);
        int dst_eleven = horizontal_add(dst_tmp11);
        int dst_thirteen = horizontal_add(dst_tmp13);
        int dst_fifteen = horizontal_add(dst_tmp15);

        Vec8i dst_1_2_5_7_9_11_13_15(dst_one, dst_three, dst_five, dst_seven, dst_nine, dst_eleven, dst_thirteen, dst_fifteen);
        dst_1_2_5_7_9_11_13_15 = dst_1_2_5_7_9_11_13_15 + add;
        dst_1_2_5_7_9_11_13_15 = dst_1_2_5_7_9_11_13_15 >> shift;

        dst[1 * line] = dst_1_2_5_7_9_11_13_15[0];
        dst[3 * line] = dst_1_2_5_7_9_11_13_15[1];
        dst[5 * line] = dst_1_2_5_7_9_11_13_15[2];
        dst[7 * line] = dst_1_2_5_7_9_11_13_15[3];
        dst[9 * line] = dst_1_2_5_7_9_11_13_15[4];
        dst[11 * line] = dst_1_2_5_7_9_11_13_15[5];
        dst[13 * line] = dst_1_2_5_7_9_11_13_15[6];
        dst[15 * line] = dst_1_2_5_7_9_11_13_15[7];

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

    p.filter[FILTER_H_8_0_0] = filter_Horizontal_8<0, 0>;
    p.filter[FILTER_H_8_0_1] = filter_Horizontal_8<0, 1>;
    p.filter[FILTER_H_8_1_0] = filter_Horizontal_8<1, 0>;
    p.filter[FILTER_H_8_1_1] = filter_Horizontal_8<1, 1>;

    p.partial_butterfly[BUTTERFLY_16] = partialButterfly16;
}

