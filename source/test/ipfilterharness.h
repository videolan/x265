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
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#ifndef _IPFILTERHARNESS_H_1
#define _IPFILTERHARNESS_H_1 1

#include "testharness.h"
#include "primitives.h"

class IPFilterHarness : public TestHarness
{
protected:

    // Assuming max_height = max_width = max_srcStride = max_dstStride = 100
    enum { TEST_BUF_SIZE = 200 * 200 };
    enum { ITERS = 100 };
    enum { TEST_CASES = 3 };
    enum { SMAX = 1 << 12 };
    enum { SMIN = -1 << 12 };

    ALIGN_VAR_32(pixel, pixel_buff[TEST_BUF_SIZE]);
    int16_t short_buff[TEST_BUF_SIZE];
    int16_t IPF_vec_output_s[TEST_BUF_SIZE];
    int16_t IPF_C_output_s[TEST_BUF_SIZE];
    pixel   IPF_vec_output_p[TEST_BUF_SIZE];
    pixel   IPF_C_output_p[TEST_BUF_SIZE];

    pixel   pixel_test_buff[TEST_CASES][TEST_BUF_SIZE];
    int16_t short_test_buff[TEST_CASES][TEST_BUF_SIZE];

    bool check_IPFilterChroma_primitive(filter_pp_t ref, filter_pp_t opt);
    bool check_IPFilterChroma_ps_primitive(filter_ps_t ref, filter_ps_t opt);
    bool check_IPFilterChroma_hps_primitive(filter_hps_t ref, filter_hps_t opt);
    bool check_IPFilterChroma_sp_primitive(filter_sp_t ref, filter_sp_t opt);
    bool check_IPFilterChroma_ss_primitive(filter_ss_t ref, filter_ss_t opt);
    bool check_IPFilterLuma_primitive(filter_pp_t ref, filter_pp_t opt);
    bool check_IPFilterLuma_ps_primitive(filter_ps_t ref, filter_ps_t opt);
    bool check_IPFilterLuma_hps_primitive(filter_hps_t ref, filter_hps_t opt);
    bool check_IPFilterLuma_sp_primitive(filter_sp_t ref, filter_sp_t opt);
    bool check_IPFilterLuma_ss_primitive(filter_ss_t ref, filter_ss_t opt);
    bool check_IPFilterLumaHV_primitive(filter_hv_pp_t ref, filter_hv_pp_t opt);
    bool check_IPFilterLumaP2S_primitive(filter_p2s_t ref, filter_p2s_t opt);
    bool check_IPFilterChromaP2S_primitive(filter_p2s_t ref, filter_p2s_t opt);

public:

    IPFilterHarness();

    const char *getName() const { return "interp"; }

    bool testCorrectness(const EncoderPrimitives& ref, const EncoderPrimitives& opt);

    void measureSpeed(const EncoderPrimitives& ref, const EncoderPrimitives& opt);
};

#endif // ifndef _FILTERHARNESS_H_1
