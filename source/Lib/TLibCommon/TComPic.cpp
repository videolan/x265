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

/** \file     TComPic.cpp
    \brief    picture class
*/

#include "TComPic.h"
#include "SEI.h"

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TComPic::TComPic()
    : m_uiTLayer(0)
    , m_bUsedByCurr(false)
    , m_bIsLongTerm(false)
    , m_bCheckLTMSB(false)
    , m_apcPicSym(NULL)
    , m_uiCurrSliceIdx(0)
    , m_pSliceSUMap(NULL)
{
    m_apcPicYuv[0] = NULL;
    m_apcPicYuv[1] = NULL;
}

TComPic::~TComPic()
{
}

Void TComPic::create(Int iWidth, Int iHeight, UInt uiMaxWidth, UInt uiMaxHeight, UInt uiMaxDepth, Window &conformanceWindow, Window &defaultDisplayWindow,
                     Int *numReorderPics, Bool bIsVirtual)
{
    m_apcPicSym = new TComPicSym;
    m_apcPicSym->create(iWidth, iHeight, uiMaxWidth, uiMaxHeight, uiMaxDepth);
    if (!bIsVirtual)
    {
        m_apcPicYuv[0] = new TComPicYuv;
        m_apcPicYuv[0]->create(iWidth, iHeight, uiMaxWidth, uiMaxHeight, uiMaxDepth);
    }
    m_apcPicYuv[1] = new TComPicYuv;
    m_apcPicYuv[1]->create(iWidth, iHeight, uiMaxWidth, uiMaxHeight, uiMaxDepth);

    // there are no SEI messages associated with this picture initially
    if (m_SEIs.size() > 0)
    {
        deleteSEIs(m_SEIs);
    }
    m_bUsedByCurr = false;

    /* store conformance window parameters with picture */
    m_conformanceWindow = conformanceWindow;

    /* store display window parameters with picture */
    m_defaultDisplayWindow = defaultDisplayWindow;

    /* store number of reorder pics with picture */
    memcpy(m_numReorderPics, numReorderPics, MAX_TLAYER * sizeof(Int));
}

Void TComPic::destroy()
{
    if (m_apcPicSym)
    {
        m_apcPicSym->destroy();
        delete m_apcPicSym;
        m_apcPicSym = NULL;
    }

    if (m_apcPicYuv[0])
    {
        m_apcPicYuv[0]->destroy();
        delete m_apcPicYuv[0];
        m_apcPicYuv[0] = NULL;
    }

    if (m_apcPicYuv[1])
    {
        m_apcPicYuv[1]->destroy();
        delete m_apcPicYuv[1];
        m_apcPicYuv[1] = NULL;
    }

    deleteSEIs(m_SEIs);
}

Void TComPic::compressMotion()
{
    TComPicSym* pPicSym = getPicSym();

    for (UInt uiCUAddr = 0; uiCUAddr < pPicSym->getFrameHeightInCU() * pPicSym->getFrameWidthInCU(); uiCUAddr++)
    {
        TComDataCU* pcCU = pPicSym->getCU(uiCUAddr);
        pcCU->compressMV();
    }
}

/** Create non-deblocked filter information
 * \param pSliceStartAddress array for storing slice start addresses
 * \param numSlices number of slices in picture
 * \param sliceGranularityDepth slice granularity
 * \param bNDBFilterCrossSliceBoundary cross-slice-boundary in-loop filtering; true for "cross".
 * \param numTiles number of tiles in picture
 * \param bNDBFilterCrossTileBoundary cross-tile-boundary in-loop filtering; true for "cross".
 */
