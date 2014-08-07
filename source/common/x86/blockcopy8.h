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

#ifndef X265_BLOCKCOPY8_H
#define X265_BLOCKCOPY8_H

void x265_cvt32to16_shr_sse2(int16_t * dst, int *src, intptr_t, int, int);
void x265_cvt16to32_shl_sse4(int32_t * dst, int16_t * src, intptr_t, int32_t, int32_t);
void x265_cvt16to32_shr_4_sse4(int32_t * dst, int16_t * src, intptr_t, int32_t, int32_t);
void x265_cvt16to32_shr_8_sse4(int32_t * dst, int16_t * src, intptr_t, int32_t, int32_t);
void x265_cvt16to32_shr_16_sse4(int32_t * dst, int16_t * src, intptr_t, int32_t, int32_t);
void x265_cvt16to32_shr_32_sse4(int32_t * dst, int16_t * src, intptr_t, int32_t, int32_t);
uint32_t x265_cvt16to32_cnt_4_sse4(int32_t * dst, int16_t * src, intptr_t);
uint32_t x265_cvt16to32_cnt_8_sse4(int32_t * dst, int16_t * src, intptr_t);
uint32_t x265_cvt16to32_cnt_16_sse4(int32_t * dst, int16_t * src, intptr_t);
uint32_t x265_cvt16to32_cnt_32_sse4(int32_t * dst, int16_t * src, intptr_t);
uint32_t x265_cvt16to32_cnt_4_avx2(int32_t * dst, int16_t * src, intptr_t);
uint32_t x265_cvt16to32_cnt_8_avx2(int32_t * dst, int16_t * src, intptr_t);
uint32_t x265_cvt16to32_cnt_16_avx2(int32_t * dst, int16_t * src, intptr_t);
uint32_t x265_cvt16to32_cnt_32_avx2(int32_t * dst, int16_t * src, intptr_t);

#define SETUP_BLOCKCOPY_FUNC(W, H, cpu) \
    void x265_blockcopy_pp_ ## W ## x ## H ## cpu(pixel * a, intptr_t stridea, pixel * b, intptr_t strideb); \
    void x265_blockcopy_sp_ ## W ## x ## H ## cpu(pixel * a, intptr_t stridea, int16_t * b, intptr_t strideb); \
    void x265_blockcopy_ss_ ## W ## x ## H ## cpu(int16_t * a, intptr_t stridea, int16_t * b, intptr_t strideb);

#define SETUP_BLOCKCOPY_PS(W, H, cpu) \
    void x265_blockcopy_ps_ ## W ## x ## H ## cpu(int16_t * dst, intptr_t dstStride, pixel * src, intptr_t srcStride);

#define SETUP_BLOCKCOPY_SP(W, H, cpu) \
    void x265_blockcopy_sp_ ## W ## x ## H ## cpu(pixel * a, intptr_t stridea, int16_t * b, intptr_t strideb);

#define SETUP_BLOCKCOPY_SS_PP(W, H, cpu) \
    void x265_blockcopy_pp_ ## W ## x ## H ## cpu(pixel * a, intptr_t stridea, pixel * b, intptr_t strideb); \
    void x265_blockcopy_ss_ ## W ## x ## H ## cpu(int16_t * a, intptr_t stridea, int16_t * b, intptr_t strideb);

