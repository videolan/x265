/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Min Chen <chenm003@163.com>
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
#include "sao.h"

namespace {

#if HIGH_BIT_DEPTH
inline double roundIDBI2(double x)
{
    return ((x) > 0) ? (int)(((int)(x) + (1 << (X265_DEPTH - 8 - 1))) / (1 << (X265_DEPTH - 8))) :
                       ((int)(((int)(x) - (1 << (X265_DEPTH - 8 - 1))) / (1 << (X265_DEPTH - 8))));
}
#endif

/* rounding with IBDI */
inline double roundIDBI(double x)
{
#if HIGH_BIT_DEPTH
    return X265_DEPTH > 8 ? roundIDBI2(x) : ((x) >= 0 ? ((int)((x) + 0.5)) : ((int)((x) - 0.5)));
#else
    return (x) >= 0 ? ((int)((x) + 0.5)) : ((int)((x) - 0.5));
#endif
}

/* get the sign of input variable (TODO: this is a dup, make common) */
inline int signOf(int x)
{
    return (x >> 31) | ((int)((((uint32_t)-x)) >> 31));
}

int convertLevelRowCol2Idx(int level, int row, int col)
{
    if (!level)
        return 0;
    else if (level == 1)
        return 1 + row * 2 + col;
    else if (level == 2)
        return 5 + row * 4 + col;
    else if (level == 3)
        return 21 + row * 8 + col;
    else // (level == 4)
        return 85 + row * 16 + col;
}

} // end anonymous namespace


