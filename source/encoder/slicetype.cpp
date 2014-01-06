/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Gopu Govindaswamy <gopu@multicorewareinc.com>
 *          Steve Borho <steve@borho.org>
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

#include "TLibCommon/TComRom.h"
#include "TLibCommon/TComPic.h"
#include "primitives.h"
#include "lowres.h"

#include "TLibEncoder/TEncCfg.h"
#include "slicetype.h"
#include "motion.h"
#include "mv.h"
#include "ratecontrol.h"

#define LOWRES_COST_MASK  ((1 << 14) - 1)
#define LOWRES_COST_SHIFT 14

// short history:
//
// This file was originally borrowed from x264 source tree circa Dec 4, 2012
// with x264 bug fixes applied from Dec 11th and Jan 8th 2013.  But without
// taking any of the threading changes because we will eventually use the x265
// thread pool and wavefront processing.  It has since been adapted for x265
// coding style and HEVC encoding structures

using namespace x265;

static inline int16_t median(int16_t a, int16_t b, int16_t c)
{
    int16_t t = (a - b) & ((a - b) >> 31);

    a -= t;
    b += t;
    b -= (b - c) & ((b - c) >> 31);
    b += (a - b) & ((a - b) >> 31);
    return b;
}

static inline void median_mv(MV &dst, MV a, MV b, MV c)
{
    dst.x = median(a.x, b.x, c.x);
    dst.y = median(a.y, b.y, c.y);
}

Lookahead::Lookahead(TEncCfg *_cfg, ThreadPool* pool) : WaveFront(pool)
{
    this->cfg = _cfg;
    numDecided = 0;
    lastKeyframe = -cfg->param.keyframeMax;
    lastNonB = NULL;
    widthInCU = ((cfg->param.sourceWidth / 2) + X265_LOWRES_CU_SIZE - 1) >> X265_LOWRES_CU_BITS;
    heightInCU = ((cfg->param.sourceHeight / 2) + X265_LOWRES_CU_SIZE - 1) >> X265_LOWRES_CU_BITS;
    wbuffer[0] = wbuffer[1] = wbuffer[2] = wbuffer[3] = 0;

    lhrows = new LookaheadRow[heightInCU];
    for (int i = 0; i < heightInCU; i++)
    {
        lhrows[i].widthInCU = widthInCU;
        lhrows[i].heightInCU = heightInCU;
    }

    scratch = (int*)x265_malloc(widthInCU * sizeof(int));
}

Lookahead::~Lookahead()
{
}

void Lookahead::init()
{
    if (!WaveFront::init(heightInCU))
    {
        m_pool = NULL;
    }
    else
    {
        WaveFront::enableAllRows();
    }
}

void Lookahead::destroy()
{
    delete[] lhrows;

    // these two queues will be empty, unless the encode was aborted
    while (!inputQueue.empty())
    {
        TComPic* pic = inputQueue.popFront();
        pic->destroy(cfg->param.bframes);
        delete pic;
    }

    while (!outputQueue.empty())
    {
        TComPic* pic = outputQueue.popFront();
        pic->destroy(cfg->param.bframes);
        delete pic;
    }

    for (int i = 0; i < 4; i++)
    {
        x265_free(wbuffer[i]);
    }
    X265_FREE(scratch);
}

void Lookahead::addPicture(TComPic *pic, int sliceType)
{
    TComPicYuv *orig = pic->getPicYuvOrg();

    pic->m_lowres.init(orig, pic->getSlice()->getPOC(), sliceType, cfg->param.bframes);
    inputQueue.pushBack(*pic);

    if (!wbuffer[0] && cfg->param.bEnableWeightedPred)
    {
        paddedLines = pic->m_lowres.lines + 2 * orig->getLumaMarginY();
        int padoffset = pic->m_lowres.lumaStride * orig->getLumaMarginY() + orig->getLumaMarginX();

        /* allocate weighted lowres buffers */
        for (int i = 0; i < 4; i++)
        {
            wbuffer[i] = (pixel*)x265_malloc(sizeof(pixel) * (pic->m_lowres.lumaStride * paddedLines));
            weightedRef.lowresPlane[i] = wbuffer[i] + padoffset;
            weightedRef.lumaStride = pic->m_lowres.lumaStride;
            weightedRef.isLowres = true;
        }

        weightedRef.fpelPlane = weightedRef.lowresPlane[0];
        weightedRef.isWeighted = false;
    }
    if (inputQueue.size() >= cfg->param.lookaheadDepth)
        slicetypeDecide();
}

void Lookahead::flush()
{
    if (!inputQueue.empty())
        slicetypeDecide();
}

// Called by RateControl to get the estimated SATD cost for a given picture.
// It assumes dpb->prepareEncode() has already been called for the picture and
// all the references are established
int Lookahead::getEstimatedPictureCost(TComPic *pic)
{
    // POC distances to each reference
    int d0, d1, p0, p1, b;
    int poc = pic->getSlice()->getPOC();
    int l0poc = pic->getSlice()->getRefPOC(REF_PIC_LIST_0, 0);
    int l1poc = pic->getSlice()->getRefPOC(REF_PIC_LIST_1, 0);

    switch (pic->getSlice()->getSliceType())
    {
    case I_SLICE:
        frames[0] = &pic->m_lowres;
        p0 = p1 = b = 0;
        break;
    case P_SLICE:
        d0 = poc - l0poc;
        frames[0] = &pic->getSlice()->getRefPic(REF_PIC_LIST_0, 0)->m_lowres;
        frames[d0] = &pic->m_lowres;
        p0 = 0;
        p1 = d0;
        b = d0;
        break;
    case B_SLICE:
        d0 = poc - l0poc;
        if (l1poc > poc)
        {
            // L1 reference is truly in the future
            d1 = l1poc - poc;
            frames[0] = &pic->getSlice()->getRefPic(REF_PIC_LIST_0, 0)->m_lowres;
            frames[d0] = &pic->m_lowres;
            frames[d0 + d1] = &pic->getSlice()->getRefPic(REF_PIC_LIST_1, 0)->m_lowres;
            p0 = 0;
            p1 = d0 + d1;
            b = d0;
        }
        else
        {
            frames[0] = &pic->getSlice()->getRefPic(REF_PIC_LIST_0, 0)->m_lowres;
            frames[d0] = &pic->m_lowres;
            p0 = 0;
            p1 = d0;
            b = d0;
        }
        break;
    default:
        return -1;
    }

    estimateFrameCost(p0, p1, b, false);
    if (cfg->param.rc.cuTree)
    {
        pic->m_lowres.satdCost = frameCostRecalculate(frames, p0, p1, b);
        return pic->m_lowres.satdCost;
    }
    if (cfg->param.rc.aqMode)
        pic->m_lowres.satdCost = pic->m_lowres.costEstAq[b - p0][p1 - b];
    else
        pic->m_lowres.satdCost = pic->m_lowres.costEst[b - p0][p1 - b];
    return pic->m_lowres.satdCost;
}

uint32_t Lookahead::weightCostLuma(int b, pixel *src, wpScalingParam *w)
{
    Lowres *fenc = frames[b];
    int stride = fenc->lumaStride;

    if (w)
    {
        int offset = w->inputOffset << (X265_DEPTH - 8);
        int scale = w->inputWeight;
        int denom = w->log2WeightDenom;
        int correction = IF_INTERNAL_PREC - X265_DEPTH;

        // Adding (IF_INTERNAL_PREC - X265_DEPTH) to cancel effect of pixel to short conversion inside the primitive
        primitives.weight_pp(src, weightedRef.fpelPlane, stride, stride, stride, fenc->lines,
                             scale, (1 << (denom - 1 + correction)), denom + correction, offset);
        src = weightedRef.fpelPlane;
    }

    uint32_t cost = 0;
    int pixoff = 0;
    int mb = 0;

    for (int y = 0; y < fenc->lines; y += 8, pixoff = y * stride)
    {
        for (int x = 0; x < fenc->width; x += 8, mb++, pixoff += 8)
        {
            int satd = primitives.satd[LUMA_8x8](src + pixoff, stride, fenc->fpelPlane + pixoff, stride);
            cost += X265_MIN(satd, fenc->intraCost[mb]);
        }
    }

    x265_emms();
    return cost;
}

