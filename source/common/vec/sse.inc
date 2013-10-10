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

#define PROCESS_SSE_SS4x1(BASE)\
    m1 = _mm_loadu_si128((__m128i const*)(fenc + BASE)); \
    n1 = _mm_loadu_si128((__m128i const*)(fref + BASE)); \
    sign1 = _mm_srai_epi16(m1, 15); \
    tmp1 = _mm_unpacklo_epi16(m1, sign1); \
    sign2 = _mm_srai_epi16(n1, 15); \
    tmp2 = _mm_unpacklo_epi16(n1, sign2); \
    diff = _mm_sub_epi32(tmp1, tmp2); \
    diff = _mm_mullo_epi32(diff, diff); \
    sum = _mm_add_epi32(sum, diff)

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

template<int ly>
int sse_ss4(short* fenc, intptr_t strideFenc, short* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i diff;
    __m128i sum  = _mm_setzero_si128();
    __m128i m1, n1, sign1, sign2, tmp1, tmp2;

    for (; rows != 0; rows--)
    {
        PROCESS_SSE_SS4x1(0);

        fenc += strideFenc;
        fref += strideFref;
    }
    __m128i sum1  = _mm_hadd_epi32(sum, sum);         // horizontally add 4 elements
    return _mm_cvtsi128_si32(_mm_hadd_epi32(sum1, sum1));
}

template<int ly>
int sse_ss8(short* fenc, intptr_t strideFenc, short* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i sum  = _mm_setzero_si128();
    __m128i m1, n1, diff, sign1, sign2, tmp1, tmp2;

    for (; rows != 0; rows--)
    {
        PROCESS_SSE_SS4x1(0);

        m1 = _mm_unpackhi_epi16(m1, sign1);
        n1 = _mm_unpackhi_epi16(n1, sign2);
        diff = _mm_sub_epi32(m1, n1);
        diff = _mm_mullo_epi32(diff, diff);
        sum = _mm_add_epi32(sum, diff);

        fenc += strideFenc;
        fref += strideFref;
    }
    __m128i sum1  = _mm_hadd_epi32(sum, sum);
    return _mm_cvtsi128_si32(_mm_hadd_epi32(sum1, sum1));
}

template<int ly>
int sse_ss12(short* fenc, intptr_t strideFenc, short* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i sum  = _mm_setzero_si128();
    __m128i m1, n1, diff, sign1, sign2, tmp1, tmp2;

    for (; rows != 0; rows--)
    {
        PROCESS_SSE_SS4x1(0);

        m1 = _mm_unpackhi_epi16(m1, sign1);
        n1 = _mm_unpackhi_epi16(n1, sign2);
        diff = _mm_sub_epi32(m1, n1);
        diff = _mm_mullo_epi32(diff, diff);
        sum = _mm_add_epi32(sum, diff);

        PROCESS_SSE_SS4x1(8);

        fenc += strideFenc;
        fref += strideFref;
    }
    __m128i sum1  = _mm_hadd_epi32(sum, sum);
    return _mm_cvtsi128_si32(_mm_hadd_epi32(sum1, sum1));
}

template<int ly>
int sse_ss16(short* fenc, intptr_t strideFenc, short* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i sum  = _mm_setzero_si128();
    __m128i m1, n1, diff, sign1, sign2, tmp1, tmp2;

    for (; rows != 0; rows--)
    {
        for(int i = 0; i < 16; i += 8)
        {
            PROCESS_SSE_SS4x1(i);

            m1 = _mm_unpackhi_epi16(m1, sign1);
            n1 = _mm_unpackhi_epi16(n1, sign2);
            diff = _mm_sub_epi32(m1, n1);
            diff = _mm_mullo_epi32(diff, diff);
            sum = _mm_add_epi32(sum, diff);
        }
        fenc += strideFenc;
        fref += strideFref;
    }
    __m128i sum1  = _mm_hadd_epi32(sum, sum);
    return _mm_cvtsi128_si32(_mm_hadd_epi32(sum1, sum1));
}

template<int ly>
int sse_ss24(short* fenc, intptr_t strideFenc, short* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i sum  = _mm_setzero_si128();
    __m128i m1, n1, diff, sign1, sign2, tmp1, tmp2;

    for (; rows != 0; rows--)
    {
        for(int i = 0; i < 24; i += 8)
        {
            PROCESS_SSE_SS4x1(i);

            m1 = _mm_unpackhi_epi16(m1, sign1);
            n1 = _mm_unpackhi_epi16(n1, sign2);
            diff = _mm_sub_epi32(m1, n1);
            diff = _mm_mullo_epi32(diff, diff);
            sum = _mm_add_epi32(sum, diff);
        }
        fenc += strideFenc;
        fref += strideFref;
    }
    __m128i sum1  = _mm_hadd_epi32(sum, sum);
    return _mm_cvtsi128_si32(_mm_hadd_epi32(sum1, sum1));
}

