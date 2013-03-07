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

/** \file     TComPicYuv.cpp
    \brief    picture YUV buffer class
*/

#include <cstdlib>
#include <assert.h>
#include <memory.h>

#ifdef __APPLE__
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif

#include "TComPicYuv.h"

//! \ingroup TLibCommon
//! \{

TComPicYuv::TComPicYuv()
{
  m_apiPicBufY      = NULL;   // Buffer (including margin)
  m_apiPicBufU      = NULL;
  m_apiPicBufV      = NULL;
  
  m_piPicOrgY       = NULL;    // m_apiPicBufY + m_iMarginLuma*getStride() + m_iMarginLuma
  m_piPicOrgU       = NULL;
  m_piPicOrgV       = NULL;
  
  m_bIsBorderExtended = false;
}

TComPicYuv::~TComPicYuv()
{
}

Void TComPicYuv::create( Int iPicWidth, Int iPicHeight, UInt uiMaxCUWidth, UInt uiMaxCUHeight, UInt uiMaxCUDepth )
{
  m_iPicWidth       = iPicWidth;
  m_iPicHeight      = iPicHeight;
  
  // --> After config finished!
  m_iCuWidth        = uiMaxCUWidth;
  m_iCuHeight       = uiMaxCUHeight;

  Int numCuInWidth  = m_iPicWidth  / m_iCuWidth  + (m_iPicWidth  % m_iCuWidth  != 0);
  Int numCuInHeight = m_iPicHeight / m_iCuHeight + (m_iPicHeight % m_iCuHeight != 0);
  
  m_iLumaMarginX    = g_uiMaxCUWidth  + 16; // for 16-byte alignment
  m_iLumaMarginY    = g_uiMaxCUHeight + 16;  // margin for 8-tap filter and infinite padding
  
  m_iChromaMarginX  = m_iLumaMarginX>>1;
  m_iChromaMarginY  = m_iLumaMarginY>>1;
  
  m_apiPicBufY      = (Pel*)xMalloc( Pel, ( m_iPicWidth       + (m_iLumaMarginX  <<1)) * ( m_iPicHeight       + (m_iLumaMarginY  <<1)));
  m_apiPicBufU      = (Pel*)xMalloc( Pel, ((m_iPicWidth >> 1) + (m_iChromaMarginX<<1)) * ((m_iPicHeight >> 1) + (m_iChromaMarginY<<1)));
  m_apiPicBufV      = (Pel*)xMalloc( Pel, ((m_iPicWidth >> 1) + (m_iChromaMarginX<<1)) * ((m_iPicHeight >> 1) + (m_iChromaMarginY<<1)));
  
  m_piPicOrgY       = m_apiPicBufY + m_iLumaMarginY   * getStride()  + m_iLumaMarginX;
  m_piPicOrgU       = m_apiPicBufU + m_iChromaMarginY * getCStride() + m_iChromaMarginX;
  m_piPicOrgV       = m_apiPicBufV + m_iChromaMarginY * getCStride() + m_iChromaMarginX;
  
  m_bIsBorderExtended = false;
  
  m_cuOffsetY = new Int[numCuInWidth * numCuInHeight];
  m_cuOffsetC = new Int[numCuInWidth * numCuInHeight];
  for (Int cuRow = 0; cuRow < numCuInHeight; cuRow++)
  {
    for (Int cuCol = 0; cuCol < numCuInWidth; cuCol++)
    {
      m_cuOffsetY[cuRow * numCuInWidth + cuCol] = getStride() * cuRow * m_iCuHeight + cuCol * m_iCuWidth;
      m_cuOffsetC[cuRow * numCuInWidth + cuCol] = getCStride() * cuRow * (m_iCuHeight / 2) + cuCol * (m_iCuWidth / 2);
    }
  }
  
  m_buOffsetY = new Int[(size_t)1 << (2 * uiMaxCUDepth)];
  m_buOffsetC = new Int[(size_t)1 << (2 * uiMaxCUDepth)];
  for (Int buRow = 0; buRow < (1 << uiMaxCUDepth); buRow++)
  {
    for (Int buCol = 0; buCol < (1 << uiMaxCUDepth); buCol++)
    {
      m_buOffsetY[(buRow << uiMaxCUDepth) + buCol] = getStride() * buRow * (uiMaxCUHeight >> uiMaxCUDepth) + buCol * (uiMaxCUWidth  >> uiMaxCUDepth);
      m_buOffsetC[(buRow << uiMaxCUDepth) + buCol] = getCStride() * buRow * (uiMaxCUHeight / 2 >> uiMaxCUDepth) + buCol * (uiMaxCUWidth / 2 >> uiMaxCUDepth);
    }
  }
  return;
}

