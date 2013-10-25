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

    lhrows = new LookaheadRow[heightInCU];
    for (int i = 0; i < heightInCU; i++)
    {
        lhrows[i].widthInCU = widthInCU;
        lhrows[i].heightInCU = heightInCU;
        lhrows[i].frames = frames;
    }
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
}

void Lookahead::addPicture(TComPic *pic, int sliceType)
{
    pic->m_lowres.init(pic->getPicYuvOrg(), pic->getSlice()->getPOC(), sliceType, cfg->param.bframes);

    inputQueue.pushBack(*pic);
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
    if (cfg->param.rc.aqMode)
        pic->m_lowres.satdCost = pic->m_lowres.costEstAq[b - p0][p1 - b];
    else
        pic->m_lowres.satdCost = pic->m_lowres.costEst[b - p0][p1 - b];
    return pic->m_lowres.satdCost;
}

#define NUM_CUS (widthInCU > 2 && heightInCU > 2 ? (widthInCU - 2) * (heightInCU - 2) : widthInCU * heightInCU)

int Lookahead::estimateFrameCost(int p0, int p1, int b, bool bIntraPenalty)
{
    int score = 0;
    Lowres *fenc = frames[b];

    curb = b;
    curp0 = p0;
    curp1 = p1;

    if (fenc->costEst[b - p0][p1 - b] >= 0 && fenc->rowSatds[b - p0][p1 - b][0] != -1)
        score = fenc->costEst[b - p0][p1 - b];
    else
    {
        /* For each list, check to see whether we have lowres motion-searched this reference */
        bDoSearch[0] = b != p0 && fenc->lowresMvs[0][b - p0 - 1][0].x == 0x7FFF;
        bDoSearch[1] = b != p1 && fenc->lowresMvs[1][p1 - b - 1][0].x == 0x7FFF;

        if (bDoSearch[0]) fenc->lowresMvs[0][b - p0 - 1][0].x = 0;
        if (bDoSearch[1]) fenc->lowresMvs[1][p1 - b - 1][0].x = 0;

        fenc->costEst[b - p0][p1 - b] = 0;
        fenc->costEstAq[b - p0][p1 - b] = 0;
        // TODO: use lowres MVs as motion candidates in full-res search

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

void LookaheadRow::estimateCUCost(int cux, int cuy, int p0, int p1, int b, bool bDoSearch[2])
{
    Lowres *fref0 = frames[p0];
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

            *fenc_costs[i] = me.motionEstimate(i ? fref1 : fref0, mvmin, mvmax, mvp, numc, mvc, merange, *fenc_mvs[i]);
            COPY2_IF_LT(bcost, *fenc_costs[i], listused, i + 1);
        }
        if (bBidir)
        {
            pixel subpelbuf1[X265_LOWRES_CU_SIZE * X265_LOWRES_CU_SIZE], subpelbuf2[X265_LOWRES_CU_SIZE * X265_LOWRES_CU_SIZE];
            pixel *src0, *src1;
            intptr_t stride0, stride1;
            if (((*fenc_mvs[0]).x | (*fenc_mvs[0]).y) & 1)
            {
                int hpelA = ((*fenc_mvs[0]).y & 2) | (((*fenc_mvs[0]).x & 2) >> 1);
                pixel *frefA = fref0->lowresPlane[hpelA] + pelOffset + ((*fenc_mvs[0]).x >> 2) + ((*fenc_mvs[0]).y >> 2) * fref0->lumaStride;

                MV qmvB = (*fenc_mvs[0]) + MV(((*fenc_mvs[0]).x & 1) * 2, ((*fenc_mvs[0]).y & 1) * 2);
                int hpelB = (qmvB.y & 2) | ((qmvB.x & 2) >> 1);
                pixel *frefB = fref0->lowresPlane[hpelB] + pelOffset + (qmvB.x >> 2) + (qmvB.y >> 2) * fref0->lumaStride;

                primitives.pixelavg_pp[LUMA_8x8](subpelbuf1, X265_LOWRES_CU_SIZE, frefA, fref0->lumaStride, frefB, fref0->lumaStride, 32);
                src0 = subpelbuf1;
                stride0 = X265_LOWRES_CU_SIZE;
            }
            else
            {
                /* FPEL/HPEL */
                int hpel = ((*fenc_mvs[0]).y & 2) | (((*fenc_mvs[0]).x & 2) >> 1);
                src0 = fref0->lowresPlane[hpel] + pelOffset + ((*fenc_mvs[0]).x >> 2) + ((*fenc_mvs[0]).y >> 2) * fref0->lumaStride;
                stride0 = fref0->lumaStride;
            }
            if (((*fenc_mvs[1]).x | (*fenc_mvs[1]).y) & 1)
            {
                int hpelA = ((*fenc_mvs[1]).y & 2) | (((*fenc_mvs[1]).x & 2) >> 1);
                pixel *frefA = fref1->lowresPlane[hpelA] + pelOffset + ((*fenc_mvs[1]).x >> 2) + ((*fenc_mvs[1]).y >> 2) * fref1->lumaStride;

                MV qmvB = (*fenc_mvs[1]) + MV(((*fenc_mvs[1]).x & 1) * 2, ((*fenc_mvs[1]).y & 1) * 2);
                int hpelB = (qmvB.y & 2) | ((qmvB.x & 2) >> 1);
                pixel *frefB = fref1->lowresPlane[hpelB] + pelOffset + (qmvB.x >> 2) + (qmvB.y >> 2) * fref1->lumaStride;

                primitives.pixelavg_pp[LUMA_8x8](subpelbuf2, X265_LOWRES_CU_SIZE, frefA, fref1->lumaStride, frefB, fref1->lumaStride, 32);
                src1 = subpelbuf2;
                stride1 = X265_LOWRES_CU_SIZE;
            }
            else
            {
                int hpel = ((*fenc_mvs[1]).y & 2) | (((*fenc_mvs[1]).x & 2) >> 1);
                src1 = fref1->lowresPlane[hpel] + pelOffset + ((*fenc_mvs[1]).x >> 2) + ((*fenc_mvs[1]).y >> 2) * fref1->lumaStride;
                stride1 = fref1->lumaStride;
            }

            pixel ref[X265_LOWRES_CU_SIZE * X265_LOWRES_CU_SIZE];
            primitives.pixelavg_pp[LUMA_8x8](ref, X265_LOWRES_CU_SIZE, src0, stride0, src1, stride1, 0);

            int bicost = primitives.satd[LUMA_8x8](fenc->lowresPlane[0] + pelOffset, fenc->lumaStride, ref, X265_LOWRES_CU_SIZE);
            COPY2_IF_LT(bcost, bicost, listused, 3);

            //Try 0,0 candidates
            src0 = fref0->lowresPlane[0] + pelOffset;
            src1 = fref1->lowresPlane[0] + pelOffset;

            primitives.pixelavg_pp[LUMA_8x8](ref, X265_LOWRES_CU_SIZE, src0, fref0->lumaStride, src1, fref1->lumaStride, 0);

            bicost = primitives.satd[LUMA_8x8](fenc->lowresPlane[0] + pelOffset, fenc->lumaStride, ref, X265_LOWRES_CU_SIZE);
            COPY2_IF_LT(bcost, bicost, listused, 3);
        }
    }
    if (!fenc->bIntraCalculated)
    {
        int nLog2SizeMinus2 = g_convertToBit[cuSize]; // partition size

        pixel _above0[X265_LOWRES_CU_SIZE * 4 + 1], *const pAbove0 = _above0 + 2 * X265_LOWRES_CU_SIZE;
        pixel _above1[X265_LOWRES_CU_SIZE * 4 + 1], *const pAbove1 = _above1 + 2 * X265_LOWRES_CU_SIZE;
        pixel _left0[X265_LOWRES_CU_SIZE * 4 + 1], *const pLeft0 = _left0 + 2 * X265_LOWRES_CU_SIZE;
        pixel _left1[X265_LOWRES_CU_SIZE * 4 + 1], *const pLeft1 = _left1 + 2 * X265_LOWRES_CU_SIZE;

        pixel *pix_cur = fenc->lowresPlane[0] + pelOffset;

        // Copy Above
        memcpy(pAbove0, pix_cur - 1 - fenc->lumaStride, cuSize + 1);

        // Copy Left
        for (int i = 0; i < cuSize + 1; i++)
        {
            pLeft0[i] = pix_cur[-1 - fenc->lumaStride + i * fenc->lumaStride];
        }

        memset(pAbove0 + cuSize + 1, pAbove0[cuSize], cuSize);
        memset(pLeft0 + cuSize + 1, pLeft0[cuSize], cuSize);

        // filtering with [1 2 1]
        // assume getUseStrongIntraSmoothing() is disabled
        pAbove1[0] = pAbove0[0];
        pAbove1[2 * cuSize] = pAbove0[2 * cuSize];
        pLeft1[0] = pLeft0[0];
        pLeft1[2 * cuSize] = pLeft0[2 * cuSize];
        for (int i = 1; i < 2 * cuSize; i++)
        {
            pAbove1[i] = (pAbove0[i - 1] + 2 * pAbove0[i] + pAbove0[i + 1] + 2) >> 2;
            pLeft1[i] = (pLeft0[i - 1] + 2 * pLeft0[i] + pLeft0[i + 1] + 2) >> 2;
        }

        int predsize = cuSize * cuSize;

        // generate 35 intra predictions into tmp
        primitives.intra_pred_dc(pAbove0 + 1, pLeft0 + 1, predictions, cuSize, cuSize, (cuSize <= 16));
        pixel *above = (cuSize >= 8) ? pAbove1 : pAbove0;
        pixel *left  = (cuSize >= 8) ? pLeft1 : pLeft0;
        primitives.intra_pred_planar((pixel*)above + 1, (pixel*)left + 1, predictions + predsize, cuSize, cuSize);
        primitives.intra_pred_allangs[nLog2SizeMinus2](predictions + 2 * predsize, pAbove0, pLeft0, pAbove1, pLeft1, (cuSize <= 16));

        // calculate 35 satd costs, keep least cost
        ALIGN_VAR_32(pixel, buf_trans[32 * 32]);
        primitives.transpose[nLog2SizeMinus2](buf_trans, me.fenc, FENC_STRIDE);
        pixelcmp_t satd = primitives.satd[PartitionFromSizes(cuSize, cuSize)];
        int icost = me.COST_MAX, cost;
        for (UInt mode = 0; mode < 35; mode++)
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
            if (fenc->m_qpAqOffset)
                costIntraAq += (icost * x265_exp2fix8(fenc->m_qpAqOffset[cuXY]) + 128) >> 8;
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
            if (fenc->m_qpAqOffset)
                costEstAq += (bcost * x265_exp2fix8(fenc->m_qpAqOffset[cuXY]) + 128) >> 8;
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

            if (frm.sliceType == X265_TYPE_BREF
                /* && h->param.i_bframe_pyramid < X264_B_PYRAMID_NORMAL && brefs == h->param.i_bframe_pyramid*/)
            {
                frm.sliceType = X265_TYPE_B;
                x265_log(&cfg->param, X265_LOG_WARNING, "B-ref is not yet supported\n");
            }

            /* pyramid with multiple B-refs needs a big enough dpb that the preceding P-frame stays available.
               smaller dpb could be supported by smart enough use of mmco, but it's easier just to forbid it.
            else if (frm.sliceType == X265_TYPE_BREF && cfg->param.i_bframe_pyramid == X265_B_PYRAMID_NORMAL &&
                     brefs && cfg->param.i_frame_reference <= (brefs+3))
            {
                frm.sliceType = X265_TYPE_B;
                x265_log(&cfg->param, X265_LOG_WARNING, "B-ref at frame %d incompatible with B-pyramid %s and %d reference frames\n",
                          frm.sliceType, x264_b_pyramid_names[h->param.i_bframe_pyramid], h->param.i_frame_reference);
            } */

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

        /* insert a bref into the sequence
        if (h->param.i_bframe_pyramid && bframes > 1 && !brefs)
        {
            h->lookahead->next.list[bframes/2]->i_type = X264_TYPE_BREF;
            brefs++;
        } */

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

            /*
            if ((p0 != p1 || bframes) && cfg->param.rc.i_vbv_buffer_size)
            {
                // We need the intra costs for row SATDs
                estimateFrameCost(b, b, b, 0);

                // We need B-frame costs for row SATDs
                p0 = 0;
                for (b = 1; b <= bframes; b++)
                {
                    if (frames[b]->i_type == X265_TYPE_B)
                        for (p1 = b; frames[p1]->sliceType == X265_TYPE_B;)
                            p1++;
                    else
                        p1 = bframes + 1;
                    estimateFrameCost( p0, p1, b, 0 );
                    if (frames[b]->sliceType == X265_TYPE_BREF)
                        p0 = b;
                }
            } */
        }

        /* Analyse for weighted P frames
        if (!h->param.rc.b_stat_read && h->lookahead->next.list[bframes]->i_type == X264_TYPE_P
            && h->param.analyse.i_weighted_pred >= X264_WEIGHTP_SIMPLE)
        {
            x265_emms();
            x264_weights_analyse(h, h->lookahead->next.list[bframes], h->lookahead->last_nonb, 0);
        }*/

        /* dequeue all frames from inputQueue that are about to be enqueued
         * in the output queue.  The order is important because TComPic can
         * only be in one list at a time */
        for (int i = 0; i <= bframes; i++)
        {
            inputQueue.popFront();
        }

        /* add non-B to output queue */
        outputQueue.pushBack(*list[bframes]);
        /* add B frames to output queue */
        for (int i = 0; i < bframes; i++)
        {
            outputQueue.pushBack(*list[i]);
        }

        return;
    }

    // Fixed GOP structures for when B-Adapt and/or lookahead are disabled
    if (numDecided == 0 || cfg->param.keyframeMax == 1)
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
                pic->m_lowres.sliceType = X265_TYPE_I;
                pic->m_lowres.bKeyframe = true;
                lastKeyframe = pic->m_lowres.frameNum;
                if (j) list[j - 1]->m_lowres.sliceType = X265_TYPE_P;
                j++;
                break;
            }
        }

        TComPic *pic = list[j - 1];
        if (pic->m_lowres.sliceType == X265_TYPE_AUTO)
            pic->m_lowres.sliceType = X265_TYPE_P;
        outputQueue.pushBack(*pic);
        numDecided++;
        for (int i = 0; i < j - 1; i++)
        {
            pic = list[i];
            if (pic->m_lowres.sliceType == X265_TYPE_AUTO)
                pic->m_lowres.sliceType = X265_TYPE_B;
            outputQueue.pushBack(*pic);
            numDecided++;
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
        // TODO: mb-tree
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

    // TODO if rc.b_mb_tree Enabled the need to call  x264_macroblock_tree currently Ignored the call
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

        /* Keep some B-frames as references: 0=off, 1=strict hierarchical, 2=normal */
        //TODO Add this into param
        int bframe_pyramid = 0;

        if (bframe_pyramid && next_p - cur_p > 2)
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

    if (!fenc->bIntraCalculated)
        fenc->rowSatds[0][0][realrow] = 0;
    fenc->rowSatds[curb - curp0][curp1 - curb][realrow] = 0;

    /* Lowres lookahead goes backwards because the MVs are used as
     * predictors in the main encode.  This considerably improves MV
     * prediction overall. */
    for (int i = widthInCU - 1 - lhrows[row].completed; i >= 0; i--)
    {
        lhrows[row].estimateCUCost(i, realrow, curp0, curp1, curb, bDoSearch);
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
