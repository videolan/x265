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
    Vec8s sum(0);

    for (int row = 0; row < ly; row++)
    {
        Vec16uc m1, n1;
        m1.fromUint32(*(uint32_t*)piOrg);
        n1.fromUint32(*(uint32_t*)piCur);
        sum += Vec8s(m1.sad(n1));

        piOrg += strideOrg;
        piCur += strideCur;
    }

    return horizontal_add(sum);
}

template<int ly>
int CDECL sad_8(pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    Vec8s sum(0);
    for (int row = 0; row < ly; row++)
    {
        Vec16uc m1, n1;
        m1.load(piOrg);
        n1.load(piCur);
        sum += Vec8s(m1.sad(n1));

        piOrg += strideOrg;
        piCur += strideCur;
    }
    return sum[0];
}

template<int ly>
int CDECL sad_16(pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    Vec8s sum(0);
    for (int row = 0; row < ly; row++)
    {
        Vec16uc m1, n1;
        m1.load_a(piOrg);
        n1.load(piCur);
        sum += Vec8s(m1.sad(n1));

        piOrg += strideOrg;
        piCur += strideCur;
    }
    return horizontal_add_x(sum);
}

template<int lx, int ly>
int CDECL sad_32(pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    // TODO: AVX2
    int sum = 0;
    for (int row = 0; row < ly; row++)
    {
        Vec8s sad(0);
        for (int col = 0; col < lx; col += 32)
        {
            Vec16uc m1, n1;
            m1.load_a(piOrg + col);
            n1.load(piCur + col);
            sad += Vec8s(m1.sad(n1));
            Vec16uc m2, n2;
            m2.load_a(piOrg + col + 16);
            n2.load(piCur + col + 16);
            sad += Vec8s(m2.sad(n2));
        }

        piOrg += strideOrg;
        piCur += strideCur;
        sum += horizontal_add_x(sad);
    }
    return sum;
}
