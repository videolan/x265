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
#include <malloc.h>

using namespace x265;

// Initialize the Func Names for all the Pixel Comp
static const char *FuncNames[NUM_PARTITIONS] =
{
    "  4x4", "  4x8", " 4x12", " 4x16", " 4x24", " 4x32", " 4x48", " 4x64",
    "  8x4", "  8x8", " 8x12", " 8x16", " 8x24", " 8x32", " 8x48", " 8x64",
    " 12x4", " 12x8", "12x12", "12x16", "12x24", "12x32", "12x48", "12x64",
    " 16x4", " 16x8", "16x12", "16x16", "16x24", "16x32", "16x48", "16x64",
    " 24x4", " 24x8", "24x12", "24x16", "24x24", "24x32", "24x48", "24x64",
    " 32x4", " 32x8", "32x12", "32x16", "32x24", "32x32", "32x48", "32x64",
    " 48x4", " 48x8", "48x12", "48x16", "48x24", "48x32", "48x48", "48x64",
    " 64x4", " 64x8", "64x12", "64x16", "64x24", "64x32", "64x48", "64x64"
};

#if HIGH_BIT_DEPTH
#define BIT_DEPTH 10
#else
#define BIT_DEPTH 8
#endif

#define PIXEL_MAX ((1 << BIT_DEPTH) - 1)

PixelHarness::PixelHarness()
{
    pbuf1 = (pixel*)TestHarness::alignedMalloc(sizeof(pixel), 64 * 64 * 32, 32);
    pbuf2 = (pixel*)TestHarness::alignedMalloc(sizeof(pixel), 64 * 64 * 32, 32);

    sbuf1 = (short*)TestHarness::alignedMalloc(sizeof(short), 64 * 64 * 32, 32);
    sbuf2 = (short*)TestHarness::alignedMalloc(sizeof(short), 64 * 64 * 32, 32);

    if (!pbuf1 || !pbuf2)
    {
        fprintf(stderr, "malloc failed, unable to initiate tests!\n");
        exit(1);
    }

    for (int i = 0; i < 64 * 64 * 32; i++)
    {
        //Generate the Random Buffer for Testing
        pbuf1[i] = rand() & PIXEL_MAX;
        pbuf2[i] = rand() & PIXEL_MAX;

        sbuf1[i] = rand() & PIXEL_MAX;
        sbuf2[i] = rand() & PIXEL_MAX;
    }
}

PixelHarness::~PixelHarness()
{
    TestHarness::alignedFree(pbuf1);
    TestHarness::alignedFree(pbuf2);
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

bool PixelHarness::check_pixel_sp_primitive(pixelcmp_sp ref, pixelcmp_sp opt)
{
    int j = 0;

    for (int i = 0; i <= 100; i++)
    {
        int vres = opt(sbuf1, STRIDE, pbuf2 + j, STRIDE);
        int cres = ref(sbuf1, STRIDE, pbuf2 + j, STRIDE);
        if (vres != cres)
            return false;

        j += INCR;
    }

    return true;
}

bool PixelHarness::check_pixel_ss_primitive(pixelcmp_ss ref, pixelcmp_ss opt)
{
    int j = 0;

    for (int i = 0; i <= 100; i++)
    {
        int vres = opt(sbuf1, STRIDE, sbuf2 + j, STRIDE);
        int cres = ref(sbuf1, STRIDE, sbuf2 + j, STRIDE);
        if (vres != cres)
            return false;

        j += INCR;
    }

    return true;
}

bool PixelHarness::check_pixel_primitive_x3(pixelcmp_x3 ref, pixelcmp_x3 opt)
{
    int j = INCR;

    ALIGN_VAR_16(int, cres[16]);
    ALIGN_VAR_16(int, vres[16]);
    for (int i = 0; i <= 100; i++)
    {
        opt(pbuf1, pbuf2 + j, pbuf2 + j + 1, pbuf2 + j - 1, FENC_STRIDE + 5, &vres[0]);
        ref(pbuf1, pbuf2 + j, pbuf2 + j + 1, pbuf2 + j - 1, FENC_STRIDE + 5, &cres[0]);

        if ((vres[0] != cres[0]) || ((vres[1] != cres[1])) || ((vres[2] != cres[2])))
            return false;

        j += INCR;
    }

    return true;
}

bool PixelHarness::check_pixel_primitive_x4(pixelcmp_x4 ref, pixelcmp_x4 opt)
{
    int j = INCR;

    ALIGN_VAR_16(int, cres[16]);
    ALIGN_VAR_16(int, vres[16]);
    for (int i = 0; i <= 100; i++)
    {
        opt(pbuf1, pbuf2 + j, pbuf2 + j + 1, pbuf2 + j - 1, pbuf2 + j - INCR, FENC_STRIDE + 5, &vres[0]);
        ref(pbuf1, pbuf2 + j, pbuf2 + j + 1, pbuf2 + j - 1, pbuf2 + j - INCR, FENC_STRIDE + 5, &cres[0]);

        if ((vres[0] != cres[0]) || ((vres[1] != cres[1])) || ((vres[2] != cres[2])) || ((vres[3] != cres[3])))
            return false;

        j += INCR;
    }

    return true;
}

bool PixelHarness::check_block_copy(x265::blockcpy_p_p ref, x265::blockcpy_p_p opt)
{
    ALIGN_VAR_16(pixel, ref_dest[64 * 64]);
    ALIGN_VAR_16(pixel, opt_dest[64 * 64]);
    int bx = 64;
    int by = 64;
    int j = 0;
    for (int i = 0; i <= 100; i++)
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

bool PixelHarness::check_block_copy_s_p(x265::blockcpy_s_p ref, x265::blockcpy_s_p opt)
{
    ALIGN_VAR_16(short, ref_dest[64 * 64]);
    ALIGN_VAR_16(short, opt_dest[64 * 64]);
    int bx = 64;
    int by = 64;
    int j = 0;
    for (int i = 0; i <= 100; i++)
    {
        opt(bx, by, opt_dest, 64, pbuf2 + j, 128);
        ref(bx, by, ref_dest, 64, pbuf2 + j, 128);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(short)))
            return false;

        j += 4;
        bx = 4 * ((rand() & 15) + 1);
        by = 4 * ((rand() & 15) + 1);
    }

    return true;
}

