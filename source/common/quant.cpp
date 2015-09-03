/*****************************************************************************
 * Copyright (C) 2015 x265 project
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

#include "common.h"
#include "primitives.h"
#include "quant.h"
#include "framedata.h"
#include "entropy.h"
#include "yuv.h"
#include "cudata.h"
#include "contexts.h"

using namespace X265_NS;

#define SIGN(x,y) ((x^(y >> 31))-(y >> 31))

namespace {

struct coeffGroupRDStats
{
    int     nnzBeforePos0;     /* indicates coeff other than pos 0 are coded */
    int64_t codedLevelAndDist; /* distortion and level cost of coded coefficients */
    int64_t uncodedDist;       /* uncoded distortion cost of coded coefficients */
    int64_t sigCost;           /* cost of signaling significant coeff bitmap */
    int64_t sigCost0;          /* cost of signaling sig coeff bit of coeff 0 */
};

inline int fastMin(int x, int y)
{
    return y + ((x - y) & ((x - y) >> (sizeof(int) * CHAR_BIT - 1))); // min(x, y)
}

inline int getICRate(uint32_t absLevel, int32_t diffLevel, const int* greaterOneBits, const int* levelAbsBits, const uint32_t absGoRice, const uint32_t maxVlc, uint32_t c1c2Idx)
{
    X265_CHECK(c1c2Idx <= 3, "c1c2Idx check failure\n");
    X265_CHECK(absGoRice <= 4, "absGoRice check failure\n");
    if (!absLevel)
    {
        X265_CHECK(diffLevel < 0, "diffLevel check failure\n");
        return 0;
    }
    int rate = 0;

    if (diffLevel < 0)
    {
        X265_CHECK(absLevel <= 2, "absLevel check failure\n");
        rate += greaterOneBits[(absLevel == 2)];

        if (absLevel == 2)
            rate += levelAbsBits[0];
    }
    else
    {
        uint32_t symbol = diffLevel;
        bool expGolomb = (symbol > maxVlc);

        if (expGolomb)
        {
            absLevel = symbol - maxVlc;

            // NOTE: mapping to x86 hardware instruction BSR
            unsigned long size;
            CLZ(size, absLevel);
            int egs = size * 2 + 1;

            rate += egs << 15;

            // NOTE: in here, expGolomb=true means (symbol >= maxVlc + 1)
            X265_CHECK(fastMin(symbol, (maxVlc + 1)) == (int)maxVlc + 1, "min check failure\n");
            symbol = maxVlc + 1;
        }

        uint32_t prefLen = (symbol >> absGoRice) + 1;
        uint32_t numBins = fastMin(prefLen + absGoRice, 8 /* g_goRicePrefixLen[absGoRice] + absGoRice */);

        rate += numBins << 15;

        if (c1c2Idx & 1)
            rate += greaterOneBits[1];

        if (c1c2Idx == 3)
            rate += levelAbsBits[1];
    }
    return rate;
}

#if CHECKED_BUILD || _DEBUG
inline int getICRateNegDiff(uint32_t absLevel, const int* greaterOneBits, const int* levelAbsBits)
{
    X265_CHECK(absLevel <= 2, "absLevel check failure\n");

    int rate;
    if (absLevel == 0)
        rate = 0;
    else if (absLevel == 2)
        rate = greaterOneBits[1] + levelAbsBits[0];
    else
        rate = greaterOneBits[0];
    return rate;
}
#endif

inline int getICRateLessVlc(uint32_t absLevel, int32_t diffLevel, const uint32_t absGoRice)
{
    X265_CHECK(absGoRice <= 4, "absGoRice check failure\n");
    if (!absLevel)
    {
        X265_CHECK(diffLevel < 0, "diffLevel check failure\n");
        return 0;
    }
    int rate;

    uint32_t symbol = diffLevel;
    uint32_t prefLen = (symbol >> absGoRice) + 1;
    uint32_t numBins = fastMin(prefLen + absGoRice, 8 /* g_goRicePrefixLen[absGoRice] + absGoRice */);

    rate = numBins << 15;

    return rate;
}

/* Calculates the cost for specific absolute transform level */
inline uint32_t getICRateCost(uint32_t absLevel, int32_t diffLevel, const int* greaterOneBits, const int* levelAbsBits, uint32_t absGoRice, uint32_t c1c2Idx)
{
    X265_CHECK(absLevel, "absLevel should not be zero\n");

    if (diffLevel < 0)
    {
        X265_CHECK((absLevel == 1) || (absLevel == 2), "absLevel range check failure\n");

        uint32_t rate = greaterOneBits[(absLevel == 2)];
        if (absLevel == 2)
            rate += levelAbsBits[0];
        return rate;
    }
    else
    {
        uint32_t rate;
        uint32_t symbol = diffLevel;
        if ((symbol >> absGoRice) < COEF_REMAIN_BIN_REDUCTION)
        {
            uint32_t length = symbol >> absGoRice;
            rate = (length + 1 + absGoRice) << 15;
        }
        else
        {
            uint32_t length = 0;
            symbol = (symbol >> absGoRice) - COEF_REMAIN_BIN_REDUCTION;
            if (symbol)
            {
                unsigned long idx;
                CLZ(idx, symbol + 1);
                length = idx;
            }

            rate = (COEF_REMAIN_BIN_REDUCTION + length + absGoRice + 1 + length) << 15;
        }
        if (c1c2Idx & 1)
            rate += greaterOneBits[1];
        if (c1c2Idx == 3)
            rate += levelAbsBits[1];
        return rate;
    }
}

}

Quant::Quant()
{
    m_resiDctCoeff = NULL;
    m_fencDctCoeff = NULL;
    m_fencShortBuf = NULL;
    m_frameNr      = NULL;
    m_nr           = NULL;
}

bool Quant::init(int rdoqLevel, double psyScale, const ScalingList& scalingList, Entropy& entropy)
{
    m_entropyCoder = &entropy;
    m_rdoqLevel    = rdoqLevel;
    m_psyRdoqScale = (int32_t)(psyScale * 256.0);
    X265_CHECK((psyScale * 256.0) < (double)MAX_INT, "psyScale value too large\n");
    m_scalingList  = &scalingList;
    m_resiDctCoeff = X265_MALLOC(int16_t, MAX_TR_SIZE * MAX_TR_SIZE * 2);
    m_fencDctCoeff = m_resiDctCoeff + (MAX_TR_SIZE * MAX_TR_SIZE);
    m_fencShortBuf = X265_MALLOC(int16_t, MAX_TR_SIZE * MAX_TR_SIZE);

    return m_resiDctCoeff && m_fencShortBuf;
}

bool Quant::allocNoiseReduction(const x265_param& param)
{
    m_frameNr = X265_MALLOC(NoiseReduction, param.frameNumThreads);
    if (m_frameNr)
        memset(m_frameNr, 0, sizeof(NoiseReduction) * param.frameNumThreads);
    else
        return false;
    return true;
}

Quant::~Quant()
{
    X265_FREE(m_frameNr);
    X265_FREE(m_resiDctCoeff);
    X265_FREE(m_fencShortBuf);
}

void Quant::setQPforQuant(const CUData& ctu, int qp)
{
    m_nr = m_frameNr ? &m_frameNr[ctu.m_encData->m_frameEncoderID] : NULL;
    m_qpParam[TEXT_LUMA].setQpParam(qp + QP_BD_OFFSET);
    setChromaQP(qp + ctu.m_slice->m_pps->chromaQpOffset[0], TEXT_CHROMA_U, ctu.m_chromaFormat);
    setChromaQP(qp + ctu.m_slice->m_pps->chromaQpOffset[1], TEXT_CHROMA_V, ctu.m_chromaFormat);
}

void Quant::setChromaQP(int qpin, TextType ttype, int chFmt)
{
    int qp = x265_clip3(-QP_BD_OFFSET, 57, qpin);
    if (qp >= 30)
    {
        if (chFmt == X265_CSP_I420)
            qp = g_chromaScale[qp];
        else
            qp = X265_MIN(qp, QP_MAX_SPEC);
    }
    m_qpParam[ttype].setQpParam(qp + QP_BD_OFFSET);
}

