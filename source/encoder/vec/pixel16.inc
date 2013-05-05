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

/* For performance - This function assumes that the *last load* can access 8 elements. */
template<int ly>
void CDECL sad_4_x3(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, intptr_t strideCur, int *res)
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
            m1.load_a(piOrg);
            n1.load(piCur1);
            n2.load(piCur2);
            n3.load(piCur3);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
            
            piOrg += FENC_STRIDE;
            piCur1 += strideCur;
            piCur2 += strideCur;
            piCur3 += strideCur;
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
        
        m1.load_a(piOrg);
        n1.load(piCur1);
        n2.load(piCur2);
        n3.load(piCur3);

        sum1 += abs(m1 - n1);
        sum2 += abs(m1 - n2);
        sum3 += abs(m1 - n3);
        
        sum1_low += extend_low(sum1);
        sum2_low += extend_low(sum2);
        sum3_low += extend_low(sum3);
        piOrg += FENC_STRIDE;
        piCur1 += strideCur;
        piCur2 += strideCur;
        piCur3 += strideCur;
    }
    res[0] += horizontal_add_x(sum1_low);
    res[1] += horizontal_add_x(sum2_low);
    res[2] += horizontal_add_x(sum3_low);
}

template<int ly>
void CDECL sad_8_x3(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, intptr_t strideCur, int *res)
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
            m1.load_a(piOrg);
            n1.load(piCur1);
            n2.load(piCur2);
            n3.load(piCur3);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);

            piOrg += FENC_STRIDE;
            piCur1 += strideCur;
            piCur2 += strideCur;
            piCur3 += strideCur;
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

        m1.load_a(piOrg);
        n1.load(piCur1);
        n2.load(piCur2);
        n3.load(piCur3);

        sum1 += abs(m1 - n1);
        sum2 += abs(m1 - n2);
        sum3 += abs(m1 - n3);

        sum1_low += extend_low(sum1);
        sum1_high += extend_high(sum1);
        sum2_low += extend_low(sum2);
        sum2_high += extend_high(sum2);
        sum3_low += extend_low(sum3);
        sum3_high += extend_high(sum3);

        piOrg += FENC_STRIDE;
        piCur1 += strideCur;
        piCur2 += strideCur;
        piCur3 += strideCur;
    }
    res[0] += horizontal_add_x(sum1_low) + horizontal_add_x(sum1_high);
    res[1] += horizontal_add_x(sum2_low) + horizontal_add_x(sum2_high);
    res[2] += horizontal_add_x(sum3_low) + horizontal_add_x(sum3_high);
}

/* For performance - This function assumes that the *last load* can access 8 elements. */
template<int ly>
void CDECL sad_12_x3(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, intptr_t strideCur, int *res)
{    
    Vec8s m1, n1, n2, n3;
    Vec4i sum1_low(0);
    Vec4i sum2_low(0);
    Vec4i sum3_low(0);
    int row;

    for (row = 0; row < ly-7; row+=8)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        for(int temp_rows = 0; temp_rows < 8; temp_rows++)
        {
            m1.load_a(piOrg);
            n1.load(piCur1);
            n2.load(piCur2);
            n3.load(piCur3);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);

            m1.load(piOrg  + 4);
            n1.load(piCur1 + 4);
            n2.load(piCur2 + 4);
            n3.load(piCur3 + 4);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);

            m1.load_a(piOrg + 8);
            n1.load(piCur1 + 8);
            n2.load(piCur2 + 8);
            n3.load(piCur3 + 8);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);

            piOrg += FENC_STRIDE;
            piCur1 += strideCur;
            piCur2 += strideCur;
            piCur3 += strideCur;
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

        m1.load_a(piOrg);
        n1.load(piCur1);
        n2.load(piCur2);
        n3.load(piCur3);

        sum1 += abs(m1 - n1);
        sum2 += abs(m1 - n2);
        sum3 += abs(m1 - n3);

        m1.load(piOrg + 4);
        n1.load(piCur1 + 4);
        n2.load(piCur2 + 4);
        n3.load(piCur3 + 4);

        sum1 += abs(m1 - n1);
        sum2 += abs(m1 - n2);
        sum3 += abs(m1 - n3);

        m1.load_a(piOrg + 8);
        n1.load(piCur1 + 8);
        n2.load(piCur2 + 8);
        n3.load(piCur3 + 8);

        sum1 += abs(m1 - n1);
        sum2 += abs(m1 - n2);
        sum3 += abs(m1 - n3);

        sum1_low += extend_low(sum1);
        sum2_low += extend_low(sum2);
        sum3_low += extend_low(sum3);
        piOrg += FENC_STRIDE;
        piCur1 += strideCur;
        piCur2 += strideCur;
        piCur3 += strideCur;
    }

    res[0] = horizontal_add(sum1_low);
    res[1] = horizontal_add(sum2_low);
    res[2] = horizontal_add(sum3_low);
}

