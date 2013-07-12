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
    : m_uiNumPartitions(0)
    , m_bLFCrossTileBoundary(true)
{
    for (UInt uiDir = 0; uiDir < 2; uiDir++)
    {
        m_aapucBS[uiDir] = NULL;
        m_aapbEdgeFilter[uiDir] = NULL;
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

Void TComLoopFilter::create(UInt uiMaxCUDepth)
{
    destroy();
    m_uiNumPartitions = 1 << (uiMaxCUDepth << 1);
    for (UInt uiDir = 0; uiDir < 2; uiDir++)
    {
        m_aapucBS[uiDir] = new UChar[m_uiNumPartitions];
        m_aapbEdgeFilter[uiDir] = new Bool[m_uiNumPartitions];
    }
}

Void TComLoopFilter::destroy()
{
    for (UInt uiDir = 0; uiDir < 2; uiDir++)
    {
        if (m_aapucBS)
        {
            delete [] m_aapucBS[uiDir];
            m_aapucBS[uiDir] = NULL;
        }
        if (m_aapbEdgeFilter[uiDir])
        {
            delete [] m_aapbEdgeFilter[uiDir];
            m_aapbEdgeFilter[uiDir] = NULL;
        }
    }
}

/**
 - call deblocking function for every CU
 .
 \param  pcPic   picture class (TComPic) pointer
 */
Void TComLoopFilter::loopFilterPic(TComPic* pcPic)
{
    // Horizontal filtering
    for (UInt cuAddr = 0; cuAddr < pcPic->getNumCUsInFrame(); cuAddr++)
    {
        TComDataCU* cu = pcPic->getCU(cuAddr);

        ::memset(m_aapucBS[EDGE_VER], 0, sizeof(UChar) * m_uiNumPartitions);
        ::memset(m_aapbEdgeFilter[EDGE_VER], 0, sizeof(Bool) * m_uiNumPartitions);

        // CU-based deblocking
        xDeblockCU(cu, 0, 0, EDGE_VER);
    }

    // Vertical filtering
    for (UInt cuAddr = 0; cuAddr < pcPic->getNumCUsInFrame(); cuAddr++)
    {
        TComDataCU* cu = pcPic->getCU(cuAddr);

        ::memset(m_aapucBS[EDGE_HOR], 0, sizeof(UChar) * m_uiNumPartitions);
        ::memset(m_aapbEdgeFilter[EDGE_HOR], 0, sizeof(Bool) * m_uiNumPartitions);

        // CU-based deblocking
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
Void TComLoopFilter::xDeblockCU(TComDataCU* cu, UInt absZOrderIdx, UInt depth, Int Edge)
{
    if (cu->getPic() == 0 || cu->getPartitionSize(absZOrderIdx) == SIZE_NONE)
    {
        return;
    }
    TComPic* pcPic     = cu->getPic();
    UInt uiCurNumParts = pcPic->getNumPartInCU() >> (depth << 1);
    UInt qNumParts   = uiCurNumParts >> 2;

    if (cu->getDepth(absZOrderIdx) > depth)
    {
        for (UInt partIdx = 0; partIdx < 4; partIdx++, absZOrderIdx += qNumParts)
        {
            UInt lpelx   = cu->getCUPelX() + g_rasterToPelX[g_zscanToRaster[absZOrderIdx]];
            UInt tpely   = cu->getCUPelY() + g_rasterToPelY[g_zscanToRaster[absZOrderIdx]];
            if ((lpelx < cu->getSlice()->getSPS()->getPicWidthInLumaSamples()) && (tpely < cu->getSlice()->getSPS()->getPicHeightInLumaSamples()))
            {
                xDeblockCU(cu, absZOrderIdx, depth + 1, Edge);
            }
        }

        return;
    }

    xSetLoopfilterParam(cu, absZOrderIdx);

    xSetEdgefilterTU(cu, absZOrderIdx, absZOrderIdx, depth);
    xSetEdgefilterPU(cu, absZOrderIdx);

    Int dir = Edge;
    for (UInt partIdx = absZOrderIdx; partIdx < absZOrderIdx + uiCurNumParts; partIdx++)
    {
        UInt uiBSCheck;
        if ((g_maxCUWidth >> g_maxCUDepth) == 4)
        {
            uiBSCheck = (dir == EDGE_VER && partIdx % 2 == 0) || (dir == EDGE_HOR && (partIdx - ((partIdx >> 2) << 2)) / 2 == 0);
        }
        else
        {
            uiBSCheck = 1;
        }

        if (m_aapbEdgeFilter[dir][partIdx] && uiBSCheck)
        {
            xGetBoundaryStrengthSingle(cu, dir, partIdx);
        }
    }

    UInt uiPelsInPart = g_maxCUWidth >> g_maxCUDepth;
    UInt PartIdxIncr = DEBLOCK_SMALLEST_BLOCK / uiPelsInPart ? DEBLOCK_SMALLEST_BLOCK / uiPelsInPart : 1;

    UInt uiSizeInPU = pcPic->getNumPartInWidth() >> (depth);

    for (UInt iEdge = 0; iEdge < uiSizeInPU; iEdge += PartIdxIncr)
    {
        xEdgeFilterLuma(cu, absZOrderIdx, depth, dir, iEdge);
        if ((uiPelsInPart > DEBLOCK_SMALLEST_BLOCK) || (iEdge % ((DEBLOCK_SMALLEST_BLOCK << 1) / uiPelsInPart)) == 0)
        {
            xEdgeFilterChroma(cu, absZOrderIdx, depth, dir, iEdge);
        }
    }
}

Void TComLoopFilter::xSetEdgefilterMultiple(TComDataCU* cu, UInt uiScanIdx, UInt depth, Int dir, Int iEdgeIdx, Bool bValue, UInt uiWidthInBaseUnits, UInt uiHeightInBaseUnits)
{
    if (uiWidthInBaseUnits == 0)
    {
        uiWidthInBaseUnits  = cu->getPic()->getNumPartInWidth() >> depth;
    }
    if (uiHeightInBaseUnits == 0)
    {
        uiHeightInBaseUnits = cu->getPic()->getNumPartInHeight() >> depth;
    }
    const UInt uiNumElem = dir == 0 ? uiHeightInBaseUnits : uiWidthInBaseUnits;
    assert(uiNumElem > 0);
    assert(uiWidthInBaseUnits > 0);
    assert(uiHeightInBaseUnits > 0);
    for (UInt ui = 0; ui < uiNumElem; ui++)
    {
        const UInt uiBsIdx = xCalcBsIdx(cu, uiScanIdx, dir, iEdgeIdx, ui);
        m_aapbEdgeFilter[dir][uiBsIdx] = bValue;
        if (iEdgeIdx == 0)
        {
            m_aapucBS[dir][uiBsIdx] = bValue;
        }
    }
}

Void TComLoopFilter::xSetEdgefilterTU(TComDataCU* cu, UInt absTUPartIdx, UInt absZOrderIdx, UInt depth)
{
    if (cu->getTransformIdx(absZOrderIdx) + cu->getDepth(absZOrderIdx) > depth)
    {
        const UInt uiCurNumParts = cu->getPic()->getNumPartInCU() >> (depth << 1);
        const UInt qNumParts   = uiCurNumParts >> 2;
        for (UInt partIdx = 0; partIdx < 4; partIdx++, absZOrderIdx += qNumParts)
        {
            UInt nsAddr = absZOrderIdx;
            xSetEdgefilterTU(cu, nsAddr, absZOrderIdx, depth + 1);
        }

        return;
    }

    Int trWidth  = cu->getWidth(absZOrderIdx) >> cu->getTransformIdx(absZOrderIdx);
    Int trHeight = cu->getHeight(absZOrderIdx) >> cu->getTransformIdx(absZOrderIdx);

    UInt uiWidthInBaseUnits  = trWidth / (g_maxCUWidth >> g_maxCUDepth);
    UInt uiHeightInBaseUnits = trHeight / (g_maxCUWidth >> g_maxCUDepth);

    xSetEdgefilterMultiple(cu, absTUPartIdx, depth, EDGE_VER, 0, m_stLFCUParam.bInternalEdge, uiWidthInBaseUnits, uiHeightInBaseUnits);
    xSetEdgefilterMultiple(cu, absTUPartIdx, depth, EDGE_HOR, 0, m_stLFCUParam.bInternalEdge, uiWidthInBaseUnits, uiHeightInBaseUnits);
}

Void TComLoopFilter::xSetEdgefilterPU(TComDataCU* cu, UInt absZOrderIdx)
{
    const UInt depth = cu->getDepth(absZOrderIdx);
    const UInt uiWidthInBaseUnits  = cu->getPic()->getNumPartInWidth() >> depth;
    const UInt uiHeightInBaseUnits = cu->getPic()->getNumPartInHeight() >> depth;
    const UInt uiHWidthInBaseUnits  = uiWidthInBaseUnits  >> 1;
    const UInt uiHHeightInBaseUnits = uiHeightInBaseUnits >> 1;
    const UInt uiQWidthInBaseUnits  = uiWidthInBaseUnits  >> 2;
    const UInt uiQHeightInBaseUnits = uiHeightInBaseUnits >> 2;

    xSetEdgefilterMultiple(cu, absZOrderIdx, depth, EDGE_VER, 0, m_stLFCUParam.bLeftEdge);
    xSetEdgefilterMultiple(cu, absZOrderIdx, depth, EDGE_HOR, 0, m_stLFCUParam.bTopEdge);

    switch (cu->getPartitionSize(absZOrderIdx))
    {
    case SIZE_2Nx2N:
    {
        break;
    }
    case SIZE_2NxN:
    {
        xSetEdgefilterMultiple(cu, absZOrderIdx, depth, EDGE_HOR, uiHHeightInBaseUnits, m_stLFCUParam.bInternalEdge);
        break;
    }
    case SIZE_Nx2N:
    {
        xSetEdgefilterMultiple(cu, absZOrderIdx, depth, EDGE_VER, uiHWidthInBaseUnits, m_stLFCUParam.bInternalEdge);
        break;
    }
    case SIZE_NxN:
    {
        xSetEdgefilterMultiple(cu, absZOrderIdx, depth, EDGE_VER, uiHWidthInBaseUnits, m_stLFCUParam.bInternalEdge);
        xSetEdgefilterMultiple(cu, absZOrderIdx, depth, EDGE_HOR, uiHHeightInBaseUnits, m_stLFCUParam.bInternalEdge);
        break;
    }
    case SIZE_2NxnU:
    {
        xSetEdgefilterMultiple(cu, absZOrderIdx, depth, EDGE_HOR, uiQHeightInBaseUnits, m_stLFCUParam.bInternalEdge);
        break;
    }
    case SIZE_2NxnD:
    {
        xSetEdgefilterMultiple(cu, absZOrderIdx, depth, EDGE_HOR, uiHeightInBaseUnits - uiQHeightInBaseUnits, m_stLFCUParam.bInternalEdge);
        break;
    }
    case SIZE_nLx2N:
    {
        xSetEdgefilterMultiple(cu, absZOrderIdx, depth, EDGE_VER, uiQWidthInBaseUnits, m_stLFCUParam.bInternalEdge);
        break;
    }
    case SIZE_nRx2N:
    {
        xSetEdgefilterMultiple(cu, absZOrderIdx, depth, EDGE_VER, uiWidthInBaseUnits - uiQWidthInBaseUnits, m_stLFCUParam.bInternalEdge);
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
    UInt uiX           = cu->getCUPelX() + g_rasterToPelX[g_zscanToRaster[absZOrderIdx]];
    UInt uiY           = cu->getCUPelY() + g_rasterToPelY[g_zscanToRaster[absZOrderIdx]];

    TComDataCU* pcTempCU;
    UInt        uiTempPartIdx;

    m_stLFCUParam.bInternalEdge = !cu->getSlice()->getDeblockingFilterDisable();

    if ((uiX == 0) || cu->getSlice()->getDeblockingFilterDisable())
    {
        m_stLFCUParam.bLeftEdge = false;
    }
    else
    {
        m_stLFCUParam.bLeftEdge = true;
    }
    if (m_stLFCUParam.bLeftEdge)
    {
        pcTempCU = cu->getPULeft(uiTempPartIdx, absZOrderIdx, !true, !m_bLFCrossTileBoundary);
        if (pcTempCU)
        {
            m_stLFCUParam.bLeftEdge = true;
        }
        else
        {
            m_stLFCUParam.bLeftEdge = false;
        }
    }

    if ((uiY == 0) || cu->getSlice()->getDeblockingFilterDisable())
    {
        m_stLFCUParam.bTopEdge = false;
    }
    else
    {
        m_stLFCUParam.bTopEdge = true;
    }
    if (m_stLFCUParam.bTopEdge)
    {
        pcTempCU = cu->getPUAbove(uiTempPartIdx, absZOrderIdx, !true, false, !m_bLFCrossTileBoundary);

        if (pcTempCU)
        {
            m_stLFCUParam.bTopEdge = true;
        }
        else
        {
            m_stLFCUParam.bTopEdge = false;
        }
    }
}

Void TComLoopFilter::xGetBoundaryStrengthSingle(TComDataCU* cu, Int dir, UInt absPartIdx)
{
    TComSlice* const slice = cu->getSlice();

    const UInt uiPartQ = absPartIdx;
    TComDataCU* const pcCUQ = cu;

    UInt uiPartP;
    TComDataCU* pcCUP;
    UInt uiBs = 0;

    //-- Calculate Block Index
    if (dir == EDGE_VER)
    {
        pcCUP = pcCUQ->getPULeft(uiPartP, uiPartQ, !true, !m_bLFCrossTileBoundary);
    }
    else // (dir == EDGE_HOR)
    {
        pcCUP = pcCUQ->getPUAbove(uiPartP, uiPartQ, !true, false, !m_bLFCrossTileBoundary);
    }

    //-- Set BS for Intra MB : BS = 4 or 3
    if (pcCUP->isIntra(uiPartP) || pcCUQ->isIntra(uiPartQ))
    {
        uiBs = 2;
    }

    //-- Set BS for not Intra MB : BS = 2 or 1 or 0
    if (!pcCUP->isIntra(uiPartP) && !pcCUQ->isIntra(uiPartQ))
    {
        UInt nsPartQ = uiPartQ;
        UInt nsPartP = uiPartP;

        if (m_aapucBS[dir][absPartIdx] && (pcCUQ->getCbf(nsPartQ, TEXT_LUMA, pcCUQ->getTransformIdx(nsPartQ)) != 0 || pcCUP->getCbf(nsPartP, TEXT_LUMA, pcCUP->getTransformIdx(nsPartP)) != 0))
        {
            uiBs = 1;
        }
        else
        {
            if (dir == EDGE_HOR)
            {
                pcCUP = pcCUQ->getPUAbove(uiPartP, uiPartQ, !true, false, !m_bLFCrossTileBoundary);
            }
            if (slice->isInterB() || pcCUP->getSlice()->isInterB())
            {
                Int refIdx;
                TComPic *piRefP0, *piRefP1, *piRefQ0, *piRefQ1;
                refIdx = pcCUP->getCUMvField(REF_PIC_LIST_0)->getRefIdx(uiPartP);
                piRefP0 = (refIdx < 0) ? NULL : pcCUP->getSlice()->getRefPic(REF_PIC_LIST_0, refIdx);
                refIdx = pcCUP->getCUMvField(REF_PIC_LIST_1)->getRefIdx(uiPartP);
                piRefP1 = (refIdx < 0) ? NULL : pcCUP->getSlice()->getRefPic(REF_PIC_LIST_1, refIdx);
                refIdx = pcCUQ->getCUMvField(REF_PIC_LIST_0)->getRefIdx(uiPartQ);
                piRefQ0 = (refIdx < 0) ? NULL : slice->getRefPic(REF_PIC_LIST_0, refIdx);
                refIdx = pcCUQ->getCUMvField(REF_PIC_LIST_1)->getRefIdx(uiPartQ);
                piRefQ1 = (refIdx < 0) ? NULL : slice->getRefPic(REF_PIC_LIST_1, refIdx);

                MV pcMvP0 = pcCUP->getCUMvField(REF_PIC_LIST_0)->getMv(uiPartP);
                MV pcMvP1 = pcCUP->getCUMvField(REF_PIC_LIST_1)->getMv(uiPartP);
                MV pcMvQ0 = pcCUQ->getCUMvField(REF_PIC_LIST_0)->getMv(uiPartQ);
                MV pcMvQ1 = pcCUQ->getCUMvField(REF_PIC_LIST_1)->getMv(uiPartQ);

                if (piRefP0 == NULL) pcMvP0 = 0;
                if (piRefP1 == NULL) pcMvP1 = 0;
                if (piRefQ0 == NULL) pcMvQ0 = 0;
                if (piRefQ1 == NULL) pcMvQ1 = 0;

                if (((piRefP0 == piRefQ0) && (piRefP1 == piRefQ1)) || ((piRefP0 == piRefQ1) && (piRefP1 == piRefQ0)))
                {
                    uiBs = 0;
                    if (piRefP0 != piRefP1) // Different L0 & L1
                    {
                        if (piRefP0 == piRefQ0)
                        {
                            uiBs  = ((abs(pcMvQ0.x - pcMvP0.x) >= 4) ||
                                     (abs(pcMvQ0.y - pcMvP0.y) >= 4) ||
                                     (abs(pcMvQ1.x - pcMvP1.x) >= 4) ||
                                     (abs(pcMvQ1.y - pcMvP1.y) >= 4)) ? 1 : 0;
                        }
                        else
                        {
                            uiBs  = ((abs(pcMvQ1.x - pcMvP0.x) >= 4) ||
                                     (abs(pcMvQ1.y - pcMvP0.y) >= 4) ||
                                     (abs(pcMvQ0.x - pcMvP1.x) >= 4) ||
                                     (abs(pcMvQ0.y - pcMvP1.y) >= 4)) ? 1 : 0;
                        }
                    }
                    else // Same L0 & L1
                    {
                        uiBs  = ((abs(pcMvQ0.x - pcMvP0.x) >= 4) ||
                                 (abs(pcMvQ0.y - pcMvP0.y) >= 4) ||
                                 (abs(pcMvQ1.x - pcMvP1.x) >= 4) ||
                                 (abs(pcMvQ1.y - pcMvP1.y) >= 4)) &&
                            ((abs(pcMvQ1.x - pcMvP0.x) >= 4) ||
                             (abs(pcMvQ1.y - pcMvP0.y) >= 4) ||
                             (abs(pcMvQ0.x - pcMvP1.x) >= 4) ||
                             (abs(pcMvQ0.y - pcMvP1.y) >= 4)) ? 1 : 0;
                    }
                }
                else // for all different Ref_Idx
                {
                    uiBs = 1;
                }
            }
            else // slice->isInterP()
            {
                Int refIdx;
                TComPic *piRefP0, *piRefQ0;
                refIdx = pcCUP->getCUMvField(REF_PIC_LIST_0)->getRefIdx(uiPartP);
                piRefP0 = (refIdx < 0) ? NULL : pcCUP->getSlice()->getRefPic(REF_PIC_LIST_0, refIdx);
                refIdx = pcCUQ->getCUMvField(REF_PIC_LIST_0)->getRefIdx(uiPartQ);
                piRefQ0 = (refIdx < 0) ? NULL : slice->getRefPic(REF_PIC_LIST_0, refIdx);
                MV pcMvP0 = pcCUP->getCUMvField(REF_PIC_LIST_0)->getMv(uiPartP);
                MV pcMvQ0 = pcCUQ->getCUMvField(REF_PIC_LIST_0)->getMv(uiPartQ);

                if (piRefP0 == NULL) pcMvP0 = 0;
                if (piRefQ0 == NULL) pcMvQ0 = 0;

                uiBs = ((piRefP0 != piRefQ0) ||
                        (abs(pcMvQ0.x - pcMvP0.x) >= 4) ||
                        (abs(pcMvQ0.y - pcMvP0.y) >= 4)) ? 1 : 0;
            }
        } // enf of "if( one of BCBP == 0 )"
    } // enf of "if( not Intra )"

    m_aapucBS[dir][absPartIdx] = uiBs;
}

Void TComLoopFilter::xEdgeFilterLuma(TComDataCU* cu, UInt absZOrderIdx, UInt depth, Int dir, Int iEdge)
{
    TComPicYuv* pcPicYuvRec = cu->getPic()->getPicYuvRec();
    Pel* piSrc    = pcPicYuvRec->getLumaAddr(cu->getAddr(), absZOrderIdx);
    Pel* piTmpSrc = piSrc;

    Int  stride = pcPicYuvRec->getStride();
    Int qp = 0;
    Int iQP_P = 0;
    Int iQP_Q = 0;
    UInt uiNumParts = cu->getPic()->getNumPartInWidth() >> depth;

    UInt  uiPelsInPart = g_maxCUWidth >> g_maxCUDepth;
    UInt  uiBsAbsIdx = 0, uiBs = 0;
    Int   iOffset, iSrcStep;

    Bool  bPCMFilter = (cu->getSlice()->getSPS()->getUsePCM() && cu->getSlice()->getSPS()->getPCMFilterDisableFlag()) ? true : false;
    Bool  bPartPNoFilter = false;
    Bool  bPartQNoFilter = false;
    UInt  uiPartPIdx = 0;
    UInt  uiPartQIdx = 0;
    TComDataCU* pcCUP = cu;
    TComDataCU* pcCUQ = cu;
    Int  betaOffsetDiv2 = pcCUQ->getSlice()->getDeblockingFilterBetaOffsetDiv2();
    Int  tcOffsetDiv2 = pcCUQ->getSlice()->getDeblockingFilterTcOffsetDiv2();

    if (dir == EDGE_VER)
    {
        iOffset = 1;
        iSrcStep = stride;
        piTmpSrc += iEdge * uiPelsInPart;
    }
    else // (dir == EDGE_HOR)
    {
        iOffset = stride;
        iSrcStep = 1;
        piTmpSrc += iEdge * uiPelsInPart * stride;
    }

    for (UInt idx = 0; idx < uiNumParts; idx++)
    {
        uiBsAbsIdx = xCalcBsIdx(cu, absZOrderIdx, dir, iEdge, idx);
        uiBs = m_aapucBS[dir][uiBsAbsIdx];
        if (uiBs)
        {
            iQP_Q = cu->getQP(uiBsAbsIdx);
            uiPartQIdx = uiBsAbsIdx;
            // Derive neighboring PU index
            if (dir == EDGE_VER)
            {
                pcCUP = pcCUQ->getPULeft(uiPartPIdx, uiPartQIdx, !true, !m_bLFCrossTileBoundary);
            }
            else // (dir == EDGE_HOR)
            {
                pcCUP = pcCUQ->getPUAbove(uiPartPIdx, uiPartQIdx, !true, false, !m_bLFCrossTileBoundary);
            }

            iQP_P = pcCUP->getQP(uiPartPIdx);
            qp = (iQP_P + iQP_Q + 1) >> 1;
            Int iBitdepthScale = 1 << (g_bitDepthY - 8);

            Int iIndexTC = Clip3(0, MAX_QP + DEFAULT_INTRA_TC_OFFSET, Int(qp + DEFAULT_INTRA_TC_OFFSET * (uiBs - 1) + (tcOffsetDiv2 << 1)));
            Int iIndexB = Clip3(0, MAX_QP, qp + (betaOffsetDiv2 << 1));

            Int iTc =  sm_tcTable[iIndexTC] * iBitdepthScale;
            Int iBeta = sm_betaTable[iIndexB] * iBitdepthScale;
            Int iSideThreshold = (iBeta + (iBeta >> 1)) >> 3;
            Int iThrCut = iTc * 10;

            UInt  uiBlocksInPart = uiPelsInPart / 4 ? uiPelsInPart / 4 : 1;
            for (UInt iBlkIdx = 0; iBlkIdx < uiBlocksInPart; iBlkIdx++)
            {
                Int dp0 = xCalcDP(piTmpSrc + iSrcStep * (idx * uiPelsInPart + iBlkIdx * 4 + 0), iOffset);
                Int dq0 = xCalcDQ(piTmpSrc + iSrcStep * (idx * uiPelsInPart + iBlkIdx * 4 + 0), iOffset);
                Int dp3 = xCalcDP(piTmpSrc + iSrcStep * (idx * uiPelsInPart + iBlkIdx * 4 + 3), iOffset);
                Int dq3 = xCalcDQ(piTmpSrc + iSrcStep * (idx * uiPelsInPart + iBlkIdx * 4 + 3), iOffset);
                Int d0 = dp0 + dq0;
                Int d3 = dp3 + dq3;

                Int dp = dp0 + dp3;
                Int dq = dq0 + dq3;
                Int d =  d0 + d3;

                if (bPCMFilter || cu->getSlice()->getPPS()->getTransquantBypassEnableFlag())
                {
                    // Check if each of PUs is I_PCM with LF disabling
                    bPartPNoFilter = (bPCMFilter && pcCUP->getIPCMFlag(uiPartPIdx));
                    bPartQNoFilter = (bPCMFilter && pcCUQ->getIPCMFlag(uiPartQIdx));

                    // check if each of PUs is lossless coded
                    bPartPNoFilter = bPartPNoFilter || (pcCUP->isLosslessCoded(uiPartPIdx));
                    bPartQNoFilter = bPartQNoFilter || (pcCUQ->isLosslessCoded(uiPartQIdx));
                }

                if (d < iBeta)
                {
                    Bool bFilterP = (dp < iSideThreshold);
                    Bool bFilterQ = (dq < iSideThreshold);

                    Bool sw =  xUseStrongFiltering(iOffset, 2 * d0, iBeta, iTc, piTmpSrc + iSrcStep * (idx * uiPelsInPart + iBlkIdx * 4 + 0))
                        && xUseStrongFiltering(iOffset, 2 * d3, iBeta, iTc, piTmpSrc + iSrcStep * (idx * uiPelsInPart + iBlkIdx * 4 + 3));

                    for (Int i = 0; i < DEBLOCK_SMALLEST_BLOCK / 2; i++)
                    {
                        xPelFilterLuma(piTmpSrc + iSrcStep * (idx * uiPelsInPart + iBlkIdx * 4 + i), iOffset, iTc, sw, bPartPNoFilter, bPartQNoFilter, iThrCut, bFilterP, bFilterQ);
                    }
                }
            }
        }
    }
}

Void TComLoopFilter::xEdgeFilterChroma(TComDataCU* cu, UInt absZOrderIdx, UInt depth, Int dir, Int iEdge)
{
    TComPicYuv* pcPicYuvRec = cu->getPic()->getPicYuvRec();
    Int         stride     = pcPicYuvRec->getCStride();
    Pel*        piSrcCb     = pcPicYuvRec->getCbAddr(cu->getAddr(), absZOrderIdx);
    Pel*        piSrcCr     = pcPicYuvRec->getCrAddr(cu->getAddr(), absZOrderIdx);
    Int qp = 0;
    Int iQP_P = 0;
    Int iQP_Q = 0;

    UInt  uiPelsInPartChroma = g_maxCUWidth >> (g_maxCUDepth + 1);

    Int   iOffset, iSrcStep;

    const UInt uiLCUWidthInBaseUnits = cu->getPic()->getNumPartInWidth();

    Bool  bPCMFilter = (cu->getSlice()->getSPS()->getUsePCM() && cu->getSlice()->getSPS()->getPCMFilterDisableFlag()) ? true : false;
    Bool  bPartPNoFilter = false;
    Bool  bPartQNoFilter = false;
    UInt  uiPartPIdx;
    UInt  uiPartQIdx;
    TComDataCU* pcCUP;
    TComDataCU* pcCUQ = cu;
    Int tcOffsetDiv2 = cu->getSlice()->getDeblockingFilterTcOffsetDiv2();

    // Vertical Position
    UInt uiEdgeNumInLCUVert = g_zscanToRaster[absZOrderIdx] % uiLCUWidthInBaseUnits + iEdge;
    UInt uiEdgeNumInLCUHor = g_zscanToRaster[absZOrderIdx] / uiLCUWidthInBaseUnits + iEdge;

    if ((uiPelsInPartChroma < DEBLOCK_SMALLEST_BLOCK) && (((uiEdgeNumInLCUVert % (DEBLOCK_SMALLEST_BLOCK / uiPelsInPartChroma)) && (dir == 0)) || ((uiEdgeNumInLCUHor % (DEBLOCK_SMALLEST_BLOCK / uiPelsInPartChroma)) && dir)))
    {
        return;
    }

    UInt  uiNumParts = cu->getPic()->getNumPartInWidth() >> depth;

    UInt  uiBsAbsIdx;
    UChar ucBs;

    Pel* piTmpSrcCb = piSrcCb;
    Pel* piTmpSrcCr = piSrcCr;

    if (dir == EDGE_VER)
    {
        iOffset   = 1;
        iSrcStep  = stride;
        piTmpSrcCb += iEdge * uiPelsInPartChroma;
        piTmpSrcCr += iEdge * uiPelsInPartChroma;
    }
    else // (dir == EDGE_HOR)
    {
        iOffset   = stride;
        iSrcStep  = 1;
        piTmpSrcCb += iEdge * stride * uiPelsInPartChroma;
        piTmpSrcCr += iEdge * stride * uiPelsInPartChroma;
    }

    for (UInt idx = 0; idx < uiNumParts; idx++)
    {
        ucBs = 0;

        uiBsAbsIdx = xCalcBsIdx(cu, absZOrderIdx, dir, iEdge, idx);
        ucBs = m_aapucBS[dir][uiBsAbsIdx];

        if (ucBs > 1)
        {
            iQP_Q = cu->getQP(uiBsAbsIdx);
            uiPartQIdx = uiBsAbsIdx;
            // Derive neighboring PU index
            if (dir == EDGE_VER)
            {
                pcCUP = pcCUQ->getPULeft(uiPartPIdx, uiPartQIdx, !true, !m_bLFCrossTileBoundary);
            }
            else // (dir == EDGE_HOR)
            {
                pcCUP = pcCUQ->getPUAbove(uiPartPIdx, uiPartQIdx, !true, false, !m_bLFCrossTileBoundary);
            }

            iQP_P = pcCUP->getQP(uiPartPIdx);

            if (bPCMFilter || cu->getSlice()->getPPS()->getTransquantBypassEnableFlag())
            {
                // Check if each of PUs is I_PCM with LF disabling
                bPartPNoFilter = (bPCMFilter && pcCUP->getIPCMFlag(uiPartPIdx));
                bPartQNoFilter = (bPCMFilter && pcCUQ->getIPCMFlag(uiPartQIdx));

                // check if each of PUs is lossless coded
                bPartPNoFilter = bPartPNoFilter || (pcCUP->isLosslessCoded(uiPartPIdx));
                bPartQNoFilter = bPartQNoFilter || (pcCUQ->isLosslessCoded(uiPartQIdx));
            }

            for (UInt chromaIdx = 0; chromaIdx < 2; chromaIdx++)
            {
                Int chromaQPOffset  = (chromaIdx == 0) ? cu->getSlice()->getPPS()->getChromaCbQpOffset() : cu->getSlice()->getPPS()->getChromaCrQpOffset();
                Pel* piTmpSrcChroma = (chromaIdx == 0) ? piTmpSrcCb : piTmpSrcCr;

                qp = QpUV(((iQP_P + iQP_Q + 1) >> 1) + chromaQPOffset);
                Int iBitdepthScale = 1 << (g_bitDepthC - 8);

                Int iIndexTC = Clip3(0, MAX_QP + DEFAULT_INTRA_TC_OFFSET, qp + DEFAULT_INTRA_TC_OFFSET * (ucBs - 1) + (tcOffsetDiv2 << 1));
                Int iTc =  sm_tcTable[iIndexTC] * iBitdepthScale;

                for (UInt uiStep = 0; uiStep < uiPelsInPartChroma; uiStep++)
                {
                    xPelFilterChroma(piTmpSrcChroma + iSrcStep * (uiStep + idx * uiPelsInPartChroma), iOffset, iTc, bPartPNoFilter, bPartQNoFilter);
                }
            }
        }
    }
}

/**
 - Deblocking for the luminance component with strong or weak filter
 .
 \param piSrc           pointer to picture data
 \param iOffset         offset value for picture data
 \param tc              tc value
 \param sw              decision strong/weak filter
 \param bPartPNoFilter  indicator to disable filtering on partP
 \param bPartQNoFilter  indicator to disable filtering on partQ
 \param iThrCut         threshold value for weak filter decision
 \param bFilterSecondP  decision weak filter/no filter for partP
 \param bFilterSecondQ  decision weak filter/no filter for partQ
*/
inline Void TComLoopFilter::xPelFilterLuma(Pel* piSrc, Int iOffset, Int tc, Bool sw, Bool bPartPNoFilter, Bool bPartQNoFilter, Int iThrCut, Bool bFilterSecondP, Bool bFilterSecondQ)
{
    Int delta;

    Short m4  = (Short)piSrc[0];
    Short m3  = (Short)piSrc[-iOffset];
    Short m5  = (Short)piSrc[iOffset];
    Short m2  = (Short)piSrc[-iOffset * 2];
    Short m6  = (Short)piSrc[iOffset * 2];
    Short m1  = (Short)piSrc[-iOffset * 3];
    Short m7  = (Short)piSrc[iOffset * 3];
    Short m0  = (Short)piSrc[-iOffset * 4];

    if (sw)
    {
        piSrc[-iOffset]   = (Pel)Clip3(m3 - 2 * tc, m3 + 2 * tc, ((m1 + 2 * m2 + 2 * m3 + 2 * m4 + m5 + 4) >> 3));
        piSrc[0]          = (Pel)Clip3(m4 - 2 * tc, m4 + 2 * tc, ((m2 + 2 * m3 + 2 * m4 + 2 * m5 + m6 + 4) >> 3));
        piSrc[-iOffset * 2] = (Pel)Clip3(m2 - 2 * tc, m2 + 2 * tc, ((m1 + m2 + m3 + m4 + 2) >> 2));
        piSrc[iOffset]   = (Pel)Clip3(m5 - 2 * tc, m5 + 2 * tc, ((m3 + m4 + m5 + m6 + 2) >> 2));
        piSrc[-iOffset * 3] = (Pel)Clip3(m1 - 2 * tc, m1 + 2 * tc, ((2 * m0 + 3 * m1 + m2 + m3 + m4 + 4) >> 3));
        piSrc[iOffset * 2] = (Pel)Clip3(m6 - 2 * tc, m6 + 2 * tc, ((m3 + m4 + m5 + 3 * m6 + 2 * m7 + 4) >> 3));
    }
    else
    {
        /* Weak filter */
        delta = (9 * (m4 - m3) - 3 * (m5 - m2) + 8) >> 4;

        if (abs(delta) < iThrCut)
        {
            delta = Clip3(-tc, tc, delta);
            piSrc[-iOffset] = (Pel)ClipY((m3 + delta));
            piSrc[0] = (Pel)ClipY((m4 - delta));

            Int tc2 = tc >> 1;
            if (bFilterSecondP)
            {
                Int delta1 = Clip3(-tc2, tc2, ((((m1 + m3 + 1) >> 1) - m2 + delta) >> 1));
                piSrc[-iOffset * 2] = (Pel)ClipY((m2 + delta1));
            }
            if (bFilterSecondQ)
            {
                Int delta2 = Clip3(-tc2, tc2, ((((m6 + m4 + 1) >> 1) - m5 - delta) >> 1));
                piSrc[iOffset] = (Pel)ClipY((m5 + delta2));
            }
        }
    }

    if (bPartPNoFilter)
    {
        piSrc[-iOffset] = (Pel)m3;
        piSrc[-iOffset * 2] = (Pel)m2;
        piSrc[-iOffset * 3] = (Pel)m1;
    }
    if (bPartQNoFilter)
    {
        piSrc[0] = (Pel)m4;
        piSrc[iOffset] = (Pel)m5;
        piSrc[iOffset * 2] = (Pel)m6;
    }
}

/**
 - Deblocking of one line/column for the chrominance component
 .
 \param piSrc           pointer to picture data
 \param iOffset         offset value for picture data
 \param tc              tc value
 \param bPartPNoFilter  indicator to disable filtering on partP
 \param bPartQNoFilter  indicator to disable filtering on partQ
 */
inline Void TComLoopFilter::xPelFilterChroma(Pel* piSrc, Int iOffset, Int tc, Bool bPartPNoFilter, Bool bPartQNoFilter)
{
    Int delta;

    Short m4  = (Short)piSrc[0];
    Short m3  = (Short)piSrc[-iOffset];
    Short m5  = (Short)piSrc[iOffset];
    Short m2  = (Short)piSrc[-iOffset * 2];

    delta = Clip3(-tc, tc, ((((m4 - m3) << 2) + m2 - m5 + 4) >> 3));
    piSrc[-iOffset] = (Pel)ClipC(m3 + delta);
    piSrc[0] = (Pel)ClipC(m4 - delta);

    if (bPartPNoFilter)
    {
        piSrc[-iOffset] = (Pel)m3;
    }
    if (bPartQNoFilter)
    {
        piSrc[0] = (Pel)m4;
    }
}

/**
 - Decision between strong and weak filter
 .
 \param offset         offset value for picture data
 \param d               d value
 \param beta            beta value
 \param tc              tc value
 \param piSrc           pointer to picture data
 */
inline Bool TComLoopFilter::xUseStrongFiltering(Int offset, Int d, Int beta, Int tc, Pel* piSrc)
{
    Short m4  = (Short)piSrc[0];
    Short m3  = (Short)piSrc[-offset];
    Short m7  = (Short)piSrc[offset * 3];
    Short m0  = (Short)piSrc[-offset * 4];

    Int d_strong = abs(m0 - m3) + abs(m7 - m4);

    return (d_strong < (beta >> 3)) && (d < (beta >> 2)) && (abs(m3 - m4) < ((tc * 5 + 1) >> 1));
}

inline Int TComLoopFilter::xCalcDP(Pel* piSrc, Int iOffset)
{
    return abs(static_cast<Int>(piSrc[-iOffset * 3]) - 2 * piSrc[-iOffset * 2] + piSrc[-iOffset]);
}

inline Int TComLoopFilter::xCalcDQ(Pel* piSrc, Int iOffset)
{
    return abs(static_cast<Int>(piSrc[0]) - 2 * piSrc[iOffset] + piSrc[iOffset * 2]);
}

//! \}
