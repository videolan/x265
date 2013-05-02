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

#if HIGH_BIT_DEPTH

/* intrinsics for when pixel type is short */

template<int ly>
int CDECL sad_4(pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    Vec8s sum(0);
    for (int row = 0; row < ly; row++)
    {
        Vec8s m1, n1;
        m1.load(piOrg);
        n1.load(piCur);
        sum += abs(m1 - n1);

        piOrg += strideOrg;
        piCur += strideCur;
    }

    return horizontal_add(extend_low(sum));
}

template<int ly>
int CDECL sad_8(pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    Vec8s sum(0);
    for (int row = 0; row < ly; row++)
    {
        Vec8s m1, n1;
        m1.load_a(piOrg);
        n1.load(piCur);
        sum += abs(m1 - n1);

        piOrg += strideOrg;
        piCur += strideCur;
    }
    return horizontal_add_x(sum);
}

template<int ly>
int CDECL sad_16(pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    Vec16s sum(0);
    for (int row = 0; row < ly; row++)
    {
        Vec16s m1, n1;
        m1.load_a(piOrg);
        n1.load(piCur);
        sum += abs(m1 - n1);

        piOrg += strideOrg;
        piCur += strideCur;
    }

    return horizontal_add(extend_low(sum)) +
           horizontal_add(extend_high(sum));
}

template<int lx, int ly>
int CDECL sad_32(pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    Vec16s sum(0);
    for (int row = 0; row < ly; row++)
    {
        for (int col = 0; col < lx; col += 32)
        {
            Vec16s m1, n1;
            m1.load(piOrg + col);
            n1.load(piCur + col);
            sum += abs(m1 - n1);
            Vec16s m2, n2;
            m2.load(piOrg + col + 16);
            n2.load(piCur + col + 16);
            sum += abs(m2 - n2);
        }

        piOrg += strideOrg;
        piCur += strideCur;
    }
    return horizontal_add_x(extend_low(sum)) +
           horizontal_add_x(extend_high(sum));
}

template<int lx, int ly>
int CDECL sad_16x24(pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    int sum = 0;
    for (int row = 0; row < ly; row += 24)
    {
        for (int col = 0; col < lx; col += 16)
        {
            sum += sad_16<24>(piOrg + row * strideOrg + col, strideOrg,
                              piCur + row * strideCur + col, strideCur);
        }
    }
    return sum;
}

/* For performance - This function assumes that the *last load* can access 8 elements. */
template<int lx, int ly>
void CDECL sad_4_x3(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, int *res)
{    
    Vec8s m1, n1, n2, n3;
    Vec4i sum1_low(0);
    Vec4i sum2_low(0);
    Vec4i sum3_low(0);
    int row;
    res[0] = 0;
    res[1] = 0;
    res[2] = 0;
    for (row = 0; row < ly-7; row+=8)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        for(int temp_rows = 0; temp_rows < 8; temp_rows++)
        {
            for (int col = 0; col < lx; col += 4)
            {
                m1.load_a(piOrg + col);
                n1.load(piCur1 + col);
                n2.load(piCur2 + col);
                n3.load(piCur3 + col);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
            }
            piOrg += FENC_STRIDE;
            piCur1 += FENC_STRIDE;
            piCur2 += FENC_STRIDE;
            piCur3 += FENC_STRIDE;
        }
        sum1_low += extend_low(sum1);
        sum2_low += extend_low(sum2);
        sum3_low += extend_low(sum3);
    }
    for (; row < ly; row++)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        for (int col = 0; col < lx; col += 4)
        {
            m1.load_a(piOrg + col);
            n1.load(piCur1 + col);
            n2.load(piCur2 + col);
            n3.load(piCur3 + col);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
        }
        sum1_low += extend_low(sum1);
        sum2_low += extend_low(sum2);
        sum3_low += extend_low(sum3);
        piOrg += FENC_STRIDE;
        piCur1 += FENC_STRIDE;
        piCur2 += FENC_STRIDE;
        piCur3 += FENC_STRIDE;
    }
    res[0] += horizontal_add_x(sum1_low);
    res[1] += horizontal_add_x(sum2_low);
    res[2] += horizontal_add_x(sum3_low);
}

/* For performance - This function assumes that the *last load* can access 8 elements. */
template<int lx, int ly>
void CDECL sad_4_4_x3(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, int *res)
{    
    Vec8s m1, n1, n2, n3;
    Vec4i sum1_low(0);
    Vec4i sum2_low(0);
    Vec4i sum3_low(0);
    int row;
    res[0] = 0;
    res[1] = 0;
    res[2] = 0;
    for (row = 0; row < ly-3; row+=4)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        for(int temp_rows = 0; temp_rows < 4; temp_rows++)
        {
            for (int col = 0; col < lx; col += 4)
            {
                m1.load_a(piOrg + col);
                n1.load(piCur1 + col);
                n2.load(piCur2 + col);
                n3.load(piCur3 + col);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
            }
            piOrg += FENC_STRIDE;
            piCur1 += FENC_STRIDE;
            piCur2 += FENC_STRIDE;
            piCur3 += FENC_STRIDE;
        }
        sum1_low += extend_low(sum1);
        sum2_low += extend_low(sum2);
        sum3_low += extend_low(sum3);
    }
    for (; row < ly; row++)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        for (int col = 0; col < lx; col += 4)
        {
            m1.load_a(piOrg + col);
            n1.load(piCur1 + col);
            n2.load(piCur2 + col);
            n3.load(piCur3 + col);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
        }
        sum1_low += extend_low(sum1);
        sum2_low += extend_low(sum2);
        sum3_low += extend_low(sum3);
        piOrg += FENC_STRIDE;
        piCur1 += FENC_STRIDE;
        piCur2 += FENC_STRIDE;
        piCur3 += FENC_STRIDE;
    }
    res[0] += horizontal_add_x(sum1_low);
    res[1] += horizontal_add_x(sum2_low);
    res[2] += horizontal_add_x(sum3_low);
}

template<int lx, int ly>
void CDECL sad_8_x3(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, int *res)
{    
    Vec8s m1, n1, n2, n3;
    Vec4i sum1_low(0), sum1_high(0);
    Vec4i sum2_low(0), sum2_high(0);
    Vec4i sum3_low(0), sum3_high(0);
    int row;
    res[0] = 0;
    res[1] = 0;
    res[2] = 0;
    for (row = 0; row < ly-7; row+=8)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        for(int temp_rows = 0; temp_rows < 8; temp_rows++)
        {
            for (int col = 0; col < lx; col += 8)
            {
                m1.load_a(piOrg + col);
                n1.load(piCur1 + col);
                n2.load(piCur2 + col);
                n3.load(piCur3 + col);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
            }
            piOrg += FENC_STRIDE;
            piCur1 += FENC_STRIDE;
            piCur2 += FENC_STRIDE;
            piCur3 += FENC_STRIDE;
        }
        sum1_low += extend_low(sum1);
        sum1_high += extend_high(sum1);
        sum2_low += extend_low(sum2);
        sum2_high += extend_high(sum2);
        sum3_low += extend_low(sum3);
        sum3_high += extend_high(sum3);
    }
    for (; row < ly; row++)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        for (int col = 0; col < lx; col += 8)
        {
            m1.load_a(piOrg + col);
            n1.load(piCur1 + col);
            n2.load(piCur2 + col);
            n3.load(piCur3 + col);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
        }
        sum1_low += extend_low(sum1);
        sum1_high += extend_high(sum1);
        sum2_low += extend_low(sum2);
        sum2_high += extend_high(sum2);
        sum3_low += extend_low(sum3);
        sum3_high += extend_high(sum3);

        piOrg += FENC_STRIDE;
        piCur1 += FENC_STRIDE;
        piCur2 += FENC_STRIDE;
        piCur3 += FENC_STRIDE;
    }
    res[0] += horizontal_add_x(sum1_low) + horizontal_add_x(sum1_high);
    res[1] += horizontal_add_x(sum2_low) + horizontal_add_x(sum2_high);
    res[2] += horizontal_add_x(sum3_low) + horizontal_add_x(sum3_high);
}

template<int lx, int ly>
void CDECL sad_8_4_x3(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, int *res)
{    
    Vec8s m1, n1, n2, n3;
    Vec4i sum1_low(0), sum1_high(0);
    Vec4i sum2_low(0), sum2_high(0);
    Vec4i sum3_low(0), sum3_high(0);
    int row;
    res[0] = 0;
    res[1] = 0;
    res[2] = 0;
    for (row = 0; row < ly-3; row+=4)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        for(int temp_rows = 0; temp_rows < 4; temp_rows++)
        {
            for (int col = 0; col < lx; col += 8)
            {
                m1.load_a(piOrg + col);
                n1.load(piCur1 + col);
                n2.load(piCur2 + col);
                n3.load(piCur3 + col);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
            }
            piOrg += FENC_STRIDE;
            piCur1 += FENC_STRIDE;
            piCur2 += FENC_STRIDE;
            piCur3 += FENC_STRIDE;
        }
        sum1_low += extend_low(sum1);
        sum1_high += extend_high(sum1);
        sum2_low += extend_low(sum2);
        sum2_high += extend_high(sum2);
        sum3_low += extend_low(sum3);
        sum3_high += extend_high(sum3);
    }
    for (; row < ly; row++)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        for (int col = 0; col < lx; col += 8)
        {
            m1.load_a(piOrg + col);
            n1.load(piCur1 + col);
            n2.load(piCur2 + col);
            n3.load(piCur3 + col);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
        }
        sum1_low += extend_low(sum1);
        sum1_high += extend_high(sum1);
        sum2_low += extend_low(sum2);
        sum2_high += extend_high(sum2);
        sum3_low += extend_low(sum3);
        sum3_high += extend_high(sum3);

        piOrg += FENC_STRIDE;
        piCur1 += FENC_STRIDE;
        piCur2 += FENC_STRIDE;
        piCur3 += FENC_STRIDE;
    }
    res[0] += horizontal_add_x(sum1_low) + horizontal_add_x(sum1_high);
    res[1] += horizontal_add_x(sum2_low) + horizontal_add_x(sum2_high);
    res[2] += horizontal_add_x(sum3_low) + horizontal_add_x(sum3_high);
}

