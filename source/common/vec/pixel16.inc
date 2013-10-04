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

/* intrinsics for when pixel type is short */

template<int ly>
int sad_4(pixel * fenc, intptr_t fencstride, pixel * fref, intptr_t frefstride)
{
    Vec8s m1, n1;

    Vec4i sum(0);
    Vec8us sad(0);
    int max_iterators = (ly >> 4) << 4;
    int row;

    for (row = 0; row < max_iterators; row += 16)
    {
        for (int i = 0; i < 16; i++)
        {
            m1.load_a(fenc);
            n1.load(fref);
            sad += abs(m1 - n1);

            fenc += fencstride;
            fref += frefstride;
        }

        sum += extend_low(sad);
        sad = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(fenc);
        n1.load(fref);
        sad += abs(m1 - n1);

        fenc += fencstride;
        fref += frefstride;
    }

    sum += extend_low(sad);

    return horizontal_add(sum);
}

template<int ly>
int sad_8(pixel * fenc, intptr_t fencstride, pixel * fref, intptr_t frefstride)
{
    Vec8s m1, n1;

    Vec4i sum(0);
    Vec8us sad(0);
    int max_iterators = (ly >> 4) << 4;
    int row;

    for (row = 0; row < max_iterators; row += 16)
    {
        for (int i = 0; i < 16; i++)
        {
            m1.load_a(fenc);
            n1.load(fref);
            sad += abs(m1 - n1);

            fenc += fencstride;
            fref += frefstride;
        }

        sum += extend_low(sad) + extend_high(sad);
        sad = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(fenc);
        n1.load(fref);
        sad += abs(m1 - n1);

        fenc += fencstride;
        fref += frefstride;
    }

    sum += extend_low(sad) + extend_high(sad);

    return horizontal_add(sum);
}

template<int ly>
int sad_12(pixel * fenc, intptr_t fencstride, pixel * fref, intptr_t frefstride)
{
    Vec8s m1, n1;

    Vec4i sum(0);
    Vec8us sad(0);
    int max_iterators = (ly >> 4) << 4;
    int row;

    for (row = 0; row < max_iterators; row += 16)
    {
        for (int i = 0; i < 16; i++)
        {
            m1.load_a(fenc);
            n1.load(fref);
            sad += abs(m1 - n1);

            m1.load_a(fenc + 8);
            m1.cutoff(4);
            n1.load(fref + 8);
            n1.cutoff(4);
            sad += abs(m1 - n1);

            fenc += fencstride;
            fref += frefstride;
        }

        sum += extend_low(sad) + extend_high(sad);
        sad = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(fenc);
        n1.load(fref);
        sad += abs(m1 - n1);

        m1.load_a(fenc + 8);
        m1.cutoff(4);
        n1.load(fref + 8);
        n1.cutoff(4);
        sad += abs(m1 - n1);

        fenc += fencstride;
        fref += frefstride;
    }

    sum += extend_low(sad) + extend_high(sad);

    return horizontal_add(sum);
}

template<int ly>
int sad_16(pixel * fenc, intptr_t fencstride, pixel * fref, intptr_t frefstride)
{
    Vec8s m1, n1;

    Vec4i sum(0);
    Vec8us sad(0);
    int max_iterators = (ly >> 3) << 3;
    int row;

    for (row = 0; row < max_iterators; row += 8)
    {
        for (int i = 0; i < 8; i++)
        {
            m1.load_a(fenc);
            n1.load(fref);
            sad += abs(m1 - n1);

            m1.load_a(fenc + 8);
            n1.load(fref + 8);
            sad += abs(m1 - n1);

            fenc += fencstride;
            fref += frefstride;
        }

        sum += extend_low(sad) + extend_high(sad);
        sad = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(fenc);
        n1.load(fref);
        sad += abs(m1 - n1);

        m1.load_a(fenc + 8);
        n1.load(fref + 8);
        sad += abs(m1 - n1);

        fenc += fencstride;
        fref += frefstride;
    }

    sum += extend_low(sad) + extend_high(sad);

    return horizontal_add(sum);
}

template<int ly>
int sad_24(pixel *fenc, intptr_t fencstride, pixel *fref, intptr_t frefstride)
{
    Vec8s m1, n1;

    Vec4i sum(0);
    Vec8us sad(0);
    int max_iterators = (ly >> 3) << 3;
    int row;

    for (row = 0; row < max_iterators; row += 8)
    {
        for (int i = 0; i < 8; i++)
        {
            m1.load_a(fenc);
            n1.load(fref);
            sad += abs(m1 - n1);

            m1.load_a(fenc + 8);
            n1.load(fref + 8);
            sad += abs(m1 - n1);

            m1.load_a(fenc + 16);
            n1.load(fref + 16);
            sad += abs(m1 - n1);

            fenc += fencstride;
            fref += frefstride;
        }

        sum += extend_low(sad) + extend_high(sad);
        sad = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(fenc);
        n1.load(fref);
        sad += abs(m1 - n1);

        m1.load_a(fenc + 8);
        n1.load(fref + 8);
        sad += abs(m1 - n1);

        m1.load_a(fenc + 16);
        n1.load(fref + 16);
        sad += abs(m1 - n1);

        fenc += fencstride;
        fref += frefstride;
    }

    sum += extend_low(sad) + extend_high(sad);

    return horizontal_add(sum);
}

