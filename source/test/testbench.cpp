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
#include "cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

using namespace x265;

int main(int argc, char *argv[])
{
    int cpuid = x265::cpu_detect();
    const char *testname = 0;
    int cpuid_user = -1;

    for (int i = 1; i < argc - 1; i += 2)
    {
        if (!strcmp(argv[i], "--cpuid"))
        {
            cpuid_user = atoi(argv[i + 1]);
        }
        if (!strcmp(argv[i], "--test"))
        {
            testname = argv[i + 1];
            printf("Testing only harnesses that match name <%s>\n", testname);
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

    struct test_arch_t
    {
        char name[12];
        int flag;
    } test_arch[] =
    {
        { "SSE2", X265_CPU_SSE2 },
        { "SSE3", X265_CPU_SSE3 },
        { "SSSE3", X265_CPU_SSSE3 },
        { "SSE4", X265_CPU_SSE4 },
        { "AVX", X265_CPU_AVX },
        { "XOP", X265_CPU_XOP },
        { "AVX2", X265_CPU_AVX2 },
        { "", 0 },
    };

    for (int i = 0; test_arch[i].flag; i++)
    {
        if (test_arch[i].flag & cpuid)
            printf("Testing primitives: %s\n", test_arch[i].name);
        else
            continue;
        if (cpuid_user == i)
            break;

#if ENABLE_VECTOR_PRIMITIVES
        EncoderPrimitives vecprim;
        memset(&vecprim, 0, sizeof(vecprim));
        Setup_Vector_Primitives(vecprim, test_arch[i].flag);
        for (size_t h = 0; h < sizeof(harness) / sizeof(TestHarness*); h++)
        {
            if (testname && strncmp(testname, harness[h]->getName(), strlen(testname)))
                continue;
            if (!harness[h]->testCorrectness(cprim, vecprim))
            {
                fprintf(stderr, "\nx265: intrinsic primitive has failed. Go and fix that Right Now!\n");
                return -1;
            }
        }

#endif // if ENABLE_VECTOR_PRIMITIVES

#if ENABLE_ASM_PRIMITIVES
        EncoderPrimitives asmprim;
        memset(&asmprim, 0, sizeof(asmprim));
        Setup_Assembly_Primitives(asmprim, test_arch[i].flag);
        for (size_t h = 0; h < sizeof(harness) / sizeof(TestHarness*); h++)
        {
            if (testname && strncmp(testname, harness[h]->getName(), strlen(testname)))
                continue;
            /* Here it should be asmprim and not vecprim. Right? */
            if (!harness[h]->testCorrectness(cprim, asmprim))
            {
                fprintf(stderr, "\nx265: asm primitive has failed. Go and fix that Right Now!\n");
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

    printf("\nTest performance improvement with full optimizations\n");

    for (size_t h = 0; h < sizeof(harness) / sizeof(TestHarness*); h++)
    {
        if (testname && strncmp(testname, harness[h]->getName(), strlen(testname)))
            continue;
        printf("== %s primitives ==\n", harness[h]->getName());
        harness[h]->measureSpeed(cprim, optprim);
    }

    printf("\n");
    return 0;
}
