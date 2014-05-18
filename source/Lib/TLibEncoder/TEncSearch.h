/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2013, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/** \file     TEncSearch.h
    \brief    encoder search class (header)
*/

#ifndef X265_TENCSEARCH_H
#define X265_TENCSEARCH_H

// Include files
#include "TLibCommon/TComYuv.h"
#include "TLibCommon/TComMotionInfo.h"
#include "TLibCommon/TComPattern.h"
#include "TLibCommon/TComPrediction.h"
#include "TLibCommon/TComTrQuant.h"
#include "TLibCommon/TComPic.h"
#include "TEncEntropy.h"
#include "TEncSbac.h"
#include "primitives.h"
#include "bitcost.h"
#include "motion.h"

#define MVP_IDX_BITS 1

//! \ingroup TLibEncoder
//! \{

namespace x265 {
// private namespace
class TEncCu;
class Encoder;

struct MotionData
{
    MV  mv;
    MV  mvp;
    int mvpIdx;
    int ref;
    uint32_t cost;
    int bits;
};

struct MergeData
{
    /* merge candidate data, cached between calls to xMergeEstimation */
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

inline int getTUBits(int idx, int numIdx)
{
    return idx + (idx < numIdx - 1);
}

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// encoder search class
class TEncSearch : public TComPrediction
{
public:

    MotionEstimate   m_me;
    MotionReference* m_mref[2][MAX_NUM_REF + 1];

    TEncSbac***     m_rdSbacCoders;
    TEncSbac*       m_rdGoOnSbacCoder;

protected:

    ShortYuv*       m_qtTempShortYuv;

    coeff_t**       m_qtTempCoeffY;
    coeff_t**       m_qtTempCoeffCb;
    coeff_t**       m_qtTempCoeffCr;
    uint8_t*        m_qtTempTrIdx;
    uint8_t*        m_qtTempCbf[3];

    coeff_t*        m_qtTempTUCoeffY;
    coeff_t*        m_qtTempTUCoeffCb;
    coeff_t*        m_qtTempTUCoeffCr;
    uint8_t*        m_qtTempTransformSkipFlag[3];
    TComYuv         m_qtTempTransformSkipYuv;

    // interface to option
    Encoder*        m_cfg;

    // interface to classes
    TComTrQuant*    m_trQuant;
    TComRdCost*     m_rdCost;
    TEncEntropy*    m_entropyCoder;

    // ME parameters
    int             m_refLagPixels;

public:

    void setRDSbacCoder(TEncSbac*** rdSbacCoders) { m_rdSbacCoders = rdSbacCoders; }

    void setEntropyCoder(TEncEntropy* entropyCoder) { m_entropyCoder = entropyCoder; }

    void setRDGoOnSbacCoder(TEncSbac* rdGoOnSbacCoder) { m_rdGoOnSbacCoder = rdGoOnSbacCoder; }

    void setQP(int QP, double crWeight, double cbWeight);

    TEncSearch();
    virtual ~TEncSearch();

    bool init(Encoder* cfg, TComRdCost* rdCost, TComTrQuant *trQuant);

    uint32_t xModeBitsIntra(TComDataCU* cu, uint32_t mode, uint32_t partOffset, uint32_t depth);
    uint32_t xModeBitsRemIntra(TComDataCU * cu, uint32_t partOffset, uint32_t depth, uint32_t preds[3], uint64_t & mpms);
    uint32_t xUpdateCandList(uint32_t mode, uint64_t cost, uint32_t fastCandNum, uint32_t* CandModeList, uint64_t* CandCostList);

    void estIntraPredQT(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, ShortYuv* resiYuv, TComYuv* reconYuv);

    void getBestIntraModeChroma(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv);

    void estIntraPredChromaQT(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, ShortYuv* resiYuv,
                              TComYuv* reconYuv);

    /// encoder estimation - inter prediction (non-skip)
    bool predInterSearch(TComDataCU* cu, TComYuv* predYuv, bool bMergeOnly, bool bChroma);

    /// encode residual and compute rd-cost for inter mode
    void encodeResAndCalcRdInterCU(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, ShortYuv* resiYuv, ShortYuv* bestResiYuv,
                                   TComYuv* reconYuv, bool bSkipRes, bool curUseRDOQ);

    void xEncPCM(TComDataCU* cu, uint32_t absPartIdx, pixel* fenc, pixel* pcm, pixel* pred, int16_t* residual, pixel* recon, uint32_t stride,
                 uint32_t width, uint32_t height, TextType ttype);

    void IPCMSearch(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, ShortYuv* resiYuv, TComYuv* reconYuv);

    void xRecurIntraCodingQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, TComYuv* fencYuv,
                             TComYuv* predYuv, ShortYuv* resiYuv, uint32_t& distY, bool bCheckFirst,
                             uint64_t& dRDCost);
    void xSetIntraResultQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, TComYuv* reconYuv);

