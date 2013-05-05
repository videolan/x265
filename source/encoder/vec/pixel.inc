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

/* File for pixels type-neutral code */

template<int ly>
int CDECL sad_4x16(pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    int sum = 0;
    for (int row = 0; row < ly; row += 16)
        sum += sad_4<16>(piOrg + row * strideOrg, strideOrg, piCur + row * strideCur, strideCur);
    return sum;
}

template<int ly>
int CDECL sad_8x8(pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    /* groups of 8x16 blocks, upcasting sum from short to int often enough to avoid overflow */
    int sum = 0;
    for (int row = 0; row < ly; row += 8)
        sum += sad_8<8>(piOrg + row * strideOrg, strideOrg, piCur + row * strideCur, strideCur);
    return sum;
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

#if HIGH_BIT_DEPTH
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
#endif

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
    p.sad[PARTITION_8x24] = sad_8x8<24>;
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
    p.sad[PARTITION_16x24] = sad_16x24<16,24>;
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
    p.sad[PARTITION_64x24] = sad_16x24<64, 24>;
    p.sad[PARTITION_64x32] = sad_16x16<64, 32>;
    p.sad[PARTITION_64x64] = sad_16x16<64, 64>;

    //sad_x3
    p.sad_x3[PARTITION_4x4] = sad_4_x3<4>;
    p.sad_x3[PARTITION_4x8] = sad_4_x3<8>;
    p.sad_x3[PARTITION_4x12] = sad_4_x3<12>;
    p.sad_x3[PARTITION_4x16] = sad_4_x3<16>;
    p.sad_x3[PARTITION_4x24] = sad_4_x3<24>;
    p.sad_x3[PARTITION_4x32] = sad_4_x3<32>;
    p.sad_x3[PARTITION_4x64] = sad_4_x3<64>;

    p.sad_x3[PARTITION_8x4] = sad_8_x3<4>;
    p.sad_x3[PARTITION_8x8] = sad_8_x3<8>;
    p.sad_x3[PARTITION_8x12] = sad_8_x3<12>;
    p.sad_x3[PARTITION_8x16] = sad_8_x3<16>;
    p.sad_x3[PARTITION_8x24] = sad_8_x3<24>;
    p.sad_x3[PARTITION_8x32] = sad_8_x3<32>;
    p.sad_x3[PARTITION_8x64] = sad_8_x3<64>;

    p.sad_x3[PARTITION_12x4] = sad_12_x3<4>;
    p.sad_x3[PARTITION_12x8] = sad_12_x3<8>;
    p.sad_x3[PARTITION_12x12] = sad_12_x3<12>;
    p.sad_x3[PARTITION_12x16] = sad_12_x3<16>;
    p.sad_x3[PARTITION_12x24] = sad_12_x3<24>;
    p.sad_x3[PARTITION_12x32] = sad_12_x3<32>;
    p.sad_x3[PARTITION_12x64] = sad_12_x3<64>;

    p.sad_x3[PARTITION_16x4] = sad_16_x3<4>;
    p.sad_x3[PARTITION_16x8] = sad_16_x3<8>;
    p.sad_x3[PARTITION_16x12] = sad_16_x3<12>;
    p.sad_x3[PARTITION_16x16] = sad_16_x3<16>;
    p.sad_x3[PARTITION_16x24] = sad_16_x3<24>;
    p.sad_x3[PARTITION_16x32] = sad_16_x3<32>;
    p.sad_x3[PARTITION_16x64] = sad_16_x3<64>;

    p.sad_x3[PARTITION_24x4] = sad_24_x3<4>;
    p.sad_x3[PARTITION_24x8] = sad_24_x3<8>;
    p.sad_x3[PARTITION_24x12] = sad_24_x3<12>;
    p.sad_x3[PARTITION_24x16] = sad_24_x3<16>;
    p.sad_x3[PARTITION_24x24] = sad_24_x3<24>;
    p.sad_x3[PARTITION_24x32] = sad_24_x3<32>;
    p.sad_x3[PARTITION_24x64] = sad_24_x3<64>;

    p.sad_x3[PARTITION_32x4] = sad_32_x3<4>;
    p.sad_x3[PARTITION_32x8] = sad_32_x3<8>;
    p.sad_x3[PARTITION_32x12] = sad_32_x3<12>;
    p.sad_x3[PARTITION_32x16] = sad_32_x3<16>;
    p.sad_x3[PARTITION_32x24] = sad_32_x3<24>;
    p.sad_x3[PARTITION_32x32] = sad_32_x3<32>;
    p.sad_x3[PARTITION_32x64] = sad_32_x3<64>;

    p.sad_x3[PARTITION_64x4] = sad_64_x3<4>;
    p.sad_x3[PARTITION_64x8] = sad_64_x3<8>;
    p.sad_x3[PARTITION_64x12] = sad_64_x3<12>;
    p.sad_x3[PARTITION_64x16] = sad_64_x3<16>;
    p.sad_x3[PARTITION_64x24] = sad_64_x3<24>;
    p.sad_x3[PARTITION_64x32] = sad_64_x3<32>;
    p.sad_x3[PARTITION_64x64] = sad_64_x3<64>;

    //// sad_x4
    p.sad_x4[PARTITION_4x4] = sad_4_x4<4>;
    p.sad_x4[PARTITION_4x8] = sad_4_x4<8>;
    p.sad_x4[PARTITION_4x12] = sad_4_x4<12>;
    p.sad_x4[PARTITION_4x16] = sad_4_x4<16>;
    p.sad_x4[PARTITION_4x24] = sad_4_x4<24>;
    p.sad_x4[PARTITION_4x32] = sad_4_x4<32>;
    p.sad_x4[PARTITION_4x64] = sad_4_x4<64>;

    p.sad_x4[PARTITION_8x4] = sad_8_x4<4>;
    p.sad_x4[PARTITION_8x8] = sad_8_x4<8>;
    p.sad_x4[PARTITION_8x12] = sad_8_x4<12>;
    p.sad_x4[PARTITION_8x16] = sad_8_x4<16>;
    p.sad_x4[PARTITION_8x24] = sad_8_x4<24>;
    p.sad_x4[PARTITION_8x32] = sad_8_x4<32>;
    p.sad_x4[PARTITION_8x64] = sad_8_x4<64>;

    p.sad_x4[PARTITION_12x4] = sad_12_x4<12, 4>;
    p.sad_x4[PARTITION_12x8] = sad_12_x4<12, 8>;
    p.sad_x4[PARTITION_12x12] = sad_12_x4<12, 12>;
    p.sad_x4[PARTITION_12x16] = sad_12_x4<12, 16>;
    p.sad_x4[PARTITION_12x24] = sad_12_x4<12, 24>;
    p.sad_x4[PARTITION_12x32] = sad_12_x4<12, 32>;
    p.sad_x4[PARTITION_12x64] = sad_12_x4<12, 64>;

    p.sad_x4[PARTITION_16x4] = sad_16_x4<16, 4>;
    p.sad_x4[PARTITION_16x8] = sad_16_x4<16, 8>;
    p.sad_x4[PARTITION_16x12] = sad_16_x4<16, 12>;
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

#if HIGH_BIT_DEPTH
    // satd
    p.satd[PARTITION_4x4] = satd_4x4;
    p.satd[PARTITION_4x8] = satd<4, 8>;
    p.satd[PARTITION_16x4] = satd<16, 4>;
    //p.sa8d_8x8 = sa8d_8x8;
#endif
}
