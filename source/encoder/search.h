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
#include "yuv.h"
#include "threadpool.h"

#include "TLibCommon/TComMotionInfo.h"

#include "rdcost.h"
#include "entropy.h"
#include "motion.h"

#define MVP_IDX_BITS 1
#define NUM_LAYERS 4

namespace x265 {
// private namespace

class Entropy;
struct ThreadLocalData;

/* All the CABAC contexts that Analysis needs to keep track of at each depth */
struct RDContexts
{
    Entropy  cur;     /* input context for current CU */
    Entropy  rqtTemp; /* residual quad-tree temp context */
    Entropy  rqtRoot; /* residual quad-tree start context */
    Entropy  rqtTest; /* residual quad-tree test context */
    ShortYuv tempResi;
};

inline int getTUBits(int idx, int numIdx)
{
    return idx + (idx < numIdx - 1);
}

class Search : public JobProvider, public Predict
{
public:

    static const pixel zeroPel[MAX_CU_SIZE];

    MotionEstimate  m_me;
    Quant           m_quant;
    RDCost          m_rdCost;
    x265_param*     m_param;
    Frame*          m_frame;
    const Slice*    m_slice;

    Entropy         m_entropyCoder;
    RDContexts      m_rdContexts[NUM_FULL_DEPTH];

    Yuv             m_predTempYuv;
    Yuv             m_bidirPredYuv[2];
    ShortYuv*       m_qtTempShortYuv;
    coeff_t*        m_qtTempCoeff[3][NUM_LAYERS];
    uint8_t*        m_qtTempCbf[3];
    uint8_t*        m_qtTempTransformSkipFlag[3];

    bool            m_bFrameParallel;
    bool            m_bEnableRDOQ;
    int             m_numLayers;
    int             m_refLagPixels;

    struct Mode
    {
        TComDataCU cu;
        const Yuv* fencYuv;
        Yuv        predYuv;
        Yuv        reconYuv;
        ShortYuv   resiYuv;
        Entropy    contexts;

        uint64_t   rdCost;     // sum of partition (psy) RD costs          (sse(fenc, recon) + lambda2 * bits)
        uint64_t   sa8dCost;   // sum of partition sa8d distortion costs   (sa8d(fenc, pred) + lambda * bits)
        uint32_t   psyEnergy;  // sum of partition psycho-visual energy difference
        uint32_t   distortion; // sum of partition SSE distortion
        uint32_t   totalBits;  // sum of partition bits (mv + coeff)
        uint32_t   mvBits;     // Mv bits + Ref + block type (or intra mode)
        uint32_t   coeffBits;  // Texture bits (DCT Coeffs)

        void initCosts()
        {
            rdCost = 0;
            sa8dCost = 0;
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
            psyEnergy += subMode.psyEnergy;
            distortion += subMode.distortion;
            totalBits += subMode.totalBits;
            mvBits += subMode.mvBits;
            coeffBits += subMode.coeffBits;
        }
    };

    struct MotionData
    {
        MV  mv;
        MV  mvp;
        int mvpIdx;
        int ref;
        uint32_t cost;
        int bits;
    };

    Search();
    ~Search();

    bool     initSearch(x265_param *param, ScalingList& scalingList);
    void     setQP(const Slice& slice, int qp);
    inline void updateModeCost(Mode& mode) const;

    // mark temp RD entropy contexts as uninitialized; useful for finding loads without stores
    void     invalidateContexts(int fromDepth);

    // RDO search of luma intra modes; result is fully encoded luma. luma distortion is returned
    uint32_t estIntraPredQT(Mode &intraMode, const CU& cuData, uint32_t depthRange[2], uint8_t* sharedModes);

    // RDO select best chroma mode from luma; result is fully encode chroma. chroma distortion is returned
    uint32_t estIntraPredChromaQT(Mode &intraMode, const CU& cuData);

    // estimation inter prediction (non-skip)
    bool     predInterSearch(Mode& interMode, const CU& cuData, bool bMergeOnly, bool bChroma);
    void     parallelInterSearch(Mode& interMode, const CU& cuData, bool bChroma);

    // encode residual and compute rd-cost for inter mode
    void     encodeResAndCalcRdInterCU(Mode& interMode, const CU& cuData);
    void     encodeResAndCalcRdSkipCU(Mode& interMode);

    void     generateCoeffRecon(Mode& mode, const CU& cuData);
    void     residualTransformQuantInter(Mode& mode, const CU& cuData, uint32_t absPartIdx, uint32_t depth, uint32_t depthRange[2]);

    void     fillOrigYUVBuffer(TComDataCU& cu, const Yuv& origYuv);

    uint32_t getIntraModeBits(TComDataCU& cu, uint32_t mode, uint32_t partOffset, uint32_t depth);
    uint32_t getIntraRemModeBits(TComDataCU & cu, uint32_t partOffset, uint32_t depth, uint32_t preds[3], uint64_t& mpms);

protected:

    /* motion estimation distribution */
    ThreadLocalData* m_tld;
    TComDataCU*   m_curMECu;
    const CU*     m_curCUData;
    int           m_curPart;
    MotionData    m_bestME[2];
    uint32_t      m_listSelBits[3];
    int           m_totalNumME;
    volatile int  m_numAcquiredME;
    volatile int  m_numCompletedME;
    Event         m_meCompletionEvent;
    Lock          m_outputLock;
    bool          m_bJobsQueued;
    void     singleMotionEstimation(TComDataCU* cu, const CU& cuData, int part, int list, int ref);