void Lookahead::weightsAnalyse(int b, int p0)
{
    wpScalingParam w;

    Lowres *fenc, *ref;

    fenc = frames[b];
    ref  = frames[p0];
    int deltaIndex = fenc->frameNum - ref->frameNum;

    /* epsilon is chosen to require at least a numerator of 127 (with denominator = 128) */
    const float epsilon = 1.f / 128.f;
    float guessScale, fencMean, refMean;
    guessScale = sqrtf((float)fenc->wp_ssd[0] / ref->wp_ssd[0]);
    fencMean = (float)fenc->wp_sum[0] / (fenc->lines * fenc->width) / (1 << (X265_DEPTH - 8));
    refMean  = (float)ref->wp_sum[0] / (fenc->lines * fenc->width) / (1 << (X265_DEPTH - 8));

    /* Early termination */
    if (fabsf(refMean - fencMean) < 0.5f && fabsf(1.f - guessScale) < epsilon)
        return;

    /* Don't check chroma in lookahead, or if there wasn't a luma weight. */
    int minoff = 0, minscale, mindenom;
    unsigned int minscore = 0, origscore = 1;
    int found = 0;

    w.setFromWeightAndOffset((int)(guessScale * 128 + 0.5), 0);
    mindenom = w.log2WeightDenom;
    minscale = w.inputWeight;

    pixel *mcbuf = frames[p0]->fpelPlane;
    origscore = minscore = weightCostLuma(b, mcbuf, NULL);

    if (!minscore)
        return;

    unsigned int s = 0;
    int curScale = minscale;
    int curOffset = (int)(fencMean - refMean * curScale / (1 << mindenom) + 0.5f);
    if (curOffset < -128 || curOffset > 127)
    {
        /* Rescale considering the constraints on curOffset. We do it in this order
            * because scale has a much wider range than offset (because of denom), so
            * it should almost never need to be clamped. */
        curOffset = Clip3(-128, 127, curOffset);
        curScale = (int)((1 << mindenom) * (fencMean - curOffset) / refMean + 0.5f);
        curScale = Clip3(0, 127, curScale);
    }
    SET_WEIGHT(w, 1, curScale, mindenom, curOffset);
    s = weightCostLuma(b, mcbuf, &w);
    COPY4_IF_LT(minscore, s, minscale, curScale, minoff, curOffset, found, 1);

    /* Use a smaller denominator if possible */
    while (mindenom > 0 && !(minscale & 1))
    {
        mindenom--;
        minscale >>= 1;
    }

    if (!found || (minscale == 1 << mindenom && minoff == 0) || (float)minscore / origscore > 0.998f)
        return;
    else
    {
        SET_WEIGHT(w, 1, minscale, mindenom, minoff);
        // set weighted delta cost
        fenc->weightedCostDelta[deltaIndex] = minscore / origscore;

        int offset = w.inputOffset << (X265_DEPTH - 8);
        int scale = w.inputWeight;
        int denom = w.log2WeightDenom;
        int correction = IF_INTERNAL_PREC - X265_DEPTH;
        int stride = ref->lumaStride;

        for (int i = 0; i < 4; i++)
        {
            // Adding (IF_INTERNAL_PREC - X265_DEPTH) to cancel effect of pixel to short conversion inside the primitive
            primitives.weight_pp(ref->buffer[i], wbuffer[i], stride, stride, stride, paddedLines,
                                 scale, (1 << (denom - 1 + correction)), denom + correction, offset);
        }

        weightedRef.isWeighted = true;
    }
}

#define NUM_CUS (widthInCU > 2 && heightInCU > 2 ? (widthInCU - 2) * (heightInCU - 2) : widthInCU * heightInCU)

int Lookahead::estimateFrameCost(int p0, int p1, int b, bool bIntraPenalty)
{
    int score = 0;
    Lowres *fenc = frames[b];

    if (fenc->costEst[b - p0][p1 - b] >= 0 && fenc->rowSatds[b - p0][p1 - b][0] != -1)
        score = fenc->costEst[b - p0][p1 - b];
    else
    {
        weightedRef.isWeighted = false;
        if (cfg->param.bEnableWeightedPred && b == p1 && b != p0 && fenc->lowresMvs[0][b - p0 - 1][0].x == 0x7FFF)
        {
            if (!fenc->bIntraCalculated)
            {
                estimateFrameCost(b, b, b, 0);
            }
            weightsAnalyse(b, p0);
        }

        /* For each list, check to see whether we have lowres motion-searched this reference */
        bDoSearch[0] = b != p0 && fenc->lowresMvs[0][b - p0 - 1][0].x == 0x7FFF;
        bDoSearch[1] = b != p1 && fenc->lowresMvs[1][p1 - b - 1][0].x == 0x7FFF;

        if (bDoSearch[0]) fenc->lowresMvs[0][b - p0 - 1][0].x = 0;
        if (bDoSearch[1]) fenc->lowresMvs[1][p1 - b - 1][0].x = 0;

        curb = b;
        curp0 = p0;
        curp1 = p1;
        fenc->costEst[b - p0][p1 - b] = 0;
        fenc->costEstAq[b - p0][p1 - b] = 0;

        for (int i = 0; i < heightInCU; i++)
        {
            lhrows[i].init();
            lhrows[i].me.setSourcePlane(fenc->lowresPlane[0], fenc->lumaStride);
        }

        rowsCompleted = false;

        if (m_pool)
        {
            WaveFront::enqueue();
            // enableAllRows must be already called
            enqueueRow(0);
            while (!rowsCompleted)
            {
                WaveFront::findJob();
            }

            WaveFront::dequeue();
        }
        else
        {
            for (int row = 0; row < heightInCU; row++)
            {
                processRow(row);
            }
        }

        // Accumulate cost from each row
        for (int row = 0; row < heightInCU; row++)
        {
            score += lhrows[row].costEst;
            fenc->costEst[0][0] += lhrows[row].costIntra;
            if (cfg->param.rc.aqMode)
            {
                fenc->costEstAq[0][0] += lhrows[row].costIntraAq;
                fenc->costEstAq[b - p0][p1 - b] += lhrows[row].costEstAq;
            }
            fenc->intraMbs[b - p0] += lhrows[row].intraMbs;
        }

        fenc->bIntraCalculated = true;

        if (b != p1)
            score = (uint64_t)score * 100 / (130 + cfg->param.bFrameBias);
        if (b != p0 || b != p1) //Not Intra cost
            fenc->costEst[b - p0][p1 - b] = score;
    }

    if (bIntraPenalty)
    {
        // arbitrary penalty for I-blocks after B-frames
        int ncu = NUM_CUS;
        score += (uint64_t)score * fenc->intraMbs[b - p0] / (ncu * 8);
    }
    x265_emms();
    return score;
}

void LookaheadRow::init()
{
    costEst = 0;
    costEstAq = 0;
    costIntra = 0;
    costIntraAq = 0;
    intraMbs = 0;
    active = false;
    completed = 0;
}

