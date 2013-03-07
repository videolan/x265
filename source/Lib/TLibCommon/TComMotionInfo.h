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

/** \file     TComMotionInfo.h
    \brief    motion information handling classes (header)
    \todo     TComMvField seems to be better to be inherited from TComMv
*/

#ifndef __TCOMMOTIONINFO__
#define __TCOMMOTIONINFO__

#include <memory.h>
#include "CommonDef.h"
#include "TComMv.h"

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Type definition
// ====================================================================================================================

/// parameters for AMVP
typedef struct _AMVPInfo
{
  TComMv m_acMvCand[ AMVP_MAX_NUM_CANDS_MEM ];  ///< array of motion vector predictor candidates
  Int    iN;                                ///< number of motion vector predictor candidates
} AMVPInfo;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// class for motion vector with reference index
class TComMvField
{
private:
  TComMv    m_acMv;
  Int       m_iRefIdx;
  
public:
  TComMvField() : m_iRefIdx( NOT_VALID ) {}
  
  Void setMvField( TComMv const & cMv, Int iRefIdx )
  {
    m_acMv    = cMv;
    m_iRefIdx = iRefIdx;
  }
  
  Void setRefIdx( Int refIdx ) { m_iRefIdx = refIdx; }
  
  TComMv const & getMv() const { return  m_acMv; }
  TComMv       & getMv()       { return  m_acMv; }
  
  Int getRefIdx() const { return  m_iRefIdx;       }
  Int getHor   () const { return  m_acMv.getHor(); }
  Int getVer   () const { return  m_acMv.getVer(); }
};

/// class for motion information in one CU
class TComCUMvField
{
private:
  TComMv*   m_pcMv;
  TComMv*   m_pcMvd;
  Char*     m_piRefIdx;
  UInt      m_uiNumPartition;
  AMVPInfo  m_cAMVPInfo;
    
  template <typename T>
  Void setAll( T *p, T const & val, PartSize eCUMode, Int iPartAddr, UInt uiDepth, Int iPartIdx );

public:
  TComCUMvField() : m_pcMv(NULL), m_pcMvd(NULL), m_piRefIdx(NULL), m_uiNumPartition(0) {}
  ~TComCUMvField() {}

  // ------------------------------------------------------------------------------------------------------------------
  // create / destroy
  // ------------------------------------------------------------------------------------------------------------------
  
  Void    create( UInt uiNumPartition );
  Void    destroy();
  
  // ------------------------------------------------------------------------------------------------------------------
  // clear / copy
  // ------------------------------------------------------------------------------------------------------------------

  Void    clearMvField();
  
  Void    copyFrom( TComCUMvField const * pcCUMvFieldSrc, Int iNumPartSrc, Int iPartAddrDst );
  Void    copyTo  ( TComCUMvField* pcCUMvFieldDst, Int iPartAddrDst ) const;
  Void    copyTo  ( TComCUMvField* pcCUMvFieldDst, Int iPartAddrDst, UInt uiOffset, UInt uiNumPart ) const;
  
  // ------------------------------------------------------------------------------------------------------------------
  // get
  // ------------------------------------------------------------------------------------------------------------------
  
  TComMv const & getMv    ( Int iIdx ) const { return  m_pcMv    [iIdx]; }
  TComMv const & getMvd   ( Int iIdx ) const { return  m_pcMvd   [iIdx]; }
  Int            getRefIdx( Int iIdx ) const { return  m_piRefIdx[iIdx]; }
  
  AMVPInfo* getAMVPInfo () { return &m_cAMVPInfo; }
  
  // ------------------------------------------------------------------------------------------------------------------
  // set
  // ------------------------------------------------------------------------------------------------------------------
  
  Void    setAllMv     ( TComMv const & rcMv,         PartSize eCUMode, Int iPartAddr, UInt uiDepth, Int iPartIdx=0 );
  Void    setAllMvd    ( TComMv const & rcMvd,        PartSize eCUMode, Int iPartAddr, UInt uiDepth, Int iPartIdx=0 );
  Void    setAllRefIdx ( Int iRefIdx,                 PartSize eMbMode, Int iPartAddr, UInt uiDepth, Int iPartIdx=0 );
  Void    setAllMvField( TComMvField const & mvField, PartSize eMbMode, Int iPartAddr, UInt uiDepth, Int iPartIdx=0 );

  Void setNumPartition( Int iNumPart )
  {
    m_uiNumPartition = iNumPart;
  }
  
  Void linkToWithOffset( TComCUMvField const * src, Int offset )
  {
    m_pcMv     = src->m_pcMv     + offset;
    m_pcMvd    = src->m_pcMvd    + offset;
    m_piRefIdx = src->m_piRefIdx + offset;
  }
  
  Void compress(Char* pePredMode, Int scale); 
};

//! \}

#endif // __TCOMMOTIONINFO__
