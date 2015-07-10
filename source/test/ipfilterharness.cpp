/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Deepthi Devaki <deepthidevaki@multicorewareinc.com>,
 *          Rajesh Paulraj <rajesh@multicorewareinc.com>
 *          Praveen Kumar Tiwari <praveen@multicorewareinc.com>
 *          Min Chen <chenm003@163.com> <min.chen@multicorewareinc.com>
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
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#include "common.h"
#include "ipfilterharness.h"

using namespace X265_NS;

IPFilterHarness::IPFilterHarness()
{
    /* [0] --- Random values
     * [1] --- Minimum
     * [2] --- Maximum */
    for (int i = 0; i < TEST_BUF_SIZE; i++)
    {
        pixel_test_buff[0][i] = rand() & PIXEL_MAX;
        short_test_buff[0][i] = (rand() % (2 * SMAX)) - SMAX;

        pixel_test_buff[1][i] = PIXEL_MIN;
        short_test_buff[1][i] = SMIN;

        pixel_test_buff[2][i] = PIXEL_MAX;
        short_test_buff[2][i] = SMAX;
    }

    memset(IPF_C_output_p, 0xCD, TEST_BUF_SIZE * sizeof(pixel));
    memset(IPF_vec_output_p, 0xCD, TEST_BUF_SIZE * sizeof(pixel));
    memset(IPF_C_output_s, 0xCD, TEST_BUF_SIZE * sizeof(int16_t));
    memset(IPF_vec_output_s, 0xCD, TEST_BUF_SIZE * sizeof(int16_t));

    int pixelMax = (1 << X265_DEPTH) - 1;
    int shortMax = (1 << 15) - 1;
    for (int i = 0; i < TEST_BUF_SIZE; i++)
    {
        pixel_buff[i] = (pixel)(rand() & pixelMax);
        int isPositive = (rand() & 1) ? 1 : -1;
        short_buff[i] = (int16_t)(isPositive * (rand() & shortMax));
    }
}

bool IPFilterHarness::check_IPFilterChroma_primitive(filter_pp_t ref, filter_pp_t opt)
{
    intptr_t rand_srcStride, rand_dstStride;

    for (int i = 0; i < ITERS; i++)
    {
        int index = i % TEST_CASES;

        for (int coeffIdx = 0; coeffIdx < 8; coeffIdx++)
        {
            rand_srcStride = rand() % 100 + 2;
            rand_dstStride = rand() % 100 + 64;

            checked(opt, pixel_test_buff[index] + 3 * rand_srcStride,
                    rand_srcStride,
                    IPF_vec_output_p,
                    rand_dstStride,
                    coeffIdx);

            ref(pixel_test_buff[index] + 3 * rand_srcStride,
                rand_srcStride,
                IPF_C_output_p,
                rand_dstStride,
                coeffIdx);

            if (memcmp(IPF_vec_output_p, IPF_C_output_p, TEST_BUF_SIZE * sizeof(pixel)))
                return false;

            reportfail();
        }
    }

    return true;
}

bool IPFilterHarness::check_IPFilterChroma_ps_primitive(filter_ps_t ref, filter_ps_t opt)
{
    intptr_t rand_srcStride, rand_dstStride;

    for (int i = 0; i < ITERS; i++)
    {
        int index = i % TEST_CASES;

        for (int coeffIdx = 0; coeffIdx < 8; coeffIdx++)
        {
            rand_srcStride = rand() % 100;
            rand_dstStride = rand() % 100 + 64;

            ref(pixel_test_buff[index] + 3 * rand_srcStride,
                rand_srcStride,
                IPF_C_output_s,
                rand_dstStride,
                coeffIdx);

            checked(opt, pixel_test_buff[index] + 3 * rand_srcStride,
                    rand_srcStride,
                    IPF_vec_output_s,
                    rand_dstStride,
                    coeffIdx);

            if (memcmp(IPF_vec_output_s, IPF_C_output_s, TEST_BUF_SIZE * sizeof(int16_t)))
            {
            ref(pixel_test_buff[index] + 3 * rand_srcStride,
                rand_srcStride,
                IPF_C_output_s,
                rand_dstStride,
                coeffIdx);
                return false;
            }

            reportfail();
        }
    }

    return true;
}

