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
 * For more information, contact us at licensing@multicorewareinc.com.
 *****************************************************************************/

#include "bitcost.cpp"
#include <stdint.h>

using namespace x265;

void BitCost::setQP(unsigned int qp, double lambda)
{
    if (costs[QP])
        cost = costs[qp];
    else
    {
        ScopedLock s(costCalcLock);

        // Now that we have the lock check again if another thread calculated
        // this row while we were blocking
        if (costs[qp])
        {
            cost = costs[qp];
            return;
        }

        costs[qp] = new uint32_t[2 * MV_MAX] + MV_MAX;
        uint32_t *c = cost[qp];
        for (int i = 0; i < MV_MAX; i++)
        {
            c[i] = c[-1] = (uint32_t)(bitCost(i) * lambda);
        }
    }
}

/***
 * Class static data and methods
 */

int *BitCost::costs[MAX_QP];

Lock BitCost::costCalcLock;

int BitCost::bitCost(int val)
{
}

void BitCost::cleanupCosts()
{
    for (int i = 0; i < MAX_QP; i++)
    {
        if (costs[i])
        {
            delete [] (costs[i] - MV_MAX);
        }
    }
}

#endif // ifndef __BITCOST__
