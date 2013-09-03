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

/** \file     TEncEntropy.h
    \brief    entropy encoder class (header)
*/

#ifndef __TENCENTROPY__
#define __TENCENTROPY__

#include "TLibCommon/TComSlice.h"
#include "TLibCommon/TComDataCU.h"
#include "TLibCommon/TComBitStream.h"
#include "TLibCommon/ContextModel.h"
#include "TLibCommon/TComPic.h"
#include "TLibCommon/TComTrQuant.h"
#include "TLibCommon/TComSampleAdaptiveOffset.h"

namespace x265 {
// private namespace

class TEncSbac;
class TEncCavlc;
class SEI;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// entropy encoder pure class
class TEncEntropyIf
{
public:

    virtual void  resetEntropy() = 0;
    virtual void  determineCabacInitIdx() = 0;
    virtual void  setBitstream(TComBitIf* p) = 0;
    virtual void  setSlice(TComSlice* p) = 0;
    virtual void  resetBits() = 0;
    virtual void  resetCoeffCost() = 0;
    virtual UInt  getNumberOfWrittenBits() = 0;
    virtual UInt  getCoeffCost() = 0;

    virtual void  codeVPS(TComVPS* pcVPS) = 0;
    virtual void  codeSPS(TComSPS* pcSPS) = 0;
    virtual void  codePPS(TComPPS* pcPPS) = 0;
    virtual void  codeSliceHeader(TComSlice* slice) = 0;

    virtual void  codeTilesWPPEntryPoint(TComSlice* pSlice) = 0;
    virtual void  codeTerminatingBit(UInt uilsLast) = 0;
    virtual void  codeSliceFinish() = 0;
    virtual void codeMVPIdx(TComDataCU* cu, UInt absPartIdx, RefPicList eRefList) = 0;
    virtual void codeScalingList(TComScalingList* scalingList) = 0;

public:

    virtual void codeCUTransquantBypassFlag(TComDataCU* cu, UInt absPartIdx) = 0;
    virtual void codeSkipFlag(TComDataCU* cu, UInt absPartIdx) = 0;
    virtual void codeMergeFlag(TComDataCU* cu, UInt absPartIdx) = 0;
    virtual void codeMergeIndex(TComDataCU* cu, UInt absPartIdx) = 0;
    virtual void codeSplitFlag(TComDataCU* cu, UInt absPartIdx, UInt depth) = 0;

    virtual void codePartSize(TComDataCU* cu, UInt absPartIdx, UInt depth) = 0;
    virtual void codePredMode(TComDataCU* cu, UInt absPartIdx) = 0;

    virtual void codeIPCMInfo(TComDataCU* cu, UInt absPartIdx) = 0;

    virtual void codeTransformSubdivFlag(UInt uiSymbol, UInt uiCtx) = 0;
    virtual void codeQtCbf(TComDataCU* cu, UInt absPartIdx, TextType ttype, UInt trDepth) = 0;
    virtual void codeQtRootCbf(TComDataCU* cu, UInt absPartIdx) = 0;
    virtual void codeQtCbfZero(TComDataCU* cu, TextType ttype, UInt trDepth) = 0;
    virtual void codeQtRootCbfZero(TComDataCU* cu) = 0;
    virtual void codeIntraDirLumaAng(TComDataCU* cu, UInt absPartIdx, Bool isMultiplePU) = 0;

    virtual void codeIntraDirChroma(TComDataCU* cu, UInt absPartIdx) = 0;
    virtual void codeInterDir(TComDataCU* cu, UInt absPartIdx) = 0;
    virtual void codeRefFrmIdx(TComDataCU* cu, UInt absPartIdx, RefPicList eRefList) = 0;
    virtual void codeMvd(TComDataCU* cu, UInt absPartIdx, RefPicList eRefList) = 0;
    virtual void codeDeltaQP(TComDataCU* cu, UInt absPartIdx) = 0;
    virtual void codeCoeffNxN(TComDataCU* cu, TCoeff* pcCoef, UInt absPartIdx, UInt width, UInt height, UInt depth, TextType eTType) = 0;
    virtual void codeTransformSkipFlags(TComDataCU* cu, UInt absPartIdx, UInt width, UInt height, TextType eTType) = 0;
    virtual void codeSAOSign(UInt code) = 0;
    virtual void codeSaoMaxUvlc(UInt code, UInt maxSymbol) = 0;
    virtual void codeSaoMerge(UInt uiCode) = 0;
    virtual void codeSaoTypeIdx(UInt uiCode) = 0;
    virtual void codeSaoUflc(UInt uiLength, UInt   uiCode) = 0;
    virtual void estBit(estBitsSbacStruct* pcEstBitsSbac, Int width, Int height, TextType eTType) = 0;

    virtual void updateContextTables(SliceType eSliceType, Int iQp, Bool bExecuteFinish)   = 0;
    virtual void updateContextTables(SliceType eSliceType, Int iQp)   = 0;

    virtual void codeDFFlag(UInt uiCode, const Char *pSymbolName) = 0;
    virtual void codeDFSvlc(Int iCode, const Char *pSymbolName)   = 0;

    virtual ~TEncEntropyIf() {}
};

/// entropy encoder class
class TEncEntropy
{
private:

    UInt    m_uiBakAbsPartIdx;
    UInt    m_uiBakChromaOffset;
    UInt    m_bakAbsPartIdxCU;

public:

