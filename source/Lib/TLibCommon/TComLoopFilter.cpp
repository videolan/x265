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

/** \file     TComLoopFilter.cpp
    \brief    deblocking filter
*/

#include "common.h"
#include "TComLoopFilter.h"
#include "slice.h"
#include "mv.h"

using namespace x265;

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Constants
// ====================================================================================================================
#define QpUV(iQpY, chFmt)  (((iQpY) < 0) ? (iQpY) : (((iQpY) > 57) ? ((iQpY) - 6) : g_chromaScale[chFmt][(iQpY)]))
#define DEFAULT_INTRA_TC_OFFSET 2 ///< Default intra TC offset

// ====================================================================================================================
// Tables
// ====================================================================================================================

const uint8_t TComLoopFilter::sm_tcTable[54] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 8, 9, 10, 11, 13, 14, 16, 18, 20, 22, 24
};

const uint8_t TComLoopFilter::sm_betaTable[52] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64
};

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TComLoopFilter::TComLoopFilter()
    : m_numPartitions(0)
{
}

TComLoopFilter::~TComLoopFilter()
{}

void TComLoopFilter::create(uint32_t maxCuDepth)
{
    destroy();
    m_numPartitions = 1 << (maxCuDepth << 1);
}

void TComLoopFilter::destroy()
{
}