/* To minimize the distortion only. No rate is considered */
uint32_t Quant::signBitHidingHDQ(int16_t* coeff, int32_t* deltaU, uint32_t numSig, const TUEntropyCodingParameters &codeParams, uint32_t log2TrSize)
{
    uint32_t trSize = 1 << log2TrSize;
    const uint16_t* scan = codeParams.scan;

    uint8_t coeffNum[MLS_GRP_NUM];      // value range[0, 16]
    uint16_t coeffSign[MLS_GRP_NUM];    // bit mask map for non-zero coeff sign
    uint16_t coeffFlag[MLS_GRP_NUM];    // bit mask map for non-zero coeff

#if CHECKED_BUILD || _DEBUG
    // clean output buffer, the asm version of scanPosLast Never output anything after latest non-zero coeff group
    memset(coeffNum, 0, sizeof(coeffNum));
    memset(coeffSign, 0, sizeof(coeffNum));
    memset(coeffFlag, 0, sizeof(coeffNum));
#endif
    const int lastScanPos = primitives.scanPosLast(codeParams.scan, coeff, coeffSign, coeffFlag, coeffNum, numSig, g_scan4x4[codeParams.scanType], trSize);
    const int cgLastScanPos = (lastScanPos >> LOG2_SCAN_SET_SIZE);
    unsigned long tmp;

    // first CG need specially processing
    const uint32_t correctOffset = 0x0F & (lastScanPos ^ 0xF);
    coeffFlag[cgLastScanPos] <<= correctOffset;

    for (int cg = cgLastScanPos; cg >= 0; cg--)
    {
        int cgStartPos = cg << LOG2_SCAN_SET_SIZE;
        int n;

#if CHECKED_BUILD || _DEBUG
        for (n = SCAN_SET_SIZE - 1; n >= 0; --n)
            if (coeff[scan[n + cgStartPos]])
                break;
        int lastNZPosInCG0 = n;
#endif

        if (coeffNum[cg] == 0)
        {
            X265_CHECK(lastNZPosInCG0 < 0, "all zero block check failure\n");
            continue;
        }

#if CHECKED_BUILD || _DEBUG
        for (n = 0;; n++)
            if (coeff[scan[n + cgStartPos]])
                break;

        int firstNZPosInCG0 = n;
#endif

        CLZ(tmp, coeffFlag[cg]);
        const int firstNZPosInCG = (15 ^ tmp);

        CTZ(tmp, coeffFlag[cg]);
        const int lastNZPosInCG = (15 ^ tmp);

        X265_CHECK(firstNZPosInCG0 == firstNZPosInCG, "firstNZPosInCG0 check failure\n");
        X265_CHECK(lastNZPosInCG0 == lastNZPosInCG, "lastNZPosInCG0 check failure\n");

        if (lastNZPosInCG - firstNZPosInCG >= SBH_THRESHOLD)
        {
            uint32_t signbit = coeff[scan[cgStartPos + firstNZPosInCG]] > 0 ? 0 : 1;
            uint32_t absSum = 0;

            for (n = firstNZPosInCG; n <= lastNZPosInCG; n++)
                absSum += coeff[scan[n + cgStartPos]];

            if (signbit != (absSum & 0x1)) // compare signbit with sum_parity
            {
                int minCostInc = MAX_INT,  minPos = -1, curCost = MAX_INT;
                int32_t finalChange = 0, curChange = 0;
                uint32_t cgFlags = coeffFlag[cg];
                if (cg == cgLastScanPos)
                    cgFlags >>= correctOffset;

                for (n = (cg == cgLastScanPos ? lastNZPosInCG : SCAN_SET_SIZE - 1); n >= 0; --n)
                {
                    uint32_t blkPos = scan[n + cgStartPos];
                    X265_CHECK(!!coeff[blkPos] == !!(cgFlags & 1), "non zero coeff check failure\n");

                    if (cgFlags & 1)
                    {
                        if (deltaU[blkPos] > 0)
                        {
                            curCost = -deltaU[blkPos];
                            curChange = 1;
                        }
                        else
                        {
                            if ((cgFlags == 1) && (abs(coeff[blkPos]) == 1))
                            {
                                X265_CHECK(n == firstNZPosInCG, "firstNZPosInCG position check failure\n");
                                curCost = MAX_INT;
                            }
                            else
                            {
                                curCost = deltaU[blkPos];
                                curChange = -1;
                            }
                        }
                    }
                    else
                    {
                        if (cgFlags == 0)
                        {
                            X265_CHECK(n < firstNZPosInCG, "firstNZPosInCG position check failure\n");
                            uint32_t thisSignBit = m_resiDctCoeff[blkPos] >= 0 ? 0 : 1;
                            if (thisSignBit != signbit)
                                curCost = MAX_INT;
                            else
                            {
                                curCost = -deltaU[blkPos];
                                curChange = 1;
                            }
                        }
                        else
                        {
                            curCost = -deltaU[blkPos];
                            curChange = 1;
                        }
                    }

                    if (curCost < minCostInc)
                    {
                        minCostInc = curCost;
                        finalChange = curChange;
                        minPos = blkPos;
                    }
                    cgFlags>>=1;
                }

                /* do not allow change to violate coeff clamp */
                if (coeff[minPos] == 32767 || coeff[minPos] == -32768)
                    finalChange = -1;

                if (!coeff[minPos])
                    numSig++;
                else if (finalChange == -1 && abs(coeff[minPos]) == 1)
                    numSig--;

                {
                    const int16_t sigMask = ((int16_t)m_resiDctCoeff[minPos]) >> 15;
                    coeff[minPos] += ((int16_t)finalChange ^ sigMask) - sigMask;
                }
            }
        }
    }

    return numSig;
}

uint32_t Quant::transformNxN(const CUData& cu, const pixel* fenc, uint32_t fencStride, const int16_t* residual, uint32_t resiStride,
                             coeff_t* coeff, uint32_t log2TrSize, TextType ttype, uint32_t absPartIdx, bool useTransformSkip)
{
    const uint32_t sizeIdx = log2TrSize - 2;

    if (cu.m_tqBypass[0])
    {
        X265_CHECK(log2TrSize >= 2 && log2TrSize <= 5, "Block size mistake!\n");
        return primitives.cu[sizeIdx].copy_cnt(coeff, residual, resiStride);
    }

    bool isLuma  = ttype == TEXT_LUMA;
    bool usePsy  = m_psyRdoqScale && isLuma && !useTransformSkip;
    int transformShift = MAX_TR_DYNAMIC_RANGE - X265_DEPTH - log2TrSize; // Represents scaling through forward transform

    X265_CHECK((cu.m_slice->m_sps->quadtreeTULog2MaxSize >= log2TrSize), "transform size too large\n");
    if (useTransformSkip)
    {
#if X265_DEPTH <= 10
        X265_CHECK(transformShift >= 0, "invalid transformShift\n");
        primitives.cu[sizeIdx].cpy2Dto1D_shl(m_resiDctCoeff, residual, resiStride, transformShift);
#else
        if (transformShift >= 0)
            primitives.cu[sizeIdx].cpy2Dto1D_shl(m_resiDctCoeff, residual, resiStride, transformShift);
        else
            primitives.cu[sizeIdx].cpy2Dto1D_shr(m_resiDctCoeff, residual, resiStride, -transformShift);
#endif
    }
    else
    {
        bool isIntra = cu.isIntra(absPartIdx);

        if (!sizeIdx && isLuma && isIntra)
            primitives.dst4x4(residual, m_resiDctCoeff, resiStride);
        else
            primitives.cu[sizeIdx].dct(residual, m_resiDctCoeff, resiStride);

        /* NOTE: if RDOQ is disabled globally, psy-rdoq is also disabled, so
         * there is no risk of performing this DCT unnecessarily */
        if (usePsy)
        {
            int trSize = 1 << log2TrSize;
            /* perform DCT on source pixels for psy-rdoq */
            primitives.cu[sizeIdx].copy_ps(m_fencShortBuf, trSize, fenc, fencStride);
            primitives.cu[sizeIdx].dct(m_fencShortBuf, m_fencDctCoeff, trSize);
        }

        if (m_nr && m_nr->offset)
        {
            /* denoise is not applied to intra residual, so DST can be ignored */
            int cat = sizeIdx + 4 * !isLuma + 8 * !isIntra;
            int numCoeff = 1 << (log2TrSize * 2);
            primitives.denoiseDct(m_resiDctCoeff, m_nr->residualSum[cat], m_nr->offset[cat], numCoeff);
            m_nr->count[cat]++;
        }
    }

    if (m_rdoqLevel)
        return rdoQuant(cu, coeff, log2TrSize, ttype, absPartIdx, usePsy);
    else
    {
        int deltaU[32 * 32];

        int scalingListType = (cu.isIntra(absPartIdx) ? 0 : 3) + ttype;
        int rem = m_qpParam[ttype].rem;
        int per = m_qpParam[ttype].per;
        const int32_t* quantCoeff = m_scalingList->m_quantCoef[log2TrSize - 2][scalingListType][rem];

        int qbits = QUANT_SHIFT + per + transformShift;
        int add = (cu.m_slice->m_sliceType == I_SLICE ? 171 : 85) << (qbits - 9);
        int numCoeff = 1 << (log2TrSize * 2);

        uint32_t numSig = primitives.quant(m_resiDctCoeff, quantCoeff, deltaU, coeff, qbits, add, numCoeff);

        if (numSig >= 2 && cu.m_slice->m_pps->bSignHideEnabled)
        {
            TUEntropyCodingParameters codeParams;
            cu.getTUEntropyCodingParameters(codeParams, absPartIdx, log2TrSize, isLuma);
            return signBitHidingHDQ(coeff, deltaU, numSig, codeParams, log2TrSize);
        }
        else
            return numSig;
    }
}

