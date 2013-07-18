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
    short_buff = (short*)TestHarness::alignedMalloc(sizeof(short), ipf_t_size, 32);
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
    TestHarness::alignedFree(short_buff);
    free(pixel_buff);
}

bool IPFilterHarness::check_IPFilter_primitive(x265::ipfilter_pp_t ref, x265::ipfilter_pp_t opt)
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

bool IPFilterHarness::check_IPFilter_primitive(x265::ipfilter_ps_t ref, x265::ipfilter_ps_t opt)
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

bool IPFilterHarness::check_IPFilter_primitive(x265::ipfilter_sp_t ref, x265::ipfilter_sp_t opt)
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

bool IPFilterHarness::check_IPFilter_primitive(x265::ipfilter_p2s_t ref, x265::ipfilter_p2s_t opt)
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

bool IPFilterHarness::check_IPFilter_primitive(x265::ipfilter_s2p_t ref, x265::ipfilter_s2p_t opt)
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

bool IPFilterHarness::check_filterVMultiplane(x265::filterVmulti_t ref, x265::filterVmulti_t opt)
{
    short rand_height = 32;                 // Can be randomly generated Height
    short rand_width = 32;                  // Can be randomly generated Width
    int marginX = 64;
    int marginY = 64;
    short rand_srcStride, rand_dstStride;

    pixel dstEvec[200 * 200];
    pixel dstIvec[200 * 200];
    pixel dstPvec[200 * 200];

    pixel dstEref[200 * 200];
    pixel dstIref[200 * 200];
    pixel dstPref[200 * 200];

    memset(dstEref, 0, 40000 * sizeof(pixel));
    memset(dstIref, 0, 40000 * sizeof(pixel));
    memset(dstPref, 0, 40000 * sizeof(pixel));

    memset(dstEvec, 0, 40000 * sizeof(pixel));
    memset(dstIvec, 0, 40000 * sizeof(pixel));
    memset(dstPvec, 0, 40000 * sizeof(pixel));
    for (int i = 0; i <= 100; i++)
    {
        rand_srcStride = 200;               // Can be randomly generated
        rand_dstStride = 200;

        opt(short_buff + 8 * rand_srcStride, rand_srcStride,
            dstEvec + marginY * rand_dstStride + marginX,
            dstIvec + marginY * rand_dstStride + marginX,
            dstPvec + marginY * rand_dstStride + marginX, rand_dstStride,
            rand_width, rand_height, marginX, marginY);
        ref(short_buff + 8 * rand_srcStride, rand_srcStride,
            dstEref + marginY * rand_dstStride + marginX,
            dstIref + marginY * rand_dstStride + marginX,
            dstPref + marginY * rand_dstStride + marginX, rand_dstStride,
            rand_width, rand_height, marginX, marginY);

        if (memcmp(dstEvec, dstEref, 200 * 200 * sizeof(pixel)) ||
            memcmp(dstIvec, dstIref, 200 * 200 * sizeof(pixel)) ||
            memcmp(dstPvec, dstPref, 200 * 200 * sizeof(pixel)))
        {
            return false;
        }
    }

    return true;
}

