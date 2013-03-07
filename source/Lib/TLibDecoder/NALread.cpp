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

/**
 \file     NALread.cpp
 \brief    reading funtionality for NAL units
 */


#include <vector>
#include <algorithm>
#include <ostream>

#include "NALread.h"
#include "TLibCommon/NAL.h"
#include "TLibCommon/TComBitStream.h"

using namespace std;

//! \ingroup TLibDecoder
//! \{
static void convertPayloadToRBSP(vector<uint8_t>& nalUnitBuf, TComInputBitstream *bitstream, Bool isVclNalUnit)
{
  UInt zeroCount = 0;
  vector<uint8_t>::iterator it_read, it_write;

  UInt pos = 0;
  bitstream->clearEmulationPreventionByteLocation();
  for (it_read = it_write = nalUnitBuf.begin(); it_read != nalUnitBuf.end(); it_read++, it_write++, pos++)
  {
    assert(zeroCount < 2 || *it_read >= 0x03);
    if (zeroCount == 2 && *it_read == 0x03)
    {
      bitstream->pushEmulationPreventionByteLocation( pos );
      pos++;
      it_read++;
      zeroCount = 0;
      if (it_read == nalUnitBuf.end())
      {
        break;
      }
    }
    zeroCount = (*it_read == 0x00) ? zeroCount+1 : 0;
    *it_write = *it_read;
  }
  assert(zeroCount == 0);
  
  if (isVclNalUnit)
  {
    // Remove cabac_zero_word from payload if present
    Int n = 0;
    
    while (it_write[-1] == 0x00)
    {
      it_write--;
      n++;
    }
    
    if (n > 0)
    {
      printf("\nDetected %d instances of cabac_zero_word", n/2);      
    }
  }

  nalUnitBuf.resize(it_write - nalUnitBuf.begin());
}

Void readNalUnitHeader(InputNALUnit& nalu)
{
  TComInputBitstream& bs = *nalu.m_Bitstream;

  Bool forbidden_zero_bit = bs.read(1);           // forbidden_zero_bit
  assert(forbidden_zero_bit == 0);
  nalu.m_nalUnitType = (NalUnitType) bs.read(6);  // nal_unit_type
  nalu.m_reservedZero6Bits = bs.read(6);       // nuh_reserved_zero_6bits
  assert(nalu.m_reservedZero6Bits == 0);
  nalu.m_temporalId = bs.read(3) - 1;             // nuh_temporal_id_plus1

  if ( nalu.m_temporalId )
  {
    assert( nalu.m_nalUnitType != NAL_UNIT_CODED_SLICE_BLA
         && nalu.m_nalUnitType != NAL_UNIT_CODED_SLICE_BLANT
         && nalu.m_nalUnitType != NAL_UNIT_CODED_SLICE_BLA_N_LP
         && nalu.m_nalUnitType != NAL_UNIT_CODED_SLICE_IDR
         && nalu.m_nalUnitType != NAL_UNIT_CODED_SLICE_IDR_N_LP
         && nalu.m_nalUnitType != NAL_UNIT_CODED_SLICE_CRA
         && nalu.m_nalUnitType != NAL_UNIT_VPS
         && nalu.m_nalUnitType != NAL_UNIT_SPS
         && nalu.m_nalUnitType != NAL_UNIT_EOS
         && nalu.m_nalUnitType != NAL_UNIT_EOB );
  }
  else
  {
    assert( nalu.m_nalUnitType != NAL_UNIT_CODED_SLICE_TLA
         && nalu.m_nalUnitType != NAL_UNIT_CODED_SLICE_TSA_N
         && nalu.m_nalUnitType != NAL_UNIT_CODED_SLICE_STSA_R
         && nalu.m_nalUnitType != NAL_UNIT_CODED_SLICE_STSA_N );
  }
}
/**
 * create a NALunit structure with given header values and storage for
 * a bitstream
 */
void read(InputNALUnit& nalu, vector<uint8_t>& nalUnitBuf)
{
  /* perform anti-emulation prevention */
  TComInputBitstream *pcBitstream = new TComInputBitstream(NULL);
  convertPayloadToRBSP(nalUnitBuf, pcBitstream, (nalUnitBuf[0] & 64) == 0);
  
  nalu.m_Bitstream = new TComInputBitstream(&nalUnitBuf);
  nalu.m_Bitstream->setEmulationPreventionByteLocation(pcBitstream->getEmulationPreventionByteLocation());
  delete pcBitstream;
  readNalUnitHeader(nalu);
}
//! \}
