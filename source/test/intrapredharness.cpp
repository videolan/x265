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
    pixel_out_33_c   = (pixel*)X265_MALLOC(pixel, out_size_33);
    pixel_out_33_vec = (pixel*)X265_MALLOC(pixel, out_size_33);

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
    X265_FREE(pixel_out_33_c);
    X265_FREE(pixel_out_33_vec);
}

bool IntraPredHarness::check_dc_primitive(intra_pred_t ref, intra_pred_t opt, int width)
{
    int j = ADI_BUF_STRIDE;

    for (int i = 0; i <= 100; i++)
    {
        int rand_filter = rand() & 1;
        if (width > 16)
            rand_filter = 0;

        pixel left[MAX_CU_SIZE * 2 + 1];
        for (int k = 0; k < width * 2 + 1; k++)
        {
            left[k] = pixel_buff[j - 1 + k * ADI_BUF_STRIDE];
        }

#if _DEBUG
        memset(pixel_out_vec, 0xCD, out_size);
        memset(pixel_out_c, 0xCD, out_size);
#endif
        ref(pixel_out_c,   FENC_STRIDE, pixel_buff + j - ADI_BUF_STRIDE, left + 1, 0, rand_filter);
        opt(pixel_out_vec, FENC_STRIDE, pixel_buff + j - ADI_BUF_STRIDE, left + 1, 0, rand_filter);

        for (int k = 0; k < width; k++)
        {
            if (memcmp(pixel_out_vec + k * FENC_STRIDE, pixel_out_c + k * FENC_STRIDE, width * sizeof(pixel)))
            {
#if _DEBUG
                ref(pixel_out_c,   FENC_STRIDE, pixel_buff + j - ADI_BUF_STRIDE, left + 1, 0, rand_filter);
                opt(pixel_out_vec, FENC_STRIDE, pixel_buff + j - ADI_BUF_STRIDE, left + 1, 0, rand_filter);
#endif
                return false;
            }
        }

        j += FENC_STRIDE;
    }

    return true;
}

bool IntraPredHarness::check_planar_primitive(intra_pred_t ref, intra_pred_t opt, int width)
{
    int j = ADI_BUF_STRIDE;

    for (int i = 0; i <= 100; i++)
    {
        pixel left[MAX_CU_SIZE * 2 + 1];
        for (int k = 0; k < width * 2 + 1; k++)
        {
            left[k] = pixel_buff[j - 1 + k * ADI_BUF_STRIDE];
        }

#if _DEBUG
        memset(pixel_out_vec, 0xCD, out_size);
        memset(pixel_out_c, 0xCD, out_size);
#endif
        ref(pixel_out_c,   FENC_STRIDE, pixel_buff + j - ADI_BUF_STRIDE, left + 1, 0, 0);
        opt(pixel_out_vec, FENC_STRIDE, pixel_buff + j - ADI_BUF_STRIDE, left + 1, 0, 0);

        for (int k = 0; k < width; k++)
        {
            if (memcmp(pixel_out_vec + k * FENC_STRIDE, pixel_out_c + k * FENC_STRIDE, width * sizeof(pixel)))
            {
#if _DEBUG
                ref(pixel_out_c,   FENC_STRIDE, pixel_buff + j - ADI_BUF_STRIDE, left + 1, 0, 0);
                memset(pixel_out_vec, 0xCD, out_size);
                opt(pixel_out_vec, FENC_STRIDE, pixel_buff + j - ADI_BUF_STRIDE, left + 1, 0, 0);
#endif
                return false;
            }
        }

        j += FENC_STRIDE;
    }

    return true;
}

bool IntraPredHarness::check_angular_primitive(const intra_pred_t ref[][NUM_INTRA_MODE], const intra_pred_t opt[][NUM_INTRA_MODE])
{
    int j = ADI_BUF_STRIDE;

    int pmode;
    bool bFilter;

    for (int size = 2; size <= 5; size++)
    {
        int width = (1 << size);
        for (int i = 0; i <= 100; i++)
        {
            bFilter = (width <= 16) && (rand() % 2);
            for (int p = 2; p <= 34; p++)
            {
                pmode = p;
                if (!opt[size - 2][pmode])
                    continue;
#if _DEBUG
                memset(pixel_out_vec, 0xCD, out_size);
                memset(pixel_out_c, 0xCD, out_size);
#endif
                pixel * refAbove = pixel_buff + j;
                pixel * refLeft = refAbove + 3 * width;
                refLeft[0] = refAbove[0];

                opt[size - 2][pmode](pixel_out_vec, FENC_STRIDE, refLeft, refAbove, pmode, bFilter);
                ref[size - 2][pmode](pixel_out_c, FENC_STRIDE, refLeft, refAbove, pmode, bFilter);

                for (int k = 0; k < width; k++)
                {
                    if (memcmp(pixel_out_vec + k * FENC_STRIDE, pixel_out_c + k * FENC_STRIDE, width * sizeof(pixel)))
                    {
                        ref[size - 2][pmode](pixel_out_c, FENC_STRIDE, refLeft, refAbove, pmode, bFilter);
                        opt[size - 2][pmode](pixel_out_vec, FENC_STRIDE, refLeft, refAbove, pmode, bFilter);
                        printf("\nFailed for width %d mode %d bfilter %d row %d \t", width, p, bFilter, k);
                        return false;
                    }
                }
            }

            j += FENC_STRIDE;
        }
    }

    return true;
}

