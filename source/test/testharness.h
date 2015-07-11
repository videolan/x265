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
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#ifndef _TESTHARNESS_H_
#define _TESTHARNESS_H_ 1

#include "common.h"
#include "primitives.h"

#if _MSC_VER
#pragma warning(disable: 4324) // structure was padded due to __declspec(align())
#endif

#define PIXEL_MAX ((1 << X265_DEPTH) - 1)
#define PIXEL_MIN 0
#define SHORT_MAX  32767
#define SHORT_MIN -32767
#define UNSIGNED_SHORT_MAX 65535

using namespace X265_NS;

extern const char* lumaPartStr[NUM_PU_SIZES];
extern const char* const* chromaPartStr[X265_CSP_COUNT];

class TestHarness
{
public:

    TestHarness() {}

    virtual ~TestHarness() {}

    virtual bool testCorrectness(const EncoderPrimitives& ref, const EncoderPrimitives& opt) = 0;

    virtual void measureSpeed(const EncoderPrimitives& ref, const EncoderPrimitives& opt) = 0;

    virtual const char *getName() const = 0;

protected:

    /* Temporary variables for stack checks */
    int      m_ok;

    uint64_t m_rand;
};

#ifdef _MSC_VER
#include <intrin.h>
#elif HAVE_RDTSC
#include <intrin.h>
#elif defined(__GNUC__)
/* fallback for older GCC/MinGW */
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
        printf("\t %-8.2lf \t %-8.2lf\n", optperf, refperf); \
    }

extern "C" {
#if X265_ARCH_X86
int PFX(stack_pagealign)(int (*func)(), int align);

/* detect when callee-saved regs aren't saved
 * needs an explicit asm check because it only sometimes crashes in normal use. */
intptr_t PFX(checkasm_call)(intptr_t (*func)(), int *ok, ...);
float PFX(checkasm_call_float)(float (*func)(), int *ok, ...);
#else
#define PFX(stack_pagealign)(func, align) func()
#endif

#if X86_64

/* Evil hack: detect incorrect assumptions that 32-bit ints are zero-extended to 64-bit.
 * This is done by clobbering the stack with junk around the stack pointer and calling the
 * assembly function through x265_checkasm_call with added dummy arguments which forces all
 * real arguments to be passed on the stack and not in registers. For 32-bit argument the
 * upper half of the 64-bit register location on the stack will now contain junk. Note that
 * this is dependent on compiler behavior and that interrupts etc. at the wrong time may
 * overwrite the junk written to the stack so there's no guarantee that it will always
 * detect all functions that assumes zero-extension.
 */
void PFX(checkasm_stack_clobber)(uint64_t clobber, ...);
#define checked(func, ...) ( \
        m_ok = 1, m_rand = (rand() & 0xffff) * 0x0001000100010001ULL, \
        PFX(checkasm_stack_clobber)(m_rand, m_rand, m_rand, m_rand, m_rand, m_rand, m_rand, m_rand, \
                                    m_rand, m_rand, m_rand, m_rand, m_rand, m_rand, m_rand, m_rand, \
                                    m_rand, m_rand, m_rand, m_rand, m_rand), /* max_args+6 */ \
        PFX(checkasm_call)((intptr_t(*)())func, &m_ok, 0, 0, 0, 0, __VA_ARGS__))

#define checked_float(func, ...) ( \
        m_ok = 1, m_rand = (rand() & 0xffff) * 0x0001000100010001ULL, \
        PFX(checkasm_stack_clobber)(m_rand, m_rand, m_rand, m_rand, m_rand, m_rand, m_rand, m_rand, \
                                    m_rand, m_rand, m_rand, m_rand, m_rand, m_rand, m_rand, m_rand, \
                                    m_rand, m_rand, m_rand, m_rand, m_rand), /* max_args+6 */ \
        PFX(checkasm_call_float)((float(*)())func, &m_ok, 0, 0, 0, 0, __VA_ARGS__))
#define reportfail() if (!m_ok) { fflush(stdout); fprintf(stderr, "stack clobber check failed at %s:%d", __FILE__, __LINE__); abort(); }
#elif ARCH_X86
#define checked(func, ...) PFX(checkasm_call)((intptr_t(*)())func, &m_ok, __VA_ARGS__);
#define checked_float(func, ...) PFX(checkasm_call_float)((float(*)())func, &m_ok, __VA_ARGS__);

#else // if X86_64
#define checked(func, ...) func(__VA_ARGS__)
#define checked_float(func, ...) func(__VA_ARGS__)
#define reportfail()
#endif // if X86_64
}

#endif // ifndef _TESTHARNESS_H_
