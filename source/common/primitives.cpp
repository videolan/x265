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
#include "bitcost.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

namespace x265 {
// x265 private namespace

//                           4   8  12  16/48   24     32/64
static int8_t psize[16] = {  0,  1,  2,  3, -1,  4, -1, 5,
                            -1, -1, -1,  6, -1, -1, -1, 7 };
int *Motion_Cost;

// Returns a Partitions enum if the size matches a supported performance primitive,
// else returns -1 (in which case you should use the slow path)
int PartitionFromSizes(int Width, int Height)
{
    int8_t w = psize[(Width >> 2) - 1];
    int8_t h = psize[(Height >> 2) - 1];

    assert(((Width | Height) & ~(4 | 8 | 16 | 32 | 64)) == 0);
    assert((w | h) >= 0);

    // there are currently eight height partitions per width
    return (w << 3) + h;
}

/* the "authoritative" set of encoder primitives */
EncoderPrimitives primitives;

void Setup_C_PixelPrimitives(EncoderPrimitives &p);
void Setup_C_MacroblockPrimitives(EncoderPrimitives &p);
void Setup_C_IPFilterPrimitives(EncoderPrimitives &p);
void Setup_C_IPredPrimitives(EncoderPrimitives &p);

void Setup_C_Primitives(EncoderPrimitives &p)
{
    Setup_C_PixelPrimitives(p);      // pixel.cpp
    Setup_C_MacroblockPrimitives(p); // macroblock.cpp
    Setup_C_IPFilterPrimitives(p);   // InterpolationFilter.cpp
    Setup_C_IPredPrimitives(p);      // IntraPred.cpp
}

/* cpuid == 0 - auto-detect CPU type, else
 * cpuid > 0 -  force CPU type
 * cpuid < 0  - auto-detect if uninitialized */
void SetupPrimitives(int cpuid)
{
    if (cpuid < 0)
    {
        if (primitives.sad[0])
            return;
        else
            cpuid = 0;
    }
    if (cpuid == 0)
    {
        cpuid = CpuIDDetect();
    }

    fprintf(stdout, "x265: performance primitives:");

    Setup_C_Primitives(primitives);

#if ENABLE_VECTOR_PRIMITIVES
    Setup_Vector_Primitives(primitives, cpuid);
    fprintf(stdout, " vector");
#endif

#if ENABLE_ASM_PRIMITIVES
    Setup_Assembly_Primitives(primitives, cpuid);
    fprintf(stdout, " assembly");
#endif

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
        if (hasXOP())  fprintf(stdout, "XOP ");
        if (hasFMA3()) fprintf(stdout, "FMA3 ");
        if (hasFMA4()) fprintf(stdout, "FMA4 ");

        fprintf(stdout, "\n");
        return iset;
    }
}
}

extern "C"
void x265_init_primitives(int cpuid)
{
    x265::SetupPrimitives(cpuid);
}

extern "C"
void x265_cleanup(void)
{
    x265::BitCost::destroy();
}

#if !defined(ENABLE_ASM_PRIMITIVES)
extern "C"
void x264_cpu_emms(void)
{
}
#endif
