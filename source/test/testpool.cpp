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

#include "threadpool.h"
#include "threading.h"
#include "libmd5/MD5.h"
#include "PPA/ppa.h"

#include <stdio.h>
#include <time.h>
#include <assert.h>

using namespace x265;

struct CUData
{
    unsigned char digest[16];
};

struct RowData
{
    RowData() : active(false), curCol(0) {}

    volatile bool active;
    volatile int  curCol;
};

// Create a fake frame class with manufactured data in each CU block.  We
// need to create an MD5 hash such that each CU's hash includes the hashes
// of the blocks that would have HEVC data dependencies (left, top-left,
// top, top-right).  This will give us one deterministic output hash.  We
// then generate the same hash using the thread pool and wave-front parallelism
// to verify the thread-pool behavior and the wave-front schedule data
// structures.
class MD5Frame : public QueueFrame
{
private:
    CUData  *cu;
    RowData *row;
    int      numrows;
    int      numcols;
    Event    complete;

public:

    MD5Frame(ThreadPool* pool) : QueueFrame(pool), cu(0), row(0) {}
    ~MD5Frame() { delete [] this->cu; delete [] this->row; }

    void Initialize( int cols, int rows );

    void Encode();

    // called by worker threads
    void ProcessRow( int row );
};

void MD5Frame::Initialize( int cols, int rows )
{
    this->cu = new CUData[ rows * cols ];
    this->row = new RowData[ rows ];
    this->numrows = rows;
    this->numcols = cols;

    if (!this->QueueFrame::InitJobQueue( rows ))
    {
        assert(!"Unable to initialize job queue");
    }
}

void MD5Frame::Encode()
{
    clock_t start = clock();

    this->JobProvider::Enqueue();

    this->complete.Wait();

    this->JobProvider::Dequeue();

    clock_t stop = clock();

    unsigned char *outdigest = this->cu[this->numrows * this->numcols - 1].digest;

    printf("%x %1.3fsec\n", outdigest, (float) (stop-start) / CLOCKS_PER_SEC);
}

void MD5Frame::ProcessRow( int rownum )
{   // Called by worker thread
    RowData &curRow = this->row[ rownum ];

    do
    {
        int id = rownum * this->numcols + curRow.curCol;
        CUData  &curCTU = this->cu[ id ];

        // * Fake CTU processing *
        PPAStartCpuEventFunc(encode_block)
        memset(curCTU.digest, id, sizeof(curCTU.digest));
        // MD5 hash
        // add hashes of all dependent CTUs
        PPAStopCpuEventFunc(encode_block)

        curRow.curCol++;
        if (curRow.curCol > 2 && rownum < this->numrows-1)
        {
            if (this->row[rownum+1].active == 0)
            {   // set active indicator so row is only enqueued once
                // row stays marked active until blocked or done
                this->row[rownum+1].active = 1;
                this->QueueFrame::EnqueueRow(rownum+1);
            }
        }
        if (rownum > 0 && row[rownum-1].curCol < curRow.curCol+2)
        {   // row is blocked, quit job
            curRow.active = 0;
            return;
        }
    }
    while (curRow.curCol < this->numcols);

    // * Row completed *

    if (rownum == this->numrows-1)
        this->complete.Trigger();
}

int main(int, char **)
{
    PPA_INIT();

    {
        ThreadPool *pool = ThreadPool::AllocThreadPool(1);
        MD5Frame frame(pool);
        frame.Initialize( 60, 40 );
        frame.Encode();
        pool->Release();
    }
    {
        ThreadPool *pool = ThreadPool::AllocThreadPool(2);
        MD5Frame frame(pool);
        frame.Initialize( 60, 40 );
        frame.Encode();
        pool->Release();
    }
    {
        ThreadPool *pool = ThreadPool::AllocThreadPool(4);
        MD5Frame frame(pool);
        frame.Initialize( 60, 40 );
        frame.Encode();
        pool->Release();
    }
    {
        ThreadPool *pool = ThreadPool::AllocThreadPool(8);
        MD5Frame frame(pool);
        frame.Initialize( 60, 40 );
        frame.Encode();
        pool->Release();
    }
}
