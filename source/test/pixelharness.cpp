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
#include <string.h>
#include <stdlib.h>

using namespace x265;

#define INCR   32
#define STRIDE 64
#define ITERS  100

PixelHarness::PixelHarness()
{
    int maxheight = 64;
    int padrows = 16;
    int bufsize = STRIDE * (maxheight + padrows) + INCR * ITERS;

    /* 64 pixels wide, 2k deep */
    pbuf1 = (pixel*)X265_MALLOC(pixel, bufsize);
    pbuf2 = (pixel*)X265_MALLOC(pixel, bufsize);
    pbuf3 = (pixel*)X265_MALLOC(pixel, bufsize);
    pbuf4 = (pixel*)X265_MALLOC(pixel, bufsize);

    ibuf1 = (int*)X265_MALLOC(int, bufsize);

    sbuf1 = (int16_t*)X265_MALLOC(int16_t, bufsize);
    sbuf2 = (int16_t*)X265_MALLOC(int16_t, bufsize);

    if (!pbuf1 || !pbuf2 || !pbuf3 || !pbuf4 || !sbuf1 || !sbuf2 || !ibuf1)
    {
        fprintf(stderr, "malloc failed, unable to initiate tests!\n");
        exit(1);
    }

    for (int i = 0; i < bufsize; i++)
    {
        pbuf1[i] = rand() & PIXEL_MAX;
        pbuf2[i] = rand() & PIXEL_MAX;
        pbuf3[i] = rand() & PIXEL_MAX;
        pbuf4[i] = rand() & PIXEL_MAX;

        sbuf1[i] = (rand() & (2 * SHORT_MAX + 1)) - SHORT_MAX - 1; //max(SHORT_MIN, min(rand(), SHORT_MAX));
        sbuf2[i] = (rand() & (2 * SHORT_MAX + 1)) - SHORT_MAX - 1; //max(SHORT_MIN, min(rand(), SHORT_MAX));

        ibuf1[i] = (rand() & (2 * SHORT_MAX + 1)) - SHORT_MAX - 1;
    }
}

PixelHarness::~PixelHarness()
{
    X265_FREE(pbuf1);
    X265_FREE(pbuf2);
    X265_FREE(pbuf3);
    X265_FREE(pbuf4);
    X265_FREE(sbuf1);
    X265_FREE(sbuf2);
}

bool PixelHarness::check_pixelcmp(pixelcmp_t ref, pixelcmp_t opt)
{
    int j = 0;

    for (int i = 0; i < ITERS; i++)
    {
        int vres = opt(pbuf1, STRIDE, pbuf2 + j, STRIDE);
        int cres = ref(pbuf1, STRIDE, pbuf2 + j, STRIDE);
        if (vres != cres)
            return false;

        j += INCR;
    }

    return true;
}

bool PixelHarness::check_pixelcmp_sp(pixelcmp_sp_t ref, pixelcmp_sp_t opt)
{
    int j = 0;

    for (int i = 0; i < ITERS; i++)
    {
        int vres = opt(sbuf1, STRIDE, pbuf2 + j, STRIDE);
        int cres = ref(sbuf1, STRIDE, pbuf2 + j, STRIDE);
        if (vres != cres)
            return false;

        j += INCR;
    }

    return true;
}

bool PixelHarness::check_pixelcmp_ss(pixelcmp_ss_t ref, pixelcmp_ss_t opt)
{
    int j = 0;

    for (int i = 0; i < ITERS; i++)
    {
        int vres = opt(sbuf1, STRIDE, sbuf2 + j, STRIDE);
        int cres = ref(sbuf1, STRIDE, sbuf2 + j, STRIDE);
        if (vres != cres)
            return false;

        j += INCR;
    }

    return true;
}

