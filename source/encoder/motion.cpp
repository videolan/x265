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
#include "common.h"
#include "motion.h"
#include "x265.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>

#if _MSC_VER
#pragma warning(disable: 4127) // conditional  expression is constant (macros use this construct)
#endif

using namespace x265;

static int size_scale[NUM_PARTITIONS];
#define SAD_THRESH(v) (bcost < (((v >> 4) * size_scale[partEnum])))

static void init_scales(void)
{
    int dims[] = { 4, 8, 12, 16, 24, 32, 48, 64 };

    int i = 0;

    for (size_t h = 0; h < sizeof(dims) / sizeof(int); h++)
    {
        for (size_t w = 0; w < sizeof(dims) / sizeof(int); w++)
        {
            size_scale[i++] = (dims[h] * dims[w]) >> 4;
        }
    }
}

void MotionEstimate::setSourcePU(int offset, int width, int height)
{
    if (size_scale[0] == 0)
        init_scales();

#if SUBSAMPLE_SAD
    subsample = 0;
    if (height > 12)
    {
        partEnum = PartitionFromSizes(width, height / 2);
        sad = primitives.sad[partEnum];
        sad_x3 = primitives.sad_x3[partEnum];
        sad_x4 = primitives.sad_x4[partEnum];
        subsample = 1;

        partEnum = PartitionFromSizes(width, height);
        satd = primitives.satd[partEnum];
    }
    else
#endif // if SUBSAMPLE_SAD
    {
        partEnum = PartitionFromSizes(width, height);
        sad = primitives.sad[partEnum];
        satd = primitives.satd[partEnum];
        sad_x3 = primitives.sad_x3[partEnum];
        sad_x4 = primitives.sad_x4[partEnum];
    }

    blockOffset = offset;

    /* copy block into local buffer */
    pixel *fencblock = fencplanes[0] + offset;
    primitives.cpyblock(width, height, fenc, FENC_STRIDE, fencblock, fencLumaStride);
#if SUBSAMPLE_SAD
    if (subsample)
    {
        /* Make sub-sampled copy of fenc block at `fencSad' for SAD calculations */
        fencSad = fenc + height * FENC_STRIDE;
        primitives.cpyblock(width, height / 2, fencSad, FENC_STRIDE, fenc, FENC_STRIDE * 2);
    }
    else
    {
        /* Else use non-sub-sampled fenc block for SAD */
        fencSad = fenc;
    }
#endif // if SUBSAMPLE_SAD
}

/* radius 2 hexagon. repeated entries are to avoid having to compute mod6 every time. */
static const MV hex2[8] = { MV(-1, -2), MV(-2, 0), MV(-1, 2), MV(1, 2), MV(2, 0), MV(1, -2), MV(-1, -2), MV(-2, 0) };
static const uint8_t mod6m1[8] = { 5, 0, 1, 2, 3, 4, 5, 0 };  /* (x-1)%6 */
static const MV square1[9] = { MV(0, 0), MV(0, -1), MV(0, 1), MV(-1, 0), MV(1, 0), MV(-1, -1), MV(-1, 1), MV(1, -1), MV(1, 1) };
static const MV hex4[16] =
{
    MV(0, -4),  MV(0, 4),  MV(-2, -3), MV(2, -3),
    MV(-4, -2), MV(4, -2), MV(-4, -1), MV(4, -1),
    MV(-4, 0),  MV(4, 0),  MV(-4, 1),  MV(4, 1),
    MV(-4, 2), MV(4, 2), MV(-2, 3), MV(2, 3),
};
static const MV offsets[] =
{
    MV(-1, 0), MV(0, -1),
    MV(-1, -1), MV(1, -1),
    MV(-1, 0), MV(1, 0),
    MV(-1, 1), MV(-1, -1),
    MV(1, -1), MV(1, 1),
    MV(-1, 0), MV(0, 1),
    MV(-1, 1), MV(1, 1),
    MV(1, 0), MV(0, 1),
}; // offsets for Two Point Search


/* sum of absolute differences between MV candidates */
static inline int x265_predictor_difference(const MV *mvc, intptr_t numCandidates)
{
    int sum = 0;

    for (int i = 0; i < numCandidates - 1; i++)
    {
        sum += abs(mvc[i].x - mvc[i + 1].x)
            +  abs(mvc[i].y - mvc[i + 1].y);
    }

    return sum;
}

#define COST_MV_PT_DIST(mx, my, point, dist) \
    do \
    { \
        MV tmv(mx, my); \
        int cost = fpelSad(fref, tmv) + mvcost(tmv << 2); \
        if (cost < bcost) { \
            bcost = cost; \
            bmv = tmv; \
            bPointNr = point; \
            bDistance = dist; \
        } \
    } while (0)

#define COST_MV(mx, my) \
    do \
    { \
        MV _tmv(mx, my); \
        int cost = fpelSad(fref, _tmv) + mvcost(_tmv << 2); \
        COPY2_IF_LT(bcost, cost, bmv, _tmv); \
    } while (0)

#if SUBSAMPLE_SAD

