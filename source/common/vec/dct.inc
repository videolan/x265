/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Mandar Gurav <mandar@multicorewareinc.com>
 *          Deepthi Devaki Akkoorath <deepthidevaki@multicorewareinc.com>
 *          Mahesh Pittala <mahesh@multicorewareinc.com>
 *          Rajesh Paulraj <rajesh@multicorewareinc.com>
 *          Min Chen <min.chen@multicorewareinc.com>
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

#include "primitives.h"
#include "TLibCommon/TypeDef.h"    // TCoeff, Int, UInt

#include <assert.h>
#include <string.h>
#include <smmintrin.h>
#include <algorithm>

extern void fastForwardDst(Short *block, Short *coeff, Int shift);

namespace {

/* Used for filter */
#define IF_INTERNAL_PREC 14 ///< Number of bits for internal precision
#define IF_FILTER_PREC    6 ///< Log2 of sum of filter taps
#define IF_INTERNAL_OFFS (1 << (IF_INTERNAL_PREC - 1)) ///< Offset used internally

void CDECL xDeQuant(int bitDepth, const int* pSrc, int* pDes, int iWidth, int iHeight, int iPer, int iRem, bool useScalingList, unsigned int uiLog2TrSize, int *piDequantCoefOrig)
{
    const int* piQCoef = pSrc;
    int* piCoef = pDes;

    int g_invQuantScales[6] = { 40, 45, 51, 57, 64, 72 };

    if (iWidth > 32)
    {
        iWidth  = 32;
        iHeight = 32;
    }

    int iShift, iAdd;

    int iTransformShift = 15 - bitDepth - uiLog2TrSize;

    iShift = 6 - iTransformShift;

    if (useScalingList)
    {
        iShift += 4;
        int *piDequantCoef = piDequantCoefOrig;

        if (iShift > iPer)
        {
            iAdd = 1 << (iShift - iPer - 1);
            Vec4i IAdd(iAdd);

            for (int n = 0; n < iWidth * iHeight; n = n + 8)
            {
                Vec4i qCoef1, qCoef2, deQuantCoef1, deQuantCoef2;

                qCoef1.load(piQCoef + n);
                qCoef2.load(piQCoef + n + 4);

                deQuantCoef1.load(piDequantCoef + n);
                deQuantCoef2.load(piDequantCoef + n + 4);

                Vec8s qCoef12 = compress_saturated(qCoef1, qCoef2);

                qCoef1 = extend_low(qCoef12);
                qCoef2 = extend_high(qCoef12);

                qCoef1 =  (qCoef1 *  deQuantCoef1 + IAdd) >> (iShift - iPer);
                qCoef2 =  (qCoef2 *  deQuantCoef2 + IAdd) >> (iShift - iPer);

                qCoef12 = compress_saturated(qCoef1, qCoef2);

                qCoef1 = extend_low(qCoef12);
                qCoef1.store(piCoef + n);
                qCoef2 = extend_high(qCoef12);
                qCoef2.store(piCoef + n + 4);
            }
        }
        else
        {
            for (int n = 0; n < iWidth * iHeight; n = n + 8)
            {
                Vec4i qCoef1, qCoef2, deQuantCoef1, deQuantCoef2;

                qCoef1.load(piQCoef + n);
                qCoef2.load(piQCoef + n + 4);

                deQuantCoef1.load(piDequantCoef + n);
                deQuantCoef2.load(piDequantCoef + n + 4);

                Vec8s qCoef12 = compress_saturated(qCoef1, qCoef2);

                qCoef1 = extend_low(qCoef12);
                qCoef2 = extend_high(qCoef12);

                qCoef1 = qCoef1 * deQuantCoef1;
                qCoef2 = qCoef2 * deQuantCoef2;

                qCoef12 = compress_saturated(qCoef1, qCoef2);

                qCoef1 = extend_low(qCoef12);
                qCoef2 = extend_high(qCoef12);

                qCoef1 = qCoef1 << (iPer - iShift);
                qCoef2 = qCoef2 << (iPer - iShift);

                qCoef12 = compress_saturated(qCoef1, qCoef2);

                qCoef1 = extend_low(qCoef12);
                qCoef1.store(piCoef + n);
                qCoef2 = extend_high(qCoef12);
                qCoef2.store(piCoef + n + 4);
            }
        }
    }
    else
    {
        iAdd = 1 << (iShift - 1);
        int scale = g_invQuantScales[iRem] << iPer;

        Vec4i Scale(scale);
        Vec4i IAdd(iAdd);

        for (int n = 0; n < iWidth * iHeight; n = n + 8)
        {
            Vec4i qCoef1, qCoef2;
            qCoef1.load(piQCoef + n);
            qCoef2.load(piQCoef + n + 4);

            Vec8s qCoef12 = compress_saturated(qCoef1, qCoef2);

            qCoef1 = extend_low(qCoef12);
            qCoef2 = extend_high(qCoef12);

            qCoef1 = (qCoef1 * Scale + IAdd) >> iShift;
            qCoef2 = (qCoef2 * Scale + IAdd) >> iShift;

            qCoef12 = compress_saturated(qCoef1, qCoef2);

            qCoef1 = extend_low(qCoef12);
            qCoef1.store(piCoef + n);
            qCoef2 = extend_high(qCoef12);
            qCoef2.store(piCoef + n + 4);
        }
    }
}

#if INSTRSET < 5
inline void inversedst(short *tmp, short *block, int shift)  // input tmp, output block
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

void xIDST4(short *pSrc, short *pDst, intptr_t stride)
{
    const int shift_1st = 7;
    const int shift_2nd = 12;
    ALIGN_VAR_32(Short, tmp[4 * 4]);
    ALIGN_VAR_32(Short, tmp2[4 * 4]);

    inversedst(pSrc, tmp, shift_1st);
    inversedst(tmp, tmp2, shift_2nd);
    for(int i=0; i<4; i++)
    {
        memcpy(&pDst[i * stride], &tmp2[i * 4], 4 * sizeof(short));
    }
}

#else // INSTRSET >= 5

ALIGN_VAR_32(static const short, tab_idst_4x4[8][8] )=
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

void xIDST4(short *pSrc, short *pDst, intptr_t stride)
{
    __m128i m128iAdd, S0, S8, m128iTmp1, m128iTmp2, m128iAC, m128iBD, m128iA, m128iD;
    m128iAdd  = _mm_set1_epi32( 64 );

    S0  = _mm_load_si128   ( (__m128i*)( pSrc      ) );
    S8  = _mm_load_si128   ( (__m128i*)( pSrc + 8  ) );

    m128iAC  = _mm_unpacklo_epi16( S0 , S8 );
    m128iBD  = _mm_unpackhi_epi16( S0 , S8 );

    m128iTmp1 = _mm_madd_epi16( m128iAC, _mm_load_si128( (__m128i*)( tab_idst_4x4[0] ) ) );
    m128iTmp2 = _mm_madd_epi16( m128iBD, _mm_load_si128( (__m128i*)( tab_idst_4x4[1] ) ) );
    S0   = _mm_add_epi32( m128iTmp1, m128iTmp2 );
    S0   = _mm_add_epi32( S0, m128iAdd );
    S0   = _mm_srai_epi32( S0, 7  );

    m128iTmp1 = _mm_madd_epi16( m128iAC, _mm_load_si128( (__m128i*)( tab_idst_4x4[2] ) ) );
    m128iTmp2 = _mm_madd_epi16( m128iBD, _mm_load_si128( (__m128i*)( tab_idst_4x4[3] ) ) );
    S8   = _mm_add_epi32( m128iTmp1, m128iTmp2 );
    S8   = _mm_add_epi32( S8, m128iAdd );
    S8   = _mm_srai_epi32( S8, 7  );

    m128iA = _mm_packs_epi32( S0, S8 );

    m128iTmp1 = _mm_madd_epi16( m128iAC, _mm_load_si128( (__m128i*)( tab_idst_4x4[4] ) ) );
    m128iTmp2 = _mm_madd_epi16( m128iBD, _mm_load_si128( (__m128i*)( tab_idst_4x4[5] ) ) );
    S0  = _mm_add_epi32( m128iTmp1, m128iTmp2 );
    S0  = _mm_add_epi32( S0, m128iAdd );
    S0  = _mm_srai_epi32( S0, 7  );

    m128iTmp1 = _mm_madd_epi16( m128iAC, _mm_load_si128( (__m128i*)( tab_idst_4x4[6] ) ) );
    m128iTmp2 = _mm_madd_epi16( m128iBD, _mm_load_si128( (__m128i*)( tab_idst_4x4[7] ) ) );
    S8  = _mm_add_epi32( m128iTmp1, m128iTmp2 );
    S8  = _mm_add_epi32( S8, m128iAdd );
    S8  = _mm_srai_epi32( S8, 7  );

    m128iD = _mm_packs_epi32( S0, S8 );

    S0 =_mm_unpacklo_epi16(  m128iA, m128iD );
    S8 =_mm_unpackhi_epi16(  m128iA, m128iD );

    m128iA =_mm_unpacklo_epi16(  S0, S8 );
    m128iD =_mm_unpackhi_epi16(  S0, S8 );

    /*   ###################    */
    m128iAdd  = _mm_set1_epi32( 2048 );

    m128iAC  = _mm_unpacklo_epi16( m128iA , m128iD );
    m128iBD  = _mm_unpackhi_epi16( m128iA , m128iD );

    m128iTmp1 = _mm_madd_epi16( m128iAC, _mm_load_si128( (__m128i*)( tab_idst_4x4[0] ) ) );
    m128iTmp2 = _mm_madd_epi16( m128iBD, _mm_load_si128( (__m128i*)( tab_idst_4x4[1] ) ) );
    S0   = _mm_add_epi32( m128iTmp1, m128iTmp2 );
    S0   = _mm_add_epi32( S0, m128iAdd );
    S0   = _mm_srai_epi32( S0, 12  );

    m128iTmp1 = _mm_madd_epi16( m128iAC, _mm_load_si128( (__m128i*)( tab_idst_4x4[2] ) ) );
    m128iTmp2 = _mm_madd_epi16( m128iBD, _mm_load_si128( (__m128i*)( tab_idst_4x4[3] ) ) );
    S8   = _mm_add_epi32( m128iTmp1, m128iTmp2 );
    S8   = _mm_add_epi32( S8, m128iAdd );
    S8   = _mm_srai_epi32( S8, 12  );

    m128iA = _mm_packs_epi32( S0, S8 );

    m128iTmp1 = _mm_madd_epi16( m128iAC, _mm_load_si128( (__m128i*)( tab_idst_4x4[4] ) ) );
    m128iTmp2 = _mm_madd_epi16( m128iBD, _mm_load_si128( (__m128i*)( tab_idst_4x4[5] ) ) );
    S0  = _mm_add_epi32( m128iTmp1, m128iTmp2 );
    S0  = _mm_add_epi32( S0, m128iAdd );
    S0  = _mm_srai_epi32( S0, 12  );

    m128iTmp1 = _mm_madd_epi16( m128iAC, _mm_load_si128( (__m128i*)( tab_idst_4x4[6] ) ) );
    m128iTmp2 = _mm_madd_epi16( m128iBD, _mm_load_si128( (__m128i*)( tab_idst_4x4[7] ) ) );
    S8  = _mm_add_epi32( m128iTmp1, m128iTmp2 );
    S8  = _mm_add_epi32( S8, m128iAdd );
    S8  = _mm_srai_epi32( S8, 12  );

    m128iD = _mm_packs_epi32( S0, S8 );

    m128iTmp1 = _mm_unpacklo_epi16(m128iA, m128iD);   // [32 30 22 20 12 10 02 00]
    m128iTmp2 = _mm_unpackhi_epi16(m128iA, m128iD);   // [33 31 23 21 13 11 03 01]
    m128iAC   = _mm_unpacklo_epi16(m128iTmp1, m128iTmp2);
    m128iBD   = _mm_unpackhi_epi16(m128iTmp1, m128iTmp2);

    _mm_storel_epi64( (__m128i*)&pDst[0 * stride], m128iAC );
    _mm_storeh_pi   ( (__m64*  )&pDst[1 * stride], _mm_castsi128_ps(m128iAC));
    _mm_storel_epi64( (__m128i*)&pDst[2 * stride], m128iBD );
    _mm_storeh_pi   ( (__m64*  )&pDst[3 * stride], _mm_castsi128_ps(m128iBD));
}

#endif // INSTRSET >= 5

#if INSTRSET < 4
void xDST4(short *pSrc, int *pDst, intptr_t nStride)
{
    const int shift_1st = 1;
    const int shift_2nd = 8;
    ALIGN_VAR_32(Short, tmp [4 * 4]);
    ALIGN_VAR_32(Short, tmp1[4 * 4]);

    for(int i=0; i<4; i++)
    {
        memcpy(&tmp1[i*4], &pSrc[i*nStride], 4*sizeof(short));
    }

    fastForwardDst(tmp1, tmp, shift_1st);
    fastForwardDst(tmp, tmp1, shift_2nd);
#define N (4)
    for(int i=0; i<N; i++)
    {
        for(int j=0; j<N; j++)
        {
            pDst[i*N+j] = tmp1[i*N+j];
        }
    }
#undef N
}

#else // INSTRSET >= 4

ALIGN_VAR_32(static const short, tab_dst_4[][8]) =
{
    {29, 55, 74, 84, 29, 55, 74, 84},
    {74, 74,  0,-74, 74, 74, 0 ,-74},
    {84,-29,-74, 55, 84,-29,-74, 55},
    {55,-84, 74,-29, 55,-84, 74,-29},
};

void xDST4(short *pSrc, int *pDst, intptr_t nStride)
{
    // Const
    __m128i c_1     = _mm_set1_epi32(1);
    __m128i c_128   = _mm_set1_epi32(128);

    // Load
    __m128i T20  = _mm_loadl_epi64((__m128i*)&pSrc[0*nStride]);
    __m128i T21  = _mm_loadl_epi64((__m128i*)&pSrc[1*nStride]);
    __m128i T22  = _mm_loadl_epi64((__m128i*)&pSrc[2*nStride]);
    __m128i T23  = _mm_loadl_epi64((__m128i*)&pSrc[3*nStride]);

    __m128i T30  = _mm_unpacklo_epi64(T20, T21);
    __m128i T31  = _mm_unpacklo_epi64(T22, T23);

    // DST1
    __m128i T40a = _mm_madd_epi16(T30, _mm_load_si128((__m128i*)tab_dst_4[0]));
    __m128i T40b = _mm_madd_epi16(T31, _mm_load_si128((__m128i*)tab_dst_4[0]));
    __m128i T41a = _mm_madd_epi16(T30, _mm_load_si128((__m128i*)tab_dst_4[1]));
    __m128i T41b = _mm_madd_epi16(T31, _mm_load_si128((__m128i*)tab_dst_4[1]));
    __m128i T42a = _mm_madd_epi16(T30, _mm_load_si128((__m128i*)tab_dst_4[2]));
    __m128i T42b = _mm_madd_epi16(T31, _mm_load_si128((__m128i*)tab_dst_4[2]));
    __m128i T43a = _mm_madd_epi16(T30, _mm_load_si128((__m128i*)tab_dst_4[3]));
    __m128i T43b = _mm_madd_epi16(T31, _mm_load_si128((__m128i*)tab_dst_4[3]));
    __m128i T50  = _mm_hadd_epi32(T40a, T40b);
    __m128i T51  = _mm_hadd_epi32(T41a, T41b);
    __m128i T52  = _mm_hadd_epi32(T42a, T42b);
    __m128i T53  = _mm_hadd_epi32(T43a, T43b);
    __m128i T60  = _mm_srai_epi32(_mm_add_epi32(T50, c_1), 1);  // [03 02 01 00]
    __m128i T61  = _mm_srai_epi32(_mm_add_epi32(T51, c_1), 1);  // [13 12 11 10]
    __m128i T62  = _mm_srai_epi32(_mm_add_epi32(T52, c_1), 1);  // [23 22 21 20]
    __m128i T63  = _mm_srai_epi32(_mm_add_epi32(T53, c_1), 1);  // [33 32 31 30]
    __m128i T70  = _mm_packs_epi32(T60, T61);
    __m128i T71  = _mm_packs_epi32(T62, T63);

    // DCT2
    T60 = _mm_madd_epi16(T70, _mm_load_si128((__m128i*)tab_dst_4[0]));
    T61 = _mm_madd_epi16(T71, _mm_load_si128((__m128i*)tab_dst_4[0]));
    T62 = _mm_madd_epi16(T70, _mm_load_si128((__m128i*)tab_dst_4[1]));
    T63 = _mm_madd_epi16(T71, _mm_load_si128((__m128i*)tab_dst_4[1]));
    T60 = _mm_hadd_epi32(T60, T61);
    T61 = _mm_hadd_epi32(T62, T63);
    T60 = _mm_srai_epi32(_mm_add_epi32(T60, c_128), 8);
    T61 = _mm_srai_epi32(_mm_add_epi32(T61, c_128), 8);
    _mm_store_si128((__m128i*)&pDst[0*4], T60);
    _mm_store_si128((__m128i*)&pDst[1*4], T61);

    T60 = _mm_madd_epi16(T70, _mm_load_si128((__m128i*)tab_dst_4[2]));
    T61 = _mm_madd_epi16(T71, _mm_load_si128((__m128i*)tab_dst_4[2]));
    T62 = _mm_madd_epi16(T70, _mm_load_si128((__m128i*)tab_dst_4[3]));
    T63 = _mm_madd_epi16(T71, _mm_load_si128((__m128i*)tab_dst_4[3]));
    T60 = _mm_hadd_epi32(T60, T61);
    T61 = _mm_hadd_epi32(T62, T63);
    T60 = _mm_srai_epi32(_mm_add_epi32(T60, c_128), 8);
    T61 = _mm_srai_epi32(_mm_add_epi32(T61, c_128), 8);
    _mm_store_si128((__m128i*)&pDst[2*4], T60);
    _mm_store_si128((__m128i*)&pDst[3*4], T61);
}
#endif // INSTRSET >=4

ALIGN_VAR_32(static const short, tab_dct_4[][8]) =
{
    { 64, 64, 64, 64, 64, 64, 64, 64},
    { 83, 36, 83, 36, 83, 36, 83, 36},
    { 64,-64, 64,-64, 64,-64, 64,-64},
    { 36,-83, 36,-83, 36,-83, 36,-83},
};
void xDCT4(short *pSrc, int *pDst, intptr_t nStride)
{
    // Const
    __m128i c_1         = _mm_set1_epi32(1);
    __m128i c_128       = _mm_set1_epi32(128);

    __m128i T20, T21;
    __m128i T30, T31, T32, T33;
    __m128i T40, T41, T50, T51, T60, T61, T62, T63, T70, T71, T72, T73;
    __m128i T50_, T51_;

    __m128i T10  = _mm_loadl_epi64((__m128i*)&pSrc[0*nStride]);
    __m128i T11  = _mm_loadl_epi64((__m128i*)&pSrc[1*nStride]);
    __m128i T12  = _mm_loadl_epi64((__m128i*)&pSrc[2*nStride]);
    __m128i T13  = _mm_loadl_epi64((__m128i*)&pSrc[3*nStride]);

    T20  = _mm_unpacklo_epi64(T10, T11);
    T21  = _mm_unpacklo_epi64(T12, T13);

    // DCT1
    T30  = _mm_shuffle_epi32(T20, 0xD8);        // [13 12 03 02 11 10 01 00]
    T31  = _mm_shuffle_epi32(T21, 0xD8);        // [33 32 23 22 31 30 21 20]
    T32  = _mm_shufflehi_epi16(T30, 0xB1);      // [12 13 02 03 11 10 01 00]
    T33  = _mm_shufflehi_epi16(T31, 0xB1);      // [32 33 22 23 31 30 21 20]

    T40  = _mm_unpacklo_epi64(T32, T33);        // [31 30 21 20 11 10 01 00]
    T41  = _mm_unpackhi_epi64(T32, T33);        // [32 33 22 23 12 13 02 03]
    T50  = _mm_add_epi16(T40, T41);             // [1+2 0+3]
    T51  = _mm_sub_epi16(T40, T41);             // [1-2 0-3]
    T60  = _mm_madd_epi16(T50, _mm_load_si128((__m128i*)tab_dct_4[0])); // [ 64*s12 + 64*s03] = [03 02 01 00]
    T61  = _mm_madd_epi16(T51, _mm_load_si128((__m128i*)tab_dct_4[1])); // [ 36*d12 + 83*d03] = [13 12 11 10]
    T62  = _mm_madd_epi16(T50, _mm_load_si128((__m128i*)tab_dct_4[2])); // [-64*s12 + 64*s03] = [23 22 21 20]
    T63  = _mm_madd_epi16(T51, _mm_load_si128((__m128i*)tab_dct_4[3])); // [-83*d12 + 36*d03] = [33 32 31 30]
    T70  = _mm_srai_epi32(_mm_add_epi32(T60, c_1), 1);  // [30 20 10 00]
    T71  = _mm_srai_epi32(_mm_add_epi32(T61, c_1), 1);  // [31 21 11 01]
    T72  = _mm_srai_epi32(_mm_add_epi32(T62, c_1), 1);  // [32 22 12 02]
    T73  = _mm_srai_epi32(_mm_add_epi32(T63, c_1), 1);  // [33 23 13 03]

    // Transpose
    T20  = _mm_packs_epi32(T70, T71);       // [13 12 11 10 03 02 01 00]
    T21  = _mm_packs_epi32(T72, T73);       // [33 32 31 30 23 22 21 20]

    T30  = _mm_shuffle_epi32(T20, 0xD8);        // [13 12 03 02 11 10 01 00]
    T31  = _mm_shuffle_epi32(T21, 0xD8);        // [33 32 23 22 31 30 21 20]
    T32  = _mm_shufflehi_epi16(T30, 0xB1);      // [12 13 02 03 11 10 01 00]
    T33  = _mm_shufflehi_epi16(T31, 0xB1);      // [32 33 22 23 31 30 21 20]

    T40  = _mm_unpacklo_epi64(T32, T33);        // [31 30 21 20 11 10 01 00]
    T41  = _mm_unpackhi_epi64(T32, T33);        // [32 33 22 23 12 13 02 03]

    T50_ = _mm_madd_epi16(T40, _mm_load_si128((__m128i*)tab_dct_4[0]));
    T51_ = _mm_madd_epi16(T41, _mm_load_si128((__m128i*)tab_dct_4[0]));
    T60  = _mm_add_epi32(T50_, T51_);
    T50_ = _mm_madd_epi16(T40, _mm_load_si128((__m128i*)tab_dct_4[1]));
    T51_ = _mm_madd_epi16(T41, _mm_load_si128((__m128i*)tab_dct_4[1]));
    T61  = _mm_sub_epi32(T50_, T51_);
    T50_ = _mm_madd_epi16(T40, _mm_load_si128((__m128i*)tab_dct_4[2]));
    T51_ = _mm_madd_epi16(T41, _mm_load_si128((__m128i*)tab_dct_4[2]));
    T62  = _mm_add_epi32(T50_, T51_);
    T50_ = _mm_madd_epi16(T40, _mm_load_si128((__m128i*)tab_dct_4[3]));
    T51_ = _mm_madd_epi16(T41, _mm_load_si128((__m128i*)tab_dct_4[3]));
    T63  = _mm_sub_epi32(T50_, T51_);
    
    T70  = _mm_srai_epi32(_mm_add_epi32(T60, c_128), 8);  // [30 20 10 00]
    T71  = _mm_srai_epi32(_mm_add_epi32(T61, c_128), 8);  // [31 21 11 01]
    T72  = _mm_srai_epi32(_mm_add_epi32(T62, c_128), 8);  // [32 22 12 02]
    T73  = _mm_srai_epi32(_mm_add_epi32(T63, c_128), 8);  // [33 23 13 03]

    _mm_storeu_si128((__m128i*)&pDst[0 * 4], T70);
    _mm_storeu_si128((__m128i*)&pDst[1 * 4], T71);
    _mm_storeu_si128((__m128i*)&pDst[2 * 4], T72);
    _mm_storeu_si128((__m128i*)&pDst[3 * 4], T73);
}


#if INSTRSET < 4
inline void partialButterfly8(short *src, short *dst, int shift, int line)
{
    int j;
    int add = 1 << (shift - 1);

    Vec4i g_aiT8_zero_row(64, 64, 0, 0);
    Vec4i g_aiT8_four_row(64, -64, 0, 0);
    Vec4i g_aiT8_two_row(83, 36, 0, 0);
    Vec4i g_aiT8_six_row(36, -83, 0, 0);

    Vec4i g_aiT8_one_row(89, 75, 50, 18);
    Vec4i g_aiT8_three_row(75, -18, -89, -50);
    Vec4i g_aiT8_five_row(50, -89, 18, 75);
    Vec4i g_aiT8_seven_row(18, -50, 75, -89);

    for (j = 0; j < line; j++)
    {
        Vec8s tmp;
        tmp.load(src);

        Vec4i E_first_half = extend_low(tmp);
        Vec4i E_second_half = extend_high(tmp);
        E_second_half = permute4i<3, 2, 1, 0>(E_second_half);

        Vec4i E = E_first_half + E_second_half;
        Vec4i O = E_first_half - E_second_half;

        Vec4i EE_first_half = permute4i<0, 1, -1, -1>(E);
        Vec4i EE_second_half = permute4i<3, 2, -1, -1>(E);
        Vec4i EE = EE_first_half + EE_second_half;
        Vec4i EO = EE_first_half - EE_second_half;

        int dst0 = ((horizontal_add(g_aiT8_zero_row * EE)) + add) >> shift;
        int dst4 = ((horizontal_add(g_aiT8_four_row * EE)) + add) >> shift;
        int dst2 = ((horizontal_add(g_aiT8_two_row * EO)) + add) >> shift;
        int dst6 = ((horizontal_add(g_aiT8_six_row * EO)) + add) >> shift;

        dst[0] = dst0;
        dst[4 * line] = dst4;
        dst[2 * line] = dst2;
        dst[6 * line] = dst6;

        int dst1 = ((horizontal_add(g_aiT8_one_row * O)) + add) >> shift;
        int dst3 = ((horizontal_add(g_aiT8_three_row * O)) + add) >> shift;
        int dst5 = ((horizontal_add(g_aiT8_five_row * O)) + add) >> shift;
        int dst7 = ((horizontal_add(g_aiT8_seven_row * O)) + add) >> shift;

        dst[line] = dst1;
        dst[3 * line] = dst3;
        dst[5 * line] = dst5;
        dst[7 * line] = dst7;

        src += 8;
        dst++;
    }
}

void xDCT8(short *pSrc, int *pDst, intptr_t nStride)
{
    const int shift_1st = 2;
    const int shift_2nd = 9;
    ALIGN_VAR_32(Short, tmp [8 * 8]);
    ALIGN_VAR_32(Short, tmp1[8 * 8]);

    for(int i=0; i<8; i++)
    {
        memcpy(&tmp1[i*8], &pSrc[i*nStride], 8*sizeof(short));
    }

    partialButterfly8(tmp1, tmp, shift_1st, 8);
    partialButterfly8(tmp, tmp1, shift_2nd, 8);
#define N (8)
    for(int i=0; i<N; i++)
    {
        for(int j=0; j<N; j++)
        {
            pDst[i*N+j] = tmp1[i*N+j];
        }
    }
#undef N
}

#else // INSTRSET >= 4

ALIGN_VAR_32(static const short, tab_dct_8[][8])=
{
    { 0x0100, 0x0F0E, 0x0706, 0x0908, 0x0302, 0x0D0C, 0x0504, 0x0B0A},

    { 64, 64, 64, 64, 64, 64, 64, 64},
    { 64,-64, 64,-64, 64,-64, 64,-64},
    { 83, 36, 83, 36, 83, 36, 83, 36},
    { 36,-83, 36,-83, 36,-83, 36,-83},
    { 89, 18, 75, 50, 89, 18, 75, 50},
    { 75,-50,-18,-89, 75,-50,-18,-89},
    { 50, 75,-89, 18, 50, 75,-89, 18},
    { 18,-89,-50, 75, 18,-89,-50, 75},

    { 83, 83,-83,-83, 36, 36,-36,-36},
    { 36, 36,-36,-36,-83,-83, 83, 83},
    { 89,-89, 18,-18, 75,-75, 50,-50},
    { 75,-75,-50, 50,-18, 18,-89, 89},
    { 50,-50, 75,-75,-89, 89, 18,-18},
    { 18,-18,-89, 89,-50, 50, 75,-75},
};
void xDCT8(short *pSrc, int *pDst, intptr_t nStride)
{
    // Const
    __m128i c_2     = _mm_set1_epi32(2);
    __m128i c_256   = _mm_set1_epi32(256);

    // DCT1
    __m128i T00, T01, T02, T03, T04, T05, T06, T07;
    __m128i T10, T11, T12, T13, T14, T15, T16, T17;
    __m128i T20, T21, T22, T23, T24, T25, T26, T27;
    __m128i T30, T31, T32, T33;
    __m128i T40, T41, T42, T43, T44, T45, T46, T47;
    __m128i T50, T51, T52, T53, T54, T55, T56, T57;

    T00 = _mm_load_si128((__m128i*)&pSrc[0 * nStride]);   // [07 06 05 04 03 02 01 00]
    T01 = _mm_load_si128((__m128i*)&pSrc[1 * nStride]);   // [17 16 15 14 13 12 11 10]
    T02 = _mm_load_si128((__m128i*)&pSrc[2 * nStride]);   // [27 26 25 24 23 22 21 20]
    T03 = _mm_load_si128((__m128i*)&pSrc[3 * nStride]);   // [37 36 35 34 33 32 31 30]
    T04 = _mm_load_si128((__m128i*)&pSrc[4 * nStride]);   // [47 46 45 44 43 42 41 40]
    T05 = _mm_load_si128((__m128i*)&pSrc[5 * nStride]);   // [57 56 55 54 53 52 51 50]
    T06 = _mm_load_si128((__m128i*)&pSrc[6 * nStride]);   // [67 66 65 64 63 62 61 60]
    T07 = _mm_load_si128((__m128i*)&pSrc[7 * nStride]);   // [77 76 75 74 73 72 71 70]

    T10 = _mm_shuffle_epi8(T00, _mm_load_si128((__m128i*)tab_dct_8[0]));  // [05 02 06 01 04 03 07 00]
    T11 = _mm_shuffle_epi8(T01, _mm_load_si128((__m128i*)tab_dct_8[0]));
    T12 = _mm_shuffle_epi8(T02, _mm_load_si128((__m128i*)tab_dct_8[0]));
    T13 = _mm_shuffle_epi8(T03, _mm_load_si128((__m128i*)tab_dct_8[0]));
    T14 = _mm_shuffle_epi8(T04, _mm_load_si128((__m128i*)tab_dct_8[0]));
    T15 = _mm_shuffle_epi8(T05, _mm_load_si128((__m128i*)tab_dct_8[0]));
    T16 = _mm_shuffle_epi8(T06, _mm_load_si128((__m128i*)tab_dct_8[0]));
    T17 = _mm_shuffle_epi8(T07, _mm_load_si128((__m128i*)tab_dct_8[0]));

    T20 = _mm_hadd_epi16(T10, T11);     // [s25_1 s16_1 s34_1 s07_1 s25_0 s16_0 s34_0 s07_0]
    T21 = _mm_hadd_epi16(T12, T13);     // [s25_3 s16_3 s34_3 s07_3 s25_2 s16_2 s34_2 s07_2]
    T22 = _mm_hadd_epi16(T14, T15);     // [s25_5 s16_5 s34_5 s07_5 s25_4 s16_4 s34_4 s07_4]
    T23 = _mm_hadd_epi16(T16, T17);     // [s25_7 s16_7 s34_7 s07_7 s25_6 s16_6 s34_6 s07_6]

    T24 = _mm_hsub_epi16(T10, T11);     // [d25_1 d16_1 d34_1 d07_1 d25_0 d16_0 d34_0 d07_0]
    T25 = _mm_hsub_epi16(T12, T13);     // [d25_3 d16_3 d34_3 d07_3 d25_2 d16_2 d34_2 d07_2]
    T26 = _mm_hsub_epi16(T14, T15);     // [d25_5 d16_5 d34_5 d07_5 d25_4 d16_4 d34_4 d07_4]
    T27 = _mm_hsub_epi16(T16, T17);     // [d25_7 d16_7 d34_7 d07_7 d25_6 d16_6 d34_6 d07_6]

    T30 = _mm_hadd_epi16(T20, T21);     // [EE1_3 EE0_3 EE1_2 EE0_2 EE1_1 EE0_1 EE1_0 EE0_0]
    T31 = _mm_hadd_epi16(T22, T23);     // [EE1_7 EE0_7 EE1_6 EE0_6 EE1_5 EE0_5 EE1_4 EE0_4]
    T32 = _mm_hsub_epi16(T20, T21);     // [EO1_3 EO0_3 EO1_2 EO0_2 EO1_1 EO0_1 EO1_0 EO0_0]
    T33 = _mm_hsub_epi16(T22, T23);     // [EO1_7 EO0_7 EO1_6 EO0_6 EO1_5 EO0_5 EO1_4 EO0_4]

    T40 = _mm_madd_epi16(T30, _mm_load_si128((__m128i*)tab_dct_8[1]));
    T41 = _mm_madd_epi16(T31, _mm_load_si128((__m128i*)tab_dct_8[1]));
    T40 = _mm_srai_epi32(_mm_add_epi32(T40, c_2), 2);
    T41 = _mm_srai_epi32(_mm_add_epi32(T41, c_2), 2);
    T50 = _mm_packs_epi32(T40, T41);

    T42 = _mm_madd_epi16(T30, _mm_load_si128((__m128i*)tab_dct_8[2]));
    T43 = _mm_madd_epi16(T31, _mm_load_si128((__m128i*)tab_dct_8[2]));
    T42 = _mm_srai_epi32(_mm_add_epi32(T42, c_2), 2);
    T43 = _mm_srai_epi32(_mm_add_epi32(T43, c_2), 2);
    T54 = _mm_packs_epi32(T42, T43);

    T44 = _mm_madd_epi16(T32, _mm_load_si128((__m128i*)tab_dct_8[3]));
    T45 = _mm_madd_epi16(T33, _mm_load_si128((__m128i*)tab_dct_8[3]));
    T44 = _mm_srai_epi32(_mm_add_epi32(T44, c_2), 2);
    T45 = _mm_srai_epi32(_mm_add_epi32(T45, c_2), 2);
    T52 = _mm_packs_epi32(T44, T45);

    T46 = _mm_madd_epi16(T32, _mm_load_si128((__m128i*)tab_dct_8[4]));
    T47 = _mm_madd_epi16(T33, _mm_load_si128((__m128i*)tab_dct_8[4]));
    T46 = _mm_srai_epi32(_mm_add_epi32(T46, c_2), 2);
    T47 = _mm_srai_epi32(_mm_add_epi32(T47, c_2), 2);
    T56 = _mm_packs_epi32(T46, T47);

    T40 = _mm_madd_epi16(T24, _mm_load_si128((__m128i*)tab_dct_8[5]));
    T41 = _mm_madd_epi16(T25, _mm_load_si128((__m128i*)tab_dct_8[5]));
    T42 = _mm_madd_epi16(T26, _mm_load_si128((__m128i*)tab_dct_8[5]));
    T43 = _mm_madd_epi16(T27, _mm_load_si128((__m128i*)tab_dct_8[5]));
    T40 = _mm_hadd_epi32(T40, T41);
    T42 = _mm_hadd_epi32(T42, T43);
    T40 = _mm_srai_epi32(_mm_add_epi32(T40, c_2), 2);
    T42 = _mm_srai_epi32(_mm_add_epi32(T42, c_2), 2);
    T51 = _mm_packs_epi32(T40, T42);

    T40 = _mm_madd_epi16(T24, _mm_load_si128((__m128i*)tab_dct_8[6]));
    T41 = _mm_madd_epi16(T25, _mm_load_si128((__m128i*)tab_dct_8[6]));
    T42 = _mm_madd_epi16(T26, _mm_load_si128((__m128i*)tab_dct_8[6]));
    T43 = _mm_madd_epi16(T27, _mm_load_si128((__m128i*)tab_dct_8[6]));
    T40 = _mm_hadd_epi32(T40, T41);
    T42 = _mm_hadd_epi32(T42, T43);
    T40 = _mm_srai_epi32(_mm_add_epi32(T40, c_2), 2);
    T42 = _mm_srai_epi32(_mm_add_epi32(T42, c_2), 2);
    T53 = _mm_packs_epi32(T40, T42);

    T40 = _mm_madd_epi16(T24, _mm_load_si128((__m128i*)tab_dct_8[7]));
    T41 = _mm_madd_epi16(T25, _mm_load_si128((__m128i*)tab_dct_8[7]));
    T42 = _mm_madd_epi16(T26, _mm_load_si128((__m128i*)tab_dct_8[7]));
    T43 = _mm_madd_epi16(T27, _mm_load_si128((__m128i*)tab_dct_8[7]));
    T40 = _mm_hadd_epi32(T40, T41);
    T42 = _mm_hadd_epi32(T42, T43);
    T40 = _mm_srai_epi32(_mm_add_epi32(T40, c_2), 2);
    T42 = _mm_srai_epi32(_mm_add_epi32(T42, c_2), 2);
    T55 = _mm_packs_epi32(T40, T42);

    T40 = _mm_madd_epi16(T24, _mm_load_si128((__m128i*)tab_dct_8[8]));
    T41 = _mm_madd_epi16(T25, _mm_load_si128((__m128i*)tab_dct_8[8]));
    T42 = _mm_madd_epi16(T26, _mm_load_si128((__m128i*)tab_dct_8[8]));
    T43 = _mm_madd_epi16(T27, _mm_load_si128((__m128i*)tab_dct_8[8]));
    T40 = _mm_hadd_epi32(T40, T41);
    T42 = _mm_hadd_epi32(T42, T43);
    T40 = _mm_srai_epi32(_mm_add_epi32(T40, c_2), 2);
    T42 = _mm_srai_epi32(_mm_add_epi32(T42, c_2), 2);
    T57 = _mm_packs_epi32(T40, T42);

    T10 = _mm_shuffle_epi8(T50, _mm_load_si128((__m128i*)tab_dct_8[0]));  // [05 02 06 01 04 03 07 00]
    T11 = _mm_shuffle_epi8(T51, _mm_load_si128((__m128i*)tab_dct_8[0]));
    T12 = _mm_shuffle_epi8(T52, _mm_load_si128((__m128i*)tab_dct_8[0]));
    T13 = _mm_shuffle_epi8(T53, _mm_load_si128((__m128i*)tab_dct_8[0]));
    T14 = _mm_shuffle_epi8(T54, _mm_load_si128((__m128i*)tab_dct_8[0]));
    T15 = _mm_shuffle_epi8(T55, _mm_load_si128((__m128i*)tab_dct_8[0]));
    T16 = _mm_shuffle_epi8(T56, _mm_load_si128((__m128i*)tab_dct_8[0]));
    T17 = _mm_shuffle_epi8(T57, _mm_load_si128((__m128i*)tab_dct_8[0]));

    // DCT2
    T20 = _mm_madd_epi16(T10, _mm_load_si128((__m128i*)tab_dct_8[1]));    // [64*s25_0 64*s16_0 64*s34_0 64*s07_0]
    T21 = _mm_madd_epi16(T11, _mm_load_si128((__m128i*)tab_dct_8[1]));    // [64*s25_1 64*s16_1 64*s34_1 64*s07_1]
    T22 = _mm_madd_epi16(T12, _mm_load_si128((__m128i*)tab_dct_8[1]));    // [64*s25_2 64*s16_2 64*s34_2 64*s07_2]
    T23 = _mm_madd_epi16(T13, _mm_load_si128((__m128i*)tab_dct_8[1]));    // [64*s25_3 64*s16_3 64*s34_3 64*s07_3]
    T24 = _mm_madd_epi16(T14, _mm_load_si128((__m128i*)tab_dct_8[1]));    // [64*s25_4 64*s16_4 64*s34_4 64*s07_4]
    T25 = _mm_madd_epi16(T15, _mm_load_si128((__m128i*)tab_dct_8[1]));    // [64*s25_5 64*s16_5 64*s34_5 64*s07_5]
    T26 = _mm_madd_epi16(T16, _mm_load_si128((__m128i*)tab_dct_8[1]));    // [64*s25_6 64*s16_6 64*s34_6 64*s07_6]
    T27 = _mm_madd_epi16(T17, _mm_load_si128((__m128i*)tab_dct_8[1]));    // [64*s25_7 64*s16_7 64*s34_7 64*s07_7]

    T30 = _mm_hadd_epi32(T20, T21); // [64*(s16+s25)_1 64*(s07+s34)_1 64*(s16+s25)_0 64*(s07+s34)_0]
    T31 = _mm_hadd_epi32(T22, T23); // [64*(s16+s25)_3 64*(s07+s34)_3 64*(s16+s25)_2 64*(s07+s34)_2]
    T32 = _mm_hadd_epi32(T24, T25); // [64*(s16+s25)_5 64*(s07+s34)_5 64*(s16+s25)_4 64*(s07+s34)_4]
    T33 = _mm_hadd_epi32(T26, T27); // [64*(s16+s25)_7 64*(s07+s34)_7 64*(s16+s25)_6 64*(s07+s34)_6]

    T40 = _mm_hadd_epi32(T30, T31); // [64*((s07+s34)+(s16+s25))_3 64*((s07+s34)+(s16+s25))_2 64*((s07+s34)+(s16+s25))_1 64*((s07+s34)+(s16+s25))_0]
    T41 = _mm_hadd_epi32(T32, T33); // [64*((s07+s34)+(s16+s25))_7 64*((s07+s34)+(s16+s25))_6 64*((s07+s34)+(s16+s25))_5 64*((s07+s34)+(s16+s25))_4]
    T42 = _mm_hsub_epi32(T30, T31); // [64*((s07+s34)-(s16+s25))_3 64*((s07+s34)-(s16+s25))_2 64*((s07+s34)-(s16+s25))_1 64*((s07+s34)-(s16+s25))_0]
    T43 = _mm_hsub_epi32(T32, T33); // [64*((s07+s34)-(s16+s25))_7 64*((s07+s34)-(s16+s25))_6 64*((s07+s34)-(s16+s25))_5 64*((s07+s34)-(s16+s25))_4]

    T50 = _mm_srai_epi32(_mm_add_epi32(T40, c_256), 9);
    T51 = _mm_srai_epi32(_mm_add_epi32(T41, c_256), 9);
    T52 = _mm_srai_epi32(_mm_add_epi32(T42, c_256), 9);
    T53 = _mm_srai_epi32(_mm_add_epi32(T43, c_256), 9);

    _mm_store_si128((__m128i*)&pDst[0*8+0], T50);
    _mm_store_si128((__m128i*)&pDst[0*8+4], T51);
    _mm_store_si128((__m128i*)&pDst[4*8+0], T52);
    _mm_store_si128((__m128i*)&pDst[4*8+4], T53);

#define MAKE_ODD(tab, dst) \
    T20 = _mm_madd_epi16(T10, _mm_load_si128((__m128i*)tab_dct_8[(tab)])); \
    T21 = _mm_madd_epi16(T11, _mm_load_si128((__m128i*)tab_dct_8[(tab)])); \
    T22 = _mm_madd_epi16(T12, _mm_load_si128((__m128i*)tab_dct_8[(tab)])); \
    T23 = _mm_madd_epi16(T13, _mm_load_si128((__m128i*)tab_dct_8[(tab)])); \
    T24 = _mm_madd_epi16(T14, _mm_load_si128((__m128i*)tab_dct_8[(tab)])); \
    T25 = _mm_madd_epi16(T15, _mm_load_si128((__m128i*)tab_dct_8[(tab)])); \
    T26 = _mm_madd_epi16(T16, _mm_load_si128((__m128i*)tab_dct_8[(tab)])); \
    T27 = _mm_madd_epi16(T17, _mm_load_si128((__m128i*)tab_dct_8[(tab)])); \
    T30 = _mm_hadd_epi32(T20, T21); \
    T31 = _mm_hadd_epi32(T22, T23); \
    T32 = _mm_hadd_epi32(T24, T25); \
    T33 = _mm_hadd_epi32(T26, T27); \
    T40 = _mm_hadd_epi32(T30, T31); \
    T41 = _mm_hadd_epi32(T32, T33); \
    T50 = _mm_srai_epi32(_mm_add_epi32(T40, c_256), 9); \
    T51 = _mm_srai_epi32(_mm_add_epi32(T41, c_256), 9); \
    _mm_store_si128((__m128i*)&pDst[(dst)*8+0], T50); \
    _mm_store_si128((__m128i*)&pDst[(dst)*8+4], T51);

    MAKE_ODD( 9, 2);
    MAKE_ODD(10, 6);
    MAKE_ODD(11, 1);
    MAKE_ODD(12, 3);
    MAKE_ODD(13, 5);
    MAKE_ODD(14, 7);
#undef MAKE_ODD
}
#endif // INSTRSET >= 4

#if INSTRSET < 4
inline void partialButterfly16(short *src, short *dst, int shift, int line)
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

void xDCT16(short *pSrc, int *pDst, intptr_t nStride)
{
    const int shift_1st = 3;
    const int shift_2nd = 10;
    ALIGN_VAR_32(Short, tmp [16 * 16]);
    ALIGN_VAR_32(Short, tmp1[16 * 16]);

    for(int i=0; i<16; i++)
    {
        memcpy(&tmp1[i*16], &pSrc[i*nStride], 16*sizeof(short));
    }

    partialButterfly16(tmp1, tmp, shift_1st, 16);
    partialButterfly16(tmp, tmp1, shift_2nd, 16);
#define N (16)
    for(int i=0; i<N; i++)
    {
        for(int j=0; j<N; j++)
        {
            pDst[i*N+j] = tmp1[i*N+j];
        }
    }
#undef N
}

#else //INSTRSET >= 4

ALIGN_VAR_32(static const short, tab_dct_16_0[][8])=
{
    { 0x0F0E, 0x0D0C, 0x0B0A, 0x0908, 0x0706, 0x0504, 0x0302, 0x0100},  // 0
    { 0x0100, 0x0F0E, 0x0706, 0x0908, 0x0302, 0x0D0C, 0x0504, 0x0B0A},  // 1
    { 0x0100, 0x0706, 0x0302, 0x0504, 0x0F0E, 0x0908, 0x0D0C, 0x0B0A},  // 2
    { 0x0F0E, 0x0908, 0x0D0C, 0x0B0A, 0x0100, 0x0706, 0x0302, 0x0504},  // 3
};

ALIGN_VAR_32(static const short, tab_dct_16_1[][8])=
{
    { 90, 87, 80, 70, 57, 43, 25,  9},  //  0
    { 87, 57,  9,-43,-80,-90,-70,-25},  //  1
    { 80,  9,-70,-87,-25, 57, 90, 43},  //  2
    { 70,-43,-87,  9, 90, 25,-80,-57},  //  3
    { 57,-80,-25, 90,- 9,-87, 43, 70},  //  4
    { 43,-90, 57, 25,-87, 70,  9,-80},  //  5
    { 25,-70, 90,-80, 43,  9,-57, 87},  //  6
    {  9,-25, 43,-57, 70,-80, 87,-90},  //  7
    { 83, 83,-83,-83, 36, 36,-36,-36},  //  8
    { 36, 36,-36,-36,-83,-83, 83, 83},  //  9
    { 89, 89, 18, 18, 75, 75, 50, 50},  // 10
    { 75, 75,-50,-50,-18,-18,-89,-89},  // 11
    { 50, 50, 75, 75,-89,-89, 18, 18},  // 12
    { 18, 18,-89,-89,-50,-50, 75, 75},  // 13

#define MAKE_COEF(a0, a1, a2, a3, a4, a5, a6, a7) \
    { (a0), -(a0), (a3), -(a3), (a1), -(a1), (a2), -(a2)}, \
    { (a7), -(a7), (a4), -(a4), (a6), -(a6), (a5), -(a5)},

    MAKE_COEF(90, 87, 80, 70, 57, 43, 25,  9)
    MAKE_COEF(87, 57,  9,-43,-80,-90,-70,-25)
    MAKE_COEF(80,  9,-70,-87,-25, 57, 90, 43)
    MAKE_COEF(70,-43,-87,  9, 90, 25,-80,-57)
    MAKE_COEF(57,-80,-25, 90, -9,-87, 43, 70)
    MAKE_COEF(43,-90, 57, 25,-87, 70,  9,-80)
    MAKE_COEF(25,-70, 90,-80, 43,  9,-57, 87)
    MAKE_COEF( 9,-25, 43,-57, 70,-80, 87,-90)
#undef MAKE_COEF
};

void xDCT16(short *pSrc, int *pDst, intptr_t nStride)
{
    // Const
    __m128i c_4     = _mm_set1_epi32(4);
    __m128i c_512   = _mm_set1_epi32(512);

    int i;

    ALIGN_VAR_32(short, tmp[16*16]);

    __m128i T00A, T01A, T02A, T03A, T04A, T05A, T06A, T07A;
    __m128i T00B, T01B, T02B, T03B, T04B, T05B, T06B, T07B;
    __m128i T10, T11, T12, T13, T14, T15, T16, T17;
    __m128i T20, T21, T22, T23, T24, T25, T26, T27;
    __m128i T30, T31, T32, T33, T34, T35, T36, T37;
    __m128i T40, T41, T42, T43, T44, T45, T46, T47;
    __m128i T50, T51, T52, T53;
    __m128i T60, T61, T62, T63, T64, T65, T66, T67;
    __m128i T70;

    // DCT1
    for( i=0; i<16; i+=8 )
    {
        T00A = _mm_load_si128((__m128i*)&pSrc[(i+0)*nStride+0]);    // [07 06 05 04 03 02 01 00]
        T00B = _mm_load_si128((__m128i*)&pSrc[(i+0)*nStride+8]);    // [0F 0E 0D 0C 0B 0A 09 08]
        T01A = _mm_load_si128((__m128i*)&pSrc[(i+1)*nStride+0]);    // [17 16 15 14 13 12 11 10]
        T01B = _mm_load_si128((__m128i*)&pSrc[(i+1)*nStride+8]);    // [1F 1E 1D 1C 1B 1A 19 18]
        T02A = _mm_load_si128((__m128i*)&pSrc[(i+2)*nStride+0]);    // [27 26 25 24 23 22 21 20]
        T02B = _mm_load_si128((__m128i*)&pSrc[(i+2)*nStride+8]);    // [2F 2E 2D 2C 2B 2A 29 28]
        T03A = _mm_load_si128((__m128i*)&pSrc[(i+3)*nStride+0]);    // [37 36 35 34 33 32 31 30]
        T03B = _mm_load_si128((__m128i*)&pSrc[(i+3)*nStride+8]);    // [3F 3E 3D 3C 3B 3A 39 38]
        T04A = _mm_load_si128((__m128i*)&pSrc[(i+4)*nStride+0]);    // [47 46 45 44 43 42 41 40]
        T04B = _mm_load_si128((__m128i*)&pSrc[(i+4)*nStride+8]);    // [4F 4E 4D 4C 4B 4A 49 48]
        T05A = _mm_load_si128((__m128i*)&pSrc[(i+5)*nStride+0]);    // [57 56 55 54 53 52 51 50]
        T05B = _mm_load_si128((__m128i*)&pSrc[(i+5)*nStride+8]);    // [5F 5E 5D 5C 5B 5A 59 58]
        T06A = _mm_load_si128((__m128i*)&pSrc[(i+6)*nStride+0]);    // [67 66 65 64 63 62 61 60]
        T06B = _mm_load_si128((__m128i*)&pSrc[(i+6)*nStride+8]);    // [6F 6E 6D 6C 6B 6A 69 68]
        T07A = _mm_load_si128((__m128i*)&pSrc[(i+7)*nStride+0]);    // [77 76 75 74 73 72 71 70]
        T07B = _mm_load_si128((__m128i*)&pSrc[(i+7)*nStride+8]);    // [7F 7E 7D 7C 7B 7A 79 78]

        T00B = _mm_shuffle_epi8(T00B, _mm_load_si128((__m128i*)tab_dct_16_0[0]));
        T01B = _mm_shuffle_epi8(T01B, _mm_load_si128((__m128i*)tab_dct_16_0[0]));
        T02B = _mm_shuffle_epi8(T02B, _mm_load_si128((__m128i*)tab_dct_16_0[0]));
        T03B = _mm_shuffle_epi8(T03B, _mm_load_si128((__m128i*)tab_dct_16_0[0]));
        T04B = _mm_shuffle_epi8(T04B, _mm_load_si128((__m128i*)tab_dct_16_0[0]));
        T05B = _mm_shuffle_epi8(T05B, _mm_load_si128((__m128i*)tab_dct_16_0[0]));
        T06B = _mm_shuffle_epi8(T06B, _mm_load_si128((__m128i*)tab_dct_16_0[0]));
        T07B = _mm_shuffle_epi8(T07B, _mm_load_si128((__m128i*)tab_dct_16_0[0]));

        T10  = _mm_add_epi16(T00A, T00B);
        T11  = _mm_add_epi16(T01A, T01B);
        T12  = _mm_add_epi16(T02A, T02B);
        T13  = _mm_add_epi16(T03A, T03B);
        T14  = _mm_add_epi16(T04A, T04B);
        T15  = _mm_add_epi16(T05A, T05B);
        T16  = _mm_add_epi16(T06A, T06B);
        T17  = _mm_add_epi16(T07A, T07B);

        T20  = _mm_sub_epi16(T00A, T00B);
        T21  = _mm_sub_epi16(T01A, T01B);
        T22  = _mm_sub_epi16(T02A, T02B);
        T23  = _mm_sub_epi16(T03A, T03B);
        T24  = _mm_sub_epi16(T04A, T04B);
        T25  = _mm_sub_epi16(T05A, T05B);
        T26  = _mm_sub_epi16(T06A, T06B);
        T27  = _mm_sub_epi16(T07A, T07B);

        T30  = _mm_shuffle_epi8(T10, _mm_load_si128((__m128i*)tab_dct_16_0[1]));
        T31  = _mm_shuffle_epi8(T11, _mm_load_si128((__m128i*)tab_dct_16_0[1]));
        T32  = _mm_shuffle_epi8(T12, _mm_load_si128((__m128i*)tab_dct_16_0[1]));
        T33  = _mm_shuffle_epi8(T13, _mm_load_si128((__m128i*)tab_dct_16_0[1]));
        T34  = _mm_shuffle_epi8(T14, _mm_load_si128((__m128i*)tab_dct_16_0[1]));
        T35  = _mm_shuffle_epi8(T15, _mm_load_si128((__m128i*)tab_dct_16_0[1]));
        T36  = _mm_shuffle_epi8(T16, _mm_load_si128((__m128i*)tab_dct_16_0[1]));
        T37  = _mm_shuffle_epi8(T17, _mm_load_si128((__m128i*)tab_dct_16_0[1]));

        T40  = _mm_hadd_epi16(T30, T31);
        T41  = _mm_hadd_epi16(T32, T33);
        T42  = _mm_hadd_epi16(T34, T35);
        T43  = _mm_hadd_epi16(T36, T37);
        T44  = _mm_hsub_epi16(T30, T31);
        T45  = _mm_hsub_epi16(T32, T33);
        T46  = _mm_hsub_epi16(T34, T35);
        T47  = _mm_hsub_epi16(T36, T37);

        T50  = _mm_hadd_epi16(T40, T41);
        T51  = _mm_hadd_epi16(T42, T43);
        T52  = _mm_hsub_epi16(T40, T41);
        T53  = _mm_hsub_epi16(T42, T43);

        T60  = _mm_madd_epi16(T50, _mm_load_si128((__m128i*)tab_dct_8[ 1]));
        T61  = _mm_madd_epi16(T51, _mm_load_si128((__m128i*)tab_dct_8[ 1]));
        T60  = _mm_srai_epi32(_mm_add_epi32(T60, c_4), 3);
        T61  = _mm_srai_epi32(_mm_add_epi32(T61, c_4), 3);
        T70  = _mm_packs_epi32(T60, T61);
        _mm_store_si128((__m128i*)&tmp[ 0*16+i], T70);

        T60  = _mm_madd_epi16(T50, _mm_load_si128((__m128i*)tab_dct_8[ 2]));
        T61  = _mm_madd_epi16(T51, _mm_load_si128((__m128i*)tab_dct_8[ 2]));
        T60  = _mm_srai_epi32(_mm_add_epi32(T60, c_4), 3);
        T61  = _mm_srai_epi32(_mm_add_epi32(T61, c_4), 3);
        T70  = _mm_packs_epi32(T60, T61);
        _mm_store_si128((__m128i*)&tmp[ 8*16+i], T70);

        T60  = _mm_madd_epi16(T52, _mm_load_si128((__m128i*)tab_dct_8[ 3]));
        T61  = _mm_madd_epi16(T53, _mm_load_si128((__m128i*)tab_dct_8[ 3]));
        T60  = _mm_srai_epi32(_mm_add_epi32(T60, c_4), 3);
        T61  = _mm_srai_epi32(_mm_add_epi32(T61, c_4), 3);
        T70  = _mm_packs_epi32(T60, T61);
        _mm_store_si128((__m128i*)&tmp[ 4*16+i], T70);

        T60  = _mm_madd_epi16(T52, _mm_load_si128((__m128i*)tab_dct_8[ 4]));
        T61  = _mm_madd_epi16(T53, _mm_load_si128((__m128i*)tab_dct_8[ 4]));
        T60  = _mm_srai_epi32(_mm_add_epi32(T60, c_4), 3);
        T61  = _mm_srai_epi32(_mm_add_epi32(T61, c_4), 3);
        T70  = _mm_packs_epi32(T60, T61);
        _mm_store_si128((__m128i*)&tmp[12*16+i], T70);

        T60  = _mm_madd_epi16(T44, _mm_load_si128((__m128i*)tab_dct_8[ 5]));
        T61  = _mm_madd_epi16(T45, _mm_load_si128((__m128i*)tab_dct_8[ 5]));
        T62  = _mm_madd_epi16(T46, _mm_load_si128((__m128i*)tab_dct_8[ 5]));
        T63  = _mm_madd_epi16(T47, _mm_load_si128((__m128i*)tab_dct_8[ 5]));
        T60  = _mm_hadd_epi32(T60, T61);
        T61  = _mm_hadd_epi32(T62, T63);
        T60  = _mm_srai_epi32(_mm_add_epi32(T60, c_4), 3);
        T61  = _mm_srai_epi32(_mm_add_epi32(T61, c_4), 3);
        T70  = _mm_packs_epi32(T60, T61);
        _mm_store_si128((__m128i*)&tmp[ 2*16+i], T70);

        T60  = _mm_madd_epi16(T44, _mm_load_si128((__m128i*)tab_dct_8[ 6]));
        T61  = _mm_madd_epi16(T45, _mm_load_si128((__m128i*)tab_dct_8[ 6]));
        T62  = _mm_madd_epi16(T46, _mm_load_si128((__m128i*)tab_dct_8[ 6]));
        T63  = _mm_madd_epi16(T47, _mm_load_si128((__m128i*)tab_dct_8[ 6]));
        T60  = _mm_hadd_epi32(T60, T61);
        T61  = _mm_hadd_epi32(T62, T63);
        T60  = _mm_srai_epi32(_mm_add_epi32(T60, c_4), 3);
        T61  = _mm_srai_epi32(_mm_add_epi32(T61, c_4), 3);
        T70  = _mm_packs_epi32(T60, T61);
        _mm_store_si128((__m128i*)&tmp[ 6*16+i], T70);

        T60  = _mm_madd_epi16(T44, _mm_load_si128((__m128i*)tab_dct_8[ 7]));
        T61  = _mm_madd_epi16(T45, _mm_load_si128((__m128i*)tab_dct_8[ 7]));
        T62  = _mm_madd_epi16(T46, _mm_load_si128((__m128i*)tab_dct_8[ 7]));
        T63  = _mm_madd_epi16(T47, _mm_load_si128((__m128i*)tab_dct_8[ 7]));
        T60  = _mm_hadd_epi32(T60, T61);
        T61  = _mm_hadd_epi32(T62, T63);
        T60  = _mm_srai_epi32(_mm_add_epi32(T60, c_4), 3);
        T61  = _mm_srai_epi32(_mm_add_epi32(T61, c_4), 3);
        T70  = _mm_packs_epi32(T60, T61);
        _mm_store_si128((__m128i*)&tmp[10*16+i], T70);

        T60  = _mm_madd_epi16(T44, _mm_load_si128((__m128i*)tab_dct_8[ 8]));
        T61  = _mm_madd_epi16(T45, _mm_load_si128((__m128i*)tab_dct_8[ 8]));
        T62  = _mm_madd_epi16(T46, _mm_load_si128((__m128i*)tab_dct_8[ 8]));
        T63  = _mm_madd_epi16(T47, _mm_load_si128((__m128i*)tab_dct_8[ 8]));
        T60  = _mm_hadd_epi32(T60, T61);
        T61  = _mm_hadd_epi32(T62, T63);
        T60  = _mm_srai_epi32(_mm_add_epi32(T60, c_4), 3);
        T61  = _mm_srai_epi32(_mm_add_epi32(T61, c_4), 3);
        T70  = _mm_packs_epi32(T60, T61);
        _mm_store_si128((__m128i*)&tmp[14*16+i], T70);

#define MAKE_ODD(tab, dst) \
        T60  = _mm_madd_epi16(T20, _mm_load_si128((__m128i*)tab_dct_16_1[(tab)])); \
        T61  = _mm_madd_epi16(T21, _mm_load_si128((__m128i*)tab_dct_16_1[(tab)])); \
        T62  = _mm_madd_epi16(T22, _mm_load_si128((__m128i*)tab_dct_16_1[(tab)])); \
        T63  = _mm_madd_epi16(T23, _mm_load_si128((__m128i*)tab_dct_16_1[(tab)])); \
        T64  = _mm_madd_epi16(T24, _mm_load_si128((__m128i*)tab_dct_16_1[(tab)])); \
        T65  = _mm_madd_epi16(T25, _mm_load_si128((__m128i*)tab_dct_16_1[(tab)])); \
        T66  = _mm_madd_epi16(T26, _mm_load_si128((__m128i*)tab_dct_16_1[(tab)])); \
        T67  = _mm_madd_epi16(T27, _mm_load_si128((__m128i*)tab_dct_16_1[(tab)])); \
        T60  = _mm_hadd_epi32(T60, T61); \
        T61  = _mm_hadd_epi32(T62, T63); \
        T62  = _mm_hadd_epi32(T64, T65); \
        T63  = _mm_hadd_epi32(T66, T67); \
        T60  = _mm_hadd_epi32(T60, T61); \
        T61  = _mm_hadd_epi32(T62, T63); \
        T60  = _mm_srai_epi32(_mm_add_epi32(T60, c_4), 3); \
        T61  = _mm_srai_epi32(_mm_add_epi32(T61, c_4), 3); \
        T70  = _mm_packs_epi32(T60, T61); \
        _mm_store_si128((__m128i*)&tmp[(dst)*16+i], T70);

        MAKE_ODD( 0, 1);
        MAKE_ODD( 1, 3);
        MAKE_ODD( 2, 5);
        MAKE_ODD( 3, 7);
        MAKE_ODD( 4, 9);
        MAKE_ODD( 5,11);
        MAKE_ODD( 6,13);
        MAKE_ODD( 7,15);
#undef MAKE_ODD
    }

    // DCT2
    for( i=0; i<16; i+=4 )
    {
        T00A = _mm_load_si128((__m128i*)&tmp[(i+0)*16+0]);    // [07 06 05 04 03 02 01 00]
        T00B = _mm_load_si128((__m128i*)&tmp[(i+0)*16+8]);    // [0F 0E 0D 0C 0B 0A 09 08]
        T01A = _mm_load_si128((__m128i*)&tmp[(i+1)*16+0]);    // [17 16 15 14 13 12 11 10]
        T01B = _mm_load_si128((__m128i*)&tmp[(i+1)*16+8]);    // [1F 1E 1D 1C 1B 1A 19 18]
        T02A = _mm_load_si128((__m128i*)&tmp[(i+2)*16+0]);    // [27 26 25 24 23 22 21 20]
        T02B = _mm_load_si128((__m128i*)&tmp[(i+2)*16+8]);    // [2F 2E 2D 2C 2B 2A 29 28]
        T03A = _mm_load_si128((__m128i*)&tmp[(i+3)*16+0]);    // [37 36 35 34 33 32 31 30]
        T03B = _mm_load_si128((__m128i*)&tmp[(i+3)*16+8]);    // [3F 3E 3D 3C 3B 3A 39 38]

        T00A = _mm_shuffle_epi8(T00A, _mm_load_si128((__m128i*)tab_dct_16_0[2]));
        T00B = _mm_shuffle_epi8(T00B, _mm_load_si128((__m128i*)tab_dct_16_0[3]));
        T01A = _mm_shuffle_epi8(T01A, _mm_load_si128((__m128i*)tab_dct_16_0[2]));
        T01B = _mm_shuffle_epi8(T01B, _mm_load_si128((__m128i*)tab_dct_16_0[3]));
        T02A = _mm_shuffle_epi8(T02A, _mm_load_si128((__m128i*)tab_dct_16_0[2]));
        T02B = _mm_shuffle_epi8(T02B, _mm_load_si128((__m128i*)tab_dct_16_0[3]));
        T03A = _mm_shuffle_epi8(T03A, _mm_load_si128((__m128i*)tab_dct_16_0[2]));
        T03B = _mm_shuffle_epi8(T03B, _mm_load_si128((__m128i*)tab_dct_16_0[3]));

        T10  = _mm_unpacklo_epi16(T00A, T00B);
        T11  = _mm_unpackhi_epi16(T00A, T00B);
        T12  = _mm_unpacklo_epi16(T01A, T01B);
        T13  = _mm_unpackhi_epi16(T01A, T01B);
        T14  = _mm_unpacklo_epi16(T02A, T02B);
        T15  = _mm_unpackhi_epi16(T02A, T02B);
        T16  = _mm_unpacklo_epi16(T03A, T03B);
        T17  = _mm_unpackhi_epi16(T03A, T03B);

        T20  = _mm_madd_epi16(T10, _mm_load_si128((__m128i*)tab_dct_8[1]));
        T21  = _mm_madd_epi16(T11, _mm_load_si128((__m128i*)tab_dct_8[1]));
        T22  = _mm_madd_epi16(T12, _mm_load_si128((__m128i*)tab_dct_8[1]));
        T23  = _mm_madd_epi16(T13, _mm_load_si128((__m128i*)tab_dct_8[1]));
        T24  = _mm_madd_epi16(T14, _mm_load_si128((__m128i*)tab_dct_8[1]));
        T25  = _mm_madd_epi16(T15, _mm_load_si128((__m128i*)tab_dct_8[1]));
        T26  = _mm_madd_epi16(T16, _mm_load_si128((__m128i*)tab_dct_8[1]));
        T27  = _mm_madd_epi16(T17, _mm_load_si128((__m128i*)tab_dct_8[1]));

        T30  = _mm_add_epi32(T20, T21);
        T31  = _mm_add_epi32(T22, T23);
        T32  = _mm_add_epi32(T24, T25);
        T33  = _mm_add_epi32(T26, T27);

        T30  = _mm_hadd_epi32(T30, T31);
        T31  = _mm_hadd_epi32(T32, T33);

        T40  = _mm_hadd_epi32(T30, T31);
        T41  = _mm_hsub_epi32(T30, T31);
        T40  = _mm_srai_epi32(_mm_add_epi32(T40, c_512), 10);
        T41  = _mm_srai_epi32(_mm_add_epi32(T41, c_512), 10);
        _mm_storeu_si128((__m128i*)&pDst[ 0*16+i], T40);
        _mm_storeu_si128((__m128i*)&pDst[ 8*16+i], T41);

        T20  = _mm_madd_epi16(T10, _mm_load_si128((__m128i*)tab_dct_16_1[ 8]));
        T21  = _mm_madd_epi16(T11, _mm_load_si128((__m128i*)tab_dct_16_1[ 8]));
        T22  = _mm_madd_epi16(T12, _mm_load_si128((__m128i*)tab_dct_16_1[ 8]));
        T23  = _mm_madd_epi16(T13, _mm_load_si128((__m128i*)tab_dct_16_1[ 8]));
        T24  = _mm_madd_epi16(T14, _mm_load_si128((__m128i*)tab_dct_16_1[ 8]));
        T25  = _mm_madd_epi16(T15, _mm_load_si128((__m128i*)tab_dct_16_1[ 8]));
        T26  = _mm_madd_epi16(T16, _mm_load_si128((__m128i*)tab_dct_16_1[ 8]));
        T27  = _mm_madd_epi16(T17, _mm_load_si128((__m128i*)tab_dct_16_1[ 8]));

        T30  = _mm_add_epi32(T20, T21);
        T31  = _mm_add_epi32(T22, T23);
        T32  = _mm_add_epi32(T24, T25);
        T33  = _mm_add_epi32(T26, T27);

        T30  = _mm_hadd_epi32(T30, T31);
        T31  = _mm_hadd_epi32(T32, T33);

        T40  = _mm_hadd_epi32(T30, T31);
        T40  = _mm_srai_epi32(_mm_add_epi32(T40, c_512), 10);
        _mm_storeu_si128((__m128i*)&pDst[ 4*16+i], T40);

        T20  = _mm_madd_epi16(T10, _mm_load_si128((__m128i*)tab_dct_16_1[ 9]));
        T21  = _mm_madd_epi16(T11, _mm_load_si128((__m128i*)tab_dct_16_1[ 9]));
        T22  = _mm_madd_epi16(T12, _mm_load_si128((__m128i*)tab_dct_16_1[ 9]));
        T23  = _mm_madd_epi16(T13, _mm_load_si128((__m128i*)tab_dct_16_1[ 9]));
        T24  = _mm_madd_epi16(T14, _mm_load_si128((__m128i*)tab_dct_16_1[ 9]));
        T25  = _mm_madd_epi16(T15, _mm_load_si128((__m128i*)tab_dct_16_1[ 9]));
        T26  = _mm_madd_epi16(T16, _mm_load_si128((__m128i*)tab_dct_16_1[ 9]));
        T27  = _mm_madd_epi16(T17, _mm_load_si128((__m128i*)tab_dct_16_1[ 9]));

        T30  = _mm_add_epi32(T20, T21);
        T31  = _mm_add_epi32(T22, T23);
        T32  = _mm_add_epi32(T24, T25);
        T33  = _mm_add_epi32(T26, T27);

        T30  = _mm_hadd_epi32(T30, T31);
        T31  = _mm_hadd_epi32(T32, T33);

        T40  = _mm_hadd_epi32(T30, T31);
        T40  = _mm_srai_epi32(_mm_add_epi32(T40, c_512), 10);
        _mm_storeu_si128((__m128i*)&pDst[12*16+i], T40);

        T20  = _mm_madd_epi16(T10, _mm_load_si128((__m128i*)tab_dct_16_1[10]));
        T21  = _mm_madd_epi16(T11, _mm_load_si128((__m128i*)tab_dct_16_1[10]));
        T22  = _mm_madd_epi16(T12, _mm_load_si128((__m128i*)tab_dct_16_1[10]));
        T23  = _mm_madd_epi16(T13, _mm_load_si128((__m128i*)tab_dct_16_1[10]));
        T24  = _mm_madd_epi16(T14, _mm_load_si128((__m128i*)tab_dct_16_1[10]));
        T25  = _mm_madd_epi16(T15, _mm_load_si128((__m128i*)tab_dct_16_1[10]));
        T26  = _mm_madd_epi16(T16, _mm_load_si128((__m128i*)tab_dct_16_1[10]));
        T27  = _mm_madd_epi16(T17, _mm_load_si128((__m128i*)tab_dct_16_1[10]));

        T30  = _mm_sub_epi32(T20, T21);
        T31  = _mm_sub_epi32(T22, T23);
        T32  = _mm_sub_epi32(T24, T25);
        T33  = _mm_sub_epi32(T26, T27);

        T30  = _mm_hadd_epi32(T30, T31);
        T31  = _mm_hadd_epi32(T32, T33);

        T40  = _mm_hadd_epi32(T30, T31);
        T40  = _mm_srai_epi32(_mm_add_epi32(T40, c_512), 10);
        _mm_storeu_si128((__m128i*)&pDst[ 2*16+i], T40);

        T20  = _mm_madd_epi16(T10, _mm_load_si128((__m128i*)tab_dct_16_1[11]));
        T21  = _mm_madd_epi16(T11, _mm_load_si128((__m128i*)tab_dct_16_1[11]));
        T22  = _mm_madd_epi16(T12, _mm_load_si128((__m128i*)tab_dct_16_1[11]));
        T23  = _mm_madd_epi16(T13, _mm_load_si128((__m128i*)tab_dct_16_1[11]));
        T24  = _mm_madd_epi16(T14, _mm_load_si128((__m128i*)tab_dct_16_1[11]));
        T25  = _mm_madd_epi16(T15, _mm_load_si128((__m128i*)tab_dct_16_1[11]));
        T26  = _mm_madd_epi16(T16, _mm_load_si128((__m128i*)tab_dct_16_1[11]));
        T27  = _mm_madd_epi16(T17, _mm_load_si128((__m128i*)tab_dct_16_1[11]));

        T30  = _mm_sub_epi32(T20, T21); 
        T31  = _mm_sub_epi32(T22, T23);
        T32  = _mm_sub_epi32(T24, T25);
        T33  = _mm_sub_epi32(T26, T27);

        T30  = _mm_hadd_epi32(T30, T31);
        T31  = _mm_hadd_epi32(T32, T33);

        T40  = _mm_hadd_epi32(T30, T31);
        T40  = _mm_srai_epi32(_mm_add_epi32(T40, c_512), 10);
        _mm_storeu_si128((__m128i*)&pDst[ 6*16+i], T40);

        T20  = _mm_madd_epi16(T10, _mm_load_si128((__m128i*)tab_dct_16_1[12]));
        T21  = _mm_madd_epi16(T11, _mm_load_si128((__m128i*)tab_dct_16_1[12]));
        T22  = _mm_madd_epi16(T12, _mm_load_si128((__m128i*)tab_dct_16_1[12]));
        T23  = _mm_madd_epi16(T13, _mm_load_si128((__m128i*)tab_dct_16_1[12]));
        T24  = _mm_madd_epi16(T14, _mm_load_si128((__m128i*)tab_dct_16_1[12]));
        T25  = _mm_madd_epi16(T15, _mm_load_si128((__m128i*)tab_dct_16_1[12]));
        T26  = _mm_madd_epi16(T16, _mm_load_si128((__m128i*)tab_dct_16_1[12]));
        T27  = _mm_madd_epi16(T17, _mm_load_si128((__m128i*)tab_dct_16_1[12]));

        T30  = _mm_sub_epi32(T20, T21); 
        T31  = _mm_sub_epi32(T22, T23);
        T32  = _mm_sub_epi32(T24, T25);
        T33  = _mm_sub_epi32(T26, T27);

        T30  = _mm_hadd_epi32(T30, T31);
        T31  = _mm_hadd_epi32(T32, T33);

        T40  = _mm_hadd_epi32(T30, T31);
        T40  = _mm_srai_epi32(_mm_add_epi32(T40, c_512), 10);
        _mm_storeu_si128((__m128i*)&pDst[10*16+i], T40);

        T20  = _mm_madd_epi16(T10, _mm_load_si128((__m128i*)tab_dct_16_1[13]));
        T21  = _mm_madd_epi16(T11, _mm_load_si128((__m128i*)tab_dct_16_1[13]));
        T22  = _mm_madd_epi16(T12, _mm_load_si128((__m128i*)tab_dct_16_1[13]));
        T23  = _mm_madd_epi16(T13, _mm_load_si128((__m128i*)tab_dct_16_1[13]));
        T24  = _mm_madd_epi16(T14, _mm_load_si128((__m128i*)tab_dct_16_1[13]));
        T25  = _mm_madd_epi16(T15, _mm_load_si128((__m128i*)tab_dct_16_1[13]));
        T26  = _mm_madd_epi16(T16, _mm_load_si128((__m128i*)tab_dct_16_1[13]));
        T27  = _mm_madd_epi16(T17, _mm_load_si128((__m128i*)tab_dct_16_1[13]));

        T30  = _mm_sub_epi32(T20, T21); 
        T31  = _mm_sub_epi32(T22, T23);
        T32  = _mm_sub_epi32(T24, T25);
        T33  = _mm_sub_epi32(T26, T27);

        T30  = _mm_hadd_epi32(T30, T31);
        T31  = _mm_hadd_epi32(T32, T33);

        T40  = _mm_hadd_epi32(T30, T31);
        T40  = _mm_srai_epi32(_mm_add_epi32(T40, c_512), 10);
        _mm_storeu_si128((__m128i*)&pDst[14*16+i], T40);


#define MAKE_ODD(tab, dst) \
        T20  = _mm_madd_epi16(T10, _mm_load_si128((__m128i*)tab_dct_16_1[(tab)  ])); /* [*O2_0 *O1_0 *O3_0 *O0_0] */ \
        T21  = _mm_madd_epi16(T11, _mm_load_si128((__m128i*)tab_dct_16_1[(tab)+1])); /* [*O5_0 *O6_0 *O4_0 *O7_0] */ \
        T22  = _mm_madd_epi16(T12, _mm_load_si128((__m128i*)tab_dct_16_1[(tab)  ])); \
        T23  = _mm_madd_epi16(T13, _mm_load_si128((__m128i*)tab_dct_16_1[(tab)+1])); \
        T24  = _mm_madd_epi16(T14, _mm_load_si128((__m128i*)tab_dct_16_1[(tab)  ])); \
        T25  = _mm_madd_epi16(T15, _mm_load_si128((__m128i*)tab_dct_16_1[(tab)+1])); \
        T26  = _mm_madd_epi16(T16, _mm_load_si128((__m128i*)tab_dct_16_1[(tab)  ])); \
        T27  = _mm_madd_epi16(T17, _mm_load_si128((__m128i*)tab_dct_16_1[(tab)+1])); \
        \
        T30  = _mm_add_epi32(T20, T21); \
        T31  = _mm_add_epi32(T22, T23); \
        T32  = _mm_add_epi32(T24, T25); \
        T33  = _mm_add_epi32(T26, T27); \
        \
        T30  = _mm_hadd_epi32(T30, T31); \
        T31  = _mm_hadd_epi32(T32, T33); \
        \
        T40  = _mm_hadd_epi32(T30, T31); \
        T40  = _mm_srai_epi32(_mm_add_epi32(T40, c_512), 10); \
        _mm_storeu_si128((__m128i*)&pDst[(dst)*16+i], T40);

        MAKE_ODD(14,  1);
        MAKE_ODD(16,  3);
        MAKE_ODD(18,  5);
        MAKE_ODD(20,  7);
        MAKE_ODD(22,  9);
        MAKE_ODD(24, 11);
        MAKE_ODD(26, 13);
        MAKE_ODD(28, 15);
#undef MAKE_ODD
    }
}
#endif // INSTRSET >= 4

#if INSTRSET < 4
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

void xDCT32(short *pSrc, int *pDst, intptr_t nStride)
{
    const int shift_1st = 4;
    const int shift_2nd = 11;
    ALIGN_VAR_32(Short, tmp [32 * 32]);
    ALIGN_VAR_32(Short, tmp1[32 * 32]);

    for(int i=0; i<32; i++)
    {
        memcpy(&tmp1[i*32], &pSrc[i*nStride], 32*sizeof(short));
    }

    partialButterfly32(tmp1, tmp, shift_1st, 32);
    partialButterfly32(tmp, tmp1, shift_2nd, 32);

#define N (32)
    for(int i=0; i<N; i++)
    {
        for(int j=0; j<N; j++)
        {
            pDst[i*N+j] = tmp1[i*N+j];
        }
    }
#undef N
}

#else // INSTRSET >= 4

ALIGN_VAR_32(static const short, tab_dct_32_0[][8])=
{
    { 0x0F0E, 0x0100, 0x0908, 0x0706, 0x0D0C, 0x0302, 0x0B0A, 0x0504},  // 0
};

ALIGN_VAR_32(static const short, tab_dct_32_1[][8])=
{
    { 89,-89, 18,-18, 75,-75, 50,-50},          //  0
    { 75,-75,-50, 50,-18, 18,-89, 89},          //  1
    { 50,-50, 75,-75,-89, 89, 18,-18},          //  2
    { 18,-18,-89, 89,-50, 50, 75,-75},          //  3

#define MAKE_COEF8(a0, a1, a2, a3, a4, a5, a6, a7) \
    { (a0), (a7), (a3), (a4), (a1), (a6), (a2), (a5)}, \

    MAKE_COEF8(90, 87, 80, 70, 57, 43, 25,  9)   //  4
    MAKE_COEF8(87, 57,  9,-43,-80,-90,-70,-25)   //  5
    MAKE_COEF8(80,  9,-70,-87,-25, 57, 90, 43)   //  6
    MAKE_COEF8(70,-43,-87,  9, 90, 25,-80,-57)   //  7
    MAKE_COEF8(57,-80,-25, 90, -9,-87, 43, 70)   //  8
    MAKE_COEF8(43,-90, 57, 25,-87, 70,  9,-80)   //  9
    MAKE_COEF8(25,-70, 90,-80, 43,  9,-57, 87)   // 10
    MAKE_COEF8( 9,-25, 43,-57, 70,-80, 87,-90)   // 11
#undef MAKE_COEF8

#define MAKE_COEF16(a00, a01, a02, a03, a04, a05, a06, a07, a08, a09, a10, a11, a12, a13, a14, a15) \
    { (a00), (a07), (a03), (a04), (a01), (a06), (a02), (a05)}, \
    { (a15), (a08), (a12), (a11), (a14), (a09), (a13), (a10)},

    MAKE_COEF16( 90, 90, 88, 85, 82, 78, 73, 67, 61, 54, 46, 38, 31, 22, 13,  4)    // 12
    MAKE_COEF16( 90, 82, 67, 46, 22, -4,-31,-54,-73,-85,-90,-88,-78,-61,-38,-13)    // 14
    MAKE_COEF16( 88, 67, 31,-13,-54,-82,-90,-78,-46, -4, 38, 73, 90, 85, 61, 22)    // 16
    MAKE_COEF16( 85, 46,-13,-67,-90,-73,-22, 38, 82, 88, 54, -4,-61,-90,-78,-31)    // 18
    MAKE_COEF16( 82, 22,-54,-90,-61, 13, 78, 85, 31,-46,-90,-67,  4, 73, 88, 38)    // 20
    MAKE_COEF16( 78, -4,-82,-73, 13, 85, 67,-22,-88,-61, 31, 90, 54,-38,-90,-46)    // 22
    MAKE_COEF16( 73,-31,-90,-22, 78, 67,-38,-90,-13, 82, 61,-46,-88, -4, 85, 54)    // 24
    MAKE_COEF16( 67,-54,-78, 38, 85,-22,-90,  4, 90, 13,-88,-31, 82, 46,-73,-61)    // 26
    MAKE_COEF16( 61,-73,-46, 82, 31,-88,-13, 90, -4,-90, 22, 85,-38,-78, 54, 67)    // 28
    MAKE_COEF16( 54,-85, -4, 88,-46,-61, 82, 13,-90, 38, 67,-78,-22, 90,-31,-73)    // 30
    MAKE_COEF16( 46,-90, 38, 54,-90, 31, 61,-88, 22, 67,-85, 13, 73,-82,  4, 78)    // 32
    MAKE_COEF16( 38,-88, 73, -4,-67, 90,-46,-31, 85,-78, 13, 61,-90, 54, 22,-82)    // 34
    MAKE_COEF16( 31,-78, 90,-61,  4, 54,-88, 82,-38,-22, 73,-90, 67,-13,-46, 85)    // 36
    MAKE_COEF16( 22,-61, 85,-90, 73,-38, -4, 46,-78, 90,-82, 54,-13,-31, 67,-88)    // 38
    MAKE_COEF16( 13,-38, 61,-78, 88,-90, 85,-73, 54,-31,  4, 22,-46, 67,-82, 90)    // 40
    MAKE_COEF16(  4,-13, 22,-31, 38,-46, 54,-61, 67,-73, 78,-82, 85,-88, 90,-90)    // 42
#undef MAKE_COEF16

    { 64, 64, 64, 64, 64, 64, 64, 64},  // 44

    { 64, 64,-64,-64,-64,-64, 64, 64},  // 45

    { 83, 83, 36, 36,-36,-36,-83,-83},  // 46
    {-83,-83,-36,-36, 36, 36, 83, 83},  // 47

    { 36, 36,-83,-83, 83, 83,-36,-36},  // 48
    {-36,-36, 83, 83,-83,-83, 36, 36},  // 49

#define MAKE_COEF16(a00, a01, a02, a03, a04, a05, a06, a07, a08, a09, a10, a11, a12, a13, a14, a15) \
    { (a00), (a00), (a01), (a01), (a02), (a02), (a03), (a03)},\
    { (a04), (a04), (a05), (a05), (a06), (a06), (a07), (a07)},\
    { (a08), (a08), (a09), (a09), (a10), (a10), (a11), (a11)},\
    { (a12), (a12), (a13), (a13), (a14), (a14), (a15), (a15)},

    MAKE_COEF16( 89, 75, 50, 18,-18,-50,-75,-89,-89,-75,-50,-18, 18, 50, 75, 89) // 50
    MAKE_COEF16( 75,-18,-89,-50, 50, 89, 18,-75,-75, 18, 89, 50,-50,-89,-18, 75) // 54

    // TODO: convert below table here
#undef MAKE_COEF16

    { 50, 50,-89,-89, 18, 18, 75, 75},  // 58
    {-75,-75,-18,-18, 89, 89,-50,-50},  // 59
    {-50,-50, 89, 89,-18,-18,-75,-75},  // 60
    { 75, 75, 18, 18,-89,-89, 50, 50},  // 61

    { 18, 18,-50,-50, 75, 75,-89,-89},  // 62
    { 89, 89,-75,-75, 50, 50,-18,-18},  // 63
    {-18,-18, 50, 50,-75,-75, 89, 89},  // 64
    {-89,-89, 75, 75,-50,-50, 18, 18},  // 65

    { 90, 90, 87, 87, 80, 80, 70, 70},  // 66
    { 57, 57, 43, 43, 25, 25,  9,  9},  // 67
    {- 9,- 9,-25,-25,-43,-43,-57,-57},  // 68
    {-70,-70,-80,-80,-87,-87,-90,-90},  // 69

    { 87, 87, 57, 57,  9,  9,-43,-43},  // 70
    {-80,-80,-90,-90,-70,-70,-25,-25},  // 71
    { 25, 25, 70, 70, 90, 90, 80, 80},  // 72
    { 43, 43,- 9,- 9,-57,-57,-87,-87},  // 73

    { 80, 80,  9,  9,-70,-70,-87,-87},  // 74
    {-25,-25, 57, 57, 90, 90, 43, 43},  // 75
    {-43,-43,-90,-90,-57,-57, 25, 25},  // 76
    { 87, 87, 70, 70,- 9,- 9,-80,-80},  // 77

    { 70, 70,-43,-43,-87,-87,  9,  9},  // 78
    { 90, 90, 25, 25,-80,-80,-57,-57},  // 79
    { 57, 57, 80, 80,-25,-25,-90,-90},  // 80
    {- 9,- 9, 87, 87, 43, 43,-70,-70},  // 81

    { 57, 57,-80,-80,-25,-25, 90, 90},  // 82
    {- 9,- 9,-87,-87, 43, 43, 70, 70},  // 83
    {-70,-70,-43,-43, 87, 87,  9,  9},  // 84
    {-90,-90, 25, 25, 80, 80,-57,-57},  // 85

    { 43, 43,-90,-90, 57, 57, 25, 25},  // 86
    {-87,-87, 70, 70,  9,  9,-80,-80},  // 87
    { 80, 80,- 9,- 9,-70,-70, 87, 87},  // 88
    {-25,-25,-57,-57, 90, 90,-43,-43},  // 89

    { 25, 25,-70,-70, 90, 90,-80,-80},  // 90
    { 43, 43,  9,  9,-57,-57, 87, 87},  // 91
    {-87,-87, 57, 57,- 9,- 9,-43,-43},  // 92
    { 80, 80,-90,-90, 70, 70,-25,-25},  // 93

    {  9,  9,-25,-25, 43, 43,-57,-57},  // 94
    { 70, 70,-80,-80, 87, 87,-90,-90},  // 95
    { 90, 90,-87,-87, 80, 80,-70,-70},  // 96
    { 57, 57,-43,-43, 25, 25,- 9,- 9},  // 97

#define MAKE_COEF16(a00, a01, a02, a03, a04, a05, a06, a07, a08, a09, a10, a11, a12, a13, a14, a15) \
    { (a00),-(a00), (a01),-(a01), (a02),-(a02), (a03),-(a03)},\
    { (a04),-(a04), (a05),-(a05), (a06),-(a06), (a07),-(a07)},\
    { (a08),-(a08), (a09),-(a09), (a10),-(a10), (a11),-(a11)},\
    { (a12),-(a12), (a13),-(a13), (a14),-(a14), (a15),-(a15)},

    MAKE_COEF16( 90, 90, 88, 85, 82, 78, 73, 67, 61, 54, 46, 38, 31, 22, 13, 4)    // 98
    MAKE_COEF16(90, 82, 67, 46, 22,- 4,-31,-54,-73,-85,-90,-88,-78,-61,-38,-13)     //102
    MAKE_COEF16(88, 67, 31,-13,-54,-82,-90,-78,-46,- 4, 38, 73, 90, 85, 61, 22)     //106
    MAKE_COEF16(85, 46,-13,-67,-90,-73,-22, 38,+82, 88, 54,- 4,-61,-90,-78,-31)     //110
    MAKE_COEF16(82, 22,-54,-90,-61, 13, 78, 85,+31,-46,-90,-67,  4, 73, 88, 38)     //114
    MAKE_COEF16(78,- 4,-82,-73, 13, 85, 67,-22,-88,-61, 31, 90, 54,-38,-90,-46)     //118
    MAKE_COEF16(73,-31,-90,-22, 78, 67,-38,-90,-13, 82, 61,-46,-88,- 4, 85, 54)     //122
    MAKE_COEF16(67,-54,-78, 38, 85,-22,-90,  4,+90, 13,-88,-31, 82, 46,-73,-61)     //126
    MAKE_COEF16(61,-73,-46, 82, 31,-88,-13, 90, -4,-90, 22, 85,-38,-78, 54, 67)     //130
    MAKE_COEF16(54,-85,- 4, 88,-46,-61, 82, 13,-90, 38, 67,-78,-22, 90,-31,-73)     //134
    MAKE_COEF16(46,-90, 38, 54,-90, 31, 61,-88,+22, 67,-85, 13, 73,-82,  4, 78)     //138
    MAKE_COEF16(38,-88, 73,- 4,-67, 90,-46,-31,+85,-78, 13, 61,-90, 54, 22,-82)     //142
    MAKE_COEF16(31,-78, 90,-61,  4, 54,-88, 82,-38,-22, 73,-90, 67,-13,-46, 85)     //146
    MAKE_COEF16(22,-61, 85,-90, 73,-38,- 4, 46,-78, 90,-82, 54,-13,-31, 67,-88)     //150
    MAKE_COEF16(13,-38, 61,-78, 88,-90, 85,-73,+54,-31,  4, 22,-46, 67,-82, 90)     //154
    MAKE_COEF16( 4,-13, 22,-31, 38,-46, 54,-61,+67,-73, 78,-82, 85,-88, 90,-90)     //158

#undef MAKE_COEF16
};

void xDCT32(short *pSrc, int *pDst, intptr_t nStride)
{
    // Const
    __m128i c_8     = _mm_set1_epi32(8);
    __m128i c_1024  = _mm_set1_epi32(1024);

    int i;

    __m128i T00A, T01A, T02A, T03A, T04A, T05A, T06A, T07A;
    __m128i T00B, T01B, T02B, T03B, T04B, T05B, T06B, T07B;
    __m128i T00C, T01C, T02C, T03C, T04C, T05C, T06C, T07C;
    __m128i T00D, T01D, T02D, T03D, T04D, T05D, T06D, T07D;
    __m128i T10A, T11A, T12A, T13A, T14A, T15A, T16A, T17A;
    __m128i T10B, T11B, T12B, T13B, T14B, T15B, T16B, T17B;
    __m128i T20, T21, T22, T23, T24, T25, T26, T27;
    __m128i T30, T31, T32, T33, T34, T35, T36, T37;
    __m128i T40, T41, T42, T43, T44, T45, T46, T47;
    __m128i T50, T51, T52, T53;
    __m128i T60, T61, T62, T63, T64, T65, T66, T67;
    __m128i im[32][4];

    // DCT1
    for( i=0; i<32/8; i++ )
    {
        T00A = _mm_load_si128((__m128i*)&pSrc[(i*8+0)*nStride+ 0]);    // [07 06 05 04 03 02 01 00]
        T00B = _mm_load_si128((__m128i*)&pSrc[(i*8+0)*nStride+ 8]);    // [15 14 13 12 11 10 09 08]
        T00C = _mm_load_si128((__m128i*)&pSrc[(i*8+0)*nStride+16]);    // [23 22 21 20 19 18 17 16]
        T00D = _mm_load_si128((__m128i*)&pSrc[(i*8+0)*nStride+24]);    // [31 30 29 28 27 26 25 24]
        T01A = _mm_load_si128((__m128i*)&pSrc[(i*8+1)*nStride+ 0]);
        T01B = _mm_load_si128((__m128i*)&pSrc[(i*8+1)*nStride+ 8]);
        T01C = _mm_load_si128((__m128i*)&pSrc[(i*8+1)*nStride+16]);
        T01D = _mm_load_si128((__m128i*)&pSrc[(i*8+1)*nStride+24]);
        T02A = _mm_load_si128((__m128i*)&pSrc[(i*8+2)*nStride+ 0]);
        T02B = _mm_load_si128((__m128i*)&pSrc[(i*8+2)*nStride+ 8]);
        T02C = _mm_load_si128((__m128i*)&pSrc[(i*8+2)*nStride+16]);
        T02D = _mm_load_si128((__m128i*)&pSrc[(i*8+2)*nStride+24]);
        T03A = _mm_load_si128((__m128i*)&pSrc[(i*8+3)*nStride+ 0]);
        T03B = _mm_load_si128((__m128i*)&pSrc[(i*8+3)*nStride+ 8]);
        T03C = _mm_load_si128((__m128i*)&pSrc[(i*8+3)*nStride+16]);
        T03D = _mm_load_si128((__m128i*)&pSrc[(i*8+3)*nStride+24]);
        T04A = _mm_load_si128((__m128i*)&pSrc[(i*8+4)*nStride+ 0]);
        T04B = _mm_load_si128((__m128i*)&pSrc[(i*8+4)*nStride+ 8]);
        T04C = _mm_load_si128((__m128i*)&pSrc[(i*8+4)*nStride+16]);
        T04D = _mm_load_si128((__m128i*)&pSrc[(i*8+4)*nStride+24]);
        T05A = _mm_load_si128((__m128i*)&pSrc[(i*8+5)*nStride+ 0]);
        T05B = _mm_load_si128((__m128i*)&pSrc[(i*8+5)*nStride+ 8]);
        T05C = _mm_load_si128((__m128i*)&pSrc[(i*8+5)*nStride+16]);
        T05D = _mm_load_si128((__m128i*)&pSrc[(i*8+5)*nStride+24]);
        T06A = _mm_load_si128((__m128i*)&pSrc[(i*8+6)*nStride+ 0]);
        T06B = _mm_load_si128((__m128i*)&pSrc[(i*8+6)*nStride+ 8]);
        T06C = _mm_load_si128((__m128i*)&pSrc[(i*8+6)*nStride+16]);
        T06D = _mm_load_si128((__m128i*)&pSrc[(i*8+6)*nStride+24]);
        T07A = _mm_load_si128((__m128i*)&pSrc[(i*8+7)*nStride+ 0]);
        T07B = _mm_load_si128((__m128i*)&pSrc[(i*8+7)*nStride+ 8]);
        T07C = _mm_load_si128((__m128i*)&pSrc[(i*8+7)*nStride+16]);
        T07D = _mm_load_si128((__m128i*)&pSrc[(i*8+7)*nStride+24]);

        T00A = _mm_shuffle_epi8(T00A, _mm_load_si128((__m128i*)tab_dct_16_0[1]));    // [05 02 06 01 04 03 07 00]
        T00B = _mm_shuffle_epi8(T00B, _mm_load_si128((__m128i*)tab_dct_32_0[0]));    // [10 13 09 14 11 12 08 15]
        T00C = _mm_shuffle_epi8(T00C, _mm_load_si128((__m128i*)tab_dct_16_0[1]));    // [21 18 22 17 20 19 23 16]
        T00D = _mm_shuffle_epi8(T00D, _mm_load_si128((__m128i*)tab_dct_32_0[0]));    // [26 29 25 30 27 28 24 31]
        T01A = _mm_shuffle_epi8(T01A, _mm_load_si128((__m128i*)tab_dct_16_0[1]));
        T01B = _mm_shuffle_epi8(T01B, _mm_load_si128((__m128i*)tab_dct_32_0[0]));
        T01C = _mm_shuffle_epi8(T01C, _mm_load_si128((__m128i*)tab_dct_16_0[1]));
        T01D = _mm_shuffle_epi8(T01D, _mm_load_si128((__m128i*)tab_dct_32_0[0]));
        T02A = _mm_shuffle_epi8(T02A, _mm_load_si128((__m128i*)tab_dct_16_0[1]));
        T02B = _mm_shuffle_epi8(T02B, _mm_load_si128((__m128i*)tab_dct_32_0[0]));
        T02C = _mm_shuffle_epi8(T02C, _mm_load_si128((__m128i*)tab_dct_16_0[1]));
        T02D = _mm_shuffle_epi8(T02D, _mm_load_si128((__m128i*)tab_dct_32_0[0]));
        T03A = _mm_shuffle_epi8(T03A, _mm_load_si128((__m128i*)tab_dct_16_0[1]));
        T03B = _mm_shuffle_epi8(T03B, _mm_load_si128((__m128i*)tab_dct_32_0[0]));
        T03C = _mm_shuffle_epi8(T03C, _mm_load_si128((__m128i*)tab_dct_16_0[1]));
        T03D = _mm_shuffle_epi8(T03D, _mm_load_si128((__m128i*)tab_dct_32_0[0]));
        T04A = _mm_shuffle_epi8(T04A, _mm_load_si128((__m128i*)tab_dct_16_0[1]));
        T04B = _mm_shuffle_epi8(T04B, _mm_load_si128((__m128i*)tab_dct_32_0[0]));
        T04C = _mm_shuffle_epi8(T04C, _mm_load_si128((__m128i*)tab_dct_16_0[1]));
        T04D = _mm_shuffle_epi8(T04D, _mm_load_si128((__m128i*)tab_dct_32_0[0]));
        T05A = _mm_shuffle_epi8(T05A, _mm_load_si128((__m128i*)tab_dct_16_0[1]));
        T05B = _mm_shuffle_epi8(T05B, _mm_load_si128((__m128i*)tab_dct_32_0[0]));
        T05C = _mm_shuffle_epi8(T05C, _mm_load_si128((__m128i*)tab_dct_16_0[1]));
        T05D = _mm_shuffle_epi8(T05D, _mm_load_si128((__m128i*)tab_dct_32_0[0]));
        T06A = _mm_shuffle_epi8(T06A, _mm_load_si128((__m128i*)tab_dct_16_0[1]));
        T06B = _mm_shuffle_epi8(T06B, _mm_load_si128((__m128i*)tab_dct_32_0[0]));
        T06C = _mm_shuffle_epi8(T06C, _mm_load_si128((__m128i*)tab_dct_16_0[1]));
        T06D = _mm_shuffle_epi8(T06D, _mm_load_si128((__m128i*)tab_dct_32_0[0]));
        T07A = _mm_shuffle_epi8(T07A, _mm_load_si128((__m128i*)tab_dct_16_0[1]));
        T07B = _mm_shuffle_epi8(T07B, _mm_load_si128((__m128i*)tab_dct_32_0[0]));
        T07C = _mm_shuffle_epi8(T07C, _mm_load_si128((__m128i*)tab_dct_16_0[1]));
        T07D = _mm_shuffle_epi8(T07D, _mm_load_si128((__m128i*)tab_dct_32_0[0]));

        T10A = _mm_add_epi16(T00A, T00D);   // [E05 E02 E06 E01 E04 E03 E07 E00]
        T10B = _mm_add_epi16(T00B, T00C);   // [E10 E13 E09 E14 E11 E12 E08 E15]
        T11A = _mm_add_epi16(T01A, T01D);
        T11B = _mm_add_epi16(T01B, T01C);
        T12A = _mm_add_epi16(T02A, T02D);
        T12B = _mm_add_epi16(T02B, T02C);
        T13A = _mm_add_epi16(T03A, T03D);
        T13B = _mm_add_epi16(T03B, T03C);
        T14A = _mm_add_epi16(T04A, T04D);
        T14B = _mm_add_epi16(T04B, T04C);
        T15A = _mm_add_epi16(T05A, T05D);
        T15B = _mm_add_epi16(T05B, T05C);
        T16A = _mm_add_epi16(T06A, T06D);
        T16B = _mm_add_epi16(T06B, T06C);
        T17A = _mm_add_epi16(T07A, T07D);
        T17B = _mm_add_epi16(T07B, T07C);

        T00A = _mm_sub_epi16(T00A, T00D);   // [O05 O02 O06 O01 O04 O03 O07 O00]
        T00B = _mm_sub_epi16(T00B, T00C);   // [O10 O13 O09 O14 O11 O12 O08 O15]
        T01A = _mm_sub_epi16(T01A, T01D);
        T01B = _mm_sub_epi16(T01B, T01C);
        T02A = _mm_sub_epi16(T02A, T02D);
        T02B = _mm_sub_epi16(T02B, T02C);
        T03A = _mm_sub_epi16(T03A, T03D);
        T03B = _mm_sub_epi16(T03B, T03C);
        T04A = _mm_sub_epi16(T04A, T04D);
        T04B = _mm_sub_epi16(T04B, T04C);
        T05A = _mm_sub_epi16(T05A, T05D);
        T05B = _mm_sub_epi16(T05B, T05C);
        T06A = _mm_sub_epi16(T06A, T06D);
        T06B = _mm_sub_epi16(T06B, T06C);
        T07A = _mm_sub_epi16(T07A, T07D);
        T07B = _mm_sub_epi16(T07B, T07C);

        T20  = _mm_add_epi16(T10A, T10B);   // [EE5 EE2 EE6 EE1 EE4 EE3 EE7 EE0]
        T21  = _mm_add_epi16(T11A, T11B);
        T22  = _mm_add_epi16(T12A, T12B);
        T23  = _mm_add_epi16(T13A, T13B);
        T24  = _mm_add_epi16(T14A, T14B);
        T25  = _mm_add_epi16(T15A, T15B);
        T26  = _mm_add_epi16(T16A, T16B);
        T27  = _mm_add_epi16(T17A, T17B);

        T30  = _mm_madd_epi16(T20, _mm_load_si128((__m128i*)tab_dct_8[1]));
        T31  = _mm_madd_epi16(T21, _mm_load_si128((__m128i*)tab_dct_8[1]));
        T32  = _mm_madd_epi16(T22, _mm_load_si128((__m128i*)tab_dct_8[1]));
        T33  = _mm_madd_epi16(T23, _mm_load_si128((__m128i*)tab_dct_8[1]));
        T34  = _mm_madd_epi16(T24, _mm_load_si128((__m128i*)tab_dct_8[1]));
        T35  = _mm_madd_epi16(T25, _mm_load_si128((__m128i*)tab_dct_8[1]));
        T36  = _mm_madd_epi16(T26, _mm_load_si128((__m128i*)tab_dct_8[1]));
        T37  = _mm_madd_epi16(T27, _mm_load_si128((__m128i*)tab_dct_8[1]));

        T40  = _mm_hadd_epi32(T30, T31);
        T41  = _mm_hadd_epi32(T32, T33);
        T42  = _mm_hadd_epi32(T34, T35);
        T43  = _mm_hadd_epi32(T36, T37);

        T50  = _mm_hadd_epi32(T40, T41);
        T51  = _mm_hadd_epi32(T42, T43);
        T50  = _mm_srai_epi32(_mm_add_epi32(T50, c_8), 4);
        T51  = _mm_srai_epi32(_mm_add_epi32(T51, c_8), 4);
        T60  = _mm_packs_epi32(T50, T51);
        im[ 0][i] = T60;

        T50  = _mm_hsub_epi32(T40, T41);
        T51  = _mm_hsub_epi32(T42, T43);
        T50  = _mm_srai_epi32(_mm_add_epi32(T50, c_8), 4);
        T51  = _mm_srai_epi32(_mm_add_epi32(T51, c_8), 4);
        T60  = _mm_packs_epi32(T50, T51);
        im[16][i] = T60;

        T30  = _mm_madd_epi16(T20, _mm_load_si128((__m128i*)tab_dct_16_1[ 8]));
        T31  = _mm_madd_epi16(T21, _mm_load_si128((__m128i*)tab_dct_16_1[ 8]));
        T32  = _mm_madd_epi16(T22, _mm_load_si128((__m128i*)tab_dct_16_1[ 8]));
        T33  = _mm_madd_epi16(T23, _mm_load_si128((__m128i*)tab_dct_16_1[ 8]));
        T34  = _mm_madd_epi16(T24, _mm_load_si128((__m128i*)tab_dct_16_1[ 8]));
        T35  = _mm_madd_epi16(T25, _mm_load_si128((__m128i*)tab_dct_16_1[ 8]));
        T36  = _mm_madd_epi16(T26, _mm_load_si128((__m128i*)tab_dct_16_1[ 8]));
        T37  = _mm_madd_epi16(T27, _mm_load_si128((__m128i*)tab_dct_16_1[ 8]));

        T40  = _mm_hadd_epi32(T30, T31);
        T41  = _mm_hadd_epi32(T32, T33);
        T42  = _mm_hadd_epi32(T34, T35);
        T43  = _mm_hadd_epi32(T36, T37);

        T50  = _mm_hadd_epi32(T40, T41);
        T51  = _mm_hadd_epi32(T42, T43);
        T50  = _mm_srai_epi32(_mm_add_epi32(T50, c_8), 4);
        T51  = _mm_srai_epi32(_mm_add_epi32(T51, c_8), 4);
        T60  = _mm_packs_epi32(T50, T51);
        im[ 8][i] = T60;
        
        T30  = _mm_madd_epi16(T20, _mm_load_si128((__m128i*)tab_dct_16_1[ 9]));
        T31  = _mm_madd_epi16(T21, _mm_load_si128((__m128i*)tab_dct_16_1[ 9]));
        T32  = _mm_madd_epi16(T22, _mm_load_si128((__m128i*)tab_dct_16_1[ 9]));
        T33  = _mm_madd_epi16(T23, _mm_load_si128((__m128i*)tab_dct_16_1[ 9]));
        T34  = _mm_madd_epi16(T24, _mm_load_si128((__m128i*)tab_dct_16_1[ 9]));
        T35  = _mm_madd_epi16(T25, _mm_load_si128((__m128i*)tab_dct_16_1[ 9]));
        T36  = _mm_madd_epi16(T26, _mm_load_si128((__m128i*)tab_dct_16_1[ 9]));
        T37  = _mm_madd_epi16(T27, _mm_load_si128((__m128i*)tab_dct_16_1[ 9]));

        T40  = _mm_hadd_epi32(T30, T31);
        T41  = _mm_hadd_epi32(T32, T33);
        T42  = _mm_hadd_epi32(T34, T35);
        T43  = _mm_hadd_epi32(T36, T37);

        T50  = _mm_hadd_epi32(T40, T41);
        T51  = _mm_hadd_epi32(T42, T43);
        T50  = _mm_srai_epi32(_mm_add_epi32(T50, c_8), 4);
        T51  = _mm_srai_epi32(_mm_add_epi32(T51, c_8), 4);
        T60  = _mm_packs_epi32(T50, T51);
        im[24][i] = T60;

#define MAKE_ODD(tab, dst) \
        T30  = _mm_madd_epi16(T20, _mm_load_si128((__m128i*)tab_dct_32_1[(tab)])); \
        T31  = _mm_madd_epi16(T21, _mm_load_si128((__m128i*)tab_dct_32_1[(tab)])); \
        T32  = _mm_madd_epi16(T22, _mm_load_si128((__m128i*)tab_dct_32_1[(tab)])); \
        T33  = _mm_madd_epi16(T23, _mm_load_si128((__m128i*)tab_dct_32_1[(tab)])); \
        T34  = _mm_madd_epi16(T24, _mm_load_si128((__m128i*)tab_dct_32_1[(tab)])); \
        T35  = _mm_madd_epi16(T25, _mm_load_si128((__m128i*)tab_dct_32_1[(tab)])); \
        T36  = _mm_madd_epi16(T26, _mm_load_si128((__m128i*)tab_dct_32_1[(tab)])); \
        T37  = _mm_madd_epi16(T27, _mm_load_si128((__m128i*)tab_dct_32_1[(tab)])); \
        \
        T40  = _mm_hadd_epi32(T30, T31); \
        T41  = _mm_hadd_epi32(T32, T33); \
        T42  = _mm_hadd_epi32(T34, T35); \
        T43  = _mm_hadd_epi32(T36, T37); \
        \
        T50  = _mm_hadd_epi32(T40, T41); \
        T51  = _mm_hadd_epi32(T42, T43); \
        T50  = _mm_srai_epi32(_mm_add_epi32(T50, c_8), 4); \
        T51  = _mm_srai_epi32(_mm_add_epi32(T51, c_8), 4); \
        T60  = _mm_packs_epi32(T50, T51); \
        im[(dst)][i] = T60;
 
        MAKE_ODD( 0, 4);
        MAKE_ODD( 1,12);
        MAKE_ODD( 2,20);
        MAKE_ODD( 3,28);

        T20  = _mm_sub_epi16(T10A, T10B);   // [EO5 EO2 EO6 EO1 EO4 EO3 EO7 EO0]
        T21  = _mm_sub_epi16(T11A, T11B);
        T22  = _mm_sub_epi16(T12A, T12B);
        T23  = _mm_sub_epi16(T13A, T13B);
        T24  = _mm_sub_epi16(T14A, T14B);
        T25  = _mm_sub_epi16(T15A, T15B);
        T26  = _mm_sub_epi16(T16A, T16B);
        T27  = _mm_sub_epi16(T17A, T17B);

        MAKE_ODD( 4, 2);
        MAKE_ODD( 5, 6);
        MAKE_ODD( 6,10);
        MAKE_ODD( 7,14);
        MAKE_ODD( 8,18);
        MAKE_ODD( 9,22);
        MAKE_ODD(10,26);
        MAKE_ODD(11,30);
#undef MAKE_ODD

#define MAKE_ODD(tab, dst) \
        T20  = _mm_madd_epi16(T00A, _mm_load_si128((__m128i*)tab_dct_32_1[(tab)  ])); \
        T21  = _mm_madd_epi16(T00B, _mm_load_si128((__m128i*)tab_dct_32_1[(tab)+1])); \
        T22  = _mm_madd_epi16(T01A, _mm_load_si128((__m128i*)tab_dct_32_1[(tab)  ])); \
        T23  = _mm_madd_epi16(T01B, _mm_load_si128((__m128i*)tab_dct_32_1[(tab)+1])); \
        T24  = _mm_madd_epi16(T02A, _mm_load_si128((__m128i*)tab_dct_32_1[(tab)  ])); \
        T25  = _mm_madd_epi16(T02B, _mm_load_si128((__m128i*)tab_dct_32_1[(tab)+1])); \
        T26  = _mm_madd_epi16(T03A, _mm_load_si128((__m128i*)tab_dct_32_1[(tab)  ])); \
        T27  = _mm_madd_epi16(T03B, _mm_load_si128((__m128i*)tab_dct_32_1[(tab)+1])); \
        T30  = _mm_madd_epi16(T04A, _mm_load_si128((__m128i*)tab_dct_32_1[(tab)  ])); \
        T31  = _mm_madd_epi16(T04B, _mm_load_si128((__m128i*)tab_dct_32_1[(tab)+1])); \
        T32  = _mm_madd_epi16(T05A, _mm_load_si128((__m128i*)tab_dct_32_1[(tab)  ])); \
        T33  = _mm_madd_epi16(T05B, _mm_load_si128((__m128i*)tab_dct_32_1[(tab)+1])); \
        T34  = _mm_madd_epi16(T06A, _mm_load_si128((__m128i*)tab_dct_32_1[(tab)  ])); \
        T35  = _mm_madd_epi16(T06B, _mm_load_si128((__m128i*)tab_dct_32_1[(tab)+1])); \
        T36  = _mm_madd_epi16(T07A, _mm_load_si128((__m128i*)tab_dct_32_1[(tab)  ])); \
        T37  = _mm_madd_epi16(T07B, _mm_load_si128((__m128i*)tab_dct_32_1[(tab)+1])); \
        \
        T40  = _mm_hadd_epi32(T20, T21); \
        T41  = _mm_hadd_epi32(T22, T23); \
        T42  = _mm_hadd_epi32(T24, T25); \
        T43  = _mm_hadd_epi32(T26, T27); \
        T44  = _mm_hadd_epi32(T30, T31); \
        T45  = _mm_hadd_epi32(T32, T33); \
        T46  = _mm_hadd_epi32(T34, T35); \
        T47  = _mm_hadd_epi32(T36, T37); \
        \
        T50  = _mm_hadd_epi32(T40, T41); \
        T51  = _mm_hadd_epi32(T42, T43); \
        T52  = _mm_hadd_epi32(T44, T45); \
        T53  = _mm_hadd_epi32(T46, T47); \
        \
        T50  = _mm_hadd_epi32(T50, T51); \
        T51  = _mm_hadd_epi32(T52, T53); \
        T50  = _mm_srai_epi32(_mm_add_epi32(T50, c_8), 4); \
        T51  = _mm_srai_epi32(_mm_add_epi32(T51, c_8), 4); \
        T60  = _mm_packs_epi32(T50, T51); \
        im[(dst)][i] = T60;

        MAKE_ODD( 12,  1);
        MAKE_ODD( 14,  3);
        MAKE_ODD( 16,  5);
        MAKE_ODD( 18,  7);
        MAKE_ODD( 20,  9);
        MAKE_ODD( 22, 11);
        MAKE_ODD( 24, 13);
        MAKE_ODD( 26, 15);
        MAKE_ODD( 28, 17);
        MAKE_ODD( 30, 19);
        MAKE_ODD( 32, 21);
        MAKE_ODD( 34, 23);
        MAKE_ODD( 36, 25);
        MAKE_ODD( 38, 27);
        MAKE_ODD( 40, 29);
        MAKE_ODD( 42, 31);

#undef MAKE_ODD
    }

    // DCT2
    for( i=0; i<32/4; i++ )
    {
        // OPT_ME: to avoid register spill, I use matrix multiply, have other way?
        T00A = im[i*4+0][0];    // [07 06 05 04 03 02 01 00]
        T00B = im[i*4+0][1];    // [15 14 13 12 11 10 09 08]
        T00C = im[i*4+0][2];    // [23 22 21 20 19 18 17 16]
        T00D = im[i*4+0][3];    // [31 30 29 28 27 26 25 24]
        T01A = im[i*4+1][0];
        T01B = im[i*4+1][1];
        T01C = im[i*4+1][2];
        T01D = im[i*4+1][3];
        T02A = im[i*4+2][0];
        T02B = im[i*4+2][1];
        T02C = im[i*4+2][2];
        T02D = im[i*4+2][3];
        T03A = im[i*4+3][0];
        T03B = im[i*4+3][1];
        T03C = im[i*4+3][2];
        T03D = im[i*4+3][3];

        T00C = _mm_shuffle_epi8(T00C, _mm_load_si128((__m128i*)tab_dct_16_0[0]));    // [16 17 18 19 20 21 22 23]
        T00D = _mm_shuffle_epi8(T00D, _mm_load_si128((__m128i*)tab_dct_16_0[0]));    // [24 25 26 27 28 29 30 31]
        T01C = _mm_shuffle_epi8(T01C, _mm_load_si128((__m128i*)tab_dct_16_0[0]));
        T01D = _mm_shuffle_epi8(T01D, _mm_load_si128((__m128i*)tab_dct_16_0[0]));
        T02C = _mm_shuffle_epi8(T02C, _mm_load_si128((__m128i*)tab_dct_16_0[0]));
        T02D = _mm_shuffle_epi8(T02D, _mm_load_si128((__m128i*)tab_dct_16_0[0]));
        T03C = _mm_shuffle_epi8(T03C, _mm_load_si128((__m128i*)tab_dct_16_0[0]));
        T03D = _mm_shuffle_epi8(T03D, _mm_load_si128((__m128i*)tab_dct_16_0[0]));

        T10A = _mm_unpacklo_epi16(T00A, T00D);  // [28 03 29 02 30 01 31 00]
        T10B = _mm_unpackhi_epi16(T00A, T00D);  // [24 07 25 06 26 05 27 04]
        T00A = _mm_unpacklo_epi16(T00B, T00C);  // [20 11 21 10 22 09 23 08]
        T00B = _mm_unpackhi_epi16(T00B, T00C);  // [16 15 17 14 18 13 19 12]
        T11A = _mm_unpacklo_epi16(T01A, T01D);
        T11B = _mm_unpackhi_epi16(T01A, T01D);
        T01A = _mm_unpacklo_epi16(T01B, T01C);
        T01B = _mm_unpackhi_epi16(T01B, T01C);
        T12A = _mm_unpacklo_epi16(T02A, T02D);
        T12B = _mm_unpackhi_epi16(T02A, T02D);
        T02A = _mm_unpacklo_epi16(T02B, T02C);
        T02B = _mm_unpackhi_epi16(T02B, T02C);
        T13A = _mm_unpacklo_epi16(T03A, T03D);
        T13B = _mm_unpackhi_epi16(T03A, T03D);
        T03A = _mm_unpacklo_epi16(T03B, T03C);
        T03B = _mm_unpackhi_epi16(T03B, T03C);

#define MAKE_ODD(tab0, tab1, tab2, tab3, dst) \
        T20  = _mm_madd_epi16(T10A, _mm_load_si128((__m128i*)tab_dct_32_1[(tab0)])); \
        T21  = _mm_madd_epi16(T10B, _mm_load_si128((__m128i*)tab_dct_32_1[(tab1)])); \
        T22  = _mm_madd_epi16(T00A, _mm_load_si128((__m128i*)tab_dct_32_1[(tab2)])); \
        T23  = _mm_madd_epi16(T00B, _mm_load_si128((__m128i*)tab_dct_32_1[(tab3)])); \
        T24  = _mm_madd_epi16(T11A, _mm_load_si128((__m128i*)tab_dct_32_1[(tab0)])); \
        T25  = _mm_madd_epi16(T11B, _mm_load_si128((__m128i*)tab_dct_32_1[(tab1)])); \
        T26  = _mm_madd_epi16(T01A, _mm_load_si128((__m128i*)tab_dct_32_1[(tab2)])); \
        T27  = _mm_madd_epi16(T01B, _mm_load_si128((__m128i*)tab_dct_32_1[(tab3)])); \
        T30  = _mm_madd_epi16(T12A, _mm_load_si128((__m128i*)tab_dct_32_1[(tab0)])); \
        T31  = _mm_madd_epi16(T12B, _mm_load_si128((__m128i*)tab_dct_32_1[(tab1)])); \
        T32  = _mm_madd_epi16(T02A, _mm_load_si128((__m128i*)tab_dct_32_1[(tab2)])); \
        T33  = _mm_madd_epi16(T02B, _mm_load_si128((__m128i*)tab_dct_32_1[(tab3)])); \
        T34  = _mm_madd_epi16(T13A, _mm_load_si128((__m128i*)tab_dct_32_1[(tab0)])); \
        T35  = _mm_madd_epi16(T13B, _mm_load_si128((__m128i*)tab_dct_32_1[(tab1)])); \
        T36  = _mm_madd_epi16(T03A, _mm_load_si128((__m128i*)tab_dct_32_1[(tab2)])); \
        T37  = _mm_madd_epi16(T03B, _mm_load_si128((__m128i*)tab_dct_32_1[(tab3)])); \
        \
        T60  = _mm_hadd_epi32(T20, T21); \
        T61  = _mm_hadd_epi32(T22, T23); \
        T62  = _mm_hadd_epi32(T24, T25); \
        T63  = _mm_hadd_epi32(T26, T27); \
        T64  = _mm_hadd_epi32(T30, T31); \
        T65  = _mm_hadd_epi32(T32, T33); \
        T66  = _mm_hadd_epi32(T34, T35); \
        T67  = _mm_hadd_epi32(T36, T37); \
        \
        T60  = _mm_hadd_epi32(T60, T61); \
        T61  = _mm_hadd_epi32(T62, T63); \
        T62  = _mm_hadd_epi32(T64, T65); \
        T63  = _mm_hadd_epi32(T66, T67); \
        \
        T60  = _mm_hadd_epi32(T60, T61); \
        T61  = _mm_hadd_epi32(T62, T63); \
        \
        T60  = _mm_hadd_epi32(T60, T61); \
        \
        T60  = _mm_srai_epi32(_mm_add_epi32(T60, c_1024), 11); \
        _mm_storeu_si128((__m128i*)&pDst[(dst)*32+(i*4)+0], T60); \

        MAKE_ODD( 44, 44, 44, 44,  0);
        MAKE_ODD( 45, 45, 45, 45, 16);
        MAKE_ODD( 46, 47, 46, 47,  8);
        MAKE_ODD( 48, 49, 48, 49, 24);

        MAKE_ODD( 50, 51, 52, 53,  4);
        MAKE_ODD( 54, 55, 56, 57, 12);
        MAKE_ODD( 58, 59, 60, 61, 20);
        MAKE_ODD( 62, 63, 64, 65, 28);

        MAKE_ODD( 66, 67, 68, 69,  2);
        MAKE_ODD( 70, 71, 72, 73,  6);
        MAKE_ODD( 74, 75, 76, 77, 10);
        MAKE_ODD( 78, 79, 80, 81, 14);

        MAKE_ODD( 82, 83, 84, 85, 18);
        MAKE_ODD( 86, 87, 88, 89, 22);
        MAKE_ODD( 90, 91, 92, 93, 26);
        MAKE_ODD( 94, 95, 96, 97, 30);

        MAKE_ODD( 98, 99,100,101,  1);
        MAKE_ODD(102,103,104,105,  3);
        MAKE_ODD(106,107,108,109,  5);
        MAKE_ODD(110,111,112,113,  7);
        MAKE_ODD(114,115,116,117,  9);
        MAKE_ODD(118,119,120,121, 11);
        MAKE_ODD(122,123,124,125, 13);
        MAKE_ODD(126,127,128,129, 15);
        MAKE_ODD(130,131,132,133, 17);
        MAKE_ODD(134,135,136,137, 19);
        MAKE_ODD(138,139,140,141, 21);
        MAKE_ODD(142,143,144,145, 23);
        MAKE_ODD(146,147,148,149, 25);
        MAKE_ODD(150,151,152,153, 27);
        MAKE_ODD(154,155,156,157, 29);
        MAKE_ODD(158,159,160,161, 31);
#undef MAKE_ODD
    }
}
#endif // INSTRSET >= 4


ALIGN_VAR_32(static const short, tab_idct_4x4[4][8] )=
{
    { 64,  64, 64,  64, 64,  64, 64,  64 },
    { 64, -64, 64, -64, 64, -64, 64, -64 },
    { 83,  36, 83,  36, 83,  36, 83,  36 },
    { 36, -83, 36, -83, 36, -83, 36, -83 },
};
void xIDCT4(short *pSrc, short *pDst, intptr_t stride)
{
    __m128i S0, S8, m128iAdd, m128Tmp1, m128Tmp2, E1, E2, O1, O2, m128iA, m128iD;
    S0   = _mm_load_si128( (__m128i*)( pSrc     ) );
    S8   = _mm_load_si128( (__m128i*)( pSrc + 8 ) );
    m128iAdd  = _mm_set1_epi32( 64 );

    m128Tmp1 = _mm_unpacklo_epi16(  S0, S8 );
    E1 = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_4x4[0] ) ) );
    E1 = _mm_add_epi32( E1, m128iAdd );

    E2 = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_4x4[1] ) ) );
    E2 = _mm_add_epi32( E2, m128iAdd );


    m128Tmp1 = _mm_unpackhi_epi16(  S0, S8 );
    O1 = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_4x4[2] ) ) );
    O2 = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_4x4[3] ) ) );

    m128iA  = _mm_add_epi32( E1, O1 );
    m128iA  = _mm_srai_epi32( m128iA, 7  );        // Sum = Sum >> iShiftNum
    m128Tmp1 = _mm_add_epi32( E2, O2 );
    m128Tmp1 = _mm_srai_epi32( m128Tmp1, 7  );       // Sum = Sum >> iShiftNum
    m128iA = _mm_packs_epi32( m128iA, m128Tmp1);

    m128iD = _mm_sub_epi32( E2, O2 );
    m128iD = _mm_srai_epi32( m128iD, 7  );         // Sum = Sum >> iShiftNum

    m128Tmp1 = _mm_sub_epi32( E1, O1 );
    m128Tmp1 = _mm_srai_epi32( m128Tmp1, 7  );       // Sum = Sum >> iShiftNum

    m128iD = _mm_packs_epi32( m128iD, m128Tmp1 );

    S0 =_mm_unpacklo_epi16(  m128iA, m128iD );
    S8 =_mm_unpackhi_epi16(  m128iA, m128iD );

    m128iA =_mm_unpacklo_epi16(  S0, S8 );
    m128iD =_mm_unpackhi_epi16(  S0, S8 );

    /*  ##########################  */


    m128iAdd  = _mm_set1_epi32( 2048 );
    m128Tmp1 = _mm_unpacklo_epi16(  m128iA, m128iD );
    E1 = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_4x4[0] ) ) );
    E1 = _mm_add_epi32( E1, m128iAdd );

    E2 = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_4x4[1] ) ) );
    E2 = _mm_add_epi32( E2, m128iAdd );


    m128Tmp1 = _mm_unpackhi_epi16(  m128iA, m128iD );
    O1 = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_4x4[2] ) ) );
    O2 = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_4x4[3] ) ) );

    m128iA   = _mm_add_epi32( E1, O1 );
    m128iA   = _mm_srai_epi32( m128iA, 12  );
    m128Tmp1 = _mm_add_epi32( E2, O2 );
    m128Tmp1 = _mm_srai_epi32( m128Tmp1, 12  );
    m128iA   = _mm_packs_epi32( m128iA, m128Tmp1);

    m128iD = _mm_sub_epi32( E2, O2 );
    m128iD = _mm_srai_epi32( m128iD, 12  );

    m128Tmp1 = _mm_sub_epi32( E1, O1 );
    m128Tmp1 = _mm_srai_epi32( m128Tmp1, 12  );

    m128iD = _mm_packs_epi32( m128iD, m128Tmp1 );

    m128Tmp1 = _mm_unpacklo_epi16(m128iA, m128iD);   // [32 30 22 20 12 10 02 00]
    m128Tmp2 = _mm_unpackhi_epi16(m128iA, m128iD);   // [33 31 23 21 13 11 03 01]
    m128iA   = _mm_unpacklo_epi16(m128Tmp1, m128Tmp2);
    m128iD   = _mm_unpackhi_epi16(m128Tmp1, m128Tmp2);

    _mm_storel_epi64( (__m128i*)&pDst[0 * stride], m128iA );
    _mm_storeh_pi   ( (__m64*  )&pDst[1 * stride], _mm_castsi128_ps(m128iA));
    _mm_storel_epi64( (__m128i*)&pDst[2 * stride], m128iD );
    _mm_storeh_pi   ( (__m64*  )&pDst[3 * stride], _mm_castsi128_ps(m128iD));
}

