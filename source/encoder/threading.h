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
#include "Windows.h"
#else
#include <pthread.h>
#include <stdlib.h>
#endif

namespace x265
{

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

    void Acquire()
    {
        EnterCriticalSection(&this->handle);
    }

    void Release()
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

    void Wait()
    {
        WaitForSingleObject(this->handle, INFINITE);
    }

    void Trigger()
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

    void Acquire()
    {
        pthread_mutex_lock(&this->handle);
    }

    void Release()
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
        pthread_cond_init(&this->condition, NULL);
        pthread_mutex_init(&this->lock, NULL);
    }

    ~Event()
    {
        pthread_mutex_destroy(&this->lock);
        pthread_cond_destroy(&this->condition);
    }

    void Wait()
    {
        pthread_mutex_lock(&this->lock);
        pthread_cond_wait(&this->condition, &this->lock);
        pthread_mutex_unlock(&this->lock);
    }

    void Trigger()
    {
        pthread_mutex_lock(&this->lock);
        pthread_cond_broadcast(&this->condition);
        pthread_mutex_unlock(&this->lock);
    }

protected:

    pthread_mutex_t lock;
    pthread_cond_t  condition;
};

#endif

class ScopedLock
{
public:
    ScopedLock(Lock &instance) : inst(instance)
    {
        this->inst.Acquire();
    }

    ~ScopedLock()
    {
        this->inst.Release();
    }

protected:

    // do not allow assignments
    ScopedLock &operator =(const ScopedLock &) { return *this; }

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
    virtual void ThreadMain() = 0;

    //< Returns true if thread was successfully created
    bool Start();
};

} // end namespace x265

#endif
