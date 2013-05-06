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

void Setup_Vec_IPFilterPrimitives(EncoderPrimitives& p)
{
    p.ipFilter_p_p[FILTER_H_P_P_4] = filterHorizontal_pel_pel<4>;
    p.ipFilter_p_p[FILTER_H_P_P_8] = filterHorizontal_pel_pel<8>;    
    p.ipFilter_p_s[FILTER_H_P_S_4] = filterHorizontal_pel_short<4>;
    p.ipFilter_p_s[FILTER_H_P_S_8] = filterHorizontal_pel_short<8>;
    p.ipfilterConvert_p_s = filterConvertPelToShort;
    p.ipfilterConvert_s_p = filterConvertShortToPel;
    p.ipFilter_s_p[FILTER_V_S_P_8] = filterVertical_short_pel<8>;
    p.ipFilter_s_p[FILTER_V_S_P_4] = filterVertical_short_pel<4>;    
}
