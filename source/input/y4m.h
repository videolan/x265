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

#ifndef X265_Y4M_H
#define X265_Y4M_H

#include "input.h"
#include "threading.h"
#include <fstream>

#define QUEUE_SIZE 5

namespace x265 {
// x265 private namespace

class Y4MInput : public Input, public Thread
{
protected:

    uint32_t rateNum;

    uint32_t rateDenom;

    int width;

    int height;

    int colorSpace;   ///< source Color Space Parameter

    uint32_t plane_size[3];

    uint32_t plane_stride[3];

    bool threadActive;

    volatile int head;

    volatile int tail;

    bool frameStat[QUEUE_SIZE];

    char* plane[QUEUE_SIZE][3];

    Event notFull;

    Event notEmpty;

    std::istream *ifs;

    bool parseHeader();

public:

    Y4MInput(const char *filename, uint32_t inputBitDepth);

    virtual ~Y4MInput();

    void setDimensions(int, int)  { /* ignore, warn */ }

    void setBitDepth(uint32_t)    { /* ignore, warn */ }

    void setColorSpace(int)       { /* ignore, warn */ }

    float getRate() const         { return ((float)rateNum) / rateDenom; }

    int getWidth() const          { return width; }

    int getHeight() const         { return height; }

    int getColorSpace() const     { return colorSpace; }

    bool isEof() const            { return ifs && ifs->eof();  }

    bool isFail()                 { return !(ifs && !ifs->fail() && threadActive); }

    void startReader();

    void release();

    int  guessFrameCount();

    void skipFrames(uint32_t numFrames);

    bool readPicture(x265_picture&);

    void pictureAlloc(int index);

    void threadMain();

    bool populateFrameQueue();

    const char *getName() const   { return "y4m"; }
};
}

#endif // ifndef X265_Y4M_H
