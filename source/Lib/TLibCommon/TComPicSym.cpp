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

/** \file     TComPicSym.cpp
    \brief    picture symbol class
*/

#include "TComPicSym.h"
#include "TComSampleAdaptiveOffset.h"

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TComPicSym::TComPicSym()
    : m_uiWidthInCU(0)
    , m_uiHeightInCU(0)
    , m_uiMaxCUWidth(0)
    , m_uiMaxCUHeight(0)
    , m_uiMinCUWidth(0)
    , m_uiMinCUHeight(0)
    , m_uhTotalDepth(0)
    , m_uiNumPartitions(0)
    , m_uiNumPartInWidth(0)
    , m_uiNumPartInHeight(0)
    , m_uiNumCUsInFrame(0)
    , m_apcTComSlice(NULL)
    , m_uiNumAllocatedSlice(0)
    , m_apcTComDataCU(NULL)
    , m_iTileBoundaryIndependenceIdr(0)
    , m_apcTComTile(NULL)
    , m_puiCUOrderMap(0)
    , m_puiInverseCUOrderMap(NULL)
{}

Void TComPicSym::create(Int iPicWidth, Int iPicHeight, UInt uiMaxWidth, UInt uiMaxHeight, UInt uiMaxDepth)
{
    UInt i;

    m_uhTotalDepth      = uiMaxDepth;
    m_uiNumPartitions   = 1 << (m_uhTotalDepth << 1);

    m_uiMaxCUWidth      = uiMaxWidth;
    m_uiMaxCUHeight     = uiMaxHeight;

    m_uiMinCUWidth      = uiMaxWidth  >> m_uhTotalDepth;
    m_uiMinCUHeight     = uiMaxHeight >> m_uhTotalDepth;

    m_uiNumPartInWidth  = m_uiMaxCUWidth  / m_uiMinCUWidth;
    m_uiNumPartInHeight = m_uiMaxCUHeight / m_uiMinCUHeight;

    m_uiWidthInCU       = (iPicWidth % m_uiMaxCUWidth) ? iPicWidth / m_uiMaxCUWidth  + 1 : iPicWidth / m_uiMaxCUWidth;
    m_uiHeightInCU      = (iPicHeight % m_uiMaxCUHeight) ? iPicHeight / m_uiMaxCUHeight + 1 : iPicHeight / m_uiMaxCUHeight;

    m_uiNumCUsInFrame   = m_uiWidthInCU * m_uiHeightInCU;
    m_apcTComDataCU     = new TComDataCU*[m_uiNumCUsInFrame];

    if (m_uiNumAllocatedSlice > 0)
    {
        for (i = 0; i < m_uiNumAllocatedSlice; i++)
        {
            delete m_apcTComSlice[i];
        }

        delete [] m_apcTComSlice;
    }
    m_apcTComSlice      = new TComSlice*[m_uiNumCUsInFrame * m_uiNumPartitions];
    m_apcTComSlice[0]   = new TComSlice;
    m_uiNumAllocatedSlice = 1;
    for (i = 0; i < m_uiNumCUsInFrame; i++)
    {
        m_apcTComDataCU[i] = new TComDataCU;
        m_apcTComDataCU[i]->create(m_uiNumPartitions, m_uiMaxCUWidth, m_uiMaxCUHeight, false, m_uiMaxCUWidth >> m_uhTotalDepth, true);
    }

    m_puiCUOrderMap = new UInt[m_uiNumCUsInFrame + 1];
    m_puiInverseCUOrderMap = new UInt[m_uiNumCUsInFrame + 1];

    for (i = 0; i < m_uiNumCUsInFrame; i++)
    {
        m_puiCUOrderMap[i] = i;
        m_puiInverseCUOrderMap[i] = i;
    }

    m_saoParam = NULL;
}

Void TComPicSym::destroy()
{
    if (m_uiNumAllocatedSlice > 0)
    {
        for (Int i = 0; i < m_uiNumAllocatedSlice; i++)
        {
            delete m_apcTComSlice[i];
        }

        delete [] m_apcTComSlice;
    }
    m_apcTComSlice = NULL;

    for (Int i = 0; i < m_uiNumCUsInFrame; i++)
    {
        m_apcTComDataCU[i]->destroy();
        delete m_apcTComDataCU[i];
        m_apcTComDataCU[i] = NULL;
    }

    delete [] m_apcTComDataCU;
    m_apcTComDataCU = NULL;

    delete m_apcTComTile;

    m_apcTComTile = NULL;

    delete [] m_puiCUOrderMap;
    m_puiCUOrderMap = NULL;

    delete [] m_puiInverseCUOrderMap;
    m_puiInverseCUOrderMap = NULL;

    if (m_saoParam)
    {
        TComSampleAdaptiveOffset::freeSaoParam(m_saoParam);
        delete m_saoParam;
        m_saoParam = NULL;
    }
}

