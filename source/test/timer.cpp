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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/timeb.h>

// Code snippet from http://www.winehq.org/pipermail/wine-devel/2003-June/018082.html begins
// this part is windows implementation of Gettimeoffday() function

#ifndef _TIMEVAL_H
#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>

#if defined(_MSC_VER)
#define EPOCHFILETIME (116444736000000000i64)
#else
#define EPOCHFILETIME (116444736000000000LL)
#endif

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    FILETIME        ft;
    LARGE_INTEGER   li;
    __int64         t;

    if (tv)
    {
        GetSystemTimeAsFileTime(&ft);
        li.LowPart  = ft.dwLowDateTime;
        li.HighPart = ft.dwHighDateTime;
        t  = li.QuadPart;       /* In 100-nanosecond intervals */
        t -= EPOCHFILETIME;     /* Offset to the Epoch time */
        t /= 10;                /* In microseconds */
        tv->tv_sec  = (long)(t / 1000000);
        tv->tv_usec = (long)(t % 1000000);
    }

    return 0;
}

#else  /* _WIN32 */

#include <sys/time.h>

#endif /* _WIN32 */
#endif /* _TIMEVAL_H */

// Code snippet from http://www.winehq.org/pipermail/wine-devel/2003-June/018082.html ends


class TimerImpl : public Timer
{
protected:

    timeval start, finish;

public:

    void Start();

    void Stop();

    float ElapsedMS();

    void Release()      { delete this; }
};

Timer *Timer::CreateTimer()
{
    return new TimerImpl();
}

void TimerImpl::Start()
{
    gettimeofday(&start, NULL);
}

void TimerImpl::Stop()
{
    gettimeofday(&finish, NULL);
}

float TimerImpl::ElapsedMS()
{
    float msec = (finish.tv_sec - start.tv_sec) * 1000;
    msec += (float)(finish.tv_usec - start.tv_usec) / 1000;
    return msec;
}
