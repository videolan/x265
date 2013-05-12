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
#include <string.h>

using namespace x265;

void MotionEstimate::setSourcePU(int offset, int width, int height)
{
    int size = PartitionFromSizes(width, height);
    sad = primitives.sad[size];
    satd = primitives.satd[size];
    sad_x3 = primitives.sad_x3[size];
    sad_x4 = primitives.sad_x4[size];

    blockWidth = width;
    blockHeight = height;
    blockOffset = offset;

    /* copy block into local buffer */
    pixel *fencblock = fencplanes[0] + offset;
    primitives.cpyblock(width, height, fenc, FENC_STRIDE, fencblock, fencLumaStride);
}

#define COPY1_IF_LT(x,y) if((y)<(x)) (x)=(y);

#define COST_MV_X4_DIR( m0x, m0y, m1x, m1y, m2x, m2y, m3x, m3y, costs )\
{\
    size_t stride = ref->lumaStride;\
    pixel *pix_base = fref + bmv.x + bmv.y * stride;\
    sad_x4(fenc,\
           pix_base + (m0x) + (m0y)*stride,\
           pix_base + (m1x) + (m1y)*stride,\
           pix_base + (m2x) + (m2y)*stride,\
           pix_base + (m3x) + (m3y)*stride,\
           stride, costs);\
    (costs)[0] += mvcost((bmv + MV(m0x,m0y)) << 2);\
    (costs)[1] += mvcost((bmv + MV(m1x,m1y)) << 2);\
    (costs)[2] += mvcost((bmv + MV(m2x,m2y)) << 2);\
    (costs)[3] += mvcost((bmv + MV(m3x,m3y)) << 2);\
}

#define COST_MV_X3_DIR( m0x, m0y, m1x, m1y, m2x, m2y, costs )\
{\
    size_t stride = ref->lumaStride;\
    pixel *pix_base = fref + bmv.x + bmv.y * stride;\
    sad_x3(fenc,\
           pix_base + (m0x) + (m0y)*stride,\
           pix_base + (m1x) + (m1y)*stride,\
           pix_base + (m2x) + (m2y)*stride,\
           stride, costs);\
    (costs)[0] += mvcost((bmv + MV(m0x,m0y)) << 2);\
    (costs)[1] += mvcost((bmv + MV(m1x,m1y)) << 2);\
    (costs)[2] += mvcost((bmv + MV(m2x,m2y)) << 2);\
}

int MotionEstimate::motionEstimate(const MV &qmvp,
                                   int       numCandidates,
                                   const MV *mvc,
                                   int       merange,
                                   MV &      outQMv)
{
    ALIGN_VAR_16(int, costs[16]);
    pixel *fref = ref->lumaPlane[0][0] + blockOffset;

    setMVP(qmvp);

    MV qmvmin = mvmin.toQPel();
    MV qmvmax = mvmax.toQPel();

    // measure SAD cost at MVP
    MV bmv = qmvp.clipped(qmvmin, qmvmax);
    MV bestpre = bmv;
    int bcost = qpelSad(bmv);

    /* measure SAD cost at each candidate */
    for (int i = 0; i < numCandidates; i++)
    {
        MV m = mvc[i].clipped(qmvmin, qmvmax);
        if (m == 0) // zero is measured later
            continue;

        int cost = qpelSad(m) + mvcost(m);
        if (cost < bcost)
        {
            bcost = cost;
            bestpre = m;
        }
    }

    if (bmv != 0)
    {
        int cost = fpelSad(fref, 0) + mvcost(0);
        if (cost < bcost)
        {
            bcost = cost;
            bestpre = 0;
        }
    }
    /* remember the best SAD cost of motion candidates */
    int bprecost = bcost;

    /* Measure full pel SAD at MVP */
    bmv = bmv.roundToFPel();
    bcost = fpelSad(fref, bmv) + mvcost(bmv.toQPel());

    int meMethod = 0;
    switch (meMethod)
    {
    case 0:
    {
        /* diamond search, radius 1 */
        bcost <<= 4;
        int i = merange;
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
    /* TODO: add UMH and ESA modes */
    }

    if (bprecost < bcost)
        bmv = bestpre;
    else
        bmv = bmv.toQPel();    // promote bmv to qpel

    /* bmv has the best full pel motion vector found by SAD search or motion candidates */

    // TODO: add chroma satd costs
    bcost = qpelSatd(bmv) + mvcost(bmv); // remeasure BMV using SATD

    /* HPEL refinement followed by QPEL refinement */

    bcost <<= 4;
    int16_t res = 2;
    do
    {
        /* TODO: include chroma satd costs */
        MV mv = bmv + MV(0, -res);
        int cost = qpelSatd(mv) + mvcost(mv);
        COPY1_IF_LT(bcost, (cost<<4)+1);

        mv = bmv + MV(0,  res);
        cost = qpelSatd(mv) + mvcost(mv);
        COPY1_IF_LT(bcost, (cost<<4)+3);

        mv = bmv + MV(-res, 0);
        cost = qpelSatd(mv) + mvcost(mv);
        COPY1_IF_LT(bcost, (cost<<4)+4);

        mv = bmv + MV(res,  0);
        cost = qpelSatd(mv) + mvcost(mv);
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

    x264_cpu_emms();
    outQMv = bmv;
    return bcost >> 4;
}
