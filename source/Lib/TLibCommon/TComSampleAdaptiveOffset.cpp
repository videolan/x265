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
        if (saoPart[i])
        {
            delete [] saoPart[i];
        }
    }
}

// ====================================================================================================================
// Tables
// ====================================================================================================================

TComSampleAdaptiveOffset::TComSampleAdaptiveOffset()
{
    m_clipTable = NULL;
    m_clipTableBase = NULL;
    m_chromaClipTable = NULL;
    m_chromaClipTableBase = NULL;
    m_offsetBo = NULL;
    m_chromaOffsetBo = NULL;
    m_lumaTableBo = NULL;
    m_chromaTableBo = NULL;
    m_upBuff1 = NULL;
    m_upBuff2 = NULL;
    m_upBufft = NULL;
    m_swap = NULL;

    m_tmpU1 = NULL;
    m_tmpU2 = NULL;
    m_tmpL1 = NULL;
    m_tmpL2 = NULL;
}

TComSampleAdaptiveOffset::~TComSampleAdaptiveOffset()
{}

const Int TComSampleAdaptiveOffset::m_numCulPartsLevel[5] =
{
    1, //level 0
    5, //level 1
    21, //level 2
    85, //level 3
    341, //level 4
};

const UInt TComSampleAdaptiveOffset::m_eoTable[9] =
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

const Int TComSampleAdaptiveOffset::m_numClass[MAX_NUM_SAO_TYPE] =
{
    SAO_EO_LEN,
    SAO_EO_LEN,
    SAO_EO_LEN,
    SAO_EO_LEN,
    SAO_BO_LEN
};

const UInt TComSampleAdaptiveOffset::m_maxDepth = SAO_MAX_DEPTH;

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
    m_picWidth  = sourceWidth;
    m_picHeight = sourceHeight;

    m_maxCUWidth  = maxCUWidth;
    m_maxCUHeight = maxCUHeight;

    m_numCuInWidth  = m_picWidth / m_maxCUWidth;
    m_numCuInWidth += (m_picWidth % m_maxCUWidth) ? 1 : 0;

    m_numCuInHeight  = m_picHeight / m_maxCUHeight;
    m_numCuInHeight += (m_picHeight % m_maxCUHeight) ? 1 : 0;

    Int maxSplitLevelHeight = (Int)(logf((Float)m_numCuInHeight) / logf(2.0));
    Int maxSplitLevelWidth  = (Int)(logf((Float)m_numCuInWidth) / logf(2.0));

    m_maxSplitLevel = (maxSplitLevelHeight < maxSplitLevelWidth) ? (maxSplitLevelHeight) : (maxSplitLevelWidth);
    m_maxSplitLevel = (m_maxSplitLevel < m_maxDepth) ? (m_maxSplitLevel) : (m_maxDepth);

    /* various structures are overloaded to store per component data.
     * m_iNumTotalParts must allow for sufficient storage in any allocated arrays */
    m_numTotalParts  = max(3, m_numCulPartsLevel[m_maxSplitLevel]);

    UInt pixelRangeY = 1 << X265_DEPTH;
    UInt boRangeShiftY = X265_DEPTH - SAO_BO_BITS;

    m_lumaTableBo = new Pel[pixelRangeY];
    for (Int k2 = 0; k2 < pixelRangeY; k2++)
    {
        m_lumaTableBo[k2] = 1 + (k2 >> boRangeShiftY);
    }

    UInt pixelRangeC = 1 << X265_DEPTH;
    UInt boRangeShiftC = X265_DEPTH - SAO_BO_BITS;

    m_chromaTableBo = new Pel[pixelRangeC];
    for (Int k2 = 0; k2 < pixelRangeC; k2++)
    {
        m_chromaTableBo[k2] = 1 + (k2 >> boRangeShiftC);
    }

    m_upBuff1 = new Int[m_picWidth + 2];
    m_upBuff2 = new Int[m_picWidth + 2];
    m_upBufft = new Int[m_picWidth + 2];

    m_upBuff1++;
    m_upBuff2++;
    m_upBufft++;
    Short i;

    UInt maxY  = (1 << X265_DEPTH) - 1;
    UInt minY  = 0;

    Int rangeExt = maxY >> 1;

    m_clipTableBase = new Pel[maxY + 2 * rangeExt];
    m_offsetBo      = new Int[maxY + 2 * rangeExt];

    for (i = 0; i < (minY + rangeExt); i++)
    {
        m_clipTableBase[i] = minY;
    }

    for (i = minY + rangeExt; i < (maxY +  rangeExt); i++)
    {
        m_clipTableBase[i] = i - rangeExt;
    }

    for (i = maxY + rangeExt; i < (maxY + 2 * rangeExt); i++)
    {
        m_clipTableBase[i] = maxY;
    }

    m_clipTable = &(m_clipTableBase[rangeExt]);

    UInt maxC = (1 << X265_DEPTH) - 1;
    UInt minC = 0;

    Int rangeExtC = maxC >> 1;

    m_chromaClipTableBase = new Pel[maxC + 2 * rangeExtC];
    m_chromaOffsetBo      = new Int[maxC + 2 * rangeExtC];

    for (i = 0; i < (minC + rangeExtC); i++)
    {
        m_chromaClipTableBase[i] = minC;
    }

    for (i = minC + rangeExtC; i < (maxC +  rangeExtC); i++)
    {
        m_chromaClipTableBase[i] = i - rangeExtC;
    }

    for (i = maxC + rangeExtC; i < (maxC + 2 * rangeExtC); i++)
    {
        m_chromaClipTableBase[i] = maxC;
    }

    m_chromaClipTable = &(m_chromaClipTableBase[rangeExtC]);

    m_tmpL1 = new Pel[m_maxCUHeight + 1];
    m_tmpL2 = new Pel[m_maxCUHeight + 1];
    m_tmpU1 = new Pel[m_picWidth];
    m_tmpU2 = new Pel[m_picWidth];
}

