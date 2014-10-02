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

#include "TLibCommon/TComYuv.h"
#include "TLibCommon/TComMotionInfo.h"
#include "TLibCommon/TComPattern.h"

#include "rdcost.h"
#include "entropy.h"
#include "motion.h"

#define MVP_IDX_BITS 1
#define NUM_LAYERS 4

namespace x265 {
// private namespace

class Entropy;

inline int getTUBits(int idx, int numIdx)
{
    return idx + (idx < numIdx - 1);
}

class Search : public Predict
{
public:

    static const pixel zeroPel[MAX_CU_SIZE];

    MotionEstimate  m_me;
    Quant           m_quant;
    RDCost          m_rdCost;
    x265_param*     m_param;

    Entropy         m_entropyCoder;
    Entropy       (*m_rdEntropyCoders)[CI_NUM];

    TComYuv         m_predTempYuv;
    TComYuv         m_bidirPredYuv[2];
    ShortYuv*       m_qtTempShortYuv;
    coeff_t*        m_qtTempCoeff[3][NUM_LAYERS];
    uint8_t*        m_qtTempCbf[3];
    uint8_t*        m_qtTempTransformSkipFlag[3];

    bool            m_bFrameParallel;
    bool            m_bEnableRDOQ;
    int             m_numLayers;
    int             m_refLagPixels;

    Search();
    ~Search();

    bool     initSearch(x265_param *param, ScalingList& scalingList);
    void     setQP(Slice* slice, int qp);

    void     estIntraPredQT(TComDataCU* cu, CU* cuData, TComYuv* fencYuv, TComYuv* predYuv, ShortYuv* resiYuv, TComYuv* reconYuv, uint32_t depthRange[2]);
    void     sharedEstIntraPredQT(TComDataCU* cu, CU* cuData, TComYuv* fencYuv, TComYuv* predYuv, ShortYuv* resiYuv, TComYuv* reconYuv, uint32_t depthRange[2], uint8_t* sharedModes);
    void     estIntraPredChromaQT(TComDataCU* cu, CU* cuData, TComYuv* fencYuv, TComYuv* predYuv, ShortYuv* resiYuv, TComYuv* reconYuv);

    // estimation inter prediction (non-skip)
    bool     predInterSearch(TComDataCU* cu, CU* cuData, TComYuv* predYuv, bool bMergeOnly, bool bChroma);

    // encode residual and compute rd-cost for inter mode
    void     encodeResAndCalcRdInterCU(TComDataCU* cu, CU* cuData, TComYuv* fencYuv, TComYuv* predYuv, ShortYuv* resiYuv, ShortYuv* bestResiYuv, TComYuv* reconYuv);
    void     encodeResAndCalcRdSkipCU(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, TComYuv* reconYuv);

    void     generateCoeffRecon(TComDataCU* cu, CU* cuData, TComYuv* fencYuv, TComYuv* predYuv, ShortYuv* resiYuv, TComYuv* reconYuv);
    void     residualTransformQuantInter(TComDataCU* cu, CU* cuData, uint32_t absPartIdx, TComYuv* fencYuv, ShortYuv* resiYuv, uint32_t depth, uint32_t depthRange[2]);

    uint32_t getIntraModeBits(TComDataCU* cu, uint32_t mode, uint32_t partOffset, uint32_t depth);
    uint32_t getIntraRemModeBits(TComDataCU * cu, uint32_t partOffset, uint32_t depth, uint32_t preds[3], uint64_t& mpms);

protected:

    void     xSetResidualQTData(TComDataCU* cu, uint32_t absPartIdx, ShortYuv* resiYuv, uint32_t depth, bool bSpatial);
    void     xSetIntraResultQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, TComYuv* reconYuv);

    void     xEncSubdivCbfQTChroma(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx,  uint32_t absPartIdxStep, uint32_t width, uint32_t height);
    void     xEncCoeffQTChroma(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, TextType ttype);
    void     xEncIntraHeaderLuma(TComDataCU* cu, CU* cuData, uint32_t trDepth, uint32_t absPartIdx);
    void     xEncIntraHeaderChroma(TComDataCU* cu, CU* cuData, uint32_t absPartIdx);

