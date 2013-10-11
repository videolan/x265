/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Mandar Gurav <mandar@multicorewareinc.com>
 *          Deepthi Devaki Akkoorath <deepthidevaki@multicorewareinc.com>
 *          Mahesh Pittala <mahesh@multicorewareinc.com>
 *          Rajesh Paulraj <rajesh@multicorewareinc.com>
 *          Min Chen <min.chen@multicorewareinc.com>
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

#include "primitives.h"
#include "TLibCommon/TypeDef.h"    // TCoeff, int, UInt
#include "TLibCommon/TComRom.h"
#include <xmmintrin.h> // SSE
#include <smmintrin.h> // SSE4.1

#include <assert.h>
#include <string.h>

using namespace x265;

namespace {
uint32_t quant(int* coef,
    int* quantCoeff,
    int* deltaU,
    int* qCoef,
    int  qBits,
    int  add,
    int  numCoeff,
    int* lastPos)
{
    int qBits8 = qBits - 8;
    uint32_t acSum = 0;
    int dstOffset = 0;
    __m128i acSum4 = _mm_setzero_si128();
    __m128i addVec = _mm_set1_epi32(add);
    __m128i maskPos4 = _mm_setr_epi32(0, 1, 2, 3);
    __m128i posNext4 = _mm_set1_epi32(4);
    __m128i lastPos4 = _mm_set1_epi32(-1);

    for (int blockpos = 0; blockpos < numCoeff; blockpos += 8)
    {
        __m128i maskZero;
        __m128i level1 = _mm_loadu_si128((__m128i*)(coef + blockpos));

        __m128i sign1 = _mm_cmplt_epi32(level1, _mm_setzero_si128());

        __m128i qCoeff1 = _mm_loadu_si128((__m128i*)(quantCoeff + blockpos));
        __m128i tmplevel1 = _mm_mullo_epi32(_mm_abs_epi32(level1), qCoeff1);
        level1 = _mm_srai_epi32(_mm_add_epi32(tmplevel1, addVec), qBits);
        __m128i deltaU1 = _mm_srai_epi32(_mm_sub_epi32(tmplevel1, _mm_slli_epi32(level1, qBits)), qBits8);
        _mm_storeu_si128((__m128i*)(deltaU + blockpos), deltaU1);
        acSum4 = _mm_add_epi32(acSum4, level1);

        maskZero = _mm_cmpeq_epi32(level1, _mm_setzero_si128());
        //lastPos4 = _mm_or_si128(_mm_andnot_si128(maskZero, maskPos4), _mm_and_si128(maskZero, lastPos4));
        //lastPos4 = _mm_blendv_epi8(maskPos4, lastPos4, maskZero);
        lastPos4 = _mm_max_epi32(lastPos4, _mm_or_si128(maskZero, maskPos4));
        maskPos4 = _mm_add_epi32(maskPos4, posNext4);

        level1 = _mm_sub_epi32(_mm_xor_si128(level1, sign1), sign1);
        level1 = _mm_cvtepi16_epi32(_mm_packs_epi32(level1, level1));
        _mm_storeu_si128((__m128i*)(qCoef + dstOffset), level1);


        __m128i level2 = _mm_loadu_si128((__m128i*)(coef + blockpos + 4));
        __m128i sign2 = _mm_cmplt_epi32(level2, _mm_setzero_si128());

        __m128i qCoeff2 = _mm_loadu_si128((__m128i*)(quantCoeff + blockpos + 4));
        __m128i tmplevel2 = _mm_mullo_epi32(_mm_abs_epi32(level2), qCoeff2);
        level2 = _mm_srai_epi32(_mm_add_epi32(tmplevel2, addVec), qBits);
        __m128i deltaU2 = _mm_srai_epi32(_mm_sub_epi32(tmplevel2, _mm_slli_epi32(level2, qBits)), qBits8);
        _mm_storeu_si128((__m128i*)(deltaU + blockpos + 4), deltaU2);
        acSum4 = _mm_add_epi32(acSum4, level2);

        maskZero = _mm_cmpeq_epi32(level2, _mm_setzero_si128());
        //lastPos4 = _mm_or_si128(_mm_andnot_si128(maskZero, maskPos4), _mm_and_si128(maskZero, lastPos4));
        //lastPos4 = _mm_blendv_epi8(maskPos4, lastPos4, maskZero);
        lastPos4 = _mm_max_epi32(lastPos4, _mm_or_si128(maskZero, maskPos4));
        maskPos4 = _mm_add_epi32(maskPos4, posNext4);

        level2 = _mm_sub_epi32(_mm_xor_si128(level2, sign2), sign2);
        level2 = _mm_cvtepi16_epi32(_mm_packs_epi32(level2, level2));
        _mm_storeu_si128((__m128i*)(qCoef + dstOffset + 4), level2);

        dstOffset += 8;
    }
    acSum4 = _mm_hadd_epi32(acSum4, acSum4);
    acSum4 = _mm_hadd_epi32(acSum4, acSum4);
    acSum  = _mm_cvtsi128_si32(acSum4);

    lastPos4 = _mm_max_epi32(lastPos4, _mm_shuffle_epi32(lastPos4, 0x0E));
    lastPos4 = _mm_max_epi32(lastPos4, _mm_shuffle_epi32(lastPos4, 0x01));
    int tmp = _mm_cvtsi128_si32(lastPos4);
    *lastPos = tmp;

    return acSum;
}

void dequant(const int* quantCoef, int* coef, int width, int height, int per, int rem, bool useScalingList, unsigned int log2TrSize, int *deQuantCoef)
{
    int invQuantScales[6] = { 40, 45, 51, 57, 64, 72 };

    if (width > 32)
    {
        width  = 32;
        height = 32;
    }

    int valueToAdd;
    int transformShift = MAX_TR_DYNAMIC_RANGE - X265_DEPTH - log2TrSize;
    int shift = QUANT_IQUANT_SHIFT - QUANT_SHIFT - transformShift;

    if (useScalingList)
    {
        shift += 4;

        if (shift > per)
        {
            valueToAdd = 1 << (shift - per - 1);
            __m128i IAdd = _mm_set1_epi32(valueToAdd);

            for (int n = 0; n < width * height; n = n + 8)
            {
                __m128i quantCoef1, quantCoef2, deQuantCoef1, deQuantCoef2, quantCoef12, sign;

                quantCoef1 = _mm_loadu_si128((__m128i*)(quantCoef + n));
                quantCoef2 = _mm_loadu_si128((__m128i*)(quantCoef + n + 4));

                deQuantCoef1 = _mm_loadu_si128((__m128i*)(deQuantCoef + n));
                deQuantCoef2 = _mm_loadu_si128((__m128i*)(deQuantCoef + n + 4));

                quantCoef12 = _mm_packs_epi32(quantCoef1, quantCoef2);
                sign = _mm_srai_epi16(quantCoef12, 15);
                quantCoef1 = _mm_unpacklo_epi16(quantCoef12, sign);
                quantCoef2 = _mm_unpackhi_epi16(quantCoef12, sign);

                quantCoef1 = _mm_sra_epi32(_mm_add_epi32(_mm_mullo_epi32(quantCoef1, deQuantCoef1), IAdd), _mm_cvtsi32_si128(shift - per));
                quantCoef2 = _mm_sra_epi32(_mm_add_epi32(_mm_mullo_epi32(quantCoef2, deQuantCoef2), IAdd), _mm_cvtsi32_si128(shift - per));

                quantCoef12 = _mm_packs_epi32(quantCoef1, quantCoef2);
                sign = _mm_srai_epi16(quantCoef12, 15);
                quantCoef1 = _mm_unpacklo_epi16(quantCoef12, sign);
                _mm_storeu_si128((__m128i*)(coef + n), quantCoef1);
                quantCoef2 = _mm_unpackhi_epi16(quantCoef12, sign);
                _mm_storeu_si128((__m128i*)(coef + n + 4), quantCoef2);
            }
        }
        else
        {
            for (int n = 0; n < width * height; n = n + 8)
            {
                __m128i quantCoef1, quantCoef2, deQuantCoef1, deQuantCoef2, quantCoef12, sign;

                quantCoef1 = _mm_loadu_si128((__m128i*)(quantCoef + n));
                quantCoef2 = _mm_loadu_si128((__m128i*)(quantCoef + n + 4));

                deQuantCoef1 = _mm_loadu_si128((__m128i*)(deQuantCoef + n));
                deQuantCoef2 = _mm_loadu_si128((__m128i*)(deQuantCoef + n + 4));

                quantCoef12 = _mm_packs_epi32(quantCoef1, quantCoef2);
                sign = _mm_srai_epi16(quantCoef12, 15);
                quantCoef1 = _mm_unpacklo_epi16(quantCoef12, sign);
                quantCoef2 = _mm_unpackhi_epi16(quantCoef12, sign);

                quantCoef1 = _mm_mullo_epi32(quantCoef1, deQuantCoef1);
                quantCoef2 = _mm_mullo_epi32(quantCoef2, deQuantCoef2);

                quantCoef12 = _mm_packs_epi32(quantCoef1, quantCoef2);
                sign = _mm_srai_epi16(quantCoef12, 15);
                quantCoef1 = _mm_unpacklo_epi16(quantCoef12, sign);
                quantCoef2 = _mm_unpackhi_epi16(quantCoef12, sign);

                quantCoef1 = _mm_sll_epi32(quantCoef1, _mm_cvtsi32_si128(per - shift));
                quantCoef2 = _mm_sll_epi32(quantCoef2, _mm_cvtsi32_si128(per - shift));

                quantCoef12 = _mm_packs_epi32(quantCoef1, quantCoef2);
                sign = _mm_srai_epi16(quantCoef12, 15);
                quantCoef1 = _mm_unpacklo_epi16(quantCoef12, sign);
                _mm_storeu_si128((__m128i*)(coef + n), quantCoef1);
                quantCoef2 = _mm_unpackhi_epi16(quantCoef12, sign);
                _mm_storeu_si128((__m128i*)(coef + n + 4), quantCoef2);
            }
        }
    }
    else
    {
        valueToAdd = 1 << (shift - 1);
        int scale = invQuantScales[rem] << per;

        __m128i vScale = _mm_set1_epi32(scale);
        __m128i vAdd = _mm_set1_epi32(valueToAdd);

        for (int n = 0; n < width * height; n = n + 8)
        {
            __m128i quantCoef1, quantCoef2, quantCoef12, sign;

            quantCoef1 = _mm_loadu_si128((__m128i*)(quantCoef + n));
            quantCoef2 = _mm_loadu_si128((__m128i*)(quantCoef + n + 4));

            quantCoef12 = _mm_packs_epi32(quantCoef1, quantCoef2);
            sign = _mm_srai_epi16(quantCoef12, 15);
            quantCoef1 = _mm_unpacklo_epi16(quantCoef12, sign);
            quantCoef2 = _mm_unpackhi_epi16(quantCoef12, sign);

            quantCoef1 = _mm_sra_epi32(_mm_add_epi32(_mm_mullo_epi32(quantCoef1, vScale), vAdd), _mm_cvtsi32_si128(shift));
            quantCoef2 = _mm_sra_epi32(_mm_add_epi32(_mm_mullo_epi32(quantCoef2, vScale), vAdd), _mm_cvtsi32_si128(shift));

            quantCoef12 = _mm_packs_epi32(quantCoef1, quantCoef2);
            sign = _mm_srai_epi16(quantCoef12, 15);
            quantCoef1 = _mm_unpacklo_epi16(quantCoef12, sign);
            _mm_storeu_si128((__m128i*)(coef + n), quantCoef1);
            quantCoef2 = _mm_unpackhi_epi16(quantCoef12, sign);
            _mm_storeu_si128((__m128i*)(coef + n + 4), quantCoef2);
        }
    }
}

ALIGN_VAR_32(static const short, tab_idst_4x4[8][8]) =
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

void idst4(int *src, short *dst, intptr_t stride)
{
    __m128i m128iAdd, S0, S8, m128iTmp1, m128iTmp2, m128iAC, m128iBD, m128iA, m128iD;

    m128iAdd  = _mm_set1_epi32(64);

    m128iTmp1 = _mm_load_si128((__m128i*)&src[0]);
    m128iTmp2 = _mm_load_si128((__m128i*)&src[4]);
    S0 = _mm_packs_epi32(m128iTmp1, m128iTmp2);

    m128iTmp1 = _mm_load_si128((__m128i*)&src[8]);
    m128iTmp2 = _mm_load_si128((__m128i*)&src[12]);
    S8 = _mm_packs_epi32(m128iTmp1, m128iTmp2);

    m128iAC  = _mm_unpacklo_epi16(S0, S8);
    m128iBD  = _mm_unpackhi_epi16(S0, S8);

    m128iTmp1 = _mm_madd_epi16(m128iAC, _mm_load_si128((__m128i*)(tab_idst_4x4[0])));
    m128iTmp2 = _mm_madd_epi16(m128iBD, _mm_load_si128((__m128i*)(tab_idst_4x4[1])));
    S0   = _mm_add_epi32(m128iTmp1, m128iTmp2);
    S0   = _mm_add_epi32(S0, m128iAdd);
    S0   = _mm_srai_epi32(S0, 7);

    m128iTmp1 = _mm_madd_epi16(m128iAC, _mm_load_si128((__m128i*)(tab_idst_4x4[2])));
    m128iTmp2 = _mm_madd_epi16(m128iBD, _mm_load_si128((__m128i*)(tab_idst_4x4[3])));
    S8   = _mm_add_epi32(m128iTmp1, m128iTmp2);
    S8   = _mm_add_epi32(S8, m128iAdd);
    S8   = _mm_srai_epi32(S8, 7);

    m128iA = _mm_packs_epi32(S0, S8);

    m128iTmp1 = _mm_madd_epi16(m128iAC, _mm_load_si128((__m128i*)(tab_idst_4x4[4])));
    m128iTmp2 = _mm_madd_epi16(m128iBD, _mm_load_si128((__m128i*)(tab_idst_4x4[5])));
    S0  = _mm_add_epi32(m128iTmp1, m128iTmp2);
    S0  = _mm_add_epi32(S0, m128iAdd);
    S0  = _mm_srai_epi32(S0, 7);

    m128iTmp1 = _mm_madd_epi16(m128iAC, _mm_load_si128((__m128i*)(tab_idst_4x4[6])));
    m128iTmp2 = _mm_madd_epi16(m128iBD, _mm_load_si128((__m128i*)(tab_idst_4x4[7])));
    S8  = _mm_add_epi32(m128iTmp1, m128iTmp2);
    S8  = _mm_add_epi32(S8, m128iAdd);
    S8  = _mm_srai_epi32(S8, 7);

    m128iD = _mm_packs_epi32(S0, S8);

    S0 = _mm_unpacklo_epi16(m128iA, m128iD);
    S8 = _mm_unpackhi_epi16(m128iA, m128iD);

    m128iA = _mm_unpacklo_epi16(S0, S8);
    m128iD = _mm_unpackhi_epi16(S0, S8);

    /*   ###################    */
    m128iAdd  = _mm_set1_epi32(2048);

    m128iAC  = _mm_unpacklo_epi16(m128iA, m128iD);
    m128iBD  = _mm_unpackhi_epi16(m128iA, m128iD);

    m128iTmp1 = _mm_madd_epi16(m128iAC, _mm_load_si128((__m128i*)(tab_idst_4x4[0])));
    m128iTmp2 = _mm_madd_epi16(m128iBD, _mm_load_si128((__m128i*)(tab_idst_4x4[1])));
    S0   = _mm_add_epi32(m128iTmp1, m128iTmp2);
    S0   = _mm_add_epi32(S0, m128iAdd);
    S0   = _mm_srai_epi32(S0, 12);

    m128iTmp1 = _mm_madd_epi16(m128iAC, _mm_load_si128((__m128i*)(tab_idst_4x4[2])));
    m128iTmp2 = _mm_madd_epi16(m128iBD, _mm_load_si128((__m128i*)(tab_idst_4x4[3])));
    S8   = _mm_add_epi32(m128iTmp1, m128iTmp2);
    S8   = _mm_add_epi32(S8, m128iAdd);
    S8   = _mm_srai_epi32(S8, 12);

    m128iA = _mm_packs_epi32(S0, S8);

    m128iTmp1 = _mm_madd_epi16(m128iAC, _mm_load_si128((__m128i*)(tab_idst_4x4[4])));
    m128iTmp2 = _mm_madd_epi16(m128iBD, _mm_load_si128((__m128i*)(tab_idst_4x4[5])));
    S0  = _mm_add_epi32(m128iTmp1, m128iTmp2);
    S0  = _mm_add_epi32(S0, m128iAdd);
    S0  = _mm_srai_epi32(S0, 12);

    m128iTmp1 = _mm_madd_epi16(m128iAC, _mm_load_si128((__m128i*)(tab_idst_4x4[6])));
    m128iTmp2 = _mm_madd_epi16(m128iBD, _mm_load_si128((__m128i*)(tab_idst_4x4[7])));
    S8  = _mm_add_epi32(m128iTmp1, m128iTmp2);
    S8  = _mm_add_epi32(S8, m128iAdd);
    S8  = _mm_srai_epi32(S8, 12);

    m128iD = _mm_packs_epi32(S0, S8);

    m128iTmp1 = _mm_unpacklo_epi16(m128iA, m128iD);   // [32 30 22 20 12 10 02 00]
    m128iTmp2 = _mm_unpackhi_epi16(m128iA, m128iD);   // [33 31 23 21 13 11 03 01]
    m128iAC   = _mm_unpacklo_epi16(m128iTmp1, m128iTmp2);
    m128iBD   = _mm_unpackhi_epi16(m128iTmp1, m128iTmp2);

    _mm_storel_epi64((__m128i*)&dst[0 * stride], m128iAC);
    _mm_storeh_pi((__m64*)&dst[1 * stride], _mm_castsi128_ps(m128iAC));
    _mm_storel_epi64((__m128i*)&dst[2 * stride], m128iBD);
    _mm_storeh_pi((__m64*)&dst[3 * stride], _mm_castsi128_ps(m128iBD));
}

inline void partialButterfly8(short *src, short *dst, int shift, int line)
{
    int j;
    int add = 1 << (shift - 1);

    __m128i zero_row = _mm_setr_epi32(64, 64, 0, 0);
    __m128i four_row = _mm_setr_epi32(64, -64, 0, 0);
    __m128i two_row = _mm_setr_epi32(83, 36, 0, 0);
    __m128i six_row = _mm_setr_epi32(36, -83, 0, 0);

    __m128i one_row = _mm_setr_epi32(89, 75, 50, 18);
    __m128i three_row = _mm_setr_epi32(75, -18, -89, -50);
    __m128i five_row = _mm_setr_epi32(50, -89, 18, 75);
    __m128i seven_row = _mm_setr_epi32(18, -50, 75, -89);

    for (j = 0; j < line; j++)
    {
        __m128i srcTmp;
        srcTmp = _mm_loadu_si128((__m128i*)(src));

        __m128i sign = _mm_srai_epi16(srcTmp, 15);
        __m128i E_first_half = _mm_unpacklo_epi16(srcTmp, sign);
        __m128i E_second_half = _mm_unpackhi_epi16(srcTmp, sign);
        E_second_half = _mm_shuffle_epi32(E_second_half, 27);

        __m128i E = _mm_add_epi32(E_first_half, E_second_half);
        __m128i O = _mm_sub_epi32(E_first_half, E_second_half);

        __m128i EE_first_half = _mm_shuffle_epi32(E, 4);
        __m128i EE_second_half = _mm_shuffle_epi32(E, 11);
        __m128i EE = _mm_add_epi32(EE_first_half, EE_second_half);
        __m128i EO = _mm_sub_epi32(EE_first_half, EE_second_half);

        int dst0 = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(_mm_mullo_epi32(zero_row, EE), _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;
        int dst4 = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(_mm_mullo_epi32(four_row, EE), _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;
        int dst2 = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(_mm_mullo_epi32(two_row, EO), _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;
        int dst6 = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(_mm_mullo_epi32(six_row, EO), _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;

        dst[0] = dst0;
        dst[4 * line] = dst4;
        dst[2 * line] = dst2;
        dst[6 * line] = dst6;

        int dst1 = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(_mm_mullo_epi32(one_row, O), _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;
        int dst3 = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(_mm_mullo_epi32(three_row, O), _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;
        int dst5 = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(_mm_mullo_epi32(five_row, O), _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;
        int dst7 = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(_mm_mullo_epi32(seven_row, O), _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;

        dst[line] = dst1;
        dst[3 * line] = dst3;
        dst[5 * line] = dst5;
        dst[7 * line] = dst7;

        src += 8;
        dst++;
    }
}

void dct8(short *src, int *dst, intptr_t stride)
{
    const int shift_1st = 2;
    const int shift_2nd = 9;

    ALIGN_VAR_32(short, coef[8 * 8]);
    ALIGN_VAR_32(short, block[8 * 8]);

    for (int i = 0; i < 8; i++)
    {
        memcpy(&block[i * 8], &src[i * stride], 8 * sizeof(short));
    }

    partialButterfly8(block, coef, shift_1st, 8);
    partialButterfly8(coef, block, shift_2nd, 8);

    for (int i = 0; i < 8; i++)
    {
        __m128i im16;
        __m128i im32L, im32H;
        __m128i sign;

        im16 = _mm_loadu_si128((__m128i*)&block[i<<3]);
        sign = _mm_srai_epi16(im16, 15);
        im32L = _mm_unpacklo_epi16(im16, sign);
        im32H = _mm_unpackhi_epi16(im16, sign);
        _mm_storeu_si128((__m128i*)dst, im32L);
        _mm_storeu_si128((__m128i*)(dst + 4), im32H);

        dst += 8;
    }
}

inline void partialButterfly16(short *src, short *dst, int shift, int line)
{
    int j;
    int add = 1 << (shift - 1);

    __m128i zero_row = _mm_setr_epi32(64, 64, 0, 0);
    __m128i four_row = _mm_setr_epi32(83, 36, 0, 0);
    __m128i eight_row = _mm_setr_epi32(64, -64, 0, 0);
    __m128i twelve_row = _mm_setr_epi32(36, -83, 0, 0);

    __m128i two_row = _mm_setr_epi32(89, 75, 50, 18);
    __m128i six_row = _mm_setr_epi32(75, -18, -89, -50);
    __m128i ten_row = _mm_setr_epi32(50, -89, 18, 75);
    __m128i fourteen_row = _mm_setr_epi32(18, -50, 75, -89);

    __m128i one_row_first_half = _mm_setr_epi32(90, 87, 80, 70);
    __m128i one_row_second_half = _mm_setr_epi32(57, 43, 25,  9);
    __m128i three_row_first_half = _mm_setr_epi32(87, 57,  9, -43);
    __m128i three_row_second_half = _mm_setr_epi32(-80, -90, -70, -25);
    __m128i five_row_first_half = _mm_setr_epi32(80,  9, -70, -87);
    __m128i five_row_second_half = _mm_setr_epi32(-25, 57, 90, 43);
    __m128i seven_row_first_half = _mm_setr_epi32(70, -43, -87,  9);
    __m128i seven_row_second_half = _mm_setr_epi32(90, 25, -80, -57);
    __m128i nine_row_first_half = _mm_setr_epi32(57, -80, -25, 90);
    __m128i nine_row_second_half = _mm_setr_epi32(-9, -87, 43, 70);
    __m128i eleven_row_first_half = _mm_setr_epi32(43, -90, 57, 25);
    __m128i eleven_row_second_half = _mm_setr_epi32(-87, 70,  9, -80);
    __m128i thirteen_row_first_half = _mm_setr_epi32(25, -70, 90, -80);
    __m128i thirteen_row_second_half = _mm_setr_epi32(43,  9, -57, 87);
    __m128i fifteen_row_first_half = _mm_setr_epi32(9, -25, 43, -57);
    __m128i fifteen_row_second_half = _mm_setr_epi32(70, -80, 87, -90);

    for (j = 0; j < line; j++)
    {
        __m128i tmp1, tmp2;
        tmp1 = _mm_loadu_si128((__m128i*)(src));

        __m128i sign = _mm_srai_epi16(tmp1, 15);
        __m128i tmp1_first_half = _mm_unpacklo_epi16(tmp1, sign);
        __m128i tmp1_second_half = _mm_unpackhi_epi16(tmp1, sign);

        tmp2 = _mm_loadu_si128((__m128i*)(src + 8));
        sign = _mm_srai_epi16(tmp2, 15);
        __m128i tmp2_first_half_tmp = _mm_unpacklo_epi16(tmp2, sign);
        __m128i tmp2_second_half_tmp = _mm_unpackhi_epi16(tmp2, sign);
        __m128i tmp2_first_half = _mm_shuffle_epi32(tmp2_second_half_tmp, 27);
        __m128i tmp2_second_half = _mm_shuffle_epi32(tmp2_first_half_tmp, 27);

        __m128i E_first_half = _mm_add_epi32(tmp1_first_half, tmp2_first_half);
        __m128i E_second_half_tmp = _mm_add_epi32(tmp1_second_half, tmp2_second_half);
        __m128i O_first_half = _mm_sub_epi32(tmp1_first_half, tmp2_first_half);
        __m128i O_second_half = _mm_sub_epi32(tmp1_second_half, tmp2_second_half);

        __m128i E_second_half = _mm_shuffle_epi32(E_second_half_tmp, 27);

        __m128i EE = _mm_add_epi32(E_first_half, E_second_half);
        __m128i EO = _mm_sub_epi32(E_first_half, E_second_half);

        __m128i EE_first_half = _mm_shuffle_epi32(EE, 4);
        __m128i EE_second_half = _mm_shuffle_epi32(EE, 11);

        __m128i EEE = _mm_add_epi32(EE_first_half, EE_second_half);
        __m128i EEO = _mm_sub_epi32(EE_first_half, EE_second_half);

        __m128i dst_tmp0 = _mm_mullo_epi32(zero_row, EEE);
        __m128i dst_tmp4 = _mm_mullo_epi32(four_row, EEO);
        __m128i dst_tmp8 = _mm_mullo_epi32(eight_row, EEE);
        __m128i dst_tmp12 = _mm_mullo_epi32(twelve_row, EEO);

        int dst_zero = _mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(dst_tmp0, _mm_setzero_si128()), _mm_setzero_si128()));
        int dst_four = _mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(dst_tmp4, _mm_setzero_si128()), _mm_setzero_si128()));
        int dst_eight = _mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(dst_tmp8, _mm_setzero_si128()), _mm_setzero_si128()));
        int dst_twelve = _mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(dst_tmp12, _mm_setzero_si128()), _mm_setzero_si128()));

        __m128i dst_0_8_4_12 = _mm_setr_epi32(dst_zero, dst_eight, dst_four, dst_twelve);

        __m128i dst_result = _mm_add_epi32(dst_0_8_4_12, _mm_set1_epi32(add));
        __m128i dst_shift_result = _mm_srai_epi32(dst_result, shift);

        dst[0] = _mm_cvtsi128_si32(dst_shift_result);
        dst[8 * line] = _mm_cvtsi128_si32(_mm_shuffle_epi32(dst_shift_result, 1));
        dst[4 * line] = _mm_cvtsi128_si32(_mm_shuffle_epi32(dst_shift_result, 2));
        dst[12 * line] = _mm_cvtsi128_si32(_mm_shuffle_epi32(dst_shift_result, 3));

        __m128i dst_tmp2 = _mm_mullo_epi32(two_row, EO);
        __m128i dst_tmp6 = _mm_mullo_epi32(six_row, EO);
        __m128i dst_tmp10 = _mm_mullo_epi32(ten_row, EO);
        __m128i dst_tmp14 = _mm_mullo_epi32(fourteen_row, EO);

        int dst_two = _mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(dst_tmp2, _mm_setzero_si128()), _mm_setzero_si128()));
        int dst_six = _mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(dst_tmp6, _mm_setzero_si128()), _mm_setzero_si128()));
        int dst_ten = _mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(dst_tmp10, _mm_setzero_si128()), _mm_setzero_si128()));
        int dst_fourteen = _mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(dst_tmp14, _mm_setzero_si128()), _mm_setzero_si128()));

        __m128i dst_2_6_10_14 = _mm_setr_epi32(dst_two, dst_six, dst_ten, dst_fourteen);
        dst_2_6_10_14 = _mm_add_epi32(dst_2_6_10_14, _mm_set1_epi32(add));
        dst_2_6_10_14 = _mm_srai_epi32(dst_2_6_10_14, shift);

        dst[2 * line] = _mm_cvtsi128_si32(dst_2_6_10_14);
        dst[6 * line] = _mm_cvtsi128_si32(_mm_shuffle_epi32(dst_2_6_10_14, 1));
        dst[10 * line] = _mm_cvtsi128_si32(_mm_shuffle_epi32(dst_2_6_10_14, 2));
        dst[14 * line] = _mm_cvtsi128_si32(_mm_shuffle_epi32(dst_2_6_10_14, 3));

        __m128i dst_tmp1_first_half = _mm_mullo_epi32(one_row_first_half, O_first_half);
        __m128i dst_tmp1_second_half = _mm_mullo_epi32(one_row_second_half, O_second_half);
        __m128i dst_tmp3_first_half = _mm_mullo_epi32(three_row_first_half, O_first_half);
        __m128i dst_tmp3_second_half = _mm_mullo_epi32(three_row_second_half, O_second_half);
        __m128i dst_tmp5_first_half = _mm_mullo_epi32(five_row_first_half, O_first_half);
        __m128i dst_tmp5_second_half = _mm_mullo_epi32(five_row_second_half, O_second_half);
        __m128i dst_tmp7_first_half = _mm_mullo_epi32(seven_row_first_half, O_first_half);
        __m128i dst_tmp7_second_half = _mm_mullo_epi32(seven_row_second_half, O_second_half);
        __m128i dst_tmp9_first_half = _mm_mullo_epi32(nine_row_first_half, O_first_half);
        __m128i dst_tmp9_second_half = _mm_mullo_epi32(nine_row_second_half, O_second_half);
        __m128i dst_tmp11_first_half = _mm_mullo_epi32(eleven_row_first_half, O_first_half);
        __m128i dst_tmp11_second_half = _mm_mullo_epi32(eleven_row_second_half, O_second_half);
        __m128i dst_tmp13_first_half = _mm_mullo_epi32(thirteen_row_first_half, O_first_half);
        __m128i dst_tmp13_second_half = _mm_mullo_epi32(thirteen_row_second_half, O_second_half);
        __m128i dst_tmp15_first_half = _mm_mullo_epi32(fifteen_row_first_half, O_first_half);
        __m128i dst_tmp15_second_half = _mm_mullo_epi32(fifteen_row_second_half, O_second_half);

        int dst_one = _mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(_mm_add_epi32(dst_tmp1_first_half, dst_tmp1_second_half), _mm_setzero_si128()), _mm_setzero_si128()));
        int dst_three = _mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(_mm_add_epi32(dst_tmp3_first_half, dst_tmp3_second_half), _mm_setzero_si128()), _mm_setzero_si128()));
        int dst_five = _mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(_mm_add_epi32(dst_tmp5_first_half, dst_tmp5_second_half), _mm_setzero_si128()), _mm_setzero_si128()));
        int dst_seven = _mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(_mm_add_epi32(dst_tmp7_first_half, dst_tmp7_second_half), _mm_setzero_si128()), _mm_setzero_si128()));
        int dst_nine = _mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(_mm_add_epi32(dst_tmp9_first_half, dst_tmp9_second_half), _mm_setzero_si128()), _mm_setzero_si128()));
        int dst_eleven = _mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(_mm_add_epi32(dst_tmp11_first_half, dst_tmp11_second_half), _mm_setzero_si128()), _mm_setzero_si128()));
        int dst_thirteen = _mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(_mm_add_epi32(dst_tmp13_first_half, dst_tmp13_second_half), _mm_setzero_si128()), _mm_setzero_si128()));
        int dst_fifteen = _mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(_mm_add_epi32(dst_tmp15_first_half, dst_tmp15_second_half), _mm_setzero_si128()), _mm_setzero_si128()));

        __m128i dst_1_3_5_7 = _mm_setr_epi32(dst_one, dst_three, dst_five, dst_seven);
        dst_1_3_5_7 = _mm_add_epi32(dst_1_3_5_7, _mm_set1_epi32(add));
        dst_1_3_5_7 = _mm_srai_epi32(dst_1_3_5_7, shift);

        __m128i dst_9_11_13_15 = _mm_setr_epi32(dst_nine, dst_eleven, dst_thirteen, dst_fifteen);
        dst_9_11_13_15 = _mm_add_epi32(dst_9_11_13_15, _mm_set1_epi32(add));
        dst_9_11_13_15 = _mm_srai_epi32(dst_9_11_13_15, shift);

        dst[1 * line] = _mm_cvtsi128_si32(dst_1_3_5_7);
        dst[3 * line] = _mm_cvtsi128_si32(_mm_shuffle_epi32(dst_1_3_5_7, 1));
        dst[5 * line] = _mm_cvtsi128_si32(_mm_shuffle_epi32(dst_1_3_5_7, 2));
        dst[7 * line] = _mm_cvtsi128_si32(_mm_shuffle_epi32(dst_1_3_5_7, 3));
        dst[9 * line] = _mm_cvtsi128_si32(dst_9_11_13_15);
        dst[11 * line] = _mm_cvtsi128_si32(_mm_shuffle_epi32(dst_9_11_13_15, 1));
        dst[13 * line] = _mm_cvtsi128_si32(_mm_shuffle_epi32(dst_9_11_13_15, 2));
        dst[15 * line] = _mm_cvtsi128_si32(_mm_shuffle_epi32(dst_9_11_13_15, 3));

        src += 16;
        dst++;
    }
}

