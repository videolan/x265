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

    BitCost() : m_cost_mvx(0), m_cost_mvy(0), m_cost(0) {}

    void setQP(unsigned int qp, double lambda);

    void setMVP(const MV& mvp)                      { m_mvp = mvp; m_cost_mvx = m_cost - mvp.x; m_cost_mvy = m_cost - mvp.y; }

    // return bit cost of motion vector difference, multiplied by lambda
    inline uint16_t mvcost(const MV& mv) const      { return m_cost_mvx[mv.x] + m_cost_mvy[mv.y]; }

    // return bit cost of motion vector difference, without lambda
    inline uint16_t bitcost(const MV& mv) const
    {
        return s_bitsizes[(abs(mv.x - m_mvp.x) << 1) + !!(mv.x < m_mvp.x)] +
               s_bitsizes[(abs(mv.y - m_mvp.y) << 1) + !!(mv.y < m_mvp.y)];
    }

    static void destroy();

protected:

    uint16_t *m_cost_mvx;

    uint16_t *m_cost_mvy;

    uint16_t *m_cost;

    MV        m_mvp;

    BitCost& operator =(const BitCost&);

private:

    static const int BC_MAX_MV = 0x8000;

    static const int BC_MAX_QP = 82;

    static uint16_t *s_bitsizes;

    static uint16_t *s_costs[BC_MAX_QP];

    static Lock s_costCalcLock;

    static void CalculateLogs();
};
}

#endif // ifndef __BITCOST__
