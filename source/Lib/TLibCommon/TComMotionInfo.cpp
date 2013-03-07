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

/** \file     TComMotionInfo.cpp
    \brief    motion information handling classes
*/

#include <memory.h>
#include "TComMotionInfo.h"
#include "assert.h"
#include <stdlib.h>

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

// --------------------------------------------------------------------------------------------------------------------
// Create / destroy
// --------------------------------------------------------------------------------------------------------------------

Void TComCUMvField::create( UInt uiNumPartition )
{
  assert(m_pcMv     == NULL);
  assert(m_pcMvd    == NULL);
  assert(m_piRefIdx == NULL);
  
  m_pcMv     = new TComMv[ uiNumPartition ];
  m_pcMvd    = new TComMv[ uiNumPartition ];
  m_piRefIdx = new Char  [ uiNumPartition ];
  
  m_uiNumPartition = uiNumPartition;
}

Void TComCUMvField::destroy()
{
  assert(m_pcMv     != NULL);
  assert(m_pcMvd    != NULL);
  assert(m_piRefIdx != NULL);
  
  delete[] m_pcMv;
  delete[] m_pcMvd;
  delete[] m_piRefIdx;
  
  m_pcMv     = NULL;
  m_pcMvd    = NULL;
  m_piRefIdx = NULL;
  
  m_uiNumPartition = 0;
}

// --------------------------------------------------------------------------------------------------------------------
// Clear / copy
// --------------------------------------------------------------------------------------------------------------------

Void TComCUMvField::clearMvField()
{
  for ( Int i = 0; i < m_uiNumPartition; i++ )
  {
    m_pcMv [ i ].setZero();
    m_pcMvd[ i ].setZero();      
  }
  assert( sizeof( *m_piRefIdx ) == 1 );
  memset( m_piRefIdx, NOT_VALID, m_uiNumPartition * sizeof( *m_piRefIdx ) );
}

Void TComCUMvField::copyFrom( TComCUMvField const * pcCUMvFieldSrc, Int iNumPartSrc, Int iPartAddrDst )
{
  Int iSizeInTComMv = sizeof( TComMv ) * iNumPartSrc;
  
  memcpy( m_pcMv     + iPartAddrDst, pcCUMvFieldSrc->m_pcMv,     iSizeInTComMv );
  memcpy( m_pcMvd    + iPartAddrDst, pcCUMvFieldSrc->m_pcMvd,    iSizeInTComMv );
  memcpy( m_piRefIdx + iPartAddrDst, pcCUMvFieldSrc->m_piRefIdx, sizeof( *m_piRefIdx ) * iNumPartSrc );
}

Void TComCUMvField::copyTo( TComCUMvField* pcCUMvFieldDst, Int iPartAddrDst ) const
{
  copyTo( pcCUMvFieldDst, iPartAddrDst, 0, m_uiNumPartition );
}

Void TComCUMvField::copyTo( TComCUMvField* pcCUMvFieldDst, Int iPartAddrDst, UInt uiOffset, UInt uiNumPart ) const
{
  Int iSizeInTComMv = sizeof( TComMv ) * uiNumPart;
  Int iOffset = uiOffset + iPartAddrDst;
  
  memcpy( pcCUMvFieldDst->m_pcMv     + iOffset, m_pcMv     + uiOffset, iSizeInTComMv );
  memcpy( pcCUMvFieldDst->m_pcMvd    + iOffset, m_pcMvd    + uiOffset, iSizeInTComMv );
  memcpy( pcCUMvFieldDst->m_piRefIdx + iOffset, m_piRefIdx + uiOffset, sizeof( *m_piRefIdx ) * uiNumPart );
}

// --------------------------------------------------------------------------------------------------------------------
// Set
// --------------------------------------------------------------------------------------------------------------------

