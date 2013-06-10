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

#include "mbdstharness.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>

using namespace x265;

const char *ButterflyConf_names[] =
{
    "4\t",
    "Inverse4",
    "8\t",
    "Inverse8",
    "16\t",
    "Inverse16",
    "32\t",
    "Inverse32"
};

const char *DctConf_names[] =
{
    "Dst4x4\t",
   "IDst4x4\t",
    "Dct4x4\t",
    "Dct8x8\t",
    "Dct16x16\t",
    "Dct32x32\t",
};

MBDstHarness::MBDstHarness()
{
    mb_t_size = 6400;

    mbuf1 = (short*)TestHarness::alignedMalloc(sizeof(short), 0x1e00, 32);
    mbuf2 = (short*)TestHarness::alignedMalloc(sizeof(short), 0x1e00, 32);
    mbuf3 = (short*)TestHarness::alignedMalloc(sizeof(short), 0x1e00, 32);
    mbuf4 = (short*)TestHarness::alignedMalloc(sizeof(short), 0x1e00, 32);

    mintbuf1 = (int*)TestHarness::alignedMalloc(sizeof(int), 0x1e00, 32);
    mintbuf2 = (int*)TestHarness::alignedMalloc(sizeof(int), 0x1e00, 32);
    mintbuf3 = (int*)TestHarness::alignedMalloc(sizeof(int), 0x1e00, 32);
    mintbuf4 = (int*)TestHarness::alignedMalloc(sizeof(int), 0x1e00, 32);

    if (!mbuf1 || !mbuf2 || !mbuf3 || !mbuf4)
    {
        fprintf(stderr, "malloc failed, unable to initiate tests!\n");
        exit(1);
    }

    if (!mintbuf1 || !mintbuf2 || !mintbuf3 || !mintbuf4)
    {
        fprintf(stderr, "malloc failed, unable to initiate tests!\n");
        exit(1);
    }

    for (int i = 0; i < 64 * 100; i++)
    {
        mbuf1[i] = rand() & PIXEL_MAX;
    }

    for (int i = 0; i < 64 * 100; i++)
    {
        mintbuf1[i] = rand() & PIXEL_MAX;
        mintbuf2[i] = rand() & PIXEL_MAX;
    }

    memset(mbuf2, 0, mb_t_size);
    memset(mbuf3, 0, mb_t_size);
    memset(mbuf4, 0, mb_t_size);

    memset(mintbuf3, 0, mb_t_size);
    memset(mintbuf4, 0, mb_t_size);
}

MBDstHarness::~MBDstHarness()
{
    TestHarness::alignedFree(mbuf1);
    TestHarness::alignedFree(mbuf2);
    TestHarness::alignedFree(mbuf3);
    TestHarness::alignedFree(mbuf4);

    TestHarness::alignedFree(mintbuf1);
    TestHarness::alignedFree(mintbuf2);
    TestHarness::alignedFree(mintbuf3);
    TestHarness::alignedFree(mintbuf4);
}

bool MBDstHarness::check_mbdst_primitive(mbdst ref, mbdst opt)
{
    int j = 0;
    int mem_cmp_size = 32;

    for (int i = 0; i <= 100; i++)
    {
        opt(mbuf1 + j, mbuf2, 16);
        ref(mbuf1 + j, mbuf3, 16);

        if (memcmp(mbuf2, mbuf3, mem_cmp_size))
            return false;

        j += 16;
        memset(mbuf2, 0, mem_cmp_size);
        memset(mbuf3, 0, mem_cmp_size);
    }

    return true;
}

bool MBDstHarness::check_butterfly16_primitive(butterfly ref, butterfly opt)
{
    int j = 0;
    int mem_cmp_size = 320; // 2*16*10 -> sizeof(short)*number of elements*number of lines

    for (int i = 0; i <= 100; i++)
    {
        opt(mbuf1 + j, mbuf2, 3, 10);
        ref(mbuf1 + j, mbuf3, 3, 10);

        if (memcmp(mbuf2, mbuf3, mem_cmp_size))
            return false;

        j += 16;
        memset(mbuf2, 0, mem_cmp_size);
        memset(mbuf3, 0, mem_cmp_size);
    }

    return true;
}

