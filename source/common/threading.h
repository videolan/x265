/*****************************************************************************
 * Copyright (C) 2013-2020 MulticoreWare, Inc
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Min Chen <chenm003@163.com>
            liwei <liwei@multicorewareinc.com>
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

#ifndef X265_THREADING_H
#define X265_THREADING_H

#include "common.h"
#include "x265.h"

#ifdef _WIN32
#include <windows.h>
#include "winxp.h"  // XP workarounds for CONDITION_VARIABLE and ATOMIC_OR
#else
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <fcntl.h>
#endif

#if MACOS
#include <sys/param.h>
#include <sys/sysctl.h>
#endif

#if NO_ATOMICS

#include <sys/time.h>
#include <unistd.h>

namespace X265_NS {
// x265 private namespace
int no_atomic_or(int* ptr, int mask);
int no_atomic_and(int* ptr, int mask);
int no_atomic_inc(int* ptr);
int no_atomic_dec(int* ptr);
int no_atomic_add(int* ptr, int val);
}

#define CLZ(id, x)            id = (unsigned long)__builtin_clz(x) ^ 31
#define CTZ(id, x)            id = (unsigned long)__builtin_ctz(x)
#define ATOMIC_OR(ptr, mask)  no_atomic_or((int*)ptr, mask)
#define ATOMIC_AND(ptr, mask) no_atomic_and((int*)ptr, mask)
#define ATOMIC_INC(ptr)       no_atomic_inc((int*)ptr)
#define ATOMIC_DEC(ptr)       no_atomic_dec((int*)ptr)
#define ATOMIC_ADD(ptr, val)  no_atomic_add((int*)ptr, val)
#define GIVE_UP_TIME()        usleep(0)

#elif __GNUC__               /* GCCs builtin atomics */

#include <sys/time.h>
#include <unistd.h>

#define CLZ(id, x)            id = (unsigned long)__builtin_clz(x) ^ 31
#define CTZ(id, x)            id = (unsigned long)__builtin_ctz(x)
#define ATOMIC_OR(ptr, mask)  __sync_fetch_and_or(ptr, mask)
#define ATOMIC_AND(ptr, mask) __sync_fetch_and_and(ptr, mask)
#define ATOMIC_INC(ptr)       __sync_add_and_fetch((volatile int32_t*)ptr, 1)
#define ATOMIC_DEC(ptr)       __sync_add_and_fetch((volatile int32_t*)ptr, -1)
#define ATOMIC_ADD(ptr, val)  __sync_fetch_and_add((volatile int32_t*)ptr, val)
#define GIVE_UP_TIME()        usleep(0)

#elif defined(_MSC_VER)       /* Windows atomic intrinsics */

#include <intrin.h>

#define CLZ(id, x)            _BitScanReverse(&id, x)
#define CTZ(id, x)            _BitScanForward(&id, x)
#define ATOMIC_INC(ptr)       InterlockedIncrement((volatile LONG*)ptr)
#define ATOMIC_DEC(ptr)       InterlockedDecrement((volatile LONG*)ptr)
#define ATOMIC_ADD(ptr, val)  InterlockedExchangeAdd((volatile LONG*)ptr, val)
#define ATOMIC_OR(ptr, mask)  _InterlockedOr((volatile LONG*)ptr, (LONG)mask)
#define ATOMIC_AND(ptr, mask) _InterlockedAnd((volatile LONG*)ptr, (LONG)mask)
#define GIVE_UP_TIME()        Sleep(0)

#endif // ifdef __GNUC__

namespace X265_NS {
// x265 private namespace

#ifdef _WIN32

typedef HANDLE ThreadHandle;

class Lock
{
public:

    Lock()
    {
        InitializeCriticalSection(&this->handle);
    }

    ~Lock()
    {
        DeleteCriticalSection(&this->handle);
    }

    void acquire()
    {
        EnterCriticalSection(&this->handle);
    }

    void release()
    {
        LeaveCriticalSection(&this->handle);
    }

protected:

    CRITICAL_SECTION handle;
};

class Event
{
public:

    Event()
    {
        this->handle = CreateEvent(NULL, FALSE, FALSE, NULL);
    }

