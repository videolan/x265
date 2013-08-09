/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Mandar Gurav <mandar@multicorewareinc.com>
 *          Gopu Govindaswamy <gopu@multicorewareinc.com>
 *          Praveen Kumar Tiwari <praveen@multicorewareinc.com>
 *          Nabajit Deka <nabajit@multicorewareinc.com>
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
int sse_pp4(pixel* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    int rows = ly;
    Vec16uc m1, n1;

    Vec8us diff(0);
    Vec4i sum(0);
    for (; rows != 0; rows--)
    {
        m1.fromUint32(*(uint32_t*)fenc);
        n1.fromUint32(*(uint32_t*)fref);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += extend_low(diff);
        fenc += strideFenc;
        fref += strideFref;
    }

    return horizontal_add(sum);
}

#if INSTRSET >= X265_CPU_LEVEL_SSE41
template<int ly>
int sse_pp8(pixel* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i sum = _mm_setzero_si128();

    for (; rows != 0; rows--)
    {
        __m128i m1 = _mm_loadu_si128((__m128i const*)(fenc));
        m1 = _mm_cvtepu8_epi16(m1);
        __m128i n1 = _mm_loadu_si128((__m128i const*)(fref));
        n1 = _mm_cvtepu8_epi16(n1);

        __m128i diff = _mm_sub_epi16(m1, n1);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        fenc += strideFenc;
        fref += strideFref;
    }

    sum = _mm_add_epi32(sum, _mm_srli_si128(sum, 8));
    sum = _mm_add_epi32(sum, _mm_srli_si128(sum, 4));
    return _mm_cvtsi128_si32(sum);
}

template<int ly>
int sse_pp12(pixel* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i sum = _mm_setzero_si128();
    __m128i zero = _mm_setzero_si128();

    for (; rows != 0; rows--)
    {
        __m128i m1 = _mm_loadu_si128((__m128i const*)(fenc));
        __m128i n1 = _mm_loadu_si128((__m128i const*)(fref));

        __m128i m1lo = _mm_cvtepu8_epi16(m1);
        __m128i m1hi = _mm_unpackhi_epi8(m1, zero);

        __m128i n1lo = _mm_cvtepu8_epi16(n1);
        __m128i n1hi = _mm_unpackhi_epi8(n1, zero);

        __m128i difflo = _mm_sub_epi16(m1lo, n1lo);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(difflo, difflo));

        __m128i diffhi = _mm_sub_epi16(m1hi, n1hi);
        __m128i sum_temp = _mm_madd_epi16(diffhi, diffhi);

        sum = _mm_add_epi32(sum, _mm_srli_si128(_mm_slli_si128(sum_temp, 8), 8));

        fenc += strideFenc;
        fref += strideFref;
    }

    sum = _mm_hadd_epi32(sum, sum);
    sum = _mm_hadd_epi32(sum, sum);
    return _mm_cvtsi128_si32(sum);
}

template<int ly>
int sse_pp16(pixel* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i sum = _mm_setzero_si128();
    __m128i zero = _mm_setzero_si128();

    for (; rows != 0; rows--)
    {
        __m128i m1 = _mm_loadu_si128((__m128i const*)(fenc));
        __m128i n1 = _mm_loadu_si128((__m128i const*)(fref));

        __m128i m1lo = _mm_cvtepu8_epi16(m1);
        __m128i m1hi = _mm_unpackhi_epi8(m1, zero);

        __m128i n1lo = _mm_cvtepu8_epi16(n1);
        __m128i n1hi = _mm_unpackhi_epi8(n1, zero);

        __m128i diff = _mm_sub_epi16(m1lo, n1lo);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        diff = _mm_sub_epi16(m1hi, n1hi);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        fenc += strideFenc;
        fref += strideFref;
    }

    sum = _mm_hadd_epi32(sum, sum);
    sum = _mm_hadd_epi32(sum, sum);
    return _mm_cvtsi128_si32(sum);
}

