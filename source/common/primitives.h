/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Mandar Gurav <mandar@multicorewareinc.com>
 *          Deepthi Devaki Akkoorath <deepthidevaki@multicorewareinc.com>
 *          Mahesh Pittala <mahesh@multicorewareinc.com>
 *          Rajesh Paulraj <rajesh@multicorewareinc.com>
 *          Praveen Kumar Tiwari <praveen@multicorewareinc.com>
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

#ifndef X265_PRIMITIVES_H
#define X265_PRIMITIVES_H

#include <stdint.h>
#include <assert.h>
#include "cpu.h"

#define FENC_STRIDE 64

#if defined(__GNUC__)
#define ALIGN_VAR_8(T, var)  T var __attribute__((aligned(8)))
#define ALIGN_VAR_16(T, var) T var __attribute__((aligned(16)))
#define ALIGN_VAR_32(T, var) T var __attribute__((aligned(32)))
#elif defined(_MSC_VER)
#define ALIGN_VAR_8(T, var)  __declspec(align(8)) T var
#define ALIGN_VAR_16(T, var) __declspec(align(16)) T var
#define ALIGN_VAR_32(T, var) __declspec(align(32)) T var
#endif // if defined(__GNUC__)

#if HIGH_BIT_DEPTH
typedef uint16_t pixel;
typedef uint32_t sum_t;
typedef uint64_t sum2_t;
typedef uint64_t pixel4;
typedef int64_t ssim_t;
#else
typedef uint8_t pixel;
typedef uint16_t sum_t;
typedef uint32_t sum2_t;
typedef uint32_t pixel4;
typedef int32_t ssim_t;
#endif // if HIGH_BIT_DEPTH

