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

#include "TLibCommon/TComRom.h"
#include "TLibCommon/ContextModel.h"
#include "primitives.h"
#include "common.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

namespace x265 {
// x265 private namespace

static int8_t maptable[] =
{
//  4          8          12          16          20  24          28  32          36  40  44  48          52  56  60  64
    LUMA_4x4,  LUMA_4x8,  -1,         LUMA_4x16,  -1, -1,         -1, -1,         -1, -1, -1, -1,         -1, -1, -1, -1,         // 4
    LUMA_8x4,  LUMA_8x8,  -1,         LUMA_8x16,  -1, -1,         -1, LUMA_8x32,  -1, -1, -1, -1,         -1, -1, -1, -1,         // 8
    -1,        -1,        -1,         LUMA_12x16, -1, -1,         -1, -1,         -1, -1, -1, -1,         -1, -1, -1, -1,         // 12
    LUMA_16x4, LUMA_16x8, LUMA_16x12, LUMA_16x16, -1, -1,         -1, LUMA_16x32, -1, -1, -1, -1,         -1, -1, -1, LUMA_16x64, // 16
    -1,        -1,        -1,         -1,         -1, -1,         -1, -1,         -1, -1, -1, -1,         -1, -1, -1, -1,         // 20
    -1,        -1,        -1,         -1,         -1, -1,         -1, LUMA_24x32, -1, -1, -1, -1,         -1, -1, -1, -1,         // 24
    -1,        -1,        -1,         -1,         -1, -1,         -1, -1,         -1, -1, -1, -1,         -1, -1, -1, -1,         // 28
    -1,        LUMA_32x8, -1,         LUMA_32x16, -1, LUMA_32x24, -1, LUMA_32x32, -1, -1, -1, -1,         -1, -1, -1, LUMA_32x64, // 32
    -1,        -1,        -1,         -1,         -1, -1,         -1, -1,         -1, -1, -1, -1,         -1, -1, -1, -1,         // 36
    -1,        -1,        -1,         -1,         -1, -1,         -1, -1,         -1, -1, -1, -1,         -1, -1, -1, -1,         // 40
    -1,        -1,        -1,         -1,         -1, -1,         -1, -1,         -1, -1, -1, -1,         -1, -1, -1, -1,         // 44
    -1,        -1,        -1,         -1,         -1, -1,         -1, -1,         -1, -1, -1, -1,         -1, -1, -1, LUMA_48x64, // 48
    -1,        -1,        -1,         -1,         -1, -1,         -1, -1,         -1, -1, -1, -1,         -1, -1, -1, -1,         // 52
    -1,        -1,        -1,         -1,         -1, -1,         -1, -1,         -1, -1, -1, -1,         -1, -1, -1, -1,         // 56
    -1,        -1,        -1,         -1,         -1, -1,         -1, -1,         -1, -1, -1, -1,         -1, -1, -1, -1,         // 60
    -1,        -1,        -1,         LUMA_64x16, -1, -1,         -1, LUMA_64x32, -1, -1, -1, LUMA_64x48, -1, -1, -1, LUMA_64x64  // 64
};

// Returns a LumaPartitions enum if the size matches a supported partition dimension
int PartitionFromSizes(int width, int height)
{
    assert(((width | height) & ~(4 | 8 | 16 | 32 | 64)) == 0);

    int8_t w = (width >> 2) - 1;
    int8_t h = (height >> 2) - 1;
    int part = (int)maptable[(w << 4) + h];
    assert(part >= 0);
    return part;
}

/* the "authoritative" set of encoder primitives */
EncoderPrimitives primitives;

void Setup_C_PixelPrimitives(EncoderPrimitives &p);
void Setup_C_DCTPrimitives(EncoderPrimitives &p);
void Setup_C_IPFilterPrimitives(EncoderPrimitives &p);
void Setup_C_IPredPrimitives(EncoderPrimitives &p);

void Setup_C_Primitives(EncoderPrimitives &p)
{
    Setup_C_PixelPrimitives(p);      // pixel.cpp
    Setup_C_DCTPrimitives(p);        // dct.cpp
    Setup_C_IPFilterPrimitives(p);   // ipfilter.cpp
    Setup_C_IPredPrimitives(p);      // intrapred.cpp
}
}

