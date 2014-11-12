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
#include "slice.h"

namespace x265 {
// private namespace

class RDCost
{
public:

    /* all weights and factors stored as FIX8 */
    uint64_t  m_lambda2;
    uint64_t  m_lambda;
    uint64_t  m_cbDistortionWeight;
    uint64_t  m_crDistortionWeight;
    uint32_t  m_psyRdBase;
    uint32_t  m_psyRd;
    int       m_qp;

    void setPsyRdScale(double scale)                { m_psyRdBase = (uint32_t)floor(256.0 * scale * 0.33); }
    void setCbDistortionWeight(uint16_t weightFix8) { m_cbDistortionWeight = weightFix8; }
    void setCrDistortionWeight(uint16_t weightFix8) { m_crDistortionWeight = weightFix8; }

    void setQP(const Slice& slice, int qp)
    {
        m_qp = qp;

        /* Scale PSY RD factor by a slice type factor */
        static const uint32_t psyScaleFix8[3] = { 300, 256, 96 }; /* B, P, I */
        m_psyRd = (m_psyRdBase * psyScaleFix8[slice.m_sliceType]) >> 8;

        setLambda(x265_lambda2_tab[qp], x265_lambda_tab[qp]);

        int qpCb = Clip3(QP_MIN, QP_MAX_MAX, qp + slice.m_pps->chromaQpOffset[0]);
        int chroma_offset_idx = X265_MIN(qp - qpCb + 12, MAX_CHROMA_LAMBDA_OFFSET);
        uint16_t lambdaOffset = m_psyRd ? x265_chroma_lambda2_offset_tab[chroma_offset_idx] : 256;
        setCbDistortionWeight(lambdaOffset);

        int qpCr = Clip3(QP_MIN, QP_MAX_MAX, qp + slice.m_pps->chromaQpOffset[1]);
        chroma_offset_idx = X265_MIN(qp - qpCr + 12, MAX_CHROMA_LAMBDA_OFFSET);
        lambdaOffset = m_psyRd ? x265_chroma_lambda2_offset_tab[chroma_offset_idx] : 256;
        setCrDistortionWeight(lambdaOffset);
    }

    void setLambda(double lambda2, double lambda)
    {
        m_lambda2 = (uint64_t)floor(256.0 * lambda2);
        m_lambda = (uint64_t)floor(256.0 * lambda);
    }

    inline uint64_t calcRdCost(uint32_t distortion, uint32_t bits) const
    {
        X265_CHECK(bits <= (UINT64_MAX - 128) / m_lambda2,
                   "calcRdCost wrap detected dist: %d, bits %d, lambda: %d\n", distortion, bits, (int)m_lambda2);
        return distortion + ((bits * m_lambda2 + 128) >> 8);
    }

    /* return the difference in energy between the source block and the recon block */
    inline int psyCost(int size, pixel *source, intptr_t sstride, pixel *recon, intptr_t rstride) const
    {
        return primitives.psy_cost_pp[size](source, sstride, recon, rstride);
    }

    /* return the difference in energy between the source block and the recon block */
    inline int psyCost(int size, int16_t *source, intptr_t sstride, int16_t *recon, intptr_t rstride) const
    {
        return primitives.psy_cost_ss[size](source, sstride, recon, rstride);
    }

    /* return the RD cost of this prediction, including the effect of psy-rd */
    inline uint64_t calcPsyRdCost(uint32_t distortion, uint32_t bits, uint32_t psycost) const
    {
        return distortion + ((m_lambda * m_psyRd * psycost) >> 16) + ((bits * m_lambda2) >> 8);
    }

    inline uint64_t calcRdSADCost(uint32_t sadCost, uint32_t bits) const
    {
        X265_CHECK(bits <= (UINT64_MAX - 128) / m_lambda,
                   "calcRdSADCost wrap detected dist: %d, bits %d, lambda: "X265_LL"\n", sadCost, bits, m_lambda);
        return sadCost + ((bits * m_lambda + 128) >> 8);
    }

    inline uint32_t scaleChromaDistCb(uint32_t dist) const
    {
        X265_CHECK(dist <= (UINT64_MAX - 128) / m_cbDistortionWeight,
                   "scaleChromaDistCb wrap detected dist: %d, lambda: "X265_LL"\n", dist, m_cbDistortionWeight);
        return (uint32_t)(((dist * m_cbDistortionWeight) + 128) >> 8);
    }

    inline uint32_t scaleChromaDistCr(uint32_t dist) const
    {
        X265_CHECK(dist <= (UINT64_MAX - 128) / m_crDistortionWeight,
                   "scaleChromaDistCr wrap detected dist: %d, lambda: "X265_LL"\n", dist, m_crDistortionWeight);
        return (uint32_t)(((dist * m_crDistortionWeight) + 128) >> 8);
    }

    inline uint32_t getCost(uint32_t bits) const
    {
        return (uint32_t)((bits * m_lambda + 128) >> 8);
    }
};
}

#endif // ifndef X265_TCOMRDCOST_H
