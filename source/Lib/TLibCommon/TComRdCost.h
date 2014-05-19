/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2013, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/** \file     TComRdCost.h
    \brief    RD cost computation classes (header)
*/

#ifndef X265_TCOMRDCOST_H
#define X265_TCOMRDCOST_H

#include "CommonDef.h"
#include "common.h"

//! \ingroup TLibCommon
//! \{

namespace x265 {
// private namespace

class TComPattern;

/// RD cost computation class
class TComRdCost
{
private:

    uint64_t  m_lambdaMotionSSE;  // m_lambda2 w/ 8 bits of fraction

    uint64_t  m_lambdaMotionSAD;  // m_lambda w/ 8 bits of fraction

    uint64_t  m_cbDistortionWeight;

    uint64_t  m_crDistortionWeight;

    uint64_t  m_psyRdScale;            // Psy RD strength w/ 8 bits of fraction

public:

    static const pixel zeroPel[MAX_CU_SIZE * MAX_CU_SIZE];

    void setLambda(double lambda2, double lambda)
    {
        m_lambdaMotionSSE = (uint64_t)floor(256.0 * lambda2);
        m_lambdaMotionSAD = (uint64_t)floor(256.0 * lambda);
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
        X265_CHECK(abs((float)((bits * m_lambdaMotionSSE + 128) >> 8) -
                       (float)bits * m_lambdaMotionSSE / 256.0) < 2,
                   "calcRdCost wrap detected dist: %d, bits %d, lambda: %d\n", distortion, bits, (int)m_lambdaMotionSSE);
        return distortion + ((bits * m_lambdaMotionSSE + 128) >> 8);
    }

    /* return the difference in energy between the source block and the recon block */
    inline uint32_t psyCost(int size, pixel *source, intptr_t sstride, pixel *recon, intptr_t rstride)
    {
        int pixelCountShift = (size + 2) * 2 - 1; // width * height / satd-scale(2)
        int sdc = primitives.sad_square[size](source, sstride, (pixel*)zeroPel, MAX_CU_SIZE) >> pixelCountShift;
        int sEnergy = primitives.sa8d[size](source, sstride, (pixel*)zeroPel, MAX_CU_SIZE) - sdc;

        int rdc = primitives.sad_square[size](recon, rstride, (pixel*)zeroPel, MAX_CU_SIZE) >> pixelCountShift;
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
        X265_CHECK(abs((float)((tot * m_lambdaMotionSSE + 128) >> 8) -
                       (float)tot * m_lambdaMotionSSE / 256.0) < 2,
                   "calcPsyRdCost wrap detected tot: "X265_LL", lambda: "X265_LL"\n", tot, m_lambdaMotionSSE);
#endif
        return distortion + ((tot * m_lambdaMotionSSE + 128) >> 8);
    }

    inline uint64_t calcRdSADCost(uint32_t sadCost, uint32_t bits)
    {
        X265_CHECK(abs((float)((bits * m_lambdaMotionSAD + 128) >> 8) -
                       (float)bits * m_lambdaMotionSAD / 256.0) < 2,
                   "calcRdSADCost wrap detected dist: %d, bits %d, lambda: "X265_LL"\n", sadCost, bits, m_lambdaMotionSAD);
        return sadCost + ((bits * m_lambdaMotionSAD + 128) >> 8);
    }

    inline uint32_t getCost(uint32_t bits)
    {
        return (uint32_t)((bits * m_lambdaMotionSAD + 128) >> 8);
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
