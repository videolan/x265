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

#ifndef __BITCOST__
#define __BITCOST__

#include "threading.h"
#include "mv.h"

#include <stdint.h>

namespace x265 {
// private x265 namespace

class BitCost
{
public:

    void setMVP(const MV& mvp)               { cost_mvx = cost - mvp.x; cost_mvy = cost - mvp.y; }

    // return bit cost of absolute motion vector
    uint32_t mvcost(const MV& mv) const      { return cost_mvx[mv.x] + cost_mvy[mv.y]; }

    void setQP(unsigned int qp, double lambda);

    static void cleanupCosts();

protected:

    uint32_t *cost_mvx;

    uint32_t *cost_mvy;

    uint32_t *cost;

    BitCost& operator=(const BitCost&);

private:

    static const int BC_MAX_MV = 0x8000;

    static const int BC_MAX_QP = 51;

    static uint32_t *costs[BC_MAX_QP];

    static Lock costCalcLock;

    static uint32_t bitCost(int val);
};
}

#endif // ifndef __BITCOST__