void LookaheadRow::estimateCUCost(Lowres **frames, ReferencePlanes *wfref0, int cux, int cuy, int p0, int p1, int b, bool bDoSearch[2])
{
    Lowres *fref1 = frames[p1];
    Lowres *fenc  = frames[b];

    const int bBidir = (b < p1);
    const int cuXY = cux + cuy * widthInCU;
    const int cuSize = X265_LOWRES_CU_SIZE;
    const int pelOffset = cuSize * cux + cuSize * cuy * fenc->lumaStride;

    // should this CU's cost contribute to the frame cost?
    const bool bFrameScoreCU = (cux > 0 && cux < widthInCU - 1 &&
                                cuy > 0 && cuy < heightInCU - 1) || widthInCU <= 2 || heightInCU <= 2;

    me.setSourcePU(pelOffset, cuSize, cuSize);

    MV(*fenc_mvs[2]) = { &fenc->lowresMvs[0][b - p0 - 1][cuXY],
                         &fenc->lowresMvs[1][p1 - b - 1][cuXY] };
    int(*fenc_costs[2]) = { &fenc->lowresMvCosts[0][b - p0 - 1][cuXY],
                            &fenc->lowresMvCosts[1][p1 - b - 1][cuXY] };

    MV mvmin, mvmax;
    int bcost = me.COST_MAX;
    int listused = 0;

    // establish search bounds that don't cross extended frame boundaries
    mvmin.x = (uint16_t)(-cux * cuSize - 8);
    mvmin.y = (uint16_t)(-cuy * cuSize - 8);
    mvmax.x = (uint16_t)((widthInCU - cux - 1) * cuSize + 8);
    mvmax.y = (uint16_t)((heightInCU - cuy - 1) * cuSize + 8);

    if (p0 != p1)
    {
        for (int i = 0; i < 1 + bBidir; i++)
        {
            if (!bDoSearch[i])
            {
                /* Use previously calculated cost */
                COPY2_IF_LT(bcost, *fenc_costs[i], listused, i + 1);
                continue;
            }
            int numc = 0;
            MV mvc[4], mvp;
            MV *fenc_mv = fenc_mvs[i];

            /* Reverse-order MV prediction. */
            mvc[0] = 0;
            mvc[2] = 0;
#define MVC(mv) mvc[numc++] = mv;
            if (cux < widthInCU - 1)
                MVC(fenc_mv[1]);
            if (cuy < heightInCU - 1)
            {
                MVC(fenc_mv[widthInCU]);
                if (cux > 0)
                    MVC(fenc_mv[widthInCU - 1]);
                if (cux < widthInCU - 1)
                    MVC(fenc_mv[widthInCU + 1]);
            }
#undef MVC
            if (numc <= 1)
                mvp = mvc[0];
            else
            {
                median_mv(mvp, mvc[0], mvc[1], mvc[2]);
            }

            *fenc_costs[i] = me.motionEstimate(i ? fref1 : wfref0, mvmin, mvmax, mvp, numc, mvc, merange, *fenc_mvs[i]);
            COPY2_IF_LT(bcost, *fenc_costs[i], listused, i + 1);
        }
        if (bBidir)
        {
            pixel subpelbuf0[X265_LOWRES_CU_SIZE * X265_LOWRES_CU_SIZE], subpelbuf1[X265_LOWRES_CU_SIZE * X265_LOWRES_CU_SIZE];
            intptr_t stride0 = X265_LOWRES_CU_SIZE, stride1 = X265_LOWRES_CU_SIZE;
            pixel *src0 = wfref0->lowresMC(pelOffset, *fenc_mvs[0], subpelbuf0, stride0);
            pixel *src1 = fref1->lowresMC(pelOffset, *fenc_mvs[1], subpelbuf1, stride1);

            pixel ref[X265_LOWRES_CU_SIZE * X265_LOWRES_CU_SIZE];
            primitives.pixelavg_pp[LUMA_8x8](ref, X265_LOWRES_CU_SIZE, src0, stride0, src1, stride1, 32);
            int bicost = primitives.satd[LUMA_8x8](fenc->lowresPlane[0] + pelOffset, fenc->lumaStride, ref, X265_LOWRES_CU_SIZE);
            COPY2_IF_LT(bcost, bicost, listused, 3);

            // Try 0,0 candidates
            src0 = wfref0->lowresPlane[0] + pelOffset;
            src1 = fref1->lowresPlane[0] + pelOffset;
            primitives.pixelavg_pp[LUMA_8x8](ref, X265_LOWRES_CU_SIZE, src0, wfref0->lumaStride, src1, fref1->lumaStride, 32);
            bicost = primitives.satd[LUMA_8x8](fenc->lowresPlane[0] + pelOffset, fenc->lumaStride, ref, X265_LOWRES_CU_SIZE);
            COPY2_IF_LT(bcost, bicost, listused, 3);
        }
    }
    if (!fenc->bIntraCalculated)
    {
        int nLog2SizeMinus2 = g_convertToBit[cuSize]; // partition size

        pixel _above0[X265_LOWRES_CU_SIZE * 4 + 1], *const above0 = _above0 + 2 * X265_LOWRES_CU_SIZE;
        pixel _above1[X265_LOWRES_CU_SIZE * 4 + 1], *const above1 = _above1 + 2 * X265_LOWRES_CU_SIZE;
        pixel _left0[X265_LOWRES_CU_SIZE * 4 + 1], *const left0 = _left0 + 2 * X265_LOWRES_CU_SIZE;
        pixel _left1[X265_LOWRES_CU_SIZE * 4 + 1], *const left1 = _left1 + 2 * X265_LOWRES_CU_SIZE;

        pixel *pix_cur = fenc->lowresPlane[0] + pelOffset;

        // Copy Above
        memcpy(above0, pix_cur - 1 - fenc->lumaStride, cuSize + 1);

        // Copy Left
        for (int i = 0; i < cuSize + 1; i++)
        {
            left0[i] = pix_cur[-1 - fenc->lumaStride + i * fenc->lumaStride];
        }

        memset(above0 + cuSize + 1, above0[cuSize], cuSize);
        memset(left0 + cuSize + 1, left0[cuSize], cuSize);

        // filtering with [1 2 1]
        // assume getUseStrongIntraSmoothing() is disabled
        above1[0] = above0[0];
        above1[2 * cuSize] = above0[2 * cuSize];
        left1[0] = left0[0];
        left1[2 * cuSize] = left0[2 * cuSize];
        for (int i = 1; i < 2 * cuSize; i++)
        {
            above1[i] = (above0[i - 1] + 2 * above0[i] + above0[i + 1] + 2) >> 2;
            left1[i] = (left0[i - 1] + 2 * left0[i] + left0[i + 1] + 2) >> 2;
        }

        int predsize = cuSize * cuSize;

        // generate 35 intra predictions into tmp
        primitives.intra_pred[nLog2SizeMinus2][DC_IDX](predictions, cuSize, left0, above0, 0, (cuSize <= 16));
        pixel *above = (cuSize >= 8) ? above1 : above0;
        pixel *left  = (cuSize >= 8) ? left1 : left0;
        primitives.intra_pred[nLog2SizeMinus2][PLANAR_IDX](predictions + predsize, cuSize, left, above, 0, 0);
        primitives.intra_pred_allangs[nLog2SizeMinus2](predictions + 2 * predsize, above0, left0, above1, left1, (cuSize <= 16));

        // calculate 35 satd costs, keep least cost
        ALIGN_VAR_32(pixel, buf_trans[32 * 32]);
        primitives.transpose[nLog2SizeMinus2](buf_trans, me.fenc, FENC_STRIDE);
        pixelcmp_t satd = primitives.satd[partitionFromSizes(cuSize, cuSize)];
        int icost = me.COST_MAX, cost;
        for (uint32_t mode = 0; mode < 35; mode++)
        {
            if ((mode >= 2) && (mode < 18))
                cost = satd(buf_trans, cuSize, &predictions[mode * predsize], cuSize);
            else
                cost = satd(me.fenc, FENC_STRIDE, &predictions[mode * predsize], cuSize);
            if (cost < icost)
                icost = cost;
        }

        // TOOD: i_icost += intra_penalty + lowres_penalty;
        fenc->intraCost[cuXY] = icost;
        fenc->rowSatds[0][0][cuy] += icost;
        if (bFrameScoreCU)
        {
            costIntra += icost;
            if (fenc->invQscaleFactor)
                costIntraAq += (icost * fenc->invQscaleFactor[cuXY] + 128) >> 8;
        }
    }
    if (!bBidir)
    {
        if (fenc->intraCost[cuXY] < bcost)
        {
            if (bFrameScoreCU) intraMbs++;
            bcost = fenc->intraCost[cuXY];
            listused = 0;
        }
    }

    /* For I frames these costs were accumulated earlier */
    if (p0 != p1)
    {
        fenc->rowSatds[b - p0][p1 - b][cuy] += bcost;
        if (bFrameScoreCU)
        {
            costEst += bcost;
            if (fenc->invQscaleFactor)
                costEstAq += (bcost * fenc->invQscaleFactor[cuXY] + 128) >> 8;
        }
    }
    fenc->lowresCosts[b - p0][p1 - b][cuXY] = (uint16_t)(X265_MIN(bcost, LOWRES_COST_MASK) | (listused << LOWRES_COST_SHIFT));
}

