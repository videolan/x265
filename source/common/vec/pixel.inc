/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Mandar Gurav <mandar@multicorewareinc.com>
 *          Mahesh Pittala <mahesh@multicorewareinc.com>
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

// Vector class versions of pixel comparison performance primitives

#include "TLibCommon/TComRom.h"
#include <assert.h>

namespace {
#if HIGH_BIT_DEPTH
#include "pixel16.inc"
#else
#include "pixel8.inc"
#endif
#include "sse.inc"

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

    for (i = 0; i < size; i++)
    {
        for (j = 0; j < size; j += 4)
        {
            __m128i im16;
            __m128i im32;

            im16 = _mm_loadl_epi64((__m128i*)&org[i * stride + j]);
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
        for (i = 0; i < height; i += 2)
        {
            __m128i T00, T01;

            T00 = _mm_loadl_epi64((__m128i*)&org[(i) * stride]);
            T01 = _mm_loadl_epi64((__m128i*)&org[(i + 1) * stride]);
            T00 = _mm_unpacklo_epi64(T00, T01);
            T00 = _mm_slli_epi16(T00, shift);
            _mm_storeu_si128((__m128i*)&dst[i * 4], T00);
        }
    }
    else
    {
        for (i = 0; i < height; i++)
        {
            for (j = 0; j < width; j += 8)
            {
                __m128i T00;

                T00 = _mm_loadu_si128((__m128i*)&org[i * stride + j]);
                T00 = _mm_slli_epi16(T00, shift);
                _mm_storeu_si128((__m128i*)&dst[i * width + j], T00);
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
}  // end anonymous namespace

#define PASTER(name, val) name ## _ ## val
#define EVALUATOR(x, y)   PASTER(x, y)
#define NAME(func)        EVALUATOR(func, ARCH)

namespace x265 {
// private x265 namespace

#define SET_FUNC_PRIMITIVE_TABLE_WIDTH(WIDTH, FUNC_PREFIX, FUNC_PREFIX_DEF, FUNC_TYPE_CAST) \
    p.FUNC_PREFIX[PARTITION_ ## WIDTH ## x4] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## WIDTH<4>;  \
    p.FUNC_PREFIX[PARTITION_ ## WIDTH ## x8] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## WIDTH<8>;  \
    p.FUNC_PREFIX[PARTITION_ ## WIDTH ## x12] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## WIDTH<12>; \
    p.FUNC_PREFIX[PARTITION_ ## WIDTH ## x16] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## WIDTH<16>; \
    p.FUNC_PREFIX[PARTITION_ ## WIDTH ## x24] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## WIDTH<24>; \
    p.FUNC_PREFIX[PARTITION_ ## WIDTH ## x32] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## WIDTH<32>; \
    p.FUNC_PREFIX[PARTITION_ ## WIDTH ## x48] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## WIDTH<48>; \
    p.FUNC_PREFIX[PARTITION_ ## WIDTH ## x64] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF ## WIDTH<64>;

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
#if INSTRSET >= X265_CPU_LEVEL_SSE41
    SET_FUNC_PRIMITIVE_TABLE(sad, sad_, pixelcmp_t)
    SET_FUNC_PRIMITIVE_TABLE(sad_x3, sad_x3_, pixelcmp_x3_t)
    SET_FUNC_PRIMITIVE_TABLE(sad_x4, sad_x4_, pixelcmp_x4_t)
#endif

    p.cvt16to32     = convert16to32;
    p.cvt16to32_shl = convert16to32_shl;
    p.cvt16to16_shl = convert16to16_shl;
    p.cvt32to16     = convert32to16;
    p.cvt32to16_shr = convert32to16_shr;

#if HIGH_BIT_DEPTH

    SET_FUNC_PRIMITIVE_TABLE(sse_pp, sse_ss, pixelcmp_t)
    SET_FUNC_PRIMITIVE_TABLE(sse_sp, sse_ss, pixelcmp_sp_t)
    SET_FUNC_PRIMITIVE_TABLE(sse_ss, sse_ss, pixelcmp_ss_t)
    p.satd[PARTITION_4x4] = satd_4x4;
    p.satd[PARTITION_4x8] = satd<4, 8>;
    p.satd[PARTITION_16x4] = satd<16, 4>;

#else

#if INSTRSET >= X265_CPU_LEVEL_SSE41
    SET_FUNC_PRIMITIVE_TABLE(sse_pp, sse_pp, pixelcmp_t)
#endif
    SET_FUNC_PRIMITIVE_TABLE(sse_sp, sse_sp, pixelcmp_sp_t)
    SET_FUNC_PRIMITIVE_TABLE(sse_ss, sse_ss, pixelcmp_ss_t)

    p.transpose[0] = transpose4;
    p.transpose[1] = transpose8;
    p.transpose[2] = transpose16;
    p.transpose[3] = transpose32;
    p.weightpUniPixel = weightUnidir<pixel>;
    p.weightpUni = weightUnidir<uint16_t>;
    p.blockfil_s[BLOCK_4x4]   = blockfil_s_4;
    p.blockfil_s[BLOCK_8x8]   = blockfil_s_8;
    p.blockfil_s[BLOCK_16x16] = blockfil_s_16;
    p.blockfil_s[BLOCK_32x32] = blockfil_s_32;

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
}
}
