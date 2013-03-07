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

/** \file     ContextModel3DBuffer.h
    \brief    context model 3D buffer class (header)
*/

#ifndef _HM_CONTEXT_MODEL_3DBUFFER_H_
#define _HM_CONTEXT_MODEL_3DBUFFER_H_

#include <stdio.h>
#include <assert.h>
#include <memory.h>

#include "CommonDef.h"
#include "ContextModel.h"

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// context model 3D buffer class
class ContextModel3DBuffer
{
protected:
  ContextModel* m_contextModel; ///< array of context models
  const UInt    m_sizeX;        ///< X size of 3D buffer
  const UInt    m_sizeXY;       ///< X times Y size of 3D buffer
  const UInt    m_sizeXYZ;      ///< total size of 3D buffer
  
public:
  ContextModel3DBuffer  ( UInt uiSizeZ, UInt uiSizeY, UInt uiSizeX, ContextModel *basePtr, Int &count );
  ~ContextModel3DBuffer () {}
  
  // access functions
  ContextModel& get( UInt uiZ, UInt uiY, UInt uiX )
  {
    return  m_contextModel[ uiZ * m_sizeXY + uiY * m_sizeX + uiX ];
  }
  ContextModel* get( UInt uiZ, UInt uiY )
  {
    return &m_contextModel[ uiZ * m_sizeXY + uiY * m_sizeX ];
  }
  ContextModel* get( UInt uiZ )
  {
    return &m_contextModel[ uiZ * m_sizeXY ];
  }
  
  // initialization & copy functions
  Void initBuffer( SliceType eSliceType, Int iQp, UChar* ctxModel );          ///< initialize 3D buffer by slice type & QP
  
  UInt calcCost( SliceType sliceType, Int qp, UChar* ctxModel );      ///< determine cost of choosing a probability table based on current probabilities
  /** copy from another buffer
   * \param src buffer to copy from
   */
  Void copyFrom( ContextModel3DBuffer* src )
  {
    assert( m_sizeXYZ == src->m_sizeXYZ );
    ::memcpy( m_contextModel, src->m_contextModel, sizeof(ContextModel) * m_sizeXYZ );
  }
};

//! \}

#endif // _HM_CONTEXT_MODEL_3DBUFFER_H_
