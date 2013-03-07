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

/** \file     TComSampleAdaptiveOffset.h
    \brief    sample adaptive offset class (header)
*/

#ifndef __TCOMSAMPLEADAPTIVEOFFSET__
#define __TCOMSAMPLEADAPTIVEOFFSET__

#include "CommonDef.h"
#include "TComPic.h"

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Constants
// ====================================================================================================================

#define SAO_MAX_DEPTH                 4
#define SAO_BO_BITS                   5
#define LUMA_GROUP_NUM                (1<<SAO_BO_BITS)
#define MAX_NUM_SAO_OFFSETS           4
#define MAX_NUM_SAO_CLASS             33
// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// Sample Adaptive Offset class
class TComSampleAdaptiveOffset
{
protected:
  TComPic*          m_pcPic;

  static const UInt m_uiMaxDepth;
  static const Int m_aiNumCulPartsLevel[5];
  static const UInt m_auiEoTable[9];
  Int *m_iOffsetBo;
  Int *m_iChromaOffsetBo;
  Int m_iOffsetEo[LUMA_GROUP_NUM];

  Int  m_iPicWidth;
  Int  m_iPicHeight;
  UInt m_uiMaxSplitLevel;
  UInt m_uiMaxCUWidth;
  UInt m_uiMaxCUHeight;
  Int  m_iNumCuInWidth;
  Int  m_iNumCuInHeight;
  Int  m_iNumTotalParts;
  static const Int m_iNumClass[MAX_NUM_SAO_TYPE];

  UInt m_uiSaoBitIncreaseY;
  UInt m_uiSaoBitIncreaseC;  //for chroma
  UInt m_uiQP;

  Pel   *m_pClipTable;
  Pel   *m_pClipTableBase;
  Pel   *m_lumaTableBo;
  Pel   *m_pChromaClipTable;
  Pel   *m_pChromaClipTableBase;
  Pel   *m_chromaTableBo;
  Int   *m_iUpBuff1;
  Int   *m_iUpBuff2;
  Int   *m_iUpBufft;
  Int   *ipSwap;
  Bool  m_bUseNIF;       //!< true for performing non-cross slice boundary ALF
  TComPicYuv* m_pcYuvTmp;    //!< temporary picture buffer pointer when non-across slice/tile boundary SAO is enabled

  Pel* m_pTmpU1;
  Pel* m_pTmpU2;
  Pel* m_pTmpL1;
  Pel* m_pTmpL2;
  Int     m_maxNumOffsetsPerPic;
  Bool    m_saoLcuBoundary;
  Bool    m_saoLcuBasedOptimization;

  Void xPCMRestoration        (TComPic* pcPic);
  Void xPCMCURestoration      (TComDataCU* pcCU, UInt uiAbsZorderIdx, UInt uiDepth);
  Void xPCMSampleRestoration  (TComDataCU* pcCU, UInt uiAbsZorderIdx, UInt uiDepth, TextType ttText);
public:
  TComSampleAdaptiveOffset         ();
  virtual ~TComSampleAdaptiveOffset();

  Void create( UInt uiSourceWidth, UInt uiSourceHeight, UInt uiMaxCUWidth, UInt uiMaxCUHeight );
  Void destroy ();

  Int  convertLevelRowCol2Idx(Int level, Int row, Int col);

  Void initSAOParam   (SAOParam *pcSaoParam, Int iPartLevel, Int iPartRow, Int iPartCol, Int iParentPartIdx, Int StartCUX, Int EndCUX, Int StartCUY, Int EndCUY, Int iYCbCr);
  Void allocSaoParam  (SAOParam* pcSaoParam);
  Void resetSAOParam  (SAOParam *pcSaoParam);
  static Void freeSaoParam   (SAOParam *pcSaoParam);
  
  Void SAOProcess(SAOParam* pcSaoParam);
  Void processSaoCu(Int iAddr, Int iSaoType, Int iYCbCr);
  Pel* getPicYuvAddr(TComPicYuv* pcPicYuv, Int iYCbCr,Int iAddr = 0);

  Void processSaoCuOrg(Int iAddr, Int iPartIdx, Int iYCbCr);  //!< LCU-basd SAO process without slice granularity 
  Void createPicSaoInfo(TComPic* pcPic);
  Void destroyPicSaoInfo();
  Void processSaoBlock(Pel* pDec, Pel* pRest, Int stride, Int iSaoType, UInt width, UInt height, Bool* pbBorderAvail, Int iYCbCr);

  Void resetLcuPart(SaoLcuParam* saoLcuParam);
  Void convertQT2SaoUnit(SAOParam* saoParam, UInt partIdx, Int yCbCr);
  Void convertOnePart2SaoUnit(SAOParam *saoParam, UInt partIdx, Int yCbCr);
  Void processSaoUnitAll(SaoLcuParam* saoLcuParam, Bool oneUnitFlag, Int yCbCr);
  Void setSaoLcuBoundary (Bool bVal)  {m_saoLcuBoundary = bVal;}
  Bool getSaoLcuBoundary ()           {return m_saoLcuBoundary;}
  Void setSaoLcuBasedOptimization (Bool bVal)  {m_saoLcuBasedOptimization = bVal;}
  Bool getSaoLcuBasedOptimization ()           {return m_saoLcuBasedOptimization;}
  Void resetSaoUnit(SaoLcuParam* saoUnit);
  Void copySaoUnit(SaoLcuParam* saoUnitDst, SaoLcuParam* saoUnitSrc );
  Void PCMLFDisableProcess    ( TComPic* pcPic);                        ///< interface function for ALF process 
};

//! \}
#endif