namespace x265 {

const int SAO::s_numCulPartsLevel[5] =
{
    1,   // level 0
    5,   // level 1
    21,  // level 2
    85,  // level 3
    341, // level 4
};

const uint32_t SAO::s_eoTable[9] =
{
    1, // 0
    2, // 1
    0, // 2
    3, // 3
    4, // 4
    0, // 5
    0, // 6
    0, // 7
    0
};

const int SAO::s_numClass[MAX_NUM_SAO_TYPE] =
{
    SAO_EO_LEN,
    SAO_EO_LEN,
    SAO_EO_LEN,
    SAO_EO_LEN,
    SAO_BO_LEN
};

SAO::SAO()
{
    m_count = NULL;
    m_offset = NULL;
    m_offsetOrg = NULL;
    m_countPreDblk = NULL;
    m_offsetOrgPreDblk = NULL;
    m_rate = NULL;
    m_dist = NULL;
    m_cost = NULL;
    m_costPartBest = NULL;
    m_distOrg = NULL;
    m_typePartBest = NULL;
    m_refDepth = 0;
    m_lumaLambda = 0;
    m_chromaLambda = 0;
    m_param = NULL;
    m_numTotalParts = 0;
    m_clipTable = NULL;
    m_clipTableBase = NULL;
    m_offsetBo = NULL;
    m_chromaOffsetBo = NULL;
    m_tableBo = NULL;
    m_tmpU1[0] = NULL;
    m_tmpU1[1] = NULL;
    m_tmpU1[2] = NULL;
    m_tmpU2[0] = NULL;
    m_tmpU2[1] = NULL;
    m_tmpU2[2] = NULL;
    m_tmpL1 = NULL;
    m_tmpL2 = NULL;

    m_depthSaoRate[0][0] = 0;
    m_depthSaoRate[0][1] = 0;
    m_depthSaoRate[0][2] = 0;
    m_depthSaoRate[0][3] = 0;
    m_depthSaoRate[1][0] = 0;
    m_depthSaoRate[1][1] = 0;
    m_depthSaoRate[1][2] = 0;
    m_depthSaoRate[1][3] = 0;
}

bool SAO::create(x265_param *param)
{
    m_param = param;
    m_hChromaShift = CHROMA_H_SHIFT(param->internalCsp);
    m_vChromaShift = CHROMA_V_SHIFT(param->internalCsp);

    m_numCuInWidth =  (m_param->sourceWidth + g_maxCUSize - 1) / g_maxCUSize;
    m_numCuInHeight = (m_param->sourceHeight + g_maxCUSize - 1) / g_maxCUSize;

    int maxSplitLevelHeight = (int)(logf((float)m_numCuInHeight) / logf(2.0));
    int maxSplitLevelWidth  = (int)(logf((float)m_numCuInWidth) / logf(2.0));

    m_maxSplitLevel = maxSplitLevelHeight < maxSplitLevelWidth ? maxSplitLevelHeight : maxSplitLevelWidth;
    m_maxSplitLevel = X265_MIN(m_maxSplitLevel, SAO_MAX_DEPTH);

    /* various structures are overloaded to store per component data.
     * m_numTotalParts must allow for sufficient storage in any allocated arrays */
    m_numTotalParts = X265_MAX(3, s_numCulPartsLevel[m_maxSplitLevel]);

    int pixelRange = 1 << X265_DEPTH;
    int boRangeShift = X265_DEPTH - SAO_BO_BITS;
    pixel maxY = (1 << X265_DEPTH) - 1;
    pixel minY = 0;
    pixel rangeExt = maxY >> 1;
    int numLcu = m_numCuInWidth * m_numCuInHeight;

    CHECKED_MALLOC(m_tableBo, pixel, pixelRange);

    CHECKED_MALLOC(m_clipTableBase, pixel, maxY + 2 * rangeExt);
    CHECKED_MALLOC(m_offsetBo,        int, maxY + 2 * rangeExt);
    CHECKED_MALLOC(m_chromaOffsetBo , int, maxY + 2 * rangeExt);

    CHECKED_MALLOC(m_tmpL1, pixel, g_maxCUSize + 1);
    CHECKED_MALLOC(m_tmpL2, pixel, g_maxCUSize + 1);

    for (int i = 0; i < 3; i++)
    {
        CHECKED_MALLOC(m_tmpU1[i], pixel, m_param->sourceWidth);
        CHECKED_MALLOC(m_tmpU2[i], pixel, m_param->sourceWidth);
    }

    CHECKED_MALLOC(m_distOrg, int64_t, m_numTotalParts);
    CHECKED_MALLOC(m_costPartBest, double, m_numTotalParts);
    CHECKED_MALLOC(m_typePartBest, int, m_numTotalParts);

    CHECKED_MALLOC(m_rate, PerType, m_numTotalParts);
    CHECKED_MALLOC(m_dist, PerType, m_numTotalParts);
    CHECKED_MALLOC(m_cost, PerTypeD, m_numTotalParts);

    CHECKED_MALLOC(m_count, PerClass, m_numTotalParts);
    CHECKED_MALLOC(m_offset, PerClass, m_numTotalParts);
    CHECKED_MALLOC(m_offsetOrg, PerClass, m_numTotalParts);

    CHECKED_MALLOC(m_countPreDblk, PerPlane, numLcu);
    CHECKED_MALLOC(m_offsetOrgPreDblk, PerPlane, numLcu);

    for (int k2 = 0; k2 < pixelRange; k2++)
        m_tableBo[k2] = (pixel)(1 + (k2 >> boRangeShift));

    for (int i = 0; i < (minY + rangeExt); i++)
        m_clipTableBase[i] = minY;

    for (int i = minY + rangeExt; i < (maxY + rangeExt); i++)
        m_clipTableBase[i] = (pixel)(i - rangeExt);

    for (int i = maxY + rangeExt; i < (maxY + 2 * rangeExt); i++)
        m_clipTableBase[i] = maxY;

    m_clipTable = &(m_clipTableBase[rangeExt]);

    return true;

fail:
    return false;
}

void SAO::destroy()
{
    X265_FREE(m_clipTableBase);
    X265_FREE(m_offsetBo);
    X265_FREE(m_tableBo);
    X265_FREE(m_chromaOffsetBo);

    X265_FREE(m_tmpL1);
    X265_FREE(m_tmpL2);

    for (int i = 0; i < 3; i++)
    {
        X265_FREE(m_tmpU1[i]);
        X265_FREE(m_tmpU2[i]);
    }

    X265_FREE(m_distOrg);
    X265_FREE(m_costPartBest);
    X265_FREE(m_typePartBest);
    X265_FREE(m_rate);
    X265_FREE(m_dist);
    X265_FREE(m_cost);
    X265_FREE(m_count);
    X265_FREE(m_offset);
    X265_FREE(m_offsetOrg);
    X265_FREE(m_countPreDblk);
    X265_FREE(m_offsetOrgPreDblk);
}

/* allocate memory for SAO parameters */
void SAO::allocSaoParam(SAOParam *saoParam) const
{
    saoParam->maxSplitLevel = m_maxSplitLevel;
    saoParam->numCuInWidth  = m_numCuInWidth;
    saoParam->numCuInHeight = m_numCuInHeight;

    saoParam->saoPart[0] = new SAOQTPart[s_numCulPartsLevel[saoParam->maxSplitLevel]];
    initSAOParam(saoParam, 0, 0, 0, -1, 0, m_numCuInWidth - 1,  0, m_numCuInHeight - 1, 0);

    saoParam->saoPart[1] = new SAOQTPart[s_numCulPartsLevel[saoParam->maxSplitLevel]];
    saoParam->saoPart[2] = new SAOQTPart[s_numCulPartsLevel[saoParam->maxSplitLevel]];
    initSAOParam(saoParam, 0, 0, 0, -1, 0, m_numCuInWidth - 1,  0, m_numCuInHeight - 1, 1);
    initSAOParam(saoParam, 0, 0, 0, -1, 0, m_numCuInWidth - 1,  0, m_numCuInHeight - 1, 2);

    saoParam->saoLcuParam[0] = new SaoLcuParam[m_numCuInHeight * m_numCuInWidth];
    saoParam->saoLcuParam[1] = new SaoLcuParam[m_numCuInHeight * m_numCuInWidth];
    saoParam->saoLcuParam[2] = new SaoLcuParam[m_numCuInHeight * m_numCuInWidth];
}

/* recursively initialize SAO parameters (only once) */
void SAO::initSAOParam(SAOParam *saoParam, int partLevel, int partRow, int partCol, int parentPartIdx, int startCUX, int endCUX, int startCUY, int endCUY, int plane) const
{
    int j;
    int partIdx = convertLevelRowCol2Idx(partLevel, partRow, partCol);

    SAOQTPart* saoPart;

    saoPart = &(saoParam->saoPart[plane][partIdx]);

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
        saoPart->offset[j] = 0;

    if (saoPart->partLevel < m_maxSplitLevel)
    {
        int downLevel    = (partLevel + 1);
        int downRowStart = (partRow << 1);
        int downColStart = (partCol << 1);

        int numCUWidth  = endCUX - startCUX + 1;
        int numCUHeight = endCUY - startCUY + 1;
        int numCULeft   = (numCUWidth  >> 1);
        int numCUTop    = (numCUHeight >> 1);

        int downStartCUX = startCUX;
        int downEndCUX  = downStartCUX + numCULeft - 1;
        int downStartCUY = startCUY;
        int downEndCUY  = downStartCUY + numCUTop  - 1;
        int downRowIdx = downRowStart + 0;
        int downColIdx = downColStart + 0;

        saoPart->downPartsIdx[0] = convertLevelRowCol2Idx(downLevel, downRowIdx, downColIdx);

        initSAOParam(saoParam, downLevel, downRowIdx, downColIdx, partIdx, downStartCUX, downEndCUX, downStartCUY, downEndCUY, plane);

        downStartCUX = startCUX + numCULeft;
        downEndCUX   = endCUX;
        downStartCUY = startCUY;
        downEndCUY   = downStartCUY + numCUTop - 1;
        downRowIdx  = downRowStart + 0;
        downColIdx  = downColStart + 1;

        saoPart->downPartsIdx[1] = convertLevelRowCol2Idx(downLevel, downRowIdx, downColIdx);

        initSAOParam(saoParam, downLevel, downRowIdx, downColIdx, partIdx,  downStartCUX, downEndCUX, downStartCUY, downEndCUY, plane);

        downStartCUX = startCUX;
        downEndCUX   = downStartCUX + numCULeft - 1;
        downStartCUY = startCUY + numCUTop;
        downEndCUY   = endCUY;
        downRowIdx  = downRowStart + 1;
        downColIdx  = downColStart + 0;

        saoPart->downPartsIdx[2] = convertLevelRowCol2Idx(downLevel, downRowIdx, downColIdx);

        initSAOParam(saoParam, downLevel, downRowIdx, downColIdx, partIdx, downStartCUX, downEndCUX, downStartCUY, downEndCUY, plane);

        downStartCUX = startCUX + numCULeft;
        downEndCUX   = endCUX;
        downStartCUY = startCUY + numCUTop;
        downEndCUY   = endCUY;
        downRowIdx  = downRowStart + 1;
        downColIdx  = downColStart + 1;

        saoPart->downPartsIdx[3] = convertLevelRowCol2Idx(downLevel, downRowIdx, downColIdx);

        initSAOParam(saoParam, downLevel, downRowIdx, downColIdx, partIdx, downStartCUX, downEndCUX, downStartCUY, downEndCUY, plane);
    }
    else
    {
        saoPart->downPartsIdx[0] = saoPart->downPartsIdx[1] = saoPart->downPartsIdx[2] = saoPart->downPartsIdx[3] = -1;
    }
}

/* reset SAO parameters once per frame */
void SAO::resetSAOParam(SAOParam *saoParam)
{
    int numComponet = 3;

    for (int c = 0; c < numComponet; c++)
    {
        if (c < 2)
            saoParam->bSaoFlag[c] = false;

        for (int i = 0; i < s_numCulPartsLevel[m_maxSplitLevel]; i++)
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

/* sample adaptive offset process for one LCU crossing LCU boundary */
void SAO::processSaoCu(int addr, int saoType, int plane)
{
    int x, y;
    TComDataCU *tmpCu = m_pic->getCU(addr);
    pixel* rec;
    int stride;
    int lcuWidth;
    int lcuHeight;
    int rpelx;
    int bpely;
    int edgeType;
    int signDown;
    int signDown1;
    int signDown2;
    int picWidthTmp;
    int picHeightTmp;
    int startX;
    int startY;
    int endX;
    int endY;
    int shift;
    int cuHeightTmp;
    pixel* tmpL;
    pixel* tmpU;
    uint32_t lpelx = tmpCu->getCUPelX();
    uint32_t tpely = tmpCu->getCUPelY();
    bool isLuma = !plane;

    picWidthTmp  = isLuma ? m_param->sourceWidth  : m_param->sourceWidth  >> m_hChromaShift;
    picHeightTmp = isLuma ? m_param->sourceHeight : m_param->sourceHeight >> m_vChromaShift;
    lcuWidth     = isLuma ? g_maxCUSize : g_maxCUSize >> m_hChromaShift;
    lcuHeight    = isLuma ? g_maxCUSize : g_maxCUSize >> m_vChromaShift;
    lpelx        = isLuma ? lpelx       : lpelx       >> m_hChromaShift;
    tpely        = isLuma ? tpely       : tpely       >> m_vChromaShift;

    rpelx        = lpelx + lcuWidth;
    bpely        = tpely + lcuHeight;
    rpelx        = rpelx > picWidthTmp  ? picWidthTmp  : rpelx;
    bpely        = bpely > picHeightTmp ? picHeightTmp : bpely;
    lcuWidth     = rpelx - lpelx;
    lcuHeight    = bpely - tpely;

    if (!tmpCu->m_pic)
        return;

    if (plane)
    {
        rec    = m_pic->getPicYuvRec()->getChromaAddr(plane, addr);
        stride = m_pic->getCStride();
    }
    else
    {
        rec    = m_pic->getPicYuvRec()->getLumaAddr(addr);
        stride = m_pic->getStride();
    }

    int32_t _upBuff1[MAX_CU_SIZE + 2], *upBuff1 = _upBuff1 + 1;
    int32_t _upBufft[MAX_CU_SIZE + 2], *upBufft = _upBufft + 1;

//   if (iSaoType!=SAO_BO_0 || iSaoType!=SAO_BO_1)
    {
        cuHeightTmp = isLuma ? g_maxCUSize : (g_maxCUSize  >> m_vChromaShift);
        shift = isLuma ? (g_maxCUSize - 1) : ((g_maxCUSize >> m_hChromaShift) - 1);
        for (int i = 0; i < cuHeightTmp + 1; i++)
        {
            m_tmpL2[i] = rec[shift];
            rec += stride;
        }

        rec -= (stride * (cuHeightTmp + 1));

        tmpL = m_tmpL1;
        tmpU = &(m_tmpU1[plane][lpelx]);
    }

    int32_t *offsetBo = isLuma ? m_offsetBo : m_chromaOffsetBo;

    switch (saoType)
    {
    case SAO_EO_0: // dir: -
    {
        pixel firstPxl = 0, lastPxl = 0;
        startX = !lpelx;
        endX   = (rpelx == picWidthTmp) ? lcuWidth - 1 : lcuWidth;
        if (lcuWidth & 15)
        {
            for (y = 0; y < lcuHeight; y++)
            {
                int signLeft = signOf(rec[startX] - tmpL[y]);
                for (x = startX; x < endX; x++)
                {
                    int signRight = signOf(rec[x] - rec[x + 1]);
                    edgeType = signRight + signLeft + 2;
                    signLeft = -signRight;

                    rec[x] = (pixel)Clip3(0, (1 << X265_DEPTH) - 1, rec[x] + m_offsetEo[edgeType]);
                }

                rec += stride;
            }
        }
        else
        {
            for (y = 0; y < lcuHeight; y++)
            {
                int signLeft = signOf(rec[startX] - tmpL[y]);

                if (!lpelx)
                    firstPxl = rec[0];

                if (rpelx == picWidthTmp)
                    lastPxl = rec[lcuWidth - 1];

                primitives.saoCuOrgE0(rec, m_offsetEo, lcuWidth, (int8_t)signLeft);

                if (!lpelx)
                    rec[0] = firstPxl;

                if (rpelx == picWidthTmp)
                    rec[lcuWidth - 1] = lastPxl;

                rec += stride;
            }
        }
        break;
    }
    case SAO_EO_1: // dir: |
    {
        startY = !tpely;
        endY   = (bpely == picHeightTmp) ? lcuHeight - 1 : lcuHeight;
        if (!tpely)
            rec += stride;

        for (x = 0; x < lcuWidth; x++)
            upBuff1[x] = signOf(rec[x] - tmpU[x]);

        for (y = startY; y < endY; y++)
        {
            for (x = 0; x < lcuWidth; x++)
            {
                signDown = signOf(rec[x] - rec[x + stride]);
                edgeType = signDown + upBuff1[x] + 2;
                upBuff1[x] = -signDown;

                rec[x] = m_clipTable[rec[x] + m_offsetEo[edgeType]];
            }

            rec += stride;
        }

        break;
    }
    case SAO_EO_2: // dir: 135
    {
        startX = !lpelx;
        endX   = (rpelx == picWidthTmp) ? lcuWidth - 1 : lcuWidth;

        startY = !tpely;
        endY   = (bpely == picHeightTmp) ? lcuHeight - 1 : lcuHeight;

        if (!tpely)
            rec += stride;

        for (x = startX; x < endX; x++)
            upBuff1[x] = signOf(rec[x] - tmpU[x - 1]);

        for (y = startY; y < endY; y++)
        {
            signDown2 = signOf(rec[stride + startX] - tmpL[y]);
            for (x = startX; x < endX; x++)
            {
                signDown1 = signOf(rec[x] - rec[x + stride + 1]);
                edgeType  = signDown1 + upBuff1[x] + 2;
                upBufft[x + 1] = -signDown1;
                rec[x] = m_clipTable[rec[x] + m_offsetEo[edgeType]];
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
            rec += stride;

        for (x = startX - 1; x < endX; x++)
            upBuff1[x] = signOf(rec[x] - tmpU[x + 1]);

        for (y = startY; y < endY; y++)
        {
            x = startX;
            signDown1 = signOf(rec[x] - tmpL[y + 1]);
            edgeType  = signDown1 + upBuff1[x] + 2;
            upBuff1[x - 1] = -signDown1;
            rec[x] = m_clipTable[rec[x] + m_offsetEo[edgeType]];
            for (x = startX + 1; x < endX; x++)
            {
                signDown1 = signOf(rec[x] - rec[x + stride - 1]);
                edgeType  = signDown1 + upBuff1[x] + 2;
                upBuff1[x - 1] = -signDown1;
                rec[x] = m_clipTable[rec[x] + m_offsetEo[edgeType]];
            }

            upBuff1[endX - 1] = signOf(rec[endX - 1 + stride] - rec[endX]);

            rec += stride;
        }

        break;
    }
    case SAO_BO:
    {
        for (y = 0; y < lcuHeight; y++)
        {
            for (x = 0; x < lcuWidth; x++)
                rec[x] = (pixel)offsetBo[rec[x]];

            rec += stride;
        }

        break;
    }
    default: break;
    }

//   if (iSaoType!=SAO_BO_0 || iSaoType!=SAO_BO_1)
    std::swap(m_tmpL1, m_tmpL2);
}

/* Process SAO all units */
void SAO::processSaoUnitAll(SaoLcuParam* saoLcuParam, bool oneUnitFlag, int plane)
{
    pixel *rec;
    int picWidthTmp;

    if (plane)
    {
        rec         = m_pic->getPicYuvRec()->getChromaAddr(plane);
        picWidthTmp = m_param->sourceWidth >> m_hChromaShift;
    }
    else
    {
        rec         = m_pic->getPicYuvRec()->getLumaAddr();
        picWidthTmp = m_param->sourceWidth;
    }

    memcpy(m_tmpU1[plane], rec, sizeof(pixel) * picWidthTmp);

    int typeIdx;
    uint32_t edgeType;

    int offset[LUMA_GROUP_NUM + 1];
    int idxX;
    int idxY;
    int addr;
    int frameWidthInCU = m_pic->getFrameWidthInCU();
    int frameHeightInCU = m_pic->getFrameHeightInCU();
    int stride;
    bool isChroma = !!plane;
    bool mergeLeftFlag;

    int32_t *offsetBo = isChroma ? m_chromaOffsetBo : m_offsetBo;

    offset[0] = 0;
    for (idxY = 0; idxY < frameHeightInCU; idxY++)
    {
        addr = idxY * frameWidthInCU;
        if (plane == 0)
        {
            rec  = m_pic->getPicYuvRec()->getLumaAddr(addr);
            stride = m_pic->getStride();
            picWidthTmp = m_param->sourceWidth;
        }
        else
        {
            rec  = m_pic->getPicYuvRec()->getChromaAddr(plane, addr);
            stride = m_pic->getCStride();
            picWidthTmp = m_param->sourceWidth >> m_hChromaShift;
        }
        uint32_t cuHeightTmp = isChroma ? (g_maxCUSize >> m_vChromaShift) : g_maxCUSize;
        for (uint32_t i = 0; i < cuHeightTmp + 1; i++)
        {
            m_tmpL1[i] = rec[0];
            rec += stride;
        }

        rec -= (stride << 1);

        memcpy(m_tmpU2[plane], rec, sizeof(pixel) * picWidthTmp);

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
                        for (int i = 0; i < SAO_MAX_BO_CLASSES + 1; i++)
                            offset[i] = 0;

                        for (int i = 0; i < saoLcuParam[addr].length; i++)
                            offset[(saoLcuParam[addr].subTypeIdx + i) % SAO_MAX_BO_CLASSES  + 1] = saoLcuParam[addr].offset[i] << SAO_BIT_INC;

                        for (int i = 0; i < (1 << X265_DEPTH); i++)
                            offsetBo[i] = m_clipTable[i + offset[m_tableBo[i]]];
                    }
                    if (typeIdx == SAO_EO_0 || typeIdx == SAO_EO_1 || typeIdx == SAO_EO_2 || typeIdx == SAO_EO_3)
                    {
                        for (int i = 0; i < saoLcuParam[addr].length; i++)
                            offset[i + 1] = saoLcuParam[addr].offset[i] << SAO_BIT_INC;

                        for (edgeType = 0; edgeType < 6; edgeType++)
                            m_offsetEo[edgeType] = (int8_t)offset[s_eoTable[edgeType]];
                    }
                }
                processSaoCu(addr, typeIdx, plane);
            }
            else
            {
                if (idxX != (frameWidthInCU - 1))
                {
                    if (isChroma)
                    {
                        rec = m_pic->getPicYuvRec()->getChromaAddr(plane, addr);
                        stride = m_pic->getCStride();
                    }
                    else
                    {
                        rec = m_pic->getPicYuvRec()->getLumaAddr(addr);
                        stride = m_pic->getStride();
                    }

                    int widthShift = isChroma ? (g_maxCUSize >> m_hChromaShift) : g_maxCUSize;
                    for (uint32_t i = 0; i < cuHeightTmp + 1; i++)
                    {
                        m_tmpL1[i] = rec[widthShift - 1];
                        rec += stride;
                    }
                }
            }
        }

        std::swap(m_tmpU1[plane], m_tmpU2[plane]);
    }
}

/* Process SAO all units */
void SAO::processSaoUnitRow(SaoLcuParam* saoLcuParam, int idxY, int plane)
{
    pixel *rec;
    int picWidthTmp;

    if (plane)
    {
        rec = m_pic->getPicYuvRec()->getChromaAddr(plane);
        picWidthTmp = m_param->sourceWidth >> m_hChromaShift;
    }
    else
    {
        rec = m_pic->getPicYuvRec()->getLumaAddr();
        picWidthTmp = m_param->sourceWidth;
    }

    if (!idxY)
        memcpy(m_tmpU1[plane], rec, sizeof(pixel) * picWidthTmp);

    int typeIdx;

    int offset[LUMA_GROUP_NUM + 1];
    int idxX;
    int addr;
    int frameWidthInCU = m_pic->getFrameWidthInCU();
    int stride;
    bool isChroma = !!plane;
    bool mergeLeftFlag;

    int32_t* offsetBo = isChroma ? m_chromaOffsetBo : m_offsetBo;

    offset[0] = 0;
    addr = idxY * frameWidthInCU;
    if (isChroma)
    {
        rec = m_pic->getPicYuvRec()->getChromaAddr(plane, addr);
        stride = m_pic->getCStride();
        picWidthTmp = m_param->sourceWidth >> m_hChromaShift;
    }
    else
    {
        rec = m_pic->getPicYuvRec()->getLumaAddr(addr);
        stride = m_pic->getStride();
        picWidthTmp = m_param->sourceWidth;
    }
    int maxCUHeight = isChroma ? (g_maxCUSize >> m_vChromaShift) : g_maxCUSize;
    for (int i = 0; i < maxCUHeight + 1; i++)
    {
        m_tmpL1[i] = rec[0];
        rec += stride;
    }

    rec -= (stride << 1);

    memcpy(m_tmpU2[plane], rec, sizeof(pixel) * picWidthTmp);

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
                    for (int i = 0; i < SAO_MAX_BO_CLASSES + 1; i++)
                        offset[i] = 0;

                    for (int i = 0; i < saoLcuParam[addr].length; i++)
                        offset[(saoLcuParam[addr].subTypeIdx + i) % SAO_MAX_BO_CLASSES  + 1] = saoLcuParam[addr].offset[i] << SAO_BIT_INC;

                    for (int i = 0; i < (1 << X265_DEPTH); i++)
                        offsetBo[i] = m_clipTable[i + offset[m_tableBo[i]]];
                }
                if (typeIdx == SAO_EO_0 || typeIdx == SAO_EO_1 || typeIdx == SAO_EO_2 || typeIdx == SAO_EO_3)
                {
                    for (int i = 0; i < saoLcuParam[addr].length; i++)
                        offset[i + 1] = saoLcuParam[addr].offset[i] << SAO_BIT_INC;

                    for (uint32_t edgeType = 0; edgeType < 6; edgeType++)
                        m_offsetEo[edgeType] = (int8_t)offset[s_eoTable[edgeType]];
                }
            }
            processSaoCu(addr, typeIdx, plane);
        }
        else
        {
            if (idxX != (frameWidthInCU - 1))
            {
                if (isChroma)
                {
                    rec = m_pic->getPicYuvRec()->getChromaAddr(plane, addr);
                    stride = m_pic->getCStride();
                }
                else
                {
                    rec = m_pic->getPicYuvRec()->getLumaAddr(addr);
                    stride = m_pic->getStride();
                }

                int widthShift = isChroma ? (g_maxCUSize >> m_hChromaShift) : g_maxCUSize;
                for (int i = 0; i < maxCUHeight + 1; i++)
                {
                    m_tmpL1[i] = rec[widthShift - 1];
                    rec += stride;
                }
            }
        }
    }

    std::swap(m_tmpU1[plane], m_tmpU2[plane]);
}

