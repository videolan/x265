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
        for(int i = 0; i < 16; i++)
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
        for(int i = 0; i < 16; i++)
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
        for(int i = 0; i < 16; i++)
        {
            m1.load_a(piOrg); m1.cutoff(12);
            n1.load(piCur); n1.cutoff(12);
            sad.addSumAbsDiff(m1, n1);

            piOrg += strideOrg;
            piCur += strideCur;
        }

        sum += extend_low(sad) + extend_high(sad);
        sad = 0;
    }
    while (row++ < ly)
    {
        m1.load_a(piOrg); m1.cutoff(12);
        n1.load(piCur); n1.cutoff(12);
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
        for(int i = 0; i < 16; i++)
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
        for(int i = 0; i < 16; i++)
        {
            m1.load_a(piOrg);
            n1.load(piCur);
            sad.addSumAbsDiff(m1, n1);

            m1.load_a(piOrg + 16); m1.cutoff(8);
            n1.load(piCur + 16); n1.cutoff(8);
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

        m1.load_a(piOrg + 16); m1.cutoff(8);
        n1.load(piCur + 16); n1.cutoff(8);
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
        for(int i = 0; i < 8; i++)
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
        for(int i = 0; i < 8; i++)
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
        for(int i = 0; i < 4; i++)
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
void CDECL sad_4_x3(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, intptr_t strideCur, int *res)
{    
    Vec16uc m1, n1, n2, n3;
    Vec4i sum1(0), sum2(0), sum3(0);
    Vec8us sad1(0), sad2(0), sad3(0);
    int main_iters = (ly >> 4) << 4;
    int row;

    for (row = 0; row < main_iters; row += 16)
    {
        for(int i = 0; i < 16; i++)
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
        sad1 = 0; sad2 = 0; sad3 = 0;
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
void CDECL sad_8_x3(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, intptr_t strideCur, int *res)
{    
    Vec16uc m1, n1, n2, n3;
    Vec4i sum1(0), sum2(0), sum3(0);
    Vec8us sad1(0), sad2(0), sad3(0);
    int main_iters = (ly >> 4) << 4;
    int row;

    for (row = 0; row < main_iters; row += 16)
    {
        for(int i = 0; i < 16; i++)
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
        sad1 = 0; sad2 = 0; sad3 = 0;
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
void CDECL sad_12_x3(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, intptr_t strideCur, int *res)
{    
    Vec16uc m1, n1, n2, n3;
    Vec4i sum1(0), sum2(0), sum3(0);
    Vec8us sad1(0), sad2(0), sad3(0);
    int main_iters = (ly >> 4) << 4;
    int row;

    for (row = 0; row < main_iters; row += 16)
    {
        for(int i = 0; i < 16; i++)
        {
            m1.load_a(piOrg); m1.cutoff(12);
            n1.load(piCur1); n1.cutoff(12);
            n2.load(piCur2); n2.cutoff(12);
            n3.load(piCur3); n3.cutoff(12);

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
        sad1 = 0; sad2 = 0; sad3 = 0;
    }
    while (row++ < ly)
    {
        m1.load_a(piOrg); m1.cutoff(12);
        n1.load(piCur1); n1.cutoff(12);
        n2.load(piCur2); n2.cutoff(12);
        n3.load(piCur3); n3.cutoff(12);

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
void CDECL sad_16_x3(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, intptr_t strideCur, int *res)
{    
    Vec16uc m1, n1, n2, n3;
    Vec4i sum1(0), sum2(0), sum3(0);
    Vec8us sad1(0), sad2(0), sad3(0);
    int main_iters = (ly >> 4) << 4;
    int row;

    for (row = 0; row < main_iters; row += 16)
    {
        for(int i = 0; i < 16; i++)
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
        sad1 = 0; sad2 = 0; sad3 = 0;
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
void CDECL sad_24_x3(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, intptr_t strideCur, int *res)
{    
    Vec16uc m1, n1, n2, n3;
    Vec4i sum1(0), sum2(0), sum3(0);
    Vec8us sad1(0), sad2(0), sad3(0);
    int main_iters = (ly >> 4) << 4;
    int row;

    for (row = 0; row < main_iters; row += 16)
    {
        for(int i = 0; i < 16; i++)
        {
            m1.load_a(piOrg);
            n1.load(piCur1);
            n2.load(piCur2);
            n3.load(piCur3);

            sad1.addSumAbsDiff(m1, n1);
            sad2.addSumAbsDiff(m1, n2);
            sad3.addSumAbsDiff(m1, n3);

            m1.load_a(piOrg + 16); m1.cutoff(8);
            n1.load(piCur1 + 16); n1.cutoff(8);
            n2.load(piCur2 + 16); n2.cutoff(8);
            n3.load(piCur3 + 16); n3.cutoff(8);

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
        sad1 = 0; sad2 = 0; sad3 = 0;
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

        m1.load_a(piOrg + 16); m1.cutoff(8);
        n1.load(piCur1 + 16); n1.cutoff(8);
        n2.load(piCur2 + 16); n2.cutoff(8);
        n3.load(piCur3 + 16); n3.cutoff(8);

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
void CDECL sad_32_x3(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, intptr_t strideCur, int *res)
{    
    Vec16uc m1, n1, n2, n3;
    Vec4i sum1(0), sum2(0), sum3(0);
    Vec8us sad1(0), sad2(0), sad3(0);
    int main_iters = (ly >> 3) << 3;
    int row;

    for (row = 0; row < main_iters; row += 8)
    {
        for(int i = 0; i < 8; i++)
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
        sad1 = 0; sad2 = 0; sad3 = 0;
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
void CDECL sad_48_x3(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, intptr_t strideCur, int *res)
{    
    Vec16uc m1, n1, n2, n3;
    Vec4i sum1(0), sum2(0), sum3(0);
    Vec8us sad1(0), sad2(0), sad3(0);
    int main_iters = (ly >> 3) << 3;
    int row;

    for (row = 0; row < main_iters; row += 8)
    {
        for(int i = 0; i < 8; i++)
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
        sad1 = 0; sad2 = 0; sad3 = 0;
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
void CDECL sad_64_x3(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, intptr_t strideCur, int *res)
{    
    Vec16uc m1, n1, n2, n3;
    Vec4i sum1(0), sum2(0), sum3(0);
    Vec8us sad1(0), sad2(0), sad3(0);
    int row;

    for (row = 0; row < ly; row += 4)
    {
        for(int i = 0; i < 4; i++)
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
        sad1 = 0; sad2 = 0; sad3 = 0;
    }

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
}


template<int ly>
void CDECL sad_4_x4(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, intptr_t strideCur, int *res)
{    
    Vec16uc m1, n1, n2, n3, n4;
    Vec4i sum1(0), sum2(0), sum3(0), sum4(0);
    Vec8us sad1(0), sad2(0), sad3(0), sad4(0);
    int main_iters = (ly >> 4) << 4;
    int row;

    for (row = 0; row < main_iters; row += 16)
    {
        for(int i = 0; i < 16; i++)
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
        sad1 = 0; sad2 = 0; sad3 = 0; sad4 = 0;
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
void CDECL sad_8_x4(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, intptr_t strideCur, int *res)
{    
    Vec16uc m1, n1, n2, n3, n4;
    Vec4i sum1(0), sum2(0), sum3(0), sum4(0);
    Vec8us sad1(0), sad2(0), sad3(0), sad4(0);
    int main_iters = (ly >> 4) << 4;
    int row;

    for (row = 0; row < main_iters; row += 16)
    {
        for(int i = 0; i < 16; i++)
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
        sad1 = 0; sad2 = 0; sad3 = 0; sad4 = 0;
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
void CDECL sad_12_x4(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, intptr_t strideCur, int *res)
{    
    Vec16uc m1, n1, n2, n3, n4;
    Vec4i sum1(0), sum2(0), sum3(0), sum4(0);
    Vec8us sad1(0), sad2(0), sad3(0), sad4(0);
    int main_iters = (ly >> 4) << 4;
    int row;

    for (row = 0; row < main_iters; row += 16)
    {
        for(int i = 0; i < 16; i++)
        {
            m1.load_a(piOrg); m1.cutoff(12);
            n1.load(piCur1); n1.cutoff(12);
            n2.load(piCur2); n2.cutoff(12);
            n3.load(piCur3); n3.cutoff(12);
            n4.load(piCur4); n4.cutoff(12);

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
        sad1 = 0; sad2 = 0; sad3 = 0; sad4 = 0;
    }
    while (row++ < ly)
    {
        m1.load_a(piOrg); m1.cutoff(12);
        n1.load(piCur1); n1.cutoff(12);
        n2.load(piCur2); n2.cutoff(12);
        n3.load(piCur3); n3.cutoff(12);
        n4.load(piCur4); n4.cutoff(12);

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
void CDECL sad_16_x4(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, intptr_t strideCur, int *res)
{    
    Vec16uc m1, n1, n2, n3, n4;
    Vec4i sum1(0), sum2(0), sum3(0), sum4(0);
    Vec8us sad1(0), sad2(0), sad3(0), sad4(0);
    int main_iters = (ly >> 4) << 4;
    int row;

    for (row = 0; row < main_iters; row += 16)
    {
        for(int i = 0; i < 16; i++)
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
        sad1 = 0; sad2 = 0; sad3 = 0; sad4 = 0;
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
void CDECL sad_24_x4(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, intptr_t strideCur, int *res)
{
    Vec16uc m1, n1, n2, n3, n4;
    Vec4i sum1(0), sum2(0), sum3(0), sum4(0);
    Vec8us sad1(0), sad2(0), sad3(0), sad4(0);
    int main_iters = (ly >> 4) << 4;
    int row;

    for (row = 0; row < main_iters; row += 16)
    {
        for(int i = 0; i < 16; i++)
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

            m1.load_a(piOrg + 16); m1.cutoff(8);
            n1.load(piCur1 + 16); n1.cutoff(8);
            n2.load(piCur2 + 16); n2.cutoff(8);
            n3.load(piCur3 + 16); n3.cutoff(8);
            n4.load(piCur4 + 16); n4.cutoff(8);

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
        sad1 = 0; sad2 = 0; sad3 = 0; sad4 = 0;
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

        m1.load_a(piOrg + 16); m1.cutoff(8);
        n1.load(piCur1 + 16); n1.cutoff(8);
        n2.load(piCur2 + 16); n2.cutoff(8);
        n3.load(piCur3 + 16); n3.cutoff(8);
        n4.load(piCur4 + 16); n4.cutoff(8);

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
void CDECL sad_32_x4(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, intptr_t strideCur, int *res)
{    
    Vec16uc m1, n1, n2, n3, n4;
    Vec4i sum1(0), sum2(0), sum3(0), sum4(0);
    Vec8us sad1(0), sad2(0), sad3(0), sad4(0);
    int main_iters = (ly >> 3) << 3;
    int row;

    for (row = 0; row < main_iters; row += 8)
    {
        for(int i = 0; i < 8; i++)
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
        sad1 = 0; sad2 = 0; sad3 = 0; sad4 = 0;
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
void CDECL sad_48_x4(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, intptr_t strideCur, int *res)
{    
    Vec16uc m1, n1, n2, n3, n4;
    Vec4i sum1(0), sum2(0), sum3(0), sum4(0);
    Vec8us sad1(0), sad2(0), sad3(0), sad4(0);
    int main_iters = (ly >> 3) << 3;
    int row;

    for (row = 0; row < main_iters; row += 8)
    {
        for(int i = 0; i < 8; i++)
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
        sad1 = 0; sad2 = 0; sad3 = 0; sad4 = 0;
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
void CDECL sad_64_x4(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, intptr_t strideCur, int *res)
{    
    Vec16uc m1, n1, n2, n3, n4;
    Vec4i sum1(0), sum2(0), sum3(0), sum4(0);
    Vec8us sad1(0), sad2(0), sad3(0), sad4(0);
    int row;

    for (row = 0; row < ly; row += 4)
    {
        for(int i = 0; i < 4; i++)
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
        sad1 = 0; sad2 = 0; sad3 = 0; sad4 = 0;
    }

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
    res[3] = horizontal_add(sum4);
}

template<int ly>
int sse_4(pixel* piOrg, intptr_t iStrideOrg, pixel* piCur, intptr_t iStrideCur)
{
    int iRows = ly;
    Vec16uc m1, n1;
    Vec8us diff(0);
    Vec4i sum(0);
    for (; iRows != 0; iRows--)
    {
        m1.fromUint32(*(uint32_t*)piOrg);
        n1.fromUint32(*(uint32_t*)piCur);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;            
        sum += extend_low(diff);
        piOrg += iStrideOrg;
        piCur += iStrideCur;
    }
    return horizontal_add(sum);
}

template<int ly>
int sse_8(pixel* piOrg, intptr_t iStrideOrg, pixel* piCur, intptr_t iStrideCur)
{
    int iRows = ly;
    Vec16uc m1, n1;
    Vec8us diff(0);
    Vec4i sum(0);
    for (; iRows != 0; iRows--)
    {
        m1.load_a(piOrg);
        n1.load(piCur);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;            
        sum += (extend_low(diff) + (extend_high(diff)));
        piOrg += iStrideOrg;
        piCur += iStrideCur;
    }
    return horizontal_add(sum);
}

template<int ly>
int sse_16(pixel* piOrg, intptr_t iStrideOrg, pixel* piCur, intptr_t iStrideCur)
{
    int iRows = ly;
    Vec16uc m1, n1;
    Vec8us diff_low(0), diff_high(0);
    Vec4i sum_low(0), sum_high(0);
    for (; iRows != 0; iRows--)
    {
        m1.load_a(piOrg);
        n1.load(piCur);
        diff_low = extend_low(m1) - extend_low(n1);
        diff_high = extend_high(m1) - extend_high(n1);
        diff_low = diff_low * diff_low;            
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));
        piOrg += iStrideOrg;
        piCur += iStrideCur;
    }
    return (horizontal_add(sum_low) + horizontal_add(sum_high));
}

template<int ly>
int sse_32(pixel* piOrg, intptr_t iStrideOrg, pixel* piCur, intptr_t iStrideCur)
{
    int iRows = ly;
    Vec16uc m1, n1;
    Vec8us diff_low(0), diff_high(0);
    Vec4i sum_low(0), sum_high(0);
    for (; iRows != 0; iRows--)
    {
        m1.load_a(piOrg);
        n1.load(piCur);
        diff_low = extend_low(m1) - extend_low(n1);
        diff_high = extend_high(m1) - extend_high(n1);
        diff_low = diff_low * diff_low;            
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));

        m1.load_a(piOrg + 16);
        n1.load(piCur + 16);
        diff_low = extend_low(m1) - extend_low(n1);
        diff_high = extend_high(m1) - extend_high(n1);
        diff_low = diff_low * diff_low;            
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));
        piOrg += iStrideOrg;
        piCur += iStrideCur;
    }
    return (horizontal_add(sum_low) + horizontal_add(sum_high));
}

template<int ly>
int sse_48(pixel* piOrg, intptr_t iStrideOrg, pixel* piCur, intptr_t iStrideCur)
{
    int iRows = ly;
    Vec16uc m1, n1;
    Vec8us diff_low(0), diff_high(0);
    Vec4i sum_low(0), sum_high(0);
    for (; iRows != 0; iRows--)
    {
        m1.load_a(piOrg);
        n1.load(piCur);
        diff_low = extend_low(m1) - extend_low(n1);
        diff_high = extend_high(m1) - extend_high(n1);
        diff_low = diff_low * diff_low;            
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));

        m1.load_a(piOrg + 16);
        n1.load(piCur + 16);
        diff_low = extend_low(m1) - extend_low(n1);
        diff_high = extend_high(m1) - extend_high(n1);
        diff_low = diff_low * diff_low;            
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));

        m1.load_a(piOrg + 32);
        n1.load(piCur + 32);
        diff_low = extend_low(m1) - extend_low(n1);
        diff_high = extend_high(m1) - extend_high(n1);
        diff_low = diff_low * diff_low;            
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));
        piOrg += iStrideOrg;
        piCur += iStrideCur;
    }
    return (horizontal_add(sum_low) + horizontal_add(sum_high));
}

template<int ly>
int sse_64(pixel* piOrg, intptr_t iStrideOrg, pixel* piCur, intptr_t iStrideCur)
{
    int iRows = ly;
    Vec16uc m1, n1;
    Vec8us diff_low(0), diff_high(0);
    Vec4i sum_low(0), sum_high(0);
    for (; iRows != 0; iRows--)
    {
        m1.load_a(piOrg);
        n1.load(piCur);
        diff_low = extend_low(m1) - extend_low(n1);
        diff_high = extend_high(m1) - extend_high(n1);
        diff_low = diff_low * diff_low;            
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));

        m1.load_a(piOrg + 16);
        n1.load(piCur + 16);
        diff_low = extend_low(m1) - extend_low(n1);
        diff_high = extend_high(m1) - extend_high(n1);
        diff_low = diff_low * diff_low;            
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));

        m1.load_a(piOrg + 32);
        n1.load(piCur + 32);
        diff_low = extend_low(m1) - extend_low(n1);
        diff_high = extend_high(m1) - extend_high(n1);
        diff_low = diff_low * diff_low;            
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));

        m1.load_a(piOrg + 48);
        n1.load(piCur + 48);
        diff_low = extend_low(m1) - extend_low(n1);
        diff_high = extend_high(m1) - extend_high(n1);
        diff_low = diff_low * diff_low;            
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));
        piOrg += iStrideOrg;
        piCur += iStrideCur;
    }
    return (horizontal_add(sum_low) + horizontal_add(sum_high));
}

