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
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#include "pixelharness.h"
#include "primitives.h"
#include "entropy.h"

using namespace X265_NS;

PixelHarness::PixelHarness()
{
    /* [0] --- Random values
     * [1] --- Minimum
     * [2] --- Maximum */
    for (int i = 0; i < BUFFSIZE; i++)
    {
        pixel_test_buff[0][i]   = rand() % PIXEL_MAX;
        short_test_buff[0][i]   = (rand() % (2 * SMAX + 1)) - SMAX - 1; // max(SHORT_MIN, min(rand(), SMAX));
        short_test_buff1[0][i]  = rand() & PIXEL_MAX;                   // For block copy only
        short_test_buff2[0][i]  = rand() % 16383;                       // for addAvg
        int_test_buff[0][i]     = rand() % SHORT_MAX;
        ushort_test_buff[0][i]  = rand() % ((1 << 16) - 1);
        uchar_test_buff[0][i]   = rand() % ((1 << 8) - 1);

        pixel_test_buff[1][i]   = PIXEL_MIN;
        short_test_buff[1][i]   = SMIN;
        short_test_buff1[1][i]  = PIXEL_MIN;
        short_test_buff2[1][i]  = -16384;
        int_test_buff[1][i]     = SHORT_MIN;
        ushort_test_buff[1][i]  = PIXEL_MIN;
        uchar_test_buff[1][i]   = PIXEL_MIN;

        pixel_test_buff[2][i]   = PIXEL_MAX;
        short_test_buff[2][i]   = SMAX;
        short_test_buff1[2][i]  = PIXEL_MAX;
        short_test_buff2[2][i]  = 16383;
        int_test_buff[2][i]     = SHORT_MAX;
        ushort_test_buff[2][i]  = ((1 << 16) - 1);
        uchar_test_buff[2][i]   = 255;

        pbuf1[i] = rand() & PIXEL_MAX;
        pbuf2[i] = rand() & PIXEL_MAX;
        pbuf3[i] = rand() & PIXEL_MAX;
        pbuf4[i] = rand() & PIXEL_MAX;

        sbuf1[i] = (rand() % (2 * SMAX + 1)) - SMAX - 1; //max(SHORT_MIN, min(rand(), SMAX));
        sbuf2[i] = (rand() % (2 * SMAX + 1)) - SMAX - 1; //max(SHORT_MIN, min(rand(), SMAX));
        ibuf1[i] = (rand() % (2 * SMAX + 1)) - SMAX - 1;
        psbuf1[i] = psbuf4[i] = (rand() % 65) - 32;                   // range is between -32 to 32
        psbuf2[i] = psbuf5[i] = (rand() % 3) - 1;                     // possible values {-1,0,1}
        psbuf3[i] = (rand() % 129) - 128;
        sbuf3[i] = rand() % PIXEL_MAX; // for blockcopy only
    }
}

