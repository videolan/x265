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
    blend8s<2, 6, 10, 14, 3, 7, 11, 15>(half0, half1).store(block + 8);
}

template<bool isLast>
void CDECL filter_Horizontal_8(const short *coeff,
                               pixel *      src,
                               int          srcStride,
                               pixel *      dst,
                               int          dstStride,
                               int          block_width,
                               int          block_height,
                               short        maxVal,
                               int          offset,
                               int          shift)
{
    int row, col;
    short *src_s = (short*)src;
    short *dst_s = (short*)dst;
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

void Setup_Vec_MacroblockPrimitives(EncoderPrimitives &p)
{
    p.inversedst = inversedst;

    /*p.filter[FILTER_H_4_0] = filter_Horizontal_4<0>;
      p.filter[FILTER_H_4_1] = filter_Horizontal_4<1>;*/
    p.filter[FILTER_H_8_0] = filter_Horizontal_8<0>;
    p.filter[FILTER_H_8_1] = filter_Horizontal_8<1>;
}
