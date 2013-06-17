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

void xIDCT32(short *pSrc, short *pDst, intptr_t stride)
{
	//Odd
	const __m128i c16_p90_p90   = _mm_set1_epi32(0x005A005A);//column 0
	const __m128i c16_p85_p88   = _mm_set1_epi32(0x00550058);
	const __m128i c16_p78_p82   = _mm_set1_epi32(0x004E0052);
	const __m128i c16_p67_p73   = _mm_set1_epi32(0x00430049);
	const __m128i c16_p54_p61   = _mm_set1_epi32(0x0036003D);
	const __m128i c16_p38_p46   = _mm_set1_epi32(0x0026002E);
	const __m128i c16_p22_p31   = _mm_set1_epi32(0x0016001F);
	const __m128i c16_p04_p13   = _mm_set1_epi32(0x0004000D);
	const __m128i c16_p82_p90   = _mm_set1_epi32(0x0052005A);//column 1
	const __m128i c16_p46_p67   = _mm_set1_epi32(0x002E0043);
	const __m128i c16_n04_p22   = _mm_set1_epi32(0xFFFC0016);
	const __m128i c16_n54_n31   = _mm_set1_epi32(0xFFCAFFE1);
	const __m128i c16_n85_n73   = _mm_set1_epi32(0xFFABFFB7);
	const __m128i c16_n88_n90   = _mm_set1_epi32(0xFFA8FFA6);
	const __m128i c16_n61_n78   = _mm_set1_epi32(0xFFC3FFB2);
	const __m128i c16_n13_n38   = _mm_set1_epi32(0xFFF3FFDA);
	const __m128i c16_p67_p88   = _mm_set1_epi32(0x00430058);//column 2
	const __m128i c16_n13_p31   = _mm_set1_epi32(0xFFF3001F);
	const __m128i c16_n82_n54   = _mm_set1_epi32(0xFFAEFFCA);
	const __m128i c16_n78_n90   = _mm_set1_epi32(0xFFB2FFA6);
	const __m128i c16_n04_n46   = _mm_set1_epi32(0xFFFCFFD2);
	const __m128i c16_p73_p38   = _mm_set1_epi32(0x00490026);
	const __m128i c16_p85_p90   = _mm_set1_epi32(0x0055005A);
	const __m128i c16_p22_p61   = _mm_set1_epi32(0x0016003D);
	const __m128i c16_p46_p85   = _mm_set1_epi32(0x002E0055);//column 3
	const __m128i c16_n67_n13   = _mm_set1_epi32(0xFFBDFFF3);
	const __m128i c16_n73_n90   = _mm_set1_epi32(0xFFB7FFA6);
	const __m128i c16_p38_n22   = _mm_set1_epi32(0x0026FFEA);
	const __m128i c16_p88_p82   = _mm_set1_epi32(0x00580052);
	const __m128i c16_n04_p54   = _mm_set1_epi32(0xFFFC0036);
	const __m128i c16_n90_n61   = _mm_set1_epi32(0xFFA6FFC3);
	const __m128i c16_n31_n78   = _mm_set1_epi32(0xFFE1FFB2);
	const __m128i c16_p22_p82   = _mm_set1_epi32(0x00160052);//column 4
	const __m128i c16_n90_n54   = _mm_set1_epi32(0xFFA6FFCA);
	const __m128i c16_p13_n61   = _mm_set1_epi32(0x000DFFC3);
	const __m128i c16_p85_p78   = _mm_set1_epi32(0x0055004E);
	const __m128i c16_n46_p31   = _mm_set1_epi32(0xFFD2001F);
	const __m128i c16_n67_n90   = _mm_set1_epi32(0xFFBDFFA6);
	const __m128i c16_p73_p04   = _mm_set1_epi32(0x00490004);
	const __m128i c16_p38_p88   = _mm_set1_epi32(0x00260058);
	const __m128i c16_n04_p78   = _mm_set1_epi32(0xFFFC004E);//column 5
	const __m128i c16_n73_n82   = _mm_set1_epi32(0xFFB7FFAE);
	const __m128i c16_p85_p13   = _mm_set1_epi32(0x0055000D);
	const __m128i c16_n22_p67   = _mm_set1_epi32(0xFFEA0043);
	const __m128i c16_n61_n88   = _mm_set1_epi32(0xFFC3FFA8);
	const __m128i c16_p90_p31   = _mm_set1_epi32(0x005A001F);
	const __m128i c16_n38_p54   = _mm_set1_epi32(0xFFDA0036);
	const __m128i c16_n46_n90   = _mm_set1_epi32(0xFFD2FFA6);
	const __m128i c16_n31_p73   = _mm_set1_epi32(0xFFE10049);//column 6
	const __m128i c16_n22_n90   = _mm_set1_epi32(0xFFEAFFA6);
	const __m128i c16_p67_p78   = _mm_set1_epi32(0x0043004E);
	const __m128i c16_n90_n38   = _mm_set1_epi32(0xFFA6FFDA);
	const __m128i c16_p82_n13   = _mm_set1_epi32(0x0052FFF3);
	const __m128i c16_n46_p61   = _mm_set1_epi32(0xFFD2003D);
	const __m128i c16_n04_n88   = _mm_set1_epi32(0xFFFCFFA8);
	const __m128i c16_p54_p85   = _mm_set1_epi32(0x00360055);
	const __m128i c16_n54_p67   = _mm_set1_epi32(0xFFCA0043);//column 7
	const __m128i c16_p38_n78   = _mm_set1_epi32(0x0026FFB2);
	const __m128i c16_n22_p85   = _mm_set1_epi32(0xFFEA0055);
	const __m128i c16_p04_n90   = _mm_set1_epi32(0x0004FFA6);
	const __m128i c16_p13_p90   = _mm_set1_epi32(0x000D005A);
	const __m128i c16_n31_n88   = _mm_set1_epi32(0xFFE1FFA8);
	const __m128i c16_p46_p82   = _mm_set1_epi32(0x002E0052);
	const __m128i c16_n61_n73   = _mm_set1_epi32(0xFFC3FFB7);
	const __m128i c16_n73_p61   = _mm_set1_epi32(0xFFB7003D);//column 8
	const __m128i c16_p82_n46   = _mm_set1_epi32(0x0052FFD2);
	const __m128i c16_n88_p31   = _mm_set1_epi32(0xFFA8001F);
	const __m128i c16_p90_n13   = _mm_set1_epi32(0x005AFFF3);
	const __m128i c16_n90_n04   = _mm_set1_epi32(0xFFA6FFFC);
	const __m128i c16_p85_p22   = _mm_set1_epi32(0x00550016);
	const __m128i c16_n78_n38   = _mm_set1_epi32(0xFFB2FFDA);
	const __m128i c16_p67_p54   = _mm_set1_epi32(0x00430036);
	const __m128i c16_n85_p54   = _mm_set1_epi32(0xFFAB0036);//column 9
	const __m128i c16_p88_n04   = _mm_set1_epi32(0x0058FFFC);
	const __m128i c16_n61_n46   = _mm_set1_epi32(0xFFC3FFD2);
	const __m128i c16_p13_p82   = _mm_set1_epi32(0x000D0052);
	const __m128i c16_p38_n90   = _mm_set1_epi32(0x0026FFA6);
	const __m128i c16_n78_p67   = _mm_set1_epi32(0xFFB20043);
	const __m128i c16_p90_n22   = _mm_set1_epi32(0x005AFFEA);
	const __m128i c16_n73_n31   = _mm_set1_epi32(0xFFB7FFE1);
	const __m128i c16_n90_p46   = _mm_set1_epi32(0xFFA6002E);//column 10
	const __m128i c16_p54_p38   = _mm_set1_epi32(0x00360026);
	const __m128i c16_p31_n90   = _mm_set1_epi32(0x001FFFA6);
	const __m128i c16_n88_p61   = _mm_set1_epi32(0xFFA8003D);
	const __m128i c16_p67_p22   = _mm_set1_epi32(0x00430016);
	const __m128i c16_p13_n85   = _mm_set1_epi32(0x000DFFAB);
	const __m128i c16_n82_p73   = _mm_set1_epi32(0xFFAE0049);
	const __m128i c16_p78_p04   = _mm_set1_epi32(0x004E0004);
	const __m128i c16_n88_p38   = _mm_set1_epi32(0xFFA80026);//column 11
	const __m128i c16_n04_p73   = _mm_set1_epi32(0xFFFC0049);
	const __m128i c16_p90_n67   = _mm_set1_epi32(0x005AFFBD);
	const __m128i c16_n31_n46   = _mm_set1_epi32(0xFFE1FFD2);
	const __m128i c16_n78_p85   = _mm_set1_epi32(0xFFB20055);
	const __m128i c16_p61_p13   = _mm_set1_epi32(0x003D000D);
	const __m128i c16_p54_n90   = _mm_set1_epi32(0x0036FFA6);
	const __m128i c16_n82_p22   = _mm_set1_epi32(0xFFAE0016);
	const __m128i c16_n78_p31   = _mm_set1_epi32(0xFFB2001F);//column 12
	const __m128i c16_n61_p90   = _mm_set1_epi32(0xFFC3005A);
	const __m128i c16_p54_p04   = _mm_set1_epi32(0x00360004);
	const __m128i c16_p82_n88   = _mm_set1_epi32(0x0052FFA8);
	const __m128i c16_n22_n38   = _mm_set1_epi32(0xFFEAFFDA);
	const __m128i c16_n90_p73   = _mm_set1_epi32(0xFFA60049);
	const __m128i c16_n13_p67   = _mm_set1_epi32(0xFFF30043);
	const __m128i c16_p85_n46   = _mm_set1_epi32(0x0055FFD2);
	const __m128i c16_n61_p22   = _mm_set1_epi32(0xFFC30016);//column 13
	const __m128i c16_n90_p85   = _mm_set1_epi32(0xFFA60055);
	const __m128i c16_n38_p73   = _mm_set1_epi32(0xFFDA0049);
	const __m128i c16_p46_n04   = _mm_set1_epi32(0x002EFFFC);
	const __m128i c16_p90_n78   = _mm_set1_epi32(0x005AFFB2);
	const __m128i c16_p54_n82   = _mm_set1_epi32(0x0036FFAE);
	const __m128i c16_n31_n13   = _mm_set1_epi32(0xFFE1FFF3);
	const __m128i c16_n88_p67   = _mm_set1_epi32(0xFFA80043);
	const __m128i c16_n38_p13   = _mm_set1_epi32(0xFFDA000D);//column 14
	const __m128i c16_n78_p61   = _mm_set1_epi32(0xFFB2003D);
	const __m128i c16_n90_p88   = _mm_set1_epi32(0xFFA60058);
	const __m128i c16_n73_p85   = _mm_set1_epi32(0xFFB70055);
	const __m128i c16_n31_p54   = _mm_set1_epi32(0xFFE10036);
	const __m128i c16_p22_p04   = _mm_set1_epi32(0x00160004);
	const __m128i c16_p67_n46   = _mm_set1_epi32(0x0043FFD2);
	const __m128i c16_p90_n82   = _mm_set1_epi32(0x005AFFAE);
	const __m128i c16_n13_p04   = _mm_set1_epi32(0xFFF30004);//column 15
	const __m128i c16_n31_p22   = _mm_set1_epi32(0xFFE10016);
	const __m128i c16_n46_p38   = _mm_set1_epi32(0xFFD20026);
	const __m128i c16_n61_p54   = _mm_set1_epi32(0xFFC30036);
	const __m128i c16_n73_p67   = _mm_set1_epi32(0xFFB70043);
	const __m128i c16_n82_p78   = _mm_set1_epi32(0xFFAE004E);
	const __m128i c16_n88_p85   = _mm_set1_epi32(0xFFA80055);
	const __m128i c16_n90_p90   = _mm_set1_epi32(0xFFA6005A);

	//EO
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
	//EEO
	const __m128i c16_p75_p89   = _mm_set1_epi32(0x004B0059);
	const __m128i c16_p18_p50   = _mm_set1_epi32(0x00120032);
	const __m128i c16_n18_p75   = _mm_set1_epi32(0xFFEE004B);
	const __m128i c16_n50_n89   = _mm_set1_epi32(0xFFCEFFA7);
	const __m128i c16_n89_p50   = _mm_set1_epi32(0xFFA70032);
	const __m128i c16_p75_p18   = _mm_set1_epi32(0x004B0012);
	const __m128i c16_n50_p18   = _mm_set1_epi32(0xFFCE0012);
	const __m128i c16_n89_p75   = _mm_set1_epi32(0xFFA7004B);
	//EEEO
	const __m128i c16_p36_p83   = _mm_set1_epi32(0x00240053);
	const __m128i c16_n83_p36   = _mm_set1_epi32(0xFFAD0024);
	//EEEE
	const __m128i c16_n64_p64   = _mm_set1_epi32(0xFFC00040);
	const __m128i c16_p64_p64   = _mm_set1_epi32(0x00400040);
	__m128i c32_rnd             = _mm_set1_epi32(64);

	Int nShift = 7;

	// DCT1
	__m128i in00[4], in01[4], in02[4], in03[4], in04[4], in05[4], in06[4], in07[4], in08[4], in09[4], in10[4], in11[4], in12[4], in13[4], in14[4], in15[4];
	__m128i in16[4], in17[4], in18[4], in19[4], in20[4], in21[4], in22[4], in23[4], in24[4], in25[4], in26[4], in27[4], in28[4], in29[4], in30[4], in31[4];
	__m128i res00[4], res01[4], res02[4], res03[4], res04[4], res05[4], res06[4], res07[4], res08[4], res09[4], res10[4], res11[4], res12[4], res13[4], res14[4], res15[4];
	__m128i res16[4], res17[4], res18[4], res19[4], res20[4], res21[4], res22[4], res23[4], res24[4], res25[4], res26[4], res27[4], res28[4], res29[4], res30[4], res31[4];

	for (Int i=0; i<4; i++) {
		Int offset = (i<<3);

		in00[i]  = _mm_loadu_si128((const __m128i*)&pSrc[ 0*32+offset]); // [07 06 05 04 03 02 01 00]
		in01[i]  = _mm_loadu_si128((const __m128i*)&pSrc[ 1*32+offset]); // [17 16 15 14 13 12 11 10]
		in02[i]  = _mm_loadu_si128((const __m128i*)&pSrc[ 2*32+offset]); // [27 26 25 24 23 22 21 20]
		in03[i]  = _mm_loadu_si128((const __m128i*)&pSrc[ 3*32+offset]); // [37 36 35 34 33 32 31 30]
		in04[i]  = _mm_loadu_si128((const __m128i*)&pSrc[ 4*32+offset]); // [47 46 45 44 43 42 41 40]
		in05[i]  = _mm_loadu_si128((const __m128i*)&pSrc[ 5*32+offset]); // [57 56 55 54 53 52 51 50]
		in06[i]  = _mm_loadu_si128((const __m128i*)&pSrc[ 6*32+offset]); // [67 66 65 64 63 62 61 60]
		in07[i]  = _mm_loadu_si128((const __m128i*)&pSrc[ 7*32+offset]); // [77 76 75 74 73 72 71 70]
		in08[i]  = _mm_loadu_si128((const __m128i*)&pSrc[ 8*32+offset]); // [x7~x0]row08
		in09[i]  = _mm_loadu_si128((const __m128i*)&pSrc[ 9*32+offset]); // [x7~x0]row09
		in10[i]  = _mm_loadu_si128((const __m128i*)&pSrc[10*32+offset]); // [x7~x0]row10
		in11[i]  = _mm_loadu_si128((const __m128i*)&pSrc[11*32+offset]); // [x7~x0]row11
		in12[i]  = _mm_loadu_si128((const __m128i*)&pSrc[12*32+offset]); // [x7~x0]row12
		in13[i]  = _mm_loadu_si128((const __m128i*)&pSrc[13*32+offset]); // [x7~x0]row13
		in14[i]  = _mm_loadu_si128((const __m128i*)&pSrc[14*32+offset]); // [x7~x0]row14
		in15[i]  = _mm_loadu_si128((const __m128i*)&pSrc[15*32+offset]); // [x7~x0]row15
		in16[i]  = _mm_loadu_si128((const __m128i*)&pSrc[16*32+offset]); // 
		in17[i]  = _mm_loadu_si128((const __m128i*)&pSrc[17*32+offset]); // 
		in18[i]  = _mm_loadu_si128((const __m128i*)&pSrc[18*32+offset]); // 
		in19[i]  = _mm_loadu_si128((const __m128i*)&pSrc[19*32+offset]); //
		in20[i]  = _mm_loadu_si128((const __m128i*)&pSrc[20*32+offset]); // 
		in21[i]  = _mm_loadu_si128((const __m128i*)&pSrc[21*32+offset]); //
		in22[i]  = _mm_loadu_si128((const __m128i*)&pSrc[22*32+offset]); // 
		in23[i]  = _mm_loadu_si128((const __m128i*)&pSrc[23*32+offset]); // 
		in24[i]  = _mm_loadu_si128((const __m128i*)&pSrc[24*32+offset]); // 
		in25[i]  = _mm_loadu_si128((const __m128i*)&pSrc[25*32+offset]); // 
		in26[i]  = _mm_loadu_si128((const __m128i*)&pSrc[26*32+offset]); // 
		in27[i]  = _mm_loadu_si128((const __m128i*)&pSrc[27*32+offset]); // 
		in28[i]  = _mm_loadu_si128((const __m128i*)&pSrc[28*32+offset]); // 
		in29[i]  = _mm_loadu_si128((const __m128i*)&pSrc[29*32+offset]); // 
		in30[i]  = _mm_loadu_si128((const __m128i*)&pSrc[30*32+offset]); // 
		in31[i]  = _mm_loadu_si128((const __m128i*)&pSrc[31*32+offset]); //
	}

	for(Int pass=0; pass<2; pass++) {
		if (pass==1) {
			c32_rnd = _mm_set1_epi32(2048);
			nShift  = 12;  
		}

		for(Int part=0; part<4; part++) {
			const __m128i T_00_00A = _mm_unpacklo_epi16(in01[part], in03[part]);             // [33 13 32 12 31 11 30 10]
			const __m128i T_00_00B = _mm_unpackhi_epi16(in01[part], in03[part]);             // [37 17 36 16 35 15 34 14]
			const __m128i T_00_01A = _mm_unpacklo_epi16(in05[part], in07[part]);             // [ ]
			const __m128i T_00_01B = _mm_unpackhi_epi16(in05[part], in07[part]);             // [ ]
			const __m128i T_00_02A = _mm_unpacklo_epi16(in09[part], in11[part]);             // [ ]
			const __m128i T_00_02B = _mm_unpackhi_epi16(in09[part], in11[part]);             // [ ]
			const __m128i T_00_03A = _mm_unpacklo_epi16(in13[part], in15[part]);             // [ ]
			const __m128i T_00_03B = _mm_unpackhi_epi16(in13[part], in15[part]);             // [ ]
			const __m128i T_00_04A = _mm_unpacklo_epi16(in17[part], in19[part]);             // [ ]
			const __m128i T_00_04B = _mm_unpackhi_epi16(in17[part], in19[part]);             // [ ]
			const __m128i T_00_05A = _mm_unpacklo_epi16(in21[part], in23[part]);             // [ ]
			const __m128i T_00_05B = _mm_unpackhi_epi16(in21[part], in23[part]);             // [ ]
			const __m128i T_00_06A = _mm_unpacklo_epi16(in25[part], in27[part]);             // [ ]
			const __m128i T_00_06B = _mm_unpackhi_epi16(in25[part], in27[part]);             // [ ]
			const __m128i T_00_07A = _mm_unpacklo_epi16(in29[part], in31[part]);             // 
			const __m128i T_00_07B = _mm_unpackhi_epi16(in29[part], in31[part]);             // [ ]

			const __m128i T_00_08A = _mm_unpacklo_epi16(in02[part], in06[part]);             // [ ]
			const __m128i T_00_08B = _mm_unpackhi_epi16(in02[part], in06[part]);             // [ ]
			const __m128i T_00_09A = _mm_unpacklo_epi16(in10[part], in14[part]);             // [ ]
			const __m128i T_00_09B = _mm_unpackhi_epi16(in10[part], in14[part]);             // [ ]
			const __m128i T_00_10A = _mm_unpacklo_epi16(in18[part], in22[part]);             // [ ]
			const __m128i T_00_10B = _mm_unpackhi_epi16(in18[part], in22[part]);             // [ ]
			const __m128i T_00_11A = _mm_unpacklo_epi16(in26[part], in30[part]);             // [ ]
			const __m128i T_00_11B = _mm_unpackhi_epi16(in26[part], in30[part]);             // [ ]

			const __m128i T_00_12A = _mm_unpacklo_epi16(in04[part], in12[part]);             // [ ]
			const __m128i T_00_12B = _mm_unpackhi_epi16(in04[part], in12[part]);             // [ ]
			const __m128i T_00_13A = _mm_unpacklo_epi16(in20[part], in28[part]);             // [ ]
			const __m128i T_00_13B = _mm_unpackhi_epi16(in20[part], in28[part]);             // [ ]

			const __m128i T_00_14A = _mm_unpacklo_epi16(in08[part], in24[part]);             // 
			const __m128i T_00_14B = _mm_unpackhi_epi16(in08[part], in24[part]);             // [ ]
			const __m128i T_00_15A = _mm_unpacklo_epi16(in00[part], in16[part]);             // 
			const __m128i T_00_15B = _mm_unpackhi_epi16(in00[part], in16[part]);             // [ ]

			__m128i O00A, O01A, O02A, O03A, O04A, O05A, O06A, O07A, O08A, O09A, O10A, O11A, O12A, O13A, O14A, O15A;
			__m128i O00B, O01B, O02B, O03B, O04B, O05B, O06B, O07B, O08B, O09B, O10B, O11B, O12B, O13B, O14B, O15B;
			{
				__m128i T00, T01, T02, T03;
	#define COMPUTE_ROW(r0103, r0507, r0911, r1315, r1719, r2123, r2527, r2931, c0103, c0507, c0911, c1315, c1719, c2123, c2527, c2931, row)\
				T00 = _mm_add_epi32(_mm_madd_epi16(r0103, c0103), _mm_madd_epi16(r0507, c0507));\
				T01 = _mm_add_epi32(_mm_madd_epi16(r0911, c0911), _mm_madd_epi16(r1315, c1315));\
				T02 = _mm_add_epi32(_mm_madd_epi16(r1719, c1719), _mm_madd_epi16(r2123, c2123));\
				T03 = _mm_add_epi32(_mm_madd_epi16(r2527, c2527), _mm_madd_epi16(r2931, c2931));\
				row = _mm_add_epi32(_mm_add_epi32(T00, T01), _mm_add_epi32(T02, T03));
			
				COMPUTE_ROW(T_00_00A,T_00_01A,T_00_02A,T_00_03A, T_00_04A,T_00_05A,T_00_06A,T_00_07A,\
					c16_p90_p90, c16_p85_p88, c16_p78_p82, c16_p67_p73, c16_p54_p61, c16_p38_p46, c16_p22_p31, c16_p04_p13, O00A)
				COMPUTE_ROW(T_00_00A,T_00_01A,T_00_02A,T_00_03A, T_00_04A,T_00_05A,T_00_06A,T_00_07A,\
				c16_p82_p90, c16_p46_p67, c16_n04_p22, c16_n54_n31, c16_n85_n73, c16_n88_n90, c16_n61_n78, c16_n13_n38, O01A)
				COMPUTE_ROW(T_00_00A,T_00_01A,T_00_02A,T_00_03A, T_00_04A,T_00_05A,T_00_06A,T_00_07A,\
				c16_p67_p88, c16_n13_p31, c16_n82_n54, c16_n78_n90, c16_n04_n46, c16_p73_p38, c16_p85_p90, c16_p22_p61, O02A)
				COMPUTE_ROW(T_00_00A,T_00_01A,T_00_02A,T_00_03A, T_00_04A,T_00_05A,T_00_06A,T_00_07A,\
				c16_p46_p85, c16_n67_n13, c16_n73_n90, c16_p38_n22, c16_p88_p82, c16_n04_p54, c16_n90_n61, c16_n31_n78, O03A)
				COMPUTE_ROW(T_00_00A,T_00_01A,T_00_02A,T_00_03A, T_00_04A,T_00_05A,T_00_06A,T_00_07A,\
				c16_p22_p82, c16_n90_n54, c16_p13_n61, c16_p85_p78, c16_n46_p31, c16_n67_n90, c16_p73_p04, c16_p38_p88, O04A)
				COMPUTE_ROW(T_00_00A,T_00_01A,T_00_02A,T_00_03A, T_00_04A,T_00_05A,T_00_06A,T_00_07A,\
				c16_n04_p78, c16_n73_n82, c16_p85_p13, c16_n22_p67, c16_n61_n88, c16_p90_p31, c16_n38_p54, c16_n46_n90, O05A)
				COMPUTE_ROW(T_00_00A,T_00_01A,T_00_02A,T_00_03A, T_00_04A,T_00_05A,T_00_06A,T_00_07A,\
				c16_n31_p73, c16_n22_n90, c16_p67_p78, c16_n90_n38, c16_p82_n13, c16_n46_p61, c16_n04_n88, c16_p54_p85, O06A)
				COMPUTE_ROW(T_00_00A,T_00_01A,T_00_02A,T_00_03A, T_00_04A,T_00_05A,T_00_06A,T_00_07A,\
				c16_n54_p67, c16_p38_n78, c16_n22_p85, c16_p04_n90, c16_p13_p90, c16_n31_n88, c16_p46_p82, c16_n61_n73, O07A)
				COMPUTE_ROW(T_00_00A,T_00_01A,T_00_02A,T_00_03A, T_00_04A,T_00_05A,T_00_06A,T_00_07A,\
				c16_n73_p61, c16_p82_n46, c16_n88_p31, c16_p90_n13, c16_n90_n04, c16_p85_p22, c16_n78_n38, c16_p67_p54, O08A)
				COMPUTE_ROW(T_00_00A,T_00_01A,T_00_02A,T_00_03A, T_00_04A,T_00_05A,T_00_06A,T_00_07A,\
				c16_n85_p54, c16_p88_n04, c16_n61_n46, c16_p13_p82, c16_p38_n90, c16_n78_p67, c16_p90_n22, c16_n73_n31, O09A)
				COMPUTE_ROW(T_00_00A,T_00_01A,T_00_02A,T_00_03A, T_00_04A,T_00_05A,T_00_06A,T_00_07A,\
				c16_n90_p46, c16_p54_p38, c16_p31_n90, c16_n88_p61, c16_p67_p22, c16_p13_n85, c16_n82_p73, c16_p78_p04, O10A)
				COMPUTE_ROW(T_00_00A,T_00_01A,T_00_02A,T_00_03A, T_00_04A,T_00_05A,T_00_06A,T_00_07A,\
				c16_n88_p38, c16_n04_p73, c16_p90_n67, c16_n31_n46, c16_n78_p85, c16_p61_p13, c16_p54_n90, c16_n82_p22, O11A)
				COMPUTE_ROW(T_00_00A,T_00_01A,T_00_02A,T_00_03A, T_00_04A,T_00_05A,T_00_06A,T_00_07A,\
				c16_n78_p31, c16_n61_p90, c16_p54_p04, c16_p82_n88, c16_n22_n38, c16_n90_p73, c16_n13_p67, c16_p85_n46, O12A)
				COMPUTE_ROW(T_00_00A,T_00_01A,T_00_02A,T_00_03A, T_00_04A,T_00_05A,T_00_06A,T_00_07A,\
				c16_n61_p22, c16_n90_p85, c16_n38_p73, c16_p46_n04, c16_p90_n78, c16_p54_n82, c16_n31_n13, c16_n88_p67, O13A)
				COMPUTE_ROW(T_00_00A,T_00_01A,T_00_02A,T_00_03A, T_00_04A,T_00_05A,T_00_06A,T_00_07A,\
				c16_n38_p13, c16_n78_p61, c16_n90_p88, c16_n73_p85, c16_n31_p54, c16_p22_p04, c16_p67_n46, c16_p90_n82, O14A)
				COMPUTE_ROW(T_00_00A,T_00_01A,T_00_02A,T_00_03A, T_00_04A,T_00_05A,T_00_06A,T_00_07A,\
				c16_n13_p04, c16_n31_p22, c16_n46_p38, c16_n61_p54, c16_n73_p67, c16_n82_p78, c16_n88_p85, c16_n90_p90, O15A)

				COMPUTE_ROW(T_00_00B,T_00_01B,T_00_02B,T_00_03B, T_00_04B,T_00_05B,T_00_06B,T_00_07B,\
				c16_p90_p90, c16_p85_p88, c16_p78_p82, c16_p67_p73, c16_p54_p61, c16_p38_p46, c16_p22_p31, c16_p04_p13, O00B)
				COMPUTE_ROW(T_00_00B,T_00_01B,T_00_02B,T_00_03B, T_00_04B,T_00_05B,T_00_06B,T_00_07B,\
				c16_p82_p90, c16_p46_p67, c16_n04_p22, c16_n54_n31, c16_n85_n73, c16_n88_n90, c16_n61_n78, c16_n13_n38, O01B)
				COMPUTE_ROW(T_00_00B,T_00_01B,T_00_02B,T_00_03B, T_00_04B,T_00_05B,T_00_06B,T_00_07B,\
				c16_p67_p88, c16_n13_p31, c16_n82_n54, c16_n78_n90, c16_n04_n46, c16_p73_p38, c16_p85_p90, c16_p22_p61, O02B)
				COMPUTE_ROW(T_00_00B,T_00_01B,T_00_02B,T_00_03B, T_00_04B,T_00_05B,T_00_06B,T_00_07B,\
				c16_p46_p85, c16_n67_n13, c16_n73_n90, c16_p38_n22, c16_p88_p82, c16_n04_p54, c16_n90_n61, c16_n31_n78, O03B)
				COMPUTE_ROW(T_00_00B,T_00_01B,T_00_02B,T_00_03B, T_00_04B,T_00_05B,T_00_06B,T_00_07B,\
				c16_p22_p82, c16_n90_n54, c16_p13_n61, c16_p85_p78, c16_n46_p31, c16_n67_n90, c16_p73_p04, c16_p38_p88, O04B)
				COMPUTE_ROW(T_00_00B,T_00_01B,T_00_02B,T_00_03B, T_00_04B,T_00_05B,T_00_06B,T_00_07B,\
				c16_n04_p78, c16_n73_n82, c16_p85_p13, c16_n22_p67, c16_n61_n88, c16_p90_p31, c16_n38_p54, c16_n46_n90, O05B)
				COMPUTE_ROW(T_00_00B,T_00_01B,T_00_02B,T_00_03B, T_00_04B,T_00_05B,T_00_06B,T_00_07B,\
				c16_n31_p73, c16_n22_n90, c16_p67_p78, c16_n90_n38, c16_p82_n13, c16_n46_p61, c16_n04_n88, c16_p54_p85, O06B)
				COMPUTE_ROW(T_00_00B,T_00_01B,T_00_02B,T_00_03B, T_00_04B,T_00_05B,T_00_06B,T_00_07B,\
				c16_n54_p67, c16_p38_n78, c16_n22_p85, c16_p04_n90, c16_p13_p90, c16_n31_n88, c16_p46_p82, c16_n61_n73, O07B)
				COMPUTE_ROW(T_00_00B,T_00_01B,T_00_02B,T_00_03B, T_00_04B,T_00_05B,T_00_06B,T_00_07B,\
				c16_n73_p61, c16_p82_n46, c16_n88_p31, c16_p90_n13, c16_n90_n04, c16_p85_p22, c16_n78_n38, c16_p67_p54, O08B)
				COMPUTE_ROW(T_00_00B,T_00_01B,T_00_02B,T_00_03B, T_00_04B,T_00_05B,T_00_06B,T_00_07B,\
				c16_n85_p54, c16_p88_n04, c16_n61_n46, c16_p13_p82, c16_p38_n90, c16_n78_p67, c16_p90_n22, c16_n73_n31, O09B)
				COMPUTE_ROW(T_00_00B,T_00_01B,T_00_02B,T_00_03B, T_00_04B,T_00_05B,T_00_06B,T_00_07B,\
				c16_n90_p46, c16_p54_p38, c16_p31_n90, c16_n88_p61, c16_p67_p22, c16_p13_n85, c16_n82_p73, c16_p78_p04, O10B)
				COMPUTE_ROW(T_00_00B,T_00_01B,T_00_02B,T_00_03B, T_00_04B,T_00_05B,T_00_06B,T_00_07B,\
				c16_n88_p38, c16_n04_p73, c16_p90_n67, c16_n31_n46, c16_n78_p85, c16_p61_p13, c16_p54_n90, c16_n82_p22, O11B)
				COMPUTE_ROW(T_00_00B,T_00_01B,T_00_02B,T_00_03B, T_00_04B,T_00_05B,T_00_06B,T_00_07B,\
				c16_n78_p31, c16_n61_p90, c16_p54_p04, c16_p82_n88, c16_n22_n38, c16_n90_p73, c16_n13_p67, c16_p85_n46, O12B)
				COMPUTE_ROW(T_00_00B,T_00_01B,T_00_02B,T_00_03B, T_00_04B,T_00_05B,T_00_06B,T_00_07B,\
				c16_n61_p22, c16_n90_p85, c16_n38_p73, c16_p46_n04, c16_p90_n78, c16_p54_n82, c16_n31_n13, c16_n88_p67, O13B)
				COMPUTE_ROW(T_00_00B,T_00_01B,T_00_02B,T_00_03B, T_00_04B,T_00_05B,T_00_06B,T_00_07B,\
				c16_n38_p13, c16_n78_p61, c16_n90_p88, c16_n73_p85, c16_n31_p54, c16_p22_p04, c16_p67_n46, c16_p90_n82, O14B)
				COMPUTE_ROW(T_00_00B,T_00_01B,T_00_02B,T_00_03B, T_00_04B,T_00_05B,T_00_06B,T_00_07B,\
				c16_n13_p04, c16_n31_p22, c16_n46_p38, c16_n61_p54, c16_n73_p67, c16_n82_p78, c16_n88_p85, c16_n90_p90, O15B)

	#undef COMPUTE_ROW
			}

			__m128i EO0A, EO1A, EO2A, EO3A, EO4A, EO5A, EO6A, EO7A;
			__m128i EO0B, EO1B, EO2B, EO3B, EO4B, EO5B, EO6B, EO7B;
			{
				__m128i T00, T01;
#define COMPUTE_ROW(row0206, row1014, row1822, row2630, c0206, c1014, c1822, c2630, row)\
				T00 = _mm_add_epi32(_mm_madd_epi16(row0206, c0206), _mm_madd_epi16(row1014, c1014));\
				T01 = _mm_add_epi32(_mm_madd_epi16(row1822, c1822), _mm_madd_epi16(row2630, c2630));\
				row = _mm_add_epi32(T00, T01);

				COMPUTE_ROW(T_00_08A,T_00_09A,T_00_10A,T_00_11A, c16_p87_p90,c16_p70_p80,c16_p43_p57,c16_p09_p25, EO0A)
					COMPUTE_ROW(T_00_08A,T_00_09A,T_00_10A,T_00_11A, c16_p57_p87,c16_n43_p09,c16_n90_n80,c16_n25_n70, EO1A)
					COMPUTE_ROW(T_00_08A,T_00_09A,T_00_10A,T_00_11A, c16_p09_p80,c16_n87_n70,c16_p57_n25,c16_p43_p90, EO2A)
					COMPUTE_ROW(T_00_08A,T_00_09A,T_00_10A,T_00_11A, c16_n43_p70,c16_p09_n87,c16_p25_p90,c16_n57_n80, EO3A)
					COMPUTE_ROW(T_00_08A,T_00_09A,T_00_10A,T_00_11A, c16_n80_p57,c16_p90_n25,c16_n87_n09,c16_p70_p43, EO4A)
					COMPUTE_ROW(T_00_08A,T_00_09A,T_00_10A,T_00_11A, c16_n90_p43,c16_p25_p57,c16_p70_n87,c16_n80_p09, EO5A)
					COMPUTE_ROW(T_00_08A,T_00_09A,T_00_10A,T_00_11A, c16_n70_p25,c16_n80_p90,c16_p09_p43,c16_p87_n57, EO6A)
					COMPUTE_ROW(T_00_08A,T_00_09A,T_00_10A,T_00_11A, c16_n25_p09,c16_n57_p43,c16_n80_p70,c16_n90_p87, EO7A)

					COMPUTE_ROW(T_00_08B,T_00_09B,T_00_10B,T_00_11B, c16_p87_p90,c16_p70_p80,c16_p43_p57,c16_p09_p25, EO0B)
					COMPUTE_ROW(T_00_08B,T_00_09B,T_00_10B,T_00_11B, c16_p57_p87,c16_n43_p09,c16_n90_n80,c16_n25_n70, EO1B)
					COMPUTE_ROW(T_00_08B,T_00_09B,T_00_10B,T_00_11B, c16_p09_p80,c16_n87_n70,c16_p57_n25,c16_p43_p90, EO2B)
					COMPUTE_ROW(T_00_08B,T_00_09B,T_00_10B,T_00_11B, c16_n43_p70,c16_p09_n87,c16_p25_p90,c16_n57_n80, EO3B)
					COMPUTE_ROW(T_00_08B,T_00_09B,T_00_10B,T_00_11B, c16_n80_p57,c16_p90_n25,c16_n87_n09,c16_p70_p43, EO4B)
					COMPUTE_ROW(T_00_08B,T_00_09B,T_00_10B,T_00_11B, c16_n90_p43,c16_p25_p57,c16_p70_n87,c16_n80_p09, EO5B)
					COMPUTE_ROW(T_00_08B,T_00_09B,T_00_10B,T_00_11B, c16_n70_p25,c16_n80_p90,c16_p09_p43,c16_p87_n57, EO6B)
					COMPUTE_ROW(T_00_08B,T_00_09B,T_00_10B,T_00_11B, c16_n25_p09,c16_n57_p43,c16_n80_p70,c16_n90_p87, EO7B)
#undef COMPUTE_ROW
			}

			const __m128i EEO0A = _mm_add_epi32(_mm_madd_epi16(T_00_12A, c16_p75_p89 ), _mm_madd_epi16(T_00_13A, c16_p18_p50 )); // EEO0
			const __m128i EEO0B = _mm_add_epi32(_mm_madd_epi16(T_00_12B, c16_p75_p89 ), _mm_madd_epi16(T_00_13B, c16_p18_p50 ));
			const __m128i EEO1A = _mm_add_epi32(_mm_madd_epi16(T_00_12A, c16_n18_p75 ), _mm_madd_epi16(T_00_13A, c16_n50_n89 )); // EEO1
			const __m128i EEO1B = _mm_add_epi32(_mm_madd_epi16(T_00_12B, c16_n18_p75 ), _mm_madd_epi16(T_00_13B, c16_n50_n89 ));
			const __m128i EEO2A = _mm_add_epi32(_mm_madd_epi16(T_00_12A, c16_n89_p50 ), _mm_madd_epi16(T_00_13A, c16_p75_p18 )); // EEO2
			const __m128i EEO2B = _mm_add_epi32(_mm_madd_epi16(T_00_12B, c16_n89_p50 ), _mm_madd_epi16(T_00_13B, c16_p75_p18 ));
			const __m128i EEO3A = _mm_add_epi32(_mm_madd_epi16(T_00_12A, c16_n50_p18 ), _mm_madd_epi16(T_00_13A, c16_n89_p75 )); // EEO3
			const __m128i EEO3B = _mm_add_epi32(_mm_madd_epi16(T_00_12B, c16_n50_p18 ), _mm_madd_epi16(T_00_13B, c16_n89_p75 ));

			const __m128i EEEO0A = _mm_madd_epi16(T_00_14A, c16_p36_p83 );
			const __m128i EEEO0B = _mm_madd_epi16(T_00_14B, c16_p36_p83 );
			const __m128i EEEO1A = _mm_madd_epi16(T_00_14A, c16_n83_p36 );
			const __m128i EEEO1B = _mm_madd_epi16(T_00_14B, c16_n83_p36 );

			const __m128i EEEE0A = _mm_madd_epi16(T_00_15A, c16_p64_p64 );
			const __m128i EEEE0B = _mm_madd_epi16(T_00_15B, c16_p64_p64 );
			const __m128i EEEE1A = _mm_madd_epi16(T_00_15A, c16_n64_p64 );
			const __m128i EEEE1B = _mm_madd_epi16(T_00_15B, c16_n64_p64 );

			const __m128i EEE0A = _mm_add_epi32(EEEE0A, EEEO0A);                // EEE0 = EEEE0 + EEEO0
			const __m128i EEE0B = _mm_add_epi32(EEEE0B, EEEO0B);
			const __m128i EEE1A = _mm_add_epi32(EEEE1A, EEEO1A);                // EEE1 = EEEE1 + EEEO1
			const __m128i EEE1B = _mm_add_epi32(EEEE1B, EEEO1B);
			const __m128i EEE3A = _mm_sub_epi32(EEEE0A, EEEO0A);                // EEE2 = EEEE0 - EEEO0
			const __m128i EEE3B = _mm_sub_epi32(EEEE0B, EEEO0B);
			const __m128i EEE2A = _mm_sub_epi32(EEEE1A, EEEO1A);                // EEE3 = EEEE1 - EEEO1
			const __m128i EEE2B = _mm_sub_epi32(EEEE1B, EEEO1B);

			const __m128i EE0A = _mm_add_epi32(EEE0A, EEO0A);                // EE0 = EEE0 + EEO0
			const __m128i EE0B = _mm_add_epi32(EEE0B, EEO0B);
			const __m128i EE1A = _mm_add_epi32(EEE1A, EEO1A);                // EE1 = EEE1 + EEO1
			const __m128i EE1B = _mm_add_epi32(EEE1B, EEO1B);
			const __m128i EE2A = _mm_add_epi32(EEE2A, EEO2A);                // EE2 = EEE0 + EEO0
			const __m128i EE2B = _mm_add_epi32(EEE2B, EEO2B);
			const __m128i EE3A = _mm_add_epi32(EEE3A, EEO3A);                // EE3 = EEE1 + EEO1
			const __m128i EE3B = _mm_add_epi32(EEE3B, EEO3B);
			const __m128i EE7A = _mm_sub_epi32(EEE0A, EEO0A);                // EE7 = EEE0 - EEO0
			const __m128i EE7B = _mm_sub_epi32(EEE0B, EEO0B);
			const __m128i EE6A = _mm_sub_epi32(EEE1A, EEO1A);                // EE6 = EEE1 - EEO1
			const __m128i EE6B = _mm_sub_epi32(EEE1B, EEO1B);
			const __m128i EE5A = _mm_sub_epi32(EEE2A, EEO2A);                // EE5 = EEE0 - EEO0
			const __m128i EE5B = _mm_sub_epi32(EEE2B, EEO2B);
			const __m128i EE4A = _mm_sub_epi32(EEE3A, EEO3A);                // EE4 = EEE1 - EEO1
			const __m128i EE4B = _mm_sub_epi32(EEE3B, EEO3B);

			const __m128i E0A = _mm_add_epi32(EE0A, EO0A);                // E0 = EE0 + EO0
			const __m128i E0B = _mm_add_epi32(EE0B, EO0B);
			const __m128i E1A = _mm_add_epi32(EE1A, EO1A);                // E1 = EE1 + EO1
			const __m128i E1B = _mm_add_epi32(EE1B, EO1B);
			const __m128i E2A = _mm_add_epi32(EE2A, EO2A);                // E2 = EE2 + EO2
			const __m128i E2B = _mm_add_epi32(EE2B, EO2B);
			const __m128i E3A = _mm_add_epi32(EE3A, EO3A);                // E3 = EE3 + EO3
			const __m128i E3B = _mm_add_epi32(EE3B, EO3B);
			const __m128i E4A = _mm_add_epi32(EE4A, EO4A);                // E4 = 
			const __m128i E4B = _mm_add_epi32(EE4B, EO4B);
			const __m128i E5A = _mm_add_epi32(EE5A, EO5A);                // E5 = 
			const __m128i E5B = _mm_add_epi32(EE5B, EO5B);
			const __m128i E6A = _mm_add_epi32(EE6A, EO6A);                // E6 = 
			const __m128i E6B = _mm_add_epi32(EE6B, EO6B);
			const __m128i E7A = _mm_add_epi32(EE7A, EO7A);                // E7 = 
			const __m128i E7B = _mm_add_epi32(EE7B, EO7B);
			const __m128i EFA = _mm_sub_epi32(EE0A, EO0A);                // EF = EE0 - EO0
			const __m128i EFB = _mm_sub_epi32(EE0B, EO0B);
			const __m128i EEA = _mm_sub_epi32(EE1A, EO1A);                // EE = EE1 - EO1
			const __m128i EEB = _mm_sub_epi32(EE1B, EO1B);
			const __m128i EDA = _mm_sub_epi32(EE2A, EO2A);                // ED = EE2 - EO2
			const __m128i EDB = _mm_sub_epi32(EE2B, EO2B);
			const __m128i ECA = _mm_sub_epi32(EE3A, EO3A);                // EC = EE3 - EO3
			const __m128i ECB = _mm_sub_epi32(EE3B, EO3B);
			const __m128i EBA = _mm_sub_epi32(EE4A, EO4A);                // EB =
			const __m128i EBB = _mm_sub_epi32(EE4B, EO4B);
			const __m128i EAA = _mm_sub_epi32(EE5A, EO5A);                // EA = 
			const __m128i EAB = _mm_sub_epi32(EE5B, EO5B);
			const __m128i E9A = _mm_sub_epi32(EE6A, EO6A);                // E9 =
			const __m128i E9B = _mm_sub_epi32(EE6B, EO6B);
			const __m128i E8A = _mm_sub_epi32(EE7A, EO7A);                // E8 = 
			const __m128i E8B = _mm_sub_epi32(EE7B, EO7B);

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
			const __m128i T18A = _mm_add_epi32(E8A, c32_rnd);               // E8 + rnd
			const __m128i T18B = _mm_add_epi32(E8B, c32_rnd);
			const __m128i T19A = _mm_add_epi32(E9A, c32_rnd);               // E9 + rnd
			const __m128i T19B = _mm_add_epi32(E9B, c32_rnd);
			const __m128i T1AA = _mm_add_epi32(EAA, c32_rnd);               // E10 + rnd
			const __m128i T1AB = _mm_add_epi32(EAB, c32_rnd);
			const __m128i T1BA = _mm_add_epi32(EBA, c32_rnd);               // E11 + rnd
			const __m128i T1BB = _mm_add_epi32(EBB, c32_rnd);
			const __m128i T1CA = _mm_add_epi32(ECA, c32_rnd);               // E12 + rnd
			const __m128i T1CB = _mm_add_epi32(ECB, c32_rnd);
			const __m128i T1DA = _mm_add_epi32(EDA, c32_rnd);               // E13 + rnd
			const __m128i T1DB = _mm_add_epi32(EDB, c32_rnd);
			const __m128i T1EA = _mm_add_epi32(EEA, c32_rnd);               // E14 + rnd
			const __m128i T1EB = _mm_add_epi32(EEB, c32_rnd);
			const __m128i T1FA = _mm_add_epi32(EFA, c32_rnd);               // E15 + rnd
			const __m128i T1FB = _mm_add_epi32(EFB, c32_rnd);
	
			const __m128i T2_00A = _mm_add_epi32(T10A, O00A);                // E0 + O0 + rnd
			const __m128i T2_00B = _mm_add_epi32(T10B, O00B);
			const __m128i T2_01A = _mm_add_epi32(T11A, O01A);                // E1 + O1 + rnd
			const __m128i T2_01B = _mm_add_epi32(T11B, O01B);
			const __m128i T2_02A = _mm_add_epi32(T12A, O02A);                // E2 + O2 + rnd
			const __m128i T2_02B = _mm_add_epi32(T12B, O02B);
			const __m128i T2_03A = _mm_add_epi32(T13A, O03A);                // E3 + O3 + rnd
			const __m128i T2_03B = _mm_add_epi32(T13B, O03B);
			const __m128i T2_04A = _mm_add_epi32(T14A, O04A);                // E4 
			const __m128i T2_04B = _mm_add_epi32(T14B, O04B);
			const __m128i T2_05A = _mm_add_epi32(T15A, O05A);                // E5 
			const __m128i T2_05B = _mm_add_epi32(T15B, O05B);
			const __m128i T2_06A = _mm_add_epi32(T16A, O06A);                // E6
			const __m128i T2_06B = _mm_add_epi32(T16B, O06B);
			const __m128i T2_07A = _mm_add_epi32(T17A, O07A);                // E7 
			const __m128i T2_07B = _mm_add_epi32(T17B, O07B);
			const __m128i T2_08A = _mm_add_epi32(T18A, O08A);                // E8
			const __m128i T2_08B = _mm_add_epi32(T18B, O08B);
			const __m128i T2_09A = _mm_add_epi32(T19A, O09A);                // E9
			const __m128i T2_09B = _mm_add_epi32(T19B, O09B);
			const __m128i T2_10A = _mm_add_epi32(T1AA, O10A);                // E10
			const __m128i T2_10B = _mm_add_epi32(T1AB, O10B);
			const __m128i T2_11A = _mm_add_epi32(T1BA, O11A);                // E11
			const __m128i T2_11B = _mm_add_epi32(T1BB, O11B);
			const __m128i T2_12A = _mm_add_epi32(T1CA, O12A);                // E12 
			const __m128i T2_12B = _mm_add_epi32(T1CB, O12B);
			const __m128i T2_13A = _mm_add_epi32(T1DA, O13A);                // E13 
			const __m128i T2_13B = _mm_add_epi32(T1DB, O13B);
			const __m128i T2_14A = _mm_add_epi32(T1EA, O14A);                // E14
			const __m128i T2_14B = _mm_add_epi32(T1EB, O14B);
			const __m128i T2_15A = _mm_add_epi32(T1FA, O15A);                // E15 
			const __m128i T2_15B = _mm_add_epi32(T1FB, O15B);
			const __m128i T2_31A = _mm_sub_epi32(T10A, O00A);                // E0 - O0 + rnd
			const __m128i T2_31B = _mm_sub_epi32(T10B, O00B);
			const __m128i T2_30A = _mm_sub_epi32(T11A, O01A);                // E1 - O1 + rnd
			const __m128i T2_30B = _mm_sub_epi32(T11B, O01B);
			const __m128i T2_29A = _mm_sub_epi32(T12A, O02A);                // E2 - O2 + rnd
			const __m128i T2_29B = _mm_sub_epi32(T12B, O02B);
			const __m128i T2_28A = _mm_sub_epi32(T13A, O03A);                // E3 - O3 + rnd
			const __m128i T2_28B = _mm_sub_epi32(T13B, O03B);
			const __m128i T2_27A = _mm_sub_epi32(T14A, O04A);                // E4
			const __m128i T2_27B = _mm_sub_epi32(T14B, O04B);
			const __m128i T2_26A = _mm_sub_epi32(T15A, O05A);                // E5
			const __m128i T2_26B = _mm_sub_epi32(T15B, O05B);
			const __m128i T2_25A = _mm_sub_epi32(T16A, O06A);                // E6
			const __m128i T2_25B = _mm_sub_epi32(T16B, O06B);
			const __m128i T2_24A = _mm_sub_epi32(T17A, O07A);                // E7
			const __m128i T2_24B = _mm_sub_epi32(T17B, O07B);
			const __m128i T2_23A = _mm_sub_epi32(T18A, O08A);                // 
			const __m128i T2_23B = _mm_sub_epi32(T18B, O08B);
			const __m128i T2_22A = _mm_sub_epi32(T19A, O09A);                // 
			const __m128i T2_22B = _mm_sub_epi32(T19B, O09B);
			const __m128i T2_21A = _mm_sub_epi32(T1AA, O10A);                // 
			const __m128i T2_21B = _mm_sub_epi32(T1AB, O10B);
			const __m128i T2_20A = _mm_sub_epi32(T1BA, O11A);                // 
			const __m128i T2_20B = _mm_sub_epi32(T1BB, O11B);
			const __m128i T2_19A = _mm_sub_epi32(T1CA, O12A);                // 
			const __m128i T2_19B = _mm_sub_epi32(T1CB, O12B);
			const __m128i T2_18A = _mm_sub_epi32(T1DA, O13A);                //
			const __m128i T2_18B = _mm_sub_epi32(T1DB, O13B);
			const __m128i T2_17A = _mm_sub_epi32(T1EA, O14A);                // 
			const __m128i T2_17B = _mm_sub_epi32(T1EB, O14B);
			const __m128i T2_16A = _mm_sub_epi32(T1FA, O15A);                // 
			const __m128i T2_16B = _mm_sub_epi32(T1FB, O15B);
		
			const __m128i T3_00A = _mm_srai_epi32(T2_00A, nShift);                   // [30 20 10 00]
			const __m128i T3_00B = _mm_srai_epi32(T2_00B, nShift);                   // [70 60 50 40]
			const __m128i T3_01A = _mm_srai_epi32(T2_01A, nShift);                   // [31 21 11 01]
			const __m128i T3_01B = _mm_srai_epi32(T2_01B, nShift);                   // [71 61 51 41]
			const __m128i T3_02A = _mm_srai_epi32(T2_02A, nShift);                   // [32 22 12 02]
			const __m128i T3_02B = _mm_srai_epi32(T2_02B, nShift);                   // [72 62 52 42]
			const __m128i T3_03A = _mm_srai_epi32(T2_03A, nShift);                   // [33 23 13 03]
			const __m128i T3_03B = _mm_srai_epi32(T2_03B, nShift);                   // [73 63 53 43]
			const __m128i T3_04A = _mm_srai_epi32(T2_04A, nShift);                   // [33 24 14 04]
			const __m128i T3_04B = _mm_srai_epi32(T2_04B, nShift);                   // [74 64 54 44]
			const __m128i T3_05A = _mm_srai_epi32(T2_05A, nShift);                   // [35 25 15 05]
			const __m128i T3_05B = _mm_srai_epi32(T2_05B, nShift);                   // [75 65 55 45]
			const __m128i T3_06A = _mm_srai_epi32(T2_06A, nShift);                   // [36 26 16 06]
			const __m128i T3_06B = _mm_srai_epi32(T2_06B, nShift);                   // [76 66 56 46]
			const __m128i T3_07A = _mm_srai_epi32(T2_07A, nShift);                   // [37 27 17 07]
			const __m128i T3_07B = _mm_srai_epi32(T2_07B, nShift);                   // [77 67 57 47]
			const __m128i T3_08A = _mm_srai_epi32(T2_08A, nShift);                   // [30 20 10 00] x8
			const __m128i T3_08B = _mm_srai_epi32(T2_08B, nShift);                   // [70 60 50 40]
			const __m128i T3_09A = _mm_srai_epi32(T2_09A, nShift);                   // [31 21 11 01] x9 
			const __m128i T3_09B = _mm_srai_epi32(T2_09B, nShift);                   // [71 61 51 41]
			const __m128i T3_10A = _mm_srai_epi32(T2_10A, nShift);                   // [32 22 12 02] xA
			const __m128i T3_10B = _mm_srai_epi32(T2_10B, nShift);                   // [72 62 52 42]
			const __m128i T3_11A = _mm_srai_epi32(T2_11A, nShift);                   // [33 23 13 03] xB
			const __m128i T3_11B = _mm_srai_epi32(T2_11B, nShift);                   // [73 63 53 43]
			const __m128i T3_12A = _mm_srai_epi32(T2_12A, nShift);                   // [33 24 14 04] xC
			const __m128i T3_12B = _mm_srai_epi32(T2_12B, nShift);                   // [74 64 54 44]
			const __m128i T3_13A = _mm_srai_epi32(T2_13A, nShift);                   // [35 25 15 05] xD
			const __m128i T3_13B = _mm_srai_epi32(T2_13B, nShift);                   // [75 65 55 45]
			const __m128i T3_14A = _mm_srai_epi32(T2_14A, nShift);                   // [36 26 16 06] xE
			const __m128i T3_14B = _mm_srai_epi32(T2_14B, nShift);                   // [76 66 56 46]
			const __m128i T3_15A = _mm_srai_epi32(T2_15A, nShift);                   // [37 27 17 07] xF
			const __m128i T3_15B = _mm_srai_epi32(T2_15B, nShift);                   // [77 67 57 47]

			const __m128i T3_16A = _mm_srai_epi32(T2_16A, nShift);                   // [30 20 10 00]
			const __m128i T3_16B = _mm_srai_epi32(T2_16B, nShift);                   // [70 60 50 40]
			const __m128i T3_17A = _mm_srai_epi32(T2_17A, nShift);                   // [31 21 11 01]
			const __m128i T3_17B = _mm_srai_epi32(T2_17B, nShift);                   // [71 61 51 41]
			const __m128i T3_18A = _mm_srai_epi32(T2_18A, nShift);                   // [32 22 12 02]
			const __m128i T3_18B = _mm_srai_epi32(T2_18B, nShift);                   // [72 62 52 42]
			const __m128i T3_19A = _mm_srai_epi32(T2_19A, nShift);                   // [33 23 13 03]
			const __m128i T3_19B = _mm_srai_epi32(T2_19B, nShift);                   // [73 63 53 43]
			const __m128i T3_20A = _mm_srai_epi32(T2_20A, nShift);                   // [33 24 14 04]
			const __m128i T3_20B = _mm_srai_epi32(T2_20B, nShift);                   // [74 64 54 44]
			const __m128i T3_21A = _mm_srai_epi32(T2_21A, nShift);                   // [35 25 15 05]
			const __m128i T3_21B = _mm_srai_epi32(T2_21B, nShift);                   // [75 65 55 45]
			const __m128i T3_22A = _mm_srai_epi32(T2_22A, nShift);                   // [36 26 16 06]
			const __m128i T3_22B = _mm_srai_epi32(T2_22B, nShift);                   // [76 66 56 46]
			const __m128i T3_23A = _mm_srai_epi32(T2_23A, nShift);                   // [37 27 17 07]
			const __m128i T3_23B = _mm_srai_epi32(T2_23B, nShift);                   // [77 67 57 47]
			const __m128i T3_24A = _mm_srai_epi32(T2_24A, nShift);                   // [30 20 10 00] x8
			const __m128i T3_24B = _mm_srai_epi32(T2_24B, nShift);                   // [70 60 50 40]
			const __m128i T3_25A = _mm_srai_epi32(T2_25A, nShift);                   // [31 21 11 01] x9 
			const __m128i T3_25B = _mm_srai_epi32(T2_25B, nShift);                   // [71 61 51 41]
			const __m128i T3_26A = _mm_srai_epi32(T2_26A, nShift);                   // [32 22 12 02] xA
			const __m128i T3_26B = _mm_srai_epi32(T2_26B, nShift);                   // [72 62 52 42]
			const __m128i T3_27A = _mm_srai_epi32(T2_27A, nShift);                   // [33 23 13 03] xB
			const __m128i T3_27B = _mm_srai_epi32(T2_27B, nShift);                   // [73 63 53 43]
			const __m128i T3_28A = _mm_srai_epi32(T2_28A, nShift);                   // [33 24 14 04] xC
			const __m128i T3_28B = _mm_srai_epi32(T2_28B, nShift);                   // [74 64 54 44]
			const __m128i T3_29A = _mm_srai_epi32(T2_29A, nShift);                   // [35 25 15 05] xD
			const __m128i T3_29B = _mm_srai_epi32(T2_29B, nShift);                   // [75 65 55 45]
			const __m128i T3_30A = _mm_srai_epi32(T2_30A, nShift);                   // [36 26 16 06] xE
			const __m128i T3_30B = _mm_srai_epi32(T2_30B, nShift);                   // [76 66 56 46]
			const __m128i T3_31A = _mm_srai_epi32(T2_31A, nShift);                   // [37 27 17 07] xF
			const __m128i T3_31B = _mm_srai_epi32(T2_31B, nShift);                   // [77 67 57 47]

			res00[part]  = _mm_packs_epi32(T3_00A, T3_00B);              // [70 60 50 40 30 20 10 00]
			res01[part]  = _mm_packs_epi32(T3_01A, T3_01B);              // [71 61 51 41 31 21 11 01]
			res02[part]  = _mm_packs_epi32(T3_02A, T3_02B);              // [72 62 52 42 32 22 12 02]
			res03[part]  = _mm_packs_epi32(T3_03A, T3_03B);              // [73 63 53 43 33 23 13 03]
			res04[part]  = _mm_packs_epi32(T3_04A, T3_04B);              // [74 64 54 44 34 24 14 04]
			res05[part]  = _mm_packs_epi32(T3_05A, T3_05B);              // [75 65 55 45 35 25 15 05]
			res06[part]  = _mm_packs_epi32(T3_06A, T3_06B);              // [76 66 56 46 36 26 16 06]
			res07[part]  = _mm_packs_epi32(T3_07A, T3_07B);              // [77 67 57 47 37 27 17 07]
			res08[part]  = _mm_packs_epi32(T3_08A, T3_08B);              // [A0 ... 80]
			res09[part]  = _mm_packs_epi32(T3_09A, T3_09B);              // [A1 ... 81]
			res10[part]  = _mm_packs_epi32(T3_10A, T3_10B);              // [A2 ... 82]
			res11[part]  = _mm_packs_epi32(T3_11A, T3_11B);              // [A3 ... 83]
			res12[part]  = _mm_packs_epi32(T3_12A, T3_12B);              // [A4 ... 84]
			res13[part]  = _mm_packs_epi32(T3_13A, T3_13B);              // [A5 ... 85]
			res14[part]  = _mm_packs_epi32(T3_14A, T3_14B);              // [A6 ... 86]
			res15[part]  = _mm_packs_epi32(T3_15A, T3_15B);              // [A7 ... 87] 
			res16[part]  = _mm_packs_epi32(T3_16A, T3_16B);              
			res17[part]  = _mm_packs_epi32(T3_17A, T3_17B);             
			res18[part]  = _mm_packs_epi32(T3_18A, T3_18B);            
			res19[part]  = _mm_packs_epi32(T3_19A, T3_19B);             
			res20[part]  = _mm_packs_epi32(T3_20A, T3_20B);             
			res21[part]  = _mm_packs_epi32(T3_21A, T3_21B);            
			res22[part]  = _mm_packs_epi32(T3_22A, T3_22B);          
			res23[part]  = _mm_packs_epi32(T3_23A, T3_23B);            
			res24[part]  = _mm_packs_epi32(T3_24A, T3_24B);            
			res25[part]  = _mm_packs_epi32(T3_25A, T3_25B);            
			res26[part]  = _mm_packs_epi32(T3_26A, T3_26B);            
			res27[part]  = _mm_packs_epi32(T3_27A, T3_27B);            
			res28[part]  = _mm_packs_epi32(T3_28A, T3_28B);          
			res29[part]  = _mm_packs_epi32(T3_29A, T3_29B);        
			res30[part]  = _mm_packs_epi32(T3_30A, T3_30B);              
			res31[part]  = _mm_packs_epi32(T3_31A, T3_31B);           
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
			TRANSPOSE_8x8_16BIT(res00[1],res01[1],res02[1],res03[1],res04[1],res05[1],res06[1],res07[1],in08[0],in09[0],in10[0],in11[0],in12[0],in13[0],in14[0],in15[0])
			TRANSPOSE_8x8_16BIT(res00[2],res01[2],res02[2],res03[2],res04[2],res05[2],res06[2],res07[2],in16[0],in17[0],in18[0],in19[0],in20[0],in21[0],in22[0],in23[0])
			TRANSPOSE_8x8_16BIT(res00[3],res01[3],res02[3],res03[3],res04[3],res05[3],res06[3],res07[3],in24[0],in25[0],in26[0],in27[0],in28[0],in29[0],in30[0],in31[0])

			TRANSPOSE_8x8_16BIT(res08[0],res09[0],res10[0],res11[0],res12[0],res13[0],res14[0],res15[0],in00[1],in01[1],in02[1],in03[1],in04[1],in05[1],in06[1],in07[1])
			TRANSPOSE_8x8_16BIT(res08[1],res09[1],res10[1],res11[1],res12[1],res13[1],res14[1],res15[1],in08[1],in09[1],in10[1],in11[1],in12[1],in13[1],in14[1],in15[1])
			TRANSPOSE_8x8_16BIT(res08[2],res09[2],res10[2],res11[2],res12[2],res13[2],res14[2],res15[2],in16[1],in17[1],in18[1],in19[1],in20[1],in21[1],in22[1],in23[1])
			TRANSPOSE_8x8_16BIT(res08[3],res09[3],res10[3],res11[3],res12[3],res13[3],res14[3],res15[3],in24[1],in25[1],in26[1],in27[1],in28[1],in29[1],in30[1],in31[1])

			TRANSPOSE_8x8_16BIT(res16[0],res17[0],res18[0],res19[0],res20[0],res21[0],res22[0],res23[0],in00[2],in01[2],in02[2],in03[2],in04[2],in05[2],in06[2],in07[2])
			TRANSPOSE_8x8_16BIT(res16[1],res17[1],res18[1],res19[1],res20[1],res21[1],res22[1],res23[1],in08[2],in09[2],in10[2],in11[2],in12[2],in13[2],in14[2],in15[2])
			TRANSPOSE_8x8_16BIT(res16[2],res17[2],res18[2],res19[2],res20[2],res21[2],res22[2],res23[2],in16[2],in17[2],in18[2],in19[2],in20[2],in21[2],in22[2],in23[2])
			TRANSPOSE_8x8_16BIT(res16[3],res17[3],res18[3],res19[3],res20[3],res21[3],res22[3],res23[3],in24[2],in25[2],in26[2],in27[2],in28[2],in29[2],in30[2],in31[2])

			TRANSPOSE_8x8_16BIT(res24[0],res25[0],res26[0],res27[0],res28[0],res29[0],res30[0],res31[0],in00[3],in01[3],in02[3],in03[3],in04[3],in05[3],in06[3],in07[3])
			TRANSPOSE_8x8_16BIT(res24[1],res25[1],res26[1],res27[1],res28[1],res29[1],res30[1],res31[1],in08[3],in09[3],in10[3],in11[3],in12[3],in13[3],in14[3],in15[3])
			TRANSPOSE_8x8_16BIT(res24[2],res25[2],res26[2],res27[2],res28[2],res29[2],res30[2],res31[2],in16[3],in17[3],in18[3],in19[3],in20[3],in21[3],in22[3],in23[3])
			TRANSPOSE_8x8_16BIT(res24[3],res25[3],res26[3],res27[3],res28[3],res29[3],res30[3],res31[3],in24[3],in25[3],in26[3],in27[3],in28[3],in29[3],in30[3],in31[3])

#undef TRANSPOSE_8x8_16BIT
		}
	}

	// Add
    for (Int i=0; i<2; i++) {
#define STROE_LINE(L0,L1,L2,L3,L4,L5,L6,L7, H0,H1,H2,H3,H4,H5,H6,H7, offsetV, offsetH)\
        _mm_storeu_si128( (__m128i *)&pDst[(0+(offsetV))*stride+(offsetH)+0], L0);\
        _mm_storeu_si128( (__m128i *)&pDst[(0+(offsetV))*stride+(offsetH)+8], H0);\
        _mm_storeu_si128( (__m128i *)&pDst[(1+(offsetV))*stride+(offsetH)+0], L1);\
        _mm_storeu_si128( (__m128i *)&pDst[(1+(offsetV))*stride+(offsetH)+8], H1);\
        _mm_storeu_si128( (__m128i *)&pDst[(2+(offsetV))*stride+(offsetH)+0], L2);\
        _mm_storeu_si128( (__m128i *)&pDst[(2+(offsetV))*stride+(offsetH)+8], H2);\
        _mm_storeu_si128( (__m128i *)&pDst[(3+(offsetV))*stride+(offsetH)+0], L3);\
        _mm_storeu_si128( (__m128i *)&pDst[(3+(offsetV))*stride+(offsetH)+8], H3);\
        _mm_storeu_si128( (__m128i *)&pDst[(4+(offsetV))*stride+(offsetH)+0], L4);\
        _mm_storeu_si128( (__m128i *)&pDst[(4+(offsetV))*stride+(offsetH)+8], H4);\
        _mm_storeu_si128( (__m128i *)&pDst[(5+(offsetV))*stride+(offsetH)+0], L5);\
        _mm_storeu_si128( (__m128i *)&pDst[(5+(offsetV))*stride+(offsetH)+8], H5);\
        _mm_storeu_si128( (__m128i *)&pDst[(6+(offsetV))*stride+(offsetH)+0], L6);\
        _mm_storeu_si128( (__m128i *)&pDst[(6+(offsetV))*stride+(offsetH)+8], H6);\
        _mm_storeu_si128( (__m128i *)&pDst[(7+(offsetV))*stride+(offsetH)+0], L7);\
        _mm_storeu_si128( (__m128i *)&pDst[(7+(offsetV))*stride+(offsetH)+8], H7);

        const int k = i*2;
        STROE_LINE(in00[k],in01[k],in02[k],in03[k],in04[k],in05[k],in06[k],in07[k], in00[k+1],in01[k+1],in02[k+1],in03[k+1],in04[k+1],in05[k+1],in06[k+1],in07[k+1], 0 ,i*16)
        STROE_LINE(in08[k],in09[k],in10[k],in11[k],in12[k],in13[k],in14[k],in15[k], in08[k+1],in09[k+1],in10[k+1],in11[k+1],in12[k+1],in13[k+1],in14[k+1],in15[k+1], 8 ,i*16)
        STROE_LINE(in16[k],in17[k],in18[k],in19[k],in20[k],in21[k],in22[k],in23[k], in16[k+1],in17[k+1],in18[k+1],in19[k+1],in20[k+1],in21[k+1],in22[k+1],in23[k+1], 16,i*16)
        STROE_LINE(in24[k],in25[k],in26[k],in27[k],in28[k],in29[k],in30[k],in31[k], in24[k+1],in25[k+1],in26[k+1],in27[k+1],in28[k+1],in29[k+1],in30[k+1],in31[k+1], 24,i*16)
#undef STROE_LINE
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
