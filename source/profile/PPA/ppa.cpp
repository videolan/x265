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
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#if defined(ENABLE_PPA)

#include "ppa.h"
#include <stdlib.h>

#define PPA_REGISTER_CPU_EVENT2GROUP(x, y) # x, # y,
#define CPU_EVENT(x) PPA_REGISTER_CPU_EVENT2GROUP(x, NoGroup)
const char *PPACpuAndGroup[] =
{
#include "../cpuEvents.h"
    ""
};
#undef CPU_EVENT
#undef PPA_REGISTER_CPU_EVENT2GROUP

extern "C" {
typedef ppa::Base *(FUNC_PPALibInit)(const char **, int);
typedef void (FUNC_PPALibRelease)(ppa::Base* &);
}

using namespace ppa;

static FUNC_PPALibRelease *_pfuncPpaRelease;
ppa::Base *ppa::ppabase;

static void _ppaReleaseAtExit()
{
    _pfuncPpaRelease(ppabase);
}

#ifdef _WIN32
#include <windows.h>

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
