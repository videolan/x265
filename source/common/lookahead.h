/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Gopu Govindaswamy <gopu@multicorewareinc.com>
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

#ifndef _LOOKAHEAD_H
#define _LOOKAHEAD_H 1

#include "TLibCommon/TComList.h"

#include "common.h"
#include "mv.h"
#include "reference.h"

class TComPic;

namespace x265 {

#define X265_BFRAME_MAX 16

struct LookaheadFrame : public ReferencePlanes
{
    /* lowres buffers, sizes and strides */
    pixel *buffer[4];
    int    stride;   // distance to below pixel
    int    width;    // width of lowres frame in pixels
    int    lines;    // height of lowres frame in pixel lines
    int    cuWidth;  // width of lowres frame in downscale CUs
    int    cuHeight; // height of lowres frame in downscale CUs
    bool   bIntraCalculated;

    /* lookahead output data */
    int       costEst[X265_BFRAME_MAX + 2][X265_BFRAME_MAX + 2];
    int      *rowSatds[X265_BFRAME_MAX + 2][X265_BFRAME_MAX + 2];
    int       intraMbs[X265_BFRAME_MAX + 2];
    uint16_t(*lowresCosts[X265_BFRAME_MAX + 2][X265_BFRAME_MAX + 2]);
    int      *lowresMvCosts[2][X265_BFRAME_MAX + 1];
    MV       *lowresMvs[2][X265_BFRAME_MAX + 1];

    // (re) initialize lowres state
    void init(int bframes)
    {
        bIntraCalculated = false;
        memset(costEst, -1, sizeof(costEst));
        for (int y = 0; y < bframes + 2; y++)
        {
            for (int x = 0; x < bframes + 2; x++)
            {
                rowSatds[y][x][0] = -1;
            }
        }
        for (int i = 0; i < bframes + 1; i++)
        {
            lowresMvs[0][i]->x = 0x7fff;
            lowresMvs[1][i]->x = 0x7fff;
        }
    }
};
}

#endif // _LOOKAHEAD_H
