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
#include <new>

#if MACOS
#include <sys/param.h>
#include <sys/sysctl.h>
#endif

#ifdef __GNUC__                         /* GCCs builtin atomics */

#include <unistd.h>
#include <limits.h>

#define CLZ64(id, x)                    id = 63 - (unsigned long)__builtin_clzll(x)
#define ATOMIC_INC(ptr)                 __sync_add_and_fetch((volatile int*)ptr, 1)
#define ATOMIC_DEC(ptr)                 __sync_add_and_fetch((volatile int*)ptr, -1)
#define ATOMIC_OR(ptr, mask)            __sync_or_and_fetch(ptr, mask)
#define ATOMIC_CAS(ptr, oldval, newval) __sync_val_compare_and_swap(ptr, oldval, newval)
#define GIVE_UP_TIME()                  usleep(0)

#elif defined(_MSC_VER)                 /* Windows atomic intrinsics */

#include <intrin.h>

#if !_WIN64
inline int _BitScanReverse64(DWORD *id, uint64_t x64)
{
    uint32_t high32 = (uint32_t)(x64 >> 32);
    uint32_t low32 = (uint32_t)x64;
    if (high32)
    {
        _BitScanReverse(id, high32);
        *id += 32;
        return 1;
    }
    else if (low32)
        return _BitScanReverse(id, low32);
    else
        return *id = 0;
}
#endif // if !_WIN64

#if _WIN32_WINNT < _WIN32_WINNT_VISTA
/* XP did not define this intrinsic */
FORCEINLINE LONGLONG _InterlockedOr64 (
    __inout LONGLONG volatile *Destination,
    __in    LONGLONG Value
    )
{
    LONGLONG Old;

    do {
        Old = *Destination;
    } while (_InterlockedCompareExchange64(Destination,
                                          Old | Value,
                                          Old) != Old);

    return Old;
}
#define ATOMIC_OR(ptr, mask)            _InterlockedOr64((volatile LONG64*)ptr, mask)
#pragma intrinsic(_InterlockedCompareExchange64)
#else
#define ATOMIC_OR(ptr, mask)            InterlockedOr64((volatile LONG64*)ptr, mask)
#endif

#define CLZ64(id, x)                    _BitScanReverse64(&id, x)
#define ATOMIC_INC(ptr)                 InterlockedIncrement((volatile LONG*)ptr)
#define ATOMIC_DEC(ptr)                 InterlockedDecrement((volatile LONG*)ptr)
#define ATOMIC_CAS(ptr, oldval, newval) (uint64_t)_InterlockedCompareExchange64((volatile LONG64*)ptr, newval, oldval)
#define GIVE_UP_TIME()                  Sleep(0)

#endif // ifdef __GNUC__

namespace x265 {
// x265 private namespace

class ThreadPoolImpl;
static int get_cpu_count();

class PoolThread : public Thread
{
private:

    ThreadPoolImpl &m_pool;

    PoolThread& operator =(const PoolThread&);

    bool           m_dirty;

    bool           m_idle;

public:

    PoolThread(ThreadPoolImpl& pool) : m_pool(pool), m_dirty(false), m_idle(false) {}

    //< query if thread is still potentially walking provider list
    bool isDirty() const { return !m_idle && m_dirty; }

    //< set m_dirty if the thread might be walking provider list
    void markDirty()     { m_dirty = !m_idle; }

    virtual ~PoolThread() {}

    void ThreadMain();

    volatile static int   s_sleepCount;
    static Event s_wakeEvent;
};

volatile int PoolThread::s_sleepCount = 0;

Event PoolThread::s_wakeEvent;

class ThreadPoolImpl : public ThreadPool
{
private:

    bool         m_ok;
    int          m_referenceCount;
    int          m_numThreads;
    PoolThread  *m_threads;

    /* Lock for write access to the provider lists.  Threads are
     * always allowed to read m_firstProvider and follow the
     * linked list.  Providers must zero their m_nextProvider
     * pointers before removing themselves from this list */
    Lock         m_writeLock;

public:

    static ThreadPoolImpl *instance;

    JobProvider *m_firstProvider;
    JobProvider *m_lastProvider;

public:

    ThreadPoolImpl(int numthreads);

    virtual ~ThreadPoolImpl();

    ThreadPoolImpl *AddReference()
    {
        m_referenceCount++;

        return this;
    }

    void Release();

    void Stop();

    bool IsValid() const
    {
        return m_ok;
    }

    void EnqueueJobProvider(JobProvider &);

    void DequeueJobProvider(JobProvider &);

    void FlushProviderList();