bool IPFilterHarness::check_IPFilterChroma_hps_primitive(filter_hps_t ref, filter_hps_t opt)
{
    intptr_t rand_srcStride, rand_dstStride;

    for (int i = 0; i < ITERS; i++)
    {
        int index = i % TEST_CASES;

        for (int coeffIdx = 0; coeffIdx < 8; coeffIdx++)
        {
            // 0 : Interpolate W x H, 1 : Interpolate W x (H + 7)
            for (int isRowExt = 0; isRowExt < 2; isRowExt++)
            {
                rand_srcStride = rand() % 100 + 2;
                rand_dstStride = rand() % 100 + 64;

                ref(pixel_test_buff[index] + 3 * rand_srcStride,
                    rand_srcStride,
                    IPF_C_output_s,
                    rand_dstStride,
                    coeffIdx,
                    isRowExt);

                checked(opt, pixel_test_buff[index] + 3 * rand_srcStride,
                        rand_srcStride,
                        IPF_vec_output_s,
                        rand_dstStride,
                        coeffIdx,
                        isRowExt);

                if (memcmp(IPF_vec_output_s, IPF_C_output_s, TEST_BUF_SIZE * sizeof(int16_t)))
                    return false;

                reportfail();
            }
        }
    }

    return true;
}

bool IPFilterHarness::check_IPFilterChroma_sp_primitive(filter_sp_t ref, filter_sp_t opt)
{
    intptr_t rand_srcStride, rand_dstStride;

    for (int i = 0; i < ITERS; i++)
    {
        int index = i % TEST_CASES;

        for (int coeffIdx = 0; coeffIdx < 8; coeffIdx++)
        {
            rand_srcStride = rand() % 100;
            rand_dstStride = rand() % 100 + 64;

            ref(short_test_buff[index] + 3 * rand_srcStride,
                rand_srcStride,
                IPF_C_output_p,
                rand_dstStride,
                coeffIdx);

            checked(opt, short_test_buff[index] + 3 * rand_srcStride,
                    rand_srcStride,
                    IPF_vec_output_p,
                    rand_dstStride,
                    coeffIdx);

            if (memcmp(IPF_vec_output_p, IPF_C_output_p, TEST_BUF_SIZE * sizeof(pixel)))
                return false;

            reportfail();
        }
    }

    return true;
}

bool IPFilterHarness::check_IPFilterChroma_ss_primitive(filter_ss_t ref, filter_ss_t opt)
{
    intptr_t rand_srcStride, rand_dstStride;

    for (int i = 0; i < ITERS; i++)
    {
        int index = i % TEST_CASES;

        for (int coeffIdx = 0; coeffIdx < 8; coeffIdx++)
        {
            rand_srcStride = rand() % 100;
            rand_dstStride = rand() % 100 + 64;

            ref(short_test_buff[index] + 3 * rand_srcStride,
                rand_srcStride,
                IPF_C_output_s,
                rand_dstStride,
                coeffIdx);

            checked(opt, short_test_buff[index] + 3 * rand_srcStride,
                    rand_srcStride,
                    IPF_vec_output_s,
                    rand_dstStride,
                    coeffIdx);

            if (memcmp(IPF_C_output_s, IPF_vec_output_s, TEST_BUF_SIZE * sizeof(int16_t)))
                return false;

            reportfail();
        }
    }

    return true;
}

