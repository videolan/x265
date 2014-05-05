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

/** \file     TEncEntropy.cpp
    \brief    entropy encoder class
*/

#include "TEncEntropy.h"
#include "TLibCommon/TypeDef.h"
#include "TLibCommon/TComSampleAdaptiveOffset.h"

using namespace x265;

//! \ingroup TLibEncoder
//! \{

void TEncEntropy::setEntropyCoder(TEncEntropyIf* e, TComSlice* slice)
{
    m_entropyCoderIf = e;
    m_entropyCoderIf->setSlice(slice);
}

void TEncEntropy::encodeSliceHeader(TComSlice* slice)
{
    if (slice->getSPS()->getUseSAO())
    {
        SAOParam *saoParam = slice->getPic()->getPicSym()->getSaoParam();
        slice->setSaoEnabledFlag(saoParam->bSaoFlag[0]);
        {
            slice->setSaoEnabledFlagChroma(saoParam->bSaoFlag[1]);
        }
    }

    m_entropyCoderIf->codeSliceHeader(slice);
}

void  TEncEntropy::encodeTilesWPPEntryPoint(TComSlice* pSlice)
{
    m_entropyCoderIf->codeTilesWPPEntryPoint(pSlice);
}

void TEncEntropy::encodeTerminatingBit(uint32_t isLast)
{
    m_entropyCoderIf->codeTerminatingBit(isLast);
}

void TEncEntropy::encodeSliceFinish()
{
    m_entropyCoderIf->codeSliceFinish();
}

void TEncEntropy::encodePPS(TComPPS* pps)
{
    m_entropyCoderIf->codePPS(pps);
}

void TEncEntropy::encodeSPS(TComSPS* sps)
{
    m_entropyCoderIf->codeSPS(sps);
}

void TEncEntropy::encodeAUD(TComSlice* slice)
{
    m_entropyCoderIf->codeAUD(slice);
}

void TEncEntropy::encodeCUTransquantBypassFlag(TComDataCU* cu, uint32_t absPartIdx)
{
    m_entropyCoderIf->codeCUTransquantBypassFlag(cu, absPartIdx);
}

void TEncEntropy::encodeVPS(TComVPS* vps)
{
    m_entropyCoderIf->codeVPS(vps);
}

void TEncEntropy::encodeSkipFlag(TComDataCU* cu, uint32_t absPartIdx)
{
    if (cu->getSlice()->isIntra())
    {
        return;
    }
    m_entropyCoderIf->codeSkipFlag(cu, absPartIdx);
}

/** encode merge flag
 * \param cu
 * \param absPartIdx
 * \returns void
 */
void TEncEntropy::encodeMergeFlag(TComDataCU* cu, uint32_t absPartIdx)
{
    // at least one merge candidate exists
    m_entropyCoderIf->codeMergeFlag(cu, absPartIdx);
}

/** encode merge index
 * \param cu
 * \param absPartIdx
 * \param uiPUIdx
 * \returns void
 */
void TEncEntropy::encodeMergeIndex(TComDataCU* cu, uint32_t absPartIdx)
{
    m_entropyCoderIf->codeMergeIndex(cu, absPartIdx);
}

/** encode prediction mode
 * \param cu
 * \param absPartIdx
 * \returns void
 */
void TEncEntropy::encodePredMode(TComDataCU* cu, uint32_t absPartIdx)
{
    if (cu->getSlice()->isIntra())
    {
        return;
    }

    m_entropyCoderIf->codePredMode(cu, absPartIdx);
}

// Split mode
void TEncEntropy::encodeSplitFlag(TComDataCU* cu, uint32_t absPartIdx, uint32_t depth)
{
    m_entropyCoderIf->codeSplitFlag(cu, absPartIdx, depth);
}

/** encode partition size
 * \param cu
 * \param absPartIdx
 * \param depth
 * \returns void
 */
void TEncEntropy::encodePartSize(TComDataCU* cu, uint32_t absPartIdx, uint32_t depth)
{
    m_entropyCoderIf->codePartSize(cu, absPartIdx, depth);
}

/** Encode I_PCM information.
 * \param cu pointer to CU
 * \param absPartIdx CU index
 * \returns void
 */
