/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Gopu Govindaswamy <gopu@govindaswamy.org>
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
 * For more information, contact us at licensing@multicorewareinc.com.
 *****************************************************************************/

#include "testharness.h"
#include "primitives.h"

#if _WIN32

#include <Windows.h>

class TimerImpl : public Timer
{
protected:

    LARGE_INTEGER freq;

    LARGE_INTEGER start, finish;

public:

    TimerImpl()         { QueryPerformanceFrequency(&freq); }

    void Start()        { QueryPerformanceCounter(&start); }

    void Stop()         { QueryPerformanceCounter(&finish); }

    uint64_t Elapsed()  { return finish.QuadPart - start.QuadPart; }

    void Release()      { delete this; }
};

#else // if _WIN32

#include <time.h>

class TimerImpl : public Timer
{
protected:

    timespec start, finish;

public:

    void Start()
    {
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
    }

    void Stop()
    {
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &finish);
    }

    uint64_t Elapsed()
    {
        return (uint64_t)(finish.tv_sec - start.tv_sec) * 1000000000 + 
               (finish.tv_nsec - start.tv_nsec);
    }

    void Release()      { delete this; }
};

#endif /* _WIN32 */

Timer *Timer::CreateTimer()
{
    return new TimerImpl();
}
