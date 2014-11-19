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

#include "common.h"
#include "threadpool.h"
#include "threading.h"

#include <new>

#if MACOS
#include <sys/param.h>
#include <sys/sysctl.h>
#endif

namespace x265 {
// x265 private namespace

class ThreadPoolImpl;

class PoolThread : public Thread
{
private:

    ThreadPoolImpl &m_pool;

    PoolThread& operator =(const PoolThread&);

    int            m_id;

    bool           m_dirty;

    bool           m_exited;

    Event          m_wakeEvent;

public:

    PoolThread(ThreadPoolImpl& pool, int id)
        : m_pool(pool)
        , m_id(id)
        , m_dirty(false)
        , m_exited(false)
    {
    }

    bool isDirty() const  { return m_dirty; }

    void markDirty()      { m_dirty = true; }

    bool isExited() const { return m_exited; }

    void poke()           { m_wakeEvent.trigger(); }

    virtual ~PoolThread() {}

    void threadMain();
};

class ThreadPoolImpl : public ThreadPool
{
private:

    bool         m_ok;
    int          m_referenceCount;
    int          m_numThreads;
    int          m_numSleepMapWords;
    PoolThread  *m_threads;
    volatile uint32_t *m_sleepMap;

    /* Lock for write access to the provider lists.  Threads are
     * always allowed to read m_firstProvider and follow the
     * linked list.  Providers must zero their m_nextProvider
     * pointers before removing themselves from this list */
    Lock         m_writeLock;

public:

    static ThreadPoolImpl *s_instance;
    static Lock s_createLock;

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

    void markThreadAsleep(int id);

    void waitForAllIdle();

    int getThreadCount() const { return m_numThreads; }

    bool IsValid() const       { return m_ok; }

    void release();

    void Stop();

    void enqueueJobProvider(JobProvider &);

    void dequeueJobProvider(JobProvider &);

    void FlushProviderList();

