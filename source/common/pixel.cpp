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

#include "primitives.h"
#include <algorithm>
#include <stdlib.h> // abs()

template<typename T>
inline T ClipY(T x) { return std::min<T>(T((1 << 8) - 1), std::max<T>(T(0), x)); }


#define SET_FUNC_PRIMITIVE_TABLE_C(FUNC_PREFIX, FUNC_PREFIX_DEF, FUNC_TYPE_CAST, DATA_TYPE1, DATA_TYPE2) \
    p. FUNC_PREFIX [PARTITION_4x4] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 4, 4, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_4x8]   = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 4, 8, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_4x12]  = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 4, 12, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_4x16]  = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 4, 16, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_4x24]  = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 4, 24, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_4x32]  = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 4, 32, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_4x48]  = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 4, 48, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_4x64]  = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 4, 64, DATA_TYPE1, DATA_TYPE2 >;  \
                                                                                                                   \
    p. FUNC_PREFIX [PARTITION_8x4]   = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 8, 4, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_8x8]   = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 8, 8, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_8x12]  = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 8, 12, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_8x16]  = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 8, 16, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_8x24]  = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 8, 24, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_8x32]  = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 8, 32, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_8x48]  = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 8, 48, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_8x64]  = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 8, 64, DATA_TYPE1, DATA_TYPE2 >;  \
                                                                                                                   \
    p. FUNC_PREFIX [PARTITION_12x4]  = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 12, 4, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_12x8]  = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 12, 8, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_12x12] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 12, 12, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_12x16] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 12, 16, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_12x24] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 12, 24, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_12x32] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 12, 32, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_12x48] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 12, 48, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_12x64] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 12, 64, DATA_TYPE1, DATA_TYPE2 >;  \
                                                                                                                   \
    p. FUNC_PREFIX [PARTITION_16x4]  = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 16, 4, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_16x8]  = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 16, 8, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_16x12] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 16, 12, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_16x16] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 16, 16, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_16x24] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 16, 24, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_16x32] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 16, 32, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_16x48] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 16, 48, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_16x64] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 16, 64, DATA_TYPE1, DATA_TYPE2 >;  \
                                                                                                                   \
    p. FUNC_PREFIX [PARTITION_24x4]  = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 24, 4, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_24x8]  = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 24, 8, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_24x12] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 24, 12, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_24x16] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 24, 16, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_24x24] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 24, 24, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_24x32] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 24, 32, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_24x48] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 24, 48, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_24x64] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 24, 64, DATA_TYPE1, DATA_TYPE2 >;  \
                                                                                                                   \
    p. FUNC_PREFIX [PARTITION_32x4]  = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 32, 4, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_32x8]  = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 32, 8, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_32x12] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 32, 12, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_32x16] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 32, 16, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_32x24] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 32, 24, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_32x32] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 32, 32, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_32x48] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 32, 48, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_32x64] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 32, 64, DATA_TYPE1, DATA_TYPE2 >;  \
                                                                                                                   \
    p. FUNC_PREFIX [PARTITION_48x4]  = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 48, 4, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_48x8]  = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 48, 8, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_48x12] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 48, 12, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_48x16] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 48, 16, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_48x24] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 48, 24, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_48x32] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 48, 32, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_48x48] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 48, 48, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_48x64] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 48, 64, DATA_TYPE1, DATA_TYPE2 >;  \
                                                                                                                   \
    p. FUNC_PREFIX [PARTITION_64x4]  = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 64, 4, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_64x8]  = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 64, 8, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_64x12] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 64, 12, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_64x16] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 64, 16, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_64x24] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 64, 24, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_64x32] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 64, 32, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_64x48] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 64, 48, DATA_TYPE1, DATA_TYPE2 >;  \
    p. FUNC_PREFIX [PARTITION_64x64] = (FUNC_TYPE_CAST)FUNC_PREFIX_DEF < 64, 64, DATA_TYPE1, DATA_TYPE2 >;  \