bool PixelHarness::check_block_copy_s_c(x265::blockcpy_s_c ref, x265::blockcpy_s_c opt)
{
    ALIGN_VAR_16(short, ref_dest[64 * 64]);
    ALIGN_VAR_16(short, opt_dest[64 * 64]);
    int bx = 64;
    int by = 64;
    int j = 0;
    for (int i = 0; i <= 100; i++)
    {
        opt(bx, by, opt_dest, 64, (uint8_t*)pbuf2 + j, 128);
        ref(bx, by, ref_dest, 64, (uint8_t*)pbuf2 + j, 128);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(short)))
            return false;

        j += 4;
        bx = 4 * ((rand() & 15) + 1);
        by = 4 * ((rand() & 15) + 1);
    }

    return true;
}

bool PixelHarness::check_block_copy_p_s(x265::blockcpy_p_s ref, x265::blockcpy_p_s opt)
{
    ALIGN_VAR_16(pixel, ref_dest[64 * 64]);
    ALIGN_VAR_16(pixel, opt_dest[64 * 64]);
    int bx = 64;
    int by = 64;
    int j = 0;
    for (int i = 0; i <= 100; i++)
    {
        opt(bx, by, opt_dest, 64, (short*)pbuf2 + j, 128);
        ref(bx, by, ref_dest, 64, (short*)pbuf2 + j, 128);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(pixel)))
            return false;

        j += 4;
        bx = 4 * ((rand() & 15) + 1);
        by = 4 * ((rand() & 15) + 1);
    }

    return true;
}

bool PixelHarness::check_calresidual(x265::calcresidual_t ref, x265::calcresidual_t opt)
{
    ALIGN_VAR_16(short, ref_dest[64 * 64]);
    ALIGN_VAR_16(short, opt_dest[64 * 64]);
    memset(ref_dest, 0, 64 * 64 * sizeof(short));
    memset(opt_dest, 0, 64 * 64 * sizeof(short));
    int j = 0;
    for (int i = 0; i <= 100; i++)
    {
        int stride = 64;
        opt(pbuf1 + j, pbuf2 + j, opt_dest, stride);
        ref(pbuf1 + j, pbuf2 + j, ref_dest, stride);

        if (memcmp(ref_dest, opt_dest, 64 * 64 * sizeof(short)))
            return false;

        j += 100;
    }

    return true;
}

