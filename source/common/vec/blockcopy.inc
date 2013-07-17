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

void blockcopy_p_p(int bx, int by, pixel *dst, intptr_t dstride, pixel *src, intptr_t sstride)
{
    if ((bx & 7) || (((size_t)dst | (size_t)src | sstride | dstride) & 15))
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
            for (int x = 0; x < bx; x += 8)
            {
                Vec8s word;
                word.load_a(src + x);
                word.store_a(dst + x);
            }

            src += sstride;
            dst += dstride;
        }
    }
}

#else /* if HIGH_BIT_DEPTH */

void blockcopy_p_p(int bx, int by, pixel *dst, intptr_t dstride, pixel *src, intptr_t sstride)
{
    size_t aligncheck = (size_t)dst | (size_t)src | bx | sstride | dstride;

#if INSTRSET >= 8
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
    else
#endif
    if (!(aligncheck & 15))
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

void blockcopy_p_s(int bx, int by, pixel *dst, intptr_t dstride, short *src, intptr_t sstride)
{
    size_t aligncheck = (size_t)dst | (size_t)src | bx | sstride | dstride;

#if INSTRSET >= 8 && 0
    if (!(aligncheck & 31))
    {
        // fast path, multiples of 32 pixel wide blocks
        for (int y = 0; y < by; y++)
        {
            for (int x = 0; x < bx; x += 32)
            {
                Vec16us word0, word1;
                word0.load_a(src + x);
                word1.load_a(src + x + 16);
                compress(word0, word1).store_a(dst + x);
            }

            src += sstride;
            dst += dstride;
        }
    }
    else
#endif
    if (!(aligncheck & 15))
    {
        // fast path, multiples of 16 pixel wide blocks
        for (int y = 0; y < by; y++)
        {
            for (int x = 0; x < bx; x += 16)
            {
                Vec8us word0, word1;
                word0.load_a(src + x);
                word1.load_a(src + x + 8);
                compress(word0, word1).store_a(dst + x);
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
            for (int x = 0; x < bx; x++)
            {
                dst[x] = (pixel)src[x];
            }

            src += sstride;
            dst += dstride;
        }
    }
}

#endif /* if HIGH_BIT_DEPTH */

void blockcopy_s_p(int bx, int by, short *dst, intptr_t dstride, uint8_t *src, intptr_t sstride)
{
    size_t aligncheck = (size_t)dst | (size_t)src | bx | sstride | dstride;
#if INSTRSET >= 8 && 0
    if (!(aligncheck & 31))
    {
        // fast path, multiples of 32 pixel wide blocks
        for (int y = 0; y < by; y++)
        {
            for (int x = 0; x < bx; x += 32)
            {
                Vec32uc word;
                word.load_a(src + x);
                extend_low(word).store_a(dst + x);
                extend_high(word).store_a(dst + x + 16);
            }

            src += sstride;
            dst += dstride;
        }
    }
    else
#endif
    if (!(aligncheck & 15))
    {
        // fast path, multiples of 16 pixel wide blocks
        for (int y = 0; y < by; y++)
        {
            for (int x = 0; x < bx; x += 16)
            {
                Vec16uc word;
                word.load_a(src + x);
                extend_low(word).store_a(dst + x);
                extend_high(word).store_a(dst + x + 8);
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
            for (int x = 0; x < bx; x++)
            {
                dst[x] = (short)src[x];
            }

            src += sstride;
            dst += dstride;
        }
    }
}

void pixelsub_sp(int bx, int by, short *dst, intptr_t dstride, uint8_t *src0, uint8_t *src1, intptr_t sstride0, intptr_t sstride1)
{
    size_t aligncheck = (size_t)dst | (size_t)src0 | bx | sstride0 | dstride;

#if INSTRSET >= 8 && 0
    if (!(aligncheck & 31))
    {
        // fast path, multiples of 32 pixel wide blocks
        // fast path, multiples of 16 pixel wide blocks
        for (int y = 0; y < by; y++)
        {
            for (int x = 0; x < bx; x += 32)
            {
                Vec32uc word0, word1;
                Vec16s  word3, word4;
                word0.load_a(src0 + x);
                word1.load_a(src1 + x);
                word3 = extend_low(word0) - extend_low(word1);
                word4 = extend_high(word0) - extend_high(word1);
                word3.store_a(dst + x);
                word4.store_a(dst + x + 16);
            }

            src0 += sstride0;
            src1 += sstride1;
            dst += dstride;
        }
    }
    else
#endif /* if INSTRSET >= 8 && 0 */
    if (!(aligncheck & 15))
    {
        // fast path, multiples of 16 pixel wide blocks
        for (int y = 0; y < by; y++)
        {
            for (int x = 0; x < bx; x += 16)
            {
                Vec16uc word0, word1;
                Vec8s word3, word4;
                word0.load_a(src0 + x);
                word1.load_a(src1 + x);
                word3 = extend_low(word0) - extend_low(word1);
                word4 = extend_high(word0) - extend_high(word1);
                word3.store_a(dst + x);
                word4.store_a(dst + x + 8);
            }

            src0 += sstride0;
            src1 += sstride1;
            dst += dstride;
        }
    }
    else
    {
        // slow path, irregular memory alignments or sizes
        for (int y = 0; y < by; y++)
        {
            for (int x = 0; x < bx; x++)
            {
                dst[x] = (short)(src0[x] - src1[x]);
            }

            src0 += sstride0;
            src1 += sstride1;
            dst += dstride;
        }
    }
}

void Setup_Vec_BlockCopyPrimitives(EncoderPrimitives &p)
{
#if HIGH_BIT_DEPTH
    // At high bit depth, a pixel is a short
    p.blockcpy_pp = blockcopy_p_p;
    p.blockcpy_ps = (x265::blockcpy_ps_t)blockcopy_p_p;
    p.blockcpy_sp = (x265::blockcpy_sp_t)blockcopy_p_p;
    p.blockcpy_sc = (x265::blockcpy_sc_t)blockcopy_s_p;
    p.pixelsubsp = pixelsub_sp;
#else
    p.blockcpy_pp = blockcopy_p_p;
    p.blockcpy_ps = blockcopy_p_s;
    p.blockcpy_sp = blockcopy_s_p;
    p.blockcpy_sc = blockcopy_s_p;
    p.pixelsubsp = pixelsub_sp;
#endif
}
