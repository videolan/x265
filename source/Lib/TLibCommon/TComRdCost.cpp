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

/** \file     TComRdCost.cpp
    \brief    RD cost computation class
*/

#include <math.h>
#include <assert.h>
#include "TComRom.h"
#include "TComRdCost.h"

//! \ingroup TLibCommon
//! \{

TComRdCost::TComRdCost()
{
  init();
}

TComRdCost::~TComRdCost()
{
#if !FIX203
  xUninit();
#endif
}

// Calculate RD functions
Double TComRdCost::calcRdCost( UInt uiBits, UInt uiDistortion, Bool bFlag, DFunc eDFunc )
{
  Double dRdCost = 0.0;
  Double dLambda = 0.0;
  
  switch ( eDFunc )
  {
    case DF_SSE:
      assert(0);
      break;
    case DF_SAD:
      dLambda = (Double)m_uiLambdaMotionSAD;
      break;
    case DF_DEFAULT:
      dLambda =         m_dLambda;
      break;
    case DF_SSE_FRAME:
      dLambda =         m_dFrameLambda;
      break;
    default:
      assert (0);
      break;
  }
  
  if (bFlag)
  {
    // Intra8x8, Intra4x4 Block only...
#if SEQUENCE_LEVEL_LOSSLESS
    dRdCost = (Double)(uiBits);
#else
    dRdCost = (((Double)uiDistortion) + ((Double)uiBits * dLambda));
#endif
  }
  else
  {
    if (eDFunc == DF_SAD)
    {
      dRdCost = ((Double)uiDistortion + (Double)((Int)(uiBits * dLambda+.5)>>16));
      dRdCost = (Double)(UInt)floor(dRdCost);
    }
    else
    {
#if SEQUENCE_LEVEL_LOSSLESS
      dRdCost = (Double)(uiBits);
#else
      dRdCost = ((Double)uiDistortion + (Double)((Int)(uiBits * dLambda+.5)));
      dRdCost = (Double)(UInt)floor(dRdCost);
#endif
    }
  }
  
  return dRdCost;
}

Double TComRdCost::calcRdCost64( UInt64 uiBits, UInt64 uiDistortion, Bool bFlag, DFunc eDFunc )
{
  Double dRdCost = 0.0;
  Double dLambda = 0.0;
  
  switch ( eDFunc )
  {
    case DF_SSE:
      assert(0);
      break;
    case DF_SAD:
      dLambda = (Double)m_uiLambdaMotionSAD;
      break;
    case DF_DEFAULT:
      dLambda =         m_dLambda;
      break;
    case DF_SSE_FRAME:
      dLambda =         m_dFrameLambda;
      break;
    default:
      assert (0);
      break;
  }
  
  if (bFlag)
  {
    // Intra8x8, Intra4x4 Block only...
#if SEQUENCE_LEVEL_LOSSLESS
    dRdCost = (Double)(uiBits);
#else
    dRdCost = (((Double)(Int64)uiDistortion) + ((Double)(Int64)uiBits * dLambda));
#endif
  }
  else
  {
    if (eDFunc == DF_SAD)
    {
      dRdCost = ((Double)(Int64)uiDistortion + (Double)((Int)((Int64)uiBits * dLambda+.5)>>16));
      dRdCost = (Double)(UInt)floor(dRdCost);
    }
    else
    {
#if SEQUENCE_LEVEL_LOSSLESS
      dRdCost = (Double)(uiBits);
#else
      dRdCost = ((Double)(Int64)uiDistortion + (Double)((Int)((Int64)uiBits * dLambda+.5)));
      dRdCost = (Double)(UInt)floor(dRdCost);
#endif
    }
  }
  
  return dRdCost;
}

Void TComRdCost::setLambda( Double dLambda )
{
  m_dLambda           = dLambda;
  m_sqrtLambda        = sqrt(m_dLambda);
  m_uiLambdaMotionSAD = (UInt)floor(65536.0 * m_sqrtLambda);
  m_uiLambdaMotionSSE = (UInt)floor(65536.0 * m_dLambda   );
}


// Initalize Function Pointer by [eDFunc]
Void TComRdCost::init()
{
  m_afpDistortFunc[0]  = NULL;                  // for DF_DEFAULT
  
  m_afpDistortFunc[1]  = TComRdCost::xGetSSE;
  m_afpDistortFunc[2]  = TComRdCost::xGetSSE4;
  m_afpDistortFunc[3]  = TComRdCost::xGetSSE8;
  m_afpDistortFunc[4]  = TComRdCost::xGetSSE16;
  m_afpDistortFunc[5]  = TComRdCost::xGetSSE32;
  m_afpDistortFunc[6]  = TComRdCost::xGetSSE64;
  m_afpDistortFunc[7]  = TComRdCost::xGetSSE16N;
  
  m_afpDistortFunc[8]  = TComRdCost::xGetSAD;
  m_afpDistortFunc[9]  = TComRdCost::xGetSAD4;
  m_afpDistortFunc[10] = TComRdCost::xGetSAD8;
  m_afpDistortFunc[11] = TComRdCost::xGetSAD16;
  m_afpDistortFunc[12] = TComRdCost::xGetSAD32;
  m_afpDistortFunc[13] = TComRdCost::xGetSAD64;
  m_afpDistortFunc[14] = TComRdCost::xGetSAD16N;
  
  m_afpDistortFunc[15] = TComRdCost::xGetSAD;
  m_afpDistortFunc[16] = TComRdCost::xGetSAD4;
  m_afpDistortFunc[17] = TComRdCost::xGetSAD8;
  m_afpDistortFunc[18] = TComRdCost::xGetSAD16;
  m_afpDistortFunc[19] = TComRdCost::xGetSAD32;
  m_afpDistortFunc[20] = TComRdCost::xGetSAD64;
  m_afpDistortFunc[21] = TComRdCost::xGetSAD16N;
  
#if AMP_SAD
  m_afpDistortFunc[43] = TComRdCost::xGetSAD12;
  m_afpDistortFunc[44] = TComRdCost::xGetSAD24;
  m_afpDistortFunc[45] = TComRdCost::xGetSAD48;

  m_afpDistortFunc[46] = TComRdCost::xGetSAD12;
  m_afpDistortFunc[47] = TComRdCost::xGetSAD24;
  m_afpDistortFunc[48] = TComRdCost::xGetSAD48;
#endif
  m_afpDistortFunc[22] = TComRdCost::xGetHADs;
  m_afpDistortFunc[23] = TComRdCost::xGetHADs;
  m_afpDistortFunc[24] = TComRdCost::xGetHADs;
  m_afpDistortFunc[25] = TComRdCost::xGetHADs;
  m_afpDistortFunc[26] = TComRdCost::xGetHADs;
  m_afpDistortFunc[27] = TComRdCost::xGetHADs;
  m_afpDistortFunc[28] = TComRdCost::xGetHADs;
  
#if !FIX203
  m_puiComponentCostOriginP = NULL;
  m_puiComponentCost        = NULL;
  m_puiVerCost              = NULL;
  m_puiHorCost              = NULL;
#endif
  m_uiCost                  = 0;
  m_iCostScale              = 0;
#if !FIX203
  m_iSearchLimit            = 0xdeaddead;
#endif
}

#if !FIX203
Void TComRdCost::initRateDistortionModel( Int iSubPelSearchLimit )
{
  // make it larger
  iSubPelSearchLimit += 4;
  iSubPelSearchLimit *= 8;
  
  if( m_iSearchLimit != iSubPelSearchLimit )
  {
    xUninit();
    
    m_iSearchLimit = iSubPelSearchLimit;
    
    m_puiComponentCostOriginP = new UInt[ 4 * iSubPelSearchLimit ];
    iSubPelSearchLimit *= 2;
    
    m_puiComponentCost = m_puiComponentCostOriginP + iSubPelSearchLimit;
    
    for( Int n = -iSubPelSearchLimit; n < iSubPelSearchLimit; n++)
    {
      m_puiComponentCost[n] = xGetComponentBits( n );
    }
  }
}

Void TComRdCost::xUninit()
{
  if( NULL != m_puiComponentCostOriginP )
  {
    delete [] m_puiComponentCostOriginP;
    m_puiComponentCostOriginP = NULL;
  }
}
#endif

UInt TComRdCost::xGetComponentBits( Int iVal )
{
  UInt uiLength = 1;
  UInt uiTemp   = ( iVal <= 0) ? (-iVal<<1)+1: (iVal<<1);
  
  assert ( uiTemp );
  
  while ( 1 != uiTemp )
  {
    uiTemp >>= 1;
    uiLength += 2;
  }
  
  return uiLength;
}

Void TComRdCost::setDistParam( UInt uiBlkWidth, UInt uiBlkHeight, DFunc eDFunc, DistParam& rcDistParam )
{
  // set Block Width / Height
  rcDistParam.iCols    = uiBlkWidth;
  rcDistParam.iRows    = uiBlkHeight;
  rcDistParam.DistFunc = m_afpDistortFunc[eDFunc + g_aucConvertToBit[ rcDistParam.iCols ] + 1 ];
  
  // initialize
  rcDistParam.iSubShift  = 0;
}