/** destroy SampleAdaptiveOffset memory.
 * \param
 */
Void TComSampleAdaptiveOffset::destroy()
{
    if (m_clipTableBase)
    {
        delete [] m_clipTableBase;
        m_clipTableBase = NULL;
    }
    if (m_offsetBo)
    {
        delete [] m_offsetBo;
        m_offsetBo = NULL;
    }
    if (m_lumaTableBo)
    {
        delete[] m_lumaTableBo;
        m_lumaTableBo = NULL;
    }

    if (m_chromaClipTableBase)
    {
        delete [] m_chromaClipTableBase;
        m_chromaClipTableBase = NULL;
    }
    if (m_chromaOffsetBo)
    {
        delete [] m_chromaOffsetBo;
        m_chromaOffsetBo = NULL;
    }
    if (m_chromaTableBo)
    {
        delete[] m_chromaTableBo;
        m_chromaTableBo = NULL;
    }

    if (m_upBuff1)
    {
        m_upBuff1--;
        delete [] m_upBuff1;
        m_upBuff1 = NULL;
    }
    if (m_upBuff2)
    {
        m_upBuff2--;
        delete [] m_upBuff2;
        m_upBuff2 = NULL;
    }
    if (m_upBufft)
    {
        m_upBufft--;
        delete [] m_upBufft;
        m_upBufft = NULL;
    }
    if (m_tmpL1)
    {
        delete [] m_tmpL1;
        m_tmpL1 = NULL;
    }
    if (m_tmpL2)
    {
        delete [] m_tmpL2;
        m_tmpL2 = NULL;
    }
    if (m_tmpU1)
    {
        delete [] m_tmpU1;
        m_tmpU1 = NULL;
    }
    if (m_tmpU2)
    {
        delete [] m_tmpU2;
        m_tmpU2 = NULL;
    }
}

/** allocate memory for SAO parameters
 * \param    *saoParam
 */
Void TComSampleAdaptiveOffset::allocSaoParam(SAOParam *saoParam)
{
    saoParam->maxSplitLevel = m_maxSplitLevel;
    saoParam->saoPart[0] = new SAOQTPart[m_numCulPartsLevel[saoParam->maxSplitLevel]];
    initSAOParam(saoParam, 0, 0, 0, -1, 0, m_numCuInWidth - 1,  0, m_numCuInHeight - 1, 0);
    saoParam->saoPart[1] = new SAOQTPart[m_numCulPartsLevel[saoParam->maxSplitLevel]];
    saoParam->saoPart[2] = new SAOQTPart[m_numCulPartsLevel[saoParam->maxSplitLevel]];
    initSAOParam(saoParam, 0, 0, 0, -1, 0, m_numCuInWidth - 1,  0, m_numCuInHeight - 1, 1);
    initSAOParam(saoParam, 0, 0, 0, -1, 0, m_numCuInWidth - 1,  0, m_numCuInHeight - 1, 2);
    saoParam->numCuInWidth  = m_numCuInWidth;
    saoParam->numCuInHeight = m_numCuInHeight;
    saoParam->saoLcuParam[0] = new SaoLcuParam[m_numCuInHeight * m_numCuInWidth];
    saoParam->saoLcuParam[1] = new SaoLcuParam[m_numCuInHeight * m_numCuInWidth];
    saoParam->saoLcuParam[2] = new SaoLcuParam[m_numCuInHeight * m_numCuInWidth];
}

