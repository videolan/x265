/*****************************************************************************
* Copyright (C) 2013 x265 project
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
#include "entropy.h"
#include "TLibCommon/TComSampleAdaptiveOffset.h"
#include "TLibCommon/TComTrQuant.h"

namespace x265 {

const uint32_t g_entropyBits[128] =
{
    // Corrected table, most notably for last state
    0x07b23, 0x085f9, 0x074a0, 0x08cbc, 0x06ee4, 0x09354, 0x067f4, 0x09c1b, 0x060b0, 0x0a62a, 0x05a9c, 0x0af5b, 0x0548d, 0x0b955, 0x04f56, 0x0c2a9,
    0x04a87, 0x0cbf7, 0x045d6, 0x0d5c3, 0x04144, 0x0e01b, 0x03d88, 0x0e937, 0x039e0, 0x0f2cd, 0x03663, 0x0fc9e, 0x03347, 0x10600, 0x03050, 0x10f95,
    0x02d4d, 0x11a02, 0x02ad3, 0x12333, 0x0286e, 0x12cad, 0x02604, 0x136df, 0x02425, 0x13f48, 0x021f4, 0x149c4, 0x0203e, 0x1527b, 0x01e4d, 0x15d00,
    0x01c99, 0x166de, 0x01b18, 0x17017, 0x019a5, 0x17988, 0x01841, 0x18327, 0x016df, 0x18d50, 0x015d9, 0x19547, 0x0147c, 0x1a083, 0x0138e, 0x1a8a3,
    0x01251, 0x1b418, 0x01166, 0x1bd27, 0x01068, 0x1c77b, 0x00f7f, 0x1d18e, 0x00eda, 0x1d91a, 0x00e19, 0x1e254, 0x00d4f, 0x1ec9a, 0x00c90, 0x1f6e0,
    0x00c01, 0x1fef8, 0x00b5f, 0x208b1, 0x00ab6, 0x21362, 0x00a15, 0x21e46, 0x00988, 0x2285d, 0x00934, 0x22ea8, 0x008a8, 0x239b2, 0x0081d, 0x24577,
    0x007c9, 0x24ce6, 0x00763, 0x25663, 0x00710, 0x25e8f, 0x006a0, 0x26a26, 0x00672, 0x26f23, 0x005e8, 0x27ef8, 0x005ba, 0x284b5, 0x0055e, 0x29057,
    0x0050c, 0x29bab, 0x004c1, 0x2a674, 0x004a7, 0x2aa5e, 0x0046f, 0x2b32f, 0x0041f, 0x2c0ad, 0x003e7, 0x2ca8d, 0x003ba, 0x2d323, 0x0010c, 0x3bfbb
};

const uint8_t g_nextState[128][2] =
{
    { 2, 1 }, { 0, 3 }, { 4, 0 }, { 1, 5 }, { 6, 2 }, { 3, 7 }, { 8, 4 }, { 5, 9 },
    { 10, 4 }, { 5, 11 }, { 12, 8 }, { 9, 13 }, { 14, 8 }, { 9, 15 }, { 16, 10 }, { 11, 17 },
    { 18, 12 }, { 13, 19 }, { 20, 14 }, { 15, 21 }, { 22, 16 }, { 17, 23 }, { 24, 18 }, { 19, 25 },
    { 26, 18 }, { 19, 27 }, { 28, 22 }, { 23, 29 }, { 30, 22 }, { 23, 31 }, { 32, 24 }, { 25, 33 },
    { 34, 26 }, { 27, 35 }, { 36, 26 }, { 27, 37 }, { 38, 30 }, { 31, 39 }, { 40, 30 }, { 31, 41 },
    { 42, 32 }, { 33, 43 }, { 44, 32 }, { 33, 45 }, { 46, 36 }, { 37, 47 }, { 48, 36 }, { 37, 49 },
    { 50, 38 }, { 39, 51 }, { 52, 38 }, { 39, 53 }, { 54, 42 }, { 43, 55 }, { 56, 42 }, { 43, 57 },
    { 58, 44 }, { 45, 59 }, { 60, 44 }, { 45, 61 }, { 62, 46 }, { 47, 63 }, { 64, 48 }, { 49, 65 },
    { 66, 48 }, { 49, 67 }, { 68, 50 }, { 51, 69 }, { 70, 52 }, { 53, 71 }, { 72, 52 }, { 53, 73 },
    { 74, 54 }, { 55, 75 }, { 76, 54 }, { 55, 77 }, { 78, 56 }, { 57, 79 }, { 80, 58 }, { 59, 81 },
    { 82, 58 }, { 59, 83 }, { 84, 60 }, { 61, 85 }, { 86, 60 }, { 61, 87 }, { 88, 60 }, { 61, 89 },
    { 90, 62 }, { 63, 91 }, { 92, 64 }, { 65, 93 }, { 94, 64 }, { 65, 95 }, { 96, 66 }, { 67, 97 },
    { 98, 66 }, { 67, 99 }, { 100, 66 }, { 67, 101 }, { 102, 68 }, { 69, 103 }, { 104, 68 }, { 69, 105 },
    { 106, 70 }, { 71, 107 }, { 108, 70 }, { 71, 109 }, { 110, 70 }, { 71, 111 }, { 112, 72 }, { 73, 113 },
    { 114, 72 }, { 73, 115 }, { 116, 72 }, { 73, 117 }, { 118, 74 }, { 75, 119 }, { 120, 74 }, { 75, 121 },
    { 122, 74 }, { 75, 123 }, { 124, 76 }, { 77, 125 }, { 124, 76 }, { 77, 125 }, { 126, 126 }, { 127, 127 }
};

SBac::SBac()
    : m_fracBits(0)
{
    memset(m_contextModels, 0, sizeof(m_contextModels));
}

void SBac::encodeTransform(TComDataCU* cu, CoeffCodeState& state, uint32_t offsetLuma, uint32_t offsetChroma, uint32_t absPartIdx,
                              uint32_t absPartIdxStep, uint32_t depth, uint32_t log2TrSize, uint32_t trIdx, bool& bCodeDQP)
{
    const bool subdiv = cu->getTransformIdx(absPartIdx) + cu->getDepth(absPartIdx) > (uint8_t)depth;
//    const uint32_t log2TrSize = cu->getSlice()->getSPS()->getLog2MaxCodingBlockSize() - depth;
    uint32_t hChromaShift = cu->getHorzChromaShift();
    uint32_t vChromaShift = cu->getVertChromaShift();
    uint32_t cbfY = cu->getCbf(absPartIdx, TEXT_LUMA, trIdx);
    uint32_t cbfU = cu->getCbf(absPartIdx, TEXT_CHROMA_U, trIdx);
    uint32_t cbfV = cu->getCbf(absPartIdx, TEXT_CHROMA_V, trIdx);

    if (!trIdx)
        state.bakAbsPartIdxCU = absPartIdx;

    if ((log2TrSize == 2) && !(cu->getChromaFormat() == CHROMA_444))
    {
        uint32_t partNum = cu->getPic()->getNumPartInCU() >> ((depth - 1) << 1);
        if ((absPartIdx & (partNum - 1)) == 0)
        {
            state.bakAbsPartIdx   = absPartIdx;
            state.bakChromaOffset = offsetChroma;
        }
        else if ((absPartIdx & (partNum - 1)) == (partNum - 1))
        {
            cbfU = cu->getCbf(state.bakAbsPartIdx, TEXT_CHROMA_U, trIdx);
            cbfV = cu->getCbf(state.bakAbsPartIdx, TEXT_CHROMA_V, trIdx);
        }
    }

    if (cu->getPredictionMode(absPartIdx) == MODE_INTRA && cu->getPartitionSize(absPartIdx) == SIZE_NxN && depth == cu->getDepth(absPartIdx))
    {
        X265_CHECK(subdiv, "subdivision state failure\n");
    }
    else if (cu->getPredictionMode(absPartIdx) == MODE_INTER && (cu->getPartitionSize(absPartIdx) != SIZE_2Nx2N) && depth == cu->getDepth(absPartIdx) &&  (cu->getSlice()->getSPS()->getQuadtreeTUMaxDepthInter() == 1))
    {
        if (log2TrSize > cu->getQuadtreeTULog2MinSizeInCU(absPartIdx))
        {
            X265_CHECK(subdiv, "subdivision state failure\n");
        }
        else
        {
            X265_CHECK(!subdiv, "subdivision state failure\n");
        }
    }
    else if (log2TrSize > cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize())
    {
        X265_CHECK(subdiv, "subdivision state failure\n");
    }
    else if (log2TrSize == cu->getSlice()->getSPS()->getQuadtreeTULog2MinSize())
    {
        X265_CHECK(!subdiv, "subdivision state failure\n");
    }
    else if (log2TrSize == cu->getQuadtreeTULog2MinSizeInCU(absPartIdx))
    {
        X265_CHECK(!subdiv, "subdivision state failure\n");
    }
    else
    {
        X265_CHECK(log2TrSize > cu->getQuadtreeTULog2MinSizeInCU(absPartIdx), "transform size failure\n");
        codeTransformSubdivFlag(subdiv, 5 - log2TrSize);
    }

    const uint32_t trDepthCurr = depth - cu->getDepth(absPartIdx);
    const bool bFirstCbfOfCU = trDepthCurr == 0;

    bool mCodeAll = true;
    const uint32_t numPels = 1 << (log2TrSize * 2 - hChromaShift - vChromaShift);
    if (numPels < (MIN_TU_SIZE * MIN_TU_SIZE))
        mCodeAll = false;

    if (bFirstCbfOfCU || mCodeAll)
    {
        uint32_t tuSize = 1 << log2TrSize;
        if (bFirstCbfOfCU || cu->getCbf(absPartIdx, TEXT_CHROMA_U, trDepthCurr - 1))
            codeQtCbf(cu, absPartIdx, absPartIdxStep, (tuSize >> hChromaShift), (tuSize >> vChromaShift), TEXT_CHROMA_U, trDepthCurr, (subdiv == 0));
        if (bFirstCbfOfCU || cu->getCbf(absPartIdx, TEXT_CHROMA_V, trDepthCurr - 1))
            codeQtCbf(cu, absPartIdx, absPartIdxStep, (tuSize >> hChromaShift), (tuSize >> vChromaShift), TEXT_CHROMA_V, trDepthCurr, (subdiv == 0));
    }
    else
    {
        X265_CHECK(cu->getCbf(absPartIdx, TEXT_CHROMA_U, trDepthCurr) == cu->getCbf(absPartIdx, TEXT_CHROMA_U, trDepthCurr - 1), "chroma xform size match failure\n");
        X265_CHECK(cu->getCbf(absPartIdx, TEXT_CHROMA_V, trDepthCurr) == cu->getCbf(absPartIdx, TEXT_CHROMA_V, trDepthCurr - 1), "chroma xform size match failure\n");
    }

    if (subdiv)
    {
        log2TrSize--;
        uint32_t numCoeff  = 1 << (log2TrSize * 2);
        uint32_t numCoeffC = (numCoeff >> (hChromaShift + vChromaShift));
        trIdx++;
        ++depth;
        absPartIdxStep >>= 2;
        const uint32_t partNum = cu->getPic()->getNumPartInCU() >> (depth << 1);

        encodeTransform(cu, state, offsetLuma, offsetChroma, absPartIdx, absPartIdxStep, depth, log2TrSize, trIdx, bCodeDQP);

        absPartIdx += partNum;
        offsetLuma += numCoeff;
        offsetChroma += numCoeffC;
        encodeTransform(cu, state, offsetLuma, offsetChroma, absPartIdx, absPartIdxStep, depth, log2TrSize, trIdx, bCodeDQP);

        absPartIdx += partNum;
        offsetLuma += numCoeff;
        offsetChroma += numCoeffC;
        encodeTransform(cu, state, offsetLuma, offsetChroma, absPartIdx, absPartIdxStep, depth, log2TrSize, trIdx, bCodeDQP);

        absPartIdx += partNum;
        offsetLuma += numCoeff;
        offsetChroma += numCoeffC;
        encodeTransform(cu, state, offsetLuma, offsetChroma, absPartIdx, absPartIdxStep, depth, log2TrSize, trIdx, bCodeDQP);
    }
    else
    {
        if (cu->getPredictionMode(absPartIdx) != MODE_INTRA && depth == cu->getDepth(absPartIdx) && !cu->getCbf(absPartIdx, TEXT_CHROMA_U, 0) && !cu->getCbf(absPartIdx, TEXT_CHROMA_V, 0))
        {
            X265_CHECK(cu->getCbf(absPartIdx, TEXT_LUMA, 0), "CBF should have been set\n");
        }
        else
            codeQtCbf(cu, absPartIdx, TEXT_LUMA, cu->getTransformIdx(absPartIdx));

        if (cbfY || cbfU || cbfV)
        {
            // dQP: only for LCU once
            if (cu->getSlice()->getPPS()->getUseDQP())
            {
                if (bCodeDQP)
                {
                    codeDeltaQP(cu, state.bakAbsPartIdxCU);
                    bCodeDQP = false;
                }
            }
        }
        if (cbfY)
            codeCoeffNxN(cu, (cu->getCoeffY() + offsetLuma), absPartIdx, log2TrSize, TEXT_LUMA);

        int chFmt = cu->getChromaFormat();
        if ((log2TrSize == 2) && !(chFmt == CHROMA_444))
        {
            uint32_t partNum = cu->getPic()->getNumPartInCU() >> ((depth - 1) << 1);
            if ((absPartIdx & (partNum - 1)) == (partNum - 1))
            {
                const uint32_t log2TrSizeC = 2;
                const bool splitIntoSubTUs = (chFmt == CHROMA_422);

                uint32_t curPartNum = cu->getPic()->getNumPartInCU() >> ((depth - 1) << 1);

                for (uint32_t chromaId = TEXT_CHROMA_U; chromaId <= TEXT_CHROMA_V; chromaId++)
                {
                    TURecurse tuIterator(splitIntoSubTUs ? VERTICAL_SPLIT : DONT_SPLIT, curPartNum, state.bakAbsPartIdx);
                    coeff_t* coeffChroma = cu->getCoeff((TextType)chromaId);
                    do
                    {
                        uint32_t cbf = cu->getCbf(tuIterator.absPartIdxTURelCU, (TextType)chromaId, trIdx + splitIntoSubTUs);
                        if (cbf)
                        {
                            uint32_t subTUOffset = tuIterator.section << (log2TrSizeC * 2);
                            codeCoeffNxN(cu, (coeffChroma + state.bakChromaOffset + subTUOffset), tuIterator.absPartIdxTURelCU, log2TrSizeC, (TextType)chromaId);
                        }
                    }
                    while (tuIterator.isNextSection());
                }
            }
        }
        else
        {
            uint32_t log2TrSizeC = log2TrSize - hChromaShift;
            const bool splitIntoSubTUs = (chFmt == CHROMA_422);
            uint32_t curPartNum = cu->getPic()->getNumPartInCU() >> (depth << 1);
            for (uint32_t chromaId = TEXT_CHROMA_U; chromaId <= TEXT_CHROMA_V; chromaId++)
            {
                TURecurse tuIterator(splitIntoSubTUs ? VERTICAL_SPLIT : DONT_SPLIT, curPartNum, absPartIdx);
                coeff_t* coeffChroma = cu->getCoeff((TextType)chromaId);
                do
                {
                    uint32_t cbf = cu->getCbf(tuIterator.absPartIdxTURelCU, (TextType)chromaId, trIdx + splitIntoSubTUs);
                    if (cbf)
                    {
                        uint32_t subTUOffset = tuIterator.section << (log2TrSizeC * 2);
                        codeCoeffNxN(cu, (coeffChroma + offsetChroma + subTUOffset), tuIterator.absPartIdxTURelCU, log2TrSizeC, (TextType)chromaId);
                    }
                }
                while (tuIterator.isNextSection());
            }
        }
    }
}

void SBac::codePredInfo(TComDataCU* cu, uint32_t absPartIdx)
{
    if (cu->isIntra(absPartIdx)) // If it is intra mode, encode intra prediction mode.
    {
        codeIntraDirLumaAng(cu, absPartIdx, true);
        int chFmt = cu->getChromaFormat();
        if (chFmt != CHROMA_400)
        {
            codeIntraDirChroma(cu, absPartIdx);

            if ((chFmt == CHROMA_444) && (cu->getPartitionSize(absPartIdx) == SIZE_NxN))
            {
                uint32_t partOffset = (cu->getPic()->getNumPartInCU() >> (cu->getDepth(absPartIdx) << 1)) >> 2;
                codeIntraDirChroma(cu, absPartIdx + partOffset);
                codeIntraDirChroma(cu, absPartIdx + partOffset * 2);
                codeIntraDirChroma(cu, absPartIdx + partOffset * 3);
            }
        }
    }
    else                        // if it is inter mode, encode motion vector and reference index
    {
        codePUWise(cu, absPartIdx);
    }
}

/** encode motion information for every PU block */
void SBac::codePUWise(TComDataCU* cu, uint32_t absPartIdx)
{
    PartSize partSize = cu->getPartitionSize(absPartIdx);
    uint32_t numPU = (partSize == SIZE_2Nx2N ? 1 : (partSize == SIZE_NxN ? 4 : 2));
    uint32_t depth = cu->getDepth(absPartIdx);
    uint32_t puOffset = (g_puOffset[uint32_t(partSize)] << ((cu->getSlice()->getSPS()->getMaxCUDepth() - depth) << 1)) >> 4;

    for (uint32_t partIdx = 0, subPartIdx = absPartIdx; partIdx < numPU; partIdx++, subPartIdx += puOffset)
    {
        codeMergeFlag(cu, subPartIdx);
        if (cu->getMergeFlag(subPartIdx))
        {
            codeMergeIndex(cu, subPartIdx);
        }
        else
        {
            uint32_t interDir = cu->getInterDir(subPartIdx);
            if (cu->getSlice()->isInterB())
                codeInterDir(cu, subPartIdx);
            for (uint32_t refListIdx = 0; refListIdx < 2; refListIdx++)
            {
                if (interDir & (1 << refListIdx))
                {
                    X265_CHECK(cu->getSlice()->getNumRefIdx(refListIdx) > 0, "numRefs should have been > 0\n");

                    codeRefFrmIdxPU(cu, subPartIdx, refListIdx);
                    codeMvd(cu, subPartIdx, refListIdx);
                    codeMVPIdx(cu->getMVPIdx(refListIdx, subPartIdx));
                }
            }
        }
    }
}