    ~Event()
    {
        CloseHandle(this->handle);
    }

    void wait()
    {
        WaitForSingleObject(this->handle, INFINITE);
    }

    bool timedWait(uint32_t milliseconds)
    {
        /* returns true if the wait timed out */
        return WaitForSingleObject(this->handle, milliseconds) == WAIT_TIMEOUT;
    }

    void trigger()
    {
        SetEvent(this->handle);
    }

protected:

    HANDLE handle;
};

/* This class is intended for use in signaling state changes safely between CPU
 * cores. One thread should be a writer and multiple threads may be readers. The
 * mutex's main purpose is to serve as a memory fence to ensure writes made by
 * the writer thread are visible prior to readers seeing the m_val change. Its
 * secondary purpose is for use with the condition variable for blocking waits */
class ThreadSafeInteger
{
public:

    ThreadSafeInteger()
    {
        m_val = 0;
        InitializeCriticalSection(&m_cs);
        InitializeConditionVariable(&m_cv);
    }

    ~ThreadSafeInteger()
    {
        DeleteCriticalSection(&m_cs);
        XP_CONDITION_VAR_FREE(&m_cv);
    }

    int waitForChange(int prev)
    {
        EnterCriticalSection(&m_cs);
        if (m_val == prev)
            SleepConditionVariableCS(&m_cv, &m_cs, INFINITE);
        LeaveCriticalSection(&m_cs);
        return m_val;
    }

    int get()
    {
        EnterCriticalSection(&m_cs);
        int ret = m_val;
        LeaveCriticalSection(&m_cs);
        return ret;
    }

    int getIncr(int n = 1)
    {
        EnterCriticalSection(&m_cs);
        int ret = m_val;
        m_val += n;
        LeaveCriticalSection(&m_cs);
        return ret;
    }

    void set(int newval)
    {
        EnterCriticalSection(&m_cs);
        m_val = newval;
        WakeAllConditionVariable(&m_cv);
        LeaveCriticalSection(&m_cs);
    }

    void poke(void)
    {
        /* awaken all waiting threads, but make no change */
        EnterCriticalSection(&m_cs);
        WakeAllConditionVariable(&m_cv);
        LeaveCriticalSection(&m_cs);
    }

    void incr()
    {
        EnterCriticalSection(&m_cs);
        m_val++;
        WakeAllConditionVariable(&m_cv);
        LeaveCriticalSection(&m_cs);
    }

    void decr()
    {
        EnterCriticalSection(&m_cs);
        m_val--;
        WakeAllConditionVariable(&m_cv);
        LeaveCriticalSection(&m_cs);
    }

protected:

    CRITICAL_SECTION   m_cs;
    CONDITION_VARIABLE m_cv;
    int                m_val;
};

class NamedSemaphore
{
public:
    NamedSemaphore() : m_sem(NULL)
    {
    }

    ~NamedSemaphore()
    {
    }

    bool create(const char* name, const int initcnt, const int maxcnt)
    {
        if(!m_sem)
        {
            m_sem = CreateSemaphoreA(NULL, initcnt, maxcnt, name);
        }
        return m_sem != NULL;
    }

    bool give(const int32_t cnt)
    {
        return ReleaseSemaphore(m_sem, (LONG)cnt, NULL) != FALSE;
    }

    bool take(const uint32_t time_out = INFINITE)
    {
        int32_t rt = WaitForSingleObject(m_sem, time_out);
        return rt != WAIT_TIMEOUT && rt != WAIT_FAILED;
    }

    void release()
    {
        CloseHandle(m_sem);
        m_sem = NULL;
    }

private:
    HANDLE m_sem;
};

#else /* POSIX / pthreads */

typedef pthread_t ThreadHandle;

class Lock
{
public:

    Lock()
    {
        pthread_mutex_init(&this->handle, NULL);
    }

    ~Lock()
    {
        pthread_mutex_destroy(&this->handle);
    }

    void acquire()
    {
        pthread_mutex_lock(&this->handle);
    }

    void release()
    {
        pthread_mutex_unlock(&this->handle);
    }

protected:

    pthread_mutex_t handle;
};

class Event
{
public:

