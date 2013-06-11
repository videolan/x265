/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Mandar Gurav <mandar@multicorewareinc.com>
 *          Mahesh Pittala <mahesh@multicorewareinc.com>
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
/* intrinsics for when pixel type is uint8_t */

template<int ly>
int CDECL sad_4(pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    Vec16uc m1, n1;

    Vec4i sum(0);
    Vec8us sad(0);
    int main_iters = (ly >> 4) << 4;
    int row;

    for (row = 0; row < main_iters; row += 16)
    {
        for (int i = 0; i < 16; i++)
        {
            m1.fromUint32(*(uint32_t*)piOrg);
            n1.fromUint32(*(uint32_t*)piCur);
            sad.addSumAbsDiff(m1, n1);

            piOrg += strideOrg;
            piCur += strideCur;
        }

        sum += extend_low(sad);
        sad = 0;
    }

    while (row++ < ly)
    {
        m1.fromUint32(*(uint32_t*)piOrg);
        n1.fromUint32(*(uint32_t*)piCur);
        sad.addSumAbsDiff(m1, n1);

        piOrg += strideOrg;
        piCur += strideCur;
    }

    sum += extend_low(sad);
    return horizontal_add(sum);
}

template<int ly>
int CDECL sad_8(pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    Vec16uc m1, n1;

    Vec4i sum(0);
    Vec8us sad(0);
    int main_iters = (ly >> 4) << 4;
    int row;

    for (row = 0; row < main_iters; row += 16)
    {
        for (int i = 0; i < 16; i++)
        {
            m1.load_a(piOrg);
            n1.load(piCur);
            sad.addSumAbsDiff(m1, n1);

            piOrg += strideOrg;
            piCur += strideCur;
        }

        sum += extend_low(sad);
        sad = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(piOrg);
        n1.load(piCur);
        sad.addSumAbsDiff(m1, n1);

        piOrg += strideOrg;
        piCur += strideCur;
    }

    sum += extend_low(sad);
    return horizontal_add(sum);
}

template<int ly>
int CDECL sad_12(pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    Vec16uc m1, n1;

    Vec4i sum(0);
    Vec8us sad(0);
    int main_iters = (ly >> 4) << 4;
    int row;

    for (row = 0; row < main_iters; row += 16)
    {
        for (int i = 0; i < 16; i++)
        {
            m1.load_a(piOrg);
            m1.cutoff(12);
            n1.load(piCur);
            n1.cutoff(12);
            sad.addSumAbsDiff(m1, n1);

            piOrg += strideOrg;
            piCur += strideCur;
        }

        sum += extend_low(sad) + extend_high(sad);
        sad = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(piOrg);
        m1.cutoff(12);
        n1.load(piCur);
        n1.cutoff(12);
        sad.addSumAbsDiff(m1, n1);

        piOrg += strideOrg;
        piCur += strideCur;
    }

    sum += extend_low(sad) + extend_high(sad);
    return horizontal_add(sum);
}

template<int ly>
int CDECL sad_16(pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    Vec16uc m1, n1;

    Vec4i sum(0);
    Vec8us sad(0);
    int main_iters = (ly >> 4) << 4;
    int row;

    for (row = 0; row < main_iters; row += 16)
    {
        for (int i = 0; i < 16; i++)
        {
            m1.load_a(piOrg);
            n1.load(piCur);
            sad.addSumAbsDiff(m1, n1);

            piOrg += strideOrg;
            piCur += strideCur;
        }

        sum += extend_low(sad) + extend_high(sad);
        sad = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(piOrg);
        n1.load(piCur);
        sad.addSumAbsDiff(m1, n1);

        piOrg += strideOrg;
        piCur += strideCur;
    }

    sum += extend_low(sad) + extend_high(sad);
    return horizontal_add(sum);
}

template<int ly>
int CDECL sad_24(pixel *piOrg, intptr_t strideOrg, pixel *piCur, intptr_t strideCur)
{
    Vec16uc m1, n1;

    Vec4i sum(0);
    Vec8us sad(0);
    int main_iters = (ly >> 4) << 4;
    int row;

    for (row = 0; row < main_iters; row += 16)
    {
        for (int i = 0; i < 16; i++)
        {
            m1.load_a(piOrg);
            n1.load(piCur);
            sad.addSumAbsDiff(m1, n1);

            m1.load_a(piOrg + 16);
            m1.cutoff(8);
            n1.load(piCur + 16);
            n1.cutoff(8);
            sad.addSumAbsDiff(m1, n1);

            piOrg += strideOrg;
            piCur += strideCur;
        }

        sum += extend_low(sad) + extend_high(sad);
        sad = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(piOrg);
        n1.load(piCur);
        sad.addSumAbsDiff(m1, n1);

        m1.load_a(piOrg + 16);
        m1.cutoff(8);
        n1.load(piCur + 16);
        n1.cutoff(8);
        sad.addSumAbsDiff(m1, n1);

        piOrg += strideOrg;
        piCur += strideCur;
    }

    sum += extend_low(sad) + extend_high(sad);
    return horizontal_add(sum);
}