// Setting the Distortion Parameter for Inter (ME)
Void TComRdCost::setDistParam( TComPattern* pcPatternKey, Pel* piRefY, Int iRefStride, DistParam& rcDistParam )
{
  // set Original & Curr Pointer / Stride
  rcDistParam.pOrg = pcPatternKey->getROIY();
  rcDistParam.pCur = piRefY;
  
  rcDistParam.iStrideOrg = pcPatternKey->getPatternLStride();
  rcDistParam.iStrideCur = iRefStride;
  
  // set Block Width / Height
  rcDistParam.iCols    = pcPatternKey->getROIYWidth();
  rcDistParam.iRows    = pcPatternKey->getROIYHeight();
  rcDistParam.DistFunc = m_afpDistortFunc[DF_SAD + g_aucConvertToBit[ rcDistParam.iCols ] + 1 ];
  
#if AMP_SAD
  if (rcDistParam.iCols == 12)
  {
    rcDistParam.DistFunc = m_afpDistortFunc[43 ];
  }
  else if (rcDistParam.iCols == 24)
  {
    rcDistParam.DistFunc = m_afpDistortFunc[44 ];
  }
  else if (rcDistParam.iCols == 48)
  {
    rcDistParam.DistFunc = m_afpDistortFunc[45 ];
  }
#endif

  // initialize
  rcDistParam.iSubShift  = 0;
}

// Setting the Distortion Parameter for Inter (subpel ME with step)
#if NS_HAD
Void TComRdCost::setDistParam( TComPattern* pcPatternKey, Pel* piRefY, Int iRefStride, Int iStep, DistParam& rcDistParam, Bool bHADME, Bool bUseNSHAD )
#else
Void TComRdCost::setDistParam( TComPattern* pcPatternKey, Pel* piRefY, Int iRefStride, Int iStep, DistParam& rcDistParam, Bool bHADME )
#endif
{
  // set Original & Curr Pointer / Stride
  rcDistParam.pOrg = pcPatternKey->getROIY();
  rcDistParam.pCur = piRefY;
  
  rcDistParam.iStrideOrg = pcPatternKey->getPatternLStride();
  rcDistParam.iStrideCur = iRefStride * iStep;
  
  // set Step for interpolated buffer
  rcDistParam.iStep = iStep;
  
  // set Block Width / Height
  rcDistParam.iCols    = pcPatternKey->getROIYWidth();
  rcDistParam.iRows    = pcPatternKey->getROIYHeight();
#if NS_HAD
  rcDistParam.bUseNSHAD = bUseNSHAD;
#endif
  
  // set distortion function
  if ( !bHADME )
  {
    rcDistParam.DistFunc = m_afpDistortFunc[DF_SADS + g_aucConvertToBit[ rcDistParam.iCols ] + 1 ];
#if AMP_SAD
    if (rcDistParam.iCols == 12)
    {
      rcDistParam.DistFunc = m_afpDistortFunc[46 ];
    }
    else if (rcDistParam.iCols == 24)
    {
      rcDistParam.DistFunc = m_afpDistortFunc[47 ];
    }
    else if (rcDistParam.iCols == 48)
    {
      rcDistParam.DistFunc = m_afpDistortFunc[48 ];
    }
#endif
  }
  else
  {
    rcDistParam.DistFunc = m_afpDistortFunc[DF_HADS + g_aucConvertToBit[ rcDistParam.iCols ] + 1 ];
  }
  
  // initialize
  rcDistParam.iSubShift  = 0;
}

Void
#if NS_HAD
TComRdCost::setDistParam( DistParam& rcDP, Pel* p1, Int iStride1, Pel* p2, Int iStride2, Int iWidth, Int iHeight, Bool bHadamard, Bool bUseNSHAD )
#else
TComRdCost::setDistParam( DistParam& rcDP, Int bitDepth, Pel* p1, Int iStride1, Pel* p2, Int iStride2, Int iWidth, Int iHeight, Bool bHadamard )
#endif
{
  rcDP.pOrg       = p1;
  rcDP.pCur       = p2;
  rcDP.iStrideOrg = iStride1;
  rcDP.iStrideCur = iStride2;
  rcDP.iCols      = iWidth;
  rcDP.iRows      = iHeight;
  rcDP.iStep      = 1;
  rcDP.iSubShift  = 0;
  rcDP.bitDepth   = bitDepth;
  rcDP.DistFunc   = m_afpDistortFunc[ ( bHadamard ? DF_HADS : DF_SADS ) + g_aucConvertToBit[ iWidth ] + 1 ];
#if NS_HAD
  rcDP.bUseNSHAD  = bUseNSHAD;
#endif
}

UInt TComRdCost::calcHAD(Int bitDepth, Pel* pi0, Int iStride0, Pel* pi1, Int iStride1, Int iWidth, Int iHeight )
{
  UInt uiSum = 0;
  Int x, y;
  
  if ( ( (iWidth % 8) == 0 ) && ( (iHeight % 8) == 0 ) )
  {
    for ( y=0; y<iHeight; y+= 8 )
    {
      for ( x=0; x<iWidth; x+= 8 )
      {
        uiSum += xCalcHADs8x8( &pi0[x], &pi1[x], iStride0, iStride1, 1 );
      }
      pi0 += iStride0*8;
      pi1 += iStride1*8;
    }
  }
  else if ( ( (iWidth % 4) == 0 ) && ( (iHeight % 4) == 0 ) )
  {
    for ( y=0; y<iHeight; y+= 4 )
    {
      for ( x=0; x<iWidth; x+= 4 )
      {
        uiSum += xCalcHADs4x4( &pi0[x], &pi1[x], iStride0, iStride1, 1 );
      }
      pi0 += iStride0*4;
      pi1 += iStride1*4;
    }
  }
  else
  {
    for ( y=0; y<iHeight; y+= 2 )
    {
      for ( x=0; x<iWidth; x+= 2 )
      {
        uiSum += xCalcHADs8x8( &pi0[x], &pi1[x], iStride0, iStride1, 1 );
      }
      pi0 += iStride0*2;
      pi1 += iStride1*2;
    }
  }
  
  return uiSum >> DISTORTION_PRECISION_ADJUSTMENT(bitDepth-8);

}

#if WEIGHTED_CHROMA_DISTORTION
UInt TComRdCost::getDistPart(Int bitDepth, Pel* piCur, Int iCurStride,  Pel* piOrg, Int iOrgStride, UInt uiBlkWidth, UInt uiBlkHeight, TextType eText, DFunc eDFunc)
#else
UInt TComRdCost::getDistPart(Int bitDepth, Pel* piCur, Int iCurStride,  Pel* piOrg, Int iOrgStride, UInt uiBlkWidth, UInt uiBlkHeight, DFunc eDFunc )
#endif
{
  DistParam cDtParam;
  setDistParam( uiBlkWidth, uiBlkHeight, eDFunc, cDtParam );
  cDtParam.pOrg       = piOrg;
  cDtParam.pCur       = piCur;
  cDtParam.iStrideOrg = iOrgStride;
  cDtParam.iStrideCur = iCurStride;
  cDtParam.iStep      = 1;

  cDtParam.bApplyWeight = false;
  cDtParam.uiComp       = 255;    // just for assert: to be sure it was set before use, since only values 0,1 or 2 are allowed.
  cDtParam.bitDepth = bitDepth;

#if WEIGHTED_CHROMA_DISTORTION
  if (eText == TEXT_CHROMA_U)
  {
   return ((Int) (m_cbDistortionWeight * cDtParam.DistFunc( &cDtParam )));
  }
  else if (eText == TEXT_CHROMA_V)
  {
   return ((Int) (m_crDistortionWeight * cDtParam.DistFunc( &cDtParam )));
  }
  else
  {
    return cDtParam.DistFunc( &cDtParam );
  }
#else
  return cDtParam.DistFunc( &cDtParam );
#endif
}

#if RATE_CONTROL_LAMBDA_DOMAIN
UInt TComRdCost::getSADPart ( Int bitDepth, Pel* pelCur, Int curStride,  Pel* pelOrg, Int orgStride, UInt width, UInt height )
{
  UInt SAD = 0;
  Int shift = DISTORTION_PRECISION_ADJUSTMENT(bitDepth-8);
  for ( Int i=0; i<height; i++ )
  {
    for( Int j=0; j<width; j++ )
    {
      SAD += abs((pelCur[j] - pelOrg[j])) >> shift;
    }
    pelCur = pelCur + curStride;
    pelOrg = pelOrg + orgStride;
  }
  return SAD;
}
#endif

// ====================================================================================================================
// Distortion functions
// ====================================================================================================================

// --------------------------------------------------------------------------------------------------------------------
// SAD
// --------------------------------------------------------------------------------------------------------------------

UInt TComRdCost::xGetSAD( DistParam* pcDtParam )
{
  if ( pcDtParam->bApplyWeight )
  {
    return xGetSADw( pcDtParam );
  }
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iCols   = pcDtParam->iCols;
  Int  iStrideCur = pcDtParam->iStrideCur;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  
  UInt uiSum = 0;
  
  for( ; iRows != 0; iRows-- )
  {
    for (Int n = 0; n < iCols; n++ )
    {
      uiSum += abs( piOrg[n] - piCur[n] );
    }
    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }
  
  return uiSum >> DISTORTION_PRECISION_ADJUSTMENT(pcDtParam->bitDepth-8);
}

UInt TComRdCost::xGetSAD4( DistParam* pcDtParam )
{
  if ( pcDtParam->bApplyWeight ) 
  {
    return xGetSADw( pcDtParam );
  }
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iSubShift  = pcDtParam->iSubShift;
  Int  iSubStep   = ( 1 << iSubShift );
  Int  iStrideCur = pcDtParam->iStrideCur*iSubStep;
  Int  iStrideOrg = pcDtParam->iStrideOrg*iSubStep;
  
  UInt uiSum = 0;
  
  for( ; iRows != 0; iRows-=iSubStep )
  {
    uiSum += abs( piOrg[0] - piCur[0] );
    uiSum += abs( piOrg[1] - piCur[1] );
    uiSum += abs( piOrg[2] - piCur[2] );
    uiSum += abs( piOrg[3] - piCur[3] );
    
    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }
  
  uiSum <<= iSubShift;
  return uiSum >> DISTORTION_PRECISION_ADJUSTMENT(pcDtParam->bitDepth-8);
}