namespace {
// place functions in anonymous namespace (file static)

template<int lx, int ly>
int CDECL sad(pixel *pix1, intptr_t stride_pix1, pixel *pix2, intptr_t stride_pix2)
{
    int sum = 0;

    for (int y = 0; y < ly; y++)
    {
        for (int x = 0; x < lx; x++)
        {
            sum += abs(pix1[x] - pix2[x]);
        }

        pix1 += stride_pix1;
        pix2 += stride_pix2;
    }

    return sum;
}

template<int lx, int ly>
void CDECL sad_x3(pixel *pix1, pixel *pix2, pixel *pix3, pixel *pix4, intptr_t strideCur, int *res)
{
    res[0] = 0;
    res[1] = 0;
    res[2] = 0;
    for (int y = 0; y < ly; y++)
    {
        for (int x = 0; x < lx; x++)
        {
            res[0] += abs(pix1[x] - pix2[x]);
            res[1] += abs(pix1[x] - pix3[x]);
            res[2] += abs(pix1[x] - pix4[x]);
        }

        pix1 += FENC_STRIDE;
        pix2 += strideCur;
        pix3 += strideCur;
        pix4 += strideCur;
    }
}

template<int lx, int ly>
void CDECL sad_x4(pixel *pix1, pixel *pix2, pixel *pix3, pixel *pix4, pixel *pix5, intptr_t strideCur, int *res)
{
    res[0] = 0;
    res[1] = 0;
    res[2] = 0;
    res[3] = 0;
    for (int y = 0; y < ly; y++)
    {
        for (int x = 0; x < lx; x++)
        {
            res[0] += abs(pix1[x] - pix2[x]);
            res[1] += abs(pix1[x] - pix3[x]);
            res[2] += abs(pix1[x] - pix4[x]);
            res[3] += abs(pix1[x] - pix5[x]);
        }

        pix1 += FENC_STRIDE;
        pix2 += strideCur;
        pix3 += strideCur;
        pix4 += strideCur;
        pix5 += strideCur;
    }
}

template<int lx, int ly, class T1, class T2>
int CDECL sse(T1 *pix1, intptr_t stride_pix1, T2 *pix2, intptr_t stride_pix2)
{
    int sum = 0;
    int iTemp;

    for (int y = 0; y < ly; y++)
    {
        for (int x = 0; x < lx; x++)
        {
            iTemp = pix1[x] - pix2[x];
            sum += (iTemp * iTemp);
        }

        pix1 += stride_pix1;
        pix2 += stride_pix2;
    }

    return sum;
}

#define BITS_PER_SUM (8 * sizeof(sum_t))

#define HADAMARD4(d0, d1, d2, d3, s0, s1, s2, s3) { \
        sum2_t t0 = s0 + s1; \
        sum2_t t1 = s0 - s1; \
        sum2_t t2 = s2 + s3; \
        sum2_t t3 = s2 - s3; \
        d0 = t0 + t2; \
        d2 = t0 - t2; \
        d1 = t1 + t3; \
        d3 = t1 - t3; \
}

// in: a pseudo-simd number of the form x+(y<<16)
// return: abs(x)+(abs(y)<<16)
inline sum2_t abs2(sum2_t a)
{
    sum2_t s = ((a >> (BITS_PER_SUM - 1)) & (((sum2_t)1 << BITS_PER_SUM) + 1)) * ((sum_t)-1);

    return (a + s) ^ s;
}

int CDECL satd_4x4(pixel *pix1, intptr_t stride_pix1, pixel *pix2, intptr_t stride_pix2)
{
    sum2_t tmp[4][2];
    sum2_t a0, a1, a2, a3, b0, b1;
    sum2_t sum = 0;

    for (int i = 0; i < 4; i++, pix1 += stride_pix1, pix2 += stride_pix2)
    {
        a0 = pix1[0] - pix2[0];
        a1 = pix1[1] - pix2[1];
        b0 = (a0 + a1) + ((a0 - a1) << BITS_PER_SUM);
        a2 = pix1[2] - pix2[2];
        a3 = pix1[3] - pix2[3];
        b1 = (a2 + a3) + ((a2 - a3) << BITS_PER_SUM);
        tmp[i][0] = b0 + b1;
        tmp[i][1] = b0 - b1;
    }

    for (int i = 0; i < 2; i++)
    {
        HADAMARD4(a0, a1, a2, a3, tmp[0][i], tmp[1][i], tmp[2][i], tmp[3][i]);
        a0 = abs2(a0) + abs2(a1) + abs2(a2) + abs2(a3);
        sum += ((sum_t)a0) + (a0 >> BITS_PER_SUM);
    }

    return (int)(sum >> 1);
}

int CDECL satd_8x4(pixel *pix1, intptr_t stride_pix1, pixel *pix2, intptr_t stride_pix2)
{
    sum2_t tmp[4][4];
    sum2_t a0, a1, a2, a3;
    sum2_t sum = 0;

    for (int i = 0; i < 4; i++, pix1 += stride_pix1, pix2 += stride_pix2)
    {
        a0 = (pix1[0] - pix2[0]) + ((sum2_t)(pix1[4] - pix2[4]) << BITS_PER_SUM);
        a1 = (pix1[1] - pix2[1]) + ((sum2_t)(pix1[5] - pix2[5]) << BITS_PER_SUM);
        a2 = (pix1[2] - pix2[2]) + ((sum2_t)(pix1[6] - pix2[6]) << BITS_PER_SUM);
        a3 = (pix1[3] - pix2[3]) + ((sum2_t)(pix1[7] - pix2[7]) << BITS_PER_SUM);
        HADAMARD4(tmp[i][0], tmp[i][1], tmp[i][2], tmp[i][3], a0, a1, a2, a3);
    }

    for (int i = 0; i < 4; i++)
    {
        HADAMARD4(a0, a1, a2, a3, tmp[0][i], tmp[1][i], tmp[2][i], tmp[3][i]);
        sum += abs2(a0) + abs2(a1) + abs2(a2) + abs2(a3);
    }

    return (((sum_t)sum) + (sum >> BITS_PER_SUM)) >> 1;
}