#define COST_MV_X3_DIR(m0x, m0y, m1x, m1y, m2x, m2y, costs) \
    { \
        pixel *pix_base = fref + bmv.x + bmv.y * stride; \
        sad_x3(fencSad, \
               pix_base + (m0x) + (m0y) * stride, \
               pix_base + (m1x) + (m1y) * stride, \
               pix_base + (m2x) + (m2y) * stride, \
               sadStride, costs); \
        (costs)[0] = ((costs)[0] << subsample) + mvcost((bmv + MV(m0x, m0y)) << 2); \
        (costs)[1] = ((costs)[1] << subsample) + mvcost((bmv + MV(m1x, m1y)) << 2); \
        (costs)[2] = ((costs)[2] << subsample) + mvcost((bmv + MV(m2x, m2y)) << 2); \
    }

#define COST_MV_PT_DIST_X4(m0x, m0y, p0, d0, m1x, m1y, p1, d1, m2x, m2y, p2, d2, m3x, m3y, p3, d3) \
    { \
        sad_x4(fencSad, \
               fref + (m0x) + (m0y) * stride, \
               fref + (m1x) + (m1y) * stride, \
               fref + (m2x) + (m2y) * stride, \
               fref + (m3x) + (m3y) * stride, \
               sadStride, costs); \
        costs[0] = (costs[0] << subsample) + mvcost(MV(m0x, m0y) << 2); \
        costs[1] = (costs[1] << subsample) + mvcost(MV(m1x, m1y) << 2); \
        costs[2] = (costs[2] << subsample) + mvcost(MV(m2x, m2y) << 2); \
        costs[3] = (costs[3] << subsample) + mvcost(MV(m3x, m3y) << 2); \
        COPY4_IF_LT(bcost, costs[0], bmv, MV(m0x, m0y), bPointNr, p0, bDistance, d0); \
        COPY4_IF_LT(bcost, costs[1], bmv, MV(m1x, m1y), bPointNr, p1, bDistance, d1); \
        COPY4_IF_LT(bcost, costs[2], bmv, MV(m2x, m2y), bPointNr, p2, bDistance, d2); \
        COPY4_IF_LT(bcost, costs[3], bmv, MV(m3x, m3y), bPointNr, p3, bDistance, d3); \
    }

#define COST_MV_X4(m0x, m0y, m1x, m1y, m2x, m2y, m3x, m3y) \
    { \
        sad_x4(fencSad, \
               pix_base + (m0x) + (m0y) * stride, \
               pix_base + (m1x) + (m1y) * stride, \
               pix_base + (m2x) + (m2y) * stride, \
               pix_base + (m3x) + (m3y) * stride, \
               sadStride, costs); \
        costs[0] = (costs[0] << subsample) + mvcost((omv + MV(m0x, m0y)) << 2); \
        costs[1] = (costs[1] << subsample) + mvcost((omv + MV(m1x, m1y)) << 2); \
        costs[2] = (costs[2] << subsample) + mvcost((omv + MV(m2x, m2y)) << 2); \
        costs[3] = (costs[3] << subsample) + mvcost((omv + MV(m3x, m3y)) << 2); \
        COPY2_IF_LT(bcost, costs[0], bmv, omv + MV(m0x, m0y)); \
        COPY2_IF_LT(bcost, costs[1], bmv, omv + MV(m1x, m1y)); \
        COPY2_IF_LT(bcost, costs[2], bmv, omv + MV(m2x, m2y)); \
        COPY2_IF_LT(bcost, costs[3], bmv, omv + MV(m3x, m3y)); \
    }

#define COST_MV_X4_DIR(m0x, m0y, m1x, m1y, m2x, m2y, m3x, m3y, costs) \
    { \
        pixel *pix_base = fref + bmv.x + bmv.y * stride; \
        sad_x4(fencSad, \
               pix_base + (m0x) + (m0y) * stride, \
               pix_base + (m1x) + (m1y) * stride, \
               pix_base + (m2x) + (m2y) * stride, \
               pix_base + (m3x) + (m3y) * stride, \
               sadStride, costs); \
        (costs)[0] = ((costs)[0] << subsample) + mvcost((bmv + MV(m0x, m0y)) << 2); \
        (costs)[1] = ((costs)[1] << subsample) + mvcost((bmv + MV(m1x, m1y)) << 2); \
        (costs)[2] = ((costs)[2] << subsample) + mvcost((bmv + MV(m2x, m2y)) << 2); \
        (costs)[3] = ((costs)[3] << subsample) + mvcost((bmv + MV(m3x, m3y)) << 2); \
    }

#else // if SUBSAMPLE_SAD

#define COST_MV_X3_DIR(m0x, m0y, m1x, m1y, m2x, m2y, costs) \
    { \
        pixel *pix_base = fref + bmv.x + bmv.y * stride; \
        sad_x3(fenc, \
               pix_base + (m0x) + (m0y) * stride, \
               pix_base + (m1x) + (m1y) * stride, \
               pix_base + (m2x) + (m2y) * stride, \
               stride, costs); \
        (costs)[0] += mvcost((bmv + MV(m0x, m0y)) << 2); \
        (costs)[1] += mvcost((bmv + MV(m1x, m1y)) << 2); \
        (costs)[2] += mvcost((bmv + MV(m2x, m2y)) << 2); \
    }

