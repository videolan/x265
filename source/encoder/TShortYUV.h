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
        
public:

    TShortYUV();
    virtual ~TShortYUV();
    
    void create(unsigned int Width, unsigned int Height);
    void destroy();
    void clear();    

    Short*    getLumaAddr()    { return YBuf; }

    Short*    getCbAddr()    { return CbBuf; }

    Short*    getCrAddr()    { return CrBuf; }

    unsigned int    getHeight()    { return height; }

    unsigned int    getWidth()    { return width; }

    unsigned int    getCHeight()    { return Cheight; }

    unsigned int    getCWidth()    { return Cwidth; }
};



#endif //end __TSHORTYUV__