Void TComPic::createNonDBFilterInfo(Int lastSliceCUAddr, Int sliceGranularityDepth, Bool bNDBFilterCrossTileBoundary)
{
    UInt maxNumSUInLCU = getNumPartInCU();
    UInt numLCUInPic   = getNumCUsInFrame();
    UInt picWidth      = getSlice(0)->getSPS()->getPicWidthInLumaSamples();
    UInt picHeight     = getSlice(0)->getSPS()->getPicHeightInLumaSamples();
    Int  numLCUsInPicWidth = getFrameWidthInCU();
    Int  numLCUsInPicHeight = getFrameHeightInCU();
    UInt maxNumSUInLCUWidth = getNumPartInWidth();
    UInt maxNumSUInLCUHeight = getNumPartInHeight();

    m_pSliceSUMap = new Int[maxNumSUInLCU * numLCUInPic];
    for (UInt i = 0; i < (maxNumSUInLCU * numLCUInPic); i++)
    {
        m_pSliceSUMap[i] = -1;
    }

    for (UInt CUAddr = 0; CUAddr < numLCUInPic; CUAddr++)
    {
        TComDataCU* pcCU = getCU(CUAddr);
        pcCU->setSliceSUMap(m_pSliceSUMap + (CUAddr * maxNumSUInLCU));
        pcCU->getNDBFilterBlocks()->clear();
    }

    UInt startAddr, endAddr, firstCUInStartLCU, startLCU, endLCU, lastCUInEndLCU, uiAddr;
    UInt LPelX, TPelY, LCUX, LCUY;
    UInt currSU;
    UInt startSU, endSU;

    //1st step: decide the real start address
    startAddr = 0;
    endAddr   = lastSliceCUAddr-1;

    startLCU            = startAddr / maxNumSUInLCU;
    firstCUInStartLCU   = startAddr % maxNumSUInLCU;

    endLCU              = endAddr   / maxNumSUInLCU;
    lastCUInEndLCU      = endAddr   % maxNumSUInLCU;

    uiAddr = (startLCU);

    LCUX      = getCU(uiAddr)->getCUPelX();
    LCUY      = getCU(uiAddr)->getCUPelY();
    LPelX     = LCUX + g_auiRasterToPelX[g_auiZscanToRaster[firstCUInStartLCU]];
    TPelY     = LCUY + g_auiRasterToPelY[g_auiZscanToRaster[firstCUInStartLCU]];
    currSU    = firstCUInStartLCU;

    Bool bMoveToNextLCU = false;
    Bool bSliceInOneLCU = (startLCU == endLCU);

    while (!(LPelX < picWidth) || !(TPelY < picHeight))
    {
        currSU++;

        if (currSU >= maxNumSUInLCU)
        {
            bMoveToNextLCU = true;
            break;
        }

        LPelX = LCUX + g_auiRasterToPelX[g_auiZscanToRaster[currSU]];
        TPelY = LCUY + g_auiRasterToPelY[g_auiZscanToRaster[currSU]];
    }

    if (currSU != firstCUInStartLCU)
    {
        if (!bMoveToNextLCU)
        {
            firstCUInStartLCU = currSU;
        }
        else
        {
            startLCU++;
            firstCUInStartLCU = 0;
            assert(startLCU < getNumCUsInFrame());
        }
        assert(startLCU * maxNumSUInLCU + firstCUInStartLCU < endAddr);
    }

    //2nd step: assign NonDBFilterInfo to each processing block
    std::vector<TComDataCU*> vSliceCUDataLink;
    vSliceCUDataLink.reserve(endLCU-startLCU+1);
    for (UInt i = startLCU; i <= endLCU; i++)
    {
        startSU = (i == startLCU) ? (firstCUInStartLCU) : (0);
        endSU   = (i == endLCU) ? (lastCUInEndLCU) : (maxNumSUInLCU - 1);

        TComDataCU* pcCU = getCU(i);
        vSliceCUDataLink.push_back(pcCU);

        createNonDBFilterInfoLCU(0, pcCU, startSU, endSU, sliceGranularityDepth, picWidth, picHeight);
    }

    //step 3: border availability
    for (Int i = 0; i < vSliceCUDataLink.size(); i++)
    {
        TComDataCU* pcCU = vSliceCUDataLink[i];
        uiAddr = pcCU->getAddr();

        if (pcCU->getPic() == 0)
        {
            continue;
        }

        pcCU->setNDBFilterBlockBorderAvailability(numLCUsInPicWidth, numLCUsInPicHeight, maxNumSUInLCUWidth, maxNumSUInLCUHeight, picWidth, picHeight, false, false, false, false, false);
    }
}

