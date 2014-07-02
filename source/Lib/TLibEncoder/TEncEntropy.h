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

#include "common.h"
#include "bitstream.h"
#include "frame.h"
#include "TLibCommon/TComSlice.h"
#include "TLibCommon/TComDataCU.h"
#include "TLibCommon/TComTrQuant.h"
#include "TLibCommon/TComSampleAdaptiveOffset.h"

#include "TEncSbac.h"

namespace x265 {
// private namespace

class TEncCavlc;
class SEI;

#define DONT_SPLIT            0
#define VERTICAL_SPLIT        1
#define QUAD_SPLIT            2
#define NUMBER_OF_SPLIT_MODES 3

static const uint32_t partIdxStepShift[NUMBER_OF_SPLIT_MODES] = { 0, 1, 2 };
static const uint32_t NUMBER_OF_SECTIONS[NUMBER_OF_SPLIT_MODES] = { 1, 2, 4 };

struct TComTURecurse
{
    uint32_t m_section;
    uint32_t m_splitMode;
    uint32_t m_absPartIdxTURelCU;
    uint32_t m_absPartIdxStep;
};

/// entropy encoder class
class TEncEntropy
{
public:

    TEncSbac* m_entropyCoder;
    uint32_t  m_bakAbsPartIdx;
    uint32_t  m_bakChromaOffset;
    uint32_t  m_bakAbsPartIdxCU;

    void setEntropyCoder(TEncSbac* e, TComSlice* slice) { m_entropyCoder = e; m_entropyCoder->setSlice(slice); }
    void setBitstream(BitInterface* p)  { m_entropyCoder->setBitstream(p); }
    void resetBits()                    { m_entropyCoder->resetBits();     }
    void resetEntropy()                 { m_entropyCoder->resetEntropy();  }
    void determineCabacInitIdx()        { m_entropyCoder->determineCabacInitIdx(); }
    uint32_t getNumberOfWrittenBits()   { return m_entropyCoder->getNumberOfWrittenBits(); }

    void encodeSliceHeader(TComSlice* slice)         { m_entropyCoder->codeSliceHeader(slice); }
    void encodeTilesWPPEntryPoint(TComSlice* slice)  { m_entropyCoder->codeTilesWPPEntryPoint(slice); }
    void encodeTerminatingBit(uint32_t isLast)       { m_entropyCoder->codeTerminatingBit(isLast); }
    void encodeSliceFinish()                         { m_entropyCoder->codeSliceFinish(); }

    void encodeVPS(TComVPS* vps)                     { m_entropyCoder->codeVPS(vps); }
    void encodeSPS(TComSPS* sps)                     { m_entropyCoder->codeSPS(sps); }
    void encodePPS(TComPPS* pps)                     { m_entropyCoder->codePPS(pps); }
    void encodeAUD(TComSlice* slice)                 { m_entropyCoder->codeAUD(slice); }

    void encodeCUTransquantBypassFlag(TComDataCU* cu, uint32_t absPartIdx) { m_entropyCoder->codeCUTransquantBypassFlag(cu, absPartIdx); }
    void encodeMergeFlag(TComDataCU* cu, uint32_t absPartIdx)              { m_entropyCoder->codeMergeFlag(cu, absPartIdx); }
    void encodeMergeIndex(TComDataCU* cu, uint32_t absPartIdx)             { m_entropyCoder->codeMergeIndex(cu, absPartIdx); }
    void encodeMvdPU(TComDataCU* cu, uint32_t absPartIdx, int list)        { m_entropyCoder->codeMvd(cu, absPartIdx, list); }
    void encodeMVPIdxPU(TComDataCU* cu, uint32_t absPartIdx, int list)     { m_entropyCoder->codeMVPIdx(cu->getMVPIdx(list, absPartIdx)); }
    void encodeIntraDirModeChroma(TComDataCU* cu, uint32_t absPartIdx)     { m_entropyCoder->codeIntraDirChroma(cu, absPartIdx); }
    void encodeTransformSubdivFlag(uint32_t symbol, uint32_t ctx)          { m_entropyCoder->codeTransformSubdivFlag(symbol, ctx); }
    void encodeQtRootCbf(TComDataCU* cu, uint32_t absPartIdx)              { m_entropyCoder->codeQtRootCbf(cu, absPartIdx); }
    void encodeQtRootCbfZero(TComDataCU* cu)                               { m_entropyCoder->codeQtRootCbfZero(cu); }
    void encodeQP(TComDataCU* cu, uint32_t absPartIdx)                     { m_entropyCoder->codeDeltaQP(cu, absPartIdx); }
    void encodeScalingList(TComScalingList* scalingList)                   { m_entropyCoder->codeScalingList(scalingList); }
    void encodeSkipFlag(TComDataCU* cu, uint32_t absPartIdx)               { m_entropyCoder->codeSkipFlag(cu, absPartIdx); }
    void encodePredMode(TComDataCU* cu, uint32_t absPartIdx)               { m_entropyCoder->codePredMode(cu, absPartIdx); }

