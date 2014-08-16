/*****************************************************************************
* Copyright (C) 2013 x265 project
*
* Author: Gopu Govindaswamy <gopu@multicorewareinc.com>
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
#include "deblock.h"
#include "frame.h"
#include "slice.h"
#include "mv.h"

using namespace x265;

#define DEBLOCK_SMALLEST_BLOCK  8
#define DEFAULT_INTRA_TC_OFFSET 2

void Deblock::deblockCTU(TComDataCU* cu, int32_t dir, bool edgeFilter[], uint8_t blockingStrength[])
{
    memset(blockingStrength, 0, sizeof(uint8_t) * m_numPartitions);
    memset(edgeFilter, 0, sizeof(bool) * m_numPartitions);

    deblockCU(cu, 0, 0, dir, edgeFilter, blockingStrength);
}

/* Deblocking filter process in CU-based (the same function as conventional's)
 * param Edge the direction of the edge in block boundary (horizonta/vertical), which is added newly */
void Deblock::deblockCU(TComDataCU* cu, uint32_t absZOrderIdx, uint32_t depth, const int32_t dir, bool edgeFilter[], uint8_t blockingStrength[])
{
    if (!cu->m_pic || cu->getPartitionSize(absZOrderIdx) == SIZE_NONE)
        return;

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
                deblockCU(cu, absZOrderIdx, depth + 1, dir, edgeFilter, blockingStrength);
        }
        return;
    }

    Param params;
    setLoopfilterParam(cu, absZOrderIdx, &params);
    setEdgefilterTU(cu, absZOrderIdx, absZOrderIdx, depth, dir, edgeFilter, blockingStrength);
    setEdgefilterPU(cu, absZOrderIdx, dir, &params, edgeFilter, blockingStrength);

    for (uint32_t partIdx = absZOrderIdx; partIdx < absZOrderIdx + curNumParts; partIdx++)
    {
        uint32_t bsCheck;

        if (g_log2UnitSize == 2)
            bsCheck = (dir == EDGE_VER && !(partIdx & 1)) || (dir == EDGE_HOR && !(partIdx & 2));
        else
            bsCheck = 1;

        if (edgeFilter[partIdx] && bsCheck)
            getBoundaryStrengthSingle(cu, dir, partIdx, blockingStrength);
    }

    uint32_t log2UnitSize = g_log2UnitSize;
    uint32_t partIdxIncr = (DEBLOCK_SMALLEST_BLOCK >> log2UnitSize) ? (DEBLOCK_SMALLEST_BLOCK >> log2UnitSize) : 1;
    uint32_t sizeInPU = pic->getNumPartInCUSize() >> (depth);
    uint32_t shiftFactor = (dir == EDGE_VER) ? cu->getHorzChromaShift() : cu->getVertChromaShift();
    const bool alwaysDoChroma = (cu->getChromaFormat() == X265_CSP_I444 || (1 << log2UnitSize) > DEBLOCK_SMALLEST_BLOCK);

    for (uint32_t e = 0; e < sizeInPU; e += partIdxIncr)
    {
        edgeFilterLuma(cu, absZOrderIdx, depth, dir, e, blockingStrength);
        if (alwaysDoChroma || !(e % ((DEBLOCK_SMALLEST_BLOCK << shiftFactor) >> log2UnitSize)))
            edgeFilterChroma(cu, absZOrderIdx, depth, dir, e, blockingStrength);
    }
}

static inline uint32_t calcBsIdx(TComDataCU* cu, uint32_t absZOrderIdx, int32_t dir, int32_t edgeIdx, int32_t baseUnitIdx)
{
    uint32_t lcuWidthInBaseUnits = cu->m_pic->getNumPartInCUSize();

    if (dir)
        return g_rasterToZscan[g_zscanToRaster[absZOrderIdx] + edgeIdx * lcuWidthInBaseUnits + baseUnitIdx];
    else
        return g_rasterToZscan[g_zscanToRaster[absZOrderIdx] + baseUnitIdx * lcuWidthInBaseUnits + edgeIdx];
}

