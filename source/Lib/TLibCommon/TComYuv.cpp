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

/** \file     TComYuv.cpp
    \brief    general YUV buffer class
    \todo     this should be merged with TComPicYuv
*/

#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <math.h>

#include "CommonDef.h"
#include "TComYuv.h"
#include "TComInterpolationFilter.h"

//! \ingroup TLibCommon
//! \{

TComYuv::TComYuv()
{
  m_apiBufY = NULL;
  m_apiBufU = NULL;
  m_apiBufV = NULL;
}

TComYuv::~TComYuv()
{
}

Void TComYuv::create( UInt iWidth, UInt iHeight )
{
  // memory allocation
  m_apiBufY  = (Pel*)xMalloc( Pel, iWidth*iHeight    );
  m_apiBufU  = (Pel*)xMalloc( Pel, iWidth*iHeight >> 2 );
  m_apiBufV  = (Pel*)xMalloc( Pel, iWidth*iHeight >> 2 );
  
  // set width and height
  m_iWidth   = iWidth;
  m_iHeight  = iHeight;
  m_iCWidth  = iWidth  >> 1;
  m_iCHeight = iHeight >> 1;
}

Void TComYuv::destroy()
{
  // memory free
  xFree( m_apiBufY ); m_apiBufY = NULL;
  xFree( m_apiBufU ); m_apiBufU = NULL;
  xFree( m_apiBufV ); m_apiBufV = NULL;
}

Void TComYuv::clear()
{
  ::memset( m_apiBufY, 0, ( m_iWidth  * m_iHeight  )*sizeof(Pel) );
  ::memset( m_apiBufU, 0, ( m_iCWidth * m_iCHeight )*sizeof(Pel) );
  ::memset( m_apiBufV, 0, ( m_iCWidth * m_iCHeight )*sizeof(Pel) );
}

Void TComYuv::copyToPicYuv   ( TComPicYuv* pcPicYuvDst, UInt iCuAddr, UInt uiAbsZorderIdx, UInt uiPartDepth, UInt uiPartIdx )
{
  copyToPicLuma  ( pcPicYuvDst, iCuAddr, uiAbsZorderIdx, uiPartDepth, uiPartIdx );
  copyToPicChroma( pcPicYuvDst, iCuAddr, uiAbsZorderIdx, uiPartDepth, uiPartIdx );
}

Void TComYuv::copyToPicLuma  ( TComPicYuv* pcPicYuvDst, UInt iCuAddr, UInt uiAbsZorderIdx, UInt uiPartDepth, UInt uiPartIdx )
{
  Int  y, iWidth, iHeight;
  iWidth  = m_iWidth >>uiPartDepth;
  iHeight = m_iHeight>>uiPartDepth;
  
  Pel* pSrc     = getLumaAddr(uiPartIdx, iWidth);
  Pel* pDst     = pcPicYuvDst->getLumaAddr ( iCuAddr, uiAbsZorderIdx );
  
  UInt  iSrcStride  = getStride();
  UInt  iDstStride  = pcPicYuvDst->getStride();
  
  for ( y = iHeight; y != 0; y-- )
  {
    ::memcpy( pDst, pSrc, sizeof(Pel)*iWidth);
    pDst += iDstStride;
    pSrc += iSrcStride;
  }
}

Void TComYuv::copyToPicChroma( TComPicYuv* pcPicYuvDst, UInt iCuAddr, UInt uiAbsZorderIdx, UInt uiPartDepth, UInt uiPartIdx )
{
  Int  y, iWidth, iHeight;
  iWidth  = m_iCWidth >>uiPartDepth;
  iHeight = m_iCHeight>>uiPartDepth;
  
  Pel* pSrcU      = getCbAddr(uiPartIdx, iWidth);
  Pel* pSrcV      = getCrAddr(uiPartIdx, iWidth);
  Pel* pDstU      = pcPicYuvDst->getCbAddr( iCuAddr, uiAbsZorderIdx );
  Pel* pDstV      = pcPicYuvDst->getCrAddr( iCuAddr, uiAbsZorderIdx );
  
  UInt  iSrcStride = getCStride();
  UInt  iDstStride = pcPicYuvDst->getCStride();
  for ( y = iHeight; y != 0; y-- )
  {
    ::memcpy( pDstU, pSrcU, sizeof(Pel)*(iWidth) );
    ::memcpy( pDstV, pSrcV, sizeof(Pel)*(iWidth) );
    pSrcU += iSrcStride;
    pSrcV += iSrcStride;
    pDstU += iDstStride;
    pDstV += iDstStride;
  }
}

