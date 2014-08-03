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

#include "common.h"
#include "primitives.h"
#include "quant.h"
#include "frame.h"
#include "TLibCommon/TComDataCU.h"
#include "TLibCommon/TComYuv.h"
#include "TLibCommon/ContextTables.h"

using namespace x265;

#define SIGN(x,y) ((x^(y >> 31))-(y >> 31))

namespace {

struct coeffGroupRDStats
{
    int    nnzBeforePos0;
    double codedLevelAndDist; // distortion and level cost only
    double uncodedDist;       // all zero coded block distortion
    double sigCost;
    double sigCost0;
};

inline int fastMin(int x, int y)
{
    return y + ((x - y) & ((x - y) >> (sizeof(int) * CHAR_BIT - 1))); // min(x, y)
}

inline void denoiseDct(coeff_t* dctCoef, uint32_t* resSum, uint16_t* offset, int numCoeff)
{
    for (int i = 0; i < numCoeff; i++)
    {
        int level = dctCoef[i];
        int sign = level >> 31;
        level = (level + sign) ^ sign;
        resSum[i] += level;
        level -= offset[i];
        dctCoef[i] = level < 0 ? 0 : (level ^ sign) - sign;
    }
}

inline int getICRate(uint32_t absLevel, int32_t diffLevel, const int *greaterOneBits, const int *levelAbsBits, uint32_t absGoRice, uint32_t c1c2Idx)
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
        const uint32_t maxVlc = g_goRiceRange[absGoRice];
        bool expGolomb = (symbol > maxVlc);

