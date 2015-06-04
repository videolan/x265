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

#ifndef X265_PIXEL_UTIL_H
#define X265_PIXEL_UTIL_H

void x265_getResidual4_sse2(const pixel* fenc, const pixel* pred, int16_t* residual, intptr_t stride);
void x265_getResidual8_sse2(const pixel* fenc, const pixel* pred, int16_t* residual, intptr_t stride);
void x265_getResidual16_sse2(const pixel* fenc, const pixel* pred, int16_t* residual, intptr_t stride);
void x265_getResidual16_sse4(const pixel* fenc, const pixel* pred, int16_t* residual, intptr_t stride);
void x265_getResidual32_sse2(const pixel* fenc, const pixel* pred, int16_t* residual, intptr_t stride);
void x265_getResidual32_sse4(const pixel* fenc, const pixel* pred, int16_t* residual, intptr_t stride);
void x265_getResidual16_avx2(const pixel* fenc, const pixel* pred, int16_t* residual, intptr_t stride);
void x265_getResidual32_avx2(const pixel* fenc, const pixel* pred, int16_t* residual, intptr_t stride);

void x265_transpose4_sse2(pixel* dest, const pixel* src, intptr_t stride);
void x265_transpose8_sse2(pixel* dest, const pixel* src, intptr_t stride);
void x265_transpose16_sse2(pixel* dest, const pixel* src, intptr_t stride);
void x265_transpose32_sse2(pixel* dest, const pixel* src, intptr_t stride);
void x265_transpose64_sse2(pixel* dest, const pixel* src, intptr_t stride);

void x265_transpose8_avx2(pixel* dest, const pixel* src, intptr_t stride);
void x265_transpose16_avx2(pixel* dest, const pixel* src, intptr_t stride);
void x265_transpose32_avx2(pixel* dest, const pixel* src, intptr_t stride);
void x265_transpose64_avx2(pixel* dest, const pixel* src, intptr_t stride);

uint32_t x265_quant_sse4(const int16_t* coef, const int32_t* quantCoeff, int32_t* deltaU, int16_t* qCoef, int qBits, int add, int numCoeff);
uint32_t x265_quant_avx2(const int16_t* coef, const int32_t* quantCoeff, int32_t* deltaU, int16_t* qCoef, int qBits, int add, int numCoeff);
uint32_t x265_nquant_sse4(const int16_t* coef, const int32_t* quantCoeff, int16_t* qCoef, int qBits, int add, int numCoeff);
uint32_t x265_nquant_avx2(const int16_t* coef, const int32_t* quantCoeff, int16_t* qCoef, int qBits, int add, int numCoeff);
void x265_dequant_normal_sse4(const int16_t* quantCoef, int16_t* coef, int num, int scale, int shift);
void x265_dequant_normal_avx2(const int16_t* quantCoef, int16_t* coef, int num, int scale, int shift);

int x265_count_nonzero_4x4_ssse3(const int16_t* quantCoeff);
int x265_count_nonzero_8x8_ssse3(const int16_t* quantCoeff);
int x265_count_nonzero_16x16_ssse3(const int16_t* quantCoeff);
int x265_count_nonzero_32x32_ssse3(const int16_t* quantCoeff);
int x265_count_nonzero_4x4_avx2(const int16_t* quantCoeff);
int x265_count_nonzero_8x8_avx2(const int16_t* quantCoeff);
int x265_count_nonzero_16x16_avx2(const int16_t* quantCoeff);
int x265_count_nonzero_32x32_avx2(const int16_t* quantCoeff);

void x265_weight_pp_sse4(const pixel* src, pixel* dst, intptr_t stride, int width, int height, int w0, int round, int shift, int offset);
void x265_weight_pp_avx2(const pixel* src, pixel* dst, intptr_t stride, int width, int height, int w0, int round, int shift, int offset);
void x265_weight_sp_sse4(const int16_t* src, pixel* dst, intptr_t srcStride, intptr_t dstStride, int width, int height, int w0, int round, int shift, int offset);

void x265_pixel_ssim_4x4x2_core_mmx2(const uint8_t* pix1, intptr_t stride1,
                                     const uint8_t* pix2, intptr_t stride2, int sums[2][4]);
void x265_pixel_ssim_4x4x2_core_sse2(const pixel* pix1, intptr_t stride1,
                                     const pixel* pix2, intptr_t stride2, int sums[2][4]);
void x265_pixel_ssim_4x4x2_core_avx(const pixel* pix1, intptr_t stride1,
                                    const pixel* pix2, intptr_t stride2, int sums[2][4]);
float x265_pixel_ssim_end4_sse2(int sum0[5][4], int sum1[5][4], int width);
float x265_pixel_ssim_end4_avx(int sum0[5][4], int sum1[5][4], int width);

void x265_scale1D_128to64_ssse3(pixel*, const pixel*);
void x265_scale1D_128to64_avx2(pixel*, const pixel*);
void x265_scale2D_64to32_ssse3(pixel*, const pixel*, intptr_t);
void x265_scale2D_64to32_avx2(pixel*, const pixel*, intptr_t);

