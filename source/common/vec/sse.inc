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

template<int ly>
int sse_pp4(pixel* Org, intptr_t strideOrg, pixel* Cur, intptr_t strideCur)
{
    int rows = ly;
    Vec16uc m1, n1;

    Vec8us diff(0);
    Vec4i sum(0);
    for (; rows != 0; rows--)
    {
        m1.fromUint32(*(uint32_t*)Org);
        n1.fromUint32(*(uint32_t*)Cur);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += extend_low(diff);
        Org += strideOrg;
        Cur += strideCur;
    }

    return horizontal_add(sum);
}

#if INSTRSET >= X265_CPU_LEVEL_SSE41
template<int ly>
int sse_pp8(pixel* Org, intptr_t strideOrg, pixel* Cur, intptr_t strideCur)
{
    int rows = ly;
    __m128i sum = _mm_set1_epi32(0);

    for (; rows != 0; rows--)
    {
        __m128i m1 = _mm_loadu_si128((__m128i const*)(Org));
        m1 = _mm_cvtepu8_epi16(m1);
        __m128i n1 = _mm_loadu_si128((__m128i const*)(Cur));
        n1 = _mm_cvtepu8_epi16(n1);

        __m128i diff = _mm_sub_epi16(m1, n1);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        Org += strideOrg;
        Cur += strideCur;
    }

    sum = _mm_add_epi32(sum, _mm_srli_si128(sum, 8));
    sum = _mm_add_epi32(sum, _mm_srli_si128(sum, 4));
    return _mm_cvtsi128_si32(sum);
}

template<int ly>
int sse_pp12(pixel* Org, intptr_t strideOrg, pixel* Cur, intptr_t strideCur)
{
    int rows = ly;
    __m128i sum = _mm_set1_epi32(0);

    for (; rows != 0; rows--)
    {
        __m128i m1 = _mm_loadu_si128((__m128i const*)(Org));
        __m128i n1 = _mm_loadu_si128((__m128i const*)(Cur));

        m1 = _mm_srli_si128(_mm_slli_si128(m1, 4), 4);
        n1 = _mm_srli_si128(_mm_slli_si128(n1, 4), 4);

        __m128i m1lo = _mm_cvtepu8_epi16(m1);
        __m128i m1hi = _mm_srli_si128(m1, 8);
        m1hi = _mm_cvtepu8_epi16(m1hi);

        __m128i n1lo = _mm_cvtepu8_epi16(n1);
        __m128i n1hi = _mm_srli_si128(n1, 8);
        n1hi = _mm_cvtepu8_epi16(n1hi);

        __m128i diff = _mm_sub_epi16(m1lo, n1lo);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        diff = _mm_sub_epi16(m1hi, n1hi);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        Org += strideOrg;
        Cur += strideCur;
    }

    sum = _mm_add_epi32(sum, _mm_srli_si128(sum, 8));
    sum = _mm_add_epi32(sum, _mm_srli_si128(sum, 4));
    return _mm_cvtsi128_si32(sum);
}

template<int ly>
int sse_pp16(pixel* Org, intptr_t strideOrg, pixel* Cur, intptr_t strideCur)
{
    int rows = ly;
    __m128i sum = _mm_set1_epi32(0);

    for (; rows != 0; rows--)
    {
        __m128i m1 = _mm_loadu_si128((__m128i const*)(Org));
        __m128i n1 = _mm_loadu_si128((__m128i const*)(Cur));

        __m128i m1lo = _mm_cvtepu8_epi16(m1);
        __m128i m1hi = _mm_srli_si128(m1, 8);
        m1hi = _mm_cvtepu8_epi16(m1hi);

        __m128i n1lo = _mm_cvtepu8_epi16(n1);
        __m128i n1hi = _mm_srli_si128(n1, 8);
        n1hi = _mm_cvtepu8_epi16(n1hi);

        __m128i diff = _mm_sub_epi16(m1lo, n1lo);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        diff = _mm_sub_epi16(m1hi, n1hi);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        Org += strideOrg;
        Cur += strideCur;
    }

    sum = _mm_add_epi32(sum, _mm_srli_si128(sum, 8));
    sum = _mm_add_epi32(sum, _mm_srli_si128(sum, 4));
    return _mm_cvtsi128_si32(sum);
}

