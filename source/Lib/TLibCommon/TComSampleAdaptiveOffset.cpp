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

/** \file     TComSampleAdaptiveOffset.cpp
    \brief    sample adaptive offset class
*/

#include "TComSampleAdaptiveOffset.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

//! \ingroup TLibCommon
//! \{

SAOParam::~SAOParam()
{
    for (Int i = 0; i < 3; i++)
    {
        if (psSaoPart[i])
        {
            delete [] psSaoPart[i];
        }
    }
}

// ====================================================================================================================
// Tables
// ====================================================================================================================

TComSampleAdaptiveOffset::TComSampleAdaptiveOffset()
{
    m_pClipTable = NULL;
    m_pClipTableBase = NULL;
    m_pChromaClipTable = NULL;
    m_pChromaClipTableBase = NULL;
    m_iOffsetBo = NULL;
    m_iChromaOffsetBo = NULL;
    m_lumaTableBo = NULL;
    m_chromaTableBo = NULL;
    m_iUpBuff1 = NULL;
    m_iUpBuff2 = NULL;
    m_iUpBufft = NULL;
    ipSwap = NULL;

    m_pTmpU1 = NULL;
    m_pTmpU2 = NULL;
    m_pTmpL1 = NULL;
    m_pTmpL2 = NULL;
}

TComSampleAdaptiveOffset::~TComSampleAdaptiveOffset()
{}

const Int TComSampleAdaptiveOffset::m_aiNumCulPartsLevel[5] =
{
    1, //level 0
    5, //level 1
    21, //level 2
    85, //level 3
    341, //level 4
};

const UInt TComSampleAdaptiveOffset::m_auiEoTable[9] =
{
    1, //0
    2, //1
    0, //2
    3, //3
    4, //4
    0, //5
    0, //6
    0, //7
    0
};

const Int TComSampleAdaptiveOffset::m_iNumClass[MAX_NUM_SAO_TYPE] =
{
    SAO_EO_LEN,
    SAO_EO_LEN,
    SAO_EO_LEN,
    SAO_EO_LEN,
    SAO_BO_LEN
};

const UInt TComSampleAdaptiveOffset::m_uiMaxDepth = SAO_MAX_DEPTH;

/** convert Level Row Col to Idx
 * \param   level,  row,  col
 */
Int  TComSampleAdaptiveOffset::convertLevelRowCol2Idx(Int level, Int row, Int col)
{
    Int idx;

    if (level == 0)
    {
        idx = 0;
    }
    else if (level == 1)
    {
        idx = 1 + row * 2 + col;
    }
    else if (level == 2)
    {
        idx = 5 + row * 4 + col;
    }
    else if (level == 3)
    {
        idx = 21 + row * 8 + col;
    }
    else // (level == 4)
    {
        idx = 85 + row * 16 + col;
    }
    return idx;
}

/** create SampleAdaptiveOffset memory.
 * \param
 */
Void TComSampleAdaptiveOffset::create(UInt sourceWidth, UInt sourceHeight, UInt maxCUWidth, UInt maxCUHeight)
{
    m_iPicWidth  = sourceWidth;
    m_iPicHeight = sourceHeight;

    m_uiMaxCUWidth  = maxCUWidth;
    m_uiMaxCUHeight = maxCUHeight;

    m_iNumCuInWidth  = m_iPicWidth / m_uiMaxCUWidth;
    m_iNumCuInWidth += (m_iPicWidth % m_uiMaxCUWidth) ? 1 : 0;

    m_iNumCuInHeight  = m_iPicHeight / m_uiMaxCUHeight;
    m_iNumCuInHeight += (m_iPicHeight % m_uiMaxCUHeight) ? 1 : 0;

    Int maxSplitLevelHeight = (Int)(logf((Float)m_iNumCuInHeight) / logf(2.0));
    Int maxSplitLevelWidth  = (Int)(logf((Float)m_iNumCuInWidth) / logf(2.0));

    m_uiMaxSplitLevel = (maxSplitLevelHeight < maxSplitLevelWidth) ? (maxSplitLevelHeight) : (maxSplitLevelWidth);
    m_uiMaxSplitLevel = (m_uiMaxSplitLevel < m_uiMaxDepth) ? (m_uiMaxSplitLevel) : (m_uiMaxDepth);

    /* various structures are overloaded to store per component data.
     * m_iNumTotalParts must allow for sufficient storage in any allocated arrays */
    m_iNumTotalParts  = max(3, m_aiNumCulPartsLevel[m_uiMaxSplitLevel]);

    UInt pixelRangeY = 1 << g_bitDepthY;
    UInt boRangeShiftY = g_bitDepthY - SAO_BO_BITS;

    m_lumaTableBo = new Pel[pixelRangeY];
    for (Int k2 = 0; k2 < pixelRangeY; k2++)
    {
        m_lumaTableBo[k2] = 1 + (k2 >> boRangeShiftY);
    }

    UInt pixelRangeC = 1 << g_bitDepthC;
    UInt boRangeShiftC = g_bitDepthC - SAO_BO_BITS;

    m_chromaTableBo = new Pel[pixelRangeC];
    for (Int k2 = 0; k2 < pixelRangeC; k2++)
    {
        m_chromaTableBo[k2] = 1 + (k2 >> boRangeShiftC);
    }

    m_iUpBuff1 = new Int[m_iPicWidth + 2];
    m_iUpBuff2 = new Int[m_iPicWidth + 2];
    m_iUpBufft = new Int[m_iPicWidth + 2];

    m_iUpBuff1++;
    m_iUpBuff2++;
    m_iUpBufft++;
    Short i;

    UInt maxY  = (1 << g_bitDepthY) - 1;
    UInt minY  = 0;

    Int rangeExt = maxY >> 1;

    m_pClipTableBase = new Pel[maxY + 2 * rangeExt];
    m_iOffsetBo      = new Int[maxY + 2 * rangeExt];

    for (i = 0; i < (minY + rangeExt); i++)
    {
        m_pClipTableBase[i] = minY;
    }

    for (i = minY + rangeExt; i < (maxY +  rangeExt); i++)
    {
        m_pClipTableBase[i] = i - rangeExt;
    }

    for (i = maxY + rangeExt; i < (maxY + 2 * rangeExt); i++)
    {
        m_pClipTableBase[i] = maxY;
    }

    m_pClipTable = &(m_pClipTableBase[rangeExt]);

    UInt maxC  = (1 << g_bitDepthC) - 1;
    UInt minC  = 0;

    Int rangeExtC = maxC >> 1;

    m_pChromaClipTableBase = new Pel[maxC + 2 * rangeExtC];
    m_iChromaOffsetBo      = new Int[maxC + 2 * rangeExtC];

    for (i = 0; i < (minC + rangeExtC); i++)
    {
        m_pChromaClipTableBase[i] = minC;
    }

    for (i = minC + rangeExtC; i < (maxC +  rangeExtC); i++)
    {
        m_pChromaClipTableBase[i] = i - rangeExtC;
    }

    for (i = maxC + rangeExtC; i < (maxC + 2 * rangeExtC); i++)
    {
        m_pChromaClipTableBase[i] = maxC;
    }

    m_pChromaClipTable = &(m_pChromaClipTableBase[rangeExtC]);

    m_pTmpL1 = new Pel[m_uiMaxCUHeight + 1];
    m_pTmpL2 = new Pel[m_uiMaxCUHeight + 1];
    m_pTmpU1 = new Pel[m_iPicWidth];
    m_pTmpU2 = new Pel[m_iPicWidth];
}

