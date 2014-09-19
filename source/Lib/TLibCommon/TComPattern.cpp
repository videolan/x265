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

/** \file     TComPattern.cpp
    \brief    neighboring pixel access classes
*/

#include "frame.h"
#include "TComPattern.h"
#include "TComDataCU.h"
#include "TComRom.h"

using namespace x265;

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Public member functions (TComPattern)
// ====================================================================================================================

void TComPattern::initAdiPattern(TComDataCU* cu, uint32_t zOrderIdxInPart, uint32_t partDepth, pixel* adiBuf,
                                 pixel* refAbove, pixel* refLeft, pixel* refAboveFlt, pixel* refLeftFlt, int dirMode)
{
    pixel* roiOrigin;
    pixel* adiTemp;

    int picStride = cu->m_pic->getStride();

    IntraNeighbors intraNeighbors;

    initIntraNeighbors(cu, zOrderIdxInPart, partDepth, true, &intraNeighbors);
    uint32_t tuSize = intraNeighbors.tuSize;
    uint32_t tuSize2 = tuSize << 1;

    roiOrigin = cu->m_pic->getPicYuvRec()->getLumaAddr(cu->getAddr(), cu->getZorderIdxInCU() + zOrderIdxInPart);
    adiTemp   = adiBuf;

    fillReferenceSamples(roiOrigin, picStride, adiTemp, intraNeighbors);

    bool bUseFilteredPredictions = (dirMode == ALL_IDX ? (8 | 16 | 32) & tuSize : g_intraFilterFlags[dirMode] & tuSize);

    if (bUseFilteredPredictions)
    {
        // generate filtered intra prediction samples
        // left and left above border + above and above right border + top left corner = length of 3. filter buffer
        int bufSize = tuSize2 + tuSize2 + 1;
        uint32_t wh = ADI_BUF_STRIDE * (tuSize2 + 1);         // number of elements in one buffer

        pixel* filterBuf  = adiBuf + wh;         // buffer for 2. filtering (sequential)
        pixel* filterBufN = filterBuf + bufSize; // buffer for 1. filtering (sequential)

        int l = 0;
        // left border from bottom to top
        for (int i = 0; i < tuSize2; i++)
        {
            filterBuf[l++] = adiTemp[ADI_BUF_STRIDE * (tuSize2 - i)];
        }

        // top left corner
        filterBuf[l++] = adiTemp[0];

        // above border from left to right
        memcpy(&filterBuf[l], &adiTemp[1], tuSize2 * sizeof(*filterBuf));

        if (tuSize >= 32 && cu->m_slice->m_sps->bUseStrongIntraSmoothing)
        {
            int bottomLeft = filterBuf[0];
            int topLeft = filterBuf[tuSize2];
            int topRight = filterBuf[bufSize - 1];
            int threshold = 1 << (X265_DEPTH - 5);
            bool bilinearLeft = abs(bottomLeft + topLeft - 2 * filterBuf[tuSize]) < threshold;
            bool bilinearAbove  = abs(topLeft + topRight - 2 * filterBuf[tuSize2 + tuSize]) < threshold;

            if (bilinearLeft && bilinearAbove)
            {
                int shift = intraNeighbors.log2TrSize + 1;
                filterBufN[0] = filterBuf[0];
                filterBufN[tuSize2] = filterBuf[tuSize2];
                filterBufN[bufSize - 1] = filterBuf[bufSize - 1];
                //TODO: Performance Primitive???
                for (int i = 1; i < tuSize2; i++)
                {
                    filterBufN[i] = ((tuSize2 - i) * bottomLeft + i * topLeft + tuSize) >> shift;
                }

                for (int i = 1; i < tuSize2; i++)
                {
                    filterBufN[tuSize2 + i] = ((tuSize2 - i) * topLeft + i * topRight + tuSize) >> shift;
                }
            }
            else
            {
                // 1. filtering with [1 2 1]
                filterBufN[0] = filterBuf[0];
                filterBufN[bufSize - 1] = filterBuf[bufSize - 1];
                for (int i = 1; i < bufSize - 1; i++)
                {
                    filterBufN[i] = (filterBuf[i - 1] + 2 * filterBuf[i] + filterBuf[i + 1] + 2) >> 2;
                }
            }
        }
        else
        {
            // 1. filtering with [1 2 1]
            filterBufN[0] = filterBuf[0];
            filterBufN[bufSize - 1] = filterBuf[bufSize - 1];
            for (int i = 1; i < bufSize - 1; i++)
            {
                filterBufN[i] = (filterBuf[i - 1] + 2 * filterBuf[i] + filterBuf[i + 1] + 2) >> 2;
            }
        }

        // initialization of ADI buffers
        refAboveFlt += tuSize - 1;
        refLeftFlt += tuSize - 1;
        memcpy(refAboveFlt, filterBufN + tuSize2, (tuSize2 + 1) * sizeof(pixel));
        for (int k = 0; k < tuSize2 + 1; k++)
        {
            refLeftFlt[k] = filterBufN[tuSize2 - k];   // Smoothened
        }
    }

    // initialization of ADI buffers
    refAbove += tuSize - 1;
    refLeft += tuSize - 1;

    //  ADI_BUF_STRIDE * (2 * tuSize + 1);
    memcpy(refAbove, adiBuf, (tuSize2 + 1) * sizeof(pixel));
    for (int k = 0; k < tuSize2 + 1; k++)
    {
        refLeft[k] = adiBuf[k * ADI_BUF_STRIDE];
    }
}

