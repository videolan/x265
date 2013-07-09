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

#include "TLibCommon/TComRom.h"
#include "intrapredharness.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

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

    pixel_out_c   = (pixel*)malloc(out_size * sizeof(pixel));
    pixel_out_vec = (pixel*)malloc(out_size * sizeof(pixel));
    pixel_out_33_c   = (pixel*)TestHarness::alignedMalloc(sizeof(pixel), out_size_33, 32);
    pixel_out_33_vec = (pixel*)TestHarness::alignedMalloc(sizeof(pixel), out_size_33, 32);

    if (!pixel_out_c || !pixel_out_vec)
    {
        fprintf(stderr, "init_IntraPred_buffers: malloc failed, unable to initiate tests!\n");
        exit(-1);
    }

    initROM();
}

IntraPredHarness::~IntraPredHarness()
{
    free(pixel_buff);
    free(pixel_out_c);
    free(pixel_out_vec);
    TestHarness::alignedFree(pixel_out_33_c);
    TestHarness::alignedFree(pixel_out_33_vec);
}

bool IntraPredHarness::check_dc_primitive(x265::intra_dc_t ref, x265::intra_dc_t opt)
{
    int j = ADI_BUF_STRIDE;

    for (int i = 0; i <= 100; i++)
    {
        int rand_width = 1 << ((rand() % 5) + 2);                  // Randomly generated Width
        int rand_filter = rand() & 1;

#if _DEBUG
        memset(pixel_out_vec, 0xCD, out_size);
        memset(pixel_out_c, 0xCD, out_size);
#endif

        opt(pixel_buff + j, ADI_BUF_STRIDE, pixel_out_vec, FENC_STRIDE, rand_width, rand_filter);
        ref(pixel_buff + j, ADI_BUF_STRIDE, pixel_out_c,   FENC_STRIDE, rand_width, rand_filter);

        for (int k = 0; k < rand_width; k++)
        {
            if (memcmp(pixel_out_vec + k * FENC_STRIDE, pixel_out_c + k * FENC_STRIDE, rand_width))
                return false;
        }

        j += FENC_STRIDE;
    }

    return true;
}

bool IntraPredHarness::check_planar_primitive(x265::intra_planar_t ref, x265::intra_planar_t opt)
{
    int j = ADI_BUF_STRIDE;

    for (int width = 4; width <= 64; width <<= 1)
    {
        for (int i = 0; i <= 100; i++)
        {
#if _DEBUG
            memset(pixel_out_vec, 0xCD, out_size);
            memset(pixel_out_c, 0xCD, out_size);
#endif
            ref(pixel_buff + j, ADI_BUF_STRIDE, pixel_out_c,   FENC_STRIDE, width);
            opt(pixel_buff + j, ADI_BUF_STRIDE, pixel_out_vec, FENC_STRIDE, width);

            for (int k = 0; k < width; k++)
            {
                if (memcmp(pixel_out_vec + k * FENC_STRIDE, pixel_out_c + k * FENC_STRIDE, width))
                {
                    return false;
                }
            }

            j += FENC_STRIDE;
        }
    }

    return true;
}

bool IntraPredHarness::check_angular_primitive(x265::intra_ang_t ref, x265::intra_ang_t opt)
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
                memset(pixel_out_vec, 0xCD, out_size);
                memset(pixel_out_c, 0xCD, out_size);