/** destroy SampleAdaptiveOffset memory.
 * \param
 */
Void TComSampleAdaptiveOffset::destroy()
{
    if (m_pClipTableBase)
    {
        delete [] m_pClipTableBase;
        m_pClipTableBase = NULL;
    }
    if (m_iOffsetBo)
    {
        delete [] m_iOffsetBo;
        m_iOffsetBo = NULL;
    }
    if (m_lumaTableBo)
    {
        delete[] m_lumaTableBo;
        m_lumaTableBo = NULL;
    }

    if (m_pChromaClipTableBase)
    {
        delete [] m_pChromaClipTableBase;
        m_pChromaClipTableBase = NULL;
    }
    if (m_iChromaOffsetBo)
    {
        delete [] m_iChromaOffsetBo;
        m_iChromaOffsetBo = NULL;
    }
    if (m_chromaTableBo)
    {
        delete[] m_chromaTableBo;
        m_chromaTableBo = NULL;
    }

    if (m_iUpBuff1)
    {
        m_iUpBuff1--;
        delete [] m_iUpBuff1;
        m_iUpBuff1 = NULL;
    }
    if (m_iUpBuff2)
    {
        m_iUpBuff2--;
        delete [] m_iUpBuff2;
        m_iUpBuff2 = NULL;
    }
    if (m_iUpBufft)
    {
        m_iUpBufft--;
        delete [] m_iUpBufft;
        m_iUpBufft = NULL;
    }
    if (m_pTmpL1)
    {
        delete [] m_pTmpL1;
        m_pTmpL1 = NULL;
    }
    if (m_pTmpL2)
    {
        delete [] m_pTmpL2;
        m_pTmpL2 = NULL;
    }
    if (m_pTmpU1)
    {
        delete [] m_pTmpU1;
        m_pTmpU1 = NULL;
    }
    if (m_pTmpU2)
    {
        delete [] m_pTmpU2;
        m_pTmpU2 = NULL;
    }
}

/** allocate memory for SAO parameters
 * \param    *pcSaoParam
 */
Void TComSampleAdaptiveOffset::allocSaoParam(SAOParam *pcSaoParam)
{
    pcSaoParam->iMaxSplitLevel = m_uiMaxSplitLevel;
    pcSaoParam->psSaoPart[0] = new SAOQTPart[m_aiNumCulPartsLevel[pcSaoParam->iMaxSplitLevel]];
    initSAOParam(pcSaoParam, 0, 0, 0, -1, 0, m_iNumCuInWidth - 1,  0, m_iNumCuInHeight - 1, 0);
    pcSaoParam->psSaoPart[1] = new SAOQTPart[m_aiNumCulPartsLevel[pcSaoParam->iMaxSplitLevel]];
    pcSaoParam->psSaoPart[2] = new SAOQTPart[m_aiNumCulPartsLevel[pcSaoParam->iMaxSplitLevel]];
    initSAOParam(pcSaoParam, 0, 0, 0, -1, 0, m_iNumCuInWidth - 1,  0, m_iNumCuInHeight - 1, 1);
    initSAOParam(pcSaoParam, 0, 0, 0, -1, 0, m_iNumCuInWidth - 1,  0, m_iNumCuInHeight - 1, 2);
    pcSaoParam->numCuInWidth  = m_iNumCuInWidth;
    pcSaoParam->numCuInHeight = m_iNumCuInHeight;
    pcSaoParam->saoLcuParam[0] = new SaoLcuParam[m_iNumCuInHeight * m_iNumCuInWidth];
    pcSaoParam->saoLcuParam[1] = new SaoLcuParam[m_iNumCuInHeight * m_iNumCuInWidth];
    pcSaoParam->saoLcuParam[2] = new SaoLcuParam[m_iNumCuInHeight * m_iNumCuInWidth];
}

/** initialize SAO parameters
 * \param    *pcSaoParam,  iPartLevel,  iPartRow,  iPartCol,  iParentPartIdx,  StartCUX,  EndCUX,  StartCUY,  EndCUY,  iYCbCr
 */