void TEncEntropy::encodeIPCMInfo(TComDataCU* cu, uint32_t absPartIdx)
{
    if (!cu->getSlice()->getSPS()->getUsePCM()
        || cu->getCUSize(absPartIdx) > (1 << cu->getSlice()->getSPS()->getPCMLog2MaxSize())
        || cu->getCUSize(absPartIdx) < (1 << cu->getSlice()->getSPS()->getPCMLog2MinSize()))
    {
        return;
    }

    m_entropyCoderIf->codeIPCMInfo(cu, absPartIdx);
}

bool TEncEntropy::isNextTUSection(TComTURecurse *tuIterator)
{
    if (tuIterator->m_splitMode == DONT_SPLIT)
    {
        tuIterator->m_section++;
        return false;
    }
    else
    {
        tuIterator->m_absPartIdxTURelCU += tuIterator->m_absPartIdxStep;

        tuIterator->m_section++;
        return tuIterator->m_section < (1 << tuIterator->m_splitMode);
    }
}

void TEncEntropy::initTUEntropySection(TComTURecurse *tuIterator, uint32_t splitMode, uint32_t absPartIdxStep, uint32_t m_absPartIdxTU)
{
    tuIterator->m_partOffset        = 0;
    tuIterator->m_section           = 0;
    tuIterator->m_absPartIdxTURelCU = m_absPartIdxTU;
    tuIterator->m_splitMode         = splitMode;
    tuIterator->m_absPartIdxStep    = absPartIdxStep >> partIdxStepShift[splitMode];
}