template<int ly>
int sse_pp24(pixel* Org, intptr_t strideOrg, pixel* Cur, intptr_t strideCur)
{
    int rows = ly;
    __m128i sum = _mm_set1_epi32(0);

    for (; rows != 0; rows--)
    {
        __m128i m1 = _mm_loadu_si128((__m128i const*)(Org));
        __m128i n1 = _mm_loadu_si128((__m128i const*)(Cur));

        __m128i m1lo = _mm_cvtepu8_epi16(m1);
        __m128i m1hi = _mm_srli_si128(m1, 8);
        m1hi = _mm_cvtepu8_epi16(m1hi);

        __m128i n1lo = _mm_cvtepu8_epi16(n1);
        __m128i n1hi = _mm_srli_si128(n1, 8);
        n1hi = _mm_cvtepu8_epi16(n1hi);

        __m128i diff = _mm_sub_epi16(m1lo, n1lo);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        diff = _mm_sub_epi16(m1hi, n1hi);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        m1 = _mm_loadu_si128((__m128i const*)(Org + 16));
        m1 = _mm_cvtepu8_epi16(m1);
        n1 = _mm_loadu_si128((__m128i const*)(Cur + 16));
        n1 = _mm_cvtepu8_epi16(n1);

        diff = _mm_sub_epi16(m1, n1);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        Org += strideOrg;
        Cur += strideCur;
    }

    sum = _mm_add_epi32(sum, _mm_srli_si128(sum, 8));
    sum = _mm_add_epi32(sum, _mm_srli_si128(sum, 4));
    return _mm_cvtsi128_si32(sum);
}

template<int ly>
int sse_pp32(pixel* Org, intptr_t strideOrg, pixel* Cur, intptr_t strideCur)
{
    int rows = ly;
    __m128i sum = _mm_set1_epi32(0);

    for (; rows != 0; rows--)
    {
        __m128i m1 = _mm_loadu_si128((__m128i const*)(Org));
        __m128i n1 = _mm_loadu_si128((__m128i const*)(Cur));

        __m128i m1lo = _mm_cvtepu8_epi16(m1);
        __m128i m1hi = _mm_srli_si128(m1, 8);
        m1hi = _mm_cvtepu8_epi16(m1hi);

        __m128i n1lo = _mm_cvtepu8_epi16(n1);
        __m128i n1hi = _mm_srli_si128(n1, 8);
        n1hi = _mm_cvtepu8_epi16(n1hi);

        __m128i diff = _mm_sub_epi16(m1lo, n1lo);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        diff = _mm_sub_epi16(m1hi, n1hi);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        m1 = _mm_loadu_si128((__m128i const*)(Org + 16));
        n1 = _mm_loadu_si128((__m128i const*)(Cur + 16));

        m1lo = _mm_cvtepu8_epi16(m1);
        m1hi = _mm_srli_si128(m1, 8);
        m1hi = _mm_cvtepu8_epi16(m1hi);

        n1lo = _mm_cvtepu8_epi16(n1);
        n1hi = _mm_srli_si128(n1, 8);
        n1hi = _mm_cvtepu8_epi16(n1hi);

        diff = _mm_sub_epi16(m1lo, n1lo);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        diff = _mm_sub_epi16(m1hi, n1hi);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        Org += strideOrg;
        Cur += strideCur;
    }

    sum = _mm_add_epi32(sum, _mm_srli_si128(sum, 8));
    sum = _mm_add_epi32(sum, _mm_srli_si128(sum, 4));
    return _mm_cvtsi128_si32(sum);
}