/* For performance - This function assumes that the *last load* can access 8 elements. */
template<int lx, int ly>
void CDECL sad_12_x3(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, int *res)
{    
    Vec8s m1, n1, n2, n3;
    Vec4i sum1_low(0);
    Vec4i sum2_low(0);
    Vec4i sum3_low(0);
    int row;
    res[0] = 0;
    res[1] = 0;
    res[2] = 0;
    for (row = 0; row < ly-7; row+=8)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        for(int temp_rows = 0; temp_rows < 8; temp_rows++)
        {
            for (int col = 0; col < lx; col += 12)
            {
                m1.load_a(piOrg + col);
                n1.load(piCur1 + col);
                n2.load(piCur2 + col);
                n3.load(piCur3 + col);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);

                m1.load(piOrg + col + 4);
                n1.load(piCur1 + col + 4);
                n2.load(piCur2 + col + 4);
                n3.load(piCur3 + col + 4);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);

                m1.load_a(piOrg + col + 8);
                n1.load(piCur1 + col + 8);
                n2.load(piCur2 + col + 8);
                n3.load(piCur3 + col + 8);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
            }
            piOrg += FENC_STRIDE;
            piCur1 += FENC_STRIDE;
            piCur2 += FENC_STRIDE;
            piCur3 += FENC_STRIDE;
        }
        sum1_low += extend_low(sum1);
        sum2_low += extend_low(sum2);
        sum3_low += extend_low(sum3);
    }
    for (; row < ly; row++)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        for (int col = 0; col < lx; col += 12)
        {
            m1.load_a(piOrg + col);
            n1.load(piCur1 + col);
            n2.load(piCur2 + col);
            n3.load(piCur3 + col);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);

            m1.load_a(piOrg + col + 4);
            n1.load(piCur1 + col + 4);
            n2.load(piCur2 + col + 4);
            n3.load(piCur3 + col + 4);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);

            m1.load_a(piOrg + col + 8);
            n1.load(piCur1 + col + 8);
            n2.load(piCur2 + col + 8);
            n3.load(piCur3 + col + 8);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
        }
        sum1_low += extend_low(sum1);
        sum2_low += extend_low(sum2);
        sum3_low += extend_low(sum3);
        piOrg += FENC_STRIDE;
        piCur1 += FENC_STRIDE;
        piCur2 += FENC_STRIDE;
        piCur3 += FENC_STRIDE;
    }
    res[0] += horizontal_add_x(sum1_low);
    res[1] += horizontal_add_x(sum2_low);
    res[2] += horizontal_add_x(sum3_low);
}

/* For performance - This function assumes that the *last load* can access 8 elements. */
template<int lx, int ly>
void CDECL sad_12_4_x3(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, int *res)
{    
    Vec8s m1, n1, n2, n3;
    Vec4i sum1_low(0);
    Vec4i sum2_low(0);
    Vec4i sum3_low(0);
    int row;
    res[0] = 0;
    res[1] = 0;
    res[2] = 0;
    for (row = 0; row < ly-3; row+=4)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        for(int temp_rows = 0; temp_rows < 4; temp_rows++)
        {
            for (int col = 0; col < lx; col += 12)
            {
                m1.load_a(piOrg + col);
                n1.load(piCur1 + col);
                n2.load(piCur2 + col);
                n3.load(piCur3 + col);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);

                m1.load(piOrg + col + 4);
                n1.load(piCur1 + col + 4);
                n2.load(piCur2 + col + 4);
                n3.load(piCur3 + col + 4);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);

                m1.load_a(piOrg + col + 8);
                n1.load(piCur1 + col + 8);
                n2.load(piCur2 + col + 8);
                n3.load(piCur3 + col + 8);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
            }
            piOrg += FENC_STRIDE;
            piCur1 += FENC_STRIDE;
            piCur2 += FENC_STRIDE;
            piCur3 += FENC_STRIDE;
        }
        sum1_low += extend_low(sum1);
        sum2_low += extend_low(sum2);
        sum3_low += extend_low(sum3);
    }
    for (; row < ly; row++)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        for (int col = 0; col < lx; col += 12)
        {
            m1.load_a(piOrg + col);
            n1.load(piCur1 + col);
            n2.load(piCur2 + col);
            n3.load(piCur3 + col);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);

            m1.load_a(piOrg + col + 4);
            n1.load(piCur1 + col + 4);
            n2.load(piCur2 + col + 4);
            n3.load(piCur3 + col + 4);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);

            m1.load_a(piOrg + col + 8);
            n1.load(piCur1 + col + 8);
            n2.load(piCur2 + col + 8);
            n3.load(piCur3 + col + 8);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
        }
        sum1_low += extend_low(sum1);
        sum2_low += extend_low(sum2);
        sum3_low += extend_low(sum3);
        piOrg += FENC_STRIDE;
        piCur1 += FENC_STRIDE;
        piCur2 += FENC_STRIDE;
        piCur3 += FENC_STRIDE;
    }
    res[0] += horizontal_add_x(sum1_low);
    res[1] += horizontal_add_x(sum2_low);
    res[2] += horizontal_add_x(sum3_low);
}

template<int lx, int ly>
void CDECL sad_16_x3(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, int *res)
{    
    Vec8s m1, n1, n2, n3;
    Vec4i sum1_low(0), sum1_high(0);
    Vec4i sum2_low(0), sum2_high(0);
    Vec4i sum3_low(0), sum3_high(0);
    int row;
    res[0] = 0;
    res[1] = 0;
    res[2] = 0;
    for (row = 0; row < ly-7; row+=8)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        for(int temp_rows = 0; temp_rows < 8; temp_rows++)
        {
            for (int col = 0; col < lx; col += 16)
            {
                m1.load_a(piOrg + col);
                n1.load(piCur1 + col);
                n2.load(piCur2 + col);
                n3.load(piCur3 + col);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);

                m1.load_a(piOrg + col + 8);
                n1.load(piCur1 + col + 8);
                n2.load(piCur2 + col + 8);
                n3.load(piCur3 + col + 8);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
            }
            piOrg += FENC_STRIDE;
            piCur1 += FENC_STRIDE;
            piCur2 += FENC_STRIDE;
            piCur3 += FENC_STRIDE;
        }
        sum1_low += extend_low(sum1);
        sum1_high += extend_high(sum1);
        sum2_low += extend_low(sum2);
        sum2_high += extend_high(sum2);
        sum3_low += extend_low(sum3);
        sum3_high += extend_high(sum3);
    }
    for (; row < ly; row++)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        for (int col = 0; col < lx; col += 16)
        {
            m1.load_a(piOrg + col);
            n1.load(piCur1 + col);
            n2.load(piCur2 + col);
            n3.load(piCur3 + col);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);

            m1.load_a(piOrg + col + 8);
            n1.load(piCur1 + col + 8);
            n2.load(piCur2 + col + 8);
            n3.load(piCur3 + col + 8);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
        }
        sum1_low += extend_low(sum1);
        sum1_high += extend_high(sum1);
        sum2_low += extend_low(sum2);
        sum2_high += extend_high(sum2);
        sum3_low += extend_low(sum3);
        sum3_high += extend_high(sum3);

        piOrg += FENC_STRIDE;
        piCur1 += FENC_STRIDE;
        piCur2 += FENC_STRIDE;
        piCur3 += FENC_STRIDE;
    }
    res[0] += horizontal_add_x(sum1_low) + horizontal_add_x(sum1_high);
    res[1] += horizontal_add_x(sum2_low) + horizontal_add_x(sum2_high);
    res[2] += horizontal_add_x(sum3_low) + horizontal_add_x(sum3_high);
}

template<int lx, int ly>
void CDECL sad_16_4_x3(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, int *res)
{    
    Vec8s m1, n1, n2, n3;
    Vec4i sum1_low(0), sum1_high(0);
    Vec4i sum2_low(0), sum2_high(0);
    Vec4i sum3_low(0), sum3_high(0);
    int row;
    res[0] = 0;
    res[1] = 0;
    res[2] = 0;
    for (row = 0; row < ly-3; row+=4)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        for(int temp_rows = 0; temp_rows < 4; temp_rows++)
        {
            for (int col = 0; col < lx; col += 16)
            {
                m1.load_a(piOrg + col);
                n1.load(piCur1 + col);
                n2.load(piCur2 + col);
                n3.load(piCur3 + col);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);

                m1.load_a(piOrg + col + 8);
                n1.load(piCur1 + col + 8);
                n2.load(piCur2 + col + 8);
                n3.load(piCur3 + col + 8);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
            }
            piOrg += FENC_STRIDE;
            piCur1 += FENC_STRIDE;
            piCur2 += FENC_STRIDE;
            piCur3 += FENC_STRIDE;
        }
        sum1_low += extend_low(sum1);
        sum1_high += extend_high(sum1);
        sum2_low += extend_low(sum2);
        sum2_high += extend_high(sum2);
        sum3_low += extend_low(sum3);
        sum3_high += extend_high(sum3);
    }
    for (; row < ly; row++)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        for (int col = 0; col < lx; col += 16)
        {
            m1.load_a(piOrg + col);
            n1.load(piCur1 + col);
            n2.load(piCur2 + col);
            n3.load(piCur3 + col);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);

            m1.load_a(piOrg + col + 8);
            n1.load(piCur1 + col + 8);
            n2.load(piCur2 + col + 8);
            n3.load(piCur3 + col + 8);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
        }
        sum1_low += extend_low(sum1);
        sum1_high += extend_high(sum1);
        sum2_low += extend_low(sum2);
        sum2_high += extend_high(sum2);
        sum3_low += extend_low(sum3);
        sum3_high += extend_high(sum3);

        piOrg += FENC_STRIDE;
        piCur1 += FENC_STRIDE;
        piCur2 += FENC_STRIDE;
        piCur3 += FENC_STRIDE;
    }
    res[0] += horizontal_add_x(sum1_low) + horizontal_add_x(sum1_high);
    res[1] += horizontal_add_x(sum2_low) + horizontal_add_x(sum2_high);
    res[2] += horizontal_add_x(sum3_low) + horizontal_add_x(sum3_high);
}