/** encode reference frame index for a PU block */
void SBac::codeRefFrmIdxPU(TComDataCU* cu, uint32_t absPartIdx, int list)
{
    X265_CHECK(!cu->isIntra(absPartIdx), "intra block expected\n");
    if ((cu->getSlice()->getNumRefIdx(list) == 1))
        return;

    X265_CHECK(cu->getInterDir(absPartIdx) & (1 << list), "inter dir failure\n");
    codeRefFrmIdx(cu, absPartIdx, list);
}

void SBac::codeCoeff(TComDataCU* cu, uint32_t absPartIdx, uint32_t depth, bool& bCodeDQP)
{
    if (!cu->isIntra(absPartIdx))
    {
        if (!(cu->getMergeFlag(absPartIdx) && cu->getPartitionSize(absPartIdx) == SIZE_2Nx2N))
            codeQtRootCbf(cu, absPartIdx);
        if (!cu->getQtRootCbf(absPartIdx))
            return;
    }

    uint32_t log2CUSize   = cu->getLog2CUSize(absPartIdx);
    uint32_t lumaOffset   = absPartIdx << cu->getPic()->getLog2UnitSize() * 2;
    uint32_t chromaOffset = lumaOffset >> (cu->getHorzChromaShift() + cu->getVertChromaShift());
    uint32_t absPartIdxStep = cu->getPic()->getNumPartInCU() >> (depth << 1);
    CoeffCodeState state;
    encodeTransform(cu, state, lumaOffset, chromaOffset, absPartIdx, absPartIdxStep, depth, log2CUSize, 0, bCodeDQP);
}

void SBac::codeSaoOffset(SaoLcuParam* saoLcuParam, uint32_t compIdx)
{
    uint32_t symbol;
    int i;

    symbol = saoLcuParam->typeIdx + 1;
    if (compIdx != 2)
        codeSaoTypeIdx(symbol);

    if (symbol)
    {
        if (saoLcuParam->typeIdx < 4 && compIdx != 2)
        {
            saoLcuParam->subTypeIdx = saoLcuParam->typeIdx;
        }
        int offsetTh = 1 << X265_MIN(X265_DEPTH - 5, 5);
        if (saoLcuParam->typeIdx == SAO_BO)
        {
            for (i = 0; i < saoLcuParam->length; i++)
            {
                uint32_t absOffset = ((saoLcuParam->offset[i] < 0) ? -saoLcuParam->offset[i] : saoLcuParam->offset[i]);
                codeSaoMaxUvlc(absOffset, offsetTh - 1);
            }

            for (i = 0; i < saoLcuParam->length; i++)
            {
                if (saoLcuParam->offset[i] != 0)
                {
                    uint32_t sign = (saoLcuParam->offset[i] < 0) ? 1 : 0;
                    codeSAOSign(sign);
                }
            }

            symbol = (uint32_t)(saoLcuParam->subTypeIdx);
            codeSaoUflc(5, symbol);
        }
        else if (saoLcuParam->typeIdx < 4)
        {
            codeSaoMaxUvlc(saoLcuParam->offset[0], offsetTh - 1);
            codeSaoMaxUvlc(saoLcuParam->offset[1], offsetTh - 1);
            codeSaoMaxUvlc(-saoLcuParam->offset[2], offsetTh - 1);
            codeSaoMaxUvlc(-saoLcuParam->offset[3], offsetTh - 1);
            if (compIdx != 2)
            {
                symbol = (uint32_t)(saoLcuParam->subTypeIdx);
                codeSaoUflc(2, symbol);
            }
        }
    }
}

void SBac::codeSaoUnitInterleaving(int compIdx, bool saoFlag, int rx, int ry, SaoLcuParam* saoLcuParam, int cuAddrInSlice, int cuAddrUpInSlice, int allowMergeLeft, int allowMergeUp)
{
    if (saoFlag)
    {
        if (rx > 0 && cuAddrInSlice != 0 && allowMergeLeft)
            codeSaoMerge(saoLcuParam->mergeLeftFlag);
        else
            saoLcuParam->mergeLeftFlag = 0;

        if (!saoLcuParam->mergeLeftFlag)
        {
            if ((ry > 0) && (cuAddrUpInSlice >= 0) && allowMergeUp)
                codeSaoMerge(saoLcuParam->mergeUpFlag);
            else
                saoLcuParam->mergeUpFlag = 0;

            if (!saoLcuParam->mergeUpFlag)
                codeSaoOffset(saoLcuParam, compIdx);
        }
    }
}

/** initialize context model with respect to QP and initialization value */
uint8_t sbacInit(int qp, int initValue)
{
    qp = Clip3(0, 51, qp);

    int  slope      = (initValue >> 4) * 5 - 45;
    int  offset     = ((initValue & 15) << 3) - 16;
    int  initState  =  X265_MIN(X265_MAX(1, (((slope * qp) >> 4) + offset)), 126);
    uint32_t mpState = (initState >= 64);
    uint32_t state = ((mpState ? (initState - 64) : (63 - initState)) << 1) + mpState;

    return (uint8_t)state;
}

static void initBuffer(ContextModel* contextModel, SliceType sliceType, int qp, uint8_t* ctxModel, int size)
{
    ctxModel += sliceType * size;

    for (int n = 0; n < size; n++)
    {
        contextModel[n].m_state = sbacInit(qp, ctxModel[n]);
        contextModel[n].bBinsCoded = 0;
    }
}

static uint32_t calcCost(ContextModel *contextModel, SliceType sliceType, int qp, uint8_t* ctxModel, int size)
{
    uint32_t cost = 0;

    ctxModel += sliceType * size;

    for (int n = 0; n < size; n++)
    {
        ContextModel tmpContextModel;
        tmpContextModel.m_state = sbacInit(qp, ctxModel[n]);

        // Map the 64 CABAC states to their corresponding probability values
        static const double aStateToProbLPS[] = { 0.50000000, 0.47460857, 0.45050660, 0.42762859, 0.40591239, 0.38529900, 0.36573242, 0.34715948, 0.32952974, 0.31279528, 0.29691064, 0.28183267, 0.26752040, 0.25393496, 0.24103941, 0.22879875, 0.21717969, 0.20615069, 0.19568177, 0.18574449, 0.17631186, 0.16735824, 0.15885931, 0.15079198, 0.14313433, 0.13586556, 0.12896592, 0.12241667, 0.11620000, 0.11029903, 0.10469773, 0.09938088, 0.09433404, 0.08954349, 0.08499621, 0.08067986, 0.07658271, 0.07269362, 0.06900203, 0.06549791, 0.06217174, 0.05901448, 0.05601756, 0.05317283, 0.05047256, 0.04790942, 0.04547644, 0.04316702, 0.04097487, 0.03889405, 0.03691890, 0.03504406, 0.03326442, 0.03157516, 0.02997168, 0.02844963, 0.02700488, 0.02563349, 0.02433175, 0.02309612, 0.02192323, 0.02080991, 0.01975312, 0.01875000 };

        double probLPS = aStateToProbLPS[sbacGetState(contextModel[n].m_state)];
        double prob0, prob1;
        if (sbacGetMps(contextModel[n].m_state) == 1)
        {
            prob0 = probLPS;
            prob1 = 1.0 - prob0;
        }
        else
        {
            prob1 = probLPS;
            prob0 = 1.0 - prob1;
        }

        if (contextModel[n].bBinsCoded > 0)
        {
            cost += (uint32_t)(prob0 * sbacGetEntropyBits(tmpContextModel.m_state, 0) + prob1 * sbacGetEntropyBits(tmpContextModel.m_state, 1));
        }
    }

    return cost;
}

void SBac::resetEntropy(TComSlice *slice)
{
    int  qp              = slice->getSliceQp();
    SliceType sliceType  = slice->getSliceType();

    int encCABACTableIdx = slice->getPPS()->getEncCABACTableIdx();

    if (!slice->isIntra() && (encCABACTableIdx == B_SLICE || encCABACTableIdx == P_SLICE) && slice->getPPS()->getCabacInitPresentFlag())
        sliceType = (SliceType)encCABACTableIdx;

    initBuffer(&m_contextModels[OFF_SPLIT_FLAG_CTX], sliceType, qp, (uint8_t*)INIT_SPLIT_FLAG, NUM_SPLIT_FLAG_CTX);
    initBuffer(&m_contextModels[OFF_SKIP_FLAG_CTX], sliceType, qp, (uint8_t*)INIT_SKIP_FLAG, NUM_SKIP_FLAG_CTX);
    initBuffer(&m_contextModels[OFF_MERGE_FLAG_EXT_CTX], sliceType, qp, (uint8_t*)INIT_MERGE_FLAG_EXT, NUM_MERGE_FLAG_EXT_CTX);
    initBuffer(&m_contextModels[OFF_MERGE_IDX_EXT_CTX], sliceType, qp, (uint8_t*)INIT_MERGE_IDX_EXT, NUM_MERGE_IDX_EXT_CTX);
    initBuffer(&m_contextModels[OFF_PART_SIZE_CTX], sliceType, qp, (uint8_t*)INIT_PART_SIZE, NUM_PART_SIZE_CTX);
    initBuffer(&m_contextModels[OFF_PRED_MODE_CTX], sliceType, qp, (uint8_t*)INIT_PRED_MODE, NUM_PRED_MODE_CTX);
    initBuffer(&m_contextModels[OFF_ADI_CTX], sliceType, qp, (uint8_t*)INIT_INTRA_PRED_MODE, NUM_ADI_CTX);
    initBuffer(&m_contextModels[OFF_CHROMA_PRED_CTX], sliceType, qp, (uint8_t*)INIT_CHROMA_PRED_MODE, NUM_CHROMA_PRED_CTX);
    initBuffer(&m_contextModels[OFF_DELTA_QP_CTX], sliceType, qp, (uint8_t*)INIT_DQP, NUM_DELTA_QP_CTX);
    initBuffer(&m_contextModels[OFF_INTER_DIR_CTX], sliceType, qp, (uint8_t*)INIT_INTER_DIR, NUM_INTER_DIR_CTX);
    initBuffer(&m_contextModels[OFF_REF_NO_CTX], sliceType, qp, (uint8_t*)INIT_REF_PIC, NUM_REF_NO_CTX);
    initBuffer(&m_contextModels[OFF_MV_RES_CTX], sliceType, qp, (uint8_t*)INIT_MVD, NUM_MV_RES_CTX);
    initBuffer(&m_contextModels[OFF_QT_CBF_CTX], sliceType, qp, (uint8_t*)INIT_QT_CBF, NUM_QT_CBF_CTX);
    initBuffer(&m_contextModels[OFF_TRANS_SUBDIV_FLAG_CTX], sliceType, qp, (uint8_t*)INIT_TRANS_SUBDIV_FLAG, NUM_TRANS_SUBDIV_FLAG_CTX);
    initBuffer(&m_contextModels[OFF_QT_ROOT_CBF_CTX], sliceType, qp, (uint8_t*)INIT_QT_ROOT_CBF, NUM_QT_ROOT_CBF_CTX);
    initBuffer(&m_contextModels[OFF_SIG_CG_FLAG_CTX], sliceType, qp, (uint8_t*)INIT_SIG_CG_FLAG, 2 * NUM_SIG_CG_FLAG_CTX);
    initBuffer(&m_contextModels[OFF_SIG_FLAG_CTX], sliceType, qp, (uint8_t*)INIT_SIG_FLAG, NUM_SIG_FLAG_CTX);
    initBuffer(&m_contextModels[OFF_CTX_LAST_FLAG_X], sliceType, qp, (uint8_t*)INIT_LAST, NUM_CTX_LAST_FLAG_XY);
    initBuffer(&m_contextModels[OFF_CTX_LAST_FLAG_Y], sliceType, qp, (uint8_t*)INIT_LAST, NUM_CTX_LAST_FLAG_XY);
    initBuffer(&m_contextModels[OFF_ONE_FLAG_CTX], sliceType, qp, (uint8_t*)INIT_ONE_FLAG, NUM_ONE_FLAG_CTX);
    initBuffer(&m_contextModels[OFF_ABS_FLAG_CTX], sliceType, qp, (uint8_t*)INIT_ABS_FLAG, NUM_ABS_FLAG_CTX);
    initBuffer(&m_contextModels[OFF_MVP_IDX_CTX], sliceType, qp, (uint8_t*)INIT_MVP_IDX, NUM_MVP_IDX_CTX);
    initBuffer(&m_contextModels[OFF_SAO_MERGE_FLAG_CTX], sliceType, qp, (uint8_t*)INIT_SAO_MERGE_FLAG, NUM_SAO_MERGE_FLAG_CTX);
    initBuffer(&m_contextModels[OFF_SAO_TYPE_IDX_CTX], sliceType, qp, (uint8_t*)INIT_SAO_TYPE_IDX, NUM_SAO_TYPE_IDX_CTX);
    initBuffer(&m_contextModels[OFF_TRANSFORMSKIP_FLAG_CTX], sliceType, qp, (uint8_t*)INIT_TRANSFORMSKIP_FLAG, 2 * NUM_TRANSFORMSKIP_FLAG_CTX);
    initBuffer(&m_contextModels[OFF_CU_TRANSQUANT_BYPASS_FLAG_CTX], sliceType, qp, (uint8_t*)INIT_CU_TRANSQUANT_BYPASS_FLAG, NUM_CU_TRANSQUANT_BYPASS_FLAG_CTX);
    // new structure

    start();
}

/* If current slice type is P/B then it determines the distance of
 * initialisation type 1 and 2 from the current CABAC states and stores the
 * index of the closest table.  This index is used for the next P/B slice when
 * cabac_init_present_flag is true.
 */
