/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Deepthi Devaki <deepthidevaki@multicorewareinc.com>,
 *          Rajesh Paulraj <rajesh@multicorewareinc.com>
 *          Praveen Kumar Tiwari <praveen@multicorewareinc.com>
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

#include "TLibCommon/TComRom.h"
#include "ipfilterharness.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

using namespace x265;

const char* IPFilterPPNames[] =
{
    "ipfilterH_pp<8>",
    "ipfilterH_pp<4>",
    "ipfilterV_pp<8>",
    "ipfilterV_pp<4>"
};

IPFilterHarness::IPFilterHarness()
{
    ipf_t_size = 200 * 200;
    pixel_buff = (pixel*)malloc(ipf_t_size * sizeof(pixel));     // Assuming max_height = max_width = max_srcStride = max_dstStride = 100
    short_buff = (short*)X265_MALLOC(short, ipf_t_size);
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
        pixel_buff[i] = (pixel)(rand() &  ((1 << 8) - 1));
        short_buff[i] = (short)(isPositive) * (rand() &  SHRT_MAX);
    }
}

IPFilterHarness::~IPFilterHarness()
{
    free(IPF_vec_output_s);
    free(IPF_C_output_s);
    free(IPF_vec_output_p);
    free(IPF_C_output_p);
    X265_FREE(short_buff);
    free(pixel_buff);
}

bool IPFilterHarness::check_IPFilter_primitive(ipfilter_pp_t ref, ipfilter_pp_t opt)
{
    int rand_height = rand() % 100;                 // Randomly generated Height
    int rand_width = rand() % 100;                  // Randomly generated Width
    int rand_val, rand_srcStride, rand_dstStride;

    if (rand_height % 2)
        rand_height++;

    for (int i = 0; i <= 100; i++)
    {
        memset(IPF_vec_output_p, 0, ipf_t_size);      // Initialize output buffer to zero
        memset(IPF_C_output_p, 0, ipf_t_size);        // Initialize output buffer to zero

        rand_val = rand() % 4;                     // Random offset in the filter
        rand_srcStride = rand() % 100;              // Randomly generated srcStride
        rand_dstStride = rand() % 100;              // Randomly generated dstStride

        if (rand_srcStride < rand_width)
            rand_srcStride = rand_width;

        if (rand_dstStride < rand_width)
            rand_dstStride = rand_width;

        opt(pixel_buff + 3 * rand_srcStride,
            rand_srcStride,
            IPF_vec_output_p,
            rand_dstStride,
            rand_width,
            rand_height, g_lumaFilter[rand_val]
            );
        ref(pixel_buff + 3 * rand_srcStride,
            rand_srcStride,
            IPF_C_output_p,
            rand_dstStride,
            rand_width,
            rand_height, g_lumaFilter[rand_val]
            );

        if (memcmp(IPF_vec_output_p, IPF_C_output_p, ipf_t_size))
            return false;
    }

    return true;
}

bool IPFilterHarness::check_IPFilter_primitive(ipfilter_ps_t ref, ipfilter_ps_t opt)
{
    int rand_height = rand() % 100;                 // Randomly generated Height
    int rand_width = rand() % 100;                  // Randomly generated Width
    short rand_val, rand_srcStride, rand_dstStride;

    for (int i = 0; i <= 100; i++)
    {
        memset(IPF_vec_output_s, 0, ipf_t_size);      // Initialize output buffer to zero
        memset(IPF_C_output_s, 0, ipf_t_size);        // Initialize output buffer to zero

        rand_val = rand() % 4;                      // Random offset in the filter
        rand_srcStride = rand() % 100;              // Randomly generated srcStride
        rand_dstStride = rand() % 100;              // Randomly generated dstStride

        opt(pixel_buff + 3 * rand_srcStride,
            rand_srcStride,
            IPF_vec_output_s,
            rand_dstStride,
            rand_width,
            rand_height, g_lumaFilter[rand_val]
            );
        ref(pixel_buff + 3 * rand_srcStride,
            rand_srcStride,
            IPF_C_output_s,
            rand_dstStride,
            rand_width,
            rand_height, g_lumaFilter[rand_val]
            );

        if (memcmp(IPF_vec_output_s, IPF_C_output_s, ipf_t_size))
            return false;
    }

    return true;
}

bool IPFilterHarness::check_IPFilter_primitive(ipfilter_sp_t ref, ipfilter_sp_t opt)
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

        opt(short_buff + 3 * rand_srcStride,
            rand_srcStride,
            IPF_vec_output_p,
            rand_dstStride,
            rand_width,
            rand_height, g_lumaFilter[rand_val]
            );
        ref(short_buff + 3 * rand_srcStride,
            rand_srcStride,
            IPF_C_output_p,
            rand_dstStride,
            rand_width,
            rand_height, g_lumaFilter[rand_val]
            );

        if (memcmp(IPF_vec_output_p, IPF_C_output_p, ipf_t_size))
            return false;
    }

    return true;
}

