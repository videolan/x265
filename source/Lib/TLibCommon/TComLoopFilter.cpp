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

#include "TComLoopFilter.h"
#include "TComSlice.h"
#include "mv.h"

using namespace x265;

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Constants
// ====================================================================================================================

#define   EDGE_VER    0
#define   EDGE_HOR    1
#define   QpUV(iQpY)  (((iQpY) < 0) ? (iQpY) : (((iQpY) > 57) ? ((iQpY) - 6) : g_chromaScale[(iQpY)]))

#define DEFAULT_INTRA_TC_OFFSET 2 ///< Default intra TC offset

// ====================================================================================================================
// Tables
// ====================================================================================================================

const UChar TComLoopFilter::sm_tcTable[54] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 8, 9, 10, 11, 13, 14, 16, 18, 20, 22, 24
};

const UChar TComLoopFilter::sm_betaTable[52] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64
};

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TComLoopFilter::TComLoopFilter()
    : m_numPartitions(0)
    , m_bLFCrossTileBoundary(true)
{
    for (UInt dir = 0; dir < 2; dir++)
    {
        m_blockingStrength[dir] = NULL;
        m_bEdgeFilter[dir] = NULL;
    }
}

TComLoopFilter::~TComLoopFilter()
{}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================
Void TComLoopFilter::setCfg(Bool bLFCrossTileBoundary)
{
    m_bLFCrossTileBoundary = bLFCrossTileBoundary;
}

Void TComLoopFilter::create(UInt maxCuDepth)
{
    destroy();
    m_numPartitions = 1 << (maxCuDepth << 1);
    for (UInt dir = 0; dir < 2; dir++)
    {
        m_blockingStrength[dir] = new UChar[m_numPartitions];
        m_bEdgeFilter[dir] = new Bool[m_numPartitions];
    }
}

Void TComLoopFilter::destroy()
{
    for (UInt dir = 0; dir < 2; dir++)
    {
        if (m_blockingStrength)
        {
            delete [] m_blockingStrength[dir];
            m_blockingStrength[dir] = NULL;
        }
        if (m_bEdgeFilter[dir])
        {
            delete [] m_bEdgeFilter[dir];
            m_bEdgeFilter[dir] = NULL;
        }
    }
}

/**
 - call deblocking function for every CU
 .
 \param  pic   picture class (TComPic) pointer
 */