UInt TComRdCost::xGetSAD8( DistParam* pcDtParam )
{
  if ( pcDtParam->bApplyWeight )
  {
    return xGetSADw( pcDtParam );
  }
  Pel* piOrg      = pcDtParam->pOrg;
  Pel* piCur      = pcDtParam->pCur;
  Int  iRows      = pcDtParam->iRows;
  Int  iSubShift  = pcDtParam->iSubShift;
  Int  iSubStep   = ( 1 << iSubShift );
  Int  iStrideCur = pcDtParam->iStrideCur*iSubStep;
  Int  iStrideOrg = pcDtParam->iStrideOrg*iSubStep;
  
  UInt uiSum = 0;
  
  for( ; iRows != 0; iRows-=iSubStep )
  {
    uiSum += abs( piOrg[0] - piCur[0] );
    uiSum += abs( piOrg[1] - piCur[1] );
    uiSum += abs( piOrg[2] - piCur[2] );
    uiSum += abs( piOrg[3] - piCur[3] );
    uiSum += abs( piOrg[4] - piCur[4] );
    uiSum += abs( piOrg[5] - piCur[5] );
    uiSum += abs( piOrg[6] - piCur[6] );
    uiSum += abs( piOrg[7] - piCur[7] );
    
    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }
  
  uiSum <<= iSubShift;
  return uiSum >> DISTORTION_PRECISION_ADJUSTMENT(pcDtParam->bitDepth-8);
}

UInt TComRdCost::xGetSAD16( DistParam* pcDtParam )
{
  if ( pcDtParam->bApplyWeight )
  {
    return xGetSADw( pcDtParam );
  }
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iSubShift  = pcDtParam->iSubShift;
  Int  iSubStep   = ( 1 << iSubShift );
  Int  iStrideCur = pcDtParam->iStrideCur*iSubStep;
  Int  iStrideOrg = pcDtParam->iStrideOrg*iSubStep;
  
  UInt uiSum = 0;
  
  for( ; iRows != 0; iRows-=iSubStep )
  {
    uiSum += abs( piOrg[0] - piCur[0] );
    uiSum += abs( piOrg[1] - piCur[1] );
    uiSum += abs( piOrg[2] - piCur[2] );
    uiSum += abs( piOrg[3] - piCur[3] );
    uiSum += abs( piOrg[4] - piCur[4] );
    uiSum += abs( piOrg[5] - piCur[5] );
    uiSum += abs( piOrg[6] - piCur[6] );
    uiSum += abs( piOrg[7] - piCur[7] );
    uiSum += abs( piOrg[8] - piCur[8] );
    uiSum += abs( piOrg[9] - piCur[9] );
    uiSum += abs( piOrg[10] - piCur[10] );
    uiSum += abs( piOrg[11] - piCur[11] );
    uiSum += abs( piOrg[12] - piCur[12] );
    uiSum += abs( piOrg[13] - piCur[13] );
    uiSum += abs( piOrg[14] - piCur[14] );
    uiSum += abs( piOrg[15] - piCur[15] );
    
    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }
  
  uiSum <<= iSubShift;
  return uiSum >> DISTORTION_PRECISION_ADJUSTMENT(pcDtParam->bitDepth-8);
}

#if AMP_SAD
UInt TComRdCost::xGetSAD12( DistParam* pcDtParam )
{
  if ( pcDtParam->bApplyWeight )
  {
    return xGetSADw( pcDtParam );
  }
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iSubShift  = pcDtParam->iSubShift;
  Int  iSubStep   = ( 1 << iSubShift );
  Int  iStrideCur = pcDtParam->iStrideCur*iSubStep;
  Int  iStrideOrg = pcDtParam->iStrideOrg*iSubStep;
  
  UInt uiSum = 0;
  
  for( ; iRows != 0; iRows-=iSubStep )
  {
    uiSum += abs( piOrg[0] - piCur[0] );
    uiSum += abs( piOrg[1] - piCur[1] );
    uiSum += abs( piOrg[2] - piCur[2] );
    uiSum += abs( piOrg[3] - piCur[3] );
    uiSum += abs( piOrg[4] - piCur[4] );
    uiSum += abs( piOrg[5] - piCur[5] );
    uiSum += abs( piOrg[6] - piCur[6] );
    uiSum += abs( piOrg[7] - piCur[7] );
    uiSum += abs( piOrg[8] - piCur[8] );
    uiSum += abs( piOrg[9] - piCur[9] );
    uiSum += abs( piOrg[10] - piCur[10] );
    uiSum += abs( piOrg[11] - piCur[11] );
    
    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }
  
  uiSum <<= iSubShift;
  return uiSum >> DISTORTION_PRECISION_ADJUSTMENT(pcDtParam->bitDepth-8);
}
#endif

UInt TComRdCost::xGetSAD16N( DistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iCols   = pcDtParam->iCols;
  Int  iSubShift  = pcDtParam->iSubShift;
  Int  iSubStep   = ( 1 << iSubShift );
  Int  iStrideCur = pcDtParam->iStrideCur*iSubStep;
  Int  iStrideOrg = pcDtParam->iStrideOrg*iSubStep;
  
  UInt uiSum = 0;
  
  for( ; iRows != 0; iRows-=iSubStep )
  {
    for (Int n = 0; n < iCols; n+=16 )
    {
      uiSum += abs( piOrg[n+ 0] - piCur[n+ 0] );
      uiSum += abs( piOrg[n+ 1] - piCur[n+ 1] );
      uiSum += abs( piOrg[n+ 2] - piCur[n+ 2] );
      uiSum += abs( piOrg[n+ 3] - piCur[n+ 3] );
      uiSum += abs( piOrg[n+ 4] - piCur[n+ 4] );
      uiSum += abs( piOrg[n+ 5] - piCur[n+ 5] );
      uiSum += abs( piOrg[n+ 6] - piCur[n+ 6] );
      uiSum += abs( piOrg[n+ 7] - piCur[n+ 7] );
      uiSum += abs( piOrg[n+ 8] - piCur[n+ 8] );
      uiSum += abs( piOrg[n+ 9] - piCur[n+ 9] );
      uiSum += abs( piOrg[n+10] - piCur[n+10] );
      uiSum += abs( piOrg[n+11] - piCur[n+11] );
      uiSum += abs( piOrg[n+12] - piCur[n+12] );
      uiSum += abs( piOrg[n+13] - piCur[n+13] );
      uiSum += abs( piOrg[n+14] - piCur[n+14] );
      uiSum += abs( piOrg[n+15] - piCur[n+15] );
    }
    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }
  
  uiSum <<= iSubShift;
  return uiSum >> DISTORTION_PRECISION_ADJUSTMENT(pcDtParam->bitDepth-8);
}

UInt TComRdCost::xGetSAD32( DistParam* pcDtParam )
{
  if ( pcDtParam->bApplyWeight )
  {
    return xGetSADw( pcDtParam );
  }
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iSubShift  = pcDtParam->iSubShift;
  Int  iSubStep   = ( 1 << iSubShift );
  Int  iStrideCur = pcDtParam->iStrideCur*iSubStep;
  Int  iStrideOrg = pcDtParam->iStrideOrg*iSubStep;
  
  UInt uiSum = 0;
  
  for( ; iRows != 0; iRows-=iSubStep )
  {
    uiSum += abs( piOrg[0] - piCur[0] );
    uiSum += abs( piOrg[1] - piCur[1] );
    uiSum += abs( piOrg[2] - piCur[2] );
    uiSum += abs( piOrg[3] - piCur[3] );
    uiSum += abs( piOrg[4] - piCur[4] );
    uiSum += abs( piOrg[5] - piCur[5] );
    uiSum += abs( piOrg[6] - piCur[6] );
    uiSum += abs( piOrg[7] - piCur[7] );
    uiSum += abs( piOrg[8] - piCur[8] );
    uiSum += abs( piOrg[9] - piCur[9] );
    uiSum += abs( piOrg[10] - piCur[10] );
    uiSum += abs( piOrg[11] - piCur[11] );
    uiSum += abs( piOrg[12] - piCur[12] );
    uiSum += abs( piOrg[13] - piCur[13] );
    uiSum += abs( piOrg[14] - piCur[14] );
    uiSum += abs( piOrg[15] - piCur[15] );
    uiSum += abs( piOrg[16] - piCur[16] );
    uiSum += abs( piOrg[17] - piCur[17] );
    uiSum += abs( piOrg[18] - piCur[18] );
    uiSum += abs( piOrg[19] - piCur[19] );
    uiSum += abs( piOrg[20] - piCur[20] );
    uiSum += abs( piOrg[21] - piCur[21] );
    uiSum += abs( piOrg[22] - piCur[22] );
    uiSum += abs( piOrg[23] - piCur[23] );
    uiSum += abs( piOrg[24] - piCur[24] );
    uiSum += abs( piOrg[25] - piCur[25] );
    uiSum += abs( piOrg[26] - piCur[26] );
    uiSum += abs( piOrg[27] - piCur[27] );
    uiSum += abs( piOrg[28] - piCur[28] );
    uiSum += abs( piOrg[29] - piCur[29] );
    uiSum += abs( piOrg[30] - piCur[30] );
    uiSum += abs( piOrg[31] - piCur[31] );
    
    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }
  
  uiSum <<= iSubShift;
  return uiSum >> DISTORTION_PRECISION_ADJUSTMENT(pcDtParam->bitDepth-8);
}

