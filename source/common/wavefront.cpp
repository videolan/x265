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

namespace x265 {
// x265 private namespace

bool WaveFront::init(int numRows)
{
    m_numRows = numRows;

    if (m_pool)
    {
        m_numWords = (numRows + 63) >> 6;
        m_queuedBitmap = new uint64_t[m_numWords];
        if (m_queuedBitmap)
            memset((void*)m_queuedBitmap, 0, sizeof(uint64_t) * m_numWords);

        m_enableBitmap = new uint64_t[m_numWords];
        if (m_enableBitmap)
            memset((void*)m_enableBitmap, 0, sizeof(uint64_t) * m_numWords);

        return m_queuedBitmap && m_enableBitmap;
    }

    return false;
}

WaveFront::~WaveFront()
{
    delete[] m_queuedBitmap;
    delete[] m_enableBitmap;
}

void WaveFront::clearEnabledRowMask()
{
    memset((void*)m_enableBitmap, 0, sizeof(uint64_t) * m_numWords);
}

void WaveFront::enqueueRow(int row)
{
    // thread safe
    uint64_t bit = 1LL << (row & 63);

    assert(row < m_numRows);
    ATOMIC_OR(&m_queuedBitmap[row >> 6], bit);
    m_pool->pokeIdleThread();
}

void WaveFront::enableRow(int row)
{
    // thread safe
    uint64_t bit = 1LL << (row & 63);

    assert(row < m_numRows);
    ATOMIC_OR(&m_enableBitmap[row >> 6], bit);
}

bool WaveFront::checkHigherPriorityRow(int curRow)
{
    int fullwords = curRow >> 6;
    uint64_t mask = (1LL << (curRow & 63)) - 1;

    // Check full bitmap words before curRow
    for (int i = 0; i < fullwords; i++)
    {
        if (m_queuedBitmap[i])
            return true;
    }

    // check the partially masked bitmap word of curRow
    if (m_queuedBitmap[fullwords] & mask)
        return true;
    return false;
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
            uint64_t mask = m_queuedBitmap[w] & m_enableBitmap[w];
            if (mask == 0) // race condition
                break;

            CTZ64(id, mask);

            uint64_t newval = oldval & ~(1LL << id);
            if (ATOMIC_CAS(&m_queuedBitmap[w], oldval, newval) == oldval)
            {
                // we cleared the bit, process row
                processRow(w * 64 + id);
                return true;
            }
            // some other thread cleared the bit, try another bit
        }
    }

    // made it through the bitmap without finding any enqueued rows
    return false;
}
}