void SBac::determineCabacInitIdx(TComSlice *slice)
{
    int qp = slice->getSliceQp();

    if (!slice->isIntra())
    {
        SliceType aSliceTypeChoices[] = { B_SLICE, P_SLICE };

        uint32_t bestCost       = MAX_UINT;
        SliceType bestSliceType = aSliceTypeChoices[0];
        for (uint32_t idx = 0; idx < 2; idx++)
        {
            uint32_t curCost = 0;
            SliceType curSliceType = aSliceTypeChoices[idx];

            curCost  = calcCost(&m_contextModels[OFF_SPLIT_FLAG_CTX], curSliceType, qp, (uint8_t*)INIT_SPLIT_FLAG, NUM_SPLIT_FLAG_CTX);
            curCost += calcCost(&m_contextModels[OFF_SKIP_FLAG_CTX], curSliceType, qp, (uint8_t*)INIT_SKIP_FLAG, NUM_SKIP_FLAG_CTX);
            curCost += calcCost(&m_contextModels[OFF_MERGE_FLAG_EXT_CTX], curSliceType, qp, (uint8_t*)INIT_MERGE_FLAG_EXT, NUM_MERGE_FLAG_EXT_CTX);
            curCost += calcCost(&m_contextModels[OFF_MERGE_IDX_EXT_CTX], curSliceType, qp, (uint8_t*)INIT_MERGE_IDX_EXT, NUM_MERGE_IDX_EXT_CTX);
            curCost += calcCost(&m_contextModels[OFF_PART_SIZE_CTX], curSliceType, qp, (uint8_t*)INIT_PART_SIZE, NUM_PART_SIZE_CTX);
            curCost += calcCost(&m_contextModels[OFF_PRED_MODE_CTX], curSliceType, qp, (uint8_t*)INIT_PRED_MODE, NUM_PRED_MODE_CTX);
            curCost += calcCost(&m_contextModels[OFF_ADI_CTX], curSliceType, qp, (uint8_t*)INIT_INTRA_PRED_MODE, NUM_ADI_CTX);
            curCost += calcCost(&m_contextModels[OFF_CHROMA_PRED_CTX], curSliceType, qp, (uint8_t*)INIT_CHROMA_PRED_MODE, NUM_CHROMA_PRED_CTX);
            curCost += calcCost(&m_contextModels[OFF_DELTA_QP_CTX], curSliceType, qp, (uint8_t*)INIT_DQP, NUM_DELTA_QP_CTX);
            curCost += calcCost(&m_contextModels[OFF_INTER_DIR_CTX], curSliceType, qp, (uint8_t*)INIT_INTER_DIR, NUM_INTER_DIR_CTX);
            curCost += calcCost(&m_contextModels[OFF_REF_NO_CTX], curSliceType, qp, (uint8_t*)INIT_REF_PIC, NUM_REF_NO_CTX);
            curCost += calcCost(&m_contextModels[OFF_MV_RES_CTX], curSliceType, qp, (uint8_t*)INIT_MVD, NUM_MV_RES_CTX);
            curCost += calcCost(&m_contextModels[OFF_QT_CBF_CTX], curSliceType, qp, (uint8_t*)INIT_QT_CBF, NUM_QT_CBF_CTX);
            curCost += calcCost(&m_contextModels[OFF_TRANS_SUBDIV_FLAG_CTX], curSliceType, qp, (uint8_t*)INIT_TRANS_SUBDIV_FLAG, NUM_TRANS_SUBDIV_FLAG_CTX);
            curCost += calcCost(&m_contextModels[OFF_QT_ROOT_CBF_CTX], curSliceType, qp, (uint8_t*)INIT_QT_ROOT_CBF, NUM_QT_ROOT_CBF_CTX);
            curCost += calcCost(&m_contextModels[OFF_SIG_CG_FLAG_CTX], curSliceType, qp, (uint8_t*)INIT_SIG_CG_FLAG, 2 * NUM_SIG_CG_FLAG_CTX);
            curCost += calcCost(&m_contextModels[OFF_SIG_FLAG_CTX], curSliceType, qp, (uint8_t*)INIT_SIG_FLAG, NUM_SIG_FLAG_CTX);
            curCost += calcCost(&m_contextModels[OFF_CTX_LAST_FLAG_X], curSliceType, qp, (uint8_t*)INIT_LAST, NUM_CTX_LAST_FLAG_XY);
            curCost += calcCost(&m_contextModels[OFF_CTX_LAST_FLAG_Y], curSliceType, qp, (uint8_t*)INIT_LAST, NUM_CTX_LAST_FLAG_XY);
            curCost += calcCost(&m_contextModels[OFF_ONE_FLAG_CTX], curSliceType, qp, (uint8_t*)INIT_ONE_FLAG, NUM_ONE_FLAG_CTX);
            curCost += calcCost(&m_contextModels[OFF_ABS_FLAG_CTX], curSliceType, qp, (uint8_t*)INIT_ABS_FLAG, NUM_ABS_FLAG_CTX);
            curCost += calcCost(&m_contextModels[OFF_MVP_IDX_CTX], curSliceType, qp, (uint8_t*)INIT_MVP_IDX, NUM_MVP_IDX_CTX);
            curCost += calcCost(&m_contextModels[OFF_SAO_MERGE_FLAG_CTX], curSliceType, qp, (uint8_t*)INIT_SAO_MERGE_FLAG, NUM_SAO_MERGE_FLAG_CTX);
            curCost += calcCost(&m_contextModels[OFF_SAO_TYPE_IDX_CTX], curSliceType, qp, (uint8_t*)INIT_SAO_TYPE_IDX, NUM_SAO_TYPE_IDX_CTX);
            curCost += calcCost(&m_contextModels[OFF_TRANSFORMSKIP_FLAG_CTX], curSliceType, qp, (uint8_t*)INIT_TRANSFORMSKIP_FLAG, 2 * NUM_TRANSFORMSKIP_FLAG_CTX);
            curCost += calcCost(&m_contextModels[OFF_CU_TRANSQUANT_BYPASS_FLAG_CTX], curSliceType, qp, (uint8_t*)INIT_CU_TRANSQUANT_BYPASS_FLAG, NUM_CU_TRANSQUANT_BYPASS_FLAG_CTX);
            if (curCost < bestCost)
            {
                bestSliceType = curSliceType;
                bestCost      = curCost;
            }
        }

        slice->getPPS()->setEncCABACTableIdx(bestSliceType);
    }
    else
    {
        slice->getPPS()->setEncCABACTableIdx(I_SLICE);
    }
}

void SBac::codeVPS(TComVPS* vps)
{
    WRITE_CODE(vps->getVPSId(),                    4,        "vps_video_parameter_set_id");
    WRITE_CODE(3,                                  2,        "vps_reserved_three_2bits");
    WRITE_CODE(0,                                  6,        "vps_reserved_zero_6bits");
    WRITE_CODE(vps->getMaxTLayers() - 1,           3,        "vps_max_sub_layers_minus1");
    WRITE_FLAG(vps->getTemporalNestingFlag(),                "vps_temporal_id_nesting_flag");
    X265_CHECK(vps->getMaxTLayers() > 1 || vps->getTemporalNestingFlag(), "layer flags not matchin\n");
    WRITE_CODE(0xffff,                            16,        "vps_reserved_ffff_16bits");
    codePTL(vps->getPTL(), true, vps->getMaxTLayers() - 1);
    WRITE_FLAG(true,             "vps_sub_layer_ordering_info_present_flag");
    for (uint32_t i = 0; i <= vps->getMaxTLayers() - 1; i++)
    {
        WRITE_UVLC(vps->getMaxDecPicBuffering(i) - 1,       "vps_max_dec_pic_buffering_minus1[i]");
        WRITE_UVLC(vps->getNumReorderPics(i),               "vps_num_reorder_pics[i]");
        WRITE_UVLC(vps->getMaxLatencyIncrease(i),           "vps_max_latency_increase_plus1[i]");
    }

    X265_CHECK(vps->getNumHrdParameters() <= MAX_VPS_NUM_HRD_PARAMETERS, "invalid HRD param\n");
    X265_CHECK(vps->getMaxNuhReservedZeroLayerId() < MAX_VPS_NUH_RESERVED_ZERO_LAYER_ID_PLUS1, "invalid layer\n");
    WRITE_CODE(vps->getMaxNuhReservedZeroLayerId(), 6,     "vps_max_nuh_reserved_zero_layer_id");
    vps->setMaxOpSets(1);
    WRITE_UVLC(vps->getMaxOpSets() - 1,                    "vps_max_op_sets_minus1");
    for (uint32_t opsIdx = 1; opsIdx <= (vps->getMaxOpSets() - 1); opsIdx++)
    {
        // Operation point set
        for (uint32_t i = 0; i <= vps->getMaxNuhReservedZeroLayerId(); i++)
        {
            // Only applicable for version 1
            vps->setLayerIdIncludedFlag(true, opsIdx, i);
            WRITE_FLAG(vps->getLayerIdIncludedFlag(opsIdx, i) ? 1 : 0, "layer_id_included_flag[opsIdx][i]");
        }
    }

    TimingInfo *timingInfo = vps->getTimingInfo();
    WRITE_FLAG(timingInfo->getTimingInfoPresentFlag(),          "vps_timing_info_present_flag");
    if (timingInfo->getTimingInfoPresentFlag())
    {
        WRITE_CODE(timingInfo->getNumUnitsInTick(), 32,           "vps_num_units_in_tick");
        WRITE_CODE(timingInfo->getTimeScale(),      32,           "vps_time_scale");
        WRITE_FLAG(timingInfo->getPocProportionalToTimingFlag(),  "vps_poc_proportional_to_timing_flag");
        if (timingInfo->getPocProportionalToTimingFlag())
        {
            WRITE_UVLC(timingInfo->getNumTicksPocDiffOneMinus1(),   "vps_num_ticks_poc_diff_one_minus1");
        }
        vps->setNumHrdParameters(0);
        WRITE_UVLC(vps->getNumHrdParameters(),                 "vps_num_hrd_parameters");

        if (vps->getNumHrdParameters() > 0)
        {
            vps->createHrdParamBuffer();
        }
        for (uint32_t i = 0; i < vps->getNumHrdParameters(); i++)
        {
            // Only applicable for version 1
            vps->setHrdOpSetIdx(0, i);
            WRITE_UVLC(vps->getHrdOpSetIdx(i),                "hrd_op_set_idx");
            if (i > 0)
            {
                WRITE_FLAG(vps->getCprmsPresentFlag(i) ? 1 : 0, "cprms_present_flag[i]");
            }
            codeHrdParameters(vps->getHrdParameters(i), vps->getCprmsPresentFlag(i), vps->getMaxTLayers() - 1);
        }
    }
    WRITE_FLAG(0,                     "vps_extension_flag");

    //future extensions here..
}

void SBac::codeShortTermRefPicSet(TComReferencePictureSet* rps)
{
    WRITE_UVLC(rps->getNumberOfNegativePictures(), "num_negative_pics");
    WRITE_UVLC(rps->getNumberOfPositivePictures(), "num_positive_pics");
    int prev = 0;
    for (int j = 0; j < rps->getNumberOfNegativePictures(); j++)
    {
        WRITE_UVLC(prev - rps->getDeltaPOC(j) - 1, "delta_poc_s0_minus1");
        prev = rps->getDeltaPOC(j);
        WRITE_FLAG(rps->getUsed(j), "used_by_curr_pic_s0_flag");
    }

    prev = 0;
    for (int j = rps->getNumberOfNegativePictures(); j < rps->getNumberOfNegativePictures() + rps->getNumberOfPositivePictures(); j++)
    {
        WRITE_UVLC(rps->getDeltaPOC(j) - prev - 1, "delta_poc_s1_minus1");
        prev = rps->getDeltaPOC(j);
        WRITE_FLAG(rps->getUsed(j), "used_by_curr_pic_s1_flag");
    }
}

void SBac::codeSPS(TComSPS* sps, TComScalingList *scalingList)
{
    WRITE_CODE(sps->getVPSId(),          4,       "sps_video_parameter_set_id");
    WRITE_CODE(sps->getMaxTLayers() - 1,  3,       "sps_max_sub_layers_minus1");
    WRITE_FLAG(sps->getTemporalIdNestingFlag() ? 1 : 0, "sps_temporal_id_nesting_flag");
    codePTL(sps->getPTL(), 1, sps->getMaxTLayers() - 1);
    WRITE_UVLC(sps->getSPSId(),                   "sps_seq_parameter_set_id");
    WRITE_UVLC(sps->getChromaFormatIdc(),         "chroma_format_idc");

    if (sps->getChromaFormatIdc() == CHROMA_444)
    {
        WRITE_FLAG(0,                             "separate_colour_plane_flag");
    }

    WRITE_UVLC(sps->getPicWidthInLumaSamples(),   "pic_width_in_luma_samples");
    WRITE_UVLC(sps->getPicHeightInLumaSamples(),  "pic_height_in_luma_samples");
    Window conf = sps->getConformanceWindow();

    WRITE_FLAG(conf.m_enabledFlag,          "conformance_window_flag");
    if (conf.m_enabledFlag)
    {
        WRITE_UVLC(conf.m_winLeftOffset   / g_winUnitX[sps->getChromaFormatIdc()], "conf_win_left_offset");
        WRITE_UVLC(conf.m_winRightOffset  / g_winUnitX[sps->getChromaFormatIdc()], "conf_win_right_offset");
        WRITE_UVLC(conf.m_winTopOffset    / g_winUnitY[sps->getChromaFormatIdc()], "conf_win_top_offset");
        WRITE_UVLC(conf.m_winBottomOffset / g_winUnitY[sps->getChromaFormatIdc()], "conf_win_bottom_offset");
    }

    WRITE_UVLC(sps->getBitDepthY() - 8,             "bit_depth_luma_minus8");
    WRITE_UVLC(sps->getBitDepthC() - 8,             "bit_depth_chroma_minus8");

    WRITE_UVLC(sps->getBitsForPOC() - 4,            "log2_max_pic_order_cnt_lsb_minus4");

    WRITE_FLAG(true,     "sps_sub_layer_ordering_info_present_flag");
    for (uint32_t i = 0; i <= sps->getMaxTLayers() - 1; i++)
    {
        WRITE_UVLC(sps->getMaxDecPicBuffering(i) - 1, "sps_max_dec_pic_buffering_minus1[i]");
        WRITE_UVLC(sps->getNumReorderPics(i),       "sps_num_reorder_pics[i]");
        WRITE_UVLC(sps->getMaxLatencyIncrease(i),   "sps_max_latency_increase_plus1[i]");
    }

    WRITE_UVLC(sps->getLog2MinCodingBlockSize() - 3,    "log2_min_coding_block_size_minus3");
    WRITE_UVLC(sps->getLog2DiffMaxMinCodingBlockSize(), "log2_diff_max_min_coding_block_size");
    WRITE_UVLC(sps->getQuadtreeTULog2MinSize() - 2,     "log2_min_transform_block_size_minus2");
    WRITE_UVLC(sps->getQuadtreeTULog2MaxSize() - sps->getQuadtreeTULog2MinSize(), "log2_diff_max_min_transform_block_size");
    WRITE_UVLC(sps->getQuadtreeTUMaxDepthInter() - 1,   "max_transform_hierarchy_depth_inter");
    WRITE_UVLC(sps->getQuadtreeTUMaxDepthIntra() - 1,   "max_transform_hierarchy_depth_intra");
    WRITE_FLAG(sps->getScalingListFlag() ? 1 : 0,       "scaling_list_enabled_flag");
    if (sps->getScalingListFlag())
    {
        WRITE_FLAG(sps->getScalingListPresentFlag() ? 1 : 0, "sps_scaling_list_data_present_flag");
        if (sps->getScalingListPresentFlag() && scalingList)
            codeScalingList(scalingList);
    }
    WRITE_FLAG(sps->getUseAMP() ? 1 : 0, "amp_enabled_flag");
    WRITE_FLAG(sps->getUseSAO() ? 1 : 0, "sample_adaptive_offset_enabled_flag");

    WRITE_FLAG(0, "pcm_enabled_flag");

    X265_CHECK(sps->getMaxTLayers() > 0, "max layers must be positive\n");

    WRITE_UVLC(0, "num_short_term_ref_pic_sets");

    WRITE_FLAG(sps->getLongTermRefsPresent() ? 1 : 0,      "long_term_ref_pics_present_flag");
    if (sps->getLongTermRefsPresent())
    {
        WRITE_UVLC(sps->getNumLongTermRefPicSPS(), "num_long_term_ref_pic_sps");
        for (uint32_t k = 0; k < sps->getNumLongTermRefPicSPS(); k++)
        {
            WRITE_CODE(sps->getLtRefPicPocLsbSps(k), sps->getBitsForPOC(), "lt_ref_pic_poc_lsb_sps");
            WRITE_FLAG(sps->getUsedByCurrPicLtSPSFlag(k), "used_by_curr_pic_lt_sps_flag");
        }
    }
    WRITE_FLAG(sps->getTMVPFlagsPresent()  ? 1 : 0,        "sps_temporal_mvp_enable_flag");

    WRITE_FLAG(sps->getUseStrongIntraSmoothing(),          "sps_strong_intra_smoothing_enable_flag");

    WRITE_FLAG(sps->getVuiParametersPresentFlag(),         "vui_parameters_present_flag");
    if (sps->getVuiParametersPresentFlag())
    {
        codeVUI(sps->getVuiParameters(), sps);
    }

    WRITE_FLAG(0, "sps_extension_flag");
}

