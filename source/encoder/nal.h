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

#ifndef X265_NAL_H
#define X265_NAL_H

#include "common.h"

namespace x265 {
// private namespace

class TComOutputBitstream;

struct NALUnit
{
    uint32_t    m_packetSize;
    uint8_t*    m_nalUnitData;
    NalUnitType m_nalUnitType;

    NALUnit() : m_packetSize(0), m_nalUnitData(NULL), m_nalUnitType(NAL_UNIT_INVALID) {}
    ~NALUnit() { X265_FREE(m_nalUnitData); }

    void serialize(NalUnitType nalUnitType, const TComOutputBitstream& bs);
};

}

#endif // ifndef X265_NAL_H