template<int ly>
int sse_pp48(pixel* Org, intptr_t strideOrg, pixel* Cur, intptr_t strideCur)
{
    int rows = ly;
    __m128i sum = _mm_set1_epi32(0);

    for (; rows != 0; rows--)
    {
        __m128i m1 = _mm_loadu_si128((__m128i const*)(Org));
        __m128i n1 = _mm_loadu_si128((__m128i const*)(Cur));

        __m128i m1lo = _mm_cvtepu8_epi16(m1);
        __m128i m1hi = _mm_srli_si128(m1, 8);
        m1hi = _mm_cvtepu8_epi16(m1hi);

        __m128i n1lo = _mm_cvtepu8_epi16(n1);
        __m128i n1hi = _mm_srli_si128(n1, 8);
        n1hi = _mm_cvtepu8_epi16(n1hi);

        __m128i diff = _mm_sub_epi16(m1lo, n1lo);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        diff = _mm_sub_epi16(m1hi, n1hi);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        m1 = _mm_loadu_si128((__m128i const*)(Org + 16));
        n1 = _mm_loadu_si128((__m128i const*)(Cur + 16));

        m1lo = _mm_cvtepu8_epi16(m1);
        m1hi = _mm_srli_si128(m1, 8);
        m1hi = _mm_cvtepu8_epi16(m1hi);

        n1lo = _mm_cvtepu8_epi16(n1);
        n1hi = _mm_srli_si128(n1, 8);
        n1hi = _mm_cvtepu8_epi16(n1hi);

        diff = _mm_sub_epi16(m1lo, n1lo);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        diff = _mm_sub_epi16(m1hi, n1hi);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        m1 = _mm_loadu_si128((__m128i const*)(Org + 32));
        n1 = _mm_loadu_si128((__m128i const*)(Cur + 32));

        m1lo = _mm_cvtepu8_epi16(m1);
        m1hi = _mm_srli_si128(m1, 8);
        m1hi = _mm_cvtepu8_epi16(m1hi);

        n1lo = _mm_cvtepu8_epi16(n1);
        n1hi = _mm_srli_si128(n1, 8);
        n1hi = _mm_cvtepu8_epi16(n1hi);

        diff = _mm_sub_epi16(m1lo, n1lo);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        diff = _mm_sub_epi16(m1hi, n1hi);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        Org += strideOrg;
        Cur += strideCur;
    }

    sum = _mm_add_epi32(sum, _mm_srli_si128(sum, 8));
    sum = _mm_add_epi32(sum, _mm_srli_si128(sum, 4));
    return _mm_cvtsi128_si32(sum);
}

#endif /* if INSTRSET >= X265_CPU_LEVEL_SSE41 */

template<int ly>
int sse_pp64(pixel* Org, intptr_t strideOrg, pixel* Cur, intptr_t strideCur)
{
    int rows = ly;
    Vec16uc m1, n1;

    Vec8us diff_low(0), diff_high(0);
    Vec4i sum_low(0), sum_high(0);
    for (; rows != 0; rows--)
    {
        m1.load(Org);
        n1.load(Cur);
        diff_low = extend_low(m1) - extend_low(n1);
        diff_high = extend_high(m1) - extend_high(n1);
        diff_low = diff_low * diff_low;
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));

        m1.load(Org + 16);
        n1.load(Cur + 16);
        diff_low = extend_low(m1) - extend_low(n1);
        diff_high = extend_high(m1) - extend_high(n1);
        diff_low = diff_low * diff_low;
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));

        m1.load(Org + 32);
        n1.load(Cur + 32);
        diff_low = extend_low(m1) - extend_low(n1);
        diff_high = extend_high(m1) - extend_high(n1);
        diff_low = diff_low * diff_low;
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));

        m1.load(Org + 48);
        n1.load(Cur + 48);
        diff_low = extend_low(m1) - extend_low(n1);
        diff_high = extend_high(m1) - extend_high(n1);
        diff_low = diff_low * diff_low;
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));
        Org += strideOrg;
        Cur += strideCur;
    }

    return horizontal_add(sum_low) + horizontal_add(sum_high);
}

