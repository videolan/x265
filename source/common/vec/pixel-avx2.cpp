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

/* this file instantiates AVX2 versions of the pixel primitives */

#include "primitives.h"

#define INSTRSET 8
#include "vectorclass.h"

using namespace x265;

namespace {
#if !HIGH_BIT_DEPTH
template<int size>
ALWAYSINLINE void unrollFunc_32_avx2(pixel *fenc, intptr_t fencstride, pixel *fref, intptr_t frefstride, Vec16us& sad)
{
    unrollFunc_32_avx2<1>(fenc, fencstride, fref, frefstride, sad);
    unrollFunc_32_avx2<size - 1>(fenc + fencstride, fencstride, fref + frefstride, frefstride, sad);
}

template<>
ALWAYSINLINE void unrollFunc_32_avx2<1>(pixel *fenc, intptr_t, pixel *fref, intptr_t, Vec16us& sad)
{
    Vec32uc m1, n1;

    m1.load_a(fenc);
    n1.load(fref);
    sad.addSumAbsDiff(m1, n1);
}

template<int ly>
int sad_avx2_32(pixel * fenc, intptr_t fencstride, pixel * fref, intptr_t frefstride)
{
    Vec8i sum(0);
    Vec16us sad(0);
    int max_iterators = (ly >> 4) << 4;
    int row = 0;

    if (ly < 16)
    {
        unrollFunc_32_avx2<ly>(fenc, fencstride, fref, frefstride, sad);
        sum += extend_low(sad) + extend_high(sad);
        return horizontal_add(sum);
    }
    for (row = 0; row < max_iterators; row += 16)
    {
        unrollFunc_32_avx2<16>(fenc, fencstride, fref, frefstride, sad);

        sum += extend_low(sad) + extend_high(sad);
        sad = 0;
        fenc += fencstride * 16;
        fref += frefstride * 16;
    }

    if (ly & 8)
    {
        unrollFunc_32_avx2<8>(fenc, fencstride, fref, frefstride, sad);
        sum += extend_low(sad) + extend_high(sad);
        return horizontal_add(sum);
    }
    return horizontal_add(sum);
}

template<int size>
ALWAYSINLINE void unrollFunc_64_avx2(pixel *fenc, intptr_t fencstride, pixel *fref, intptr_t frefstride, Vec16s& sad)
{
    unrollFunc_64_avx2<1>(fenc, fencstride, fref, frefstride, sad);
    unrollFunc_64_avx2<size - 1>(fenc + fencstride, fencstride, fref + frefstride, frefstride, sad);
}

template<>
ALWAYSINLINE void unrollFunc_64_avx2<1>(pixel *fenc, intptr_t, pixel *fref, intptr_t, Vec16s& sad)
{
    Vec32uc m1, n1;

    m1.load_a(fenc);
    n1.load(fref);
    sad.addSumAbsDiff(m1, n1);

    m1.load_a(fenc + 32);
    n1.load(fref + 32);
    sad.addSumAbsDiff(m1, n1);
}

template<int ly>
int sad_avx2_64(pixel * fenc, intptr_t fencstride, pixel * fref, intptr_t frefstride)
{
    Vec8i sum(0);
    Vec16s sad;
    int max_iterators = (ly >> 2) << 2;
    int row;

    if (ly == 4)
    {
        sad = 0;
        unrollFunc_64_avx2<4>(fenc, fencstride, fref, frefstride, sad);
        sum += extend_low(sad) + extend_high(sad);
        return horizontal_add(sum);
    }
    for (row = 0; row < max_iterators; row += 4)
    {
        sad = 0;
        unrollFunc_64_avx2<4>(fenc, fencstride, fref, frefstride, sad);
        sum += extend_low(sad) + extend_high(sad);
        fenc += fencstride * 4;
        fref += frefstride * 4;
    }

    return horizontal_add(sum);
}

template<int ly>
void sad_avx2_x3_32(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, intptr_t frefstride, int *res)
{
    Vec32uc m1, n1, n2, n3;

    Vec8i sum1(0), sum2(0), sum3(0);
    Vec16us sad1(0), sad2(0), sad3(0);
    int max_iterators = (ly >> 4) << 4;
    int row;

    for (row = 0; row < max_iterators; row += 16)
    {
        for (int i = 0; i < 16; i++)
        {
            m1.load_a(fenc);
            n1.load(fref1);
            n2.load(fref2);
            n3.load(fref3);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            fenc += FENC_STRIDE;
            fref1 += frefstride;
            fref2 += frefstride;
            fref3 += frefstride;
        }

        sum1 += extend_low(sad1) + extend_high(sad1);
        sum2 += extend_low(sad2) + extend_high(sad2);
        sum3 += extend_low(sad3) + extend_high(sad3);
        sad1 = 0;
        sad2 = 0;
        sad3 = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(fenc);
        n1.load(fref1);
        n2.load(fref2);
        n3.load(fref3);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);

        fenc += FENC_STRIDE;
        fref1 += frefstride;
        fref2 += frefstride;
        fref3 += frefstride;
    }

