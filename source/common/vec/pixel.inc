/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Mandar Gurav <mandar@multicorewareinc.com>
 *          Mahesh Pittala <mahesh@multicorewareinc.com>
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

// Vector class versions of pixel comparison performance primitives

#define SET_FUNC_PRIMITIVE_TABLE(FUNC_PREFIX, FUNC_PREFIX_DEF, FUNC_TYPE_CAST) \
    p. FUNC_PREFIX [PARTITION_4x4] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 4 < 4 >;   \
    p. FUNC_PREFIX [PARTITION_4x8] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 4 < 8 >;   \
    p. FUNC_PREFIX [PARTITION_4x12] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 4 < 12 >;   \
    p. FUNC_PREFIX [PARTITION_4x16] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 4 < 16 >;   \
    p. FUNC_PREFIX [PARTITION_4x24] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 4 < 24 >;   \
    p. FUNC_PREFIX [PARTITION_4x32] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 4 < 32 >;   \
    p. FUNC_PREFIX [PARTITION_4x48] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 4 < 48 >;   \
    p. FUNC_PREFIX [PARTITION_4x64] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 4 < 64 >;   \
                                                                   \
    p. FUNC_PREFIX [PARTITION_8x4] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 8 < 4 >;   \
    p. FUNC_PREFIX [PARTITION_8x8] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 8 < 8 >;   \
    p. FUNC_PREFIX [PARTITION_8x12] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 8 < 12 >;   \
    p. FUNC_PREFIX [PARTITION_8x16] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 8 < 16 >;   \
    p. FUNC_PREFIX [PARTITION_8x24] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 8 < 24 >;   \
    p. FUNC_PREFIX [PARTITION_8x32] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 8 < 32 >;   \
    p. FUNC_PREFIX [PARTITION_8x48] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 8 < 48 >;   \
    p. FUNC_PREFIX [PARTITION_8x64] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 8 < 64 >;   \
                                                                   \
    p. FUNC_PREFIX [PARTITION_12x4] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 12 < 4 >;   \
    p. FUNC_PREFIX [PARTITION_12x8] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 12 < 8 >;   \
    p. FUNC_PREFIX [PARTITION_12x12] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 12 < 12 >;   \
    p. FUNC_PREFIX [PARTITION_12x16] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 12 < 16 >;   \
    p. FUNC_PREFIX [PARTITION_12x24] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 12 < 24 >;   \
    p. FUNC_PREFIX [PARTITION_12x32] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 12 < 32 >;   \
    p. FUNC_PREFIX [PARTITION_12x48] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 12 < 48 >;   \
    p. FUNC_PREFIX [PARTITION_12x64] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 12 < 64 >;   \
                                                                   \
    p. FUNC_PREFIX [PARTITION_16x4] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 16 < 4 >;   \
    p. FUNC_PREFIX [PARTITION_16x8] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 16 < 8 >;   \
    p. FUNC_PREFIX [PARTITION_16x12] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 16 < 12 >;   \
    p. FUNC_PREFIX [PARTITION_16x16] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 16 < 16 >;   \
    p. FUNC_PREFIX [PARTITION_16x24] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 16 < 24 >;   \
    p. FUNC_PREFIX [PARTITION_16x32] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 16 < 32 >;   \
    p. FUNC_PREFIX [PARTITION_16x48] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 16 < 48 >;   \
    p. FUNC_PREFIX [PARTITION_16x64] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 16 < 64 >;   \
                                                                   \
    p. FUNC_PREFIX [PARTITION_24x4] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 24 < 4 >;   \
    p. FUNC_PREFIX [PARTITION_24x8] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 24 < 8 >;   \
    p. FUNC_PREFIX [PARTITION_24x12] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 24 < 12 >;   \
    p. FUNC_PREFIX [PARTITION_24x16] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 24 < 16 >;   \
    p. FUNC_PREFIX [PARTITION_24x24] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 24 < 24 >;   \
    p. FUNC_PREFIX [PARTITION_24x32] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 24 < 32 >;   \
    p. FUNC_PREFIX [PARTITION_24x48] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 24 < 48 >;   \
    p. FUNC_PREFIX [PARTITION_24x64] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 24 < 64 >;   \
                                                                   \
    p. FUNC_PREFIX [PARTITION_32x4] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 32 < 4 >;   \
    p. FUNC_PREFIX [PARTITION_32x8] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 32 < 8 >;   \
    p. FUNC_PREFIX [PARTITION_32x12] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 32 < 12 >;   \
    p. FUNC_PREFIX [PARTITION_32x16] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 32 < 16 >;   \
    p. FUNC_PREFIX [PARTITION_32x24] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 32 < 24 >;   \
    p. FUNC_PREFIX [PARTITION_32x32] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 32 < 32 >;   \
    p. FUNC_PREFIX [PARTITION_32x48] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 32 < 48 >;   \
    p. FUNC_PREFIX [PARTITION_32x64] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 32 < 64 >;   \
                                                                   \
    p. FUNC_PREFIX [PARTITION_48x4] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 48 < 4 >;   \
    p. FUNC_PREFIX [PARTITION_48x8] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 48 < 8 >;   \
    p. FUNC_PREFIX [PARTITION_48x12] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 48 < 12 >;   \
    p. FUNC_PREFIX [PARTITION_48x16] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 48 < 16 >;   \
    p. FUNC_PREFIX [PARTITION_48x24] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 48 < 24 >;   \
    p. FUNC_PREFIX [PARTITION_48x32] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 48 < 32 >;   \
    p. FUNC_PREFIX [PARTITION_48x48] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 48 < 48 >;   \
    p. FUNC_PREFIX [PARTITION_48x64] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 48 < 64 >;   \
                                                                   \
    p. FUNC_PREFIX [PARTITION_64x4] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 64 < 4 >;   \
    p. FUNC_PREFIX [PARTITION_64x8] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 64 < 8 >;   \
    p. FUNC_PREFIX [PARTITION_64x12] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 64 < 12 >;   \
    p. FUNC_PREFIX [PARTITION_64x16] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 64 < 16 >;   \
    p. FUNC_PREFIX [PARTITION_64x24] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 64 < 24 >;   \
    p. FUNC_PREFIX [PARTITION_64x32] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 64 < 32 >;   \
    p. FUNC_PREFIX [PARTITION_64x48] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 64 < 48 >;   \
    p. FUNC_PREFIX [PARTITION_64x64] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## 64 < 64 >;   \


