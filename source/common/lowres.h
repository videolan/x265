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

#ifndef X265_LOWRES_H
#define X265_LOWRES_H

#include "primitives.h"
#include "common.h"
#include "mv.h"

namespace x265 {
// private namespace

class TComPic;
class TComPicYuv;

struct ReferencePlanes
{
    ReferencePlanes() : isWeighted(false), isLowres(false) {}

    pixel* fpelPlane;
    pixel* lowresPlane[4];
    pixel* unweightedFPelPlane;

    bool isWeighted;
    bool isLowres;
    int  lumaStride;
    int  weight;
    int  offset;
    int  shift;
    int  round;

    /* lowres motion compensation, you must provide a buffer and stride for QPEL averaged pixels
     * in case QPEL is required.  Else it returns a pointer to the HPEL pixels */
    inline pixel *lowresMC(intptr_t blockOffset, const MV& qmv, pixel *buf, intptr_t& outstride)
    {
        if ((qmv.x | qmv.y) & 1)
        {
            int hpelA = (qmv.y & 2) | ((qmv.x & 2) >> 1);
            pixel *frefA = lowresPlane[hpelA] + blockOffset + (qmv.x >> 2) + (qmv.y >> 2) * lumaStride;

            MV qmvB = qmv + MV((qmv.x & 1) * 2, (qmv.y & 1) * 2);
            int hpelB = (qmvB.y & 2) | ((qmvB.x & 2) >> 1);
            
            pixel *frefB = lowresPlane[hpelB] + blockOffset + (qmvB.x >> 2) + (qmvB.y >> 2) * lumaStride;
            primitives.pixelavg_pp[LUMA_8x8](buf, outstride, frefA, lumaStride, frefB, lumaStride, 32);
            return buf;
        }
        else
        {
            outstride = lumaStride;
            int hpel = (qmv.y & 2) | ((qmv.x & 2) >> 1);
            return lowresPlane[hpel] + blockOffset + (qmv.x >> 2) + (qmv.y >> 2) * lumaStride;
        }
    }

    inline int lowresQPelCost(pixel *fenc, intptr_t blockOffset, const MV& qmv, pixelcmp_t comp)
    {
        if ((qmv.x | qmv.y) & 1)
        {
            ALIGN_VAR_16(pixel, subpelbuf[8 * 8]);
            int hpelA = (qmv.y & 2) | ((qmv.x & 2) >> 1);
            pixel *frefA = lowresPlane[hpelA] + blockOffset + (qmv.x >> 2) + (qmv.y >> 2) * lumaStride;
            MV qmvB = qmv + MV((qmv.x & 1) * 2, (qmv.y & 1) * 2);
            int hpelB = (qmvB.y & 2) | ((qmvB.x & 2) >> 1);
            pixel *frefB = lowresPlane[hpelB] + blockOffset + (qmvB.x >> 2) + (qmvB.y >> 2) * lumaStride;
            primitives.pixelavg_pp[LUMA_8x8](subpelbuf, 8, frefA, lumaStride, frefB, lumaStride, 32);
            return comp(fenc, FENC_STRIDE, subpelbuf, 8);
        }
        else
        {
            int hpel = (qmv.y & 2) | ((qmv.x & 2) >> 1);
            pixel *fref = lowresPlane[hpel] + blockOffset + (qmv.x >> 2) + (qmv.y >> 2) * lumaStride;
            return comp(fenc, FENC_STRIDE, fref, lumaStride);
        }
    }
};

struct Lowres : public ReferencePlanes
{
    /* lowres buffers, sizes and strides */
    pixel *buffer[4];
    double *qpAqOffset; // qp Aq offset values for each Cu
    int    *invQscaleFactor; // qScale values for qp Aq Offsets
    int    width;     // width of lowres frame in pixels
    int    lines;     // height of lowres frame in pixel lines
    int    frameNum;  // Presentation frame number
    int    sliceType; // Slice type decided by lookahead
    int    leadingBframes; // number of leading B frames for P or I
    uint64_t wp_ssd[3];  // This is different than m_SSDY, this is sum(pixel^2) - sum(pixel)^2 for entire frame
    uint64_t wp_sum[3];

    bool   bIntraCalculated;
    bool   bScenecut; // Set to false if the frame cannot possibly be part of a real scenecut.
    bool   bKeyframe;
    bool   bLastMiniGopBFrame;

    /* lookahead output data */
    int       costEst[X265_BFRAME_MAX + 2][X265_BFRAME_MAX + 2];
    int       costEstAq[X265_BFRAME_MAX + 2][X265_BFRAME_MAX + 2];
    int32_t*  rowSatds[X265_BFRAME_MAX + 2][X265_BFRAME_MAX + 2];
    int       intraMbs[X265_BFRAME_MAX + 2];
    int32_t*  intraCost;
    int       satdCost;
    uint16_t(*lowresCosts[X265_BFRAME_MAX + 2][X265_BFRAME_MAX + 2]);
    int32_t  *lowresMvCosts[2][X265_BFRAME_MAX + 1];
    MV       *lowresMvs[2][X265_BFRAME_MAX + 1];

    void create(TComPic *pic, int bframes, int32_t *aqMode);
    void destroy(int bframes);
    void init(TComPicYuv *orig, int poc, int sliceType, int bframes);
};
}

#endif // ifndef X265_LOWRES_H
