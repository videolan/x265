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

/** \file     TEncSbac.h
    \brief    Context-adaptive entropy encoder class (header)
*/

#ifndef __TENCSBAC__
#define __TENCSBAC__

#include "TLibCommon/TComBitStream.h"
#include "TLibCommon/ContextTables.h"
#include "TLibCommon/ContextModel.h"
#include "TLibCommon/ContextModel3DBuffer.h"
#include "TEncEntropy.h"
#include "TEncBinCoder.h"
#include "TEncBinCoderCABAC.h"
#include "TEncBinCoderCABACCounter.h"

namespace x265 {
// private namespace

class TEncTop;

//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// SBAC encoder class
class TEncSbac : public TEncEntropyIf
{
public:

    TEncSbac();
    virtual ~TEncSbac();

    void  init(TEncBinIf* p)          { m_pcBinIf = p; }

    void  uninit()                    { m_pcBinIf = 0; }

    //  Virtual list
    void  resetEntropy();
    void  determineCabacInitIdx();
    void  setBitstream(TComBitIf* p)  { m_pcBitIf = p; m_pcBinIf->init(p); }

    void  setSlice(TComSlice* p)      { m_pcSlice = p; }

    // SBAC RD
    void  resetCoeffCost()            { m_uiCoeffCost = 0;  }

    UInt  getCoeffCost()              { return m_uiCoeffCost;  }

    void  load(TEncSbac* pScr);
    void  loadIntraDirModeLuma(TEncSbac* pScr);
    void  store(TEncSbac* pDest);
    void  loadContexts(TEncSbac* pScr);
    void  resetBits()                { m_pcBinIf->resetBits(); m_pcBitIf->resetBits(); }

    UInt  getNumberOfWrittenBits()   { return m_pcBinIf->getNumWrittenBits(); }

    //--SBAC RD

    void  codeVPS(TComVPS* pcVPS);
    void  codeSPS(TComSPS* pcSPS);
    void  codePPS(TComPPS* pcPPS);
    void  codeSliceHeader(TComSlice* slice);
    void  codeTilesWPPEntryPoint(TComSlice* pSlice);
    void  codeTerminatingBit(UInt uilsLast);
    void  codeSliceFinish();
    void  codeSaoMaxUvlc(UInt code, UInt maxSymbol);
    void  codeSaoMerge(UInt uiCode);
    void  codeSaoTypeIdx(UInt uiCode);
    void  codeSaoUflc(UInt uiLength, UInt  uiCode);
    void  codeSAOSign(UInt uiCode);         //<! code SAO offset sign
    void  codeScalingList(TComScalingList* /*scalingList*/) { assert(0); }

private:

    void  xWriteUnarySymbol(UInt uiSymbol, ContextModel* pcSCModel, Int offset);
    void  xWriteUnaryMaxSymbol(UInt uiSymbol, ContextModel* pcSCModel, Int offset, UInt uiMaxSymbol);
    void  xWriteEpExGolomb(UInt uiSymbol, UInt uiCount);
    void  xWriteCoefRemainExGolomb(UInt symbol, UInt &rParam);

    void  xCopyFrom(TEncSbac* src);
    void  xCopyContextsFrom(TEncSbac* src);

    void codeDFFlag(UInt /*uiCode*/, const char* /*pSymbolName*/)       { printf("Not supported in codeDFFlag()\n"); assert(0); exit(1); }

    void codeDFSvlc(Int /*iCode*/, const char* /*pSymbolName*/)         { printf("Not supported in codeDFSvlc()\n"); assert(0); exit(1); }

protected:

    TComBitIf*    m_pcBitIf;
    TComSlice*    m_pcSlice;
    TEncBinIf*    m_pcBinIf;
    //SBAC RD
    UInt          m_uiCoeffCost;

    //--Adaptive loop filter

public:

    void codeCUTransquantBypassFlag(TComDataCU* cu, UInt absPartIdx);
    void codeSkipFlag(TComDataCU* cu, UInt absPartIdx);
    void codeMergeFlag(TComDataCU* cu, UInt absPartIdx);
    void codeMergeIndex(TComDataCU* cu, UInt absPartIdx);
    void codeSplitFlag(TComDataCU* cu, UInt absPartIdx, UInt depth);
    void codeMVPIdx(TComDataCU* cu, UInt absPartIdx, RefPicList eRefList);

