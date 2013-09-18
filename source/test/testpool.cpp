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
#include "wavefront.h"
#include "threading.h"
#include "md5.h"
#include "PPA/ppa.h"

#include <time.h>
#include <assert.h>
#include <string.h>
#include <sstream>
#include <iostream>

using namespace x265;

struct CUData
{
    CUData()
    {
        memset(digest, 0, sizeof(digest));
    }

    unsigned char digest[16];
};

struct RowData
{
    RowData() : active(false), curCol(0) {}

    Lock          lock;
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
class MD5Frame : public WaveFront
{
private:

    CUData  *cu;
    RowData *row;
    int      numrows;
    int      numcols;
    Event    complete;

public:

    MD5Frame(ThreadPool *pool) : WaveFront(pool), cu(0), row(0) {}

    virtual ~MD5Frame()
    {
        // ensure no threads are lingering on FindJob() before allowing
        // this object's vtable to be destroyed
        JobProvider::flush();

        delete[] this->cu;
        delete[] this->row;
    }

    void initialize(int cols, int rows);

    void encode();

    void processRow(int row);
};

void MD5Frame::initialize(int cols, int rows)
{
    this->cu = new CUData[rows * cols];
    this->row = new RowData[rows];
    this->numrows = rows;
    this->numcols = cols;

    if (!this->WaveFront::init(rows))
    {
        assert(!"Unable to initialize job queue");
    }
}

void MD5Frame::encode()
{
    this->JobProvider::enqueue();

    this->WaveFront::enqueueRow(0);

    this->complete.wait();

    this->JobProvider::dequeue();

    unsigned int *outdigest = (unsigned int*)this->cu[this->numrows * this->numcols - 1].digest;

    std::stringstream ss;

    for (int i = 0; i < 4; i++)
    {
        ss << std::hex << outdigest[i];
    }

    if (ss.str().compare("da667b741a7a9d0ee862158da2dd1882"))
        std::cout << "Bad hash: " << ss.str() << std::endl;
}

void MD5Frame::processRow(int rownum)
{
    // Called by worker thread
    RowData &curRow = this->row[rownum];

    assert(rownum < this->numrows && rownum >= 0);
    assert(curRow.curCol < this->numcols);

    while (curRow.curCol < this->numcols)
    {
        int id = rownum * this->numcols + curRow.curCol;
        CUData  &curCTU = this->cu[id];
        MD5 hash;

        // * Fake CTU processing *
        PPAStartCpuEventFunc(encode_block);
        memset(curCTU.digest, id, sizeof(curCTU.digest));
        hash.update(curCTU.digest, sizeof(curCTU.digest));
        if (curRow.curCol > 0)
            hash.update(this->cu[id - 1].digest, sizeof(curCTU.digest));

        if (rownum > 0)
        {
            if (curRow.curCol > 0)
                hash.update(this->cu[id - this->numcols - 1].digest, sizeof(curCTU.digest));

            hash.update(this->cu[id - this->numcols].digest, sizeof(curCTU.digest));
            if (curRow.curCol < this->numcols - 1)
                hash.update(this->cu[id - this->numcols + 1].digest, sizeof(curCTU.digest));
        }

        hash.finalize(curCTU.digest);
        PPAStopCpuEventFunc(encode_block);

        curRow.curCol++;

        if (curRow.curCol >= 2 && rownum < this->numrows - 1)
        {
            ScopedLock below(this->row[rownum + 1].lock);

            if (this->row[rownum + 1].active == false &&
                this->row[rownum + 1].curCol + 2 <= curRow.curCol)
            {
                // set active indicator so row is only enqueued once
                // row stays marked active until blocked or done
                this->row[rownum + 1].active = true;
                this->WaveFront::enqueueRow(rownum + 1);
            }
        }

        ScopedLock self(curRow.lock);

        if (rownum > 0 &&
            curRow.curCol < this->numcols - 1 &&
            this->row[rownum - 1].curCol < curRow.curCol + 2)
        {
            // row is blocked, quit job
            curRow.active = false;
            return;
        }
    }

    // * Row completed *

    if (rownum == this->numrows - 1)
        this->complete.trigger();
}

int main(int, char **)
{
    ThreadPool *pool;

    PPA_INIT();

    pool = ThreadPool::allocThreadPool(1);
    {
        MD5Frame frame(pool);
        frame.initialize(60, 40);
        frame.encode();
    }
    pool->release();
    pool = ThreadPool::allocThreadPool(2);
    {
        MD5Frame frame(pool);
        frame.initialize(60, 40);
        frame.encode();
    }
    pool->release();
    pool = ThreadPool::allocThreadPool(4);
    {
        MD5Frame frame(pool);
        frame.initialize(60, 40);
        frame.encode();
    }
    pool->release();
    pool = ThreadPool::allocThreadPool(8);
    {
        MD5Frame frame(pool);
        frame.initialize(60, 40);
        frame.encode();
    }
    pool->release();
}