template<int ly>
int CDECL sad_32(pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    Vec16uc m1, n1;

    Vec4i sum(0);
    Vec8us sad(0);
    int main_iters = (ly >> 3) << 3;
    int row;

    for (row = 0; row < main_iters; row += 8)
    {
        for (int i = 0; i < 8; i++)
        {
            m1.load_a(piOrg);
            n1.load(piCur);
            sad.addSumAbsDiff(m1, n1);

            m1.load_a(piOrg + 16);
            n1.load(piCur + 16);
            sad.addSumAbsDiff(m1, n1);

            piOrg += strideOrg;
            piCur += strideCur;
        }

        sum += extend_low(sad) + extend_high(sad);
        sad = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(piOrg);
        n1.load(piCur);
        sad.addSumAbsDiff(m1, n1);

        m1.load_a(piOrg + 16);
        n1.load(piCur + 16);
        sad.addSumAbsDiff(m1, n1);

        piOrg += strideOrg;
        piCur += strideCur;
    }

    sum += extend_low(sad) + extend_high(sad);
    return horizontal_add(sum);
}

template<int ly>
int CDECL sad_48(pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    Vec16uc m1, n1;

    Vec4i sum(0);
    Vec8us sad(0);
    int main_iters = (ly >> 3) << 3;
    int row;

    for (row = 0; row < main_iters; row += 8)
    {
        for (int i = 0; i < 8; i++)
        {
            m1.load_a(piOrg);
            n1.load(piCur);
            sad.addSumAbsDiff(m1, n1);

            m1.load_a(piOrg + 16);
            n1.load(piCur + 16);
            sad.addSumAbsDiff(m1, n1);

            m1.load_a(piOrg + 32);
            n1.load(piCur + 32);
            sad.addSumAbsDiff(m1, n1);

            piOrg += strideOrg;
            piCur += strideCur;
        }

        sum += extend_low(sad) + extend_high(sad);
        sad = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(piOrg);
        n1.load(piCur);
        sad.addSumAbsDiff(m1, n1);

        m1.load_a(piOrg + 16);
        n1.load(piCur + 16);
        sad.addSumAbsDiff(m1, n1);

        m1.load_a(piOrg + 32);
        n1.load(piCur + 32);
        sad.addSumAbsDiff(m1, n1);

        piOrg += strideOrg;
        piCur += strideCur;
    }

    sum += extend_low(sad) + extend_high(sad);
    return horizontal_add(sum);
}

template<int ly>
int CDECL sad_64(pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    Vec16uc m1, n1;

    Vec4i sum(0);
    Vec8us sad(0);
    int main_iters = (ly >> 2) << 2;
    int row;

    for (row = 0; row < main_iters; row += 4)
    {
        for (int i = 0; i < 4; i++)
        {
            m1.load_a(piOrg);
            n1.load(piCur);
            sad.addSumAbsDiff(m1, n1);

            m1.load_a(piOrg + 16);
            n1.load(piCur + 16);
            sad.addSumAbsDiff(m1, n1);

            m1.load_a(piOrg + 32);
            n1.load(piCur + 32);
            sad.addSumAbsDiff(m1, n1);

            m1.load_a(piOrg + 48);
            n1.load(piCur + 48);
            sad.addSumAbsDiff(m1, n1);

            piOrg += strideOrg;
            piCur += strideCur;
        }

        sum += extend_low(sad) + extend_high(sad);
        sad = 0;
    }

    return horizontal_add(sum);
}

template<int ly>
void CDECL sad_x3_4(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, intptr_t strideCur, int *res)
{
    Vec16uc m1, n1, n2, n3;

    Vec4i sum1(0), sum2(0), sum3(0);
    Vec8us sad1(0), sad2(0), sad3(0);
    int main_iters = (ly >> 4) << 4;
    int row;

    for (row = 0; row < main_iters; row += 16)
    {
        for (int i = 0; i < 16; i++)
        {
            m1.fromUint32(*(uint32_t*)piOrg);
            n1.fromUint32(*(uint32_t*)piCur1);
            n2.fromUint32(*(uint32_t*)piCur2);
            n3.fromUint32(*(uint32_t*)piCur3);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            piOrg += FENC_STRIDE;
            piCur1 += strideCur;
            piCur2 += strideCur;
            piCur3 += strideCur;
        }

        sum1 += extend_low(sad1);
        sum2 += extend_low(sad2);
        sum3 += extend_low(sad3);
        sad1 = 0;
        sad2 = 0;
        sad3 = 0;
    }

    while (row++ < ly)
    {
        m1.fromUint32(*(uint32_t*)piOrg);
        n1.fromUint32(*(uint32_t*)piCur1);
        n2.fromUint32(*(uint32_t*)piCur2);
        n3.fromUint32(*(uint32_t*)piCur3);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);

        piOrg += FENC_STRIDE;
        piCur1 += strideCur;
        piCur2 += strideCur;
        piCur3 += strideCur;
    }

    sum1 += extend_low(sad1);
    sum2 += extend_low(sad2);
    sum3 += extend_low(sad3);

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
}

