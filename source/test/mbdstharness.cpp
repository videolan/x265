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

#include "mbdstharness.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>

#ifdef __MINGW32__ 
#define _aligned_malloc __mingw_aligned_malloc 
#define _aligned_free  __mingw_aligned_free 
#endif

using namespace x265;

const char *ButterflyConf_names[] =
{
"4",
"Inverse4",
"8",
"Inverse8",
"16",
"Inverse16",
"32",
"Inverse32"
};

enum Butterflies
{
    butterfly_4,
    butterfly_inverse_4,
    butterfly_8,
    butterfly_inverse_8,
    butterfly_16,
    butterfly_inverse_16,
    butterfly_32,
    butterfly_inverse_32,
    num_butterflies
};

MBDstHarness::MBDstHarness()
{
    mb_t_size = 6400;
#if _WIN32
    mbuf1 = (short*)_aligned_malloc(0x1e00 * sizeof(short), 32);
    mbuf2 = (short*)_aligned_malloc(mb_t_size, 32);
    mbuf3 = (short*)_aligned_malloc(mb_t_size, 32);
#else
    posix_memalign((void **)&mbuf1, 32, 0x1e00 * sizeof(short));
    posix_memalign((void **)&mbuf2, 32, mb_t_size);
    posix_memalign((void **)&mbuf3, 32, mb_t_size);
#endif
    if (!mbuf1 || !mbuf2 || !mbuf3)
    {
        fprintf(stderr, "malloc failed, unable to initiate tests!\n");
        exit(1);
    }

    for (int i = 0; i < 64 * 100; i++)
    {
        mbuf1[i] = rand() & PIXEL_MAX;
    }

    memset(mbuf2, 0, mb_t_size);
    memset(mbuf3, 0, mb_t_size);
}

MBDstHarness::~MBDstHarness()
{
#if _WIN32
    _aligned_free(mbuf1);
    _aligned_free(mbuf2);
    _aligned_free(mbuf3);
#else
    free(mbuf1);
    free(mbuf2);
    free(mbuf3);
#endif
}

bool MBDstHarness::check_mbdst_primitive(mbdst ref, mbdst opt)
{
    int j = 0;
    int mem_cmp_size = 32;

    for (int i = 0; i <= 100; i++)
    {
        opt(mbuf1 + j, mbuf2, 16);
        ref(mbuf1 + j, mbuf3, 16);

        if (memcmp(mbuf2, mbuf3, mem_cmp_size))
            return false;

        j += 16;
        memset(mbuf2, 0, mem_cmp_size);
        memset(mbuf3, 0, mem_cmp_size);
    }

    return true;
}

bool MBDstHarness::check_butterfly16_primitive(butterfly ref, butterfly opt)
{
    int j = 0;
    int mem_cmp_size = 320; // 2*16*10 -> sizeof(short)*number of elements*number of lines

    for (int i = 0; i <= 100; i++)
    {
        opt(mbuf1 + j, mbuf2, 3, 10);
        ref(mbuf1 + j, mbuf3, 3, 10);

        if (memcmp(mbuf2, mbuf3, mem_cmp_size))
            return false;

        j += 16;
        memset(mbuf2, 0, mem_cmp_size);
        memset(mbuf3, 0, mem_cmp_size);
    }

    return true;
}

bool MBDstHarness::testCorrectness(const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{
    if (opt.inversedst)
    {
        if (!check_mbdst_primitive(ref.inversedst, opt.inversedst))
        {
            printf("Inversedst: Failed!\n");
            return false;
        }
    }

    if (opt.partial_butterfly[butterfly_16])
    {
        if (!check_butterfly16_primitive(ref.partial_butterfly[butterfly_16], opt.partial_butterfly[butterfly_16]))
        {
            printf("\npartialButterfly%s failed\n", ButterflyConf_names[butterfly_16]);
            return false;
        }
    }

    return true;
}

#define MBDST_ITERATIONS 4000000

void MBDstHarness::measureSpeed(const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{
    Timer *t = Timer::CreateTimer();

    if (opt.inversedst)
    {
        t->Start();
        for (int j = 0; j < MBDST_ITERATIONS; j++)
        {
            opt.inversedst(mbuf1, mbuf2, 16);
        }

        t->Stop();
        printf("\nInverseDST\tVec: (%1.2f ms) ", t->ElapsedMS());

        t->Start();
        for (int j = 0; j < MBDST_ITERATIONS; j++)
        {
            ref.inversedst(mbuf1, mbuf2, 16);
        }

        t->Stop();
        printf("\tC: (%1.2f ms) ", t->ElapsedMS());
    }

    for (int value = 0; value < 8; value++)
    {
        memset(mbuf2, 0, mb_t_size); // Initialize output buffer to zero
        memset(mbuf3, 0, mb_t_size); // Initialize output buffer to zero
        if (opt.partial_butterfly[value])
        {
            t->Start();
            for (int j = 0; j < MBDST_ITERATIONS; j++)
            {
                opt.partial_butterfly[value](mbuf1, mbuf2, 3, 10);
            }

            t->Stop();
            printf("\npartialButterfly%s\tVec: (%1.2f ms) ", ButterflyConf_names[value], t->ElapsedMS());

            t->Start();
            for (int j = 0; j < MBDST_ITERATIONS; j++)
            {
                ref.partial_butterfly[value](mbuf1, mbuf2, 3, 10);
            }

            t->Stop();
            printf("\tC: (%1.2f ms) ", t->ElapsedMS());
        }
    }

    t->Release();
}
