/*****************************************************************************
* Copyright (C) 2013 x265 project
*
* Authors: Deepthi Nandakumar <deepthi@multicorewareinc.com>
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

#define ANGULAR_MODE_ID 2
#define AMP_ID 3
#define INTER_MODES 4
#define INTRA_MODES 3

#include "common.h"
#include "predict.h"
#include "quant.h"
#include "shortyuv.h"
#include "threadpool.h"
#include "TLibCommon/TComYuv.h"
#include "TLibCommon/TComDataCU.h"

#include "entropy.h"
#include "search.h"

namespace x265 {
// private namespace

struct StatisticLog
{
    uint64_t cntInter[4];
    uint64_t cntIntra[4];
    uint64_t cuInterDistribution[4][INTER_MODES];
    uint64_t cuIntraDistribution[4][INTRA_MODES];
    uint64_t cntIntraNxN;
    uint64_t cntSkipCu[4];
    uint64_t cntTotalCu[4];
    uint64_t totalCu;

    /* These states store the count of inter,intra and skip ctus within quad tree structure of each CU */
    uint32_t qTreeInterCnt[4];
    uint32_t qTreeIntraCnt[4];
    uint32_t qTreeSkipCnt[4];

    StatisticLog()
    {
        memset(this, 0, sizeof(StatisticLog));
    }
};

class Encoder;
class Entropy;
struct ThreadLocalData;

class Analysis : public JobProvider, public Search
{
public:

    enum {
        PRED_2Nx2N,
        PRED_Nx2N,
        PRED_2NxN,
        PRED_MERGE,
        PRED_INTRA,
        MAX_PRED_TYPES
    };

    TComDataCU*  m_memPool;

    TComDataCU*  m_interCU_2Nx2N[NUM_CU_DEPTH];
    TComDataCU*  m_interCU_2NxN[NUM_CU_DEPTH];
    TComDataCU*  m_interCU_Nx2N[NUM_CU_DEPTH];
    TComDataCU*  m_intraInInterCU[NUM_CU_DEPTH];
    TComDataCU*  m_mergeCU[NUM_CU_DEPTH];
    TComDataCU*  m_bestMergeCU[NUM_CU_DEPTH];
    TComDataCU*  m_bestCU[NUM_CU_DEPTH]; // Best CUs at each depth
    TComDataCU*  m_tempCU[NUM_CU_DEPTH]; // Temporary CUs at each depth

    TComYuv**    m_bestPredYuv;          // Best Prediction Yuv for each depth
    ShortYuv**   m_bestResiYuv;          // Best Residual Yuv for each depth
    TComYuv**    m_bestRecoYuv;          // Best Reconstruction Yuv for each depth

    TComYuv**    m_tmpPredYuv;           // Temporary Prediction Yuv for each depth
    ShortYuv**   m_tmpResiYuv;           // Temporary Residual Yuv for each depth
    TComYuv**    m_tmpRecoYuv;           // Temporary Reconstruction Yuv for each depth
    TComYuv**    m_modePredYuv[MAX_PRED_TYPES]; // Prediction buffers for inter, intra, rect(2) and merge
    TComYuv**    m_bestMergeRecoYuv;
    TComYuv**    m_bestIntraRecoYuv;
    TComYuv**    m_origYuv;             // Original Yuv at each depth

    bool         m_bEncodeDQP;

    StatisticLog  m_sliceTypeLog[3];
    StatisticLog* m_log;

    Analysis();
    bool create(uint32_t totalDepth, uint32_t maxWidth, ThreadLocalData* tld);
    void destroy();
    void compressCU(TComDataCU* cu);

protected:

    /* Job provider details */
    CU*           m_curCUData;
    int           m_curDepth;
    ThreadLocalData* m_tld;
    bool          m_bJobsQueued;
    bool findJob(int threadId);

    /* mode analysis distribution */
    Entropy       m_intraContexts;
    int           m_totalNumJobs;
    volatile int  m_numAcquiredJobs;
    volatile int  m_numCompletedJobs;
    Event         m_modeCompletionEvent;
    void parallelAnalysisJob(int threadId, int jobId);

    /* motion estimation distribution */
    TComDataCU*   m_curMECu;
    int           m_curPart;
    MotionData    m_bestME[2];
    uint32_t      m_listSelBits[3];
    int           m_totalNumME;
    volatile int  m_numAcquiredME;
    volatile int  m_numCompletedME;
    Event         m_meCompletionEvent;
    Lock          m_outputLock;
    void parallelME(int threadId, int meId);

    /* Warning: The interface for these functions will undergo significant changes as a major refactor is under progress */
    void compressIntraCU(TComDataCU*& outBestCU, TComDataCU*& outTempCU, uint32_t depth, CU *cu);
    void checkIntra(TComDataCU*& outTempCU, PartSize partSize, CU *cu, uint8_t* sharedModes);
    void compressSharedIntraCTU(TComDataCU*& outBestCU, TComDataCU*& outTempCU, uint32_t depth, CU *cu, uint8_t* sharedDepth,
                                char* sharedPartSizes, uint8_t* sharedModes, uint32_t &zOrder);
    void compressInterCU_rd0_4(TComDataCU*& outBestCU, TComDataCU*& outTempCU, TComDataCU* parentCU, uint32_t depth, CU *cu,
                               int bInsidePicture, uint32_t partitionIndex, uint32_t minDepth);
    void compressInterCU_rd5_6(TComDataCU*& outBestCU, TComDataCU*& outTempCU, uint32_t depth, CU *cu);
    void checkMerge2Nx2N_rd0_4(CU* cu, uint32_t depth);
    void checkMerge2Nx2N_rd5_6(TComDataCU*& outBestCU, TComDataCU*& outTempCU, CU* cu, bool *earlyDetectionSkipMode,
                               TComYuv*& outBestPredYuv, TComYuv*& yuvReconBest);
    void checkInter_rd0_4(TComDataCU* outTempCU, CU* cu, TComYuv* outPredYUV, PartSize partSize);
    void parallelInterSearch(TComDataCU* cu, CU* cuData, TComYuv* predYuv, bool bChroma);
    void checkInter_rd5_6(TComDataCU*& outBestCU, TComDataCU*& outTempCU, CU* cu, PartSize partSize, bool bMergeOnly);
    void checkIntraInInter_rd0_4(TComDataCU* cu, CU* cuData);
    void checkIntraInInter_rd5_6(TComDataCU*& outBestCU, TComDataCU*& outTempCU, CU* cu, PartSize partSize);

    void checkBestMode(TComDataCU*& outBestCU, TComDataCU*& outTempCU, uint32_t depth);
    void encodeIntraInInter(TComDataCU* cu, CU* cuData, TComYuv* fencYuv, TComYuv* predYuv, ShortYuv* outResiYuv, TComYuv* outReconYuv, Entropy&);
    void encodeResidue(TComDataCU* ctu, CU* cuData, uint32_t absPartIdx, uint32_t depth);
    void checkDQP(TComDataCU* cu);
    void deriveTestModeAMP(TComDataCU* bestCU, PartSize parentSize, bool &bTestAMP_Hor, bool &bTestAMP_Ver,
                           bool &bTestMergeAMP_Hor, bool &bTestMergeAMP_Ver);
    void fillOrigYUVBuffer(TComDataCU* outCU, TComYuv* origYuv);
};

struct ThreadLocalData
{
    Analysis analysis;

    ~ThreadLocalData() { analysis.destroy(); }
};

}

#endif // ifndef X265_ANALYSIS_H
