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

static inline int16_t x265_median(int16_t a, int16_t b, int16_t c)
{
    int16_t t = (a - b) & ((a - b) >> 31);

    a -= t;
    b += t;
    b -= (b - c) & ((b - c) >> 31);
    b += (a - b) & ((a - b) >> 31);
    return b;
}

static inline void x265_median_mv(MV &dst, MV a, MV b, MV c)
{
    dst.x = x265_median(a.x, b.x, c.x);
    dst.y = x265_median(a.y, b.y, c.y);
}

Lookahead::Lookahead(TEncCfg *_cfg)
{
    this->cfg = _cfg;
    numDecided = 0;
    predictions = (pixel*)X265_MALLOC(pixel, 35 * 8 * 8);
    me.setQP(X265_LOOKAHEAD_QP);
    me.setSearchMethod(X265_HEX_SEARCH);
    me.setSubpelRefine(1);
    merange = 16;
    widthInCU = ((cfg->param.sourceWidth / 2) + X265_LOWRES_CU_SIZE - 1) >> X265_LOWRES_CU_BITS;
    heightInCU = ((cfg->param.sourceHeight / 2) + X265_LOWRES_CU_SIZE - 1) >> X265_LOWRES_CU_BITS;
}

Lookahead::~Lookahead()
{
}