Void TComSampleAdaptiveOffset::initSAOParam(SAOParam *pcSaoParam, Int partLevel, Int partRow, Int partCol, Int parentPartIdx, Int startCUX, Int endCUX, Int startCUY, Int endCUY, Int yCbCr)
{
    Int j;
    Int partIdx = convertLevelRowCol2Idx(partLevel, partRow, partCol);

    SAOQTPart* pSaoPart;

    pSaoPart = &(pcSaoParam->psSaoPart[yCbCr][partIdx]);

    pSaoPart->PartIdx   = partIdx;
    pSaoPart->PartLevel = partLevel;
    pSaoPart->PartRow   = partRow;
    pSaoPart->PartCol   = partCol;

    pSaoPart->StartCUX  = startCUX;
    pSaoPart->EndCUX    = endCUX;
    pSaoPart->StartCUY  = startCUY;
    pSaoPart->EndCUY    = endCUY;

    pSaoPart->UpPartIdx = parentPartIdx;
    pSaoPart->iBestType   = -1;
    pSaoPart->iLength     =  0;

    pSaoPart->subTypeIdx = 0;

    for (j = 0; j < MAX_NUM_SAO_OFFSETS; j++)
    {
        pSaoPart->iOffset[j] = 0;
    }

    if (pSaoPart->PartLevel != m_uiMaxSplitLevel)
    {
        Int downLevel    = (partLevel + 1);
        Int downRowStart = (partRow << 1);
        Int downColStart = (partCol << 1);

        Int downRowIdx, downColIdx;
        Int numCUWidth,  numCUHeight;
        Int numCULeft;
        Int numCUTop;

        Int downStartCUX, downStartCUY;
        Int downEndCUX, downEndCUY;

        numCUWidth  = endCUX - startCUX + 1;
        numCUHeight = endCUY - startCUY + 1;
        numCULeft   = (numCUWidth  >> 1);
        numCUTop    = (numCUHeight >> 1);

        downStartCUX = startCUX;
        downEndCUX  = downStartCUX + numCULeft - 1;
        downStartCUY = startCUY;
        downEndCUY  = downStartCUY + numCUTop  - 1;
        downRowIdx = downRowStart + 0;
        downColIdx = downColStart + 0;

        pSaoPart->DownPartsIdx[0] = convertLevelRowCol2Idx(downLevel, downRowIdx, downColIdx);

        initSAOParam(pcSaoParam, downLevel, downRowIdx, downColIdx, partIdx, downStartCUX, downEndCUX, downStartCUY, downEndCUY, yCbCr);

        downStartCUX = startCUX + numCULeft;
        downEndCUX   = endCUX;
        downStartCUY = startCUY;
        downEndCUY   = downStartCUY + numCUTop - 1;
        downRowIdx  = downRowStart + 0;
        downColIdx  = downColStart + 1;

        pSaoPart->DownPartsIdx[1] = convertLevelRowCol2Idx(downLevel, downRowIdx, downColIdx);

        initSAOParam(pcSaoParam, downLevel, downRowIdx, downColIdx, partIdx,  downStartCUX, downEndCUX, downStartCUY, downEndCUY, yCbCr);

        downStartCUX = startCUX;
        downEndCUX   = downStartCUX + numCULeft - 1;
        downStartCUY = startCUY + numCUTop;
        downEndCUY   = endCUY;
        downRowIdx  = downRowStart + 1;
        downColIdx  = downColStart + 0;

        pSaoPart->DownPartsIdx[2] = convertLevelRowCol2Idx(downLevel, downRowIdx, downColIdx);

        initSAOParam(pcSaoParam, downLevel, downRowIdx, downColIdx, partIdx, downStartCUX, downEndCUX, downStartCUY, downEndCUY, yCbCr);

        downStartCUX = startCUX + numCULeft;
        downEndCUX   = endCUX;
        downStartCUY = startCUY + numCUTop;
        downEndCUY   = endCUY;
        downRowIdx  = downRowStart + 1;
        downColIdx  = downColStart + 1;

        pSaoPart->DownPartsIdx[3] = convertLevelRowCol2Idx(downLevel, downRowIdx, downColIdx);

        initSAOParam(pcSaoParam, downLevel, downRowIdx, downColIdx, partIdx, downStartCUX, downEndCUX, downStartCUY, downEndCUY, yCbCr);
    }
    else
    {
        pSaoPart->DownPartsIdx[0] = pSaoPart->DownPartsIdx[1] = pSaoPart->DownPartsIdx[2] = pSaoPart->DownPartsIdx[3] = -1;
    }
}

/** free memory of SAO parameters
 * \param   pcSaoParam
 */
Void TComSampleAdaptiveOffset::freeSaoParam(SAOParam *pcSaoParam)
{
    delete [] pcSaoParam->psSaoPart[0];
    delete [] pcSaoParam->psSaoPart[1];
    delete [] pcSaoParam->psSaoPart[2];
    pcSaoParam->psSaoPart[0] = 0;
    pcSaoParam->psSaoPart[1] = 0;
    pcSaoParam->psSaoPart[2] = 0;
    if (pcSaoParam->saoLcuParam[0])
    {
        delete [] pcSaoParam->saoLcuParam[0];
        pcSaoParam->saoLcuParam[0] = NULL;
    }
    if (pcSaoParam->saoLcuParam[1])
    {
        delete [] pcSaoParam->saoLcuParam[1];
        pcSaoParam->saoLcuParam[1] = NULL;
    }
    if (pcSaoParam->saoLcuParam[2])
    {
        delete [] pcSaoParam->saoLcuParam[2];
        pcSaoParam->saoLcuParam[2] = NULL;
    }
}

/** reset SAO parameters
 * \param   pcSaoParam
 */
Void TComSampleAdaptiveOffset::resetSAOParam(SAOParam *pcSaoParam)
{
    Int numComponet = 3;

    for (Int c = 0; c < numComponet; c++)
    {
        if (c < 2)
        {
            pcSaoParam->bSaoFlag[c] = 0;
        }
        for (Int i = 0; i < m_aiNumCulPartsLevel[m_uiMaxSplitLevel]; i++)
        {
            pcSaoParam->psSaoPart[c][i].iBestType     = -1;
            pcSaoParam->psSaoPart[c][i].iLength       =  0;
            pcSaoParam->psSaoPart[c][i].bSplit        = false;
            pcSaoParam->psSaoPart[c][i].bProcessed    = false;
            pcSaoParam->psSaoPart[c][i].dMinCost      = MAX_DOUBLE;
            pcSaoParam->psSaoPart[c][i].iMinDist      = MAX_INT;
            pcSaoParam->psSaoPart[c][i].iMinRate      = MAX_INT;
            pcSaoParam->psSaoPart[c][i].subTypeIdx    = 0;
            for (Int j = 0; j < MAX_NUM_SAO_OFFSETS; j++)
            {
                pcSaoParam->psSaoPart[c][i].iOffset[j] = 0;
                pcSaoParam->psSaoPart[c][i].iOffset[j] = 0;
                pcSaoParam->psSaoPart[c][i].iOffset[j] = 0;
            }
        }

        pcSaoParam->oneUnitFlag[0]   = 0;
        pcSaoParam->oneUnitFlag[1]   = 0;
        pcSaoParam->oneUnitFlag[2]   = 0;
        resetLcuPart(pcSaoParam->saoLcuParam[0]);
        resetLcuPart(pcSaoParam->saoLcuParam[1]);
        resetLcuPart(pcSaoParam->saoLcuParam[2]);
    }
}

/** get the sign of input variable
 * \param   x
 */
inline Int xSign(Int x)
{
    return (x >> 31) | ((Int)((((UInt) - x)) >> 31));
}

/** initialize variables for SAO process
 * \param  pcPic picture data pointer
 */
Void TComSampleAdaptiveOffset::createPicSaoInfo(TComPic* pcPic)
{
    m_pcPic = pcPic;
}

Void TComSampleAdaptiveOffset::destroyPicSaoInfo()
{
}

/** sample adaptive offset process for one LCU
 * \param   iAddr, iSaoType, iYCbCr
 */
Void TComSampleAdaptiveOffset::processSaoCu(Int addr, Int saoType, Int yCbCr)
{
    processSaoCuOrg(addr, saoType, yCbCr);
}

/** Perform SAO for non-cross-slice or non-cross-tile process
 * \param  pDec to-be-filtered block buffer pointer
 * \param  pRest filtered block buffer pointer
 * \param  stride picture buffer stride
 * \param  saoType SAO offset type
 * \param  xPos x coordinate
 * \param  yPos y coordinate
 * \param  width block width
 * \param  height block height
 * \param  pbBorderAvail availabilities of block border pixels
 */
