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
 * For more information, contact us at license @ x265.com
 *****************************************************************************/

#include "TLibCommon/TComRom.h"
#include "primitives.h"
#include <xmmintrin.h> // SSE
#include <pmmintrin.h> // SSE3

namespace {
#if HIGH_BIT_DEPTH
void blockcopy_pp(int bx, int by, pixel *dst, intptr_t dstride, pixel *src, intptr_t sstride)
{
    if ((bx & 7) || (((intptr_t)dst | (intptr_t)src | sstride | dstride) & 15))
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
                __m128i word = _mm_load_si128((__m128i const*)(src + x));
                _mm_store_si128((__m128i*)&dst[x], word);
            }

            src += sstride;
            dst += dstride;
        }
    }
}

#else // if HIGH_BIT_DEPTH
void blockcopy_pp(int bx, int by, pixel *dst, intptr_t dstride, pixel *src, intptr_t sstride)
{
    intptr_t aligncheck = (intptr_t)dst | (intptr_t)src | bx | sstride | dstride;

    if (!(aligncheck & 15))
    {
        // fast path, multiples of 16 pixel wide blocks
        for (int y = 0; y < by; y++)
        {
            for (int x = 0; x < bx; x += 16)
            {
                __m128i word0 = _mm_load_si128((__m128i const*)(src + x)); // load block of 16 byte from src
                _mm_store_si128((__m128i*)&dst[x], word0); // store block into dst
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

void blockcopy_ps(int bx, int by, pixel *dst, intptr_t dstride, int16_t *src, intptr_t sstride)
{
    intptr_t aligncheck = (intptr_t)dst | (intptr_t)src | bx | sstride | dstride;

    if (!(aligncheck & 15))
    {
        // fast path, multiples of 16 pixel wide blocks
        for (int y = 0; y < by; y++)
        {
            for (int x = 0; x < bx; x += 16)
            {
                __m128i word0 = _mm_load_si128((__m128i const*)(src + x));       // load block of 16 byte from src
                __m128i word1 = _mm_load_si128((__m128i const*)(src + x + 8));

                __m128i mask = _mm_set1_epi32(0x00FF00FF);                  // mask for low bytes
                __m128i low_mask = _mm_and_si128(word0, mask);              // bytes of low
                __m128i high_mask = _mm_and_si128(word1, mask);             // bytes of high
                __m128i word01 = _mm_packus_epi16(low_mask, high_mask);     // unsigned pack
                _mm_store_si128((__m128i*)&dst[x], word01);                 // store block into dst
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

void pixeladd_ss(int bx, int by, int16_t *dst, intptr_t dstride, int16_t *src0, int16_t *src1, intptr_t sstride0, intptr_t sstride1)
{
    intptr_t aligncheck = (intptr_t)dst | (intptr_t)src0 | sstride0 | sstride1 | dstride;

    if (!(aligncheck & 15) && !(bx & 7))
    {
        __m128i maxval = _mm_set1_epi16((1 << X265_DEPTH) - 1);
        __m128i zero = _mm_setzero_si128();

        // fast path, multiples of 8 pixel wide blocks
        for (int y = 0; y < by; y++)
        {
            for (int x = 0; x < bx; x += 8)
            {
                __m128i word0, word1, sum;

                word0 = _mm_load_si128((__m128i*)(src0 + x));    // load 16 bytes from src1
                word1 = _mm_load_si128((__m128i*)(src1 + x));    // load 16 bytes from src2

                sum = _mm_adds_epi16(word0, word1);
                sum = _mm_max_epi16(sum, zero);
                sum = _mm_min_epi16(sum, maxval);

                _mm_store_si128((__m128i*)&dst[x], sum);    // store block into dst
            }

            src0 += sstride0;
            src1 += sstride1;
            dst += dstride;
        }
    }
    else if (!(bx & 7))
    {
        __m128i maxval = _mm_set1_epi16((1 << X265_DEPTH) - 1);
        __m128i zero = _mm_setzero_si128();

        for (int y = 0; y < by; y++)
        {
            for (int x = 0; x < bx; x += 8)
            {
                __m128i word0, word1, sum;

                word0 = _mm_load_si128((__m128i*)(src0 + x));    // load 16 bytes from src1
                word1 = _mm_load_si128((__m128i*)(src1 + x));    // load 16 bytes from src2

                sum = _mm_adds_epi16(word0, word1);
                sum = _mm_max_epi16(sum, zero);
                sum = _mm_min_epi16(sum, maxval);

                _mm_store_si128((__m128i*)&dst[x], sum);    // store block into dst
            }

            src0 += sstride0;
            src1 += sstride1;
            dst += dstride;
        }
    }
    else
    {
        int tmp;
        int max = (1 << X265_DEPTH) - 1;
        // slow path, irregular memory alignments or sizes
        for (int y = 0; y < by; y++)
        {
            for (int x = 0; x < bx; x++)
            {
                tmp = src0[x] + src1[x];
                tmp = tmp < 0 ? 0 : tmp;
                tmp = tmp > max ? max : tmp;
                dst[x] = (int16_t)tmp;
            }

            src0 += sstride0;
            src1 += sstride1;
            dst += dstride;
        }
    }
}
}

namespace x265 {
void Setup_Vec_BlockCopyPrimitives_sse3(EncoderPrimitives &p)
{
#if HIGH_BIT_DEPTH
    p.blockcpy_pp = blockcopy_pp;
    p.blockcpy_ps = (blockcpy_ps_t)blockcopy_pp;
    // At high bit depth, a pixel is a short
    p.pixeladd_ss = pixeladd_ss;
#else
    p.blockcpy_pp = blockcopy_pp;
    p.blockcpy_ps = blockcopy_ps;
    p.pixeladd_ss = pixeladd_ss;
#endif // if HIGH_BIT_DEPTH
}
}