    sum1 += extend_low(sad1) + extend_high(sad1);
    sum2 += extend_low(sad2) + extend_high(sad2);
    sum3 += extend_low(sad3) + extend_high(sad3);

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
}

template<int ly>
void sad_avx2_x3_64(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, intptr_t frefstride, int *res)
{
    Vec32uc m1, n1, n2, n3;

    Vec8i sum1(0), sum2(0), sum3(0);
    Vec16s sad1(0), sad2(0), sad3(0);
    int max_iterators = (ly >> 3) << 3;
    int row;

    for (row = 0; row < max_iterators; row += 8)
    {
        for (int i = 0; i < 8; i++)
        {
            m1.load_a(fenc);
            n1.load(fref1);
            n2.load(fref2);
            n3.load(fref3);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            m1.load_a(fenc + 32);
            n1.load(fref1 + 32);
            n2.load(fref2 + 32);
            n3.load(fref3 + 32);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            fenc += FENC_STRIDE;
            fref1 += frefstride;
            fref2 += frefstride;
            fref3 += frefstride;
        }

        sum1 += extend_low(sad1) + extend_high(sad1);
        sum2 += extend_low(sad2) + extend_high(sad2);
        sum3 += extend_low(sad3) + extend_high(sad3);
        sad1 = 0;
        sad2 = 0;
        sad3 = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(fenc);
        n1.load(fref1);
        n2.load(fref2);
        n3.load(fref3);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);

        m1.load_a(fenc + 32);
        n1.load(fref1 + 32);
        n2.load(fref2 + 32);
        n3.load(fref3 + 32);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);

        fenc += FENC_STRIDE;
        fref1 += frefstride;
        fref2 += frefstride;
        fref3 += frefstride;
    }

    sum1 += extend_low(sad1) + extend_high(sad1);
    sum2 += extend_low(sad2) + extend_high(sad2);
    sum3 += extend_low(sad3) + extend_high(sad3);

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
}

template<int ly>
void sad_avx2_x4_32(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, pixel *fref4, intptr_t frefstride, int *res)
{
    Vec32uc m1, n1, n2, n3, n4;

    Vec8i sum1(0), sum2(0), sum3(0), sum4(0);
    Vec16s sad1(0), sad2(0), sad3(0), sad4(0);
    int max_iterators = (ly >> 4) << 4;
    int row;

    for (row = 0; row < max_iterators; row += 16)
    {
        for (int i = 0; i < 16; i++)
        {
            m1.load_a(fenc);
            n1.load(fref1);
            n2.load(fref2);
            n3.load(fref3);
            n4.load(fref4);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            fenc += FENC_STRIDE;
            fref1 += frefstride;
            fref2 += frefstride;
            fref3 += frefstride;
            fref4 += frefstride;
        }

        sum1 += extend_low(sad1) + extend_high(sad1);
        sum2 += extend_low(sad2) + extend_high(sad2);
        sum3 += extend_low(sad3) + extend_high(sad3);
        sum4 += extend_low(sad4) + extend_high(sad4);
        sad1 = 0;
        sad2 = 0;
        sad3 = 0;
        sad4 = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(fenc);
        n1.load(fref1);
        n2.load(fref2);
        n3.load(fref3);
        n4.load(fref4);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);
        sad4.addSumAbsDiff(m1, n4);

        fenc += FENC_STRIDE;
        fref1 += frefstride;
        fref2 += frefstride;
        fref3 += frefstride;
        fref4 += frefstride;
    }

    sum1 += extend_low(sad1) + extend_high(sad1);
    sum2 += extend_low(sad2) + extend_high(sad2);
    sum3 += extend_low(sad3) + extend_high(sad3);
    sum4 += extend_low(sad4) + extend_high(sad4);

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
    res[3] = horizontal_add(sum4);
}