    void     xSetResidualQTData(TComDataCU* cu, uint32_t absPartIdx, ShortYuv* resiYuv, uint32_t depth, bool bSpatial);
    void     xSetIntraResultQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, Yuv* reconYuv);
    void     xSetIntraResultChromaQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, Yuv* reconYuv);

    void     xEncSubdivCbfQTChroma(const TComDataCU& cu, uint32_t trDepth, uint32_t absPartIdx,  uint32_t absPartIdxStep, uint32_t width, uint32_t height);
    void     xEncCoeffQTChroma(const TComDataCU& cu, uint32_t trDepth, uint32_t absPartIdx, TextType ttype);
    void     xEncIntraHeaderLuma(const TComDataCU& cu, const CU& cuData, uint32_t trDepth, uint32_t absPartIdx);
    void     xEncIntraHeaderChroma(const TComDataCU& cu, const CU& cuData, uint32_t absPartIdx);

    uint32_t xGetIntraBitsQTChroma(const TComDataCU& cu, const CU& cuData, uint32_t trDepth, uint32_t absPartIdx, uint32_t absPartIdxStep);
    uint32_t xGetIntraBitsLuma(const TComDataCU& cu, const CU& cuData, uint32_t trDepth, uint32_t absPartIdx, uint32_t log2TrSize, const coeff_t* coeff, uint32_t depthRange[2]);
    uint32_t xGetIntraBitsChroma(const TComDataCU& cu, uint32_t absPartIdx, uint32_t log2TrSizeC, uint32_t chromaId, const coeff_t* coeff);

    uint32_t xIntraCodingLumaBlk(Mode& mode, const CU& cuData, uint32_t absPartIdx, uint32_t log2TrSize, int16_t* reconQt, uint32_t reconQtStride, coeff_t* coeff, uint32_t& cbf);

    uint32_t xEstimateResidualQT(Mode& mode, const CU& cuData, uint32_t absPartIdx, ShortYuv* resiYuv, uint32_t depth,
                                 uint64_t &rdCost, uint32_t &outBits, uint32_t *zeroDist, uint32_t tuDepthRange[2]);

    uint32_t xRecurIntraCodingQT(Mode& mode, const CU& cuData, uint32_t trDepth, uint32_t absPartIdx, bool bAllowRQTSplit,
                                 uint64_t& rdCost, uint32_t& puBits, uint32_t& psyEnergy, uint32_t depthRange[2]);

    uint32_t xRecurIntraChromaCodingQT(Mode& mode, const CU& cuData, uint32_t trDepth, uint32_t absPartIdx, uint32_t& psyEnergy);

    uint32_t xIntraCodingChromaBlk(Mode& mode, const CU& cuData, uint32_t absPartIdx, uint32_t chromaId, uint32_t log2TrSizeC,
                                   int16_t* reconQt, uint32_t reconQtStride, coeff_t* coeff, uint32_t& cbf);

    void     residualTransformQuantIntra(Mode& mode, const CU& cuData, uint32_t trDepth, uint32_t absPartIdx, uint32_t depthRange[2]);
    void     residualQTIntraChroma(Mode& mode, const CU& cuData, uint32_t trDepth, uint32_t absPartIdx);

    void     xEncodeResidualQT(TComDataCU* cu, uint32_t absPartIdx, uint32_t depth, bool bSubdivAndCbf, TextType ttype, uint32_t depthRange[2]);

    void     xLoadIntraResultQT(TComDataCU* cu, const CU& cuData, uint32_t absPartIdx, uint32_t log2TrSize, int16_t* reconQt, uint32_t reconQtStride);
    void     xLoadIntraResultChromaQT(TComDataCU* cu, const CU& cuData, uint32_t absPartIdx, uint32_t log2TrSizeC, uint32_t chromaId, int16_t* reconQt, uint32_t reconQtStride);

    void     offsetSubTUCBFs(TComDataCU* cu, TextType ttype, uint32_t trDepth, uint32_t absPartIdx);

    struct MergeData
    {
        /* merge candidate data, cached between calls to mergeEstimation */
        TComMvField mvFieldNeighbours[MRG_MAX_NUM_CANDS][2];
        uint8_t     interDirNeighbours[MRG_MAX_NUM_CANDS];
        uint32_t    maxNumMergeCand;

        /* data updated for each partition */
        uint32_t    absPartIdx;
        int         width;
        int         height;

        /* outputs */
        TComMvField mvField[2];
        uint32_t    interDir;
        uint32_t    index;
        uint32_t    bits;
    };

    /* inter/ME helper functions */
    void     checkBestMVP(MV* amvpCand, MV cMv, MV& mvPred, int& mvpIdx, uint32_t& outBits, uint32_t& outCost) const;
    void     getBlkBits(PartSize cuMode, bool bPSlice, int partIdx, uint32_t lastMode, uint32_t blockBit[3]) const;
    void     setSearchRange(const TComDataCU& cu, MV mvp, int merange, MV& mvmin, MV& mvmax) const;
    uint32_t getInterSymbolBits(Mode& mode, uint32_t depthRange[2]);
    uint32_t mergeEstimation(TComDataCU* cu, const CU& cuData, int partIdx, MergeData& m);

    /* intra helper functions */
    enum { MAX_RD_INTRA_MODES = 16 };
    void     updateCandList(uint32_t mode, uint64_t cost, int maxCandCount, uint32_t* candModeList, uint64_t* candCostList);
    void     getBestIntraModeChroma(Mode& intraMode, const CU& cuData);
};
}

#endif // ifndef X265_SEARCH_H
