/*****************************************************************************
* Copyright (C) 2013 x265 project
*
* Authors: Praveen Kumar Tiwari <praveen@multicorewareinc.com>
*          Dnyaneshwar Gorade <dnyaneshwar@multicorewareinc.com>
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

#include "TLibCommon/TypeDef.h"
#include "primitives.h"

#define PIXEL_MIN 0
#define PIXEL_MAX ((1 << X265_DEPTH) - 1)

void processSaoCUE0(pixel * rec, int8_t * offsetEo, int width, int8_t signLeft)
{
    int x;
    int8_t signRight;
    int8_t edgeType;

    for (x = 0; x < width; x++)
    {
        signRight = ((rec[x] - rec[x + 1]) < 0) ? -1 : ((rec[x] - rec[x + 1]) > 0) ? 1 : 0;
        edgeType = signRight + signLeft + 2;
        signLeft  = -signRight;

        short v = rec[x] + offsetEo[edgeType];
        rec[x] = (pixel)(v < 0 ? 0 : (v > (PIXEL_MAX)) ? (PIXEL_MAX) : v);
    }
}

namespace x265 {
void Setup_C_LoopFilterPrimitives(EncoderPrimitives &p)
{
    p.saoCuOrgE0 = processSaoCUE0;
}
}
