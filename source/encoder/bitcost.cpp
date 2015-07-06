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

#include "common.h"
#include "primitives.h"
#include "bitcost.h"

using namespace X265_NS;

void BitCost::setQP(unsigned int qp)
{
    if (!s_costs[qp])
    {
        ScopedLock s(s_costCalcLock);

        // Now that we have acquired the lock, check again if another thread calculated
        // this row while we were blocked
        if (!s_costs[qp])
        {
            x265_emms(); // just to be safe

            CalculateLogs();
            s_costs[qp] = new uint16_t[4 * BC_MAX_MV + 1] + 2 * BC_MAX_MV;
            double lambda = x265_lambda_tab[qp];

            // estimate same cost for negative and positive MVD
            for (int i = 0; i <= 2 * BC_MAX_MV; i++)
                s_costs[qp][i] = s_costs[qp][-i] = (uint16_t)X265_MIN(s_bitsizes[i] * lambda + 0.5f, (1 << 15) - 1);
        }
    }

    m_cost = s_costs[qp];
}

/***
 * Class static data and methods
 */

uint16_t *BitCost::s_costs[BC_MAX_QP];

float *BitCost::s_bitsizes;

Lock BitCost::s_costCalcLock;

void BitCost::CalculateLogs()
{
    if (!s_bitsizes)
    {
        s_bitsizes = new float[2 * BC_MAX_MV + 1];
        s_bitsizes[0] = 0.718f;
        float log2_2 = 2.0f / log(2.0f);  // 2 x 1/log(2)
        for (int i = 1; i <= 2 * BC_MAX_MV; i++)
            s_bitsizes[i] = log((float)(i + 1)) * log2_2 + 1.718f;
    }
}

void BitCost::destroy()
{
    for (int i = 0; i < BC_MAX_QP; i++)
    {
        if (s_costs[i])
        {
            delete [] (s_costs[i] - 2 * BC_MAX_MV);

            s_costs[i] = 0;
        }
    }

    delete [] s_bitsizes;
    s_bitsizes = 0;
}
