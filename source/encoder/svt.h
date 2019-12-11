/*****************************************************************************
* Copyright (C) 2013-2018 MulticoreWare, Inc
*
* Authors: Radhakrishnan <radhakrishnan@multicorewareinc.com>
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


#ifndef SVT_H
#define SVT_H

#ifdef SVT_HEVC

#include "EbApi.h"
#include "EbErrorCodes.h"

namespace X265_NS {

#define INPUT_SIZE_576p_TH     0x90000    // 0.58 Million
#define INPUT_SIZE_1080i_TH    0xB71B0    // 0.75 Million
#define INPUT_SIZE_1080p_TH    0x1AB3F0   // 1.75 Million
#define INPUT_SIZE_4K_TH       0x29F630   // 2.75 Million

#define EB_OUTPUTSTREAMBUFFERSIZE_MACRO(ResolutionSize)    ((ResolutionSize) < (INPUT_SIZE_1080i_TH) ? 0x1E8480 : (ResolutionSize) < (INPUT_SIZE_1080p_TH) ? 0x2DC6C0 : (ResolutionSize) < (INPUT_SIZE_4K_TH) ? 0x2DC6C0 : 0x2DC6C0)

void svt_param_default(x265_param* param);
int svt_set_preset(x265_param* param, const char* preset);
int svt_param_parse(x265_param* param, const char* name, const char* value);
void svt_initialise_app_context(x265_encoder *enc);
int svt_initialise_input_buffer(x265_encoder *enc);
}

#endif // ifdef SVT_HEVC

#endif // ifndef SVT_H
