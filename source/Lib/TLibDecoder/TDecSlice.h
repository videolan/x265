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

/** \file     TDecSlice.h
    \brief    slice decoder class (header)
*/

#ifndef __TDECSLICE__
#define __TDECSLICE__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TLibCommon/CommonDef.h"
#include "TLibCommon/TComBitStream.h"
#include "TLibCommon/TComPic.h"
#include "TDecEntropy.h"
#include "TDecCu.h"
#include "TDecSbac.h"
#include "TDecBinCoderCABAC.h"

//! \ingroup TLibDecoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// slice decoder class
class TDecSlice
{
private:
  // access channel
  TDecEntropy*    m_pcEntropyDecoder;
  TDecCu*         m_pcCuDecoder;
  UInt            m_uiCurrSliceIdx;

  TDecSbac*       m_pcBufferSbacDecoders;   ///< line to store temporary contexts, one per column of tiles.
  TDecBinCABAC*   m_pcBufferBinCABACs;
  TDecSbac*       m_pcBufferLowLatSbacDecoders;   ///< dependent tiles: line to store temporary contexts, one per column of tiles.
  TDecBinCABAC*   m_pcBufferLowLatBinCABACs;
  std::vector<TDecSbac*> CTXMem;
  
public:
  TDecSlice();
  virtual ~TDecSlice();
  
  Void  init              ( TDecEntropy* pcEntropyDecoder, TDecCu* pcMbDecoder );
  Void  create            ();
  Void  destroy           ();
  
  Void  decompressSlice   ( TComInputBitstream** ppcSubstreams,   TComPic*& rpcPic, TDecSbac* pcSbacDecoder, TDecSbac* pcSbacDecoders );
  Void      initCtxMem(  UInt i );
  Void      setCtxMem( TDecSbac* sb, Int b )   { CTXMem[b] = sb; }
};


class ParameterSetManagerDecoder:public ParameterSetManager
{
public:
  ParameterSetManagerDecoder();
  virtual ~ParameterSetManagerDecoder();
  Void     storePrefetchedVPS(TComVPS *vps)  { m_vpsBuffer.storePS( vps->getVPSId(), vps); };
  TComVPS* getPrefetchedVPS  (Int vpsId);
  Void     storePrefetchedSPS(TComSPS *sps)  { m_spsBuffer.storePS( sps->getSPSId(), sps); };
  TComSPS* getPrefetchedSPS  (Int spsId);
  Void     storePrefetchedPPS(TComPPS *pps)  { m_ppsBuffer.storePS( pps->getPPSId(), pps); };
  TComPPS* getPrefetchedPPS  (Int ppsId);
  Void     applyPrefetchedPS();

private:
  ParameterSetMap<TComVPS> m_vpsBuffer;
  ParameterSetMap<TComSPS> m_spsBuffer; 
  ParameterSetMap<TComPPS> m_ppsBuffer;
};


//! \}

#endif