    void    setEntropyCoder(TEncEntropyIf* e, TComSlice* slice);
    void    setBitstream(TComBitIf* p) { m_pcEntropyCoderIf->setBitstream(p);  }

    void    resetBits() { m_pcEntropyCoderIf->resetBits();      }

    void    resetCoeffCost() { m_pcEntropyCoderIf->resetCoeffCost(); }

    UInt    getNumberOfWrittenBits() { return m_pcEntropyCoderIf->getNumberOfWrittenBits(); }

    UInt    getCoeffCost() { return m_pcEntropyCoderIf->getCoeffCost(); }

    void    resetEntropy() { m_pcEntropyCoderIf->resetEntropy();  }

    void    determineCabacInitIdx() { m_pcEntropyCoderIf->determineCabacInitIdx(); }

    void    encodeSliceHeader(TComSlice* slice);
    void    encodeTilesWPPEntryPoint(TComSlice* pSlice);
    void    encodeTerminatingBit(UInt uiIsLast);
    void    encodeSliceFinish();
    TEncEntropyIf*      m_pcEntropyCoderIf;

public:

    void encodeVPS(TComVPS* pcVPS);
    // SPS
    void encodeSPS(TComSPS* pcSPS);
    void encodePPS(TComPPS* pcPPS);
    void encodeSplitFlag(TComDataCU* cu, UInt absPartIdx, UInt depth, Bool bRD = false);
    void encodeCUTransquantBypassFlag(TComDataCU* cu, UInt absPartIdx, Bool bRD = false);
    void encodeSkipFlag(TComDataCU* cu, UInt absPartIdx, Bool bRD = false);
    void encodePUWise(TComDataCU* cu, UInt absPartIdx, Bool bRD = false);
    void encodeInterDirPU(TComDataCU* pcSubCU, UInt absPartIdx);
    void encodeRefFrmIdxPU(TComDataCU* pcSubCU, UInt absPartIdx, RefPicList eRefList);
    void encodeMvdPU(TComDataCU* pcSubCU, UInt absPartIdx, RefPicList eRefList);
    void encodeMVPIdxPU(TComDataCU* pcSubCU, UInt absPartIdx, RefPicList eRefList);
    void encodeMergeFlag(TComDataCU* cu, UInt absPartIdx);
    void encodeMergeIndex(TComDataCU* cu, UInt absPartIdx, Bool bRD = false);
    void encodePredMode(TComDataCU* cu, UInt absPartIdx, Bool bRD = false);
    void encodePartSize(TComDataCU* cu, UInt absPartIdx, UInt depth, Bool bRD = false);
    void encodeIPCMInfo(TComDataCU* cu, UInt absPartIdx, Bool bRD = false);
    void encodePredInfo(TComDataCU* cu, UInt absPartIdx, Bool bRD = false);
    void encodeIntraDirModeLuma(TComDataCU* cu, UInt absPartIdx, Bool isMultiplePU = false);

    void encodeIntraDirModeChroma(TComDataCU* cu, UInt absPartIdx, Bool bRD = false);

    void encodeTransformSubdivFlag(UInt uiSymbol, UInt uiCtx);
    void encodeQtCbf(TComDataCU* cu, UInt absPartIdx, TextType ttype, UInt trDepth);
    void encodeQtCbfZero(TComDataCU* cu, TextType ttype, UInt trDepth);
    void encodeQtRootCbfZero(TComDataCU* cu);
    void encodeQtRootCbf(TComDataCU* cu, UInt absPartIdx);
    void encodeQP(TComDataCU* cu, UInt absPartIdx, Bool bRD = false);
    void updateContextTables(SliceType eSliceType, Int iQp, Bool bExecuteFinish)   { m_pcEntropyCoderIf->updateContextTables(eSliceType, iQp, bExecuteFinish);     }

    void updateContextTables(SliceType eSliceType, Int iQp)                        { m_pcEntropyCoderIf->updateContextTables(eSliceType, iQp, true);               }

    void encodeScalingList(TComScalingList* scalingList);

private:

    void xEncodeTransform(TComDataCU* cu, UInt offsetLumaOffset, UInt offsetChroma, UInt absPartIdx, UInt depth, UInt width, UInt height, UInt uiTrIdx, Bool& bCodeDQP);

public:

    void encodeCoeff(TComDataCU* cu, UInt absPartIdx, UInt depth, UInt width, UInt height, Bool& bCodeDQP);

    void encodeCoeffNxN(TComDataCU* cu, TCoeff* pcCoeff, UInt absPartIdx, UInt uiTrWidth, UInt uiTrHeight, UInt depth, TextType ttype);

    void estimateBit(estBitsSbacStruct* pcEstBitsSbac, Int width, Int height, TextType eTType);
    void    encodeSaoOffset(SaoLcuParam* saoLcuParam, UInt compIdx);
    void    encodeSaoUnitInterleaving(Int compIdx, Bool saoFlag, Int rx, Int ry, SaoLcuParam* saoLcuParam, Int cuAddrInSlice, Int cuAddrUpInSlice, Int allowMergeLeft, Int allowMergeUp);
    static Int countNonZeroCoeffs(TCoeff* pcCoef, UInt uiSize);
}; // END CLASS DEFINITION TEncEntropy
}
//! \}

#endif // __TENCENTROPY__
