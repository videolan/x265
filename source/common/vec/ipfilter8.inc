/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Deepthi Devaki <deepthidevaki@multicorewareinc.com>,
 *          Rajesh Paulraj <rajesh@multicorewareinc.com>
 *          Mandar Gurav <mandar@multicorewareinc.com>
 *          Mahesh Pittala <mahesh@multicorewareinc.com>
 *          Praveen Kumar Tiwari <praveen@multicorewareinc.com>
 *          Nabajit Deka <nabajit@multicorewareinc.com>
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

#if INSTRSET >= X265_CPU_LEVEL_SSE41
template<int N>
void filterVertical_s_p(short *src, int srcStride, pixel *dst, int dstStride, int width, int height, short const *coeff)
{
    src -= (N / 2 - 1) * srcStride;

    int offset;
    int headRoom = IF_INTERNAL_PREC - X265_DEPTH;
    int shift = IF_FILTER_PREC;
    shift += headRoom;
    offset = 1 << (shift - 1);
    offset +=  IF_INTERNAL_OFFS << IF_FILTER_PREC;

    __m128i coeffTemp = _mm_loadu_si128((__m128i const*)coeff);
    __m128i coeffTempLow = _mm_cvtepi16_epi32(coeffTemp);
    coeffTemp = _mm_srli_si128(coeffTemp, 8);
    __m128i coeffTempHigh = _mm_cvtepi16_epi32(coeffTemp);

    __m128i filterCoeff0 = _mm_shuffle_epi32(coeffTempLow, 0x00);
    __m128i filterCoeff1 = _mm_shuffle_epi32(coeffTempLow, 0x55);
    __m128i filterCoeff2 = _mm_shuffle_epi32(coeffTempLow, 0xAA);
    __m128i filterCoeff3 = _mm_shuffle_epi32(coeffTempLow, 0xFF);

    __m128i filterCoeff4 = _mm_shuffle_epi32(coeffTempHigh, 0x00);
    __m128i filterCoeff5 = _mm_shuffle_epi32(coeffTempHigh, 0x55);
    __m128i filterCoeff6 = _mm_shuffle_epi32(coeffTempHigh, 0xAA);
    __m128i filterCoeff7 = _mm_shuffle_epi32(coeffTempHigh, 0xFF);

    __m128i mask4 = _mm_cvtsi32_si128(0xFFFFFFFF);

    int row, col;

    for (row = 0; row < height; row++)
    {
        col = 0;
        for (; col < (width - 7); col += 8)
        {
            __m128i srcCoeff = _mm_loadu_si128((__m128i const*)(&src[col]));
            __m128i srcCoeffTemp1 = _mm_cvtepi16_epi32(srcCoeff);
            __m128i T00 = _mm_mullo_epi32(srcCoeffTemp1, filterCoeff0);
            srcCoeff = _mm_srli_si128(srcCoeff, 8);
            srcCoeff = _mm_cvtepi16_epi32(srcCoeff);
            __m128i T01 = _mm_mullo_epi32(srcCoeff, filterCoeff0);

            srcCoeff = _mm_loadu_si128((__m128i const*)(&src[col + srcStride]));
            __m128i srcCoeffTemp2 = _mm_cvtepi16_epi32(srcCoeff);
            __m128i T10 = _mm_mullo_epi32(srcCoeffTemp2, filterCoeff1);
            srcCoeff = _mm_srli_si128(srcCoeff, 8);
            srcCoeff = _mm_cvtepi16_epi32(srcCoeff);
            __m128i T11 = _mm_mullo_epi32(srcCoeff, filterCoeff1);

            srcCoeff = _mm_loadu_si128((__m128i const*)(&src[col + 2 * srcStride]));
            __m128i srcCoeffTemp3 = _mm_cvtepi16_epi32(srcCoeff);
            __m128i T20 = _mm_mullo_epi32(srcCoeffTemp3, filterCoeff2);
            srcCoeff = _mm_srli_si128(srcCoeff, 8);
            srcCoeff = _mm_cvtepi16_epi32(srcCoeff);
            __m128i T21 = _mm_mullo_epi32(srcCoeff, filterCoeff2);

            srcCoeff = _mm_loadu_si128((__m128i const*)(&src[col + 3 * srcStride]));
            __m128i srcCoeffTemp4 = _mm_cvtepi16_epi32(srcCoeff);
            __m128i T30 = _mm_mullo_epi32(srcCoeffTemp4, filterCoeff3);
            srcCoeff = _mm_srli_si128(srcCoeff, 8);
            srcCoeff = _mm_cvtepi16_epi32(srcCoeff);
            __m128i T31 = _mm_mullo_epi32(srcCoeff, filterCoeff3);

            __m128i sum01 = _mm_add_epi32(T00, T10);
            __m128i sum23 = _mm_add_epi32(T20, T30);
            __m128i sumlo0123 = _mm_add_epi32(sum01, sum23);

            __m128i sum45 = _mm_add_epi32(T01, T11);
            __m128i sum67 = _mm_add_epi32(T21, T31);
            __m128i sumhi0123 = _mm_add_epi32(sum45, sum67);

            if (N == 8)
            {
                srcCoeff = _mm_loadu_si128((__m128i const*)(&src[col + 4 * srcStride]));
                srcCoeffTemp1 = _mm_cvtepi16_epi32(srcCoeff);
                T00 = _mm_mullo_epi32(srcCoeffTemp1, filterCoeff4);
                srcCoeff = _mm_srli_si128(srcCoeff, 8);
                srcCoeff = _mm_cvtepi16_epi32(srcCoeff);
                T01 = _mm_mullo_epi32(srcCoeff, filterCoeff4);

                srcCoeff = _mm_loadu_si128((__m128i const*)(&src[col + 5 * srcStride]));
                srcCoeffTemp2 = _mm_cvtepi16_epi32(srcCoeff);
                T10 = _mm_mullo_epi32(srcCoeffTemp2, filterCoeff5);
                srcCoeff = _mm_srli_si128(srcCoeff, 8);
                srcCoeff = _mm_cvtepi16_epi32(srcCoeff);
                T11 = _mm_mullo_epi32(srcCoeff, filterCoeff5);

                srcCoeff = _mm_loadu_si128((__m128i const*)(&src[col + 6 * srcStride]));
                srcCoeffTemp3 = _mm_cvtepi16_epi32(srcCoeff);
                T20 = _mm_mullo_epi32(srcCoeffTemp3, filterCoeff6);
                srcCoeff = _mm_srli_si128(srcCoeff, 8);
                srcCoeff = _mm_cvtepi16_epi32(srcCoeff);
                T21 = _mm_mullo_epi32(srcCoeff, filterCoeff6);

                srcCoeff = _mm_loadu_si128((__m128i const*)(&src[col + 7 * srcStride]));
                srcCoeffTemp4 = _mm_cvtepi16_epi32(srcCoeff);
                T30 = _mm_mullo_epi32(srcCoeffTemp4, filterCoeff7);
                srcCoeff = _mm_srli_si128(srcCoeff, 8);
                srcCoeff = _mm_cvtepi16_epi32(srcCoeff);
                T31 = _mm_mullo_epi32(srcCoeff, filterCoeff7);

                sum01 = _mm_add_epi32(T00, T10);
                sum23 = _mm_add_epi32(T20, T30);
                sumlo0123 = _mm_add_epi32(sumlo0123, _mm_add_epi32(sum01, sum23));

                sum45 = _mm_add_epi32(T01, T11);
                sum67 = _mm_add_epi32(T21, T31);
                sumhi0123 = _mm_add_epi32(sumhi0123, _mm_add_epi32(sum45, sum67));
            }
            __m128i sumOffset = _mm_set1_epi32(offset);

            __m128i val1 = _mm_add_epi32(sumlo0123, sumOffset);
            val1 = _mm_srai_epi32(val1, shift);

            __m128i val2 = _mm_add_epi32(sumhi0123, sumOffset);
            val2 = _mm_srai_epi32(val2, shift);

            __m128i val = _mm_packs_epi32(val1, val2);
            __m128i res = _mm_packus_epi16(val, val);
            _mm_storel_epi64((__m128i*)&dst[col], res);
        }

        for (; col < width; col += 4)
        {
            __m128i srcCoeff = _mm_loadl_epi64((__m128i const*)(&src[col]));
            __m128i srcCoeffTemp1 = _mm_cvtepi16_epi32(srcCoeff);
            __m128i T00 = _mm_mullo_epi32(srcCoeffTemp1, filterCoeff0);

            srcCoeff = _mm_loadl_epi64((__m128i const*)(&src[col + srcStride]));
            __m128i srcCoeffTemp2 = _mm_cvtepi16_epi32(srcCoeff);
            __m128i T10 = _mm_mullo_epi32(srcCoeffTemp2, filterCoeff1);

            srcCoeff = _mm_loadl_epi64((__m128i const*)(&src[col + 2 * srcStride]));
            __m128i srcCoeffTemp3 = _mm_cvtepi16_epi32(srcCoeff);
            __m128i T20 = _mm_mullo_epi32(srcCoeffTemp3, filterCoeff2);

            srcCoeff = _mm_loadl_epi64((__m128i const*)(&src[col + 3 * srcStride]));
            __m128i srcCoeffTemp4 = _mm_cvtepi16_epi32(srcCoeff);
            __m128i T30 = _mm_mullo_epi32(srcCoeffTemp4, filterCoeff3);

            __m128i sum01 = _mm_add_epi32(T00, T10);
            __m128i sum23 = _mm_add_epi32(T20, T30);
            __m128i sumlo0123 = _mm_add_epi32(sum01, sum23);

            if (N == 8)
            {
                srcCoeff = _mm_loadl_epi64((__m128i const*)(&src[col + 4 * srcStride]));
                srcCoeffTemp1 = _mm_cvtepi16_epi32(srcCoeff);
                T00 = _mm_mullo_epi32(srcCoeffTemp1, filterCoeff4);

                srcCoeff = _mm_loadl_epi64((__m128i const*)(&src[col + 5 * srcStride]));
                srcCoeffTemp2 = _mm_cvtepi16_epi32(srcCoeff);
                T10 = _mm_mullo_epi32(srcCoeffTemp2, filterCoeff5);

                srcCoeff = _mm_loadl_epi64((__m128i const*)(&src[col + 6 * srcStride]));
                srcCoeffTemp3 = _mm_cvtepi16_epi32(srcCoeff);
                T20 = _mm_mullo_epi32(srcCoeffTemp3, filterCoeff6);

                srcCoeff = _mm_loadl_epi64((__m128i const*)(&src[col + 7 * srcStride]));
                srcCoeffTemp4 = _mm_cvtepi16_epi32(srcCoeff);
                T30 = _mm_mullo_epi32(srcCoeffTemp4, filterCoeff7);

                sum01 = _mm_add_epi32(T00, T10);
                sum23 = _mm_add_epi32(T20, T30);
                sumlo0123 = _mm_add_epi32(sumlo0123, _mm_add_epi32(sum01, sum23));
            }

            __m128i zero16 = _mm_set1_epi16(0);
            __m128i zero32 = _mm_set1_epi32(0);
            __m128i sumOffset = _mm_set1_epi32(offset);

            __m128i val1 = _mm_add_epi32(sumlo0123, sumOffset);
            val1 = _mm_srai_epi32(val1, shift);

            __m128i val = _mm_packs_epi32(val1, zero32);
            __m128i res = _mm_packus_epi16(val, zero16);

            int n = width - col;
            __m128i mask1, mask2, mask3;

            switch (n)   // store either 1, 2, 3 or 4 8-bit results in dst
            {
            case 1: mask1 = _mm_srli_si128(mask4, 3);
                _mm_maskmoveu_si128(res, mask1, (char*)&dst[col]);
                break;

            case 2:  mask2 = _mm_srli_si128(mask4, 2);
                _mm_maskmoveu_si128(res, mask2, (char*)&dst[col]);
                break;

            case 3: mask3 = _mm_srli_si128(mask4, 1);
                _mm_maskmoveu_si128(res, mask3, (char*)&dst[col]);
                break;

            default: _mm_maskmoveu_si128(res, mask4, (char*)&dst[col]);
                break;
            }
        }

        src += srcStride;
        dst += dstStride;
    }
}

/*
    Please refer Fig 7 in HEVC Overview document to familiarize with variables' naming convention
    Input: Subpel from the Horizontal filter - 'src'
    Output: All planes in the corresponding column - 'dst<A|E|I|P>'
*/

#define PROCESSROW(a0, a1, a2, a3, a4, a5, a6, a7) { \
        tmp = _mm_loadu_si128((__m128i const*)(src + col + (row + 7) * srcStride)); \
        a7 = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15)); \
        exp1 = _mm_sub_epi32(_mm_sub_epi32(_mm_sll_epi32(a1, _mm_cvtsi32_si128(2)), a0), _mm_mullo_epi32(a2, _mm_set1_epi32(10))); \
        exp2 = _mm_mullo_epi32(a3, _mm_set1_epi32(40)); \
        exp3 = _mm_mullo_epi32(a3, _mm_set1_epi32(17)); \
        exp4 = _mm_mullo_epi32(a4, _mm_set1_epi32(17)); \
        exp5 = _mm_mullo_epi32(a4, _mm_set1_epi32(40)); \
        exp6 = _mm_sub_epi32(_mm_sub_epi32(_mm_sll_epi32(a6, _mm_cvtsi32_si128(2)), a7), _mm_mullo_epi32(a5, _mm_set1_epi32(10))); \
        sume = _mm_add_epi32(exp1, _mm_add_epi32(_mm_add_epi32(exp2, exp3), \
                                                 _mm_add_epi32(_mm_add_epi32(a3, exp4), \
                                                               _mm_add_epi32(_mm_mullo_epi32(a5, _mm_set1_epi32(-5)), \
                                                                             a6) \
                                                               ) \
                                                 ) \
                             ); \
        sumi = _mm_sub_epi32(_mm_add_epi32(_mm_add_epi32(exp1, exp2), _mm_add_epi32(exp5, exp6)), \
                             _mm_add_epi32(a2, a5)); \
        sump = _mm_add_epi32(a1, _mm_add_epi32(_mm_add_epi32(exp3, exp4), \
                                               _mm_add_epi32(_mm_add_epi32(exp5, exp6), \
                                                             _mm_add_epi32(_mm_mullo_epi32(a2, _mm_set1_epi32(-5)), \
                                                                           a4) \
                                                             ) \
                                               ) \
                             ); \
        /* store results */ \
        sumi = _mm_sra_epi32(_mm_add_epi32(sumi, _mm_set1_epi32(offset)), _mm_cvtsi32_si128(12)); \
        tmp  =  _mm_packs_epi32(sumi, _mm_setzero_si128()); \
        sumi = _mm_packus_epi16(tmp, _mm_setzero_si128()); \
        *(uint32_t*)(dstI + row * dstStride + col) = _mm_cvtsi128_si32(sumi); \
        sume = _mm_sra_epi32(_mm_add_epi32(sume, _mm_set1_epi32(offset)), _mm_cvtsi32_si128(12)); \
        tmp  =  _mm_packs_epi32(sume, _mm_setzero_si128()); \
        sume = _mm_packus_epi16(tmp, _mm_setzero_si128()); \
        *(uint32_t*)(dstE + row * dstStride + col) = _mm_cvtsi128_si32(sume); \
        sump = _mm_sra_epi32(_mm_add_epi32(sump, _mm_set1_epi32(offset)), _mm_cvtsi32_si128(12)); \
        tmp  =  _mm_packs_epi32(sump, _mm_setzero_si128()); \
        sump = _mm_packus_epi16(tmp, _mm_setzero_si128()); \
        *(uint32_t*)(dstP + row * dstStride + col) = _mm_cvtsi128_si32(sump); \
}