#if AMP_SAD
UInt TComRdCost::xGetSAD24( DistParam* pcDtParam )
{
  if ( pcDtParam->bApplyWeight )
  {
    return xGetSADw( pcDtParam );
  }
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iSubShift  = pcDtParam->iSubShift;
  Int  iSubStep   = ( 1 << iSubShift );
  Int  iStrideCur = pcDtParam->iStrideCur*iSubStep;
  Int  iStrideOrg = pcDtParam->iStrideOrg*iSubStep;
  
  UInt uiSum = 0;
  
  for( ; iRows != 0; iRows-=iSubStep )
  {
    uiSum += abs( piOrg[0] - piCur[0] );
    uiSum += abs( piOrg[1] - piCur[1] );
    uiSum += abs( piOrg[2] - piCur[2] );
    uiSum += abs( piOrg[3] - piCur[3] );
    uiSum += abs( piOrg[4] - piCur[4] );
    uiSum += abs( piOrg[5] - piCur[5] );
    uiSum += abs( piOrg[6] - piCur[6] );
    uiSum += abs( piOrg[7] - piCur[7] );
    uiSum += abs( piOrg[8] - piCur[8] );
    uiSum += abs( piOrg[9] - piCur[9] );
    uiSum += abs( piOrg[10] - piCur[10] );
    uiSum += abs( piOrg[11] - piCur[11] );
    uiSum += abs( piOrg[12] - piCur[12] );
    uiSum += abs( piOrg[13] - piCur[13] );
    uiSum += abs( piOrg[14] - piCur[14] );
    uiSum += abs( piOrg[15] - piCur[15] );
    uiSum += abs( piOrg[16] - piCur[16] );
    uiSum += abs( piOrg[17] - piCur[17] );
    uiSum += abs( piOrg[18] - piCur[18] );
    uiSum += abs( piOrg[19] - piCur[19] );
    uiSum += abs( piOrg[20] - piCur[20] );
    uiSum += abs( piOrg[21] - piCur[21] );
    uiSum += abs( piOrg[22] - piCur[22] );
    uiSum += abs( piOrg[23] - piCur[23] );
    
    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }
  
  uiSum <<= iSubShift;
  return uiSum >> DISTORTION_PRECISION_ADJUSTMENT(pcDtParam->bitDepth-8);
}

#endif

UInt TComRdCost::xGetSAD64( DistParam* pcDtParam )
{
  if ( pcDtParam->bApplyWeight )
  {
    return xGetSADw( pcDtParam );
  }
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iSubShift  = pcDtParam->iSubShift;
  Int  iSubStep   = ( 1 << iSubShift );
  Int  iStrideCur = pcDtParam->iStrideCur*iSubStep;
  Int  iStrideOrg = pcDtParam->iStrideOrg*iSubStep;
  
  UInt uiSum = 0;
  
  for( ; iRows != 0; iRows-=iSubStep )
  {
    uiSum += abs( piOrg[0] - piCur[0] );
    uiSum += abs( piOrg[1] - piCur[1] );
    uiSum += abs( piOrg[2] - piCur[2] );
    uiSum += abs( piOrg[3] - piCur[3] );
    uiSum += abs( piOrg[4] - piCur[4] );
    uiSum += abs( piOrg[5] - piCur[5] );
    uiSum += abs( piOrg[6] - piCur[6] );
    uiSum += abs( piOrg[7] - piCur[7] );
    uiSum += abs( piOrg[8] - piCur[8] );
    uiSum += abs( piOrg[9] - piCur[9] );
    uiSum += abs( piOrg[10] - piCur[10] );
    uiSum += abs( piOrg[11] - piCur[11] );
    uiSum += abs( piOrg[12] - piCur[12] );
    uiSum += abs( piOrg[13] - piCur[13] );
    uiSum += abs( piOrg[14] - piCur[14] );
    uiSum += abs( piOrg[15] - piCur[15] );
    uiSum += abs( piOrg[16] - piCur[16] );
    uiSum += abs( piOrg[17] - piCur[17] );
    uiSum += abs( piOrg[18] - piCur[18] );
    uiSum += abs( piOrg[19] - piCur[19] );
    uiSum += abs( piOrg[20] - piCur[20] );
    uiSum += abs( piOrg[21] - piCur[21] );
    uiSum += abs( piOrg[22] - piCur[22] );
    uiSum += abs( piOrg[23] - piCur[23] );
    uiSum += abs( piOrg[24] - piCur[24] );
    uiSum += abs( piOrg[25] - piCur[25] );
    uiSum += abs( piOrg[26] - piCur[26] );
    uiSum += abs( piOrg[27] - piCur[27] );
    uiSum += abs( piOrg[28] - piCur[28] );
    uiSum += abs( piOrg[29] - piCur[29] );
    uiSum += abs( piOrg[30] - piCur[30] );
    uiSum += abs( piOrg[31] - piCur[31] );
    uiSum += abs( piOrg[32] - piCur[32] );
    uiSum += abs( piOrg[33] - piCur[33] );
    uiSum += abs( piOrg[34] - piCur[34] );
    uiSum += abs( piOrg[35] - piCur[35] );
    uiSum += abs( piOrg[36] - piCur[36] );
    uiSum += abs( piOrg[37] - piCur[37] );
    uiSum += abs( piOrg[38] - piCur[38] );
    uiSum += abs( piOrg[39] - piCur[39] );
    uiSum += abs( piOrg[40] - piCur[40] );
    uiSum += abs( piOrg[41] - piCur[41] );
    uiSum += abs( piOrg[42] - piCur[42] );
    uiSum += abs( piOrg[43] - piCur[43] );
    uiSum += abs( piOrg[44] - piCur[44] );
    uiSum += abs( piOrg[45] - piCur[45] );
    uiSum += abs( piOrg[46] - piCur[46] );
    uiSum += abs( piOrg[47] - piCur[47] );
    uiSum += abs( piOrg[48] - piCur[48] );
    uiSum += abs( piOrg[49] - piCur[49] );
    uiSum += abs( piOrg[50] - piCur[50] );
    uiSum += abs( piOrg[51] - piCur[51] );
    uiSum += abs( piOrg[52] - piCur[52] );
    uiSum += abs( piOrg[53] - piCur[53] );
    uiSum += abs( piOrg[54] - piCur[54] );
    uiSum += abs( piOrg[55] - piCur[55] );
    uiSum += abs( piOrg[56] - piCur[56] );
    uiSum += abs( piOrg[57] - piCur[57] );
    uiSum += abs( piOrg[58] - piCur[58] );
    uiSum += abs( piOrg[59] - piCur[59] );
    uiSum += abs( piOrg[60] - piCur[60] );
    uiSum += abs( piOrg[61] - piCur[61] );
    uiSum += abs( piOrg[62] - piCur[62] );
    uiSum += abs( piOrg[63] - piCur[63] );
    
    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }
  
  uiSum <<= iSubShift;
  return uiSum >> DISTORTION_PRECISION_ADJUSTMENT(pcDtParam->bitDepth-8);
}

#if AMP_SAD
UInt TComRdCost::xGetSAD48( DistParam* pcDtParam )
{
  if ( pcDtParam->bApplyWeight )
  {
    return xGetSADw( pcDtParam );
  }
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iSubShift  = pcDtParam->iSubShift;
  Int  iSubStep   = ( 1 << iSubShift );
  Int  iStrideCur = pcDtParam->iStrideCur*iSubStep;
  Int  iStrideOrg = pcDtParam->iStrideOrg*iSubStep;
  
  UInt uiSum = 0;
  
  for( ; iRows != 0; iRows-=iSubStep )
  {
    uiSum += abs( piOrg[0] - piCur[0] );
    uiSum += abs( piOrg[1] - piCur[1] );
    uiSum += abs( piOrg[2] - piCur[2] );
    uiSum += abs( piOrg[3] - piCur[3] );
    uiSum += abs( piOrg[4] - piCur[4] );
    uiSum += abs( piOrg[5] - piCur[5] );
    uiSum += abs( piOrg[6] - piCur[6] );
    uiSum += abs( piOrg[7] - piCur[7] );
    uiSum += abs( piOrg[8] - piCur[8] );
    uiSum += abs( piOrg[9] - piCur[9] );
    uiSum += abs( piOrg[10] - piCur[10] );
    uiSum += abs( piOrg[11] - piCur[11] );
    uiSum += abs( piOrg[12] - piCur[12] );
    uiSum += abs( piOrg[13] - piCur[13] );
    uiSum += abs( piOrg[14] - piCur[14] );
    uiSum += abs( piOrg[15] - piCur[15] );
    uiSum += abs( piOrg[16] - piCur[16] );
    uiSum += abs( piOrg[17] - piCur[17] );
    uiSum += abs( piOrg[18] - piCur[18] );
    uiSum += abs( piOrg[19] - piCur[19] );
    uiSum += abs( piOrg[20] - piCur[20] );
    uiSum += abs( piOrg[21] - piCur[21] );
    uiSum += abs( piOrg[22] - piCur[22] );
    uiSum += abs( piOrg[23] - piCur[23] );
    uiSum += abs( piOrg[24] - piCur[24] );
    uiSum += abs( piOrg[25] - piCur[25] );
    uiSum += abs( piOrg[26] - piCur[26] );
    uiSum += abs( piOrg[27] - piCur[27] );
    uiSum += abs( piOrg[28] - piCur[28] );
    uiSum += abs( piOrg[29] - piCur[29] );
    uiSum += abs( piOrg[30] - piCur[30] );
    uiSum += abs( piOrg[31] - piCur[31] );
    uiSum += abs( piOrg[32] - piCur[32] );
    uiSum += abs( piOrg[33] - piCur[33] );
    uiSum += abs( piOrg[34] - piCur[34] );
    uiSum += abs( piOrg[35] - piCur[35] );
    uiSum += abs( piOrg[36] - piCur[36] );
    uiSum += abs( piOrg[37] - piCur[37] );
    uiSum += abs( piOrg[38] - piCur[38] );
    uiSum += abs( piOrg[39] - piCur[39] );
    uiSum += abs( piOrg[40] - piCur[40] );
    uiSum += abs( piOrg[41] - piCur[41] );
    uiSum += abs( piOrg[42] - piCur[42] );
    uiSum += abs( piOrg[43] - piCur[43] );
    uiSum += abs( piOrg[44] - piCur[44] );
    uiSum += abs( piOrg[45] - piCur[45] );
    uiSum += abs( piOrg[46] - piCur[46] );
    uiSum += abs( piOrg[47] - piCur[47] );
    
    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }
  
  uiSum <<= iSubShift;
  return uiSum >> DISTORTION_PRECISION_ADJUSTMENT(pcDtParam->bitDepth-8);
}
#endif

