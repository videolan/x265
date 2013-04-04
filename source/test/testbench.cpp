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

#if _MSC_VER
#define snprintf _snprintf
#define strdup _strdup
#endif

using namespace x265;

/* pbuf1, pbuf2: initialized to random pixel data and shouldn't write into them. */
pixel *pbuf1, *pbuf2;
uint16_t do_singleprimitivecheck = 0;
uint16_t numofprim = 0;
uint16_t cpuid = 0;

#define BENCH_ALIGNS 16
#if HIGH_BIT_DEPTH
#define PIXEL_MAX ((1 << 10) - 1)
#else
#define PIXEL_MAX ((1 << 8) - 1)
#endif

#if _MSC_VER
#pragma warning(disable: 4505)
#endif

//Sample Testing for satdx*x
static int check_pixelprimitives(const EncoderPrimitives& cprimitives, const EncoderPrimitives& vectorprimitives)
{
    uint32_t ret = 0;
    uint32_t j = 0, i = 0;
    uint32_t var_v[100], var_c[100];

    //Do the bench for 16 - Number of Partitions
    while (numofprim < NUM_PARTITIONS)
    {
        //if the satd is not available for vector no need to testbench
        if (vectorprimitives.satd[tprimitives[numofprim]])
        {
            //run the Vectorised primitives 100 times and store the output
            j = 0;
            for (i = 0; i <= 100; i++)
            {
                var_v[i] = vectorprimitives.satd[tprimitives[numofprim]](pbuf1 + j, 16, pbuf2, 16);
                j += 16;
            }

            //run the c primitives 100 times and store the output
            j = 0;
            for (i = 0; i <= 100; i++)
            {
                var_c[i] = cprimitives.satd[tprimitives[numofprim]](pbuf1 + j, 16, pbuf2, 16);
                j += 16;
            }

            //compare both the output
            i = 0;
            while (i != 100)
            {
                if (var_c[i] != var_v[i])
                {
                    printf("FAILED COMPARISON for Primitives - %d \n", numofprim);
                    return -1;
                }

                i++;
            }

            numofprim++;
        }
        else //if there is no vectorized function for satd then need not to do testbench
            numofprim++;

        if (do_singleprimitivecheck == 1)
            break;
    }

    return ret;
}

static int check_all_funcs(const EncoderPrimitives& cprimitives, const EncoderPrimitives& vectorprimitives)
{
    return check_pixelprimitives(cprimitives, vectorprimitives);
}

int main(int argc, char *argv[])
{
    int ret = 0;

    if (argc > 1 && !strncmp(argv[1], "--primitive", 11))
    {
        do_singleprimitivecheck = 1;
        numofprim = atoi(argv[2]);
    }

    if (argc > 1 && !strncmp(argv[1], "--cpuid", 7))
    {
        cpuid = atoi(argv[2]);
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
    if (cpuid == 0)
        cpuid = CpuIDDetect();

#if defined(__GNUC__) || defined(_MSC_VER)
    if (cpuid > 1) Setup_Vec_Primitives_sse2(vecprim);

    if (cpuid > 2) Setup_Vec_Primitives_sse3(vecprim);

    if (cpuid > 3) Setup_Vec_Primitives_ssse3(vecprim);

    if (cpuid > 4) Setup_Vec_Primitives_sse41(vecprim);

    if (cpuid > 5) Setup_Vec_Primitives_sse42(vecprim);

#endif // if defined(__GNUC__) || defined(_MSC_VER)
#if (defined(_MSC_VER) && _MSC_VER >= 1600) || defined(__GNUC__)
    if (cpuid > 6) Setup_Vec_Primitives_avx(vecprim);
#endif
#if defined(_MSC_VER) && _MSC_VER >= 1700
    if (cpuid > 7) Setup_Vec_Primitives_avx2(vecprim);
#endif

    ret = check_all_funcs(cprim, vecprim); //do the output validation for c and vector primitives
    if (ret)
    {
        fprintf(stderr, "x265: at least one test has failed. Go and fix that Right Now!\n");
        return -1;
    }

    fprintf(stderr, "x265: All tests passed Yeah :)\n");

    return 0;
}
