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

/** \file     WeightPredAnalysis.h
    \brief    weighted prediction encoder class
*/
#ifndef X265_WEIGHTPREDANALYSIS_H
#define X265_WEIGHTPREDANALYSIS_H

#include "TLibCommon/TypeDef.h"
#include "TLibCommon/TComSlice.h"

namespace x265 {
// private namespace

class WeightPredAnalysis
{
    bool m_weighted_pred_flag;
    bool m_weighted_bipred_flag;
    wpScalingParam  m_wp[2][MAX_NUM_REF][3];

    Int64   xCalcDCValueSlice(TComSlice *slice, Pel *pels, int32_t *sample);
    Int64   xCalcACValueSlice(TComSlice *slice, Pel *pels, Int64 dc);
    Int64   xCalcDCValueUVSlice(TComSlice *slice, Pel *pels, int32_t *sample);
    Int64   xCalcACValueUVSlice(TComSlice *slice, Pel *pels, Int64 dc);
    Int64   xCalcSADvalueWPSlice(TComSlice *slice, Pel *orgPel, Pel *refPel, int denom, int inputWeight, int inputOffset);

    Int64   xCalcDCValue(Pel *pels, int width, int height, int stride);
    Int64   xCalcACValue(Pel *pels, int width, int height, int stride, Int64 iDC);
    Int64   xCalcSADvalueWP(int bitDepth, Pel *orgPel, Pel *refPel, int width, int height, int orgStride, int refStride, int denom, int inputWeight, int inputOffset);
    bool    xSelectWP(TComSlice * slice, wpScalingParam weightPredTable[2][MAX_NUM_REF][3], int denom);
    bool    xUpdatingWPParameters(TComSlice* slice, int log2Denom);

public:

    WeightPredAnalysis();

    // WP analysis :
    bool  xCalcACDCParamSlice(TComSlice *slice);
    bool  xEstimateWPParamSlice(TComSlice *slice);
    void  xStoreWPparam(bool weighted_pred_flag, bool weighted_bipred_flag);
    void  xRestoreWPparam(TComSlice *slice);
    void  xCheckWPEnable(TComSlice *slice);
};
}
#endif // ifndef X265_WEIGHTPREDANALYSIS_H