#define COST_MV_PT_DIST_X4(m0x, m0y, p0, d0, m1x, m1y, p1, d1, m2x, m2y, p2, d2, m3x, m3y, p3, d3) \
    { \
        sad_x4(fenc, \
               fref + (m0x) + (m0y) * stride, \
               fref + (m1x) + (m1y) * stride, \
               fref + (m2x) + (m2y) * stride, \
               fref + (m3x) + (m3y) * stride, \
               stride, costs); \
        costs[0] += mvcost(MV(m0x, m0y) << 2); \
        costs[1] += mvcost(MV(m1x, m1y) << 2); \
        costs[2] += mvcost(MV(m2x, m2y) << 2); \
        costs[3] += mvcost(MV(m3x, m3y) << 2); \
        COPY4_IF_LT(bcost, costs[0], bmv, MV(m0x, m0y), bPointNr, p0, bDistance, d0); \
        COPY4_IF_LT(bcost, costs[1], bmv, MV(m1x, m1y), bPointNr, p1, bDistance, d1); \
        COPY4_IF_LT(bcost, costs[2], bmv, MV(m2x, m2y), bPointNr, p2, bDistance, d2); \
        COPY4_IF_LT(bcost, costs[3], bmv, MV(m3x, m3y), bPointNr, p3, bDistance, d3); \
    }

#define COST_MV_X4(m0x, m0y, m1x, m1y, m2x, m2y, m3x, m3y) \
    { \
        sad_x4(fenc, \
               pix_base + (m0x) + (m0y) * stride, \
               pix_base + (m1x) + (m1y) * stride, \
               pix_base + (m2x) + (m2y) * stride, \
               pix_base + (m3x) + (m3y) * stride, \
               stride, costs); \
        costs[0] += mvcost((omv + MV(m0x, m0y)) << 2); \
        costs[1] += mvcost((omv + MV(m1x, m1y)) << 2); \
        costs[2] += mvcost((omv + MV(m2x, m2y)) << 2); \
        costs[3] += mvcost((omv + MV(m3x, m3y)) << 2); \
        COPY2_IF_LT(bcost, costs[0], bmv, omv + MV(m0x, m0y)); \
        COPY2_IF_LT(bcost, costs[1], bmv, omv + MV(m1x, m1y)); \
        COPY2_IF_LT(bcost, costs[2], bmv, omv + MV(m2x, m2y)); \
        COPY2_IF_LT(bcost, costs[3], bmv, omv + MV(m3x, m3y)); \
    }

#define COST_MV_X4_DIR(m0x, m0y, m1x, m1y, m2x, m2y, m3x, m3y, costs) \
    { \
        pixel *pix_base = fref + bmv.x + bmv.y * stride; \
        sad_x4(fenc, \
               pix_base + (m0x) + (m0y) * stride, \
               pix_base + (m1x) + (m1y) * stride, \
               pix_base + (m2x) + (m2y) * stride, \
               pix_base + (m3x) + (m3y) * stride, \
               stride, costs); \
        (costs)[0] += mvcost((bmv + MV(m0x, m0y)) << 2); \
        (costs)[1] += mvcost((bmv + MV(m1x, m1y)) << 2); \
        (costs)[2] += mvcost((bmv + MV(m2x, m2y)) << 2); \
        (costs)[3] += mvcost((bmv + MV(m3x, m3y)) << 2); \
    }
#endif // if SUBSAMPLE_SAD

#define DIA1_ITER(mx, my) \
    { \
        omv.x = mx; omv.y = my; \
        COST_MV_X4(0, -1, 0, 1, -1, 0, 1, 0); \
    }

#define CROSS(start, x_max, y_max) \
    { \
        int16_t i = start; \
        if ((x_max) <= X265_MIN(mvmax.x - omv.x, omv.x - mvmin.x)) \
            for (; i < (x_max) - 2; i += 4) { \
                COST_MV_X4(i, 0, -i, 0, i + 2, 0, -i - 2, 0); } \
        for (; i < (x_max); i += 2) \
        { \
            if (omv.x + i <= mvmax.x) \
                COST_MV(omv.x + i, omv.y); \
            if (omv.x - i >= mvmin.x) \
                COST_MV(omv.x - i, omv.y); \
        } \
        i = start; \
        if ((y_max) <= X265_MIN(mvmax.y - omv.y, omv.y - mvmin.y)) \
            for (; i < (y_max) - 2; i += 4) { \
                COST_MV_X4(0, i, 0, -i, 0, i + 2, 0, -i - 2); } \
        for (; i < (y_max); i += 2) \
        { \
            if (omv.y + i <= mvmax.y) \
                COST_MV(omv.x, omv.y + i); \
            if (omv.y - i >= mvmin.y) \
                COST_MV(omv.x, omv.y - i); \
        } \
    }