Void TComYuv::copyFromPicYuv   ( TComPicYuv* pcPicYuvSrc, UInt iCuAddr, UInt uiAbsZorderIdx )
{
  copyFromPicLuma  ( pcPicYuvSrc, iCuAddr, uiAbsZorderIdx );
  copyFromPicChroma( pcPicYuvSrc, iCuAddr, uiAbsZorderIdx );
}

Void TComYuv::copyFromPicLuma  ( TComPicYuv* pcPicYuvSrc, UInt iCuAddr, UInt uiAbsZorderIdx )
{
  Int  y;
  
  Pel* pDst     = m_apiBufY;
  Pel* pSrc     = pcPicYuvSrc->getLumaAddr ( iCuAddr, uiAbsZorderIdx );
  
  UInt  iDstStride  = getStride();
  UInt  iSrcStride  = pcPicYuvSrc->getStride();
  for ( y = m_iHeight; y != 0; y-- )
  {
    ::memcpy( pDst, pSrc, sizeof(Pel)*m_iWidth);
    pDst += iDstStride;
    pSrc += iSrcStride;
  }
}

Void TComYuv::copyFromPicChroma( TComPicYuv* pcPicYuvSrc, UInt iCuAddr, UInt uiAbsZorderIdx )
{
  Int  y;
  
  Pel* pDstU      = m_apiBufU;
  Pel* pDstV      = m_apiBufV;
  Pel* pSrcU      = pcPicYuvSrc->getCbAddr( iCuAddr, uiAbsZorderIdx );
  Pel* pSrcV      = pcPicYuvSrc->getCrAddr( iCuAddr, uiAbsZorderIdx );
  
  UInt  iDstStride = getCStride();
  UInt  iSrcStride = pcPicYuvSrc->getCStride();
  for ( y = m_iCHeight; y != 0; y-- )
  {
    ::memcpy( pDstU, pSrcU, sizeof(Pel)*(m_iCWidth) );
    ::memcpy( pDstV, pSrcV, sizeof(Pel)*(m_iCWidth) );
    pSrcU += iSrcStride;
    pSrcV += iSrcStride;
    pDstU += iDstStride;
    pDstV += iDstStride;
  }
}

Void TComYuv::copyToPartYuv( TComYuv* pcYuvDst, UInt uiDstPartIdx )
{
  copyToPartLuma  ( pcYuvDst, uiDstPartIdx );
  copyToPartChroma( pcYuvDst, uiDstPartIdx );
}

Void TComYuv::copyToPartLuma( TComYuv* pcYuvDst, UInt uiDstPartIdx )
{
  Int  y;
  
  Pel* pSrc     = m_apiBufY;
  Pel* pDst     = pcYuvDst->getLumaAddr( uiDstPartIdx );
  
  UInt  iSrcStride  = getStride();
  UInt  iDstStride  = pcYuvDst->getStride();
  for ( y = m_iHeight; y != 0; y-- )
  {
    ::memcpy( pDst, pSrc, sizeof(Pel)*m_iWidth);
    pDst += iDstStride;
    pSrc += iSrcStride;
  }
}

