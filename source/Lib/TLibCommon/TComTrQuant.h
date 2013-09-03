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

namespace x265 {
// private namespace

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Constants
// ====================================================================================================================

#define QP_BITS 15

// ====================================================================================================================
// Type definition
// ====================================================================================================================

typedef struct
{
    int significantCoeffGroupBits[NUM_SIG_CG_FLAG_CTX][2];
    int significantBits[NUM_SIG_FLAG_CTX][2];
    int lastXBits[32];
    int lastYBits[32];
    int greaterOneBits[NUM_ONE_FLAG_CTX][2];
    int levelAbsBits[NUM_ABS_FLAG_CTX][2];

    int blockCbpBits[3 * NUM_QT_CBF_CTX][2];
    int blockRootCbpBits[4][2];
} estBitsSbacStruct;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

class QpParam
{
public:

    QpParam() {}

    int m_qp;
    int m_per;
    int m_rem;

    int m_bits;

public:

    void setQpParam(int qpScaled)
    {
        m_qp   = qpScaled;
        m_per  = qpScaled / 6;
        m_rem  = qpScaled % 6;
        m_bits = QP_BITS + m_per;
    }

    void clear()
    {
        m_qp   = 0;
        m_per  = 0;
        m_rem  = 0;
        m_bits = 0;
    }

    int per()   const { return m_per; }

    int rem()   const { return m_rem; }

    int bits()  const { return m_bits; }

    int qp() { return m_qp; }
};

/// transform and quantization class
class TComTrQuant
{
public:

    TComTrQuant();
    ~TComTrQuant();

    // initialize class
    void init(UInt maxTrSize, int useRDOQ, int useRDOQTS, int useTransformSkipFast);

    // transform & inverse transform functions
    UInt transformNxN(TComDataCU* cu, short* residual, UInt stride, TCoeff* coeff, UInt width, UInt height,
                      TextType ttype, UInt absPartIdx, int* lastPos, Bool useTransformSkip = false);

    void invtransformNxN(Bool transQuantBypass, UInt mode, short* residual, UInt stride, TCoeff* coeff, UInt width, UInt height, int scalingListType, Bool useTransformSkip = false, int lastPos = MAX_INT);

    // Misc functions
    void setQPforQuant(int qpy, TextType ttype, int qpBdOffset, int chromaQPOffset);

    void setLambda(double lambdaLuma, double lambdaChroma) { m_lumaLambda = lambdaLuma; m_chromaLambda = lambdaChroma; }

    void selectLambda(TextType ttype) { m_lambda = (ttype == TEXT_LUMA) ? m_lumaLambda : m_chromaLambda; }

    void initScalingList();
    void destroyScalingList();
    void setErrScaleCoeff(UInt list, UInt size, UInt qp);
    double* getErrScaleCoeff(UInt list, UInt size, UInt qp) { return m_errScale[size][list][qp]; }   //!< get Error Scale Coefficent

    int* getQuantCoeff(UInt list, UInt qp, UInt size) { return m_quantCoef[size][list][qp]; }        //!< get Quant Coefficent

    int* getDequantCoeff(UInt list, UInt qp, UInt size) { return m_dequantCoef[size][list][qp]; }    //!< get DeQuant Coefficent

    void setUseScalingList(Bool bUseScalingList) { m_scalingListEnabledFlag = bUseScalingList; }

    Bool getUseScalingList() { return m_scalingListEnabledFlag; }

    void setFlatScalingList();
    void xsetFlatScalingList(UInt list, UInt size, UInt qp);
    void xSetScalingListEnc(TComScalingList *scalingList, UInt list, UInt size, UInt qp);
    void xSetScalingListDec(TComScalingList *scalingList, UInt list, UInt size, UInt qp);
    void setScalingList(TComScalingList *scalingList);
    void setScalingListDec(TComScalingList *scalingList);
    void processScalingListEnc(int *coeff, int *quantcoeff, int quantScales, UInt height, UInt width, UInt ratio, int sizuNum, UInt dc);
    void processScalingListDec(int *coeff, int *dequantcoeff, int invQuantScales, UInt height, UInt width, UInt ratio, int sizuNum, UInt dc);

