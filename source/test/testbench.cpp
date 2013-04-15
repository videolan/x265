/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Gopu Govindaswamy <gopu@govindaswamy.org>
 *          Mandar Gurav <mandar@multicorewareinc.com>
 *          Mahesh Pittala <mahesh@multicorewareinc.com>
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
#include <time.h>
#include <iostream>
#include <ctype.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Code snippet from http://www.winehq.org/pipermail/wine-devel/2003-June/018082.html begins
// this part is windows implementation of Gettimeoffday() function

#ifndef _TIMEVAL_H
#define _TIMEVAL_H

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <time.h>

#if defined(_MSC_VER) || defined(__BORLANDC__)
#define EPOCHFILETIME (116444736000000000i64)
#else
#define EPOCHFILETIME (116444736000000000LL)
#endif
__inline int gettimeofday(struct timeval *tv,  struct timezone *tz)
{
    FILETIME        ft;
    LARGE_INTEGER   li;
    __int64         t;

    if (tv)
    {
        GetSystemTimeAsFileTime(&ft);
        li.LowPart  = ft.dwLowDateTime;
        li.HighPart = ft.dwHighDateTime;
        t  = li.QuadPart;       /* In 100-nanosecond intervals */
        t -= EPOCHFILETIME;     /* Offset to the Epoch time */
        t /= 10;                /* In microseconds */
        tv->tv_sec  = (long)(t / 1000000);
        tv->tv_usec = (long)(t % 1000000);
    }

    return 0;
}

#else  /* _WIN32 */

#include <sys/time.h>

#endif /* _WIN32 */

#endif /* _TIMEVAL_H */

// Code snippet from http://www.winehq.org/pipermail/wine-devel/2003-June/018082.html ends

using namespace x265;

/* Used for filter */
#define IF_INTERNAL_PREC 14 ///< Number of bits for internal precision
#define IF_FILTER_PREC    6 ///< Log2 of sum of filter taps
#define IF_INTERNAL_OFFS (1 << (IF_INTERNAL_PREC - 1)) ///< Offset used internally
#define NTAPS_LUMA       8 ///< Number of taps for luma
const short m_lumaFilter[4][NTAPS_LUMA] =
{
{
    0, 0,   0, 64,  0,   0, 0,  0
},
{
    -1, 4, -10, 58, 17,  -5, 1,  0
},
{
    -1, 4, -11, 40, 40, -11, 4, -1
},
{
    0, 1,  -5, 17, 58, -10, 4, -1
}
};
char FilterConf_names[16][40] =
{
    //Naming convention used is - isVertical_N_isFirst_isLast
    "Hor_N=4_isFirst=0_isLast=0",
    "Hor_N=4_isFirst=0_isLast=1",
    "Hor_N=4_isFirst=1_isLast=0",
    "Hor_N=4_isFirst=1_isLast=1",

    "Hor_N=8_isFirst=0_isLast=0",
    "Hor_N=8_isFirst=0_isLast=1",
    "Hor_N=8_isFirst=1_isLast=0",
    "Hor_N=8_isFirst=1_isLast=1",

    "Ver_N=4_isFirst=0_isLast=0",
    "Ver_N=4_isFirst=0_isLast=1",
    "Ver_N=4_isFirst=1_isLast=0",
    "Ver_N=4_isFirst=1_isLast=1",

    "Ver_N=8_isFirst=0_isLast=0",
    "Ver_N=8_isFirst=0_isLast=1",
    "Ver_N=8_isFirst=1_isLast=0",
    "Ver_N=8_isFirst=1_isLast=1"
};
pixel *pixel_buff;
short *IPF_buff1, *IPF_buff2;
int t_size;

/* pbuf1, pbuf2: initialized to random pixel data and shouldn't write into them. */
pixel *pbuf1, *pbuf2;
short *mbuf1, *mbuf2, *mbuf3;
#define BENCH_ALIGNS 16