template<int w, int h>
int CDECL satd4(pixel *pix1, intptr_t stride_pix1, pixel *pix2, intptr_t stride_pix2)
{
    int satd = 0;

    for (int row = 0; row < h; row += 4)
    {
        for (int col = 0; col < w; col += 4)
        {
            satd += satd_4x4(pix1 + row * stride_pix1 + col, stride_pix1,
                             pix2 + row * stride_pix2 + col, stride_pix2);
        }
    }

    return satd;
}

template<int w, int h>
int CDECL satd8(pixel *pix1, intptr_t stride_pix1, pixel *pix2, intptr_t stride_pix2)
{
    int satd = 0;

    for (int row = 0; row < h; row += 4)
    {
        for (int col = 0; col < w; col += 8)
        {
            satd += satd_8x4(pix1 + row * stride_pix1 + col, stride_pix1,
                             pix2 + row * stride_pix2 + col, stride_pix2);
        }
    }

    return satd;
}

template<int h>
int CDECL satd12(pixel *pix1, intptr_t stride_pix1, pixel *pix2, intptr_t stride_pix2)
{
    int satd = 0;

    for (int row = 0; row < h; row += 4)
    {
        satd += satd_8x4(pix1 + row * stride_pix1, stride_pix1,
                         pix2 + row * stride_pix2, stride_pix2);
        satd += satd_4x4(pix1 + row * stride_pix1 + 8, stride_pix1,
                         pix2 + row * stride_pix2 + 8, stride_pix2);
    }

    return satd;
}

int CDECL sa8d_8x8(pixel *pix1, intptr_t i_pix1, pixel *pix2, intptr_t i_pix2)
{
    sum2_t tmp[8][4];
    sum2_t a0, a1, a2, a3, a4, a5, a6, a7, b0, b1, b2, b3;
    sum2_t sum = 0;

    for (int i = 0; i < 8; i++, pix1 += i_pix1, pix2 += i_pix2)
    {
        a0 = pix1[0] - pix2[0];
        a1 = pix1[1] - pix2[1];
        b0 = (a0 + a1) + ((a0 - a1) << BITS_PER_SUM);
        a2 = pix1[2] - pix2[2];
        a3 = pix1[3] - pix2[3];
        b1 = (a2 + a3) + ((a2 - a3) << BITS_PER_SUM);
        a4 = pix1[4] - pix2[4];
        a5 = pix1[5] - pix2[5];
        b2 = (a4 + a5) + ((a4 - a5) << BITS_PER_SUM);
        a6 = pix1[6] - pix2[6];
        a7 = pix1[7] - pix2[7];
        b3 = (a6 + a7) + ((a6 - a7) << BITS_PER_SUM);
        HADAMARD4(tmp[i][0], tmp[i][1], tmp[i][2], tmp[i][3], b0, b1, b2, b3);
    }

    for (int i = 0; i < 4; i++)
    {
        HADAMARD4(a0, a1, a2, a3, tmp[0][i], tmp[1][i], tmp[2][i], tmp[3][i]);
        HADAMARD4(a4, a5, a6, a7, tmp[4][i], tmp[5][i], tmp[6][i], tmp[7][i]);
        b0  = abs2(a0 + a4) + abs2(a0 - a4);
        b0 += abs2(a1 + a5) + abs2(a1 - a5);
        b0 += abs2(a2 + a6) + abs2(a2 - a6);
        b0 += abs2(a3 + a7) + abs2(a3 - a7);
        sum += (sum_t)b0 + (b0 >> BITS_PER_SUM);
    }

    return (int)sum;
}

int CDECL pixel_sa8d_8x8(pixel *pix1, intptr_t i_pix1, pixel *pix2, intptr_t i_pix2)
{
    int sum = sa8d_8x8(pix1, i_pix1, pix2, i_pix2);

    return (sum + 2) >> 2;
}

int CDECL pixel_sa8d_16x16(pixel *pix1, intptr_t i_pix1, pixel *pix2, intptr_t i_pix2)
{
    int sum = sa8d_8x8(pix1, i_pix1, pix2, i_pix2)
        + sa8d_8x8(pix1 + 8, i_pix1, pix2 + 8, i_pix2)
        + sa8d_8x8(pix1 + 8 * i_pix1, i_pix1, pix2 + 8 * i_pix2, i_pix2)
        + sa8d_8x8(pix1 + 8 + 8 * i_pix1, i_pix1, pix2 + 8 + 8 * i_pix2, i_pix2);

    return (sum + 2) >> 2;
}

int CDECL pixel_sa8d_32x32(pixel *pix1, intptr_t i_pix1, pixel *pix2, intptr_t i_pix2)
{
    int sum = 0;

    for (int y = 0; y < 32; y += 8)
    {
        for (int x = 0; x < 32; x += 8)
        {
            sum += sa8d_8x8(pix1 + y * i_pix1 + x, i_pix1, pix2 + y * i_pix2 + x, i_pix2);
        }
    }

    return (sum + 2) >> 2;
}

int CDECL pixel_sa8d_64x64(pixel *pix1, intptr_t i_pix1, pixel *pix2, intptr_t i_pix2)
{
    int sum = 0;

    for (int y = 0; y < 64; y += 8)
    {
        for (int x = 0; x < 64; x += 8)
        {
            sum += sa8d_8x8(pix1 + y * i_pix1 + x, i_pix1, pix2 + y * i_pix2 + x, i_pix2);
        }
    }

    return (sum + 2) >> 2;
}

