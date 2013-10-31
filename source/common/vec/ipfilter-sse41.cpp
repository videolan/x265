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

#include "primitives.h"
#include "TLibCommon/TComRom.h"
#include <xmmintrin.h> // SSE
#include <smmintrin.h> // SSE4.1
#include <assert.h>
#include <string.h>

using namespace x265;

#if !HIGH_BIT_DEPTH
namespace {
ALIGN_VAR_32(const uint16_t, c_512[16]) =
{
    512, 512, 512, 512, 512, 512, 512, 512
};

template<int N>
void filterVertical_sp(int16_t *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int width, int height, int coeffIdx)
{
    const int16_t *coeff = (N == 8 ? g_lumaFilter[coeffIdx] : g_chromaFilter[coeffIdx]);

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

template<int N>
void filterVertical_pp(pixel *src, intptr_t srcStride,
                        pixel *dst, intptr_t dstStride,
                        int width, int height,
                        const int16_t *coeff)
{
    src -= (N / 2 - 1) * srcStride;

    const __m128i coeffTemp = _mm_loadu_si128((__m128i const*)coeff);

    int row, col;

    if (N == 4)
        assert(height % 2 == 0);

    uint32_t leftCols = (8 - (width & 7)) * 8;
    uint32_t mask_shift = ((uint32_t) ~0 >> leftCols);
    uint32_t mask0 = (width & 7) <= 4 ? mask_shift : ~0;
    uint32_t mask1 = (width & 7) <= 4 ? 0 : mask_shift;
    __m128i leftmask = _mm_setr_epi32(mask0, mask1, 0, 0);

    if (N == 8)
    {
        __m128i vm01 = _mm_shuffle_epi32(coeffTemp, 0x00);
        __m128i vm23 = _mm_shuffle_epi32(coeffTemp, 0x55);
        __m128i vm45 = _mm_shuffle_epi32(coeffTemp, 0xAA);
        __m128i vm67 = _mm_shuffle_epi32(coeffTemp, 0xFF);
        vm01 = _mm_packs_epi16(vm01, vm01);
        vm23 = _mm_packs_epi16(vm23, vm23);
        vm45 = _mm_packs_epi16(vm45, vm45);
        vm67 = _mm_packs_epi16(vm67, vm67);

        __m128i T00, T01, T02, T03, T04, T05, T06, T07 /*, T08*/;
        __m128i T10, T11, T12, T13;
        for (row = 0; row < height; row += 1)
        {
            for (col = 0; col < (width & ~7); col += 8)
            {
                T00 = _mm_loadl_epi64((__m128i*)&src[(0) * srcStride + col]);
                T01 = _mm_loadl_epi64((__m128i*)&src[(1) * srcStride + col]);
                T02 = _mm_loadl_epi64((__m128i*)&src[(2) * srcStride + col]);
                T03 = _mm_loadl_epi64((__m128i*)&src[(3) * srcStride + col]);
                T04 = _mm_loadl_epi64((__m128i*)&src[(4) * srcStride + col]);
                T05 = _mm_loadl_epi64((__m128i*)&src[(5) * srcStride + col]);
                T06 = _mm_loadl_epi64((__m128i*)&src[(6) * srcStride + col]);
                T07 = _mm_loadl_epi64((__m128i*)&src[(7) * srcStride + col]);

                T10 = _mm_unpacklo_epi8(T00, T01);
                T11 = _mm_unpacklo_epi8(T02, T03);
                T12 = _mm_unpacklo_epi8(T04, T05);
                T13 = _mm_unpacklo_epi8(T06, T07);

                T10 = _mm_maddubs_epi16(T10, vm01);
                T11 = _mm_maddubs_epi16(T11, vm23);
                T12 = _mm_maddubs_epi16(T12, vm45);
                T13 = _mm_maddubs_epi16(T13, vm67);
                T10 = _mm_add_epi16(T10, T11);
                T11 = _mm_add_epi16(T12, T13);
                T10 = _mm_add_epi16(T10, T11);
                T10 = _mm_mulhrs_epi16(T10, _mm_load_si128((__m128i*)c_512));
                T10 = _mm_packus_epi16(T10, T10);
                _mm_storel_epi64((__m128i*)&dst[0 * dstStride + col], T10);
            }

            assert((width - col) < 8);
            if (col != width)
            {
                T00 = _mm_loadl_epi64((__m128i*)&src[(0) * srcStride + col]);
                T01 = _mm_loadl_epi64((__m128i*)&src[(1) * srcStride + col]);
                T02 = _mm_loadl_epi64((__m128i*)&src[(2) * srcStride + col]);
                T03 = _mm_loadl_epi64((__m128i*)&src[(3) * srcStride + col]);
                T04 = _mm_loadl_epi64((__m128i*)&src[(4) * srcStride + col]);
                T05 = _mm_loadl_epi64((__m128i*)&src[(5) * srcStride + col]);
                T06 = _mm_loadl_epi64((__m128i*)&src[(6) * srcStride + col]);
                T07 = _mm_loadl_epi64((__m128i*)&src[(7) * srcStride + col]);

                T10 = _mm_unpacklo_epi8(T00, T01);
                T11 = _mm_unpacklo_epi8(T02, T03);
                T12 = _mm_unpacklo_epi8(T04, T05);
                T13 = _mm_unpacklo_epi8(T06, T07);

                T10 = _mm_maddubs_epi16(T10, vm01);
                T11 = _mm_maddubs_epi16(T11, vm23);
                T12 = _mm_maddubs_epi16(T12, vm45);
                T13 = _mm_maddubs_epi16(T13, vm67);
                T10 = _mm_add_epi16(T10, T11);
                T11 = _mm_add_epi16(T12, T13);
                T10 = _mm_add_epi16(T10, T11);
                T10 = _mm_mulhrs_epi16(T10, _mm_load_si128((__m128i*)c_512));
                T10 = _mm_packus_epi16(T10, T10);
                _mm_maskmoveu_si128(T10, leftmask, (char*)&dst[(0) * dstStride + col]);
            }

            src += 1 * srcStride;
            dst += 1 * dstStride;
        }   // end of row loop
    } // end of N==8

    if (N == 4)
    {
        __m128i vm01 = _mm_shuffle_epi32(coeffTemp, 0x00);
        __m128i vm23 = _mm_shuffle_epi32(coeffTemp, 0x55);
        vm01 = _mm_packs_epi16(vm01, vm01);
        vm23 = _mm_packs_epi16(vm23, vm23);

        __m128i T00, T01, T02, T03, T04;
        __m128i T10, T11;
        __m128i T20, T21;
        for (row = 0; row < height; row += 2)
        {
            for (col = 0; col < (width & ~7); col += 8)
            {
                T00 = _mm_loadl_epi64((__m128i*)&src[(0) * srcStride + col]);
                T01 = _mm_loadl_epi64((__m128i*)&src[(1) * srcStride + col]);
                T02 = _mm_loadl_epi64((__m128i*)&src[(2) * srcStride + col]);
                T03 = _mm_loadl_epi64((__m128i*)&src[(3) * srcStride + col]);
                T04 = _mm_loadl_epi64((__m128i*)&src[(4) * srcStride + col]);

                T10 = _mm_unpacklo_epi8(T00, T01);
                T11 = _mm_unpacklo_epi8(T02, T03);

                T10 = _mm_maddubs_epi16(T10, vm01);
                T11 = _mm_maddubs_epi16(T11, vm23);
                T10 = _mm_add_epi16(T10, T11);
                T10 = _mm_mulhrs_epi16(T10, _mm_load_si128((__m128i*)c_512));
                T10 = _mm_packus_epi16(T10, T10);
                _mm_storel_epi64((__m128i*)&dst[0 * dstStride + col], T10);

                T20 = _mm_unpacklo_epi8(T01, T02);
                T21 = _mm_unpacklo_epi8(T03, T04);

                T20 = _mm_maddubs_epi16(T20, vm01);
                T21 = _mm_maddubs_epi16(T21, vm23);
                T20 = _mm_add_epi16(T20, T21);
                T20 = _mm_mulhrs_epi16(T20, _mm_load_si128((__m128i*)c_512));
                T20 = _mm_packus_epi16(T20, T20);
                _mm_storel_epi64((__m128i*)&dst[1 * dstStride + col], T20);
            }

            assert((width - col) < 8);
            if (col != width)
            {
                T00 = _mm_loadl_epi64((__m128i*)&src[(0) * srcStride + col]);
                T01 = _mm_loadl_epi64((__m128i*)&src[(1) * srcStride + col]);
                T02 = _mm_loadl_epi64((__m128i*)&src[(2) * srcStride + col]);
                T03 = _mm_loadl_epi64((__m128i*)&src[(3) * srcStride + col]);
                T04 = _mm_loadl_epi64((__m128i*)&src[(4) * srcStride + col]);

                T10 = _mm_unpacklo_epi8(T00, T01);
                T11 = _mm_unpacklo_epi8(T02, T03);

                T10 = _mm_maddubs_epi16(T10, vm01);
                T11 = _mm_maddubs_epi16(T11, vm23);
                T10 = _mm_add_epi16(T10, T11);
                T10 = _mm_mulhrs_epi16(T10, _mm_load_si128((__m128i*)c_512));
                T10 = _mm_packus_epi16(T10, T10);
                _mm_maskmoveu_si128(T10, leftmask, (char*)&dst[(0) * dstStride + col]);

                T20 = _mm_unpacklo_epi8(T01, T02);
                T21 = _mm_unpacklo_epi8(T03, T04);

                T20 = _mm_maddubs_epi16(T20, vm01);
                T21 = _mm_maddubs_epi16(T21, vm23);
                T20 = _mm_add_epi16(T20, T21);
                T20 = _mm_mulhrs_epi16(T20, _mm_load_si128((__m128i*)c_512));
                T20 = _mm_packus_epi16(T20, T20);
                _mm_maskmoveu_si128(T20, leftmask, (char*)&dst[(1) * dstStride + col]);
            }

            src += 2 * srcStride;
            dst += 2 * dstStride;
        }   // end of row loop
    } // end of N==4
}

ALIGN_VAR_32(const uint8_t, Tm[][16]) =
{
    // TODO: merge row0-3 into ipfilterH_0[0-3]
    { 0, 1, 2, 3, 4, 5, 6, 7, 1, 2, 3, 4, 5, 6, 7, 8 },
    { 2, 3, 4, 5, 6, 7, 8, 9, 3, 4, 5, 6, 7, 8, 9, 10 },
    { 4, 5, 6, 7, 8, 9, 10, 11, 5, 6, 7, 8, 9, 10, 11, 12 },
    { 6, 7, 8, 9, 10, 11, 12, 13, 7, 8, 9, 10, 11, 12, 13, 14 },
    { 0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5, 3, 4, 5, 6 },
    { 4, 5, 6, 7, 5, 6, 7, 8, 6, 7, 8, 9, 7, 8, 9, 10 }
};
ALIGN_VAR_32(const int8_t, tab_leftmask[16]) =
{
    -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

template<int N>
void filterHorizontal_pp(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int width, int height, int16_t const *coeff)
{
    assert(X265_DEPTH == 8);

    int row, col;
    __m128i val;

    src -= (N / 2 - 1);

    __m128i a = _mm_loadu_si128((__m128i*)coeff);
    __m128i coef2 = _mm_packs_epi16(a, a);

    const __m128i S = _mm_shuffle_epi32(coef2, 0);

    __m128i leftmask = _mm_loadl_epi64((__m128i*)&tab_leftmask[7 - (width & 7)]);

    // TODO: unroll
    for (row = 0; row < height; row++)
    {
        for (col = 0; col < (width & ~7); col += 8)
        {
            __m128i sum;
            __m128i srcCoeff = _mm_loadu_si128((__m128i*)(src + col));

            if (N == 4)
            {
                __m128i T00 = _mm_shuffle_epi8(srcCoeff, _mm_load_si128((__m128i*)Tm[4]));
                __m128i T20 = _mm_maddubs_epi16(T00, S);

                __m128i T30 = _mm_shuffle_epi8(srcCoeff, _mm_load_si128((__m128i*)Tm[5]));
                __m128i T40 = _mm_maddubs_epi16(T30, S);

                sum = _mm_hadd_epi16(T20, T40);
            }
            else // (N == 8)
            {
                __m128i T00 = _mm_shuffle_epi8(srcCoeff, _mm_load_si128((__m128i*)Tm[0]));
                __m128i T20 = _mm_maddubs_epi16(T00, coef2);

                __m128i T30 = _mm_shuffle_epi8(srcCoeff, _mm_load_si128((__m128i*)Tm[1]));
                __m128i T40 = _mm_maddubs_epi16(T30, coef2);

                __m128i T50 = _mm_shuffle_epi8(srcCoeff, _mm_load_si128((__m128i*)Tm[2]));
                __m128i T60 = _mm_maddubs_epi16(T50, coef2);

                __m128i T70 = _mm_shuffle_epi8(srcCoeff, _mm_load_si128((__m128i*)Tm[3]));
                __m128i T80 = _mm_maddubs_epi16(T70, coef2);

                __m128i s1 = _mm_hadd_epi16(T20, T40);
                __m128i s2 = _mm_hadd_epi16(T60, T80);
                sum = _mm_hadd_epi16(s1, s2);
            }

            val = _mm_mulhrs_epi16(sum, _mm_load_si128((__m128i*)c_512));
            val = _mm_packus_epi16(val, val);
            _mm_storel_epi64((__m128i*)&dst[col], val);
        }

        assert((width - col) < 8);

        if (col != width)
        {
            __m128i sum;
            __m128i srcCoeff = _mm_loadu_si128((__m128i*)(src + col));

            if (N == 4)
            {
                __m128i T00 = _mm_shuffle_epi8(srcCoeff, _mm_load_si128((__m128i*)Tm[4]));
                __m128i T20 = _mm_maddubs_epi16(T00, S);

                __m128i T30 = _mm_shuffle_epi8(srcCoeff, _mm_load_si128((__m128i*)Tm[5]));
                __m128i T40 = _mm_maddubs_epi16(T30, S);

                sum = _mm_hadd_epi16(T20, T40);
            }
            else // (N == 8)
            {
                __m128i T00 = _mm_shuffle_epi8(srcCoeff, _mm_load_si128((__m128i*)Tm[0]));
                __m128i T20 = _mm_maddubs_epi16(T00, coef2);

                __m128i T30 = _mm_shuffle_epi8(srcCoeff, _mm_load_si128((__m128i*)Tm[1]));
                __m128i T40 = _mm_maddubs_epi16(T30, coef2);

                __m128i T50 = _mm_shuffle_epi8(srcCoeff, _mm_load_si128((__m128i*)Tm[2]));
                __m128i T60 = _mm_maddubs_epi16(T50, coef2);

                __m128i T70 = _mm_shuffle_epi8(srcCoeff, _mm_load_si128((__m128i*)Tm[3]));
                __m128i T80 = _mm_maddubs_epi16(T70, coef2);

                __m128i s1 = _mm_hadd_epi16(T20, T40);
                __m128i s2 = _mm_hadd_epi16(T60, T80);
                sum = _mm_hadd_epi16(s1, s2);
            }

            val = _mm_mulhrs_epi16(sum, _mm_load_si128((__m128i*)c_512));
            val = _mm_packus_epi16(val, val);

            // TODO: optimize me: in here the really encode's size always be equal to 4
#if (defined(_MSC_VER) && (_MSC_VER == 1500)) && (X86_64)
            _mm_maskmoveu_si128(val, leftmask, (char*)&dst[col]);
#else
            __m128i oldval = _mm_loadl_epi64((__m128i*)&dst[col]);
            _mm_storel_epi64((__m128i*)&dst[col], _mm_blendv_epi8(oldval, val, leftmask));
#endif
        }

        src += srcStride;
        dst += dstStride;
    }
}
}

#else // if !HIGH_BIT_DEPTH

#define INSTRSET 5
#include "vectorclass.h"
namespace {
template<int N>
void filterVertical_sp(int16_t *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int block_width, int block_height, int coeffIdx)
{
    const int16_t *coeff = (N == 8 ? g_lumaFilter[coeffIdx] : g_chromaFilter[coeffIdx]);
    int row, col;

    src -= (N / 2 - 1) * srcStride;

    int offset;
    int16_t maxVal;
    int headRoom = IF_INTERNAL_PREC - X265_DEPTH;
    int shift = IF_FILTER_PREC;

    shift += headRoom;
    offset = 1 << (shift - 1);
    offset +=  IF_INTERNAL_OFFS << IF_FILTER_PREC;
    maxVal = (1 << X265_DEPTH) - 1;

    ALIGN_VAR_16(int, cm[8][4]);
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
            row1.load(&src[col + srcStride]);

            c0.load(cm[0]);
            c1.load(cm[1]);

            row0_first = extend_low(row0);
            row1_first = extend_low(row1);
            row0_last  = extend_high(row0);
            row1_last  = extend_high(row1);

            row0_first = row0_first * c0;
            row1_first = row1_first * c1;
            row0_last = row0_last * c0;
            row1_last = row1_last * c1;

            sum_first = row0_first + row1_first;
            sum_last = row0_last + row1_last;

            row2.load(&src[col + 2 * srcStride]);
            row3.load(&src[col + 3 * srcStride]);

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
                row4.load(&src[col + 4 * srcStride]);
                row5.load(&src[col + 5 * srcStride]);

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

                row6.load(&src[col + 6 * srcStride]);
                row7.load(&src[col + 7 * srcStride]);

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

            sum.store(dst + col);
        }

        //Handle the case when block_width is not multiple of 8
        for (; col < block_width; col += 4)
        {
            Vec8s row0, row1, row2, row3, row4, row5, row6, row7, sum;
            Vec4i row0_first, row0_last, row1_first, row1_last, sum_first, sum_last;
            Vec4i c0, c1, c2, c3, c4, c5, c6, c7;

            row0.load(&src[col]);
            row1.load(&src[col + srcStride]);

            c0.load(cm[0]);
            c1.load(cm[1]);

            row0_first = extend_low(row0);
            row1_first = extend_low(row1);
            row0_first = row0_first * c0;
            row1_first = row1_first * c1;

            sum_first = row0_first + row1_first;

            row2.load(&src[col + 2 * srcStride]);
            row3.load(&src[col + 3 * srcStride]);

            c2.load(cm[2]);
            c3.load(cm[3]);

            row0_first = extend_low(row2);
            row0_first = row0_first * c2;
            row1_first = extend_low(row3);
            row1_first = row1_first * c3;
            sum_first += row0_first + row1_first;

            if (N == 8)
            {
                row4.load(&src[col + 4 * srcStride]);
                row5.load(&src[col + 5 * srcStride]);

                c4.load(cm[4]);
                c5.load(cm[5]);

                row0_first = extend_low(row4);
                row0_first = row0_first * c4;
                row1_first = extend_low(row5);
                row1_first = row1_first * c5;
                sum_first += row0_first + row1_first;

                row6.load(&src[col + 6 * srcStride]);
                row7.load(&src[col + 7 * srcStride]);

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

            sum.store_partial(block_width - col, dst + col);
        }

        src += srcStride;
        dst += dstStride;
    }
}

template<int N>
void filterVertical_pp(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int block_width, int block_height, const int16_t *coeff)
{
    int row, col;

    src -= (N / 2 - 1) * srcStride;

    int offset;
    int16_t maxVal;

    int shift = IF_FILTER_PREC;

    offset = 1 << (shift - 1);
    maxVal = (1 << X265_DEPTH) - 1;

    ALIGN_VAR_16(int, cm[8][4]);
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
            row1.load(&src[col + srcStride]);

            c0.load(cm[0]);
            c1.load(cm[1]);

            row0_first = extend_low(row0);
            row1_first = extend_low(row1);
            row0_last  = extend_high(row0);
            row1_last  = extend_high(row1);

            row0_first = row0_first * c0;
            row1_first = row1_first * c1;
            row0_last = row0_last * c0;
            row1_last = row1_last * c1;

            sum_first = row0_first + row1_first;
            sum_last = row0_last + row1_last;

            row2.load(&src[col + 2 * srcStride]);
            row3.load(&src[col + 3 * srcStride]);

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
                row4.load(&src[col + 4 * srcStride]);
                row5.load(&src[col + 5 * srcStride]);

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

                row6.load(&src[col + 6 * srcStride]);
                row7.load(&src[col + 7 * srcStride]);

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

            sum.store(dst + col);
        }

        //Handle the case when block_width is not multiple of 8
        for (; col < block_width; col += 4)
        {
            Vec8s row0, row1, row2, row3, row4, row5, row6, row7, sum;
            Vec4i row0_first, row0_last, row1_first, row1_last, sum_first, sum_last;
            Vec4i c0, c1, c2, c3, c4, c5, c6, c7;

            row0.load(&src[col]);
            row1.load(&src[col + srcStride]);

            c0.load(cm[0]);
            c1.load(cm[1]);

            row0_first = extend_low(row0);
            row1_first = extend_low(row1);
            row0_first = row0_first * c0;
            row1_first = row1_first * c1;

            sum_first = row0_first + row1_first;

            row2.load(&src[col + 2 * srcStride]);
            row3.load(&src[col + 3 * srcStride]);

            c2.load(cm[2]);
            c3.load(cm[3]);

            row0_first = extend_low(row2);
            row0_first = row0_first * c2;
            row1_first = extend_low(row3);
            row1_first = row1_first * c3;
            sum_first += row0_first + row1_first;

            if (N == 8)
            {
                row4.load(&src[col + 4 * srcStride]);
                row5.load(&src[col + 5 * srcStride]);

                c4.load(cm[4]);
                c5.load(cm[5]);

                row0_first = extend_low(row4);
                row0_first = row0_first * c4;
                row1_first = extend_low(row5);
                row1_first = row1_first * c5;
                sum_first += row0_first + row1_first;

                row6.load(&src[col + 6 * srcStride]);
                row7.load(&src[col + 7 * srcStride]);

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

            sum.store_partial(block_width - col, dst + col);
        }

        src += srcStride;
        dst += dstStride;
    }
}

template<int N>
void filterHorizontal_pp(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int block_width, int block_height, const int16_t *coeff)
{
    int row, col;
    int offset;
    int16_t maxVal;
    int headRoom = IF_INTERNAL_PREC - X265_DEPTH;

    offset =  (1 << (headRoom - 1));
    maxVal = (1 << X265_DEPTH) - 1;
    src -= (N / 2 - 1);

    Vec4i vec_sum_low, vec_sum_high;
    Vec8s vec_src0, vec_sum, vec_c;
    vec_c.load(coeff);
    Vec8s vec_c0(coeff[0]), vec_c1(coeff[1]), vec_c2(coeff[2]), vec_c3(coeff[3]), vec_c4(coeff[4]), vec_c5(coeff[5]),
    vec_c6(coeff[6]), vec_c7(coeff[7]);
    Vec4i vec_offset(offset);
    Vec8s vec_maxVal(maxVal);
    for (row = 0; row < block_height; row++)
    {
        col = 0;
        for (; col < (block_width - 7); col += 8)                   // Iterations multiple of 8
        {
            vec_src0.load(src + col);                         // Load the 8 elements
            vec_src0 = vec_src0 * vec_c0;                       // Multiply by c[0]
            vec_sum_low = extend_low(vec_src0);                 // Convert to integer lower bits
            vec_sum_high = extend_high(vec_src0);               // Convert to integer higher bits

            vec_src0.load(src + col + 1);                     // Load the 8 elements
            vec_src0 = vec_src0 * vec_c1;                       // Multiply by c[1]
            vec_sum_low += extend_low(vec_src0);                // Add integer lower bits to sum_low bits
            vec_sum_high += extend_high(vec_src0);              // Add integer higer bits to sum_high bits

            vec_src0.load(src + col + 2);                     // Load the 8 elements
            vec_src0 = vec_src0 * vec_c2;                       // Multiply by c[2]
            vec_sum_low += extend_low(vec_src0);                // Add integer lower bits to sum_low bits
            vec_sum_high += extend_high(vec_src0);              // Add integer higer bits to sum_high bits

            vec_src0.load(src + col + 3);                     // Load the 8 elements
            vec_src0 = vec_src0 * vec_c3;                       // Multiply by c[2]
            vec_sum_low += extend_low(vec_src0);                // Add integer lower bits to sum_low bits
            vec_sum_high += extend_high(vec_src0);              // Add integer higer bits to sum_high bits

            if (N == 8)
            {
                vec_src0.load(src + col + 4);                     // Load the 8 elements
                vec_src0 = vec_src0 * vec_c4;                       // Multiply by c[2]
                vec_sum_low += extend_low(vec_src0);                // Add integer lower bits to sum_low bits
                vec_sum_high += extend_high(vec_src0);              // Add integer higer bits to sum_high bits

                vec_src0.load(src + col + 5);                     // Load the 8 elements
                vec_src0 = vec_src0 * vec_c5;                       // Multiply by c[2]
                vec_sum_low += extend_low(vec_src0);                // Add integer lower bits to sum_low bits
                vec_sum_high += extend_high(vec_src0);              // Add integer higer bits to sum_high bits

                vec_src0.load(src + col + 6);                     // Load the 8 elements
                vec_src0 = vec_src0 * vec_c6;                       // Multiply by c[2]
                vec_sum_low += extend_low(vec_src0);                // Add integer lower bits to sum_low bits
                vec_sum_high += extend_high(vec_src0);              // Add integer higer bits to sum_high bits

                vec_src0.load(src + col + 7);                     // Load the 8 elements
                vec_src0 = vec_src0 * vec_c7;                       // Multiply by c[2]
                vec_sum_low += extend_low(vec_src0);                // Add integer lower bits to sum_low bits
                vec_sum_high += extend_high(vec_src0);              // Add integer higer bits to sum_high bits
            }

            vec_sum_low = (vec_sum_low + vec_offset);           // Add offset(value copied into all integer vector elements) to sum_low
            vec_sum_high = (vec_sum_high + vec_offset);         // Add offset(value copied into all integer vector elements) to sum_high

            vec_sum = compress(vec_sum_low, vec_sum_high);       // Save two integer vectors(Vec4i) to single short vector(Vec8s)
            vec_sum = vec_sum >> headRoom;                         // This shift must be done after saving integer(two vec4i) data to short(Vec8s)

            vec_sum = max(vec_sum, 0);                          // (val < 0) ? 0 : val;
            vec_sum = min(vec_sum, vec_maxVal);                 // (val > maxVal) ? maxVal : val;

            vec_sum.store(dst + col);                           // Store vector
        }

        for (; col < block_width; col++)                           // Remaining iterations
        {
            if (N == 8)
            {
                vec_src0.load(src + col);
            }
            else
            {
                vec_src0 = load_partial(const_int(8), (src + col));
            }

            vec_src0 = vec_src0 * vec_c;                        // Assuming that there is no overflow (Everywhere in this function!)
            int sum = horizontal_add(vec_src0);
            int16_t val = (int16_t)(sum + offset) >> headRoom;
            val = (val < 0) ? 0 : val;
            val = (val > maxVal) ? maxVal : val;

            dst[col] = (pixel)val;
        }

        src += srcStride;
        dst += dstStride;
    }
}

template<int N>
void filterHorizontal_ps(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int block_width, int block_height, const int16_t *coeff)
{
    int row, col;
    int headRoom = IF_INTERNAL_PREC - X265_DEPTH;
    int shift = IF_FILTER_PREC - headRoom;
    int offset = -IF_INTERNAL_OFFS << shift;

    src -= (N / 2 - 1);

    Vec4i vec_sum_low, vec_sum_high;
    Vec8s vec_src0, vec_sum, vec_c;
    vec_c.load(coeff);
    Vec8s vec_c0(coeff[0]), vec_c1(coeff[1]), vec_c2(coeff[2]), vec_c3(coeff[3]), vec_c4(coeff[4]), vec_c5(coeff[5]),
    vec_c6(coeff[6]), vec_c7(coeff[7]);
    Vec4i vec_offset(offset);

    for (row = 0; row < block_height; row++)
    {
        col = 0;
        for (; col < (block_width - 7); col += 8)                   // Iterations multiple of 8
        {
            vec_src0.load(src + col);                         // Load the 8 elements
            vec_src0 = vec_src0 * vec_c0;                       // Multiply by c[0]
            vec_sum_low = extend_low(vec_src0);                 // Convert to integer lower bits
            vec_sum_high = extend_high(vec_src0);               // Convert to integer higher bits

            vec_src0.load(src + col + 1);                     // Load the 8 elements
            vec_src0 = vec_src0 * vec_c1;                       // Multiply by c[1]
            vec_sum_low += extend_low(vec_src0);                // Add integer lower bits to sum_low bits
            vec_sum_high += extend_high(vec_src0);              // Add integer higer bits to sum_high bits

            vec_src0.load(src + col + 2);                     // Load the 8 elements
            vec_src0 = vec_src0 * vec_c2;                       // Multiply by c[2]
            vec_sum_low += extend_low(vec_src0);                // Add integer lower bits to sum_low bits
            vec_sum_high += extend_high(vec_src0);              // Add integer higer bits to sum_high bits

            vec_src0.load(src + col + 3);                     // Load the 8 elements
            vec_src0 = vec_src0 * vec_c3;                       // Multiply by c[2]
            vec_sum_low += extend_low(vec_src0);                // Add integer lower bits to sum_low bits
            vec_sum_high += extend_high(vec_src0);              // Add integer higer bits to sum_high bits

            if (N == 8)
            {
                vec_src0.load(src + col + 4);                     // Load the 8 elements
                vec_src0 = vec_src0 * vec_c4;                       // Multiply by c[2]
                vec_sum_low += extend_low(vec_src0);                // Add integer lower bits to sum_low bits
                vec_sum_high += extend_high(vec_src0);              // Add integer higer bits to sum_high bits

                vec_src0.load(src + col + 5);                     // Load the 8 elements
                vec_src0 = vec_src0 * vec_c5;                       // Multiply by c[2]
                vec_sum_low += extend_low(vec_src0);                // Add integer lower bits to sum_low bits
                vec_sum_high += extend_high(vec_src0);              // Add integer higer bits to sum_high bits

                vec_src0.load(src + col + 6);                     // Load the 8 elements
                vec_src0 = vec_src0 * vec_c6;                       // Multiply by c[2]
                vec_sum_low += extend_low(vec_src0);                // Add integer lower bits to sum_low bits
                vec_sum_high += extend_high(vec_src0);              // Add integer higer bits to sum_high bits

                vec_src0.load(src + col + 7);                     // Load the 8 elements
                vec_src0 = vec_src0 * vec_c7;                       // Multiply by c[2]
                vec_sum_low += extend_low(vec_src0);                // Add integer lower bits to sum_low bits
                vec_sum_high += extend_high(vec_src0);              // Add integer higer bits to sum_high bits
            }

            vec_sum_low = (vec_sum_low + vec_offset);           // Add offset(value copied into all integer vector elements) to sum_low
            vec_sum_high = (vec_sum_high + vec_offset);         // Add offset(value copied into all integer vector elements) to sum_high

            vec_sum = compress(vec_sum_low, vec_sum_high);       // Save two integer vectors(Vec4i) to single short vector(Vec8s)
            vec_sum = vec_sum >> shift;                         // This shift must be done after saving integer(two vec4i) data to short(Vec8s)

            vec_sum.store(dst + col);                           // Store vector
        }

        for (; col < block_width; col++)                           // Remaining iterations
        {
            if (N == 8)
            {
                vec_src0.load(src + col);
            }
            else
            {
                vec_src0 = load_partial(const_int(8), (src + col));
            }

            vec_src0 = vec_src0 * vec_c;                        // Assuming that there is no overflow (Everywhere in this function!)
            int sum = horizontal_add(vec_src0);
            int16_t val = (int16_t)(sum + offset) >> shift;

            dst[col] = val;
        }

        src += srcStride;
        dst += dstStride;
    }
}

void filterConvertPelToShort(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int width, int height)
{
    pixel* srcOrg = src;
    int16_t* dstOrg = dst;
    int shift = IF_INTERNAL_PREC - X265_DEPTH;
    int row, col;
    Vec8s src_v, dst_v, val_v;

    for (row = 0; row < height; row++)
    {
        for (col = 0; col < width - 7; col += 8)
        {
            src_v.load(src + col);
            val_v = src_v << shift;
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
            val_v = src_v << shift;
            dst_v = val_v - IF_INTERNAL_OFFS;
            dst_v.store_partial(width - col, dst + col);

            src += srcStride;
            dst += dstStride;
        }
    }
}

void filterConvertShortToPel(int16_t *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int width, int height)
{
    int16_t* srcOrg = src;
    pixel* dstOrg = dst;
    int shift = IF_INTERNAL_PREC - X265_DEPTH;
    int16_t offset = IF_INTERNAL_OFFS;

    offset += shift ? (1 << (shift - 1)) : 0;
    int16_t maxVal = (1 << X265_DEPTH) - 1;
    Vec8s minVal(0);
    int row, col;
    Vec8s src_c, val_c;
    for (row = 0; row < height; row++)
    {
        for (col = 0; col < width - 7; col += 8)
        {
            src_c.load(src + col);
            val_c = add_saturated(src_c, offset) >> shift;
            val_c = max(val_c, minVal);
            val_c = min(val_c, maxVal);
            val_c.store(dst + col);
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
            val_c.store_partial(width - col, dst + col);

            src += srcStride;
            dst += dstStride;
        }
    }
}
}
#endif // if !HIGH_BIT_DEPTH

namespace x265 {
void Setup_Vec_IPFilterPrimitives_sse41(EncoderPrimitives& p)
{
    p.ipfilter_pp[FILTER_H_P_P_4] = filterHorizontal_pp<4>;
    p.ipfilter_pp[FILTER_H_P_P_8] = filterHorizontal_pp<8>;

    p.ipfilter_pp[FILTER_V_P_P_4] = filterVertical_pp<4>;
    p.ipfilter_pp[FILTER_V_P_P_8] = filterVertical_pp<8>;

    p.ipfilter_sp[FILTER_V_S_P_4] = filterVertical_sp<4>;
    p.ipfilter_sp[FILTER_V_S_P_8] = filterVertical_sp<8>;

#if HIGH_BIT_DEPTH
    p.ipfilter_ps[FILTER_H_P_S_4] = filterHorizontal_ps<4>;
    p.ipfilter_ps[FILTER_H_P_S_8] = filterHorizontal_ps<8>;

    p.ipfilter_p2s = filterConvertPelToShort;
    p.ipfilter_s2p = filterConvertShortToPel;
#endif
}
}
