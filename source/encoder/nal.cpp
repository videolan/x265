/*****************************************************************************
* Copyright (C) 2013 x265 project
*
* Authors: Steve Borho <steve@borho.org>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
*
* This program is also available under a commercial proprietary license.
* For more information, contact us at license @ x265.com.
*****************************************************************************/

#include "common.h"
#include "TLibCommon/TComBitStream.h"
#include "nal.h"

namespace x265 {
// private namespace

void NALUnit::serialize(NalUnitType nalUnitType, const TComOutputBitstream& bs, uint8_t* extra, uint32_t extraBytes)
{
    uint32_t bitsSize = bs.getNumberOfWrittenBytes();
    const uint8_t* bpayload = bs.getFIFO();
    if (!bpayload)
        return;

    /* padded allocation for emulation prevention bytes */
    uint8_t* out = m_nalUnitData = X265_MALLOC(uint8_t, 2 + bitsSize + (bitsSize >> 1) + extraBytes);
    if (!out)
        return;

    /* 16bit NAL header:
     * forbidden_zero_bit       1-bit
     * nal_unit_type            6-bits
     * nuh_reserved_zero_6bits  6-bits
     * nuh_temporal_id_plus1    3-bits */
    out[0] = (uint8_t)nalUnitType << 1;
    out[1] = 1;
    uint32_t bytes = 2;

    /* 7.4.1 ...
     * Within the NAL unit, the following three-byte sequences shall not occur at
     * any byte-aligned position:
     *  - 0x000000
     *  - 0x000001
     *  - 0x000002
     */
    for (uint32_t i = 0; i < bitsSize; i++)
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

    if (extra)
    {
        memcpy(out + bytes, extra, extraBytes);
        bytes += extraBytes;
    }

    /* 7.4.1.1
     * ... when the last byte of the RBSP data is equal to 0x00 (which can
     * only occur when the RBSP ends in a cabac_zero_word), a final byte equal
     * to 0x03 is appended to the end of the data.
     */
    if (!out[bytes - 1])
        out[bytes++] = 0x03;

    X265_CHECK(bytes <= 2 + bitsSize + (bitsSize >> 1) + extraBytes, "NAL buffer overflow\n");

    m_nalUnitType = nalUnitType;
    m_packetSize = bytes;
}

/* concatenate and escape multiple sub-streams, return final escaped lengths and
 * concatenated buffer. Caller is responsible for freeing the returned buffer */
uint8_t *NALUnit::serializeMultiple(uint32_t* streamSizeBytes, uint32_t& totalBytes, uint32_t streamCount, const TComOutputBitstream* streams)
{
    uint32_t estSize = 0;
    for (uint32_t s = 0; s < streamCount; s++)
        estSize += streams[s].getNumberOfWrittenBytes();
    totalBytes = 0;

    /* padded allocation for emulation prevention bytes */
    uint8_t* out = X265_MALLOC(uint8_t, estSize + (estSize >> 1));
    if (!out)
        return NULL;

    for (uint32_t s = 0; s < streamCount; s++)
    {
        const TComOutputBitstream& stream = streams[s];
        uint32_t inSize = stream.getNumberOfWrittenBytes();
        const uint8_t *inBytes = stream.getFIFO();
        uint32_t prevBufSize = totalBytes;

        for (uint32_t i = 0; i < inSize; i++)
        {
            if (totalBytes > 2 && !out[totalBytes - 2] && !out[totalBytes - 3] && out[totalBytes - 1] <= 0x03)
            {
                /* inject 0x03 to prevent emulating a start code */
                out[totalBytes] = out[totalBytes - 1];
                out[totalBytes - 1] = 0x03;
                totalBytes++;
            }

            out[totalBytes++] = inBytes[i];
        }

        if (s < streamCount - 1)
            streamSizeBytes[s] = (totalBytes - prevBufSize) << 3;
    }

    return out;
}

}