#endif
                pixel * refAbove = pixel_buff + j;
                pixel * refLeft = refAbove + 3 * width;
                refLeft[0] = refAbove[0];

                opt(BIT_DEPTH, pixel_out_vec, FENC_STRIDE, width, pmode, bFilter, refAbove, refLeft);
                ref(BIT_DEPTH, pixel_out_c, FENC_STRIDE, width, pmode, bFilter, refAbove, refLeft);

                for (int k = 0; k < width; k++)
                {
                    if (memcmp(pixel_out_vec + k * FENC_STRIDE, pixel_out_c + k * FENC_STRIDE, width))
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

bool IntraPredHarness::check_allangs_primitive(const x265::intra_allangs_t ref[], const x265::intra_allangs_t opt[])
{
    int j = ADI_BUF_STRIDE;

    Bool isLuma;

    for (int size = 2; size <= 5; size++)
    {
        if (opt[size-2] == NULL) continue;

        const int width = (1<<size);

        for (int i = 0; i <= 100; i++)
        {
            isLuma = (width <= 16) && (rand()%2);

            pixel * refAbove0 = pixel_buff + j;
            pixel * refLeft0 = refAbove0 + 3 * width;

            pixel * refAbove1 = pixel_buff + j + 3 * FENC_STRIDE;   // keep this offset, since vector code may broken input buffer range [-(width-1), 0]
            pixel * refLeft1 = refAbove1 + 3 * width + FENC_STRIDE;
            refLeft0[0] = refAbove0[0] = refLeft1[0] = refAbove1[0];;

#if _DEBUG
            memset(pixel_out_33_vec, 0xCD, out_size_33);
            memset(pixel_out_33_c, 0xCD, out_size_33);
#endif

            ref[size-2](pixel_out_33_c,   refAbove0, refLeft0, refAbove1, refLeft1, isLuma);
            opt[size-2](pixel_out_33_vec, refAbove0, refLeft0, refAbove1, refLeft1, isLuma);
            for (int p = 2-2; p <= 34-2; p++)
            {
                for (int k = 0; k < width; k++)
                {
                    if (memcmp(pixel_out_33_c + p * (width *width) + k * width, pixel_out_33_vec + p * (width *width) + k * width, width))
                    {
                        printf("\nFailed: (%dx%d) Mode(%2d), Line[%2d], bfilter=%d\n", width, width, p+2, k, isLuma);
                        opt[size-2](pixel_out_33_vec, refAbove0, refLeft0, refAbove1, refLeft1, isLuma);
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
    if (opt.intra_pred_dc)
    {
        if (!check_dc_primitive(ref.intra_pred_dc, opt.intra_pred_dc))
        {
            printf("intra_dc failed\n");
            return false;
        }
    }
    if (opt.intra_pred_planar)
    {
        if (!check_planar_primitive(ref.intra_pred_planar, opt.intra_pred_planar))
        {
            printf("intra_planar failed\n");
            return false;
        }
    }
    if (opt.intra_pred_ang)
    {
        if (!check_angular_primitive(ref.intra_pred_ang, opt.intra_pred_ang))
        {
            printf("intra_angular failed\n");
            return false;
        }
    }
    if (opt.intra_pred_allangs[0])
    {
        if (!check_allangs_primitive(ref.intra_pred_allangs, opt.intra_pred_allangs))
        {
            printf("intra_allangs failed\n");
            return false;
        }
    }

    return true;
}

void IntraPredHarness::measureSpeed(const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{
    int width = 64;
    short srcStride = 96;

    if (opt.intra_pred_dc)
    {
        printf("intra_dc[filter=0]");
        REPORT_SPEEDUP(opt.intra_pred_dc, ref.intra_pred_dc,
                       pixel_buff + srcStride, srcStride, pixel_out_vec, FENC_STRIDE, width, 0);
        printf("intra_dc[filter=1]");
        REPORT_SPEEDUP(opt.intra_pred_dc, ref.intra_pred_dc,
                       pixel_buff + srcStride, srcStride, pixel_out_vec, FENC_STRIDE, width, 1);
    }
    if (opt.intra_pred_planar)
    {
        for (int ii = 4; ii <= 64; ii <<= 1)
        {
            width = ii;
            printf("intra_planar%2dx%d", ii, ii);
            REPORT_SPEEDUP(opt.intra_pred_planar, ref.intra_pred_planar,
                           pixel_buff + srcStride, srcStride, pixel_out_vec, FENC_STRIDE, width);
        }
    }
    if (opt.intra_pred_ang)
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
                printf("intra_ang%dx%d[%02d]", ii, ii, pmode);
                REPORT_SPEEDUP(opt.intra_pred_ang, ref.intra_pred_ang,
                               BIT_DEPTH, pixel_out_vec, FENC_STRIDE, width, pmode, bFilter, refAbove, refLeft);
            }
        }
    }
    for (int size = 2; size <= 6; size++)
    {
        if (opt.intra_pred_allangs[size-2])
        {
            for (int ii = 4; ii <= 4; ii <<= 1)
            {
                width = ii;
                bool bFilter  = (width <= 16);
                pixel * refAbove = pixel_buff + srcStride;
                pixel * refLeft = refAbove + 3 * width;
                refLeft[0] = refAbove[0];
                printf("intra_allangs%dx%d", 1<<size, 1<<size);
                REPORT_SPEEDUP(opt.intra_pred_allangs[size-2], ref.intra_pred_allangs[size-2],
                               pixel_out_33_vec, refAbove, refLeft, refAbove, refLeft, bFilter);
            }
        }
    }
}
