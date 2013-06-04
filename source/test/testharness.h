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

#ifndef _TESTHARNESS_H_
#define _TESTHARNESS_H_ 1

#include "primitives.h"
#include <stddef.h>

#if HIGH_BIT_DEPTH
#define BIT_DEPTH 10
#else
#define BIT_DEPTH 8
#endif
#define PIXEL_MAX ((1 << BIT_DEPTH) - 1)

class TestHarness
{
public:

    TestHarness() {}

    virtual ~TestHarness() {}

    virtual bool testCorrectness(const x265::EncoderPrimitives& ref, const x265::EncoderPrimitives& opt) = 0;

    virtual void measureSpeed(const x265::EncoderPrimitives& ref, const x265::EncoderPrimitives& opt) = 0;

    static void *alignedMalloc(size_t size, int count, int alignment);

    static void alignedFree(void *ptr);
};

#ifdef _MSC_VER
#include <intrin.h>
#elif defined(__GNUC__)
static inline uint32_t __rdtsc(void)
{
    uint32_t a = 0;

    asm volatile("rdtsc" : "=a" (a) ::"edx");
    return a;
}

#endif // ifdef _MSC_VER

#define BENCH_RUNS 1000

// Adapted from checkasm.c, runs each optimized primitive four times, measures rdtsc
// and discards invalid times.  Repeats 1000 times to get a good average.  Then measures
// the C reference with fewer runs and reports X factor and average cycles.
#define REPORT_SPEEDUP(RUNOPT, RUNREF, ...) \
    { \
        uint32_t cycles = 0; int runs = 0; \
        RUNOPT(__VA_ARGS__); \
        for (int ti = 0; ti < BENCH_RUNS; ti++) { \
            uint32_t t0 = (uint32_t)__rdtsc(); \
            RUNOPT(__VA_ARGS__); \
            RUNOPT(__VA_ARGS__); \
            RUNOPT(__VA_ARGS__); \
            RUNOPT(__VA_ARGS__); \
            uint32_t t1 = (uint32_t)__rdtsc() - t0; \
            if (t1 * runs <= cycles * 4 && ti > 0) { cycles += t1; runs++; } \
        } \
        uint32_t refcycles = 0; int refruns = 0; \
        RUNREF(__VA_ARGS__); \
        for (int ti = 0; ti < BENCH_RUNS / 4; ti++) { \
            uint32_t t0 = (uint32_t)__rdtsc(); \
            RUNREF(__VA_ARGS__); \
            RUNREF(__VA_ARGS__); \
            RUNREF(__VA_ARGS__); \
            RUNREF(__VA_ARGS__); \
            uint32_t t1 = (uint32_t)__rdtsc() - t0; \
            if (t1 * refruns <= refcycles * 4 && ti > 0) { refcycles += t1; refruns++; } \
        } \
        x265_emms(); \
        float optperf = (10.0f * cycles / runs) / 4; \
        float refperf = (10.0f * refcycles / refruns) / 4; \
        printf("\t%3.2fx ", refperf / optperf); \
        printf("\t %-4.2lf \t %-4.2lf\n", optperf, refperf); \
    }

#endif // ifndef _TESTHARNESS_H_