namespace {
/* File for pixels type-neutral code */

void CDECL convert16to32(short *psOrg, int *piDst, int num)
{
    int i;

    for (i = 0; i < num; i += 8)
    {
        Vec8s im16;
        Vec4i im32L, im32H;

        im16.load(psOrg);
        im32L = extend_low(im16);
        im32H = extend_high(im16);
        im32L.store(piDst);
        im32H.store(piDst + 4);

        psOrg += 8;
        piDst += 8;
    }
}

void CDECL convert16to32_shl(int *piDst, short *psOrg, intptr_t iStride, int shift, int size)
{
    int i, j;

    for (i=0; i <size; i++)
    {
        for (j=0; j<size; j+=4)
        {
            __m128i im16;
            __m128i im32;

            im16 = _mm_loadl_epi64((__m128i*)&psOrg[i*iStride+j]);
            im32 = _mm_srai_epi32(_mm_unpacklo_epi16(im16, im16), 16);
            im32 = _mm_slli_epi32(im32, shift);
            _mm_storeu_si128((__m128i*)piDst, im32);

            piDst += 4;
        }
    }
}

void CDECL convert16to16_shl(short *psDst, short *psOrg, int width, int height, intptr_t stride, int shift)
{
    int i, j;

    if (width == 4)
    {
        for (i = 0; i < height; i+=2)
        {
            __m128i T00, T01;

            T00 = _mm_loadl_epi64((__m128i*)&psOrg[(i  )*stride]);
            T01 = _mm_loadl_epi64((__m128i*)&psOrg[(i+1)*stride]);
            T00 = _mm_unpacklo_epi64(T00, T01);
            T00 = _mm_slli_epi16(T00, shift);
            _mm_storeu_si128((__m128i*)&psDst[i*4], T00);
        }
    }
    else
    {
        for (i = 0; i < height; i++)
        {
            for( j=0; j<width; j+=8 )
            {
                __m128i T00;

                T00 = _mm_loadu_si128((__m128i*)&psOrg[i*stride+j]);
                T00 = _mm_slli_epi16(T00, shift);
                _mm_storeu_si128((__m128i*)&psDst[i*width+j], T00);
            }
        }
    }
}

void CDECL convert32to16(int *psOrg, short *piDst, int num)
{
    int i;

    for (i = 0; i < num; i += 8)
    {
        Vec4i im32L, im32H;
        Vec8s im16;

        im32L.load(psOrg);
        im32H.load(psOrg + 4);
        im16 = compress_saturated(im32L, im32H);
        im16.store(piDst);

        psOrg += 8;
        piDst += 8;
    }
}

void CDECL convert32to16_shr(short *piDst, int *psOrg, int shift, int num)
{
    int i;
    Vec4i round = _mm_set1_epi32(1 << (shift - 1));

    for (i = 0; i < num; i += 4)
    {
        Vec4i im32;
        Vec8s im16;

        im32.load(psOrg);
        im32 = (im32 + round) >> shift;
        im16 = compress_saturated(im32, im32);
        store_partial(const_int(8), piDst, im16);

        psOrg += 4;
        piDst += 4;
    }
}

template <int blockSize>
void CDECL transpose(pixel* pDst, pixel* pSrc, intptr_t nStride)
{
    for (int k = 0; k < blockSize; k++)
    {
        for (int l = 0; l < blockSize; l++)
        {
            pDst[k * blockSize + l] = pSrc[l * nStride + k];
        }
    }
}

#if !HIGH_BIT_DEPTH
void CDECL transpose4(pixel* pDst, pixel* pSrc, intptr_t nStride)
{
    __m128i T00, T01, T02, T03;

    T00 = _mm_cvtsi32_si128(*(int*)&pSrc[0*nStride]);   // [03 02 01 00]
    T01 = _mm_cvtsi32_si128(*(int*)&pSrc[1*nStride]);   // [13 12 11 10]
    T02 = _mm_cvtsi32_si128(*(int*)&pSrc[2*nStride]);   // [23 22 21 20]
    T03 = _mm_cvtsi32_si128(*(int*)&pSrc[3*nStride]);   // [33 32 31 30]

    T00 = _mm_unpacklo_epi8(T00, T01);
    T01 = _mm_unpacklo_epi8(T02, T03);

    T00 = _mm_unpacklo_epi16(T00, T01);

    _mm_store_si128((__m128i*)pDst, T00);
}

#define TRANSPOSE_8X8(in0, in1, in2, in3, in4, in5, in6, in7, \
                      out0, out1, out2, out3) \
  {                                                     \
    const __m128i tr0_0 = _mm_unpacklo_epi8(in0, in1); \
    const __m128i tr0_1 = _mm_unpacklo_epi8(in2, in3); \
    const __m128i tr0_2 = _mm_unpacklo_epi8(in4, in5); \
    const __m128i tr0_3 = _mm_unpacklo_epi8(in6, in7); \
                                                        \
    const __m128i tr1_0 = _mm_unpacklo_epi16(tr0_0, tr0_1); \
    const __m128i tr1_1 = _mm_unpackhi_epi16(tr0_0, tr0_1); \
    const __m128i tr1_2 = _mm_unpacklo_epi16(tr0_2, tr0_3); \
    const __m128i tr1_3 = _mm_unpackhi_epi16(tr0_2, tr0_3); \
                                                            \
    out0 = _mm_unpacklo_epi32(tr1_0, tr1_2); \
    out1 = _mm_unpackhi_epi32(tr1_0, tr1_2); \
    out2 = _mm_unpacklo_epi32(tr1_1, tr1_3); \
    out3 = _mm_unpackhi_epi32(tr1_1, tr1_3); \
}

