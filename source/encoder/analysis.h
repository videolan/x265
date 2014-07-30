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
#include "TLibCommon/TComYuv.h"
#include "predict.h"
#include "TLibCommon/TComTrQuant.h"
#include "TLibCommon/TComDataCU.h"
#include "shortyuv.h"

#include "entropy.h"
#include "TLibEncoder/TEncSearch.h"

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

namespace x265 {
// private namespace

class Encoder;
class Entropy;

class Analysis : public TEncSearch
{
public:

    static const int MAX_PRED_TYPES = 6;

    TComDataCU*  m_memPool;

    TComDataCU*  m_interCU_2Nx2N[MAX_CU_DEPTH];
    TComDataCU*  m_interCU_2NxN[MAX_CU_DEPTH];
    TComDataCU*  m_interCU_Nx2N[MAX_CU_DEPTH];
    TComDataCU*  m_intraInInterCU[MAX_CU_DEPTH];
    TComDataCU*  m_mergeCU[MAX_CU_DEPTH];
    TComDataCU*  m_bestMergeCU[MAX_CU_DEPTH];
    TComDataCU*  m_bestCU[MAX_CU_DEPTH]; // Best CUs at each depth
    TComDataCU*  m_tempCU[MAX_CU_DEPTH]; // Temporary CUs at each depth

    TComYuv**    m_bestPredYuv;          // Best Prediction Yuv for each depth
    ShortYuv**   m_bestResiYuv;          // Best Residual Yuv for each depth
    TComYuv**    m_bestRecoYuv;          // Best Reconstruction Yuv for each depth

    TComYuv**    m_tmpPredYuv;           // Temporary Prediction Yuv for each depth
    ShortYuv**   m_tmpResiYuv;           // Temporary Residual Yuv for each depth
    TComYuv**    m_tmpRecoYuv;           // Temporary Reconstruction Yuv for each depth
    TComYuv**    m_modePredYuv[MAX_PRED_TYPES]; // Prediction buffers for inter, intra, rect(2) and merge
    TComYuv**    m_bestMergeRecoYuv;
    TComYuv**    m_origYuv;             // Original Yuv at each depth

    bool         m_bEncodeDQP;

    StatisticLog  m_sliceTypeLog[3];
    StatisticLog* m_log;

    Analysis();

    bool create(uint8_t totalDepth, uint32_t maxWidth);
    void destroy();
    void compressCU(TComDataCU* cu);
    void encodeCU(TComDataCU* cu);

protected:

    void compressIntraCU(TComDataCU*& outBestCU, TComDataCU*& outTempCU, uint8_t depth, bool bInsidePicture);
    void checkIntra(TComDataCU*& outBestCU, TComDataCU*& outTempCU, PartSize partSize);

    void compressInterCU_rd0_4(TComDataCU*& outBestCU, TComDataCU*& outTempCU, TComDataCU* cu, uint8_t depth,
                               bool bInsidePicture, uint32_t partitionIndex, uint8_t minDepth);
    void compressInterCU_rd5_6(TComDataCU*& outBestCU, TComDataCU*& outTempCU, uint8_t depth, bool bInsidePicture,
        PartSize parentSize = SIZE_NONE);
    void checkMerge2Nx2N_rd0_4(TComDataCU*& outBestCU, TComDataCU*& outTempCU, TComYuv*& bestPredYuv, TComYuv*& tmpPredYuv);
    void checkMerge2Nx2N_rd5_6(TComDataCU*& outBestCU, TComDataCU*& outTempCU, bool *earlyDetectionSkipMode,
                               TComYuv*& outBestPredYuv, TComYuv*& rpcYuvReconBest);

    void checkInter_rd0_4(TComDataCU* outTempCU, TComYuv* outPredYUV, PartSize partSize, bool bUseMRG = false);
    void checkInter_rd5_6(TComDataCU*& outBestCU, TComDataCU*& outTempCU, PartSize partSize, bool bUseMRG = false);

    void checkIntraInInter_rd0_4(TComDataCU* cu, PartSize partSize);
    void checkIntraInInter_rd5_6(TComDataCU*& outBestCU, TComDataCU*& outTempCU, PartSize partSize);

    void encodeCU(TComDataCU* cu, uint32_t absPartIdx, uint32_t depth, bool bInsidePicture);
    void checkBestMode(TComDataCU*& outBestCU, TComDataCU*& outTempCU, uint32_t depth);
    void encodeIntraInInter(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, ShortYuv* outResiYuv, TComYuv* outReconYuv);
    void encodeResidue(TComDataCU* lcu, TComDataCU* cu, uint32_t absPartIdx, uint8_t depth);
    void checkDQP(TComDataCU* cu);
    void copyYuv2Pic(Frame* outPic, uint32_t cuAddr, uint32_t absPartIdx, uint32_t depth);
    void copyYuv2Tmp(uint32_t uhPartUnitIdx, uint32_t depth);
    void deriveTestModeAMP(TComDataCU* bestCU, PartSize parentSize, bool &bTestAMP_Hor, bool &bTestAMP_Ver,
                           bool &bTestMergeAMP_Hor, bool &bTestMergeAMP_Ver);
    void fillOrigYUVBuffer(TComDataCU* outCU, TComYuv* origYuv);
    void finishCU(TComDataCU* cu, uint32_t absPartIdx, uint32_t depth);
};
}

#endif // ifndef X265_ANALYSIS_H
