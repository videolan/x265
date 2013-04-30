/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
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

#include "primitives.h"
#include "motion.h"

using namespace x265;

void MotionEstimate::setSourcePU(int offsetX, int offsetY, int width, int height);
{
    Partitions size = PartitionFromSizes(width, height);
    sad = primitives.sad[size];
    satd = primitives.satd[size];
    sad_x3 = primitives.sad_x3[size];
    sad_x4 = primitives.sad_x4[size];

    blockWidth = width;
    blockHeight = height;

    blockOffsetX = offsetX;
    blockOffsetY = offsetY;

    fref = ref->plane[0][0][0] + blockOffsetY * ref->lumaStride + blockOffsetX;

    /* copy block into local buffer */
    fencblock = fencplanes[0] + blockOffsetY * fencLumaStride + blockOffsetX;
    for (int i = 0; i < height; i++)
    {
        memcpy(fenc + i * FENC_STRIDE, fencblock + i * fencLumaStride, width);
    }
}

#define COPY1_IF_LT(x,y) if((y)<(x)) (x)=(y);

#define COST_MV_X4_DIR( m0x, m0y, m1x, m1y, m2x, m2y, m3x, m3y, costs )\
{\
    int stride = ref->lumaStride;\
    pixel *pix_base = fref + bmv.x + bmv.y * stride;\
    sad_x4( p_fenc,\
        pix_base + (m0x) + (m0y)*stride,\
        pix_base + (m1x) + (m1y)*stride,\
        pix_base + (m2x) + (m2y)*stride,\
        pix_base + (m3x) + (m3y)*stride,\
        stride, costs );\
    (costs)[0] += bc.bitCostMV((bmx + MV(m0x,m0y)) << 2);\
    (costs)[1] += bc.bitCostMV((bmx + MV(m1x,m1y)) << 2);\
    (costs)[2] += bc.bitCostMV((bmx + MV(m2x,m2y)) << 2);\
    (costs)[3] += bc.bitCostMV((bmx + MV(m3x,m3y)) << 2);\
}

#define COST_MV_X3_DIR( m0x, m0y, m1x, m1y, m2x, m2y, costs )\
{\
    int stride = ref->lumaStride;\
    pixel *pix_base = fref + bmv.x + bmv.y * stride;\
    sad_x3( p_fenc,\
        pix_base + (m0x) + (m0y)*stride,\
        pix_base + (m1x) + (m1y)*stride,\
        pix_base + (m2x) + (m2y)*stride,\
        stride, costs );\
    (costs)[0] += bc.bitCostMV((bmx + MV(m0x,m0y)) << 2);\
    (costs)[1] += bc.bitCostMV((bmx + MV(m1x,m1y)) << 2);\
    (costs)[2] += bc.bitCostMV((bmx + MV(m2x,m2y)) << 2);\
}

