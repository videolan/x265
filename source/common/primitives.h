/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Mandar Gurav <mandar@multicorewareinc.com>
 *          Deepthi Devaki Akkoorath <deepthidevaki@multicorewareinc.com>
 *          Mahesh Pittala <mahesh@multicorewareinc.com>
 *          Rajesh Paulraj <rajesh@multicorewareinc.com>
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
#include "x265.h"

#define FENC_STRIDE 64

// from cpu-a.asm, if ASM primitives are compiled, else primitives.cpp
extern "C" void x265_cpu_emms(void);

#if _MSC_VER && _WIN64
#define x265_emms() x265_cpu_emms()
#elif _MSC_VER
#include <mmintrin.h>
#define x265_emms() _mm_empty()
#elif __GNUC__
// Cannot use _mm_empty() directly without compiling all the source with
// a fixed CPU arch, which we would like to avoid at the moment
#define x265_emms() x265_cpu_emms()
#else
#define x265_emms() x265_cpu_emms()
#endif

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
#else
typedef uint8_t pixel;
typedef uint16_t sum_t;
typedef uint32_t sum2_t;
typedef uint32_t pixel4;
#endif // if HIGH_BIT_DEPTH

namespace x265 {
// x265 private namespace

enum Partitions
{
    PARTITION_4x4,  PARTITION_4x8,  PARTITION_4x12,  PARTITION_4x16,  PARTITION_4x24,  PARTITION_4x32,  PARTITION_4x48,   PARTITION_4x64,
    PARTITION_8x4,  PARTITION_8x8,  PARTITION_8x12,  PARTITION_8x16,  PARTITION_8x24,  PARTITION_8x32,  PARTITION_8x48,   PARTITION_8x64,
    PARTITION_12x4, PARTITION_12x8, PARTITION_12x12, PARTITION_12x16, PARTITION_12x24, PARTITION_12x32, PARTITION_12x48,  PARTITION_12x64,
    PARTITION_16x4, PARTITION_16x8, PARTITION_16x12, PARTITION_16x16, PARTITION_16x24, PARTITION_16x32, PARTITION_16x48,  PARTITION_16x64,
    PARTITION_24x4, PARTITION_24x8, PARTITION_24x12, PARTITION_24x16, PARTITION_24x24, PARTITION_24x32, PARTITION_24x48,  PARTITION_24x64,
    PARTITION_32x4, PARTITION_32x8, PARTITION_32x12, PARTITION_32x16, PARTITION_32x24, PARTITION_32x32, PARTITION_32x48,  PARTITION_32x64,
    PARTITION_48x4, PARTITION_48x8, PARTITION_48x12, PARTITION_48x16, PARTITION_48x24, PARTITION_48x32, PARTITION_48x48,  PARTITION_48x64,
    PARTITION_64x4, PARTITION_64x8, PARTITION_64x12, PARTITION_64x16, PARTITION_64x24, PARTITION_64x32, PARTITION_64x48,  PARTITION_64x64,
    NUM_PARTITIONS
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

enum FilterConf
{
    FILTER_H_4_0_0,
    FILTER_H_4_0_1,
    FILTER_H_4_1_0,
    FILTER_H_4_1_1,

    FILTER_H_8_0_0,
    FILTER_H_8_0_1,
    FILTER_H_8_1_0,
    FILTER_H_8_1_1,

    FILTER_V_4_0_0,
    FILTER_V_4_0_1,
    FILTER_V_4_1_0,
    FILTER_V_4_1_1,

