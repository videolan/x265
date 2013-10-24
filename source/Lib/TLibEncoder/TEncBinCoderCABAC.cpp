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

/** \file     TEncBinCoderCABAC.cpp
    \brief    binary entropy encoder of CABAC
*/

#include "TEncBinCoderCABAC.h"
#include "TLibCommon/TComRom.h"

using namespace x265;

//! \ingroup TLibEncoder
//! \{

TEncBinCABAC::TEncBinCABAC(bool isCounter)
    : m_bitIf(0)
    , m_fracBits(0)
    , bIsCounter(isCounter)
{}

TEncBinCABAC::~TEncBinCABAC()
{}

void TEncBinCABAC::init(TComBitIf* bitIf)
{
    m_bitIf = bitIf;
}

void TEncBinCABAC::uninit()
{
    m_bitIf = 0;
}

void TEncBinCABAC::start()
{
    m_low              = 0;
    m_range            = 510;
    m_bitsLeft         = -12;
    m_numBufferedBytes = 0;
    m_bufferedByte     = 0xff;
}

void TEncBinCABAC::finish()
{
    if (bIsCounter)
    {
        // TODO: why write 0 bits?
        m_bitIf->write(0, UInt(m_fracBits >> 15));
        m_fracBits &= 32767;
        assert(0);
    }

    if (m_low >> (21 + m_bitsLeft))
    {
        //assert( m_numBufferedBytes > 0 );
        //assert( m_bufferedByte != 0xff );
        m_bitIf->writeByte(m_bufferedByte + 1);
        while (m_numBufferedBytes > 1)
        {
            m_bitIf->writeByte(0x00);
            m_numBufferedBytes--;
        }

        m_low -= 1 << (21 + m_bitsLeft);
    }
    else
    {
        if (m_numBufferedBytes > 0)
        {
            m_bitIf->writeByte(m_bufferedByte);
        }
        while (m_numBufferedBytes > 1)
        {
            m_bitIf->writeByte(0xff);
            m_numBufferedBytes--;
        }
    }
    m_bitIf->write(m_low >> 8, 13 + m_bitsLeft);
}

void TEncBinCABAC::flush()
{
    encodeBinTrm(1);
    finish();
    m_bitIf->write(1, 1);
    m_bitIf->writeAlignZero();

    start();
}

/** Reset BAC register and counter values.
 * \returns void
 */
void TEncBinCABAC::resetBac()
{
    start();
}

/** Encode PCM alignment zero bits.
 * \returns void
 */
void TEncBinCABAC::encodePCMAlignBits()
{
    finish();
    m_bitIf->write(1, 1);
    m_bitIf->writeAlignZero(); // pcm align zero
}

/** Write a PCM code.
 * \param code code value
 * \param length code bit-depth
 * \returns void
 */
void TEncBinCABAC::xWritePCMCode(UInt code, UInt length)
{
    m_bitIf->write(code, length);
}

void TEncBinCABAC::copyState(TEncBinCABAC* binCABAC)
{
    m_low              = binCABAC->m_low;
    m_range            = binCABAC->m_range;
    m_bitsLeft         = binCABAC->m_bitsLeft;
    m_bufferedByte     = binCABAC->m_bufferedByte;
    m_numBufferedBytes = binCABAC->m_numBufferedBytes;
    m_fracBits         = binCABAC->m_fracBits;
}

void TEncBinCABAC::resetBits()
{
    m_low              = 0;
    m_bitsLeft         = -12;
    m_numBufferedBytes = 0;
    m_bufferedByte     = 0xff;
    m_fracBits        &= 32767;
}

/**
 * \brief Encode bin
 *
 * \param binValue   bin value
 * \param rcCtxModel context model
 */
