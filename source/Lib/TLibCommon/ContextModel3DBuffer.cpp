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

/** \file     ContextModel3DBuffer.cpp
    \brief    context model 3D buffer class
*/

#include "ContextModel3DBuffer.h"

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Constructor / destructor / initialization / destroy
// ====================================================================================================================

ContextModel3DBuffer::ContextModel3DBuffer( UInt uiSizeZ, UInt uiSizeY, UInt uiSizeX, ContextModel *basePtr, Int &count )
: m_sizeX  ( uiSizeX )
, m_sizeXY ( uiSizeX * uiSizeY )
, m_sizeXYZ( uiSizeX * uiSizeY * uiSizeZ )
{
  // allocate 3D buffer
  m_contextModel = basePtr;
  count += m_sizeXYZ;
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/**
 * Initialize 3D buffer with respect to slicetype, QP and given initial probability table
 *
 * \param  eSliceType      slice type
 * \param  iQp             input QP value
 * \param  psCtxModel      given probability table
 */
Void ContextModel3DBuffer::initBuffer( SliceType sliceType, Int qp, UChar* ctxModel )
{
  ctxModel += sliceType * m_sizeXYZ;
  
  for ( Int n = 0; n < m_sizeXYZ; n++ )
  {
    m_contextModel[ n ].init( qp, ctxModel[ n ] );
    m_contextModel[ n ].setBinsCoded( 0 );
  }
}

/**
 * Calculate the cost of choosing a probability table based on the current probability of CABAC at encoder
 *
 * \param  sliceType      slice type
 * \param  qp             input QP value
 * \param  ctxModel      given probability table
 */
UInt ContextModel3DBuffer::calcCost( SliceType sliceType, Int qp, UChar* ctxModel )
{
  UInt cost = 0;
  ctxModel += sliceType * m_sizeXYZ;

  for ( Int n = 0; n < m_sizeXYZ; n++ )
  {
    ContextModel tmpContextModel;
    tmpContextModel.init( qp, ctxModel[ n ] );

    // Map the 64 CABAC states to their corresponding probability values
    static Double aStateToProbLPS[] = {0.50000000, 0.47460857, 0.45050660, 0.42762859, 0.40591239, 0.38529900, 0.36573242, 0.34715948, 0.32952974, 0.31279528, 0.29691064, 0.28183267, 0.26752040, 0.25393496, 0.24103941, 0.22879875, 0.21717969, 0.20615069, 0.19568177, 0.18574449, 0.17631186, 0.16735824, 0.15885931, 0.15079198, 0.14313433, 0.13586556, 0.12896592, 0.12241667, 0.11620000, 0.11029903, 0.10469773, 0.09938088, 0.09433404, 0.08954349, 0.08499621, 0.08067986, 0.07658271, 0.07269362, 0.06900203, 0.06549791, 0.06217174, 0.05901448, 0.05601756, 0.05317283, 0.05047256, 0.04790942, 0.04547644, 0.04316702, 0.04097487, 0.03889405, 0.03691890, 0.03504406, 0.03326442, 0.03157516, 0.02997168, 0.02844963, 0.02700488, 0.02563349, 0.02433175, 0.02309612, 0.02192323, 0.02080991, 0.01975312, 0.01875000};

    Double probLPS          = aStateToProbLPS[ m_contextModel[ n ].getState() ];
    Double prob0, prob1;
    if (m_contextModel[ n ].getMps()==1)
    {
      prob0 = probLPS;
      prob1 = 1.0-prob0;
    }
    else
    {
      prob1 = probLPS;
      prob0 = 1.0-prob1;
    }

    if (m_contextModel[ n ].getBinsCoded()>0)
    {
      cost += (UInt) (prob0 * tmpContextModel.getEntropyBits( 0 ) + prob1 * tmpContextModel.getEntropyBits( 1 ));
    }
  }

  return cost;
}
//! \}
