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

/** \file     TComMv.h
    \brief    motion vector class (header)
*/

#ifndef __TCOMMV__
#define __TCOMMV__

#include "CommonDef.h"

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// basic motion vector class
class TComMv
{
private:
  Short m_iHor;     ///< horizontal component of motion vector
  Short m_iVer;     ///< vertical component of motion vector
  
public:
  
  // ------------------------------------------------------------------------------------------------------------------
  // constructors
  // ------------------------------------------------------------------------------------------------------------------
  
  TComMv() :
  m_iHor(0),
  m_iVer(0)
  {
  }
  
  TComMv( Short iHor, Short iVer ) :
  m_iHor(iHor),
  m_iVer(iVer)
  {
  }
  
  // ------------------------------------------------------------------------------------------------------------------
  // set
  // ------------------------------------------------------------------------------------------------------------------
  
  Void  set       ( Short iHor, Short iVer)     { m_iHor = iHor;  m_iVer = iVer;            }
  Void  setHor    ( Short i )                   { m_iHor = i;                               }
  Void  setVer    ( Short i )                   { m_iVer = i;                               }
  Void  setZero   ()                            { m_iHor = m_iVer = 0;  }
  
  // ------------------------------------------------------------------------------------------------------------------
  // get
  // ------------------------------------------------------------------------------------------------------------------
  
  Int   getHor    () const { return m_iHor;          }
  Int   getVer    () const { return m_iVer;          }
  Int   getAbsHor () const { return abs( m_iHor );   }
  Int   getAbsVer () const { return abs( m_iVer );   }
  
  // ------------------------------------------------------------------------------------------------------------------
  // operations
  // ------------------------------------------------------------------------------------------------------------------
  
  const TComMv& operator += (const TComMv& rcMv)
  {
    m_iHor += rcMv.m_iHor;
    m_iVer += rcMv.m_iVer;
    return  *this;
  }
  
  const TComMv& operator-= (const TComMv& rcMv)
  {
    m_iHor -= rcMv.m_iHor;
    m_iVer -= rcMv.m_iVer;
    return  *this;
  }
  
  const TComMv& operator>>= (const Int i)
  {
    m_iHor >>= i;
    m_iVer >>= i;
    return  *this;
  }
  
  const TComMv& operator<<= (const Int i)
  {
    m_iHor <<= i;
    m_iVer <<= i;
    return  *this;
  }
  
  const TComMv operator - ( const TComMv& rcMv ) const
  {
    return TComMv( m_iHor - rcMv.m_iHor, m_iVer - rcMv.m_iVer );
  }
  
  const TComMv operator + ( const TComMv& rcMv ) const
  {
    return TComMv( m_iHor + rcMv.m_iHor, m_iVer + rcMv.m_iVer );
  }
  
  Bool operator== ( const TComMv& rcMv ) const
  {
    return (m_iHor==rcMv.m_iHor && m_iVer==rcMv.m_iVer);
  }
  
  Bool operator!= ( const TComMv& rcMv ) const
  {
    return (m_iHor!=rcMv.m_iHor || m_iVer!=rcMv.m_iVer);
  }
  
  const TComMv scaleMv( Int iScale ) const
  {
    Int mvx = Clip3( -32768, 32767, (iScale * getHor() + 127 + (iScale * getHor() < 0)) >> 8 );
    Int mvy = Clip3( -32768, 32767, (iScale * getVer() + 127 + (iScale * getVer() < 0)) >> 8 );
    return TComMv( mvx, mvy );
  }
};// END CLASS DEFINITION TComMV

//! \}

#endif // __TCOMMV__