template<int ly>
int sse_ss4(short* Org, intptr_t strideOrg, short* Cur, intptr_t strideCur)
{
    int rows = ly;

    Vec4i diff(0);
    Vec8s m1, n1;
    Vec4i sum(0);
    for (; rows != 0; rows--)
    {
        m1.load(Org);
        n1.load(Cur);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        Org += strideOrg;
        Cur += strideCur;
    }

    return horizontal_add(sum);
}

template<int ly>
int sse_ss8(short* Org, intptr_t strideOrg, short* Cur, intptr_t strideCur)
{
    int rows = ly;

    Vec4i diff(0);
    Vec8s m1, n1;
    Vec4i sum(0);
    for (; rows != 0; rows--)
    {
        m1.load(Org);
        n1.load(Cur);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;

        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;
        Org += strideOrg;
        Cur += strideCur;
    }

    return horizontal_add(sum);
}

template<int ly>
int sse_ss12(short* Org, intptr_t strideOrg, short* Cur, intptr_t strideCur)
{
    int rows = ly;

    Vec4i diff(0);
    Vec8s m1, n1;
    Vec4i sum(0);
    for (; rows != 0; rows--)
    {
        m1.load(Org);
        n1.load(Cur);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(Org + 8);
        n1.load(Cur + 8);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;

        Org += strideOrg;
        Cur += strideCur;
    }

    return horizontal_add(sum);
}

template<int ly>
int sse_ss16(short* Org, intptr_t strideOrg, short* Cur, intptr_t strideCur)
{
    int rows = ly;

    Vec4i diff(0);
    Vec8s m1, n1;
    Vec4i sum(0);
    for (; rows != 0; rows--)
    {
        m1.load(Org);
        n1.load(Cur);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(Org + 8);
        n1.load(Cur + 8);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        Org += strideOrg;
        Cur += strideCur;
    }

    return horizontal_add(sum);
}

template<int ly>
int sse_ss24(short* Org, intptr_t strideOrg, short* Cur, intptr_t strideCur)
{
    int rows = ly;

    Vec4i diff(0);
    Vec8s m1, n1;
    Vec4i sum(0);
    for (; rows != 0; rows--)
    {
        m1.load(Org);
        n1.load(Cur);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(Org + 8);
        n1.load(Cur + 8);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(Org + 16);
        n1.load(Cur + 16);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        Org += strideOrg;
        Cur += strideCur;
    }

    return horizontal_add(sum);
}

template<int ly>
int sse_ss32(short* Org, intptr_t strideOrg, short* Cur, intptr_t strideCur)
{
    int rows = ly;

    Vec4i diff(0);
    Vec8s m1, n1;
    Vec4i sum(0);
    for (; rows != 0; rows--)
    {
        m1.load(Org);
        n1.load(Cur);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(Org + 8);
        n1.load(Cur + 8);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(Org + 16);
        n1.load(Cur + 16);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(Org + 24);
        n1.load(Cur + 24);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        Org += strideOrg;
        Cur += strideCur;
    }

    return horizontal_add(sum);
}

template<int ly>
int sse_ss48(short* Org, intptr_t strideOrg, short* Cur, intptr_t strideCur)
{
    int rows = ly;

    Vec4i diff(0);
    Vec8s m1, n1;
    Vec4i sum(0);
    for (; rows != 0; rows--)
    {
        m1.load(Org);
        n1.load(Cur);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(Org + 8);
        n1.load(Cur + 8);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(Org + 16);
        n1.load(Cur + 16);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(Org + 24);
        n1.load(Cur + 24);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(Org + 32);
        n1.load(Cur + 32);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(Org + 40);
        n1.load(Cur + 40);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        Org += strideOrg;
        Cur += strideCur;
    }

    return horizontal_add(sum);
}

template<int ly>
int sse_ss64(short* Org, intptr_t strideOrg, short* Cur, intptr_t strideCur)
{
    int rows = ly;

    Vec4i diff(0);
    Vec8s m1, n1;
    Vec4i sum(0);
    for (; rows != 0; rows--)
    {
        m1.load(Org);
        n1.load(Cur);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(Org + 8);
        n1.load(Cur + 8);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(Org + 16);
        n1.load(Cur + 16);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(Org + 24);
        n1.load(Cur + 24);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(Org + 32);
        n1.load(Cur + 32);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(Org + 40);
        n1.load(Cur + 40);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(Org + 48);
        n1.load(Cur + 48);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(Org + 56);
        n1.load(Cur + 56);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        Org += strideOrg;
        Cur += strideCur;
    }

    return horizontal_add(sum);
}