bool IPFilterHarness::check_IPFilterLuma_primitive(filter_pp_t ref, filter_pp_t opt)
{
    intptr_t rand_srcStride, rand_dstStride;

    for (int i = 0; i < ITERS; i++)
    {
        int index = i % TEST_CASES;

        for (int coeffIdx = 0; coeffIdx < 4; coeffIdx++)
        {
            rand_srcStride = rand() % 100;
            rand_dstStride = rand() % 100 + 64;

            checked(opt, pixel_test_buff[index] + 3 * rand_srcStride + 6,
                    rand_srcStride,
                    IPF_vec_output_p,
                    rand_dstStride,
                    coeffIdx);

            ref(pixel_test_buff[index] + 3 * rand_srcStride + 6,
                rand_srcStride,
                IPF_C_output_p,
                rand_dstStride,
                coeffIdx);

            if (memcmp(IPF_vec_output_p, IPF_C_output_p, TEST_BUF_SIZE))
                return false;

            reportfail();
        }
    }

    return true;
}

bool IPFilterHarness::check_IPFilterLuma_ps_primitive(filter_ps_t ref, filter_ps_t opt)
{
    intptr_t rand_srcStride, rand_dstStride;

    for (int i = 0; i < ITERS; i++)
    {
        int index = i % TEST_CASES;

        for (int coeffIdx = 0; coeffIdx < 4; coeffIdx++)
        {
            rand_srcStride = rand() % 100;
            rand_dstStride = rand() % 100 + 64;

            ref(pixel_test_buff[index] + 3 * rand_srcStride,
                rand_srcStride,
                IPF_C_output_s,
                rand_dstStride,
                coeffIdx);

            checked(opt, pixel_test_buff[index] + 3 * rand_srcStride,
                    rand_srcStride,
                    IPF_vec_output_s,
                    rand_dstStride,
                    coeffIdx);

            if (memcmp(IPF_vec_output_s, IPF_C_output_s, TEST_BUF_SIZE * sizeof(int16_t)))
                return false;

            reportfail();
        }
    }

    return true;
}

bool IPFilterHarness::check_IPFilterLuma_hps_primitive(filter_hps_t ref, filter_hps_t opt)
{
    intptr_t rand_srcStride, rand_dstStride;

    for (int i = 0; i < ITERS; i++)
    {
        int index = i % TEST_CASES;

        for (int coeffIdx = 0; coeffIdx < 4; coeffIdx++)
        {
            // 0 : Interpolate W x H, 1 : Interpolate W x (H + 7)
            for (int isRowExt = 0; isRowExt < 2; isRowExt++)
            {
                rand_srcStride = rand() % 100;
                rand_dstStride = rand() % 100 + 64;

                ref(pixel_test_buff[index] + 3 * rand_srcStride + 6,
                    rand_srcStride,
                    IPF_C_output_s,
                    rand_dstStride,
                    coeffIdx,
                    isRowExt);

                checked(opt, pixel_test_buff[index] + 3 * rand_srcStride + 6,
                        rand_srcStride,
                        IPF_vec_output_s,
                        rand_dstStride,
                        coeffIdx,
                        isRowExt);

                if (memcmp(IPF_vec_output_s, IPF_C_output_s, TEST_BUF_SIZE * sizeof(int16_t)))
                    return false;

                reportfail();
            }
        }
    }

    return true;
}

bool IPFilterHarness::check_IPFilterLuma_sp_primitive(filter_sp_t ref, filter_sp_t opt)
{
    intptr_t rand_srcStride, rand_dstStride;

    for (int i = 0; i < ITERS; i++)
    {
        int index = i % TEST_CASES;

        for (int coeffIdx = 0; coeffIdx < 4; coeffIdx++)
        {
            rand_srcStride = rand() % 100;
            rand_dstStride = rand() % 100 + 64;

            ref(short_test_buff[index] + 3 * rand_srcStride,
                rand_srcStride,
                IPF_C_output_p,
                rand_dstStride,
                coeffIdx);

            checked(opt, short_test_buff[index] + 3 * rand_srcStride,
                    rand_srcStride,
                    IPF_vec_output_p,
                    rand_dstStride,
                    coeffIdx);

            if (memcmp(IPF_vec_output_p, IPF_C_output_p, TEST_BUF_SIZE * sizeof(pixel)))
                return false;

            reportfail();
        }
    }

    return true;
}