bool MBDstHarness::check_butterfly32_primitive(butterfly ref, butterfly opt)
{
    int j = 0;
    int mem_cmp_size = 640; // 2*32*10 -> sizeof(short)*number of elements*number of lines

    for (int i = 0; i <= 100; i++)
    {
        opt(mbuf1 + j, mbuf2, 3, 10);
        ref(mbuf1 + j, mbuf3, 3, 10);

        if (memcmp(mbuf2, mbuf3, mem_cmp_size))
            return false;

        j += 16;
        memset(mbuf2, 0, mem_cmp_size);
        memset(mbuf3, 0, mem_cmp_size);
    }

    return true;
}

bool MBDstHarness::check_butterfly8_primitive(butterfly ref, butterfly opt)
{
    int j = 0;
    int mem_cmp_size = 128; // 2*8*8 -> sizeof(short)*number of elements*number of lines

    for (int i = 0; i <= 100; i++)
    {
        opt(mbuf1 + j, mbuf2, 3, 8);
        ref(mbuf1 + j, mbuf3, 3, 8);

        if (memcmp(mbuf2, mbuf3, mem_cmp_size))
            return false;

        j += 16;
        memset(mbuf2, 0, mem_cmp_size);
        memset(mbuf3, 0, mem_cmp_size);
    }

    return true;
}

bool MBDstHarness::check_butterfly4_primitive(butterfly ref, butterfly opt)
{
    int j = 0;
    int mem_cmp_size = 32; // 2*4*4 -> sizeof(short)*number of elements*number of lines

    for (int i = 0; i <= 100; i++)
    {
        opt(mbuf1 + j, mbuf2, 1, 4);
        ref(mbuf1 + j, mbuf3, 1, 4);
        ref(mbuf3, mbuf4, 8, 4);

        if (memcmp(mbuf2, mbuf4, mem_cmp_size))
            return false;

        j += 16;
        memset(mbuf2, 0, mem_cmp_size);
        memset(mbuf3, 0, mem_cmp_size);
        memset(mbuf4, 0, mem_cmp_size);
    }

    return true;
}

bool MBDstHarness::check_butterfly4_inverse_primitive(butterfly ref, butterfly opt)
{
    int j = 0;
    int mem_cmp_size = 80; // 2*4*10 -> sizeof(short)*number of elements*number of lines

    for (int i = 0; i <= 100; i++)
    {
        opt(mbuf1 + j, mbuf2, 3, 10);
        ref(mbuf1 + j, mbuf3, 3, 10);

        if (memcmp(mbuf2, mbuf3, mem_cmp_size))
            return false;

        j += 16;
        memset(mbuf2, 0, mem_cmp_size);
        memset(mbuf3, 0, mem_cmp_size);
    }

    return true;
}

bool MBDstHarness::check_butterfly8_inverse_primitive(butterfly ref, butterfly opt)
{
    int j = 0;
    int mem_cmp_size = 160; // 2*8*10 -> sizeof(short)*number of elements*number of lines

    for (int i = 0; i <= 100; i++)
    {
        opt(mbuf1 + j, mbuf2, 3, 10);
        ref(mbuf1 + j, mbuf3, 3, 10);

        if (memcmp(mbuf2, mbuf3, mem_cmp_size))
            return false;

        j += 16;
        memset(mbuf2, 0, mem_cmp_size);
        memset(mbuf3, 0, mem_cmp_size);
    }

    return true;
}

bool MBDstHarness::check_butterfly16_inverse_primitive(butterfly ref, butterfly opt)
{
    int j = 0;
    int mem_cmp_size = 320; // 2*16*10 -> sizeof(short)*number of elements*number of lines

    for (int i = 0; i <= 5; i++)
    {
        opt(mbuf1 + j, mbuf2, 3, 10);
        ref(mbuf1 + j, mbuf3, 3, 10);

        if (memcmp(mbuf2, mbuf3, mem_cmp_size))
            return false;

        j += 16;
        memset(mbuf2, 0, mem_cmp_size);
        memset(mbuf3, 0, mem_cmp_size);
    }

    return true;
}

