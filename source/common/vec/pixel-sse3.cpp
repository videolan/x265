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

/* this file instantiates SSE3 versions of the pixel primitives */

#include "primitives.h"
#include <assert.h>
#include <xmmintrin.h> // SSE
#include <pmmintrin.h> // SSE3

using namespace x265;

namespace {
void convert16to32_shl(int32_t *dst, int16_t *org, intptr_t stride, int shift, int size)
{
    int i, j;

    for (i = 0; i < size; i++)
    {
        for (j = 0; j < size; j += 4)
        {
            __m128i im16;
            __m128i im32;

            im16 = _mm_loadl_epi64((__m128i*)&org[i * stride + j]);
            im32 = _mm_srai_epi32(_mm_unpacklo_epi16(im16, im16), 16);
            im32 = _mm_slli_epi32(im32, shift);
            _mm_storeu_si128((__m128i*)dst, im32);

            dst += 4;
        }
    }
}

void convert16to16_shl(int16_t *dst, int16_t *org, int width, int height, intptr_t stride, int shift)
{
    int i, j;

    if (width == 4)
    {
        for (i = 0; i < height; i += 2)
        {
            __m128i T00, T01;

            T00 = _mm_loadl_epi64((__m128i*)&org[(i) * stride]);
            T01 = _mm_loadl_epi64((__m128i*)&org[(i + 1) * stride]);
            T00 = _mm_unpacklo_epi64(T00, T01);
            T00 = _mm_slli_epi16(T00, shift);
            _mm_storeu_si128((__m128i*)&dst[i * 4], T00);
        }
    }
    else
    {
        for (i = 0; i < height; i++)
        {
            for (j = 0; j < width; j += 8)
            {
                __m128i T00;

                T00 = _mm_loadu_si128((__m128i*)&org[i * stride + j]);
                T00 = _mm_slli_epi16(T00, shift);
                _mm_storeu_si128((__m128i*)&dst[i * width + j], T00);
            }
        }
    }
}
}

namespace x265 {
void Setup_Vec_PixelPrimitives_sse3(EncoderPrimitives &p)
{
    p.cvt16to32_shl = convert16to32_shl;
    p.cvt16to16_shl = convert16to16_shl;
}
}
