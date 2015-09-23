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
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#include "common.h"
#include "predict.h"
#include "intrapredharness.h"

using namespace X265_NS;

IntraPredHarness::IntraPredHarness()
{
    for (int i = 0; i < INPUT_SIZE; i++)
        pixel_buff[i] = rand() % PIXEL_MAX;

    /* [0] --- Random values
     * [1] --- Minimum
     * [2] --- Maximum */
    for (int i = 0; i < BUFFSIZE; i++)
    {
        pixel_test_buff[0][i]   = rand() % PIXEL_MAX;
        pixel_test_buff[1][i]   = PIXEL_MIN;
        pixel_test_buff[2][i]   = PIXEL_MAX;
    }
}

bool IntraPredHarness::check_dc_primitive(intra_pred_t ref, intra_pred_t opt, int width)
{
    int j = Predict::ADI_BUF_STRIDE;
    intptr_t stride = FENC_STRIDE;

#if _DEBUG
    memset(pixel_out_vec, 0xCD, OUTPUT_SIZE);
    memset(pixel_out_c, 0xCD, OUTPUT_SIZE);
#endif

    for (int i = 0; i <= 100; i++)
    {
        int rand_filter = rand() & 1;
        if (width > 16)
            rand_filter = 0;

        ref(pixel_out_c, stride, pixel_buff + j - Predict::ADI_BUF_STRIDE, 0, rand_filter);
        checked(opt, pixel_out_vec, stride, pixel_buff + j - Predict::ADI_BUF_STRIDE, 0, rand_filter);

        for (int k = 0; k < width; k++)
        {
            if (memcmp(pixel_out_vec + k * FENC_STRIDE, pixel_out_c + k * FENC_STRIDE, width * sizeof(pixel)))
                return false;
        }

        reportfail();
        j += FENC_STRIDE;
    }

    return true;
}

bool IntraPredHarness::check_planar_primitive(intra_pred_t ref, intra_pred_t opt, int width)
{
    int j = Predict::ADI_BUF_STRIDE;
    intptr_t stride = FENC_STRIDE;

#if _DEBUG
    memset(pixel_out_vec, 0xCD, OUTPUT_SIZE);
    memset(pixel_out_c, 0xCD, OUTPUT_SIZE);
#endif

    for (int i = 0; i <= 100; i++)
    {
        ref(pixel_out_c, stride, pixel_buff + j - Predict::ADI_BUF_STRIDE, 0, 0);
        checked(opt, pixel_out_vec, stride, pixel_buff + j - Predict::ADI_BUF_STRIDE, 0, 0);

        for (int k = 0; k < width; k++)
        {
            if (memcmp(pixel_out_vec + k * FENC_STRIDE, pixel_out_c + k * FENC_STRIDE, width * sizeof(pixel)))
                return false;
        }

        reportfail();
        j += FENC_STRIDE;
    }

    return true;
}

bool IntraPredHarness::check_angular_primitive(const intra_pred_t ref[], const intra_pred_t opt[], int sizeIdx)
{
    int j = Predict::ADI_BUF_STRIDE;
    intptr_t stride = FENC_STRIDE;

#if _DEBUG
    memset(pixel_out_vec, 0xCD, OUTPUT_SIZE);
    memset(pixel_out_c, 0xCD, OUTPUT_SIZE);
#endif

    int width = 1 << (sizeIdx + 2);
    for (int i = 0; i <= 100; i++)
    {
        int bFilter = (width <= 16) && (rand() % 2);
        for (int pmode = 2; pmode <= 34; pmode++)
        {
            if (!opt[pmode])
                continue;

            checked(opt[pmode], pixel_out_vec, stride, pixel_buff + j, pmode, bFilter);
            ref[pmode](pixel_out_c, stride, pixel_buff + j, pmode, bFilter);

            for (int k = 0; k < width; k++)
            {
                if (memcmp(pixel_out_vec + k * FENC_STRIDE, pixel_out_c + k * FENC_STRIDE, width * sizeof(pixel)))
                {
                    printf("ang_%dx%d, Mode = %d, Row = %d failed !!\n", width, width, pmode, k);
                    ref[pmode](pixel_out_c, stride, pixel_buff + j, pmode, bFilter);
                    opt[pmode](pixel_out_vec, stride, pixel_buff + j, pmode, bFilter);
                    return false;
                }
            }

            reportfail();
        }

        j += FENC_STRIDE;
    }

    return true;
}

bool IntraPredHarness::check_allangs_primitive(const intra_allangs_t ref, const intra_allangs_t opt, int sizeIdx)
{
    int j = Predict::ADI_BUF_STRIDE;
    int isLuma;

#if _DEBUG
    memset(pixel_out_33_vec, 0xCD, OUTPUT_SIZE_33);
    memset(pixel_out_33_c, 0xCD, OUTPUT_SIZE_33);
#endif

    const int width = 1 << (sizeIdx + 2);

    for (int i = 0; i <= 100; i++)
    {
        isLuma = (width <= 16) ? true : false;  // bFilter is true for 4x4, 8x8, 16x16 and false for 32x32

        pixel * refAbove0 = pixel_buff + j + 3 * FENC_STRIDE;   // keep this offset, since vector code may broken input buffer range [-(width-1), 0];
        pixel * refLeft0 = refAbove0 + 3 * width + FENC_STRIDE;

        refLeft0[0] = refAbove0[0];

        ref(pixel_out_33_c,   refAbove0, refLeft0, isLuma);
        checked(opt, pixel_out_33_vec, refAbove0, refLeft0, isLuma);

        for (int p = 2 - 2; p <= 34 - 2; p++)
        {
            for (int k = 0; k < width; k++)
            {
                if (memcmp(pixel_out_33_c + p * (width * width) + k * width, pixel_out_33_vec + p * (width * width) + k * width, width * sizeof(pixel)))
                {
                    printf("\nFailed: (%dx%d) Mode(%2d), Line[%2d], bfilter=%d\n", width, width, p + 2, k, isLuma);
                    opt(pixel_out_33_vec, refAbove0, refLeft0, isLuma);
                    return false;
                }
            }
        }

        reportfail();
        j += FENC_STRIDE;
    }

    return true;
}