Void TComPicYuv::destroy()
{
  m_piPicOrgY       = NULL;
  m_piPicOrgU       = NULL;
  m_piPicOrgV       = NULL;
  
  if( m_apiPicBufY ){ xFree( m_apiPicBufY );    m_apiPicBufY = NULL; }
  if( m_apiPicBufU ){ xFree( m_apiPicBufU );    m_apiPicBufU = NULL; }
  if( m_apiPicBufV ){ xFree( m_apiPicBufV );    m_apiPicBufV = NULL; }

  delete[] m_cuOffsetY;
  delete[] m_cuOffsetC;
  delete[] m_buOffsetY;
  delete[] m_buOffsetC;
}

Void TComPicYuv::createLuma( Int iPicWidth, Int iPicHeight, UInt uiMaxCUWidth, UInt uiMaxCUHeight, UInt uiMaxCUDepth )
{
  m_iPicWidth       = iPicWidth;
  m_iPicHeight      = iPicHeight;
  
  // --> After config finished!
  m_iCuWidth        = uiMaxCUWidth;
  m_iCuHeight       = uiMaxCUHeight;
  
  Int numCuInWidth  = m_iPicWidth  / m_iCuWidth  + (m_iPicWidth  % m_iCuWidth  != 0);
  Int numCuInHeight = m_iPicHeight / m_iCuHeight + (m_iPicHeight % m_iCuHeight != 0);
  
  m_iLumaMarginX    = g_uiMaxCUWidth  + 16; // for 16-byte alignment
  m_iLumaMarginY    = g_uiMaxCUHeight + 16;  // margin for 8-tap filter and infinite padding
  
  m_apiPicBufY      = (Pel*)xMalloc( Pel, ( m_iPicWidth       + (m_iLumaMarginX  <<1)) * ( m_iPicHeight       + (m_iLumaMarginY  <<1)));
  m_piPicOrgY       = m_apiPicBufY + m_iLumaMarginY   * getStride()  + m_iLumaMarginX;
  
  m_cuOffsetY = new Int[numCuInWidth * numCuInHeight];
  m_cuOffsetC = NULL;
  for (Int cuRow = 0; cuRow < numCuInHeight; cuRow++)
  {
    for (Int cuCol = 0; cuCol < numCuInWidth; cuCol++)
    {
      m_cuOffsetY[cuRow * numCuInWidth + cuCol] = getStride() * cuRow * m_iCuHeight + cuCol * m_iCuWidth;
    }
  }
  
  m_buOffsetY = new Int[(size_t)1 << (2 * uiMaxCUDepth)];
  m_buOffsetC = NULL;
  for (Int buRow = 0; buRow < (1 << uiMaxCUDepth); buRow++)
  {
    for (Int buCol = 0; buCol < (1 << uiMaxCUDepth); buCol++)
    {
      m_buOffsetY[(buRow << uiMaxCUDepth) + buCol] = getStride() * buRow * (uiMaxCUHeight >> uiMaxCUDepth) + buCol * (uiMaxCUWidth  >> uiMaxCUDepth);
    }
  }
  return;
}

Void TComPicYuv::destroyLuma()
{
  m_piPicOrgY       = NULL;
  
  if( m_apiPicBufY ){ xFree( m_apiPicBufY );    m_apiPicBufY = NULL; }
  
  delete[] m_cuOffsetY;
  delete[] m_buOffsetY;
}

Void  TComPicYuv::copyToPic (TComPicYuv*  pcPicYuvDst)
{
  assert( m_iPicWidth  == pcPicYuvDst->getWidth()  );
  assert( m_iPicHeight == pcPicYuvDst->getHeight() );
  
  ::memcpy ( pcPicYuvDst->getBufY(), m_apiPicBufY, sizeof (Pel) * ( m_iPicWidth       + (m_iLumaMarginX   << 1)) * ( m_iPicHeight       + (m_iLumaMarginY   << 1)) );
  ::memcpy ( pcPicYuvDst->getBufU(), m_apiPicBufU, sizeof (Pel) * ((m_iPicWidth >> 1) + (m_iChromaMarginX << 1)) * ((m_iPicHeight >> 1) + (m_iChromaMarginY << 1)) );
  ::memcpy ( pcPicYuvDst->getBufV(), m_apiPicBufV, sizeof (Pel) * ((m_iPicWidth >> 1) + (m_iChromaMarginX << 1)) * ((m_iPicHeight >> 1) + (m_iChromaMarginY << 1)) );
  return;
}

Void  TComPicYuv::copyToPicLuma (TComPicYuv*  pcPicYuvDst)
{
  assert( m_iPicWidth  == pcPicYuvDst->getWidth()  );
  assert( m_iPicHeight == pcPicYuvDst->getHeight() );
  
  ::memcpy ( pcPicYuvDst->getBufY(), m_apiPicBufY, sizeof (Pel) * ( m_iPicWidth       + (m_iLumaMarginX   << 1)) * ( m_iPicHeight       + (m_iLumaMarginY   << 1)) );
  return;
}