void SBac::codePPS(TComPPS* pps, TComScalingList* scalingList)
{
    WRITE_UVLC(pps->getPPSId(),                            "pps_pic_parameter_set_id");
    WRITE_UVLC(pps->getSPSId(),                            "pps_seq_parameter_set_id");
    WRITE_FLAG(0,                                          "dependent_slice_segments_enabled_flag");
    WRITE_FLAG(pps->getOutputFlagPresentFlag() ? 1 : 0,    "output_flag_present_flag");
    WRITE_CODE(pps->getNumExtraSliceHeaderBits(), 3,       "num_extra_slice_header_bits");
    WRITE_FLAG(pps->getSignHideFlag(),                     "sign_data_hiding_flag");
    WRITE_FLAG(pps->getCabacInitPresentFlag() ? 1 : 0,     "cabac_init_present_flag");
    WRITE_UVLC(pps->getNumRefIdxL0DefaultActive() - 1,     "num_ref_idx_l0_default_active_minus1");
    WRITE_UVLC(pps->getNumRefIdxL1DefaultActive() - 1,     "num_ref_idx_l1_default_active_minus1");

    WRITE_SVLC(pps->getPicInitQPMinus26(),                 "init_qp_minus26");
    WRITE_FLAG(pps->getConstrainedIntraPred() ? 1 : 0,     "constrained_intra_pred_flag");
    WRITE_FLAG(pps->getUseTransformSkip() ? 1 : 0,         "transform_skip_enabled_flag");
    WRITE_FLAG(pps->getUseDQP() ? 1 : 0,                   "cu_qp_delta_enabled_flag");
    if (pps->getUseDQP())
    {
        WRITE_UVLC(pps->getMaxCuDQPDepth(),                "diff_cu_qp_delta_depth");
    }
    WRITE_SVLC(pps->getChromaCbQpOffset(),                 "pps_cb_qp_offset");
    WRITE_SVLC(pps->getChromaCrQpOffset(),                 "pps_cr_qp_offset");
    WRITE_FLAG(pps->getSliceChromaQpFlag() ? 1 : 0,        "pps_slice_chroma_qp_offsets_present_flag");

    WRITE_FLAG(pps->getUseWP() ? 1 : 0,                    "weighted_pred_flag");   // Use of Weighting Prediction (P_SLICE)
    WRITE_FLAG(pps->getWPBiPred() ? 1 : 0,                 "weighted_bipred_flag");  // Use of Weighting Bi-Prediction (B_SLICE)
    WRITE_FLAG(pps->getTransquantBypassEnableFlag() ? 1 : 0, "transquant_bypass_enable_flag");
    WRITE_FLAG(0,                                          "tiles_enabled_flag");
    WRITE_FLAG(pps->getEntropyCodingSyncEnabledFlag() ? 1 : 0, "entropy_coding_sync_enabled_flag");
    WRITE_FLAG(1,                                          "loop_filter_across_slices_enabled_flag");

    // TODO: Here have some time sequence problem, we set below field in initEncSlice(), but use them in getStreamHeaders() early
    WRITE_FLAG(pps->getDeblockingFilterControlPresentFlag() ? 1 : 0, "deblocking_filter_control_present_flag");
    if (pps->getDeblockingFilterControlPresentFlag())
    {
        WRITE_FLAG(pps->getDeblockingFilterOverrideEnabledFlag() ? 1 : 0,  "deblocking_filter_override_enabled_flag");
        WRITE_FLAG(pps->getPicDisableDeblockingFilterFlag() ? 1 : 0,       "pps_disable_deblocking_filter_flag");
        if (!pps->getPicDisableDeblockingFilterFlag())
        {
            WRITE_SVLC(pps->getDeblockingFilterBetaOffsetDiv2(), "pps_beta_offset_div2");
            WRITE_SVLC(pps->getDeblockingFilterTcOffsetDiv2(),   "pps_tc_offset_div2");
        }
    }
    WRITE_FLAG(pps->getScalingListPresentFlag() ? 1 : 0,         "pps_scaling_list_data_present_flag");
    if (pps->getScalingListPresentFlag() && scalingList)
        codeScalingList(scalingList);

    WRITE_FLAG(pps->getListsModificationPresentFlag(), "lists_modification_present_flag");
    WRITE_UVLC(pps->getLog2ParallelMergeLevelMinus2(), "log2_parallel_merge_level_minus2");
    WRITE_FLAG(pps->getSliceHeaderExtensionPresentFlag() ? 1 : 0, "slice_segment_header_extension_present_flag");
    WRITE_FLAG(0, "pps_extension_flag");
}

void SBac::codeVUI(TComVUI *vui, TComSPS* sps)
{
    WRITE_FLAG(vui->getAspectRatioInfoPresentFlag(),  "aspect_ratio_info_present_flag");
    if (vui->getAspectRatioInfoPresentFlag())
    {
        WRITE_CODE(vui->getAspectRatioIdc(), 8,       "aspect_ratio_idc");
        if (vui->getAspectRatioIdc() == 255)
        {
            WRITE_CODE(vui->getSarWidth(), 16,        "sar_width");
            WRITE_CODE(vui->getSarHeight(), 16,       "sar_height");
        }
    }
    WRITE_FLAG(vui->getOverscanInfoPresentFlag(),     "overscan_info_present_flag");
    if (vui->getOverscanInfoPresentFlag())
    {
        WRITE_FLAG(vui->getOverscanAppropriateFlag(), "overscan_appropriate_flag");
    }
    WRITE_FLAG(vui->getVideoSignalTypePresentFlag(),  "video_signal_type_present_flag");
    if (vui->getVideoSignalTypePresentFlag())
    {
        WRITE_CODE(vui->getVideoFormat(), 3,          "video_format");
        WRITE_FLAG(vui->getVideoFullRangeFlag(),      "video_full_range_flag");
        WRITE_FLAG(vui->getColourDescriptionPresentFlag(), "colour_description_present_flag");
        if (vui->getColourDescriptionPresentFlag())
        {
            WRITE_CODE(vui->getColourPrimaries(), 8,         "colour_primaries");
            WRITE_CODE(vui->getTransferCharacteristics(), 8, "transfer_characteristics");
            WRITE_CODE(vui->getMatrixCoefficients(), 8,      "matrix_coefficients");
        }
    }

    WRITE_FLAG(vui->getChromaLocInfoPresentFlag(),           "chroma_loc_info_present_flag");
    if (vui->getChromaLocInfoPresentFlag())
    {
        WRITE_UVLC(vui->getChromaSampleLocTypeTopField(),    "chroma_sample_loc_type_top_field");
        WRITE_UVLC(vui->getChromaSampleLocTypeBottomField(), "chroma_sample_loc_type_bottom_field");
    }

    WRITE_FLAG(vui->getNeutralChromaIndicationFlag(),        "neutral_chroma_indication_flag");
    WRITE_FLAG(vui->getFieldSeqFlag(),                       "field_seq_flag");
    WRITE_FLAG(vui->getFrameFieldInfoPresentFlag(),          "frame_field_info_present_flag");

    Window defaultDisplayWindow = vui->getDefaultDisplayWindow();
    WRITE_FLAG(defaultDisplayWindow.m_enabledFlag,           "default_display_window_flag");
    if (defaultDisplayWindow.m_enabledFlag)
    {
        WRITE_UVLC(defaultDisplayWindow.m_winLeftOffset,     "def_disp_win_left_offset");
        WRITE_UVLC(defaultDisplayWindow.m_winRightOffset,    "def_disp_win_right_offset");
        WRITE_UVLC(defaultDisplayWindow.m_winTopOffset,      "def_disp_win_top_offset");
        WRITE_UVLC(defaultDisplayWindow.m_winBottomOffset,   "def_disp_win_bottom_offset");
    }
    TimingInfo *timingInfo = vui->getTimingInfo();
    WRITE_FLAG(timingInfo->getTimingInfoPresentFlag(),       "vui_timing_info_present_flag");
    if (timingInfo->getTimingInfoPresentFlag())
    {
        WRITE_CODE(timingInfo->getNumUnitsInTick(), 32,      "vui_num_units_in_tick");
        WRITE_CODE(timingInfo->getTimeScale(),      32,      "vui_time_scale");
        WRITE_FLAG(timingInfo->getPocProportionalToTimingFlag(),  "vui_poc_proportional_to_timing_flag");
        if (timingInfo->getPocProportionalToTimingFlag())
        {
            WRITE_UVLC(timingInfo->getNumTicksPocDiffOneMinus1(),   "vui_num_ticks_poc_diff_one_minus1");
        }
        WRITE_FLAG(vui->getHrdParametersPresentFlag(),              "hrd_parameters_present_flag");
        if (vui->getHrdParametersPresentFlag())
        {
            codeHrdParameters(vui->getHrdParameters(), 1, sps->getMaxTLayers() - 1);
        }
    }
    WRITE_FLAG(vui->getBitstreamRestrictionFlag(),                "bitstream_restriction_flag");
    if (vui->getBitstreamRestrictionFlag())
    {
        WRITE_FLAG(vui->getTilesFixedStructureFlag(),             "tiles_fixed_structure_flag");
        WRITE_FLAG(vui->getMotionVectorsOverPicBoundariesFlag(),  "motion_vectors_over_pic_boundaries_flag");
        WRITE_FLAG(vui->getRestrictedRefPicListsFlag(),           "restricted_ref_pic_lists_flag");
        WRITE_UVLC(vui->getMinSpatialSegmentationIdc(),           "min_spatial_segmentation_idc");
        WRITE_UVLC(vui->getMaxBytesPerPicDenom(),                 "max_bytes_per_pic_denom");
        WRITE_UVLC(vui->getMaxBitsPerMinCuDenom(),                "max_bits_per_mincu_denom");
        WRITE_UVLC(vui->getLog2MaxMvLengthHorizontal(),           "log2_max_mv_length_horizontal");
        WRITE_UVLC(vui->getLog2MaxMvLengthVertical(),             "log2_max_mv_length_vertical");
    }
}

void SBac::codeAUD(TComSlice* slice)
{
    int picType;

    switch (slice->getSliceType())
    {
    case I_SLICE:
        picType = 0;
        break;
    case P_SLICE:
        picType = 1;
        break;
    case B_SLICE:
        picType = 2;
        break;
    default:
        picType = 7;
        break;
    }

    WRITE_CODE(picType, 3, "pic_type");
}

void SBac::codeHrdParameters(TComHRD *hrd, bool commonInfPresentFlag, uint32_t maxNumSubLayersMinus1)
{
    if (commonInfPresentFlag)
    {
        WRITE_FLAG(hrd->getNalHrdParametersPresentFlag() ? 1 : 0,  "nal_hrd_parameters_present_flag");
        WRITE_FLAG(hrd->getVclHrdParametersPresentFlag() ? 1 : 0,  "vcl_hrd_parameters_present_flag");
        if (hrd->getNalHrdParametersPresentFlag() || hrd->getVclHrdParametersPresentFlag())
        {
            WRITE_FLAG(hrd->getSubPicHrdParamsPresentFlag() ? 1 : 0,  "sub_pic_hrd_params_present_flag");
            if (hrd->getSubPicHrdParamsPresentFlag())
            {
                WRITE_CODE(hrd->getTickDivisorMinus2(), 8,              "tick_divisor_minus2");
                WRITE_CODE(hrd->getDuCpbRemovalDelayLengthMinus1(), 5,  "du_cpb_removal_delay_length_minus1");
                WRITE_FLAG(hrd->getSubPicCpbParamsInPicTimingSEIFlag() ? 1 : 0, "sub_pic_cpb_params_in_pic_timing_sei_flag");
                WRITE_CODE(hrd->getDpbOutputDelayDuLengthMinus1(), 5,   "dpb_output_delay_du_length_minus1");
            }
            WRITE_CODE(hrd->getBitRateScale(), 4,                     "bit_rate_scale");
            WRITE_CODE(hrd->getCpbSizeScale(), 4,                     "cpb_size_scale");
            if (hrd->getSubPicHrdParamsPresentFlag())
            {
                WRITE_CODE(hrd->getDuCpbSizeScale(), 4,                "du_cpb_size_scale");
            }
            WRITE_CODE(hrd->getInitialCpbRemovalDelayLengthMinus1(), 5, "initial_cpb_removal_delay_length_minus1");
            WRITE_CODE(hrd->getCpbRemovalDelayLengthMinus1(),        5, "au_cpb_removal_delay_length_minus1");
            WRITE_CODE(hrd->getDpbOutputDelayLengthMinus1(),         5, "dpb_output_delay_length_minus1");
        }
    }
    int nalOrVcl;
    for (uint32_t i = 0; i <= maxNumSubLayersMinus1; i++)
    {
        WRITE_FLAG(hrd->getFixedPicRateFlag(i) ? 1 : 0,          "fixed_pic_rate_general_flag");
        if (!hrd->getFixedPicRateFlag(i))
        {
            WRITE_FLAG(hrd->getFixedPicRateWithinCvsFlag(i) ? 1 : 0, "fixed_pic_rate_within_cvs_flag");
        }
        else
        {
            hrd->setFixedPicRateWithinCvsFlag(i, true);
        }
        if (hrd->getFixedPicRateWithinCvsFlag(i))
        {
            WRITE_UVLC(hrd->getPicDurationInTcMinus1(i), "elemental_duration_in_tc_minus1");
        }
        else
        {
            WRITE_FLAG(hrd->getLowDelayHrdFlag(i) ? 1 : 0, "low_delay_hrd_flag");
        }
        if (!hrd->getLowDelayHrdFlag(i))
        {
            WRITE_UVLC(hrd->getCpbCntMinus1(i), "cpb_cnt_minus1");
        }

        for (nalOrVcl = 0; nalOrVcl < 2; nalOrVcl++)
        {
            if (((nalOrVcl == 0) && (hrd->getNalHrdParametersPresentFlag())) ||
                ((nalOrVcl == 1) && (hrd->getVclHrdParametersPresentFlag())))
            {
                for (uint32_t j = 0; j <= (hrd->getCpbCntMinus1(i)); j++)
                {
                    WRITE_UVLC(hrd->getBitRateValueMinus1(i, j, nalOrVcl), "bit_rate_value_minus1");
                    WRITE_UVLC(hrd->getCpbSizeValueMinus1(i, j, nalOrVcl), "cpb_size_value_minus1");
                    if (hrd->getSubPicHrdParamsPresentFlag())
                    {
                        WRITE_UVLC(hrd->getDuCpbSizeValueMinus1(i, j, nalOrVcl), "cpb_size_du_value_minus1");
                        WRITE_UVLC(hrd->getDuBitRateValueMinus1(i, j, nalOrVcl), "bit_rate_du_value_minus1");
                    }
                    WRITE_FLAG(hrd->getCbrFlag(i, j, nalOrVcl) ? 1 : 0, "cbr_flag");
                }
            }
        }
    }
}

void SBac::codePTL(TComPTL* ptl, bool profilePresentFlag, int maxNumSubLayersMinus1)
{
    if (profilePresentFlag)
        codeProfileTier(ptl->getGeneralPTL()); // general_...

    WRITE_CODE(ptl->getGeneralPTL()->getLevelIdc(), 8, "general_level_idc");

    for (int i = 0; i < maxNumSubLayersMinus1; i++)
    {
        if (profilePresentFlag)
        {
            WRITE_FLAG(ptl->getSubLayerProfilePresentFlag(i), "sub_layer_profile_present_flag[i]");
        }

        WRITE_FLAG(ptl->getSubLayerLevelPresentFlag(i), "sub_layer_level_present_flag[i]");
    }

    if (maxNumSubLayersMinus1 > 0)
    {
        for (int i = maxNumSubLayersMinus1; i < 8; i++)
        {
            WRITE_CODE(0, 2, "reserved_zero_2bits");
        }
    }

    for (int i = 0; i < maxNumSubLayersMinus1; i++)
    {
        if (profilePresentFlag && ptl->getSubLayerProfilePresentFlag(i))
        {
            codeProfileTier(ptl->getSubLayerPTL(i)); // sub_layer_...
        }
        if (ptl->getSubLayerLevelPresentFlag(i))
        {
            WRITE_CODE(ptl->getSubLayerPTL(i)->getLevelIdc(), 8, "sub_layer_level_idc[i]");
        }
    }
}

void SBac::codeProfileTier(ProfileTierLevel* ptl)
{
    WRITE_CODE(ptl->getProfileSpace(), 2, "XXX_profile_space[]");
    WRITE_FLAG(ptl->getTierFlag(),        "XXX_tier_flag[]");
    WRITE_CODE(ptl->getProfileIdc(), 5,   "XXX_profile_idc[]");
    for (int j = 0; j < 32; j++)
    {
        WRITE_FLAG(ptl->getProfileCompatibilityFlag(j), "XXX_profile_compatibility_flag[][j]");
    }

    WRITE_FLAG(ptl->getProgressiveSourceFlag(),   "general_progressive_source_flag");
    WRITE_FLAG(ptl->getInterlacedSourceFlag(),    "general_interlaced_source_flag");
    WRITE_FLAG(ptl->getNonPackedConstraintFlag(), "general_non_packed_constraint_flag");
    WRITE_FLAG(ptl->getFrameOnlyConstraintFlag(), "general_frame_only_constraint_flag");

    WRITE_CODE(0, 16, "XXX_reserved_zero_44bits[0..15]");
    WRITE_CODE(0, 16, "XXX_reserved_zero_44bits[16..31]");
    WRITE_CODE(0, 12, "XXX_reserved_zero_44bits[32..43]");
}