// --------------------------------------------------------------------------------------------------------------------
// SSE
// --------------------------------------------------------------------------------------------------------------------

UInt TComRdCost::xGetSSE( DistParam* pcDtParam )
{
  if ( pcDtParam->bApplyWeight )
  {
    return xGetSSEw( pcDtParam );
  }
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iCols   = pcDtParam->iCols;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  Int  iStrideCur = pcDtParam->iStrideCur;
  
  UInt uiSum = 0;
  UInt uiShift = DISTORTION_PRECISION_ADJUSTMENT((pcDtParam->bitDepth-8) << 1);
  
  Int iTemp;
  
  for( ; iRows != 0; iRows-- )
  {
    for (Int n = 0; n < iCols; n++ )
    {
      iTemp = piOrg[n  ] - piCur[n  ];
      uiSum += ( iTemp * iTemp ) >> uiShift;
    }
    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }
  
  return ( uiSum );
}

UInt TComRdCost::xGetSSE4( DistParam* pcDtParam )
{
  if ( pcDtParam->bApplyWeight )
  {
    assert( pcDtParam->iCols == 4 );
    return xGetSSEw( pcDtParam );
  }
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  Int  iStrideCur = pcDtParam->iStrideCur;
  
  UInt uiSum = 0;
  UInt uiShift = DISTORTION_PRECISION_ADJUSTMENT((pcDtParam->bitDepth-8) << 1);
  
  Int  iTemp;
  
  for( ; iRows != 0; iRows-- )
  {
    
    iTemp = piOrg[0] - piCur[0]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[1] - piCur[1]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[2] - piCur[2]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[3] - piCur[3]; uiSum += ( iTemp * iTemp ) >> uiShift;
    
    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }
  
  return ( uiSum );
}

UInt TComRdCost::xGetSSE8( DistParam* pcDtParam )
{
  if ( pcDtParam->bApplyWeight )
  {
    assert( pcDtParam->iCols == 8 );
    return xGetSSEw( pcDtParam );
  }
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  Int  iStrideCur = pcDtParam->iStrideCur;
  
  UInt uiSum = 0;
  UInt uiShift = DISTORTION_PRECISION_ADJUSTMENT((pcDtParam->bitDepth-8) << 1);
  
  Int  iTemp;
  
  for( ; iRows != 0; iRows-- )
  {
    iTemp = piOrg[0] - piCur[0]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[1] - piCur[1]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[2] - piCur[2]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[3] - piCur[3]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[4] - piCur[4]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[5] - piCur[5]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[6] - piCur[6]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[7] - piCur[7]; uiSum += ( iTemp * iTemp ) >> uiShift;
    
    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }
  
  return ( uiSum );
}

UInt TComRdCost::xGetSSE16( DistParam* pcDtParam )
{
  if ( pcDtParam->bApplyWeight )
  {
    assert( pcDtParam->iCols == 16 );
    return xGetSSEw( pcDtParam );
  }
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  Int  iStrideCur = pcDtParam->iStrideCur;
  
  UInt uiSum = 0;
  UInt uiShift = DISTORTION_PRECISION_ADJUSTMENT((pcDtParam->bitDepth-8) << 1);
  
  Int  iTemp;
  
  for( ; iRows != 0; iRows-- )
  {
    
    iTemp = piOrg[ 0] - piCur[ 0]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 1] - piCur[ 1]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 2] - piCur[ 2]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 3] - piCur[ 3]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 4] - piCur[ 4]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 5] - piCur[ 5]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 6] - piCur[ 6]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 7] - piCur[ 7]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 8] - piCur[ 8]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 9] - piCur[ 9]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[10] - piCur[10]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[11] - piCur[11]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[12] - piCur[12]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[13] - piCur[13]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[14] - piCur[14]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[15] - piCur[15]; uiSum += ( iTemp * iTemp ) >> uiShift;
    
    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }
  
  return ( uiSum );
}

UInt TComRdCost::xGetSSE16N( DistParam* pcDtParam )
{
  if ( pcDtParam->bApplyWeight )
  {
    return xGetSSEw( pcDtParam );
  }
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iCols   = pcDtParam->iCols;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  Int  iStrideCur = pcDtParam->iStrideCur;
  
  UInt uiSum = 0;
  UInt uiShift = DISTORTION_PRECISION_ADJUSTMENT((pcDtParam->bitDepth-8) << 1);
  Int  iTemp;
  
  for( ; iRows != 0; iRows-- )
  {
    for (Int n = 0; n < iCols; n+=16 )
    {
      
      iTemp = piOrg[n+ 0] - piCur[n+ 0]; uiSum += ( iTemp * iTemp ) >> uiShift;
      iTemp = piOrg[n+ 1] - piCur[n+ 1]; uiSum += ( iTemp * iTemp ) >> uiShift;
      iTemp = piOrg[n+ 2] - piCur[n+ 2]; uiSum += ( iTemp * iTemp ) >> uiShift;
      iTemp = piOrg[n+ 3] - piCur[n+ 3]; uiSum += ( iTemp * iTemp ) >> uiShift;
      iTemp = piOrg[n+ 4] - piCur[n+ 4]; uiSum += ( iTemp * iTemp ) >> uiShift;
      iTemp = piOrg[n+ 5] - piCur[n+ 5]; uiSum += ( iTemp * iTemp ) >> uiShift;
      iTemp = piOrg[n+ 6] - piCur[n+ 6]; uiSum += ( iTemp * iTemp ) >> uiShift;
      iTemp = piOrg[n+ 7] - piCur[n+ 7]; uiSum += ( iTemp * iTemp ) >> uiShift;
      iTemp = piOrg[n+ 8] - piCur[n+ 8]; uiSum += ( iTemp * iTemp ) >> uiShift;
      iTemp = piOrg[n+ 9] - piCur[n+ 9]; uiSum += ( iTemp * iTemp ) >> uiShift;
      iTemp = piOrg[n+10] - piCur[n+10]; uiSum += ( iTemp * iTemp ) >> uiShift;
      iTemp = piOrg[n+11] - piCur[n+11]; uiSum += ( iTemp * iTemp ) >> uiShift;
      iTemp = piOrg[n+12] - piCur[n+12]; uiSum += ( iTemp * iTemp ) >> uiShift;
      iTemp = piOrg[n+13] - piCur[n+13]; uiSum += ( iTemp * iTemp ) >> uiShift;
      iTemp = piOrg[n+14] - piCur[n+14]; uiSum += ( iTemp * iTemp ) >> uiShift;
      iTemp = piOrg[n+15] - piCur[n+15]; uiSum += ( iTemp * iTemp ) >> uiShift;
      
    }
    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }
  
  return ( uiSum );
}

UInt TComRdCost::xGetSSE32( DistParam* pcDtParam )
{
  if ( pcDtParam->bApplyWeight )
  {
    assert( pcDtParam->iCols == 32 );
    return xGetSSEw( pcDtParam );
  }
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  Int  iStrideCur = pcDtParam->iStrideCur;
  
  UInt uiSum = 0;
  UInt uiShift = DISTORTION_PRECISION_ADJUSTMENT((pcDtParam->bitDepth-8) << 1);
  Int  iTemp;
  
  for( ; iRows != 0; iRows-- )
  {
    
    iTemp = piOrg[ 0] - piCur[ 0]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 1] - piCur[ 1]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 2] - piCur[ 2]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 3] - piCur[ 3]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 4] - piCur[ 4]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 5] - piCur[ 5]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 6] - piCur[ 6]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 7] - piCur[ 7]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 8] - piCur[ 8]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 9] - piCur[ 9]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[10] - piCur[10]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[11] - piCur[11]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[12] - piCur[12]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[13] - piCur[13]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[14] - piCur[14]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[15] - piCur[15]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[16] - piCur[16]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[17] - piCur[17]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[18] - piCur[18]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[19] - piCur[19]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[20] - piCur[20]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[21] - piCur[21]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[22] - piCur[22]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[23] - piCur[23]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[24] - piCur[24]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[25] - piCur[25]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[26] - piCur[26]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[27] - piCur[27]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[28] - piCur[28]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[29] - piCur[29]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[30] - piCur[30]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[31] - piCur[31]; uiSum += ( iTemp * iTemp ) >> uiShift;
    
    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }
  
  return ( uiSum );
}