Void  TComPicYuv::copyToPicCb (TComPicYuv*  pcPicYuvDst)
{
  assert( m_iPicWidth  == pcPicYuvDst->getWidth()  );
  assert( m_iPicHeight == pcPicYuvDst->getHeight() );
  
  ::memcpy ( pcPicYuvDst->getBufU(), m_apiPicBufU, sizeof (Pel) * ((m_iPicWidth >> 1) + (m_iChromaMarginX << 1)) * ((m_iPicHeight >> 1) + (m_iChromaMarginY << 1)) );
  return;
}

Void  TComPicYuv::copyToPicCr (TComPicYuv*  pcPicYuvDst)
{
  assert( m_iPicWidth  == pcPicYuvDst->getWidth()  );
  assert( m_iPicHeight == pcPicYuvDst->getHeight() );
  
  ::memcpy ( pcPicYuvDst->getBufV(), m_apiPicBufV, sizeof (Pel) * ((m_iPicWidth >> 1) + (m_iChromaMarginX << 1)) * ((m_iPicHeight >> 1) + (m_iChromaMarginY << 1)) );
  return;
}

Void TComPicYuv::extendPicBorder ()
{
  if ( m_bIsBorderExtended ) return;
  
  xExtendPicCompBorder( getLumaAddr(), getStride(),  getWidth(),      getHeight(),      m_iLumaMarginX,   m_iLumaMarginY   );
  xExtendPicCompBorder( getCbAddr()  , getCStride(), getWidth() >> 1, getHeight() >> 1, m_iChromaMarginX, m_iChromaMarginY );
  xExtendPicCompBorder( getCrAddr()  , getCStride(), getWidth() >> 1, getHeight() >> 1, m_iChromaMarginX, m_iChromaMarginY );
  
  m_bIsBorderExtended = true;
}

Void TComPicYuv::xExtendPicCompBorder  (Pel* piTxt, Int iStride, Int iWidth, Int iHeight, Int iMarginX, Int iMarginY)
{
  Int   x, y;
  Pel*  pi;
  
  pi = piTxt;
  for ( y = 0; y < iHeight; y++)
  {
    for ( x = 0; x < iMarginX; x++ )
    {
      pi[ -iMarginX + x ] = pi[0];
      pi[    iWidth + x ] = pi[iWidth-1];
    }
    pi += iStride;
  }
  
  pi -= (iStride + iMarginX);
  for ( y = 0; y < iMarginY; y++ )
  {
    ::memcpy( pi + (y+1)*iStride, pi, sizeof(Pel)*(iWidth + (iMarginX<<1)) );
  }
  
  pi -= ((iHeight-1) * iStride);
  for ( y = 0; y < iMarginY; y++ )
  {
    ::memcpy( pi - (y+1)*iStride, pi, sizeof(Pel)*(iWidth + (iMarginX<<1)) );
  }
}


Void TComPicYuv::dump (Char* pFileName, Bool bAdd)
{
  FILE* pFile;
  if (!bAdd)
  {
    pFile = fopen (pFileName, "wb");
  }
  else
  {
    pFile = fopen (pFileName, "ab");
  }
  
  Int     shift = g_bitDepthY-8;
  Int     offset = (shift>0)?(1<<(shift-1)):0;
  
  Int   x, y;
  UChar uc;
  
  Pel*  piY   = getLumaAddr();
  Pel*  piCb  = getCbAddr();
  Pel*  piCr  = getCrAddr();
  
  for ( y = 0; y < m_iPicHeight; y++ )
  {
    for ( x = 0; x < m_iPicWidth; x++ )
    {
      uc = (UChar)Clip3<Pel>(0, 255, (piY[x]+offset)>>shift);
      
      fwrite( &uc, sizeof(UChar), 1, pFile );
    }
    piY += getStride();
  }
  
  shift = g_bitDepthC-8;
  offset = (shift>0)?(1<<(shift-1)):0;

  for ( y = 0; y < m_iPicHeight >> 1; y++ )
  {
    for ( x = 0; x < m_iPicWidth >> 1; x++ )
    {
      uc = (UChar)Clip3<Pel>(0, 255, (piCb[x]+offset)>>shift);
      fwrite( &uc, sizeof(UChar), 1, pFile );
    }
    piCb += getCStride();
  }
  
  for ( y = 0; y < m_iPicHeight >> 1; y++ )
  {
    for ( x = 0; x < m_iPicWidth >> 1; x++ )
    {
      uc = (UChar)Clip3<Pel>(0, 255, (piCr[x]+offset)>>shift);
      fwrite( &uc, sizeof(UChar), 1, pFile );
    }
    piCr += getCStride();
  }
  
  fclose(pFile);
}


//! \}