    static int  calcPatternSigCtx(const UInt* sigCoeffGroupFlag, UInt posXCG, UInt posYCG, int width, int height);

    static int  getSigCtxInc(int patternSigCtx, UInt scanIdx, int posX, int posY, int log2BlkSize, TextType ttype);

    static UInt getSigCoeffGroupCtxInc(const UInt* sigCoeffGroupFlag, UInt cGPosX, UInt cGPosY, int width, int height);

    estBitsSbacStruct* m_estBitsSbac;

protected:

    QpParam  m_qpParam;

    double   m_lambda;
    double   m_lumaLambda;
    double   m_chromaLambda;

    UInt     m_maxTrSize;
    Bool     m_useRDOQ;
    Bool     m_useRDOQTS;
    Bool     m_useTransformSkipFast;
    Bool     m_scalingListEnabledFlag;

    int*     m_tmpCoeff;
    int*     m_quantCoef[SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM][SCALING_LIST_REM_NUM];     ///< array of quantization matrix coefficient 4x4
    int*     m_dequantCoef[SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM][SCALING_LIST_REM_NUM];   ///< array of dequantization matrix coefficient 4x4

    double  *m_errScale[SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM][SCALING_LIST_REM_NUM];

private:

    void xTransformSkip(short* resiBlock, UInt stride, int* coeff, int width, int height);

    void signBitHidingHDQ(TCoeff* qcoeff, TCoeff* coeff, const UInt* scan, int* deltaU, int width, int height);

    UInt xQuant(TComDataCU* cu, int* src, TCoeff* dst, int width, int height, TextType ttype, UInt absPartIdx, int *lastPos);

    // RDOQ functions
    UInt xRateDistOptQuant(TComDataCU* cu, int* srcCoeff, TCoeff* dstCoeff, UInt width, UInt height, TextType ttype, UInt absPartIdx, int *lastPos);

    inline UInt xGetCodedLevel(double& codedCost, double& codedCost0, double& codedCostSig, int levelDouble,
                                 UInt maxAbsLevel, UShort ctxNumSig, UShort ctxNumOne, UShort ctxNumAbs, UShort absGoRice,
                                 UInt c1Idx, UInt c2Idx, int qbits, double scale, Bool bLast) const;

    inline double xGetICRateCost(UInt absLevel, UShort ctxNumOne, UShort ctxNumAbs, UShort absGoRice, UInt c1Idx, UInt c2Idx) const;

    inline int    xGetICRate(UInt absLevel, UShort ctxNumOne, UShort ctxNumAbs, UShort absGoRice, UInt c1Idx, UInt c2Idx) const;

    inline double xGetRateLast(UInt posx, UInt posy) const;

    inline double xGetRateSigCoeffGroup(UShort sigCoeffGroup, UShort ctxNumSig) const { return m_lambda * m_estBitsSbac->significantCoeffGroupBits[ctxNumSig][sigCoeffGroup]; }

    inline double xGetRateSigCoef(UShort sig, UShort ctxNumSig) const { return m_lambda * m_estBitsSbac->significantBits[ctxNumSig][sig]; }

    inline double xGetICost(double rage) const { return m_lambda * rage; } ///< Get the cost for a specific rate

    inline double xGetIEPRate() const          { return 32768; }            ///< Get the cost of an equal probable bit

    void xDeQuant(const TCoeff* src, int* dst, int width, int height, int scalingListType);

    void xIT(UInt mode, int* coeff, short* residual, UInt stride, int width, int height);

    void xITransformSkip(int* coeff, short* residual, UInt stride, int width, int height);
};
}
//! \}

#endif // __TCOMTRQUANT__
