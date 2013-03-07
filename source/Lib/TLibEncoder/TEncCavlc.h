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

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

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

  Void  xWritePCMAlignZero    ();
  Void  xWriteEpExGolomb      ( UInt uiSymbol, UInt uiCount );
  Void  xWriteExGolombLevel    ( UInt uiSymbol );
  Void  xWriteUnaryMaxSymbol  ( UInt uiSymbol, UInt uiMaxSymbol );

  Void codeShortTermRefPicSet              ( TComSPS* pcSPS, TComReferencePictureSet* pcRPS, Bool calledFromSliceHeader, Int idx );
  Bool findMatchingLTRP ( TComSlice* pcSlice, UInt *ltrpsIndex, Int ltrpPOC, Bool usedFlag );
  
public:
  
  Void  resetEntropy          ();
  Void  determineCabacInitIdx  () {};

  Void  setBitstream          ( TComBitIf* p )  { m_pcBitIf = p;  }
  Void  setSlice              ( TComSlice* p )  { m_pcSlice = p;  }
  Void  resetBits             ()                { m_pcBitIf->resetBits(); }
  Void  resetCoeffCost        ()                { m_uiCoeffCost = 0;  }
  UInt  getNumberOfWrittenBits()                { return  m_pcBitIf->getNumberOfWrittenBits();  }
  UInt  getCoeffCost          ()                { return  m_uiCoeffCost;  }
  Void  codeVPS                 ( TComVPS* pcVPS );
  Void  codeVUI                 ( TComVUI *pcVUI, TComSPS* pcSPS );
  Void  codeSPS                 ( TComSPS* pcSPS );
  Void  codePPS                 ( TComPPS* pcPPS );
  Void  codeSliceHeader         ( TComSlice* pcSlice );
  Void  codePTL                 ( TComPTL* pcPTL, Bool profilePresentFlag, Int maxNumSubLayersMinus1);
  Void  codeProfileTier         ( ProfileTierLevel* ptl );
#if SIGNAL_BITRATE_PICRATE_IN_VPS
  Void codeBitratePicRateInfo(TComBitRatePicRateInfo *info, Int tempLevelLow, Int tempLevelHigh);
#endif
  Void  codeHrdParameters       ( TComHRD *hrd, Bool commonInfPresentFlag, UInt maxNumSubLayersMinus1 );
  Void  codeTilesWPPEntryPoint( TComSlice* pSlice );
  Void  codeTerminatingBit      ( UInt uilsLast );
  Void  codeSliceFinish         ();
  
  Void codeMVPIdx ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList );
  Void codeSAOSign       ( UInt code   ) { printf("Not supported\n"); assert (0); }
  Void codeSaoMaxUvlc    ( UInt   code, UInt maxSymbol ){printf("Not supported\n"); assert (0);}
  Void codeSaoMerge  ( UInt uiCode ){printf("Not supported\n"); assert (0);}
  Void codeSaoTypeIdx    ( UInt uiCode ){printf("Not supported\n"); assert (0);}
  Void codeSaoUflc       ( UInt uiLength, UInt   uiCode ){ assert(uiCode < 32); printf("Not supported\n"); assert (0);}

  Void codeCUTransquantBypassFlag( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void codeSkipFlag      ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void codeMergeFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void codeMergeIndex    ( TComDataCU* pcCU, UInt uiAbsPartIdx );
 
  Void codeInterModeFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiEncMode );
  Void codeSplitFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  
  Void codePartSize      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void codePredMode      ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  
  Void codeIPCMInfo      ( TComDataCU* pcCU, UInt uiAbsPartIdx );

  Void codeTransformSubdivFlag( UInt uiSymbol, UInt uiCtx );
  Void codeQtCbf         ( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth );
  Void codeQtRootCbf     ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void codeQtCbfZero     ( TComDataCU* pcCU, TextType eType, UInt uiTrDepth );
  Void codeQtRootCbfZero ( TComDataCU* pcCU );
  Void codeIntraDirLumaAng( TComDataCU* pcCU, UInt absPartIdx, Bool isMultiple);
  Void codeIntraDirChroma( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void codeInterDir      ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void codeRefFrmIdx     ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList );
  Void codeMvd           ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList );
  
  Void codeDeltaQP       ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  
  Void codeCoeffNxN      ( TComDataCU* pcCU, TCoeff* pcCoef, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, UInt uiDepth, TextType eTType );
  Void codeTransformSkipFlags ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt width, UInt height, TextType eTType );

  Void estBit               (estBitsSbacStruct* pcEstBitsSbac, Int width, Int height, TextType eTType);
  
  Void xCodePredWeightTable          ( TComSlice* pcSlice );
  Void updateContextTables           ( SliceType eSliceType, Int iQp, Bool bExecuteFinish=true ) { return;   }
  Void updateContextTables           ( SliceType eSliceType, Int iQp  )                          { return;   }

  Void codeScalingList  ( TComScalingList* scalingList );
  Void xCodeScalingList ( TComScalingList* scalingList, UInt sizeId, UInt listId);
  Void codeDFFlag       ( UInt uiCode, const Char *pSymbolName );
  Void codeDFSvlc       ( Int   iCode, const Char *pSymbolName );

};

//! \}

#endif // !defined(AFX_TENCCAVLC_H__EE8A0B30_945B_4169_B290_24D3AD52296F__INCLUDED_)

