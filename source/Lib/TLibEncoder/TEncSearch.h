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

class TEncCu;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// encoder search class
class TEncSearch : public TComPrediction
{
public:

    x265::MotionEstimate m_me;

protected:

    TShortYUV*      m_qtTempTComYuv;
    Pel*            m_sharedPredTransformSkip[3];

    TCoeff**        m_qtTempCoeffY;
    TCoeff**        m_qtTempCoeffCb;
    TCoeff**        m_qtTempCoeffCr;
    Int**           m_qtTempArlCoeffY;
    Int**           m_qtTempArlCoeffCb;
    Int**           m_qtTempArlCoeffCr;
    UChar*          m_qtTempTrIdx;
    UChar*          m_qtTempCbf[3];

    TCoeff*         m_qtTempTUCoeffY;
    TCoeff*         m_qtTempTUCoeffCb;
    TCoeff*         m_qtTempTUCoeffCr;
    Int*            m_qtTempTUArlCoeffY;
    Int*            m_qtTempTUArlCoeffCb;
    Int*            m_qtTempTUArlCoeffCr;
    UChar*          m_qtTempTransformSkipFlag[3];
    TComYuv         m_qtTempTransformSkipTComYuv;

    x265::BitCost   m_bc; // TODO: m_bc will go away with HM ME

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
    x265::MV        m_mvPredictors[3];

    // RD computation
    DistParam       m_distParam;
    Int             m_mvCostScale;

    TComYuv         m_tmpYuvPred; // to avoid constant memory allocation/deallocation in xGetInterPredictionError()
    Pel*            m_tempPel;    // avoid mallocs in xEstimateResidualQT

    // AMVP cost of a given mvp index for a given mvp candidate count
    UInt            m_mvpIdxCost[AMVP_MAX_NUM_CANDS + 1][AMVP_MAX_NUM_CANDS + 1];

public:

    TEncSbac***     m_rdSbacCoders;
    TEncSbac*       m_rdGoOnSbacCoder;

    Void setRDSbacCoder(TEncSbac*** rdSbacCoders) { m_rdSbacCoders = rdSbacCoders; }

    Void setEntropyCoder(TEncEntropy* entropyCoder) { m_entropyCoder = entropyCoder; }

    Void setRDGoOnSbacCoder(TEncSbac* rdGoOnSbacCoder) { m_rdGoOnSbacCoder = rdGoOnSbacCoder; }

    Void setQPLambda(Int QP, Double lambdaLuma, Double lambdaChroma);

    TEncSearch();
    virtual ~TEncSearch();

    Void init(TEncCfg* cfg, TComRdCost* rdCost, TComTrQuant *trQuant);

protected:

    // integer motion search
    typedef struct
    {
        Pel*  fref;
        Int   lumaStride;
        Int   bestx;
        Int   besty;
        UInt  bestRound;
        UInt  bestDistance;
        UInt  bcost;
        UChar bestPointDir;
    } IntTZSearchStruct;

    inline Void xTZSearchHelp(TComPattern* patternKey, IntTZSearchStruct& data, Int searchX, Int searchY, UChar pointDir, UInt distance);
    inline Void xTZ2PointSearch(TComPattern* patternKey, IntTZSearchStruct& data, x265::MV* mvmin, x265::MV* mvmax);
    inline Void xTZ8PointDiamondSearch(TComPattern* patternKey, IntTZSearchStruct& data, x265::MV* mvmin, x265::MV* mvmax,
                                       Int startX, Int startY, Int distance);

    /// motion vector refinement used in fractional-pel accuracy
    UInt xPatternRefinement(TComPattern* patternKey, x265::MV baseRefMv, Int fracBits, x265::MV& outFracMv, TComPicYuv* refPic, Int offset,
                            TComDataCU* cu, UInt partAddr);

    UInt xGetInterPredictionError(TComDataCU* cu, TComYuv* fencYuv, Int partIdx);

public:

    UInt xModeBitsIntra(TComDataCU* cu, UInt mode, UInt pu, UInt partOffset, UInt depth, UInt initTrDepth);
    UInt xUpdateCandList(UInt mode, UInt64 cost, UInt fastCandNum, UInt* CandModeList, UInt64* CandCostList);

    Void preestChromaPredMode(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv);
    Void estIntraPredQT(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, TShortYUV* resiYuv, TComYuv* reconYuv, UInt& ruiDistC, Bool bLumaOnly);