UInt TComRdCost::xGetSSE64( DistParam* pcDtParam )
{
  if ( pcDtParam->bApplyWeight )
  {
    assert( pcDtParam->iCols == 64 );
    return xGetSSEw( pcDtParam );
  }
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  Int  iStrideCur = pcDtParam->iStrideCur;
  
  UInt uiSum = 0;
  UInt uiShift = DISTORTION_PRECISION_ADJUSTMENT((pcDtParam->bitDepth-8) << 1);
  Int  iTemp;
  
  for( ; iRows != 0; iRows-- )
  {
    iTemp = piOrg[ 0] - piCur[ 0]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 1] - piCur[ 1]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 2] - piCur[ 2]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 3] - piCur[ 3]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 4] - piCur[ 4]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 5] - piCur[ 5]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 6] - piCur[ 6]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 7] - piCur[ 7]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 8] - piCur[ 8]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 9] - piCur[ 9]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[10] - piCur[10]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[11] - piCur[11]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[12] - piCur[12]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[13] - piCur[13]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[14] - piCur[14]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[15] - piCur[15]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[16] - piCur[16]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[17] - piCur[17]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[18] - piCur[18]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[19] - piCur[19]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[20] - piCur[20]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[21] - piCur[21]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[22] - piCur[22]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[23] - piCur[23]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[24] - piCur[24]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[25] - piCur[25]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[26] - piCur[26]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[27] - piCur[27]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[28] - piCur[28]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[29] - piCur[29]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[30] - piCur[30]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[31] - piCur[31]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[32] - piCur[32]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[33] - piCur[33]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[34] - piCur[34]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[35] - piCur[35]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[36] - piCur[36]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[37] - piCur[37]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[38] - piCur[38]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[39] - piCur[39]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[40] - piCur[40]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[41] - piCur[41]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[42] - piCur[42]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[43] - piCur[43]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[44] - piCur[44]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[45] - piCur[45]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[46] - piCur[46]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[47] - piCur[47]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[48] - piCur[48]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[49] - piCur[49]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[50] - piCur[50]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[51] - piCur[51]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[52] - piCur[52]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[53] - piCur[53]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[54] - piCur[54]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[55] - piCur[55]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[56] - piCur[56]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[57] - piCur[57]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[58] - piCur[58]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[59] - piCur[59]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[60] - piCur[60]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[61] - piCur[61]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[62] - piCur[62]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[63] - piCur[63]; uiSum += ( iTemp * iTemp ) >> uiShift;
    
    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }
  
  return ( uiSum );
}

// --------------------------------------------------------------------------------------------------------------------
// HADAMARD with step (used in fractional search)
// --------------------------------------------------------------------------------------------------------------------

UInt TComRdCost::xCalcHADs2x2( Pel *piOrg, Pel *piCur, Int iStrideOrg, Int iStrideCur, Int iStep )
{
  Int satd = 0, diff[4], m[4];
  assert( iStep == 1 );
  diff[0] = piOrg[0             ] - piCur[0];
  diff[1] = piOrg[1             ] - piCur[1];
  diff[2] = piOrg[iStrideOrg    ] - piCur[0 + iStrideCur];
  diff[3] = piOrg[iStrideOrg + 1] - piCur[1 + iStrideCur];
  m[0] = diff[0] + diff[2];
  m[1] = diff[1] + diff[3];
  m[2] = diff[0] - diff[2];
  m[3] = diff[1] - diff[3];
  
  satd += abs(m[0] + m[1]);
  satd += abs(m[0] - m[1]);
  satd += abs(m[2] + m[3]);
  satd += abs(m[2] - m[3]);
  
  return satd;
}

UInt TComRdCost::xCalcHADs4x4( Pel *piOrg, Pel *piCur, Int iStrideOrg, Int iStrideCur, Int iStep )
{
  Int k, satd = 0, diff[16], m[16], d[16];
  
  assert( iStep == 1 );
  for( k = 0; k < 16; k+=4 )
  {
    diff[k+0] = piOrg[0] - piCur[0];
    diff[k+1] = piOrg[1] - piCur[1];
    diff[k+2] = piOrg[2] - piCur[2];
    diff[k+3] = piOrg[3] - piCur[3];
    
    piCur += iStrideCur;
    piOrg += iStrideOrg;
  }
  
  /*===== hadamard transform =====*/
  m[ 0] = diff[ 0] + diff[12];
  m[ 1] = diff[ 1] + diff[13];
  m[ 2] = diff[ 2] + diff[14];
  m[ 3] = diff[ 3] + diff[15];
  m[ 4] = diff[ 4] + diff[ 8];
  m[ 5] = diff[ 5] + diff[ 9];
  m[ 6] = diff[ 6] + diff[10];
  m[ 7] = diff[ 7] + diff[11];
  m[ 8] = diff[ 4] - diff[ 8];
  m[ 9] = diff[ 5] - diff[ 9];
  m[10] = diff[ 6] - diff[10];
  m[11] = diff[ 7] - diff[11];
  m[12] = diff[ 0] - diff[12];
  m[13] = diff[ 1] - diff[13];
  m[14] = diff[ 2] - diff[14];
  m[15] = diff[ 3] - diff[15];
  
  d[ 0] = m[ 0] + m[ 4];
  d[ 1] = m[ 1] + m[ 5];
  d[ 2] = m[ 2] + m[ 6];
  d[ 3] = m[ 3] + m[ 7];
  d[ 4] = m[ 8] + m[12];
  d[ 5] = m[ 9] + m[13];
  d[ 6] = m[10] + m[14];
  d[ 7] = m[11] + m[15];
  d[ 8] = m[ 0] - m[ 4];
  d[ 9] = m[ 1] - m[ 5];
  d[10] = m[ 2] - m[ 6];
  d[11] = m[ 3] - m[ 7];
  d[12] = m[12] - m[ 8];
  d[13] = m[13] - m[ 9];
  d[14] = m[14] - m[10];
  d[15] = m[15] - m[11];
  
  m[ 0] = d[ 0] + d[ 3];
  m[ 1] = d[ 1] + d[ 2];
  m[ 2] = d[ 1] - d[ 2];
  m[ 3] = d[ 0] - d[ 3];
  m[ 4] = d[ 4] + d[ 7];
  m[ 5] = d[ 5] + d[ 6];
  m[ 6] = d[ 5] - d[ 6];
  m[ 7] = d[ 4] - d[ 7];
  m[ 8] = d[ 8] + d[11];
  m[ 9] = d[ 9] + d[10];
  m[10] = d[ 9] - d[10];
  m[11] = d[ 8] - d[11];
  m[12] = d[12] + d[15];
  m[13] = d[13] + d[14];
  m[14] = d[13] - d[14];
  m[15] = d[12] - d[15];
  
  d[ 0] = m[ 0] + m[ 1];
  d[ 1] = m[ 0] - m[ 1];
  d[ 2] = m[ 2] + m[ 3];
  d[ 3] = m[ 3] - m[ 2];
  d[ 4] = m[ 4] + m[ 5];
  d[ 5] = m[ 4] - m[ 5];
  d[ 6] = m[ 6] + m[ 7];
  d[ 7] = m[ 7] - m[ 6];
  d[ 8] = m[ 8] + m[ 9];
  d[ 9] = m[ 8] - m[ 9];
  d[10] = m[10] + m[11];
  d[11] = m[11] - m[10];
  d[12] = m[12] + m[13];
  d[13] = m[12] - m[13];
  d[14] = m[14] + m[15];
  d[15] = m[15] - m[14];
  
  for (k=0; k<16; ++k)
  {
    satd += abs(d[k]);
  }
  satd = ((satd+1)>>1);
  
  return satd;
}

UInt TComRdCost::xCalcHADs8x8( Pel *piOrg, Pel *piCur, Int iStrideOrg, Int iStrideCur, Int iStep )
{
  Int k, i, j, jj, sad=0;
  Int diff[64], m1[8][8], m2[8][8], m3[8][8];
  assert( iStep == 1 );
  for( k = 0; k < 64; k += 8 )
  {
    diff[k+0] = piOrg[0] - piCur[0];
    diff[k+1] = piOrg[1] - piCur[1];
    diff[k+2] = piOrg[2] - piCur[2];
    diff[k+3] = piOrg[3] - piCur[3];
    diff[k+4] = piOrg[4] - piCur[4];
    diff[k+5] = piOrg[5] - piCur[5];
    diff[k+6] = piOrg[6] - piCur[6];
    diff[k+7] = piOrg[7] - piCur[7];
    
    piCur += iStrideCur;
    piOrg += iStrideOrg;
  }
  
  //horizontal
  for (j=0; j < 8; j++)
  {
    jj = j << 3;
    m2[j][0] = diff[jj  ] + diff[jj+4];
    m2[j][1] = diff[jj+1] + diff[jj+5];
    m2[j][2] = diff[jj+2] + diff[jj+6];
    m2[j][3] = diff[jj+3] + diff[jj+7];
    m2[j][4] = diff[jj  ] - diff[jj+4];
    m2[j][5] = diff[jj+1] - diff[jj+5];
    m2[j][6] = diff[jj+2] - diff[jj+6];
    m2[j][7] = diff[jj+3] - diff[jj+7];
    
    m1[j][0] = m2[j][0] + m2[j][2];
    m1[j][1] = m2[j][1] + m2[j][3];
    m1[j][2] = m2[j][0] - m2[j][2];
    m1[j][3] = m2[j][1] - m2[j][3];
    m1[j][4] = m2[j][4] + m2[j][6];
    m1[j][5] = m2[j][5] + m2[j][7];
    m1[j][6] = m2[j][4] - m2[j][6];
    m1[j][7] = m2[j][5] - m2[j][7];
    
    m2[j][0] = m1[j][0] + m1[j][1];
    m2[j][1] = m1[j][0] - m1[j][1];
    m2[j][2] = m1[j][2] + m1[j][3];
    m2[j][3] = m1[j][2] - m1[j][3];
    m2[j][4] = m1[j][4] + m1[j][5];
    m2[j][5] = m1[j][4] - m1[j][5];
    m2[j][6] = m1[j][6] + m1[j][7];
    m2[j][7] = m1[j][6] - m1[j][7];
  }
  
  //vertical
  for (i=0; i < 8; i++)
  {
    m3[0][i] = m2[0][i] + m2[4][i];
    m3[1][i] = m2[1][i] + m2[5][i];
    m3[2][i] = m2[2][i] + m2[6][i];
    m3[3][i] = m2[3][i] + m2[7][i];
    m3[4][i] = m2[0][i] - m2[4][i];
    m3[5][i] = m2[1][i] - m2[5][i];
    m3[6][i] = m2[2][i] - m2[6][i];
    m3[7][i] = m2[3][i] - m2[7][i];
    
    m1[0][i] = m3[0][i] + m3[2][i];
    m1[1][i] = m3[1][i] + m3[3][i];
    m1[2][i] = m3[0][i] - m3[2][i];
    m1[3][i] = m3[1][i] - m3[3][i];
    m1[4][i] = m3[4][i] + m3[6][i];
    m1[5][i] = m3[5][i] + m3[7][i];
    m1[6][i] = m3[4][i] - m3[6][i];
    m1[7][i] = m3[5][i] - m3[7][i];
    
    m2[0][i] = m1[0][i] + m1[1][i];
    m2[1][i] = m1[0][i] - m1[1][i];
    m2[2][i] = m1[2][i] + m1[3][i];
    m2[3][i] = m1[2][i] - m1[3][i];
    m2[4][i] = m1[4][i] + m1[5][i];
    m2[5][i] = m1[4][i] - m1[5][i];
    m2[6][i] = m1[6][i] + m1[7][i];
    m2[7][i] = m1[6][i] - m1[7][i];
  }
  
  for (i = 0; i < 8; i++)
  {
    for (j = 0; j < 8; j++)
    {
      sad += abs(m2[i][j]);
    }
  }
  
  sad=((sad+2)>>2);
  
  return sad;
}

