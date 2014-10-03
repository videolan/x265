/*****************************************************************************
 * x265: ShortYUV class for short sized YUV-style frames
 *****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Deepthi Nandakumar <deepthi@multicorewareinc.com>
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
 * For more information, contact us at license @ x265.com
 *****************************************************************************/

#ifndef X265_SHORTYUV_H
#define X265_SHORTYUV_H

#include "TLibCommon/TComRom.h"

namespace x265 {
// private namespace

class TComYuv;

class ShortYuv
{
public:

    int16_t* m_buf[3];

    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_cwidth;
    uint32_t m_cheight;

    int m_csp;
    int m_hChromaShift;
    int m_vChromaShift;

    ShortYuv();
    ~ShortYuv();

    int getChromaAddrOffset(uint32_t idx, uint32_t width)
    {
        int blkX = g_zscanToPelX[idx] >> m_hChromaShift;
        int blkY = g_zscanToPelY[idx] >> m_vChromaShift;

        return blkX + blkY * width;
    }

    static int getAddrOffset(uint32_t idx, uint32_t width)
    {
        int blkX = g_zscanToPelX[idx];
        int blkY = g_zscanToPelY[idx];

        return blkX + blkY * width;
    }

    bool create(uint32_t width, uint32_t height, int csp);

    void destroy();
    void clear();

    int16_t* getLumaAddr()  { return m_buf[0]; }

    int16_t* getCbAddr()    { return m_buf[1]; }

    int16_t* getCrAddr()    { return m_buf[2]; }

    int16_t* getChromaAddr(uint32_t chromaId)    { return m_buf[chromaId]; }

    // Access starting position of YUV partition unit buffer
    int16_t* getLumaAddr(uint32_t partUnitIdx) { return m_buf[0] + getAddrOffset(partUnitIdx, m_width); }

    int16_t* getCbAddr(uint32_t partUnitIdx) { return m_buf[1] + getChromaAddrOffset(partUnitIdx, m_cwidth); }

    int16_t* getCrAddr(uint32_t partUnitIdx) { return m_buf[2] + getChromaAddrOffset(partUnitIdx, m_cwidth); }

    int16_t* getChromaAddr(uint32_t chromaId, uint32_t partUnitIdx) { return m_buf[chromaId] + getChromaAddrOffset(partUnitIdx, m_cwidth); }

    void subtract(TComYuv* srcYuv0, TComYuv* srcYuv1, uint32_t log2Size);

    void copyPartToPartLuma(ShortYuv* dstPicYuv, uint32_t partIdx, uint32_t log2Size);
    void copyPartToPartChroma(ShortYuv* dstPicYuv, uint32_t partIdx, uint32_t log2SizeL);
    void copyPartToPartLuma(TComYuv* dstPicYuv, uint32_t partIdx, uint32_t log2Size);
    void copyPartToPartChroma(TComYuv* dstPicYuv, uint32_t partIdx, uint32_t log2SizeL);
};
}

#endif // ifndef X265_SHORTYUV_H
