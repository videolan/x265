/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Deepthi Devaki <deepthidevaki@multicorewareinc.com>,
 *          Rajesh Paulraj <rajesh@multicorewareinc.com>
 *          Mandar Gurav <mandar@multicorewareinc.com>
 *          Mahesh Pittala <mahesh@multicorewareinc.com>
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

#define IF_INTERNAL_PREC 14 ///< Number of bits for internal precision
#define IF_FILTER_PREC    6 ///< Log2 of sum of filter taps
#define IF_INTERNAL_OFFS (1 << (IF_INTERNAL_PREC - 1)) ///< Offset used internally

template<int N>
void CDECL filterVertical_short_pel(int bitDepth, short *src, int srcStride, pixel *dst, int dstStride, int block_width, int block_height, short const *coeff)
{
    int row, col;
    int cstride =  srcStride;
    src -= (N / 2 - 1) * cstride;
    int offset;
    short maxVal;
    int headRoom = IF_INTERNAL_PREC - bitDepth;
    int shift = IF_FILTER_PREC;
    shift += headRoom;
    offset = 1 << (shift - 1);
    offset +=  IF_INTERNAL_OFFS << IF_FILTER_PREC;
    maxVal = (1 << bitDepth) - 1;
    int cm[8][4];
    Vec16uc sum_uc;
    Vec8s vec_zero(0);
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

            row0.load(&src[col]);
            row1.load(&src[col + cstride]);

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

            row2.load(&src[col + 2 * cstride]);
            row3.load(&src[col + 3 * cstride]);

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
                row4.load(&src[col + 4 * cstride]);
                row5.load(&src[col + 5 * cstride]);

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

                row6.load(&src[col + 6 * cstride]);
                row7.load(&src[col + 7 * cstride]);

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
            sum = max(sum, 0);
            Vec8s maxVal_v(maxVal);
            sum = min(sum, maxVal_v);
            sum_uc = compress(sum, vec_zero);
            sum_uc.store_partial(8, dst + col);
        }
        //Handle the case when block_width is not multiple of 8
        for (; col < block_width; col += 4)
        {
            Vec8s row0, row1, row2, row3, row4, row5, row6, row7, sum;
            Vec4i row0_first, row0_last, row1_first, row1_last, sum_first, sum_last;
            Vec4i c0, c1, c2, c3, c4, c5, c6, c7;

            row0.load(&src[col]);
            row1.load(&src[col + cstride]);

            c0.load(cm[0]);
            c1.load(cm[1]);

            row0_first = extend_low(row0);
            row1_first = extend_low(row1);
            row0_first = row0_first * c0;
            row1_first = row1_first * c1;

            sum_first = row0_first + row1_first;

            row2.load(&src[col + 2 * cstride]);
            row3.load(&src[col + 3 * cstride]);

            c2.load(cm[2]);
            c3.load(cm[3]);

            row0_first = extend_low(row2);
            row0_first = row0_first * c2;
            row1_first = extend_low(row3);
            row1_first = row1_first * c3;
            sum_first += row0_first + row1_first;
            if (N == 8)
            {
                row4.load(&src[col + 4 * cstride]);
                row5.load(&src[col + 5 * cstride]);

                c4.load(cm[4]);
                c5.load(cm[5]);

                row0_first = extend_low(row4);
                row0_first = row0_first * c4;
                row1_first = extend_low(row5);
                row1_first = row1_first * c5;
                sum_first += row0_first + row1_first;

                row6.load(&src[col + 6 * cstride]);
                row7.load(&src[col + 7 * cstride]);

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
            sum = max(sum, 0);
            Vec8s maxVal_v(maxVal);
            sum = min(sum, maxVal_v);
            sum_uc = compress(sum, vec_zero);
            sum_uc.store_partial(block_width - col, dst + col);           
        }
        src += srcStride;
        dst += dstStride;
    }
}

template<int N>
void CDECL filterHorizontal_pel_pel(int bitDepth, pixel *src, int srcStride, pixel *dst, int dstStride, int block_width, int block_height, short const *coeff)