    Event()
    {
        m_counter = 0;
        if (pthread_mutex_init(&m_mutex, NULL) ||
            pthread_cond_init(&m_cond, NULL))
        {
            x265_log(NULL, X265_LOG_ERROR, "fatal: unable to initialize conditional variable\n");
        }
    }

    ~Event()
    {
        pthread_cond_destroy(&m_cond);
        pthread_mutex_destroy(&m_mutex);
    }

    void wait()
    {
        pthread_mutex_lock(&m_mutex);

        /* blocking wait on conditional variable, mutex is atomically released
         * while blocked. When condition is signaled, mutex is re-acquired */
        while (!m_counter)
            pthread_cond_wait(&m_cond, &m_mutex);

        m_counter--;
        pthread_mutex_unlock(&m_mutex);
    }

    bool timedWait(uint32_t waitms)
    {
        bool bTimedOut = false;

        pthread_mutex_lock(&m_mutex);
        if (!m_counter)
        {
            struct timeval tv;
            struct timespec ts;
            gettimeofday(&tv, NULL);
            /* convert current time from (sec, usec) to (sec, nsec) */
            ts.tv_sec = tv.tv_sec;
            ts.tv_nsec = tv.tv_usec * 1000;

            ts.tv_nsec += 1000 * 1000 * (waitms % 1000);    /* add ms to tv_nsec */
            ts.tv_sec += ts.tv_nsec / (1000 * 1000 * 1000); /* overflow tv_nsec */
            ts.tv_nsec %= (1000 * 1000 * 1000);             /* clamp tv_nsec */
            ts.tv_sec += waitms / 1000;                     /* add seconds */

            /* blocking wait on conditional variable, mutex is atomically released
             * while blocked. When condition is signaled, mutex is re-acquired.
             * ts is absolute time to stop waiting */
            bTimedOut = pthread_cond_timedwait(&m_cond, &m_mutex, &ts) == ETIMEDOUT;
        }
        if (m_counter > 0)
        {
            m_counter--;
            bTimedOut = false;
        }
        pthread_mutex_unlock(&m_mutex);
        return bTimedOut;
    }

    void trigger()
    {
        pthread_mutex_lock(&m_mutex);
        if (m_counter < UINT_MAX)
            m_counter++;
        /* Signal a single blocking thread */
        pthread_cond_signal(&m_cond);
        pthread_mutex_unlock(&m_mutex);
    }

protected:

    pthread_mutex_t m_mutex;
    pthread_cond_t  m_cond;
    uint32_t        m_counter;
};

/* This class is intended for use in signaling state changes safely between CPU
 * cores. One thread should be a writer and multiple threads may be readers. The
 * mutex's main purpose is to serve as a memory fence to ensure writes made by
 * the writer thread are visible prior to readers seeing the m_val change. Its
 * secondary purpose is for use with the condition variable for blocking waits */
class ThreadSafeInteger
{
public:

    ThreadSafeInteger()
    {
        m_val = 0;
        if (pthread_mutex_init(&m_mutex, NULL) ||
            pthread_cond_init(&m_cond, NULL))
        {
            x265_log(NULL, X265_LOG_ERROR, "fatal: unable to initialize conditional variable\n");
        }
    }

    ~ThreadSafeInteger()
    {
        pthread_cond_destroy(&m_cond);
        pthread_mutex_destroy(&m_mutex);
    }

    int waitForChange(int prev)
    {
        pthread_mutex_lock(&m_mutex);
        if (m_val == prev)
            pthread_cond_wait(&m_cond, &m_mutex);
        pthread_mutex_unlock(&m_mutex);
        return m_val;
    }

    int get()
    {
        pthread_mutex_lock(&m_mutex);
        int ret = m_val;
        pthread_mutex_unlock(&m_mutex);
        return ret;
    }

    int getIncr(int n = 1)
    {
        pthread_mutex_lock(&m_mutex);
        int ret = m_val;
        m_val += n;
        pthread_mutex_unlock(&m_mutex);
        return ret;
    }

