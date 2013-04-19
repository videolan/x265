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
 * For more information, contact us at licensing@multicorewareinc.com.
 *****************************************************************************/

#ifndef _YUV_H_
#define _YUV_H_

#include "input.h"
#include <stdio.h>

namespace x265 {
class YUVInput : public Input
{
protected:

    int rateNum;

    int rateDenom;

    int width;

    int height;

    int depth;

    int LumaMarginX;

    Pel* buf;

    FILE *fp;

    bool eof;

public:

    YUVInput(const char *filename);

    virtual ~YUVInput();

    void setDimensions(int w, int h)              { width = w;height = h;}

    void setRate(int numerator, int denominator)  { rateNum = numerator;rateDenom = denominator;}

    void setBitDepth(int bitDepth)                { depth = bitDepth;}

    void setLumaMarginX(int lumamarginX)          { LumaMarginX = lumamarginX;}

    float getRate() const                         { return ((float)rateNum) / rateDenom;}

    int getWidth() const                          { return width;}

    int getHeight() const                         { return height;}

    int getBitDepth() const                       { return depth;}

    virtual Pel* getBuf() const                  { return buf;}

    void allocBuf();              // set the char buffer to store raw data from file.

    bool isEof() const
    {
        if (feof(fp))
            return true;

        return false;
    }

    bool isFail() const                           { return !!fp;}

    void release()
    {
        if (fp)
            fclose(fp);

        delete this;
    }

    int  guessFrameCount() const;

    void skipFrames(int numFrames);

    bool readPicture(Picture&);
};
}

#endif // _YUV_H_