#define EXTENDCOL(X, Y) { /*X=0 for leftmost column, X=block_width+marginX for rightmost column*/ \
        tmp16e = _mm_shuffle_epi8(sume, _mm_set1_epi8(Y)); \
        tmp16i = _mm_shuffle_epi8(sumi, _mm_set1_epi8(Y)); \
        tmp16p = _mm_shuffle_epi8(sump, _mm_set1_epi8(Y)); \
        for (int i = -marginX; i < -16; i += 16) \
        { \
            _mm_storeu_si128((__m128i*)(dstE + row * dstStride + X + i), tmp16e); \
            _mm_storeu_si128((__m128i*)(dstI + row * dstStride + X + i), tmp16i); \
            _mm_storeu_si128((__m128i*)(dstP + row * dstStride + X + i), tmp16p); \
        } \
        _mm_storeu_si128((__m128i*)(dstE + row * dstStride + X - 16), tmp16e); /*Assuming marginX > 16*/ \
        _mm_storeu_si128((__m128i*)(dstI + row * dstStride + X - 16), tmp16i); \
        _mm_storeu_si128((__m128i*)(dstP + row * dstStride + X - 16), tmp16p); \
}

void filterVerticalMultiplaneExtend(short *src, int srcStride,
                                    pixel *dstE, pixel *dstI, pixel *dstP, int dstStride,
                                    int block_width, int block_height,
                                    int marginX, int marginY)
{
    int row, col;
    int offset;
    int headRoom = IF_INTERNAL_PREC - X265_DEPTH;
    int shift = IF_FILTER_PREC + headRoom;

    offset = 1 << (shift - 1);
    offset +=  IF_INTERNAL_OFFS << IF_FILTER_PREC;
    src -= (8 / 2 - 1) * srcStride;

    __m128i tmp16e, tmp16i, tmp16p;
    __m128i a0, a1, a2, a3, a4, a5, a6, a7;
    __m128i tmp;
    __m128i sume, sumi, sump;
    __m128i exp1, exp2, exp3, exp4, exp5, exp6;

    col = 0;

    tmp = _mm_loadu_si128((__m128i const*)(src + col));
    a0  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
    tmp = _mm_loadu_si128((__m128i const*)(src + col + srcStride));
    a1  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
    tmp = _mm_loadu_si128((__m128i const*)(src + col + 2 * srcStride));
    a2  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
    tmp = _mm_loadu_si128((__m128i const*)(src + col + 3 * srcStride));
    a3  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
    tmp = _mm_loadu_si128((__m128i const*)(src + col + 4 * srcStride));
    a4  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
    tmp = _mm_loadu_si128((__m128i const*)(src + col + 5 * srcStride));
    a5  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
    tmp = _mm_loadu_si128((__m128i const*)(src + col + 6 * srcStride));
    a6  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));

    for (row = 0; row < block_height; row++)
    {
        PROCESSROW(a0, a1, a2, a3, a4, a5, a6, a7) EXTENDCOL(0, 0) row++;
        PROCESSROW(a1, a2, a3, a4, a5, a6, a7, a0) EXTENDCOL(0, 0) row++;
        PROCESSROW(a2, a3, a4, a5, a6, a7, a0, a1) EXTENDCOL(0, 0) row++;
        PROCESSROW(a3, a4, a5, a6, a7, a0, a1, a2) EXTENDCOL(0, 0) row++;
        PROCESSROW(a4, a5, a6, a7, a0, a1, a2, a3) EXTENDCOL(0, 0) row++;
        PROCESSROW(a5, a6, a7, a0, a1, a2, a3, a4) EXTENDCOL(0, 0) row++;
        PROCESSROW(a6, a7, a0, a1, a2, a3, a4, a5) EXTENDCOL(0, 0) row++;
        PROCESSROW(a7, a0, a1, a2, a3, a4, a5, a6) EXTENDCOL(0, 0)
    }

    col += 4;

    for (; col < block_width - 4; col += 4)         // Considering block width is always a multiple of 4
    {
        tmp = _mm_loadu_si128((__m128i const*)(src + col));
        a0  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
        tmp = _mm_loadu_si128((__m128i const*)(src + col + srcStride));
        a1  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
        tmp = _mm_loadu_si128((__m128i const*)(src + col + 2 * srcStride));
        a2  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
        tmp = _mm_loadu_si128((__m128i const*)(src + col + 3 * srcStride));
        a3  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
        tmp = _mm_loadu_si128((__m128i const*)(src + col + 4 * srcStride));
        a4  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
        tmp = _mm_loadu_si128((__m128i const*)(src + col + 5 * srcStride));
        a5  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
        tmp = _mm_loadu_si128((__m128i const*)(src + col + 6 * srcStride));
        a6  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));

        for (row = 0; row < block_height; row++)
        {
            PROCESSROW(a0, a1, a2, a3, a4, a5, a6, a7) row++;
            PROCESSROW(a1, a2, a3, a4, a5, a6, a7, a0) row++;
            PROCESSROW(a2, a3, a4, a5, a6, a7, a0, a1) row++;
            PROCESSROW(a3, a4, a5, a6, a7, a0, a1, a2) row++;
            PROCESSROW(a4, a5, a6, a7, a0, a1, a2, a3) row++;
            PROCESSROW(a5, a6, a7, a0, a1, a2, a3, a4) row++;
            PROCESSROW(a6, a7, a0, a1, a2, a3, a4, a5) row++;
            PROCESSROW(a7, a0, a1, a2, a3, a4, a5, a6)
        }
    }

    tmp = _mm_loadu_si128((__m128i const*)(src + col));
    a0  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
    tmp = _mm_loadu_si128((__m128i const*)(src + col + srcStride));
    a1  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
    tmp = _mm_loadu_si128((__m128i const*)(src + col + 2 * srcStride));
    a2  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
    tmp = _mm_loadu_si128((__m128i const*)(src + col + 3 * srcStride));
    a3  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
    tmp = _mm_loadu_si128((__m128i const*)(src + col + 4 * srcStride));
    a4  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
    tmp = _mm_loadu_si128((__m128i const*)(src + col + 5 * srcStride));
    a5  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
    tmp = _mm_loadu_si128((__m128i const*)(src + col + 6 * srcStride));
    a6  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));

    for (row = 0; row < block_height; row++)
    {
        PROCESSROW(a0, a1, a2, a3, a4, a5, a6, a7) EXTENDCOL((block_width + marginX), 3) row++;
        PROCESSROW(a1, a2, a3, a4, a5, a6, a7, a0) EXTENDCOL((block_width + marginX), 3) row++;
        PROCESSROW(a2, a3, a4, a5, a6, a7, a0, a1) EXTENDCOL((block_width + marginX), 3) row++;
        PROCESSROW(a3, a4, a5, a6, a7, a0, a1, a2) EXTENDCOL((block_width + marginX), 3) row++;
        PROCESSROW(a4, a5, a6, a7, a0, a1, a2, a3) EXTENDCOL((block_width + marginX), 3) row++;
        PROCESSROW(a5, a6, a7, a0, a1, a2, a3, a4) EXTENDCOL((block_width + marginX), 3) row++;
        PROCESSROW(a6, a7, a0, a1, a2, a3, a4, a5) EXTENDCOL((block_width + marginX), 3) row++;
        PROCESSROW(a7, a0, a1, a2, a3, a4, a5, a6) EXTENDCOL((block_width + marginX), 3)
    }

    // Extending bottom rows
    pixel *pe, *pi, *pp;
    pe = dstE + (block_height - 1) * dstStride - marginX;
    pi = dstI + (block_height - 1) * dstStride - marginX;
    pp = dstP + (block_height - 1) * dstStride - marginX;
    for (int y = 1; y <= marginY; y++)
    {
        memcpy(pe + y * dstStride, pe, block_width + marginX * 2);
    }

    for (int y = 1; y <= marginY; y++)
    {
        memcpy(pi + y * dstStride, pi, block_width + marginX * 2);
    }

    for (int y = 1; y <= marginY; y++)
    {
        memcpy(pp + y * dstStride, pp, block_width + marginX * 2);
    }

    // Extending top rows
    pe -= ((block_height - 1) * dstStride);
    pi -= ((block_height - 1) * dstStride);
    pp -= ((block_height - 1) * dstStride);
    for (int y = 1; y <= marginY; y++)
    {
        memcpy(pe - y * dstStride, pe, block_width + marginX * 2);
    }

    for (int y = 1; y <= marginY; y++)
    {
        memcpy(pi - y * dstStride, pi, block_width + marginX * 2);
    }

    for (int y = 1; y <= marginY; y++)
    {
        memcpy(pp - y * dstStride, pp, block_width + marginX * 2);
    }
}

#define PROCESSROWWGHTD(a0, a1, a2, a3, a4, a5, a6, a7) { \
        tmp = _mm_loadu_si128((__m128i const*)(src + col + (row + 7) * srcStride)); \
        a7 = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15)); \
        exp1 = _mm_sub_epi32(_mm_sub_epi32(_mm_sll_epi32(a1, _mm_cvtsi32_si128(2)), a0), _mm_mullo_epi32(a2, _mm_set1_epi32(10))); \
        exp2 = _mm_mullo_epi32(a3, _mm_set1_epi32(40)); \
        exp3 = _mm_mullo_epi32(a3, _mm_set1_epi32(17)); \
        exp4 = _mm_mullo_epi32(a4, _mm_set1_epi32(17)); \
        exp5 = _mm_mullo_epi32(a4, _mm_set1_epi32(40)); \
        exp6 = _mm_sub_epi32(_mm_sub_epi32(_mm_sll_epi32(a6, _mm_cvtsi32_si128(2)), a7), _mm_mullo_epi32(a5, _mm_set1_epi32(10))); \
        sume = _mm_add_epi32(exp1, _mm_add_epi32(_mm_add_epi32(exp2, exp3), \
                                                 _mm_add_epi32(_mm_add_epi32(a3, exp4), \
                                                               _mm_add_epi32(_mm_mullo_epi32(a5, _mm_set1_epi32(-5)), \
                                                                             a6) \
                                                               ) \
                                                 ) \
                             ); \
        sumi = _mm_sub_epi32(_mm_add_epi32(_mm_add_epi32(exp1, exp2), _mm_add_epi32(exp5, exp6)), \
                             _mm_add_epi32(a2, a5)); \
        sump = _mm_add_epi32(a1, _mm_add_epi32(_mm_add_epi32(exp3, exp4), \
                                               _mm_add_epi32(_mm_add_epi32(exp5, exp6), \
                                                             _mm_add_epi32(_mm_mullo_epi32(a2, _mm_set1_epi32(-5)), \
                                                                           a4) \
                                                             ) \
                                               ) \
                             ); \
        /* store results */ \
        sumi = _mm_srai_epi32(sumi, IF_FILTER_PREC); \
        tmp =  _mm_packus_epi32(_mm_and_si128(sumi, _mm_set1_epi32(0x0000FFFF)), _mm_set1_epi32(0)); \
        sumi = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15)); \
        /*Apply weight*/    \
        sumi = _mm_mullo_epi32(sumi, vscale);   \
        sumi = _mm_add_epi32(sumi, vround); \
        sumi = _mm_srai_epi32(sumi, wshift); \
        sumi = _mm_add_epi32(sumi, ofs);    \
        tmp  =  _mm_packs_epi32(sumi, _mm_setzero_si128()); \
        sumi = _mm_packus_epi16(tmp, _mm_setzero_si128()); \
        *(uint32_t*)(dstI + row * dstStride + col) = _mm_cvtsi128_si32(sumi); \
        sume = _mm_srai_epi32(sume, IF_FILTER_PREC); \
        tmp =  _mm_packus_epi32(_mm_and_si128(sume, _mm_set1_epi32(0x0000FFFF)), _mm_set1_epi32(0)); \
        sume = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15)); \
        /*Apply weight*/    \
        sume = _mm_mullo_epi32(sume, vscale);   \
        sume = _mm_add_epi32(sume, vround); \
        sume = _mm_srai_epi32(sume, wshift); \
        sume = _mm_add_epi32(sume, ofs);    \
        tmp  =  _mm_packs_epi32(sume, _mm_setzero_si128()); \
        sume = _mm_packus_epi16(tmp, _mm_setzero_si128()); \
        *(uint32_t*)(dstE + row * dstStride + col) = _mm_cvtsi128_si32(sume); \
        sump = _mm_srai_epi32(sump, IF_FILTER_PREC); \
        tmp =  _mm_packus_epi32(_mm_and_si128(sump, _mm_set1_epi32(0x0000FFFF)), _mm_set1_epi32(0)); \
        sump = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15)); \
        /*Apply weight*/    \
        sump = _mm_mullo_epi32(sump, vscale);   \
        sump = _mm_add_epi32(sump, vround); \
        sump = _mm_srai_epi32(sump, wshift); \
        sump = _mm_add_epi32(sump, ofs);    \
        tmp  =  _mm_packs_epi32(sump, _mm_setzero_si128()); \
        sump = _mm_packus_epi16(tmp, _mm_setzero_si128()); \
        *(uint32_t*)(dstP + row * dstStride + col) = _mm_cvtsi128_si32(sump); \
}

