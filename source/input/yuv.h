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

#ifndef X265_YUV_H
#define X265_YUV_H

#include "input.h"
#include <fstream>

#if defined(ENABLE_THREAD)
#define QUEUE_SIZE 5
#include "threading.h"
#endif

namespace x265 {
// private x265 namespace

#if defined(ENABLE_THREAD)
class YUVInput : public Input, public Thread
#else
class YUVInput : public Input
#endif
{
protected:

    int width;

    int height;

    int depth;

    int pixelbytes;

    int framesize;

    bool threadActive;

#if defined(ENABLE_THREAD)
    volatile int head;

    volatile int tail;

    bool frameStat[QUEUE_SIZE];

    char* buf[QUEUE_SIZE];

    Event notFull;

    Event notEmpty;
#else // if defined(ENABLE_THREAD)
    char* buf;
#endif // if defined(ENABLE_THREAD)

    std::istream *ifs;

public:

    YUVInput(const char *filename);

    virtual ~YUVInput();

    void setDimensions(int w, int h);

    void setBitDepth(int bitDepth)                { depth = bitDepth; }

    float getRate() const                         { return 0.0f; }

    int getWidth() const                          { return width; }

    int getHeight() const                         { return height; }

    bool isEof() const                            { return (ifs && ifs->eof()); }

    bool isFail()                                 { return !(ifs && !ifs->fail() && threadActive); }

    void startReader();

    void release();

    int  guessFrameCount();

    void skipFrames(int numFrames);

    bool readPicture(x265_picture&);

#if defined(ENABLE_THREAD)

    void threadMain();

    bool populateFrameQueue();

#endif

    const char *getName() const                   { return "yuv"; }
};
}

#endif // ifndef X265_YUV_H