Void TComSampleAdaptiveOffset::processSaoBlock(Pel* pDec, Pel* pRest, Int stride, Int saoType, UInt width, UInt height, Bool* pbBorderAvail, Int iYCbCr)
{
    //variables
    Int startX, startY, endX, endY, x, y;
    Int signLeft, signRight, signDown, signDown1;
    UInt edgeType;
    Pel *pClipTbl = (iYCbCr == 0) ? m_pClipTable : m_pChromaClipTable;
    Int *pOffsetBo = (iYCbCr == 0) ? m_iOffsetBo : m_iChromaOffsetBo;

    switch (saoType)
    {
    case SAO_EO_0: // dir: -
    {
        startX = (pbBorderAvail[SGU_L]) ? 0 : 1;
        endX   = (pbBorderAvail[SGU_R]) ? width : (width - 1);
        for (y = 0; y < height; y++)
        {
            signLeft = xSign(pDec[startX] - pDec[startX - 1]);
            for (x = startX; x < endX; x++)
            {
                signRight =  xSign(pDec[x] - pDec[x + 1]);
                edgeType =  signRight + signLeft + 2;
                signLeft  = -signRight;

                pRest[x] = pClipTbl[pDec[x] + m_iOffsetEo[edgeType]];
            }

            pDec  += stride;
            pRest += stride;
        }

        break;
    }
    case SAO_EO_1: // dir: |
    {
        startY = (pbBorderAvail[SGU_T]) ? 0 : 1;
        endY   = (pbBorderAvail[SGU_B]) ? height : height - 1;
        if (!pbBorderAvail[SGU_T])
        {
            pDec  += stride;
            pRest += stride;
        }
        for (x = 0; x < width; x++)
        {
            m_iUpBuff1[x] = xSign(pDec[x] - pDec[x - stride]);
        }

        for (y = startY; y < endY; y++)
        {
            for (x = 0; x < width; x++)
            {
                signDown  = xSign(pDec[x] - pDec[x + stride]);
                edgeType = signDown + m_iUpBuff1[x] + 2;
                m_iUpBuff1[x] = -signDown;

                pRest[x] = pClipTbl[pDec[x] + m_iOffsetEo[edgeType]];
            }

            pDec  += stride;
            pRest += stride;
        }

        break;
    }
    case SAO_EO_2: // dir: 135
    {
        Int posShift = stride + 1;

        startX = (pbBorderAvail[SGU_L]) ? 0 : 1;
        endX   = (pbBorderAvail[SGU_R]) ? width : (width - 1);

        //prepare 2nd line upper sign
        pDec += stride;
        for (x = startX; x < endX + 1; x++)
        {
            m_iUpBuff1[x] = xSign(pDec[x] - pDec[x - posShift]);
        }

        //1st line
        pDec -= stride;
        if (pbBorderAvail[SGU_TL])
        {
            x = 0;
            edgeType      =  xSign(pDec[x] - pDec[x - posShift]) - m_iUpBuff1[x + 1] + 2;
            pRest[x] = pClipTbl[pDec[x] + m_iOffsetEo[edgeType]];
        }
        if (pbBorderAvail[SGU_T])
        {
            for (x = 1; x < endX; x++)
            {
                edgeType      =  xSign(pDec[x] - pDec[x - posShift]) - m_iUpBuff1[x + 1] + 2;
                pRest[x] = pClipTbl[pDec[x] + m_iOffsetEo[edgeType]];
            }
        }
        pDec   += stride;
        pRest  += stride;

        //middle lines
        for (y = 1; y < height - 1; y++)
        {
            for (x = startX; x < endX; x++)
            {
                signDown1      =  xSign(pDec[x] - pDec[x + posShift]);
                edgeType      =  signDown1 + m_iUpBuff1[x] + 2;
                pRest[x] = pClipTbl[pDec[x] + m_iOffsetEo[edgeType]];

                m_iUpBufft[x + 1] = -signDown1;
            }

            m_iUpBufft[startX] = xSign(pDec[stride + startX] - pDec[startX - 1]);

            ipSwap     = m_iUpBuff1;
            m_iUpBuff1 = m_iUpBufft;
            m_iUpBufft = ipSwap;

            pDec  += stride;
            pRest += stride;
        }

        //last line
        if (pbBorderAvail[SGU_B])
        {
            for (x = startX; x < width - 1; x++)
            {
                edgeType =  xSign(pDec[x] - pDec[x + posShift]) + m_iUpBuff1[x] + 2;
                pRest[x] = pClipTbl[pDec[x] + m_iOffsetEo[edgeType]];
            }
        }
        if (pbBorderAvail[SGU_BR])
        {
            x = width - 1;
            edgeType =  xSign(pDec[x] - pDec[x + posShift]) + m_iUpBuff1[x] + 2;
            pRest[x] = pClipTbl[pDec[x] + m_iOffsetEo[edgeType]];
        }
        break;
    }
    case SAO_EO_3: // dir: 45
    {
        Int  posShift     = stride - 1;
        startX = (pbBorderAvail[SGU_L]) ? 0 : 1;
        endX   = (pbBorderAvail[SGU_R]) ? width : (width - 1);

        //prepare 2nd line upper sign
        pDec += stride;
        for (x = startX - 1; x < endX; x++)
        {
            m_iUpBuff1[x] = xSign(pDec[x] - pDec[x - posShift]);
        }

        //first line
        pDec -= stride;
        if (pbBorderAvail[SGU_T])
        {
            for (x = startX; x < width - 1; x++)
            {
                edgeType = xSign(pDec[x] - pDec[x - posShift]) - m_iUpBuff1[x - 1] + 2;
                pRest[x] = pClipTbl[pDec[x] + m_iOffsetEo[edgeType]];
            }
        }
        if (pbBorderAvail[SGU_TR])
        {
            x = width - 1;
            edgeType = xSign(pDec[x] - pDec[x - posShift]) - m_iUpBuff1[x - 1] + 2;
            pRest[x] = pClipTbl[pDec[x] + m_iOffsetEo[edgeType]];
        }
        pDec  += stride;
        pRest += stride;

        //middle lines
        for (y = 1; y < height - 1; y++)
        {
            for (x = startX; x < endX; x++)
            {
                signDown1      =  xSign(pDec[x] - pDec[x + posShift]);
                edgeType      =  signDown1 + m_iUpBuff1[x] + 2;

                pRest[x] = pClipTbl[pDec[x] + m_iOffsetEo[edgeType]];
                m_iUpBuff1[x - 1] = -signDown1;
            }

            m_iUpBuff1[endX - 1] = xSign(pDec[endX - 1 + stride] - pDec[endX]);

            pDec  += stride;
            pRest += stride;
        }

        //last line
        if (pbBorderAvail[SGU_BL])
        {
            x = 0;
            edgeType = xSign(pDec[x] - pDec[x + posShift]) + m_iUpBuff1[x] + 2;
            pRest[x] = pClipTbl[pDec[x] + m_iOffsetEo[edgeType]];
        }
        if (pbBorderAvail[SGU_B])
        {
            for (x = 1; x < endX; x++)
            {
                edgeType = xSign(pDec[x] - pDec[x + posShift]) + m_iUpBuff1[x] + 2;
                pRest[x] = pClipTbl[pDec[x] + m_iOffsetEo[edgeType]];
            }
        }
        break;
    }
    case SAO_BO:
    {
        for (y = 0; y < height; y++)
        {
            for (x = 0; x < width; x++)
            {
                pRest[x] = pOffsetBo[pDec[x]];
            }

            pRest += stride;
            pDec  += stride;
        }

        break;
    }
    default: break;
    }
}