void TEncBinCABAC::encodeBin(UInt binValue, ContextModel &ctxModel)
{
    {
        DTRACE_CABAC_VL(g_nSymbolCounter++)
        DTRACE_CABAC_T("\tstate=")
        DTRACE_CABAC_V((ctxModel.getState() << 1) + ctxModel.getMps())
        DTRACE_CABAC_T("\tsymbol=")
        DTRACE_CABAC_V(binValue)
        DTRACE_CABAC_T("\n")
    }

    UInt mstate = ctxModel.m_state;
    ctxModel.m_state = sbacNext(ctxModel.m_state, binValue);

    if (bIsCounter)
    {
        m_fracBits += sbacGetEntropyBits(mstate, binValue);
        return;
    }
    ctxModel.bBinsCoded = 1;

    UInt mps = sbacGetMps(mstate);
    UInt state = sbacGetState(mstate);
    UInt lps = g_lpsTable[state][(m_range >> 6) & 3];
    m_range -= lps;

    int numBits = g_renormTable[lps >> 3];
    if (binValue != mps)
    {
        m_low     = (m_low + m_range) << numBits;
        m_range   = lps << numBits;
    }
    else
    {
        if (m_range >= 256)
        {
            return;
        }
        numBits = 1;
        m_low <<= 1;
        m_range <<= 1;
    }
    m_bitsLeft += numBits;

    testAndWriteOut();
}

/**
 * \brief Encode equiprobable bin
 *
 * \param binValue bin value
 */
void TEncBinCABAC::encodeBinEP(UInt binValue)
{
    {
        DTRACE_CABAC_VL(g_nSymbolCounter++)
        DTRACE_CABAC_T("\tEPsymbol=")
        DTRACE_CABAC_V(binValue)
        DTRACE_CABAC_T("\n")
    }
    if (bIsCounter)
    {
        m_fracBits += 32768;
        return;
    }
    m_low <<= 1;
    if (binValue)
    {
        m_low += m_range;
    }
    m_bitsLeft++;

    testAndWriteOut();
}

/**
 * \brief Encode equiprobable bins
 *
 * \param binValues bin values
 * \param numBins number of bins
 */
void TEncBinCABAC::encodeBinsEP(UInt binValues, int numBins)
{
    if (bIsCounter)
    {
        m_fracBits += 32768 * numBins;
        return;
    }

    for (int i = 0; i < numBins; i++)
    {
        DTRACE_CABAC_VL(g_nSymbolCounter++)
        DTRACE_CABAC_T("\tEPsymbol=")
        DTRACE_CABAC_V((binValues >> (numBins - 1 - i)) & 1)
        DTRACE_CABAC_T("\n")
    }

    while (numBins > 8)
    {
        numBins -= 8;
        UInt pattern = binValues >> numBins;
        m_low <<= 8;
        m_low += m_range * pattern;
        binValues -= pattern << numBins;
        m_bitsLeft += 8;

        testAndWriteOut();
    }

    m_low <<= numBins;
    m_low += m_range * binValues;
    m_bitsLeft += numBins;

    testAndWriteOut();
}

/**
 * \brief Encode terminating bin
 *
 * \param binValue bin value
 */
void TEncBinCABAC::encodeBinTrm(UInt binValue)
{
    if (bIsCounter)
    {
        m_fracBits += sbacGetEntropyBitsTrm(binValue);
        return;
    }

    m_range -= 2;
    if (binValue)
    {
        m_low  += m_range;
        m_low <<= 7;
        m_range = 2 << 7;
        m_bitsLeft += 7;
    }
    else if (m_range >= 256)
    {
        return;
    }
    else
    {
        m_low   <<= 1;
        m_range <<= 1;
        m_bitsLeft++;
    }

    testAndWriteOut();
}

void TEncBinCABAC::testAndWriteOut()
{
    if (m_bitsLeft >= 0)
    {
        writeOut();
    }
}

/**
 * \brief Move bits from register into bitstream
 */
void TEncBinCABAC::writeOut()
{
    UInt leadByte = m_low >> (13 + m_bitsLeft);

    m_bitsLeft -= 8;
    m_low &= 0xffffffffu >> (11 - m_bitsLeft);

    if (leadByte == 0xff)
    {
        m_numBufferedBytes++;
    }
    else
    {
        if (m_numBufferedBytes > 0)
        {
            UInt carry = leadByte >> 8;
            UInt byte = m_bufferedByte + carry;
            m_bufferedByte = leadByte & 0xff;
            m_bitIf->writeByte(byte);

            byte = (0xff + carry) & 0xff;
            while (m_numBufferedBytes > 1)
            {
                m_bitIf->writeByte(byte);
                m_numBufferedBytes--;
            }
        }
        else
        {
            m_numBufferedBytes = 1;
            m_bufferedByte = leadByte;
        }
    }
}

//! \}
