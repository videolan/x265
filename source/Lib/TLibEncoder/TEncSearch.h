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
#include "TEncCfg.h"

#include "primitives.h"
#include "bitcost.h"
#include "motion.h"

//! \ingroup TLibEncoder
//! \{

namespace x265 {
// private namespace

class TEncCu;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// encoder search class
class TEncSearch : public TComPrediction
{
public:

    MotionEstimate   m_me;
    MotionReference* m_mref[2][MAX_NUM_REF + 1];

protected:

    TShortYUV*      m_qtTempTComYuv;
    Pel*            m_sharedPredTransformSkip[3];

    TCoeff**        m_qtTempCoeffY;
    TCoeff**        m_qtTempCoeffCb;
    TCoeff**        m_qtTempCoeffCr;
    UChar*          m_qtTempTrIdx;
    UChar*          m_qtTempCbf[3];

    TCoeff*         m_qtTempTUCoeffY;
    TCoeff*         m_qtTempTUCoeffCb;
    TCoeff*         m_qtTempTUCoeffCr;
    UChar*          m_qtTempTransformSkipFlag[3];
    TComYuv         m_qtTempTransformSkipTComYuv;

    // interface to option
    TEncCfg*        m_cfg;

    // interface to classes
    TComTrQuant*    m_trQuant;
    TComRdCost*     m_rdCost;
    TEncEntropy*    m_entropyCoder;

    // ME parameters
    int             m_refLagPixels;
    int             m_adaptiveRange[2][33];
    MV              m_mvPredictors[3];

    TComYuv         m_tmpYuvPred; // to avoid constant memory allocation/deallocation in xGetInterPredictionError()
    Pel*            m_tempPel;    // avoid mallocs in xEstimateResidualQT

    // AMVP cost of a given mvp index for a given mvp candidate count
    uint32_t        m_mvpIdxCost[AMVP_MAX_NUM_CANDS + 1][AMVP_MAX_NUM_CANDS + 1];

    // Color space parameters
    int             m_hChromaShift;
    int             m_vChromaShift;

public:

    TEncSbac***     m_rdSbacCoders;
    TEncSbac*       m_rdGoOnSbacCoder;

    void setRDSbacCoder(TEncSbac*** rdSbacCoders) { m_rdSbacCoders = rdSbacCoders; }

    void setEntropyCoder(TEncEntropy* entropyCoder) { m_entropyCoder = entropyCoder; }

    void setRDGoOnSbacCoder(TEncSbac* rdGoOnSbacCoder) { m_rdGoOnSbacCoder = rdGoOnSbacCoder; }

    void setQPLambda(int QP, double lambdaLuma, double lambdaChroma);

    TEncSearch();
    virtual ~TEncSearch();

    void init(TEncCfg* cfg, TComRdCost* rdCost, TComTrQuant *trQuant);

protected:

    uint32_t xGetInterPredictionError(TComDataCU* cu, int partIdx);

public:

    uint32_t xModeBitsIntra(TComDataCU* cu, uint32_t mode, uint32_t partOffset, uint32_t depth, uint32_t initTrDepth);
    uint32_t xUpdateCandList(uint32_t mode, UInt64 cost, uint32_t fastCandNum, uint32_t* CandModeList, UInt64* CandCostList);

    void preestChromaPredMode(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv);
    void estIntraPredQT(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, TShortYUV* resiYuv, TComYuv* reconYuv, uint32_t& ruiDistC, bool bLumaOnly);

    void estIntraPredChromaQT(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, TShortYUV* resiYuv,
                              TComYuv* reconYuv, uint32_t precalcDistC);

    /// encoder estimation - inter prediction (non-skip)
    void predInterSearch(TComDataCU* cu, TComYuv* predYuv, bool bUseMRG = false, bool bLuma = true, bool bChroma = true);

    /// encode residual and compute rd-cost for inter mode
    void encodeResAndCalcRdInterCU(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, TShortYUV* resiYuv, TShortYUV* bestResiYuv,
                                   TComYuv* reconYuv, bool bSkipRes, bool curUseRDOQ = true);

    void estimateRDInterCU(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, TShortYUV* resiYuv, TShortYUV* bestResiYuv,
                           TComYuv* reconYuv, bool bSkipRes, bool curUseRDOQ = true);

    uint32_t estimateZerobits(TComDataCU* cu);

    uint32_t estimateZeroDist(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv);

    void generateRecon(TComDataCU* cu, TComYuv* predYuv, TShortYUV* resiYuv, TComYuv* reconYuv, bool skipRes);

    void estimateBitsDist(TComDataCU* cu, TShortYUV* resiYuv, uint32_t& bits, uint32_t& distortion, bool curUseRDOQ);

    /// set ME search range
    void setAdaptiveSearchRange(int dir, int refIdx, int merange) { m_adaptiveRange[dir][refIdx] = merange; }

    void xEncPCM(TComDataCU* cu, uint32_t absPartIdx, Pel* fenc, Pel* pcm, Pel* pred, int16_t* residual, Pel* recon, uint32_t stride,
                 uint32_t width, uint32_t height, TextType ttype);

