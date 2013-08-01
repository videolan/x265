/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Deepthi Devaki <deepthidevaki@multicorewareinc.com>,
 *          Rajesh Paulraj <rajesh@multicorewareinc.com>
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

#include "utils.h"
#include "primitives.h"
#include "TLibCommon/TComRom.h"
#include <assert.h>
#include <string.h>

namespace {
#if HIGH_BIT_DEPTH
#include "ipfilter16.inc"
#else
#include "ipfilter8.inc"
#endif
}

namespace x265 {
void NAME(Setup_Vec_IPFilterPrimitives)(EncoderPrimitives& p)
{
#if INSTRSET > 3
    p.ipfilter_pp[FILTER_H_P_P_4] = filterHorizontal_p_p<4>;
    p.ipfilter_pp[FILTER_H_P_P_8] = filterHorizontal_p_p<8>;
    p.ipfilter_ps[FILTER_H_P_S_4] = filterHorizontal_p_s<4>;
    p.ipfilter_ps[FILTER_H_P_S_8] = filterHorizontal_p_s<8>;
#endif

    p.ipfilter_pp[FILTER_V_P_P_4] = filterVertical_p_p<4>;
    p.ipfilter_pp[FILTER_V_P_P_8] = filterVertical_p_p<8>;

#if INSTRSET > 4
    p.ipfilter_sp[FILTER_V_S_P_4] = filterVertical_s_p<4>;
    p.ipfilter_sp[FILTER_V_S_P_8] = filterVertical_s_p<8>;
#endif

    p.ipfilter_p2s = filterConvertPelToShort;
    p.ipfilter_s2p = filterConvertShortToPel;

#if !HIGH_BIT_DEPTH
    p.filterVmulti = filterVerticalMultiplaneExtend;
#if INSTRSET > 4 && !(defined(_MSC_VER) && _MSC_VER == 1500 && X86_64)
    p.filterHmulti = filterHorizontalMultiplaneExtend;
#endif
#endif
}
}
