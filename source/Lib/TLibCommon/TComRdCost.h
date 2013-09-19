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
#include <math.h>

//! \ingroup TLibCommon
//! \{

namespace x265 {
// private namespace

class TComPattern;

/// RD cost computation class
class TComRdCost
{
private:

    double                  m_lambda2;

    double                  m_lambda;

    UInt64                  m_lambdaMotionSSE;  // m_lambda2 w/ 16 bits of fraction

    UInt64                  m_lambdaMotionSAD;  // m_lambda w/ 16 bits of fraction

    UInt                    m_cbDistortionWeight;

    UInt                    m_crDistortionWeight;

public:

    void setLambda(double lambda)
    {
        m_lambda2         = lambda;
        m_lambda          = sqrt(m_lambda2);
        m_lambdaMotionSAD = (UInt64)floor(65536.0 * m_lambda);
        m_lambdaMotionSSE = (UInt64)floor(65536.0 * m_lambda2);
    }

    void setCbDistortionWeight(double cbDistortionWeight)
    {
        m_cbDistortionWeight = (UInt)floor(256.0 * cbDistortionWeight);
    }

    void setCrDistortionWeight(double crDistortionWeight)
    {
        m_crDistortionWeight = (UInt)floor(256.0 * crDistortionWeight);
    }

    inline UInt64  calcRdCost(UInt distortion, UInt bits) { return distortion + ((bits * m_lambdaMotionSSE + 32768) >> 16); }

    inline UInt64  calcRdSADCost(UInt sadCost, UInt bits) { return sadCost + ((bits * m_lambdaMotionSAD + 32768) >> 16); }

    inline UInt    getCost(UInt bits)                     { return (UInt)((bits * m_lambdaMotionSAD + 32768) >> 16); }

    inline UInt    scaleChromaDistCb(UInt dist)           { return ((dist * m_cbDistortionWeight) + 128) >> 8; }

    inline UInt    scaleChromaDistCr(UInt dist)           { return ((dist * m_crDistortionWeight) + 128) >> 8; }

    inline double  getSADLambda() const                   { return m_lambda; }
};
}
//! \}

#endif // ifndef X265_TCOMRDCOST_H
