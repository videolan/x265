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

/** \file     TEncBinCoder.h
    \brief    binary entropy encoder interface
*/

#ifndef __TENC_BIN_CODER__
#define __TENC_BIN_CODER__

#include "TLibCommon/ContextModel.h"
#include "TLibCommon/TComBitStream.h"

//! \ingroup TLibEncoder
//! \{

namespace x265 {
// private namespace

class TEncBinCABAC;

class TEncBinIf
{
public:

    virtual void  init(TComBitIf* bitIf) = 0;
    virtual void  uninit() = 0;

    virtual void  start() = 0;
    virtual void  finish() = 0;
    virtual void  copyState(TEncBinIf* binIf) = 0;
    virtual void  flush() = 0;

    virtual void  resetBac() = 0;
    virtual void  encodePCMAlignBits() = 0;
    virtual void  xWritePCMCode(UInt code, UInt length) = 0;

    virtual void  resetBits() = 0;
    virtual UInt  getNumWrittenBits() = 0;

    virtual void  encodeBin(UInt bin,  ContextModel& ctx) = 0;
    virtual void  encodeBinEP(UInt bin) = 0;
    virtual void  encodeBinsEP(UInt bins, int numBins) = 0;
    virtual void  encodeBinTrm(UInt bin) = 0;

    virtual TEncBinCABAC*   getTEncBinCABAC()  { return 0; }

    virtual ~TEncBinIf() {}
};
}
//! \}

#endif // ifndef __TENC_BIN_CODER__