void SAO::resetLcuPart(SaoLcuParam* saoLcuParam)
{
    for (int i = 0; i < m_numCuInWidth * m_numCuInHeight; i++)
    {
        saoLcuParam[i].mergeUpFlag   =  1;
        saoLcuParam[i].mergeLeftFlag =  0;
        saoLcuParam[i].partIdx       =  0;
        saoLcuParam[i].typeIdx       = -1;
        saoLcuParam[i].subTypeIdx    =  0;
        for (int j = 0; j < MAX_NUM_SAO_OFFSETS; j++)
            saoLcuParam[i].offset[j] = 0;
    }
}

void SAO::resetSaoUnit(SaoLcuParam* saoUnit)
{
    saoUnit->mergeUpFlag   = 0;
    saoUnit->mergeLeftFlag = 0;
    saoUnit->partIdx       = 0;
    saoUnit->partIdxTmp    = 0;
    saoUnit->typeIdx       = -1;
    saoUnit->length        = 0;
    saoUnit->subTypeIdx    = 0;

    for (int i = 0; i < 4; i++)
        saoUnit->offset[i] = 0;
}

void SAO::copySaoUnit(SaoLcuParam* saoUnitDst, SaoLcuParam* saoUnitSrc)
{
    saoUnitDst->mergeLeftFlag = saoUnitSrc->mergeLeftFlag;
    saoUnitDst->mergeUpFlag   = saoUnitSrc->mergeUpFlag;
    saoUnitDst->typeIdx       = saoUnitSrc->typeIdx;
    saoUnitDst->length        = saoUnitSrc->length;

    saoUnitDst->subTypeIdx  = saoUnitSrc->subTypeIdx;
    for (int i = 0; i < 4; i++)
        saoUnitDst->offset[i] = saoUnitSrc->offset[i];
}

/* convert QP part to SAO unit */
void SAO::convertQT2SaoUnit(SAOParam *saoParam, uint32_t partIdx, int plane)
{
    SAOQTPart* saoPart = &(saoParam->saoPart[plane][partIdx]);

    if (!saoPart->bSplit)
    {
        convertOnePart2SaoUnit(saoParam, partIdx, plane);
        return;
    }

    if (saoPart->partLevel < m_maxSplitLevel)
    {
        convertQT2SaoUnit(saoParam, saoPart->downPartsIdx[0], plane);
        convertQT2SaoUnit(saoParam, saoPart->downPartsIdx[1], plane);
        convertQT2SaoUnit(saoParam, saoPart->downPartsIdx[2], plane);
        convertQT2SaoUnit(saoParam, saoPart->downPartsIdx[3], plane);
    }
}

/* convert one SAO part to SAO unit */
void SAO::convertOnePart2SaoUnit(SAOParam *saoParam, uint32_t partIdx, int plane)
{
    int frameWidthInCU = m_pic->getFrameWidthInCU();
    SAOQTPart* saoQTPart = saoParam->saoPart[plane];
    SaoLcuParam* saoLcuParam = saoParam->saoLcuParam[plane];

    for (int idxY = saoQTPart[partIdx].startCUY; idxY <= saoQTPart[partIdx].endCUY; idxY++)
    {
        for (int idxX = saoQTPart[partIdx].startCUX; idxX <= saoQTPart[partIdx].endCUX; idxX++)
        {
            int addr = idxY * frameWidthInCU + idxX;
            saoLcuParam[addr].partIdxTmp = (int)partIdx;
            saoLcuParam[addr].typeIdx    = saoQTPart[partIdx].bestType;
            saoLcuParam[addr].subTypeIdx = saoQTPart[partIdx].subTypeIdx;
            if (saoLcuParam[addr].typeIdx != -1)
            {
                saoLcuParam[addr].length = saoQTPart[partIdx].length;
                for (int j = 0; j < MAX_NUM_SAO_OFFSETS; j++)
                    saoLcuParam[addr].offset[j] = saoQTPart[partIdx].offset[j];
            }
            else
            {
                saoLcuParam[addr].length = 0;
                saoLcuParam[addr].subTypeIdx = saoQTPart[partIdx].subTypeIdx;
                for (int j = 0; j < MAX_NUM_SAO_OFFSETS; j++)
                    saoLcuParam[addr].offset[j] = 0;
            }
        }
    }
}

/* process SAO for one partition */
void SAO::rdoSaoOnePart(SAOQTPart *psQTPart, int partIdx, int plane)
{
    int typeIdx;
    int numTotalType = MAX_NUM_SAO_TYPE;
    SAOQTPart* onePart = &(psQTPart[partIdx]);

    int64_t estDist;
    int classIdx;

    m_distOrg[partIdx] = 0;

    int    bestClassTableBo = 0;
    int    currentDistortionTableBo[MAX_NUM_SAO_CLASS];
    double currentRdCostTableBo[MAX_NUM_SAO_CLASS];
    double bestRDCostTableBo = MAX_DOUBLE;

    int allowMergeLeft;
    int allowMergeUp;
    SaoLcuParam saoLcuParamRdo;

    for (typeIdx = -1; typeIdx < numTotalType; typeIdx++)
    {
        m_entropyCoder.load(m_rdEntropyCoders[onePart->partLevel][CI_CURR_BEST]);
        m_entropyCoder.resetBits();

        if (typeIdx == -1)
        {
            for (int ry = onePart->startCUY; ry <= onePart->endCUY; ry++)
            {
                for (int rx = onePart->startCUX; rx <= onePart->endCUX; rx++)
                {
                    // get bits for iTypeIdx = -1
                    allowMergeLeft = 1;
                    allowMergeUp   = 1;

                    // reset
                    resetSaoUnit(&saoLcuParamRdo);

                    // set merge flag
                    saoLcuParamRdo.mergeUpFlag   = 1;
                    saoLcuParamRdo.mergeLeftFlag = 1;

                    if (ry == onePart->startCUY)
                        saoLcuParamRdo.mergeUpFlag = 0;

                    if (rx == onePart->startCUX)
                        saoLcuParamRdo.mergeLeftFlag = 0;

                    m_entropyCoder.codeSaoUnitInterleaving(plane, 1, rx, ry,  &saoLcuParamRdo, 1,  1,  allowMergeLeft, allowMergeUp);
                }
            }
        }

        if (typeIdx >= 0)
        {
            estDist = estSaoTypeDist(partIdx, typeIdx, 0, m_lumaLambda, currentDistortionTableBo, currentRdCostTableBo);
            if (typeIdx == SAO_BO)
            {
                // Estimate Best Position
                double currentRDCost = 0.0;

                for (int i = 0; i < SAO_MAX_BO_CLASSES - SAO_BO_LEN + 1; i++)
                {
                    currentRDCost = 0.0;
                    for (int j = i; j < i + SAO_BO_LEN; j++)
                        currentRDCost += currentRdCostTableBo[j];

                    if (currentRDCost < bestRDCostTableBo)
                    {
                        bestRDCostTableBo = currentRDCost;
                        bestClassTableBo  = i;
                    }
                }

                // Recode all offsets
                for (classIdx = bestClassTableBo; classIdx < bestClassTableBo + SAO_BO_LEN; classIdx++)
                    estDist += currentDistortionTableBo[classIdx];
            }

            for (int ry = onePart->startCUY; ry <= onePart->endCUY; ry++)
            {
                for (int rx = onePart->startCUX; rx <= onePart->endCUX; rx++)
                {
                    // get bits for typeIdx = -1
                    allowMergeLeft = 1;
                    allowMergeUp   = 1;

                    // reset
                    resetSaoUnit(&saoLcuParamRdo);

                    // set merge flag
                    saoLcuParamRdo.mergeUpFlag   = 1;
                    saoLcuParamRdo.mergeLeftFlag = 1;

                    if (ry == onePart->startCUY)
                        saoLcuParamRdo.mergeUpFlag = 0;

                    if (rx == onePart->startCUX)
                        saoLcuParamRdo.mergeLeftFlag = 0;

                    // set type and offsets
                    saoLcuParamRdo.typeIdx = typeIdx;
                    saoLcuParamRdo.subTypeIdx = (typeIdx == SAO_BO) ? bestClassTableBo : 0;
                    saoLcuParamRdo.length = s_numClass[typeIdx];
                    for (classIdx = 0; classIdx < saoLcuParamRdo.length; classIdx++)
                        saoLcuParamRdo.offset[classIdx] = (int)m_offset[partIdx][typeIdx][classIdx + saoLcuParamRdo.subTypeIdx + 1];

                    m_entropyCoder.codeSaoUnitInterleaving(plane, 1, rx, ry, &saoLcuParamRdo, 1, 1, allowMergeLeft, allowMergeUp);
                }
            }

            m_dist[partIdx][typeIdx] = estDist;
            m_rate[partIdx][typeIdx] = m_entropyCoder.getNumberOfWrittenBits();

            m_cost[partIdx][typeIdx] = (double)((double)m_dist[partIdx][typeIdx] + m_lumaLambda * (double)m_rate[partIdx][typeIdx]);

            if (m_cost[partIdx][typeIdx] < m_costPartBest[partIdx])
            {
                m_distOrg[partIdx] = 0;
                m_costPartBest[partIdx] = m_cost[partIdx][typeIdx];
                m_typePartBest[partIdx] = typeIdx;
                m_entropyCoder.store(m_rdEntropyCoders[onePart->partLevel][CI_TEMP_BEST]);
            }
        }
        else
        {
            if (m_distOrg[partIdx] < m_costPartBest[partIdx])
            {
                m_costPartBest[partIdx] = (double)m_distOrg[partIdx] + m_entropyCoder.getNumberOfWrittenBits() * m_lumaLambda;
                m_typePartBest[partIdx] = -1;
                m_entropyCoder.store(m_rdEntropyCoders[onePart->partLevel][CI_TEMP_BEST]);
            }
        }
    }

    onePart->bProcessed = true;
    onePart->bSplit    = false;
    onePart->minDist   =       m_typePartBest[partIdx] >= 0 ? m_dist[partIdx][m_typePartBest[partIdx]] : m_distOrg[partIdx];
    onePart->minRate   = (int)(m_typePartBest[partIdx] >= 0 ? m_rate[partIdx][m_typePartBest[partIdx]] : 0);
    onePart->minCost   = onePart->minDist + m_lumaLambda * onePart->minRate;
    onePart->bestType  = m_typePartBest[partIdx];

    if (onePart->bestType != -1)
    {
        onePart->length = s_numClass[onePart->bestType];
        int minIndex = 0;
        if (onePart->bestType == SAO_BO)
        {
            onePart->subTypeIdx = bestClassTableBo;
            minIndex = onePart->subTypeIdx;
        }
        for (int i = 0; i < onePart->length; i++)
            onePart->offset[i] = (int)m_offset[partIdx][onePart->bestType][minIndex + i + 1];
    }
    else
        onePart->length = 0;
}

