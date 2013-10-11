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
}

namespace x265 {
void Setup_Vec_DCTPrimitives_sse41(EncoderPrimitives &p)
{
    p.quant = quant;
    p.dequant = dequant;
    p.dct[DCT_8x8] = dct8;
    p.idct[IDST_4x4] = idst4;
}
}
