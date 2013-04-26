/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Deepthi Devaki <deepthidevaki@multicorewareinc.com>,
 *          Rajesh Paulraj <rajesh@multicorewareinc.com>
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

#include "ipfilterharness.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

using namespace x265;

const short m_lumaFilter[4][8] =
{
    { 0, 0,   0, 64,  0,   0, 0,  0 },
    { -1, 4, -10, 58, 17,  -5, 1,  0 },
    { -1, 4, -11, 40, 40, -11, 4, -1 },
    { 0, 1,  -5, 17, 58, -10, 4, -1 }
};

IPFilterHarness::IPFilterHarness()
{
    ipf_t_size = 200 * 200;
    pixel_buff = (pixel*)malloc(ipf_t_size * sizeof(pixel));     // Assuming max_height = max_width = max_srcStride = max_dstStride = 100
    short_buff = (short*)malloc(ipf_t_size * sizeof(short));
    IPF_vec_output_s = (short*)malloc(ipf_t_size * sizeof(short)); // Output Buffer1
    IPF_C_output_s = (short*)malloc(ipf_t_size * sizeof(short));   // Output Buffer2
    IPF_vec_output_p = (pixel*)malloc(ipf_t_size * sizeof(pixel)); // Output Buffer1
    IPF_C_output_p = (pixel*)malloc(ipf_t_size * sizeof(pixel));   // Output Buffer2

    if (!pixel_buff || !short_buff || !IPF_vec_output_s || !IPF_vec_output_p || !IPF_C_output_s || !IPF_C_output_p)
    {
        fprintf(stderr, "init_IPFilter_buffers: malloc failed, unable to initiate tests!\n");
        exit(-1);
    }

    for (int i = 0; i < ipf_t_size; i++)                         // Initialize input buffer
    {
        int isPositive = rand() & 1;                             // To randomly generate Positive and Negative values
        isPositive = (isPositive) ? 1 : -1;
        pixel_buff[i] = (pixel)(rand() &  PIXEL_MAX);
        short_buff[i] = (isPositive) * (rand() &  SHRT_MAX);
    }
}

IPFilterHarness::~IPFilterHarness()
{
    free(IPF_vec_output_s);
    free(IPF_C_output_s);
    free(IPF_vec_output_p);
    free(IPF_C_output_p);
    free(short_buff);
    free(pixel_buff);
}

bool IPFilterHarness::check_IPFilter_primitive(x265::IPFilter_p_p ref, x265::IPFilter_p_p opt)
{
    int rand_height = rand() % 100;                 // Randomly generated Height
    int rand_width = rand() % 100;                  // Randomly generated Width
    short rand_val, rand_srcStride, rand_dstStride;

    for (int i = 0; i <= 100; i++)
    {
        memset(IPF_vec_output_p, 0, ipf_t_size);      // Initialize output buffer to zero
        memset(IPF_C_output_p, 0, ipf_t_size);        // Initialize output buffer to zero

        rand_val = rand() % 4;                     // Random offset in the filter
        rand_srcStride = rand() % 100;              // Randomly generated srcStride
        rand_dstStride = rand() % 100;              // Randomly generated dstStride

        opt(8, pixel_buff + 3 * rand_srcStride,
            rand_srcStride,
            IPF_vec_output_p,
            rand_dstStride,
            rand_width,
            rand_height,  m_lumaFilter[rand_val]
            );
        ref(8, pixel_buff + 3 * rand_srcStride,
            rand_srcStride,
            IPF_C_output_p,
            rand_dstStride,
            rand_width,
            rand_height,  m_lumaFilter[rand_val]
            );

        if (memcmp(IPF_vec_output_p, IPF_C_output_p, ipf_t_size))
            return false;
    }

    return true;
}

bool IPFilterHarness::check_IPFilter_primitive(x265::IPFilter_p_s ref, x265::IPFilter_p_s opt)
{
    int rand_height = rand() % 100;                 // Randomly generated Height
    int rand_width = rand() % 100;                  // Randomly generated Width
    short rand_val, rand_srcStride, rand_dstStride;

    for (int i = 0; i <= 100; i++)
    {
        memset(IPF_vec_output_s, 0, ipf_t_size);      // Initialize output buffer to zero
        memset(IPF_C_output_s, 0, ipf_t_size);        // Initialize output buffer to zero

        rand_val = rand() % 4;                     // Random offset in the filter
        rand_srcStride = rand() % 100;              // Randomly generated srcStride
        rand_dstStride = rand() % 100;              // Randomly generated dstStride

        opt(8, pixel_buff + 3 * rand_srcStride,
            rand_srcStride,
            IPF_vec_output_s,
            rand_dstStride,
            rand_width,
            rand_height,  m_lumaFilter[rand_val]
            );
        ref(8, pixel_buff + 3 * rand_srcStride,
            rand_srcStride,
            IPF_C_output_s,
            rand_dstStride,
            rand_width,
            rand_height,  m_lumaFilter[rand_val]
            );

        if (memcmp(IPF_vec_output_s, IPF_C_output_s, ipf_t_size))
            return false;
    }

    return true;
}

