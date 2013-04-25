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

#if ENABLE_PRIMITIVES
                        //  4   8      16             32 / 64
static int8_t psize[16] = { 0,  1, -1,  2, -1, -1, -1, 3, 
                           -1, -1, -1, -1, -1, -1, -1, 4 };

// Returns true if the given height could support an optimized primitive
bool FastHeight(int Height)
{
    return !(Height & ~3) && psize[(Height >> 2) - 1] >= 0;
}

// Returns a Partitions enum if the size matches a supported performance primitive,
// else returns -1 (in which case you should use the slow path)
int PartitionFromSizes(int Width, int Height)
{
    // Check for bits in the unexpected places
    if ((Width | Height) & ~(4 | 8 | 16 | 32 | 64))
        return -1;

    int8_t w = psize[(Width >> 2) - 1];
    int8_t h = psize[(Height >> 2) - 1];
    if ((w | h) < 0)
        return -1;

    // there are currently five height partitions per width
    Partitions part = (Partitions)(w * 5 + h);
    return (int) part;
}

/* the "authoritative" set of encoder primitives */
EncoderPrimitives primitives;

void Setup_C_PixelPrimitives(EncoderPrimitives &p);
void Setup_C_MacroblockPrimitives(EncoderPrimitives &p);
void Setup_C_IPFilterPrimitives(EncoderPrimitives &p);

void Setup_C_Primitives(EncoderPrimitives &p)
{
    Setup_C_PixelPrimitives(p);      // pixel.cpp
    Setup_C_MacroblockPrimitives(p); // macroblock.cpp
    Setup_C_IPFilterPrimitives(p);   // InterpolationFilter.cpp
}

#endif // if ENABLE_PRIMITIVES

/* cpuid == 0 - auto-detect CPU type, else
 * cpuid != 0 - force CPU type */
void SetupPrimitives(int cpuid)
{
    if (cpuid == 0)
    {
        cpuid = CpuIDDetect();
    }

    fprintf(stdout, "x265: performance primitives:");

#if ENABLE_PRIMITIVES
    Setup_C_Primitives(primitives);

#if ENABLE_VECTOR_PRIMITIVES
    Setup_Vector_Primitives(primitives, cpuid);
    fprintf(stdout, " vector");
#endif

#if ENABLE_ASM_PRIMITIVES
    Setup_Assembly_Primitives(primitives, cpuid);
    fprintf(stdout, " assembly");
#endif

#else // if ENABLE_PRIMITIVES
    fprintf(stdout, " disabled!");
#endif // if ENABLE_PRIMITIVES

    fprintf(stdout, "\n");
}

static const char *CpuType[] =
{
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
        for (int i = 1; i <= iset; i++)
        {
            fprintf(stdout, "%s ", CpuType[i]);
        }

        fprintf(stdout, "\n");
        return iset;
    }
}
}
