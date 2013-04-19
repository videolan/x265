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

#ifndef _INPUT_H_
#define _INPUT_H_

#include <stdint.h>
#include <iostream>
#include <TLibCommon/TypeDef.h>

namespace x265 {
struct Picture
{
    void *planes[3];

    int   stride[3];

    int   bitDepth;

    ChromaFormat csp;
};

class Input
{
protected:

    virtual ~Input()  {}

public:

    Input()           {}

    static Input* Open(const char *filename);

    virtual void setDimensions(int width, int height) = 0;

    virtual void setRate(int numerator, int denominator) = 0;

    virtual void setBitDepth(int bitDepth) = 0;

    virtual float getRate() const = 0;

    virtual int getWidth() const = 0;

    virtual int getHeight() const = 0;

    virtual void release() = 0;

    virtual void skipFrames(int numFrames) = 0;

    virtual bool readPicture(Picture& pic) = 0;

    virtual bool isEof() const = 0;

    virtual bool isFail() const = 0;

    virtual int  guessFrameCount() const = 0;
};
}

#endif // _INPUT_H_
