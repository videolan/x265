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


/** \file     ContextModel.h
    \brief    context model class (header)
*/

#ifndef __CONTEXT_MODEL__
#define __CONTEXT_MODEL__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "CommonDef.h"

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// context model class
class ContextModel
{
public:
  ContextModel  ()                        { m_ucState = 0; m_binsCoded = 0; }
  ~ContextModel ()                        {}
  
  UChar getState  ()                { return ( m_ucState >> 1 ); }                    ///< get current state
  UChar getMps    ()                { return ( m_ucState  & 1 ); }                    ///< get curret MPS
  Void  setStateAndMps( UChar ucState, UChar ucMPS) { m_ucState = (ucState << 1) + ucMPS; } ///< set state and MPS
  
  Void init ( Int qp, Int initValue );   ///< initialize state with initial probability
  
  Void updateLPS ()
  {
    m_ucState = m_aucNextStateLPS[ m_ucState ];
  }
  
  Void updateMPS ()
  {
    m_ucState = m_aucNextStateMPS[ m_ucState ];
  }
  
  Int getEntropyBits(Short val) { return m_entropyBits[m_ucState ^ val]; }
    
#if FAST_BIT_EST
  Void update( Int binVal )
  {
    m_ucState = m_nextState[m_ucState][binVal];
  }
  static Void buildNextStateTable();
  static Int getEntropyBitsTrm( Int val ) { return m_entropyBits[126 ^ val]; }
#endif
  Void setBinsCoded(UInt val)   { m_binsCoded = val;  }
  UInt getBinsCoded()           { return m_binsCoded;   }
  
private:
  UChar         m_ucState;                                                                  ///< internal state variable
  static const  UChar m_aucNextStateMPS[ 128 ];
  static const  UChar m_aucNextStateLPS[ 128 ];
  static const Int m_entropyBits[ 128 ];
#if FAST_BIT_EST
  static UChar m_nextState[128][2];
#endif
  UInt          m_binsCoded;
};

//! \}

#endif