void Deblock::setEdgefilterMultiple(TComDataCU* cu, uint32_t scanIdx, uint32_t depth, int32_t dir, int32_t edgeIdx, bool value, bool edgeFilter[], uint8_t blockingStrength[], uint32_t widthInBaseUnits)
{
    if (!widthInBaseUnits)
        widthInBaseUnits = cu->m_pic->getNumPartInCUSize() >> depth;

    const uint32_t numElem = widthInBaseUnits;
    X265_CHECK(numElem > 0, "numElem edge filter check\n");
    for (uint32_t i = 0; i < numElem; i++)
    {
        const uint32_t bsidx = calcBsIdx(cu, scanIdx, dir, edgeIdx, i);
        edgeFilter[bsidx] = value;
        if (!edgeIdx)
            blockingStrength[bsidx] = value;
    }
}

void Deblock::setEdgefilterTU(TComDataCU* cu, uint32_t absTUPartIdx, uint32_t absZOrderIdx, uint32_t depth, int32_t dir, bool edgeFilter[], uint8_t blockingStrength[])
{
    if (cu->getTransformIdx(absZOrderIdx) + cu->getDepth(absZOrderIdx) > (uint8_t)depth)
    {
        const uint32_t curNumParts = cu->m_pic->getNumPartInCU() >> (depth << 1);
        const uint32_t qNumParts   = curNumParts >> 2;

        for (uint32_t partIdx = 0; partIdx < 4; partIdx++, absZOrderIdx += qNumParts)
        {
            uint32_t nsAddr = absZOrderIdx;
            setEdgefilterTU(cu, nsAddr, absZOrderIdx, depth + 1, dir, edgeFilter, blockingStrength);
        }
        return;
    }

    uint32_t widthInBaseUnits  = 1 << (cu->getLog2CUSize(absZOrderIdx) - cu->getTransformIdx(absZOrderIdx) - g_log2UnitSize);
    setEdgefilterMultiple(cu, absTUPartIdx, depth, dir, 0, true, edgeFilter, blockingStrength, widthInBaseUnits);
}

void Deblock::setEdgefilterPU(TComDataCU* cu, uint32_t absZOrderIdx, int32_t dir, Param *params, bool edgeFilter[], uint8_t blockingStrength[])
{
    const uint32_t depth = cu->getDepth(absZOrderIdx);
    const uint32_t widthInBaseUnits  = cu->m_pic->getNumPartInCUSize() >> depth;
    const uint32_t hWidthInBaseUnits  = widthInBaseUnits  >> 1;
    const uint32_t qWidthInBaseUnits  = widthInBaseUnits  >> 2;

    setEdgefilterMultiple(cu, absZOrderIdx, depth, dir, 0, (dir == EDGE_VER ? params->leftEdge : params->topEdge), edgeFilter, blockingStrength);

    int32_t mode = cu->getPartitionSize(absZOrderIdx);
    switch (mode)
    {
    case SIZE_2NxN:
    case SIZE_Nx2N:
        {
            const int32_t realDir = (mode == SIZE_2NxN ? EDGE_HOR : EDGE_VER);
            if (realDir == dir)
                setEdgefilterMultiple(cu, absZOrderIdx, depth, dir, hWidthInBaseUnits, true, edgeFilter, blockingStrength);
            break;
        }
    case SIZE_NxN:
        {
            setEdgefilterMultiple(cu, absZOrderIdx, depth, dir, hWidthInBaseUnits, true, edgeFilter, blockingStrength);
            break;
        }
    case SIZE_2NxnU:
    case SIZE_nLx2N:
        {
            const int32_t realDir = (mode == SIZE_2NxnU ? EDGE_HOR : EDGE_VER);
            if (realDir == dir)
                setEdgefilterMultiple(cu, absZOrderIdx, depth, dir, qWidthInBaseUnits, true, edgeFilter, blockingStrength);
            break;
        }
    case SIZE_2NxnD:
    case SIZE_nRx2N:
        {
            const int32_t realDir = (mode == SIZE_2NxnD ? EDGE_HOR : EDGE_VER);
            if (realDir == dir)
                setEdgefilterMultiple(cu, absZOrderIdx, depth, dir, widthInBaseUnits - qWidthInBaseUnits, true, edgeFilter, blockingStrength);
            break;
        }

    case SIZE_2Nx2N:
    default:
        break;
    }
}