template <typename T>
Void TComCUMvField::setAll( T *p, T const & val, PartSize eCUMode, Int iPartAddr, UInt uiDepth, Int iPartIdx  )
{
  Int i;
  p += iPartAddr;
  Int numElements = m_uiNumPartition >> ( 2 * uiDepth );
  
  switch( eCUMode )
  {
    case SIZE_2Nx2N:
      for ( i = 0; i < numElements; i++ )
      {
        p[ i ] = val;
      }
      break;
      
    case SIZE_2NxN:
      numElements >>= 1;
      for ( i = 0; i < numElements; i++ )
      {
        p[ i ] = val;
      }
      break;
      
    case SIZE_Nx2N:
      numElements >>= 2;
      for ( i = 0; i < numElements; i++ )
      {
        p[ i                   ] = val;
        p[ i + 2 * numElements ] = val;
      }
      break;
      
    case SIZE_NxN:
      numElements >>= 2;
      for ( i = 0; i < numElements; i++)
      {
        p[ i ] = val;
      }
      break;
    case SIZE_2NxnU:
    {
      Int iCurrPartNumQ = numElements>>2;
      if( iPartIdx == 0 )
      {
        T *pT  = p;
        T *pT2 = p + iCurrPartNumQ;
        for (i = 0; i < (iCurrPartNumQ>>1); i++)
        {
          pT [i] = val;
          pT2[i] = val;
        }
      }
      else
      {
        T *pT  = p;
        for (i = 0; i < (iCurrPartNumQ>>1); i++)
        {
          pT[i] = val;
        }

        pT = p + iCurrPartNumQ;
        for (i = 0; i < ( (iCurrPartNumQ>>1) + (iCurrPartNumQ<<1) ); i++)
        {
          pT[i] = val;
        }
      }
      break;
    }
  case SIZE_2NxnD:
    {
      Int iCurrPartNumQ = numElements>>2;
      if( iPartIdx == 0 )
      {
        T *pT  = p;
        for (i = 0; i < ( (iCurrPartNumQ>>1) + (iCurrPartNumQ<<1) ); i++)
        {
          pT[i] = val;
        }
        pT = p + ( numElements - iCurrPartNumQ );
        for (i = 0; i < (iCurrPartNumQ>>1); i++)
        {
          pT[i] = val;
        }
      }
      else
      {
        T *pT  = p;
        T *pT2 = p + iCurrPartNumQ;
        for (i = 0; i < (iCurrPartNumQ>>1); i++)
        {
          pT [i] = val;
          pT2[i] = val;
        }
      }
      break;
    }
  case SIZE_nLx2N:
    {
      Int iCurrPartNumQ = numElements>>2;
      if( iPartIdx == 0 )
      {
        T *pT  = p;
        T *pT2 = p + (iCurrPartNumQ<<1);
        T *pT3 = p + (iCurrPartNumQ>>1);
        T *pT4 = p + (iCurrPartNumQ<<1) + (iCurrPartNumQ>>1);

        for (i = 0; i < (iCurrPartNumQ>>2); i++)
        {
          pT [i] = val;
          pT2[i] = val;
          pT3[i] = val;
          pT4[i] = val;
        }
      }
      else
      {
        T *pT  = p;
        T *pT2 = p + (iCurrPartNumQ<<1);
        for (i = 0; i < (iCurrPartNumQ>>2); i++)
        {
          pT [i] = val;
          pT2[i] = val;
        }

        pT  = p + (iCurrPartNumQ>>1);
        pT2 = p + (iCurrPartNumQ<<1) + (iCurrPartNumQ>>1);
        for (i = 0; i < ( (iCurrPartNumQ>>2) + iCurrPartNumQ ); i++)
        {
          pT [i] = val;
          pT2[i] = val;
        }
      }
      break;
    }
  case SIZE_nRx2N:
    {
      Int iCurrPartNumQ = numElements>>2;
      if( iPartIdx == 0 )
      {
        T *pT  = p;
        T *pT2 = p + (iCurrPartNumQ<<1);
        for (i = 0; i < ( (iCurrPartNumQ>>2) + iCurrPartNumQ ); i++)
        {
          pT [i] = val;
          pT2[i] = val;
        }

        pT  = p + iCurrPartNumQ + (iCurrPartNumQ>>1);
        pT2 = p + numElements - iCurrPartNumQ + (iCurrPartNumQ>>1);
        for (i = 0; i < (iCurrPartNumQ>>2); i++)
        {
          pT [i] = val;
          pT2[i] = val;
        }
      }
      else
      {
        T *pT  = p;
        T *pT2 = p + (iCurrPartNumQ>>1);
        T *pT3 = p + (iCurrPartNumQ<<1);
        T *pT4 = p + (iCurrPartNumQ<<1) + (iCurrPartNumQ>>1);
        for (i = 0; i < (iCurrPartNumQ>>2); i++)
        {
          pT [i] = val;
          pT2[i] = val;
          pT3[i] = val;
          pT4[i] = val;
        }
      }
      break;
    }
    default:
      assert(0);
      break;
  }
}

Void TComCUMvField::setAllMv( TComMv const & mv, PartSize eCUMode, Int iPartAddr, UInt uiDepth, Int iPartIdx )
{
  setAll(m_pcMv, mv, eCUMode, iPartAddr, uiDepth, iPartIdx);
}

Void TComCUMvField::setAllMvd( TComMv const & mvd, PartSize eCUMode, Int iPartAddr, UInt uiDepth, Int iPartIdx )
{
  setAll(m_pcMvd, mvd, eCUMode, iPartAddr, uiDepth, iPartIdx);
}

Void TComCUMvField::setAllRefIdx ( Int iRefIdx, PartSize eCUMode, Int iPartAddr, UInt uiDepth, Int iPartIdx )
{
  setAll(m_piRefIdx, static_cast<Char>(iRefIdx), eCUMode, iPartAddr, uiDepth, iPartIdx);
}

Void TComCUMvField::setAllMvField( TComMvField const & mvField, PartSize eCUMode, Int iPartAddr, UInt uiDepth, Int iPartIdx )
{
  setAllMv    ( mvField.getMv(),     eCUMode, iPartAddr, uiDepth, iPartIdx );
  setAllRefIdx( mvField.getRefIdx(), eCUMode, iPartAddr, uiDepth, iPartIdx );
}

/**Subsampling of the stored prediction mode, reference index and motion vector
 * \param pePredMode Pointer to prediction modes
 * \param scale      Factor by which to subsample motion information
 */
Void TComCUMvField::compress(Char* pePredMode, Int scale)
{
  Int N = scale * scale;
  assert( N > 0 && N <= m_uiNumPartition);
  
  for ( Int uiPartIdx = 0; uiPartIdx < m_uiNumPartition; uiPartIdx += N )
  {
    TComMv cMv(0,0); 
    PredMode predMode = MODE_INTRA;
    Int iRefIdx = 0;
    
    cMv = m_pcMv[ uiPartIdx ];
    predMode = static_cast<PredMode>( pePredMode[ uiPartIdx ] );
    iRefIdx = m_piRefIdx[ uiPartIdx ];
    for ( Int i = 0; i < N; i++ )
    {
      m_pcMv[ uiPartIdx + i ] = cMv;
      pePredMode[ uiPartIdx + i ] = predMode;
      m_piRefIdx[ uiPartIdx + i ] = iRefIdx;
    }
  }
} 
//! \}