void CDECL transpose8(pixel* pDst, pixel* pSrc, intptr_t nStride)
{
    __m128i T00, T01, T02, T03, T04, T05, T06, T07;

    T00 = _mm_loadl_epi64((__m128i*)&pSrc[0*nStride]);   // [07 06 05 04 03 02 01 00]
    T01 = _mm_loadl_epi64((__m128i*)&pSrc[1*nStride]);   // [17 16 15 14 13 12 11 10]
    T02 = _mm_loadl_epi64((__m128i*)&pSrc[2*nStride]);   // [27 26 25 24 23 22 21 20]
    T03 = _mm_loadl_epi64((__m128i*)&pSrc[3*nStride]);   // [37 36 35 34 33 32 31 30]
    T04 = _mm_loadl_epi64((__m128i*)&pSrc[4*nStride]);   // [47 46 45 44 43 42 41 40]
    T05 = _mm_loadl_epi64((__m128i*)&pSrc[5*nStride]);   // [57 56 55 54 53 52 51 50]
    T06 = _mm_loadl_epi64((__m128i*)&pSrc[6*nStride]);   // [67 66 65 64 63 62 61 60]
    T07 = _mm_loadl_epi64((__m128i*)&pSrc[7*nStride]);   // [77 76 75 74 73 72 71 70]

    TRANSPOSE_8X8(T00, T01, T02, T03, T04, T05, T06, T07, T00, T01, T02, T03);

    _mm_store_si128((__m128i*)&pDst[0*8], T00);
    _mm_store_si128((__m128i*)&pDst[2*8], T01);
    _mm_store_si128((__m128i*)&pDst[4*8], T02);
    _mm_store_si128((__m128i*)&pDst[6*8], T03);
}

