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

#include "TLibCommon/TComPicYuv.h"
#include "common.h"
#include "reference.h"
#include "mv.h"

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
    int    bframes;
    bool   bIntraCalculated;

    /* lookahead output data */
    int       costEst[X265_BFRAME_MAX + 2][X265_BFRAME_MAX + 2];
    int      *rowSatds[X265_BFRAME_MAX + 2][X265_BFRAME_MAX + 2];
    int       intraMbs[X265_BFRAME_MAX + 2];
    uint16_t(*lowresCosts[X265_BFRAME_MAX + 2][X265_BFRAME_MAX + 2]);
    int      *lowresMvCosts[2][X265_BFRAME_MAX + 1];
    MV       *lowresMvs[2][X265_BFRAME_MAX + 1];

    void create(TComPic *pic, int bframes);

    void destroy()
    {
        for (int i = 0; i < 4; i++)
        {
            if (buffer[i])
                X265_FREE(buffer[i]);
        }

        for (int i = 0; i < bframes + 2; i++)
        {
            for (int j = 0; j < bframes + 2; j++)
            {   
                if (rowSatds[i][j]) X265_FREE(rowSatds[i][j]);
                if (lowresCosts[i][j]) X265_FREE(lowresCosts[i][j]);
            }
        }

        for (int i = 0; i < bframes + 1; i++)
        {
            if (lowresMvs[0][i]) X265_FREE(lowresMvs[0][i]);
            if (lowresMvs[1][i]) X265_FREE(lowresMvs[1][i]);
            if (lowresMvCosts[0][i]) X265_FREE(lowresMvCosts[0][i]);
            if (lowresMvCosts[1][i]) X265_FREE(lowresMvCosts[1][i]);
        }
    }

    // (re) initialize lowres state
    void init(TComPicYuv *orig)
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

        /* downscale and generate 4 HPEL planes for lookahead */
        x265::primitives.frame_init_lowres_core(orig->getLumaAddr(),
            m_lumaPlane[0][0], m_lumaPlane[2][0], m_lumaPlane[0][2], m_lumaPlane[2][2],
            orig->getStride(), stride, width, lines);

        /* extend hpel planes for motion search */
        orig->xExtendPicCompBorder(m_lumaPlane[0][0], stride, width, lines, orig->getLumaMarginX(), orig->getLumaMarginY());
        orig->xExtendPicCompBorder(m_lumaPlane[2][0], stride, width, lines, orig->getLumaMarginX(), orig->getLumaMarginY());
        orig->xExtendPicCompBorder(m_lumaPlane[0][2], stride, width, lines, orig->getLumaMarginX(), orig->getLumaMarginY());
        orig->xExtendPicCompBorder(m_lumaPlane[2][2], stride, width, lines, orig->getLumaMarginX(), orig->getLumaMarginY());
    }
};
}

#endif // _LOOKAHEAD_H