void TComPattern::initAdiPatternChroma(TComDataCU* cu, uint32_t zOrderIdxInPart, uint32_t partDepth, pixel* adiBuf, uint32_t chromaId)
{
    pixel*  roiOrigin;
    pixel*  adiTemp;

    int picStride = cu->m_pic->getCStride();

    IntraNeighbors intraNeighbors;

    initIntraNeighbors(cu, zOrderIdxInPart, partDepth, false, &intraNeighbors);
    uint32_t tuSize = intraNeighbors.tuSize;

    roiOrigin = cu->m_pic->getPicYuvRec()->getChromaAddr(chromaId, cu->getAddr(), cu->getZorderIdxInCU() + zOrderIdxInPart);
    adiTemp   = getAdiChromaBuf(chromaId, tuSize, adiBuf);

    fillReferenceSamples(roiOrigin, picStride, adiTemp, intraNeighbors);
}

void TComPattern::initIntraNeighbors(TComDataCU* cu, uint32_t zOrderIdxInPart, uint32_t partDepth, bool isLuma, IntraNeighbors *intraNeighbors)
{
    uint32_t log2TrSize = cu->getLog2CUSize(0) - partDepth;
    int log2UnitWidth  = LOG2_UNIT_SIZE;
    int log2UnitHeight = LOG2_UNIT_SIZE;

    if (!isLuma)
    {
        log2TrSize     -= cu->getHorzChromaShift();
        log2UnitWidth  -= cu->getHorzChromaShift();
        log2UnitHeight -= cu->getVertChromaShift();
    }

    int   numIntraNeighbor = 0;
    bool *bNeighborFlags = intraNeighbors->bNeighborFlags;

    uint32_t partIdxLT, partIdxRT, partIdxLB;

    cu->deriveLeftRightTopIdxAdi(partIdxLT, partIdxRT, zOrderIdxInPart, partDepth);

    uint32_t tuSize  = 1 << log2TrSize;
    int  tuWidthInUnits  = tuSize >> log2UnitWidth;
    int  tuHeightInUnits = tuSize >> log2UnitHeight;
    int  aboveUnits      = tuWidthInUnits << 1;
    int  leftUnits       = tuHeightInUnits << 1;
    int  partIdxStride   = cu->m_pic->getNumPartInCUSize();
    partIdxLB            = g_rasterToZscan[g_zscanToRaster[partIdxLT] + ((tuHeightInUnits - 1) * partIdxStride)];

    bNeighborFlags[leftUnits] = isAboveLeftAvailable(cu, partIdxLT);
    numIntraNeighbor += (int)(bNeighborFlags[leftUnits]);
    numIntraNeighbor += isAboveAvailable(cu, partIdxLT, partIdxRT, (bNeighborFlags + leftUnits + 1));
    numIntraNeighbor += isAboveRightAvailable(cu, partIdxLT, partIdxRT, (bNeighborFlags + leftUnits + 1 + tuWidthInUnits));
    numIntraNeighbor += isLeftAvailable(cu, partIdxLT, partIdxLB, (bNeighborFlags + leftUnits - 1));
    numIntraNeighbor += isBelowLeftAvailable(cu, partIdxLT, partIdxLB, (bNeighborFlags + leftUnits   - 1 - tuHeightInUnits));

    intraNeighbors->numIntraNeighbor = numIntraNeighbor;
    intraNeighbors->totalUnits       = aboveUnits + leftUnits + 1;
    intraNeighbors->aboveUnits       = aboveUnits;
    intraNeighbors->leftUnits        = leftUnits;
    intraNeighbors->unitWidth        = 1 << log2UnitWidth;
    intraNeighbors->unitHeight       = 1 << log2UnitHeight;
    intraNeighbors->tuSize           = tuSize;
    intraNeighbors->log2TrSize       = log2TrSize;
}