template<int ly>
int sse_pp24(pixel* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i sum = _mm_setzero_si128();
    __m128i zero = _mm_setzero_si128();

    for (; rows != 0; rows--)
    {
        __m128i m1 = _mm_loadu_si128((__m128i const*)(fenc));
        __m128i n1 = _mm_loadu_si128((__m128i const*)(fref));

        __m128i m1lo = _mm_cvtepu8_epi16(m1);
        __m128i m1hi = _mm_unpackhi_epi8(m1, zero);

        __m128i n1lo = _mm_cvtepu8_epi16(n1);
        __m128i n1hi = _mm_unpackhi_epi8(n1, zero);

        __m128i diff = _mm_sub_epi16(m1lo, n1lo);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        diff = _mm_sub_epi16(m1hi, n1hi);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        m1 = _mm_loadu_si128((__m128i const*)(fenc + 16));
        m1 = _mm_cvtepu8_epi16(m1);
        n1 = _mm_loadu_si128((__m128i const*)(fref + 16));
        n1 = _mm_cvtepu8_epi16(n1);

        diff = _mm_sub_epi16(m1, n1);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        fenc += strideFenc;
        fref += strideFref;
    }

    sum = _mm_hadd_epi32(sum, sum);
    sum = _mm_hadd_epi32(sum, sum);
    return _mm_cvtsi128_si32(sum);
}

template<int ly>
int sse_pp32(pixel* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i sum = _mm_setzero_si128();
    __m128i zero = _mm_setzero_si128();

    for (; rows != 0; rows--)
    {
        __m128i m1 = _mm_loadu_si128((__m128i const*)(fenc));
        __m128i n1 = _mm_loadu_si128((__m128i const*)(fref));

        __m128i m1lo = _mm_cvtepu8_epi16(m1);
        __m128i m1hi = _mm_unpackhi_epi8(m1, zero);

        __m128i n1lo = _mm_cvtepu8_epi16(n1);
        __m128i n1hi = _mm_unpackhi_epi8(n1, zero);

        __m128i diff = _mm_sub_epi16(m1lo, n1lo);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        diff = _mm_sub_epi16(m1hi, n1hi);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        m1 = _mm_loadu_si128((__m128i const*)(fenc + 16));
        n1 = _mm_loadu_si128((__m128i const*)(fref + 16));

        m1lo = _mm_cvtepu8_epi16(m1);
        m1hi = _mm_unpackhi_epi8(m1, zero);

        n1lo = _mm_cvtepu8_epi16(n1);
        n1hi = _mm_unpackhi_epi8(n1, zero);

        diff = _mm_sub_epi16(m1lo, n1lo);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        diff = _mm_sub_epi16(m1hi, n1hi);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        fenc += strideFenc;
        fref += strideFref;
    }

    sum = _mm_hadd_epi32(sum, sum);
    sum = _mm_hadd_epi32(sum, sum);
    return _mm_cvtsi128_si32(sum);
}

template<int ly>
int sse_pp48(pixel* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i sum = _mm_setzero_si128();
    __m128i zero = _mm_setzero_si128();

    for (; rows != 0; rows--)
    {
        __m128i m1 = _mm_loadu_si128((__m128i const*)(fenc));
        __m128i n1 = _mm_loadu_si128((__m128i const*)(fref));

        __m128i m1lo = _mm_cvtepu8_epi16(m1);
        __m128i m1hi = _mm_unpackhi_epi8(m1, zero);

        __m128i n1lo = _mm_cvtepu8_epi16(n1);
        __m128i n1hi = _mm_unpackhi_epi8(n1, zero);

        __m128i diff = _mm_sub_epi16(m1lo, n1lo);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        diff = _mm_sub_epi16(m1hi, n1hi);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        m1 = _mm_loadu_si128((__m128i const*)(fenc + 16));
        n1 = _mm_loadu_si128((__m128i const*)(fref + 16));

        m1lo = _mm_cvtepu8_epi16(m1);
        m1hi = _mm_unpackhi_epi8(m1, zero);

        n1lo = _mm_cvtepu8_epi16(n1);
        n1hi = _mm_unpackhi_epi8(n1, zero);

        diff = _mm_sub_epi16(m1lo, n1lo);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        diff = _mm_sub_epi16(m1hi, n1hi);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        m1 = _mm_loadu_si128((__m128i const*)(fenc + 32));
        n1 = _mm_loadu_si128((__m128i const*)(fref + 32));

        m1lo = _mm_cvtepu8_epi16(m1);
        m1hi = _mm_unpackhi_epi8(m1, zero);

        n1lo = _mm_cvtepu8_epi16(n1);
        n1hi = _mm_unpackhi_epi8(n1, zero);

        diff = _mm_sub_epi16(m1lo, n1lo);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        diff = _mm_sub_epi16(m1hi, n1hi);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        fenc += strideFenc;
        fref += strideFref;
    }

    sum = _mm_hadd_epi32(sum, sum);
    sum = _mm_hadd_epi32(sum, sum);
    return _mm_cvtsi128_si32(sum);
}

