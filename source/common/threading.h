/*****************************************************************************
 * x265: threading class and intrinsics
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

#ifndef _THREADING_H_
#define _THREADING_H_

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <semaphore.h>
#include <cstdlib>
#include <stdio.h>
#include <errno.h>
#endif
#include <stdint.h>

namespace x265 {
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

    void trigger()
    {
        SetEvent(this->handle);
    }

protected:

    HANDLE handle;
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
        do {
            snprintf(name, sizeof(name), "/x265_%d", s_incr++);
            this->semaphore = sem_open(name, O_CREAT | O_EXCL, 0777, 0);
        } while (this->semaphore == SEM_FAILED);
    }

    ~Event()
    {
        sem_close(this->semaphore);
        sem_unlink(name);
    }

    void wait()
    {
        // keep waiting even if interrupted
        while (sem_wait(this->semaphore) < 0)
            if (errno != EINTR)
                break;
    }

    void trigger()
    {
        sem_post(this->semaphore);
    }

protected:
    
    static int s_incr;
    char name[64];

    /* the POSIX version uses a counting semaphore */
    sem_t *semaphore;
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
} // end namespace x265


#if MACOS
#include <sys/param.h>
#include <sys/sysctl.h>
#endif

#ifdef __GNUC__                         /* GCCs builtin atomics */

#include <unistd.h>
#include <limits.h>

#define CTZ64(id, x)                        id = (unsigned long)__builtin_ctzll(x)
#define ATOMIC_OR(ptr, mask)                __sync_or_and_fetch(ptr, mask)
#define ATOMIC_CAS(ptr, oldval, newval)     __sync_val_compare_and_swap(ptr, oldval, newval)
#define ATOMIC_CAS32(ptr, oldval, newval)   __sync_val_compare_and_swap(ptr, oldval, newval)
#define ATOMIC_INC(ptr)                     __sync_add_and_fetch((volatile int*)ptr, 1)
#define ATOMIC_DEC(ptr)                     __sync_add_and_fetch((volatile int*)ptr, -1)
#define GIVE_UP_TIME()                      usleep(0)

#elif defined(_MSC_VER)                 /* Windows atomic intrinsics */

#include <intrin.h>

#if !_WIN64
inline int _BitScanReverse64(DWORD *id, uint64_t x64) // fake 64bit CLZ
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

inline int _BitScanForward64(DWORD *id, uint64_t x64) // fake 64bit CLZ
{
    uint32_t high32 = (uint32_t)(x64 >> 32);
    uint32_t low32 = (uint32_t)x64;

    if (high32)
    {
        _BitScanForward(id, high32);
        *id += 32;
        return 1;
    }
    else if (low32)
        return _BitScanForward(id, low32);
    else
        return *id = 0;
}
#endif // if !_WIN64

#if _WIN32_WINNT <= _WIN32_WINNT_WINXP
/* Windows XP did not define this intrinsic */
FORCEINLINE LONGLONG _InterlockedOr64(__inout LONGLONG volatile *Destination,
                                      __in    LONGLONG           Value)
{
    LONGLONG Old;

    do
    {
        Old = *Destination;
    }
    while (_InterlockedCompareExchange64(Destination, Old | Value, Old) != Old);

    return Old;
}

#define ATOMIC_OR(ptr, mask)            _InterlockedOr64((volatile LONG64*)ptr, mask)
#if defined(__MSC_VER) && !defined(__INTEL_COMPILER)
#pragma intrinsic(_InterlockedCompareExchange64)
#endif
#else // if _WIN32_WINNT <= _WIN32_WINNT_WINXP
#define ATOMIC_OR(ptr, mask)            InterlockedOr64((volatile LONG64*)ptr, mask)
#endif // if _WIN32_WINNT <= _WIN32_WINNT_WINXP

#define CTZ64(id, x)                        _BitScanForward64(&id, x)
#define ATOMIC_CAS(ptr, oldval, newval)     (uint64_t)_InterlockedCompareExchange64((volatile LONG64*)ptr, newval, oldval)
#define ATOMIC_CAS32(ptr, oldval, newval)   (uint64_t)_InterlockedCompareExchange((volatile LONG*)ptr, newval, oldval)
#define ATOMIC_INC(ptr)                     InterlockedIncrement((volatile LONG*)ptr)
#define ATOMIC_DEC(ptr)                     InterlockedDecrement((volatile LONG*)ptr)
#define GIVE_UP_TIME()                      Sleep(0)

#endif // ifdef __GNUC__

#endif // ifndef _THREADING_H_