void TComPattern::fillReferenceSamples(pixel* roiOrigin, int picStride, pixel* adiTemp, const IntraNeighbors& intraNeighbors)
{
    int numIntraNeighbor = intraNeighbors.numIntraNeighbor;
    int totalUnits       = intraNeighbors.totalUnits;
    uint32_t tuSize      = intraNeighbors.tuSize;

    uint32_t refSize = tuSize * 2 + 1;
    pixel* roiTemp;
    int  i, j;
    int  dcValue = 1 << (X265_DEPTH - 1);

    if (numIntraNeighbor == 0)
    {
        // Fill border with DC value
        for (i = 0; i < refSize; i++)
        {
            adiTemp[i] = dcValue;
        }

        for (i = 1; i < refSize; i++)
        {
            adiTemp[i * ADI_BUF_STRIDE] = dcValue;
        }
    }
    else if (numIntraNeighbor == totalUnits)
    {
        // Fill top border with rec. samples
        roiTemp = roiOrigin - picStride - 1;
        memcpy(adiTemp, roiTemp, refSize * sizeof(*adiTemp));

        // Fill left border with rec. samples
        roiTemp = roiOrigin - 1;
        for (i = 1; i < refSize; i++)
        {
            adiTemp[i * ADI_BUF_STRIDE] = roiTemp[0];
            roiTemp += picStride;
        }
    }
    else // reference samples are partially available
    {
        const bool *bNeighborFlags = intraNeighbors.bNeighborFlags;
        int aboveUnits       = intraNeighbors.aboveUnits;
        int leftUnits        = intraNeighbors.leftUnits;
        int unitWidth        = intraNeighbors.unitWidth;
        int unitHeight       = intraNeighbors.unitHeight;
        int  totalSamples = (leftUnits * unitHeight) + ((aboveUnits + 1) * unitWidth);
        pixel pAdiLine[5 * MAX_CU_SIZE];
        pixel *pAdiLineTemp;
        const bool  *pNeighborFlags;
        int   next, curr;

        // Initialize
        for (i = 0; i < totalSamples; i++)
        {
            pAdiLine[i] = dcValue;
        }

        // Fill top-left sample
        roiTemp = roiOrigin - picStride - 1;
        pAdiLineTemp = pAdiLine + (leftUnits * unitHeight);
        pNeighborFlags = bNeighborFlags + leftUnits;
        if (*pNeighborFlags)
        {
            pixel topLeftVal = roiTemp[0];
            for (i = 0; i < unitWidth; i++)
            {
                pAdiLineTemp[i] = topLeftVal;
            }
        }

        // Fill left & below-left samples
        roiTemp += picStride;
        pAdiLineTemp--;
        pNeighborFlags--;
        for (j = 0; j < leftUnits; j++)
        {
            if (*pNeighborFlags)
            {
                for (i = 0; i < unitHeight; i++)
                {
                    pAdiLineTemp[-i] = roiTemp[i * picStride];
                }
            }
            roiTemp += unitHeight * picStride;
            pAdiLineTemp -= unitHeight;
            pNeighborFlags--;
        }

        // Fill above & above-right samples
        roiTemp = roiOrigin - picStride;
        pAdiLineTemp = pAdiLine + (leftUnits * unitHeight) + unitWidth;
        pNeighborFlags = bNeighborFlags + leftUnits + 1;
        for (j = 0; j < aboveUnits; j++)
        {
            if (*pNeighborFlags)
            {
                memcpy(pAdiLineTemp, roiTemp, unitWidth * sizeof(*adiTemp));
            }
            roiTemp += unitWidth;
            pAdiLineTemp += unitWidth;
            pNeighborFlags++;
        }

        // Pad reference samples when necessary
        curr = 0;
        next = 1;
        pAdiLineTemp = pAdiLine;
        int pAdiLineTopRowOffset = leftUnits * (unitHeight - unitWidth);
        if (!bNeighborFlags[0])
        {
            // very bottom unit of bottom-left; at least one unit will be valid.
            while (next < totalUnits && !bNeighborFlags[next])
            {
                next++;
            }

            pixel *pAdiLineNext = pAdiLine + ((next < leftUnits) ? (next * unitHeight) : (pAdiLineTopRowOffset + (next * unitWidth)));
            const pixel refSample = *pAdiLineNext;
            // Pad unavailable samples with new value
            int nextOrTop = std::min<int>(next, leftUnits);
            // fill left column
            while (curr < nextOrTop)
            {
                for (i = 0; i < unitHeight; i++)
                {
                    pAdiLineTemp[i] = refSample;
                }

                pAdiLineTemp += unitHeight;
                curr++;
            }

            // fill top row
            while (curr < next)
            {
                for (i = 0; i < unitWidth; i++)
                {
                    pAdiLineTemp[i] = refSample;
                }

                pAdiLineTemp += unitWidth;
                curr++;
            }
        }

        // pad all other reference samples.
        while (curr < totalUnits)
        {
            if (!bNeighborFlags[curr]) // samples not available
            {
                int numSamplesInCurrUnit = (curr >= leftUnits) ? unitWidth : unitHeight;
                const pixel refSample = *(pAdiLineTemp - 1);
                for (i = 0; i < numSamplesInCurrUnit; i++)
                {
                    pAdiLineTemp[i] = refSample;
                }

                pAdiLineTemp += numSamplesInCurrUnit;
                curr++;
            }
            else
            {
                pAdiLineTemp += (curr >= leftUnits) ? unitWidth : unitHeight;
                curr++;
            }
        }

        // Copy processed samples
        pAdiLineTemp = pAdiLine + refSize + unitWidth - 2;
        memcpy(adiTemp, pAdiLineTemp, refSize * sizeof(*adiTemp));

        pAdiLineTemp = pAdiLine + refSize - 1;
        for (i = 1; i < refSize; i++)
        {
            adiTemp[i * ADI_BUF_STRIDE] = pAdiLineTemp[-i];
        }
    }
}