    Void estIntraPredChromaQT(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, TShortYUV* resiYuv,
                              TComYuv* reconYuv, UInt precalcDistC);

    /// encoder estimation - inter prediction (non-skip)
    Void predInterSearch(TComDataCU* cu, TComYuv* fencYuv, TComYuv*& predYuv, Bool bUseMRG = false);

    /// encode residual and compute rd-cost for inter mode
    Void encodeResAndCalcRdInterCU(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, TShortYUV*& resiYuv, TShortYUV*& bestResiYuv,
                                   TComYuv*& reconYuv, Bool bSkipRes);

    /// set ME search range
    Void setAdaptiveSearchRange(Int dir, Int refIdx, Int merange) { m_adaptiveRange[dir][refIdx] = merange; }

    Void xEncPCM(TComDataCU* cu, UInt absPartIdx, Pel* fenc, Pel* pcm, Pel* pred, Short* residual, Pel* recon, UInt stride,
                 UInt width, UInt height, TextType ttype);

    Void IPCMSearch(TComDataCU* cu, TComYuv* fencYuv, TComYuv*& predYuv, TShortYUV*& resiYuv, TComYuv*& reconYuv);

    UInt estimateHeaderBits(TComDataCU* cu, UInt absPartIdx);

    Void xRecurIntraCodingQT(TComDataCU* cu, UInt trDepth, UInt absPartIdx, Bool bLumaOnly, TComYuv* fencYuv,
                             TComYuv* predYuv, TShortYUV* resiYuv, UInt& distY, UInt& distC, Bool bCheckFirst,
                             UInt64& dRDCost);

    Void xSetIntraResultQT(TComDataCU* cu, UInt trDepth, UInt absPartIdx, Bool bLumaOnly, TComYuv* reconYuv);

protected:

    // --------------------------------------------------------------------------------------------
    // Intra search
    // --------------------------------------------------------------------------------------------

    Void xEncSubdivCbfQT(TComDataCU* cu, UInt trDepth, UInt absPartIdx, Bool bLuma, Bool bChroma);

    Void xEncCoeffQT(TComDataCU* cu, UInt trDepth, UInt absPartIdx, TextType ttype);
    Void xEncIntraHeader(TComDataCU* cu, UInt trDepth, UInt absPartIdx, Bool bLuma, Bool bChroma);
    UInt xGetIntraBitsQT(TComDataCU* cu, UInt trDepth, UInt absPartIdx, Bool bLuma, Bool bChroma);
    UInt xGetIntraBitsQTChroma(TComDataCU* cu, UInt trDepth, UInt absPartIdx, UInt uiChromaId);
    Void xIntraCodingLumaBlk(TComDataCU* cu, UInt trDepth, UInt absPartIdx, TComYuv* fencYuv, TComYuv* predYuv,
                             TShortYUV* resiYuv, UInt& outDist, Int default0Save1Load2 = 0);

    Void xIntraCodingChromaBlk(TComDataCU* cu, UInt trDepth, UInt absPartIdx, TComYuv* fencYuv, TComYuv* predYuv,
                               TShortYUV* resiYuv, UInt& outDist, UInt uiChromaId, Int default0Save1Load2 = 0);

    Void xRecurIntraChromaCodingQT(TComDataCU* cu, UInt trDepth, UInt absPartIdx, TComYuv* fencYuv,
                                   TComYuv* predYuv, TShortYUV* resiYuv, UInt& outDist);

    Void xSetIntraResultChromaQT(TComDataCU* cu, UInt trDepth, UInt absPartIdx, TComYuv* reconYuv);

    Void xStoreIntraResultQT(TComDataCU* cu, UInt trDepth, UInt absPartIdx, Bool bLumaOnly);
    Void xLoadIntraResultQT(TComDataCU* cu, UInt trDepth, UInt absPartIdx, Bool bLumaOnly);
    Void xStoreIntraResultChromaQT(TComDataCU* cu, UInt trDepth, UInt absPartIdx, UInt stateU0V1Both2);
    Void xLoadIntraResultChromaQT(TComDataCU* cu, UInt trDepth, UInt absPartIdx, UInt stateU0V1Both2);

    // --------------------------------------------------------------------------------------------
    // Inter search (AMP)
    // --------------------------------------------------------------------------------------------