void Lookahead::destroy()
{
    if (predictions)
        X265_FREE(predictions);

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

void Lookahead::addPicture(TComPic *pic)
{
    pic->m_lowres.init(pic->getPicYuvOrg(), cfg->param.bframes);
    pic->m_lowres.frameNum = pic->getSlice()->getPOC();

    inputQueue.pushBack(pic);
    if (inputQueue.size() >= (size_t)cfg->param.lookaheadDepth)
        slicetypeDecide();
}

void Lookahead::flush()
{
    if (!inputQueue.empty())
        slicetypeDecide();
}

void Lookahead::slicetypeDecide()
{
    if (numDecided == 0)
    {
        // Special case for POC 0, send directly to output queue as I slice
        TComPic *pic = inputQueue.popFront();
        pic->m_lowres.sliceType = X265_TYPE_I;
        outputQueue.pushBack(pic);
        numDecided++;
        lastKeyframe = 0;
        pic->m_lowres.keyframe = 1;
        frames[0] = &(pic->m_lowres);
        return;
    }
    else if (cfg->param.bFrameAdaptive && cfg->param.lookaheadDepth && cfg->param.bframes)
    {
        slicetypeAnalyse(false);

        int dframes;
        TComPic* picsAnalysed[X265_LOOKAHEAD_MAX];  //Used for sorting the pics into encode order
        int idx = 1;

        for (dframes = 0; (frames[dframes + 1] != NULL) && (frames[dframes + 1]->sliceType != X265_TYPE_AUTO); dframes++)
        {
            if (frames[dframes + 1]->sliceType == X265_TYPE_I)
            {
                frames[dframes + 1]->keyframe = 1;
                lastKeyframe = frames[dframes]->frameNum;
                if (cfg->param.decodingRefreshType == 2 && dframes > 0) //If an IDR frame following a B
                {
                    frames[dframes]->sliceType = X265_TYPE_P;
                    dframes--;
                }
            }
            if (!IS_X265_TYPE_B(frames[dframes + 1]->sliceType))
            {
                dframes++;
                break;
            }
        }

        TComPic *pic = NULL;
        for (int i = 1; i <= dframes && !inputQueue.empty(); i++)
        {
            pic = inputQueue.popFront();
            picsAnalysed[idx++] = pic;
        }

        picsAnalysed[0] = pic;  //Move the P-frame following B-frames to the beginning

        //Push pictures in encode order
        for (int i = 0; i < dframes; i++)
        {
            outputQueue.pushBack(picsAnalysed[i]);
        }

        if (pic)
            frames[0] = &(pic->m_lowres); // last nonb
        return;
    }

    // Fixed GOP structures for when B-Adapt and/or lookahead are disabled
    if (cfg->param.keyframeMax == 1)
    {
        TComPic *pic = inputQueue.popFront();

        pic->m_lowres.sliceType = X265_TYPE_I;
        pic->m_lowres.keyframe = 1;
        outputQueue.pushBack(pic);
        numDecided++;
    }
    else if (cfg->param.bframes == 0 || inputQueue.size() == 1)
    {
        TComPic *pic = inputQueue.popFront();

        bool forceIntra = (pic->getPOC() % cfg->param.keyframeMax) == 0;
        pic->m_lowres.sliceType = forceIntra ? X265_TYPE_I : X265_TYPE_P;
        pic->m_lowres.keyframe = forceIntra ? 1 : 0;
        outputQueue.pushBack(pic);
        numDecided++;
    }
    else
    {
        TComPic *picB = inputQueue.popFront();
        TComPic *picP = inputQueue.popFront();

        bool forceIntra = (picP->getPOC() % cfg->param.keyframeMax) == 0 || (picB->getPOC() % cfg->param.keyframeMax) == 0;
        if (forceIntra)
        {
            picB->m_lowres.sliceType = (picB->getPOC() % cfg->param.keyframeMax) ? X265_TYPE_P : X265_TYPE_I;
            picB->m_lowres.keyframe = (picB->getPOC() % cfg->param.keyframeMax) ? 0 : 1;
            outputQueue.pushBack(picB);
            numDecided++;

            picP->m_lowres.sliceType = (picP->getPOC() % cfg->param.keyframeMax) ? X265_TYPE_P : X265_TYPE_I;
            picP->m_lowres.keyframe = (picP->getPOC() % cfg->param.keyframeMax) ? 0 : 1;
            outputQueue.pushBack(picP);
            numDecided++;
        }
        else
        {
            picP->m_lowres.sliceType = X265_TYPE_P;
            outputQueue.pushBack(picP);
            numDecided++;

            picB->m_lowres.sliceType = X265_TYPE_B;
            outputQueue.pushBack(picB);
            numDecided++;
        }
    }
}

// Called by RateControl to get the estimated SATD cost for a given picture.
// It assumes dpb->prepareEncode() has already been called for the picture and
// all the references are established
int Lookahead::getEstimatedPictureCost(TComPic *pic)
{
    // POC distances to each reference
    int d0, d1;
    int poc = pic->getSlice()->getPOC();
    int l0poc = pic->getSlice()->getRefPOC(REF_PIC_LIST_0, 0);
    int l1poc = pic->getSlice()->getRefPOC(REF_PIC_LIST_1, 0);

    switch (pic->getSlice()->getSliceType())
    {
    case I_SLICE:
        frames[0] = &pic->m_lowres;
        return estimateFrameCost(0, 0, 0, false);
    case P_SLICE:
        d0 = poc - l0poc;
        frames[0] = &pic->getSlice()->getRefPic(REF_PIC_LIST_0, 0)->m_lowres;
        frames[d0] = &pic->m_lowres;
        return estimateFrameCost(0, d0, d0, false);
    case B_SLICE:
        d0 = poc - l0poc;
        if (l1poc > poc)
        {
            // L1 reference is truly in the future
            d1 = l1poc - poc;
            frames[0] = &pic->getSlice()->getRefPic(REF_PIC_LIST_0, 0)->m_lowres;
            frames[d0] = &pic->m_lowres;
            frames[d0 + d1] = &pic->getSlice()->getRefPic(REF_PIC_LIST_1, 0)->m_lowres;
            return estimateFrameCost(0, d0 + d1, d0, false);
        }
        else
        {
            frames[0] = &pic->getSlice()->getRefPic(REF_PIC_LIST_0, 0)->m_lowres;
            frames[d0] = &pic->m_lowres;
            return estimateFrameCost(0, d0, d0, false);
        }
    }

    return -1;
}

#define NUM_CUS (widthInCU > 2 && heightInCU > 2 ? (widthInCU - 2) * (heightInCU - 2) : widthInCU * heightInCU)

int Lookahead::estimateFrameCost(int p0, int p1, int b, bool bIntraPenalty)
{
    int score = 0;
    bool bDoSearch[2];
    Lowres *fenc = frames[b];

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

        /* Lowres lookahead goes backwards because the MVs are used as
         * predictors in the main encode.  This considerably improves MV
         * prediction overall. */
        // TODO: use lowres MVs as motion candidates in full-res search
        me.setSourcePlane(fenc->lowresPlane[0], fenc->lumaStride);
        for (int j = heightInCU - 1; j >= 0; j--)
        {
            if (!fenc->bIntraCalculated)
                fenc->rowSatds[0][0][j] = 0;
            fenc->rowSatds[b - p0][p1 - b][j] = 0;

            for (int i = widthInCU - 1; i >= 0; i--)
            {
                estimateCUCost(i, j, p0, p1, b, bDoSearch);
            }
        }

        fenc->bIntraCalculated = true;

        score = fenc->costEst[b - p0][p1 - b];

        if (b != p1)
            score = (uint64_t)score * 100 / (130 + cfg->param.bFrameBias);

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

void Lookahead::estimateCUCost(int cux, int cuy, int p0, int p1, int b, bool bDoSearch[2])
{
    Lowres *fref0 = frames[p0];
    Lowres *fref1 = frames[p1];
    Lowres *fenc  = frames[b];

    const bool bBidir = (b < p1);
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

    for (int i = 0; i < 1 + bBidir; i++)
    {
        if (!bDoSearch[i])
            continue;

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
            x265_median_mv(mvp, mvc[0], mvc[1], mvc[2]);
        }

        *fenc_costs[i] = me.motionEstimate(i ? fref1 : fref0, mvmin, mvmax, mvp, numc, mvc, merange, *fenc_mvs[i]);
        COPY2_IF_LT(bcost, *fenc_costs[i], listused, i + 1);
    }

    if (!fenc->bIntraCalculated)
    {
        int nLog2SizeMinus2 = g_convertToBit[cuSize]; // partition size

        pixel _above0[32 * 4 + 1], *const pAbove0 = _above0 + 2 * 32;
        pixel _above1[32 * 4 + 1], *const pAbove1 = _above1 + 2 * 32;
        pixel _left0[32 * 4 + 1], *const pLeft0 = _left0 + 2 * 32;
        pixel _left1[32 * 4 + 1], *const pLeft1 = _left1 + 2 * 32;

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
        pAbove1[cuSize - 1] = pAbove0[cuSize - 1];
        pLeft1[0] = pLeft0[0];
        pLeft1[cuSize - 1] = pLeft0[cuSize - 1];
        for (int i = 1; i < cuSize - 1; i++)
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
        if (bFrameScoreCU) fenc->costEst[0][0] += icost;
    }

    if (!bBidir)
    {
        if (fenc->intraCost[cuXY] < bcost)
        {
            if (bFrameScoreCU) fenc->intraMbs[b - p0]++;
            bcost = fenc->intraCost[cuXY];
            listused = 0;
        }
    }

    /* For I frames these costs were accumulated earlier */
    if (p0 != p1)
    {
        fenc->rowSatds[b - p0][p1 - b][cuy] += bcost;
        if (bFrameScoreCU) fenc->costEst[b - p0][p1 - b] += bcost;
    }
    fenc->lowresCosts[b - p0][p1 - b][cuXY] = (uint16_t)(X265_MIN(bcost, LOWRES_COST_MASK) | (listused << LOWRES_COST_SHIFT));
}

void Lookahead::slicetypeAnalyse(bool bKeyframe)
{
    int num_frames, origNumFrames, keyint_limit, framecnt;
    int maxSearch = X265_MIN(cfg->param.lookaheadDepth, X265_LOOKAHEAD_MAX);
    int cuCount = NUM_CUS;
    int cost1p0, cost2p0, cost1b1, cost2p1;
    int reset_start;
    int vbv_lookahead = 0;

    TComList<TComPic*>::iterator iterPic = inputQueue.begin();
    for (framecnt = 0; (framecnt < maxSearch) && (framecnt < (int)inputQueue.size()) && (*iterPic)->m_lowres.sliceType == X265_TYPE_AUTO; framecnt++)
    {
        frames[framecnt + 1] = &((*iterPic++)->m_lowres);
    }

    if (!framecnt)
    {
        frames[1] = &((*iterPic)->m_lowres);
        frames[2] = NULL;
        return;
    }

    frames[framecnt + 1] = NULL;

    keyint_limit = cfg->param.keyframeMax - frames[1]->frameNum + lastKeyframe;
    origNumFrames = num_frames = X265_MIN(framecnt, keyint_limit);

    /* This is important psy-wise: if we have a non-scenecut keyframe,
     * there will be significant visual artifacts if the frames just before
     * go down in quality due to being referenced less, despite it being
     * more RD-optimal. */

    if (vbv_lookahead)
        num_frames = framecnt;
    else if (num_frames < framecnt)
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

        reset_start = !bKeyframe + 1;
        num_bframes = 0;
    }

    // TODO if rc.b_mb_tree Enabled the need to call  x264_macroblock_tree currently Ignored the call

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
        for (int curp1 = p1; curp1 <= maxp1; curp1++)
        {
            if (!scenecutInternal(p0, curp1, 0))
                /* Any frame in between p0 and cur_p1 cannot be a real scenecut. */
                for (int i = curp1; i > p0; i--)
                {
                    frames[i]->scenecut = 0;
                }
        }

        /* Where A-F are scenes: AAAAABBCCDDEEFFFFFF
         * If each of BB ... EE are shorter than (maxp1-p0), they are
         * detected as flashes and not considered scenecuts.
         * Instead, the first F frame becomes a scenecut.
         * If the video ends before F, no frame becomes a scenecut. */
        for (int curp0 = p0; curp0 <= maxp1; curp0++)
        {
            if (origmaxp1 > maxSearch || (curp0 < maxp1 && scenecutInternal(curp0, maxp1, 0)))
                /* If cur_p0 is the p0 of a scenecut, it cannot be the p1 of a scenecut. */
                frames[curp0]->scenecut = 0;
        }
    }

    /* Ignore frames that are part of a flash, i.e. cannot be real scenecuts. */
    if (!frames[p1]->scenecut)
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
    int best_cost = me.COST_MAX;
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