void filterVerticalWeighted(short *src, int srcStride,
                            pixel *dstE, pixel *dstI, pixel *dstP, int dstStride,
                            int block_width, int block_height,
                            int marginX, int marginY,
                            int scale, int wround, int wshift, int woffset)
{
    int row, col;

    int headRoom = IF_INTERNAL_PREC - X265_DEPTH;
    int shift = IF_FILTER_PREC + headRoom;

    int offset = 1 << (shift - 1);

    offset +=  IF_INTERNAL_OFFS << IF_FILTER_PREC;

    src -= (8 / 2 - 1) * srcStride;

    __m128i tmp16e, tmp16i, tmp16p;
    __m128i a0, a1, a2, a3, a4, a5, a6, a7;
    __m128i tmp;
    __m128i sume, sumi, sump;
    __m128i exp1, exp2, exp3, exp4, exp5, exp6;

    Int shiftNum = IF_INTERNAL_PREC - X265_DEPTH;
    wshift = wshift + shiftNum;
    wround = wshift ? (1 << (wshift - 1)) : 0;

    __m128i vround = _mm_set1_epi32(wround + scale * IF_INTERNAL_OFFS);
    __m128i ofs = _mm_set1_epi32(woffset);
    __m128i vscale = _mm_set1_epi32(scale);

    col = 0;

    tmp = _mm_loadu_si128((__m128i const*)(src + col));
    a0  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
    tmp = _mm_loadu_si128((__m128i const*)(src + col + srcStride));
    a1  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
    tmp = _mm_loadu_si128((__m128i const*)(src + col + 2 * srcStride));
    a2  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
    tmp = _mm_loadu_si128((__m128i const*)(src + col + 3 * srcStride));
    a3  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
    tmp = _mm_loadu_si128((__m128i const*)(src + col + 4 * srcStride));
    a4  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
    tmp = _mm_loadu_si128((__m128i const*)(src + col + 5 * srcStride));
    a5  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
    tmp = _mm_loadu_si128((__m128i const*)(src + col + 6 * srcStride));
    a6  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));

    for (row = 0; row < block_height; row++)
    {
        PROCESSROWWGHTD(a0, a1, a2, a3, a4, a5, a6, a7) EXTENDCOL(0, 0) row++;
        PROCESSROWWGHTD(a1, a2, a3, a4, a5, a6, a7, a0) EXTENDCOL(0, 0) row++;
        PROCESSROWWGHTD(a2, a3, a4, a5, a6, a7, a0, a1) EXTENDCOL(0, 0) row++;
        PROCESSROWWGHTD(a3, a4, a5, a6, a7, a0, a1, a2) EXTENDCOL(0, 0) row++;
        PROCESSROWWGHTD(a4, a5, a6, a7, a0, a1, a2, a3) EXTENDCOL(0, 0) row++;
        PROCESSROWWGHTD(a5, a6, a7, a0, a1, a2, a3, a4) EXTENDCOL(0, 0) row++;
        PROCESSROWWGHTD(a6, a7, a0, a1, a2, a3, a4, a5) EXTENDCOL(0, 0) row++;
        PROCESSROWWGHTD(a7, a0, a1, a2, a3, a4, a5, a6) EXTENDCOL(0, 0)
    }

    col += 4;

    for (; col < block_width - 4; col += 4)         // Considering block width is always a multiple of 4
    {
        tmp = _mm_loadu_si128((__m128i const*)(src + col));
        a0  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
        tmp = _mm_loadu_si128((__m128i const*)(src + col + srcStride));
        a1  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
        tmp = _mm_loadu_si128((__m128i const*)(src + col + 2 * srcStride));
        a2  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
        tmp = _mm_loadu_si128((__m128i const*)(src + col + 3 * srcStride));
        a3  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
        tmp = _mm_loadu_si128((__m128i const*)(src + col + 4 * srcStride));
        a4  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
        tmp = _mm_loadu_si128((__m128i const*)(src + col + 5 * srcStride));
        a5  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
        tmp = _mm_loadu_si128((__m128i const*)(src + col + 6 * srcStride));
        a6  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));

        for (row = 0; row < block_height; row++)
        {
            PROCESSROWWGHTD(a0, a1, a2, a3, a4, a5, a6, a7) row++;
            PROCESSROWWGHTD(a1, a2, a3, a4, a5, a6, a7, a0) row++;
            PROCESSROWWGHTD(a2, a3, a4, a5, a6, a7, a0, a1) row++;
            PROCESSROWWGHTD(a3, a4, a5, a6, a7, a0, a1, a2) row++;
            PROCESSROWWGHTD(a4, a5, a6, a7, a0, a1, a2, a3) row++;
            PROCESSROWWGHTD(a5, a6, a7, a0, a1, a2, a3, a4) row++;
            PROCESSROWWGHTD(a6, a7, a0, a1, a2, a3, a4, a5) row++;
            PROCESSROWWGHTD(a7, a0, a1, a2, a3, a4, a5, a6)
        }
    }

    tmp = _mm_loadu_si128((__m128i const*)(src + col));
    a0  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
    tmp = _mm_loadu_si128((__m128i const*)(src + col + srcStride));
    a1  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
    tmp = _mm_loadu_si128((__m128i const*)(src + col + 2 * srcStride));
    a2  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
    tmp = _mm_loadu_si128((__m128i const*)(src + col + 3 * srcStride));
    a3  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
    tmp = _mm_loadu_si128((__m128i const*)(src + col + 4 * srcStride));
    a4  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
    tmp = _mm_loadu_si128((__m128i const*)(src + col + 5 * srcStride));
    a5  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
    tmp = _mm_loadu_si128((__m128i const*)(src + col + 6 * srcStride));
    a6  = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));

    for (row = 0; row < block_height; row++)
    {
        PROCESSROWWGHTD(a0, a1, a2, a3, a4, a5, a6, a7) EXTENDCOL((block_width + marginX), 3) row++;
        PROCESSROWWGHTD(a1, a2, a3, a4, a5, a6, a7, a0) EXTENDCOL((block_width + marginX), 3) row++;
        PROCESSROWWGHTD(a2, a3, a4, a5, a6, a7, a0, a1) EXTENDCOL((block_width + marginX), 3) row++;
        PROCESSROWWGHTD(a3, a4, a5, a6, a7, a0, a1, a2) EXTENDCOL((block_width + marginX), 3) row++;
        PROCESSROWWGHTD(a4, a5, a6, a7, a0, a1, a2, a3) EXTENDCOL((block_width + marginX), 3) row++;
        PROCESSROWWGHTD(a5, a6, a7, a0, a1, a2, a3, a4) EXTENDCOL((block_width + marginX), 3) row++;
        PROCESSROWWGHTD(a6, a7, a0, a1, a2, a3, a4, a5) EXTENDCOL((block_width + marginX), 3) row++;
        PROCESSROWWGHTD(a7, a0, a1, a2, a3, a4, a5, a6) EXTENDCOL((block_width + marginX), 3)
    }

    // Extending bottom rows
    pixel *pe, *pi, *pp;
    pe = dstE + (block_height - 1) * dstStride - marginX;
    pi = dstI + (block_height - 1) * dstStride - marginX;
    pp = dstP + (block_height - 1) * dstStride - marginX;
    for (int y = 1; y <= marginY; y++)
    {
        memcpy(pe + y * dstStride, pe, block_width + marginX * 2);
    }

    for (int y = 1; y <= marginY; y++)
    {
        memcpy(pi + y * dstStride, pi, block_width + marginX * 2);
    }

    for (int y = 1; y <= marginY; y++)
    {
        memcpy(pp + y * dstStride, pp, block_width + marginX * 2);
    }

    // Extending top rows
    pe -= ((block_height - 1) * dstStride);
    pi -= ((block_height - 1) * dstStride);
    pp -= ((block_height - 1) * dstStride);
    for (int y = 1; y <= marginY; y++)
    {
        memcpy(pe - y * dstStride, pe, block_width + marginX * 2);
    }

    for (int y = 1; y <= marginY; y++)
    {
        memcpy(pi - y * dstStride, pi, block_width + marginX * 2);
    }

    for (int y = 1; y <= marginY; y++)
    {
        memcpy(pp - y * dstStride, pp, block_width + marginX * 2);
    }
}

