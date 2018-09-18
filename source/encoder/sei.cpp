/*****************************************************************************
* Copyright (C) 2013-2017 MulticoreWare, Inc
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
void SEI::writeSEImessages(Bitstream& bs, const SPS& sps, NalUnitType nalUnitType, NALList& list, int isNested)
{
    if (!isNested)
        bs.resetBits();

    BitCounter counter;
    m_bitIf = &counter;
    writeSEI(sps);
    /* count the size of the payload and return the size in bits */
    X265_CHECK(0 == (counter.getNumberOfWrittenBits() & 7), "payload unaligned\n");
    uint32_t payloadData = counter.getNumberOfWrittenBits() >> 3;

    // set bitstream
    m_bitIf = &bs;

    uint32_t payloadType = m_payloadType;
    for (; payloadType >= 0xff; payloadType -= 0xff)
        WRITE_CODE(0xff, 8, "payload_type");
    WRITE_CODE(payloadType, 8, "payload_type");

    uint32_t payloadSize = payloadData;
    for (; payloadSize >= 0xff; payloadSize -= 0xff)
        WRITE_CODE(0xff, 8, "payload_size");
    WRITE_CODE(payloadSize, 8, "payload_size");

    // virtual writeSEI method, write to bs 
    writeSEI(sps);

    if (!isNested)
    {
        bs.writeByteAlignment();
        list.serialize(nalUnitType, bs);
    }
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

void SEI::setSize(uint32_t size)
{
    m_payloadSize = size;
}

/* charSet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/" */

char* SEI::base64Decode(char encodedString[], int base64EncodeLength)
{
    char* decodedString;
    decodedString = (char*)malloc(sizeof(char) * ((base64EncodeLength / 4) * 3));
    int i, j, k = 0;
    // stores the bitstream
    int bitstream = 0;
    // countBits stores current number of bits in bitstream
    int countBits = 0;
    // selects 4 characters from encodedString at a time. Find the position of each encoded character in charSet and stores in bitstream
    for (i = 0; i < base64EncodeLength; i += 4)
    {
        bitstream = 0, countBits = 0;
        for (j = 0; j < 4; j++)
        {
            // make space for 6 bits
            if (encodedString[i + j] != '=')
            {
                bitstream = bitstream << 6;
                countBits += 6;
            }
            // Finding the position of each encoded character in charSet and storing in bitstream, use OR '|' operator to store bits

            if (encodedString[i + j] >= 'A' && encodedString[i + j] <= 'Z')
                bitstream = bitstream | (encodedString[i + j] - 'A');

            else if (encodedString[i + j] >= 'a' && encodedString[i + j] <= 'z')
                bitstream = bitstream | (encodedString[i + j] - 'a' + 26);
            
            else if (encodedString[i + j] >= '0' && encodedString[i + j] <= '9')
                bitstream = bitstream | (encodedString[i + j] - '0' + 52);
            
            // '+' occurs in 62nd position in charSet
            else if (encodedString[i + j] == '+')
                bitstream = bitstream | 62;
            
            // '/' occurs in 63rd position in charSet
            else if (encodedString[i + j] == '/')
                bitstream = bitstream | 63;
            
            // to delete appended bits during encoding
            else
            {
                bitstream = bitstream >> 2;
                countBits -= 2;
            }
        }
    
        while (countBits != 0)
        {
            countBits -= 8;
            decodedString[k++] = (bitstream >> countBits) & 255;
        }
    }
    return decodedString;
}