void Quant::invtransformNxN(const CUData& cu, int16_t* residual, uint32_t resiStride, const coeff_t* coeff,
                            uint32_t log2TrSize, TextType ttype, bool bIntra, bool useTransformSkip, uint32_t numSig)
{
    const uint32_t sizeIdx = log2TrSize - 2;

    if (cu.m_tqBypass[0])
    {
        primitives.cu[sizeIdx].cpy1Dto2D_shl(residual, coeff, resiStride, 0);
        return;
    }

    // Values need to pass as input parameter in dequant
    int rem = m_qpParam[ttype].rem;
    int per = m_qpParam[ttype].per;
    int transformShift = MAX_TR_DYNAMIC_RANGE - X265_DEPTH - log2TrSize;
    int shift = QUANT_IQUANT_SHIFT - QUANT_SHIFT - transformShift;
    int numCoeff = 1 << (log2TrSize * 2);

    if (m_scalingList->m_bEnabled)
    {
        int scalingListType = (bIntra ? 0 : 3) + ttype;
        const int32_t* dequantCoef = m_scalingList->m_dequantCoef[sizeIdx][scalingListType][rem];
        primitives.dequant_scaling(coeff, dequantCoef, m_resiDctCoeff, numCoeff, per, shift);
    }
    else
    {
        int scale = m_scalingList->s_invQuantScales[rem] << per;
        primitives.dequant_normal(coeff, m_resiDctCoeff, numCoeff, scale, shift);
    }

    if (useTransformSkip)
    {
#if X265_DEPTH <= 10
        X265_CHECK(transformShift > 0, "invalid transformShift\n");
        primitives.cu[sizeIdx].cpy1Dto2D_shr(residual, m_resiDctCoeff, resiStride, transformShift);
#else
        if (transformShift > 0)
            primitives.cu[sizeIdx].cpy1Dto2D_shr(residual, m_resiDctCoeff, resiStride, transformShift);
        else
            primitives.cu[sizeIdx].cpy1Dto2D_shl(residual, m_resiDctCoeff, resiStride, -transformShift);
#endif
    }
    else
    {
        int useDST = !sizeIdx && ttype == TEXT_LUMA && bIntra;
        X265_CHECK((int)numSig == primitives.cu[log2TrSize - 2].count_nonzero(coeff), "numSig differ\n");
        // DC only
        if (numSig == 1 && coeff[0] != 0 && !useDST)
        {
            const int shift_1st = 7 - 6;
            const int add_1st = 1 << (shift_1st - 1);
            const int shift_2nd = 12 - (X265_DEPTH - 8) - 3;
            const int add_2nd = 1 << (shift_2nd - 1);

            int dc_val = (((m_resiDctCoeff[0] * (64 >> 6) + add_1st) >> shift_1st) * (64 >> 3) + add_2nd) >> shift_2nd;
            primitives.cu[sizeIdx].blockfill_s(residual, resiStride, (int16_t)dc_val);
            return;
        }

        if (useDST)
            primitives.idst4x4(m_resiDctCoeff, residual, resiStride);
        else
            primitives.cu[sizeIdx].idct(m_resiDctCoeff, residual, resiStride);
    }
}

/* Rate distortion optimized quantization for entropy coding engines using
 * probability models like CABAC */