    Void xEstimateMvPredAMVP(TComDataCU* cu, TComYuv* fencYuv, UInt partIdx, RefPicList picList,
                             Int refIdx, x265::MV& mvPred, Bool bFilled = false, UInt* distBiP = NULL);

    Void xCheckBestMVP(TComDataCU* cu, RefPicList picList, x265::MV cMv, x265::MV& mvPred, Int& mvpIdx,
                       UInt& outBits, UInt& outCost);

    UInt xGetTemplateCost(TComDataCU* cu, UInt partIdx, UInt partAddr, TComYuv* fencYuv,
                          TComYuv* templateCand, x265::MV mvCand, Int mvpIdx, Int mvpCandCount,
                          RefPicList picList, Int refIdx, Int sizex, Int sizey);

    Void xCopyAMVPInfo(AMVPInfo* src, AMVPInfo* dst);
    UInt xGetMvpIdxBits(Int idx, Int num);
    Void xGetBlkBits(PartSize cuMode, Bool bPSlice, Int partIdx, UInt lastMode, UInt blockBit[3]);

    Void xMergeEstimation(TComDataCU* cu, TComYuv* fencYuv, Int partIdx, UInt& uiInterDir,
                          TComMvField* pacMvField, UInt& mergeIndex, UInt& outCost,
                          TComMvField* mvFieldNeighbors, UChar* interDirNeighbors, Int& numValidMergeCand);

    Void xRestrictBipredMergeCand(TComDataCU* cu, UInt puIdx, TComMvField* mvFieldNeighbours,
                                  UChar* interDirNeighbours, Int numValidMergeCand);

    // -------------------------------------------------------------------------------------------------------------------
    // motion estimation
    // -------------------------------------------------------------------------------------------------------------------

    Void xMotionEstimation(TComDataCU* cu, TComYuv* fencYuv, Int partIdx, RefPicList picList, x265::MV* mvp,
                           Int refIdxPred, x265::MV& outmv, UInt& outBits, UInt& outCost, Bool bBi = false);

    Void xSetSearchRange(TComDataCU* cu, x265::MV mvp, Int merange, x265::MV& mvmin, x265::MV& mvmax);

    Void xPatternSearchFast(TComDataCU* cu, TComPattern* patternKey, Pel* refY, Int stride,
                            x265::MV* mvmin, x265::MV* mvmax, x265::MV& outmv, UInt& ruiSAD);

    Void xPatternSearch(TComPattern* patternKey, Pel* refY, Int stride, x265::MV* mvmin, x265::MV* mvmax,
                        x265::MV& outmv, UInt& ruiSAD);

    Void xPatternSearchFracDIF(TComDataCU* cu, TComPattern* patternKey, Pel* refY, Int stride,
                               x265::MV* mvfpel, x265::MV& mvhpel, x265::MV& mvqpel, UInt& outCost,
                               Bool biPred, TComPicYuv* refPic, UInt partAddr);

    Void xExtDIFUpSamplingH(TComPattern* pcPattern, Bool biPred);
    Void xExtDIFUpSamplingQ(TComPattern* patternKey, x265::MV halfPelRef, Bool biPred);

    // -------------------------------------------------------------------------------------------------------------------
    // T & Q & Q-1 & T-1
    // -------------------------------------------------------------------------------------------------------------------

    Void xEncodeResidualQT(TComDataCU* cu, UInt absPartIdx, UInt depth, Bool bSubdivAndCbf, TextType ttype);
    Void xEstimateResidualQT(TComDataCU* cu, UInt quadrant, UInt absPartIdx, UInt absTUPartIdx, TShortYUV* resiYuv, UInt depth, UInt64 &rdCost, UInt &outBits, UInt &outDist, UInt *puiZeroDist);
    Void xSetResidualQTData(TComDataCU* cu, UInt quadrant, UInt absPartIdx, UInt absTUPartIdx, TShortYUV* resiYuv, UInt depth, Bool bSpatial);

    // -------------------------------------------------------------------------------------------------------------------
    // compute symbol bits
    // -------------------------------------------------------------------------------------------------------------------

    Void xAddSymbolBitsInter(TComDataCU* cu, UInt qp, UInt trMode, UInt& outBits,
                             TShortYUV*& outReconYuv, TComYuv* predYuv, TShortYUV*& outResiYuv);

    Void setWpScalingDistParam(TComDataCU* cu, Int refIdx, RefPicList picList);
};

//! \}

#endif // __TENCSEARCH__