/** initialize SAO parameters
 * \param    *saoParam,  iPartLevel,  iPartRow,  iPartCol,  iParentPartIdx,  StartCUX,  EndCUX,  StartCUY,  EndCUY,  yCbCr
 */
Void TComSampleAdaptiveOffset::initSAOParam(SAOParam *saoParam, Int partLevel, Int partRow, Int partCol, Int parentPartIdx, Int startCUX, Int endCUX, Int startCUY, Int endCUY, Int yCbCr)
{
    Int j;
    Int partIdx = convertLevelRowCol2Idx(partLevel, partRow, partCol);

    SAOQTPart* saoPart;

    saoPart = &(saoParam->saoPart[yCbCr][partIdx]);

    saoPart->partIdx   = partIdx;
    saoPart->partLevel = partLevel;
    saoPart->partRow   = partRow;
    saoPart->partCol   = partCol;

    saoPart->startCUX  = startCUX;
    saoPart->endCUX    = endCUX;
    saoPart->startCUY  = startCUY;
    saoPart->endCUY    = endCUY;

    saoPart->upPartIdx = parentPartIdx;
    saoPart->bestType  = -1;
    saoPart->length    =  0;

    saoPart->subTypeIdx = 0;

    for (j = 0; j < MAX_NUM_SAO_OFFSETS; j++)
    {
        saoPart->offset[j] = 0;
    }

    if (saoPart->partLevel != m_maxSplitLevel)
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

        saoPart->downPartsIdx[0] = convertLevelRowCol2Idx(downLevel, downRowIdx, downColIdx);

        initSAOParam(saoParam, downLevel, downRowIdx, downColIdx, partIdx, downStartCUX, downEndCUX, downStartCUY, downEndCUY, yCbCr);

        downStartCUX = startCUX + numCULeft;
        downEndCUX   = endCUX;
        downStartCUY = startCUY;
        downEndCUY   = downStartCUY + numCUTop - 1;
        downRowIdx  = downRowStart + 0;
        downColIdx  = downColStart + 1;

        saoPart->downPartsIdx[1] = convertLevelRowCol2Idx(downLevel, downRowIdx, downColIdx);

        initSAOParam(saoParam, downLevel, downRowIdx, downColIdx, partIdx,  downStartCUX, downEndCUX, downStartCUY, downEndCUY, yCbCr);

        downStartCUX = startCUX;
        downEndCUX   = downStartCUX + numCULeft - 1;
        downStartCUY = startCUY + numCUTop;
        downEndCUY   = endCUY;
        downRowIdx  = downRowStart + 1;
        downColIdx  = downColStart + 0;

        saoPart->downPartsIdx[2] = convertLevelRowCol2Idx(downLevel, downRowIdx, downColIdx);

        initSAOParam(saoParam, downLevel, downRowIdx, downColIdx, partIdx, downStartCUX, downEndCUX, downStartCUY, downEndCUY, yCbCr);

        downStartCUX = startCUX + numCULeft;
        downEndCUX   = endCUX;
        downStartCUY = startCUY + numCUTop;
        downEndCUY   = endCUY;
        downRowIdx  = downRowStart + 1;
        downColIdx  = downColStart + 1;

        saoPart->downPartsIdx[3] = convertLevelRowCol2Idx(downLevel, downRowIdx, downColIdx);

        initSAOParam(saoParam, downLevel, downRowIdx, downColIdx, partIdx, downStartCUX, downEndCUX, downStartCUY, downEndCUY, yCbCr);
    }
    else
    {
        saoPart->downPartsIdx[0] = saoPart->downPartsIdx[1] = saoPart->downPartsIdx[2] = saoPart->downPartsIdx[3] = -1;
    }
}