#if NS_HAD
UInt TComRdCost::xCalcHADs16x4( Pel *piOrg, Pel *piCur, Int iStrideOrg, Int iStrideCur, Int iStep )
{
  Int k, i, j, jj, sad=0;
  Int diff[64], m1[4][16], m2[4][16];
  assert( iStep == 1 );
  for( k = 0; k < 64; k += 16 )
  {
    diff[k+0] = piOrg[0] - piCur[0];
    diff[k+1] = piOrg[1] - piCur[1];
    diff[k+2] = piOrg[2] - piCur[2];
    diff[k+3] = piOrg[3] - piCur[3];
    diff[k+4] = piOrg[4] - piCur[4];
    diff[k+5] = piOrg[5] - piCur[5];
    diff[k+6] = piOrg[6] - piCur[6];
    diff[k+7] = piOrg[7] - piCur[7];

    diff[k+8]  = piOrg[8]  - piCur[8] ;
    diff[k+9]  = piOrg[9]  - piCur[9] ;
    diff[k+10] = piOrg[10] - piCur[10];
    diff[k+11] = piOrg[11] - piCur[11];
    diff[k+12] = piOrg[12] - piCur[12];
    diff[k+13] = piOrg[13] - piCur[13];
    diff[k+14] = piOrg[14] - piCur[14];
    diff[k+15] = piOrg[15] - piCur[15];

    piCur += iStrideCur;
    piOrg += iStrideOrg;
  }

  //horizontal
  for (j=0; j < 4; j++)
  {
    jj = j << 4;

    m2[j][0]  = diff[jj  ] + diff[jj+8];
    m2[j][1]  = diff[jj+1] + diff[jj+9];
    m2[j][2]  = diff[jj+2] + diff[jj+10];
    m2[j][3]  = diff[jj+3] + diff[jj+11];
    m2[j][4]  = diff[jj+4] + diff[jj+12];
    m2[j][5]  = diff[jj+5] + diff[jj+13];
    m2[j][6]  = diff[jj+6] + diff[jj+14];
    m2[j][7]  = diff[jj+7] + diff[jj+15];
    m2[j][8]  = diff[jj  ] - diff[jj+8];
    m2[j][9]  = diff[jj+1] - diff[jj+9];
    m2[j][10] = diff[jj+2] - diff[jj+10];
    m2[j][11] = diff[jj+3] - diff[jj+11];
    m2[j][12] = diff[jj+4] - diff[jj+12];
    m2[j][13] = diff[jj+5] - diff[jj+13];
    m2[j][14] = diff[jj+6] - diff[jj+14];
    m2[j][15] = diff[jj+7] - diff[jj+15];

    m1[j][0]  = m2[j][0]  + m2[j][4];
    m1[j][1]  = m2[j][1]  + m2[j][5];
    m1[j][2]  = m2[j][2]  + m2[j][6];
    m1[j][3]  = m2[j][3]  + m2[j][7];
    m1[j][4]  = m2[j][0]  - m2[j][4];
    m1[j][5]  = m2[j][1]  - m2[j][5];
    m1[j][6]  = m2[j][2]  - m2[j][6];
    m1[j][7]  = m2[j][3]  - m2[j][7];
    m1[j][8]  = m2[j][8]  + m2[j][12];
    m1[j][9]  = m2[j][9]  + m2[j][13];
    m1[j][10] = m2[j][10] + m2[j][14];
    m1[j][11] = m2[j][11] + m2[j][15];
    m1[j][12] = m2[j][8]  - m2[j][12];
    m1[j][13] = m2[j][9]  - m2[j][13];
    m1[j][14] = m2[j][10] - m2[j][14];
    m1[j][15] = m2[j][11] - m2[j][15];

    m2[j][0]  = m1[j][0]  + m1[j][2];
    m2[j][1]  = m1[j][1]  + m1[j][3];
    m2[j][2]  = m1[j][0]  - m1[j][2];
    m2[j][3]  = m1[j][1]  - m1[j][3];
    m2[j][4]  = m1[j][4]  + m1[j][6];
    m2[j][5]  = m1[j][5]  + m1[j][7];
    m2[j][6]  = m1[j][4]  - m1[j][6];
    m2[j][7]  = m1[j][5]  - m1[j][7];
    m2[j][8]  = m1[j][8]  + m1[j][10];
    m2[j][9]  = m1[j][9]  + m1[j][11];
    m2[j][10] = m1[j][8]  - m1[j][10];
    m2[j][11] = m1[j][9]  - m1[j][11];
    m2[j][12] = m1[j][12] + m1[j][14];
    m2[j][13] = m1[j][13] + m1[j][15];
    m2[j][14] = m1[j][12] - m1[j][14];
    m2[j][15] = m1[j][13] - m1[j][15];

    m1[j][0]  = m2[j][0]  + m2[j][1];
    m1[j][1]  = m2[j][0]  - m2[j][1];
    m1[j][2]  = m2[j][2]  + m2[j][3];
    m1[j][3]  = m2[j][2]  - m2[j][3];
    m1[j][4]  = m2[j][4]  + m2[j][5];
    m1[j][5]  = m2[j][4]  - m2[j][5];
    m1[j][6]  = m2[j][6]  + m2[j][7];
    m1[j][7]  = m2[j][6]  - m2[j][7];
    m1[j][8]  = m2[j][8]  + m2[j][9];
    m1[j][9]  = m2[j][8]  - m2[j][9];
    m1[j][10] = m2[j][10] + m2[j][11];
    m1[j][11] = m2[j][10] - m2[j][11];
    m1[j][12] = m2[j][12] + m2[j][13];
    m1[j][13] = m2[j][12] - m2[j][13];
    m1[j][14] = m2[j][14] + m2[j][15];
    m1[j][15] = m2[j][14] - m2[j][15];
  }

  //vertical
  for (i=0; i < 16; i++)
  {    
    m2[0][i] = m1[0][i] + m1[2][i];
    m2[1][i] = m1[1][i] + m1[3][i];
    m2[2][i] = m1[0][i] - m1[2][i];
    m2[3][i] = m1[1][i] - m1[3][i];

    m1[0][i] = m2[0][i] + m2[1][i];
    m1[1][i] = m2[0][i] - m2[1][i];
    m1[2][i] = m2[2][i] + m2[3][i];
    m1[3][i] = m2[2][i] - m2[3][i];
  }

  for (i = 0; i < 4; i++)
  {
    for (j = 0; j < 16; j++)
    {
      sad += abs(m1[i][j]);
    }
  }

  sad=((sad+2)>>2);

  return sad;
}