void Deblock::setLoopfilterParam(TComDataCU* cu, uint32_t absZOrderIdx, Param *params)
{
    uint32_t x = cu->getCUPelX() + g_rasterToPelX[g_zscanToRaster[absZOrderIdx]];
    uint32_t y = cu->getCUPelY() + g_rasterToPelY[g_zscanToRaster[absZOrderIdx]];

    TComDataCU* tempCU;
    uint32_t    tempPartIdx;

    if (!x)
        params->leftEdge = false;
    else
    {
        tempCU = cu->getPULeft(tempPartIdx, absZOrderIdx);
        if (tempCU)
            params->leftEdge = true;
        else
            params->leftEdge = false;
    }

    if (!y)
        params->topEdge = false;
    else
    {
        tempCU = cu->getPUAbove(tempPartIdx, absZOrderIdx);
        if (tempCU)
            params->topEdge = true;
        else
            params->topEdge = false;
    }
}

void Deblock::getBoundaryStrengthSingle(TComDataCU* cu, int32_t dir, uint32_t absPartIdx, uint8_t blockingStrength[])
{
    Slice* const slice = cu->m_slice;
    const uint32_t partQ = absPartIdx;
    TComDataCU* const cuQ = cu;

    uint32_t partP;
    TComDataCU* cuP;
    uint8_t bs = 0;

    // Calculate block index
    if (dir == EDGE_VER)
        cuP = cuQ->getPULeft(partP, partQ);
    else // (dir == EDGE_HOR)
        cuP = cuQ->getPUAbove(partP, partQ);

    // Set BS for Intra MB : BS = 4 or 3
    if (cuP->isIntra(partP) || cuQ->isIntra(partQ))
        bs = 2;

    // Set BS for not Intra MB : BS = 2 or 1 or 0
    if (!cuP->isIntra(partP) && !cuQ->isIntra(partQ))
    {
        uint32_t nsPartQ = partQ;
        uint32_t nsPartP = partP;

        if (blockingStrength[absPartIdx] && (cuQ->getCbf(nsPartQ, TEXT_LUMA, cuQ->getTransformIdx(nsPartQ)) ||
            cuP->getCbf(nsPartP, TEXT_LUMA, cuP->getTransformIdx(nsPartP))))
            bs = 1;
        else
        {
            if (dir == EDGE_HOR)
                cuP = cuQ->getPUAbove(partP, partQ);

            if (slice->isInterB() || cuP->m_slice->isInterB())
            {
                int32_t refIdx;
                Frame *refP0, *refP1, *refQ0, *refQ1;
                refIdx = cuP->getCUMvField(0)->getRefIdx(partP);
                refP0 = (refIdx < 0) ? NULL : cuP->m_slice->m_refPicList[0][refIdx];
                refIdx = cuP->getCUMvField(1)->getRefIdx(partP);
                refP1 = (refIdx < 0) ? NULL : cuP->m_slice->m_refPicList[1][refIdx];
                refIdx = cuQ->getCUMvField(0)->getRefIdx(partQ);
                refQ0 = (refIdx < 0) ? NULL : slice->m_refPicList[0][refIdx];
                refIdx = cuQ->getCUMvField(1)->getRefIdx(partQ);
                refQ1 = (refIdx < 0) ? NULL : slice->m_refPicList[1][refIdx];

                MV mvp0 = cuP->getCUMvField(0)->getMv(partP);
                MV mvp1 = cuP->getCUMvField(1)->getMv(partP);
                MV mvq0 = cuQ->getCUMvField(0)->getMv(partQ);
                MV mvq1 = cuQ->getCUMvField(1)->getMv(partQ);

                if (!refP0) mvp0 = 0;
                if (!refP1) mvp1 = 0;
                if (!refQ0) mvq0 = 0;
                if (!refQ1) mvq1 = 0;

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
                    bs = 1;
            }
            else // slice->isInterP()
            {
                int32_t refIdx;
                Frame *refp0, *refq0;
                refIdx = cuP->getCUMvField(0)->getRefIdx(partP);
                refp0 = (refIdx < 0) ? NULL : cuP->m_slice->m_refPicList[0][refIdx];
                refIdx = cuQ->getCUMvField(0)->getRefIdx(partQ);
                refq0 = (refIdx < 0) ? NULL : slice->m_refPicList[0][refIdx];
                MV mvp0 = cuP->getCUMvField(0)->getMv(partP);
                MV mvq0 = cuQ->getCUMvField(0)->getMv(partQ);

                if (!refp0) mvp0 = 0;
                if (!refq0) mvq0 = 0;

                bs = ((refp0 != refq0) ||
                      (abs(mvq0.x - mvp0.x) >= 4) ||
                      (abs(mvq0.y - mvp0.y) >= 4)) ? 1 : 0;
            }
        }
    }

    blockingStrength[absPartIdx] = bs;
}

