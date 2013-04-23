/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
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
int CDECL sad_8x16(pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    int sum = 0;
    for (int row = 0; row < ly; row += 16)
        sum += sad_8<16>(piOrg + row * strideOrg, strideOrg, piCur + row * strideCur, strideCur);
    return sum;
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
int CDECL sad_8x16(pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    /* groups of 8x16 blocks, upcasting sum from short to int often enough to avoid overflow */
    int sum = 0;
    for (int row = 0; row < ly; row += 16)
        sum += sad_8<16>(piOrg + row * strideOrg, strideOrg, piCur + row * strideCur, strideCur);
    return sum;
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
    p.sad[PARTITION_4x4] = sad_4<4>;
    p.sad[PARTITION_4x8] = sad_4<8>;
    p.sad[PARTITION_8x4] = sad_8<4>;
    p.sad[PARTITION_8x8] = sad_8<8>;
    p.sad[PARTITION_16x4] = sad_16<4>;
    p.sad[PARTITION_4x16] = sad_4<16>;
    p.sad[PARTITION_16x8] = sad_16<8>;
    p.sad[PARTITION_8x16] = sad_8<16>;
    p.sad[PARTITION_16x16] = sad_16<16>;
    p.sad[PARTITION_4x32] = sad_4<32>;
    p.sad[PARTITION_32x4] = sad_32<32, 4>;
    p.sad[PARTITION_8x32] = sad_8x16<32>;
    p.sad[PARTITION_32x8] = sad_32<32, 8>;
    p.sad[PARTITION_16x32] = sad_16x16<16, 32>;
    p.sad[PARTITION_32x16] = sad_16x16<32, 16>;
    p.sad[PARTITION_32x32] = sad_16x16<32, 32>;
    p.sad[PARTITION_4x64] = sad_4<64>;
    p.sad[PARTITION_64x4] = sad_32<64, 4>;
    p.sad[PARTITION_64x8] = sad_32<64, 8>;
    p.sad[PARTITION_8x64] = sad_8x16<64>;
    p.sad[PARTITION_16x64] = sad_16x16<16, 64>;
    p.sad[PARTITION_64x16] = sad_16x16<64, 16>;
    p.sad[PARTITION_32x64] = sad_16x16<32, 64>;
    p.sad[PARTITION_64x32] = sad_16x16<64, 32>;
    p.sad[PARTITION_64x64] = sad_16x16<64, 64>;

#if HIGH_BIT_DEPTH
    p.satd[PARTITION_4x4] = satd_4x4;
    p.satd[PARTITION_4x8] = satd<4, 8>;
    p.satd[PARTITION_16x4] = satd<16, 4>;
    // p.satd[PARTITION_8x4] = satd<8, 4>;  // slower than SWAR C version
    p.sa8d_8x8 = sa8d_8x8;
#else
#endif
}
