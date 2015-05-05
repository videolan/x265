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

#if X86_64

#ifdef __GNUC__

#define SLEEPBITMAP_CTZ(id, x)     id = (unsigned long)__builtin_ctzll(x)
#define SLEEPBITMAP_OR(ptr, mask)  __sync_fetch_and_or(ptr, mask)
#define SLEEPBITMAP_AND(ptr, mask) __sync_fetch_and_and(ptr, mask)

#elif defined(_MSC_VER)

#define SLEEPBITMAP_CTZ(id, x)     _BitScanForward64(&id, x)
#define SLEEPBITMAP_OR(ptr, mask)  InterlockedOr64((volatile LONG64*)ptr, (LONG)mask)
#define SLEEPBITMAP_AND(ptr, mask) InterlockedAnd64((volatile LONG64*)ptr, (LONG)mask)

#endif // ifdef __GNUC__

#else

/* use 32-bit primitives defined in threading.h */
#define SLEEPBITMAP_CTZ CTZ
#define SLEEPBITMAP_OR  ATOMIC_OR
#define SLEEPBITMAP_AND ATOMIC_AND

#endif

#if MACOS
#include <sys/param.h>
#include <sys/sysctl.h>
#endif
#if HAVE_LIBNUMA
#include <numa.h>
#endif

namespace x265 {
// x265 private namespace

class WorkerThread : public Thread
{
private:

    ThreadPool&  m_pool;
    int          m_id;
    Event        m_wakeEvent;

    WorkerThread& operator =(const WorkerThread&);

public:

    JobProvider*     m_curJobProvider;
    BondedTaskGroup* m_bondMaster;

    WorkerThread(ThreadPool& pool, int id) : m_pool(pool), m_id(id) {}
    virtual ~WorkerThread() {}

