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

#include <vector>
#include <sstream>
#include "CommonDef.h"

class TComOutputBitstream;

/**
 * Represents a single NALunit header and the associated RBSPayload
 */
struct NALUnit
{
  NalUnitType m_nalUnitType; ///< nal_unit_type
  UInt        m_temporalId;  ///< temporal_id
  UInt        m_reservedZero6Bits; ///< reserved_zero_6bits

  /** construct an NALunit structure with given header values. */
  NALUnit(
    NalUnitType nalUnitType,
    Int         temporalId = 0,
    Int         reservedZero6Bits = 0)
    :m_nalUnitType (nalUnitType)
    ,m_temporalId  (temporalId)
    ,m_reservedZero6Bits(reservedZero6Bits)
  {}

  /** default constructor - no initialization; must be perfomed by user */
  NALUnit() {}

  /** returns true if the NALunit is a slice NALunit */
  Bool isSlice()
  {
    return m_nalUnitType == NAL_UNIT_CODED_SLICE_TRAIL_R
        || m_nalUnitType == NAL_UNIT_CODED_SLICE_TRAIL_N
        || m_nalUnitType == NAL_UNIT_CODED_SLICE_TLA
        || m_nalUnitType == NAL_UNIT_CODED_SLICE_TSA_N
        || m_nalUnitType == NAL_UNIT_CODED_SLICE_STSA_R
        || m_nalUnitType == NAL_UNIT_CODED_SLICE_STSA_N
        || m_nalUnitType == NAL_UNIT_CODED_SLICE_BLA
        || m_nalUnitType == NAL_UNIT_CODED_SLICE_BLANT
        || m_nalUnitType == NAL_UNIT_CODED_SLICE_BLA_N_LP
        || m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR
        || m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_N_LP
        || m_nalUnitType == NAL_UNIT_CODED_SLICE_CRA
        || m_nalUnitType == NAL_UNIT_CODED_SLICE_RADL_N
        || m_nalUnitType == NAL_UNIT_CODED_SLICE_DLP
        || m_nalUnitType == NAL_UNIT_CODED_SLICE_RASL_N
        || m_nalUnitType == NAL_UNIT_CODED_SLICE_TFD;
  }
#if L0045_NON_NESTED_SEI_RESTRICTIONS
  Bool isSei()
  {
    return m_nalUnitType == NAL_UNIT_SEI 
        || m_nalUnitType == NAL_UNIT_SEI_SUFFIX;
  }

  Bool isVcl()
  {
    return ( (UInt)m_nalUnitType < 32 );
  }
#endif
};

struct OutputNALUnit;

/**
 * A single NALunit, with complete payload in EBSP format.
 */
struct NALUnitEBSP : public NALUnit
{
  std::ostringstream m_nalUnitData;

  /**
   * convert the OutputNALUnit #nalu# into EBSP format by writing out
   * the NALUnit header, then the rbsp_bytes including any
   * emulation_prevention_three_byte symbols.
   */
  NALUnitEBSP(OutputNALUnit& nalu);
};
//! \}
//! \}