    void codePartSize(TComDataCU* cu, UInt absPartIdx, UInt depth);
    void codePredMode(TComDataCU* cu, UInt absPartIdx);
    void codeIPCMInfo(TComDataCU* cu, UInt absPartIdx);
    void codeTransformSubdivFlag(UInt uiSymbol, UInt uiCtx);
    void codeQtCbf(TComDataCU* cu, UInt absPartIdx, TextType ttype, UInt trDepth);
    void codeQtRootCbf(TComDataCU* cu, UInt absPartIdx);
    void codeQtCbfZero(TComDataCU* cu, TextType ttype, UInt trDepth);
    void codeQtRootCbfZero(TComDataCU* cu);
    void codeIntraDirLumaAng(TComDataCU* cu, UInt absPartIdx, Bool isMultiple);

    void codeIntraDirChroma(TComDataCU* cu, UInt absPartIdx);
    void codeInterDir(TComDataCU* cu, UInt absPartIdx);
    void codeRefFrmIdx(TComDataCU* cu, UInt absPartIdx, RefPicList eRefList);
    void codeMvd(TComDataCU* cu, UInt absPartIdx, RefPicList eRefList);

    void codeDeltaQP(TComDataCU* cu, UInt absPartIdx);

    void codeLastSignificantXY(UInt posx, UInt posy, Int width, Int height, TextType eTType, UInt uiScanIdx);
    void codeCoeffNxN(TComDataCU* cu, TCoeff* pcCoef, UInt absPartIdx, UInt width, UInt height, UInt depth, TextType eTType);
    void codeTransformSkipFlags(TComDataCU* cu, UInt absPartIdx, UInt width, UInt height, TextType eTType);

    // -------------------------------------------------------------------------------------------------------------------
    // for RD-optimizatioon
    // -------------------------------------------------------------------------------------------------------------------

    void estBit(estBitsSbacStruct* pcEstBitsSbac, Int width, Int height, TextType eTType);
    void estCBFBit(estBitsSbacStruct* pcEstBitsSbac);
    void estSignificantCoeffGroupMapBit(estBitsSbacStruct* pcEstBitsSbac, TextType eTType);
    void estSignificantMapBit(estBitsSbacStruct* pcEstBitsSbac, Int width, Int height, TextType eTType);
    void estSignificantCoefficientsBit(estBitsSbacStruct* pcEstBitsSbac, TextType eTType);

    void updateContextTables(SliceType eSliceType, Int iQp, Bool bExecuteFinish = true);
    void updateContextTables(SliceType eSliceType, Int iQp) { this->updateContextTables(eSliceType, iQp, true); }

    TEncBinIf* getEncBinIf()  { return m_pcBinIf; }

private:

    UInt                 m_uiLastQp;

    ContextModel         m_contextModels[MAX_NUM_CTX_MOD];
    Int                  m_numContextModels;
    ContextModel3DBuffer m_cCUSplitFlagSCModel;
    ContextModel3DBuffer m_cCUSkipFlagSCModel;
    ContextModel3DBuffer m_cCUMergeFlagExtSCModel;
    ContextModel3DBuffer m_cCUMergeIdxExtSCModel;
    ContextModel3DBuffer m_cCUPartSizeSCModel;
    ContextModel3DBuffer m_cCUPredModeSCModel;
    ContextModel3DBuffer m_cCUIntraPredSCModel;
    ContextModel3DBuffer m_cCUChromaPredSCModel;
    ContextModel3DBuffer m_cCUDeltaQpSCModel;
    ContextModel3DBuffer m_cCUInterDirSCModel;
    ContextModel3DBuffer m_cCURefPicSCModel;
    ContextModel3DBuffer m_cCUMvdSCModel;
    ContextModel3DBuffer m_cCUQtCbfSCModel;
    ContextModel3DBuffer m_cCUTransSubdivFlagSCModel;
    ContextModel3DBuffer m_cCUQtRootCbfSCModel;

    ContextModel3DBuffer m_cCUSigCoeffGroupSCModel;
    ContextModel3DBuffer m_cCUSigSCModel;
    ContextModel3DBuffer m_cCuCtxLastX;
    ContextModel3DBuffer m_cCuCtxLastY;
    ContextModel3DBuffer m_cCUOneSCModel;
    ContextModel3DBuffer m_cCUAbsSCModel;

    ContextModel3DBuffer m_cMVPIdxSCModel;

    ContextModel3DBuffer m_cCUAMPSCModel;
    ContextModel3DBuffer m_cSaoMergeSCModel;
    ContextModel3DBuffer m_cSaoTypeIdxSCModel;
    ContextModel3DBuffer m_cTransformSkipSCModel;
    ContextModel3DBuffer m_CUTransquantBypassFlagSCModel;
};
}
//! \}

#endif // !defined(__TENCSBAC__)