bool IPFilterHarness::check_IPFilter_primitive(x265::IPFilter_s_p ref, x265::IPFilter_s_p opt)
{
    int rand_height = rand() % 100;                 // Randomly generated Height
    int rand_width = rand() % 100;                  // Randomly generated Width
    short rand_val, rand_srcStride, rand_dstStride;

    for (int i = 0; i <= 100; i++)
    {
        memset(IPF_vec_output_p, 0, ipf_t_size);      // Initialize output buffer to zero
        memset(IPF_C_output_p, 0, ipf_t_size);        // Initialize output buffer to zero

        rand_val = rand() % 4;                     // Random offset in the filter
        rand_srcStride = rand() % 100;              // Randomly generated srcStride
        rand_dstStride = rand() % 100;              // Randomly generated dstStride

        opt(8, short_buff + 3 * rand_srcStride,
            rand_srcStride,
            IPF_vec_output_p,
            rand_dstStride,
            rand_width,
            rand_height,  m_lumaFilter[rand_val]
            );
        ref(8, short_buff + 3 * rand_srcStride,
            rand_srcStride,
            IPF_C_output_p,
            rand_dstStride,
            rand_width,
            rand_height,  m_lumaFilter[rand_val]
            );

        if (memcmp(IPF_vec_output_p, IPF_C_output_p, ipf_t_size))
            return false;
    }

    return true;
}

bool IPFilterHarness::check_IPFilter_primitive(x265::IPFilterConvert_p_s ref, x265::IPFilterConvert_p_s opt)
{
    int rand_height = rand() % 100;                 // Randomly generated Height
    int rand_width = rand() % 100;                  // Randomly generated Width
    short rand_srcStride, rand_dstStride;

    for (int i = 0; i <= 100; i++)
    {
        memset(IPF_vec_output_p, 0, ipf_t_size);      // Initialize output buffer to zero
        memset(IPF_C_output_p, 0, ipf_t_size);        // Initialize output buffer to zero
                             
        rand_srcStride = rand() % 100;              // Randomly generated srcStride
        rand_dstStride = rand() % 100;              // Randomly generated dstStride

        opt(8, pixel_buff,
            rand_srcStride,
            IPF_vec_output_s,
            rand_dstStride,
            rand_width,
            rand_height);
        ref(8, pixel_buff,
            rand_srcStride,
            IPF_C_output_s,
            rand_dstStride,
            rand_width,
            rand_height);

        if (memcmp(IPF_vec_output_p, IPF_C_output_p, ipf_t_size))
            return false;
    }

    return true;
}

bool IPFilterHarness::check_IPFilter_primitive(x265::IPFilterConvert_s_p ref, x265::IPFilterConvert_s_p opt)
{
    int rand_height = rand() % 100;                 // Randomly generated Height
    int rand_width = rand() % 100;                  // Randomly generated Width
    short rand_srcStride, rand_dstStride;

    for (int i = 0; i <= 100; i++)
    {
        memset(IPF_vec_output_p, 0, ipf_t_size);      // Initialize output buffer to zero
        memset(IPF_C_output_p, 0, ipf_t_size);        // Initialize output buffer to zero

        rand_srcStride = rand() % 100;              // Randomly generated srcStride
        rand_dstStride = rand() % 100;              // Randomly generated dstStride

        opt(8, short_buff,
            rand_srcStride,
            IPF_vec_output_p,
            rand_dstStride,
            rand_width,
            rand_height);
        ref(8, short_buff,
            rand_srcStride,
            IPF_C_output_p,
            rand_dstStride,
            rand_width,
            rand_height);

        if (memcmp(IPF_vec_output_p, IPF_C_output_p, ipf_t_size))
            return false;
    }

    return true;
}

