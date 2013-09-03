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

/** \file     TEncCavlc.h
    \brief    CAVLC encoder class (header)
*/

#ifndef __TENCCAVLC__
#define __TENCCAVLC__

#include "TLibCommon/CommonDef.h"
#include "TLibCommon/TComBitStream.h"
#include "TLibCommon/TComRom.h"
#include "TEncEntropy.h"
#include "SyntaxElementWriter.h"

//! \ingroup TLibEncoder
//! \{

namespace x265 {
// private namespace

class TEncTop;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// CAVLC encoder class
class TEncCavlc : public SyntaxElementWriter, public TEncEntropyIf
{
public:

    TEncCavlc();
    virtual ~TEncCavlc();

protected:

    TComSlice*    m_pcSlice;
    UInt          m_uiCoeffCost;

    void codeShortTermRefPicSet(TComReferencePictureSet* pcRPS, Bool calledFromSliceHeader, Int idx);
    Bool findMatchingLTRP(TComSlice* slice, UInt *ltrpsIndex, Int ltrpPOC, Bool usedFlag);

public:

    void  resetEntropy();
    void  determineCabacInitIdx() {}

    void  setBitstream(TComBitIf* p)  { m_bitIf = p;  }

    void  setSlice(TComSlice* p)      { m_pcSlice = p; }

    void  resetBits()                 { m_bitIf->resetBits(); }

    void  resetCoeffCost()            { m_uiCoeffCost = 0; }

    UInt  getNumberOfWrittenBits()    { return m_bitIf->getNumberOfWrittenBits(); }

    UInt  getCoeffCost()              { return m_uiCoeffCost; }

    void  codeVPS(TComVPS* pcVPS);
    void  codeVUI(TComVUI *pcVUI, TComSPS* pcSPS);
    void  codeSPS(TComSPS* pcSPS);
    void  codePPS(TComPPS* pcPPS);
    void  codeSliceHeader(TComSlice* slice);
    void  codePTL(TComPTL* pcPTL, Bool profilePresentFlag, Int maxNumSubLayersMinus1);
    void  codeProfileTier(ProfileTierLevel* ptl);
    void  codeHrdParameters(TComHRD *hrd, Bool commonInfPresentFlag, UInt maxNumSubLayersMinus1);
    void  codeTilesWPPEntryPoint(TComSlice* pSlice);
    void  codeTerminatingBit(UInt uilsLast);
    void  codeSliceFinish();

    void codeMVPIdx(TComDataCU* cu, UInt absPartIdx, RefPicList eRefList);

    void codeSAOSign(UInt) { printf("Not supported\n"); assert(0); }

    void codeSaoMaxUvlc(UInt, UInt) { printf("Not supported\n"); assert(0); }

    void codeSaoMerge(UInt) { printf("Not supported\n"); assert(0); }

    void codeSaoTypeIdx(UInt) { printf("Not supported\n"); assert(0); }

    void codeSaoUflc(UInt, UInt) { printf("Not supported\n"); assert(0); }

    void updateContextTables(SliceType, Int, Bool) {}

    void updateContextTables(SliceType, Int) {}

    void codeCUTransquantBypassFlag(TComDataCU* cu, UInt absPartIdx);
    void codeSkipFlag(TComDataCU* cu, UInt absPartIdx);
    void codeMergeFlag(TComDataCU* cu, UInt absPartIdx);
    void codeMergeIndex(TComDataCU* cu, UInt absPartIdx);

    void codeInterModeFlag(TComDataCU* cu, UInt absPartIdx, UInt depth, UInt uiEncMode);
    void codeSplitFlag(TComDataCU* cu, UInt absPartIdx, UInt depth);

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

    void codeCoeffNxN(TComDataCU* cu, TCoeff* pcCoef, UInt absPartIdx, UInt width, UInt height, UInt depth, TextType eTType);
    void codeTransformSkipFlags(TComDataCU* cu, UInt absPartIdx, UInt width, UInt height, TextType eTType);

    void estBit(estBitsSbacStruct* pcEstBitsSbac, Int width, Int height, TextType eTType);

    void xCodePredWeightTable(TComSlice* slice);

    void codeScalingList(TComScalingList* scalingList);
    void xCodeScalingList(TComScalingList* scalingList, UInt sizeId, UInt listId);
    void codeDFFlag(UInt uiCode, const char *pSymbolName);
    void codeDFSvlc(Int iCode, const char *pSymbolName);
};
}
//! \}

#endif // !defined(_TENCCAVLC_)
