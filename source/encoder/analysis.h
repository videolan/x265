/*****************************************************************************
* Copyright (C) 2013 x265 project
*
* Authors: Deepthi Nandakumar <deepthi@multicorewareinc.com>
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
* For more information, contact us at license @ x265.com.
*****************************************************************************/

#ifndef X265_ANALYSIS_H
#define X265_ANALYSIS_H

#include "common.h"
#include "predict.h"
#include "quant.h"
#include "yuv.h"
#include "shortyuv.h"
#include "cudata.h"

#include "entropy.h"
#include "search.h"

namespace x265 {
// private namespace

class Entropy;

class Analysis : public Search
{
public:

    enum {
        PRED_MERGE,
        PRED_SKIP,
        PRED_INTRA,
        PRED_2Nx2N,
        PRED_BIDIR,
        PRED_Nx2N,
        PRED_2NxN,
        PRED_SPLIT,
        PRED_2NxnU,
        PRED_2NxnD,
        PRED_nLx2N,
        PRED_nRx2N,
        PRED_INTRA_NxN, /* 4x4 intra PU blocks for 8x8 CU */
        PRED_LOSSLESS,  /* lossless encode of best mode */
        MAX_PRED_TYPES
    };

    struct ModeDepth
    {
        Mode           pred[MAX_PRED_TYPES];
        Mode*          bestMode;
        Yuv            fencYuv;
        CUDataMemPool  cuMemPool;
    };

    ModeDepth m_modeDepth[NUM_CU_DEPTH];
    bool      m_bTryLossless;

    /* Analysis data for load/save modes, keeps getting incremented as CTU analysis proceeds and data is consumed or read */
    analysis_intra_data* m_reuseIntraDataCTU;
    analysis_inter_data* m_reuseInterDataCTU;
    Analysis();
    bool create(ThreadLocalData* tld);
    void destroy();
    Mode& compressCTU(CUData& ctu, Frame& frame, const CUGeom& cuGeom, const Entropy& initialContext);

protected:

    /* mode analysis distribution */
    int           m_totalNumJobs;
    volatile int  m_numAcquiredJobs;
    volatile int  m_numCompletedJobs;
    Event         m_modeCompletionEvent;
    bool findJob(int threadId);
    void parallelModeAnalysis(int threadId, int jobId);
    void parallelME(int threadId, int meId);

    /* full analysis for an I-slice CU */
    void compressIntraCU(const CUData& parentCTU, const CUGeom& cuGeom, analysis_intra_data* sdata, uint32_t &zOrder);

    /* full analysis for a P or B slice CU */
    void compressInterCU_dist(const CUData& parentCTU, const CUGeom& cuGeom);
    void compressInterCU_rd0_4(const CUData& parentCTU, const CUGeom& cuGeom);
    void compressInterCU_rd5_6(const CUData& parentCTU, const CUGeom& cuGeom);

    /* measure merge and skip */
    void checkMerge2Nx2N_rd0_4(Mode& skip, Mode& merge, const CUGeom& cuGeom);
    void checkMerge2Nx2N_rd5_6(Mode& skip, Mode& merge, const CUGeom& cuGeom);

    /* measure inter options */
    void checkInter_rd0_4(Mode& interMode, const CUGeom& cuGeom, PartSize partSize);
    void checkInter_rd5_6(Mode& interMode, const CUGeom& cuGeom, PartSize partSize, bool bMergeOnly);

    void checkBidir2Nx2N(Mode& inter2Nx2N, Mode& bidir2Nx2N, const CUGeom& cuGeom);

    /* encode current bestMode losslessly, pick best RD cost */
    void tryLossless(const CUGeom& cuGeom);

    /* add the RD cost of coding a split flag (0 or 1) to the given mode */
    void addSplitFlagCost(Mode& mode, uint32_t depth);

    /* update CBF flags and QP values to be internally consistent */
    void checkDQP(CUData& cu, const CUGeom& cuGeom);

    /* work-avoidance heuristics for RD levels < 5 */
    uint32_t topSkipMinDepth(const CUData& parentCTU, const CUGeom& cuGeom);
    bool recursionDepthCheck(const CUData& parentCTU, const CUGeom& cuGeom, const Mode& bestMode);

    /* generate residual and recon pixels for an entire CTU recursively (RD0) */
    void encodeResidue(const CUData& parentCTU, const CUGeom& cuGeom);

    /* check whether current mode is the new best */
    inline void checkBestMode(Mode& mode, uint32_t depth)
    {
        ModeDepth& md = m_modeDepth[depth];
        if (md.bestMode)
        {
            if (mode.rdCost < md.bestMode->rdCost)
                md.bestMode = &mode;
        }
        else
            md.bestMode = &mode;
    }
};

struct ThreadLocalData
{
    Analysis analysis;

    void destroy() { analysis.destroy(); }
};

}

#endif // ifndef X265_ANALYSIS_H
