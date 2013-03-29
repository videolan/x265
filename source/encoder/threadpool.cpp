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

#include "threadpool.h"
#include "threading.h"
#include <assert.h>
#include <string.h>

#if MACOS
#include <sys/param.h>
#include <sys/sysctl.h>
#endif

#ifdef __GNUC__                         /* GCCs builtin atomics */

#include <unistd.h>
#include <limits.h>

#define CLZ64(x)                        __builtin_clzll(x)

#define ATOMIC_AND(ptr,mask)            __sync_and_and_fetch(ptr,mask)
#define ATOMIC_OR(ptr,mask)             __sync_or_and_fetch(ptr,mask)
#define GIVE_UP_TIME()                  usleep(0);

#elif defined(_MSC_VER)                 /* Windows atomic intrinsics */

#include <intrin.h>

#if _WIN64
#define CLZ64(x)                        __lzcnt64(x)
#else
#define CLZ64(x)                        __lzcnt_2x32(x)
inline int __lzcnt_2x32(uint64_t x64)
{
    int val = __lzcnt((uint32_t)(x64 >> 32));
    if (val)
        return val + 32;
    return __lzcnt((uint32_t) x64);
}
#endif

#define ATOMIC_AND(ptr,mask)           InterlockedAnd64((volatile LONG64*)ptr,mask)
#define ATOMIC_OR(ptr,mask)            InterlockedOr64((volatile LONG64*)ptr,mask)
#define GIVE_UP_TIME()                 Sleep(0);

#endif

namespace x265
{

class ThreadPoolImpl;

class PoolThread : public Thread
{
private:

    ThreadPoolImpl *m_pool;
    Event m_awake;

public:

    volatile PoolThread *m_idleNext;

    PoolThread() : m_idleNext(NULL) {}

    ~PoolThread() {}

    void Initialize(ThreadPoolImpl *p)
    {
        m_pool = p;
        m_awake.Trigger();
    }

    void Awaken()
    {
        m_awake.Trigger();
    }

    void ThreadMain();
};

class ThreadPoolImpl : public ThreadPool
{
private:

    bool         m_ok;
    int          m_referenceCount;
    int          m_numThreads;
    PoolThread  *m_threads;

    Lock         m_writeLock;

public:

    static ThreadPoolImpl *instance;

    JobProvider *m_firstProvider;
    JobProvider *m_lastProvider;

public:

    ThreadPoolImpl(int numthreads);

    ~ThreadPoolImpl();

    ThreadPoolImpl *AddReference()
    {
        m_referenceCount++;
        return this;
    }

    void Release();

    bool IsValid() const
    {
        return m_ok;
    }

    void EnqueueJobProvider(JobProvider &);

    void DequeueJobProvider(JobProvider &);