ALIGN_VAR_32(static const short, tab_idct_8x8[12][8] )=
{
    {  89,  75,  89,  75, 89,  75, 89,  75 },
    {  50,  18,  50,  18, 50,  18, 50,  18 },
    {  75, -18,  75, -18, 75, -18, 75, -18 },
    { -89, -50, -89, -50,-89, -50,-89, -50 },
    {  50, -89,  50, -89, 50, -89, 50, -89 },
    {  18,  75,  18,  75, 18,  75, 18,  75 },
    {  18, -50,  18, -50, 18, -50, 18, -50 },
    {  75, -89,  75, -89, 75, -89, 75, -89 },
    {  64,  64,  64,  64, 64,  64, 64,  64 },
    {  64, -64,  64, -64, 64, -64, 64, -64 },
    {  83,  36,  83,  36, 83,  36, 83,  36 },
    {  36, -83,  36, -83, 36, -83, 36, -83 }
};
void xIDCT8(short *pSrc, short *pDst, intptr_t stride)
{
    __m128i m128iS0, m128iS1, m128iS2, m128iS3, m128iS4, m128iS5, m128iS6, m128iS7, m128iAdd, m128Tmp0, m128Tmp1, m128Tmp2, m128Tmp3, E0h, E1h, E2h, E3h, E0l, E1l, E2l, E3l, O0h, O1h, O2h, O3h, O0l, O1l, O2l, O3l, EE0l, EE1l, E00l, E01l, EE0h, EE1h, E00h, E01h;
    m128iAdd  = _mm_set1_epi32( 64 );

    m128iS1   = _mm_load_si128( (__m128i*)( pSrc + 8   ) );
    m128iS3   = _mm_load_si128( (__m128i*)( pSrc + 24 ) );
    m128Tmp0 = _mm_unpacklo_epi16(  m128iS1, m128iS3 );
    E1l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_8x8[0] ) ) );
    m128Tmp1 = _mm_unpackhi_epi16(  m128iS1, m128iS3 );
    E1h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_8x8[0] ) ) );
    m128iS5   = _mm_load_si128( (__m128i*)( pSrc + 40   ) );
    m128iS7   = _mm_load_si128( (__m128i*)( pSrc + 56 ) );
    m128Tmp2 =  _mm_unpacklo_epi16(  m128iS5, m128iS7 );
    E2l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_8x8[1] ) ) );
    m128Tmp3 = _mm_unpackhi_epi16(  m128iS5, m128iS7 );
    E2h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_8x8[1] ) ) );
    O0l = _mm_add_epi32(E1l, E2l);
    O0h = _mm_add_epi32(E1h, E2h);

    E1l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_8x8[2] ) ) );
    E1h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_8x8[2] ) ) );
    E2l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_8x8[3] ) ) );
    E2h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_8x8[3] ) ) );

    O1l = _mm_add_epi32(E1l, E2l);
    O1h = _mm_add_epi32(E1h, E2h);

    E1l =  _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_8x8[4] ) ) );
    E1h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_8x8[4] ) ) );
    E2l =  _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_8x8[5] ) ) );
    E2h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_8x8[5] ) ) );
    O2l = _mm_add_epi32(E1l, E2l);
    O2h = _mm_add_epi32(E1h, E2h);

    E1l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_8x8[6] ) ) );
    E1h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_8x8[6] ) ) );
    E2l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_8x8[7] ) ) );
    E2h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_8x8[7] ) ) );
    O3h = _mm_add_epi32(E1h, E2h);
    O3l = _mm_add_epi32(E1l, E2l);

    /*    -------     */

    m128iS0   = _mm_load_si128( (__m128i*)( pSrc + 0   ) );
    m128iS4   = _mm_load_si128( (__m128i*)( pSrc + 32   ) );
    m128Tmp0 = _mm_unpacklo_epi16(  m128iS0, m128iS4 );
    EE0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_8x8[8] ) ) );
    m128Tmp1 = _mm_unpackhi_epi16(  m128iS0, m128iS4 );
    EE0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_8x8[8] ) ) );

    EE1l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_8x8[9] ) ) );
    EE1h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_8x8[9] ) ) );


    /*    -------     */

    m128iS2   = _mm_load_si128( (__m128i*)( pSrc  +16) );
    m128iS6   = _mm_load_si128( (__m128i*)( pSrc + 48   ) );
    m128Tmp0 = _mm_unpacklo_epi16(  m128iS2, m128iS6 );
    E00l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_8x8[10] ) ) );
    m128Tmp1 = _mm_unpackhi_epi16(  m128iS2, m128iS6 );
    E00h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_8x8[10] ) ) );
    E01l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_8x8[11] ) ) );
    E01h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_8x8[11] ) ) );
    E0l = _mm_add_epi32(EE0l , E00l);
    E0l = _mm_add_epi32(E0l, m128iAdd);
    E0h = _mm_add_epi32(EE0h , E00h);
    E0h = _mm_add_epi32(E0h, m128iAdd);
    E3l = _mm_sub_epi32(EE0l , E00l);
    E3l = _mm_add_epi32(E3l , m128iAdd);
    E3h = _mm_sub_epi32(EE0h , E00h);
    E3h = _mm_add_epi32(E3h , m128iAdd);

    E1l = _mm_add_epi32(EE1l , E01l);
    E1l = _mm_add_epi32(E1l , m128iAdd);
    E1h = _mm_add_epi32(EE1h , E01h);
    E1h = _mm_add_epi32(E1h , m128iAdd);
    E2l = _mm_sub_epi32(EE1l , E01l);
    E2l = _mm_add_epi32(E2l , m128iAdd);
    E2h = _mm_sub_epi32(EE1h , E01h);
    E2h = _mm_add_epi32(E2h , m128iAdd);
    m128iS0 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E0l, O0l),7), _mm_srai_epi32(_mm_add_epi32(E0h, O0h), 7));
    m128iS1 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E1l, O1l),7), _mm_srai_epi32(_mm_add_epi32(E1h, O1h), 7));
    m128iS2 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E2l, O2l),7), _mm_srai_epi32(_mm_add_epi32(E2h, O2h), 7));
    m128iS3 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E3l, O3l),7), _mm_srai_epi32(_mm_add_epi32(E3h, O3h), 7));
    m128iS4 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E3l, O3l),7), _mm_srai_epi32(_mm_sub_epi32(E3h, O3h), 7));
    m128iS5 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E2l, O2l),7), _mm_srai_epi32(_mm_sub_epi32(E2h, O2h), 7));
    m128iS6 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E1l, O1l),7), _mm_srai_epi32(_mm_sub_epi32(E1h, O1h), 7));
    m128iS7 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E0l, O0l),7), _mm_srai_epi32(_mm_sub_epi32(E0h, O0h), 7));
    /*  Invers matrix   */

    E0l = _mm_unpacklo_epi16(m128iS0, m128iS4);
    E1l = _mm_unpacklo_epi16(m128iS1, m128iS5);
    E2l = _mm_unpacklo_epi16(m128iS2, m128iS6);
    E3l = _mm_unpacklo_epi16(m128iS3, m128iS7);
    O0l = _mm_unpackhi_epi16(m128iS0, m128iS4);
    O1l = _mm_unpackhi_epi16(m128iS1, m128iS5);
    O2l = _mm_unpackhi_epi16(m128iS2, m128iS6);
    O3l = _mm_unpackhi_epi16(m128iS3, m128iS7);
    m128Tmp0 = _mm_unpacklo_epi16(E0l, E2l);
    m128Tmp1 = _mm_unpacklo_epi16(E1l, E3l);
    m128iS0  = _mm_unpacklo_epi16(m128Tmp0, m128Tmp1);
    m128iS1  = _mm_unpackhi_epi16(m128Tmp0, m128Tmp1);
    m128Tmp2 = _mm_unpackhi_epi16(E0l, E2l);
    m128Tmp3 = _mm_unpackhi_epi16(E1l, E3l);
    m128iS2 = _mm_unpacklo_epi16(m128Tmp2, m128Tmp3);
    m128iS3 = _mm_unpackhi_epi16(m128Tmp2, m128Tmp3);
    m128Tmp0 = _mm_unpacklo_epi16(O0l, O2l);
    m128Tmp1 = _mm_unpacklo_epi16(O1l, O3l);
    m128iS4  = _mm_unpacklo_epi16(m128Tmp0, m128Tmp1);
    m128iS5  = _mm_unpackhi_epi16(m128Tmp0, m128Tmp1);
    m128Tmp2 = _mm_unpackhi_epi16(O0l, O2l);
    m128Tmp3 = _mm_unpackhi_epi16(O1l, O3l);
    m128iS6 = _mm_unpacklo_epi16(m128Tmp2, m128Tmp3);
    m128iS7 = _mm_unpackhi_epi16(m128Tmp2, m128Tmp3);

    m128iAdd  = _mm_set1_epi32( 2048 );


    m128Tmp0 = _mm_unpacklo_epi16(  m128iS1, m128iS3 );
    E1l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_8x8[0] ) ) );
    m128Tmp1 = _mm_unpackhi_epi16(  m128iS1, m128iS3 );
    E1h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_8x8[0] ) ) );
    m128Tmp2 =  _mm_unpacklo_epi16(  m128iS5, m128iS7 );
    E2l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_8x8[1] ) ) );
    m128Tmp3 = _mm_unpackhi_epi16(  m128iS5, m128iS7 );
    E2h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_8x8[1] ) ) );
    O0l = _mm_add_epi32(E1l, E2l);
    O0h = _mm_add_epi32(E1h, E2h);
    E1l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_8x8[2] ) ) );
    E1h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_8x8[2] ) ) );
    E2l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_8x8[3] ) ) );
    E2h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_8x8[3] ) ) );
    O1l = _mm_add_epi32(E1l, E2l);
    O1h = _mm_add_epi32(E1h, E2h);
    E1l =  _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_8x8[4] ) ) );
    E1h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_8x8[4] ) ) );
    E2l =  _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_8x8[5] ) ) );
    E2h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_8x8[5] ) ) );
    O2l = _mm_add_epi32(E1l, E2l);
    O2h = _mm_add_epi32(E1h, E2h);
    E1l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_8x8[6] ) ) );
    E1h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_8x8[6] ) ) );
    E2l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_8x8[7] ) ) );
    E2h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_8x8[7] ) ) );
    O3h = _mm_add_epi32(E1h, E2h);
    O3l = _mm_add_epi32(E1l, E2l);

    m128Tmp0 = _mm_unpacklo_epi16(  m128iS0, m128iS4 );
    EE0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_8x8[8] ) ) );
    m128Tmp1 = _mm_unpackhi_epi16(  m128iS0, m128iS4 );
    EE0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_8x8[8] ) ) );
    EE1l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_8x8[9] ) ) );
    EE1h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_8x8[9] ) ) );

    m128Tmp0 = _mm_unpacklo_epi16(  m128iS2, m128iS6 );
    E00l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_8x8[10] ) ) );
    m128Tmp1 = _mm_unpackhi_epi16(  m128iS2, m128iS6 );
    E00h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_8x8[10] ) ) );
    E01l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_8x8[11] ) ) );
    E01h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_8x8[11] ) ) );
    E0l = _mm_add_epi32(EE0l , E00l);
    E0l = _mm_add_epi32(E0l, m128iAdd);
    E0h = _mm_add_epi32(EE0h , E00h);
    E0h = _mm_add_epi32(E0h, m128iAdd);
    E3l = _mm_sub_epi32(EE0l , E00l);
    E3l = _mm_add_epi32(E3l , m128iAdd);
    E3h = _mm_sub_epi32(EE0h , E00h);
    E3h = _mm_add_epi32(E3h , m128iAdd);
    E1l = _mm_add_epi32(EE1l , E01l);
    E1l = _mm_add_epi32(E1l , m128iAdd);
    E1h = _mm_add_epi32(EE1h , E01h);
    E1h = _mm_add_epi32(E1h , m128iAdd);
    E2l = _mm_sub_epi32(EE1l , E01l);
    E2l = _mm_add_epi32(E2l , m128iAdd);
    E2h = _mm_sub_epi32(EE1h , E01h);
    E2h = _mm_add_epi32(E2h , m128iAdd);

    m128iS0 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E0l, O0l),12), _mm_srai_epi32(_mm_add_epi32(E0h, O0h), 12));
    m128iS1 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E1l, O1l),12), _mm_srai_epi32(_mm_add_epi32(E1h, O1h), 12));
    m128iS2 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E2l, O2l),12), _mm_srai_epi32(_mm_add_epi32(E2h, O2h), 12));
    m128iS3 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E3l, O3l),12), _mm_srai_epi32(_mm_add_epi32(E3h, O3h), 12));
    m128iS4 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E3l, O3l),12), _mm_srai_epi32(_mm_sub_epi32(E3h, O3h), 12));
    m128iS5 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E2l, O2l),12), _mm_srai_epi32(_mm_sub_epi32(E2h, O2h), 12));
    m128iS6 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E1l, O1l),12), _mm_srai_epi32(_mm_sub_epi32(E1h, O1h), 12));
    m128iS7 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E0l, O0l),12), _mm_srai_epi32(_mm_sub_epi32(E0h, O0h), 12));

    // [07 06 05 04 03 02 01 00]
    // [17 16 15 14 13 12 11 10]
    // [27 26 25 24 23 22 21 20]
    // [37 36 35 34 33 32 31 30]
    // [47 46 45 44 43 42 41 40]
    // [57 56 55 54 53 52 51 50]
    // [67 66 65 64 63 62 61 60]
    // [77 76 75 74 73 72 71 70]

    __m128i T00 = _mm_unpacklo_epi16(m128iS0, m128iS1);     // [13 03 12 02 11 01 10 00]
    __m128i T01 = _mm_unpackhi_epi16(m128iS0, m128iS1);     // [17 07 16 06 15 05 14 04]
    __m128i T02 = _mm_unpacklo_epi16(m128iS2, m128iS3);     // [33 23 32 22 31 21 30 20]
    __m128i T03 = _mm_unpackhi_epi16(m128iS2, m128iS3);     // [37 27 36 26 35 25 34 24]
    __m128i T04 = _mm_unpacklo_epi16(m128iS4, m128iS5);     // [53 43 52 42 51 41 50 40]
    __m128i T05 = _mm_unpackhi_epi16(m128iS4, m128iS5);     // [57 47 56 46 55 45 54 44]
    __m128i T06 = _mm_unpacklo_epi16(m128iS6, m128iS7);     // [73 63 72 62 71 61 70 60]
    __m128i T07 = _mm_unpackhi_epi16(m128iS6, m128iS7);     // [77 67 76 66 75 65 74 64]

    __m128i T10, T11;
    T10 = _mm_unpacklo_epi32(T00, T02);                                     // [31 21 11 01 30 20 10 00]
    T11 = _mm_unpackhi_epi32(T00, T02);                                     // [33 23 13 03 32 22 12 02]
    _mm_storel_epi64( (__m128i*)&pDst[0 * stride +  0], T10 );                   // [30 20 10 00]
    _mm_storeh_pi   ( (__m64*  )&pDst[1 * stride +  0], _mm_castsi128_ps(T10));  // [31 21 11 01]
    _mm_storel_epi64( (__m128i*)&pDst[2 * stride +  0], T11 );                   // [32 22 12 02]
    _mm_storeh_pi   ( (__m64*  )&pDst[3 * stride +  0], _mm_castsi128_ps(T11));  // [33 23 13 03]

    T10 = _mm_unpacklo_epi32(T04, T06);                                     // [71 61 51 41 70 60 50 40]
    T11 = _mm_unpackhi_epi32(T04, T06);                                     // [73 63 53 43 72 62 52 42]
    _mm_storel_epi64( (__m128i*)&pDst[0 * stride +  4], T10 );
    _mm_storeh_pi   ( (__m64*  )&pDst[1 * stride +  4], _mm_castsi128_ps(T10));
    _mm_storel_epi64( (__m128i*)&pDst[2 * stride +  4], T11 );
    _mm_storeh_pi   ( (__m64*  )&pDst[3 * stride +  4], _mm_castsi128_ps(T11));

    T10 = _mm_unpacklo_epi32(T01, T03);                                     // [35 25 15 05 34 24 14 04]
    T11 = _mm_unpackhi_epi32(T01, T03);                                     // [37 27 17 07 36 26 16 06]
    _mm_storel_epi64( (__m128i*)&pDst[4 * stride +  0], T10 );
    _mm_storeh_pi   ( (__m64*  )&pDst[5 * stride +  0], _mm_castsi128_ps(T10));
    _mm_storel_epi64( (__m128i*)&pDst[6 * stride +  0], T11 );
    _mm_storeh_pi   ( (__m64*  )&pDst[7 * stride +  0], _mm_castsi128_ps(T11));

    T10 = _mm_unpacklo_epi32(T05, T07);                                     // [75 65 55 45 74 64 54 44]
    T11 = _mm_unpackhi_epi32(T05, T07);                                     // [77 67 57 47 76 56 46 36]
    _mm_storel_epi64( (__m128i*)&pDst[4 * stride +  4], T10 );
    _mm_storeh_pi   ( (__m64*  )&pDst[5 * stride +  4], _mm_castsi128_ps(T10));
    _mm_storel_epi64( (__m128i*)&pDst[6 * stride +  4], T11 );
    _mm_storeh_pi   ( (__m64*  )&pDst[7 * stride +  4], _mm_castsi128_ps(T11));
}