/* code explicit wp tables */
void SBac::codePredWeightTable(TComSlice* slice)
{
    wpScalingParam  *wp;
    bool            bChroma      = true; // color always present in HEVC ?
    int             numRefDirs   = (slice->getSliceType() == B_SLICE) ? (2) : (1);
    bool            bDenomCoded  = false;
    uint32_t        mode = 0;
    uint32_t        totalSignalledWeightFlags = 0;

    if ((slice->getSliceType() == P_SLICE && slice->getPPS()->getUseWP()) || (slice->getSliceType() == B_SLICE && slice->getPPS()->getWPBiPred()))
    {
        mode = 1; // explicit
    }
    if (mode == 1)
    {
        for (int picList = 0; picList < numRefDirs; picList++)
        {
            for (int refIdx = 0; refIdx < slice->getNumRefIdx(picList); refIdx++)
            {
                slice->getWpScaling(picList, refIdx, wp);
                if (!bDenomCoded)
                {
                    int iDeltaDenom;
                    WRITE_UVLC(wp[0].log2WeightDenom, "luma_log2_weight_denom"); // ue(v): luma_log2_weight_denom

                    if (bChroma)
                    {
                        iDeltaDenom = (wp[1].log2WeightDenom - wp[0].log2WeightDenom);
                        WRITE_SVLC(iDeltaDenom, "delta_chroma_log2_weight_denom"); // se(v): delta_chroma_log2_weight_denom
                    }
                    bDenomCoded = true;
                }
                WRITE_FLAG(wp[0].bPresentFlag, "luma_weight_lX_flag");         // u(1): luma_weight_lX_flag
                totalSignalledWeightFlags += wp[0].bPresentFlag;
            }

            if (bChroma)
            {
                for (int refIdx = 0; refIdx < slice->getNumRefIdx(picList); refIdx++)
                {
                    slice->getWpScaling(picList, refIdx, wp);
                    WRITE_FLAG(wp[1].bPresentFlag, "chroma_weight_lX_flag");   // u(1): chroma_weight_lX_flag
                    totalSignalledWeightFlags += 2 * wp[1].bPresentFlag;
                }
            }

            for (int refIdx = 0; refIdx < slice->getNumRefIdx(picList); refIdx++)
            {
                slice->getWpScaling(picList, refIdx, wp);
                if (wp[0].bPresentFlag)
                {
                    int iDeltaWeight = (wp[0].inputWeight - (1 << wp[0].log2WeightDenom));
                    WRITE_SVLC(iDeltaWeight, "delta_luma_weight_lX");          // se(v): delta_luma_weight_lX
                    WRITE_SVLC(wp[0].inputOffset, "luma_offset_lX");           // se(v): luma_offset_lX
                }

                if (bChroma)
                {
                    if (wp[1].bPresentFlag)
                    {
                        for (int j = 1; j < 3; j++)
                        {
                            int iDeltaWeight = (wp[j].inputWeight - (1 << wp[1].log2WeightDenom));
                            WRITE_SVLC(iDeltaWeight, "delta_chroma_weight_lX"); // se(v): delta_chroma_weight_lX

                            int pred = (128 - ((128 * wp[j].inputWeight) >> (wp[j].log2WeightDenom)));
                            int iDeltaChroma = (wp[j].inputOffset - pred);
                            WRITE_SVLC(iDeltaChroma, "delta_chroma_offset_lX"); // se(v): delta_chroma_offset_lX
                        }
                    }
                }
            }
        }

        X265_CHECK(totalSignalledWeightFlags <= 24, "total weights must be <= 24\n");
    }
}

