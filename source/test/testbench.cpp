/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Gopu Govindaswamy <gopu@govindaswamy.org>
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

#include "unittest.h"
#include "primitives.h"

#include <ctype.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

using namespace x265;

/* pbuf1, pbuf2: initialized to random pixel data and shouldn't write into them. */
pixel *pbuf1, *pbuf2;
uint16_t do_singleprimitivecheck = 0;
uint16_t curpar = 0;

#define BENCH_ALIGNS 16
#if HIGH_BIT_DEPTH
#define PIXEL_MAX ((1 << 10) - 1)
#else
#define PIXEL_MAX ((1 << 8) - 1)
#endif

// test all implemented pixel comparison primitives
static int check_pixelprimitives(const EncoderPrimitives& cprimitives, const EncoderPrimitives& vectorprimitives)
{
    uint32_t j = 0, i = 0;

    for (; curpar < NUM_PARTITIONS; curpar++)
    {
        // if the satd is not available for vector no need to test
        if (vectorprimitives.satd[curpar])
        {
            j = 0;
            for (i = 0; i <= 100; i++)
            {
                int vres = vectorprimitives.satd[curpar](pbuf1 + j, 16, pbuf2, 16);
                int cres = cprimitives.satd[curpar](pbuf1 + j, 16, pbuf2, 16);
                if (vres != cres)
                {
                    printf("FAILED COMPARISON for SATD partition - %d \n", curpar);
                    return -1;
                }
                j += 16;
            }
        }
        // if the satd is not available for vector no need to test
        if (vectorprimitives.sad[curpar])
        {
            j = 0;
            for (i = 0; i <= 100; i++)
            {
                int vres = vectorprimitives.sad[curpar](pbuf1 + j, 16, pbuf2, 16);
                int cres = cprimitives.sad[curpar](pbuf1 + j, 16, pbuf2, 16);
                if (vres != cres)
                {
                    printf("FAILED COMPARISON for SATD partition - %d \n", curpar);
                    return -1;
                }
                j += 16;
            }
        }

        if (do_singleprimitivecheck == 1)
            return 0;
    }
    if (vectorprimitives.sa8d_8x8)
    {
        j = 0;
        for (i = 0; i <= 100; i++)
        {
            int vres = vectorprimitives.sa8d_8x8(pbuf1 + j, 16, pbuf2, 16);
            int cres = cprimitives.sa8d_8x8(pbuf1 + j, 16, pbuf2, 16);
            if (vres != cres)
            {
                printf("FAILED COMPARISON for sa8d_8x8\n");
                return -1;
            }
            j += 16;
        }
    }
    if (vectorprimitives.sa8d_16x16)
    {
        j = 0;
        for (i = 0; i <= 100; i++)
        {
            int vres = vectorprimitives.sa8d_16x16(pbuf1 + j, 16, pbuf2, 16);
            int cres = cprimitives.sa8d_16x16(pbuf1 + j, 16, pbuf2, 16);
            if (vres != cres)
            {
                printf("FAILED COMPARISON for sa8d_8x8\n");
                return -1;
            }
            j += 16;
        }
    }

    return 0;
}

static int check_all_funcs(const EncoderPrimitives& cprimitives, const EncoderPrimitives& vectorprimitives)
{
    return check_pixelprimitives(cprimitives, vectorprimitives);
}

int main(int argc, char *argv[])
{
    int ret = 0;
    int cpuid = CpuIDDetect();

    for (int i = 1; i < argc-1; i+=2)
    {
        if (!strcmp(argv[i], "--primitive"))
        {
            do_singleprimitivecheck = 1;
            curpar = atoi(argv[i+1]);
        }
        else if (!strcmp(argv[i], "--cpuid"))
        {
            cpuid = atoi(argv[i+1]);
        }
    }

    pbuf1 = (pixel*)malloc(0x1e00 * sizeof(pixel) + 16 * BENCH_ALIGNS);
    pbuf2 = (pixel*)malloc(0x1e00 * sizeof(pixel) + 16 * BENCH_ALIGNS);
    if (!pbuf1 || !pbuf2)
    {
        fprintf(stderr, "malloc failed, unable to initiate tests!\n");
        return -1;
    }

    for (int i = 0; i < 0x1e00; i++)
    {
        //Generate the Random Buffer for Testing
        pbuf1[i] = rand() & PIXEL_MAX;
        pbuf2[i] = rand() & PIXEL_MAX;
    }

    EncoderPrimitives cprim;
    Setup_C_Primitives(cprim);

    EncoderPrimitives vecprim;
    memset(&vecprim, 0, sizeof(vecprim));
    EncoderPrimitives asmprim;
    memset(&asmprim, 0, sizeof(asmprim));

#if ENABLE_VECTOR_PRIMITIVES
    Setup_Vector_Primitives(vecprim, cpuid);
    ret = check_all_funcs(cprim, vecprim);
    if (ret)
    {
        fprintf(stderr, "x265: at least vector primitive has failed. Go and fix that Right Now!\n");
        return -1;
    }
#endif

#if ENABLE_ASM_PRIMITIVES
    Setup_Vector_Primitives(asmprim, cpuid);
    ret = check_all_funcs(cprim, asmprim);
    if (ret)
    {
        fprintf(stderr, "x265: at least assembly primitive has failed. Go and fix that Right Now!\n");
        return -1;
    }
#endif

    fprintf(stderr, "x265: All tests passed Yeah :)\n");

    return 0;
}