template<int ly>
int sad_32(pixel * fenc, intptr_t fencstride, pixel * fref, intptr_t frefstride)
{
    Vec8s m1, n1;

    Vec4i sum(0);
    Vec8us sad(0);
    int max_iterators = (ly >> 3) << 3;
    int row;

    for (row = 0; row < max_iterators; row += 8)
    {
        for (int i = 0; i < 8; i++)
        {
            m1.load_a(fenc);
            n1.load(fref);
            sad += abs(m1 - n1);

            m1.load_a(fenc + 8);
            n1.load(fref + 8);
            sad += abs(m1 - n1);

            m1.load_a(fenc + 16);
            n1.load(fref + 16);
            sad += abs(m1 - n1);

            m1.load_a(fenc + 24);
            n1.load(fref + 24);
            sad += abs(m1 - n1);

            fenc += fencstride;
            fref += frefstride;
        }

        sum += extend_low(sad) + extend_high(sad);
        sad = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(fenc);
        n1.load(fref);
        sad += abs(m1 - n1);

        m1.load_a(fenc + 8);
        n1.load(fref + 8);
        sad += abs(m1 - n1);

        m1.load_a(fenc + 16);
        n1.load(fref + 16);
        sad += abs(m1 - n1);

        m1.load_a(fenc + 24);
        n1.load(fref + 24);
        sad += abs(m1 - n1);

        fenc += fencstride;
        fref += frefstride;
    }

    sum += extend_low(sad) + extend_high(sad);

    return horizontal_add(sum);
}

template<int ly>
int sad_48(pixel * fenc, intptr_t fencstride, pixel * fref, intptr_t frefstride)
{
    Vec8s m1, n1;

    Vec4i sum(0);
    Vec8us sad(0);
    int max_iterators = (ly >> 3) << 3;
    int row;

    for (row = 0; row < max_iterators; row += 8)
    {
        for (int i = 0; i < 8; i++)
        {
            m1.load_a(fenc);
            n1.load(fref);
            sad += abs(m1 - n1);

            m1.load_a(fenc + 8);
            n1.load(fref + 8);
            sad += abs(m1 - n1);

            m1.load_a(fenc + 16);
            n1.load(fref + 16);
            sad += abs(m1 - n1);

            m1.load_a(fenc + 24);
            n1.load(fref + 24);
            sad += abs(m1 - n1);

            m1.load_a(fenc + 32);
            n1.load(fref + 32);
            sad += abs(m1 - n1);

            m1.load_a(fenc + 40);
            n1.load(fref + 40);
            sad += abs(m1 - n1);

            fenc += fencstride;
            fref += frefstride;
        }

        sum += extend_low(sad) + extend_high(sad);
        sad = 0;
    }

    while (row++ < ly)
    {
        m1.load_a(fenc);
        n1.load(fref);
        sad += abs(m1 - n1);

        m1.load_a(fenc + 8);
        n1.load(fref + 8);
        sad += abs(m1 - n1);

        m1.load_a(fenc + 16);
        n1.load(fref + 16);
        sad += abs(m1 - n1);

        m1.load_a(fenc + 24);
        n1.load(fref + 24);
        sad += abs(m1 - n1);

        m1.load_a(fenc + 32);
        n1.load(fref + 32);
        sad += abs(m1 - n1);

        m1.load_a(fenc + 40);
        n1.load(fref + 40);
        sad += abs(m1 - n1);

        fenc += fencstride;
        fref += frefstride;
    }

    sum += extend_low(sad) + extend_high(sad);

    return horizontal_add(sum);
}