#endif !HIGH_BIT_DEPTH

}  // end anonymous namespace

#include "utils.h"
#include "sse.inc"

namespace x265 {
// private x265 namespace

void NAME(Setup_Vec_PixelPrimitives)(EncoderPrimitives &p)
{
    SET_FUNC_PRIMITIVE_TABLE(sad, sad_, pixelcmp)
    SET_FUNC_PRIMITIVE_TABLE(sad_x3, sad_x3_, pixelcmp_x3)
    SET_FUNC_PRIMITIVE_TABLE(sad_x4, sad_x4_, pixelcmp_x4)

    p.cvt16to32     = convert16to32;
    p.cvt16to32_shl = convert16to32_shl;
    p.cvt16to16_shl = convert16to16_shl;
    p.cvt32to16     = convert32to16;
    p.cvt32to16_shr = convert32to16_shr;

#if HIGH_BIT_DEPTH
    SET_FUNC_PRIMITIVE_TABLE(sse_pp, sse_ss, pixelcmp)
    SET_FUNC_PRIMITIVE_TABLE(sse_sp, sse_ss, pixelcmp_sp)
    SET_FUNC_PRIMITIVE_TABLE(sse_ss, sse_ss, pixelcmp_ss)
    p.satd[PARTITION_4x4] = satd_4x4;
    p.satd[PARTITION_4x8] = satd<4, 8>;
    p.satd[PARTITION_16x4] = satd<16, 4>;
#else
    SET_FUNC_PRIMITIVE_TABLE(sse_pp, sse_pp, pixelcmp)
    SET_FUNC_PRIMITIVE_TABLE(sse_sp, sse_sp, pixelcmp_sp)
    SET_FUNC_PRIMITIVE_TABLE(sse_ss, sse_ss, pixelcmp_ss)

    p.calcresidual[BLOCK_4x4] = getResidual4;
    p.calcresidual[BLOCK_8x8] = getResidual8;
    p.calcresidual[BLOCK_16x16] = getResidual<16>;
    p.calcresidual[BLOCK_32x32] = getResidual<32>;
    p.calcresidual[BLOCK_64x64] = getResidual<64>;
    p.calcrecon[BLOCK_4x4] = calcRecons4;
    p.calcrecon[BLOCK_8x8] = calcRecons8;
    p.calcrecon[BLOCK_16x16] = calcRecons<16>;
    p.calcrecon[BLOCK_32x32] = calcRecons<32>;
    p.calcrecon[BLOCK_64x64] = calcRecons<64>;
#endif /* if HIGH_BIT_DEPTH */

#if !HIGh_BIT_DEPTH
    p.transpose[0] = transpose4;
    p.transpose[1] = transpose8;
    p.transpose[2] = transpose<16>;
    p.transpose[3] = transpose<32>;
    p.transpose[4] = transpose<64>;
#endif
}
}