bool PixelHarness::check_pixelcmp_x3(pixelcmp_x3_t ref, pixelcmp_x3_t opt)
{
    ALIGN_VAR_16(int, cres[16]);
    ALIGN_VAR_16(int, vres[16]);
    int j = 0;
    for (int i = 0; i < ITERS; i++)
    {
        opt(pbuf1, pbuf2 + j, pbuf2 + j + 1, pbuf2 + j + 2, FENC_STRIDE - 5, &vres[0]);
        ref(pbuf1, pbuf2 + j, pbuf2 + j + 1, pbuf2 + j + 2, FENC_STRIDE - 5, &cres[0]);

        if ((vres[0] != cres[0]) || ((vres[1] != cres[1])) || ((vres[2] != cres[2])))
            return false;

        j += INCR;
    }

    return true;
}

bool PixelHarness::check_pixelcmp_x4(pixelcmp_x4_t ref, pixelcmp_x4_t opt)
{
    ALIGN_VAR_16(int, cres[16]);
    ALIGN_VAR_16(int, vres[16]);
    int j = 0;
    for (int i = 0; i < ITERS; i++)
    {
        opt(pbuf1, pbuf2 + j, pbuf2 + j + 1, pbuf2 + j + 2, pbuf2 + j + 3, FENC_STRIDE - 5, &vres[0]);
        ref(pbuf1, pbuf2 + j, pbuf2 + j + 1, pbuf2 + j + 2, pbuf2 + j + 3, FENC_STRIDE - 5, &cres[0]);

        if ((vres[0] != cres[0]) || ((vres[1] != cres[1])) || ((vres[2] != cres[2])) || ((vres[3] != cres[3])))
            return false;

        j += INCR;
    }

    return true;
}

bool PixelHarness::check_block_copy(blockcpy_pp_t ref, blockcpy_pp_t opt)
{
    ALIGN_VAR_16(pixel, ref_dest[64 * 64]);
    ALIGN_VAR_16(pixel, opt_dest[64 * 64]);
    int bx = 64;
    int by = 64;
    int j = 0;
    for (int i = 0; i < ITERS; i++)
    {
        opt(bx, by, opt_dest, 64, pbuf2 + j, 128);
        ref(bx, by, ref_dest, 64, pbuf2 + j, 128);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(pixel)))
            return false;

        j += 4;
        bx = 4 * ((rand() & 15) + 1);
        by = 4 * ((rand() & 15) + 1);
    }

    return true;
}

bool PixelHarness::check_block_copy_s_p(blockcpy_sp_t ref, blockcpy_sp_t opt)
{
    ALIGN_VAR_16(int16_t, ref_dest[64 * 64]);
    ALIGN_VAR_16(int16_t, opt_dest[64 * 64]);
    int bx = 64;
    int by = 64;
    int j = 0;
    for (int i = 0; i < ITERS; i++)
    {
        opt(bx, by, opt_dest, 64, pbuf2 + j, 128);
        ref(bx, by, ref_dest, 64, pbuf2 + j, 128);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(int16_t)))
            return false;

        j += 4;
        bx = 4 * ((rand() & 15) + 1);
        by = 4 * ((rand() & 15) + 1);
    }

    return true;
}

bool PixelHarness::check_block_copy_s_c(blockcpy_sc_t ref, blockcpy_sc_t opt)
{
    ALIGN_VAR_16(int16_t, ref_dest[64 * 64]);
    ALIGN_VAR_16(int16_t, opt_dest[64 * 64]);
    int bx = 64;
    int by = 64;
    int j = 0;
    for (int i = 0; i < ITERS; i++)
    {
        opt(bx, by, opt_dest, 64, (uint8_t*)pbuf2 + j, STRIDE);
        ref(bx, by, ref_dest, 64, (uint8_t*)pbuf2 + j, STRIDE);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(int16_t)))
            return false;

        j += 4;
        bx = 4 * ((rand() & 15) + 1);
        by = 4 * ((rand() & 15) + 1);
    }

    return true;
}

