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

/** \file     TComTrQuant.h
    \brief    transform and quantization class (header)
*/

#ifndef __TCOMTRQUANT__
#define __TCOMTRQUANT__

#include "CommonDef.h"
#include "TComYuv.h"
#include "TComDataCU.h"
#include "ContextTables.h"

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Constants
// ====================================================================================================================

#define QP_BITS                 15

// ====================================================================================================================
// Type definition
// ====================================================================================================================

typedef struct
{
    Int significantCoeffGroupBits[NUM_SIG_CG_FLAG_CTX][2];
    Int significantBits[NUM_SIG_FLAG_CTX][2];
    Int lastXBits[32];
    Int lastYBits[32];
    Int greaterOneBits[NUM_ONE_FLAG_CTX][2];
    Int levelAbsBits[NUM_ABS_FLAG_CTX][2];

    Int blockCbpBits[3 * NUM_QT_CBF_CTX][2];
    Int blockRootCbpBits[4][2];
} estBitsSbacStruct;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

class QpParam
{
public:

    QpParam() {}

    Int m_iQP;
    Int m_iPer;
    Int m_iRem;

public:

    Int m_iBits;

    Void setQpParam(Int qpScaled)
    {
        m_iQP   = qpScaled;
        m_iPer  = qpScaled / 6;
        m_iRem  = qpScaled % 6;
        m_iBits = QP_BITS + m_iPer;
    }

    Void clear()
    {
        m_iQP   = 0;
        m_iPer  = 0;
        m_iRem  = 0;
        m_iBits = 0;
    }

    Int per()   const { return m_iPer; }

    Int rem()   const { return m_iRem; }

    Int bits()  const { return m_iBits; }

    Int qp() { return m_iQP; }
};

/// transform and quantization class
class TComTrQuant
{
public:

    TComTrQuant();
    ~TComTrQuant();

    // initialize class
    Void init(UInt uiMaxTrSize, Bool useRDOQ, Bool useRDOQTS, Bool bEnc, Bool useTransformSkipFast, Bool bUseAdaptQpSelect);

    // transform & inverse transform functions
    UInt transformNxN(TComDataCU* cu, Short* residual, UInt stride, TCoeff* coeff, Int* arlCoeff, UInt width, UInt height,
                      TextType ttype, UInt absPartIdx, Bool useTransformSkip = false);

    Void invtransformNxN(Bool transQuantBypass, TextType ttype, UInt mode, Short* residual, UInt stride, TCoeff* coeff,
                         UInt width, UInt height, Int scalingListType, Bool useTransformSkip = false);

    Void invRecurTransformNxN(TComDataCU* cu, UInt absPartIdx, TextType ttype, Short* residual, UInt addr, UInt stride,
                              UInt width, UInt height, UInt maxTrMode, UInt trMode, TCoeff* coeff);

    // Misc functions
    Void setQPforQuant(Int qpy, TextType ttype, Int qpBdOffset, Int chromaQPOffset);

    Void setLambda(Double dLambdaLuma, Double dLambdaChroma) { m_dLambdaLuma = dLambdaLuma; m_dLambdaChroma = dLambdaChroma; }

    Void selectLambda(TextType ttype) { m_dLambda = (ttype == TEXT_LUMA) ? m_dLambdaLuma : m_dLambdaChroma; }

    Void setRDOQOffset(UInt uiRDOQOffset) { m_uiRDOQOffset = uiRDOQOffset; }

    Void initScalingList();
    Void destroyScalingList();
    Void setErrScaleCoeff(UInt list, UInt size, UInt qp);
    Double* getErrScaleCoeff(UInt list, UInt size, UInt qp) { return m_errScale[size][list][qp]; }   //!< get Error Scale Coefficent

    Int* getQuantCoeff(UInt list, UInt qp, UInt size) { return m_quantCoef[size][list][qp]; }        //!< get Quant Coefficent

    Int* getDequantCoeff(UInt list, UInt qp, UInt size) { return m_dequantCoef[size][list][qp]; }    //!< get DeQuant Coefficent

    Void setUseScalingList(Bool bUseScalingList) { m_scalingListEnabledFlag = bUseScalingList; }

    Bool getUseScalingList() { return m_scalingListEnabledFlag; }

    Void setFlatScalingList();
    Void xsetFlatScalingList(UInt list, UInt size, UInt qp);
    Void xSetScalingListEnc(TComScalingList *scalingList, UInt list, UInt size, UInt qp);
    Void xSetScalingListDec(TComScalingList *scalingList, UInt list, UInt size, UInt qp);
    Void setScalingList(TComScalingList *scalingList);
    Void setScalingListDec(TComScalingList *scalingList);
    Void processScalingListEnc(Int *coeff, Int *quantcoeff, Int quantScales, UInt height, UInt width, UInt ratio, Int sizuNum, UInt dc);
    Void processScalingListDec(Int *coeff, Int *dequantcoeff, Int invQuantScales, UInt height, UInt width, UInt ratio, Int sizuNum, UInt dc);
    Void initSliceQpDelta();
    Void storeSliceQpNext(TComSlice* pcSlice);
    Void clearSliceARLCnt();

