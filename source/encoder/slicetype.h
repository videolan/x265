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

#ifndef X265_SLICETYPE_H
#define X265_SLICETYPE_H

#include "motion.h"
#include "piclist.h"
#include "wavefront.h"

namespace x265 {
// private namespace

struct Lowres;
class TComPic;
class TEncCfg;

#define SET_WEIGHT(w, b, s, d, o) \
    { \
        (w).inputWeight = (s); \
        (w).log2WeightDenom = (d); \
        (w).inputOffset = (o); \
        (w).bPresentFlag = b; \
    }

struct LookaheadRow
{
    Lock                lock;
    volatile bool       active;
    volatile uint32_t   completed;      // Number of CUs in this row for which cost estimation is completed
    pixel*              predictions;    // buffer for 35 intra predictions
    MotionEstimate      me;
    int                 costEst;        // Estimated cost for all CUs in a row
    int                 costEstAq;      // Estimated weight Aq cost for all CUs in a row
    int                 costIntra;      // Estimated Intra cost for all CUs in a row
    int                 costIntraAq;    // Estimated weighted Aq Intra cost for all CUs in a row
    int                 intraMbs;       // Number of Intra CUs

    int                 widthInCU;
    int                 heightInCU;
    int                 merange;

    LookaheadRow()
    {
        me.setQP(X265_LOOKAHEAD_QP);
        me.setSearchMethod(X265_HEX_SEARCH);
        me.setSubpelRefine(1);
        predictions = (pixel*)X265_MALLOC(pixel, 35 * 8 * 8);
        merange = 16;
    }

    ~LookaheadRow()
    {
        X265_FREE(predictions);
    }

    void init();

    void estimateCUCost(Lowres * *frames, ReferencePlanes * wfref0, int cux, int cuy, int p0, int p1, int b, bool bDoSearch[2]);
};

struct Lookahead : public WaveFront
{
    TEncCfg         *cfg;
    Lowres          *frames[X265_LOOKAHEAD_MAX];
    Lowres          *lastNonB;
    int              numDecided;
    int              lastKeyframe;
    int              widthInCU;       // width of lowres frame in downscale CUs
    int              heightInCU;      // height of lowres frame in downscale CUs

    ReferencePlanes  weightedRef;
    pixel           *wbuffer[4];
    int              paddedLines;

    PicList inputQueue;  // input pictures in order received
    PicList outputQueue; // pictures to be encoded, in encode order

    bool bDoSearch[2];
    int curb, curp0, curp1;
    bool rowsCompleted;

    int *scratch; // temp buffer

    LookaheadRow* lhrows;

    Lookahead(TEncCfg *, ThreadPool *);
    ~Lookahead();

    void init();
    void addPicture(TComPic*, int sliceType);
    void flush();
    void destroy();
    void slicetypeDecide();
    int getEstimatedPictureCost(TComPic *pic);

    int estimateFrameCost(int p0, int p1, int b, bool bIntraPenalty);

    void slicetypeAnalyse(bool bKeyframe);
    int scenecut(int p0, int p1, bool bRealScenecut, int numFrames, int maxSearch);
    int scenecutInternal(int p0, int p1, bool bRealScenecut);
    void slicetypePath(int length, char(*best_paths)[X265_LOOKAHEAD_MAX + 1]);
    int slicetypePathCost(char *path, int threshold);

    void processRow(int row);

    void weightsAnalyse(int b, int p0);
    uint32_t weightCostLuma(int b, pixel *src, wpScalingParam *w);

    void cuTree(Lowres **Frames, int numframes, bool bintra);
    void estimateCUPropagate(Lowres **Frames, double average_duration, int p0, int p1, int b, int referenced);
    void estimateCUPropagateCost(int *dst, uint16_t *propagateIn, int32_t *intraCosts, uint16_t *interCosts, int32_t *invQscales, double *fpsFactor, int len);
    void cuTreeFinish(Lowres *Frame, double averageDuration, int ref0Distance);
    int frameCostRecalculate(Lowres **Frames, int p0, int p1, int b);
};
}

#endif // ifndef X265_SLICETYPE_H
