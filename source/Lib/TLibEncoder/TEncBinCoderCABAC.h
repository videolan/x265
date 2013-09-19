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

#ifndef X265_TENCBINCODERCABAC_H
#define X265_TENCBINCODERCABAC_H

#include "TEncBinCoder.h"

//! \ingroup TLibEncoder
//! \{

namespace x265 {
// private namespace

class TEncBinCABAC : public TEncBinIf
{
public:

    TEncBinCABAC();
    virtual ~TEncBinCABAC();

    void  init(TComBitIf* pcTComBitIf);
    void  uninit();

    void  start();
    void  finish();
    void  copyState(TEncBinIf* pcTEncBinIf);
    void  flush();

    void  resetBac();
    void  encodePCMAlignBits();
    void  xWritePCMCode(UInt uiCode, UInt uiLength);

    void  resetBits();
    UInt  getNumWrittenBits();

    void  encodeBin(UInt binValue, ContextModel& rcCtxModel);
    void  encodeBinEP(UInt binValue);
    void  encodeBinsEP(UInt binValues, int numBins);
    void  encodeBinTrm(UInt binValue);

    TEncBinCABAC* getTEncBinCABAC() { return this; }

    void  setBinsCoded(UInt val) { m_uiBinsCoded = val; }

    UInt  getBinsCoded() { return m_uiBinsCoded; }

    void  setBinCountingEnableFlag(bool bFlag) { m_binCountIncrement = bFlag ? 1 : 0; }

    bool  getBinCountingEnableFlag() { return m_binCountIncrement != 0; }

protected:

    void testAndWriteOut();
    void writeOut();

public:
    TComBitIf*          m_pcTComBitIf;
    UInt                m_uiLow;
    UInt                m_uiRange;
    UInt                m_bufferedByte;
    int                 m_numBufferedBytes;
    int                 m_bitsLeft;
    UInt                m_uiBinsCoded;
    int                 m_binCountIncrement;
    UInt64              m_fracBits;
};
}
//! \}

#endif // ifndef X265_TENCBINCODERCABAC_H
