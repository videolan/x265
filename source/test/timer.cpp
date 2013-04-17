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

    float ElapsedMS()
    {
        return ((float)(finish.QuadPart - start.QuadPart) / freq.QuadPart) * 1000.0f;
    }

    void Release()      { delete this; }
};

#else

#include <sys/types.h>
#include <sys/timeb.h>
#include <sys/time.h>

class TimerImpl : public Timer
{
protected:

    timeval start, finish;

public:

    void Start()        { gettimeofday(&start, 0); }

    void Stop()         { gettimeofday(&finish, 0); }

    float ElapsedMS()
    {
        return (finish.tv_sec - start.tv_sec) * 1000 +
        (float)(finish.tv_usec - start.tv_usec) / 1000;
    }

    void Release()      { delete this; }
};

#endif /* _WIN32 */

Timer *Timer::CreateTimer()
{
    return new TimerImpl();
}
