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

#define FENC_STRIDE 64

// from cpu-a.asm, if ASM primitives are compiled.  Else primitives.cpp
extern "C" void x265_cpu_emms(void);

#if X86_64
#define x265_emms()
#elif _MSC_VER
#include <mmintrin.h>
#define x265_emms() _mm_empty()
#elif __GNUC__
// Cannot use _mm_empty() directly without compiling all the source with
// a fixed CPU arch, which we would like to avoid at the moment
#define x265_emms() x265_cpu_emms()
#else
#define x265_emms()
#endif

#if defined(__GNUC__)
#define ALIGN_VAR_8(T, var)  T var __attribute__((aligned(8)))
#define ALIGN_VAR_16(T, var) T var __attribute__((aligned(16)))
#define ALIGN_VAR_32(T, var) T var __attribute__((aligned(32)))
#ifndef CDECL
#define CDECL
#endif
#elif defined(_MSC_VER)
#define ALIGN_VAR_8(T, var)  __declspec(align(8)) T var
#define ALIGN_VAR_16(T, var) __declspec(align(16)) T var
#define ALIGN_VAR_32(T, var) __declspec(align(32)) T var
#ifndef CDECL
#define CDECL                _cdecl
#endif
#endif // if defined(__GNUC__)

#if HIGH_BIT_DEPTH
typedef uint16_t pixel;
typedef uint32_t sum_t;
typedef uint64_t sum2_t;
typedef uint64_t pixel4;
#define PIXEL_SPLAT_X4(x) ((x) * 0x0001000100010001ULL)
#else
typedef uint8_t pixel;
typedef uint16_t sum_t;
typedef uint32_t sum2_t;
typedef uint32_t pixel4;
#define PIXEL_SPLAT_X4(x) ((x) * 0x01010101U)
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

enum SquareBlocks   // Routines can be indexed using (Blocksize/8) gives (0,1,2,4,8)
{
    BLOCK_4x4,
    BLOCK_8x8,
    BLOCK_16x16,
    BLOCK_32x32,
    BLOCK_64x64,
    NUM_BLOCKS
};

enum FilterConf
{
    //Naming convention used is - FILTER_isVertical_N_isFirst_isLast
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

// NOTE: Not all DCT functions support Dest Stride
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
    NUM_IPFILTER_P_S
};

enum IPFilterConf_S_P
{
    FILTER_V_S_P_8,
    FILTER_V_S_P_4,
    NUM_IPFILTER_S_P
};

// Returns a Partitions enum if the size matches a supported performance primitive,
// else returns -1 (in which case you should use the slow path)
int PartitionFromSizes(int Width, int Height);

typedef int  (CDECL * pixelcmp)(pixel *fenc, intptr_t fencstride, pixel *fref, intptr_t frefstride); // fenc is aligned
typedef int  (CDECL * pixelcmp_ss)(short *fenc, intptr_t fencstride, short *fref, intptr_t frefstride);
typedef int  (CDECL * pixelcmp_sp)(short *fenc, intptr_t fencstride, pixel *fref, intptr_t frefstride);
typedef void (CDECL * pixelcmp_x3)(pixel *fenc, pixel *fref0, pixel *fref1, pixel *fref2, intptr_t frefstride, int *res);
typedef void (CDECL * pixelcmp_x4)(pixel *fenc, pixel *fref0, pixel *fref1, pixel *fref2, pixel *fref3, intptr_t frefstride, int *res);
typedef void (CDECL * IPFilter)(const short *coeff, short *src, int srcStride, short *dst, int dstStride, int block_width, int block_height, int bitDepth);
typedef void (CDECL * IPFilter_p_p)(int bit_Depth, pixel *src, int srcStride, pixel *dst, int dstStride, int width, int height, short const *coeff);
typedef void (CDECL * IPFilter_p_s)(int bit_Depth, pixel *src, int srcStride, short *dst, int dstStride, int width, int height, short const *coeff);
typedef void (CDECL * IPFilter_s_p)(int bit_Depth, short *src, int srcStride, pixel *dst, int dstStride, int width, int height, short const *coeff);
typedef void (CDECL * IPFilterConvert_p_s)(int bit_Depth, pixel *src, int srcStride, short *dst, int dstStride, int width, int height);
typedef void (CDECL * IPFilterConvert_s_p)(int bit_Depth, short *src, int srcStride, pixel *dst, int dstStride, int width, int height);
typedef void (CDECL * blockcpy_p_p)(int bx, int by, pixel *dst, intptr_t dstride, pixel *src, intptr_t sstride); // dst is aligned
typedef void (CDECL * blockcpy_s_p)(int bx, int by, short *dst, intptr_t dstride, pixel *src, intptr_t sstride); // dst is aligned
typedef void (CDECL * blockcpy_p_s)(int bx, int by, pixel *dst, intptr_t dstride, short *src, intptr_t sstride); // dst is aligned
typedef void (CDECL * blockcpy_s_c)(int bx, int by, short *dst, intptr_t dstride, uint8_t *src, intptr_t sstride); // dst is aligned
typedef void (CDECL * getIPredDC_p)(pixel* pSrc, intptr_t srcStride, pixel* pDst, intptr_t dstStride, int width, int /*height*/, int blkAboveAvailable, int blkLeftAvailable, int bFilter);
typedef void (CDECL * getIPredPlanar_p)(pixel* pSrc, intptr_t srcStride, pixel* rpDst, intptr_t dstStride, int width, int /*height*/);
typedef void (CDECL * getIPredAng_p)(int bitDepth, pixel* rpDst, int dstStride, int width, int dirMode, bool bFilter, pixel *refLeft, pixel *refAbove);
typedef void (CDECL * quant)(int bitDepth, const int* pSrc, int* pDes, int iWidth, int iHeight, int mcqp_miper, int mcqp_mirem, bool useScalingList, unsigned int uiLog2TrSize, int *piDequantCoef);
typedef void (CDECL * cvt16to32_t)(short *psOrg, int *piDst, int);
typedef void (CDECL * cvt16to32_shl_t)(int *piDst, short *psOrg, intptr_t, int, int);
typedef void (CDECL * cvt16to16_shl_t)(short *psDst, short *psOrg, int, int, intptr_t, int);
typedef void (CDECL * cvt32to16_t)(int *psOrg, short *piDst, int);
typedef void (CDECL * cvt32to16_shr_t)(short *piDst, int *psOrg, int, int);
typedef void (CDECL * dct_t)(short *pSrc, int *pDst, intptr_t stride);
typedef void (CDECL * idct_t)(int *pSrc, short *pDst, intptr_t stride);
typedef void (CDECL * calcresidual_t)(pixel *piOrig, pixel *piPred, short *piRes, int stride);
typedef void (CDECL * calcrecon_t)(pixel* piPred, short* piResi,pixel*  piReco, short* piRecQt, pixel *piRecIPred, int uiStride, int uiRecQtStride, int uiRecIPredStride);
typedef void (CDECL * filterVmulti_t)(int bitDepth, short *src, int srcStride, pixel *dstA, pixel *dstE, pixel *dstI, pixel *dstP, int dstStride, int block_width, int block_height);
typedef void (CDECL * filterHmulti_t)(int bitDepth, pixel *src, int srcStride, short *dstF, short* dstA, short* dstB, short* dstC, int dstStride, int block_width, int block_height);

