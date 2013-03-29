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
#include <math.h>

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

}  // end anonymous namespace


namespace x265
{

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

    return 1;
}

static int forceinit = Setup();

};