/* Run partition tree disable */
void SAO::disablePartTree(SAOQTPart *psQTPart, int partIdx)
{
    SAOQTPart* pOnePart = &(psQTPart[partIdx]);

    pOnePart->bSplit   = false;
    pOnePart->length   =  0;
    pOnePart->bestType = -1;

    if (pOnePart->partLevel < (int)m_maxSplitLevel)
    {
        for (int i = 0; i < SAOQTPart::NUM_DOWN_PART; i++)
            disablePartTree(psQTPart, pOnePart->downPartsIdx[i]);
    }
}

/* Run quadtree decision function */
void SAO::runQuadTreeDecision(SAOQTPart *qtPart, int partIdx, double &costFinal, int maxLevel, int plane)
{
    SAOQTPart* onePart = &(qtPart[partIdx]);

    uint32_t nextDepth = onePart->partLevel + 1;

    if (!partIdx)
        costFinal = 0;

    // SAO for this part
    if (!onePart->bProcessed)
        rdoSaoOnePart(qtPart, partIdx, plane);

    // SAO for sub 4 parts
    if (onePart->partLevel < maxLevel)
    {
        double costNotSplit = m_lumaLambda + onePart->minCost;
        double costSplit    = m_lumaLambda;

        for (int i = 0; i < SAOQTPart::NUM_DOWN_PART; i++)
        {
            if (i) //initialize RD with previous depth buffer
                m_rdEntropyCoders[nextDepth][CI_CURR_BEST].load(m_rdEntropyCoders[nextDepth][CI_NEXT_BEST]);
            else
                m_rdEntropyCoders[nextDepth][CI_CURR_BEST].load(m_rdEntropyCoders[onePart->partLevel][CI_CURR_BEST]);

            runQuadTreeDecision(qtPart, onePart->downPartsIdx[i], costFinal, maxLevel, plane);
            costSplit += costFinal;
            m_rdEntropyCoders[nextDepth][CI_NEXT_BEST].load(m_rdEntropyCoders[nextDepth][CI_TEMP_BEST]);
        }

        if (costSplit < costNotSplit)
        {
            costFinal = costSplit;
            onePart->bSplit   = true;
            onePart->length   =  0;
            onePart->bestType = -1;
            m_rdEntropyCoders[onePart->partLevel][CI_NEXT_BEST].load(m_rdEntropyCoders[nextDepth][CI_NEXT_BEST]);
        }
        else
        {
            costFinal = costNotSplit;
            onePart->bSplit = false;
            for (int i = 0; i < SAOQTPart::NUM_DOWN_PART; i++)
                disablePartTree(qtPart, onePart->downPartsIdx[i]);

            m_rdEntropyCoders[onePart->partLevel][CI_NEXT_BEST].load(m_rdEntropyCoders[onePart->partLevel][CI_TEMP_BEST]);
        }
    }
    else
        costFinal = onePart->minCost;
}

/* Calculate SAO statistics for current LCU without non-crossing slice */
void SAO::calcSaoStatsCu(int addr, int partIdx, int plane)
{
    int x, y;
    TComDataCU *cu = m_pic->getCU(addr);

    pixel* fenc;
    pixel* recon;
    int stride;
    int lcuHeight;
    int lcuWidth;
    uint32_t rpelx;
    uint32_t bpely;
    uint32_t picWidthTmp;
    uint32_t picHeightTmp;
    int64_t* stats;
    int64_t* counts;
    int classIdx;
    int startX;
    int startY;
    int endX;
    int endY;
    uint32_t lpelx = cu->getCUPelX();
    uint32_t tpely = cu->getCUPelY();

    int isLuma = !plane;
    int isChroma = !!plane;
    int numSkipLine = isChroma ? 4 - (2 * m_vChromaShift) : 4;

    if (!m_param->saoLcuBasedOptimization)
        numSkipLine = 0;

    int numSkipLineRight = isChroma ? 5 - (2 * m_hChromaShift) : 5;

    if (!m_param->saoLcuBasedOptimization)
        numSkipLineRight = 0;

    picWidthTmp  = isLuma ? m_param->sourceWidth  : m_param->sourceWidth  >> m_hChromaShift;
    picHeightTmp = isLuma ? m_param->sourceHeight : m_param->sourceHeight >> m_vChromaShift;
    lcuWidth     = isLuma ? g_maxCUSize : g_maxCUSize >> m_hChromaShift;
    lcuHeight    = isLuma ? g_maxCUSize : g_maxCUSize >> m_vChromaShift;
    lpelx        = isLuma ? lpelx       : lpelx       >> m_hChromaShift;
    tpely        = isLuma ? tpely       : tpely       >> m_vChromaShift;

    rpelx     = lpelx + lcuWidth;
    bpely     = tpely + lcuHeight;
    rpelx     = rpelx > picWidthTmp  ? picWidthTmp  : rpelx;
    bpely     = bpely > picHeightTmp ? picHeightTmp : bpely;
    lcuWidth  = rpelx - lpelx;
    lcuHeight = bpely - tpely;
    stride    =  (plane == 0) ? m_pic->getStride() : m_pic->getCStride();

    //if(iSaoType == BO_0 || iSaoType == BO_1)
    {
        if (m_param->saoLcuBasedOptimization && m_param->saoLcuBoundary)
        {
            numSkipLine      = isChroma ? 3 - (2 * m_vChromaShift) : 3;
            numSkipLineRight = isChroma ? 4 - (2 * m_hChromaShift) : 4;
        }
        stats = m_offsetOrg[partIdx][SAO_BO];
        counts = m_count[partIdx][SAO_BO];

        fenc = m_pic->getPicYuvOrg()->getPlaneAddr(plane, addr);
        recon = m_pic->getPicYuvRec()->getPlaneAddr(plane, addr);

        endX = (rpelx == picWidthTmp) ? lcuWidth : lcuWidth - numSkipLineRight;
        endY = (bpely == picHeightTmp) ? lcuHeight : lcuHeight - numSkipLine;
        for (y = 0; y < endY; y++)
        {
            for (x = 0; x < endX; x++)
            {
                classIdx = m_tableBo[recon[x]];
                if (classIdx)
                {
                    stats[classIdx] += (fenc[x] - recon[x]);
                    counts[classIdx]++;
                }
            }

            fenc += stride;
            recon += stride;
        }
    }

    int signLeft;
    int signRight;
    int signDown;
    int signDown1;
    int signDown2;
    uint32_t edgeType;
    int32_t _upBuff1[MAX_CU_SIZE + 2], *upBuff1 = _upBuff1 + 1;
    int32_t _upBufft[MAX_CU_SIZE + 2], *upBufft = _upBufft + 1;

    //if (iSaoType == EO_0  || iSaoType == EO_1 || iSaoType == EO_2 || iSaoType == EO_3)
    {
        //if (iSaoType == EO_0)
        {
            if (m_param->saoLcuBasedOptimization && m_param->saoLcuBoundary)
            {
                numSkipLine      = isChroma ? 3 - (2 * m_vChromaShift) : 3;
                numSkipLineRight = isChroma ? 5 - (2 * m_hChromaShift) : 5;
            }
            stats = m_offsetOrg[partIdx][SAO_EO_0];
            counts = m_count[partIdx][SAO_EO_0];

            fenc = m_pic->getPicYuvOrg()->getPlaneAddr(plane, addr);
            recon = m_pic->getPicYuvRec()->getPlaneAddr(plane, addr);

            startX = (lpelx == 0) ? 1 : 0;
            endX   = (rpelx == picWidthTmp) ? lcuWidth - 1 : lcuWidth - numSkipLineRight;
            for (y = 0; y < lcuHeight - numSkipLine; y++)
            {
                signLeft = signOf(recon[startX] - recon[startX - 1]);
                for (x = startX; x < endX; x++)
                {
                    signRight = signOf(recon[x] - recon[x + 1]);
                    edgeType = signRight + signLeft + 2;
                    signLeft = -signRight;

                    stats[s_eoTable[edgeType]] += (fenc[x] - recon[x]);
                    counts[s_eoTable[edgeType]]++;
                }

                fenc += stride;
                recon += stride;
            }
        }

        //if (iSaoType == EO_1)
        {
            if (m_param->saoLcuBasedOptimization && m_param->saoLcuBoundary)
            {
                numSkipLine      = isChroma ? 4 - (2 * m_vChromaShift) : 4;
                numSkipLineRight = isChroma ? 4 - (2 * m_hChromaShift) : 4;
            }
            stats = m_offsetOrg[partIdx][SAO_EO_1];
            counts = m_count[partIdx][SAO_EO_1];

            fenc = m_pic->getPicYuvOrg()->getPlaneAddr(plane, addr);
            recon = m_pic->getPicYuvRec()->getPlaneAddr(plane, addr);

            startY = (tpely == 0) ? 1 : 0;
            endX   = (rpelx == picWidthTmp) ? lcuWidth : lcuWidth - numSkipLineRight;
            endY   = (bpely == picHeightTmp) ? lcuHeight - 1 : lcuHeight - numSkipLine;
            if (tpely == 0)
            {
                fenc += stride;
                recon += stride;
            }

            for (x = 0; x < lcuWidth; x++)
                upBuff1[x] = signOf(recon[x] - recon[x - stride]);

            for (y = startY; y < endY; y++)
            {
                for (x = 0; x < endX; x++)
                {
                    signDown = signOf(recon[x] - recon[x + stride]);
                    edgeType = signDown + upBuff1[x] + 2;
                    upBuff1[x] = -signDown;

                    stats[s_eoTable[edgeType]] += (fenc[x] - recon[x]);
                    counts[s_eoTable[edgeType]]++;
                }

                fenc += stride;
                recon += stride;
            }
        }
        //if (iSaoType == EO_2)
        {
            if (m_param->saoLcuBasedOptimization && m_param->saoLcuBoundary)
            {
                numSkipLine      = isChroma ? 4 - (2 * m_vChromaShift) : 4;
                numSkipLineRight = isChroma ? 5 - (2 * m_hChromaShift) : 5;
            }
            stats = m_offsetOrg[partIdx][SAO_EO_2];
            counts = m_count[partIdx][SAO_EO_2];

            fenc = m_pic->getPicYuvOrg()->getPlaneAddr(plane, addr);
            recon = m_pic->getPicYuvRec()->getPlaneAddr(plane, addr);

            startX = (lpelx == 0) ? 1 : 0;
            endX   = (rpelx == picWidthTmp) ? lcuWidth - 1 : lcuWidth - numSkipLineRight;

            startY = (tpely == 0) ? 1 : 0;
            endY   = (bpely == picHeightTmp) ? lcuHeight - 1 : lcuHeight - numSkipLine;
            if (tpely == 0)
            {
                fenc += stride;
                recon += stride;
            }

            for (x = startX; x < endX; x++)
                upBuff1[x] = signOf(recon[x] - recon[x - stride - 1]);

            for (y = startY; y < endY; y++)
            {
                signDown2 = signOf(recon[stride + startX] - recon[startX - 1]);
                for (x = startX; x < endX; x++)
                {
                    signDown1 = signOf(recon[x] - recon[x + stride + 1]);
                    edgeType  = signDown1 + upBuff1[x] + 2;
                    upBufft[x + 1] = -signDown1;
                    stats[s_eoTable[edgeType]] += (fenc[x] - recon[x]);
                    counts[s_eoTable[edgeType]]++;
                }

                upBufft[startX] = signDown2;
                std::swap(upBuff1, upBufft);

                recon += stride;
                fenc += stride;
            }
        }
        //if (iSaoType == EO_3)
        {
            if (m_param->saoLcuBasedOptimization && m_param->saoLcuBoundary)
            {
                numSkipLine      = isChroma ? 4 - (2 * m_vChromaShift) : 4;
                numSkipLineRight = isChroma ? 5 - (2 * m_hChromaShift) : 5;
            }
            stats = m_offsetOrg[partIdx][SAO_EO_3];
            counts = m_count[partIdx][SAO_EO_3];

            fenc = m_pic->getPicYuvOrg()->getPlaneAddr(plane, addr);
            recon = m_pic->getPicYuvRec()->getPlaneAddr(plane, addr);

            startX = (lpelx == 0) ? 1 : 0;
            endX   = (rpelx == picWidthTmp) ? lcuWidth - 1 : lcuWidth - numSkipLineRight;

            startY = (tpely == 0) ? 1 : 0;
            endY   = (bpely == picHeightTmp) ? lcuHeight - 1 : lcuHeight - numSkipLine;
            if (startY == 1)
            {
                fenc += stride;
                recon += stride;
            }

            for (x = startX - 1; x < endX; x++)
                upBuff1[x] = signOf(recon[x] - recon[x - stride + 1]);

            for (y = startY; y < endY; y++)
            {
                for (x = startX; x < endX; x++)
                {
                    signDown1 = signOf(recon[x] - recon[x + stride - 1]);
                    edgeType  = signDown1 + upBuff1[x] + 2;
                    upBuff1[x - 1] = -signDown1;
                    stats[s_eoTable[edgeType]] += (fenc[x] - recon[x]);
                    counts[s_eoTable[edgeType]]++;
                }

                upBuff1[endX - 1] = signOf(recon[endX - 1 + stride] - recon[endX]);

                recon += stride;
                fenc += stride;
            }
        }
    }
}

