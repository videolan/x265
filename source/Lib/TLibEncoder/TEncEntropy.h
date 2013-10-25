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

#ifndef X265_TENCENTROPY_H
#define X265_TENCENTROPY_H

#include "TLibCommon/TComSlice.h"
#include "TLibCommon/TComDataCU.h"
#include "TLibCommon/TComBitStream.h"
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

    virtual void resetEntropy() = 0;
    virtual void determineCabacInitIdx() = 0;
    virtual void setBitstream(TComBitIf* p) = 0;
    virtual void setSlice(TComSlice* p) = 0;
    virtual void resetBits() = 0;
    virtual void resetCoeffCost() = 0;
    virtual UInt getNumberOfWrittenBits() = 0;
    virtual UInt getCoeffCost() = 0;

    virtual void codeVPS(TComVPS* vps) = 0;
    virtual void codeSPS(TComSPS* sps) = 0;
    virtual void codePPS(TComPPS* pps) = 0;
    virtual void codeSliceHeader(TComSlice* slice) = 0;

    virtual void codeTilesWPPEntryPoint(TComSlice* slice) = 0;
    virtual void codeTerminatingBit(UInt isLast) = 0;
    virtual void codeSliceFinish() = 0;
    virtual void codeMVPIdx(TComDataCU* cu, UInt absPartIdx, RefPicList list) = 0;
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

    virtual void codeTransformSubdivFlag(UInt symbol, UInt ctx) = 0;
    virtual void codeQtCbf(TComDataCU* cu, UInt absPartIdx, TextType ttype, UInt trDepth) = 0;
    virtual void codeQtRootCbf(TComDataCU* cu, UInt absPartIdx) = 0;
    virtual void codeQtCbfZero(TComDataCU* cu, TextType ttype, UInt trDepth) = 0;
    virtual void codeQtRootCbfZero(TComDataCU* cu) = 0;
    virtual void codeIntraDirLumaAng(TComDataCU* cu, UInt absPartIdx, bool isMultiplePU) = 0;

    virtual void codeIntraDirChroma(TComDataCU* cu, UInt absPartIdx) = 0;
    virtual void codeInterDir(TComDataCU* cu, UInt absPartIdx) = 0;
    virtual void codeRefFrmIdx(TComDataCU* cu, UInt absPartIdx, RefPicList eRefList) = 0;
    virtual void codeMvd(TComDataCU* cu, UInt absPartIdx, RefPicList eRefList) = 0;
    virtual void codeDeltaQP(TComDataCU* cu, UInt absPartIdx) = 0;
    virtual void codeCoeffNxN(TComDataCU* cu, TCoeff* pcCoef, UInt absPartIdx, UInt width, UInt height, UInt depth, TextType ttype) = 0;
    virtual void codeTransformSkipFlags(TComDataCU* cu, UInt absPartIdx, UInt width, UInt height, TextType ttype) = 0;
    virtual void codeSAOSign(UInt code) = 0;
    virtual void codeSaoMaxUvlc(UInt code, UInt maxSymbol) = 0;
    virtual void codeSaoMerge(UInt code) = 0;
    virtual void codeSaoTypeIdx(UInt code) = 0;
    virtual void codeSaoUflc(UInt length, UInt code) = 0;
    virtual void estBit(estBitsSbacStruct* estBitsSbac, int width, int height, TextType ttype) = 0;

    virtual void codeDFFlag(UInt code, const char *symbolName) = 0;
    virtual void codeDFSvlc(int code, const char *symbolName)   = 0;

    virtual ~TEncEntropyIf() {}
};

/// entropy encoder class
class TEncEntropy
{
private:

    UInt    m_bakAbsPartIdx;
    UInt    m_bakChromaOffset;
    UInt    m_bakAbsPartIdxCU;

public:

    void    setEntropyCoder(TEncEntropyIf* e, TComSlice* slice);
    void    setBitstream(TComBitIf* p) { m_entropyCoderIf->setBitstream(p);  }

    void    resetBits() { m_entropyCoderIf->resetBits();      }

    void    resetCoeffCost() { m_entropyCoderIf->resetCoeffCost(); }

    UInt    getNumberOfWrittenBits() { return m_entropyCoderIf->getNumberOfWrittenBits(); }