    uint32_t xGetIntraBitsQTChroma(TComDataCU* cu, CU* cuData, uint32_t trDepth, uint32_t absPartIdx, uint32_t absPartIdxStep);
    uint32_t xGetIntraBitsLuma(TComDataCU* cu, CU* cuData, uint32_t trDepth, uint32_t absPartIdx, uint32_t log2TrSize, coeff_t* coeff, uint32_t depthRange[2]);
    uint32_t xGetIntraBitsChroma(TComDataCU* cu, uint32_t absPartIdx, uint32_t log2TrSizeC, uint32_t chromaId, coeff_t* coeff);
    uint32_t xIntraCodingLumaBlk(TComDataCU* cu, CU* cuData, uint32_t absPartIdx, uint32_t log2TrSize, TComYuv* fencYuv, TComYuv* predYuv, ShortYuv* resiYuv,
                                 int16_t* reconQt, uint32_t reconQtStride, coeff_t* coeff, uint32_t& cbf);

    uint32_t xEstimateResidualQT(TComDataCU* cu, CU* cuData, uint32_t absPartIdx, TComYuv* fencYuv, TComYuv* predYuv, ShortYuv* resiYuv, uint32_t depth,
                                 uint64_t &rdCost, uint32_t &outBits, uint32_t *zeroDist, uint32_t tuDepthRange[2]);

    uint32_t xRecurIntraCodingQT(TComDataCU* cu, CU* cuData, uint32_t trDepth, uint32_t absPartIdx, TComYuv* fencYuv, TComYuv* predYuv,
                                 ShortYuv* resiYuv, bool bAllowRQTSplit, uint64_t& dRDCost, uint32_t& puBits, uint32_t& psyEnergy, uint32_t depthRange[2]);

    uint32_t xRecurIntraChromaCodingQT(TComDataCU* cu, CU* cuData, uint32_t trDepth, uint32_t absPartIdx, TComYuv* fencYuv, TComYuv* predYuv, ShortYuv* resiYuv,
                                       uint32_t& psyEnergy);

    uint32_t xIntraCodingChromaBlk(TComDataCU* cu, CU* cuData, uint32_t absPartIdx, TComYuv* fencYuv, TComYuv* predYuv, ShortYuv* resiYuv,
                                   int16_t* reconQt, uint32_t reconQtStride, coeff_t* coeff, uint32_t& cbf, uint32_t chromaId, uint32_t log2TrSizeC);

    void     residualTransformQuantIntra(TComDataCU* cu, CU* cuData, uint32_t trDepth, uint32_t absPartIdx, TComYuv* fencYuv,
                                         TComYuv* predYuv, ShortYuv* resiYuv, TComYuv* reconYuv, uint32_t depthRange[2]);

    void     residualQTIntraChroma(TComDataCU* cu, CU* cuData, uint32_t trDepth, uint32_t absPartIdx, TComYuv* fencYuv,
                                   TComYuv* predYuv, ShortYuv* resiYuv, TComYuv* reconYuv);

    void     xEncodeResidualQT(TComDataCU* cu, uint32_t absPartIdx, uint32_t depth, bool bSubdivAndCbf, TextType ttype, uint32_t depthRange[2]);
    void     xSetIntraResultChromaQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, TComYuv* reconYuv);

    void     xLoadIntraResultQT(TComDataCU* cu, CU* cuData, uint32_t absPartIdx, uint32_t log2TrSize, int16_t* reconQt, uint32_t reconQtStride);
    void     xLoadIntraResultChromaQT(TComDataCU* cu, CU* cuData, uint32_t absPartIdx, uint32_t log2TrSizeC, uint32_t chromaId, int16_t* reconQt, uint32_t reconQtStride);

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

    struct MotionData
    {
        MV  mv;
        MV  mvp;
        int mvpIdx;
        int ref;
        uint32_t cost;
        int bits;
    };

    /* inter/ME helper functions */
    void     checkBestMVP(MV* amvpCand, MV cMv, MV& mvPred, int& mvpIdx, uint32_t& outBits, uint32_t& outCost) const;
    void     getBlkBits(PartSize cuMode, bool bPSlice, int partIdx, uint32_t lastMode, uint32_t blockBit[3]) const;
    void     setSearchRange(TComDataCU* cu, MV mvp, int merange, MV& mvmin, MV& mvmax) const;
    uint32_t getInterSymbolBits(TComDataCU* cu, uint32_t depthRange[2]);
    uint32_t mergeEstimation(TComDataCU* cu, CU* cuData, int partIdx, MergeData& m);

    /* intra helper functions */
    enum { MAX_RD_INTRA_MODES = 16 };
    void     updateCandList(uint32_t mode, uint64_t cost, int maxCandCount, uint32_t* candModeList, uint64_t* candCostList);
    void     getBestIntraModeChroma(TComDataCU* cu, CU* cuData, TComYuv* fencYuv, TComYuv* predYuv);
};
}

#endif // ifndef X265_SEARCH_H