/* For performance - This function assumes that the *last load* can access 16 elements. */
template<int ly>
void CDECL sad_x3_8(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, intptr_t strideCur, int *res)
{
    Vec16uc m1, n1, n2, n3;

    Vec4i sum1(0), sum2(0), sum3(0);
    Vec8us sad1(0), sad2(0), sad3(0);
    int main_iters = (ly >> 4) << 4;
    int row;

    for (row = 0; row < main_iters; row += 16)
    {
        for (int i = 0; i < 16; i++)
        {
            m1.load_a(piOrg);
            n1.load(piCur1);
            n2.load(piCur2);
            n3.load(piCur3);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            piOrg += FENC_STRIDE;
            piCur1 += strideCur;
            piCur2 += strideCur;
            piCur3 += strideCur;
        }

        sum1 += extend_low(sad1);
        sum2 += extend_low(sad2);
        sum3 += extend_low(sad3);
        sad1 = 0;
        sad2 = 0;
        sad3 = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(piOrg);
        n1.load(piCur1);
        n2.load(piCur2);
        n3.load(piCur3);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);

        piOrg += FENC_STRIDE;
        piCur1 += strideCur;
        piCur2 += strideCur;
        piCur3 += strideCur;
    }

    sum1 += extend_low(sad1);
    sum2 += extend_low(sad2);
    sum3 += extend_low(sad3);

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
}

/* For performance - This function assumes that the *last load* can access 16 elements. */
template<int ly>
void CDECL sad_x3_12(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, intptr_t strideCur, int *res)
{
    Vec16uc m1, n1, n2, n3;

    Vec4i sum1(0), sum2(0), sum3(0);
    Vec8us sad1(0), sad2(0), sad3(0);
    int main_iters = (ly >> 4) << 4;
    int row;

    for (row = 0; row < main_iters; row += 16)
    {
        for (int i = 0; i < 16; i++)
        {
            m1.load_a(piOrg);
            m1.cutoff(12);
            n1.load(piCur1);
            n1.cutoff(12);
            n2.load(piCur2);
            n2.cutoff(12);
            n3.load(piCur3);
            n3.cutoff(12);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            piOrg += FENC_STRIDE;
            piCur1 += strideCur;
            piCur2 += strideCur;
            piCur3 += strideCur;
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
        m1.load_a(piOrg);
        m1.cutoff(12);
        n1.load(piCur1);
        n1.cutoff(12);
        n2.load(piCur2);
        n2.cutoff(12);
        n3.load(piCur3);
        n3.cutoff(12);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);

        piOrg += FENC_STRIDE;
        piCur1 += strideCur;
        piCur2 += strideCur;
        piCur3 += strideCur;
    }

    sum1 += extend_low(sad1) + extend_high(sad1);
    sum2 += extend_low(sad2) + extend_high(sad2);
    sum3 += extend_low(sad3) + extend_high(sad3);

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
}

template<int ly>
void CDECL sad_x3_16(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, intptr_t strideCur, int *res)
{
    Vec16uc m1, n1, n2, n3;

    Vec4i sum1(0), sum2(0), sum3(0);
    Vec8us sad1(0), sad2(0), sad3(0);
    int main_iters = (ly >> 4) << 4;
    int row;

    for (row = 0; row < main_iters; row += 16)
    {
        for (int i = 0; i < 16; i++)
        {
            m1.load_a(piOrg);
            n1.load(piCur1);
            n2.load(piCur2);
            n3.load(piCur3);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            piOrg += FENC_STRIDE;
            piCur1 += strideCur;
            piCur2 += strideCur;
            piCur3 += strideCur;
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
        m1.load_a(piOrg);
        n1.load(piCur1);
        n2.load(piCur2);
        n3.load(piCur3);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);

        piOrg += FENC_STRIDE;
        piCur1 += strideCur;
        piCur2 += strideCur;
        piCur3 += strideCur;
    }

    sum1 += extend_low(sad1) + extend_high(sad1);
    sum2 += extend_low(sad2) + extend_high(sad2);
    sum3 += extend_low(sad3) + extend_high(sad3);

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
}

