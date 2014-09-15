/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Nabajit Deka <nabajit@multicorewareinc.com>
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

#ifndef X265_DCT8_H
#define X265_DCT8_H
void x265_dct4_sse2(int16_t *src, int32_t *dst, intptr_t stride);
void x265_dct4_avx2(int16_t *src, int32_t *dst, intptr_t stride);
void x265_idct4_sse2(int32_t *src, int16_t *dst, intptr_t stride);
void x265_idct8_ssse3(int32_t *src, int16_t *dst, intptr_t stride);
void x265_dst4_ssse3(int16_t *src, int32_t *dst, intptr_t stride);
void x265_idst4_sse2(int32_t *src, int16_t *dst, intptr_t stride);
void x265_dct8_sse4(int16_t *src, int32_t *dst, intptr_t stride);
void x265_dct16_avx2(int16_t *src, int32_t *dst, intptr_t stride);
void x265_dct32_avx2(int16_t *src, int32_t *dst, intptr_t stride);

void x265_denoise_dct_mmx(int32_t *dct, uint32_t *sum, uint16_t *offset, int size);
void x265_denoise_dct_sse2(int32_t *dct, uint32_t *sum, uint16_t *offset, int size);
void x265_denoise_dct_ssse3(int32_t *dct, uint32_t *sum, uint16_t *offset, int size);
void x265_denoise_dct_avx(int32_t *dct, uint32_t *sum, uint16_t *offset, int size);
void x265_denoise_dct_avx2(int32_t *dct, uint32_t *sum, uint16_t *offset, int size);

#endif // ifndef X265_DCT8_H