template<int ly>
int sse_pp64(pixel* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i sum = _mm_setzero_si128();
    __m128i zero = _mm_setzero_si128();

    for (; rows != 0; rows--)
    {
        __m128i m1 = _mm_loadu_si128((__m128i const*)(fenc));
        __m128i n1 = _mm_loadu_si128((__m128i const*)(fref));

        __m128i m1lo = _mm_cvtepu8_epi16(m1);
        __m128i m1hi = _mm_unpackhi_epi8(m1, zero);

        __m128i n1lo = _mm_cvtepu8_epi16(n1);
        __m128i n1hi = _mm_unpackhi_epi8(n1, zero);

        __m128i diff = _mm_sub_epi16(m1lo, n1lo);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        diff = _mm_sub_epi16(m1hi, n1hi);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        m1 = _mm_loadu_si128((__m128i const*)(fenc + 16));
        n1 = _mm_loadu_si128((__m128i const*)(fref + 16));

        m1lo = _mm_cvtepu8_epi16(m1);
        m1hi = _mm_unpackhi_epi8(m1, zero);

        n1lo = _mm_cvtepu8_epi16(n1);
        n1hi = _mm_unpackhi_epi8(n1, zero);

        diff = _mm_sub_epi16(m1lo, n1lo);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        diff = _mm_sub_epi16(m1hi, n1hi);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        m1 = _mm_loadu_si128((__m128i const*)(fenc + 32));
        n1 = _mm_loadu_si128((__m128i const*)(fref + 32));

        m1lo = _mm_cvtepu8_epi16(m1);
        m1hi = _mm_unpackhi_epi8(m1, zero);

        n1lo = _mm_cvtepu8_epi16(n1);
        n1hi = _mm_unpackhi_epi8(n1, zero);

        diff = _mm_sub_epi16(m1lo, n1lo);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        diff = _mm_sub_epi16(m1hi, n1hi);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        m1 = _mm_loadu_si128((__m128i const*)(fenc + 48));
        n1 = _mm_loadu_si128((__m128i const*)(fref + 48));

        m1lo = _mm_cvtepu8_epi16(m1);
        m1hi = _mm_unpackhi_epi8(m1, zero);

        n1lo = _mm_cvtepu8_epi16(n1);
        n1hi = _mm_unpackhi_epi8(n1, zero);

        diff = _mm_sub_epi16(m1lo, n1lo);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        diff = _mm_sub_epi16(m1hi, n1hi);
        sum = _mm_add_epi32(sum, _mm_madd_epi16(diff, diff));

        fenc += strideFenc;
        fref += strideFref;
    }

    sum = _mm_hadd_epi32(sum, sum);
    sum = _mm_hadd_epi32(sum, sum);
    return _mm_cvtsi128_si32(sum);
}

#endif /* if INSTRSET >= X265_CPU_LEVEL_SSE41 */

