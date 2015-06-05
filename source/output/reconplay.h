/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Peixuan Zhang <zhangpeixuancn@gmail.com>
 *          Chunli Zhang <chunli@multicorewareinc.com>
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
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#ifndef X265_RECONPLAY_H
#define X265_RECONPLAY_H

#include "x265.h"
#include "threading.h"
#include <cstdio>

namespace X265_NS {
// private x265 namespace

class ReconPlay : public Thread
{
public:

    ReconPlay(const char* commandLine, x265_param& param);

    virtual ~ReconPlay();

    bool writePicture(const x265_picture& pic);

    static bool pipeValid;

protected:

    enum { RECON_BUF_SIZE = 40 };

    FILE*  outputPipe;     /* The output pipe for player */
    size_t frameSize;      /* size of one frame in pixels */
    bool   threadActive;   /* worker thread is active */
    int    width;          /* width of frame */
    int    height;         /* height of frame */
    int    colorSpace;     /* color space of frame */

    int    poc[RECON_BUF_SIZE];
    pixel* frameData[RECON_BUF_SIZE];

    /* Note that the class uses read and write counters to signal that reads and
     * writes have occurred in the ring buffer, but writes into the buffer
     * happen in decode order and the reader must check that the POC it next
     * needs to send to the pipe is in fact present.  The counters are used to
     * prevent the writer from getting too far ahead of the reader */
    ThreadSafeInteger readCount;
    ThreadSafeInteger writeCount;

    void threadMain();
    bool outputFrame();
};
}

#endif // ifndef X265_RECONPLAY_H
