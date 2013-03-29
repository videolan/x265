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

#include "primitives.h"
#include <stdlib.h> // abs()

namespace // place functions in anonymous namespace (file static)
{

template<int lx, int ly>
int CDECL sad( pixel *pix1, intptr_t stride_pix1, pixel *pix2, intptr_t stride_pix2 )
{
    // TODO: we could use SWAR here fairly easily.  Would it help?
    int sum = 0;
    for( int y = 0; y < ly; y++ )
    {
        for( int x = 0; x < lx; x++ )
        {
            sum += abs( pix1[x] - pix2[x] );
        }
        pix1 += stride_pix1;
        pix2 += stride_pix2;
    }
    return sum;
}

#define BITS_PER_SUM (8 * sizeof(sum_t))

#define HADAMARD4(d0, d1, d2, d3, s0, s1, s2, s3) {\
    sum2_t t0 = s0 + s1;\
    sum2_t t1 = s0 - s1;\
    sum2_t t2 = s2 + s3;\
    sum2_t t3 = s2 - s3;\
    d0 = t0 + t2;\
    d2 = t0 - t2;\
    d1 = t1 + t3;\
    d3 = t1 - t3;\
}

// in: a pseudo-simd number of the form x+(y<<16)
// return: abs(x)+(abs(y)<<16)
inline sum2_t abs2( sum2_t a )
{
    sum2_t s = ((a>>(BITS_PER_SUM-1))&(((sum2_t)1<<BITS_PER_SUM)+1))*((sum_t)-1);
    return (a+s)^s;
}

int CDECL satd_4x4( pixel *pix1, intptr_t stride_pix1, pixel *pix2, intptr_t stride_pix2 )
{
    sum2_t tmp[4][2];
    sum2_t a0, a1, a2, a3, b0, b1;
    sum2_t sum = 0;
    for( int i = 0; i < 4; i++, pix1 += stride_pix1, pix2 += stride_pix2 )
    {
        a0 = pix1[0] - pix2[0];
        a1 = pix1[1] - pix2[1];
        b0 = (a0+a1) + ((a0-a1)<<BITS_PER_SUM);
        a2 = pix1[2] - pix2[2];
        a3 = pix1[3] - pix2[3];
        b1 = (a2+a3) + ((a2-a3)<<BITS_PER_SUM);
        tmp[i][0] = b0 + b1;
        tmp[i][1] = b0 - b1;
    }
    for( int i = 0; i < 2; i++ )
    {
        HADAMARD4( a0, a1, a2, a3, tmp[0][i], tmp[1][i], tmp[2][i], tmp[3][i] );
        a0 = abs2(a0) + abs2(a1) + abs2(a2) + abs2(a3);
        sum += ((sum_t)a0) + (a0>>BITS_PER_SUM);
    }
    return (int)(sum >> 1);
}

int CDECL satd_8x4( pixel *pix1, intptr_t stride_pix1, pixel *pix2, intptr_t stride_pix2 )
{
    sum2_t tmp[4][4];
    sum2_t a0, a1, a2, a3;
    sum2_t sum = 0;
    for( int i = 0; i < 4; i++, pix1 += stride_pix1, pix2 += stride_pix2 )
    {
        a0 = (pix1[0] - pix2[0]) + ((sum2_t)(pix1[4] - pix2[4]) << BITS_PER_SUM);
        a1 = (pix1[1] - pix2[1]) + ((sum2_t)(pix1[5] - pix2[5]) << BITS_PER_SUM);
        a2 = (pix1[2] - pix2[2]) + ((sum2_t)(pix1[6] - pix2[6]) << BITS_PER_SUM);
        a3 = (pix1[3] - pix2[3]) + ((sum2_t)(pix1[7] - pix2[7]) << BITS_PER_SUM);
        HADAMARD4( tmp[i][0], tmp[i][1], tmp[i][2], tmp[i][3], a0,a1,a2,a3 );
    }
    for( int i = 0; i < 4; i++ )
    {
        HADAMARD4( a0, a1, a2, a3, tmp[0][i], tmp[1][i], tmp[2][i], tmp[3][i] );
        sum += abs2(a0) + abs2(a1) + abs2(a2) + abs2(a3);
    }
    return (((sum_t)sum) + (sum>>BITS_PER_SUM)) >> 1;
}

