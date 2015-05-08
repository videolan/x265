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
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#ifndef X265_IPFILTER8_H
#define X265_IPFILTER8_H

#define SETUP_LUMA_FUNC_DEF(W, H, cpu) \
    void x265_interp_8tap_horiz_pp_ ## W ## x ## H ## cpu(const pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int coeffIdx); \
    void x265_interp_8tap_horiz_ps_ ## W ## x ## H ## cpu(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt); \
    void x265_interp_8tap_vert_pp_ ## W ## x ## H ## cpu(const pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int coeffIdx); \
    void x265_interp_8tap_vert_ps_ ## W ## x ## H ## cpu(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx);

#define LUMA_FILTERS(cpu) \
    SETUP_LUMA_FUNC_DEF(4,   4, cpu); \
    SETUP_LUMA_FUNC_DEF(8,   8, cpu); \
    SETUP_LUMA_FUNC_DEF(8,   4, cpu); \
    SETUP_LUMA_FUNC_DEF(4,   8, cpu); \
    SETUP_LUMA_FUNC_DEF(16, 16, cpu); \
    SETUP_LUMA_FUNC_DEF(16,  8, cpu); \
    SETUP_LUMA_FUNC_DEF(8,  16, cpu); \
    SETUP_LUMA_FUNC_DEF(16, 12, cpu); \
    SETUP_LUMA_FUNC_DEF(12, 16, cpu); \
    SETUP_LUMA_FUNC_DEF(16,  4, cpu); \
    SETUP_LUMA_FUNC_DEF(4,  16, cpu); \
    SETUP_LUMA_FUNC_DEF(32, 32, cpu); \
    SETUP_LUMA_FUNC_DEF(32, 16, cpu); \
    SETUP_LUMA_FUNC_DEF(16, 32, cpu); \
    SETUP_LUMA_FUNC_DEF(32, 24, cpu); \
    SETUP_LUMA_FUNC_DEF(24, 32, cpu); \
    SETUP_LUMA_FUNC_DEF(32,  8, cpu); \
    SETUP_LUMA_FUNC_DEF(8,  32, cpu); \
    SETUP_LUMA_FUNC_DEF(64, 64, cpu); \
    SETUP_LUMA_FUNC_DEF(64, 32, cpu); \
    SETUP_LUMA_FUNC_DEF(32, 64, cpu); \
    SETUP_LUMA_FUNC_DEF(64, 48, cpu); \
    SETUP_LUMA_FUNC_DEF(48, 64, cpu); \
    SETUP_LUMA_FUNC_DEF(64, 16, cpu); \
    SETUP_LUMA_FUNC_DEF(16, 64, cpu)

#define SETUP_LUMA_SP_FUNC_DEF(W, H, cpu) \
    void x265_interp_8tap_vert_sp_ ## W ## x ## H ## cpu(const int16_t* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int coeffIdx);