/* Define a structure containing function pointers to optimized encoder
 * primitives.  Each pointer can reference either an assembly routine,
 * a vectorized primitive, or a C function. */
struct EncoderPrimitives
{
    /* All pixel comparison functions take the same arguments */
    pixelcmp sad[NUM_PARTITIONS];   // Sum of Differences for each size
    pixelcmp_x3 sad_x3[NUM_PARTITIONS];   // Sum of Differences for each size
    pixelcmp_x4 sad_x4[NUM_PARTITIONS];   // Sum of Differences for each size
    pixelcmp sse_pp[NUM_PARTITIONS];   // SSE (pixel, pixel) fenc alignment not assumed
    pixelcmp_ss sse_ss[NUM_PARTITIONS]; // SSE (short, short) fenc alignment not assumed
    pixelcmp_sp sse_sp[NUM_PARTITIONS]; // SSE (short, pixel) fenc alignment not assumed
    pixelcmp satd[NUM_PARTITIONS];  // Sum of Transformed differences (HADAMARD)
    pixelcmp sa8d_8x8;
    pixelcmp sa8d_16x16;
    pixelcmp sa8d_32x32;
    pixelcmp sa8d_64x64;
    IPFilter filter[NUM_FILTER];
    IPFilter_p_p ipFilter_p_p[NUM_IPFILTER_P_P];
    IPFilter_p_s ipFilter_p_s[NUM_IPFILTER_P_S];
    IPFilter_s_p ipFilter_s_p[NUM_IPFILTER_S_P];
    IPFilterConvert_p_s ipfilterConvert_p_s;
    IPFilterConvert_s_p ipfilterConvert_s_p;
    blockcpy_p_p cpyblock;     // pixel from pixel
    blockcpy_p_s cpyblock_p_s; // pixel from short
    blockcpy_s_p cpyblock_s_p; // short from pixel
    blockcpy_s_c cpyblock_s_c; // short from unsigned char
    getIPredDC_p getIPredDC;
    getIPredPlanar_p getIPredPlanar;
    getIPredAng_p getIPredAng;
    quant deQuant;
    dct_t dct[NUM_DCTS];
    idct_t idct[NUM_IDCTS];
    cvt16to32_t cvt16to32;
    cvt16to32_shl_t cvt16to32_shl;
    cvt16to16_shl_t cvt16to16_shl;
    cvt32to16_t cvt32to16;
    cvt32to16_shr_t cvt32to16_shr;
    calcresidual_t calcresidual[NUM_BLOCKS];
    calcrecon_t calcrecon[NUM_BLOCKS];
    filterVmulti_t filterVmulti;
    filterHmulti_t filterHmulti;
};

/* This copy of the table is what gets used by all by the encoder.
 * It must be initialized before the encoder begins. */
extern EncoderPrimitives primitives;

void Setup_C_Primitives(EncoderPrimitives &p);
void Setup_Vector_Primitives(EncoderPrimitives &p, int cpuid);
void Setup_Assembly_Primitives(EncoderPrimitives &p, int cpuid);
}

#endif // ifndef X265_PRIMITIVES_H