template<int ly>
void CDECL sad_x3_24(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, intptr_t strideCur, int *res)
{
    Vec16uc m1, n1, n2, n3;

    Vec4i sum1(0), sum2(0), sum3(0);
    Vec8us sad1(0), sad2(0), sad3(0);
    int main_iters = (ly >> 4) << 4;
    int row;

    for (row = 0; row < main_iters; row += 16)
    {
        for (int i = 0; i < 16; i++)
        {
            m1.load_a(piOrg);
            n1.load(piCur1);
            n2.load(piCur2);
            n3.load(piCur3);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            m1.load_a(piOrg + 16);
            m1.cutoff(8);
            n1.load(piCur1 + 16);
            n1.cutoff(8);
            n2.load(piCur2 + 16);
            n2.cutoff(8);
            n3.load(piCur3 + 16);
            n3.cutoff(8);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            piOrg += FENC_STRIDE;
            piCur1 += strideCur;
            piCur2 += strideCur;
            piCur3 += strideCur;
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
        m1.load_a(piOrg);
        n1.load(piCur1);
        n2.load(piCur2);
        n3.load(piCur3);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);

        m1.load_a(piOrg + 16);
        m1.cutoff(8);
        n1.load(piCur1 + 16);
        n1.cutoff(8);
        n2.load(piCur2 + 16);
        n2.cutoff(8);
        n3.load(piCur3 + 16);
        n3.cutoff(8);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);

        piOrg += FENC_STRIDE;
        piCur1 += strideCur;
        piCur2 += strideCur;
        piCur3 += strideCur;
    }

    sum1 += extend_low(sad1) + extend_high(sad1);
    sum2 += extend_low(sad2) + extend_high(sad2);
    sum3 += extend_low(sad3) + extend_high(sad3);

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
}

template<int ly>
void CDECL sad_x3_32(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, intptr_t strideCur, int *res)
{
    Vec16uc m1, n1, n2, n3;

    Vec4i sum1(0), sum2(0), sum3(0);
    Vec8us sad1(0), sad2(0), sad3(0);
    int main_iters = (ly >> 3) << 3;
    int row;

    for (row = 0; row < main_iters; row += 8)
    {
        for (int i = 0; i < 8; i++)
        {
            m1.load_a(piOrg);
            n1.load(piCur1);
            n2.load(piCur2);
            n3.load(piCur3);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            m1.load_a(piOrg + 16);
            n1.load(piCur1 + 16);
            n2.load(piCur2 + 16);
            n3.load(piCur3 + 16);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            piOrg += FENC_STRIDE;
            piCur1 += strideCur;
            piCur2 += strideCur;
            piCur3 += strideCur;
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
        m1.load_a(piOrg);
        n1.load(piCur1);
        n2.load(piCur2);
        n3.load(piCur3);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);

        m1.load_a(piOrg + 16);
        n1.load(piCur1 + 16);
        n2.load(piCur2 + 16);
        n3.load(piCur3 + 16);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);

        piOrg += FENC_STRIDE;
        piCur1 += strideCur;
        piCur2 += strideCur;
        piCur3 += strideCur;
    }

    sum1 += extend_low(sad1) + extend_high(sad1);
    sum2 += extend_low(sad2) + extend_high(sad2);
    sum3 += extend_low(sad3) + extend_high(sad3);

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
}

template<int ly>
void CDECL sad_x3_48(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, intptr_t strideCur, int *res)
{
    Vec16uc m1, n1, n2, n3;

    Vec4i sum1(0), sum2(0), sum3(0);
    Vec8us sad1(0), sad2(0), sad3(0);
    int main_iters = (ly >> 3) << 3;
    int row;

    for (row = 0; row < main_iters; row += 8)
    {
        for (int i = 0; i < 8; i++)
        {
            m1.load_a(piOrg);
            n1.load(piCur1);
            n2.load(piCur2);
            n3.load(piCur3);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            m1.load_a(piOrg + 16);
            n1.load(piCur1 + 16);
            n2.load(piCur2 + 16);
            n3.load(piCur3 + 16);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            m1.load_a(piOrg + 32);
            n1.load(piCur1 + 32);
            n2.load(piCur2 + 32);
            n3.load(piCur3 + 32);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            piOrg += FENC_STRIDE;
            piCur1 += strideCur;
            piCur2 += strideCur;
            piCur3 += strideCur;
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
        m1.load_a(piOrg);
        n1.load(piCur1);
        n2.load(piCur2);
        n3.load(piCur3);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);

        m1.load_a(piOrg + 16);
        n1.load(piCur1 + 16);
        n2.load(piCur2 + 16);
        n3.load(piCur3 + 16);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);

        m1.load_a(piOrg + 32);
        n1.load(piCur1 + 32);
        n2.load(piCur2 + 32);
        n3.load(piCur3 + 32);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);

        piOrg += FENC_STRIDE;
        piCur1 += strideCur;
        piCur2 += strideCur;
        piCur3 += strideCur;
    }

    sum1 += extend_low(sad1) + extend_high(sad1);
    sum2 += extend_low(sad2) + extend_high(sad2);
    sum3 += extend_low(sad3) + extend_high(sad3);

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
}

template<int ly>
void CDECL sad_x3_64(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, intptr_t strideCur, int *res)
{
    Vec16uc m1, n1, n2, n3;

    Vec4i sum1(0), sum2(0), sum3(0);
    Vec8us sad1(0), sad2(0), sad3(0);
    int row;

    for (row = 0; row < ly; row += 4)
    {
        for (int i = 0; i < 4; i++)
        {
            m1.load_a(piOrg);
            n1.load(piCur1);
            n2.load(piCur2);
            n3.load(piCur3);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            m1.load_a(piOrg + 16);
            n1.load(piCur1 + 16);
            n2.load(piCur2 + 16);
            n3.load(piCur3 + 16);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            m1.load_a(piOrg + 32);
            n1.load(piCur1 + 32);
            n2.load(piCur2 + 32);
            n3.load(piCur3 + 32);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            m1.load_a(piOrg + 48);
            n1.load(piCur1 + 48);
            n2.load(piCur2 + 48);
            n3.load(piCur3 + 48);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            piOrg += FENC_STRIDE;
            piCur1 += strideCur;
            piCur2 += strideCur;
            piCur3 += strideCur;
        }

        sum1 += extend_low(sad1) + extend_high(sad1);
        sum2 += extend_low(sad2) + extend_high(sad2);
        sum3 += extend_low(sad3) + extend_high(sad3);
        sad1 = 0;
        sad2 = 0;
        sad3 = 0;
    }

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
}