#define LUMA_SP_FILTERS(cpu) \
    SETUP_LUMA_SP_FUNC_DEF(4,   4, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(8,   8, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(8,   4, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(4,   8, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(16, 16, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(16,  8, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(8,  16, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(16, 12, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(12, 16, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(16,  4, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(4,  16, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(32, 32, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(32, 16, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(16, 32, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(32, 24, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(24, 32, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(32,  8, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(8,  32, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(64, 64, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(64, 32, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(32, 64, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(64, 48, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(48, 64, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(64, 16, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(16, 64, cpu);

#define SETUP_LUMA_SS_FUNC_DEF(W, H, cpu) \
    void x265_interp_8tap_vert_ss_ ## W ## x ## H ## cpu(const int16_t* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx);

#define LUMA_SS_FILTERS(cpu) \
    SETUP_LUMA_SS_FUNC_DEF(4,   4, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(8,   8, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(8,   4, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(4,   8, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(16, 16, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(16,  8, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(8,  16, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(16, 12, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(12, 16, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(16,  4, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(4,  16, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(32, 32, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(32, 16, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(16, 32, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(32, 24, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(24, 32, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(32,  8, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(8,  32, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(64, 64, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(64, 32, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(32, 64, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(64, 48, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(48, 64, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(64, 16, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(16, 64, cpu);

#if HIGH_BIT_DEPTH

#define SETUP_CHROMA_420_VERT_FUNC_DEF(W, H, cpu) \
    void x265_interp_4tap_vert_ss_ ## W ## x ## H ## cpu(const int16_t* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx); \
    void x265_interp_4tap_vert_sp_ ## W ## x ## H ## cpu(const int16_t* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int coeffIdx); \
    void x265_interp_4tap_vert_pp_ ## W ## x ## H ## cpu(const pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int coeffIdx); \
    void x265_interp_4tap_vert_ps_ ## W ## x ## H ## cpu(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx);

#define CHROMA_420_VERT_FILTERS(cpu) \
    SETUP_CHROMA_420_VERT_FUNC_DEF(4, 4, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(8, 8, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(8, 4, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(4, 8, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(8, 6, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(8, 2, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(16, 16, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(16, 8, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(8, 16, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(16, 12, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(12, 16, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(16, 4, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(4, 16, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(32, 32, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(32, 16, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(16, 32, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(32, 24, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(24, 32, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(32, 8, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(8, 32, cpu)

#define CHROMA_420_VERT_FILTERS_SSE4(cpu) \
    SETUP_CHROMA_420_VERT_FUNC_DEF(2, 4, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(2, 8, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(4, 2, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(6, 8, cpu);

#define CHROMA_422_VERT_FILTERS(cpu) \
    SETUP_CHROMA_420_VERT_FUNC_DEF(4, 8, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(8, 16, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(8, 8, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(4, 16, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(8, 12, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(8, 4, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(16, 32, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(16, 16, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(8, 32, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(16, 24, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(12, 32, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(16, 8, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(4, 32, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(32, 64, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(32, 32, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(16, 64, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(32, 48, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(24, 64, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(32, 16, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(8, 64, cpu);

#define CHROMA_422_VERT_FILTERS_SSE4(cpu) \
    SETUP_CHROMA_420_VERT_FUNC_DEF(2, 8, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(2, 16, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(4, 4, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(6, 16, cpu);

#define CHROMA_444_VERT_FILTERS(cpu) \
    SETUP_CHROMA_420_VERT_FUNC_DEF(8, 8, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(8, 4, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(4, 8, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(16, 16, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(16, 8, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(8, 16, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(16, 12, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(12, 16, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(16, 4, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(4, 16, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(32, 32, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(32, 16, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(16, 32, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(32, 24, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(24, 32, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(32, 8, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(8, 32, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(64, 64, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(64, 32, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(32, 64, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(64, 48, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(48, 64, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(64, 16, cpu); \
    SETUP_CHROMA_420_VERT_FUNC_DEF(16, 64, cpu)

#define SETUP_CHROMA_420_HORIZ_FUNC_DEF(W, H, cpu) \
    void x265_interp_4tap_horiz_pp_ ## W ## x ## H ## cpu(const pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int coeffIdx); \
    void x265_interp_4tap_horiz_ps_ ## W ## x ## H ## cpu(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt);

#define CHROMA_420_HORIZ_FILTERS(cpu) \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(4, 4, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(4, 2, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(2, 4, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(8, 8, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(8, 4, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(4, 8, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(8, 6, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(6, 8, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(8, 2, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(2, 8, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(16, 16, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(16, 8, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(8, 16, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(16, 12, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(12, 16, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(16, 4, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(4, 16, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(32, 32, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(32, 16, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(16, 32, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(32, 24, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(24, 32, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(32, 8, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(8, 32, cpu)

#define CHROMA_422_HORIZ_FILTERS(cpu) \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(4, 8, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(4, 4, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(2, 8, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(8, 16, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(8, 8, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(4, 16, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(8, 12, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(6, 16, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(8, 4, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(2, 16, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(16, 32, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(16, 16, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(8, 32, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(16, 24, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(12, 32, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(16, 8, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(4, 32, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(32, 64, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(32, 32, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(16, 64, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(32, 48, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(24, 64, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(32, 16, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(8, 64, cpu)

#define CHROMA_444_HORIZ_FILTERS(cpu) \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(8, 8, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(8, 4, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(4, 8, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(16, 16, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(16, 8, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(8, 16, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(16, 12, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(12, 16, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(16, 4, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(4, 16, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(32, 32, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(32, 16, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(16, 32, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(32, 24, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(24, 32, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(32, 8, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(8, 32, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(64, 64, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(64, 32, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(32, 64, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(64, 48, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(48, 64, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(64, 16, cpu); \
    SETUP_CHROMA_420_HORIZ_FUNC_DEF(16, 64, cpu)

void x265_filterPixelToShort_4x4_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_4x8_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_4x16_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_8x4_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_8x8_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_8x16_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_8x32_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_16x4_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_16x8_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_16x12_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_16x16_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_16x32_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_16x64_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_32x8_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_32x16_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_32x24_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_32x32_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_32x64_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_64x16_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_64x32_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_64x48_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_64x64_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_24x32_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_12x16_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_48x64_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_16x4_avx2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_16x8_avx2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_16x12_avx2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_16x16_avx2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_16x32_avx2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_16x64_avx2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_32x8_avx2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_32x16_avx2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_32x24_avx2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_32x32_avx2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_32x64_avx2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_64x16_avx2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_64x32_avx2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_64x48_avx2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_64x64_avx2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_24x32_avx2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_48x64_avx2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);

#define SETUP_CHROMA_P2S_FUNC_DEF(W, H, cpu) \
    void x265_filterPixelToShort_ ## W ## x ## H ## cpu(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);

#define CHROMA_420_P2S_FILTERS_SSSE3(cpu) \
    SETUP_CHROMA_P2S_FUNC_DEF(4, 2, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(8, 2, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(8, 6, cpu);

#define CHROMA_420_P2S_FILTERS_SSE4(cpu) \
    SETUP_CHROMA_P2S_FUNC_DEF(2, 4, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(2, 8, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(6, 8, cpu);

#define CHROMA_422_P2S_FILTERS_SSSE3(cpu) \
    SETUP_CHROMA_P2S_FUNC_DEF(4, 32, cpu) \
    SETUP_CHROMA_P2S_FUNC_DEF(8, 12, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(8, 64, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(12, 32, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(16, 24, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(16, 64, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(24, 64, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(32, 48, cpu);

#define CHROMA_422_P2S_FILTERS_SSE4(cpu) \
    SETUP_CHROMA_P2S_FUNC_DEF(2, 8, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(2, 16, cpu) \
    SETUP_CHROMA_P2S_FUNC_DEF(6, 16, cpu);

#define CHROMA_420_P2S_FILTERS_AVX2(cpu) \
    SETUP_CHROMA_P2S_FUNC_DEF(16, 4, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(16, 8, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(16, 12, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(16, 16, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(16, 32, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(24, 32, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(32, 8, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(32, 16, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(32, 24, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(32, 32, cpu);

#define CHROMA_422_P2S_FILTERS_AVX2(cpu) \
    SETUP_CHROMA_P2S_FUNC_DEF(16, 8, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(16, 16, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(16, 24, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(16, 32, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(16, 64, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(24, 64, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(32, 16, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(32, 32, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(32, 48, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(32, 64, cpu);

CHROMA_420_VERT_FILTERS(_sse2);
CHROMA_420_HORIZ_FILTERS(_sse4);
CHROMA_420_VERT_FILTERS_SSE4(_sse4);
CHROMA_420_P2S_FILTERS_SSSE3(_ssse3);
CHROMA_420_P2S_FILTERS_SSE4(_sse4);
CHROMA_420_P2S_FILTERS_AVX2(_avx2);

CHROMA_422_VERT_FILTERS(_sse2);
CHROMA_422_HORIZ_FILTERS(_sse4);
CHROMA_422_VERT_FILTERS_SSE4(_sse4);
CHROMA_422_P2S_FILTERS_SSE4(_sse4);
CHROMA_422_P2S_FILTERS_SSSE3(_ssse3);
CHROMA_422_P2S_FILTERS_AVX2(_avx2);

CHROMA_444_VERT_FILTERS(_sse2);
CHROMA_444_HORIZ_FILTERS(_sse4);

#undef CHROMA_420_VERT_FILTERS_SSE4
#undef CHROMA_420_VERT_FILTERS
#undef SETUP_CHROMA_420_VERT_FUNC_DEF
#undef CHROMA_420_HORIZ_FILTERS
#undef SETUP_CHROMA_420_HORIZ_FUNC_DEF

#undef CHROMA_422_VERT_FILTERS
#undef CHROMA_422_VERT_FILTERS_SSE4
#undef CHROMA_422_HORIZ_FILTERS

#undef CHROMA_444_VERT_FILTERS
#undef CHROMA_444_HORIZ_FILTERS

#else // if HIGH_BIT_DEPTH

#define SETUP_CHROMA_FUNC_DEF(W, H, cpu) \
    void x265_interp_4tap_horiz_pp_ ## W ## x ## H ## cpu(const pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int coeffIdx); \
    void x265_interp_4tap_horiz_ps_ ## W ## x ## H ## cpu(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt); \
    void x265_interp_4tap_vert_pp_ ## W ## x ## H ## cpu(const pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int coeffIdx); \
    void x265_interp_4tap_vert_ps_ ## W ## x ## H ## cpu(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx);

#define CHROMA_420_FILTERS(cpu) \
    SETUP_CHROMA_FUNC_DEF(4, 4, cpu); \
    SETUP_CHROMA_FUNC_DEF(4, 2, cpu); \
    SETUP_CHROMA_FUNC_DEF(2, 4, cpu); \
    SETUP_CHROMA_FUNC_DEF(8, 8, cpu); \
    SETUP_CHROMA_FUNC_DEF(8, 4, cpu); \
    SETUP_CHROMA_FUNC_DEF(4, 8, cpu); \
    SETUP_CHROMA_FUNC_DEF(8, 6, cpu); \
    SETUP_CHROMA_FUNC_DEF(6, 8, cpu); \
    SETUP_CHROMA_FUNC_DEF(8, 2, cpu); \
    SETUP_CHROMA_FUNC_DEF(2, 8, cpu); \
    SETUP_CHROMA_FUNC_DEF(16, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF(16, 8, cpu); \
    SETUP_CHROMA_FUNC_DEF(8, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF(16, 12, cpu); \
    SETUP_CHROMA_FUNC_DEF(12, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF(16, 4, cpu); \
    SETUP_CHROMA_FUNC_DEF(4, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF(32, 32, cpu); \
    SETUP_CHROMA_FUNC_DEF(32, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF(16, 32, cpu); \
    SETUP_CHROMA_FUNC_DEF(32, 24, cpu); \
    SETUP_CHROMA_FUNC_DEF(24, 32, cpu); \
    SETUP_CHROMA_FUNC_DEF(32, 8, cpu); \
    SETUP_CHROMA_FUNC_DEF(8, 32, cpu)

#define CHROMA_422_FILTERS(cpu) \
    SETUP_CHROMA_FUNC_DEF(4, 8, cpu); \
    SETUP_CHROMA_FUNC_DEF(4, 4, cpu); \
    SETUP_CHROMA_FUNC_DEF(2, 8, cpu); \
    SETUP_CHROMA_FUNC_DEF(8, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF(8, 8, cpu); \
    SETUP_CHROMA_FUNC_DEF(4, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF(8, 12, cpu); \
    SETUP_CHROMA_FUNC_DEF(6, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF(8, 4, cpu); \
    SETUP_CHROMA_FUNC_DEF(2, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF(16, 32, cpu); \
    SETUP_CHROMA_FUNC_DEF(16, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF(8, 32, cpu); \
    SETUP_CHROMA_FUNC_DEF(16, 24, cpu); \
    SETUP_CHROMA_FUNC_DEF(12, 32, cpu); \
    SETUP_CHROMA_FUNC_DEF(16, 8, cpu); \
    SETUP_CHROMA_FUNC_DEF(4, 32, cpu); \
    SETUP_CHROMA_FUNC_DEF(32, 64, cpu); \
    SETUP_CHROMA_FUNC_DEF(32, 32, cpu); \
    SETUP_CHROMA_FUNC_DEF(16, 64, cpu); \
    SETUP_CHROMA_FUNC_DEF(32, 48, cpu); \
    SETUP_CHROMA_FUNC_DEF(24, 64, cpu); \
    SETUP_CHROMA_FUNC_DEF(32, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF(8, 64, cpu);

#define CHROMA_444_FILTERS(cpu) \
    SETUP_CHROMA_FUNC_DEF(8, 8, cpu); \
    SETUP_CHROMA_FUNC_DEF(8, 4, cpu); \
    SETUP_CHROMA_FUNC_DEF(4, 8, cpu); \
    SETUP_CHROMA_FUNC_DEF(16, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF(16, 8, cpu); \
    SETUP_CHROMA_FUNC_DEF(8, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF(16, 12, cpu); \
    SETUP_CHROMA_FUNC_DEF(12, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF(16, 4, cpu); \
    SETUP_CHROMA_FUNC_DEF(4, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF(32, 32, cpu); \
    SETUP_CHROMA_FUNC_DEF(32, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF(16, 32, cpu); \
    SETUP_CHROMA_FUNC_DEF(32, 24, cpu); \
    SETUP_CHROMA_FUNC_DEF(24, 32, cpu); \
    SETUP_CHROMA_FUNC_DEF(32, 8, cpu); \
    SETUP_CHROMA_FUNC_DEF(8, 32, cpu); \
    SETUP_CHROMA_FUNC_DEF(64, 64, cpu); \
    SETUP_CHROMA_FUNC_DEF(64, 32, cpu); \
    SETUP_CHROMA_FUNC_DEF(32, 64, cpu); \
    SETUP_CHROMA_FUNC_DEF(64, 48, cpu); \
    SETUP_CHROMA_FUNC_DEF(48, 64, cpu); \
    SETUP_CHROMA_FUNC_DEF(64, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF(16, 64, cpu);

#define SETUP_CHROMA_SP_FUNC_DEF(W, H, cpu) \
    void x265_interp_4tap_vert_sp_ ## W ## x ## H ## cpu(const int16_t* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int coeffIdx);

#define CHROMA_420_SP_FILTERS(cpu) \
    SETUP_CHROMA_SP_FUNC_DEF(8, 2, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(8, 4, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(8, 6, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(8, 8, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(8, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(8, 32, cpu);

#define CHROMA_420_SP_FILTERS_SSE4(cpu) \
    SETUP_CHROMA_SP_FUNC_DEF(2, 4, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(2, 8, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(4, 2, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(4, 4, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(4, 8, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(4, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(6, 8, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(16, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(16, 8, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(16, 12, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(12, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(16, 4, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(32, 32, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(32, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(16, 32, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(32, 24, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(24, 32, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(32, 8, cpu);

#define CHROMA_422_SP_FILTERS(cpu) \
    SETUP_CHROMA_SP_FUNC_DEF(8, 4, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(8, 8, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(8, 12, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(8, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(8, 32, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(8, 64, cpu);

#define CHROMA_422_SP_FILTERS_SSE4(cpu) \
    SETUP_CHROMA_SP_FUNC_DEF(2, 8, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(2, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(4, 4, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(4, 8, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(4, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(4, 32, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(6, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(16, 32, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(16, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(16, 24, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(12, 32, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(16, 8, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(32, 64, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(32, 32, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(16, 64, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(32, 48, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(24, 64, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(32, 16, cpu);

#define CHROMA_444_SP_FILTERS(cpu) \
    SETUP_CHROMA_SP_FUNC_DEF(8, 8, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(8, 4, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(4, 8, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(16, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(16, 8, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(8, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(16, 12, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(12, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(16, 4, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(4, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(32, 32, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(32, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(16, 32, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(32, 24, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(24, 32, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(32, 8, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(8, 32, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(64, 64, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(64, 32, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(32, 64, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(64, 48, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(48, 64, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(64, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(16, 64, cpu);

#define SETUP_CHROMA_SS_FUNC_DEF(W, H, cpu) \
    void x265_interp_4tap_vert_ss_ ## W ## x ## H ## cpu(const int16_t* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx);

#define CHROMA_420_SS_FILTERS(cpu) \
    SETUP_CHROMA_SS_FUNC_DEF(4, 4, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(4, 2, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(8, 8, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(8, 4, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(4, 8, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(8, 6, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(8, 2, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(16, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(16, 8, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(8, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(16, 12, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(12, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(16, 4, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(4, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(32, 32, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(32, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(16, 32, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(32, 24, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(24, 32, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(32, 8, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(8, 32, cpu);

#define CHROMA_420_SS_FILTERS_SSE4(cpu) \
    SETUP_CHROMA_SS_FUNC_DEF(2, 4, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(2, 8, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(6, 8, cpu);

#define CHROMA_422_SS_FILTERS(cpu) \
    SETUP_CHROMA_SS_FUNC_DEF(4, 8, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(4, 4, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(8, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(8, 8, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(4, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(8, 12, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(8, 4, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(16, 32, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(16, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(8, 32, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(16, 24, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(12, 32, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(16, 8, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(4, 32, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(32, 64, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(32, 32, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(16, 64, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(32, 48, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(24, 64, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(32, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(8, 64, cpu);

#define CHROMA_422_SS_FILTERS_SSE4(cpu) \
    SETUP_CHROMA_SS_FUNC_DEF(2, 8, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(2, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(6, 16, cpu);

#define CHROMA_444_SS_FILTERS(cpu) \
    SETUP_CHROMA_SS_FUNC_DEF(8, 8, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(8, 4, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(4, 8, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(16, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(16, 8, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(8, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(16, 12, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(12, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(16, 4, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(4, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(32, 32, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(32, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(16, 32, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(32, 24, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(24, 32, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(32, 8, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(8, 32, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(64, 64, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(64, 32, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(32, 64, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(64, 48, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(48, 64, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(64, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(16, 64, cpu);

#define SETUP_CHROMA_P2S_FUNC_DEF(W, H, cpu) \
    void x265_filterPixelToShort_ ## W ## x ## H ## cpu(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);

#define CHROMA_420_P2S_FILTERS_SSE4(cpu) \
    SETUP_CHROMA_P2S_FUNC_DEF(2, 4, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(2, 8, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(4, 2, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(6, 8, cpu); 

#define CHROMA_420_P2S_FILTERS_SSSE3(cpu) \
    SETUP_CHROMA_P2S_FUNC_DEF(8, 2, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(8, 6, cpu);

#define CHROMA_422_P2S_FILTERS_SSE4(cpu) \
    SETUP_CHROMA_P2S_FUNC_DEF(2, 8, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(2, 16, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(6, 16, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(4, 32, cpu);

#define CHROMA_422_P2S_FILTERS_SSSE3(cpu) \
    SETUP_CHROMA_P2S_FUNC_DEF(8, 12, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(8, 64, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(12, 32, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(16, 24, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(16, 64, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(24, 64, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(32, 48, cpu);

#define CHROMA_420_P2S_FILTERS_AVX2(cpu) \
    SETUP_CHROMA_P2S_FUNC_DEF(24, 32, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(32, 8, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(32, 16, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(32, 24, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(32, 32, cpu);

#define CHROMA_422_P2S_FILTERS_AVX2(cpu) \
    SETUP_CHROMA_P2S_FUNC_DEF(24, 64, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(32, 16, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(32, 32, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(32, 48, cpu); \
    SETUP_CHROMA_P2S_FUNC_DEF(32, 64, cpu);

CHROMA_420_FILTERS(_sse4);
CHROMA_420_FILTERS(_avx2);
CHROMA_420_SP_FILTERS(_sse2);
CHROMA_420_SP_FILTERS_SSE4(_sse4);
CHROMA_420_SP_FILTERS(_avx2);
CHROMA_420_SP_FILTERS_SSE4(_avx2);
CHROMA_420_SS_FILTERS(_sse2);
CHROMA_420_SS_FILTERS_SSE4(_sse4);
CHROMA_420_SS_FILTERS(_avx2);
CHROMA_420_SS_FILTERS_SSE4(_avx2);
CHROMA_420_P2S_FILTERS_SSE4(_sse4);
CHROMA_420_P2S_FILTERS_SSSE3(_ssse3);
CHROMA_420_P2S_FILTERS_AVX2(_avx2);

CHROMA_422_FILTERS(_sse4);
CHROMA_422_FILTERS(_avx2);
CHROMA_422_SP_FILTERS(_sse2);
CHROMA_422_SP_FILTERS(_avx2);
CHROMA_422_SP_FILTERS_SSE4(_sse4);
CHROMA_422_SP_FILTERS_SSE4(_avx2);
CHROMA_422_SS_FILTERS(_sse2);
CHROMA_422_SS_FILTERS(_avx2);
CHROMA_422_SS_FILTERS_SSE4(_sse4);
CHROMA_422_SS_FILTERS_SSE4(_avx2);
CHROMA_422_P2S_FILTERS_SSE4(_sse4);
CHROMA_422_P2S_FILTERS_SSSE3(_ssse3);
CHROMA_422_P2S_FILTERS_AVX2(_avx2);
void x265_interp_4tap_vert_ss_2x4_avx2(const int16_t* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_vert_sp_2x4_avx2(const int16_t* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int coeffIdx);

CHROMA_444_FILTERS(_sse4);
CHROMA_444_SP_FILTERS(_sse4);
CHROMA_444_SS_FILTERS(_sse2);
CHROMA_444_FILTERS(_avx2);
CHROMA_444_SP_FILTERS(_avx2);
CHROMA_444_SS_FILTERS(_avx2);

#undef SETUP_CHROMA_FUNC_DEF
#undef SETUP_CHROMA_SP_FUNC_DEF
#undef SETUP_CHROMA_SS_FUNC_DEF
#undef CHROMA_420_FILTERS
#undef CHROMA_420_SP_FILTERS
#undef CHROMA_420_SS_FILTERS
#undef CHROMA_420_SS_FILTERS_SSE4
#undef CHROMA_420_SP_FILTERS_SSE4

#undef CHROMA_422_FILTERS
#undef CHROMA_422_SP_FILTERS
#undef CHROMA_422_SS_FILTERS
#undef CHROMA_422_SS_FILTERS_SSE4
#undef CHROMA_422_SP_FILTERS_SSE4

#undef CHROMA_444_FILTERS
#undef CHROMA_444_SP_FILTERS
#undef CHROMA_444_SS_FILTERS

#endif // if HIGH_BIT_DEPTH

LUMA_FILTERS(_sse4);
LUMA_SP_FILTERS(_sse4);
LUMA_SS_FILTERS(_sse2);
LUMA_FILTERS(_avx2);
LUMA_SP_FILTERS(_avx2);
LUMA_SS_FILTERS(_avx2);
void x265_interp_8tap_hv_pp_8x8_ssse3(const pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int idxX, int idxY);
void x265_interp_8tap_hv_pp_16x16_avx2(const pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int idxX, int idxY);
void x265_filterPixelToShort_4x4_sse4(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_4x8_sse4(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_4x16_sse4(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_8x4_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_8x8_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_8x16_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_8x32_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_16x4_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_16x8_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_16x12_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_16x16_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_16x32_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_16x64_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_32x8_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_32x16_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_32x24_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_32x32_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_32x64_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_64x16_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_64x32_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_64x48_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_64x64_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_12x16_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_24x32_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_48x64_ssse3(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_32x8_avx2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_32x16_avx2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_32x24_avx2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_32x32_avx2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_32x64_avx2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_64x16_avx2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_64x32_avx2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_64x48_avx2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_64x64_avx2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_48x64_avx2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_filterPixelToShort_24x32_avx2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride);
void x265_interp_4tap_horiz_pp_2x4_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_2x8_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_2x16_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_4x2_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_4x4_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_4x8_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_4x16_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_4x32_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_6x8_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_6x16_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_8x2_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_8x4_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_8x6_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_8x8_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_8x12_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_8x16_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_8x32_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_8x64_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_12x16_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_12x32_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_16x4_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_16x8_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_16x12_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_16x16_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_16x24_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_16x32_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_16x64_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_24x32_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_24x64_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_32x8_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_32x16_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_32x24_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_32x32_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_32x48_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_32x64_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_48x64_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_64x16_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_64x32_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_64x48_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_horiz_pp_64x64_sse3(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_8tap_horiz_pp_4x4_sse2(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_8tap_horiz_pp_4x8_sse2(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_8tap_horiz_pp_4x16_sse2(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_8tap_horiz_pp_8x4_sse2(const pixel* src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_8tap_horiz_pp_8x8_sse2(const pixel* src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_8tap_horiz_pp_8x16_sse2(const pixel* src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_8tap_horiz_pp_8x32_sse2(const pixel* src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_8tap_horiz_pp_12x16_sse2(const pixel* src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_8tap_horiz_pp_16x4_sse2(const pixel* src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_8tap_horiz_pp_16x8_sse2(const pixel* src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_8tap_horiz_pp_16x12_sse2(const pixel* src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_8tap_horiz_pp_16x16_sse2(const pixel* src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_8tap_horiz_pp_16x32_sse2(const pixel* src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_8tap_horiz_pp_16x64_sse2(const pixel* src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_8tap_horiz_pp_24x32_sse2(const pixel* src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_8tap_horiz_pp_32x8_sse2(const pixel* src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_8tap_horiz_pp_32x16_sse2(const pixel* src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_8tap_horiz_pp_32x24_sse2(const pixel* src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_8tap_horiz_pp_32x32_sse2(const pixel* src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_8tap_horiz_pp_32x64_sse2(const pixel* src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_8tap_horiz_pp_48x64_sse2(const pixel* src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_8tap_horiz_pp_64x16_sse2(const pixel* src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_8tap_horiz_pp_64x32_sse2(const pixel* src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_8tap_horiz_pp_64x48_sse2(const pixel* src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_8tap_horiz_pp_64x64_sse2(const pixel* src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_8tap_horiz_ps_4x4_sse2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt);
void x265_interp_8tap_horiz_ps_4x8_sse2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt);
void x265_interp_8tap_horiz_ps_4x16_sse2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt);
void x265_interp_8tap_horiz_ps_8x4_sse2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt);
void x265_interp_8tap_horiz_ps_8x8_sse2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt);
void x265_interp_8tap_horiz_ps_8x16_sse2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt);
void x265_interp_8tap_horiz_ps_8x32_sse2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt);
void x265_interp_8tap_horiz_ps_12x16_sse2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt);
void x265_interp_8tap_horiz_ps_16x4_sse2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt);
void x265_interp_8tap_horiz_ps_16x8_sse2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt);
void x265_interp_8tap_horiz_ps_16x12_sse2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt);
void x265_interp_8tap_horiz_ps_16x16_sse2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt);
void x265_interp_8tap_horiz_ps_16x32_sse2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt);
void x265_interp_8tap_horiz_ps_16x64_sse2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt);
void x265_interp_8tap_horiz_ps_24x32_sse2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt);
void x265_interp_8tap_horiz_ps_32x8_sse2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt);
void x265_interp_8tap_horiz_ps_32x16_sse2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt);
void x265_interp_8tap_horiz_ps_32x24_sse2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt);
void x265_interp_8tap_horiz_ps_32x32_sse2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt);
void x265_interp_8tap_horiz_ps_32x64_sse2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt);
void x265_interp_8tap_horiz_ps_48x64_sse2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt);
void x265_interp_8tap_horiz_ps_64x16_sse2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt);
void x265_interp_8tap_horiz_ps_64x32_sse2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt);
void x265_interp_8tap_horiz_ps_64x48_sse2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt);
void x265_interp_8tap_horiz_ps_64x64_sse2(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt);
void x265_interp_8tap_hv_pp_8x8_sse3(const pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int idxX, int idxY);
void x265_interp_4tap_vert_pp_2x4_sse2(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_vert_pp_2x8_sse2(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
void x265_interp_4tap_vert_pp_2x16_sse2(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
#undef LUMA_FILTERS
#undef LUMA_SP_FILTERS
#undef LUMA_SS_FILTERS
#undef SETUP_LUMA_FUNC_DEF
#undef SETUP_LUMA_SP_FUNC_DEF
#undef SETUP_LUMA_SS_FUNC_DEF

#endif // ifndef X265_MC_H
