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

#include "TEncBinCoderCABACCounter.h"
#include "TLibCommon/TComRom.h"

#if FAST_BIT_EST

//! \ingroup TLibEncoder
//! \{


TEncBinCABACCounter::TEncBinCABACCounter()
{
}

TEncBinCABACCounter::~TEncBinCABACCounter()
{
}

Void TEncBinCABACCounter::finish()
{
  m_pcTComBitIf->write(0, UInt(m_fracBits >> 15) );
  m_fracBits &= 32767;
}

UInt TEncBinCABACCounter::getNumWrittenBits()
{
  return m_pcTComBitIf->getNumberOfWrittenBits() + UInt( m_fracBits >> 15 );
}

/**
 * \brief Encode bin
 *
 * \param binValue   bin value
 * \param rcCtxModel context model
 */
Void TEncBinCABACCounter::encodeBin( UInt binValue, ContextModel &rcCtxModel )
{
  m_uiBinsCoded += m_binCountIncrement;
  
  m_fracBits += rcCtxModel.getEntropyBits( binValue );
  rcCtxModel.update( binValue );
}

/**
 * \brief Encode equiprobable bin
 *
 * \param binValue bin value
 */
Void TEncBinCABACCounter::encodeBinEP( UInt binValue )
{
  m_uiBinsCoded += m_binCountIncrement;
  m_fracBits += 32768;
}

/**
 * \brief Encode equiprobable bins
 *
 * \param binValues bin values
 * \param numBins number of bins
 */
Void TEncBinCABACCounter::encodeBinsEP( UInt binValues, Int numBins )
{
  m_uiBinsCoded += numBins & -m_binCountIncrement;
  m_fracBits += 32768 * numBins;
}

/**
 * \brief Encode terminating bin
 *
 * \param binValue bin value
 */
Void TEncBinCABACCounter::encodeBinTrm( UInt binValue )
{
  m_uiBinsCoded += m_binCountIncrement;
  m_fracBits += ContextModel::getEntropyBitsTrm( binValue );
}

//! \}
#endif