namespace x265 {
// x265 private namespace

enum LumaPartitions
{ // Square     Rectangular             Asymmetrical (0.75, 0.25)
    LUMA_4x4,
    LUMA_8x8,   LUMA_8x4,   LUMA_4x8,
    LUMA_16x16, LUMA_16x8,  LUMA_8x16,  LUMA_16x12, LUMA_12x16, LUMA_16x4,  LUMA_4x16,
    LUMA_32x32, LUMA_32x16, LUMA_16x32, LUMA_32x24, LUMA_24x32, LUMA_32x8,  LUMA_8x32,
    LUMA_64x64, LUMA_64x32, LUMA_32x64, LUMA_64x48, LUMA_48x64, LUMA_64x16, LUMA_16x64,
    NUM_LUMA_PARTITIONS
};

// 4:2:0 chroma partition sizes
enum ChromaPartions
{
    CHROMA_2x2, // never used by HEVC
    CHROMA_4x4,   CHROMA_4x2,   CHROMA_2x4,
    CHROMA_8x8,   CHROMA_8x4,   CHROMA_4x8,   CHROMA_8x6,   CHROMA_6x8,   CHROMA_8x2,  CHROMA_2x8,
    CHROMA_16x16, CHROMA_16x8,  CHROMA_8x16,  CHROMA_16x12, CHROMA_12x16, CHROMA_16x4, CHROMA_4x16,
    CHROMA_32x32, CHROMA_32x16, CHROMA_16x32, CHROMA_32x24, CHROMA_24x32, CHROMA_32x8, CHROMA_8x32,
    NUM_CHROMA_PARTITIONS
};

enum SquareBlocks   // Routines can be indexed using log2n(width)
{
    BLOCK_4x4,
    BLOCK_8x8,
    BLOCK_16x16,
    BLOCK_32x32,
    BLOCK_64x64,
    NUM_SQUARE_BLOCKS
};

// NOTE: Not all DCT functions support dest stride
enum Dcts
{
    DST_4x4,
    DCT_4x4,
    DCT_8x8,
    DCT_16x16,
    DCT_32x32,
    NUM_DCTS
};

enum IDcts
{
    IDST_4x4,
    IDCT_4x4,
    IDCT_8x8,
    IDCT_16x16,
    IDCT_32x32,
    NUM_IDCTS
};

enum IPFilterConf_P_S
{
    FILTER_H_P_S_8,
    FILTER_H_P_S_4,
    FILTER_V_P_S_8,
    FILTER_V_P_S_4,
    NUM_IPFILTER_P_S
};

enum IPFilterConf_S_P
{
    FILTER_V_S_P_8,
    FILTER_V_S_P_4,
    NUM_IPFILTER_S_P
};

enum IPFilterConf_S_S
{
    FILTER_V_S_S_8,
    FILTER_V_S_S_4,
    NUM_IPFILTER_S_S
};

// Returns a LumaPartitions enum for the given size, always expected to return a valid enum
inline int partitionFromSizes(int width, int height)
{
    assert(((width | height) & ~(4 | 8 | 16 | 32 | 64)) == 0);
    extern uint8_t lumaPartitioneMapTable[];
    int w = (width >> 2) - 1;
    int h = (height >> 2) - 1;
    int part = (int)lumaPartitioneMapTable[(w << 4) + h];
    assert(part != 255);
    return part;
}

typedef int  (*pixelcmp_t)(pixel *fenc, intptr_t fencstride, pixel *fref, intptr_t frefstride); // fenc is aligned
typedef int  (*pixelcmp_ss_t)(int16_t *fenc, intptr_t fencstride, int16_t *fref, intptr_t frefstride);
typedef int  (*pixelcmp_sp_t)(int16_t *fenc, intptr_t fencstride, pixel *fref, intptr_t frefstride);
typedef void (*pixelcmp_x4_t)(pixel *fenc, pixel *fref0, pixel *fref1, pixel *fref2, pixel *fref3, intptr_t frefstride, int32_t *res);
typedef void (*pixelcmp_x3_t)(pixel *fenc, pixel *fref0, pixel *fref1, pixel *fref2, intptr_t frefstride, int32_t *res);
typedef void (*ipfilter_ps_t)(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int width, int height, const int16_t *coeff);
typedef void (*ipfilter_sp_t)(int16_t *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int width, int height, const int coeffIdx);
typedef void (*ipfilter_ss_t)(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int width, int height, const int coeffIdx);
typedef void (*ipfilter_p2s_t)(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int width, int height);
typedef void (*ipfilter_s2p_t)(int16_t *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int width, int height);
typedef void (*blockcpy_pp_t)(int bx, int by, pixel *dst, intptr_t dstride, pixel *src, intptr_t sstride); // dst is aligned
typedef void (*blockcpy_sp_t)(int bx, int by, int16_t *dst, intptr_t dstride, pixel *src, intptr_t sstride); // dst is aligned
typedef void (*blockcpy_ps_t)(int bx, int by, pixel *dst, intptr_t dstride, int16_t *src, intptr_t sstride); // dst is aligned
typedef void (*blockcpy_sc_t)(int bx, int by, int16_t *dst, intptr_t dstride, uint8_t *src, intptr_t sstride); // dst is aligned
typedef void (*pixelsub_ps_t)(int bx, int by, int16_t *dst, intptr_t dstride, pixel *src0, pixel *src1, intptr_t sstride0, intptr_t sstride1);
typedef void (*pixeladd_ss_t)(int bx, int by, int16_t *dst, intptr_t dstride, int16_t *src0, int16_t *src1, intptr_t sstride0, intptr_t sstride1);
typedef void (*pixeladd_pp_t)(int bx, int by, pixel *dst, intptr_t dstride, pixel *src0, pixel *src1, intptr_t sstride0, intptr_t sstride1);
typedef void (*pixelavg_pp_t)(pixel *dst, intptr_t dstride, pixel *src0, intptr_t sstride0, pixel *src1, intptr_t sstride1, int weight);
typedef void (*blockfill_s_t)(int16_t *dst, intptr_t dstride, int16_t val);

typedef void (*intra_dc_t)(pixel* above, pixel* left, pixel* dst, intptr_t dstStride, int width, int bFilter);
typedef void (*intra_planar_t)(pixel* above, pixel* left, pixel* dst, intptr_t dstStride, int width);
typedef void (*intra_ang_t)(pixel* dst, int dstStride, int width, int dirMode, bool bFilter, pixel *refLeft, pixel *refAbove);
typedef void (*intra_allangs_t)(pixel *dst, pixel *above0, pixel *left0, pixel *above1, pixel *left1, bool bLuma);

typedef void (*cvt16to32_shl_t)(int32_t *dst, int16_t *src, intptr_t, int, int);
typedef void (*cvt16to16_shl_t)(int16_t *dst, int16_t *src, int, int, intptr_t, int);
typedef void (*cvt32to16_shr_t)(int16_t *dst, int32_t *src, intptr_t, int, int);

typedef void (*dct_t)(int16_t *src, int32_t *dst, intptr_t stride);
typedef void (*idct_t)(int32_t *src, int16_t *dst, intptr_t stride);
typedef void (*calcresidual_t)(pixel *fenc, pixel *pred, int16_t *residual, int stride);
typedef void (*calcrecon_t)(pixel* pred, int16_t* residual, pixel* recon, int16_t* reconqt, pixel *reconipred, int stride, int strideqt, int strideipred);
typedef void (*transpose_t)(pixel* dst, pixel* src, intptr_t stride);
typedef uint32_t (*quant_t)(int32_t *coef, int32_t *quantCoeff, int32_t *deltaU, int32_t *qCoef, int qBits, int add, int numCoeff, int32_t* lastPos);
typedef void (*dequant_t)(const int32_t* src, int32_t* dst, int width, int height, int mcqp_miper, int mcqp_mirem, bool useScalingList,
                          unsigned int trSizeLog2, int32_t *dequantCoef);

typedef void (*weightpUniPixel_t)(pixel *src, pixel *dst, intptr_t srcStride, intptr_t dstStride, int width, int height, int w0, int round, int shift, int offset);
typedef void (*weightpUni_t)(int16_t *src, pixel *dst, intptr_t srcStride, intptr_t dstStride, int width, int height, int w0, int round, int shift, int offset);
typedef void (*scale_t)(pixel *dst, pixel *src, intptr_t stride);
typedef void (*downscale_t)(pixel *src0, pixel *dstf, pixel *dsth, pixel *dstv, pixel *dstc,
                            intptr_t src_stride, intptr_t dst_stride, int width, int height);
typedef void (*extendCURowBorder_t)(pixel* txt, intptr_t stride, int width, int height, int marginX);
typedef void (*ssim_4x4x2_core_t)(const pixel *pix1, intptr_t stride1, const pixel *pix2, intptr_t stride2, ssim_t sums[2][4]);
typedef float (*ssim_end4_t)(ssim_t sum0[5][4], ssim_t sum1[5][4], int width);
typedef uint64_t (*var_t)(pixel *pix, intptr_t stride);
typedef void (*plane_copy_deinterleave_t)(pixel *dstu, intptr_t dstuStride, pixel *dstv, intptr_t dstvStride, pixel *src,  intptr_t srcStride, int w, int h);

typedef void (*filter_pp_t) (pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx);
typedef void (*filter_ps_t) (pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx);
typedef void (*filter_hv_pp_t) (pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int idxX, int idxY);
typedef void (*filter_p2s_t)(pixel *src, intptr_t srcStride, int16_t *dst, int width, int height);

typedef void (*copy_pp_t)(pixel *dst, intptr_t dstride, pixel *src, intptr_t sstride); // dst is aligned
typedef void (*copy_sp_t)(pixel *dst, intptr_t dstStride, int16_t *src, intptr_t srcStride);

/* Define a structure containing function pointers to optimized encoder
 * primitives.  Each pointer can reference either an assembly routine,
 * a vectorized primitive, or a C function. */
struct EncoderPrimitives
{
    pixelcmp_t      sad[NUM_LUMA_PARTITIONS];        // Sum of Differences for each size
    pixelcmp_x3_t   sad_x3[NUM_LUMA_PARTITIONS];     // Sum of Differences 3x for each size
    pixelcmp_x4_t   sad_x4[NUM_LUMA_PARTITIONS];     // Sum of Differences 4x for each size
    pixelcmp_t      sse_pp[NUM_LUMA_PARTITIONS];     // Sum of Square Error (pixel, pixel) fenc alignment not assumed
    pixelcmp_ss_t   sse_ss[NUM_LUMA_PARTITIONS];     // Sum of Square Error (short, short) fenc alignment not assumed
    pixelcmp_sp_t   sse_sp[NUM_LUMA_PARTITIONS];     // Sum of Square Error (short, pixel) fenc alignment not assumed
    pixelcmp_t      satd[NUM_LUMA_PARTITIONS];       // Sum of Transformed differences (HADAMARD)
    pixelcmp_t      sa8d_inter[NUM_LUMA_PARTITIONS]; // sa8d primitives for motion search partitions
    pixelcmp_t      sa8d[NUM_SQUARE_BLOCKS];         // sa8d primitives for square intra blocks