bool PixelHarness::check_block_copy_p_s(blockcpy_ps_t ref, blockcpy_ps_t opt)
{
    ALIGN_VAR_16(pixel, ref_dest[64 * 64]);
    ALIGN_VAR_16(pixel, opt_dest[64 * 64]);
    int bx = 64;
    int by = 64;
    int j = 0;
    for (int i = 0; i < ITERS; i++)
    {
        opt(bx, by, opt_dest, 64, (int16_t*)pbuf2 + j, STRIDE);
        ref(bx, by, ref_dest, 64, (int16_t*)pbuf2 + j, STRIDE);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(pixel)))
            return false;

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
    for (int i = 0; i < ITERS; i++)
    {
        opt(pbuf1 + j, pbuf2 + j, opt_dest, STRIDE);
        ref(pbuf1 + j, pbuf2 + j, ref_dest, STRIDE);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(int16_t)))
            return false;

        j += INCR;
    }

    return true;
}

bool PixelHarness::check_calcrecon(calcrecon_t ref, calcrecon_t opt)
{
    ALIGN_VAR_16(int16_t, ref_recq[64 * 64]);
    ALIGN_VAR_16(int16_t, opt_recq[64 * 64]);

    ALIGN_VAR_16(pixel, ref_reco[64 * 64]);
    ALIGN_VAR_16(pixel, opt_reco[64 * 64]);

    ALIGN_VAR_16(pixel, ref_pred[64 * 64]);
    ALIGN_VAR_16(pixel, opt_pred[64 * 64]);

    memset(ref_recq, 0, 64 * 64 * sizeof(int16_t));
    memset(opt_recq, 0, 64 * 64 * sizeof(int16_t));
    memset(ref_reco, 0, 64 * 64 * sizeof(pixel));
    memset(opt_reco, 0, 64 * 64 * sizeof(pixel));
    memset(ref_pred, 0, 64 * 64 * sizeof(pixel));
    memset(opt_pred, 0, 64 * 64 * sizeof(pixel));

    int j = 0;
    for (int i = 0; i < ITERS; i++)
    {
        int stride = STRIDE;
        opt(pbuf1 + j, sbuf1 + j, opt_reco, opt_recq, opt_pred, stride, stride, stride);
        ref(pbuf1 + j, sbuf1 + j, ref_reco, ref_recq, ref_pred, stride, stride, stride);

        if (memcmp(ref_recq, opt_recq, 64 * 64 * sizeof(int16_t)))
            return false;
        if (memcmp(ref_reco, opt_reco, 64 * 64 * sizeof(pixel)))
            return false;
        if (memcmp(ref_pred, opt_pred, 64 * 64 * sizeof(pixel)))
            return false;

        j += INCR;
    }

    return true;
}

bool PixelHarness::check_weightpUni(weightpUni_t ref, weightpUni_t opt)
{
    ALIGN_VAR_16(pixel, ref_dest[64 * 64]);
    ALIGN_VAR_16(pixel, opt_dest[64 * 64]);

    memset(ref_dest, 0, 64 * 64 * sizeof(pixel));
    memset(opt_dest, 0, 64 * 64 * sizeof(pixel));
    int j = 0;
    int width = (2 * rand()) % 64;
    int height = 8;
    int w0 = rand() % 256;
    int shift = rand() % 12;
    int round = shift ? (1 << (shift - 1)) : 0;
    int offset = (rand() % 256) - 128;
    for (int i = 0; i < ITERS; i++)
    {
        opt((int16_t*)sbuf1 + j, opt_dest, 64, 64, width, height, w0, round, shift, offset);
        ref((int16_t*)sbuf1 + j, ref_dest, 64, 64, width, height, w0, round, shift, offset);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(pixel)))
            return false;

        j += INCR;
    }

    return true;
}

