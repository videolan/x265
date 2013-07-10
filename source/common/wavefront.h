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
 * For more information, contact us at licensing@multicorewareinc.com
 *****************************************************************************/

#ifndef _WAVEFRONT_H_
#define _WAVEFRONT_H_

#include "threadpool.h"
#include <stdint.h>

namespace x265 {
// x265 private namespace

// Generic wave-front scheduler, manages busy-state of frame rows as a priority
// queue
//
// Frame processing classes derive from this class and implement ProcessRow().
// Processed row may be partially completed and need to be resumed.
class WaveFront : public JobProvider
{
private:

    //< bitmap of rows queued for processing, uses atomic intrinsics to
    //< set and clear bits, for thread safety
    uint64_t volatile *m_queuedBitmap;

    int m_numWords;

    int m_numRows;

    //< WaveFront's internal implementation. Consults queuedBitmap and calls
    //< ProcessRow(row) for lowest numbered queued row or returns false
    bool findJob();

public:

    WaveFront(ThreadPool *pool) : JobProvider(pool), m_queuedBitmap(0) {}

    virtual ~WaveFront();

    //< Must be called just once after the frame is allocated.  Returns true on
    //< success.  It it returns false, the frame must encode in series.
    bool initJobQueue(int numRows);

    //< Enqueue a row to be processed. A worker thread will later call ProcessRow(row)
    //< ThreadPoolEnqueue() must be called before EnqueueRow()
    void enqueueRow(int row);

    //< Start or resume encode processing of this row.  When the bottom row is
    //< finished, ThreadPoolDequeue() must be called.
    virtual void processRow(int row) = 0;
};
} // end namespace x265

#endif // ifndef _WAVEFRONT_H_
