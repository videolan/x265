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

#ifndef __TENCSEARCH__
#define __TENCSEARCH__

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

    MotionEstimate  m_me;
    int             m_referenceRowsAvailable;

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
    Int             m_searchRange;
    Int             m_bipredSearchRange; // Search range for bi-prediction
    Int             m_searchMethod;
    Int             m_adaptiveRange[2][33];
    MV              m_mvPredictors[3];

    TComYuv         m_tmpYuvPred; // to avoid constant memory allocation/deallocation in xGetInterPredictionError()
    Pel*            m_tempPel;    // avoid mallocs in xEstimateResidualQT

    // AMVP cost of a given mvp index for a given mvp candidate count
    UInt            m_mvpIdxCost[AMVP_MAX_NUM_CANDS + 1][AMVP_MAX_NUM_CANDS + 1];

public:

    TEncSbac***     m_rdSbacCoders;
    TEncSbac*       m_rdGoOnSbacCoder;

    void setRDSbacCoder(TEncSbac*** rdSbacCoders) { m_rdSbacCoders = rdSbacCoders; }

    void setEntropyCoder(TEncEntropy* entropyCoder) { m_entropyCoder = entropyCoder; }

    void setRDGoOnSbacCoder(TEncSbac* rdGoOnSbacCoder) { m_rdGoOnSbacCoder = rdGoOnSbacCoder; }

    void setQPLambda(Int QP, Double lambdaLuma, Double lambdaChroma);

    TEncSearch();
    virtual ~TEncSearch();

    void init(TEncCfg* cfg, TComRdCost* rdCost, TComTrQuant *trQuant);

protected:

    /// motion vector refinement used in fractional-pel accuracy
    UInt xPatternRefinement(TComPattern* patternKey, Pel *fenc, Int fracBits, x265::MV& outFracMv, TComPicYuv* refPic, TComDataCU* cu, UInt partAddr);

    UInt xGetInterPredictionError(TComDataCU* cu, Int partIdx);

public:

    UInt xModeBitsIntra(TComDataCU* cu, UInt mode, UInt partOffset, UInt depth, UInt initTrDepth);
    UInt xUpdateCandList(UInt mode, UInt64 cost, UInt fastCandNum, UInt* CandModeList, UInt64* CandCostList);

    void preestChromaPredMode(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv);
    void estIntraPredQT(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, TShortYUV* resiYuv, TComYuv* reconYuv, UInt& ruiDistC, Bool bLumaOnly);

    void estIntraPredChromaQT(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, TShortYUV* resiYuv,
                              TComYuv* reconYuv, UInt precalcDistC);

    /// encoder estimation - inter prediction (non-skip)
    void predInterSearch(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, Bool bUseMRG = false);

    /// encode residual and compute rd-cost for inter mode
    void encodeResAndCalcRdInterCU(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, TShortYUV* resiYuv, TShortYUV* bestResiYuv,
                                   TComYuv* reconYuv, Bool bSkipRes);

    /// set ME search range
    void setAdaptiveSearchRange(Int dir, Int refIdx, Int merange) { m_adaptiveRange[dir][refIdx] = merange; }

    void xEncPCM(TComDataCU* cu, UInt absPartIdx, Pel* fenc, Pel* pcm, Pel* pred, short* residual, Pel* recon, UInt stride,
                 UInt width, UInt height, TextType ttype);

    void IPCMSearch(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, TShortYUV* resiYuv, TComYuv* reconYuv);

    UInt estimateHeaderBits(TComDataCU* cu, UInt absPartIdx);

    void xRecurIntraCodingQT(TComDataCU* cu, UInt trDepth, UInt absPartIdx, Bool bLumaOnly, TComYuv* fencYuv,
                             TComYuv* predYuv, TShortYUV* resiYuv, UInt& distY, UInt& distC, Bool bCheckFirst,
                             UInt64& dRDCost);

    void xSetIntraResultQT(TComDataCU* cu, UInt trDepth, UInt absPartIdx, Bool bLumaOnly, TComYuv* reconYuv);