template<int ly>
int sse_sp4(short* Org, intptr_t strideOrg, pixel* Cur, intptr_t strideCur)
{
    int rows = ly;
    Vec8s m1;
    Vec16uc n1;

    Vec4i diff_low(0);
    Vec4i sum_low(0);
    for (; rows != 0; rows--)
    {
        m1.load(Org);
        n1.fromUint32(*(uint32_t*)Cur);
        diff_low = extend_low(m1) - extend_low(extend_low(n1));
        diff_low = diff_low * diff_low;
        sum_low += diff_low;

        Org += strideOrg;
        Cur += strideCur;
    }

    return horizontal_add(sum_low);
}

template<int ly>
int sse_sp8(short* Org, intptr_t strideOrg, pixel* Cur, intptr_t strideCur)
{
    int rows = ly;
    Vec8s m1;
    Vec16uc n1;

    Vec8us diff_low(0), diff_high(0);
    Vec4i sum_low(0), sum_high(0);
    for (; rows != 0; rows--)
    {
        n1.load(Cur);
        m1.load(Org);
        diff_low = m1 - extend_low(n1);
        diff_low = diff_low * diff_low;
        sum_low += extend_low(diff_low);
        sum_high += extend_high(diff_low);

        Org += strideOrg;
        Cur += strideCur;
    }

    return horizontal_add(sum_low) + horizontal_add(sum_high);
}

template<int ly>
int sse_sp12(short* Org, intptr_t strideOrg, pixel* Cur, intptr_t strideCur)
{
    int rows = ly;
    Vec8s m1;
    Vec16uc n1;

    Vec8us diff_low(0);
    Vec4i diff_high(0);
    Vec4i sum_low(0), sum_high(0);
    for (; rows != 0; rows--)
    {
        n1.load(Cur);
        n1.cutoff(12);
        m1.load(Org);
        diff_low = m1 - extend_low(n1);
        m1.load(Org + 8);
        diff_high = extend_low(m1) - extend_low(extend_high(n1));
        diff_low = diff_low * diff_low;
        diff_high = diff_high * diff_high;
        sum_low += extend_low(diff_low);
        sum_high += (extend_high(diff_low) + diff_high);

        Org += strideOrg;
        Cur += strideCur;
    }

    return horizontal_add(sum_low) + horizontal_add(sum_high);
}

template<int ly>
int sse_sp16(short* Org, intptr_t strideOrg, pixel* Cur, intptr_t strideCur)
{
    int rows = ly;
    Vec8s m1;
    Vec16uc n1;

    Vec8us diff_low(0), diff_high(0);
    Vec4i sum_low(0), sum_high(0);
    for (; rows != 0; rows--)
    {
        n1.load(Cur);
        m1.load(Org);
        diff_low = m1 - extend_low(n1);
        m1.load(Org + 8);
        diff_high = m1 - extend_high(n1);
        diff_low = diff_low * diff_low;
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));

        Org += strideOrg;
        Cur += strideCur;
    }

    return horizontal_add(sum_low) + horizontal_add(sum_high);
}

template<int ly>
int sse_sp24(short* Org, intptr_t strideOrg, pixel* Cur, intptr_t strideCur)
{
    int rows = ly;
    Vec8s m1;
    Vec16uc n1;

    Vec8us diff_low(0), diff_high(0);
    Vec4i sum_low(0), sum_high(0);
    for (; rows != 0; rows--)
    {
        n1.load(Cur);
        m1.load(Org);
        diff_low = m1 - extend_low(n1);
        m1.load(Org + 8);
        diff_high = m1 - extend_high(n1);
        diff_low = diff_low * diff_low;
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));

        n1.load(Cur + 16);
        m1.load(Org + 16);
        diff_low = m1 - extend_low(n1);
        diff_low = diff_low * diff_low;
        sum_low += extend_low(diff_low);
        sum_high += extend_high(diff_low);

        Org += strideOrg;
        Cur += strideCur;
    }

    return horizontal_add(sum_low) + horizontal_add(sum_high);
}