    FILTER_V_8_0_0,
    FILTER_V_8_0_1,
    FILTER_V_8_1_0,
    FILTER_V_8_1_1,
    NUM_FILTER
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

enum IPFilterConf_P_P
{
    FILTER_H_P_P_8,
    FILTER_H_P_P_4,
    FILTER_V_P_P_8,
    FILTER_V_P_P_4,
    NUM_IPFILTER_P_P
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

// Returns a Partitions enum for the given size, always expected to return a valid enum
int PartitionFromSizes(int width, int height);

typedef int  (*pixelcmp_t)(pixel *fenc, intptr_t fencstride, pixel *fref, intptr_t frefstride); // fenc is aligned
typedef int  (*pixelcmp_ss_t)(short *fenc, intptr_t fencstride, short *fref, intptr_t frefstride);
typedef int  (*pixelcmp_sp_t)(short *fenc, intptr_t fencstride, pixel *fref, intptr_t frefstride);
typedef void (*pixelcmp_x4_t)(pixel *fenc, pixel *fref0, pixel *fref1, pixel *fref2, pixel *fref3, intptr_t frefstride, int *res);
typedef void (*pixelcmp_x3_t)(pixel *fenc, pixel *fref0, pixel *fref1, pixel *fref2, intptr_t frefstride, int *res);
typedef void (*ipfilter_t)(const short *coeff, short *src, int srcStride, short *dst, int dstStride, int block_width, int block_height);
typedef void (*ipfilter_pp_t)(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int width, int height, const short *coeff);
typedef void (*ipfilter_ps_t)(pixel *src, intptr_t srcStride, short *dst, intptr_t dstStride, int width, int height, const short *coeff);
typedef void (*ipfilter_sp_t)(short *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int width, int height, const short *coeff);
typedef void (*ipfilter_ss_t)(short *src, intptr_t srcStride, short *dst, intptr_t dstStride, int width, int height, const short *coeff);
typedef void (*ipfilter_p2s_t)(pixel *src, intptr_t srcStride, short *dst, intptr_t dstStride, int width, int height);
typedef void (*ipfilter_s2p_t)(short *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int width, int height);
typedef void (*blockcpy_pp_t)(int bx, int by, pixel *dst, intptr_t dstride, pixel *src, intptr_t sstride); // dst is aligned
typedef void (*blockcpy_sp_t)(int bx, int by, short *dst, intptr_t dstride, pixel *src, intptr_t sstride); // dst is aligned
typedef void (*blockcpy_ps_t)(int bx, int by, pixel *dst, intptr_t dstride, short *src, intptr_t sstride); // dst is aligned
typedef void (*blockcpy_sc_t)(int bx, int by, short *dst, intptr_t dstride, uint8_t *src, intptr_t sstride); // dst is aligned
typedef void (*pixelsub_sp_t)(int bx, int by, short *dst, intptr_t dstride, pixel *src0, pixel *src1, intptr_t sstride0, intptr_t sstride1);
typedef void (*pixeladd_ss_t)(int bx, int by, short *dst, intptr_t dstride, short *src0, short *src1, intptr_t sstride0, intptr_t sstride1);
typedef void (*pixeladd_pp_t)(int bx, int by, pixel *dst, intptr_t dstride, pixel *src0, pixel *src1, intptr_t sstride0, intptr_t sstride1);
typedef void (*pixelavg_pp_t)(pixel *dst, intptr_t dstride, pixel *src0, intptr_t sstride0, pixel *src1, intptr_t sstride1, int weight);
typedef void (*blockfil_s_t)(short *dst, intptr_t dstride, short val);

typedef void (*intra_dc_t)(pixel* above, pixel* left, pixel* dst, intptr_t dstStride, int width, int bFilter);
typedef void (*intra_planar_t)(pixel* above, pixel* left, pixel* dst, intptr_t dstStride, int width);
typedef void (*intra_ang_t)(pixel* dst, int dstStride, int width, int dirMode, bool bFilter, pixel *refLeft, pixel *refAbove);
typedef void (*intra_allangs_t)(pixel *dst, pixel *above0, pixel *left0, pixel *above1, pixel *left1, bool bLuma);

typedef void (*cvt16to32_t)(short *src, int *dst, int);
typedef void (*cvt16to32_shl_t)(int *dst, short *src, intptr_t, int, int);
typedef void (*cvt16to16_shl_t)(short *dst, short *src, int, int, intptr_t, int);
typedef void (*cvt32to16_t)(int *src, short *dst, int);
typedef void (*cvt32to16_shr_t)(short *dst, int *src, int, int);

typedef void (*dct_t)(short *src, int *dst, intptr_t stride);
typedef void (*idct_t)(int *src, short *dst, intptr_t stride);
typedef void (*calcresidual_t)(pixel *fenc, pixel *pred, short *residual, int stride);
typedef void (*calcrecon_t)(pixel* pred, short* residual, pixel* recon, short* reconqt, pixel *reconipred, int stride, int strideqt, int strideipred);
typedef void (*transpose_t)(pixel* dst, pixel* src, intptr_t stride);
typedef uint32_t (*quant_t)(int *coef, int *quantCoeff, int *deltaU, int *qCoef, int qBits, int add, int numCoeff, int* lastPos);
typedef void (*dequant_t)(const int* src, int* dst, int width, int height, int mcqp_miper, int mcqp_mirem, bool useScalingList,
                          unsigned int trSizeLog2, int *dequantCoef);

typedef void (*filterVwghtd_t)(short *src, intptr_t srcStride, pixel *dstE, pixel *dstI, pixel *dstP, intptr_t dstStride, int block_width,
                               int block_height, int marginX, int marginY, int w, int roundw, int shiftw, int offsetw);
typedef void (*filterHwghtd_t)(pixel *src, intptr_t srcStride, short *midF, short* midA, short* midB, short* midC, intptr_t midStride,
                               pixel *dstF, pixel *dstA, pixel *dstB, pixel *dstC, intptr_t dstStride, int block_width, int block_height,
                               int marginX, int marginY, int w, int roundw, int shiftw, int offsetw);
typedef void (*filterRowH_t)(pixel *src, intptr_t srcStride, short* midA, short* midB, short* midC, intptr_t midStride, pixel *dstA, pixel *dstB, pixel *dstC, int width, int height, int marginX, int marginY, int row, int isLastRow);
typedef void (*filterRowV_0_t)(pixel *src, intptr_t srcStride, pixel *dstA, pixel *dstB, pixel *dstC, int width, int height, int marginX, int marginY, int row, int isLastRow);
typedef void (*filterRowV_N_t)(short *midA, intptr_t midStride, pixel *dstA, pixel *dstB, pixel *dstC, intptr_t dstStride, int width, int height, int marginX, int marginY, int row, int isLastRow);
typedef void (*extendCURowBorder_t)(pixel* txt, intptr_t stride, int width, int height, int marginX);


typedef void (*weightpUniPixel_t)(pixel *src, pixel *dst, intptr_t srcStride, intptr_t dstStride, int width, int height, int w0, int round, int shift, int offset);
typedef void (*weightpUni_t)(int16_t *src, pixel *dst, intptr_t srcStride, intptr_t dstStride, int width, int height, int w0, int round, int shift, int offset);
typedef void (*scale_t)(pixel *dst, pixel *src, intptr_t stride);
typedef void (*downscale_t)(pixel *src0, pixel *dstf, pixel *dsth, pixel *dstv, pixel *dstc,
                            intptr_t src_stride, intptr_t dst_stride, int width, int height);

/* Define a structure containing function pointers to optimized encoder
 * primitives.  Each pointer can reference either an assembly routine,
 * a vectorized primitive, or a C function. */
struct EncoderPrimitives
{
    pixelcmp_t      sad[NUM_PARTITIONS];        // Sum of Differences for each size
    pixelcmp_x3_t   sad_x3[NUM_PARTITIONS];     // Sum of Differences 3x for each size
    pixelcmp_x4_t   sad_x4[NUM_PARTITIONS];     // Sum of Differences 4x for each size
    pixelcmp_t      sse_pp[NUM_PARTITIONS];     // Sum of Square Error (pixel, pixel) fenc alignment not assumed
    pixelcmp_ss_t   sse_ss[NUM_PARTITIONS];     // Sum of Square Error (short, short) fenc alignment not assumed
    pixelcmp_sp_t   sse_sp[NUM_PARTITIONS];     // Sum of Square Error (short, pixel) fenc alignment not assumed
    pixelcmp_t      satd[NUM_PARTITIONS];       // Sum of Transformed differences (HADAMARD)
    pixelcmp_t      sa8d_inter[NUM_PARTITIONS]; // sa8d primitives for motion search partitions
    pixelcmp_t      sa8d[NUM_SQUARE_BLOCKS];    // sa8d primitives for square intra blocks