template<int ly>
void CDECL sad_x4_4(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, intptr_t strideCur, int *res)
{
    Vec16uc m1, n1, n2, n3, n4;

    Vec4i sum1(0), sum2(0), sum3(0), sum4(0);
    Vec8us sad1(0), sad2(0), sad3(0), sad4(0);
    int main_iters = (ly >> 4) << 4;
    int row;

    for (row = 0; row < main_iters; row += 16)
    {
        for (int i = 0; i < 16; i++)
        {
            m1.fromUint32(*(uint32_t*)piOrg);
            n1.fromUint32(*(uint32_t*)piCur1);
            n2.fromUint32(*(uint32_t*)piCur2);
            n3.fromUint32(*(uint32_t*)piCur3);
            n4.fromUint32(*(uint32_t*)piCur4);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            piOrg += FENC_STRIDE;
            piCur1 += strideCur;
            piCur2 += strideCur;
            piCur3 += strideCur;
            piCur4 += strideCur;
        }

        sum1 += extend_low(sad1);
        sum2 += extend_low(sad2);
        sum3 += extend_low(sad3);
        sum4 += extend_low(sad4);
        sad1 = 0;
        sad2 = 0;
        sad3 = 0;
        sad4 = 0;
    }

    while (row++ < ly)
    {
        m1.fromUint32(*(uint32_t*)piOrg);
        n1.fromUint32(*(uint32_t*)piCur1);
        n2.fromUint32(*(uint32_t*)piCur2);
        n3.fromUint32(*(uint32_t*)piCur3);
        n4.fromUint32(*(uint32_t*)piCur4);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);
        sad4.addSumAbsDiff(m1, n4);

        piOrg += FENC_STRIDE;
        piCur1 += strideCur;
        piCur2 += strideCur;
        piCur3 += strideCur;
        piCur4 += strideCur;
    }

    sum1 += extend_low(sad1);
    sum2 += extend_low(sad2);
    sum3 += extend_low(sad3);
    sum4 += extend_low(sad4);

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
    res[3] = horizontal_add(sum4);
}

template<int ly>
void CDECL sad_x4_8(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, intptr_t strideCur, int *res)
{
    Vec16uc m1, n1, n2, n3, n4;

    Vec4i sum1(0), sum2(0), sum3(0), sum4(0);
    Vec8us sad1(0), sad2(0), sad3(0), sad4(0);
    int main_iters = (ly >> 4) << 4;
    int row;

    for (row = 0; row < main_iters; row += 16)
    {
        for (int i = 0; i < 16; i++)
        {
            m1.load_a(piOrg);
            n1.load(piCur1);
            n2.load(piCur2);
            n3.load(piCur3);
            n4.load(piCur4);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            piOrg += FENC_STRIDE;
            piCur1 += strideCur;
            piCur2 += strideCur;
            piCur3 += strideCur;
            piCur4 += strideCur;
        }

        sum1 += extend_low(sad1);
        sum2 += extend_low(sad2);
        sum3 += extend_low(sad3);
        sum4 += extend_low(sad4);
        sad1 = 0;
        sad2 = 0;
        sad3 = 0;
        sad4 = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(piOrg);
        n1.load(piCur1);
        n2.load(piCur2);
        n3.load(piCur3);
        n4.load(piCur4);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);
        sad4.addSumAbsDiff(m1, n4);

        piOrg += FENC_STRIDE;
        piCur1 += strideCur;
        piCur2 += strideCur;
        piCur3 += strideCur;
        piCur4 += strideCur;
    }

    sum1 += extend_low(sad1);
    sum2 += extend_low(sad2);
    sum3 += extend_low(sad3);
    sum4 += extend_low(sad4);

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
    res[3] = horizontal_add(sum4);
}