void SAO::calcSaoStatsCu_BeforeDblk(Frame* pic, int idxX, int idxY)
{
    int addr;
    int x, y;

    pixel* fenc;
    pixel* recon;
    int stride;
    uint32_t rPelX;
    uint32_t bPelY;
    int64_t* stats;
    int64_t* count;
    int classIdx;
    int startX;
    int startY;
    int endX;
    int endY;
    int firstX, firstY;

    int frameWidthInCU  = m_numCuInWidth;

    int isChroma;
    int numSkipLine, numSkipLineRight;

    uint32_t lPelX, tPelY;
    TComDataCU *cu;
    int32_t _upBuff1[MAX_CU_SIZE + 2], *upBuff1 = _upBuff1 + 1;
    int32_t _upBufft[MAX_CU_SIZE + 2], *upBufft = _upBufft + 1;

    // NOTE: Row
    {
        // NOTE: Col
        {
            addr    = idxX + frameWidthInCU * idxY;
            cu      = pic->getCU(addr);

            uint32_t picWidthTmp  = m_param->sourceWidth;
            uint32_t picHeightTmp = m_param->sourceHeight;
            int lcuWidth  = g_maxCUSize;
            int lcuHeight = g_maxCUSize;
            lPelX   = cu->getCUPelX();
            tPelY   = cu->getCUPelY();
            rPelX     = lPelX + lcuWidth;
            bPelY     = tPelY + lcuHeight;
            rPelX     = rPelX > picWidthTmp  ? picWidthTmp  : rPelX;
            bPelY     = bPelY > picHeightTmp ? picHeightTmp : bPelY;
            lcuWidth  = rPelX - lPelX;
            lcuHeight = bPelY - tPelY;

            memset(m_countPreDblk[addr], 0, sizeof(PerPlane));
            memset(m_offsetOrgPreDblk[addr], 0, sizeof(PerPlane));

            for (int plane = 0; plane < 3; plane++)
            {
                isChroma = !!plane;
                if (plane == 1)
                {
                    picWidthTmp  >>= m_hChromaShift;
                    picHeightTmp >>= m_vChromaShift;
                    lcuWidth     >>= m_hChromaShift;
                    lcuHeight    >>= m_vChromaShift;
                    lPelX        >>= m_hChromaShift;
                    tPelY        >>= m_vChromaShift;
                    rPelX     = lPelX + lcuWidth;
                    bPelY     = tPelY + lcuHeight;
                }

                stride   = (plane == 0) ? pic->getStride() : pic->getCStride();

                //if(iSaoType == BO)

                numSkipLine = isChroma ? 1 : 3;
                numSkipLineRight = isChroma ? 2 : 4;

                stats = m_offsetOrgPreDblk[addr][plane][SAO_BO];
                count = m_countPreDblk[addr][plane][SAO_BO];

                fenc = m_pic->getPicYuvOrg()->getPlaneAddr(plane, addr);
                recon = m_pic->getPicYuvRec()->getPlaneAddr(plane, addr);

                startX = (rPelX == picWidthTmp) ? lcuWidth : lcuWidth - numSkipLineRight;
                startY = (bPelY == picHeightTmp) ? lcuHeight : lcuHeight - numSkipLine;

                for (y = 0; y < lcuHeight; y++)
                {
                    for (x = 0; x < lcuWidth; x++)
                    {
                        if (x < startX && y < startY)
                            continue;

                        classIdx = m_tableBo[recon[x]];
                        if (classIdx)
                        {
                            stats[classIdx] += (fenc[x] - recon[x]);
                            count[classIdx]++;
                        }
                    }

                    fenc += stride;
                    recon += stride;
                }

                int signLeft;
                int signRight;
                int signDown;
                int signDown1;
                int signDown2;

                uint32_t edgeType;

                //if (iSaoType == EO_0)

                numSkipLine = isChroma ? 1 : 3;
                numSkipLineRight = isChroma ? 3 : 5;

                stats = m_offsetOrgPreDblk[addr][plane][SAO_EO_0];
                count = m_countPreDblk[addr][plane][SAO_EO_0];

                fenc = m_pic->getPicYuvOrg()->getPlaneAddr(plane, addr);
                recon = m_pic->getPicYuvRec()->getPlaneAddr(plane, addr);

                startX = (rPelX == picWidthTmp) ? lcuWidth - 1 : lcuWidth - numSkipLineRight;
                startY = (bPelY == picHeightTmp) ? lcuHeight : lcuHeight - numSkipLine;
                firstX = (lPelX == 0) ? 1 : 0;
                endX   = (rPelX == picWidthTmp) ? lcuWidth - 1 : lcuWidth;

                for (y = 0; y < lcuHeight; y++)
                {
                    signLeft = signOf(recon[firstX] - recon[firstX - 1]);
                    for (x = firstX; x < endX; x++)
                    {
                        signRight =  signOf(recon[x] - recon[x + 1]);
                        edgeType =  signRight + signLeft + 2;
                        signLeft  = -signRight;

                        if (x < startX && y < startY)
                            continue;

                        stats[s_eoTable[edgeType]] += (fenc[x] - recon[x]);
                        count[s_eoTable[edgeType]]++;
                    }

                    fenc += stride;
                    recon += stride;
                }

                //if (iSaoType == EO_1)

                numSkipLine = isChroma ? 2 : 4;
                numSkipLineRight = isChroma ? 2 : 4;

                stats = m_offsetOrgPreDblk[addr][plane][SAO_EO_1];
                count = m_countPreDblk[addr][plane][SAO_EO_1];

                fenc = m_pic->getPicYuvOrg()->getPlaneAddr(plane, addr);
                recon = m_pic->getPicYuvRec()->getPlaneAddr(plane, addr);

                startX = (rPelX == picWidthTmp) ? lcuWidth : lcuWidth - numSkipLineRight;
                startY = (bPelY == picHeightTmp) ? lcuHeight - 1 : lcuHeight - numSkipLine;
                firstY = (tPelY == 0) ? 1 : 0;
                endY   = (bPelY == picHeightTmp) ? lcuHeight - 1 : lcuHeight;
                if (firstY == 1)
                {
                    fenc += stride;
                    recon += stride;
                }

                for (x = 0; x < lcuWidth; x++)
                    upBuff1[x] = signOf(recon[x] - recon[x - stride]);

                for (y = firstY; y < endY; y++)
                {
                    for (x = 0; x < lcuWidth; x++)
                    {
                        signDown = signOf(recon[x] - recon[x + stride]);
                        edgeType = signDown + upBuff1[x] + 2;
                        upBuff1[x] = -signDown;

                        if (x < startX && y < startY)
                            continue;

                        stats[s_eoTable[edgeType]] += (fenc[x] - recon[x]);
                        count[s_eoTable[edgeType]]++;
                    }

                    fenc += stride;
                    recon += stride;
                }

                //if (iSaoType == EO_2)

                numSkipLine = isChroma ? 2 : 4;
                numSkipLineRight = isChroma ? 3 : 5;

                stats = m_offsetOrgPreDblk[addr][plane][SAO_EO_2];
                count = m_countPreDblk[addr][plane][SAO_EO_2];

                fenc = m_pic->getPicYuvOrg()->getPlaneAddr(plane, addr);
                recon = m_pic->getPicYuvRec()->getPlaneAddr(plane, addr);

                startX = (rPelX == picWidthTmp) ? lcuWidth - 1 : lcuWidth - numSkipLineRight;
                startY = (bPelY == picHeightTmp) ? lcuHeight - 1 : lcuHeight - numSkipLine;
                firstX = (lPelX == 0) ? 1 : 0;
                firstY = (tPelY == 0) ? 1 : 0;
                endX   = (rPelX == picWidthTmp) ? lcuWidth - 1 : lcuWidth;
                endY   = (bPelY == picHeightTmp) ? lcuHeight - 1 : lcuHeight;
                if (firstY == 1)
                {
                    fenc += stride;
                    recon += stride;
                }

                for (x = firstX; x < endX; x++)
                    upBuff1[x] = signOf(recon[x] - recon[x - stride - 1]);

                for (y = firstY; y < endY; y++)
                {
                    signDown2 = signOf(recon[stride + startX] - recon[startX - 1]);
                    for (x = firstX; x < endX; x++)
                    {
                        signDown1 = signOf(recon[x] - recon[x + stride + 1]);
                        edgeType = signDown1 + upBuff1[x] + 2;
                        upBufft[x + 1] = -signDown1;

                        if (x < startX && y < startY)
                            continue;

                        stats[s_eoTable[edgeType]] += (fenc[x] - recon[x]);
                        count[s_eoTable[edgeType]]++;
                    }

                    upBufft[firstX] = signDown2;
                    std::swap(upBuff1, upBufft);

                    recon += stride;
                    fenc += stride;
                }

                //if (iSaoType == EO_3)

                numSkipLine = isChroma ? 2 : 4;
                numSkipLineRight = isChroma ? 3 : 5;

                stats = m_offsetOrgPreDblk[addr][plane][SAO_EO_3];
                count = m_countPreDblk[addr][plane][SAO_EO_3];

                fenc = m_pic->getPicYuvOrg()->getPlaneAddr(plane, addr);
                recon = m_pic->getPicYuvRec()->getPlaneAddr(plane, addr);

                startX = (rPelX == picWidthTmp) ? lcuWidth - 1 : lcuWidth - numSkipLineRight;
                startY = (bPelY == picHeightTmp) ? lcuHeight - 1 : lcuHeight - numSkipLine;
                firstX = (lPelX == 0) ? 1 : 0;
                firstY = (tPelY == 0) ? 1 : 0;
                endX   = (rPelX == picWidthTmp) ? lcuWidth - 1 : lcuWidth;
                endY   = (bPelY == picHeightTmp) ? lcuHeight - 1 : lcuHeight;
                if (firstY == 1)
                {
                    fenc += stride;
                    recon += stride;
                }

                for (x = firstX - 1; x < endX; x++)
                    upBuff1[x] = signOf(recon[x] - recon[x - stride + 1]);

                for (y = firstY; y < endY; y++)
                {
                    for (x = firstX; x < endX; x++)
                    {
                        signDown1 = signOf(recon[x] - recon[x + stride - 1]);
                        edgeType  = signDown1 + upBuff1[x] + 2;
                        upBuff1[x - 1] = -signDown1;

                        if (x < startX && y < startY)
                            continue;

                        stats[s_eoTable[edgeType]] += (fenc[x] - recon[x]);
                        count[s_eoTable[edgeType]]++;
                    }

                    upBuff1[endX - 1] = signOf(recon[endX - 1 + stride] - recon[endX]);

                    recon += stride;
                    fenc += stride;
                }
            }
        }
    }
}

