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
#include <fstream>

namespace x265 {
// private x265 namespace

class YUVInput : public Input
{
protected:

    int width;

    int height;

    int depth;

    char* buf;

    std::ifstream ifs;

public:

    YUVInput(const char *filename);

    virtual ~YUVInput();

    void setDimensions(int w, int h)              { width = w; height = h; }

    void setBitDepth(int bitDepth)                { depth = bitDepth; }

    float getRate() const                         { return 0.0f; }

    int getWidth() const                          { return width; }

    int getHeight() const                         { return height; }

    bool isEof() const                            { return ifs.eof(); }

    bool isFail() const                           { return !ifs.is_open(); }

    void release()                                { delete this; }

    int  guessFrameCount();

    void skipFrames(int numFrames);

    bool readPicture(x265_picture&);
};
}

#endif // _YUV_H_
