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
    x265::BitCost        m_bc; // TODO: m_bc will go away with HM ME

    TCoeff**        m_ppcQTTempCoeffY;
    TCoeff**        m_ppcQTTempCoeffCb;
    TCoeff**        m_ppcQTTempCoeffCr;
    TCoeff*         m_pcQTTempCoeffY;
    TCoeff*         m_pcQTTempCoeffCb;
    TCoeff*         m_pcQTTempCoeffCr;
    Int**           m_ppcQTTempArlCoeffY;
    Int**           m_ppcQTTempArlCoeffCb;
    Int**           m_ppcQTTempArlCoeffCr;
    Int*            m_pcQTTempArlCoeffY;
    Int*            m_pcQTTempArlCoeffCb;
    Int*            m_pcQTTempArlCoeffCr;
    UChar*          m_puhQTTempTrIdx;
    UChar*          m_puhQTTempCbf[3];

    TShortYUV*      m_pcQTTempTComYuv;
    TComYuv         m_tmpYuvPred; // To be used in xGetInterPredictionError() to avoid constant memory allocation/deallocation
    Pel*            m_pSharedPredTransformSkip[3];
    TCoeff*         m_pcQTTempTUCoeffY;
    TCoeff*         m_pcQTTempTUCoeffCb;
    TCoeff*         m_pcQTTempTUCoeffCr;
    UChar*          m_puhQTTempTransformSkipFlag[3];
    TComYuv         m_pcQTTempTransformSkipTComYuv;
    Int*            m_ppcQTTempTUArlCoeffY;
    Int*            m_ppcQTTempTUArlCoeffCb;
    Int*            m_ppcQTTempTUArlCoeffCr;

protected:

    // interface to option
    TEncCfg*        m_pcEncCfg;

    // interface to classes
    TComTrQuant*    m_pcTrQuant;
    TComRdCost*     m_pcRdCost;
    TEncEntropy*    m_pcEntropyCoder;

    // ME parameters
    Int             m_iSearchRange;
    Int             m_bipredSearchRange; // Search range for bi-prediction
    Int             m_iSearchMethod;
    Int             m_adaptiveRange[2][33];
    x265::MV        m_cSrchRngLT;
    x265::MV        m_cSrchRngRB;
    x265::MV        m_mvPredictors[3];

    // RD computation

    DistParam       m_cDistParam;

    // Misc.
    Pel*            m_pTempPel;
    const UInt*     m_puiDFilter;

    // AMVP cost computation
    UInt            m_mvpIdxCost[AMVP_MAX_NUM_CANDS + 1][AMVP_MAX_NUM_CANDS + 1]; //th array bounds

public:

    TEncSbac***     m_pppcRDSbacCoder;
    TEncSbac*       m_pcRDGoOnSbacCoder;

    Void set_pppcRDSbacCoder(TEncSbac*** pppcRDSbacCoder) { m_pppcRDSbacCoder = pppcRDSbacCoder; }

    Void set_pcEntropyCoder(TEncEntropy* pcEntropyCoder) { m_pcEntropyCoder = pcEntropyCoder; }

    Void set_pcRDGoOnSbacCoder(TEncSbac* pcRDGoOnSbacCoder) { m_pcRDGoOnSbacCoder = pcRDGoOnSbacCoder; }

    Void setQPLambda(Int QP, Double lambdaLuma, Double lambdaChroma);

    TEncSearch();
    virtual ~TEncSearch();

    Void init(TEncCfg* pcEncCfg, TComRdCost* pcRdCost, TComTrQuant *pcTrQuant);
    UInt xModeBitsIntra(TComDataCU* cu, UInt uiMode, UInt uiPU, UInt uiPartOffset, UInt uiDepth, UInt uiInitTrDepth);
    UInt xUpdateCandList(UInt uiMode, UInt64 uiCost, UInt uiFastCandNum, UInt * CandModeList, UInt64 * CandCostList);

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
    UInt  xPatternRefinement(TComPattern* patternKey, x265::MV baseRefMv, Int fracBits, x265::MV& outFracMv, TComPicYuv* refPic, Int offset,
                             TComDataCU* cu, UInt partAddr);

    UInt xGetInterPredictionError(TComDataCU* cu, TComYuv* fencYuv, Int partIdx);