ALIGN_VAR_32(static const int16_t, tab_idct_16x16_1[4][8][8] )=
{
    {/*1-3*/ /*2-6*/
        { 90,  87,  90,  87,  90,  87,  90,  87 },
        { 87,  57,  87,  57,  87,  57,  87,  57 },
        { 80,   9,  80,   9,  80,   9,  80,   9 },
        { 70, -43,  70, -43,  70, -43,  70, -43 },
        { 57, -80,  57, -80,  57, -80,  57, -80 },
        { 43, -90,  43, -90,  43, -90,  43, -90 },
        { 25, -70,  25, -70,  25, -70,  25, -70 },
        { 9,  -25,   9, -25,   9, -25,   9, -25 },
    },{ /*5-7*/ /*10-14*/
        {  80,  70,  80,  70,  80,  70,  80,  70 },
        {   9, -43,   9, -43,   9, -43,   9, -43 },
        { -70, -87, -70, -87, -70, -87, -70, -87 },
        { -87,   9, -87,   9, -87,   9, -87,   9 },
        { -25,  90, -25,  90, -25,  90, -25,  90 },
        {  57,  25,  57,  25,  57,  25,  57,  25 },
        {  90, -80,  90, -80,  90, -80,  90, -80 },
        {  43, -57,  43, -57,  43, -57,  43, -57 },
    },{ /*9-11*/ /*18-22*/
        {  57,  43,  57,  43,  57,  43,  57,  43 },
        { -80, -90, -80, -90, -80, -90, -80, -90 },
        { -25,  57, -25,  57, -25,  57, -25,  57 },
        {  90,  25,  90,  25,  90,  25,  90,  25 },
        {  -9,  -87, -9,  -87, -9,  -87, -9, -87 },
        { -87,  70, -87,  70, -87,  70, -87,  70 },
        {  43,   9,  43,   9,  43,   9,  43,   9 },
        {  70, -80,  70, -80,  70, -80,  70, -80 },
    },{/*13-15*/ /*  26-30   */
        {  25,   9,  25,   9,  25,   9,  25,   9 },
        { -70, -25, -70, -25, -70, -25, -70, -25 },
        {  90,  43,  90,  43,  90,  43,  90,  43 },
        { -80, -57, -80, -57, -80, -57, -80, -57 },
        {  43,  70,  43,  70,  43,  70,  43,  70 },
        {  9,  -80,   9, -80,   9, -80,   9, -80 },
        { -57,  87, -57,  87, -57,  87, -57,  87 },
        {  87, -90,  87, -90,  87, -90,  87, -90 },
    }
};

