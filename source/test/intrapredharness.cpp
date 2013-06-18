/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Min Chen <chenm003@163.com>
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

#include "intrapredharness.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "TLibCommon/TComRom.h"

using namespace x265;

IntraPredHarness::IntraPredHarness()
{
    pixel_buff = (pixel*)malloc(ip_t_size * sizeof(pixel));     // Assuming max_height = max_width = max_srcStride = max_dstStride = 100

    if (!pixel_buff)
    {
        fprintf(stderr, "init_IntraPred_buffers: malloc failed, unable to initiate tests!\n");
        exit(-1);
    }

    for (int i = 0; i < ip_t_size; i++)                         // Initialize input buffer
    {
        pixel_buff[i] = rand() % PIXEL_MAX;
    }

    pixel_out_C   = (pixel*)malloc(out_size * sizeof(pixel));
    pixel_out_Vec = (pixel*)malloc(out_size * sizeof(pixel));
    pixel_out_33_C   = (pixel*)TestHarness::alignedMalloc(sizeof(pixel), out_size_33, 32);
    pixel_out_33_Vec = (pixel*)TestHarness::alignedMalloc(sizeof(pixel), out_size_33, 32);

    if (!pixel_out_C || !pixel_out_Vec)
    {
        fprintf(stderr, "init_IntraPred_buffers: malloc failed, unable to initiate tests!\n");
        exit(-1);
    }

    initROM();
}

IntraPredHarness::~IntraPredHarness()
{
    free(pixel_buff);
    free(pixel_out_C);
    free(pixel_out_Vec);
    TestHarness::alignedFree(pixel_out_33_C);
    TestHarness::alignedFree(pixel_out_33_Vec);
}

bool IntraPredHarness::check_getIPredDC_primitive(x265::getIPredDC_t ref, x265::getIPredDC_t opt)
{
    int j = ADI_BUF_STRIDE;

    for (int i = 0; i <= 100; i++)
    {
        int rand_width = 1 << ((rand() % 5) + 2);                  // Randomly generated Width
        int rand_filter = rand() & 1;

#if _DEBUG
        memset(pixel_out_Vec, 0xCD, out_size);
        memset(pixel_out_C, 0xCD, out_size);
#endif

        opt(pixel_buff + j, ADI_BUF_STRIDE, pixel_out_Vec, FENC_STRIDE, rand_width, rand_filter);
        ref(pixel_buff + j, ADI_BUF_STRIDE, pixel_out_C,   FENC_STRIDE, rand_width, rand_filter);

        for (int k = 0; k < rand_width; k++)
        {
            if (memcmp(pixel_out_Vec + k * FENC_STRIDE, pixel_out_C + k * FENC_STRIDE, rand_width))
                return false;
        }

        j += FENC_STRIDE;
    }

    return true;
}

bool IntraPredHarness::check_getIPredPlanar_primitive(x265::getIPredPlanar_t ref, x265::getIPredPlanar_t opt)
{
    int j = ADI_BUF_STRIDE;

    for (int width = 4; width <= 64; width <<= 1)
    {
        for (int i = 0; i <= 100; i++)
        {
#if _DEBUG
            memset(pixel_out_Vec, 0xCD, out_size);
            memset(pixel_out_C, 0xCD, out_size);
#endif
            ref(pixel_buff + j, ADI_BUF_STRIDE, pixel_out_C,   FENC_STRIDE, width);
            opt(pixel_buff + j, ADI_BUF_STRIDE, pixel_out_Vec, FENC_STRIDE, width);

            for (int k = 0; k < width; k++)
            {
                if (memcmp(pixel_out_Vec + k * FENC_STRIDE, pixel_out_C + k * FENC_STRIDE, width))
                {
                    return false;
                }
            }

            j += FENC_STRIDE;
        }
    }

    return true;
}

bool IntraPredHarness::check_getIPredAng_primitive(x265::getIPredAng_p ref, x265::getIPredAng_p opt)
{
    int j = ADI_BUF_STRIDE;

    int pmode;
    Bool bFilter;

    for (int width = 4; width <= 32; width <<= 1)
    {
        for (int i = 0; i <= 100; i++)
        {
            bFilter = (width <= 16) && (rand()%2);
            for (int p = 2; p <= 34; p++)
            {
                pmode = p;

#if _DEBUG
                memset(pixel_out_Vec, 0xCD, out_size);
                memset(pixel_out_C, 0xCD, out_size);
#endif
                pixel * refAbove = pixel_buff + j;
                pixel * refLeft = refAbove + 3 * width;
                refLeft[0] = refAbove[0];

                opt(BIT_DEPTH, pixel_out_Vec, FENC_STRIDE, width, pmode, bFilter, refAbove, refLeft);
                ref(BIT_DEPTH, pixel_out_C, FENC_STRIDE, width, pmode, bFilter, refAbove, refLeft);

                for (int k = 0; k < width; k++)
                {
                    if (memcmp(pixel_out_Vec + k * FENC_STRIDE, pixel_out_C + k * FENC_STRIDE, width))
                    {
                        printf("\nFailed for width %d mode %d bfilter %d row %d \t",width, p, bFilter, k);
                        return false;
                    }
                }
            }

            j += FENC_STRIDE;
        }
    }

    return true;
}

