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

/** \file     TDecTop.h
    \brief    decoder class (header)
*/

#ifndef __TDECTOP__
#define __TDECTOP__

#include "TLibCommon/CommonDef.h"
#include "TLibCommon/TComList.h"
#include "TLibCommon/TComPicYuv.h"
#include "TLibCommon/TComPic.h"
#include "TLibCommon/TComTrQuant.h"
#include "TLibCommon/SEI.h"

#include "TDecGop.h"
#include "TDecEntropy.h"
#include "TDecSbac.h"
#include "TDecCAVLC.h"
#include "SEIread.h"

struct InputNALUnit;

//! \ingroup TLibDecoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// decoder class
class TDecTop
{
private:
  Int                     m_iMaxRefPicNum;
  
  Int                     m_pocCRA;            ///< POC number of the latest CRA picture
  Bool                    m_prevRAPisBLA;      ///< true if the previous RAP (CRA/CRANT/BLA/BLANT/IDR) picture is a BLA/BLANT picture
  Int                     m_pocRandomAccess;   ///< POC number of the random access point (the first IDR or CRA picture)

  TComList<TComPic*>      m_cListPic;         //  Dynamic buffer
  ParameterSetManagerDecoder m_parameterSetManagerDecoder;  // storage for parameter sets 
  TComSlice*              m_apcSlicePilot;
  
  SEIMessages             m_SEIs; ///< List of SEI messages that have been received before the first slice and between slices

  // functional classes
  TComPrediction          m_cPrediction;
  TComTrQuant             m_cTrQuant;
  TDecGop                 m_cGopDecoder;
  TDecSlice               m_cSliceDecoder;
  TDecCu                  m_cCuDecoder;
  TDecEntropy             m_cEntropyDecoder;
  TDecCavlc               m_cCavlcDecoder;
  TDecSbac                m_cSbacDecoder;
  TDecBinCABAC            m_cBinCABAC;
  SEIReader               m_seiReader;
  TComLoopFilter          m_cLoopFilter;
  TComSampleAdaptiveOffset m_cSAO;

  Bool isSkipPictureForBLA(Int& iPOCLastDisplay);
  Bool isRandomAccessSkipPicture(Int& iSkipFrame,  Int& iPOCLastDisplay);
  TComPic*                m_pcPic;
  UInt                    m_uiSliceIdx;
  Int                     m_prevPOC;
  Bool                    m_bFirstSliceInPicture;
  Bool                    m_bFirstSliceInSequence;

public:
  TDecTop();
  virtual ~TDecTop();
  
  Void  create  ();
  Void  destroy ();

  void setDecodedPictureHashSEIEnabled(Int enabled) { m_cGopDecoder.setDecodedPictureHashSEIEnabled(enabled); }

  Void  init();
  Bool  decode(InputNALUnit& nalu, Int& iSkipFrame, Int& iPOCLastDisplay);
  
  Void  deletePicBuffer();

  Void executeLoopFilters(Int& poc, TComList<TComPic*>*& rpcListPic);

protected:
  Void  xGetNewPicBuffer  (TComSlice* pcSlice, TComPic*& rpcPic);
  Void  xCreateLostPicture (Int iLostPOC);

  Void      xActivateParameterSets();
  Bool      xDecodeSlice(InputNALUnit &nalu, Int &iSkipFrame, Int iPOCLastDisplay);
  Void      xDecodeVPS();
  Void      xDecodeSPS();
  Void      xDecodePPS();
  Void      xDecodeSEI( TComInputBitstream* bs, const NalUnitType nalUnitType );

};// END CLASS DEFINITION TDecTop


//! \}

#endif // __TDECTOP__

