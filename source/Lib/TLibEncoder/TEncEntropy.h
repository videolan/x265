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
    virtual uint32_t getNumberOfWrittenBits() = 0;
    virtual uint32_t getCoeffCost() = 0;

    virtual void codeVPS(TComVPS* vps) = 0;
    virtual void codeSPS(TComSPS* sps) = 0;
    virtual void codePPS(TComPPS* pps) = 0;
    virtual void codeSliceHeader(TComSlice* slice) = 0;

    virtual void codeTilesWPPEntryPoint(TComSlice* slice) = 0;
    virtual void codeTerminatingBit(uint32_t isLast) = 0;
    virtual void codeSliceFinish() = 0;
    virtual void codeMVPIdx(TComDataCU* cu, uint32_t absPartIdx, int list) = 0;
    virtual void codeScalingList(TComScalingList* scalingList) = 0;

public:

    virtual void codeCUTransquantBypassFlag(TComDataCU* cu, uint32_t absPartIdx) = 0;
    virtual void codeSkipFlag(TComDataCU* cu, uint32_t absPartIdx) = 0;
    virtual void codeMergeFlag(TComDataCU* cu, uint32_t absPartIdx) = 0;
    virtual void codeMergeIndex(TComDataCU* cu, uint32_t absPartIdx) = 0;
    virtual void codeSplitFlag(TComDataCU* cu, uint32_t absPartIdx, uint32_t depth) = 0;

    virtual void codePartSize(TComDataCU* cu, uint32_t absPartIdx, uint32_t depth) = 0;
    virtual void codePredMode(TComDataCU* cu, uint32_t absPartIdx) = 0;

    virtual void codeIPCMInfo(TComDataCU* cu, uint32_t absPartIdx) = 0;

    virtual void codeTransformSubdivFlag(uint32_t symbol, uint32_t ctx) = 0;
    virtual void codeQtCbf(TComDataCU* cu, uint32_t absPartIdx, TextType ttype, uint32_t trDepth) = 0;
    virtual void codeQtRootCbf(TComDataCU* cu, uint32_t absPartIdx) = 0;
    virtual void codeQtCbfZero(TComDataCU* cu, TextType ttype, uint32_t trDepth) = 0;
    virtual void codeQtRootCbfZero(TComDataCU* cu) = 0;
    virtual void codeIntraDirLumaAng(TComDataCU* cu, uint32_t absPartIdx, bool isMultiplePU) = 0;

    virtual void codeIntraDirChroma(TComDataCU* cu, uint32_t absPartIdx) = 0;
    virtual void codeInterDir(TComDataCU* cu, uint32_t absPartIdx) = 0;
    virtual void codeRefFrmIdx(TComDataCU* cu, uint32_t absPartIdx, int eRefList) = 0;
    virtual void codeMvd(TComDataCU* cu, uint32_t absPartIdx, int eRefList) = 0;
    virtual void codeDeltaQP(TComDataCU* cu, uint32_t absPartIdx) = 0;
    virtual void codeCoeffNxN(TComDataCU* cu, TCoeff* pcCoef, uint32_t absPartIdx, uint32_t width, uint32_t height, uint32_t depth, TextType ttype) = 0;
    virtual void codeTransformSkipFlags(TComDataCU* cu, uint32_t absPartIdx, uint32_t width, uint32_t height, TextType ttype) = 0;
    virtual void codeSAOSign(uint32_t code) = 0;
    virtual void codeSaoMaxUvlc(uint32_t code, uint32_t maxSymbol) = 0;
    virtual void codeSaoMerge(uint32_t code) = 0;
    virtual void codeSaoTypeIdx(uint32_t code) = 0;
    virtual void codeSaoUflc(uint32_t length, uint32_t code) = 0;
    virtual void estBit(estBitsSbacStruct* estBitsSbac, int width, int height, TextType ttype) = 0;

    virtual void codeDFFlag(uint32_t code, const char *symbolName) = 0;
    virtual void codeDFSvlc(int code, const char *symbolName)   = 0;

    virtual ~TEncEntropyIf() {}
};

/// entropy encoder class
class TEncEntropy
{
private:

    uint32_t    m_bakAbsPartIdx;
    uint32_t    m_bakChromaOffset;
    uint32_t    m_bakAbsPartIdxCU;

public:

    void    setEntropyCoder(TEncEntropyIf* e, TComSlice* slice);
    void    setBitstream(TComBitIf* p) { m_entropyCoderIf->setBitstream(p);  }

    void    resetBits() { m_entropyCoderIf->resetBits();      }

    void    resetCoeffCost() { m_entropyCoderIf->resetCoeffCost(); }