void TEncEntropy::xEncodeTransform(TComDataCU* cu, uint32_t offsetLuma, uint32_t offsetChroma, uint32_t absPartIdx, uint32_t absPartIdxStep, uint32_t depth, uint32_t width, uint32_t height, uint32_t trIdx, bool& bCodeDQP)
{
    const uint32_t subdiv = cu->getTransformIdx(absPartIdx) + cu->getDepth(absPartIdx) > depth;
    const uint32_t log2TrafoSize = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUSize()] + 2 - depth;
    uint32_t hChromaShift        = cu->getHorzChromaShift();
    uint32_t vChromaShift        = cu->getVertChromaShift();
    uint32_t cbfY = cu->getCbf(absPartIdx, TEXT_LUMA, trIdx);
    uint32_t cbfU = cu->getCbf(absPartIdx, TEXT_CHROMA_U, trIdx);
    uint32_t cbfV = cu->getCbf(absPartIdx, TEXT_CHROMA_V, trIdx);

    if (trIdx == 0)
    {
        m_bakAbsPartIdxCU = absPartIdx;
    }

    if ((log2TrafoSize == 2) && !(cu->getChromaFormat() == CHROMA_444))
    {
        uint32_t partNum = cu->getPic()->getNumPartInCU() >> ((depth - 1) << 1);
        if ((absPartIdx % partNum) == 0)
        {
            m_bakAbsPartIdx   = absPartIdx;
            m_bakChromaOffset = offsetChroma;
        }
        else if ((absPartIdx % partNum) == (partNum - 1))
        {
            cbfU = cu->getCbf(m_bakAbsPartIdx, TEXT_CHROMA_U, trIdx);
            cbfV = cu->getCbf(m_bakAbsPartIdx, TEXT_CHROMA_V, trIdx);
        }
    }

    if (cu->getPredictionMode(absPartIdx) == MODE_INTRA && cu->getPartitionSize(absPartIdx) == SIZE_NxN && depth == cu->getDepth(absPartIdx))
    {
        assert(subdiv);
    }
    else if (cu->getPredictionMode(absPartIdx) == MODE_INTER && (cu->getPartitionSize(absPartIdx) != SIZE_2Nx2N) && depth == cu->getDepth(absPartIdx) &&  (cu->getSlice()->getSPS()->getQuadtreeTUMaxDepthInter() == 1))
    {
        if (log2TrafoSize > cu->getQuadtreeTULog2MinSizeInCU(absPartIdx))
        {
            assert(subdiv);
        }
        else
        {
            assert(!subdiv);
        }
    }
    else if (log2TrafoSize > cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize())
    {
        assert(subdiv);
    }
    else if (log2TrafoSize == cu->getSlice()->getSPS()->getQuadtreeTULog2MinSize())
    {
        assert(!subdiv);
    }
    else if (log2TrafoSize == cu->getQuadtreeTULog2MinSizeInCU(absPartIdx))
    {
        assert(!subdiv);
    }
    else
    {
        assert(log2TrafoSize > cu->getQuadtreeTULog2MinSizeInCU(absPartIdx));
        m_entropyCoderIf->codeTransformSubdivFlag(subdiv, 5 - log2TrafoSize);
    }

    const uint32_t trDepthCurr = depth - cu->getDepth(absPartIdx);
    const bool bFirstCbfOfCU = trDepthCurr == 0;

    bool mCodeAll = true;
    const uint32_t numPels = (width >> hChromaShift) * (height >> vChromaShift);
    if (numPels < (MIN_TU_SIZE * MIN_TU_SIZE))
    {
        mCodeAll = false;
    }

    if (bFirstCbfOfCU || mCodeAll)
    {
        if (bFirstCbfOfCU || cu->getCbf(absPartIdx, TEXT_CHROMA_U, trDepthCurr - 1))
        {
            m_entropyCoderIf->codeQtCbf(cu, absPartIdx, TEXT_CHROMA_U, trDepthCurr, absPartIdxStep, (width  >> hChromaShift), (height >> vChromaShift), (subdiv == 0));
        }
        if (bFirstCbfOfCU || cu->getCbf(absPartIdx, TEXT_CHROMA_V, trDepthCurr - 1))
        {
            m_entropyCoderIf->codeQtCbf(cu, absPartIdx, TEXT_CHROMA_V, trDepthCurr, absPartIdxStep, (width  >> hChromaShift), (height >> vChromaShift), (subdiv == 0));
        }
    }
    else
    {
        assert(cu->getCbf(absPartIdx, TEXT_CHROMA_U, trDepthCurr) == cu->getCbf(absPartIdx, TEXT_CHROMA_U, trDepthCurr - 1));
        assert(cu->getCbf(absPartIdx, TEXT_CHROMA_V, trDepthCurr) == cu->getCbf(absPartIdx, TEXT_CHROMA_V, trDepthCurr - 1));
    }

    if (subdiv)
    {
        uint32_t size;
        width  >>= 1;
        height >>= 1;
        size = width * height;
        trIdx++;
        ++depth;
        absPartIdxStep    = absPartIdxStep >> partIdxStepShift[QUAD_SPLIT];
        const uint32_t partNum = cu->getPic()->getNumPartInCU() >> (depth << 1);

        xEncodeTransform(cu, offsetLuma, offsetChroma, absPartIdx, absPartIdxStep, depth, width, height, trIdx, bCodeDQP);

        absPartIdx += partNum;
        offsetLuma += size;
        offsetChroma += (size >> (hChromaShift + vChromaShift));
        xEncodeTransform(cu, offsetLuma, offsetChroma, absPartIdx, absPartIdxStep, depth, width, height, trIdx, bCodeDQP);

        absPartIdx += partNum;
        offsetLuma += size;
        offsetChroma += (size >> (hChromaShift + vChromaShift));
        xEncodeTransform(cu, offsetLuma, offsetChroma, absPartIdx, absPartIdxStep, depth, width, height, trIdx, bCodeDQP);

        absPartIdx += partNum;
        offsetLuma += size;
        offsetChroma += (size >> (hChromaShift + vChromaShift));
        xEncodeTransform(cu, offsetLuma, offsetChroma, absPartIdx, absPartIdxStep, depth, width, height, trIdx, bCodeDQP);
    }
    else
    {
        {
            DTRACE_CABAC_VL(g_nSymbolCounter++);
            DTRACE_CABAC_T("\tTrIdx: abspart=");
            DTRACE_CABAC_V(absPartIdx);
            DTRACE_CABAC_T("\tdepth=");
            DTRACE_CABAC_V(depth);
            DTRACE_CABAC_T("\ttrdepth=");
            DTRACE_CABAC_V(cu->getTransformIdx(absPartIdx));
            DTRACE_CABAC_T("\n");
        }

        if (cu->getPredictionMode(absPartIdx) != MODE_INTRA && depth == cu->getDepth(absPartIdx) && !cu->getCbf(absPartIdx, TEXT_CHROMA_U, 0) && !cu->getCbf(absPartIdx, TEXT_CHROMA_V, 0))
        {
            assert(cu->getCbf(absPartIdx, TEXT_LUMA, 0));
            //      printf( "saved one bin! " );
        }
        else
        {
            m_entropyCoderIf->codeQtCbf(cu, absPartIdx, TEXT_LUMA, cu->getTransformIdx(absPartIdx), absPartIdxStep, width, height, (subdiv == 0));
        }

        if (cbfY || cbfU || cbfV)
        {
            // dQP: only for LCU once
            if (cu->getSlice()->getPPS()->getUseDQP())
            {
                if (bCodeDQP)
                {
                    encodeQP(cu, m_bakAbsPartIdxCU);
                    bCodeDQP = false;
                }
            }
        }
        if (cbfY)
        {
            m_entropyCoderIf->codeCoeffNxN(cu, (cu->getCoeffY() + offsetLuma), absPartIdx, width, depth, TEXT_LUMA);
        }

        int chFmt = cu->getChromaFormat();
        if ((log2TrafoSize == 2) && !(chFmt == CHROMA_444))
        {
            uint32_t partNum = cu->getPic()->getNumPartInCU() >> ((depth - 1) << 1);
            if ((absPartIdx % partNum) == (partNum - 1))
            {
                uint32_t trWidthC          = log2TrafoSize << 1;
                const bool splitIntoSubTUs = (chFmt == CHROMA_422);

                uint32_t curPartNum = cu->getPic()->getNumPartInCU() >> ((depth - 1) << 1);

                for (uint32_t chromaId = TEXT_CHROMA; chromaId < MAX_NUM_COMPONENT; chromaId++)
                {
                    TComTURecurse tuIterator;
                    initTUEntropySection(&tuIterator, splitIntoSubTUs ? VERTICAL_SPLIT : DONT_SPLIT, curPartNum, m_bakAbsPartIdx);
                    coeff_t* coeffChroma = (chromaId == 1) ? cu->getCoeffCb() : cu->getCoeffCr();
                    do
                    {
                        uint32_t cbf = cu->getCbf(tuIterator.m_absPartIdxTURelCU, (TextType)chromaId, trIdx);
                        uint32_t subTUIndex = tuIterator.m_section * trWidthC * trWidthC;
                        if (cbf)
                        {
                            m_entropyCoderIf->codeCoeffNxN(cu, (coeffChroma + m_bakChromaOffset + subTUIndex), tuIterator.m_absPartIdxTURelCU, trWidthC, depth, (TextType)chromaId);
                        }
                    }
                    while (isNextTUSection(&tuIterator));
                }
            }
        }
        else
        {
            uint32_t trWidthC  = width  >> hChromaShift;
            uint32_t trHeightC = height >> vChromaShift;
            const bool splitIntoSubTUs = (chFmt == CHROMA_422);
            trHeightC = splitIntoSubTUs ? trHeightC >> 1 : trHeightC;
            uint32_t curPartNum = cu->getPic()->getNumPartInCU() >> (depth << 1);
            for (uint32_t chromaId = TEXT_CHROMA; chromaId < MAX_NUM_COMPONENT; chromaId++)
            {
                TComTURecurse tuIterator;
                initTUEntropySection(&tuIterator, splitIntoSubTUs ? VERTICAL_SPLIT : DONT_SPLIT, curPartNum, absPartIdx);
                coeff_t* coeffChroma = (chromaId == 1) ? cu->getCoeffCb() : cu->getCoeffCr();
                do
                {
                    uint32_t cbf = cu->getCbf(tuIterator.m_absPartIdxTURelCU, (TextType)chromaId, trIdx);
                    uint32_t subTUIndex = tuIterator.m_section * trWidthC * trHeightC;
                    if (cbf)
                    {
                        m_entropyCoderIf->codeCoeffNxN(cu, (coeffChroma + offsetChroma + subTUIndex), tuIterator.m_absPartIdxTURelCU, trWidthC, depth, (TextType)chromaId);
                    }
                }
                while (isNextTUSection(&tuIterator));
            }
        }
    }
}