/** code quantization matrix */
void SBac::codeScalingList(TComScalingList* scalingList)
{
    uint32_t listId, sizeId;
    bool scalingListPredModeFlag;

    //for each size
    for (sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
    {
        for (listId = 0; listId < g_scalingListNum[sizeId]; listId++)
        {
            scalingListPredModeFlag = scalingList->checkPredMode(sizeId, listId);
            WRITE_FLAG(scalingListPredModeFlag, "scaling_list_pred_mode_flag");
            if (!scalingListPredModeFlag) // Copy Mode
                WRITE_UVLC((int)listId - (int)scalingList->getRefMatrixId(sizeId, listId), "scaling_list_pred_matrix_id_delta");
            else // DPCM Mode
                codeScalingList(scalingList, sizeId, listId);
        }
    }
}

void SBac::codeScalingList(TComScalingList* scalingList, uint32_t sizeId, uint32_t listId)
{
    int coefNum = X265_MIN(MAX_MATRIX_COEF_NUM, (int)g_scalingListSize[sizeId]);
    const uint16_t* scan = g_scanOrder[SCAN_UNGROUPED][SCAN_DIAG][sizeId == 0 ? 2 : 3];
    int nextCoef = SCALING_LIST_START_VALUE;
    int data;
    int32_t *src = scalingList->getScalingListAddress(sizeId, listId);

    if (sizeId > SCALING_LIST_8x8)
    {
        WRITE_SVLC(scalingList->getScalingListDC(sizeId, listId) - 8, "scaling_list_dc_coef_minus8");
        nextCoef = scalingList->getScalingListDC(sizeId, listId);
    }
    for (int i = 0; i < coefNum; i++)
    {
        data = src[scan[i]] - nextCoef;
        nextCoef = src[scan[i]];
        if (data > 127)
        {
            data = data - 256;
        }
        if (data < -128)
        {
            data = data + 256;
        }

        WRITE_SVLC(data,  "scaling_list_delta_coef");
    }
}

bool SBac::findMatchingLTRP(TComSlice* slice, uint32_t *ltrpsIndex, int ltrpPOC, bool usedFlag)
{
    // bool state = true, state2 = false;
    uint32_t lsb = ltrpPOC % (1 << slice->getSPS()->getBitsForPOC());

    for (uint32_t k = 0; k < slice->getSPS()->getNumLongTermRefPicSPS(); k++)
    {
        if ((lsb == slice->getSPS()->getLtRefPicPocLsbSps(k)) && (usedFlag == slice->getSPS()->getUsedByCurrPicLtSPSFlag(k)))
        {
            *ltrpsIndex = k;
            return true;
        }
    }

    return false;
}

bool TComScalingList::checkPredMode(uint32_t sizeId, int listId)
{
    for (int predListIdx = listId; predListIdx >= 0; predListIdx--)
    {
        if (!memcmp(getScalingListAddress(sizeId, listId),
                    ((listId == predListIdx) ? getScalingListDefaultAddress(sizeId, predListIdx) : getScalingListAddress(sizeId, predListIdx)),
                    sizeof(int) * X265_MIN(MAX_MATRIX_COEF_NUM, (int)g_scalingListSize[sizeId])) // check value of matrix
            && ((sizeId < SCALING_LIST_16x16) || (getScalingListDC(sizeId, listId) == getScalingListDC(sizeId, predListIdx)))) // check DC value
        {
            setRefMatrixId(sizeId, listId, predListIdx);
            return false;
        }
    }

    return true;
}

void SBac::codeSliceHeader(TComSlice* slice)
{
    //calculate number of bits required for slice address
    int maxSliceSegmentAddress = slice->getPic()->getNumCUsInFrame();
    int bitsSliceSegmentAddress = 0;
    while (maxSliceSegmentAddress > (1 << bitsSliceSegmentAddress))
    {
        bitsSliceSegmentAddress++;
    }

    WRITE_FLAG(1, "first_slice_segment_in_pic_flag");
    if (slice->getRapPicFlag())
    {
        WRITE_FLAG(0, "no_output_of_prior_pics_flag");
    }
    WRITE_UVLC(slice->getPPS()->getPPSId(), "slice_pic_parameter_set_id");

    /* x265 does not use dependent slices, so always write all this data */
    for (int i = 0; i < slice->getPPS()->getNumExtraSliceHeaderBits(); i++)
    {
        X265_CHECK(0, "slice_reserved_undetermined_flag[]");
        WRITE_FLAG(0, "slice_reserved_undetermined_flag[]");
    }

    WRITE_UVLC(slice->getSliceType(),       "slice_type");

    if (slice->getPPS()->getOutputFlagPresentFlag())
    {
        WRITE_FLAG(slice->getPicOutputFlag() ? 1 : 0, "pic_output_flag");
    }
    if (slice->getSPS()->getChromaFormatIdc() == CHROMA_444)
    {
        //In this version separate_color_plane_flag is 0
        if (slice->getSPS()->getSeparateColorPlaneFlag())
        {
            //plane_id values 0, 1, and 2 correspond to the Y, Cb, and Cr planes, respectively.
            //WRITE_FLAG(0, "colour_plane_id");
        }
    }
    if (!slice->getIdrPicFlag())
    {
        int picOrderCntLSB = (slice->getPOC() - slice->getLastIDR() + (1 << slice->getSPS()->getBitsForPOC())) % (1 << slice->getSPS()->getBitsForPOC());
        WRITE_CODE(picOrderCntLSB, slice->getSPS()->getBitsForPOC(), "pic_order_cnt_lsb");
        TComReferencePictureSet* rps = slice->getRPS();

#if _DEBUG || CHECKED_BUILD
        // check for bitstream restriction stating that:
        // If the current picture is a BLA or CRA picture, the value of NumPocTotalCurr shall be equal to 0.
        // Ideally this process should not be repeated for each slice in a picture
        if (slice->isIRAP())
            for (int picIdx = 0; picIdx < rps->getNumberOfPictures(); picIdx++)
                X265_CHECK(!rps->getUsed(picIdx), "pic unused failure\n");
#endif

        WRITE_FLAG(0, "short_term_ref_pic_set_sps_flag");
        codeShortTermRefPicSet(rps);

        if (slice->getSPS()->getLongTermRefsPresent())
        {
            int numLtrpInSH = rps->getNumberOfLongtermPictures();
            int ltrpInSPS[MAX_NUM_REF_PICS];
            int numLtrpInSPS = 0;
            uint32_t ltrpIndex;
            int counter = 0;
            for (int k = rps->getNumberOfPictures() - 1; k > rps->getNumberOfPictures() - rps->getNumberOfLongtermPictures() - 1; k--)
            {
                if (findMatchingLTRP(slice, &ltrpIndex, rps->getPOC(k), rps->getUsed(k)))
                {
                    ltrpInSPS[numLtrpInSPS] = ltrpIndex;
                    numLtrpInSPS++;
                }
                else
                {
                    counter++;
                }
            }

            numLtrpInSH -= numLtrpInSPS;

            int bitsForLtrpInSPS = 0;
            while (slice->getSPS()->getNumLongTermRefPicSPS() > (uint32_t)(1 << bitsForLtrpInSPS))
            {
                bitsForLtrpInSPS++;
            }

            if (slice->getSPS()->getNumLongTermRefPicSPS() > 0)
            {
                WRITE_UVLC(numLtrpInSPS, "num_long_term_sps");
            }
            WRITE_UVLC(numLtrpInSH, "num_long_term_pics");
            // Note that the LSBs of the LT ref. pic. POCs must be sorted before.
            // Not sorted here because LT ref indices will be used in setRefPicList()
            int prevDeltaMSB = 0, prevLSB = 0;
            int offset = rps->getNumberOfNegativePictures() + rps->getNumberOfPositivePictures();
            for (int i = rps->getNumberOfPictures() - 1; i > offset - 1; i--)
            {
                if (counter < numLtrpInSPS)
                {
                    if (bitsForLtrpInSPS > 0)
                    {
                        WRITE_CODE(ltrpInSPS[counter], bitsForLtrpInSPS, "lt_idx_sps[i]");
                    }
                }
                else
                {
                    WRITE_CODE(rps->getPocLSBLT(i), slice->getSPS()->getBitsForPOC(), "poc_lsb_lt");
                    WRITE_FLAG(rps->getUsed(i), "used_by_curr_pic_lt_flag");
                }
                WRITE_FLAG(rps->getDeltaPocMSBPresentFlag(i), "delta_poc_msb_present_flag");

                if (rps->getDeltaPocMSBPresentFlag(i))
                {
                    bool deltaFlag = false;
                    //  First LTRP from SPS                 ||  First LTRP from SH                              || curr LSB            != prev LSB
                    if ((i == rps->getNumberOfPictures() - 1) || (i == rps->getNumberOfPictures() - 1 - numLtrpInSPS) || (rps->getPocLSBLT(i) != prevLSB))
                    {
                        deltaFlag = true;
                    }
                    if (deltaFlag)
                    {
                        WRITE_UVLC(rps->getDeltaPocMSBCycleLT(i), "delta_poc_msb_cycle_lt[i]");
                    }
                    else
                    {
                        int differenceInDeltaMSB = rps->getDeltaPocMSBCycleLT(i) - prevDeltaMSB;
                        X265_CHECK(differenceInDeltaMSB >= 0, "delta MSB must be positive\n");
                        WRITE_UVLC(differenceInDeltaMSB, "delta_poc_msb_cycle_lt[i]");
                    }
                    prevLSB = rps->getPocLSBLT(i);
                    prevDeltaMSB = rps->getDeltaPocMSBCycleLT(i);
                }
            }
        }
        if (slice->getSPS()->getTMVPFlagsPresent())
        {
            WRITE_FLAG(slice->getEnableTMVPFlag() ? 1 : 0, "slice_temporal_mvp_enable_flag");
        }
    }
    if (slice->getSPS()->getUseSAO())
    {
        if (slice->getSPS()->getUseSAO())
        {
            WRITE_FLAG(slice->getSaoEnabledFlag(), "slice_sao_luma_flag");
            {
                SAOParam *saoParam = slice->getPic()->getPicSym()->getSaoParam();
                WRITE_FLAG(saoParam->bSaoFlag[1], "slice_sao_chroma_flag");
            }
        }
    }

    // check if numrefidxes match the defaults. If not, override

    if (!slice->isIntra())
    {
        bool overrideFlag = (slice->getNumRefIdx(REF_PIC_LIST_0) != (int)slice->getPPS()->getNumRefIdxL0DefaultActive() ||
                            (slice->isInterB() && slice->getNumRefIdx(REF_PIC_LIST_1) != (int)slice->getPPS()->getNumRefIdxL1DefaultActive()));
        WRITE_FLAG(overrideFlag ? 1 : 0,                            "num_ref_idx_active_override_flag");
        if (overrideFlag)
        {
            WRITE_UVLC(slice->getNumRefIdx(REF_PIC_LIST_0) - 1,     "num_ref_idx_l0_active_minus1");
            if (slice->isInterB())
            {
                WRITE_UVLC(slice->getNumRefIdx(REF_PIC_LIST_1) - 1, "num_ref_idx_l1_active_minus1");
            }
            else
            {
                slice->setNumRefIdx(REF_PIC_LIST_1, 0);
            }
        }
    }
    else
    {
        slice->setNumRefIdx(REF_PIC_LIST_0, 0);
        slice->setNumRefIdx(REF_PIC_LIST_1, 0);
    }

    if (slice->isInterB())
    {
        WRITE_FLAG(slice->getMvdL1ZeroFlag() ? 1 : 0,   "mvd_l1_zero_flag");
    }

    if (!slice->isIntra())
    {
        if (!slice->isIntra() && slice->getPPS()->getCabacInitPresentFlag())
        {
            SliceType sliceType   = slice->getSliceType();
            int  encCABACTableIdx = slice->getPPS()->getEncCABACTableIdx();
            bool encCabacInitFlag = (sliceType != encCABACTableIdx && encCABACTableIdx != I_SLICE) ? true : false;
            slice->setCabacInitFlag(encCabacInitFlag);
            WRITE_FLAG(encCabacInitFlag ? 1 : 0, "cabac_init_flag");
        }
    }

    if (slice->getEnableTMVPFlag())
    {
        if (slice->getSliceType() == B_SLICE)
        {
            WRITE_FLAG(slice->getColFromL0Flag(), "collocated_from_l0_flag");
        }

        if (slice->getSliceType() != I_SLICE &&
            ((slice->getColFromL0Flag() == 1 && slice->getNumRefIdx(REF_PIC_LIST_0) > 1) ||
                (slice->getColFromL0Flag() == 0 && slice->getNumRefIdx(REF_PIC_LIST_1) > 1)))
        {
            WRITE_UVLC(slice->getColRefIdx(), "collocated_ref_idx");
        }
    }
    if ((slice->getPPS()->getUseWP() && slice->getSliceType() == P_SLICE) || (slice->getPPS()->getWPBiPred() && slice->getSliceType() == B_SLICE))
    {
        codePredWeightTable(slice);
    }
    X265_CHECK(slice->getMaxNumMergeCand() <= MRG_MAX_NUM_CANDS, "too many merge candidates\n");
    if (!slice->isIntra())
    {
        WRITE_UVLC(MRG_MAX_NUM_CANDS - slice->getMaxNumMergeCand(), "five_minus_max_num_merge_cand");
    }
    int code = slice->getSliceQp() - (slice->getPPS()->getPicInitQPMinus26() + 26);
    WRITE_SVLC(code, "slice_qp_delta");
    if (slice->getPPS()->getSliceChromaQpFlag())
    {
        code = slice->getSliceQpDeltaCb();
        WRITE_SVLC(code, "slice_qp_delta_cb");
        code = slice->getSliceQpDeltaCr();
        WRITE_SVLC(code, "slice_qp_delta_cr");
    }
    if (slice->getPPS()->getDeblockingFilterControlPresentFlag())
    {
        if (slice->getPPS()->getDeblockingFilterOverrideEnabledFlag())
        {
            WRITE_FLAG(slice->getDeblockingFilterOverrideFlag(), "deblocking_filter_override_flag");
        }
        if (slice->getDeblockingFilterOverrideFlag())
        {
            WRITE_FLAG(slice->getDeblockingFilterDisable(), "slice_disable_deblocking_filter_flag");
            if (!slice->getDeblockingFilterDisable())
            {
                WRITE_SVLC(slice->getDeblockingFilterBetaOffsetDiv2(), "slice_beta_offset_div2");
                WRITE_SVLC(slice->getDeblockingFilterTcOffsetDiv2(),   "slice_tc_offset_div2");
            }
        }
    }

    bool isSAOEnabled = (!slice->getSPS()->getUseSAO()) ? (false) : (slice->getSaoEnabledFlag() || slice->getSaoEnabledFlagChroma());
    bool isDBFEnabled = (!slice->getDeblockingFilterDisable());

    if (isSAOEnabled || isDBFEnabled)
    {
        WRITE_FLAG(1, "slice_loop_filter_across_slices_enabled_flag");
    }

    if (slice->getPPS()->getSliceHeaderExtensionPresentFlag())
    {
        WRITE_UVLC(0, "slice_header_extension_length");
    }
}

/** write wavefront substreams sizes for the slice header */
void  SBac::codeTilesWPPEntryPoint(TComSlice* slice)
{
    if (!slice->getPPS()->getEntropyCodingSyncEnabledFlag())
    {
        return;
    }
    uint32_t numEntryPointOffsets = 0, offsetLenMinus1 = 0, maxOffset = 0;
    uint32_t *entryPointOffset = NULL;
    if (slice->getPPS()->getEntropyCodingSyncEnabledFlag())
    {
        uint32_t* substreamSizes          = slice->getSubstreamSizes();
        int maxNumParts                   = slice->getPic()->getNumPartInCU();
        int numZeroSubstreamsAtEndOfSlice = slice->getPic()->getFrameHeightInCU() - 1 - ((slice->getSliceCurEndCUAddr() - 1) / maxNumParts / slice->getPic()->getFrameWidthInCU());
        numEntryPointOffsets              = slice->getPic()->getFrameHeightInCU() - numZeroSubstreamsAtEndOfSlice - 1;
        slice->setNumEntryPointOffsets(numEntryPointOffsets);
        entryPointOffset = new uint32_t[numEntryPointOffsets];
        for (uint32_t idx = 0; idx < numEntryPointOffsets; idx++)
        {
            entryPointOffset[idx] = (substreamSizes[idx] >> 3);
            if (entryPointOffset[idx] > maxOffset)
            {
                maxOffset = entryPointOffset[idx];
            }
        }
    }
    // Determine number of bits "offsetLenMinus1+1" required for entry point information
    offsetLenMinus1 = 0;
    while (maxOffset >= (1u << (offsetLenMinus1 + 1)))
    {
        offsetLenMinus1++;
        X265_CHECK(offsetLenMinus1 + 1 < 32, "offsetLenMinus1 is too large\n");
    }

    WRITE_UVLC(numEntryPointOffsets, "num_entry_point_offsets");
    if (numEntryPointOffsets > 0)
    {
        WRITE_UVLC(offsetLenMinus1, "offset_len_minus1");
    }

    for (uint32_t idx = 0; idx < numEntryPointOffsets; idx++)
    {
        WRITE_CODE(entryPointOffset[idx] - 1, offsetLenMinus1 + 1, "entry_point_offset_minus1");
    }

    delete [] entryPointOffset;
}

void SBac::codeTerminatingBit(uint32_t lsLast)
{
    encodeBinTrm(lsLast);
}

void SBac::codeSliceFinish()
{
    finish();
}

void SBac::writeUnaryMaxSymbol(uint32_t symbol, ContextModel* scmModel, int offset, uint32_t maxSymbol)
{
    X265_CHECK(maxSymbol > 0, "maxSymbol too small\n");

    encodeBin(symbol ? 1 : 0, scmModel[0]);

    if (symbol == 0)
        return;

    bool bCodeLast = (maxSymbol > symbol);

    while (--symbol)
        encodeBin(1, scmModel[offset]);

    if (bCodeLast)
        encodeBin(0, scmModel[offset]);
}

void SBac::writeEpExGolomb(uint32_t symbol, uint32_t count)
{
    uint32_t bins = 0;
    int numBins = 0;

    while (symbol >= (uint32_t)(1 << count))
    {
        bins = 2 * bins + 1;
        numBins++;
        symbol -= 1 << count;
        count++;
    }

    bins = 2 * bins + 0;
    numBins++;

    bins = (bins << count) | symbol;
    numBins += count;

    X265_CHECK(numBins <= 32, "numBins too large\n");
    encodeBinsEP(bins, numBins);
}

/** Coding of coeff_abs_level_minus3 */
void SBac::writeCoefRemainExGolomb(uint32_t codeNumber, uint32_t absGoRice)
{
    uint32_t length;
    const uint32_t codeRemain = codeNumber & ((1 << absGoRice) - 1);

    if ((codeNumber >> absGoRice) < COEF_REMAIN_BIN_REDUCTION)
    {
        length = codeNumber >> absGoRice;

        X265_CHECK(codeNumber - (length << absGoRice) == (codeNumber & ((1 << absGoRice) - 1)), "codeNumber failure\n");
        X265_CHECK(length + 1 + absGoRice < 32, "length failure\n");
        encodeBinsEP((((1 << (length + 1)) - 2) << absGoRice) + codeRemain, length + 1 + absGoRice);
    }
    else
    {
        length = 0;
        codeNumber = (codeNumber >> absGoRice) - COEF_REMAIN_BIN_REDUCTION;
        if (codeNumber != 0)
        {
            unsigned long idx;
            CLZ32(idx, codeNumber + 1);
            length = idx;
            codeNumber -= (1 << idx) - 1;
        }
        codeNumber = (codeNumber << absGoRice) + codeRemain;

        encodeBinsEP((1 << (COEF_REMAIN_BIN_REDUCTION + length + 1)) - 2, COEF_REMAIN_BIN_REDUCTION + length + 1);
        encodeBinsEP(codeNumber, length + absGoRice);
    }
}

// SBAC RD
void  SBac::load(SBac& src)
{
    this->copyFrom(src);
}

void  SBac::loadIntraDirModeLuma(SBac& src)
{
    copyState(src);

    ::memcpy(&m_contextModels[OFF_ADI_CTX], &src.m_contextModels[OFF_ADI_CTX], sizeof(ContextModel) * NUM_ADI_CTX);
}

void  SBac::store(SBac& dest)
{
    dest.copyFrom(*this);
}

void SBac::copyFrom(SBac& src)
{
    copyState(src);

    memcpy(m_contextModels, src.m_contextModels, MAX_OFF_CTX_MOD * sizeof(ContextModel));
}

void SBac::codeMVPIdx(uint32_t symbol)
{
    encodeBin(symbol, m_contextModels[OFF_MVP_IDX_CTX]);
}

void SBac::codePartSize(TComDataCU* cu, uint32_t absPartIdx, uint32_t depth)
{
    PartSize partSize = cu->getPartitionSize(absPartIdx);

    if (cu->isIntra(absPartIdx))
    {
        if (depth == g_maxCUDepth - g_addCUDepth)
            encodeBin(partSize == SIZE_2Nx2N ? 1 : 0, m_contextModels[OFF_PART_SIZE_CTX]);
        return;
    }

    switch (partSize)
    {
    case SIZE_2Nx2N:
        encodeBin(1, m_contextModels[OFF_PART_SIZE_CTX]);
        break;

    case SIZE_2NxN:
    case SIZE_2NxnU:
    case SIZE_2NxnD:
        encodeBin(0, m_contextModels[OFF_PART_SIZE_CTX + 0]);
        encodeBin(1, m_contextModels[OFF_PART_SIZE_CTX + 1]);
        if (cu->getSlice()->getSPS()->getAMPAcc(depth))
        {
            encodeBin((partSize == SIZE_2NxN) ? 1 : 0, m_contextModels[OFF_PART_SIZE_CTX + 3]);
            if (partSize != SIZE_2NxN)
                encodeBinEP((partSize == SIZE_2NxnU ? 0 : 1));
        }
        break;

    case SIZE_Nx2N:
    case SIZE_nLx2N:
    case SIZE_nRx2N:
        encodeBin(0, m_contextModels[OFF_PART_SIZE_CTX + 0]);
        encodeBin(0, m_contextModels[OFF_PART_SIZE_CTX + 1]);
        if (depth == g_maxCUDepth - g_addCUDepth && !(cu->getLog2CUSize(absPartIdx) == 3))
            encodeBin(1, m_contextModels[OFF_PART_SIZE_CTX + 2]);
        if (cu->getSlice()->getSPS()->getAMPAcc(depth))
        {
            encodeBin((partSize == SIZE_Nx2N) ? 1 : 0, m_contextModels[OFF_PART_SIZE_CTX + 3]);
            if (partSize != SIZE_Nx2N)
                encodeBinEP((partSize == SIZE_nLx2N ? 0 : 1));
        }
        break;

    case SIZE_NxN:
        if (depth == g_maxCUDepth - g_addCUDepth && !(cu->getLog2CUSize(absPartIdx) == 3))
        {
            encodeBin(0, m_contextModels[OFF_PART_SIZE_CTX + 0]);
            encodeBin(0, m_contextModels[OFF_PART_SIZE_CTX + 1]);
            encodeBin(0, m_contextModels[OFF_PART_SIZE_CTX + 2]);
        }
        break;

    default:
        X265_CHECK(0, "invalid CU partition\n");
        break;
    }
}

void SBac::codePredMode(TComDataCU* cu, uint32_t absPartIdx)
{
    // get context function is here
    int predMode = cu->getPredictionMode(absPartIdx);
    encodeBin(predMode == MODE_INTER ? 0 : 1, m_contextModels[OFF_PRED_MODE_CTX]);
}

void SBac::codeCUTransquantBypassFlag(TComDataCU* cu, uint32_t absPartIdx)
{
    uint32_t symbol = cu->getCUTransquantBypass(absPartIdx);
    encodeBin(symbol, m_contextModels[OFF_CU_TRANSQUANT_BYPASS_FLAG_CTX]);
}

void SBac::codeSkipFlag(TComDataCU* cu, uint32_t absPartIdx)
{
    // get context function is here
    uint32_t symbol = cu->isSkipped(absPartIdx) ? 1 : 0;
    uint32_t ctxSkip = cu->getCtxSkipFlag(absPartIdx);

    encodeBin(symbol, m_contextModels[OFF_SKIP_FLAG_CTX + ctxSkip]);
}

void SBac::codeMergeFlag(TComDataCU* cu, uint32_t absPartIdx)
{
    const uint32_t symbol = cu->getMergeFlag(absPartIdx) ? 1 : 0;

    encodeBin(symbol, m_contextModels[OFF_MERGE_FLAG_EXT_CTX]);
}

void SBac::codeMergeIndex(TComDataCU* cu, uint32_t absPartIdx)
{
    uint32_t numCand = cu->getSlice()->getMaxNumMergeCand();

    if (numCand > 1)
    {
        uint32_t unaryIdx = cu->getMergeIndex(absPartIdx);
        encodeBin((unaryIdx != 0), m_contextModels[OFF_MERGE_IDX_EXT_CTX]);

        X265_CHECK(unaryIdx < numCand, "unaryIdx out of range\n");

        if (unaryIdx != 0)
        {
            uint32_t mask = (1 << unaryIdx) - 2;
            mask >>= (unaryIdx == numCand - 1) ? 1 : 0;
            encodeBinsEP(mask, unaryIdx - (unaryIdx == numCand - 1));
        }
    }
}

void SBac::codeSplitFlag(TComDataCU* cu, uint32_t absPartIdx, uint32_t depth)
{
    if (depth == g_maxCUDepth - g_addCUDepth)
        return;

    uint32_t ctx           = cu->getCtxSplitFlag(absPartIdx, depth);
    uint32_t currSplitFlag = (cu->getDepth(absPartIdx) > depth) ? 1 : 0;

    X265_CHECK(ctx < 3, "ctx out of range\n");
    encodeBin(currSplitFlag, m_contextModels[OFF_SPLIT_FLAG_CTX + ctx]);
}

void SBac::codeTransformSubdivFlag(uint32_t symbol, uint32_t ctx)
{
    encodeBin(symbol, m_contextModels[OFF_TRANS_SUBDIV_FLAG_CTX + ctx]);
}

void SBac::codeIntraDirLumaAng(TComDataCU* cu, uint32_t absPartIdx, bool isMultiple)
{
    uint32_t dir[4], j;
    uint32_t preds[4][3];
    int predIdx[4];
    PartSize mode = cu->getPartitionSize(absPartIdx);
    uint32_t partNum = isMultiple ? (mode == SIZE_NxN ? 4 : 1) : 1;
    uint32_t partOffset = (cu->getPic()->getNumPartInCU() >> (cu->getDepth(absPartIdx) << 1)) >> 2;

    for (j = 0; j < partNum; j++)
    {
        dir[j] = cu->getLumaIntraDir(absPartIdx + partOffset * j);
        cu->getIntraDirLumaPredictor(absPartIdx + partOffset * j, preds[j]);
        predIdx[j] = -1;
        for (uint32_t i = 0; i < 3; i++)
        {
            if (dir[j] == preds[j][i])
            {
                predIdx[j] = i;
            }
        }

        encodeBin((predIdx[j] != -1) ? 1 : 0, m_contextModels[OFF_ADI_CTX]);
    }

    for (j = 0; j < partNum; j++)
    {
        if (predIdx[j] != -1)
        {
            X265_CHECK((predIdx[j] >= 0) && (predIdx[j] <= 2), "predIdx out of range\n");
            // NOTE: Mapping
            //       0 = 0
            //       1 = 10
            //       2 = 11
            int nonzero = (!!predIdx[j]);
            encodeBinsEP(predIdx[j] + nonzero, 1 + nonzero);
        }
        else
        {
            if (preds[j][0] > preds[j][1])
            {
                std::swap(preds[j][0], preds[j][1]);
            }
            if (preds[j][0] > preds[j][2])
            {
                std::swap(preds[j][0], preds[j][2]);
            }
            if (preds[j][1] > preds[j][2])
            {
                std::swap(preds[j][1], preds[j][2]);
            }
            dir[j] += (dir[j] > preds[j][2]) ? -1 : 0;
            dir[j] += (dir[j] > preds[j][1]) ? -1 : 0;
            dir[j] += (dir[j] > preds[j][0]) ? -1 : 0;

            encodeBinsEP(dir[j], 5);
        }
    }
}

void SBac::codeIntraDirChroma(TComDataCU* cu, uint32_t absPartIdx)
{
    uint32_t intraDirChroma = cu->getChromaIntraDir(absPartIdx);

    if (intraDirChroma == DM_CHROMA_IDX)
        encodeBin(0, m_contextModels[OFF_CHROMA_PRED_CTX]);
    else
    {
        uint32_t allowedChromaDir[NUM_CHROMA_MODE];
        cu->getAllowedChromaDir(absPartIdx, allowedChromaDir);

        for (int i = 0; i < NUM_CHROMA_MODE - 1; i++)
        {
            if (intraDirChroma == allowedChromaDir[i])
            {
                intraDirChroma = i;
                break;
            }
        }

        encodeBin(1, m_contextModels[OFF_CHROMA_PRED_CTX]);
        encodeBinsEP(intraDirChroma, 2);
    }
}

void SBac::codeInterDir(TComDataCU* cu, uint32_t absPartIdx)
{
    const uint32_t interDir = cu->getInterDir(absPartIdx) - 1;
    const uint32_t ctx      = cu->getCtxInterDir(absPartIdx);

    if (cu->getPartitionSize(absPartIdx) == SIZE_2Nx2N || cu->getLog2CUSize(absPartIdx) != 3)
        encodeBin(interDir == 2 ? 1 : 0, m_contextModels[OFF_INTER_DIR_CTX + ctx]);
    if (interDir < 2)
        encodeBin(interDir, m_contextModels[OFF_INTER_DIR_CTX + 4]);
}

void SBac::codeRefFrmIdx(TComDataCU* cu, uint32_t absPartIdx, int list)
{
    uint32_t refFrame = cu->getCUMvField(list)->getRefIdx(absPartIdx);

    encodeBin(refFrame > 0, m_contextModels[OFF_REF_NO_CTX]);

    if (refFrame > 0)
    {
        uint32_t refNum = cu->getSlice()->getNumRefIdx(list) - 2;
        if (refNum == 0)
            return;

        refFrame--;
        encodeBin(refFrame > 0, m_contextModels[OFF_REF_NO_CTX + 1]);
        if (refFrame > 0)
        {
            uint32_t mask = (1 << refFrame) - 2;
            mask >>= (refFrame == refNum) ? 1 : 0;
            encodeBinsEP(mask, refFrame - (refFrame == refNum));
        }
    }
}

void SBac::codeMvd(TComDataCU* cu, uint32_t absPartIdx, int list)
{
    if (list == REF_PIC_LIST_1 && cu->getSlice()->getMvdL1ZeroFlag() && cu->getInterDir(absPartIdx) == 3)
        return;

    const TComCUMvField* cuMvField = cu->getCUMvField(list);
    const int hor = cuMvField->getMvd(absPartIdx).x;
    const int ver = cuMvField->getMvd(absPartIdx).y;

    encodeBin(hor != 0 ? 1 : 0, m_contextModels[OFF_MV_RES_CTX]);
    encodeBin(ver != 0 ? 1 : 0, m_contextModels[OFF_MV_RES_CTX]);

    const bool bHorAbsGr0 = hor != 0;
    const bool bVerAbsGr0 = ver != 0;
    const uint32_t horAbs   = 0 > hor ? -hor : hor;
    const uint32_t verAbs   = 0 > ver ? -ver : ver;

    if (bHorAbsGr0)
        encodeBin(horAbs > 1 ? 1 : 0, m_contextModels[OFF_MV_RES_CTX + 1]);

    if (bVerAbsGr0)
        encodeBin(verAbs > 1 ? 1 : 0, m_contextModels[OFF_MV_RES_CTX + 1]);

    if (bHorAbsGr0)
    {
        if (horAbs > 1)
            writeEpExGolomb(horAbs - 2, 1);

        encodeBinEP(0 > hor ? 1 : 0);
    }

    if (bVerAbsGr0)
    {
        if (verAbs > 1)
            writeEpExGolomb(verAbs - 2, 1);

        encodeBinEP(0 > ver ? 1 : 0);
    }
}

void SBac::codeDeltaQP(TComDataCU* cu, uint32_t absPartIdx)
{
    int dqp = cu->getQP(absPartIdx) - cu->getRefQP(absPartIdx);

    int qpBdOffsetY = QP_BD_OFFSET;

    dqp = (dqp + 78 + qpBdOffsetY + (qpBdOffsetY / 2)) % (52 + qpBdOffsetY) - 26 - (qpBdOffsetY / 2);

    uint32_t absDQp = (uint32_t)((dqp > 0) ? dqp  : (-dqp));
    uint32_t TUValue = X265_MIN((int)absDQp, CU_DQP_TU_CMAX);
    writeUnaryMaxSymbol(TUValue, &m_contextModels[OFF_DELTA_QP_CTX], 1, CU_DQP_TU_CMAX);
    if (absDQp >= CU_DQP_TU_CMAX)
        writeEpExGolomb(absDQp - CU_DQP_TU_CMAX, CU_DQP_EG_k);
   

    if (absDQp > 0)
    {
        uint32_t sign = (dqp > 0 ? 0 : 1);
        encodeBinEP(sign);
    }
}

void SBac::codeQtCbf(TComDataCU* cu, uint32_t absPartIdx, uint32_t absPartIdxStep, uint32_t width, uint32_t height, TextType ttype, uint32_t trDepth, bool lowestLevel)
{
    uint32_t ctx = cu->getCtxQtCbf(ttype, trDepth);

    bool canQuadSplit       = (width >= (MIN_TU_SIZE * 2)) && (height >= (MIN_TU_SIZE * 2));
    uint32_t lowestTUDepth  = trDepth + ((!lowestLevel && !canQuadSplit) ? 1 : 0); // unsplittable TUs inherit their parent's CBF

    if ((width != height) && (lowestLevel || !canQuadSplit)) // if sub-TUs are present
    {
        uint32_t subTUDepth        = lowestTUDepth + 1;   // if this is the lowest level of the TU-tree, the sub-TUs are directly below.
                                                          // Otherwise, this must be the level above the lowest level (as specified above)
        uint32_t partIdxesPerSubTU = absPartIdxStep >> 1;

        for (uint32_t subTU = 0; subTU < 2; subTU++)
        {
            uint32_t subTUAbsPartIdx = absPartIdx + (subTU * partIdxesPerSubTU);
            uint32_t cbf = cu->getCbf(subTUAbsPartIdx, ttype, subTUDepth);

            encodeBin(cbf, m_contextModels[OFF_QT_CBF_CTX + ctx]);
        }
    }
    else
    {
        uint32_t cbf = cu->getCbf(absPartIdx, ttype, lowestTUDepth);

        encodeBin(cbf, m_contextModels[OFF_QT_CBF_CTX + ctx]);
    }
}

void SBac::codeQtCbf(TComDataCU* cu, uint32_t absPartIdx, TextType ttype, uint32_t trDepth)
{
    uint32_t ctx = cu->getCtxQtCbf(ttype, trDepth);
    uint32_t cbf = cu->getCbf(absPartIdx, ttype, trDepth);
    encodeBin(cbf, m_contextModels[OFF_QT_CBF_CTX + ctx]);
}

void SBac::codeTransformSkipFlags(TComDataCU* cu, uint32_t absPartIdx, uint32_t trSize, TextType ttype)
{
    if (cu->getCUTransquantBypass(absPartIdx))
        return;
    if (trSize != 4)
        return;

    uint32_t useTransformSkip = cu->getTransformSkip(absPartIdx, ttype);
    encodeBin(useTransformSkip, m_contextModels[OFF_TRANSFORMSKIP_FLAG_CTX + (ttype ? NUM_TRANSFORMSKIP_FLAG_CTX : 0)]);
}

void SBac::codeQtRootCbf(TComDataCU* cu, uint32_t absPartIdx)
{
    uint32_t cbf = cu->getQtRootCbf(absPartIdx);
    uint32_t ctx = 0;

    encodeBin(cbf, m_contextModels[OFF_QT_ROOT_CBF_CTX + ctx]);
}

void SBac::codeQtCbfZero(TComDataCU* cu, TextType ttype, uint32_t trDepth)
{
    // this function is only used to estimate the bits when cbf is 0
    // and will never be called when writing the bistream. do not need to write log
    uint32_t cbf = 0;
    uint32_t ctx = cu->getCtxQtCbf(ttype, trDepth);

    encodeBin(cbf, m_contextModels[OFF_QT_CBF_CTX + ctx]);
}

void SBac::codeQtRootCbfZero(TComDataCU*)
{
    // this function is only used to estimate the bits when cbf is 0
    // and will never be called when writing the bistream. do not need to write log
    uint32_t cbf = 0;
    uint32_t ctx = 0;

    encodeBin(cbf, m_contextModels[OFF_QT_ROOT_CBF_CTX + ctx]);
}

/** Encode (X,Y) position of the last significant coefficient
 * \param posx X component of last coefficient
 * \param posy Y component of last coefficient
 * \param width  Block width
 * \param height Block height
 * \param ttype plane type / luminance or chrominance
 * \param uiScanIdx scan type (zig-zag, hor, ver)
 * This method encodes the X and Y component within a block of the last significant coefficient.
 */
void SBac::codeLastSignificantXY(uint32_t posx, uint32_t posy, uint32_t log2TrSize, TextType ttype, uint32_t scanIdx)
{
    X265_CHECK((ttype == TEXT_LUMA) || (ttype == TEXT_CHROMA), "invalid texture type\n");

    // swap
    if (scanIdx == SCAN_VER)
        std::swap(posx, posy);

    uint32_t ctxLast;
    uint32_t groupIdxX = getGroupIdx(posx);
    uint32_t groupIdxY = getGroupIdx(posy);

    int blkSizeOffset = ttype ? NUM_CTX_LAST_FLAG_XY_LUMA : ((log2TrSize - 2) * 3 + ((log2TrSize - 1) >> 2));
    int ctxShift = ttype ? log2TrSize - 2 : ((log2TrSize + 1) >> 2);
    uint32_t maxGroupIdx = log2TrSize * 2 - 1;

    // posX
    ContextModel *ctxX = &m_contextModels[OFF_CTX_LAST_FLAG_X];
    for (ctxLast = 0; ctxLast < groupIdxX; ctxLast++)
        encodeBin(1, *(ctxX + blkSizeOffset + (ctxLast >> ctxShift)));

    if (groupIdxX < maxGroupIdx)
        encodeBin(0, *(ctxX + blkSizeOffset + (ctxLast >> ctxShift)));

    // posY
    ContextModel *ctxY = &m_contextModels[OFF_CTX_LAST_FLAG_Y];
    for (ctxLast = 0; ctxLast < groupIdxY; ctxLast++)
        encodeBin(1, *(ctxY + blkSizeOffset + (ctxLast >> ctxShift)));

    if (groupIdxY < maxGroupIdx)
        encodeBin(0, *(ctxY + blkSizeOffset + (ctxLast >> ctxShift)));

    if (groupIdxX > 3)
    {
        uint32_t count = (groupIdxX - 2) >> 1;
        posx = posx - g_minInGroup[groupIdxX];
        encodeBinsEP(posx, count);
    }
    if (groupIdxY > 3)
    {
        uint32_t count = (groupIdxY - 2) >> 1;
        posy = posy - g_minInGroup[groupIdxY];
        encodeBinsEP(posy, count);
    }
}

void SBac::codeCoeffNxN(TComDataCU* cu, coeff_t* coeff, uint32_t absPartIdx, uint32_t log2TrSize, TextType ttype)
{
    uint32_t trSize = 1 << log2TrSize;

    // compute number of significant coefficients
    uint32_t numSig = primitives.count_nonzero(coeff, (1 << (log2TrSize << 1)));

    X265_CHECK(numSig > 0, "cbf check fail\n");

    bool beValid;
    if (cu->getCUTransquantBypass(absPartIdx))
        beValid = false;
    else
        beValid = cu->getSlice()->getPPS()->getSignHideFlag() > 0;

    if (cu->getSlice()->getPPS()->getUseTransformSkip())
        codeTransformSkipFlags(cu, absPartIdx, trSize, ttype);

    ttype = ttype == TEXT_LUMA ? TEXT_LUMA : TEXT_CHROMA;

    // select scans
    TUEntropyCodingParameters codingParameters;
    TComTrQuant::getTUEntropyCodingParameters(cu, codingParameters, absPartIdx, log2TrSize, ttype);

    //----- encode significance map -----

    // Find position of last coefficient
    int scanPosLast = 0;
    uint32_t posLast;
    uint64_t sigCoeffGroupFlag64 = 0;
    const uint32_t maskPosXY = ((uint32_t)~0 >> (31 - log2TrSize + MLS_CG_LOG2_SIZE)) >> 1;
    assert((uint32_t)((1 << (log2TrSize - MLS_CG_LOG2_SIZE)) - 1) == (((uint32_t)~0 >> (31 - log2TrSize + MLS_CG_LOG2_SIZE)) >> 1));
    do
    {
        posLast = codingParameters.scan[scanPosLast++];

        const uint32_t isNZCoeff = (coeff[posLast] != 0);
        // get L1 sig map
        // NOTE: the new algorithm is complicated, so I keep reference code here
        //uint32_t posy   = posLast >> log2TrSize;
        //uint32_t posx   = posLast - (posy << log2TrSize);
        //uint32_t blkIdx0 = ((posy >> MLS_CG_LOG2_SIZE) << codingParameters.log2TrSizeCG) + (posx >> MLS_CG_LOG2_SIZE);
        const uint32_t blkIdx = ((posLast >> (2 * MLS_CG_LOG2_SIZE)) & ~maskPosXY) + ((posLast >> MLS_CG_LOG2_SIZE) & maskPosXY);
        sigCoeffGroupFlag64 |= ((uint64_t)isNZCoeff << blkIdx);
        numSig -= isNZCoeff;
    }
    while (numSig > 0);
    scanPosLast--;

    // Code position of last coefficient
    int posLastY = posLast >> log2TrSize;
    int posLastX = posLast & (trSize - 1);
    codeLastSignificantXY(posLastX, posLastY, log2TrSize, ttype, codingParameters.scanType);
    //===== code significance flag =====
    ContextModel * const baseCoeffGroupCtx = &m_contextModels[OFF_SIG_CG_FLAG_CTX + (ttype ? NUM_SIG_CG_FLAG_CTX : 0)];
    ContextModel * const baseCtx = (ttype == TEXT_LUMA) ? &m_contextModels[OFF_SIG_FLAG_CTX] : &m_contextModels[OFF_SIG_FLAG_CTX + NUM_SIG_FLAG_CTX_LUMA];
    const int lastScanSet = scanPosLast >> MLS_CG_SIZE;
    uint32_t c1 = 1;
    uint32_t goRiceParam = 0;
    int scanPosSig = scanPosLast;

    for (int subSet = lastScanSet; subSet >= 0; subSet--)
    {
        int numNonZero = 0;
        int subPos     = subSet << MLS_CG_SIZE;
        goRiceParam    = 0;
        int absCoeff[1 << MLS_CG_SIZE];
        uint32_t coeffSigns = 0;
        int lastNZPosInCG = -1;
        int firstNZPosInCG = 1 << MLS_CG_SIZE;
        if (scanPosSig == scanPosLast)
        {
            absCoeff[0] = int(abs(coeff[posLast]));
            coeffSigns  = (coeff[posLast] < 0);
            numNonZero  = 1;
            lastNZPosInCG  = scanPosSig;
            firstNZPosInCG = scanPosSig;
            scanPosSig--;
        }
        // encode significant_coeffgroup_flag
        const int cgBlkPos = codingParameters.scanCG[subSet];
        const int cgPosY   = cgBlkPos >> codingParameters.log2TrSizeCG;
        const int cgPosX   = cgBlkPos - (cgPosY << codingParameters.log2TrSizeCG);
        const uint64_t cgBlkPosMask = ((uint64_t)1 << cgBlkPos);

        if (subSet == lastScanSet || subSet == 0)
            sigCoeffGroupFlag64 |= cgBlkPosMask;
        else
        {
            uint32_t sigCoeffGroup = ((sigCoeffGroupFlag64 & cgBlkPosMask) != 0);
            uint32_t ctxSig = TComTrQuant::getSigCoeffGroupCtxInc(sigCoeffGroupFlag64, cgPosX, cgPosY, codingParameters.log2TrSizeCG);
            encodeBin(sigCoeffGroup, baseCoeffGroupCtx[ctxSig]);
        }

        // encode significant_coeff_flag
        if (sigCoeffGroupFlag64 & cgBlkPosMask)
        {
            const int patternSigCtx = TComTrQuant::calcPatternSigCtx(sigCoeffGroupFlag64, cgPosX, cgPosY, codingParameters.log2TrSizeCG);
            uint32_t blkPos, sig, ctxSig;
            for (; scanPosSig >= subPos; scanPosSig--)
            {
                blkPos  = codingParameters.scan[scanPosSig];
                sig     = (coeff[blkPos] != 0);
                if (scanPosSig > subPos || subSet == 0 || numNonZero)
                {
                    ctxSig = TComTrQuant::getSigCtxInc(patternSigCtx, log2TrSize, trSize, blkPos, ttype, codingParameters.firstSignificanceMapContext);
                    encodeBin(sig, baseCtx[ctxSig]);
                }
                if (sig)
                {
                    absCoeff[numNonZero] = int(abs(coeff[blkPos]));
                    coeffSigns = 2 * coeffSigns + ((uint32_t)coeff[blkPos] >> 31);
                    numNonZero++;
                    if (lastNZPosInCG < 0)
                        lastNZPosInCG = scanPosSig;
                    firstNZPosInCG = scanPosSig;
                }
            }
        }
        else
            scanPosSig = subPos - 1;

        if (numNonZero > 0)
        {
            bool signHidden = (lastNZPosInCG - firstNZPosInCG >= SBH_THRESHOLD);
            uint32_t ctxSet = (subSet > 0 && ttype == TEXT_LUMA) ? 2 : 0;

            if (c1 == 0)
                ctxSet++;

            c1 = 1;
            ContextModel *baseCtxMod = (ttype == TEXT_LUMA) ? &m_contextModels[OFF_ONE_FLAG_CTX + 4 * ctxSet] : &m_contextModels[OFF_ONE_FLAG_CTX + NUM_ONE_FLAG_CTX_LUMA + 4 * ctxSet];

            int numC1Flag = X265_MIN(numNonZero, C1FLAG_NUMBER);
            int firstC2FlagIdx = -1;
            for (int idx = 0; idx < numC1Flag; idx++)
            {
                uint32_t symbol = absCoeff[idx] > 1;
                encodeBin(symbol, baseCtxMod[c1]);
                if (symbol)
                {
                    c1 = 0;

                    if (firstC2FlagIdx == -1)
                        firstC2FlagIdx = idx;
                }
                else if ((c1 < 3) && (c1 > 0))
                    c1++;
            }

            if (c1 == 0)
            {
                baseCtxMod = (ttype == TEXT_LUMA) ? &m_contextModels[OFF_ABS_FLAG_CTX + ctxSet] : &m_contextModels[OFF_ABS_FLAG_CTX + NUM_ABS_FLAG_CTX_LUMA + ctxSet];
                if (firstC2FlagIdx != -1)
                {
                    uint32_t symbol = absCoeff[firstC2FlagIdx] > 2;
                    encodeBin(symbol, baseCtxMod[0]);
                }
            }

            if (beValid && signHidden)
                encodeBinsEP((coeffSigns >> 1), numNonZero - 1);
            else
                encodeBinsEP(coeffSigns, numNonZero);

            int firstCoeff2 = 1;
            if (c1 == 0 || numNonZero > C1FLAG_NUMBER)
            {
                for (int idx = 0; idx < numNonZero; idx++)
                {
                    int baseLevel = (idx < C1FLAG_NUMBER) ? (2 + firstCoeff2) : 1;

                    if (absCoeff[idx] >= baseLevel)
                    {
                        writeCoefRemainExGolomb(absCoeff[idx] - baseLevel, goRiceParam);
                        if (absCoeff[idx] > 3 * (1 << goRiceParam))
                            goRiceParam = std::min<uint32_t>(goRiceParam + 1, 4);
                    }
                    if (absCoeff[idx] >= 2)
                        firstCoeff2 = 0;
                }
            }
        }
    }
}

void SBac::codeSaoMaxUvlc(uint32_t code, uint32_t maxSymbol)
{
    X265_CHECK(maxSymbol > 0, "maxSymbol too small\n");

    uint32_t isCodeLast = (maxSymbol > code) ? 1 : 0;
    uint32_t isCodeNonZero = (code != 0) ? 1 : 0;

    encodeBinEP(isCodeNonZero);
    if (isCodeNonZero)
    {
        uint32_t mask = (1 << (code - 1)) - 1;
        uint32_t len = code - 1 + isCodeLast;
        mask <<= isCodeLast;

        encodeBinsEP(mask, len);
    }
}

/** Code SAO type index */
void SBac::codeSaoTypeIdx(uint32_t code)
{
    encodeBin((code == 0) ? 0 : 1, m_contextModels[OFF_SAO_TYPE_IDX_CTX]);
    if (code)
        encodeBinEP(code <= 4 ? 1 : 0);
}

/* estimate bit cost for CBP, significant map and significant coefficients */
void SBac::estBit(EstBitsSbac* estBitsSbac, uint32_t log2TrSize, TextType ttype)
{
    estCBFBit(estBitsSbac);

    estSignificantCoeffGroupMapBit(estBitsSbac, ttype);

    // encode significance map
    estSignificantMapBit(estBitsSbac, log2TrSize, ttype);

    // encode significant coefficients
    estSignificantCoefficientsBit(estBitsSbac, ttype);
}

/* estimate bit cost for each CBP bit */
void SBac::estCBFBit(EstBitsSbac* estBitsSbac)
{
    ContextModel *ctx = &m_contextModels[OFF_QT_CBF_CTX];

    for (uint32_t ctxInc = 0; ctxInc < NUM_QT_CBF_CTX; ctxInc++)
    {
        estBitsSbac->blockCbpBits[ctxInc][0] = sbacGetEntropyBits(ctx[ctxInc].m_state, 0);
        estBitsSbac->blockCbpBits[ctxInc][1] = sbacGetEntropyBits(ctx[ctxInc].m_state, 1);
    }

    ctx = &m_contextModels[OFF_QT_ROOT_CBF_CTX];

    for (uint32_t ctxInc = 0; ctxInc < NUM_QT_ROOT_CBF_CTX; ctxInc++)
    {
        estBitsSbac->blockRootCbpBits[ctxInc][0] = sbacGetEntropyBits(ctx[ctxInc].m_state, 0);
        estBitsSbac->blockRootCbpBits[ctxInc][1] = sbacGetEntropyBits(ctx[ctxInc].m_state, 1);
    }
}

/* estimate SAMBAC bit cost for significant coefficient group map */
void SBac::estSignificantCoeffGroupMapBit(EstBitsSbac* estBitsSbac, TextType ttype)
{
    X265_CHECK((ttype == TEXT_LUMA) || (ttype == TEXT_CHROMA), "invalid texture type\n");
    int firstCtx = 0, numCtx = NUM_SIG_CG_FLAG_CTX;

    for (int ctxIdx = firstCtx; ctxIdx < firstCtx + numCtx; ctxIdx++)
        for (uint32_t bin = 0; bin < 2; bin++)
            estBitsSbac->significantCoeffGroupBits[ctxIdx][bin] = sbacGetEntropyBits(m_contextModels[OFF_SIG_CG_FLAG_CTX + ((ttype ? NUM_SIG_CG_FLAG_CTX : 0) + ctxIdx)].m_state, bin);
}

/* estimate SAMBAC bit cost for significant coefficient map */
void SBac::estSignificantMapBit(EstBitsSbac* estBitsSbac, uint32_t log2TrSize, TextType ttype)
{
    int firstCtx = 1, numCtx = 8;

    if (log2TrSize >= 4)
    {
        firstCtx = (ttype == TEXT_LUMA) ? 21 : 12;
        numCtx = (ttype == TEXT_LUMA) ? 6 : 3;
    }
    else if (log2TrSize == 3)
    {
        firstCtx = 9;
        numCtx = (ttype == TEXT_LUMA) ? 12 : 3;
    }

    if (ttype == TEXT_LUMA)
    {
        for (uint32_t bin = 0; bin < 2; bin++)
        {
            estBitsSbac->significantBits[0][bin] = sbacGetEntropyBits(m_contextModels[OFF_SIG_FLAG_CTX].m_state, bin);
        }

        for (int ctxIdx = firstCtx; ctxIdx < firstCtx + numCtx; ctxIdx++)
        {
            for (uint32_t bin = 0; bin < 2; bin++)
            {
                estBitsSbac->significantBits[ctxIdx][bin] = sbacGetEntropyBits(m_contextModels[OFF_SIG_FLAG_CTX + ctxIdx].m_state, bin);
            }
        }
    }
    else
    {
        for (uint32_t bin = 0; bin < 2; bin++)
        {
            estBitsSbac->significantBits[0][bin] = sbacGetEntropyBits(m_contextModels[OFF_SIG_FLAG_CTX + (NUM_SIG_FLAG_CTX_LUMA + 0)].m_state, bin);
        }

        for (int ctxIdx = firstCtx; ctxIdx < firstCtx + numCtx; ctxIdx++)
        {
            for (uint32_t bin = 0; bin < 2; bin++)
            {
                estBitsSbac->significantBits[ctxIdx][bin] = sbacGetEntropyBits(m_contextModels[OFF_SIG_FLAG_CTX + (NUM_SIG_FLAG_CTX_LUMA + ctxIdx)].m_state, bin);
            }
        }
    }
    int bitsX = 0, bitsY = 0;

    int blkSizeOffset = ttype ? NUM_CTX_LAST_FLAG_XY_LUMA : ((log2TrSize - 2) * 3 + ((log2TrSize - 1) >> 2));
    int ctxShift = ttype ? log2TrSize - 2 : ((log2TrSize + 1) >> 2);
    uint32_t maxGroupIdx = log2TrSize * 2 - 1;

    X265_CHECK((ttype == TEXT_LUMA) || (ttype == TEXT_CHROMA), "invalid texture type\n");
    uint32_t ctx;
    const ContextModel *ctxX = &m_contextModels[OFF_CTX_LAST_FLAG_X];
    for (ctx = 0; ctx < maxGroupIdx; ctx++)
    {
        int ctxOffset = blkSizeOffset + (ctx >> ctxShift);
        estBitsSbac->lastXBits[ctx] = bitsX + sbacGetEntropyBits(ctxX[ctxOffset].m_state, 0);
        bitsX += sbacGetEntropyBits(ctxX[ctxOffset].m_state, 1);
    }

    estBitsSbac->lastXBits[ctx] = bitsX;

    const ContextModel *ctxY = &m_contextModels[OFF_CTX_LAST_FLAG_Y];
    for (ctx = 0; ctx < maxGroupIdx; ctx++)
    {
        int ctxOffset = blkSizeOffset + (ctx >> ctxShift);
        estBitsSbac->lastYBits[ctx] = bitsY + sbacGetEntropyBits(ctxY[ctxOffset].m_state, 0);
        bitsY += sbacGetEntropyBits(ctxY[ctxOffset].m_state, 1);
    }

    estBitsSbac->lastYBits[ctx] = bitsY;
}

/* estimate bit cost of significant coefficient */
void SBac::estSignificantCoefficientsBit(EstBitsSbac* estBitsSbac, TextType ttype)
{
    if (ttype == TEXT_LUMA)
    {
        ContextModel *ctxOne = &m_contextModels[OFF_ONE_FLAG_CTX];
        ContextModel *ctxAbs = &m_contextModels[OFF_ABS_FLAG_CTX];

        for (int ctxIdx = 0; ctxIdx < NUM_ONE_FLAG_CTX_LUMA; ctxIdx++)
        {
            estBitsSbac->greaterOneBits[ctxIdx][0] = sbacGetEntropyBits(ctxOne[ctxIdx].m_state, 0);
            estBitsSbac->greaterOneBits[ctxIdx][1] = sbacGetEntropyBits(ctxOne[ctxIdx].m_state, 1);
        }

        for (int ctxIdx = 0; ctxIdx < NUM_ABS_FLAG_CTX_LUMA; ctxIdx++)
        {
            estBitsSbac->levelAbsBits[ctxIdx][0] = sbacGetEntropyBits(ctxAbs[ctxIdx].m_state, 0);
            estBitsSbac->levelAbsBits[ctxIdx][1] = sbacGetEntropyBits(ctxAbs[ctxIdx].m_state, 1);
        }
    }
    else
    {
        ContextModel *ctxOne = &m_contextModels[OFF_ONE_FLAG_CTX + NUM_ONE_FLAG_CTX_LUMA];
        ContextModel *ctxAbs = &m_contextModels[OFF_ABS_FLAG_CTX + NUM_ABS_FLAG_CTX_LUMA];

        for (int ctxIdx = 0; ctxIdx < NUM_ONE_FLAG_CTX_CHROMA; ctxIdx++)
        {
            estBitsSbac->greaterOneBits[ctxIdx][0] = sbacGetEntropyBits(ctxOne[ctxIdx].m_state, 0);
            estBitsSbac->greaterOneBits[ctxIdx][1] = sbacGetEntropyBits(ctxOne[ctxIdx].m_state, 1);
        }

        for (int ctxIdx = 0; ctxIdx < NUM_ABS_FLAG_CTX_CHROMA; ctxIdx++)
        {
            estBitsSbac->levelAbsBits[ctxIdx][0] = sbacGetEntropyBits(ctxAbs[ctxIdx].m_state, 0);
            estBitsSbac->levelAbsBits[ctxIdx][1] = sbacGetEntropyBits(ctxAbs[ctxIdx].m_state, 1);
        }
    }
}

/* Initialize our context information from the nominated source */
void SBac::copyContextsFrom(SBac& src)
{
    memcpy(m_contextModels, src.m_contextModels, MAX_OFF_CTX_MOD * sizeof(m_contextModels[0]));
}

void SBac::start()
{
    m_low = 0;
    m_range = 510;
    m_bitsLeft = -12;
    m_numBufferedBytes = 0;
    m_bufferedByte = 0xff;
}

void SBac::finish()
{
    if (m_low >> (21 + m_bitsLeft))
    {
        m_bitIf->writeByte(m_bufferedByte + 1);
        while (m_numBufferedBytes > 1)
        {
            m_bitIf->writeByte(0x00);
            m_numBufferedBytes--;
        }

        m_low -= 1 << (21 + m_bitsLeft);
    }
    else
    {
        if (m_numBufferedBytes > 0)
            m_bitIf->writeByte(m_bufferedByte);

        while (m_numBufferedBytes > 1)
        {
            m_bitIf->writeByte(0xff);
            m_numBufferedBytes--;
        }
    }
    m_bitIf->write(m_low >> 8, 13 + m_bitsLeft);
}

void SBac::flush()
{
    encodeBinTrm(1);
    finish();
    m_bitIf->write(1, 1);
    m_bitIf->writeAlignZero();

    start();
}

void SBac::copyState(SBac& other)
{
    m_low = other.m_low;
    m_range = other.m_range;
    m_bitsLeft = other.m_bitsLeft;
    m_bufferedByte = other.m_bufferedByte;
    m_numBufferedBytes = other.m_numBufferedBytes;
    m_fracBits = other.m_fracBits;
}

void SBac::resetBits()
{
    m_low = 0;
    m_bitsLeft = -12;
    m_numBufferedBytes = 0;
    m_bufferedByte = 0xff;
    m_fracBits &= 32767;
    if (m_bitIf)
        m_bitIf->resetBits();
}

/** Encode bin */
void SBac::encodeBin(uint32_t binValue, ContextModel &ctxModel)
{
    uint32_t mstate = ctxModel.m_state;

    ctxModel.m_state = sbacNext(mstate, binValue);

    if (!m_bitIf)
    {
        m_fracBits += sbacGetEntropyBits(mstate, binValue);
        return;
    }
    ctxModel.bBinsCoded = 1;

    uint32_t range = m_range;
    uint32_t state = sbacGetState(mstate);
    uint32_t lps = g_lpsTable[state][((uint8_t)range >> 6)];
    range -= lps;

    X265_CHECK(lps >= 2, "lps is too small\n");

    int numBits = (uint32_t)(range - 256) >> 31;
    uint32_t low = m_low;

    // NOTE: MPS must be LOWEST bit in mstate
    X265_CHECK((uint32_t)((binValue ^ mstate) & 1) == (uint32_t)(binValue != sbacGetMps(mstate)), "binValue failure\n");
    if ((binValue ^ mstate) & 1)
    {
        // NOTE: lps is non-zero and the maximum of idx is 8 because lps less than 256
        //numBits   = g_renormTable[lps >> 3];
        unsigned long idx;
        CLZ32(idx, lps);
        X265_CHECK(state != 63 || idx == 1, "state failure\n");

        numBits = 8 - idx;
        if (state >= 63)
            numBits = 6;
        X265_CHECK(numBits <= 6, "numBits failure\n");

        low += range;
        range = lps;
    }
    m_low = (low << numBits);
    m_range = (range << numBits);
    m_bitsLeft += numBits;

    if (m_bitsLeft >= 0)
        writeOut();
}

/** Encode equiprobable bin */
void SBac::encodeBinEP(uint32_t binValue)
{
    if (!m_bitIf)
    {
        m_fracBits += 32768;
        return;
    }
    m_low <<= 1;
    if (binValue)
        m_low += m_range;
    m_bitsLeft++;

    if (m_bitsLeft >= 0)
        writeOut();
}

/** Encode equiprobable bins */
void SBac::encodeBinsEP(uint32_t binValues, int numBins)
{
    if (!m_bitIf)
    {
        m_fracBits += 32768 * numBins;
        return;
    }

    while (numBins > 8)
    {
        numBins -= 8;
        uint32_t pattern = binValues >> numBins;
        m_low <<= 8;
        m_low += m_range * pattern;
        binValues -= pattern << numBins;
        m_bitsLeft += 8;

        if (m_bitsLeft >= 0)
            writeOut();
    }

    m_low <<= numBins;
    m_low += m_range * binValues;
    m_bitsLeft += numBins;

    if (m_bitsLeft >= 0)
        writeOut();
}

/** Encode terminating bin */
void SBac::encodeBinTrm(uint32_t binValue)
{
    if (!m_bitIf)
    {
        m_fracBits += sbacGetEntropyBitsTrm(binValue);
        return;
    }

    m_range -= 2;
    if (binValue)
    {
        m_low += m_range;
        m_low <<= 7;
        m_range = 2 << 7;
        m_bitsLeft += 7;
    }
    else if (m_range >= 256)
        return;
    else
    {
        m_low <<= 1;
        m_range <<= 1;
        m_bitsLeft++;
    }

    if (m_bitsLeft >= 0)
        writeOut();
}

/** Move bits from register into bitstream */
void SBac::writeOut()
{
    uint32_t leadByte = m_low >> (13 + m_bitsLeft);
    uint32_t low_mask = (uint32_t)(~0) >> (11 + 8 - m_bitsLeft);

    m_bitsLeft -= 8;
    m_low &= low_mask;

    if (leadByte == 0xff)
        m_numBufferedBytes++;
    else
    {
        uint32_t numBufferedBytes = m_numBufferedBytes;
        if (numBufferedBytes > 0)
        {
            uint32_t carry = leadByte >> 8;
            uint32_t byteTowrite = m_bufferedByte + carry;
            m_bitIf->writeByte(byteTowrite);

            byteTowrite = (0xff + carry) & 0xff;
            while (numBufferedBytes > 1)
            {
                m_bitIf->writeByte(byteTowrite);
                numBufferedBytes--;
            }
        }
        m_numBufferedBytes = 1;
        m_bufferedByte = (uint8_t)leadByte;
    }
}

}
