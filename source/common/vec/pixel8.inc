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


template< int size >
__forceinline void unrollFunc_16(pixel *piOrg, intptr_t strideOrg, pixel *piCur, intptr_t strideCur, Vec8us *sad, int i)
{
    unrollFunc_16< 1 >(piOrg, strideOrg, piCur, strideCur, sad, i);
    unrollFunc_16< size - 1 >(piOrg, strideOrg, piCur, strideCur, sad, ++i);
}

template<>
__forceinline void unrollFunc_16< 1 >(pixel *piOrg, intptr_t strideOrg, pixel *piCur, intptr_t strideCur, Vec8us *sad, int i)
{
    Vec16uc m1, n1;
    pixel *piOrg1 = piOrg + strideOrg*i;
    pixel *piCur1 = piCur + strideCur*i;
    m1.load_a(piOrg1);
    n1.load(piCur1);
    sad[0].addSumAbsDiff(m1, n1);
} 

template<int ly>
int CDECL sad_16(pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    Vec16uc m1, n1;
    Vec4i sum(0);
    Vec8us sad(0);
    int main_iters = (ly >> 4) << 4;
    int row = 0;

    if(ly == 4)
    {
        unrollFunc_16< 4 >(piOrg, strideOrg, piCur, strideCur, &sad, 0);
        sum += extend_low(sad) + extend_high(sad);
        return horizontal_add(sum);
    }
    if(ly == 8)
    {
        unrollFunc_16< 8 >(piOrg, strideOrg, piCur, strideCur, &sad, 0);
        sum += extend_low(sad) + extend_high(sad);
        return horizontal_add(sum);
    }
    if(ly == 12)
    {
        unrollFunc_16< 12 >(piOrg, strideOrg, piCur, strideCur, &sad, 0);
        sum += extend_low(sad) + extend_high(sad);
        return horizontal_add(sum);
    }

    for (row = 0; row < main_iters; row += 16)
    {
        unrollFunc_16< 16 >(piOrg, strideOrg, piCur, strideCur, &sad, 0);
        sum += extend_low(sad) + extend_high(sad);  
        sad = 0;
        piOrg += strideOrg*16;
        piCur += strideCur*16;
    }
    if(ly==24)
    {
        unrollFunc_16< 8 >(piOrg, strideOrg, piCur, strideCur, &sad, 0);
        sum += extend_low(sad) + extend_high(sad);  
        return horizontal_add(sum);
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