uint32_t Quant::rdoQuant(const CUData& cu, int16_t* dstCoeff, uint32_t log2TrSize, TextType ttype, uint32_t absPartIdx, bool usePsy)
{
    int transformShift = MAX_TR_DYNAMIC_RANGE - X265_DEPTH - log2TrSize; /* Represents scaling through forward transform */
    int scalingListType = (cu.isIntra(absPartIdx) ? 0 : 3) + ttype;
    const uint32_t usePsyMask = usePsy ? -1 : 0;

    X265_CHECK(scalingListType < 6, "scaling list type out of range\n");

    int rem = m_qpParam[ttype].rem;
    int per = m_qpParam[ttype].per;
    int qbits = QUANT_SHIFT + per + transformShift; /* Right shift of non-RDOQ quantizer level = (coeff*Q + offset)>>q_bits */
    int add = (1 << (qbits - 1));
    const int32_t* qCoef = m_scalingList->m_quantCoef[log2TrSize - 2][scalingListType][rem];

    int numCoeff = 1 << (log2TrSize * 2);
    uint32_t numSig = primitives.nquant(m_resiDctCoeff, qCoef, dstCoeff, qbits, add, numCoeff);
    X265_CHECK((int)numSig == primitives.cu[log2TrSize - 2].count_nonzero(dstCoeff), "numSig differ\n");
    if (!numSig)
        return 0;

    uint32_t trSize = 1 << log2TrSize;
    int64_t lambda2 = m_qpParam[ttype].lambda2;
    const int64_t psyScale = ((int64_t)m_psyRdoqScale * m_qpParam[ttype].lambda);

    /* unquant constants for measuring distortion. Scaling list quant coefficients have a (1 << 4)
     * scale applied that must be removed during unquant. Note that in real dequant there is clipping
     * at several stages. We skip the clipping for simplicity when measuring RD cost */
    const int32_t* unquantScale = m_scalingList->m_dequantCoef[log2TrSize - 2][scalingListType][rem];
    int unquantShift = QUANT_IQUANT_SHIFT - QUANT_SHIFT - transformShift + (m_scalingList->m_bEnabled ? 4 : 0);
    int unquantRound = (unquantShift > per) ? 1 << (unquantShift - per - 1) : 0;
    int scaleBits = SCALE_BITS - 2 * transformShift;

#define UNQUANT(lvl)    (((lvl) * (unquantScale[blkPos] << per) + unquantRound) >> unquantShift)
#define SIGCOST(bits)   ((lambda2 * (bits)) >> 8)
#define RDCOST(d, bits) ((((int64_t)d * d) << scaleBits) + SIGCOST(bits))
#define PSYVALUE(rec)   ((psyScale * (rec)) >> (2 * transformShift + 1))

    int64_t costCoeff[32 * 32];   /* d*d + lambda * bits */
    int64_t costUncoded[32 * 32]; /* d*d + lambda * 0    */
    int64_t costSig[32 * 32];     /* lambda * bits       */

    int rateIncUp[32 * 32];      /* signal overhead of increasing level */
    int rateIncDown[32 * 32];    /* signal overhead of decreasing level */
    int sigRateDelta[32 * 32];   /* signal difference between zero and non-zero */

    int64_t costCoeffGroupSig[MLS_GRP_NUM]; /* lambda * bits of group coding cost */
    uint64_t sigCoeffGroupFlag64 = 0;

    const uint32_t cgSize = (1 << MLS_CG_SIZE); /* 4x4 num coef = 16 */
    bool bIsLuma = ttype == TEXT_LUMA;

    /* total rate distortion cost of transform block, as CBF=0 */
    int64_t totalUncodedCost = 0;

    /* Total rate distortion cost of this transform block, counting te distortion of uncoded blocks,
     * the distortion and signal cost of coded blocks, and the coding cost of significant
     * coefficient and coefficient group bitmaps */
    int64_t totalRdCost = 0;

    TUEntropyCodingParameters codeParams;
    cu.getTUEntropyCodingParameters(codeParams, absPartIdx, log2TrSize, bIsLuma);
    const uint32_t cgNum = 1 << (codeParams.log2TrSizeCG * 2);
    const uint32_t cgStride = (trSize >> MLS_CG_LOG2_SIZE);

    uint8_t coeffNum[MLS_GRP_NUM];      // value range[0, 16]
    uint16_t coeffSign[MLS_GRP_NUM];    // bit mask map for non-zero coeff sign
    uint16_t coeffFlag[MLS_GRP_NUM];    // bit mask map for non-zero coeff

#if CHECKED_BUILD || _DEBUG
    // clean output buffer, the asm version of scanPosLast Never output anything after latest non-zero coeff group
    memset(coeffNum, 0, sizeof(coeffNum));
    memset(coeffSign, 0, sizeof(coeffNum));
    memset(coeffFlag, 0, sizeof(coeffNum));
#endif
    const int lastScanPos = primitives.scanPosLast(codeParams.scan, dstCoeff, coeffSign, coeffFlag, coeffNum, numSig, g_scan4x4[codeParams.scanType], trSize);
    const int cgLastScanPos = (lastScanPos >> LOG2_SCAN_SET_SIZE);


    /* TODO: update bit estimates if dirty */
    EstBitsSbac& estBitsSbac = m_entropyCoder->m_estBitsSbac;

    uint32_t scanPos = 0;
    uint32_t c1 = 1;

    // process trail all zero Coeff Group

    /* coefficients after lastNZ have no distortion signal cost */
    const int zeroCG = cgNum - 1 - cgLastScanPos;
    memset(&costCoeff[(cgLastScanPos + 1) << MLS_CG_SIZE], 0, zeroCG * MLS_CG_BLK_SIZE * sizeof(int64_t));
    memset(&costSig[(cgLastScanPos + 1) << MLS_CG_SIZE], 0, zeroCG * MLS_CG_BLK_SIZE * sizeof(int64_t));

    /* sum zero coeff (uncodec) cost */

    // TODO: does we need these cost?
    if (usePsyMask)
    {
        for (int cgScanPos = cgLastScanPos + 1; cgScanPos < (int)cgNum ; cgScanPos++)
        {
            X265_CHECK(coeffNum[cgScanPos] == 0, "count of coeff failure\n");

            uint32_t scanPosBase = (cgScanPos << MLS_CG_SIZE);
            uint32_t blkPos      = codeParams.scan[scanPosBase];

            // TODO: we can't SIMD optimize because PSYVALUE need 64-bits multiplication, convert to Double can work faster by FMA
            for (int y = 0; y < MLS_CG_SIZE; y++)
            {
                for (int x = 0; x < MLS_CG_SIZE; x++)
                {
                    int signCoef         = m_resiDctCoeff[blkPos + x];            /* pre-quantization DCT coeff */
                    int predictedCoef    = m_fencDctCoeff[blkPos + x] - signCoef; /* predicted DCT = source DCT - residual DCT*/

                    costUncoded[blkPos + x] = ((int64_t)signCoef * signCoef) << scaleBits;

                    /* when no residual coefficient is coded, predicted coef == recon coef */
                    costUncoded[blkPos + x] -= PSYVALUE(predictedCoef);

                    totalUncodedCost += costUncoded[blkPos + x];
                    totalRdCost += costUncoded[blkPos + x];
                }
                blkPos += trSize;
            }
        }
    }
    else
    {
        // non-psy path
        for (int cgScanPos = cgLastScanPos + 1; cgScanPos < (int)cgNum ; cgScanPos++)
        {
            X265_CHECK(coeffNum[cgScanPos] == 0, "count of coeff failure\n");

            uint32_t scanPosBase = (cgScanPos << MLS_CG_SIZE);
            uint32_t blkPos      = codeParams.scan[scanPosBase];

            for (int y = 0; y < MLS_CG_SIZE; y++)
            {
                for (int x = 0; x < MLS_CG_SIZE; x++)
                {
                    int signCoef = m_resiDctCoeff[blkPos + x];            /* pre-quantization DCT coeff */
                    costUncoded[blkPos + x] = ((int64_t)signCoef * signCoef) << scaleBits;

                    totalUncodedCost += costUncoded[blkPos + x];
                    totalRdCost += costUncoded[blkPos + x];
                }
                blkPos += trSize;
            }
        }
    }

    static const uint8_t table_cnt[5][SCAN_SET_SIZE] =
    {
        // patternSigCtx = 0
        {
            2, 1, 1, 0,
            1, 1, 0, 0,
            1, 0, 0, 0,
            0, 0, 0, 0,
        },
        // patternSigCtx = 1
        {
            2, 2, 2, 2,
            1, 1, 1, 1,
            0, 0, 0, 0,
            0, 0, 0, 0,
        },
        // patternSigCtx = 2
        {
            2, 1, 0, 0,
            2, 1, 0, 0,
            2, 1, 0, 0,
            2, 1, 0, 0,
        },
        // patternSigCtx = 3
        {
            2, 2, 2, 2,
            2, 2, 2, 2,
            2, 2, 2, 2,
            2, 2, 2, 2,
        },
        // 4x4
        {
            0, 1, 4, 5,
            2, 3, 4, 5,
            6, 6, 8, 8,
            7, 7, 8, 8
        }
    };

    /* iterate over coding groups in reverse scan order */
    for (int cgScanPos = cgLastScanPos; cgScanPos >= 0; cgScanPos--)
    {
        uint32_t ctxSet = (cgScanPos && bIsLuma) ? 2 : 0;
        const uint32_t cgBlkPos = codeParams.scanCG[cgScanPos];
        const uint32_t cgPosY   = cgBlkPos >> codeParams.log2TrSizeCG;
        const uint32_t cgPosX   = cgBlkPos - (cgPosY << codeParams.log2TrSizeCG);
        const uint64_t cgBlkPosMask = ((uint64_t)1 << cgBlkPos);
        const int patternSigCtx = calcPatternSigCtx(sigCoeffGroupFlag64, cgPosX, cgPosY, cgBlkPos, cgStride);
        const int ctxSigOffset = codeParams.firstSignificanceMapContext + (cgScanPos && bIsLuma ? 3 : 0);

        if (c1 == 0)
            ctxSet++;
        c1 = 1;

        if (cgScanPos && (coeffNum[cgScanPos] == 0))
        {
            // TODO: does we need zero-coeff cost?
            const uint32_t scanPosBase = (cgScanPos << MLS_CG_SIZE);
            uint32_t blkPos = codeParams.scan[scanPosBase];

            if (usePsyMask)
            {
                // TODO: we can't SIMD optimize because PSYVALUE need 64-bits multiplication, convert to Double can work faster by FMA
                for (int y = 0; y < MLS_CG_SIZE; y++)
                {
                    for (int x = 0; x < MLS_CG_SIZE; x++)
                    {
                        int signCoef         = m_resiDctCoeff[blkPos + x];            /* pre-quantization DCT coeff */
                        int predictedCoef    = m_fencDctCoeff[blkPos + x] - signCoef; /* predicted DCT = source DCT - residual DCT*/

                        costUncoded[blkPos + x] = ((int64_t)signCoef * signCoef) << scaleBits;

                        /* when no residual coefficient is coded, predicted coef == recon coef */
                        costUncoded[blkPos + x] -= PSYVALUE(predictedCoef);

                        totalUncodedCost += costUncoded[blkPos + x];
                        totalRdCost += costUncoded[blkPos + x];

                        const uint32_t scanPosOffset =  y * MLS_CG_SIZE + x;
                        const uint32_t ctxSig = table_cnt[patternSigCtx][g_scan4x4[codeParams.scanType][scanPosOffset]] + ctxSigOffset;
                        X265_CHECK(trSize > 4, "trSize check failure\n");
                        X265_CHECK(ctxSig == getSigCtxInc(patternSigCtx, log2TrSize, trSize, codeParams.scan[scanPosBase + scanPosOffset], bIsLuma, codeParams.firstSignificanceMapContext), "sigCtx check failure\n");

                        costSig[scanPosBase + scanPosOffset] = SIGCOST(estBitsSbac.significantBits[0][ctxSig]);
                        costCoeff[scanPosBase + scanPosOffset] = costUncoded[blkPos + x];
                        sigRateDelta[blkPos + x] = estBitsSbac.significantBits[1][ctxSig] - estBitsSbac.significantBits[0][ctxSig];
                    }
                    blkPos += trSize;
                }
            }
            else
            {
                // non-psy path
                for (int y = 0; y < MLS_CG_SIZE; y++)
                {
                    for (int x = 0; x < MLS_CG_SIZE; x++)
                    {
                        int signCoef = m_resiDctCoeff[blkPos + x];            /* pre-quantization DCT coeff */
                        costUncoded[blkPos + x] = ((int64_t)signCoef * signCoef) << scaleBits;

                        totalUncodedCost += costUncoded[blkPos + x];
                        totalRdCost += costUncoded[blkPos + x];

                        const uint32_t scanPosOffset =  y * MLS_CG_SIZE + x;
                        const uint32_t ctxSig = table_cnt[patternSigCtx][g_scan4x4[codeParams.scanType][scanPosOffset]] + ctxSigOffset;
                        X265_CHECK(trSize > 4, "trSize check failure\n");
                        X265_CHECK(ctxSig == getSigCtxInc(patternSigCtx, log2TrSize, trSize, codeParams.scan[scanPosBase + scanPosOffset], bIsLuma, codeParams.firstSignificanceMapContext), "sigCtx check failure\n");

                        costSig[scanPosBase + scanPosOffset] = SIGCOST(estBitsSbac.significantBits[0][ctxSig]);
                        costCoeff[scanPosBase + scanPosOffset] = costUncoded[blkPos + x];
                        sigRateDelta[blkPos + x] = estBitsSbac.significantBits[1][ctxSig] - estBitsSbac.significantBits[0][ctxSig];
                    }
                    blkPos += trSize;
                }
            }

            /* there were no coded coefficients in this coefficient group */
            {
                uint32_t ctxSig = getSigCoeffGroupCtxInc(sigCoeffGroupFlag64, cgPosX, cgPosY, cgBlkPos, cgStride);
                costCoeffGroupSig[cgScanPos] = SIGCOST(estBitsSbac.significantCoeffGroupBits[ctxSig][0]);
                totalRdCost += costCoeffGroupSig[cgScanPos];  /* add cost of 0 bit in significant CG bitmap */
            }
            continue;
        }

        coeffGroupRDStats cgRdStats;
        memset(&cgRdStats, 0, sizeof(coeffGroupRDStats));

        uint32_t subFlagMask = coeffFlag[cgScanPos];
        int    c2            = 0;
        uint32_t goRiceParam = 0;
        uint32_t c1Idx       = 0;
        uint32_t c2Idx       = 0;
        /* iterate over coefficients in each group in reverse scan order */
        for (int scanPosinCG = cgSize - 1; scanPosinCG >= 0; scanPosinCG--)
        {
            scanPos              = (cgScanPos << MLS_CG_SIZE) + scanPosinCG;
            uint32_t blkPos      = codeParams.scan[scanPos];
            uint32_t maxAbsLevel = abs(dstCoeff[blkPos]);             /* abs(quantized coeff) */
            int signCoef         = m_resiDctCoeff[blkPos];            /* pre-quantization DCT coeff */
            int predictedCoef    = m_fencDctCoeff[blkPos] - signCoef; /* predicted DCT = source DCT - residual DCT*/

            /* RDOQ measures distortion as the squared difference between the unquantized coded level
             * and the original DCT coefficient. The result is shifted scaleBits to account for the
             * FIX15 nature of the CABAC cost tables minus the forward transform scale */

            /* cost of not coding this coefficient (all distortion, no signal bits) */
            costUncoded[blkPos] = ((int64_t)signCoef * signCoef) << scaleBits;
            X265_CHECK((!!scanPos ^ !!blkPos) == 0, "failed on (blkPos=0 && scanPos!=0)\n");
            if (usePsyMask & scanPos)
                /* when no residual coefficient is coded, predicted coef == recon coef */
                costUncoded[blkPos] -= PSYVALUE(predictedCoef);

            totalUncodedCost += costUncoded[blkPos];

            // coefficient level estimation
            const int* greaterOneBits = estBitsSbac.greaterOneBits[4 * ctxSet + c1];
            //const uint32_t ctxSig = (blkPos == 0) ? 0 : table_cnt[(trSize == 4) ? 4 : patternSigCtx][g_scan4x4[codeParams.scanType][scanPosinCG]] + ctxSigOffset;
            static const uint64_t table_cnt64[4] = {0x0000000100110112ULL, 0x0000000011112222ULL, 0x0012001200120012ULL, 0x2222222222222222ULL};
            uint64_t ctxCnt = table_cnt64[patternSigCtx];
            if (trSize == 4)
                ctxCnt = 0x8877886654325410ULL;
            const uint32_t ctxSig = (blkPos == 0) ? 0 : ((ctxCnt >> (4 * g_scan4x4[codeParams.scanType][scanPosinCG])) & 0xF) + ctxSigOffset;
            // NOTE: above equal to 'table_cnt[(trSize == 4) ? 4 : patternSigCtx][g_scan4x4[codeParams.scanType][scanPosinCG]] + ctxSigOffset'
            X265_CHECK(ctxSig == getSigCtxInc(patternSigCtx, log2TrSize, trSize, blkPos, bIsLuma, codeParams.firstSignificanceMapContext), "sigCtx check failure\n");

            // before find lastest non-zero coeff
            if (scanPos > (uint32_t)lastScanPos)
            {
                /* coefficients after lastNZ have no distortion signal cost */
                costCoeff[scanPos] = 0;
                costSig[scanPos] = 0;

                /* No non-zero coefficient yet found, but this does not mean
                 * there is no uncoded-cost for this coefficient. Pre-
                 * quantization the coefficient may have been non-zero */
                totalRdCost += costUncoded[blkPos];
            }
            else if (!(subFlagMask & 1))
            {
                // fast zero coeff path
                /* set default costs to uncoded costs */
                costSig[scanPos] = SIGCOST(estBitsSbac.significantBits[0][ctxSig]);
                costCoeff[scanPos] = costUncoded[blkPos] + costSig[scanPos];
                sigRateDelta[blkPos] = estBitsSbac.significantBits[1][ctxSig] - estBitsSbac.significantBits[0][ctxSig];
                totalRdCost += costCoeff[scanPos];
                rateIncUp[blkPos] = greaterOneBits[0];

                subFlagMask >>= 1;
            }
            else
            {
                subFlagMask >>= 1;

                const uint32_t c1c2Idx = ((c1Idx - 8) >> (sizeof(int) * CHAR_BIT - 1)) + (((-(int)c2Idx) >> (sizeof(int) * CHAR_BIT - 1)) + 1) * 2;
                const uint32_t baseLevel = ((uint32_t)0xD9 >> (c1c2Idx * 2)) & 3;  // {1, 2, 1, 3}

                X265_CHECK(!!((int)c1Idx < C1FLAG_NUMBER) == (int)((c1Idx - 8) >> (sizeof(int) * CHAR_BIT - 1)), "scan validation 1\n");
                X265_CHECK(!!(c2Idx == 0) == ((-(int)c2Idx) >> (sizeof(int) * CHAR_BIT - 1)) + 1, "scan validation 2\n");
                X265_CHECK((int)baseLevel == ((c1Idx < C1FLAG_NUMBER) ? (2 + (c2Idx == 0)) : 1), "scan validation 3\n");

                // coefficient level estimation
                const int* levelAbsBits = estBitsSbac.levelAbsBits[ctxSet + c2];

                uint32_t level = 0;
                uint32_t sigCoefBits = 0;
                costCoeff[scanPos] = MAX_INT64;

                if ((int)scanPos == lastScanPos)
                    sigRateDelta[blkPos] = 0;
                else
                {
                    if (maxAbsLevel < 3)
                    {
                        /* set default costs to uncoded costs */
                        costSig[scanPos] = SIGCOST(estBitsSbac.significantBits[0][ctxSig]);
                        costCoeff[scanPos] = costUncoded[blkPos] + costSig[scanPos];
                    }
                    sigRateDelta[blkPos] = estBitsSbac.significantBits[1][ctxSig] - estBitsSbac.significantBits[0][ctxSig];
                    sigCoefBits = estBitsSbac.significantBits[1][ctxSig];
                }

                // NOTE: X265_MAX(maxAbsLevel - 1, 1) ==> (X>=2 -> X-1), (X<2 -> 1)  | (0 < X < 2 ==> X=1)
                if (maxAbsLevel == 1)
                {
                    uint32_t levelBits = (c1c2Idx & 1) ? greaterOneBits[0] + IEP_RATE : ((1 + goRiceParam) << 15) + IEP_RATE;
                    X265_CHECK(levelBits == getICRateCost(1, 1 - baseLevel, greaterOneBits, levelAbsBits, goRiceParam, c1c2Idx) + IEP_RATE, "levelBits mistake\n");

                    int unquantAbsLevel = UNQUANT(1);
                    int d = abs(signCoef) - unquantAbsLevel;
                    int64_t curCost = RDCOST(d, sigCoefBits + levelBits);

                    /* Psy RDOQ: bias in favor of higher AC coefficients in the reconstructed frame */
                    if (usePsyMask & scanPos)
                    {
                        int reconCoef = abs(unquantAbsLevel + SIGN(predictedCoef, signCoef));
                        curCost -= PSYVALUE(reconCoef);
                    }

                    if (curCost < costCoeff[scanPos])
                    {
                        level = 1;
                        costCoeff[scanPos] = curCost;
                        costSig[scanPos] = SIGCOST(sigCoefBits);
                    }
                }
                else if (maxAbsLevel)
                {
                    uint32_t levelBits0 = getICRateCost(maxAbsLevel,     maxAbsLevel     - baseLevel, greaterOneBits, levelAbsBits, goRiceParam, c1c2Idx) + IEP_RATE;
                    uint32_t levelBits1 = getICRateCost(maxAbsLevel - 1, maxAbsLevel - 1 - baseLevel, greaterOneBits, levelAbsBits, goRiceParam, c1c2Idx) + IEP_RATE;

                    int unquantAbsLevel0 = UNQUANT(maxAbsLevel);
                    int d0 = abs(signCoef) - unquantAbsLevel0;
                    int64_t curCost0 = RDCOST(d0, sigCoefBits + levelBits0);

                    int unquantAbsLevel1 = UNQUANT(maxAbsLevel - 1);
                    int d1 = abs(signCoef) - unquantAbsLevel1;
                    int64_t curCost1 = RDCOST(d1, sigCoefBits + levelBits1);

                    /* Psy RDOQ: bias in favor of higher AC coefficients in the reconstructed frame */
                    if (usePsyMask & scanPos)
                    {
                        int reconCoef;
                        reconCoef = abs(unquantAbsLevel0 + SIGN(predictedCoef, signCoef));
                        curCost0 -= PSYVALUE(reconCoef);

                        reconCoef = abs(unquantAbsLevel1 + SIGN(predictedCoef, signCoef));
                        curCost1 -= PSYVALUE(reconCoef);
                    }
                    if (curCost0 < costCoeff[scanPos])
                    {
                        level = maxAbsLevel;
                        costCoeff[scanPos] = curCost0;
                        costSig[scanPos] = SIGCOST(sigCoefBits);
                    }
                    if (curCost1 < costCoeff[scanPos])
                    {
                        level = maxAbsLevel - 1;
                        costCoeff[scanPos] = curCost1;
                        costSig[scanPos] = SIGCOST(sigCoefBits);
                    }
                }

                dstCoeff[blkPos] = (int16_t)level;
                totalRdCost += costCoeff[scanPos];

                /* record costs for sign-hiding performed at the end */
                if ((cu.m_slice->m_pps->bSignHideEnabled ? ~0 : 0) & level)
                {
                    const int32_t diff0 = level - 1 - baseLevel;
                    const int32_t diff2 = level + 1 - baseLevel;
                    const int32_t maxVlc = g_goRiceRange[goRiceParam];
                    int rate0, rate1, rate2;

                    if (diff0 < -2)  // prob (92.9, 86.5, 74.5)%
                    {
                        // NOTE: Min: L - 1 - {1,2,1,3} < -2 ==> L < {0,1,0,2}
                        //            additional L > 0, so I got (L > 0 && L < 2) ==> L = 1
                        X265_CHECK(level == 1, "absLevel check failure\n");

                        const int rateEqual2 = greaterOneBits[1] + levelAbsBits[0];;
                        const int rateNotEqual2 = greaterOneBits[0];

                        rate0 = 0;
                        rate2 = rateEqual2;
                        rate1 = rateNotEqual2;

                        X265_CHECK(rate1 == getICRateNegDiff(level + 0, greaterOneBits, levelAbsBits), "rate1 check failure!\n");
                        X265_CHECK(rate2 == getICRateNegDiff(level + 1, greaterOneBits, levelAbsBits), "rate1 check failure!\n");
                        X265_CHECK(rate0 == getICRateNegDiff(level - 1, greaterOneBits, levelAbsBits), "rate1 check failure!\n");
                    }
                    else if (diff0 >= 0 && diff2 <= maxVlc)     // prob except from above path (98.6, 97.9, 96.9)%
                    {
                        // NOTE: no c1c2 correct rate since all of rate include this factor
                        rate1 = getICRateLessVlc(level + 0, diff0 + 1, goRiceParam);
                        rate2 = getICRateLessVlc(level + 1, diff0 + 2, goRiceParam);
                        rate0 = getICRateLessVlc(level - 1, diff0 + 0, goRiceParam);
                    }
                    else
                    {
                        rate1 = getICRate(level + 0, diff0 + 1, greaterOneBits, levelAbsBits, goRiceParam, maxVlc, c1c2Idx);
                        rate2 = getICRate(level + 1, diff0 + 2, greaterOneBits, levelAbsBits, goRiceParam, maxVlc, c1c2Idx);
                        rate0 = getICRate(level - 1, diff0 + 0, greaterOneBits, levelAbsBits, goRiceParam, maxVlc, c1c2Idx);
                    }
                    rateIncUp[blkPos] = rate2 - rate1;
                    rateIncDown[blkPos] = rate0 - rate1;
                }
                else
                {
                    rateIncUp[blkPos] = greaterOneBits[0];
                    rateIncDown[blkPos] = 0;
                }

                /* Update CABAC estimation state */
                if (level >= baseLevel && goRiceParam < 4 && level > (3U << goRiceParam))
                    goRiceParam++;

                c1Idx -= (-(int32_t)level) >> 31;

                /* update bin model */
                if (level > 1)
                {
                    c1 = 0;
                    c2 += (uint32_t)(c2 - 2) >> 31;
                    c2Idx++;
                }
                else if ((c1 < 3) && (c1 > 0) && level)
                    c1++;

                if (dstCoeff[blkPos])
                {
                    sigCoeffGroupFlag64 |= cgBlkPosMask;
                    cgRdStats.codedLevelAndDist += costCoeff[scanPos] - costSig[scanPos];
                    cgRdStats.uncodedDist += costUncoded[blkPos];
                    cgRdStats.nnzBeforePos0 += scanPosinCG;
                }
            }

            cgRdStats.sigCost += costSig[scanPos];
        } /* end for (scanPosinCG) */

        X265_CHECK((cgScanPos << MLS_CG_SIZE) == (int)scanPos, "scanPos mistake\n");
        cgRdStats.sigCost0 = costSig[scanPos];

        costCoeffGroupSig[cgScanPos] = 0;

        /* nothing to do at this case */
        X265_CHECK(cgLastScanPos >= 0, "cgLastScanPos check failure\n");

        if (!cgScanPos || cgScanPos == cgLastScanPos)
        {
            /* coeff group 0 is implied to be present, no signal cost */
            /* coeff group with last NZ is implied to be present, handled below */
        }
        else if (sigCoeffGroupFlag64 & cgBlkPosMask)
        {
            if (!cgRdStats.nnzBeforePos0)
            {
                /* if only coeff 0 in this CG is coded, its significant coeff bit is implied */
                totalRdCost -= cgRdStats.sigCost0;
                cgRdStats.sigCost -= cgRdStats.sigCost0;
            }

            /* there are coded coefficients in this group, but now we include the signaling cost
             * of the significant coefficient group flag and evaluate whether the RD cost of the
             * coded group is more than the RD cost of the uncoded group */

            uint32_t sigCtx = getSigCoeffGroupCtxInc(sigCoeffGroupFlag64, cgPosX, cgPosY, cgBlkPos, cgStride);

            int64_t costZeroCG = totalRdCost + SIGCOST(estBitsSbac.significantCoeffGroupBits[sigCtx][0]);
            costZeroCG += cgRdStats.uncodedDist;       /* add distortion for resetting non-zero levels to zero levels */
            costZeroCG -= cgRdStats.codedLevelAndDist; /* remove distortion and level cost of coded coefficients */
            costZeroCG -= cgRdStats.sigCost;           /* remove signaling cost of significant coeff bitmap */

            costCoeffGroupSig[cgScanPos] = SIGCOST(estBitsSbac.significantCoeffGroupBits[sigCtx][1]);
            totalRdCost += costCoeffGroupSig[cgScanPos];  /* add the cost of 1 bit in significant CG bitmap */

            if (costZeroCG < totalRdCost && m_rdoqLevel > 1)
            {
                sigCoeffGroupFlag64 &= ~cgBlkPosMask;
                totalRdCost = costZeroCG;
                costCoeffGroupSig[cgScanPos] = SIGCOST(estBitsSbac.significantCoeffGroupBits[sigCtx][0]);

                /* reset all coeffs to 0. UNCODE THIS COEFF GROUP! */
                const uint32_t blkPos = codeParams.scan[cgScanPos * cgSize];
                memset(&dstCoeff[blkPos + 0 * trSize], 0, 4 * sizeof(*dstCoeff));
                memset(&dstCoeff[blkPos + 1 * trSize], 0, 4 * sizeof(*dstCoeff));
                memset(&dstCoeff[blkPos + 2 * trSize], 0, 4 * sizeof(*dstCoeff));
                memset(&dstCoeff[blkPos + 3 * trSize], 0, 4 * sizeof(*dstCoeff));
            }
        }
        else
        {
            /* there were no coded coefficients in this coefficient group */
            uint32_t ctxSig = getSigCoeffGroupCtxInc(sigCoeffGroupFlag64, cgPosX, cgPosY, cgBlkPos, cgStride);
            costCoeffGroupSig[cgScanPos] = SIGCOST(estBitsSbac.significantCoeffGroupBits[ctxSig][0]);
            totalRdCost += costCoeffGroupSig[cgScanPos];  /* add cost of 0 bit in significant CG bitmap */
            totalRdCost -= cgRdStats.sigCost;             /* remove cost of significant coefficient bitmap */
        }
    } /* end for (cgScanPos) */

    X265_CHECK(lastScanPos >= 0, "numSig non zero, but no coded CG\n");

    /* calculate RD cost of uncoded block CBF=0, and add cost of CBF=1 to total */
    int64_t bestCost;
    if (!cu.isIntra(absPartIdx) && bIsLuma && !cu.m_tuDepth[absPartIdx])
    {
        bestCost = totalUncodedCost + SIGCOST(estBitsSbac.blockRootCbpBits[0]);
        totalRdCost += SIGCOST(estBitsSbac.blockRootCbpBits[1]);
    }
    else
    {
        int ctx = ctxCbf[ttype][cu.m_tuDepth[absPartIdx]];
        bestCost = totalUncodedCost + SIGCOST(estBitsSbac.blockCbpBits[ctx][0]);
        totalRdCost += SIGCOST(estBitsSbac.blockCbpBits[ctx][1]);
    }

    /* This loop starts with the last non-zero found in the first loop and then refines this last
     * non-zero by measuring the true RD cost of the last NZ at this position, and then the RD costs
     * at all previous coefficients until a coefficient greater than 1 is encountered or we run out
     * of coefficients to evaluate.  This will factor in the cost of coding empty groups and empty
     * coeff prior to the last NZ. The base best cost is the RD cost of CBF=0 */
    int  bestLastIdx = 0;
    bool foundLast = false;
    for (int cgScanPos = cgLastScanPos; cgScanPos >= 0 && !foundLast; cgScanPos--)
    {
        if (!cgScanPos || cgScanPos == cgLastScanPos)
        {
            /* the presence of these coefficient groups are inferred, they have no bit in
             * sigCoeffGroupFlag64 and no saved costCoeffGroupSig[] cost */
        }
        else if (sigCoeffGroupFlag64 & (1ULL << codeParams.scanCG[cgScanPos]))
        {
            /* remove cost of significant coeff group flag, the group's presence would be inferred
             * from lastNZ if it were present in this group */
            totalRdCost -= costCoeffGroupSig[cgScanPos];
        }
        else
        {
            /* remove cost of signaling this empty group as not present */
            totalRdCost -= costCoeffGroupSig[cgScanPos];
            continue;
        }

        for (int scanPosinCG = cgSize - 1; scanPosinCG >= 0; scanPosinCG--)
        {
            scanPos = cgScanPos * cgSize + scanPosinCG;
            if ((int)scanPos > lastScanPos)
                continue;

            /* if the coefficient was coded, measure the RD cost of it as the last non-zero and then
             * continue as if it were uncoded. If the coefficient was already uncoded, remove the
             * cost of signaling it as not-significant */
            uint32_t blkPos = codeParams.scan[scanPos];
            if (dstCoeff[blkPos])
            {
                // Calculates the cost of signaling the last significant coefficient in the block 
                uint32_t pos[2] = { (blkPos & (trSize - 1)), (blkPos >> log2TrSize) };
                if (codeParams.scanType == SCAN_VER)
                    std::swap(pos[0], pos[1]);
                uint32_t bitsLastNZ = 0;

                for (int i = 0; i < 2; i++)
                {
                    int temp = g_lastCoeffTable[pos[i]];
                    int prefixOnes = temp & 15;
                    int suffixLen = temp >> 4;

                    bitsLastNZ += m_entropyCoder->m_estBitsSbac.lastBits[i][prefixOnes];
                    bitsLastNZ += IEP_RATE * suffixLen;
                }

                int64_t costAsLast = totalRdCost - costSig[scanPos] + SIGCOST(bitsLastNZ);

                if (costAsLast < bestCost)
                {
                    bestLastIdx = scanPos + 1;
                    bestCost = costAsLast;
                }
                if (dstCoeff[blkPos] > 1 || m_rdoqLevel == 1)
                {
                    foundLast = true;
                    break;
                }

                totalRdCost -= costCoeff[scanPos];
                totalRdCost += costUncoded[blkPos];
            }
            else
                totalRdCost -= costSig[scanPos];
        }
    }

    /* recount non-zero coefficients and re-apply sign of DCT coef */
    numSig = 0;
    for (int pos = 0; pos < bestLastIdx; pos++)
    {
        int blkPos = codeParams.scan[pos];
        int level  = dstCoeff[blkPos];
        numSig += (level != 0);

        uint32_t mask = (int32_t)m_resiDctCoeff[blkPos] >> 31;
        dstCoeff[blkPos] = (int16_t)((level ^ mask) - mask);
    }

    // Average 49.62 pixels
    /* clean uncoded coefficients */
    for (int pos = bestLastIdx; pos <= fastMin(lastScanPos, (bestLastIdx | (SCAN_SET_SIZE - 1))); pos++)
    {
        dstCoeff[codeParams.scan[pos]] = 0;
    }
    for (int pos = (bestLastIdx & ~(SCAN_SET_SIZE - 1)) + SCAN_SET_SIZE; pos <= lastScanPos; pos += SCAN_SET_SIZE)
    {
        const uint32_t blkPos = codeParams.scan[pos];
        memset(&dstCoeff[blkPos + 0 * trSize], 0, 4 * sizeof(*dstCoeff));
        memset(&dstCoeff[blkPos + 1 * trSize], 0, 4 * sizeof(*dstCoeff));
        memset(&dstCoeff[blkPos + 2 * trSize], 0, 4 * sizeof(*dstCoeff));
        memset(&dstCoeff[blkPos + 3 * trSize], 0, 4 * sizeof(*dstCoeff));
    }

    /* rate-distortion based sign-hiding */
    if (cu.m_slice->m_pps->bSignHideEnabled && numSig >= 2)
    {
        const int realLastScanPos = (bestLastIdx - 1) >> LOG2_SCAN_SET_SIZE;
        int lastCG = true;
        for (int subSet = realLastScanPos; subSet >= 0; subSet--)
        {
            int subPos = subSet << LOG2_SCAN_SET_SIZE;
            int n;

            if (!(sigCoeffGroupFlag64 & (1ULL << codeParams.scanCG[subSet])))
                continue;

            /* measure distance between first and last non-zero coef in this
             * coding group */
            const uint32_t posFirstLast = primitives.findPosFirstLast(&dstCoeff[codeParams.scan[subPos]], trSize, g_scan4x4[codeParams.scanType]);
            int firstNZPosInCG = (uint16_t)posFirstLast;
            int lastNZPosInCG = posFirstLast >> 16;


            if (lastNZPosInCG - firstNZPosInCG >= SBH_THRESHOLD)
            {
                uint32_t signbit = (dstCoeff[codeParams.scan[subPos + firstNZPosInCG]] > 0 ? 0 : 1);
                int absSum = 0;

                for (n = firstNZPosInCG; n <= lastNZPosInCG; n++)
                    absSum += dstCoeff[codeParams.scan[n + subPos]];

                if (signbit != (absSum & 1U))
                {
                    /* We must find a coeff to toggle up or down so the sign bit of the first non-zero coeff
                     * is properly implied. Note dstCoeff[] are signed by this point but curChange and
                     * finalChange imply absolute levels (+1 is away from zero, -1 is towards zero) */

                    int64_t minCostInc = MAX_INT64, curCost = MAX_INT64;
                    int minPos = -1;
                    int16_t finalChange = 0, curChange = 0;

                    for (n = (lastCG ? lastNZPosInCG : SCAN_SET_SIZE - 1); n >= 0; --n)
                    {
                        uint32_t blkPos = codeParams.scan[n + subPos];
                        int signCoef    = m_resiDctCoeff[blkPos]; /* pre-quantization DCT coeff */
                        int absLevel    = abs(dstCoeff[blkPos]);

                        int d = abs(signCoef) - UNQUANT(absLevel);
                        int64_t origDist = (((int64_t)d * d)) << scaleBits;

#define DELTARDCOST(d, deltabits) ((((int64_t)d * d) << scaleBits) - origDist + ((lambda2 * (int64_t)(deltabits)) >> 8))

                        if (dstCoeff[blkPos])
                        {
                            d = abs(signCoef) - UNQUANT(absLevel + 1);
                            int64_t costUp = DELTARDCOST(d, rateIncUp[blkPos]);

                            /* if decrementing would make the coeff 0, we can include the
                             * significant coeff flag cost savings */
                            d = abs(signCoef) - UNQUANT(absLevel - 1);
                            bool isOne = abs(dstCoeff[blkPos]) == 1;
                            int downBits = rateIncDown[blkPos] - (isOne ? (IEP_RATE + sigRateDelta[blkPos]) : 0);
                            int64_t costDown = DELTARDCOST(d, downBits);

                            if (lastCG && lastNZPosInCG == n && isOne)
                                costDown -= 4 * IEP_RATE;

                            if (costUp < costDown)
                            {
                                curCost = costUp;
                                curChange =  1;
                            }
                            else
                            {
                                curChange = -1;
                                if (n == firstNZPosInCG && isOne)
                                    curCost = MAX_INT64;
                                else
                                    curCost = costDown;
                            }
                        }
                        else if (n < firstNZPosInCG && signbit != (signCoef >= 0 ? 0 : 1U))
                        {
                            /* don't try to make a new coded coeff before the first coeff if its
                             * sign would be different than the first coeff, the inferred sign would
                             * still be wrong and we'd have to do this again. */
                            curCost = MAX_INT64;
                        }
                        else
                        {
                            /* evaluate changing an uncoded coeff 0 to a coded coeff +/-1 */
                            d = abs(signCoef) - UNQUANT(1);
                            curCost = DELTARDCOST(d, rateIncUp[blkPos] + IEP_RATE + sigRateDelta[blkPos]);
                            curChange = 1;
                        }

                        if (curCost < minCostInc)
                        {
                            minCostInc = curCost;
                            finalChange = curChange;
                            minPos = blkPos;
                        }
                    }

                    if (dstCoeff[minPos] == 32767 || dstCoeff[minPos] == -32768)
                        /* don't allow sign hiding to violate the SPEC range */
                        finalChange = -1;

                    if (dstCoeff[minPos] == 0)
                        numSig++;
                    else if (finalChange == -1 && abs(dstCoeff[minPos]) == 1)
                        numSig--;

                    if (m_resiDctCoeff[minPos] >= 0)
                        dstCoeff[minPos] += finalChange;
                    else
                        dstCoeff[minPos] -= finalChange;
                }
            }

            lastCG = false;
        }
    }

    return numSig;
}

