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

#include "threading.h"

namespace x265 {
// x265 private namespace

/* C shim for forced stack alignment */
static void stackAlignMain(Thread *instance)
{
    instance->threadMain();
}

#if _WIN32

static DWORD WINAPI ThreadShim(Thread *instance)
{
    // defer processing to the virtual function implemented in the derived class
    x265_stack_align(stackAlignMain, instance);

    return 0;
}

bool Thread::start()
{
    DWORD threadId;

    this->thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadShim, this, 0, &threadId);

    return threadId > 0;
}

void Thread::stop()
{
    if (this->thread)
        WaitForSingleObject(this->thread, INFINITE);
}

Thread::~Thread()
{
    if (this->thread)
        CloseHandle(this->thread);
}

#else /* POSIX / pthreads */

static void *ThreadShim(void *opaque)
{
    // defer processing to the virtual function implemented in the derived class
    Thread *instance = reinterpret_cast<Thread *>(opaque);

    x265_stack_align(stackAlignMain, instance);

    return NULL;
}

bool Thread::start()
{
    if (pthread_create(&this->thread, NULL, ThreadShim, this))
    {
        this->thread = 0;

        return false;
    }

    return true;
}

void Thread::stop()
{
    if (this->thread)
        pthread_join(this->thread, NULL);
}

Thread::~Thread() {}

#endif // if _WIN32

Thread::Thread()
{
    this->thread = 0;
}
}
