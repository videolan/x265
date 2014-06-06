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

#include "common.h"
#include "TComBitStream.h"

using namespace x265;

#define MIN_FIFO_SIZE 1000

TComOutputBitstream::TComOutputBitstream()
{
    m_fifo = X265_MALLOC(uint8_t, MIN_FIFO_SIZE);
    m_byteAlloc = MIN_FIFO_SIZE;
    clear();
}

void TComOutputBitstream::push_back(uint8_t val)
{
    if (!m_fifo)
        return;

    if (m_byteOccupancy >= m_byteAlloc)
    {
        /** reallocate buffer with doubled size */
        uint8_t *temp = X265_MALLOC(uint8_t, m_byteAlloc * 2);
        if (temp)
        {
            ::memcpy(temp, m_fifo, m_byteOccupancy);
            X265_FREE(m_fifo);
            m_fifo = temp;
            m_byteAlloc *= 2;
        }
        else
        {
            x265_log(NULL, X265_LOG_ERROR, "Unable to realloc bitstream buffer");
            return;
        }
    }
    m_fifo[m_byteOccupancy++] = val;
}

void TComOutputBitstream::write(uint32_t val, uint32_t numBits)
{
    X265_CHECK(numBits <= 32, "numBits out of range\n");
    X265_CHECK(numBits == 32 || (val & (~0 << numBits)) == 0, "numBits & val out of range\n");

    uint32_t totalPartialBits = m_partialByteBits + numBits;
    uint32_t nextPartialBits = totalPartialBits & 7;
    uint8_t  nextHeldByte = val << (8 - nextPartialBits);
    uint32_t writeBytes = totalPartialBits >> 3;

    if (writeBytes)
    {
        /* topword aligns m_partialByte with the msb of val */
        uint32_t topword = (numBits - nextPartialBits) & ~7;
        uint32_t write_bits = (m_partialByte << topword) | (val >> nextPartialBits);

        switch (writeBytes)
        {
        case 4: push_back(write_bits >> 24);
        case 3: push_back(write_bits >> 16);
        case 2: push_back(write_bits >> 8);
        case 1: push_back(write_bits);
        }

        m_partialByte = nextHeldByte;
        m_partialByteBits = nextPartialBits;
    }
    else
    {
        m_partialByte |= nextHeldByte;
        m_partialByteBits = nextPartialBits;
    }
}

void TComOutputBitstream::writeByte(uint32_t val)
{
    // Only CABAC will call writeByte, the fifo must be byte aligned
    X265_CHECK(!m_partialByteBits, "expecting m_partialByteBits = 0\n");

    push_back(val);
}

void TComOutputBitstream::writeAlignOne()
{
    uint32_t numBits = (8 - m_partialByteBits) & 0x7;

    write((1 << numBits) - 1, numBits);
}

void TComOutputBitstream::writeAlignZero()
{
    if (m_partialByteBits)
    {
        push_back(m_partialByte);
        m_partialByte = 0;
        m_partialByteBits = 0;
    }
}

void TComOutputBitstream::writeByteAlignment()
{
    write(1, 1);
    writeAlignZero();
}

int TComOutputBitstream::countStartCodeEmulations()
{
    int numStartCodes = 0;

    for (uint32_t i = 0; i + 2 < m_byteOccupancy; i++)
    {
        if (!m_fifo[i] && !m_fifo[i + 1] && m_fifo[i + 2] <= 3)
        {
            numStartCodes++;
            i++;
        }
    }

    return numStartCodes;
}

void TComOutputBitstream::appendSubstream(TComOutputBitstream* substream)
{
    if (m_byteOccupancy + substream->m_byteOccupancy > m_byteAlloc)
    {
        /* reallocate buffer with doubled size */
        uint8_t *temp = X265_MALLOC(uint8_t, (m_byteOccupancy + substream->m_byteOccupancy) * 2);
        if (temp)
        {
            ::memcpy(temp, m_fifo, m_byteOccupancy);
            X265_FREE(m_fifo);
            m_fifo = temp;
            m_byteAlloc = (m_byteOccupancy + substream->m_byteOccupancy) * 2;
        }
        else
        {
            x265_log(NULL, X265_LOG_ERROR, "Unable to realloc bitstream buffer");
            return;
        }
    }
    if (m_partialByteBits)
    {
        for (uint32_t i = 0; i < substream->m_byteOccupancy; i++)
            write(substream->m_fifo[i], 8);
    }
    else
    {
        memcpy(m_fifo + m_byteOccupancy, substream->m_fifo, substream->m_byteOccupancy);
        m_byteOccupancy += substream->m_byteOccupancy;
    }
    if (substream->m_partialByteBits)
        write(substream->m_partialByte >> (8 - substream->m_partialByteBits), substream->m_partialByteBits);
}