/** Create non-deblocked filter information for LCU
 * \param tileID tile index
 * \param sliceID slice index
 * \param pcCU CU data pointer
 * \param startSU start SU index in LCU
 * \param endSU end SU index in LCU
 * \param sliceGranularyDepth slice granularity
 * \param picWidth picture width
 * \param picHeight picture height
 */
Void TComPic::createNonDBFilterInfoLCU(Int sliceID, TComDataCU* pcCU, UInt startSU, UInt endSU, Int sliceGranularyDepth, UInt picWidth, UInt picHeight)
{
    UInt LCUX          = pcCU->getCUPelX();
    UInt LCUY          = pcCU->getCUPelY();
    Int* pCUSliceMap    = pcCU->getSliceSUMap();
    UInt maxNumSUInLCU = getNumPartInCU();
    UInt maxNumSUInSGU = maxNumSUInLCU >> (sliceGranularyDepth << 1);
    UInt maxNumSUInLCUWidth = getNumPartInWidth();
    UInt LPelX, TPelY;
    UInt currSU;

    //get the number of valid NBFilterBLock
    currSU   = startSU;
    while (currSU <= endSU)
    {
        LPelX = LCUX + g_auiRasterToPelX[g_auiZscanToRaster[currSU]];
        TPelY = LCUY + g_auiRasterToPelY[g_auiZscanToRaster[currSU]];

        while (!(LPelX < picWidth) || !(TPelY < picHeight))
        {
            currSU += maxNumSUInSGU;
            if (currSU >= maxNumSUInLCU || currSU > endSU)
            {
                break;
            }
            LPelX = LCUX + g_auiRasterToPelX[g_auiZscanToRaster[currSU]];
            TPelY = LCUY + g_auiRasterToPelY[g_auiZscanToRaster[currSU]];
        }

        if (currSU >= maxNumSUInLCU || currSU > endSU)
        {
            break;
        }

        NDBFBlockInfo NDBFBlock;

        NDBFBlock.sliceID = sliceID;
        NDBFBlock.posY    = TPelY;
        NDBFBlock.posX    = LPelX;
        NDBFBlock.startSU = currSU;

        UInt uiLastValidSU  = currSU;
        UInt uiIdx, uiLPelX_su, uiTPelY_su;
        for (uiIdx = currSU; uiIdx < currSU + maxNumSUInSGU; uiIdx++)
        {
            if (uiIdx > endSU)
            {
                break;
            }
            uiLPelX_su   = LCUX + g_auiRasterToPelX[g_auiZscanToRaster[uiIdx]];
            uiTPelY_su   = LCUY + g_auiRasterToPelY[g_auiZscanToRaster[uiIdx]];
            if (!(uiLPelX_su < picWidth) || !(uiTPelY_su < picHeight))
            {
                continue;
            }
            pCUSliceMap[uiIdx] = sliceID;
            uiLastValidSU = uiIdx;
        }

        NDBFBlock.endSU = uiLastValidSU;

        UInt rTLSU = g_auiZscanToRaster[NDBFBlock.startSU];
        UInt rBRSU = g_auiZscanToRaster[NDBFBlock.endSU];
        NDBFBlock.widthSU  = (rBRSU % maxNumSUInLCUWidth) - (rTLSU % maxNumSUInLCUWidth) + 1;
        NDBFBlock.heightSU = (UInt)(rBRSU / maxNumSUInLCUWidth) - (UInt)(rTLSU / maxNumSUInLCUWidth) + 1;
        NDBFBlock.width    = NDBFBlock.widthSU  * getMinCUWidth();
        NDBFBlock.height   = NDBFBlock.heightSU * getMinCUHeight();

        pcCU->getNDBFilterBlocks()->push_back(NDBFBlock);

        currSU += maxNumSUInSGU;
    }
}

/** destroy non-deblocked filter information for LCU
 */
Void TComPic::destroyNonDBFilterInfo()
{
    if (m_pSliceSUMap != NULL)
    {
        delete[] m_pSliceSUMap;
        m_pSliceSUMap = NULL;
    }
    for (UInt CUAddr = 0; CUAddr < getNumCUsInFrame(); CUAddr++)
    {
        TComDataCU* pcCU = getCU(CUAddr);
        pcCU->getNDBFilterBlocks()->clear();
    }
}

//! \}
