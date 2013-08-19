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
#include "mv.h"

using namespace x265;

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TComPic::TComPic()
    : m_picSym(NULL)
    , m_origPicYuv(NULL)
    , m_reconPicYuv(NULL)
    , m_bUsedByCurr(false)
    , m_bIsLongTerm(false)
    , m_bCheckLTMSB(false)
    , m_complete_enc(NULL)
{
    memset(&m_lowres, 0, sizeof(m_lowres));
}

TComPic::~TComPic()
{}

Void TComPic::create(Int width, Int height, UInt maxWidth, UInt maxHeight, UInt maxDepth, Window &conformanceWindow, Window &defaultDisplayWindow, Int bframes)
{
    m_picSym = new TComPicSym;
    m_picSym->create(width, height, maxWidth, maxHeight, maxDepth);

    m_origPicYuv = new TComPicYuv;
    m_origPicYuv->create(width, height, maxWidth, maxHeight, maxDepth);

    m_reconPicYuv = new TComPicYuv;
    m_reconPicYuv->create(width, height, maxWidth, maxHeight, maxDepth);

    /* store conformance window parameters with picture */
    m_conformanceWindow = conformanceWindow;

    /* store display window parameters with picture */
    m_defaultDisplayWindow = defaultDisplayWindow;

    /* configure lowres dimensions */
    m_lowres.create(this, bframes);

    int numRows = (height + maxHeight - 1) / maxHeight;
    m_complete_enc = new uint32_t[numRows]; // initial in FrameEncoder::encode()
}

Void TComPic::destroy()
{
    if (m_picSym)
    {
        m_picSym->destroy();
        delete m_picSym;
        m_picSym = NULL;
    }

    if (m_origPicYuv)
    {
        m_origPicYuv->destroy();
        delete m_origPicYuv;
        m_origPicYuv = NULL;
    }

    if (m_reconPicYuv)
    {
        m_reconPicYuv->destroy();
        delete m_reconPicYuv;
        m_reconPicYuv = NULL;
    }

    if (m_complete_enc)
    {
        delete[] m_complete_enc;
    }

    m_lowres.destroy();
}

Void TComPic::compressMotion()
{
    TComPicSym* sym = getPicSym();

    for (UInt cuAddr = 0; cuAddr < sym->getFrameHeightInCU() * sym->getFrameWidthInCU(); cuAddr++)
    {
        TComDataCU* cu = sym->getCU(cuAddr);
        cu->compressMV();
    }
}

/** Create non-deblocked filter information
 * \param pSliceStartAddress array for storing slice start addresses
 * \param numSlices number of slices in picture
 * \param sliceGranularityDepth slice granularity
 * \param bNDBFilterCrossSliceBoundary cross-slice-boundary in-loop filtering; true for "cross".
 * \param numTiles number of tiles in picture
 */
