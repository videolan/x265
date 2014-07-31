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

#ifndef X265_RDCOST_H
#define X265_RDCOST_H

#include "common.h"

namespace x265 {
// private namespace

class RDCost
{
private:

    uint64_t  m_lambdaSSE;  // m_lambda2 w/ 8 bits of fraction

    uint64_t  m_lambdaSAD;  // m_lambda w/ 8 bits of fraction

    uint64_t  m_cbDistortionWeight;

    uint64_t  m_crDistortionWeight;

public:

    uint32_t  m_psyRd;

    void setPsyRdScale(double scale)                { m_psyRd = (uint32_t)floor(256.0 * scale); }
    void setCbDistortionWeight(uint16_t weightFix8) { m_cbDistortionWeight = weightFix8; }
    void setCrDistortionWeight(uint16_t weightFix8) { m_crDistortionWeight = weightFix8; }

    void setLambda(double lambda2, double lambda)
    {
        m_lambdaSSE = (uint64_t)floor(256.0 * lambda2);
        m_lambdaSAD = (uint64_t)floor(256.0 * lambda);
    }

    inline uint64_t calcRdCost(uint32_t distortion, uint32_t bits)
    {
        X265_CHECK(bits <= (UINT64_MAX - 128) / m_lambdaSSE,
                   "calcRdCost wrap detected dist: %d, bits %d, lambda: %d\n", distortion, bits, (int)m_lambdaSSE);
        return distortion + ((bits * m_lambdaSSE + 128) >> 8);
    }

    /* return the difference in energy between the source block and the recon block */
    inline int psyCost(int size, pixel *source, intptr_t sstride, pixel *recon, intptr_t rstride)
    {
        return primitives.psy_cost[size](source, sstride, recon, rstride);
    }

    /* return the RD cost of this prediction, including the effect of psy-rd */
    inline uint64_t calcPsyRdCost(uint32_t distortion, uint32_t bits, uint32_t psycost)
    {
        return distortion + ((m_lambdaSAD * m_psyRd * psycost) >> 16) + ((bits * m_lambdaSSE) >> 8);
    }

    inline uint64_t calcRdSADCost(uint32_t sadCost, uint32_t bits)
    {
        X265_CHECK(bits <= (UINT64_MAX - 128) / m_lambdaSAD,
                   "calcRdSADCost wrap detected dist: %d, bits %d, lambda: "X265_LL"\n", sadCost, bits, m_lambdaSAD);
        return sadCost + ((bits * m_lambdaSAD + 128) >> 8);
    }

    inline uint32_t scaleChromaDistCb(uint32_t dist)
    {
        X265_CHECK(dist <= (UINT64_MAX - 128) / m_cbDistortionWeight,
                   "scaleChromaDistCb wrap detected dist: %d, lambda: "X265_LL"\n", dist, m_cbDistortionWeight);
        return (uint32_t)(((dist * m_cbDistortionWeight) + 128) >> 8);
    }

    inline uint32_t scaleChromaDistCr(uint32_t dist)
    {
        X265_CHECK(dist <= (UINT64_MAX - 128) / m_crDistortionWeight,
                   "scaleChromaDistCr wrap detected dist: %d, lambda: "X265_LL"\n", dist, m_crDistortionWeight);
        return (uint32_t)(((dist * m_crDistortionWeight) + 128) >> 8);
    }

    inline uint32_t getCost(uint32_t bits)
    {
        return (uint32_t)((bits * m_lambdaSAD + 128) >> 8);
    }
};
}
//! \}

#endif // ifndef X265_TCOMRDCOST_H
