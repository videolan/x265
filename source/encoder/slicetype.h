/*****************************************************************************
 * Copyright (C) 2013-2020 MulticoreWare, Inc
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Min Chen <chenm003@163.com>
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
#include "threadpool.h"

namespace X265_NS {
// private namespace

struct Lowres;
class Frame;
class Lookahead;

#define LOWRES_COST_MASK  ((1 << 14) - 1)
#define LOWRES_COST_SHIFT 14
#define AQ_EDGE_BIAS 0.5
#define EDGE_INCLINATION 45
#define TEMPORAL_SCENECUT_THRESHOLD 50

#define X265_ABS(a)                        (((a) < 0) ? (-(a)) : (a))

#define PICTURE_DIFF_VARIANCE_TH            390
#define PICTURE_VARIANCE_TH                 1500
#define LOW_VAR_SCENE_CHANGE_TH             2250
#define HIGH_VAR_SCENE_CHANGE_TH            3500

#define PICTURE_DIFF_VARIANCE_CHROMA_TH     10
#define PICTURE_VARIANCE_CHROMA_TH          20
#define LOW_VAR_SCENE_CHANGE_CHROMA_TH      2250/4
#define HIGH_VAR_SCENE_CHANGE_CHROMA_TH     3500/4

#define FLASH_TH                            1.5
#define FADE_TH                             4
#define INTENSITY_CHANGE_TH                 4

#define NUM64x64INPIC(w,h)                  ((w*h)>> (MAX_LOG2_CU_SIZE<<1))

#if HIGH_BIT_DEPTH
#define EDGE_THRESHOLD 1023.0
#else
#define EDGE_THRESHOLD 255.0
#endif
#define PI 3.14159265

/* Thread local data for lookahead tasks */
struct LookaheadTLD
{
    MotionEstimate  me;
    pixel*          wbuffer[4];
    int             widthInCU;
    int             heightInCU;
    int             ncu;
    int             paddedLines;

#if DETAILED_CU_STATS
    int64_t         batchElapsedTime;
    int64_t         coopSliceElapsedTime;
    uint64_t        countBatches;
    uint64_t        countCoopSlices;
#endif

    LookaheadTLD()
    {
        me.init(X265_CSP_I400);
        me.setQP(X265_LOOKAHEAD_QP);
        for (int i = 0; i < 4; i++)
            wbuffer[i] = NULL;
        widthInCU = heightInCU = ncu = paddedLines = 0;

#if DETAILED_CU_STATS
        batchElapsedTime = 0;
        coopSliceElapsedTime = 0;
        countBatches = 0;
        countCoopSlices = 0;
#endif
    }

    void init(int w, int h, int n)
    {
        widthInCU = w;
        heightInCU = h;
        ncu = n;
    }

    ~LookaheadTLD() { X265_FREE(wbuffer[0]); }

    void collectPictureStatistics(Frame *curFrame);
    void computeIntensityHistogramBinsLuma(Frame *curFrame, uint64_t *sumAvgIntensityTotalSegmentsLuma);

    void computeIntensityHistogramBinsChroma(
        Frame    *curFrame,
        uint64_t *sumAverageIntensityCb,
        uint64_t *sumAverageIntensityCr);

    void calculateHistogram(
        pixel    *inputSrc,
        uint32_t  inputWidth,
        uint32_t  inputHeight,
        intptr_t  stride,
        uint8_t   dsFactor,
        uint32_t *histogram,
        uint64_t *sum);

    void computePictureStatistics(Frame *curFrame);

    uint32_t calcVariance(pixel* src, intptr_t stride, intptr_t blockOffset, uint32_t plane);

    void calcAdaptiveQuantFrame(Frame *curFrame, x265_param* param);
    void calcFrameSegment(Frame *curFrame);
    void lowresIntraEstimate(Lowres& fenc, uint32_t qgSize);

    void weightsAnalyse(Lowres& fenc, Lowres& ref);
    void xPreanalyze(Frame* curFrame);
    void xPreanalyzeQp(Frame* curFrame);
protected:

    uint32_t acEnergyCu(Frame* curFrame, uint32_t blockX, uint32_t blockY, int csp, uint32_t qgSize);
    uint32_t edgeDensityCu(Frame* curFrame, uint32_t &avgAngle, uint32_t blockX, uint32_t blockY, uint32_t qgSize);
    uint32_t lumaSumCu(Frame* curFrame, uint32_t blockX, uint32_t blockY, uint32_t qgSize);
    uint32_t weightCostLuma(Lowres& fenc, Lowres& ref, WeightParam& wp);
    bool     allocWeightedRef(Lowres& fenc);
};

class Lookahead : public JobProvider
{
public:

    PicList       m_inputQueue;      // input pictures in order received
    PicList       m_outputQueue;     // pictures to be encoded, in encode order
    Lock          m_inputLock;
    Lock          m_outputLock;
    Event         m_outputSignal;
    LookaheadTLD* m_tld;
    x265_param*   m_param;
    Lowres*       m_lastNonB;
    int*          m_scratch;         // temp buffer for cutree propagate

    /* pre-lookahead */
    int           m_fullQueueSize;
    int           m_lastKeyframe;
    int           m_8x8Width;
    int           m_8x8Height;
    int           m_8x8Blocks;
    int           m_cuCount;
    int           m_numCoopSlices;
    int           m_numRowsPerSlice;
    int           m_inputCount;
    double        m_cuTreeStrength;

    /* HME */
    int           m_4x4Width;
    int           m_4x4Height;