static inline int32_t calcDP(pixel* src, int32_t offset)
{
    return abs(static_cast<int32_t>(src[-offset * 3]) - 2 * src[-offset * 2] + src[-offset]);
}

static inline int32_t calcDQ(pixel* src, int32_t offset)
{
    return abs(static_cast<int32_t>(src[0]) - 2 * src[offset] + src[offset * 2]);
}

static inline bool useStrongFiltering(int32_t offset, int32_t d, int32_t beta, int32_t tc, pixel* src)
{
    int16_t m4     = (int16_t)src[0];
    int16_t m3     = (int16_t)src[-offset];
    int16_t m7     = (int16_t)src[offset * 3];
    int16_t m0     = (int16_t)src[-offset * 4];
    int32_t strong = abs(m0 - m3) + abs(m7 - m4);

    return (strong < (beta >> 3)) && (d < (beta >> 2)) && (abs(m3 - m4) < ((tc * 5 + 1) >> 1));
}

/* Deblocking for the luminance component with strong or weak filter
 * \param src            pointer to picture data
 * \param offset         offset value for picture data
 * \param tc             tc value
 * \param sw             decision strong/weak filter
 * \param partPNoFilter  indicator to disable filtering on partP
 * \param partQNoFilter  indicator to disable filtering on partQ
 * \param thrCut         threshold value for weak filter decision
 * \param filterSecondP  decision weak filter/no filter for partP
 * \param filterSecondQ  decision weak filter/no filter for partQ */
static inline void pelFilterLuma(pixel* src, int32_t offset, int32_t tc, bool sw, bool partPNoFilter, bool partQNoFilter,
                                 int32_t thrCut, bool filterSecondP, bool filterSecondQ)
{
    int32_t delta;

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

            int32_t tc2 = tc >> 1;
            if (filterSecondP)
            {
                int32_t delta1 = Clip3(-tc2, tc2, ((((m1 + m3 + 1) >> 1) - m2 + delta) >> 1));
                src[-offset * 2] = Clip(m2 + delta1);
            }
            if (filterSecondQ)
            {
                int32_t delta2 = Clip3(-tc2, tc2, ((((m6 + m4 + 1) >> 1) - m5 - delta) >> 1));
                src[offset] = Clip(m5 + delta2);
            }
        }
    }

    if (partPNoFilter)
    {
        src[-offset] = (pixel)m3;
        src[-offset * 2] = (pixel)m2;
        src[-offset * 3] = (pixel)m1;
    }
    if (partQNoFilter)
    {
        src[0] = (pixel)m4;
        src[offset] = (pixel)m5;
        src[offset * 2] = (pixel)m6;
    }
}

/* Deblocking of one line/column for the chrominance component
 * \param src            pointer to picture data
 * \param offset         offset value for picture data
 * \param tc             tc value
 * \param partPNoFilter  indicator to disable filtering on partP
 * \param partQNoFilter  indicator to disable filtering on partQ */
static inline void pelFilterChroma(pixel* src, int32_t offset, int32_t tc, bool partPNoFilter, bool partQNoFilter)
{
    int32_t delta;

    int16_t m4  = (int16_t)src[0];
    int16_t m3  = (int16_t)src[-offset];
    int16_t m5  = (int16_t)src[offset];
    int16_t m2  = (int16_t)src[-offset * 2];

    delta = Clip3(-tc, tc, ((((m4 - m3) << 2) + m2 - m5 + 4) >> 3));
    src[-offset] = Clip(m3 + delta);
    src[0] = Clip(m4 - delta);

    if (partPNoFilter)
        src[-offset] = (pixel)m3;
    if (partQNoFilter)
        src[0] = (pixel)m4;
}