void CDECL blockcopy_p_p(int bx, int by, pixel *a, intptr_t stridea, pixel *b, intptr_t strideb)
{
    for (int y = 0; y < by; y++)
    {
        for (int x = 0; x < bx; x++)
        {
            a[x] = b[x];
        }

        a += stridea;
        b += strideb;
    }
}

void CDECL blockcopy_s_p(int bx, int by, short *a, intptr_t stridea, pixel *b, intptr_t strideb)
{
    for (int y = 0; y < by; y++)
    {
        for (int x = 0; x < bx; x++)
        {
            a[x] = (short)b[x];
        }

        a += stridea;
        b += strideb;
    }
}

void CDECL blockcopy_p_s(int bx, int by, pixel *a, intptr_t stridea, short *b, intptr_t strideb)
{
    for (int y = 0; y < by; y++)
    {
        for (int x = 0; x < bx; x++)
        {
            a[x] = (pixel)b[x];
        }

        a += stridea;
        b += strideb;
    }
}

void CDECL blockcopy_s_c(int bx, int by, short *a, intptr_t stridea, uint8_t *b, intptr_t strideb)
{
    for (int y = 0; y < by; y++)
    {
        for (int x = 0; x < bx; x++)
        {
            a[x] = (short)b[x];
        }

        a += stridea;
        b += strideb;
    }
}

void CDECL convert16to32(short *psOrg, int *piDst, int num)
{
    for (int i = 0; i < num; i++)
    {
        piDst[i] = (int)psOrg[i];
    }
}

void CDECL convert16to32_shl(int *piDst, short *psOrg, int shift, int num)
{
    for (int i = 0; i < num; i++)
    {
        piDst[i] = ((int)psOrg[i]) << shift;
    }
}

void CDECL convert32to16(int *psOrg, short *piDst, int num)
{
    for (int i = 0; i < num; i++)
    {
        piDst[i] = (short)psOrg[i];
    }
}

void CDECL convert32to16_shr(short *piDst, int *psOrg, int shift, int num)
{
    for (int i = 0; i < num; i++)
    {
        piDst[i] = (short)(psOrg[i] >> shift);
    }
}

void CDECL getResidualIntra(pixel *piOrg, pixel *piPred, short *piResi, int height, int width, int stride)
    {
        // get residual
        pixel*  pOrg    = piOrg;
        pixel*  pPred   = piPred;
        short* pResi  = piResi;
        //TODO : performance primitive?
        for (int uiY = 0; uiY < height; uiY++)
        {
            for (int uiX = 0; uiX < width; uiX++)
            {
                pResi[uiX] = static_cast<short>(pOrg[uiX]) - static_cast<short>(pPred[uiX]);
            }

            pOrg  += stride;
            pResi += stride;
            pPred += stride;
        }
    }


void CDECL calcRecons(pixel* piPred, short* piResi,pixel*  piReco, short* piRecQt, pixel* piRecIPred, int uiStride, int uiRecQtStride, int uiRecIPredStride, int uiHeight, int uiWidth)
    {
        pixel*   pPred      = piPred;
        short* pResi      = piResi;
        pixel*   pReco      = piReco;
        short* pRecQt     = piRecQt;
        pixel*   pRecIPred  = piRecIPred;
        //TODO : performance primitive?
        for (int uiY = 0; uiY < uiHeight; uiY++)
        {
            for (int uiX = 0; uiX < uiWidth; uiX++)
            {
                pReco[uiX] = (pixel) ClipY(static_cast<short>(pPred[uiX]) + pResi[uiX]);
                pRecQt[uiX] = (short)pReco[uiX];
                pRecIPred[uiX] = pReco[uiX];
            }

            pPred     += uiStride;
            pResi     += uiStride;
            pReco     += uiStride;
            pRecQt    += uiRecQtStride;
            pRecIPred += uiRecIPredStride;
        }
    }

}  // end anonymous namespace

