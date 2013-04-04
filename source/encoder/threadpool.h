/*****************************************************************************
 * x265: singleton thread pool and interface classes
 *****************************************************************************
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

#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include "stdint.h"

namespace x265 {
// x265 private namespace

class ThreadPool;

//< Any class that wants to distribute work to the thread pool must
//< derive from JobProvider and implement FindJob().
class JobProvider
{
protected:

    ThreadPool   *m_pool;

    JobProvider  *m_nextProvider;
    JobProvider  *m_prevProvider;

public:

    JobProvider(ThreadPool *p) : m_pool(p), m_nextProvider(0), m_prevProvider(0) {}

    virtual ~JobProvider();

    //< Register this job provider with the thread pool, jpbs are available
    void Enqueue();

    //< Remove this job provider from the thread pool, all jobs complete
    void Dequeue();

    //< Worker threads will call this method to find a job.  Must return true if
    //< work was completed.  False if no work was available.
    virtual bool FindJob() = 0;

    friend class ThreadPoolImpl;
    friend class PoolThread;
};

//< Encoder frame class must derive from this class and implement
//< ProcessRow().  Row may be partially completed and needs to be resumed.
class QueueFrame : public JobProvider
{
private:

    //< bitmap of rows queued for processing.  Must use atomic intrinsics to
    //< set and clear bits, for thread safety
    uint64_t volatile *m_queuedBitmap;

    int m_numWords;

    int m_numRows;

    //< QueueFrame's internal implementation. Consults queuedBitmap and calls
    //< ProcessRow(row) for lowest numbered queued row or returns false
    bool FindJob();

public:

    QueueFrame(ThreadPool *pool) : JobProvider(pool), m_queuedBitmap(0) {}

    //< Must be called just once after the frame is allocated.  Returns true on
    //< success.  It it returns false, the frame must encode in series.
    bool InitJobQueue(int numRows);

    virtual ~QueueFrame();

    //< Enqueue a row to be processed. A worker thread will later call ProcessRow(row)
    //< ThreadPoolEnqueue() must be called before EnqueueRow()
    void EnqueueRow(int row);

    //< Start or resume encode processing of this row.  When the bottom row is
    //< finished, ThreadPoolDequeue() must be called.
    virtual void ProcessRow(int row) = 0;
};

//< Abstract interface to ThreadPool.  Each encoder instance should call
//< AllocThreadPool() to get a handle to the singleton object and then make
//< it available to their frame structures.
class ThreadPool
{
protected:

    //< Destructor is inaccessable, force the use of reference counted Release()
    ~ThreadPool() {}

public:

    //< When numthreads == 0, a default thread count is used. A request may grow
    //< an existing pool but it will never shrink.
    static ThreadPool *AllocThreadPool(int numthreads = 0);

    //< The pool is reference counted so all calls to AllocThreadPool() should be
    //< followed by a call to Release()
    virtual void Release() = 0;

    virtual void EnqueueJobProvider(JobProvider &) = 0;

    virtual void DequeueJobProvider(JobProvider &) = 0;

    virtual void PokeIdleThreads() = 0;
};
} // end namespace x265

#endif // ifndef _THREADPOOL_H_
