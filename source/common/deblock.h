/*****************************************************************************
* Copyright (C) 2013 x265 project
*
* Author: Gopu Govindaswamy <gopu@multicorewareinc.com>
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

#ifndef X265_DEBLOCK_H
#define X265_DEBLOCK_H

#include "common.h"
#include "TLibCommon/TComRom.h"

namespace x265 {
// private namespace

class TComDataCU;

class Deblock
{
public:
    enum { EDGE_VER, EDGE_HOR };

    uint32_t m_numPartitions;

    Deblock() : m_numPartitions(0) {}

    void init() { m_numPartitions = 1 << g_maxFullDepth * 2; }

    void deblockCTU(TComDataCU* cu, int32_t dir, bool edgeFilter[], uint8_t blockingStrength[]);

protected:

    // CU-level deblocking function
    void deblockCU(TComDataCU* cu, uint32_t absZOrderIdx, uint32_t depth, const int32_t Edge, bool edgeFilter[], uint8_t blockingStrength[]);

    struct Param
    {
        bool leftEdge;
        bool topEdge;
    };

    // set filtering functions
    void setLoopfilterParam(TComDataCU* cu, uint32_t absZOrderIdx, Param *params);
    void setEdgefilterTU(TComDataCU* cu, uint32_t absTUPartIdx, uint32_t absZOrderIdx, uint32_t depth, int32_t dir, bool edgeFilter[], uint8_t blockingStrength[]);
    void setEdgefilterPU(TComDataCU* cu, uint32_t absZOrderIdx, int32_t dir, Param *params, bool edgeFilter[], uint8_t blockingStrength[]);
    void setEdgefilterMultiple(TComDataCU* cu, uint32_t absZOrderIdx, uint32_t depth, int32_t dir, int32_t edgeIdx, bool value, bool edgeFilter[], uint8_t blockingStrength[], uint32_t widthInBaseUnits = 0);

    // get filtering functions
    void getBoundaryStrengthSingle(TComDataCU* cu, int32_t dir, uint32_t partIdx, uint8_t blockingStrength[]);

    // filter luma/chroma functions
    void edgeFilterLuma(TComDataCU* cu, uint32_t absZOrderIdx, uint32_t depth, int32_t dir, int32_t edge, uint8_t blockingStrength[]);
    void edgeFilterChroma(TComDataCU* cu, uint32_t absZOrderIdx, uint32_t depth, int32_t dir, int32_t edge, uint8_t blockingStrength[]);

    static const uint8_t s_tcTable[54];
    static const uint8_t s_betaTable[52];
};
}
#endif // ifndef X265_DEBLOCK_H
