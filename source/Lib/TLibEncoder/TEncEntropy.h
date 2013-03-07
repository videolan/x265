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
  virtual Void  resetEntropy          ()                = 0;
  virtual Void  determineCabacInitIdx ()                = 0;
  virtual Void  setBitstream          ( TComBitIf* p )  = 0;
  virtual Void  setSlice              ( TComSlice* p )  = 0;
  virtual Void  resetBits             ()                = 0;
  virtual Void  resetCoeffCost        ()                = 0;
  virtual UInt  getNumberOfWrittenBits()                = 0;
  virtual UInt  getCoeffCost          ()                = 0;

  virtual Void  codeVPS                 ( TComVPS* pcVPS )                                      = 0;
  virtual Void  codeSPS                 ( TComSPS* pcSPS )                                      = 0;
  virtual Void  codePPS                 ( TComPPS* pcPPS )                                      = 0;
  virtual Void  codeSliceHeader         ( TComSlice* pcSlice )                                  = 0;

  virtual Void  codeTilesWPPEntryPoint  ( TComSlice* pSlice )     = 0;
  virtual Void  codeTerminatingBit      ( UInt uilsLast )                                       = 0;
  virtual Void  codeSliceFinish         ()                                                      = 0;
  virtual Void codeMVPIdx ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList ) = 0;
  virtual Void codeScalingList   ( TComScalingList* scalingList )      = 0;
  
