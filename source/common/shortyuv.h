/*****************************************************************************
 * x265: TShortYUV classes for short sized YUV-style frames
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
 * For more information, contact us at licensing@multicorewareinc.com
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

    int16_t* m_bufY;
    int16_t* m_bufCb;
    int16_t* m_bufCr;

    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_cwidth;
    uint32_t m_cheight;

    int m_csp;
    int m_hChromaShift;
    int m_vChromaShift;

    ShortYuv();
    virtual ~ShortYuv();

    int getChromaAddrOffset(uint32_t partUnitIdx, uint32_t width)
    {
        int blkX = g_rasterToPelX[g_zscanToRaster[partUnitIdx]] >> m_hChromaShift;
        int blkY = g_rasterToPelY[g_zscanToRaster[partUnitIdx]] >> m_vChromaShift;

        return blkX + blkY * width;
    }

    static int getAddrOffset(uint32_t idx, uint32_t width)
    {
        int blkX = g_rasterToPelX[g_zscanToRaster[idx]];
        int blkY = g_rasterToPelY[g_zscanToRaster[idx]];

        return blkX + blkY * width;
    }

    static int getAddrOffset(uint32_t idx, uint32_t size, uint32_t width)
    {
        int blkX = (idx * size) &  (width - 1);
        int blkY = (idx * size) & ~(width - 1);

        return blkX + blkY * size;
    }

    int getChromaAddrOffset(uint32_t unitIdx, uint32_t size, uint32_t width)
    {
        int blkX = (unitIdx * size) &  (width - 1);
        int blkY = (unitIdx * size) & ~(width - 1);

        if (m_csp == CHROMA_422) blkY <<= 1;

        return blkX + blkY * size;
    }

    bool create(uint32_t width, uint32_t height, int csp);

    void destroy();
    void clear();

    int16_t* getLumaAddr()  { return m_bufY; }

    int16_t* getCbAddr()    { return m_bufCb; }

    int16_t* getCrAddr()    { return m_bufCr; }

    // Access starting position of YUV partition unit buffer
    int16_t* getLumaAddr(uint32_t partUnitIdx) { return m_bufY + getAddrOffset(partUnitIdx, m_width); }

    int16_t* getCbAddr(uint32_t partUnitIdx) { return m_bufCb + getChromaAddrOffset(partUnitIdx, m_cwidth); }

    int16_t* getCrAddr(uint32_t partUnitIdx) { return m_bufCr + getChromaAddrOffset(partUnitIdx, m_cwidth); }

    // Access starting position of YUV transform unit buffer
    int16_t* getLumaAddr(uint32_t partIdx, uint32_t size) { return m_bufY + getAddrOffset(partIdx, size, m_width); }

    int16_t* getCbAddr(uint32_t partIdx, uint32_t size) { return m_bufCb + getChromaAddrOffset(partIdx, size, m_cwidth); }

    int16_t* getCrAddr(uint32_t partIdx, uint32_t size) { return m_bufCr + getChromaAddrOffset(partIdx, size, m_cwidth); }

    void subtract(TComYuv* srcYuv0, TComYuv* srcYuv1, uint32_t partSize);
    void addClip(ShortYuv* srcYuv0, ShortYuv* srcYuv1, uint32_t partSize);

    void copyPartToPartLuma(ShortYuv* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height);
    void copyPartToPartChroma(ShortYuv* dstPicYuv, uint32_t partIdx, uint32_t lumaSize, bool bChromaSame);
    void copyPartToPartShortChroma(ShortYuv* dstPicYuv, uint32_t partIdx, uint32_t lumaSize, uint32_t chromaId);

    void copyPartToPartLuma(TComYuv* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height);
    void copyPartToPartChroma(TComYuv* dstPicYuv, uint32_t partIdx, uint32_t lumaSize, bool bChromaSame);
    void copyPartToPartYuvChroma(TComYuv* dstPicYuv, uint32_t partIdx, uint32_t lumaSize, uint32_t chromaId, const bool splitIntoSubTUs);

    // -------------------------------------------------------------------------------------------------------------------
    // member functions to support multiple color space formats
    // -------------------------------------------------------------------------------------------------------------------

    int  getHorzChromaShift()  { return m_hChromaShift; }
    int  getVertChromaShift()  { return m_vChromaShift; }
};
}

#endif // ifndef X265_SHORTYUV_H