    blockcpy_pp_t   blockcpy_pp;                // block copy pixel from pixel
    blockcpy_ps_t   blockcpy_ps;                // block copy pixel from short
    blockcpy_sp_t   blockcpy_sp;                // block copy short from pixel
    blockcpy_sc_t   blockcpy_sc;                // block copy short from unsigned char
    blockfil_s_t    blockfil_s[NUM_SQUARE_BLOCKS];  // block fill with value
    cvt16to32_t     cvt16to32;
    cvt16to32_shl_t cvt16to32_shl;
    cvt16to16_shl_t cvt16to16_shl;
    cvt32to16_t     cvt32to16;
    cvt32to16_shr_t cvt32to16_shr;

    ipfilter_pp_t   ipfilter_pp[NUM_IPFILTER_P_P];
    ipfilter_ps_t   ipfilter_ps[NUM_IPFILTER_P_S];
    ipfilter_sp_t   ipfilter_sp[NUM_IPFILTER_S_P];
    ipfilter_ss_t   ipfilter_ss[NUM_IPFILTER_S_S];
    ipfilter_p2s_t  ipfilter_p2s;
    ipfilter_s2p_t  ipfilter_s2p;
    filterRowH_t    filterRowH;
    filterRowV_0_t  filterRowV_0;
    filterRowV_N_t  filterRowV_N;
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
    pixelsub_sp_t   pixelsub_sp;
    pixeladd_ss_t   pixeladd_ss;
    pixeladd_pp_t   pixeladd_pp;
    pixelavg_pp_t   pixelavg_pp[NUM_PARTITIONS];

    filterVwghtd_t  filterVwghtd;
    filterHwghtd_t  filterHwghtd;

    scale_t         scale1D_128to64;
    scale_t         scale2D_64to32;
    downscale_t     frame_init_lowres_core;
};

/* This copy of the table is what gets used by the encoder.
 * It must be initialized before the encoder begins. */
extern EncoderPrimitives primitives;

void Setup_C_Primitives(EncoderPrimitives &p);
void Setup_Vector_Primitives(EncoderPrimitives &p, int cpuMask);
void Setup_Assembly_Primitives(EncoderPrimitives &p, int cpuMask);
}

#endif // ifndef X265_PRIMITIVES_H