    void generateCoeffRecon(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, ShortYuv* resiYuv, TComYuv* reconYuv, bool skipRes);

    void xEstimateResidualQT(TComDataCU* cu, uint32_t absPartIdx, uint32_t absTUPartIdx, ShortYuv* resiYuv, uint32_t depth,
                             uint64_t &rdCost, uint32_t &outBits, uint32_t &outDist, uint32_t *puiZeroDist, bool curUseRDOQ = true);
    void xSetResidualQTData(TComDataCU* cu, uint32_t absPartIdx, uint32_t absTUPartIdx, ShortYuv* resiYuv, uint32_t depth, bool bSpatial);

    void residualTransformQuantInter(TComDataCU* cu, uint32_t absPartIdx, uint32_t absTUPartIdx, ShortYuv* resiYuv, uint32_t depth, bool curUseRDOQ = true);

    // -------------------------------------------------------------------------------------------------------------------
    // compute symbol bits
    // -------------------------------------------------------------------------------------------------------------------

    uint32_t xSymbolBitsInter(TComDataCU* cu);
    bool isNextSection(TComTURecurse *TUIterator);
    bool isLastSection(TComTURecurse *TUIterator);
    void initSection(TComTURecurse *TUIterator, uint32_t splitMode, uint32_t absPartIdxStep, uint32_t m_absPartIdxTU = 0);
    void offsetSubTUCBFs(TComDataCU* cu, TextType ttype, uint32_t trDepth, uint32_t absPartIdx);

protected:

    // --------------------------------------------------------------------------------------------
    // Intra search
    // --------------------------------------------------------------------------------------------

    void xEncSubdivCbfQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx,  uint32_t absPartIdxStep, uint32_t width, uint32_t height, bool bLuma, bool bChroma);

    void xEncCoeffQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, TextType ttype, const bool splitIntoSubTUs);
    void xEncIntraHeader(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, bool bLuma, bool bChroma);
    uint32_t xGetIntraBitsQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, uint32_t absPartIdxStep, bool bLuma, bool bChroma);
    uint32_t xGetIntraBitsQTChroma(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, uint32_t chromaId, const bool splitIntoSubTUs);
    void xIntraCodingLumaBlk(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, TComYuv* fencYuv, TComYuv* predYuv,
                             ShortYuv* resiYuv, uint32_t& outDist, bool bReusePred = false);

    void xIntraCodingChromaBlk(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, uint32_t absPartIdxStep, TComYuv* fencYuv, TComYuv* predYuv,
                               ShortYuv* resiYuv, uint32_t& outDist, uint32_t uiChromaId, bool bReusePred = false);

    void xRecurIntraChromaCodingQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, TComYuv* fencYuv,
                                   TComYuv* predYuv, ShortYuv* resiYuv, uint32_t& outDist);

    void residualTransformQuantIntra(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, TComYuv* fencYuv,
                                     TComYuv* predYuv, ShortYuv* resiYuv, TComYuv* reconYuv);
    void residualQTIntrachroma(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, TComYuv* fencYuv,
                               TComYuv* predYuv, ShortYuv* resiYuv, TComYuv* reconYuv);

    void xSetIntraResultChromaQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, TComYuv* reconYuv);

    void xStoreIntraResultQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx);
    void xLoadIntraResultQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx);
    void xStoreIntraResultChromaQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, uint32_t stateU0V1Both2, const bool splitIntoSubTUs);
    void xLoadIntraResultChromaQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, uint32_t stateU0V1Both2, const bool splitIntoSubTUs);

    // --------------------------------------------------------------------------------------------
    // Inter search (AMP)
    // --------------------------------------------------------------------------------------------

    void xCheckBestMVP(AMVPInfo* amvpInfo, MV cMv, MV& mvPred, int& mvpIdx,
                       uint32_t& outBits, uint32_t& outCost);

    void xGetBlkBits(PartSize cuMode, bool bPSlice, int partIdx, uint32_t lastMode, uint32_t blockBit[3]);

    uint32_t xMergeEstimation(TComDataCU* cu, int partIdx, MergeData& m);

    // -------------------------------------------------------------------------------------------------------------------
    // motion estimation
    // -------------------------------------------------------------------------------------------------------------------

    void xSetSearchRange(TComDataCU* cu, MV mvp, int merange, MV& mvmin, MV& mvmax);

    // -------------------------------------------------------------------------------------------------------------------
    // T & Q & Q-1 & T-1
    // -------------------------------------------------------------------------------------------------------------------

    void xEncodeResidualQT(TComDataCU* cu, uint32_t absPartIdx, uint32_t depth, bool bSubdivAndCbf, TextType ttype);
};
}
//! \}

#endif // ifndef X265_TENCSEARCH_H