namespace x265 {
// x265 private namespace

/* It should initialize entries for pixel functions defined in this file. */
void Setup_C_PixelPrimitives(EncoderPrimitives &p)
{
    // sad
    p.sad[PARTITION_4x4]   = sad<4, 4>;
    p.sad[PARTITION_4x8]   = sad<4, 8>;
    p.sad[PARTITION_4x12]  = sad<4, 12>;
    p.sad[PARTITION_4x16]  = sad<4, 16>;
    p.sad[PARTITION_4x24]  = sad<4, 24>;
    p.sad[PARTITION_4x32]  = sad<4, 32>;
    p.sad[PARTITION_4x48]  = sad<4, 48>;
    p.sad[PARTITION_4x64]  = sad<4, 64>;

    p.sad[PARTITION_8x4]   = sad<8, 4>;
    p.sad[PARTITION_8x8]   = sad<8, 8>;
    p.sad[PARTITION_8x12]  = sad<8, 12>;
    p.sad[PARTITION_8x16]  = sad<8, 16>;
    p.sad[PARTITION_8x24]  = sad<8, 24>;
    p.sad[PARTITION_8x32]  = sad<8, 32>;
    p.sad[PARTITION_8x48]  = sad<8, 48>;
    p.sad[PARTITION_8x64]  = sad<8, 64>;

    p.sad[PARTITION_12x4]  = sad<12, 4>;
    p.sad[PARTITION_12x8]  = sad<12, 8>;
    p.sad[PARTITION_12x12] = sad<12, 12>;
    p.sad[PARTITION_12x16] = sad<12, 16>;
    p.sad[PARTITION_12x24] = sad<12, 24>;
    p.sad[PARTITION_12x32] = sad<12, 32>;
    p.sad[PARTITION_12x48] = sad<12, 48>;
    p.sad[PARTITION_12x64] = sad<12, 64>;

    p.sad[PARTITION_16x4]  = sad<16, 4>;
    p.sad[PARTITION_16x8]  = sad<16, 8>;
    p.sad[PARTITION_16x12] = sad<16, 12>;
    p.sad[PARTITION_16x16] = sad<16, 16>;
    p.sad[PARTITION_16x24] = sad<16, 24>;
    p.sad[PARTITION_16x32] = sad<16, 32>;
    p.sad[PARTITION_16x48] = sad<16, 48>;
    p.sad[PARTITION_16x64] = sad<16, 64>;

    p.sad[PARTITION_24x4]  = sad<24, 4>;
    p.sad[PARTITION_24x8]  = sad<24, 8>;
    p.sad[PARTITION_24x12] = sad<24, 12>;
    p.sad[PARTITION_24x16] = sad<24, 16>;
    p.sad[PARTITION_24x24] = sad<24, 24>;
    p.sad[PARTITION_24x32] = sad<24, 32>;
    p.sad[PARTITION_24x48] = sad<24, 48>;
    p.sad[PARTITION_24x64] = sad<24, 64>;

    p.sad[PARTITION_32x4]  = sad<32, 4>;
    p.sad[PARTITION_32x8]  = sad<32, 8>;
    p.sad[PARTITION_32x12] = sad<32, 12>;
    p.sad[PARTITION_32x16] = sad<32, 16>;
    p.sad[PARTITION_32x24] = sad<32, 24>;
    p.sad[PARTITION_32x32] = sad<32, 32>;
    p.sad[PARTITION_32x48] = sad<32, 48>;
    p.sad[PARTITION_32x64] = sad<32, 64>;

    p.sad[PARTITION_48x4]  = sad<48, 4>;
    p.sad[PARTITION_48x8]  = sad<48, 8>;
    p.sad[PARTITION_48x12] = sad<48, 12>;
    p.sad[PARTITION_48x16] = sad<48, 16>;
    p.sad[PARTITION_48x24] = sad<48, 24>;
    p.sad[PARTITION_48x32] = sad<48, 32>;
    p.sad[PARTITION_48x48] = sad<48, 48>;
    p.sad[PARTITION_48x64] = sad<48, 64>;

    p.sad[PARTITION_64x4]  = sad<64, 4>;
    p.sad[PARTITION_64x8]  = sad<64, 8>;
    p.sad[PARTITION_64x12] = sad<64, 12>;
    p.sad[PARTITION_64x16] = sad<64, 16>;
    p.sad[PARTITION_64x24] = sad<64, 24>;
    p.sad[PARTITION_64x32] = sad<64, 32>;
    p.sad[PARTITION_64x48] = sad<64, 48>;
    p.sad[PARTITION_64x64] = sad<64, 64>;

    // satd
    p.satd[PARTITION_4x4]   = satd_4x4;
    p.satd[PARTITION_4x8]   = satd4<4, 8>;
    p.satd[PARTITION_4x12]  = satd4<4, 12>;
    p.satd[PARTITION_4x16]  = satd4<4, 16>;
    p.satd[PARTITION_4x24]  = satd4<4, 24>;
    p.satd[PARTITION_4x32]  = satd4<4, 32>;
    p.satd[PARTITION_4x48]  = satd4<4, 48>;
    p.satd[PARTITION_4x64]  = satd4<4, 64>;

    p.satd[PARTITION_8x4]   = satd_8x4;
    p.satd[PARTITION_8x8]   = satd8<8, 8>;
    p.satd[PARTITION_8x12]  = satd8<8, 12>;
    p.satd[PARTITION_8x16]  = satd8<8, 16>;
    p.satd[PARTITION_8x24]  = satd8<8, 24>;
    p.satd[PARTITION_8x32]  = satd8<8, 32>;
    p.satd[PARTITION_8x48]  = satd8<8, 48>;
    p.satd[PARTITION_8x64]  = satd8<8, 64>;

    p.satd[PARTITION_12x4]  = satd12<4>;
    p.satd[PARTITION_12x8]  = satd12<8>;
    p.satd[PARTITION_12x12] = satd12<12>;
    p.satd[PARTITION_12x16] = satd12<16>;
    p.satd[PARTITION_12x24] = satd12<24>;
    p.satd[PARTITION_12x32] = satd12<32>;
    p.satd[PARTITION_12x48] = satd12<48>;
    p.satd[PARTITION_12x64] = satd12<64>;

    p.satd[PARTITION_16x4]  = satd8<16, 4>;
    p.satd[PARTITION_16x8]  = satd8<16, 8>;
    p.satd[PARTITION_16x12] = satd8<16, 12>;
    p.satd[PARTITION_16x16] = satd8<16, 16>;
    p.satd[PARTITION_16x24] = satd8<16, 24>;
    p.satd[PARTITION_16x32] = satd8<16, 32>;
    p.satd[PARTITION_16x48] = satd8<16, 48>;
    p.satd[PARTITION_16x64] = satd8<16, 64>;

    p.satd[PARTITION_24x4]  = satd8<24, 4>;
    p.satd[PARTITION_24x8]  = satd8<24, 8>;
    p.satd[PARTITION_24x12] = satd8<24, 12>;
    p.satd[PARTITION_24x16] = satd8<24, 16>;
    p.satd[PARTITION_24x24] = satd8<24, 24>;
    p.satd[PARTITION_24x32] = satd8<24, 32>;
    p.satd[PARTITION_24x48] = satd8<24, 48>;
    p.satd[PARTITION_24x64] = satd8<24, 64>;

    p.satd[PARTITION_32x4]  = satd8<32, 4>;
    p.satd[PARTITION_32x8]  = satd8<32, 8>;
    p.satd[PARTITION_32x12] = satd8<32, 12>;
    p.satd[PARTITION_32x16] = satd8<32, 16>;
    p.satd[PARTITION_32x24] = satd8<32, 24>;
    p.satd[PARTITION_32x32] = satd8<32, 32>;
    p.satd[PARTITION_32x48] = satd8<32, 48>;
    p.satd[PARTITION_32x64] = satd8<32, 64>;

    p.satd[PARTITION_48x4]  = satd8<48, 4>;
    p.satd[PARTITION_48x8]  = satd8<48, 8>;
    p.satd[PARTITION_48x12] = satd8<48, 12>;
    p.satd[PARTITION_48x16] = satd8<48, 16>;
    p.satd[PARTITION_48x24] = satd8<48, 24>;
    p.satd[PARTITION_48x32] = satd8<48, 32>;
    p.satd[PARTITION_48x48] = satd8<48, 48>;
    p.satd[PARTITION_48x64] = satd8<48, 64>;

    p.satd[PARTITION_64x4]  = satd8<64, 4>;
    p.satd[PARTITION_64x8]  = satd8<64, 8>;
    p.satd[PARTITION_64x12] = satd8<64, 12>;
    p.satd[PARTITION_64x16] = satd8<64, 16>;
    p.satd[PARTITION_64x24] = satd8<64, 24>;
    p.satd[PARTITION_64x32] = satd8<64, 32>;
    p.satd[PARTITION_64x48] = satd8<64, 48>;
    p.satd[PARTITION_64x64] = satd8<64, 64>;

    // sad_x3
    p.sad_x3[PARTITION_4x4]   = sad_x3<4, 4>;
    p.sad_x3[PARTITION_4x8]   = sad_x3<4, 8>;
    p.sad_x3[PARTITION_4x12]  = sad_x3<4, 12>;
    p.sad_x3[PARTITION_4x16]  = sad_x3<4, 16>;
    p.sad_x3[PARTITION_4x24]  = sad_x3<4, 24>;
    p.sad_x3[PARTITION_4x32]  = sad_x3<4, 32>;
    p.sad_x3[PARTITION_4x48]  = sad_x3<4, 48>;
    p.sad_x3[PARTITION_4x64]  = sad_x3<4, 64>;

    p.sad_x3[PARTITION_8x4]   = sad_x3<8, 4>;
    p.sad_x3[PARTITION_8x8]   = sad_x3<8, 8>;
    p.sad_x3[PARTITION_8x12]  = sad_x3<8, 12>;
    p.sad_x3[PARTITION_8x16]  = sad_x3<8, 16>;
    p.sad_x3[PARTITION_8x24]  = sad_x3<8, 24>;
    p.sad_x3[PARTITION_8x32]  = sad_x3<8, 32>;
    p.sad_x3[PARTITION_8x48]  = sad_x3<8, 48>;
    p.sad_x3[PARTITION_8x64]  = sad_x3<8, 64>;

    p.sad_x3[PARTITION_12x4]  = sad_x3<12, 4>;
    p.sad_x3[PARTITION_12x8]  = sad_x3<12, 8>;
    p.sad_x3[PARTITION_12x12] = sad_x3<12, 12>;
    p.sad_x3[PARTITION_12x16] = sad_x3<12, 16>;
    p.sad_x3[PARTITION_12x24] = sad_x3<12, 24>;
    p.sad_x3[PARTITION_12x32] = sad_x3<12, 32>;
    p.sad_x3[PARTITION_12x48] = sad_x3<12, 48>;
    p.sad_x3[PARTITION_12x64] = sad_x3<12, 64>;

    p.sad_x3[PARTITION_16x4]  = sad_x3<16, 4>;
    p.sad_x3[PARTITION_16x8]  = sad_x3<16, 8>;
    p.sad_x3[PARTITION_16x12] = sad_x3<16, 12>;
    p.sad_x3[PARTITION_16x16] = sad_x3<16, 16>;
    p.sad_x3[PARTITION_16x24] = sad_x3<16, 24>;
    p.sad_x3[PARTITION_16x32] = sad_x3<16, 32>;
    p.sad_x3[PARTITION_16x48] = sad_x3<16, 48>;
    p.sad_x3[PARTITION_16x64] = sad_x3<16, 64>;

    p.sad_x3[PARTITION_24x4]  = sad_x3<24, 4>;
    p.sad_x3[PARTITION_24x8]  = sad_x3<24, 8>;
    p.sad_x3[PARTITION_24x12] = sad_x3<24, 12>;
    p.sad_x3[PARTITION_24x16] = sad_x3<24, 16>;
    p.sad_x3[PARTITION_24x24] = sad_x3<24, 24>;
    p.sad_x3[PARTITION_24x32] = sad_x3<24, 32>;
    p.sad_x3[PARTITION_24x48] = sad_x3<24, 48>;
    p.sad_x3[PARTITION_24x64] = sad_x3<24, 64>;

    p.sad_x3[PARTITION_32x4]  = sad_x3<32, 4>;
    p.sad_x3[PARTITION_32x8]  = sad_x3<32, 8>;
    p.sad_x3[PARTITION_32x12] = sad_x3<32, 12>;
    p.sad_x3[PARTITION_32x16] = sad_x3<32, 16>;
    p.sad_x3[PARTITION_32x24] = sad_x3<32, 24>;
    p.sad_x3[PARTITION_32x32] = sad_x3<32, 32>;
    p.sad_x3[PARTITION_32x48] = sad_x3<32, 48>;
    p.sad_x3[PARTITION_32x64] = sad_x3<32, 64>;

    p.sad_x3[PARTITION_48x4]  = sad_x3<48, 4>;
    p.sad_x3[PARTITION_48x8]  = sad_x3<48, 8>;
    p.sad_x3[PARTITION_48x12] = sad_x3<48, 12>;
    p.sad_x3[PARTITION_48x16] = sad_x3<48, 16>;
    p.sad_x3[PARTITION_48x24] = sad_x3<48, 24>;
    p.sad_x3[PARTITION_48x32] = sad_x3<48, 32>;
    p.sad_x3[PARTITION_48x48] = sad_x3<48, 48>;
    p.sad_x3[PARTITION_48x64] = sad_x3<48, 64>;

    p.sad_x3[PARTITION_64x4]  = sad_x3<64, 4>;
    p.sad_x3[PARTITION_64x8]  = sad_x3<64, 8>;
    p.sad_x3[PARTITION_64x12] = sad_x3<64, 12>;
    p.sad_x3[PARTITION_64x16] = sad_x3<64, 16>;
    p.sad_x3[PARTITION_64x24] = sad_x3<64, 24>;
    p.sad_x3[PARTITION_64x32] = sad_x3<64, 32>;
    p.sad_x3[PARTITION_64x48] = sad_x3<64, 48>;
    p.sad_x3[PARTITION_64x64] = sad_x3<64, 64>;

    // sad_x4
    p.sad_x4[PARTITION_4x4]   = sad_x4<4, 4>;
    p.sad_x4[PARTITION_4x8]   = sad_x4<4, 8>;
    p.sad_x4[PARTITION_4x12]  = sad_x4<4, 12>;
    p.sad_x4[PARTITION_4x16]  = sad_x4<4, 16>;
    p.sad_x4[PARTITION_4x24]  = sad_x4<4, 24>;
    p.sad_x4[PARTITION_4x32]  = sad_x4<4, 32>;
    p.sad_x4[PARTITION_4x48]  = sad_x4<4, 48>;
    p.sad_x4[PARTITION_4x64]  = sad_x4<4, 64>;

    p.sad_x4[PARTITION_8x4]   = sad_x4<8, 4>;
    p.sad_x4[PARTITION_8x8]   = sad_x4<8, 8>;
    p.sad_x4[PARTITION_8x12]  = sad_x4<8, 12>;
    p.sad_x4[PARTITION_8x16]  = sad_x4<8, 16>;
    p.sad_x4[PARTITION_8x24]  = sad_x4<8, 24>;
    p.sad_x4[PARTITION_8x32]  = sad_x4<8, 32>;
    p.sad_x4[PARTITION_8x48]  = sad_x4<8, 48>;
    p.sad_x4[PARTITION_8x64]  = sad_x4<8, 64>;

    p.sad_x4[PARTITION_12x4]  = sad_x4<12, 4>;
    p.sad_x4[PARTITION_12x8]  = sad_x4<12, 8>;
    p.sad_x4[PARTITION_12x12] = sad_x4<12, 12>;
    p.sad_x4[PARTITION_12x16] = sad_x4<12, 16>;
    p.sad_x4[PARTITION_12x24] = sad_x4<12, 24>;
    p.sad_x4[PARTITION_12x32] = sad_x4<12, 32>;
    p.sad_x4[PARTITION_12x48] = sad_x4<12, 48>;
    p.sad_x4[PARTITION_12x64] = sad_x4<12, 64>;

    p.sad_x4[PARTITION_16x4]  = sad_x4<16, 4>;
    p.sad_x4[PARTITION_16x8]  = sad_x4<16, 8>;
    p.sad_x4[PARTITION_16x12] = sad_x4<16, 12>;
    p.sad_x4[PARTITION_16x16] = sad_x4<16, 16>;
    p.sad_x4[PARTITION_16x24] = sad_x4<16, 24>;
    p.sad_x4[PARTITION_16x32] = sad_x4<16, 32>;
    p.sad_x4[PARTITION_16x48] = sad_x4<16, 48>;
    p.sad_x4[PARTITION_16x64] = sad_x4<16, 64>;

    p.sad_x4[PARTITION_24x4]  = sad_x4<24, 4>;
    p.sad_x4[PARTITION_24x8]  = sad_x4<24, 8>;
    p.sad_x4[PARTITION_24x12] = sad_x4<24, 12>;
    p.sad_x4[PARTITION_24x16] = sad_x4<24, 16>;
    p.sad_x4[PARTITION_24x24] = sad_x4<24, 24>;
    p.sad_x4[PARTITION_24x32] = sad_x4<24, 32>;
    p.sad_x4[PARTITION_24x48] = sad_x4<24, 48>;
    p.sad_x4[PARTITION_24x64] = sad_x4<24, 64>;

    p.sad_x4[PARTITION_32x4]  = sad_x4<32, 4>;
    p.sad_x4[PARTITION_32x8]  = sad_x4<32, 8>;
    p.sad_x4[PARTITION_32x12] = sad_x4<32, 12>;
    p.sad_x4[PARTITION_32x16] = sad_x4<32, 16>;
    p.sad_x4[PARTITION_32x24] = sad_x4<32, 24>;
    p.sad_x4[PARTITION_32x32] = sad_x4<32, 32>;
    p.sad_x4[PARTITION_32x48] = sad_x4<32, 48>;
    p.sad_x4[PARTITION_32x64] = sad_x4<32, 64>;

    p.sad_x4[PARTITION_48x4]  = sad_x4<48, 4>;
    p.sad_x4[PARTITION_48x8]  = sad_x4<48, 8>;
    p.sad_x4[PARTITION_48x12] = sad_x4<48, 12>;
    p.sad_x4[PARTITION_48x16] = sad_x4<48, 16>;
    p.sad_x4[PARTITION_48x24] = sad_x4<48, 24>;
    p.sad_x4[PARTITION_48x32] = sad_x4<48, 32>;
    p.sad_x4[PARTITION_48x48] = sad_x4<48, 48>;
    p.sad_x4[PARTITION_48x64] = sad_x4<48, 64>;

    p.sad_x4[PARTITION_64x4]  = sad_x4<64, 4>;
    p.sad_x4[PARTITION_64x8]  = sad_x4<64, 8>;
    p.sad_x4[PARTITION_64x12] = sad_x4<64, 12>;
    p.sad_x4[PARTITION_64x16] = sad_x4<64, 16>;
    p.sad_x4[PARTITION_64x24] = sad_x4<64, 24>;
    p.sad_x4[PARTITION_64x32] = sad_x4<64, 32>;
    p.sad_x4[PARTITION_64x48] = sad_x4<64, 48>;
    p.sad_x4[PARTITION_64x64] = sad_x4<64, 64>;

    //sse
#if HIGH_BIT_DEPTH
    SET_FUNC_PRIMITIVE_TABLE_C(sse_pp, sse, pixelcmp, short, short)
    SET_FUNC_PRIMITIVE_TABLE_C(sse_sp, sse, pixelcmp_sp, short, short)
    SET_FUNC_PRIMITIVE_TABLE_C(sse_ss, sse, pixelcmp_ss, short, short)
#else
    SET_FUNC_PRIMITIVE_TABLE_C(sse_pp, sse, pixelcmp, pixel, pixel)
    SET_FUNC_PRIMITIVE_TABLE_C(sse_sp, sse, pixelcmp_sp, short, pixel)
    SET_FUNC_PRIMITIVE_TABLE_C(sse_ss, sse, pixelcmp_ss, short, short)
#endif
    p.cpyblock     = blockcopy_p_p;
    p.cpyblock_s_p = blockcopy_s_p;
    p.cpyblock_p_s = blockcopy_p_s;
    p.cpyblock_s_c = blockcopy_s_c;

    p.cvt16to32     = convert16to32;
    p.cvt16to32_shl = convert16to32_shl;
    p.cvt32to16     = convert32to16;
    p.cvt32to16_shr = convert32to16_shr;

    // sa8d
    p.sa8d_8x8   = pixel_sa8d_8x8;
    p.sa8d_16x16 = pixel_sa8d_16x16;
    p.sa8d_32x32 = pixel_sa8d_32x32;
    p.sa8d_64x64 = pixel_sa8d_64x64;

    p.getResidue = getResidualIntra;
    p.calcRecons = calcRecons;
}
}
