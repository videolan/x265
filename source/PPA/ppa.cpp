/*
 * File             : ppa.cpp
 *                    ( C++ source file )
 *
 * Description      : Parallel Path Analyzer
 *                    ( This is a source of the PPA SDK. )
 *                    Include this file in your application.
 *
 * Copyright        : MulticoreWare Inc.
 *                    ( http://www.multicorewareinc.com )
 *                    Copyright (c) 2011 MulticoreWare Inc. All rights reserved
 *
 *
 * License          : PPA Software License v0.1
 *
 *
 * This software is governed by MulticoreWare Inc. Permission to use, reproduce,
 * copy, modify, display, distribute, execute and transmit this software
 * for any purpose and any use of its accompanying documentation is hereby granted.
 * Provided that the above copyright notice appear in all copies, and that
 * this software is not used in advertising or publicity without specific, written
 * prior permission by MulticoreWare Inc, and that the following disclaimer is
 * included.
 *
 *
 * THE SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
 * INCLUDING, WITHOUT LIMITATION, ANY WARRANTY OF DESIGN, MERCHANTABILITY
 * OR FITNESS FOR A PARTICULAR PURPOSE, AND WITHOUT WARRANTY AS TO NON-INFRINGEMENT
 * OR THE PERFORMANCE OR RESULTS YOU MAY OBTAIN BY USING THE SOFTWARE.
 */

#if defined(ENABLE_PPA)

#include "ppa.h"
#include <stdlib.h>

#define PPA_REGISTER_CPU_EVENT2GROUP(x, y) # x, # y,
#define PPA_REGISTER_CPU_EVENT(x) PPA_REGISTER_CPU_EVENT2GROUP(x, NoGroup)
const char *PPACpuAndGroup[] =
{
#include "ppaCPUEvents.h"
    ""
};
#undef PPA_REGISTER_CPU_EVENT
#undef PPA_REGISTER_CPU_EVENT2GROUP

extern "C" {
typedef ppa::Base *(FUNC_PPALibInit)(const char **, int);
typedef void (FUNC_PPALibRelease)(ppa::Base* &);
}

static FUNC_PPALibRelease *_pfuncPpaRelease;
ppa::Base *ppabase;

static void _ppaReleaseAtExit()
{
    _pfuncPpaRelease(ppabase);
}

#ifdef _WIN32
#include <Windows.h>

#if defined(_M_X64) || defined(__x86_64__) || defined(__amd64__)
# ifdef UNICODE
# define PPA_DLL_NAME L"ppa64.dll"
# else
# define PPA_DLL_NAME "ppa64.dll"
# endif
#else
# ifdef UNICODE
# define PPA_DLL_NAME L"ppa.dll"
# else
# define PPA_DLL_NAME "ppa.dll"
# endif
#endif // if defined(_M_X64) || defined(__x86_64__) || defined(__amd64__)

void initializePPA(void)
{
    if (ppabase)
        return;

    HMODULE _ppaLibHandle = LoadLibrary(PPA_DLL_NAME);
    if (!_ppaLibHandle)
        return;

    FUNC_PPALibInit *_pfuncPpaInit = (FUNC_PPALibInit*)GetProcAddress(_ppaLibHandle, "InitPpaUtil");
    _pfuncPpaRelease  = (FUNC_PPALibRelease*)GetProcAddress(_ppaLibHandle, "DeletePpa");

    if (!_pfuncPpaInit || !_pfuncPpaRelease)
    {
        FreeLibrary(_ppaLibHandle);
        return;
    }

    ppabase = _pfuncPpaInit(PPACpuAndGroup, PPACpuGroupNums);
    if (!ppabase)
    {
        FreeLibrary(_ppaLibHandle);
        return;
    }

    atexit(_ppaReleaseAtExit);
}

#else /* linux & unix & cygwin */
#include <dlfcn.h>
#include <stdio.h>

#if defined(_M_X64) || defined(__x86_64__) || defined(__amd64__)
# define PPA_LIB_NAME "libppa64.so"
#else
# define PPA_LIB_NAME "libppa.so"
#endif

void initializePPA(void)
{
    if (ppabase)
    {
        printf("PPA: already initialized\n");
        return;
    }

    void *_ppaDllHandle = dlopen(PPA_LIB_NAME, RTLD_LAZY);
    if (!_ppaDllHandle)
    {
        printf("PPA: Unable to load %s\n", PPA_LIB_NAME);
        return;
    }

    FUNC_PPALibInit *_pfuncPpaInit = (FUNC_PPALibInit*)dlsym(_ppaDllHandle, "InitPpaUtil");
    _pfuncPpaRelease = (FUNC_PPALibRelease*)dlsym(_ppaDllHandle, "DeletePpa");

    if (!_pfuncPpaInit || !_pfuncPpaRelease)
    {
        printf("PPA: Function bindings failed\n");
        dlclose(_ppaDllHandle);
        return;
    }

    ppabase = _pfuncPpaInit(PPACpuAndGroup, PPACpuGroupNums);
    if (!ppabase)
    {
        printf("PPA: Init failed\n");
        dlclose(_ppaDllHandle);
        return;
    }

    atexit(_ppaReleaseAtExit);
}

#endif /* !_WIN32 */

#endif /* defined(ENABLE_PPA) */