template<int ly>
int sse_ss4(short* fenc, intptr_t strideFenc, short* fref, intptr_t strideFref)
{
    int rows = ly;

    Vec4i diff(0);
    Vec8s m1, n1;
    Vec4i sum(0);
    for (; rows != 0; rows--)
    {
        m1.load(fenc);
        n1.load(fref);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        fenc += strideFenc;
        fref += strideFref;
    }

    return horizontal_add(sum);
}

template<int ly>
int sse_ss8(short* fenc, intptr_t strideFenc, short* fref, intptr_t strideFref)
{
    int rows = ly;

    Vec4i diff(0);
    Vec8s m1, n1;
    Vec4i sum(0);
    for (; rows != 0; rows--)
    {
        m1.load(fenc);
        n1.load(fref);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;

        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;
        fenc += strideFenc;
        fref += strideFref;
    }

    return horizontal_add(sum);
}

template<int ly>
int sse_ss12(short* fenc, intptr_t strideFenc, short* fref, intptr_t strideFref)
{
    int rows = ly;

    Vec4i diff(0);
    Vec8s m1, n1;
    Vec4i sum(0);
    for (; rows != 0; rows--)
    {
        m1.load(fenc);
        n1.load(fref);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(fenc + 8);
        n1.load(fref + 8);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;

        fenc += strideFenc;
        fref += strideFref;
    }

    return horizontal_add(sum);
}

template<int ly>
int sse_ss16(short* fenc, intptr_t strideFenc, short* fref, intptr_t strideFref)
{
    int rows = ly;

    Vec4i diff(0);
    Vec8s m1, n1;
    Vec4i sum(0);
    for (; rows != 0; rows--)
    {
        m1.load(fenc);
        n1.load(fref);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(fenc + 8);
        n1.load(fref + 8);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        fenc += strideFenc;
        fref += strideFref;
    }

    return horizontal_add(sum);
}

template<int ly>
int sse_ss24(short* fenc, intptr_t strideFenc, short* fref, intptr_t strideFref)
{
    int rows = ly;

    Vec4i diff(0);
    Vec8s m1, n1;
    Vec4i sum(0);
    for (; rows != 0; rows--)
    {
        m1.load(fenc);
        n1.load(fref);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(fenc + 8);
        n1.load(fref + 8);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(fenc + 16);
        n1.load(fref + 16);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        fenc += strideFenc;
        fref += strideFref;
    }

    return horizontal_add(sum);
}

template<int ly>
int sse_ss32(short* fenc, intptr_t strideFenc, short* fref, intptr_t strideFref)
{
    int rows = ly;

    Vec4i diff(0);
    Vec8s m1, n1;
    Vec4i sum(0);
    for (; rows != 0; rows--)
    {
        m1.load(fenc);
        n1.load(fref);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(fenc + 8);
        n1.load(fref + 8);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(fenc + 16);
        n1.load(fref + 16);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(fenc + 24);
        n1.load(fref + 24);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        fenc += strideFenc;
        fref += strideFref;
    }

    return horizontal_add(sum);
}

template<int ly>
int sse_ss48(short* fenc, intptr_t strideFenc, short* fref, intptr_t strideFref)
{
    int rows = ly;

    Vec4i diff(0);
    Vec8s m1, n1;
    Vec4i sum(0);
    for (; rows != 0; rows--)
    {
        m1.load(fenc);
        n1.load(fref);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(fenc + 8);
        n1.load(fref + 8);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(fenc + 16);
        n1.load(fref + 16);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(fenc + 24);
        n1.load(fref + 24);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(fenc + 32);
        n1.load(fref + 32);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(fenc + 40);
        n1.load(fref + 40);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        fenc += strideFenc;
        fref += strideFref;
    }

    return horizontal_add(sum);
}