// Initialize the Func Names for all the Pixel Comp
static const char *FuncNames[NUM_PARTITIONS] =
{
    "4x4", "8x4", "4x8", "8x8", "4x16", "16x4", "8x16", "16x8", "16x16", "4x32", "32x4", "8x32",
    "32x8", "16x32", "32x16", "32x32", "4x64", "64x4", "8x64", "64x8", "16x64", "64x16", "32x64", "64x32", "64x64"
};

#if HIGH_BIT_DEPTH
#define BIT_DEPTH 10
#else
#define BIT_DEPTH 8
#endif

#define PIXEL_MAX ((1 << BIT_DEPTH) - 1)

/* To-do List: Generate the stride values at run time in each run
 *
 */

#define MILSECS_IN_SEC 1000     // Number of milliseconds in a second
#define NUM_ITERATIONS_CYCLE 10000000    // Number of iterations for cycle count
#define INCR 16     // Number of bytes the input window shifts across the total buffer
#define STRIDE 16   // Stride value used while calling primitives

static double timevaldiff(struct timeval *starttime, struct timeval *finishtime)
{
    double msec;

    msec =  (finishtime->tv_sec - starttime->tv_sec) * MILSECS_IN_SEC;
    msec += (double)(finishtime->tv_usec - starttime->tv_usec) / MILSECS_IN_SEC;
    return msec;
}

