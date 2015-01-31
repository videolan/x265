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

#ifndef X265_SEARCH_H
#define X265_SEARCH_H

#include "common.h"
#include "predict.h"
#include "quant.h"
#include "bitcost.h"
#include "framedata.h"
#include "yuv.h"
#include "threadpool.h"

#include "rdcost.h"
#include "entropy.h"
#include "motion.h"

#if DETAILED_CU_STATS
#define ProfileCUScopeNamed(name, cu, acc, count) \
    m_stats[cu.m_encData->m_frameEncoderID].count++; \
    ScopedElapsedTime name(m_stats[cu.m_encData->m_frameEncoderID].acc)
#define ProfileCUScope(cu, acc, count) ProfileCUScopeNamed(timedScope, cu, acc, count)
#else
#define ProfileCUScopeNamed(name, cu, acc, count)
#define ProfileCUScope(cu, acc, count)
#endif

namespace x265 {
// private namespace

class Entropy;
struct ThreadLocalData;

/* All the CABAC contexts that Analysis needs to keep track of at each depth
 * and temp buffers for residual, coeff, and recon for use during residual
 * quad-tree depth recursion */
struct RQTData
{
    Entropy  cur;     /* starting context for current CU */

    /* these are indexed by qtLayer (log2size - 2) so nominally 0=4x4, 1=8x8, 2=16x16, 3=32x32
     * the coeffRQT and reconQtYuv are allocated to the max CU size at every depth. The parts
     * which are reconstructed at each depth are valid. At the end, the transform depth table
     * is walked and the coeff and recon at the final split depths are collected */
    Entropy  rqtRoot;      /* residual quad-tree start context */
    Entropy  rqtTemp;      /* residual quad-tree temp context */
    Entropy  rqtTest;      /* residual quad-tree test context */
    coeff_t* coeffRQT[3];  /* coeff storage for entire CTU for each RQT layer */
    Yuv      reconQtYuv;   /* recon storage for entire CTU for each RQT layer (intra) */
    ShortYuv resiQtYuv;    /* residual storage for entire CTU for each RQT layer (inter) */
    
    /* per-depth temp buffers for inter prediction */
    ShortYuv tmpResiYuv;
    Yuv      tmpPredYuv;
    Yuv      bidirPredYuv[2];
};

struct MotionData
{
    MV       mv;
    MV       mvp;
    int      mvpIdx;
    int      ref;
    uint32_t cost;
    int      bits;
};

struct Mode
{
    CUData     cu;
    const Yuv* fencYuv;
    Yuv        predYuv;
    Yuv        reconYuv;
    Entropy    contexts;

    enum { MAX_INTER_PARTS = 2 };

    MotionData bestME[MAX_INTER_PARTS][2];
    MV         amvpCand[2][MAX_NUM_REF][AMVP_NUM_CANDS];

    uint64_t   rdCost;     // sum of partition (psy) RD costs          (sse(fenc, recon) + lambda2 * bits)
    uint64_t   sa8dCost;   // sum of partition sa8d distortion costs   (sa8d(fenc, pred) + lambda * bits)
    uint32_t   sa8dBits;   // signal bits used in sa8dCost calculation
    uint32_t   psyEnergy;  // sum of partition psycho-visual energy difference
    uint32_t   distortion; // sum of partition SSE distortion
    uint32_t   totalBits;  // sum of partition bits (mv + coeff)
    uint32_t   mvBits;     // Mv bits + Ref + block type (or intra mode)
    uint32_t   coeffBits;  // Texture bits (DCT Coeffs)

    void initCosts()
    {
        rdCost = 0;
        sa8dCost = 0;
        sa8dBits = 0;
        psyEnergy = 0;
        distortion = 0;
        totalBits = 0;
        mvBits = 0;
        coeffBits = 0;
    }

    void addSubCosts(const Mode& subMode)
    {
        rdCost += subMode.rdCost;
        sa8dCost += subMode.sa8dCost;
        sa8dBits += subMode.sa8dBits;
        psyEnergy += subMode.psyEnergy;
        distortion += subMode.distortion;
        totalBits += subMode.totalBits;
        mvBits += subMode.mvBits;
        coeffBits += subMode.coeffBits;
    }
};

#if DETAILED_CU_STATS
/* This structure is intended for performance debugging and we make no attempt
 * to handle dynamic range overflows. Care should be taken to avoid long encodes
 * if you care about the accuracy of these elapsed times and counters. This
 * profiling is orthoganal to PPA/VTune and can be enabled indepedently from
 * either of them */
struct CUStats
{
    int64_t  intraRDOElapsedTime;
    int64_t  interRDOElapsedTime;
    int64_t  intraAnalysisElapsedTime;    /* in RD > 4, includes RDO cost */
    int64_t  motionEstimationElapsedTime;
    int64_t  pmeTime;
    int64_t  pmeBlockTime;
    int64_t  pmodeTime;
    int64_t  pmodeBlockTime;
    int64_t  totalCTUTime;