#define BLOCKCOPY_COMMON(cpu) \
    SETUP_BLOCKCOPY_FUNC(4, 4, cpu); \
    SETUP_BLOCKCOPY_FUNC(4, 2, cpu); \
    SETUP_BLOCKCOPY_FUNC(8, 8, cpu); \
    SETUP_BLOCKCOPY_FUNC(8, 4, cpu); \
    SETUP_BLOCKCOPY_FUNC(4, 8, cpu); \
    SETUP_BLOCKCOPY_FUNC(8, 6, cpu); \
    SETUP_BLOCKCOPY_FUNC(8, 2, cpu); \
    SETUP_BLOCKCOPY_FUNC(16, 16, cpu); \
    SETUP_BLOCKCOPY_FUNC(16, 8, cpu); \
    SETUP_BLOCKCOPY_FUNC(8, 16, cpu); \
    SETUP_BLOCKCOPY_FUNC(16, 12, cpu); \
    SETUP_BLOCKCOPY_FUNC(12, 16, cpu); \
    SETUP_BLOCKCOPY_FUNC(16, 4, cpu); \
    SETUP_BLOCKCOPY_FUNC(4, 16, cpu); \
    SETUP_BLOCKCOPY_FUNC(32, 32, cpu); \
    SETUP_BLOCKCOPY_FUNC(32, 16, cpu); \
    SETUP_BLOCKCOPY_FUNC(16, 32, cpu); \
    SETUP_BLOCKCOPY_FUNC(32, 24, cpu); \
    SETUP_BLOCKCOPY_FUNC(24, 32, cpu); \
    SETUP_BLOCKCOPY_FUNC(32, 8, cpu); \
    SETUP_BLOCKCOPY_FUNC(8, 32, cpu); \
    SETUP_BLOCKCOPY_FUNC(64, 64, cpu); \
    SETUP_BLOCKCOPY_FUNC(64, 32, cpu); \
    SETUP_BLOCKCOPY_FUNC(32, 64, cpu); \
    SETUP_BLOCKCOPY_FUNC(64, 48, cpu); \
    SETUP_BLOCKCOPY_FUNC(48, 64, cpu); \
    SETUP_BLOCKCOPY_FUNC(64, 16, cpu); \
    SETUP_BLOCKCOPY_FUNC(16, 64, cpu);

#define BLOCKCOPY_SP(cpu) \
    SETUP_BLOCKCOPY_SP(2, 4, cpu); \
    SETUP_BLOCKCOPY_SP(2, 8, cpu); \
    SETUP_BLOCKCOPY_SP(6, 8, cpu); \
    \
    SETUP_BLOCKCOPY_SP(2, 16, cpu); \
    SETUP_BLOCKCOPY_SP(4, 32, cpu); \
    SETUP_BLOCKCOPY_SP(6, 16, cpu); \
    SETUP_BLOCKCOPY_SP(8, 12, cpu); \
    SETUP_BLOCKCOPY_SP(8, 64, cpu); \
    SETUP_BLOCKCOPY_SP(12, 32, cpu); \
    SETUP_BLOCKCOPY_SP(16, 24, cpu); \
    SETUP_BLOCKCOPY_SP(24, 64, cpu); \
    SETUP_BLOCKCOPY_SP(32, 48, cpu);

#define BLOCKCOPY_SS_PP(cpu) \
    SETUP_BLOCKCOPY_SS_PP(2, 4, cpu); \
    SETUP_BLOCKCOPY_SS_PP(2, 8, cpu); \
    SETUP_BLOCKCOPY_SS_PP(6, 8, cpu); \
    \
    SETUP_BLOCKCOPY_SS_PP(2, 16, cpu); \
    SETUP_BLOCKCOPY_SS_PP(4, 32, cpu); \
    SETUP_BLOCKCOPY_SS_PP(6, 16, cpu); \
    SETUP_BLOCKCOPY_SS_PP(8, 12, cpu); \
    SETUP_BLOCKCOPY_SS_PP(8, 64, cpu); \
    SETUP_BLOCKCOPY_SS_PP(12, 32, cpu); \
    SETUP_BLOCKCOPY_SS_PP(16, 24, cpu); \
    SETUP_BLOCKCOPY_SS_PP(24, 64, cpu); \
    SETUP_BLOCKCOPY_SS_PP(32, 48, cpu);
    