#if _MSC_VER
#pragma warning(disable: 4127) // conditional expression is constant (yes, deliberately)
#endif
// handles all partitions up to 16x16
template<int w, int h, x265::pixelcmp sub>
int CDECL satd( pixel *pix1, intptr_t stride_pix1, pixel *pix2, intptr_t stride_pix2 )
{
    int sum = sub( pix1, stride_pix1, pix2, stride_pix2 )
            + sub( pix1+4*stride_pix1, stride_pix1, pix2+4*stride_pix2, stride_pix2 );
    if( w==16 )
        sum+= sub( pix1+8, stride_pix1, pix2+8, stride_pix2 )
            + sub( pix1+8+4*stride_pix1, stride_pix1, pix2+8+4*stride_pix2, stride_pix2 );
    if( h==16 )
        sum+= sub( pix1+8*stride_pix1, stride_pix1, pix2+8*stride_pix2, stride_pix2 )
            + sub( pix1+12*stride_pix1, stride_pix1, pix2+12*stride_pix2, stride_pix2 );
    if( w==16 && h==16 )
        sum+= sub( pix1+8+8*stride_pix1, stride_pix1, pix2+8+8*stride_pix2, stride_pix2 )
            + sub( pix1+8+12*stride_pix1, stride_pix1, pix2+8+12*stride_pix2, stride_pix2 );
    return sum;
}

int CDECL satd_4x32( pixel *pix1, intptr_t stride_pix1, pixel *pix2, intptr_t stride_pix2 )
{
    return satd_4x4( pix1, stride_pix1, pix2, stride_pix2) +
        satd_4x4( pix1+4*stride_pix1, stride_pix1, pix2+4*stride_pix2, stride_pix2) +
        satd_4x4( pix1+8*stride_pix1, stride_pix1, pix2+8*stride_pix2, stride_pix2) +
        satd_4x4( pix1+12*stride_pix1, stride_pix1, pix2+12*stride_pix2, stride_pix2) +
        satd_4x4( pix1+16*stride_pix1, stride_pix1, pix2+16*stride_pix2, stride_pix2) +
        satd_4x4( pix1+24*stride_pix1, stride_pix1, pix2+24*stride_pix2, stride_pix2) +
        satd_4x4( pix1+28*stride_pix1, stride_pix1, pix2+28*stride_pix2, stride_pix2);
}

int CDECL satd_32x4( pixel *pix1, intptr_t stride_pix1, pixel *pix2, intptr_t stride_pix2 )
{
    return satd_8x4( pix1, stride_pix1, pix2, stride_pix2) +
        satd_8x4( pix1+8, stride_pix1, pix2+8, stride_pix2) +
        satd_8x4( pix1+16, stride_pix1, pix2+16, stride_pix2) +
        satd_8x4( pix1+24, stride_pix1, pix2+24, stride_pix2);
}

int CDECL satd_32x8( pixel *pix1, intptr_t stride_pix1, pixel *pix2, intptr_t stride_pix2 )
{
    return satd_32x4( pix1, stride_pix1, pix2, stride_pix2) +
        satd_32x4( pix1+4*stride_pix1, stride_pix1, pix2+4*stride_pix2, stride_pix2);
}