template<int lx, int ly>
void CDECL sad_32_x3(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, int *res)
{    
    Vec8s m1, n1, n2, n3;
    Vec4i sum1_low(0), sum1_high(0);
    Vec4i sum2_low(0), sum2_high(0);
    Vec4i sum3_low(0), sum3_high(0);
    int row;
    res[0] = 0;
    res[1] = 0;
    res[2] = 0;
    for (row = 0; row < ly-3; row+=4)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        for(int temp_rows = 0; temp_rows < 4; temp_rows++)
        {
            for (int col = 0; col < lx; col += 32)
            {
                m1.load_a(piOrg + col);
                n1.load(piCur1 + col);
                n2.load(piCur2 + col);
                n3.load(piCur3 + col);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);

                m1.load_a(piOrg + col + 8);
                n1.load(piCur1 + col + 8);
                n2.load(piCur2 + col + 8);
                n3.load(piCur3 + col + 8);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);

                m1.load_a(piOrg + col + 16);
                n1.load(piCur1 + col + 16);
                n2.load(piCur2 + col + 16);
                n3.load(piCur3 + col + 16);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);

                m1.load_a(piOrg + col + 24);
                n1.load(piCur1 + col + 24);
                n2.load(piCur2 + col + 24);
                n3.load(piCur3 + col + 24);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
            }
            piOrg += FENC_STRIDE;
            piCur1 += FENC_STRIDE;
            piCur2 += FENC_STRIDE;
            piCur3 += FENC_STRIDE;
        }
        sum1_low += extend_low(sum1);
        sum1_high += extend_high(sum1);
        sum2_low += extend_low(sum2);
        sum2_high += extend_high(sum2);
        sum3_low += extend_low(sum3);
        sum3_high += extend_high(sum3);
    }
    for (; row < ly; row++)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        for (int col = 0; col < lx; col += 32)
        {
            m1.load_a(piOrg + col);
            n1.load(piCur1 + col);
            n2.load(piCur2 + col);
            n3.load(piCur3 + col);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);

            m1.load_a(piOrg + col + 8);
            n1.load(piCur1 + col + 8);
            n2.load(piCur2 + col + 8);
            n3.load(piCur3 + col + 8);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);

            m1.load_a(piOrg + col + 16);
            n1.load(piCur1 + col + 16);
            n2.load(piCur2 + col + 16);
            n3.load(piCur3 + col + 16);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);

            m1.load_a(piOrg + col + 24);
            n1.load(piCur1 + col + 24);
            n2.load(piCur2 + col + 24);
            n3.load(piCur3 + col + 24);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
        }
        sum1_low += extend_low(sum1);
        sum1_high += extend_high(sum1);
        sum2_low += extend_low(sum2);
        sum2_high += extend_high(sum2);
        sum3_low += extend_low(sum3);
        sum3_high += extend_high(sum3);

        piOrg += FENC_STRIDE;
        piCur1 += FENC_STRIDE;
        piCur2 += FENC_STRIDE;
        piCur3 += FENC_STRIDE;
    }
    res[0] += horizontal_add_x(sum1_low) + horizontal_add_x(sum1_high);
    res[1] += horizontal_add_x(sum2_low) + horizontal_add_x(sum2_high);
    res[2] += horizontal_add_x(sum3_low) + horizontal_add_x(sum3_high);
}

template<int lx, int ly>
void CDECL sad_64_x3(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, int *res)
{    
    Vec8s m1, n1, n2, n3;
    Vec4i sum1_low(0), sum1_high(0);
    Vec4i sum2_low(0), sum2_high(0);
    Vec4i sum3_low(0), sum3_high(0);
    int row;
    res[0] = 0;
    res[1] = 0;
    res[2] = 0;
    for (row = 0; row < ly-1; row+=2)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        for(int temp_rows = 0; temp_rows < 2; temp_rows++)
        {
            for (int col = 0; col < lx; col += 64)
            {
                m1.load_a(piOrg + col);
                n1.load(piCur1 + col);
                n2.load(piCur2 + col);
                n3.load(piCur3 + col);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);

                m1.load_a(piOrg + col + 8);
                n1.load(piCur1 + col + 8);
                n2.load(piCur2 + col + 8);
                n3.load(piCur3 + col + 8);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);

                m1.load_a(piOrg + col + 16);
                n1.load(piCur1 + col + 16);
                n2.load(piCur2 + col + 16);
                n3.load(piCur3 + col + 16);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);

                m1.load_a(piOrg + col + 24);
                n1.load(piCur1 + col + 24);
                n2.load(piCur2 + col + 24);
                n3.load(piCur3 + col + 24);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);

                m1.load_a(piOrg + col + 32);
                n1.load(piCur1 + col + 32);
                n2.load(piCur2 + col + 32);
                n3.load(piCur3 + col + 32);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);

                m1.load_a(piOrg + col + 40);
                n1.load(piCur1 + col + 40);
                n2.load(piCur2 + col + 40);
                n3.load(piCur3 + col + 40);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);

                m1.load_a(piOrg + col + 48);
                n1.load(piCur1 + col + 48);
                n2.load(piCur2 + col + 48);
                n3.load(piCur3 + col + 48);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);

                m1.load_a(piOrg + col + 56);
                n1.load(piCur1 + col + 56);
                n2.load(piCur2 + col + 56);
                n3.load(piCur3 + col + 56);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
            }
            piOrg += FENC_STRIDE;
            piCur1 += FENC_STRIDE;
            piCur2 += FENC_STRIDE;
            piCur3 += FENC_STRIDE;
        }
        sum1_low += extend_low(sum1);
        sum1_high += extend_high(sum1);
        sum2_low += extend_low(sum2);
        sum2_high += extend_high(sum2);
        sum3_low += extend_low(sum3);
        sum3_high += extend_high(sum3);
    }
    for (; row < ly; row++)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        for (int col = 0; col < lx; col += 64)
        {
            m1.load_a(piOrg + col);
            n1.load(piCur1 + col);
            n2.load(piCur2 + col);
            n3.load(piCur3 + col);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);

            m1.load_a(piOrg + col + 8);
            n1.load(piCur1 + col + 8);
            n2.load(piCur2 + col + 8);
            n3.load(piCur3 + col + 8);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);

            m1.load_a(piOrg + col + 16);
            n1.load(piCur1 + col + 16);
            n2.load(piCur2 + col + 16);
            n3.load(piCur3 + col + 16);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);

            m1.load_a(piOrg + col + 24);
            n1.load(piCur1 + col + 24);
            n2.load(piCur2 + col + 24);
            n3.load(piCur3 + col + 24);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);

            m1.load_a(piOrg + col + 32);
            n1.load(piCur1 + col + 32);
            n2.load(piCur2 + col + 32);
            n3.load(piCur3 + col + 32);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);

            m1.load_a(piOrg + col + 40);
            n1.load(piCur1 + col + 40);
            n2.load(piCur2 + col + 40);
            n3.load(piCur3 + col + 40);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);

            m1.load_a(piOrg + col + 48);
            n1.load(piCur1 + col + 48);
            n2.load(piCur2 + col + 48);
            n3.load(piCur3 + col + 48);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);

            m1.load_a(piOrg + col + 56);
            n1.load(piCur1 + col + 56);
            n2.load(piCur2 + col + 56);
            n3.load(piCur3 + col + 56);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
        }
        sum1_low += extend_low(sum1);
        sum1_high += extend_high(sum1);
        sum2_low += extend_low(sum2);
        sum2_high += extend_high(sum2);
        sum3_low += extend_low(sum3);
        sum3_high += extend_high(sum3);

        piOrg += FENC_STRIDE;
        piCur1 += FENC_STRIDE;
        piCur2 += FENC_STRIDE;
        piCur3 += FENC_STRIDE;
    }
    res[0] += horizontal_add_x(sum1_low) + horizontal_add_x(sum1_high);
    res[1] += horizontal_add_x(sum2_low) + horizontal_add_x(sum2_high);
    res[2] += horizontal_add_x(sum3_low) + horizontal_add_x(sum3_high);
}

/* For performance - This function assumes that the *last load* can access 8 elements. */
template<int lx, int ly>
void CDECL sad_4_x4(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, int *res)
{    
    Vec8s m1, n1, n2, n3, n4;
    Vec4i sum1_low(0);
    Vec4i sum2_low(0);
    Vec4i sum3_low(0);
    Vec4i sum4_low(0);
    int row;
    res[0] = 0;
    res[1] = 0;
    res[2] = 0;
    res[3] = 0;
    for (row = 0; row < ly-7; row+=8)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        Vec8s sum4(0);
        for(int temp_rows = 0; temp_rows < 8; temp_rows++)
        {
            for (int col = 0; col < lx; col += 4)
            {
                m1.load_a(piOrg + col);
                n1.load(piCur1 + col);
                n2.load(piCur2 + col);
                n3.load(piCur3 + col);
                n4.load(piCur4 + col);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
                sum4 += abs(m1 - n4);
            }
            piOrg += FENC_STRIDE;
            piCur1 += FENC_STRIDE;
            piCur2 += FENC_STRIDE;
            piCur3 += FENC_STRIDE;
            piCur4 += FENC_STRIDE;
        }
        sum1_low += extend_low(sum1);
        sum2_low += extend_low(sum2);
        sum3_low += extend_low(sum3);
        sum4_low += extend_low(sum4);
    }
    for (; row < ly; row++)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        Vec8s sum4(0);
        for (int col = 0; col < lx; col += 4)
        {
            m1.load_a(piOrg + col);
            n1.load(piCur1 + col);
            n2.load(piCur2 + col);
            n3.load(piCur3 + col);
            n4.load(piCur4 + col);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
            sum4 += abs(m1 - n4);
        }
        sum1_low += extend_low(sum1);
        sum2_low += extend_low(sum2);
        sum3_low += extend_low(sum3);
        sum4_low += extend_low(sum4);
        piOrg += FENC_STRIDE;
        piCur1 += FENC_STRIDE;
        piCur2 += FENC_STRIDE;
        piCur3 += FENC_STRIDE;
        piCur4 += FENC_STRIDE;
    }
    res[0] += horizontal_add_x(sum1_low);
    res[1] += horizontal_add_x(sum2_low);
    res[2] += horizontal_add_x(sum3_low);
    res[3] += horizontal_add_x(sum4_low);
}