    uint64_t countIntraRDO;
    uint64_t countInterRDO;
    uint64_t countIntraAnalysis;
    uint64_t countMotionEstimate;
    uint64_t countPMETasks;
    uint64_t countPMEMasters;
    uint64_t countPModeTasks;
    uint64_t countPModeMasters;
    uint64_t totalCTUs;

    CUStats() { clear(); }

    void clear()
    {
        memset(this, 0, sizeof(*this));
    }

    void accumulate(CUStats& other)
    {
        intraRDOElapsedTime += other.intraRDOElapsedTime;
        interRDOElapsedTime += other.interRDOElapsedTime;
        intraAnalysisElapsedTime += other.intraAnalysisElapsedTime;
        motionEstimationElapsedTime += other.motionEstimationElapsedTime;
        pmeTime += other.pmeTime;
        pmeBlockTime += other.pmeBlockTime;
        pmodeTime += other.pmodeTime;
        pmodeBlockTime += other.pmodeBlockTime;
        totalCTUTime += other.totalCTUTime;

        countIntraRDO += other.countIntraRDO;
        countInterRDO += other.countInterRDO;
        countIntraAnalysis += other.countIntraAnalysis;
        countMotionEstimate += other.countMotionEstimate;
        countPMETasks += other.countPMETasks;
        countPMEMasters += other.countPMEMasters;
        countPModeTasks += other.countPModeTasks;
        countPModeMasters += other.countPModeMasters;
        totalCTUs += other.totalCTUs;

        other.clear();
    }
}; 
#endif

inline int getTUBits(int idx, int numIdx)
{
    return idx + (idx < numIdx - 1);
}

class Search : public JobProvider, public Predict
{
public:

    static const pixel   zeroPixel[MAX_CU_SIZE];
    static const int16_t zeroShort[MAX_CU_SIZE];

    MotionEstimate  m_me;
    Quant           m_quant;
    RDCost          m_rdCost;
    const x265_param* m_param;
    Frame*          m_frame;
    const Slice*    m_slice;

    Entropy         m_entropyCoder;
    RQTData         m_rqt[NUM_FULL_DEPTH];

    uint8_t*        m_qtTempCbf[3];
    uint8_t*        m_qtTempTransformSkipFlag[3];

    bool            m_bFrameParallel;
    bool            m_bEnableRDOQ;
    uint32_t        m_numLayers;
    uint32_t        m_refLagPixels;

#if DETAILED_CU_STATS
    /* Accumulate CU statistics seperately for each frame encoder */
    CUStats         m_stats[X265_MAX_FRAME_THREADS];
#endif

    Search();
    ~Search();

    bool     initSearch(const x265_param& param, ScalingList& scalingList);
    void     setQP(const Slice& slice, int qp);

    // mark temp RD entropy contexts as uninitialized; useful for finding loads without stores
    void     invalidateContexts(int fromDepth);

    // full RD search of intra modes. if sharedModes is not NULL, it directly uses them
    void     checkIntra(Mode& intraMode, const CUGeom& cuGeom, PartSize partSize, uint8_t* sharedModes);

    // select best intra mode using only sa8d costs, cannot measure NxN intra
    void     checkIntraInInter(Mode& intraMode, const CUGeom& cuGeom);
    // encode luma mode selected by checkIntraInInter, then pick and encode a chroma mode
    void     encodeIntraInInter(Mode& intraMode, const CUGeom& cuGeom);

    // estimation inter prediction (non-skip)
    bool     predInterSearch(Mode& interMode, const CUGeom& cuGeom, bool bMergeOnly, bool bChroma);

    // encode residual and compute rd-cost for inter mode
    void     encodeResAndCalcRdInterCU(Mode& interMode, const CUGeom& cuGeom);
    void     encodeResAndCalcRdSkipCU(Mode& interMode);

    // encode residual without rd-cost
    void     residualTransformQuantInter(Mode& mode, const CUGeom& cuGeom, uint32_t absPartIdx, uint32_t tuDepth, const uint32_t depthRange[2]);
    void     residualTransformQuantIntra(Mode& mode, const CUGeom& cuGeom, uint32_t absPartIdx, uint32_t tuDepth, const uint32_t depthRange[2]);
    void     residualQTIntraChroma(Mode& mode, const CUGeom& cuGeom, uint32_t absPartIdx, uint32_t tuDepth);

    // pick be chroma mode from available using just sa8d costs
    void     getBestIntraModeChroma(Mode& intraMode, const CUGeom& cuGeom);

protected:

    /* motion estimation distribution */
    ThreadLocalData* m_tld;
    Mode*         m_curInterMode;
    const CUGeom* m_curGeom;
    int           m_curPart;
    uint32_t      m_listSelBits[3];
    int           m_totalNumME;
    volatile int  m_numAcquiredME;
    volatile int  m_numCompletedME;
    Event         m_meCompletionEvent;
    Lock          m_meLock;
    bool          m_bJobsQueued;
    void     singleMotionEstimation(Search& master, Mode& interMode, const CUGeom& cuGeom, int part, int list, int ref);