void TComLoopFilter::loopFilterCU(TComDataCU* cu, int dir, bool edgeFilter[], uint8_t blockingStrength[])
{
    ::memset(blockingStrength, 0, sizeof(uint8_t) * m_numPartitions);
    ::memset(edgeFilter, 0, sizeof(bool) * m_numPartitions);

    // CU-based deblocking
    xDeblockCU(cu, 0, 0, dir, edgeFilter, blockingStrength);
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

/**
 - Deblocking filter process in CU-based (the same function as conventional's)
 .
 \param Edge          the direction of the edge in block boundary (horizonta/vertical), which is added newly
*/
void TComLoopFilter::xDeblockCU(TComDataCU* cu, uint32_t absZOrderIdx, uint32_t depth, const int dir, bool edgeFilter[], uint8_t blockingStrength[])
{
    if (cu->m_pic == 0 || cu->getPartitionSize(absZOrderIdx) == SIZE_NONE)
    {
        return;
    }
    Frame* pic = cu->m_pic;
    uint32_t curNumParts = pic->getNumPartInCU() >> (depth << 1);
    uint32_t qNumParts   = curNumParts >> 2;

    if (cu->getDepth(absZOrderIdx) > depth)
    {
        for (uint32_t partIdx = 0; partIdx < 4; partIdx++, absZOrderIdx += qNumParts)
        {
            uint32_t lpelx = cu->getCUPelX() + g_rasterToPelX[g_zscanToRaster[absZOrderIdx]];
            uint32_t tpely = cu->getCUPelY() + g_rasterToPelY[g_zscanToRaster[absZOrderIdx]];
            if ((lpelx < cu->m_slice->m_sps->picWidthInLumaSamples) && (tpely < cu->m_slice->m_sps->picHeightInLumaSamples))
            {
                xDeblockCU(cu, absZOrderIdx, depth + 1, dir, edgeFilter, blockingStrength);
            }
        }

        return;
    }

    ///< status structure
    LFCUParam lfcuParam;
    xSetLoopfilterParam(cu, absZOrderIdx, &lfcuParam);

    xSetEdgefilterTU(cu, absZOrderIdx, absZOrderIdx, depth, dir, edgeFilter, blockingStrength);
    xSetEdgefilterPU(cu, absZOrderIdx, dir, &lfcuParam, edgeFilter, blockingStrength);

    for (uint32_t partIdx = absZOrderIdx; partIdx < absZOrderIdx + curNumParts; partIdx++)
    {
        uint32_t bsCheck;
        if (g_log2UnitSize == 2)
        {
            bsCheck = (dir == EDGE_VER && (partIdx & 1) == 0) || (dir == EDGE_HOR && (partIdx & 2) == 0);
        }
        else
        {
            bsCheck = 1;
        }

        if (edgeFilter[partIdx] && bsCheck)
        {
            xGetBoundaryStrengthSingle(cu, dir, partIdx, blockingStrength);
        }
    }

    uint32_t log2UnitSize = g_log2UnitSize;
    uint32_t partIdxIncr = (DEBLOCK_SMALLEST_BLOCK >> log2UnitSize) ? (DEBLOCK_SMALLEST_BLOCK >> log2UnitSize) : 1;

    uint32_t sizeInPU = pic->getNumPartInCUSize() >> (depth);
    uint32_t shiftFactor = (dir == EDGE_VER) ? cu->getHorzChromaShift() : cu->getVertChromaShift();
    const bool bAlwaysDoChroma = (cu->getChromaFormat() == CHROMA_444 || (1 << log2UnitSize) > DEBLOCK_SMALLEST_BLOCK);
    for (uint32_t e = 0; e < sizeInPU; e += partIdxIncr)
    {
        xEdgeFilterLuma(cu, absZOrderIdx, depth, dir, e, blockingStrength);
        if (bAlwaysDoChroma || (e % ((DEBLOCK_SMALLEST_BLOCK << shiftFactor) >> log2UnitSize)) == 0)
        {
            xEdgeFilterChroma(cu, absZOrderIdx, depth, dir, e, blockingStrength);
        }
    }
}

void TComLoopFilter::xSetEdgefilterMultiple(TComDataCU* cu, uint32_t scanIdx, uint32_t depth, int dir, int edgeIdx, bool bValue, bool edgeFilter[], uint8_t blockingStrength[], uint32_t widthInBaseUnits)
{
    if (widthInBaseUnits == 0)
    {
        widthInBaseUnits  = cu->m_pic->getNumPartInCUSize() >> depth;
    }
    const uint32_t numElem = widthInBaseUnits;
    X265_CHECK(numElem > 0, "numElem edge filter check\n");
    for (uint32_t i = 0; i < numElem; i++)
    {
        const uint32_t bsidx = xCalcBsIdx(cu, scanIdx, dir, edgeIdx, i);
        edgeFilter[bsidx] = bValue;
        if (edgeIdx == 0)
        {
            blockingStrength[bsidx] = bValue;
        }
    }
}

void TComLoopFilter::xSetEdgefilterTU(TComDataCU* cu, uint32_t absTUPartIdx, uint32_t absZOrderIdx, uint32_t depth, int dir, bool edgeFilter[], uint8_t blockingStrength[])
{
    if (cu->getTransformIdx(absZOrderIdx) + cu->getDepth(absZOrderIdx) > depth)
    {
        const uint32_t curNumParts = cu->m_pic->getNumPartInCU() >> (depth << 1);
        const uint32_t qNumParts   = curNumParts >> 2;
        for (uint32_t partIdx = 0; partIdx < 4; partIdx++, absZOrderIdx += qNumParts)
        {
            uint32_t nsAddr = absZOrderIdx;
            xSetEdgefilterTU(cu, nsAddr, absZOrderIdx, depth + 1, dir, edgeFilter, blockingStrength);
        }

        return;
    }

    uint32_t widthInBaseUnits  = 1 << (cu->getLog2CUSize(absZOrderIdx) - cu->getTransformIdx(absZOrderIdx) - g_log2UnitSize);

    xSetEdgefilterMultiple(cu, absTUPartIdx, depth, dir, 0, true, edgeFilter, blockingStrength, widthInBaseUnits);
}

void TComLoopFilter::xSetEdgefilterPU(TComDataCU* cu, uint32_t absZOrderIdx, int dir, LFCUParam *lfcuParam, bool edgeFilter[], uint8_t blockingStrength[])
{
    const uint32_t depth = cu->getDepth(absZOrderIdx);
    const uint32_t widthInBaseUnits  = cu->m_pic->getNumPartInCUSize() >> depth;
    const uint32_t hWidthInBaseUnits  = widthInBaseUnits  >> 1;
    const uint32_t qWidthInBaseUnits  = widthInBaseUnits  >> 2;

    xSetEdgefilterMultiple(cu, absZOrderIdx, depth, dir, 0, (dir == EDGE_VER ? lfcuParam->bLeftEdge : lfcuParam->bTopEdge), edgeFilter, blockingStrength);

    int mode = cu->getPartitionSize(absZOrderIdx);
    switch (mode)
    {
    case SIZE_2Nx2N:
    {
        break;
    }
    case SIZE_2NxN:
    case SIZE_Nx2N:
    {
        const int realDir = (mode == SIZE_2NxN ? EDGE_HOR : EDGE_VER);

        if (realDir == dir)
            xSetEdgefilterMultiple(cu, absZOrderIdx, depth, dir, hWidthInBaseUnits, true, edgeFilter, blockingStrength);
        break;
    }
    case SIZE_NxN:
    {
        xSetEdgefilterMultiple(cu, absZOrderIdx, depth, dir, hWidthInBaseUnits, true, edgeFilter, blockingStrength);
        break;
    }
    case SIZE_2NxnU:
    case SIZE_nLx2N:
    {
        const int realDir = (mode == SIZE_2NxnU ? EDGE_HOR : EDGE_VER);

        if (realDir == dir)
            xSetEdgefilterMultiple(cu, absZOrderIdx, depth, dir, qWidthInBaseUnits, true, edgeFilter, blockingStrength);
        break;
    }
    case SIZE_2NxnD:
    case SIZE_nRx2N:
    {
        const int realDir = (mode == SIZE_2NxnD ? EDGE_HOR : EDGE_VER);

        if (realDir == dir)
            xSetEdgefilterMultiple(cu, absZOrderIdx, depth, dir, widthInBaseUnits - qWidthInBaseUnits, true, edgeFilter, blockingStrength);
        break;
    }
    default:
    {
        break;
    }
    }
}

void TComLoopFilter::xSetLoopfilterParam(TComDataCU* cu, uint32_t absZOrderIdx, LFCUParam *lfcuParam)
{
    uint32_t x = cu->getCUPelX() + g_rasterToPelX[g_zscanToRaster[absZOrderIdx]];
    uint32_t y = cu->getCUPelY() + g_rasterToPelY[g_zscanToRaster[absZOrderIdx]];

    TComDataCU* tempCU;
    uint32_t    tempPartIdx;

    if (x == 0)
    {
        lfcuParam->bLeftEdge = false;
    }
    else
    {
        tempCU = cu->getPULeft(tempPartIdx, absZOrderIdx);
        if (tempCU)
        {
            lfcuParam->bLeftEdge = true;
        }
        else
        {
            lfcuParam->bLeftEdge = false;
        }
    }

    if (y == 0)
    {
        lfcuParam->bTopEdge = false;
    }
    else
    {
        tempCU = cu->getPUAbove(tempPartIdx, absZOrderIdx);
        if (tempCU)
        {
            lfcuParam->bTopEdge = true;
        }
        else
        {
            lfcuParam->bTopEdge = false;
        }
    }
}

void TComLoopFilter::xGetBoundaryStrengthSingle(TComDataCU* cu, int dir, uint32_t absPartIdx, uint8_t blockingStrength[])
{
    Slice* const slice = cu->m_slice;

    const uint32_t partQ = absPartIdx;
    TComDataCU* const cuQ = cu;

    uint32_t partP;
    TComDataCU* cuP;
    uint32_t bs = 0;

    //-- Calculate Block Index
    if (dir == EDGE_VER)
    {
        cuP = cuQ->getPULeft(partP, partQ);
    }
    else // (dir == EDGE_HOR)
    {
        cuP = cuQ->getPUAbove(partP, partQ);
    }

    //-- Set BS for Intra MB : BS = 4 or 3
    if (cuP->isIntra(partP) || cuQ->isIntra(partQ))
    {
        bs = 2;
    }

    //-- Set BS for not Intra MB : BS = 2 or 1 or 0
    if (!cuP->isIntra(partP) && !cuQ->isIntra(partQ))
    {
        uint32_t nsPartQ = partQ;
        uint32_t nsPartP = partP;

        if (blockingStrength[absPartIdx] && (cuQ->getCbf(nsPartQ, TEXT_LUMA, cuQ->getTransformIdx(nsPartQ)) != 0 || cuP->getCbf(nsPartP, TEXT_LUMA, cuP->getTransformIdx(nsPartP)) != 0))
        {
            bs = 1;
        }
        else
        {
            if (dir == EDGE_HOR)
            {
                cuP = cuQ->getPUAbove(partP, partQ);
            }
            if (slice->isInterB() || cuP->m_slice->isInterB())
            {
                int refIdx;
                Frame *refP0, *refP1, *refQ0, *refQ1;
                refIdx = cuP->getCUMvField(REF_PIC_LIST_0)->getRefIdx(partP);
                refP0 = (refIdx < 0) ? NULL : cuP->m_slice->m_refPicList[0][refIdx];
                refIdx = cuP->getCUMvField(REF_PIC_LIST_1)->getRefIdx(partP);
                refP1 = (refIdx < 0) ? NULL : cuP->m_slice->m_refPicList[1][refIdx];
                refIdx = cuQ->getCUMvField(REF_PIC_LIST_0)->getRefIdx(partQ);
                refQ0 = (refIdx < 0) ? NULL : slice->m_refPicList[0][refIdx];
                refIdx = cuQ->getCUMvField(REF_PIC_LIST_1)->getRefIdx(partQ);
                refQ1 = (refIdx < 0) ? NULL : slice->m_refPicList[1][refIdx];

                MV mvp0 = cuP->getCUMvField(REF_PIC_LIST_0)->getMv(partP);
                MV mvp1 = cuP->getCUMvField(REF_PIC_LIST_1)->getMv(partP);
                MV mvq0 = cuQ->getCUMvField(REF_PIC_LIST_0)->getMv(partQ);
                MV mvq1 = cuQ->getCUMvField(REF_PIC_LIST_1)->getMv(partQ);

                if (refP0 == NULL) mvp0 = 0;
                if (refP1 == NULL) mvp1 = 0;
                if (refQ0 == NULL) mvq0 = 0;
                if (refQ1 == NULL) mvq1 = 0;

                if (((refP0 == refQ0) && (refP1 == refQ1)) || ((refP0 == refQ1) && (refP1 == refQ0)))
                {
                    if (refP0 != refP1) // Different L0 & L1
                    {
                        if (refP0 == refQ0)
                        {
                            bs  = ((abs(mvq0.x - mvp0.x) >= 4) ||
                                   (abs(mvq0.y - mvp0.y) >= 4) ||
                                   (abs(mvq1.x - mvp1.x) >= 4) ||
                                   (abs(mvq1.y - mvp1.y) >= 4)) ? 1 : 0;
                        }
                        else
                        {
                            bs  = ((abs(mvq1.x - mvp0.x) >= 4) ||
                                   (abs(mvq1.y - mvp0.y) >= 4) ||
                                   (abs(mvq0.x - mvp1.x) >= 4) ||
                                   (abs(mvq0.y - mvp1.y) >= 4)) ? 1 : 0;
                        }
                    }
                    else // Same L0 & L1
                    {
                        bs  = ((abs(mvq0.x - mvp0.x) >= 4) ||
                               (abs(mvq0.y - mvp0.y) >= 4) ||
                               (abs(mvq1.x - mvp1.x) >= 4) ||
                               (abs(mvq1.y - mvp1.y) >= 4)) &&
                            ((abs(mvq1.x - mvp0.x) >= 4) ||
                             (abs(mvq1.y - mvp0.y) >= 4) ||
                             (abs(mvq0.x - mvp1.x) >= 4) ||
                             (abs(mvq0.y - mvp1.y) >= 4)) ? 1 : 0;
                    }
                }
                else // for all different Ref_Idx
                {
                    bs = 1;
                }
            }
            else // slice->isInterP()
            {
                int refIdx;
                Frame *refp0, *refq0;
                refIdx = cuP->getCUMvField(REF_PIC_LIST_0)->getRefIdx(partP);
                refp0 = (refIdx < 0) ? NULL : cuP->m_slice->m_refPicList[0][refIdx];
                refIdx = cuQ->getCUMvField(REF_PIC_LIST_0)->getRefIdx(partQ);
                refq0 = (refIdx < 0) ? NULL : slice->m_refPicList[0][refIdx];
                MV mvp0 = cuP->getCUMvField(REF_PIC_LIST_0)->getMv(partP);
                MV mvq0 = cuQ->getCUMvField(REF_PIC_LIST_0)->getMv(partQ);

                if (refp0 == NULL) mvp0 = 0;
                if (refq0 == NULL) mvq0 = 0;

                bs = ((refp0 != refq0) ||
                      (abs(mvq0.x - mvp0.x) >= 4) ||
                      (abs(mvq0.y - mvp0.y) >= 4)) ? 1 : 0;
            }
        } // enf of "if( one of BCBP == 0 )"
    } // enf of "if( not Intra )"

    blockingStrength[absPartIdx] = bs;
}

void TComLoopFilter::xEdgeFilterLuma(TComDataCU* cu, uint32_t absZOrderIdx, uint32_t depth, int dir, int edge, uint8_t blockingStrength[])
{
    TComPicYuv* reconYuv = cu->m_pic->getPicYuvRec();
    pixel* src = reconYuv->getLumaAddr(cu->getAddr(), absZOrderIdx);
    pixel* tmpsrc = src;

    int stride = reconYuv->getStride();
    int qp = 0;
    int qpP = 0;
    int qpQ = 0;
    uint32_t numParts = cu->m_pic->getNumPartInCUSize() >> depth;

    uint32_t log2UnitSize = g_log2UnitSize;
    uint32_t blocksInPart = (log2UnitSize - 2) > 0 ? 1 << (log2UnitSize - 2) : 1;
    uint32_t bsAbsIdx = 0, bs = 0;
    int  offset, srcStep;

    bool  bPartPNoFilter = false;
    bool  bPartQNoFilter = false;
    uint32_t  partP = 0;
    uint32_t  partQ = 0;
    TComDataCU* cuP = cu;
    TComDataCU* cuQ = cu;
    int  betaOffsetDiv2 = cuQ->m_slice->m_pps->deblockingFilterBetaOffsetDiv2;
    int  tcOffsetDiv2 = cuQ->m_slice->m_pps->deblockingFilterTcOffsetDiv2;

    if (dir == EDGE_VER)
    {
        offset = 1;
        srcStep = stride;
        tmpsrc += (edge << log2UnitSize);
    }
    else // (dir == EDGE_HOR)
    {
        offset = stride;
        srcStep = 1;
        tmpsrc += (edge << log2UnitSize) * stride;
    }

    for (uint32_t idx = 0; idx < numParts; idx++)
    {
        uint32_t partOffset = idx << log2UnitSize;
        bsAbsIdx = xCalcBsIdx(cu, absZOrderIdx, dir, edge, idx);
        bs = blockingStrength[bsAbsIdx];
        if (bs)
        {
            qpQ = cu->getQP(bsAbsIdx);
            partQ = bsAbsIdx;
            // Derive neighboring PU index
            if (dir == EDGE_VER)
            {
                cuP = cuQ->getPULeft(partP, partQ);
            }
            else // (dir == EDGE_HOR)
            {
                cuP = cuQ->getPUAbove(partP, partQ);
            }

            qpP = cuP->getQP(partP);
            qp = (qpP + qpQ + 1) >> 1;
            int bitdepthScale = 1 << (X265_DEPTH - 8);

            int indexTC = Clip3(0, MAX_QP + DEFAULT_INTRA_TC_OFFSET, int(qp + DEFAULT_INTRA_TC_OFFSET * (bs - 1) + (tcOffsetDiv2 << 1)));
            int indexB = Clip3(0, MAX_QP, qp + (betaOffsetDiv2 << 1));

            int tc =  sm_tcTable[indexTC] * bitdepthScale;
            int beta = sm_betaTable[indexB] * bitdepthScale;
            int sideThreshold = (beta + (beta >> 1)) >> 3;
            int thrCut = tc * 10;

            for (uint32_t blkIdx = 0; blkIdx < blocksInPart; blkIdx++)
            {
                int dp0 = xCalcDP(tmpsrc + srcStep * (partOffset + blkIdx * 4 + 0), offset);
                int dq0 = xCalcDQ(tmpsrc + srcStep * (partOffset + blkIdx * 4 + 0), offset);
                int dp3 = xCalcDP(tmpsrc + srcStep * (partOffset + blkIdx * 4 + 3), offset);
                int dq3 = xCalcDQ(tmpsrc + srcStep * (partOffset + blkIdx * 4 + 3), offset);
                int d0 = dp0 + dq0;
                int d3 = dp3 + dq3;

                int dp = dp0 + dp3;
                int dq = dq0 + dq3;
                int d =  d0 + d3;

                if (cu->m_slice->m_pps->bTransquantBypassEnabled)
                {
                    // check if each of PUs is lossless coded
                    bPartPNoFilter = cuP->isLosslessCoded(partP);
                    bPartQNoFilter = cuQ->isLosslessCoded(partQ);
                }

                if (d < beta)
                {
                    bool bFilterP = (dp < sideThreshold);
                    bool bFilterQ = (dq < sideThreshold);

                    bool sw =  xUseStrongFiltering(offset, 2 * d0, beta, tc, tmpsrc + srcStep * (partOffset + blkIdx * 4 + 0))
                        && xUseStrongFiltering(offset, 2 * d3, beta, tc, tmpsrc + srcStep * (partOffset + blkIdx * 4 + 3));

                    for (int i = 0; i < DEBLOCK_SMALLEST_BLOCK / 2; i++)
                    {
                        xPelFilterLuma(tmpsrc + srcStep * (partOffset + blkIdx * 4 + i), offset, tc, sw, bPartPNoFilter, bPartQNoFilter, thrCut, bFilterP, bFilterQ);
                    }
                }
            }
        }
    }
}

void TComLoopFilter::xEdgeFilterChroma(TComDataCU* cu, uint32_t absZOrderIdx, uint32_t depth, int dir, int edge, uint8_t blockingStrength[])
{
    TComPicYuv* reconYuv = cu->m_pic->getPicYuvRec();
    int stride = reconYuv->getCStride();
    pixel* srcCb = reconYuv->getCbAddr(cu->getAddr(), absZOrderIdx);
    pixel* srcCr = reconYuv->getCrAddr(cu->getAddr(), absZOrderIdx);
    int qp = 0;
    int qpP = 0;
    int qpQ = 0;
    uint32_t log2UnitSizeH = g_log2UnitSize - cu->getHorzChromaShift();
    uint32_t log2UnitSizeV = g_log2UnitSize - cu->getVertChromaShift();
    uint32_t unitSizeChromaH = 1 << log2UnitSizeH;
    uint32_t unitSizeChromaV = 1 << log2UnitSizeV;
    int   offset, srcStep;

    const uint32_t lcuWidthInBaseUnits = cu->m_pic->getNumPartInCUSize();

    bool  bPartPNoFilter = false;
    bool  bPartQNoFilter = false;
    uint32_t  partP;
    uint32_t  partQ;
    TComDataCU* cuP;
    TComDataCU* cuQ = cu;
    int tcOffsetDiv2 = cu->m_slice->m_pps->deblockingFilterTcOffsetDiv2;

    // Vertical Position
    uint32_t edgeNumInLCUVert = g_zscanToRaster[absZOrderIdx] % lcuWidthInBaseUnits + edge;
    uint32_t edgeNumInLCUHor = g_zscanToRaster[absZOrderIdx] / lcuWidthInBaseUnits + edge;

    if ((unitSizeChromaH < DEBLOCK_SMALLEST_BLOCK) && (unitSizeChromaV < DEBLOCK_SMALLEST_BLOCK) &&
        (((edgeNumInLCUVert % (DEBLOCK_SMALLEST_BLOCK >> log2UnitSizeH)) && (dir == 0)) ||
         ((edgeNumInLCUHor % (DEBLOCK_SMALLEST_BLOCK >> log2UnitSizeV)) && dir)))
    {
        return;
    }

    uint32_t numParts = cu->m_pic->getNumPartInCUSize() >> depth;

    uint32_t bsAbsIdx;
    uint8_t bs;

    pixel* tmpSrcCb = srcCb;
    pixel* tmpSrcCr = srcCr;
    uint32_t loopLength;

    if (dir == EDGE_VER)
    {
        offset     = 1;
        srcStep    = stride;
        tmpSrcCb   += (edge << log2UnitSizeH);
        tmpSrcCr   += (edge << log2UnitSizeH);
        loopLength = unitSizeChromaV;
    }
    else // (dir == EDGE_HOR)
    {
        offset     = stride;
        srcStep    = 1;
        tmpSrcCb   += edge * stride << log2UnitSizeV;
        tmpSrcCr   += edge * stride << log2UnitSizeV;
        loopLength = unitSizeChromaH;
    }

    for (uint32_t idx = 0; idx < numParts; idx++)
    {
        bsAbsIdx = xCalcBsIdx(cu, absZOrderIdx, dir, edge, idx);
        bs = blockingStrength[bsAbsIdx];

        if (bs > 1)
        {
            qpQ = cu->getQP(bsAbsIdx);
            partQ = bsAbsIdx;
            // Derive neighboring PU index
            if (dir == EDGE_VER)
            {
                cuP = cuQ->getPULeft(partP, partQ);
            }
            else // (dir == EDGE_HOR)
            {
                cuP = cuQ->getPUAbove(partP, partQ);
            }

            qpP = cuP->getQP(partP);

            if (cu->m_slice->m_pps->bTransquantBypassEnabled)
            {
                // check if each of PUs is lossless coded
                bPartPNoFilter = cuP->isLosslessCoded(partP);
                bPartQNoFilter = cuQ->isLosslessCoded(partQ);
            }

            for (uint32_t chromaIdx = 0; chromaIdx < 2; chromaIdx++)
            {
                int chromaQPOffset  = (chromaIdx == 0) ? cu->m_slice->m_pps->chromaCbQpOffset : cu->m_slice->m_pps->chromaCrQpOffset;
                pixel* piTmpSrcChroma = (chromaIdx == 0) ? tmpSrcCb : tmpSrcCr;
                qp = QpUV((((qpP + qpQ + 1) >> 1) + chromaQPOffset), cu->getChromaFormat());
                int iBitdepthScale = 1 << (X265_DEPTH - 8);

                int iIndexTC = Clip3(0, MAX_QP + DEFAULT_INTRA_TC_OFFSET, qp + DEFAULT_INTRA_TC_OFFSET * (bs - 1) + (tcOffsetDiv2 << 1));
                int iTc =  sm_tcTable[iIndexTC] * iBitdepthScale;

                for (uint32_t uiStep = 0; uiStep < loopLength; uiStep++)
                {
                    xPelFilterChroma(piTmpSrcChroma + srcStep * (uiStep + idx * loopLength), offset, iTc, bPartPNoFilter, bPartQNoFilter);
                }
            }
        }
    }
}

/**
 - Deblocking for the luminance component with strong or weak filter
 .
 \param src           pointer to picture data
 \param offset         offset value for picture data
 \param tc              tc value
 \param sw              decision strong/weak filter
 \param bPartPNoFilter  indicator to disable filtering on partP
 \param bPartQNoFilter  indicator to disable filtering on partQ
 \param iThrCut         threshold value for weak filter decision
 \param bFilterSecondP  decision weak filter/no filter for partP
 \param bFilterSecondQ  decision weak filter/no filter for partQ
*/
inline void TComLoopFilter::xPelFilterLuma(pixel* src, int offset, int tc, bool sw, bool bPartPNoFilter, bool bPartQNoFilter, int thrCut, bool bFilterSecondP, bool bFilterSecondQ)
{
    int delta;

    int16_t m4  = (int16_t)src[0];
    int16_t m3  = (int16_t)src[-offset];
    int16_t m5  = (int16_t)src[offset];
    int16_t m2  = (int16_t)src[-offset * 2];
    int16_t m6  = (int16_t)src[offset * 2];
    int16_t m1  = (int16_t)src[-offset * 3];
    int16_t m7  = (int16_t)src[offset * 3];
    int16_t m0  = (int16_t)src[-offset * 4];

    if (sw)
    {
        src[-offset]     = (pixel)Clip3(m3 - 2 * tc, m3 + 2 * tc, ((m1 + 2 * m2 + 2 * m3 + 2 * m4 + m5 + 4) >> 3));
        src[0]           = (pixel)Clip3(m4 - 2 * tc, m4 + 2 * tc, ((m2 + 2 * m3 + 2 * m4 + 2 * m5 + m6 + 4) >> 3));
        src[-offset * 2] = (pixel)Clip3(m2 - 2 * tc, m2 + 2 * tc, ((m1 + m2 + m3 + m4 + 2) >> 2));
        src[offset]      = (pixel)Clip3(m5 - 2 * tc, m5 + 2 * tc, ((m3 + m4 + m5 + m6 + 2) >> 2));
        src[-offset * 3] = (pixel)Clip3(m1 - 2 * tc, m1 + 2 * tc, ((2 * m0 + 3 * m1 + m2 + m3 + m4 + 4) >> 3));
        src[offset * 2]  = (pixel)Clip3(m6 - 2 * tc, m6 + 2 * tc, ((m3 + m4 + m5 + 3 * m6 + 2 * m7 + 4) >> 3));
    }
    else
    {
        /* Weak filter */
        delta = (9 * (m4 - m3) - 3 * (m5 - m2) + 8) >> 4;

        if (abs(delta) < thrCut)
        {
            delta = Clip3(-tc, tc, delta);
            src[-offset] = Clip(m3 + delta);
            src[0] = Clip(m4 - delta);

            int tc2 = tc >> 1;
            if (bFilterSecondP)
            {
                int delta1 = Clip3(-tc2, tc2, ((((m1 + m3 + 1) >> 1) - m2 + delta) >> 1));
                src[-offset * 2] = Clip(m2 + delta1);
            }
            if (bFilterSecondQ)
            {
                int delta2 = Clip3(-tc2, tc2, ((((m6 + m4 + 1) >> 1) - m5 - delta) >> 1));
                src[offset] = Clip(m5 + delta2);
            }
        }
    }

    if (bPartPNoFilter)
    {
        src[-offset] = (pixel)m3;
        src[-offset * 2] = (pixel)m2;
        src[-offset * 3] = (pixel)m1;
    }
    if (bPartQNoFilter)
    {
        src[0] = (pixel)m4;
        src[offset] = (pixel)m5;
        src[offset * 2] = (pixel)m6;
    }
}

/**
 - Deblocking of one line/column for the chrominance component
 .
 \param src           pointer to picture data
 \param offset         offset value for picture data
 \param tc              tc value
 \param bPartPNoFilter  indicator to disable filtering on partP
 \param bPartQNoFilter  indicator to disable filtering on partQ
 */
inline void TComLoopFilter::xPelFilterChroma(pixel* src, int offset, int tc, bool bPartPNoFilter, bool bPartQNoFilter)
{
    int delta;

    int16_t m4  = (int16_t)src[0];
    int16_t m3  = (int16_t)src[-offset];
    int16_t m5  = (int16_t)src[offset];
    int16_t m2  = (int16_t)src[-offset * 2];

    delta = Clip3(-tc, tc, ((((m4 - m3) << 2) + m2 - m5 + 4) >> 3));
    src[-offset] = Clip(m3 + delta);
    src[0] = Clip(m4 - delta);

    if (bPartPNoFilter)
    {
        src[-offset] = (pixel)m3;
    }
    if (bPartQNoFilter)
    {
        src[0] = (pixel)m4;
    }
}

/**
 - Decision between strong and weak filter
 .
 \param offset         offset value for picture data
 \param d               d value
 \param beta            beta value
 \param tc              tc value
 \param src           pointer to picture data
 */
inline bool TComLoopFilter::xUseStrongFiltering(int offset, int d, int beta, int tc, pixel* src)
{
    int16_t m4  = (int16_t)src[0];
    int16_t m3  = (int16_t)src[-offset];
    int16_t m7  = (int16_t)src[offset * 3];
    int16_t m0  = (int16_t)src[-offset * 4];

    int d_strong = abs(m0 - m3) + abs(m7 - m4);

    return (d_strong < (beta >> 3)) && (d < (beta >> 2)) && (abs(m3 - m4) < ((tc * 5 + 1) >> 1));
}

inline int TComLoopFilter::xCalcDP(pixel* src, int offset)
{
    return abs(static_cast<int>(src[-offset * 3]) - 2 * src[-offset * 2] + src[-offset]);
}

inline int TComLoopFilter::xCalcDQ(pixel* src, int offset)
{
    return abs(static_cast<int>(src[0]) - 2 * src[offset] + src[offset * 2]);
}

//! \}