static void check_cycle_count(const EncoderPrimitives& cprim, const EncoderPrimitives& vecprim)
{
    struct timeval ts, te;

    for (int curpar = 0; curpar < NUM_PARTITIONS; curpar++)
    {
        if (vecprim.satd[curpar])
        {
            gettimeofday(&ts, NULL);
            for (int j = 0; j < NUM_ITERATIONS_CYCLE; j++)
            {
                vecprim.satd[curpar](pbuf1, STRIDE, pbuf2, STRIDE);
            }

            gettimeofday(&te, NULL);
            printf("\nsatd[%s] vectorized primitive: (%1.4f ms) ", FuncNames[curpar], timevaldiff(&ts, &te));

            gettimeofday(&ts, NULL);
            for (int j = 0; j < NUM_ITERATIONS_CYCLE; j++)
            {
                cprim.satd[curpar](pbuf1, STRIDE, pbuf2, STRIDE);
            }

            gettimeofday(&te, NULL);
            printf("\tC primitive: (%1.4f ms) ", timevaldiff(&ts, &te));
        }

        if (vecprim.sad[curpar])
        {
            gettimeofday(&ts, NULL);
            for (int j = 0; j < NUM_ITERATIONS_CYCLE; j++)
            {
                vecprim.sad[curpar](pbuf1, STRIDE, pbuf2, STRIDE);
            }

            gettimeofday(&te, NULL);
            printf("\nsad[%s] vectorized primitive: (%1.4f ms) ", FuncNames[curpar], timevaldiff(&ts, &te));

            gettimeofday(&ts, NULL);
            for (int j = 0; j < NUM_ITERATIONS_CYCLE; j++)
            {
                cprim.sad[curpar](pbuf1, STRIDE, pbuf2, STRIDE);
            }

            gettimeofday(&te, NULL);
            printf("\tC primitive: (%1.4f ms) ", timevaldiff(&ts, &te));
        }
    }

    if (vecprim.sa8d_8x8)
    {
        gettimeofday(&ts, NULL);
        for (int j = 0; j < NUM_ITERATIONS_CYCLE; j++)
        {
            vecprim.sa8d_8x8(pbuf1, STRIDE, pbuf2, STRIDE);
        }

        gettimeofday(&te, NULL);
        printf("\nsa8d_8x8 vectorized primitive: (%1.4f ms) ", timevaldiff(&ts, &te));

        gettimeofday(&ts, NULL);
        for (int j = 0; j < NUM_ITERATIONS_CYCLE; j++)
        {
            cprim.sa8d_8x8(pbuf1, STRIDE, pbuf2, STRIDE);
        }

        gettimeofday(&te, NULL);
        printf("\tC primitive: (%1.4f ms) ", timevaldiff(&ts, &te));
    }

    if (vecprim.sa8d_16x16)
    {
        gettimeofday(&ts, NULL);
        for (int j = 0; j < NUM_ITERATIONS_CYCLE; j++)
        {
            vecprim.sa8d_16x16(pbuf1, STRIDE, pbuf2, STRIDE);
        }

        gettimeofday(&te, NULL);
        printf("\nsa8d_16x16 vectorized primitive: (%1.4f ms) ", timevaldiff(&ts, &te));

        gettimeofday(&ts, NULL);
        for (int j = 0; j < NUM_ITERATIONS_CYCLE; j++)
        {
            cprim.sa8d_16x16(pbuf1, STRIDE, pbuf2, STRIDE);
        }

        gettimeofday(&te, NULL);
        printf("\tC primitive: (%1.4f ms) ", timevaldiff(&ts, &te));
    }

    if (vecprim.inversedst)
    {
        gettimeofday(&ts, NULL);
        for (int j = 0; j < NUM_ITERATIONS_CYCLE; j++)
        {
            vecprim.inversedst(mbuf1, mbuf2, 16);
        }

        gettimeofday(&te, NULL);
        printf("\nInversedst vectorized primitive: (%1.4f ms) ", timevaldiff(&ts, &te));

        gettimeofday(&ts, NULL);
        for (int j = 0; j < NUM_ITERATIONS_CYCLE; j++)
        {
            cprim.inversedst(mbuf1, mbuf2, 16);
        }

        gettimeofday(&te, NULL);
        printf("\tC primitive: (%1.4f ms) ", timevaldiff(&ts, &te));
    }

    /* Add logic here for testing performance of your new primitive*/
    int rand_height = rand() % 100;                 // Randomly generated Height
    int rand_width = rand() % 100;                  // Randomly generated Width
    short rand_val, rand_srcStride, rand_dstStride;

    rand_val = rand() % 24;                     // Random offset in the filter
    rand_srcStride = rand() % 100;              // Randomly generated srcStride
    rand_dstStride = rand() % 100;              // Randomly generated dstStride

    for (int value = 4; value < 8; value++)
    {
        memset(IPF_buff1, 0, t_size);               // Initialize output buffer to zero
        memset(IPF_buff2, 0, t_size);               // Initialize output buffer to zero
        if (vecprim.filter[value])
        {
            gettimeofday(&ts, NULL);
            for (int j = 0; j < NUM_ITERATIONS_CYCLE; j++)
            {
                vecprim.filter[value]((short*)(m_lumaFilter + rand_val), pixel_buff, rand_srcStride, (pixel*)IPF_buff1,
                                      rand_dstStride, rand_height, rand_width, BIT_DEPTH);
            }

            gettimeofday(&te, NULL);
            printf("\nfilter[%s] vectorized primitive: (%1.4f ms) ", FilterConf_names[value], timevaldiff(&ts, &te));

            gettimeofday(&ts, NULL);
            for (int j = 0; j < NUM_ITERATIONS_CYCLE; j++)
            {
                cprim.filter[value]((short*)(m_lumaFilter + rand_val), pixel_buff, rand_srcStride, (pixel*)IPF_buff1,
                                    rand_dstStride, rand_height, rand_width, BIT_DEPTH);
            }

            gettimeofday(&te, NULL);
            printf("\tC primitive: (%1.4f ms) ", timevaldiff(&ts, &te));
        }
    }
}

static int check_pixel_primitive(pixelcmp ref, pixelcmp opt)
{
    int j = 0;

    for (int i = 0; i <= 100; i++)
    {
        int vres = opt(pbuf1, STRIDE, pbuf2 + j, STRIDE);
        int cres = ref(pbuf1, STRIDE, pbuf2 + j, STRIDE);
        if (vres != cres)
            return -1;

        j += INCR;
    }

    return 0;
}

//Find the Output Comp and Cycle count
static int check_mbdst_primitive(mbdst ref, mbdst opt)
{
    int j = 0;
    int t_size = 32;

    for (int i = 0; i <= 100; i++)
    {
        opt(mbuf1 + j, mbuf2, 16);
        ref(mbuf1 + j, mbuf3, 16);

        if (memcmp(mbuf2, mbuf3, 32))
            return -1;

        j += INCR;
        memset(mbuf2, 0, t_size);
        memset(mbuf3, 0, t_size);
    }

    return 0;
}