using namespace x265;

/* cpuid == 0 - auto-detect CPU type, else
 * cpuid > 0 -  force CPU type
 * cpuid < 0  - auto-detect if uninitialized */
extern "C"
void x265_setup_primitives(x265_param_t *param, int cpuid)
{
    // initialize global variables
    initROM();
    ContextModel::buildNextStateTable();

    if (cpuid < 0)
    {
        if (primitives.sad[0])
            return;
        else
            cpuid = 0;
    }
    if (cpuid == 0)
    {
        cpuid = x265::cpu_detect();
    }
    if (param->logLevel >= X265_LOG_INFO)
    {
        char buf[1000];
        char *p = buf + sprintf(buf, "using cpu capabilities:");
        for (int i = 0; x265::cpu_names[i].flags; i++)
        {
            if (!strcmp(x265::cpu_names[i].name, "SSE2")
                && cpuid & (X265_CPU_SSE2_IS_FAST | X265_CPU_SSE2_IS_SLOW))
                continue;
            if (!strcmp(x265::cpu_names[i].name, "SSE3")
                && (cpuid & X265_CPU_SSSE3 || !(cpuid & X265_CPU_CACHELINE_64)))
                continue;
            if (!strcmp(x265::cpu_names[i].name, "SSE4.1")
                && (cpuid & X265_CPU_SSE42))
                continue;
            if ((cpuid & x265::cpu_names[i].flags) == x265::cpu_names[i].flags
                && (!i || x265::cpu_names[i].flags != x265::cpu_names[i - 1].flags))
                p += sprintf(p, " %s", x265::cpu_names[i].name);
        }

        if (!cpuid)
            p += sprintf(p, " none!");
        x265_log(param, X265_LOG_INFO, "%s\n", buf);
    }

    x265_log(param, X265_LOG_INFO, "performance primitives:");

    Setup_C_Primitives(primitives);

#if ENABLE_VECTOR_PRIMITIVES
    Setup_Vector_Primitives(primitives, cpuid);
#endif
#if ENABLE_ASM_PRIMITIVES
    Setup_Assembly_Primitives(primitives, cpuid);
#endif

    primitives.sa8d_inter[LUMA_8x8] = primitives.sa8d[BLOCK_8x8];
    primitives.sa8d_inter[LUMA_16x16] = primitives.sa8d[BLOCK_16x16];
    primitives.sa8d_inter[LUMA_32x32] = primitives.sa8d[BLOCK_32x32];
    primitives.sa8d_inter[LUMA_64x64] = primitives.sa8d[BLOCK_64x64];

    // SA8D devolves to SATD for blocks not even multiples of 8x8
    primitives.sa8d_inter[LUMA_4x4]   = primitives.satd[LUMA_4x4];
    primitives.sa8d_inter[LUMA_4x8]   = primitives.satd[LUMA_4x8];
    primitives.sa8d_inter[LUMA_4x16]  = primitives.satd[LUMA_4x16];
    primitives.sa8d_inter[LUMA_8x4]   = primitives.satd[LUMA_8x4];
    primitives.sa8d_inter[LUMA_16x4]  = primitives.satd[LUMA_16x4];
    primitives.sa8d_inter[LUMA_16x12] = primitives.satd[LUMA_16x12];
    primitives.sa8d_inter[LUMA_12x16] = primitives.satd[LUMA_12x16];

#if ENABLE_VECTOR_PRIMITIVES
    if (param->logLevel >= X265_LOG_INFO) fprintf(stderr, " intrinsic");
#endif
#if ENABLE_ASM_PRIMITIVES
    if (param->logLevel >= X265_LOG_INFO) fprintf(stderr, " assembly");
#endif

    if (param->logLevel >= X265_LOG_INFO) fprintf(stderr, "\n");
}

#if !defined(ENABLE_ASM_PRIMITIVES)
// the intrinsic primitives will not use MMX instructions, so if assembly
// is disabled there should be no reason to use EMMS.
extern "C" void x265_cpu_emms(void) {}

#endif