ALIGN_VAR_32(static const int16_t, tab_idct_16x16_2[2][4][8] )=
{
    { /*2-6*/ /*4-12*/
        { 89,  75,  89,  75, 89,  75, 89,  75 },
        { 75, -18,  75, -18, 75, -18, 75, -18 },
        { 50, -89,  50, -89, 50, -89, 50, -89 },
        { 18, -50,  18, -50, 18, -50, 18, -50 },
    },{ /*10-14*/  /*20-28*/
        {  50,  18,  50,  18,  50,  18,  50,  18 },
        { -89, -50, -89, -50, -89, -50, -89, -50 },
        {  18,  75,  18,  75,  18,  75,  18,  75 },
        {  75, -89,  75, -89,  75, -89,  75, -89 },
    }
};

ALIGN_VAR_32(static const int16_t, tab_idct_16x16_3[2][2][8] )=
{
    {/*4-12*/ /*8-24*/
        {  83,  36,  83,  36,  83,  36,  83,  36 },
        {  36, -83,  36, -83,  36, -83,  36, -83 },
    },{ /*0-8*/  /*0-16*/
        { 64,  64, 64,  64, 64,  64, 64,  64 },
        { 64, -64, 64, -64, 64, -64, 64, -64 },
    }
};

void xIDCT16(short *pSrc, short *pDst, intptr_t stride)
{
	const __m128i c16_p87_p90   = _mm_set1_epi32(0x0057005A);//row0 87high - 90low address
	const __m128i c16_p70_p80   = _mm_set1_epi32(0x00460050);
	const __m128i c16_p43_p57   = _mm_set1_epi32(0x002B0039);
	const __m128i c16_p09_p25   = _mm_set1_epi32(0x00090019);
	const __m128i c16_p57_p87   = _mm_set1_epi32(0x00390057);//row1
	const __m128i c16_n43_p09   = _mm_set1_epi32(0xFFD50009);
	const __m128i c16_n90_n80   = _mm_set1_epi32(0xFFA6FFB0);
	const __m128i c16_n25_n70   = _mm_set1_epi32(0xFFE7FFBA);
	const __m128i c16_p09_p80   = _mm_set1_epi32(0x00090050);//row2
	const __m128i c16_n87_n70   = _mm_set1_epi32(0xFFA9FFBA);
	const __m128i c16_p57_n25   = _mm_set1_epi32(0x0039FFE7);
	const __m128i c16_p43_p90   = _mm_set1_epi32(0x002B005A);
	const __m128i c16_n43_p70   = _mm_set1_epi32(0xFFD50046);//row3
	const __m128i c16_p09_n87   = _mm_set1_epi32(0x0009FFA9);
	const __m128i c16_p25_p90   = _mm_set1_epi32(0x0019005A);
	const __m128i c16_n57_n80   = _mm_set1_epi32(0xFFC7FFB0);
	const __m128i c16_n80_p57   = _mm_set1_epi32(0xFFB00039);//row4
	const __m128i c16_p90_n25   = _mm_set1_epi32(0x005AFFE7);
	const __m128i c16_n87_n09   = _mm_set1_epi32(0xFFA9FFF7);
	const __m128i c16_p70_p43   = _mm_set1_epi32(0x0046002B);
	const __m128i c16_n90_p43   = _mm_set1_epi32(0xFFA6002B);//row5
	const __m128i c16_p25_p57   = _mm_set1_epi32(0x00190039);
	const __m128i c16_p70_n87   = _mm_set1_epi32(0x0046FFA9);
	const __m128i c16_n80_p09   = _mm_set1_epi32(0xFFB00009);
	const __m128i c16_n70_p25   = _mm_set1_epi32(0xFFBA0019);//row6
	const __m128i c16_n80_p90   = _mm_set1_epi32(0xFFB0005A);
	const __m128i c16_p09_p43   = _mm_set1_epi32(0x0009002B);
	const __m128i c16_p87_n57   = _mm_set1_epi32(0x0057FFC7);
	const __m128i c16_n25_p09   = _mm_set1_epi32(0xFFE70009);//row7
	const __m128i c16_n57_p43   = _mm_set1_epi32(0xFFC7002B);
	const __m128i c16_n80_p70   = _mm_set1_epi32(0xFFB00046);
	const __m128i c16_n90_p87   = _mm_set1_epi32(0xFFA60057);
	
	const __m128i c16_p75_p89   = _mm_set1_epi32(0x004B0059);
	const __m128i c16_p18_p50   = _mm_set1_epi32(0x00120032);
	const __m128i c16_n18_p75   = _mm_set1_epi32(0xFFEE004B);
	const __m128i c16_n50_n89   = _mm_set1_epi32(0xFFCEFFA7);
	const __m128i c16_n89_p50   = _mm_set1_epi32(0xFFA70032);
	const __m128i c16_p75_p18   = _mm_set1_epi32(0x004B0012);
	const __m128i c16_n50_p18   = _mm_set1_epi32(0xFFCE0012);
	const __m128i c16_n89_p75   = _mm_set1_epi32(0xFFA7004B);

	const __m128i c16_p36_p83   = _mm_set1_epi32(0x00240053);
	const __m128i c16_n83_p36   = _mm_set1_epi32(0xFFAD0024);

	const __m128i c16_n64_p64   = _mm_set1_epi32(0xFFC00040);
	const __m128i c16_p64_p64   = _mm_set1_epi32(0x00400040);
	__m128i c32_rnd             = _mm_set1_epi32(64);

	int nShift = 7;

	// DCT1
	__m128i in00[2], in01[2], in02[2], in03[2], in04[2], in05[2], in06[2], in07[2];
	__m128i in08[2], in09[2], in10[2], in11[2], in12[2], in13[2], in14[2], in15[2];
	__m128i res00[2], res01[2], res02[2], res03[2], res04[2], res05[2], res06[2], res07[2];
	__m128i res08[2], res09[2], res10[2], res11[2], res12[2], res13[2], res14[2], res15[2];

	for (int i=0; i<2; i++){
		int offset = (i<<3);

		in00[i]  = _mm_loadu_si128((const __m128i*)&pSrc[ 0*16+offset]); // [07 06 05 04 03 02 01 00]
		in01[i]  = _mm_loadu_si128((const __m128i*)&pSrc[ 1*16+offset]); // [17 16 15 14 13 12 11 10]
		in02[i]  = _mm_loadu_si128((const __m128i*)&pSrc[ 2*16+offset]); // [27 26 25 24 23 22 21 20]
		in03[i]  = _mm_loadu_si128((const __m128i*)&pSrc[ 3*16+offset]); // [37 36 35 34 33 32 31 30]
		in04[i]  = _mm_loadu_si128((const __m128i*)&pSrc[ 4*16+offset]); // [47 46 45 44 43 42 41 40]
		in05[i]  = _mm_loadu_si128((const __m128i*)&pSrc[ 5*16+offset]); // [57 56 55 54 53 52 51 50]
		in06[i]  = _mm_loadu_si128((const __m128i*)&pSrc[ 6*16+offset]); // [67 66 65 64 63 62 61 60]
		in07[i]  = _mm_loadu_si128((const __m128i*)&pSrc[ 7*16+offset]); // [77 76 75 74 73 72 71 70]
		in08[i]  = _mm_loadu_si128((const __m128i*)&pSrc[ 8*16+offset]); // [x7~x0]row08
		in09[i]  = _mm_loadu_si128((const __m128i*)&pSrc[ 9*16+offset]); // [x7~x0]row09
		in10[i]  = _mm_loadu_si128((const __m128i*)&pSrc[10*16+offset]); // [x7~x0]row10
		in11[i]  = _mm_loadu_si128((const __m128i*)&pSrc[11*16+offset]); // [x7~x0]row11
		in12[i]  = _mm_loadu_si128((const __m128i*)&pSrc[12*16+offset]); // [x7~x0]row12
		in13[i]  = _mm_loadu_si128((const __m128i*)&pSrc[13*16+offset]); // [x7~x0]row13
		in14[i]  = _mm_loadu_si128((const __m128i*)&pSrc[14*16+offset]); // [x7~x0]row14
		in15[i]  = _mm_loadu_si128((const __m128i*)&pSrc[15*16+offset]); // [x7~x0]row15
	}
	

	for(Int pass=0; pass<2; pass++) {
		if (pass==1) {
			c32_rnd = _mm_set1_epi32(2048);
			nShift  = 12;  
		}

		for(Int part=0; part<2; part++){
			const __m128i T_00_00A = _mm_unpacklo_epi16(in01[part], in03[part]);             // [33 13 32 12 31 11 30 10]
			const __m128i T_00_00B = _mm_unpackhi_epi16(in01[part], in03[part]);             // [37 17 36 16 35 15 34 14]
			const __m128i T_00_01A = _mm_unpacklo_epi16(in05[part], in07[part]);             // [ ]
			const __m128i T_00_01B = _mm_unpackhi_epi16(in05[part], in07[part]);             // [ ]
			const __m128i T_00_02A = _mm_unpacklo_epi16(in09[part], in11[part]);             // [ ]
			const __m128i T_00_02B = _mm_unpackhi_epi16(in09[part], in11[part]);             // [ ]
			const __m128i T_00_03A = _mm_unpacklo_epi16(in13[part], in15[part]);             // [ ]
			const __m128i T_00_03B = _mm_unpackhi_epi16(in13[part], in15[part]);             // [ ]
			const __m128i T_00_04A = _mm_unpacklo_epi16(in02[part], in06[part]);             // [ ]
			const __m128i T_00_04B = _mm_unpackhi_epi16(in02[part], in06[part]);             // [ ]
			const __m128i T_00_05A = _mm_unpacklo_epi16(in10[part], in14[part]);             // [ ]
			const __m128i T_00_05B = _mm_unpackhi_epi16(in10[part], in14[part]);             // [ ]
			const __m128i T_00_06A = _mm_unpacklo_epi16(in04[part], in12[part]);             // [ ]row
			const __m128i T_00_06B = _mm_unpackhi_epi16(in04[part], in12[part]);             // [ ]
			const __m128i T_00_07A = _mm_unpacklo_epi16(in00[part], in08[part]);             // [83 03 82 02 81 01 81 00] row08 row00
			const __m128i T_00_07B = _mm_unpackhi_epi16(in00[part], in08[part]);             // [87 07 86 06 85 05 84 04]

			__m128i O0A, O1A, O2A, O3A, O4A, O5A, O6A, O7A;
			__m128i O0B, O1B, O2B, O3B, O4B, O5B, O6B, O7B;
			{
				__m128i T00, T01;
	#define COMPUTE_ROW(row0103, row0507, row0911, row1315, c0103, c0507, c0911, c1315, row)\
				T00 = _mm_add_epi32(_mm_madd_epi16(row0103, c0103), _mm_madd_epi16(row0507, c0507));\
				T01 = _mm_add_epi32(_mm_madd_epi16(row0911, c0911), _mm_madd_epi16(row1315, c1315));\
				row = _mm_add_epi32(T00, T01);
			
				COMPUTE_ROW(T_00_00A,T_00_01A,T_00_02A,T_00_03A, c16_p87_p90,c16_p70_p80,c16_p43_p57,c16_p09_p25, O0A)
				COMPUTE_ROW(T_00_00A,T_00_01A,T_00_02A,T_00_03A, c16_p57_p87,c16_n43_p09,c16_n90_n80,c16_n25_n70, O1A)
				COMPUTE_ROW(T_00_00A,T_00_01A,T_00_02A,T_00_03A, c16_p09_p80,c16_n87_n70,c16_p57_n25,c16_p43_p90, O2A)
				COMPUTE_ROW(T_00_00A,T_00_01A,T_00_02A,T_00_03A, c16_n43_p70,c16_p09_n87,c16_p25_p90,c16_n57_n80, O3A)
				COMPUTE_ROW(T_00_00A,T_00_01A,T_00_02A,T_00_03A, c16_n80_p57,c16_p90_n25,c16_n87_n09,c16_p70_p43, O4A)
				COMPUTE_ROW(T_00_00A,T_00_01A,T_00_02A,T_00_03A, c16_n90_p43,c16_p25_p57,c16_p70_n87,c16_n80_p09, O5A)
				COMPUTE_ROW(T_00_00A,T_00_01A,T_00_02A,T_00_03A, c16_n70_p25,c16_n80_p90,c16_p09_p43,c16_p87_n57, O6A)
				COMPUTE_ROW(T_00_00A,T_00_01A,T_00_02A,T_00_03A, c16_n25_p09,c16_n57_p43,c16_n80_p70,c16_n90_p87, O7A)

				COMPUTE_ROW(T_00_00B,T_00_01B,T_00_02B,T_00_03B, c16_p87_p90,c16_p70_p80,c16_p43_p57,c16_p09_p25, O0B)
				COMPUTE_ROW(T_00_00B,T_00_01B,T_00_02B,T_00_03B, c16_p57_p87,c16_n43_p09,c16_n90_n80,c16_n25_n70, O1B)
				COMPUTE_ROW(T_00_00B,T_00_01B,T_00_02B,T_00_03B, c16_p09_p80,c16_n87_n70,c16_p57_n25,c16_p43_p90, O2B)
				COMPUTE_ROW(T_00_00B,T_00_01B,T_00_02B,T_00_03B, c16_n43_p70,c16_p09_n87,c16_p25_p90,c16_n57_n80, O3B)
				COMPUTE_ROW(T_00_00B,T_00_01B,T_00_02B,T_00_03B, c16_n80_p57,c16_p90_n25,c16_n87_n09,c16_p70_p43, O4B)
				COMPUTE_ROW(T_00_00B,T_00_01B,T_00_02B,T_00_03B, c16_n90_p43,c16_p25_p57,c16_p70_n87,c16_n80_p09, O5B)
				COMPUTE_ROW(T_00_00B,T_00_01B,T_00_02B,T_00_03B, c16_n70_p25,c16_n80_p90,c16_p09_p43,c16_p87_n57, O6B)
				COMPUTE_ROW(T_00_00B,T_00_01B,T_00_02B,T_00_03B, c16_n25_p09,c16_n57_p43,c16_n80_p70,c16_n90_p87, O7B)
	#undef COMPUTE_ROW
			}

			__m128i EO0A, EO1A, EO2A, EO3A;
			__m128i EO0B, EO1B, EO2B, EO3B;
			EO0A = _mm_add_epi32(_mm_madd_epi16(T_00_04A, c16_p75_p89 ), _mm_madd_epi16(T_00_05A, c16_p18_p50 )); // EO0
			EO0B = _mm_add_epi32(_mm_madd_epi16(T_00_04B, c16_p75_p89 ), _mm_madd_epi16(T_00_05B, c16_p18_p50 ));
			EO1A = _mm_add_epi32(_mm_madd_epi16(T_00_04A, c16_n18_p75 ), _mm_madd_epi16(T_00_05A, c16_n50_n89 )); // EO1
			EO1B = _mm_add_epi32(_mm_madd_epi16(T_00_04B, c16_n18_p75 ), _mm_madd_epi16(T_00_05B, c16_n50_n89 ));
			EO2A = _mm_add_epi32(_mm_madd_epi16(T_00_04A, c16_n89_p50 ), _mm_madd_epi16(T_00_05A, c16_p75_p18 )); // EO2
			EO2B = _mm_add_epi32(_mm_madd_epi16(T_00_04B, c16_n89_p50 ), _mm_madd_epi16(T_00_05B, c16_p75_p18 ));
			EO3A = _mm_add_epi32(_mm_madd_epi16(T_00_04A, c16_n50_p18 ), _mm_madd_epi16(T_00_05A, c16_n89_p75 )); // EO3
			EO3B = _mm_add_epi32(_mm_madd_epi16(T_00_04B, c16_n50_p18 ), _mm_madd_epi16(T_00_05B, c16_n89_p75 ));

			__m128i EEO0A, EEO1A;
			__m128i EEO0B, EEO1B;
			EEO0A = _mm_madd_epi16(T_00_06A, c16_p36_p83 );
			EEO0B = _mm_madd_epi16(T_00_06B, c16_p36_p83 );
			EEO1A = _mm_madd_epi16(T_00_06A, c16_n83_p36 );
			EEO1B = _mm_madd_epi16(T_00_06B, c16_n83_p36 );

			__m128i EEE0A, EEE1A;
			__m128i EEE0B, EEE1B;
			EEE0A = _mm_madd_epi16(T_00_07A, c16_p64_p64 );
			EEE0B = _mm_madd_epi16(T_00_07B, c16_p64_p64 );
			EEE1A = _mm_madd_epi16(T_00_07A, c16_n64_p64 );
			EEE1B = _mm_madd_epi16(T_00_07B, c16_n64_p64 );

			const __m128i EE0A = _mm_add_epi32(EEE0A, EEO0A);                // EE0 = EEE0 + EEO0
			const __m128i EE0B = _mm_add_epi32(EEE0B, EEO0B);
			const __m128i EE1A = _mm_add_epi32(EEE1A, EEO1A);                // EE1 = EEE1 + EEO1
			const __m128i EE1B = _mm_add_epi32(EEE1B, EEO1B);
			const __m128i EE3A = _mm_sub_epi32(EEE0A, EEO0A);                // EE2 = EEE0 - EEO0
			const __m128i EE3B = _mm_sub_epi32(EEE0B, EEO0B);
			const __m128i EE2A = _mm_sub_epi32(EEE1A, EEO1A);                // EE3 = EEE1 - EEO1
			const __m128i EE2B = _mm_sub_epi32(EEE1B, EEO1B);

			const __m128i E0A = _mm_add_epi32(EE0A, EO0A);                // E0 = EE0 + EO0
			const __m128i E0B = _mm_add_epi32(EE0B, EO0B);
			const __m128i E1A = _mm_add_epi32(EE1A, EO1A);                // E1 = EE1 + EO1
			const __m128i E1B = _mm_add_epi32(EE1B, EO1B);
			const __m128i E2A = _mm_add_epi32(EE2A, EO2A);                // E2 = EE2 + EO2
			const __m128i E2B = _mm_add_epi32(EE2B, EO2B);
			const __m128i E3A = _mm_add_epi32(EE3A, EO3A);                // E3 = EE3 + EO3
			const __m128i E3B = _mm_add_epi32(EE3B, EO3B);
			const __m128i E7A = _mm_sub_epi32(EE0A, EO0A);                // E0 = EE0 - EO0
			const __m128i E7B = _mm_sub_epi32(EE0B, EO0B);
			const __m128i E6A = _mm_sub_epi32(EE1A, EO1A);                // E1 = EE1 - EO1
			const __m128i E6B = _mm_sub_epi32(EE1B, EO1B);
			const __m128i E5A = _mm_sub_epi32(EE2A, EO2A);                // E2 = EE2 - EO2
			const __m128i E5B = _mm_sub_epi32(EE2B, EO2B);
			const __m128i E4A = _mm_sub_epi32(EE3A, EO3A);                // E3 = EE3 - EO3
			const __m128i E4B = _mm_sub_epi32(EE3B, EO3B);

			const __m128i T10A = _mm_add_epi32(E0A, c32_rnd);               // E0 + rnd
			const __m128i T10B = _mm_add_epi32(E0B, c32_rnd);
			const __m128i T11A = _mm_add_epi32(E1A, c32_rnd);               // E1 + rnd
			const __m128i T11B = _mm_add_epi32(E1B, c32_rnd);
			const __m128i T12A = _mm_add_epi32(E2A, c32_rnd);               // E2 + rnd
			const __m128i T12B = _mm_add_epi32(E2B, c32_rnd);
			const __m128i T13A = _mm_add_epi32(E3A, c32_rnd);               // E3 + rnd
			const __m128i T13B = _mm_add_epi32(E3B, c32_rnd);
			const __m128i T14A = _mm_add_epi32(E4A, c32_rnd);               // E4 + rnd
			const __m128i T14B = _mm_add_epi32(E4B, c32_rnd);
			const __m128i T15A = _mm_add_epi32(E5A, c32_rnd);               // E5 + rnd
			const __m128i T15B = _mm_add_epi32(E5B, c32_rnd);
			const __m128i T16A = _mm_add_epi32(E6A, c32_rnd);               // E6 + rnd
			const __m128i T16B = _mm_add_epi32(E6B, c32_rnd);
			const __m128i T17A = _mm_add_epi32(E7A, c32_rnd);               // E7 + rnd
			const __m128i T17B = _mm_add_epi32(E7B, c32_rnd);
	
			const __m128i T20A = _mm_add_epi32(T10A, O0A);                // E0 + O0 + rnd
			const __m128i T20B = _mm_add_epi32(T10B, O0B);
			const __m128i T21A = _mm_add_epi32(T11A, O1A);                // E1 + O1 + rnd
			const __m128i T21B = _mm_add_epi32(T11B, O1B);
			const __m128i T22A = _mm_add_epi32(T12A, O2A);                // E2 + O2 + rnd
			const __m128i T22B = _mm_add_epi32(T12B, O2B);
			const __m128i T23A = _mm_add_epi32(T13A, O3A);                // E3 + O3 + rnd
			const __m128i T23B = _mm_add_epi32(T13B, O3B);
			const __m128i T24A = _mm_add_epi32(T14A, O4A);                // E4 
			const __m128i T24B = _mm_add_epi32(T14B, O4B);
			const __m128i T25A = _mm_add_epi32(T15A, O5A);                // E5 
			const __m128i T25B = _mm_add_epi32(T15B, O5B);
			const __m128i T26A = _mm_add_epi32(T16A, O6A);                // E6
			const __m128i T26B = _mm_add_epi32(T16B, O6B);
			const __m128i T27A = _mm_add_epi32(T17A, O7A);                // E7 
			const __m128i T27B = _mm_add_epi32(T17B, O7B);
			const __m128i T2FA = _mm_sub_epi32(T10A, O0A);                // E0 - O0 + rnd
			const __m128i T2FB = _mm_sub_epi32(T10B, O0B);
			const __m128i T2EA = _mm_sub_epi32(T11A, O1A);                // E1 - O1 + rnd
			const __m128i T2EB = _mm_sub_epi32(T11B, O1B);
			const __m128i T2DA = _mm_sub_epi32(T12A, O2A);                // E2 - O2 + rnd
			const __m128i T2DB = _mm_sub_epi32(T12B, O2B);
			const __m128i T2CA = _mm_sub_epi32(T13A, O3A);                // E3 - O3 + rnd
			const __m128i T2CB = _mm_sub_epi32(T13B, O3B);
			const __m128i T2BA = _mm_sub_epi32(T14A, O4A);                // E4
			const __m128i T2BB = _mm_sub_epi32(T14B, O4B);
			const __m128i T2AA = _mm_sub_epi32(T15A, O5A);                // E5
			const __m128i T2AB = _mm_sub_epi32(T15B, O5B);
			const __m128i T29A = _mm_sub_epi32(T16A, O6A);                // E6
			const __m128i T29B = _mm_sub_epi32(T16B, O6B);
			const __m128i T28A = _mm_sub_epi32(T17A, O7A);                // E7
			const __m128i T28B = _mm_sub_epi32(T17B, O7B);
		
			const __m128i T30A = _mm_srai_epi32(T20A, nShift);                   // [30 20 10 00]
			const __m128i T30B = _mm_srai_epi32(T20B, nShift);                   // [70 60 50 40]
			const __m128i T31A = _mm_srai_epi32(T21A, nShift);                   // [31 21 11 01]
			const __m128i T31B = _mm_srai_epi32(T21B, nShift);                   // [71 61 51 41]
			const __m128i T32A = _mm_srai_epi32(T22A, nShift);                   // [32 22 12 02]
			const __m128i T32B = _mm_srai_epi32(T22B, nShift);                   // [72 62 52 42]
			const __m128i T33A = _mm_srai_epi32(T23A, nShift);                   // [33 23 13 03]
			const __m128i T33B = _mm_srai_epi32(T23B, nShift);                   // [73 63 53 43]
			const __m128i T34A = _mm_srai_epi32(T24A, nShift);                   // [33 24 14 04]
			const __m128i T34B = _mm_srai_epi32(T24B, nShift);                   // [74 64 54 44]
			const __m128i T35A = _mm_srai_epi32(T25A, nShift);                   // [35 25 15 05]
			const __m128i T35B = _mm_srai_epi32(T25B, nShift);                   // [75 65 55 45]
			const __m128i T36A = _mm_srai_epi32(T26A, nShift);                   // [36 26 16 06]
			const __m128i T36B = _mm_srai_epi32(T26B, nShift);                   // [76 66 56 46]
			const __m128i T37A = _mm_srai_epi32(T27A, nShift);                   // [37 27 17 07]
			const __m128i T37B = _mm_srai_epi32(T27B, nShift);                   // [77 67 57 47]

			const __m128i T38A = _mm_srai_epi32(T28A, nShift);                   // [30 20 10 00] x8
			const __m128i T38B = _mm_srai_epi32(T28B, nShift);                   // [70 60 50 40]
			const __m128i T39A = _mm_srai_epi32(T29A, nShift);                   // [31 21 11 01] x9 
			const __m128i T39B = _mm_srai_epi32(T29B, nShift);                   // [71 61 51 41]
			const __m128i T3AA = _mm_srai_epi32(T2AA, nShift);                   // [32 22 12 02] xA
			const __m128i T3AB = _mm_srai_epi32(T2AB, nShift);                   // [72 62 52 42]
			const __m128i T3BA = _mm_srai_epi32(T2BA, nShift);                   // [33 23 13 03] xB
			const __m128i T3BB = _mm_srai_epi32(T2BB, nShift);                   // [73 63 53 43]
			const __m128i T3CA = _mm_srai_epi32(T2CA, nShift);                   // [33 24 14 04] xC
			const __m128i T3CB = _mm_srai_epi32(T2CB, nShift);                   // [74 64 54 44]
			const __m128i T3DA = _mm_srai_epi32(T2DA, nShift);                   // [35 25 15 05] xD
			const __m128i T3DB = _mm_srai_epi32(T2DB, nShift);                   // [75 65 55 45]
			const __m128i T3EA = _mm_srai_epi32(T2EA, nShift);                   // [36 26 16 06] xE
			const __m128i T3EB = _mm_srai_epi32(T2EB, nShift);                   // [76 66 56 46]
			const __m128i T3FA = _mm_srai_epi32(T2FA, nShift);                   // [37 27 17 07] xF
			const __m128i T3FB = _mm_srai_epi32(T2FB, nShift);                   // [77 67 57 47]

			res00[part]  = _mm_packs_epi32(T30A, T30B);              // [70 60 50 40 30 20 10 00]
			res01[part]  = _mm_packs_epi32(T31A, T31B);              // [71 61 51 41 31 21 11 01]
			res02[part]  = _mm_packs_epi32(T32A, T32B);              // [72 62 52 42 32 22 12 02]
			res03[part]  = _mm_packs_epi32(T33A, T33B);              // [73 63 53 43 33 23 13 03]
			res04[part]  = _mm_packs_epi32(T34A, T34B);              // [74 64 54 44 34 24 14 04]
			res05[part]  = _mm_packs_epi32(T35A, T35B);              // [75 65 55 45 35 25 15 05]
			res06[part]  = _mm_packs_epi32(T36A, T36B);              // [76 66 56 46 36 26 16 06]
			res07[part]  = _mm_packs_epi32(T37A, T37B);              // [77 67 57 47 37 27 17 07]

			res08[part]  = _mm_packs_epi32(T38A, T38B);              // [A0 ... 80]
			res09[part]  = _mm_packs_epi32(T39A, T39B);              // [A1 ... 81]
			res10[part]  = _mm_packs_epi32(T3AA, T3AB);              // [A2 ... 82]
			res11[part]  = _mm_packs_epi32(T3BA, T3BB);              // [A3 ... 83]
			res12[part]  = _mm_packs_epi32(T3CA, T3CB);              // [A4 ... 84]
			res13[part]  = _mm_packs_epi32(T3DA, T3DB);              // [A5 ... 85]
			res14[part]  = _mm_packs_epi32(T3EA, T3EB);              // [A6 ... 86]
			res15[part]  = _mm_packs_epi32(T3FA, T3FB);              // [A7 ... 87] 
		}
		//transpose matrix 8x8 16bit. 
		{
			__m128i tr0_0, tr0_1, tr0_2, tr0_3, tr0_4, tr0_5, tr0_6, tr0_7; 
			__m128i tr1_0, tr1_1, tr1_2, tr1_3, tr1_4, tr1_5, tr1_6, tr1_7; 
#define TRANSPOSE_8x8_16BIT(I0, I1, I2, I3, I4, I5, I6, I7, O0, O1, O2, O3, O4, O5, O6, O7)\
			tr0_0 = _mm_unpacklo_epi16(I0, I1);\
			tr0_1 = _mm_unpacklo_epi16(I2, I3);\
			tr0_2 = _mm_unpackhi_epi16(I0, I1);\
			tr0_3 = _mm_unpackhi_epi16(I2, I3);\
			tr0_4 = _mm_unpacklo_epi16(I4, I5);\
			tr0_5 = _mm_unpacklo_epi16(I6, I7);\
			tr0_6 = _mm_unpackhi_epi16(I4, I5);\
			tr0_7 = _mm_unpackhi_epi16(I6, I7);\
			tr1_0 = _mm_unpacklo_epi32(tr0_0, tr0_1);\
			tr1_1 = _mm_unpacklo_epi32(tr0_2, tr0_3);\
			tr1_2 = _mm_unpackhi_epi32(tr0_0, tr0_1);\
			tr1_3 = _mm_unpackhi_epi32(tr0_2, tr0_3);\
			tr1_4 = _mm_unpacklo_epi32(tr0_4, tr0_5);\
			tr1_5 = _mm_unpacklo_epi32(tr0_6, tr0_7);\
			tr1_6 = _mm_unpackhi_epi32(tr0_4, tr0_5);\
			tr1_7 = _mm_unpackhi_epi32(tr0_6, tr0_7);\
			O0 = _mm_unpacklo_epi64(tr1_0, tr1_4);\
			O1 = _mm_unpackhi_epi64(tr1_0, tr1_4);\
			O2 = _mm_unpacklo_epi64(tr1_2, tr1_6);\
			O3 = _mm_unpackhi_epi64(tr1_2, tr1_6);\
			O4 = _mm_unpacklo_epi64(tr1_1, tr1_5);\
			O5 = _mm_unpackhi_epi64(tr1_1, tr1_5);\
			O6 = _mm_unpacklo_epi64(tr1_3, tr1_7);\
			O7 = _mm_unpackhi_epi64(tr1_3, tr1_7);\

			TRANSPOSE_8x8_16BIT(res00[0],res01[0],res02[0],res03[0],res04[0],res05[0],res06[0],res07[0],in00[0],in01[0],in02[0],in03[0],in04[0],in05[0],in06[0],in07[0])
			TRANSPOSE_8x8_16BIT(res08[0],res09[0],res10[0],res11[0],res12[0],res13[0],res14[0],res15[0],in00[1],in01[1],in02[1],in03[1],in04[1],in05[1],in06[1],in07[1])
			TRANSPOSE_8x8_16BIT(res00[1],res01[1],res02[1],res03[1],res04[1],res05[1],res06[1],res07[1],in08[0],in09[0],in10[0],in11[0],in12[0],in13[0],in14[0],in15[0])
			TRANSPOSE_8x8_16BIT(res08[1],res09[1],res10[1],res11[1],res12[1],res13[1],res14[1],res15[1],in08[1],in09[1],in10[1],in11[1],in12[1],in13[1],in14[1],in15[1])

#undef TRANSPOSE_8x8_16BIT
		}
	}

	_mm_store_si128( (__m128i *)&pDst[ 0*stride+0], in00[0]);
	_mm_store_si128( (__m128i *)&pDst[ 0*stride+8], in00[1]);
	_mm_store_si128( (__m128i *)&pDst[ 1*stride+0], in01[0]);
	_mm_store_si128( (__m128i *)&pDst[ 1*stride+8], in01[1]);
	_mm_store_si128( (__m128i *)&pDst[ 2*stride+0], in02[0]);
	_mm_store_si128( (__m128i *)&pDst[ 2*stride+8], in02[1]);
	_mm_store_si128( (__m128i *)&pDst[ 3*stride+0], in03[0]);
	_mm_store_si128( (__m128i *)&pDst[ 3*stride+8], in03[1]);
	_mm_store_si128( (__m128i *)&pDst[ 4*stride+0], in04[0]);
	_mm_store_si128( (__m128i *)&pDst[ 4*stride+8], in04[1]);
	_mm_store_si128( (__m128i *)&pDst[ 5*stride+0], in05[0]);
	_mm_store_si128( (__m128i *)&pDst[ 5*stride+8], in05[1]);
	_mm_store_si128( (__m128i *)&pDst[ 6*stride+0], in06[0]);
	_mm_store_si128( (__m128i *)&pDst[ 6*stride+8], in06[1]);
	_mm_store_si128( (__m128i *)&pDst[ 7*stride+0], in07[0]);
	_mm_store_si128( (__m128i *)&pDst[ 7*stride+8], in07[1]);
	_mm_store_si128( (__m128i *)&pDst[ 8*stride+0], in08[0]);
	_mm_store_si128( (__m128i *)&pDst[ 8*stride+8], in08[1]);
	_mm_store_si128( (__m128i *)&pDst[ 9*stride+0], in09[0]);
	_mm_store_si128( (__m128i *)&pDst[ 9*stride+8], in09[1]);
	_mm_store_si128( (__m128i *)&pDst[10*stride+0], in10[0]);
	_mm_store_si128( (__m128i *)&pDst[10*stride+8], in10[1]);
	_mm_store_si128( (__m128i *)&pDst[11*stride+0], in11[0]);
	_mm_store_si128( (__m128i *)&pDst[11*stride+8], in11[1]);
	_mm_store_si128( (__m128i *)&pDst[12*stride+0], in12[0]);
	_mm_store_si128( (__m128i *)&pDst[12*stride+8], in12[1]);
	_mm_store_si128( (__m128i *)&pDst[13*stride+0], in13[0]);
	_mm_store_si128( (__m128i *)&pDst[13*stride+8], in13[1]);
	_mm_store_si128( (__m128i *)&pDst[14*stride+0], in14[0]);
	_mm_store_si128( (__m128i *)&pDst[14*stride+8], in14[1]);
	_mm_store_si128( (__m128i *)&pDst[15*stride+0], in15[0]);
	_mm_store_si128( (__m128i *)&pDst[15*stride+8], in15[1]);
}