bool TComPattern::isAboveLeftAvailable(TComDataCU* cu, uint32_t partIdxLT)
{
    uint32_t partAboveLeft;
    TComDataCU* pcCUAboveLeft = cu->getPUAboveLeft(partAboveLeft, partIdxLT);

    if (!cu->m_slice->m_pps->bConstrainedIntraPred)
        return pcCUAboveLeft ? true : false;
    else
        return pcCUAboveLeft && pcCUAboveLeft->isIntra(partAboveLeft);
}

int TComPattern::isAboveAvailable(TComDataCU* cu, uint32_t partIdxLT, uint32_t partIdxRT, bool *bValidFlags)
{
    const uint32_t rasterPartBegin = g_zscanToRaster[partIdxLT];
    const uint32_t rasterPartEnd = g_zscanToRaster[partIdxRT] + 1;
    const uint32_t idxStep = 1;
    bool *validFlagPtr = bValidFlags;
    int numIntra = 0;

    for (uint32_t rasterPart = rasterPartBegin; rasterPart < rasterPartEnd; rasterPart += idxStep)
    {
        uint32_t uiPartAbove;
        TComDataCU* pcCUAbove = cu->getPUAbove(uiPartAbove, g_rasterToZscan[rasterPart]);
        if (pcCUAbove && (!cu->m_slice->m_pps->bConstrainedIntraPred || pcCUAbove->isIntra(uiPartAbove)))
        {
            numIntra++;
            *validFlagPtr = true;
        }
        else
        {
            *validFlagPtr = false;
        }
        validFlagPtr++;
    }

    return numIntra;
}