Void TComPicSym::allocateNewSlice()
{
    m_apcTComSlice[m_uiNumAllocatedSlice++] = new TComSlice;
    if (m_uiNumAllocatedSlice >= 2)
    {
        m_apcTComSlice[m_uiNumAllocatedSlice - 1]->copySliceInfo(m_apcTComSlice[m_uiNumAllocatedSlice - 2]);
        m_apcTComSlice[m_uiNumAllocatedSlice - 1]->initSlice();
    }
}

Void TComPicSym::clearSliceBuffer()
{
    UInt i;

    for (i = 1; i < m_uiNumAllocatedSlice; i++)
    {
        delete m_apcTComSlice[i];
    }

    m_uiNumAllocatedSlice = 1;
}

UInt TComPicSym::getPicSCUEncOrder(UInt SCUAddr)
{
    return getInverseCUOrderMap(SCUAddr / m_uiNumPartitions) * m_uiNumPartitions + SCUAddr % m_uiNumPartitions;
}

UInt TComPicSym::getPicSCUAddr(UInt SCUEncOrder)
{
    return getCUOrderMap(SCUEncOrder / m_uiNumPartitions) * m_uiNumPartitions + SCUEncOrder % m_uiNumPartitions;
}

Void TComPicSym::xCreateTComTileArray()
{
    m_apcTComTile = new TComTile;
}

Void TComPicSym::xInitTiles()
{
    UInt  uiRightEdgePosInCU;
    UInt  uiBottomEdgePosInCU;

    //initialize each tile of the current picture
            //initialize the RightEdgePosInCU for each tile
            uiRightEdgePosInCU = 0;
                uiRightEdgePosInCU += this->getTComTile()->getTileWidth();

            this->getTComTile()->setRightEdgePosInCU(uiRightEdgePosInCU - 1);

            //initialize the BottomEdgePosInCU for each tile
            uiBottomEdgePosInCU = 0;
                uiBottomEdgePosInCU += this->getTComTile()->getTileHeight();

            this->getTComTile()->setBottomEdgePosInCU(uiBottomEdgePosInCU - 1);

            //initialize the FirstCUAddr for each tile
            this->getTComTile()->setFirstCUAddr((this->getTComTile()->getBottomEdgePosInCU() - this->getTComTile()->getTileHeight() + 1) * m_uiWidthInCU +
                                                 this->getTComTile()->getRightEdgePosInCU() - this->getTComTile()->getTileWidth() + 1);
}

UInt TComPicSym::xCalculateNxtCUAddr(UInt uiCurrCUAddr)
{
    UInt  uiNxtCUAddr;

    //get the raster scan address for the next LCU
    if (uiCurrCUAddr % m_uiWidthInCU == this->getTComTile()->getRightEdgePosInCU() && uiCurrCUAddr / m_uiWidthInCU == this->getTComTile()->getBottomEdgePosInCU())
    //the current LCU is the last LCU of the tile
    {
            uiNxtCUAddr = m_uiNumCUsInFrame;
    }
    else //the current LCU is not the last LCU of the tile
    {
        if (uiCurrCUAddr % m_uiWidthInCU == this->getTComTile()->getRightEdgePosInCU()) //the current LCU is on the rightmost edge of the tile
        {
            uiNxtCUAddr = uiCurrCUAddr + m_uiWidthInCU - this->getTComTile()->getTileWidth() + 1;
        }
        else
        {
            uiNxtCUAddr = uiCurrCUAddr + 1;
        }
    }

    return uiNxtCUAddr;
}

Void TComPicSym::allocSaoParam(TComSampleAdaptiveOffset *sao)
{
    m_saoParam = new SAOParam;
    sao->allocSaoParam(m_saoParam);
}

TComTile::TComTile()
{}

TComTile::~TComTile()
{}

//! \}
