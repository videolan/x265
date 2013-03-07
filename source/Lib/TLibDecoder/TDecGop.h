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

/** \file     TDecGop.h
    \brief    GOP decoder class (header)
*/

#ifndef __TDECGOP__
#define __TDECGOP__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TLibCommon/CommonDef.h"
#include "TLibCommon/TComBitStream.h"
#include "TLibCommon/TComList.h"
#include "TLibCommon/TComPicYuv.h"
#include "TLibCommon/TComPic.h"
#include "TLibCommon/TComLoopFilter.h"
#include "TLibCommon/TComSampleAdaptiveOffset.h"

#include "TDecEntropy.h"
#include "TDecSlice.h"
#include "TDecBinCoder.h"
#include "TDecBinCoderCABAC.h"

//! \ingroup TLibDecoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// GOP decoder class
class TDecGop
{
private:
  TComList<TComPic*>    m_cListPic;         //  Dynamic buffer
  
  //  Access channel
  TDecEntropy*          m_pcEntropyDecoder;
  TDecSbac*             m_pcSbacDecoder;
  TDecBinCABAC*         m_pcBinCABAC;
  TDecSbac*             m_pcSbacDecoders; // independant CABAC decoders
  TDecBinCABAC*         m_pcBinCABACs;
  TDecCavlc*            m_pcCavlcDecoder;
  TDecSlice*            m_pcSliceDecoder;
  TComLoopFilter*       m_pcLoopFilter;
  
  TComSampleAdaptiveOffset*     m_pcSAO;
  Double                m_dDecTime;
  Int                   m_decodedPictureHashSEIEnabled;  ///< Checksum(3)/CRC(2)/MD5(1)/disable(0) acting on decoded picture hash SEI message

  //! list that contains the CU address of each slice plus the end address 
  std::vector<Int> m_sliceStartCUAddress;
  std::vector<Bool> m_LFCrossSliceBoundaryFlag;

public:
  TDecGop();
  virtual ~TDecGop();
  
  Void  init    ( TDecEntropy*            pcEntropyDecoder, 
                 TDecSbac*               pcSbacDecoder, 
                 TDecBinCABAC*           pcBinCABAC,
                 TDecCavlc*              pcCavlcDecoder, 
                 TDecSlice*              pcSliceDecoder, 
                 TComLoopFilter*         pcLoopFilter,
                 TComSampleAdaptiveOffset* pcSAO
                 );
  Void  create  ();
  Void  destroy ();
  Void  decompressSlice(TComInputBitstream* pcBitstream, TComPic*& rpcPic );
  Void  filterPicture  (TComPic*& rpcPic );

  void setDecodedPictureHashSEIEnabled(Int enabled) { m_decodedPictureHashSEIEnabled = enabled; }

};

//! \}

#endif // !defined(AFX_TDECGOP_H__29440B7A_7CC0_48C7_8DD5_1A531D3CED45__INCLUDED_)