void Lookahead::slicetypeDecide()
{
    if (cfg->param.bFrameAdaptive && cfg->param.lookaheadDepth && cfg->param.bframes)
    {
        slicetypeAnalyse(false);

        TComPic *list[X265_LOOKAHEAD_MAX];
        TComPic *ipic = inputQueue.first();
        int j;
        for (j = 0; ipic && j < cfg->param.bframes + 2; ipic = ipic->m_next)
        {
            list[j++] = ipic;
        }

        list[j] = NULL;

        int bframes, brefs;
        for (bframes = 0, brefs = 0;; bframes++)
        {
            Lowres& frm = list[bframes]->m_lowres;

            if (frm.sliceType == X265_TYPE_BREF && !cfg->param.bBPyramid && brefs == cfg->param.bBPyramid)
            {
                frm.sliceType = X265_TYPE_B;
                x265_log(&cfg->param, X265_LOG_WARNING, "B-ref at frame %d incompatible with B-pyramid\n",
                         frm.frameNum);
            }

            /* pyramid with multiple B-refs needs a big enough dpb that the preceding P-frame stays available.
               smaller dpb could be supported by smart enough use of mmco, but it's easier just to forbid it.*/
            else if (frm.sliceType == X265_TYPE_BREF && cfg->param.bBPyramid && brefs &&
                     cfg->param.maxNumReferences <= (brefs + 3))
            {
                frm.sliceType = X265_TYPE_B;
                x265_log(&cfg->param, X265_LOG_WARNING, "B-ref at frame %d incompatible with B-pyramid and %d reference frames\n",
                         frm.sliceType, cfg->param.maxNumReferences);
            }

            if (frm.sliceType == X265_TYPE_KEYFRAME)
                frm.sliceType = cfg->param.bOpenGOP ? X265_TYPE_I : X265_TYPE_IDR;
            if (( /* !cfg->param.intraRefresh || */ frm.frameNum == 0) && frm.frameNum - lastKeyframe >= cfg->param.keyframeMax)
            {
                if (frm.sliceType == X265_TYPE_AUTO || frm.sliceType == X265_TYPE_I)
                    frm.sliceType = cfg->param.bOpenGOP && lastKeyframe >= 0 ? X265_TYPE_I : X265_TYPE_IDR;
                bool warn = frm.sliceType != X265_TYPE_IDR;
                if (warn && cfg->param.bOpenGOP)
                    warn &= frm.sliceType != X265_TYPE_I;
                if (warn)
                {
                    x265_log(&cfg->param, X265_LOG_WARNING, "specified frame type (%d) at %d is not compatible with keyframe interval\n", frm.sliceType, frm.frameNum);
                    frm.sliceType = cfg->param.bOpenGOP && lastKeyframe >= 0 ? X265_TYPE_I : X265_TYPE_IDR;
                }
            }
            if (frm.sliceType == X265_TYPE_I && frm.frameNum - lastKeyframe >= cfg->param.keyframeMin)
            {
                if (cfg->param.bOpenGOP)
                {
                    lastKeyframe = frm.frameNum;
                    frm.bKeyframe = true;
                }
                else
                    frm.sliceType = X265_TYPE_IDR;
            }
            if (frm.sliceType == X265_TYPE_IDR)
            {
                /* Closed GOP */
                lastKeyframe = frm.frameNum;
                frm.bKeyframe = true;
                if (bframes > 0)
                {
                    frames[bframes]->sliceType = X265_TYPE_P;
                    bframes--;
                }
            }

            if (bframes == cfg->param.bframes || !list[bframes + 1])
            {
                if (IS_X265_TYPE_B(frm.sliceType))
                    x265_log(&cfg->param, X265_LOG_WARNING, "specified frame type is not compatible with max B-frames\n");
                if (frm.sliceType == X265_TYPE_AUTO || IS_X265_TYPE_B(frm.sliceType))
                    frm.sliceType = X265_TYPE_P;
            }
            if (frm.sliceType == X265_TYPE_BREF)
                brefs++;
            if (frm.sliceType == X265_TYPE_AUTO)
                frm.sliceType = X265_TYPE_B;
            else if (!IS_X265_TYPE_B(frm.sliceType))
                break;
        }

        if (bframes)
            list[bframes - 1]->m_lowres.bLastMiniGopBFrame = true;
        list[bframes]->m_lowres.leadingBframes = bframes;
        lastNonB = &list[bframes]->m_lowres;

        /* insert a bref into the sequence */
        if (cfg->param.bBPyramid && bframes > 1 && !brefs)
        {
            list[bframes / 2]->m_lowres.sliceType = X265_TYPE_BREF;
            brefs++;
        }

        /* calculate the frame costs ahead of time for x264_rc_analyse_slice while we still have lowres */
        if (cfg->param.rc.rateControlMode != X265_RC_CQP)
        {
            int p0, p1, b;
            p1 = b = bframes + 1;

            frames[0] = lastNonB;
            for (int i = 0; i <= bframes; i++)
            {
                frames[i + 1] = &list[i]->m_lowres;
            }

            if (IS_X265_TYPE_I(frames[bframes + 1]->sliceType))
                p0 = bframes + 1;
            else // P
                p0 = 0;

            estimateFrameCost(p0, p1, b, 0);

            if ((p0 != p1 || bframes) /*&& cfg->param.rc.i_vbv_buffer_size*/)
            {
                // We need the intra costs for row SATDs
                estimateFrameCost(b, b, b, 0);

                // We need B-frame costs for row SATDs
                p0 = 0;
                for (b = 1; b <= bframes; b++)
                {
                    if (frames[b]->sliceType == X265_TYPE_B)
                        for (p1 = b; frames[p1]->sliceType == X265_TYPE_B; )
                        {
                            p1++;
                        }

                    else
                        p1 = bframes + 1;
                    estimateFrameCost(p0, p1, b, 0);
                    if (frames[b]->sliceType == X265_TYPE_BREF)
                        p0 = b;
                }
            }
        }

        /* dequeue all frames from inputQueue that are about to be enqueued
         * in the output queue.  The order is important because TComPic can
         * only be in one list at a time */
        for (int i = 0; i <= bframes; i++)
        {
            inputQueue.popFront();
        }

        /* add non-B to output queue */
        outputQueue.pushBack(*list[bframes]);

        /* Add B-ref frame next to P frame in output queue, the B-ref encode before non B-ref frame */
        if (bframes > 1 && cfg->param.bBPyramid)
        {
            for (int i = 0; i < bframes; i++)
            {
                if (list[i]->m_lowres.sliceType == X265_TYPE_BREF)
                    outputQueue.pushBack(*list[i]);
            }
        }

        /* add B frames to output queue */
        for (int i = 0; i < bframes; i++)
        {
            /* push all the B frames into output queue except B-ref, which already pushed into output queue*/
            if (list[i]->m_lowres.sliceType != X265_TYPE_BREF)
                outputQueue.pushBack(*list[i]);
        }

        return;
    }

    // Fixed GOP structures for when B-Adapt and/or lookahead are disabled
    if (numDecided == 0 || cfg->param.keyframeMax <= 1)
    {
        TComPic *pic = inputQueue.popFront();
        pic->m_lowres.sliceType = X265_TYPE_I;
        pic->m_lowres.bKeyframe = true;
        lastKeyframe = pic->m_lowres.frameNum;
        lastNonB = &pic->m_lowres;
        numDecided++;
        outputQueue.pushBack(*pic);
    }
    else if (cfg->param.bframes == 0 || inputQueue.size() == 1)
    {
        TComPic *pic = inputQueue.popFront();
        if (pic->getPOC() % cfg->param.keyframeMax)
            pic->m_lowres.sliceType = X265_TYPE_P;
        else
        {
            pic->m_lowres.sliceType = X265_TYPE_I;
            pic->m_lowres.bKeyframe = true;
            lastKeyframe = pic->m_lowres.frameNum;
        }
        lastNonB = &pic->m_lowres;
        outputQueue.pushBack(*pic);
        numDecided++;
    }
    else
    {
        TComPic *list[X265_BFRAME_MAX + 1];
        int j;
        for (j = 0; j <= cfg->param.bframes && !inputQueue.empty(); j++)
        {
            TComPic *pic = inputQueue.popFront();
            list[j] = pic;
            if (pic->m_lowres.frameNum >= lastKeyframe + cfg->param.keyframeMax)
            {
                if (j)
                {
                    list[j - 1]->m_lowres.sliceType = X265_TYPE_P;
                    inputQueue.pushFront(*pic); // push I-frame back onto input queue
                }
                else
                {
                    pic->m_lowres.sliceType = X265_TYPE_I;
                    pic->m_lowres.bKeyframe = true;
                    lastKeyframe = pic->m_lowres.frameNum;
                    j++;
                }
                break;
            }
        }

        if (!j)
            return;
        int bframes = j - 1;
        if (bframes)
            list[bframes - 1]->m_lowres.bLastMiniGopBFrame = true;
        list[bframes]->m_lowres.leadingBframes = bframes;
        lastNonB = &list[bframes]->m_lowres;

        TComPic *pic = list[bframes];
        if (pic->m_lowres.sliceType == X265_TYPE_AUTO)
            pic->m_lowres.sliceType = X265_TYPE_P;
        outputQueue.pushBack(*pic);
        numDecided++;

        if (cfg->param.bBPyramid && bframes > 1)
        {
            int bref = bframes / 2;
            if (list[bref - 1]->m_lowres.sliceType == X265_TYPE_AUTO)
            {
                list[bref - 1]->m_lowres.sliceType = X265_TYPE_BREF;
                outputQueue.pushBack(*list[bref - 1]);
                numDecided++;
            }
        }

        for (int i = 0; i < bframes; i++)
        {
            pic = list[i];
            if (pic->m_lowres.sliceType == X265_TYPE_AUTO)
                pic->m_lowres.sliceType = X265_TYPE_B;

            if (pic->m_lowres.sliceType != X265_TYPE_BREF)
            {
                outputQueue.pushBack(*pic);
                numDecided++;
            }
        }
    }
}

