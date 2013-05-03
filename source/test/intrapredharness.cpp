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

using namespace x265;

IntraPredHarness::IntraPredHarness()
{
    ip_t_size = 4 * 65 * 65;
    pixel_buff = (pixel*)malloc(ip_t_size * sizeof(pixel));     // Assuming max_height = max_width = max_srcStride = max_dstStride = 100

    if (!pixel_buff)
    {
        fprintf(stderr, "init_IntraPred_buffers: malloc failed, unable to initiate tests!\n");
        exit(-1);
    }

    for (int i = 0; i < ip_t_size; i++)                         // Initialize input buffer
    {
        pixel_buff[i] = (pixel)(rand() &  ((1 << 8) - 1));
    }
}

IntraPredHarness::~IntraPredHarness()
{
    free(pixel_buff);
}

bool IntraPredHarness::check_getDCVal_p_primitive(x265::getDCVal_p ref, x265::getDCVal_p opt)
{
    int rand_width = 1 << ((rand() % 5) + 2);                  // Randomly generated Width

    for (int i = 0; i <= 100; i++)
    {
        int rand_srcStride = rand() % 100;              // Randomly generated srcStride
        int blkAboveAvailable = rand() & 1;
        int blkLeftAvailable = rand() & 1;

        for (int i = 0; i < ip_t_size; i++)                         // fill input buffer with random value
        {
            pixel_buff[i] = (pixel)(rand() &  ((1 << 8) - 1));
        }

        // The Left and Above can't false both
        if (!blkLeftAvailable)
            blkAboveAvailable = 1;

        pixel val_o = opt(pixel_buff, rand_srcStride, rand_width, rand_width, blkAboveAvailable, blkLeftAvailable);
        pixel val_r = ref(pixel_buff, rand_srcStride, rand_width, rand_width, blkAboveAvailable, blkLeftAvailable);

        if (val_o != val_r)
            return false;
    }

    return true;
}

bool IntraPredHarness::testCorrectness(const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{
    if (opt.getdcval_p)
    {
        if (!check_getDCVal_p_primitive(ref.getdcval_p, opt.getdcval_p))
        {
            printf("intrapred_getDCVal_pel failed\n");
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

    if (opt.getdcval_p)
    {
        printf("intrapred_getDCVal_pel");
        REPORT_SPEEDUP(INTRAPRED_ITERATIONS,
                       opt.getdcval_p(pixel_buff, srcStride,
                                      width, width,
                                      blkAboveAvailable, blkLeftAvailable),
                       ref.getdcval_p(pixel_buff, srcStride,
                                      width, width,
                                      blkAboveAvailable, blkLeftAvailable)
                       );
    }

    t->Release();
}