bool IntraPredHarness::check_intra_filter_primitive(const intra_filter_t ref, const intra_filter_t opt)
{
    memset(pixel_out_c, 0, 64 * 64 * sizeof(pixel));
    memset(pixel_out_vec, 0, 64 * 64 * sizeof(pixel));
    int j = 0;

    for (int i = 0; i < 100; i++)
    {
        int index = rand() % TEST_CASES;

        ref(pixel_test_buff[index] + j, pixel_out_c);
        checked(opt, pixel_test_buff[index] + j, pixel_out_vec);

        if (memcmp(pixel_out_c, pixel_out_vec, 64 * 64 * sizeof(pixel)))
            return false;

        reportfail();
        j += FENC_STRIDE;
    }
    return true;
}
bool IntraPredHarness::testCorrectness(const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{
    for (int i = BLOCK_4x4; i <= BLOCK_32x32; i++)
    {
        const int size = (1 << (i + 2));
        if (opt.cu[i].intra_pred[PLANAR_IDX])
        {
            if (!check_planar_primitive(ref.cu[i].intra_pred[PLANAR_IDX], opt.cu[i].intra_pred[PLANAR_IDX], size))
            {
                printf("intra_planar %dx%d failed\n", size, size);
                return false;
            }
        }
        if (opt.cu[i].intra_pred[DC_IDX])
        {
            if (!check_dc_primitive(ref.cu[i].intra_pred[DC_IDX], opt.cu[i].intra_pred[DC_IDX], size))
            {
                printf("intra_dc %dx%d failed\n", size, size);
                return false;
            }
        }

        if (!check_angular_primitive(ref.cu[i].intra_pred, opt.cu[i].intra_pred, i))
        {
            printf("intra_angular failed\n");
            return false;
        }

        if (opt.cu[i].intra_pred_allangs)
        {
            if (!check_allangs_primitive(ref.cu[i].intra_pred_allangs, opt.cu[i].intra_pred_allangs, i))
            {
                printf("intra_allangs failed\n");
                return false;
            }
        }
        if (opt.cu[i].intra_filter)
        {
            if (!check_intra_filter_primitive(ref.cu[i].intra_filter, opt.cu[i].intra_filter))
            {
                printf("intra_filter_%dx%d failed\n", size, size);
                return false;
            }
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
        if (opt.cu[i].intra_pred[PLANAR_IDX])
        {
            printf("intra_planar_%dx%d", size, size);
            REPORT_SPEEDUP(opt.cu[i].intra_pred[PLANAR_IDX], ref.cu[i].intra_pred[PLANAR_IDX],
                           pixel_out_vec, FENC_STRIDE, pixel_buff + srcStride, 0, 0);
        }
        if (opt.cu[i].intra_pred[DC_IDX])
        {
            printf("intra_dc_%dx%d[f=0]", size, size);
            REPORT_SPEEDUP(opt.cu[i].intra_pred[DC_IDX], ref.cu[i].intra_pred[DC_IDX],
                pixel_out_vec, FENC_STRIDE, pixel_buff + srcStride, 0, 0);
            if (size <= 16)
            {
                printf("intra_dc_%dx%d[f=1]", size, size);
                REPORT_SPEEDUP(opt.cu[i].intra_pred[DC_IDX], ref.cu[i].intra_pred[DC_IDX],
                    pixel_out_vec, FENC_STRIDE, pixel_buff + srcStride, 0, 1);
            }
        }
        if (opt.cu[i].intra_pred_allangs)
        {
            bool bFilter = (size <= 16);
            pixel * refAbove = pixel_buff + srcStride;
            pixel * refLeft = refAbove + 3 * size;
            refLeft[0] = refAbove[0];
            printf("intra_allangs%dx%d", size, size);
            REPORT_SPEEDUP(opt.cu[i].intra_pred_allangs, ref.cu[i].intra_pred_allangs,
                           pixel_out_33_vec, refAbove, refLeft, bFilter);
        }
        for (int mode = 2; mode <= 34; mode += 1)
        {
            if (opt.cu[i].intra_pred[mode])
            {
                width = 1 << (i + 2);
                bool bFilter = (width <= 16);
                pixel * refAbove = pixel_buff + srcStride;
                pixel * refLeft = refAbove + 3 * width;
                refLeft[0] = refAbove[0];
                printf("intra_ang_%dx%d[%2d]", width, width, mode);
                REPORT_SPEEDUP(opt.cu[i].intra_pred[mode], ref.cu[i].intra_pred[mode],
                               pixel_out_vec, FENC_STRIDE, pixel_buff + srcStride, mode, bFilter);
            }
        }
        if (opt.cu[i].intra_filter)
        {
            printf("intra_filter_%dx%d", size, size);
            REPORT_SPEEDUP(opt.cu[i].intra_filter, ref.cu[i].intra_filter, pixel_buff, pixel_out_c);
        }
    }
}
