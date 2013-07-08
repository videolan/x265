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

#include <assert.h>
#include "sse.inc"
#include "utils.h"

namespace {
/* File for pixels type-explicit functions */

void convert16to32(short *org, int *dst, int num)
{
    int i;

    for (i = 0; i < num; i += 8)
    {
        Vec8s im16;
        Vec4i im32L, im32H;

        im16.load(org);
        im32L = extend_low(im16);
        im32H = extend_high(im16);
        im32L.store(dst);
        im32H.store(dst + 4);

        org += 8;
        dst += 8;
    }
}

void convert16to32_shl(int *dst, short *org, intptr_t stride, int shift, int size)
{
    int i, j;

    for (i=0; i <size; i++)
    {
        for (j=0; j<size; j+=4)
        {
            __m128i im16;
            __m128i im32;

            im16 = _mm_loadl_epi64((__m128i*)&org[i*stride+j]);
            im32 = _mm_srai_epi32(_mm_unpacklo_epi16(im16, im16), 16);
            im32 = _mm_slli_epi32(im32, shift);
            _mm_storeu_si128((__m128i*)dst, im32);

            dst += 4;
        }
    }
}

void convert16to16_shl(short *dst, short *org, int width, int height, intptr_t stride, int shift)
{
    int i, j;

    if (width == 4)
    {
        for (i = 0; i < height; i+=2)
        {
            __m128i T00, T01;

            T00 = _mm_loadl_epi64((__m128i*)&org[(i  )*stride]);
            T01 = _mm_loadl_epi64((__m128i*)&org[(i+1)*stride]);
            T00 = _mm_unpacklo_epi64(T00, T01);
            T00 = _mm_slli_epi16(T00, shift);
            _mm_storeu_si128((__m128i*)&dst[i*4], T00);
        }
    }
    else
    {
        for (i = 0; i < height; i++)
        {
            for( j=0; j<width; j+=8 )
            {
                __m128i T00;

                T00 = _mm_loadu_si128((__m128i*)&org[i*stride+j]);
                T00 = _mm_slli_epi16(T00, shift);
                _mm_storeu_si128((__m128i*)&dst[i*width+j], T00);
            }
        }
    }
}

void convert32to16(int *org, short *dst, int num)
{
    int i;

    for (i = 0; i < num; i += 8)
    {
        Vec4i im32L, im32H;
        Vec8s im16;

        im32L.load(org);
        im32H.load(org + 4);
        im16 = compress_saturated(im32L, im32H);
        im16.store(dst);

        org += 8;
        dst += 8;
    }
}

void convert32to16_shr(short *dst, int *org, int shift, int num)
{
    int i;
    Vec4i round = _mm_set1_epi32(1 << (shift - 1));

    for (i = 0; i < num; i += 4)
    {
        Vec4i im32;
        Vec8s im16;

        im32.load(org);
        im32 = (im32 + round) >> shift;
        im16 = compress_saturated(im32, im32);
        store_partial(const_int(8), dst, im16);

        org += 4;
        dst += 4;
    }
}

template <int blockSize>
void transpose(pixel* dst, pixel* src, intptr_t stride)
{
    for (int k = 0; k < blockSize; k++)
    {
        for (int l = 0; l < blockSize; l++)
        {
            dst[k * blockSize + l] = src[l * stride + k];
        }
    }
}

#if !HIGH_BIT_DEPTH
void transpose4(pixel* dst, pixel* src, intptr_t stride)
{
    __m128i T00, T01, T02, T03;

    T00 = _mm_cvtsi32_si128(*(int*)&src[0*stride]);   // [03 02 01 00]
    T01 = _mm_cvtsi32_si128(*(int*)&src[1*stride]);   // [13 12 11 10]
    T02 = _mm_cvtsi32_si128(*(int*)&src[2*stride]);   // [23 22 21 20]
    T03 = _mm_cvtsi32_si128(*(int*)&src[3*stride]);   // [33 32 31 30]

    T00 = _mm_unpacklo_epi8(T00, T01);
    T01 = _mm_unpacklo_epi8(T02, T03);

    T00 = _mm_unpacklo_epi16(T00, T01);

    _mm_store_si128((__m128i*)dst, T00);
}

#define TRANSPOSE_8X8(in0, in1, in2, in3, in4, in5, in6, in7, out0, out1, out2, out3) \
{                                                      \
    const __m128i tr0_0 = _mm_unpacklo_epi8(in0, in1); \
    const __m128i tr0_1 = _mm_unpacklo_epi8(in2, in3); \
    const __m128i tr0_2 = _mm_unpacklo_epi8(in4, in5); \
    const __m128i tr0_3 = _mm_unpacklo_epi8(in6, in7); \
    const __m128i tr1_0 = _mm_unpacklo_epi16(tr0_0, tr0_1); \
    const __m128i tr1_1 = _mm_unpackhi_epi16(tr0_0, tr0_1); \
    const __m128i tr1_2 = _mm_unpacklo_epi16(tr0_2, tr0_3); \
    const __m128i tr1_3 = _mm_unpackhi_epi16(tr0_2, tr0_3); \
    out0 = _mm_unpacklo_epi32(tr1_0, tr1_2); \
    out1 = _mm_unpackhi_epi32(tr1_0, tr1_2); \
    out2 = _mm_unpacklo_epi32(tr1_1, tr1_3); \
    out3 = _mm_unpackhi_epi32(tr1_1, tr1_3); \
}

void transpose8(pixel* dst, pixel* src, intptr_t stride)
{
    __m128i T00, T01, T02, T03, T04, T05, T06, T07;

    T00 = _mm_loadl_epi64((__m128i*)&src[0*stride]);   // [07 06 05 04 03 02 01 00]
    T01 = _mm_loadl_epi64((__m128i*)&src[1*stride]);   // [17 16 15 14 13 12 11 10]
    T02 = _mm_loadl_epi64((__m128i*)&src[2*stride]);   // [27 26 25 24 23 22 21 20]
    T03 = _mm_loadl_epi64((__m128i*)&src[3*stride]);   // [37 36 35 34 33 32 31 30]
    T04 = _mm_loadl_epi64((__m128i*)&src[4*stride]);   // [47 46 45 44 43 42 41 40]
    T05 = _mm_loadl_epi64((__m128i*)&src[5*stride]);   // [57 56 55 54 53 52 51 50]
    T06 = _mm_loadl_epi64((__m128i*)&src[6*stride]);   // [67 66 65 64 63 62 61 60]
    T07 = _mm_loadl_epi64((__m128i*)&src[7*stride]);   // [77 76 75 74 73 72 71 70]

    TRANSPOSE_8X8(T00, T01, T02, T03, T04, T05, T06, T07, T00, T01, T02, T03);

    _mm_store_si128((__m128i*)&dst[0*8], T00);
    _mm_store_si128((__m128i*)&dst[2*8], T01);
    _mm_store_si128((__m128i*)&dst[4*8], T02);
    _mm_store_si128((__m128i*)&dst[6*8], T03);
}

ALWAYSINLINE void transpose16_dummy(pixel* dst, intptr_t dststride, pixel* src, intptr_t srcstride)
{
    __m128i T00, T01, T02, T03, T04, T05, T06, T07;

    T00 = _mm_loadl_epi64((__m128i*)&src[ 0 * srcstride]);
    T01 = _mm_loadl_epi64((__m128i*)&src[ 1 * srcstride]);
    T02 = _mm_loadl_epi64((__m128i*)&src[ 2 * srcstride]);
    T03 = _mm_loadl_epi64((__m128i*)&src[ 3 * srcstride]);
    T04 = _mm_loadl_epi64((__m128i*)&src[ 4 * srcstride]);
    T05 = _mm_loadl_epi64((__m128i*)&src[ 5 * srcstride]);
    T06 = _mm_loadl_epi64((__m128i*)&src[ 6 * srcstride]);
    T07 = _mm_loadl_epi64((__m128i*)&src[ 7 * srcstride]);
    TRANSPOSE_8X8(T00, T01, T02, T03, T04, T05, T06, T07, T00, T01, T02, T03);
    _mm_storel_epi64((__m128i*)&dst[ 0 * dststride], T00);
    _mm_storeh_pi(     (__m64*)&dst[ 1 * dststride], _mm_castsi128_ps(T00));
    _mm_storel_epi64((__m128i*)&dst[ 2 * dststride], T01);
    _mm_storeh_pi(     (__m64*)&dst[ 3 * dststride], _mm_castsi128_ps(T01));
    _mm_storel_epi64((__m128i*)&dst[ 4 * dststride], T02);
    _mm_storeh_pi(     (__m64*)&dst[ 5 * dststride], _mm_castsi128_ps(T02));
    _mm_storel_epi64((__m128i*)&dst[ 6 * dststride], T03);
    _mm_storeh_pi(     (__m64*)&dst[ 7 * dststride], _mm_castsi128_ps(T03));

    T00 = _mm_loadl_epi64((__m128i*)&src[ 0 * srcstride + 8]);
    T01 = _mm_loadl_epi64((__m128i*)&src[ 1 * srcstride + 8]);
    T02 = _mm_loadl_epi64((__m128i*)&src[ 2 * srcstride + 8]);
    T03 = _mm_loadl_epi64((__m128i*)&src[ 3 * srcstride + 8]);
    T04 = _mm_loadl_epi64((__m128i*)&src[ 4 * srcstride + 8]);
    T05 = _mm_loadl_epi64((__m128i*)&src[ 5 * srcstride + 8]);
    T06 = _mm_loadl_epi64((__m128i*)&src[ 6 * srcstride + 8]);
    T07 = _mm_loadl_epi64((__m128i*)&src[ 7 * srcstride + 8]);
    TRANSPOSE_8X8(T00, T01, T02, T03, T04, T05, T06, T07, T00, T01, T02, T03);
    _mm_storel_epi64((__m128i*)&dst[ 8 * dststride], T00);
    _mm_storeh_pi(     (__m64*)&dst[ 9 * dststride], _mm_castsi128_ps(T00));
    _mm_storel_epi64((__m128i*)&dst[10 * dststride], T01);
    _mm_storeh_pi(     (__m64*)&dst[11 * dststride], _mm_castsi128_ps(T01));
    _mm_storel_epi64((__m128i*)&dst[12 * dststride], T02);
    _mm_storeh_pi(     (__m64*)&dst[13 * dststride], _mm_castsi128_ps(T02));
    _mm_storel_epi64((__m128i*)&dst[14 * dststride], T03);
    _mm_storeh_pi(     (__m64*)&dst[15 * dststride], _mm_castsi128_ps(T03));

    T00 = _mm_loadl_epi64((__m128i*)&src[ 8 * srcstride]);
    T01 = _mm_loadl_epi64((__m128i*)&src[ 9 * srcstride]);
    T02 = _mm_loadl_epi64((__m128i*)&src[10 * srcstride]);
    T03 = _mm_loadl_epi64((__m128i*)&src[11 * srcstride]);
    T04 = _mm_loadl_epi64((__m128i*)&src[12 * srcstride]);
    T05 = _mm_loadl_epi64((__m128i*)&src[13 * srcstride]);
    T06 = _mm_loadl_epi64((__m128i*)&src[14 * srcstride]);
    T07 = _mm_loadl_epi64((__m128i*)&src[15 * srcstride]);
    TRANSPOSE_8X8(T00, T01, T02, T03, T04, T05, T06, T07, T00, T01, T02, T03);
    _mm_storel_epi64((__m128i*)&dst[ 0 * dststride + 8], T00);
    _mm_storeh_pi(     (__m64*)&dst[ 1 * dststride + 8], _mm_castsi128_ps(T00));
    _mm_storel_epi64((__m128i*)&dst[ 2 * dststride + 8], T01);
    _mm_storeh_pi(     (__m64*)&dst[ 3 * dststride + 8], _mm_castsi128_ps(T01));
    _mm_storel_epi64((__m128i*)&dst[ 4 * dststride + 8], T02);
    _mm_storeh_pi(     (__m64*)&dst[ 5 * dststride + 8], _mm_castsi128_ps(T02));
    _mm_storel_epi64((__m128i*)&dst[ 6 * dststride + 8], T03);
    _mm_storeh_pi(     (__m64*)&dst[ 7 * dststride + 8], _mm_castsi128_ps(T03));

    T00 = _mm_loadl_epi64((__m128i*)&src[ 8 * srcstride + 8]);
    T01 = _mm_loadl_epi64((__m128i*)&src[ 9 * srcstride + 8]);
    T02 = _mm_loadl_epi64((__m128i*)&src[10 * srcstride + 8]);
    T03 = _mm_loadl_epi64((__m128i*)&src[11 * srcstride + 8]);
    T04 = _mm_loadl_epi64((__m128i*)&src[12 * srcstride + 8]);
    T05 = _mm_loadl_epi64((__m128i*)&src[13 * srcstride + 8]);
    T06 = _mm_loadl_epi64((__m128i*)&src[14 * srcstride + 8]);
    T07 = _mm_loadl_epi64((__m128i*)&src[15 * srcstride + 8]);
    TRANSPOSE_8X8(T00, T01, T02, T03, T04, T05, T06, T07, T00, T01, T02, T03);
    _mm_storel_epi64((__m128i*)&dst[ 8 * dststride + 8], T00);
    _mm_storeh_pi(     (__m64*)&dst[ 9 * dststride + 8], _mm_castsi128_ps(T00));
    _mm_storel_epi64((__m128i*)&dst[10 * dststride + 8], T01);
    _mm_storeh_pi(     (__m64*)&dst[11 * dststride + 8], _mm_castsi128_ps(T01));
    _mm_storel_epi64((__m128i*)&dst[12 * dststride + 8], T02);
    _mm_storeh_pi(     (__m64*)&dst[13 * dststride + 8], _mm_castsi128_ps(T02));
    _mm_storel_epi64((__m128i*)&dst[14 * dststride + 8], T03);
    _mm_storeh_pi(     (__m64*)&dst[15 * dststride + 8], _mm_castsi128_ps(T03));
}

void transpose16(pixel* dst, pixel* src, intptr_t srcstride)
{
    transpose16_dummy(dst, 16, src, srcstride);
}

void transpose32(pixel* dst, pixel* src, intptr_t srcstride)
{
    assert( dst != src );

    transpose16_dummy(dst,                32, src,                      srcstride);
    transpose16_dummy(dst + 16 * 32,      32, src + 16,                 srcstride);
    transpose16_dummy(dst + 16 * 32 + 16, 32, src + 16 * srcstride + 16, srcstride);
    transpose16_dummy(dst + 16,           32, src + 16 * srcstride,      srcstride);
}
#endif // !HIGH_BIT_DEPTH

}  // end anonymous namespace