Void TComYuv::copyToPartChroma( TComYuv* pcYuvDst, UInt uiDstPartIdx )
{
  Int  y;
  
  Pel* pSrcU      = m_apiBufU;
  Pel* pSrcV      = m_apiBufV;
  Pel* pDstU      = pcYuvDst->getCbAddr( uiDstPartIdx );
  Pel* pDstV      = pcYuvDst->getCrAddr( uiDstPartIdx );
  
  UInt  iSrcStride = getCStride();
  UInt  iDstStride = pcYuvDst->getCStride();
  for ( y = m_iCHeight; y != 0; y-- )
  {
    ::memcpy( pDstU, pSrcU, sizeof(Pel)*(m_iCWidth) );
    ::memcpy( pDstV, pSrcV, sizeof(Pel)*(m_iCWidth) );
    pSrcU += iSrcStride;
    pSrcV += iSrcStride;
    pDstU += iDstStride;
    pDstV += iDstStride;
  }
}

Void TComYuv::copyPartToYuv( TComYuv* pcYuvDst, UInt uiSrcPartIdx )
{
  copyPartToLuma  ( pcYuvDst, uiSrcPartIdx );
  copyPartToChroma( pcYuvDst, uiSrcPartIdx );
}

Void TComYuv::copyPartToLuma( TComYuv* pcYuvDst, UInt uiSrcPartIdx )
{
  Int  y;
  
  Pel* pSrc     = getLumaAddr(uiSrcPartIdx);
  Pel* pDst     = pcYuvDst->getLumaAddr( 0 );
  
  UInt  iSrcStride  = getStride();
  UInt  iDstStride  = pcYuvDst->getStride();
  
  UInt uiHeight = pcYuvDst->getHeight();
  UInt uiWidth = pcYuvDst->getWidth();
  
  for ( y = uiHeight; y != 0; y-- )
  {
    ::memcpy( pDst, pSrc, sizeof(Pel)*uiWidth);
    pDst += iDstStride;
    pSrc += iSrcStride;
  }
}

Void TComYuv::copyPartToChroma( TComYuv* pcYuvDst, UInt uiSrcPartIdx )
{
  Int  y;
  
  Pel* pSrcU      = getCbAddr( uiSrcPartIdx );
  Pel* pSrcV      = getCrAddr( uiSrcPartIdx );
  Pel* pDstU      = pcYuvDst->getCbAddr( 0 );
  Pel* pDstV      = pcYuvDst->getCrAddr( 0 );
  
  UInt  iSrcStride = getCStride();
  UInt  iDstStride = pcYuvDst->getCStride();
  
  UInt uiCHeight = pcYuvDst->getCHeight();
  UInt uiCWidth = pcYuvDst->getCWidth();
  
  for ( y = uiCHeight; y != 0; y-- )
  {
    ::memcpy( pDstU, pSrcU, sizeof(Pel)*(uiCWidth) );
    ::memcpy( pDstV, pSrcV, sizeof(Pel)*(uiCWidth) );
    pSrcU += iSrcStride;
    pSrcV += iSrcStride;
    pDstU += iDstStride;
    pDstV += iDstStride;
  }
}

Void TComYuv::copyPartToPartYuv   ( TComYuv* pcYuvDst, UInt uiPartIdx, UInt iWidth, UInt iHeight )
{
  copyPartToPartLuma   (pcYuvDst, uiPartIdx, iWidth, iHeight );
  copyPartToPartChroma (pcYuvDst, uiPartIdx, iWidth>>1, iHeight>>1 );
}

Void TComYuv::copyPartToPartLuma  ( TComYuv* pcYuvDst, UInt uiPartIdx, UInt iWidth, UInt iHeight )
{
  Pel* pSrc =           getLumaAddr(uiPartIdx);
  Pel* pDst = pcYuvDst->getLumaAddr(uiPartIdx);
  if( pSrc == pDst )
  {
    //th not a good idea
    //th best would be to fix the caller 
    return ;
  }
  
  UInt  iSrcStride = getStride();
  UInt  iDstStride = pcYuvDst->getStride();
  for ( UInt y = iHeight; y != 0; y-- )
  {
    ::memcpy( pDst, pSrc, iWidth * sizeof(Pel) );
    pSrc += iSrcStride;
    pDst += iDstStride;
  }
}