protected:

    // --------------------------------------------------------------------------------------------
    // Intra search
    // --------------------------------------------------------------------------------------------

    void xEncSubdivCbfQT(TComDataCU* cu, UInt trDepth, UInt absPartIdx, Bool bLuma, Bool bChroma);

    void xEncCoeffQT(TComDataCU* cu, UInt trDepth, UInt absPartIdx, TextType ttype);
    void xEncIntraHeader(TComDataCU* cu, UInt trDepth, UInt absPartIdx, Bool bLuma, Bool bChroma);
    UInt xGetIntraBitsQT(TComDataCU* cu, UInt trDepth, UInt absPartIdx, Bool bLuma, Bool bChroma);
    UInt xGetIntraBitsQTChroma(TComDataCU* cu, UInt trDepth, UInt absPartIdx, UInt uiChromaId);
    void xIntraCodingLumaBlk(TComDataCU* cu, UInt trDepth, UInt absPartIdx, TComYuv* fencYuv, TComYuv* predYuv,
                             TShortYUV* resiYuv, UInt& outDist, Int default0Save1Load2 = 0);

    void xIntraCodingChromaBlk(TComDataCU* cu, UInt trDepth, UInt absPartIdx, TComYuv* fencYuv, TComYuv* predYuv,
                               TShortYUV* resiYuv, UInt& outDist, UInt uiChromaId, Int default0Save1Load2 = 0);

    void xRecurIntraChromaCodingQT(TComDataCU* cu, UInt trDepth, UInt absPartIdx, TComYuv* fencYuv,
                                   TComYuv* predYuv, TShortYUV* resiYuv, UInt& outDist);

    void xSetIntraResultChromaQT(TComDataCU* cu, UInt trDepth, UInt absPartIdx, TComYuv* reconYuv);

    void xStoreIntraResultQT(TComDataCU* cu, UInt trDepth, UInt absPartIdx, Bool bLumaOnly);
    void xLoadIntraResultQT(TComDataCU* cu, UInt trDepth, UInt absPartIdx, Bool bLumaOnly);
    void xStoreIntraResultChromaQT(TComDataCU* cu, UInt trDepth, UInt absPartIdx, UInt stateU0V1Both2);
    void xLoadIntraResultChromaQT(TComDataCU* cu, UInt trDepth, UInt absPartIdx, UInt stateU0V1Both2);

    // --------------------------------------------------------------------------------------------
    // Inter search (AMP)
    // --------------------------------------------------------------------------------------------

    void xEstimateMvPredAMVP(TComDataCU* cu, UInt partIdx, RefPicList picList, Int refIdx,
                             x265::MV& mvPred, UInt* distBiP = NULL);

    void xCheckBestMVP(TComDataCU* cu, RefPicList picList, x265::MV cMv, x265::MV& mvPred, Int& mvpIdx,
                       UInt& outBits, UInt& outCost);

    UInt xGetTemplateCost(TComDataCU* cu, UInt partAddr, TComYuv* templateCand, x265::MV mvCand, Int mvpIdx,
                          Int mvpCandCount, RefPicList picList, Int refIdx, Int sizex, Int sizey);

    void xCopyAMVPInfo(AMVPInfo* src, AMVPInfo* dst);
    UInt xGetMvpIdxBits(Int idx, Int num);
    void xGetBlkBits(PartSize cuMode, Bool bPSlice, Int partIdx, UInt lastMode, UInt blockBit[3]);

    void xMergeEstimation(TComDataCU* cu, Int partIdx, UInt& uiInterDir,
                          TComMvField* pacMvField, UInt& mergeIndex, UInt& outCost,
                          TComMvField* mvFieldNeighbors, UChar* interDirNeighbors, Int& numValidMergeCand);

    void xRestrictBipredMergeCand(TComDataCU* cu, UInt puIdx, TComMvField* mvFieldNeighbours,
                                  UChar* interDirNeighbours, Int numValidMergeCand);

    // -------------------------------------------------------------------------------------------------------------------
    // motion estimation
    // -------------------------------------------------------------------------------------------------------------------

    void xMotionEstimation(TComDataCU* cu, TComYuv* fencYuv, Int partIdx, RefPicList picList, x265::MV* mvp,
                           Int refIdxPred, x265::MV& outmv, UInt& outBits, UInt& outCost);

    void xSetSearchRange(TComDataCU* cu, x265::MV mvp, Int merange, x265::MV& mvmin, x265::MV& mvmax);

    void xPatternSearch(TComPattern* patternKey, Pel *fenc, Pel* refY, Int stride, x265::MV* mvmin, x265::MV* mvmax,
                        x265::MV& outmv, UInt& ruiSAD);

    void xExtDIFUpSamplingH(TComPattern* pcPattern);
    void xExtDIFUpSamplingQ(TComPattern* patternKey, x265::MV halfPelRef);

    // -------------------------------------------------------------------------------------------------------------------
    // T & Q & Q-1 & T-1
    // -------------------------------------------------------------------------------------------------------------------

    void xEncodeResidualQT(TComDataCU* cu, UInt absPartIdx, UInt depth, Bool bSubdivAndCbf, TextType ttype);
    void xEstimateResidualQT(TComDataCU* cu, UInt absPartIdx, UInt absTUPartIdx, TShortYUV* resiYuv, UInt depth,
                             UInt64 &rdCost, UInt &outBits, UInt &outDist, UInt *puiZeroDist);
    void xSetResidualQTData(TComDataCU* cu, UInt absPartIdx, UInt absTUPartIdx, TShortYUV* resiYuv, UInt depth, Bool bSpatial);

    // -------------------------------------------------------------------------------------------------------------------
    // compute symbol bits
    // -------------------------------------------------------------------------------------------------------------------

    UInt xSymbolBitsInter(TComDataCU* cu);

    void setWpScalingDistParam(TComDataCU* cu, Int refIdx, RefPicList picList);
};
}
//! \}

#endif // __TENCSEARCH__