/** sample adaptive offset process for one LCU crossing LCU boundary
 * \param   iAddr, iSaoType, iYCbCr
 */
Void TComSampleAdaptiveOffset::processSaoCuOrg(Int addr, Int saoType, Int yCbCr)
{
    Int x, y;
    TComDataCU *tmpCu = m_pcPic->getCU(addr);
    Pel* rec;
    Int  stride;
    Int  lcuWidth  = m_uiMaxCUWidth;
    Int  lcuHeight = m_uiMaxCUHeight;
    UInt lpelx    = tmpCu->getCUPelX();
    UInt tpely    = tmpCu->getCUPelY();
    UInt rpelx;
    UInt bpely;
    Int  signLeft;
    Int  signRight;
    Int  signDown;
    Int  signDown1;
    Int  signDown2;
    UInt edgeType;
    Int picWidthTmp;
    Int picHeightTmp;
    Int startX;
    Int startY;
    Int endX;
    Int endY;
    Int isChroma = (yCbCr != 0) ? 1 : 0;
    Int shift;
    Int cuHeightTmp;
    Pel *tmpLSwap;
    Pel *tmpL;
    Pel *tmpU;
    Pel *clipTbl = NULL;
    Int *offsetBo = NULL;

    picWidthTmp  = m_iPicWidth  >> isChroma;
    picHeightTmp = m_iPicHeight >> isChroma;
    lcuWidth     = lcuWidth    >> isChroma;
    lcuHeight    = lcuHeight   >> isChroma;
    lpelx       = lpelx      >> isChroma;
    tpely       = tpely      >> isChroma;
    rpelx       = lpelx + lcuWidth;
    bpely       = tpely + lcuHeight;
    rpelx       = rpelx > picWidthTmp  ? picWidthTmp  : rpelx;
    bpely       = bpely > picHeightTmp ? picHeightTmp : bpely;
    lcuWidth     = rpelx - lpelx;
    lcuHeight    = bpely - tpely;

    if (tmpCu->getPic() == 0)
    {
        return;
    }
    if (yCbCr == 0)
    {
        rec       = m_pcPic->getPicYuvRec()->getLumaAddr(addr);
        stride    = m_pcPic->getStride();
    }
    else if (yCbCr == 1)
    {
        rec       = m_pcPic->getPicYuvRec()->getCbAddr(addr);
        stride    = m_pcPic->getCStride();
    }
    else
    {
        rec       = m_pcPic->getPicYuvRec()->getCrAddr(addr);
        stride    = m_pcPic->getCStride();
    }

//   if (iSaoType!=SAO_BO_0 || iSaoType!=SAO_BO_1)
    {
        cuHeightTmp = (m_uiMaxCUHeight >> isChroma);
        shift = (m_uiMaxCUWidth >> isChroma) - 1;
        for (Int i = 0; i < cuHeightTmp + 1; i++)
        {
            m_pTmpL2[i] = rec[shift];
            rec += stride;
        }

        rec -= (stride * (cuHeightTmp + 1));

        tmpL = m_pTmpL1;
        tmpU = &(m_pTmpU1[lpelx]);
    }

    clipTbl = (yCbCr == 0) ? m_pClipTable : m_pChromaClipTable;
    offsetBo = (yCbCr == 0) ? m_iOffsetBo : m_iChromaOffsetBo;

    switch (saoType)
    {
    case SAO_EO_0: // dir: -
    {
        startX = (lpelx == 0) ? 1 : 0;
        endX   = (rpelx == picWidthTmp) ? lcuWidth - 1 : lcuWidth;
        for (y = 0; y < lcuHeight; y++)
        {
            signLeft = xSign(rec[startX] - tmpL[y]);
            for (x = startX; x < endX; x++)
            {
                signRight =  xSign(rec[x] - rec[x + 1]);
                edgeType =  signRight + signLeft + 2;
                signLeft  = -signRight;

                rec[x] = clipTbl[rec[x] + m_iOffsetEo[edgeType]];
            }

            rec += stride;
        }

        break;
    }
    case SAO_EO_1: // dir: |
    {
        startY = (tpely == 0) ? 1 : 0;
        endY   = (bpely == picHeightTmp) ? lcuHeight - 1 : lcuHeight;
        if (tpely == 0)
        {
            rec += stride;
        }
        for (x = 0; x < lcuWidth; x++)
        {
            m_iUpBuff1[x] = xSign(rec[x] - tmpU[x]);
        }

        for (y = startY; y < endY; y++)
        {
            for (x = 0; x < lcuWidth; x++)
            {
                signDown  = xSign(rec[x] - rec[x + stride]);
                edgeType = signDown + m_iUpBuff1[x] + 2;
                m_iUpBuff1[x] = -signDown;

                rec[x] = clipTbl[rec[x] + m_iOffsetEo[edgeType]];
            }

            rec += stride;
        }

        break;
    }
    case SAO_EO_2: // dir: 135
    {
        startX = (lpelx == 0)            ? 1 : 0;
        endX   = (rpelx == picWidthTmp) ? lcuWidth - 1 : lcuWidth;

        startY = (tpely == 0) ?             1 : 0;
        endY   = (bpely == picHeightTmp) ? lcuHeight - 1 : lcuHeight;

        if (tpely == 0)
        {
            rec += stride;
        }

        for (x = startX; x < endX; x++)
        {
            m_iUpBuff1[x] = xSign(rec[x] - tmpU[x - 1]);
        }

        for (y = startY; y < endY; y++)
        {
            signDown2 = xSign(rec[stride + startX] - tmpL[y]);
            for (x = startX; x < endX; x++)
            {
                signDown1      =  xSign(rec[x] - rec[x + stride + 1]);
                edgeType      =  signDown1 + m_iUpBuff1[x] + 2;
                m_iUpBufft[x + 1] = -signDown1;
                rec[x] = clipTbl[rec[x] + m_iOffsetEo[edgeType]];
            }

            m_iUpBufft[startX] = signDown2;

            ipSwap     = m_iUpBuff1;
            m_iUpBuff1 = m_iUpBufft;
            m_iUpBufft = ipSwap;

            rec += stride;
        }

        break;
    }
    case SAO_EO_3: // dir: 45
    {
        startX = (lpelx == 0) ? 1 : 0;
        endX   = (rpelx == picWidthTmp) ? lcuWidth - 1 : lcuWidth;

        startY = (tpely == 0) ? 1 : 0;
        endY   = (bpely == picHeightTmp) ? lcuHeight - 1 : lcuHeight;

        if (startY == 1)
        {
            rec += stride;
        }

        for (x = startX - 1; x < endX; x++)
        {
            m_iUpBuff1[x] = xSign(rec[x] - tmpU[x + 1]);
        }

        for (y = startY; y < endY; y++)
        {
            x = startX;
            signDown1      =  xSign(rec[x] - tmpL[y + 1]);
            edgeType      =  signDown1 + m_iUpBuff1[x] + 2;
            m_iUpBuff1[x - 1] = -signDown1;
            rec[x] = clipTbl[rec[x] + m_iOffsetEo[edgeType]];
            for (x = startX + 1; x < endX; x++)
            {
                signDown1      =  xSign(rec[x] - rec[x + stride - 1]);
                edgeType      =  signDown1 + m_iUpBuff1[x] + 2;
                m_iUpBuff1[x - 1] = -signDown1;
                rec[x] = clipTbl[rec[x] + m_iOffsetEo[edgeType]];
            }

            m_iUpBuff1[endX - 1] = xSign(rec[endX - 1 + stride] - rec[endX]);

            rec += stride;
        }

        break;
    }
    case SAO_BO:
    {
        for (y = 0; y < lcuHeight; y++)
        {
            for (x = 0; x < lcuWidth; x++)
            {
                rec[x] = offsetBo[rec[x]];
            }

            rec += stride;
        }

        break;
    }
    default: break;
    }

//   if (iSaoType!=SAO_BO_0 || iSaoType!=SAO_BO_1)
    {
        tmpLSwap = m_pTmpL1;
        m_pTmpL1  = m_pTmpL2;
        m_pTmpL2  = tmpLSwap;
    }
}

