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

#ifdef __GNUC__

#include <limits.h>
#define CLZ64(x) __builtin_clzll(x)
#define ATOMIC_AND(ptr,mask) __sync_and_and_fetch(ptr,mask)
#define ATOMIC_OR(ptr,mask)  __sync_or_and_fetch(ptr,mask)

#elif defined(_MSC_VER)

#include <intrin.h>
#if _WIN64
#define CLZ64(x) __lzcnt64(x)
#else
#define CLZ64(x) __lzcnt_2x32(x)
inline int __lzcnt_2x32(uint64_t x64)
{
    int val = __lzcnt((uint32_t)(x64 >> 32));
    if (val)
        return val + 32;
    return __lzcnt((uint32_t) x64);
}
#endif
#define ATOMIC_AND(ptr,mask) InterlockedAnd(ptr,mask)
#define ATOMIC_OR(ptr,mask)  InterlockedOr(ptr,mask)

#endif

namespace x265
{

class PoolThread : public Thread
{
public:
     PoolThread() {}
    ~PoolThread() {}

     void ThreadMain();
};

class ThreadPoolImpl : public ThreadPool
{
private:

    int          m_referenceCount;
    int          m_numThreads;

    PoolThread  *m_threads;

    Lock         m_writeLock;
    JobProvider *m_firstProvider;
    JobProvider *m_lastProvider;

public:

    ThreadPoolImpl( int numThreads )
        : m_referenceCount(1)
        , m_firstProvider(NULL)
        , m_lastProvider(NULL)
    {
        m_threads = new PoolThread[ numThreads ];

        if (m_threads)
        {
            for (int i = 0; i < numThreads; i++)
                m_threads[i].Start();
        }
    }

    ~ThreadPoolImpl()
    {
        if (m_threads)
        {
            delete [] m_threads;
            m_threads = NULL;
        }
    }

    void Release() { if (--m_referenceCount == 0) delete this; }

    void EnqueueJobProvider(JobProvider&);

    void DequeueJobProvider(JobProvider&);

    void PokeIdleThread();

    friend class ThreadPool;
};

/* static */
ThreadPool *ThreadPool::AllocThreadPool( int numthreads )
{
    static ThreadPoolImpl *_impl;
    if (_impl)
    {
        _impl->m_referenceCount++;
        return _impl;
    }
    _impl = new ThreadPoolImpl( numthreads );
    return _impl;
}

void ThreadPoolImpl::EnqueueJobProvider(JobProvider& p)
{ // only one list writer at a time
    ScopedLock l(m_writeLock);
       
    p.m_nextProvider = NULL;
    p.m_prevProvider = m_lastProvider;
    m_lastProvider = &p;

    if (p.m_prevProvider)
        p.m_prevProvider->m_nextProvider = &p;
    else
        m_firstProvider = &p;
}

void ThreadPoolImpl::DequeueJobProvider(JobProvider& p)
{ // only one list writer at a time
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

void ThreadPoolImpl::PokeIdleThread()
{
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
    m_pool->EnqueueJobProvider( *this );
    m_pool->PokeIdleThread(); // Come and get it!
}

void JobProvider::Dequeue()
{
    // Remove this provider from the thread pool's job provider list
    m_pool->DequeueJobProvider( *this );
    m_pool->PokeIdleThread(); // in case a workder thread's list walk was broken
}

void JobProvider::NewJobAvailable()
{
    m_pool->PokeIdleThread();
}



bool QueueFrame::InitJobQueue( int numRows )
{
    m_numRows = numRows;

    if (m_pool)
    {
        m_queuedBitmap = new uint64_t[ (numRows + 63) >> 6 ];
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

void QueueFrame::EnqueueRow( int row )
{ // thread safe
    uint64_t bit = 1LL << (row&63);
    assert(row < m_numRows);
    ATOMIC_OR(&m_queuedBitmap[row>>6], bit);
    NewJobAvailable();
}

bool QueueFrame::FindJob()
{ // thread safe
    for (int w = 0; w < ((m_numRows+63)>>6); w++)
    {
        while (m_queuedBitmap[w])
        {
            uint64_t word = m_queuedBitmap[w];
            if (word == 0) // race condition
                break;
            int bit = 64 - (int) CLZ64(word);
            uint64_t mask = ~(1LL << bit);

            if (ATOMIC_AND(&m_queuedBitmap[w], mask) & (1LL << bit))
            { // if the bit was actually flipped. process row, else try again
                ProcessRow( w * 32 + bit );
                return true;
            }
        }
    }

    // made it through the bitmap without finding any enqueued rows
    return false;
}


} // end namespace x265