/* Context derivation process of coeff_abs_significant_flag */
uint32_t Quant::getSigCtxInc(uint32_t patternSigCtx, uint32_t log2TrSize, uint32_t trSize, uint32_t blkPos, bool bIsLuma,
                             uint32_t firstSignificanceMapContext)
{
    static const uint8_t ctxIndMap[16] =
    {
        0, 1, 4, 5,
        2, 3, 4, 5,
        6, 6, 8, 8,
        7, 7, 8, 8
    };

    if (!blkPos) // special case for the DC context variable
        return 0;

    if (log2TrSize == 2) // 4x4
        return ctxIndMap[blkPos];

    const uint32_t posY = blkPos >> log2TrSize;
    const uint32_t posX = blkPos & (trSize - 1);
    X265_CHECK((blkPos - (posY << log2TrSize)) == posX, "block pos check failed\n");

    int posXinSubset = blkPos & 3;
    X265_CHECK((posX & 3) == (blkPos & 3), "pos alignment fail\n");
    int posYinSubset = posY & 3;

    // NOTE: [patternSigCtx][posXinSubset][posYinSubset]
    static const uint8_t table_cnt[4][4][4] =
    {
        // patternSigCtx = 0
        {
            { 2, 1, 1, 0 },
            { 1, 1, 0, 0 },
            { 1, 0, 0, 0 },
            { 0, 0, 0, 0 },
        },
        // patternSigCtx = 1
        {
            { 2, 1, 0, 0 },
            { 2, 1, 0, 0 },
            { 2, 1, 0, 0 },
            { 2, 1, 0, 0 },
        },
        // patternSigCtx = 2
        {
            { 2, 2, 2, 2 },
            { 1, 1, 1, 1 },
            { 0, 0, 0, 0 },
            { 0, 0, 0, 0 },
        },
        // patternSigCtx = 3
        {
            { 2, 2, 2, 2 },
            { 2, 2, 2, 2 },
            { 2, 2, 2, 2 },
            { 2, 2, 2, 2 },
        }
    };

    int cnt = table_cnt[patternSigCtx][posXinSubset][posYinSubset];
    int offset = firstSignificanceMapContext;

    offset += cnt;

    return (bIsLuma && (posX | posY) >= 4) ? 3 + offset : offset;
}