bool IntraPredHarness::check_allangs_primitive(const intra_allangs_t ref[], const intra_allangs_t opt[])
{
    int j = ADI_BUF_STRIDE;

    bool isLuma;

    for (int size = 2; size <= 5; size++)
    {
        if (opt[size - 2] == NULL) continue;

        const int width = (1 << size);

        for (int i = 0; i <= 100; i++)
        {
            isLuma = (width <= 16) ? true : false;  // bFilter is true for 4x4, 8x8, 16x16 and false for 32x32

            pixel * refAbove0 = pixel_buff + j;
            pixel * refLeft0 = refAbove0 + 3 * width;

            pixel * refAbove1 = pixel_buff + j + 3 * FENC_STRIDE;   // keep this offset, since vector code may broken input buffer range [-(width-1), 0]
            pixel * refLeft1 = refAbove1 + 3 * width + FENC_STRIDE;
            refLeft0[0] = refAbove0[0] = refLeft1[0] = refAbove1[0];

#if _DEBUG
            memset(pixel_out_33_vec, 0xCD, out_size_33);
            memset(pixel_out_33_c, 0xCD, out_size_33);
#endif

            ref[size - 2](pixel_out_33_c,   refAbove0, refLeft0, refAbove1, refLeft1, isLuma);
            opt[size - 2](pixel_out_33_vec, refAbove0, refLeft0, refAbove1, refLeft1, isLuma);
            for (int p = 2 - 2; p <= 34 - 2; p++)
            {
                for (int k = 0; k < width; k++)
                {
                    if (memcmp(pixel_out_33_c + p * (width * width) + k * width, pixel_out_33_vec + p * (width * width) + k * width, width * sizeof(pixel)))
                    {
                        printf("\nFailed: (%dx%d) Mode(%2d), Line[%2d], bfilter=%d\n", width, width, p + 2, k, isLuma);
                        opt[size - 2](pixel_out_33_vec, refAbove0, refLeft0, refAbove1, refLeft1, isLuma);
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
    for (int i = BLOCK_4x4; i <= BLOCK_32x32; i++)
    {
        if (opt.intra_pred[i][1])
        {
            const int size = (1 << (i + 2));
            if (!check_dc_primitive(ref.intra_pred[i][1], opt.intra_pred[i][1], size))
            {
                printf("intra_dc %dx%d failed\n", size, size);
                return false;
            }
        }
        if (opt.intra_pred[i][0])
        {
            const int size = (1 << (i + 2));
            if (!check_planar_primitive(ref.intra_pred[i][0], opt.intra_pred[i][0], size))
            {
                printf("intra_planar %dx%d failed\n", size, size);
                return false;
            }
        }
    }

    // NOTE: always call since this function have check pointer in loop
    if (!check_angular_primitive(ref.intra_pred, opt.intra_pred))
    {
        printf("intra_angular failed\n");
        return false;
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
    uint16_t srcStride = 96;

    for (int i = BLOCK_4x4; i <= BLOCK_32x32; i++)
    {
        const int size = (1 << (i + 2));
        if (opt.intra_pred[i][1])
        {
            printf("intra_dc_%dx%d[f=0]", size, size);
            REPORT_SPEEDUP(opt.intra_pred[i][1], ref.intra_pred[i][1],
                           pixel_out_vec, FENC_STRIDE, pixel_buff + srcStride, pixel_buff, 0, 0);
            if (size <= 16)
            {
                printf("intra_dc_%dx%d[f=1]", size, size);
                REPORT_SPEEDUP(opt.intra_pred[i][1], ref.intra_pred[i][1],
                               pixel_out_vec, FENC_STRIDE, pixel_buff + srcStride, pixel_buff, 0, 0);
            }
        }
        if (opt.intra_pred[i][0])
        {
            printf("intra_planar %2dx%d", size, size);
            REPORT_SPEEDUP(opt.intra_pred[i][0], ref.intra_pred[i][0],
                           pixel_out_vec, FENC_STRIDE, pixel_buff + srcStride, pixel_buff, 0, 0);
        }
        if (opt.intra_pred_allangs[i])
        {
            bool bFilter = (size <= 16);
            pixel * refAbove = pixel_buff + srcStride;
            pixel * refLeft = refAbove + 3 * size;
            refLeft[0] = refAbove[0];
            printf("intra_allangs%dx%d", size, size);
            REPORT_SPEEDUP(opt.intra_pred_allangs[i], ref.intra_pred_allangs[i],
                           pixel_out_33_vec, refAbove, refLeft, refAbove, refLeft, bFilter);
        }
    }

    for (int ii = 2; ii <= 5; ii++)
    {
        for (int p = 2; p <= 34; p += 1)
        {
            int pmode = p;  //(rand()%33)+2;
            if (opt.intra_pred[ii - 2][pmode])
            {
                width = (1 << ii);
                bool bFilter = (width <= 16);
                pixel * refAbove = pixel_buff + srcStride;
                pixel * refLeft = refAbove + 3 * width;
                refLeft[0] = refAbove[0];
                printf("intra_ang%dx%d[%2d]", width, width, pmode);
                REPORT_SPEEDUP(opt.intra_pred[ii - 2][pmode], ref.intra_pred[ii - 2][pmode],
                               pixel_out_vec, FENC_STRIDE, refAbove, refLeft, pmode, bFilter);
            }
        }
    }
}