bool IPFilterHarness::check_filterHMultiplane(x265::filterHmulti_t ref, x265::filterHmulti_t opt)
{
    short rand_height;
    short rand_width;
    int rand_srcStride, rand_dstStride;
    int marginX, marginY;

    short dstAvec[100 * 100];
    short dstEvec[100 * 100];
    short dstIvec[100 * 100];
    short dstPvec[100 * 100];
    short dstAref[100 * 100];
    short dstEref[100 * 100];
    short dstIref[100 * 100];
    short dstPref[100 * 100];
    pixel pDstAvec[200 * 200];
    pixel pDstAref[200 * 200];
    pixel pDstBvec[200 * 200];
    pixel pDstBref[200 * 200];
    pixel pDstCvec[200 * 200];
    pixel pDstCref[200 * 200];

    memset(dstAref, 0, 10000 * sizeof(short));
    memset(dstEref, 0, 10000 * sizeof(short));
    memset(dstIref, 0, 10000 * sizeof(short));
    memset(dstPref, 0, 10000 * sizeof(short));
    memset(dstAvec, 0, 10000 * sizeof(short));
    memset(dstEvec, 0, 10000 * sizeof(short));
    memset(dstIvec, 0, 10000 * sizeof(short));
    memset(dstPvec, 0, 10000 * sizeof(short));
    memset(pDstAvec, 0, 40000 * sizeof(pixel));
    memset(pDstAref, 0, 40000 * sizeof(pixel));
    memset(pDstBvec, 0, 40000 * sizeof(pixel));
    memset(pDstBref, 0, 40000 * sizeof(pixel));
    memset(pDstCvec, 0, 40000 * sizeof(pixel));
    memset(pDstCref, 0, 40000 * sizeof(pixel));

    for (int i = 0; i <= 100; i++)
    {
        rand_height = (rand() % 32) + 1;
        rand_width = (rand() % 32) + 8;
        marginX = (rand() % 16) + 16;
        marginY = (rand() % 16) + 16;
        rand_srcStride = rand_width;               // Can be randomly generated
        rand_dstStride = rand_width + 2 * marginX;
        opt(pixel_buff + 8 * rand_srcStride, rand_srcStride,
            dstAvec, dstEvec, dstIvec, dstPvec, rand_dstStride,
            pDstAvec + marginY * rand_dstStride + marginX,
            pDstBvec + marginY * rand_dstStride + marginX,
            pDstCvec + marginY * rand_dstStride + marginX, rand_dstStride,
            rand_width, rand_height, marginX, marginY);
        ref(pixel_buff + 8 * rand_srcStride, rand_srcStride,
            dstAref, dstEref, dstIref, dstPref, rand_dstStride,
            pDstAref + marginY * rand_dstStride + marginX,
            pDstBref + marginY * rand_dstStride + marginX,
            pDstCref + marginY * rand_dstStride + marginX, rand_dstStride,
            rand_width, rand_height, marginX, marginY);

        if (memcmp(dstAvec, dstAref, 100 * 100 * sizeof(short)) || memcmp(dstEvec, dstEref, 100 * 100 * sizeof(short)) ||
            memcmp(dstIvec, dstIref, 100 * 100 * sizeof(short)) || memcmp(dstPvec, dstPref, 100 * 100 * sizeof(short)) ||
            memcmp(pDstAvec, pDstAref, 200 * 200 * sizeof(pixel)) || memcmp(pDstBvec, pDstBref, 200 * 200 * sizeof(pixel)) ||
            memcmp(pDstCvec, pDstCref, 200 * 200 * sizeof(pixel)))
        {
            return false;
        }
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

    if (opt.filterVmulti)
    {
        if (!check_filterVMultiplane(ref.filterVmulti, opt.filterVmulti))
        {
            printf("Filter-V-multiplane failed\n");
            return false;
        }
    }

    if (opt.filterHmulti)
    {
        if (!check_filterHMultiplane(ref.filterHmulti, opt.filterHmulti))
        {
            printf("Filter-H-multiplane failed\n");
            return false;
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

    for (int value = 0; value < NUM_IPFILTER_P_P; value++)
    {
        if (opt.ipfilter_pp[value])
        {
            printf("%s\t", IPFilterPPNames[value]);
            REPORT_SPEEDUP(opt.ipfilter_pp[value], ref.ipfilter_pp[value],
                           pixel_buff + 3 * srcStride, srcStride, IPF_vec_output_p, dstStride, width, height, g_lumaFilter[val]);
        }
    }

    for (int value = 0; value < NUM_IPFILTER_P_S; value++)
    {
        if (opt.ipfilter_ps[value])
        {
            printf("ipfilter_ps %d\t", 8 / (value + 1));
            REPORT_SPEEDUP(opt.ipfilter_ps[value], ref.ipfilter_ps[value],
                           pixel_buff + 3 * srcStride, srcStride, IPF_vec_output_s, dstStride, width, height, g_lumaFilter[val]);
        }
    }

    for (int value = 0; value < NUM_IPFILTER_S_P; value++)
    {
        if (opt.ipfilter_sp[value])
        {
            printf("ipfilter_sp %d\t", 8 / (value + 1));
            REPORT_SPEEDUP(opt.ipfilter_sp[value], ref.ipfilter_sp[value],
                           short_buff + 3 * srcStride, srcStride, IPF_vec_output_p, dstStride, width, height, g_lumaFilter[val]);
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

    if (opt.filterVmulti)
    {
        printf("Filter-V-multiplane");
        REPORT_SPEEDUP(opt.filterVmulti, ref.filterVmulti,
                       short_buff + 8 * srcStride, srcStride, IPF_C_output_p + 64 * 200 + 64, IPF_vec_output_p + 64 * 200 + 64, IPF_C_output_p + 64 * 200 + 64, dstStride, width, height, 64, 64);
    }

    if (opt.filterHmulti)
    {
        printf("Filter-H-multiplane");
        REPORT_SPEEDUP(opt.filterHmulti, ref.filterHmulti,
                       pixel_buff + 8 * srcStride, srcStride, IPF_vec_output_s, IPF_C_output_s, IPF_vec_output_s, IPF_C_output_s, dstStride, IPF_vec_output_p + 64 * 200 + 64, IPF_C_output_p + 64 * 200 + 64, IPF_vec_output_p + 64 * 200 + 64, dstStride, width, height, 64, 64);
    }
}