#define BLOCKCOPY_PS(cpu) \
    SETUP_BLOCKCOPY_PS(2, 4, cpu); \
    SETUP_BLOCKCOPY_PS(2, 8, cpu); \
    SETUP_BLOCKCOPY_PS(4, 2, cpu); \
    SETUP_BLOCKCOPY_PS(4, 4, cpu); \
    SETUP_BLOCKCOPY_PS(4, 8, cpu); \
    SETUP_BLOCKCOPY_PS(4, 16, cpu); \
    SETUP_BLOCKCOPY_PS(6, 8, cpu); \
    SETUP_BLOCKCOPY_PS(8, 2, cpu); \
    SETUP_BLOCKCOPY_PS(8, 4, cpu); \
    SETUP_BLOCKCOPY_PS(8, 6, cpu); \
    SETUP_BLOCKCOPY_PS(8, 8, cpu); \
    SETUP_BLOCKCOPY_PS(8, 16, cpu); \
    SETUP_BLOCKCOPY_PS(8, 32, cpu); \
    SETUP_BLOCKCOPY_PS(12, 16, cpu); \
    SETUP_BLOCKCOPY_PS(16, 4, cpu); \
    SETUP_BLOCKCOPY_PS(16, 8, cpu); \
    SETUP_BLOCKCOPY_PS(16, 12, cpu); \
    SETUP_BLOCKCOPY_PS(16, 16, cpu); \
    SETUP_BLOCKCOPY_PS(16, 32, cpu); \
    SETUP_BLOCKCOPY_PS(24, 32, cpu); \
    SETUP_BLOCKCOPY_PS(32,  8, cpu); \
    SETUP_BLOCKCOPY_PS(32, 16, cpu); \
    SETUP_BLOCKCOPY_PS(32, 24, cpu); \
    SETUP_BLOCKCOPY_PS(32, 32, cpu); \
    SETUP_BLOCKCOPY_PS(16, 64, cpu); \
    SETUP_BLOCKCOPY_PS(32, 64, cpu); \
    SETUP_BLOCKCOPY_PS(48, 64, cpu); \
    SETUP_BLOCKCOPY_PS(64, 16, cpu); \
    SETUP_BLOCKCOPY_PS(64, 32, cpu); \
    SETUP_BLOCKCOPY_PS(64, 48, cpu); \
    SETUP_BLOCKCOPY_PS(64, 64, cpu); \
    \
    SETUP_BLOCKCOPY_PS(2, 16, cpu); \
    SETUP_BLOCKCOPY_PS(4, 32, cpu); \
    SETUP_BLOCKCOPY_PS(6, 16, cpu); \
    SETUP_BLOCKCOPY_PS(8, 12, cpu); \
    SETUP_BLOCKCOPY_PS(8, 64, cpu); \
    SETUP_BLOCKCOPY_PS(12, 32, cpu); \
    SETUP_BLOCKCOPY_PS(16, 24, cpu); \
    SETUP_BLOCKCOPY_PS(24, 64, cpu); \
    SETUP_BLOCKCOPY_PS(32, 48, cpu);

BLOCKCOPY_COMMON(_sse2);
BLOCKCOPY_SS_PP(_sse2);
BLOCKCOPY_SP(_sse4);
BLOCKCOPY_PS(_sse4);

BLOCKCOPY_SP(_sse2);

void x265_blockfill_s_4x4_sse2(int16_t *dst, intptr_t dstride, int16_t val);
void x265_blockfill_s_8x8_sse2(int16_t *dst, intptr_t dstride, int16_t val);
void x265_blockfill_s_16x16_sse2(int16_t *dst, intptr_t dstride, int16_t val);
void x265_blockfill_s_32x32_sse2(int16_t *dst, intptr_t dstride, int16_t val);

#undef BLOCKCOPY_COMMON
#undef BLOCKCOPY_SS_PP
#undef BLOCKCOPY_SP
#undef BLOCKCOPY_PS
#undef SETUP_BLOCKCOPY_PS
#undef SETUP_BLOCKCOPY_SP
#undef SETUP_BLOCKCOPY_SS_PP
#undef SETUP_BLOCKCOPY_FUNC

#endif // ifndef X265_I386_PIXEL_H
