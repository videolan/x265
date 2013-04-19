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

#if ENABLE_ASM_PRIMITIVES && !defined(_MSC_VER)
extern "C" void x264_cpu_emms( void );
#else
#define x264_cpu_emms()
#endif

#if defined(__GNUC__)
#define ALIGN_VAR_8(T, var)  T var __attribute__((aligned(8)))
#define ALIGN_VAR_16(T, var) T var __attribute__((aligned(16)))
#define CDECL
#elif defined(_MSC_VER)
#define ALIGN_VAR_8(T, var)  __declspec(align(8)) T var
#define ALIGN_VAR_16(T, var) __declspec(align(16)) T var
#define CDECL                _cdecl
#endif

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
    PARTITION_4x4,
    PARTITION_8x4,
    PARTITION_4x8,
    PARTITION_8x8,
    PARTITION_4x16,
    PARTITION_16x4,
    PARTITION_8x16,
    PARTITION_16x8,
    PARTITION_16x16,
    PARTITION_4x32,
    PARTITION_32x4,
    PARTITION_8x32,
    PARTITION_32x8,
    PARTITION_16x32,
    PARTITION_32x16,
    PARTITION_32x32,
    PARTITION_4x64,
    PARTITION_64x4,
    PARTITION_8x64,
    PARTITION_64x8,
    PARTITION_16x64,
    PARTITION_64x16,
    PARTITION_32x64,
    PARTITION_64x32,
    PARTITION_64x64,
    NUM_PARTITIONS
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

enum Butterflies
{
    BUTTERFLY_4,
    BUTTERFLY_INVERSE_4,
    BUTTERFLY_8,
    BUTTERFLY_INVERSE_8,
    BUTTERFLY_16,
    BUTTERFLY_INVERSE_16,
    BUTTERFLY_32,
    BUTTERFLY_INVERSE_32,
    NUM_BUTTERFLIES
};

// Returns a Partitions enum if the size matches a supported performance primitive,
// else returns -1 (in which case you should use the slow path)
int PartitionFromSizes(int Width, int Height);

typedef int (CDECL * pixelcmp)(pixel *fenc, intptr_t fencstride, pixel *fref, intptr_t frefstride);
typedef void (CDECL * mbdst)(short *block, short *coeff, int shift);
typedef void (CDECL * IPFilter)(const short *coeff, pixel *src, int srcStride, pixel *dst, int dstStride, int block_width,
                                int block_height, int bitDepth);
typedef void (CDECL * butterfly)(short *src, short *dst, int shift, int line);

/* Define a structure containing function pointers to optimized encoder
 * primitives.  Each pointer can reference either an assembly routine,
 * a vectorized primitive, or a C function. */
struct EncoderPrimitives
{
    /* All pixel comparison functions take the same arguments */
    pixelcmp sad[NUM_PARTITIONS];   // Sum of Differences for each size
    pixelcmp satd[NUM_PARTITIONS];  // Sum of Transformed differences (HADAMARD)
    pixelcmp sa8d_8x8;
    pixelcmp sa8d_16x16;
    IPFilter filter[NUM_FILTER];
    mbdst inversedst;
    butterfly partial_butterfly[NUM_BUTTERFLIES];
};

/* This copy of the table is what gets used by all by the encoder.
 * It must be initialized before the encoder begins. */
extern EncoderPrimitives primitives;
void SetupPrimitives(int cpuid = 0);
int CpuIDDetect(void);

void Setup_C_Primitives(EncoderPrimitives &p);
void Setup_Vector_Primitives(EncoderPrimitives &p, int cpuid);
void Setup_Assembly_Primitives(EncoderPrimitives &p, int cpuid);
}

#endif // ifndef X265_PRIMITIVES_H