template<int N>
void filterVertical_p_p(pixel *src, int srcStride,
                        pixel *dst, int dstStride,
                        int width, int height,
                        const short *coeff)
{
    int offset;
    int shift = IF_FILTER_PREC;

    src -= (N / 2 - 1) * srcStride;
    offset = 1 << (shift - 1);

    __m128i coeffTemp = _mm_loadu_si128((__m128i const*)coeff);

    __m128i vm0 = _mm_setr_epi8(0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1);
    __m128i vm1 = _mm_setr_epi8(2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3);
    __m128i vm2 = _mm_setr_epi8(4, 5, 4, 5, 4, 5, 4, 5, 4, 5, 4, 5, 4, 5, 4, 5);
    __m128i vm3 = _mm_setr_epi8(6, 7, 6, 7, 6, 7, 6, 7, 6, 7, 6, 7, 6, 7, 6, 7);
    __m128i vm4 = _mm_setr_epi8(8, 9, 8, 9, 8, 9, 8, 9, 8, 9, 8, 9, 8, 9, 8, 9);
    __m128i vm5 = _mm_setr_epi8(10, 11, 10, 11, 10, 11, 10, 11, 10, 11, 10, 11, 10, 11, 10, 11);
    __m128i vm6 = _mm_setr_epi8(12, 13, 12, 13, 12, 13, 12, 13, 12, 13, 12, 13, 12, 13, 12, 13);
    __m128i vm7 = _mm_setr_epi8(14, 15, 14, 15, 14, 15, 14, 15, 14, 15, 14, 15, 14, 15, 14, 15);

    __m128i coeff0 = _mm_shuffle_epi8(coeffTemp, vm0);
    __m128i coeff1 = _mm_shuffle_epi8(coeffTemp, vm1);
    __m128i coeff2 = _mm_shuffle_epi8(coeffTemp, vm2);
    __m128i coeff3 = _mm_shuffle_epi8(coeffTemp, vm3);
    __m128i coeff4 = _mm_shuffle_epi8(coeffTemp, vm4);
    __m128i coeff5 = _mm_shuffle_epi8(coeffTemp, vm5);
    __m128i coeff6 = _mm_shuffle_epi8(coeffTemp, vm6);
    __m128i coeff7 = _mm_shuffle_epi8(coeffTemp, vm7);

    __m128i mask7 = _mm_setr_epi8(-1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    int row, col;

    for (row = 0; row < height; row++)
    {
        for (col = 0; col < (width - 15); col += 16)
        {
            __m128i srcCoeff = _mm_loadu_si128((__m128i*)&src[col]);
            __m128i srcCoeffTemp1 = _mm_cvtepu8_epi16(srcCoeff);
            __m128i T00 = _mm_mullo_epi16(srcCoeffTemp1, coeff0);
            srcCoeff = _mm_srli_si128(srcCoeff, 8);
            srcCoeff = _mm_cvtepu8_epi16(srcCoeff);
            __m128i T01 = _mm_mullo_epi16(srcCoeff, coeff0);

            srcCoeff = _mm_loadu_si128((__m128i const*)(&src[col + srcStride]));
            __m128i srcCoeffTemp2 = _mm_cvtepu8_epi16(srcCoeff);
            __m128i T10 = _mm_mullo_epi16(srcCoeffTemp2, coeff1);
            srcCoeff = _mm_srli_si128(srcCoeff, 8);
            srcCoeff = _mm_cvtepu8_epi16(srcCoeff);
            __m128i T11 = _mm_mullo_epi16(srcCoeff, coeff1);

            srcCoeff = _mm_loadu_si128((__m128i const*)(&src[col + 2 * srcStride]));
            __m128i srcCoeffTemp3 = _mm_cvtepu8_epi16(srcCoeff);
            __m128i T20 = _mm_mullo_epi16(srcCoeffTemp3, coeff2);
            srcCoeff = _mm_srli_si128(srcCoeff, 8);
            srcCoeff = _mm_cvtepu8_epi16(srcCoeff);
            __m128i T21 = _mm_mullo_epi16(srcCoeff, coeff2);

            srcCoeff = _mm_loadu_si128((__m128i const*)(&src[col + 3 * srcStride]));
            __m128i srcCoeffTemp4 = _mm_cvtepu8_epi16(srcCoeff);
            __m128i T30 = _mm_mullo_epi16(srcCoeffTemp4, coeff3);
            srcCoeff = _mm_srli_si128(srcCoeff, 8);
            srcCoeff = _mm_cvtepu8_epi16(srcCoeff);
            __m128i T31 = _mm_mullo_epi16(srcCoeff, coeff3);

            __m128i sum0 = _mm_add_epi16(T00, T10);
            __m128i sum1 = _mm_add_epi16(T20, T30);
            __m128i sumlo = _mm_add_epi16(sum0, sum1);

            __m128i sum2 = _mm_add_epi16(T01, T11);
            __m128i sum3 = _mm_add_epi16(T21, T31);
            __m128i sumhi = _mm_add_epi16(sum2, sum3);

            if (N == 8)
            {
                srcCoeff = _mm_loadu_si128((__m128i const*)(&src[col + 4 * srcStride]));
                srcCoeffTemp1 = _mm_cvtepu8_epi16(srcCoeff);
                T00 = _mm_mullo_epi16(srcCoeffTemp1, coeff4);
                srcCoeff = _mm_srli_si128(srcCoeff, 8);
                srcCoeff = _mm_cvtepu8_epi16(srcCoeff);
                T01 = _mm_mullo_epi16(srcCoeff, coeff4);

                srcCoeff = _mm_loadu_si128((__m128i const*)(&src[col + 5 * srcStride]));
                srcCoeffTemp2 = _mm_cvtepu8_epi16(srcCoeff);
                T10 = _mm_mullo_epi16(srcCoeffTemp2, coeff5);
                srcCoeff = _mm_srli_si128(srcCoeff, 8);
                srcCoeff = _mm_cvtepu8_epi16(srcCoeff);
                T11 = _mm_mullo_epi16(srcCoeff, coeff5);

                srcCoeff = _mm_loadu_si128((__m128i const*)(&src[col + 6 * srcStride]));
                srcCoeffTemp3 = _mm_cvtepu8_epi16(srcCoeff);
                T20 = _mm_mullo_epi16(srcCoeffTemp3, coeff6);
                srcCoeff = _mm_srli_si128(srcCoeff, 8);
                srcCoeff = _mm_cvtepu8_epi16(srcCoeff);
                T21 = _mm_mullo_epi16(srcCoeff, coeff6);

                srcCoeff = _mm_loadu_si128((__m128i const*)(&src[col + 7 * srcStride]));
                srcCoeffTemp4 = _mm_cvtepu8_epi16(srcCoeff);
                T30 = _mm_mullo_epi16(srcCoeffTemp4, coeff7);
                srcCoeff = _mm_srli_si128(srcCoeff, 8);
                srcCoeff = _mm_cvtepu8_epi16(srcCoeff);
                T31 = _mm_mullo_epi16(srcCoeff, coeff7);

                sum0 = _mm_add_epi16(T00, T10);
                sum1 = _mm_add_epi16(T20, T30);
                sumlo = _mm_add_epi16(sumlo, _mm_add_epi16(sum0, sum1));

                sum2 = _mm_add_epi16(T01, T11);
                sum3 = _mm_add_epi16(T21, T31);
                sumhi = _mm_add_epi16(sumhi, _mm_add_epi16(sum2, sum3));
            }

            __m128i sumOffset = _mm_set1_epi16(offset);

            __m128i val1 = _mm_add_epi16(sumlo, sumOffset);
            val1 = _mm_srai_epi16(val1, shift);

            __m128i val2 = _mm_add_epi16(sumhi, sumOffset);
            val2 = _mm_srai_epi16(val2, shift);

            __m128i res = _mm_packus_epi16(val1, val2);
            _mm_storeu_si128((__m128i*)&dst[col], res);
        }

        for (; col < (width - 7); col += 8)
        {
            __m128i srcCoeff = _mm_loadl_epi64((__m128i*)&src[col]);
            __m128i srcCoeffTemp1 = _mm_cvtepu8_epi16(srcCoeff);
            __m128i T00 = _mm_mullo_epi16(srcCoeffTemp1, coeff0);

            srcCoeff = _mm_loadl_epi64((__m128i const*)(&src[col + srcStride]));
            __m128i srcCoeffTemp2 = _mm_cvtepu8_epi16(srcCoeff);
            __m128i T10 = _mm_mullo_epi16(srcCoeffTemp2, coeff1);

            srcCoeff = _mm_loadl_epi64((__m128i const*)(&src[col + 2 * srcStride]));
            __m128i srcCoeffTemp3 = _mm_cvtepu8_epi16(srcCoeff);
            __m128i T20 = _mm_mullo_epi16(srcCoeffTemp3, coeff2);

            srcCoeff = _mm_loadl_epi64((__m128i const*)(&src[col + 3 * srcStride]));
            __m128i srcCoeffTemp4 = _mm_cvtepu8_epi16(srcCoeff);
            __m128i T30 = _mm_mullo_epi16(srcCoeffTemp4, coeff3);

            __m128i sum0 = _mm_add_epi16(T00, T10);
            __m128i sum1 = _mm_add_epi16(T20, T30);
            __m128i sumlo = _mm_add_epi16(sum0, sum1);

            if (N == 8)
            {
                srcCoeff = _mm_loadl_epi64((__m128i const*)(&src[col + 4 * srcStride]));
                srcCoeffTemp1 = _mm_cvtepu8_epi16(srcCoeff);
                T00 = _mm_mullo_epi16(srcCoeffTemp1, coeff4);

                srcCoeff = _mm_loadl_epi64((__m128i const*)(&src[col + 5 * srcStride]));
                srcCoeffTemp2 = _mm_cvtepu8_epi16(srcCoeff);
                T10 = _mm_mullo_epi16(srcCoeffTemp2, coeff5);

                srcCoeff = _mm_loadl_epi64((__m128i const*)(&src[col + 6 * srcStride]));
                srcCoeffTemp3 = _mm_cvtepu8_epi16(srcCoeff);
                T20 = _mm_mullo_epi16(srcCoeffTemp3, coeff6);

                srcCoeff = _mm_loadl_epi64((__m128i const*)(&src[col + 7 * srcStride]));
                srcCoeffTemp4 = _mm_cvtepu8_epi16(srcCoeff);
                T30 = _mm_mullo_epi16(srcCoeffTemp4, coeff7);

                sum0 = _mm_add_epi16(T00, T10);
                sum1 = _mm_add_epi16(T20, T30);
                sumlo = _mm_add_epi16(sumlo, _mm_add_epi16(sum0, sum1));
            }
            __m128i zero = _mm_set1_epi16(0);
            __m128i sumOffset = _mm_set1_epi16(offset);

            __m128i val1 = _mm_add_epi16(sumlo, sumOffset);
            val1 = _mm_srai_epi16(val1, shift);

            __m128i res = _mm_packus_epi16(val1, zero);
            _mm_storel_epi64((__m128i*)&dst[col], res);
        }

        for (; col < width; col += 8)
        {
            __m128i srcCoeff = _mm_loadl_epi64((__m128i*)&src[col]);
            __m128i srcCoeffTemp1 = _mm_cvtepu8_epi16(srcCoeff);
            __m128i T00 = _mm_mullo_epi16(srcCoeffTemp1, coeff0);

            srcCoeff = _mm_loadl_epi64((__m128i const*)(&src[col + srcStride]));
            __m128i srcCoeffTemp2 = _mm_cvtepu8_epi16(srcCoeff);
            __m128i T10 = _mm_mullo_epi16(srcCoeffTemp2, coeff1);

            srcCoeff = _mm_loadl_epi64((__m128i const*)(&src[col + 2 * srcStride]));
            __m128i srcCoeffTemp3 = _mm_cvtepu8_epi16(srcCoeff);
            __m128i T20 = _mm_mullo_epi16(srcCoeffTemp3, coeff2);

            srcCoeff = _mm_loadl_epi64((__m128i const*)(&src[col + 3 * srcStride]));
            __m128i srcCoeffTemp4 = _mm_cvtepu8_epi16(srcCoeff);
            __m128i T30 = _mm_mullo_epi16(srcCoeffTemp4, coeff3);

            __m128i sum0 = _mm_add_epi16(T00, T10);
            __m128i sum1 = _mm_add_epi16(T20, T30);
            __m128i sumlo = _mm_add_epi16(sum0, sum1);

            if (N == 8)
            {
                srcCoeff = _mm_loadl_epi64((__m128i const*)(&src[col + 4 * srcStride]));
                srcCoeffTemp1 = _mm_cvtepu8_epi16(srcCoeff);
                T00 = _mm_mullo_epi16(srcCoeffTemp1, coeff4);

                srcCoeff = _mm_loadl_epi64((__m128i const*)(&src[col + 5 * srcStride]));
                srcCoeffTemp2 = _mm_cvtepu8_epi16(srcCoeff);
                T10 = _mm_mullo_epi16(srcCoeffTemp2, coeff5);

                srcCoeff = _mm_loadl_epi64((__m128i const*)(&src[col + 6 * srcStride]));
                srcCoeffTemp3 = _mm_cvtepu8_epi16(srcCoeff);
                T20 = _mm_mullo_epi16(srcCoeffTemp3, coeff6);

                srcCoeff = _mm_loadl_epi64((__m128i const*)(&src[col + 7 * srcStride]));
                srcCoeffTemp4 = _mm_cvtepu8_epi16(srcCoeff);
                T30 = _mm_mullo_epi16(srcCoeffTemp4, coeff7);

                sum0 = _mm_add_epi16(T00, T10);
                sum1 = _mm_add_epi16(T20, T30);
                sumlo = _mm_add_epi16(sumlo, _mm_add_epi16(sum0, sum1));
            }
            __m128i zero = _mm_set1_epi16(0);
            __m128i sumOffset = _mm_set1_epi16(offset);

            __m128i val1 = _mm_add_epi16(sumlo, sumOffset);
            val1 = _mm_srai_epi16(val1, shift);

            __m128i res = _mm_packus_epi16(val1, zero);

            int n = width - col;
            __m128i mask1, mask2, mask3, mask4, mask5, mask6;

            switch (n) // store either 1, 2, 3, 4, 5, 6, or 7 8-bit results in dst
            {
            case 1: mask1 = _mm_srli_si128(mask7, 6);
                _mm_maskmoveu_si128(res, mask1, (char*)&dst[col]);
                break;

            case 2: mask2 = _mm_srli_si128(mask7, 5);
                _mm_maskmoveu_si128(res, mask2, (char*)&dst[col]);
                break;

            case 3: mask3 = _mm_srli_si128(mask7, 4);
                _mm_maskmoveu_si128(res, mask3, (char*)&dst[col]);
                break;

            case 4: mask4 = _mm_srli_si128(mask7, 3);
                _mm_maskmoveu_si128(res, mask4, (char*)&dst[col]);
                break;

            case 5: mask5 = _mm_srli_si128(mask7, 2);
                _mm_maskmoveu_si128(res, mask5, (char*)&dst[col]);
                break;

            case 6: mask6 = _mm_srli_si128(mask7, 1);
                _mm_maskmoveu_si128(res, mask6, (char*)&dst[col]);
                break;

            case 7: _mm_maskmoveu_si128(res, mask7, (char*)&dst[col]);
                break;
            }
        }

        src += srcStride;
        dst += dstStride;
    }
}

#endif /* if INSTRSET >= X265_CPU_LEVEL_SSSE3 */

#if INSTRSET >= X265_CPU_LEVEL_SSE41
template<int N>
void filterHorizontal_p_p(pixel *src, int srcStride, pixel *dst, int dstStride, int width, int height, short const *coeff)
{
    int row, col;
    int offset;
    short maxVal;
    int headRoom = IF_INTERNAL_PREC - X265_DEPTH;

    offset =  (1 << (headRoom - 1));
    maxVal = (1 << X265_DEPTH) - 1;
    src -= (N / 2 - 1);

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

            else // (N == 8)
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
            __m128i zero = _mm_set1_epi16(0);
            __m128i val = _mm_add_epi16(sum, sumOffset);

            val = _mm_srai_epi16(val, headRoom);
            val = _mm_packus_epi16(val, zero);
            _mm_storel_epi64((__m128i*)&dst[col], val);
        }

        for (; col < width; col++)                        // Remaining iterations
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
            short val = (short)(sum + offset) >> headRoom;
            val = (val < 0) ? 0 : val;
            val = (val > maxVal) ? maxVal : val;
            dst[col] = (pixel)val;
        }

        src += srcStride;
        dst += dstStride;
    }
}

