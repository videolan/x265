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
    Vec8us sum(0);
    Vec16uc m1, n1;

    for (int row = 0; row < ly; row++)
    {
        m1.fromUint32(*(uint32_t*)piOrg);
        n1.fromUint32(*(uint32_t*)piCur);
        sum.addSumAbsDiff(m1, n1);

        piOrg += strideOrg;
        piCur += strideCur;
    }

    return horizontal_add(sum);
}

template<int ly>
int CDECL sad_8(pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    Vec8us sum(0);
    Vec16uc m1, n1;
    for (int row = 0; row < ly; row++)
    {
        m1.load(piOrg);
        n1.load(piCur);
        sum.addSumAbsDiff(m1, n1);

        piOrg += strideOrg;
        piCur += strideCur;
    }
    return sum[0];
}

template<int ly>
int CDECL sad_16(pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    Vec8us sum(0);
    Vec16uc m1, n1;

    for (int row = 0; row < ly; row++)
    {
        m1.load_a(piOrg);
        n1.load(piCur);
        sum.addSumAbsDiff(m1, n1);

        piOrg += strideOrg;
        piCur += strideCur;
    }
    return horizontal_add_x(sum);
}

template<int lx, int ly>
int CDECL sad_32(pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    Vec8us sad(0);
    Vec16uc m1, n1;

    // TODO: AVX2
    for (int row = 0; row < ly; row++)
    {
        for (int col = 0; col < lx; col += 32)
        {
            m1.load_a(piOrg + col);
            n1.load(piCur + col);
            sad.addSumAbsDiff(m1, n1);
            m1.load_a(piOrg + col + 16);
            n1.load(piCur + col + 16);
            sad.addSumAbsDiff(m1, n1);
        }

        piOrg += strideOrg;
        piCur += strideCur;
    }
    return horizontal_add_x(sad);
}

template<int ly>
void CDECL sad_4_x3(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, intptr_t strideCur, int *res)
{    
    Vec16uc m1, n1, n2, n3;
    Vec4i sum1_low(0);
    Vec4i sum2_low(0);
    Vec4i sum3_low(0);
    Vec8us sum1(0);
    Vec8us sum2(0);
    Vec8us sum3(0);
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

            sum1.addSumAbsDiff(m1, n1);
            sum2.addSumAbsDiff(m1, n2);
            sum3.addSumAbsDiff(m1, n3);

            piOrg += FENC_STRIDE;
            piCur1 += strideCur;
            piCur2 += strideCur;
            piCur3 += strideCur;
        }

        sum1_low += extend_low(sum1);
        sum2_low += extend_low(sum2);
        sum3_low += extend_low(sum3);
        sum1 = 0;
        sum2 = 0;
        sum3 = 0;
    }
    for (; row < ly; row++)
    {
        m1.fromUint32(*(uint32_t*)piOrg);
        n1.fromUint32(*(uint32_t*)piCur1);
        n2.fromUint32(*(uint32_t*)piCur2);
        n3.fromUint32(*(uint32_t*)piCur3);

        sum1.addSumAbsDiff(m1, n1);
        sum2.addSumAbsDiff(m1, n2);
        sum3.addSumAbsDiff(m1, n3);

        piOrg += FENC_STRIDE;
        piCur1 += strideCur;
        piCur2 += strideCur;
        piCur3 += strideCur;
    }
    sum1_low += extend_low(sum1);
    sum2_low += extend_low(sum2);
    sum3_low += extend_low(sum3);

    res[0] = horizontal_add(sum1_low);
    res[1] = horizontal_add(sum2_low);
    res[2] = horizontal_add(sum3_low);
}