Void TComYuv::copyPartToPartChroma( TComYuv* pcYuvDst, UInt uiPartIdx, UInt iWidth, UInt iHeight )
{
  Pel*  pSrcU =           getCbAddr(uiPartIdx);
  Pel*  pSrcV =           getCrAddr(uiPartIdx);
  Pel*  pDstU = pcYuvDst->getCbAddr(uiPartIdx);
  Pel*  pDstV = pcYuvDst->getCrAddr(uiPartIdx);
  
  if( pSrcU == pDstU && pSrcV == pDstV)
  {
    //th not a good idea
    //th best would be to fix the caller 
    return ;
  }
  
  UInt   iSrcStride = getCStride();
  UInt   iDstStride = pcYuvDst->getCStride();
  for ( UInt y = iHeight; y != 0; y-- )
  {
    ::memcpy( pDstU, pSrcU, iWidth * sizeof(Pel) );
    ::memcpy( pDstV, pSrcV, iWidth * sizeof(Pel) );
    pSrcU += iSrcStride;
    pSrcV += iSrcStride;
    pDstU += iDstStride;
    pDstV += iDstStride;
  }
}

Void TComYuv::copyPartToPartChroma( TComYuv* pcYuvDst, UInt uiPartIdx, UInt iWidth, UInt iHeight, UInt chromaId)
{
  if(chromaId == 0)
  {
    Pel*  pSrcU =           getCbAddr(uiPartIdx);
    Pel*  pDstU = pcYuvDst->getCbAddr(uiPartIdx);
    if( pSrcU == pDstU)
    {
      return ;
    }
    UInt   iSrcStride = getCStride();
    UInt   iDstStride = pcYuvDst->getCStride();
    for ( UInt y = iHeight; y != 0; y-- )
    {
      ::memcpy( pDstU, pSrcU, iWidth * sizeof(Pel) );
      pSrcU += iSrcStride;
      pDstU += iDstStride;
    }
  }
  else if (chromaId == 1)
  {
    Pel*  pSrcV =           getCrAddr(uiPartIdx);
    Pel*  pDstV = pcYuvDst->getCrAddr(uiPartIdx);
    if( pSrcV == pDstV)
    {
      return;
    }
    UInt   iSrcStride = getCStride();
    UInt   iDstStride = pcYuvDst->getCStride();
    for ( UInt y = iHeight; y != 0; y-- )
    { 
      ::memcpy( pDstV, pSrcV, iWidth * sizeof(Pel) );
      pSrcV += iSrcStride;
      pDstV += iDstStride;
    }
  }
  else
  {
    Pel*  pSrcU =           getCbAddr(uiPartIdx);
    Pel*  pSrcV =           getCrAddr(uiPartIdx);
    Pel*  pDstU = pcYuvDst->getCbAddr(uiPartIdx);
    Pel*  pDstV = pcYuvDst->getCrAddr(uiPartIdx);
    
    if( pSrcU == pDstU && pSrcV == pDstV)
    {
      //th not a good idea
      //th best would be to fix the caller 
      return ;
    }
    UInt   iSrcStride = getCStride();
    UInt   iDstStride = pcYuvDst->getCStride();
    for ( UInt y = iHeight; y != 0; y-- )
    {
      ::memcpy( pDstU, pSrcU, iWidth * sizeof(Pel) );
      ::memcpy( pDstV, pSrcV, iWidth * sizeof(Pel) );
      pSrcU += iSrcStride;
      pSrcV += iSrcStride;
      pDstU += iDstStride;
      pDstV += iDstStride;
    }
  }
}