template<int ly>
int sad_64(pixel * fenc, intptr_t fencstride, pixel * fref, intptr_t frefstride)
{
    Vec8s m1, n1;

    Vec4i sum(0);
    Vec8us sad(0);
    int row;

    for (row = 0; row < ly; row += 4)
    {
        for (int i = 0; i < 4; i++)
        {
            m1.load_a(fenc);
            n1.load(fref);
            sad += abs(m1 - n1);

            m1.load_a(fenc + 8);
            n1.load(fref + 8);
            sad += abs(m1 - n1);

            m1.load_a(fenc + 16);
            n1.load(fref + 16);
            sad += abs(m1 - n1);

            m1.load_a(fenc + 24);
            n1.load(fref + 24);
            sad += abs(m1 - n1);

            m1.load_a(fenc + 32);
            n1.load(fref + 32);
            sad += abs(m1 - n1);

            m1.load_a(fenc + 40);
            n1.load(fref + 40);
            sad += abs(m1 - n1);

            m1.load_a(fenc + 48);
            n1.load(fref + 48);
            sad += abs(m1 - n1);

            m1.load_a(fenc + 56);
            n1.load(fref + 56);
            sad += abs(m1 - n1);

            fenc += fencstride;
            fref += frefstride;
        }

        sum += extend_low(sad) + extend_high(sad);
        sad = 0;
    }

    return horizontal_add(sum);
}

template<int ly>
void sad_x3_4(pixel *fenc, pixel *Cur1, pixel *Cur2, pixel *Cur3, intptr_t frefstride, int *res)
{
    Vec8s m1, n1, n2, n3;

    Vec8us sad1(0), sad2(0), sad3(0);
    Vec4i sum1(0), sum2(0), sum3(0);
    int max_iterators = (ly >> 3) << 3;
    int row;

    for (row = 0; row < max_iterators; row += 8)
    {
        for (int i = 0; i < 8; i++)
        {
            m1.load_a(fenc);
            n1.load(Cur1);
            n2.load(Cur2);
            n3.load(Cur3);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);

            fenc += FENC_STRIDE;
            Cur1 += frefstride;
            Cur2 += frefstride;
            Cur3 += frefstride;
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
        m1.load_a(fenc);
        n1.load(Cur1);
        n2.load(Cur2);
        n3.load(Cur3);

        sad1 += abs(m1 - n1);
        sad2 += abs(m1 - n2);
        sad3 += abs(m1 - n3);

        fenc += FENC_STRIDE;
        Cur1 += frefstride;
        Cur2 += frefstride;
        Cur3 += frefstride;
    }

    sum1 += extend_low(sad1);
    sum2 += extend_low(sad2);
    sum3 += extend_low(sad3);

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
}

template<int ly>
void sad_x3_8(pixel *fenc, pixel *Cur1, pixel *Cur2, pixel *Cur3, intptr_t frefstride, int *res)
{
    Vec8s m1, n1, n2, n3;

    Vec8us sad1(0), sad2(0), sad3(0);
    Vec4i sum1(0), sum2(0), sum3(0);
    int max_iterators = (ly >> 3) << 3;
    int row;

    for (row = 0; row < max_iterators; row += 8)
    {
        for (int i = 0; i < 8; i++)
        {
            m1.load_a(fenc);
            n1.load(Cur1);
            n2.load(Cur2);
            n3.load(Cur3);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);

            fenc += FENC_STRIDE;
            Cur1 += frefstride;
            Cur2 += frefstride;
            Cur3 += frefstride;
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
        n1.load(Cur1);
        n2.load(Cur2);
        n3.load(Cur3);

        sad1 += abs(m1 - n1);
        sad2 += abs(m1 - n2);
        sad3 += abs(m1 - n3);

        fenc += FENC_STRIDE;
        Cur1 += frefstride;
        Cur2 += frefstride;
        Cur3 += frefstride;
    }

    sum1 += extend_low(sad1) + extend_high(sad1);
    sum2 += extend_low(sad2) + extend_high(sad2);
    sum3 += extend_low(sad3) + extend_high(sad3);

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
}

template<int ly>
void sad_x3_12(pixel *fenc, pixel *Cur1, pixel *Cur2, pixel *Cur3, intptr_t frefstride, int *res)
{
    Vec8s m1, n1, n2, n3;

    Vec8us sad1(0), sad2(0), sad3(0);
    Vec4i sum1(0), sum2(0), sum3(0);
    int max_iterators = (ly >> 3) << 3;
    int row;

    for (row = 0; row < max_iterators; row += 8)
    {
        for (int i = 0; i < 8; i++)
        {
            m1.load_a(fenc);
            n1.load(Cur1);
            n2.load(Cur2);
            n3.load(Cur3);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);

            m1.load_a(fenc + 8);
            m1.cutoff(4);
            n1.load(Cur1 + 8);
            n1.cutoff(4);
            n2.load(Cur2 + 8);
            n2.cutoff(4);
            n3.load(Cur3 + 8);
            n3.cutoff(4);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);

            fenc += FENC_STRIDE;
            Cur1 += frefstride;
            Cur2 += frefstride;
            Cur3 += frefstride;
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
        n1.load(Cur1);
        n2.load(Cur2);
        n3.load(Cur3);

        sad1 += abs(m1 - n1);
        sad2 += abs(m1 - n2);
        sad3 += abs(m1 - n3);

        m1.load_a(fenc + 8);
        m1.cutoff(4);
        n1.load(Cur1 + 8);
        n1.cutoff(4);
        n2.load(Cur2 + 8);
        n2.cutoff(4);
        n3.load(Cur3 + 8);
        n3.cutoff(4);

        sad1 += abs(m1 - n1);
        sad2 += abs(m1 - n2);
        sad3 += abs(m1 - n3);

        fenc += FENC_STRIDE;
        Cur1 += frefstride;
        Cur2 += frefstride;
        Cur3 += frefstride;
    }

    sum1 += extend_low(sad1) + extend_high(sad1);
    sum2 += extend_low(sad2) + extend_high(sad2);
    sum3 += extend_low(sad3) + extend_high(sad3);

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
}

