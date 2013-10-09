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

#include "TLibCommon/NAL.h"
#include "TLibCommon/TComBitStream.h"
#include "NALwrite.h"
#include "common.h"

#include <algorithm>
#include <cstring>

using namespace std;

namespace x265 {

//! \ingroup TLibEncoder
//! \{

static const char emulation_prevention_three_byte[] = { 3 };

/**
 * write nalu to bytestream out, performing RBSP anti startcode
 * emulation as required.  nalu.m_RBSPayload must be byte aligned.
 */
void write(uint8_t*& out, OutputNALUnit& nalu, UInt &packetSize)
{
    packetSize = 0;
    TComOutputBitstream bsNALUHeader;
    bsNALUHeader.write(0, 1);                 // forbidden_zero_bit
    bsNALUHeader.write(nalu.m_nalUnitType, 6); // nal_unit_type
    bsNALUHeader.write(nalu.m_reservedZero6Bits, 6);                 // nuh_reserved_zero_6bits
    bsNALUHeader.write(nalu.m_temporalId + 1, 3); // nuh_temporal_id_plus1
    
    packetSize += bsNALUHeader.getByteStreamLength();
    out = (uint8_t *) malloc(packetSize);
    ::memcpy(out, bsNALUHeader.getByteStream(), packetSize);
    
    /* write out rsbp_byte's, inserting any required
     * emulation_prevention_three_byte's */

    /* 7.4.1 ...
     * emulation_prevention_three_byte is a byte equal to 0x03. When an
     * emulation_prevention_three_byte is present in the NAL unit, it shall be
     * discarded by the decoding process.
     * The last byte of the NAL unit shall not be equal to 0x00.
     * Within the NAL unit, the following three-byte sequences shall not occur at
     * any byte-aligned position:
     *  - 0x000000
     *  - 0x000001
     *  - 0x000002
     * Within the NAL unit, any four-byte sequence that starts with 0x000003
     * other than the following sequences shall not occur at any byte-aligned
     * position:
     *  - 0x00000300
     *  - 0x00000301
     *  - 0x00000302
     *  - 0x00000303
     */
    UInt fsize = nalu.m_Bitstream.getByteStreamLength();
    uint8_t* fifo = nalu.m_Bitstream.getFIFO();
    uint8_t* emulation = (uint8_t *)X265_MALLOC(uint8_t, fsize + EMULATION_SIZE);
    UInt nalsize = 0;
    
    if (emulation)
    {
        for (int count = 0; count < fsize; count++)
        {
            uint8_t val = fifo[count];
            if (count > 3 && (emulation[nalsize - 1] == 0x00 || emulation[nalsize - 1] == 0x01|| emulation[nalsize - 1] == 0x02 || emulation[nalsize - 1] == 0x03)
                && emulation[nalsize - 2] == 0x00 && emulation[nalsize - 3] == 0x00)
            {
                uint8_t tmp = emulation[nalsize - 1];
                emulation[nalsize - 1] = emulation_prevention_three_byte[0];
                emulation[nalsize] = tmp;
                emulation[nalsize + 1] = val;
                nalsize++;
            }
            else
            {
                emulation[nalsize] = val;
            }
            nalsize++;
        }
        UInt i = packetSize;
        out = (uint8_t *) realloc (out, nalsize + 4 );
        memcpy(out + packetSize, emulation, nalsize);
        packetSize += nalsize;
        
        /* 7.4.1.1
        * ... when the last byte of the RBSP data is equal to 0x00 (which can
        * only occur when the RBSP ends in a cabac_zero_word), a final byte equal
        * to 0x03 is appended to the end of the data.
        */
        if (out[packetSize - 1] == 0x00)
        {
            out[i] = 3;
            packetSize += 1;
        }
        X265_FREE(emulation);
    }
}

/**
 * Write rbsp_trailing_bits to bs causing it to become byte-aligned
 */
void writeRBSPTrailingBits(TComOutputBitstream& bs)
{
    bs.write(1, 1);
    bs.writeAlignZero();
}

}
//! \}