/* For performance - This function assumes that the *last load* can access 16 elements. */
template<int lx, int ly>
void CDECL sad_8_x3(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, intptr_t strideCur, int *res)
{    
    Vec16uc m1, n1, n2, n3;
    Vec4i sum1_low(0);
    Vec4i sum2_low(0);
    Vec4i sum3_low(0);
    Vec8us sum1(0);
    Vec8us sum2(0);
    Vec8us sum3(0);
    int main_iters = (ly >> 3) << 3;
    int row;

    for (row = 0; row < main_iters; row += 8)
    {
        for(int temp_rows = 0; temp_rows < 8; temp_rows++)
        {
            for (int col = 0; col < lx; col += 8)
            {
                m1.load_a(piOrg + col);
                n1.load(piCur1 + col);
                n2.load(piCur2 + col);
                n3.load(piCur3 + col);

                sum1.addSumAbsDiff(m1, n1);
                sum2.addSumAbsDiff(m1, n2);
                sum3.addSumAbsDiff(m1, n3);
            }
            piOrg += FENC_STRIDE;
            piCur1 += strideCur;
            piCur2 += strideCur;
            piCur3 += strideCur;
        }
        sum1_low += extend_low(sum1);
        sum2_low += extend_low(sum2);
        sum3_low += extend_low(sum3);
        sum1 = 0; sum2 = 0; sum3 = 0;
    }
    for (; row < ly; row++)
    {
        for (int col = 0; col < lx; col += 8)
        {
            m1.load_a(piOrg + col);
            n1.load(piCur1 + col);
            n2.load(piCur2 + col);
            n3.load(piCur3 + col);

            sum1.addSumAbsDiff(m1, n1);
            sum2.addSumAbsDiff(m1, n2);
            sum3.addSumAbsDiff(m1, n3);
        }

        piOrg += FENC_STRIDE;
        piCur1 += strideCur;
        piCur2 += strideCur;
        piCur3 += strideCur;
    }

    sum1_low += extend_low(sum1);
    sum2_low += extend_low(sum2);
    sum3_low += extend_low(sum3);

    res[0] = horizontal_add(sum1_low);
    res[1] = horizontal_add(sum2_low);
    res[2] = horizontal_add(sum3_low);
}