void SAO::getSaoStats(SAOQTPart *psQTPart, int plane)
{
    int levelIdx, partIdx, typeIdx, classIdx;
    int i;
    int numTotalType = MAX_NUM_SAO_TYPE;
    int lcuIdx;
    int lcuIdy;
    int addr;
    int frameWidthInCU = m_pic->getFrameWidthInCU();
    int downPartIdx;
    int partStart;
    int partEnd;
    SAOQTPart* onePart;

    if (!m_maxSplitLevel)
    {
        partIdx = 0;
        onePart = &(psQTPart[partIdx]);
        for (lcuIdy = onePart->startCUY; lcuIdy <= onePart->endCUY; lcuIdy++)
        {
            for (lcuIdx = onePart->startCUX; lcuIdx <= onePart->endCUX; lcuIdx++)
            {
                addr = lcuIdy * frameWidthInCU + lcuIdx;
                calcSaoStatsCu(addr, partIdx, plane);
            }
        }
    }
    else
    {
        for (partIdx = s_numCulPartsLevel[m_maxSplitLevel - 1]; partIdx < s_numCulPartsLevel[m_maxSplitLevel]; partIdx++)
        {
            onePart = &(psQTPart[partIdx]);
            for (lcuIdy = onePart->startCUY; lcuIdy <= onePart->endCUY; lcuIdy++)
            {
                for (lcuIdx = onePart->startCUX; lcuIdx <= onePart->endCUX; lcuIdx++)
                {
                    addr = lcuIdy * frameWidthInCU + lcuIdx;
                    calcSaoStatsCu(addr, partIdx, plane);
                }
            }
        }

        for (levelIdx = m_maxSplitLevel - 1; levelIdx >= 0; levelIdx--)
        {
            partStart = (levelIdx > 0) ? s_numCulPartsLevel[levelIdx - 1] : 0;
            partEnd   = s_numCulPartsLevel[levelIdx];

            for (partIdx = partStart; partIdx < partEnd; partIdx++)
            {
                onePart = &(psQTPart[partIdx]);
                for (i = 0; i < SAOQTPart::NUM_DOWN_PART; i++)
                {
                    downPartIdx = onePart->downPartsIdx[i];
                    for (typeIdx = 0; typeIdx < numTotalType; typeIdx++)
                    {
                        for (classIdx = 0; classIdx < (typeIdx < SAO_BO ? s_numClass[typeIdx] : SAO_MAX_BO_CLASSES) + 1; classIdx++)
                        {
                            m_offsetOrg[partIdx][typeIdx][classIdx] += m_offsetOrg[downPartIdx][typeIdx][classIdx];
                            m_count[partIdx][typeIdx][classIdx]    += m_count[downPartIdx][typeIdx][classIdx];
                        }
                    }
                }
            }
        }
    }
}

/* reset offset statistics */
void SAO::resetStats()
{
    for (int i = 0; i < m_numTotalParts; i++)
    {
        m_costPartBest[i] = MAX_DOUBLE;
        m_typePartBest[i] = -1;
        m_distOrg[i] = 0;
        for (int j = 0; j < MAX_NUM_SAO_TYPE; j++)
        {
            m_dist[i][j] = 0;
            m_rate[i][j] = 0;
            m_cost[i][j] = 0;
            for (int k = 0; k < MAX_NUM_SAO_CLASS; k++)
            {
                m_count[i][j][k] = 0;
                m_offset[i][j][k] = 0;
                m_offsetOrg[i][j][k] = 0;
            }
        }
    }
}

/* Sample adaptive offset process */
void SAO::SAOProcess(SAOParam *saoParam)
{
    X265_CHECK(!m_param->saoLcuBasedOptimization, "SAO LCU mode failure\n"); 
    double costFinal = 0;
    saoParam->bSaoFlag[0] = true;
    saoParam->bSaoFlag[1] = false;

    getSaoStats(saoParam->saoPart[0], 0);
    runQuadTreeDecision(saoParam->saoPart[0], 0, costFinal, m_maxSplitLevel, 0);
    saoParam->bSaoFlag[0] = costFinal < 0;

    if (saoParam->bSaoFlag[0])
    {
        convertQT2SaoUnit(saoParam, 0, 0);
        assignSaoUnitSyntax(saoParam->saoLcuParam[0], saoParam->saoPart[0], saoParam->oneUnitFlag[0]);
        processSaoUnitAll(saoParam->saoLcuParam[0], saoParam->oneUnitFlag[0], 0);
    }
    if (saoParam->bSaoFlag[1])
    {
        processSaoUnitAll(saoParam->saoLcuParam[1], saoParam->oneUnitFlag[1], 1);
        processSaoUnitAll(saoParam->saoLcuParam[2], saoParam->oneUnitFlag[2], 2);
    }
}

/* Check merge SAO unit */
void SAO::checkMerge(SaoLcuParam * saoUnitCurr, SaoLcuParam * saoUnitCheck, int dir)
{
    int i;
    int countDiff = 0;

    if (saoUnitCurr->partIdx != saoUnitCheck->partIdx)
    {
        if (saoUnitCurr->typeIdx != -1)
        {
            if (saoUnitCurr->typeIdx == saoUnitCheck->typeIdx)
            {
                for (i = 0; i < saoUnitCurr->length; i++)
                    countDiff += (saoUnitCurr->offset[i] != saoUnitCheck->offset[i]);

                countDiff += (saoUnitCurr->subTypeIdx != saoUnitCheck->subTypeIdx);
                if (countDiff == 0)
                {
                    saoUnitCurr->partIdx = saoUnitCheck->partIdx;
                    if (dir == 1)
                    {
                        saoUnitCurr->mergeUpFlag = 1;
                        saoUnitCurr->mergeLeftFlag = 0;
                    }
                    else
                    {
                        saoUnitCurr->mergeUpFlag = 0;
                        saoUnitCurr->mergeLeftFlag = 1;
                    }
                }
            }
        }
        else
        {
            if (saoUnitCurr->typeIdx == saoUnitCheck->typeIdx)
            {
                saoUnitCurr->partIdx = saoUnitCheck->partIdx;
                if (dir == 1)
                {
                    saoUnitCurr->mergeUpFlag = 1;
                    saoUnitCurr->mergeLeftFlag = 0;
                }
                else
                {
                    saoUnitCurr->mergeUpFlag = 0;
                    saoUnitCurr->mergeLeftFlag = 1;
                }
            }
        }
    }
}

/** Assign SAO unit syntax from picture-based algorithm */
void SAO::assignSaoUnitSyntax(SaoLcuParam* saoLcuParam,  SAOQTPart* saoPart, bool &oneUnitFlag)
{
    if (saoPart->bSplit == 0)
        oneUnitFlag = 1;
    else
    {
        int i, j, addr, addrUp, addrLeft,  idx, idxUp, idxLeft,  idxCount;

        oneUnitFlag = 0;

        idxCount = -1;
        saoLcuParam[0].mergeUpFlag = 0;
        saoLcuParam[0].mergeLeftFlag = 0;

        for (j = 0; j < m_numCuInHeight; j++)
        {
            for (i = 0; i < m_numCuInWidth; i++)
            {
                addr     = i + j * m_numCuInWidth;
                addrLeft = (addr % m_numCuInWidth == 0) ? -1 : addr - 1;
                addrUp   = (addr < m_numCuInWidth)      ? -1 : addr - m_numCuInWidth;
                idx      = saoLcuParam[addr].partIdxTmp;
                idxLeft  = (addrLeft == -1) ? -1 : saoLcuParam[addrLeft].partIdxTmp;
                idxUp    = (addrUp == -1)   ? -1 : saoLcuParam[addrUp].partIdxTmp;

                if (idx != idxLeft && idx != idxUp)
                {
                    saoLcuParam[addr].mergeUpFlag   = 0;
                    idxCount++;
                    saoLcuParam[addr].mergeLeftFlag = 0;
                    saoLcuParam[addr].partIdx = idxCount;
                }
                else if (idx == idxLeft)
                {
                    saoLcuParam[addr].mergeUpFlag   = 1;
                    saoLcuParam[addr].mergeLeftFlag = 1;
                    saoLcuParam[addr].partIdx = saoLcuParam[addrLeft].partIdx;
                }
                else if (idx == idxUp)
                {
                    saoLcuParam[addr].mergeUpFlag   = 1;
                    saoLcuParam[addr].mergeLeftFlag = 0;
                    saoLcuParam[addr].partIdx = saoLcuParam[addrUp].partIdx;
                }
                if (addrUp != -1)
                    checkMerge(&saoLcuParam[addr], &saoLcuParam[addrUp], 1);
                if (addrLeft != -1)
                    checkMerge(&saoLcuParam[addr], &saoLcuParam[addrLeft], 0);
            }
        }
    }
}

void SAO::rdoSaoUnitRowInit(SAOParam *saoParam)
{
    saoParam->bSaoFlag[0] = true;
    saoParam->bSaoFlag[1] = true;
    saoParam->oneUnitFlag[0] = false;
    saoParam->oneUnitFlag[1] = false;
    saoParam->oneUnitFlag[2] = false;

    m_numNoSao[0] = 0; // Luma
    m_numNoSao[1] = 0; // Chroma
    if (m_refDepth > 0 && m_depthSaoRate[0][m_refDepth - 1] > SAO_ENCODING_RATE)
        saoParam->bSaoFlag[0] = false;
    if (m_refDepth > 0 && m_depthSaoRate[1][m_refDepth - 1] > SAO_ENCODING_RATE_CHROMA)
        saoParam->bSaoFlag[1] = false;
}

void SAO::rdoSaoUnitRowEnd(SAOParam *saoParam, int numlcus)
{
    if (!saoParam->bSaoFlag[0])
        m_depthSaoRate[0][m_refDepth] = 1.0;
    else
        m_depthSaoRate[0][m_refDepth] = m_numNoSao[0] / ((double)numlcus);

    if (!saoParam->bSaoFlag[1])
        m_depthSaoRate[1][m_refDepth] = 1.0;
    else
        m_depthSaoRate[1][m_refDepth] = m_numNoSao[1] / ((double)numlcus * 2);
}