Void TComLoopFilter::loopFilterPic(TComPic* pic)
{
    // TODO: Min, thread parallelism later
    // Horizontal filtering
    for (UInt cuAddr = 0; cuAddr < pic->getNumCUsInFrame(); cuAddr++)
    {
        TComDataCU* cu = pic->getCU(cuAddr);

        ::memset(m_blockingStrength[EDGE_VER], 0, sizeof(UChar) * m_numPartitions);
        ::memset(m_bEdgeFilter[EDGE_VER], 0, sizeof(Bool) * m_numPartitions);

        // CU-based deblocking
        xDeblockCU(cu, 0, 0, EDGE_VER);

        // Vertical filtering
        // NOTE: delay one CU to avoid conflict between V and H
        if (cuAddr > 0)
        {
            cu = pic->getCU(cuAddr-1);
            ::memset(m_blockingStrength[EDGE_HOR], 0, sizeof(UChar) * m_numPartitions);
            ::memset(m_bEdgeFilter[EDGE_HOR], 0, sizeof(Bool) * m_numPartitions);

            xDeblockCU(cu, 0, 0, EDGE_HOR);
        }
    }

    // Last H-Filter
    {
        TComDataCU* cu = pic->getCU(pic->getNumCUsInFrame()-1);
        ::memset(m_blockingStrength[EDGE_HOR], 0, sizeof(UChar) * m_numPartitions);
        ::memset(m_bEdgeFilter[EDGE_HOR], 0, sizeof(Bool) * m_numPartitions);

        xDeblockCU(cu, 0, 0, EDGE_HOR);
    }
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

/**
 - Deblocking filter process in CU-based (the same function as conventional's)
 .
 \param Edge          the direction of the edge in block boundary (horizonta/vertical), which is added newly
*/
Void TComLoopFilter::xDeblockCU(TComDataCU* cu, UInt absZOrderIdx, UInt depth, Int edge)
{
    if (cu->getPic() == 0 || cu->getPartitionSize(absZOrderIdx) == SIZE_NONE)
    {
        return;
    }
    TComPic* pic     = cu->getPic();
    UInt curNumParts = pic->getNumPartInCU() >> (depth << 1);
    UInt qNumParts   = curNumParts >> 2;

    if (cu->getDepth(absZOrderIdx) > depth)
    {
        for (UInt partIdx = 0; partIdx < 4; partIdx++, absZOrderIdx += qNumParts)
        {
            UInt lpelx   = cu->getCUPelX() + g_rasterToPelX[g_zscanToRaster[absZOrderIdx]];
            UInt tpely   = cu->getCUPelY() + g_rasterToPelY[g_zscanToRaster[absZOrderIdx]];
            if ((lpelx < cu->getSlice()->getSPS()->getPicWidthInLumaSamples()) && (tpely < cu->getSlice()->getSPS()->getPicHeightInLumaSamples()))
            {
                xDeblockCU(cu, absZOrderIdx, depth + 1, edge);
            }
        }

        return;
    }

    xSetLoopfilterParam(cu, absZOrderIdx);

    xSetEdgefilterTU(cu, absZOrderIdx, absZOrderIdx, depth);
    xSetEdgefilterPU(cu, absZOrderIdx);

    Int dir = edge;
    for (UInt partIdx = absZOrderIdx; partIdx < absZOrderIdx + curNumParts; partIdx++)
    {
        UInt bsCheck;
        if ((g_maxCUWidth >> g_maxCUDepth) == 4)
        {
            bsCheck = (dir == EDGE_VER && partIdx % 2 == 0) || (dir == EDGE_HOR && (partIdx - ((partIdx >> 2) << 2)) / 2 == 0);
        }
        else
        {
            bsCheck = 1;
        }

        if (m_bEdgeFilter[dir][partIdx] && bsCheck)
        {
            xGetBoundaryStrengthSingle(cu, dir, partIdx);
        }
    }

    UInt pelsInPart = g_maxCUWidth >> g_maxCUDepth;
    UInt partIdxIncr = DEBLOCK_SMALLEST_BLOCK / pelsInPart ? DEBLOCK_SMALLEST_BLOCK / pelsInPart : 1;

    UInt sizeInPU = pic->getNumPartInWidth() >> (depth);

    for (UInt e = 0; e < sizeInPU; e += partIdxIncr)
    {
        xEdgeFilterLuma(cu, absZOrderIdx, depth, dir, e);
        if ((pelsInPart > DEBLOCK_SMALLEST_BLOCK) || (e % ((DEBLOCK_SMALLEST_BLOCK << 1) / pelsInPart)) == 0)
        {
            xEdgeFilterChroma(cu, absZOrderIdx, depth, dir, e);
        }
    }
}

Void TComLoopFilter::xSetEdgefilterMultiple(TComDataCU* cu, UInt scanIdx, UInt depth, Int dir, Int edgeIdx, Bool bValue, UInt widthInBaseUnits, UInt heightInBaseUnits)
{
    if (widthInBaseUnits == 0)
    {
        widthInBaseUnits  = cu->getPic()->getNumPartInWidth() >> depth;
    }
    if (heightInBaseUnits == 0)
    {
        heightInBaseUnits = cu->getPic()->getNumPartInHeight() >> depth;
    }
    const UInt numElem = dir == 0 ? heightInBaseUnits : widthInBaseUnits;
    assert(numElem > 0);
    assert(widthInBaseUnits > 0);
    assert(heightInBaseUnits > 0);
    for (UInt i = 0; i < numElem; i++)
    {
        const UInt bsidx = xCalcBsIdx(cu, scanIdx, dir, edgeIdx, i);
        m_bEdgeFilter[dir][bsidx] = bValue;
        if (edgeIdx == 0)
        {
            m_blockingStrength[dir][bsidx] = bValue;
        }
    }
}

Void TComLoopFilter::xSetEdgefilterTU(TComDataCU* cu, UInt absTUPartIdx, UInt absZOrderIdx, UInt depth)
{
    if (cu->getTransformIdx(absZOrderIdx) + cu->getDepth(absZOrderIdx) > depth)
    {
        const UInt curNumParts = cu->getPic()->getNumPartInCU() >> (depth << 1);
        const UInt qNumParts   = curNumParts >> 2;
        for (UInt partIdx = 0; partIdx < 4; partIdx++, absZOrderIdx += qNumParts)
        {
            UInt nsAddr = absZOrderIdx;
            xSetEdgefilterTU(cu, nsAddr, absZOrderIdx, depth + 1);
        }

        return;
    }

    Int trWidth  = cu->getWidth(absZOrderIdx) >> cu->getTransformIdx(absZOrderIdx);
    Int trHeight = cu->getHeight(absZOrderIdx) >> cu->getTransformIdx(absZOrderIdx);

    UInt widthInBaseUnits  = trWidth / (g_maxCUWidth >> g_maxCUDepth);
    UInt heightInBaseUnits = trHeight / (g_maxCUWidth >> g_maxCUDepth);

    xSetEdgefilterMultiple(cu, absTUPartIdx, depth, EDGE_VER, 0, true, widthInBaseUnits, heightInBaseUnits);
    xSetEdgefilterMultiple(cu, absTUPartIdx, depth, EDGE_HOR, 0, true, widthInBaseUnits, heightInBaseUnits);
}

Void TComLoopFilter::xSetEdgefilterPU(TComDataCU* cu, UInt absZOrderIdx)
{
    const UInt depth = cu->getDepth(absZOrderIdx);
    const UInt widthInBaseUnits  = cu->getPic()->getNumPartInWidth() >> depth;
    const UInt heightInBaseUnits = cu->getPic()->getNumPartInHeight() >> depth;
    const UInt hWidthInBaseUnits  = widthInBaseUnits  >> 1;
    const UInt hHeightInBaseUnits = heightInBaseUnits >> 1;
    const UInt qWidthInBaseUnits  = widthInBaseUnits  >> 2;
    const UInt qHeightInBaseUnits = heightInBaseUnits >> 2;

    xSetEdgefilterMultiple(cu, absZOrderIdx, depth, EDGE_VER, 0, m_lfcuParam.bLeftEdge);
    xSetEdgefilterMultiple(cu, absZOrderIdx, depth, EDGE_HOR, 0, m_lfcuParam.bTopEdge);

    switch (cu->getPartitionSize(absZOrderIdx))
    {
    case SIZE_2Nx2N:
    {
        break;
    }
    case SIZE_2NxN:
    {
        xSetEdgefilterMultiple(cu, absZOrderIdx, depth, EDGE_HOR, hHeightInBaseUnits, true);
        break;
    }
    case SIZE_Nx2N:
    {
        xSetEdgefilterMultiple(cu, absZOrderIdx, depth, EDGE_VER, hWidthInBaseUnits, true);
        break;
    }
    case SIZE_NxN:
    {
        xSetEdgefilterMultiple(cu, absZOrderIdx, depth, EDGE_VER, hWidthInBaseUnits, true);
        xSetEdgefilterMultiple(cu, absZOrderIdx, depth, EDGE_HOR, hHeightInBaseUnits, true);
        break;
    }
    case SIZE_2NxnU:
    {
        xSetEdgefilterMultiple(cu, absZOrderIdx, depth, EDGE_HOR, qHeightInBaseUnits, true);
        break;
    }
    case SIZE_2NxnD:
    {
        xSetEdgefilterMultiple(cu, absZOrderIdx, depth, EDGE_HOR, heightInBaseUnits - qHeightInBaseUnits, true);
        break;
    }
    case SIZE_nLx2N:
    {
        xSetEdgefilterMultiple(cu, absZOrderIdx, depth, EDGE_VER, qWidthInBaseUnits, true);
        break;
    }
    case SIZE_nRx2N:
    {
        xSetEdgefilterMultiple(cu, absZOrderIdx, depth, EDGE_VER, widthInBaseUnits - qWidthInBaseUnits, true);
        break;
    }
    default:
    {
        break;
    }
    }
}

Void TComLoopFilter::xSetLoopfilterParam(TComDataCU* cu, UInt absZOrderIdx)
{
    UInt x = cu->getCUPelX() + g_rasterToPelX[g_zscanToRaster[absZOrderIdx]];
    UInt y = cu->getCUPelY() + g_rasterToPelY[g_zscanToRaster[absZOrderIdx]];

    TComDataCU* tempCU;
    UInt        tempPartIdx;

    // We can't here when DeblockingDisable flag is true
    assert(!cu->getSlice()->getDeblockingFilterDisable());

    if (x == 0)
    {
        m_lfcuParam.bLeftEdge = false;
    }
    else
    {
        tempCU = cu->getPULeft(tempPartIdx, absZOrderIdx, !true, !m_bLFCrossTileBoundary);
        if (tempCU)
        {
            m_lfcuParam.bLeftEdge = true;
        }
        else
        {
            m_lfcuParam.bLeftEdge = false;
        }
    }

    if (y == 0)
    {
        m_lfcuParam.bTopEdge = false;
    }
    else
    {
        tempCU = cu->getPUAbove(tempPartIdx, absZOrderIdx, !true, false, !m_bLFCrossTileBoundary);
        if (tempCU)
        {
            m_lfcuParam.bTopEdge = true;
        }
        else
        {
            m_lfcuParam.bTopEdge = false;
        }
    }
}

Void TComLoopFilter::xGetBoundaryStrengthSingle(TComDataCU* cu, Int dir, UInt absPartIdx)
{
    TComSlice* const slice = cu->getSlice();

    const UInt partQ = absPartIdx;
    TComDataCU* const cuQ = cu;

    UInt partP;
    TComDataCU* cuP;
    UInt bs = 0;

    //-- Calculate Block Index
    if (dir == EDGE_VER)
    {
        cuP = cuQ->getPULeft(partP, partQ, !true, !m_bLFCrossTileBoundary);
    }
    else // (dir == EDGE_HOR)
    {
        cuP = cuQ->getPUAbove(partP, partQ, !true, false, !m_bLFCrossTileBoundary);
    }

    //-- Set BS for Intra MB : BS = 4 or 3
    if (cuP->isIntra(partP) || cuQ->isIntra(partQ))
    {
        bs = 2;
    }

    //-- Set BS for not Intra MB : BS = 2 or 1 or 0
    if (!cuP->isIntra(partP) && !cuQ->isIntra(partQ))
    {
        UInt nsPartQ = partQ;
        UInt nsPartP = partP;

        if (m_blockingStrength[dir][absPartIdx] && (cuQ->getCbf(nsPartQ, TEXT_LUMA, cuQ->getTransformIdx(nsPartQ)) != 0 || cuP->getCbf(nsPartP, TEXT_LUMA, cuP->getTransformIdx(nsPartP)) != 0))
        {
            bs = 1;
        }
        else
        {
            if (dir == EDGE_HOR)
            {
                cuP = cuQ->getPUAbove(partP, partQ, !true, false, !m_bLFCrossTileBoundary);
            }
            if (slice->isInterB() || cuP->getSlice()->isInterB())
            {
                Int refIdx;
                TComPic *refP0, *refP1, *refQ0, *refQ1;
                refIdx = cuP->getCUMvField(REF_PIC_LIST_0)->getRefIdx(partP);
                refP0 = (refIdx < 0) ? NULL : cuP->getSlice()->getRefPic(REF_PIC_LIST_0, refIdx);
                refIdx = cuP->getCUMvField(REF_PIC_LIST_1)->getRefIdx(partP);
                refP1 = (refIdx < 0) ? NULL : cuP->getSlice()->getRefPic(REF_PIC_LIST_1, refIdx);
                refIdx = cuQ->getCUMvField(REF_PIC_LIST_0)->getRefIdx(partQ);
                refQ0 = (refIdx < 0) ? NULL : slice->getRefPic(REF_PIC_LIST_0, refIdx);
                refIdx = cuQ->getCUMvField(REF_PIC_LIST_1)->getRefIdx(partQ);
                refQ1 = (refIdx < 0) ? NULL : slice->getRefPic(REF_PIC_LIST_1, refIdx);

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
                    bs = 0;
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
                Int refIdx;
                TComPic *refp0, *refq0;
                refIdx = cuP->getCUMvField(REF_PIC_LIST_0)->getRefIdx(partP);
                refp0 = (refIdx < 0) ? NULL : cuP->getSlice()->getRefPic(REF_PIC_LIST_0, refIdx);
                refIdx = cuQ->getCUMvField(REF_PIC_LIST_0)->getRefIdx(partQ);
                refq0 = (refIdx < 0) ? NULL : slice->getRefPic(REF_PIC_LIST_0, refIdx);
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

    m_blockingStrength[dir][absPartIdx] = bs;
}

Void TComLoopFilter::xEdgeFilterLuma(TComDataCU* cu, UInt absZOrderIdx, UInt depth, Int dir, Int edge)
{
    TComPicYuv* reconYuv = cu->getPic()->getPicYuvRec();
    Pel* src = reconYuv->getLumaAddr(cu->getAddr(), absZOrderIdx);
    Pel* tmpsrc = src;

    Int stride = reconYuv->getStride();
    Int qp = 0;
    Int qpP = 0;
    Int qpQ = 0;
    UInt numParts = cu->getPic()->getNumPartInWidth() >> depth;

    UInt pelsInPart = g_maxCUWidth >> g_maxCUDepth;
    UInt bsAbsIdx = 0, bs = 0;
    Int  offset, srcStep;

    Bool  bPCMFilter = (cu->getSlice()->getSPS()->getUsePCM() && cu->getSlice()->getSPS()->getPCMFilterDisableFlag()) ? true : false;
    Bool  bPartPNoFilter = false;
    Bool  bPartQNoFilter = false;
    UInt  partP = 0;
    UInt  partQ = 0;
    TComDataCU* cuP = cu;
    TComDataCU* cuQ = cu;
    Int  betaOffsetDiv2 = cuQ->getSlice()->getDeblockingFilterBetaOffsetDiv2();
    Int  tcOffsetDiv2 = cuQ->getSlice()->getDeblockingFilterTcOffsetDiv2();

    if (dir == EDGE_VER)
    {
        offset = 1;
        srcStep = stride;
        tmpsrc += edge * pelsInPart;
    }
    else // (dir == EDGE_HOR)
    {
        offset = stride;
        srcStep = 1;
        tmpsrc += edge * pelsInPart * stride;
    }

    for (UInt idx = 0; idx < numParts; idx++)
    {
        bsAbsIdx = xCalcBsIdx(cu, absZOrderIdx, dir, edge, idx);
        bs = m_blockingStrength[dir][bsAbsIdx];
        if (bs)
        {
            qpQ = cu->getQP(bsAbsIdx);
            partQ = bsAbsIdx;
            // Derive neighboring PU index
            if (dir == EDGE_VER)
            {
                cuP = cuQ->getPULeft(partP, partQ, !true, !m_bLFCrossTileBoundary);
            }
            else // (dir == EDGE_HOR)
            {
                cuP = cuQ->getPUAbove(partP, partQ, !true, false, !m_bLFCrossTileBoundary);
            }

            qpP = cuP->getQP(partP);
            qp = (qpP + qpQ + 1) >> 1;
            Int bitdepthScale = 1 << (X265_DEPTH - 8);

            Int indexTC = Clip3(0, MAX_QP + DEFAULT_INTRA_TC_OFFSET, Int(qp + DEFAULT_INTRA_TC_OFFSET * (bs - 1) + (tcOffsetDiv2 << 1)));
            Int indexB = Clip3(0, MAX_QP, qp + (betaOffsetDiv2 << 1));

            Int tc =  sm_tcTable[indexTC] * bitdepthScale;
            Int beta = sm_betaTable[indexB] * bitdepthScale;
            Int sideThreshold = (beta + (beta >> 1)) >> 3;
            Int thrCut = tc * 10;

            UInt blocksInPart = pelsInPart / 4 ? pelsInPart / 4 : 1;
            for (UInt blkIdx = 0; blkIdx < blocksInPart; blkIdx++)
            {
                Int dp0 = xCalcDP(tmpsrc + srcStep * (idx * pelsInPart + blkIdx * 4 + 0), offset);
                Int dq0 = xCalcDQ(tmpsrc + srcStep * (idx * pelsInPart + blkIdx * 4 + 0), offset);
                Int dp3 = xCalcDP(tmpsrc + srcStep * (idx * pelsInPart + blkIdx * 4 + 3), offset);
                Int dq3 = xCalcDQ(tmpsrc + srcStep * (idx * pelsInPart + blkIdx * 4 + 3), offset);
                Int d0 = dp0 + dq0;
                Int d3 = dp3 + dq3;

                Int dp = dp0 + dp3;
                Int dq = dq0 + dq3;
                Int d =  d0 + d3;

                if (bPCMFilter || cu->getSlice()->getPPS()->getTransquantBypassEnableFlag())
                {
                    // Check if each of PUs is I_PCM with LF disabling
                    bPartPNoFilter = (bPCMFilter && cuP->getIPCMFlag(partP));
                    bPartQNoFilter = (bPCMFilter && cuQ->getIPCMFlag(partQ));

                    // check if each of PUs is lossless coded
                    bPartPNoFilter = bPartPNoFilter || (cuP->isLosslessCoded(partP));
                    bPartQNoFilter = bPartQNoFilter || (cuQ->isLosslessCoded(partQ));
                }

                if (d < beta)
                {
                    Bool bFilterP = (dp < sideThreshold);
                    Bool bFilterQ = (dq < sideThreshold);

                    Bool sw =  xUseStrongFiltering(offset, 2 * d0, beta, tc, tmpsrc + srcStep * (idx * pelsInPart + blkIdx * 4 + 0))
                        && xUseStrongFiltering(offset, 2 * d3, beta, tc, tmpsrc + srcStep * (idx * pelsInPart + blkIdx * 4 + 3));

                    for (Int i = 0; i < DEBLOCK_SMALLEST_BLOCK / 2; i++)
                    {
                        xPelFilterLuma(tmpsrc + srcStep * (idx * pelsInPart + blkIdx * 4 + i), offset, tc, sw, bPartPNoFilter, bPartQNoFilter, thrCut, bFilterP, bFilterQ);
                    }
                }
            }
        }
    }
}

Void TComLoopFilter::xEdgeFilterChroma(TComDataCU* cu, UInt absZOrderIdx, UInt depth, Int dir, Int edge)
{
    TComPicYuv* reconYuv = cu->getPic()->getPicYuvRec();
    Int stride = reconYuv->getCStride();
    Pel* srcCb = reconYuv->getCbAddr(cu->getAddr(), absZOrderIdx);
    Pel* srcCr = reconYuv->getCrAddr(cu->getAddr(), absZOrderIdx);
    Int qp = 0;
    Int qpP = 0;
    Int qpQ = 0;

    UInt  pelsInPartChroma = g_maxCUWidth >> (g_maxCUDepth + 1);
    Int   offset, srcStep;

    const UInt lcuWidthInBaseUnits = cu->getPic()->getNumPartInWidth();

    Bool  bPCMFilter = (cu->getSlice()->getSPS()->getUsePCM() && cu->getSlice()->getSPS()->getPCMFilterDisableFlag()) ? true : false;
    Bool  bPartPNoFilter = false;
    Bool  bPartQNoFilter = false;
    UInt  partP;
    UInt  partQ;
    TComDataCU* cuP;
    TComDataCU* cuQ = cu;
    Int tcOffsetDiv2 = cu->getSlice()->getDeblockingFilterTcOffsetDiv2();

    // Vertical Position
    UInt edgeNumInLCUVert = g_zscanToRaster[absZOrderIdx] % lcuWidthInBaseUnits + edge;
    UInt edgeNumInLCUHor = g_zscanToRaster[absZOrderIdx] / lcuWidthInBaseUnits + edge;

    if ((pelsInPartChroma < DEBLOCK_SMALLEST_BLOCK) &&
        (((edgeNumInLCUVert % (DEBLOCK_SMALLEST_BLOCK / pelsInPartChroma)) && (dir == 0)) ||
        ((edgeNumInLCUHor % (DEBLOCK_SMALLEST_BLOCK / pelsInPartChroma)) && dir)))
    {
        return;
    }

    UInt numParts = cu->getPic()->getNumPartInWidth() >> depth;

    UInt bsAbsIdx;
    UChar bs;

    Pel* tmpSrcCb = srcCb;
    Pel* tmpSrcCr = srcCr;

    if (dir == EDGE_VER)
    {
        offset   = 1;
        srcStep  = stride;
        tmpSrcCb += edge * pelsInPartChroma;
        tmpSrcCr += edge * pelsInPartChroma;
    }
    else // (dir == EDGE_HOR)
    {
        offset   = stride;
        srcStep  = 1;
        tmpSrcCb += edge * stride * pelsInPartChroma;
        tmpSrcCr += edge * stride * pelsInPartChroma;
    }

    for (UInt idx = 0; idx < numParts; idx++)
    {
        bs = 0;

        bsAbsIdx = xCalcBsIdx(cu, absZOrderIdx, dir, edge, idx);
        bs = m_blockingStrength[dir][bsAbsIdx];

        if (bs > 1)
        {
            qpQ = cu->getQP(bsAbsIdx);
            partQ = bsAbsIdx;
            // Derive neighboring PU index
            if (dir == EDGE_VER)
            {
                cuP = cuQ->getPULeft(partP, partQ, !true, !m_bLFCrossTileBoundary);
            }
            else // (dir == EDGE_HOR)
            {
                cuP = cuQ->getPUAbove(partP, partQ, !true, false, !m_bLFCrossTileBoundary);
            }

            qpP = cuP->getQP(partP);

            if (bPCMFilter || cu->getSlice()->getPPS()->getTransquantBypassEnableFlag())
            {
                // Check if each of PUs is I_PCM with LF disabling
                bPartPNoFilter = (bPCMFilter && cuP->getIPCMFlag(partP));
                bPartQNoFilter = (bPCMFilter && cuQ->getIPCMFlag(partQ));

                // check if each of PUs is lossless coded
                bPartPNoFilter = bPartPNoFilter || (cuP->isLosslessCoded(partP));
                bPartQNoFilter = bPartQNoFilter || (cuQ->isLosslessCoded(partQ));
            }

            for (UInt chromaIdx = 0; chromaIdx < 2; chromaIdx++)
            {
                Int chromaQPOffset  = (chromaIdx == 0) ? cu->getSlice()->getPPS()->getChromaCbQpOffset() : cu->getSlice()->getPPS()->getChromaCrQpOffset();
                Pel* piTmpSrcChroma = (chromaIdx == 0) ? tmpSrcCb : tmpSrcCr;

                qp = QpUV(((qpP + qpQ + 1) >> 1) + chromaQPOffset);
                Int iBitdepthScale = 1 << (X265_DEPTH - 8);

                Int iIndexTC = Clip3(0, MAX_QP + DEFAULT_INTRA_TC_OFFSET, qp + DEFAULT_INTRA_TC_OFFSET * (bs - 1) + (tcOffsetDiv2 << 1));
                Int iTc =  sm_tcTable[iIndexTC] * iBitdepthScale;

                for (UInt uiStep = 0; uiStep < pelsInPartChroma; uiStep++)
                {
                    xPelFilterChroma(piTmpSrcChroma + srcStep * (uiStep + idx * pelsInPartChroma), offset, iTc, bPartPNoFilter, bPartQNoFilter);
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
inline Void TComLoopFilter::xPelFilterLuma(Pel* src, Int offset, Int tc, Bool sw, Bool bPartPNoFilter, Bool bPartQNoFilter, Int thrCut, Bool bFilterSecondP, Bool bFilterSecondQ)
{
    Int delta;

    Short m4  = (Short)src[0];
    Short m3  = (Short)src[-offset];
    Short m5  = (Short)src[offset];
    Short m2  = (Short)src[-offset * 2];
    Short m6  = (Short)src[offset * 2];
    Short m1  = (Short)src[-offset * 3];
    Short m7  = (Short)src[offset * 3];
    Short m0  = (Short)src[-offset * 4];

    if (sw)
    {
        src[-offset]     = (Pel)Clip3(m3 - 2 * tc, m3 + 2 * tc, ((m1 + 2 * m2 + 2 * m3 + 2 * m4 + m5 + 4) >> 3));
        src[0]           = (Pel)Clip3(m4 - 2 * tc, m4 + 2 * tc, ((m2 + 2 * m3 + 2 * m4 + 2 * m5 + m6 + 4) >> 3));
        src[-offset * 2] = (Pel)Clip3(m2 - 2 * tc, m2 + 2 * tc, ((m1 + m2 + m3 + m4 + 2) >> 2));
        src[offset]      = (Pel)Clip3(m5 - 2 * tc, m5 + 2 * tc, ((m3 + m4 + m5 + m6 + 2) >> 2));
        src[-offset * 3] = (Pel)Clip3(m1 - 2 * tc, m1 + 2 * tc, ((2 * m0 + 3 * m1 + m2 + m3 + m4 + 4) >> 3));
        src[offset * 2]  = (Pel)Clip3(m6 - 2 * tc, m6 + 2 * tc, ((m3 + m4 + m5 + 3 * m6 + 2 * m7 + 4) >> 3));
    }
    else
    {
        /* Weak filter */
        delta = (9 * (m4 - m3) - 3 * (m5 - m2) + 8) >> 4;

        if (abs(delta) < thrCut)
        {
            delta = Clip3(-tc, tc, delta);
            src[-offset] = (Pel)ClipY((m3 + delta));
            src[0] = (Pel)ClipY((m4 - delta));

            Int tc2 = tc >> 1;
            if (bFilterSecondP)
            {
                Int delta1 = Clip3(-tc2, tc2, ((((m1 + m3 + 1) >> 1) - m2 + delta) >> 1));
                src[-offset * 2] = (Pel)ClipY((m2 + delta1));
            }
            if (bFilterSecondQ)
            {
                Int delta2 = Clip3(-tc2, tc2, ((((m6 + m4 + 1) >> 1) - m5 - delta) >> 1));
                src[offset] = (Pel)ClipY((m5 + delta2));
            }
        }
    }

    if (bPartPNoFilter)
    {
        src[-offset] = (Pel)m3;
        src[-offset * 2] = (Pel)m2;
        src[-offset * 3] = (Pel)m1;
    }
    if (bPartQNoFilter)
    {
        src[0] = (Pel)m4;
        src[offset] = (Pel)m5;
        src[offset * 2] = (Pel)m6;
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
inline Void TComLoopFilter::xPelFilterChroma(Pel* src, Int offset, Int tc, Bool bPartPNoFilter, Bool bPartQNoFilter)
{
    Int delta;

    Short m4  = (Short)src[0];
    Short m3  = (Short)src[-offset];
    Short m5  = (Short)src[offset];
    Short m2  = (Short)src[-offset * 2];

    delta = Clip3(-tc, tc, ((((m4 - m3) << 2) + m2 - m5 + 4) >> 3));
    src[-offset] = (Pel)ClipC(m3 + delta);
    src[0] = (Pel)ClipC(m4 - delta);

    if (bPartPNoFilter)
    {
        src[-offset] = (Pel)m3;
    }
    if (bPartQNoFilter)
    {
        src[0] = (Pel)m4;
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
inline Bool TComLoopFilter::xUseStrongFiltering(Int offset, Int d, Int beta, Int tc, Pel* src)
{
    Short m4  = (Short)src[0];
    Short m3  = (Short)src[-offset];
    Short m7  = (Short)src[offset * 3];
    Short m0  = (Short)src[-offset * 4];

    Int d_strong = abs(m0 - m3) + abs(m7 - m4);

    return (d_strong < (beta >> 3)) && (d < (beta >> 2)) && (abs(m3 - m4) < ((tc * 5 + 1) >> 1));
}

inline Int TComLoopFilter::xCalcDP(Pel* src, Int offset)
{
    return abs(static_cast<Int>(src[-offset * 3]) - 2 * src[-offset * 2] + src[-offset]);
}

inline Int TComLoopFilter::xCalcDQ(Pel* src, Int offset)
{
    return abs(static_cast<Int>(src[0]) - 2 * src[offset] + src[offset * 2]);
}

//! \}
