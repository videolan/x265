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

using namespace x265;

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
        psbuf1[i] = (rand() % 65) - 32;                   // range is between -32 to 32
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

bool PixelHarness::check_pixelcmp_sp(pixelcmp_sp_t ref, pixelcmp_sp_t opt)
{
    int j = 0;
    intptr_t stride = STRIDE;

    for (int i = 0; i < ITERS; i++)
    {
        int index1 = rand() % TEST_CASES;
        int index2 = rand() % TEST_CASES;
        int vres = (int)checked(opt, short_test_buff[index1], stride, pixel_test_buff[index2] + j, stride);
        int cres = ref(short_test_buff[index1], stride, pixel_test_buff[index2] + j, stride);
        if (vres != cres)
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_pixelcmp_ss(pixelcmp_ss_t ref, pixelcmp_ss_t opt)
{
    int j = 0;
    intptr_t stride = STRIDE;

    for (int i = 0; i < ITERS; i++)
    {
        int index1 = rand() % TEST_CASES;
        int index2 = rand() % TEST_CASES;
        int vres = (int)checked(opt, short_test_buff[index1], stride, short_test_buff[index2] + j, stride);
        int cres = ref(short_test_buff[index1], stride, short_test_buff[index2] + j, stride);
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

bool PixelHarness::check_blockcopy_pp(blockcpy_pp_t ref, blockcpy_pp_t opt)
{
    ALIGN_VAR_16(pixel, ref_dest[64 * 64]);
    ALIGN_VAR_16(pixel, opt_dest[64 * 64]);
    int bx = 64;
    int by = 64;
    int j = 0;
    intptr_t stride1 = 64, stride2 = 128;
    for (int i = 0; i < ITERS; i++)
    {
        int index = i % TEST_CASES;
        checked(opt, bx, by, opt_dest, stride1, pixel_test_buff[index] + j, stride2);
        ref(bx, by, ref_dest, stride1, pixel_test_buff[index] + j, stride2);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(pixel)))
            return false;

        reportfail();
        j += 4;
        bx = 4 * ((rand() & 15) + 1);
        by = 4 * ((rand() & 15) + 1);
    }

    return true;
}

bool PixelHarness::check_blockcopy_ps(blockcpy_ps_t ref, blockcpy_ps_t opt)
{
    ALIGN_VAR_16(pixel, ref_dest[64 * 64]);
    ALIGN_VAR_16(pixel, opt_dest[64 * 64]);
    int bx = 64;
    int by = 64;
    int j = 0;
    intptr_t stride1 = 64, stride2 = STRIDE;
    for (int i = 0; i < ITERS; i++)
    {
        int index = i % TEST_CASES;
        checked(opt, bx, by, opt_dest, stride1, short_test_buff1[index] + j, stride2);
        ref(bx, by, ref_dest, stride1, short_test_buff1[index] + j, stride2);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(pixel)))
            return false;

        reportfail();
        j += 4;
        bx = 4 * ((rand() & 15) + 1);
        by = 4 * ((rand() & 15) + 1);
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

bool PixelHarness::check_calcrecon(calcrecon_t ref, calcrecon_t opt)
{
    ALIGN_VAR_16(int16_t, ref_recq[64 * 64 * 2]);
    ALIGN_VAR_16(int16_t, opt_recq[64 * 64 * 2]);

    ALIGN_VAR_16(pixel, ref_reco[64 * 64 * 2]);
    ALIGN_VAR_16(pixel, opt_reco[64 * 64 * 2]);

    ALIGN_VAR_16(pixel, ref_pred[64 * 64 * 2]);
    ALIGN_VAR_16(pixel, opt_pred[64 * 64 * 2]);

    memset(ref_recq, 0xCD, 64 * 64 * 2 * sizeof(int16_t));
    memset(opt_recq, 0xCD, 64 * 64 * 2 * sizeof(int16_t));
    memset(ref_reco, 0xCD, 64 * 64 * 2 * sizeof(pixel));
    memset(opt_reco, 0xCD, 64 * 64 * 2 * sizeof(pixel));
    memset(ref_pred, 0xCD, 64 * 64 * 2 * sizeof(pixel));
    memset(opt_pred, 0xCD, 64 * 64 * 2 * sizeof(pixel));

    int j = 0;
    for (int i = 0; i < ITERS; i++)
    {
        // NOTE: stride must be multiple of 16, because minimum block is 4x4
        int stride0 = (STRIDE + (rand() % STRIDE)) & ~15;
        int stride1 = (STRIDE + (rand() % STRIDE)) & ~15;
        int stride2 = (STRIDE + (rand() % STRIDE)) & ~15;
        int index1 = rand() % TEST_CASES;
        int index2 = rand() % TEST_CASES;
        ref(pixel_test_buff[index1] + j, short_test_buff[index2] + j, ref_recq, ref_pred, stride0, stride1, stride2);
        checked(opt, pixel_test_buff[index1] + j, short_test_buff[index2] + j, opt_recq, opt_pred, stride0, stride1, stride2);

        if (memcmp(ref_recq, opt_recq, 64 * stride0 * sizeof(int16_t)))
        {
            return false;
        }
        if (memcmp(ref_pred, opt_pred, 64 * stride2 * sizeof(pixel)))
        {
            return false;
        }

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
        {
            return false;
        }

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_weightp(weightp_sp_t ref, weightp_sp_t opt)
{
    ALIGN_VAR_16(pixel, ref_dest[64 * 64]);
    ALIGN_VAR_16(pixel, opt_dest[64 * 64]);

    memset(ref_dest, 0, 64 * 64 * sizeof(pixel));
    memset(opt_dest, 0, 64 * 64 * sizeof(pixel));
    int j = 0;
    int width = 2 * (rand() % 32 + 1);
    int height = 8;
    int w0 = rand() % 128;
    int shift = rand() % 15;
    int round = shift ? (1 << (shift - 1)) : 0;
    int offset = (rand() % 256) - 128;
    intptr_t stride = 64;
    for (int i = 0; i < ITERS; i++)
    {
        int index = i % TEST_CASES;
        checked(opt, short_test_buff[index] + j, opt_dest, stride, stride, width, height, w0, round, shift, offset);
        ref(short_test_buff[index] + j, ref_dest, stride, stride, width, height, w0, round, shift, offset);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(pixel)))
            return false;

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
    int shift = rand() % 15;
    int round = shift ? (1 << (shift - 1)) : 0;
    int offset = (rand() % 256) - 128;
    intptr_t stride = 64;
    for (int i = 0; i < ITERS; i++)
    {
        int index = i % TEST_CASES;
        checked(opt, pixel_test_buff[index] + j, opt_dest, stride, stride, width, height, w0, round, shift, offset);
        ref(pixel_test_buff[index] + j, ref_dest, stride, stride, width, height, w0, round, shift, offset);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(pixel)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_pixeladd_ss(pixeladd_ss_t ref, pixeladd_ss_t opt)
{
    ALIGN_VAR_16(int16_t, ref_dest[64 * 64]);
    ALIGN_VAR_16(int16_t, opt_dest[64 * 64]);
    int bx = 64;
    int by = 64;
    int j = 0;
    intptr_t stride = 64;
    for (int i = 0; i < ITERS; i++)
    {
        int index1 = rand() % TEST_CASES;
        int index2 = rand() % TEST_CASES;
        checked(opt, bx, by, opt_dest, stride, short_test_buff[index1] + j,
                short_test_buff[index2] + j, stride, stride);
        ref(bx, by, ref_dest, stride, short_test_buff[index1] + j,
            short_test_buff[index2] + j, stride, stride);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(int16_t)))
            return false;

        reportfail();
        j += INCR;
        bx = 4 * ((rand() & 15) + 1);
        by = 4 * ((rand() & 15) + 1);
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

bool PixelHarness::check_cvt32to16_shr_t(cvt32to16_shr_t ref, cvt32to16_shr_t opt)
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
        checked(opt, opt_dest, int_test_buff[index] + j, stride, shift, (int)STRIDE);
        ref(ref_dest, int_test_buff[index] + j, stride, shift, (int)STRIDE);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(int16_t)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_cvt16to32_shl_t(cvt16to32_shl_t ref, cvt16to32_shl_t opt)
{
    ALIGN_VAR_16(int32_t, ref_dest[64 * 64]);
    ALIGN_VAR_16(int32_t, opt_dest[64 * 64]);

    int j = 0;
    intptr_t stride = STRIDE;
    for (int i = 0; i < ITERS; i++)
    {
        int shift = (rand() % 7 + 1);

        int index = i % TEST_CASES;
        checked(opt, opt_dest, short_test_buff[index] + j, stride, shift, (int)stride);
        ref(ref_dest, short_test_buff[index] + j, stride, shift, (int)stride);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(int32_t)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_cvt16to32_shr_t(cvt16to32_shr_t ref, cvt16to32_shr_t opt)
{
    ALIGN_VAR_16(int32_t, ref_dest[64 * 64]);
    ALIGN_VAR_16(int32_t, opt_dest[64 * 64]);

    memset(ref_dest, 0xCD, sizeof(ref_dest));
    memset(opt_dest, 0xCD, sizeof(opt_dest));

    int j = 0;
    intptr_t stride = STRIDE;
    for (int i = 0; i < ITERS; i++)
    {
        int shift = (rand() % 7 + 1);

        int index = i % TEST_CASES;
        checked(opt, opt_dest, short_test_buff[index] + j, stride, shift, (int)stride);
        ref(ref_dest, short_test_buff[index] + j, stride, shift, (int)stride);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(int32_t)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_cvt32to16_shl_t(cvt32to16_shl_t ref, cvt32to16_shl_t opt)
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
        checked(opt, opt_dest, int_test_buff[index] + j, stride, shift);
        ref(ref_dest, int_test_buff[index] + j, stride, shift);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(int16_t)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_cvt16to32_cnt_t(cvt16to32_cnt_t ref, cvt16to32_cnt_t opt)
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

bool PixelHarness::check_scale_pp(scale_t ref, scale_t opt)
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

/* TODO: This function causes crashes when checked. Is this a real bug? */
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

bool PixelHarness::check_saoCuOrgE0_t(saoCuOrgE0_t ref, saoCuOrgE0_t opt)
{
    ALIGN_VAR_16(pixel, ref_dest[64 * 64]);
    ALIGN_VAR_16(pixel, opt_dest[64 * 64]);

    memset(ref_dest, 0xCD, sizeof(ref_dest));
    memset(opt_dest, 0xCD, sizeof(opt_dest));

    int j = 0;

    for (int i = 0; i < ITERS; i++)
    {
        int width = 16 * (rand() % 4 + 1);
        int8_t sign = rand() % 3;
        if (sign == 2)
        {
            sign = -1;
        }

        ref(ref_dest, psbuf1 + j, width, sign);
        checked(opt, opt_dest, psbuf1 + j, width, sign);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(pixel)))
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

    int width = 16 + rand() % 48;
    int height = 16 + rand() % 48;
    intptr_t srcStride = 64;
    intptr_t dstStride = width;
    int j = 0;

    for (int i = 0; i < ITERS; i++)
    {
        int index = i % TEST_CASES;
        checked(opt, ushort_test_buff[index] + j, srcStride, opt_dest, dstStride, width, height, (int)8, (uint16_t)255);
        ref(ushort_test_buff[index] + j, srcStride, ref_dest, dstStride, width, height, (int)8, (uint16_t)255);

        if (memcmp(ref_dest, opt_dest, width * height * sizeof(pixel)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::check_planecopy_cp(planecopy_cp_t ref, planecopy_cp_t opt)
{
    ALIGN_VAR_16(pixel, ref_dest[64 * 64]);
    ALIGN_VAR_16(pixel, opt_dest[64 * 64]);

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

        if (memcmp(ref_dest, opt_dest, width * height * sizeof(pixel)))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool PixelHarness::testPartition(int part, const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{
    if (opt.satd[part])
    {
        if (!check_pixelcmp(ref.satd[part], opt.satd[part]))
        {
            printf("satd[%s]: failed!\n", lumaPartStr[part]);
            return false;
        }
    }

    if (opt.sa8d_inter[part])
    {
        if (!check_pixelcmp(ref.sa8d_inter[part], opt.sa8d_inter[part]))
        {
            printf("sa8d_inter[%s]: failed!\n", lumaPartStr[part]);
            return false;
        }
    }

    if (opt.sad[part])
    {
        if (!check_pixelcmp(ref.sad[part], opt.sad[part]))
        {
            printf("sad[%s]: failed!\n", lumaPartStr[part]);
            return false;
        }
    }

    if (opt.sse_pp[part])
    {
        if (!check_pixelcmp(ref.sse_pp[part], opt.sse_pp[part]))
        {
            printf("sse_pp[%s]: failed!\n", lumaPartStr[part]);
            return false;
        }
    }

    if (opt.sse_sp[part])
    {
        if (!check_pixelcmp_sp(ref.sse_sp[part], opt.sse_sp[part]))
        {
            printf("sse_sp[%s]: failed!\n", lumaPartStr[part]);
            return false;
        }
    }

    if (opt.sse_ss[part])
    {
        if (!check_pixelcmp_ss(ref.sse_ss[part], opt.sse_ss[part]))
        {
            printf("sse_ss[%s]: failed!\n", lumaPartStr[part]);
            return false;
        }
    }

    if (opt.sad_x3[part])
    {
        if (!check_pixelcmp_x3(ref.sad_x3[part], opt.sad_x3[part]))
        {
            printf("sad_x3[%s]: failed!\n", lumaPartStr[part]);
            return false;
        }
    }

    if (opt.sad_x4[part])
    {
        if (!check_pixelcmp_x4(ref.sad_x4[part], opt.sad_x4[part]))
        {
            printf("sad_x4[%s]: failed!\n", lumaPartStr[part]);
            return false;
        }
    }

    if (opt.pixelavg_pp[part])
    {
        if (!check_pixelavg_pp(ref.pixelavg_pp[part], opt.pixelavg_pp[part]))
        {
            printf("pixelavg_pp[%s]: failed!\n", lumaPartStr[part]);
            return false;
        }
    }

    if (opt.luma_copy_pp[part])
    {
        if (!check_copy_pp(ref.luma_copy_pp[part], opt.luma_copy_pp[part]))
        {
            printf("luma_copy_pp[%s] failed\n", lumaPartStr[part]);
            return false;
        }
    }

    if (opt.luma_copy_sp[part])
    {
        if (!check_copy_sp(ref.luma_copy_sp[part], opt.luma_copy_sp[part]))
        {
            printf("luma_copy_sp[%s] failed\n", lumaPartStr[part]);
            return false;
        }
    }

    if (opt.luma_copy_ps[part])
    {
        if (!check_copy_ps(ref.luma_copy_ps[part], opt.luma_copy_ps[part]))
        {
            printf("luma_copy_ps[%s] failed\n", lumaPartStr[part]);
            return false;
        }
    }

    if (opt.luma_copy_ss[part])
    {
        if (!check_copy_ss(ref.luma_copy_ss[part], opt.luma_copy_ss[part]))
        {
            printf("luma_copy_ss[%s] failed\n", lumaPartStr[part]);
            return false;
        }
    }

    if (opt.luma_addAvg[part])
    {
        if (!check_addAvg(ref.luma_addAvg[part], opt.luma_addAvg[part]))
        {
            printf("luma_addAvg[%s] failed\n", lumaPartStr[part]);
            return false;
        }
    }

    if (part < NUM_SQUARE_BLOCKS)
    {
        if (opt.luma_sub_ps[part])
        {
            if (!check_pixel_sub_ps(ref.luma_sub_ps[part], opt.luma_sub_ps[part]))
            {
                printf("luma_sub_ps[%s] failed\n", lumaPartStr[part]);
                return false;
            }
        }

        if (opt.luma_add_ps[part])
        {
            if (!check_pixel_add_ps(ref.luma_add_ps[part], opt.luma_add_ps[part]))
            {
                printf("luma_add_ps[%s] failed\n", lumaPartStr[part]);
                return false;
            }
        }
    }

    for (int i = 0; i < X265_CSP_COUNT; i++)
    {
        if (opt.chroma[i].copy_pp[part])
        {
            if (!check_copy_pp(ref.chroma[i].copy_pp[part], opt.chroma[i].copy_pp[part]))
            {
                printf("chroma_copy_pp[%s][%s] failed\n", x265_source_csp_names[i], chromaPartStr[i][part]);
                return false;
            }
        }
        if (opt.chroma[i].copy_sp[part])
        {
            if (!check_copy_sp(ref.chroma[i].copy_sp[part], opt.chroma[i].copy_sp[part]))
            {
                printf("chroma_copy_sp[%s][%s] failed\n", x265_source_csp_names[i], chromaPartStr[i][part]);
                return false;
            }
        }
        if (opt.chroma[i].copy_ps[part])
        {
            if (!check_copy_ps(ref.chroma[i].copy_ps[part], opt.chroma[i].copy_ps[part]))
            {
                printf("chroma_copy_ps[%s][%s] failed\n", x265_source_csp_names[i], chromaPartStr[i][part]);
                return false;
            }
        }
        if (opt.chroma[i].copy_ss[part])
        {
            if (!check_copy_ss(ref.chroma[i].copy_ss[part], opt.chroma[i].copy_ss[part]))
            {
                printf("chroma_copy_ss[%s][%s] failed\n", x265_source_csp_names[i], chromaPartStr[i][part]);
                return false;
            }
        }
        if (opt.chroma[i].addAvg[part])
        {
            if (!check_addAvg(ref.chroma[i].addAvg[part], opt.chroma[i].addAvg[part]))
            {
                printf("chroma_addAvg[%s][%s] failed\n", x265_source_csp_names[i], chromaPartStr[i][part]);
                return false;
            }
        }
        if (part < NUM_SQUARE_BLOCKS)
        {
            if (opt.chroma[i].sub_ps[part])
            {
                if (!check_pixel_sub_ps(ref.chroma[i].sub_ps[part], opt.chroma[i].sub_ps[part]))
                {
                    printf("chroma_sub_ps[%s][%s] failed\n", x265_source_csp_names[i], chromaPartStr[i][part]);
                    return false;
                }
            }
            if (opt.chroma[i].add_ps[part])
            {
                if (!check_pixel_add_ps(ref.chroma[i].add_ps[part], opt.chroma[i].add_ps[part]))
                {
                    printf("chroma_add_ps[%s][%s] failed\n", x265_source_csp_names[i], chromaPartStr[i][part]);
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
        if (!testPartition(part, ref, opt)) return false;

        if (size > 4)
        {
            part = partitionFromSizes(size, size >> 1); // 2NxN
            if (!testPartition(part, ref, opt)) return false;
            part = partitionFromSizes(size >> 1, size); // Nx2N
            if (!testPartition(part, ref, opt)) return false;
        }
        if (size > 8)
        {
            // 4 AMP modes
            part = partitionFromSizes(size, size >> 2);
            if (!testPartition(part, ref, opt)) return false;
            part = partitionFromSizes(size, 3 * (size >> 2));
            if (!testPartition(part, ref, opt)) return false;

            part = partitionFromSizes(size >> 2, size);
            if (!testPartition(part, ref, opt)) return false;
            part = partitionFromSizes(3 * (size >> 2), size);
            if (!testPartition(part, ref, opt)) return false;
        }
    }

    for (int i = 0; i < NUM_SQUARE_BLOCKS; i++)
    {
        if (opt.calcresidual[i])
        {
            if (!check_calresidual(ref.calcresidual[i], opt.calcresidual[i]))
            {
                printf("calcresidual width: %d failed!\n", 4 << i);
                return false;
            }
        }
        if (opt.calcrecon[i])
        {
            if (!check_calcrecon(ref.calcrecon[i], opt.calcrecon[i]))
            {
                printf("calcRecon width:%d failed!\n", 4 << i);
                return false;
            }
        }
        if (opt.sa8d[i])
        {
            if (!check_pixelcmp(ref.sa8d[i], opt.sa8d[i]))
            {
                printf("sa8d[%dx%d]: failed!\n", 4 << i, 4 << i);
                return false;
            }
        }

        if ((i <= BLOCK_32x32) && opt.ssd_s[i])
        {
            if (!check_ssd_s(ref.ssd_s[i], opt.ssd_s[i]))
            {
                printf("ssd_s[%dx%d]: failed!\n", 4 << i, 4 << i);
                return false;
            }
        }

        if (opt.blockfill_s[i])
        {
            if (!check_blockfill_s(ref.blockfill_s[i], opt.blockfill_s[i]))
            {
                printf("blockfill_s[%dx%d]: failed!\n", 4 << i, 4 << i);
                return false;
            }
        }
        if (opt.transpose[i])
        {
            if (!check_transpose(ref.transpose[i], opt.transpose[i]))
            {
                printf("transpose[%dx%d] failed\n", 4 << i, 4 << i);
                return false;
            }
        }

        if (opt.var[i])
        {
            if (!check_pixel_var(ref.var[i], opt.var[i]))
            {
                printf("var[%dx%d] failed\n", 4 << i, 4 << i);
                return false;
            }
        }

        if ((i < BLOCK_64x64) && opt.cvt16to32_cnt[i])
        {
            if (!check_cvt16to32_cnt_t(ref.cvt16to32_cnt[i], opt.cvt16to32_cnt[i]))
            {
                printf("cvt16to32_cnt[%dx%d] failed!\n", 4 << i, 4 << i);
                return false;
            }
        }

        if ((i < BLOCK_64x64) && opt.cvt16to32_shr[i])
        {
            if (!check_cvt16to32_shr_t(ref.cvt16to32_shr[i], opt.cvt16to32_shr[i]))
            {
                printf("cvt16to32_shr failed!\n");
                return false;
            }
        }

        if ((i < BLOCK_64x64) && opt.cvt32to16_shl[i])
        {
            if (!check_cvt32to16_shl_t(ref.cvt32to16_shl[i], opt.cvt32to16_shl[i]))
            {
                printf("cvt32to16_shl failed!\n");
                return false;
            }
        }
    }

    if (opt.cvt32to16_shr)
    {
        if (!check_cvt32to16_shr_t(ref.cvt32to16_shr, opt.cvt32to16_shr))
        {
            printf("cvt32to16 failed!\n");
            return false;
        }
    }

    if (opt.cvt16to32_shl)
    {
        if (!check_cvt16to32_shl_t(ref.cvt16to32_shl, opt.cvt16to32_shl))
        {
            printf("cvt16to32_shl failed!\n");
            return false;
        }
    }

    if (opt.blockcpy_pp)
    {
        if (!check_blockcopy_pp(ref.blockcpy_pp, opt.blockcpy_pp))
        {
            printf("block copy failed!\n");
            return false;
        }
    }

    if (opt.blockcpy_ps)
    {
        if (!check_blockcopy_ps(ref.blockcpy_ps, opt.blockcpy_ps))
        {
            printf("block copy pixel_short failed!\n");
            return false;
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

    if (opt.pixeladd_ss)
    {
        if (!check_pixeladd_ss(ref.pixeladd_ss, opt.pixeladd_ss))
        {
            printf("pixel add clip failed!\n");
            return false;
        }
    }

    if (opt.frame_init_lowres_core)
    {
        if (!check_downscale_t(ref.frame_init_lowres_core, opt.frame_init_lowres_core))
        {
            printf("downscale failed!\n");
            return false;
        }
    }

    if (opt.scale1D_128to64)
    {
        if (!check_scale_pp(ref.scale1D_128to64, opt.scale1D_128to64))
        {
            printf("scale1D_128to64 failed!\n");
            return false;
        }
    }

    if (opt.scale2D_64to32)
    {
        if (!check_scale_pp(ref.scale2D_64to32, opt.scale2D_64to32))
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

    if (opt.saoCuOrgE0)
    {
        if (!check_saoCuOrgE0_t(ref.saoCuOrgE0, opt.saoCuOrgE0))
        {
            printf("SAO_EO_0 failed\n");
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

    if (opt.planecopy_cp)
    {
        if (!check_planecopy_cp(ref.planecopy_cp, opt.planecopy_cp))
        {
            printf("planecopy_cp failed\n");
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

    if (opt.satd[part])
    {
        HEADER("satd[%s]", lumaPartStr[part]);
        REPORT_SPEEDUP(opt.satd[part], ref.satd[part], pbuf1, STRIDE, fref, STRIDE);
    }

    if (opt.pixelavg_pp[part])
    {
        HEADER("avg_pp[%s]", lumaPartStr[part]);
        REPORT_SPEEDUP(opt.pixelavg_pp[part], ref.pixelavg_pp[part], pbuf1, STRIDE, pbuf2, STRIDE, pbuf3, STRIDE, 32);
    }

    if (opt.sa8d_inter[part])
    {
        HEADER("sa8d[%s]", lumaPartStr[part]);
        REPORT_SPEEDUP(opt.sa8d_inter[part], ref.sa8d_inter[part], pbuf1, STRIDE, fref, STRIDE);
    }

    if (opt.sad[part])
    {
        HEADER("sad[%s]", lumaPartStr[part]);
        REPORT_SPEEDUP(opt.sad[part], ref.sad[part], pbuf1, STRIDE, fref, STRIDE);
    }

    if (opt.sad_x3[part])
    {
        HEADER("sad_x3[%s]", lumaPartStr[part]);
        REPORT_SPEEDUP(opt.sad_x3[part], ref.sad_x3[part], pbuf1, fref, fref + 1, fref - 1, FENC_STRIDE + 5, &cres[0]);
    }

    if (opt.sad_x4[part])
    {
        HEADER("sad_x4[%s]", lumaPartStr[part]);
        REPORT_SPEEDUP(opt.sad_x4[part], ref.sad_x4[part], pbuf1, fref, fref + 1, fref - 1, fref - INCR, FENC_STRIDE + 5, &cres[0]);
    }

    if (opt.sse_pp[part])
    {
        HEADER("sse_pp[%s]", lumaPartStr[part]);
        REPORT_SPEEDUP(opt.sse_pp[part], ref.sse_pp[part], pbuf1, STRIDE, fref, STRIDE);
    }

    if (opt.sse_sp[part])
    {
        HEADER("sse_sp[%s]", lumaPartStr[part]);
        REPORT_SPEEDUP(opt.sse_sp[part], ref.sse_sp[part], (int16_t*)pbuf1, STRIDE, fref, STRIDE);
    }

    if (opt.sse_ss[part])
    {
        HEADER("sse_ss[%s]", lumaPartStr[part]);
        REPORT_SPEEDUP(opt.sse_ss[part], ref.sse_ss[part], (int16_t*)pbuf1, STRIDE, (int16_t*)fref, STRIDE);
    }

    if (opt.luma_copy_pp[part])
    {
        HEADER("luma_copy_pp[%s]", lumaPartStr[part]);
        REPORT_SPEEDUP(opt.luma_copy_pp[part], ref.luma_copy_pp[part], pbuf1, 64, pbuf2, 128);
    }

    if (opt.luma_copy_sp[part])
    {
        HEADER("luma_copy_sp[%s]", lumaPartStr[part]);
        REPORT_SPEEDUP(opt.luma_copy_sp[part], ref.luma_copy_sp[part], pbuf1, 64, sbuf3, 128);
    }

    if (opt.luma_copy_ps[part])
    {
        HEADER("luma_copy_ps[%s]", lumaPartStr[part]);
        REPORT_SPEEDUP(opt.luma_copy_ps[part], ref.luma_copy_ps[part], sbuf1, 64, pbuf1, 128);
    }
    if (opt.luma_copy_ss[part])
    {
        HEADER("luma_copy_ss[%s]", lumaPartStr[part]);
        REPORT_SPEEDUP(opt.luma_copy_ss[part], ref.luma_copy_ss[part], sbuf1, 64, sbuf2, 128);
    }
    if (opt.luma_addAvg[part])
    {
        HEADER("luma_addAvg[%s]", lumaPartStr[part]);
        REPORT_SPEEDUP(opt.luma_addAvg[part], ref.luma_addAvg[part], sbuf1, sbuf2, pbuf1, STRIDE, STRIDE, STRIDE);
    }
    if (part < NUM_SQUARE_BLOCKS)
    {
        if (opt.luma_sub_ps[part])
        {
            HEADER("luma_sub_ps[%s]", lumaPartStr[part]);
            REPORT_SPEEDUP(opt.luma_sub_ps[part], ref.luma_sub_ps[part], (int16_t*)pbuf1, FENC_STRIDE, pbuf2, pbuf1, STRIDE, STRIDE);
        }
        if (opt.luma_add_ps[part])
        {
            HEADER("luma_add_ps[%s]", lumaPartStr[part]);
            REPORT_SPEEDUP(opt.luma_add_ps[part], ref.luma_add_ps[part], pbuf1, FENC_STRIDE, pbuf2, sbuf1, STRIDE, STRIDE);
        }
    }

    for (int i = 0; i < X265_CSP_COUNT; i++)
    {
        if (opt.chroma[i].copy_pp[part])
        {
            HEADER("[%s] copy_pp[%s]", x265_source_csp_names[i], chromaPartStr[i][part]);
            REPORT_SPEEDUP(opt.chroma[i].copy_pp[part], ref.chroma[i].copy_pp[part], pbuf1, 64, pbuf2, 128);
        }
        if (opt.chroma[i].copy_sp[part])
        {
            HEADER("[%s] copy_sp[%s]", x265_source_csp_names[i], chromaPartStr[i][part]);
            REPORT_SPEEDUP(opt.chroma[i].copy_sp[part], ref.chroma[i].copy_sp[part], pbuf1, 64, sbuf3, 128);
        }
        if (opt.chroma[i].copy_ps[part])
        {
            HEADER("[%s] copy_ps[%s]", x265_source_csp_names[i], chromaPartStr[i][part]);
            REPORT_SPEEDUP(opt.chroma[i].copy_ps[part], ref.chroma[i].copy_ps[part], sbuf1, 64, pbuf1, 128);
        }
        if (opt.chroma[i].copy_ss[part])
        {
            HEADER("[%s] copy_ss[%s]", x265_source_csp_names[i], chromaPartStr[i][part]);
            REPORT_SPEEDUP(opt.chroma[i].copy_ss[part], ref.chroma[i].copy_ss[part], sbuf1, 64, sbuf2, 128);
        }
        if (opt.chroma[i].addAvg[part])
        {
            HEADER("[%s]  addAvg[%s]", x265_source_csp_names[i], chromaPartStr[i][part]);
            REPORT_SPEEDUP(opt.chroma[i].addAvg[part], ref.chroma[i].addAvg[part], sbuf1, sbuf2, pbuf1, STRIDE, STRIDE, STRIDE);
        }
        if (part < NUM_SQUARE_BLOCKS)
        {
            if (opt.chroma[i].sub_ps[part])
            {
                HEADER("[%s]  sub_ps[%s]", x265_source_csp_names[i], chromaPartStr[i][part]);
                REPORT_SPEEDUP(opt.chroma[i].sub_ps[part], ref.chroma[i].sub_ps[part], (int16_t*)pbuf1, FENC_STRIDE, pbuf2, pbuf1, STRIDE, STRIDE);
            }
            if (opt.chroma[i].add_ps[part])
            {
                HEADER("[%s]  add_ps[%s]", x265_source_csp_names[i], chromaPartStr[i][part]);
                REPORT_SPEEDUP(opt.chroma[i].add_ps[part], ref.chroma[i].add_ps[part], pbuf1, FENC_STRIDE, pbuf2, sbuf1, STRIDE, STRIDE);
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

    for (int i = 0; i < NUM_SQUARE_BLOCKS; i++)
    {
        if ((i <= BLOCK_32x32) && opt.ssd_s[i])
        {
            HEADER("ssd_s[%dx%d]", 4 << i, 4 << i);
            REPORT_SPEEDUP(opt.ssd_s[i], ref.ssd_s[i], sbuf1, STRIDE);
        }
        if (opt.sa8d[i])
        {
            HEADER("sa8d[%dx%d]", 4 << i, 4 << i);
            REPORT_SPEEDUP(opt.sa8d[i], ref.sa8d[i], pbuf1, STRIDE, pbuf2, STRIDE);
        }
        if (opt.calcresidual[i])
        {
            HEADER("residual[%dx%d]", 4 << i, 4 << i);
            REPORT_SPEEDUP(opt.calcresidual[i], ref.calcresidual[i], pbuf1, pbuf2, sbuf1, 64);
        }

        if (opt.calcrecon[i])
        {
            HEADER("recon[%dx%d]", 4 << i, 4 << i);
            REPORT_SPEEDUP(opt.calcrecon[i], ref.calcrecon[i], pbuf1, sbuf1, sbuf1, pbuf1, 64, 64, 64);
        }

        if (opt.blockfill_s[i])
        {
            HEADER("blkfill[%dx%d]", 4 << i, 4 << i);
            REPORT_SPEEDUP(opt.blockfill_s[i], ref.blockfill_s[i], sbuf1, 64, SHORT_MAX);
        }

        if (opt.transpose[i])
        {
            HEADER("transpose[%dx%d]", 4 << i, 4 << i);
            REPORT_SPEEDUP(opt.transpose[i], ref.transpose[i], pbuf1, pbuf2, STRIDE);
        }

        if (opt.var[i])
        {
            HEADER("var[%dx%d]", 4 << i, 4 << i);
            REPORT_SPEEDUP(opt.var[i], ref.var[i], pbuf1, STRIDE);
        }

        if ((i < BLOCK_64x64) && opt.cvt16to32_shr[i])
        {
            HEADER("cvt16to32_shr[%dx%d]", 4 << i, 4 << i);
            REPORT_SPEEDUP(opt.cvt16to32_shr[i], ref.cvt16to32_shr[i], ibuf1, sbuf2, STRIDE, 3, 4);
        }

        if ((i < BLOCK_64x64) && opt.cvt32to16_shl[i])
        {
            HEADER("cvt32to16_shl[%dx%d]", 4 << i, 4 << i);
            REPORT_SPEEDUP(opt.cvt32to16_shl[i], ref.cvt32to16_shl[i], sbuf2, ibuf1, STRIDE, 3);
        }

        if ((i < BLOCK_64x64) && opt.cvt16to32_cnt[i])
        {
            HEADER("cvt16to32_cnt[%dx%d]", 4 << i, 4 << i);
            REPORT_SPEEDUP(opt.cvt16to32_cnt[i], ref.cvt16to32_cnt[i], sbuf1, sbuf2, STRIDE);
        }
    }

    if (opt.cvt32to16_shr)
    {
        HEADER0("cvt32to16_shr");
        REPORT_SPEEDUP(opt.cvt32to16_shr, ref.cvt32to16_shr, sbuf1, ibuf1, 64, 5, 64);
    }

    if (opt.cvt16to32_shl)
    {
        HEADER0("cvt16to32_shl");
        REPORT_SPEEDUP(opt.cvt16to32_shl, ref.cvt16to32_shl, ibuf1, sbuf1, 64, 5, 64);
    }

    if (opt.blockcpy_pp)
    {
        HEADER0("blockcpy_pp");
        REPORT_SPEEDUP(opt.blockcpy_pp, ref.blockcpy_pp, 64, 64, pbuf1, FENC_STRIDE, pbuf2, STRIDE);
    }

    if (opt.blockcpy_ps)
    {
        HEADER0("blockcpy_ps");
        REPORT_SPEEDUP(opt.blockcpy_ps, ref.blockcpy_ps, 64, 64, pbuf1, FENC_STRIDE, (int16_t*)sbuf3, STRIDE);
    }

    if (opt.weight_pp)
    {
        HEADER0("weight_pp");
        REPORT_SPEEDUP(opt.weight_pp, ref.weight_pp, pbuf1, pbuf2, 64, 64, 32, 32, 128, 1 << 9, 10, 100);
    }

    if (opt.weight_sp)
    {
        HEADER0("weight_sp");
        REPORT_SPEEDUP(opt.weight_sp, ref.weight_sp, (int16_t*)sbuf1, pbuf1, 64, 64, 32, 32, 128, 1 << 9, 10, 100);
    }

    if (opt.pixeladd_ss)
    {
        HEADER0("pixeladd_ss");
        REPORT_SPEEDUP(opt.pixeladd_ss, ref.pixeladd_ss, 64, 64, (int16_t*)pbuf1, FENC_STRIDE, (int16_t*)pbuf2, (int16_t*)pbuf1, STRIDE, STRIDE);
    }

    if (opt.frame_init_lowres_core)
    {
        HEADER0("downscale");
        REPORT_SPEEDUP(opt.frame_init_lowres_core, ref.frame_init_lowres_core, pbuf2, pbuf1, pbuf2, pbuf3, pbuf4, 64, 64, 64, 64);
    }

    if (opt.scale1D_128to64)
    {
        HEADER0("scale1D_128to64");
        REPORT_SPEEDUP(opt.scale1D_128to64, ref.scale1D_128to64, pbuf2, pbuf1, 64);
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

    if (opt.saoCuOrgE0)
    {
        HEADER0("SAO_EO_0");
        REPORT_SPEEDUP(opt.saoCuOrgE0, ref.saoCuOrgE0, pbuf1, psbuf1, 64, 1);
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
}
