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
#include "TLibCommon/AccessUnit.h"
#include "NALwrite.h"

//! \ingroup TLibEncoder
//! \{

/**
 * write all NALunits in au to bytestream out in a manner satisfying
 * AnnexB of AVC.  NALunits are written in the order they are found in au.
 * the zero_byte word is appended to:
 *  - the initial startcode in the access unit,
 *  - any SPS/PPS nal units
 */
static std::vector<UInt> writeAnnexB(std::ostream& out, const AccessUnit& au)
{
  std::vector<UInt> annexBsizes;

  for (AccessUnit::const_iterator it = au.begin(); it != au.end(); it++)
  {
    const NALUnitEBSP& nalu = **it;
    UInt size = 0; /* size of annexB unit in bytes */

    static const Char start_code_prefix[] = {0,0,0,1};
    if (it == au.begin() || nalu.m_nalUnitType == NAL_UNIT_SPS || nalu.m_nalUnitType == NAL_UNIT_PPS)
    {
      /* From AVC, When any of the following conditions are fulfilled, the
       * zero_byte syntax element shall be present:
       *  - the nal_unit_type within the nal_unit() is equal to 7 (sequence
       *    parameter set) or 8 (picture parameter set),
       *  - the byte stream NAL unit syntax structure contains the first NAL
       *    unit of an access unit in decoding order, as specified by subclause
       *    7.4.1.2.3.
       */
      out.write(start_code_prefix, 4);
      size += 4;
    }
    else
    {
      out.write(start_code_prefix+1, 3);
      size += 3;
    }
    out << nalu.m_nalUnitData.str();
    size += UInt(nalu.m_nalUnitData.str().size());

    annexBsizes.push_back(size);
  }

  return annexBsizes;
}
//! \}