int TComPattern::isLeftAvailable(TComDataCU* cu, uint32_t partIdxLT, uint32_t partIdxLB, bool *bValidFlags)
{
    const uint32_t rasterPartBegin = g_zscanToRaster[partIdxLT];
    const uint32_t rasterPartEnd = g_zscanToRaster[partIdxLB] + 1;
    const uint32_t idxStep = cu->m_pic->getNumPartInCUSize();
    bool *validFlagPtr = bValidFlags;
    int numIntra = 0;

    for (uint32_t rasterPart = rasterPartBegin; rasterPart < rasterPartEnd; rasterPart += idxStep)
    {
        uint32_t partLeft;
        TComDataCU* pcCULeft = cu->getPULeft(partLeft, g_rasterToZscan[rasterPart]);
        if (pcCULeft && (!cu->m_slice->m_pps->bConstrainedIntraPred || pcCULeft->isIntra(partLeft)))
        {
            numIntra++;
            *validFlagPtr = true;
        }
        else
        {
            *validFlagPtr = false;
        }
        validFlagPtr--; // opposite direction
    }

    return numIntra;
}

int TComPattern::isAboveRightAvailable(TComDataCU* cu, uint32_t partIdxLT, uint32_t partIdxRT, bool *bValidFlags)
{
    const uint32_t numUnitsInPU = g_zscanToRaster[partIdxRT] - g_zscanToRaster[partIdxLT] + 1;
    bool *validFlagPtr = bValidFlags;
    int numIntra = 0;

    for (uint32_t offset = 1; offset <= numUnitsInPU; offset++)
    {
        uint32_t uiPartAboveRight;
        TComDataCU* pcCUAboveRight = cu->getPUAboveRightAdi(uiPartAboveRight, partIdxRT, offset);
        if (pcCUAboveRight && (!cu->m_slice->m_pps->bConstrainedIntraPred || pcCUAboveRight->isIntra(uiPartAboveRight)))
        {
            numIntra++;
            *validFlagPtr = true;
        }
        else
        {
            *validFlagPtr = false;
        }
        validFlagPtr++;
    }

    return numIntra;
}

int TComPattern::isBelowLeftAvailable(TComDataCU* cu, uint32_t partIdxLT, uint32_t partIdxLB, bool *bValidFlags)
{
    const uint32_t numUnitsInPU = (g_zscanToRaster[partIdxLB] - g_zscanToRaster[partIdxLT]) / cu->m_pic->getNumPartInCUSize() + 1;
    bool *validFlagPtr = bValidFlags;
    int numIntra = 0;

    for (uint32_t offset = 1; offset <= numUnitsInPU; offset++)
    {
        uint32_t uiPartBelowLeft;
        TComDataCU* pcCUBelowLeft = cu->getPUBelowLeftAdi(uiPartBelowLeft, partIdxLB, offset);
        if (pcCUBelowLeft && (!cu->m_slice->m_pps->bConstrainedIntraPred || pcCUBelowLeft->isIntra(uiPartBelowLeft)))
        {
            numIntra++;
            *validFlagPtr = true;
        }
        else
        {
            *validFlagPtr = false;
        }
        validFlagPtr--; // opposite direction
    }

    return numIntra;
}

//! \}