void SAO::rdoSaoUnitRow(SAOParam *saoParam, int idxY)
{
    int idxX;
    int frameWidthInCU  = saoParam->numCuInWidth;
    int j, k;
    int addr = 0;
    int addrUp = -1;
    int addrLeft = -1;
    int compIdx = 0;
    SaoLcuParam mergeSaoParam[3][2];
    double compDistortion[3];

    for (idxX = 0; idxX < frameWidthInCU; idxX++)
    {
        addr     = idxX  + frameWidthInCU * idxY;
        addrUp   = addr < frameWidthInCU ? -1 : idxX     + frameWidthInCU * (idxY - 1);
        addrLeft = idxX == 0             ? -1 : idxX - 1 + frameWidthInCU * idxY;
        int allowMergeLeft = 1;
        int allowMergeUp   = 1;
        uint32_t rate;
        double bestCost, mergeCost;
        if (idxX == 0)
            allowMergeLeft = 0;
        if (idxY == 0)
            allowMergeUp = 0;

        compDistortion[0] = 0;
        compDistortion[1] = 0;
        compDistortion[2] = 0;
        m_entropyCoder.load(m_rdEntropyCoders[0][CI_CURR_BEST]);
        if (allowMergeLeft)
            m_entropyCoder.codeSaoMerge(0);
        if (allowMergeUp)
            m_entropyCoder.codeSaoMerge(0);
        m_entropyCoder.store(m_rdEntropyCoders[0][CI_TEMP_BEST]);
        // reset stats Y, Cb, Cr
        for (compIdx = 0; compIdx < 3; compIdx++)
        {
            for (j = 0; j < MAX_NUM_SAO_TYPE; j++)
            {
                for (k = 0; k < MAX_NUM_SAO_CLASS; k++)
                {
                    m_offset[compIdx][j][k] = 0;
                    if (m_param->saoLcuBasedOptimization && m_param->saoLcuBoundary)
                    {
                        m_count[compIdx][j][k] = m_countPreDblk[addr][compIdx][j][k];
                        m_offsetOrg[compIdx][j][k] = m_offsetOrgPreDblk[addr][compIdx][j][k];
                    }
                    else
                    {
                        m_count[compIdx][j][k] = 0;
                        m_offsetOrg[compIdx][j][k] = 0;
                    }
                }
            }

            saoParam->saoLcuParam[compIdx][addr].typeIdx       =  -1;
            saoParam->saoLcuParam[compIdx][addr].mergeUpFlag   = 0;
            saoParam->saoLcuParam[compIdx][addr].mergeLeftFlag = 0;
            saoParam->saoLcuParam[compIdx][addr].subTypeIdx    = 0;
            if ((compIdx == 0 && saoParam->bSaoFlag[0]) || (compIdx > 0 && saoParam->bSaoFlag[1]))
                calcSaoStatsCu(addr, compIdx,  compIdx);
        }

        saoComponentParamDist(allowMergeLeft, allowMergeUp, saoParam, addr, addrUp, addrLeft, 0, 
                              &mergeSaoParam[0][0], &compDistortion[0]);

        sao2ChromaParamDist(allowMergeLeft, allowMergeUp, saoParam, addr, addrUp, addrLeft,
                            &mergeSaoParam[1][0], &mergeSaoParam[2][0], &compDistortion[0]);

        if (saoParam->bSaoFlag[0] || saoParam->bSaoFlag[1])
        {
            // Cost of new SAO_params
            m_entropyCoder.load(m_rdEntropyCoders[0][CI_CURR_BEST]);
            m_entropyCoder.resetBits();
            if (allowMergeLeft)
                m_entropyCoder.codeSaoMerge(0);
            if (allowMergeUp)
                m_entropyCoder.codeSaoMerge(0);
            for (compIdx = 0; compIdx < 3; compIdx++)
            {
                if ((compIdx == 0 && saoParam->bSaoFlag[0]) || (compIdx > 0 && saoParam->bSaoFlag[1]))
                    m_entropyCoder.codeSaoOffset(&saoParam->saoLcuParam[compIdx][addr], compIdx);
            }

            rate = m_entropyCoder.getNumberOfWrittenBits();
            bestCost = compDistortion[0] + (double)rate;
            m_entropyCoder.store(m_rdEntropyCoders[0][CI_TEMP_BEST]);

            // Cost of Merge
            for (int mergeUp = 0; mergeUp < 2; ++mergeUp)
            {
                if ((allowMergeLeft && !mergeUp) || (allowMergeUp && mergeUp))
                {
                    m_entropyCoder.load(m_rdEntropyCoders[0][CI_CURR_BEST]);
                    m_entropyCoder.resetBits();
                    if (allowMergeLeft)
                        m_entropyCoder.codeSaoMerge(1 - mergeUp);
                    if (allowMergeUp && (mergeUp == 1))
                        m_entropyCoder.codeSaoMerge(1);

                    rate = m_entropyCoder.getNumberOfWrittenBits();
                    mergeCost = compDistortion[mergeUp + 1] + (double)rate;
                    if (mergeCost < bestCost)
                    {
                        bestCost = mergeCost;
                        m_entropyCoder.store(m_rdEntropyCoders[0][CI_TEMP_BEST]);
                        for (compIdx = 0; compIdx < 3; compIdx++)
                        {
                            mergeSaoParam[compIdx][mergeUp].mergeLeftFlag = !mergeUp;
                            mergeSaoParam[compIdx][mergeUp].mergeUpFlag = !!mergeUp;
                            if ((compIdx == 0 && saoParam->bSaoFlag[0]) || (compIdx > 0 && saoParam->bSaoFlag[1]))
                                copySaoUnit(&saoParam->saoLcuParam[compIdx][addr], &mergeSaoParam[compIdx][mergeUp]);
                        }
                    }
                }
            }

            if (saoParam->saoLcuParam[0][addr].typeIdx == -1)
                m_numNoSao[0]++;
            if (saoParam->saoLcuParam[1][addr].typeIdx == -1)
                m_numNoSao[1] += 2;
            m_entropyCoder.load(m_rdEntropyCoders[0][CI_TEMP_BEST]);
            m_entropyCoder.store(m_rdEntropyCoders[0][CI_CURR_BEST]);
        }
    }
}

/** rate distortion optimization of SAO unit */
inline int64_t SAO::estSaoTypeDist(int compIdx, int typeIdx, int shift, double lambda, int32_t *currentDistortionTableBo, double *currentRdCostTableBo)
{
    int64_t estDist = 0;
    int classIdx;

    for (classIdx = 1; classIdx < ((typeIdx < SAO_BO) ?  s_numClass[typeIdx] + 1 : SAO_MAX_BO_CLASSES + 1); classIdx++)
    {
        if (typeIdx == SAO_BO)
        {
            currentDistortionTableBo[classIdx - 1] = 0;
            currentRdCostTableBo[classIdx - 1] = lambda;
        }
        if (m_count[compIdx][typeIdx][classIdx])
        {
            m_offset[compIdx][typeIdx][classIdx] = (int64_t)roundIDBI((double)(m_offsetOrg[compIdx][typeIdx][classIdx] << (X265_DEPTH - 8)) / (double)(m_count[compIdx][typeIdx][classIdx] << SAO_BIT_INC));
            m_offset[compIdx][typeIdx][classIdx] = Clip3(-OFFSET_THRESH + 1, OFFSET_THRESH - 1, (int)m_offset[compIdx][typeIdx][classIdx]);
            if (typeIdx < 4)
            {
                if (m_offset[compIdx][typeIdx][classIdx] < 0 && classIdx < 3)
                    m_offset[compIdx][typeIdx][classIdx] = 0;
                if (m_offset[compIdx][typeIdx][classIdx] > 0 && classIdx >= 3)
                    m_offset[compIdx][typeIdx][classIdx] = 0;
            }
            m_offset[compIdx][typeIdx][classIdx] = estIterOffset(typeIdx, classIdx, lambda, m_offset[compIdx][typeIdx][classIdx], m_count[compIdx][typeIdx][classIdx], m_offsetOrg[compIdx][typeIdx][classIdx], shift, SAO_BIT_INC, currentDistortionTableBo, currentRdCostTableBo, OFFSET_THRESH);
        }
        else
        {
            m_offsetOrg[compIdx][typeIdx][classIdx] = 0;
            m_offset[compIdx][typeIdx][classIdx] = 0;
        }
        if (typeIdx != SAO_BO)
            estDist += estSaoDist(m_count[compIdx][typeIdx][classIdx], m_offset[compIdx][typeIdx][classIdx] << SAO_BIT_INC, m_offsetOrg[compIdx][typeIdx][classIdx], shift);
    }

    return estDist;
}

inline int64_t SAO::estSaoDist(int64_t count, int64_t offset, int64_t offsetOrg, int shift)
{
    return (count * offset * offset - offsetOrg * offset * 2) >> shift;
}

inline int64_t SAO::estIterOffset(int typeIdx, int classIdx, double lambda, int64_t offsetInput, int64_t count, int64_t offsetOrg, int shift, int bitIncrease, int32_t *currentDistortionTableBo, double *currentRdCostTableBo, int offsetTh)
{
    //Clean up, best_q_offset.
    int64_t iterOffset, tempOffset;
    int64_t tempDist, tempRate;
    double tempCost, tempMinCost;
    int64_t offsetOutput = 0;

    iterOffset = offsetInput;
    // Assuming sending quantized value 0 results in zero offset and sending the value zero needs 1 bit. entropy coder can be used to measure the exact rate here.
    tempMinCost = lambda;
    while (iterOffset != 0)
    {
        // Calculate the bits required for signalling the offset
        tempRate = (typeIdx == SAO_BO) ? (abs((int)iterOffset) + 2) : (abs((int)iterOffset) + 1);
        if (abs((int)iterOffset) == offsetTh - 1)
            tempRate--;

        // Do the dequntization before distorion calculation
        tempOffset = iterOffset << bitIncrease;
        tempDist   = estSaoDist(count, tempOffset, offsetOrg, shift);
        tempCost   = ((double)tempDist + lambda * (double)tempRate);
        if (tempCost < tempMinCost)
        {
            tempMinCost = tempCost;
            offsetOutput = iterOffset;
            if (typeIdx == SAO_BO)
            {
                currentDistortionTableBo[classIdx - 1] = (int)tempDist;
                currentRdCostTableBo[classIdx - 1] = tempCost;
            }
        }
        iterOffset = (iterOffset > 0) ? (iterOffset - 1) : (iterOffset + 1);
    }

    return offsetOutput;
}

void SAO::saoComponentParamDist(int allowMergeLeft, int allowMergeUp, SAOParam *saoParam, int addr, int addrUp, int addrLeft, int plane,
                                SaoLcuParam *compSaoParam, double *compDistortion)
{
    int typeIdx;

    int64_t estDist;
    int classIdx;
    int64_t bestDist;

    SaoLcuParam* saoLcuParam = &(saoParam->saoLcuParam[plane][addr]);
    SaoLcuParam* saoLcuParamNeighbor = NULL;

    resetSaoUnit(saoLcuParam);
    resetSaoUnit(&compSaoParam[0]);
    resetSaoUnit(&compSaoParam[1]);

    double dCostPartBest = MAX_DOUBLE;
    double bestRDCostTableBo = MAX_DOUBLE;
    int    bestClassTableBo  = 0;
    int    currentDistortionTableBo[MAX_NUM_SAO_CLASS];
    double currentRdCostTableBo[MAX_NUM_SAO_CLASS];

    SaoLcuParam saoLcuParamRdo;
    double estRate = 0;

    resetSaoUnit(&saoLcuParamRdo);

    m_entropyCoder.load(m_rdEntropyCoders[0][CI_TEMP_BEST]);
    m_entropyCoder.resetBits();
    m_entropyCoder.codeSaoOffset(&saoLcuParamRdo, plane);
    dCostPartBest = m_entropyCoder.getNumberOfWrittenBits() * m_lumaLambda;
    copySaoUnit(saoLcuParam, &saoLcuParamRdo);
    bestDist = 0;

    for (typeIdx = 0; typeIdx < MAX_NUM_SAO_TYPE; typeIdx++)
    {
        estDist = estSaoTypeDist(plane, typeIdx, 0, m_lumaLambda, currentDistortionTableBo, currentRdCostTableBo);

        if (typeIdx == SAO_BO)
        {
            // Estimate Best Position
            double currentRDCost = 0.0;

            for (int i = 0; i < SAO_MAX_BO_CLASSES - SAO_BO_LEN + 1; i++)
            {
                currentRDCost = 0.0;
                for (int j = i; j < i + SAO_BO_LEN; j++)
                    currentRDCost += currentRdCostTableBo[j];

                if (currentRDCost < bestRDCostTableBo)
                {
                    bestRDCostTableBo = currentRDCost;
                    bestClassTableBo  = i;
                }
            }

            // Re code all Offsets
            // Code Center
            estDist = 0;
            for (classIdx = bestClassTableBo; classIdx < bestClassTableBo + SAO_BO_LEN; classIdx++)
                estDist += currentDistortionTableBo[classIdx];
        }
        resetSaoUnit(&saoLcuParamRdo);
        saoLcuParamRdo.length = s_numClass[typeIdx];
        saoLcuParamRdo.typeIdx = typeIdx;
        saoLcuParamRdo.mergeLeftFlag = 0;
        saoLcuParamRdo.mergeUpFlag   = 0;
        saoLcuParamRdo.subTypeIdx = (typeIdx == SAO_BO) ? bestClassTableBo : 0;
        for (classIdx = 0; classIdx < saoLcuParamRdo.length; classIdx++)
            saoLcuParamRdo.offset[classIdx] = (int)m_offset[plane][typeIdx][classIdx + saoLcuParamRdo.subTypeIdx + 1];

        m_entropyCoder.load(m_rdEntropyCoders[0][CI_TEMP_BEST]);
        m_entropyCoder.resetBits();
        m_entropyCoder.codeSaoOffset(&saoLcuParamRdo, plane);

        estRate = m_entropyCoder.getNumberOfWrittenBits();
        m_cost[plane][typeIdx] = (double)((double)estDist + m_lumaLambda * (double)estRate);

        if (m_cost[plane][typeIdx] < dCostPartBest)
        {
            dCostPartBest = m_cost[plane][typeIdx];
            copySaoUnit(saoLcuParam, &saoLcuParamRdo);
            bestDist = estDist;
        }
    }

    compDistortion[0] += ((double)bestDist / m_lumaLambda);
    m_entropyCoder.load(m_rdEntropyCoders[0][CI_TEMP_BEST]);
    m_entropyCoder.codeSaoOffset(saoLcuParam, plane);
    m_entropyCoder.store(m_rdEntropyCoders[0][CI_TEMP_BEST]);

    // merge left or merge up

    for (int idxNeighbor = 0; idxNeighbor < 2; idxNeighbor++)
    {
        saoLcuParamNeighbor = NULL;
        if (allowMergeLeft && addrLeft >= 0 && idxNeighbor == 0)
            saoLcuParamNeighbor = &(saoParam->saoLcuParam[plane][addrLeft]);
        else if (allowMergeUp && addrUp >= 0 && idxNeighbor == 1)
            saoLcuParamNeighbor = &(saoParam->saoLcuParam[plane][addrUp]);
        if (saoLcuParamNeighbor != NULL)
        {
            estDist = 0;
            typeIdx = saoLcuParamNeighbor->typeIdx;
            if (typeIdx >= 0)
            {
                int mergeBandPosition = (typeIdx == SAO_BO) ? saoLcuParamNeighbor->subTypeIdx : 0;
                int mergeOffset;
                for (classIdx = 0; classIdx < s_numClass[typeIdx]; classIdx++)
                {
                    mergeOffset = saoLcuParamNeighbor->offset[classIdx];
                    estDist += estSaoDist(m_count[plane][typeIdx][classIdx + mergeBandPosition + 1], mergeOffset, m_offsetOrg[plane][typeIdx][classIdx + mergeBandPosition + 1],  0);
                }
            }
            else
                estDist = 0;

            copySaoUnit(&compSaoParam[idxNeighbor], saoLcuParamNeighbor);
            compSaoParam[idxNeighbor].mergeUpFlag   = !!idxNeighbor;
            compSaoParam[idxNeighbor].mergeLeftFlag = !idxNeighbor;

            compDistortion[idxNeighbor + 1] += ((double)estDist / m_lumaLambda);
        }
    }
}