UInt TComRdCost::xCalcHADs4x16( Pel *piOrg, Pel *piCur, Int iStrideOrg, Int iStrideCur, Int iStep )
{
  Int k, i, j, jj, sad=0;
  Int diff[64], m1[16][4], m2[16][4], m3[16][4];
  assert( iStep == 1 );
  for( k = 0; k < 64; k += 4 )
  {
    diff[k+0] = piOrg[0] - piCur[0];
    diff[k+1] = piOrg[1] - piCur[1];
    diff[k+2] = piOrg[2] - piCur[2];
    diff[k+3] = piOrg[3] - piCur[3];

    piCur += iStrideCur;
    piOrg += iStrideOrg;
  }

  //horizontal
  for (j=0; j < 16; j++)
  {
    jj = j << 2;
    m2[j][0] = diff[jj  ] + diff[jj+2];
    m2[j][1] = diff[jj+1] + diff[jj+3];
    m2[j][2] = diff[jj  ] - diff[jj+2];
    m2[j][3] = diff[jj+1] - diff[jj+3];

    m1[j][0] = m2[j][0] + m2[j][1];
    m1[j][1] = m2[j][0] - m2[j][1];
    m1[j][2] = m2[j][2] + m2[j][3];
    m1[j][3] = m2[j][2] - m2[j][3];
  }

  //vertical
  for (i=0; i < 4; i++)
  {
    m2[0][i]  = m1[0][i] + m1[8][i];
    m2[1][i]  = m1[1][i] + m1[9][i];
    m2[2][i]  = m1[2][i] + m1[10][i];
    m2[3][i]  = m1[3][i] + m1[11][i];
    m2[4][i]  = m1[4][i] + m1[12][i];
    m2[5][i]  = m1[5][i] + m1[13][i];
    m2[6][i]  = m1[6][i] + m1[14][i];
    m2[7][i]  = m1[7][i] + m1[15][i];
    m2[8][i]  = m1[0][i] - m1[8][i];
    m2[9][i]  = m1[1][i] - m1[9][i];
    m2[10][i] = m1[2][i] - m1[10][i];
    m2[11][i] = m1[3][i] - m1[11][i];
    m2[12][i] = m1[4][i] - m1[12][i];
    m2[13][i] = m1[5][i] - m1[13][i];
    m2[14][i] = m1[6][i] - m1[14][i];
    m2[15][i] = m1[7][i] - m1[15][i];

    m3[0][i]  = m2[0][i]  + m2[4][i];
    m3[1][i]  = m2[1][i]  + m2[5][i];
    m3[2][i]  = m2[2][i]  + m2[6][i];
    m3[3][i]  = m2[3][i]  + m2[7][i];
    m3[4][i]  = m2[0][i]  - m2[4][i];
    m3[5][i]  = m2[1][i]  - m2[5][i];
    m3[6][i]  = m2[2][i]  - m2[6][i];
    m3[7][i]  = m2[3][i]  - m2[7][i];
    m3[8][i]  = m2[8][i]  + m2[12][i];
    m3[9][i]  = m2[9][i]  + m2[13][i];
    m3[10][i] = m2[10][i] + m2[14][i];
    m3[11][i] = m2[11][i] + m2[15][i];
    m3[12][i] = m2[8][i]  - m2[12][i];
    m3[13][i] = m2[9][i]  - m2[13][i];
    m3[14][i] = m2[10][i] - m2[14][i];
    m3[15][i] = m2[11][i] - m2[15][i];

    m1[0][i]  = m3[0][i]  + m3[2][i];
    m1[1][i]  = m3[1][i]  + m3[3][i];
    m1[2][i]  = m3[0][i]  - m3[2][i];
    m1[3][i]  = m3[1][i]  - m3[3][i];
    m1[4][i]  = m3[4][i]  + m3[6][i];
    m1[5][i]  = m3[5][i]  + m3[7][i];
    m1[6][i]  = m3[4][i]  - m3[6][i];
    m1[7][i]  = m3[5][i]  - m3[7][i];
    m1[8][i]  = m3[8][i]  + m3[10][i];
    m1[9][i]  = m3[9][i]  + m3[11][i];
    m1[10][i] = m3[8][i]  - m3[10][i];
    m1[11][i] = m3[9][i]  - m3[11][i];
    m1[12][i] = m3[12][i] + m3[14][i];
    m1[13][i] = m3[13][i] + m3[15][i];
    m1[14][i] = m3[12][i] - m3[14][i];
    m1[15][i] = m3[13][i] - m3[15][i];

    m2[0][i]  = m1[0][i]  + m1[1][i];
    m2[1][i]  = m1[0][i]  - m1[1][i];
    m2[2][i]  = m1[2][i]  + m1[3][i];
    m2[3][i]  = m1[2][i]  - m1[3][i];
    m2[4][i]  = m1[4][i]  + m1[5][i];
    m2[5][i]  = m1[4][i]  - m1[5][i];
    m2[6][i]  = m1[6][i]  + m1[7][i];
    m2[7][i]  = m1[6][i]  - m1[7][i];
    m2[8][i]  = m1[8][i]  + m1[9][i];
    m2[9][i]  = m1[8][i]  - m1[9][i];
    m2[10][i] = m1[10][i] + m1[11][i];
    m2[11][i] = m1[10][i] - m1[11][i];
    m2[12][i] = m1[12][i] + m1[13][i];
    m2[13][i] = m1[12][i] - m1[13][i];
    m2[14][i] = m1[14][i] + m1[15][i];
    m2[15][i] = m1[14][i] - m1[15][i];
  }

  for (i = 0; i < 16; i++)
  {
    for (j = 0; j < 4; j++)
    {
      sad += abs(m2[i][j]);
    }
  }

  sad=((sad+2)>>2);

  return sad;
}
#endif

UInt TComRdCost::xGetHADs4( DistParam* pcDtParam )
{
  if ( pcDtParam->bApplyWeight )
  {
    return xGetHADs4w( pcDtParam );
  }
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iStrideCur = pcDtParam->iStrideCur;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  Int  iStep  = pcDtParam->iStep;
  Int  y;
  Int  iOffsetOrg = iStrideOrg<<2;
  Int  iOffsetCur = iStrideCur<<2;
  
  UInt uiSum = 0;
  
  for ( y=0; y<iRows; y+= 4 )
  {
    uiSum += xCalcHADs4x4( piOrg, piCur, iStrideOrg, iStrideCur, iStep );
    piOrg += iOffsetOrg;
    piCur += iOffsetCur;
  }
  
  return uiSum >> DISTORTION_PRECISION_ADJUSTMENT(pcDtParam->bitDepth-8);
}

UInt TComRdCost::xGetHADs8( DistParam* pcDtParam )
{
  if ( pcDtParam->bApplyWeight )
  {
    return xGetHADs8w( pcDtParam );
  }
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iStrideCur = pcDtParam->iStrideCur;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  Int  iStep  = pcDtParam->iStep;
  Int  y;
  
  UInt uiSum = 0;
  
  if ( iRows == 4 )
  {
    uiSum += xCalcHADs4x4( piOrg+0, piCur        , iStrideOrg, iStrideCur, iStep );
    uiSum += xCalcHADs4x4( piOrg+4, piCur+4*iStep, iStrideOrg, iStrideCur, iStep );
  }
  else
  {
    Int  iOffsetOrg = iStrideOrg<<3;
    Int  iOffsetCur = iStrideCur<<3;
    for ( y=0; y<iRows; y+= 8 )
    {
      uiSum += xCalcHADs8x8( piOrg, piCur, iStrideOrg, iStrideCur, iStep );
      piOrg += iOffsetOrg;
      piCur += iOffsetCur;
    }
  }
  
  return uiSum >> DISTORTION_PRECISION_ADJUSTMENT(pcDtParam->bitDepth-8);
}

UInt TComRdCost::xGetHADs( DistParam* pcDtParam )
{
  if ( pcDtParam->bApplyWeight )
  {
    return xGetHADsw( pcDtParam );
  }
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iCols   = pcDtParam->iCols;
  Int  iStrideCur = pcDtParam->iStrideCur;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  Int  iStep  = pcDtParam->iStep;
  
  Int  x, y;
  
  UInt uiSum = 0;
  
#if NS_HAD
  if( ( ( iRows % 8 == 0) && (iCols % 8 == 0) && ( iRows == iCols ) ) || ( ( iRows % 8 == 0 ) && (iCols % 8 == 0) && !pcDtParam->bUseNSHAD ) )
#else
  if( ( iRows % 8 == 0) && (iCols % 8 == 0) )
#endif
  {
    Int  iOffsetOrg = iStrideOrg<<3;
    Int  iOffsetCur = iStrideCur<<3;
    for ( y=0; y<iRows; y+= 8 )
    {
      for ( x=0; x<iCols; x+= 8 )
      {
        uiSum += xCalcHADs8x8( &piOrg[x], &piCur[x*iStep], iStrideOrg, iStrideCur, iStep );
      }
      piOrg += iOffsetOrg;
      piCur += iOffsetCur;
    }
  }
#if NS_HAD
  else if ( ( iCols > 8 ) && ( iCols > iRows ) && pcDtParam->bUseNSHAD ) 
  {
    Int  iOffsetOrg = iStrideOrg<<2;
    Int  iOffsetCur = iStrideCur<<2;
    for ( y=0; y<iRows; y+= 4 )
    {
      for ( x=0; x<iCols; x+= 16 )
      {
        uiSum += xCalcHADs16x4( &piOrg[x], &piCur[x*iStep], iStrideOrg, iStrideCur, iStep );
      }
      piOrg += iOffsetOrg;
      piCur += iOffsetCur;
    }
  }
  else if ( ( iRows > 8 ) && ( iCols < iRows ) && pcDtParam->bUseNSHAD ) 
  {
    Int  iOffsetOrg = iStrideOrg<<4;
    Int  iOffsetCur = iStrideCur<<4;
    for ( y=0; y<iRows; y+= 16 )
    {
      for ( x=0; x<iCols; x+= 4 )
      {
        uiSum += xCalcHADs4x16( &piOrg[x], &piCur[x*iStep], iStrideOrg, iStrideCur, iStep );
      }
      piOrg += iOffsetOrg;
      piCur += iOffsetCur;
    }
  }
#endif
  else if( ( iRows % 4 == 0) && (iCols % 4 == 0) )
  {
    Int  iOffsetOrg = iStrideOrg<<2;
    Int  iOffsetCur = iStrideCur<<2;
    
    for ( y=0; y<iRows; y+= 4 )
    {
      for ( x=0; x<iCols; x+= 4 )
      {
        uiSum += xCalcHADs4x4( &piOrg[x], &piCur[x*iStep], iStrideOrg, iStrideCur, iStep );
      }
      piOrg += iOffsetOrg;
      piCur += iOffsetCur;
    }
  }
  else if( ( iRows % 2 == 0) && (iCols % 2 == 0) )
  {
    Int  iOffsetOrg = iStrideOrg<<1;
    Int  iOffsetCur = iStrideCur<<1;
    for ( y=0; y<iRows; y+=2 )
    {
      for ( x=0; x<iCols; x+=2 )
      {
        uiSum += xCalcHADs2x2( &piOrg[x], &piCur[x*iStep], iStrideOrg, iStrideCur, iStep );
      }
      piOrg += iOffsetOrg;
      piCur += iOffsetCur;
    }
  }
  else
  {
    assert(false);
  }
  
  return uiSum >> DISTORTION_PRECISION_ADJUSTMENT(pcDtParam->bitDepth-8);
}

//! \}
