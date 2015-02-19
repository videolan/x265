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
 * For more information, contact us at license @ x265.com
 *****************************************************************************/

#ifndef X265_THREADPOOL_H
#define X265_THREADPOOL_H

#include "common.h"
#include "threading.h"

namespace x265 {
// x265 private namespace

class ThreadPool;
class WorkerThread;
class BondedTaskGroup;

#if X86_64
typedef uint64_t sleepbitmap_t;
#else
typedef uint32_t sleepbitmap_t;
#endif

static const sleepbitmap_t ALL_POOL_THREADS = (sleepbitmap_t)-1;
enum { MAX_POOL_THREADS = sizeof(sleepbitmap_t) * 8 };
enum { INVALID_SLICE_PRIORITY = 10 }; // a value larger than any X265_TYPE_* macro

// Frame level job providers. FrameEncoder and Lookahead derive from
// this class and implement findJob()
class JobProvider
{
public:

    ThreadPool*   m_pool;
    sleepbitmap_t m_ownerBitmap;
    int           m_jpId;
    int           m_sliceType;
    bool          m_helpWanted;
    bool          m_isFrameEncoder; /* rather ugly hack, but nothing better presents itself */

    JobProvider()
        : m_pool(NULL)
        , m_ownerBitmap(0)
        , m_jpId(-1)
        , m_sliceType(INVALID_SLICE_PRIORITY)
        , m_helpWanted(false)
        , m_isFrameEncoder(false)
    {}

    virtual ~JobProvider() {}

    // Worker threads will call this method to perform work
    virtual void findJob(int workerThreadId) = 0;

    // Will awaken one idle thread, preferring a thread which most recently
    // performed work for this provider.
    void tryWakeOne();
};

class ThreadPool
{
public:

    sleepbitmap_t m_sleepBitmap;
    int           m_numProviders;
    int           m_numWorkers;
    int           m_numaNode;
    bool          m_isActive;

    JobProvider** m_jpTable;
    WorkerThread* m_workers;

    ThreadPool();
    ~ThreadPool();

    bool create(int numThreads, int maxProviders, int node);
    bool start();
    void setCurrentThreadAffinity();
    int  tryAcquireSleepingThread(sleepbitmap_t firstTryBitmap, sleepbitmap_t secondTryBitmap);
    int  tryBondPeers(int maxPeers, sleepbitmap_t peerBitmap, BondedTaskGroup& master);

    static ThreadPool* allocThreadPools(x265_param* p, int& numPools);

    static int  getCpuCount();
    static int  getNumaNodeCount();
    static void setThreadNodeAffinity(int node);
};
} // end namespace x265

#endif // ifndef X265_THREADPOOL_H
