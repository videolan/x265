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
    ip_t_size = 4 * 65 * 65 * 100;
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

    pixel_out_C   = (pixel*)malloc(ip_t_size * sizeof(pixel));
    pixel_out_Vec = (pixel*)malloc(ip_t_size * sizeof(pixel));

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
}

bool IntraPredHarness::check_getIPredDC_primitive(x265::getIPredDC_p ref, x265::getIPredDC_p opt)
{
    int j = ADI_BUF_STRIDE;

    for (int i = 0; i <= 100; i++)
    {
        int blkAboveAvailable = rand() & 1;
        int blkLeftAvailable = rand() & 1;
        int rand_width = 1 << ((rand() % 5) + 2);                  // Randomly generated Width
        int rand_filter = rand() & 1;

        // The Left and Above can't both be false
        if (!blkLeftAvailable)
            blkAboveAvailable = 1;

        memset(pixel_out_Vec, 0xCD, ip_t_size);      // Initialize output buffer to zero
        memset(pixel_out_C, 0xCD, ip_t_size);        // Initialize output buffer to zero

        opt(pixel_buff + j, ADI_BUF_STRIDE, pixel_out_Vec, FENC_STRIDE, rand_width, rand_width, blkAboveAvailable, blkLeftAvailable, rand_filter);
        ref(pixel_buff + j, ADI_BUF_STRIDE, pixel_out_C,   FENC_STRIDE, rand_width, rand_width, blkAboveAvailable, blkLeftAvailable, rand_filter);

        for (int k = 0; k < rand_width; k++)
        {
            if (memcmp(pixel_out_Vec + k * FENC_STRIDE, pixel_out_C + k * FENC_STRIDE, rand_width))
                return false;
        }

        j += FENC_STRIDE;
    }

    return true;
}

bool IntraPredHarness::check_getIPredPlanar_primitive(x265::getIPredPlanar_p ref, x265::getIPredPlanar_p opt)
{
    int j = ADI_BUF_STRIDE;

    for (int width = 4; width <= 16; width <<= 1)
    {
        for (int i = 0; i <= 100; i++)
        {
            int blkAboveAvailable = rand() & 1;
            int blkLeftAvailable = rand() & 1;
            int rand_width = width;

            // The Left and Above can't both be false
            if (!blkLeftAvailable)
                blkAboveAvailable = 1;

            memset(pixel_out_Vec, 0xCD, ip_t_size);  // Initialize output buffer to zero
            memset(pixel_out_C, 0xCD, ip_t_size);    // Initialize output buffer to zero

            opt(pixel_buff + j, ADI_BUF_STRIDE, pixel_out_Vec, FENC_STRIDE, rand_width, 0);
            ref(pixel_buff + j, ADI_BUF_STRIDE, pixel_out_C,   FENC_STRIDE, rand_width, 0);

            for (int k = 0; k < rand_width; k++)
            {
                if (memcmp(pixel_out_Vec + k * FENC_STRIDE, pixel_out_C + k * FENC_STRIDE, rand_width))
                {
                    return false;
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

    return true;
}

#define INTRAPRED_ITERATIONS   50000

void IntraPredHarness::measureSpeed(const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{
    Timer *t = Timer::CreateTimer();

    int width = 64;
    short srcStride = 96;
    int blkAboveAvailable = 1;
    int blkLeftAvailable = 1;

    if (opt.getIPredDC)
    {
        printf("IPred_getIPredDC_pel[filter=0]");
        REPORT_SPEEDUP(INTRAPRED_ITERATIONS,
                       opt.getIPredDC(pixel_buff + srcStride, srcStride,
                                      pixel_out_Vec, FENC_STRIDE,
                                      width, width,
                                      blkAboveAvailable, blkLeftAvailable, 0),
                       ref.getIPredDC(pixel_buff + srcStride, srcStride,
                                      pixel_out_C, FENC_STRIDE,
                                      width, width,
                                      blkAboveAvailable, blkLeftAvailable, 0)
                       );
        printf("IPred_getIPredDC_pel[filter=1]");
        REPORT_SPEEDUP(INTRAPRED_ITERATIONS,
                       opt.getIPredDC(pixel_buff + srcStride, srcStride,
                                      pixel_out_Vec, FENC_STRIDE,
                                      width, width,
                                      blkAboveAvailable, blkLeftAvailable, 1),
                       ref.getIPredDC(pixel_buff + srcStride, srcStride,
                                      pixel_out_C, FENC_STRIDE,
                                      width, width,
                                      blkAboveAvailable, blkLeftAvailable, 1)
                       );
    }
    if (opt.getIPredPlanar)
    {
        for (int ii = 4; ii <= 16; ii <<= 1)
        {
            width = ii;
            printf("IPred_getIPredPlanar[width=%d]", ii);
            REPORT_SPEEDUP(INTRAPRED_ITERATIONS,
                           opt.getIPredPlanar(pixel_buff + srcStride, srcStride,
                                              pixel_out_Vec, FENC_STRIDE,
                                              width, 0),
                           ref.getIPredPlanar(pixel_buff + srcStride, srcStride,
                                              pixel_out_C, FENC_STRIDE,
                                              width, 0)
                           );
        }
    }

    t->Release();
}