/* For performance - This function assumes that the *last load* can access 8 elements. */
template<int lx, int ly>
void CDECL sad_4_4_x4(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, int *res)
{    
    Vec8s m1, n1, n2, n3, n4;
    Vec4i sum1_low(0);
    Vec4i sum2_low(0);
    Vec4i sum3_low(0);
    Vec4i sum4_low(0);
    int row;
    res[0] = 0;
    res[1] = 0;
    res[2] = 0;
    res[3] = 0;
    for (row = 0; row < ly-3; row+=4)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        Vec8s sum4(0);
        for(int temp_rows = 0; temp_rows < 4; temp_rows++)
        {
            for (int col = 0; col < lx; col += 4)
            {
                m1.load_a(piOrg + col);
                n1.load(piCur1 + col);
                n2.load(piCur2 + col);
                n3.load(piCur3 + col);
                n4.load(piCur4 + col);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
                sum4 += abs(m1 - n4);
            }
            piOrg += FENC_STRIDE;
            piCur1 += FENC_STRIDE;
            piCur2 += FENC_STRIDE;
            piCur3 += FENC_STRIDE;
            piCur4 += FENC_STRIDE;
        }
        sum1_low += extend_low(sum1);
        sum2_low += extend_low(sum2);
        sum3_low += extend_low(sum3);
        sum4_low += extend_low(sum4);
    }
    for (; row < ly; row++)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        Vec8s sum4(0);
        for (int col = 0; col < lx; col += 4)
        {
            m1.load_a(piOrg + col);
            n1.load(piCur1 + col);
            n2.load(piCur2 + col);
            n3.load(piCur3 + col);
            n4.load(piCur4 + col);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
            sum4 += abs(m1 - n4);
        }
        sum1_low += extend_low(sum1);
        sum2_low += extend_low(sum2);
        sum3_low += extend_low(sum3);
        sum4_low += extend_low(sum4);
        piOrg += FENC_STRIDE;
        piCur1 += FENC_STRIDE;
        piCur2 += FENC_STRIDE;
        piCur3 += FENC_STRIDE;
        piCur4 += FENC_STRIDE;
    }
    res[0] += horizontal_add_x(sum1_low);
    res[1] += horizontal_add_x(sum2_low);
    res[2] += horizontal_add_x(sum3_low);
    res[3] += horizontal_add_x(sum4_low);
}

template<int lx, int ly>
void CDECL sad_8_x4(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, int *res)
{    
    Vec8s m1, n1, n2, n3, n4;
    Vec4i sum1_low(0), sum1_high(0);
    Vec4i sum2_low(0), sum2_high(0);
    Vec4i sum3_low(0), sum3_high(0);
    Vec4i sum4_low(0), sum4_high(0);
    int row;
    res[0] = 0;
    res[1] = 0;
    res[2] = 0;
    res[3] = 0; 
    for (row = 0; row < ly-7; row+=8)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        Vec8s sum4(0);
        for(int temp_rows = 0; temp_rows < 8; temp_rows++)
        {
            for (int col = 0; col < lx; col += 8)
            {
                m1.load_a(piOrg + col);
                n1.load(piCur1 + col);
                n2.load(piCur2 + col);
                n3.load(piCur3 + col);
                n4.load(piCur4 + col);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
                sum4 += abs(m1 - n4);
            }
            piOrg += FENC_STRIDE;
            piCur1 += FENC_STRIDE;
            piCur2 += FENC_STRIDE;
            piCur3 += FENC_STRIDE;
            piCur4 += FENC_STRIDE;
        }
        sum1_low += extend_low(sum1);
        sum1_high += extend_high(sum1);
        sum2_low += extend_low(sum2);
        sum2_high += extend_high(sum2);
        sum3_low += extend_low(sum3);
        sum3_high += extend_high(sum3);
        sum4_low += extend_low(sum4);
        sum4_high += extend_high(sum4);
    }
    for (; row < ly; row++)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        Vec8s sum4(0);
        for (int col = 0; col < lx; col += 8)
        {
            m1.load_a(piOrg + col);
            n1.load(piCur1 + col);
            n2.load(piCur2 + col);
            n3.load(piCur3 + col);
            n4.load(piCur4 + col);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
            sum4 += abs(m1 - n4);
        }
        sum1_low += extend_low(sum1);
        sum1_high += extend_high(sum1);
        sum2_low += extend_low(sum2);
        sum2_high += extend_high(sum2);
        sum3_low += extend_low(sum3);
        sum3_high += extend_high(sum3);
        sum4_low += extend_low(sum4);
        sum4_high += extend_high(sum4);
        piOrg += FENC_STRIDE;
        piCur1 += FENC_STRIDE;
        piCur2 += FENC_STRIDE;
        piCur3 += FENC_STRIDE;
        piCur4 += FENC_STRIDE;
    }
    res[0] += horizontal_add_x(sum1_low) + horizontal_add_x(sum1_high);
    res[1] += horizontal_add_x(sum2_low) + horizontal_add_x(sum2_high);
    res[2] += horizontal_add_x(sum3_low) + horizontal_add_x(sum3_high);
    res[3] += horizontal_add_x(sum4_low) + horizontal_add_x(sum4_high);
}

template<int lx, int ly>
void CDECL sad_8_4_x4(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, int *res)
{    
    Vec8s m1, n1, n2, n3, n4;
    Vec4i sum1_low(0), sum1_high(0);
    Vec4i sum2_low(0), sum2_high(0);
    Vec4i sum3_low(0), sum3_high(0);
    Vec4i sum4_low(0), sum4_high(0);
    int row;
    res[0] = 0;
    res[1] = 0;
    res[2] = 0;
    res[3] = 0; 
    for (row = 0; row < ly-3; row+=4)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        Vec8s sum4(0);
        for(int temp_rows = 0; temp_rows < 4; temp_rows++)
        {
            for (int col = 0; col < lx; col += 8)
            {
                m1.load_a(piOrg + col);
                n1.load(piCur1 + col);
                n2.load(piCur2 + col);
                n3.load(piCur3 + col);
                n4.load(piCur4 + col);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
                sum4 += abs(m1 - n4);
            }
            piOrg += FENC_STRIDE;
            piCur1 += FENC_STRIDE;
            piCur2 += FENC_STRIDE;
            piCur3 += FENC_STRIDE;
            piCur4 += FENC_STRIDE;
        }
        sum1_low += extend_low(sum1);
        sum1_high += extend_high(sum1);
        sum2_low += extend_low(sum2);
        sum2_high += extend_high(sum2);
        sum3_low += extend_low(sum3);
        sum3_high += extend_high(sum3);
        sum4_low += extend_low(sum4);
        sum4_high += extend_high(sum4);
    }
    for (; row < ly; row++)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        Vec8s sum4(0);
        for (int col = 0; col < lx; col += 8)
        {
            m1.load_a(piOrg + col);
            n1.load(piCur1 + col);
            n2.load(piCur2 + col);
            n3.load(piCur3 + col);
            n4.load(piCur4 + col);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
            sum4 += abs(m1 - n4);
        }
        sum1_low += extend_low(sum1);
        sum1_high += extend_high(sum1);
        sum2_low += extend_low(sum2);
        sum2_high += extend_high(sum2);
        sum3_low += extend_low(sum3);
        sum3_high += extend_high(sum3);
        sum4_low += extend_low(sum4);
        sum4_high += extend_high(sum4);
        piOrg += FENC_STRIDE;
        piCur1 += FENC_STRIDE;
        piCur2 += FENC_STRIDE;
        piCur3 += FENC_STRIDE;
        piCur4 += FENC_STRIDE;
    }
    res[0] += horizontal_add_x(sum1_low) + horizontal_add_x(sum1_high);
    res[1] += horizontal_add_x(sum2_low) + horizontal_add_x(sum2_high);
    res[2] += horizontal_add_x(sum3_low) + horizontal_add_x(sum3_high);
    res[3] += horizontal_add_x(sum4_low) + horizontal_add_x(sum4_high);
}

