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

#ifndef X265_TCOMTRQUANT_H
#define X265_TCOMTRQUANT_H

#include "common.h"
#include "scalinglist.h"
#include "TComYuv.h"
#include "TComDataCU.h"
#include "ContextTables.h"

namespace x265 {
// private namespace

struct EstBitsSbac
{
    int significantCoeffGroupBits[NUM_SIG_CG_FLAG_CTX][2];
    uint32_t significantBits[NUM_SIG_FLAG_CTX][2];
    int lastXBits[10];
    int lastYBits[10];
    int greaterOneBits[NUM_ONE_FLAG_CTX][2];
    int levelAbsBits[NUM_ABS_FLAG_CTX][2];

    int blockCbpBits[NUM_QT_CBF_CTX][2];
    int blockRootCbpBits[NUM_QT_ROOT_CBF_CTX][2];
};

// TU settings for entropy encoding
struct TUEntropyCodingParameters
{
    const uint16_t *scan;
    const uint16_t *scanCG;
    ScanType        scanType;
    uint32_t        log2TrSizeCG;
    uint32_t        firstSignificanceMapContext;
};

struct QpParam
{
    int rem;
    int per;
    int qp;

    QpParam()
    {
        rem = 0;
        per = 0;
        qp  = 0;
    }

    void setQpParam(int qpScaled)
    {
        if (qp != qpScaled)
        {
            rem  = qpScaled % 6;
            per  = qpScaled / 6;
            qp   = qpScaled;
        }
    }
};

class TComTrQuant
{
public:

    TComTrQuant();
    ~TComTrQuant();

    /* one-time setup */
    bool init(bool useRDOQ, const ScalingList& scalingList);

    /* CU setup */
    void setQPforQuant(TComDataCU* cu);
    void setLambdas(double lambdaY, double lambdaCb, double lambdaCr) { m_lambdas[0] = lambdaY; m_lambdas[1] = lambdaCb; m_lambdas[2] = lambdaCr; }

    uint32_t transformNxN(TComDataCU* cu, int16_t* residual, uint32_t stride, coeff_t* coeff, uint32_t log2TrSize,
                          TextType ttype, uint32_t absPartIdx, bool useTransformSkip, bool curUseRDOQ);

    void invtransformNxN(bool transQuantBypass, int16_t* residual, uint32_t stride, coeff_t* coeff,
                         uint32_t log2TrSize, TextType ttype, bool bIntra, bool useTransformSkip, uint32_t numSig);

    EstBitsSbac        m_estBitsSbac;
    NoiseReduction*    m_nr;
    const ScalingList* m_scalingList;

    QpParam  m_qpParam[3];
    double   m_lambda;
    double   m_lambdas[3];

    bool     m_useRDOQ;
    coeff_t* m_resiDctCoeff;

    static const uint32_t IEP_RATE = 32768; // cost of an equal probable bit

    void selectLambda(TextType ttype) { m_lambda = m_lambdas[ttype]; }
    void setQPforQuant(int qpy, TextType ttype, int chromaQPOffset, int chFmt);

protected:

    uint32_t signBitHidingHDQ(coeff_t* qcoeff, coeff_t* coeff, int32_t* deltaU, uint32_t numSig, const TUEntropyCodingParameters &codingParameters);
    uint32_t quant(TComDataCU* cu, coeff_t* dst, uint32_t log2TrSize, TextType ttype, uint32_t absPartIdx);

    /* RDOQ functions */

    uint32_t rdoQuant(TComDataCU* cu, coeff_t* dstCoeff, uint32_t log2TrSize, TextType ttype, uint32_t absPartIdx);

    inline uint32_t getCodedLevel(double& codedCost, const double curCostSig, double& codedCostSig, int levelDouble,
                                  uint32_t maxAbsLevel, uint32_t baseLevel, const int *greaterOneBits, const int *levelAbsBits,
                                  uint32_t absGoRice, uint32_t c1c2Idx, int qbits, double scale) const;

    inline uint32_t getICRateCost(uint32_t absLevel, int32_t diffLevel, const int *greaterOneBits, const int *levelAbsBits,
                                  uint32_t absGoRice, uint32_t c1c2Idx) const;

    inline int getICRate(uint32_t absLevel, int32_t diffLevel, const int *greaterOneBits, const int *levelAbsBits,
                         uint32_t absGoRice, uint32_t c1c2Idx) const;

    inline double getRateLast(uint32_t posx, uint32_t posy) const;

public:

    /* static methods shared with entropy.cpp */

    static uint32_t calcPatternSigCtx(const uint64_t sigCoeffGroupFlag64, uint32_t cgPosX, uint32_t cgPosY, uint32_t log2TrSizeCG);
    static uint32_t getSigCtxInc(uint32_t patternSigCtx, const uint32_t log2TrSize, const uint32_t trSize, const uint32_t blkPos, const TextType ctype, const uint32_t firstSignificanceMapContext);
    static uint32_t getSigCoeffGroupCtxInc(const uint64_t sigCoeffGroupFlag64, uint32_t cgPosX, uint32_t cgPosY, const uint32_t log2TrSizeCG);
};

static inline void getTUEntropyCodingParameters(TComDataCU* cu, TUEntropyCodingParameters &result, uint32_t absPartIdx, uint32_t log2TrSize, TextType ttype)
{
    // set the group layout
    const uint32_t log2TrSizeCG = log2TrSize - 2;

    result.log2TrSizeCG = log2TrSizeCG;

    // set the scan orders
    result.scanType = cu->getCoefScanIdx(absPartIdx, log2TrSize, ttype == TEXT_LUMA, cu->isIntra(absPartIdx));
    result.scan = g_scanOrder[result.scanType][log2TrSize - 2];
    result.scanCG = g_scanOrderCG[result.scanType][log2TrSizeCG];

    // set the significance map context selection parameters
    TextType ctype = (ttype == TEXT_LUMA) ? TEXT_LUMA : TEXT_CHROMA;
    if (log2TrSize == 2)
    {
        result.firstSignificanceMapContext = 0;
        X265_CHECK(!significanceMapContextSetStart[ctype][CONTEXT_TYPE_4x4], "context failure\n");
    }
    else if (log2TrSize == 3)
    {
        result.firstSignificanceMapContext = 9;
        X265_CHECK(significanceMapContextSetStart[ctype][CONTEXT_TYPE_8x8] == 9, "context failure\n");
        if (result.scanType != SCAN_DIAG && !ctype)
        {
            result.firstSignificanceMapContext += 6;
            X265_CHECK(nonDiagonalScan8x8ContextOffset[ctype] == 6, "context failure\n");
        }
    }
    else
    {
        result.firstSignificanceMapContext = (ctype ? 12 : 21);
        X265_CHECK(significanceMapContextSetStart[ctype][CONTEXT_TYPE_NxN] == (uint32_t)(ctype ? 12 : 21), "context failure\n");
    }
}

static inline uint32_t getGroupIdx(const uint32_t idx)
{
    // TODO: Why is this not a table lookup?

    uint32_t group = (idx >> 3);

    if (idx >= 24)
        group = 2;
    uint32_t groupIdx = ((idx >> (group + 1)) - 2) + 4 + (group << 1);
    if (idx <= 3)
        groupIdx = idx;

#ifdef _DEBUG
    static const uint8_t g_groupIdx[32] = { 0, 1, 2, 3, 4, 4, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9 };
    assert(groupIdx == g_groupIdx[idx]);
#endif

    return groupIdx;
}

}

#endif // ifndef X265_TCOMTRQUANT_H
