/*****************************************************************************
 * Copyright (C) 2016 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
;*          Min Chen <chenm003@163.com>
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

#ifndef X265_PIXEL_UTIL_ARM_H
#define X265_PIXEL_UTIL_ARM_H

uint64_t x265_pixel_var_8x8_neon(const pixel* pix, intptr_t stride);
uint64_t x265_pixel_var_16x16_neon(const pixel* pix, intptr_t stride);
uint64_t x265_pixel_var_32x32_neon(const pixel* pix, intptr_t stride);
uint64_t x265_pixel_var_64x64_neon(const pixel* pix, intptr_t stride);

void x265_getResidual4_neon(const pixel* fenc, const pixel* pred, int16_t* residual, intptr_t stride);
void x265_getResidual8_neon(const pixel* fenc, const pixel* pred, int16_t* residual, intptr_t stride);
void x265_getResidual16_neon(const pixel* fenc, const pixel* pred, int16_t* residual, intptr_t stride);
void x265_getResidual32_neon(const pixel* fenc, const pixel* pred, int16_t* residual, intptr_t stride);

void x265_scale1D_128to64_neon(pixel *dst, const pixel *src);
#endif // ifndef X265_PIXEL_UTIL_ARM_H