// Intra direction for Luma
void TEncEntropy::encodeIntraDirModeLuma(TComDataCU* cu, uint32_t absPartIdx, bool isMultiplePU)
{
    m_entropyCoderIf->codeIntraDirLumaAng(cu, absPartIdx, isMultiplePU);
}

// Intra direction for Chroma
void TEncEntropy::encodeIntraDirModeChroma(TComDataCU* cu, uint32_t absPartIdx)
{
    m_entropyCoderIf->codeIntraDirChroma(cu, absPartIdx);
}

void TEncEntropy::encodePredInfo(TComDataCU* cu, uint32_t absPartIdx)
{
    if (cu->isIntra(absPartIdx)) // If it is Intra mode, encode intra prediction mode.
    {
        encodeIntraDirModeLuma(cu, absPartIdx, true);
        int chFmt = cu->getChromaFormat();
        if (chFmt != CHROMA_400)
        {
            encodeIntraDirModeChroma(cu, absPartIdx);

            if ((chFmt == CHROMA_444) && (cu->getPartitionSize(absPartIdx) == SIZE_NxN))
            {
                uint32_t partOffset = (cu->getPic()->getNumPartInCU() >> (cu->getDepth(absPartIdx) << 1)) >> 2;
                encodeIntraDirModeChroma(cu, absPartIdx + partOffset);
                encodeIntraDirModeChroma(cu, absPartIdx + partOffset * 2);
                encodeIntraDirModeChroma(cu, absPartIdx + partOffset * 3);
            }
        }
    }
    else                        // if it is Inter mode, encode motion vector and reference index
    {
        encodePUWise(cu, absPartIdx);
    }
}