int MotionEstimate::motionEstimate(const MV &qmvp,
                                   int       numCandidates,
                                   const MV *mvc,
                                   int       merange,
                                   MV &      outQMv)
{
    ALIGN_VAR_16(int, costs[16]);
    size_t stride = ref->lumaStride;
    pixel *fref = ref->lumaPlane[0][0] + blockOffset;

#if SUBSAMPLE_SAD
    sadStride = ref->lumaStride << subsample;
#endif

    setMVP(qmvp);

    MV qmvmin = mvmin.toQPel();
    MV qmvmax = mvmax.toQPel();

    // measure SAD cost at clipped QPEL MVP
    MV pmv = qmvp.clipped(qmvmin, qmvmax);
    int bprecost = qpelSad(pmv); // ignore MVD cost for clipped MVP
    MV bestpre = pmv;

    /* re-measure full pel rounded MVP with SAD as search start point */
    MV bmv = pmv.roundToFPel();
    int bcost = pmv.isSubpel() ? fpelSad(fref, bmv) + mvcost(bmv << 2) : bprecost;

    // measure SAD cost at MV(0) if MVP is not zero
    if (pmv.notZero())
    {
#if SUBSAMPLE_SAD
        int cost = (sad(fencSad, FENC_STRIDE, fref, sadStride) << subsample) + mvcost(0);
#else
        int cost = sad(fenc, FENC_STRIDE, fref, stride) + mvcost(0);
#endif
        if (cost < bcost)
        {
            bcost = cost;
            bmv = 0;
        }
    }

    // measure SAD cost at each QPEL motion vector candidate
    for (int i = 0; i < numCandidates; i++)
    {
        MV m = mvc[i].clipped(qmvmin, qmvmax);
        if (m.notZero() && m != pmv && m != bestpre) // check already measured
        {
            int cost = qpelSad(m) + mvcost(m);
            if (cost < bprecost)
            {
                bprecost = cost;
                bestpre = m;
            }
        }
    }

    pmv = pmv.roundToFPel();
    MV omv = bmv;  // current search origin or starting point

    switch (searchMethod)
    {
    case X265_DIA_SEARCH:
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

    case X265_HEX_SEARCH:
    {
me_hex2:
        /* hexagon search, radius 2 */
#if 0
        for (int i = 0; i < merange / 2; i++)
        {
            omv = bmv;
            COST_MV(omv.x - 2, omv.y);
            COST_MV(omv.x - 1, omv.y + 2);
            COST_MV(omv.x + 1, omv.y + 2);
            COST_MV(omv.x + 2, omv.y);
            COST_MV(omv.x + 1, omv.y - 2);
            COST_MV(omv.x - 1, omv.y - 2);
            if (omv == bmv)
                break;
            if (!bmv.checkRange(mvmin, mvmax))
                break;
        }

#else // if 0
      /* equivalent to the above, but eliminates duplicate candidates */
        COST_MV_X3_DIR(-2, 0, -1, 2,  1, 2, costs);
        COST_MV_X3_DIR(2, 0,  1, -2, -1, -2, costs + 3);
        bcost <<= 3;
        COPY1_IF_LT(bcost, (costs[0] << 3) + 2);
        COPY1_IF_LT(bcost, (costs[1] << 3) + 3);
        COPY1_IF_LT(bcost, (costs[2] << 3) + 4);
        COPY1_IF_LT(bcost, (costs[3] << 3) + 5);
        COPY1_IF_LT(bcost, (costs[4] << 3) + 6);
        COPY1_IF_LT(bcost, (costs[5] << 3) + 7);

        if (bcost & 7)
        {
            int dir = (bcost & 7) - 2;
            bmv += hex2[dir + 1];

            /* half hexagon, not overlapping the previous iteration */
            for (int i = (merange >> 1) - 1; i > 0 && bmv.checkRange(mvmin, mvmax); i--)
            {
                COST_MV_X3_DIR(hex2[dir + 0].x, hex2[dir + 0].y,
                               hex2[dir + 1].x, hex2[dir + 1].y,
                               hex2[dir + 2].x, hex2[dir + 2].y,
                               costs);
                bcost &= ~7;
                COPY1_IF_LT(bcost, (costs[0] << 3) + 1);
                COPY1_IF_LT(bcost, (costs[1] << 3) + 2);
                COPY1_IF_LT(bcost, (costs[2] << 3) + 3);
                if (!(bcost & 7))
                    break;
                dir += (bcost & 7) - 2;
                dir = mod6m1[dir + 1];
                bmv += hex2[dir + 1];
            }
        }
        bcost >>= 3;
#endif // if 0

        /* square refine */
        int dir = 0;
        COST_MV_X4_DIR(0, -1,  0, 1, -1, 0, 1, 0, costs);
        COPY2_IF_LT(bcost, costs[0], dir, 1);
        COPY2_IF_LT(bcost, costs[1], dir, 2);
        COPY2_IF_LT(bcost, costs[2], dir, 3);
        COPY2_IF_LT(bcost, costs[3], dir, 4);
        COST_MV_X4_DIR(-1, -1, -1, 1, 1, -1, 1, 1, costs);
        COPY2_IF_LT(bcost, costs[0], dir, 5);
        COPY2_IF_LT(bcost, costs[1], dir, 6);
        COPY2_IF_LT(bcost, costs[2], dir, 7);
        COPY2_IF_LT(bcost, costs[3], dir, 8);
        bmv += square1[dir];
        break;
    }

    case X265_UMH_SEARCH:
    {
        int ucost1, ucost2;
        int16_t cross_start = 1;
        assert(PARTITION_4x4 != partEnum);

        /* refine predictors */
        omv = bmv;
        pixel *pix_base = fref + omv.x + omv.y * stride;
        ucost1 = bcost;
        DIA1_ITER(pmv.x, pmv.y);
        if (pmv.notZero())
            DIA1_ITER(0, 0);

        ucost2 = bcost;
        if (bmv.notZero() && bmv != pmv)
            DIA1_ITER(bmv.x, bmv.y);
        if (bcost == ucost2)
            cross_start = 3;

        /* Early Termination */
        omv = bmv;
        if (bcost == ucost2 && SAD_THRESH(2000))
        {
            COST_MV_X4(0, -2, -1, -1, 1, -1, -2, 0);
            COST_MV_X4(2, 0, -1, 1, 1, 1,  0, 2);
            if (bcost == ucost1 && SAD_THRESH(500))
                break;
            if (bcost == ucost2)
            {
                int16_t range = (int16_t)(merange >> 1) | 1;
                CROSS(3, range, range);
                COST_MV_X4(-1, -2, 1, -2, -2, -1, 2, -1);
                COST_MV_X4(-2, 1, 2, 1, -1, 2, 1, 2);
                if (bcost == ucost2)
                    break;
                cross_start = range + 2;
            }
        }

        // TODO: Need to study x264's logic for building mvc list to understand why they
        //       have special cases here for 16x16, and whether they apply to HEVC CTU

        // adaptive search range based on mvc variability
        if (numCandidates)
        {
            /* range multipliers based on casual inspection of some statistics of
             * average distance between current predictor and final mv found by ESA.
             * these have not been tuned much by actual encoding. */
            static const uint8_t range_mul[4][4] =
            {
                { 3, 3, 4, 4 },
                { 3, 4, 4, 4 },
                { 4, 4, 4, 5 },
                { 4, 4, 5, 6 },
            };

            int mvd;
            int sad_ctx, mvd_ctx;
            int denom = 1;

            if (numCandidates == 1)
            {
                if (PARTITION_64x64 == partEnum)
                    /* mvc is probably the same as mvp, so the difference isn't meaningful.
                     * but prediction usually isn't too bad, so just use medium range */
                    mvd = 25;
                else
                    mvd = abs(qmvp.x - mvc[0].x) + abs(qmvp.y - mvc[0].y);
            }
            else
            {
                /* calculate the degree of agreement between predictors. */

                /* in 64x64, mvc includes all the neighbors used to make mvp,
                 * so don't count mvp separately. */

                denom = numCandidates - 1;
                mvd = 0;
                if (partEnum != PARTITION_64x64)
                {
                    mvd = abs(qmvp.x - mvc[0].x) + abs(qmvp.y - mvc[0].y);
                    denom++;
                }
                mvd += x265_predictor_difference(mvc, numCandidates);
            }

            sad_ctx = SAD_THRESH(1000) ? 0
                : SAD_THRESH(2000) ? 1
                : SAD_THRESH(4000) ? 2 : 3;
            mvd_ctx = mvd < 10 * denom ? 0
                : mvd < 20 * denom ? 1
                : mvd < 40 * denom ? 2 : 3;

            merange = (merange * range_mul[mvd_ctx][sad_ctx]) >> 2;
        }

        /* FIXME if the above DIA2/OCT2/CROSS found a new mv, it has not updated omx/omy.
         * we are still centered on the same place as the DIA2. is this desirable? */
        CROSS(cross_start, merange, merange >> 1);
        COST_MV_X4(-2, -2, -2, 2, 2, -2, 2, 2);

        /* hexagon grid */
        omv = bmv;
        const uint16_t *p_cost_omvx = m_cost_mvx + omv.x * 4;
        const uint16_t *p_cost_omvy = m_cost_mvy + omv.y * 4;
        int16_t i = 1;
        do
        {
            if (4 * i > X265_MIN4(mvmax.x - omv.x, omv.x - mvmin.x,
                                  mvmax.y - omv.y, omv.y - mvmin.y))
            {
                for (int j = 0; j < 16; j++)
                {
                    MV mv = omv + (hex4[j] * i);
                    if (mv.checkRange(mvmin, mvmax))
                        COST_MV(mv.x, mv.y);
                }
            }
            else
            {
                int16_t dir = 0;
                pixel *fref_base = fref + omv.x + (omv.y - 4 * i) * stride;
                size_t dy = (size_t)i * stride;
#if SUBSAMPLE_SAD
#define SADS(k, x0, y0, x1, y1, x2, y2, x3, y3) \
    sad_x4(fencSad, \
           fref_base x0 * i + (y0 - 2 * k + 4) * dy, \
           fref_base x1 * i + (y1 - 2 * k + 4) * dy, \
           fref_base x2 * i + (y2 - 2 * k + 4) * dy, \
           fref_base x3 * i + (y3 - 2 * k + 4) * dy, \
           sadStride, costs + 4 * k); \
    fref_base += 2 * dy;
#define ADD_MVCOST(k, x, y) costs[k] = (costs[k] << subsample) + p_cost_omvx[x * 4 * i] + p_cost_omvy[y * 4 * i]
#else
#define SADS(k, x0, y0, x1, y1, x2, y2, x3, y3) \
    sad_x4(fenc, \
           fref_base x0 * i + (y0 - 2 * k + 4) * dy, \
           fref_base x1 * i + (y1 - 2 * k + 4) * dy, \
           fref_base x2 * i + (y2 - 2 * k + 4) * dy, \
           fref_base x3 * i + (y3 - 2 * k + 4) * dy, \
           stride, costs + 4 * k); \
    fref_base += 2 * dy;
#define ADD_MVCOST(k, x, y) costs[k] += p_cost_omvx[x * 4 * i] + p_cost_omvy[y * 4 * i]
#endif // if SUBSAMPLE_SAD
#define MIN_MV(k, x, y)     COPY2_IF_LT(bcost, costs[k], dir, x * 16 + (y & 15))

                SADS(0, +0, -4, +0, +4, -2, -3, +2, -3);
                SADS(1, -4, -2, +4, -2, -4, -1, +4, -1);
                SADS(2, -4, +0, +4, +0, -4, +1, +4, +1);
                SADS(3, -4, +2, +4, +2, -2, +3, +2, +3);
                ADD_MVCOST(0, 0, -4);
                ADD_MVCOST(1, 0, 4);
                ADD_MVCOST(2, -2, -3);
                ADD_MVCOST(3, 2, -3);
                ADD_MVCOST(4, -4, -2);
                ADD_MVCOST(5, 4, -2);
                ADD_MVCOST(6, -4, -1);
                ADD_MVCOST(7, 4, -1);
                ADD_MVCOST(8, -4, 0);
                ADD_MVCOST(9, 4, 0);
                ADD_MVCOST(10, -4, 1);
                ADD_MVCOST(11, 4, 1);
                ADD_MVCOST(12, -4, 2);
                ADD_MVCOST(13, 4, 2);
                ADD_MVCOST(14, -2, 3);
                ADD_MVCOST(15, 2, 3);
                MIN_MV(0, 0, -4);
                MIN_MV(1, 0, 4);
                MIN_MV(2, -2, -3);
                MIN_MV(3, 2, -3);
                MIN_MV(4, -4, -2);
                MIN_MV(5, 4, -2);
                MIN_MV(6, -4, -1);
                MIN_MV(7, 4, -1);
                MIN_MV(8, -4, 0);
                MIN_MV(9, 4, 0);
                MIN_MV(10, -4, 1);
                MIN_MV(11, 4, 1);
                MIN_MV(12, -4, 2);
                MIN_MV(13, 4, 2);
                MIN_MV(14, -2, 3);
                MIN_MV(15, 2, 3);
#undef SADS
#undef ADD_MVCOST
#undef MIN_MV
                if (dir)
                {
                    bmv.x = omv.x + i * (dir >> 4);
                    bmv.y = omv.y + i * ((dir << 28) >> 28);
                }
            }
        }
        while (++i <= merange >> 2);
        if (bmv.checkRange(mvmin, mvmax))
            goto me_hex2;
        break;
    }

    case X265_STAR_SEARCH: // Adapted from HM ME
    {
        int bPointNr = 0;
        int bDistance = 0;
        int rounds = 0;

        const int earlyStopRounds = 3;
        for (int16_t dist = 1; dist <= (int16_t)merange; dist *= 2)
        {
            int saved = bcost;
            StarPatternSearch(bmv, bcost, bPointNr, bDistance, dist, omv);

            // Break if we go earlyStopRounds without an improved prediction
            if (bcost < saved)
                rounds = 0;
            else if (++rounds >= earlyStopRounds)
                break;
        }

        if (bDistance == 1)
        {
            // if best distance was only 1, check two missing points.  If no new point is found, stop
            if (bPointNr)
            {
                /* For a given direction 1 to 8, check nearest two outer X pixels
                     X   X
                   X 1 2 3 X
                     4 * 5
                   X 6 7 8 X
                     X   X
                */
                int saved = bcost;
                const MV mv1 = bmv + offsets[(bPointNr - 1) * 2];
                const MV mv2 = bmv + offsets[(bPointNr - 1) * 2 + 1];
                if (mv1.checkRange(mvmin, mvmax))
                {
                    COST_MV(mv1.x, mv1.y);
                }
                if (mv2.checkRange(mvmin, mvmax))
                {
                    COST_MV(mv2.x, mv2.y);
                }
                if (bcost == saved)
                    break;
            }
            else
                break;
        }

        const int rasterDistance = 5;
        if (bDistance > rasterDistance)
        {
            // raster search refinement if original search distance was too big
            MV tmv;
            for (tmv.y = mvmin.y; tmv.y <= mvmax.y; tmv.y += rasterDistance)
            {
                for (tmv.x = mvmin.x; tmv.x <= mvmax.x; tmv.x += rasterDistance)
                {
                    if (tmv.x + (rasterDistance * 3) <= mvmax.x)
                    {
                        pixel *pix_base = fref + tmv.y * stride + tmv.x;
#if SUBSAMPLE_SAD
                        sad_x4(fencSad,
                               pix_base,
                               pix_base + rasterDistance,
                               pix_base + rasterDistance * 2,
                               pix_base + rasterDistance * 3,
                               sadStride, costs);
                        costs[0] = (costs[0] << subsample) + mvcost(tmv << 2);
                        COPY2_IF_LT(bcost, costs[0], bmv, tmv);
                        tmv.x += rasterDistance;
                        costs[1] = (costs[1] << subsample) + mvcost(tmv << 2);
                        COPY2_IF_LT(bcost, costs[1], bmv, tmv);
                        tmv.x += rasterDistance;
                        costs[2] = (costs[2] << subsample) + mvcost(tmv << 2);
                        COPY2_IF_LT(bcost, costs[2], bmv, tmv);
                        tmv.x += rasterDistance;
                        costs[3] = (costs[3] << subsample) + mvcost(tmv << 3);
                        COPY2_IF_LT(bcost, costs[3], bmv, tmv);
#else // if SUBSAMPLE_SAD
                        sad_x4(fenc,
                               pix_base,
                               pix_base + rasterDistance,
                               pix_base + rasterDistance * 2,
                               pix_base + rasterDistance * 3,
                               stride, costs);
                        costs[0] += mvcost(tmv << 2);
                        COPY2_IF_LT(bcost, costs[0], bmv, tmv);
                        tmv.x += rasterDistance;
                        costs[1] += mvcost(tmv << 2);
                        COPY2_IF_LT(bcost, costs[1], bmv, tmv);
                        tmv.x += rasterDistance;
                        costs[2] += mvcost(tmv << 2);
                        COPY2_IF_LT(bcost, costs[2], bmv, tmv);
                        tmv.x += rasterDistance;
                        costs[3] += mvcost(tmv << 3);
                        COPY2_IF_LT(bcost, costs[3], bmv, tmv);
#endif // if SUBSAMPLE_SAD
                    }
                    else
                        COST_MV(tmv.x, tmv.y);
                }
            }
        }

        while (bDistance > 0)
        {
            // center a new search around current best
            omv = bmv;
            bDistance = 0;
            bPointNr = 0;
            for (int16_t dist = 1; dist <= (int16_t)merange; dist *= 2)
            {
                StarPatternSearch(bmv, bcost, bPointNr, bDistance, dist, omv);
            }

            if (bDistance == 1)
            {
                if (bPointNr)
                {
                    /* For a given direction 1 to 8, check nearest 2 outer X pixels
                         X   X
                       X 1 2 3 X
                         4 * 5
                       X 6 7 8 X
                         X   X
                    */
                    const MV mv1 = bmv + offsets[(bPointNr - 1) * 2];
                    const MV mv2 = bmv + offsets[(bPointNr - 1) * 2 + 1];
                    if (mv1.checkRange(mvmin, mvmax))
                    {
                        COST_MV(mv1.x, mv1.y);
                    }
                    if (mv2.checkRange(mvmin, mvmax))
                    {
                        COST_MV(mv2.x, mv2.y);
                    }
                }
                break;
            }
        }
    }
    break;

    default:
        assert(0);
        break;
    }

    if (bprecost < bcost)
        bmv = bestpre;
    else
        bmv = bmv.toQPel(); // promote search bmv to qpel

    /* HPEL square refinement, dir 0 has no offset - remeasures bmv with SATD */
    int bdir = 0;
    bcost = COST_MAX;
    for (int i = 0; i < 9; i++)
    {
        MV mv = bmv + square1[i] * 2;
        int cost = qpelSatd(mv) + mvcost(mv);
        COPY2_IF_LT(bcost, cost, bdir, i);
    }

    bmv += square1[bdir] * 2;

    /* QPEL square refinement, do not remeasure 0 offset */
    bdir = 0;
    for (int i = 1; i < 9; i++)
    {
        MV mv = bmv + square1[i];
        int cost = qpelSatd(mv) + mvcost(mv);
        COPY2_IF_LT(bcost, cost, bdir, i);
    }

    bmv += square1[bdir];

    x265_emms();
    outQMv = bmv;
    return bcost;
}