static int check_IPFilter_primitive(IPFilter ref, IPFilter opt)
{
    int rand_height = rand() & 100;                 // Randomly generated Height
    int rand_width = rand() & 100;                  // Randomly generated Width
    int flag = 0;                                   // Return value
    short rand_val, rand_srcStride, rand_dstStride;

    for (int i = 0; i <= 100; i++)
    {
        memset(IPF_buff1, 0, t_size);               // Initialize output buffer to zero
        memset(IPF_buff2, 0, t_size);               // Initialize output buffer to zero

        rand_val = rand() & 24;                     // Random offset in the filter
        rand_srcStride = rand() & 100;              // Randomly generated srcStride
        rand_dstStride = rand() & 100;              // Randomly generated dstStride

        opt((short*)(m_lumaFilter + rand_val),
            pixel_buff,
            rand_srcStride,
            (pixel*)IPF_buff1,
            rand_dstStride,
            rand_height,
            rand_width,
            BIT_DEPTH);
        ref((short*)(m_lumaFilter + rand_val),
            pixel_buff,
            rand_srcStride,
            (pixel*)IPF_buff2,
            rand_dstStride,
            rand_height,
            rand_width,
            BIT_DEPTH);

        if (memcmp(IPF_buff1, IPF_buff2, t_size))
        {
            flag = -1;                                          // Test Failed
            break;
        }
    }

    return flag;
}

int init_pixelcmp_buffers()
{
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

    return 0;
}

int init_IPFilter_buffers()
{
    t_size = 200 * 200;
    pixel_buff = (pixel*)malloc(t_size * sizeof(pixel));     // Assuming max_height = max_width = max_srcStride = max_dstStride = 100
    IPF_buff1 = (short*)malloc(t_size * sizeof(short));      // Output Buffer1
    IPF_buff2 = (short*)malloc(t_size * sizeof(short));      // Output Buffer2

    if (!pixel_buff || !IPF_buff1 || !IPF_buff2)
    {
        fprintf(stderr, "init_IPFilter_buffers: malloc failed, unable to initiate tests!\n");
        return -1;
    }

    for (int i = 0; i < t_size; i++)                                    // Initialize input buffer
    {
        int isPositive = rand() & 1;                                    // To randomly generate Positive and Negative values
        isPositive = (isPositive) ? 1 : -1;
        pixel_buff[i] = isPositive * (rand() & PIXEL_MAX);
    }

    return 0;
}

int clean_pixelcmp_buffers()
{
    free(pbuf1);
    free(pbuf2);
    return 0;
}

int clean_IPFilter_buffers()
{
    free(IPF_buff1);
    free(IPF_buff2);
    free(pixel_buff);
    return 0;
}

int init_mbdst_buffers()
{
    int t_size = 32;

    mbuf1 = (short*)malloc(0x1e00 * sizeof(pixel) + 16 * BENCH_ALIGNS);
    mbuf2 = (short*)malloc(t_size);
    mbuf3 = (short*)malloc(t_size);
    if (!mbuf1 || !mbuf2 || !mbuf3)
    {
        fprintf(stderr, "malloc failed, unable to initiate tests!\n");
        return -1;
    }

    memcpy(mbuf1, pbuf1, 64 * 100);
    memset(mbuf2, 0, t_size);
    memset(mbuf3, 0, t_size);
    return 0;
}

int clean_mbdst_buffers()
{
    free(mbuf1);
    free(mbuf2);
    free(mbuf3);
    return 0;
}

