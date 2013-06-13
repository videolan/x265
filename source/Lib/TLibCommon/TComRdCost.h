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

#ifndef __TCOMRDCOST__
#define __TCOMRDCOST__

#include "CommonDef.h"
#include "TComPattern.h"
#include "TComMv.h"

#include "TComSlice.h"
#include "TComRdCostWeightPrediction.h"

//! \ingroup TLibCommon
//! \{

/// distortion function index
enum DFunc
{
    DF_DEFAULT  = 0,
    DF_SSE      = 1,    ///< general size SSE
    DF_SSE4     = 2,    ///<   4xM SSE
    DF_SSE8     = 3,    ///<   8xM SSE
    DF_SSE16    = 4,    ///<  16xM SSE
    DF_SSE32    = 5,    ///<  32xM SSE
    DF_SSE64    = 6,    ///<  64xM SSE
    DF_SSE16N   = 7,    ///< 16NxM SSE

    DF_SAD      = 8,    ///< general size SAD
    DF_SAD4     = 9,    ///<   4xM SAD
    DF_SAD8     = 10,   ///<   8xM SAD
    DF_SAD16    = 11,   ///<  16xM SAD
    DF_SAD32    = 12,   ///<  32xM SAD
    DF_SAD64    = 13,   ///<  64xM SAD
    DF_SAD16N   = 14,   ///< 16NxM SAD

    DF_SADS     = 15,   ///< general size SAD with step
    DF_SADS4    = 16,   ///<   4xM SAD with step
    DF_SADS8    = 17,   ///<   8xM SAD with step
    DF_SADS16   = 18,   ///<  16xM SAD with step
    DF_SADS32   = 19,   ///<  32xM SAD with step
    DF_SADS64   = 20,   ///<  64xM SAD with step
    DF_SADS16N  = 21,   ///< 16NxM SAD with step

    DF_HADS     = 22,   ///< general size Hadamard with step
    DF_HADS4    = 23,   ///<   4xM HAD with step
    DF_HADS8    = 24,   ///<   8xM HAD with step
    DF_HADS16   = 25,   ///<  16xM HAD with step
    DF_HADS32   = 26,   ///<  32xM HAD with step
    DF_HADS64   = 27,   ///<  64xM HAD with step
    DF_HADS16N  = 28,   ///< 16NxM HAD with step

    DF_SAD12    = 43,
    DF_SAD24    = 44,
    DF_SAD48    = 45,

    DF_SADS12   = 46,
    DF_SADS24   = 47,
    DF_SADS48   = 48,

    DF_SSE_FRAME = 50   ///< Frame-based SSE
};

class DistParam;
class TComPattern;

// ====================================================================================================================
// Type definition
// ====================================================================================================================

// for function pointer
typedef UInt (*FpDistFunc) (DistParam*);

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// distortion parameter class
class DistParam
{
public:

    Pel*  pOrg;
    Pel*  pCur;
    Int   iStrideOrg;
    Int   iStrideCur;
    Int   iRows;
    Int   iCols;
    Int   iStep;
    FpDistFunc DistFunc;
    Int   bitDepth;

    Bool            bApplyWeight;   // whether weithed prediction is used or not
    wpScalingParam  *wpCur;         // weithed prediction scaling parameters for current ref
    UInt            uiComp;         // uiComp = 0 (luma Y), 1 (chroma U), 2 (chroma V)

    // (vertical) subsampling shift (for reducing complexity)
    // - 0 = no subsampling, 1 = even rows, 2 = every 4th, etc.
    Int   iSubShift;

    DistParam()
    {
        pOrg = NULL;
        pCur = NULL;
        iStrideOrg = 0;
        iStrideCur = 0;
        iRows = 0;
        iCols = 0;
        iStep = 1;
        DistFunc = NULL;
        iSubShift = 0;
        bitDepth = 0;
    }
};

class DistParamSSE : public DistParam
{
public:

    Short* ptr1;
    Short* ptr2;
};

/// RD cost computation class
class TComRdCost : public TComRdCostWeightPrediction
{
private:

    FpDistFunc              m_afpDistortFunc[64]; // [eDFunc]

    Double                  m_dLambda;

    Double                  m_sqrtLambda;

    UInt64                  m_uiLambdaMotionSSE;  // m_dLambda w/ 16 bits of fraction

