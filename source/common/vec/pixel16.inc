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

int satd_4x4(pixel * fenc, intptr_t istrideOrg, pixel * fref, intptr_t istrideCur)
{
    int satd = 0;

    Vec8s v1, v2, m1, m2;

    {
        Vec8s temp1, temp2, temp3, temp4, Org_v, Cur_v;
        temp1.load(fenc);
        temp2.load(fref);
        fref += istrideCur;
        fenc += istrideOrg;

        temp3.load(fenc);
        temp4.load(fref);
        fref += istrideCur;
        fenc += istrideOrg;

        Org_v = blend2q<0, 2>((Vec2q)temp1, (Vec2q)temp3);
        Cur_v = blend2q<0, 2>((Vec2q)temp2, (Vec2q)temp4);

        temp1.load(fenc);
        temp2.load(fref);
        fref += istrideCur;
        fenc += istrideOrg;

        temp3.load(fenc);
        temp4.load(fref);
        fref += istrideCur;
        fenc += istrideOrg;

        v1 = Org_v - Cur_v; //diff

        Org_v = blend2q<0, 2>((Vec2q)temp3, (Vec2q)temp1);
        Cur_v = blend2q<0, 2>((Vec2q)temp4, (Vec2q)temp2);
        v2 = Org_v - Cur_v; //diff
    }

    for (int i = 0; i < 2; i++)
    {
        m1 = v1 + v2;
        m2 = v1 - v2;

        v1 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>(m1, m2);
        v2 = blend8s<4, 12, 5, 13, 6, 14, 7, 15>(m1, m2);
    }

    v2 = permute2q<1, 0>((Vec2q)v2);

    m1 = v1 + v2;
    m2 = v1 - v2;

    v1 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>(m1, m2);
    v2 = blend8s<4, 12, 5, 13, 6, 14, 7, 15>(m1, m2);

    m1 = v1 + v2;
    m2 = v1 - v2;

    v1 = abs(m1);
    v2 = abs(m2);
    v1 = v1 + v2;
    satd = horizontal_add_x(v1);

    satd = ((satd + 1) >> 1);

    return satd;
}

int sa8d_8x8(pixel * fenc, intptr_t istrideOrg, pixel * fref, intptr_t istrideCur)
{
    ALIGN_VAR_16(short, m2[8][8]);

    Vec8s diff_v1, diff_v2, Org_v1, Org_v2, Cur_v1, Cur_v2;
    Vec8s v1, v2, t1, t2;

    int  j, satd = 0;

    for (j = 0; j < 8; j += 2)
    {
        Org_v1.load_a(fenc);
        Cur_v1.load(fref);
        fref += istrideCur;
        fenc += istrideOrg;

        Org_v2.load_a(fenc);
        Cur_v2.load(fref);
        fref += istrideCur;
        fenc += istrideOrg;

        diff_v1 = Org_v1 - Cur_v1;
        diff_v2 = Org_v2 - Cur_v2;

        v1 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>(diff_v1, diff_v2);
        v2 = blend8s<4, 12, 5, 13, 6, 14, 7, 15>(diff_v1, diff_v2);

        t1 = v1 + v2; //m2
        t2 = v1 - v2;

        v1 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>(t1, t2);
        v2 = blend8s<4, 12, 5, 13, 6, 14, 7, 15>(t1, t2);

        t1 = v1 + v2; //m1
        t2 = v1 - v2;

        v1 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>(t1, t2);
        v2 = blend8s<4, 12, 5, 13, 6, 14, 7, 15>(t1, t2);

        t1 = v1 + v2; //m2
        t2 = v1 - v2;

        v1 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>(t1, t2);   //m2[j][0...7]
        v2 = blend8s<4, 12, 5, 13, 6, 14, 7, 15>(t1, t2); //m2[j+1][0..7]

        v1.store_a(m2[j]);
        v2.store_a(m2[j + 1]);
    }

    //vertical
    {
        Vec8s v0, v3, v4, v5, v6, v7;

        v0.load_a(m2[0]);
        v4.load_a(m2[4]);
        t1 = v0 + v4;
        t2 = v0 - v4;
        v0 = t1;
        v4 = t2;

        v1.load_a(m2[1]);
        v5.load_a(m2[5]);
        t1 = v1 + v5;
        t2 = v1 - v5;
        v1 = t1;
        v5 = t2;

        v2.load_a(m2[2]);
        v6.load_a(m2[6]);
        t1 = v2 + v6;
        t2 = v2 - v6;
        v2 = t1;
        v6 = t2;

        v3.load_a(m2[3]);
        v7.load_a(m2[7]);
        t1 = v3 + v7;
        t2 = v3 - v7;
        v3 = t1;
        v7 = t2;

        //Calculate m2[0][] - m2[3][]

        t1 = v0 + v2;
        t2 = v0 - v2;
        v0 = t1;
        v2 = t2;

        t1 = v1 + v3;
        t2 = v1 - v3;
        v1 = t1;
        v3 = t2;

        t1 = v0 + v1;
        t2 = v0 - v1;
        v0 = abs(t1);
        v1 = abs(t2);

        t1 = v2 + v3;
        t2 = v2 - v3;
        v2 = abs(t1);
        v3 = abs(t2);

        //Calculate m2[4][] - m2[7][]

        t1 = v4 + v6;
        t2 = v4 - v6;
        v4 = t1;
        v6 = t2;

        t1 = v5 + v7;
        t2 = v5 - v7;
        v5 = t1;
        v7 = t2;

        t1 = v4 + v5;
        t2 = v4 - v5;
        v4 = abs(t1);
        v5 = abs(t2);

        t1 = v6 + v7;
        t2 = v6 - v7;
        v6 = abs(t1);
        v7 = abs(t2);

        Vec4i s0, s1, s2, s3, s4, s5, s6, s7, s8;
        s0 = extend_low(v0);
        s1 = extend_high(v0);
        s0 = s0 + s1;

        s1 = extend_low(v1);
        s2 = extend_high(v1);
        s1 = s1 + s2;

        s2 = extend_low(v2);
        s3 = extend_high(v2);
        s2 = s2 + s3;

        s3 = extend_low(v3);
        s4 = extend_high(v3);
        s3 = s3 + s4;

        s4 = extend_low(v4);
        s5 = extend_high(v4);
        s4 = s4 + s5;

        s5 = extend_low(v5);
        s6 = extend_high(v5);
        s5 = s5 + s6;

        s6 = extend_low(v6);
        s7 = extend_high(v6);
        s6 = s6 + s7;

        s7 = extend_low(v7);
        s8 = extend_high(v7);
        s7 = s7 + s8;

        s0 = (s0 + s1) + (s2 + s3) + (s4 + s5) + (s6 + s7);

        satd = horizontal_add_x(s0);
    }

    return (satd + 2) >> 2;
}

template<int lx, int ly>
int satd(pixel * fenc, intptr_t fencstride, pixel * fref, intptr_t frefstride)
{
    int uiSum = 0;

    for (int row = 0; row < ly; row += 4)
    {
        for (int col = 0; col < lx; col += 4)
        {
            uiSum += satd_4x4(fenc + fencstride * row + col, fencstride,
                              fref + frefstride * row + col, frefstride);
        }
    }

    return uiSum;
}


