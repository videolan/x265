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
#include <intrin.h>
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

class Timer
{
public:

    Timer() {}

    virtual ~Timer() {}

    static Timer *CreateTimer();

    virtual void Start() = 0;

    virtual void Stop() = 0;

    virtual uint64_t Elapsed() = 0;

    virtual void Release() = 0;
};

static inline uint64_t read_time(void)
{
    unsigned __int64 i=0;
#ifdef _MSC_VER    
    i = __rdtsc();
#endif
    return i;
}

#define REPORT_SPEEDUP(ITERATIONS, RUNOPT, RUNREF) \
{ \
    t->Start(); unsigned __int64 ticks1 = read_time(); for (int X=0; X < ITERATIONS; X++) { RUNOPT; } unsigned __int64 ticks2 = read_time(); t->Stop(); \
    uint64_t optelapsed = t->Elapsed(); \
    uint64_t optelapsed2 = ticks2 - ticks1; \
    ticks1 = read_time(); t->Start(); for (int X=0; X < ITERATIONS; X++) { RUNREF; } t->Stop(); ticks2 = read_time(); \
    uint64_t refelapsed = t->Elapsed(); \
    uint64_t refelapsed2 = ticks2 - ticks1; \
    printf("\t%3.2fx ", (double)refelapsed/optelapsed ); \
    printf("\t %.2lf \t %.2lf\n", (double)optelapsed2/ITERATIONS, (double)refelapsed2/ITERATIONS); \
}

#endif // ifndef _TESTHARNESS_H_
