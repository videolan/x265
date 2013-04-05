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

#include "primitives.h"
#include "instrset.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

namespace x265 {
// x265 private namespace

static int8_t psize[16][16] =
{
    // 4, 8, 12, 16, 20, 24, 28, 32
    { PARTITION_4x4, PARTITION_4x8, -1, PARTITION_4x16, -1, -1, -1, PARTITION_4x32
      -1, -1, -1, -1, -1, -1, -1, PARTITION_4x64},
    { PARTITION_8x4, PARTITION_8x8, -1, PARTITION_8x16, -1, -1, -1, PARTITION_8x32,
      -1, -1, -1, -1, -1, -1, -1, PARTITION_8x64},
    { -1, -1, -1, -1, -1, -1, -1, -1 , -1},
    { PARTITION_16x4, PARTITION_16x8, -1, PARTITION_16x16, -1, -1, -1, PARTITION_16x32,
      -1, -1, -1, -1, -1, -1, -1, PARTITION_16x64},
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    { PARTITION_32x4, PARTITION_32x8, -1, PARTITION_32x16, -1, -1, -1, PARTITION_32x32,
      -1, -1, -1, -1, -1, -1, -1, PARTITION_32x64},
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    { PARTITION_64x4, PARTITION_64x8, -1, PARTITION_64x16, -1, -1, -1, PARTITION_64x32,
      -1, -1, -1, -1, -1, -1, -1, PARTITION_64x64}
};

// Returns a Partitions enum if the size matches a supported performance primitive,
// else returns -1 (in which case you should use the slow path)
int PartitionFromSizes(int Width, int Height)
{
    if ((Width | Height) & ~(4 | 8 | 16 | 32)) // Check for bits in the wrong places
        return -1;

    if (Width > 32 || Height > 32)
        return -1;

    return (int)psize[(Width >> 2) - 1][(Height >> 2) - 1];
}

/* the "authoritative" set of encoder primitives */
#if ENABLE_PRIMITIVES
EncoderPrimitives primitives;
#endif

void Setup_C_Primitives(EncoderPrimitives &p)
{
    Setup_C_PixelPrimitives(p);      // pixel.cpp
    Setup_C_MacroblockPrimitives(p); // macroblock.cpp
}

/* cpuid == 0 - auto-detect CPU type, else
 * cpuid != 0 - force CPU type */
void SetupPrimitives(int cpuid)
{
    if (cpuid == 0)
    {
        cpuid = CpuIDDetect();
    }

#if ENABLE_PRIMITIVES
    Setup_C_Primitives(primitives);

    /* Pick best vector architecture to use as a baseline. */
#if defined(__GNUC__) || defined(_MSC_VER)
    if (cpuid > 1) Setup_Vec_Primitives_sse2(primitives);

    if (cpuid > 2) Setup_Vec_Primitives_sse3(primitives);

    if (cpuid > 3) Setup_Vec_Primitives_ssse3(primitives);

    if (cpuid > 4) Setup_Vec_Primitives_sse41(primitives);

    if (cpuid > 5) Setup_Vec_Primitives_sse42(primitives);

#endif // if defined(__GNUC__) || defined(_MSC_VER)
#if (defined(_MSC_VER) && _MSC_VER >= 1600) || defined(__GNUC__)
    if (cpuid > 6) Setup_Vec_Primitives_avx(primitives);

#endif
#if defined(_MSC_VER) && _MSC_VER >= 1700
    if (cpuid > 7) Setup_Vec_Primitives_avx2(primitives);

#endif

    /* .. upgrade functions with available assembly code. */
#endif // if ENABLE_PRIMITIVES
}

static const char *CpuType[] = {
    "auto-detect (80386)",
    "SSE XMM",
    "SSE2",
    "SSE3",
    "SSSE3",
    "SSE4.1",
    "SSE4.2",
    "AVX",
    "AVX2",
    0
};

int CpuIDDetect(void)
{
    int iset = instrset_detect(); // Detect supported instruction set

    if (iset < 1)
    {
        fprintf(stderr, "\nError: Instruction set detect is not supported on this computer");
        return 0;
    }
    else
    {
        fprintf(stdout, "x265: detected SIMD architectures ");
        for (int i = 1; i <= iset; i++ )
            fprintf(stdout, "%s ", CpuType[i]);
        fprintf(stdout, "\n");
        return iset;
    }
}

}
