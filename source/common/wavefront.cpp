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
 * For more information, contact us at licensing@multicorewareinc.com
 *****************************************************************************/

#include "threadpool.h"
#include "threading.h"
#include "wavefront.h"
#include <assert.h>
#include <string.h>
#include <new>

#if MACOS
#include <sys/param.h>
#include <sys/sysctl.h>
#endif

#ifdef __GNUC__                         /* GCCs builtin atomics */

#include <unistd.h>
#include <limits.h>

#define CLZ64(id, x)                    id = 63 - (unsigned long)__builtin_clzll(x)
#define ATOMIC_OR(ptr, mask)            __sync_or_and_fetch(ptr, mask)
#define ATOMIC_CAS(ptr, oldval, newval) __sync_val_compare_and_swap(ptr, oldval, newval)

#elif defined(_MSC_VER)                 /* Windows atomic intrinsics */

#include <intrin.h>

#if !_WIN64
inline int _BitScanReverse64(DWORD *id, uint64_t x64)
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

#endif // if !_WIN64

#if _WIN32_WINNT <= _WIN32_WINNT_WINXP
/* XP did not define this intrinsic */
FORCEINLINE LONGLONG _InterlockedOr64(__inout LONGLONG volatile *Destination,
                                      __in    LONGLONG           Value
                                      )
{
    LONGLONG Old;

    do
    {
        Old = *Destination;
    }
    while (_InterlockedCompareExchange64(Destination,
                                         Old | Value,
                                         Old) != Old);

    return Old;
}

#define ATOMIC_OR(ptr, mask)            _InterlockedOr64((volatile LONG64*)ptr, mask)
#if defined(__MSC_VER) && !defined(__INTEL_COMPILER)
#pragma intrinsic(_InterlockedCompareExchange64)
#endif
#else // if _WIN32_WINNT <= _WIN32_WINNT_WINXP
#define ATOMIC_OR(ptr, mask)            InterlockedOr64((volatile LONG64*)ptr, mask)
#endif // if _WIN32_WINNT <= _WIN32_WINNT_WINXP

#define CLZ64(id, x)                    _BitScanReverse64(&id, x)
#define ATOMIC_CAS(ptr, oldval, newval) (uint64_t)_InterlockedCompareExchange64((volatile LONG64*)ptr, newval, oldval)

#endif // ifdef __GNUC__

namespace x265 {
// x265 private namespace

bool WaveFront::initJobQueue(int numRows)
{
    m_numRows = numRows;

    if (m_pool)
    {
        m_numWords = (numRows + 63) >> 6;
        m_queuedBitmap = new uint64_t[m_numWords];
        memset((void*)m_queuedBitmap, 0, sizeof(uint64_t) * m_numWords);
        return m_queuedBitmap != NULL;
    }

    return false;
}

WaveFront::~WaveFront()
{
    if (m_queuedBitmap)
    {
        delete[] m_queuedBitmap;
        m_queuedBitmap = NULL;
    }
}

void WaveFront::enqueueRow(int row)
{
    // thread safe
    uint64_t bit = 1LL << (row & 63);

    assert(row < m_numRows);
    ATOMIC_OR(&m_queuedBitmap[row >> 6], bit);
    m_pool->pokeIdleThread();
}

bool WaveFront::findJob()
{
    unsigned long id;

    // thread safe
    for (int w = 0; w < m_numWords; w++)
    {
        while (m_queuedBitmap[w])
        {
            uint64_t oldval = m_queuedBitmap[w];
            if (oldval == 0) // race condition
                break;

            CLZ64(id, oldval);
            uint64_t newval = oldval & ~(1LL << id);

            if (ATOMIC_CAS(&m_queuedBitmap[w], oldval, newval) == oldval)
            {
                // if the bit was actually flipped, process row, else try again
                processRow(w * 64 + id);
                return true;
            }
        }
    }

    // made it through the bitmap without finding any enqueued rows
    return false;
}
}