Void TComYuv::addClip( TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, UInt uiTrUnitIdx, UInt uiPartSize )
{
  addClipLuma   ( pcYuvSrc0, pcYuvSrc1, uiTrUnitIdx, uiPartSize     );
  addClipChroma ( pcYuvSrc0, pcYuvSrc1, uiTrUnitIdx, uiPartSize>>1  );
}

Void TComYuv::addClipLuma( TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, UInt uiTrUnitIdx, UInt uiPartSize )
{
  Int x, y;
  
  Pel* pSrc0 = pcYuvSrc0->getLumaAddr( uiTrUnitIdx, uiPartSize );
  Pel* pSrc1 = pcYuvSrc1->getLumaAddr( uiTrUnitIdx, uiPartSize );
  Pel* pDst  = getLumaAddr( uiTrUnitIdx, uiPartSize );
  
  UInt iSrc0Stride = pcYuvSrc0->getStride();
  UInt iSrc1Stride = pcYuvSrc1->getStride();
  UInt iDstStride  = getStride();
  for ( y = uiPartSize-1; y >= 0; y-- )
  {
    for ( x = uiPartSize-1; x >= 0; x-- )
    {
      pDst[x] = ClipY( pSrc0[x] + pSrc1[x] );
    }
    pSrc0 += iSrc0Stride;
    pSrc1 += iSrc1Stride;
    pDst  += iDstStride;
  }
}

Void TComYuv::addClipChroma( TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, UInt uiTrUnitIdx, UInt uiPartSize )
{
  Int x, y;
  
  Pel* pSrcU0 = pcYuvSrc0->getCbAddr( uiTrUnitIdx, uiPartSize );
  Pel* pSrcU1 = pcYuvSrc1->getCbAddr( uiTrUnitIdx, uiPartSize );
  Pel* pSrcV0 = pcYuvSrc0->getCrAddr( uiTrUnitIdx, uiPartSize );
  Pel* pSrcV1 = pcYuvSrc1->getCrAddr( uiTrUnitIdx, uiPartSize );
  Pel* pDstU = getCbAddr( uiTrUnitIdx, uiPartSize );
  Pel* pDstV = getCrAddr( uiTrUnitIdx, uiPartSize );
  
  UInt  iSrc0Stride = pcYuvSrc0->getCStride();
  UInt  iSrc1Stride = pcYuvSrc1->getCStride();
  UInt  iDstStride  = getCStride();
  for ( y = uiPartSize-1; y >= 0; y-- )
  {
    for ( x = uiPartSize-1; x >= 0; x-- )
    {
      pDstU[x] = ClipC( pSrcU0[x] + pSrcU1[x] );
      pDstV[x] = ClipC( pSrcV0[x] + pSrcV1[x] );
    }
    
    pSrcU0 += iSrc0Stride;
    pSrcU1 += iSrc1Stride;
    pSrcV0 += iSrc0Stride;
    pSrcV1 += iSrc1Stride;
    pDstU  += iDstStride;
    pDstV  += iDstStride;
  }
}

Void TComYuv::subtract( TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, UInt uiTrUnitIdx, UInt uiPartSize )
{
  subtractLuma  ( pcYuvSrc0, pcYuvSrc1,  uiTrUnitIdx, uiPartSize    );
  subtractChroma( pcYuvSrc0, pcYuvSrc1,  uiTrUnitIdx, uiPartSize>>1 );
}

Void TComYuv::subtractLuma( TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, UInt uiTrUnitIdx, UInt uiPartSize )
{
  Int x, y;
  
  Pel* pSrc0 = pcYuvSrc0->getLumaAddr( uiTrUnitIdx, uiPartSize );
  Pel* pSrc1 = pcYuvSrc1->getLumaAddr( uiTrUnitIdx, uiPartSize );
  Pel* pDst  = getLumaAddr( uiTrUnitIdx, uiPartSize );
  
  Int  iSrc0Stride = pcYuvSrc0->getStride();
  Int  iSrc1Stride = pcYuvSrc1->getStride();
  Int  iDstStride  = getStride();
  for ( y = uiPartSize-1; y >= 0; y-- )
  {
    for ( x = uiPartSize-1; x >= 0; x-- )
    {
      pDst[x] = pSrc0[x] - pSrc1[x];
    }
    pSrc0 += iSrc0Stride;
    pSrc1 += iSrc1Stride;
    pDst  += iDstStride;
  }
}

