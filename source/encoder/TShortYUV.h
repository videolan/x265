/*****************************************************************************
 * x265: TshortYUV classes for short based YUV-style frames
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

#include "TLibCommon\CommonDef.h"
#include "TLibCommon\TComPicYuv.h"

class TShortYUV
{
private:

    short* YBuf;
    short* CbBuf;
    short* CrBuf;

    unsigned int width;
    unsigned int height;
    unsigned int Cwidth;
    unsigned int Cheight;
       
    static Int getAddrOffset(UInt uiPartUnitIdx, UInt width)
    {
        Int blkX = g_auiRasterToPelX[g_auiZscanToRaster[uiPartUnitIdx]];
        Int blkY = g_auiRasterToPelY[g_auiZscanToRaster[uiPartUnitIdx]];

        return blkX + blkY * width;
    }

    static Int getAddrOffset(UInt iTransUnitIdx, UInt iBlkSize, UInt width)
    {
        Int blkX = (iTransUnitIdx * iBlkSize) &  (width - 1);
        Int blkY = (iTransUnitIdx * iBlkSize) & ~(width - 1);

        return blkX + blkY * iBlkSize;
    }
public:

    TShortYUV();
    virtual ~TShortYUV();
    
    void create(unsigned int Width, unsigned int Height);
    void destroy();
    void clear();    

    Short*    getLumaAddr()    { return YBuf; }

    Short*    getCbAddr()    { return CbBuf; }

    Short*    getCrAddr()    { return CrBuf; }

    //  Access starting position of YUV partition unit buffer
    Short* getLumaAddr(UInt iPartUnitIdx) { return YBuf +   getAddrOffset(iPartUnitIdx, width); }

    Short* getCbAddr(UInt iPartUnitIdx) { return CbBuf + (getAddrOffset(iPartUnitIdx, Cwidth) >> 1); }

    Short* getCrAddr(UInt iPartUnitIdx) { return CrBuf + (getAddrOffset(iPartUnitIdx, Cwidth) >> 1); }

    //  Access starting position of YUV transform unit buffer
    Short* getLumaAddr(UInt iTransUnitIdx, UInt iBlkSize) { return YBuf + getAddrOffset(iTransUnitIdx, iBlkSize, width); }

    Short* getCbAddr(UInt iTransUnitIdx, UInt iBlkSize) { return CbBuf + getAddrOffset(iTransUnitIdx, iBlkSize, Cwidth); }

    Short* getCrAddr(UInt iTransUnitIdx, UInt iBlkSize) { return CrBuf + getAddrOffset(iTransUnitIdx, iBlkSize, Cwidth); }


    unsigned int    getHeight()    { return height; }

    unsigned int    getWidth()    { return width; }

    unsigned int    getCHeight()    { return Cheight; }

    unsigned int    getCWidth()    { return Cwidth; }
};



#endif //end __TSHORTYUV__