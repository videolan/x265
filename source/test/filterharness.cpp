/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
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

#include "filterharness.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

using namespace x265;
#define IF_INTERNAL_PREC 14 ///< Number of bits for internal precision
#define IF_FILTER_PREC    6 ///< Log2 of sum of filter taps
#define IF_INTERNAL_OFFS (1 << (IF_INTERNAL_PREC - 1)) ///< Offset used internally
#define NTAPS_LUMA       8 ///< Number of taps for luma

const short m_lumaFilter[4][8] =
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

const char *FilterConf_names[] =
{
    // Naming convention used is - isVertical_N_isFirst_isLast
    "Hor_N=4_isLast=0",
    "Hor_N=4_isLast=1",

    "Hor_N=8_isLast=0",
    "Hor_N=8_isLast=1",

    "Ver_N=4_isLast=0",
    "Ver_N=4_isLast=1",

    "Ver_N=8_isLast=0",
    "Ver_N=8_isLast=1"
};

FilterHarness::FilterHarness()
{
    ipf_t_size = 200 * 200;
    pixel_buff = (pixel*)malloc(ipf_t_size * sizeof(pixel));     // Assuming max_height = max_width = max_srcStride = max_dstStride = 100
    IPF_vec_output = (short*)malloc(ipf_t_size * sizeof(short)); // Output Buffer1
    IPF_C_output = (short*)malloc(ipf_t_size * sizeof(short));   // Output Buffer2

    if (!pixel_buff || !IPF_vec_output || !IPF_C_output)
    {
        fprintf(stderr, "init_IPFilter_buffers: malloc failed, unable to initiate tests!\n");
        exit(-1);
    }

    for (int i = 0; i < ipf_t_size; i++)                         // Initialize input buffer
    {
        int isPositive = rand() & 1;                             // To randomly generate Positive and Negative values
        isPositive = (isPositive) ? 1 : -1;
        pixel_buff[i] = isPositive * (rand() & PIXEL_MAX);
    }
}

FilterHarness::~FilterHarness()
{
    free(IPF_vec_output);
    free(IPF_C_output);
    free(pixel_buff);
}

bool FilterHarness::check_IPFilter_primitive(IPFilter ref, IPFilter opt)
{
    int rand_height = rand() & 100;                 // Randomly generated Height
    int rand_width = rand() & 100;                  // Randomly generated Width
    short rand_val, rand_srcStride, rand_dstStride;

    static int i = 0;
    int isFirst = false, isLast;
    int bitDepth = 8;

    int offset;
    short maxVal;
    int headRoom = IF_INTERNAL_PREC - bitDepth;
    int shift = IF_FILTER_PREC;

    isLast = i;
    if (isLast)
    {
        shift += (isFirst) ? 0 : headRoom;
        offset = 1 << (shift - 1);
        offset += (isFirst) ? 0 : IF_INTERNAL_OFFS << IF_FILTER_PREC;
        maxVal = (1 << bitDepth) - 1;
        i++;
    }
    else
    {
        shift -= (isFirst) ? headRoom : 0;
        offset = (isFirst) ? -IF_INTERNAL_OFFS << shift : 0;
        maxVal = 0;
        i++;
    }

    for (int i = 0; i <= 100; i++)
    {
        memset(IPF_vec_output, 0, ipf_t_size);          // Initialize output buffer to zero
        memset(IPF_C_output, 0, ipf_t_size);            // Initialize output buffer to zero

        rand_val = rand() & 24;                         // Random offset in the filter
        rand_srcStride = rand() & 100;                  // Randomly generated srcStride
        rand_dstStride = rand() & 100;                  // Randomly generated dstStride

        opt((short*)(m_lumaFilter + rand_val),
            pixel_buff,
            rand_srcStride,
            (pixel*)IPF_vec_output,
            rand_dstStride,
            rand_height,
            rand_width,
            maxVal, offset, shift);
        ref((short*)(m_lumaFilter + rand_val),
            pixel_buff,
            rand_srcStride,
            (pixel*)IPF_C_output,
            rand_dstStride,
            rand_height,
            rand_width,
            maxVal, offset, shift);

        if (memcmp(IPF_vec_output, IPF_C_output, ipf_t_size))
            return false;
    }

    return true;
}

bool FilterHarness::testCorrectness(const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{
    for (int value = 2; value < 4; value++)
    {
        if (opt.filter[value])
        {
            if (!check_IPFilter_primitive(ref.filter[value], opt.filter[value]))
            {
                printf("\nfilter[%s] failed\n", FilterConf_names[value]);
                return false;
            }
        }
    }

    return true;
}

#define FILTER_ITERATIONS   100000

void FilterHarness::measureSpeed(const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{
    Timer *t = Timer::CreateTimer();
    /* Add logic here for testing performance of your new primitive*/
    int rand_height = rand() % 100;             // Randomly generated Height
    int rand_width = rand() % 100;              // Randomly generated Width
    short rand_val, rand_srcStride, rand_dstStride;

    rand_val = rand() % 24;                     // Random offset in the filter
    rand_srcStride = rand() % 100;              // Randomly generated srcStride
    rand_dstStride = rand() % 100;              // Randomly generated dstStride
    int flag = 0;
    int isFirst = false, isLast;
    int bitDepth = 8;
    for (int value = 2; value < 4; value++)
    {
        if (flag == 2)
        {
            isFirst = true;
        }
        else if (flag == 3)
        {
            isFirst = true;
            flag = -1;
        }

        flag++;
        isLast = value % 2;
        int offset;
        short maxVal;
        int headRoom = IF_INTERNAL_PREC - bitDepth;
        int shift = IF_FILTER_PREC;
        if (isLast)
        {
            shift += (isFirst) ? 0 : headRoom;
            offset = 1 << (shift - 1);
            offset += (isFirst) ? 0 : IF_INTERNAL_OFFS << IF_FILTER_PREC;
            maxVal = (1 << bitDepth) - 1;
        }
        else
        {
            shift -= (isFirst) ? headRoom : 0;
            offset = (isFirst) ? -IF_INTERNAL_OFFS << shift : 0;
            maxVal = 0;
        }

        memset(IPF_vec_output, 0, ipf_t_size);  // Initialize output buffer to zero
        memset(IPF_C_output, 0, ipf_t_size);    // Initialize output buffer to zero
        if (opt.filter[value])
        {
            t->Start();
            for (int j = 0; j < FILTER_ITERATIONS; j++)
            {
                opt.filter[value]((short*)(m_lumaFilter + rand_val), pixel_buff, rand_srcStride, (pixel*)IPF_vec_output,
                                  rand_dstStride, rand_height, rand_width,  maxVal, offset, shift);
            }

            t->Stop();
            printf("\nfilter[%s]\tVec: (%1.2f ms) ", FilterConf_names[value], t->ElapsedMS());

            t->Start();
            for (int j = 0; j < FILTER_ITERATIONS; j++)
            {
                ref.filter[value]((short*)(m_lumaFilter + rand_val), pixel_buff, rand_srcStride, (pixel*)IPF_vec_output,
                                  rand_dstStride, rand_height, rand_width,  maxVal, offset, shift);
            }

            t->Stop();
            printf("\tC: (%1.2f ms) ", t->ElapsedMS());
        }
    }

    t->Release();
}
