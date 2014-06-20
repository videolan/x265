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
#include "TLibCommon/TComSlice.h"
#include "sei.h"

using namespace x265;

#if ENC_DEC_TRACE
#define LOG(string) fprintf(g_hTrace, string)
#else
#define LOG(string)
#endif

/**
 * marshal a single SEI message sei, storing the marshalled representation
 * in bitstream bs.
 */
void SEI::write(Bitstream& bs, TComSPS& sps)
{
    /* disable logging while we measure the SEI */
#if ENC_DEC_TRACE
    bool traceEnable = g_HLSTraceEnable;
    g_HLSTraceEnable = false;
#endif

    BitCounter count;
    setBitstream(&count);
    writeSEI(sps);

#if ENC_DEC_TRACE
    g_HLSTraceEnable = traceEnable;
#endif

    LOG("=========== SEI message ===========\n");

    setBitstream(&bs);
    uint32_t type = payloadType();
    for (; type >= 0xff; type -= 0xff)
        WRITE_CODE(0xff, 8, "payload_type");
    WRITE_CODE(type, 8, "payload_type");

    X265_CHECK(0 == (count.getNumberOfWrittenBits() & 7), "payload unaligned\n");
    uint32_t payloadSize = count.getNumberOfWrittenBits() >> 3;
    for (; payloadSize >= 0xff; payloadSize -= 0xff)
        WRITE_CODE(0xff, 8, "payload_size");
    WRITE_CODE(payloadSize, 8, "payload_size");

    /* virtual writeSEI method */
    writeSEI(sps);
}

void SEI::writeByteAlign()
{
    // TODO: bs.writeByteAlignment()
    if (m_bitIf->getNumberOfWrittenBits() % 8 != 0)
    {
        WRITE_FLAG(1, "bit_equal_to_one");
        while (m_bitIf->getNumberOfWrittenBits() % 8 != 0)
        {
            WRITE_FLAG(0, "bit_equal_to_zero");
        }
    }
}
