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

#ifndef X265_BLOCKCOPY8_H
#define X265_BLOCKCOPY8_H

void x265_cvt32to16_shr_sse2(int16_t *dst, int *src, intptr_t, int, int);

#define SETUP_CHROMA_BLOCKCOPY_FUNC(W, H, cpu) \
    void x265_blockcopy_pp_ ## W ## x ## H ## cpu(pixel * a, intptr_t stridea, pixel * b, intptr_t strideb); \
    void x265_blockcopy_sp_ ## W ## x ## H ## cpu(pixel * a, intptr_t stridea, int16_t * b, intptr_t strideb);

#define CHROMA_BLOCKCOPY_DEF(cpu) \
    SETUP_CHROMA_BLOCKCOPY_FUNC(4, 4, cpu); \
    SETUP_CHROMA_BLOCKCOPY_FUNC(4, 2, cpu); \
    SETUP_CHROMA_BLOCKCOPY_FUNC(2, 4, cpu); \
    SETUP_CHROMA_BLOCKCOPY_FUNC(8, 8, cpu); \
    SETUP_CHROMA_BLOCKCOPY_FUNC(8, 4, cpu); \
    SETUP_CHROMA_BLOCKCOPY_FUNC(4, 8, cpu); \
    SETUP_CHROMA_BLOCKCOPY_FUNC(8, 6, cpu); \
    SETUP_CHROMA_BLOCKCOPY_FUNC(6, 8, cpu); \
    SETUP_CHROMA_BLOCKCOPY_FUNC(8, 2, cpu); \
    SETUP_CHROMA_BLOCKCOPY_FUNC(2, 8, cpu); \
    SETUP_CHROMA_BLOCKCOPY_FUNC(16, 16, cpu); \
    SETUP_CHROMA_BLOCKCOPY_FUNC(16, 8, cpu); \
    SETUP_CHROMA_BLOCKCOPY_FUNC(8, 16, cpu); \
    SETUP_CHROMA_BLOCKCOPY_FUNC(16, 12, cpu); \
    SETUP_CHROMA_BLOCKCOPY_FUNC(12, 16, cpu); \
    SETUP_CHROMA_BLOCKCOPY_FUNC(16, 4, cpu); \
    SETUP_CHROMA_BLOCKCOPY_FUNC(4, 16, cpu); \
    SETUP_CHROMA_BLOCKCOPY_FUNC(32, 32, cpu); \
    SETUP_CHROMA_BLOCKCOPY_FUNC(32, 16, cpu); \
    SETUP_CHROMA_BLOCKCOPY_FUNC(16, 32, cpu); \
    SETUP_CHROMA_BLOCKCOPY_FUNC(32, 24, cpu); \
    SETUP_CHROMA_BLOCKCOPY_FUNC(24, 32, cpu); \
    SETUP_CHROMA_BLOCKCOPY_FUNC(32, 8, cpu); \
    SETUP_CHROMA_BLOCKCOPY_FUNC(8, 32, cpu);

#define SETUP_LUMA_BLOCKCOPY_FUNC(W, H, cpu) \
    void x265_blockcopy_pp_ ## W ## x ## H ## cpu(pixel * a, intptr_t stridea, pixel * b, intptr_t strideb); \
    void x265_blockcopy_sp_ ## W ## x ## H ## cpu(pixel * a, intptr_t stridea, int16_t * b, intptr_t strideb);

#define LUMA_BLOCKCOPY_DEF(cpu) \
    SETUP_LUMA_BLOCKCOPY_FUNC(4,   4, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC(8,   8, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC(8,   4, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC(4,   8, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC(16, 16, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC(16,  8, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC(8,  16, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC(16, 12, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC(12, 16, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC(16,  4, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC(4,  16, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC(32, 32, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC(32, 16, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC(16, 32, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC(32, 24, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC(24, 32, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC(32,  8, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC(8,  32, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC(64, 64, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC(64, 32, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC(32, 64, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC(64, 48, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC(48, 64, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC(64, 16, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC(16, 64, cpu);

CHROMA_BLOCKCOPY_DEF(_sse2);
LUMA_BLOCKCOPY_DEF(_sse2);

void x265_blockcopy_sp_2x4_sse4(pixel *a, intptr_t stridea, int16_t *b, intptr_t strideb);
void x265_blockcopy_sp_2x8_sse4(pixel *a, intptr_t stridea, int16_t *b, intptr_t strideb);
void x265_blockcopy_sp_6x8_sse4(pixel *a, intptr_t stridea, int16_t *b, intptr_t strideb);

void x265_blockfill_s_4x4_sse2(int16_t *dst, intptr_t dstride, int16_t val);
void x265_blockfill_s_8x8_sse2(int16_t *dst, intptr_t dstride, int16_t val);

#undef SETUP_CHROMA_BLOCKCOPY_FUNC
#undef SETUP_LUMA_BLOCK_FUNC
#undef CHROMA_BLOCKCOPY_DEF
#undef LUMA_BLOCKCOPY_DEF

#endif // ifndef X265_I386_PIXEL_H
