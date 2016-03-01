/*****************************************************************************
 * Copyright (C) 2016 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Dnyaneshwar Gorade <dnyaneshwar@multicorewareinc.com>
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
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#ifndef X265_IPFILTER8_ARM_H
#define X265_IPFILTER8_ARM_H

void x265_filterPixelToShort_4x4_neon(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_4x8_neon(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_4x16_neon(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_8x4_neon(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_8x8_neon(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_8x16_neon(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_8x32_neon(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_12x16_neon(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_16x4_neon(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_16x8_neon(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_16x12_neon(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_16x16_neon(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_16x32_neon(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_16x64_neon(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_24x32_neon(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_32x8_neon(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_32x16_neon(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_32x24_neon(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_32x32_neon(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_32x64_neon(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_48x64_neon(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_64x16_neon(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_64x32_neon(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_64x48_neon(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_64x64_neon(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);

#endif // ifndef X265_IPFILTER8_ARM_H