void filterHorizontalMultiplaneExtend(pixel *src, int srcStride,
                                      short *intF, short* intA, short* intB, short* intC, int intStride,
                                      pixel *dstA, pixel *dstB, pixel *dstC, int dstStride,
                                      int block_width, int block_height,
                                      int marginX, int marginY)
{
    int row, col;
    int headRoom = IF_INTERNAL_PREC - X265_DEPTH;
    int shift = IF_FILTER_PREC - headRoom;
    int offset = -IF_INTERNAL_OFFS << shift;

    src -= (8 / 2 - 1);
    __m128i vec_src0;
    __m128i vec_offset = _mm_set1_epi16(offset);
    __m128i sumaL, sumbL, sumcL, tmp, exp1;
    __m128i tmp16a, tmp16b, tmp16c;

    // Load Ai, ai += Ai*coefi
    for (row = 0; row < block_height; row++)
    {
        col = 0;

        vec_src0 = _mm_loadu_si128((__m128i const*)(src + col));
        sumbL = (_mm_unpacklo_epi8(vec_src0, _mm_setzero_si128()));
        sumbL = _mm_sub_epi16(_mm_setzero_si128(), sumbL);

        // a = b+=4*a1,  c+=1*a1
        vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 1));
        sumcL = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
        sumbL = _mm_add_epi16(sumbL, _mm_sll_epi16(sumcL, _mm_cvtsi32_si128(2)));
        sumaL = sumbL;

        // a +=-10*a2    b+=-11*a2      c+=-5*a2
        vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 2));
        tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
        sumbL = _mm_sub_epi16(sumbL, tmp);
        tmp = _mm_mullo_epi16(tmp, _mm_set1_epi16(-5));
        sumcL = _mm_add_epi16(sumcL, tmp);
        tmp = _mm_sll_epi16(tmp, _mm_cvtsi32_si128(1));
        sumaL = _mm_add_epi16(sumaL, tmp);
        sumbL = _mm_add_epi16(sumbL, tmp);

        // a +=58*a3    b+=40*a3      c+=17*a3
        vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 3));
        tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
        _mm_storeu_si128((__m128i*)(intF + col), _mm_sub_epi16(_mm_sll_epi16(tmp, _mm_cvtsi32_si128(6)), _mm_set1_epi16(IF_INTERNAL_OFFS)));
        exp1 = _mm_add_epi16(tmp, _mm_sll_epi16(tmp, _mm_cvtsi32_si128(4)));
        sumcL = _mm_add_epi16(sumcL, exp1);
        sumaL = _mm_add_epi16(sumaL, tmp);
        tmp = _mm_mullo_epi16(tmp, _mm_set1_epi16(40));
        sumbL = _mm_add_epi16(sumbL, tmp);
        sumaL = _mm_add_epi16(sumaL, _mm_add_epi16(exp1, tmp));

        // a +=17*a4    b+=40*a4      c+=58*a4
        vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 4));
        tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
        exp1 = _mm_add_epi16(tmp, _mm_sll_epi16(tmp, _mm_cvtsi32_si128(4)));
        sumaL = _mm_add_epi16(sumaL, exp1);
        sumcL = _mm_add_epi16(sumcL, tmp);
        tmp = _mm_mullo_epi16(tmp, _mm_set1_epi16(40));
        sumbL = _mm_add_epi16(sumbL, tmp);
        sumcL = _mm_add_epi16(sumcL, _mm_add_epi16(exp1, tmp));

        // a +=-5*a5    b+=-11*a5      c+=-10*a5
        vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 5));
        tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
        sumbL = _mm_sub_epi16(sumbL, tmp);
        tmp = _mm_mullo_epi16(tmp, _mm_set1_epi16(-5));
        sumaL = _mm_add_epi16(sumaL, tmp);
        tmp = _mm_sll_epi16(tmp, _mm_cvtsi32_si128(1));
        sumcL = _mm_add_epi16(sumcL, tmp);
        sumbL = _mm_add_epi16(sumbL, tmp);

        // a +=1*a6    b+=4*a6      c+=4*a6
        vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 6));
        tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
        sumaL = _mm_add_epi16(sumaL, tmp);
        tmp = _mm_sll_epi16(tmp, _mm_cvtsi32_si128(2));
        sumbL = _mm_add_epi16(sumbL, tmp);
        sumcL = _mm_add_epi16(sumcL, tmp);

        // a +=0*a7    b+=-1*a7      c+=-1*a7
        vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 7));
        tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
        sumbL = _mm_sub_epi16(sumbL, tmp);
        sumcL = _mm_sub_epi16(sumcL, tmp);
        sumaL = _mm_add_epi16(sumaL, vec_offset);
        sumbL = _mm_add_epi16(sumbL, vec_offset);
        sumcL = _mm_add_epi16(sumcL, vec_offset);

        _mm_storeu_si128((__m128i*)(intA + col), sumaL);
        sumaL = _mm_add_epi16(sumaL, _mm_set1_epi16(IF_INTERNAL_OFFS + 32));
        sumaL = _mm_sra_epi16(sumaL, _mm_cvtsi32_si128(6));
        tmp16a = _mm_packus_epi16(sumaL, sumaL);
        _mm_storel_epi64((__m128i*)(dstA + row * dstStride + col), tmp16a);

        _mm_storeu_si128((__m128i*)(intB + col), sumbL);
        sumbL = _mm_add_epi16(sumbL, _mm_set1_epi16(IF_INTERNAL_OFFS + 32));
        sumbL = _mm_sra_epi16(sumbL, _mm_cvtsi32_si128(6));
        tmp16b = _mm_packus_epi16(sumbL, sumbL);
        _mm_storel_epi64((__m128i*)(dstB + row * dstStride + col), tmp16b);

        _mm_storeu_si128((__m128i*)(intC + col), sumcL);
        sumcL = _mm_add_epi16(sumcL, _mm_set1_epi16(IF_INTERNAL_OFFS + 32));
        sumcL = _mm_sra_epi16(sumcL, _mm_cvtsi32_si128(6));
        tmp16c = _mm_packus_epi16(sumcL, sumcL);
        _mm_storel_epi64((__m128i*)(dstC + row * dstStride + col), tmp16c);

        // Extend First column
        __m128i ma, mb, mc;
        ma = _mm_shuffle_epi8(tmp16a, _mm_set1_epi8(0));
        mb = _mm_shuffle_epi8(tmp16b, _mm_set1_epi8(0));
        mc = _mm_shuffle_epi8(tmp16c, _mm_set1_epi8(0));

        for (int i = -marginX; i < -16; i += 16)
        {
            _mm_storeu_si128((__m128i*)(dstA + row * dstStride +  i), ma);
            _mm_storeu_si128((__m128i*)(dstB + row * dstStride +  i), mb);
            _mm_storeu_si128((__m128i*)(dstC + row * dstStride +  i), mc);
        }

        _mm_storeu_si128((__m128i*)(dstA + row * dstStride - 16), ma); /*Assuming marginX > 16*/
        _mm_storeu_si128((__m128i*)(dstB + row * dstStride - 16), mb);
        _mm_storeu_si128((__m128i*)(dstC + row * dstStride - 16), mc);

        col += 8;

        for (; col + 8 /*16*/ <= (block_width); col += 8 /*16*/)               // Iterations multiple of 8
        {
            vec_src0 = _mm_loadu_si128((__m128i const*)(src + col));
            sumbL = (_mm_unpacklo_epi8(vec_src0, _mm_setzero_si128()));
            sumbL = _mm_sub_epi16(_mm_setzero_si128(), sumbL);

            // a = b+=4*a1,  c+=1*a1
            vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 1));
            sumcL = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
            sumbL = _mm_add_epi16(sumbL, _mm_sll_epi16(sumcL, _mm_cvtsi32_si128(2)));
            sumaL = sumbL;

            // a +=-10*a2    b+=-11*a2      c+=-5*a2
            vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 2));
            tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
            sumbL = _mm_sub_epi16(sumbL, tmp);
            tmp = _mm_mullo_epi16(tmp, _mm_set1_epi16(-5));
            sumcL = _mm_add_epi16(sumcL, tmp);
            tmp = _mm_sll_epi16(tmp, _mm_cvtsi32_si128(1));
            sumaL = _mm_add_epi16(sumaL, tmp);
            sumbL = _mm_add_epi16(sumbL, tmp);

            // a +=58*a3    b+=40*a3      c+=17*a3
            vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 3));
            tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
            _mm_storeu_si128((__m128i*)(intF + col), _mm_sub_epi16(_mm_sll_epi16(tmp, _mm_cvtsi32_si128(6)), _mm_set1_epi16(IF_INTERNAL_OFFS)));
            exp1 = _mm_add_epi16(tmp, _mm_sll_epi16(tmp, _mm_cvtsi32_si128(4)));
            sumcL = _mm_add_epi16(sumcL, exp1);
            sumaL = _mm_add_epi16(sumaL, tmp);
            tmp = _mm_mullo_epi16(tmp, _mm_set1_epi16(40));
            sumbL = _mm_add_epi16(sumbL, tmp);
            sumaL = _mm_add_epi16(sumaL, _mm_add_epi16(exp1, tmp));

            // a +=17*a4    b+=40*a4      c+=58*a4
            vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 4));
            tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
            exp1 = _mm_add_epi16(tmp, _mm_sll_epi16(tmp, _mm_cvtsi32_si128(4)));
            sumaL = _mm_add_epi16(sumaL, exp1);
            sumcL = _mm_add_epi16(sumcL, tmp);
            tmp = _mm_mullo_epi16(tmp, _mm_set1_epi16(40));
            sumbL = _mm_add_epi16(sumbL, tmp);
            sumcL = _mm_add_epi16(sumcL, _mm_add_epi16(exp1, tmp));

            // a +=-5*a5    b+=-11*a5      c+=-10*a5
            vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 5));
            tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
            sumbL = _mm_sub_epi16(sumbL, tmp);
            tmp = _mm_mullo_epi16(tmp, _mm_set1_epi16(-5));
            sumaL = _mm_add_epi16(sumaL, tmp);
            tmp = _mm_sll_epi16(tmp, _mm_cvtsi32_si128(1));
            sumcL = _mm_add_epi16(sumcL, tmp);
            sumbL = _mm_add_epi16(sumbL, tmp);

            // a +=1*a6    b+=4*a6      c+=4*a6
            vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 6));
            tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
            sumaL = _mm_add_epi16(sumaL, tmp);
            tmp = _mm_sll_epi16(tmp, _mm_cvtsi32_si128(2));
            sumbL = _mm_add_epi16(sumbL, tmp);
            sumcL = _mm_add_epi16(sumcL, tmp);

            // a +=0*a7    b+=-1*a7      c+=-1*a7
            vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 7));
            tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
            sumbL = _mm_sub_epi16(sumbL, tmp);
            sumcL = _mm_sub_epi16(sumcL, tmp);
            sumaL = _mm_add_epi16(sumaL, vec_offset);
            sumbL = _mm_add_epi16(sumbL, vec_offset);
            sumcL = _mm_add_epi16(sumcL, vec_offset);

            _mm_storeu_si128((__m128i*)(intA + col), sumaL);
            sumaL = _mm_add_epi16(sumaL, _mm_set1_epi16(IF_INTERNAL_OFFS + 32));
            sumaL = _mm_sra_epi16(sumaL, _mm_cvtsi32_si128(6));
            tmp16a = _mm_packus_epi16(sumaL, sumaL);
            _mm_storel_epi64((__m128i*)(dstA + row * dstStride + col), tmp16a);

            _mm_storeu_si128((__m128i*)(intB + col), sumbL);
            sumbL = _mm_add_epi16(sumbL, _mm_set1_epi16(IF_INTERNAL_OFFS + 32));
            sumbL = _mm_sra_epi16(sumbL, _mm_cvtsi32_si128(6));
            tmp16b = _mm_packus_epi16(sumbL, sumbL);
            _mm_storel_epi64((__m128i*)(dstB + row * dstStride + col), tmp16b);

            _mm_storeu_si128((__m128i*)(intC + col), sumcL);
            sumcL = _mm_add_epi16(sumcL, _mm_set1_epi16(IF_INTERNAL_OFFS + 32));
            sumcL = _mm_sra_epi16(sumcL, _mm_cvtsi32_si128(6));
            tmp16c = _mm_packus_epi16(sumcL, sumcL);
            _mm_storel_epi64((__m128i*)(dstC + row * dstStride + col), tmp16c);
        }

        if (block_width - col > 0)
        {
            vec_src0 = _mm_loadu_si128((__m128i const*)(src + block_width - 5));
            tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
            _mm_storeu_si128((__m128i*)(intF + block_width - 8), _mm_sub_epi16(_mm_sll_epi16(tmp, _mm_cvtsi32_si128(6)), _mm_set1_epi16(IF_INTERNAL_OFFS)));
            __m128i a, b, c, sum1, sum2, sum3 = _mm_setzero_si128();
            for (; col < block_width; col++)                           // Remaining iterations
            {
                vec_src0 = _mm_loadu_si128((__m128i const*)(src + col));
                tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());    // Assuming that there is no overflow (Everywhere in this function!)
                a = _mm_setr_epi16(-1, 4, -10, 58, 17,  -5, 1,  0);
                a = _mm_mullo_epi16(tmp, a);
                b = _mm_setr_epi16(-1, 4, -11, 40, 40, -11, 4, -1);
                b = _mm_mullo_epi16(tmp, b);
                c = _mm_setr_epi16(0, 1,  -5, 17, 58, -10, 4, -1);
                c = _mm_mullo_epi16(tmp, c);
                sum1  = _mm_hadd_epi16(a, b);                   // horizontally add 8 elements in 3 steps
                sum2  = _mm_hadd_epi16(c, c);
                sum2  = _mm_hadd_epi16(sum1, sum2);
                sum3  = _mm_hadd_epi16(sum2, sum2);
                sum3  = _mm_add_epi16(sum3, vec_offset);
                sum3  = _mm_sra_epi16(sum3, _mm_cvtsi32_si128(shift));
                intA[col] = _mm_cvtsi128_si32(sum3);
                intB[col] = _mm_extract_epi16(sum3, 1);
                intC[col] = _mm_extract_epi16(sum3, 2);
                sum3 = _mm_add_epi16(sum3, _mm_set1_epi16(IF_INTERNAL_OFFS + 32));
                sum3 = _mm_sra_epi16(sum3, _mm_cvtsi32_si128(6));
                sum3 = _mm_packus_epi16(sum3, sum3);
                dstA[row * dstStride + col] = _mm_extract_epi8(sum3, 0);
                dstB[row * dstStride + col] = _mm_extract_epi8(sum3, 1);
                dstC[row * dstStride + col] = _mm_extract_epi8(sum3, 2);
            }

            tmp16a = _mm_shuffle_epi8(sum3, _mm_set1_epi8(0));
            tmp16b = _mm_shuffle_epi8(sum3, _mm_set1_epi8(1));
            tmp16c = _mm_shuffle_epi8(sum3, _mm_set1_epi8(2));
        }
        else
        {
            tmp16a = _mm_shuffle_epi8(tmp16a, _mm_set1_epi8(15));
            tmp16b = _mm_shuffle_epi8(tmp16b, _mm_set1_epi8(15));
            tmp16c = _mm_shuffle_epi8(tmp16c, _mm_set1_epi8(15));
        }
        // Extend last column
        for (int i = -marginX; i < -16; i += 16)
        {
            _mm_storeu_si128((__m128i*)(dstA + row * dstStride + block_width + marginX + i), tmp16a);
            _mm_storeu_si128((__m128i*)(dstB + row * dstStride + block_width + marginX + i), tmp16b);
            _mm_storeu_si128((__m128i*)(dstC + row * dstStride + block_width + marginX + i), tmp16c);
        }

        _mm_storeu_si128((__m128i*)(dstA + row * dstStride + block_width + marginX - 16), tmp16a); /*Assuming marginX > 16*/
        _mm_storeu_si128((__m128i*)(dstB + row * dstStride + block_width + marginX - 16), tmp16b);
        _mm_storeu_si128((__m128i*)(dstC + row * dstStride + block_width + marginX - 16), tmp16c);

        src += srcStride;
        intF += intStride;
        intA += intStride;
        intB += intStride;
        intC += intStride;
    }

    // Extending bottom rows
    pixel *pe, *pi, *pp;
    pe = dstA + (block_height - 1) * dstStride - marginX;
    pi = dstB + (block_height - 1) * dstStride - marginX;
    pp = dstC + (block_height - 1) * dstStride - marginX;
    for (int y = 1; y <= marginY; y++)
    {
        memcpy(pe + y * dstStride, pe, block_width + marginX * 2);
    }

    for (int y = 1; y <= marginY; y++)
    {
        memcpy(pi + y * dstStride, pi, block_width + marginX * 2);
    }

    for (int y = 1; y <= marginY; y++)
    {
        memcpy(pp + y * dstStride, pp, block_width + marginX * 2);
    }

    // Extending top rows
    pe  = dstA - marginX;
    pi  = dstB - marginX;
    pp  = dstC - marginX;
    for (int y = 1; y <= marginY; y++)
    {
        memcpy(pe - y * dstStride, pe, block_width + marginX * 2);
    }

    for (int y = 1; y <= marginY; y++)
    {
        memcpy(pi - y * dstStride, pi, block_width + marginX * 2);
    }

    for (int y = 1; y <= marginY; y++)
    {
        memcpy(pp - y * dstStride, pp, block_width + marginX * 2);
    }
}