Void TComYuv::subtractChroma( TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, UInt uiTrUnitIdx, UInt uiPartSize )
{
  Int x, y;
  
  Pel* pSrcU0 = pcYuvSrc0->getCbAddr( uiTrUnitIdx, uiPartSize );
  Pel* pSrcU1 = pcYuvSrc1->getCbAddr( uiTrUnitIdx, uiPartSize );
  Pel* pSrcV0 = pcYuvSrc0->getCrAddr( uiTrUnitIdx, uiPartSize );
  Pel* pSrcV1 = pcYuvSrc1->getCrAddr( uiTrUnitIdx, uiPartSize );
  Pel* pDstU  = getCbAddr( uiTrUnitIdx, uiPartSize );
  Pel* pDstV  = getCrAddr( uiTrUnitIdx, uiPartSize );
  
  Int  iSrc0Stride = pcYuvSrc0->getCStride();
  Int  iSrc1Stride = pcYuvSrc1->getCStride();
  Int  iDstStride  = getCStride();
  for ( y = uiPartSize-1; y >= 0; y-- )
  {
    for ( x = uiPartSize-1; x >= 0; x-- )
    {
      pDstU[x] = pSrcU0[x] - pSrcU1[x];
      pDstV[x] = pSrcV0[x] - pSrcV1[x];
    }
    pSrcU0 += iSrc0Stride;
    pSrcU1 += iSrc1Stride;
    pSrcV0 += iSrc0Stride;
    pSrcV1 += iSrc1Stride;
    pDstU  += iDstStride;
    pDstV  += iDstStride;
  }
}

Void TComYuv::addAvg( TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, UInt iPartUnitIdx, UInt iWidth, UInt iHeight )
{
  Int x, y;
  
  Pel* pSrcY0  = pcYuvSrc0->getLumaAddr( iPartUnitIdx );
  Pel* pSrcU0  = pcYuvSrc0->getCbAddr  ( iPartUnitIdx );
  Pel* pSrcV0  = pcYuvSrc0->getCrAddr  ( iPartUnitIdx );
  
  Pel* pSrcY1  = pcYuvSrc1->getLumaAddr( iPartUnitIdx );
  Pel* pSrcU1  = pcYuvSrc1->getCbAddr  ( iPartUnitIdx );
  Pel* pSrcV1  = pcYuvSrc1->getCrAddr  ( iPartUnitIdx );
  
  Pel* pDstY   = getLumaAddr( iPartUnitIdx );
  Pel* pDstU   = getCbAddr  ( iPartUnitIdx );
  Pel* pDstV   = getCrAddr  ( iPartUnitIdx );
  
  UInt  iSrc0Stride = pcYuvSrc0->getStride();
  UInt  iSrc1Stride = pcYuvSrc1->getStride();
  UInt  iDstStride  = getStride();
  Int shiftNum = IF_INTERNAL_PREC + 1 - g_bitDepthY;
  Int offset = ( 1 << ( shiftNum - 1 ) ) + 2 * IF_INTERNAL_OFFS;
  
  for ( y = 0; y < iHeight; y++ )
  {
    for ( x = 0; x < iWidth; x += 4 )
    {
      pDstY[ x + 0 ] = ClipY( ( pSrcY0[ x + 0 ] + pSrcY1[ x + 0 ] + offset ) >> shiftNum );
      pDstY[ x + 1 ] = ClipY( ( pSrcY0[ x + 1 ] + pSrcY1[ x + 1 ] + offset ) >> shiftNum );
      pDstY[ x + 2 ] = ClipY( ( pSrcY0[ x + 2 ] + pSrcY1[ x + 2 ] + offset ) >> shiftNum );
      pDstY[ x + 3 ] = ClipY( ( pSrcY0[ x + 3 ] + pSrcY1[ x + 3 ] + offset ) >> shiftNum );
    }
    pSrcY0 += iSrc0Stride;
    pSrcY1 += iSrc1Stride;
    pDstY  += iDstStride;
  }
  
  shiftNum = IF_INTERNAL_PREC + 1 - g_bitDepthC;
  offset = ( 1 << ( shiftNum - 1 ) ) + 2 * IF_INTERNAL_OFFS;

  iSrc0Stride = pcYuvSrc0->getCStride();
  iSrc1Stride = pcYuvSrc1->getCStride();
  iDstStride  = getCStride();
  
  iWidth  >>=1;
  iHeight >>=1;
  
  for ( y = iHeight-1; y >= 0; y-- )
  {
    for ( x = iWidth-1; x >= 0; )
    {
      // note: chroma min width is 2
      pDstU[x] = ClipC((pSrcU0[x] + pSrcU1[x] + offset) >> shiftNum);
      pDstV[x] = ClipC((pSrcV0[x] + pSrcV1[x] + offset) >> shiftNum); x--;
      pDstU[x] = ClipC((pSrcU0[x] + pSrcU1[x] + offset) >> shiftNum);
      pDstV[x] = ClipC((pSrcV0[x] + pSrcV1[x] + offset) >> shiftNum); x--;
    }
    
    pSrcU0 += iSrc0Stride;
    pSrcU1 += iSrc1Stride;
    pSrcV0 += iSrc0Stride;
    pSrcV1 += iSrc1Stride;
    pDstU  += iDstStride;
    pDstV  += iDstStride;
  }
}

