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
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#include "common.h"
#include "primitives.h"
#include "pixelharness.h"
#include "mbdstharness.h"
#include "ipfilterharness.h"
#include "intrapredharness.h"
#include "param.h"
#include "cpu.h"

using namespace x265;

const char* lumaPartStr[NUM_LUMA_PARTITIONS] =
{
    "  4x4",
    "  8x8", "  8x4", "  4x8",
    "16x16", " 16x8", " 8x16", "16x12", "12x16", " 16x4", " 4x16",
    "32x32", "32x16", "16x32", "32x24", "24x32", " 32x8", " 8x32",
    "64x64", "64x32", "32x64", "64x48", "48x64", "64x16", "16x64",
};

const char* chromaPartStr[NUM_CHROMA_PARTITIONS] =
{
    "  2x2", // never used by HEVC
    "  4x4", "  4x2", "  2x4",
    "  8x8", "  8x4", "  4x8", "  8x6", "  6x8", "  8x2", "  2x8",
    "16x16", " 16x8", " 8x16", "16x12", "12x16", " 16x4", " 4x16",
    "32x32", "32x16", "16x32", "32x24", "24x32", " 32x8", " 8x32",
};

void do_help()
{
    printf("x265 optimized primitive testbench\n\n");
    printf("usage: TestBench [--cpuid CPU] [--testbench BENCH] [--help]\n\n");
    printf("       CPU is comma separated SIMD arch list, example: SSE4,AVX\n");
    printf("       BENCH is one of (pixel,transforms,interp,intrapred)\n\n");
    printf("By default, the test bench will test all benches on detected CPU architectures\n");
    printf("Options and testbench name may be truncated.\n");
}

int main(int argc, char *argv[])
{
    int cpuid = x265::cpu_detect();
    const char *testname = 0;

    if (!(argc & 1))
    {
        do_help();
        return 0;
    }
    for (int i = 1; i < argc - 1; i += 2)
    {
        if (strncmp(argv[i], "--", 2))
        {
            printf("** invalid long argument: %s\n\n", argv[i]);
            do_help();
            return 1;
        }
        const char *name = argv[i] + 2;
        const char *value = argv[i + 1];
        if (!strncmp(name, "cpuid", strlen(name)))
        {
            bool bError = false;
            cpuid = parseCpuName(value, bError);
            if (bError)
            {
                printf("Invalid CPU name: %s\n", value);
                return 1;
            }
        }
        else if (!strncmp(name, "testbench", strlen(name)))
        {
            testname = value;
            printf("Testing only harnesses that match name <%s>\n", testname);
        }
        else
        {
            printf("** invalid long argument: %s\n\n", name);
            do_help();
            return 1;
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
    Setup_Alias_Primitives(cprim);

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

        EncoderPrimitives vecprim;
        memset(&vecprim, 0, sizeof(vecprim));
        Setup_Instrinsic_Primitives(vecprim, test_arch[i].flag);
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

        EncoderPrimitives asmprim;
        memset(&asmprim, 0, sizeof(asmprim));
        Setup_Assembly_Primitives(asmprim, test_arch[i].flag);
        for (size_t h = 0; h < sizeof(harness) / sizeof(TestHarness*); h++)
        {
            if (testname && strncmp(testname, harness[h]->getName(), strlen(testname)))
                continue;
            if (!harness[h]->testCorrectness(cprim, asmprim))
            {
                fprintf(stderr, "\nx265: asm primitive has failed. Go and fix that Right Now!\n");
                return -1;
            }
        }
    }

    /******************* Cycle count for all primitives **********************/

    EncoderPrimitives optprim;
    memset(&optprim, 0, sizeof(optprim));
    Setup_Instrinsic_Primitives(optprim, cpuid);
    Setup_Assembly_Primitives(optprim, cpuid);
    Setup_Alias_Primitives(optprim);

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
