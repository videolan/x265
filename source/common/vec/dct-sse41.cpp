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

#define INSTRSET 5
#include "vectorclass.h"

#include "primitives.h"
#include "TLibCommon/TypeDef.h"    // TCoeff, int, UInt
#include "TLibCommon/TComRom.h"

#include <assert.h>
#include <string.h>
#include <smmintrin.h>

using namespace x265;

namespace {
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
}

namespace x265 {
void Setup_Vec_DCTPrimitives_sse41(EncoderPrimitives &p)
{
    p.quant = quant;
    p.idct[IDST_4x4] = idst4;
}
}
