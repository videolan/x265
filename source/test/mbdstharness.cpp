/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Min Chen <min.chen@multicorewareinc.com>
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

#include "mbdstharness.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>

using namespace x265;

struct DctConf_t {
    const char *name;
    int width;
};
const DctConf_t DctConf_infos[] =
{
   { "Dst4x4\t",    4},
   { "Dct4x4\t",    4},
   { "Dct8x8\t",    8},
   { "Dct16x16",   16},
   { "Dct32x32",   32},
};

const DctConf_t IDctConf_infos[] =
{
   {"IDst4x4\t",    4},
   {"IDct4x4\t",    4},
   {"IDct8x8\t",    8},
   {"IDct16x16",   16},
   {"IDct32x32",   32},
};

MBDstHarness::MBDstHarness()
{
    mbuf1 = (short*)TestHarness::alignedMalloc(sizeof(short), mb_t_size, 32);
    mbufdct = (short*)TestHarness::alignedMalloc(sizeof(short), mb_t_size, 32);

    mbuf2 = (short*)TestHarness::alignedMalloc(sizeof(short), mem_cmp_size, 32);
    mbuf3 = (short*)TestHarness::alignedMalloc(sizeof(short), mem_cmp_size, 32);
    mbuf4 = (short*)TestHarness::alignedMalloc(sizeof(short), mem_cmp_size, 32);

    mintbuf1 = (int*)TestHarness::alignedMalloc(sizeof(int), mb_t_size, 32);
    mintbuf2 = (int*)TestHarness::alignedMalloc(sizeof(int), mb_t_size, 32);
    mintbuf3 = (int*)TestHarness::alignedMalloc(sizeof(int), mem_cmp_size, 32);
    mintbuf4 = (int*)TestHarness::alignedMalloc(sizeof(int), mem_cmp_size, 32);

    if (!mbuf1 || !mbuf2 || !mbuf3 || !mbuf4 || !mbufdct)
    {
        fprintf(stderr, "malloc failed, unable to initiate tests!\n");
        exit(1);
    }

    if (!mintbuf1 || !mintbuf2 || !mintbuf3 || !mintbuf4)
    {
        fprintf(stderr, "malloc failed, unable to initiate tests!\n");
        exit(1);
    }

    for (int i = 0; i < mb_t_size; i++)
    {
        mbuf1[i] = rand() & PIXEL_MAX;
        mbufdct[i] = (rand() & PIXEL_MAX) - (rand() & PIXEL_MAX);
    }

    for (int i = 0; i < mb_t_size; i++)
    {
        mintbuf1[i] = rand() & PIXEL_MAX;
        mintbuf2[i] = rand() & PIXEL_MAX;
    }

#if _DEBUG
    memset(mbuf2, 0, mem_cmp_size);
    memset(mbuf3, 0, mem_cmp_size);
    memset(mbuf4, 0, mem_cmp_size);

    memset(mintbuf3, 0, mem_cmp_size);
    memset(mintbuf4, 0, mem_cmp_size);
#endif
}

MBDstHarness::~MBDstHarness()
{
    TestHarness::alignedFree(mbuf1);
    TestHarness::alignedFree(mbuf2);
    TestHarness::alignedFree(mbuf3);
    TestHarness::alignedFree(mbuf4);
    TestHarness::alignedFree(mbufdct);

    TestHarness::alignedFree(mintbuf1);
    TestHarness::alignedFree(mintbuf2);
    TestHarness::alignedFree(mintbuf3);
    TestHarness::alignedFree(mintbuf4);
}

bool MBDstHarness::check_dct_primitive(dct_t ref, dct_t opt, int width)
{
    int j = 0;
    int cmp_size = sizeof(int) * width * width;

    for (int i = 0; i <= 100; i++)
    {
        ref(mbufdct + j, mintbuf1, width);
        opt(mbufdct + j, mintbuf2, width);

        if (memcmp(mintbuf1, mintbuf2, cmp_size))
        {
#if _DEBUG
            // redo for debug
            ref(mbufdct + j, mintbuf1, width);
            opt(mbufdct + j, mintbuf2, width);
#endif
            return false;
        }

        j += 16;
#if _DEBUG
        memset(mbuf2, 0xCD, mem_cmp_size);
        memset(mbuf3, 0xCD, mem_cmp_size);
#endif
    }

    return true;
}

