/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Loren Merritt <lorenm@u.washington.edu>
 *          Laurent Aimar <fenrir@via.ecp.fr>
 *          Jason Garrett-Glaser <darkshikari@gmail.com>
 *          Steve Borho <steve@borho.org>
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

#include "cpu.h"
#include "common.h"

#include <cstring>
#include <assert.h>

#if MACOS || SYS_FREEBSD
#include <sys/types.h>
#include <sys/sysctl.h>
#endif
#if SYS_OPENBSD
#include <sys/param.h>
#include <sys/sysctl.h>
#include <machine/cpu.h>
#endif

namespace x265 {
const cpu_name_t cpu_names[] =
{
#define MMX2 X265_CPU_MMX | X265_CPU_MMX2 | X265_CPU_CMOV
    { "MMX2",        MMX2 },
    { "MMXEXT",      MMX2 },
    { "SSE",         MMX2 | X265_CPU_SSE },
#define SSE2 MMX2 | X265_CPU_SSE | X265_CPU_SSE2
    { "SSE2Slow",    SSE2 | X265_CPU_SSE2_IS_SLOW },
    { "SSE2",        SSE2 },
    { "SSE2Fast",    SSE2 | X265_CPU_SSE2_IS_FAST },
    { "SSE3",        SSE2 | X265_CPU_SSE3 },
    { "SSSE3",       SSE2 | X265_CPU_SSE3 | X265_CPU_SSSE3 },
    { "SSE4.1",      SSE2 | X265_CPU_SSE3 | X265_CPU_SSSE3 | X265_CPU_SSE4 },
    { "SSE4",        SSE2 | X265_CPU_SSE3 | X265_CPU_SSSE3 | X265_CPU_SSE4 },
    { "SSE4.2",      SSE2 | X265_CPU_SSE3 | X265_CPU_SSSE3 | X265_CPU_SSE4 | X265_CPU_SSE42 },
#define AVX SSE2 | X265_CPU_SSE3 | X265_CPU_SSSE3 | X265_CPU_SSE4 | X265_CPU_SSE42 | X265_CPU_AVX
    { "AVX",         AVX },
    { "XOP",         AVX | X265_CPU_XOP },
    { "FMA4",        AVX | X265_CPU_FMA4 },
    { "AVX2",        AVX | X265_CPU_AVX2 },
    { "FMA3",        AVX | X265_CPU_FMA3 },
#undef AVX
#undef SSE2
#undef MMX2
    { "Cache32",         X265_CPU_CACHELINE_32 },
    { "Cache64",         X265_CPU_CACHELINE_64 },
    { "LZCNT",           X265_CPU_LZCNT },
    { "BMI1",            X265_CPU_BMI1 },
    { "BMI2",            X265_CPU_BMI1 | X265_CPU_BMI2 },
    { "SlowCTZ",         X265_CPU_SLOW_CTZ },
    { "SlowAtom",        X265_CPU_SLOW_ATOM },
    { "SlowPshufb",      X265_CPU_SLOW_PSHUFB },
    { "SlowPalignr",     X265_CPU_SLOW_PALIGNR },
    { "SlowShuffle",     X265_CPU_SLOW_SHUFFLE },
    { "UnalignedStack",  X265_CPU_STACK_MOD4 },
    { "", 0 },
};

extern "C" {
/* cpu-a.asm */
int x265_cpu_cpuid_test(void);
void x265_cpu_cpuid(uint32_t op, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx);
void x265_cpu_xgetbv(uint32_t op, uint32_t *eax, uint32_t *edx);
}

#if defined(_MSC_VER)
#pragma warning(disable: 4309) // truncation of constant value
#endif

uint32_t cpu_detect(void)
{
    uint32_t cpu = 0;
    uint32_t eax, ebx, ecx, edx;
    uint32_t vendor[4] = { 0 };
    uint32_t max_extended_cap, max_basic_cap;
    int cache;

#if !X86_64
    if (!x265_cpu_cpuid_test())
        return 0;
#endif

    x265_cpu_cpuid(0, &eax, vendor + 0, vendor + 2, vendor + 1);
    max_basic_cap = eax;
    if (max_basic_cap == 0)
        return 0;

    x265_cpu_cpuid(1, &eax, &ebx, &ecx, &edx);
    if (edx & 0x00800000)
        cpu |= X265_CPU_MMX;
    else
        return cpu;
    if (edx & 0x02000000)
        cpu |= X265_CPU_MMX2 | X265_CPU_SSE;
    if (edx & 0x00008000)
        cpu |= X265_CPU_CMOV;
    else
        return cpu;
    if (edx & 0x04000000)
        cpu |= X265_CPU_SSE2;
    if (ecx & 0x00000001)
        cpu |= X265_CPU_SSE3;
    if (ecx & 0x00000200)
        cpu |= X265_CPU_SSSE3;
    if (ecx & 0x00080000)
        cpu |= X265_CPU_SSE4;
    if (ecx & 0x00100000)
        cpu |= X265_CPU_SSE42;
    /* Check OXSAVE and AVX bits */
    if ((ecx & 0x18000000) == 0x18000000)
    {
        /* Check for OS support */
        x265_cpu_xgetbv(0, &eax, &edx);
        if ((eax & 0x6) == 0x6)
        {
            cpu |= X265_CPU_AVX;
            if (ecx & 0x00001000)
                cpu |= X265_CPU_FMA3;
        }
    }

    if (max_basic_cap >= 7)
    {
        x265_cpu_cpuid(7, &eax, &ebx, &ecx, &edx);
        /* AVX2 requires OS support, but BMI1/2 don't. */
        if ((cpu & X265_CPU_AVX) && (ebx & 0x00000020))
            cpu |= X265_CPU_AVX2;
        if (ebx & 0x00000008)
        {
            cpu |= X265_CPU_BMI1;
            if (ebx & 0x00000100)
                cpu |= X265_CPU_BMI2;
        }
    }

    if (cpu & X265_CPU_SSSE3)
        cpu |= X265_CPU_SSE2_IS_FAST;

    x265_cpu_cpuid(0x80000000, &eax, &ebx, &ecx, &edx);
    max_extended_cap = eax;

    if (max_extended_cap >= 0x80000001)
    {
        x265_cpu_cpuid(0x80000001, &eax, &ebx, &ecx, &edx);

        if (ecx & 0x00000020)
            cpu |= X265_CPU_LZCNT; /* Supported by Intel chips starting with Haswell */
        if (ecx & 0x00000040) /* SSE4a, AMD only */
        {
            int family = ((eax >> 8) & 0xf) + ((eax >> 20) & 0xff);
            cpu |= X265_CPU_SSE2_IS_FAST;      /* Phenom and later CPUs have fast SSE units */
            if (family == 0x14)
            {
                cpu &= ~X265_CPU_SSE2_IS_FAST; /* SSSE3 doesn't imply fast SSE anymore... */
                cpu |= X265_CPU_SSE2_IS_SLOW;  /* Bobcat has 64-bit SIMD units */
                cpu |= X265_CPU_SLOW_PALIGNR;  /* palignr is insanely slow on Bobcat */
            }
            if (family == 0x16)
            {
                cpu |= X265_CPU_SLOW_PSHUFB;   /* Jaguar's pshufb isn't that slow, but it's slow enough
                                                * compared to alternate instruction sequences that this
                                                * is equal or faster on almost all such functions. */
            }
        }

        if (cpu & X265_CPU_AVX)
        {
            if (ecx & 0x00000800) /* XOP */
                cpu |= X265_CPU_XOP;
            if (ecx & 0x00010000) /* FMA4 */
                cpu |= X265_CPU_FMA4;
        }

        if (!strcmp((char*)vendor, "AuthenticAMD"))
        {
            if (edx & 0x00400000)
                cpu |= X265_CPU_MMX2;
            if (!(cpu & X265_CPU_LZCNT))
                cpu |= X265_CPU_SLOW_CTZ;
            if ((cpu & X265_CPU_SSE2) && !(cpu & X265_CPU_SSE2_IS_FAST))
                cpu |= X265_CPU_SSE2_IS_SLOW; /* AMD CPUs come in two types: terrible at SSE and great at it */
        }
    }

    if (!strcmp((char*)vendor, "GenuineIntel"))
    {
        x265_cpu_cpuid(1, &eax, &ebx, &ecx, &edx);
        int family = ((eax >> 8) & 0xf) + ((eax >> 20) & 0xff);
        int model  = ((eax >> 4) & 0xf) + ((eax >> 12) & 0xf0);
        if (family == 6)
        {
            /* 6/9 (pentium-m "banias"), 6/13 (pentium-m "dothan"), and 6/14 (core1 "yonah")
             * theoretically support sse2, but it's significantly slower than mmx for
             * almost all of x264's functions, so let's just pretend they don't. */
            if (model == 9 || model == 13 || model == 14)
            {
                cpu &= ~(X265_CPU_SSE2 | X265_CPU_SSE3);
                assert(!(cpu & (X265_CPU_SSSE3 | X265_CPU_SSE4)));
            }
            /* Detect Atom CPU */
            else if (model == 28)
            {
                cpu |= X265_CPU_SLOW_ATOM;
                cpu |= X265_CPU_SLOW_CTZ;
                cpu |= X265_CPU_SLOW_PSHUFB;
            }

            /* Conroe has a slow shuffle unit. Check the model number to make sure not
             * to include crippled low-end Penryns and Nehalems that don't have SSE4. */
            else if ((cpu & X265_CPU_SSSE3) && !(cpu & X265_CPU_SSE4) && model < 23)
                cpu |= X265_CPU_SLOW_SHUFFLE;
        }
    }

    if ((!strcmp((char*)vendor, "GenuineIntel") || !strcmp((char*)vendor, "CyrixInstead")) && !(cpu & X265_CPU_SSE42))
    {
        /* cacheline size is specified in 3 places, any of which may be missing */
        x265_cpu_cpuid(1, &eax, &ebx, &ecx, &edx);
        cache = (ebx & 0xff00) >> 5; // cflush size
        if (!cache && max_extended_cap >= 0x80000006)
        {
            x265_cpu_cpuid(0x80000006, &eax, &ebx, &ecx, &edx);
            cache = ecx & 0xff; // cacheline size
        }
        if (!cache && max_basic_cap >= 2)
        {
            // Cache and TLB Information
            static const char cache32_ids[] = { 0x0a, 0x0c, 0x41, 0x42, 0x43, 0x44, 0x45, 0x82, 0x83, 0x84, 0x85, 0 };
            static const char cache64_ids[] = { 0x22, 0x23, 0x25, 0x29, 0x2c, 0x46, 0x47, 0x49, 0x60, 0x66, 0x67,
                                                0x68, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7c, 0x7f, 0x86, 0x87, 0 };
            uint32_t buf[4];
            int max, i = 0;
            do
            {
                x265_cpu_cpuid(2, buf + 0, buf + 1, buf + 2, buf + 3);
                max = buf[0] & 0xff;
                buf[0] &= ~0xff;
                for (int j = 0; j < 4; j++)
                {
                    if (!(buf[j] >> 31))
                        while (buf[j])
                        {
                            if (strchr(cache32_ids, buf[j] & 0xff))
                                cache = 32;
                            if (strchr(cache64_ids, buf[j] & 0xff))
                                cache = 64;
                            buf[j] >>= 8;
                        }
                }
            }
            while (++i < max);
        }

        if (cache == 32)
            cpu |= X265_CPU_CACHELINE_32;
        else if (cache == 64)
            cpu |= X265_CPU_CACHELINE_64;
        else
            x265_log(NULL, X265_LOG_WARNING, "unable to determine cacheline size\n");
    }

#if BROKEN_STACK_ALIGNMENT
    cpu |= X265_CPU_STACK_MOD4;
#endif

    return cpu;
}
}