bool PixelHarness::check_pixelcmp(pixelcmp_t ref, pixelcmp_t opt)
{
    int j = 0;
    intptr_t stride = STRIDE;

    for (int i = 0; i < ITERS; i++)
    {
        int index1 = rand() % TEST_CASES;
        int index2 = rand() % TEST_CASES;
        int vres = (int)checked(opt, pixel_test_buff[index1], stride, pixel_test_buff[index2] + j, stride);
        int cres = ref(pixel_test_buff[index1], stride, pixel_test_buff[index2] + j, stride);
        if (vres != cres)
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_pixel_sse(pixel_sse_t ref, pixel_sse_t opt)
{
    int j = 0;
    intptr_t stride = STRIDE;

    for (int i = 0; i < ITERS; i++)
    {
        int index1 = rand() % TEST_CASES;
        int index2 = rand() % TEST_CASES;
        sse_ret_t vres = (sse_ret_t)checked(opt, pixel_test_buff[index1], stride, pixel_test_buff[index2] + j, stride);
        sse_ret_t cres = ref(pixel_test_buff[index1], stride, pixel_test_buff[index2] + j, stride);
        if (vres != cres)
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_pixel_sse_ss(pixel_sse_ss_t ref, pixel_sse_ss_t opt)
{
    int j = 0;
    intptr_t stride = STRIDE;

    for (int i = 0; i < ITERS; i++)
    {
        int index1 = rand() % TEST_CASES;
        int index2 = rand() % TEST_CASES;
        sse_ret_t vres = (sse_ret_t)checked(opt, short_test_buff[index1], stride, short_test_buff[index2] + j, stride);
        sse_ret_t cres = ref(short_test_buff[index1], stride, short_test_buff[index2] + j, stride);
        if (vres != cres)
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_pixelcmp_x3(pixelcmp_x3_t ref, pixelcmp_x3_t opt)
{
    ALIGN_VAR_16(int, cres[16]);
    ALIGN_VAR_16(int, vres[16]);
    int j = 0;
    intptr_t stride = FENC_STRIDE - 5;
    for (int i = 0; i < ITERS; i++)
    {
        int index1 = rand() % TEST_CASES;
        int index2 = rand() % TEST_CASES;
        checked(opt, pixel_test_buff[index1],
                pixel_test_buff[index2] + j,
                pixel_test_buff[index2] + j + 1,
                pixel_test_buff[index2] + j + 2, stride, &vres[0]);
        ref(pixel_test_buff[index1],
            pixel_test_buff[index2] + j,
            pixel_test_buff[index2] + j + 1,
            pixel_test_buff[index2] + j + 2, stride, &cres[0]);
        if ((vres[0] != cres[0]) || ((vres[1] != cres[1])) || ((vres[2] != cres[2])))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_pixelcmp_x4(pixelcmp_x4_t ref, pixelcmp_x4_t opt)
{
    ALIGN_VAR_16(int, cres[16]);
    ALIGN_VAR_16(int, vres[16]);
    int j = 0;
    intptr_t stride = FENC_STRIDE - 5;
    for (int i = 0; i < ITERS; i++)
    {
        int index1 = rand() % TEST_CASES;
        int index2 = rand() % TEST_CASES;
        checked(opt, pixel_test_buff[index1],
                pixel_test_buff[index2] + j,
                pixel_test_buff[index2] + j + 1,
                pixel_test_buff[index2] + j + 2,
                pixel_test_buff[index2] + j + 3, stride, &vres[0]);
        ref(pixel_test_buff[index1],
            pixel_test_buff[index2] + j,
            pixel_test_buff[index2] + j + 1,
            pixel_test_buff[index2] + j + 2,
            pixel_test_buff[index2] + j + 3, stride, &cres[0]);

        if ((vres[0] != cres[0]) || ((vres[1] != cres[1])) || ((vres[2] != cres[2])) || ((vres[3] != cres[3])))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_calresidual(calcresidual_t ref, calcresidual_t opt)
{
    ALIGN_VAR_16(int16_t, ref_dest[64 * 64]);
    ALIGN_VAR_16(int16_t, opt_dest[64 * 64]);
    memset(ref_dest, 0, 64 * 64 * sizeof(int16_t));
    memset(opt_dest, 0, 64 * 64 * sizeof(int16_t));

    int j = 0;
    intptr_t stride = STRIDE;
    for (int i = 0; i < ITERS; i++)
    {
        int index = i % TEST_CASES;
        checked(opt, pbuf1 + j, pixel_test_buff[index] + j, opt_dest, stride);
        ref(pbuf1 + j, pixel_test_buff[index] + j, ref_dest, stride);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(int16_t)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_ssd_s(pixel_ssd_s_t ref, pixel_ssd_s_t opt)
{
    int j = 0;
    for (int i = 0; i < ITERS; i++)
    {
        // NOTE: stride must be multiple of 16, because minimum block is 4x4
        int stride = (STRIDE + (rand() % STRIDE)) & ~15;
        int cres = ref(sbuf1 + j, stride);
        int vres = (int)checked(opt, sbuf1 + j, (intptr_t)stride);

        if (cres != vres)
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_weightp(weightp_sp_t ref, weightp_sp_t opt)
{
    ALIGN_VAR_16(pixel, ref_dest[64 * (64 + 1)]);
    ALIGN_VAR_16(pixel, opt_dest[64 * (64 + 1)]);

    memset(ref_dest, 0, 64 * 64 * sizeof(pixel));
    memset(opt_dest, 0, 64 * 64 * sizeof(pixel));
    int j = 0;
    int width = 2 * (rand() % 32 + 1);
    int height = 8;
    int w0 = rand() % 128;
    int shift = rand() % 8; // maximum is 7, see setFromWeightAndOffset()
    int round = shift ? (1 << (shift - 1)) : 0;
    int offset = (rand() % 256) - 128;
    intptr_t stride = 64;
    const int correction = (IF_INTERNAL_PREC - X265_DEPTH);

    for (int i = 0; i < ITERS; i++)
    {
        int index = i % TEST_CASES;
        checked(opt, short_test_buff[index] + j, opt_dest, stride, stride + 1, width, height, w0, round << correction, shift + correction, offset);
        ref(short_test_buff[index] + j, ref_dest, stride, stride + 1, width, height, w0, round << correction, shift + correction, offset);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(pixel)))
        {
            opt(short_test_buff[index] + j, opt_dest, stride, stride + 1, width, height, w0, round << correction, shift + correction, offset);
            return false;
        }

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_weightp(weightp_pp_t ref, weightp_pp_t opt)
{
    ALIGN_VAR_16(pixel, ref_dest[64 * 64]);
    ALIGN_VAR_16(pixel, opt_dest[64 * 64]);

    memset(ref_dest, 0, 64 * 64 * sizeof(pixel));
    memset(opt_dest, 0, 64 * 64 * sizeof(pixel));
    int j = 0;
    int width = 16 * (rand() % 4 + 1);
    int height = 8;
    int w0 = rand() % 128;
    int shift = rand() % 8; // maximum is 7, see setFromWeightAndOffset()
    int round = shift ? (1 << (shift - 1)) : 0;
    int offset = (rand() % 256) - 128;
    intptr_t stride = 64;
    const int correction = (IF_INTERNAL_PREC - X265_DEPTH);
    for (int i = 0; i < ITERS; i++)
    {
        int index = i % TEST_CASES;
        checked(opt, pixel_test_buff[index] + j, opt_dest, stride, width, height, w0, round << correction, shift + correction, offset);
        ref(pixel_test_buff[index] + j, ref_dest, stride, width, height, w0, round << correction, shift + correction, offset);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(pixel)))
        {
            checked(opt, pixel_test_buff[index] + j, opt_dest, stride, width, height, w0, round << correction, shift + correction, offset);
            return false;
        }

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_downscale_t(downscale_t ref, downscale_t opt)
{
    ALIGN_VAR_16(pixel, ref_destf[32 * 32]);
    ALIGN_VAR_16(pixel, opt_destf[32 * 32]);

    ALIGN_VAR_16(pixel, ref_desth[32 * 32]);
    ALIGN_VAR_16(pixel, opt_desth[32 * 32]);

    ALIGN_VAR_16(pixel, ref_destv[32 * 32]);
    ALIGN_VAR_16(pixel, opt_destv[32 * 32]);

    ALIGN_VAR_16(pixel, ref_destc[32 * 32]);
    ALIGN_VAR_16(pixel, opt_destc[32 * 32]);

    intptr_t src_stride = 64;
    intptr_t dst_stride = 32;
    int bx = 32;
    int by = 32;
    int j = 0;
    for (int i = 0; i < ITERS; i++)
    {
        int index = i % TEST_CASES;
        ref(pixel_test_buff[index] + j, ref_destf, ref_desth, ref_destv,
            ref_destc, src_stride, dst_stride, bx, by);
        checked(opt, pixel_test_buff[index] + j, opt_destf, opt_desth, opt_destv,
                opt_destc, src_stride, dst_stride, bx, by);

        if (memcmp(ref_destf, opt_destf, 32 * 32 * sizeof(pixel)))
            return false;
        if (memcmp(ref_desth, opt_desth, 32 * 32 * sizeof(pixel)))
            return false;
        if (memcmp(ref_destv, opt_destv, 32 * 32 * sizeof(pixel)))
            return false;
        if (memcmp(ref_destc, opt_destc, 32 * 32 * sizeof(pixel)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_cpy2Dto1D_shl_t(cpy2Dto1D_shl_t ref, cpy2Dto1D_shl_t opt)
{
    ALIGN_VAR_16(int16_t, ref_dest[64 * 64]);
    ALIGN_VAR_16(int16_t, opt_dest[64 * 64]);

    memset(ref_dest, 0xCD, sizeof(ref_dest));
    memset(opt_dest, 0xCD, sizeof(opt_dest));

    int j = 0;
    intptr_t stride = STRIDE;
    for (int i = 0; i < ITERS; i++)
    {
        int shift = (rand() % 7 + 1);

        int index = i % TEST_CASES;
        checked(opt, opt_dest, short_test_buff[index] + j, stride, shift);
        ref(ref_dest, short_test_buff[index] + j, stride, shift);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(int16_t)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_cpy2Dto1D_shr_t(cpy2Dto1D_shr_t ref, cpy2Dto1D_shr_t opt)
{
    ALIGN_VAR_16(int16_t, ref_dest[64 * 64]);
    ALIGN_VAR_16(int16_t, opt_dest[64 * 64]);

    memset(ref_dest, 0xCD, sizeof(ref_dest));
    memset(opt_dest, 0xCD, sizeof(opt_dest));

    int j = 0;
    intptr_t stride = STRIDE;
    for (int i = 0; i < ITERS; i++)
    {
        int shift = (rand() % 7 + 1);

        int index = i % TEST_CASES;
        checked(opt, opt_dest, short_test_buff[index] + j, stride, shift);
        ref(ref_dest, short_test_buff[index] + j, stride, shift);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(int16_t)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_copy_cnt_t(copy_cnt_t ref, copy_cnt_t opt)
{
    ALIGN_VAR_16(int16_t, ref_dest[64 * 64]);
    ALIGN_VAR_16(int16_t, opt_dest[64 * 64]);

    memset(ref_dest, 0xCD, sizeof(ref_dest));
    memset(opt_dest, 0xCD, sizeof(opt_dest));

    int j = 0;
    intptr_t stride = STRIDE;
    for (int i = 0; i < ITERS; i++)
    {
        int index = i % TEST_CASES;
        int opt_cnt = (int)checked(opt, opt_dest, short_test_buff1[index] + j, stride);
        int ref_cnt = ref(ref_dest, short_test_buff1[index] + j, stride);

        if ((ref_cnt != opt_cnt) || memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(int16_t)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_cpy1Dto2D_shl_t(cpy1Dto2D_shl_t ref, cpy1Dto2D_shl_t opt)
{
    ALIGN_VAR_16(int16_t, ref_dest[64 * 64]);
    ALIGN_VAR_16(int16_t, opt_dest[64 * 64]);

    memset(ref_dest, 0xCD, sizeof(ref_dest));
    memset(opt_dest, 0xCD, sizeof(opt_dest));

    int j = 0;
    intptr_t stride = STRIDE;
    for (int i = 0; i < ITERS; i++)
    {
        int shift = (rand() % 7 + 1);

        int index = i % TEST_CASES;
        checked(opt, opt_dest, short_test_buff[index] + j, stride, shift);
        ref(ref_dest, short_test_buff[index] + j, stride, shift);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(int16_t)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_cpy1Dto2D_shr_t(cpy1Dto2D_shr_t ref, cpy1Dto2D_shr_t opt)
{
    ALIGN_VAR_16(int16_t, ref_dest[64 * 64]);
    ALIGN_VAR_16(int16_t, opt_dest[64 * 64]);

    memset(ref_dest, 0xCD, sizeof(ref_dest));
    memset(opt_dest, 0xCD, sizeof(opt_dest));

    int j = 0;
    intptr_t stride = STRIDE;
    for (int i = 0; i < ITERS; i++)
    {
        int shift = (rand() % 7 + 1);

        int index = i % TEST_CASES;
        checked(opt, opt_dest, short_test_buff[index] + j, stride, shift);
        ref(ref_dest, short_test_buff[index] + j, stride, shift);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(int16_t)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_pixelavg_pp(pixelavg_pp_t ref, pixelavg_pp_t opt)
{
    ALIGN_VAR_16(pixel, ref_dest[64 * 64]);
    ALIGN_VAR_16(pixel, opt_dest[64 * 64]);

    int j = 0;

    memset(ref_dest, 0xCD, sizeof(ref_dest));
    memset(opt_dest, 0xCD, sizeof(opt_dest));

    intptr_t stride = STRIDE;
    for (int i = 0; i < ITERS; i++)
    {
        int index1 = rand() % TEST_CASES;
        int index2 = rand() % TEST_CASES;
        checked(ref, ref_dest, stride, pixel_test_buff[index1] + j,
                stride, pixel_test_buff[index2] + j, stride, 32);
        opt(opt_dest, stride, pixel_test_buff[index1] + j,
            stride, pixel_test_buff[index2] + j, stride, 32);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(pixel)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_copy_pp(copy_pp_t ref, copy_pp_t opt)
{
    ALIGN_VAR_16(pixel, ref_dest[64 * 64]);
    ALIGN_VAR_16(pixel, opt_dest[64 * 64]);

    // we don't know the partition size so we are checking the entire output buffer so
    // we must initialize the buffers
    memset(ref_dest, 0, sizeof(ref_dest));
    memset(opt_dest, 0, sizeof(opt_dest));

    int j = 0;
    intptr_t stride = STRIDE;
    for (int i = 0; i < ITERS; i++)
    {
        int index = i % TEST_CASES;
        checked(opt, opt_dest, stride, pixel_test_buff[index] + j, stride);
        ref(ref_dest, stride, pixel_test_buff[index] + j, stride);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(pixel)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_copy_sp(copy_sp_t ref, copy_sp_t opt)
{
    ALIGN_VAR_16(pixel, ref_dest[64 * 64]);
    ALIGN_VAR_16(pixel, opt_dest[64 * 64]);

    // we don't know the partition size so we are checking the entire output buffer so
    // we must initialize the buffers
    memset(ref_dest, 0xCD, sizeof(ref_dest));
    memset(opt_dest, 0xCD, sizeof(opt_dest));

    int j = 0;
    intptr_t stride1 = 64, stride2 = STRIDE;
    for (int i = 0; i < ITERS; i++)
    {
        int index = i % TEST_CASES;
        checked(opt, opt_dest, stride1, short_test_buff1[index] + j, stride2);
        ref(ref_dest, stride1, short_test_buff1[index] + j, stride2);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(pixel)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_copy_ps(copy_ps_t ref, copy_ps_t opt)
{
    ALIGN_VAR_16(int16_t, ref_dest[64 * 64]);
    ALIGN_VAR_16(int16_t, opt_dest[64 * 64]);

    // we don't know the partition size so we are checking the entire output buffer so
    // we must initialize the buffers
    memset(ref_dest, 0xCD, sizeof(ref_dest));
    memset(opt_dest, 0xCD, sizeof(opt_dest));

    int j = 0;
    intptr_t stride = STRIDE;
    for (int i = 0; i < ITERS; i++)
    {
        int index = i % TEST_CASES;
        checked(opt, opt_dest, stride, pixel_test_buff[index] + j, stride);
        ref(ref_dest, stride, pixel_test_buff[index] + j, stride);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(int16_t)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_copy_ss(copy_ss_t ref, copy_ss_t opt)
{
    ALIGN_VAR_16(int16_t, ref_dest[64 * 64]);
    ALIGN_VAR_16(int16_t, opt_dest[64 * 64]);

    // we don't know the partition size so we are checking the entire output buffer so
    // we must initialize the buffers
    memset(ref_dest, 0xCD, sizeof(ref_dest));
    memset(opt_dest, 0xCD, sizeof(opt_dest));

    int j = 0;
    intptr_t stride = STRIDE;
    for (int i = 0; i < ITERS; i++)
    {
        int index = i % TEST_CASES;
        checked(opt, opt_dest, stride, short_test_buff1[index] + j, stride);
        ref(ref_dest, stride, short_test_buff1[index] + j, stride);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(int16_t)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_blockfill_s(blockfill_s_t ref, blockfill_s_t opt)
{
    ALIGN_VAR_16(int16_t, ref_dest[64 * 64]);
    ALIGN_VAR_16(int16_t, opt_dest[64 * 64]);

    memset(ref_dest, 0xCD, sizeof(ref_dest));
    memset(opt_dest, 0xCD, sizeof(opt_dest));

    intptr_t stride = 64;
    for (int i = 0; i < ITERS; i++)
    {
        int16_t value = (rand() % SHORT_MAX) + 1;

        checked(opt, opt_dest, stride, value);
        ref(ref_dest, stride, value);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(int16_t)))
            return false;

        reportfail();
    }

    return true;
}

bool PixelHarness::check_pixel_sub_ps(pixel_sub_ps_t ref, pixel_sub_ps_t opt)
{
    ALIGN_VAR_16(int16_t, ref_dest[64 * 64]);
    ALIGN_VAR_16(int16_t, opt_dest[64 * 64]);

    memset(ref_dest, 0xCD, sizeof(ref_dest));
    memset(opt_dest, 0xCD, sizeof(opt_dest));

    int j = 0;
    intptr_t stride2 = 64, stride = STRIDE;
    for (int i = 0; i < 1; i++)
    {
        int index1 = rand() % TEST_CASES;
        int index2 = rand() % TEST_CASES;
        checked(opt, opt_dest, stride2, pixel_test_buff[index1] + j,
                pixel_test_buff[index2] + j, stride, stride);
        ref(ref_dest, stride2, pixel_test_buff[index1] + j,
            pixel_test_buff[index2] + j, stride, stride);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(int16_t)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_scale1D_pp(scale1D_t ref, scale1D_t opt)
{
    ALIGN_VAR_16(pixel, ref_dest[64 * 64]);
    ALIGN_VAR_16(pixel, opt_dest[64 * 64]);

    memset(ref_dest, 0, sizeof(ref_dest));
    memset(opt_dest, 0, sizeof(opt_dest));

    int j = 0;
    for (int i = 0; i < ITERS; i++)
    {
        int index = i % TEST_CASES;
        checked(opt, opt_dest, pixel_test_buff[index] + j);
        ref(ref_dest, pixel_test_buff[index] + j);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(pixel)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_scale2D_pp(scale2D_t ref, scale2D_t opt)
{
    ALIGN_VAR_16(pixel, ref_dest[64 * 64]);
    ALIGN_VAR_16(pixel, opt_dest[64 * 64]);

    memset(ref_dest, 0, sizeof(ref_dest));
    memset(opt_dest, 0, sizeof(opt_dest));

    int j = 0;
    intptr_t stride = STRIDE;
    for (int i = 0; i < ITERS; i++)
    {
        int index = i % TEST_CASES;
        checked(opt, opt_dest, pixel_test_buff[index] + j, stride);
        ref(ref_dest, pixel_test_buff[index] + j, stride);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(pixel)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_transpose(transpose_t ref, transpose_t opt)
{
    ALIGN_VAR_16(pixel, ref_dest[64 * 64]);
    ALIGN_VAR_16(pixel, opt_dest[64 * 64]);

    memset(ref_dest, 0, sizeof(ref_dest));
    memset(opt_dest, 0, sizeof(opt_dest));

    int j = 0;
    intptr_t stride = STRIDE;
    for (int i = 0; i < ITERS; i++)
    {
        int index = i % TEST_CASES;
        checked(opt, opt_dest, pixel_test_buff[index] + j, stride);
        ref(ref_dest, pixel_test_buff[index] + j, stride);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(pixel)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_pixel_add_ps(pixel_add_ps_t ref, pixel_add_ps_t opt)
{
    ALIGN_VAR_16(pixel, ref_dest[64 * 64]);
    ALIGN_VAR_16(pixel, opt_dest[64 * 64]);

    memset(ref_dest, 0xCD, sizeof(ref_dest));
    memset(opt_dest, 0xCD, sizeof(opt_dest));

    int j = 0;
    intptr_t stride2 = 64, stride = STRIDE;
    for (int i = 0; i < ITERS; i++)
    {
        int index1 = rand() % TEST_CASES;
        int index2 = rand() % TEST_CASES;
        checked(opt, opt_dest, stride2, pixel_test_buff[index1] + j, short_test_buff[index2] + j, stride, stride);
        ref(ref_dest, stride2, pixel_test_buff[index1] + j, short_test_buff[index2] + j, stride, stride);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(pixel)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_pixel_var(var_t ref, var_t opt)
{
    int j = 0;

    intptr_t stride = STRIDE;

    for (int i = 0; i < ITERS; i++)
    {
        int index = i % TEST_CASES;
        uint64_t vres = checked(opt, pixel_test_buff[index], stride);
        uint64_t cres = ref(pixel_test_buff[index], stride);
        if (vres != cres)
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_ssim_4x4x2_core(ssim_4x4x2_core_t ref, ssim_4x4x2_core_t opt)
{
    ALIGN_VAR_32(int, sum0[2][4]);
    ALIGN_VAR_32(int, sum1[2][4]);

    for (int i = 0; i < ITERS; i++)
    {
        intptr_t stride = rand() % 64;
        int index1 = rand() % TEST_CASES;
        int index2 = rand() % TEST_CASES;
        ref(pixel_test_buff[index1] + i, stride, pixel_test_buff[index2] + i, stride, sum0);
        checked(opt, pixel_test_buff[index1] + i, stride, pixel_test_buff[index2] + i, stride, sum1);

        if (memcmp(sum0, sum1, sizeof(sum0)))
            return false;

        reportfail();
    }

    return true;
}

bool PixelHarness::check_ssim_end(ssim_end4_t ref, ssim_end4_t opt)
{
    ALIGN_VAR_32(int, sum0[5][4]);
    ALIGN_VAR_32(int, sum1[5][4]);

    for (int i = 0; i < ITERS; i++)
    {
        for (int j = 0; j < 5; j++)
        {
            for (int k = 0; k < 4; k++)
            {
                sum0[j][k] = rand() % (1 << 12);
                sum1[j][k] = rand() % (1 << 12);
            }
        }

        int width = (rand() % 4) + 1; // range[1-4]
        float cres = ref(sum0, sum1, width);
        float vres = checked_float(opt, sum0, sum1, width);
        if (fabs(vres - cres) > 0.00001)
            return false;

        reportfail();
    }

    return true;
}

bool PixelHarness::check_addAvg(addAvg_t ref, addAvg_t opt)
{
    ALIGN_VAR_16(pixel, ref_dest[64 * 64]);
    ALIGN_VAR_16(pixel, opt_dest[64 * 64]);

    int j = 0;

    memset(ref_dest, 0xCD, sizeof(ref_dest));
    memset(opt_dest, 0xCD, sizeof(opt_dest));
    intptr_t stride = STRIDE;

    for (int i = 0; i < ITERS; i++)
    {
        int index1 = rand() % TEST_CASES;
        int index2 = rand() % TEST_CASES;
        ref(short_test_buff2[index1] + j, short_test_buff2[index2] + j, ref_dest, stride, stride, stride);
        checked(opt, short_test_buff2[index1] + j, short_test_buff2[index2] + j, opt_dest, stride, stride, stride);
        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(pixel)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_calSign(sign_t ref, sign_t opt)
{
    ALIGN_VAR_16(int8_t, ref_dest[64 * 2]);
    ALIGN_VAR_16(int8_t, opt_dest[64 * 2]);

    memset(ref_dest, 0xCD, sizeof(ref_dest));
    memset(opt_dest, 0xCD, sizeof(opt_dest));

    int j = 0;

    for (int i = 0; i < ITERS; i++)
    {
        int width = (rand() % 64) + 1;

        ref(ref_dest, pbuf2 + j, pbuf3 + j, width);
        checked(opt, opt_dest, pbuf2 + j, pbuf3 + j, width);

        if (memcmp(ref_dest, opt_dest, sizeof(ref_dest)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_saoCuOrgE0_t(saoCuOrgE0_t ref, saoCuOrgE0_t opt)
{
    ALIGN_VAR_16(pixel, ref_dest[64 * 64]);
    ALIGN_VAR_16(pixel, opt_dest[64 * 64]);

    for (int i = 0; i < 64 * 64; i++)
        ref_dest[i] = opt_dest[i] = rand() % (PIXEL_MAX);

    int j = 0;

    for (int i = 0; i < ITERS; i++)
    {
        int width = 16 * (rand() % 4 + 1);
        int stride = width + 1;

        ref(ref_dest, psbuf1 + j, width, psbuf2 + j, stride);
        checked(opt, opt_dest, psbuf1 + j, width, psbuf5 + j, stride);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(pixel)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_saoCuOrgE1_t(saoCuOrgE1_t ref, saoCuOrgE1_t opt)
{
    ALIGN_VAR_16(pixel, ref_dest[64 * 64]);
    ALIGN_VAR_16(pixel, opt_dest[64 * 64]);

    for (int i = 0; i < 64 * 64; i++)
        ref_dest[i] = opt_dest[i] = rand() % (PIXEL_MAX);

    int j = 0;

    for (int i = 0; i < ITERS; i++)
    {
        int width = 16 * (rand() % 4 + 1);
        int stride = width + 1;

        ref(ref_dest, psbuf2 + j, psbuf1 + j, stride, width);
        checked(opt, opt_dest, psbuf5 + j, psbuf1 + j, stride, width);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(pixel)) || memcmp(psbuf2, psbuf5, BUFFSIZE))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_saoCuOrgE2_t(saoCuOrgE2_t ref[2], saoCuOrgE2_t opt[2])
{
    ALIGN_VAR_16(pixel, ref_dest[64 * 64]);
    ALIGN_VAR_16(pixel, opt_dest[64 * 64]);

    for (int i = 0; i < 64 * 64; i++)
        ref_dest[i] = opt_dest[i] = rand() % (PIXEL_MAX);

    for (int id = 0; id < 2; id++)
    {
        int j = 0;
        if (opt[id])
        {
            for (int i = 0; i < ITERS; i++)
            {
                int width = 16 * (1 << (id * (rand() % 2 + 1))) - (rand() % 2);
                int stride = width + 1;

                ref[width > 16](ref_dest, psbuf1 + j, psbuf2 + j, psbuf3 + j, width, stride);
                checked(opt[width > 16], opt_dest, psbuf4 + j, psbuf2 + j, psbuf3 + j, width, stride);

                if (memcmp(psbuf1 + j, psbuf4 + j, width * sizeof(int8_t)))
                    return false;

                if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(pixel)))
                    return false;

                reportfail();
                j += INCR;
            }
        }
    }

    return true;
}

bool PixelHarness::check_saoCuOrgE3_t(saoCuOrgE3_t ref, saoCuOrgE3_t opt)
{
    ALIGN_VAR_16(pixel, ref_dest[64 * 64]);
    ALIGN_VAR_16(pixel, opt_dest[64 * 64]);

    for (int i = 0; i < 64 * 64; i++)
        ref_dest[i] = opt_dest[i] = rand() % (PIXEL_MAX);

    int j = 0;

    for (int i = 0; i < ITERS; i++)
    {
        int stride = 16 * (rand() % 4 + 1);
        int start = rand() % 2;
        int end = 16 - rand() % 2;

        ref(ref_dest, psbuf2 + j, psbuf1 + j, stride, start, end);
        checked(opt, opt_dest, psbuf5 + j, psbuf1 + j, stride, start, end);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(pixel)) || memcmp(psbuf2, psbuf5, BUFFSIZE))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_saoCuStatsBO_t(saoCuStatsBO_t ref, saoCuStatsBO_t opt)
{
    enum { NUM_EDGETYPE = 33 }; // classIdx = 1 + (rec[x] >> 3);
    int32_t stats_ref[NUM_EDGETYPE];
    int32_t stats_vec[NUM_EDGETYPE];

    int32_t count_ref[NUM_EDGETYPE];
    int32_t count_vec[NUM_EDGETYPE];

    int j = 0;
    for (int i = 0; i < ITERS; i++)
    {
        // initialize input data to random, the dynamic range wrong but good to verify our asm code
        for (int x = 0; x < NUM_EDGETYPE; x++)
        {
            stats_ref[x] = stats_vec[x] = rand();
            count_ref[x] = count_vec[x] = rand();
        }

        intptr_t stride = 16 * (rand() % 4 + 1);
        int endX = MAX_CU_SIZE - (rand() % 5);
        int endY = MAX_CU_SIZE - (rand() % 4) - 1;

        ref(pbuf2 + j + 1, pbuf3 + 1, stride, endX, endY, stats_ref, count_ref);
        checked(opt, pbuf2 + j + 1, pbuf3 + 1, stride, endX, endY, stats_vec, count_vec);

        if (memcmp(stats_ref, stats_vec, sizeof(stats_ref)) || memcmp(count_ref, count_vec, sizeof(count_ref)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_saoCuStatsE0_t(saoCuStatsE0_t ref, saoCuStatsE0_t opt)
{
    enum { NUM_EDGETYPE = 5 };
    int32_t stats_ref[NUM_EDGETYPE];
    int32_t stats_vec[NUM_EDGETYPE];

    int32_t count_ref[NUM_EDGETYPE];
    int32_t count_vec[NUM_EDGETYPE];

    int j = 0;
    for (int i = 0; i < ITERS; i++)
    {
        // initialize input data to random, the dynamic range wrong but good to verify our asm code
        for (int x = 0; x < NUM_EDGETYPE; x++)
        {
            stats_ref[x] = stats_vec[x] = rand();
            count_ref[x] = count_vec[x] = rand();
        }

        intptr_t stride = 16 * (rand() % 4 + 1);
        int endX = MAX_CU_SIZE - (rand() % 5) - 1;
        int endY = MAX_CU_SIZE - (rand() % 4) - 1;

        ref(pbuf2 + j + 1, pbuf3 + j + 1, stride, endX, endY, stats_ref, count_ref);
        checked(opt, pbuf2 + j + 1, pbuf3 + j + 1, stride, endX, endY, stats_vec, count_vec);

        if (memcmp(stats_ref, stats_vec, sizeof(stats_ref)) || memcmp(count_ref, count_vec, sizeof(count_ref)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_saoCuStatsE1_t(saoCuStatsE1_t ref, saoCuStatsE1_t opt)
{
    enum { NUM_EDGETYPE = 5 };
    int32_t stats_ref[NUM_EDGETYPE];
    int32_t stats_vec[NUM_EDGETYPE];

    int32_t count_ref[NUM_EDGETYPE];
    int32_t count_vec[NUM_EDGETYPE];

    int8_t _upBuff1_ref[MAX_CU_SIZE + 2], *upBuff1_ref = _upBuff1_ref + 1;
    int8_t _upBuff1_vec[MAX_CU_SIZE + 2], *upBuff1_vec = _upBuff1_vec + 1;

    int j = 0;

    for (int i = 0; i < ITERS; i++)
    {
        // initialize input data to random, the dynamic range wrong but good to verify our asm code
        for (int x = 0; x < NUM_EDGETYPE; x++)
        {
            stats_ref[x] = stats_vec[x] = rand();
            count_ref[x] = count_vec[x] = rand();
        }

        // initial sign
        for (int x = 0; x < MAX_CU_SIZE + 2; x++)
            _upBuff1_ref[x] = _upBuff1_vec[x] = (rand() % 3) - 1;

        intptr_t stride = 16 * (rand() % 4 + 1);
        int endX = MAX_CU_SIZE - (rand() % 5);
        int endY = MAX_CU_SIZE - (rand() % 4) - 1;

        ref(pbuf2 + 1, pbuf3 + 1, stride, upBuff1_ref, endX, endY, stats_ref, count_ref);
        checked(opt, pbuf2 + 1, pbuf3 + 1, stride, upBuff1_vec, endX, endY, stats_vec, count_vec);

        if (   memcmp(_upBuff1_ref, _upBuff1_vec, sizeof(_upBuff1_ref))
            || memcmp(stats_ref, stats_vec, sizeof(stats_ref))
            || memcmp(count_ref, count_vec, sizeof(count_ref)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_saoCuStatsE2_t(saoCuStatsE2_t ref, saoCuStatsE2_t opt)
{
    enum { NUM_EDGETYPE = 5 };
    int32_t stats_ref[NUM_EDGETYPE];
    int32_t stats_vec[NUM_EDGETYPE];

    int32_t count_ref[NUM_EDGETYPE];
    int32_t count_vec[NUM_EDGETYPE];

    int8_t _upBuff1_ref[MAX_CU_SIZE + 2], *upBuff1_ref = _upBuff1_ref + 1;
    int8_t _upBufft_ref[MAX_CU_SIZE + 2], *upBufft_ref = _upBufft_ref + 1;
    int8_t _upBuff1_vec[MAX_CU_SIZE + 2], *upBuff1_vec = _upBuff1_vec + 1;
    int8_t _upBufft_vec[MAX_CU_SIZE + 2], *upBufft_vec = _upBufft_vec + 1;

    int j = 0;

    // NOTE: verify more times since our asm is NOT exact match to C, the output of upBuff* will be DIFFERENT
    for (int i = 0; i < ITERS * 10; i++)
    {
        // initialize input data to random, the dynamic range wrong but good to verify our asm code
        for (int x = 0; x < NUM_EDGETYPE; x++)
        {
            stats_ref[x] = stats_vec[x] = rand();
            count_ref[x] = count_vec[x] = rand();
        }

        // initial sign
        for (int x = 0; x < MAX_CU_SIZE + 2; x++)
        {
            _upBuff1_ref[x] = _upBuff1_vec[x] = (rand() % 3) - 1;
            _upBufft_ref[x] = _upBufft_vec[x] = (rand() % 3) - 1;
        }

        intptr_t stride = 16 * (rand() % 4 + 1);
        int endX = MAX_CU_SIZE - (rand() % 5) - 1;
        int endY = MAX_CU_SIZE - (rand() % 4) - 1;

        ref(pbuf2 + 1, pbuf3 + 1, stride, upBuff1_ref, upBufft_ref, endX, endY, stats_ref, count_ref);
        checked(opt, pbuf2 + 1, pbuf3 + 1, stride, upBuff1_vec, upBufft_vec, endX, endY, stats_vec, count_vec);

        // TODO: don't check upBuff*, the latest output pixels different, and can move into stack temporary buffer in future
        if (   memcmp(_upBuff1_ref, _upBuff1_vec, sizeof(_upBuff1_ref))
            || memcmp(_upBufft_ref, _upBufft_vec, sizeof(_upBufft_ref))
            || memcmp(stats_ref, stats_vec, sizeof(stats_ref))
            || memcmp(count_ref, count_vec, sizeof(count_ref)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_saoCuStatsE3_t(saoCuStatsE3_t ref, saoCuStatsE3_t opt)
{
    enum { NUM_EDGETYPE = 5 };
    int32_t stats_ref[NUM_EDGETYPE];
    int32_t stats_vec[NUM_EDGETYPE];

    int32_t count_ref[NUM_EDGETYPE];
    int32_t count_vec[NUM_EDGETYPE];

    int8_t _upBuff1_ref[MAX_CU_SIZE + 2], *upBuff1_ref = _upBuff1_ref + 1;
    int8_t _upBuff1_vec[MAX_CU_SIZE + 2], *upBuff1_vec = _upBuff1_vec + 1;

    int j = 0;

    // (const pixel *fenc, const pixel *rec, intptr_t stride, int8_t *upBuff1, int endX, int endY, int32_t *stats, int32_t *count)
    for (int i = 0; i < ITERS; i++)
    {
        // initialize input data to random, the dynamic range wrong but good to verify our asm code
        for (int x = 0; x < NUM_EDGETYPE; x++)
        {
            stats_ref[x] = stats_vec[x] = rand();
            count_ref[x] = count_vec[x] = rand();
        }

        // initial sign
        for (int x = 0; x < (int)sizeof(_upBuff1_ref); x++)
        {
            _upBuff1_ref[x] = _upBuff1_vec[x] = (rand() % 3) - 1;
        }

        intptr_t stride = 16 * (rand() % 4 + 1);
        int endX = MAX_CU_SIZE - (rand() % 5) - 1;
        int endY = MAX_CU_SIZE - (rand() % 4) - 1;

        ref(pbuf2, pbuf3, stride, upBuff1_ref, endX, endY, stats_ref, count_ref);
        checked(opt, pbuf2, pbuf3, stride, upBuff1_vec, endX, endY, stats_vec, count_vec);

        if (   memcmp(_upBuff1_ref, _upBuff1_vec, sizeof(_upBuff1_ref))
            || memcmp(stats_ref, stats_vec, sizeof(stats_ref))
            || memcmp(count_ref, count_vec, sizeof(count_ref)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_saoCuOrgE3_32_t(saoCuOrgE3_t ref, saoCuOrgE3_t opt)
{
    ALIGN_VAR_16(pixel, ref_dest[64 * 64]);
    ALIGN_VAR_16(pixel, opt_dest[64 * 64]);

    for (int i = 0; i < 64 * 64; i++)
        ref_dest[i] = opt_dest[i] = rand() % (PIXEL_MAX);

    int j = 0;

    for (int i = 0; i < ITERS; i++)
    {
        int stride = 32 * (rand() % 2 + 1);
        int start = rand() % 2;
        int end = (32 * (rand() % 2 + 1)) - rand() % 2;

        ref(ref_dest, psbuf2 + j, psbuf1 + j, stride, start, end);
        checked(opt, opt_dest, psbuf5 + j, psbuf1 + j, stride, start, end);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(pixel)) || memcmp(psbuf2, psbuf5, BUFFSIZE))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_planecopy_sp(planecopy_sp_t ref, planecopy_sp_t opt)
{
    ALIGN_VAR_16(pixel, ref_dest[64 * 64]);
    ALIGN_VAR_16(pixel, opt_dest[64 * 64]);

    memset(ref_dest, 0xCD, sizeof(ref_dest));
    memset(opt_dest, 0xCD, sizeof(opt_dest));
    int width = 32 + rand() % 32;
    int height = 32 + rand() % 32;
    intptr_t srcStride = 64;
    intptr_t dstStride = width;
    int j = 0;

    for (int i = 0; i < ITERS; i++)
    {
        int index = i % TEST_CASES;
        checked(opt, ushort_test_buff[index] + j, srcStride, opt_dest, dstStride, width, height, (int)8, (uint16_t)((1 << X265_DEPTH) - 1));
        ref(ushort_test_buff[index] + j, srcStride, ref_dest, dstStride, width, height, (int)8, (uint16_t)((1 << X265_DEPTH) - 1));

        if (memcmp(ref_dest, opt_dest, width * height * sizeof(pixel)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_planecopy_cp(planecopy_cp_t ref, planecopy_cp_t opt)
{
    ALIGN_VAR_16(pixel, ref_dest[64 * 64 * 2]);
    ALIGN_VAR_16(pixel, opt_dest[64 * 64 * 2]);

    memset(ref_dest, 0xCD, sizeof(ref_dest));
    memset(opt_dest, 0xCD, sizeof(opt_dest));

    int width = 16 + rand() % 48;
    int height = 16 + rand() % 48;
    intptr_t srcStride = 64;
    intptr_t dstStride = width;
    int j = 0;

    for (int i = 0; i < ITERS; i++)
    {
        int index = i % TEST_CASES;
        checked(opt, uchar_test_buff[index] + j, srcStride, opt_dest, dstStride, width, height, (int)2);
        ref(uchar_test_buff[index] + j, srcStride, ref_dest, dstStride, width, height, (int)2);

        if (memcmp(ref_dest, opt_dest, sizeof(ref_dest)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_cutree_propagate_cost(cutree_propagate_cost ref, cutree_propagate_cost opt)
{
    ALIGN_VAR_16(int, ref_dest[64 * 64]);
    ALIGN_VAR_16(int, opt_dest[64 * 64]);

    memset(ref_dest, 0xCD, sizeof(ref_dest));
    memset(opt_dest, 0xCD, sizeof(opt_dest));

    double fps = 1.0;
    int width = 16 + rand() % 64;
    int j = 0;

    for (int i = 0; i < ITERS; i++)
    {
        int index = i % TEST_CASES;
        checked(opt, opt_dest, ushort_test_buff[index] + j, int_test_buff[index] + j, ushort_test_buff[index] + j, int_test_buff[index] + j, &fps, width);
        ref(ref_dest, ushort_test_buff[index] + j, int_test_buff[index] + j, ushort_test_buff[index] + j, int_test_buff[index] + j, &fps, width);

        if (memcmp(ref_dest, opt_dest, width * sizeof(pixel)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_psyCost_pp(pixelcmp_t ref, pixelcmp_t opt)
{
    int j = 0, index1, index2, optres, refres;
    intptr_t stride = STRIDE;

    for (int i = 0; i < ITERS; i++)
    {
        index1 = rand() % TEST_CASES;
        index2 = rand() % TEST_CASES;
        optres = (int)checked(opt, pixel_test_buff[index1], stride, pixel_test_buff[index2] + j, stride);
        refres = ref(pixel_test_buff[index1], stride, pixel_test_buff[index2] + j, stride);

        if (optres != refres)
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_psyCost_ss(pixelcmp_ss_t ref, pixelcmp_ss_t opt)
{
    int j = 0, index1, index2, optres, refres;
    intptr_t stride = STRIDE;

    for (int i = 0; i < ITERS; i++)
    {
        index1 = rand() % TEST_CASES;
        index2 = rand() % TEST_CASES;
        optres = (int)checked(opt, short_test_buff[index1], stride, short_test_buff[index2] + j, stride);
        refres = ref(short_test_buff[index1], stride, short_test_buff[index2] + j, stride);

        if (optres != refres)
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_saoCuOrgB0_t(saoCuOrgB0_t ref, saoCuOrgB0_t opt)
{
    ALIGN_VAR_16(pixel, ref_dest[64 * 64]);
    ALIGN_VAR_16(pixel, opt_dest[64 * 64]);

    for (int i = 0; i < 64 * 64; i++)
        ref_dest[i] = opt_dest[i] = rand() % (PIXEL_MAX);

    int j = 0;

    for (int i = 0; i < ITERS; i++)
    {
        int width = 16 * (rand() % 4 + 1);
        int height = rand() % 63 + 2;
        int stride = width;

        ref(ref_dest, psbuf1 + j, width, height, stride);
        checked(opt, opt_dest, psbuf1 + j, width, height, stride);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(pixel)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_scanPosLast(scanPosLast_t ref, scanPosLast_t opt)
{
    ALIGN_VAR_16(coeff_t, ref_src[32 * 32 + ITERS * 2]);
    uint8_t ref_coeffNum[MLS_GRP_NUM], opt_coeffNum[MLS_GRP_NUM];      // value range[0, 16]
    uint16_t ref_coeffSign[MLS_GRP_NUM], opt_coeffSign[MLS_GRP_NUM];    // bit mask map for non-zero coeff sign
    uint16_t ref_coeffFlag[MLS_GRP_NUM], opt_coeffFlag[MLS_GRP_NUM];    // bit mask map for non-zero coeff

    int totalCoeffs = 0;
    for (int i = 0; i < 32 * 32; i++)
    {
        ref_src[i] = rand() & SHORT_MAX;

        // more zero coeff
        if (ref_src[i] < SHORT_MAX * 2 / 3)
            ref_src[i] = 0;

        // more negtive
        if ((rand() % 10) < 8)
            ref_src[i] *= -1;
        totalCoeffs += (ref_src[i] != 0);
    }

    // extra test area all of 0x1234
    for (int i = 0; i < ITERS * 2; i++)
    {
        ref_src[32 * 32 + i] = 0x1234;
    }
    

    memset(ref_coeffNum, 0xCD, sizeof(ref_coeffNum));
    memset(ref_coeffSign, 0xCD, sizeof(ref_coeffSign));
    memset(ref_coeffFlag, 0xCD, sizeof(ref_coeffFlag));

    memset(opt_coeffNum, 0xCD, sizeof(opt_coeffNum));
    memset(opt_coeffSign, 0xCD, sizeof(opt_coeffSign));
    memset(opt_coeffFlag, 0xCD, sizeof(opt_coeffFlag));

    for (int i = 0; i < ITERS; i++)
    {
        int rand_scan_type = rand() % NUM_SCAN_TYPE;
        int rand_scan_size = rand() % NUM_SCAN_SIZE;
        int rand_numCoeff = 0;

        for (int j = 0; j < 1 << (2 * (rand_scan_size + 2)); j++)
            rand_numCoeff += (ref_src[i + j] != 0);

        // at least one coeff in transform block
        if (rand_numCoeff == 0)
        {
            ref_src[i + (1 << (2 * (rand_scan_size + 2))) - 1] = -1;
            rand_numCoeff = 1;
        }

        const int trSize = (1 << (rand_scan_size + 2));
        const uint16_t* const scanTbl = g_scanOrder[rand_scan_type][rand_scan_size];
        const uint16_t* const scanTblCG4x4 = g_scan4x4[rand_scan_size <= (MDCS_LOG2_MAX_SIZE - 2) ? rand_scan_type : SCAN_DIAG];

        int ref_scanPos = ref(scanTbl, ref_src + i, ref_coeffSign, ref_coeffFlag, ref_coeffNum, rand_numCoeff, scanTblCG4x4, trSize);
        int opt_scanPos = (int)checked(opt, scanTbl, ref_src + i, opt_coeffSign, opt_coeffFlag, opt_coeffNum, rand_numCoeff, scanTblCG4x4, trSize);

        if (ref_scanPos != opt_scanPos)
            return false;

        for (int j = 0; rand_numCoeff; j++)
        {
            if (ref_coeffSign[j] != opt_coeffSign[j])
                return false;

            if (ref_coeffFlag[j] != opt_coeffFlag[j])
                return false;

            if (ref_coeffNum[j] != opt_coeffNum[j])
                return false;

            rand_numCoeff -= ref_coeffNum[j];
        }

        if (rand_numCoeff != 0)
            return false;

        reportfail();
    }

    return true;
}

bool PixelHarness::check_findPosFirstLast(findPosFirstLast_t ref, findPosFirstLast_t opt)
{
    ALIGN_VAR_16(coeff_t, ref_src[4 * 32 + ITERS * 2]);
    memset(ref_src, 0, sizeof(ref_src));

    // minus ITERS for keep probability to generate all zeros block
    for (int i = 0; i < 4 * 32 - ITERS; i++)
    {
        ref_src[i] = rand() & SHORT_MAX;
    }

    // extra test area all of Zeros

    for (int i = 0; i < ITERS; i++)
    {
        int rand_scan_type = rand() % NUM_SCAN_TYPE;
        int rand_scan_size = (rand() % NUM_SCAN_SIZE) + 2;
        const int trSize = (1 << rand_scan_size);
        coeff_t *rand_src = ref_src + i;

        const uint16_t* const scanTbl = g_scan4x4[rand_scan_type];

        int j;
        for (j = 0; j < SCAN_SET_SIZE; j++)
        {
            const uint32_t idxY = j / MLS_CG_SIZE;
            const uint32_t idxX = j % MLS_CG_SIZE;
            if (rand_src[idxY * trSize + idxX]) break;
        }

        uint32_t ref_scanPos = ref(rand_src, trSize, scanTbl);
        uint32_t opt_scanPos = (int)checked(opt, rand_src, trSize, scanTbl);

        // specially case: all coeff group are zero
        if (j >= SCAN_SET_SIZE)
        {
            // all zero block the high 16-bits undefined
            if ((uint16_t)ref_scanPos != (uint16_t)opt_scanPos)
                return false;
        }
        else if (ref_scanPos != opt_scanPos)
            return false;

        reportfail();
    }

    return true;
}

bool PixelHarness::check_costCoeffNxN(costCoeffNxN_t ref, costCoeffNxN_t opt)
{
    ALIGN_VAR_16(coeff_t, ref_src[32 * 32 + ITERS * 3]);
    ALIGN_VAR_32(uint16_t, ref_absCoeff[1 << MLS_CG_SIZE]);
    ALIGN_VAR_32(uint16_t, opt_absCoeff[1 << MLS_CG_SIZE]);

    memset(ref_absCoeff, 0xCD, sizeof(ref_absCoeff));
    memset(opt_absCoeff, 0xCD, sizeof(opt_absCoeff));

    int totalCoeffs = 0;
    for (int i = 0; i < 32 * 32; i++)
    {
        ref_src[i] = rand() & SHORT_MAX;

        // more zero coeff
        if (ref_src[i] < SHORT_MAX * 2 / 3)
            ref_src[i] = 0;

        // more negtive
        if ((rand() % 10) < 8)
            ref_src[i] *= -1;
        totalCoeffs += (ref_src[i] != 0);
    }

    // extra test area all of 0x1234
    for (int i = 0; i < ITERS * 3; i++)
    {
        ref_src[32 * 32 + i] = 0x1234;
    }

    // generate CABAC context table
    uint8_t m_contextState_ref[OFF_SIG_FLAG_CTX + NUM_SIG_FLAG_CTX_LUMA];
    uint8_t m_contextState_opt[OFF_SIG_FLAG_CTX + NUM_SIG_FLAG_CTX_LUMA];
    for (int k = 0; k < (OFF_SIG_FLAG_CTX + NUM_SIG_FLAG_CTX_LUMA); k++)
    {
        m_contextState_ref[k] = (rand() % (125 - 2)) + 2;
        m_contextState_opt[k] = m_contextState_ref[k];
    }
    uint8_t *const ref_baseCtx = m_contextState_ref;
    uint8_t *const opt_baseCtx = m_contextState_opt;

    for (int i = 0; i < ITERS * 2; i++)
    {
        int rand_scan_type = rand() % NUM_SCAN_TYPE;
        int rand_scanPosSigOff = rand() % 16; //rand_scanPosSigOff range is [1,15]
        int rand_patternSigCtx = rand() % 4; //range [0,3]
        int rand_scan_size = rand() % NUM_SCAN_SIZE;
        int offset; // the value have a exact range, details in CoeffNxN()
        if (rand_scan_size == 2)
            offset = 0;
        else if (rand_scan_size == 3)
            offset = 9;
        else
            offset = 12;

        const int trSize = (1 << (rand_scan_size + 2));
        ALIGN_VAR_32(static const uint8_t, table_cnt[5][SCAN_SET_SIZE]) =
        {
            // patternSigCtx = 0
            {
                2, 1, 1, 0,
                1, 1, 0, 0,
                1, 0, 0, 0,
                0, 0, 0, 0,
            },
            // patternSigCtx = 1
            {
                2, 2, 2, 2,
                1, 1, 1, 1,
                0, 0, 0, 0,
                0, 0, 0, 0,
            },
            // patternSigCtx = 2
            {
                2, 1, 0, 0,
                2, 1, 0, 0,
                2, 1, 0, 0,
                2, 1, 0, 0,
            },
            // patternSigCtx = 3
            {
                2, 2, 2, 2,
                2, 2, 2, 2,
                2, 2, 2, 2,
                2, 2, 2, 2,
            },
            // 4x4
            {
                0, 1, 4, 5,
                2, 3, 4, 5,
                6, 6, 8, 8,
                7, 7, 8, 8
            }
        };
        const uint8_t *rand_tabSigCtx = table_cnt[(rand_scan_size == 2) ? 4 : (uint32_t)rand_patternSigCtx];
        const uint16_t* const scanTbl = g_scanOrder[rand_scan_type][rand_scan_size];
        const uint16_t* const scanTblCG4x4 = g_scan4x4[rand_scan_size <= (MDCS_LOG2_MAX_SIZE - 2) ? rand_scan_type : SCAN_DIAG];

        int rand_scanPosCG = rand() % (trSize * trSize / MLS_CG_BLK_SIZE);
        int subPosBase = rand_scanPosCG * MLS_CG_BLK_SIZE;
        int rand_numCoeff = 0;
        uint32_t scanFlagMask = 0;
        const int numNonZero = (rand_scanPosSigOff < (MLS_CG_BLK_SIZE - 1)) ? 1 : 0;

        for(int k = 0; k <= rand_scanPosSigOff; k++)
        {
            uint32_t pos = scanTbl[subPosBase + k];
            coeff_t tmp_coeff = ref_src[i + pos];
            if (tmp_coeff != 0)
            {
                rand_numCoeff++;
            }
            scanFlagMask = scanFlagMask * 2 + (tmp_coeff != 0);
        }

        // can't process all zeros block
        if (rand_numCoeff == 0)
            continue;

        const uint32_t blkPosBase = scanTbl[subPosBase];
        uint32_t ref_sum = ref(scanTblCG4x4, &ref_src[blkPosBase + i], trSize, ref_absCoeff + numNonZero, rand_tabSigCtx, scanFlagMask, (uint8_t*)ref_baseCtx, offset, rand_scanPosSigOff, subPosBase);
        uint32_t opt_sum = (uint32_t)checked(opt, scanTblCG4x4, &ref_src[blkPosBase + i], trSize, opt_absCoeff + numNonZero, rand_tabSigCtx, scanFlagMask, (uint8_t*)opt_baseCtx, offset, rand_scanPosSigOff, subPosBase);

        if (ref_sum != opt_sum)
            return false;
        if (memcmp(ref_baseCtx, opt_baseCtx, sizeof(m_contextState_ref)))
            return false;

        // NOTE: just first rand_numCoeff valid, but I check full buffer for confirm no overwrite bug
        if (memcmp(ref_absCoeff, opt_absCoeff, sizeof(ref_absCoeff)))
            return false;

        reportfail();
    }
    return true;
}

bool PixelHarness::check_costCoeffRemain(costCoeffRemain_t ref, costCoeffRemain_t opt)
{
    ALIGN_VAR_32(uint16_t, absCoeff[(1 << MLS_CG_SIZE) + ITERS]);

    for (int i = 0; i < (1 << MLS_CG_SIZE) + ITERS; i++)
    {
        absCoeff[i] = rand() & SHORT_MAX;
        // more coeff with value one
        if (absCoeff[i] < SHORT_MAX * 2 / 3)
            absCoeff[i] = 1;
    }
    for (int i = 0; i < ITERS; i++)
    {
        uint32_t firstC2Idx = 0;
        int k = 0;
        int numNonZero = rand() % 17; //can be random, range[1, 16]
        for (k = 0; k < C1FLAG_NUMBER; k++)
        {
            if (absCoeff[i + k] >= 2)
            {
                break;
            }
        }
        firstC2Idx = k; // it is index of exact first coeff that value more than 2
        int ref_sum = ref(absCoeff + i, numNonZero, firstC2Idx);
        int opt_sum = (int)checked(opt, absCoeff + i, numNonZero, firstC2Idx);
        if (ref_sum != opt_sum)
            return false;
    }
    return true;
}

bool PixelHarness::check_costC1C2Flag(costC1C2Flag_t ref, costC1C2Flag_t opt)
{
    ALIGN_VAR_32(uint16_t, absCoeff[(1 << MLS_CG_SIZE)]);

    // generate CABAC context table
    uint8_t ref_baseCtx[8];
    uint8_t opt_baseCtx[8];
    for (int k = 0; k < 8; k++)
    {
        ref_baseCtx[k] =
        opt_baseCtx[k] = (rand() % (125 - 2)) + 2;
    }

    for (int i = 0; i < ITERS; i++)
    {
        int rand_offset = rand() % 4;
        int numNonZero = 0;

        // generate test data, all are Absolute value and Aligned
        for (int k = 0; k < C1FLAG_NUMBER; k++)
        {
            int value = rand() & SHORT_MAX;
            // more coeff with value [0,2]
            if (value < SHORT_MAX * 1 / 3)
                value = 0;
            else if (value < SHORT_MAX * 2 / 3)
                value = 1;
            else if (value < SHORT_MAX * 3 / 4)
                value = 2;

            if (value)
            {
                absCoeff[numNonZero] = (uint16_t)value;
                numNonZero++;
            }
        }

        int ref_sum = ref(absCoeff, (intptr_t)numNonZero, ref_baseCtx, (intptr_t)rand_offset);
        int opt_sum = (int)checked(opt, absCoeff, (intptr_t)numNonZero, opt_baseCtx, (intptr_t)rand_offset);
        if (ref_sum != opt_sum)
        {
            ref_sum = ref(absCoeff, (intptr_t)numNonZero, ref_baseCtx, (intptr_t)rand_offset);
            opt_sum = opt(absCoeff, (intptr_t)numNonZero, opt_baseCtx, (intptr_t)rand_offset);
            return false;
        }
    }
    return true;
}

bool PixelHarness::check_planeClipAndMax(planeClipAndMax_t ref, planeClipAndMax_t opt)
{
    for (int i = 0; i < ITERS; i++)
    {
        intptr_t rand_stride = rand() % STRIDE;
        int rand_width = (rand() % (STRIDE * 2)) + 1;
        const int rand_height = (rand() % MAX_HEIGHT) + 1;
        const pixel rand_min = rand() % 32;
        const pixel rand_max = PIXEL_MAX - (rand() % 32);
        uint64_t ref_sum, opt_sum;

        // video width must be more than or equal to 32
        if (rand_width < 32)
            rand_width = 32;

        // stride must be more than or equal to width
        if (rand_stride < rand_width)
            rand_stride = rand_width;

        pixel ref_max = ref(pbuf1, rand_stride, rand_width, rand_height, &ref_sum, rand_min, rand_max);
        pixel opt_max = (pixel)checked(opt, pbuf1, rand_stride, rand_width, rand_height, &opt_sum, rand_min, rand_max);

        if (ref_max != opt_max)
            return false;
    }
    return true;
}

bool PixelHarness::testPU(int part, const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{
    if (opt.pu[part].satd)
    {
        if (!check_pixelcmp(ref.pu[part].satd, opt.pu[part].satd))
        {
            printf("satd[%s]: failed!\n", lumaPartStr[part]);
            return false;
        }
    }

    if (opt.pu[part].sad)
    {
        if (!check_pixelcmp(ref.pu[part].sad, opt.pu[part].sad))
        {
            printf("sad[%s]: failed!\n", lumaPartStr[part]);
            return false;
        }
    }

    if (opt.pu[part].sad_x3)
    {
        if (!check_pixelcmp_x3(ref.pu[part].sad_x3, opt.pu[part].sad_x3))
        {
            printf("sad_x3[%s]: failed!\n", lumaPartStr[part]);
            return false;
        }
    }

    if (opt.pu[part].sad_x4)
    {
        if (!check_pixelcmp_x4(ref.pu[part].sad_x4, opt.pu[part].sad_x4))
        {
            printf("sad_x4[%s]: failed!\n", lumaPartStr[part]);
            return false;
        }
    }

    if (opt.pu[part].pixelavg_pp)
    {
        if (!check_pixelavg_pp(ref.pu[part].pixelavg_pp, opt.pu[part].pixelavg_pp))
        {
            printf("pixelavg_pp[%s]: failed!\n", lumaPartStr[part]);
            return false;
        }
    }

    if (opt.pu[part].copy_pp)
    {
        if (!check_copy_pp(ref.pu[part].copy_pp, opt.pu[part].copy_pp))
        {
            printf("copy_pp[%s] failed\n", lumaPartStr[part]);
            return false;
        }
    }

    if (opt.pu[part].addAvg)
    {
        if (!check_addAvg(ref.pu[part].addAvg, opt.pu[part].addAvg))
        {
            printf("addAvg[%s] failed\n", lumaPartStr[part]);
            return false;
        }
    }

    if (part < NUM_CU_SIZES)
    {
        if (opt.cu[part].sse_pp)
        {
            if (!check_pixel_sse(ref.cu[part].sse_pp, opt.cu[part].sse_pp))
            {
                printf("sse_pp[%s]: failed!\n", lumaPartStr[part]);
                return false;
            }
        }

        if (opt.cu[part].sse_ss)
        {
            if (!check_pixel_sse_ss(ref.cu[part].sse_ss, opt.cu[part].sse_ss))
            {
                printf("sse_ss[%s]: failed!\n", lumaPartStr[part]);
                return false;
            }
        }

        if (opt.cu[part].sub_ps)
        {
            if (!check_pixel_sub_ps(ref.cu[part].sub_ps, opt.cu[part].sub_ps))
            {
                printf("sub_ps[%s] failed\n", lumaPartStr[part]);
                return false;
            }
        }

        if (opt.cu[part].add_ps)
        {
            if (!check_pixel_add_ps(ref.cu[part].add_ps, opt.cu[part].add_ps))
            {
                printf("add_ps[%s] failed\n", lumaPartStr[part]);
                return false;
            }
        }

        if (opt.cu[part].copy_ss)
        {
            if (!check_copy_ss(ref.cu[part].copy_ss, opt.cu[part].copy_ss))
            {
                printf("copy_ss[%s] failed\n", lumaPartStr[part]);
                return false;
            }
        }

        if (opt.cu[part].copy_sp)
        {
            if (!check_copy_sp(ref.cu[part].copy_sp, opt.cu[part].copy_sp))
            {
                printf("copy_sp[%s] failed\n", lumaPartStr[part]);
                return false;
            }
        }

        if (opt.cu[part].copy_ps)
        {
            if (!check_copy_ps(ref.cu[part].copy_ps, opt.cu[part].copy_ps))
            {
                printf("copy_ps[%s] failed\n", lumaPartStr[part]);
                return false;
            }
        }
    }

    for (int i = 0; i < X265_CSP_COUNT; i++)
    {
        if (opt.chroma[i].pu[part].copy_pp)
        {
            if (!check_copy_pp(ref.chroma[i].pu[part].copy_pp, opt.chroma[i].pu[part].copy_pp))
            {
                printf("chroma_copy_pp[%s][%s] failed\n", x265_source_csp_names[i], chromaPartStr[i][part]);
                return false;
            }
        }
        if (opt.chroma[i].pu[part].addAvg)
        {
            if (!check_addAvg(ref.chroma[i].pu[part].addAvg, opt.chroma[i].pu[part].addAvg))
            {
                printf("chroma_addAvg[%s][%s] failed\n", x265_source_csp_names[i], chromaPartStr[i][part]);
                return false;
            }
        }
        if (opt.chroma[i].pu[part].satd)
        {
            if (!check_pixelcmp(ref.chroma[i].pu[part].satd, opt.chroma[i].pu[part].satd))
            {
                printf("chroma_satd[%s][%s] failed!\n", x265_source_csp_names[i], chromaPartStr[i][part]);
                return false;
            }
        }
        if (part < NUM_CU_SIZES)
        {
            if (opt.chroma[i].cu[part].sse_pp)
            {
                if (!check_pixel_sse(ref.chroma[i].cu[part].sse_pp, opt.chroma[i].cu[part].sse_pp))
                {
                    printf("chroma_sse_pp[%s][%s]: failed!\n", x265_source_csp_names[i], chromaPartStr[i][part]);
                    return false;
                }
            }
            if (opt.chroma[i].cu[part].sub_ps)
            {
                if (!check_pixel_sub_ps(ref.chroma[i].cu[part].sub_ps, opt.chroma[i].cu[part].sub_ps))
                {
                    printf("chroma_sub_ps[%s][%s] failed\n", x265_source_csp_names[i], chromaPartStr[i][part]);
                    return false;
                }
            }
            if (opt.chroma[i].cu[part].add_ps)
            {
                if (!check_pixel_add_ps(ref.chroma[i].cu[part].add_ps, opt.chroma[i].cu[part].add_ps))
                {
                    printf("chroma_add_ps[%s][%s] failed\n", x265_source_csp_names[i], chromaPartStr[i][part]);
                    return false;
                }
            }
            if (opt.chroma[i].cu[part].copy_sp)
            {
                if (!check_copy_sp(ref.chroma[i].cu[part].copy_sp, opt.chroma[i].cu[part].copy_sp))
                {
                    printf("chroma_copy_sp[%s][%s] failed\n", x265_source_csp_names[i], chromaPartStr[i][part]);
                    return false;
                }
            }
            if (opt.chroma[i].cu[part].copy_ps)
            {
                if (!check_copy_ps(ref.chroma[i].cu[part].copy_ps, opt.chroma[i].cu[part].copy_ps))
                {
                    printf("chroma_copy_ps[%s][%s] failed\n", x265_source_csp_names[i], chromaPartStr[i][part]);
                    return false;
                }
            }
            if (opt.chroma[i].cu[part].copy_ss)
            {
                if (!check_copy_ss(ref.chroma[i].cu[part].copy_ss, opt.chroma[i].cu[part].copy_ss))
                {
                    printf("chroma_copy_ss[%s][%s] failed\n", x265_source_csp_names[i], chromaPartStr[i][part]);
                    return false;
                }
            }
            if (opt.chroma[i].cu[part].sa8d)
            {
                if (!check_pixelcmp(ref.chroma[i].cu[part].sa8d, opt.chroma[i].cu[part].sa8d))
                {
                    printf("chroma_sa8d[%s][%s] failed\n", x265_source_csp_names[i], chromaPartStr[i][part]);
                    return false;
                }
            }
        }
    }

    return true;
}

bool PixelHarness::testCorrectness(const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{
    for (int size = 4; size <= 64; size *= 2)
    {
        int part = partitionFromSizes(size, size); // 2Nx2N
        if (!testPU(part, ref, opt)) return false;

        if (size > 4)
        {
            part = partitionFromSizes(size, size >> 1); // 2NxN
            if (!testPU(part, ref, opt)) return false;
            part = partitionFromSizes(size >> 1, size); // Nx2N
            if (!testPU(part, ref, opt)) return false;
        }
        if (size > 8)
        {
            // 4 AMP modes
            part = partitionFromSizes(size, size >> 2);
            if (!testPU(part, ref, opt)) return false;
            part = partitionFromSizes(size, 3 * (size >> 2));
            if (!testPU(part, ref, opt)) return false;

            part = partitionFromSizes(size >> 2, size);
            if (!testPU(part, ref, opt)) return false;
            part = partitionFromSizes(3 * (size >> 2), size);
            if (!testPU(part, ref, opt)) return false;
        }
    }

    for (int i = 0; i < NUM_CU_SIZES; i++)
    {
        if (opt.cu[i].sa8d)
        {
            if (!check_pixelcmp(ref.cu[i].sa8d, opt.cu[i].sa8d))
            {
                printf("sa8d[%dx%d]: failed!\n", 4 << i, 4 << i);
                return false;
            }
        }

        if (opt.cu[i].blockfill_s)
        {
            if (!check_blockfill_s(ref.cu[i].blockfill_s, opt.cu[i].blockfill_s))
            {
                printf("blockfill_s[%dx%d]: failed!\n", 4 << i, 4 << i);
                return false;
            }
        }

        if (opt.cu[i].var)
        {
            if (!check_pixel_var(ref.cu[i].var, opt.cu[i].var))
            {
                printf("var[%dx%d] failed\n", 4 << i, 4 << i);
                return false;
            }
        }

        if (opt.cu[i].psy_cost_pp)
        {
            if (!check_psyCost_pp(ref.cu[i].psy_cost_pp, opt.cu[i].psy_cost_pp))
            {
                printf("\npsy_cost_pp[%dx%d] failed!\n", 4 << i, 4 << i);
                return false;
            }
        }

        if (opt.cu[i].psy_cost_ss)
        {
            if (!check_psyCost_ss(ref.cu[i].psy_cost_ss, opt.cu[i].psy_cost_ss))
            {
                printf("\npsy_cost_ss[%dx%d] failed!\n", 4 << i, 4 << i);
                return false;
            }
        }

        if (i < BLOCK_64x64)
        {
            /* TU only primitives */

            if (opt.cu[i].calcresidual)
            {
                if (!check_calresidual(ref.cu[i].calcresidual, opt.cu[i].calcresidual))
                {
                    printf("calcresidual width: %d failed!\n", 4 << i);
                    return false;
                }
            }

            if (opt.cu[i].transpose)
            {
                if (!check_transpose(ref.cu[i].transpose, opt.cu[i].transpose))
                {
                    printf("transpose[%dx%d] failed\n", 4 << i, 4 << i);
                    return false;
                }
            }

            if (opt.cu[i].ssd_s)
            {
                if (!check_ssd_s(ref.cu[i].ssd_s, opt.cu[i].ssd_s))
                {
                    printf("ssd_s[%dx%d]: failed!\n", 4 << i, 4 << i);
                    return false;
                }
            }

            if (opt.cu[i].copy_cnt)
            {
                if (!check_copy_cnt_t(ref.cu[i].copy_cnt, opt.cu[i].copy_cnt))
                {
                    printf("copy_cnt[%dx%d] failed!\n", 4 << i, 4 << i);
                    return false;
                }
            }

            if (opt.cu[i].cpy2Dto1D_shl)
            {
                if (!check_cpy2Dto1D_shl_t(ref.cu[i].cpy2Dto1D_shl, opt.cu[i].cpy2Dto1D_shl))
                {
                    printf("cpy2Dto1D_shl[%dx%d] failed!\n", 4 << i, 4 << i);
                    return false;
                }
            }

            if (opt.cu[i].cpy2Dto1D_shr)
            {
                if (!check_cpy2Dto1D_shr_t(ref.cu[i].cpy2Dto1D_shr, opt.cu[i].cpy2Dto1D_shr))
                {
                    printf("cpy2Dto1D_shr failed!\n");
                    return false;
                }
            }

            if (opt.cu[i].cpy1Dto2D_shl)
            {
                if (!check_cpy1Dto2D_shl_t(ref.cu[i].cpy1Dto2D_shl, opt.cu[i].cpy1Dto2D_shl))
                {
                    printf("cpy1Dto2D_shl[%dx%d] failed!\n", 4 << i, 4 << i);
                    return false;
                }
            }

            if (opt.cu[i].cpy1Dto2D_shr)
            {
                if (!check_cpy1Dto2D_shr_t(ref.cu[i].cpy1Dto2D_shr, opt.cu[i].cpy1Dto2D_shr))
                {
                    printf("cpy1Dto2D_shr[%dx%d] failed!\n", 4 << i, 4 << i);
                    return false;
                }
            }
        }
    }

    if (opt.weight_pp)
    {
        if (!check_weightp(ref.weight_pp, opt.weight_pp))
        {
            printf("Weighted Prediction (pixel) failed!\n");
            return false;
        }
    }

    if (opt.weight_sp)
    {
        if (!check_weightp(ref.weight_sp, opt.weight_sp))
        {
            printf("Weighted Prediction (short) failed!\n");
            return false;
        }
    }

    if (opt.frameInitLowres)
    {
        if (!check_downscale_t(ref.frameInitLowres, opt.frameInitLowres))
        {
            printf("downscale failed!\n");
            return false;
        }
    }

    if (opt.scale1D_128to64)
    {
        if (!check_scale1D_pp(ref.scale1D_128to64, opt.scale1D_128to64))
        {
            printf("scale1D_128to64 failed!\n");
            return false;
        }
    }

    if (opt.scale2D_64to32)
    {
        if (!check_scale2D_pp(ref.scale2D_64to32, opt.scale2D_64to32))
        {
            printf("scale2D_64to32 failed!\n");
            return false;
        }
    }

    if (opt.ssim_4x4x2_core)
    {
        if (!check_ssim_4x4x2_core(ref.ssim_4x4x2_core, opt.ssim_4x4x2_core))
        {
            printf("ssim_end_4 failed!\n");
            return false;
        }
    }

    if (opt.ssim_end_4)
    {
        if (!check_ssim_end(ref.ssim_end_4, opt.ssim_end_4))
        {
            printf("ssim_end_4 failed!\n");
            return false;
        }
    }

    if (opt.sign)
    {
        if (!check_calSign(ref.sign, opt.sign))
        {
            printf("calSign failed\n");
            return false;
        }
    }

    if (opt.saoCuOrgE0)
    {
        if (!check_saoCuOrgE0_t(ref.saoCuOrgE0, opt.saoCuOrgE0))
        {
            printf("SAO_EO_0 failed\n");
            return false;
        }
    }

    if (opt.saoCuOrgE1)
    {
        if (!check_saoCuOrgE1_t(ref.saoCuOrgE1, opt.saoCuOrgE1))
        {
            printf("SAO_EO_1 failed\n");
            return false;
        }
    }

    if (opt.saoCuOrgE1_2Rows)
    {
        if (!check_saoCuOrgE1_t(ref.saoCuOrgE1_2Rows, opt.saoCuOrgE1_2Rows))
        {
            printf("SAO_EO_1_2Rows failed\n");
            return false;
        }
    }

    if (opt.saoCuOrgE2[0] || opt.saoCuOrgE2[1])
    {
        saoCuOrgE2_t ref1[] = { ref.saoCuOrgE2[0], ref.saoCuOrgE2[1] };
        saoCuOrgE2_t opt1[] = { opt.saoCuOrgE2[0], opt.saoCuOrgE2[1] };

        if (!check_saoCuOrgE2_t(ref1, opt1))
        {
            printf("SAO_EO_2[0] && SAO_EO_2[1] failed\n");
            return false;
        }
    }

    if (opt.saoCuOrgE3[0])
    {
        if (!check_saoCuOrgE3_t(ref.saoCuOrgE3[0], opt.saoCuOrgE3[0]))
        {
            printf("SAO_EO_3[0] failed\n");
            return false;
        }
    }

    if (opt.saoCuOrgE3[1])
    {
        if (!check_saoCuOrgE3_32_t(ref.saoCuOrgE3[1], opt.saoCuOrgE3[1]))
        {
            printf("SAO_EO_3[1] failed\n");
            return false;
        }
    }

    if (opt.saoCuOrgB0)
    {
        if (!check_saoCuOrgB0_t(ref.saoCuOrgB0, opt.saoCuOrgB0))
        {
            printf("SAO_BO_0 failed\n");
            return false;
        }
    }

    if (opt.saoCuStatsBO)
    {
        if (!check_saoCuStatsBO_t(ref.saoCuStatsBO, opt.saoCuStatsBO))
        {
            printf("saoCuStatsBO failed\n");
            return false;
        }
    }

    if (opt.saoCuStatsE0)
    {
        if (!check_saoCuStatsE0_t(ref.saoCuStatsE0, opt.saoCuStatsE0))
        {
            printf("saoCuStatsE0 failed\n");
            return false;
        }
    }

    if (opt.saoCuStatsE1)
    {
        if (!check_saoCuStatsE1_t(ref.saoCuStatsE1, opt.saoCuStatsE1))
        {
            printf("saoCuStatsE1 failed\n");
            return false;
        }
    }

    if (opt.saoCuStatsE2)
    {
        if (!check_saoCuStatsE2_t(ref.saoCuStatsE2, opt.saoCuStatsE2))
        {
            printf("saoCuStatsE2 failed\n");
            return false;
        }
    }

    if (opt.saoCuStatsE3)
    {
        if (!check_saoCuStatsE3_t(ref.saoCuStatsE3, opt.saoCuStatsE3))
        {
            printf("saoCuStatsE3 failed\n");
            return false;
        }
    }

    if (opt.planecopy_sp)
    {
        if (!check_planecopy_sp(ref.planecopy_sp, opt.planecopy_sp))
        {
            printf("planecopy_sp failed\n");
            return false;
        }
    }

    if (opt.planecopy_sp_shl)
    {
        if (!check_planecopy_sp(ref.planecopy_sp_shl, opt.planecopy_sp_shl))
        {
            printf("planecopy_sp_shl failed\n");
            return false;
        }
    }

    if (opt.planecopy_cp)
    {
        if (!check_planecopy_cp(ref.planecopy_cp, opt.planecopy_cp))
        {
            printf("planecopy_cp failed\n");
            return false;
        }
    }

    if (opt.propagateCost)
    {
        if (!check_cutree_propagate_cost(ref.propagateCost, opt.propagateCost))
        {
            printf("propagateCost failed\n");
            return false;
        }
    }

    if (opt.scanPosLast)
    {
        if (!check_scanPosLast(ref.scanPosLast, opt.scanPosLast))
        {
            printf("scanPosLast failed!\n");
            return false;
        }
    }

    if (opt.findPosFirstLast)
    {
        if (!check_findPosFirstLast(ref.findPosFirstLast, opt.findPosFirstLast))
        {
            printf("findPosFirstLast failed!\n");
            return false;
        }
    }

    if (opt.costCoeffNxN)
    {
        if (!check_costCoeffNxN(ref.costCoeffNxN, opt.costCoeffNxN))
        {
            printf("costCoeffNxN failed!\n");
            return false;
        }
    }

    if (opt.costCoeffRemain)
    {
        if (!check_costCoeffRemain(ref.costCoeffRemain, opt.costCoeffRemain))
        {
            printf("costCoeffRemain failed!\n");
            return false;
        }
    }

    if (opt.costC1C2Flag)
    {
        if (!check_costC1C2Flag(ref.costC1C2Flag, opt.costC1C2Flag))
        {
            printf("costC1C2Flag failed!\n");
            return false;
        }
    }
    

    if (opt.planeClipAndMax)
    {
        if (!check_planeClipAndMax(ref.planeClipAndMax, opt.planeClipAndMax))
        {
            printf("planeClipAndMax failed!\n");
            return false;
        }
    }

    return true;
}

void PixelHarness::measurePartition(int part, const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{
    ALIGN_VAR_16(int, cres[16]);
    pixel *fref = pbuf2 + 2 * INCR;
    char header[128];
#define HEADER(str, ...) sprintf(header, str, __VA_ARGS__); printf("%22s", header);

    if (opt.pu[part].satd)
    {
        HEADER("satd[%s]", lumaPartStr[part]);
        REPORT_SPEEDUP(opt.pu[part].satd, ref.pu[part].satd, pbuf1, STRIDE, fref, STRIDE);
    }

    if (opt.pu[part].pixelavg_pp)
    {
        HEADER("avg_pp[%s]", lumaPartStr[part]);
        REPORT_SPEEDUP(opt.pu[part].pixelavg_pp, ref.pu[part].pixelavg_pp, pbuf1, STRIDE, pbuf2, STRIDE, pbuf3, STRIDE, 32);
    }

    if (opt.pu[part].sad)
    {
        HEADER("sad[%s]", lumaPartStr[part]);
        REPORT_SPEEDUP(opt.pu[part].sad, ref.pu[part].sad, pbuf1, STRIDE, fref, STRIDE);
    }

    if (opt.pu[part].sad_x3)
    {
        HEADER("sad_x3[%s]", lumaPartStr[part]);
        REPORT_SPEEDUP(opt.pu[part].sad_x3, ref.pu[part].sad_x3, pbuf1, fref, fref + 1, fref - 1, FENC_STRIDE + 5, &cres[0]);
    }

    if (opt.pu[part].sad_x4)
    {
        HEADER("sad_x4[%s]", lumaPartStr[part]);
        REPORT_SPEEDUP(opt.pu[part].sad_x4, ref.pu[part].sad_x4, pbuf1, fref, fref + 1, fref - 1, fref - INCR, FENC_STRIDE + 5, &cres[0]);
    }

    if (opt.pu[part].copy_pp)
    {
        HEADER("copy_pp[%s]", lumaPartStr[part]);
        REPORT_SPEEDUP(opt.pu[part].copy_pp, ref.pu[part].copy_pp, pbuf1, 64, pbuf2, 64);
    }

    if (opt.pu[part].addAvg)
    {
        HEADER("addAvg[%s]", lumaPartStr[part]);
        REPORT_SPEEDUP(opt.pu[part].addAvg, ref.pu[part].addAvg, sbuf1, sbuf2, pbuf1, STRIDE, STRIDE, STRIDE);
    }

    if (part < NUM_CU_SIZES)
    {
        if (opt.cu[part].sse_pp)
        {
            HEADER("sse_pp[%s]", lumaPartStr[part]);
            REPORT_SPEEDUP(opt.cu[part].sse_pp, ref.cu[part].sse_pp, pbuf1, STRIDE, fref, STRIDE);
        }

        if (opt.cu[part].sse_ss)
        {
            HEADER("sse_ss[%s]", lumaPartStr[part]);
            REPORT_SPEEDUP(opt.cu[part].sse_ss, ref.cu[part].sse_ss, (int16_t*)pbuf1, STRIDE, (int16_t*)fref, STRIDE);
        }
        if (opt.cu[part].sub_ps)
        {
            HEADER("sub_ps[%s]", lumaPartStr[part]);
            REPORT_SPEEDUP(opt.cu[part].sub_ps, ref.cu[part].sub_ps, (int16_t*)pbuf1, FENC_STRIDE, pbuf2, pbuf1, STRIDE, STRIDE);
        }
        if (opt.cu[part].add_ps)
        {
            HEADER("add_ps[%s]", lumaPartStr[part]);
            REPORT_SPEEDUP(opt.cu[part].add_ps, ref.cu[part].add_ps, pbuf1, FENC_STRIDE, pbuf2, sbuf1, STRIDE, STRIDE);
        }
        if (opt.cu[part].copy_ss)
        {
            HEADER("copy_ss[%s]", lumaPartStr[part]);
            REPORT_SPEEDUP(opt.cu[part].copy_ss, ref.cu[part].copy_ss, sbuf1, 128, sbuf2, 128);
        }
        if (opt.cu[part].copy_sp)
        {
            HEADER("copy_sp[%s]", lumaPartStr[part]);
            REPORT_SPEEDUP(opt.cu[part].copy_sp, ref.cu[part].copy_sp, pbuf1, 64, sbuf3, 128);
        }
        if (opt.cu[part].copy_ps)
        {
            HEADER("copy_ps[%s]", lumaPartStr[part]);
            REPORT_SPEEDUP(opt.cu[part].copy_ps, ref.cu[part].copy_ps, sbuf1, 128, pbuf1, 64);
        }
    }

    for (int i = 0; i < X265_CSP_COUNT; i++)
    {
        if (opt.chroma[i].pu[part].copy_pp)
        {
            HEADER("[%s] copy_pp[%s]", x265_source_csp_names[i], chromaPartStr[i][part]);
            REPORT_SPEEDUP(opt.chroma[i].pu[part].copy_pp, ref.chroma[i].pu[part].copy_pp, pbuf1, 64, pbuf2, 128);
        }
        if (opt.chroma[i].pu[part].addAvg)
        {
            HEADER("[%s]  addAvg[%s]", x265_source_csp_names[i], chromaPartStr[i][part]);
            REPORT_SPEEDUP(opt.chroma[i].pu[part].addAvg, ref.chroma[i].pu[part].addAvg, sbuf1, sbuf2, pbuf1, STRIDE, STRIDE, STRIDE);
        }
        if (opt.chroma[i].pu[part].satd)
        {
            HEADER("[%s] satd[%s]", x265_source_csp_names[i], chromaPartStr[i][part]);
            REPORT_SPEEDUP(opt.chroma[i].pu[part].satd, ref.chroma[i].pu[part].satd, pbuf1, STRIDE, fref, STRIDE);
        }
        if (part < NUM_CU_SIZES)
        {
            if (opt.chroma[i].cu[part].copy_ss)
            {
                HEADER("[%s] copy_ss[%s]", x265_source_csp_names[i], chromaPartStr[i][part]);
                REPORT_SPEEDUP(opt.chroma[i].cu[part].copy_ss, ref.chroma[i].cu[part].copy_ss, sbuf1, 64, sbuf2, 128);
            }
            if (opt.chroma[i].cu[part].copy_ps)
            {
                HEADER("[%s] copy_ps[%s]", x265_source_csp_names[i], chromaPartStr[i][part]);
                REPORT_SPEEDUP(opt.chroma[i].cu[part].copy_ps, ref.chroma[i].cu[part].copy_ps, sbuf1, 64, pbuf1, 128);
            }
            if (opt.chroma[i].cu[part].copy_sp)
            {
                HEADER("[%s] copy_sp[%s]", x265_source_csp_names[i], chromaPartStr[i][part]);
                REPORT_SPEEDUP(opt.chroma[i].cu[part].copy_sp, ref.chroma[i].cu[part].copy_sp, pbuf1, 64, sbuf3, 128);
            }
            if (opt.chroma[i].cu[part].sse_pp)
            {
                HEADER("[%s] sse_pp[%s]", x265_source_csp_names[i], chromaPartStr[i][part]);
                REPORT_SPEEDUP(opt.chroma[i].cu[part].sse_pp, ref.chroma[i].cu[part].sse_pp, pbuf1, STRIDE, fref, STRIDE);
            }
            if (opt.chroma[i].cu[part].sub_ps)
            {
                HEADER("[%s]  sub_ps[%s]", x265_source_csp_names[i], chromaPartStr[i][part]);
                REPORT_SPEEDUP(opt.chroma[i].cu[part].sub_ps, ref.chroma[i].cu[part].sub_ps, (int16_t*)pbuf1, FENC_STRIDE, pbuf2, pbuf1, STRIDE, STRIDE);
            }
            if (opt.chroma[i].cu[part].add_ps)
            {
                HEADER("[%s]  add_ps[%s]", x265_source_csp_names[i], chromaPartStr[i][part]);
                REPORT_SPEEDUP(opt.chroma[i].cu[part].add_ps, ref.chroma[i].cu[part].add_ps, pbuf1, FENC_STRIDE, pbuf2, sbuf1, STRIDE, STRIDE);
            }
            if (opt.chroma[i].cu[part].sa8d)
            {
                HEADER("[%s] sa8d[%s]", x265_source_csp_names[i], chromaPartStr[i][part]);
                REPORT_SPEEDUP(opt.chroma[i].cu[part].sa8d, ref.chroma[i].cu[part].sa8d, pbuf1, STRIDE, pbuf2, STRIDE);
            }
        }
    }

#undef HEADER
}

void PixelHarness::measureSpeed(const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{
    char header[128];

#define HEADER(str, ...) sprintf(header, str, __VA_ARGS__); printf("%22s", header);
#define HEADER0(str) printf("%22s", str);

    for (int size = 4; size <= 64; size *= 2)
    {
        int part = partitionFromSizes(size, size); // 2Nx2N
        measurePartition(part, ref, opt);

        if (size > 4)
        {
            part = partitionFromSizes(size, size >> 1); // 2NxN
            measurePartition(part, ref, opt);
            part = partitionFromSizes(size >> 1, size); // Nx2N
            measurePartition(part, ref, opt);
        }
        if (size > 8)
        {
            // 4 AMP modes
            part = partitionFromSizes(size, size >> 2);
            measurePartition(part, ref, opt);
            part = partitionFromSizes(size, 3 * (size >> 2));
            measurePartition(part, ref, opt);

            part = partitionFromSizes(size >> 2, size);
            measurePartition(part, ref, opt);
            part = partitionFromSizes(3 * (size >> 2), size);
            measurePartition(part, ref, opt);
        }
    }

    for (int i = 0; i < NUM_CU_SIZES; i++)
    {
        if ((i <= BLOCK_32x32) && opt.cu[i].ssd_s)
        {
            HEADER("ssd_s[%dx%d]", 4 << i, 4 << i);
            REPORT_SPEEDUP(opt.cu[i].ssd_s, ref.cu[i].ssd_s, sbuf1, STRIDE);
        }
        if (opt.cu[i].sa8d)
        {
            HEADER("sa8d[%dx%d]", 4 << i, 4 << i);
            REPORT_SPEEDUP(opt.cu[i].sa8d, ref.cu[i].sa8d, pbuf1, STRIDE, pbuf2, STRIDE);
        }
        if (opt.cu[i].calcresidual)
        {
            HEADER("residual[%dx%d]", 4 << i, 4 << i);
            REPORT_SPEEDUP(opt.cu[i].calcresidual, ref.cu[i].calcresidual, pbuf1, pbuf2, sbuf1, 64);
        }

        if (opt.cu[i].blockfill_s)
        {
            HEADER("blkfill[%dx%d]", 4 << i, 4 << i);
            REPORT_SPEEDUP(opt.cu[i].blockfill_s, ref.cu[i].blockfill_s, sbuf1, 64, SHORT_MAX);
        }

        if (opt.cu[i].transpose)
        {
            HEADER("transpose[%dx%d]", 4 << i, 4 << i);
            REPORT_SPEEDUP(opt.cu[i].transpose, ref.cu[i].transpose, pbuf1, pbuf2, STRIDE);
        }

        if (opt.cu[i].var)
        {
            HEADER("var[%dx%d]", 4 << i, 4 << i);
            REPORT_SPEEDUP(opt.cu[i].var, ref.cu[i].var, pbuf1, STRIDE);
        }

        if ((i < BLOCK_64x64) && opt.cu[i].cpy2Dto1D_shl)
        {
            HEADER("cpy2Dto1D_shl[%dx%d]", 4 << i, 4 << i);
            const int shift = MAX_TR_DYNAMIC_RANGE - X265_DEPTH - (i + 2);
            REPORT_SPEEDUP(opt.cu[i].cpy2Dto1D_shl, ref.cu[i].cpy2Dto1D_shl, sbuf1, sbuf2, STRIDE, X265_MAX(0, shift));
        }

        if ((i < BLOCK_64x64) && opt.cu[i].cpy2Dto1D_shr)
        {
            HEADER("cpy2Dto1D_shr[%dx%d]", 4 << i, 4 << i);
            REPORT_SPEEDUP(opt.cu[i].cpy2Dto1D_shr, ref.cu[i].cpy2Dto1D_shr, sbuf1, sbuf2, STRIDE, 3);
        }

        if ((i < BLOCK_64x64) && opt.cu[i].cpy1Dto2D_shl)
        {
            HEADER("cpy1Dto2D_shl[%dx%d]", 4 << i, 4 << i);
            REPORT_SPEEDUP(opt.cu[i].cpy1Dto2D_shl, ref.cu[i].cpy1Dto2D_shl, sbuf1, sbuf2, STRIDE, 64);
        }

        if ((i < BLOCK_64x64) && opt.cu[i].cpy1Dto2D_shr)
        {
            HEADER("cpy1Dto2D_shr[%dx%d]", 4 << i, 4 << i);
            REPORT_SPEEDUP(opt.cu[i].cpy1Dto2D_shr, ref.cu[i].cpy1Dto2D_shr, sbuf1, sbuf2, STRIDE, 64);
        }

        if ((i < BLOCK_64x64) && opt.cu[i].copy_cnt)
        {
            HEADER("copy_cnt[%dx%d]", 4 << i, 4 << i);
            REPORT_SPEEDUP(opt.cu[i].copy_cnt, ref.cu[i].copy_cnt, sbuf1, sbuf2, STRIDE);
        }

        if (opt.cu[i].psy_cost_pp)
        {
            HEADER("psy_cost_pp[%dx%d]", 4 << i, 4 << i);
            REPORT_SPEEDUP(opt.cu[i].psy_cost_pp, ref.cu[i].psy_cost_pp, pbuf1, STRIDE, pbuf2, STRIDE);
        }

        if (opt.cu[i].psy_cost_ss)
        {
            HEADER("psy_cost_ss[%dx%d]", 4 << i, 4 << i);
            REPORT_SPEEDUP(opt.cu[i].psy_cost_ss, ref.cu[i].psy_cost_ss, sbuf1, STRIDE, sbuf2, STRIDE);
        }
    }

    if (opt.weight_pp)
    {
        HEADER0("weight_pp");
        REPORT_SPEEDUP(opt.weight_pp, ref.weight_pp, pbuf1, pbuf2, 64, 32, 32, 128, 1 << 9, 10, 100);
    }

    if (opt.weight_sp)
    {
        HEADER0("weight_sp");
        REPORT_SPEEDUP(opt.weight_sp, ref.weight_sp, (int16_t*)sbuf1, pbuf1, 64, 64, 32, 32, 128, 1 << 9, 10, 100);
    }

    if (opt.frameInitLowres)
    {
        HEADER0("downscale");
        REPORT_SPEEDUP(opt.frameInitLowres, ref.frameInitLowres, pbuf2, pbuf1, pbuf2, pbuf3, pbuf4, 64, 64, 64, 64);
    }

    if (opt.scale1D_128to64)
    {
        HEADER0("scale1D_128to64");
        REPORT_SPEEDUP(opt.scale1D_128to64, ref.scale1D_128to64, pbuf2, pbuf1);
    }

    if (opt.scale2D_64to32)
    {
        HEADER0("scale2D_64to32");
        REPORT_SPEEDUP(opt.scale2D_64to32, ref.scale2D_64to32, pbuf2, pbuf1, 64);
    }

    if (opt.ssim_4x4x2_core)
    {
        HEADER0("ssim_4x4x2_core");
        REPORT_SPEEDUP(opt.ssim_4x4x2_core, ref.ssim_4x4x2_core, pbuf1, 64, pbuf2, 64, (int(*)[4])sbuf1);
    }

    if (opt.ssim_end_4)
    {
        HEADER0("ssim_end_4");
        REPORT_SPEEDUP(opt.ssim_end_4, ref.ssim_end_4, (int(*)[4])pbuf2, (int(*)[4])pbuf1, 4);
    }

    if (opt.sign)
    {
        HEADER0("calSign");
        REPORT_SPEEDUP(opt.sign, ref.sign, psbuf1, pbuf1, pbuf2, 64);
    }

    if (opt.saoCuOrgE0)
    {
        HEADER0("SAO_EO_0");
        REPORT_SPEEDUP(opt.saoCuOrgE0, ref.saoCuOrgE0, pbuf1, psbuf1, 64, psbuf2, 64);
    }

    if (opt.saoCuOrgE1)
    {
        HEADER0("SAO_EO_1");
        REPORT_SPEEDUP(opt.saoCuOrgE1, ref.saoCuOrgE1, pbuf1, psbuf2, psbuf1, 64, 64);
    }

    if (opt.saoCuOrgE1_2Rows)
    {
        HEADER0("SAO_EO_1_2Rows");
        REPORT_SPEEDUP(opt.saoCuOrgE1_2Rows, ref.saoCuOrgE1_2Rows, pbuf1, psbuf2, psbuf1, 64, 64);
    }

    if (opt.saoCuOrgE2[0])
    {
        HEADER0("SAO_EO_2[0]");
        REPORT_SPEEDUP(opt.saoCuOrgE2[0], ref.saoCuOrgE2[0], pbuf1, psbuf1, psbuf2, psbuf3, 16, 64);
    }

    if (opt.saoCuOrgE2[1])
    {
        HEADER0("SAO_EO_2[1]");
        REPORT_SPEEDUP(opt.saoCuOrgE2[1], ref.saoCuOrgE2[1], pbuf1, psbuf1, psbuf2, psbuf3, 64, 64);
    }

    if (opt.saoCuOrgE3[0])
    {
        HEADER0("SAO_EO_3[0]");
        REPORT_SPEEDUP(opt.saoCuOrgE3[0], ref.saoCuOrgE3[0], pbuf1, psbuf2, psbuf1, 64, 0, 16);
    }

    if (opt.saoCuOrgE3[1])
    {
        HEADER0("SAO_EO_3[1]");
        REPORT_SPEEDUP(opt.saoCuOrgE3[1], ref.saoCuOrgE3[1], pbuf1, psbuf2, psbuf1, 64, 0, 64);
    }

    if (opt.saoCuOrgB0)
    {
        HEADER0("SAO_BO_0");
        REPORT_SPEEDUP(opt.saoCuOrgB0, ref.saoCuOrgB0, pbuf1, psbuf1, 64, 64, 64);
    }

    if (opt.saoCuStatsBO)
    {
        int32_t stats[33], count[33];
        HEADER0("saoCuStatsBO");
        REPORT_SPEEDUP(opt.saoCuStatsBO, ref.saoCuStatsBO, pbuf2, pbuf3, 64, 60, 61, stats, count);
    }

    if (opt.saoCuStatsE0)
    {
        int32_t stats[33], count[33];
        HEADER0("saoCuStatsE0");
        REPORT_SPEEDUP(opt.saoCuStatsE0, ref.saoCuStatsE0, pbuf2, pbuf3, 64, 60, 61, stats, count);
    }

    if (opt.saoCuStatsE1)
    {
        int32_t stats[5], count[5];
        int8_t upBuff1[MAX_CU_SIZE + 2];
        memset(upBuff1, 1, sizeof(upBuff1));
        HEADER0("saoCuStatsE1");
        REPORT_SPEEDUP(opt.saoCuStatsE1, ref.saoCuStatsE1, pbuf2, pbuf3, 64, upBuff1 + 1,60, 61, stats, count);
    }

    if (opt.saoCuStatsE2)
    {
        int32_t stats[5], count[5];
        int8_t upBuff1[MAX_CU_SIZE + 2];
        int8_t upBufft[MAX_CU_SIZE + 2];
        memset(upBuff1, 1, sizeof(upBuff1));
        memset(upBufft, -1, sizeof(upBufft));
        HEADER0("saoCuStatsE2");
        REPORT_SPEEDUP(opt.saoCuStatsE2, ref.saoCuStatsE2, pbuf2, pbuf3, 64, upBuff1 + 1, upBufft + 1, 60, 61, stats, count);
    }

    if (opt.saoCuStatsE3)
    {
        int8_t upBuff1[MAX_CU_SIZE + 2];
        int32_t stats[5], count[5];
        memset(upBuff1, 1, sizeof(upBuff1));
        HEADER0("saoCuStatsE3");
        REPORT_SPEEDUP(opt.saoCuStatsE3, ref.saoCuStatsE3, pbuf2, pbuf3, 64, upBuff1 + 1, 60, 61, stats, count);
    }

    if (opt.planecopy_sp)
    {
        HEADER0("planecopy_sp");
        REPORT_SPEEDUP(opt.planecopy_sp, ref.planecopy_sp, ushort_test_buff[0], 64, pbuf1, 64, 64, 64, 8, 255);
    }

    if (opt.planecopy_cp)
    {
        HEADER0("planecopy_cp");
        REPORT_SPEEDUP(opt.planecopy_cp, ref.planecopy_cp, uchar_test_buff[0], 64, pbuf1, 64, 64, 64, 2);
    }

    if (opt.propagateCost)
    {
        HEADER0("propagateCost");
        REPORT_SPEEDUP(opt.propagateCost, ref.propagateCost, ibuf1, ushort_test_buff[0], int_test_buff[0], ushort_test_buff[0], int_test_buff[0], double_test_buff[0], 80);
    }

    if (opt.scanPosLast)
    {
        HEADER0("scanPosLast");
        coeff_t coefBuf[32 * 32];
        memset(coefBuf, 0, sizeof(coefBuf));
        memset(coefBuf + 32 * 31, 1, 32 * sizeof(coeff_t));
        REPORT_SPEEDUP(opt.scanPosLast, ref.scanPosLast, g_scanOrder[SCAN_DIAG][NUM_SCAN_SIZE - 1], coefBuf, (uint16_t*)sbuf1, (uint16_t*)sbuf2, (uint8_t*)psbuf1, 32, g_scan4x4[SCAN_DIAG], 32);
    }

    if (opt.findPosFirstLast)
    {
        HEADER0("findPosFirstLast");
        coeff_t coefBuf[32 * MLS_CG_SIZE];
        memset(coefBuf, 0, sizeof(coefBuf));
        // every CG can't be all zeros!
        coefBuf[3 + 0 * 32] = 0x0BAD;
        coefBuf[3 + 1 * 32] = 0x0BAD;
        coefBuf[3 + 2 * 32] = 0x0BAD;
        coefBuf[3 + 3 * 32] = 0x0BAD;
        REPORT_SPEEDUP(opt.findPosFirstLast, ref.findPosFirstLast, coefBuf, 32, g_scan4x4[SCAN_DIAG]);
    }

    if (opt.costCoeffNxN)
    {
        HEADER0("costCoeffNxN");
        coeff_t coefBuf[32 * 32];
        uint16_t tmpOut[16];
        memset(coefBuf, 1, sizeof(coefBuf));
        ALIGN_VAR_32(static uint8_t const, ctxSig[]) =
        {
            0, 1, 4, 5,
            2, 3, 4, 5,
            6, 6, 8, 8,
            7, 7, 8, 8
        };
        uint8_t ctx[OFF_SIG_FLAG_CTX + NUM_SIG_FLAG_CTX_LUMA];
        memset(ctx, 120, sizeof(ctx));

        REPORT_SPEEDUP(opt.costCoeffNxN, ref.costCoeffNxN, g_scan4x4[SCAN_DIAG], coefBuf, 32, tmpOut, ctxSig, 0xFFFF, ctx, 1, 15, 32);
    }

    if (opt.costCoeffRemain)
    {
        HEADER0("costCoeffRemain");
        uint16_t abscoefBuf[32 * 32];
        memset(abscoefBuf, 0, sizeof(abscoefBuf));
        memset(abscoefBuf + 32 * 31, 1, 32 * sizeof(uint16_t));
        REPORT_SPEEDUP(opt.costCoeffRemain, ref.costCoeffRemain, abscoefBuf, 16, 3);
    }

    if (opt.costC1C2Flag)
    {
        HEADER0("costC1C2Flag");
        ALIGN_VAR_32(uint16_t, abscoefBuf[C1FLAG_NUMBER]);
        memset(abscoefBuf, 1, sizeof(abscoefBuf));
        abscoefBuf[C1FLAG_NUMBER - 2] = 2;
        abscoefBuf[C1FLAG_NUMBER - 1] = 3;
        REPORT_SPEEDUP(opt.costC1C2Flag, ref.costC1C2Flag, abscoefBuf, C1FLAG_NUMBER, (uint8_t*)psbuf1, 1);
    }

    if (opt.planeClipAndMax)
    {
        HEADER0("planeClipAndMax");
        uint64_t dummy;
        REPORT_SPEEDUP(opt.planeClipAndMax, ref.planeClipAndMax, pbuf1, 128, 63, 62, &dummy, 1, PIXEL_MAX - 1);
    }
}
