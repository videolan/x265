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
#include "common.h"

namespace x265 {
//! \ingroup TLibCommon
//! \{
SAOParam::~SAOParam()
{
    for (int i = 0; i < 3; i++)
    {
        delete [] saoPart[i];
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
    m_tmpU1[0] = NULL;
    m_tmpU1[1] = NULL;
    m_tmpU1[2] = NULL;
    m_tmpU2[0] = NULL;
    m_tmpU2[1] = NULL;
    m_tmpU2[2] = NULL;
    m_tmpL1 = NULL;
    m_tmpL2 = NULL;
}

TComSampleAdaptiveOffset::~TComSampleAdaptiveOffset()
{}

const int TComSampleAdaptiveOffset::m_numCulPartsLevel[5] =
{
    1, //level 0
    5, //level 1
    21, //level 2
    85, //level 3
    341, //level 4
};

const uint32_t TComSampleAdaptiveOffset::m_eoTable[9] =
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

const int TComSampleAdaptiveOffset::m_numClass[MAX_NUM_SAO_TYPE] =
{
    SAO_EO_LEN,
    SAO_EO_LEN,
    SAO_EO_LEN,
    SAO_EO_LEN,
    SAO_BO_LEN
};

const uint32_t TComSampleAdaptiveOffset::m_maxDepth = SAO_MAX_DEPTH;

/** convert Level Row Col to Idx
 * \param   level,  row,  col
 */
int  TComSampleAdaptiveOffset::convertLevelRowCol2Idx(int level, int row, int col) const
{
    int idx;

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
void TComSampleAdaptiveOffset::create(uint32_t sourceWidth, uint32_t sourceHeight, uint32_t maxCUWidth, uint32_t maxCUHeight, int csp)
{
    m_hChromaShift = CHROMA_H_SHIFT(csp);
    m_vChromaShift = CHROMA_V_SHIFT(csp);

    m_picWidth  = sourceWidth;
    m_picHeight = sourceHeight;

    m_maxCUWidth  = maxCUWidth;
    m_maxCUHeight = maxCUHeight;

    m_numCuInWidth  = m_picWidth / m_maxCUWidth;
    m_numCuInWidth += (m_picWidth % m_maxCUWidth) ? 1 : 0;

    m_numCuInHeight  = m_picHeight / m_maxCUHeight;
    m_numCuInHeight += (m_picHeight % m_maxCUHeight) ? 1 : 0;

    int maxSplitLevelHeight = (int)(logf((float)m_numCuInHeight) / logf(2.0));
    int maxSplitLevelWidth  = (int)(logf((float)m_numCuInWidth) / logf(2.0));

    m_maxSplitLevel = (maxSplitLevelHeight < maxSplitLevelWidth) ? (maxSplitLevelHeight) : (maxSplitLevelWidth);
    m_maxSplitLevel = (m_maxSplitLevel < m_maxDepth) ? (m_maxSplitLevel) : (m_maxDepth);

    /* various structures are overloaded to store per component data.
     * m_iNumTotalParts must allow for sufficient storage in any allocated arrays */
    m_numTotalParts  = X265_MAX(3, m_numCulPartsLevel[m_maxSplitLevel]);

    uint32_t pixelRangeY = 1 << X265_DEPTH;
    uint32_t boRangeShiftY = X265_DEPTH - SAO_BO_BITS;

    m_lumaTableBo = X265_MALLOC(pixel, pixelRangeY);
    for (int k2 = 0; k2 < pixelRangeY; k2++)
    {
        m_lumaTableBo[k2] = 1 + (k2 >> boRangeShiftY);
    }

    uint32_t pixelRangeC = 1 << X265_DEPTH;
    uint32_t boRangeShiftC = X265_DEPTH - SAO_BO_BITS;

    m_chromaTableBo = X265_MALLOC(pixel, pixelRangeC);
    for (int k2 = 0; k2 < pixelRangeC; k2++)
    {
        m_chromaTableBo[k2] = 1 + (k2 >> boRangeShiftC);
    }

    int16_t i;

    uint32_t maxY  = (1 << X265_DEPTH) - 1;
    uint32_t minY  = 0;

    int rangeExt = maxY >> 1;

    m_clipTableBase = X265_MALLOC(pixel, maxY + 2 * rangeExt);
    m_offsetBo      = X265_MALLOC(int, maxY + 2 * rangeExt);

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

    uint32_t maxC = (1 << X265_DEPTH) - 1;
    uint32_t minC = 0;

    int rangeExtC = maxC >> 1;

    m_chromaClipTableBase = X265_MALLOC(pixel, maxC + 2 * rangeExtC);
    m_chromaOffsetBo      = X265_MALLOC(int, maxC + 2 * rangeExtC);

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

    m_tmpL1 = X265_MALLOC(pixel, m_maxCUHeight + 1);
    m_tmpL2 = X265_MALLOC(pixel, m_maxCUHeight + 1);

    m_tmpU1[0] = X265_MALLOC(pixel, m_picWidth);
    m_tmpU1[1] = X265_MALLOC(pixel, m_picWidth);
    m_tmpU1[2] = X265_MALLOC(pixel, m_picWidth);

    m_tmpU2[0] = X265_MALLOC(pixel, m_picWidth);
    m_tmpU2[1] = X265_MALLOC(pixel, m_picWidth);
    m_tmpU2[2] = X265_MALLOC(pixel, m_picWidth);
}

/** destroy SampleAdaptiveOffset memory.
 * \param
 */
void TComSampleAdaptiveOffset::destroy()
{
    X265_FREE(m_clipTableBase);
    m_clipTableBase = NULL;
    X265_FREE(m_offsetBo);
    m_offsetBo = NULL;
    X265_FREE(m_lumaTableBo);
    m_lumaTableBo = NULL;
    X265_FREE(m_chromaClipTableBase);
    m_chromaClipTableBase = NULL;
    X265_FREE(m_chromaOffsetBo);
    m_chromaOffsetBo = NULL;
    X265_FREE(m_chromaTableBo);
    m_chromaTableBo = NULL;

    X265_FREE(m_tmpL1);
    m_tmpL1 = NULL;
    X265_FREE(m_tmpL2);
    m_tmpL2 = NULL;
    X265_FREE(m_tmpU1[0]);

    m_tmpU1[0] = NULL;
    X265_FREE(m_tmpU1[1]);
    m_tmpU1[1] = NULL;
    X265_FREE(m_tmpU1[2]);
    m_tmpU1[2] = NULL;

    X265_FREE(m_tmpU2[0]);
    m_tmpU2[0] = NULL;
    X265_FREE(m_tmpU2[1]);
    m_tmpU2[1] = NULL;
    X265_FREE(m_tmpU2[2]);
    m_tmpU2[2] = NULL;
}

/** allocate memory for SAO parameters
 * \param    *saoParam
 */
void TComSampleAdaptiveOffset::allocSaoParam(SAOParam *saoParam) const
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
void TComSampleAdaptiveOffset::initSAOParam(SAOParam *saoParam, int partLevel, int partRow, int partCol, int parentPartIdx, int startCUX, int endCUX, int startCUY, int endCUY, int yCbCr) const
{
    int j;
    int partIdx = convertLevelRowCol2Idx(partLevel, partRow, partCol);

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
        int downLevel    = (partLevel + 1);
        int downRowStart = (partRow << 1);
        int downColStart = (partCol << 1);

        int downRowIdx, downColIdx;
        int numCUWidth,  numCUHeight;
        int numCULeft;
        int numCUTop;

        int downStartCUX, downStartCUY;
        int downEndCUX, downEndCUY;

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
void TComSampleAdaptiveOffset::freeSaoParam(SAOParam *saoParam)
{
    delete [] saoParam->saoPart[0];
    delete [] saoParam->saoPart[1];
    delete [] saoParam->saoPart[2];
    saoParam->saoPart[0] = 0;
    saoParam->saoPart[1] = 0;
    saoParam->saoPart[2] = 0;
    delete [] saoParam->saoLcuParam[0];
    saoParam->saoLcuParam[0] = NULL;
    delete [] saoParam->saoLcuParam[1];
    saoParam->saoLcuParam[1] = NULL;
    delete [] saoParam->saoLcuParam[2];
    saoParam->saoLcuParam[2] = NULL;
}

/** reset SAO parameters
 * \param   saoParam
 */
void TComSampleAdaptiveOffset::resetSAOParam(SAOParam *saoParam)
{
    int numComponet = 3;

    for (int c = 0; c < numComponet; c++)
    {
        if (c < 2)
        {
            saoParam->bSaoFlag[c] = 0;
        }
        for (int i = 0; i < m_numCulPartsLevel[m_maxSplitLevel]; i++)
        {
            saoParam->saoPart[c][i].bestType     = -1;
            saoParam->saoPart[c][i].length       =  0;
            saoParam->saoPart[c][i].bSplit       = false;
            saoParam->saoPart[c][i].bProcessed   = false;
            saoParam->saoPart[c][i].minCost      = MAX_DOUBLE;
            saoParam->saoPart[c][i].minDist      = MAX_INT;
            saoParam->saoPart[c][i].minRate      = MAX_INT;
            saoParam->saoPart[c][i].subTypeIdx   = 0;
            for (int j = 0; j < MAX_NUM_SAO_OFFSETS; j++)
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
inline int xSign(int x)
{
    return (x >> 31) | ((int)((((uint32_t)-x)) >> 31));
}

/** initialize variables for SAO process
 * \param  pic picture data pointer
 */
void TComSampleAdaptiveOffset::createPicSaoInfo(Frame* pic)
{
    m_pic = pic;
}

/** sample adaptive offset process for one LCU crossing LCU boundary
 * \param   addr, iSaoType, yCbCr
 */
void TComSampleAdaptiveOffset::processSaoCu(int addr, int saoType, int yCbCr)
{
    int x, y;
    TComDataCU *tmpCu = m_pic->getCU(addr);
    pixel* rec;
    int  stride;
    int  lcuWidth  = m_maxCUWidth;
    int  lcuHeight = m_maxCUHeight;
    uint32_t lpelx = tmpCu->getCUPelX();
    uint32_t tpely = tmpCu->getCUPelY();
    uint32_t rpelx;
    uint32_t bpely;
    int  edgeType;
    int  signDown;
    int  signDown1;
    int  signDown2;
    int picWidthTmp;
    int picHeightTmp;
    int startX;
    int startY;
    int endX;
    int endY;
    int isChroma = (yCbCr != 0) ? 1 : 0;
    int shift;
    int cuHeightTmp;
    pixel* tmpL;
    pixel* tmpU;
    pixel* clipTbl = NULL;
    int32_t *offsetBo = NULL;

    picWidthTmp  = (isChroma == 0) ? m_picWidth  : m_picWidth  >> m_hChromaShift;
    picHeightTmp = (isChroma == 0) ? m_picHeight : m_picHeight >> m_vChromaShift;
    lcuWidth     = (isChroma == 0) ? lcuWidth    : lcuWidth    >> m_hChromaShift;
    lcuHeight    = (isChroma == 0) ? lcuHeight   : lcuHeight   >> m_vChromaShift;
    lpelx        = (isChroma == 0) ? lpelx       : lpelx       >> m_hChromaShift;
    tpely        = (isChroma == 0) ? tpely       : tpely       >> m_vChromaShift;

    rpelx        = lpelx + lcuWidth;
    bpely        = tpely + lcuHeight;
    rpelx        = rpelx > picWidthTmp  ? picWidthTmp  : rpelx;
    bpely        = bpely > picHeightTmp ? picHeightTmp : bpely;
    lcuWidth     = rpelx - lpelx;
    lcuHeight    = bpely - tpely;

    if (tmpCu->m_pic == 0)
    {
        return;
    }
    if (yCbCr == 0)
    {
        rec    = m_pic->getPicYuvRec()->getLumaAddr(addr);
        stride = m_pic->getStride();
    }
    else
    {
        rec    = m_pic->getPicYuvRec()->getChromaAddr(yCbCr, addr);
        stride = m_pic->getCStride();
    }

    int32_t _upBuff1[MAX_CU_SIZE + 2], *upBuff1 = _upBuff1 + 1;
    int32_t _upBufft[MAX_CU_SIZE + 2], *upBufft = _upBufft + 1;

//   if (iSaoType!=SAO_BO_0 || iSaoType!=SAO_BO_1)
    {
        cuHeightTmp  = (isChroma == 0) ? m_maxCUHeight  : (m_maxCUHeight  >> m_vChromaShift);
        shift = (isChroma == 0) ? (m_maxCUWidth - 1) : ((m_maxCUWidth >> m_hChromaShift) - 1);
        for (int i = 0; i < cuHeightTmp + 1; i++)
        {
            m_tmpL2[i] = rec[shift];
            rec += stride;
        }

        rec -= (stride * (cuHeightTmp + 1));

        tmpL = m_tmpL1;
        tmpU = &(m_tmpU1[yCbCr][lpelx]);
    }

    clipTbl = (yCbCr == 0) ? m_clipTable : m_chromaClipTable;
    offsetBo = (yCbCr == 0) ? m_offsetBo : m_chromaOffsetBo;

    switch (saoType)
    {
    case SAO_EO_0: // dir: -
    {
        pixel firstPxl = 0, lastPxl = 0;
        startX = (lpelx == 0) ? 1 : 0;
        endX   = (rpelx == picWidthTmp) ? lcuWidth - 1 : lcuWidth;
        if (lcuWidth % 16)
        {
            int8_t signRight;
            for (y = 0; y < lcuHeight; y++)
            {
                int8_t signLeft = xSign(rec[startX] - tmpL[y]);
                for (x = startX; x < endX; x++)
                {
                    signRight = xSign(rec[x] - rec[x + 1]);
                    edgeType = signRight + signLeft + 2;
                    signLeft  = -signRight;

                    rec[x] =  Clip3(0, (1 << X265_DEPTH) - 1, rec[x] + m_offsetEo[edgeType]);
                }

                rec += stride;
            }
        }
        else
        {
            for (y = 0; y < lcuHeight; y++)
            {
                int8_t signLeft = xSign(rec[startX] - tmpL[y]);

                if (lpelx == 0)
                {
                    firstPxl = rec[0];
                }

                if (rpelx == picWidthTmp)
                {
                    lastPxl = rec[lcuWidth - 1];
                }

                primitives.saoCuOrgE0(rec, m_offsetEo, lcuWidth, signLeft);

                if (lpelx == 0)
                {
                    rec[0] = firstPxl;
                }

                if (rpelx == picWidthTmp)
                {
                    rec[lcuWidth - 1] = lastPxl;
                }
                rec += stride;
            }
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
            upBuff1[x] = xSign(rec[x] - tmpU[x]);
        }

        for (y = startY; y < endY; y++)
        {
            for (x = 0; x < lcuWidth; x++)
            {
                signDown  = xSign(rec[x] - rec[x + stride]);
                edgeType = signDown + upBuff1[x] + 2;
                upBuff1[x] = -signDown;

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
            upBuff1[x] = xSign(rec[x] - tmpU[x - 1]);
        }

        for (y = startY; y < endY; y++)
        {
            signDown2 = xSign(rec[stride + startX] - tmpL[y]);
            for (x = startX; x < endX; x++)
            {
                signDown1      =  xSign(rec[x] - rec[x + stride + 1]);
                edgeType      =  signDown1 + upBuff1[x] + 2;
                upBufft[x + 1] = -signDown1;
                rec[x] = clipTbl[rec[x] + m_offsetEo[edgeType]];
            }

            upBufft[startX] = signDown2;

            std::swap(upBuff1, upBufft);

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
            upBuff1[x] = xSign(rec[x] - tmpU[x + 1]);
        }

        for (y = startY; y < endY; y++)
        {
            x = startX;
            signDown1      =  xSign(rec[x] - tmpL[y + 1]);
            edgeType      =  signDown1 + upBuff1[x] + 2;
            upBuff1[x - 1] = -signDown1;
            rec[x] = clipTbl[rec[x] + m_offsetEo[edgeType]];
            for (x = startX + 1; x < endX; x++)
            {
                signDown1      =  xSign(rec[x] - rec[x + stride - 1]);
                edgeType      =  signDown1 + upBuff1[x] + 2;
                upBuff1[x - 1] = -signDown1;
                rec[x] = clipTbl[rec[x] + m_offsetEo[edgeType]];
            }

            upBuff1[endX - 1] = xSign(rec[endX - 1 + stride] - rec[endX]);

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
        std::swap(m_tmpL1, m_tmpL2);
    }
}

/** Sample adaptive offset process
 * \param pic, saoParam
 */
void TComSampleAdaptiveOffset::SAOProcess(SAOParam* saoParam)
{
    {
        m_saoBitIncreaseY = X265_MAX(X265_DEPTH - 10, 0);
        m_saoBitIncreaseC = X265_MAX(X265_DEPTH - 10, 0);

        if (m_saoLcuBasedOptimization)
        {
            saoParam->oneUnitFlag[0] = 0;
            saoParam->oneUnitFlag[1] = 0;
            saoParam->oneUnitFlag[2] = 0;
        }
        int iY  = 0;
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

pixel* TComSampleAdaptiveOffset::getPicYuvAddr(TComPicYuv* picYuv, int yCbCr, int addr)
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
void TComSampleAdaptiveOffset::processSaoUnitAll(SaoLcuParam* saoLcuParam, bool oneUnitFlag, int yCbCr)
{
    pixel *rec;
    int picWidthTmp;

    if (yCbCr == 0)
    {
        rec         = m_pic->getPicYuvRec()->getLumaAddr();
        picWidthTmp = m_picWidth;
    }
    else
    {
        rec         = m_pic->getPicYuvRec()->getChromaAddr(yCbCr);
        picWidthTmp = m_picWidth >> m_hChromaShift;
    }

    memcpy(m_tmpU1[yCbCr], rec, sizeof(pixel) * picWidthTmp);

    int  i;
    uint32_t edgeType;
    pixel* lumaTable = NULL;
    pixel* clipTable = NULL;
    int32_t* offsetBo = NULL;
    int  typeIdx;

    int offset[LUMA_GROUP_NUM + 1];
    int idxX;
    int idxY;
    int addr;
    int frameWidthInCU = m_pic->getFrameWidthInCU();
    int frameHeightInCU = m_pic->getFrameHeightInCU();
    int stride;
    int sChroma = (yCbCr == 0) ? 0 : 1;
    bool mergeLeftFlag;
    int saoBitIncrease = (yCbCr == 0) ? m_saoBitIncreaseY : m_saoBitIncreaseC;

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
        else
        {
            rec  = m_pic->getPicYuvRec()->getChromaAddr(yCbCr, addr);
            stride = m_pic->getCStride();
            picWidthTmp = m_picWidth >> m_hChromaShift;
        }
        uint32_t cuHeightTmp  = (sChroma == 0) ? m_maxCUHeight  : (m_maxCUHeight  >> m_vChromaShift);
        for (i = 0; i < cuHeightTmp + 1; i++)
        {
            m_tmpL1[i] = rec[0];
            rec += stride;
        }

        rec -= (stride << 1);

        memcpy(m_tmpU2[yCbCr], rec, sizeof(pixel) * picWidthTmp);

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
                    else
                    {
                        rec  = m_pic->getPicYuvRec()->getChromaAddr(yCbCr, addr);
                        stride = m_pic->getCStride();
                    }

                    int widthShift = (sChroma == 0) ? m_maxCUWidth : (m_maxCUWidth >> m_hChromaShift);
                    for (i = 0; i < cuHeightTmp + 1; i++)
                    {
                        m_tmpL1[i] = rec[widthShift - 1];
                        rec += stride;
                    }
                }
            }
        }

        std::swap(m_tmpU1[yCbCr], m_tmpU2[yCbCr]);
    }
}

/** Process SAO all units
 * \param saoLcuParam SAO LCU parameters
 * \param oneUnitFlag one unit flag
 * \param yCbCr color componet index
 */
void TComSampleAdaptiveOffset::processSaoUnitRow(SaoLcuParam* saoLcuParam, int idxY, int yCbCr)
{
    pixel *rec;
    int picWidthTmp;

    if (yCbCr == 0)
    {
        rec        = m_pic->getPicYuvRec()->getLumaAddr();
        picWidthTmp = m_picWidth;
    }
    else
    {
        rec        = m_pic->getPicYuvRec()->getChromaAddr(yCbCr);
        picWidthTmp = m_picWidth >> m_hChromaShift;
    }

    if (idxY == 0)
        memcpy(m_tmpU1[yCbCr], rec, sizeof(pixel) * picWidthTmp);

    int  i;
    uint32_t edgeType;
    pixel* lumaTable = NULL;
    pixel* clipTable = NULL;
    int32_t* offsetBo = NULL;
    int  typeIdx;

    int offset[LUMA_GROUP_NUM + 1];
    int idxX;
    int addr;
    int frameWidthInCU = m_pic->getFrameWidthInCU();
    int stride;
    int sChroma = (yCbCr == 0) ? 0 : 1;
    bool mergeLeftFlag;
    int saoBitIncrease = (yCbCr == 0) ? m_saoBitIncreaseY : m_saoBitIncreaseC;

    offsetBo = (yCbCr == 0) ? m_offsetBo : m_chromaOffsetBo;

    offset[0] = 0;
    {
        addr = idxY * frameWidthInCU;
        if (yCbCr == 0)
        {
            rec  = m_pic->getPicYuvRec()->getLumaAddr(addr);
            stride = m_pic->getStride();
            picWidthTmp = m_picWidth;
        }
        else
        {
            rec  = m_pic->getPicYuvRec()->getChromaAddr(yCbCr, addr);
            stride = m_pic->getCStride();
            picWidthTmp = m_picWidth >> m_hChromaShift;
        }
        uint32_t maxCUHeight  = (sChroma == 0) ? m_maxCUHeight : (m_maxCUHeight >> m_vChromaShift);
        for (i = 0; i < maxCUHeight + 1; i++)
        {
            m_tmpL1[i] = rec[0];
            rec += stride;
        }

        rec -= (stride << 1);

        memcpy(m_tmpU2[yCbCr], rec, sizeof(pixel) * picWidthTmp);

        for (idxX = 0; idxX < frameWidthInCU; idxX++)
        {
            addr = idxY * frameWidthInCU + idxX;

            typeIdx = saoLcuParam[addr].typeIdx;
            mergeLeftFlag = saoLcuParam[addr].mergeLeftFlag;

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
                    else
                    {
                        rec  = m_pic->getPicYuvRec()->getChromaAddr(yCbCr, addr);
                        stride = m_pic->getCStride();
                    }

                    int widthShift = (sChroma == 0) ? m_maxCUWidth : (m_maxCUWidth >> m_hChromaShift);
                    for (i = 0; i < maxCUHeight + 1; i++)
                    {
                        m_tmpL1[i] = rec[widthShift - 1];
                        rec += stride;
                    }
                }
            }
        }

        std::swap(m_tmpU1[yCbCr], m_tmpU2[yCbCr]);
    }
}

void TComSampleAdaptiveOffset::resetLcuPart(SaoLcuParam* saoLcuParam)
{
    int i, j;

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
void TComSampleAdaptiveOffset::convertQT2SaoUnit(SAOParam *saoParam, uint32_t partIdx, int yCbCr)
{
    SAOQTPart* saoPart = &(saoParam->saoPart[yCbCr][partIdx]);

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
void TComSampleAdaptiveOffset::convertOnePart2SaoUnit(SAOParam *saoParam, uint32_t partIdx, int yCbCr)
{
    int j;
    int idxX;
    int idxY;
    int addr;
    int frameWidthInCU = m_pic->getFrameWidthInCU();
    SAOQTPart* saoQTPart = saoParam->saoPart[yCbCr];
    SaoLcuParam* saoLcuParam = saoParam->saoLcuParam[yCbCr];

    for (idxY = saoQTPart[partIdx].startCUY; idxY <= saoQTPart[partIdx].endCUY; idxY++)
    {
        for (idxX = saoQTPart[partIdx].startCUX; idxX <= saoQTPart[partIdx].endCUX; idxX++)
        {
            addr = idxY * frameWidthInCU + idxX;
            saoLcuParam[addr].partIdxTmp = (int)partIdx;
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

void TComSampleAdaptiveOffset::resetSaoUnit(SaoLcuParam* saoUnit)
{
    saoUnit->partIdx       = 0;
    saoUnit->partIdxTmp    = 0;
    saoUnit->mergeLeftFlag = 0;
    saoUnit->mergeUpFlag   = 0;
    saoUnit->typeIdx       = -1;
    saoUnit->length        = 0;
    saoUnit->subTypeIdx    = 0;

    for (int i = 0; i < 4; i++)
    {
        saoUnit->offset[i] = 0;
    }
}

void TComSampleAdaptiveOffset::copySaoUnit(SaoLcuParam* saoUnitDst, SaoLcuParam* saoUnitSrc)
{
    saoUnitDst->mergeLeftFlag = saoUnitSrc->mergeLeftFlag;
    saoUnitDst->mergeUpFlag   = saoUnitSrc->mergeUpFlag;
    saoUnitDst->typeIdx       = saoUnitSrc->typeIdx;
    saoUnitDst->length        = saoUnitSrc->length;

    saoUnitDst->subTypeIdx  = saoUnitSrc->subTypeIdx;
    for (int i = 0; i < 4; i++)
    {
        saoUnitDst->offset[i] = saoUnitSrc->offset[i];
    }
}

static void restoreOrigLosslessYuv(TComDataCU* cu, uint32_t absZOrderIdx, uint32_t depth);

/** Original Lossless YUV LF disable process.
 * \param pic picture (TComPic) pointer
 * \returns void
 *
 * \note Replace filtered sample values of Lossless mode blocks with the transmitted and reconstructed ones.
 */
void restoreLFDisabledOrigYuv(Frame* pic)
{
    if (pic->m_picSym->m_slice->m_pps->bTransquantBypassEnabled)
    {
        for (uint32_t cuAddr = 0; cuAddr < pic->getNumCUsInFrame(); cuAddr++)
        {
            TComDataCU* cu = pic->getCU(cuAddr);

            xOrigCUSampleRestoration(cu, 0, 0);
        }
    }
}

/** Original YUV restoration for CU in lossless coding.
 * \param cu pointer to current CU
 * \param absPartIdx part index
 * \param depth CU depth
 * \returns void
 */
void xOrigCUSampleRestoration(TComDataCU* cu, uint32_t absZOrderIdx, uint32_t depth)
{
    Frame* pic = cu->m_pic;
    uint32_t curNumParts = pic->getNumPartInCU() >> (depth << 1);
    uint32_t qNumParts   = curNumParts >> 2;

    // go to sub-CU
    if (cu->getDepth(absZOrderIdx) > depth)
    {
        for (uint32_t partIdx = 0; partIdx < 4; partIdx++, absZOrderIdx += qNumParts)
        {
            uint32_t lpelx = cu->getCUPelX() + g_rasterToPelX[g_zscanToRaster[absZOrderIdx]];
            uint32_t tpely = cu->getCUPelY() + g_rasterToPelY[g_zscanToRaster[absZOrderIdx]];
            if ((lpelx < cu->m_slice->m_sps->picWidthInLumaSamples) && (tpely < cu->m_slice->m_sps->picHeightInLumaSamples))
                xOrigCUSampleRestoration(cu, absZOrderIdx, depth + 1);
        }

        return;
    }

    // restore original YUV samples
    if (cu->isLosslessCoded(absZOrderIdx))
        restoreOrigLosslessYuv(cu, absZOrderIdx, depth);
}

/** Original Lossless YUV  sample restoration.
 * \param cu pointer to current CU
 * \param absPartIdx part index
 * \param depth CU depth
 * \param ttText texture component type
 * \returns void
 */
static void restoreOrigLosslessYuv(TComDataCU* cu, uint32_t absZOrderIdx, uint32_t depth)
{
    TComPicYuv* pcPicYuvRec = cu->m_pic->getPicYuvRec();
    int hChromaShift = cu->getHorzChromaShift();
    int vChromaShift = cu->getVertChromaShift();
    uint32_t lumaOffset   = absZOrderIdx << cu->m_pic->getLog2UnitSize() * 2;
    uint32_t chromaOffset = lumaOffset >> (hChromaShift + vChromaShift);

    pixel* dst = pcPicYuvRec->getLumaAddr(cu->getAddr(), absZOrderIdx);
    pixel* src = cu->getLumaOrigYuv() + lumaOffset;
    uint32_t stride = pcPicYuvRec->getStride();
    uint32_t width  = (g_maxCUSize >> depth);
    uint32_t height = (g_maxCUSize >> depth);

    //TODO Optimized Primitives
    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            dst[x] = src[x];
        }

        src += width;
        dst += stride;
    }

    pixel* dstCb = pcPicYuvRec->getChromaAddr(1, cu->getAddr(), absZOrderIdx);
    pixel* srcCb = cu->getChromaOrigYuv(1) + chromaOffset;

    pixel* dstCr = pcPicYuvRec->getChromaAddr(2, cu->getAddr(), absZOrderIdx);
    pixel* srcCr = cu->getChromaOrigYuv(2) + chromaOffset;

    stride = pcPicYuvRec->getCStride();
    width  = ((g_maxCUSize >> depth) >> hChromaShift);
    height = ((g_maxCUSize >> depth) >> vChromaShift);

    //TODO Optimized Primitives
    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            dstCb[x] = srcCb[x];
            dstCr[x] = srcCr[x];
        }

        srcCb += width;
        dstCb += stride;
        srcCr += width;
        dstCr += stride;
    }
}

}
//! \}