ALIGN_VAR_32(static const short, tab_idct_32x32[8][16][8] )=
{
    { /*   1-3     */
        { 90,  90, 90,  90, 90,  90, 90,  90 },
        { 90,  82, 90,  82, 90,  82, 90,  82 },
        { 88,  67, 88,  67, 88,  67, 88,  67 },
        { 85,  46, 85,  46, 85,  46, 85,  46 },
        { 82,  22, 82,  22, 82,  22, 82,  22 },
        { 78,  -4, 78,  -4, 78,  -4, 78,  -4 },
        { 73, -31, 73, -31, 73, -31, 73, -31 },
        { 67, -54, 67, -54, 67, -54, 67, -54 },
        { 61, -73, 61, -73, 61, -73, 61, -73 },
        { 54, -85, 54, -85, 54, -85, 54, -85 },
        { 46, -90, 46, -90, 46, -90, 46, -90 },
        { 38, -88, 38, -88, 38, -88, 38, -88 },
        { 31, -78, 31, -78, 31, -78, 31, -78 },
        { 22, -61, 22, -61, 22, -61, 22, -61 },
        { 13, -38, 13, -38, 13, -38, 13, -38 },
        { 4,  -13,  4, -13,  4, -13,  4, -13 },
    },{/*  5-7 */
        {  88,  85,  88,  85,  88,  85,  88,  85 },
        {  67,  46,  67,  46,  67,  46,  67,  46 },
        {  31, -13,  31, -13,  31, -13,  31, -13 },
        { -13, -67, -13, -67, -13, -67, -13, -67 },
        { -54, -90, -54, -90, -54, -90, -54, -90 },
        { -82, -73, -82, -73, -82, -73, -82, -73 },
        { -90, -22, -90, -22, -90, -22, -90, -22 },
        { -78,  38, -78,  38, -78,  38, -78,  38 },
        { -46,  82, -46,  82, -46,  82, -46,  82 },
        {  -4,  88,  -4,  88,  -4,  88,  -4,  88 },
        {  38,  54,  38,  54,  38,  54,  38,  54 },
        {  73,  -4,  73,  -4,  73,  -4,  73,  -4 },
        {  90, -61,  90, -61,  90, -61,  90, -61 },
        {  85, -90,  85, -90,  85, -90,  85, -90 },
        {  61, -78,  61, -78,  61, -78,  61, -78 },
        {  22, -31,  22, -31,  22, -31,  22, -31 },
    },{/*  9-11   */
        {  82,  78,  82,  78,  82,  78,  82,  78 },
        {  22,  -4,  22,  -4,  22,  -4,  22,  -4 },
        { -54, -82, -54, -82, -54, -82, -54, -82 },
        { -90, -73, -90, -73, -90, -73, -90, -73 },
        { -61,  13, -61,  13, -61,  13, -61,  13 },
        {  13,  85,  13,  85,  13,  85,  13,  85 },
        {  78,  67,  78,  67,  78,  67,  78,  67 },
        {  85, -22,  85, -22,  85, -22,  85, -22 },
        {  31, -88,  31, -88,  31, -88,  31, -88 },
        { -46, -61, -46, -61, -46, -61, -46, -61 },
        { -90,  31, -90,  31, -90,  31, -90,  31 },
        { -67,  90, -67,  90, -67,  90, -67,  90 },
        {   4,  54,   4,  54,   4,  54,   4,  54 },
        {  73, -38,  73, -38,  73, -38,  73, -38 },
        {  88, -90,  88, -90,  88, -90,  88, -90 },
        {  38, -46,  38, -46,  38, -46,  38, -46 },
    },{/*  13-15   */
        {  73,  67,  73,  67,  73,  67,  73,  67 },
        { -31, -54, -31, -54, -31, -54, -31, -54 },
        { -90, -78, -90, -78, -90, -78, -90, -78 },
        { -22,  38, -22,  38, -22,  38, -22,  38 },
        {  78,  85,  78,  85,  78,  85,  78,  85 },
        {  67, -22,  67, -22,  67, -22,  67, -22 },
        { -38, -90, -38, -90, -38, -90, -38, -90 },
        { -90,   4, -90,   4, -90,   4, -90,   4 },
        { -13,  90, -13,  90, -13,  90, -13,  90 },
        {  82,  13,  82,  13,  82,  13,  82,  13 },
        {  61, -88,  61, -88,  61, -88,  61, -88 },
        { -46, -31, -46, -31, -46, -31, -46, -31 },
        { -88,  82, -88,  82, -88,  82, -88,  82 },
        { -4,   46, -4,   46, -4,   46, -4,   46 },
        {  85, -73,  85, -73,  85, -73,  85, -73 },
        {  54, -61,  54, -61,  54, -61,  54, -61 },
    },{/*  17-19   */
        {  61,  54,  61,  54,  61,  54,  61,  54 },
        { -73, -85, -73, -85, -73, -85, -73, -85 },
        { -46,  -4, -46,  -4, -46,  -4, -46,  -4 },
        {  82,  88,  82,  88,  82,  88,  82,  88 },
        {  31, -46,  31, -46,  31, -46,  31, -46 },
        { -88, -61, -88, -61, -88, -61, -88, -61 },
        { -13,  82, -13,  82, -13,  82, -13,  82 },
        {  90,  13,  90,  13,  90,  13,  90,  13 },
        { -4, -90,  -4, -90,  -4, -90,  -4, -90 },
        { -90,  38, -90,  38, -90,  38, -90,  38 },
        {  22,  67,  22,  67,  22,  67,  22,  67 },
        {  85, -78,  85, -78,  85, -78,  85, -78 },
        { -38, -22, -38, -22, -38, -22, -38, -22 },
        { -78,  90, -78,  90, -78,  90, -78,  90 },
        {  54, -31,  54, -31,  54, -31,  54, -31 },
        {  67, -73,  67, -73,  67, -73,  67, -73 },
    },{ /*  21-23   */
        {  46,  38,  46,  38,  46,  38,  46,  38 },
        { -90, -88, -90, -88, -90, -88, -90, -88 },
        {  38,  73,  38,  73,  38,  73,  38,  73 },
        {  54,  -4,  54,  -4,  54,  -4,  54,  -4 },
        { -90, -67, -90, -67, -90, -67, -90, -67 },
        {  31,  90,  31,  90,  31,  90,  31,  90 },
        {  61, -46,  61, -46,  61, -46,  61, -46 },
        { -88, -31, -88, -31, -88, -31, -88, -31 },
        {  22,  85,  22,  85,  22,  85,  22,  85 },
        {  67, -78,  67, -78,  67, -78,  67, -78 },
        { -85,  13, -85,  13, -85,  13, -85,  13 },
        {  13,  61,  13,  61,  13,  61,  13,  61 },
        {  73, -90,  73, -90,  73, -90,  73, -90 },
        { -82,  54, -82,  54, -82,  54, -82,  54 },
        {   4,  22,   4,  22,   4,  22,   4,  22 },
        {  78, -82,  78, -82,  78, -82,  78, -82 },
    },{ /*  25-27   */
        {  31,  22,  31,  22,  31,  22,  31,  22 },
        { -78, -61, -78, -61, -78, -61, -78, -61 },
        {  90,  85,  90,  85,  90,  85,  90,  85 },
        { -61, -90, -61, -90, -61, -90, -61, -90 },
        {   4,  73,   4,  73,   4,  73,   4,  73 },
        {  54, -38,  54, -38,  54, -38,  54, -38 },
        { -88,  -4, -88,  -4, -88,  -4, -88,  -4 },
        {  82,  46,  82,  46,  82,  46,  82,  46 },
        { -38, -78, -38, -78, -38, -78, -38, -78 },
        { -22,  90, -22,  90, -22,  90, -22,  90 },
        {  73, -82,  73, -82,  73, -82,  73, -82 },
        { -90,  54, -90,  54, -90,  54, -90,  54 },
        {  67, -13,  67, -13,  67, -13,  67, -13 },
        { -13, -31, -13, -31, -13, -31, -13, -31 },
        { -46,  67, -46,  67, -46,  67, -46,  67 },
        {  85, -88,  85, -88,  85, -88,  85, -88 },
    },{/*  29-31   */
        {  13,   4,  13,   4,  13,   4,  13,   4 },
        { -38, -13, -38, -13, -38, -13, -38, -13 },
        {  61,  22,  61,  22,  61,  22,  61,  22 },
        { -78, -31, -78, -31, -78, -31, -78, -31 },
        {  88,  38,  88,  38,  88,  38,  88,  38 },
        { -90, -46, -90, -46, -90, -46, -90, -46 },
        {  85,  54,  85,  54,  85,  54,  85,  54 },
        { -73, -61, -73, -61, -73, -61, -73, -61 },
        {  54,  67,  54,  67,  54,  67,  54,  67 },
        { -31, -73, -31, -73, -31, -73, -31, -73 },
        {   4,  78,   4,  78,   4,  78,   4,  78 },
        {  22, -82,  22, -82,  22, -82,  22, -82 },
        { -46,  85, -46,  85, -46,  85, -46,  85 },
        {  67, -88,  67, -88,  67, -88,  67, -88 },
        { -82,  90, -82,  90, -82,  90, -82,  90 },
        {  90, -90,  90, -90,  90, -90,  90, -90 },
    }
};