// test all implemented primitives
static int check_all_primitives(const EncoderPrimitives& cprimitives, const EncoderPrimitives& vectorprimitives)
{
    uint16_t curpar = 0;

    /****************** Initialise and run pixelcmp primitives **************************/

    if (init_pixelcmp_buffers() < 0)
        return -1;

    for (; curpar < NUM_PARTITIONS; curpar++)
    {
        if (vectorprimitives.satd[curpar])
        {
            if (check_pixel_primitive(cprimitives.satd[curpar], vectorprimitives.satd[curpar]) < 0)
            {
                printf("satd[%s]: failed!\n", FuncNames[curpar]);
                return -1;
            }

            printf("\nsatd[%s]: passed ", FuncNames[curpar]);
        }

        if (vectorprimitives.sad[curpar])
        {
            if (check_pixel_primitive(cprimitives.sad[curpar], vectorprimitives.sad[curpar]) < 0)
            {
                printf("sad[%s]: failed!\n", FuncNames[curpar]);
                return -1;
            }

            printf("\nsad[%s]: passed ", FuncNames[curpar]);
        }
    }

    if (vectorprimitives.sa8d_8x8)
    {
        if (check_pixel_primitive(cprimitives.sa8d_8x8, vectorprimitives.sa8d_8x8) < 0)
        {
            printf("sa8d_8x8: failed!\n");
            return -1;
        }

        printf("\nsa8d_8x8: passed ");
    }

    if (vectorprimitives.sa8d_16x16)
    {
        if (check_pixel_primitive(cprimitives.sa8d_16x16, vectorprimitives.sa8d_16x16) < 0)
        {
            printf("sa8d_16x16: failed!\n");
            return -1;
        }

        printf("\nsa8d_16x16: passed ");
    }

    /********** Run Filter Primitives *******************/
    if (init_IPFilter_buffers() < 0)
        return -1;

    for (int value = 4; value < 8; value++)
    {
        if (vectorprimitives.filter[value])
        {
            if (check_IPFilter_primitive(cprimitives.filter[value], vectorprimitives.filter[value]) < 0)
            {
                printf("\nfilter: Failed!\n");
                return -1;
            }

            printf("\nFilter[%s]: passed ", FilterConf_names[value]);
        }
    }

    /********** Initialise and run mbdst Primitives *******************/

    if (init_mbdst_buffers() < 0)
        return -1;

    if (vectorprimitives.inversedst)
    {
        if (check_mbdst_primitive(cprimitives.inversedst, vectorprimitives.inversedst) < 0)
        {
            printf("Inversedst: Failed!\n");
            return -1;
        }

        printf("\nInversedst: passed ");
    }

    /* Initialise and check your primitives here **********/

    /******************* Cycle count for all primitives **********************/
    check_cycle_count(cprimitives, vectorprimitives);

    /********************* Clean all buffers *****************************/
    clean_pixelcmp_buffers();
    clean_mbdst_buffers();
    clean_IPFilter_buffers();
    return 0;
}

int main(int argc, char *argv[])
{
    int ret = 0;
    int cpuid = CpuIDDetect();

    for (int i = 1; i < argc - 1; i += 2)
    {
        if (!strcmp(argv[i], "--cpuid"))
        {
            cpuid = atoi(argv[i + 1]);
        }
    }

    EncoderPrimitives cprim;
    Setup_C_Primitives(cprim);

    EncoderPrimitives vecprim;
    memset(&vecprim, 0, sizeof(vecprim));

#if ENABLE_VECTOR_PRIMITIVES
    Setup_Vector_Primitives(vecprim, cpuid);
    printf("Testing vector class primitives\n");
    ret = check_all_primitives(cprim, vecprim);
    if (ret)
    {
        fprintf(stderr, "x265: at least one vector primitive has failed. Go and fix that Right Now!\n");
        return -1;
    }

#endif

#if ENABLE_ASM_PRIMITIVES
    EncoderPrimitives asmprim;
    memset(&asmprim, 0, sizeof(asmprim));
    Setup_Assembly_Primitives(asmprim, cpuid);
    printf("Testing assembly primitives\n");
    ret = check_all_primitives(cprim, asmprim);
    if (ret)
    {
        fprintf(stderr, "x265: at least one assembly primitive has failed. Go and fix that Right Now!\n");
        return -1;
    }

#endif // if ENABLE_ASM_PRIMITIVES

    fprintf(stderr, "x265: All tests passed Yeah :)\n");

    return 0;
}