bool IntraPredHarness::check_getIPredAngs4_primitive(x265::getIPredAngs_t ref, x265::getIPredAngs_t opt)
{
    int j = ADI_BUF_STRIDE;

    Bool isLuma;

    for (int width = 4; width <= 4; width <<= 1)
    {
        for (int i = 0; i <= 100; i++)
        {
            isLuma = (width <= 16) && (rand()%2);

            pixel * refAbove0 = pixel_buff + j;
            pixel * refLeft0 = refAbove0 + 3 * width;
            refLeft0[0] = refAbove0[0];

            pixel * refAbove1 = pixel_buff + j + FENC_STRIDE;
            pixel * refLeft1 = refAbove1 + 3 * width + FENC_STRIDE;
            refLeft1[0] = refAbove1[0];

#if _DEBUG
            memset(pixel_out_33_Vec, 0xCD, out_size);
            memset(pixel_out_33_C, 0xCD, out_size);
#endif

            ref(pixel_out_33_C,   refAbove0, refLeft0, refAbove1, refLeft1, isLuma);
            opt(pixel_out_33_Vec, refAbove0, refLeft0, refAbove1, refLeft1, isLuma);
            for (int p = 2-2; p <= 34-2; p++)
            {
                for (int k = 0; k < width; k++)
                {
                    if (memcmp(pixel_out_33_C + p * (width *width) + k * width, pixel_out_33_Vec + p * (width *width) + k * width, width))
                    {
                        printf("\nFailed: [%2d]: width=%d, mode=%d, bfilter=%d\n", k, width, p+2, isLuma);
                        return false;
                    }
                }
            }

            j += FENC_STRIDE;
        }
    }

    return true;
}

bool IntraPredHarness::testCorrectness(const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{
    if (opt.getIPredDC)
    {
        if (!check_getIPredDC_primitive(ref.getIPredDC, opt.getIPredDC))
        {
            printf("intrapred_getIPredDC_pel failed\n");
            return false;
        }
    }
    if (opt.getIPredPlanar)
    {
        if (!check_getIPredPlanar_primitive(ref.getIPredPlanar, opt.getIPredPlanar))
        {
            printf("intrapred_planar_pel failed\n");
            return false;
        }
    }
    if (opt.getIPredAng)
    {
        if (!check_getIPredAng_primitive(ref.getIPredAng, opt.getIPredAng))
        {
            printf("intrapred_angular_pel failed\n");
            return false;
        }
    }
    if (opt.getIPredAngs4)
    {
        if (!check_getIPredAngs4_primitive(ref.getIPredAngs4, opt.getIPredAngs4))
        {
            printf("intrapred_angular_4x4_33_modes failed\n");
            return false;
        }
    }

    return true;
}

void IntraPredHarness::measureSpeed(const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{
    int width = 64;
    short srcStride = 96;

    if (opt.getIPredDC)
    {
        printf("IPred_getIPredDC_pel[filter=0]");
        REPORT_SPEEDUP(opt.getIPredDC, ref.getIPredDC,
                       pixel_buff + srcStride, srcStride, pixel_out_Vec, FENC_STRIDE, width, 0);
        printf("IPred_getIPredDC_pel[filter=1]");
        REPORT_SPEEDUP(opt.getIPredDC, ref.getIPredDC,
                       pixel_buff + srcStride, srcStride, pixel_out_Vec, FENC_STRIDE, width, 1);
    }
    if (opt.getIPredPlanar)
    {
        for (int ii = 4; ii <= 64; ii <<= 1)
        {
            width = ii;
            printf("IPred_getIPredPlanar[width=%d]", ii);
            REPORT_SPEEDUP(opt.getIPredPlanar, ref.getIPredPlanar,
                           pixel_buff + srcStride, srcStride, pixel_out_Vec, FENC_STRIDE, width);
        }
    }
    if (opt.getIPredAng)
    {
        for (int ii = 4; ii <= 32; ii <<= 1)
        {
            for (int p = 2; p <= 34; p += 1)
            {
                width = ii;
                bool bFilter  = (width <= 16);
                pixel * refAbove = pixel_buff + srcStride;
                pixel * refLeft = refAbove + 3 * width;
                refLeft[0] = refAbove[0];
                int pmode = p;  //(rand()%33)+2;
                printf("IPred_getIPredAng[width=%d][mode=%d]", ii, pmode);
                REPORT_SPEEDUP(opt.getIPredAng, ref.getIPredAng,
                               BIT_DEPTH, pixel_out_Vec, FENC_STRIDE, width, pmode, bFilter, refAbove, refLeft);
            }
        }
    }
    if (opt.getIPredAngs4)
    {
        for (int ii = 4; ii <= 4; ii <<= 1)
        {
            width = ii;
            bool bFilter  = (width <= 16);
            pixel * refAbove = pixel_buff + srcStride;
            pixel * refLeft = refAbove + 3 * width;
            refLeft[0] = refAbove[0];
            printf("IPred_getIPredAngs4\t\t");
            REPORT_SPEEDUP(opt.getIPredAngs4, ref.getIPredAngs4,
                           pixel_out_33_Vec, refAbove, refLeft, refAbove, refLeft, bFilter);
        }
    }
}
