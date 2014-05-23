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

    uint64_t  m_psyRdScale; // Psy RD strength w/ 8 bits of fraction

public:

    static const pixel zeroPel[MAX_CU_SIZE * MAX_CU_SIZE];

    void setLambda(double lambda2, double lambda)
    {
        m_lambdaSSE = (uint64_t)floor(256.0 * lambda2);
        m_lambdaSAD = (uint64_t)floor(256.0 * lambda);
    }

    void setCbDistortionWeight(double cbDistortionWeight)
    {
        m_cbDistortionWeight = (uint64_t)floor(256.0 * cbDistortionWeight);
    }

    void setCrDistortionWeight(double crDistortionWeight)
    {
        m_crDistortionWeight = (uint64_t)floor(256.0 * crDistortionWeight);
    }

    void setPsyRdScale(double scale)
    {
        m_psyRdScale = (uint64_t)floor(256.0 * scale);
    }

    inline bool psyRdEnabled() const
    {
        return !!m_psyRdScale;
    }

    inline uint64_t calcRdCost(uint32_t distortion, uint32_t bits)
    {
        X265_CHECK(abs((float)((bits * m_lambdaSSE + 128) >> 8) -
                       (float)bits * m_lambdaSSE / 256.0) < 2,
                   "calcRdCost wrap detected dist: %d, bits %d, lambda: %d\n", distortion, bits, (int)m_lambdaSSE);
        return distortion + ((bits * m_lambdaSSE + 128) >> 8);
    }

    /* return the difference in energy between the source block and the recon block */
    inline uint32_t psyCost(int size, pixel *source, intptr_t sstride, pixel *recon, intptr_t rstride)
    {
        int sdc = primitives.sad_square[size](source, sstride, (pixel*)zeroPel, MAX_CU_SIZE) >> 2;
        int sEnergy = primitives.sa8d[size](source, sstride, (pixel*)zeroPel, MAX_CU_SIZE) - sdc;

        int rdc = primitives.sad_square[size](recon, rstride, (pixel*)zeroPel, MAX_CU_SIZE) >> 2;
        int rEnergy = primitives.sa8d[size](recon, rstride, (pixel*)zeroPel, MAX_CU_SIZE) - rdc;

        X265_CHECK(sdc <= sEnergy && rdc <= rEnergy, "DC component of energy is more than total cost\n")
        return abs(sEnergy - rEnergy);
    }

    /* return the RD cost of this prediction, including the effect of psy-rd */
    inline uint64_t calcPsyRdCost(uint32_t distortion, uint32_t bits, uint32_t psycost)
    {
        uint64_t tot = bits + (((psycost * m_psyRdScale) + 128) >> 8);
#if CHECKED_BUILD || _DEBUG
        x265_emms();
        X265_CHECK(abs((float)((tot * m_lambdaSSE + 128) >> 8) -
                       (float)tot * m_lambdaSSE / 256.0) < 2,
                   "calcPsyRdCost wrap detected tot: "X265_LL", lambda: "X265_LL"\n", tot, m_lambdaSSE);
#endif
        return distortion + ((tot * m_lambdaSSE + 128) >> 8);
    }

    inline uint64_t calcRdSADCost(uint32_t sadCost, uint32_t bits)
    {
        X265_CHECK(abs((float)((bits * m_lambdaSAD + 128) >> 8) -
                       (float)bits * m_lambdaSAD / 256.0) < 2,
                   "calcRdSADCost wrap detected dist: %d, bits %d, lambda: "X265_LL"\n", sadCost, bits, m_lambdaSAD);
        return sadCost + ((bits * m_lambdaSAD + 128) >> 8);
    }

    inline uint32_t getCost(uint32_t bits)
    {
        return (uint32_t)((bits * m_lambdaSAD + 128) >> 8);
    }

    inline uint32_t scaleChromaDistCb(uint32_t dist)
    {
        X265_CHECK(abs((float)((dist * m_cbDistortionWeight + 128) >> 8) -
                       (float)dist * m_cbDistortionWeight / 256.0) < 2,
                   "scaleChromaDistCb wrap detected dist: %d, lambda: "X265_LL"\n", dist, m_cbDistortionWeight);
        return (uint32_t)(((dist * m_cbDistortionWeight) + 128) >> 8);
    }

    inline uint32_t scaleChromaDistCr(uint32_t dist)
    {
        X265_CHECK(abs((float)((dist * m_crDistortionWeight + 128) >> 8) -
                       (float)dist * m_crDistortionWeight / 256.0) < 2,
                   "scaleChromaDistCr wrap detected dist: %d, lambda: "X265_LL"\n", dist, m_crDistortionWeight);
        return (uint32_t)(((dist * m_crDistortionWeight) + 128) >> 8);
    }
};
}
//! \}

#endif // ifndef X265_TCOMRDCOST_H
