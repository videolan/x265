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

#include "bitcost.h"
#include <stdint.h>
#include <algorithm>
#include <math.h>

using namespace x265;

void BitCost::setQP(unsigned int qp, double lambda)
{
    if (!costs[qp])
    {
        ScopedLock s(costCalcLock);

        // Now that we have acquired the lock, check again if another thread calculated
        // this row while we were blocked
        if (!costs[qp])
        {
            CalculateLogs();
            costs[qp] = new uint16_t[2 * BC_MAX_MV] + BC_MAX_MV;
            uint16_t *c = costs[qp];
            const uint16_t max16 = (1 << 16)-1;

#if X264_APPROACH
            // estimate same cost for negative and positive MVD
            for (int i = 0; i < BC_MAX_MV; i++)
                c[i] = c[-i] = std::min<uint16_t>(max16, logs[i] * lambda + 0.5);
#else
            // penalize negative MVD by one bit
            c[0] = (uint16_t)lambda;
            for (int i = 1; i < BC_MAX_MV; i++)
            {
                c[i]  = std::min<uint16_t>(max16, (uint16_t)(logs[i<<1] * lambda + 0.5));
                c[-i] = std::min<uint16_t>(max16, (uint16_t)(logs[(i<<1)+1] * lambda + 0.5));
            }
#endif
        }
    }

    cost = costs[qp];
}

/***
 * Class static data and methods
 */

uint16_t *BitCost::costs[BC_MAX_QP];

float *BitCost::logs;

Lock BitCost::costCalcLock;

void BitCost::CalculateLogs()
{
    if (!logs)
    {
        logs = new float[2*BC_MAX_MV + 1];
        logs[0] = 0.718f;
#if X264_APPROACH
        float log2_2 = (float)(2.0/log(2.0));  // 2 x 1/log(2)
        for( int i = 1; i <= 2*BC_MAX_MV; i++ )
            logs[i] = log((float)(i+1)) * log2_2 + 1.718f;
#else
        logs[1] = 0.718f;
        float log2_2 = (float)(2.0/log(2.0));  // 2 x 1/log(2)
        for( int i = 2; i <= 2*BC_MAX_MV; i++ )
            logs[i] = log((float)(i+1)) * log2_2 + 0.718f;
#endif
    }
}

void BitCost::destroy()
{
    for (int i = 0; i < BC_MAX_QP; i++)
    {
        if (costs[i])
        {
            delete [] (costs[i] - BC_MAX_MV);

            costs[i] = 0;
        }
    }
    if (logs)
    {
        delete [] logs;

        logs = 0;
    }
}