bool PixelHarness::check_weightpUni(weightpUniPixel_t ref, weightpUniPixel_t opt)
{
    ALIGN_VAR_16(pixel, ref_dest[64 * 64]);
    ALIGN_VAR_16(pixel, opt_dest[64 * 64]);

    memset(ref_dest, 0, 64 * 64 * sizeof(pixel));
    memset(opt_dest, 0, 64 * 64 * sizeof(pixel));
    int j = 0;
    int width = (2 * rand()) % 64;
    int height = 8;
    int w0 = rand() % 256;
    int shift = rand() % 12;
    int round = shift ? (1 << (shift - 1)) : 0;
    int offset = (rand() % 256) - 128;
    for (int i = 0; i < ITERS; i++)
    {
        opt(pbuf1 + j, opt_dest, 64, 64, width, height, w0, round, shift, offset);
        ref(pbuf1 + j, ref_dest, 64, 64, width, height, w0, round, shift, offset);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(pixel)))
            return false;

        j += INCR;
    }

    return true;
}

bool PixelHarness::check_pixelsub_sp(pixelsub_sp_t ref, pixelsub_sp_t opt)
{
    ALIGN_VAR_16(int16_t, ref_dest[64 * 64]);
    ALIGN_VAR_16(int16_t, opt_dest[64 * 64]);
    int bx = 64;
    int by = 64;
    int j = 0;
    for (int i = 0; i < ITERS; i++)
    {
        opt(bx, by, opt_dest, 64, pbuf2 + j, pbuf1 + j, STRIDE, STRIDE);
        ref(bx, by, ref_dest, 64, pbuf2 + j, pbuf1 + j, STRIDE, STRIDE);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(int16_t)))
            return false;

        j += INCR;
        bx = 4 * ((rand() & 15) + 1);
        by = 4 * ((rand() & 15) + 1);
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
    for (int i = 0; i < ITERS; i++)
    {
        opt(bx, by, opt_dest, STRIDE, (int16_t*)pbuf2 + j, (int16_t*)pbuf1 + j, STRIDE, STRIDE);
        ref(bx, by, ref_dest, STRIDE, (int16_t*)pbuf2 + j, (int16_t*)pbuf1 + j, STRIDE, STRIDE);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(int16_t)))
            return false;

        j += INCR;
        bx = 4 * ((rand() & 15) + 1);
        by = 4 * ((rand() & 15) + 1);
    }

    return true;
}

bool PixelHarness::check_pixeladd_pp(pixeladd_pp_t ref, pixeladd_pp_t opt)
{
    ALIGN_VAR_16(pixel, ref_dest[64 * 64]);
    ALIGN_VAR_16(pixel, opt_dest[64 * 64]);
    int bx = 64;
    int by = 64;
    int j = 0;
    for (int i = 0; i < ITERS; i++)
    {
        opt(bx, by, opt_dest, STRIDE, pbuf2 + j, pbuf1 + j, STRIDE, STRIDE);
        ref(bx, by, ref_dest, STRIDE, pbuf2 + j, pbuf1 + j, STRIDE, STRIDE);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(pixel)))
            return false;

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
        ref(pbuf2 + j, ref_destf, ref_desth, ref_destv, ref_destc, src_stride, dst_stride, bx, by);
        opt(pbuf2 + j, opt_destf, opt_desth, opt_destv, opt_destc, src_stride, dst_stride, bx, by);

        if (memcmp(ref_destf, opt_destf, 32 * 32 * sizeof(pixel)))
            return false;
        if (memcmp(ref_desth, opt_desth, 32 * 32 * sizeof(pixel)))
            return false;
        if (memcmp(ref_destv, opt_destv, 32 * 32 * sizeof(pixel)))
            return false;
        if (memcmp(ref_destc, opt_destc, 32 * 32 * sizeof(pixel)))
            return false;

        j += INCR;
    }

    return true;
}

bool PixelHarness::check_cvt32to16_shr_t(cvt32to16_shr_t ref, cvt32to16_shr_t opt)
{
    ALIGN_VAR_16(int16_t, ref_dest[64 * 64]);
    ALIGN_VAR_16(int16_t, opt_dest[64 * 64]);

    int j = 0;
    for (int i = 0; i < ITERS; i++)
    {
        int shift = (rand() % 7 + 1);

        opt(opt_dest, ibuf1 + j, STRIDE, shift, STRIDE);
        ref(ref_dest, ibuf1 + j, STRIDE, shift, STRIDE);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(int16_t)))
            return false;

        j += INCR;
    }

    return true;
}