bool MBDstHarness::check_butterfly32_inverse_primitive(butterfly ref, butterfly opt)
{
    int j = 0;
    int mem_cmp_size = 640; // 2*16*10 -> sizeof(short)*number of elements*number of lines

    for (int i = 0; i <= 5; i++)
    {
        opt(mbuf1 + j, mbuf2, 3, 10);
        ref(mbuf1 + j, mbuf3, 3, 10);

        if (memcmp(mbuf2, mbuf3, mem_cmp_size))
            return false;

        j += 16;
        memset(mbuf2, 0, mem_cmp_size);
        memset(mbuf3, 0, mem_cmp_size);
    }

    return true;
}

bool MBDstHarness::check_dct4_primitive(dct_t ref, dct_t opt)
{
    int j = 0;
    int mem_cmp_size = 32; // 2*4*4 -> sizeof(short)*number of elements*number of lines

    for (int i = 0; i <= 100; i++)
    {
        ref(mbuf1 + j, mbuf3);
        opt(mbuf1 + j, mbuf2);

        if (memcmp(mbuf2, mbuf3, mem_cmp_size))
            return false;

        j += 16;
        memset(mbuf2, 0xCD, mem_cmp_size);
        memset(mbuf3, 0xCD, mem_cmp_size);
    }

    return true;
}

bool MBDstHarness::check_dct8_primitive(dct_t ref, dct_t opt)
{
    int j = 0;
    int mem_cmp_size = 128; // 2*8*8 -> sizeof(short)*number of elements*number of lines

    for (int i = 0; i <= 100; i++)
    {
        opt(mbuf1 + j, mbuf2);
        ref(mbuf1 + j, mbuf3);

        if (memcmp(mbuf2, mbuf3, mem_cmp_size))
            return false;

        j += 16;
        memset(mbuf2, 0xCD, mem_cmp_size);
        memset(mbuf3, 0xCD, mem_cmp_size);
    }

    return true;
}

bool MBDstHarness::check_xdequant_primitive(quant ref, quant opt)
{
    int j = 0;
    int mem_cmp_size = 1024; // 32*32

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
        memset(mintbuf3, 0, mem_cmp_size);
        memset(mintbuf4, 0, mem_cmp_size);
    }

    return true;
}