{
    int row, col;
    src -= (N / 2 - 1);                                   // Here cStride = 1
    int offset;
    short maxVal;
    int headRoom = IF_INTERNAL_PREC - bitDepth;
    offset =  (1 << (headRoom - 1));
    maxVal = (1 << bitDepth) - 1;

    Vec8s vec_sum_low, vec_zero(0);
    Vec16uc vec_src0, vec_sum;
    Vec8s vec_c;
    vec_c.load(coeff);
    Vec8s vec_c0(coeff[0]), vec_c1(coeff[1]), vec_c2(coeff[2]), vec_c3(coeff[3]), vec_c4(coeff[4]), vec_c5(coeff[5]), vec_c6(coeff[6]), vec_c7(coeff[7]);
    Vec8s vec_offset(offset);
    Vec8s vec_maxVal(maxVal);
    for (row = 0; row < block_height; row++)
    {
        col = 0;
        for (; col < (block_width - 7); col += 8)               // Iterations multiple of 8
        {
            vec_src0.load(src + col);                           // Load the 8 elements
            vec_sum_low = extend_low(vec_src0) * vec_c0;        // Multiply by c[0]
            
            vec_src0.load(src + col + 1);                       // Load the 8 elements
            vec_sum_low += extend_low(vec_src0) * vec_c1;       // Multiply by c[1]         

            vec_src0.load(src + col + 2);                       // Load the 8 elements
            vec_sum_low += extend_low(vec_src0) * vec_c2;       // Multiply by c[2]

            vec_src0.load(src + col + 3);                       // Load the 8 elements
            vec_sum_low += extend_low(vec_src0) * vec_c3;       // Multiply by c[3]

            if (N == 8)
            {
                vec_src0.load(src + col + 4);                     // Load the 8/16 elements
                vec_sum_low += extend_low(vec_src0) * vec_c4;     // Multiply by c[4]

                vec_src0.load(src + col + 5);                     // Load the 8/16 elements
                vec_sum_low += extend_low(vec_src0) * vec_c5;     // Multiply by c[5]

                vec_src0.load(src + col + 6);                     // Load the 8/16 elements
                vec_sum_low += extend_low(vec_src0) * vec_c6;     // Multiply by c[6]

                vec_src0.load(src + col + 7);                     // Load the 8/16 elements
                vec_sum_low += extend_low(vec_src0) * vec_c7;     // Multiply by c[7]
            }

            vec_sum_low = (vec_sum_low + vec_offset);               // Add offset(value copied into all short vector elements) to sum_low
            vec_sum_low = vec_sum_low >> headRoom;                  
            vec_sum_low = max(vec_sum_low, 0);                      // (val < 0) ? 0 : val;
            vec_sum_low = min(vec_sum_low, vec_maxVal);             // (val > maxVal) ? maxVal : val;
            vec_sum = compress(vec_sum_low, vec_zero);              // Save two short vectors(Vec8s, Vec8s(0)) to single short vector(Vec8s)
            vec_sum.store_partial(8,dst + col);                     // Store vector
        }
        for (; col < block_width; col++)                           // Remaining iterations
        {
            if (N == 8)
            {
                vec_src0.load(src + col);
            }
            else
            {
                vec_src0.load_partial(4, src + col);
            }
            vec_sum_low = extend_low(vec_src0) * vec_c;                        // Assuming that there is no overflow (Everywhere in this function!)
            int sum = horizontal_add(vec_sum_low);
            short val = (short)(sum + offset) >> headRoom;
            val = (val < 0) ? 0 : val;
            val = (val > maxVal) ? maxVal : val;
            dst[col] = (pixel)val;
        }
        src += srcStride;
        dst += dstStride;
    }
}

