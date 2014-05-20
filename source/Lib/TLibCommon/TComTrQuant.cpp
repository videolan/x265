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

/** \file     TComTrQuant.cpp
    \brief    transform and quantization class
*/

#include "TComTrQuant.h"
#include "TComPic.h"
#include "ContextTables.h"
#include "primitives.h"

using namespace x265;

typedef struct
{
    int    nnzBeforePos0;
    double codedLevelAndDist; // distortion and level cost only
    double uncodedDist;  // all zero coded block distortion
    double sigCost;
    double sigCost0;
} coeffGroupRDStats;

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Constants
// ====================================================================================================================

#define RDOQ_CHROMA 1  ///< use of RDOQ in chroma

inline static int x265_min_fast(int x, int y)
{
    return y + ((x - y) & ((x - y) >> (sizeof(int) * CHAR_BIT - 1))); // min(x, y)
}

// ====================================================================================================================
// TComTrQuant class member functions
// ====================================================================================================================

TComTrQuant::TComTrQuant()
{
    m_qpParam.clear();

    // allocate temporary buffers
    // OPT_ME: I may reduce this to short and output matched, but I am not sure it is right.
    m_tmpCoeff = X265_MALLOC(int32_t, MAX_CU_SIZE * MAX_CU_SIZE);

    // allocate bit estimation class (for RDOQ)
    m_estBitsSbac = new estBitsSbacStruct;
    initScalingList();
}

TComTrQuant::~TComTrQuant()
{
    // delete temporary buffers
    if (m_tmpCoeff)
    {
        X265_FREE(m_tmpCoeff);
    }

    // delete bit estimation class
    delete m_estBitsSbac;
    destroyScalingList();
}

/** Set qP for Quantization.
 * \param qpy QPy
 * \param bLowpass
 * \param sliceType
 * \param ttype
 * \param qpBdOffset
 * \param chromaQPOffset
 *
 * return void
 */
void TComTrQuant::setQPforQuant(int qpy, TextType ttype, int qpBdOffset, int chromaQPOffset, int chFmt)
{
    int qpScaled;

    if (ttype == TEXT_LUMA)
    {
        qpScaled = qpy + qpBdOffset;
    }
    else
    {
        qpScaled = Clip3(-qpBdOffset, 57, qpy + chromaQPOffset);

        if (qpScaled < 0)
        {
            qpScaled = qpScaled + qpBdOffset;
        }
        else
        {
            qpScaled = g_chromaScale[chFmt][qpScaled] + qpBdOffset;
        }
    }
    m_qpParam.setQpParam(qpScaled);
}