template<int ly>
void CDECL sad_16_x3(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, intptr_t strideCur, int *res)
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
            m1.load_a(piOrg);
            n1.load(piCur1);
            n2.load(piCur2);
            n3.load(piCur3);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);

            m1.load_a(piOrg + 8);
            n1.load(piCur1 + 8);
            n2.load(piCur2 + 8);
            n3.load(piCur3 + 8);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);

            piOrg += FENC_STRIDE;
            piCur1 += strideCur;
            piCur2 += strideCur;
            piCur3 += strideCur;
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

        m1.load_a(piOrg);
        n1.load(piCur1);
        n2.load(piCur2);
        n3.load(piCur3);

        sum1 += abs(m1 - n1);
        sum2 += abs(m1 - n2);
        sum3 += abs(m1 - n3);

        m1.load_a(piOrg + 8);
        n1.load(piCur1 + 8);
        n2.load(piCur2 + 8);
        n3.load(piCur3 + 8);

        sum1 += abs(m1 - n1);
        sum2 += abs(m1 - n2);
        sum3 += abs(m1 - n3);

        sum1_low += extend_low(sum1);
        sum1_high += extend_high(sum1);
        sum2_low += extend_low(sum2);
        sum2_high += extend_high(sum2);
        sum3_low += extend_low(sum3);
        sum3_high += extend_high(sum3);

        piOrg += FENC_STRIDE;
        piCur1 += strideCur;
        piCur2 += strideCur;
        piCur3 += strideCur;
    }
    res[0] += horizontal_add_x(sum1_low) + horizontal_add_x(sum1_high);
    res[1] += horizontal_add_x(sum2_low) + horizontal_add_x(sum2_high);
    res[2] += horizontal_add_x(sum3_low) + horizontal_add_x(sum3_high);
}

