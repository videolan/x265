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

/** \file     SyntaxElementWriter.h
    \brief    CAVLC encoder class (header)
*/

#ifndef X265_SYNTAXELEMENTWRITER_H
#define X265_SYNTAXELEMENTWRITER_H

#include "TLibCommon/CommonDef.h"
#include "TLibCommon/TComBitStream.h"
#include "TLibCommon/TComRom.h"

//! \ingroup TLibEncoder
//! \{

#if ENC_DEC_TRACE

#define WRITE_CODE(value, length, name)    xWriteCodeTr(value, length, name)
#define WRITE_UVLC(value,         name)    xWriteUvlcTr(value,         name)
#define WRITE_SVLC(value,         name)    xWriteSvlcTr(value,         name)
#define WRITE_FLAG(value,         name)    xWriteFlagTr(value,         name)

#else

#define WRITE_CODE(value, length, name)     xWriteCode(value, length)
#define WRITE_UVLC(value,         name)     xWriteUvlc(value)
#define WRITE_SVLC(value,         name)     xWriteSvlc(value)
#define WRITE_FLAG(value,         name)     xWriteFlag(value)

#endif // if ENC_DEC_TRACE

namespace x265 {
// private namespace

class SyntaxElementWriter
{
protected:

    TComBitIf* m_bitIf;

    SyntaxElementWriter()
        : m_bitIf(NULL)
    {}

    virtual ~SyntaxElementWriter() {}

    void  setBitstream(TComBitIf* p)  { m_bitIf = p;  }

    void  xWriteCode(UInt code, UInt len);
    void  xWriteUvlc(UInt code);
    void  xWriteSvlc(int code);
    void  xWriteFlag(UInt code);
#if ENC_DEC_TRACE
    void  xWriteCodeTr(UInt value, UInt  length, const char *symbolName);
    void  xWriteUvlcTr(UInt value,               const char *symbolName);
    void  xWriteSvlcTr(int value,                const char *symbolName);
    void  xWriteFlagTr(UInt value,               const char *symbolName);
#endif

    UInt  xConvertToUInt(int val) { return (val <= 0) ? -val << 1 : (val << 1) - 1; }
};
}
//! \}

#endif // ifndef X265_SYNTAXELEMENTWRITER_H