    void encodeSplitFlag(TComDataCU* cu, uint32_t absPartIdx, uint32_t depth) { m_entropyCoder->codeSplitFlag(cu, absPartIdx, depth); }
    void encodePartSize(TComDataCU* cu, uint32_t absPartIdx, uint32_t depth)  { m_entropyCoder->codePartSize(cu, absPartIdx, depth); }

    void encodeIntraDirModeLuma(TComDataCU* cu, uint32_t absPartIdx, bool isMultiplePU = false)
    {
        m_entropyCoder->codeIntraDirLumaAng(cu, absPartIdx, isMultiplePU);
    }
    void encodeQtCbf(TComDataCU* cu, uint32_t absPartIdx, uint32_t absPartIdxStep, uint32_t width, uint32_t height, TextType ttype, uint32_t trDepth, bool lowestLevel)
    {
        m_entropyCoder->codeQtCbf(cu, absPartIdx, ttype, trDepth, absPartIdxStep, width, height, lowestLevel);
    }
    void encodeQtCbf(TComDataCU* cu, uint32_t absPartIdx, TextType ttype, uint32_t trDepth)
    {
        m_entropyCoder->codeQtCbf(cu, absPartIdx, ttype, trDepth);
    }
    void encodeQtCbfZero(TComDataCU* cu, TextType ttype, uint32_t trDepth)
    {
        m_entropyCoder->codeQtCbfZero(cu, ttype, trDepth);
    }

    void encodePUWise(TComDataCU* cu, uint32_t absPartIdx);
    void encodeRefFrmIdxPU(TComDataCU* subCU, uint32_t absPartIdx, int eRefList);
    void encodePredInfo(TComDataCU* cu, uint32_t absPartIdx);

    void encodeCoeff(TComDataCU* cu, uint32_t absPartIdx, uint32_t depth, uint32_t cuSize, bool& bCodeDQP);
    void encodeCoeffNxN(TComDataCU* cu, coeff_t* coeff, uint32_t absPartIdx, uint32_t log2TrSize, TextType ttype)
    {
        m_entropyCoder->codeCoeffNxN(cu, coeff, absPartIdx, log2TrSize, ttype);
    }

    void estimateBit(estBitsSbacStruct* estBitsSbac, int trSize, TextType ttype)
    {
        m_entropyCoder->estBit(estBitsSbac, trSize, ttype == TEXT_LUMA ? TEXT_LUMA : TEXT_CHROMA);
    }

    void encodeSaoOffset(SaoLcuParam* saoLcuParam, uint32_t compIdx);
    void encodeSaoUnitInterleaving(int compIdx, bool saoFlag, int rx, int ry, SaoLcuParam* saoLcuParam, int cuAddrInSlice, int cuAddrUpInSlice, int allowMergeLeft, int allowMergeUp);
    void initTUEntropySection(TComTURecurse *TUIterator, uint32_t splitMode, uint32_t absPartIdxStep, uint32_t m_absPartIdxTU);
    bool isNextTUSection(TComTURecurse *TUIterator);

    void xEncodeTransform(TComDataCU* cu, uint32_t offsetLumaOffset, uint32_t offsetChroma, uint32_t absPartIdx, uint32_t absPartIdxStep, uint32_t depth, uint32_t tuSize, uint32_t uiTrIdx, bool& bCodeDQP);
};
}

#endif // ifndef X265_TENCENTROPY_H
