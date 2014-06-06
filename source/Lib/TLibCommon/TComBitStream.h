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

/** \file     TComBitStream.h
    \brief    class for handling bitstream (header)
*/

#ifndef X265_COMBITSTREAM_H
#define X265_COMBITSTREAM_H

#include "common.h"

namespace x265 {
// private namespace

class TComBitIf
{
public:

    virtual void     writeAlignOne() {}
    virtual void     writeAlignZero() {}
    virtual void     write(uint32_t val, uint32_t numBits)  = 0;
    virtual void     writeByte(uint32_t val)                = 0;
    virtual void     resetBits()                            = 0;
    virtual uint32_t getNumberOfWrittenBits() const         = 0;
    virtual ~TComBitIf() {}
};

class TComBitCounter : public TComBitIf
{
protected:

    uint32_t  m_bitCounter;

public:

    void     write(uint32_t, uint32_t num)  { m_bitCounter += num; }
    void     writeByte(uint32_t)            { m_bitCounter += 8;   }
    void     resetBits()                    { m_bitCounter = 0;    }
    uint32_t getNumberOfWrittenBits() const { return m_bitCounter; }
};


class TComOutputBitstream : public TComBitIf
{
public:

    TComOutputBitstream();
    ~TComOutputBitstream()                   { X265_FREE(m_fifo); }

    void     clear()                         { m_partialByteBits = m_byteOccupancy = 0; m_partialByte = 0; }
    uint32_t getNumberOfWrittenBytes() const { return m_byteOccupancy; }
    uint32_t getNumberOfWrittenBits()  const { return m_byteOccupancy * 8 + m_partialByteBits; }
    const uint8_t* getFIFO() const           { return m_fifo; }

    void     write(uint32_t val, uint32_t numBits);
    void     writeByte(uint32_t val);

    void     writeAlignOne();      // insert one bits until the bitstream is byte-aligned
    void     writeAlignZero();     // insert zero bits until the bitstream is byte-aligned
    void     writeByteAlignment(); // insert 1 bit, then pad to byte-align with zero

    void     appendSubstream(TComOutputBitstream* substream);
    int      countStartCodeEmulations();

private:

    uint8_t *m_fifo;
    uint32_t m_byteAlloc;
    uint32_t m_byteOccupancy;
    uint32_t m_partialByteBits;
    uint8_t  m_partialByte;

    void     resetBits() { X265_CHECK(0, "resetBits called on base class\n"); }
    void     push_back(uint8_t val);
};
}

#endif // ifndef X265_COMBITSTREAM_H
