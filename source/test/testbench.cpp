/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Gopu Govindaswamy <gopu@govindaswamy.org>
 *          Mandar Gurav <mandar@multicorewareinc.com>
 *          Mahesh Pittala <mahesh@multicorewareinc.com>
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
#include "pixelharness.h"
#include "mbdstharness.h"
#include "ipfilterharness.h"
#include "intrapredharness.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <malloc.h>

using namespace x265;

#ifdef __MINGW32__
#define _aligned_malloc __mingw_aligned_malloc
#define _aligned_free  __mingw_aligned_free
#endif

void *TestHarness::alignedMalloc(size_t size, int count, int alignment)
{
#if _WIN32
    return _aligned_malloc(count * size, alignment);
#else
    void *ptr;
    if (posix_memalign((void**)&ptr, alignment, count * size) == 0)
        return ptr;
    else
        return NULL;
#endif
}

void TestHarness::alignedFree(void *ptr)
{
#if _WIN32
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

int main(int argc, char *argv[])
{
    int cpuid = CpuIDDetect();

    for (int i = 1; i < argc - 1; i += 2)
    {
        if (!strcmp(argv[i], "--cpuid"))
        {
            cpuid = atoi(argv[i + 1]);
        }
    }

    int seed = (int)time(NULL);
    const char *bpp[] = { "8bpp", "16bpp" };
    printf("Using random seed %X %s\n", seed, bpp[HIGH_BIT_DEPTH]);
    srand(seed);

    PixelHarness  HPixel;
    MBDstHarness  HMBDist;
    IPFilterHarness HIPFilter;
    IntraPredHarness HIPred;

    // To disable classes of tests, simply comment them out in this list
    TestHarness *harness[] =
    {
        &HPixel,
        &HMBDist,
        &HIPFilter,
        &HIPred
    };

    EncoderPrimitives cprim;
    memset(&cprim, 0, sizeof(EncoderPrimitives));
    Setup_C_Primitives(cprim);

    for (int i = 1; i <= cpuid; i++)
    {
#if ENABLE_VECTOR_PRIMITIVES
        EncoderPrimitives vecprim;
        memset(&vecprim, 0, sizeof(vecprim));
        Setup_Vector_Primitives(vecprim, i);
        printf("Testing vector class primitives: CPUID %d\n", i);
        for (size_t h = 0; h < sizeof(harness) / sizeof(TestHarness*); h++)
        {
            if (!harness[h]->testCorrectness(cprim, vecprim))
            {
                fprintf(stderr, "\nx265: vector primitive has failed. Go and fix that Right Now!\n");
                return -1;
            }
        }
#endif // if ENABLE_VECTOR_PRIMITIVES

#if ENABLE_ASM_PRIMITIVES
        EncoderPrimitives asmprim;
        memset(&asmprim, 0, sizeof(asmprim));
        Setup_Assembly_Primitives(asmprim, i);
        printf("Testing assembly primitives: CPUID %d\n", i);
        for (size_t h = 0; h < sizeof(harness) / sizeof(TestHarness*); h++)
        {
            if (!harness[h]->testCorrectness(cprim, vecprim))
            {
                fprintf(stderr, "\nx265: ASM primitive has failed. Go and fix that Right Now!\n");
                return -1;
            }
        }
#endif // if ENABLE_ASM_PRIMITIVES
    }

    /******************* Cycle count for all primitives **********************/

    EncoderPrimitives optprim;
    memset(&optprim, 0, sizeof(optprim));
#if ENABLE_VECTOR_PRIMITIVES
    Setup_Vector_Primitives(optprim, cpuid);
#endif
#if ENABLE_ASM_PRIMITIVES
    Setup_Assembly_Primitives(optprim, cpuid);
#endif

    fprintf(stderr, "\nTest performance improvement with full optimizations\n");

    for (size_t h = 0; h < sizeof(harness) / sizeof(TestHarness*); h++)
    {
        harness[h]->measureSpeed(cprim, optprim);
    }

    printf("\n");
    return 0;
}