void MotionEstimate::StarPatternSearch(MV &bmv, int &bcost, int &bPointNr, int &bDistance, int16_t dist, const MV& omv)
{
    ALIGN_VAR_16(int, costs[16]);
    pixel *fref = ref->lumaPlane[0][0] + blockOffset;
    size_t stride = ref->lumaStride;

    if (dist == 1)
    {
        /* bPointNr
              2
            4 * 5
              7
         */
        const int16_t iTop    = omv.y - dist;
        const int16_t iBottom = omv.y + dist;
        const int16_t iLeft   = omv.x - dist;
        const int16_t iRight  = omv.x + dist;

        if (iTop >= mvmin.y && iLeft >= mvmin.x && iRight <= mvmax.x && iBottom <= mvmax.y)
        {
            COST_MV_PT_DIST_X4(omv.x,  iTop,    2, dist,
                               iLeft,  omv.y,   4, dist,
                               iRight, omv.y,   5, dist,
                               omv.x,  iBottom, 7, dist);
        }
        else
        {
            if (iTop >= mvmin.y) // check top
            {
                COST_MV_PT_DIST(omv.x, iTop, 2, dist);
            }
            if (iLeft >= mvmin.x) // check middle left
            {
                COST_MV_PT_DIST(iLeft, omv.y, 4, dist);
            }
            if (iRight <= mvmax.x) // check middle right
            {
                COST_MV_PT_DIST(iRight, omv.y, 5, dist);
            }
            if (iBottom <= mvmax.y) // check bottom
            {
                COST_MV_PT_DIST(omv.x, iBottom, 7, dist);
            }
        }
    }
    else if (dist <= 8)
    {
        /* bPointNr
              2
             1 3
            4 * 5
             6 8
              7
         Points 2, 4, 5, 7 are dist
         Points 1, 3, 6, 8 are dist>>1
         */
        const int16_t iTop      = omv.y - dist;
        const int16_t iBottom   = omv.y + dist;
        const int16_t iLeft     = omv.x - dist;
        const int16_t iRight    = omv.x + dist;
        const int16_t iTop_2    = omv.y - (dist >> 1);
        const int16_t iBottom_2 = omv.y + (dist >> 1);
        const int16_t iLeft_2   = omv.x - (dist >> 1);
        const int16_t iRight_2  = omv.x + (dist >> 1);

        if (iTop >= mvmin.y && iLeft >= mvmin.x &&
            iRight <= mvmax.x && iBottom <= mvmax.y) // check border
        {
            COST_MV_PT_DIST_X4(omv.x,    iTop,      2, dist,
                               iLeft_2,  iTop_2,    1, dist >> 1,
                               iRight_2, iTop_2,    3, dist >> 1,
                               iLeft,    omv.y,     4, dist);
            COST_MV_PT_DIST_X4(iRight,   omv.y,     5, dist,
                               iLeft_2,  iBottom_2, 6, dist >> 1,
                               iRight_2, iBottom_2, 8, dist >> 1,
                               omv.x,    iBottom,   7, dist);
        }
        else // check border for each mv
        {
            if (iTop >= mvmin.y) // check top
            {
                COST_MV_PT_DIST(omv.x, iTop, 2, dist);
            }
            if (iTop_2 >= mvmin.y) // check half top
            {
                if (iLeft_2 >= mvmin.x) // check half left
                {
                    COST_MV_PT_DIST(iLeft_2, iTop_2, 1, (dist >> 1));
                }
                if (iRight_2 <= mvmax.x) // check half right
                {
                    COST_MV_PT_DIST(iRight_2, iTop_2, 3, (dist >> 1));
                }
            } // check half top
            if (iLeft >= mvmin.x) // check left
            {
                COST_MV_PT_DIST(iLeft, omv.y, 4, dist);
            }
            if (iRight <= mvmax.x) // check right
            {
                COST_MV_PT_DIST(iRight, omv.y, 5, dist);
            }
            if (iBottom_2 <= mvmax.y) // check half bottom
            {
                if (iLeft_2 >= mvmin.x) // check half left
                {
                    COST_MV_PT_DIST(iLeft_2, iBottom_2, 6, (dist >> 1));
                }
                if (iRight_2 <= mvmax.x) // check half right
                {
                    COST_MV_PT_DIST(iRight_2, iBottom_2, 8, (dist >> 1));
                }
            } // check half bottom
            if (iBottom <= mvmax.y) // check bottom
            {
                COST_MV_PT_DIST(omv.x, iBottom, 7, dist);
            }
        } // check border for each mv
    }
    else
    {
        const int16_t iTop    = omv.y - dist;
        const int16_t iBottom = omv.y + dist;
        const int16_t iLeft   = omv.x - dist;
        const int16_t iRight  = omv.x + dist;

        if (iTop >= mvmin.y && iLeft >= mvmin.x &&
            iRight <= mvmax.x && iBottom <= mvmax.y) // check border
        {
            /* index
                  0
                  3
                  2
                  1
          0 3 2 1 * 1 2 3 0
                  1
                  2
                  3
                  0
            */

            COST_MV_PT_DIST_X4(omv.x,  iTop,    0, dist,
                               iLeft,  omv.y,   0, dist,
                               iRight, omv.y,   0, dist,
                               omv.x,  iBottom, 0, dist);

            for (int16_t index = 1; index < 4; index++)
            {
                int16_t iPosYT = iTop    + ((dist >> 2) * index);
                int16_t iPosYB = iBottom - ((dist >> 2) * index);
                int16_t iPosXL = omv.x - ((dist >> 2) * index);
                int16_t iPosXR = omv.x + ((dist >> 2) * index);

                COST_MV_PT_DIST_X4(iPosXL, iPosYT, 0, dist,
                                   iPosXR, iPosYT, 0, dist,
                                   iPosXL, iPosYB, 0, dist,
                                   iPosXR, iPosYB, 0, dist);
            }
        }
        else // check border for each mv
        {
            if (iTop >= mvmin.y) // check top
            {
                COST_MV_PT_DIST(omv.x, iTop, 0, dist);
            }
            if (iLeft >= mvmin.x) // check left
            {
                COST_MV_PT_DIST(iLeft, omv.y, 0, dist);
            }
            if (iRight <= mvmax.x) // check right
            {
                COST_MV_PT_DIST(iRight, omv.y, 0, dist);
            }
            if (iBottom <= mvmax.y) // check bottom
            {
                COST_MV_PT_DIST(omv.x, iBottom, 0, dist);
            }
            for (int16_t index = 1; index < 4; index++)
            {
                int16_t iPosYT = iTop    + ((dist >> 2) * index);
                int16_t iPosYB = iBottom - ((dist >> 2) * index);
                int16_t iPosXL = omv.x - ((dist >> 2) * index);
                int16_t iPosXR = omv.x + ((dist >> 2) * index);

                if (iPosYT >= mvmin.y) // check top
                {
                    if (iPosXL >= mvmin.x) // check left
                    {
                        COST_MV_PT_DIST(iPosXL, iPosYT, 0, dist);
                    }
                    if (iPosXR <= mvmax.x) // check right
                    {
                        COST_MV_PT_DIST(iPosXR, iPosYT, 0, dist);
                    }
                } // check top
                if (iPosYB <= mvmax.y) // check bottom
                {
                    if (iPosXL >= mvmin.x) // check left
                    {
                        COST_MV_PT_DIST(iPosXL, iPosYB, 0, dist);
                    }
                    if (iPosXR <= mvmax.x) // check right
                    {
                        COST_MV_PT_DIST(iPosXR, iPosYB, 0, dist);
                    }
                } // check bottom
            } // for ...
        } // check border for each mv
    } // dist > 8
}
