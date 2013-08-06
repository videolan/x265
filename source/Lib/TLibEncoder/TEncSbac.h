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

    Void  init(TEncBinIf* p)          { m_pcBinIf = p; }

    Void  uninit()                    { m_pcBinIf = 0; }

    //  Virtual list
    Void  resetEntropy();
    Void  determineCabacInitIdx();
    Void  setBitstream(TComBitIf* p)  { m_pcBitIf = p; m_pcBinIf->init(p); }

    Void  setSlice(TComSlice* p)      { m_pcSlice = p; }

    // SBAC RD
    Void  resetCoeffCost()            { m_uiCoeffCost = 0;  }

    UInt  getCoeffCost()              { return m_uiCoeffCost;  }

    Void  load(TEncSbac* pScr);
    Void  loadIntraDirModeLuma(TEncSbac* pScr);
    Void  store(TEncSbac* pDest);
    Void  loadContexts(TEncSbac* pScr);
    Void  resetBits()                { m_pcBinIf->resetBits(); m_pcBitIf->resetBits(); }

    UInt  getNumberOfWrittenBits()   { return m_pcBinIf->getNumWrittenBits(); }

    //--SBAC RD

    Void  codeVPS(TComVPS* pcVPS);
    Void  codeSPS(TComSPS* pcSPS);
    Void  codePPS(TComPPS* pcPPS);
    Void  codeSliceHeader(TComSlice* slice);
    Void  codeTilesWPPEntryPoint(TComSlice* pSlice);
    Void  codeTerminatingBit(UInt uilsLast);
    Void  codeSliceFinish();
    Void  codeSaoMaxUvlc(UInt code, UInt maxSymbol);
    Void  codeSaoMerge(UInt uiCode);
    Void  codeSaoTypeIdx(UInt uiCode);
    Void  codeSaoUflc(UInt uiLength, UInt  uiCode);
    Void  codeSAOSign(UInt uiCode);         //<! code SAO offset sign
    Void  codeScalingList(TComScalingList* /*scalingList*/) { assert(0); }

private:

    Void  xWriteUnarySymbol(UInt uiSymbol, ContextModel* pcSCModel, Int offset);
    Void  xWriteUnaryMaxSymbol(UInt uiSymbol, ContextModel* pcSCModel, Int offset, UInt uiMaxSymbol);
    Void  xWriteEpExGolomb(UInt uiSymbol, UInt uiCount);
    Void  xWriteCoefRemainExGolomb(UInt symbol, UInt &rParam);

    Void  xCopyFrom(TEncSbac* src);
    Void  xCopyContextsFrom(TEncSbac* src);

    Void codeDFFlag(UInt /*uiCode*/, const Char* /*pSymbolName*/)       { printf("Not supported in codeDFFlag()\n"); assert(0); exit(1); }

    Void codeDFSvlc(Int /*iCode*/, const Char* /*pSymbolName*/)         { printf("Not supported in codeDFSvlc()\n"); assert(0); exit(1); }

protected:

    TComBitIf*    m_pcBitIf;
    TComSlice*    m_pcSlice;
    TEncBinIf*    m_pcBinIf;
    //SBAC RD
    UInt          m_uiCoeffCost;

    //--Adaptive loop filter

public:

    Void codeCUTransquantBypassFlag(TComDataCU* cu, UInt absPartIdx);
    Void codeSkipFlag(TComDataCU* cu, UInt absPartIdx);
    Void codeMergeFlag(TComDataCU* cu, UInt absPartIdx);
    Void codeMergeIndex(TComDataCU* cu, UInt absPartIdx);
    Void codeSplitFlag(TComDataCU* cu, UInt absPartIdx, UInt depth);
    Void codeMVPIdx(TComDataCU* cu, UInt absPartIdx, RefPicList eRefList);

    Void codePartSize(TComDataCU* cu, UInt absPartIdx, UInt depth);
    Void codePredMode(TComDataCU* cu, UInt absPartIdx);
    Void codeIPCMInfo(TComDataCU* cu, UInt absPartIdx);
    Void codeTransformSubdivFlag(UInt uiSymbol, UInt uiCtx);
    Void codeQtCbf(TComDataCU* cu, UInt absPartIdx, TextType ttype, UInt trDepth);
    Void codeQtRootCbf(TComDataCU* cu, UInt absPartIdx);
    Void codeQtCbfZero(TComDataCU* cu, TextType ttype, UInt trDepth);
    Void codeQtRootCbfZero(TComDataCU* cu);
    Void codeIntraDirLumaAng(TComDataCU* cu, UInt absPartIdx, Bool isMultiple);

    Void codeIntraDirChroma(TComDataCU* cu, UInt absPartIdx);
    Void codeInterDir(TComDataCU* cu, UInt absPartIdx);
    Void codeRefFrmIdx(TComDataCU* cu, UInt absPartIdx, RefPicList eRefList);
    Void codeMvd(TComDataCU* cu, UInt absPartIdx, RefPicList eRefList);

    Void codeDeltaQP(TComDataCU* cu, UInt absPartIdx);

    Void codeLastSignificantXY(UInt posx, UInt posy, Int width, Int height, TextType eTType, UInt uiScanIdx);
    Void codeCoeffNxN(TComDataCU* cu, TCoeff* pcCoef, UInt absPartIdx, UInt width, UInt height, UInt depth, TextType eTType);
    void codeTransformSkipFlags(TComDataCU* cu, UInt absPartIdx, UInt width, UInt height, TextType eTType);

    // -------------------------------------------------------------------------------------------------------------------
    // for RD-optimizatioon
    // -------------------------------------------------------------------------------------------------------------------

    Void estBit(estBitsSbacStruct* pcEstBitsSbac, Int width, Int height, TextType eTType);
    Void estCBFBit(estBitsSbacStruct* pcEstBitsSbac);
    Void estSignificantCoeffGroupMapBit(estBitsSbacStruct* pcEstBitsSbac, TextType eTType);
    Void estSignificantMapBit(estBitsSbacStruct* pcEstBitsSbac, Int width, Int height, TextType eTType);
    Void estSignificantCoefficientsBit(estBitsSbacStruct* pcEstBitsSbac, TextType eTType);

    Void updateContextTables(SliceType eSliceType, Int iQp, Bool bExecuteFinish = true);
    Void updateContextTables(SliceType eSliceType, Int iQp) { this->updateContextTables(eSliceType, iQp, true); }

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

//! \}

#endif // !defined(__TENCSBAC__)