bool MBDstHarness::testCorrectness(const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{
    if (opt.inversedst)
    {
        if (!check_mbdst_primitive(ref.inversedst, opt.inversedst))
        {
            printf("Inversedst: Failed!\n");
            return false;
        }
    }

    if (opt.partial_butterfly[BUTTERFLY_16])
    {
        if (!check_butterfly16_primitive(ref.partial_butterfly[BUTTERFLY_16], opt.partial_butterfly[BUTTERFLY_16]))
        {
            printf("\npartialButterfly%s failed\n", ButterflyConf_names[BUTTERFLY_16]);
            return false;
        }
    }

    if (opt.partial_butterfly[BUTTERFLY_32])
    {
        if (!check_butterfly32_primitive(ref.partial_butterfly[BUTTERFLY_32], opt.partial_butterfly[BUTTERFLY_32]))
        {
            printf("\npartialButterfly%s failed\n", ButterflyConf_names[BUTTERFLY_32]);
            return false;
        }
    }

    if (opt.partial_butterfly[BUTTERFLY_8])
    {
        if (!check_butterfly8_primitive(ref.partial_butterfly[BUTTERFLY_8], opt.partial_butterfly[BUTTERFLY_8]))
        {
            printf("\npartialButterfly%s failed\n", ButterflyConf_names[BUTTERFLY_8]);
            return false;
        }
    }

    if (opt.partial_butterfly[BUTTERFLY_INVERSE_4])
    {
        if (!check_butterfly4_inverse_primitive(ref.partial_butterfly[BUTTERFLY_INVERSE_4], opt.partial_butterfly[BUTTERFLY_INVERSE_4]))
        {
            printf("\npartialButterfly%s failed\n", ButterflyConf_names[BUTTERFLY_INVERSE_4]);
            return false;
        }
    }

    if (opt.partial_butterfly[BUTTERFLY_INVERSE_8])
    {
        if (!check_butterfly8_inverse_primitive(ref.partial_butterfly[BUTTERFLY_INVERSE_8], opt.partial_butterfly[BUTTERFLY_INVERSE_8]))
        {
            printf("\npartialButterfly%s failed\n", ButterflyConf_names[BUTTERFLY_INVERSE_8]);
            return false;
        }
    }

    if (opt.partial_butterfly[BUTTERFLY_INVERSE_16])
    {
        if (!check_butterfly16_inverse_primitive(ref.partial_butterfly[BUTTERFLY_INVERSE_16], opt.partial_butterfly[BUTTERFLY_INVERSE_16]))
        {
            printf("\npartialButterfly%s failed\n", ButterflyConf_names[BUTTERFLY_INVERSE_16]);
            return false;
        }
    }

    if (opt.partial_butterfly[BUTTERFLY_INVERSE_32])
    {
        if (!check_butterfly32_inverse_primitive(ref.partial_butterfly[BUTTERFLY_INVERSE_32], opt.partial_butterfly[BUTTERFLY_INVERSE_32]))
        {
            printf("\npartialButterfly%s failed\n", ButterflyConf_names[BUTTERFLY_INVERSE_32]);
            return false;
        }
    }

    if (opt.partial_butterfly[BUTTERFLY_4])
    {
        if (!check_butterfly4_primitive(ref.partial_butterfly[BUTTERFLY_4], opt.partial_butterfly[BUTTERFLY_4]))
        {
            printf("\npartialButterfly%s failed\n", ButterflyConf_names[BUTTERFLY_4]);
            return false;
        }
    }

    if (opt.dct[DCT_4x4])
    {
        if (!check_dct4_primitive(ref.dct[DCT_4x4], opt.dct[DCT_4x4]))
        {
            printf("\n%s failed\n", DctConf_names[DCT_4x4]);
            return false;
        }
    }

    if (opt.dct[DCT_8x8])
    {
        if (!check_dct8_primitive(ref.dct[DCT_8x8], opt.dct[DCT_8x8]))
        {
            printf("\n%s failed\n", DctConf_names[DCT_8x8]);
            return false;
        }
    }

    if (opt.dct[IDST_4x4])
    {
        if (!check_dct4_primitive(ref.dct[IDST_4x4], opt.dct[IDST_4x4]))
        {
            printf("\n%s failed\n", DctConf_names[IDST_4x4]);
            return false;
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
    if (opt.inversedst)
    {
        printf("InverseDST\t\t");
        REPORT_SPEEDUP(opt.inversedst, ref.inversedst, mbuf1, mbuf2, 16);
    }

    for (int value = 0; value < 8; value++)
    {
        memset(mbuf2, 0, mb_t_size); // Initialize output buffer to zero
        memset(mbuf3, 0, mb_t_size); // Initialize output buffer to zero
        if (opt.partial_butterfly[value])
        {
            printf("partialButterfly%s", ButterflyConf_names[value]);
            REPORT_SPEEDUP(opt.partial_butterfly[value], ref.partial_butterfly[value], mbuf1, mbuf2, 3, 10);
        }
    }

    for (int value = 0; value < NUM_DCTS; value++)
    {
        memset(mbuf2, 0, mb_t_size); // Initialize output buffer to zero
        memset(mbuf3, 0, mb_t_size); // Initialize output buffer to zero
        if (opt.dct[value])
        {
            printf("%s\t\t", DctConf_names[value]);
            REPORT_SPEEDUP(opt.dct[value], ref.dct[value], mbuf1, mbuf2);
        }
    }

    if (opt.deQuant)
    {
        printf("xDeQuant\t\t");
        REPORT_SPEEDUP(opt.deQuant, ref.deQuant, 8, mintbuf1, mintbuf3, 32, 32, 5, 2, false, 5, mintbuf2);
    }
}