/** Sample adaptive offset process
 * \param pcPic, pcSaoParam
 */
Void TComSampleAdaptiveOffset::SAOProcess(SAOParam* pcSaoParam)
{
    {
        m_uiSaoBitIncreaseY = max(g_bitDepthY - 10, 0);
        m_uiSaoBitIncreaseC = max(g_bitDepthC - 10, 0);

        if (m_saoLcuBasedOptimization)
        {
            pcSaoParam->oneUnitFlag[0] = 0;
            pcSaoParam->oneUnitFlag[1] = 0;
            pcSaoParam->oneUnitFlag[2] = 0;
        }
        Int iY  = 0;
        {
            processSaoUnitAll(pcSaoParam->saoLcuParam[iY], pcSaoParam->oneUnitFlag[iY], iY);
        }
        {
            processSaoUnitAll(pcSaoParam->saoLcuParam[1], pcSaoParam->oneUnitFlag[1], 1); //Cb
            processSaoUnitAll(pcSaoParam->saoLcuParam[2], pcSaoParam->oneUnitFlag[2], 2); //Cr
        }
        m_pcPic = NULL;
    }
}

Pel* TComSampleAdaptiveOffset::getPicYuvAddr(TComPicYuv* pcPicYuv, Int yCbCr, Int addr)
{
    switch (yCbCr)
    {
    case 0:
        return pcPicYuv->getLumaAddr(addr);
        break;
    case 1:
        return pcPicYuv->getCbAddr(addr);
        break;
    case 2:
        return pcPicYuv->getCrAddr(addr);
        break;
    default:
        return NULL;
        break;
    }
}

/** Process SAO all units
 * \param saoLcuParam SAO LCU parameters
 * \param oneUnitFlag one unit flag
 * \param yCbCr color componet index
 */