template<int ly>
int sse_ss32(short* fenc, intptr_t strideFenc, short* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i sum  = _mm_setzero_si128();
    __m128i m1, n1, diff, sign1, sign2, tmp1, tmp2;

    for (; rows != 0; rows--)
    {
        for(int i = 0; i < 32; i += 8)
        {
            PROCESS_SSE_SS4x1(i);

            m1 = _mm_unpackhi_epi16(m1, sign1);
            n1 = _mm_unpackhi_epi16(n1, sign2);
            diff = _mm_sub_epi32(m1, n1);
            diff = _mm_mullo_epi32(diff, diff);
            sum = _mm_add_epi32(sum, diff);
        }
        fenc += strideFenc;
        fref += strideFref;
    }
    __m128i sum1  = _mm_hadd_epi32(sum, sum);
    return _mm_cvtsi128_si32(_mm_hadd_epi32(sum1, sum1));
}

template<int ly>
int sse_ss48(short* fenc, intptr_t strideFenc, short* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i sum  = _mm_setzero_si128();
    __m128i m1, n1, diff, sign1, sign2, tmp1, tmp2;

    for (; rows != 0; rows--)
    {
        for(int i = 0; i < 48; i += 8)
        {
            PROCESS_SSE_SS4x1(i);

            m1 = _mm_unpackhi_epi16(m1, sign1);
            n1 = _mm_unpackhi_epi16(n1, sign2);
            diff = _mm_sub_epi32(m1, n1);
            diff = _mm_mullo_epi32(diff, diff);
            sum = _mm_add_epi32(sum, diff);
        }
        fenc += strideFenc;
        fref += strideFref;
    }
    __m128i sum1  = _mm_hadd_epi32(sum, sum);
    return _mm_cvtsi128_si32(_mm_hadd_epi32(sum1, sum1));
}

template<int ly>
int sse_ss64(short* fenc, intptr_t strideFenc, short* fref, intptr_t strideFref)
{
    int rows = ly;
    __m128i sum  = _mm_setzero_si128();
    __m128i m1, n1, diff, sign1, sign2, tmp1, tmp2;

    for (; rows != 0; rows--)
    {
        for(int i = 0; i < 64; i += 8)
        {
            PROCESS_SSE_SS4x1(i);

            m1 = _mm_unpackhi_epi16(m1, sign1);
            n1 = _mm_unpackhi_epi16(n1, sign2);
            diff = _mm_sub_epi32(m1, n1);
            diff = _mm_mullo_epi32(diff, diff);
            sum = _mm_add_epi32(sum, diff);
        }
        fenc += strideFenc;
        fref += strideFref;
    }
    __m128i sum1  = _mm_hadd_epi32(sum, sum);
    return _mm_cvtsi128_si32(_mm_hadd_epi32(sum1, sum1));
}

template<int ly>
int sse_sp4(short* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    __m128i sum = _mm_setzero_si128();

    for(int i = 0; i < ly; i++)
    {
        __m128i T00, T01, T02, T03;
        T00 = _mm_loadu_si128((__m128i*)(fenc));
        T01 = _mm_cvtsi32_si128(*(uint32_t*)(fref));
        T00 = _mm_unpacklo_epi16(T00, _mm_setzero_si128());
        T01 = _mm_unpacklo_epi8(T01, _mm_setzero_si128());
        T01 = _mm_unpacklo_epi16(T01, _mm_setzero_si128());
        T02 = _mm_sub_epi32(T00, T01);
        T03 = _mm_mullo_epi32(T02, T02);
        sum = _mm_add_epi32(sum, T03);

        fenc += strideFenc;
        fref += strideFref;
    }
    sum = _mm_hadd_epi32(sum, _mm_setzero_si128());
    sum = _mm_hadd_epi32(sum, _mm_setzero_si128());

    return _mm_cvtsi128_si32(sum);
}

#define SSE_SP8x1 \
    T10 = _mm_unpacklo_epi16(T00, _mm_setzero_si128()); \
    T11 = _mm_unpacklo_epi16(T02, _mm_setzero_si128()); \
    T12 = _mm_sub_epi32(T10, T11); \
    T13 = _mm_mullo_epi32(T12, T12); \
    sum0 = _mm_add_epi32(sum0, T13); \
    T10 = _mm_unpackhi_epi16(T00, _mm_setzero_si128()); \
    T11 = _mm_unpackhi_epi16(T02, _mm_setzero_si128()); \
    T12 = _mm_sub_epi32(T10, T11); \
    T13 = _mm_mullo_epi32(T12, T12); \
    sum1 = _mm_add_epi32(sum1, T13)

template<int ly>
int sse_sp8(short* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    __m128i sum0 = _mm_setzero_si128();
    __m128i sum1 = _mm_setzero_si128();

    for(int i = 0; i < ly; i++)
    {
        __m128i T00, T01, T02;
        __m128i T10, T11, T12, T13;

        T00 = _mm_loadu_si128((__m128i*)(fenc));
        T01 = _mm_loadu_si128((__m128i*)(fref));
        T02 = _mm_unpacklo_epi8(T01, _mm_setzero_si128());

        SSE_SP8x1;

        fenc += strideFenc;
        fref += strideFref;
    }
    sum0 = _mm_add_epi32(sum0, sum1);

    sum0 = _mm_hadd_epi32(sum0, _mm_setzero_si128());
    sum0 = _mm_hadd_epi32(sum0, _mm_setzero_si128());

    return _mm_cvtsi128_si32(sum0);
}