    void PokeIdleThreads();
};


void PoolThread::ThreadMain()
{
    // Wait for pool to initialize our state
    m_awake.Wait();

    while (m_pool->IsValid())
    {
        /* Walk list of job providers, looking for work */
        JobProvider *cur = m_pool->m_firstProvider;
        while (cur)
        {
            // FindJob() may perform actual work and return true.  If
            // it does we restart the job search
            if (cur->FindJob() == true)
                break;

            cur = cur->m_nextProvider;
        }

        // Keep spinning so long as there is at least one job provider
        if (!m_pool->m_firstProvider)
            // unemployed, oh bugger
            m_awake.Wait();
        else if (cur == NULL)
            // Allow another thread to use this CPU for a bit
            GIVE_UP_TIME();
    }
}

void ThreadPoolImpl::PokeIdleThreads()
{
    for (int i = 0; i < m_numThreads; i++)
        m_threads[i].Awaken();
}

static int get_cpu_count()
{
#if WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
#elif __unix__
    return sysconf(_SC_NPROCESSORS_ONLN);
#elif MACOS
    int nm[2];
    size_t len = 4;
    uint32_t count;

    nm[0] = CTL_HW; nm[1] = HW_AVAILCPU;
    sysctl(nm, 2, &count, &len, NULL, 0);

    if (count < 1)
    {
        nm[1] = HW_NCPU;
        sysctl(nm, 2, &count, &len, NULL, 0);
        if (count < 1)
            count = 1;
    }
    return count;
#else
    return 2; // default to 2 threads, everywhere else
#endif
}

ThreadPoolImpl *ThreadPoolImpl::instance;

/* static */
ThreadPool *ThreadPool::AllocThreadPool(int numthreads)
{
    if (ThreadPoolImpl::instance)
        return ThreadPoolImpl::instance->AddReference();
    ThreadPoolImpl::instance = new ThreadPoolImpl(numthreads);
    return ThreadPoolImpl::instance;
}

void ThreadPoolImpl::Release()
{
    if (--m_referenceCount == 0)
    {
        assert(this == ThreadPoolImpl::instance);
        ThreadPoolImpl::instance = NULL;
        delete this;
    }
}

ThreadPoolImpl::ThreadPoolImpl(int numThreads)
    : m_ok(false)
    , m_referenceCount(1)
    , m_numThreads(numThreads)
    , m_firstProvider(NULL)
    , m_lastProvider(NULL)
{
    if (numThreads == 0)
        numThreads = get_cpu_count();

    m_threads = new PoolThread[ numThreads ];

    if (m_threads)
    {
        m_ok = true;
        for (int i = 0; i < numThreads; i++)
            m_ok &= m_threads[i].Start();
    }
    if (m_ok)
    {
        for (int i = 0; i < numThreads; i++)
            m_threads[i].Initialize(this);
    }
}

ThreadPoolImpl::~ThreadPoolImpl()
{
    if (m_ok && m_threads)
    {
        m_ok = false;
        for (int i = 0; i < m_numThreads; i++)
            m_threads[i].Awaken();
        // destructors will block for thread completions
        delete [] m_threads;
        m_threads = NULL;
    }
    // leak threads on program exit if there were resource failures
}

void ThreadPoolImpl::EnqueueJobProvider(JobProvider &p)
{
    // only one list writer at a time
    ScopedLock l(m_writeLock);

    p.m_nextProvider = NULL;
    p.m_prevProvider = m_lastProvider;
    m_lastProvider = &p;

    if (p.m_prevProvider)
        p.m_prevProvider->m_nextProvider = &p;
    else
        m_firstProvider = &p;
}

void ThreadPoolImpl::DequeueJobProvider(JobProvider &p)
{
    // only one list writer at a time
    ScopedLock l(m_writeLock);

    // update pool entry pointers first
    if (m_firstProvider == &p)
        m_firstProvider = p.m_nextProvider;
    if (m_lastProvider == &p)
        m_lastProvider = p.m_prevProvider;

    // extract self from doubly linked lists
    if (p.m_nextProvider)
        p.m_nextProvider->m_prevProvider = p.m_prevProvider;
    if (p.m_prevProvider)
        p.m_prevProvider->m_nextProvider = p.m_nextProvider;

    p.m_nextProvider = NULL;
    p.m_prevProvider = NULL;
}


JobProvider::~JobProvider()
{
    if (m_nextProvider || m_prevProvider)
        Dequeue();
}

void JobProvider::Enqueue()
{
    // Add this provider to the end of the thread pool's job provider list
    assert(!m_nextProvider && !m_prevProvider && m_pool);
    m_pool->EnqueueJobProvider(*this);
    m_pool->PokeIdleThreads();
}

void JobProvider::Dequeue()
{
    // Remove this provider from the thread pool's job provider list
    m_pool->DequeueJobProvider(*this);
}


bool QueueFrame::InitJobQueue(int numRows)
{
    m_numRows = numRows;

    if (m_pool)
    {
        m_numWords = (numRows + 63) >> 6;
        m_queuedBitmap = new uint64_t[ m_numWords ];
        memset((void *)m_queuedBitmap, 0, sizeof(uint64_t) * m_numWords);
        return m_queuedBitmap != NULL;
    }

    return false;
}

QueueFrame::~QueueFrame()
{
    if (m_queuedBitmap)
    {
        delete [] m_queuedBitmap;
        m_queuedBitmap = NULL;
    }
}

void QueueFrame::EnqueueRow(int row)
{
    // thread safe
    uint64_t bit = 1LL << (row & 63);
    assert(row < m_numRows);
    ATOMIC_OR(&m_queuedBitmap[row >> 6], bit);
}

bool QueueFrame::FindJob()
{
    // thread safe
    for (int w = 0; w < m_numWords; w++)
    {
        while (m_queuedBitmap[w])
        {
            uint64_t word = m_queuedBitmap[w];
            if (word == 0) // race condition
                break;
            int id = 63 - (int) CLZ64(word);
            uint64_t bit = 1LL << id;
            uint64_t mask = ~bit;

            if (ATOMIC_AND(&m_queuedBitmap[w], mask) & bit)
            {
                // if the bit was actually flipped. process row, else try again
                ProcessRow(w * 64 + id);
                return true;
            }
        }
    }

    // made it through the bitmap without finding any enqueued rows
    return false;
}


} // end namespace x265
