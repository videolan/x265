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

struct EstimateRow
{
    MotionEstimate      me;
    Lock                lock;
    pixel*              predictions;    // buffer for 35 intra predictions

    volatile uint32_t   completed;      // Number of CUs in this row for which cost estimation is completed
    volatile bool       active;

    uint64_t            costEst;        // Estimated cost for all CUs in a row
    uint64_t            costEstAq;      // Estimated weight Aq cost for all CUs in a row
    uint64_t            costIntraAq;    // Estimated weighted Aq Intra cost for all CUs in a row
    int                 intraMbs;       // Number of Intra CUs
    int                 costIntra;      // Estimated Intra cost for all CUs in a row

    int                 widthInCU;
    int                 heightInCU;
    int                 merange;
    int                 lookAheadLambda;

    EstimateRow()
    {
        me.setQP(X265_LOOKAHEAD_QP);
        me.setSearchMethod(X265_HEX_SEARCH);
        me.setSubpelRefine(1);
        predictions = X265_MALLOC(pixel, 35 * 8 * 8);
        merange = 16;
        lookAheadLambda = (int)x265_lambda2_non_I[X265_LOOKAHEAD_QP];
    }

    ~EstimateRow()
    {
        X265_FREE(predictions);
    }

    void init();

    void estimateCUCost(Lowres * *frames, ReferencePlanes * wfref0, int cux, int cuy, int p0, int p1, int b, bool bDoSearch[2]);
};

/* CostEstimate manages the cost estimation of a single frame, ie:
 * estimateFrameCost() and everything below it in the call graph */
struct CostEstimate : public WaveFront
{
    CostEstimate(ThreadPool *p);
    ~CostEstimate();
    void init(TEncCfg *, TComPic *);

    TEncCfg         *cfg;
    EstimateRow     *rows;
    pixel           *wbuffer[4];
    Lowres         **curframes;

    ReferencePlanes  weightedRef;
    wpScalingParam   w;

    int              paddedLines;     // number of lines in padded frame
    int              widthInCU;       // width of lowres frame in downscale CUs
    int              heightInCU;      // height of lowres frame in downscale CUs

    bool             bDoSearch[2];
    bool             rowsCompleted;
    int              curb, curp0, curp1;

    void     processRow(int row);
    int64_t  estimateFrameCost(Lowres **frames, int p0, int p1, int b, bool bIntraPenalty);

protected:

    void     weightsAnalyse(Lowres **frames, int b, int p0);
    uint32_t weightCostLuma(Lowres **frames, int b, pixel *src, wpScalingParam *w);
};

struct Lookahead
{
    Lookahead(TEncCfg *, ThreadPool *pool);
    ~Lookahead();
    void init();
    void destroy();

    CostEstimate     est;             // Frame cost estimator
    PicList          inputQueue;      // input pictures in order received
    PicList          outputQueue;     // pictures to be encoded, in encode order

    TEncCfg         *cfg;
    Lowres          *lastNonB;
    int             *scratch;         // temp buffer

    int              widthInCU;       // width of lowres frame in downscale CUs
    int              heightInCU;      // height of lowres frame in downscale CUs
    int              lastKeyframe;

    void addPicture(TComPic*, int sliceType);
    void flush();

    int64_t getEstimatedPictureCost(TComPic *pic);

protected:

    /* called by addPicture() or flush() to trigger slice decisions */
    void slicetypeDecide();
    void slicetypeAnalyse(Lowres **frames, bool bKeyframe);

    /* called by slicetypeAnalyse() to make slice decisions */
    bool    scenecut(Lowres **frames, int p0, int p1, bool bRealScenecut, int numFrames, int maxSearch);
    bool    scenecutInternal(Lowres **frames, int p0, int p1, bool bRealScenecut);
    void    slicetypePath(Lowres **frames, int length, char(*best_paths)[X265_LOOKAHEAD_MAX + 1]);
    int64_t slicetypePathCost(Lowres **frames, char *path, int64_t threshold);

    /* called by slicetypeAnalyse() to effect cuTree adjustments to adaptive
     * quant offsets */
    void cuTree(Lowres **frames, int numframes, bool bintra);
    void estimateCUPropagate(Lowres **frames, double average_duration, int p0, int p1, int b, int referenced);
    void estimateCUPropagateCost(int *dst, uint16_t *propagateIn, int32_t *intraCosts, uint16_t *interCosts,
                                 int32_t *invQscales, double *fpsFactor, int len);
    void cuTreeFinish(Lowres *frame, double averageDuration, int ref0Distance);

    /* called by getEstimatedPictureCost() to finalize cuTree costs */
    int64_t frameCostRecalculate(Lowres **frames, int p0, int p1, int b);
};
}

#endif // ifndef X265_SLICETYPE_H