/** free memory of SAO parameters
 * \param   saoParam
 */
Void TComSampleAdaptiveOffset::freeSaoParam(SAOParam *saoParam)
{
    delete [] saoParam->saoPart[0];
    delete [] saoParam->saoPart[1];
    delete [] saoParam->saoPart[2];
    saoParam->saoPart[0] = 0;
    saoParam->saoPart[1] = 0;
    saoParam->saoPart[2] = 0;
    if (saoParam->saoLcuParam[0])
    {
        delete [] saoParam->saoLcuParam[0];
        saoParam->saoLcuParam[0] = NULL;
    }
    if (saoParam->saoLcuParam[1])
    {
        delete [] saoParam->saoLcuParam[1];
        saoParam->saoLcuParam[1] = NULL;
    }
    if (saoParam->saoLcuParam[2])
    {
        delete [] saoParam->saoLcuParam[2];
        saoParam->saoLcuParam[2] = NULL;
    }
}

/** reset SAO parameters
 * \param   saoParam
 */
Void TComSampleAdaptiveOffset::resetSAOParam(SAOParam *saoParam)
{
    Int numComponet = 3;

    for (Int c = 0; c < numComponet; c++)
    {
        if (c < 2)
        {
            saoParam->bSaoFlag[c] = 0;
        }
        for (Int i = 0; i < m_numCulPartsLevel[m_maxSplitLevel]; i++)
        {
            saoParam->saoPart[c][i].bestType     = -1;
            saoParam->saoPart[c][i].length       =  0;
            saoParam->saoPart[c][i].bSplit       = false;
            saoParam->saoPart[c][i].bProcessed   = false;
            saoParam->saoPart[c][i].minCost      = MAX_DOUBLE;
            saoParam->saoPart[c][i].minDist      = MAX_INT;
            saoParam->saoPart[c][i].minRate      = MAX_INT;
            saoParam->saoPart[c][i].subTypeIdx   = 0;
            for (Int j = 0; j < MAX_NUM_SAO_OFFSETS; j++)
            {
                saoParam->saoPart[c][i].offset[j] = 0;
                saoParam->saoPart[c][i].offset[j] = 0;
                saoParam->saoPart[c][i].offset[j] = 0;
            }
        }

        saoParam->oneUnitFlag[0] = 0;
        saoParam->oneUnitFlag[1] = 0;
        saoParam->oneUnitFlag[2] = 0;
        resetLcuPart(saoParam->saoLcuParam[0]);
        resetLcuPart(saoParam->saoLcuParam[1]);
        resetLcuPart(saoParam->saoLcuParam[2]);
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
 * \param  pic picture data pointer
 */
Void TComSampleAdaptiveOffset::createPicSaoInfo(TComPic* pic)
{
    m_pic = pic;
}

Void TComSampleAdaptiveOffset::destroyPicSaoInfo()
{
}

/** sample adaptive offset process for one LCU
 * \param   addr, iSaoType, yCbCr
 */
Void TComSampleAdaptiveOffset::processSaoCu(Int addr, Int saoType, Int yCbCr)
{
    processSaoCuOrg(addr, saoType, yCbCr);
}

/** sample adaptive offset process for one LCU crossing LCU boundary
 * \param   addr, iSaoType, yCbCr
 */
Void TComSampleAdaptiveOffset::processSaoCuOrg(Int addr, Int saoType, Int yCbCr)
{
    Int x, y;
    TComDataCU *tmpCu = m_pic->getCU(addr);
    Pel* rec;
    Int  stride;
    Int  lcuWidth  = m_maxCUWidth;
    Int  lcuHeight = m_maxCUHeight;
    UInt lpelx     = tmpCu->getCUPelX();
    UInt tpely     = tmpCu->getCUPelY();
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

    picWidthTmp  = m_picWidth  >> isChroma;
    picHeightTmp = m_picHeight >> isChroma;
    lcuWidth     = lcuWidth    >> isChroma;
    lcuHeight    = lcuHeight   >> isChroma;
    lpelx        = lpelx      >> isChroma;
    tpely        = tpely      >> isChroma;
    rpelx        = lpelx + lcuWidth;
    bpely        = tpely + lcuHeight;
    rpelx        = rpelx > picWidthTmp  ? picWidthTmp  : rpelx;
    bpely        = bpely > picHeightTmp ? picHeightTmp : bpely;
    lcuWidth     = rpelx - lpelx;
    lcuHeight    = bpely - tpely;

    if (tmpCu->getPic() == 0)
    {
        return;
    }
    if (yCbCr == 0)
    {
        rec    = m_pic->getPicYuvRec()->getLumaAddr(addr);
        stride = m_pic->getStride();
    }
    else if (yCbCr == 1)
    {
        rec    = m_pic->getPicYuvRec()->getCbAddr(addr);
        stride = m_pic->getCStride();
    }
    else
    {
        rec    = m_pic->getPicYuvRec()->getCrAddr(addr);
        stride = m_pic->getCStride();
    }

//   if (iSaoType!=SAO_BO_0 || iSaoType!=SAO_BO_1)
    {
        cuHeightTmp = (m_maxCUHeight >> isChroma);
        shift = (m_maxCUWidth >> isChroma) - 1;
        for (Int i = 0; i < cuHeightTmp + 1; i++)
        {
            m_tmpL2[i] = rec[shift];
            rec += stride;
        }

        rec -= (stride * (cuHeightTmp + 1));

        tmpL = m_tmpL1;
        tmpU = &(m_tmpU1[lpelx]);
    }

    clipTbl = (yCbCr == 0) ? m_clipTable : m_chromaClipTable;
    offsetBo = (yCbCr == 0) ? m_offsetBo : m_chromaOffsetBo;

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

                rec[x] = clipTbl[rec[x] + m_offsetEo[edgeType]];
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
            m_upBuff1[x] = xSign(rec[x] - tmpU[x]);
        }

        for (y = startY; y < endY; y++)
        {
            for (x = 0; x < lcuWidth; x++)
            {
                signDown  = xSign(rec[x] - rec[x + stride]);
                edgeType = signDown + m_upBuff1[x] + 2;
                m_upBuff1[x] = -signDown;

                rec[x] = clipTbl[rec[x] + m_offsetEo[edgeType]];
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
            m_upBuff1[x] = xSign(rec[x] - tmpU[x - 1]);
        }

        for (y = startY; y < endY; y++)
        {
            signDown2 = xSign(rec[stride + startX] - tmpL[y]);
            for (x = startX; x < endX; x++)
            {
                signDown1      =  xSign(rec[x] - rec[x + stride + 1]);
                edgeType      =  signDown1 + m_upBuff1[x] + 2;
                m_upBufft[x + 1] = -signDown1;
                rec[x] = clipTbl[rec[x] + m_offsetEo[edgeType]];
            }

            m_upBufft[startX] = signDown2;

            m_swap    = m_upBuff1;
            m_upBuff1 = m_upBufft;
            m_upBufft = m_swap;

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
            m_upBuff1[x] = xSign(rec[x] - tmpU[x + 1]);
        }

        for (y = startY; y < endY; y++)
        {
            x = startX;
            signDown1      =  xSign(rec[x] - tmpL[y + 1]);
            edgeType      =  signDown1 + m_upBuff1[x] + 2;
            m_upBuff1[x - 1] = -signDown1;
            rec[x] = clipTbl[rec[x] + m_offsetEo[edgeType]];
            for (x = startX + 1; x < endX; x++)
            {
                signDown1      =  xSign(rec[x] - rec[x + stride - 1]);
                edgeType      =  signDown1 + m_upBuff1[x] + 2;
                m_upBuff1[x - 1] = -signDown1;
                rec[x] = clipTbl[rec[x] + m_offsetEo[edgeType]];
            }

            m_upBuff1[endX - 1] = xSign(rec[endX - 1 + stride] - rec[endX]);

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
        tmpLSwap = m_tmpL1;
        m_tmpL1  = m_tmpL2;
        m_tmpL2  = tmpLSwap;
    }
}

/** Sample adaptive offset process
 * \param pic, saoParam
 */
Void TComSampleAdaptiveOffset::SAOProcess(SAOParam* saoParam)
{
    {
        m_saoBitIncreaseY = max(X265_DEPTH - 10, 0);
        m_saoBitIncreaseC = max(X265_DEPTH - 10, 0);

        if (m_saoLcuBasedOptimization)
        {
            saoParam->oneUnitFlag[0] = 0;
            saoParam->oneUnitFlag[1] = 0;
            saoParam->oneUnitFlag[2] = 0;
        }
        Int iY  = 0;
        {
            processSaoUnitAll(saoParam->saoLcuParam[iY], saoParam->oneUnitFlag[iY], iY);
        }
        {
            processSaoUnitAll(saoParam->saoLcuParam[1], saoParam->oneUnitFlag[1], 1); //Cb
            processSaoUnitAll(saoParam->saoLcuParam[2], saoParam->oneUnitFlag[2], 2); //Cr
        }
        m_pic = NULL;
    }
}

Pel* TComSampleAdaptiveOffset::getPicYuvAddr(TComPicYuv* picYuv, Int yCbCr, Int addr)
{
    switch (yCbCr)
    {
    case 0:
        return picYuv->getLumaAddr(addr);
        break;
    case 1:
        return picYuv->getCbAddr(addr);
        break;
    case 2:
        return picYuv->getCrAddr(addr);
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
        rec        = m_pic->getPicYuvRec()->getLumaAddr();
        picWidthTmp = m_picWidth;
    }
    else if (yCbCr == 1)
    {
        rec        = m_pic->getPicYuvRec()->getCbAddr();
        picWidthTmp = m_picWidth >> 1;
    }
    else
    {
        rec        = m_pic->getPicYuvRec()->getCrAddr();
        picWidthTmp = m_picWidth >> 1;
    }

    memcpy(m_tmpU1, rec, sizeof(Pel) * picWidthTmp);

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
    Int frameWidthInCU = m_pic->getFrameWidthInCU();
    Int frameHeightInCU = m_pic->getFrameHeightInCU();
    Int stride;
    Pel *tmpUSwap;
    Int sChroma = (yCbCr == 0) ? 0 : 1;
    Bool mergeLeftFlag;
    Int saoBitIncrease = (yCbCr == 0) ? m_saoBitIncreaseY : m_saoBitIncreaseC;

    offsetBo = (yCbCr == 0) ? m_offsetBo : m_chromaOffsetBo;

    offset[0] = 0;
    for (idxY = 0; idxY < frameHeightInCU; idxY++)
    {
        addr = idxY * frameWidthInCU;
        if (yCbCr == 0)
        {
            rec  = m_pic->getPicYuvRec()->getLumaAddr(addr);
            stride = m_pic->getStride();
            picWidthTmp = m_picWidth;
        }
        else if (yCbCr == 1)
        {
            rec  = m_pic->getPicYuvRec()->getCbAddr(addr);
            stride = m_pic->getCStride();
            picWidthTmp = m_picWidth >> 1;
        }
        else
        {
            rec  = m_pic->getPicYuvRec()->getCrAddr(addr);
            stride = m_pic->getCStride();
            picWidthTmp = m_picWidth >> 1;
        }

        //     pRec += stride*(m_uiMaxCUHeight-1);
        for (i = 0; i < (m_maxCUHeight >> sChroma) + 1; i++)
        {
            m_tmpL1[i] = rec[0];
            rec += stride;
        }

        rec -= (stride << 1);

        memcpy(m_tmpU2, rec, sizeof(Pel) * picWidthTmp);

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
                        clipTable = (yCbCr == 0) ? m_clipTable : m_chromaClipTable;

                        for (i = 0; i < (1 << X265_DEPTH); i++)
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
                            m_offsetEo[edgeType] = offset[m_eoTable[edgeType]];
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
                        rec  = m_pic->getPicYuvRec()->getLumaAddr(addr);
                        stride = m_pic->getStride();
                    }
                    else if (yCbCr == 1)
                    {
                        rec  = m_pic->getPicYuvRec()->getCbAddr(addr);
                        stride = m_pic->getCStride();
                    }
                    else
                    {
                        rec  = m_pic->getPicYuvRec()->getCrAddr(addr);
                        stride = m_pic->getCStride();
                    }
                    Int widthShift = m_maxCUWidth >> sChroma;
                    for (i = 0; i < (m_maxCUHeight >> sChroma) + 1; i++)
                    {
                        m_tmpL1[i] = rec[widthShift - 1];
                        rec += stride;
                    }
                }
            }
        }

        tmpUSwap = m_tmpU1;
        m_tmpU1 = m_tmpU2;
        m_tmpU2 = tmpUSwap;
    }
}