    void threadMain();
    void awaken()           { m_wakeEvent.trigger(); }
};

void WorkerThread::threadMain()
{
    THREAD_NAME("Worker", m_id);

#if _WIN32
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
#else
    __attribute__((unused)) int val = nice(10);
#endif

    m_pool.setCurrentThreadAffinity();

    sleepbitmap_t idBit = (sleepbitmap_t)1 << m_id;
    m_curJobProvider = m_pool.m_jpTable[0];
    m_bondMaster = NULL;

    SLEEPBITMAP_OR(&m_curJobProvider->m_ownerBitmap, idBit);
    SLEEPBITMAP_OR(&m_pool.m_sleepBitmap, idBit);
    m_wakeEvent.wait();

    while (m_pool.m_isActive)
    {
        if (m_bondMaster)
        {
            m_bondMaster->processTasks(m_id);
            m_bondMaster->m_exitedPeerCount.incr();
            m_bondMaster = NULL;
        }

        do
        {
            /* do pending work for current job provider */
            m_curJobProvider->findJob(m_id);

            /* if the current job provider still wants help, only switch to a
             * higher priority provider (lower slice type). Else take the first
             * available job provider with the highest priority */
            int curPriority = (m_curJobProvider->m_helpWanted) ? m_curJobProvider->m_sliceType :
                                                                 INVALID_SLICE_PRIORITY + 1;
            int nextProvider = -1;
            for (int i = 0; i < m_pool.m_numProviders; i++)
            {
                if (m_pool.m_jpTable[i]->m_helpWanted &&
                    m_pool.m_jpTable[i]->m_sliceType < curPriority)
                {
                    nextProvider = i;
                    curPriority = m_pool.m_jpTable[i]->m_sliceType;
                }
            }
            if (nextProvider != -1 && m_curJobProvider != m_pool.m_jpTable[nextProvider])
            {
                SLEEPBITMAP_AND(&m_curJobProvider->m_ownerBitmap, ~idBit);
                m_curJobProvider = m_pool.m_jpTable[nextProvider];
                SLEEPBITMAP_OR(&m_curJobProvider->m_ownerBitmap, idBit);
            }
        }
        while (m_curJobProvider->m_helpWanted);

        /* While the worker sleeps, a job-provider or bond-group may acquire this
         * worker's sleep bitmap bit. Once acquired, that thread may modify 
         * m_bondMaster or m_curJobProvider, then waken the thread */
        SLEEPBITMAP_OR(&m_pool.m_sleepBitmap, idBit);
        m_wakeEvent.wait();
    }

    SLEEPBITMAP_OR(&m_pool.m_sleepBitmap, idBit);
}

void JobProvider::tryWakeOne()
{
    int id = m_pool->tryAcquireSleepingThread(m_ownerBitmap, ALL_POOL_THREADS);
    if (id < 0)
    {
        m_helpWanted = true;
        return;
    }

    WorkerThread& worker = m_pool->m_workers[id];
    if (worker.m_curJobProvider != this) /* poaching */
    {
        sleepbitmap_t bit = (sleepbitmap_t)1 << id;
        SLEEPBITMAP_AND(&worker.m_curJobProvider->m_ownerBitmap, ~bit);
        worker.m_curJobProvider = this;
        SLEEPBITMAP_OR(&worker.m_curJobProvider->m_ownerBitmap, bit);
    }
    worker.awaken();
}

int ThreadPool::tryAcquireSleepingThread(sleepbitmap_t firstTryBitmap, sleepbitmap_t secondTryBitmap)
{
    unsigned long id;

    sleepbitmap_t masked = m_sleepBitmap & firstTryBitmap;
    while (masked)
    {
        SLEEPBITMAP_CTZ(id, masked);

        sleepbitmap_t bit = (sleepbitmap_t)1 << id;
        if (SLEEPBITMAP_AND(&m_sleepBitmap, ~bit) & bit)
            return (int)id;

        masked = m_sleepBitmap & firstTryBitmap;
    }

    masked = m_sleepBitmap & secondTryBitmap;
    while (masked)
    {
        SLEEPBITMAP_CTZ(id, masked);

        sleepbitmap_t bit = (sleepbitmap_t)1 << id;
        if (SLEEPBITMAP_AND(&m_sleepBitmap, ~bit) & bit)
            return (int)id;

        masked = m_sleepBitmap & secondTryBitmap;
    }

    return -1;
}

int ThreadPool::tryBondPeers(int maxPeers, sleepbitmap_t peerBitmap, BondedTaskGroup& master)
{
    int bondCount = 0;
    do
    {
        int id = tryAcquireSleepingThread(peerBitmap, 0);
        if (id < 0)
            return bondCount;

        m_workers[id].m_bondMaster = &master;
        m_workers[id].awaken();
        bondCount++;
    }
    while (bondCount < maxPeers);

    return bondCount;
}

ThreadPool* ThreadPool::allocThreadPools(x265_param* p, int& numPools)
{
    enum { MAX_NODE_NUM = 127 };
    int cpusPerNode[MAX_NODE_NUM + 1];

    memset(cpusPerNode, 0, sizeof(cpusPerNode));
    int numNumaNodes = X265_MIN(getNumaNodeCount(), MAX_NODE_NUM);
    int cpuCount = getCpuCount();
    bool bNumaSupport = false;

#if defined(_WIN32_WINNT) && _WIN32_WINNT >= _WIN32_WINNT_WIN7 
    bNumaSupport = true;
#elif HAVE_LIBNUMA
    bNumaSupport = numa_available() >= 0;
#endif


    for (int i = 0; i < cpuCount; i++)
    {
#if defined(_WIN32_WINNT) && _WIN32_WINNT >= _WIN32_WINNT_WIN7 
        UCHAR node;
        if (GetNumaProcessorNode((UCHAR)i, &node))
            cpusPerNode[X265_MIN(node, (UCHAR)MAX_NODE_NUM)]++;
        else
#elif HAVE_LIBNUMA
        if (bNumaSupport >= 0)
            cpusPerNode[X265_MIN(numa_node_of_cpu(i), MAX_NODE_NUM)]++;
        else
#endif
            cpusPerNode[0]++;
    }

    if (bNumaSupport && p->logLevel >= X265_LOG_DEBUG)
        for (int i = 0; i < numNumaNodes; i++)
            x265_log(p, X265_LOG_DEBUG, "detected NUMA node %d with %d logical cores\n", i, cpusPerNode[i]);

    /* limit nodes based on param->numaPools */
    if (p->numaPools && *p->numaPools)
    {
        const char *nodeStr = p->numaPools;
        for (int i = 0; i < numNumaNodes; i++)
        {
            if (!*nodeStr)
            {
                cpusPerNode[i] = 0;
                continue;
            }
            else if (*nodeStr == '-')
                cpusPerNode[i] = 0;
            else if (*nodeStr == '*')
                break;
            else if (*nodeStr == '+')
                ;
            else
            {
                int count = atoi(nodeStr);
                cpusPerNode[i] = X265_MIN(count, cpusPerNode[i]);
            }

            /* consume current node string, comma, and white-space */
            while (*nodeStr && *nodeStr != ',')
               ++nodeStr;
            if (*nodeStr == ',' || *nodeStr == ' ')
               ++nodeStr;
        }
    }

    numPools = 0;
    for (int i = 0; i < numNumaNodes; i++)
    {
        if (bNumaSupport)
            x265_log(p, X265_LOG_DEBUG, "NUMA node %d may use %d logical cores\n", i, cpusPerNode[i]);
        if (cpusPerNode[i])
            numPools += (cpusPerNode[i] + MAX_POOL_THREADS - 1) / MAX_POOL_THREADS;
    }

    if (!numPools)
        return NULL;

    if (numPools > p->frameNumThreads)
    {
        x265_log(p, X265_LOG_DEBUG, "Reducing number of thread pools for frame thread count\n");
        numPools = X265_MAX(p->frameNumThreads / 2, 1);
    }

    ThreadPool *pools = new ThreadPool[numPools];
    if (pools)
    {
        int maxProviders = (p->frameNumThreads + 1 + numPools - 1) / numPools; /* +1 is Lookahead */
        int node = 0;
        for (int i = 0; i < numPools; i++)
        {
            while (!cpusPerNode[node])
                node++;
            int cores = X265_MIN(MAX_POOL_THREADS, cpusPerNode[node]);
            if (!pools[i].create(cores, maxProviders, node))
            {
                X265_FREE(pools);
                numPools = 0;
                return NULL;
            }
            if (numNumaNodes > 1)
                x265_log(p, X265_LOG_INFO, "Thread pool %d using %d threads on NUMA node %d\n", i, cores, node);
            else
                x265_log(p, X265_LOG_INFO, "Thread pool created using %d threads\n", cores);
            cpusPerNode[node] -= cores;
        }
    }
    else
        numPools = 0;
    return pools;
}

ThreadPool::ThreadPool()
{
    memset(this, 0, sizeof(*this));
}

bool ThreadPool::create(int numThreads, int maxProviders, int node)
{
    X265_CHECK(numThreads <= MAX_POOL_THREADS, "a single thread pool cannot have more than MAX_POOL_THREADS threads\n");

    m_numaNode = node;
    m_numWorkers = numThreads;

    m_workers = X265_MALLOC(WorkerThread, numThreads);
    /* placement new initialization */
    if (m_workers)
        for (int i = 0; i < numThreads; i++)
            new (m_workers + i)WorkerThread(*this, i);

    m_jpTable = X265_MALLOC(JobProvider*, maxProviders);
    m_numProviders = 0;

    return m_workers && m_jpTable;
}

bool ThreadPool::start()
{
    m_isActive = true;
    for (int i = 0; i < m_numWorkers; i++)
    {
        if (!m_workers[i].start())
        {
            m_isActive = false;
            return false;
        }
    }
    return true;
}

void ThreadPool::stopWorkers()
{
    if (m_workers)
    {
        m_isActive = false;
        for (int i = 0; i < m_numWorkers; i++)
        {
            while (!(m_sleepBitmap & ((sleepbitmap_t)1 << i)))
                GIVE_UP_TIME();
            m_workers[i].awaken();
            m_workers[i].stop();
        }
    }
}

ThreadPool::~ThreadPool()
{
    if (m_workers)
    {
        for (int i = 0; i < m_numWorkers; i++)
            m_workers[i].~WorkerThread();
    }

    X265_FREE(m_workers);
    X265_FREE(m_jpTable);
}

void ThreadPool::setCurrentThreadAffinity()
{
    setThreadNodeAffinity(m_numaNode);
}

/* static */
void ThreadPool::setThreadNodeAffinity(int numaNode)
{
#if defined(_WIN32_WINNT) && _WIN32_WINNT >= _WIN32_WINNT_WIN7 
    GROUP_AFFINITY groupAffinity;
    if (GetNumaNodeProcessorMaskEx((USHORT)numaNode, &groupAffinity))
    {
        if (SetThreadAffinityMask(GetCurrentThread(), (DWORD_PTR)groupAffinity.Mask))
            return;
    }
    x265_log(NULL, X265_LOG_ERROR, "unable to set thread affinity to NUMA node %d\n", numaNode);
#elif HAVE_LIBNUMA
    if (numa_available() >= 0)
    {
        numa_run_on_node(numaNode);
        numa_set_preferred(numaNode);
        numa_set_localalloc();
        return;
    }
    x265_log(NULL, X265_LOG_ERROR, "unable to set thread affinity to NUMA node %d\n", numaNode);
#else
    (void)numaNode;
#endif
}

/* static */
int ThreadPool::getNumaNodeCount()
{
#if defined(_WIN32_WINNT) && _WIN32_WINNT >= _WIN32_WINNT_WIN7 
    ULONG num = 1;
    if (GetNumaHighestNodeNumber(&num))
        num++;
    return (int)num;
#elif HAVE_LIBNUMA
    if (numa_available() >= 0)
        return numa_max_node() + 1;
    else
        return 1;
#else
    return 1;
#endif
}

/* static */
int ThreadPool::getCpuCount()
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
#else
    return 2; // default to 2 threads, everywhere else
#endif
}

} // end namespace x265