    blockcpy_pp_t   blockcpy_pp;                // block copy pixel from pixel
    blockcpy_ps_t   blockcpy_ps;                // block copy pixel from short
    blockcpy_sp_t   blockcpy_sp;                // block copy short from pixel
    blockfill_s_t   blockfill_s[NUM_SQUARE_BLOCKS];  // block fill with value
    cvt16to32_shl_t cvt16to32_shl;
    cvt16to16_shl_t cvt16to16_shl;
    cvt32to16_shr_t cvt32to16_shr;

    copy_pp_t       luma_copy_pp[NUM_LUMA_PARTITIONS];
    copy_pp_t       chroma_copy_pp[NUM_CHROMA_PARTITIONS];
    copy_sp_t       luma_copy_sp[NUM_LUMA_PARTITIONS];
    copy_sp_t       chroma_copy_sp[NUM_CHROMA_PARTITIONS];

    ipfilter_ps_t   ipfilter_ps[NUM_IPFILTER_P_S];
    ipfilter_sp_t   ipfilter_sp[NUM_IPFILTER_S_P];
    ipfilter_ss_t   ipfilter_ss[NUM_IPFILTER_S_S];
    ipfilter_p2s_t  ipfilter_p2s;
    ipfilter_s2p_t  ipfilter_s2p;
    filter_pp_t     chroma_hpp[NUM_CHROMA_PARTITIONS];
    filter_pp_t     luma_hpp[NUM_LUMA_PARTITIONS];
    filter_ps_t     luma_hps[NUM_LUMA_PARTITIONS];
    filter_pp_t     chroma_vpp[NUM_CHROMA_PARTITIONS];
    filter_pp_t     luma_vpp[NUM_LUMA_PARTITIONS];
    filter_ps_t     luma_vps[NUM_LUMA_PARTITIONS];
    filter_hv_pp_t  luma_hvpp[NUM_LUMA_PARTITIONS];
    filter_p2s_t    luma_p2s;
    filter_p2s_t    chroma_p2s;
    extendCURowBorder_t extendRowBorder;