template<int ly>
void sad_avx2_x4_64(pixel *fenc, pixel *fref1, pixel *fref2, pixel *fref3, pixel *fref4, intptr_t frefstride, int *res)
{
    Vec32uc m1, n1, n2, n3, n4;

    Vec8i sum1(0), sum2(0), sum3(0), sum4(0);
    Vec16s sad1(0), sad2(0), sad3(0), sad4(0);
    int max_iterators = (ly >> 3) << 3;
    int row;

    for (row = 0; row < max_iterators; row += 8)
    {
        for (int i = 0; i < 8; i++)
        {
            m1.load_a(fenc);
            n1.load(fref1);
            n2.load(fref2);
            n3.load(fref3);
            n4.load(fref4);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            m1.load_a(fenc + 32);
            n1.load(fref1 + 32);
            n2.load(fref2 + 32);
            n3.load(fref3 + 32);
            n4.load(fref4 + 32);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            fenc += FENC_STRIDE;
            fref1 += frefstride;
            fref2 += frefstride;
            fref3 += frefstride;
            fref4 += frefstride;
        }

        sum1 += extend_low(sad1) + extend_high(sad1);
        sum2 += extend_low(sad2) + extend_high(sad2);
        sum3 += extend_low(sad3) + extend_high(sad3);
        sum4 += extend_low(sad4) + extend_high(sad4);
        sad1 = 0;
        sad2 = 0;
        sad3 = 0;
        sad4 = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(fenc);
        n1.load(fref1);
        n2.load(fref2);
        n3.load(fref3);
        n4.load(fref4);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);
        sad4.addSumAbsDiff(m1, n4);

        m1.load_a(fenc + 32);
        n1.load(fref1 + 32);
        n2.load(fref2 + 32);
        n3.load(fref3 + 32);
        n4.load(fref4 + 32);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);
        sad4.addSumAbsDiff(m1, n4);

        fenc += FENC_STRIDE;
        fref1 += frefstride;
        fref2 += frefstride;
        fref3 += frefstride;
        fref4 += frefstride;
    }

    sum1 += extend_low(sad1) + extend_high(sad1);
    sum2 += extend_low(sad2) + extend_high(sad2);
    sum3 += extend_low(sad3) + extend_high(sad3);
    sum4 += extend_low(sad4) + extend_high(sad4);

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
    res[3] = horizontal_add(sum4);
}

#endif // if !HIGH_BIT_DEPTH
}

namespace x265 {
void Setup_Vec_PixelPrimitives_avx2(EncoderPrimitives &p)
{
    p.sad[0] = p.sad[0];
#define SET_SADS(W, H) \
    p.sad[LUMA_ ## W ## x ## H] = sad_avx2_ ## W<H>; \
    p.sad_x3[LUMA_ ## W ## x ## H] = sad_avx2_x3_ ## W<H>; \
    p.sad_x4[LUMA_ ## W ## x ## H] = sad_avx2_x4_ ## W<H>; \

#if !HIGH_BIT_DEPTH
#if (defined(__GNUC__) || defined(__INTEL_COMPILER)) || defined(__clang__)
    SET_SADS(32, 8);
    SET_SADS(32, 16);
    SET_SADS(32, 24);
    SET_SADS(32, 32);
    SET_SADS(32, 64);
    SET_SADS(64, 16);
    SET_SADS(64, 32);
    SET_SADS(64, 48);
    SET_SADS(64, 64);
#endif
#endif // if !HIGH_BIT_DEPTH
}
}