/* For performance - This function assumes that the *last load* can access 8 elements. */
template<int lx, int ly>
void CDECL sad_12_x4(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, int *res)
{    
    Vec8s m1, n1, n2, n3, n4;
    Vec4i sum1_low(0), sum1_high(0);
    Vec4i sum2_low(0), sum2_high(0);
    Vec4i sum3_low(0), sum3_high(0);
    Vec4i sum4_low(0), sum4_high(0);
    int row;
    res[0] = 0;
    res[1] = 0;
    res[2] = 0;
    res[3] = 0; 
    for (row = 0; row < ly-7; row+=8)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        Vec8s sum4(0);
        for(int temp_rows = 0; temp_rows < 8; temp_rows++)
        {
            for (int col = 0; col < lx; col += 12)
            {
                m1.load_a(piOrg + col);
                n1.load(piCur1 + col);
                n2.load(piCur2 + col);
                n3.load(piCur3 + col);
                n4.load(piCur4 + col);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
                sum4 += abs(m1 - n4);

                m1.load(piOrg + col + 4);
                n1.load(piCur1 + col + 4);
                n2.load(piCur2 + col + 4);
                n3.load(piCur3 + col + 4);
                n4.load(piCur4 + col + 4);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
                sum4 += abs(m1 - n4);

                m1.load_a(piOrg + col + 8);
                n1.load(piCur1 + col + 8);
                n2.load(piCur2 + col + 8);
                n3.load(piCur3 + col + 8);
                n4.load(piCur4 + col + 8);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
                sum4 += abs(m1 - n4);
            }
            piOrg += FENC_STRIDE;
            piCur1 += FENC_STRIDE;
            piCur2 += FENC_STRIDE;
            piCur3 += FENC_STRIDE;
            piCur4 += FENC_STRIDE;
        }
        sum1_low += extend_low(sum1);
        sum2_low += extend_low(sum2);
        sum3_low += extend_low(sum3);
        sum4_low += extend_low(sum4);
    }
    for (; row < ly; row++)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        Vec8s sum4(0);
        for (int col = 0; col < lx; col += 12)
        {
            m1.load_a(piOrg + col);
            n1.load(piCur1 + col);
            n2.load(piCur2 + col);
            n3.load(piCur3 + col);
            n4.load(piCur4 + col);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
            sum4 += abs(m1 - n4);

            m1.load_a(piOrg + col + 4);
            n1.load(piCur1 + col + 4);
            n2.load(piCur2 + col + 4);
            n3.load(piCur3 + col + 4);
            n4.load(piCur4 + col + 4);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
            sum4 += abs(m1 - n4);

            m1.load_a(piOrg + col + 8);
            n1.load(piCur1 + col + 8);
            n2.load(piCur2 + col + 8);
            n3.load(piCur3 + col + 8);
            n4.load(piCur4 + col + 8);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
            sum4 += abs(m1 - n4);
        }
        sum1_low += extend_low(sum1);
        sum2_low += extend_low(sum2);
        sum3_low += extend_low(sum3);
        sum4_low += extend_low(sum4);
        piOrg += FENC_STRIDE;
        piCur1 += FENC_STRIDE;
        piCur2 += FENC_STRIDE;
        piCur3 += FENC_STRIDE;
        piCur4 += FENC_STRIDE;
    }
    res[0] += horizontal_add_x(sum1_low);
    res[1] += horizontal_add_x(sum2_low);
    res[2] += horizontal_add_x(sum3_low);
    res[3] += horizontal_add_x(sum4_low);
}

/* For performance - This function assumes that the *last load* can access 8 elements. */
template<int lx, int ly>
void CDECL sad_12_4_x4(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, int *res)
{    
    Vec8s m1, n1, n2, n3, n4;
    Vec4i sum1_low(0), sum1_high(0);
    Vec4i sum2_low(0), sum2_high(0);
    Vec4i sum3_low(0), sum3_high(0);
    Vec4i sum4_low(0), sum4_high(0);
    int row;
    res[0] = 0;
    res[1] = 0;
    res[2] = 0;
    res[3] = 0; 
    for (row = 0; row < ly-3; row+=4)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        Vec8s sum4(0);
        for(int temp_rows = 0; temp_rows < 4; temp_rows++)
        {
            for (int col = 0; col < lx; col += 12)
            {
                m1.load_a(piOrg + col);
                n1.load(piCur1 + col);
                n2.load(piCur2 + col);
                n3.load(piCur3 + col);
                n4.load(piCur4 + col);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
                sum4 += abs(m1 - n4);

                m1.load(piOrg + col + 4);
                n1.load(piCur1 + col + 4);
                n2.load(piCur2 + col + 4);
                n3.load(piCur3 + col + 4);
                n4.load(piCur4 + col + 4);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
                sum4 += abs(m1 - n4);

                m1.load_a(piOrg + col + 8);
                n1.load(piCur1 + col + 8);
                n2.load(piCur2 + col + 8);
                n3.load(piCur3 + col + 8);
                n4.load(piCur4 + col + 8);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
                sum4 += abs(m1 - n4);
            }
            piOrg += FENC_STRIDE;
            piCur1 += FENC_STRIDE;
            piCur2 += FENC_STRIDE;
            piCur3 += FENC_STRIDE;
            piCur4 += FENC_STRIDE;
        }
        sum1_low += extend_low(sum1);
        sum2_low += extend_low(sum2);
        sum3_low += extend_low(sum3);
        sum4_low += extend_low(sum4);
    }
    for (; row < ly; row++)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        Vec8s sum4(0);
        for (int col = 0; col < lx; col += 12)
        {
            m1.load_a(piOrg + col);
            n1.load(piCur1 + col);
            n2.load(piCur2 + col);
            n3.load(piCur3 + col);
            n4.load(piCur4 + col);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
            sum4 += abs(m1 - n4);

            m1.load_a(piOrg + col + 4);
            n1.load(piCur1 + col + 4);
            n2.load(piCur2 + col + 4);
            n3.load(piCur3 + col + 4);
            n4.load(piCur4 + col + 4);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
            sum4 += abs(m1 - n4);

            m1.load_a(piOrg + col + 8);
            n1.load(piCur1 + col + 8);
            n2.load(piCur2 + col + 8);
            n3.load(piCur3 + col + 8);
            n4.load(piCur4 + col + 8);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
            sum4 += abs(m1 - n4);
        }
        sum1_low += extend_low(sum1);
        sum2_low += extend_low(sum2);
        sum3_low += extend_low(sum3);
        sum4_low += extend_low(sum4);
        piOrg += FENC_STRIDE;
        piCur1 += FENC_STRIDE;
        piCur2 += FENC_STRIDE;
        piCur3 += FENC_STRIDE;
        piCur4 += FENC_STRIDE;
    }
    res[0] += horizontal_add_x(sum1_low);
    res[1] += horizontal_add_x(sum2_low);
    res[2] += horizontal_add_x(sum3_low);
    res[3] += horizontal_add_x(sum4_low);
}

template<int lx, int ly>
void CDECL sad_16_x4(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, int *res)
{    
    Vec8s m1, n1, n2, n3, n4;
    Vec4i sum1_low(0), sum1_high(0);
    Vec4i sum2_low(0), sum2_high(0);
    Vec4i sum3_low(0), sum3_high(0);
    Vec4i sum4_low(0), sum4_high(0);
    int row;
    res[0] = 0;
    res[1] = 0;
    res[2] = 0;
    res[3] = 0; 
    for (row = 0; row < ly-7; row+=8)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        Vec8s sum4(0);
        for(int temp_rows = 0; temp_rows < 8; temp_rows++)
        {
            for (int col = 0; col < lx; col += 16)
            {
                m1.load_a(piOrg + col);
                n1.load(piCur1 + col);
                n2.load(piCur2 + col);
                n3.load(piCur3 + col);
                n4.load(piCur4 + col);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
                sum4 += abs(m1 - n4);

                m1.load_a(piOrg + col + 8);
                n1.load(piCur1 + col + 8);
                n2.load(piCur2 + col + 8);
                n3.load(piCur3 + col + 8);
                n4.load(piCur4 + col + 8);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
                sum4 += abs(m1 - n4);
            }
            piOrg += FENC_STRIDE;
            piCur1 += FENC_STRIDE;
            piCur2 += FENC_STRIDE;
            piCur3 += FENC_STRIDE;
            piCur4 += FENC_STRIDE;
        }
        sum1_low += extend_low(sum1);
        sum1_high += extend_high(sum1);
        sum2_low += extend_low(sum2);
        sum2_high += extend_high(sum2);
        sum3_low += extend_low(sum3);
        sum3_high += extend_high(sum3);
        sum4_low += extend_low(sum4);
        sum4_high += extend_high(sum4);
    }
    for (; row < ly; row++)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        Vec8s sum4(0);
        for (int col = 0; col < lx; col += 16)
        {
            m1.load_a(piOrg + col);
            n1.load(piCur1 + col);
            n2.load(piCur2 + col);
            n3.load(piCur3 + col);
            n4.load(piCur4 + col);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
            sum4 += abs(m1 - n4);

            m1.load_a(piOrg + col + 8);
            n1.load(piCur1 + col + 8);
            n2.load(piCur2 + col + 8);
            n3.load(piCur3 + col + 8);
            n4.load(piCur4 + col + 8);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
            sum4 += abs(m1 - n4);
        }
        sum1_low += extend_low(sum1);
        sum1_high += extend_high(sum1);
        sum2_low += extend_low(sum2);
        sum2_high += extend_high(sum2);
        sum3_low += extend_low(sum3);
        sum3_high += extend_high(sum3);
        sum4_low += extend_low(sum4);
        sum4_high += extend_high(sum4);
        piOrg += FENC_STRIDE;
        piCur1 += FENC_STRIDE;
        piCur2 += FENC_STRIDE;
        piCur3 += FENC_STRIDE;
        piCur4 += FENC_STRIDE;
    }
    res[0] += horizontal_add_x(sum1_low) + horizontal_add_x(sum1_high);
    res[1] += horizontal_add_x(sum2_low) + horizontal_add_x(sum2_high);
    res[2] += horizontal_add_x(sum3_low) + horizontal_add_x(sum3_high);
    res[3] += horizontal_add_x(sum4_low) + horizontal_add_x(sum4_high);
}

