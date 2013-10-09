/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Mandar Gurav <mandar@multicorewareinc.com>
 *          Mahesh Pittala <mahesh@multicorewareinc.com>
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

/* intrinsics for when pixel type is uint8_t */

void weightUnidir(short *src, pixel *dst, intptr_t srcStride, intptr_t dstStride, int width, int height, int w0, int round, int shift, int offset)
{
    __m128i w00, roundoff, ofs, fs, tmpsrc, tmpdst, tmp;
    int x, y;

    w00 = _mm_set1_epi32(w0);
    ofs = _mm_set1_epi32(IF_INTERNAL_OFFS);
    fs = _mm_set1_epi32(offset);
    roundoff = _mm_set1_epi32(round);
    for (y = height - 1; y >= 0; y--)
    {
        for (x = 0; x <= width - 4; x += 4)
        {
            tmpsrc = _mm_loadl_epi64((__m128i*)(src + x));
            tmpsrc = _mm_unpacklo_epi16(tmpsrc, _mm_setzero_si128());
            tmpdst = _mm_add_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_mullo_epi32(w00, _mm_add_epi32(tmpsrc, ofs)), roundoff), shift), fs);
            *(uint32_t*)(dst + x) = _mm_cvtsi128_si32(_mm_packus_epi16(_mm_packs_epi32(tmpdst, tmpdst), _mm_setzero_si128()));
        }

        if (width > x)
        {
            tmpsrc = _mm_loadl_epi64((__m128i*)(src + x));
            tmpsrc = _mm_unpacklo_epi16(tmpsrc, _mm_setzero_si128());
            tmpdst = _mm_add_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_mullo_epi32(w00, _mm_add_epi32(tmpsrc, ofs)), roundoff), shift), fs);
            tmp = _mm_packus_epi16(_mm_packs_epi32(tmpdst, tmpdst), _mm_setzero_si128());
            union
            {
                int8_t  c[16];
                int16_t s[8];
            } u;

            _mm_storeu_si128((__m128i*)u.c, tmp);
            ((int16_t*)(dst + x))[0] = u.s[0];    //to store only first 16-bit from 128-bit to memory
        }
        src += srcStride;
        dst += dstStride;
    }
}


void weightUnidirPixel(pixel *source, pixel *dest, intptr_t sourceStride, intptr_t destStride, int width, int height, int w0, int arg_round, int shift, int offset)
 {
     int x, y;
    __m128i temp;
    __m128i vw0    = _mm_set1_epi32(w0);                // broadcast (32-bit integer) w0 to all elements of vw0
    __m128i iofs   = _mm_set1_epi32(IF_INTERNAL_OFFS);
    __m128i ofs    = _mm_set1_epi32(offset);
    __m128i round  = _mm_set1_epi32(arg_round);
    __m128i src, dst;

     for (y = height - 1; y >= 0; y--)
     {
         for (x = 0; x <= width - 4; x += 4)
         {
             // The intermediate results would outgrow 16 bits because internal offset is too high
            temp = _mm_cvtsi32_si128(*(uint32_t*)(source + x));
            src = _mm_unpacklo_epi16(_mm_unpacklo_epi8(temp,_mm_setzero_si128()), _mm_setzero_si128());
            dst = _mm_add_epi32((_mm_mullo_epi32(vw0, _mm_add_epi32(src, iofs))), round);
            dst =  _mm_sra_epi32(dst, _mm_cvtsi32_si128(shift));
            dst = _mm_add_epi32(dst, ofs);
            *(uint32_t*)(dest + x) = _mm_cvtsi128_si32(_mm_packus_epi16(_mm_packs_epi32(dst, dst), _mm_setzero_si128()));
         }

         if (width > x)
         {
            temp = _mm_cvtsi32_si128(*(uint32_t*)(source + x));
            src = _mm_unpacklo_epi16(_mm_unpacklo_epi8(temp, _mm_setzero_si128()), _mm_setzero_si128());
            dst = _mm_add_epi32((_mm_mullo_epi32(vw0, _mm_add_epi32(src, iofs))), round);
            dst =  _mm_sra_epi32(dst, _mm_cvtsi32_si128(shift));
            dst = _mm_add_epi32(dst, ofs);
            temp = _mm_packus_epi16(_mm_packs_epi32(dst,dst), _mm_setzero_si128());

            union 
            {
                int8_t  c[16];
                int16_t s[8];
            } u;

            _mm_storeu_si128((__m128i*)u.c, temp);
            ((int16_t*)(dest + x))[0] = u.s[0];
         }
        source += sourceStride;
        dest += destStride;
     }
 }
