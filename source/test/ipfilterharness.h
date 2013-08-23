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

#ifndef _IPFILTERHARNESS_H_1
#define _IPFILTERHARNESS_H_1 1

#include "testharness.h"
#include "primitives.h"

class IPFilterHarness : public TestHarness
{
protected:

    pixel *pixel_buff;
    short *short_buff;

    pixel *IPF_vec_output_p, *IPF_C_output_p;
    short *IPF_vec_output_s, *IPF_C_output_s;

    int ipf_t_size;

    bool check_IPFilter_primitive(x265::ipfilter_pp_t ref, x265::ipfilter_pp_t opt);
    bool check_IPFilter_primitive(x265::ipfilter_ps_t ref, x265::ipfilter_ps_t opt);
    bool check_IPFilter_primitive(x265::ipfilter_sp_t ref, x265::ipfilter_sp_t opt);
    bool check_IPFilter_primitive(x265::ipfilter_p2s_t ref, x265::ipfilter_p2s_t opt);
    bool check_IPFilter_primitive(x265::ipfilter_s2p_t ref, x265::ipfilter_s2p_t opt);
    bool check_filterVMultiplane(x265::filterVmulti_t ref, x265::filterVmulti_t opt);
    bool check_filterHMultiplane(x265::filterHmulti_t ref, x265::filterHmulti_t opt);
    bool check_filterHMultiplaneWghtd(x265::filterHwghtd_t ref, x265::filterHwghtd_t opt);
    bool check_filterVMultiplaneWghtd(x265::filterVwghtd_t ref, x265::filterVwghtd_t opt);
    bool check_filterHMultiplaneCU(x265::cuRowfilterHmulti_t ref, x265::cuRowfilterHmulti_t opt);

public:

    IPFilterHarness();

    virtual ~IPFilterHarness();

    const char *getName() const { return "interp"; }

    bool testCorrectness(const x265::EncoderPrimitives& ref, const x265::EncoderPrimitives& opt);

    void measureSpeed(const x265::EncoderPrimitives& ref, const x265::EncoderPrimitives& opt);
};

#endif // ifndef _FILTERHARNESS_H_1