/** Reset SAO LCU part
 * \param saoLcuParam
 */
Void TComSampleAdaptiveOffset::resetLcuPart(SaoLcuParam* saoLcuParam)
{
    Int i, j;

    for (i = 0; i < m_numCuInWidth * m_numCuInHeight; i++)
    {
        saoLcuParam[i].mergeUpFlag   =  1;
        saoLcuParam[i].mergeLeftFlag =  0;
        saoLcuParam[i].partIdx       =  0;
        saoLcuParam[i].typeIdx       = -1;
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
    SAOQTPart*  saoPart = &(saoParam->saoPart[yCbCr][partIdx]);

    if (!saoPart->bSplit)
    {
        convertOnePart2SaoUnit(saoParam, partIdx, yCbCr);
        return;
    }

    if (saoPart->partLevel < m_maxSplitLevel)
    {
        convertQT2SaoUnit(saoParam, saoPart->downPartsIdx[0], yCbCr);
        convertQT2SaoUnit(saoParam, saoPart->downPartsIdx[1], yCbCr);
        convertQT2SaoUnit(saoParam, saoPart->downPartsIdx[2], yCbCr);
        convertQT2SaoUnit(saoParam, saoPart->downPartsIdx[3], yCbCr);
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
    Int frameWidthInCU = m_pic->getFrameWidthInCU();
    SAOQTPart* saoQTPart = saoParam->saoPart[yCbCr];
    SaoLcuParam* saoLcuParam = saoParam->saoLcuParam[yCbCr];

    for (idxY = saoQTPart[partIdx].startCUY; idxY <= saoQTPart[partIdx].endCUY; idxY++)
    {
        for (idxX = saoQTPart[partIdx].startCUX; idxX <= saoQTPart[partIdx].endCUX; idxX++)
        {
            addr = idxY * frameWidthInCU + idxX;
            saoLcuParam[addr].partIdxTmp = (Int)partIdx;
            saoLcuParam[addr].typeIdx    = saoQTPart[partIdx].bestType;
            saoLcuParam[addr].subTypeIdx = saoQTPart[partIdx].subTypeIdx;
            if (saoLcuParam[addr].typeIdx != -1)
            {
                saoLcuParam[addr].length    = saoQTPart[partIdx].length;
                for (j = 0; j < MAX_NUM_SAO_OFFSETS; j++)
                {
                    saoLcuParam[addr].offset[j] = saoQTPart[partIdx].offset[j];
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

static Void xPCMRestoration(TComPic* pic);
static Void xPCMCURestoration(TComDataCU* cu, UInt absZOrderIdx, UInt depth);
static Void xPCMSampleRestoration(TComDataCU* cu, UInt absZOrderIdx, UInt depth, TextType ttText);

/** PCM LF disable process.
 * \param pic picture (TComPic) pointer
 * \returns Void
 *
 * \note Replace filtered sample values of PCM mode blocks with the transmitted and reconstructed ones.
 */
Void PCMLFDisableProcess(TComPic* pic)
{
    xPCMRestoration(pic);
}

/** Picture-level PCM restoration.
 * \param pic picture (TComPic) pointer
 * \returns Void
 */
static Void xPCMRestoration(TComPic* pic)
{
    Bool  bPCMFilter = (pic->getSlice()->getSPS()->getUsePCM() && pic->getSlice()->getSPS()->getPCMFilterDisableFlag()) ? true : false;

    if (bPCMFilter || pic->getSlice()->getPPS()->getTransquantBypassEnableFlag())
    {
        for (UInt cuAddr = 0; cuAddr < pic->getNumCUsInFrame(); cuAddr++)
        {
            TComDataCU* cu = pic->getCU(cuAddr);

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
static Void xPCMCURestoration(TComDataCU* cu, UInt absZOrderIdx, UInt depth)
{
    TComPic* pic     = cu->getPic();
    UInt curNumParts = pic->getNumPartInCU() >> (depth << 1);
    UInt qNumParts   = curNumParts >> 2;

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
    if ((cu->getIPCMFlag(absZOrderIdx) && pic->getSlice()->getSPS()->getPCMFilterDisableFlag()) || cu->isLosslessCoded(absZOrderIdx))
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
static Void xPCMSampleRestoration(TComDataCU* cu, UInt absZOrderIdx, UInt depth, TextType ttText)
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
            pcmLeftShiftBit = X265_DEPTH - cu->getSlice()->getSPS()->getPCMBitDepthLuma();
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
            pcmLeftShiftBit = X265_DEPTH - cu->getSlice()->getSPS()->getPCMBitDepthChroma();
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