    UInt64                  m_uiLambdaMotionSAD;  // m_sqrtLambda w/ 16 bits of fraction

    UInt                    m_cbDistortionWeight;

    UInt                    m_crDistortionWeight;

public:

    TComMv                  m_mvPredictor;
    Int                     m_iCostScale;

    TComRdCost();

    virtual ~TComRdCost();

    Void    setLambda(Double dLambda);

    Void    setCbDistortionWeight(Double cbDistortionWeight) { m_cbDistortionWeight = (UInt)floor(256.0 * cbDistortionWeight); }

    Void    setCrDistortionWeight(Double crDistortionWeight) { m_crDistortionWeight = (UInt)floor(256.0 * crDistortionWeight); }

    inline UInt64  calcRdCost(UInt distortion, UInt bits) { return distortion + ((bits * m_uiLambdaMotionSSE + 32768) >> 16); }

    inline UInt64  calcRdSADCost(UInt sadCost, UInt bits) { return sadCost + ((bits * m_uiLambdaMotionSAD + 32768) >> 16); }

    inline UInt    scaleChromaDistCb(UInt dist) { return ((dist * m_cbDistortionWeight) + 128) >> 8; }

    inline UInt    scaleChromaDistCr(UInt dist) { return ((dist * m_crDistortionWeight) + 128) >> 8; }

    Double  getSqrtLambda()                    { return m_sqrtLambda; }

    Double  getLambda()                        { return m_dLambda; }

    // for motion cost
    Void    setPredictor(TComMv& rcMv)         { m_mvPredictor = rcMv; }

    Void    setCostScale(Int iCostScale)       { m_iCostScale = iCostScale; }

    UInt    getCost(UInt b)                    { return (UInt)((m_uiLambdaMotionSAD * b + 32768) >> 16); }

    // Distortion Functions
    Void    init();

    Void    setDistParam(UInt uiBlkWidth, UInt uiBlkHeight, DFunc eDFunc, DistParam& rcDistParam);
    Void    setDistParam(TComPattern* pcPatternKey, Pel* piRefY, Int iRefStride,            DistParam& rcDistParam);
    Void    setDistParam(TComPattern* pcPatternKey, Pel* piRefY, Int iRefStride, Int iStep, DistParam& rcDistParam, Bool bHADME = false);
    Void    setDistParam(DistParam& rcDP, Int bitDepth, Pel* p1, Int iStride1, Pel* p2, Int iStride2, Int iWidth, Int iHeight, Bool bHadamard = false);

    UInt    calcHAD(Int bitDepth, Pel* pi0, Int iStride0, Pel* pi1, Int iStride1, Int iWidth, Int iHeight);

private:

    static UInt xGetSSE(DistParam* pcDtParam);
    static UInt xGetSSE4(DistParam* pcDtParam);
    static UInt xGetSSE8(DistParam* pcDtParam);
    static UInt xGetSSE16(DistParam* pcDtParam);
    static UInt xGetSSE32(DistParam* pcDtParam);
    static UInt xGetSSE64(DistParam* pcDtParam);
    static UInt xGetSSE16N(DistParam* pcDtParam);

    static UInt xGetSAD(DistParam* pcDtParam);
    static UInt xGetSAD4(DistParam* pcDtParam);
    static UInt xGetSAD8(DistParam* pcDtParam);
    static UInt xGetSAD16(DistParam* pcDtParam);
    static UInt xGetSAD32(DistParam* pcDtParam);
    static UInt xGetSAD64(DistParam* pcDtParam);
    static UInt xGetSAD16N(DistParam* pcDtParam);

    static UInt xGetSAD12(DistParam* pcDtParam);
    static UInt xGetSAD24(DistParam* pcDtParam);
    static UInt xGetSAD48(DistParam* pcDtParam);

    static UInt xGetHADs4(DistParam* pcDtParam);
    static UInt xGetHADs8(DistParam* pcDtParam);
    static UInt xGetHADs(DistParam* pcDtParam);

    static UInt xCalcHADs4x4(Pel *piOrg, Pel *piCurr, Int iStrideOrg, Int iStrideCur, Int iStep);
    static UInt xCalcHADs8x8(Pel *piOrg, Pel *piCurr, Int iStrideOrg, Int iStrideCur, Int iStep);
}; // END CLASS DEFINITION TComRdCost

//! \}

#endif // __TCOMRDCOST__