public:
  virtual Void codeCUTransquantBypassFlag( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
  virtual Void codeSkipFlag      ( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
  virtual Void codeMergeFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
  virtual Void codeMergeIndex    ( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
  virtual Void codeSplitFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth ) = 0;
  
  virtual Void codePartSize      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth ) = 0;
  virtual Void codePredMode      ( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
  
  virtual Void codeIPCMInfo      ( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;

  virtual Void codeTransformSubdivFlag( UInt uiSymbol, UInt uiCtx ) = 0;
  virtual Void codeQtCbf         ( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth ) = 0;
  virtual Void codeQtRootCbf     ( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
  virtual Void codeQtCbfZero     ( TComDataCU* pcCU, TextType eType, UInt uiTrDepth ) = 0;
  virtual Void codeQtRootCbfZero ( TComDataCU* pcCU ) = 0;
  virtual Void codeIntraDirLumaAng( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool isMultiplePU ) = 0;
  
  virtual Void codeIntraDirChroma( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
  virtual Void codeInterDir      ( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
  virtual Void codeRefFrmIdx     ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )      = 0;
  virtual Void codeMvd           ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )      = 0;
  virtual Void codeDeltaQP       ( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
  virtual Void codeCoeffNxN      ( TComDataCU* pcCU, TCoeff* pcCoef, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, UInt uiDepth, TextType eTType ) = 0;
  virtual Void codeTransformSkipFlags ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt width, UInt height, TextType eTType ) = 0;
  virtual Void codeSAOSign          ( UInt code   ) = 0;
  virtual Void codeSaoMaxUvlc       ( UInt code, UInt maxSymbol ) = 0;
  virtual Void codeSaoMerge    ( UInt   uiCode  ) = 0;
  virtual Void codeSaoTypeIdx      ( UInt   uiCode) = 0;
  virtual Void codeSaoUflc         ( UInt uiLength, UInt   uiCode ) = 0;
  virtual Void estBit               (estBitsSbacStruct* pcEstBitsSbac, Int width, Int height, TextType eTType) = 0;
  
  virtual Void updateContextTables ( SliceType eSliceType, Int iQp, Bool bExecuteFinish )   = 0;
  virtual Void updateContextTables ( SliceType eSliceType, Int iQp )   = 0;

  virtual Void codeDFFlag (UInt uiCode, const Char *pSymbolName) = 0;
  virtual Void codeDFSvlc (Int iCode, const Char *pSymbolName)   = 0;

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
  Void    setEntropyCoder           ( TEncEntropyIf* e, TComSlice* pcSlice );
  Void    setBitstream              ( TComBitIf* p )          { m_pcEntropyCoderIf->setBitstream(p);  }
  Void    resetBits                 ()                        { m_pcEntropyCoderIf->resetBits();      }
  Void    resetCoeffCost            ()                        { m_pcEntropyCoderIf->resetCoeffCost(); }
  UInt    getNumberOfWrittenBits    ()                        { return m_pcEntropyCoderIf->getNumberOfWrittenBits(); }
  UInt    getCoeffCost              ()                        { return  m_pcEntropyCoderIf->getCoeffCost(); }
  Void    resetEntropy              ()                        { m_pcEntropyCoderIf->resetEntropy();  }
  Void    determineCabacInitIdx     ()                        { m_pcEntropyCoderIf->determineCabacInitIdx(); }
  
  Void    encodeSliceHeader         ( TComSlice* pcSlice );
  Void    encodeTilesWPPEntryPoint( TComSlice* pSlice );
  Void    encodeTerminatingBit      ( UInt uiIsLast );
  Void    encodeSliceFinish         ();
  TEncEntropyIf*      m_pcEntropyCoderIf;
  
public:
  Void encodeVPS               ( TComVPS* pcVPS);
  // SPS
  Void encodeSPS               ( TComSPS* pcSPS );
  Void encodePPS               ( TComPPS* pcPPS );
  Void encodeSplitFlag         ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, Bool bRD = false );
  Void encodeCUTransquantBypassFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD = false );
  Void encodeSkipFlag          ( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD = false );
  Void encodePUWise       ( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD = false );
  Void encodeInterDirPU   ( TComDataCU* pcSubCU, UInt uiAbsPartIdx  );
  Void encodeRefFrmIdxPU  ( TComDataCU* pcSubCU, UInt uiAbsPartIdx, RefPicList eRefList );
  Void encodeMvdPU        ( TComDataCU* pcSubCU, UInt uiAbsPartIdx, RefPicList eRefList );
  Void encodeMVPIdxPU     ( TComDataCU* pcSubCU, UInt uiAbsPartIdx, RefPicList eRefList );
  Void encodeMergeFlag    ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void encodeMergeIndex   ( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD = false );
  Void encodePredMode          ( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD = false );
  Void encodePartSize          ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, Bool bRD = false );
  Void encodeIPCMInfo          ( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD = false );
  Void encodePredInfo          ( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD = false );
  Void encodeIntraDirModeLuma  ( TComDataCU* pcCU, UInt absPartIdx, Bool isMultiplePU = false );
  
  Void encodeIntraDirModeChroma( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD = false );
  
  Void encodeTransformSubdivFlag( UInt uiSymbol, UInt uiCtx );
  Void encodeQtCbf             ( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth );
  Void encodeQtCbfZero         ( TComDataCU* pcCU, TextType eType, UInt uiTrDepth );
  Void encodeQtRootCbfZero     ( TComDataCU* pcCU );
  Void encodeQtRootCbf         ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void encodeQP                ( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD = false );
  Void updateContextTables     ( SliceType eSliceType, Int iQp, Bool bExecuteFinish )   { m_pcEntropyCoderIf->updateContextTables( eSliceType, iQp, bExecuteFinish );     }
  Void updateContextTables     ( SliceType eSliceType, Int iQp )                        { m_pcEntropyCoderIf->updateContextTables( eSliceType, iQp, true );               }

  Void encodeScalingList       ( TComScalingList* scalingList );

private:
  Void xEncodeTransform        ( TComDataCU* pcCU,UInt offsetLumaOffset, UInt offsetChroma, UInt uiAbsPartIdx, UInt uiDepth, UInt width, UInt height, UInt uiTrIdx, Bool& bCodeDQP );
public:
  Void encodeCoeff             ( TComDataCU* pcCU,                 UInt uiAbsPartIdx, UInt uiDepth, UInt uiWidth, UInt uiHeight, Bool& bCodeDQP );
  
  Void encodeCoeffNxN         ( TComDataCU* pcCU, TCoeff* pcCoeff, UInt uiAbsPartIdx, UInt uiTrWidth, UInt uiTrHeight, UInt uiDepth, TextType eType );
  
  Void estimateBit             ( estBitsSbacStruct* pcEstBitsSbac, Int width, Int height, TextType eTType);
  Void    encodeSaoOffset(SaoLcuParam* saoLcuParam, UInt compIdx);
  Void    encodeSaoUnitInterleaving(Int compIdx, Bool saoFlag, Int rx, Int ry, SaoLcuParam* saoLcuParam, Int cuAddrInSlice, Int cuAddrUpInSlice, Int allowMergeLeft, Int allowMergeUp);
  static Int countNonZeroCoeffs( TCoeff* pcCoef, UInt uiSize );

};// END CLASS DEFINITION TEncEntropy

//! \}

#endif // __TENCENTROPY__