// To minimize the distortion only. No rate is considered.
void TComTrQuant::signBitHidingHDQ(coeff_t* qCoef, coeff_t* coef, int32_t* deltaU, const TUEntropyCodingParameters &codingParameters)
{
    const uint32_t log2TrSizeCG = codingParameters.log2TrSizeCG;

    int lastCG = 1;

    for (int subSet = (1 << log2TrSizeCG * 2) - 1; subSet >= 0; subSet--)
    {
        int subPos = subSet << LOG2_SCAN_SET_SIZE;
        int n;

        for (n = SCAN_SET_SIZE - 1; n >= 0; --n)
        {
            if (qCoef[codingParameters.scan[n + subPos]])
            {
                break;
            }
        }

        if (n < 0) continue;

        int  lastNZPosInCG = n;

        for (n = 0;; n++)
        {
            if (qCoef[codingParameters.scan[n + subPos]])
            {
                break;
            }
        }

        int  firstNZPosInCG = n;

        if (lastNZPosInCG - firstNZPosInCG >= SBH_THRESHOLD)
        {
            uint32_t signbit = (qCoef[codingParameters.scan[subPos + firstNZPosInCG]] > 0 ? 0 : 1);
            int  absSum = 0;

            for (n = firstNZPosInCG; n <= lastNZPosInCG; n++)
            {
                absSum += qCoef[codingParameters.scan[n + subPos]];
            }

            if (signbit != (absSum & 0x1)) //compare signbit with sum_parity
            {
                int minCostInc = MAX_INT,  minPos = -1, finalChange = 0, curCost = MAX_INT, curChange = 0;

                for (n = (lastCG == 1 ? lastNZPosInCG : SCAN_SET_SIZE - 1); n >= 0; --n)
                {
                    uint32_t blkPos   = codingParameters.scan[n + subPos];
                    if (qCoef[blkPos] != 0)
                    {
                        if (deltaU[blkPos] > 0)
                        {
                            curCost = -deltaU[blkPos];
                            curChange = 1;
                        }
                        else
                        {
                            //curChange =-1;
                            if (n == firstNZPosInCG && abs(qCoef[blkPos]) == 1)
                            {
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
                        if (n < firstNZPosInCG)
                        {
                            uint32_t thisSignBit = (coef[blkPos] >= 0 ? 0 : 1);
                            if (thisSignBit != signbit)
                            {
                                curCost = MAX_INT;
                            }
                            else
                            {
                                curCost = -(deltaU[blkPos]);
                                curChange = 1;
                            }
                        }
                        else
                        {
                            curCost = -(deltaU[blkPos]);
                            curChange = 1;
                        }
                    }

                    if (curCost < minCostInc)
                    {
                        minCostInc = curCost;
                        finalChange = curChange;
                        minPos = blkPos;
                    }
                } //CG loop

                if (qCoef[minPos] == 32767 || qCoef[minPos] == -32768)
                {
                    finalChange = -1;
                }

                if (coef[minPos] >= 0)
                {
                    qCoef[minPos] += finalChange;
                }
                else
                {
                    qCoef[minPos] -= finalChange;
                }
            } // Hide
        }
        lastCG = 0;
    } // TU loop
}

uint32_t TComTrQuant::xQuant(TComDataCU* cu, int32_t* coef, coeff_t* qCoef, int trSize,
                             TextType ttype, uint32_t absPartIdx, int32_t *lastPos, bool curUseRDOQ)
{
    uint32_t acSum = 0;
    int add = 0;
    bool useRDOQ = (cu->getTransformSkip(absPartIdx, ttype) ? m_useRDOQTS : m_useRDOQ) && curUseRDOQ;

#if _MSC_VER
#pragma warning(disable: 4127) // conditional expression is constant
#endif
    if (useRDOQ && (ttype == TEXT_LUMA || RDOQ_CHROMA))
    {
        acSum = xRateDistOptQuant(cu, coef, qCoef, trSize, ttype, absPartIdx, lastPos);
    }
    else
    {
        const uint32_t log2TrSize = g_convertToBit[trSize] + 2;
        TUEntropyCodingParameters codingParameters;
        getTUEntropyCodingParameters(cu, codingParameters, absPartIdx, log2TrSize, ttype);
        int deltaU[32 * 32];

        int scalingListType = (cu->isIntra(absPartIdx) ? 0 : 3) + ttype;
        X265_CHECK(scalingListType < 6, "scaling list type out of range\n");
        int32_t *quantCoeff = 0;
        quantCoeff = getQuantCoeff(scalingListType, m_qpParam.m_rem, log2TrSize - 2);

        int transformShift = MAX_TR_DYNAMIC_RANGE - X265_DEPTH - log2TrSize; // Represents scaling through forward transform

        int qbits = QUANT_SHIFT + m_qpParam.m_per + transformShift;
        add = (cu->getSlice()->getSliceType() == I_SLICE ? 171 : 85) << (qbits - 9);

        int numCoeff = 1 << log2TrSize * 2;
        acSum += primitives.quant(coef, quantCoeff, deltaU, qCoef, qbits, add, numCoeff, lastPos);

        if (cu->getSlice()->getPPS()->getSignHideFlag() && acSum >= 2)
        {
            signBitHidingHDQ(qCoef, coef, deltaU, codingParameters);
        }
    }
    return acSum;
}

void TComTrQuant::init(uint32_t maxTrSize, int useRDOQ, int useRDOQTS, int useTransformSkipFast)
{
    m_maxTrSize            = maxTrSize;
    m_useRDOQ              = useRDOQ;
    m_useRDOQTS            = useRDOQTS;
    m_useTransformSkipFast = useTransformSkipFast;
}

uint32_t TComTrQuant::transformNxN(TComDataCU* cu,
                                   int16_t*    residual,
                                   uint32_t    stride,
                                   coeff_t*    coeff,
                                   uint32_t    trSize,
                                   TextType    ttype,
                                   uint32_t    absPartIdx,
                                   int32_t*    lastPos,
                                   bool        useTransformSkip,
                                   bool        curUseRDOQ)
{
    if (cu->getCUTransquantBypass(absPartIdx))
    {
        uint32_t absSum = 0;
        for (uint32_t k = 0; k < trSize; k++)
        {
            for (uint32_t j = 0; j < trSize; j++)
            {
                coeff[k * trSize + j] = ((int16_t)residual[k * stride + j]);
                absSum += abs(residual[k * stride + j]);
            }
        }

        return absSum;
    }

    uint32_t mode; //luma intra pred
    if (ttype == TEXT_LUMA && cu->getPredictionMode(absPartIdx) == MODE_INTRA)
    {
        mode = cu->getLumaIntraDir(absPartIdx);
    }
    else
    {
        mode = REG_DCT;
    }

    X265_CHECK((cu->getSlice()->getSPS()->getMaxTrSize() >= trSize), "transform size too large\n");
    if (useTransformSkip)
    {
        xTransformSkip(residual, stride, m_tmpCoeff, trSize);
    }
    else
    {
        // TODO: this may need larger data types for X265_DEPTH > 8
        const uint32_t log2BlockSize = g_convertToBit[trSize];
        primitives.dct[DCT_4x4 + log2BlockSize - ((trSize == 4) && (mode != REG_DCT))](residual, m_tmpCoeff, stride);
    }
    return xQuant(cu, m_tmpCoeff, coeff, trSize, ttype, absPartIdx, lastPos, curUseRDOQ);
}

void TComTrQuant::invtransformNxN(bool transQuantBypass, uint32_t mode, int16_t* residual, uint32_t stride, coeff_t* coeff, uint32_t trSize, int scalingListType, bool useTransformSkip, int lastPos)
{
    if (transQuantBypass)
    {
        for (uint32_t k = 0; k < trSize; k++)
        {
            for (uint32_t j = 0; j < trSize; j++)
            {
                residual[k * stride + j] = (int16_t)(coeff[k * trSize + j]);
            }
        }

        return;
    }

    // Values need to pass as input parameter in dequant
    int per = m_qpParam.m_per;
    int rem = m_qpParam.m_rem;
    bool useScalingList = getUseScalingList();
    const uint32_t log2TrSize = g_convertToBit[trSize] + 2;
    int transformShift = MAX_TR_DYNAMIC_RANGE - X265_DEPTH - log2TrSize;
    int shift = QUANT_IQUANT_SHIFT - QUANT_SHIFT - transformShift;
    int32_t *dequantCoef = getDequantCoeff(scalingListType, m_qpParam.m_rem, log2TrSize - 2);

    if (!useScalingList)
    {
        static const int invQuantScales[6] = { 40, 45, 51, 57, 64, 72 };
        int scale = invQuantScales[rem] << per;
        primitives.dequant_normal(coeff, m_tmpCoeff, trSize * trSize, scale, shift);
    }
    else
    {
        // CHECK_ME: the code is not verify since this is DEAD path
        primitives.dequant_scaling(coeff, dequantCoef, m_tmpCoeff, trSize * trSize, per, shift);
    }

    if (useTransformSkip == true)
    {
        xITransformSkip(m_tmpCoeff, residual, stride, trSize);
    }
    else
    {
        // CHECK_ME: we can't here when no any coeff
        X265_CHECK(lastPos >= 0, "lastPos negative\n");

        const uint32_t log2BlockSize = log2TrSize - 2;

        // DC only
        if (lastPos == 0 && !((trSize == 4) && (mode != REG_DCT)))
        {
            const int shift_1st = 7;
            const int add_1st = 1 << (shift_1st - 1);
            const int shift_2nd = 12 - (X265_DEPTH - 8);
            const int add_2nd = 1 << (shift_2nd - 1);

            int dc_val = (((m_tmpCoeff[0] * 64 + add_1st) >> shift_1st) * 64 + add_2nd) >> shift_2nd;
            primitives.blockfill_s[log2BlockSize](residual, stride, dc_val);

            return;
        }

        // TODO: this may need larger data types for X265_DEPTH > 8
        primitives.idct[IDCT_4x4 + log2BlockSize - ((trSize == 4) && (mode != REG_DCT))](m_tmpCoeff, residual, stride);
    }
}

// ------------------------------------------------------------------------------------------------
// Logical transform
// ------------------------------------------------------------------------------------------------

/** Wrapper function between HM interface and core 4x4 transform skipping
 *  \param resiBlock input data (residual)
 *  \param psCoeff output data (transform coefficients)
 *  \param stride stride of input residual data
 *  \param size transform size (size x size)
 */
void TComTrQuant::xTransformSkip(int16_t* resiBlock, uint32_t stride, int32_t* coeff, int trSize)
{
    uint32_t log2TrSize = g_convertToBit[trSize] + 2;
    int  shift = MAX_TR_DYNAMIC_RANGE - X265_DEPTH - log2TrSize;
    uint32_t transformSkipShift;
    int  j, k;

    if (shift >= 0)
    {
        primitives.cvt16to32_shl(coeff, resiBlock, stride, shift, trSize);
    }
    else
    {
        //The case when X265_DEPTH > 13
        int offset;
        transformSkipShift = -shift;
        offset = (1 << (transformSkipShift - 1));
        for (j = 0; j < trSize; j++)
        {
            for (k = 0; k < trSize; k++)
            {
                coeff[j * trSize + k] = (resiBlock[j * stride + k] + offset) >> transformSkipShift;
            }
        }
    }
}

/** Wrapper function between HM interface and core NxN transform skipping
 *  \param plCoef input data (coefficients)
 *  \param pResidual output data (residual)
 *  \param stride stride of input residual data
 *  \param size transform size (size x size)
 */
void TComTrQuant::xITransformSkip(int32_t* coef, int16_t* residual, uint32_t stride, int trSize)
{
    uint32_t log2TrSize = g_convertToBit[trSize] + 2;
    int  shift = MAX_TR_DYNAMIC_RANGE - X265_DEPTH - log2TrSize;
    int  j, k;

    if (shift > 0)
    {
        primitives.cvt32to16_shr(residual, coef, stride, shift, trSize);
    }
    else
    {
        //The case when X265_DEPTH >= 13
        uint32_t transformSkipShift = -shift;
        for (j = 0; j < trSize; j++)
        {
            for (k = 0; k < trSize; k++)
            {
                residual[j * stride + k] =  coef[j * trSize + k] << transformSkipShift;
            }
        }
    }
}

/** RDOQ with CABAC
 * \param cu pointer to coding unit structure
 * \param plSrcCoeff pointer to input buffer
 * \param piDstCoeff reference to pointer to output buffer
 * \param width block width
 * \param height block height
 * \param uiAbsSum reference to absolute sum of quantized transform coefficient
 * \param ttype plane type / luminance or chrominance
 * \param absPartIdx absolute partition index
 * \returns void
 * Rate distortion optimized quantization for entropy
 * coding engines using probability models like CABAC
 */
uint32_t TComTrQuant::xRateDistOptQuant(TComDataCU* cu, int32_t* srcCoeff, coeff_t* dstCoeff, uint32_t trSize,
                                        TextType ttype, uint32_t absPartIdx, int32_t *lastPos)
{
    const uint32_t log2TrSize = g_convertToBit[trSize] + 2;
    uint32_t absSum = 0;
    int transformShift = MAX_TR_DYNAMIC_RANGE - X265_DEPTH - log2TrSize; // Represents scaling through forward transform
    uint32_t goRiceParam = 0;
    double blockUncodedCost = 0;
    int scalingListType = (cu->isIntra(absPartIdx) ? 0 : 3) + ttype;

    X265_CHECK(scalingListType < 6, "scaling list type out of range\n");

    int qbits = QUANT_SHIFT + m_qpParam.m_per + transformShift; // Right shift of non-RDOQ quantizer;  level = (coeff*Q + offset)>>q_bits
    double *errScaleOrg = getErrScaleCoeff(scalingListType, log2TrSize - 2, m_qpParam.m_rem);
    int32_t *qCoefOrg = getQuantCoeff(scalingListType, m_qpParam.m_rem, log2TrSize - 2);
    int32_t *qCoef = qCoefOrg;
    double *errScale = errScaleOrg;

    double costCoeff[32 * 32];
    double costSig[32 * 32];
    double costCoeff0[32 * 32];

    int rateIncUp[32 * 32];
    int rateIncDown[32 * 32];
    int sigRateDelta[32 * 32];
    int deltaU[32 * 32];
    TUEntropyCodingParameters codingParameters;
    getTUEntropyCodingParameters(cu, codingParameters, absPartIdx, log2TrSize, ttype);

    const uint32_t cgSize = (1 << MLS_CG_SIZE); // 16
    double costCoeffGroupSig[MLS_GRP_NUM];
    uint64_t sigCoeffGroupFlag64 = 0;
    uint32_t ctxSet      = 0;
    int    c1            = 1;
    int    c2            = 0;
    double baseCost      = 0;
    int    lastScanPos   = -1;
    uint32_t c1Idx       = 0;
    uint32_t c2Idx       = 0;
    int cgLastScanPos    = -1;
    uint32_t cgNum = 1 << codingParameters.log2TrSizeCG * 2;

    int scanPos;
    coeffGroupRDStats rdStats;

    for (int cgScanPos = cgNum - 1; cgScanPos >= 0; cgScanPos--)
    {
        const uint32_t cgBlkPos = codingParameters.scanCG[cgScanPos];
        const uint32_t cgPosY   = cgBlkPos >> codingParameters.log2TrSizeCG;
        const uint32_t cgPosX   = cgBlkPos - (cgPosY << codingParameters.log2TrSizeCG);
        const uint64_t cgBlkPosMask = ((uint64_t)1 << cgBlkPos);
        memset(&rdStats, 0, sizeof(coeffGroupRDStats));
        X265_CHECK((trSize >> 2) == (1 << codingParameters.log2TrSizeCG), "transform size invalid\n");
        const int patternSigCtx = TComTrQuant::calcPatternSigCtx(sigCoeffGroupFlag64, cgPosX, cgPosY, codingParameters.log2TrSizeCG);
        for (int scanPosinCG = cgSize - 1; scanPosinCG >= 0; scanPosinCG--)
        {
            scanPos = cgScanPos * cgSize + scanPosinCG;
            //===== quantization =====
            uint32_t blkPos = codingParameters.scan[scanPos];
            // set coeff
            int Q = qCoef[blkPos];
            double scaleFactor = errScale[blkPos];
            int levelDouble    = srcCoeff[blkPos];
            levelDouble        = (int)std::min<int64_t>((int64_t)abs((int)levelDouble) * Q, MAX_INT - (1 << (qbits - 1)));

            uint32_t maxAbsLevel = (levelDouble + (1 << (qbits - 1))) >> qbits;

            costCoeff0[scanPos] = ((uint64_t)levelDouble * levelDouble) * scaleFactor;
            blockUncodedCost   += costCoeff0[scanPos];
            dstCoeff[blkPos]    = maxAbsLevel;

            if (maxAbsLevel > 0 && lastScanPos < 0)
            {
                lastScanPos   = scanPos;
                ctxSet        = (scanPos < SCAN_SET_SIZE || ttype != TEXT_LUMA) ? 0 : 2;
                cgLastScanPos = cgScanPos;
            }

            if (lastScanPos >= 0)
            {
                const uint32_t c1c2Idx = ((c1Idx - 8) >> (sizeof(int) * CHAR_BIT - 1)) + (((-(int)c2Idx) >> (sizeof(int) * CHAR_BIT - 1)) + 1) * 2;
                const uint32_t baseLevel = ((uint32_t)0xD9 >> (c1c2Idx * 2)) & 3;  // {1, 2, 1, 3}
                X265_CHECK(C2FLAG_NUMBER == 1, "scan validation 1\n");
                X265_CHECK(!!(c1Idx < C1FLAG_NUMBER) == ((c1Idx - 8) >> (sizeof(int) * CHAR_BIT - 1)), "scan validation 2\n");
                X265_CHECK(!!(c2Idx == 0) == ((-(int)c2Idx) >> (sizeof(int) * CHAR_BIT - 1)) + 1, "scan validation 3\n");
                X265_CHECK(baseLevel == ((c1Idx < C1FLAG_NUMBER) ? (2 + (c2Idx == 0)) : 1), "scan validation 4\n");

                rateIncUp[blkPos] = 0;
                rateIncDown[blkPos] = 0;
                deltaU[blkPos] = 0;
                sigRateDelta[blkPos] = 0;

                //===== coefficient level estimation =====
                uint32_t level;
                const uint32_t oneCtx = 4 * ctxSet + c1;
                const uint32_t absCtx = ctxSet + c2;
                const int *greaterOneBits = m_estBitsSbac->greaterOneBits[oneCtx];
                const int *levelAbsBits = m_estBitsSbac->levelAbsBits[absCtx];
                double curCostSig = 0;

                costCoeff[scanPos] = MAX_DOUBLE;
                if (scanPos == lastScanPos)
                {
                    level = xGetCodedLevel(costCoeff[scanPos], curCostSig, costSig[scanPos],
                                           levelDouble, maxAbsLevel, baseLevel, greaterOneBits, levelAbsBits, goRiceParam,
                                           c1c2Idx, qbits, scaleFactor, 1);
                }
                else
                {
                    // NOTE: ttype is different to ctype, but getSigCtxInc may safety use it
                    const uint32_t ctxSig = getSigCtxInc(patternSigCtx, log2TrSize, trSize, blkPos, ttype, codingParameters.firstSignificanceMapContext);
                    if (maxAbsLevel < 3)
                    {
                        costSig[scanPos] = xGetRateSigCoef(0, ctxSig);
                        costCoeff[scanPos] = costCoeff0[scanPos] + costSig[scanPos];
                    }
                    if (maxAbsLevel != 0)
                    {
                        curCostSig = xGetRateSigCoef(1, ctxSig);
                        level = xGetCodedLevel(costCoeff[scanPos], curCostSig, costSig[scanPos],
                                               levelDouble, maxAbsLevel, baseLevel, greaterOneBits, levelAbsBits, goRiceParam,
                                               c1c2Idx, qbits, scaleFactor, 0);
                    }
                    else
                    {
                        level = 0;
                    }
                    sigRateDelta[blkPos] = m_estBitsSbac->significantBits[ctxSig][1] - m_estBitsSbac->significantBits[ctxSig][0];
                }
                deltaU[blkPos] = (levelDouble - ((int)level << qbits)) >> (qbits - 8);
                if (level > 0)
                {
                    int rateNow = xGetICRate(level, level - baseLevel, greaterOneBits, levelAbsBits, goRiceParam, c1c2Idx);
                    rateIncUp[blkPos] = xGetICRate(level + 1, level + 1 - baseLevel, greaterOneBits, levelAbsBits, goRiceParam, c1c2Idx) - rateNow;
                    rateIncDown[blkPos] = xGetICRate(level - 1, level - 1 - baseLevel, greaterOneBits, levelAbsBits, goRiceParam, c1c2Idx) - rateNow;
                }
                else // level == 0
                {
                    rateIncUp[blkPos] = greaterOneBits[0];
                }
                dstCoeff[blkPos] = level;
                baseCost           += costCoeff[scanPos];

                if (level >= baseLevel)
                {
                    if (goRiceParam < 4 && level > (3 << goRiceParam))
                    {
                        goRiceParam++;
                    }
                }
                c1Idx -= (-(int32_t)level) >> 31;

                //===== update bin model =====
                if (level > 1)
                {
                    c1 = 0;
                    c2 += (uint32_t)(c2 - 2) >> 31;
                    c2Idx++;
                }
                else if ((c1 < 3) && (c1 > 0) && level)
                {
                    c1++;
                }

                //===== context set update =====
                if ((scanPos % SCAN_SET_SIZE == 0) && (scanPos > 0))
                {
                    c2 = 0;
                    goRiceParam = 0;

                    c1Idx = 0;
                    c2Idx = 0;
                    ctxSet = (scanPos == SCAN_SET_SIZE || ttype != TEXT_LUMA) ? 0 : 2;
                    X265_CHECK(c1 >= 0, "c1 is negative\n");
                    ctxSet -= ((int32_t)(c1 - 1) >> 31);
                    c1 = 1;
                }
            }
            else
            {
                costCoeff[scanPos] = 0;
                baseCost += costCoeff0[scanPos];
            }
            rdStats.sigCost += costSig[scanPos];
            if (scanPosinCG == 0)
            {
                rdStats.sigCost0 = costSig[scanPos];
            }
            if (dstCoeff[blkPos])
            {
                sigCoeffGroupFlag64 |= cgBlkPosMask;
                rdStats.codedLevelAndDist += costCoeff[scanPos] - costSig[scanPos];
                rdStats.uncodedDist += costCoeff0[scanPos];
                if (scanPosinCG != 0)
                {
                    rdStats.nnzBeforePos0++;
                }
            }
        } //end for (scanPosinCG)

        if (cgLastScanPos >= 0)
        {
            costCoeffGroupSig[cgScanPos] = 0;
            if (cgScanPos)
            {
                if ((sigCoeffGroupFlag64 & cgBlkPosMask) == 0)
                {
                    uint32_t ctxSig = getSigCoeffGroupCtxInc(sigCoeffGroupFlag64, cgPosX, cgPosY, codingParameters.log2TrSizeCG);
                    baseCost += xGetRateSigCoeffGroup(0, ctxSig) - rdStats.sigCost;
                    costCoeffGroupSig[cgScanPos] = xGetRateSigCoeffGroup(0, ctxSig);
                }
                else
                {
                    if (cgScanPos < cgLastScanPos) //skip the last coefficient group, which will be handled together with last position below.
                    {
                        if (!rdStats.nnzBeforePos0)
                        {
                            baseCost -= rdStats.sigCost0;
                            rdStats.sigCost -= rdStats.sigCost0;
                        }
                        // rd-cost if SigCoeffGroupFlag = 0, initialization
                        double costZeroCG = baseCost;

                        // add SigCoeffGroupFlag cost to total cost
                        uint32_t ctxSig = getSigCoeffGroupCtxInc(sigCoeffGroupFlag64, cgPosX, cgPosY, codingParameters.log2TrSizeCG);
                        if (cgScanPos < cgLastScanPos)
                        {
                            baseCost  += xGetRateSigCoeffGroup(1, ctxSig);
                            costZeroCG += xGetRateSigCoeffGroup(0, ctxSig);
                            costCoeffGroupSig[cgScanPos] = xGetRateSigCoeffGroup(1, ctxSig);
                        }

                        // try to convert the current coeff group from non-zero to all-zero
                        costZeroCG += rdStats.uncodedDist; // distortion for resetting non-zero levels to zero levels
                        costZeroCG -= rdStats.codedLevelAndDist; // distortion and level cost for keeping all non-zero levels
                        costZeroCG -= rdStats.sigCost; // sig cost for all coeffs, including zero levels and non-zerl levels

                        // if we can save cost, change this block to all-zero block
                        if (costZeroCG < baseCost)
                        {
                            sigCoeffGroupFlag64 &= ~cgBlkPosMask;
                            baseCost = costZeroCG;
                            if (cgScanPos < cgLastScanPos)
                            {
                                costCoeffGroupSig[cgScanPos] = xGetRateSigCoeffGroup(0, ctxSig);
                            }
                            // reset coeffs to 0 in this block
                            for (int scanPosinCG = cgSize - 1; scanPosinCG >= 0; scanPosinCG--)
                            {
                                scanPos      = cgScanPos * cgSize + scanPosinCG;
                                uint32_t blkPos = codingParameters.scan[scanPos];
                                if (dstCoeff[blkPos])
                                {
                                    costCoeff[scanPos] = costCoeff0[scanPos];
                                    costSig[scanPos] = 0;
                                }
                                dstCoeff[blkPos] = 0;
                            }
                        } // end if ( d64CostAllZeros < baseCost )
                    }
                } // end if if (sigCoeffGroupFlag[ cgBlkPos ] == 0)
            }
            else
            {
                sigCoeffGroupFlag64 |= cgBlkPosMask;
            }
        }
    } //end for (cgScanPos)

    //===== estimate last position =====
    if (lastScanPos < 0)
    {
        return absSum;
    }

    double bestCost = 0;
    int    ctxCbf = 0;
    int    bestLastIdxp1 = 0;
    if (!cu->isIntra(absPartIdx) && ttype == TEXT_LUMA && cu->getTransformIdx(absPartIdx) == 0)
    {
        ctxCbf    = 0;
        bestCost  = blockUncodedCost + xGetICost(m_estBitsSbac->blockRootCbpBits[ctxCbf][0]);
        baseCost += xGetICost(m_estBitsSbac->blockRootCbpBits[ctxCbf][1]);
    }
    else
    {
        ctxCbf    = cu->getCtxQtCbf(ttype, cu->getTransformIdx(absPartIdx));
        bestCost  = blockUncodedCost + xGetICost(m_estBitsSbac->blockCbpBits[ctxCbf][0]);
        baseCost += xGetICost(m_estBitsSbac->blockCbpBits[ctxCbf][1]);
    }

    bool foundLast = false;
    for (int cgScanPos = cgLastScanPos; cgScanPos >= 0; cgScanPos--)
    {
        uint32_t cgBlkPos = codingParameters.scanCG[cgScanPos];
        baseCost -= costCoeffGroupSig[cgScanPos];
        if (sigCoeffGroupFlag64 & ((uint64_t)1 << cgBlkPos))
        {
            for (int scanPosinCG = cgSize - 1; scanPosinCG >= 0; scanPosinCG--)
            {
                scanPos = cgScanPos * cgSize + scanPosinCG;
                if (scanPos > lastScanPos) continue;
                uint32_t blkPos = codingParameters.scan[scanPos];
                if (dstCoeff[blkPos])
                {
                    uint32_t posY = blkPos >> log2TrSize;
                    uint32_t posX = blkPos - (posY << log2TrSize);
                    double costLast = codingParameters.scanType == SCAN_VER ? xGetRateLast(posY, posX) : xGetRateLast(posX, posY);
                    double totalCost = baseCost + costLast - costSig[scanPos];

                    if (totalCost < bestCost)
                    {
                        bestLastIdxp1  = scanPos + 1;
                        bestCost     = totalCost;
                    }
                    if (dstCoeff[blkPos] > 1)
                    {
                        foundLast = true;
                        break;
                    }
                    baseCost -= costCoeff[scanPos];
                    baseCost += costCoeff0[scanPos];
                }
                else
                {
                    baseCost -= costSig[scanPos];
                }
            } //end for

            if (foundLast)
            {
                break;
            }
        } // end if (sigCoeffGroupFlag[ cgBlkPos ])
    } // end for

    for (int pos = 0; pos < bestLastIdxp1; pos++)
    {
        int blkPos = codingParameters.scan[pos];
        int level  = dstCoeff[blkPos];
        absSum += level;
        if (level)
            *lastPos = blkPos;
        uint32_t mask = (int32_t)srcCoeff[blkPos] >> 31;
        dstCoeff[blkPos] = (level ^ mask) - mask;
    }

    //===== clean uncoded coefficients =====
    for (int pos = bestLastIdxp1; pos <= lastScanPos; pos++)
    {
        dstCoeff[codingParameters.scan[pos]] = 0;
    }

    if (cu->getSlice()->getPPS()->getSignHideFlag() && absSum >= 2)
    {
        int64_t rdFactor = (int64_t)(
                g_invQuantScales[m_qpParam.rem()] * g_invQuantScales[m_qpParam.rem()] * (1 << (2 * m_qpParam.m_per))
                / m_lambda / 16 / (1 << DISTORTION_PRECISION_ADJUSTMENT(2 * (X265_DEPTH - 8)))
                + 0.5);
        int lastCG = 1;

        for (int subSet = cgLastScanPos; subSet >= 0; subSet--)
        {
            int subPos = subSet << LOG2_SCAN_SET_SIZE;
            int n;

            for (n = SCAN_SET_SIZE - 1; n >= 0; --n)
            {
                if (dstCoeff[codingParameters.scan[n + subPos]])
                {
                    break;
                }
            }

            if (n < 0) continue;

            int lastNZPosInCG = n;

            for (n = 0;; n++)
            {
                if (dstCoeff[codingParameters.scan[n + subPos]])
                {
                    break;
                }
            }

            int firstNZPosInCG = n;

            if (lastNZPosInCG - firstNZPosInCG >= SBH_THRESHOLD)
            {
                uint32_t signbit = (dstCoeff[codingParameters.scan[subPos + firstNZPosInCG]] > 0 ? 0 : 1);
                int tmpSum = 0;

                for (n = firstNZPosInCG; n <= lastNZPosInCG; n++)
                {
                    tmpSum += dstCoeff[codingParameters.scan[n + subPos]];
                }

                if (signbit != (tmpSum & 0x1)) // hide but need tune
                {
                    // calculate the cost
                    int64_t minCostInc = MAX_INT64, curCost = MAX_INT64;
                    int minPos = -1, finalChange = 0, curChange = 0;

                    for (n = (lastCG == 1 ? lastNZPosInCG : SCAN_SET_SIZE - 1); n >= 0; --n)
                    {
                        uint32_t blkPos   = codingParameters.scan[n + subPos];
                        if (dstCoeff[blkPos] != 0)
                        {
                            int64_t costUp   = rdFactor * (-deltaU[blkPos]) + rateIncUp[blkPos];
                            int64_t costDown = rdFactor * (deltaU[blkPos]) + rateIncDown[blkPos] -
                                (abs(dstCoeff[blkPos]) == 1 ? ((1 << 15) + sigRateDelta[blkPos]) : 0);

                            if (lastCG == 1 && lastNZPosInCG == n && abs(dstCoeff[blkPos]) == 1)
                            {
                                costDown -= (4 << 15);
                            }

                            if (costUp < costDown)
                            {
                                curCost = costUp;
                                curChange =  1;
                            }
                            else
                            {
                                curChange = -1;
                                if (n == firstNZPosInCG && abs(dstCoeff[blkPos]) == 1)
                                {
                                    curCost = MAX_INT64;
                                }
                                else
                                {
                                    curCost = costDown;
                                }
                            }
                        }
                        else
                        {
                            curCost = rdFactor * (-(abs(deltaU[blkPos]))) + (1 << 15) + rateIncUp[blkPos] + sigRateDelta[blkPos];
                            curChange = 1;

                            if (n < firstNZPosInCG)
                            {
                                uint32_t thissignbit = (srcCoeff[blkPos] >= 0 ? 0 : 1);
                                if (thissignbit != signbit)
                                {
                                    curCost = MAX_INT64;
                                }
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
                    {
                        finalChange = -1;
                    }

                    if (srcCoeff[minPos] >= 0)
                    {
                        dstCoeff[minPos] += finalChange;
                    }
                    else
                    {
                        dstCoeff[minPos] -= finalChange;
                    }
                }
            }
            lastCG = 0;
        }
    }

    return absSum;
}

/** Pattern decision for context derivation process of significant_coeff_flag
 * \param sigCoeffGroupFlag pointer to prior coded significant coeff group
 * \param cgPosX column of current coefficient group
 * \param cgPosY row of current coefficient group
 * \param width width of the block
 * \param height height of the block
 * \returns pattern for current coefficient group
 */
uint32_t TComTrQuant::calcPatternSigCtx(const uint64_t sigCoeffGroupFlag64, const uint32_t cgPosX, const uint32_t cgPosY, const uint32_t log2TrSizeCG)
{
    if (log2TrSizeCG == 0) return 0;

    const uint32_t trSizeCG = 1 << log2TrSizeCG;
    X265_CHECK(trSizeCG <= 32, "transform CG is too large\n");
    const uint32_t sigPos = sigCoeffGroupFlag64 >> (1 + (cgPosY << log2TrSizeCG) + cgPosX);
    const uint32_t sigRight = ((int32_t)(cgPosX - (trSizeCG - 1)) >> 31) & (sigPos & 1);
    const uint32_t sigLower = ((int32_t)(cgPosY - (trSizeCG - 1)) >> 31) & (sigPos >> (trSizeCG - 2)) & 2;

    return sigRight + sigLower;
}

/** Context derivation process of coeff_abs_significant_flag
 * \param patternSigCtx pattern for current coefficient group
 * \param posX column of current scan position
 * \param posY row of current scan position
 * \param log2BlockSize log2 value of block size (square block)
 * \param width width of the block
 * \param height height of the block
 * \param textureType texture type (TEXT_LUMA...)
 * \returns ctxInc for current scan position
 */
uint32_t TComTrQuant::getSigCtxInc(const uint32_t patternSigCtx,
                                   const uint32_t log2TrSize,
                                   const uint32_t trSize,
                                   const uint32_t blkPos,
                                   const TextType ctype,
                                   const uint32_t firstSignificanceMapContext)
{
    static const uint8_t ctxIndMap[16] =
    {
        0, 1, 4, 5,
        2, 3, 4, 5,
        6, 6, 8, 8,
        7, 7, 8, 8
    };

    if (blkPos == 0) return 0; //special case for the DC context variable

    if (log2TrSize == 2) //4x4
    {
        return ctxIndMap[blkPos];
    }

    const uint32_t posY           = blkPos >> log2TrSize;
    const uint32_t posX           = blkPos & (trSize - 1);
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

    return (ctype == TEXT_LUMA && (posX | posY) >= 4) ? 3 + offset : offset;
}

/** Get the best level in RD sense
 * \param codedCost reference to coded cost
 * \param codedCost0 reference to cost when coefficient is 0
 * \param codedCostSig reference to cost of significant coefficient
 * \param levelDouble reference to unscaled quantized level
 * \param maxAbsLevel scaled quantized level
 * \param ctxNumSig current ctxInc for coeff_abs_significant_flag
 * \param ctxNumOne current ctxInc for coeff_abs_level_greater1 (1st bin of coeff_abs_level_minus1 in AVC)
 * \param ctxNumAbs current ctxInc for coeff_abs_level_greater2 (remaining bins of coeff_abs_level_minus1 in AVC)
 * \param absGoRice current Rice parameter for coeff_abs_level_minus3
 * \param qbits quantization step size
 * \param scaleFactor correction factor
 * \param bLast indicates if the coefficient is the last significant
 * \returns best quantized transform level for given scan position
 * This method calculates the best quantized transform level for a given scan position.
 */
inline uint32_t TComTrQuant::xGetCodedLevel(double&      codedCost,
                                            const double curCostSig,
                                            double&      codedCostSig,
                                            int          levelDouble,
                                            uint32_t     maxAbsLevel,
                                            uint32_t     baseLevel,
                                            const int *  greaterOneBits,
                                            const int *  levelAbsBits,
                                            uint32_t     absGoRice,
                                            uint32_t     c1c2Idx,
                                            int          qbits,
                                            double       scaleFactor,
                                            bool         last) const
{
    uint32_t   bestAbsLevel = 0;

    if (!last && maxAbsLevel == 0)
    {
        X265_CHECK(0, "get coded level failure\n");
    }

    int32_t minAbsLevel = maxAbsLevel - 1;
    if (minAbsLevel < 1)
        minAbsLevel = 1;

    // NOTE: (A + B) ^ 2 = (A ^ 2) + 2 * A * B + (B ^ 2)
    X265_CHECK(abs((double)levelDouble - (maxAbsLevel << qbits)) < INT_MAX, "levelDouble range check failure\n");
    const int32_t err1 = levelDouble - (maxAbsLevel << qbits);            // A
    double err2 = (double)((int64_t)err1 * err1);                         // A^ 2
    const int64_t err3 = (int64_t)2 * err1 * ((int64_t)1 << qbits);       // 2 * A * B
    const int64_t err4 = ((int64_t)1 << qbits) * ((int64_t)1 << qbits);   // B ^ 2
    const double errInc = (err3 + err4) * scaleFactor;

    err2 *= scaleFactor;

    double bestCodedCost = codedCost;
    double bestCodedCostSig = codedCostSig;
    int diffLevel = maxAbsLevel - baseLevel;
    for (int absLevel = maxAbsLevel; absLevel >= minAbsLevel; absLevel--)
    {
        X265_CHECK(fabs((double)err2 - double(levelDouble  - (absLevel << qbits)) * double(levelDouble  - (absLevel << qbits)) * scaleFactor) < 1e-5, "err2 check failure\n");
        double curCost = err2 + xGetICRateCost(absLevel, diffLevel, greaterOneBits, levelAbsBits, absGoRice, c1c2Idx);
        curCost       += curCostSig;

        if (curCost < bestCodedCost)
        {
            bestAbsLevel = absLevel;
            bestCodedCost = curCost;
            bestCodedCostSig = curCostSig;
        }
        err2 += errInc;
        diffLevel--;
    }

    codedCost = bestCodedCost;
    codedCostSig = bestCodedCostSig;
    return bestAbsLevel;
}

/** Calculates the cost for specific absolute transform level
 * \param absLevel scaled quantized level
 * \param ctxNumOne current ctxInc for coeff_abs_level_greater1 (1st bin of coeff_abs_level_minus1 in AVC)
 * \param ctxNumAbs current ctxInc for coeff_abs_level_greater2 (remaining bins of coeff_abs_level_minus1 in AVC)
 * \param absGoRice Rice parameter for coeff_abs_level_minus3
 * \returns cost of given absolute transform level
 */
inline double TComTrQuant::xGetICRateCost(uint32_t   absLevel,
                                          int32_t    diffLevel,
                                          const int *greaterOneBits,
                                          const int *levelAbsBits,
                                          uint32_t   absGoRice,
                                          uint32_t   c1c2Idx) const
{
    X265_CHECK(absLevel, "absLevel should not be zero\n");
    uint32_t rate = xGetIEPRate();

    if (diffLevel < 0)
    {
        X265_CHECK((absLevel == 1) || (absLevel == 2), "absLevel range check failure\n");
        rate += greaterOneBits[(absLevel == 2)];

        if (absLevel == 2)
        {
            rate += levelAbsBits[0];
        }
    }
    else
    {
        uint32_t symbol = diffLevel;
        uint32_t length;
        if ((symbol >> absGoRice) < COEF_REMAIN_BIN_REDUCTION)
        {
            length = symbol >> absGoRice;
            rate += (length + 1 + absGoRice) << 15;
        }
        else
        {
            length = 0;
            symbol = (symbol >> absGoRice) - COEF_REMAIN_BIN_REDUCTION;
            if (symbol != 0)
            {
                unsigned long idx;
                CLZ32(idx, symbol + 1);
                length = idx;
            }

            rate += (COEF_REMAIN_BIN_REDUCTION + length + absGoRice + 1 + length) << 15;
        }
        if (c1c2Idx & 1)
        {
            rate += greaterOneBits[1];
        }

        if (c1c2Idx == 3)
        {
            rate += levelAbsBits[1];
        }
    }

    return xGetICost(rate);
}

inline int TComTrQuant::xGetICRate(uint32_t   absLevel,
                                   int32_t    diffLevel,
                                   const int *greaterOneBits,
                                   const int *levelAbsBits,
                                   uint32_t   absGoRice,
                                   uint32_t   c1c2Idx) const
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
        {
            rate += levelAbsBits[0];
        }
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
            X265_CHECK(x265_min_fast(symbol, (maxVlc + 1)) == maxVlc + 1, "min check failure\n");
            symbol = maxVlc + 1;
        }

        uint32_t prefLen = (symbol >> absGoRice) + 1;
        uint32_t numBins = x265_min_fast(prefLen + absGoRice, 8 /* g_goRicePrefixLen[absGoRice] + absGoRice */);

        rate += numBins << 15;

        if (c1c2Idx & 1)
        {
            rate += greaterOneBits[1];
        }

        if (c1c2Idx == 3)
        {
            rate += levelAbsBits[1];
        }
    }
    return rate;
}

/** Calculates the cost of signaling the last significant coefficient in the block
 * \param posx X coordinate of the last significant coefficient
 * \param posy Y coordinate of the last significant coefficient
 * \returns cost of last significant coefficient
 */
inline double TComTrQuant::xGetRateLast(uint32_t posx, uint32_t posy) const
{
    uint32_t ctxX = getGroupIdx(posx);
    uint32_t ctxY = getGroupIdx(posy);
    uint32_t cost = m_estBitsSbac->lastXBits[ctxX] + m_estBitsSbac->lastYBits[ctxY];

    int32_t maskX = (int32_t)(2 - posx) >> 31;
    int32_t maskY = (int32_t)(2 - posy) >> 31;

    cost += maskX & (xGetIEPRate() * ((ctxX - 2) >> 1));
    cost += maskY & (xGetIEPRate() * ((ctxY - 2) >> 1));
    return xGetICost(cost);
}

/** Context derivation process of coeff_abs_significant_flag
 * \param sigCoeffGroupFlag significance map of L1
 * \param uiBlkX column of current scan position
 * \param uiBlkY row of current scan position
 * \param uiLog2BlkSize log2 value of block size
 * \returns ctxInc for current scan position
 */
uint32_t TComTrQuant::getSigCoeffGroupCtxInc(const uint64_t sigCoeffGroupFlag64,
                                             const uint32_t cgPosX,
                                             const uint32_t cgPosY,
                                             const uint32_t log2TrSizeCG)
{
    const uint32_t trSizeCG = 1 << log2TrSizeCG;

    X265_CHECK(trSizeCG <= 32, "transform size too large\n");
    const uint32_t sigPos = sigCoeffGroupFlag64 >> (1 + (cgPosY << log2TrSizeCG) + cgPosX);
    const uint32_t sigRight = ((int32_t)(cgPosX - (trSizeCG - 1)) >> 31) & sigPos;
    const uint32_t sigLower = ((int32_t)(cgPosY - (trSizeCG - 1)) >> 31) & (sigPos >> (trSizeCG - 1));

    return (sigRight | sigLower) & 1;
}

/** set quantized matrix coefficient for encode
 * \param scalingList quantized matrix address
 */
void TComTrQuant::setScalingList(TComScalingList *scalingList)
{
    uint32_t size, list;
    uint32_t qp;

    for (size = 0; size < SCALING_LIST_SIZE_NUM; size++)
    {
        for (list = 0; list < g_scalingListNum[size]; list++)
        {
            for (qp = 0; qp < SCALING_LIST_REM_NUM; qp++)
            {
                xSetScalingListEnc(scalingList, list, size, qp);
                xSetScalingListDec(scalingList, list, size, qp);
                setErrScaleCoeff(list, size, qp);
            }
        }
    }
}

/** set error scale coefficients
 * \param list List ID
 * \param uiSize Size
 * \param uiQP Quantization parameter
 */
void TComTrQuant::setErrScaleCoeff(uint32_t list, uint32_t size, uint32_t qp)
{
    uint32_t log2TrSize = g_convertToBit[g_scalingListSizeX[size]] + 2;
    int transformShift = MAX_TR_DYNAMIC_RANGE - X265_DEPTH - log2TrSize; // Represents scaling through forward transform

    uint32_t i, maxNumCoeff = g_scalingListSize[size];
    int32_t *quantCoeff;
    double *errScale;

    quantCoeff   = getQuantCoeff(list, qp, size);
    errScale     = getErrScaleCoeff(list, size, qp);

    double scalingBits = (double)(1 << SCALE_BITS);               // Compensate for scaling of bitcount in Lagrange cost function
    scalingBits = scalingBits * pow(2.0, -2.0 * transformShift);  // Compensate for scaling through forward transform
    for (i = 0; i < maxNumCoeff; i++)
    {
        errScale[i] = scalingBits / quantCoeff[i] / quantCoeff[i] / (1 << DISTORTION_PRECISION_ADJUSTMENT(2 * (X265_DEPTH - 8)));
    }
}

/** set quantized matrix coefficient for encode
 * \param scalingList quantized matrix address
 * \param listId List index
 * \param sizeId size index
 * \param uiQP Quantization parameter
 */
void TComTrQuant::xSetScalingListEnc(TComScalingList *scalingList, uint32_t listId, uint32_t sizeId, uint32_t qp)
{
    uint32_t width = g_scalingListSizeX[sizeId];
    uint32_t height = g_scalingListSizeX[sizeId];
    uint32_t ratio = g_scalingListSizeX[sizeId] / X265_MIN(MAX_MATRIX_SIZE_NUM, (int)g_scalingListSizeX[sizeId]);
    int32_t *quantcoeff;
    int32_t *coeff = scalingList->getScalingListAddress(sizeId, listId);

    quantcoeff   = getQuantCoeff(listId, qp, sizeId);
    processScalingListEnc(coeff, quantcoeff, g_quantScales[qp] << 4, height, width, ratio, X265_MIN(MAX_MATRIX_SIZE_NUM, (int)g_scalingListSizeX[sizeId]), scalingList->getScalingListDC(sizeId, listId));
}

/** set quantized matrix coefficient for decode
 * \param scalingList quantized matrix address
 * \param list List index
 * \param size size index
 * \param uiQP Quantization parameter
 */
void TComTrQuant::xSetScalingListDec(TComScalingList *scalingList, uint32_t listId, uint32_t sizeId, uint32_t qp)
{
    uint32_t width = g_scalingListSizeX[sizeId];
    uint32_t height = g_scalingListSizeX[sizeId];
    uint32_t ratio = g_scalingListSizeX[sizeId] / X265_MIN(MAX_MATRIX_SIZE_NUM, (int)g_scalingListSizeX[sizeId]);
    int32_t *dequantcoeff;
    int32_t *coeff = scalingList->getScalingListAddress(sizeId, listId);

    dequantcoeff = getDequantCoeff(listId, qp, sizeId);
    processScalingListDec(coeff, dequantcoeff, g_invQuantScales[qp], height, width, ratio, X265_MIN(MAX_MATRIX_SIZE_NUM, (int)g_scalingListSizeX[sizeId]), scalingList->getScalingListDC(sizeId, listId));
}

/** set flat matrix value to quantized coefficient
 */
void TComTrQuant::setFlatScalingList()
{
    uint32_t size, list;
    uint32_t qp;

    for (size = 0; size < SCALING_LIST_SIZE_NUM; size++)
    {
        for (list = 0; list < g_scalingListNum[size]; list++)
        {
            for (qp = 0; qp < SCALING_LIST_REM_NUM; qp++)
            {
                xsetFlatScalingList(list, size, qp);
                setErrScaleCoeff(list, size, qp);
            }
        }
    }
}

/** set flat matrix value to quantized coefficient
 * \param list List ID
 * \param uiQP Quantization parameter
 * \param uiSize Size
 */
void TComTrQuant::xsetFlatScalingList(uint32_t list, uint32_t size, uint32_t qp)
{
    uint32_t i, num = g_scalingListSize[size];
    int32_t *quantcoeff;
    int32_t *dequantcoeff;
    int quantScales = g_quantScales[qp];
    int invQuantScales = g_invQuantScales[qp] << 4;

    quantcoeff   = getQuantCoeff(list, qp, size);
    dequantcoeff = getDequantCoeff(list, qp, size);

    for (i = 0; i < num; i++)
    {
        *quantcoeff++ = quantScales;
        *dequantcoeff++ = invQuantScales;
    }
}

/** set quantized matrix coefficient for encode
 * \param coeff quantized matrix address
 * \param quantcoeff quantized matrix address
 * \param quantScales Q(QP%6)
 * \param height height
 * \param width width
 * \param ratio ratio for upscale
 * \param sizuNum matrix size
 * \param dc dc parameter
 */
void TComTrQuant::processScalingListEnc(int32_t *coeff, int32_t *quantcoeff, int quantScales, uint32_t height, uint32_t width, uint32_t ratio, int sizuNum, uint32_t dc)
{
    int nsqth = (height < width) ? 4 : 1; // height ratio for NSQT
    int nsqtw = (width < height) ? 4 : 1; // width ratio for NSQT

    for (uint32_t j = 0; j < height; j++)
    {
        for (uint32_t i = 0; i < width; i++)
        {
            quantcoeff[j * width + i] = quantScales / coeff[sizuNum * (j * nsqth / ratio) + i * nsqtw / ratio];
        }
    }

    if (ratio > 1)
    {
        quantcoeff[0] = quantScales / dc;
    }
}

/** set quantized matrix coefficient for decode
 * \param coeff quantized matrix address
 * \param dequantcoeff quantized matrix address
 * \param invQuantScales IQ(QP%6))
 * \param height height
 * \param width width
 * \param ratio ratio for upscale
 * \param sizuNum matrix size
 * \param dc dc parameter
 */
void TComTrQuant::processScalingListDec(int32_t *coeff, int32_t *dequantcoeff, int invQuantScales, uint32_t height, uint32_t width, uint32_t ratio, int sizuNum, uint32_t dc)
{
    for (uint32_t j = 0; j < height; j++)
    {
        for (uint32_t i = 0; i < width; i++)
        {
            dequantcoeff[j * width + i] = invQuantScales * coeff[sizuNum * (j / ratio) + i / ratio];
        }
    }

    if (ratio > 1)
    {
        dequantcoeff[0] = invQuantScales * dc;
    }
}

/** initialization process of scaling list array
 */
void TComTrQuant::initScalingList()
{
    for (uint32_t sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
    {
        for (uint32_t listId = 0; listId < g_scalingListNum[sizeId]; listId++)
        {
            for (uint32_t qp = 0; qp < SCALING_LIST_REM_NUM; qp++)
            {
                m_quantCoef[sizeId][listId][qp] = new int[g_scalingListSize[sizeId]];
                m_dequantCoef[sizeId][listId][qp] = new int[g_scalingListSize[sizeId]];
                m_errScale[sizeId][listId][qp] = new double[g_scalingListSize[sizeId]];
            }
        }
    }
}

/** destroy quantization matrix array
 */
void TComTrQuant::destroyScalingList()
{
    for (uint32_t sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
    {
        for (uint32_t listId = 0; listId < g_scalingListNum[sizeId]; listId++)
        {
            for (uint32_t qp = 0; qp < SCALING_LIST_REM_NUM; qp++)
            {
                delete [] m_quantCoef[sizeId][listId][qp];
                delete [] m_dequantCoef[sizeId][listId][qp];
                delete [] m_errScale[sizeId][listId][qp];
            }
        }
    }
}

//! \}