template<int ly>
int sse_sp32(short* Org, intptr_t strideOrg, pixel* Cur, intptr_t strideCur)
{
    int rows = ly;
    Vec8s m1;
    Vec16uc n1;

    Vec8us diff_low(0), diff_high(0);
    Vec4i sum_low(0), sum_high(0);
    for (; rows != 0; rows--)
    {
        n1.load(Cur);
        m1.load(Org);
        diff_low = m1 - extend_low(n1);
        m1.load(Org + 8);
        diff_high = m1 - extend_high(n1);
        diff_low = diff_low * diff_low;
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));

        n1.load(Cur + 16);
        m1.load(Org + 16);
        diff_low = m1 - extend_low(n1);
        m1.load(Org + 24);
        diff_high = m1 - extend_high(n1);
        diff_low = diff_low * diff_low;
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));

        Org += strideOrg;
        Cur += strideCur;
    }

    return horizontal_add(sum_low) + horizontal_add(sum_high);
}

template<int ly>
int sse_sp48(short* Org, intptr_t strideOrg, pixel* Cur, intptr_t strideCur)
{
    int rows = ly;
    Vec8s m1;
    Vec16uc n1;

    Vec8us diff_low(0), diff_high(0);
    Vec4i sum_low(0), sum_high(0);
    for (; rows != 0; rows--)
    {
        n1.load(Cur);
        m1.load(Org);
        diff_low = m1 - extend_low(n1);
        m1.load_a(Org + 8);
        diff_high = m1 - extend_high(n1);
        diff_low = diff_low * diff_low;
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));

        n1.load(Cur + 16);
        m1.load(Org + 16);
        diff_low = m1 - extend_low(n1);
        m1.load(Org + 24);
        diff_high = m1 - extend_high(n1);
        diff_low = diff_low * diff_low;
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));

        n1.load(Cur + 32);
        m1.load(Org + 32);
        diff_low = m1 - extend_low(n1);
        m1.load(Org + 40);
        diff_high = m1 - extend_high(n1);
        diff_low = diff_low * diff_low;
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));

        Org += strideOrg;
        Cur += strideCur;
    }

    return horizontal_add(sum_low) + horizontal_add(sum_high);
}

template<int ly>
int sse_sp64(short* Org, intptr_t strideOrg, pixel* Cur, intptr_t strideCur)
{
    int rows = ly;
    Vec8s m1;
    Vec16uc n1;

    Vec8us diff_low(0), diff_high(0);
    Vec4i sum_low(0), sum_high(0);
    for (; rows != 0; rows--)
    {
        n1.load(Cur);
        m1.load(Org);
        diff_low = m1 - extend_low(n1);
        m1.load(Org + 8);
        diff_high = m1 - extend_high(n1);
        diff_low = diff_low * diff_low;
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));

        n1.load(Cur + 16);
        m1.load(Org + 16);
        diff_low = m1 - extend_low(n1);
        m1.load(Org + 24);
        diff_high = m1 - extend_high(n1);
        diff_low = diff_low * diff_low;
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));

        n1.load(Cur + 32);
        m1.load(Org + 32);
        diff_low = m1 - extend_low(n1);
        m1.load(Org + 40);
        diff_high = m1 - extend_high(n1);
        diff_low = diff_low * diff_low;
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));

        n1.load(Cur + 48);
        m1.load(Org + 48);
        diff_low = m1 - extend_low(n1);
        m1.load(Org + 56);
        diff_high = m1 - extend_high(n1);
        diff_low = diff_low * diff_low;
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));

        Org += strideOrg;
        Cur += strideCur;
    }

    return horizontal_add(sum_low) + horizontal_add(sum_high);
}