bool IPFilterHarness::check_IPFilterLuma_ss_primitive(filter_ss_t ref, filter_ss_t opt)
{
    intptr_t rand_srcStride, rand_dstStride;

    for (int i = 0; i < ITERS; i++)
    {
        int index = i % TEST_CASES;

        for (int coeffIdx = 0; coeffIdx < 4; coeffIdx++)
        {
            rand_srcStride = rand() % 100;
            rand_dstStride = rand() % 100 + 64;

            ref(short_test_buff[index] + 3 * rand_srcStride,
                rand_srcStride,
                IPF_C_output_s,
                rand_dstStride,
                coeffIdx);

            checked(opt, short_test_buff[index] + 3 * rand_srcStride,
                    rand_srcStride,
                    IPF_vec_output_s,
                    rand_dstStride,
                    coeffIdx);

            if (memcmp(IPF_C_output_s, IPF_vec_output_s, TEST_BUF_SIZE * sizeof(int16_t)))
                return false;

            reportfail();
        }
    }

    return true;
}

bool IPFilterHarness::check_IPFilterLumaHV_primitive(filter_hv_pp_t ref, filter_hv_pp_t opt)
{
    intptr_t rand_srcStride, rand_dstStride;

    for (int i = 0; i < ITERS; i++)
    {
        int index = i % TEST_CASES;

        for (int coeffIdxX = 0; coeffIdxX < 4; coeffIdxX++)
        {
            for (int coeffIdxY = 0; coeffIdxY < 4; coeffIdxY++)
            {
                rand_srcStride = rand() % 100;
                rand_dstStride = rand() % 100 + 64;

                ref(pixel_test_buff[index] + 3 * rand_srcStride + 3,
                    rand_srcStride,
                    IPF_C_output_p,
                    rand_dstStride,
                    coeffIdxX,
                    coeffIdxY);

                checked(opt, pixel_test_buff[index] + 3 * rand_srcStride + 3,
                        rand_srcStride,
                        IPF_vec_output_p,
                        rand_dstStride,
                        coeffIdxX,
                        coeffIdxY);

                if (memcmp(IPF_vec_output_p, IPF_C_output_p, TEST_BUF_SIZE * sizeof(pixel)))
                    return false;

                reportfail();
            }
        }
    }

    return true;
}

bool IPFilterHarness::check_IPFilterLumaP2S_primitive(filter_p2s_t ref, filter_p2s_t opt)
{
    for (int i = 0; i < ITERS; i++)
    {
        intptr_t rand_srcStride = rand() % 100;
        int index = i % TEST_CASES;
        intptr_t dstStride = rand() % 100 + 64;

        ref(pixel_test_buff[index] + i, rand_srcStride, IPF_C_output_s, dstStride);

        checked(opt, pixel_test_buff[index] + i, rand_srcStride, IPF_vec_output_s, dstStride);

        if (memcmp(IPF_vec_output_s, IPF_C_output_s, TEST_BUF_SIZE * sizeof(int16_t)))
            return false;

        reportfail();
    }

    return true;
}

bool IPFilterHarness::check_IPFilterChromaP2S_primitive(filter_p2s_t ref, filter_p2s_t opt)
{
    for (int i = 0; i < ITERS; i++)
    {
        intptr_t rand_srcStride = rand() % 100;
        int index = i % TEST_CASES;
        intptr_t dstStride = rand() % 100 + 64;

        ref(pixel_test_buff[index] + i, rand_srcStride, IPF_C_output_s, dstStride);

        checked(opt, pixel_test_buff[index] + i, rand_srcStride, IPF_vec_output_s, dstStride);

        if (memcmp(IPF_vec_output_s, IPF_C_output_s, TEST_BUF_SIZE * sizeof(int16_t)))
            return false;

        reportfail();
    }

    return true;
}

