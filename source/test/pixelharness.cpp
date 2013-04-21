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

#include "pixelharness.h"
#include "primitives.h"
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#ifdef __MINGW32__
#define _aligned_malloc __mingw_aligned_malloc
#define _aligned_free  __mingw_aligned_free
#endif

using namespace x265;

// Initialize the Func Names for all the Pixel Comp
static const char *FuncNames[NUM_PARTITIONS] =
{
    "4x4", "8x4", "4x8", "8x8", "4x16", "16x4", "8x16", "16x8", "16x16",
    "4x32", "32x4", "8x32", "32x8", "16x32", "32x16", "32x32", "4x64",
    "64x4", "8x64", "64x8", "16x64", "64x16", "32x64", "64x32", "64x64"
};

#if HIGH_BIT_DEPTH
#define BIT_DEPTH 10
#else
#define BIT_DEPTH 8
#endif

#define PIXEL_MAX ((1 << BIT_DEPTH) - 1)

PixelHarness::PixelHarness()
{
#if _WIN32
    pbuf1 = (pixel*)_aligned_malloc(0x1e00 * sizeof(pixel), 32);
    pbuf2 = (pixel*)_aligned_malloc(0x1e00 * sizeof(pixel), 32);
#else
    posix_memalign((void**)&pbuf1, 32, 0x1e00 * sizeof(pixel));
    posix_memalign((void**)&pbuf2, 32, 0x1e00 * sizeof(pixel));
#endif
    if (!pbuf1 || !pbuf2)
    {
        fprintf(stderr, "malloc failed, unable to initiate tests!\n");
        exit(1);
    }

    for (int i = 0; i < 0x1e00; i++)
    {
        //Generate the Random Buffer for Testing
        pbuf1[i] = rand() & PIXEL_MAX;
        pbuf2[i] = rand() & PIXEL_MAX;
    }
}

PixelHarness::~PixelHarness()
{
#if _WIN32
    _aligned_free(pbuf1);
    _aligned_free(pbuf2);
#else
    free(pbuf1);
    free(pbuf2);
#endif
}

#define INCR 16
#define STRIDE 16

bool PixelHarness::check_pixel_primitive(pixelcmp ref, pixelcmp opt)
{
    int j = 0;

    for (int i = 0; i <= 100; i++)
    {
        int vres = opt(pbuf1, STRIDE, pbuf2 + j, STRIDE);
        int cres = ref(pbuf1, STRIDE, pbuf2 + j, STRIDE);
        if (vres != cres)
            return false;

        j += INCR;
    }

    return true;
}

bool PixelHarness::testCorrectness(const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{
    for (uint16_t curpar = 0; curpar < NUM_PARTITIONS; curpar++)
    {
        if (opt.satd[curpar])
        {
            if (!check_pixel_primitive(ref.satd[curpar], opt.satd[curpar]))
            {
                printf("satd[%s]: failed!\n", FuncNames[curpar]);
                return false;
            }
        }

        if (opt.sad[curpar])
        {
            if (!check_pixel_primitive(ref.sad[curpar], opt.sad[curpar]))
            {
                printf("sad[%s]: failed!\n", FuncNames[curpar]);
                return false;
            }
        }
    }

    if (opt.sa8d_8x8)
    {
        if (!check_pixel_primitive(ref.sa8d_8x8, opt.sa8d_8x8))
        {
            printf("sa8d_8x8: failed!\n");
            return false;
        }
    }

    if (opt.sa8d_16x16)
    {
        if (!check_pixel_primitive(ref.sa8d_16x16, opt.sa8d_16x16))
        {
            printf("sa8d_16x16: failed!\n");
            return false;
        }
    }

    return true;
}

#define PIXELCMP_ITERATIONS 2000000

void PixelHarness::measureSpeed(const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{
    Timer *t = Timer::CreateTimer();

    for (int curpar = 0; curpar < NUM_PARTITIONS; curpar++)
    {
        if (opt.satd[curpar])
        {
            printf("\nsatd[%s]\topt: ", FuncNames[curpar]);

            t->Start();
            for (int j = 0; j < PIXELCMP_ITERATIONS; j++)
            {
                opt.satd[curpar](pbuf1, STRIDE, pbuf2, STRIDE);
            }

            t->Stop();
            printf("(%1.2f ms)\t", t->ElapsedMS());

            t->Start();
            for (int j = 0; j < PIXELCMP_ITERATIONS; j++)
            {
                ref.satd[curpar](pbuf1, STRIDE, pbuf2, STRIDE);
            }

            t->Stop();
            printf("C: (%1.2f ms)", t->ElapsedMS());
        }

        if (opt.sad[curpar])
        {
            printf("\nsad[%s]\topt: ", FuncNames[curpar]);

            t->Start();
            for (int j = 0; j < PIXELCMP_ITERATIONS; j++)
            {
                opt.sad[curpar](pbuf1, STRIDE, pbuf2, STRIDE);
            }

            t->Stop();
            printf("(%1.2f ms)\t", t->ElapsedMS());

            t->Start();
            for (int j = 0; j < PIXELCMP_ITERATIONS; j++)
            {
                ref.sad[curpar](pbuf1, STRIDE, pbuf2, STRIDE);
            }

            t->Stop();
            printf("C: (%1.2f ms) ", t->ElapsedMS());
        }
    }

    if (opt.sa8d_8x8)
    {
        t->Start();
        for (int j = 0; j < PIXELCMP_ITERATIONS; j++)
        {
            opt.sa8d_8x8(pbuf1, STRIDE, pbuf2, STRIDE);
        }

        t->Stop();
        printf("\nsa8d_8x8\tVec: (%1.2f ms) ", t->ElapsedMS());

        t->Start();
        for (int j = 0; j < PIXELCMP_ITERATIONS; j++)
        {
            ref.sa8d_8x8(pbuf1, STRIDE, pbuf2, STRIDE);
        }

        t->Stop();
        printf("\tC: (%1.2f ms) ", t->ElapsedMS());
    }

    if (opt.sa8d_16x16)
    {
        t->Start();
        for (int j = 0; j < PIXELCMP_ITERATIONS; j++)
        {
            opt.sa8d_16x16(pbuf1, STRIDE, pbuf2, STRIDE);
        }

        t->Stop();
        printf("\nsa8d_16x16\tVec: (%1.2f ms) ", t->ElapsedMS());

        t->Start();
        for (int j = 0; j < PIXELCMP_ITERATIONS; j++)
        {
            ref.sa8d_16x16(pbuf1, STRIDE, pbuf2, STRIDE);
        }

        t->Stop();
        printf("\tC: (%1.2f ms) ", t->ElapsedMS());
    }

    t->Release();
}