    void pokeIdleThread();
};

void PoolThread::threadMain()
{
#if _WIN32
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
#else
    __attribute__((unused)) int val = nice(10);
#endif

    while (m_pool.IsValid())
    {
        /* Walk list of job providers, looking for work */
        JobProvider *cur = m_pool.m_firstProvider;
        while (cur)
        {
            // FindJob() may perform actual work and return true.  If
            // it does we restart the job search
            if (cur->findJob(m_id) == true)
                break;

            cur = cur->m_nextProvider;
        }

        // this thread has reached the end of the provider list
        m_dirty = false;

        if (cur == NULL)
        {
            m_pool.markThreadAsleep(m_id);
            m_wakeEvent.wait();
        }
    }

    m_exited = true;
}

void ThreadPoolImpl::markThreadAsleep(int id)
{
    int word = id >> 5;
    uint32_t bit = 1 << (id & 31);

    ATOMIC_OR(&m_sleepMap[word], bit);
}

void ThreadPoolImpl::pokeIdleThread()
{
    /* Find a bit in the sleeping thread bitmap and poke it awake, do
     * not give up until a thread is awakened or all of them are awake */
    for (int i = 0; i < m_numSleepMapWords; i++)
    {
        uint32_t oldval = m_sleepMap[i];
        while (oldval)
        {
            unsigned long id;
            CTZ(id, oldval);

            uint32_t bit = 1 << id;
            if (ATOMIC_AND(&m_sleepMap[i], ~bit) & bit)
            {
                m_threads[i * 32 + id].poke();
                return;
            }

            oldval = m_sleepMap[i];
        }
    }
}

ThreadPoolImpl *ThreadPoolImpl::s_instance;
Lock ThreadPoolImpl::s_createLock;

/* static */
ThreadPool *ThreadPool::allocThreadPool(int numthreads)
{
    if (ThreadPoolImpl::s_instance)
        return ThreadPoolImpl::s_instance->AddReference();

    /* acquire the lock to create the instance */
    ThreadPoolImpl::s_createLock.acquire();

    if (ThreadPoolImpl::s_instance)
        /* pool was allocated while we waited for the lock */
        ThreadPoolImpl::s_instance->AddReference();
    else
        ThreadPoolImpl::s_instance = new ThreadPoolImpl(numthreads);
    ThreadPoolImpl::s_createLock.release();

    return ThreadPoolImpl::s_instance;
}

ThreadPool *ThreadPool::getThreadPool()
{
    X265_CHECK(ThreadPoolImpl::s_instance, "getThreadPool() called prior to allocThreadPool()\n");
    return ThreadPoolImpl::s_instance;
}

void ThreadPoolImpl::release()
{
    if (--m_referenceCount == 0)
    {
        X265_CHECK(this == ThreadPoolImpl::s_instance, "multiple thread pool instances detected\n");
        ThreadPoolImpl::s_instance = NULL;
        this->Stop();
        delete this;
    }
}

ThreadPoolImpl::ThreadPoolImpl(int numThreads)
    : m_ok(false)
    , m_referenceCount(1)
    , m_firstProvider(NULL)
    , m_lastProvider(NULL)
{
    m_numSleepMapWords = (numThreads + 31) >> 5;
    m_sleepMap = X265_MALLOC(uint32_t, m_numSleepMapWords);

    char *buffer = (char*)X265_MALLOC(PoolThread, numThreads);
    m_threads = reinterpret_cast<PoolThread*>(buffer);
    m_numThreads = numThreads;

    if (m_threads && m_sleepMap)
    {
        for (int i = 0; i < m_numSleepMapWords; i++)
            m_sleepMap[i] = 0;

        m_ok = true;
        int i;
        for (i = 0; i < numThreads; i++)
        {
            new (buffer)PoolThread(*this, i);
            buffer += sizeof(PoolThread);
            if (!m_threads[i].start())
            {
                m_ok = false;
                break;
            }
        }

        if (m_ok)
            waitForAllIdle();
        else
        {
            // stop threads that did start up
            for (int j = 0; j < i; j++)
            {
                m_threads[j].poke();
                m_threads[j].stop();
            }
        }
    }
}

void ThreadPoolImpl::waitForAllIdle()
{
    if (!m_ok)
        return;

    int id = 0;
    do
    {
        int word = id >> 5;
        uint32_t bit = 1 << (id & 31);
        if (m_sleepMap[word] & bit)
            id++;
        else
        {
            GIVE_UP_TIME();
        }
    }
    while (id < m_numThreads);
}

void ThreadPoolImpl::Stop()
{
    if (m_ok)
    {
        waitForAllIdle();

        // set invalid flag, then wake them up so they exit their main func
        m_ok = false;
        for (int i = 0; i < m_numThreads; i++)
        {
            m_threads[i].poke();
            m_threads[i].stop();
        }
    }
}

ThreadPoolImpl::~ThreadPoolImpl()
{
    X265_FREE((void*)m_sleepMap);

    if (m_threads)
    {
        // cleanup thread handles
        for (int i = 0; i < m_numThreads; i++)
            m_threads[i].~PoolThread();

        X265_FREE(reinterpret_cast<char*>(m_threads));
    }
}

void ThreadPoolImpl::enqueueJobProvider(JobProvider &p)
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

void ThreadPoolImpl::dequeueJobProvider(JobProvider &p)
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

/* Ensure all threads have made a full pass through the provider list, ensuring
 * dequeued providers are safe for deletion. */
void ThreadPoolImpl::FlushProviderList()
{
    for (int i = 0; i < m_numThreads; i++)
    {
        m_threads[i].markDirty();
        m_threads[i].poke();
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

void JobProvider::flush()
{
    if (m_nextProvider || m_prevProvider)
        dequeue();
    dynamic_cast<ThreadPoolImpl*>(m_pool)->FlushProviderList();
}

void JobProvider::enqueue()
{
    // Add this provider to the end of the thread pool's job provider list
    X265_CHECK(!m_nextProvider && !m_prevProvider && m_pool, "job provider was already queued\n");
    m_pool->enqueueJobProvider(*this);
    m_pool->pokeIdleThread();
}

void JobProvider::dequeue()
{
    // Remove this provider from the thread pool's job provider list
    m_pool->dequeueJobProvider(*this);
    // Ensure no jobs were missed while the provider was being removed
    m_pool->pokeIdleThread();
}

int getCpuCount()
{
#if _WIN32
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
#else // if _WIN32
    return 2; // default to 2 threads, everywhere else
#endif // if _WIN32
}
} // end namespace x265