template<int ly>
void CDECL sad_24_x3(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, intptr_t strideCur, int *res)
{    
    Vec8s m1, n1, n2, n3;
    Vec4i sum1_low(0);
    Vec4i sum2_low(0);
    Vec4i sum3_low(0);
    Vec8us sum1(0);
    Vec8us sum2(0);
    Vec8us sum3(0);
    int row;

    for (row = 0; row < ly-7; row+=8)
    {
        for(int i = 0; i < 8; i++)
        {
            m1.load_a(piOrg);
            n1.load(piCur1);
            n2.load(piCur2);
            n3.load(piCur3);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);

            m1.load_a(piOrg + 8);
            n1.load(piCur1 + 8);
            n2.load(piCur2 + 8);
            n3.load(piCur3 + 8);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);

            m1.load_a(piOrg + 16);
            n1.load(piCur1 + 16);
            n2.load(piCur2 + 16);
            n3.load(piCur3 + 16);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);

            piOrg += FENC_STRIDE;
            piCur1 += strideCur;
            piCur2 += strideCur;
            piCur3 += strideCur;
        }

        sum1_low += extend_low(sum1) + extend_high(sum1);
        sum2_low += extend_low(sum2) + extend_high(sum2);
        sum3_low += extend_low(sum3) + extend_high(sum3);
        sum1 = 0; sum2 = 0; sum3 = 0;
    }
    for (; row < ly; row++)
    {
        m1.load_a(piOrg);
        n1.load(piCur1);
        n2.load(piCur2);
        n3.load(piCur3);

        sum1 += abs(m1 - n1);
        sum2 += abs(m1 - n2);
        sum3 += abs(m1 - n3);

        m1.load_a(piOrg + 8);
        n1.load(piCur1 + 8);
        n2.load(piCur2 + 8);
        n3.load(piCur3 + 8);

        sum1 += abs(m1 - n1);
        sum2 += abs(m1 - n2);
        sum3 += abs(m1 - n3);

        m1.load_a(piOrg + 16);
        n1.load(piCur1 + 16);
        n2.load(piCur2 + 16);
        n3.load(piCur3 + 16);

        sum1 += abs(m1 - n1);
        sum2 += abs(m1 - n2);
        sum3 += abs(m1 - n3);

        piOrg += FENC_STRIDE;
        piCur1 += strideCur;
        piCur2 += strideCur;
        piCur3 += strideCur;
    }

    sum1_low += extend_low(sum1) + extend_high(sum1);
    sum2_low += extend_low(sum2) + extend_high(sum2);
    sum3_low += extend_low(sum3) + extend_high(sum3);

    res[0] = horizontal_add(sum1_low);
    res[1] = horizontal_add(sum2_low);
    res[2] = horizontal_add(sum3_low);
}


template<int ly>
void CDECL sad_32_x3(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, intptr_t strideCur, int *res)
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
            m1.load_a(piOrg);
            n1.load(piCur1);
            n2.load(piCur2);
            n3.load(piCur3);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);

            m1.load_a(piOrg + 8);
            n1.load(piCur1 + 8);
            n2.load(piCur2 + 8);
            n3.load(piCur3 + 8);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);

            m1.load_a(piOrg + 16);
            n1.load(piCur1 + 16);
            n2.load(piCur2 + 16);
            n3.load(piCur3 + 16);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);

            m1.load_a(piOrg + 24);
            n1.load(piCur1 + 24);
            n2.load(piCur2 + 24);
            n3.load(piCur3 + 24);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
         
            piOrg += FENC_STRIDE;
            piCur1 += strideCur;
            piCur2 += strideCur;
            piCur3 += strideCur;
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

        m1.load_a(piOrg);
        n1.load(piCur1);
        n2.load(piCur2);
        n3.load(piCur3);

        sum1 += abs(m1 - n1);
        sum2 += abs(m1 - n2);
        sum3 += abs(m1 - n3);

        m1.load_a(piOrg + 8);
        n1.load(piCur1 + 8);
        n2.load(piCur2 + 8);
        n3.load(piCur3 + 8);

        sum1 += abs(m1 - n1);
        sum2 += abs(m1 - n2);
        sum3 += abs(m1 - n3);

        m1.load_a(piOrg + 16);
        n1.load(piCur1 + 16);
        n2.load(piCur2 + 16);
        n3.load(piCur3 + 16);

        sum1 += abs(m1 - n1);
        sum2 += abs(m1 - n2);
        sum3 += abs(m1 - n3);

        m1.load_a(piOrg + 24);
        n1.load(piCur1 + 24);
        n2.load(piCur2 + 24);
        n3.load(piCur3 + 24);

        sum1 += abs(m1 - n1);
        sum2 += abs(m1 - n2);
        sum3 += abs(m1 - n3);

        sum1_low += extend_low(sum1);
        sum1_high += extend_high(sum1);
        sum2_low += extend_low(sum2);
        sum2_high += extend_high(sum2);
        sum3_low += extend_low(sum3);
        sum3_high += extend_high(sum3);

        piOrg += FENC_STRIDE;
        piCur1 += strideCur;
        piCur2 += strideCur;
        piCur3 += strideCur;
    }

    res[0] += horizontal_add_x(sum1_low) + horizontal_add_x(sum1_high);
    res[1] += horizontal_add_x(sum2_low) + horizontal_add_x(sum2_high);
    res[2] += horizontal_add_x(sum3_low) + horizontal_add_x(sum3_high);
}

