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

#ifndef X265_PRIMITIVES_H
#define X265_PRIMITIVES_H

#include <stdint.h>

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
    NUM_PARTITIONS
};

// Returns a Partitions enum if the size matches a supported performance primitive,
// else returns -1 (in which case you should use the slow path)
int PartitionFromSizes(int Width, int Height);

typedef int (CDECL * pixelcmp)(pixel *fenc, intptr_t fencstride, pixel *fref, intptr_t frefstride);
typedef int (CDECL * dst)(short *block, short *coeff, int shift);

/* Define a structure containing function pointers to optimized encoder
 * primitives.  Each pointer can reference either an assembly routine,
 * a vectorized primitive, or a C function. */
struct EncoderPrimitives
{
    /* All pixel comparison functions take the same arguments */
    pixelcmp sad[NUM_PARTITIONS];   // Sum of Differences for each size
    pixelcmp satd[NUM_PARTITIONS];  // Sum of Transformed differences (HADAMARD)

   /* primitive for fastInverseDst function */
	dst inversedst;

    /* .. Define primitives for more things .. */
};

/* This copy of the table is what gets used by all by the encoder.
 * It must be initialized before the encoder begins. */
extern EncoderPrimitives primitives;
void SetupPrimitives(int cpuid = 0);
int CpuIDDetect(void);

void Setup_C_Primitives(EncoderPrimitives &p);
void Setup_C_PixelPrimitives(EncoderPrimitives &p);
void Setup_C_MacroblockPrimitives(EncoderPrimitives &p);

/* These functions are defined by C++ files in encoder/vec. Depending on your
 * compiler, some of them may be undefined.  The #if logic here must match the
 * file lists in vec/CMakeLists.txt */
#if defined(__GNUC__) || defined(_MSC_VER)
extern void Setup_Vec_Primitives_sse42(EncoderPrimitives&);
extern void Setup_Vec_Primitives_sse41(EncoderPrimitives&);
extern void Setup_Vec_Primitives_ssse3(EncoderPrimitives&);
extern void Setup_Vec_Primitives_sse3(EncoderPrimitives&);
extern void Setup_Vec_Primitives_sse2(EncoderPrimitives&);
#endif
#if (defined(_MSC_VER) && _MSC_VER >= 1600) || defined(__GNUC__)
extern void Setup_Vec_Primitives_avx(EncoderPrimitives&);
#endif
#if defined(_MSC_VER) && _MSC_VER >= 1700
extern void Setup_Vec_Primitives_avx2(EncoderPrimitives&);
#endif
}

#endif // ifndef X265_PRIMITIVES_H
