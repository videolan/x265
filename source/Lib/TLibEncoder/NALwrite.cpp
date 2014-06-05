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
#include "TLibCommon/NAL.h"
#include "TLibCommon/TComBitStream.h"
#include "NALwrite.h"

namespace x265 {

/**
 * write nalu to bytestream out, performing RBSP anti startcode
 * emulation as required.  nalu.m_RBSPayload must be byte aligned.
 * caller is responsible for freeing returned allocated pointer
 */
uint8_t *write(const OutputNALUnit& nalu, uint32_t &bytes)
{
    TComOutputBitstream header;
    header.write(0, 1);
    header.write(nalu.m_nalUnitType, 6);
    header.write(nalu.m_reservedZero6Bits, 6);
    header.write(nalu.m_temporalId + 1, 3);

    uint32_t headerSize = header.getByteStreamLength();
    char*    hpayload = header.getByteStream();

    uint32_t bitsSize = nalu.m_bitstream.getByteStreamLength();
    uint8_t* bpayload = nalu.m_bitstream.getFIFO();
    if (!bpayload || !hpayload)
        return NULL;

    /* padded allocation for emulation prevention bytes */
    uint8_t* out = X265_MALLOC(uint8_t, headerSize + bitsSize + (bitsSize + 2) / 3);
    if (!out)
        return NULL;

    memcpy(out, hpayload, headerSize);
    bytes = headerSize;

    /* 7.4.1 ...
     * Within the NAL unit, the following three-byte sequences shall not occur at
     * any byte-aligned position:
     *  - 0x000000
     *  - 0x000001
     *  - 0x000002
     */
    for (int i = 0; i < bitsSize; i++)
    {
        if (i > 2 && !out[bytes - 2] && !out[bytes - 3] && out[bytes - 1] <= 0x03)
        {
            /* inject 0x03 to prevent emulating a start code */
            out[bytes] = out[bytes - 1];
            out[bytes - 1] = 0x03;
            bytes++;
        }

        out[bytes++] = bpayload[i];
    }

    /* 7.4.1.1
     * ... when the last byte of the RBSP data is equal to 0x00 (which can
     * only occur when the RBSP ends in a cabac_zero_word), a final byte equal
     * to 0x03 is appended to the end of the data.
     */
    if (!out[bytes - 1])
        out[bytes++] = 0x03;

    X265_CHECK(bytes <= headerSize + bitsSize + (bitsSize + 2) / 3, "NAL buffer overflow\n");

    return out;
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