void Lookahead::slicetypeAnalyse(bool bKeyframe)
{
    int num_frames, origNumFrames, keyint_limit, framecnt;
    int maxSearch = X265_MIN(cfg->param.lookaheadDepth, X265_LOOKAHEAD_MAX);
    int cuCount = NUM_CUS;
    int cost1p0, cost2p0, cost1b1, cost2p1;
    int reset_start;

    if (!lastNonB)
        return;
    frames[0] = lastNonB;
    TComPic* pic = inputQueue.first();
    for (framecnt = 0; (framecnt < maxSearch) && pic && pic->m_lowres.sliceType == X265_TYPE_AUTO; framecnt++)
    {
        frames[framecnt + 1] = &pic->m_lowres;
        pic = pic->m_next;
    }

    if (!framecnt)
    {
        if (cfg->param.rc.cuTree)
            cuTree(frames, 0, bKeyframe);
        return;
    }

    frames[framecnt + 1] = NULL;

    keyint_limit = cfg->param.keyframeMax - frames[0]->frameNum + lastKeyframe - 1;
    origNumFrames = num_frames = X265_MIN(framecnt, keyint_limit);

    if (cfg->param.bOpenGOP && num_frames < framecnt)
        num_frames++;
    else if (num_frames == 0)
    {
        frames[1]->sliceType = X265_TYPE_I;
        return;
    }

    int num_bframes = 0;
    int num_analysed_frames = num_frames;
    if (cfg->param.scenecutThreshold && scenecut(0, 1, 1, origNumFrames, maxSearch))
    {
        frames[1]->sliceType = X265_TYPE_I;
        return;
    }

    if (cfg->param.bframes)
    {
        if (cfg->param.bFrameAdaptive == X265_B_ADAPT_TRELLIS)
        {
            if (num_frames > 1)
            {
                char best_paths[X265_BFRAME_MAX + 1][X265_LOOKAHEAD_MAX + 1] = { "", "P" };
                int best_path_index = num_frames % (X265_BFRAME_MAX + 1);

                /* Perform the frametype analysis. */
                for (int j = 2; j <= num_frames; j++)
                {
                    slicetypePath(j, best_paths);
                }

                num_bframes = (int)strspn(best_paths[best_path_index], "B");

                /* Load the results of the analysis into the frame types. */
                for (int j = 1; j < num_frames; j++)
                {
                    frames[j]->sliceType  = best_paths[best_path_index][j - 1] == 'B' ? X265_TYPE_B : X265_TYPE_P;
                }
            }
            frames[num_frames]->sliceType = X265_TYPE_P;
        }
        else if (cfg->param.bFrameAdaptive == X265_B_ADAPT_FAST)
        {
            for (int i = 0; i <= num_frames - 2; )
            {
                cost2p1 = estimateFrameCost(i + 0, i + 2, i + 2, 1);
                if (frames[i + 2]->intraMbs[2] > cuCount / 2)
                {
                    frames[i + 1]->sliceType = X265_TYPE_P;
                    frames[i + 2]->sliceType = X265_TYPE_P;
                    i += 2;
                    continue;
                }

                cost1b1 = estimateFrameCost(i + 0, i + 2, i + 1, 0);
                cost1p0 = estimateFrameCost(i + 0, i + 1, i + 1, 0);
                cost2p0 = estimateFrameCost(i + 1, i + 2, i + 2, 0);

                if (cost1p0 + cost2p0 < cost1b1 + cost2p1)
                {
                    frames[i + 1]->sliceType = X265_TYPE_P;
                    i += 1;
                    continue;
                }

// arbitrary and untuned
#define INTER_THRESH 300
#define P_SENS_BIAS (50 - cfg->param.bFrameBias)
                frames[i + 1]->sliceType = X265_TYPE_B;

                int j;
                for (j = i + 2; j <= X265_MIN(i + cfg->param.bframes, num_frames - 1); j++)
                {
                    int pthresh = X265_MAX(INTER_THRESH - P_SENS_BIAS * (j - i - 1), INTER_THRESH / 10);
                    int pcost = estimateFrameCost(i + 0, j + 1, j + 1, 1);
                    if (pcost > pthresh * cuCount || frames[j + 1]->intraMbs[j - i + 1] > cuCount / 3)
                        break;
                    frames[j]->sliceType = X265_TYPE_B;
                }

                frames[j]->sliceType = X265_TYPE_P;
                i = j;
            }
            frames[num_frames]->sliceType = X265_TYPE_P;
            num_bframes = 0;
            while (num_bframes < num_frames && frames[num_bframes + 1]->sliceType == X265_TYPE_B)
            {
                num_bframes++;
            }
        }
        else
        {
            num_bframes = X265_MIN(num_frames - 1, cfg->param.bframes);
            for (int j = 1; j < num_frames; j++)
            {
                frames[j]->sliceType = (j % (num_bframes + 1)) ? X265_TYPE_B : X265_TYPE_P;
            }

            frames[num_frames]->sliceType = X265_TYPE_P;
        }
        /* Check scenecut on the first minigop. */
        for (int j = 1; j < num_bframes + 1; j++)
        {
            if (cfg->param.scenecutThreshold && scenecut(j, j + 1, 0, origNumFrames, maxSearch))
            {
                frames[j]->sliceType = X265_TYPE_P;
                num_analysed_frames = j;
                break;
            }
        }

        reset_start = bKeyframe ? 1 : X265_MIN(num_bframes + 2, num_analysed_frames + 1);
    }
    else
    {
        for (int j = 1; j <= num_frames; j++)
        {
            frames[j]->sliceType = X265_TYPE_P;
        }

        reset_start = bKeyframe ? 1 : 2;
        num_bframes = 0;
    }

    if (cfg->param.rc.cuTree)
        cuTree(frames, X265_MIN(num_frames, cfg->param.keyframeMax), bKeyframe);

    // if (!cfg->param.bIntraRefresh)
    for (int j = keyint_limit + 1; j <= num_frames; j += cfg->param.keyframeMax)
    {
        frames[j]->sliceType = X265_TYPE_I;
        reset_start = X265_MIN(reset_start, j + 1);
    }

    /* Restore frametypes for all frames that haven't actually been decided yet. */
    for (int j = reset_start; j <= num_frames; j++)
    {
        frames[j]->sliceType = X265_TYPE_AUTO;
    }
}