template<int ly>
int sse_ss64(short* fenc, intptr_t strideFenc, short* fref, intptr_t strideFref)
{
    int rows = ly;

    Vec4i diff(0);
    Vec8s m1, n1;
    Vec4i sum(0);
    for (; rows != 0; rows--)
    {
        m1.load(fenc);
        n1.load(fref);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(fenc + 8);
        n1.load(fref + 8);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(fenc + 16);
        n1.load(fref + 16);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(fenc + 24);
        n1.load(fref + 24);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(fenc + 32);
        n1.load(fref + 32);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(fenc + 40);
        n1.load(fref + 40);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(fenc + 48);
        n1.load(fref + 48);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        m1.load(fenc + 56);
        n1.load(fref + 56);
        diff = extend_low(m1) - extend_low(n1);
        diff = diff * diff;
        sum += diff;
        diff = extend_high(m1) - extend_high(n1);
        diff = diff * diff;
        sum += diff;

        fenc += strideFenc;
        fref += strideFref;
    }

    return horizontal_add(sum);
}

template<int ly>
int sse_sp4(short* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    int rows = ly;
    Vec8s m1;
    Vec16uc n1;

    Vec4i diff_low(0);
    Vec4i sum_low(0);
    for (; rows != 0; rows--)
    {
        m1.load(fenc);
        n1.fromUint32(*(uint32_t*)fref);
        diff_low = extend_low(m1) - extend_low(extend_low(n1));
        diff_low = diff_low * diff_low;
        sum_low += diff_low;

        fenc += strideFenc;
        fref += strideFref;
    }

    return horizontal_add(sum_low);
}

template<int ly>
int sse_sp8(short* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    int rows = ly;
    Vec8s m1;
    Vec16uc n1;

    Vec8us diff_low(0), diff_high(0);
    Vec4i sum_low(0), sum_high(0);
    for (; rows != 0; rows--)
    {
        n1.load(fref);
        m1.load(fenc);
        diff_low = m1 - extend_low(n1);
        diff_low = diff_low * diff_low;
        sum_low += extend_low(diff_low);
        sum_high += extend_high(diff_low);

        fenc += strideFenc;
        fref += strideFref;
    }

    return horizontal_add(sum_low) + horizontal_add(sum_high);
}

template<int ly>
int sse_sp12(short* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    int rows = ly;
    Vec8s m1;
    Vec16uc n1;

    Vec8us diff_low(0);
    Vec4i diff_high(0);
    Vec4i sum_low(0), sum_high(0);
    for (; rows != 0; rows--)
    {
        n1.load(fref);
        n1.cutoff(12);
        m1.load(fenc);
        diff_low = m1 - extend_low(n1);
        m1.load(fenc + 8);
        diff_high = extend_low(m1) - extend_low(extend_high(n1));
        diff_low = diff_low * diff_low;
        diff_high = diff_high * diff_high;
        sum_low += extend_low(diff_low);
        sum_high += (extend_high(diff_low) + diff_high);

        fenc += strideFenc;
        fref += strideFref;
    }

    return horizontal_add(sum_low) + horizontal_add(sum_high);
}

template<int ly>
int sse_sp16(short* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    int rows = ly;
    Vec8s m1;
    Vec16uc n1;

    Vec8us diff_low(0), diff_high(0);
    Vec4i sum_low(0), sum_high(0);
    for (; rows != 0; rows--)
    {
        n1.load(fref);
        m1.load(fenc);
        diff_low = m1 - extend_low(n1);
        m1.load(fenc + 8);
        diff_high = m1 - extend_high(n1);
        diff_low = diff_low * diff_low;
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));

        fenc += strideFenc;
        fref += strideFref;
    }

    return horizontal_add(sum_low) + horizontal_add(sum_high);
}

template<int ly>
int sse_sp24(short* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    int rows = ly;
    Vec8s m1;
    Vec16uc n1;

    Vec8us diff_low(0), diff_high(0);
    Vec4i sum_low(0), sum_high(0);
    for (; rows != 0; rows--)
    {
        n1.load(fref);
        m1.load(fenc);
        diff_low = m1 - extend_low(n1);
        m1.load(fenc + 8);
        diff_high = m1 - extend_high(n1);
        diff_low = diff_low * diff_low;
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));

        n1.load(fref + 16);
        m1.load(fenc + 16);
        diff_low = m1 - extend_low(n1);
        diff_low = diff_low * diff_low;
        sum_low += extend_low(diff_low);
        sum_high += extend_high(diff_low);

        fenc += strideFenc;
        fref += strideFref;
    }

    return horizontal_add(sum_low) + horizontal_add(sum_high);
}

