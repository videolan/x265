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

#pragma once

#include <ostream>

#include "TLibCommon/TypeDef.h"
#include "TLibCommon/TComBitStream.h"
#include "TLibCommon/NAL.h"

//! \ingroup TLibEncoder
//! \{

/**
 * A convenience wrapper to NALUnit that also provides a
 * bitstream object.
 */
struct OutputNALUnit : public NALUnit
{
  /**
   * construct an OutputNALunit structure with given header values and
   * storage for a bitstream.  Upon construction the NALunit header is
   * written to the bitstream.
   */
  OutputNALUnit(
    NalUnitType nalUnitType,
    UInt temporalID = 0,
    UInt reserved_zero_6bits = 0)
  : NALUnit(nalUnitType, temporalID, reserved_zero_6bits)
  , m_Bitstream()
  {}

  OutputNALUnit& operator=(const NALUnit& src)
  {
    m_Bitstream.clear();
    static_cast<NALUnit*>(this)->operator=(src);
    return *this;
  }

  TComOutputBitstream m_Bitstream;
};

void write(std::ostream& out, OutputNALUnit& nalu);
void writeRBSPTrailingBits(TComOutputBitstream& bs);

inline NALUnitEBSP::NALUnitEBSP(OutputNALUnit& nalu)
  : NALUnit(nalu)
{
  write(m_nalUnitData, nalu);
}

void copyNaluData(OutputNALUnit& naluDest, const OutputNALUnit& naluSrc);

//! \}