    void     saveResidualQTData(CUData& cu, ShortYuv& resiYuv, uint32_t absPartIdx, uint32_t tuDepth);

    // RDO search of luma intra modes; result is fully encoded luma. luma distortion is returned
    uint32_t estIntraPredQT(Mode &intraMode, const CUGeom& cuGeom, const uint32_t depthRange[2], uint8_t* sharedModes);

    // RDO select best chroma mode from luma; result is fully encode chroma. chroma distortion is returned
    uint32_t estIntraPredChromaQT(Mode &intraMode, const CUGeom& cuGeom);

    void     codeSubdivCbfQTChroma(const CUData& cu, uint32_t tuDepth, uint32_t absPartIdx);
    void     codeInterSubdivCbfQT(CUData& cu, uint32_t absPartIdx, const uint32_t tuDepth, const uint32_t depthRange[2]);
    void     codeCoeffQTChroma(const CUData& cu, uint32_t tuDepth, uint32_t absPartIdx, TextType ttype);

    struct Cost
    {
        uint64_t rdcost;
        uint32_t bits;
        uint32_t distortion;
        uint32_t energy;
        Cost() { rdcost = 0; bits = 0; distortion = 0; energy = 0; }
    };

    uint64_t estimateNullCbfCost(uint32_t &dist, uint32_t &psyEnergy, uint32_t tuDepth, TextType compId);
    void     estimateResidualQT(Mode& mode, const CUGeom& cuGeom, uint32_t absPartIdx, uint32_t depth, ShortYuv& resiYuv, Cost& costs, const uint32_t depthRange[2]);

    // generate prediction, generate residual and recon. if bAllowSplit, find optimal RQT splits
    void     codeIntraLumaQT(Mode& mode, const CUGeom& cuGeom, uint32_t tuDepth, uint32_t absPartIdx, bool bAllowSplit, Cost& costs, const uint32_t depthRange[2]);
    void     codeIntraLumaTSkip(Mode& mode, const CUGeom& cuGeom, uint32_t tuDepth, uint32_t absPartIdx, Cost& costs);
    void     extractIntraResultQT(CUData& cu, Yuv& reconYuv, uint32_t tuDepth, uint32_t absPartIdx);

    // generate chroma prediction, generate residual and recon
    uint32_t codeIntraChromaQt(Mode& mode, const CUGeom& cuGeom, uint32_t tuDepth, uint32_t absPartIdx, uint32_t& psyEnergy);
    uint32_t codeIntraChromaTSkip(Mode& mode, const CUGeom& cuGeom, uint32_t tuDepth, uint32_t tuDepthC, uint32_t absPartIdx, uint32_t& psyEnergy);
    void     extractIntraResultChromaQT(CUData& cu, Yuv& reconYuv, uint32_t absPartIdx, uint32_t tuDepth);

    // reshuffle CBF flags after coding a pair of 4:2:2 chroma blocks
    void     offsetSubTUCBFs(CUData& cu, TextType ttype, uint32_t tuDepth, uint32_t absPartIdx);

    struct MergeData
    {
        /* merge candidate data, cached between calls to mergeEstimation */
        MVField  mvFieldNeighbours[MRG_MAX_NUM_CANDS][2];
        uint8_t  interDirNeighbours[MRG_MAX_NUM_CANDS];
        uint32_t maxNumMergeCand;

        /* data updated for each partition */
        uint32_t absPartIdx;
        int      width;
        int      height;

        /* outputs */
        MVField  mvField[2];
        uint32_t interDir;
        uint32_t index;
        uint32_t bits;
    };

    /* inter/ME helper functions */
    void     checkBestMVP(MV* amvpCand, MV cMv, MV& mvPred, int& mvpIdx, uint32_t& outBits, uint32_t& outCost) const;
    void     setSearchRange(const CUData& cu, MV mvp, int merange, MV& mvmin, MV& mvmax) const;
    uint32_t mergeEstimation(CUData& cu, const CUGeom& cuGeom, int partIdx, MergeData& m);
    static void getBlkBits(PartSize cuMode, bool bPSlice, int partIdx, uint32_t lastMode, uint32_t blockBit[3]);

    /* intra helper functions */
    enum { MAX_RD_INTRA_MODES = 16 };
    static void updateCandList(uint32_t mode, uint64_t cost, int maxCandCount, uint32_t* candModeList, uint64_t* candCostList);

    // get most probable luma modes for CU part, and bit cost of all non mpm modes
    uint32_t getIntraRemModeBits(CUData & cu, uint32_t absPartIdx, uint32_t preds[3], uint64_t& mpms) const;

    void updateModeCost(Mode& m) const { m.rdCost = m_rdCost.m_psyRd ? m_rdCost.calcPsyRdCost(m.distortion, m.totalBits, m.psyEnergy) : m_rdCost.calcRdCost(m.distortion, m.totalBits); }
};
}

#endif // ifndef X265_SEARCH_H