bool IPFilterHarness::check_IPFilter_primitive(ipfilter_p2s_t ref, ipfilter_p2s_t opt)
{
    short rand_height = (short)rand() % 100;                 // Randomly generated Height
    short rand_width = (short)rand() % 100;                  // Randomly generated Width
    short rand_srcStride, rand_dstStride;

    for (int i = 0; i <= 100; i++)
    {
        memset(IPF_vec_output_s, 0, ipf_t_size);      // Initialize output buffer to zero
        memset(IPF_C_output_s, 0, ipf_t_size);        // Initialize output buffer to zero

        rand_srcStride = rand_width + rand() % 100;              // Randomly generated srcStride
        rand_dstStride = rand_width + rand() % 100;              // Randomly generated dstStride

        opt(pixel_buff,
            rand_srcStride,
            IPF_vec_output_s,
            rand_dstStride,
            rand_width,
            rand_height);
        ref(pixel_buff,
            rand_srcStride,
            IPF_C_output_s,
            rand_dstStride,
            rand_width,
            rand_height);

        if (memcmp(IPF_vec_output_s, IPF_C_output_s, ipf_t_size))
            return false;
    }

    return true;
}

bool IPFilterHarness::check_IPFilter_primitive(ipfilter_s2p_t ref, ipfilter_s2p_t opt)
{
    short rand_height = (short)rand() % 100;                 // Randomly generated Height
    short rand_width = (short)rand() % 100;                  // Randomly generated Width
    short rand_srcStride, rand_dstStride;

    for (int i = 0; i <= 100; i++)
    {
        memset(IPF_vec_output_p, 0, ipf_t_size);      // Initialize output buffer to zero
        memset(IPF_C_output_p, 0, ipf_t_size);        // Initialize output buffer to zero

        rand_srcStride = rand_width + rand() % 100;              // Randomly generated srcStride
        rand_dstStride = rand_width + rand() % 100;              // Randomly generated dstStride

        opt(short_buff,
            rand_srcStride,
            IPF_vec_output_p,
            rand_dstStride,
            rand_width,
            rand_height);
        ref(short_buff,
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

bool IPFilterHarness::check_IPFilterChroma_primitive(filter_pp_t ref, filter_pp_t opt)
{
    int rand_srcStride, rand_dstStride, rand_coeffIdx;

    for (int i = 0; i <= 100; i++)
    {
        rand_coeffIdx = rand() % 8;                // Random coeffIdex in the filter

        rand_srcStride = rand() % 100;              // Randomly generated srcStride
        rand_dstStride = rand() % 100;              // Randomly generated dstStride

       // maxVerticalfilterHalfDistance = 3

        opt(pixel_buff + 3 * rand_srcStride,
            rand_srcStride,
            IPF_vec_output_p,
            rand_dstStride,
            rand_coeffIdx
            );
        ref(pixel_buff + 3 * rand_srcStride,
            rand_srcStride,
            IPF_C_output_p,
            rand_dstStride,
           rand_coeffIdx
            );

        if (memcmp(IPF_vec_output_p, IPF_C_output_p, ipf_t_size))
            return false;
    }

    return true;
}

bool IPFilterHarness::check_IPFilterLuma_primitive(filter_pp_t ref, filter_pp_t opt)
{
    int rand_srcStride, rand_dstStride, rand_coeffIdx;

    for (int i = 0; i <= 100; i++)
    {
        rand_coeffIdx = rand() % 3;                // Random coeffIdex in the filter

        rand_srcStride = rand() % 100;             // Randomly generated srcStride
        rand_dstStride = rand() % 100;             // Randomly generated dstStride

        opt(pixel_buff + 3 * rand_srcStride,
            rand_srcStride,
            IPF_vec_output_p,
            rand_dstStride,
            rand_coeffIdx);
        ref(pixel_buff + 3 * rand_srcStride,
            rand_srcStride,
            IPF_C_output_p,
            rand_dstStride,
            rand_coeffIdx);

        if (memcmp(IPF_vec_output_p, IPF_C_output_p, ipf_t_size))
            return false;
    }

    return true;
}

bool IPFilterHarness::testCorrectness(const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{
    for (int value = 0; value < NUM_IPFILTER_P_P; value++)
    {
        if (opt.ipfilter_pp[value])
        {
            if (!check_IPFilter_primitive(ref.ipfilter_pp[value], opt.ipfilter_pp[value]))
            {
                printf("%s failed\n", IPFilterPPNames[value]);
                return false;
            }
        }
    }

    for (int value = 0; value < NUM_IPFILTER_P_S; value++)
    {
        if (opt.ipfilter_ps[value])
        {
            if (!check_IPFilter_primitive(ref.ipfilter_ps[value], opt.ipfilter_ps[value]))
            {
                printf("ipfilter_ps %d failed\n", 8 / (value + 1));
                return false;
            }
        }
    }

    for (int value = 0; value < NUM_IPFILTER_S_P; value++)
    {
        if (opt.ipfilter_sp[value])
        {
            if (!check_IPFilter_primitive(ref.ipfilter_sp[value], opt.ipfilter_sp[value]))
            {
                printf("ipfilter_sp %d failed\n", 8 / (value + 1));
                return false;
            }
        }
    }

    if (opt.ipfilter_p2s)
    {
        if (!check_IPFilter_primitive(ref.ipfilter_p2s, opt.ipfilter_p2s))
        {
            printf("ipfilter_p2s failed\n");
            return false;
        }
    }

    if (opt.ipfilter_s2p)
    {
        if (!check_IPFilter_primitive(ref.ipfilter_s2p, opt.ipfilter_s2p))
        {
            printf("\nfilterConvertShorttoPel failed\n");
            return false;
        }
    }

    for (int value = 0; value < NUM_LUMA_PARTITIONS; value++)
    {
        if (opt.luma_hpp[value])
        {
            if (!check_IPFilterLuma_primitive(ref.luma_hpp[value], opt.luma_hpp[value]))
            {
                printf("luma_hpp[%s]", lumaPartStr[value]);
                return false;
            }
        }
    }

    for (int value = 0; value < NUM_CHROMA_PARTITIONS; value++)
    {
        if (opt.chroma_hpp[value])
        {
            if (!check_IPFilterChroma_primitive(ref.chroma_hpp[value], opt.chroma_hpp[value]))
            {
                printf("chroma_hpp[%s]", chromaPartStr[value]);
                return false;
            }
        }
        if (opt.chroma_vpp[value])
        {
            if (!check_IPFilterChroma_primitive(ref.chroma_vpp[value], opt.chroma_vpp[value]))
            {
                printf("chroma_vpp[%s]", chromaPartStr[value]);
                return false;
            }
        }
    }

    return true;
}

void IPFilterHarness::measureSpeed(const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{
    int height = 64;
    int width = 64;
    short val = 2;
    short srcStride = 96;
    short dstStride = 96;
    int maxVerticalfilterHalfDistance = 3;

    for (int value = 0; value < NUM_IPFILTER_P_P; value++)
    {
        if (opt.ipfilter_pp[value])
        {
            printf("%s\t", IPFilterPPNames[value]);
            REPORT_SPEEDUP(opt.ipfilter_pp[value], ref.ipfilter_pp[value],
                           pixel_buff + maxVerticalfilterHalfDistance * srcStride, srcStride, IPF_vec_output_p, dstStride, width, height, g_lumaFilter[val]);
        }
    }

    for (int value = 0; value < NUM_IPFILTER_P_S; value++)
    {
        if (opt.ipfilter_ps[value])
        {
            printf("ipfilter_ps %d\t", 8 / (value + 1));
            REPORT_SPEEDUP(opt.ipfilter_ps[value], ref.ipfilter_ps[value],
                           pixel_buff + maxVerticalfilterHalfDistance * srcStride, srcStride, IPF_vec_output_s, dstStride, width, height, g_lumaFilter[val]);
        }
    }

    for (int value = 0; value < NUM_IPFILTER_S_P; value++)
    {
        if (opt.ipfilter_sp[value])
        {
            printf("ipfilter_sp %d\t", 8 / (value + 1));
            REPORT_SPEEDUP(opt.ipfilter_sp[value], ref.ipfilter_sp[value],
                           short_buff + maxVerticalfilterHalfDistance * srcStride, srcStride,
                           IPF_vec_output_p, dstStride, width, height, g_lumaFilter[val]);
        }
    }

    if (opt.ipfilter_p2s)
    {
        printf("ipfilter_p2s\t");
        REPORT_SPEEDUP(opt.ipfilter_p2s, ref.ipfilter_p2s,
                       pixel_buff, srcStride, IPF_vec_output_s, dstStride, width, height);
    }

    if (opt.ipfilter_s2p)
    {
        printf("ipfilter_s2p\t");
        REPORT_SPEEDUP(opt.ipfilter_s2p, ref.ipfilter_s2p,
                       short_buff, srcStride, IPF_vec_output_p, dstStride, width, height);
    }

    for (int value = 0; value < NUM_LUMA_PARTITIONS; value++)
    {
        if (opt.luma_hpp[value])
        {
            printf("  luma_hpp[%s]", lumaPartStr[value]);
            REPORT_SPEEDUP(opt.luma_hpp[value], ref.luma_hpp[value],
                           pixel_buff + srcStride, srcStride, IPF_vec_output_p, dstStride, 1);
        }
    }

    for (int value = 0; value < NUM_CHROMA_PARTITIONS; value++)
    {
        if (opt.chroma_hpp[value])
        {
            printf("chroma_hpp[%s]", chromaPartStr[value]);
            REPORT_SPEEDUP(opt.chroma_hpp[value], ref.chroma_hpp[value],
                           pixel_buff + srcStride, srcStride, IPF_vec_output_p, dstStride, 1);
        }
        if (opt.chroma_vpp[value])
        {
            printf("chroma_vpp[%s]", chromaPartStr[value]);
            REPORT_SPEEDUP(opt.chroma_vpp[value], ref.chroma_vpp[value],
                           pixel_buff + maxVerticalfilterHalfDistance * srcStride, srcStride,
                           IPF_vec_output_p, dstStride, 1);
        }
    }
}