int x265_scanPosLast_x64(const uint16_t *scan, const coeff_t *coeff, uint16_t *coeffSign, uint16_t *coeffFlag, uint8_t *coeffNum, int numSig, const uint16_t* scanCG4x4, const int trSize);
int x265_scanPosLast_avx2_bmi2(const uint16_t *scan, const coeff_t *coeff, uint16_t *coeffSign, uint16_t *coeffFlag, uint8_t *coeffNum, int numSig, const uint16_t* scanCG4x4, const int trSize);
uint32_t x265_findPosFirstLast_ssse3(const int16_t *dstCoeff, const intptr_t trSize, const uint16_t scanTbl[16]);

uint32_t x265_costCoeffNxN_sse4(const uint16_t *scan, const coeff_t *coeff, intptr_t trSize, uint16_t *absCoeff, const uint8_t *tabSigCtx, uint32_t scanFlagMask, uint8_t *baseCtx, int offset, int scanPosSigOff, int subPosBase);


#define SETUP_CHROMA_PIXELSUB_PS_FUNC(W, H, cpu) \
    void x265_pixel_sub_ps_ ## W ## x ## H ## cpu(int16_t*  dest, intptr_t destride, const pixel* src0, const pixel* src1, intptr_t srcstride0, intptr_t srcstride1); \
    void x265_pixel_add_ps_ ## W ## x ## H ## cpu(pixel* dest, intptr_t destride, const pixel* src0, const int16_t*  src1, intptr_t srcStride0, intptr_t srcStride1);

#define CHROMA_420_PIXELSUB_DEF(cpu) \
    SETUP_CHROMA_PIXELSUB_PS_FUNC(4, 4, cpu); \
    SETUP_CHROMA_PIXELSUB_PS_FUNC(8, 8, cpu); \
    SETUP_CHROMA_PIXELSUB_PS_FUNC(16, 16, cpu); \
    SETUP_CHROMA_PIXELSUB_PS_FUNC(32, 32, cpu);

#define CHROMA_422_PIXELSUB_DEF(cpu) \
    SETUP_CHROMA_PIXELSUB_PS_FUNC(4, 8, cpu); \
    SETUP_CHROMA_PIXELSUB_PS_FUNC(8, 16, cpu); \
    SETUP_CHROMA_PIXELSUB_PS_FUNC(16, 32, cpu); \
    SETUP_CHROMA_PIXELSUB_PS_FUNC(32, 64, cpu);

#define SETUP_LUMA_PIXELSUB_PS_FUNC(W, H, cpu) \
    void x265_pixel_sub_ps_ ## W ## x ## H ## cpu(int16_t*  dest, intptr_t destride, const pixel* src0, const pixel* src1, intptr_t srcstride0, intptr_t srcstride1); \
    void x265_pixel_add_ps_ ## W ## x ## H ## cpu(pixel* dest, intptr_t destride, const pixel* src0, const int16_t*  src1, intptr_t srcStride0, intptr_t srcStride1);

#define LUMA_PIXELSUB_DEF(cpu) \
    SETUP_LUMA_PIXELSUB_PS_FUNC(8,   8, cpu); \
    SETUP_LUMA_PIXELSUB_PS_FUNC(16, 16, cpu); \
    SETUP_LUMA_PIXELSUB_PS_FUNC(32, 32, cpu); \
    SETUP_LUMA_PIXELSUB_PS_FUNC(64, 64, cpu);

LUMA_PIXELSUB_DEF(_sse2);
CHROMA_420_PIXELSUB_DEF(_sse2);
CHROMA_422_PIXELSUB_DEF(_sse2);

LUMA_PIXELSUB_DEF(_sse4);
CHROMA_420_PIXELSUB_DEF(_sse4);
CHROMA_422_PIXELSUB_DEF(_sse4);

#define SETUP_LUMA_PIXELVAR_FUNC(W, H, cpu) \
    uint64_t x265_pixel_var_ ## W ## x ## H ## cpu(const pixel* pix, intptr_t pixstride);

#define LUMA_PIXELVAR_DEF(cpu) \
    SETUP_LUMA_PIXELVAR_FUNC(8,   8, cpu); \
    SETUP_LUMA_PIXELVAR_FUNC(16, 16, cpu); \
    SETUP_LUMA_PIXELVAR_FUNC(32, 32, cpu); \
    SETUP_LUMA_PIXELVAR_FUNC(64, 64, cpu);

LUMA_PIXELVAR_DEF(_sse2);
LUMA_PIXELVAR_DEF(_xop);
LUMA_PIXELVAR_DEF(_avx);

#undef CHROMA_420_PIXELSUB_DEF
#undef CHROMA_422_PIXELSUB_DEF
#undef LUMA_PIXELSUB_DEF
#undef LUMA_PIXELVAR_DEF
#undef SETUP_CHROMA_PIXELSUB_PS_FUNC
#undef SETUP_LUMA_PIXELSUB_PS_FUNC
#undef SETUP_LUMA_PIXELVAR_FUNC

#endif // ifndef X265_PIXEL_UTIL_H
