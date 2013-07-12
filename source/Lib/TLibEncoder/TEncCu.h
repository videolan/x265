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

/** \file     TEncCu.h
    \brief    Coding Unit (CU) encoder class (header)
*/

#ifndef __TENCCU__
#define __TENCCU__

#include "TLibCommon/CommonDef.h"
#include "TLibCommon/TComYuv.h"
#include "TLibCommon/TComPrediction.h"
#include "TLibCommon/TComTrQuant.h"
#include "TLibCommon/TComBitCounter.h"
#include "TLibCommon/TComDataCU.h"
#include "TShortYUV.h"

#include "TEncEntropy.h"
#include "TEncSearch.h"
#include "TEncRateCtrl.h"

//! \ingroup TLibEncoder
//! \{

class TEncTop;
class TEncSbac;
class TEncCavlc;
class TEncSlice;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// CU encoder class
class TEncCu
{
private:

    static const int MAX_PRED_TYPES = 6;

    TComDataCU** m_interCU_2Nx2N;
    TComDataCU** m_interCU_2NxN;
    TComDataCU** m_interCU_Nx2N;
    TComDataCU** m_intraInInterCU;
    TComDataCU** m_mergeCU;
    TComDataCU** m_bestMergeCU;
    TComDataCU** m_bestCU;      ///< Best CUs at each depth
    TComDataCU** m_tempCU;      ///< Temporary CUs at each depth

    TComYuv**    m_bestPredYuv; ///< Best Prediction Yuv for each depth
    TShortYUV**  m_bestResiYuv; ///< Best Residual Yuv for each depth
    TComYuv**    m_bestRecoYuv; ///< Best Reconstruction Yuv for each depth

    TComYuv**    m_tmpPredYuv;  ///< Temporary Prediction Yuv for each depth
    TShortYUV**  m_tmpResiYuv;  ///< Temporary Residual Yuv for each depth
    TComYuv**    m_tmpRecoYuv;  ///< Temporary Reconstruction Yuv for each depth
    TComYuv**    m_modePredYuv[MAX_PRED_TYPES]; ///< Prediction buffers for inter, intra, rect(2) and merge
    TComYuv**    m_bestMergeRecoYuv;
    TComYuv**    m_origYuv;     ///< Original Yuv at each depth

    TEncCfg*     m_cfg;
    TEncSearch*  m_search;
    TComTrQuant* m_trQuant;
    TComRdCost*  m_rdCost;
    TEncEntropy* m_entropyCoder;
    TComBitCounter* m_bitCounter;

    // SBAC RD
    TEncSbac***  m_rdSbacCoders;
    TEncSbac*    m_rdGoOnSbacCoder;
    TEncRateCtrl* m_rateControl;

    UInt         m_LCUPredictionSAD;
    Int          m_addSADDepth;
    Int          m_temporalSAD;
    UChar        m_totalDepth;

    Bool         m_bEncodeDQP;
    Bool         m_abortFlag; // aborts recursion when the child CU costs more than parent CU

public:

    TEncCu();

    Void init(TEncTop* top);
    Void create(UChar totalDepth, UInt maxWidth, UInt maxHeight);
    Void destroy();
    Void compressCU(TComDataCU* cu);
    Void encodeCU(TComDataCU* cu);

    Void setRDSbacCoder(TEncSbac*** rdSbacCoder) { m_rdSbacCoders = rdSbacCoder; }

    Void setEntropyCoder(TEncEntropy* entropyCoder) { m_entropyCoder = entropyCoder; }

    Void setPredSearch(TEncSearch* predSearch) { m_search = predSearch; }

    Void setRDGoOnSbacCoder(TEncSbac* rdGoOnSbacCoder) { m_rdGoOnSbacCoder = rdGoOnSbacCoder; }

    Void setTrQuant(TComTrQuant* trQuant) { m_trQuant = trQuant; }