void dct16(short *src, int *dst, intptr_t stride)
{
    const int shift_1st = 3;
    const int shift_2nd = 10;

    ALIGN_VAR_32(short, coef[16 * 16]);
    ALIGN_VAR_32(short, block[16 * 16]);

    for (int i = 0; i < 16; i++)
    {
        memcpy(&block[i * 16], &src[i * stride], 16 * sizeof(short));
    }

    partialButterfly16(block, coef, shift_1st, 16);
    partialButterfly16(coef, block, shift_2nd, 16);

    for (int i = 0; i < 16*16; i += 8)
    {
        __m128i im16;
        __m128i im32L, im32H;
        __m128i sign;

        im16 = _mm_loadu_si128((__m128i*)&block[i]);
        sign = _mm_srai_epi16(im16, 15);
        im32L = _mm_unpacklo_epi16(im16, sign);
        im32H = _mm_unpackhi_epi16(im16, sign);
        _mm_storeu_si128((__m128i*)dst, im32L);
        _mm_storeu_si128((__m128i*)(dst + 4), im32H);

        dst += 8;
    }
}

void partialButterfly32(short *src, short *dst, int shift, int line)
{
    int j;
    int add = 1 << (shift - 1);

    __m128i zero_row_first_two = _mm_setr_epi32(64, 64, 0, 0);
    __m128i eight_row_first_two = _mm_setr_epi32(83, 36, 0, 0);
    __m128i sixten_row_first_two = _mm_setr_epi32(64, -64, 0, 0);
    __m128i twentyfour_row_first_two = _mm_setr_epi32(36, -83, 0, 0);

    __m128i four_row_first_four = _mm_setr_epi32(89, 75, 50, 18);
    __m128i twelve_row_first_four = _mm_setr_epi32(75, -18, -89, -50);
    __m128i twenty_row_first_four = _mm_setr_epi32(50, -89, 18, 75);
    __m128i twentyeight_row_first_four = _mm_setr_epi32(18, -50, 75, -89);

    __m128i two_row_first_four = _mm_setr_epi32(90, 87, 80, 70);
    __m128i two_row_second_four = _mm_setr_epi32(57, 43, 25,  9);
    __m128i six_row_first_four = _mm_setr_epi32(87, 57,  9, -43);
    __m128i six_row_second_four = _mm_setr_epi32(-80, -90, -70, -25);
    __m128i ten_row_first_four = _mm_setr_epi32(80,  9, -70, -87);
    __m128i ten_row_second_four = _mm_setr_epi32(-25, 57, 90, 43);
    __m128i fourteen_row_first_four = _mm_setr_epi32(70, -43, -87,  9);
    __m128i fourteen_row_second_four = _mm_setr_epi32(90, 25, -80, -57);
    __m128i eighteen_row_first_four = _mm_setr_epi32(57, -80, -25, 90);
    __m128i eighteen_row_second_four = _mm_setr_epi32(-9, -87, 43, 70);
    __m128i twentytwo_row_first_four = _mm_setr_epi32(43, -90, 57, 25);
    __m128i twentytwo_row_second_four = _mm_setr_epi32(-87, 70,  9, -80);
    __m128i twentysix_row_first_four = _mm_setr_epi32(25, -70, 90, -80);
    __m128i twentysix_row_second_four = _mm_setr_epi32(43,  9, -57, 87);
    __m128i thirty_row_first_four = _mm_setr_epi32(9, -25, 43, -57);
    __m128i thirty_row_second_four = _mm_setr_epi32(70, -80, 87, -90);

    __m128i one_row_first_four = _mm_setr_epi32(90, 90, 88, 85);
    __m128i one_row_second_four = _mm_setr_epi32(82, 78, 73, 67);
    __m128i one_row_third_four = _mm_setr_epi32(61, 54, 46, 38);
    __m128i one_row_fourth_four = _mm_setr_epi32(31, 22, 13,  4);

    __m128i three_row_first_four = _mm_setr_epi32(90, 82, 67, 46);
    __m128i three_row_second_four = _mm_setr_epi32(22, -4, -31, -54);
    __m128i three_row_third_four = _mm_setr_epi32(-73, -85, -90, -88);
    __m128i three_row_fourth_four = _mm_setr_epi32(-78, -61, -38, -13);

    __m128i five_row_first_four = _mm_setr_epi32(88, 67, 31, -13);
    __m128i five_row_second_four = _mm_setr_epi32(-54, -82, -90, -78);
    __m128i five_row_third_four = _mm_setr_epi32(-46, -4, 38, 73);
    __m128i five_row_fourth_four = _mm_setr_epi32(90, 85, 61, 22);

    __m128i seven_row_first_four = _mm_setr_epi32(85, 46, -13, -67);
    __m128i seven_row_second_four = _mm_setr_epi32(-90, -73, -22, 38);
    __m128i seven_row_third_four = _mm_setr_epi32(82, 88, 54, -4);
    __m128i seven_row_fourth_four = _mm_setr_epi32(-61, -90, -78, -31);

    __m128i nine_row_first_four = _mm_setr_epi32(82, 22, -54, -90);
    __m128i nine_row_second_four = _mm_setr_epi32(-61, 13, 78, 85);
    __m128i nine_row_third_four = _mm_setr_epi32(31, -46, -90, -67);
    __m128i nine_row_fourth_four = _mm_setr_epi32(4, 73, 88, 38);

    __m128i eleven_row_first_four = _mm_setr_epi32(78, -4, -82, -73);
    __m128i eleven_row_second_four = _mm_setr_epi32(13, 85, 67, -22);
    __m128i eleven_row_third_four = _mm_setr_epi32(-88, -61, 31, 90);
    __m128i eleven_row_fourth_four = _mm_setr_epi32(54, -38, -90, -46);

    __m128i thirteen_row_first_four = _mm_setr_epi32(73, -31, -90, -22);
    __m128i thirteen_row_second_four = _mm_setr_epi32(78, 67, -38, -90);
    __m128i thirteen_row_third_four = _mm_setr_epi32(-13, 82, 61, -46);
    __m128i thirteen_row_fourth_four = _mm_setr_epi32(-88, -4, 85, 54);

    __m128i fifteen_row_first_four = _mm_setr_epi32(67, -54, -78, 38);
    __m128i fifteen_row_second_four = _mm_setr_epi32(85, -22, -90,  4);
    __m128i fifteen_row_third_four = _mm_setr_epi32(90, 13, -88, -31);
    __m128i fifteen_row_fourth_four = _mm_setr_epi32(82, 46, -73, -61);

    __m128i seventeen_row_first_four = _mm_setr_epi32(61, -73, -46, 82);
    __m128i seventeen_row_second_four = _mm_setr_epi32(31, -88, -13, 90);
    __m128i seventeen_row_third_four = _mm_setr_epi32(-4, -90, 22, 85);
    __m128i seventeen_row_fourth_four = _mm_setr_epi32(-38, -78, 54, 67);

    __m128i nineteen_row_first_four = _mm_setr_epi32(54, -85, -4, 88);
    __m128i nineteen_row_second_four = _mm_setr_epi32(-46, -61, 82, 13);
    __m128i nineteen_row_third_four = _mm_setr_epi32(-90, 38, 67, -78);
    __m128i nineteen_row_fourth_four = _mm_setr_epi32(-22, 90, -31, -73);

    __m128i twentyone_row_first_four = _mm_setr_epi32(46, -90, 38, 54);
    __m128i twentyone_row_second_four = _mm_setr_epi32(-90, 31, 61, -88);
    __m128i twentyone_row_third_four = _mm_setr_epi32(22, 67, -85, 13);
    __m128i twentyone_row_fourth_four = _mm_setr_epi32(73, -82,  4, 78);

    __m128i twentythree_row_first_four = _mm_setr_epi32(38, -88, 73, -4);
    __m128i twentythree_row_second_four = _mm_setr_epi32(-67, 90, -46, -31);
    __m128i twentythree_row_third_four = _mm_setr_epi32(85, -78, 13, 61);
    __m128i twentythree_row_fourth_four = _mm_setr_epi32(-90, 54, 22, -82);

    __m128i twentyfive_row_first_four = _mm_setr_epi32(31, -78, 90, -61);
    __m128i twentyfive_row_second_four = _mm_setr_epi32(4, 54, -88, 82);
    __m128i twentyfive_row_third_four = _mm_setr_epi32(-38, -22, 73, -90);
    __m128i twentyfive_row_fourth_four = _mm_setr_epi32(67, -13, -46, 85);

    __m128i twentyseven_row_first_four = _mm_setr_epi32(22, -61, 85, -90);
    __m128i twentyseven_row_second_four = _mm_setr_epi32(73, -38, -4, 46);
    __m128i twentyseven_row_third_four = _mm_setr_epi32(-78, 90, -82, 54);
    __m128i twentyseven_row_fourth_four = _mm_setr_epi32(-13, -31, 67, -88);

    __m128i twentynine_row_first_four = _mm_setr_epi32(13, -38, 61, -78);
    __m128i twentynine_row_second_four = _mm_setr_epi32(88, -90, 85, -73);
    __m128i twentynine_row_third_four = _mm_setr_epi32(54, -31,  4, 22);
    __m128i twentynine_row_fourth_four = _mm_setr_epi32(-46, 67, -82, 90);

    __m128i thirtyone_row_first_four = _mm_setr_epi32(4, -13, 22, -31);
    __m128i thirtyone_row_second_four = _mm_setr_epi32(38, -46, 54, -61);
    __m128i thirtyone_row_third_four = _mm_setr_epi32(67, -73, 78, -82);
    __m128i thirtyone_row_fourth_four = _mm_setr_epi32(85, -88, 90, -90);

    for (j = 0; j < line; j++)
    {
        __m128i tmp1, tmp2, tmp3, tmp4;

        tmp1 = _mm_loadu_si128((__m128i*)(src));

        __m128i sign = _mm_srai_epi16(tmp1, 15);
        __m128i tmp1_first_half = _mm_unpacklo_epi16(tmp1, sign);
        __m128i tmp1_second_half = _mm_unpackhi_epi16(tmp1, sign);

        tmp2 = _mm_loadu_si128((__m128i*)(src + 8));
        sign = _mm_srai_epi16(tmp2, 15);
        __m128i tmp2_first_half = _mm_unpacklo_epi16(tmp2, sign);
        __m128i tmp2_second_half = _mm_unpackhi_epi16(tmp2, sign);

        tmp3 = _mm_loadu_si128((__m128i*)(src + 16));
        sign = _mm_srai_epi16(tmp3, 15);
        __m128i tmp3_first_half_tmp = _mm_unpacklo_epi16(tmp3, sign);
        __m128i tmp3_second_half_tmp = _mm_unpackhi_epi16(tmp3, sign);
        __m128i tmp3_first_half = _mm_shuffle_epi32(tmp3_first_half_tmp, 27);
        __m128i tmp3_second_half = _mm_shuffle_epi32(tmp3_second_half_tmp, 27);

        tmp4 = _mm_loadu_si128((__m128i*)(src + 24));
        sign = _mm_srai_epi16(tmp4, 15);
        __m128i tmp4_first_half_tmp = _mm_unpacklo_epi16(tmp4, sign);
        __m128i tmp4_second_half_tmp = _mm_unpackhi_epi16(tmp4, sign);
        __m128i tmp4_first_half = _mm_shuffle_epi32(tmp4_first_half_tmp, 27);
        __m128i tmp4_second_half = _mm_shuffle_epi32(tmp4_second_half_tmp, 27);

        __m128i E_first_four = _mm_add_epi32(tmp1_first_half, tmp4_second_half);
        __m128i E_second_four = _mm_add_epi32(tmp1_second_half, tmp4_first_half);
        __m128i E_third_four = _mm_add_epi32(tmp2_first_half, tmp3_second_half);
        __m128i E_last_four = _mm_add_epi32(tmp2_second_half, tmp3_first_half);

        __m128i O_first_four = _mm_sub_epi32(tmp1_first_half, tmp4_second_half);
        __m128i O_second_four = _mm_sub_epi32(tmp1_second_half, tmp4_first_half);
        __m128i O_third_four = _mm_sub_epi32(tmp2_first_half, tmp3_second_half);
        __m128i O_last_four = _mm_sub_epi32(tmp2_second_half, tmp3_first_half);

        __m128i E_last_four_rev = _mm_shuffle_epi32(E_last_four, 27);
        __m128i E_third_four_rev = _mm_shuffle_epi32(E_third_four, 27);

        __m128i EE_first_four = _mm_add_epi32(E_first_four, E_last_four_rev);
        __m128i EE_last_four = _mm_add_epi32(E_second_four, E_third_four_rev);
        __m128i EO_first_four = _mm_sub_epi32(E_first_four, E_last_four_rev);
        __m128i EO_last_four = _mm_sub_epi32(E_second_four, E_third_four_rev);

        __m128i EE_last_four_rev = _mm_shuffle_epi32(EE_last_four, 27);

        __m128i EEE = _mm_add_epi32(EE_first_four, EE_last_four_rev);
        __m128i EEO = _mm_sub_epi32(EE_first_four, EE_last_four_rev);

        __m128i EEEE_first_half = _mm_shuffle_epi32(EEE, 4);
        __m128i EEEE_second_half = _mm_shuffle_epi32(EEE, 11);
        __m128i EEEE = _mm_add_epi32(EEEE_first_half, EEEE_second_half);
        __m128i EEEO = _mm_sub_epi32(EEEE_first_half, EEEE_second_half);

        int dst0_hresult = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(_mm_mullo_epi32(zero_row_first_two, EEEE), _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;
        int dst8_hresult = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(_mm_mullo_epi32(eight_row_first_two, EEEO), _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;
        int dst16_hresult = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(_mm_mullo_epi32(sixten_row_first_two, EEEE), _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;
        int dst24_hresult = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(_mm_mullo_epi32(twentyfour_row_first_two, EEEO), _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;

        dst[0] = dst0_hresult;
        dst[8 * line] = dst8_hresult;
        dst[16 * line] = dst16_hresult;
        dst[24 * line] = dst24_hresult;

        int dst4_hresult = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(_mm_mullo_epi32(four_row_first_four, EEO), _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;
        int dst12_hresult = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(_mm_mullo_epi32(twelve_row_first_four, EEO), _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;
        int dst20_hresult = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(_mm_mullo_epi32(twenty_row_first_four, EEO), _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;
        int dst28_hresult = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(_mm_mullo_epi32(twentyeight_row_first_four, EEO), _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;

        dst[4 * line] = dst4_hresult;
        dst[12 * line] = dst12_hresult;
        dst[20 * line] = dst20_hresult;
        dst[28 * line] = dst28_hresult;
        __m128i tmp = _mm_add_epi32(_mm_mullo_epi32(two_row_first_four, EO_first_four), _mm_mullo_epi32(two_row_second_four, EO_last_four));
        int dst2_hresult = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(tmp, _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;

        tmp = _mm_add_epi32(_mm_mullo_epi32(six_row_first_four, EO_first_four), _mm_mullo_epi32(six_row_second_four, EO_last_four));
        int dst6_hresult = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(tmp, _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;

        tmp = _mm_add_epi32(_mm_mullo_epi32(ten_row_first_four, EO_first_four), _mm_mullo_epi32(ten_row_second_four, EO_last_four));
        int dst10_hresult = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(tmp, _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;

        tmp = _mm_add_epi32(_mm_mullo_epi32(fourteen_row_first_four, EO_first_four), _mm_mullo_epi32(fourteen_row_second_four, EO_last_four));
        int dst14_hresult = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(tmp, _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;

        tmp = _mm_add_epi32(_mm_mullo_epi32(eighteen_row_first_four, EO_first_four), _mm_mullo_epi32(eighteen_row_second_four, EO_last_four));
        int dst18_hresult = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(tmp, _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;

        tmp = _mm_add_epi32(_mm_mullo_epi32(twentytwo_row_first_four, EO_first_four), _mm_mullo_epi32(twentytwo_row_second_four, EO_last_four));
        int dst22_hresult = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(tmp, _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;

        tmp = _mm_add_epi32(_mm_mullo_epi32(twentysix_row_first_four, EO_first_four), _mm_mullo_epi32(twentysix_row_second_four, EO_last_four));
        int dst26_hresult = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(tmp, _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;

        tmp = _mm_add_epi32(_mm_mullo_epi32(thirty_row_first_four, EO_first_four), _mm_mullo_epi32(thirty_row_second_four, EO_last_four));
        int dst30_hresult = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(tmp, _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;

        dst[2 * line] = dst2_hresult;
        dst[6 * line] = dst6_hresult;
        dst[10 * line] = dst10_hresult;
        dst[14 * line] = dst14_hresult;
        dst[18 * line] = dst18_hresult;
        dst[22 * line] = dst22_hresult;
        dst[26 * line] = dst26_hresult;
        dst[30 * line] = dst30_hresult;

        tmp1 = _mm_add_epi32(_mm_mullo_epi32(one_row_first_four, O_first_four), _mm_mullo_epi32(one_row_second_four, O_second_four));
        tmp2 = _mm_add_epi32(_mm_mullo_epi32(one_row_third_four, O_third_four), _mm_mullo_epi32(one_row_fourth_four, O_last_four));
        __m128i dst1_temp = _mm_add_epi32(tmp1, tmp2);

        tmp1 = _mm_add_epi32(_mm_mullo_epi32(three_row_first_four, O_first_four), _mm_mullo_epi32(three_row_second_four, O_second_four));
        tmp2 = _mm_add_epi32(_mm_mullo_epi32(three_row_third_four, O_third_four), _mm_mullo_epi32(three_row_fourth_four, O_last_four));
        __m128i dst3_temp = _mm_add_epi32(tmp1, tmp2);

        tmp1 = _mm_add_epi32(_mm_mullo_epi32(five_row_first_four, O_first_four), _mm_mullo_epi32(five_row_second_four, O_second_four));
        tmp2 = _mm_add_epi32(_mm_mullo_epi32(five_row_third_four, O_third_four), _mm_mullo_epi32(five_row_fourth_four, O_last_four));
        __m128i dst5_temp = _mm_add_epi32(tmp1, tmp2);

        tmp1 = _mm_add_epi32(_mm_mullo_epi32(seven_row_first_four, O_first_four), _mm_mullo_epi32(seven_row_second_four, O_second_four));
        tmp2 = _mm_add_epi32(_mm_mullo_epi32(seven_row_third_four, O_third_four), _mm_mullo_epi32(seven_row_fourth_four, O_last_four));
        __m128i dst7_temp = _mm_add_epi32(tmp1, tmp2);

        tmp1 = _mm_add_epi32(_mm_mullo_epi32(nine_row_first_four, O_first_four), _mm_mullo_epi32(nine_row_second_four, O_second_four));
        tmp2 = _mm_add_epi32(_mm_mullo_epi32(nine_row_third_four, O_third_four), _mm_mullo_epi32(nine_row_fourth_four, O_last_four));
        __m128i dst9_temp = _mm_add_epi32(tmp1, tmp2);

        tmp1 = _mm_add_epi32(_mm_mullo_epi32(eleven_row_first_four, O_first_four), _mm_mullo_epi32(eleven_row_second_four, O_second_four));
        tmp2 = _mm_add_epi32(_mm_mullo_epi32(eleven_row_third_four, O_third_four), _mm_mullo_epi32(eleven_row_fourth_four, O_last_four));
        __m128i dst11_temp = _mm_add_epi32(tmp1, tmp2);

        tmp1 = _mm_add_epi32(_mm_mullo_epi32(thirteen_row_first_four, O_first_four), _mm_mullo_epi32(thirteen_row_second_four, O_second_four));
        tmp2 = _mm_add_epi32(_mm_mullo_epi32(thirteen_row_third_four, O_third_four), _mm_mullo_epi32(thirteen_row_fourth_four, O_last_four));
        __m128i dst13_temp = _mm_add_epi32(tmp1, tmp2);

        tmp1 = _mm_add_epi32(_mm_mullo_epi32(fifteen_row_first_four, O_first_four), _mm_mullo_epi32(fifteen_row_second_four, O_second_four));
        tmp2 = _mm_add_epi32(_mm_mullo_epi32(fifteen_row_third_four, O_third_four), _mm_mullo_epi32(fifteen_row_fourth_four, O_last_four));
        __m128i dst15_temp = _mm_add_epi32(tmp1, tmp2);

        tmp1 = _mm_add_epi32(_mm_mullo_epi32(seventeen_row_first_four, O_first_four), _mm_mullo_epi32(seventeen_row_second_four, O_second_four));
        tmp2 = _mm_add_epi32(_mm_mullo_epi32(seventeen_row_third_four, O_third_four), _mm_mullo_epi32(seventeen_row_fourth_four, O_last_four));
        __m128i dst17_temp = _mm_add_epi32(tmp1, tmp2);

        tmp1 = _mm_add_epi32(_mm_mullo_epi32(nineteen_row_first_four, O_first_four), _mm_mullo_epi32(nineteen_row_second_four, O_second_four));
        tmp2 = _mm_add_epi32(_mm_mullo_epi32(nineteen_row_third_four, O_third_four), _mm_mullo_epi32(nineteen_row_fourth_four, O_last_four));
        __m128i dst19_temp = _mm_add_epi32(tmp1, tmp2);

        tmp1 = _mm_add_epi32(_mm_mullo_epi32(twentyone_row_first_four, O_first_four), _mm_mullo_epi32(twentyone_row_second_four, O_second_four));
        tmp2 = _mm_add_epi32(_mm_mullo_epi32(twentyone_row_third_four, O_third_four), _mm_mullo_epi32(twentyone_row_fourth_four, O_last_four));
        __m128i dst21_temp = _mm_add_epi32(tmp1, tmp2);

        tmp1 = _mm_add_epi32(_mm_mullo_epi32(twentythree_row_first_four, O_first_four), _mm_mullo_epi32(twentythree_row_second_four, O_second_four));
        tmp2 = _mm_add_epi32(_mm_mullo_epi32(twentythree_row_third_four, O_third_four), _mm_mullo_epi32(twentythree_row_fourth_four, O_last_four));
        __m128i dst23_temp = _mm_add_epi32(tmp1, tmp2);

        tmp1 = _mm_add_epi32(_mm_mullo_epi32(twentyfive_row_first_four, O_first_four), _mm_mullo_epi32(twentyfive_row_second_four, O_second_four));
        tmp2 = _mm_add_epi32(_mm_mullo_epi32(twentyfive_row_third_four, O_third_four), _mm_mullo_epi32(twentyfive_row_fourth_four, O_last_four));
        __m128i dst25_temp = _mm_add_epi32(tmp1, tmp2);

        tmp1 = _mm_add_epi32(_mm_mullo_epi32(twentyseven_row_first_four, O_first_four), _mm_mullo_epi32(twentyseven_row_second_four, O_second_four));
        tmp2 = _mm_add_epi32(_mm_mullo_epi32(twentyseven_row_third_four, O_third_four), _mm_mullo_epi32(twentyseven_row_fourth_four, O_last_four));
        __m128i dst27_temp = _mm_add_epi32(tmp1, tmp2);

        tmp1 = _mm_add_epi32(_mm_mullo_epi32(twentynine_row_first_four, O_first_four), _mm_mullo_epi32(twentynine_row_second_four, O_second_four));
        tmp2 = _mm_add_epi32(_mm_mullo_epi32(twentynine_row_third_four, O_third_four), _mm_mullo_epi32(twentynine_row_fourth_four, O_last_four));
        __m128i dst29_temp = _mm_add_epi32(tmp1, tmp2);

        tmp1 = _mm_add_epi32(_mm_mullo_epi32(thirtyone_row_first_four, O_first_four), _mm_mullo_epi32(thirtyone_row_second_four, O_second_four));
        tmp2 = _mm_add_epi32(_mm_mullo_epi32(thirtyone_row_third_four, O_third_four), _mm_mullo_epi32(thirtyone_row_fourth_four, O_last_four));
        __m128i dst31_temp = _mm_add_epi32(tmp1, tmp2);

        dst[1 * line] = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(dst1_temp, _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;
        dst[3 * line] = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(dst3_temp, _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;
        dst[5 * line] = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(dst5_temp, _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;
        dst[7 * line] = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(dst7_temp, _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;
        dst[9 * line] = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(dst9_temp, _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;
        dst[11 * line] = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(dst11_temp, _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;
        dst[13 * line] = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(dst13_temp, _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;
        dst[15 * line] = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(dst15_temp, _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;
        dst[17 * line] = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(dst17_temp, _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;
        dst[19 * line] = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(dst19_temp, _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;
        dst[21 * line] = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(dst21_temp, _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;
        dst[23 * line] = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(dst23_temp, _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;
        dst[25 * line] = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(dst25_temp, _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;
        dst[27 * line] = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(dst27_temp, _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;
        dst[29 * line] = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(dst29_temp, _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;
        dst[31 * line] = (_mm_cvtsi128_si32(_mm_hadd_epi32(_mm_hadd_epi32(dst31_temp, _mm_setzero_si128()), _mm_setzero_si128())) + add) >> shift;

        src += 32;
        dst++;
    }
}

void dct32(short *src, int *dst, intptr_t stride)
{
    const int shift_1st = 4;
    const int shift_2nd = 11;

    ALIGN_VAR_32(short, coef[32 * 32]);
    ALIGN_VAR_32(short, block[32 * 32]);

    for (int i = 0; i < 32; i++)
    {
        memcpy(&block[i * 32], &src[i * stride], 32 * sizeof(short));
    }

    partialButterfly32(block, coef, shift_1st, 32);
    partialButterfly32(coef, block, shift_2nd, 32);

    for (int i = 0; i < 32*32; i += 8)
    {
        __m128i im16;
        __m128i im32L, im32H;
        __m128i sign;

        im16 = _mm_loadu_si128((__m128i*)&block[i]);
        sign = _mm_srai_epi16(im16, 15);
        im32L = _mm_unpacklo_epi16(im16, sign);
        im32H = _mm_unpackhi_epi16(im16, sign);
        _mm_storeu_si128((__m128i*)dst, im32L);
        _mm_storeu_si128((__m128i*)(dst + 4), im32H);

        dst += 8;
    }
}
}

namespace x265 {
void Setup_Vec_DCTPrimitives_sse41(EncoderPrimitives &p)
{
    p.quant = quant;
    p.dequant = dequant;
    p.idct[IDST_4x4] = idst4;
    p.dct[DCT_8x8] = dct8;
    p.dct[DCT_16x16] = dct16;
    p.dct[DCT_32x32] = dct32;
}
}
