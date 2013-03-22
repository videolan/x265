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
#include "libmd5/MD5.h"

using namespace x265;

typedef struct
{
    unsigned char digest[16];
}
CUData;

typedef struct
{
    volatile bool active;
    volatile int  curCol;
}
RowData;

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

public:

    MD5Frame(ThreadPool* pool) : QueueFrame(pool), cu(0), row(0) {}
    ~MD5Frame();

    void Initialize( int cols, int rows );

    void Encode();

    // called by worker threads
    void ProcessRow( int row );
};

int main(int, char **)
{
    ThreadPool *pool = ThreadPool::AllocThreadPool(); // default size

    pool->Release();
}
