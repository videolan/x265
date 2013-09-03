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

//! \ingroup TLibEncoder
//! \{

namespace x265 {
// private namespace

class TEncTop;
class TEncSbac;
class TEncCavlc;

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
    TComDataCU** m_interCU_NxN[4];
    TComDataCU** m_bestCU;      ///< Best CUs at each depth
    TComDataCU** m_tempCU;      ///< Temporary CUs at each depth

    TComYuv**    m_bestPredYuv; ///< Best Prediction Yuv for each depth
    TShortYUV**  m_bestResiYuv; ///< Best Residual Yuv for each depth
    TComYuv**    m_bestRecoYuv; ///< Best Reconstruction Yuv for each depth
    TComYuv**    m_bestPredYuvNxN[4];

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

    UInt         m_LCUPredictionSAD;
    int          m_addSADDepth;
    int          m_temporalSAD;
    UChar        m_totalDepth;

    bool         m_bEncodeDQP;
    bool         m_abortFlag; // aborts recursion when the child CU costs more than parent CU

public:

    TEncCu();

    void init(TEncTop* top);
    void create(UChar totalDepth, UInt maxWidth);
    void destroy();
    void compressCU(TComDataCU* cu);
    void encodeCU(TComDataCU* cu);

    void setRDSbacCoder(TEncSbac*** rdSbacCoder) { m_rdSbacCoders = rdSbacCoder; }

    void setEntropyCoder(TEncEntropy* entropyCoder) { m_entropyCoder = entropyCoder; }

    void setPredSearch(TEncSearch* predSearch) { m_search = predSearch; }

    void setRDGoOnSbacCoder(TEncSbac* rdGoOnSbacCoder) { m_rdGoOnSbacCoder = rdGoOnSbacCoder; }

    void setTrQuant(TComTrQuant* trQuant) { m_trQuant = trQuant; }

    void setRdCost(TComRdCost* rdCost) { m_rdCost = rdCost; }

    void setBitCounter(TComBitCounter* pcBitCounter) { m_bitCounter = pcBitCounter; }

    UInt getLCUPredictionSAD() { return m_LCUPredictionSAD; }

protected:

    void finishCU(TComDataCU* cu, UInt absPartIdx, UInt depth);
    void xCompressCU(TComDataCU*& outBestCU, TComDataCU*& outTempCU, UInt depth, PartSize parentSize = SIZE_NONE);
    void xCompressIntraCU(TComDataCU*& outBestCU, TComDataCU*& outTempCU, UInt depth);
    void xCompressInterCU(TComDataCU*& outBestCU, TComDataCU*& outTempCU, TComDataCU*& cu, UInt depth, UInt partitionIndex);
    void xEncodeCU(TComDataCU* cu, UInt absPartIdx, UInt depth);
    int  xComputeQP(TComDataCU* cu);
    void xCheckBestMode(TComDataCU*& outBestCU, TComDataCU*& outTempCU, UInt depth);

    void xCheckRDCostMerge2Nx2N(TComDataCU*& outBestCU, TComDataCU*& outTempCU, bool *earlyDetectionSkipMode,
                                TComYuv*& outBestPredYuv, TComYuv*& rpcYuvReconBest);
    void xComputeCostIntraInInter(TComDataCU*& outTempCU, PartSize partSize);
    void xCheckRDCostInter(TComDataCU*& outBestCU, TComDataCU*& outTempCU, PartSize partSize, bool bUseMRG = false);
    void xComputeCostInter(TComDataCU* outTempCU, TComYuv* outPredYUV, PartSize partSize, bool bUseMRG = false);
    void xEncodeIntraInInter(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, TShortYUV* outResiYuv, TComYuv* outReconYuv);
    void xCheckRDCostIntra(TComDataCU*& outBestCU, TComDataCU*& outTempCU, PartSize partSize);
    void xCheckRDCostIntraInInter(TComDataCU*& outBestCU, TComDataCU*& outTempCU, PartSize partSize);
    void xCheckDQP(TComDataCU* cu);

    void xCheckIntraPCM(TComDataCU*& outBestCU, TComDataCU*& outTempCU);
    void xCopyAMVPInfo(AMVPInfo* src, AMVPInfo* dst);
    void xCopyYuv2Pic(TComPic* outPic, UInt cuAddr, UInt absPartIdx, UInt depth, UInt uiSrcDepth, TComDataCU* cu,
                      UInt lpelx, UInt tpely);
    void xCopyYuv2Tmp(UInt uhPartUnitIdx, UInt depth);
    void xCopyYuv2Best(UInt partUnitIdx, UInt uiNextDepth);

    bool getdQPFlag()        { return m_bEncodeDQP; }

    void setdQPFlag(bool b)  { m_bEncodeDQP = b; }

    void deriveTestModeAMP(TComDataCU* bestCU, PartSize parentSize, bool &bTestAMP_Hor, bool &bTestAMP_Ver,
                           bool &bTestMergeAMP_Hor, bool &bTestMergeAMP_Ver);

    void xFillPCMBuffer(TComDataCU* outCU, TComYuv* origYuv);
};
}
//! \}

#endif // __TENCMB__