template<int ly>
void sad_x3_16(pixel *fenc, pixel *Cur1, pixel *Cur2, pixel *Cur3, intptr_t frefstride, int *res)
{
    Vec8s m1, n1, n2, n3;

    Vec8us sad1(0), sad2(0), sad3(0);
    Vec4i sum1(0), sum2(0), sum3(0);
    int max_iterators = (ly >> 3) << 3;
    int row;

    for (row = 0; row < max_iterators; row += 8)
    {
        for (int i = 0; i < 8; i++)
        {
            m1.load_a(fenc);
            n1.load(Cur1);
            n2.load(Cur2);
            n3.load(Cur3);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);

            m1.load_a(fenc + 8);
            n1.load(Cur1 + 8);
            n2.load(Cur2 + 8);
            n3.load(Cur3 + 8);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);

            fenc += FENC_STRIDE;
            Cur1 += frefstride;
            Cur2 += frefstride;
            Cur3 += frefstride;
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
        n1.load(Cur1);
        n2.load(Cur2);
        n3.load(Cur3);

        sad1 += abs(m1 - n1);
        sad2 += abs(m1 - n2);
        sad3 += abs(m1 - n3);

        m1.load_a(fenc + 8);
        n1.load(Cur1 + 8);
        n2.load(Cur2 + 8);
        n3.load(Cur3 + 8);

        sad1 += abs(m1 - n1);
        sad2 += abs(m1 - n2);
        sad3 += abs(m1 - n3);

        fenc += FENC_STRIDE;
        Cur1 += frefstride;
        Cur2 += frefstride;
        Cur3 += frefstride;
    }

    sum1 += extend_low(sad1) + extend_high(sad1);
    sum2 += extend_low(sad2) + extend_high(sad2);
    sum3 += extend_low(sad3) + extend_high(sad3);

    res[0] = horizontal_add(sum1);
    res[1] = horizontal_add(sum2);
    res[2] = horizontal_add(sum3);
}