template<int lx, int ly>
void CDECL sad_16_4_x4(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, int *res)
{    
    Vec8s m1, n1, n2, n3, n4;
    Vec4i sum1_low(0), sum1_high(0);
    Vec4i sum2_low(0), sum2_high(0);
    Vec4i sum3_low(0), sum3_high(0);
    Vec4i sum4_low(0), sum4_high(0);
    int row;
    res[0] = 0;
    res[1] = 0;
    res[2] = 0;
    res[3] = 0; 
    for (row = 0; row < ly-3; row+=4)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        Vec8s sum4(0);
        for(int temp_rows = 0; temp_rows < 4; temp_rows++)
        {
            for (int col = 0; col < lx; col += 16)
            {
                m1.load_a(piOrg + col);
                n1.load(piCur1 + col);
                n2.load(piCur2 + col);
                n3.load(piCur3 + col);
                n4.load(piCur4 + col);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
                sum4 += abs(m1 - n4);

                m1.load_a(piOrg + col + 8);
                n1.load(piCur1 + col + 8);
                n2.load(piCur2 + col + 8);
                n3.load(piCur3 + col + 8);
                n4.load(piCur4 + col + 8);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
                sum4 += abs(m1 - n4);
            }
            piOrg += FENC_STRIDE;
            piCur1 += FENC_STRIDE;
            piCur2 += FENC_STRIDE;
            piCur3 += FENC_STRIDE;
            piCur4 += FENC_STRIDE;
        }
        sum1_low += extend_low(sum1);
        sum1_high += extend_high(sum1);
        sum2_low += extend_low(sum2);
        sum2_high += extend_high(sum2);
        sum3_low += extend_low(sum3);
        sum3_high += extend_high(sum3);
        sum4_low += extend_low(sum4);
        sum4_high += extend_high(sum4);
    }
    for (; row < ly; row++)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        Vec8s sum4(0);
        for (int col = 0; col < lx; col += 16)
        {
            m1.load_a(piOrg + col);
            n1.load(piCur1 + col);
            n2.load(piCur2 + col);
            n3.load(piCur3 + col);
            n4.load(piCur4 + col);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
            sum4 += abs(m1 - n4);

            m1.load_a(piOrg + col + 8);
            n1.load(piCur1 + col + 8);
            n2.load(piCur2 + col + 8);
            n3.load(piCur3 + col + 8);
            n4.load(piCur4 + col + 8);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
            sum4 += abs(m1 - n4);
        }
        sum1_low += extend_low(sum1);
        sum1_high += extend_high(sum1);
        sum2_low += extend_low(sum2);
        sum2_high += extend_high(sum2);
        sum3_low += extend_low(sum3);
        sum3_high += extend_high(sum3);
        sum4_low += extend_low(sum4);
        sum4_high += extend_high(sum4);
        piOrg += FENC_STRIDE;
        piCur1 += FENC_STRIDE;
        piCur2 += FENC_STRIDE;
        piCur3 += FENC_STRIDE;
        piCur4 += FENC_STRIDE;
    }
    res[0] += horizontal_add_x(sum1_low) + horizontal_add_x(sum1_high);
    res[1] += horizontal_add_x(sum2_low) + horizontal_add_x(sum2_high);
    res[2] += horizontal_add_x(sum3_low) + horizontal_add_x(sum3_high);
    res[3] += horizontal_add_x(sum4_low) + horizontal_add_x(sum4_high);
}

template<int lx, int ly>
void CDECL sad_32_x4(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, int *res)
{    
    Vec8s m1, n1, n2, n3, n4;
    Vec4i sum1_low(0), sum1_high(0);
    Vec4i sum2_low(0), sum2_high(0);
    Vec4i sum3_low(0), sum3_high(0);
    Vec4i sum4_low(0), sum4_high(0);
    int row;
    res[0] = 0;
    res[1] = 0;
    res[2] = 0;
    res[3] = 0; 
    for (row = 0; row < ly-3; row+=4)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        Vec8s sum4(0);
        for(int temp_rows = 0; temp_rows < 4; temp_rows++)
        {
            for (int col = 0; col < lx; col += 32)
            {
                m1.load_a(piOrg + col);
                n1.load(piCur1 + col);
                n2.load(piCur2 + col);
                n3.load(piCur3 + col);
                n4.load(piCur4 + col);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
                sum4 += abs(m1 - n4);

                m1.load_a(piOrg + col + 8);
                n1.load(piCur1 + col + 8);
                n2.load(piCur2 + col + 8);
                n3.load(piCur3 + col + 8);
                n4.load(piCur4 + col + 8);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
                sum4 += abs(m1 - n4);

                m1.load_a(piOrg + col + 16);
                n1.load(piCur1 + col + 16);
                n2.load(piCur2 + col + 16);
                n3.load(piCur3 + col + 16);
                n4.load(piCur4 + col + 16);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
                sum4 += abs(m1 - n4);

                m1.load_a(piOrg + col + 24);
                n1.load(piCur1 + col + 24);
                n2.load(piCur2 + col + 24);
                n3.load(piCur3 + col + 24);
                n4.load(piCur4 + col + 24);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
                sum4 += abs(m1 - n4);
            }
            piOrg += FENC_STRIDE;
            piCur1 += FENC_STRIDE;
            piCur2 += FENC_STRIDE;
            piCur3 += FENC_STRIDE;
            piCur4 += FENC_STRIDE;
        }
        sum1_low += extend_low(sum1);
        sum1_high += extend_high(sum1);
        sum2_low += extend_low(sum2);
        sum2_high += extend_high(sum2);
        sum3_low += extend_low(sum3);
        sum3_high += extend_high(sum3);
        sum4_low += extend_low(sum4);
        sum4_high += extend_high(sum4);
    }
    for (; row < ly; row++)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        Vec8s sum4(0);
        for (int col = 0; col < lx; col += 32)
        {
            m1.load_a(piOrg + col);
            n1.load(piCur1 + col);
            n2.load(piCur2 + col);
            n3.load(piCur3 + col);
            n4.load(piCur4 + col);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
            sum4 += abs(m1 - n4);

            m1.load_a(piOrg + col + 8);
            n1.load(piCur1 + col + 8);
            n2.load(piCur2 + col + 8);
            n3.load(piCur3 + col + 8);
            n4.load(piCur4 + col + 8);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
            sum4 += abs(m1 - n4);

            m1.load_a(piOrg + col + 16);
            n1.load(piCur1 + col + 16);
            n2.load(piCur2 + col + 16);
            n3.load(piCur3 + col + 16);
            n4.load(piCur4 + col + 16);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
            sum4 += abs(m1 - n4);

            m1.load_a(piOrg + col + 24);
            n1.load(piCur1 + col + 24);
            n2.load(piCur2 + col + 24);
            n3.load(piCur3 + col + 24);
            n4.load(piCur4 + col + 24 );

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
            sum4 += abs(m1 - n4);
        }
        sum1_low += extend_low(sum1);
        sum1_high += extend_high(sum1);
        sum2_low += extend_low(sum2);
        sum2_high += extend_high(sum2);
        sum3_low += extend_low(sum3);
        sum3_high += extend_high(sum3);
        sum4_low += extend_low(sum4);
        sum4_high += extend_high(sum4);
        piOrg += FENC_STRIDE;
        piCur1 += FENC_STRIDE;
        piCur2 += FENC_STRIDE;
        piCur3 += FENC_STRIDE;
        piCur4 += FENC_STRIDE;
    }
    res[0] += horizontal_add_x(sum1_low) + horizontal_add_x(sum1_high);
    res[1] += horizontal_add_x(sum2_low) + horizontal_add_x(sum2_high);
    res[2] += horizontal_add_x(sum3_low) + horizontal_add_x(sum3_high);
    res[3] += horizontal_add_x(sum4_low) + horizontal_add_x(sum4_high);
}

template<int lx, int ly>
void CDECL sad_64_x4(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, int *res)
{    
    Vec8s m1, n1, n2, n3, n4;
    Vec4i sum1_low(0), sum1_high(0);
    Vec4i sum2_low(0), sum2_high(0);
    Vec4i sum3_low(0), sum3_high(0);
    Vec4i sum4_low(0), sum4_high(0);
    int row;
    res[0] = 0;
    res[1] = 0;
    res[2] = 0;
    res[3] = 0; 
    for (row = 0; row < ly-1; row+=2)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        Vec8s sum4(0);
        for(int temp_rows = 0; temp_rows < 2; temp_rows++)
        {
            for (int col = 0; col < lx; col += 64)
            {
                m1.load_a(piOrg + col);
                n1.load(piCur1 + col);
                n2.load(piCur2 + col);
                n3.load(piCur3 + col);
                n4.load(piCur4 + col);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
                sum4 += abs(m1 - n4);

                m1.load_a(piOrg + col + 8);
                n1.load(piCur1 + col + 8);
                n2.load(piCur2 + col + 8);
                n3.load(piCur3 + col + 8);
                n4.load(piCur4 + col + 8);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
                sum4 += abs(m1 - n4);

                m1.load_a(piOrg + col + 16);
                n1.load(piCur1 + col + 16);
                n2.load(piCur2 + col + 16);
                n3.load(piCur3 + col + 16);
                n4.load(piCur4 + col + 16);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
                sum4 += abs(m1 - n4);

                m1.load_a(piOrg + col + 24);
                n1.load(piCur1 + col + 24);
                n2.load(piCur2 + col + 24);
                n3.load(piCur3 + col + 24);
                n4.load(piCur4 + col + 24);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
                sum4 += abs(m1 - n4);

                m1.load_a(piOrg + col + 32);
                n1.load(piCur1 + col + 32);
                n2.load(piCur2 + col + 32);
                n3.load(piCur3 + col + 32);
                n4.load(piCur4 + col + 32);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
                sum4 += abs(m1 - n4);

                m1.load_a(piOrg + col + 40);
                n1.load(piCur1 + col + 40);
                n2.load(piCur2 + col + 40);
                n3.load(piCur3 + col + 40);
                n4.load(piCur4 + col + 40);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
                sum4 += abs(m1 - n4);

                m1.load_a(piOrg + col + 48);
                n1.load(piCur1 + col + 48);
                n2.load(piCur2 + col + 48);
                n3.load(piCur3 + col + 48);
                n4.load(piCur4 + col + 48);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
                sum4 += abs(m1 - n4);

                m1.load_a(piOrg + col + 56);
                n1.load(piCur1 + col + 56);
                n2.load(piCur2 + col + 56);
                n3.load(piCur3 + col + 56);
                n4.load(piCur4 + col + 56);

                sum1 += abs(m1 - n1);
                sum2 += abs(m1 - n2);
                sum3 += abs(m1 - n3);
                sum4 += abs(m1 - n4);
            }
            piOrg += FENC_STRIDE;
            piCur1 += FENC_STRIDE;
            piCur2 += FENC_STRIDE;
            piCur3 += FENC_STRIDE;
            piCur4 += FENC_STRIDE;
        }
        sum1_low += extend_low(sum1);
        sum1_high += extend_high(sum1);
        sum2_low += extend_low(sum2);
        sum2_high += extend_high(sum2);
        sum3_low += extend_low(sum3);
        sum3_high += extend_high(sum3);
        sum4_low += extend_low(sum4);
        sum4_high += extend_high(sum4);
    }
    for (; row < ly; row++)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        Vec8s sum4(0);
        for (int col = 0; col < lx; col += 64)
        {
            m1.load_a(piOrg + col);
            n1.load(piCur1 + col);
            n2.load(piCur2 + col);
            n3.load(piCur3 + col);
            n4.load(piCur4 + col);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
            sum4 += abs(m1 - n4);

            m1.load_a(piOrg + col + 8);
            n1.load(piCur1 + col + 8);
            n2.load(piCur2 + col + 8);
            n3.load(piCur3 + col + 8);
            n4.load(piCur4 + col + 8);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
            sum4 += abs(m1 - n4);

            m1.load_a(piOrg + col + 16);
            n1.load(piCur1 + col + 16);
            n2.load(piCur2 + col + 16);
            n3.load(piCur3 + col + 16);
            n4.load(piCur4 + col + 16);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
            sum4 += abs(m1 - n4);

            m1.load_a(piOrg + col + 24);
            n1.load(piCur1 + col + 24);
            n2.load(piCur2 + col + 24);
            n3.load(piCur3 + col + 24);
            n4.load(piCur4 + col + 24);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
            sum4 += abs(m1 - n4);

            m1.load_a(piOrg + col + 32);
            n1.load(piCur1 + col + 32);
            n2.load(piCur2 + col + 32);
            n3.load(piCur3 + col + 32);
            n4.load(piCur4 + col + 32);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
            sum4 += abs(m1 - n4);

            m1.load_a(piOrg + col + 40);
            n1.load(piCur1 + col + 40);
            n2.load(piCur2 + col + 40);
            n3.load(piCur3 + col + 40);
            n4.load(piCur4 + col + 40);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
            sum4 += abs(m1 - n4);

            m1.load_a(piOrg + col + 48);
            n1.load(piCur1 + col + 48);
            n2.load(piCur2 + col + 48);
            n3.load(piCur3 + col + 48);
            n4.load(piCur4 + col + 48);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
            sum4 += abs(m1 - n4);

            m1.load_a(piOrg + col + 56);
            n1.load(piCur1 + col + 56);
            n2.load(piCur2 + col + 56);
            n3.load(piCur3 + col + 56);
            n4.load(piCur4 + col + 56);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
            sum4 += abs(m1 - n4);
        }
        sum1_low += extend_low(sum1);
        sum1_high += extend_high(sum1);
        sum2_low += extend_low(sum2);
        sum2_high += extend_high(sum2);
        sum3_low += extend_low(sum3);
        sum3_high += extend_high(sum3);
        sum4_low += extend_low(sum4);
        sum4_high += extend_high(sum4);
        piOrg += FENC_STRIDE;
        piCur1 += FENC_STRIDE;
        piCur2 += FENC_STRIDE;
        piCur3 += FENC_STRIDE;
        piCur4 += FENC_STRIDE;
    }
    res[0] += horizontal_add_x(sum1_low) + horizontal_add_x(sum1_high);
    res[1] += horizontal_add_x(sum2_low) + horizontal_add_x(sum2_high);
    res[2] += horizontal_add_x(sum3_low) + horizontal_add_x(sum3_high);
    res[3] += horizontal_add_x(sum4_low) + horizontal_add_x(sum4_high);
}