/** encode motion information for every PU block
 * \param cu
 * \param absPartIdx
 * \returns void
 */
void TEncEntropy::encodePUWise(TComDataCU* cu, uint32_t absPartIdx)
{
    PartSize partSize = cu->getPartitionSize(absPartIdx);
    uint32_t numPU = (partSize == SIZE_2Nx2N ? 1 : (partSize == SIZE_NxN ? 4 : 2));
    uint32_t depth = cu->getDepth(absPartIdx);
    uint32_t puOffset = (g_puOffset[uint32_t(partSize)] << ((cu->getSlice()->getSPS()->getMaxCUDepth() - depth) << 1)) >> 4;

    for (uint32_t partIdx = 0, subPartIdx = absPartIdx; partIdx < numPU; partIdx++, subPartIdx += puOffset)
    {
        encodeMergeFlag(cu, subPartIdx);
        if (cu->getMergeFlag(subPartIdx))
        {
            encodeMergeIndex(cu, subPartIdx);
        }
        else
        {
            uint32_t interDir = cu->getInterDir(subPartIdx);
            encodeInterDirPU(cu, subPartIdx);
            for (uint32_t refListIdx = 0; refListIdx < 2; refListIdx++)
            {
                if (interDir & (1 << refListIdx))
                {
                    assert(cu->getSlice()->getNumRefIdx(refListIdx) > 0);

                    encodeRefFrmIdxPU(cu, subPartIdx, refListIdx);
                    encodeMvdPU(cu, subPartIdx, refListIdx);
                    encodeMVPIdxPU(cu, subPartIdx, refListIdx);
                }
            }
        }
    }
}

void TEncEntropy::encodeInterDirPU(TComDataCU* cu, uint32_t absPartIdx)
{
    if (!cu->getSlice()->isInterB())
    {
        return;
    }

    m_entropyCoderIf->codeInterDir(cu, absPartIdx);
}

/** encode reference frame index for a PU block
 * \param cu
 * \param absPartIdx
 * \param eRefList
 * \returns void
 */
void TEncEntropy::encodeRefFrmIdxPU(TComDataCU* cu, uint32_t absPartIdx, int list)
{
    assert(!cu->isIntra(absPartIdx));
    {
        if ((cu->getSlice()->getNumRefIdx(list) == 1))
        {
            return;
        }

        assert(cu->getInterDir(absPartIdx) & (1 << list));
        {
            m_entropyCoderIf->codeRefFrmIdx(cu, absPartIdx, list);
        }
    }
}

