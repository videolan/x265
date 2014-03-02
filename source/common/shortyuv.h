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

    bool create(uint32_t width, uint32_t height, int csp);

    void destroy();
    void clear();

    int16_t* getLumaAddr()  { return m_bufY; }

    int16_t* getCbAddr()    { return m_bufCb; }

    int16_t* getCrAddr()    { return m_bufCr; }

    //  Access starting position of YUV partition unit buffer
    int16_t* getLumaAddr(uint32_t partUnitIdx) { return m_bufY + getAddrOffset(partUnitIdx, m_width); }

    int16_t* getCbAddr(uint32_t partUnitIdx) { return m_bufCb + (getAddrOffset(partUnitIdx, m_cwidth) >> m_hChromaShift); }

    int16_t* getCrAddr(uint32_t partUnitIdx) { return m_bufCr + (getAddrOffset(partUnitIdx, m_cwidth) >> m_hChromaShift); }

    //  Access starting position of YUV transform unit buffer
    int16_t* getLumaAddr(uint32_t partIdx, uint32_t size) { return m_bufY + getAddrOffset(partIdx, size, m_width); }

    int16_t* getCbAddr(uint32_t partIdx, uint32_t size) { return m_bufCb + getAddrOffset(partIdx, size, m_cwidth); }

    int16_t* getCrAddr(uint32_t partIdx, uint32_t size) { return m_bufCr + getAddrOffset(partIdx, size, m_cwidth); }

    void subtractLuma(TComYuv* srcYuv0, TComYuv* srcYuv1, uint32_t trUnitIdx, uint32_t partSize, uint32_t part);
    void subtractChroma(TComYuv* srcYuv0, TComYuv* srcYuv1, uint32_t trUnitIdx, uint32_t partSize, uint32_t part);
    void subtract(TComYuv* srcYuv0, TComYuv* srcYuv1, uint32_t trUnitIdx, uint32_t partSize);

    void addClip(ShortYuv* srcYuv0, ShortYuv* srcYuv1, uint32_t trUnitIdx, uint32_t partSize);
    void addClipLuma(ShortYuv* srcYuv0, ShortYuv* srcYuv1, uint32_t trUnitIdx, uint32_t partSize);
    void addClipChroma(ShortYuv* srcYuv0, ShortYuv* srcYuv1, uint32_t trUnitIdx, uint32_t partSize);

    void copyPartToPartYuv(ShortYuv* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height);
    void copyPartToPartLuma(ShortYuv* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height);
    void copyPartToPartChroma(ShortYuv* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height);
    void copyPartToPartChroma(ShortYuv* dstPicYuv, uint32_t partIdx, uint32_t iWidth, uint32_t iHeight, uint32_t chromaId);

    void copyPartToPartYuv(TComYuv* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height);
    void copyPartToPartLuma(TComYuv* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height);
    void copyPartToPartChroma(TComYuv* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height);
    void copyPartToPartChroma(TComYuv* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height, uint32_t chromaId);
};
}

#endif // ifndef X265_SHORTYUV_H