    Void setRdCost(TComRdCost* rdCost) { m_rdCost = rdCost; }

    Void setBitCounter(TComBitCounter* pcBitCounter) { m_bitCounter = pcBitCounter; }

    UInt getLCUPredictionSAD() { return m_LCUPredictionSAD; }

protected:

    Void finishCU(TComDataCU* cu, UInt absPartIdx, UInt depth);
    Void xCompressCU(TComDataCU*& outBestCU, TComDataCU*& outTempCU, TComDataCU* parentCU,  UInt depth, UInt partUnitIdx,
                     PartSize parentSize = SIZE_NONE);
    Void xCompressIntraCU(TComDataCU*& outBestCU, TComDataCU*& outTempCU, TComDataCU* parentCU,  UInt depth,
                          PartSize parentSize = SIZE_NONE);
    Void xCompressInterCU(TComDataCU*& outBestCU, TComDataCU*& outTempCU, TComDataCU*& cu, UInt depth, UInt partitionIndex);
    Void xEncodeCU(TComDataCU* cu, UInt absPartIdx, UInt depth);
    Int  xComputeQP(TComDataCU* cu, UInt depth);
    Void xCheckBestMode(TComDataCU*& outBestCU, TComDataCU*& outTempCU, UInt depth);

    Void xCheckRDCostMerge2Nx2N(TComDataCU*& outBestCU, TComDataCU*& outTempCU, Bool *earlyDetectionSkipMode,
                                TComYuv*& outBestPredYuv, TComYuv*& rpcYuvReconBest);
    Void xComputeCostIntraInInter(TComDataCU*& outTempCU, PartSize partSize);
    Void xCheckRDCostInter(TComDataCU*& outBestCU, TComDataCU*& outTempCU, PartSize partSize, Bool bUseMRG = false);
    Void xComputeCostInter(TComDataCU*& outTempCU, PartSize partSize, UInt Index, Bool bUseMRG = false);
    Void xEncodeIntraInInter(TComDataCU*& cu, TComYuv* fencYuv, TComYuv* predYuv, TShortYUV*& outResiYuv, TComYuv*& outReconYuv);
    Void xCheckRDCostIntra(TComDataCU*& outBestCU, TComDataCU*& outTempCU, PartSize partSize);
    Void xCheckRDCostIntraInInter(TComDataCU*& outBestCU, TComDataCU*& outTempCU, PartSize partSize);
    Void xCheckDQP(TComDataCU* cu);

    Void xCheckIntraPCM(TComDataCU*& outBestCU, TComDataCU*& outTempCU);
    Void xCopyAMVPInfo(AMVPInfo* src, AMVPInfo* dst);
    Void xCopyYuv2Pic(TComPic* outPic, UInt cuAddr, UInt absPartIdx, UInt depth, UInt uiSrcDepth, TComDataCU* cu,
                      UInt uiLPelX, UInt uiTPelY);
    Void xCopyYuv2Tmp(UInt uhPartUnitIdx, UInt depth);
    Void xCopyYuv2Best(UInt partUnitIdx, UInt uiNextDepth);

    Bool getdQPFlag()        { return m_bEncodeDQP; }

    Void setdQPFlag(Bool b)  { m_bEncodeDQP = b; }

    // Adaptive reconstruction level (ARL) statistics collection functions
    Void xLcuCollectARLStats(TComDataCU* cu);
    Int  xTuCollectARLStats(TCoeff* coeff, Int* arlCoeff, Int numCoeffInCU, Double* coeffSum, UInt* numSamples);

    Void deriveTestModeAMP(TComDataCU*& outBestCU, PartSize parentSize, Bool &bTestAMP_Hor, Bool &bTestAMP_Ver,
                           Bool &bTestMergeAMP_Hor, Bool &bTestMergeAMP_Ver);

    Void xFillPCMBuffer(TComDataCU*& outCU, TComYuv* origYuv);
};

//! \}

#endif // __TENCMB__