bool MBDstHarness::check_idct_primitive(idct_t ref, idct_t opt, int width)
{
    int j = 0;
    int cmp_size = sizeof(short) * width * width;

    for (int i = 0; i <= 100; i++)
    {
        if (width >= 16)
        {
            // IDCT16x16 and IDCT32x32 may broken input buffer, so copy one
            ALIGN_VAR_32(short, tmp[32*32]);
            memcpy(tmp, mbufdct + j, sizeof(short)* 32 *32);
            ref(mbufdct + j, mbuf2, width);
            opt(tmp, mbuf3, width);
        }
        else
        {
            ref(mbufdct + j, mbuf2, width);
            opt(mbufdct + j, mbuf3, width);
        }

        if (memcmp(mbuf2, mbuf3, cmp_size))
        {
#if _DEBUG
            // redo for debug
            ref(mbufdct + j, mbuf2, width);
            opt(mbufdct + j, mbuf3, width);
#endif
            return false;
        }

        j += 16;
#if _DEBUG
        memset(mbuf2, 0xCD, mem_cmp_size);
        memset(mbuf3, 0xCD, mem_cmp_size);
#endif
    }

    return true;
}

bool MBDstHarness::check_xdequant_primitive(quant ref, quant opt)
{
    int j = 0;

    for (int i = 0; i <= 5; i++)
    {
        int iWidth = (rand() % 4 + 1) * 4;

        if (iWidth == 12)
        {
            iWidth = 32;
        }
        int iHeight = iWidth;

        int tmp = rand() % 58;
        int iPer = tmp / 6;
        int iRem = tmp % 6;

        bool useScalingList = (tmp % 2 == 0) ? false : true;

        unsigned int uiLog2TrSize = (rand() % 4) + 2;

        opt(8, mintbuf1 + j, mintbuf3, iWidth, iHeight, iPer, iRem, useScalingList, uiLog2TrSize, mintbuf2 + j);  // g_bitDepthY  = 8, g_bitDepthC = 8
        ref(8, mintbuf1 + j, mintbuf4, iWidth, iHeight, iPer, iRem, useScalingList, uiLog2TrSize, mintbuf2 + j);

        if (memcmp(mintbuf3, mintbuf4, mem_cmp_size))
            return false;

        j += 16;
#if _DEBUG
        memset(mintbuf3, 0, mem_cmp_size);
        memset(mintbuf4, 0, mem_cmp_size);
#endif
    }

    return true;
}

bool MBDstHarness::testCorrectness(const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{
    for( int i=0; i<NUM_DCTS; i++ )
    {
        if (opt.dct[i])
        {
            if (!check_dct_primitive(ref.dct[i], opt.dct[i], DctConf_infos[i].width))
            {
                printf("\n%s failed\n", DctConf_infos[i].name);
                return false;
            }
        }
    }

    for( int i=0; i<NUM_IDCTS; i++ )
    {
        if (opt.idct[i])
        {
            if (!check_idct_primitive(ref.idct[i], opt.idct[i], IDctConf_infos[i].width))
            {
                printf("\n%s failed\n", IDctConf_infos[i].name);
                return false;
            }
        }
    }

    if (opt.deQuant)
    {
        if (!check_xdequant_primitive(ref.deQuant, opt.deQuant))
        {
            printf("XDeQuant: Failed!\n");
            return false;
        }
    }

    return true;
}

void MBDstHarness::measureSpeed(const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{
    for (int value = 0; value < NUM_DCTS; value++)
    {
        if (opt.dct[value])
        {
            printf("%s\t\t", DctConf_infos[value].name);
            REPORT_SPEEDUP(opt.dct[value], ref.dct[value], mbuf1, mintbuf1, DctConf_infos[value].width);
        }
    }

    for (int value = 0; value < NUM_IDCTS; value++)
    {
        if (opt.idct[value])
        {
            printf("%s\t\t", IDctConf_infos[value].name);
            REPORT_SPEEDUP(opt.idct[value], ref.idct[value], mbuf1, mbuf2, IDctConf_infos[value].width);
        }
    }

    if (opt.deQuant)
    {
        printf("xDeQuant\t\t");
        REPORT_SPEEDUP(opt.deQuant, ref.deQuant, 8, mintbuf1, mintbuf3, 32, 32, 5, 2, false, 5, mintbuf2);
    }
}