        if (expGolomb)
        {
            absLevel = symbol - maxVlc;

            // NOTE: mapping to x86 hardware instruction BSR
            unsigned long size;
            CLZ32(size, absLevel);
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

/* Calculates the cost for specific absolute transform level */
inline uint32_t getICRateCost(uint32_t absLevel, int32_t diffLevel, const int *greaterOneBits, const int *levelAbsBits, uint32_t absGoRice, uint32_t c1c2Idx)
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
                CLZ32(idx, symbol + 1);
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
}

bool Quant::init(bool useRDOQ, double psyScale, const ScalingList& scalingList)
{
    m_useRDOQ = useRDOQ;
    m_psyRdoqScale = (uint64_t)(psyScale * 256.0);
    m_scalingList = &scalingList;
    m_resiDctCoeff = X265_MALLOC(coeff_t, MAX_TR_SIZE * MAX_TR_SIZE * 2);
    m_fencDctCoeff = m_resiDctCoeff + (MAX_TR_SIZE * MAX_TR_SIZE);
    m_fencShortBuf = X265_MALLOC(int16_t, MAX_TR_SIZE * MAX_TR_SIZE);
    
    return m_resiDctCoeff && m_fencShortBuf;
}

Quant::~Quant()
{
    X265_FREE(m_resiDctCoeff);
    X265_FREE(m_fencShortBuf);
}

void Quant::setQPforQuant(TComDataCU* cu)
{
    int qpy = cu->getQP(0);
    int chFmt = cu->getChromaFormat();

    m_qpParam[TEXT_LUMA].setQpParam(qpy + QP_BD_OFFSET);
    setQPforQuant(qpy, TEXT_CHROMA_U, cu->m_slice->m_pps->chromaCbQpOffset, chFmt);
    setQPforQuant(qpy, TEXT_CHROMA_V, cu->m_slice->m_pps->chromaCrQpOffset, chFmt);
}

void Quant::setQPforQuant(int qpy, TextType ttype, int chromaQPOffset, int chFmt)
{
    X265_CHECK(ttype == TEXT_CHROMA_U || ttype == TEXT_CHROMA_V, "invalid ttype\n");

    int qp = Clip3(-QP_BD_OFFSET, 57, qpy + chromaQPOffset);
    if (qp >= 30)
    {
        if (chFmt == X265_CSP_I420)
            qp = g_chromaScale[qp];
        else
            qp = X265_MIN(qp, 51);
    }
    m_qpParam[ttype].setQpParam(qp + QP_BD_OFFSET);
}

/* To minimize the distortion only. No rate is considered */
uint32_t Quant::signBitHidingHDQ(coeff_t* coeff, int32_t* deltaU, uint32_t numSig, const TUEntropyCodingParameters &codingParameters)
{
    const uint32_t log2TrSizeCG = codingParameters.log2TrSizeCG;
    const uint16_t *scan = codingParameters.scan;
    bool lastCG = true;

    for (int cg = (1 << log2TrSizeCG * 2) - 1; cg >= 0; cg--)
    {
        int cgStartPos = cg << LOG2_SCAN_SET_SIZE;
        int n;

        for (n = SCAN_SET_SIZE - 1; n >= 0; --n)
            if (coeff[scan[n + cgStartPos]])
                break;
        if (n < 0)
            continue;

        int lastNZPosInCG = n;

        for (n = 0;; n++)
            if (coeff[scan[n + cgStartPos]])
                break;

        int firstNZPosInCG = n;

        if (lastNZPosInCG - firstNZPosInCG >= SBH_THRESHOLD)
        {
            uint32_t signbit = coeff[scan[cgStartPos + firstNZPosInCG]] > 0 ? 0 : 1;
            uint32_t absSum = 0;

            for (n = firstNZPosInCG; n <= lastNZPosInCG; n++)
                absSum += coeff[scan[n + cgStartPos]];

            if (signbit != (absSum & 0x1)) // compare signbit with sum_parity
            {
                int minCostInc = MAX_INT,  minPos = -1, finalChange = 0, curCost = MAX_INT, curChange = 0;

                for (n = (lastCG ? lastNZPosInCG : SCAN_SET_SIZE - 1); n >= 0; --n)
                {
                    uint32_t blkPos = scan[n + cgStartPos];
                    if (coeff[blkPos])
                    {
                        if (deltaU[blkPos] > 0)
                        {
                            curCost = -deltaU[blkPos];
                            curChange = 1;
                        }
                        else
                        {
                            if (n == firstNZPosInCG && abs(coeff[blkPos]) == 1)
                                curCost = MAX_INT;
                            else
                            {
                                curCost = deltaU[blkPos];
                                curChange = -1;
                            }
                        }
                    }
                    else
                    {
                        if (n < firstNZPosInCG)
                        {
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
                }

                /* do not allow change to violate coeff clamp */
                if (coeff[minPos] == 32767 || coeff[minPos] == -32768)
                    finalChange = -1;

                if (!coeff[minPos])
                    numSig++;
                else if (finalChange == -1 && abs(coeff[minPos]) == 1)
                    numSig--;

                if (m_resiDctCoeff[minPos] >= 0)
                    coeff[minPos] += finalChange;
                else
                    coeff[minPos] -= finalChange;
            }
        }

        lastCG = false;
    }

    return numSig;
}

uint32_t Quant::transformNxN(TComDataCU* cu,
                             pixel*      fenc,
                             uint32_t    fencStride,
                             int16_t*    residual,
                             uint32_t    stride,
                             coeff_t*    coeff,
                             uint32_t    log2TrSize,
                             TextType    ttype,
                             uint32_t    absPartIdx,
                             bool        useTransformSkip)
{
    if (cu->getCUTransquantBypass(absPartIdx))
    {
        X265_CHECK(log2TrSize >= 2 && log2TrSize <= 5, "Block size mistake!\n");
        return primitives.cvt16to32_cnt[log2TrSize - 2](coeff, residual, stride);
    }

    bool usePsy = m_psyRdoqScale && ttype == TEXT_LUMA && !useTransformSkip;
    int trSize = 1 << log2TrSize;

    X265_CHECK((cu->m_slice->m_sps->quadtreeTULog2MaxSize >= log2TrSize), "transform size too large\n");
    if (useTransformSkip)
    {
        int shift = MAX_TR_DYNAMIC_RANGE - X265_DEPTH - log2TrSize;

        if (shift >= 0)
            primitives.cvt16to32_shl(m_resiDctCoeff, residual, stride, shift, trSize);
        else
        {
            /* X265_DEPTH > 13 */
            shift = -shift;
            int offset = (1 << (shift - 1));
            for (int j = 0; j < trSize; j++)
                for (int k = 0; k < trSize; k++)
                    m_resiDctCoeff[j * trSize + k] = (residual[j * stride + k] + offset) >> shift;
        }
    }
    else
    {
        const uint32_t sizeIdx = log2TrSize - 2;
        int useDST = !sizeIdx && ttype == TEXT_LUMA && cu->getPredictionMode(absPartIdx) == MODE_INTRA;
        int index = DCT_4x4 + sizeIdx - useDST;

        primitives.dct[index](residual, m_resiDctCoeff, stride);

        /* NOTE: if RDOQ is disabled globally, psy-rdoq is also disabled, so
         * there is no risk of performing this DCT unnecessarily */
        if (usePsy)
        {
            /* perform DCT on source pixels for psy-rdoq */
            primitives.square_copy_ps[sizeIdx](m_fencShortBuf, trSize, fenc, fencStride);
            primitives.dct[index](m_fencShortBuf, m_fencDctCoeff, trSize);
        }

        if (m_nr->bNoiseReduction && !useDST)
        {
            denoiseDct(m_resiDctCoeff, m_nr->residualSum[sizeIdx], m_nr->offset[sizeIdx], trSize << 1);
            m_nr->count[sizeIdx]++;
        }
    }

    if (m_useRDOQ)
        return rdoQuant(cu, coeff, log2TrSize, ttype, absPartIdx, usePsy);
    else
    {
        int deltaU[32 * 32];

        int scalingListType = ttype + (cu->isIntra(absPartIdx) ? 0 : 3);
        int rem = m_qpParam[ttype].rem;
        int per = m_qpParam[ttype].per;
        int32_t *quantCoeff = m_scalingList->m_quantCoef[log2TrSize - 2][scalingListType][rem];

        int transformShift = MAX_TR_DYNAMIC_RANGE - X265_DEPTH - log2TrSize; // Represents scaling through forward transform
        int qbits = QUANT_SHIFT + per + transformShift;
        int add = (cu->m_slice->m_sliceType == I_SLICE ? 171 : 85) << (qbits - 9);
        int numCoeff = 1 << log2TrSize * 2;

        uint32_t numSig = primitives.quant(m_resiDctCoeff, quantCoeff, deltaU, coeff, qbits, add, numCoeff);

        if (numSig >= 2 && cu->m_slice->m_pps->bSignHideEnabled)
        {
            TUEntropyCodingParameters codingParameters;
            cu->getTUEntropyCodingParameters(codingParameters, absPartIdx, log2TrSize, ttype == TEXT_LUMA);
            return signBitHidingHDQ(coeff, deltaU, numSig, codingParameters);
        }
        else
            return numSig;
    }
}

void Quant::invtransformNxN(bool transQuantBypass, int16_t* residual, uint32_t stride, coeff_t* coeff, uint32_t log2TrSize, TextType ttype, bool bIntra, bool useTransformSkip, uint32_t numSig)
{
    if (transQuantBypass)
    {
        int trSize = 1 << log2TrSize;
        for (int k = 0; k < trSize; k++)
            for (int j = 0; j < trSize; j++)
                residual[k * stride + j] = (int16_t)(coeff[k * trSize + j]);
        return;
    }

    // Values need to pass as input parameter in dequant
    int rem = m_qpParam[ttype].rem;
    int per = m_qpParam[ttype].per;
    int transformShift = MAX_TR_DYNAMIC_RANGE - X265_DEPTH - log2TrSize;
    int shift = QUANT_IQUANT_SHIFT - QUANT_SHIFT - transformShift;
    int numCoeff = 1 << log2TrSize * 2;

    if (m_scalingList->m_bEnabled)
    {
        int scalingListType = (bIntra ? 0 : 3) + ttype;
        int32_t *dequantCoef = m_scalingList->m_dequantCoef[log2TrSize - 2][scalingListType][rem];
        primitives.dequant_scaling(coeff, dequantCoef, m_resiDctCoeff, numCoeff, per, shift);
    }
    else
    {
        int scale = m_scalingList->s_invQuantScales[rem] << per;
        primitives.dequant_normal(coeff, m_resiDctCoeff, numCoeff, scale, shift);
    }

    if (useTransformSkip)
    {
        int trSize = 1 << log2TrSize;
        shift = transformShift;

        if (shift > 0)
            primitives.cvt32to16_shr(residual, m_resiDctCoeff, stride, shift, trSize);
        else
        {
            // The case when X265_DEPTH >= 13
            shift = -shift;
            for (int j = 0; j < trSize; j++)
                for (int k = 0; k < trSize; k++)
                    residual[j * stride + k] = (int16_t)m_resiDctCoeff[j * trSize + k] << shift;
        }
    }
    else
    {
        const uint32_t sizeIdx = log2TrSize - 2;
        int useDST = !sizeIdx && ttype == TEXT_LUMA && bIntra;

        X265_CHECK((int)numSig == primitives.count_nonzero(coeff, 1 << log2TrSize * 2), "numSig differ\n");

        // DC only
        if (numSig == 1 && coeff[0] != 0 && !useDST)
        {
            const int shift_1st = 7;
            const int add_1st = 1 << (shift_1st - 1);
            const int shift_2nd = 12 - (X265_DEPTH - 8);
            const int add_2nd = 1 << (shift_2nd - 1);

            int dc_val = (((m_resiDctCoeff[0] * 64 + add_1st) >> shift_1st) * 64 + add_2nd) >> shift_2nd;
            primitives.blockfill_s[sizeIdx](residual, stride, (int16_t)dc_val);
            return;
        }

        // TODO: this may need larger data types for X265_DEPTH > 10
        primitives.idct[IDCT_4x4 + sizeIdx - useDST](m_resiDctCoeff, residual, stride);
    }
}

/* Rate distortion optimized quantization for entropy coding engines using
 * probability models like CABAC */
uint32_t Quant::rdoQuant(TComDataCU* cu, coeff_t* dstCoeff, uint32_t log2TrSize, TextType ttype, uint32_t absPartIdx, bool usePsy)
{
    uint32_t trSize = 1 << log2TrSize;
    int transformShift = MAX_TR_DYNAMIC_RANGE - X265_DEPTH - log2TrSize; /* Represents scaling through forward transform */
    int scalingListType = (cu->isIntra(absPartIdx) ? 0 : 3) + ttype;

    X265_CHECK(scalingListType < 6, "scaling list type out of range\n");

    int rem = m_qpParam[ttype].rem;
    int per = m_qpParam[ttype].per;
    int qbits = QUANT_SHIFT + per + transformShift; /* Right shift of non-RDOQ quantizer level = (coeff*Q + offset)>>q_bits */
    int add = (1 << (qbits - 1));
    int32_t *qCoef = m_scalingList->m_quantCoef[log2TrSize - 2][scalingListType][rem];

    int numCoeff = 1 << log2TrSize * 2;
    int scaledCoeff[32 * 32];
    uint32_t numSig = primitives.nquant(m_resiDctCoeff, qCoef, scaledCoeff, dstCoeff, qbits, add, numCoeff);

    X265_CHECK((int)numSig == primitives.count_nonzero(dstCoeff, numCoeff), "numSig differ\n");
    if (!numSig)
        return 0;

    x265_emms();

    /* unquant constants for psy-rdoq */
    int32_t *unquantScale = m_scalingList->m_dequantCoef[log2TrSize - 2][scalingListType][rem];
    int unquantShift = QUANT_IQUANT_SHIFT - QUANT_SHIFT - transformShift;
    int unquantRound = 1 << (unquantShift - 1);
    int scaleBits = SCALE_BITS - 2 * transformShift;

    double lambda2 = m_lambdas[ttype];
    double *errScale = m_scalingList->m_errScale[log2TrSize - 2][scalingListType][rem];
    bool bIsLuma = ttype == TEXT_LUMA;

    double totalUncodedCost = 0;
    double costCoeff[32 * 32];   /* d*d + lambda * bits */
    double costUncoded[32 * 32]; /* d*d + lambda * 0    */
    double costSig[32 * 32];     /* lambda * bits       */

    int rateIncUp[32 * 32];      /* signal overhead of increasing level */
    int rateIncDown[32 * 32];    /* signal overhead of decreasing level */
    int sigRateDelta[32 * 32];   /* signal difference between zero and non-zero */
    int deltaU[32 * 32];

    double   costCoeffGroupSig[MLS_GRP_NUM]; /* lambda * bits of group coding cost */
    uint64_t sigCoeffGroupFlag64 = 0;

    uint32_t ctxSet      = 0;
    int    c1            = 1;
    int    c2            = 0;
    double baseCost      = 0;
    int    lastScanPos   = -1;
    uint32_t goRiceParam = 0;
    uint32_t c1Idx       = 0;
    uint32_t c2Idx       = 0;
    int cgLastScanPos    = -1;
    const uint32_t cgSize = (1 << MLS_CG_SIZE); /* 4x4 num coef = 16 */

    TUEntropyCodingParameters codingParameters;
    cu->getTUEntropyCodingParameters(codingParameters, absPartIdx, log2TrSize, bIsLuma);
    const uint32_t cgNum = 1 << codingParameters.log2TrSizeCG * 2;

    uint32_t scanPos;
    coeffGroupRDStats rdStats;

    /* iterate over coding groups in reverse scan order */
    for (int cgScanPos = cgNum - 1; cgScanPos >= 0; cgScanPos--)
    {
        const uint32_t cgBlkPos = codingParameters.scanCG[cgScanPos];
        const uint32_t cgPosY   = cgBlkPos >> codingParameters.log2TrSizeCG;
        const uint32_t cgPosX   = cgBlkPos - (cgPosY << codingParameters.log2TrSizeCG);
        const uint64_t cgBlkPosMask = ((uint64_t)1 << cgBlkPos);
        memset(&rdStats, 0, sizeof(coeffGroupRDStats));

        const int patternSigCtx = calcPatternSigCtx(sigCoeffGroupFlag64, cgPosX, cgPosY, codingParameters.log2TrSizeCG);

        /* iterate over coefficients in each group in reverse scan order */
        for (int scanPosinCG = cgSize - 1; scanPosinCG >= 0; scanPosinCG--)
        {
            scanPos              = (cgScanPos << MLS_CG_SIZE) + scanPosinCG;
            uint32_t blkPos      = codingParameters.scan[scanPos];
            double scaleFactor   = errScale[blkPos];       /* (1 << scaleBits) / (quantCoef * quantCoef) */
            int levelScaled      = scaledCoeff[blkPos];    /* abs(coef) * quantCoef */
            uint32_t maxAbsLevel = abs(dstCoeff[blkPos]);  /* abs(coef) */

            /* RDOQ measures distortion as the scaled level squared times a
             * scale factor which tries to remove the quantCoef back out, but
             * adds scaleBits to account for IEP_RATE which is 32k (1 << SCALE_BITS) */

            /* cost of not coding this coefficient (no signal bits) */
            costUncoded[scanPos] = ((uint64_t)levelScaled * levelScaled) * scaleFactor;
            totalUncodedCost += costUncoded[scanPos];

            if (maxAbsLevel && lastScanPos < 0)
            {
                /* remember the first non-zero coef found in this reverse scan as the last pos */
                lastScanPos   = scanPos;
                ctxSet        = (scanPos < SCAN_SET_SIZE || !bIsLuma) ? 0 : 2;
                cgLastScanPos = cgScanPos;
            }

            if (lastScanPos < 0)
            {
                /* No non-zero coefficient yet found, but this does not mean
                 * there is no uncoded-cost for this coefficient. Pre-
                 * quantization the coefficient may have been non-zero */
                costCoeff[scanPos] = 0;
                baseCost += costUncoded[scanPos];
            }
            else
            {
                const uint32_t c1c2Idx = ((c1Idx - 8) >> (sizeof(int) * CHAR_BIT - 1)) + (((-(int)c2Idx) >> (sizeof(int) * CHAR_BIT - 1)) + 1) * 2;
                const uint32_t baseLevel = ((uint32_t)0xD9 >> (c1c2Idx * 2)) & 3;  // {1, 2, 1, 3}

                X265_CHECK(!!((int)c1Idx < C1FLAG_NUMBER) == (int)((c1Idx - 8) >> (sizeof(int) * CHAR_BIT - 1)), "scan validation 1\n");
                X265_CHECK(!!(c2Idx == 0) == ((-(int)c2Idx) >> (sizeof(int) * CHAR_BIT - 1)) + 1, "scan validation 2\n");
                X265_CHECK((int)baseLevel == ((c1Idx < C1FLAG_NUMBER) ? (2 + (c2Idx == 0)) : 1), "scan validation 3\n");

                // coefficient level estimation
                const uint32_t oneCtx = 4 * ctxSet + c1;
                const uint32_t absCtx = ctxSet + c2;
                const int *greaterOneBits = m_estBitsSbac.greaterOneBits[oneCtx];
                const int *levelAbsBits = m_estBitsSbac.levelAbsBits[absCtx];

                uint32_t level = 0;
                uint32_t codedSigBits = 0;
                costCoeff[scanPos] = MAX_DOUBLE;

                if ((int)scanPos == lastScanPos)
                    sigRateDelta[blkPos] = 0;
                else
                {
                    const uint32_t ctxSig = getSigCtxInc(patternSigCtx, log2TrSize, trSize, blkPos, bIsLuma, codingParameters.firstSignificanceMapContext);
                    if (maxAbsLevel < 3)
                    {
                        /* set default costs to uncoded costs.
                         * TODO: is there really a need to check maxAbsLevel < 3 here?
                         * TODO: psy cost not considered for level = 0 */
                        costSig[scanPos] = lambda2 * m_estBitsSbac.significantBits[ctxSig][0];
                        costCoeff[scanPos] = costUncoded[scanPos] + costSig[scanPos];
                    }
                    sigRateDelta[blkPos] = m_estBitsSbac.significantBits[ctxSig][1] - m_estBitsSbac.significantBits[ctxSig][0];
                    codedSigBits = m_estBitsSbac.significantBits[ctxSig][1];
                }
                if (maxAbsLevel)
                {
                    int signCoef = m_resiDctCoeff[blkPos];
                    int predictedCoef = m_fencDctCoeff[blkPos] - signCoef;

                    const int64_t err1 = levelScaled - ((int64_t)maxAbsLevel << qbits);
                    double err2 = (double)(err1 * err1);

                    uint32_t minAbsLevel = X265_MAX(maxAbsLevel - 1, 1);
                    for (uint32_t lvl = maxAbsLevel; lvl >= minAbsLevel; lvl--)
                    {
                        uint32_t rateCost = getICRateCost(lvl, lvl - baseLevel, greaterOneBits, levelAbsBits, goRiceParam, c1c2Idx);
                        double curCost = err2 * scaleFactor + lambda2 * (codedSigBits + rateCost + IEP_RATE);

                        // Psy RDOQ: bias in favor of higher AC coefficients in the reconstructed frame
                        int psyValue = 0;
                        if (usePsy && blkPos)
                        {
                            int unquantAbsLevel = (lvl * (unquantScale[blkPos] << per) + unquantRound) >> unquantShift;
                            int reconCoef = abs(unquantAbsLevel + SIGN(predictedCoef, signCoef));
                            psyValue = (int)(((m_psyRdoqScale * reconCoef) << scaleBits) >> 8);
                        }

                        if (curCost - psyValue < costCoeff[scanPos])
                        {
                            level = lvl;
                            costCoeff[scanPos] = curCost - psyValue;
                            costSig[scanPos] = lambda2 * codedSigBits;
                        }

                        if (lvl > minAbsLevel)
                        {
                            // add deltas to get squared distortion at minAbsLevel
                            int64_t err3 = (int64_t)2 * err1 * ((int64_t)1 << qbits);
                            int64_t err4 = ((int64_t)1 << qbits) * ((int64_t)1 << qbits);
                            err2 += err3 + err4;
                        }
                    }
                }

                deltaU[blkPos] = (levelScaled - ((int)level << qbits)) >> (qbits - 8);
                dstCoeff[blkPos] = level;
                baseCost += costCoeff[scanPos];

                /* record costs for sign-hiding performed at the end */
                if (level)
                {
                    int rateNow = getICRate(level, level - baseLevel, greaterOneBits, levelAbsBits, goRiceParam, c1c2Idx);
                    rateIncUp[blkPos] = getICRate(level + 1, level + 1 - baseLevel, greaterOneBits, levelAbsBits, goRiceParam, c1c2Idx) - rateNow;
                    rateIncDown[blkPos] = getICRate(level - 1, level - 1 - baseLevel, greaterOneBits, levelAbsBits, goRiceParam, c1c2Idx) - rateNow;
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

                /* context set update */
                if (!(scanPos % SCAN_SET_SIZE) && scanPos)
                {
                    c2 = 0;
                    goRiceParam = 0;

                    c1Idx = 0;
                    c2Idx = 0;
                    ctxSet = (scanPos == SCAN_SET_SIZE || !bIsLuma) ? 0 : 2;
                    X265_CHECK(c1 >= 0, "c1 is negative\n");
                    ctxSet -= ((int32_t)(c1 - 1) >> 31);
                    c1 = 1;
                }
            }

            rdStats.sigCost += costSig[scanPos];
            if (scanPosinCG == 0)
                rdStats.sigCost0 = costSig[scanPos];

            if (dstCoeff[blkPos])
            {
                sigCoeffGroupFlag64 |= cgBlkPosMask;
                rdStats.codedLevelAndDist += costCoeff[scanPos] - costSig[scanPos];
                rdStats.uncodedDist += costUncoded[scanPos];
                if (scanPosinCG != 0)
                    rdStats.nnzBeforePos0++;
            }
        } /* end for (scanPosinCG) */

        /* Summarize costs for the coeff group */
        if (cgLastScanPos >= 0)
        {
            costCoeffGroupSig[cgScanPos] = 0;
            if (!cgScanPos)
            {
                /* coeff group 0 is always signaled, if numSig */
                sigCoeffGroupFlag64 |= cgBlkPosMask;
            }
            else
            {
                if (!(sigCoeffGroupFlag64 & cgBlkPosMask))
                {
                    /* this coeff group is already empty */
                    uint32_t ctxSig = getSigCoeffGroupCtxInc(sigCoeffGroupFlag64, cgPosX, cgPosY, codingParameters.log2TrSizeCG);
                    baseCost += lambda2 * m_estBitsSbac.significantCoeffGroupBits[ctxSig][0] - rdStats.sigCost;
                    costCoeffGroupSig[cgScanPos] = lambda2 * m_estBitsSbac.significantCoeffGroupBits[ctxSig][0];
                }
                else
                {
                    /* skip the last coefficient group, which will be handled together with last position below */
                    if (cgScanPos < cgLastScanPos)
                    {
                        if (!rdStats.nnzBeforePos0)
                        {
                            baseCost -= rdStats.sigCost0;
                            rdStats.sigCost -= rdStats.sigCost0;
                        }
                        /* rd-cost if SigCoeffGroupFlag = 0, initialization */
                        double costZeroCG = baseCost;

                        /* add SigCoeffGroupFlag cost to total cost */
                        uint32_t ctxSig = getSigCoeffGroupCtxInc(sigCoeffGroupFlag64, cgPosX, cgPosY, codingParameters.log2TrSizeCG);
                        if (cgScanPos < cgLastScanPos)
                        {
                            baseCost += lambda2 * m_estBitsSbac.significantCoeffGroupBits[ctxSig][1];
                            costZeroCG += lambda2 * m_estBitsSbac.significantCoeffGroupBits[ctxSig][0];
                            costCoeffGroupSig[cgScanPos] = lambda2 * m_estBitsSbac.significantCoeffGroupBits[ctxSig][1];
                        }

                        /* try to convert the current coeff group from non-zero to all-zero */
                        costZeroCG += rdStats.uncodedDist;       /* distortion for resetting non-zero levels to zero levels */
                        costZeroCG -= rdStats.codedLevelAndDist; /* distortion and level cost for keeping all non-zero levels */
                        costZeroCG -= rdStats.sigCost;           /* sig cost for all coeffs, including zero levels and non-zero levels */

                        /* if we can save cost, change this block to all-zero block */
                        if (costZeroCG < baseCost)
                        {
                            sigCoeffGroupFlag64 &= ~cgBlkPosMask;
                            baseCost = costZeroCG;
                            if (cgScanPos < cgLastScanPos)
                                costCoeffGroupSig[cgScanPos] = lambda2 * m_estBitsSbac.significantCoeffGroupBits[ctxSig][0];

                            /* reset all coeffs to 0. UNCODE THIS BLOCK! */
                            for (int scanPosinCG = cgSize - 1; scanPosinCG >= 0; scanPosinCG--)
                            {
                                scanPos         = cgScanPos * cgSize + scanPosinCG;
                                uint32_t blkPos = codingParameters.scan[scanPos];
                                if (dstCoeff[blkPos])
                                {
                                    costCoeff[scanPos] = costUncoded[scanPos];
                                    costSig[scanPos] = 0;
                                }
                                dstCoeff[blkPos] = 0;
                            }
                        }
                    }
                }
            }
        }
    } /* end for (cgScanPos) */

    if (lastScanPos < 0)
    {
        /* this should be un-possible */
        return 0;
    }

    /* estimate cost of uncoded block */
    double bestCost;
    if (!cu->isIntra(absPartIdx) && bIsLuma && !cu->getTransformIdx(absPartIdx))
    {
        bestCost  = totalUncodedCost + lambda2 * m_estBitsSbac.blockRootCbpBits[0][0];
        baseCost += lambda2 * m_estBitsSbac.blockRootCbpBits[0][1];
    }
    else
    {
        int ctx = cu->getCtxQtCbf(ttype, cu->getTransformIdx(absPartIdx));
        bestCost = totalUncodedCost + lambda2 * m_estBitsSbac.blockCbpBits[ctx][0];
        baseCost += lambda2 * m_estBitsSbac.blockCbpBits[ctx][1];
    }

    /* Find the least cost last non-zero coefficient position  */
    int  bestLastIdx = 0;
    bool foundLast = false;
    for (int cgScanPos = cgLastScanPos; cgScanPos >= 0 && !foundLast; cgScanPos--)
    {
        uint32_t cgBlkPos = codingParameters.scanCG[cgScanPos];
        baseCost -= costCoeffGroupSig[cgScanPos];

        if (!(sigCoeffGroupFlag64 & ((uint64_t)1 << cgBlkPos))) /* skip empty CGs */
            continue;

        for (int scanPosinCG = cgSize - 1; scanPosinCG >= 0; scanPosinCG--)
        {
            scanPos = cgScanPos * cgSize + scanPosinCG;
            if ((int)scanPos > lastScanPos)
                continue;

            uint32_t blkPos = codingParameters.scan[scanPos];
            if (dstCoeff[blkPos])
            {
                /* found the current last non-zero; estimate the trade-off of setting it to zero */
                uint32_t posY = blkPos >> log2TrSize;
                uint32_t posX = blkPos - (posY << log2TrSize);
                uint32_t costLast = codingParameters.scanType == SCAN_VER ? getRateLast(posY, posX) : getRateLast(posX, posY);
                double totalCost = baseCost + lambda2 * costLast - costSig[scanPos];

                /* TODO: perhaps psy-cost should be used here as well */

                if (totalCost < bestCost)
                {
                    bestLastIdx = scanPos + 1;
                    bestCost = totalCost;
                }
                if (dstCoeff[blkPos] > 1)
                {
                    foundLast = true;
                    break;
                }
                /* UNCODE THIS COEFF! */
                baseCost -= costCoeff[scanPos];
                baseCost += costUncoded[scanPos];
            }
            else
                baseCost -= costSig[scanPos];
        }
    }

    /* recount non-zero coefficients and re-apply sign of DCT coef */
    numSig = 0;
    for (int pos = 0; pos < bestLastIdx; pos++)
    {
        int blkPos = codingParameters.scan[pos];
        int level  = dstCoeff[blkPos];
        numSig += (level != 0);

        uint32_t mask = (int32_t)m_resiDctCoeff[blkPos] >> 31;
        dstCoeff[blkPos] = (level ^ mask) - mask;
    }

    /* clean uncoded coefficients */
    for (int pos = bestLastIdx; pos <= lastScanPos; pos++)
        dstCoeff[codingParameters.scan[pos]] = 0;

    /* rate-distortion based sign-hiding */
    if (cu->m_slice->m_pps->bSignHideEnabled && numSig >= 2)
    {
        int64_t invQuant = ScalingList::s_invQuantScales[rem] << per;
        int64_t rdFactor = (int64_t)((invQuant * invQuant) / (lambda2 * 16) + 0.5);

        int lastCG = true;
        for (int subSet = cgLastScanPos; subSet >= 0; subSet--)
        {
            int subPos = subSet << LOG2_SCAN_SET_SIZE;
            int n;

            /* measure distance between first and last non-zero coef in this
             * coding group */
            for (n = SCAN_SET_SIZE - 1; n >= 0; --n)
                if (dstCoeff[codingParameters.scan[n + subPos]])
                    break;
            if (n < 0)
                continue;

            int lastNZPosInCG = n;

            for (n = 0;; n++)
                if (dstCoeff[codingParameters.scan[n + subPos]])
                    break;

            int firstNZPosInCG = n;

            if (lastNZPosInCG - firstNZPosInCG >= SBH_THRESHOLD)
            {
                uint32_t signbit = (dstCoeff[codingParameters.scan[subPos + firstNZPosInCG]] > 0 ? 0 : 1);
                int absSum = 0;

                for (n = firstNZPosInCG; n <= lastNZPosInCG; n++)
                    absSum += dstCoeff[codingParameters.scan[n + subPos]];

                if (signbit != (absSum & 1U))
                {
                    /* We must find a coeff to toggle up or down so the sign bit of the first non-zero coeff
                     * is properly implied. Note dstCoeff[] are signed by this point but curChange and
                     * finalChange imply absolute levels (+1 is away from zero, -1 is towards zero) */

                    int64_t minCostInc = MAX_INT64, curCost = MAX_INT64;
                    int minPos = -1, finalChange = 0, curChange = 0;

                    /* in this section, RDOQ uses yet another novel approach at measuring cost. rdFactor is
                     * roughly 1/errScale of the earlier section.  Except it also divides by lambda2 to avoid
                     * having to multiply the signal bits portion. Here again the cost scale factor (1 << 15)
                     * is hard coded in three more places */

                    for (n = (lastCG ? lastNZPosInCG : SCAN_SET_SIZE - 1); n >= 0; --n)
                    {
                        uint32_t blkPos = codingParameters.scan[n + subPos];
                        if (dstCoeff[blkPos])
                        {
                            int64_t costUp = rdFactor * (-deltaU[blkPos]) + rateIncUp[blkPos];

                            /* if decrementing would make the coeff 0, we can remove the sigRateDelta */
                            bool isOne = abs(dstCoeff[blkPos]) == 1;
                            int64_t costDown = rdFactor * (deltaU[blkPos]) + rateIncDown[blkPos] -
                                (isOne ? ((1 << 15) + sigRateDelta[blkPos]) : 0);

                            if (lastCG && lastNZPosInCG == n && isOne)
                                costDown -= (4 << 15);

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
                        else
                        {
                            /* evaluate changing an uncoded coeff 0 to a coded coeff +/-1 */
                            curCost = rdFactor * (-(abs(deltaU[blkPos]))) + (1 << 15) + rateIncUp[blkPos] + sigRateDelta[blkPos];
                            curChange = 1;

                            if (n < firstNZPosInCG)
                            {
                                uint32_t thissignbit = (m_resiDctCoeff[blkPos] >= 0 ? 0 : 1);
                                if (thissignbit != signbit)
                                    curCost = MAX_INT64;
                            }
                        }

                        if (curCost < minCostInc)
                        {
                            minCostInc = curCost;
                            finalChange = curChange;
                            minPos = blkPos;
                        }
                    }

                    if (dstCoeff[minPos] == 32767 || dstCoeff[minPos] == -32768)
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

/** Pattern decision for context derivation process of significant_coeff_flag */
uint32_t Quant::calcPatternSigCtx(uint64_t sigCoeffGroupFlag64, uint32_t cgPosX, uint32_t cgPosY, uint32_t log2TrSizeCG)
{
    if (!log2TrSizeCG)
        return 0;

    const uint32_t trSizeCG = 1 << log2TrSizeCG;
    X265_CHECK(trSizeCG <= 8, "transform CG is too large\n");
    const uint32_t sigPos = (uint32_t)(sigCoeffGroupFlag64 >> (1 + (cgPosY << log2TrSizeCG) + cgPosX));
    const uint32_t sigRight = ((int32_t)(cgPosX - (trSizeCG - 1)) >> 31) & (sigPos & 1);
    const uint32_t sigLower = ((int32_t)(cgPosY - (trSizeCG - 1)) >> 31) & (sigPos >> (trSizeCG - 2)) & 2;

    return sigRight + sigLower;
}

/** Context derivation process of coeff_abs_significant_flag */
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

/** Calculates the cost of signaling the last significant coefficient in the block
 * \param posx X coordinate of the last significant coefficient
 * \param posy Y coordinate of the last significant coefficient
 * \returns cost of last significant coefficient
 */
inline uint32_t Quant::getRateLast(uint32_t posx, uint32_t posy) const
{
    uint32_t ctxX = getGroupIdx(posx);
    uint32_t ctxY = getGroupIdx(posy);
    uint32_t cost = m_estBitsSbac.lastXBits[ctxX] + m_estBitsSbac.lastYBits[ctxY];

    int32_t maskX = (int32_t)(2 - posx) >> 31;
    int32_t maskY = (int32_t)(2 - posy) >> 31;

    cost += maskX & (IEP_RATE * ((ctxX - 2) >> 1));
    cost += maskY & (IEP_RATE * ((ctxY - 2) >> 1));
    return cost;
}

/** Context derivation process of coeff_abs_significant_flag
 * \param sigCoeffGroupFlag significance map of L1
 * \param cgPosX column of current scan position
 * \param cgPosY row of current scan position
 * \param log2TrSizeCG log2 value of block size
 * \returns ctxInc for current scan position
 */
uint32_t Quant::getSigCoeffGroupCtxInc(uint64_t sigCoeffGroupFlag64,
                                       uint32_t cgPosX,
                                       uint32_t cgPosY,
                                       uint32_t log2TrSizeCG)
{
    const uint32_t trSizeCG = 1 << log2TrSizeCG;

    X265_CHECK(trSizeCG <= 8, "transform size too large\n");
    const uint32_t sigPos = (uint32_t)(sigCoeffGroupFlag64 >> (1 + (cgPosY << log2TrSizeCG) + cgPosX));
    const uint32_t sigRight = ((int32_t)(cgPosX - (trSizeCG - 1)) >> 31) & sigPos;
    const uint32_t sigLower = ((int32_t)(cgPosY - (trSizeCG - 1)) >> 31) & (sigPos >> (trSizeCG - 1));

    return (sigRight | sigLower) & 1;
}