    uint32_t    getNumberOfWrittenBits() { return m_entropyCoderIf->getNumberOfWrittenBits(); }

    uint32_t    getCoeffCost() { return m_entropyCoderIf->getCoeffCost(); }

    void    resetEntropy() { m_entropyCoderIf->resetEntropy();  }

    void    determineCabacInitIdx() { m_entropyCoderIf->determineCabacInitIdx(); }

    void    encodeSliceHeader(TComSlice* slice);
    void    encodeTilesWPPEntryPoint(TComSlice* pSlice);
    void    encodeTerminatingBit(uint32_t uiIsLast);
    void    encodeSliceFinish();
    TEncEntropyIf*      m_entropyCoderIf;

public:

    void encodeVPS(TComVPS* vps);
    // SPS
    void encodeSPS(TComSPS* sps);
    void encodePPS(TComPPS* pps);
    void encodeSplitFlag(TComDataCU* cu, uint32_t absPartIdx, uint32_t depth, bool bRD = false);
    void encodeCUTransquantBypassFlag(TComDataCU* cu, uint32_t absPartIdx, bool bRD = false);
    void encodeSkipFlag(TComDataCU* cu, uint32_t absPartIdx, bool bRD = false);
    void encodePUWise(TComDataCU* cu, uint32_t absPartIdx, bool bRD = false);
    void encodeInterDirPU(TComDataCU* pcSubCU, uint32_t absPartIdx);
    void encodeRefFrmIdxPU(TComDataCU* pcSubCU, uint32_t absPartIdx, int eRefList);
    void encodeMvdPU(TComDataCU* pcSubCU, uint32_t absPartIdx, int eRefList);
    void encodeMVPIdxPU(TComDataCU* pcSubCU, uint32_t absPartIdx, int eRefList);
    void encodeMergeFlag(TComDataCU* cu, uint32_t absPartIdx);
    void encodeMergeIndex(TComDataCU* cu, uint32_t absPartIdx, bool bRD = false);
    void encodePredMode(TComDataCU* cu, uint32_t absPartIdx, bool bRD = false);
    void encodePartSize(TComDataCU* cu, uint32_t absPartIdx, uint32_t depth, bool bRD = false);
    void encodeIPCMInfo(TComDataCU* cu, uint32_t absPartIdx, bool bRD = false);
    void encodePredInfo(TComDataCU* cu, uint32_t absPartIdx, bool bRD = false);
    void encodeIntraDirModeLuma(TComDataCU* cu, uint32_t absPartIdx, bool isMultiplePU = false);

    void encodeIntraDirModeChroma(TComDataCU* cu, uint32_t absPartIdx, bool bRD = false);

    void encodeTransformSubdivFlag(uint32_t symbol, uint32_t ctx);
    void encodeQtCbf(TComDataCU* cu, uint32_t absPartIdx, TextType ttype, uint32_t trDepth);
    void encodeQtCbfZero(TComDataCU* cu, TextType ttype, uint32_t trDepth);
    void encodeQtRootCbfZero(TComDataCU* cu);
    void encodeQtRootCbf(TComDataCU* cu, uint32_t absPartIdx);
    void encodeQP(TComDataCU* cu, uint32_t absPartIdx, bool bRD = false);

    void encodeScalingList(TComScalingList* scalingList);

private:

    void xEncodeTransform(TComDataCU* cu, uint32_t offsetLumaOffset, uint32_t offsetChroma, uint32_t absPartIdx, uint32_t depth, uint32_t width, uint32_t height, uint32_t uiTrIdx, bool& bCodeDQP);

public:

    void encodeCoeff(TComDataCU* cu, uint32_t absPartIdx, uint32_t depth, uint32_t width, uint32_t height, bool& bCodeDQP);

    void encodeCoeffNxN(TComDataCU* cu, TCoeff* pcCoeff, uint32_t absPartIdx, uint32_t trWidth, uint32_t trHeight, uint32_t depth, TextType ttype);

    void estimateBit(estBitsSbacStruct* estBitsSbac, int width, int height, TextType ttype);
    void encodeSaoOffset(SaoLcuParam* saoLcuParam, uint32_t compIdx);
    void encodeSaoUnitInterleaving(int compIdx, bool saoFlag, int rx, int ry, SaoLcuParam* saoLcuParam, int cuAddrInSlice, int cuAddrUpInSlice, int allowMergeLeft, int allowMergeUp);
    static int countNonZeroCoeffs(TCoeff* pcCoef, uint32_t uiSize);
}; // END CLASS DEFINITION TEncEntropy
}
//! \}

#endif // ifndef X265_TENCENTROPY_H