void Deblock::edgeFilterLuma(TComDataCU* cu, uint32_t absZOrderIdx, uint32_t depth, int32_t dir, int32_t edge, uint8_t blockingStrength[])
{
    TComPicYuv* reconYuv = cu->m_pic->getPicYuvRec();
    pixel* src = reconYuv->getLumaAddr(cu->getAddr(), absZOrderIdx);
    pixel* tmpsrc = src;

    int32_t stride = reconYuv->getStride();
    uint32_t numParts = cu->m_pic->getNumPartInCUSize() >> depth;

    uint32_t log2UnitSize = g_log2UnitSize;
    uint32_t blocksInPart = (log2UnitSize - 2) > 0 ? 1 << (log2UnitSize - 2) : 1;
    uint32_t bsAbsIdx = 0, bs = 0;
    int32_t offset, srcStep;

    bool  partPNoFilter = false;
    bool  partQNoFilter = false;
    uint32_t  partP = 0;
    uint32_t  partQ = 0;
    TComDataCU* cuP = cu;
    TComDataCU* cuQ = cu;
    int32_t betaOffset = cuQ->m_slice->m_pps->deblockingFilterBetaOffsetDiv2 << 1;
    int32_t tcOffset = cuQ->m_slice->m_pps->deblockingFilterTcOffsetDiv2 << 1;

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
        bsAbsIdx = calcBsIdx(cu, absZOrderIdx, dir, edge, idx);
        bs = blockingStrength[bsAbsIdx];
        if (bs)
        {
            int32_t qpQ = cu->getQP(bsAbsIdx);
            partQ = bsAbsIdx;

            // Derive neighboring PU index
            if (dir == EDGE_VER)
                cuP = cuQ->getPULeft(partP, partQ);
            else // (dir == EDGE_HOR)
                cuP = cuQ->getPUAbove(partP, partQ);

            int32_t qpP = cuP->getQP(partP);
            int32_t qp = (qpP + qpQ + 1) >> 1;
            int32_t bitdepthScale = 1 << (X265_DEPTH - 8);

            int32_t indexTC = Clip3(0, MAX_QP + DEFAULT_INTRA_TC_OFFSET, int32_t(qp + DEFAULT_INTRA_TC_OFFSET * (bs - 1) + tcOffset));
            int32_t indexB = Clip3(0, MAX_QP, qp + betaOffset);

            int32_t tc = s_tcTable[indexTC] * bitdepthScale;
            int32_t beta = s_betaTable[indexB] * bitdepthScale;
            int32_t sideThreshold = (beta + (beta >> 1)) >> 3;
            int32_t thrCut = tc * 10;

            for (uint32_t blkIdx = 0; blkIdx < blocksInPart; blkIdx++)
            {
                int32_t dp0 = calcDP(tmpsrc + srcStep * (partOffset + blkIdx * 4 + 0), offset);
                int32_t dq0 = calcDQ(tmpsrc + srcStep * (partOffset + blkIdx * 4 + 0), offset);
                int32_t dp3 = calcDP(tmpsrc + srcStep * (partOffset + blkIdx * 4 + 3), offset);
                int32_t dq3 = calcDQ(tmpsrc + srcStep * (partOffset + blkIdx * 4 + 3), offset);
                int32_t d0 = dp0 + dq0;
                int32_t d3 = dp3 + dq3;

                int32_t dp = dp0 + dp3;
                int32_t dq = dq0 + dq3;
                int32_t d =  d0 + d3;

                if (cu->m_slice->m_pps->bTransquantBypassEnabled)
                {
                    // check if each of PUs is lossless coded
                    partPNoFilter = cuP->isLosslessCoded(partP);
                    partQNoFilter = cuQ->isLosslessCoded(partQ);
                }

                if (d < beta)
                {
                    bool filterP = (dp < sideThreshold);
                    bool filterQ = (dq < sideThreshold);

                    bool sw = useStrongFiltering(offset, 2 * d0, beta, tc, tmpsrc + srcStep * (partOffset + blkIdx * 4 + 0))
                           && useStrongFiltering(offset, 2 * d3, beta, tc, tmpsrc + srcStep * (partOffset + blkIdx * 4 + 3));

                    for (int32_t i = 0; i < DEBLOCK_SMALLEST_BLOCK / 2; i++)
                        pelFilterLuma(tmpsrc + srcStep * (partOffset + blkIdx * 4 + i), offset, tc, sw, partPNoFilter, partQNoFilter, thrCut, filterP, filterQ);
                }
            }
        }
    }
}