template<int ly>
int sse_sp12(short* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    __m128i sum0 = _mm_setzero_si128();

    for(int i = 0; i < ly; i++)
    {
        __m128i T00, T01;
        __m128i T10, T11, T12, T13;
        T00 = _mm_loadu_si128((__m128i*)(fenc));
        T01 = _mm_loadu_si128((__m128i*)(fref));
        T01 = _mm_srli_si128(_mm_slli_si128(T01, 4), 4);    //masking last 4 8-bit integers

        T10 = _mm_unpacklo_epi16(T00, _mm_setzero_si128());
        T11 = _mm_unpacklo_epi8(T01, _mm_setzero_si128());
        T11 = _mm_unpacklo_epi16(T11, _mm_setzero_si128());
        T12 = _mm_sub_epi32(T10, T11);
        T13 = _mm_mullo_epi32(T12, T12);
        sum0 = _mm_add_epi32(sum0, T13);

        T10 = _mm_unpackhi_epi16(T00, _mm_setzero_si128());
        T11 = _mm_unpacklo_epi8(T01, _mm_setzero_si128());
        T11 = _mm_unpackhi_epi16(T11, _mm_setzero_si128());
        T12 = _mm_sub_epi32(T10, T11);
        T13 = _mm_mullo_epi32(T12, T12);
        sum0 = _mm_add_epi32(sum0, T13);

        T00 = _mm_loadu_si128((__m128i*)(fenc + 8));

        T10 = _mm_unpacklo_epi16(T00, _mm_setzero_si128());
        T11 = _mm_unpackhi_epi8(T01, _mm_setzero_si128());
        T11 = _mm_unpacklo_epi16(T11, _mm_setzero_si128());
        T12 = _mm_sub_epi32(T10, T11);
        T13 = _mm_mullo_epi32(T12, T12);
        sum0 = _mm_add_epi32(sum0, T13);

        fenc += strideFenc;
        fref += strideFref;
    }
    sum0 = _mm_hadd_epi32(sum0, _mm_setzero_si128());
    sum0 = _mm_hadd_epi32(sum0, _mm_setzero_si128());

    return _mm_cvtsi128_si32(sum0);
}

template<int ly>
int sse_sp16(short* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    __m128i sum0 = _mm_setzero_si128();
    __m128i sum1 = _mm_setzero_si128();

    for(int i = 0; i < ly; i++)
    {
        __m128i T00, T01, T02;
        __m128i T10, T11, T12, T13;

        T00 = _mm_loadu_si128((__m128i*)(fenc));
        T01 = _mm_loadu_si128((__m128i*)(fref));
        T02 = _mm_unpacklo_epi8(T01, _mm_setzero_si128());

        SSE_SP8x1;

        T00 = _mm_loadu_si128((__m128i*)(fenc + 8));
        T02 = _mm_unpackhi_epi8(T01, _mm_setzero_si128());

        SSE_SP8x1;

        fenc += strideFenc;
        fref += strideFref;
    }
    sum0 = _mm_add_epi32(sum0, sum1);

    sum0 = _mm_hadd_epi32(sum0, _mm_setzero_si128());
    sum0 = _mm_hadd_epi32(sum0, _mm_setzero_si128());

    return _mm_cvtsi128_si32(sum0);
}

template<int ly>
int sse_sp24(short* fenc, intptr_t strideFenc, pixel* fref, intptr_t strideFref)
{
    __m128i sum0 = _mm_setzero_si128();
    __m128i sum1 = _mm_setzero_si128();

    for(int i = 0; i < ly; i++)
    {
        __m128i T00, T01, T02;
        __m128i T10, T11, T12, T13;

        T00 = _mm_loadu_si128((__m128i*)(fenc));
        T01 = _mm_loadu_si128((__m128i*)(fref));
        T02 = _mm_unpacklo_epi8(T01, _mm_setzero_si128());

        SSE_SP8x1;

        T00 = _mm_loadu_si128((__m128i*)(fenc + 8));
        T02 = _mm_unpackhi_epi8(T01, _mm_setzero_si128());

        SSE_SP8x1;

        T00 = _mm_loadu_si128((__m128i*)(fenc + 16));
        T01 = _mm_loadu_si128((__m128i*)(fref + 16));
        T02 = _mm_unpacklo_epi8(T01, _mm_setzero_si128());

        SSE_SP8x1;

        fenc += strideFenc;
        fref += strideFref;
    }
    sum0 = _mm_add_epi32(sum0, sum1);

    sum0 = _mm_hadd_epi32(sum0, _mm_setzero_si128());
    sum0 = _mm_hadd_epi32(sum0, _mm_setzero_si128());

    return _mm_cvtsi128_si32(sum0);
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
