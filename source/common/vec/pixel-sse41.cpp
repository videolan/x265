/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
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
 * For more information, contact us at licensing@multicorewareinc.com
 *****************************************************************************/

/* this file instantiates SSE4.1 versions of the pixel primitives */

#include "TLibCommon/TComRom.h"
#include "primitives.h"
#include <assert.h>
#include <xmmintrin.h> // SSE
#include <smmintrin.h> // SSE4.1

using namespace x265;

namespace {
#define PROCESS_SSE_SS4x1(BASE) \
    m1 = _mm_loadu_si128((__m128i const*)(fenc + BASE)); \
    n1 = _mm_loadu_si128((__m128i const*)(fref + BASE)); \
    tmp1 = _mm_cvtepi16_epi32(m1); \
    tmp2 = _mm_cvtepi16_epi32(n1); \
    diff = _mm_sub_epi32(tmp1, tmp2); \
    diff = _mm_mullo_epi32(diff, diff); \
    sum = _mm_add_epi32(sum, diff)

template<int ly>
int sse_ss4(int16_t* fenc, intptr_t strideFenc, int16_t* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i diff;
    __m128i sum  = _mm_setzero_si128();
    __m128i m1, n1, tmp1, tmp2;

    for (; rows != 0; rows--)
    {
        PROCESS_SSE_SS4x1(0);

        fenc += strideFenc;
        fref += strideFref;
    }

    __m128i sum1  = _mm_hadd_epi32(sum, sum);         // horizontally add 4 elements
    return _mm_cvtsi128_si32(_mm_hadd_epi32(sum1, sum1));
}

template<int ly>
int sse_ss8(int16_t* fenc, intptr_t strideFenc, int16_t* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i sum  = _mm_setzero_si128();
    __m128i m1, n1, diff, sign, tmp1, tmp2;

    for (; rows != 0; rows--)
    {
        PROCESS_SSE_SS4x1(0);

        sign = _mm_srai_epi16(m1, 15);
        m1 = _mm_unpackhi_epi16(m1, sign);
        sign = _mm_srai_epi16(n1, 15);
        n1 = _mm_unpackhi_epi16(n1, sign);
        diff = _mm_sub_epi32(m1, n1);
        diff = _mm_mullo_epi32(diff, diff);
        sum = _mm_add_epi32(sum, diff);

        fenc += strideFenc;
        fref += strideFref;
    }

    __m128i sum1  = _mm_hadd_epi32(sum, sum);
    return _mm_cvtsi128_si32(_mm_hadd_epi32(sum1, sum1));
}

template<int ly>
int sse_ss12(int16_t* fenc, intptr_t strideFenc, int16_t* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i sum  = _mm_setzero_si128();
    __m128i m1, n1, diff, sign, tmp1, tmp2;

    for (; rows != 0; rows--)
    {
        PROCESS_SSE_SS4x1(0);

        sign = _mm_srai_epi16(m1, 15);
        m1 = _mm_unpackhi_epi16(m1, sign);
        sign = _mm_srai_epi16(n1, 15);
        n1 = _mm_unpackhi_epi16(n1, sign);
        diff = _mm_sub_epi32(m1, n1);
        diff = _mm_mullo_epi32(diff, diff);
        sum = _mm_add_epi32(sum, diff);

        PROCESS_SSE_SS4x1(8);

        fenc += strideFenc;
        fref += strideFref;
    }

    __m128i sum1  = _mm_hadd_epi32(sum, sum);
    return _mm_cvtsi128_si32(_mm_hadd_epi32(sum1, sum1));
}

template<int ly>
int sse_ss16(int16_t* fenc, intptr_t strideFenc, int16_t* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i sum  = _mm_setzero_si128();
    __m128i m1, n1, diff, sign, tmp1, tmp2;

    for (; rows != 0; rows--)
    {
        for (int i = 0; i < 16; i += 8)
        {
            PROCESS_SSE_SS4x1(i);

            sign = _mm_srai_epi16(m1, 15);
            m1 = _mm_unpackhi_epi16(m1, sign);
            sign = _mm_srai_epi16(n1, 15);
            n1 = _mm_unpackhi_epi16(n1, sign);
            diff = _mm_sub_epi32(m1, n1);
            diff = _mm_mullo_epi32(diff, diff);
            sum = _mm_add_epi32(sum, diff);
        }

        fenc += strideFenc;
        fref += strideFref;
    }

    __m128i sum1  = _mm_hadd_epi32(sum, sum);
    return _mm_cvtsi128_si32(_mm_hadd_epi32(sum1, sum1));
}

template<int ly>
int sse_ss24(int16_t* fenc, intptr_t strideFenc, int16_t* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i sum  = _mm_setzero_si128();
    __m128i m1, n1, diff, sign, tmp1, tmp2;

    for (; rows != 0; rows--)
    {
        for (int i = 0; i < 24; i += 8)
        {
            PROCESS_SSE_SS4x1(i);

            sign = _mm_srai_epi16(m1, 15);
            m1 = _mm_unpackhi_epi16(m1, sign);
            sign = _mm_srai_epi16(n1, 15);
            n1 = _mm_unpackhi_epi16(n1, sign);
            diff = _mm_sub_epi32(m1, n1);
            diff = _mm_mullo_epi32(diff, diff);
            sum = _mm_add_epi32(sum, diff);
        }

        fenc += strideFenc;
        fref += strideFref;
    }

    __m128i sum1  = _mm_hadd_epi32(sum, sum);
    return _mm_cvtsi128_si32(_mm_hadd_epi32(sum1, sum1));
}

template<int ly>
int sse_ss32(int16_t* fenc, intptr_t strideFenc, int16_t* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i sum  = _mm_setzero_si128();
    __m128i m1, n1, diff, sign, tmp1, tmp2;

    for (; rows != 0; rows--)
    {
        for (int i = 0; i < 32; i += 8)
        {
            PROCESS_SSE_SS4x1(i);

            sign = _mm_srai_epi16(m1, 15);
            m1 = _mm_unpackhi_epi16(m1, sign);
            sign = _mm_srai_epi16(n1, 15);
            n1 = _mm_unpackhi_epi16(n1, sign);
            diff = _mm_sub_epi32(m1, n1);
            diff = _mm_mullo_epi32(diff, diff);
            sum = _mm_add_epi32(sum, diff);
        }

        fenc += strideFenc;
        fref += strideFref;
    }

    __m128i sum1  = _mm_hadd_epi32(sum, sum);
    return _mm_cvtsi128_si32(_mm_hadd_epi32(sum1, sum1));
}

template<int ly>
int sse_ss48(int16_t* fenc, intptr_t strideFenc, int16_t* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i sum  = _mm_setzero_si128();
    __m128i m1, n1, diff, sign, tmp1, tmp2;

    for (; rows != 0; rows--)
    {
        for (int i = 0; i < 48; i += 8)
        {
            PROCESS_SSE_SS4x1(i);

            sign = _mm_srai_epi16(m1, 15);
            m1 = _mm_unpackhi_epi16(m1, sign);
            sign = _mm_srai_epi16(n1, 15);
            n1 = _mm_unpackhi_epi16(n1, sign);
            diff = _mm_sub_epi32(m1, n1);
            diff = _mm_mullo_epi32(diff, diff);
            sum = _mm_add_epi32(sum, diff);
        }

        fenc += strideFenc;
        fref += strideFref;
    }

    __m128i sum1  = _mm_hadd_epi32(sum, sum);
    return _mm_cvtsi128_si32(_mm_hadd_epi32(sum1, sum1));
}

template<int ly>
int sse_ss64(int16_t* fenc, intptr_t strideFenc, int16_t* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i sum  = _mm_setzero_si128();
    __m128i m1, n1, diff, sign, tmp1, tmp2;

    for (; rows != 0; rows--)
    {
        for (int i = 0; i < 64; i += 8)
        {
            PROCESS_SSE_SS4x1(i);

            sign = _mm_srai_epi16(m1, 15);
            m1 = _mm_unpackhi_epi16(m1, sign);
            sign = _mm_srai_epi16(n1, 15);
            n1 = _mm_unpackhi_epi16(n1, sign);
            diff = _mm_sub_epi32(m1, n1);
            diff = _mm_mullo_epi32(diff, diff);
            sum = _mm_add_epi32(sum, diff);
        }

        fenc += strideFenc;
        fref += strideFref;
    }

    __m128i sum1  = _mm_hadd_epi32(sum, sum);
    return _mm_cvtsi128_si32(_mm_hadd_epi32(sum1, sum1));
}
}

