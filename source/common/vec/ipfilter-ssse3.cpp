/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Deepthi Devaki <deepthidevaki@multicorewareinc.com>,
 *          Rajesh Paulraj <rajesh@multicorewareinc.com>
 *          Mandar Gurav <mandar@multicorewareinc.com>
 *          Mahesh Pittala <mahesh@multicorewareinc.com>
 *          Praveen Kumar Tiwari <praveen@multicorewareinc.com>
 *          Nabajit Deka <nabajit@multicorewareinc.com>
 *          Min Chen <chenm003@163.com>
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

#define INSTRSET 4
#include "vectorclass.h"

#include "primitives.h"
#include "TLibCommon/TComRom.h"
#include <string.h>

namespace {
template<int N>
void filterHorizontal_p_s(pixel *src, intptr_t srcStride, short *dst, intptr_t dstStride, int width, int height, short const *coeff)
{
    src -= (N / 2 - 1);

    int offset;
    int headRoom = IF_INTERNAL_PREC - X265_DEPTH;
    int shift = IF_FILTER_PREC;
    shift -= headRoom;
    offset = -IF_INTERNAL_OFFS << shift;

    int row, col;

    __m128i a = _mm_loadu_si128((__m128i*)coeff);
    __m128i T10 = _mm_packs_epi16(a, a);

    __m128i S1 = _mm_slli_si128(T10, 12);
    __m128i S2 = _mm_srli_si128(S1, 4);
    __m128i S3 = _mm_srli_si128(S2, 4);
    __m128i S4 = _mm_srli_si128(S3, 4);
    __m128i S = _mm_add_epi8(S1, _mm_add_epi8(S2, S3));
    S =  _mm_add_epi8(S, S4);

    __m128i Tm1 = _mm_setr_epi8(0, 1, 2, 3, 4, 5, 6, 7, 1, 2, 3, 4, 5, 6, 7, 8);
    __m128i Tm2 = _mm_setr_epi8(2, 3, 4, 5, 6, 7, 8, 9, 3, 4, 5, 6, 7, 8, 9, 10);
    __m128i Tm3 = _mm_setr_epi8(4, 5, 6, 7, 8, 9, 10, 11, 5, 6, 7, 8, 9, 10, 11, 12);
    __m128i Tm4 = _mm_setr_epi8(6, 7, 8, 9, 10, 11, 12, 13, 7, 8, 9, 10, 11, 12, 13, 14);
    __m128i Tm5 = _mm_setr_epi8(0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5, 3, 4, 5, 6);
    __m128i Tm6 = _mm_setr_epi8(4, 5, 6, 7, 5, 6, 7, 8, 6, 7, 8, 9, 7, 8, 9, 10);

    for (row = 0; row < height; row++)
    {
        col = 0;
        for (; col < (width - 7); col += 8)
        {
            __m128i srcCoeff = _mm_loadu_si128((__m128i*)(src + col));
            __m128i sum;

            if (N == 4)
            {
                __m128i T00 = _mm_shuffle_epi8(srcCoeff, Tm5);
                __m128i T20 = _mm_maddubs_epi16(T00, S);

                __m128i T30 = _mm_shuffle_epi8(srcCoeff, Tm6);
                __m128i T40 = _mm_maddubs_epi16(T30, S);

                sum = _mm_hadd_epi16(T20, T40);
            }
            else  // (N == 8)
            {
                __m128i T00 = _mm_shuffle_epi8(srcCoeff, Tm1);
                __m128i T20 = _mm_maddubs_epi16(T00, T10);

                __m128i T30 = _mm_shuffle_epi8(srcCoeff, Tm2);
                __m128i T40 = _mm_maddubs_epi16(T30, T10);

                __m128i T50 = _mm_shuffle_epi8(srcCoeff, Tm3);
                __m128i T60 = _mm_maddubs_epi16(T50, T10);

                __m128i T70 = _mm_shuffle_epi8(srcCoeff, Tm4);
                __m128i T80 = _mm_maddubs_epi16(T70, T10);

                __m128i s1 = _mm_hadd_epi16(T20, T40);
                __m128i s2 = _mm_hadd_epi16(T60, T80);
                sum = _mm_hadd_epi16(s1, s2);
            }

            __m128i sumOffset = _mm_set1_epi16(offset);
            __m128i val = _mm_add_epi16(sum, sumOffset);

            val = _mm_srai_epi16(val, shift);
            _mm_storeu_si128((__m128i*)&dst[col], val);
        }

        for (; col < width; col++)                    // Remaining iterations
        {
            __m128i NewSrc = _mm_loadl_epi64((__m128i*)(src + col));
            __m128i T00 = _mm_maddubs_epi16(NewSrc, T10);
            __m128i add = _mm_hadd_epi16(T00, T00);
            short sum = _mm_extract_epi16(add, 0);
            if (N == 8)
            {
                add = _mm_hadd_epi16(add, add);
                sum = _mm_extract_epi16(add, 0);
            }
            short val = (short)(sum + offset) >> shift;
            dst[col] = val;
        }

        src += srcStride;
        dst += dstStride;
    }
}

void filterConvertPelToShort(pixel *source, intptr_t sourceStride, short *dest, intptr_t destStride, int width, int height)
{
    pixel* src = source;
    short* dst = dest;
    int shift = IF_INTERNAL_PREC - X265_DEPTH;
    int row, col;

    __m128i val1, val2, val3;

    for (row = 0; row < height; row++)
    {
        for (col = 0; col < width - 7; col += 8)
        {
            val1 = _mm_loadu_si128((__m128i const*)(source + col));
            val2 = _mm_sll_epi16(_mm_unpacklo_epi8(val1, _mm_setzero_si128()), _mm_cvtsi32_si128(shift));
            val3 = _mm_sub_epi16(val2, _mm_set1_epi16(IF_INTERNAL_OFFS));
            _mm_storeu_si128((__m128i*)(dest + col), val3);
        }
        source += sourceStride;
        dest += destStride;
    }
    if (width % 8 != 0)
    {
        source = src;
        dest = dst;
        col = width - (width % 8);
        for (row = 0; row < height; row++)
        {
            val1 = _mm_loadu_si128((__m128i const*)(source + col));
            val2 = _mm_sll_epi16(_mm_unpacklo_epi8(val1, _mm_setzero_si128()), _mm_cvtsi32_si128(shift));
            val3 = _mm_sub_epi16(val2, _mm_set1_epi16(IF_INTERNAL_OFFS));

            int n = width - col;
            if (n >= 8) 
            {
                _mm_storeu_si128((__m128i*)(dest + col), val3);
            }
            else if (n <= 0) ;    // do nothing if value of is n less than 0
            else
            {
                union
                {
                    int8_t  c[16];
                    int16_t s[8];
                    int32_t i[4];
                    int64_t q[2];
                } u;
                _mm_storeu_si128((__m128i*)u.c, val3);
                int j = 0;
                if (n & 4)    // n == (4,5,6,7)
                {
                    *(int64_t*)(dest + col) = u.q[0];
                    j += 8;
                }
                if (n & 2)    // n == (2,3,6,7)
                {
                    ((int32_t*)(dest + col))[j/4] = u.i[j/4];
                    j += 4;
                }
                if (n & 1)    // n == (1,3,5,7)
                {
                    ((int16_t*)(dest + col))[j/2] = u.s[j/2];
                }
            }
            source += sourceStride;
            dest += destStride;
        }
    }
}

void filterConvertShortToPel(short *source, intptr_t sourceStride, pixel *dest, intptr_t destStride, int width, int height)
{
    short* src = source;
    pixel* dst = dest;
    int shift = IF_INTERNAL_PREC - X265_DEPTH;
    short offset = IF_INTERNAL_OFFS;
    offset += shift ? (1 << (shift - 1)) : 0;
    short maxval = (1 << X265_DEPTH) - 1;
    int row, col;

    __m128i minval  = _mm_setzero_si128();
    __m128i zeroval = _mm_setzero_si128();
    __m128i val1, val2, val3;

    for (row = 0; row < height; row++)
    {
        for (col = 0; col < width - 7; col += 8)
        {
            val1 = _mm_loadu_si128((__m128i const*)(source + col));
            val2 = _mm_sra_epi16(_mm_adds_epi16(val1, _mm_set1_epi16(offset)), _mm_cvtsi32_si128(shift));
            val2 = _mm_max_epi16(val2, minval);
            val2 = _mm_min_epi16(val2, _mm_set1_epi16(maxval));

            __m128i mask  = _mm_set1_epi32(0x00FF00FF);           // mask for low bytes
            __m128i lowm  = _mm_and_si128(val2, mask);            // bytes of low
            __m128i highm = _mm_and_si128(zeroval, mask);         // bytes of high
            val3 = _mm_packus_epi16(lowm, highm);                 // unsigned pack

            union
            {
                int8_t  c[16];
                int64_t q[2];
            } u;
            _mm_storeu_si128((__m128i*)u.c, val3);
            *(int64_t*)(dest + col) = u.q[0];
        }
        source += sourceStride;
        dest += destStride;
    }

    if (width % 8 != 0)
    {
        source = src;
        dest = dst;
        col = width - (width % 8);
        for (row = 0; row < height; row++)
        {
            val1 = _mm_loadu_si128((__m128i const*)(source + col));
            val2 = _mm_sra_epi16(_mm_adds_epi16(val1, _mm_set1_epi16(offset)), _mm_cvtsi32_si128(shift));
            val2 = _mm_max_epi16(val2, minval);
            val2 = _mm_min_epi16(val2, _mm_set1_epi16(maxval));

            __m128i mask  = _mm_set1_epi32(0x00FF00FF);           // mask for low bytes
            __m128i lowm  = _mm_and_si128(val2, mask);            // bytes of low
            __m128i highm = _mm_and_si128(zeroval, mask);         // bytes of high
            val3 = _mm_packus_epi16(lowm, highm);                 // unsigned pack

            int n = width - col;
            if (n >= 16) 
            {
                _mm_storeu_si128((__m128i*)(dest + col), val3);
            }
            else if (n <= 0) ;    // do nothing if value of is n less than 0
            else
            {
                union
                {
                    int8_t  c[16];
                    int16_t s[8];
                    int32_t i[4];
                    int64_t q[2];
                } u;
                _mm_storeu_si128((__m128i*)u.c, val3);
                int j = 0;
                if (n & 8)    // n == (8,9,10,11,12,13,14,15)
                {
                    *(int64_t*)(dest + col) = u.q[0];
                    j += 8;
                }
                if (n & 4)    // n == (4,5,6,7,12,13,14,15)
                {
                    ((int32_t*)(dest + col))[j/4] = u.i[j/4];
                    j += 4;
                }
                if (n & 2)    // n == (2,3,6,7,10,11,14,15)
                {
                    ((int16_t*)(dest + col))[j/2] = u.s[j/2];
                    j += 2;
                }
                if (n & 1)    // n == (1,3,5,7,9,11,13,15)
                {
                    ((int8_t*)(dest + col))[j] = u.c[j];
                }
            }
            source += sourceStride;
            dest += destStride;
        }
    }
}
}

namespace x265 {
void Setup_Vec_IPFilterPrimitives_ssse3(EncoderPrimitives& p)
{
#if HIGH_BIT_DEPTH
    p.ipfilter_p2s = p.ipfilter_p2s;
#else
    p.ipfilter_ps[FILTER_H_P_S_4] = filterHorizontal_p_s<4>;
    p.ipfilter_ps[FILTER_H_P_S_8] = filterHorizontal_p_s<8>;

    p.ipfilter_p2s = filterConvertPelToShort;
    p.ipfilter_s2p = filterConvertShortToPel;
#endif
}
}
