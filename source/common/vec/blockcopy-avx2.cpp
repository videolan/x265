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
 * For more information, contact us at licensing@multicorewareinc.com
 *****************************************************************************/

#include "TLibCommon/TComRom.h"

#define INSTRSET 8
#include "vectorclass.h"

#include "primitives.h"
#include <string.h>

namespace {
#if !HIGH_BIT_DEPTH
void blockcopy_p_p(int bx, int by, pixel *dst, intptr_t dstride, pixel *src, intptr_t sstride)
{
    size_t aligncheck = (size_t)dst | (size_t)src | bx | sstride | dstride;

    if (!(aligncheck & 31))
    {
        // fast path, multiples of 32 pixel wide blocks
        for (int y = 0; y < by; y++)
        {
            for (int x = 0; x < bx; x += 32)
            {
                Vec32c word;
                word.load_a(src + x);
                word.store_a(dst + x);
            }

            src += sstride;
            dst += dstride;
        }
    }
    else if (!(aligncheck & 15))
    {
        // fast path, multiples of 16 pixel wide blocks
        for (int y = 0; y < by; y++)
        {
            for (int x = 0; x < bx; x += 16)
            {
                Vec16c word;
                word.load_a(src + x);
                word.store_a(dst + x);
            }

            src += sstride;
            dst += dstride;
        }
    }
    else
    {
        // slow path, irregular memory alignments or sizes
        for (int y = 0; y < by; y++)
        {
            memcpy(dst, src, bx * sizeof(pixel));
            src += sstride;
            dst += dstride;
        }
    }
}

#endif // if !HIGH_BIT_DEPTH
}

namespace x265 {
void Setup_Vec_BlockCopyPrimitives_avx2(EncoderPrimitives &p)
{
#if HIGH_BIT_DEPTH
    p.blockcpy_pp = p.blockcpy_pp;
#else
    p.blockcpy_pp = blockcopy_p_p;
#endif
}
}