/** encode motion vector difference for a PU block
 * \param cu
 * \param absPartIdx
 * \param eRefList
 * \returns void
 */
void TEncEntropy::encodeMvdPU(TComDataCU* cu, uint32_t absPartIdx, int list)
{
    assert(!cu->isIntra(absPartIdx));

    assert(cu->getInterDir(absPartIdx) & (1 << list));
    {
        m_entropyCoderIf->codeMvd(cu, absPartIdx, list);
    }
}

void TEncEntropy::encodeMVPIdxPU(TComDataCU* cu, uint32_t absPartIdx, int list)
{
    assert(cu->getInterDir(absPartIdx) & (1 << list));
    {
        m_entropyCoderIf->codeMVPIdx(cu->getMVPIdx(list, absPartIdx));
    }
}

void TEncEntropy::encodeQtCbf(TComDataCU* cu, uint32_t absPartIdx, uint32_t absPartIdxStep, uint32_t width, uint32_t height, TextType ttype, uint32_t trDepth, bool lowestLevel)
{
    m_entropyCoderIf->codeQtCbf(cu, absPartIdx, ttype, trDepth, absPartIdxStep, width, height, lowestLevel);
}

void TEncEntropy::encodeTransformSubdivFlag(uint32_t symbol, uint32_t ctx)
{
    m_entropyCoderIf->codeTransformSubdivFlag(symbol, ctx);
}

void TEncEntropy::encodeQtRootCbf(TComDataCU* cu, uint32_t absPartIdx)
{
    m_entropyCoderIf->codeQtRootCbf(cu, absPartIdx);
}

void TEncEntropy::encodeQtCbfZero(TComDataCU* cu, TextType ttype, uint32_t trDepth)
{
    m_entropyCoderIf->codeQtCbfZero(cu, ttype, trDepth);
}

void TEncEntropy::encodeQtRootCbfZero(TComDataCU* cu)
{
    m_entropyCoderIf->codeQtRootCbfZero(cu);
}

// dQP
void TEncEntropy::encodeQP(TComDataCU* cu, uint32_t absPartIdx)
{
    m_entropyCoderIf->codeDeltaQP(cu, absPartIdx);
}

// texture

/** encode coefficients
 * \param cu
 * \param absPartIdx
 * \param depth
 * \param width
 * \param height
 */
void TEncEntropy::encodeCoeff(TComDataCU* cu, uint32_t absPartIdx, uint32_t depth, uint32_t width, uint32_t height, bool& bCodeDQP)
{
    uint32_t lumaOffset   = absPartIdx << cu->getPic()->getLog2UnitSize() * 2;
    uint32_t chromaOffset = lumaOffset >> (cu->getHorzChromaShift() + cu->getVertChromaShift());

    if (cu->isIntra(absPartIdx))
    {
        DTRACE_CABAC_VL(g_nSymbolCounter++)
        DTRACE_CABAC_T("\tdecodeTransformIdx()\tCUDepth=")
        DTRACE_CABAC_V(depth)
        DTRACE_CABAC_T("\n")
    }
    else
    {
        if (!(cu->getMergeFlag(absPartIdx) && cu->getPartitionSize(absPartIdx) == SIZE_2Nx2N))
        {
            m_entropyCoderIf->codeQtRootCbf(cu, absPartIdx);
        }
        if (!cu->getQtRootCbf(absPartIdx))
        {
            return;
        }
    }

    uint32_t absPartIdxStep = cu->getPic()->getNumPartInCU() >> (depth << 1);
    xEncodeTransform(cu, lumaOffset, chromaOffset, absPartIdx, absPartIdxStep, depth, width, height, 0, bCodeDQP);
}

void TEncEntropy::encodeCoeffNxN(TComDataCU* cu, coeff_t* coeff, uint32_t absPartIdx, uint32_t trWidth, uint32_t trHeight, uint32_t depth, TextType ttype)
{
    // This is for Transform unit processing. This may be used at mode selection stage for Inter.
    if (trWidth != trHeight)
    {
        uint32_t curPartNum = cu->getPic()->getNumPartInCU() >> (depth << 1);
        TComTURecurse tuIterator;
        initTUEntropySection(&tuIterator, VERTICAL_SPLIT, curPartNum, absPartIdx);
        trHeight >>= 1;
        uint32_t subTUSize = trWidth * trHeight;

        do
        {
            m_entropyCoderIf->codeCoeffNxN(cu, coeff + tuIterator.m_section * subTUSize, tuIterator.m_absPartIdxTURelCU, trWidth, depth, ttype);
        }
        while (isNextTUSection(&tuIterator));
    }
    else
    {
        m_entropyCoderIf->codeCoeffNxN(cu, coeff, absPartIdx, trWidth, depth, ttype);
    }
}