bool PixelHarness::check_pixelavg_pp(pixelavg_pp_t ref, pixelavg_pp_t opt)
{
    ALIGN_VAR_16(pixel, ref_dest[64 * 64]);
    ALIGN_VAR_16(pixel, opt_dest[64 * 64]);

    int j = 0;

    for (int i = 0; i < ITERS; i++)
    {
        opt(opt_dest, STRIDE, pbuf1 + j, STRIDE, pbuf2 + j, STRIDE, 32);
        ref(ref_dest, STRIDE, pbuf1 + j, STRIDE, pbuf2 + j, STRIDE, 32);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(pixel)))
            return false;

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

    return true;
}

bool PixelHarness::testCorrectness(const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{
    for (int size = 4; size <= 64; size *= 2)
    {
        int part = PartitionFromSizes(size, size); // 2Nx2N
        if (!testPartition(part, ref, opt)) return false;

        if (size > 4)
        {
            part = PartitionFromSizes(size, size >> 1); // 2NxN
            if (!testPartition(part, ref, opt)) return false;
            part = PartitionFromSizes(size >> 1, size); // Nx2N
            if (!testPartition(part, ref, opt)) return false;
        }
        if (size > 8)
        {
            // 4 AMP modes
            part = PartitionFromSizes(size, size >> 2);
            if (!testPartition(part, ref, opt)) return false;
            part = PartitionFromSizes(size, 3 * (size >> 2));
            if (!testPartition(part, ref, opt)) return false;

            part = PartitionFromSizes(size >> 2, size);
            if (!testPartition(part, ref, opt)) return false;
            part = PartitionFromSizes(3 * (size >> 2), size);
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
    }

    if (opt.cvt32to16_shr)
    {
        if (!check_cvt32to16_shr_t(ref.cvt32to16_shr, opt.cvt32to16_shr))
        {
            printf("cvt32to16 failed!\n");
            return false;
        }
    }

    if (opt.blockcpy_pp)
    {
        if (!check_block_copy(ref.blockcpy_pp, opt.blockcpy_pp))
        {
            printf("block copy failed!\n");
            return false;
        }
    }

    if (opt.blockcpy_ps)
    {
        if (!check_block_copy_p_s(ref.blockcpy_ps, opt.blockcpy_ps))
        {
            printf("block copy pixel_short failed!\n");
            return false;
        }
    }

    if (opt.blockcpy_sp)
    {
        if (!check_block_copy_s_p(ref.blockcpy_sp, opt.blockcpy_sp))
        {
            printf("block copy short_pixel failed!\n");
            return false;
        }
    }

    if (opt.blockcpy_sc)
    {
        if (!check_block_copy_s_c(ref.blockcpy_sc, opt.blockcpy_sc))
        {
            printf("block copy short_char failed!\n");
            return false;
        }
    }

    if (opt.weightpUniPixel)
    {
        if (!check_weightpUni(ref.weightpUniPixel, opt.weightpUniPixel))
        {
            printf("Weighted Prediction for Unidir (Pixel) failed!\n");
            return false;
        }
    }

    if (opt.weightpUni)
    {
        if (!check_weightpUni(ref.weightpUni, opt.weightpUni))
        {
            printf("Weighted Prediction for Unidir (int16_t) failed!\n");
            return false;
        }
    }

    if (opt.pixelsub_sp)
    {
        if (!check_pixelsub_sp(ref.pixelsub_sp, opt.pixelsub_sp))
        {
            printf("Luma Substract failed!\n");
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

    if (opt.pixeladd_pp)
    {
        if (!check_pixeladd_pp(ref.pixeladd_pp, opt.pixeladd_pp))
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
    return true;
}

void PixelHarness::measurePartition(int part, const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{
    ALIGN_VAR_16(int, cres[16]);
    pixel *fref = pbuf2 + 2 * INCR;

    if (opt.satd[part])
    {
        printf("  satd[%s]", lumaPartStr[part]);
        REPORT_SPEEDUP(opt.satd[part], ref.satd[part], pbuf1, STRIDE, fref, STRIDE);
    }

    if (opt.pixelavg_pp[part])
    {
        printf("avg_pp[%s]", lumaPartStr[part]);
        REPORT_SPEEDUP(opt.pixelavg_pp[part], ref.pixelavg_pp[part], pbuf1, STRIDE, pbuf2, STRIDE, pbuf3, STRIDE, 0);
    }

    if (opt.sa8d_inter[part])
    {
        printf("  sa8d[%s]", lumaPartStr[part]);
        REPORT_SPEEDUP(opt.sa8d_inter[part], ref.sa8d_inter[part], pbuf1, STRIDE, fref, STRIDE);
    }

    if (opt.sad[part])
    {
        printf("   sad[%s]", lumaPartStr[part]);
        REPORT_SPEEDUP(opt.sad[part], ref.sad[part], pbuf1, STRIDE, fref, STRIDE);
    }

    if (opt.sad_x3[part])
    {
        printf("sad_x3[%s]", lumaPartStr[part]);
        REPORT_SPEEDUP(opt.sad_x3[part], ref.sad_x3[part], pbuf1, fref, fref + 1, fref - 1, FENC_STRIDE + 5, &cres[0]);
    }

    if (opt.sad_x4[part])
    {
        printf("sad_x4[%s]", lumaPartStr[part]);
        REPORT_SPEEDUP(opt.sad_x4[part], ref.sad_x4[part], pbuf1, fref, fref + 1, fref - 1, fref - INCR, FENC_STRIDE + 5, &cres[0]);
    }

    if (opt.sse_pp[part])
    {
        printf("sse_pp[%s]", lumaPartStr[part]);
        REPORT_SPEEDUP(opt.sse_pp[part], ref.sse_pp[part], pbuf1, STRIDE, fref, STRIDE);
    }

    if (opt.sse_sp[part])
    {
        printf("sse_sp[%s]", lumaPartStr[part]);
        REPORT_SPEEDUP(opt.sse_sp[part], ref.sse_sp[part], (int16_t*)pbuf1, STRIDE, fref, STRIDE);
    }

    if (opt.sse_ss[part])
    {
        printf("sse_ss[%s]", lumaPartStr[part]);
        REPORT_SPEEDUP(opt.sse_ss[part], ref.sse_ss[part], (int16_t*)pbuf1, STRIDE, (int16_t*)fref, STRIDE);
    }
}

void PixelHarness::measureSpeed(const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{
    for (int size = 4; size <= 64; size *= 2)
    {
        int part = PartitionFromSizes(size, size); // 2Nx2N
        measurePartition(part, ref, opt);

        if (size > 4)
        {
            part = PartitionFromSizes(size, size >> 1); // 2NxN
            measurePartition(part, ref, opt);
            part = PartitionFromSizes(size >> 1, size); // Nx2N
            measurePartition(part, ref, opt);
        }
        if (size > 8)
        {
            // 4 AMP modes
            part = PartitionFromSizes(size, size >> 2);
            measurePartition(part, ref, opt);
            part = PartitionFromSizes(size, 3 * (size >> 2));
            measurePartition(part, ref, opt);

            part = PartitionFromSizes(size >> 2, size);
            measurePartition(part, ref, opt);
            part = PartitionFromSizes(3 * (size >> 2), size);
            measurePartition(part, ref, opt);
        }
    }

    for (int i = 0; i < NUM_SQUARE_BLOCKS; i++)
    {
        if (opt.sa8d[i])
        {
            printf("sa8d[%dx%d]", 4 << i, 4 << i);
            REPORT_SPEEDUP(opt.sa8d[i], ref.sa8d[i], pbuf1, STRIDE, pbuf2, STRIDE);
        }
        if (opt.calcresidual[i])
        {
            printf("residual[%dx%d]", 4 << i, 4 << i);
            REPORT_SPEEDUP(opt.calcresidual[i], ref.calcresidual[i], pbuf1, pbuf2, sbuf1, 64);
        }

        if (opt.calcrecon[i])
        {
            printf("recon[%dx%d]", 4 << i, 4 << i);
            REPORT_SPEEDUP(opt.calcrecon[i], ref.calcrecon[i], pbuf1, sbuf1, pbuf2, sbuf1, pbuf1, 64, 64, 64);
        }
    }

    if (opt.cvt32to16_shr)
    {
        printf("cvt32to16_shr");
        REPORT_SPEEDUP(opt.cvt32to16_shr, ref.cvt32to16_shr, sbuf1, ibuf1, 64, 5, 64);
    }

    if (opt.blockcpy_pp)
    {
        printf("block cpy");
        REPORT_SPEEDUP(opt.blockcpy_pp, ref.blockcpy_pp, 64, 64, pbuf1, FENC_STRIDE, pbuf2, STRIDE);
    }

    if (opt.blockcpy_ps)
    {
        printf("p_s   cpy");
        REPORT_SPEEDUP(opt.blockcpy_ps, ref.blockcpy_ps, 64, 64, pbuf1, FENC_STRIDE, (int16_t*)pbuf2, STRIDE);
    }

    if (opt.blockcpy_sp)
    {
        printf("s_p   cpy");
        REPORT_SPEEDUP(opt.blockcpy_sp, ref.blockcpy_sp, 64, 64, (int16_t*)pbuf1, FENC_STRIDE, pbuf2, STRIDE);
    }

    if (opt.blockcpy_sc)
    {
        printf("s_c   cpy");
        REPORT_SPEEDUP(opt.blockcpy_sc, ref.blockcpy_sc, 64, 64, (int16_t*)pbuf1, FENC_STRIDE, (uint8_t*)pbuf2, STRIDE);
    }

    if (opt.weightpUniPixel)
    {
        printf("Weightp 8bpp");
        REPORT_SPEEDUP(opt.weightpUniPixel, ref.weightpUniPixel, pbuf1, pbuf2, 64, 64, 32, 32, 128, 1 << 9, 10, 100);
    }

    if (opt.weightpUni)
    {
        printf("Weightp16bpp");
        REPORT_SPEEDUP(opt.weightpUni, ref.weightpUni, (int16_t*)sbuf1, pbuf1, 64, 64, 32, 32, 128, 1 << 9, 10, 100);
    }

    if (opt.pixelsub_sp)
    {
        printf("Pixel Sub");
        REPORT_SPEEDUP(opt.pixelsub_sp, ref.pixelsub_sp, 64, 64, (int16_t*)pbuf1, FENC_STRIDE, pbuf2, pbuf1, STRIDE, STRIDE);
    }

    if (opt.pixeladd_ss)
    {
        printf("pixel_ss add");
        REPORT_SPEEDUP(opt.pixeladd_ss, ref.pixeladd_ss, 64, 64, (int16_t*)pbuf1, FENC_STRIDE, (int16_t*)pbuf2, (int16_t*)pbuf1, STRIDE, STRIDE);
    }

    if (opt.pixeladd_pp)
    {
        printf("pixel_pp add");
        REPORT_SPEEDUP(opt.pixeladd_pp, ref.pixeladd_pp, 64, 64, pbuf1, FENC_STRIDE, pbuf2, pbuf1, STRIDE, STRIDE);
    }

    if (opt.frame_init_lowres_core)
    {
        printf("downscale");
        REPORT_SPEEDUP(opt.frame_init_lowres_core, ref.frame_init_lowres_core, pbuf2, pbuf1, pbuf2, pbuf3, pbuf4, 64, 64, 64, 64);
    }
}