template<int ly>
void CDECL sad_64_x3(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, intptr_t strideCur, int *res)
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
            m1.load_a(piOrg);
            n1.load(piCur1);
            n2.load(piCur2);
            n3.load(piCur3);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);

            m1.load_a(piOrg + 8);
            n1.load(piCur1 + 8);
            n2.load(piCur2 + 8);
            n3.load(piCur3 + 8);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);

            m1.load_a(piOrg + 16);
            n1.load(piCur1 + 16);
            n2.load(piCur2 + 16);
            n3.load(piCur3 + 16);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);

            m1.load_a(piOrg + 24);
            n1.load(piCur1 + 24);
            n2.load(piCur2 + 24);
            n3.load(piCur3 + 24);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);

            m1.load_a(piOrg + 32);
            n1.load(piCur1 + 32);
            n2.load(piCur2 + 32);
            n3.load(piCur3 + 32);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);

            m1.load_a(piOrg + 40);
            n1.load(piCur1 + 40);
            n2.load(piCur2 + 40);
            n3.load(piCur3 + 40);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);

            m1.load_a(piOrg + 48);
            n1.load(piCur1 + 48);
            n2.load(piCur2 + 48);
            n3.load(piCur3 + 48);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);

            m1.load_a(piOrg + 56);
            n1.load(piCur1 + 56);
            n2.load(piCur2 + 56);
            n3.load(piCur3 + 56);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
        
            piOrg += FENC_STRIDE;
            piCur1 += strideCur;
            piCur2 += strideCur;
            piCur3 += strideCur;
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

        m1.load_a(piOrg);
        n1.load(piCur1);
        n2.load(piCur2);
        n3.load(piCur3);

        sum1 += abs(m1 - n1);
        sum2 += abs(m1 - n2);
        sum3 += abs(m1 - n3);

        m1.load_a(piOrg + 8);
        n1.load(piCur1 + 8);
        n2.load(piCur2 + 8);
        n3.load(piCur3 + 8);

        sum1 += abs(m1 - n1);
        sum2 += abs(m1 - n2);
        sum3 += abs(m1 - n3);

        m1.load_a(piOrg + 16);
        n1.load(piCur1 + 16);
        n2.load(piCur2 + 16);
        n3.load(piCur3 + 16);

        sum1 += abs(m1 - n1);
        sum2 += abs(m1 - n2);
        sum3 += abs(m1 - n3);

        m1.load_a(piOrg + 24);
        n1.load(piCur1 + 24);
        n2.load(piCur2 + 24);
        n3.load(piCur3 + 24);

        sum1 += abs(m1 - n1);
        sum2 += abs(m1 - n2);
        sum3 += abs(m1 - n3);

        m1.load_a(piOrg + 32);
        n1.load(piCur1 + 32);
        n2.load(piCur2 + 32);
        n3.load(piCur3 + 32);

        sum1 += abs(m1 - n1);
        sum2 += abs(m1 - n2);
        sum3 += abs(m1 - n3);

        m1.load_a(piOrg + 40);
        n1.load(piCur1 + 40);
        n2.load(piCur2 + 40);
        n3.load(piCur3 + 40);

        sum1 += abs(m1 - n1);
        sum2 += abs(m1 - n2);
        sum3 += abs(m1 - n3);

        m1.load_a(piOrg + 48);
        n1.load(piCur1 + 48);
        n2.load(piCur2 + 48);
        n3.load(piCur3 + 48);

        sum1 += abs(m1 - n1);
        sum2 += abs(m1 - n2);
        sum3 += abs(m1 - n3);

        m1.load_a(piOrg + 56);
        n1.load(piCur1 + 56);
        n2.load(piCur2 + 56);
        n3.load(piCur3 + 56);

        sum1 += abs(m1 - n1);
        sum2 += abs(m1 - n2);
        sum3 += abs(m1 - n3);

        sum1_low += extend_low(sum1);
        sum1_high += extend_high(sum1);
        sum2_low += extend_low(sum2);
        sum2_high += extend_high(sum2);
        sum3_low += extend_low(sum3);
        sum3_high += extend_high(sum3);

        piOrg += FENC_STRIDE;
        piCur1 += strideCur;
        piCur2 += strideCur;
        piCur3 += strideCur;
    }

    res[0] += horizontal_add_x(sum1_low) + horizontal_add_x(sum1_high);
    res[1] += horizontal_add_x(sum2_low) + horizontal_add_x(sum2_high);
    res[2] += horizontal_add_x(sum3_low) + horizontal_add_x(sum3_high);
}