void filterHorizontalWeighted(pixel *src, int srcStride,
                              short *intF, short* intA, short* intB, short* intC, int intStride,
                              pixel * dstF, pixel *dstA, pixel *dstB, pixel *dstC, int dstStride,
                              int block_width, int block_height,
                              int marginX, int marginY, int scale, int wround, int wshift, int woffset)
{
    int row, col;
    int headRoom = IF_INTERNAL_PREC - X265_DEPTH;
    int shift = IF_FILTER_PREC - headRoom;
    int offset = -IF_INTERNAL_OFFS << shift;

    src -= (8 / 2 - 1);
    __m128i vec_src0;
    __m128i vec_offset = _mm_set1_epi16(offset);
    __m128i sumaL, sumbL, sumcL, tmp, exp1;
    __m128i tmp16a, tmp16b, tmp16c, tmp16f, tmpwlo, tmpwhi;

    Int shiftNum = IF_INTERNAL_PREC - X265_DEPTH;
    wshift = wshift + shiftNum;
    wround = wshift ? (1 << (wshift - 1)) : 0;

    __m128i vround = _mm_set1_epi32(wround + scale * IF_INTERNAL_OFFS);
    __m128i ofs = _mm_set1_epi32(woffset);
    __m128i vscale = _mm_set1_epi32(scale);

    // Load Ai, ai += Ai*coefi
    for (row = 0; row < block_height; row++)
    {
        col = 0;

        vec_src0 = _mm_loadu_si128((__m128i const*)(src + col));
        sumbL = (_mm_unpacklo_epi8(vec_src0, _mm_setzero_si128()));
        sumbL = _mm_sub_epi16(_mm_setzero_si128(), sumbL);

        // a = b+=4*a1,  c+=1*a1
        vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 1));
        sumcL = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
        sumbL = _mm_add_epi16(sumbL, _mm_sll_epi16(sumcL, _mm_cvtsi32_si128(2)));
        sumaL = sumbL;

        // a +=-10*a2    b+=-11*a2      c+=-5*a2
        vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 2));
        tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
        sumbL = _mm_sub_epi16(sumbL, tmp);
        tmp = _mm_mullo_epi16(tmp, _mm_set1_epi16(-5));
        sumcL = _mm_add_epi16(sumcL, tmp);
        tmp = _mm_slli_epi16(tmp, 1);
        sumaL = _mm_add_epi16(sumaL, tmp);
        sumbL = _mm_add_epi16(sumbL, tmp);

        // a +=58*a3    b+=40*a3      c+=17*a3
        vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 3));
        tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
        tmp16f = _mm_sub_epi16(_mm_slli_epi16(tmp, 6), _mm_set1_epi16(IF_INTERNAL_OFFS));
        _mm_storeu_si128((__m128i*)(intF + col), tmp16f);
        //Apply weight on Full pel
        tmpwlo = _mm_unpacklo_epi16(tmp16f, _mm_srai_epi16(tmp16f, 15));
        tmpwlo = _mm_mullo_epi32(tmpwlo, vscale);
        tmpwlo = _mm_add_epi32(tmpwlo, vround);
        tmpwlo = _mm_srai_epi32(tmpwlo, wshift);
        tmpwlo = _mm_add_epi32(tmpwlo, ofs);
        tmpwhi = _mm_unpackhi_epi16(tmp16f, _mm_srai_epi16(tmp16f, 15));
        tmpwhi = _mm_mullo_epi32(tmpwhi, vscale);
        tmpwhi = _mm_add_epi32(tmpwhi, vround);
        tmpwhi = _mm_srai_epi32(tmpwhi, wshift);
        tmpwhi = _mm_add_epi32(tmpwhi, ofs);
        tmp16f = _mm_packus_epi16(_mm_packs_epi32(tmpwlo, tmpwhi), _mm_setzero_si128());
        _mm_storel_epi64((__m128i*)(dstF + row * dstStride + col), tmp16f);

        exp1 = _mm_add_epi16(tmp, _mm_slli_epi16(tmp, 4));
        sumcL = _mm_add_epi16(sumcL, exp1);
        sumaL = _mm_add_epi16(sumaL, tmp);
        tmp = _mm_mullo_epi16(tmp, _mm_set1_epi16(40));
        sumbL = _mm_add_epi16(sumbL, tmp);
        sumaL = _mm_add_epi16(sumaL, _mm_add_epi16(exp1, tmp));

        // a +=17*a4    b+=40*a4      c+=58*a4
        vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 4));
        tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
        exp1 = _mm_add_epi16(tmp, _mm_slli_epi16(tmp, 4));
        sumaL = _mm_add_epi16(sumaL, exp1);
        sumcL = _mm_add_epi16(sumcL, tmp);
        tmp = _mm_mullo_epi16(tmp, _mm_set1_epi16(40));
        sumbL = _mm_add_epi16(sumbL, tmp);
        sumcL = _mm_add_epi16(sumcL, _mm_add_epi16(exp1, tmp));

        // a +=-5*a5    b+=-11*a5      c+=-10*a5
        vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 5));
        tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
        sumbL = _mm_sub_epi16(sumbL, tmp);
        tmp = _mm_mullo_epi16(tmp, _mm_set1_epi16(-5));
        sumaL = _mm_add_epi16(sumaL, tmp);
        tmp = _mm_slli_epi16(tmp, 1);
        sumcL = _mm_add_epi16(sumcL, tmp);
        sumbL = _mm_add_epi16(sumbL, tmp);

        // a +=1*a6    b+=4*a6      c+=4*a6
        vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 6));
        tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
        sumaL = _mm_add_epi16(sumaL, tmp);
        tmp = _mm_slli_epi16(tmp, 2);
        sumbL = _mm_add_epi16(sumbL, tmp);
        sumcL = _mm_add_epi16(sumcL, tmp);

        // a +=0*a7    b+=-1*a7      c+=-1*a7
        vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 7));
        tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
        sumbL = _mm_sub_epi16(sumbL, tmp);
        sumcL = _mm_sub_epi16(sumcL, tmp);
        sumaL = _mm_add_epi16(sumaL, vec_offset);
        sumbL = _mm_add_epi16(sumbL, vec_offset);
        sumcL = _mm_add_epi16(sumcL, vec_offset);

        _mm_storeu_si128((__m128i*)(intA + col), sumaL);
        //Apply weight
        tmpwlo = _mm_unpacklo_epi16(sumaL, _mm_srai_epi16(sumaL, 15));
        tmpwlo = _mm_mullo_epi32(tmpwlo, vscale);
        tmpwlo = _mm_add_epi32(tmpwlo, vround);
        tmpwlo = _mm_srai_epi32(tmpwlo, wshift);
        tmpwlo = _mm_add_epi32(tmpwlo, ofs);
        tmpwhi = _mm_unpackhi_epi16(sumaL, _mm_srai_epi16(sumaL, 15));
        tmpwhi = _mm_mullo_epi32(tmpwhi, vscale);
        tmpwhi = _mm_add_epi32(tmpwhi, vround);
        tmpwhi = _mm_srai_epi32(tmpwhi, wshift);
        tmpwhi = _mm_add_epi32(tmpwhi, ofs);
        tmp16a = _mm_packus_epi16(_mm_packs_epi32(tmpwlo, tmpwhi), _mm_setzero_si128());
        _mm_storel_epi64((__m128i*)(dstA + row * dstStride + col), tmp16a);

        _mm_storeu_si128((__m128i*)(intB + col), sumbL);
        //Apply weight
        tmpwlo = _mm_unpacklo_epi16(sumbL, _mm_srai_epi16(sumbL, 15));
        tmpwlo = _mm_mullo_epi32(tmpwlo, vscale);
        tmpwlo = _mm_add_epi32(tmpwlo, vround);
        tmpwlo = _mm_srai_epi32(tmpwlo, wshift);
        tmpwlo = _mm_add_epi32(tmpwlo, ofs);
        tmpwhi = _mm_unpackhi_epi16(sumbL, _mm_srai_epi16(sumbL, 15));
        tmpwhi = _mm_mullo_epi32(tmpwhi, vscale);
        tmpwhi = _mm_add_epi32(tmpwhi, vround);
        tmpwhi = _mm_srai_epi32(tmpwhi, wshift);
        tmpwhi = _mm_add_epi32(tmpwhi, ofs);
        tmp16b = _mm_packus_epi16(_mm_packs_epi32(tmpwlo, tmpwhi), _mm_setzero_si128());
        _mm_storel_epi64((__m128i*)(dstB + row * dstStride + col), tmp16b);

        _mm_storeu_si128((__m128i*)(intC + col), sumcL);
        //Apply weight
        tmpwlo = _mm_unpacklo_epi16(sumcL, _mm_srai_epi16(sumcL, 15));
        tmpwlo = _mm_mullo_epi32(tmpwlo, vscale);
        tmpwlo = _mm_add_epi32(tmpwlo, vround);
        tmpwlo = _mm_srai_epi32(tmpwlo, wshift);
        tmpwlo = _mm_add_epi32(tmpwlo, ofs);
        tmpwhi = _mm_unpackhi_epi16(sumcL, _mm_srai_epi16(sumcL, 15));
        tmpwhi = _mm_mullo_epi32(tmpwhi, vscale);
        tmpwhi = _mm_add_epi32(tmpwhi, vround);
        tmpwhi = _mm_srai_epi32(tmpwhi, wshift);
        tmpwhi = _mm_add_epi32(tmpwhi, ofs);
        tmp16c = _mm_packus_epi16(_mm_packs_epi32(tmpwlo, tmpwhi), _mm_setzero_si128());
        _mm_storel_epi64((__m128i*)(dstC + row * dstStride + col), tmp16c);

        // Extend First column
        __m128i ma, mb, mc, mf;
        mf = _mm_shuffle_epi8(tmp16f, _mm_set1_epi8(0));
        ma = _mm_shuffle_epi8(tmp16a, _mm_set1_epi8(0));
        mb = _mm_shuffle_epi8(tmp16b, _mm_set1_epi8(0));
        mc = _mm_shuffle_epi8(tmp16c, _mm_set1_epi8(0));

        for (int i = -marginX; i < -16; i += 16)
        {
            _mm_storeu_si128((__m128i*)(dstF + row * dstStride +  i), mf);
            _mm_storeu_si128((__m128i*)(dstA + row * dstStride +  i), ma);
            _mm_storeu_si128((__m128i*)(dstB + row * dstStride +  i), mb);
            _mm_storeu_si128((__m128i*)(dstC + row * dstStride +  i), mc);
        }

        _mm_storeu_si128((__m128i*)(dstF + row * dstStride - 16), mf); /*Assuming marginX > 16*/
        _mm_storeu_si128((__m128i*)(dstA + row * dstStride - 16), ma);
        _mm_storeu_si128((__m128i*)(dstB + row * dstStride - 16), mb);
        _mm_storeu_si128((__m128i*)(dstC + row * dstStride - 16), mc);

        col += 8;

        for (; col + 8 /*16*/ <= (block_width); col += 8 /*16*/)               // Iterations multiple of 8
        {
            vec_src0 = _mm_loadu_si128((__m128i const*)(src + col));
            sumbL = (_mm_unpacklo_epi8(vec_src0, _mm_setzero_si128()));
            sumbL = _mm_sub_epi16(_mm_setzero_si128(), sumbL);

            // a = b+=4*a1,  c+=1*a1
            vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 1));
            sumcL = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
            sumbL = _mm_add_epi16(sumbL, _mm_slli_epi16(sumcL, 2));
            sumaL = sumbL;

            // a +=-10*a2    b+=-11*a2      c+=-5*a2
            vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 2));
            tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
            sumbL = _mm_sub_epi16(sumbL, tmp);
            tmp = _mm_mullo_epi16(tmp, _mm_set1_epi16(-5));
            sumcL = _mm_add_epi16(sumcL, tmp);
            tmp = _mm_slli_epi16(tmp, 1);
            sumaL = _mm_add_epi16(sumaL, tmp);
            sumbL = _mm_add_epi16(sumbL, tmp);

            // a +=58*a3    b+=40*a3      c+=17*a3
            vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 3));
            tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
            tmp16f = _mm_sub_epi16(_mm_slli_epi16(tmp, 6), _mm_set1_epi16(IF_INTERNAL_OFFS));
            _mm_storeu_si128((__m128i*)(intF + col), tmp16f);
            //Apply weight
            tmpwlo = _mm_unpacklo_epi16(tmp16f, _mm_srai_epi16(tmp16f, 15));
            tmpwlo = _mm_mullo_epi32(tmpwlo, vscale);
            tmpwlo = _mm_add_epi32(tmpwlo, vround);
            tmpwlo = _mm_srai_epi32(tmpwlo, wshift);
            tmpwlo = _mm_add_epi32(tmpwlo, ofs);
            tmpwhi = _mm_unpackhi_epi16(tmp16f, _mm_srai_epi16(tmp16f, 15));
            tmpwhi = _mm_mullo_epi32(tmpwhi, vscale);
            tmpwhi = _mm_add_epi32(tmpwhi, vround);
            tmpwhi = _mm_srai_epi32(tmpwhi, wshift);
            tmpwhi = _mm_add_epi32(tmpwhi, ofs);
            tmp16f = _mm_packus_epi16(_mm_packs_epi32(tmpwlo, tmpwhi), _mm_setzero_si128());
            _mm_storel_epi64((__m128i*)(dstF + row * dstStride + col), tmp16f);

            exp1 = _mm_add_epi16(tmp, _mm_slli_epi16(tmp, 4));
            sumcL = _mm_add_epi16(sumcL, exp1);
            sumaL = _mm_add_epi16(sumaL, tmp);
            tmp = _mm_mullo_epi16(tmp, _mm_set1_epi16(40));
            sumbL = _mm_add_epi16(sumbL, tmp);
            sumaL = _mm_add_epi16(sumaL, _mm_add_epi16(exp1, tmp));

            // a +=17*a4    b+=40*a4      c+=58*a4
            vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 4));
            tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
            exp1 = _mm_add_epi16(tmp, _mm_slli_epi16(tmp, 4));
            sumaL = _mm_add_epi16(sumaL, exp1);
            sumcL = _mm_add_epi16(sumcL, tmp);
            tmp = _mm_mullo_epi16(tmp, _mm_set1_epi16(40));
            sumbL = _mm_add_epi16(sumbL, tmp);
            sumcL = _mm_add_epi16(sumcL, _mm_add_epi16(exp1, tmp));

            // a +=-5*a5    b+=-11*a5      c+=-10*a5
            vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 5));
            tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
            sumbL = _mm_sub_epi16(sumbL, tmp);
            tmp = _mm_mullo_epi16(tmp, _mm_set1_epi16(-5));
            sumaL = _mm_add_epi16(sumaL, tmp);
            tmp = _mm_slli_epi16(tmp, 1);
            sumcL = _mm_add_epi16(sumcL, tmp);
            sumbL = _mm_add_epi16(sumbL, tmp);

            // a +=1*a6    b+=4*a6      c+=4*a6
            vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 6));
            tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
            sumaL = _mm_add_epi16(sumaL, tmp);
            tmp = _mm_slli_epi16(tmp, 2);
            sumbL = _mm_add_epi16(sumbL, tmp);
            sumcL = _mm_add_epi16(sumcL, tmp);

            // a +=0*a7    b+=-1*a7      c+=-1*a7
            vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 7));
            tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
            sumbL = _mm_sub_epi16(sumbL, tmp);
            sumcL = _mm_sub_epi16(sumcL, tmp);
            sumaL = _mm_add_epi16(sumaL, vec_offset);
            sumbL = _mm_add_epi16(sumbL, vec_offset);
            sumcL = _mm_add_epi16(sumcL, vec_offset);

            _mm_storeu_si128((__m128i*)(intA + col), sumaL);
            //Apply weight
            tmpwlo = _mm_unpacklo_epi16(sumaL, _mm_srai_epi16(sumaL, 15));
            tmpwlo = _mm_mullo_epi32(tmpwlo, vscale);
            tmpwlo = _mm_add_epi32(tmpwlo, vround);
            tmpwlo = _mm_srai_epi32(tmpwlo, wshift);
            tmpwlo = _mm_add_epi32(tmpwlo, ofs);
            tmpwhi = _mm_unpackhi_epi16(sumaL, _mm_srai_epi16(sumaL, 15));
            tmpwhi = _mm_mullo_epi32(tmpwhi, vscale);
            tmpwhi = _mm_add_epi32(tmpwhi, vround);
            tmpwhi = _mm_srai_epi32(tmpwhi, wshift);
            tmpwhi = _mm_add_epi32(tmpwhi, ofs);
            tmp16a = _mm_packus_epi16(_mm_packs_epi32(tmpwlo, tmpwhi), _mm_setzero_si128());
            _mm_storel_epi64((__m128i*)(dstA + row * dstStride + col), tmp16a);

            _mm_storeu_si128((__m128i*)(intB + col), sumbL);
            //Apply weight
            tmpwlo = _mm_unpacklo_epi16(sumbL, _mm_srai_epi16(sumbL, 15));
            tmpwlo = _mm_mullo_epi32(tmpwlo, vscale);
            tmpwlo = _mm_add_epi32(tmpwlo, vround);
            tmpwlo = _mm_srai_epi32(tmpwlo, wshift);
            tmpwlo = _mm_add_epi32(tmpwlo, ofs);
            tmpwhi = _mm_unpackhi_epi16(sumbL, _mm_srai_epi16(sumbL, 15));
            tmpwhi = _mm_mullo_epi32(tmpwhi, vscale);
            tmpwhi = _mm_add_epi32(tmpwhi, vround);
            tmpwhi = _mm_srai_epi32(tmpwhi, wshift);
            tmpwhi = _mm_add_epi32(tmpwhi, ofs);
            tmp16b = _mm_packus_epi16(_mm_packs_epi32(tmpwlo, tmpwhi), _mm_setzero_si128());
            _mm_storel_epi64((__m128i*)(dstB + row * dstStride + col), tmp16b);

            _mm_storeu_si128((__m128i*)(intC + col), sumcL);
            //Apply weight
            tmpwlo = _mm_unpacklo_epi16(sumcL, _mm_srai_epi16(sumcL, 15));
            tmpwlo = _mm_mullo_epi32(tmpwlo, vscale);
            tmpwlo = _mm_add_epi32(tmpwlo, vround);
            tmpwlo = _mm_srai_epi32(tmpwlo, wshift);
            tmpwlo = _mm_add_epi32(tmpwlo, ofs);
            tmpwhi = _mm_unpackhi_epi16(sumcL, _mm_srai_epi16(sumcL, 15));
            tmpwhi = _mm_mullo_epi32(tmpwhi, vscale);
            tmpwhi = _mm_add_epi32(tmpwhi, vround);
            tmpwhi = _mm_srai_epi32(tmpwhi, wshift);
            tmpwhi = _mm_add_epi32(tmpwhi, ofs);
            tmp16c = _mm_packus_epi16(_mm_packs_epi32(tmpwlo, tmpwhi), _mm_setzero_si128());
            _mm_storel_epi64((__m128i*)(dstC + row * dstStride + col), tmp16c);
        }

        if (block_width - col > 0)
        {
            vec_src0 = _mm_loadu_si128((__m128i const*)(src + block_width - 5));
            tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
            tmp = _mm_sub_epi16(_mm_slli_epi16(tmp, 6), _mm_set1_epi16(IF_INTERNAL_OFFS));
            _mm_storeu_si128((__m128i*)(intF + block_width - 8), tmp);
            //Apply weight
            tmpwlo = _mm_unpacklo_epi16(tmp, _mm_srai_epi16(tmp, 15));
            tmpwlo = _mm_mullo_epi32(tmpwlo, vscale);
            tmpwlo = _mm_add_epi32(tmpwlo, vround);
            tmpwlo = _mm_srai_epi32(tmpwlo, wshift);
            tmpwlo = _mm_add_epi32(tmpwlo, ofs);
            tmpwhi = _mm_unpackhi_epi16(tmp, _mm_srai_epi16(tmp, 15));
            tmpwhi = _mm_mullo_epi32(tmpwhi, vscale);
            tmpwhi = _mm_add_epi32(tmpwhi, vround);
            tmpwhi = _mm_srai_epi32(tmpwhi, wshift);
            tmpwhi = _mm_add_epi32(tmpwhi, ofs);
            tmp = _mm_packus_epi16(_mm_packs_epi32(tmpwlo, tmpwhi), _mm_setzero_si128());
            _mm_storel_epi64((__m128i*)(dstF + row * dstStride + block_width - 8), tmp);
            tmp16f = _mm_shuffle_epi8(tmp, _mm_set1_epi8(7));

            __m128i a, b, c, sum1, sum2, sum3 = _mm_setzero_si128();
            for (; col < block_width; col++)                           // Remaining iterations
            {
                vec_src0 = _mm_loadu_si128((__m128i const*)(src + col));
                tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());    // Assuming that there is no overflow (Everywhere in this function!)
                a = _mm_setr_epi16(-1, 4, -10, 58, 17,  -5, 1,  0);
                a = _mm_mullo_epi16(tmp, a);
                b = _mm_setr_epi16(-1, 4, -11, 40, 40, -11, 4, -1);
                b = _mm_mullo_epi16(tmp, b);
                c = _mm_setr_epi16(0, 1,  -5, 17, 58, -10, 4, -1);
                c = _mm_mullo_epi16(tmp, c);
                sum1  = _mm_hadd_epi16(a, b);                   // horizontally add 8 elements in 3 steps
                sum2  = _mm_hadd_epi16(c, c);
                sum2  = _mm_hadd_epi16(sum1, sum2);
                sum3  = _mm_hadd_epi16(sum2, sum2);
                sum3  = _mm_add_epi16(sum3, vec_offset);
                sum3  = _mm_sra_epi16(sum3, _mm_cvtsi32_si128(shift));
                intA[col] = _mm_cvtsi128_si32(sum3);
                intB[col] = _mm_extract_epi16(sum3, 1);
                intC[col] = _mm_extract_epi16(sum3, 2);

                tmpwlo = _mm_unpacklo_epi16(sum3, _mm_srai_epi16(sum3, 15));
                tmpwlo = _mm_mullo_epi32(tmpwlo, vscale);
                tmpwlo = _mm_add_epi32(tmpwlo, vround);
                tmpwlo = _mm_srai_epi32(tmpwlo, wshift);
                tmpwlo = _mm_add_epi32(tmpwlo, ofs);
                sum3 = _mm_packus_epi16(_mm_packs_epi32(tmpwlo, tmpwlo), _mm_setzero_si128());

                dstA[row * dstStride + col] = _mm_extract_epi8(sum3, 0);
                dstB[row * dstStride + col] = _mm_extract_epi8(sum3, 1);
                dstC[row * dstStride + col] = _mm_extract_epi8(sum3, 2);
            }

            tmp16a = _mm_shuffle_epi8(sum3, _mm_set1_epi8(0));
            tmp16b = _mm_shuffle_epi8(sum3, _mm_set1_epi8(1));
            tmp16c = _mm_shuffle_epi8(sum3, _mm_set1_epi8(2));
        }
        else
        {
            tmp16f = _mm_shuffle_epi8(tmp16f, _mm_set1_epi8(7));
            tmp16a = _mm_shuffle_epi8(tmp16a, _mm_set1_epi8(7));
            tmp16b = _mm_shuffle_epi8(tmp16b, _mm_set1_epi8(7));
            tmp16c = _mm_shuffle_epi8(tmp16c, _mm_set1_epi8(7));
        }
        // Extend last column
        for (int i = -marginX; i < -16; i += 16)
        {
            _mm_storeu_si128((__m128i*)(dstF + row * dstStride + block_width + marginX + i), tmp16f);
            _mm_storeu_si128((__m128i*)(dstA + row * dstStride + block_width + marginX + i), tmp16a);
            _mm_storeu_si128((__m128i*)(dstB + row * dstStride + block_width + marginX + i), tmp16b);
            _mm_storeu_si128((__m128i*)(dstC + row * dstStride + block_width + marginX + i), tmp16c);
        }

        _mm_storeu_si128((__m128i*)(dstF + row * dstStride + block_width + marginX - 16), tmp16f); /*Assuming marginX > 16*/
        _mm_storeu_si128((__m128i*)(dstA + row * dstStride + block_width + marginX - 16), tmp16a);
        _mm_storeu_si128((__m128i*)(dstB + row * dstStride + block_width + marginX - 16), tmp16b);
        _mm_storeu_si128((__m128i*)(dstC + row * dstStride + block_width + marginX - 16), tmp16c);

        src += srcStride;
        intF += intStride;
        intA += intStride;
        intB += intStride;
        intC += intStride;
    }

    // Extending bottom rows
    pixel *pe, *pi, *pp, *pf;
    pf = dstF + (block_height - 1) * dstStride - marginX;
    pe = dstA + (block_height - 1) * dstStride - marginX;
    pi = dstB + (block_height - 1) * dstStride - marginX;
    pp = dstC + (block_height - 1) * dstStride - marginX;
    for (int y = 1; y <= marginY; y++)
    {
        memcpy(pf + y * dstStride, pf, block_width + marginX * 2);
    }

    for (int y = 1; y <= marginY; y++)
    {
        memcpy(pe + y * dstStride, pe, block_width + marginX * 2);
    }

    for (int y = 1; y <= marginY; y++)
    {
        memcpy(pi + y * dstStride, pi, block_width + marginX * 2);
    }

    for (int y = 1; y <= marginY; y++)
    {
        memcpy(pp + y * dstStride, pp, block_width + marginX * 2);
    }

    // Extending top rows
    pf  = dstF - marginX;
    pe  = dstA - marginX;
    pi  = dstB - marginX;
    pp  = dstC - marginX;
    for (int y = 1; y <= marginY; y++)
    {
        memcpy(pf - y * dstStride, pf, block_width + marginX * 2);
    }

    for (int y = 1; y <= marginY; y++)
    {
        memcpy(pe - y * dstStride, pe, block_width + marginX * 2);
    }

    for (int y = 1; y <= marginY; y++)
    {
        memcpy(pi - y * dstStride, pi, block_width + marginX * 2);
    }

    for (int y = 1; y <= marginY; y++)
    {
        memcpy(pp - y * dstStride, pp, block_width + marginX * 2);
    }
}