    UInt    getCoeffCost() { return m_entropyCoderIf->getCoeffCost(); }

    void    resetEntropy() { m_entropyCoderIf->resetEntropy();  }

    void    determineCabacInitIdx() { m_entropyCoderIf->determineCabacInitIdx(); }

    void    encodeSliceHeader(TComSlice* slice);
    void    encodeTilesWPPEntryPoint(TComSlice* pSlice);
    void    encodeTerminatingBit(UInt uiIsLast);
    void    encodeSliceFinish();
    TEncEntropyIf*      m_entropyCoderIf;

public:

    void encodeVPS(TComVPS* vps);
    // SPS
    void encodeSPS(TComSPS* sps);
    void encodePPS(TComPPS* pps);
    void encodeSplitFlag(TComDataCU* cu, UInt absPartIdx, UInt depth, bool bRD = false);
    void encodeCUTransquantBypassFlag(TComDataCU* cu, UInt absPartIdx, bool bRD = false);
    void encodeSkipFlag(TComDataCU* cu, UInt absPartIdx, bool bRD = false);
    void encodePUWise(TComDataCU* cu, UInt absPartIdx, bool bRD = false);
    void encodeInterDirPU(TComDataCU* pcSubCU, UInt absPartIdx);
    void encodeRefFrmIdxPU(TComDataCU* pcSubCU, UInt absPartIdx, RefPicList eRefList);
    void encodeMvdPU(TComDataCU* pcSubCU, UInt absPartIdx, RefPicList eRefList);
    void encodeMVPIdxPU(TComDataCU* pcSubCU, UInt absPartIdx, RefPicList eRefList);
    void encodeMergeFlag(TComDataCU* cu, UInt absPartIdx);
    void encodeMergeIndex(TComDataCU* cu, UInt absPartIdx, bool bRD = false);
    void encodePredMode(TComDataCU* cu, UInt absPartIdx, bool bRD = false);
    void encodePartSize(TComDataCU* cu, UInt absPartIdx, UInt depth, bool bRD = false);
    void encodeIPCMInfo(TComDataCU* cu, UInt absPartIdx, bool bRD = false);
    void encodePredInfo(TComDataCU* cu, UInt absPartIdx, bool bRD = false);
    void encodeIntraDirModeLuma(TComDataCU* cu, UInt absPartIdx, bool isMultiplePU = false);

    void encodeIntraDirModeChroma(TComDataCU* cu, UInt absPartIdx, bool bRD = false);

    void encodeTransformSubdivFlag(UInt symbol, UInt ctx);
    void encodeQtCbf(TComDataCU* cu, UInt absPartIdx, TextType ttype, UInt trDepth);
    void encodeQtCbfZero(TComDataCU* cu, TextType ttype, UInt trDepth);
    void encodeQtRootCbfZero(TComDataCU* cu);
    void encodeQtRootCbf(TComDataCU* cu, UInt absPartIdx);
    void encodeQP(TComDataCU* cu, UInt absPartIdx, bool bRD = false);

    void encodeScalingList(TComScalingList* scalingList);

private:

    void xEncodeTransform(TComDataCU* cu, UInt offsetLumaOffset, UInt offsetChroma, UInt absPartIdx, UInt depth, UInt width, UInt height, UInt uiTrIdx, bool& bCodeDQP);

public:

    void encodeCoeff(TComDataCU* cu, UInt absPartIdx, UInt depth, UInt width, UInt height, bool& bCodeDQP);

    void encodeCoeffNxN(TComDataCU* cu, TCoeff* pcCoeff, UInt absPartIdx, UInt trWidth, UInt trHeight, UInt depth, TextType ttype);

    void estimateBit(estBitsSbacStruct* estBitsSbac, int width, int height, TextType ttype);
    void encodeSaoOffset(SaoLcuParam* saoLcuParam, UInt compIdx);
    void encodeSaoUnitInterleaving(int compIdx, bool saoFlag, int rx, int ry, SaoLcuParam* saoLcuParam, int cuAddrInSlice, int cuAddrUpInSlice, int allowMergeLeft, int allowMergeUp);
    static int countNonZeroCoeffs(TCoeff* pcCoef, UInt uiSize);
}; // END CLASS DEFINITION TEncEntropy
}
//! \}

#endif // ifndef X265_TENCENTROPY_H
