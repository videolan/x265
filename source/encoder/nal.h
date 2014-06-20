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
#include "x265.h"

namespace x265 {
// private namespace

class Bitstream;

class NALList
{
    static const int MAX_NAL_UNITS = 16;

public:

    x265_nal    m_nal[MAX_NAL_UNITS];
    uint32_t    m_numNal;

    uint8_t*    m_buffer;
    uint32_t    m_occupancy;
    uint32_t    m_allocSize;

    NALList() : m_numNal(0), m_buffer(NULL), m_occupancy(0), m_allocSize(0) {}
    ~NALList() { X265_FREE(m_buffer); }

    void takeContents(NALList& other);

    void serialize(NalUnitType nalUnitType, const Bitstream& bs, uint8_t *extra = NULL, uint32_t extraBytes = 0);

    static uint8_t *serializeMultiple(uint32_t* streamSizeBytes, uint32_t& totalBytes, uint32_t streamCount, const Bitstream* streams);
};

}

#endif // ifndef X265_NAL_H