int CDECL satd_8x32( pixel *pix1, intptr_t stride_pix1, pixel *pix2, intptr_t stride_pix2 )
{
    return satd_8x4( pix1, stride_pix1, pix2, stride_pix2) +
        satd_8x4( pix1+4*stride_pix1, stride_pix1, pix2+4*stride_pix2, stride_pix2) +
        satd_8x4( pix1+8*stride_pix1, stride_pix1, pix2+8*stride_pix2, stride_pix2) +
        satd_8x4( pix1+12*stride_pix1, stride_pix1, pix2+12*stride_pix2, stride_pix2) +
        satd_8x4( pix1+16*stride_pix1, stride_pix1, pix2+16*stride_pix2, stride_pix2) +
        satd_8x4( pix1+24*stride_pix1, stride_pix1, pix2+24*stride_pix2, stride_pix2) +
        satd_8x4( pix1+28*stride_pix1, stride_pix1, pix2+28*stride_pix2, stride_pix2);
}

// Handles partitions that are multiples of 16x16
template<int w, int h>
int CDECL satd32( pixel *pix1, intptr_t stride_pix1, pixel *pix2, intptr_t stride_pix2 )
{
    int sum = satd<16,16,satd_8x4>( pix1, stride_pix1, pix2, stride_pix2 );
    if (w == 32)
        sum += satd<16,16,satd_8x4>( pix1+16, stride_pix1, pix2+16, stride_pix2 );
    if (h == 32)
        sum += satd<16,16,satd_8x4>( pix1+16*stride_pix1, stride_pix1, pix2+16*stride_pix2, stride_pix2 );
    if (w == 32 && h == 32)
        sum += satd<16,16,satd_8x4>( pix1+16+16*stride_pix1, stride_pix1, pix2+16+16*stride_pix2, stride_pix2 );
    return sum;
}

}  // end anonymous namespace


namespace x265
{

extern EncoderPrimitives primitives_c;


/* Setup() will be called before main().  It should initialize 
 * primitive_c entries for pixel functions defined in this file.
 */
static int Setup()
{
    EncoderPrimitives &p = primitives_c;

    p.sad[PARTITION_4x4]   = sad<4,4>;
    p.sad[PARTITION_8x4]   = sad<8,4>;
    p.sad[PARTITION_4x8]   = sad<4,8>;
    p.sad[PARTITION_8x8]   = sad<8,8>;
    p.sad[PARTITION_16x4]  = sad<4,16>;
    p.sad[PARTITION_4x16]  = sad<16,4>;
    p.sad[PARTITION_8x16]  = sad<8,16>;
    p.sad[PARTITION_16x8]  = sad<16,8>;
    p.sad[PARTITION_16x16] = sad<16,16>;
    p.sad[PARTITION_8x32]  = sad<8,32>;
    p.sad[PARTITION_32x8]  = sad<32,8>;
    p.sad[PARTITION_4x32]  = sad<4,32>;
    p.sad[PARTITION_32x4]  = sad<32,4>;
    p.sad[PARTITION_16x32] = sad<16,32>;
    p.sad[PARTITION_32x16] = sad<32,16>;
    p.sad[PARTITION_32x32] = sad<32,32>;

    p.satd[PARTITION_4x4]   = satd_4x4;
    p.satd[PARTITION_8x4]   = satd_8x4;
    p.satd[PARTITION_4x8]   = satd<4,8, satd_4x4>;
    p.satd[PARTITION_8x8]   = satd<8,8, satd_8x4>;
    p.satd[PARTITION_16x4]  = satd<4,16, satd_8x4>;
    p.satd[PARTITION_4x16]  = satd<16,4, satd_4x4>;
    p.satd[PARTITION_8x16]  = satd<8,16, satd_8x4>;
    p.satd[PARTITION_16x8]  = satd<16,8, satd_8x4>;
    p.satd[PARTITION_16x16] = satd<16,16, satd_8x4>;
    p.satd[PARTITION_4x32]  = satd_4x32;
    p.satd[PARTITION_32x4]  = satd_32x4;
    p.satd[PARTITION_8x32]  = satd_8x32;
    p.satd[PARTITION_32x8]  = satd_32x8;
    p.satd[PARTITION_16x32] = satd32<16,32>;
    p.satd[PARTITION_32x16] = satd32<32,16>;
    p.satd[PARTITION_32x32] = satd32<32,32>;

    return 1;
}

static int forceinit = Setup();

};
