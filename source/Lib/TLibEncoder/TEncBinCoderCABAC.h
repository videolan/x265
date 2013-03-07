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

/** \file     TEncBinCoderCABAC.h
    \brief    binary entropy encoder of CABAC
*/

#ifndef __TENC_BIN_CODER_CABAC__
#define __TENC_BIN_CODER_CABAC__

#include "TLibCommon/TComCABACTables.h"
#include "TEncBinCoder.h"

//! \ingroup TLibEncoder
//! \{

class TEncBinCABAC : public TEncBinIf
{
public:
  TEncBinCABAC ();
  virtual ~TEncBinCABAC();
  
  Void  init              ( TComBitIf* pcTComBitIf );
  Void  uninit            ();
  
  Void  start             ();
  Void  finish            ();
  Void  copyState         ( TEncBinIf* pcTEncBinIf );
  Void  flush            ();

  Void  resetBac          ();
  Void  encodePCMAlignBits();
  Void  xWritePCMCode     ( UInt uiCode, UInt uiLength );
  
  Void  resetBits         ();
  UInt  getNumWrittenBits ();
  
  Void  encodeBin         ( UInt  binValue,  ContextModel& rcCtxModel );
  Void  encodeBinEP       ( UInt  binValue                            );
  Void  encodeBinsEP      ( UInt  binValues, Int numBins              );
  Void  encodeBinTrm      ( UInt  binValue                            );
  
  TEncBinCABAC* getTEncBinCABAC()  { return this; }
  
  Void  setBinsCoded              ( UInt uiVal )  { m_uiBinsCoded = uiVal;               }
  UInt  getBinsCoded              ()              { return m_uiBinsCoded;                }
  Void  setBinCountingEnableFlag  ( Bool bFlag )  { m_binCountIncrement = bFlag ? 1 : 0; }
  Bool  getBinCountingEnableFlag  ()              { return m_binCountIncrement != 0;     }
  
#if FAST_BIT_EST
protected:
#else
private:
#endif
  Void testAndWriteOut();
  Void writeOut();
  
  TComBitIf*          m_pcTComBitIf;
  UInt                m_uiLow;
  UInt                m_uiRange;
  UInt                m_bufferedByte;
  Int                 m_numBufferedBytes;
  Int                 m_bitsLeft;
  UInt                m_uiBinsCoded;
  Int                 m_binCountIncrement;
#if FAST_BIT_EST
  UInt64 m_fracBits;
#endif
};

//! \}

#endif