Void TComSampleAdaptiveOffset::processSaoUnitAll(SaoLcuParam* saoLcuParam, Bool oneUnitFlag, Int yCbCr)
{
    Pel *rec;
    Int picWidthTmp;

    if (yCbCr == 0)
    {
        rec        = m_pcPic->getPicYuvRec()->getLumaAddr();
        picWidthTmp = m_iPicWidth;
    }
    else if (yCbCr == 1)
    {
        rec        = m_pcPic->getPicYuvRec()->getCbAddr();
        picWidthTmp = m_iPicWidth >> 1;
    }
    else
    {
        rec        = m_pcPic->getPicYuvRec()->getCrAddr();
        picWidthTmp = m_iPicWidth >> 1;
    }

    memcpy(m_pTmpU1, rec, sizeof(Pel) * picWidthTmp);

    Int  i;
    UInt edgeType;
    Pel* lumaTable = NULL;
    Pel* clipTable = NULL;
    Int* offsetBo = NULL;
    Int  typeIdx;

    Int offset[LUMA_GROUP_NUM + 1];
    Int idxX;
    Int idxY;
    Int addr;
    Int frameWidthInCU = m_pcPic->getFrameWidthInCU();
    Int frameHeightInCU = m_pcPic->getFrameHeightInCU();
    Int stride;
    Pel *tmpUSwap;
    Int sChroma = (yCbCr == 0) ? 0 : 1;
    Bool mergeLeftFlag;
    Int saoBitIncrease = (yCbCr == 0) ? m_uiSaoBitIncreaseY : m_uiSaoBitIncreaseC;

    offsetBo = (yCbCr == 0) ? m_iOffsetBo : m_iChromaOffsetBo;

    offset[0] = 0;
    for (idxY = 0; idxY < frameHeightInCU; idxY++)
    {
        addr = idxY * frameWidthInCU;
        if (yCbCr == 0)
        {
            rec  = m_pcPic->getPicYuvRec()->getLumaAddr(addr);
            stride = m_pcPic->getStride();
            picWidthTmp = m_iPicWidth;
        }
        else if (yCbCr == 1)
        {
            rec  = m_pcPic->getPicYuvRec()->getCbAddr(addr);
            stride = m_pcPic->getCStride();
            picWidthTmp = m_iPicWidth >> 1;
        }
        else
        {
            rec  = m_pcPic->getPicYuvRec()->getCrAddr(addr);
            stride = m_pcPic->getCStride();
            picWidthTmp = m_iPicWidth >> 1;
        }

        //     pRec += stride*(m_uiMaxCUHeight-1);
        for (i = 0; i < (m_uiMaxCUHeight >> sChroma) + 1; i++)
        {
            m_pTmpL1[i] = rec[0];
            rec += stride;
        }

        rec -= (stride << 1);

        memcpy(m_pTmpU2, rec, sizeof(Pel) * picWidthTmp);

        for (idxX = 0; idxX < frameWidthInCU; idxX++)
        {
            addr = idxY * frameWidthInCU + idxX;

            if (oneUnitFlag)
            {
                typeIdx = saoLcuParam[0].typeIdx;
                mergeLeftFlag = (addr == 0) ? 0 : 1;
            }
            else
            {
                typeIdx = saoLcuParam[addr].typeIdx;
                mergeLeftFlag = saoLcuParam[addr].mergeLeftFlag;
            }
            if (typeIdx >= 0)
            {
                if (!mergeLeftFlag)
                {
                    if (typeIdx == SAO_BO)
                    {
                        for (i = 0; i < SAO_MAX_BO_CLASSES + 1; i++)
                        {
                            offset[i] = 0;
                        }

                        for (i = 0; i < saoLcuParam[addr].length; i++)
                        {
                            offset[(saoLcuParam[addr].subTypeIdx + i) % SAO_MAX_BO_CLASSES  + 1] = saoLcuParam[addr].offset[i] << saoBitIncrease;
                        }

                        lumaTable = (yCbCr == 0) ? m_lumaTableBo : m_chromaTableBo;
                        clipTable = (yCbCr == 0) ? m_pClipTable : m_pChromaClipTable;

                        Int bitDepth = (yCbCr == 0) ? g_bitDepthY : g_bitDepthC;
                        for (i = 0; i < (1 << bitDepth); i++)
                        {
                            offsetBo[i] = clipTable[i + offset[lumaTable[i]]];
                        }
                    }
                    if (typeIdx == SAO_EO_0 || typeIdx == SAO_EO_1 || typeIdx == SAO_EO_2 || typeIdx == SAO_EO_3)
                    {
                        for (i = 0; i < saoLcuParam[addr].length; i++)
                        {
                            offset[i + 1] = saoLcuParam[addr].offset[i] << saoBitIncrease;
                        }

                        for (edgeType = 0; edgeType < 6; edgeType++)
                        {
                            m_iOffsetEo[edgeType] = offset[m_auiEoTable[edgeType]];
                        }
                    }
                }
                processSaoCu(addr, typeIdx, yCbCr);
            }
            else
            {
                if (idxX != (frameWidthInCU - 1))
                {
                    if (yCbCr == 0)
                    {
                        rec  = m_pcPic->getPicYuvRec()->getLumaAddr(addr);
                        stride = m_pcPic->getStride();
                    }
                    else if (yCbCr == 1)
                    {
                        rec  = m_pcPic->getPicYuvRec()->getCbAddr(addr);
                        stride = m_pcPic->getCStride();
                    }
                    else
                    {
                        rec  = m_pcPic->getPicYuvRec()->getCrAddr(addr);
                        stride = m_pcPic->getCStride();
                    }
                    Int widthShift = m_uiMaxCUWidth >> sChroma;
                    for (i = 0; i < (m_uiMaxCUHeight >> sChroma) + 1; i++)
                    {
                        m_pTmpL1[i] = rec[widthShift - 1];
                        rec += stride;
                    }
                }
            }
        }

        tmpUSwap = m_pTmpU1;
        m_pTmpU1 = m_pTmpU2;
        m_pTmpU2 = tmpUSwap;
    }
}

/** Reset SAO LCU part
 * \param saoLcuParam
 */
Void TComSampleAdaptiveOffset::resetLcuPart(SaoLcuParam* saoLcuParam)
{
    Int i, j;

    for (i = 0; i < m_iNumCuInWidth * m_iNumCuInHeight; i++)
    {
        saoLcuParam[i].mergeUpFlag     =  1;
        saoLcuParam[i].mergeLeftFlag =  0;
        saoLcuParam[i].partIdx   =  0;
        saoLcuParam[i].typeIdx      = -1;
        for (j = 0; j < MAX_NUM_SAO_OFFSETS; j++)
        {
            saoLcuParam[i].offset[j] = 0;
        }

        saoLcuParam[i].subTypeIdx = 0;
    }
}

/** convert QP part to SAO unit
* \param saoParam SAO parameter
* \param partIdx SAO part index
* \param yCbCr color component index
 */
Void TComSampleAdaptiveOffset::convertQT2SaoUnit(SAOParam *saoParam, UInt partIdx, Int yCbCr)
{
    SAOQTPart*  saoPart = &(saoParam->psSaoPart[yCbCr][partIdx]);

    if (!saoPart->bSplit)
    {
        convertOnePart2SaoUnit(saoParam, partIdx, yCbCr);
        return;
    }

    if (saoPart->PartLevel < m_uiMaxSplitLevel)
    {
        convertQT2SaoUnit(saoParam, saoPart->DownPartsIdx[0], yCbCr);
        convertQT2SaoUnit(saoParam, saoPart->DownPartsIdx[1], yCbCr);
        convertQT2SaoUnit(saoParam, saoPart->DownPartsIdx[2], yCbCr);
        convertQT2SaoUnit(saoParam, saoPart->DownPartsIdx[3], yCbCr);
    }
}

/** convert one SAO part to SAO unit
* \param saoParam SAO parameter
* \param partIdx SAO part index
* \param yCbCr color component index
 */
Void TComSampleAdaptiveOffset::convertOnePart2SaoUnit(SAOParam *saoParam, UInt partIdx, Int yCbCr)
{
    Int j;
    Int idxX;
    Int idxY;
    Int addr;
    Int frameWidthInCU = m_pcPic->getFrameWidthInCU();
    SAOQTPart* saoQTPart = saoParam->psSaoPart[yCbCr];
    SaoLcuParam* saoLcuParam = saoParam->saoLcuParam[yCbCr];

    for (idxY = saoQTPart[partIdx].StartCUY; idxY <= saoQTPart[partIdx].EndCUY; idxY++)
    {
        for (idxX = saoQTPart[partIdx].StartCUX; idxX <= saoQTPart[partIdx].EndCUX; idxX++)
        {
            addr = idxY * frameWidthInCU + idxX;
            saoLcuParam[addr].partIdxTmp = (Int)partIdx;
            saoLcuParam[addr].typeIdx    = saoQTPart[partIdx].iBestType;
            saoLcuParam[addr].subTypeIdx = saoQTPart[partIdx].subTypeIdx;
            if (saoLcuParam[addr].typeIdx != -1)
            {
                saoLcuParam[addr].length    = saoQTPart[partIdx].iLength;
                for (j = 0; j < MAX_NUM_SAO_OFFSETS; j++)
                {
                    saoLcuParam[addr].offset[j] = saoQTPart[partIdx].iOffset[j];
                }
            }
            else
            {
                saoLcuParam[addr].length    = 0;
                saoLcuParam[addr].subTypeIdx = saoQTPart[partIdx].subTypeIdx;
                for (j = 0; j < MAX_NUM_SAO_OFFSETS; j++)
                {
                    saoLcuParam[addr].offset[j] = 0;
                }
            }
        }
    }
}

