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

using namespace x265;

MBDstHarness::MBDstHarness()
{
    mb_t_size = 32;

    mbuf1 = (short*)malloc(0x1e00 * sizeof(pixel));
    mbuf2 = (short*)malloc(mb_t_size);
    mbuf3 = (short*)malloc(mb_t_size);
    if (!mbuf1 || !mbuf2 || !mbuf3)
    {
        fprintf(stderr, "malloc failed, unable to initiate tests!\n");
        exit(1);
    }

    for (int i = 0; i < 64*100; i++)
        mbuf1[i] = rand() & PIXEL_MAX;
    memset(mbuf2, 0, mb_t_size);
    memset(mbuf3, 0, mb_t_size);
}

MBDstHarness::~MBDstHarness()
{
    free(mbuf1);
    free(mbuf2);
    free(mbuf3);
}

bool MBDstHarness::check_mbdst_primitive(mbdst ref, mbdst opt)
{
    int j = 0;

    for (int i = 0; i <= 100; i++)
    {
        opt(mbuf1 + j, mbuf2, 16);
        ref(mbuf1 + j, mbuf3, 16);

        if (memcmp(mbuf2, mbuf3, mb_t_size))
            return false;

        j += 16;
        memset(mbuf2, 0, mb_t_size);
        memset(mbuf3, 0, mb_t_size);
    }

    return true;
}

bool MBDstHarness::testCorrectness( const EncoderPrimitives& ref, const EncoderPrimitives& opt )
{
    if (opt.inversedst)
    {
        if (!check_mbdst_primitive(ref.inversedst, opt.inversedst))
        {
            printf("Inversedst: Failed!\n");
            return false;
        }
    }

    return true;
}

#define MBDST_ITERATIONS 4000000

void MBDstHarness::measureSpeed( const EncoderPrimitives& ref, const EncoderPrimitives& opt )
{
    Timer *t = Timer::CreateTimer();

    if (opt.inversedst)
    {
        t->Start();
        for (int j = 0; j < MBDST_ITERATIONS; j++)
            opt.inversedst(mbuf1, mbuf2, 16);
        t->Stop();
        printf("\nInverseDST\tVec: (%1.2f ms) ", t->ElapsedMS());

        t->Start();
        for (int j = 0; j < MBDST_ITERATIONS; j++)
            ref.inversedst(mbuf1, mbuf2, 16);
        t->Stop();
        printf("\tC: (%1.2f ms) ", t->ElapsedMS());
    }

    t->Release();
}
