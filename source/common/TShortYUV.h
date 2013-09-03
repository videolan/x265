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

#ifndef __TSHORTYUV__
#define __TSHORTYUV__

#include "TLibCommon/TComRom.h"

namespace x265 {
// private namespace

class TComYuv;

class TShortYUV
{
private:

    static int getAddrOffset(unsigned int idx, unsigned int width)
    {
        int blkX = g_rasterToPelX[g_zscanToRaster[idx]];
        int blkY = g_rasterToPelY[g_zscanToRaster[idx]];

        return blkX + blkY * width;
    }

    static int getAddrOffset(unsigned int idx, unsigned int size, unsigned int width)
    {
        int blkX = (idx * size) &  (width - 1);
        int blkY = (idx * size) & ~(width - 1);

        return blkX + blkY * size;
    }

public:

    short* m_bufY;
    short* m_bufCb;
    short* m_bufCr;

    unsigned int m_width;
    unsigned int m_height;
    unsigned int m_cwidth;
    unsigned int m_cheight;

    TShortYUV();
    virtual ~TShortYUV();

    void create(unsigned int width, unsigned int height);
    void destroy();
    void clear();

    short* getLumaAddr()  { return m_bufY; }

    short* getCbAddr()    { return m_bufCb; }

    short* getCrAddr()    { return m_bufCr; }

    //  Access starting position of YUV partition unit buffer
    short* getLumaAddr(unsigned int partUnitIdx) { return m_bufY + getAddrOffset(partUnitIdx, m_width); }

    short* getCbAddr(unsigned int partUnitIdx) { return m_bufCb + (getAddrOffset(partUnitIdx, m_cwidth) >> 1); }

    short* getCrAddr(unsigned int partUnitIdx) { return m_bufCr + (getAddrOffset(partUnitIdx, m_cwidth) >> 1); }

    //  Access starting position of YUV transform unit buffer
    short* getLumaAddr(unsigned int partIdx, unsigned int size) { return m_bufY + getAddrOffset(partIdx, size, m_width); }

    short* getCbAddr(unsigned int partIdx, unsigned int size) { return m_bufCb + getAddrOffset(partIdx, size, m_cwidth); }

    short* getCrAddr(unsigned int partIdx, unsigned int size) { return m_bufCr + getAddrOffset(partIdx, size, m_cwidth); }

    void subtractLuma(TComYuv* srcYuv0, TComYuv* srcYuv1, unsigned int trUnitIdx, unsigned int partSize);
    void subtractChroma(TComYuv* srcYuv0, TComYuv* srcYuv1, unsigned int trUnitIdx, unsigned int partSize);
    void subtract(TComYuv* srcYuv0, TComYuv* srcYuv1, unsigned int trUnitIdx, unsigned int partSize);

    void addClip(TShortYUV* srcYuv0, TShortYUV* srcYuv1, unsigned int trUnitIdx, unsigned int partSize);
    void addClipLuma(TShortYUV* srcYuv0, TShortYUV* srcYuv1, unsigned int trUnitIdx, unsigned int partSize);
    void addClipChroma(TShortYUV* srcYuv0, TShortYUV* srcYuv1, unsigned int trUnitIdx, unsigned int partSize);

    void copyPartToPartYuv(TShortYUV* dstPicYuv, unsigned int partIdx, unsigned int width, unsigned int height);
    void copyPartToPartLuma(TShortYUV* dstPicYuv, unsigned int partIdx, unsigned int width, unsigned int height);
    void copyPartToPartChroma(TShortYUV* dstPicYuv, unsigned int partIdx, unsigned int width, unsigned int height);
    void copyPartToPartChroma(TShortYUV* dstPicYuv, unsigned int partIdx, unsigned int iWidth, unsigned int iHeight, unsigned int chromaId);

    void copyPartToPartYuv(TComYuv* dstPicYuv, unsigned int partIdx, unsigned int width, unsigned int height);
    void copyPartToPartLuma(TComYuv* dstPicYuv, unsigned int partIdx, unsigned int width, unsigned int height);
    void copyPartToPartChroma(TComYuv* dstPicYuv, unsigned int partIdx, unsigned int width, unsigned int height);
    void copyPartToPartChroma(TComYuv* dstPicYuv, unsigned int partIdx, unsigned int width, unsigned int height, unsigned int chromaId);
};
}

#endif //end __TSHORTYUV__
