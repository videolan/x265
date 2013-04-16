/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Siva Viswanathan <siva@multicorewareinc.com>
 *          Praveen Tiwari <praveen@multicorewareinc.com>
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

#include "butterflyharness.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

ButterflyHarness::ButterflyHarness()
{
    bb_t_size = 6400;

    bbuf1 = (short*)malloc(0x1e00 * sizeof(short));
    bbuf2 = (short*)malloc(bb_t_size);
    bbuf3 = (short*)malloc(bb_t_size);
    if (!bbuf1 || !bbuf2 || !bbuf3)
    {
        fprintf(stderr, "malloc failed, unable to initiate tests!\n");
        exit(1);
    }

    for (int i = 0; i < 64 * 100; i++)
    {
        bbuf1[i] = rand() & PIXEL_MAX;
    }

    memset(bbuf2, 0, bb_t_size);
    memset(bbuf3, 0, bb_t_size);
}

ButterflyHarness::~ButterflyHarness()
{
    free(bbuf1);
    free(bbuf2);
    free(bbuf3);
}

bool ButterflyHarness::check_butterfly16_primitive(butterfly ref, butterfly opt)
{
    int j = 0;
    int mem_cmp_size = 320; // 2*16*10 -> sizeof(short)*number of elements*number of lines

    for (int i = 0; i <= 100; i++)
    {
        opt(bbuf1 + j, bbuf2, 3, 10);
        ref(bbuf1 + j, bbuf3, 3, 10);

        if (memcmp(bbuf2, bbuf3, mem_cmp_size))
            return false;

        j += 16;
        memset(bbuf2, 0, mem_cmp_size);
        memset(bbuf3, 0, mem_cmp_size);
    }

    return true;
}

bool ButterflyHarness::testCorrectness(const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{
    for (int value = 0; value < 8; value++)
    {
        if (opt.partial_butterfly[value])
        {
            if (!check_butterfly16_primitive(ref.partial_butterfly[value], opt.partial_butterfly[value]))
            {
                printf("\npartialButterfly%s failed\n", ButterflyConf_names[value]);
                return false;
            }
        }
    }

    return true;
}

#define BUTTERFLY_ITERATIONS 4000000

void ButterflyHarness::measureSpeed(const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{
    Timer *t = Timer::CreateTimer();

    for (int value = 0; value < 8; value++)
    {
        memset(bbuf2, 0, bb_t_size); // Initialize output buffer to zero
        memset(bbuf3, 0, bb_t_size); // Initialize output buffer to zero
        if (opt.partial_butterfly[value])
        {
            t->Start();
            for (int j = 0; j < BUTTERFLY_ITERATIONS; j++)
            {
                opt.partial_butterfly[value](bbuf1, bbuf2, 3, 10);
            }

            t->Stop();
            printf("\npartialButterfly%s\tVec: (%1.2f ms) ", ButterflyConf_names[value], t->ElapsedMS());

            t->Start();
            for (int j = 0; j < BUTTERFLY_ITERATIONS; j++)
            {
                ref.partial_butterfly[value](bbuf1, bbuf2, 3, 10);
            }

            t->Stop();
            printf("\tC: (%1.2f ms) ", t->ElapsedMS());
        }
    }

    t->Release();
}