    void PokeIdleThreads();
};

void PoolThread::ThreadMain()
{
    while (m_pool.IsValid())
    {
        /* Walk list of job providers, looking for work */
        JobProvider *cur = m_pool.m_firstProvider;
        while (cur)
        {
            // FindJob() may perform actual work and return true.  If
            // it does we restart the job search
            if (cur->FindJob() == true)
                break;

            cur = cur->m_nextProvider;
        }

        m_dirty = false;
        if (cur == NULL)
        {
            m_idle = true;
            ATOMIC_INC(&s_sleepCount);
            s_wakeEvent.Wait();
            ATOMIC_DEC(&s_sleepCount);
            m_idle = false;
        }
    }
}

void ThreadPoolImpl::PokeIdleThreads()
{
    PoolThread::s_wakeEvent.Trigger();
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
        this->Stop();
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

    char *buffer = new char[sizeof(PoolThread) * numThreads];
    m_threads = reinterpret_cast<PoolThread*>(buffer);

    if (m_threads)
    {
        m_ok = true;
        for (int i = 0; i < numThreads; i++)
        {
            new (buffer)PoolThread(*this);
            buffer += sizeof(PoolThread);
            m_ok = m_ok && m_threads[i].Start();
        }

        // Wait for threads to spin up and idle
        while (PoolThread::s_sleepCount < m_numThreads)
        {
            GIVE_UP_TIME();
        }
    }
}

void ThreadPoolImpl::Stop()
{
    if (m_ok)
    {
        // wait for all threads to idle
        while (PoolThread::s_sleepCount < m_numThreads)
        {
            GIVE_UP_TIME();
        }

        // set invalid flag, then wake them up so they exit their main func
        m_ok = false;
        for (int i = 0; i < m_numThreads; i++)
        {
            PokeIdleThreads();
        }

        // wait for each thread to exit
        for (int i = 0; i < m_numThreads; i++)
        {
            m_threads[i].Stop();
        }
    }
}

ThreadPoolImpl::~ThreadPoolImpl()
{
    if (m_threads)
    {
        // cleanup thread handles
        for (int i = 0; i < m_numThreads; i++)
        {
            m_threads[i].~PoolThread();
        }

        delete[] reinterpret_cast<char*>(m_threads);
    }
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

/* Ensure all threads are either idle, or have made a full
 * pass through the provider list, ensuring dequeued providers
 * are safe for deletion. */
void ThreadPoolImpl::FlushProviderList()
{
    for (int i = 0; i < m_numThreads; i++)
    {
        m_threads[i].markDirty();
    }

    int i;
    do
    {
        for (i = 0; i < m_numThreads; i++)
        {
            if (m_threads[i].isDirty())
            {
                GIVE_UP_TIME();
                break;
            }
        }
    }
    while (i < m_numThreads);
}

void JobProvider::Flush()
{
    if (m_nextProvider || m_prevProvider)
        Dequeue();
    dynamic_cast<ThreadPoolImpl*>(m_pool)->FlushProviderList();
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
        m_queuedBitmap = new uint64_t[m_numWords];
        memset((void*)m_queuedBitmap, 0, sizeof(uint64_t) * m_numWords);
        return m_queuedBitmap != NULL;
    }

    return false;
}

QueueFrame::~QueueFrame()
{
    if (m_queuedBitmap)
    {
        delete[] m_queuedBitmap;
        m_queuedBitmap = NULL;
    }
}

void QueueFrame::EnqueueRow(int row)
{
    // thread safe
    uint64_t bit = 1LL << (row & 63);

    assert(row < m_numRows);
    ATOMIC_OR(&m_queuedBitmap[row >> 6], bit);
    m_pool->PokeIdleThreads();
}

bool QueueFrame::FindJob()
{
    unsigned long id;

    // thread safe
    for (int w = 0; w < m_numWords; w++)
    {
        while (m_queuedBitmap[w])
        {
            uint64_t oldval = m_queuedBitmap[w];
            if (oldval == 0) // race condition
                break;

            CLZ64(id, oldval);
            uint64_t newval = oldval & ~(1LL << id);

            if (ATOMIC_CAS(&m_queuedBitmap[w], oldval, newval) == oldval)
            {
                // if the bit was actually flipped, process row, else try again
                ProcessRow(w * 64 + id);
                return true;
            }
        }
    }

    // made it through the bitmap without finding any enqueued rows
    return false;
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

    nm[0] = CTL_HW;
    nm[1] = HW_AVAILCPU;
    sysctl(nm, 2, &count, &len, NULL, 0);

    if (count < 1)
    {
        nm[1] = HW_NCPU;
        sysctl(nm, 2, &count, &len, NULL, 0);
        if (count < 1)
            count = 1;
    }

    return count;
#else // if WIN32
    return 2; // default to 2 threads, everywhere else
#endif // if WIN32
}
} // end namespace x265

extern "C"
void x265_init_threading( int threadcount )
{
    x265::ThreadPool::AllocThreadPool(threadcount);
}