int Lookahead::scenecut(int p0, int p1, bool bRealScenecut, int num_frames, int maxSearch)
{
    /* Only do analysis during a normal scenecut check. */
    if (bRealScenecut && cfg->param.bframes)
    {
        int origmaxp1 = p0 + 1;
        /* Look ahead to avoid coding short flashes as scenecuts. */
        if (cfg->param.bFrameAdaptive == X265_B_ADAPT_TRELLIS)
            /* Don't analyse any more frames than the trellis would have covered. */
            origmaxp1 += cfg->param.bframes;
        else
            origmaxp1++;
        int maxp1 = X265_MIN(origmaxp1, num_frames);

        /* Where A and B are scenes: AAAAAABBBAAAAAA
         * If BBB is shorter than (maxp1-p0), it is detected as a flash
         * and not considered a scenecut. */
        for (int cp1 = p1; cp1 <= maxp1; cp1++)
        {
            if (!scenecutInternal(p0, cp1, 0))
                /* Any frame in between p0 and cur_p1 cannot be a real scenecut. */
                for (int i = cp1; i > p0; i--)
                {
                    frames[i]->bScenecut = false;
                }
        }

        /* Where A-F are scenes: AAAAABBCCDDEEFFFFFF
         * If each of BB ... EE are shorter than (maxp1-p0), they are
         * detected as flashes and not considered scenecuts.
         * Instead, the first F frame becomes a scenecut.
         * If the video ends before F, no frame becomes a scenecut. */
        for (int cp0 = p0; cp0 <= maxp1; cp0++)
        {
            if (origmaxp1 > maxSearch || (cp0 < maxp1 && scenecutInternal(cp0, maxp1, 0)))
                /* If cur_p0 is the p0 of a scenecut, it cannot be the p1 of a scenecut. */
                frames[cp0]->bScenecut = false;
        }
    }

    /* Ignore frames that are part of a flash, i.e. cannot be real scenecuts. */
    if (!frames[p1]->bScenecut)
        return 0;
    return scenecutInternal(p0, p1, bRealScenecut);
}

int Lookahead::scenecutInternal(int p0, int p1, bool /* bRealScenecut */)
{
    Lowres *frame = frames[p1];

    estimateFrameCost(p0, p1, p1, 0);

    int icost = frame->costEst[0][0];
    int pcost = frame->costEst[p1 - p0][0];
    float bias;
    int gopSize = frame->frameNum - lastKeyframe;
    float threshMax = (float)(cfg->param.scenecutThreshold / 100.0);
    /* magic numbers pulled out of thin air */
    float threshMin = (float)(threshMax * 0.25);
    int res;

    if (cfg->param.keyframeMin == cfg->param.keyframeMax)
        threshMin = threshMax;
    if (gopSize <= cfg->param.keyframeMin / 4)
        bias = threshMin / 4;
    else if (gopSize <= cfg->param.keyframeMin)
        bias = threshMin * gopSize / cfg->param.keyframeMin;
    else
    {
        bias = threshMin
            + (threshMax - threshMin)
            * (gopSize - cfg->param.keyframeMin)
            / (cfg->param.keyframeMax - cfg->param.keyframeMin);
    }

    res = pcost >= (1.0 - bias) * icost;
    return res;
}

void Lookahead::slicetypePath(int length, char(*best_paths)[X265_LOOKAHEAD_MAX + 1])
{
    char paths[2][X265_LOOKAHEAD_MAX + 1];
    int num_paths = X265_MIN(cfg->param.bframes + 1, length);
    int best_cost = MotionEstimate::COST_MAX;
    int idx = 0;

    /* Iterate over all currently possible paths */
    for (int path = 0; path < num_paths; path++)
    {
        /* Add suffixes to the current path */
        int len = length - (path + 1);
        memcpy(paths[idx], best_paths[len % (X265_BFRAME_MAX + 1)], len);
        memset(paths[idx] + len, 'B', path);
        strcpy(paths[idx] + len + path, "P");

        /* Calculate the actual cost of the current path */
        int cost = slicetypePathCost(paths[idx], best_cost);
        if (cost < best_cost)
        {
            best_cost = cost;
            idx ^= 1;
        }
    }

    /* Store the best path. */
    memcpy(best_paths[length % (X265_BFRAME_MAX + 1)], paths[idx ^ 1], length);
}