/* For performance - This function assumes that the *last load* can access 8 elements. */
template<int ly>
void CDECL sad_4_x4(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, intptr_t strideCur, int *res)
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
            m1.load_a(piOrg);
            n1.load(piCur1);
            n2.load(piCur2);
            n3.load(piCur3);
            n4.load(piCur4);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
            sum4 += abs(m1 - n4);
        
            piOrg += FENC_STRIDE;
            piCur1 += strideCur;
            piCur2 += strideCur;
            piCur3 += strideCur;
            piCur4 += strideCur;
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

        m1.load_a(piOrg);
        n1.load(piCur1);
        n2.load(piCur2);
        n3.load(piCur3);
        n4.load(piCur4);

        sum1 += abs(m1 - n1);
        sum2 += abs(m1 - n2);
        sum3 += abs(m1 - n3);
        sum4 += abs(m1 - n4);

        sum1_low += extend_low(sum1);
        sum2_low += extend_low(sum2);
        sum3_low += extend_low(sum3);
        sum4_low += extend_low(sum4);
        piOrg += FENC_STRIDE;
        piCur1 += strideCur;
        piCur2 += strideCur;
        piCur3 += strideCur;
        piCur4 += strideCur;
    }
    res[0] += horizontal_add_x(sum1_low);
    res[1] += horizontal_add_x(sum2_low);
    res[2] += horizontal_add_x(sum3_low);
    res[3] += horizontal_add_x(sum4_low);
}

