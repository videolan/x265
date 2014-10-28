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
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#ifndef X265_SLICETYPE_H
#define X265_SLICETYPE_H

#include "common.h"
#include "slice.h"
#include "motion.h"
#include "piclist.h"
#include "wavefront.h"

namespace x265 {
// private namespace

struct Lowres;
class Frame;

#define LOWRES_COST_MASK  ((1 << 14) - 1)
#define LOWRES_COST_SHIFT 14

#define SET_WEIGHT(w, b, s, d, o) \
    { \
        (w).inputWeight = (s); \
        (w).log2WeightDenom = (d); \
        (w).inputOffset = (o); \
        (w).bPresentFlag = b; \
    }

class EstimateRow
{
public:
    x265_param*         m_param;
    MotionEstimate      m_me;
    Lock                m_lock;
    pixel*              m_predictions;    // buffer for 35 intra predictions

    volatile uint32_t   m_completed;      // Number of CUs in this row for which cost estimation is completed
    volatile bool       m_active;

    uint64_t            m_costEst;        // Estimated cost for all CUs in a row
    uint64_t            m_costEstAq;      // Estimated weight Aq cost for all CUs in a row
    uint64_t            m_costIntraAq;    // Estimated weighted Aq Intra cost for all CUs in a row
    int                 m_intraMbs;       // Number of Intra CUs
    int                 m_costIntra;      // Estimated Intra cost for all CUs in a row

    int                 m_merange;
    int                 m_lookAheadLambda;

    int                 m_widthInCU;
    int                 m_heightInCU;

    EstimateRow()
    {
        m_me.setQP(X265_LOOKAHEAD_QP);
        m_me.setSearchMethod(X265_HEX_SEARCH);
        m_me.setSubpelRefine(1);
        m_predictions = X265_MALLOC(pixel, 35 * 8 * 8);
        m_merange = 16;
        m_lookAheadLambda = (int)x265_lambda_tab[X265_LOOKAHEAD_QP];
    }

    ~EstimateRow()
    {
        X265_FREE(m_predictions);
    }

    void init();

    void estimateCUCost(Lowres * *frames, ReferencePlanes * wfref0, int cux, int cuy, int p0, int p1, int b, bool bDoSearch[2]);
};

/* CostEstimate manages the cost estimation of a single frame, ie:
 * estimateFrameCost() and everything below it in the call graph */
class CostEstimate : public WaveFront
{
public:
    CostEstimate(ThreadPool *p);
    ~CostEstimate();
    void init(x265_param *, Frame *);

    x265_param      *m_param;
    EstimateRow     *m_rows;
    pixel           *m_wbuffer[4];
    Lowres         **m_curframes;

    ReferencePlanes  m_weightedRef;
    WeightParam      m_w;

    int              m_paddedLines;     // number of lines in padded frame
    int              m_widthInCU;       // width of lowres frame in downscale CUs
    int              m_heightInCU;      // height of lowres frame in downscale CUs

    bool             m_bDoSearch[2];
    volatile bool    m_bFrameCompleted;
    int              m_curb, m_curp0, m_curp1;

    void     processRow(int row, int threadId);
    int64_t  estimateFrameCost(Lowres **frames, int p0, int p1, int b, bool bIntraPenalty);

protected:

    void     weightsAnalyse(Lowres **frames, int b, int p0);
    uint32_t weightCostLuma(Lowres **frames, int b, int p0, WeightParam *w);
};

class Lookahead : public JobProvider
{
public:

    Lookahead(x265_param *param, ThreadPool *pool);
    ~Lookahead();
    void init();
    void destroy();

    CostEstimate     m_est;             // Frame cost estimator
    PicList          m_inputQueue;      // input pictures in order received
    PicList          m_outputQueue;     // pictures to be encoded, in encode order

    x265_param      *m_param;
    Lowres          *m_lastNonB;
    int             *m_scratch;         // temp buffer

    int              m_widthInCU;       // width of lowres frame in downscale CUs
    int              m_heightInCU;      // height of lowres frame in downscale CUs
    int              m_lastKeyframe;
    int              m_histogram[X265_BFRAME_MAX + 1];

    void addPicture(Frame*, int sliceType);
    void flush();
    Frame* getDecidedPicture();

    void getEstimatedPictureCost(Frame *pic);

protected:

    Lock  m_inputQueueLock;
    Lock  m_outputQueueLock;
    Lock  m_decideLock;
    Event m_outputAvailable;
    volatile int  m_bReady;
    volatile bool m_bFilling;
    volatile bool m_bFlushed;
    bool findJob(int);

    /* called by addPicture() or flush() to trigger slice decisions */
    void slicetypeDecide();
    void slicetypeAnalyse(Lowres **frames, bool bKeyframe);

    /* called by slicetypeAnalyse() to make slice decisions */
    bool    scenecut(Lowres **frames, int p0, int p1, bool bRealScenecut, int numFrames, int maxSearch);
    bool    scenecutInternal(Lowres **frames, int p0, int p1, bool bRealScenecut);
    void    slicetypePath(Lowres **frames, int length, char(*best_paths)[X265_LOOKAHEAD_MAX + 1]);
    int64_t slicetypePathCost(Lowres **frames, char *path, int64_t threshold);
    int64_t vbvFrameCost(Lowres **frames, int p0, int p1, int b);
    void    vbvLookahead(Lowres **frames, int numFrames, int keyframes);

    /* called by slicetypeAnalyse() to effect cuTree adjustments to adaptive
     * quant offsets */
    void cuTree(Lowres **frames, int numframes, bool bintra);
    void estimateCUPropagate(Lowres **frames, double average_duration, int p0, int p1, int b, int referenced);
    void cuTreeFinish(Lowres *frame, double averageDuration, int ref0Distance);

    /* called by getEstimatedPictureCost() to finalize cuTree costs */
    int64_t frameCostRecalculate(Lowres **frames, int p0, int p1, int b);
};
}

#endif // ifndef X265_SLICETYPE_H