void xIDCT32(short *pSrc, short *pDst, intptr_t stride)
{
    int i,j;
    int shift;

    __m128i m128iS0, m128iS1, m128iS2, m128iS3, m128iS4, m128iS5, m128iS6, m128iS7, m128iS8, m128iS9, m128iS10, m128iS11, m128iS12, m128iS13, m128iS14, m128iS15 ,  m128iAdd, m128Tmp0,     m128Tmp1,m128Tmp2, m128Tmp3, m128Tmp4, m128Tmp5,m128Tmp6, m128Tmp7, E0h, E1h, E2h, E3h, E0l, E1l, E2l, E3l, O0h, O1h, O2h, O3h, O4h, O5h, O6h, O7h,O0l, O1l, O2l, O3l, O4l, O5l, O6l, O7l,EE0l, EE1l, EE2l, EE3l, E00l, E01l, EE0h, EE1h, EE2h, EE3h,E00h, E01h;
    __m128i E4l, E5l, E6l, E7l, E8l, E9l, E10l, E11l, E12l, E13l, E14l, E15l;
    __m128i E4h, E5h, E6h, E7h, E8h, E9h, E10h, E11h, E12h, E13h, E14h, E15h, EEE0l, EEE1l, EEE0h, EEE1h;
    __m128i m128iS16, m128iS17, m128iS18, m128iS19, m128iS20, m128iS21, m128iS22, m128iS23, m128iS24, m128iS25, m128iS26, m128iS27, m128iS28, m128iS29, m128iS30, m128iS31, m128Tmp8, m128Tmp9, m128Tmp10, m128Tmp11, m128Tmp12, m128Tmp13, m128Tmp14, m128Tmp15, O8h, O9h, O10h, O11h, O12h, O13h, O14h, O15h,O8l, O9l, O10l, O11l, O12l, O13l, O14l, O15l, E02l, E02h, E03l, E03h, EE7l, EE6l, EE5l, EE4l, EE7h, EE6h, EE5h, EE4h;
    m128iS0   = _mm_load_si128( (__m128i*)( pSrc       ) );
    m128iS1   = _mm_load_si128( (__m128i*)( pSrc +  32 ) );
    m128iS2   = _mm_load_si128( (__m128i*)( pSrc +  64 ) );
    m128iS3   = _mm_load_si128( (__m128i*)( pSrc +  96 ) );
    m128iS4   = _mm_loadu_si128((__m128i*)( pSrc + 128 ) );
    m128iS5   = _mm_load_si128( (__m128i*)( pSrc + 160 ) );
    m128iS6   = _mm_load_si128( (__m128i*)( pSrc + 192 ) );
    m128iS7   = _mm_load_si128( (__m128i*)( pSrc + 224 ) );
    m128iS8   = _mm_load_si128( (__m128i*)( pSrc + 256 ) );
    m128iS9   = _mm_load_si128( (__m128i*)( pSrc + 288 ) );
    m128iS10  = _mm_load_si128( (__m128i*)( pSrc + 320 ) );
    m128iS11  = _mm_load_si128( (__m128i*)( pSrc + 352 ) );
    m128iS12  = _mm_loadu_si128((__m128i*)( pSrc + 384 ) );
    m128iS13  = _mm_load_si128( (__m128i*)( pSrc + 416 ) );
    m128iS14  = _mm_load_si128( (__m128i*)( pSrc + 448 ) );
    m128iS15  = _mm_load_si128( (__m128i*)( pSrc + 480 ) );
    m128iS16  = _mm_load_si128( (__m128i*)( pSrc + 512 ) );
    m128iS17  = _mm_load_si128( (__m128i*)( pSrc + 544 ) );
    m128iS18  = _mm_load_si128( (__m128i*)( pSrc + 576 ) );
    m128iS19  = _mm_load_si128( (__m128i*)( pSrc + 608 ) );
    m128iS20  = _mm_load_si128( (__m128i*)( pSrc + 640 ) );
    m128iS21  = _mm_load_si128( (__m128i*)( pSrc + 672 ) );
    m128iS22  = _mm_load_si128( (__m128i*)( pSrc + 704 ) );
    m128iS23  = _mm_load_si128( (__m128i*)( pSrc + 736 ) );
    m128iS24  = _mm_load_si128( (__m128i*)( pSrc + 768 ) );
    m128iS25  = _mm_load_si128( (__m128i*)( pSrc + 800 ) );
    m128iS26  = _mm_load_si128( (__m128i*)( pSrc + 832 ) );
    m128iS27  = _mm_load_si128( (__m128i*)( pSrc + 864 ) );
    m128iS28  = _mm_load_si128( (__m128i*)( pSrc + 896 ) );
    m128iS29  = _mm_load_si128( (__m128i*)( pSrc + 928 ) );
    m128iS30  = _mm_load_si128( (__m128i*)( pSrc + 960 ) );
    m128iS31  = _mm_load_si128( (__m128i*)( pSrc + 992 ) );

    shift = 7;
    m128iAdd  = _mm_set1_epi32( 64 );

    for(j=0; j< 2; j++) {
        for(i=0; i < 32; i+=8) {
            m128Tmp0 = _mm_unpacklo_epi16(  m128iS1, m128iS3 );
            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][0] ) ) );
            m128Tmp1 = _mm_unpackhi_epi16(  m128iS1, m128iS3 );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][0] ) ) );


            m128Tmp2 =  _mm_unpacklo_epi16(  m128iS5, m128iS7 );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][0] ) ) );
            m128Tmp3 = _mm_unpackhi_epi16(  m128iS5, m128iS7 );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][0] ) ) );


            m128Tmp4 =  _mm_unpacklo_epi16(  m128iS9, m128iS11 );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][0] ) ) );
            m128Tmp5 = _mm_unpackhi_epi16(  m128iS9, m128iS11 );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][0] ) ) );


            m128Tmp6 =  _mm_unpacklo_epi16(  m128iS13, m128iS15 );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][0] ) ) );
            m128Tmp7 = _mm_unpackhi_epi16(  m128iS13, m128iS15 );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][0] ) ) );

            m128Tmp8 =  _mm_unpacklo_epi16(  m128iS17, m128iS19 );
            E4l = _mm_madd_epi16( m128Tmp8, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][0] ) ) );
            m128Tmp9 = _mm_unpackhi_epi16(  m128iS17, m128iS19 );
            E4h = _mm_madd_epi16( m128Tmp9, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][0] ) ) );

            m128Tmp10 =  _mm_unpacklo_epi16(  m128iS21, m128iS23 );
            E5l = _mm_madd_epi16( m128Tmp10, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][0] ) ) );
            m128Tmp11 = _mm_unpackhi_epi16(  m128iS21, m128iS23 );
            E5h = _mm_madd_epi16( m128Tmp11, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][0] ) ) );

            m128Tmp12 =  _mm_unpacklo_epi16(  m128iS25, m128iS27 );
            E6l = _mm_madd_epi16( m128Tmp12, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][0] ) ) );
            m128Tmp13 = _mm_unpackhi_epi16(  m128iS25, m128iS27 );
            E6h = _mm_madd_epi16( m128Tmp13, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][0] ) ) );

            m128Tmp14 =  _mm_unpacklo_epi16(  m128iS29, m128iS31 );
            E7l = _mm_madd_epi16( m128Tmp14, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][0] ) ) );
            m128Tmp15 = _mm_unpackhi_epi16(  m128iS29, m128iS31 );
            E7h = _mm_madd_epi16( m128Tmp15, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][0] ) ) );


            O0l = _mm_add_epi32(E0l, E1l);
            O0l = _mm_add_epi32(O0l, E2l);
            O0l = _mm_add_epi32(O0l, E3l);
            O0l = _mm_add_epi32(O0l, E4l);
            O0l = _mm_add_epi32(O0l, E5l);
            O0l = _mm_add_epi32(O0l, E6l);
            O0l = _mm_add_epi32(O0l, E7l);


            O0h = _mm_add_epi32(E0h, E1h);
            O0h = _mm_add_epi32(O0h, E2h);
            O0h = _mm_add_epi32(O0h, E3h);
            O0h = _mm_add_epi32(O0h, E4h);
            O0h = _mm_add_epi32(O0h, E5h);
            O0h = _mm_add_epi32(O0h, E6h);
            O0h = _mm_add_epi32(O0h, E7h);


            /* Compute O1*/
            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][1] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][1] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][1] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][1] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][1] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][1] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][1] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][1] ) ) );

            E4l = _mm_madd_epi16( m128Tmp8, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][1] ) ) );
            E4h = _mm_madd_epi16( m128Tmp9, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][1] ) ) );
            E5l = _mm_madd_epi16( m128Tmp10, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][1] ) ) );
            E5h = _mm_madd_epi16( m128Tmp11, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][1] ) ) );
            E6l = _mm_madd_epi16( m128Tmp12, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][1] ) ) );
            E6h = _mm_madd_epi16( m128Tmp13, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][1] ) ) );
            E7l = _mm_madd_epi16( m128Tmp14, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][1] ) ) );
            E7h = _mm_madd_epi16( m128Tmp15, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][1] ) ) );




            O1l = _mm_add_epi32(E0l, E1l);
            O1l = _mm_add_epi32(O1l, E2l);
            O1l = _mm_add_epi32(O1l, E3l);
            O1l = _mm_add_epi32(O1l, E4l);
            O1l = _mm_add_epi32(O1l, E5l);
            O1l = _mm_add_epi32(O1l, E6l);
            O1l = _mm_add_epi32(O1l, E7l);

            O1h = _mm_add_epi32(E0h, E1h);
            O1h = _mm_add_epi32(O1h, E2h);
            O1h = _mm_add_epi32(O1h, E3h);
            O1h = _mm_add_epi32(O1h, E4h);
            O1h = _mm_add_epi32(O1h, E5h);
            O1h = _mm_add_epi32(O1h, E6h);
            O1h = _mm_add_epi32(O1h, E7h);
            /* Compute O2*/
            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][2] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][2] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][2] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][2] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][2] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][2] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][2] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][2] ) ) );

            E4l = _mm_madd_epi16( m128Tmp8, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][2] ) ) );
            E4h = _mm_madd_epi16( m128Tmp9, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][2] ) ) );
            E5l = _mm_madd_epi16( m128Tmp10, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][2] ) ) );
            E5h = _mm_madd_epi16( m128Tmp11, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][2] ) ) );
            E6l = _mm_madd_epi16( m128Tmp12, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][2] ) ) );
            E6h = _mm_madd_epi16( m128Tmp13, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][2] ) ) );
            E7l = _mm_madd_epi16( m128Tmp14, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][2] ) ) );
            E7h = _mm_madd_epi16( m128Tmp15, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][2] ) ) );


            O2l = _mm_add_epi32(E0l, E1l);
            O2l = _mm_add_epi32(O2l, E2l);
            O2l = _mm_add_epi32(O2l, E3l);
            O2l = _mm_add_epi32(O2l, E4l);
            O2l = _mm_add_epi32(O2l, E5l);
            O2l = _mm_add_epi32(O2l, E6l);
            O2l = _mm_add_epi32(O2l, E7l);

            O2h = _mm_add_epi32(E0h, E1h);
            O2h = _mm_add_epi32(O2h, E2h);
            O2h = _mm_add_epi32(O2h, E3h);
            O2h = _mm_add_epi32(O2h, E4h);
            O2h = _mm_add_epi32(O2h, E5h);
            O2h = _mm_add_epi32(O2h, E6h);
            O2h = _mm_add_epi32(O2h, E7h);
            /* Compute O3*/
            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][3] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][3] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][3] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][3] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][3] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][3] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][3] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][3] ) ) );


            E4l = _mm_madd_epi16( m128Tmp8, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][3] ) ) );
            E4h = _mm_madd_epi16( m128Tmp9, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][3] ) ) );
            E5l = _mm_madd_epi16( m128Tmp10, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][3] ) ) );
            E5h = _mm_madd_epi16( m128Tmp11, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][3] ) ) );
            E6l = _mm_madd_epi16( m128Tmp12, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][3] ) ) );
            E6h = _mm_madd_epi16( m128Tmp13, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][3] ) ) );
            E7l = _mm_madd_epi16( m128Tmp14, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][3] ) ) );
            E7h = _mm_madd_epi16( m128Tmp15, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][3] ) ) );


            O3l = _mm_add_epi32(E0l, E1l);
            O3l = _mm_add_epi32(O3l, E2l);
            O3l = _mm_add_epi32(O3l, E3l);
            O3l = _mm_add_epi32(O3l, E4l);
            O3l = _mm_add_epi32(O3l, E5l);
            O3l = _mm_add_epi32(O3l, E6l);
            O3l = _mm_add_epi32(O3l, E7l);

            O3h = _mm_add_epi32(E0h, E1h);
            O3h = _mm_add_epi32(O3h, E2h);
            O3h = _mm_add_epi32(O3h, E3h);
            O3h = _mm_add_epi32(O3h, E4h);
            O3h = _mm_add_epi32(O3h, E5h);
            O3h = _mm_add_epi32(O3h, E6h);
            O3h = _mm_add_epi32(O3h, E7h);
            /* Compute O4*/

            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][4] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][4] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][4] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][4] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][4] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][4] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][4] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][4] ) ) );


            E4l = _mm_madd_epi16( m128Tmp8, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][4] ) ) );
            E4h = _mm_madd_epi16( m128Tmp9, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][4] ) ) );
            E5l = _mm_madd_epi16( m128Tmp10, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][4] ) ) );
            E5h = _mm_madd_epi16( m128Tmp11, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][4] ) ) );
            E6l = _mm_madd_epi16( m128Tmp12, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][4] ) ) );
            E6h = _mm_madd_epi16( m128Tmp13, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][4] ) ) );
            E7l = _mm_madd_epi16( m128Tmp14, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][4] ) ) );
            E7h = _mm_madd_epi16( m128Tmp15, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][4] ) ) );


            O4l = _mm_add_epi32(E0l, E1l);
            O4l = _mm_add_epi32(O4l, E2l);
            O4l = _mm_add_epi32(O4l, E3l);
            O4l = _mm_add_epi32(O4l, E4l);
            O4l = _mm_add_epi32(O4l, E5l);
            O4l = _mm_add_epi32(O4l, E6l);
            O4l = _mm_add_epi32(O4l, E7l);

            O4h = _mm_add_epi32(E0h, E1h);
            O4h = _mm_add_epi32(O4h, E2h);
            O4h = _mm_add_epi32(O4h, E3h);
            O4h = _mm_add_epi32(O4h, E4h);
            O4h = _mm_add_epi32(O4h, E5h);
            O4h = _mm_add_epi32(O4h, E6h);
            O4h = _mm_add_epi32(O4h, E7h);


            /* Compute O5*/
            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][5] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][5] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][5] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][5] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][5] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][5] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][5] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][5] ) ) );


            E4l = _mm_madd_epi16( m128Tmp8, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][5] ) ) );
            E4h = _mm_madd_epi16( m128Tmp9, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][5] ) ) );
            E5l = _mm_madd_epi16( m128Tmp10, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][5] ) ) );
            E5h = _mm_madd_epi16( m128Tmp11, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][5] ) ) );
            E6l = _mm_madd_epi16( m128Tmp12, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][5] ) ) );
            E6h = _mm_madd_epi16( m128Tmp13, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][5] ) ) );
            E7l = _mm_madd_epi16( m128Tmp14, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][5] ) ) );
            E7h = _mm_madd_epi16( m128Tmp15, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][5] ) ) );


            O5l = _mm_add_epi32(E0l, E1l);
            O5l = _mm_add_epi32(O5l, E2l);
            O5l = _mm_add_epi32(O5l, E3l);
            O5l = _mm_add_epi32(O5l, E4l);
            O5l = _mm_add_epi32(O5l, E5l);
            O5l = _mm_add_epi32(O5l, E6l);
            O5l = _mm_add_epi32(O5l, E7l);

            O5h = _mm_add_epi32(E0h, E1h);
            O5h = _mm_add_epi32(O5h, E2h);
            O5h = _mm_add_epi32(O5h, E3h);
            O5h = _mm_add_epi32(O5h, E4h);
            O5h = _mm_add_epi32(O5h, E5h);
            O5h = _mm_add_epi32(O5h, E6h);
            O5h = _mm_add_epi32(O5h, E7h);

            /* Compute O6*/

            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][6] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][6] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][6] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][6] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][6] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][6] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][6] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][6] ) ) );


            E4l = _mm_madd_epi16( m128Tmp8, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][6] ) ) );
            E4h = _mm_madd_epi16( m128Tmp9, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][6] ) ) );
            E5l = _mm_madd_epi16( m128Tmp10, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][6] ) ) );
            E5h = _mm_madd_epi16( m128Tmp11, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][6] ) ) );
            E6l = _mm_madd_epi16( m128Tmp12, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][6] ) ) );
            E6h = _mm_madd_epi16( m128Tmp13, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][6] ) ) );
            E7l = _mm_madd_epi16( m128Tmp14, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][6] ) ) );
            E7h = _mm_madd_epi16( m128Tmp15, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][6] ) ) );


            O6l = _mm_add_epi32(E0l, E1l);
            O6l = _mm_add_epi32(O6l, E2l);
            O6l = _mm_add_epi32(O6l, E3l);
            O6l = _mm_add_epi32(O6l, E4l);
            O6l = _mm_add_epi32(O6l, E5l);
            O6l = _mm_add_epi32(O6l, E6l);
            O6l = _mm_add_epi32(O6l, E7l);

            O6h = _mm_add_epi32(E0h, E1h);
            O6h = _mm_add_epi32(O6h, E2h);
            O6h = _mm_add_epi32(O6h, E3h);
            O6h = _mm_add_epi32(O6h, E4h);
            O6h = _mm_add_epi32(O6h, E5h);
            O6h = _mm_add_epi32(O6h, E6h);
            O6h = _mm_add_epi32(O6h, E7h);

            /* Compute O7*/

            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][7] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][7] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][7] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][7] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][7] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][7] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][7] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][7] ) ) );


            E4l = _mm_madd_epi16( m128Tmp8, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][7] ) ) );
            E4h = _mm_madd_epi16( m128Tmp9, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][7] ) ) );
            E5l = _mm_madd_epi16( m128Tmp10, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][7] ) ) );
            E5h = _mm_madd_epi16( m128Tmp11, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][7] ) ) );
            E6l = _mm_madd_epi16( m128Tmp12, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][7] ) ) );
            E6h = _mm_madd_epi16( m128Tmp13, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][7] ) ) );
            E7l = _mm_madd_epi16( m128Tmp14, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][7] ) ) );
            E7h = _mm_madd_epi16( m128Tmp15, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][7] ) ) );


            O7l = _mm_add_epi32(E0l, E1l);
            O7l = _mm_add_epi32(O7l, E2l);
            O7l = _mm_add_epi32(O7l, E3l);
            O7l = _mm_add_epi32(O7l, E4l);
            O7l = _mm_add_epi32(O7l, E5l);
            O7l = _mm_add_epi32(O7l, E6l);
            O7l = _mm_add_epi32(O7l, E7l);

            O7h = _mm_add_epi32(E0h, E1h);
            O7h = _mm_add_epi32(O7h, E2h);
            O7h = _mm_add_epi32(O7h, E3h);
            O7h = _mm_add_epi32(O7h, E4h);
            O7h = _mm_add_epi32(O7h, E5h);
            O7h = _mm_add_epi32(O7h, E6h);
            O7h = _mm_add_epi32(O7h, E7h);

            /* Compute O8*/

            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][8] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][8] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][8] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][8] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][8] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][8] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][8] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][8] ) ) );


            E4l = _mm_madd_epi16( m128Tmp8, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][8] ) ) );
            E4h = _mm_madd_epi16( m128Tmp9, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][8] ) ) );
            E5l = _mm_madd_epi16( m128Tmp10, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][8] ) ) );
            E5h = _mm_madd_epi16( m128Tmp11, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][8] ) ) );
            E6l = _mm_madd_epi16( m128Tmp12, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][8] ) ) );
            E6h = _mm_madd_epi16( m128Tmp13, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][8] ) ) );
            E7l = _mm_madd_epi16( m128Tmp14, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][8] ) ) );
            E7h = _mm_madd_epi16( m128Tmp15, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][8] ) ) );


            O8l = _mm_add_epi32(E0l, E1l);
            O8l = _mm_add_epi32(O8l, E2l);
            O8l = _mm_add_epi32(O8l, E3l);
            O8l = _mm_add_epi32(O8l, E4l);
            O8l = _mm_add_epi32(O8l, E5l);
            O8l = _mm_add_epi32(O8l, E6l);
            O8l = _mm_add_epi32(O8l, E7l);

            O8h = _mm_add_epi32(E0h, E1h);
            O8h = _mm_add_epi32(O8h, E2h);
            O8h = _mm_add_epi32(O8h, E3h);
            O8h = _mm_add_epi32(O8h, E4h);
            O8h = _mm_add_epi32(O8h, E5h);
            O8h = _mm_add_epi32(O8h, E6h);
            O8h = _mm_add_epi32(O8h, E7h);


            /* Compute O9*/

            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][9] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][9] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][9] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][9] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][9] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][9] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][9] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][9] ) ) );


            E4l = _mm_madd_epi16( m128Tmp8, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][9] ) ) );
            E4h = _mm_madd_epi16( m128Tmp9, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][9] ) ) );
            E5l = _mm_madd_epi16( m128Tmp10, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][9] ) ) );
            E5h = _mm_madd_epi16( m128Tmp11, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][9] ) ) );
            E6l = _mm_madd_epi16( m128Tmp12, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][9] ) ) );
            E6h = _mm_madd_epi16( m128Tmp13, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][9] ) ) );
            E7l = _mm_madd_epi16( m128Tmp14, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][9] ) ) );
            E7h = _mm_madd_epi16( m128Tmp15, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][9] ) ) );


            O9l = _mm_add_epi32(E0l, E1l);
            O9l = _mm_add_epi32(O9l, E2l);
            O9l = _mm_add_epi32(O9l, E3l);
            O9l = _mm_add_epi32(O9l, E4l);
            O9l = _mm_add_epi32(O9l, E5l);
            O9l = _mm_add_epi32(O9l, E6l);
            O9l = _mm_add_epi32(O9l, E7l);

            O9h = _mm_add_epi32(E0h, E1h);
            O9h = _mm_add_epi32(O9h, E2h);
            O9h = _mm_add_epi32(O9h, E3h);
            O9h = _mm_add_epi32(O9h, E4h);
            O9h = _mm_add_epi32(O9h, E5h);
            O9h = _mm_add_epi32(O9h, E6h);
            O9h = _mm_add_epi32(O9h, E7h);


            /* Compute 10*/

            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][10] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][10] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][10] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][10] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][10] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][10] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][10] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][10] ) ) );


            E4l = _mm_madd_epi16( m128Tmp8, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][10] ) ) );
            E4h = _mm_madd_epi16( m128Tmp9, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][10] ) ) );
            E5l = _mm_madd_epi16( m128Tmp10, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][10] ) ) );
            E5h = _mm_madd_epi16( m128Tmp11, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][10] ) ) );
            E6l = _mm_madd_epi16( m128Tmp12, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][10] ) ) );
            E6h = _mm_madd_epi16( m128Tmp13, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][10] ) ) );
            E7l = _mm_madd_epi16( m128Tmp14, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][10] ) ) );
            E7h = _mm_madd_epi16( m128Tmp15, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][10] ) ) );


            O10l = _mm_add_epi32(E0l, E1l);
            O10l = _mm_add_epi32(O10l, E2l);
            O10l = _mm_add_epi32(O10l, E3l);
            O10l = _mm_add_epi32(O10l, E4l);
            O10l = _mm_add_epi32(O10l, E5l);
            O10l = _mm_add_epi32(O10l, E6l);
            O10l = _mm_add_epi32(O10l, E7l);

            O10h = _mm_add_epi32(E0h, E1h);
            O10h = _mm_add_epi32(O10h, E2h);
            O10h = _mm_add_epi32(O10h, E3h);
            O10h = _mm_add_epi32(O10h, E4h);
            O10h = _mm_add_epi32(O10h, E5h);
            O10h = _mm_add_epi32(O10h, E6h);
            O10h = _mm_add_epi32(O10h, E7h);




            /* Compute 11*/

            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][11] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][11] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][11] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][11] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][11] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][11] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][11] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][11] ) ) );


            E4l = _mm_madd_epi16( m128Tmp8, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][11] ) ) );
            E4h = _mm_madd_epi16( m128Tmp9, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][11] ) ) );
            E5l = _mm_madd_epi16( m128Tmp10, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][11] ) ) );
            E5h = _mm_madd_epi16( m128Tmp11, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][11] ) ) );
            E6l = _mm_madd_epi16( m128Tmp12, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][11] ) ) );
            E6h = _mm_madd_epi16( m128Tmp13, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][11] ) ) );
            E7l = _mm_madd_epi16( m128Tmp14, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][11] ) ) );
            E7h = _mm_madd_epi16( m128Tmp15, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][11] ) ) );


            O11l = _mm_add_epi32(E0l, E1l);
            O11l = _mm_add_epi32(O11l, E2l);
            O11l = _mm_add_epi32(O11l, E3l);
            O11l = _mm_add_epi32(O11l, E4l);
            O11l = _mm_add_epi32(O11l, E5l);
            O11l = _mm_add_epi32(O11l, E6l);
            O11l = _mm_add_epi32(O11l, E7l);

            O11h = _mm_add_epi32(E0h, E1h);
            O11h = _mm_add_epi32(O11h, E2h);
            O11h = _mm_add_epi32(O11h, E3h);
            O11h = _mm_add_epi32(O11h, E4h);
            O11h = _mm_add_epi32(O11h, E5h);
            O11h = _mm_add_epi32(O11h, E6h);
            O11h = _mm_add_epi32(O11h, E7h);



            /* Compute 12*/

            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][12] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][12] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][12] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][12] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][12] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][12] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][12] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][12] ) ) );


            E4l = _mm_madd_epi16( m128Tmp8, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][12] ) ) );
            E4h = _mm_madd_epi16( m128Tmp9, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][12] ) ) );
            E5l = _mm_madd_epi16( m128Tmp10, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][12] ) ) );
            E5h = _mm_madd_epi16( m128Tmp11, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][12] ) ) );
            E6l = _mm_madd_epi16( m128Tmp12, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][12] ) ) );
            E6h = _mm_madd_epi16( m128Tmp13, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][12] ) ) );
            E7l = _mm_madd_epi16( m128Tmp14, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][12] ) ) );
            E7h = _mm_madd_epi16( m128Tmp15, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][12] ) ) );


            O12l = _mm_add_epi32(E0l, E1l);
            O12l = _mm_add_epi32(O12l, E2l);
            O12l = _mm_add_epi32(O12l, E3l);
            O12l = _mm_add_epi32(O12l, E4l);
            O12l = _mm_add_epi32(O12l, E5l);
            O12l = _mm_add_epi32(O12l, E6l);
            O12l = _mm_add_epi32(O12l, E7l);

            O12h = _mm_add_epi32(E0h, E1h);
            O12h = _mm_add_epi32(O12h, E2h);
            O12h = _mm_add_epi32(O12h, E3h);
            O12h = _mm_add_epi32(O12h, E4h);
            O12h = _mm_add_epi32(O12h, E5h);
            O12h = _mm_add_epi32(O12h, E6h);
            O12h = _mm_add_epi32(O12h, E7h);



            /* Compute 13*/

            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][13] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][13] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][13] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][13] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][13] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][13] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][13] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][13] ) ) );


            E4l = _mm_madd_epi16( m128Tmp8, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][13] ) ) );
            E4h = _mm_madd_epi16( m128Tmp9, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][13] ) ) );
            E5l = _mm_madd_epi16( m128Tmp10, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][13] ) ) );
            E5h = _mm_madd_epi16( m128Tmp11, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][13] ) ) );
            E6l = _mm_madd_epi16( m128Tmp12, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][13] ) ) );
            E6h = _mm_madd_epi16( m128Tmp13, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][13] ) ) );
            E7l = _mm_madd_epi16( m128Tmp14, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][13] ) ) );
            E7h = _mm_madd_epi16( m128Tmp15, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][13] ) ) );


            O13l = _mm_add_epi32(E0l, E1l);
            O13l = _mm_add_epi32(O13l, E2l);
            O13l = _mm_add_epi32(O13l, E3l);
            O13l = _mm_add_epi32(O13l, E4l);
            O13l = _mm_add_epi32(O13l, E5l);
            O13l = _mm_add_epi32(O13l, E6l);
            O13l = _mm_add_epi32(O13l, E7l);

            O13h = _mm_add_epi32(E0h, E1h);
            O13h = _mm_add_epi32(O13h, E2h);
            O13h = _mm_add_epi32(O13h, E3h);
            O13h = _mm_add_epi32(O13h, E4h);
            O13h = _mm_add_epi32(O13h, E5h);
            O13h = _mm_add_epi32(O13h, E6h);
            O13h = _mm_add_epi32(O13h, E7h);


            /* Compute O14  */

            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][14] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][14] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][14] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][14] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][14] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][14] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][14] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][14] ) ) );


            E4l = _mm_madd_epi16( m128Tmp8, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][14] ) ) );
            E4h = _mm_madd_epi16( m128Tmp9, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][14] ) ) );
            E5l = _mm_madd_epi16( m128Tmp10, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][14] ) ) );
            E5h = _mm_madd_epi16( m128Tmp11, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][14] ) ) );
            E6l = _mm_madd_epi16( m128Tmp12, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][14] ) ) );
            E6h = _mm_madd_epi16( m128Tmp13, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][14] ) ) );
            E7l = _mm_madd_epi16( m128Tmp14, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][14] ) ) );
            E7h = _mm_madd_epi16( m128Tmp15, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][14] ) ) );


            O14l = _mm_add_epi32(E0l, E1l);
            O14l = _mm_add_epi32(O14l, E2l);
            O14l = _mm_add_epi32(O14l, E3l);
            O14l = _mm_add_epi32(O14l, E4l);
            O14l = _mm_add_epi32(O14l, E5l);
            O14l = _mm_add_epi32(O14l, E6l);
            O14l = _mm_add_epi32(O14l, E7l);

            O14h = _mm_add_epi32(E0h, E1h);
            O14h = _mm_add_epi32(O14h, E2h);
            O14h = _mm_add_epi32(O14h, E3h);
            O14h = _mm_add_epi32(O14h, E4h);
            O14h = _mm_add_epi32(O14h, E5h);
            O14h = _mm_add_epi32(O14h, E6h);
            O14h = _mm_add_epi32(O14h, E7h);



            /* Compute O15*/

            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][15] ) ) );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_32x32[0][15] ) ) );
            E1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][15] ) ) );
            E1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_32x32[1][15] ) ) );
            E2l = _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][15] ) ) );
            E2h = _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_32x32[2][15] ) ) );
            E3l = _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][15] ) ) );
            E3h = _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_32x32[3][15] ) ) );


            E4l = _mm_madd_epi16( m128Tmp8, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][15] ) ) );
            E4h = _mm_madd_epi16( m128Tmp9, _mm_load_si128( (__m128i*)( tab_idct_32x32[4][15] ) ) );
            E5l = _mm_madd_epi16( m128Tmp10, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][15] ) ) );
            E5h = _mm_madd_epi16( m128Tmp11, _mm_load_si128( (__m128i*)( tab_idct_32x32[5][15] ) ) );
            E6l = _mm_madd_epi16( m128Tmp12, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][15] ) ) );
            E6h = _mm_madd_epi16( m128Tmp13, _mm_load_si128( (__m128i*)( tab_idct_32x32[6][15] ) ) );
            E7l = _mm_madd_epi16( m128Tmp14, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][15] ) ) );
            E7h = _mm_madd_epi16( m128Tmp15, _mm_load_si128( (__m128i*)( tab_idct_32x32[7][15] ) ) );


            O15l = _mm_add_epi32(E0l, E1l);
            O15l = _mm_add_epi32(O15l, E2l);
            O15l = _mm_add_epi32(O15l, E3l);
            O15l = _mm_add_epi32(O15l, E4l);
            O15l = _mm_add_epi32(O15l, E5l);
            O15l = _mm_add_epi32(O15l, E6l);
            O15l = _mm_add_epi32(O15l, E7l);

            O15h = _mm_add_epi32(E0h, E1h);
            O15h = _mm_add_epi32(O15h, E2h);
            O15h = _mm_add_epi32(O15h, E3h);
            O15h = _mm_add_epi32(O15h, E4h);
            O15h = _mm_add_epi32(O15h, E5h);
            O15h = _mm_add_epi32(O15h, E6h);
            O15h = _mm_add_epi32(O15h, E7h);
            /*  Compute E0  */

            m128Tmp0 = _mm_unpacklo_epi16(  m128iS2, m128iS6 );
            E0l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][0] ) ) );
            m128Tmp1 = _mm_unpackhi_epi16(  m128iS2, m128iS6 );
            E0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][0] ) ) );


            m128Tmp2 =  _mm_unpacklo_epi16(  m128iS10, m128iS14 );
            E0l = _mm_add_epi32(E0l, _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][0] ) ) ));
            m128Tmp3 = _mm_unpackhi_epi16(  m128iS10, m128iS14 );
            E0h = _mm_add_epi32(E0h, _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][0] ) ) ));

            m128Tmp4 =  _mm_unpacklo_epi16(  m128iS18, m128iS22 );
            E0l = _mm_add_epi32(E0l, _mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][0] ) ) ));
            m128Tmp5 = _mm_unpackhi_epi16(  m128iS18, m128iS22 );
            E0h = _mm_add_epi32(E0h, _mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][0] ) ) ));


            m128Tmp6 =  _mm_unpacklo_epi16(  m128iS26, m128iS30 );
            E0l = _mm_add_epi32(E0l, _mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][0] ) ) ));
            m128Tmp7 = _mm_unpackhi_epi16(  m128iS26, m128iS30 );
            E0h = _mm_add_epi32(E0h, _mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][0] ) ) ));

            /*  Compute E1  */
            E1l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][1] ) ));
            E1h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][1] ) ) );
            E1l = _mm_add_epi32(E1l,_mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][1] ) ) ));
            E1h = _mm_add_epi32(E1h,_mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][1] ) ) ));
            E1l = _mm_add_epi32(E1l,_mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][1] ) ) ));
            E1h = _mm_add_epi32(E1h,_mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][1] ) ) ));
            E1l = _mm_add_epi32(E1l,_mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][1] ) ) ));
            E1h = _mm_add_epi32(E1h,_mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][1] ) ) ));

            /*  Compute E2  */
            E2l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][2] ) ) );
            E2h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][2] ) ) );
            E2l = _mm_add_epi32(E2l,_mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][2] ) ) ));
            E2h = _mm_add_epi32(E2h,_mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][2] ) ) ));
            E2l = _mm_add_epi32(E2l,_mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][2] ) ) ));
            E2h = _mm_add_epi32(E2h,_mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][2] ) ) ));
            E2l = _mm_add_epi32(E2l,_mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][2] ) ) ));
            E2h = _mm_add_epi32(E2h,_mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][2] ) ) ));


            /*  Compute E3  */
            E3l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][3] ) ) );
            E3h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][3] ) ) );
            E3l = _mm_add_epi32(E3l,_mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][3] ) ) ));
            E3h = _mm_add_epi32(E3h,_mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][3] ) ) ));
            E3l = _mm_add_epi32(E3l,_mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][3] ) ) ));
            E3h = _mm_add_epi32(E3h,_mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][3] ) ) ));
            E3l = _mm_add_epi32(E3l,_mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][3] ) ) ));
            E3h = _mm_add_epi32(E3h,_mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][3] ) ) ));

            /*  Compute E4  */
            E4l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][4] ) ) );
            E4h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][4] ) ) );
            E4l = _mm_add_epi32(E4l,_mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][4] ) ) ));
            E4h = _mm_add_epi32(E4h,_mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][4] ) ) ));
            E4l = _mm_add_epi32(E4l,_mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][4] ) ) ));
            E4h = _mm_add_epi32(E4h,_mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][4] ) ) ));
            E4l = _mm_add_epi32(E4l,_mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][4] ) ) ));
            E4h = _mm_add_epi32(E4h,_mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][4] ) ) ));


            /*  Compute E3  */
            E5l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][5] ) ) );
            E5h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][5] ) ) );
            E5l = _mm_add_epi32(E5l,_mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][5] ) ) ));
            E5h = _mm_add_epi32(E5h,_mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][5] ) ) ));
            E5l = _mm_add_epi32(E5l,_mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][5] ) ) ));
            E5h = _mm_add_epi32(E5h,_mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][5] ) ) ));
            E5l = _mm_add_epi32(E5l,_mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][5] ) ) ));
            E5h = _mm_add_epi32(E5h,_mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][5] ) ) ));


            /*  Compute E6  */
            E6l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][6] ) ) );
            E6h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][6] ) ) );
            E6l = _mm_add_epi32(E6l,_mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][6] ) ) ));
            E6h = _mm_add_epi32(E6h,_mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][6] ) ) ));
            E6l = _mm_add_epi32(E6l,_mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][6] ) ) ));
            E6h = _mm_add_epi32(E6h,_mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][6] ) ) ));
            E6l = _mm_add_epi32(E6l,_mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][6] ) ) ));
            E6h = _mm_add_epi32(E6h,_mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][6] ) ) ));

            /*  Compute E7  */
            E7l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][7] ) ) );
            E7h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[0][7] ) ) );
            E7l = _mm_add_epi32(E7l,_mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][7] ) ) ));
            E7h = _mm_add_epi32(E7h,_mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[1][7] ) ) ));
            E7l = _mm_add_epi32(E7l,_mm_madd_epi16( m128Tmp4, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][7] ) ) ));
            E7h = _mm_add_epi32(E7h,_mm_madd_epi16( m128Tmp5, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[2][7] ) ) ));
            E7l = _mm_add_epi32(E7l,_mm_madd_epi16( m128Tmp6, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][7] ) ) ));
            E7h = _mm_add_epi32(E7h,_mm_madd_epi16( m128Tmp7, _mm_load_si128( (__m128i*)( tab_idct_16x16_1[3][7] ) ) ));


            /*  Compute EE0 and EEE */


            m128Tmp0 = _mm_unpacklo_epi16(  m128iS4, m128iS12 );
            E00l =  _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[0][0] ) ) );
            m128Tmp1 = _mm_unpackhi_epi16(  m128iS4, m128iS12 );
            E00h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[0][0] ) ) );

            m128Tmp2 = _mm_unpacklo_epi16(  m128iS20, m128iS28 );
            E00l =  _mm_add_epi32(E00l, _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[1][0] ) ) ));
            m128Tmp3 = _mm_unpackhi_epi16(  m128iS20, m128iS28 );
            E00h = _mm_add_epi32(E00h,_mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[1][0] ) ) ));



            E01l =  _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[0][1] ) ) );
            E01h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[0][1] ) ) );
            E01l =  _mm_add_epi32(E01l, _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[1][1] ) ) ));
            E01h = _mm_add_epi32(E01h,_mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[1][1] ) ) ));

            E02l =  _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[0][2] ) ) );
            E02h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[0][2] ) ) );
            E02l =  _mm_add_epi32(E02l, _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[1][2] ) ) ));
            E02h = _mm_add_epi32(E02h,_mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[1][2] ) ) ));

            E03l =  _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[0][3] ) ) );
            E03h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[0][3] ) ) );
            E03l =  _mm_add_epi32(E03l, _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[1][3] ) ) ));
            E03h = _mm_add_epi32(E03h,_mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_2[1][3] ) ) ));

            /*  Compute EE0 and EEE */


            m128Tmp0 = _mm_unpacklo_epi16(  m128iS8, m128iS24 );
            EE0l =  _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_3[0][0] ) ) );
            m128Tmp1 = _mm_unpackhi_epi16(  m128iS8, m128iS24 );
            EE0h = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_3[0][0] ) ) );

            m128Tmp2 =  _mm_unpacklo_epi16(  m128iS0, m128iS16 );
            EEE0l =  _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_3[1][0] ) ) );
            m128Tmp3 = _mm_unpackhi_epi16(  m128iS0, m128iS16 );
            EEE0h =  _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_3[1][0] ) ) );


            EE1l = _mm_madd_epi16( m128Tmp0, _mm_load_si128( (__m128i*)( tab_idct_16x16_3[0][1] ) ) );
            EE1h  = _mm_madd_epi16( m128Tmp1, _mm_load_si128( (__m128i*)( tab_idct_16x16_3[0][1] ) ) );

            EEE1l = _mm_madd_epi16( m128Tmp2, _mm_load_si128( (__m128i*)( tab_idct_16x16_3[1][1] ) ) );
            EEE1h = _mm_madd_epi16( m128Tmp3, _mm_load_si128( (__m128i*)( tab_idct_16x16_3[1][1] ) ) );

            /*  Compute EE    */

            EE2l = _mm_sub_epi32(EEE1l,EE1l);
            EE3l = _mm_sub_epi32(EEE0l,EE0l);
            EE2h = _mm_sub_epi32(EEE1h,EE1h);
            EE3h = _mm_sub_epi32(EEE0h,EE0h);

            EE0l = _mm_add_epi32(EEE0l,EE0l);
            EE1l = _mm_add_epi32(EEE1l,EE1l);
            EE0h = _mm_add_epi32(EEE0h,EE0h);
            EE1h = _mm_add_epi32(EEE1h,EE1h);
            /**/

            EE7l = _mm_sub_epi32(EE0l, E00l);
            EE6l = _mm_sub_epi32(EE1l, E01l);
            EE5l = _mm_sub_epi32(EE2l, E02l);
            EE4l = _mm_sub_epi32(EE3l, E03l);

            EE7h = _mm_sub_epi32(EE0h, E00h);
            EE6h = _mm_sub_epi32(EE1h, E01h);
            EE5h = _mm_sub_epi32(EE2h, E02h);
            EE4h = _mm_sub_epi32(EE3h, E03h);


            EE0l = _mm_add_epi32(EE0l, E00l);
            EE1l = _mm_add_epi32(EE1l, E01l);
            EE2l = _mm_add_epi32(EE2l, E02l);
            EE3l = _mm_add_epi32(EE3l, E03l);

            EE0h = _mm_add_epi32(EE0h, E00h);
            EE1h = _mm_add_epi32(EE1h, E01h);
            EE2h = _mm_add_epi32(EE2h, E02h);
            EE3h = _mm_add_epi32(EE3h, E03h);
            /*      Compute E       */

            E15l = _mm_sub_epi32(EE0l,E0l);
            E15l = _mm_add_epi32(E15l, m128iAdd);
            E14l = _mm_sub_epi32(EE1l,E1l);
            E14l = _mm_add_epi32(E14l, m128iAdd);
            E13l = _mm_sub_epi32(EE2l,E2l);
            E13l = _mm_add_epi32(E13l, m128iAdd);
            E12l = _mm_sub_epi32(EE3l,E3l);
            E12l = _mm_add_epi32(E12l, m128iAdd);
            E11l = _mm_sub_epi32(EE4l,E4l);
            E11l = _mm_add_epi32(E11l, m128iAdd);
            E10l = _mm_sub_epi32(EE5l,E5l);
            E10l = _mm_add_epi32(E10l, m128iAdd);
            E9l = _mm_sub_epi32(EE6l,E6l);
            E9l = _mm_add_epi32(E9l, m128iAdd);
            E8l = _mm_sub_epi32(EE7l,E7l);
            E8l = _mm_add_epi32(E8l, m128iAdd);

            E0l = _mm_add_epi32(EE0l,E0l);
            E0l = _mm_add_epi32(E0l, m128iAdd);
            E1l = _mm_add_epi32(EE1l,E1l);
            E1l = _mm_add_epi32(E1l, m128iAdd);
            E2l = _mm_add_epi32(EE2l,E2l);
            E2l = _mm_add_epi32(E2l, m128iAdd);
            E3l = _mm_add_epi32(EE3l,E3l);
            E3l = _mm_add_epi32(E3l, m128iAdd);
            E4l = _mm_add_epi32(EE4l,E4l);
            E4l = _mm_add_epi32(E4l, m128iAdd);
            E5l = _mm_add_epi32(EE5l,E5l);
            E5l = _mm_add_epi32(E5l, m128iAdd);
            E6l = _mm_add_epi32(EE6l,E6l);
            E6l = _mm_add_epi32(E6l, m128iAdd);
            E7l = _mm_add_epi32(EE7l,E7l);
            E7l = _mm_add_epi32(E7l, m128iAdd);


            E15h = _mm_sub_epi32(EE0h,E0h);
            E15h = _mm_add_epi32(E15h, m128iAdd);
            E14h = _mm_sub_epi32(EE1h,E1h);
            E14h = _mm_add_epi32(E14h, m128iAdd);
            E13h = _mm_sub_epi32(EE2h,E2h);
            E13h = _mm_add_epi32(E13h, m128iAdd);
            E12h = _mm_sub_epi32(EE3h,E3h);
            E12h = _mm_add_epi32(E12h, m128iAdd);
            E11h = _mm_sub_epi32(EE4h,E4h);
            E11h = _mm_add_epi32(E11h, m128iAdd);
            E10h = _mm_sub_epi32(EE5h,E5h);
            E10h = _mm_add_epi32(E10h, m128iAdd);
            E9h = _mm_sub_epi32(EE6h,E6h);
            E9h = _mm_add_epi32(E9h, m128iAdd);
            E8h = _mm_sub_epi32(EE7h,E7h);
            E8h = _mm_add_epi32(E8h, m128iAdd);

            E0h = _mm_add_epi32(EE0h,E0h);
            E0h = _mm_add_epi32(E0h, m128iAdd);
            E1h = _mm_add_epi32(EE1h,E1h);
            E1h = _mm_add_epi32(E1h, m128iAdd);
            E2h = _mm_add_epi32(EE2h,E2h);
            E2h = _mm_add_epi32(E2h, m128iAdd);
            E3h = _mm_add_epi32(EE3h,E3h);
            E3h = _mm_add_epi32(E3h, m128iAdd);
            E4h = _mm_add_epi32(EE4h,E4h);
            E4h = _mm_add_epi32(E4h, m128iAdd);
            E5h = _mm_add_epi32(EE5h,E5h);
            E5h = _mm_add_epi32(E5h, m128iAdd);
            E6h = _mm_add_epi32(EE6h,E6h);
            E6h = _mm_add_epi32(E6h, m128iAdd);
            E7h = _mm_add_epi32(EE7h,E7h);
            E7h = _mm_add_epi32(E7h, m128iAdd);


            m128iS0 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E0l, O0l),shift), _mm_srai_epi32(_mm_add_epi32(E0h, O0h), shift));
            m128iS1 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E1l, O1l),shift), _mm_srai_epi32(_mm_add_epi32(E1h, O1h), shift));
            m128iS2 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E2l, O2l),shift), _mm_srai_epi32(_mm_add_epi32(E2h, O2h), shift));
            m128iS3 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E3l, O3l),shift), _mm_srai_epi32(_mm_add_epi32(E3h, O3h), shift));
            m128iS4 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E4l, O4l),shift), _mm_srai_epi32(_mm_add_epi32(E4h, O4h), shift));
            m128iS5 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E5l, O5l),shift), _mm_srai_epi32(_mm_add_epi32(E5h, O5h), shift));
            m128iS6 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E6l, O6l),shift), _mm_srai_epi32(_mm_add_epi32(E6h, O6h), shift));
            m128iS7 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E7l, O7l),shift), _mm_srai_epi32(_mm_add_epi32(E7h, O7h), shift));
            m128iS8 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E8l, O8l),shift), _mm_srai_epi32(_mm_add_epi32(E8h, O8h), shift));
            m128iS9 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E9l, O9l),shift), _mm_srai_epi32(_mm_add_epi32(E9h, O9h), shift));
            m128iS10 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E10l, O10l),shift), _mm_srai_epi32(_mm_add_epi32(E10h, O10h), shift));
            m128iS11 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E11l, O11l),shift), _mm_srai_epi32(_mm_add_epi32(E11h, O11h), shift));
            m128iS12 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E12l, O12l),shift), _mm_srai_epi32(_mm_add_epi32(E12h, O12h), shift));
            m128iS13 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E13l, O13l),shift), _mm_srai_epi32(_mm_add_epi32(E13h, O13h), shift));
            m128iS14 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E14l, O14l),shift), _mm_srai_epi32(_mm_add_epi32(E14h, O14h), shift));
            m128iS15 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(E15l, O15l),shift), _mm_srai_epi32(_mm_add_epi32(E15h, O15h), shift));

            m128iS31 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E0l, O0l),shift), _mm_srai_epi32(_mm_sub_epi32(E0h, O0h), shift));
            m128iS30 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E1l, O1l),shift), _mm_srai_epi32(_mm_sub_epi32(E1h, O1h), shift));
            m128iS29 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E2l, O2l),shift), _mm_srai_epi32(_mm_sub_epi32(E2h, O2h), shift));
            m128iS28 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E3l, O3l),shift), _mm_srai_epi32(_mm_sub_epi32(E3h, O3h), shift));
            m128iS27 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E4l, O4l),shift), _mm_srai_epi32(_mm_sub_epi32(E4h, O4h), shift));
            m128iS26 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E5l, O5l),shift), _mm_srai_epi32(_mm_sub_epi32(E5h, O5h), shift));
            m128iS25 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E6l, O6l),shift), _mm_srai_epi32(_mm_sub_epi32(E6h, O6h), shift));
            m128iS24 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E7l, O7l),shift), _mm_srai_epi32(_mm_sub_epi32(E7h, O7h), shift));
            m128iS23 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E8l, O8l),shift), _mm_srai_epi32(_mm_sub_epi32(E8h, O8h), shift));
            m128iS22 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E9l, O9l),shift), _mm_srai_epi32(_mm_sub_epi32(E9h, O9h), shift));
            m128iS21 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E10l, O10l),shift), _mm_srai_epi32(_mm_sub_epi32(E10h, O10h), shift));
            m128iS20 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E11l, O11l),shift), _mm_srai_epi32(_mm_sub_epi32(E11h, O11h), shift));
            m128iS19 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E12l, O12l),shift), _mm_srai_epi32(_mm_sub_epi32(E12h, O12h), shift));
            m128iS18 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E13l, O13l),shift), _mm_srai_epi32(_mm_sub_epi32(E13h, O13h), shift));
            m128iS17 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E14l, O14l),shift), _mm_srai_epi32(_mm_sub_epi32(E14h, O14h), shift));
            m128iS16 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(E15l, O15l),shift), _mm_srai_epi32(_mm_sub_epi32(E15h, O15h), shift));

            if(!j){
                /*      Inverse the matrix      */
                E0l = _mm_unpacklo_epi16(m128iS0, m128iS16);
                E1l = _mm_unpacklo_epi16(m128iS1, m128iS17);
                E2l = _mm_unpacklo_epi16(m128iS2, m128iS18);
                E3l = _mm_unpacklo_epi16(m128iS3, m128iS19);
                E4l = _mm_unpacklo_epi16(m128iS4, m128iS20);
                E5l = _mm_unpacklo_epi16(m128iS5, m128iS21);
                E6l = _mm_unpacklo_epi16(m128iS6, m128iS22);
                E7l = _mm_unpacklo_epi16(m128iS7, m128iS23);
                E8l = _mm_unpacklo_epi16(m128iS8, m128iS24);
                E9l = _mm_unpacklo_epi16(m128iS9, m128iS25);
                E10l = _mm_unpacklo_epi16(m128iS10, m128iS26);
                E11l = _mm_unpacklo_epi16(m128iS11, m128iS27);
                E12l = _mm_unpacklo_epi16(m128iS12, m128iS28);
                E13l = _mm_unpacklo_epi16(m128iS13, m128iS29);
                E14l = _mm_unpacklo_epi16(m128iS14, m128iS30);
                E15l = _mm_unpacklo_epi16(m128iS15, m128iS31);

                O0l = _mm_unpackhi_epi16(m128iS0, m128iS16);
                O1l = _mm_unpackhi_epi16(m128iS1, m128iS17);
                O2l = _mm_unpackhi_epi16(m128iS2, m128iS18);
                O3l = _mm_unpackhi_epi16(m128iS3, m128iS19);
                O4l = _mm_unpackhi_epi16(m128iS4, m128iS20);
                O5l = _mm_unpackhi_epi16(m128iS5, m128iS21);
                O6l = _mm_unpackhi_epi16(m128iS6, m128iS22);
                O7l = _mm_unpackhi_epi16(m128iS7, m128iS23);
                O8l = _mm_unpackhi_epi16(m128iS8, m128iS24);
                O9l = _mm_unpackhi_epi16(m128iS9, m128iS25);
                O10l = _mm_unpackhi_epi16(m128iS10, m128iS26);
                O11l = _mm_unpackhi_epi16(m128iS11, m128iS27);
                O12l = _mm_unpackhi_epi16(m128iS12, m128iS28);
                O13l = _mm_unpackhi_epi16(m128iS13, m128iS29);
                O14l = _mm_unpackhi_epi16(m128iS14, m128iS30);
                O15l = _mm_unpackhi_epi16(m128iS15, m128iS31);

                E0h  = _mm_unpacklo_epi16(E0l, E8l);
                E1h  = _mm_unpacklo_epi16(E1l, E9l);
                E2h = _mm_unpacklo_epi16(E2l, E10l);
                E3h  = _mm_unpacklo_epi16(E3l, E11l);
                E4h  = _mm_unpacklo_epi16(E4l, E12l);
                E5h  = _mm_unpacklo_epi16(E5l, E13l);
                E6h  = _mm_unpacklo_epi16(E6l, E14l);
                E7h  = _mm_unpacklo_epi16(E7l, E15l);

                E8h = _mm_unpackhi_epi16(E0l, E8l);
                E9h = _mm_unpackhi_epi16(E1l, E9l);
                E10h = _mm_unpackhi_epi16(E2l, E10l);
                E11h = _mm_unpackhi_epi16(E3l, E11l);
                E12h = _mm_unpackhi_epi16(E4l, E12l);
                E13h = _mm_unpackhi_epi16(E5l, E13l);
                E14h = _mm_unpackhi_epi16(E6l, E14l);
                E15h = _mm_unpackhi_epi16(E7l, E15l);


                m128Tmp0 = _mm_unpacklo_epi16(E0h, E4h);
                m128Tmp1 = _mm_unpacklo_epi16(E1h, E5h);
                m128Tmp2 = _mm_unpacklo_epi16(E2h, E6h);
                m128Tmp3 = _mm_unpacklo_epi16(E3h, E7h);

                m128Tmp4 = _mm_unpacklo_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpacklo_epi16(m128Tmp1, m128Tmp3);
                m128iS0  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS1  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                m128Tmp4 = _mm_unpackhi_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpackhi_epi16(m128Tmp1, m128Tmp3);
                m128iS2  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS3  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                m128Tmp0 = _mm_unpackhi_epi16(E0h, E4h);
                m128Tmp1 = _mm_unpackhi_epi16(E1h, E5h);
                m128Tmp2 = _mm_unpackhi_epi16(E2h, E6h);
                m128Tmp3 = _mm_unpackhi_epi16(E3h, E7h);

                m128Tmp4 = _mm_unpacklo_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpacklo_epi16(m128Tmp1, m128Tmp3);
                m128iS4  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS5  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                m128Tmp4 = _mm_unpackhi_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpackhi_epi16(m128Tmp1, m128Tmp3);
                m128iS6  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS7  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                m128Tmp0 = _mm_unpacklo_epi16(E8h, E12h);
                m128Tmp1 = _mm_unpacklo_epi16(E9h, E13h);
                m128Tmp2 = _mm_unpacklo_epi16(E10h, E14h);
                m128Tmp3 = _mm_unpacklo_epi16(E11h, E15h);

                m128Tmp4 = _mm_unpacklo_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpacklo_epi16(m128Tmp1, m128Tmp3);
                m128iS8  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS9  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                m128Tmp4 = _mm_unpackhi_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpackhi_epi16(m128Tmp1, m128Tmp3);
                m128iS10  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS11  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                m128Tmp0 = _mm_unpackhi_epi16(E8h, E12h);
                m128Tmp1 = _mm_unpackhi_epi16(E9h, E13h);
                m128Tmp2 = _mm_unpackhi_epi16(E10h, E14h);
                m128Tmp3 = _mm_unpackhi_epi16(E11h, E15h);

                m128Tmp4 = _mm_unpacklo_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpacklo_epi16(m128Tmp1, m128Tmp3);
                m128iS12  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS13  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                m128Tmp4 = _mm_unpackhi_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpackhi_epi16(m128Tmp1, m128Tmp3);
                m128iS14  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS15  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                /*  */
                E0h  = _mm_unpacklo_epi16(O0l, O8l);
                E1h  = _mm_unpacklo_epi16(O1l, O9l);
                E2h = _mm_unpacklo_epi16(O2l, O10l);
                E3h  = _mm_unpacklo_epi16(O3l, O11l);
                E4h  = _mm_unpacklo_epi16(O4l, O12l);
                E5h  = _mm_unpacklo_epi16(O5l, O13l);
                E6h  = _mm_unpacklo_epi16(O6l, O14l);
                E7h  = _mm_unpacklo_epi16(O7l, O15l);

                E8h = _mm_unpackhi_epi16(O0l, O8l);
                E9h = _mm_unpackhi_epi16(O1l, O9l);
                E10h = _mm_unpackhi_epi16(O2l, O10l);
                E11h = _mm_unpackhi_epi16(O3l, O11l);
                E12h = _mm_unpackhi_epi16(O4l, O12l);
                E13h = _mm_unpackhi_epi16(O5l, O13l);
                E14h = _mm_unpackhi_epi16(O6l, O14l);
                E15h = _mm_unpackhi_epi16(O7l, O15l);

                m128Tmp0 = _mm_unpacklo_epi16(E0h, E4h);
                m128Tmp1 = _mm_unpacklo_epi16(E1h, E5h);
                m128Tmp2 = _mm_unpacklo_epi16(E2h, E6h);
                m128Tmp3 = _mm_unpacklo_epi16(E3h, E7h);

                m128Tmp4 = _mm_unpacklo_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpacklo_epi16(m128Tmp1, m128Tmp3);
                m128iS16  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS17  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                m128Tmp4 = _mm_unpackhi_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpackhi_epi16(m128Tmp1, m128Tmp3);
                m128iS18  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS19  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                m128Tmp0 = _mm_unpackhi_epi16(E0h, E4h);
                m128Tmp1 = _mm_unpackhi_epi16(E1h, E5h);
                m128Tmp2 = _mm_unpackhi_epi16(E2h, E6h);
                m128Tmp3 = _mm_unpackhi_epi16(E3h, E7h);

                m128Tmp4 = _mm_unpacklo_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpacklo_epi16(m128Tmp1, m128Tmp3);
                m128iS20  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS21  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                m128Tmp4 = _mm_unpackhi_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpackhi_epi16(m128Tmp1, m128Tmp3);
                m128iS22  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS23  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                m128Tmp0 = _mm_unpacklo_epi16(E8h, E12h);
                m128Tmp1 = _mm_unpacklo_epi16(E9h, E13h);
                m128Tmp2 = _mm_unpacklo_epi16(E10h, E14h);
                m128Tmp3 = _mm_unpacklo_epi16(E11h, E15h);

                m128Tmp4 = _mm_unpacklo_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpacklo_epi16(m128Tmp1, m128Tmp3);
                m128iS24  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS25  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                m128Tmp4 = _mm_unpackhi_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpackhi_epi16(m128Tmp1, m128Tmp3);
                m128iS26  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS27  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                m128Tmp0 = _mm_unpackhi_epi16(E8h, E12h);
                m128Tmp1 = _mm_unpackhi_epi16(E9h, E13h);
                m128Tmp2 = _mm_unpackhi_epi16(E10h, E14h);
                m128Tmp3 = _mm_unpackhi_epi16(E11h, E15h);

                m128Tmp4 = _mm_unpacklo_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpacklo_epi16(m128Tmp1, m128Tmp3);
                m128iS28  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS29  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);

                m128Tmp4 = _mm_unpackhi_epi16(m128Tmp0, m128Tmp2);
                m128Tmp5 = _mm_unpackhi_epi16(m128Tmp1, m128Tmp3);
                m128iS30  = _mm_unpacklo_epi16(m128Tmp4, m128Tmp5);
                m128iS31  = _mm_unpackhi_epi16(m128Tmp4, m128Tmp5);
                /*  */
                _mm_store_si128((__m128i*)( pSrc +      i ), m128iS0);
                _mm_store_si128((__m128i*)( pSrc + 32 + i ), m128iS1);
                _mm_store_si128((__m128i*)( pSrc + 64 + i ), m128iS2);
                _mm_store_si128((__m128i*)( pSrc + 96 + i ), m128iS3);
                _mm_store_si128((__m128i*)( pSrc + 128 + i ), m128iS4);
                _mm_store_si128((__m128i*)( pSrc + 160 + i ), m128iS5);
                _mm_store_si128((__m128i*)( pSrc + 192 + i ), m128iS6);
                _mm_store_si128((__m128i*)( pSrc + 224 + i), m128iS7);
                _mm_store_si128((__m128i*)( pSrc + 256 + i), m128iS8);
                _mm_store_si128((__m128i*)( pSrc + 288 + i ), m128iS9);
                _mm_store_si128((__m128i*)( pSrc + 320 + i), m128iS10);
                _mm_store_si128((__m128i*)( pSrc + 352 + i), m128iS11);
                _mm_store_si128((__m128i*)( pSrc + 384 + i ), m128iS12);
                _mm_store_si128((__m128i*)( pSrc + 416 + i), m128iS13);
                _mm_store_si128((__m128i*)( pSrc + 448 + i), m128iS14);
                _mm_store_si128((__m128i*)( pSrc + 480 + i ), m128iS15);
                _mm_store_si128((__m128i*)( pSrc + 512 +i ), m128iS16);
                _mm_store_si128((__m128i*)( pSrc + 544 + i ), m128iS17);
                _mm_store_si128((__m128i*)( pSrc + 576 + i ), m128iS18);
                _mm_store_si128((__m128i*)( pSrc + 608 + i ), m128iS19);
                _mm_store_si128((__m128i*)( pSrc + 640 + i ), m128iS20);
                _mm_store_si128((__m128i*)( pSrc + 672 + i ), m128iS21);
                _mm_store_si128((__m128i*)( pSrc + 704 + i ), m128iS22);
                _mm_store_si128((__m128i*)( pSrc + 736 + i), m128iS23);
                _mm_store_si128((__m128i*)( pSrc + 768 + i), m128iS24);
                _mm_store_si128((__m128i*)( pSrc + 800 + i ), m128iS25);
                _mm_store_si128((__m128i*)( pSrc + 832 + i), m128iS26);
                _mm_store_si128((__m128i*)( pSrc + 864 + i), m128iS27);
                _mm_store_si128((__m128i*)( pSrc + 896 + i ), m128iS28);
                _mm_store_si128((__m128i*)( pSrc + 928 + i), m128iS29);
                _mm_store_si128((__m128i*)( pSrc + 960 + i), m128iS30);
                _mm_store_si128((__m128i*)( pSrc + 992+ i ), m128iS31);

                if(i <= 16 ) {
                    int k = i+8;
                    m128iS0   = _mm_load_si128( (__m128i*)( pSrc + k ) );
                    m128iS1   = _mm_load_si128( (__m128i*)( pSrc + 32 + k) );
                    m128iS2   = _mm_load_si128( (__m128i*)( pSrc + 64 + k) );
                    m128iS3   = _mm_load_si128( (__m128i*)( pSrc + 96 + k) );
                    m128iS4   = _mm_load_si128( (__m128i*)( pSrc + 128 + k ) );
                    m128iS5   = _mm_load_si128( (__m128i*)( pSrc + 160 + k) );
                    m128iS6   = _mm_load_si128( (__m128i*)( pSrc + 192 +k) );
                    m128iS7   = _mm_load_si128( (__m128i*)( pSrc + 224 +k ) );
                    m128iS8   = _mm_load_si128( (__m128i*)( pSrc + 256 +k ) );
                    m128iS9   = _mm_load_si128( (__m128i*)( pSrc + 288 +k ));
                    m128iS10   = _mm_load_si128( (__m128i*)( pSrc + 320 + k ) );
                    m128iS11   = _mm_load_si128( (__m128i*)( pSrc + 352 + k));
                    m128iS12   = _mm_load_si128( (__m128i*)( pSrc + 384 +k ) );
                    m128iS13   = _mm_load_si128( (__m128i*)( pSrc + 416 + k) );
                    m128iS14   = _mm_load_si128( (__m128i*)( pSrc + 448 + k) );
                    m128iS15   = _mm_load_si128( (__m128i*)( pSrc + 480 + k) );

                    m128iS16   = _mm_load_si128( (__m128i*)( pSrc + 512 + k) );
                    m128iS17   = _mm_load_si128( (__m128i*)( pSrc + 544 + k) );
                    m128iS18   = _mm_load_si128( (__m128i*)( pSrc + 576 + k) );
                    m128iS19   = _mm_load_si128( (__m128i*)( pSrc + 608 + k) );
                    m128iS20   = _mm_load_si128( (__m128i*)( pSrc + 640 + k ) );
                    m128iS21   = _mm_load_si128( (__m128i*)( pSrc + 672 + k) );
                    m128iS22   = _mm_load_si128( (__m128i*)( pSrc + 704 + k) );
                    m128iS23   = _mm_load_si128( (__m128i*)( pSrc + 736 + k ) );
                    m128iS24   = _mm_load_si128( (__m128i*)( pSrc + 768 + k ) );
                    m128iS25   = _mm_load_si128( (__m128i*)( pSrc + 800 + k ));
                    m128iS26   = _mm_load_si128( (__m128i*)( pSrc + 832 + k ) );
                    m128iS27   = _mm_load_si128( (__m128i*)( pSrc + 864 + k));
                    m128iS28   = _mm_load_si128( (__m128i*)( pSrc + 896 + k ) );
                    m128iS29   = _mm_load_si128( (__m128i*)( pSrc + 928 + k) );
                    m128iS30   = _mm_load_si128( (__m128i*)( pSrc + 960 + k) );
                    m128iS31   = _mm_load_si128( (__m128i*)( pSrc + 992 + k) );
                } else {
                    m128iS0   = _mm_load_si128( (__m128i*)( pSrc) );
                    m128iS1   = _mm_load_si128( (__m128i*)( pSrc + 128) );
                    m128iS2   = _mm_load_si128( (__m128i*)( pSrc + 256 ) );
                    m128iS3   = _mm_load_si128( (__m128i*)( pSrc + 384 ) );
                    m128iS4   = _mm_loadu_si128((__m128i*)( pSrc  + 512 ) );
                    m128iS5   = _mm_load_si128( (__m128i*)( pSrc + 640 ) );
                    m128iS6   = _mm_load_si128( (__m128i*)( pSrc  + 768) );
                    m128iS7   = _mm_load_si128( (__m128i*)( pSrc + 896) );
                    m128iS8   = _mm_load_si128( (__m128i*)( pSrc + 8) );
                    m128iS9   = _mm_load_si128( (__m128i*)( pSrc + 128 +8));
                    m128iS10  = _mm_load_si128( (__m128i*)( pSrc + 256  +8 ) );
                    m128iS11  = _mm_load_si128( (__m128i*)( pSrc + 384 +8));
                    m128iS12  = _mm_loadu_si128((__m128i*)( pSrc + 512 +8) );
                    m128iS13  = _mm_load_si128( (__m128i*)( pSrc + 640 +8) );
                    m128iS14  = _mm_load_si128( (__m128i*)( pSrc + 768 +8) );
                    m128iS15  = _mm_load_si128( (__m128i*)( pSrc + 896 +8) );
                    m128iS16  = _mm_load_si128( (__m128i*)( pSrc + 16) );
                    m128iS17  = _mm_load_si128( (__m128i*)( pSrc + 128 +16));
                    m128iS18  = _mm_load_si128( (__m128i*)( pSrc + 256  +16 ) );
                    m128iS19  = _mm_load_si128( (__m128i*)( pSrc + 384 +16));
                    m128iS20  = _mm_loadu_si128((__m128i*)( pSrc + 512 +16) );
                    m128iS21  = _mm_load_si128( (__m128i*)( pSrc + 640 +16) );
                    m128iS22  = _mm_load_si128( (__m128i*)( pSrc + 768 +16) );
                    m128iS23  = _mm_load_si128( (__m128i*)( pSrc + 896 +16) );
                    m128iS24  = _mm_load_si128( (__m128i*)( pSrc + 24) );
                    m128iS25  = _mm_load_si128( (__m128i*)( pSrc + 128 +24));
                    m128iS26  = _mm_load_si128( (__m128i*)( pSrc + 256  +24 ) );
                    m128iS27  = _mm_load_si128( (__m128i*)( pSrc + 384 +24));
                    m128iS28  = _mm_loadu_si128((__m128i*)( pSrc + 512 +24) );
                    m128iS29  = _mm_load_si128( (__m128i*)( pSrc + 640 +24) );
                    m128iS30  = _mm_load_si128( (__m128i*)( pSrc + 768 +24) );
                    m128iS31  = _mm_load_si128( (__m128i*)( pSrc + 896 +24) );
                    shift = 12;
                    m128iAdd  = _mm_set1_epi32( 2048 );
                }

            } else {
                __m128i T00, T01, T02, T03;
                __m128i T10, T11;

#define STORE_4x8(_COL, A, B, C, D) \
                T00 = _mm_unpacklo_epi16((A), (B)); \
                T01 = _mm_unpackhi_epi16((A), (B)); \
                T02 = _mm_unpacklo_epi16((C), (D)); \
                T03 = _mm_unpackhi_epi16((C), (D)); \
                T10 = _mm_unpacklo_epi32(T00, T02); \
                T11 = _mm_unpackhi_epi32(T00, T02); \
                _mm_storel_epi64( (__m128i*)&pDst[(i+0)*stride + (_COL)], T10 ); \
                _mm_storeh_pi   ( (__m64*  )&pDst[(i+1)*stride + (_COL)], _mm_castsi128_ps(T10)); \
                _mm_storel_epi64( (__m128i*)&pDst[(i+2)*stride + (_COL)], T11 ); \
                _mm_storeh_pi   ( (__m64*  )&pDst[(i+3)*stride + (_COL)], _mm_castsi128_ps(T11)); \
                T10 = _mm_unpacklo_epi32(T01, T03); \
                T11 = _mm_unpackhi_epi32(T01, T03); \
                _mm_storel_epi64( (__m128i*)&pDst[(i+4)*stride + (_COL)], T10 ); \
                _mm_storeh_pi   ( (__m64*  )&pDst[(i+5)*stride + (_COL)], _mm_castsi128_ps(T10)); \
                _mm_storel_epi64( (__m128i*)&pDst[(i+6)*stride + (_COL)], T11 ); \
                _mm_storeh_pi   ( (__m64*  )&pDst[(i+7)*stride + (_COL)], _mm_castsi128_ps(T11)); \


                STORE_4x8( 0, m128iS0,  m128iS1,  m128iS2,  m128iS3);
                STORE_4x8( 4, m128iS4,  m128iS5,  m128iS6,  m128iS7);
                STORE_4x8( 8, m128iS8,  m128iS9,  m128iS10, m128iS11);
                STORE_4x8(12, m128iS12, m128iS13, m128iS14, m128iS15);
                STORE_4x8(16, m128iS16, m128iS17, m128iS18, m128iS19);
                STORE_4x8(20, m128iS20, m128iS21, m128iS22, m128iS23);
                STORE_4x8(24, m128iS24, m128iS25, m128iS26, m128iS27);
                STORE_4x8(28, m128iS28, m128iS29, m128iS30, m128iS31);
#undef STORE_4x8

                if(i<=16){
                    int k = (i+8)*4;
                    m128iS0   = _mm_load_si128( (__m128i*)( pSrc + k) );
                    m128iS1   = _mm_load_si128( (__m128i*)( pSrc + 128 + k) );
                    m128iS2   = _mm_load_si128( (__m128i*)( pSrc + 256 + k ) );
                    m128iS3   = _mm_load_si128( (__m128i*)( pSrc + 384 + k ) );
                    m128iS4   = _mm_loadu_si128((__m128i*)( pSrc + 512 + k ) );
                    m128iS5   = _mm_load_si128( (__m128i*)( pSrc + 640 + k ) );
                    m128iS6   = _mm_load_si128( (__m128i*)( pSrc + 768 + k) );
                    m128iS7   = _mm_load_si128( (__m128i*)( pSrc + 896 + k) );
                    m128iS8   = _mm_load_si128( (__m128i*)( pSrc + 8 + k) );
                    m128iS9   = _mm_load_si128( (__m128i*)( pSrc + 128 +8 + k));
                    m128iS10  = _mm_load_si128( (__m128i*)( pSrc + 256  +8  + k) );
                    m128iS11  = _mm_load_si128( (__m128i*)( pSrc + 384 +8 + k));
                    m128iS12  = _mm_loadu_si128((__m128i*)( pSrc + 512 +8 + k) );
                    m128iS13  = _mm_load_si128( (__m128i*)( pSrc + 640 +8 + k) );
                    m128iS14  = _mm_load_si128( (__m128i*)( pSrc + 768 +8 + k) );
                    m128iS15  = _mm_load_si128( (__m128i*)( pSrc + 896 +8 + k) );
                    m128iS16  = _mm_load_si128( (__m128i*)( pSrc + 16 + k) );
                    m128iS17  = _mm_load_si128( (__m128i*)( pSrc + 128 +16 + k));
                    m128iS18  = _mm_load_si128( (__m128i*)( pSrc + 256  +16 + k) );
                    m128iS19  = _mm_load_si128( (__m128i*)( pSrc + 384 +16 + k));
                    m128iS20  = _mm_loadu_si128((__m128i*)( pSrc + 512 +16 + k) );
                    m128iS21  = _mm_load_si128( (__m128i*)( pSrc + 640 +16 + k) );
                    m128iS22  = _mm_load_si128( (__m128i*)( pSrc + 768 +16 + k) );
                    m128iS23  = _mm_load_si128( (__m128i*)( pSrc + 896 +16 + k) );
                    m128iS24  = _mm_load_si128( (__m128i*)( pSrc + 24 + k) );
                    m128iS25  = _mm_load_si128( (__m128i*)( pSrc + 128 +24 + k));
                    m128iS26  = _mm_load_si128( (__m128i*)( pSrc + 256  +24  + k) );
                    m128iS27  = _mm_load_si128( (__m128i*)( pSrc + 384 +24 + k));
                    m128iS28  = _mm_loadu_si128((__m128i*)( pSrc + 512 +24 + k) );
                    m128iS29  = _mm_load_si128( (__m128i*)( pSrc + 640 +24 + k) );
                    m128iS30  = _mm_load_si128( (__m128i*)( pSrc + 768 +24 + k) );
                    m128iS31  = _mm_load_si128( (__m128i*)( pSrc + 896 +24 + k) );
                }
            }
        }
    }
}
}

#include "utils.h"

namespace x265 {
// private x265 namespace

void NAME(Setup_Vec_DCTPrimitives)(EncoderPrimitives &p)
{
    p.deQuant = xDeQuant;
    p.dct[DST_4x4] = xDST4;
    p.dct[DCT_4x4] = xDCT4;
    p.dct[DCT_8x8] = xDCT8;
    p.dct[DCT_16x16] = xDCT16;
    p.dct[DCT_32x32] = xDCT32;
    p.idct[IDST_4x4] = xIDST4;
    p.idct[IDCT_4x4] = xIDCT4;
    p.idct[IDCT_8x8] = xIDCT8;
    p.idct[IDCT_16x16] = xIDCT16;
    p.idct[IDCT_32x32] = xIDCT32;
}
}
