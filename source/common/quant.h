/*****************************************************************************
 * Copyright (C) 2014 x265 project
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

#ifndef X265_QUANT_H
#define X265_QUANT_H

#include "common.h"
#include "scalinglist.h"
#include "TLibCommon/ContextTables.h"

namespace x265 {
// private namespace

class CUData;
class Entropy;
struct TUEntropyCodingParameters;

struct QpParam
{
    int rem;
    int per;
    int qp;
    int64_t lambda2; /* FIX8 */
    int64_t lambda;  /* FIX8 */

    QpParam() : qp(MAX_INT) {}

    void setQpParam(int qpScaled)
    {
        if (qp != qpScaled)
        {
            rem = qpScaled % 6;
            per = qpScaled / 6;
            qp  = qpScaled;
            lambda2 = (int64_t)(x265_lambda2_tab[qp - QP_BD_OFFSET] * 256. + 0.5);
            lambda  = (int64_t)(x265_lambda_tab[qp - QP_BD_OFFSET] * 256. + 0.5);
        }
    }
};

class Quant
{
protected:

    const ScalingList* m_scalingList;
    Entropy*           m_entropyCoder;

    QpParam            m_qpParam[3];

    bool               m_useRDOQ;
    int64_t            m_psyRdoqScale;
    int32_t*           m_resiDctCoeff;
    int32_t*           m_fencDctCoeff;
    int16_t*           m_fencShortBuf;

    enum { IEP_RATE = 32768 }; /* FIX15 cost of an equal probable bit */

public:

    NoiseReduction*    m_nr;

    Quant();
    ~Quant();

    /* one-time setup */
    bool init(bool useRDOQ, double psyScale, const ScalingList& scalingList, Entropy& entropy);

    /* CU setup */
    void setQPforQuant(const CUData& ctu);

    uint32_t transformNxN(CUData* cu, pixel *fenc, uint32_t fencstride, int16_t* residual, uint32_t stride, coeff_t* coeff,
                          uint32_t log2TrSize, TextType ttype, uint32_t absPartIdx, bool useTransformSkip);

    void invtransformNxN(bool transQuantBypass, int16_t* residual, uint32_t stride, coeff_t* coeff,
                         uint32_t log2TrSize, TextType ttype, bool bIntra, bool useTransformSkip, uint32_t numSig);

    /* static methods shared with entropy.cpp */
    static uint32_t calcPatternSigCtx(uint64_t sigCoeffGroupFlag64, uint32_t cgPosX, uint32_t cgPosY, uint32_t log2TrSizeCG);
    static uint32_t getSigCtxInc(uint32_t patternSigCtx, uint32_t log2TrSize, uint32_t trSize, uint32_t blkPos, bool bIsLuma, uint32_t firstSignificanceMapContext);
    static uint32_t getSigCoeffGroupCtxInc(uint64_t sigCoeffGroupFlag64, uint32_t cgPosX, uint32_t cgPosY, uint32_t log2TrSizeCG);

protected:

    void setChromaQP(int qpin, TextType ttype, int chFmt);

    uint32_t signBitHidingHDQ(int16_t* qcoeff, int32_t* deltaU, uint32_t numSig, const TUEntropyCodingParameters &codingParameters);

    uint32_t rdoQuant(CUData* cu, int16_t* dstCoeff, uint32_t log2TrSize, TextType ttype, uint32_t absPartIdx, bool usePsy);
    inline uint32_t getRateLast(uint32_t posx, uint32_t posy) const;
};

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

#endif // ifndef X265_QUANT_H
