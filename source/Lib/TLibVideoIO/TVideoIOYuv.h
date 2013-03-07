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

/** \file     TVideoIOYuv.h
    \brief    YUV file I/O class (header)
*/

#ifndef __TVIDEOIOYUV__
#define __TVIDEOIOYUV__

#include <stdio.h>
#include <fstream>
#include <iostream>
#include "TLibCommon/CommonDef.h"
#include "TLibCommon/TComPicYuv.h"

using namespace std;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// YUV file I/O class
class TVideoIOYuv
{
private:
  fstream   m_cHandle;                                      ///< file handle
  Int m_fileBitDepthY; ///< bitdepth of input/output video file luma component
  Int m_fileBitDepthC; ///< bitdepth of input/output video file chroma component
  Int m_bitDepthShiftY;  ///< number of bits to increase or decrease luma by before/after write/read
  Int m_bitDepthShiftC;  ///< number of bits to increase or decrease chroma by before/after write/read
  
public:
  TVideoIOYuv()           {}
  virtual ~TVideoIOYuv()  {}
  
  Void  open  ( Char* pchFile, Bool bWriteMode, Int fileBitDepthY, Int fileBitDepthC, Int internalBitDepthY, Int internalBitDepthC ); ///< open or create file
  Void  close ();                                           ///< close file

  void skipFrames(UInt numFrames, UInt width, UInt height);
  
  Bool  read  ( TComPicYuv*   pPicYuv, Int aiPad[2] );     ///< read  one YUV frame with padding parameter
  Bool  write( TComPicYuv*    pPicYuv, Int confLeft=0, Int confRight=0, Int confTop=0, Int confBottom=0 );
  
  Bool  isEof ();                                           ///< check for end-of-file
  Bool  isFail();                                           ///< check for failure
  
};

#endif // __TVIDEOIOYUV__