int CDECL satd_4x4(pixel * piOrg, intptr_t iStrideOrg, pixel * piCur, intptr_t iStrideCur)
{
    int satd = 0;

    Vec8s v1, v2, m1, m2;

    {
        Vec8s temp1, temp2, temp3, temp4, piOrg_v, piCur_v;
        temp1.load(piOrg);
        temp2.load(piCur);
        piCur += iStrideCur;
        piOrg += iStrideOrg;

        temp3.load(piOrg);
        temp4.load(piCur);
        piCur += iStrideCur;
        piOrg += iStrideOrg;

        piOrg_v = blend2q<0, 2>((Vec2q)temp1, (Vec2q)temp3);
        piCur_v = blend2q<0, 2>((Vec2q)temp2, (Vec2q)temp4);

        temp1.load(piOrg);
        temp2.load(piCur);
        piCur += iStrideCur;
        piOrg += iStrideOrg;

        temp3.load(piOrg);
        temp4.load(piCur);
        piCur += iStrideCur;
        piOrg += iStrideOrg;

        v1 = piOrg_v - piCur_v; //diff

        piOrg_v = blend2q<0, 2>((Vec2q)temp3, (Vec2q)temp1);
        piCur_v = blend2q<0, 2>((Vec2q)temp4, (Vec2q)temp2);
        v2 = piOrg_v - piCur_v; //diff
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

int CDECL sa8d_8x8(pixel * piOrg, intptr_t iStrideOrg, pixel * piCur, intptr_t iStrideCur)
{
    ALIGN_VAR_16(short, m2[8][8]);

    Vec8s diff_v1, diff_v2, piOrg_v1, piOrg_v2, piCur_v1, piCur_v2;
    Vec8s v1, v2, t1, t2;

    int  j, satd = 0;

    for (j = 0; j < 8; j += 2)
    {
        piOrg_v1.load_a(piOrg);
        piCur_v1.load(piCur);
        piCur += iStrideCur;
        piOrg += iStrideOrg;

        piOrg_v2.load_a(piOrg);
        piCur_v2.load(piCur);
        piCur += iStrideCur;
        piOrg += iStrideOrg;

        diff_v1 = piOrg_v1 - piCur_v1;
        diff_v2 = piOrg_v2 - piCur_v2;

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

#else

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

#endif

template<int ly>
int CDECL sad_4x16(pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    int sum = 0;
    for (int row = 0; row < ly; row += 16)
        sum += sad_4<16>(piOrg + row * strideOrg, strideOrg, piCur + row * strideCur, strideCur);
    return sum;
}

template<int ly>
int CDECL sad_8x16(pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    int sum = 0;
    for (int row = 0; row < ly; row += 16)
        sum += sad_8<16>(piOrg + row * strideOrg, strideOrg, piCur + row * strideCur, strideCur);
    return sum;
}

template<int lx, int ly>
int CDECL sad_32x12(pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    int sum = 0;
    for (int row = 0; row < ly; row += 12)
    {
        for (int col = 0; col < lx; col += 32)
        {
            sum += sad_32<32,12>(piOrg + row * strideOrg + col, strideOrg,
                              piCur + row * strideCur + col, strideCur);
        }
    }
    return sum;
}

template<int lx, int ly>
int CDECL sad_16x16(pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    int sum = 0;
    for (int row = 0; row < ly; row += 16)
    {
        for (int col = 0; col < lx; col += 16)
        {
            sum += sad_16<16>(piOrg + row * strideOrg + col, strideOrg,
                              piCur + row * strideCur + col, strideCur);
        }
    }
    return sum;
}

template<int lx, int ly>
int CDECL sad_32x8(pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    int sum = 0;
    for (int row = 0; row < ly; row += 8)
    {
        for (int col = 0; col < lx; col += 32)
        {
            sum += sad_32<32, 8>(piOrg + row * strideOrg + col, strideOrg,
                                 piCur + row * strideCur + col, strideCur);
        }
    }
    return sum;
}

template<int lx, int ly>
int CDECL sad_32x16(pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    int sum = 0;
    for (int row = 0; row < ly; row += 16)
    {
        for (int col = 0; col < lx; col += 32)
        {
            sum += sad_32<32, 16>(piOrg + row * strideOrg + col, strideOrg,
                                 piCur + row * strideCur + col, strideCur);
        }
    }
    return sum;
}

template<int ly>
int CDECL sad_12(pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    int sum = 0;
    for (int row = 0; row < ly; row += 4)
    {
        // cannot assume 8pel alignment, must use 4x4
        sum += sad_4<4>(piOrg + row * strideOrg, strideOrg,
                        piCur + row * strideCur, strideCur);
        sum += sad_4<4>(piOrg + row * strideOrg + 4, strideOrg,
                        piCur + row * strideCur + 4, strideCur);
        sum += sad_4<4>(piOrg + row * strideOrg + 8, strideOrg,
                        piCur + row * strideCur + 8, strideCur);
    }
    return sum;
}

template<int ly>
int CDECL sad_12x8(pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    int sum = 0;
    for (int row = 0; row < ly; row += 8)
    {
        // cannot assume 8pel alignment, must use 4x8
        sum += sad_4<8>(piOrg + row * strideOrg, strideOrg,
                        piCur + row * strideCur, strideCur);
        sum += sad_4<8>(piOrg + row * strideOrg + 4, strideOrg,
                        piCur + row * strideCur + 4, strideCur);
        sum += sad_4<8>(piOrg + row * strideOrg + 8, strideOrg,
                        piCur + row * strideCur + 8, strideCur);
    }
    return sum;
}

template<int lx, int ly>
int CDECL sad_24(pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    int sum = 0;
    for (int row = 0; row < ly; row += 4)
    {
        for (int col = 0; col < lx; col += 8)
        {
            sum += sad_8<4>(piOrg + row * strideOrg + col, strideOrg,
                            piCur + row * strideCur + col, strideCur);
        }
    }
    return sum;
}

template<int lx, int ly>
int CDECL satd(pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    int uiSum = 0;

    for (int row = 0; row < ly; row += 4)
    {
        for (int col = 0; col < lx; col += 4)
        {
            uiSum += satd_4x4(piOrg + strideOrg * row + col, strideOrg,
                              piCur + strideCur * row + col, strideCur);
        }
    }

    return uiSum;
}

void Setup_Vec_PixelPrimitives(EncoderPrimitives &p)
{
    // sad
    p.sad[PARTITION_4x4] = sad_4<4>;
    p.sad[PARTITION_4x8] = sad_4<8>;
    p.sad[PARTITION_4x12] = sad_4<12>;
    p.sad[PARTITION_4x16] = sad_4<16>;
    p.sad[PARTITION_4x24] = sad_4<24>;
    p.sad[PARTITION_4x32] = sad_4<32>;
    p.sad[PARTITION_4x64] = sad_4x16<64>;

    p.sad[PARTITION_8x4] = sad_8<4>;
    p.sad[PARTITION_8x8] = sad_8<8>;
    p.sad[PARTITION_8x12] = sad_8<12>;
    p.sad[PARTITION_8x16] = sad_8<16>;
    p.sad[PARTITION_8x24] = sad_8<24>;
    p.sad[PARTITION_8x32] = sad_8x16<32>;
    p.sad[PARTITION_8x64] = sad_8x16<64>;

    p.sad[PARTITION_12x4] = sad_12<4>;
    p.sad[PARTITION_12x8] = sad_12<8>;
    p.sad[PARTITION_12x12] = sad_12<12>;
    p.sad[PARTITION_12x16] = sad_12x8<16>;
    p.sad[PARTITION_12x24] = sad_12x8<24>;
    p.sad[PARTITION_12x32] = sad_12x8<32>;
    p.sad[PARTITION_12x64] = sad_12x8<64>;

    p.sad[PARTITION_16x4] = sad_16<4>;
    p.sad[PARTITION_16x8] = sad_16<8>;
    p.sad[PARTITION_16x12] = sad_16<12>;
    p.sad[PARTITION_16x16] = sad_16<16>;
    //p.sad[PARTITION_16x24] = sad_16x24<16,24>;
    p.sad[PARTITION_16x32] = sad_16x16<16, 32>;
    p.sad[PARTITION_16x64] = sad_16x16<16, 64>;

    p.sad[PARTITION_32x4] = sad_32<32, 4>;
    p.sad[PARTITION_32x8] = sad_32x8<32, 8>;
    p.sad[PARTITION_32x12] = sad_32<32, 12>;
    p.sad[PARTITION_32x16] = sad_16x16<32, 16>;
    p.sad[PARTITION_32x24] = sad_32x8<32, 24>;
    p.sad[PARTITION_32x32] = sad_32x16<32, 32>;
    p.sad[PARTITION_32x64] = sad_16x16<32, 64>;

    p.sad[PARTITION_64x4] = sad_32<64, 4>;
    p.sad[PARTITION_64x8] = sad_32x8<64, 8>;
    p.sad[PARTITION_64x12] = sad_32x12<64,12>;
    p.sad[PARTITION_64x16] = sad_16x16<64, 16>;
    //p.sad[PARTITION_64x24] = sad_16x24<64, 24>;
    p.sad[PARTITION_64x32] = sad_16x16<64, 32>;
    p.sad[PARTITION_64x64] = sad_16x16<64, 64>;

#if HIGH_BIT_DEPTH
    // sad_x3
    p.sad_x3[PARTITION_4x4] = sad_4_4_x3<4, 4>;
    p.sad_x3[PARTITION_4x8] = sad_4_x3<4, 8>;
    p.sad_x3[PARTITION_4x12] = sad_4_4_x3<4, 12>;
    p.sad_x3[PARTITION_4x16] = sad_4_x3<4, 16>;
    p.sad_x3[PARTITION_4x24] = sad_4_x3<4, 24>;
    p.sad_x3[PARTITION_4x32] = sad_4_x3<4, 32>;
    p.sad_x3[PARTITION_4x64] = sad_4_x3<4, 64>;

    p.sad_x3[PARTITION_8x4] = sad_8_4_x3<8, 4>;
    p.sad_x3[PARTITION_8x8] = sad_8_x3<8, 8>;
    p.sad_x3[PARTITION_8x12] = sad_8_4_x3<8, 12>;
    p.sad_x3[PARTITION_8x16] = sad_8_x3<8, 16>;
    p.sad_x3[PARTITION_8x24] = sad_8_x3<8, 24>;
    p.sad_x3[PARTITION_8x32] = sad_8_x3<8, 32>;
    p.sad_x3[PARTITION_8x64] = sad_8_x3<8, 64>;

    p.sad_x3[PARTITION_12x4] = sad_12_4_x3<12, 4>;
    p.sad_x3[PARTITION_12x8] = sad_12_x3<12, 8>;
    p.sad_x3[PARTITION_12x12] = sad_12_4_x3<12, 12>;
    p.sad_x3[PARTITION_12x16] = sad_12_x3<12, 16>;
    p.sad_x3[PARTITION_12x24] = sad_12_x3<12, 24>;
    p.sad_x3[PARTITION_12x32] = sad_12_x3<12, 32>;
    p.sad_x3[PARTITION_12x64] = sad_12_x3<12, 64>;

    p.sad_x3[PARTITION_16x4] = sad_16_4_x3<16, 4>;
    p.sad_x3[PARTITION_16x8] = sad_16_x3<16, 8>;
    p.sad_x3[PARTITION_16x12] = sad_16_4_x3<16, 12>;
    p.sad_x3[PARTITION_16x16] = sad_16_x3<16, 16>;
    p.sad_x3[PARTITION_16x24] = sad_16_x3<16, 24>;
    p.sad_x3[PARTITION_16x32] = sad_16_x3<16, 32>;
    p.sad_x3[PARTITION_16x64] = sad_16_x3<16, 64>;

    p.sad_x3[PARTITION_32x4] = sad_32_x3<32, 4>;
    p.sad_x3[PARTITION_32x8] = sad_32_x3<32, 8>;
    p.sad_x3[PARTITION_32x12] = sad_32_x3<32, 12>;
    p.sad_x3[PARTITION_32x16] = sad_32_x3<32, 16>;
    p.sad_x3[PARTITION_32x24] = sad_32_x3<32, 24>;
    p.sad_x3[PARTITION_32x32] = sad_32_x3<32, 32>;
    p.sad_x3[PARTITION_32x64] = sad_32_x3<32, 64>;

    p.sad_x3[PARTITION_64x4] = sad_64_x3<64, 4>;
    p.sad_x3[PARTITION_64x8] = sad_64_x3<64, 8>;
    p.sad_x3[PARTITION_64x12] = sad_64_x3<64,12>;
    p.sad_x3[PARTITION_64x16] = sad_64_x3<64, 16>;
    p.sad_x3[PARTITION_64x24] = sad_64_x3<64, 24>;
    p.sad_x3[PARTITION_64x32] = sad_64_x3<64, 32>;
    p.sad_x3[PARTITION_64x64] = sad_64_x3<64, 64>;

    // sad_x4
    p.sad_x4[PARTITION_4x4] = sad_4_4_x4<4, 4>;
    p.sad_x4[PARTITION_4x8] = sad_4_x4<4, 8>;
    p.sad_x4[PARTITION_4x12] = sad_4_4_x4<4, 12>;
    p.sad_x4[PARTITION_4x16] = sad_4_x4<4, 16>;
    p.sad_x4[PARTITION_4x24] = sad_4_x4<4, 24>;
    p.sad_x4[PARTITION_4x32] = sad_4_x4<4, 32>;
    p.sad_x4[PARTITION_4x64] = sad_4_x4<4, 64>;

    p.sad_x4[PARTITION_8x4] = sad_8_4_x4<8, 4>;
    p.sad_x4[PARTITION_8x8] = sad_8_x4<8, 8>;
    p.sad_x4[PARTITION_8x12] = sad_8_4_x4<8, 12>;
    p.sad_x4[PARTITION_8x16] = sad_8_x4<8, 16>;
    p.sad_x4[PARTITION_8x24] = sad_8_x4<8, 24>;
    p.sad_x4[PARTITION_8x32] = sad_8_x4<8, 32>;
    p.sad_x4[PARTITION_8x64] = sad_8_x4<8, 64>;

    p.sad_x4[PARTITION_12x4] = sad_12_4_x4<12, 4>;
    p.sad_x4[PARTITION_12x8] = sad_12_x4<12, 8>;
    p.sad_x4[PARTITION_12x12] = sad_12_4_x4<12, 12>;
    p.sad_x4[PARTITION_12x16] = sad_12_x4<12, 16>;
    p.sad_x4[PARTITION_12x24] = sad_12_x4<12, 24>;
    p.sad_x4[PARTITION_12x32] = sad_12_x4<12, 32>;
    p.sad_x4[PARTITION_12x64] = sad_12_x4<12, 64>;

    p.sad_x4[PARTITION_16x4] = sad_16_4_x4<16, 4>;
    p.sad_x4[PARTITION_16x8] = sad_16_x4<16, 8>;
    p.sad_x4[PARTITION_16x12] = sad_16_4_x4<16, 12>;
    p.sad_x4[PARTITION_16x16] = sad_16_x4<16, 16>;
    p.sad_x4[PARTITION_16x24] = sad_16_x4<16, 24>;
    p.sad_x4[PARTITION_16x32] = sad_16_x4<16, 32>;
    p.sad_x4[PARTITION_16x64] = sad_16_x4<16, 64>;

    p.sad_x4[PARTITION_32x4] = sad_32_x4<32, 4>;
    p.sad_x4[PARTITION_32x8] = sad_32_x4<32, 8>;
    p.sad_x4[PARTITION_32x12] = sad_32_x4<32, 12>;
    p.sad_x4[PARTITION_32x16] = sad_32_x4<32, 16>;
    p.sad_x4[PARTITION_32x24] = sad_32_x4<32, 24>;
    p.sad_x4[PARTITION_32x32] = sad_32_x4<32, 32>;
    p.sad_x4[PARTITION_32x64] = sad_32_x4<32, 64>;

    p.sad_x4[PARTITION_64x4] = sad_64_x4<64, 4>;
    p.sad_x4[PARTITION_64x8] = sad_64_x4<64, 8>;
    p.sad_x4[PARTITION_64x12] = sad_64_x4<64,12>;
    p.sad_x4[PARTITION_64x16] = sad_64_x4<64, 16>;
    p.sad_x4[PARTITION_64x24] = sad_64_x4<64, 24>;
    p.sad_x4[PARTITION_64x32] = sad_64_x4<64, 32>;
    p.sad_x4[PARTITION_64x64] = sad_64_x4<64, 64>;

    // satd
    p.satd[PARTITION_4x4] = satd_4x4;
    p.satd[PARTITION_4x8] = satd<4, 8>;
    p.satd[PARTITION_16x4] = satd<16, 4>;
    // p.satd[PARTITION_8x4] = satd<8, 4>;  // slower than SWAR C version
    //p.sa8d_8x8 = sa8d_8x8;
#else
#endif
}
