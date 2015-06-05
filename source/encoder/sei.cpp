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
#include "bitstream.h"
#include "slice.h"
#include "sei.h"

using namespace X265_NS;

/* x265's identifying GUID */
const uint8_t SEIuserDataUnregistered::m_uuid_iso_iec_11578[16] = {
    0x2C, 0xA2, 0xDE, 0x09, 0xB5, 0x17, 0x47, 0xDB,
    0xBB, 0x55, 0xA4, 0xFE, 0x7F, 0xC2, 0xFC, 0x4E
};

/* marshal a single SEI message sei, storing the marshalled representation
 * in bitstream bs */
void SEI::write(Bitstream& bs, const SPS& sps)
{
    BitCounter count;
    m_bitIf = &count;

    /* virtual writeSEI method, write to bit counter */
    writeSEI(sps);

    m_bitIf = &bs;
    uint32_t type = payloadType();
    for (; type >= 0xff; type -= 0xff)
        WRITE_CODE(0xff, 8, "payload_type");
    WRITE_CODE(type, 8, "payload_type");

    X265_CHECK(0 == (count.getNumberOfWrittenBits() & 7), "payload unaligned\n");
    uint32_t payloadSize = count.getNumberOfWrittenBits() >> 3;
    for (; payloadSize >= 0xff; payloadSize -= 0xff)
        WRITE_CODE(0xff, 8, "payload_size");
    WRITE_CODE(payloadSize, 8, "payload_size");

    /* virtual writeSEI method, write to bs */
    writeSEI(sps);
}

void SEI::writeByteAlign()
{
    // TODO: expose bs.writeByteAlignment() as virtual function
    if (m_bitIf->getNumberOfWrittenBits() % 8 != 0)
    {
        WRITE_FLAG(1, "bit_equal_to_one");
        while (m_bitIf->getNumberOfWrittenBits() % 8 != 0)
        {
            WRITE_FLAG(0, "bit_equal_to_zero");
        }
    }
}
