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

void NALUnit::serialize(NalUnitType nalUnitType, const TComOutputBitstream& bs)
{
    TComOutputBitstream header;
    header.write(0, 1);
    header.write(nalUnitType, 6);
    header.write(0, 6);
    header.write(0 + 1, 3);

    uint32_t headerSize = header.getByteStreamLength();
    uint8_t* hpayload = header.getFIFO();

    uint32_t bitsSize = bs.getByteStreamLength();
    uint8_t* bpayload = bs.getFIFO();
    if (!bpayload || !hpayload)
        return;

    /* padded allocation for emulation prevention bytes */
    uint8_t* out = m_nalUnitData = X265_MALLOC(uint8_t, headerSize + bitsSize + (bitsSize + 2) / 3);
    if (!out)
        return;

    memcpy(out, hpayload, headerSize);
    uint32_t bytes = headerSize;

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

    /* 7.4.1.1
     * ... when the last byte of the RBSP data is equal to 0x00 (which can
     * only occur when the RBSP ends in a cabac_zero_word), a final byte equal
     * to 0x03 is appended to the end of the data.
     */
    if (!out[bytes - 1])
        out[bytes++] = 0x03;

    X265_CHECK(bytes <= headerSize + bitsSize + (bitsSize + 2) / 3, "NAL buffer overflow\n");

    m_nalUnitType = nalUnitType;
    m_packetSize = bytes;
}

}