int MotionEstimate::motionEstimate(const MV &qmvp,
                                   int       numCandidates,
                                   const MV *mvc,
                                   int       merange,
                                   MV &      outQMv)
{
    ALIGN_VAR_16(int, costs[16]);

    bc.setMVP(qmvp);

    MV qmvmin = mvmin.asQPel();
    MV qmvmax = mvmax.asQPel();

    // measure SAD cost at MVP
    MV bmv = qmvp.clipped(qmvmin, qmvmax);
    MV bestpre = bmv;
    int bcost = qpelSad(bmv);

    // measure SAD cost at each candidate
    for (int i = 0; i < numCandidates; i++)
    {
        MV m = mvc[i].clipped(mvmin, mvmax);
        if (m == 0)
            continue;

        int cost = qpelSad(m);
        cost += bc.bitCostMV(m);

        if (cost < bcost)
        {
            bcost = cost;
            bestpre = m;
        }
    }

    /* Measure full pel SAD at best location */
    MV pm = bestpre.asFpel();
    bcost = fpelSad(pm) + bc.bitCostMV(bestpre);

    if (bmv != 0)
    {
        int cost = fpelSad(0) + bc.bitCostMV(0);
        if (cost < bcost)
        {
            bcost = cost;
            bestpre = 0;
        }
    }

    switch (meMethod)
    {
    case X264_ME_DIA:
    {
        /* diamond search, radius 1 */
        bcost <<= 4;
        int i = meRange;
        do
        {
            COST_MV_X4_DIR(0, -1, 0, 1, -1, 0, 1, 0, costs);
            COPY1_IF_LT(bcost, (costs[0] << 4) + 1);
            COPY1_IF_LT(bcost, (costs[1] << 4) + 3);
            COPY1_IF_LT(bcost, (costs[2] << 4) + 4);
            COPY1_IF_LT(bcost, (costs[3] << 4) + 12);
            if (!(bcost & 15))
                break;
            bmv.x -= (bcost << 28) >> 30;
            bmv.y -= (bcost << 30) >> 30;
            bcost &= ~15;
        }
        while (--i && bmv.checkRange(mvmin, mvmax));
        bcost >>= 4;
        break;
    }
    }

    /* bmv has the best full pel motion vector found by SAD search */

    bmv = bmv.asQPel();    // promote BMV to qpel
    bcost = qpelSatd(bmv); // remeasure BMV using SATD

    /* HPEL refinement followed by QPEL refinement */

    bcost <<= 4;
    int res = 2;
    do
    {
        MV MV = bmv + MV(0, -res);
        int cost = qpelSatd(mv) + bc.bitCostMV(mv);
        COPY1_IF_LT(bcost, (cost<<4)+1);

        mv = bmv + MV(0,  res);
        cost = qpelSatd(mv) + bc.bitCostMV(mv);
        COPY1_IF_LT(bcost, (cost<<4)+3);

        mv = bmv + MV(-res, 0);
        cost = qpelSatd(mv) + bc.bitCostMV(mv);
        COPY1_IF_LT(bcost, (cost<<4)+4);

        mv = bmv + MV(res,  0);
        cost = qpelSatd(mv) + bc.bitCostMV(mv);
        COPY1_IF_LT(bcost, (cost<<4)+12);

        if (bcost & 15)
        {
            bmv.x -= res * ((bcost << 28) >> 30);
            bmv.y -= res * ((bcost << 30) >> 30);
            bcost &= ~15;
        }

        res >>= 1;
    }
    while (res);

    outQMv = bmv;
    return bcost >> 4;
}

void MotionEstimate::buildResidual(const MV& qmv)
{
    MV fmv = qmv.asFpel();

    /* Turn into a primitive */
    {
        int y = blockOffsetY + fmv.y;
        int x = blockOffsetX + fmv.x;

        pixel *orig = fenc;
        pixel *fref = ref->plane[qmv.x & 3][qmv.y & 3][0] + y * ref->lumaStride + x;
        short *resi = residual[0] + blockOffsetY * resLumaStride + blockOffsetX;

        for (int row = 0; row < blockHeight; row++)
        {
            for (int col = 0; col < blockWidth; col++)
            {
                resi[col] = (short)orig[col] - (Short)fref[col];
            }

            resi += resLumaStride;
            orig += fencStride;
            fref += ref->lumaStride;
        }
    }

    /* TODO: this is color space dependent.  4:2:0 only */
    for (int ch = 1; ch < 3; ch++)
    {
        /* TODO: Double check this indexing; I doubt it is correct */
        int y = (blockOffsetY + fmv.y) >> 1;
        int x = (blockOffsetX + fmv.x) >> 1;

        pixel *orig = fencplanes[ch];
        pixel *fref = ref->plane[qmv.x & 3][qmv.y & 3][ch] + y * ref->chromaStride + x;
        short *resi = residual[ch] + blockOffsetY * resChromaStride + blockOffsetX;

        for (int row = 0; row < blockHeight >> 1; row++)
        {
            for (int col = 0; col < blockWidth >> 1; col++)
            {
                resi[col] = (short)orig[col] - (Short)fref[col];
            }

            resi += resChromaStride;
            orig += fencChromaStride;
            fref += ref->chromaStride;
        }
    }
}