int Lookahead::slicetypePathCost(char *path, int threshold)
{
    int loc = 1;
    int cost = 0;
    int cur_p = 0;

    path--; /* Since the 1st path element is really the second frame */
    while (path[loc])
    {
        int next_p = loc;
        /* Find the location of the next P-frame. */
        while (path[next_p] != 'P')
        {
            next_p++;
        }

        /* Add the cost of the P-frame found above */
        cost += estimateFrameCost(cur_p, next_p, next_p, 0);
        /* Early terminate if the cost we have found is larger than the best path cost so far */
        if (cost > threshold)
            break;

        if (cfg->param.bBPyramid && next_p - cur_p > 2)
        {
            int middle = cur_p + (next_p - cur_p) / 2;
            cost += estimateFrameCost(cur_p, next_p, middle, 0);
            for (int next_b = loc; next_b < middle && cost < threshold; next_b++)
            {
                cost += estimateFrameCost(cur_p, middle, next_b, 0);
            }

            for (int next_b = middle + 1; next_b < next_p && cost < threshold; next_b++)
            {
                cost += estimateFrameCost(middle, next_p, next_b, 0);
            }
        }
        else
        {
            for (int next_b = loc; next_b < next_p && cost < threshold; next_b++)
            {
                cost += estimateFrameCost(cur_p, next_p, next_b, 0);
            }
        }

        loc = next_p + 1;
        cur_p = next_p;
    }

    return cost;
}

void Lookahead::processRow(int row)
{
    int realrow = heightInCU - 1 - row;
    Lowres *fenc = frames[curb];
    ReferencePlanes *wfref0 = weightedRef.isWeighted ? &weightedRef : frames[curp0];

    if (!fenc->bIntraCalculated)
        fenc->rowSatds[0][0][realrow] = 0;
    fenc->rowSatds[curb - curp0][curp1 - curb][realrow] = 0;

    /* Lowres lookahead goes backwards because the MVs are used as
     * predictors in the main encode.  This considerably improves MV
     * prediction overall. */
    for (int i = widthInCU - 1 - lhrows[row].completed; i >= 0; i--)
    {
        // TODO: use lowres MVs as motion candidates in full-res search
        lhrows[row].estimateCUCost(frames, wfref0, i, realrow, curp0, curp1, curb, bDoSearch);
        lhrows[row].completed++;

        if (lhrows[row].completed >= 2 && row < heightInCU - 1)
        {
            ScopedLock below(lhrows[row + 1].lock);
            if (lhrows[row + 1].active == false &&
                lhrows[row + 1].completed + 2 <= lhrows[row].completed)
            {
                lhrows[row + 1].active = true;
                enqueueRow(row + 1);
            }
        }

        ScopedLock self(lhrows[row].lock);
        if (row > 0 && (int32_t)lhrows[row].completed < widthInCU - 1 && lhrows[row - 1].completed < lhrows[row].completed + 2)
        {
            lhrows[row].active = false;
            return;
        }
    }

    if (row == heightInCU - 1)
    {
        rowsCompleted = true;
    }
}

void Lookahead::cuTree(Lowres **Frames, int numframes, bool bintra)
{
    int idx = !bintra;
    int lastnonb, curnonb = 1;
    int bframes = 0;

    x265_emms();
    double totalDuration = 0.0;
    for (int j = 0; j <= numframes; j++)
        totalDuration += 1.0 / cfg->param.frameRate;
    double averageDuration = totalDuration / (numframes + 1);

    int i = numframes;
    int cuCount = widthInCU * heightInCU;

    if (bintra)
        estimateFrameCost(0, 0, 0, 0);

    while (i > 0 && Frames[i]->sliceType == X265_TYPE_B)
        i--;
    lastnonb = i;

    /* Lookaheadless MB-tree is not a theoretically distinct case; the same extrapolation could
     * be applied to the end of a lookahead buffer of any size.  However, it's most needed when
     * lookahead=0, so that's what's currently implemented. */
    if (!cfg->param.lookaheadDepth)
    {
        if (bintra)
        {
            memset(Frames[0]->propagateCost, 0, cuCount * sizeof(uint16_t));
            memcpy(Frames[0]->qpOffset, Frames[0]->qpAqOffset, cuCount * sizeof(double));
            return;
        }
        std::swap(Frames[lastnonb]->propagateCost, Frames[0]->propagateCost);
        memset(Frames[0]->propagateCost, 0, cuCount * sizeof(uint16_t));
    }
    else
    {
        if (lastnonb < idx)
            return;
        memset(Frames[lastnonb]->propagateCost, 0, cuCount * sizeof(uint16_t));
    }

    while (i-- > idx)
    {
        curnonb = i;
        while (Frames[curnonb]->sliceType == X265_TYPE_B && curnonb > 0)
            curnonb--;
        if (curnonb < idx)
            break;

        estimateFrameCost(curnonb, lastnonb, lastnonb, 0);
        memset(Frames[curnonb]->propagateCost, 0, cuCount * sizeof(uint16_t));
        bframes = lastnonb - curnonb - 1;
        if (cfg->param.bBPyramid && bframes > 1)
        {
            int middle = (bframes + 1) / 2 + curnonb;
            estimateFrameCost(curnonb, lastnonb, middle, 0);
            memset(Frames[middle]->propagateCost, 0, cuCount * sizeof(uint16_t));
            while (i > curnonb)
            {
                int p0 = i > middle ? middle : curnonb;
                int p1 = i < middle ? middle : lastnonb;
                if (i != middle)
                {
                    estimateFrameCost(p0, p1, i, 0);
                    estimateCUPropagate(Frames, averageDuration, p0, p1, i, 0);
                }
                i--;
            }
            estimateCUPropagate(Frames, averageDuration, curnonb, lastnonb, middle, 1);
        }
        else
        {
            while (i > curnonb)
            {
                estimateFrameCost(curnonb, lastnonb, i, 0);
                estimateCUPropagate(Frames, averageDuration, curnonb, lastnonb, i, 0);
                i--;
            }
        }
        estimateCUPropagate(Frames, averageDuration, curnonb, lastnonb, lastnonb, 1);
        lastnonb = curnonb;
    }

    if (!cfg->param.lookaheadDepth)
    {
        estimateFrameCost(0, lastnonb, lastnonb, 0);
        estimateCUPropagate(Frames, averageDuration, 0, lastnonb, lastnonb, 1);
        std::swap(Frames[lastnonb]->propagateCost, Frames[0]->propagateCost);
    }

    cuTreeFinish(Frames[lastnonb], averageDuration, lastnonb);
    if (cfg->param.bBPyramid && bframes > 1 && !cfg->param.rc.vbvBufferSize)
        cuTreeFinish(Frames[lastnonb + (bframes + 1) / 2], averageDuration, 0);
}