namespace x265 {
// private x265 namespace

#define SET_FUNC_PRIMITIVE_TABLE_WIDTH(WIDTH, FUNC_PREFIX, FUNC_PREFIX_DEF, FUNC_TYPE_CAST) \
    p. FUNC_PREFIX [PARTITION_##WIDTH##x4 ] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## WIDTH < 4 >;  \
    p. FUNC_PREFIX [PARTITION_##WIDTH##x8 ] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## WIDTH < 8 >;  \
    p. FUNC_PREFIX [PARTITION_##WIDTH##x12] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## WIDTH < 12 >; \
    p. FUNC_PREFIX [PARTITION_##WIDTH##x16] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## WIDTH < 16 >; \
    p. FUNC_PREFIX [PARTITION_##WIDTH##x24] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## WIDTH < 24 >; \
    p. FUNC_PREFIX [PARTITION_##WIDTH##x32] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## WIDTH < 32 >; \
    p. FUNC_PREFIX [PARTITION_##WIDTH##x48] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## WIDTH < 48 >; \
    p. FUNC_PREFIX [PARTITION_##WIDTH##x64] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## WIDTH < 64 >;

#define SET_FUNC_PRIMITIVE_TABLE(FUNC_PREFIX, FUNC_PREFIX_DEF, FUNC_TYPE_CAST) \
    SET_FUNC_PRIMITIVE_TABLE_WIDTH(4,  FUNC_PREFIX, FUNC_PREFIX_DEF, FUNC_TYPE_CAST) \
    SET_FUNC_PRIMITIVE_TABLE_WIDTH(8,  FUNC_PREFIX, FUNC_PREFIX_DEF, FUNC_TYPE_CAST) \
    SET_FUNC_PRIMITIVE_TABLE_WIDTH(12, FUNC_PREFIX, FUNC_PREFIX_DEF, FUNC_TYPE_CAST) \
    SET_FUNC_PRIMITIVE_TABLE_WIDTH(16, FUNC_PREFIX, FUNC_PREFIX_DEF, FUNC_TYPE_CAST) \
    SET_FUNC_PRIMITIVE_TABLE_WIDTH(24, FUNC_PREFIX, FUNC_PREFIX_DEF, FUNC_TYPE_CAST) \
    SET_FUNC_PRIMITIVE_TABLE_WIDTH(32, FUNC_PREFIX, FUNC_PREFIX_DEF, FUNC_TYPE_CAST) \
    SET_FUNC_PRIMITIVE_TABLE_WIDTH(48, FUNC_PREFIX, FUNC_PREFIX_DEF, FUNC_TYPE_CAST) \
    SET_FUNC_PRIMITIVE_TABLE_WIDTH(64, FUNC_PREFIX, FUNC_PREFIX_DEF, FUNC_TYPE_CAST)

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
    p.calcresidual[BLOCK_16x16] = getResidual16;
    p.calcresidual[BLOCK_32x32] = getResidual32;
    p.calcresidual[BLOCK_64x64] = getResidual64;
    p.calcrecon[BLOCK_4x4] = calcRecons4;
    p.calcrecon[BLOCK_8x8] = calcRecons8;
    p.calcrecon[BLOCK_16x16] = calcRecons<16>;
    p.calcrecon[BLOCK_32x32] = calcRecons<32>;
    p.calcrecon[BLOCK_64x64] = calcRecons<64>;
#endif /* if HIGH_BIT_DEPTH */

#if !HIGH_BIT_DEPTH
    p.transpose[0] = transpose4;
    p.transpose[1] = transpose8;
    p.transpose[2] = transpose16;
    p.transpose[3] = transpose32;
    p.transpose[4] = transpose<64>;
#endif
}
}