public:

    Void  preestChromaPredMode(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv);
    Void  estIntraPredQT(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, TShortYUV* resiYuv, TComYuv* reconYuv, UInt& ruiDistC, Bool bLumaOnly);

    Void  estIntraPredChromaQT(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, TShortYUV* resiYuv,
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

    Void  xRecurIntraCodingQT(TComDataCU* cu, UInt trDepth, UInt absPartIdx, Bool bLumaOnly, TComYuv* fencYuv,
                              TComYuv* predYuv, TShortYUV* resiYuv, UInt& distY, UInt& distC, Bool bCheckFirst,
                              UInt64& dRDCost);

    Void  xSetIntraResultQT(TComDataCU* cu,
                            UInt        trDepth,
                            UInt        uiAbsPartIdx,
                            Bool        bLumaOnly,
                            TComYuv*    pcRecoYuv);

protected:

    // -------------------------------------------------------------------------------------------------------------------
    // Intra search
    // -------------------------------------------------------------------------------------------------------------------

    Void  xEncSubdivCbfQT(TComDataCU* cu,
                          UInt        trDepth,
                          UInt        uiAbsPartIdx,
                          Bool        bLuma,
                          Bool        bChroma);

    Void  xEncCoeffQT(TComDataCU* cu,
                      UInt        trDepth,
                      UInt        uiAbsPartIdx,
                      TextType    eTextType,
                      Bool        bRealCoeff);
    Void  xEncIntraHeader(TComDataCU* cu,
                          UInt        trDepth,
                          UInt        uiAbsPartIdx,
                          Bool        bLuma,
                          Bool        bChroma);
    UInt  xGetIntraBitsQT(TComDataCU* cu,
                          UInt        trDepth,
                          UInt        uiAbsPartIdx,
                          Bool        bLuma,
                          Bool        bChroma,
                          Bool        bRealCoeff);
    UInt  xGetIntraBitsQTChroma(TComDataCU* cu,
                                UInt        trDepth,
                                UInt        uiAbsPartIdx,
                                UInt        uiChromaId,
                                Bool        bRealCoeff);

    Void  xIntraCodingLumaBlk(TComDataCU* cu,
                              UInt        trDepth,
                              UInt        uiAbsPartIdx,
                              TComYuv*    fencYuv,
                              TComYuv*    pcPredYuv,
                              TShortYUV*  pcResiYuv,
                              UInt&       ruiDist,
                              Int         default0Save1Load2 = 0);
    Void  xIntraCodingChromaBlk(TComDataCU* cu,
                                UInt        trDepth,
                                UInt        uiAbsPartIdx,
                                TComYuv*    fencYuv,
                                TComYuv*    pcPredYuv,
                                TShortYUV*  pcResiYuv,
                                UInt&       ruiDist,
                                UInt        uiChromaId,
                                Int         default0Save1Load2 = 0);

    Void  xRecurIntraChromaCodingQT(TComDataCU* cu,
                                    UInt        trDepth,
                                    UInt        uiAbsPartIdx,
                                    TComYuv*    fencYuv,
                                    TComYuv*    pcPredYuv,
                                    TShortYUV*  pcResiYuv,
                                    UInt&       ruiDist);
    Void  xSetIntraResultChromaQT(TComDataCU* cu,
                                  UInt        trDepth,
                                  UInt        uiAbsPartIdx,
                                  TComYuv*    pcRecoYuv);

    Void  xStoreIntraResultQT(TComDataCU* cu,
                              UInt        trDepth,
                              UInt        uiAbsPartIdx,
                              Bool        bLumaOnly);
    Void  xLoadIntraResultQT(TComDataCU* cu,
                             UInt        trDepth,
                             UInt        uiAbsPartIdx,
                             Bool        bLumaOnly);
    Void xStoreIntraResultChromaQT(TComDataCU* cu,
                                   UInt        trDepth,
                                   UInt        uiAbsPartIdx,
                                   UInt        stateU0V1Both2);
    Void xLoadIntraResultChromaQT(TComDataCU* cu,
                                  UInt        trDepth,
                                  UInt        uiAbsPartIdx,
                                  UInt        stateU0V1Both2);

    // -------------------------------------------------------------------------------------------------------------------
    // Inter search (AMP)
    // -------------------------------------------------------------------------------------------------------------------

    Void xEstimateMvPredAMVP(TComDataCU* cu,
                             TComYuv*    fencYuv,
                             UInt        uiPartIdx,
                             RefPicList  picList,
                             Int         iRefIdx,
                             x265::MV&   mvPred,
                             Bool        bFilled = false,
                             UInt*       puiDistBiP = NULL);

    Void xCheckBestMVP(TComDataCU* cu,
                       RefPicList  picList,
                       x265::MV    cMv,
                       x265::MV&   mvPred,
                       Int&        mvpIdx,
                       UInt&       outBits,
                       UInt&       outCost);

    UInt xGetTemplateCost(TComDataCU* cu,
                          UInt        uiPartIdx,
                          UInt        partAddr,
                          TComYuv*    fencYuv,
                          TComYuv*    pcTemplateCand,
                          x265::MV    cMvCand,
                          Int         iMVPIdx,
                          Int         iMVPNum,
                          RefPicList  picList,
                          Int         iRefIdx,
                          Int         iSizeX,
                          Int         iSizeY);

    Void xCopyAMVPInfo(AMVPInfo* pSrc, AMVPInfo* pDst);
    UInt xGetMvpIdxBits(Int iIdx, Int iNum);
    Void xGetBlkBits(PartSize  eCUMode, Bool bPSlice, Int partIdx,  UInt uiLastMode, UInt uiBlkBit[3]);

    Void xMergeEstimation(TComDataCU*  cu,
                          TComYuv*     fencYuv,
                          Int          partIdx,
                          UInt&        uiInterDir,
                          TComMvField* pacMvField,
                          UInt&        uiMergeIndex,
                          UInt&        outCost,
                          TComMvField* cMvFieldNeighbours,
                          UChar*       uhInterDirNeighbours,
                          Int&         numValidMergeCand);

    Void xRestrictBipredMergeCand(TComDataCU*  cu,
                                  UInt         puIdx,
                                  TComMvField* mvFieldNeighbours,
                                  UChar*       interDirNeighbours,
                                  Int          numValidMergeCand);

    // -------------------------------------------------------------------------------------------------------------------
    // motion estimation
    // -------------------------------------------------------------------------------------------------------------------

    Void xMotionEstimation(TComDataCU* cu,
                           TComYuv*    fencYuv,
                           Int         partIdx,
                           RefPicList  picList,
                           x265::MV*   pcMvPred,
                           Int         iRefIdxPred,
                           x265::MV&   rcMv,
                           UInt&       outBits,
                           UInt&       outCost,
                           Bool        bBi = false);

    Void xSetSearchRange(TComDataCU* cu,
                         x265::MV    cMvPred,
                         Int         iSrchRng,
                         x265::MV&   rcMvSrchRngLT,
                         x265::MV&   rcMvSrchRngRB);

    Void xPatternSearchFast(TComDataCU*  cu,
                            TComPattern* pcPatternKey,
                            Pel*         piRefY,
                            Int          iRefStride,
                            x265::MV*    pcMvSrchRngLT,
                            x265::MV*    pcMvSrchRngRB,
                            x265::MV&    rcMv,
                            UInt&        ruiSAD);

    Void xPatternSearch(TComPattern* pcPatternKey,
                        Pel*         piRefY,
                        Int          iRefStride,
                        x265::MV*    pcMvSrchRngLT,
                        x265::MV*    pcMvSrchRngRB,
                        x265::MV&    rcMv,
                        UInt&        ruiSAD);

    Void xPatternSearchFracDIF(TComDataCU*  cu,
                               TComPattern* pcPatternKey,
                               Pel*         piRefY,
                               Int          iRefStride,
                               x265::MV*    pcMvInt,
                               x265::MV&    rcMvHalf,
                               x265::MV&    rcMvQter,
                               UInt&        outCost,
                               Bool         biPred,
                               TComPicYuv*  refPic,
                               UInt         partAddr);

    Void xExtDIFUpSamplingH(TComPattern* pcPattern, Bool biPred);
    Void xExtDIFUpSamplingQ(TComPattern* pcPatternKey, x265::MV halfPelRef, Bool biPred);

    // -------------------------------------------------------------------------------------------------------------------
    // T & Q & Q-1 & T-1
    // -------------------------------------------------------------------------------------------------------------------

    Void xEncodeResidualQT(TComDataCU* cu, UInt uiAbsPartIdx, const UInt uiDepth, Bool bSubdivAndCbf, TextType eType);
    Void xEstimateResidualQT(TComDataCU* cu, UInt uiQuadrant, UInt uiAbsPartIdx, UInt absTUPartIdx, TShortYUV* pcResi, const UInt uiDepth, UInt64 &rdCost, UInt &outBits, UInt &ruiDist, UInt *puiZeroDist);
    Void xSetResidualQTData(TComDataCU* cu, UInt uiQuadrant, UInt uiAbsPartIdx, UInt absTUPartIdx, TShortYUV* pcResi, UInt uiDepth, Bool bSpatial);

    // -------------------------------------------------------------------------------------------------------------------
    // compute symbol bits
    // -------------------------------------------------------------------------------------------------------------------

    Void xAddSymbolBitsInter(TComDataCU* cu,
                             UInt        uiQp,
                             UInt        uiTrMode,
                             UInt&       outBits,
                             TShortYUV*& rpcYuvRec,
                             TComYuv*    predYuv,
                             TShortYUV*& outResiYuv);

    Void  setWpScalingDistParam(TComDataCU* cu, Int iRefIdx, RefPicList eRefPicListCur);
}; // END CLASS DEFINITION TEncSearch

//! \}

#endif // __TENCSEARCH__