void filterHorizontalExtendCol(pixel *src, int srcStride,
                                      short *intF, short* intA, short* intB, short* intC, int intStride,
                                      pixel *dstA, pixel *dstB, pixel *dstC, int dstStride,
                                      int block_width, int block_height,
                                      int marginX)
{
    int row, col;
    int headRoom = IF_INTERNAL_PREC - X265_DEPTH;
    int shift = IF_FILTER_PREC - headRoom;
    int offset = -IF_INTERNAL_OFFS << shift;

    src -= (8 / 2 - 1);
    __m128i vec_src0;
    __m128i vec_offset = _mm_set1_epi16(offset);
    __m128i sumaL, sumbL, sumcL, tmp, exp1;
    __m128i tmp16a, tmp16b, tmp16c;

    // Load Ai, ai += Ai*coefi
    for (row = 0; row < block_height; row++)
    {
        col = 0;

        vec_src0 = _mm_loadu_si128((__m128i const*)(src + col));
        sumbL = (_mm_unpacklo_epi8(vec_src0, _mm_setzero_si128()));
        sumbL = _mm_sub_epi16(_mm_setzero_si128(), sumbL);

        // a = b+=4*a1,  c+=1*a1
        vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 1));
        sumcL = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
        sumbL = _mm_add_epi16(sumbL, _mm_sll_epi16(sumcL, _mm_cvtsi32_si128(2)));
        sumaL = sumbL;

        // a +=-10*a2    b+=-11*a2      c+=-5*a2
        vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 2));
        tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
        sumbL = _mm_sub_epi16(sumbL, tmp);
        tmp = _mm_mullo_epi16(tmp, _mm_set1_epi16(-5));
        sumcL = _mm_add_epi16(sumcL, tmp);
        tmp = _mm_sll_epi16(tmp, _mm_cvtsi32_si128(1));
        sumaL = _mm_add_epi16(sumaL, tmp);
        sumbL = _mm_add_epi16(sumbL, tmp);

        // a +=58*a3    b+=40*a3      c+=17*a3
        vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 3));
        tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
        _mm_storeu_si128((__m128i*)(intF + col), _mm_sub_epi16(_mm_sll_epi16(tmp, _mm_cvtsi32_si128(6)), _mm_set1_epi16(IF_INTERNAL_OFFS)));
        exp1 = _mm_add_epi16(tmp, _mm_sll_epi16(tmp, _mm_cvtsi32_si128(4)));
        sumcL = _mm_add_epi16(sumcL, exp1);
        sumaL = _mm_add_epi16(sumaL, tmp);
        tmp = _mm_mullo_epi16(tmp, _mm_set1_epi16(40));
        sumbL = _mm_add_epi16(sumbL, tmp);
        sumaL = _mm_add_epi16(sumaL, _mm_add_epi16(exp1, tmp));

        // a +=17*a4    b+=40*a4      c+=58*a4
        vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 4));
        tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
        exp1 = _mm_add_epi16(tmp, _mm_sll_epi16(tmp, _mm_cvtsi32_si128(4)));
        sumaL = _mm_add_epi16(sumaL, exp1);
        sumcL = _mm_add_epi16(sumcL, tmp);
        tmp = _mm_mullo_epi16(tmp, _mm_set1_epi16(40));
        sumbL = _mm_add_epi16(sumbL, tmp);
        sumcL = _mm_add_epi16(sumcL, _mm_add_epi16(exp1, tmp));

        // a +=-5*a5    b+=-11*a5      c+=-10*a5
        vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 5));
        tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
        sumbL = _mm_sub_epi16(sumbL, tmp);
        tmp = _mm_mullo_epi16(tmp, _mm_set1_epi16(-5));
        sumaL = _mm_add_epi16(sumaL, tmp);
        tmp = _mm_sll_epi16(tmp, _mm_cvtsi32_si128(1));
        sumcL = _mm_add_epi16(sumcL, tmp);
        sumbL = _mm_add_epi16(sumbL, tmp);

        // a +=1*a6    b+=4*a6      c+=4*a6
        vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 6));
        tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
        sumaL = _mm_add_epi16(sumaL, tmp);
        tmp = _mm_sll_epi16(tmp, _mm_cvtsi32_si128(2));
        sumbL = _mm_add_epi16(sumbL, tmp);
        sumcL = _mm_add_epi16(sumcL, tmp);

        // a +=0*a7    b+=-1*a7      c+=-1*a7
        vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 7));
        tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
        sumbL = _mm_sub_epi16(sumbL, tmp);
        sumcL = _mm_sub_epi16(sumcL, tmp);
        sumaL = _mm_add_epi16(sumaL, vec_offset);
        sumbL = _mm_add_epi16(sumbL, vec_offset);
        sumcL = _mm_add_epi16(sumcL, vec_offset);

        _mm_storeu_si128((__m128i*)(intA + col), sumaL);
        sumaL = _mm_add_epi16(sumaL, _mm_set1_epi16(IF_INTERNAL_OFFS + 32));
        sumaL = _mm_sra_epi16(sumaL, _mm_cvtsi32_si128(6));
        tmp16a = _mm_packus_epi16(sumaL, sumaL);
        _mm_storel_epi64((__m128i*)(dstA + row * dstStride + col), tmp16a);

        _mm_storeu_si128((__m128i*)(intB + col), sumbL);
        sumbL = _mm_add_epi16(sumbL, _mm_set1_epi16(IF_INTERNAL_OFFS + 32));
        sumbL = _mm_sra_epi16(sumbL, _mm_cvtsi32_si128(6));
        tmp16b = _mm_packus_epi16(sumbL, sumbL);
        _mm_storel_epi64((__m128i*)(dstB + row * dstStride + col), tmp16b);

        _mm_storeu_si128((__m128i*)(intC + col), sumcL);
        sumcL = _mm_add_epi16(sumcL, _mm_set1_epi16(IF_INTERNAL_OFFS + 32));
        sumcL = _mm_sra_epi16(sumcL, _mm_cvtsi32_si128(6));
        tmp16c = _mm_packus_epi16(sumcL, sumcL);
        _mm_storel_epi64((__m128i*)(dstC + row * dstStride + col), tmp16c);

        // Extend First column
        __m128i ma, mb, mc;
        ma = _mm_shuffle_epi8(tmp16a, _mm_set1_epi8(0));
        mb = _mm_shuffle_epi8(tmp16b, _mm_set1_epi8(0));
        mc = _mm_shuffle_epi8(tmp16c, _mm_set1_epi8(0));

        for (int i = -marginX; i < -16; i += 16)
        {
            _mm_storeu_si128((__m128i*)(dstA + row * dstStride +  i), ma);
            _mm_storeu_si128((__m128i*)(dstB + row * dstStride +  i), mb);
            _mm_storeu_si128((__m128i*)(dstC + row * dstStride +  i), mc);
        }

        _mm_storeu_si128((__m128i*)(dstA + row * dstStride - 16), ma); /*Assuming marginX > 16*/
        _mm_storeu_si128((__m128i*)(dstB + row * dstStride - 16), mb);
        _mm_storeu_si128((__m128i*)(dstC + row * dstStride - 16), mc);

        col += 8;

        for (; col + 8 /*16*/ <= (block_width); col += 8 /*16*/)               // Iterations multiple of 8
        {
            vec_src0 = _mm_loadu_si128((__m128i const*)(src + col));
            sumbL = (_mm_unpacklo_epi8(vec_src0, _mm_setzero_si128()));
            sumbL = _mm_sub_epi16(_mm_setzero_si128(), sumbL);

            // a = b+=4*a1,  c+=1*a1
            vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 1));
            sumcL = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
            sumbL = _mm_add_epi16(sumbL, _mm_sll_epi16(sumcL, _mm_cvtsi32_si128(2)));
            sumaL = sumbL;

            // a +=-10*a2    b+=-11*a2      c+=-5*a2
            vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 2));
            tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
            sumbL = _mm_sub_epi16(sumbL, tmp);
            tmp = _mm_mullo_epi16(tmp, _mm_set1_epi16(-5));
            sumcL = _mm_add_epi16(sumcL, tmp);
            tmp = _mm_sll_epi16(tmp, _mm_cvtsi32_si128(1));
            sumaL = _mm_add_epi16(sumaL, tmp);
            sumbL = _mm_add_epi16(sumbL, tmp);

            // a +=58*a3    b+=40*a3      c+=17*a3
            vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 3));
            tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
            _mm_storeu_si128((__m128i*)(intF + col), _mm_sub_epi16(_mm_sll_epi16(tmp, _mm_cvtsi32_si128(6)), _mm_set1_epi16(IF_INTERNAL_OFFS)));
            exp1 = _mm_add_epi16(tmp, _mm_sll_epi16(tmp, _mm_cvtsi32_si128(4)));
            sumcL = _mm_add_epi16(sumcL, exp1);
            sumaL = _mm_add_epi16(sumaL, tmp);
            tmp = _mm_mullo_epi16(tmp, _mm_set1_epi16(40));
            sumbL = _mm_add_epi16(sumbL, tmp);
            sumaL = _mm_add_epi16(sumaL, _mm_add_epi16(exp1, tmp));

            // a +=17*a4    b+=40*a4      c+=58*a4
            vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 4));
            tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
            exp1 = _mm_add_epi16(tmp, _mm_sll_epi16(tmp, _mm_cvtsi32_si128(4)));
            sumaL = _mm_add_epi16(sumaL, exp1);
            sumcL = _mm_add_epi16(sumcL, tmp);
            tmp = _mm_mullo_epi16(tmp, _mm_set1_epi16(40));
            sumbL = _mm_add_epi16(sumbL, tmp);
            sumcL = _mm_add_epi16(sumcL, _mm_add_epi16(exp1, tmp));

            // a +=-5*a5    b+=-11*a5      c+=-10*a5
            vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 5));
            tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
            sumbL = _mm_sub_epi16(sumbL, tmp);
            tmp = _mm_mullo_epi16(tmp, _mm_set1_epi16(-5));
            sumaL = _mm_add_epi16(sumaL, tmp);
            tmp = _mm_sll_epi16(tmp, _mm_cvtsi32_si128(1));
            sumcL = _mm_add_epi16(sumcL, tmp);
            sumbL = _mm_add_epi16(sumbL, tmp);

            // a +=1*a6    b+=4*a6      c+=4*a6
            vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 6));
            tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
            sumaL = _mm_add_epi16(sumaL, tmp);
            tmp = _mm_sll_epi16(tmp, _mm_cvtsi32_si128(2));
            sumbL = _mm_add_epi16(sumbL, tmp);
            sumcL = _mm_add_epi16(sumcL, tmp);

            // a +=0*a7    b+=-1*a7      c+=-1*a7
            vec_src0 = _mm_loadu_si128((__m128i const*)(src + col + 7));
            tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
            sumbL = _mm_sub_epi16(sumbL, tmp);
            sumcL = _mm_sub_epi16(sumcL, tmp);
            sumaL = _mm_add_epi16(sumaL, vec_offset);
            sumbL = _mm_add_epi16(sumbL, vec_offset);
            sumcL = _mm_add_epi16(sumcL, vec_offset);

            _mm_storeu_si128((__m128i*)(intA + col), sumaL);
            sumaL = _mm_add_epi16(sumaL, _mm_set1_epi16(IF_INTERNAL_OFFS + 32));
            sumaL = _mm_sra_epi16(sumaL, _mm_cvtsi32_si128(6));
            tmp16a = _mm_packus_epi16(sumaL, sumaL);
            _mm_storel_epi64((__m128i*)(dstA + row * dstStride + col), tmp16a);

            _mm_storeu_si128((__m128i*)(intB + col), sumbL);
            sumbL = _mm_add_epi16(sumbL, _mm_set1_epi16(IF_INTERNAL_OFFS + 32));
            sumbL = _mm_sra_epi16(sumbL, _mm_cvtsi32_si128(6));
            tmp16b = _mm_packus_epi16(sumbL, sumbL);
            _mm_storel_epi64((__m128i*)(dstB + row * dstStride + col), tmp16b);

            _mm_storeu_si128((__m128i*)(intC + col), sumcL);
            sumcL = _mm_add_epi16(sumcL, _mm_set1_epi16(IF_INTERNAL_OFFS + 32));
            sumcL = _mm_sra_epi16(sumcL, _mm_cvtsi32_si128(6));
            tmp16c = _mm_packus_epi16(sumcL, sumcL);
            _mm_storel_epi64((__m128i*)(dstC + row * dstStride + col), tmp16c);
        }

        if (block_width - col > 0)
        {
            vec_src0 = _mm_loadu_si128((__m128i const*)(src + block_width - 5));
            tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());
            _mm_storeu_si128((__m128i*)(intF + block_width - 8), _mm_sub_epi16(_mm_sll_epi16(tmp, _mm_cvtsi32_si128(6)), _mm_set1_epi16(IF_INTERNAL_OFFS)));
            __m128i a, b, c, sum1, sum2, sum3 = _mm_setzero_si128();
            for (; col < block_width; col++)                           // Remaining iterations
            {
                vec_src0 = _mm_loadu_si128((__m128i const*)(src + col));
                tmp = _mm_unpacklo_epi8(vec_src0, _mm_setzero_si128());    // Assuming that there is no overflow (Everywhere in this function!)
                a = _mm_setr_epi16(-1, 4, -10, 58, 17,  -5, 1,  0);
                a = _mm_mullo_epi16(tmp, a);
                b = _mm_setr_epi16(-1, 4, -11, 40, 40, -11, 4, -1);
                b = _mm_mullo_epi16(tmp, b);
                c = _mm_setr_epi16(0, 1,  -5, 17, 58, -10, 4, -1);
                c = _mm_mullo_epi16(tmp, c);
                sum1  = _mm_hadd_epi16(a, b);                   // horizontally add 8 elements in 3 steps
                sum2  = _mm_hadd_epi16(c, c);
                sum2  = _mm_hadd_epi16(sum1, sum2);
                sum3  = _mm_hadd_epi16(sum2, sum2);
                sum3  = _mm_add_epi16(sum3, vec_offset);
                sum3  = _mm_sra_epi16(sum3, _mm_cvtsi32_si128(shift));
                intA[col] = _mm_cvtsi128_si32(sum3);
                intB[col] = _mm_extract_epi16(sum3, 1);
                intC[col] = _mm_extract_epi16(sum3, 2);
                sum3 = _mm_add_epi16(sum3, _mm_set1_epi16(IF_INTERNAL_OFFS + 32));
                sum3 = _mm_sra_epi16(sum3, _mm_cvtsi32_si128(6));
                sum3 = _mm_packus_epi16(sum3, sum3);
                dstA[row * dstStride + col] = _mm_extract_epi8(sum3, 0);
                dstB[row * dstStride + col] = _mm_extract_epi8(sum3, 1);
                dstC[row * dstStride + col] = _mm_extract_epi8(sum3, 2);
            }

            tmp16a = _mm_shuffle_epi8(sum3, _mm_set1_epi8(0));
            tmp16b = _mm_shuffle_epi8(sum3, _mm_set1_epi8(1));
            tmp16c = _mm_shuffle_epi8(sum3, _mm_set1_epi8(2));
        }
        else
        {
            tmp16a = _mm_shuffle_epi8(tmp16a, _mm_set1_epi8(15));
            tmp16b = _mm_shuffle_epi8(tmp16b, _mm_set1_epi8(15));
            tmp16c = _mm_shuffle_epi8(tmp16c, _mm_set1_epi8(15));
        }
        // Extend last column
        for (int i = -marginX; i < -16; i += 16)
        {
            _mm_storeu_si128((__m128i*)(dstA + row * dstStride + block_width + marginX + i), tmp16a);
            _mm_storeu_si128((__m128i*)(dstB + row * dstStride + block_width + marginX + i), tmp16b);
            _mm_storeu_si128((__m128i*)(dstC + row * dstStride + block_width + marginX + i), tmp16c);
        }

        _mm_storeu_si128((__m128i*)(dstA + row * dstStride + block_width + marginX - 16), tmp16a); /*Assuming marginX > 16*/
        _mm_storeu_si128((__m128i*)(dstB + row * dstStride + block_width + marginX - 16), tmp16b);
        _mm_storeu_si128((__m128i*)(dstC + row * dstStride + block_width + marginX - 16), tmp16c);

        src += srcStride;
        intF += intStride;
        intA += intStride;
        intB += intStride;
        intC += intStride;
    }
 }
#endif /* if INSTRSET >= X265_CPU_LEVEL_SSE41 */

#if INSTRSET >= X265_CPU_LEVEL_SSSE3
template<int N>
void filterHorizontal_p_s(pixel *src, int srcStride, short *dst, int dstStride, int width, int height, short const *coeff)
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

#endif /* if INSTRSET >= X265_CPU_LEVEL_SSSE3 */

void filterConvertPelToShort(pixel *src, int srcStride, short *dst, int dstStride, int width, int height)
{
    pixel* srcOrg = src;
    short* dstOrg = dst;
    int shift = IF_INTERNAL_PREC - X265_DEPTH;
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

void filterConvertShortToPel(short *src, int srcStride, pixel *dst, int dstStride, int width, int height)
{
    short* srcOrg = src;
    pixel* dstOrg = dst;
    int shift = IF_INTERNAL_PREC - X265_DEPTH;
    short offset = IF_INTERNAL_OFFS;

    offset += shift ? (1 << (shift - 1)) : 0;
    short maxVal = (1 << X265_DEPTH) - 1;
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
            val_uc.store_partial(8, dst + col);
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