namespace x265 {
extern void Setup_Vec_Pixel16Primitives_sse41(EncoderPrimitives &p);

#if HIGH_BIT_DEPTH
#define SETUP_SSE(W, H) \
    p.sse_sp[LUMA_ ## W ## x ## H] = (pixelcmp_sp_t)sse_ss ## W<H>; \
    p.sse_ss[LUMA_ ## W ## x ## H] = sse_ss ## W < H >
#else
#define SETUP_SSE(W, H) \
    p.sse_ss[LUMA_ ## W ## x ## H] = sse_ss ## W < H >
#endif // if HIGH_BIT_DEPTH

void Setup_Vec_PixelPrimitives_sse41(EncoderPrimitives &p)
{
    /* 2Nx2N, 2NxN, Nx2N, 4Ax3A, 4AxA, 3Ax4A, Ax4A */
    SETUP_SSE(64, 64);
    SETUP_SSE(64, 32);
    SETUP_SSE(32, 64);
    SETUP_SSE(64, 16);
    SETUP_SSE(64, 48);
    SETUP_SSE(16, 64);
    SETUP_SSE(48, 64);

    SETUP_SSE(32, 32);
    SETUP_SSE(32, 16);
    SETUP_SSE(16, 32);
    SETUP_SSE(32, 8);
    SETUP_SSE(32, 24);
    SETUP_SSE(8, 32);
    SETUP_SSE(24, 32);

    SETUP_SSE(16, 16);
    SETUP_SSE(16, 8);
    SETUP_SSE(8, 16);
    SETUP_SSE(16, 4);
    SETUP_SSE(16, 12);
    SETUP_SSE(4, 16);
    SETUP_SSE(12, 16);

    SETUP_SSE(8, 8);
    SETUP_SSE(8, 4);
    SETUP_SSE(4, 8);
    /* 8x8 is too small for AMP partitions */

    SETUP_SSE(4, 4);
    /* 4x4 is too small for any sub partitions */

#if HIGH_BIT_DEPTH
    Setup_Vec_Pixel16Primitives_sse41(p);
#else
#endif /* !HIGH_BIT_DEPTH */
}
}
