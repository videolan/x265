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

#ifndef X265_WAVEFRONT_H
#define X265_WAVEFRONT_H

#include "threadpool.h"
#include <stdint.h>

namespace x265 {
// x265 private namespace

// Generic wave-front scheduler, manages busy-state of CU rows as a priority
// queue (higher CU rows have priority over lower rows)
//
// Derived classes must implement ProcessRow().
class WaveFront : public JobProvider
{
private:

    // bitmap of rows queued for processing, uses atomic intrinsics
    uint64_t volatile *m_queuedBitmap;
    uint64_t volatile *m_enableBitmap;

    // number of words in the bitmap
    int m_numWords;

    int m_numRows;

public:

    WaveFront(ThreadPool *pool) : JobProvider(pool), m_queuedBitmap(0) {}

    virtual ~WaveFront();

    // If returns false, the frame must be encoded in series.
    bool init(int numRows);

    // Enqueue a row to be processed. A worker thread will later call ProcessRow(row)
    // This provider must be enqueued in the pool before enqueuing a row
    void enqueueRow(int row);

    void enableRow(int row);

    void enableAllRows();

    void clearEnabledRowMask();

    // WaveFront's implementation of JobProvider::findJob. Consults
    // m_queuedBitmap and calls ProcessRow(row) for lowest numbered queued row
    // or returns false
    bool findJob();

    // Returns true if a row above curRow is available for processing.  The processRow()
    // method may call this function periodically and voluntarily exit
    bool checkHigherPriorityRow(int curRow);

    // Start or resume encode processing of this row, must be implemented by
    // derived classes.
    virtual void processRow(int row) = 0;
};
} // end namespace x265

#endif // ifndef X265_WAVEFRONT_H