template<int ly>
int sse_sp32(short* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    int rows = ly;
    Vec8s m1;
    Vec16uc n1;

    Vec8us diff_low(0), diff_high(0);
    Vec4i sum_low(0), sum_high(0);
    for (; rows != 0; rows--)
    {
        n1.load(fref);
        m1.load(fenc);
        diff_low = m1 - extend_low(n1);
        m1.load(fenc + 8);
        diff_high = m1 - extend_high(n1);
        diff_low = diff_low * diff_low;
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));

        n1.load(fref + 16);
        m1.load(fenc + 16);
        diff_low = m1 - extend_low(n1);
        m1.load(fenc + 24);
        diff_high = m1 - extend_high(n1);
        diff_low = diff_low * diff_low;
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));

        fenc += strideFenc;
        fref += strideFref;
    }

    return horizontal_add(sum_low) + horizontal_add(sum_high);
}

template<int ly>
int sse_sp48(short* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    int rows = ly;
    Vec8s m1;
    Vec16uc n1;

    Vec8us diff_low(0), diff_high(0);
    Vec4i sum_low(0), sum_high(0);
    for (; rows != 0; rows--)
    {
        n1.load(fref);
        m1.load(fenc);
        diff_low = m1 - extend_low(n1);
        m1.load_a(fenc + 8);
        diff_high = m1 - extend_high(n1);
        diff_low = diff_low * diff_low;
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));

        n1.load(fref + 16);
        m1.load(fenc + 16);
        diff_low = m1 - extend_low(n1);
        m1.load(fenc + 24);
        diff_high = m1 - extend_high(n1);
        diff_low = diff_low * diff_low;
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));

        n1.load(fref + 32);
        m1.load(fenc + 32);
        diff_low = m1 - extend_low(n1);
        m1.load(fenc + 40);
        diff_high = m1 - extend_high(n1);
        diff_low = diff_low * diff_low;
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));

        fenc += strideFenc;
        fref += strideFref;
    }

    return horizontal_add(sum_low) + horizontal_add(sum_high);
}

template<int ly>
int sse_sp64(short* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    int rows = ly;
    Vec8s m1;
    Vec16uc n1;

    Vec8us diff_low(0), diff_high(0);
    Vec4i sum_low(0), sum_high(0);
    for (; rows != 0; rows--)
    {
        n1.load(fref);
        m1.load(fenc);
        diff_low = m1 - extend_low(n1);
        m1.load(fenc + 8);
        diff_high = m1 - extend_high(n1);
        diff_low = diff_low * diff_low;
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));

        n1.load(fref + 16);
        m1.load(fenc + 16);
        diff_low = m1 - extend_low(n1);
        m1.load(fenc + 24);
        diff_high = m1 - extend_high(n1);
        diff_low = diff_low * diff_low;
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));

        n1.load(fref + 32);
        m1.load(fenc + 32);
        diff_low = m1 - extend_low(n1);
        m1.load(fenc + 40);
        diff_high = m1 - extend_high(n1);
        diff_low = diff_low * diff_low;
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));

        n1.load(fref + 48);
        m1.load(fenc + 48);
        diff_low = m1 - extend_low(n1);
        m1.load(fenc + 56);
        diff_high = m1 - extend_high(n1);
        diff_low = diff_low * diff_low;
        diff_high = diff_high * diff_high;
        sum_low += (extend_low(diff_low) + extend_low(diff_high));
        sum_high += (extend_high(diff_low) + extend_high(diff_high));

        fenc += strideFenc;
        fref += strideFref;
    }

    return horizontal_add(sum_low) + horizontal_add(sum_high);
}