/* For performance - This function assumes that the *last load* can access 16 elements. */
template<int ly>
void CDECL sad_x4_12(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, intptr_t strideCur, int *res)
{
    Vec16uc m1, n1, n2, n3, n4;

    Vec4i sum1(0), sum2(0), sum3(0), sum4(0);
    Vec8us sad1(0), sad2(0), sad3(0), sad4(0);
    int main_iters = (ly >> 4) << 4;
    int row;

    for (row = 0; row < main_iters; row += 16)
    {
        for (int i = 0; i < 16; i++)
        {
            m1.load_a(piOrg);
            m1.cutoff(12);
            n1.load(piCur1);
            n1.cutoff(12);
            n2.load(piCur2);
            n2.cutoff(12);
            n3.load(piCur3);
            n3.cutoff(12);
            n4.load(piCur4);
            n4.cutoff(12);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            piOrg += FENC_STRIDE;
            piCur1 += strideCur;
            piCur2 += strideCur;
            piCur3 += strideCur;
            piCur4 += strideCur;
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
        m1.load_a(piOrg);
        m1.cutoff(12);
        n1.load(piCur1);
        n1.cutoff(12);
        n2.load(piCur2);
        n2.cutoff(12);
        n3.load(piCur3);
        n3.cutoff(12);
        n4.load(piCur4);
        n4.cutoff(12);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);
        sad4.addSumAbsDiff(m1, n4);

        piOrg += FENC_STRIDE;
        piCur1 += strideCur;
        piCur2 += strideCur;
        piCur3 += strideCur;
        piCur4 += strideCur;
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
void CDECL sad_x4_16(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, intptr_t strideCur, int *res)
{
    Vec16uc m1, n1, n2, n3, n4;

    Vec4i sum1(0), sum2(0), sum3(0), sum4(0);
    Vec8us sad1(0), sad2(0), sad3(0), sad4(0);
    int main_iters = (ly >> 4) << 4;
    int row;

    for (row = 0; row < main_iters; row += 16)
    {
        for (int i = 0; i < 16; i++)
        {
            m1.load_a(piOrg);
            n1.load(piCur1);
            n2.load(piCur2);
            n3.load(piCur3);
            n4.load(piCur4);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            piOrg += FENC_STRIDE;
            piCur1 += strideCur;
            piCur2 += strideCur;
            piCur3 += strideCur;
            piCur4 += strideCur;
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
        m1.load_a(piOrg);
        n1.load(piCur1);
        n2.load(piCur2);
        n3.load(piCur3);
        n4.load(piCur4);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);
        sad4.addSumAbsDiff(m1, n4);

        piOrg += FENC_STRIDE;
        piCur1 += strideCur;
        piCur2 += strideCur;
        piCur3 += strideCur;
        piCur4 += strideCur;
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
void CDECL sad_x4_24(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, intptr_t strideCur, int *res)
{
    Vec16uc m1, n1, n2, n3, n4;

    Vec4i sum1(0), sum2(0), sum3(0), sum4(0);
    Vec8us sad1(0), sad2(0), sad3(0), sad4(0);
    int main_iters = (ly >> 4) << 4;
    int row;

    for (row = 0; row < main_iters; row += 16)
    {
        for (int i = 0; i < 16; i++)
        {
            m1.load_a(piOrg);
            n1.load(piCur1);
            n2.load(piCur2);
            n3.load(piCur3);
            n4.load(piCur4);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            m1.load_a(piOrg + 16);
            m1.cutoff(8);
            n1.load(piCur1 + 16);
            n1.cutoff(8);
            n2.load(piCur2 + 16);
            n2.cutoff(8);
            n3.load(piCur3 + 16);
            n3.cutoff(8);
            n4.load(piCur4 + 16);
            n4.cutoff(8);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            piOrg += FENC_STRIDE;
            piCur1 += strideCur;
            piCur2 += strideCur;
            piCur3 += strideCur;
            piCur4 += strideCur;
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
        m1.load_a(piOrg);
        n1.load(piCur1);
        n2.load(piCur2);
        n3.load(piCur3);
        n4.load(piCur4);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);
        sad4.addSumAbsDiff(m1, n4);

        m1.load_a(piOrg + 16);
        m1.cutoff(8);
        n1.load(piCur1 + 16);
        n1.cutoff(8);
        n2.load(piCur2 + 16);
        n2.cutoff(8);
        n3.load(piCur3 + 16);
        n3.cutoff(8);
        n4.load(piCur4 + 16);
        n4.cutoff(8);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);
        sad4.addSumAbsDiff(m1, n4);

        piOrg += FENC_STRIDE;
        piCur1 += strideCur;
        piCur2 += strideCur;
        piCur3 += strideCur;
        piCur4 += strideCur;
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
void CDECL sad_x4_32(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, intptr_t strideCur, int *res)
{
    Vec16uc m1, n1, n2, n3, n4;

    Vec4i sum1(0), sum2(0), sum3(0), sum4(0);
    Vec8us sad1(0), sad2(0), sad3(0), sad4(0);
    int main_iters = (ly >> 3) << 3;
    int row;

    for (row = 0; row < main_iters; row += 8)
    {
        for (int i = 0; i < 8; i++)
        {
            m1.load_a(piOrg);
            n1.load(piCur1);
            n2.load(piCur2);
            n3.load(piCur3);
            n4.load(piCur4);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            m1.load_a(piOrg + 16);
            n1.load(piCur1 + 16);
            n2.load(piCur2 + 16);
            n3.load(piCur3 + 16);
            n4.load(piCur4 + 16);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            piOrg += FENC_STRIDE;
            piCur1 += strideCur;
            piCur2 += strideCur;
            piCur3 += strideCur;
            piCur4 += strideCur;
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
        m1.load_a(piOrg);
        n1.load(piCur1);
        n2.load(piCur2);
        n3.load(piCur3);
        n4.load(piCur4);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);
        sad4.addSumAbsDiff(m1, n4);

        m1.load_a(piOrg + 16);
        n1.load(piCur1 + 16);
        n2.load(piCur2 + 16);
        n3.load(piCur3 + 16);
        n4.load(piCur4 + 16);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);
        sad4.addSumAbsDiff(m1, n4);

        piOrg += FENC_STRIDE;
        piCur1 += strideCur;
        piCur2 += strideCur;
        piCur3 += strideCur;
        piCur4 += strideCur;
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
void CDECL sad_x4_48(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, intptr_t strideCur, int *res)
{
    Vec16uc m1, n1, n2, n3, n4;

    Vec4i sum1(0), sum2(0), sum3(0), sum4(0);
    Vec8us sad1(0), sad2(0), sad3(0), sad4(0);
    int main_iters = (ly >> 3) << 3;
    int row;

    for (row = 0; row < main_iters; row += 8)
    {
        for (int i = 0; i < 8; i++)
        {
            m1.load_a(piOrg);
            n1.load(piCur1);
            n2.load(piCur2);
            n3.load(piCur3);
            n4.load(piCur4);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            m1.load_a(piOrg + 16);
            n1.load(piCur1 + 16);
            n2.load(piCur2 + 16);
            n3.load(piCur3 + 16);
            n4.load(piCur4 + 16);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            m1.load_a(piOrg + 32);
            n1.load(piCur1 + 32);
            n2.load(piCur2 + 32);
            n3.load(piCur3 + 32);
            n4.load(piCur4 + 32);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            piOrg += FENC_STRIDE;
            piCur1 += strideCur;
            piCur2 += strideCur;
            piCur3 += strideCur;
            piCur4 += strideCur;
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
        m1.load_a(piOrg);
        n1.load(piCur1);
        n2.load(piCur2);
        n3.load(piCur3);
        n4.load(piCur4);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);
        sad4.addSumAbsDiff(m1, n4);

        m1.load_a(piOrg + 16);
        n1.load(piCur1 + 16);
        n2.load(piCur2 + 16);
        n3.load(piCur3 + 16);
        n4.load(piCur4 + 16);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);
        sad4.addSumAbsDiff(m1, n4);

        m1.load_a(piOrg + 32);
        n1.load(piCur1 + 32);
        n2.load(piCur2 + 32);
        n3.load(piCur3 + 32);
        n4.load(piCur4 + 32);

        sad1.addSumAbsDiff(m1, n1);
        sad2.addSumAbsDiff(m1, n2);
        sad3.addSumAbsDiff(m1, n3);
        sad4.addSumAbsDiff(m1, n4);

        piOrg += FENC_STRIDE;
        piCur1 += strideCur;
        piCur2 += strideCur;
        piCur3 += strideCur;
        piCur4 += strideCur;
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
void CDECL sad_x4_64(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, intptr_t strideCur, int *res)
{
    Vec16uc m1, n1, n2, n3, n4;

    Vec4i sum1(0), sum2(0), sum3(0), sum4(0);
    Vec8us sad1(0), sad2(0), sad3(0), sad4(0);
    int row;

    for (row = 0; row < ly; row += 4)
    {
        for (int i = 0; i < 4; i++)
        {
            m1.load_a(piOrg);
            n1.load(piCur1);
            n2.load(piCur2);
            n3.load(piCur3);
            n4.load(piCur4);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            m1.load_a(piOrg + 16);
            n1.load(piCur1 + 16);
            n2.load(piCur2 + 16);
            n3.load(piCur3 + 16);
            n4.load(piCur4 + 16);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            m1.load_a(piOrg + 32);
            n1.load(piCur1 + 32);
            n2.load(piCur2 + 32);
            n3.load(piCur3 + 32);
            n4.load(piCur4 + 32);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            m1.load_a(piOrg + 48);
            n1.load(piCur1 + 48);
            n2.load(piCur2 + 48);
            n3.load(piCur3 + 48);
            n4.load(piCur4 + 48);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);
            sad4.addSumAbsDiff(m1, n4);

            piOrg += FENC_STRIDE;
            piCur1 += strideCur;
            piCur2 += strideCur;
            piCur3 += strideCur;
            piCur4 += strideCur;
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

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
    res[3] = horizontal_add(sum4);
}

void CDECL getResidualIntra(pixel *piOrg, pixel *piPred, short *piResi, int height, int width, int stride)
{
        // get residual
        pixel*  pOrg    = piOrg;
        pixel*  pPred   = piPred;
        short* pResi  = piResi;
        
        Vec8s v_pOrg, v_pPred, v_pResilo, v_pResihi;
        Vec16uc v_temp1, v_temp2;
        if(width == 4)
        {
        for (int uiY = 0; uiY < height; uiY++)
        {
                v_temp1 = load_partial(const_int(4), pOrg);
                
                v_pOrg = extend_low(v_temp1);
                v_temp2 = load_partial(const_int(4), pPred);
                v_pPred = extend_low(v_temp2);

                v_pResilo = v_pOrg - v_pPred;
                store_partial(const_int(8), pResi, v_pResilo);

            pOrg  += stride;
            pResi += stride;
            pPred += stride;
        }
        }
        else if(width == 8)
        {
            for (int uiY = 0; uiY < height; uiY++)
            {
                //pResi[uiX] = static_cast<short>(pOrg[uiX]) - static_cast<short>(pPred[uiX]);
                v_temp1.load(pOrg);
                v_pOrg = extend_low(v_temp1);
                v_temp2.load(pPred);
                v_pPred = extend_low(v_temp2);

                v_pResilo = v_pOrg - v_pPred;
                v_pResilo.store(pResi);

                pOrg  += stride;
                pResi += stride;
                pPred += stride;
            }
            
        }
        else 
        {
        for (int uiY = 0; uiY < height; uiY++)
        {
            for (int uiX = 0; uiX < width; uiX+=16)
            {
                v_temp1.load(pOrg+uiX);
                v_pOrg = extend_low(v_temp1);
                v_temp2.load(pPred+uiX);
                v_pPred = extend_low(v_temp2);

                v_pResilo = v_pOrg - v_pPred;
                v_pResilo.store(pResi+uiX);

                v_pOrg = extend_high(v_temp1);
                v_pPred = extend_high(v_temp2);
                v_pResihi = v_pOrg - v_pPred;
                v_pResihi.store(pResi+uiX+8);

            }

            pOrg  += stride;
            pResi += stride;
            pPred += stride;
        }
        }

}


void CDECL calcRecons(pixel* piPred, short* piResi,pixel*  piReco, short* piRecQt, pixel* piRecIPred, int uiStride, int uiRecQtStride, int uiRecIPredStride, int uiHeight, int uiWidth)
    {
        pixel*   pPred      = piPred;
        short* pResi      = piResi;
        pixel*   pReco      = piReco;
        short* pRecQt     = piRecQt;
        pixel*   pRecIPred  = piRecIPred;
        //TODO : performance primitive?

        if(uiWidth == 4)
        {
            for (int uiY = 0; uiY < uiHeight; uiY++)
            {
                    Vec8s vresi, vpred, vres, vsum;
                    Vec16uc tmp;

                    tmp = load_partial(const_int(4), pPred );
                    vpred = extend_low(tmp);
                
                    vresi = load_partial(const_int(8), pResi);
                    vsum = vpred + vresi;
                
                    vsum = min(255, max(vsum, 0));

                    store_partial(const_int(8), pRecQt, vsum);

                    tmp = compress(vsum, vsum);

                    store_partial(const_int(4), pReco, tmp);
                    store_partial(const_int(4), pRecIPred, tmp);

                pPred     += uiStride;
                pResi     += uiStride;
                pReco     += uiStride;
                pRecQt    += uiRecQtStride;
                pRecIPred += uiRecIPredStride;
            }

        }
        else if(uiWidth == 8)
        {
            for (int uiY = 0; uiY < uiHeight; uiY++)
            {
                    Vec8s vresi, vpred, vres, vsum;
                    Vec16uc tmp;

                    tmp.load(pPred);
                    vpred = extend_low(tmp);
                
                    vresi.load(pResi);
                    vsum = vpred + vresi;
                
                    vsum = min(255, max(vsum, 0));

                    vsum.store(pRecQt);

                    tmp = compress(vsum, vsum);

                    store_partial(const_int(8), pReco, tmp);
                    store_partial(const_int(8), pRecIPred, tmp);

                pPred     += uiStride;
                pResi     += uiStride;
                pReco     += uiStride;
                pRecQt    += uiRecQtStride;
                pRecIPred += uiRecIPredStride;
            }

        }
        else
        {
            for (int uiY = 0; uiY < uiHeight; uiY++)
            {
                for (int uiX = 0; uiX < uiWidth; uiX+=16)
                {
                    Vec8s vresi, vpred, vres, vsum1, vsum2;
                    Vec16uc tmp;

                    tmp.load(pPred + uiX);

                    vpred = extend_low(tmp);
                    vresi.load(pResi+uiX);
                    vsum1 = vpred + vresi;
                    vsum1 = min(255, max(vsum1, 0));
                    vsum1.store(pRecQt + uiX);

                    vpred = extend_high(tmp);
                    vresi.load(pResi+uiX + 8);
                    vsum2 = vpred + vresi;
                    vsum2 = min(255, max(vsum2, 0));
                    vsum2.store(pRecQt + uiX + 8);

                    tmp = compress(vsum1, vsum2);
                    tmp.store(pReco + uiX);
                    tmp.store(pRecIPred + uiX);
                }

                pPred     += uiStride;
                pResi     += uiStride;
                pReco     += uiStride;
                pRecQt    += uiRecQtStride;
                pRecIPred += uiRecIPredStride;
            }
        }
    }