template<int ly>
void sad_x3_24(pixel *fenc, pixel *Cur1, pixel *Cur2, pixel *Cur3, intptr_t frefstride, int *res)
{
    Vec8s m1, n1, n2, n3;

    Vec8us sad1(0), sad2(0), sad3(0);
    Vec4i sum1(0), sum2(0), sum3(0);
    int row;

    for (row = 0; row < ly; row += 4)
    {
        for (int i = 0; i < 4; i++)
        {
            m1.load_a(fenc);
            n1.load(Cur1);
            n2.load(Cur2);
            n3.load(Cur3);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);

            m1.load_a(fenc + 8);
            n1.load(Cur1 + 8);
            n2.load(Cur2 + 8);
            n3.load(Cur3 + 8);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);

            m1.load_a(fenc + 16);
            n1.load(Cur1 + 16);
            n2.load(Cur2 + 16);
            n3.load(Cur3 + 16);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);

            fenc += FENC_STRIDE;
            Cur1 += frefstride;
            Cur2 += frefstride;
            Cur3 += frefstride;
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
void sad_x3_32(pixel *fenc, pixel *Cur1, pixel *Cur2, pixel *Cur3, intptr_t frefstride, int *res)
{
    Vec8s m1, n1, n2, n3;

    Vec8us sad1(0), sad2(0), sad3(0);
    Vec4i sum1(0), sum2(0), sum3(0);
    int row;

    for (row = 0; row < ly; row += 4)
    {
        for (int i = 0; i < 4; i++)
        {
            m1.load_a(fenc);
            n1.load(Cur1);
            n2.load(Cur2);
            n3.load(Cur3);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);

            m1.load_a(fenc + 8);
            n1.load(Cur1 + 8);
            n2.load(Cur2 + 8);
            n3.load(Cur3 + 8);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);

            m1.load_a(fenc + 16);
            n1.load(Cur1 + 16);
            n2.load(Cur2 + 16);
            n3.load(Cur3 + 16);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);

            m1.load_a(fenc + 24);
            n1.load(Cur1 + 24);
            n2.load(Cur2 + 24);
            n3.load(Cur3 + 24);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);

            fenc += FENC_STRIDE;
            Cur1 += frefstride;
            Cur2 += frefstride;
            Cur3 += frefstride;
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
void sad_x3_48(pixel *fenc, pixel *Cur1, pixel *Cur2, pixel *Cur3, intptr_t frefstride, int *res)
{
    Vec8s m1, n1, n2, n3;

    Vec8us sad1(0), sad2(0), sad3(0);
    Vec4i sum1(0), sum2(0), sum3(0);
    int row;

    for (row = 0; row < ly; row += 2)
    {
        for (int i = 0; i < 2; i++)
        {
            m1.load_a(fenc);
            n1.load(Cur1);
            n2.load(Cur2);
            n3.load(Cur3);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);

            m1.load_a(fenc + 8);
            n1.load(Cur1 + 8);
            n2.load(Cur2 + 8);
            n3.load(Cur3 + 8);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);

            m1.load_a(fenc + 16);
            n1.load(Cur1 + 16);
            n2.load(Cur2 + 16);
            n3.load(Cur3 + 16);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);

            m1.load_a(fenc + 24);
            n1.load(Cur1 + 24);
            n2.load(Cur2 + 24);
            n3.load(Cur3 + 24);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);

            m1.load_a(fenc + 32);
            n1.load(Cur1 + 32);
            n2.load(Cur2 + 32);
            n3.load(Cur3 + 32);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);

            m1.load_a(fenc + 40);
            n1.load(Cur1 + 40);
            n2.load(Cur2 + 40);
            n3.load(Cur3 + 40);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);

            fenc += FENC_STRIDE;
            Cur1 += frefstride;
            Cur2 += frefstride;
            Cur3 += frefstride;
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
void sad_x3_64(pixel *fenc, pixel *Cur1, pixel *Cur2, pixel *Cur3, intptr_t frefstride, int *res)
{
    Vec8s m1, n1, n2, n3;

    Vec8us sad1(0), sad2(0), sad3(0);
    Vec4i sum1(0), sum2(0), sum3(0);
    int row;

    for (row = 0; row < ly; row += 2)
    {
        for (int i = 0; i < 2; i++)
        {
            m1.load_a(fenc);
            n1.load(Cur1);
            n2.load(Cur2);
            n3.load(Cur3);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);

            m1.load_a(fenc + 8);
            n1.load(Cur1 + 8);
            n2.load(Cur2 + 8);
            n3.load(Cur3 + 8);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);

            m1.load_a(fenc + 16);
            n1.load(Cur1 + 16);
            n2.load(Cur2 + 16);
            n3.load(Cur3 + 16);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);

            m1.load_a(fenc + 24);
            n1.load(Cur1 + 24);
            n2.load(Cur2 + 24);
            n3.load(Cur3 + 24);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);

            m1.load_a(fenc + 32);
            n1.load(Cur1 + 32);
            n2.load(Cur2 + 32);
            n3.load(Cur3 + 32);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);

            m1.load_a(fenc + 40);
            n1.load(Cur1 + 40);
            n2.load(Cur2 + 40);
            n3.load(Cur3 + 40);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);

            m1.load_a(fenc + 48);
            n1.load(Cur1 + 48);
            n2.load(Cur2 + 48);
            n3.load(Cur3 + 48);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);

            m1.load_a(fenc + 56);
            n1.load(Cur1 + 56);
            n2.load(Cur2 + 56);
            n3.load(Cur3 + 56);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);

            fenc += FENC_STRIDE;
            Cur1 += frefstride;
            Cur2 += frefstride;
            Cur3 += frefstride;
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
void sad_x4_4(pixel *fenc, pixel *Cur1, pixel *Cur2, pixel *Cur3, pixel *Cur4, intptr_t frefstride, int *res)
{
    Vec8s m1, n1, n2, n3, n4;

    Vec8us sad1(0), sad2(0), sad3(0), sad4(0);
    Vec4i sum1(0), sum2(0), sum3(0), sum4(0);
    int max_iterators = (ly >> 3) << 3;
    int row;

    for (row = 0; row < max_iterators; row += 8)
    {
        for (int i = 0; i < 8; i++)
        {
            m1.load_a(fenc);
            n1.load(Cur1);
            n2.load(Cur2);
            n3.load(Cur3);
            n4.load(Cur4);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);
            sad4 += abs(m1 - n4);

            fenc += FENC_STRIDE;
            Cur1 += frefstride;
            Cur2 += frefstride;
            Cur3 += frefstride;
            Cur4 += frefstride;
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
        m1.load_a(fenc);
        n1.load(Cur1);
        n2.load(Cur2);
        n3.load(Cur3);
        n4.load(Cur4);

        sad1 += abs(m1 - n1);
        sad2 += abs(m1 - n2);
        sad3 += abs(m1 - n3);
        sad4 += abs(m1 - n4);

        fenc += FENC_STRIDE;
        Cur1 += frefstride;
        Cur2 += frefstride;
        Cur3 += frefstride;
        Cur4 += frefstride;
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
void sad_x4_8(pixel *fenc, pixel *Cur1, pixel *Cur2, pixel *Cur3, pixel *Cur4, intptr_t frefstride, int *res)
{
    Vec8s m1, n1, n2, n3, n4;

    Vec8us sad1(0), sad2(0), sad3(0), sad4(0);
    Vec4i sum1(0), sum2(0), sum3(0), sum4(0);
    int max_iterators = (ly >> 3) << 3;
    int row;

    for (row = 0; row < max_iterators; row += 8)
    {
        for (int i = 0; i < 8; i++)
        {
            m1.load_a(fenc);
            n1.load(Cur1);
            n2.load(Cur2);
            n3.load(Cur3);
            n4.load(Cur4);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);
            sad4 += abs(m1 - n4);

            fenc += FENC_STRIDE;
            Cur1 += frefstride;
            Cur2 += frefstride;
            Cur3 += frefstride;
            Cur4 += frefstride;
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
        n1.load(Cur1);
        n2.load(Cur2);
        n3.load(Cur3);
        n4.load(Cur4);

        sad1 += abs(m1 - n1);
        sad2 += abs(m1 - n2);
        sad3 += abs(m1 - n3);
        sad4 += abs(m1 - n4);

        fenc += FENC_STRIDE;
        Cur1 += frefstride;
        Cur2 += frefstride;
        Cur3 += frefstride;
        Cur4 += frefstride;
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
void sad_x4_12(pixel *fenc, pixel *Cur1, pixel *Cur2, pixel *Cur3, pixel *Cur4, intptr_t frefstride, int *res)
{
    Vec8s m1, n1, n2, n3, n4;

    Vec8us sad1(0), sad2(0), sad3(0), sad4(0);
    Vec4i sum1(0), sum2(0), sum3(0), sum4(0);
    int max_iterators = (ly >> 3) << 3;
    int row;

    for (row = 0; row < max_iterators; row += 8)
    {
        for (int i = 0; i < 8; i++)
        {
            m1.load_a(fenc);
            n1.load(Cur1);
            n2.load(Cur2);
            n3.load(Cur3);
            n4.load(Cur4);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);
            sad4 += abs(m1 - n4);

            m1.load_a(fenc + 8);
            m1.cutoff(4);
            n1.load(Cur1 + 8);
            n1.cutoff(4);
            n2.load(Cur2 + 8);
            n2.cutoff(4);
            n3.load(Cur3 + 8);
            n3.cutoff(4);
            n4.load(Cur4 + 8);
            n4.cutoff(4);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);
            sad4 += abs(m1 - n4);

            fenc += FENC_STRIDE;
            Cur1 += frefstride;
            Cur2 += frefstride;
            Cur3 += frefstride;
            Cur4 += frefstride;
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
        n1.load(Cur1);
        n2.load(Cur2);
        n3.load(Cur3);
        n4.load(Cur4);

        sad1 += abs(m1 - n1);
        sad2 += abs(m1 - n2);
        sad3 += abs(m1 - n3);
        sad4 += abs(m1 - n4);

        m1.load_a(fenc + 8);
        m1.cutoff(4);
        n1.load(Cur1 + 8);
        n1.cutoff(4);
        n2.load(Cur2 + 8);
        n2.cutoff(4);
        n3.load(Cur3 + 8);
        n3.cutoff(4);
        n4.load(Cur4 + 8);
        n4.cutoff(4);

        sad1 += abs(m1 - n1);
        sad2 += abs(m1 - n2);
        sad3 += abs(m1 - n3);
        sad4 += abs(m1 - n4);

        fenc += FENC_STRIDE;
        Cur1 += frefstride;
        Cur2 += frefstride;
        Cur3 += frefstride;
        Cur4 += frefstride;
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
void sad_x4_16(pixel *fenc, pixel *Cur1, pixel *Cur2, pixel *Cur3, pixel *Cur4, intptr_t frefstride, int *res)
{
    Vec8s m1, n1, n2, n3, n4;

    Vec8us sad1(0), sad2(0), sad3(0), sad4(0);
    Vec4i sum1(0), sum2(0), sum3(0), sum4(0);
    int max_iterators = (ly >> 3) << 3;
    int row;

    for (row = 0; row < max_iterators; row += 8)
    {
        for (int i = 0; i < 8; i++)
        {
            m1.load_a(fenc);
            n1.load(Cur1);
            n2.load(Cur2);
            n3.load(Cur3);
            n4.load(Cur4);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);
            sad4 += abs(m1 - n4);

            m1.load_a(fenc + 8);
            n1.load(Cur1 + 8);
            n2.load(Cur2 + 8);
            n3.load(Cur3 + 8);
            n4.load(Cur4 + 8);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);
            sad4 += abs(m1 - n4);

            fenc += FENC_STRIDE;
            Cur1 += frefstride;
            Cur2 += frefstride;
            Cur3 += frefstride;
            Cur4 += frefstride;
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
        n1.load(Cur1);
        n2.load(Cur2);
        n3.load(Cur3);
        n4.load(Cur4);

        sad1 += abs(m1 - n1);
        sad2 += abs(m1 - n2);
        sad3 += abs(m1 - n3);
        sad4 += abs(m1 - n4);

        m1.load_a(fenc + 8);
        n1.load(Cur1 + 8);
        n2.load(Cur2 + 8);
        n3.load(Cur3 + 8);
        n4.load(Cur4 + 8);

        sad1 += abs(m1 - n1);
        sad2 += abs(m1 - n2);
        sad3 += abs(m1 - n3);
        sad4 += abs(m1 - n4);

        fenc += FENC_STRIDE;
        Cur1 += frefstride;
        Cur2 += frefstride;
        Cur3 += frefstride;
        Cur4 += frefstride;
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
void sad_x4_24(pixel *fenc, pixel *Cur1, pixel *Cur2, pixel *Cur3, pixel *Cur4, intptr_t frefstride, int *res)
{
    Vec8s m1, n1, n2, n3, n4;

    Vec8us sad1(0), sad2(0), sad3(0), sad4(0);
    Vec4i sum1(0), sum2(0), sum3(0), sum4(0);
    int row;

    for (row = 0; row < ly; row += 4)
    {
        for (int i = 0; i < 4; i++)
        {
            m1.load_a(fenc);
            n1.load(Cur1);
            n2.load(Cur2);
            n3.load(Cur3);
            n4.load(Cur4);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);
            sad4 += abs(m1 - n4);

            m1.load_a(fenc + 8);
            n1.load(Cur1 + 8);
            n2.load(Cur2 + 8);
            n3.load(Cur3 + 8);
            n4.load(Cur4 + 8);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);
            sad4 += abs(m1 - n4);

            m1.load_a(fenc + 16);
            n1.load(Cur1 + 16);
            n2.load(Cur2 + 16);
            n3.load(Cur3 + 16);
            n4.load(Cur4 + 16);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);
            sad4 += abs(m1 - n4);

            fenc += FENC_STRIDE;
            Cur1 += frefstride;
            Cur2 += frefstride;
            Cur3 += frefstride;
            Cur4 += frefstride;
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

template<int ly>
void sad_x4_32(pixel *fenc, pixel *Cur1, pixel *Cur2, pixel *Cur3, pixel *Cur4, intptr_t frefstride, int *res)
{
    Vec8s m1, n1, n2, n3, n4;

    Vec8us sad1(0), sad2(0), sad3(0), sad4(0);
    Vec4i sum1(0), sum2(0), sum3(0), sum4(0);
    int row;

    for (row = 0; row < ly; row += 4)
    {
        for (int i = 0; i < 4; i++)
        {
            m1.load_a(fenc);
            n1.load(Cur1);
            n2.load(Cur2);
            n3.load(Cur3);
            n4.load(Cur4);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);
            sad4 += abs(m1 - n4);

            m1.load_a(fenc + 8);
            n1.load(Cur1 + 8);
            n2.load(Cur2 + 8);
            n3.load(Cur3 + 8);
            n4.load(Cur4 + 8);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);
            sad4 += abs(m1 - n4);

            m1.load_a(fenc + 16);
            n1.load(Cur1 + 16);
            n2.load(Cur2 + 16);
            n3.load(Cur3 + 16);
            n4.load(Cur4 + 16);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);
            sad4 += abs(m1 - n4);

            m1.load_a(fenc + 24);
            n1.load(Cur1 + 24);
            n2.load(Cur2 + 24);
            n3.load(Cur3 + 24);
            n4.load(Cur4 + 24);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);
            sad4 += abs(m1 - n4);

            fenc += FENC_STRIDE;
            Cur1 += frefstride;
            Cur2 += frefstride;
            Cur3 += frefstride;
            Cur4 += frefstride;
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

template<int ly>
void sad_x4_48(pixel *fenc, pixel *Cur1, pixel *Cur2, pixel *Cur3, pixel *Cur4, intptr_t frefstride, int *res)
{
    Vec8s m1, n1, n2, n3, n4;

    Vec8us sad1(0), sad2(0), sad3(0), sad4(0);
    Vec4i sum1(0), sum2(0), sum3(0), sum4(0);
    int row;

    for (row = 0; row < ly; row += 2)
    {
        for (int i = 0; i < 2; i++)
        {
            m1.load_a(fenc);
            n1.load(Cur1);
            n2.load(Cur2);
            n3.load(Cur3);
            n4.load(Cur4);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);
            sad4 += abs(m1 - n4);

            m1.load_a(fenc + 8);
            n1.load(Cur1 + 8);
            n2.load(Cur2 + 8);
            n3.load(Cur3 + 8);
            n4.load(Cur4 + 8);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);
            sad4 += abs(m1 - n4);

            m1.load_a(fenc + 16);
            n1.load(Cur1 + 16);
            n2.load(Cur2 + 16);
            n3.load(Cur3 + 16);
            n4.load(Cur4 + 16);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);
            sad4 += abs(m1 - n4);

            m1.load_a(fenc + 24);
            n1.load(Cur1 + 24);
            n2.load(Cur2 + 24);
            n3.load(Cur3 + 24);
            n4.load(Cur4 + 24);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);
            sad4 += abs(m1 - n4);

            m1.load_a(fenc + 32);
            n1.load(Cur1 + 32);
            n2.load(Cur2 + 32);
            n3.load(Cur3 + 32);
            n4.load(Cur4 + 32);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);
            sad4 += abs(m1 - n4);

            m1.load_a(fenc + 40);
            n1.load(Cur1 + 40);
            n2.load(Cur2 + 40);
            n3.load(Cur3 + 40);
            n4.load(Cur4 + 40);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);
            sad4 += abs(m1 - n4);

            fenc += FENC_STRIDE;
            Cur1 += frefstride;
            Cur2 += frefstride;
            Cur3 += frefstride;
            Cur4 += frefstride;
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

template<int ly>
void sad_x4_64(pixel *fenc, pixel *Cur1, pixel *Cur2, pixel *Cur3, pixel *Cur4, intptr_t frefstride, int *res)
{
    Vec8s m1, n1, n2, n3, n4;

    Vec8us sad1(0), sad2(0), sad3(0), sad4(0);
    Vec4i sum1(0), sum2(0), sum3(0), sum4(0);
    int row;

    for (row = 0; row < ly; row += 2)
    {
        for (int i = 0; i < 2; i++)
        {
            m1.load_a(fenc);
            n1.load(Cur1);
            n2.load(Cur2);
            n3.load(Cur3);
            n4.load(Cur4);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);
            sad4 += abs(m1 - n4);

            m1.load_a(fenc + 8);
            n1.load(Cur1 + 8);
            n2.load(Cur2 + 8);
            n3.load(Cur3 + 8);
            n4.load(Cur4 + 8);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);
            sad4 += abs(m1 - n4);

            m1.load_a(fenc + 16);
            n1.load(Cur1 + 16);
            n2.load(Cur2 + 16);
            n3.load(Cur3 + 16);
            n4.load(Cur4 + 16);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);
            sad4 += abs(m1 - n4);

            m1.load_a(fenc + 24);
            n1.load(Cur1 + 24);
            n2.load(Cur2 + 24);
            n3.load(Cur3 + 24);
            n4.load(Cur4 + 24);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);
            sad4 += abs(m1 - n4);

            m1.load_a(fenc + 32);
            n1.load(Cur1 + 32);
            n2.load(Cur2 + 32);
            n3.load(Cur3 + 32);
            n4.load(Cur4 + 32);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);
            sad4 += abs(m1 - n4);

            m1.load_a(fenc + 40);
            n1.load(Cur1 + 40);
            n2.load(Cur2 + 40);
            n3.load(Cur3 + 40);
            n4.load(Cur4 + 40);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);
            sad4 += abs(m1 - n4);

            m1.load_a(fenc + 48);
            n1.load(Cur1 + 48);
            n2.load(Cur2 + 48);
            n3.load(Cur3 + 48);
            n4.load(Cur4 + 48);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);
            sad4 += abs(m1 - n4);

            m1.load_a(fenc + 56);
            n1.load(Cur1 + 56);
            n2.load(Cur2 + 56);
            n3.load(Cur3 + 56);
            n4.load(Cur4 + 56);

            sad1 += abs(m1 - n1);
            sad2 += abs(m1 - n2);
            sad3 += abs(m1 - n3);
            sad4 += abs(m1 - n4);

            fenc += FENC_STRIDE;
            Cur1 += frefstride;
            Cur2 += frefstride;
            Cur3 += frefstride;
            Cur4 += frefstride;
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
