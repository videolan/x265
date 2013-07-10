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

    Void codeShortTermRefPicSet(TComSPS* pcSPS, TComReferencePictureSet* pcRPS, Bool calledFromSliceHeader, Int idx);
    Bool findMatchingLTRP(TComSlice* pcSlice, UInt *ltrpsIndex, Int ltrpPOC, Bool usedFlag);

public:

    Void  resetEntropy();
    Void  determineCabacInitIdx() {}

    Void  setBitstream(TComBitIf* p)  { m_pcBitIf = p;  }

    Void  setSlice(TComSlice* p)      { m_pcSlice = p; }

    Void  resetBits()                 { m_pcBitIf->resetBits(); }

    Void  resetCoeffCost()            { m_uiCoeffCost = 0; }

    UInt  getNumberOfWrittenBits()    { return m_pcBitIf->getNumberOfWrittenBits(); }

    UInt  getCoeffCost()              { return m_uiCoeffCost; }

    Void  codeVPS(TComVPS* pcVPS);
    Void  codeVUI(TComVUI *pcVUI, TComSPS* pcSPS);
    Void  codeSPS(TComSPS* pcSPS);
    Void  codePPS(TComPPS* pcPPS);
    Void  codeSliceHeader(TComSlice* pcSlice);
    Void  codePTL(TComPTL* pcPTL, Bool profilePresentFlag, Int maxNumSubLayersMinus1);
    Void  codeProfileTier(ProfileTierLevel* ptl);
    Void  codeHrdParameters(TComHRD *hrd, Bool commonInfPresentFlag, UInt maxNumSubLayersMinus1);
    Void  codeTilesWPPEntryPoint(TComSlice* pSlice);
    Void  codeTerminatingBit(UInt uilsLast);
    Void  codeSliceFinish();

    Void codeMVPIdx(TComDataCU* cu, UInt uiAbsPartIdx, RefPicList eRefList);

    Void codeSAOSign(UInt) { printf("Not supported\n"); assert(0); }

    Void codeSaoMaxUvlc(UInt, UInt) { printf("Not supported\n"); assert(0); }

    Void codeSaoMerge(UInt) { printf("Not supported\n"); assert(0); }

    Void codeSaoTypeIdx(UInt) { printf("Not supported\n"); assert(0); }

    Void codeSaoUflc(UInt, UInt) { printf("Not supported\n"); assert(0); }

    Void updateContextTables(SliceType, Int, Bool) {}

    Void updateContextTables(SliceType, Int) {}

    Void codeCUTransquantBypassFlag(TComDataCU* cu, UInt uiAbsPartIdx);
    Void codeSkipFlag(TComDataCU* cu, UInt uiAbsPartIdx);
    Void codeMergeFlag(TComDataCU* cu, UInt uiAbsPartIdx);
    Void codeMergeIndex(TComDataCU* cu, UInt uiAbsPartIdx);

    Void codeInterModeFlag(TComDataCU* cu, UInt uiAbsPartIdx, UInt uiDepth, UInt uiEncMode);
    Void codeSplitFlag(TComDataCU* cu, UInt uiAbsPartIdx, UInt uiDepth);

    Void codePartSize(TComDataCU* cu, UInt uiAbsPartIdx, UInt uiDepth);
    Void codePredMode(TComDataCU* cu, UInt uiAbsPartIdx);

    Void codeIPCMInfo(TComDataCU* cu, UInt uiAbsPartIdx);

    Void codeTransformSubdivFlag(UInt uiSymbol, UInt uiCtx);
    Void codeQtCbf(TComDataCU* cu, UInt uiAbsPartIdx, TextType eType, UInt trDepth);
    Void codeQtRootCbf(TComDataCU* cu, UInt uiAbsPartIdx);
    Void codeQtCbfZero(TComDataCU* cu, TextType eType, UInt trDepth);
    Void codeQtRootCbfZero(TComDataCU* cu);
    Void codeIntraDirLumaAng(TComDataCU* cu, UInt absPartIdx, Bool isMultiple);
    Void codeIntraDirChroma(TComDataCU* cu, UInt uiAbsPartIdx);
    Void codeInterDir(TComDataCU* cu, UInt uiAbsPartIdx);
    Void codeRefFrmIdx(TComDataCU* cu, UInt uiAbsPartIdx, RefPicList eRefList);
    Void codeMvd(TComDataCU* cu, UInt uiAbsPartIdx, RefPicList eRefList);

    Void codeDeltaQP(TComDataCU* cu, UInt uiAbsPartIdx);

    Void codeCoeffNxN(TComDataCU* cu, TCoeff* pcCoef, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, UInt uiDepth, TextType eTType);
    Void codeTransformSkipFlags(TComDataCU* cu, UInt uiAbsPartIdx, UInt width, UInt height, TextType eTType);

    Void estBit(estBitsSbacStruct* pcEstBitsSbac, Int width, Int height, TextType eTType);

    Void xCodePredWeightTable(TComSlice* pcSlice);

    Void codeScalingList(TComScalingList* scalingList);
    Void xCodeScalingList(TComScalingList* scalingList, UInt sizeId, UInt listId);
    Void codeDFFlag(UInt uiCode, const Char *pSymbolName);
    Void codeDFSvlc(Int iCode, const Char *pSymbolName);
};

//! \}

#endif // !defined(_TENCCAVLC_)