Void TComYuv::removeHighFreq( TComYuv* pcYuvSrc, UInt uiPartIdx, UInt uiWidht, UInt uiHeight )
{
  Int x, y;
  
  Pel* pSrc  = pcYuvSrc->getLumaAddr(uiPartIdx);
  Pel* pSrcU = pcYuvSrc->getCbAddr(uiPartIdx);
  Pel* pSrcV = pcYuvSrc->getCrAddr(uiPartIdx);
  
  Pel* pDst  = getLumaAddr(uiPartIdx);
  Pel* pDstU = getCbAddr(uiPartIdx);
  Pel* pDstV = getCrAddr(uiPartIdx);
  
  Int  iSrcStride = pcYuvSrc->getStride();
  Int  iDstStride = getStride();
  
  for ( y = uiHeight-1; y >= 0; y-- )
  {
    for ( x = uiWidht-1; x >= 0; x-- )
    {
#if DISABLING_CLIP_FOR_BIPREDME
      pDst[x ] = (pDst[x ]<<1) - pSrc[x ] ;
#else
      pDst[x ] = Clip( (pDst[x ]<<1) - pSrc[x ] );
#endif
    }
    pSrc += iSrcStride;
    pDst += iDstStride;
  }
  
  iSrcStride = pcYuvSrc->getCStride();
  iDstStride = getCStride();
  
  uiHeight >>= 1;
  uiWidht  >>= 1;
  
  for ( y = uiHeight-1; y >= 0; y-- )
  {
    for ( x = uiWidht-1; x >= 0; x-- )
    {
#if DISABLING_CLIP_FOR_BIPREDME
      pDstU[x ] = (pDstU[x ]<<1) - pSrcU[x ] ;
      pDstV[x ] = (pDstV[x ]<<1) - pSrcV[x ] ;
#else
      pDstU[x ] = Clip( (pDstU[x ]<<1) - pSrcU[x ] );
      pDstV[x ] = Clip( (pDstV[x ]<<1) - pSrcV[x ] );
#endif
    }
    pSrcU += iSrcStride;
    pSrcV += iSrcStride;
    pDstU += iDstStride;
    pDstV += iDstStride;
  }
}
//! \}
