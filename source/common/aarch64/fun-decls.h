/*****************************************************************************
 * Copyright (C) 2021 MulticoreWare, Inc
 *
 * Authors: Sebastian Pop <spop@amazon.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111, USA.
 *
 * This program is also available under a commercial proprietary license.
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#define FUNCDEF_TU(ret, name, cpu, ...) \
    ret PFX(name ## _4x4_ ## cpu(__VA_ARGS__)); \
    ret PFX(name ## _8x8_ ## cpu(__VA_ARGS__)); \
    ret PFX(name ## _16x16_ ## cpu(__VA_ARGS__)); \
    ret PFX(name ## _32x32_ ## cpu(__VA_ARGS__)); \
    ret PFX(name ## _64x64_ ## cpu(__VA_ARGS__))

#define FUNCDEF_TU_S(ret, name, cpu, ...) \
    ret PFX(name ## _4_ ## cpu(__VA_ARGS__)); \
    ret PFX(name ## _8_ ## cpu(__VA_ARGS__)); \
    ret PFX(name ## _16_ ## cpu(__VA_ARGS__)); \
    ret PFX(name ## _32_ ## cpu(__VA_ARGS__)); \
    ret PFX(name ## _64_ ## cpu(__VA_ARGS__))

#define FUNCDEF_TU_S2(ret, name, cpu, ...) \
    ret PFX(name ## 4_ ## cpu(__VA_ARGS__)); \
    ret PFX(name ## 8_ ## cpu(__VA_ARGS__)); \
    ret PFX(name ## 16_ ## cpu(__VA_ARGS__)); \
    ret PFX(name ## 32_ ## cpu(__VA_ARGS__)); \
    ret PFX(name ## 64_ ## cpu(__VA_ARGS__))

#define FUNCDEF_PU(ret, name, cpu, ...) \
    ret PFX(name ## _4x4_   ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _8x8_   ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _16x16_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _32x32_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _64x64_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _8x4_   ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _4x8_   ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _16x8_  ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _8x16_  ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _16x32_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _32x16_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _64x32_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _32x64_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _16x12_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _12x16_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _16x4_  ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _4x16_  ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _32x24_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _24x32_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _32x8_  ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _8x32_  ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _64x48_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _48x64_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _64x16_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _16x64_ ## cpu)(__VA_ARGS__)

#define FUNCDEF_CHROMA_PU(ret, name, cpu, ...) \
    FUNCDEF_PU(ret, name, cpu, __VA_ARGS__); \
    ret PFX(name ## _4x2_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _4x4_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _2x4_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _8x2_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _2x8_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _8x6_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _6x8_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _8x12_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _12x8_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _6x16_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _16x6_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _2x16_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _16x2_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _4x12_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _12x4_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _32x12_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _12x32_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _32x4_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _4x32_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _32x48_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _48x32_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _16x24_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _24x16_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _8x64_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _64x8_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _64x24_ ## cpu)(__VA_ARGS__); \
    ret PFX(name ## _24x64_ ## cpu)(__VA_ARGS__);

#define DECLS(cpu) \
    FUNCDEF_TU(void, cpy2Dto1D_shl, cpu, int16_t* dst, const int16_t* src, intptr_t srcStride, int shift); \
    FUNCDEF_TU(void, cpy2Dto1D_shr, cpu, int16_t* dst, const int16_t* src, intptr_t srcStride, int shift); \
    FUNCDEF_TU(void, cpy1Dto2D_shl, cpu, int16_t* dst, const int16_t* src, intptr_t srcStride, int shift); \
    FUNCDEF_TU(void, cpy1Dto2D_shl_aligned, cpu, int16_t* dst, const int16_t* src, intptr_t srcStride, int shift); \
    FUNCDEF_TU(void, cpy1Dto2D_shr, cpu, int16_t* dst, const int16_t* src, intptr_t srcStride, int shift); \
    FUNCDEF_TU_S(uint32_t, copy_cnt, cpu, int16_t* dst, const int16_t* src, intptr_t srcStride); \
    FUNCDEF_TU_S(int, count_nonzero, cpu, const int16_t* quantCoeff); \
    FUNCDEF_TU(void, blockfill_s, cpu, int16_t* dst, intptr_t dstride, int16_t val); \
    FUNCDEF_TU(void, blockfill_s_aligned, cpu, int16_t* dst, intptr_t dstride, int16_t val); \
    FUNCDEF_CHROMA_PU(void, blockcopy_ss, cpu, int16_t* dst, intptr_t dstStride, const int16_t* src, intptr_t srcStride); \
    FUNCDEF_CHROMA_PU(void, blockcopy_pp, cpu, pixel* dst, intptr_t dstStride, const pixel* src, intptr_t srcStride); \
    FUNCDEF_PU(void, blockcopy_sp, cpu, pixel* dst, intptr_t dstStride, const int16_t* src, intptr_t srcStride); \
    FUNCDEF_PU(void, blockcopy_ps, cpu, int16_t* dst, intptr_t dstStride, const pixel* src, intptr_t srcStride); \
    FUNCDEF_PU(void, interp_8tap_horiz_pp, cpu, const pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int coeffIdx); \
    FUNCDEF_PU(void, interp_8tap_horiz_ps, cpu, const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt); \
    FUNCDEF_PU(void, interp_8tap_vert_pp, cpu, const pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int coeffIdx); \
    FUNCDEF_PU(void, interp_8tap_vert_ps, cpu, const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx); \
    FUNCDEF_PU(void, interp_8tap_vert_sp, cpu, const int16_t* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int coeffIdx); \
    FUNCDEF_PU(void, interp_8tap_vert_ss, cpu, const int16_t* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx); \
    FUNCDEF_PU(void, interp_8tap_hv_pp, cpu, const pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int idxX, int idxY); \
    FUNCDEF_CHROMA_PU(void, filterPixelToShort, cpu, const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride); \
    FUNCDEF_CHROMA_PU(void, filterPixelToShort_aligned, cpu, const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride); \
    FUNCDEF_CHROMA_PU(void, interp_horiz_pp, cpu, const pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int coeffIdx); \
    FUNCDEF_CHROMA_PU(void, interp_4tap_horiz_pp, cpu, const pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int coeffIdx); \
    FUNCDEF_CHROMA_PU(void, interp_horiz_ps, cpu, const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt); \
    FUNCDEF_CHROMA_PU(void, interp_4tap_horiz_ps, cpu, const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt); \
    FUNCDEF_CHROMA_PU(void, interp_4tap_vert_pp, cpu, const pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int coeffIdx); \
    FUNCDEF_CHROMA_PU(void, interp_4tap_vert_ps, cpu, const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx); \
    FUNCDEF_CHROMA_PU(void, interp_4tap_vert_sp, cpu, const int16_t* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int coeffIdx); \
    FUNCDEF_CHROMA_PU(void, interp_4tap_vert_ss, cpu, const int16_t* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx); \
    FUNCDEF_CHROMA_PU(void, addAvg, cpu, const int16_t*, const int16_t*, pixel*, intptr_t, intptr_t, intptr_t); \
    FUNCDEF_CHROMA_PU(void, addAvg_aligned, cpu, const int16_t*, const int16_t*, pixel*, intptr_t, intptr_t, intptr_t); \
    FUNCDEF_PU(void, pixel_avg_pp, cpu, pixel* dst, intptr_t dstride, const pixel* src0, intptr_t sstride0, const pixel* src1, intptr_t sstride1, int); \
    FUNCDEF_PU(void, pixel_avg_pp_aligned, cpu, pixel* dst, intptr_t dstride, const pixel* src0, intptr_t sstride0, const pixel* src1, intptr_t sstride1, int); \
    FUNCDEF_PU(void, sad_x3, cpu, const pixel*, const pixel*, const pixel*, const pixel*, intptr_t, int32_t*); \
    FUNCDEF_PU(void, sad_x4, cpu, const pixel*, const pixel*, const pixel*, const pixel*, const pixel*, intptr_t, int32_t*); \
    FUNCDEF_CHROMA_PU(int, pixel_sad, cpu, const pixel*, intptr_t, const pixel*, intptr_t); \
    FUNCDEF_CHROMA_PU(sse_t, pixel_ssd_s, cpu, const int16_t*, intptr_t); \
    FUNCDEF_CHROMA_PU(sse_t, pixel_ssd_s_aligned, cpu, const int16_t*, intptr_t); \
    FUNCDEF_TU_S(sse_t, pixel_ssd_s, cpu, const int16_t*, intptr_t); \
    FUNCDEF_TU_S(sse_t, pixel_ssd_s_aligned, cpu, const int16_t*, intptr_t); \
    FUNCDEF_PU(sse_t, pixel_sse_pp, cpu, const pixel*, intptr_t, const pixel*, intptr_t); \
    FUNCDEF_CHROMA_PU(sse_t, pixel_sse_ss, cpu, const int16_t*, intptr_t, const int16_t*, intptr_t); \
    FUNCDEF_PU(void, pixel_sub_ps, cpu, int16_t* a, intptr_t dstride, const pixel* b0, const pixel* b1, intptr_t sstride0, intptr_t sstride1); \
    FUNCDEF_PU(void, pixel_add_ps, cpu, pixel* a, intptr_t dstride, const pixel* b0, const int16_t* b1, intptr_t sstride0, intptr_t sstride1); \
    FUNCDEF_PU(void, pixel_add_ps_aligned, cpu, pixel* a, intptr_t dstride, const pixel* b0, const int16_t* b1, intptr_t sstride0, intptr_t sstride1); \
    FUNCDEF_CHROMA_PU(int, pixel_satd, cpu, const pixel*, intptr_t, const pixel*, intptr_t); \
    FUNCDEF_TU_S2(void, ssimDist, cpu, const pixel *fenc, uint32_t fStride, const pixel *recon, intptr_t rstride, uint64_t *ssBlock, int shift, uint64_t *ac_k); \
    FUNCDEF_TU_S2(void, normFact, cpu, const pixel *src, uint32_t blockSize, int shift, uint64_t *z_k)

DECLS(neon);

void x265_pixel_planecopy_cp_neon(const uint8_t* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int width, int height, int shift);

uint64_t x265_pixel_var_8x8_neon(const pixel* pix, intptr_t stride);
uint64_t x265_pixel_var_16x16_neon(const pixel* pix, intptr_t stride);
uint64_t x265_pixel_var_32x32_neon(const pixel* pix, intptr_t stride);
uint64_t x265_pixel_var_64x64_neon(const pixel* pix, intptr_t stride);

void x265_getResidual4_neon(const pixel* fenc, const pixel* pred, int16_t* residual, intptr_t stride);
void x265_getResidual8_neon(const pixel* fenc, const pixel* pred, int16_t* residual, intptr_t stride);
void x265_getResidual16_neon(const pixel* fenc, const pixel* pred, int16_t* residual, intptr_t stride);
void x265_getResidual32_neon(const pixel* fenc, const pixel* pred, int16_t* residual, intptr_t stride);

void x265_scale1D_128to64_neon(pixel *dst, const pixel *src);
void x265_scale2D_64to32_neon(pixel* dst, const pixel* src, intptr_t stride);

int x265_pixel_satd_4x4_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_4x8_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_4x16_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_4x32_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_8x4_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_8x8_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_8x12_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_8x16_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_8x32_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_8x64_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_12x16_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_12x32_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_16x4_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_16x8_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_16x12_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_16x16_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_16x24_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_16x32_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_16x64_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_24x32_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_24x64_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_32x8_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_32x16_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_32x24_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_32x32_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_32x48_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_32x64_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_48x64_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_64x16_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_64x32_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_64x48_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_64x64_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);

int x265_pixel_sa8d_8x8_neon(const pixel* pix1, intptr_t i_pix1, const pixel* pix2, intptr_t i_pix2);
int x265_pixel_sa8d_8x16_neon(const pixel* pix1, intptr_t i_pix1, const pixel* pix2, intptr_t i_pix2);
int x265_pixel_sa8d_16x16_neon(const pixel* pix1, intptr_t i_pix1, const pixel* pix2, intptr_t i_pix2);
int x265_pixel_sa8d_16x32_neon(const pixel* pix1, intptr_t i_pix1, const pixel* pix2, intptr_t i_pix2);
int x265_pixel_sa8d_32x32_neon(const pixel* pix1, intptr_t i_pix1, const pixel* pix2, intptr_t i_pix2);
int x265_pixel_sa8d_32x64_neon(const pixel* pix1, intptr_t i_pix1, const pixel* pix2, intptr_t i_pix2);
int x265_pixel_sa8d_64x64_neon(const pixel* pix1, intptr_t i_pix1, const pixel* pix2, intptr_t i_pix2);

uint32_t PFX(quant_neon)(const int16_t* coef, const int32_t* quantCoeff, int32_t* deltaU, int16_t* qCoef, int qBits, int add, int numCoeff);
uint32_t PFX(nquant_neon)(const int16_t* coef, const int32_t* quantCoeff, int16_t* qCoef, int qBits, int add, int numCoeff);

void x265_dequant_scaling_neon(const int16_t* quantCoef, const int32_t* deQuantCoef, int16_t* coef, int num, int per, int shift);
void x265_dequant_normal_neon(const int16_t* quantCoef, int16_t* coef, int num, int scale, int shift);

void x265_ssim_4x4x2_core_neon(const pixel* pix1, intptr_t stride1, const pixel* pix2, intptr_t stride2, int sums[2][4]);

int PFX(psyCost_4x4_neon)(const pixel* source, intptr_t sstride, const pixel* recon, intptr_t rstride);
int PFX(psyCost_8x8_neon)(const pixel* source, intptr_t sstride, const pixel* recon, intptr_t rstride);
void PFX(weight_pp_neon)(const pixel* src, pixel* dst, intptr_t stride, int width, int height, int w0, int round, int shift, int offset);
void PFX(weight_sp_neon)(const int16_t* src, pixel* dst, intptr_t srcStride, intptr_t dstStride, int width, int height, int w0, int round, int shift, int offset);
int PFX(scanPosLast_neon)(const uint16_t *scan, const coeff_t *coeff, uint16_t *coeffSign, uint16_t *coeffFlag, uint8_t *coeffNum, int numSig, const uint16_t* scanCG4x4, const int trSize);
uint32_t PFX(costCoeffNxN_neon)(const uint16_t *scan, const coeff_t *coeff, intptr_t trSize, uint16_t *absCoeff, const uint8_t *tabSigCtx, uint32_t scanFlagMask, uint8_t *baseCtx, int offset, int scanPosSigOff, int subPosBase);