void TEncEntropy::estimateBit(estBitsSbacStruct* estBitsSBac, int trSize, TextType ttype)
{
    ttype = ttype == TEXT_LUMA ? TEXT_LUMA : TEXT_CHROMA;

    m_entropyCoderIf->estBit(estBitsSBac, trSize, ttype);
}

/** Encode SAO Offset
 * \param  saoLcuParam SAO LCU paramters
 */
void TEncEntropy::encodeSaoOffset(SaoLcuParam* saoLcuParam, uint32_t compIdx)
{
    uint32_t symbol;
    int i;

    symbol = saoLcuParam->typeIdx + 1;
    if (compIdx != 2)
    {
        m_entropyCoderIf->codeSaoTypeIdx(symbol);
    }
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
                m_entropyCoderIf->codeSaoMaxUvlc(absOffset, offsetTh - 1);
            }

            for (i = 0; i < saoLcuParam->length; i++)
            {
                if (saoLcuParam->offset[i] != 0)
                {
                    uint32_t sign = (saoLcuParam->offset[i] < 0) ? 1 : 0;
                    m_entropyCoderIf->codeSAOSign(sign);
                }
            }

            symbol = (uint32_t)(saoLcuParam->subTypeIdx);
            m_entropyCoderIf->codeSaoUflc(5, symbol);
        }
        else if (saoLcuParam->typeIdx < 4)
        {
            m_entropyCoderIf->codeSaoMaxUvlc(saoLcuParam->offset[0], offsetTh - 1);
            m_entropyCoderIf->codeSaoMaxUvlc(saoLcuParam->offset[1], offsetTh - 1);
            m_entropyCoderIf->codeSaoMaxUvlc(-saoLcuParam->offset[2], offsetTh - 1);
            m_entropyCoderIf->codeSaoMaxUvlc(-saoLcuParam->offset[3], offsetTh - 1);
            if (compIdx != 2)
            {
                symbol = (uint32_t)(saoLcuParam->subTypeIdx);
                m_entropyCoderIf->codeSaoUflc(2, symbol);
            }
        }
    }
}

/** Encode SAO unit interleaving
* \param  rx
* \param  ry
* \param  pSaoParam
* \param  cu
* \param  iCUAddrInSlice
* \param  iCUAddrUpInSlice
* \param  bLFCrossSliceBoundaryFlag
 */
void TEncEntropy::encodeSaoUnitInterleaving(int compIdx, bool saoFlag, int rx, int ry, SaoLcuParam* saoLcuParam, int cuAddrInSlice, int cuAddrUpInSlice, int allowMergeLeft, int allowMergeUp)
{
    if (saoFlag)
    {
        if (rx > 0 && cuAddrInSlice != 0 && allowMergeLeft)
        {
            m_entropyCoderIf->codeSaoMerge(saoLcuParam->mergeLeftFlag);
        }
        else
        {
            saoLcuParam->mergeLeftFlag = 0;
        }
        if (saoLcuParam->mergeLeftFlag == 0)
        {
            if ((ry > 0) && (cuAddrUpInSlice >= 0) && allowMergeUp)
            {
                m_entropyCoderIf->codeSaoMerge(saoLcuParam->mergeUpFlag);
            }
            else
            {
                saoLcuParam->mergeUpFlag = 0;
            }
            if (!saoLcuParam->mergeUpFlag)
            {
                encodeSaoOffset(saoLcuParam, compIdx);
            }
        }
    }
}

/** encode quantization matrix
 * \param scalingList quantization matrix information
 */
void TEncEntropy::encodeScalingList(TComScalingList* scalingList)
{
    m_entropyCoderIf->codeScalingList(scalingList);
}

//! \}