template<int N>
void CDECL filterHorizontal_pel_short(int bitDepth, pixel *src, int srcStride, short *dst, int dstStride, int block_width, int block_height, short const *coeff)
{
    int row, col;
    src -= (N / 2 - 1);
    int offset;
    int headRoom = IF_INTERNAL_PREC - bitDepth;
    int shift = IF_FILTER_PREC;
    shift -= headRoom;
    offset = -IF_INTERNAL_OFFS << shift;

    Vec8s vec_sum_low, vec_sum_high;
    Vec16uc vec_src0, vec_sum;
    Vec8s vec_c;
    vec_c.load(coeff);
    Vec8s vec_c0(coeff[0]), vec_c1(coeff[1]), vec_c2(coeff[2]), vec_c3(coeff[3]), vec_c4(coeff[4]), vec_c5(coeff[5]), vec_c6(
        coeff[6]), vec_c7(coeff[7]);
    Vec8s vec_offset(offset);

    for (row = 0; row < block_height; row++)
    {
        col = 0;
        for (; col < (block_width - 7); col += 8)               // Iterations multiple of 8
        {
            vec_src0.load(src + col);                           // Load the 8 elements
            vec_sum_low = extend_low(vec_src0) * vec_c0;        // Multiply by c[0]
            
            vec_src0.load(src + col + 1);                       // Load the 8 elements
            vec_sum_low += extend_low(vec_src0) * vec_c1;       // Multiply by c[1]         

            vec_src0.load(src + col + 2);                       // Load the 8 elements
            vec_sum_low += extend_low(vec_src0) * vec_c2;       // Multiply by c[2]

            vec_src0.load(src + col + 3);                       // Load the 8 elements
            vec_sum_low += extend_low(vec_src0) * vec_c3;       // Multiply by c[3]

            if (N == 8)
            {
                vec_src0.load(src + col + 4);                     // Load the 8/16 elements
                vec_sum_low += extend_low(vec_src0) * vec_c4;     // Multiply by c[4]

                vec_src0.load(src + col + 5);                     // Load the 8/16 elements
                vec_sum_low += extend_low(vec_src0) * vec_c5;     // Multiply by c[5]

                vec_src0.load(src + col + 6);                     // Load the 8/16 elements
                vec_sum_low += extend_low(vec_src0) * vec_c6;     // Multiply by c[6]

                vec_src0.load(src + col + 7);                     // Load the 8/16 elements
                vec_sum_low += extend_low(vec_src0) * vec_c7;     // Multiply by c[7]
            }
            vec_sum_low = (vec_sum_low + vec_offset);               // Add offset(value copied into all integer vector elements) to sum_low 
            vec_sum_low = vec_sum_low >> shift;                    
            vec_sum_low.store(dst + col);                           // Store vector
        }
        for (; col < block_width; col++)                           // Remaining iterations
        {
            if (N == 8)
            {
                vec_src0.load(src + col);
            }
            else
            {
                vec_src0.load_partial(4, src + col);
            }
            vec_sum_low = extend_low(vec_src0) * vec_c;                        // Assuming that there is no overflow (Everywhere in this function!)
            int sum = horizontal_add(vec_sum_low);
            short val = (short)(sum + offset) >> shift;
            dst[col] = val;
        }
        src += srcStride;
        dst += dstStride;
    }
}

void CDECL filterConvertPelToShort(int bitDepth, pixel *src, int srcStride, short *dst, int dstStride, int width, int height)
{
    pixel* srcOrg = src;
    short* dstOrg = dst;
    int shift = IF_INTERNAL_PREC - bitDepth;
    int row, col;
    Vec16uc src_v;
    Vec8s dst_v, val_v;
    for (row = 0; row < height; row++)
    {
        for (col = 0; col < width - 7; col += 8)
        {
            src_v.load(src + col);
            val_v = extend_low(src_v) << shift;
            dst_v = val_v - IF_INTERNAL_OFFS;
            dst_v.store(dst + col);
        }
        src += srcStride;
        dst += dstStride;
    }
    if (width % 8 != 0)
    {
        src = srcOrg;
        dst = dstOrg;
        col = width - (width % 8);
        for (row = 0; row < height; row++)
        {
            src_v.load(src + col);
            val_v = extend_low(src_v) << shift;
            dst_v = val_v - IF_INTERNAL_OFFS;
            dst_v.store_partial(width - col, dst + col);
            src += srcStride;
            dst += dstStride;
        }
    }
}

void CDECL filterConvertShortToPel(int bitDepth, short *src, int srcStride, pixel *dst, int dstStride, int width, int height)
{
    short* srcOrg = src;
    pixel* dstOrg = dst;
    int shift = IF_INTERNAL_PREC - bitDepth;
    short offset = IF_INTERNAL_OFFS;
    offset += shift ? (1 << (shift - 1)) : 0;
    short maxVal = (1 << bitDepth) - 1;
    Vec8s minVal(0);
    int row, col;
    Vec8s src_c, val_c, val_zero(0);
    Vec16uc val_uc;
    for (row = 0; row < height; row++)
    {
        for (col = 0; col < width - 7; col += 8)
        {
            src_c.load(src + col);
            val_c = add_saturated(src_c, offset) >> shift;
            val_c = max(val_c, minVal);
            val_c = min(val_c, maxVal);
            val_uc = compress(val_c, val_zero);
            val_uc.store_partial(8,dst + col);
        }
        src += srcStride;
        dst += dstStride;
    }
    if (width % 8 != 0)
    {
        src = srcOrg;
        dst = dstOrg;
        col = width - (width % 8);
        for (row = 0; row < height; row++)
        {
            src_c.load(src + col);
            val_c = add_saturated(src_c, offset) >> shift;
            val_c = max(val_c, minVal);
            val_c = min(val_c, maxVal);
            val_uc = compress(val_c, val_zero);
            val_uc.store_partial(width - col, dst + col);
            src += srcStride;
            dst += dstStride;
        }
    }
}