Void TComSampleAdaptiveOffset::resetSaoUnit(SaoLcuParam* saoUnit)
{
    saoUnit->partIdx       = 0;
    saoUnit->partIdxTmp    = 0;
    saoUnit->mergeLeftFlag = 0;
    saoUnit->mergeUpFlag   = 0;
    saoUnit->typeIdx       = -1;
    saoUnit->length        = 0;
    saoUnit->subTypeIdx    = 0;

    for (Int i = 0; i < 4; i++)
    {
        saoUnit->offset[i] = 0;
    }
}

Void TComSampleAdaptiveOffset::copySaoUnit(SaoLcuParam* saoUnitDst, SaoLcuParam* saoUnitSrc)
{
    saoUnitDst->mergeLeftFlag = saoUnitSrc->mergeLeftFlag;
    saoUnitDst->mergeUpFlag   = saoUnitSrc->mergeUpFlag;
    saoUnitDst->typeIdx       = saoUnitSrc->typeIdx;
    saoUnitDst->length        = saoUnitSrc->length;

    saoUnitDst->subTypeIdx  = saoUnitSrc->subTypeIdx;
    for (Int i = 0; i < 4; i++)
    {
        saoUnitDst->offset[i] = saoUnitSrc->offset[i];
    }
}

/** PCM LF disable process.
 * \param pcPic picture (TComPic) pointer
 * \returns Void
 *
 * \note Replace filtered sample values of PCM mode blocks with the transmitted and reconstructed ones.
 */
Void TComSampleAdaptiveOffset::PCMLFDisableProcess(TComPic* pcPic)
{
    xPCMRestoration(pcPic);
}

/** Picture-level PCM restoration.
 * \param pcPic picture (TComPic) pointer
 * \returns Void
 */
Void TComSampleAdaptiveOffset::xPCMRestoration(TComPic* pcPic)
{
    Bool  bPCMFilter = (pcPic->getSlice()->getSPS()->getUsePCM() && pcPic->getSlice()->getSPS()->getPCMFilterDisableFlag()) ? true : false;

    if (bPCMFilter || pcPic->getSlice()->getPPS()->getTransquantBypassEnableFlag())
    {
        for (UInt cuAddr = 0; cuAddr < pcPic->getNumCUsInFrame(); cuAddr++)
        {
            TComDataCU* cu = pcPic->getCU(cuAddr);

            xPCMCURestoration(cu, 0, 0);
        }
    }
}

/** PCM CU restoration.
 * \param cu pointer to current CU
 * \param absPartIdx part index
 * \param depth CU depth
 * \returns Void
 */
Void TComSampleAdaptiveOffset::xPCMCURestoration(TComDataCU* cu, UInt absZOrderIdx, UInt depth)
{
    TComPic* pcPic     = cu->getPic();
    UInt uiCurNumParts = pcPic->getNumPartInCU() >> (depth << 1);
    UInt qNumParts   = uiCurNumParts >> 2;

    // go to sub-CU
    if (cu->getDepth(absZOrderIdx) > depth)
    {
        for (UInt partIdx = 0; partIdx < 4; partIdx++, absZOrderIdx += qNumParts)
        {
            UInt lpelx   = cu->getCUPelX() + g_rasterToPelX[g_zscanToRaster[absZOrderIdx]];
            UInt tpely   = cu->getCUPelY() + g_rasterToPelY[g_zscanToRaster[absZOrderIdx]];
            if ((lpelx < cu->getSlice()->getSPS()->getPicWidthInLumaSamples()) && (tpely < cu->getSlice()->getSPS()->getPicHeightInLumaSamples()))
                xPCMCURestoration(cu, absZOrderIdx, depth + 1);
        }

        return;
    }

    // restore PCM samples
    if ((cu->getIPCMFlag(absZOrderIdx) && pcPic->getSlice()->getSPS()->getPCMFilterDisableFlag()) || cu->isLosslessCoded(absZOrderIdx))
    {
        xPCMSampleRestoration(cu, absZOrderIdx, depth, TEXT_LUMA);
        xPCMSampleRestoration(cu, absZOrderIdx, depth, TEXT_CHROMA_U);
        xPCMSampleRestoration(cu, absZOrderIdx, depth, TEXT_CHROMA_V);
    }
}

/** PCM sample restoration.
 * \param cu pointer to current CU
 * \param absPartIdx part index
 * \param depth CU depth
 * \param ttText texture component type
 * \returns Void
 */
Void TComSampleAdaptiveOffset::xPCMSampleRestoration(TComDataCU* cu, UInt absZOrderIdx, UInt depth, TextType ttText)
{
    TComPicYuv* pcPicYuvRec = cu->getPic()->getPicYuvRec();
    Pel* src;
    Pel* pcm;
    UInt stride;
    UInt width;
    UInt height;
    UInt pcmLeftShiftBit;
    UInt x, y;
    UInt minCoeffSize = cu->getPic()->getMinCUWidth() * cu->getPic()->getMinCUHeight();
    UInt lumaOffset   = minCoeffSize * absZOrderIdx;
    UInt chromaOffset = lumaOffset >> 2;

    if (ttText == TEXT_LUMA)
    {
        src = pcPicYuvRec->getLumaAddr(cu->getAddr(), absZOrderIdx);
        pcm = cu->getPCMSampleY() + lumaOffset;
        stride  = pcPicYuvRec->getStride();
        width  = (g_maxCUWidth >> depth);
        height = (g_maxCUHeight >> depth);
        if (cu->isLosslessCoded(absZOrderIdx) && !cu->getIPCMFlag(absZOrderIdx))
        {
            pcmLeftShiftBit = 0;
        }
        else
        {
            pcmLeftShiftBit = g_bitDepthY - cu->getSlice()->getSPS()->getPCMBitDepthLuma();
        }
    }
    else
    {
        if (ttText == TEXT_CHROMA_U)
        {
            src = pcPicYuvRec->getCbAddr(cu->getAddr(), absZOrderIdx);
            pcm = cu->getPCMSampleCb() + chromaOffset;
        }
        else
        {
            src = pcPicYuvRec->getCrAddr(cu->getAddr(), absZOrderIdx);
            pcm = cu->getPCMSampleCr() + chromaOffset;
        }

        stride = pcPicYuvRec->getCStride();
        width  = ((g_maxCUWidth >> depth) / 2);
        height = ((g_maxCUWidth >> depth) / 2);
        if (cu->isLosslessCoded(absZOrderIdx) && !cu->getIPCMFlag(absZOrderIdx))
        {
            pcmLeftShiftBit = 0;
        }
        else
        {
            pcmLeftShiftBit = g_bitDepthC - cu->getSlice()->getSPS()->getPCMBitDepthChroma();
        }
    }

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            src[x] = (pcm[x] << pcmLeftShiftBit);
        }

        pcm += width;
        src += stride;
    }
}

//! \}
