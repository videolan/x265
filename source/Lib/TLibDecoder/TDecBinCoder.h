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

/** \file     TDecBinCoder.h
    \brief    binary entropy decoder interface
*/

#ifndef __TDEC_BIN_CODER__
#define __TDEC_BIN_CODER__

#include "TLibCommon/ContextModel.h"
#include "TLibCommon/TComBitStream.h"

//! \ingroup TLibDecoder
//! \{
class TDecBinCABAC;

class TDecBinIf
{
public:
  virtual Void  init              ( TComInputBitstream* pcTComBitstream )     = 0;
  virtual Void  uninit            ()                                          = 0;

  virtual Void  start             ()                                          = 0;
  virtual Void  finish            ()                                          = 0;
  virtual Void  flush            ()                                           = 0;

  virtual Void  decodeBin         ( UInt& ruiBin, ContextModel& rcCtxModel )  = 0;
  virtual Void  decodeBinEP       ( UInt& ruiBin                           )  = 0;
  virtual Void  decodeBinsEP      ( UInt& ruiBins, Int numBins             )  = 0;
  virtual Void  decodeBinTrm      ( UInt& ruiBin                           )  = 0;
  
  virtual Void  resetBac          ()                                          = 0;
  virtual Void  decodePCMAlignBits()                                          = 0;
  virtual Void  xReadPCMCode      ( UInt uiLength, UInt& ruiCode)              = 0;

  virtual ~TDecBinIf() {}

  virtual Void  copyState         ( TDecBinIf* pcTDecBinIf )                  = 0;
  virtual TDecBinCABAC*   getTDecBinCABAC   ()  { return 0; }
};

//! \}

#endif