    bool          m_isActive;
    bool          m_sliceTypeBusy;
    bool          m_bAdaptiveQuant;
    bool          m_outputSignalRequired;
    bool          m_bBatchMotionSearch;
    bool          m_bBatchFrameCosts;
    bool          m_filled;
    bool          m_isSceneTransition;
    int           m_numPools;
    bool          m_extendGopBoundary;
    double        m_frameVariance[X265_BFRAME_MAX + 4];
    bool          m_isFadeIn;
    uint64_t      m_fadeCount;
    int           m_fadeStart;

    uint32_t    **m_accHistDiffRunningAvgCb;
    uint32_t    **m_accHistDiffRunningAvgCr;
    uint32_t    **m_accHistDiffRunningAvg;

    bool          m_resetRunningAvg;
    uint32_t      m_segmentCountThreshold;

    int8_t                  m_gopId;

    Lookahead(x265_param *param, ThreadPool *pool);
#if DETAILED_CU_STATS
    int64_t       m_slicetypeDecideElapsedTime;
    int64_t       m_preLookaheadElapsedTime;
    uint64_t      m_countSlicetypeDecide;
    uint64_t      m_countPreLookahead;
    void          getWorkerStats(int64_t& batchElapsedTime, uint64_t& batchCount, int64_t& coopSliceElapsedTime, uint64_t& coopSliceCount);
#endif

    bool    create();
    void    destroy();
    void    stopJobs();

    void    addPicture(Frame&, int sliceType);
    void    addPicture(Frame& curFrame);
    void    checkLookaheadQueue(int &frameCnt);
    void    flush();
    Frame*  getDecidedPicture();

    void    getEstimatedPictureCost(Frame *pic);
    void    setLookaheadQueue();
    int     findSliceType(int poc);

protected:

    void    findJob(int workerThreadID);
    void    slicetypeDecide();
    void    slicetypeAnalyse(Lowres **frames, bool bKeyframe);

    /* called by slicetypeAnalyse() to make slice decisions */
    bool    scenecut(Lowres **frames, int p0, int p1, bool bRealScenecut, int numFrames);
    bool    scenecutInternal(Lowres **frames, int p0, int p1, bool bRealScenecut);

    bool    histBasedScenecut(Lowres **frames, int p0, int p1, int numFrames);
    bool    detectHistBasedSceneChange(Lowres **frames, int p0, int p1, int p2);

    void    slicetypePath(Lowres **frames, int length, char(*best_paths)[X265_LOOKAHEAD_MAX + 1]);
    int64_t slicetypePathCost(Lowres **frames, char *path, int64_t threshold);
    int64_t vbvFrameCost(Lowres **frames, int p0, int p1, int b);
    void    vbvLookahead(Lowres **frames, int numFrames, int keyframes);
    void    aqMotion(Lowres **frames, bool bintra);
    void    calcMotionAdaptiveQuantFrame(Lowres **frames, int p0, int p1, int b);
    /* called by slicetypeAnalyse() to effect cuTree adjustments to adaptive
     * quant offsets */
    void    cuTree(Lowres **frames, int numframes, bool bintra);
    void    estimateCUPropagate(Lowres **frames, double average_duration, int p0, int p1, int b, int referenced);
    void    cuTreeFinish(Lowres *frame, double averageDuration, int ref0Distance);
    void    computeCUTreeQpOffset(Lowres *frame, double averageDuration, int ref0Distance);

    /* called by getEstimatedPictureCost() to finalize cuTree costs */
    int64_t frameCostRecalculate(Lowres **frames, int p0, int p1, int b);
    /*Compute index for positioning B-Ref frames*/
    void     placeBref(Frame** frames, int start, int end, int num, int *brefs);
    void     compCostBref(Lowres **frame, int start, int end, int num);
};

class PreLookaheadGroup : public BondedTaskGroup
{
public:

    Frame* m_preframes[X265_LOOKAHEAD_MAX];
    Lookahead& m_lookahead;

    PreLookaheadGroup(Lookahead& l) : m_lookahead(l) {}

    void processTasks(int workerThreadID);

protected:

    PreLookaheadGroup& operator=(const PreLookaheadGroup&);
};

class CostEstimateGroup : public BondedTaskGroup
{
public:

    Lookahead& m_lookahead;
    Lowres**   m_frames;
    bool       m_batchMode;

    CostEstimateGroup(Lookahead& l, Lowres** f) : m_lookahead(l), m_frames(f), m_batchMode(false) {}

    /* Cooperative cost estimate using multiple slices of downscaled frame */
    struct Coop
    {
        int  p0, b, p1;
        bool bDoSearch[2];
    } m_coop;

    enum { MAX_COOP_SLICES = 32 };
    struct Slice
    {
        int  costEst;
        int  costEstAq;
        int  intraMbs;
    } m_slice[MAX_COOP_SLICES];

    int64_t singleCost(int p0, int p1, int b, bool intraPenalty = false);

    /* Batch cost estimates, using one worker thread per estimateFrameCost() call */
    enum { MAX_BATCH_SIZE = 512 };
    struct Estimate
    {
        int  p0, b, p1;
    } m_estimates[MAX_BATCH_SIZE];

    void add(int p0, int p1, int b);
    void finishBatch();

protected:

    static const int s_merange = 16;

    void    processTasks(int workerThreadID);

    int64_t estimateFrameCost(LookaheadTLD& tld, int p0, int p1, int b, bool intraPenalty);
    void    estimateCUCost(LookaheadTLD& tld, int cux, int cuy, int p0, int p1, int b, bool bDoSearch[2], bool lastRow, int slice, bool hme);

    CostEstimateGroup& operator=(const CostEstimateGroup&);
};

bool computeEdge(pixel* edgePic, pixel* refPic, pixel* edgeTheta, intptr_t stride, int height, int width, bool bcalcTheta, pixel whitePixel = EDGE_THRESHOLD);
}
#endif // ifndef X265_SLICETYPE_H