    Int     getQpDelta(Int qp) { return m_qpDelta[qp]; }

    Int*    getSliceNSamples() { return m_sliceNsamples; }

    Double* getSliceSumC()     { return m_sliceSumC; }

    static Int  calcPatternSigCtx(const UInt* sigCoeffGroupFlag, UInt posXCG, UInt posYCG, Int width, Int height);

    static Int  getSigCtxInc(Int patternSigCtx, UInt scanIdx, Int posX, Int posY, Int log2BlkSize, TextType ttype);

    static UInt getSigCoeffGroupCtxInc(const UInt* sigCoeffGroupFlag, UInt cGPosX, UInt cGPosY, Int width, Int height);

    estBitsSbacStruct* m_pcEstBitsSbac;

protected:

    Int      m_qpDelta[MAX_QP + 1];
    Int      m_sliceNsamples[LEVEL_RANGE + 1];
    Double   m_sliceSumC[LEVEL_RANGE + 1];
    Int*     m_plTempCoeff;

    QpParam  m_cQP;

    Double   m_dLambda;
    Double   m_dLambdaLuma;
    Double   m_dLambdaChroma;

    UInt     m_uiRDOQOffset;
    UInt     m_uiMaxTrSize;
    Bool     m_bEnc;
    Bool     m_useRDOQ;
    Bool     m_useRDOQTS;
    Bool     m_bUseAdaptQpSelect;
    Bool     m_useTransformSkipFast;
    Bool     m_scalingListEnabledFlag;
    Int     *m_quantCoef[SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM][SCALING_LIST_REM_NUM];     ///< array of quantization matrix coefficient 4x4
    Int     *m_dequantCoef[SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM][SCALING_LIST_REM_NUM];   ///< array of dequantization matrix coefficient 4x4

    Double  *m_errScale[SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM][SCALING_LIST_REM_NUM];

private:

    Void xTransformSkip(Int bitDepth, Short* piBlkResi, UInt uiStride, Int* psCoeff, Int width, Int height);

    Void signBitHidingHDQ(TCoeff* pQCoef, TCoeff* pCoef, UInt const *scan, Int* deltaU, Int width, Int height);

    UInt xQuant(TComDataCU* cu, Int* src, TCoeff* dst, Int* arlDes, Int width, Int height, TextType ttype, UInt absPartIdx);

    // RDOQ functions
    UInt xRateDistOptQuant(TComDataCU* cu, Int* srcCoeff, TCoeff* dstCoeff, Int* arlDstCoeff, UInt width, UInt height,
                           TextType ttype, UInt absPartIdx);

    __inline UInt xGetCodedLevel(Double& codedCost, Double& codedCost0, Double& codedCostSig, Int levelDouble,
                                 UInt maxAbsLevel, UShort ctxNumSig, UShort ctxNumOne, UShort ctxNumAbs, UShort absGoRice,
                                 UInt c1Idx, UInt c2Idx, Int qbits, Double scale, Bool bLast) const;

    __inline Double xGetICRateCost(UInt uiAbsLevel, UShort ctxNumOne, UShort ctxNumAbs, UShort absGoRice, UInt c1Idx, UInt c2Idx) const;

    __inline Int    xGetICRate(UInt uiAbsLevel, UShort ctxNumOne, UShort ctxNumAbs, UShort absGoRice, UInt c1Idx, UInt c2Idx) const;

    __inline Double xGetRateLast(UInt uiPosX, UInt uiPosY) const;

    __inline Double xGetRateSigCoeffGroup(UShort sigCoeffGroup, UShort ctxNumSig) const { return m_dLambda * m_pcEstBitsSbac->significantCoeffGroupBits[ctxNumSig][sigCoeffGroup]; }
    __inline Double xGetRateSigCoef(UShort sig, UShort ctxNumSig) const { return m_dLambda * m_pcEstBitsSbac->significantBits[ctxNumSig][sig]; }
    __inline Double xGetICost(Double rage) const { return m_dLambda * rage; } ///< Get the cost for a specific rate
    __inline Double xGetIEPRate() const          { return 32768; }            ///< Get the cost of an equal probable bit

    Void xDeQuant(Int bitDepth, const TCoeff* src, Int* dst, Int width, Int height, Int scalingListType);

    Void xIT(Int bitDepth, UInt mode, Int* coeff, Short* residual, UInt stride, Int width, Int height);

    Void xITransformSkip(Int bitDepth, Int* coeff, Short* residual, UInt stride, Int width, Int height);
};

//! \}

#endif // __TCOMTRQUANT__