bool PixelHarness::check_calcrecon(x265::calcrecon_t ref, x265::calcrecon_t opt)
{
    ALIGN_VAR_16(short, ref_recq[64 * 64]);
    ALIGN_VAR_16(short, opt_recq[64 * 64]);

    ALIGN_VAR_16(pixel, ref_reco[64 * 64]);
    ALIGN_VAR_16(pixel, opt_reco[64 * 64]);

    ALIGN_VAR_16(pixel, ref_pred[64 * 64]);
    ALIGN_VAR_16(pixel, opt_pred[64 * 64]);

    memset(ref_recq, 0, 64 * 64 * sizeof(short));
    memset(opt_recq, 0, 64 * 64 * sizeof(short));
    memset(ref_reco, 0, 64 * 64 * sizeof(pixel));
    memset(opt_reco, 0, 64 * 64 * sizeof(pixel));
    memset(ref_pred, 0, 64 * 64 * sizeof(pixel));
    memset(opt_pred, 0, 64 * 64 * sizeof(pixel));

    int j = 0;
    for (int i = 0; i <= 100; i++)
    {
        int stride = 64;
        opt(pbuf1 + j, sbuf1 + j, opt_reco, opt_recq, opt_pred, stride, stride, stride);
        ref(pbuf1 + j, sbuf1 + j, ref_reco, ref_recq, ref_pred, stride, stride, stride);

        if (memcmp(ref_recq, opt_recq, 64 * 64 * sizeof(short)) || memcmp(ref_reco, opt_reco, 64 * 64 * sizeof(pixel)) || memcmp(ref_pred, opt_pred, 64 * 64 * sizeof(pixel)))
            return false;

        j += 100;
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

        if (opt.sse_pp[curpar])
        {
            if (!check_pixel_primitive(ref.sse_pp[curpar], opt.sse_pp[curpar]))
            {
                printf("sse_pp[%s]: failed!\n", FuncNames[curpar]);
                return false;
            }
        }

        if (opt.sse_sp[curpar])
        {
            if (!check_pixel_sp_primitive(ref.sse_sp[curpar], opt.sse_sp[curpar]))
            {
                printf("sse_sp[%s]: failed!\n", FuncNames[curpar]);
                return false;
            }
        }

        if (opt.sse_ss[curpar])
        {
            if (!check_pixel_ss_primitive(ref.sse_ss[curpar], opt.sse_ss[curpar]))
            {
                printf("sse_ss[%s]: failed!\n", FuncNames[curpar]);
                return false;
            }
        }

        if (opt.sad_x3[curpar])
        {
            if (!check_pixel_primitive_x3(ref.sad_x3[curpar], opt.sad_x3[curpar]))
            {
                printf("sad_x3[%s]: failed!\n", FuncNames[curpar]);
                return false;
            }
        }

        if (opt.sad_x4[curpar])
        {
            if (!check_pixel_primitive_x4(ref.sad_x4[curpar], opt.sad_x4[curpar]))
            {
                printf("sad_x4[%s]: failed!\n", FuncNames[curpar]);
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

    if (opt.sa8d_32x32)
    {
        if (!check_pixel_primitive(ref.sa8d_32x32, opt.sa8d_32x32))
        {
            printf("sa8d_32x32: failed!\n");
            return false;
        }
    }

    if (opt.sa8d_64x64)
    {
        if (!check_pixel_primitive(ref.sa8d_64x64, opt.sa8d_64x64))
        {
            printf("sa8d_64x64: failed!\n");
            return false;
        }
    }

    if (opt.cpyblock)
    {
        if (!check_block_copy(ref.cpyblock, opt.cpyblock))
        {
            printf("block copy failed!\n");
            return false;
        }
    }

    if (opt.cpyblock_p_s)
    {
        if (!check_block_copy_p_s(ref.cpyblock_p_s, opt.cpyblock_p_s))
        {
            printf("block copy pixel_short failed!\n");
            return false;
        }
    }

    if (opt.cpyblock_s_p)
    {
        if (!check_block_copy_s_p(ref.cpyblock_s_p, opt.cpyblock_s_p))
        {
            printf("block copy short_pixel failed!\n");
            return false;
        }
    }

    if (opt.cpyblock_s_c)
    {
        if (!check_block_copy_s_c(ref.cpyblock_s_c, opt.cpyblock_s_c))
        {
            printf("block copy short_char failed!\n");
            return false;
        }
    }

    for (int i = 0; i < NUM_BLOCKS; i++)
    {
        if (opt.calcresidual[i])
        {
            if (!check_calresidual(ref.calcresidual[i], opt.calcresidual[i]))
            {
                printf("getResidue width:%d failed!\n", 4 << i);
                return false;
            }
        }
    }

    for (int i = 0; i < NUM_BLOCKS; i++)
    {
        if (opt.calcrecon[i])
        {
            if (!check_calcrecon(ref.calcrecon[i], opt.calcrecon[i]))
            {
                printf("calcRecon width:%d failed!\n", 4 << i);
                return false;
            }
        }
    }

    return true;
}

void PixelHarness::measureSpeed(const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{
    ALIGN_VAR_16(int, cres[16]);
    pixel *fref = pbuf2 + 2 * INCR;

    for (int curpar = 0; curpar < NUM_PARTITIONS; curpar++)
    {
        if (opt.satd[curpar])
        {
            printf("  satd[%s]", FuncNames[curpar]);
            REPORT_SPEEDUP(opt.satd[curpar], ref.satd[curpar], pbuf1, STRIDE, fref, STRIDE);
        }

        if (opt.sad[curpar])
        {
            printf("   sad[%s]", FuncNames[curpar]);
            REPORT_SPEEDUP(opt.sad[curpar], ref.sad[curpar], pbuf1, STRIDE, fref, STRIDE);
        }

        if (opt.sse_pp[curpar])
        {
            printf("sse_pp[%s]", FuncNames[curpar]);
            REPORT_SPEEDUP(opt.sse_pp[curpar], ref.sse_pp[curpar], pbuf1, STRIDE, fref, STRIDE);
        }

        if (opt.sse_sp[curpar])
        {
            printf("sse_sp[%s]", FuncNames[curpar]);
            REPORT_SPEEDUP(opt.sse_sp[curpar], ref.sse_sp[curpar], (short*)pbuf1, STRIDE, fref, STRIDE);
        }

        if (opt.sse_ss[curpar])
        {
            printf("sse_ss[%s]", FuncNames[curpar]);
            REPORT_SPEEDUP(opt.sse_ss[curpar], ref.sse_ss[curpar], (short*)pbuf1, STRIDE, (short*)fref, STRIDE);
        }

        if (opt.sad_x3[curpar])
        {
            printf("sad_x3[%s]", FuncNames[curpar]);
            REPORT_SPEEDUP(opt.sad_x3[curpar], ref.sad_x3[curpar], pbuf1, fref, fref + 1, fref - 1, FENC_STRIDE + 5, &cres[0]);
        }

        if (opt.sad_x4[curpar])
        {
            printf("sad_x4[%s]", FuncNames[curpar]);
            REPORT_SPEEDUP(opt.sad_x4[curpar], ref.sad_x4[curpar], pbuf1, fref, fref + 1, fref - 1, fref - INCR, FENC_STRIDE + 5, &cres[0]);
        }
    }

    if (opt.sa8d_8x8)
    {
        printf("sa8d_8x8");
        REPORT_SPEEDUP(opt.sa8d_8x8, ref.sa8d_8x8, pbuf1, STRIDE, pbuf2, STRIDE);
    }

    if (opt.sa8d_16x16)
    {
        printf("sa8d_16x16");
        REPORT_SPEEDUP(opt.sa8d_16x16, ref.sa8d_16x16, pbuf1, STRIDE, pbuf2, STRIDE);
    }

    if (opt.sa8d_32x32)
    {
        printf("sa8d_32x32");
        REPORT_SPEEDUP(opt.sa8d_32x32, ref.sa8d_32x32, pbuf1, STRIDE, pbuf2, STRIDE);
    }

    if (opt.sa8d_64x64)
    {
        printf("sa8d_64x64");
        REPORT_SPEEDUP(opt.sa8d_64x64, ref.sa8d_64x64, pbuf1, STRIDE, pbuf2, STRIDE);
    }

    if (opt.cpyblock)
    {
        printf("block cpy");
        REPORT_SPEEDUP(opt.cpyblock, ref.cpyblock, 64, 64, pbuf1, FENC_STRIDE, pbuf2, STRIDE);
    }

    if (opt.cpyblock_p_s)
    {
        printf("p_s   cpy");
        REPORT_SPEEDUP(opt.cpyblock_p_s, ref.cpyblock_p_s, 64, 64, pbuf1, FENC_STRIDE, (short*)pbuf2, STRIDE);
    }

    if (opt.cpyblock_s_p)
    {
        printf("s_p   cpy");
        REPORT_SPEEDUP(opt.cpyblock_s_p, ref.cpyblock_s_p, 64, 64, (short*)pbuf1, FENC_STRIDE, pbuf2, STRIDE);
    }

    if (opt.cpyblock_s_c)
    {
        printf("s_c   cpy");
        REPORT_SPEEDUP(opt.cpyblock_s_c, ref.cpyblock_s_c, 64, 64, (short*)pbuf1, FENC_STRIDE, (uint8_t*)pbuf2, STRIDE);
    }

    for (int i = 0; i < NUM_BLOCKS; i++)
    {
        if (opt.calcresidual[i])
        {
            printf("residual[%dx%d]", 4 << i, 4 << i);
            REPORT_SPEEDUP(opt.calcresidual[i], ref.calcresidual[i], pbuf1, pbuf2, sbuf1, 64);
        }
    }

    for (int i = 0; i < NUM_BLOCKS; i++)
    {
        if (opt.calcrecon[i])
        {
            printf("recon[%dx%d]", 4 << i, 4 << i);
            REPORT_SPEEDUP(opt.calcrecon[i], ref.calcrecon[i], pbuf1, sbuf1, pbuf2, sbuf1, pbuf1, 64, 64, 64);
        }
    }
}