    void set(int newval)
    {
        pthread_mutex_lock(&m_mutex);
        m_val = newval;
        pthread_cond_broadcast(&m_cond);
        pthread_mutex_unlock(&m_mutex);
    }

    void poke(void)
    {
        /* awaken all waiting threads, but make no change */
        pthread_mutex_lock(&m_mutex);
        pthread_cond_broadcast(&m_cond);
        pthread_mutex_unlock(&m_mutex);
    }

    void incr()
    {
        pthread_mutex_lock(&m_mutex);
        m_val++;
        pthread_cond_broadcast(&m_cond);
        pthread_mutex_unlock(&m_mutex);
    }

    void decr()
    {
        pthread_mutex_lock(&m_mutex);
        m_val--;
        pthread_cond_broadcast(&m_cond);
        pthread_mutex_unlock(&m_mutex);
    }

protected:

    pthread_mutex_t m_mutex;
    pthread_cond_t  m_cond;
    int             m_val;
};

#define TIMEOUT_INFINITE 0xFFFFFFFF

class NamedSemaphore
{
public:
    NamedSemaphore() 
        : m_sem(NULL)
#ifndef __APPLE__
        , m_name(NULL)
#endif //__APPLE__
    {
    }

    ~NamedSemaphore()
    {
    }

    bool create(const char* name, const int initcnt, const int maxcnt)
    {
        bool ret = false;

        if (initcnt >= maxcnt)
        {
            return false;
        }

#ifdef __APPLE__
        do
        {
            int32_t pshared = name != NULL ? PTHREAD_PROCESS_SHARED : PTHREAD_PROCESS_PRIVATE;

            m_sem = (mac_sem_t *)malloc(sizeof(mac_sem_t));
            if (!m_sem)
            {
                break;
            }

            if (pthread_mutexattr_init(&m_sem->mutexAttr))
            {
                break;
            }

            if (pthread_mutexattr_setpshared(&m_sem->mutexAttr, pshared))
            {
                break;
            }

            if (pthread_condattr_init(&m_sem->condAttr))
            {
                break;
            }

            if (pthread_condattr_setpshared(&m_sem->condAttr, pshared))
            {
                break;
            }

            if (pthread_mutex_init(&m_sem->mutex, &m_sem->mutexAttr))
            {
                break;
            }

            if (pthread_cond_init(&m_sem->cond, &m_sem->condAttr))
            {
                break;
            }

            m_sem->curCnt = initcnt;
            m_sem->maxCnt = maxcnt;

            ret = true;
        } while (0);
        
        if (!ret)
        {
            release();
        }

#else  //__APPLE__
        m_sem = sem_open(name, O_CREAT | O_EXCL, 0666, initcnt);
        if (m_sem != SEM_FAILED) 
        {
            m_name = strdup(name);
            ret = true;
        }
        else 
        {
            if (EEXIST == errno) 
            {
                m_sem = sem_open(name, 0);
                if (m_sem != SEM_FAILED) 
                {
                    m_name = strdup(name);
                    ret = true;
                }
            }
        }
#endif //__APPLE__

        return ret;
    }

    bool give(const int32_t cnt)
    {
        if (!m_sem)
        {
            return false;
        }

#ifdef __APPLE__
        if (pthread_mutex_lock(&m_sem->mutex))
        {
            return false;
        }

        int oldCnt = m_sem->curCnt;
        m_sem->curCnt += cnt;
        if (m_sem->curCnt > m_sem->maxCnt)
        {
            m_sem->curCnt = m_sem->maxCnt;
        }

        bool ret = true;
        if (!oldCnt)
        {
            ret = 0 == pthread_cond_broadcast(&m_sem->cond);
        }

        if (pthread_mutex_unlock(&m_sem->mutex))
        {
            return false;
        }

        return ret;
#else //__APPLE__
        int ret = 0;
        int32_t curCnt = cnt;
        while (curCnt-- && !ret) {
            ret = sem_post(m_sem);
        }

        return 0 == ret;
#endif //_APPLE__
    }