bool IPFilterHarness::testCorrectness(const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{

    for (int value = 0; value < NUM_PU_SIZES; value++)
    {
        if (opt.pu[value].luma_hpp)
        {
            if (!check_IPFilterLuma_primitive(ref.pu[value].luma_hpp, opt.pu[value].luma_hpp))
            {
                printf("luma_hpp[%s]", lumaPartStr[value]);
                return false;
            }
        }
        if (opt.pu[value].luma_hps)
        {
            if (!check_IPFilterLuma_hps_primitive(ref.pu[value].luma_hps, opt.pu[value].luma_hps))
            {
                printf("luma_hps[%s]", lumaPartStr[value]);
                return false;
            }
        }
        if (opt.pu[value].luma_vpp)
        {
            if (!check_IPFilterLuma_primitive(ref.pu[value].luma_vpp, opt.pu[value].luma_vpp))
            {
                printf("luma_vpp[%s]", lumaPartStr[value]);
                return false;
            }
        }
        if (opt.pu[value].luma_vps)
        {
            if (!check_IPFilterLuma_ps_primitive(ref.pu[value].luma_vps, opt.pu[value].luma_vps))
            {
                printf("luma_vps[%s]", lumaPartStr[value]);
                return false;
            }
        }
        if (opt.pu[value].luma_vsp)
        {
            if (!check_IPFilterLuma_sp_primitive(ref.pu[value].luma_vsp, opt.pu[value].luma_vsp))
            {
                printf("luma_vsp[%s]", lumaPartStr[value]);
                return false;
            }
        }
        if (opt.pu[value].luma_vss)
        {
            if (!check_IPFilterLuma_ss_primitive(ref.pu[value].luma_vss, opt.pu[value].luma_vss))
            {
                printf("luma_vss[%s]", lumaPartStr[value]);
                return false;
            }
        }
        if (opt.pu[value].luma_hvpp)
        {
            if (!check_IPFilterLumaHV_primitive(ref.pu[value].luma_hvpp, opt.pu[value].luma_hvpp))
            {
                printf("luma_hvpp[%s]", lumaPartStr[value]);
                return false;
            }
        }
        if (opt.pu[value].convert_p2s)
        {
            if (!check_IPFilterLumaP2S_primitive(ref.pu[value].convert_p2s, opt.pu[value].convert_p2s))
            {
                printf("convert_p2s[%s]", lumaPartStr[value]);
                return false;
            }
        }
    }

    for (int csp = X265_CSP_I420; csp < X265_CSP_COUNT; csp++)
    {
        for (int value = 0; value < NUM_PU_SIZES; value++)
        {
            if (opt.chroma[csp].pu[value].filter_hpp)
            {
                if (!check_IPFilterChroma_primitive(ref.chroma[csp].pu[value].filter_hpp, opt.chroma[csp].pu[value].filter_hpp))
                {
                    printf("chroma_hpp[%s]", chromaPartStr[csp][value]);
                    return false;
                }
            }
            if (opt.chroma[csp].pu[value].filter_hps)
            {
                if (!check_IPFilterChroma_hps_primitive(ref.chroma[csp].pu[value].filter_hps, opt.chroma[csp].pu[value].filter_hps))
                {
                    printf("chroma_hps[%s]", chromaPartStr[csp][value]);
                    return false;
                }
            }
            if (opt.chroma[csp].pu[value].filter_vpp)
            {
                if (!check_IPFilterChroma_primitive(ref.chroma[csp].pu[value].filter_vpp, opt.chroma[csp].pu[value].filter_vpp))
                {
                    printf("chroma_vpp[%s]", chromaPartStr[csp][value]);
                    return false;
                }
            }
            if (opt.chroma[csp].pu[value].filter_vps)
            {
                if (!check_IPFilterChroma_ps_primitive(ref.chroma[csp].pu[value].filter_vps, opt.chroma[csp].pu[value].filter_vps))
                {
                    printf("chroma_vps[%s]", chromaPartStr[csp][value]);
                    return false;
                }
            }
            if (opt.chroma[csp].pu[value].filter_vsp)
            {
                if (!check_IPFilterChroma_sp_primitive(ref.chroma[csp].pu[value].filter_vsp, opt.chroma[csp].pu[value].filter_vsp))
                {
                    printf("chroma_vsp[%s]", chromaPartStr[csp][value]);
                    return false;
                }
            }
            if (opt.chroma[csp].pu[value].filter_vss)
            {
                if (!check_IPFilterChroma_ss_primitive(ref.chroma[csp].pu[value].filter_vss, opt.chroma[csp].pu[value].filter_vss))
                {
                    printf("chroma_vss[%s]", chromaPartStr[csp][value]);
                    return false;
                }
            }
            if (opt.chroma[csp].pu[value].p2s)
            {
                if (!check_IPFilterChromaP2S_primitive(ref.chroma[csp].pu[value].p2s, opt.chroma[csp].pu[value].p2s))
                {
                    printf("chroma_p2s[%s]", chromaPartStr[csp][value]);
                    return false;
                }
            }
        }
    }

    return true;
}

void IPFilterHarness::measureSpeed(const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{
    int16_t srcStride = 96;
    int16_t dstStride = 96;
    int maxVerticalfilterHalfDistance = 3;

    for (int value = 0; value < NUM_PU_SIZES; value++)
    {
        if (opt.pu[value].luma_hpp)
        {
            printf("luma_hpp[%s]\t", lumaPartStr[value]);
            REPORT_SPEEDUP(opt.pu[value].luma_hpp, ref.pu[value].luma_hpp,
                           pixel_buff + srcStride, srcStride, IPF_vec_output_p, dstStride, 1);
        }

        if (opt.pu[value].luma_hps)
        {
            printf("luma_hps[%s]\t", lumaPartStr[value]);
            REPORT_SPEEDUP(opt.pu[value].luma_hps, ref.pu[value].luma_hps,
                           pixel_buff + maxVerticalfilterHalfDistance * srcStride, srcStride,
                           IPF_vec_output_s, dstStride, 1, 1);
        }

        if (opt.pu[value].luma_vpp)
        {
            printf("luma_vpp[%s]\t", lumaPartStr[value]);
            REPORT_SPEEDUP(opt.pu[value].luma_vpp, ref.pu[value].luma_vpp,
                           pixel_buff + maxVerticalfilterHalfDistance * srcStride, srcStride,
                           IPF_vec_output_p, dstStride, 1);
        }

        if (opt.pu[value].luma_vps)
        {
            printf("luma_vps[%s]\t", lumaPartStr[value]);
            REPORT_SPEEDUP(opt.pu[value].luma_vps, ref.pu[value].luma_vps,
                           pixel_buff + maxVerticalfilterHalfDistance * srcStride, srcStride,
                           IPF_vec_output_s, dstStride, 1);
        }

        if (opt.pu[value].luma_vsp)
        {
            printf("luma_vsp[%s]\t", lumaPartStr[value]);
            REPORT_SPEEDUP(opt.pu[value].luma_vsp, ref.pu[value].luma_vsp,
                           short_buff + maxVerticalfilterHalfDistance * srcStride, srcStride,
                           IPF_vec_output_p, dstStride, 1);
        }

        if (opt.pu[value].luma_vss)
        {
            printf("luma_vss[%s]\t", lumaPartStr[value]);
            REPORT_SPEEDUP(opt.pu[value].luma_vss, ref.pu[value].luma_vss,
                           short_buff + maxVerticalfilterHalfDistance * srcStride, srcStride,
                           IPF_vec_output_s, dstStride, 1);
        }

        if (opt.pu[value].luma_hvpp)
        {
            printf("luma_hv [%s]\t", lumaPartStr[value]);
            REPORT_SPEEDUP(opt.pu[value].luma_hvpp, ref.pu[value].luma_hvpp,
                           pixel_buff + 3 * srcStride, srcStride, IPF_vec_output_p, srcStride, 1, 3);
        }

        if (opt.pu[value].convert_p2s)
        {
            printf("convert_p2s[%s]\t", lumaPartStr[value]);
            REPORT_SPEEDUP(opt.pu[value].convert_p2s, ref.pu[value].convert_p2s,
                               pixel_buff, srcStride,
                               IPF_vec_output_s, dstStride);
        }
    }

    for (int csp = X265_CSP_I420; csp < X265_CSP_COUNT; csp++)
    {
        printf("= Color Space %s =\n", x265_source_csp_names[csp]);
        for (int value = 0; value < NUM_PU_SIZES; value++)
        {
            if (opt.chroma[csp].pu[value].filter_hpp)
            {
                printf("chroma_hpp[%s]", chromaPartStr[csp][value]);
                REPORT_SPEEDUP(opt.chroma[csp].pu[value].filter_hpp, ref.chroma[csp].pu[value].filter_hpp,
                               pixel_buff + srcStride, srcStride, IPF_vec_output_p, dstStride, 1);
            }
            if (opt.chroma[csp].pu[value].filter_hps)
            {
                printf("chroma_hps[%s]", chromaPartStr[csp][value]);
                REPORT_SPEEDUP(opt.chroma[csp].pu[value].filter_hps, ref.chroma[csp].pu[value].filter_hps,
                               pixel_buff + srcStride, srcStride, IPF_vec_output_s, dstStride, 1, 1);
            }
            if (opt.chroma[csp].pu[value].filter_vpp)
            {
                printf("chroma_vpp[%s]", chromaPartStr[csp][value]);
                REPORT_SPEEDUP(opt.chroma[csp].pu[value].filter_vpp, ref.chroma[csp].pu[value].filter_vpp,
                               pixel_buff + maxVerticalfilterHalfDistance * srcStride, srcStride,
                               IPF_vec_output_p, dstStride, 1);
            }
            if (opt.chroma[csp].pu[value].filter_vps)
            {
                printf("chroma_vps[%s]", chromaPartStr[csp][value]);
                REPORT_SPEEDUP(opt.chroma[csp].pu[value].filter_vps, ref.chroma[csp].pu[value].filter_vps,
                               pixel_buff + maxVerticalfilterHalfDistance * srcStride, srcStride,
                               IPF_vec_output_s, dstStride, 1);
            }
            if (opt.chroma[csp].pu[value].filter_vsp)
            {
                printf("chroma_vsp[%s]", chromaPartStr[csp][value]);
                REPORT_SPEEDUP(opt.chroma[csp].pu[value].filter_vsp, ref.chroma[csp].pu[value].filter_vsp,
                               short_buff + maxVerticalfilterHalfDistance * srcStride, srcStride,
                               IPF_vec_output_p, dstStride, 1);
            }
            if (opt.chroma[csp].pu[value].filter_vss)
            {
                printf("chroma_vss[%s]", chromaPartStr[csp][value]);
                REPORT_SPEEDUP(opt.chroma[csp].pu[value].filter_vss, ref.chroma[csp].pu[value].filter_vss,
                               short_buff + maxVerticalfilterHalfDistance * srcStride, srcStride,
                               IPF_vec_output_s, dstStride, 1);
            }
            if (opt.chroma[csp].pu[value].p2s)
            {
                printf("chroma_p2s[%s]\t", chromaPartStr[csp][value]);
                REPORT_SPEEDUP(opt.chroma[csp].pu[value].p2s, ref.chroma[csp].pu[value].p2s,
                               pixel_buff, srcStride, IPF_vec_output_s, dstStride);
            }
        }
    }
}