void Lookahead::estimateCUPropagate(Lowres **Frames, double averageDuration, int p0, int p1, int b, int referenced)
{
    uint16_t *refCosts[2] = {Frames[p0]->propagateCost, Frames[p1]->propagateCost};
    int distScaleFactor = (((b - p0) << 8) + ((p1 - p0) >> 1)) / (p1 - p0);
    int bipredWeight = cfg->param.bEnableWeightedBiPred ? 64 - (distScaleFactor >> 2) : 32;
    MV *mvs[2] = {Frames[b]->lowresMvs[0][b - p0 -1], Frames[b]->lowresMvs[1][p1 - b - 1]};
    int bipredWeights[2] = {bipredWeight, 64 - bipredWeight};
    memset(scratch, 0, widthInCU * sizeof(int));

    uint16_t *propagate_cost = Frames[b]->propagateCost;

    x265_emms();
    double fpsFactor = CLIP_DURATION(1.0 / cfg->param.frameRate) / CLIP_DURATION(averageDuration);

    /* For non-refferd frames the source costs are always zero, so just memset one row and re-use it. */
    if (!referenced)
        memset(Frames[b]->propagateCost, 0, widthInCU * sizeof(uint16_t));

    uint16_t StrideInCU = (uint16_t)widthInCU;
    for (uint16_t block_y = 0; block_y < heightInCU; block_y++)
    {
        int cuIndex = block_y * StrideInCU;
        /* TODO This function go into ASM */
        estimateCUPropagateCost(scratch, propagate_cost,
            Frames[b]->intraCost + cuIndex, Frames[b]->lowresCosts[b - p0][p1 - b] + cuIndex,
            Frames[b]->invQscaleFactor + cuIndex, &fpsFactor, widthInCU);

        if (referenced)
            propagate_cost += widthInCU;
        for (uint16_t block_x = 0; block_x < widthInCU; block_x++, cuIndex++)
        {
            int propagate_amount = scratch[block_x];
            /* Don't propagate for an intra block. */
            if (propagate_amount > 0)
            {
                /* Access width-2 bitfield. */
                int lists_used = Frames[b]->lowresCosts[b - p0][p1 - b][cuIndex] >> LOWRES_COST_SHIFT;
                /* Follow the MVs to the previous frame(s). */
                for (uint16_t list = 0; list < 2; list++)
                    if ((lists_used >> list) & 1)
                    {
#define CLIP_ADD(s, x) (s) = X265_MIN((s) + (x),(1 << 16) - 1)
                        uint16_t listamount = (uint16_t)propagate_amount;
                        /* Apply bipred weighting. */
                        if (lists_used == 3)
                            listamount = (uint16_t)(listamount * bipredWeights[list] + 32) >> 6;

                        /* Early termination for simple case of mv0. */
                        if (mvs[list]->notZero())
                        {
                            CLIP_ADD(refCosts[list][cuIndex], listamount);
                            continue;
                        }

                        uint16_t x = mvs[list]->x;
                        uint16_t y = mvs[list]->y;
                        int cux = (x >> 5) + block_x;
                        int cuy = (y >> 5) + block_y;
                        int idx0 = cux + cuy * StrideInCU;
                        int idx1 = idx0 + 1;
                        int idx2 = idx0 + StrideInCU;
                        int idx3 = idx0 + StrideInCU + 1;
                        x &= 31;
                        y &= 31;
                        uint16_t idx0weight = (uint16_t) (32 - y) * (32 - x);
                        uint16_t idx1weight = (uint16_t) (32 - y) * x;
                        uint16_t idx2weight = (uint16_t) y * (32 - x);
                        uint16_t idx3weight = (uint16_t) y * x;

                        /* We could just clip the MVs, but pixels that lie outside the frame probably shouldn't
                         * be counted. */
                        if (cux < widthInCU - 1 && cuy < heightInCU - 1 && cux >= 0 && cuy >= 0)
                        {
                            CLIP_ADD(refCosts[list][idx0], (listamount * idx0weight + 512) >> 10);
                            CLIP_ADD(refCosts[list][idx1], (listamount * idx1weight + 512) >> 10);
                            CLIP_ADD(refCosts[list][idx2], (listamount * idx2weight + 512) >> 10);
                            CLIP_ADD(refCosts[list][idx3], (listamount * idx3weight + 512) >> 10);
                        }
                        else /* Check offsets individually */
                        {
                            if (cux < widthInCU && cuy < heightInCU && cux >= 0 && cuy >= 0)
                                CLIP_ADD(refCosts[list][idx0], (listamount * idx0weight + 512) >> 10);
                            if (cux + 1 < widthInCU && cuy < heightInCU && cux + 1 >= 0 && cuy >= 0)
                                CLIP_ADD(refCosts[list][idx1], (listamount * idx1weight + 512) >> 10);
                            if (cux < widthInCU && cuy + 1 < heightInCU && cux >= 0 && cuy + 1 >= 0)
                                CLIP_ADD(refCosts[list][idx2], (listamount * idx2weight + 512) >> 10);
                            if (cux + 1 < widthInCU && cuy + 1 < heightInCU && cux + 1 >= 0 && cuy + 1 >= 0)
                                CLIP_ADD(refCosts[list][idx3], (listamount * idx3weight + 512) >> 10);
                        }
                    }
            }
        }
    }

    if(cfg->param.rc.vbvBufferSize && cfg->param.logLevel && referenced)
        cuTreeFinish(Frames[b], averageDuration, b == p1 ? b - p0 : 0);

}

void Lookahead::cuTreeFinish(Lowres *Frame, double averageDuration, int ref0Distance)
{
    int fpsFactor = (int)(CLIP_DURATION(averageDuration) / CLIP_DURATION(1.0 / cfg->param.frameRate) * 256);
    double weightdelta = 0.0;

    if (ref0Distance && Frame->weightedCostDelta[ref0Distance - 1] > 0)
        weightdelta = (1.0 - Frame->weightedCostDelta[ref0Distance - 1]);

    /* Allow the strength to be adjusted via qcompress, since the two
     * concepts are very similar. */

    int cuCount = widthInCU * heightInCU;
    double strength = 5.0 * (1.0 - cfg->param.rc.qCompress);

    for (int cuIndex = 0; cuIndex < cuCount; cuIndex++)
    {
        int intracost = (Frame->intraCost[cuIndex] * Frame->invQscaleFactor[cuIndex] + 128) >> 8;
        if (intracost)
        {
            int propagate_cost = (Frame->propagateCost[cuIndex] * fpsFactor + 128) >> 8;
            double log2_ratio = X265_LOG2(intracost + propagate_cost) - X265_LOG2(intracost) + weightdelta;
            Frame->qpOffset[cuIndex] = Frame->qpAqOffset[cuIndex] - strength * log2_ratio;
        }
    }
}

/* Estimate the total amount of influence on future quality that could be had if we
 * were to improve the reference samples used to inter predict any given macroblock. */
void Lookahead::estimateCUPropagateCost(int *dst, uint16_t *propagateIn, int32_t *intraCosts, uint16_t *interCosts, int32_t *invQscales, double *fpsFactor, int len)
{
    double fps = *fpsFactor / 256;
    for(int i = 0; i < len; i++)
    {
        double intraCost       = intraCosts[i] * invQscales[i];
        double propagateAmount = (double)propagateIn[i] + intraCost * fps;
        double propagateNum    = (double)intraCosts[i] - (interCosts[i] & LOWRES_COST_MASK);
        double propagateDenom  = (double)intraCosts[i];
        dst[i] = (int)(propagateAmount * propagateNum / propagateDenom + 0.5);
    }
}

/* If MB-tree changes the quantizers, we need to recalculate the frame cost without
 * re-running lookahead. */
int Lookahead::frameCostRecalculate(Lowres** Frames, int p0, int p1, int b)
{
    int score = 0;
    int *row_satd = Frames[b]->rowSatds[b-p0][p1-b]; 
    double *qp_offset = IS_X265_TYPE_B(Frames[0]->sliceType) ? Frames[b]->qpAqOffset : Frames[b]->qpOffset;
    x265_emms();
    for (int cuy = heightInCU - 1; cuy >= 0; cuy--)
    {
        row_satd[cuy] = 0;
        for (int cux = widthInCU - 1; cux >= 0; cux-- )
        {
            int cuxy = cux + cuy * widthInCU;
            int cuCost = Frames[b]->lowresCosts[b-p0][p1-b][cuxy] & LOWRES_COST_MASK;
            double qp_adj = qp_offset[cuxy];
            cuCost = (cuCost * x265_exp2fix8(qp_adj) + 128) >> 8;
            row_satd[cuy ] += cuCost;
            if( (cuy > 0 && cuy < heightInCU - 1 &&
                 cux > 0 && cux < widthInCU - 1) ||
                 widthInCU <= 2 || heightInCU <= 2 )
            {
                score += cuCost;
            }
        }
    }
    return score;
}