    void IPCMSearch(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, TShortYUV* resiYuv, TComYuv* reconYuv);

    uint32_t estimateHeaderBits(TComDataCU* cu, uint32_t absPartIdx);

    void xRecurIntraCodingQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, bool bLumaOnly, TComYuv* fencYuv,
                             TComYuv* predYuv, TShortYUV* resiYuv, uint32_t& distY, uint32_t& distC, bool bCheckFirst,
                             UInt64& dRDCost);

    void xSetIntraResultQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, bool bLumaOnly, TComYuv* reconYuv);

    // -------------------------------------------------------------------------------------------------------------------
    // compute symbol bits
    // -------------------------------------------------------------------------------------------------------------------

    uint32_t xSymbolBitsInter(TComDataCU* cu);

protected:

    // --------------------------------------------------------------------------------------------
    // Intra search
    // --------------------------------------------------------------------------------------------

    void xEncSubdivCbfQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, bool bLuma, bool bChroma);

    void xEncCoeffQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, TextType ttype);
    void xEncIntraHeader(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, bool bLuma, bool bChroma);
    uint32_t xGetIntraBitsQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, bool bLuma, bool bChroma);
    uint32_t xGetIntraBitsQTChroma(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, uint32_t uiChromaId);
    void xIntraCodingLumaBlk(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, TComYuv* fencYuv, TComYuv* predYuv,
                             TShortYUV* resiYuv, uint32_t& outDist, int default0Save1Load2 = 0);

    void xIntraCodingChromaBlk(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, TComYuv* fencYuv, TComYuv* predYuv,
                               TShortYUV* resiYuv, uint32_t& outDist, uint32_t uiChromaId, int default0Save1Load2 = 0);

    void xRecurIntraChromaCodingQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, TComYuv* fencYuv,
                                   TComYuv* predYuv, TShortYUV* resiYuv, uint32_t& outDist);

    void xSetIntraResultChromaQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, TComYuv* reconYuv);

    void xStoreIntraResultQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, bool bLumaOnly);
    void xLoadIntraResultQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, bool bLumaOnly);
    void xStoreIntraResultChromaQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, uint32_t stateU0V1Both2);
    void xLoadIntraResultChromaQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, uint32_t stateU0V1Both2);

    // --------------------------------------------------------------------------------------------
    // Inter search (AMP)
    // --------------------------------------------------------------------------------------------

    void xEstimateMvPredAMVP(TComDataCU* cu, uint32_t partIdx, int picList, int refIdx,
                             MV& mvPred, uint32_t* distBiP = NULL);

    void xCheckBestMVP(TComDataCU* cu, int picList, MV cMv, MV& mvPred, int& mvpIdx,
                       uint32_t& outBits, uint32_t& outCost);

    uint32_t xGetTemplateCost(TComDataCU* cu, uint32_t partAddr, TComYuv* templateCand, MV mvCand, int mvpIdx,
                              int mvpCandCount, int picList, int refIdx, int sizex, int sizey);

    void xCopyAMVPInfo(AMVPInfo* src, AMVPInfo* dst);
    uint32_t xGetMvpIdxBits(int idx, int num);
    void xGetBlkBits(PartSize cuMode, bool bPSlice, int partIdx, uint32_t lastMode, uint32_t blockBit[3]);

    void xMergeEstimation(TComDataCU* cu, int partIdx, uint32_t& uiInterDir,
                          TComMvField* pacMvField, uint32_t& mergeIndex, uint32_t& outCost, uint32_t& outbits,
                          TComMvField* mvFieldNeighbors, UChar* interDirNeighbors, int& numValidMergeCand);

    void xRestrictBipredMergeCand(TComDataCU* cu, uint32_t puIdx, TComMvField* mvFieldNeighbours,
                                  UChar* interDirNeighbours, int numValidMergeCand);

    // -------------------------------------------------------------------------------------------------------------------
    // motion estimation
    // -------------------------------------------------------------------------------------------------------------------

    void xSetSearchRange(TComDataCU* cu, MV mvp, int merange, MV& mvmin, MV& mvmax);

    // -------------------------------------------------------------------------------------------------------------------
    // T & Q & Q-1 & T-1
    // -------------------------------------------------------------------------------------------------------------------

    void xEncodeResidualQT(TComDataCU* cu, uint32_t absPartIdx, uint32_t depth, bool bSubdivAndCbf, TextType ttype);
    void xEstimateResidualQT(TComDataCU* cu, uint32_t absPartIdx, uint32_t absTUPartIdx, TShortYUV* resiYuv, uint32_t depth,
                             UInt64 &rdCost, uint32_t &outBits, uint32_t &outDist, uint32_t *puiZeroDist, bool curUseRDOQ = true);
    void xSetResidualQTData(TComDataCU* cu, uint32_t absPartIdx, uint32_t absTUPartIdx, TShortYUV* resiYuv, uint32_t depth, bool bSpatial);

    void setWpScalingDistParam(TComDataCU* cu, int refIdx, int picList);
};
}
//! \}

#endif // ifndef X265_TENCSEARCH_H