bool IPFilterHarness::testCorrectness(const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{
    for (int value = 0; value < NUM_IPFILTER_P_P; value++)
    {
        if (opt.ipFilter_p_p[value])
        {
            if (!check_IPFilter_primitive(ref.ipFilter_p_p[value], opt.ipFilter_p_p[value]))
            {
                printf("\nfilter_H_P_P_%d failed\n", 8 / (value + 1));
                return false;
            }
        }
    }

    for (int value = 0; value < NUM_IPFILTER_P_S; value++)
    {
        if (opt.ipFilter_p_s[value])
        {
            if (!check_IPFilter_primitive(ref.ipFilter_p_s[value], opt.ipFilter_p_s[value]))
            {
                printf("\nfilter_H_P_S_%d failed\n", 8 / (value + 1));
                return false;
            }
        }
    }

    for (int value = 0; value < NUM_IPFILTER_S_P; value++)
    {
        if (opt.ipFilter_s_p[value])
        {
            if (!check_IPFilter_primitive(ref.ipFilter_s_p[value], opt.ipFilter_s_p[value]))
            {
                printf("\nfilter_H_S_P_%d failed\n", 8 / (value + 1));
                return false;
            }
        }
    }

    if (opt.ipfilterConvert_p_s)
    {
        if (!check_IPFilter_primitive(ref.ipfilterConvert_p_s, opt.ipfilterConvert_p_s))
        {
            printf("\nfilterConvertPeltoShort failed\n");
            return false;
        }
    }

    if (opt.ipfilterConvert_s_p)
    {
        if (!check_IPFilter_primitive(ref.ipfilterConvert_s_p, opt.ipfilterConvert_s_p))
        {
            printf("\nfilterConvertShorttoPel failed\n");
            return false;
        }
    }

    return true;
}

#define FILTER_ITERATIONS   50000

void IPFilterHarness::measureSpeed(const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{
    Timer *t = Timer::CreateTimer();

    int height = 64;
    int width = 64;
    short val = 2;
    short srcStride = 96;
    short dstStride = 96;

    //for (int value = 0; value < 16; value++)
    //{
    //    memset(IPF_vec_output, 0, ipf_t_size);  // Initialize output buffer to zero
    //    memset(IPF_C_output, 0, ipf_t_size);    // Initialize output buffer to zero
    //    if (opt.filter[value])
    //    {
    //        printf("filter[%s]", FilterConf_names[value]);
    //        REPORT_SPEEDUP(FILTER_ITERATIONS,
    //                       opt.filter[value]((short*)(m_lumaFilter + val), pixel_buff + 3 * srcStride, srcStride,
    //                                         IPF_vec_output, dstStride, height, width, BIT_DEPTH),
    //                       ref.filter[value]((short*)(m_lumaFilter + val), pixel_buff + 3 * srcStride, srcStride,
    //                                         IPF_vec_output, dstStride, height, width, BIT_DEPTH));
    //    }
    //}

    for (int value = 0; value < NUM_IPFILTER_P_P; value++)
    {
        if (opt.ipFilter_p_p[value])
        {
            printf("filter_H_P_P_%d", 8 / (value + 1));
            REPORT_SPEEDUP(FILTER_ITERATIONS,
                           opt.ipFilter_p_p[value](8, pixel_buff + 3 * srcStride,
                                                   srcStride,
                                                   IPF_vec_output_p,
                                                   dstStride,
                                                   width,
                                                   height,  m_lumaFilter[val]),
                           ref.ipFilter_p_p[value](8, pixel_buff + 3 * srcStride,
                                                   srcStride,
                                                   IPF_C_output_p,
                                                   dstStride,
                                                   width,
                                                   height,  m_lumaFilter[val]
                                                   )
                           );
        }
    }

    for (int value = 0; value < NUM_IPFILTER_P_S; value++)
    {
        if (opt.ipFilter_p_s[value])
        {
            printf("filter_H_P_S_%d", 8 / (value + 1));
            REPORT_SPEEDUP(FILTER_ITERATIONS,
                           opt.ipFilter_p_s[value](8, pixel_buff + 3 * srcStride,
                                                   srcStride,
                                                   IPF_vec_output_s,
                                                   dstStride,
                                                   width,
                                                   height,  m_lumaFilter[val]),
                           ref.ipFilter_p_s[value](8, pixel_buff + 3 * srcStride,
                                                   srcStride,
                                                   IPF_C_output_s,
                                                   dstStride,
                                                   width,
                                                   height,  m_lumaFilter[val]
                                                   )
                           );
        }
    }

    for (int value = 0; value < NUM_IPFILTER_S_P; value++)
    {
        if (opt.ipFilter_s_p[value])
        {
            printf("filter_H_S_P_%d", 8 / (value + 1));
            REPORT_SPEEDUP(FILTER_ITERATIONS,
                           opt.ipFilter_s_p[value](8, short_buff + 3 * srcStride,
                                                   srcStride,
                                                   IPF_vec_output_p,
                                                   dstStride,
                                                   width,
                                                   height,  m_lumaFilter[val]),
                           ref.ipFilter_s_p[value](8, short_buff + 3 * srcStride,
                                                   srcStride,
                                                   IPF_C_output_p,
                                                   dstStride,
                                                   width,
                                                   height,  m_lumaFilter[val]
                                                   )
                           );
        }
    }

    if (opt.ipfilterConvert_p_s)
    {
        printf("filterConvertPeltoShort");
        REPORT_SPEEDUP(FILTER_ITERATIONS,
                       opt.ipfilterConvert_p_s(8, pixel_buff,
                                               srcStride,
                                               IPF_vec_output_s,
                                               dstStride,
                                               width,
                                               height),
                       ref.ipfilterConvert_p_s(8, pixel_buff,
                                               srcStride,
                                               IPF_C_output_s,
                                               dstStride,
                                               width,
                                               height)
                       );
    }

    if (opt.ipfilterConvert_s_p)
    {
        printf("filterConvertShorttoPel");
        REPORT_SPEEDUP(FILTER_ITERATIONS,
                       opt.ipfilterConvert_s_p(8, short_buff,
                                               srcStride,
                                               IPF_vec_output_p,
                                               dstStride,
                                               width,
                                               height),
                       ref.ipfilterConvert_s_p(8, short_buff,
                                               srcStride,
                                               IPF_C_output_p,
                                               dstStride,
                                               width,
                                               height)
                       );
    }

    t->Release();
}
