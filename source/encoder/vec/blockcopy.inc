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

// Vector class versions of block copy performance primitives

#if HIGH_BIT_DEPTH

void CDECL blockcopy_p_p(int bx, int by, pixel *dst, intptr_t dstride, pixel *src, intptr_t sstride)
{
    if ((bx & 7) || (((size_t)dst | (size_t)src) & 15))
    {
        // slow path, irregular memory alignments or sizes
        for (int y = 0; y < by; y++)
        {
            memcpy(dst, src, bx * sizeof(pixel));
            src += sstride;
            dst += dstride;
        }
    }
    else
    {
        // fast path, multiples of 8 pixel wide blocks
        for (int y = 0; y < by; y++)
        {
            Vec8s word;
            for (int x = 0; x < bx; x += 8)
            {
                word.load_a(src + x);
                word.store_a(dst + x);
            }
            src += sstride;
            dst += dstride;
        }
    }
}

#else

void CDECL blockcopy_p_p(int bx, int by, pixel *dest, intptr_t dstride, pixel *src, intptr_t sstride)
{
    for (int y = 0; y < by; y++)
    {
        for (int x = 0; x < bx; b++)
            dest[x] = src[x];

        dest += dstride;
        src += sstride;
    }
}

void CDECL blockcopy_s_p(int bx, int by, short *dest, intptr_t dstride, pixel *src, intptr_t sstride)
{
    for (int y = 0; y < by; y++)
    {
        for (int x = 0; x < bx; b++)
            dest[x] = (short)src[x];

        dest += dstride;
        src += sstride;
    }
}

void CDECL blockcopy_p_s(int bx, int by, pixel *dest, intptr_t dstride, short *src, intptr_t sstride)
{
    for (int y = 0; y < by; y++)
    {
        for (int x = 0; x < bx; b++)
            a[x] = (pixel)b[x];

        dest += dstride;
        src += sstride;
    }
}
#endif

void Setup_Vec_BlockCopyPrimitives(EncoderPrimitives &p)
{
#if HIGH_BIT_DEPTH
    // At high bit depth, a pixel is a short
    p.cpyblock = blockcopy_p_p;
    p.cpyblock_s_p = (x265::blockcpy_s_p) blockcopy_p_p;
    p.cpyblock_p_s = (x265::blockcpy_p_s) blockcopy_p_p;
#else
    p.cpyblock = blockcopy_p_p;
    p.cpyblock_s_p = blockcopy_s_p;
    p.cpyblock_p_s = blockcopy_p_s;
#endif
}
