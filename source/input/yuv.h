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
#include <stdint.h>

namespace x265 {
// private x265 namespace

class YUVInput : public Input
{
protected:

    int rateNum;

    int rateDenom;

    int width;

    int height;

    int depth;

    int LumaMarginX;

    uint8_t* buf;

    FILE *fp;

    bool eof;

public:

    YUVInput(const char *filename);

    virtual ~YUVInput();

    void setDimensions(int w, int h)              { width = w; height = h; }

    void setRate(int numerator, int denominator)  { rateNum = numerator; rateDenom = denominator; }

    void setBitDepth(int bitDepth)                { depth = bitDepth; }

    float getRate() const                         { return ((float)rateNum) / rateDenom; }

    int getWidth() const                          { return width; }

    int getHeight() const                         { return height; }

    int getBitDepth() const                       { return depth; }

    bool isEof() const                            { return !!feof(fp); }

    bool isFail() const                           { return !fp; }

    void release()
    {
        if (fp)
            fclose(fp);

        delete this;
    }

    int  guessFrameCount() const;

    void skipFrames(int numFrames);

    bool readPicture(x265_picture&);
};
}

#endif // _YUV_H_
