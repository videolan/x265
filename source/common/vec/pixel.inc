/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Mandar Gurav <mandar@multicorewareinc.com>
 *          Mahesh Pittala <mahesh@multicorewareinc.com>
 *          Min Chen <min.chen@multicorewareinc.com>
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

#include "TLibCommon/TComRom.h"
#include <assert.h>

namespace {
#if HIGH_BIT_DEPTH
#include "pixel16.inc"
#else
#include "pixel8.inc"
#endif
#include "sse.inc"

void convert16to32(short *org, int *dst, int num)
{
    int i;

    for (i = 0; i < num; i += 8)
    {
        Vec8s im16;
        Vec4i im32L, im32H;

        im16.load(org);
        im32L = extend_low(im16);
        im32H = extend_high(im16);
        im32L.store(dst);
        im32H.store(dst + 4);

        org += 8;
        dst += 8;
    }
}

void convert32to16(int *org, short *dst, int num)
{
    int i;

    for (i = 0; i < num; i += 8)
    {
        Vec4i im32L, im32H;
        Vec8s im16;

        im32L.load(org);
        im32H.load(org + 4);
        im16 = compress_saturated(im32L, im32H);
        im16.store(dst);

        org += 8;
        dst += 8;
    }
}

void convert32to16_shr(short *dst, int *org, int shift, int num)
{
    int i;
    Vec4i round = _mm_set1_epi32(1 << (shift - 1));

    for (i = 0; i < num; i += 4)
    {
        Vec4i im32;
        Vec8s im16;

        im32.load(org);
        im32 = (im32 + round) >> shift;
        im16 = compress_saturated(im32, im32);
        store_partial(const_int(8), dst, im16);

        org += 4;
        dst += 4;
    }
}
}  // end anonymous namespace