    bool take(const uint32_t time_out = TIMEOUT_INFINITE)
    {
        if (!m_sem)
        {
            return false;
        }

#ifdef __APPLE__

        if (pthread_mutex_lock(&m_sem->mutex))
        {
            return false;
        }

        bool ret = true;
        if (TIMEOUT_INFINITE == time_out) 
        {
            if (!m_sem->curCnt)
            {
                if (pthread_cond_wait(&m_sem->cond, &m_sem->mutex))
                {
                    ret = false;
                } 
            }

            if (m_sem->curCnt && ret)
            {
                m_sem->curCnt--;
            }
        }
        else
        {
            if (0 == time_out)
            {
                if (m_sem->curCnt)
                {
                    m_sem->curCnt--;
                }
                else
                {
                    ret = false;
                }
            }
            else
            {
                if (!m_sem->curCnt)
                {
                    struct timespec ts;
                    ts.tv_sec = time_out / 1000L;
                    ts.tv_nsec = (time_out * 1000000L) - ts.tv_sec * 1000 * 1000 * 1000;

                    if (pthread_cond_timedwait(&m_sem->cond, &m_sem->mutex, &ts))
                    {
                        ret = false;
                    }
                }

                if (m_sem->curCnt && ret)
                {
                    m_sem->curCnt--;
                }
            }
        }

        if (pthread_mutex_unlock(&m_sem->mutex))
        {
            return false;
        }

        return ret;
#else //__APPLE__
        if (TIMEOUT_INFINITE == time_out) 
        {
            return 0 == sem_wait(m_sem);
        }
        else 
        {
            if (0 == time_out)
            {
                return 0 == sem_trywait(m_sem);
            }
            else
            {
                struct timespec ts;
                ts.tv_sec = time_out / 1000L;
                ts.tv_nsec = (time_out * 1000000L) - ts.tv_sec * 1000 * 1000 * 1000;
                return 0 == sem_timedwait(m_sem, &ts);
            }
        }
#endif //_APPLE__
    }

    void release()
    {
        if (m_sem)
        {
#ifdef __APPLE__
            pthread_condattr_destroy(&m_sem->condAttr);
            pthread_mutexattr_destroy(&m_sem->mutexAttr);
            pthread_mutex_destroy(&m_sem->mutex);
            pthread_cond_destroy(&m_sem->cond);
            free(m_sem);
            m_sem = NULL;
#else //__APPLE__
            sem_close(m_sem);
            sem_unlink(m_name);
            m_sem = NULL;
            free(m_name);
            m_name = NULL;
#endif //__APPLE__
        }
    }

private:
#ifdef __APPLE__
    typedef struct
    {
        pthread_mutex_t     mutex;
        pthread_cond_t      cond;
        pthread_mutexattr_t mutexAttr;
        pthread_condattr_t  condAttr;
        uint32_t            curCnt;
        uint32_t            maxCnt;
    }mac_sem_t;
    mac_sem_t *m_sem;
#else // __APPLE__
    sem_t *m_sem;
    char  *m_name;
#endif // __APPLE_
};

#endif // ifdef _WIN32

class ScopedLock
{
public:

    ScopedLock(Lock &instance) : inst(instance)
    {
        this->inst.acquire();
    }

    ~ScopedLock()
    {
        this->inst.release();
    }

protected:

    // do not allow assignments
    ScopedLock &operator =(const ScopedLock &);

    Lock &inst;
};

// Utility class which adds elapsed time of the scope of the object into the
// accumulator provided to the constructor
struct ScopedElapsedTime
{
    ScopedElapsedTime(int64_t& accum) : accumlatedTime(accum) { startTime = x265_mdate(); }

    ~ScopedElapsedTime() { accumlatedTime += x265_mdate() - startTime; }

protected:

    int64_t  startTime;
    int64_t& accumlatedTime;

    // do not allow assignments
    ScopedElapsedTime &operator =(const ScopedElapsedTime &);
};

//< Simplistic portable thread class.  Shutdown signalling left to derived class
class Thread
{
private:

    ThreadHandle thread;

public:

    Thread();

    virtual ~Thread();

    //< Derived class must implement ThreadMain.
    virtual void threadMain() = 0;

    //< Returns true if thread was successfully created
    bool start();

    void stop();
};
} // end namespace X265_NS

#endif // ifndef X265_THREADING_H