Void TComPic::createNonDBFilterInfo(Int lastSlicecuAddr, Int sliceGranularityDepth)
{
    UInt maxNumSUInLCU = getNumPartInCU();
    UInt numLCUInPic   = getNumCUsInFrame();
    UInt picWidth      = getSlice()->getSPS()->getPicWidthInLumaSamples();
    UInt picHeight     = getSlice()->getSPS()->getPicHeightInLumaSamples();
    Int  numLCUsInPicWidth = getFrameWidthInCU();
    Int  numLCUsInPicHeight = getFrameHeightInCU();
    UInt maxNumSUInLCUWidth = getNumPartInWidth();
    UInt maxNumSUInLCUHeight = getNumPartInHeight();

    for (UInt cuAddr = 0; cuAddr < numLCUInPic; cuAddr++)
    {
        TComDataCU* cu = getCU(cuAddr);
        cu->getNDBFilterBlocks()->clear();
    }

    UInt startAddr, endAddr, firstCUInStartLCU, startLCU, endLCU, lastCUInEndLCU, uiAddr;
    UInt LPelX, TPelY, LCUX, LCUY;
    UInt currSU;
    UInt startSU, endSU;

    //1st step: decide the real start address
    startAddr = 0;
    endAddr   = lastSlicecuAddr - 1;

    startLCU            = startAddr / maxNumSUInLCU;
    firstCUInStartLCU   = startAddr % maxNumSUInLCU;

    endLCU              = endAddr   / maxNumSUInLCU;
    lastCUInEndLCU      = endAddr   % maxNumSUInLCU;

    uiAddr = (startLCU);

    LCUX      = getCU(uiAddr)->getCUPelX();
    LCUY      = getCU(uiAddr)->getCUPelY();
    LPelX     = LCUX + g_rasterToPelX[g_zscanToRaster[firstCUInStartLCU]];
    TPelY     = LCUY + g_rasterToPelY[g_zscanToRaster[firstCUInStartLCU]];
    currSU    = firstCUInStartLCU;

    Bool bMoveToNextLCU = false;

    while (!(LPelX < picWidth) || !(TPelY < picHeight))
    {
        currSU++;

        if (currSU >= maxNumSUInLCU)
        {
            bMoveToNextLCU = true;
            break;
        }

        LPelX = LCUX + g_rasterToPelX[g_zscanToRaster[currSU]];
        TPelY = LCUY + g_rasterToPelY[g_zscanToRaster[currSU]];
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
    for (UInt i = startLCU; i <= endLCU; i++)
    {
        startSU = (i == startLCU) ? (firstCUInStartLCU) : (0);
        endSU   = (i == endLCU) ? (lastCUInEndLCU) : (maxNumSUInLCU - 1);
        TComDataCU* cu = getCU(i);
        createNonDBFilterInfoLCU(0, cu, startSU, endSU, sliceGranularityDepth, picWidth, picHeight);
    }

    //step 3: border availability
    for (UInt i = startLCU; i <= endLCU; i++)
    {
        getCU(i)->setNDBFilterBlockBorderAvailability(numLCUsInPicWidth, numLCUsInPicHeight, maxNumSUInLCUWidth, maxNumSUInLCUHeight, picWidth, picHeight, false, false, false, false, false);
    }
}

/** Create non-deblocked filter information for LCU
 * \param tileID tile index
 * \param sliceID slice index
 * \param cu CU data pointer
 * \param startSU start SU index in LCU
 * \param endSU end SU index in LCU
 * \param sliceGranularyDepth slice granularity
 * \param picWidth picture width
 * \param picHeight picture height
 */
Void TComPic::createNonDBFilterInfoLCU(Int sliceID, TComDataCU* cu, UInt startSU, UInt endSU, Int sliceGranularyDepth, UInt picWidth, UInt picHeight)
{
    UInt LCUX          = cu->getCUPelX();
    UInt LCUY          = cu->getCUPelY();
    UInt maxNumSUInLCU = getNumPartInCU();
    UInt maxNumSUInSGU = maxNumSUInLCU >> (sliceGranularyDepth << 1);
    UInt maxNumSUInLCUWidth = getNumPartInWidth();
    UInt LPelX, TPelY;
    UInt curSU;

    //get the number of valid NBFilterBLock
    curSU   = startSU;
    while (curSU <= endSU)
    {
        LPelX = LCUX + g_rasterToPelX[g_zscanToRaster[curSU]];
        TPelY = LCUY + g_rasterToPelY[g_zscanToRaster[curSU]];

        while (!(LPelX < picWidth) || !(TPelY < picHeight))
        {
            curSU += maxNumSUInSGU;
            if (curSU >= maxNumSUInLCU || curSU > endSU)
            {
                break;
            }
            LPelX = LCUX + g_rasterToPelX[g_zscanToRaster[curSU]];
            TPelY = LCUY + g_rasterToPelY[g_zscanToRaster[curSU]];
        }

        if (curSU >= maxNumSUInLCU || curSU > endSU)
        {
            break;
        }

        NDBFBlockInfo NDBFBlock;

        NDBFBlock.sliceID = sliceID;
        NDBFBlock.posY    = TPelY;
        NDBFBlock.posX    = LPelX;
        NDBFBlock.startSU = curSU;

        UInt lastValidSU  = curSU;
        UInt idx, lpelXSU, tpelYSU;
        for (idx = curSU; idx < curSU + maxNumSUInSGU; idx++)
        {
            if (idx > endSU)
            {
                break;
            }
            lpelXSU   = LCUX + g_rasterToPelX[g_zscanToRaster[idx]];
            tpelYSU   = LCUY + g_rasterToPelY[g_zscanToRaster[idx]];
            if (!(lpelXSU < picWidth) || !(tpelYSU < picHeight))
            {
                continue;
            }
            lastValidSU = idx;
        }

        NDBFBlock.endSU = lastValidSU;

        UInt rTLSU = g_zscanToRaster[NDBFBlock.startSU];
        UInt rBRSU = g_zscanToRaster[NDBFBlock.endSU];
        NDBFBlock.widthSU  = (rBRSU % maxNumSUInLCUWidth) - (rTLSU % maxNumSUInLCUWidth) + 1;
        NDBFBlock.heightSU = (UInt)(rBRSU / maxNumSUInLCUWidth) - (UInt)(rTLSU / maxNumSUInLCUWidth) + 1;
        NDBFBlock.width    = NDBFBlock.widthSU  * getMinCUWidth();
        NDBFBlock.height   = NDBFBlock.heightSU * getMinCUHeight();

        cu->getNDBFilterBlocks()->push_back(NDBFBlock);

        curSU += maxNumSUInSGU;
    }
}

/** destroy non-deblocked filter information for LCU
 */
Void TComPic::destroyNonDBFilterInfo()
{
    for (UInt cuAddr = 0; cuAddr < getNumCUsInFrame(); cuAddr++)
    {
        TComDataCU* cu = getCU(cuAddr);
        cu->getNDBFilterBlocks()->clear();
    }
}

//! \}
