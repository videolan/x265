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

#include "threadpool.h"
#include "threading.h"
#include "wavefront.h"
#include "common.h"

namespace x265 {
// x265 private namespace

bool WaveFront::init(int numRows)
{
    m_numRows = numRows;

    m_numWords = (numRows + 63) >> 6;
    m_internalDependencyBitmap = X265_MALLOC(uint64_t, m_numWords);
    if (m_internalDependencyBitmap)
        memset((void*)m_internalDependencyBitmap, 0, sizeof(uint64_t) * m_numWords);

    m_externalDependencyBitmap = X265_MALLOC(uint64_t, m_numWords);
    if (m_externalDependencyBitmap)
        memset((void*)m_externalDependencyBitmap, 0, sizeof(uint64_t) * m_numWords);

    return m_internalDependencyBitmap && m_externalDependencyBitmap;
}

WaveFront::~WaveFront()
{
    x265_free((void*)m_internalDependencyBitmap);
    x265_free((void*)m_externalDependencyBitmap);
}

void WaveFront::clearEnabledRowMask()
{
    memset((void*)m_externalDependencyBitmap, 0, sizeof(uint64_t) * m_numWords);
}

void WaveFront::enqueueRow(int row)
{
    // thread safe
    uint64_t bit = 1LL << (row & 63);

    X265_CHECK(row < m_numRows, "invalid row\n");
    ATOMIC_OR(&m_internalDependencyBitmap[row >> 6], bit);
    if (m_pool) m_pool->pokeIdleThread();
}

void WaveFront::enableRow(int row)
{
    // thread safe
    uint64_t bit = 1LL << (row & 63);

    X265_CHECK(row < m_numRows, "invalid row\n");
    ATOMIC_OR(&m_externalDependencyBitmap[row >> 6], bit);
}

void WaveFront::enableAllRows()
{
    memset((void*)m_externalDependencyBitmap, ~0, sizeof(uint64_t) * m_numWords);
}

bool WaveFront::checkHigherPriorityRow(int curRow)
{
    int fullwords = curRow >> 6;
    uint64_t mask = (1LL << (curRow & 63)) - 1;

    // Check full bitmap words before curRow
    for (int i = 0; i < fullwords; i++)
    {
        if (m_internalDependencyBitmap[i] & m_externalDependencyBitmap[i])
            return true;
    }

    // check the partially masked bitmap word of curRow
    if (m_internalDependencyBitmap[fullwords] & m_externalDependencyBitmap[fullwords] & mask)
        return true;
    return false;
}

bool WaveFront::dequeueRow(int row)
{
    uint64_t oldval, newval;

    oldval = m_internalDependencyBitmap[row >> 6];
    newval = oldval & ~(1LL << (row & 63));
    return ATOMIC_CAS(&m_internalDependencyBitmap[row >> 6], oldval, newval) == oldval;
}

bool WaveFront::findJob(int threadId)
{
    unsigned long id;

    // thread safe
    for (int w = 0; w < m_numWords; w++)
    {
        uint64_t oldval = m_internalDependencyBitmap[w];
        while (oldval & m_externalDependencyBitmap[w])
        {
            uint64_t mask = oldval & m_externalDependencyBitmap[w];

            CTZ64(id, mask);

            uint64_t newval = oldval & ~(1LL << id);
            if (ATOMIC_CAS(&m_internalDependencyBitmap[w], oldval, newval) == oldval)
            {
                // we cleared the bit, process row
                processRow(w * 64 + id, threadId);
                return true;
            }
            // some other thread cleared the bit, try another bit
            oldval = m_internalDependencyBitmap[w];
        }
    }

    // made it through the bitmap without finding any enqueued rows
    return false;
}
}