void Deblock::edgeFilterChroma(TComDataCU* cu, uint32_t absZOrderIdx, uint32_t depth, int32_t dir, int32_t edge, uint8_t blockingStrength[])
{
    int32_t chFmt = cu->getChromaFormat();
    TComPicYuv* reconYuv = cu->m_pic->getPicYuvRec();
    int32_t stride = reconYuv->getCStride();
    pixel* srcCb = reconYuv->getCbAddr(cu->getAddr(), absZOrderIdx);
    pixel* srcCr = reconYuv->getCrAddr(cu->getAddr(), absZOrderIdx);
    uint32_t log2UnitSizeH = g_log2UnitSize - cu->getHorzChromaShift();
    uint32_t log2UnitSizeV = g_log2UnitSize - cu->getVertChromaShift();
    uint32_t sizeChromaH = 1 << log2UnitSizeH;
    uint32_t sizeChromaV = 1 << log2UnitSizeV;
    int32_t offset, srcStep;

    const uint32_t lcuWidthInBaseUnits = cu->m_pic->getNumPartInCUSize();

    bool partPNoFilter = false;
    bool partQNoFilter = false;
    uint32_t partP;
    uint32_t partQ;
    TComDataCU* cuP;
    TComDataCU* cuQ = cu;
    int32_t tcOffset = cu->m_slice->m_pps->deblockingFilterTcOffsetDiv2 << 1;

    // Vertical Position
    uint32_t edgeNumInLCUVert = g_zscanToRaster[absZOrderIdx] % lcuWidthInBaseUnits + edge;
    uint32_t edgeNumInLCUHor = g_zscanToRaster[absZOrderIdx] / lcuWidthInBaseUnits + edge;

    if ((sizeChromaH < DEBLOCK_SMALLEST_BLOCK) && (sizeChromaV < DEBLOCK_SMALLEST_BLOCK) &&
        (((edgeNumInLCUVert % (DEBLOCK_SMALLEST_BLOCK >> log2UnitSizeH)) && !dir) ||
         ((edgeNumInLCUHor % (DEBLOCK_SMALLEST_BLOCK >> log2UnitSizeV)) && dir)))
        return;

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
        tmpSrcCb  += (edge << log2UnitSizeH);
        tmpSrcCr  += (edge << log2UnitSizeH);
        loopLength = sizeChromaV;
    }
    else // (dir == EDGE_HOR)
    {
        offset     = stride;
        srcStep    = 1;
        tmpSrcCb  += edge * stride << log2UnitSizeV;
        tmpSrcCr  += edge * stride << log2UnitSizeV;
        loopLength = sizeChromaH;
    }

    for (uint32_t idx = 0; idx < numParts; idx++)
    {
        bsAbsIdx = calcBsIdx(cu, absZOrderIdx, dir, edge, idx);
        bs = blockingStrength[bsAbsIdx];

        if (bs > 1)
        {
            int32_t qpQ = cu->getQP(bsAbsIdx);
            partQ = bsAbsIdx;

            // Derive neighboring PU index
            if (dir == EDGE_VER)
                cuP = cuQ->getPULeft(partP, partQ);
            else // (dir == EDGE_HOR)
                cuP = cuQ->getPUAbove(partP, partQ);

            int32_t qpP = cuP->getQP(partP);

            if (cu->m_slice->m_pps->bTransquantBypassEnabled)
            {
                // check if each of PUs is lossless coded
                partPNoFilter = cuP->isLosslessCoded(partP);
                partQNoFilter = cuQ->isLosslessCoded(partQ);
            }

            for (uint32_t chromaIdx = 0; chromaIdx < 2; chromaIdx++)
            {
                int32_t chromaQPOffset  = !chromaIdx ? cu->m_slice->m_pps->chromaCbQpOffset : cu->m_slice->m_pps->chromaCrQpOffset;
                pixel* tmpSrcChroma = !chromaIdx ? tmpSrcCb : tmpSrcCr;
                int32_t qp = ((qpP + qpQ + 1) >> 1) + chromaQPOffset;
                if (qp >= 30)
                {
                    if (chFmt == X265_CSP_I420)
                        qp = g_chromaScale[qp];
                    else
                        qp = X265_MIN(qp, 51);
                }

                int32_t bitdepthScale = 1 << (X265_DEPTH - 8);
                int32_t indexTC = Clip3(0, MAX_QP + DEFAULT_INTRA_TC_OFFSET, qp + DEFAULT_INTRA_TC_OFFSET * (bs - 1) + tcOffset);
                int32_t tc = s_tcTable[indexTC] * bitdepthScale;

                for (uint32_t step = 0; step < loopLength; step++)
                    pelFilterChroma(tmpSrcChroma + srcStep * (step + idx * loopLength), offset, tc, partPNoFilter, partQNoFilter);
            }
        }
    }
}

const uint8_t Deblock::s_tcTable[54] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2,
    2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 8, 9, 10, 11, 13, 14, 16, 18, 20, 22, 24
};

const uint8_t Deblock::s_betaTable[52] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
    18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64
};