/* For performance - This function assumes that the *last load* can access 16 elements. */
template<int ly>
void CDECL sad_12_x3(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, intptr_t strideCur, int *res)
{    
    Vec16uc m1, n1, n2, n3;
    Vec4i sum1_low(0);
    Vec4i sum2_low(0);
    Vec4i sum3_low(0);    
    Vec8us sum1(0);
    Vec8us sum2(0);
    Vec8us sum3(0);
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

            sum1.addSumAbsDiff(m1, n1);
            sum2.addSumAbsDiff(m1, n2);
            sum3.addSumAbsDiff(m1, n3);

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
        m1.load_a(piOrg); m1.cutoff(12);
        n1.load(piCur1); n1.cutoff(12);
        n2.load(piCur2); n2.cutoff(12);
        n3.load(piCur3); n3.cutoff(12);

        sum1.addSumAbsDiff(m1, n1);
        sum2.addSumAbsDiff(m1, n2);
        sum3.addSumAbsDiff(m1, n3);

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

template<int lx, int ly>
void CDECL sad_16_x3(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, intptr_t strideCur, int *res)
{    
    Vec16uc m1, n1, n2, n3;
    Vec4i sum1_low(0);
    Vec4i sum2_low(0);
    Vec4i sum3_low(0);
    Vec8us sum1(0);
    Vec8us sum2(0);
    Vec8us sum3(0);
    int main_iters = (ly >> 3) << 3;
    int row;

    for (row = 0; row < main_iters; row += 8)
    {
        for(int i = 0; i < 8; i++)
        {
            for (int col = 0; col < lx; col += 16)
            {
                m1.load_a(piOrg + col);
                n1.load(piCur1 + col);
                n2.load(piCur2 + col);
                n3.load(piCur3 + col);

                sum1.addSumAbsDiff(m1, n1);
                sum2.addSumAbsDiff(m1, n2);
                sum3.addSumAbsDiff(m1, n3);
            }

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
        for (int col = 0; col < lx; col += 16)
        {
            m1.load_a(piOrg + col);
            n1.load(piCur1 + col);
            n2.load(piCur2 + col);
            n3.load(piCur3 + col);

            sum1.addSumAbsDiff(m1, n1);
            sum2.addSumAbsDiff(m1, n2);
            sum3.addSumAbsDiff(m1, n3);
        }
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

template<int lx, int ly>
void CDECL sad_32_x3(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, intptr_t strideCur, int *res)
{    
    Vec16uc m1, n1, n2, n3;
    Vec4i sum1_low(0);
    Vec4i sum2_low(0);
    Vec4i sum3_low(0);
    Vec8us sum1(0);
    Vec8us sum2(0);
    Vec8us sum3(0);
    int row;

    for (row = 0; row < ly-7; row+=8)
    {
        for(int temp_rows = 0; temp_rows < 8; temp_rows++)
        {
            for (int col = 0; col < lx; col += 32)
            {
                m1.load_a(piOrg + col);
                n1.load(piCur1 + col);
                n2.load(piCur2 + col);
                n3.load(piCur3 + col);

                sum1.addSumAbsDiff(m1, n1);
                sum2.addSumAbsDiff(m1, n2);
                sum3.addSumAbsDiff(m1, n3);

                m1.load_a(piOrg + col + 16);
                n1.load(piCur1 + col + 16);
                n2.load(piCur2 + col + 16);
                n3.load(piCur3 + col + 16);

                sum1.addSumAbsDiff(m1, n1);
                sum2.addSumAbsDiff(m1, n2);
                sum3.addSumAbsDiff(m1, n3);
            }

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
        for (int col = 0; col < lx; col += 32)
        {
            m1.load_a(piOrg + col);
            n1.load(piCur1 + col);
            n2.load(piCur2 + col);
            n3.load(piCur3 + col);

            sum1.addSumAbsDiff(m1, n1);
            sum2.addSumAbsDiff(m1, n2);
            sum3.addSumAbsDiff(m1, n3);

            m1.load_a(piOrg + col + 16);
            n1.load(piCur1 + col + 16);
            n2.load(piCur2 + col + 16);
            n3.load(piCur3 + col + 16);

            sum1.addSumAbsDiff(m1, n1);
            sum2.addSumAbsDiff(m1, n2);
            sum3.addSumAbsDiff(m1, n3);
        }
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

template<int lx, int ly>
void CDECL sad_64_x3(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, intptr_t strideCur, int *res)
{    
    Vec16uc m1, n1, n2, n3;
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
            for (int col = 0; col < lx; col += 64)
            {
                m1.load_a(piOrg + col);
                n1.load(piCur1 + col);
                n2.load(piCur2 + col);
                n3.load(piCur3 + col);

                sum1 += Vec8s(m1.sad(n1));
                sum2 += Vec8s(m1.sad(n2));
                sum3 += Vec8s(m1.sad(n3));

                m1.load_a(piOrg + col + 16);
                n1.load(piCur1 + col + 16);
                n2.load(piCur2 + col + 16);
                n3.load(piCur3 + col + 16);

                sum1 += Vec8s(m1.sad(n1));
                sum2 += Vec8s(m1.sad(n2));
                sum3 += Vec8s(m1.sad(n3));

                m1.load_a(piOrg + col + 32);
                n1.load(piCur1 + col + 32);
                n2.load(piCur2 + col + 32);
                n3.load(piCur3 + col + 32);

                sum1 += Vec8s(m1.sad(n1));
                sum2 += Vec8s(m1.sad(n2));
                sum3 += Vec8s(m1.sad(n3));

                m1.load_a(piOrg + col + 48);
                n1.load(piCur1 + col + 48);
                n2.load(piCur2 + col + 48);
                n3.load(piCur3 + col + 48);

                sum1 += Vec8s(m1.sad(n1));
                sum2 += Vec8s(m1.sad(n2));
                sum3 += Vec8s(m1.sad(n3));
            }
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
        for (int col = 0; col < lx; col += 64)
        {
            m1.load_a(piOrg + col);
            n1.load(piCur1 + col);
            n2.load(piCur2 + col);
            n3.load(piCur3 + col);

            sum1 += Vec8s(m1.sad(n1));
            sum2 += Vec8s(m1.sad(n2));
            sum3 += Vec8s(m1.sad(n3));

            m1.load_a(piOrg + col + 16);
            n1.load(piCur1 + col + 16);
            n2.load(piCur2 + col + 16);
            n3.load(piCur3 + col + 16);

            sum1 += Vec8s(m1.sad(n1));
            sum2 += Vec8s(m1.sad(n2));
            sum3 += Vec8s(m1.sad(n3));

            m1.load_a(piOrg + col + 32);
            n1.load(piCur1 + col + 32);
            n2.load(piCur2 + col + 32);
            n3.load(piCur3 + col + 32);

            sum1 += Vec8s(m1.sad(n1));
            sum2 += Vec8s(m1.sad(n2));
            sum3 += Vec8s(m1.sad(n3));

            m1.load_a(piOrg + col + 48);
            n1.load(piCur1 + col + 48);
            n2.load(piCur2 + col + 48);
            n3.load(piCur3 + col + 48);

            sum1 += Vec8s(m1.sad(n1));
            sum2 += Vec8s(m1.sad(n2));
            sum3 += Vec8s(m1.sad(n3));
        }
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


template<int lx, int ly>
void CDECL sad_4_x4(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, intptr_t strideCur, int *res)
{    
    Vec16uc m1, n1, n2, n3, n4;
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
                m1.fromUint32(*(uint32_t*)piOrg);
                n1.fromUint32(*(uint32_t*)piCur1);
                n2.fromUint32(*(uint32_t*)piCur2);
                n3.fromUint32(*(uint32_t*)piCur3);
                n4.fromUint32(*(uint32_t*)piCur4);

                sum1 += Vec8s(m1.sad(n1));
                sum2 += Vec8s(m1.sad(n2));
                sum3 += Vec8s(m1.sad(n3));
                sum4 += Vec8s(m1.sad(n4));
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
        for (int col = 0; col < lx; col += 4)
        {
            m1.fromUint32(*(uint32_t*)piOrg);
            n1.fromUint32(*(uint32_t*)piCur1);
            n2.fromUint32(*(uint32_t*)piCur2);
            n3.fromUint32(*(uint32_t*)piCur3);
            n4.fromUint32(*(uint32_t*)piCur4);

            sum1 += Vec8s(m1.sad(n1));
            sum2 += Vec8s(m1.sad(n2));
            sum3 += Vec8s(m1.sad(n3));
            sum4 += Vec8s(m1.sad(n4));
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
void CDECL sad_4_4_x4(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, intptr_t strideCur, int *res)
{    
    Vec16uc m1, n1, n2, n3, n4;
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
                m1.fromUint32(*(uint32_t*)piOrg);
                n1.fromUint32(*(uint32_t*)piCur1);
                n2.fromUint32(*(uint32_t*)piCur2);
                n3.fromUint32(*(uint32_t*)piCur3);
                n4.fromUint32(*(uint32_t*)piCur4);

                sum1 += Vec8s(m1.sad(n1));
                sum2 += Vec8s(m1.sad(n2));
                sum3 += Vec8s(m1.sad(n3));
                sum4 += Vec8s(m1.sad(n4));
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
        for (int col = 0; col < lx; col += 4)
        {
            m1.fromUint32(*(uint32_t*)piOrg);
            n1.fromUint32(*(uint32_t*)piCur1);
            n2.fromUint32(*(uint32_t*)piCur2);
            n3.fromUint32(*(uint32_t*)piCur3);
            n4.fromUint32(*(uint32_t*)piCur4);

            sum1 += Vec8s(m1.sad(n1));
            sum2 += Vec8s(m1.sad(n2));
            sum3 += Vec8s(m1.sad(n3));
            sum4 += Vec8s(m1.sad(n4));
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

/* For performance - This function assumes that the *last load* can access 16 elements. */
template<int lx, int ly>
void CDECL sad_8_x4(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, intptr_t strideCur, int *res)
{    
    Vec16uc m1, n1, n2, n3, n4;
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
            for (int col = 0; col < lx; col += 8)
            {
                m1.load_a(piOrg + col);
                n1.load(piCur1 + col);
                n2.load(piCur2 + col);
                n3.load(piCur3 + col);
                n4.load(piCur4 + col);

                sum1 += Vec8s(m1.sad(n1));
                sum2 += Vec8s(m1.sad(n2));
                sum3 += Vec8s(m1.sad(n3));
                sum4 += Vec8s(m1.sad(n4));
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
        for (int col = 0; col < lx; col += 8)
        {
            m1.load_a(piOrg + col);
            n1.load(piCur1 + col);
            n2.load(piCur2 + col);
            n3.load(piCur3 + col);
            n4.load(piCur4 + col);

            sum1 += Vec8s(m1.sad(n1));
            sum2 += Vec8s(m1.sad(n2));
            sum3 += Vec8s(m1.sad(n3));
            sum4 += Vec8s(m1.sad(n4));
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

/* For performance - This function assumes that the *last load* can access 16 elements. */
template<int lx, int ly>
void CDECL sad_8_4_x4(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, intptr_t strideCur, int *res)
{    
    Vec16uc m1, n1, n2, n3, n4;
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
            for (int col = 0; col < lx; col += 8)
            {
                m1.load_a(piOrg + col);
                n1.load(piCur1 + col);
                n2.load(piCur2 + col);
                n3.load(piCur3 + col);
                n4.load(piCur4 + col);

                sum1 += Vec8s(m1.sad(n1));
                sum2 += Vec8s(m1.sad(n2));
                sum3 += Vec8s(m1.sad(n3));
                sum4 += Vec8s(m1.sad(n4));
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
        for (int col = 0; col < lx; col += 8)
        {
            m1.load_a(piOrg + col);
            n1.load(piCur1 + col);
            n2.load(piCur2 + col);
            n3.load(piCur3 + col);
            n4.load(piCur4 + col);

            sum1 += Vec8s(m1.sad(n1));
            sum2 += Vec8s(m1.sad(n2));
            sum3 += Vec8s(m1.sad(n3));
            sum4 += Vec8s(m1.sad(n4));
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

/* For performance - This function assumes that the *last load* can access 16 elements. */
template<int lx, int ly>
void CDECL sad_12_x4(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, intptr_t strideCur, int *res)
{    
    Vec16uc m1, n1, n2, n3, n4;
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
            for (int col = 0; col < lx; col += 12)
            {
                m1.load_a(piOrg + col);
                n1.load(piCur1 + col);
                n2.load(piCur2 + col);
                n3.load(piCur3 + col);
                n4.load(piCur4 + col);

                sum1 += Vec8s(m1.sad(n1));
                sum2 += Vec8s(m1.sad(n2));
                sum3 += Vec8s(m1.sad(n3));
                sum4 += Vec8s(m1.sad(n4));

                m1.fromUint32(*(uint32_t*)(piOrg + 8));
                n1.fromUint32(*(uint32_t*)(piCur1 + 8));
                n2.fromUint32(*(uint32_t*)(piCur2 + 8));
                n3.fromUint32(*(uint32_t*)(piCur3 + 8));
                n4.fromUint32(*(uint32_t*)(piCur4 + 8));

                sum1 += Vec8s(m1.sad(n1));
                sum2 += Vec8s(m1.sad(n2));
                sum3 += Vec8s(m1.sad(n3));
                sum4 += Vec8s(m1.sad(n4));
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

            sum1 += Vec8s(m1.sad(n1));
            sum2 += Vec8s(m1.sad(n2));
            sum3 += Vec8s(m1.sad(n3));
            sum4 += Vec8s(m1.sad(n4));

            m1.fromUint32(*(uint32_t*)(piOrg + 8));
            n1.fromUint32(*(uint32_t*)(piCur1 + 8));
            n2.fromUint32(*(uint32_t*)(piCur2 + 8));
            n3.fromUint32(*(uint32_t*)(piCur3 + 8));
            n4.fromUint32(*(uint32_t*)(piCur4 + 8));

            sum1 += Vec8s(m1.sad(n1));
            sum2 += Vec8s(m1.sad(n2));
            sum3 += Vec8s(m1.sad(n3));
            sum4 += Vec8s(m1.sad(n4));
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

/* For performance - This function assumes that the *last load* can access 16 elements. */
template<int lx, int ly>
void CDECL sad_12_4_x4(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, intptr_t strideCur, int *res)
{    
    Vec16uc m1, n1, n2, n3, n4;
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
            for (int col = 0; col < lx; col += 12)
            {
                m1.load_a(piOrg + col);
                n1.load(piCur1 + col);
                n2.load(piCur2 + col);
                n3.load(piCur3 + col);
                n4.load(piCur4 + col);

                sum1 += Vec8s(m1.sad(n1));
                sum2 += Vec8s(m1.sad(n2));
                sum3 += Vec8s(m1.sad(n3));
                sum4 += Vec8s(m1.sad(n4));

                m1.fromUint32(*(uint32_t*)(piOrg + 8));
                n1.fromUint32(*(uint32_t*)(piCur1 + 8));
                n2.fromUint32(*(uint32_t*)(piCur2 + 8));
                n3.fromUint32(*(uint32_t*)(piCur3 + 8));
                n4.fromUint32(*(uint32_t*)(piCur4 + 8));

                sum1 += Vec8s(m1.sad(n1));
                sum2 += Vec8s(m1.sad(n2));
                sum3 += Vec8s(m1.sad(n3));
                sum4 += Vec8s(m1.sad(n4));
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

            sum1 += Vec8s(m1.sad(n1));
            sum2 += Vec8s(m1.sad(n2));
            sum3 += Vec8s(m1.sad(n3));
            sum4 += Vec8s(m1.sad(n4));

            m1.fromUint32(*(uint32_t*)(piOrg + 8));
            n1.fromUint32(*(uint32_t*)(piCur1 + 8));
            n2.fromUint32(*(uint32_t*)(piCur2 + 8));
            n3.fromUint32(*(uint32_t*)(piCur3 + 8));
            n4.fromUint32(*(uint32_t*)(piCur4 + 8));

            sum1 += Vec8s(m1.sad(n1));
            sum2 += Vec8s(m1.sad(n2));
            sum3 += Vec8s(m1.sad(n3));
            sum4 += Vec8s(m1.sad(n4));
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
    Vec16uc m1, n1, n2, n3, n4;
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

                sum1 += Vec8s(m1.sad(n1));
                sum2 += Vec8s(m1.sad(n2));
                sum3 += Vec8s(m1.sad(n3));
                sum4 += Vec8s(m1.sad(n4));
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

            sum1 += Vec8s(m1.sad(n1));
            sum2 += Vec8s(m1.sad(n2));
            sum3 += Vec8s(m1.sad(n3));
            sum4 += Vec8s(m1.sad(n4));
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
void CDECL sad_16_4_x4(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, intptr_t strideCur, int *res)
{    
    Vec16uc m1, n1, n2, n3, n4;
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

                sum1 += Vec8s(m1.sad(n1));
                sum2 += Vec8s(m1.sad(n2));
                sum3 += Vec8s(m1.sad(n3));
                sum4 += Vec8s(m1.sad(n4));
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

            sum1 += Vec8s(m1.sad(n1));
            sum2 += Vec8s(m1.sad(n2));
            sum3 += Vec8s(m1.sad(n3));
            sum4 += Vec8s(m1.sad(n4));
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
    Vec16uc m1, n1, n2, n3, n4;
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

                sum1 += Vec8s(m1.sad(n1));
                sum2 += Vec8s(m1.sad(n2));
                sum3 += Vec8s(m1.sad(n3));
                sum4 += Vec8s(m1.sad(n4));

                m1.load_a(piOrg + col + 16);
                n1.load(piCur1 + col + 16);
                n2.load(piCur2 + col + 16);
                n3.load(piCur3 + col + 16);
                n4.load(piCur4 + col + 16);

                sum1 += Vec8s(m1.sad(n1));
                sum2 += Vec8s(m1.sad(n2));
                sum3 += Vec8s(m1.sad(n3));
                sum4 += Vec8s(m1.sad(n4));
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

            sum1 += Vec8s(m1.sad(n1));
            sum2 += Vec8s(m1.sad(n2));
            sum3 += Vec8s(m1.sad(n3));
            sum4 += Vec8s(m1.sad(n4));

            m1.load_a(piOrg + col + 16);
            n1.load(piCur1 + col + 16);
            n2.load(piCur2 + col + 16);
            n3.load(piCur3 + col + 16);
            n4.load(piCur4 + col + 16);

            sum1 += Vec8s(m1.sad(n1));
            sum2 += Vec8s(m1.sad(n2));
            sum3 += Vec8s(m1.sad(n3));
            sum4 += Vec8s(m1.sad(n4));
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
void CDECL sad_32_4_x4(pixel *piOrg, pixel *piCur1, pixel *piCur2, pixel *piCur3, pixel *piCur4, intptr_t strideCur, int *res)
{    
    Vec16uc m1, n1, n2, n3, n4;
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

                sum1 += Vec8s(m1.sad(n1));
                sum2 += Vec8s(m1.sad(n2));
                sum3 += Vec8s(m1.sad(n3));
                sum4 += Vec8s(m1.sad(n4));

                m1.load_a(piOrg + col + 16);
                n1.load(piCur1 + col + 16);
                n2.load(piCur2 + col + 16);
                n3.load(piCur3 + col + 16);
                n4.load(piCur4 + col + 16);

                sum1 += Vec8s(m1.sad(n1));
                sum2 += Vec8s(m1.sad(n2));
                sum3 += Vec8s(m1.sad(n3));
                sum4 += Vec8s(m1.sad(n4));
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

            sum1 += Vec8s(m1.sad(n1));
            sum2 += Vec8s(m1.sad(n2));
            sum3 += Vec8s(m1.sad(n3));
            sum4 += Vec8s(m1.sad(n4));

            m1.load_a(piOrg + col + 16);
            n1.load(piCur1 + col + 16);
            n2.load(piCur2 + col + 16);
            n3.load(piCur3 + col + 16);
            n4.load(piCur4 + col + 16);

            sum1 += Vec8s(m1.sad(n1));
            sum2 += Vec8s(m1.sad(n2));
            sum3 += Vec8s(m1.sad(n3));
            sum4 += Vec8s(m1.sad(n4));
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
    Vec16uc m1, n1, n2, n3, n4;
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
            for (int col = 0; col < lx; col += 64)
            {
                m1.load_a(piOrg + col);
                n1.load(piCur1 + col);
                n2.load(piCur2 + col);
                n3.load(piCur3 + col);
                n4.load(piCur4 + col);

                sum1 += Vec8s(m1.sad(n1));
                sum2 += Vec8s(m1.sad(n2));
                sum3 += Vec8s(m1.sad(n3));
                sum4 += Vec8s(m1.sad(n4));

                m1.load_a(piOrg + col + 16);
                n1.load(piCur1 + col + 16);
                n2.load(piCur2 + col + 16);
                n3.load(piCur3 + col + 16);
                n4.load(piCur4 + col + 16);

                sum1 += Vec8s(m1.sad(n1));
                sum2 += Vec8s(m1.sad(n2));
                sum3 += Vec8s(m1.sad(n3));
                sum4 += Vec8s(m1.sad(n4));

                m1.load_a(piOrg + col + 32);
                n1.load(piCur1 + col + 32);
                n2.load(piCur2 + col + 32);
                n3.load(piCur3 + col + 32);
                n4.load(piCur4 + col + 32);

                sum1 += Vec8s(m1.sad(n1));
                sum2 += Vec8s(m1.sad(n2));
                sum3 += Vec8s(m1.sad(n3));
                sum4 += Vec8s(m1.sad(n4));

                m1.load_a(piOrg + col + 48);
                n1.load(piCur1 + col + 48);
                n2.load(piCur2 + col + 48);
                n3.load(piCur3 + col + 48);
                n4.load(piCur4 + col + 48);

                sum1 += Vec8s(m1.sad(n1));
                sum2 += Vec8s(m1.sad(n2));
                sum3 += Vec8s(m1.sad(n3));
                sum4 += Vec8s(m1.sad(n4));
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

            sum1 += Vec8s(m1.sad(n1));
            sum2 += Vec8s(m1.sad(n2));
            sum3 += Vec8s(m1.sad(n3));
            sum4 += Vec8s(m1.sad(n4));

            m1.load_a(piOrg + col + 16);
            n1.load(piCur1 + col + 16);
            n2.load(piCur2 + col + 16);
            n3.load(piCur3 + col + 16);
            n4.load(piCur4 + col + 16);

            sum1 += Vec8s(m1.sad(n1));
            sum2 += Vec8s(m1.sad(n2));
            sum3 += Vec8s(m1.sad(n3));
            sum4 += Vec8s(m1.sad(n4));

            m1.load_a(piOrg + col + 32);
            n1.load(piCur1 + col + 32);
            n2.load(piCur2 + col + 32);
            n3.load(piCur3 + col + 32);
            n4.load(piCur4 + col + 32);

            sum1 += Vec8s(m1.sad(n1));
            sum2 += Vec8s(m1.sad(n2));
            sum3 += Vec8s(m1.sad(n3));
            sum4 += Vec8s(m1.sad(n4));

            m1.load_a(piOrg + col + 48);
            n1.load(piCur1 + col + 48);
            n2.load(piCur2 + col + 48);
            n3.load(piCur3 + col + 48);
            n4.load(piCur4 + col + 48);

            sum1 += Vec8s(m1.sad(n1));
            sum2 += Vec8s(m1.sad(n2));
            sum3 += Vec8s(m1.sad(n3));
            sum4 += Vec8s(m1.sad(n4));
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