template<int ly>
void CDECL sad_8_x4(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, intptr_t strideCur, int *res)
{    
    Vec8s m1, n1, n2, n3, n4;
    Vec4i sum1_low(0), sum1_high(0);
    Vec4i sum2_low(0), sum2_high(0);
    Vec4i sum3_low(0), sum3_high(0);
    Vec4i sum4_low(0), sum4_high(0);
    int row;

    for (row = 0; row < ly-7; row+=8)
    {
        Vec8s sum1(0);
        Vec8s sum2(0);
        Vec8s sum3(0);
        Vec8s sum4(0);
        for(int temp_rows = 0; temp_rows < 8; temp_rows++)
        {
            m1.load_a(piOrg);
            n1.load(piCur1);
            n2.load(piCur2);
            n3.load(piCur3);
            n4.load(piCur4);

            sum1 += abs(m1 - n1);
            sum2 += abs(m1 - n2);
            sum3 += abs(m1 - n3);
            sum4 += abs(m1 - n4);

            piOrg += FENC_STRIDE;
            piCur1 += strideCur;
            piCur2 += strideCur;
            piCur3 += strideCur;
            piCur4 += strideCur;
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

        m1.load_a(piOrg);
        n1.load(piCur1);
        n2.load(piCur2);
        n3.load(piCur3);
        n4.load(piCur4);

        sum1 += abs(m1 - n1);
        sum2 += abs(m1 - n2);
        sum3 += abs(m1 - n3);
        sum4 += abs(m1 - n4);
        
        piOrg += FENC_STRIDE;
        piCur1 += strideCur;
        piCur2 += strideCur;
        piCur3 += strideCur;
        piCur4 += strideCur;

        sum1_low += extend_low(sum1);
        sum1_high += extend_high(sum1);
        sum2_low += extend_low(sum2);
        sum2_high += extend_high(sum2);
        sum3_low += extend_low(sum3);
        sum3_high += extend_high(sum3);
        sum4_low += extend_low(sum4);
        sum4_high += extend_high(sum4);
    }

    res[0] = horizontal_add_x(sum1_low) + horizontal_add_x(sum1_high);
    res[1] = horizontal_add_x(sum2_low) + horizontal_add_x(sum2_high);
    res[2] = horizontal_add_x(sum3_low) + horizontal_add_x(sum3_high);
    res[3] = horizontal_add_x(sum4_low) + horizontal_add_x(sum4_high);
}

/* For performance - This function assumes that the *last load* can access 8 elements. */
template<int lx, int ly>
void CDECL sad_12_x4(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, intptr_t strideCur, int *res)
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
            piCur1 += strideCur;
            piCur2 += strideCur;
            piCur3 += strideCur;
            piCur4 += strideCur;
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
        sum1_low += extend_low(sum1);
        sum2_low += extend_low(sum2);
        sum3_low += extend_low(sum3);
        sum4_low += extend_low(sum4);
        piOrg += FENC_STRIDE;
        piCur1 += strideCur;
        piCur2 += strideCur;
        piCur3 += strideCur;
        piCur4 += strideCur;
    }
    res[0] += horizontal_add_x(sum1_low);
    res[1] += horizontal_add_x(sum2_low);
    res[2] += horizontal_add_x(sum3_low);
    res[3] += horizontal_add_x(sum4_low);
}

template<int lx, int ly>
void CDECL sad_16_x4(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, intptr_t strideCur, int *res)
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
            piCur1 += strideCur;
            piCur2 += strideCur;
            piCur3 += strideCur;
            piCur4 += strideCur;
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
        piCur1 += strideCur;
        piCur2 += strideCur;
        piCur3 += strideCur;
        piCur4 += strideCur;
    }
    res[0] += horizontal_add_x(sum1_low) + horizontal_add_x(sum1_high);
    res[1] += horizontal_add_x(sum2_low) + horizontal_add_x(sum2_high);
    res[2] += horizontal_add_x(sum3_low) + horizontal_add_x(sum3_high);
    res[3] += horizontal_add_x(sum4_low) + horizontal_add_x(sum4_high);
}

template<int lx, int ly>
void CDECL sad_32_x4(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, intptr_t strideCur, int *res)
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
            piCur1 += strideCur;
            piCur2 += strideCur;
            piCur3 += strideCur;
            piCur4 += strideCur;
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
        piCur1 += strideCur;
        piCur2 += strideCur;
        piCur3 += strideCur;
        piCur4 += strideCur;
    }
    res[0] += horizontal_add_x(sum1_low) + horizontal_add_x(sum1_high);
    res[1] += horizontal_add_x(sum2_low) + horizontal_add_x(sum2_high);
    res[2] += horizontal_add_x(sum3_low) + horizontal_add_x(sum3_high);
    res[3] += horizontal_add_x(sum4_low) + horizontal_add_x(sum4_high);
}

template<int lx, int ly>
void CDECL sad_64_x4(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, intptr_t strideCur, int *res)
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
            piCur1 += strideCur;
            piCur2 += strideCur;
            piCur3 += strideCur;
            piCur4 += strideCur;
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
        piCur1 += strideCur;
        piCur2 += strideCur;
        piCur3 += strideCur;
        piCur4 += strideCur;
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
