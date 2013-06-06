/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Mandar Gurav <mandar@multicorewareinc.com>
 *          Gopu Govindaswamy <gopu@multicorewareinc.com>
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
int sse_pp4(pixel* piOrg, intptr_t iStrideOrg, pixel* piCur, intptr_t iStrideCur)
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
int sse_pp8(pixel* piOrg, intptr_t iStrideOrg, pixel* piCur, intptr_t iStrideCur)
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
int sse_pp12(pixel* piOrg, intptr_t iStrideOrg, pixel* piCur, intptr_t iStrideCur)
{
    int iRows = ly;
    Vec16uc m1, n1;

    Vec8us diff(0);
    Vec4i sum(0);
    for (; iRows != 0; iRows--)
    {
        m1.load_a(piOrg);
        m1.cutoff(12);
        n1.load(piCur);
        n1.cutoff(12);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += (extend_low(diff) + (extend_high(diff)));

        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += (extend_low(diff) + (extend_high(diff)));

        piOrg += iStrideOrg;
        piCur += iStrideCur;
    }

    return horizontal_add(sum);
}

template<int ly>
int sse_pp16(pixel* piOrg, intptr_t iStrideOrg, pixel* piCur, intptr_t iStrideCur)
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

    return horizontal_add(sum_low) + horizontal_add(sum_high);
}

template<int ly>
int sse_pp24(pixel* piOrg, intptr_t iStrideOrg, pixel* piCur, intptr_t iStrideCur)
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
        diff_low = diff_low * diff_low;
        sum_low += extend_low(diff_low);
        sum_high += extend_high(diff_low);

        piOrg += iStrideOrg;
        piCur += iStrideCur;
    }

    return horizontal_add(sum_low) + horizontal_add(sum_high);
}

template<int ly>
int sse_pp32(pixel* piOrg, intptr_t iStrideOrg, pixel* piCur, intptr_t iStrideCur)
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

    return horizontal_add(sum_low) + horizontal_add(sum_high);
}

template<int ly>
int sse_pp48(pixel* piOrg, intptr_t iStrideOrg, pixel* piCur, intptr_t iStrideCur)
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

    return horizontal_add(sum_low) + horizontal_add(sum_high);
}

template<int ly>
int sse_pp64(pixel* piOrg, intptr_t iStrideOrg, pixel* piCur, intptr_t iStrideCur)
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

    return horizontal_add(sum_low) + horizontal_add(sum_high);
}

template<int ly>
int sse_ss4(short* piOrg, intptr_t iStrideOrg, short* piCur, intptr_t iStrideCur)
{
    int iRows = ly;

    Vec4i diff(0);
    Vec8s m1, n1;
    Vec4i sum(0);
    for (; iRows != 0; iRows--)
    {
        m1.load_a(piOrg);
        n1.load(piCur);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        piOrg += iStrideOrg;
        piCur += iStrideCur;
    }

    return horizontal_add(sum);
}

template<int ly>
int sse_ss8(short* piOrg, intptr_t iStrideOrg, short* piCur, intptr_t iStrideCur)
{
    int iRows = ly;

    Vec4i diff(0);
    Vec8s m1, n1;
    Vec4i sum(0);
    for (; iRows != 0; iRows--)
    {
        m1.load_a(piOrg);
        n1.load(piCur);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;

        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;
        piOrg += iStrideOrg;
        piCur += iStrideCur;
    }

    return horizontal_add(sum);
}

template<int ly>
int sse_ss12(short* piOrg, intptr_t iStrideOrg, short* piCur, intptr_t iStrideCur)
{
    int iRows = ly;

    Vec4i diff(0);
    Vec8s m1, n1;
    Vec4i sum(0);
    for (; iRows != 0; iRows--)
    {
        m1.load_a(piOrg);
        n1.load(piCur);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load_a(piOrg + 8);
        n1.load(piCur + 8);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;

        piOrg += iStrideOrg;
        piCur += iStrideCur;
    }

    return horizontal_add(sum);
}

template<int ly>
int sse_ss16(short* piOrg, intptr_t iStrideOrg, short* piCur, intptr_t iStrideCur)
{
    int iRows = ly;

    Vec4i diff(0);
    Vec8s m1, n1;
    Vec4i sum(0);
    for (; iRows != 0; iRows--)
    {
        m1.load_a(piOrg);
        n1.load(piCur);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load_a(piOrg + 8);
        n1.load(piCur + 8);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        piOrg += iStrideOrg;
        piCur += iStrideCur;
    }

    return horizontal_add(sum);
}