    intra_dc_t      intra_pred_dc;
    intra_planar_t  intra_pred_planar;
    intra_ang_t     intra_pred_ang;
    intra_allangs_t intra_pred_allangs[NUM_SQUARE_BLOCKS];

    dct_t           dct[NUM_DCTS];
    idct_t          idct[NUM_IDCTS];
    quant_t         quant;
    dequant_t       dequant;

    calcresidual_t  calcresidual[NUM_SQUARE_BLOCKS];
    calcrecon_t     calcrecon[NUM_SQUARE_BLOCKS];
    transpose_t     transpose[NUM_SQUARE_BLOCKS];

    weightpUni_t    weightpUni;
    weightpUniPixel_t weightpUniPixel;
    pixelsub_ps_t   pixelsub_ps;
    pixeladd_ss_t   pixeladd_ss;
    pixeladd_pp_t   pixeladd_pp;
    pixelavg_pp_t   pixelavg_pp[NUM_LUMA_PARTITIONS];

    scale_t         scale1D_128to64;
    scale_t         scale2D_64to32;
    downscale_t     frame_init_lowres_core;
    ssim_end4_t     ssim_end_4;
    var_t           var[NUM_LUMA_PARTITIONS];
    ssim_4x4x2_core_t ssim_4x4x2_core;
    plane_copy_deinterleave_t plane_copy_deinterleave_c;
};

/* This copy of the table is what gets used by the encoder.
 * It must be initialized before the encoder begins. */
extern EncoderPrimitives primitives;

void Setup_C_Primitives(EncoderPrimitives &p);
void Setup_Vector_Primitives(EncoderPrimitives &p, int cpuMask);
void Setup_Assembly_Primitives(EncoderPrimitives &p, int cpuMask);
}

#endif // ifndef X265_PRIMITIVES_H