void SAO::sao2ChromaParamDist(int allowMergeLeft, int allowMergeUp, SAOParam *saoParam, int addr, int addrUp, int addrLeft,
                              SaoLcuParam *crSaoParam, SaoLcuParam *cbSaoParam, double *distortion)
{
    int64_t estDist[2];
    int64_t bestDist = 0;
    int typeIdx;
    int classIdx;

    SaoLcuParam* saoLcuParam[2] = { &(saoParam->saoLcuParam[1][addr]), &(saoParam->saoLcuParam[2][addr]) };
    SaoLcuParam* saoLcuParamNeighbor[2] = { NULL, NULL };
    SaoLcuParam* saoMergeParam[2][2];

    saoMergeParam[0][0] = &crSaoParam[0];
    saoMergeParam[0][1] = &crSaoParam[1];
    saoMergeParam[1][0] = &cbSaoParam[0];
    saoMergeParam[1][1] = &cbSaoParam[1];

    resetSaoUnit(saoLcuParam[0]);
    resetSaoUnit(saoLcuParam[1]);
    resetSaoUnit(saoMergeParam[0][0]);
    resetSaoUnit(saoMergeParam[0][1]);
    resetSaoUnit(saoMergeParam[1][0]);
    resetSaoUnit(saoMergeParam[1][1]);

    double costPartBest = MAX_DOUBLE;
    double bestRDCostTableBo;
    double currentRdCostTableBo[MAX_NUM_SAO_CLASS];
    double estRate = 0;
    int    bestClassTableBo[2] = { 0, 0 };
    int    currentDistortionTableBo[MAX_NUM_SAO_CLASS];

    SaoLcuParam saoLcuParamRdo[2];

    resetSaoUnit(&saoLcuParamRdo[0]);
    resetSaoUnit(&saoLcuParamRdo[1]);

    m_entropyCoder.load(m_rdEntropyCoders[0][CI_TEMP_BEST]);
    m_entropyCoder.resetBits();
    m_entropyCoder.codeSaoOffset(&saoLcuParamRdo[0], 1);
    m_entropyCoder.codeSaoOffset(&saoLcuParamRdo[1], 2);

    costPartBest = m_entropyCoder.getNumberOfWrittenBits() * m_chromaLambda;
    copySaoUnit(saoLcuParam[0], &saoLcuParamRdo[0]);
    copySaoUnit(saoLcuParam[1], &saoLcuParamRdo[1]);

    for (typeIdx = 0; typeIdx < MAX_NUM_SAO_TYPE; typeIdx++)
    {
        if (typeIdx == SAO_BO)
        {
            // Estimate Best Position
            for (int compIdx = 0; compIdx < 2; compIdx++)
            {
                double currentRDCost = 0.0;
                bestRDCostTableBo = MAX_DOUBLE;
                estDist[compIdx] = estSaoTypeDist(compIdx + 1, typeIdx, 0, m_chromaLambda, currentDistortionTableBo, currentRdCostTableBo);
                for (int i = 0; i < SAO_MAX_BO_CLASSES - SAO_BO_LEN + 1; i++)
                {
                    currentRDCost = 0.0;
                    for (int j = i; j < i + SAO_BO_LEN; j++)
                        currentRDCost += currentRdCostTableBo[j];

                    if (currentRDCost < bestRDCostTableBo)
                    {
                        bestRDCostTableBo = currentRDCost;
                        bestClassTableBo[compIdx]  = i;
                    }
                }

                // Re code all Offsets
                // Code Center
                estDist[compIdx] = 0;
                for (classIdx = bestClassTableBo[compIdx]; classIdx < bestClassTableBo[compIdx] + SAO_BO_LEN; classIdx++)
                    estDist[compIdx] += currentDistortionTableBo[classIdx];
            }
        }
        else
        {
            estDist[0] = estSaoTypeDist(1, typeIdx, 0, m_chromaLambda, currentDistortionTableBo, currentRdCostTableBo);
            estDist[1] = estSaoTypeDist(2, typeIdx, 0, m_chromaLambda, currentDistortionTableBo, currentRdCostTableBo);
        }

        m_entropyCoder.load(m_rdEntropyCoders[0][CI_TEMP_BEST]);
        m_entropyCoder.resetBits();

        for (int compIdx = 0; compIdx < 2; compIdx++)
        {
            resetSaoUnit(&saoLcuParamRdo[compIdx]);
            saoLcuParamRdo[compIdx].length = s_numClass[typeIdx];
            saoLcuParamRdo[compIdx].typeIdx = typeIdx;
            saoLcuParamRdo[compIdx].mergeLeftFlag = 0;
            saoLcuParamRdo[compIdx].mergeUpFlag   = 0;
            saoLcuParamRdo[compIdx].subTypeIdx = (typeIdx == SAO_BO) ? bestClassTableBo[compIdx] : 0;
            for (classIdx = 0; classIdx < saoLcuParamRdo[compIdx].length; classIdx++)
                saoLcuParamRdo[compIdx].offset[classIdx] = (int)m_offset[compIdx + 1][typeIdx][classIdx + saoLcuParamRdo[compIdx].subTypeIdx + 1];

            m_entropyCoder.codeSaoOffset(&saoLcuParamRdo[compIdx], compIdx + 1);
        }

        estRate = m_entropyCoder.getNumberOfWrittenBits();
        m_cost[1][typeIdx] = (double)((double)(estDist[0] + estDist[1]) + m_chromaLambda * (double)estRate);

        if (m_cost[1][typeIdx] < costPartBest)
        {
            costPartBest = m_cost[1][typeIdx];
            copySaoUnit(saoLcuParam[0], &saoLcuParamRdo[0]);
            copySaoUnit(saoLcuParam[1], &saoLcuParamRdo[1]);
            bestDist = (estDist[0] + estDist[1]);
        }
    }

    distortion[0] += ((double)bestDist / m_chromaLambda);
    m_entropyCoder.load(m_rdEntropyCoders[0][CI_TEMP_BEST]);
    m_entropyCoder.codeSaoOffset(saoLcuParam[0], 1);
    m_entropyCoder.codeSaoOffset(saoLcuParam[1], 2);
    m_entropyCoder.store(m_rdEntropyCoders[0][CI_TEMP_BEST]);

    // merge left or merge up

    for (int idxNeighbor = 0; idxNeighbor < 2; idxNeighbor++)
    {
        for (int compIdx = 0; compIdx < 2; compIdx++)
        {
            saoLcuParamNeighbor[compIdx] = NULL;
            if (allowMergeLeft && addrLeft >= 0 && idxNeighbor == 0)
                saoLcuParamNeighbor[compIdx] = &(saoParam->saoLcuParam[compIdx + 1][addrLeft]);
            else if (allowMergeUp && addrUp >= 0 && idxNeighbor == 1)
                saoLcuParamNeighbor[compIdx] = &(saoParam->saoLcuParam[compIdx + 1][addrUp]);
            if (saoLcuParamNeighbor[compIdx] != NULL)
            {
                estDist[compIdx] = 0;
                typeIdx = saoLcuParamNeighbor[compIdx]->typeIdx;
                if (typeIdx >= 0)
                {
                    int mergeBandPosition = (typeIdx == SAO_BO) ? saoLcuParamNeighbor[compIdx]->subTypeIdx : 0;
                    for (classIdx = 0; classIdx < s_numClass[typeIdx]; classIdx++)
                    {
                        int mergeOffset = saoLcuParamNeighbor[compIdx]->offset[classIdx];
                        estDist[compIdx] += estSaoDist(m_count[compIdx + 1][typeIdx][classIdx + mergeBandPosition + 1], mergeOffset, m_offsetOrg[compIdx + 1][typeIdx][classIdx + mergeBandPosition + 1],  0);
                    }
                }
                else
                    estDist[compIdx] = 0;

                copySaoUnit(saoMergeParam[compIdx][idxNeighbor], saoLcuParamNeighbor[compIdx]);
                saoMergeParam[compIdx][idxNeighbor]->mergeUpFlag   = !!idxNeighbor;
                saoMergeParam[compIdx][idxNeighbor]->mergeLeftFlag = !idxNeighbor;
                distortion[idxNeighbor + 1] += ((double)estDist[compIdx] / m_chromaLambda);
            }
        }
    }
}

static void restoreOrigLosslessYuv(TComDataCU* cu, uint32_t absZOrderIdx, uint32_t depth);

/* Original Lossless YUV LF disable process */
void restoreLFDisabledOrigYuv(Frame* pic)
{
    if (pic->m_picSym->m_slice->m_pps->bTransquantBypassEnabled)
    {
        for (uint32_t cuAddr = 0; cuAddr < pic->getNumCUsInFrame(); cuAddr++)
        {
            TComDataCU* cu = pic->getCU(cuAddr);

            origCUSampleRestoration(cu, 0, 0);
        }
    }
}

/* Original YUV restoration for CU in lossless coding */
void origCUSampleRestoration(TComDataCU* cu, uint32_t absZOrderIdx, uint32_t depth)
{
    // go to sub-CU
    if (cu->getDepth(absZOrderIdx) > depth)
    {
        Frame* pic = cu->m_pic;
        uint32_t curNumParts = pic->getNumPartInCU() >> (depth << 1);
        uint32_t qNumParts   = curNumParts >> 2;
        uint32_t xmax = cu->m_slice->m_sps->picWidthInLumaSamples  - cu->getCUPelX();
        uint32_t ymax = cu->m_slice->m_sps->picHeightInLumaSamples - cu->getCUPelY();
        for (uint32_t partIdx = 0; partIdx < 4; partIdx++, absZOrderIdx += qNumParts)
        {
            if (g_zscanToPelX[absZOrderIdx] < xmax && g_zscanToPelY[absZOrderIdx] < ymax)
                origCUSampleRestoration(cu, absZOrderIdx, depth + 1);
        }

        return;
    }

    // restore original YUV samples
    if (cu->isLosslessCoded(absZOrderIdx))
        restoreOrigLosslessYuv(cu, absZOrderIdx, depth);
}

/* Original Lossless YUV sample restoration */
static void restoreOrigLosslessYuv(TComDataCU* cu, uint32_t absZOrderIdx, uint32_t depth)
{
    TComPicYuv* pcPicYuvRec = cu->m_pic->getPicYuvRec();
    int hChromaShift = cu->getHorzChromaShift();
    int vChromaShift = cu->getVertChromaShift();
    uint32_t lumaOffset   = absZOrderIdx << LOG2_UNIT_SIZE * 2;
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