template<int ly>
int sse_ss24(short* piOrg, intptr_t iStrideOrg, short* piCur, intptr_t iStrideCur)
{
    int iRows = ly;

    Vec4i diff(0);
    Vec8s m1, n1;
    Vec4i sum(0);
    for (; iRows != 0; iRows--)
    {
        m1.load_a(piOrg);
        n1.load(piCur);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load_a(piOrg + 8);
        n1.load(piCur + 8);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load_a(piOrg + 16);
        n1.load(piCur + 16);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        piOrg += iStrideOrg;
        piCur += iStrideCur;
    }

    return horizontal_add(sum);
}

template<int ly>
int sse_ss32(short* piOrg, intptr_t iStrideOrg, short* piCur, intptr_t iStrideCur)
{
    int iRows = ly;

    Vec4i diff(0);
    Vec8s m1, n1;
    Vec4i sum(0);
    for (; iRows != 0; iRows--)
    {
        m1.load_a(piOrg);
        n1.load(piCur);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load_a(piOrg + 8);
        n1.load(piCur + 8);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load_a(piOrg + 16);
        n1.load(piCur + 16);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load_a(piOrg + 24);
        n1.load(piCur + 24);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        piOrg += iStrideOrg;
        piCur += iStrideCur;
    }

    return horizontal_add(sum);
}

template<int ly>
int sse_ss48(short* piOrg, intptr_t iStrideOrg, short* piCur, intptr_t iStrideCur)
{
    int iRows = ly;

    Vec4i diff(0);
    Vec8s m1, n1;
    Vec4i sum(0);
    for (; iRows != 0; iRows--)
    {
        m1.load_a(piOrg);
        n1.load(piCur);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load_a(piOrg + 8);
        n1.load(piCur + 8);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load_a(piOrg + 16);
        n1.load(piCur + 16);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load_a(piOrg + 24);
        n1.load(piCur + 24);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load_a(piOrg + 32);
        n1.load(piCur + 32);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load_a(piOrg + 40);
        n1.load(piCur + 40);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        piOrg += iStrideOrg;
        piCur += iStrideCur;
    }

    return horizontal_add(sum);
}

template<int ly>
int sse_ss64(short* piOrg, intptr_t iStrideOrg, short* piCur, intptr_t iStrideCur)
{
    int iRows = ly;

    Vec4i diff(0);
    Vec8s m1, n1;
    Vec4i sum(0);
    for (; iRows != 0; iRows--)
    {
        m1.load_a(piOrg);
        n1.load(piCur);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load_a(piOrg + 8);
        n1.load(piCur + 8);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load_a(piOrg + 16);
        n1.load(piCur + 16);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load_a(piOrg + 24);
        n1.load(piCur + 24);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load_a(piOrg + 32);
        n1.load(piCur + 32);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load_a(piOrg + 40);
        n1.load(piCur + 40);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load_a(piOrg + 48);
        n1.load(piCur + 48);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load_a(piOrg + 56);
        n1.load(piCur + 56);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        piOrg += iStrideOrg;
        piCur += iStrideCur;
    }

    return horizontal_add(sum);
}

template<int ly>
int sse_sp4(short* piOrg, intptr_t iStrideOrg, pixel* piCur, intptr_t iStrideCur)
{
    int iRows = ly;
    Vec8s m1;
    Vec16uc n1;

    Vec4i diff_low(0);
    Vec4i sum_low(0);
    for (; iRows != 0; iRows--)
    {
        m1.load_a(piOrg);
        n1.fromUint32(*(uint32_t*)piCur);
        diff_low = extend_low(m1) - extend_low(extend_low(n1));
        diff_low = diff_low * diff_low;
        sum_low += diff_low;

        piOrg += iStrideOrg;
        piCur += iStrideCur;
    }

    return horizontal_add(sum_low);
}

template<int ly>
int sse_sp8(short* piOrg, intptr_t iStrideOrg, pixel* piCur, intptr_t iStrideCur)
{
    int iRows = ly;
    Vec8s m1;
    Vec16uc n1;

    Vec4ui diff_low, diff_high; 
    Vec8us diff_s;
    Vec4ui sum(0);
    for (; iRows != 0; iRows--)
    {        
        m1.load_a(piOrg);
        n1.load(piCur);
        diff_s = m1 - extend_low(n1);

        diff_low = extend_low(diff_s);
        diff_low = diff_low * diff_low;

        diff_high = extend_high(diff_s);
        diff_high = diff_high * diff_high;

        sum += diff_low + diff_high;

        piOrg += iStrideOrg;
        piCur += iStrideCur;
    }

    return horizontal_add(sum);
}


