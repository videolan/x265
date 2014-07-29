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

/**
 \file     TEncSampleAdaptiveOffset.cpp
 \brief       estimation part of sample adaptive offset class
 */

#include "TEncSampleAdaptiveOffset.h"
#include "common.h"

using namespace x265;

TEncSampleAdaptiveOffset::TEncSampleAdaptiveOffset()
    : m_entropyCoder(NULL)
    , m_count(NULL)
    , m_offset(NULL)
    , m_offsetOrg(NULL)
    , m_countPreDblk(NULL)
    , m_offsetOrgPreDblk(NULL)
    , m_rate(NULL)
    , m_dist(NULL)
    , m_cost(NULL)
    , m_costPartBest(NULL)
    , m_distOrg(NULL)
    , m_typePartBest(NULL)
    , lumaLambda(0.)
    , chromaLambda(0.)
    , depth(0)
{
    m_depthSaoRate[0][0] = 0;
    m_depthSaoRate[0][1] = 0;
    m_depthSaoRate[0][2] = 0;
    m_depthSaoRate[0][3] = 0;
    m_depthSaoRate[1][0] = 0;
    m_depthSaoRate[1][1] = 0;
    m_depthSaoRate[1][2] = 0;
    m_depthSaoRate[1][3] = 0;

    m_saoBitIncreaseY = X265_MAX(X265_DEPTH - 10, 0);
    m_saoBitIncreaseC = X265_MAX(X265_DEPTH - 10, 0);
    m_offsetThY = 1 << X265_MIN(X265_DEPTH - 5, 5);
    m_offsetThC = 1 << X265_MIN(X265_DEPTH - 5, 5);
}

TEncSampleAdaptiveOffset::~TEncSampleAdaptiveOffset()
{}

#if HIGH_BIT_DEPTH
inline double roundIDBI2(double x)
{
    return ((x) > 0) ? (int)(((int)(x) + (1 << (X265_DEPTH - 8 - 1))) / (1 << (X265_DEPTH - 8))) : ((int)(((int)(x) - (1 << (X265_DEPTH - 8 - 1))) / (1 << (X265_DEPTH - 8))));
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

/** process SAO for one partition */
void TEncSampleAdaptiveOffset::rdoSaoOnePart(SAOQTPart *psQTPart, int partIdx, double lambda, int yCbCr)
{
    int typeIdx;
    int numTotalType = MAX_NUM_SAO_TYPE;
    SAOQTPart* onePart = &(psQTPart[partIdx]);

    int64_t estDist;
    int classIdx;
    int shift = 2 * DISTORTION_PRECISION_ADJUSTMENT(X265_DEPTH - 8);

    m_distOrg[partIdx] =  0;

    double bestRDCostTableBo = MAX_DOUBLE;
    int    bestClassTableBo  = 0;
    int    currentDistortionTableBo[MAX_NUM_SAO_CLASS];
    double currentRdCostTableBo[MAX_NUM_SAO_CLASS];

    int allowMergeLeft;
    int allowMergeUp;
    SaoLcuParam saoLcuParamRdo;

    for (typeIdx = -1; typeIdx < numTotalType; typeIdx++)
    {
        m_entropyCoder->load(m_rdEntropyCoders[onePart->partLevel][CI_CURR_BEST]);
        m_entropyCoder->resetBits();

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
                    {
                        saoLcuParamRdo.mergeUpFlag = 0;
                    }

                    if (rx == onePart->startCUX)
                        saoLcuParamRdo.mergeLeftFlag = 0;

                    m_entropyCoder->codeSaoUnitInterleaving(yCbCr, 1, rx, ry,  &saoLcuParamRdo, 1,  1,  allowMergeLeft, allowMergeUp);
                }
            }
        }

        if (typeIdx >= 0)
        {
            estDist = estSaoTypeDist(partIdx, typeIdx, shift, lambda, currentDistortionTableBo, currentRdCostTableBo);
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
                for (classIdx = bestClassTableBo; classIdx < bestClassTableBo + SAO_BO_LEN; classIdx++)
                    estDist += currentDistortionTableBo[classIdx];
            }

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

                    // set type and offsets
                    saoLcuParamRdo.typeIdx = typeIdx;
                    saoLcuParamRdo.subTypeIdx = (typeIdx == SAO_BO) ? bestClassTableBo : 0;
                    saoLcuParamRdo.length = m_numClass[typeIdx];
                    for (classIdx = 0; classIdx < saoLcuParamRdo.length; classIdx++)
                        saoLcuParamRdo.offset[classIdx] = (int)m_offset[partIdx][typeIdx][classIdx + saoLcuParamRdo.subTypeIdx + 1];

                    m_entropyCoder->codeSaoUnitInterleaving(yCbCr, 1, rx, ry, &saoLcuParamRdo, 1, 1, allowMergeLeft, allowMergeUp);
                }
            }

            m_dist[partIdx][typeIdx] = estDist;
            m_rate[partIdx][typeIdx] = m_entropyCoder->getNumberOfWrittenBits();

            m_cost[partIdx][typeIdx] = (double)((double)m_dist[partIdx][typeIdx] + lambda * (double)m_rate[partIdx][typeIdx]);

            if (m_cost[partIdx][typeIdx] < m_costPartBest[partIdx])
            {
                m_distOrg[partIdx] = 0;
                m_costPartBest[partIdx] = m_cost[partIdx][typeIdx];
                m_typePartBest[partIdx] = typeIdx;
                m_entropyCoder->store(m_rdEntropyCoders[onePart->partLevel][CI_TEMP_BEST]);
            }
        }
        else
        {
            if (m_distOrg[partIdx] < m_costPartBest[partIdx])
            {
                m_costPartBest[partIdx] = (double)m_distOrg[partIdx] + m_entropyCoder->getNumberOfWrittenBits() * lambda;
                m_typePartBest[partIdx] = -1;
                m_entropyCoder->store(m_rdEntropyCoders[onePart->partLevel][CI_TEMP_BEST]);
            }
        }
    }

    onePart->bProcessed = true;
    onePart->bSplit    = false;
    onePart->minDist   =       m_typePartBest[partIdx] >= 0 ? m_dist[partIdx][m_typePartBest[partIdx]] : m_distOrg[partIdx];
    onePart->minRate   = (int)(m_typePartBest[partIdx] >= 0 ? m_rate[partIdx][m_typePartBest[partIdx]] : 0);
    onePart->minCost   = onePart->minDist + lambda * onePart->minRate;
    onePart->bestType  = m_typePartBest[partIdx];

    if (onePart->bestType != -1)
    {
        onePart->length = m_numClass[onePart->bestType];
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
        onePart->length     = 0;
}

/* Run partition tree disable */
void TEncSampleAdaptiveOffset::disablePartTree(SAOQTPart *psQTPart, int partIdx)
{
    SAOQTPart* pOnePart = &(psQTPart[partIdx]);

    pOnePart->bSplit   = false;
    pOnePart->length   =  0;
    pOnePart->bestType = -1;

    if (pOnePart->partLevel < (int)m_maxSplitLevel)
    {
        for (int i = 0; i < NUM_DOWN_PART; i++)
            disablePartTree(psQTPart, pOnePart->downPartsIdx[i]);
    }
}

/** Run quadtree decision function */
void TEncSampleAdaptiveOffset::runQuadTreeDecision(SAOQTPart *qtPart, int partIdx, double &costFinal, int maxLevel, double lambda, int yCbCr)
{
    SAOQTPart* onePart = &(qtPart[partIdx]);

    uint32_t nextDepth = onePart->partLevel + 1;

    if (!partIdx)
        costFinal = 0;

    // SAO for this part
    if (!onePart->bProcessed)
        rdoSaoOnePart(qtPart, partIdx, lambda, yCbCr);

    // SAO for sub 4 parts
    if (onePart->partLevel < maxLevel)
    {
        double costNotSplit = lambda + onePart->minCost;
        double costSplit    = lambda;

        for (int i = 0; i < NUM_DOWN_PART; i++)
        {
            if (i) //initialize RD with previous depth buffer
                m_rdEntropyCoders[nextDepth][CI_CURR_BEST].load(m_rdEntropyCoders[nextDepth][CI_NEXT_BEST]);
            else
                m_rdEntropyCoders[nextDepth][CI_CURR_BEST].load(m_rdEntropyCoders[onePart->partLevel][CI_CURR_BEST]);

            runQuadTreeDecision(qtPart, onePart->downPartsIdx[i], costFinal, maxLevel, lambda, yCbCr);
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
            for (int i = 0; i < NUM_DOWN_PART; i++)
                disablePartTree(qtPart, onePart->downPartsIdx[i]);

            m_rdEntropyCoders[onePart->partLevel][CI_NEXT_BEST].load(m_rdEntropyCoders[onePart->partLevel][CI_TEMP_BEST]);
        }
    }
    else
        costFinal = onePart->minCost;
}

/** delete allocated memory of TEncSampleAdaptiveOffset class.
 */
void TEncSampleAdaptiveOffset::destroyEncBuffer()
{
    for (int i = 0; i < m_numTotalParts; i++)
    {
        for (int j = 0; j < MAX_NUM_SAO_TYPE; j++)
        {
            X265_FREE(m_count[i][j]);
            X265_FREE(m_offset[i][j]);
            X265_FREE(m_offsetOrg[i][j]);
        }

        X265_FREE(m_rate[i]);
        X265_FREE(m_dist[i]);
        X265_FREE(m_cost[i]);
        X265_FREE(m_count[i]);
        X265_FREE(m_offset[i]);
        X265_FREE(m_offsetOrg[i]);
    }

    X265_FREE(m_distOrg);
    m_distOrg = NULL;
    X265_FREE(m_costPartBest);
    m_costPartBest = NULL;
    X265_FREE(m_typePartBest);
    m_typePartBest = NULL;
    X265_FREE(m_rate);
    m_rate = NULL;
    X265_FREE(m_dist);
    m_dist = NULL;
    X265_FREE(m_cost);
    m_cost = NULL;
    X265_FREE(m_count);
    m_count = NULL;
    X265_FREE(m_offset);
    m_offset = NULL;
    X265_FREE(m_offsetOrg);
    m_offsetOrg = NULL;

    delete[] m_countPreDblk;
    m_countPreDblk = NULL;

    delete[] m_offsetOrgPreDblk;
    m_offsetOrgPreDblk = NULL;
}

/* create Encoder Buffer for SAO */
void TEncSampleAdaptiveOffset::createEncBuffer()
{
    m_distOrg = X265_MALLOC(int64_t, m_numTotalParts);
    m_costPartBest = X265_MALLOC(double, m_numTotalParts);
    m_typePartBest = X265_MALLOC(int, m_numTotalParts);

    m_rate = X265_MALLOC(int64_t*, m_numTotalParts);
    m_dist = X265_MALLOC(int64_t*, m_numTotalParts);
    m_cost = X265_MALLOC(double*, m_numTotalParts);

    m_count  = X265_MALLOC(int64_t * *, m_numTotalParts);
    m_offset = X265_MALLOC(int64_t * *, m_numTotalParts);
    m_offsetOrg = X265_MALLOC(int64_t * *, m_numTotalParts);

    for (int i = 0; i < m_numTotalParts; i++)
    {
        m_rate[i] = X265_MALLOC(int64_t, MAX_NUM_SAO_TYPE);
        m_dist[i] = X265_MALLOC(int64_t, MAX_NUM_SAO_TYPE);
        m_cost[i] = X265_MALLOC(double, MAX_NUM_SAO_TYPE);

        m_count[i] = X265_MALLOC(int64_t*, MAX_NUM_SAO_TYPE);
        m_offset[i] = X265_MALLOC(int64_t*, MAX_NUM_SAO_TYPE);
        m_offsetOrg[i] = X265_MALLOC(int64_t*, MAX_NUM_SAO_TYPE);

        for (int j = 0; j < MAX_NUM_SAO_TYPE; j++)
        {
            m_count[i][j] = X265_MALLOC(int64_t, MAX_NUM_SAO_CLASS);
            m_offset[i][j] = X265_MALLOC(int64_t, MAX_NUM_SAO_CLASS);
            m_offsetOrg[i][j] = X265_MALLOC(int64_t, MAX_NUM_SAO_CLASS);
        }
    }

    if (!m_countPreDblk)
    {
        int numLcu = m_numCuInWidth * m_numCuInHeight;
        m_countPreDblk = new int64_t[numLcu][3][MAX_NUM_SAO_TYPE][MAX_NUM_SAO_CLASS];
        m_offsetOrgPreDblk = new int64_t[numLcu][3][MAX_NUM_SAO_TYPE][MAX_NUM_SAO_CLASS];
    }
}

/* Start SAO encoder */
void TEncSampleAdaptiveOffset::startSaoEnc(Frame* pic, Entropy* entropyCoder)
{
    m_pic = pic;

    m_entropyCoder = entropyCoder;
    m_entropyCoder->resetEntropy(pic->m_picSym->m_slice);
    m_entropyCoder->resetBits();
    m_entropyCoder->store(m_rdEntropyCoders[0][CI_NEXT_BEST]);
    m_rdEntropyCoders[0][CI_CURR_BEST].load(m_rdEntropyCoders[0][CI_NEXT_BEST]);
}

/* End SAO encoder */
void TEncSampleAdaptiveOffset::endSaoEnc()
{
    m_pic = NULL;
}

inline int signOf(int x)
{
    return (x >> 31) | ((int)((((uint32_t)-x)) >> 31));
}

/* Calculate SAO statistics for current LCU without non-crossing slice */
void TEncSampleAdaptiveOffset::calcSaoStatsCu(int addr, int partIdx, int yCbCr)
{
    int x, y;
    TComDataCU *cu = m_pic->getCU(addr);

    pixel* fenc;
    pixel* recon;
    int stride;
    int lcuHeight = g_maxCUSize;
    int lcuWidth  = g_maxCUSize;
    uint32_t lpelx = cu->getCUPelX();
    uint32_t tpely = cu->getCUPelY();
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
    pixel* pTableBo = (yCbCr == 0) ? m_lumaTableBo : m_chromaTableBo;

    int iIsChroma = (yCbCr != 0) ? 1 : 0;
    int numSkipLine = iIsChroma ? 4 - (2 * m_vChromaShift) : 4;

    if (m_saoLcuBasedOptimization == 0)
        numSkipLine = 0;

    int numSkipLineRight = iIsChroma ? 5 - (2 * m_hChromaShift) : 5;

    if (m_saoLcuBasedOptimization == 0)
        numSkipLineRight = 0;

    picWidthTmp  = (iIsChroma == 0) ? m_picWidth  : m_picWidth  >> m_hChromaShift;
    picHeightTmp = (iIsChroma == 0) ? m_picHeight : m_picHeight >> m_vChromaShift;
    lcuWidth     = (iIsChroma == 0) ? lcuWidth    : lcuWidth   >> m_hChromaShift;
    lcuHeight    = (iIsChroma == 0) ? lcuHeight   : lcuHeight  >> m_vChromaShift;
    lpelx        = (iIsChroma == 0) ? lpelx       : lpelx       >> m_hChromaShift;
    tpely        = (iIsChroma == 0) ? tpely       : tpely       >> m_vChromaShift;

    rpelx     = lpelx + lcuWidth;
    bpely     = tpely + lcuHeight;
    rpelx     = rpelx > picWidthTmp  ? picWidthTmp  : rpelx;
    bpely     = bpely > picHeightTmp ? picHeightTmp : bpely;
    lcuWidth  = rpelx - lpelx;
    lcuHeight = bpely - tpely;
    stride    =  (yCbCr == 0) ? m_pic->getStride() : m_pic->getCStride();

    //if(iSaoType == BO_0 || iSaoType == BO_1)
    {
        if (m_saoLcuBasedOptimization && m_saoLcuBoundary)
        {
            numSkipLine      = iIsChroma ? 3 - (2 * m_vChromaShift) : 3;
            numSkipLineRight = iIsChroma ? 4 - (2 * m_hChromaShift) : 4;
        }
        stats = m_offsetOrg[partIdx][SAO_BO];
        counts = m_count[partIdx][SAO_BO];

        fenc = getPicYuvAddr(m_pic->getPicYuvOrg(), yCbCr, addr);
        recon = getPicYuvAddr(m_pic->getPicYuvRec(), yCbCr, addr);

        endX = (rpelx == picWidthTmp) ? lcuWidth : lcuWidth - numSkipLineRight;
        endY = (bpely == picHeightTmp) ? lcuHeight : lcuHeight - numSkipLine;
        for (y = 0; y < endY; y++)
        {
            for (x = 0; x < endX; x++)
            {
                classIdx = pTableBo[recon[x]];
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

    //if (iSaoType == EO_0  || iSaoType == EO_1 || iSaoType == EO_2 || iSaoType == EO_3)
    {
        //if (iSaoType == EO_0)
        {
            if (m_saoLcuBasedOptimization && m_saoLcuBoundary)
            {
                numSkipLine      = iIsChroma ? 3 - (2 * m_vChromaShift) : 3;
                numSkipLineRight = iIsChroma ? 5 - (2 * m_hChromaShift) : 5;
            }
            stats = m_offsetOrg[partIdx][SAO_EO_0];
            counts = m_count[partIdx][SAO_EO_0];

            fenc = getPicYuvAddr(m_pic->getPicYuvOrg(), yCbCr, addr);
            recon = getPicYuvAddr(m_pic->getPicYuvRec(), yCbCr, addr);

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

                    stats[m_eoTable[edgeType]] += (fenc[x] - recon[x]);
                    counts[m_eoTable[edgeType]]++;
                }

                fenc += stride;
                recon += stride;
            }
        }

        //if (iSaoType == EO_1)
        {
            if (m_saoLcuBasedOptimization && m_saoLcuBoundary)
            {
                numSkipLine      = iIsChroma ? 4 - (2 * m_vChromaShift) : 4;
                numSkipLineRight = iIsChroma ? 4 - (2 * m_hChromaShift) : 4;
            }
            stats = m_offsetOrg[partIdx][SAO_EO_1];
            counts = m_count[partIdx][SAO_EO_1];

            fenc = getPicYuvAddr(m_pic->getPicYuvOrg(), yCbCr, addr);
            recon = getPicYuvAddr(m_pic->getPicYuvRec(), yCbCr, addr);

            startY = (tpely == 0) ? 1 : 0;
            endX   = (rpelx == picWidthTmp) ? lcuWidth : lcuWidth - numSkipLineRight;
            endY   = (bpely == picHeightTmp) ? lcuHeight - 1 : lcuHeight - numSkipLine;
            if (tpely == 0)
            {
                fenc += stride;
                recon += stride;
            }

            for (x = 0; x < lcuWidth; x++)
                m_upBuff1[x] = signOf(recon[x] - recon[x - stride]);

            for (y = startY; y < endY; y++)
            {
                for (x = 0; x < endX; x++)
                {
                    signDown = signOf(recon[x] - recon[x + stride]);
                    edgeType = signDown + m_upBuff1[x] + 2;
                    m_upBuff1[x] = -signDown;

                    stats[m_eoTable[edgeType]] += (fenc[x] - recon[x]);
                    counts[m_eoTable[edgeType]]++;
                }

                fenc += stride;
                recon += stride;
            }
        }
        //if (iSaoType == EO_2)
        {
            if (m_saoLcuBasedOptimization && m_saoLcuBoundary)
            {
                numSkipLine      = iIsChroma ? 4 - (2 * m_vChromaShift) : 4;
                numSkipLineRight = iIsChroma ? 5 - (2 * m_hChromaShift) : 5;
            }
            stats = m_offsetOrg[partIdx][SAO_EO_2];
            counts = m_count[partIdx][SAO_EO_2];

            fenc = getPicYuvAddr(m_pic->getPicYuvOrg(), yCbCr, addr);
            recon = getPicYuvAddr(m_pic->getPicYuvRec(), yCbCr, addr);

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
                m_upBuff1[x] = signOf(recon[x] - recon[x - stride - 1]);

            for (y = startY; y < endY; y++)
            {
                signDown2 = signOf(recon[stride + startX] - recon[startX - 1]);
                for (x = startX; x < endX; x++)
                {
                    signDown1 = signOf(recon[x] - recon[x + stride + 1]);
                    edgeType  = signDown1 + m_upBuff1[x] + 2;
                    m_upBufft[x + 1] = -signDown1;
                    stats[m_eoTable[edgeType]] += (fenc[x] - recon[x]);
                    counts[m_eoTable[edgeType]]++;
                }

                m_upBufft[startX] = signDown2;
                std::swap(m_upBuff1, m_upBufft);

                recon += stride;
                fenc += stride;
            }
        }
        //if (iSaoType == EO_3)
        {
            if (m_saoLcuBasedOptimization && m_saoLcuBoundary)
            {
                numSkipLine      = iIsChroma ? 4 - (2 * m_vChromaShift) : 4;
                numSkipLineRight = iIsChroma ? 5 - (2 * m_hChromaShift) : 5;
            }
            stats = m_offsetOrg[partIdx][SAO_EO_3];
            counts = m_count[partIdx][SAO_EO_3];

            fenc = getPicYuvAddr(m_pic->getPicYuvOrg(), yCbCr, addr);
            recon = getPicYuvAddr(m_pic->getPicYuvRec(), yCbCr, addr);

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
                m_upBuff1[x] = signOf(recon[x] - recon[x - stride + 1]);

            for (y = startY; y < endY; y++)
            {
                for (x = startX; x < endX; x++)
                {
                    signDown1 = signOf(recon[x] - recon[x + stride - 1]);
                    edgeType  = signDown1 + m_upBuff1[x] + 2;
                    m_upBuff1[x - 1] = -signDown1;
                    stats[m_eoTable[edgeType]] += (fenc[x] - recon[x]);
                    counts[m_eoTable[edgeType]]++;
                }

                m_upBuff1[endX - 1] = signOf(recon[endX - 1 + stride] - recon[endX]);

                recon += stride;
                fenc += stride;
            }
        }
    }
}

void TEncSampleAdaptiveOffset::calcSaoStatsCu_BeforeDblk(Frame* pic, int idxX, int idxY)
{
    int addr;
    int x, y;

    pixel* fenc;
    pixel* recon;
    int stride;
    int lcuHeight;
    int lcuWidth;
    uint32_t rPelX;
    uint32_t bPelY;
    int64_t* stats;
    int64_t* count;
    uint32_t picWidthTmp = 0;
    uint32_t picHeightTmp = 0;
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
    pixel* pTableBo;

    // NOTE: Row
    {
        // NOTE: Col
        {
            lcuHeight = g_maxCUSize;
            lcuWidth  = g_maxCUSize;
            addr    = idxX + frameWidthInCU * idxY;
            cu      = pic->getCU(addr);
            lPelX   = cu->getCUPelX();
            tPelY   = cu->getCUPelY();

            memset(m_countPreDblk[addr], 0, 3 * MAX_NUM_SAO_TYPE * MAX_NUM_SAO_CLASS * sizeof(int64_t));
            memset(m_offsetOrgPreDblk[addr], 0, 3 * MAX_NUM_SAO_TYPE * MAX_NUM_SAO_CLASS * sizeof(int64_t));
            for (int yCbCr = 0; yCbCr < 3; yCbCr++)
            {
                isChroma = (yCbCr != 0) ? 1 : 0;

                if (yCbCr == 0)
                {
                    picWidthTmp  = m_picWidth;
                    picHeightTmp = m_picHeight;
                }
                else if (yCbCr == 1)
                {
                    picWidthTmp  = m_picWidth  >> isChroma;
                    picHeightTmp = m_picHeight >> isChroma;
                    lcuWidth     = lcuWidth    >> isChroma;
                    lcuHeight    = lcuHeight   >> isChroma;
                    lPelX        = lPelX       >> isChroma;
                    tPelY        = tPelY       >> isChroma;
                }
                rPelX     = lPelX + lcuWidth;
                bPelY     = tPelY + lcuHeight;
                rPelX     = rPelX > picWidthTmp  ? picWidthTmp  : rPelX;
                bPelY     = bPelY > picHeightTmp ? picHeightTmp : bPelY;
                lcuWidth  = rPelX - lPelX;
                lcuHeight = bPelY - tPelY;

                stride   = (yCbCr == 0) ? pic->getStride() : pic->getCStride();
                pTableBo = (yCbCr == 0) ? m_lumaTableBo : m_chromaTableBo;

                //if(iSaoType == BO)

                numSkipLine = isChroma ? 1 : 3;
                numSkipLineRight = isChroma ? 2 : 4;

                stats = m_offsetOrgPreDblk[addr][yCbCr][SAO_BO];
                count = m_countPreDblk[addr][yCbCr][SAO_BO];

                fenc = getPicYuvAddr(pic->getPicYuvOrg(), yCbCr, addr);
                recon = getPicYuvAddr(pic->getPicYuvRec(), yCbCr, addr);

                startX = (rPelX == picWidthTmp) ? lcuWidth : lcuWidth - numSkipLineRight;
                startY = (bPelY == picHeightTmp) ? lcuHeight : lcuHeight - numSkipLine;

                for (y = 0; y < lcuHeight; y++)
                {
                    for (x = 0; x < lcuWidth; x++)
                    {
                        if (x < startX && y < startY)
                            continue;

                        classIdx = pTableBo[recon[x]];
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

                stats = m_offsetOrgPreDblk[addr][yCbCr][SAO_EO_0];
                count = m_countPreDblk[addr][yCbCr][SAO_EO_0];

                fenc = getPicYuvAddr(pic->getPicYuvOrg(), yCbCr, addr);
                recon = getPicYuvAddr(pic->getPicYuvRec(), yCbCr, addr);

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

                        stats[m_eoTable[edgeType]] += (fenc[x] - recon[x]);
                        count[m_eoTable[edgeType]]++;
                    }

                    fenc += stride;
                    recon += stride;
                }

                //if (iSaoType == EO_1)

                numSkipLine = isChroma ? 2 : 4;
                numSkipLineRight = isChroma ? 2 : 4;

                stats = m_offsetOrgPreDblk[addr][yCbCr][SAO_EO_1];
                count = m_countPreDblk[addr][yCbCr][SAO_EO_1];

                fenc = getPicYuvAddr(pic->getPicYuvOrg(), yCbCr, addr);
                recon = getPicYuvAddr(pic->getPicYuvRec(), yCbCr, addr);

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
                    m_upBuff1[x] = signOf(recon[x] - recon[x - stride]);

                for (y = firstY; y < endY; y++)
                {
                    for (x = 0; x < lcuWidth; x++)
                    {
                        signDown = signOf(recon[x] - recon[x + stride]);
                        edgeType = signDown + m_upBuff1[x] + 2;
                        m_upBuff1[x] = -signDown;

                        if (x < startX && y < startY)
                            continue;

                        stats[m_eoTable[edgeType]] += (fenc[x] - recon[x]);
                        count[m_eoTable[edgeType]]++;
                    }

                    fenc += stride;
                    recon += stride;
                }

                //if (iSaoType == EO_2)

                numSkipLine = isChroma ? 2 : 4;
                numSkipLineRight = isChroma ? 3 : 5;

                stats = m_offsetOrgPreDblk[addr][yCbCr][SAO_EO_2];
                count = m_countPreDblk[addr][yCbCr][SAO_EO_2];

                fenc = getPicYuvAddr(pic->getPicYuvOrg(), yCbCr, addr);
                recon = getPicYuvAddr(pic->getPicYuvRec(), yCbCr, addr);

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
                    m_upBuff1[x] = signOf(recon[x] - recon[x - stride - 1]);

                for (y = firstY; y < endY; y++)
                {
                    signDown2 = signOf(recon[stride + startX] - recon[startX - 1]);
                    for (x = firstX; x < endX; x++)
                    {
                        signDown1 = signOf(recon[x] - recon[x + stride + 1]);
                        edgeType = signDown1 + m_upBuff1[x] + 2;
                        m_upBufft[x + 1] = -signDown1;

                        if (x < startX && y < startY)
                            continue;

                        stats[m_eoTable[edgeType]] += (fenc[x] - recon[x]);
                        count[m_eoTable[edgeType]]++;
                    }

                    m_upBufft[firstX] = signDown2;
                    std::swap(m_upBuff1, m_upBufft);

                    recon += stride;
                    fenc += stride;
                }

                //if (iSaoType == EO_3)

                numSkipLine = isChroma ? 2 : 4;
                numSkipLineRight = isChroma ? 3 : 5;

                stats = m_offsetOrgPreDblk[addr][yCbCr][SAO_EO_3];
                count = m_countPreDblk[addr][yCbCr][SAO_EO_3];

                fenc = getPicYuvAddr(pic->getPicYuvOrg(), yCbCr, addr);
                recon = getPicYuvAddr(pic->getPicYuvRec(), yCbCr, addr);

                startX   = (rPelX == picWidthTmp) ? lcuWidth - 1 : lcuWidth - numSkipLineRight;
                startY   = (bPelY == picHeightTmp) ? lcuHeight - 1 : lcuHeight - numSkipLine;
                firstX   = (lPelX == 0) ? 1 : 0;
                firstY = (tPelY == 0) ? 1 : 0;
                endX   = (rPelX == picWidthTmp) ? lcuWidth - 1 : lcuWidth;
                endY   = (bPelY == picHeightTmp) ? lcuHeight - 1 : lcuHeight;
                if (firstY == 1)
                {
                    fenc += stride;
                    recon += stride;
                }

                for (x = firstX - 1; x < endX; x++)
                    m_upBuff1[x] = signOf(recon[x] - recon[x - stride + 1]);

                for (y = firstY; y < endY; y++)
                {
                    for (x = firstX; x < endX; x++)
                    {
                        signDown1 = signOf(recon[x] - recon[x + stride - 1]);
                        edgeType  = signDown1 + m_upBuff1[x] + 2;
                        m_upBuff1[x - 1] = -signDown1;

                        if (x < startX && y < startY)
                            continue;

                        stats[m_eoTable[edgeType]] += (fenc[x] - recon[x]);
                        count[m_eoTable[edgeType]]++;
                    }

                    m_upBuff1[endX - 1] = signOf(recon[endX - 1 + stride] - recon[endX]);

                    recon += stride;
                    fenc += stride;
                }
            }
        }
    }
}

void TEncSampleAdaptiveOffset::getSaoStats(SAOQTPart *psQTPart, int yCbCr)
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
                calcSaoStatsCu(addr, partIdx, yCbCr);
            }
        }
    }
    else
    {
        for (partIdx = m_numCulPartsLevel[m_maxSplitLevel - 1]; partIdx < m_numCulPartsLevel[m_maxSplitLevel]; partIdx++)
        {
            onePart = &(psQTPart[partIdx]);
            for (lcuIdy = onePart->startCUY; lcuIdy <= onePart->endCUY; lcuIdy++)
            {
                for (lcuIdx = onePart->startCUX; lcuIdx <= onePart->endCUX; lcuIdx++)
                {
                    addr = lcuIdy * frameWidthInCU + lcuIdx;
                    calcSaoStatsCu(addr, partIdx, yCbCr);
                }
            }
        }

        for (levelIdx = m_maxSplitLevel - 1; levelIdx >= 0; levelIdx--)
        {
            partStart = (levelIdx > 0) ? m_numCulPartsLevel[levelIdx - 1] : 0;
            partEnd   = m_numCulPartsLevel[levelIdx];

            for (partIdx = partStart; partIdx < partEnd; partIdx++)
            {
                onePart = &(psQTPart[partIdx]);
                for (i = 0; i < NUM_DOWN_PART; i++)
                {
                    downPartIdx = onePart->downPartsIdx[i];
                    for (typeIdx = 0; typeIdx < numTotalType; typeIdx++)
                    {
                        for (classIdx = 0; classIdx < (typeIdx < SAO_BO ? m_numClass[typeIdx] : SAO_MAX_BO_CLASSES) + 1; classIdx++)
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
void TEncSampleAdaptiveOffset::resetStats()
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
void TEncSampleAdaptiveOffset::SAOProcess(SAOParam *saoParam)
{
    X265_CHECK(!m_saoLcuBasedOptimization, "SAO LCU mode failure\n"); 
    double costFinal = 0;
    saoParam->bSaoFlag[0] = 1;
    saoParam->bSaoFlag[1] = 0;

    getSaoStats(saoParam->saoPart[0], 0);
    runQuadTreeDecision(saoParam->saoPart[0], 0, costFinal, m_maxSplitLevel, lumaLambda, 0);
    saoParam->bSaoFlag[0] = costFinal < 0 ? 1 : 0;

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
void TEncSampleAdaptiveOffset::checkMerge(SaoLcuParam * saoUnitCurr, SaoLcuParam * saoUnitCheck, int dir)
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
void TEncSampleAdaptiveOffset::assignSaoUnitSyntax(SaoLcuParam* saoLcuParam,  SAOQTPart* saoPart, bool &oneUnitFlag)
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

void TEncSampleAdaptiveOffset::rdoSaoUnitRowInit(SAOParam *saoParam)
{
    saoParam->bSaoFlag[0] = true;
    saoParam->bSaoFlag[1] = true;
    saoParam->oneUnitFlag[0] = false;
    saoParam->oneUnitFlag[1] = false;
    saoParam->oneUnitFlag[2] = false;

    numNoSao[0] = 0; // Luma
    numNoSao[1] = 0; // Chroma
    if (depth > 0 && m_depthSaoRate[0][depth - 1] > SAO_ENCODING_RATE)
        saoParam->bSaoFlag[0] = false;
    if (depth > 0 && m_depthSaoRate[1][depth - 1] > SAO_ENCODING_RATE_CHROMA)
        saoParam->bSaoFlag[1] = false;
}

void TEncSampleAdaptiveOffset::rdoSaoUnitRowEnd(SAOParam *saoParam, int numlcus)
{
    if (!saoParam->bSaoFlag[0])
        m_depthSaoRate[0][depth] = 1.0;
    else
        m_depthSaoRate[0][depth] = numNoSao[0] / ((double)numlcus);

    if (!saoParam->bSaoFlag[1])
        m_depthSaoRate[1][depth] = 1.0;
    else
        m_depthSaoRate[1][depth] = numNoSao[1] / ((double)numlcus * 2);
}

void TEncSampleAdaptiveOffset::rdoSaoUnitRow(SAOParam *saoParam, int idxY)
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
        m_entropyCoder->load(m_rdEntropyCoders[0][CI_CURR_BEST]);
        if (allowMergeLeft)
            m_entropyCoder->codeSaoMerge(0);
        if (allowMergeUp)
            m_entropyCoder->codeSaoMerge(0);
        m_entropyCoder->store(m_rdEntropyCoders[0][CI_TEMP_BEST]);
        // reset stats Y, Cb, Cr
        for (compIdx = 0; compIdx < 3; compIdx++)
        {
            for (j = 0; j < MAX_NUM_SAO_TYPE; j++)
            {
                for (k = 0; k < MAX_NUM_SAO_CLASS; k++)
                {
                    m_offset[compIdx][j][k] = 0;
                    if (m_saoLcuBasedOptimization && m_saoLcuBoundary)
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

        saoComponentParamDist(allowMergeLeft, allowMergeUp, saoParam, addr, addrUp, addrLeft, 0, lumaLambda, &mergeSaoParam[0][0], &compDistortion[0]);
        sao2ChromaParamDist(allowMergeLeft, allowMergeUp, saoParam, addr, addrUp, addrLeft, chromaLambda, &mergeSaoParam[1][0], &mergeSaoParam[2][0], &compDistortion[0]);
        if (saoParam->bSaoFlag[0] || saoParam->bSaoFlag[1])
        {
            // Cost of new SAO_params
            m_entropyCoder->load(m_rdEntropyCoders[0][CI_CURR_BEST]);
            m_entropyCoder->resetBits();
            if (allowMergeLeft)
                m_entropyCoder->codeSaoMerge(0);
            if (allowMergeUp)
                m_entropyCoder->codeSaoMerge(0);
            for (compIdx = 0; compIdx < 3; compIdx++)
            {
                if ((compIdx == 0 && saoParam->bSaoFlag[0]) || (compIdx > 0 && saoParam->bSaoFlag[1]))
                    m_entropyCoder->codeSaoOffset(&saoParam->saoLcuParam[compIdx][addr], compIdx);
            }

            rate = m_entropyCoder->getNumberOfWrittenBits();
            bestCost = compDistortion[0] + (double)rate;
            m_entropyCoder->store(m_rdEntropyCoders[0][CI_TEMP_BEST]);

            // Cost of Merge
            for (int mergeUp = 0; mergeUp < 2; ++mergeUp)
            {
                if ((allowMergeLeft && !mergeUp) || (allowMergeUp && mergeUp))
                {
                    m_entropyCoder->load(m_rdEntropyCoders[0][CI_CURR_BEST]);
                    m_entropyCoder->resetBits();
                    if (allowMergeLeft)
                        m_entropyCoder->codeSaoMerge(1 - mergeUp);
                    if (allowMergeUp && (mergeUp == 1))
                        m_entropyCoder->codeSaoMerge(1);

                    rate = m_entropyCoder->getNumberOfWrittenBits();
                    mergeCost = compDistortion[mergeUp + 1] + (double)rate;
                    if (mergeCost < bestCost)
                    {
                        bestCost = mergeCost;
                        m_entropyCoder->store(m_rdEntropyCoders[0][CI_TEMP_BEST]);
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
                numNoSao[0]++;
            if (saoParam->saoLcuParam[1][addr].typeIdx == -1)
                numNoSao[1] += 2;
            m_entropyCoder->load(m_rdEntropyCoders[0][CI_TEMP_BEST]);
            m_entropyCoder->store(m_rdEntropyCoders[0][CI_CURR_BEST]);
        }
    }
}

/** rate distortion optimization of SAO unit */
inline int64_t TEncSampleAdaptiveOffset::estSaoTypeDist(int compIdx, int typeIdx, int shift, double lambda, int32_t *currentDistortionTableBo, double *currentRdCostTableBo)
{
    int64_t estDist = 0;
    int classIdx;
    int saoBitIncrease = (compIdx == 0) ? m_saoBitIncreaseY : m_saoBitIncreaseC;
    int saoOffsetTh = (compIdx == 0) ? m_offsetThY : m_offsetThC;

    for (classIdx = 1; classIdx < ((typeIdx < SAO_BO) ?  m_numClass[typeIdx] + 1 : SAO_MAX_BO_CLASSES + 1); classIdx++)
    {
        if (typeIdx == SAO_BO)
        {
            currentDistortionTableBo[classIdx - 1] = 0;
            currentRdCostTableBo[classIdx - 1] = lambda;
        }
        if (m_count[compIdx][typeIdx][classIdx])
        {
            m_offset[compIdx][typeIdx][classIdx] = (int64_t)roundIDBI((double)(m_offsetOrg[compIdx][typeIdx][classIdx] << (X265_DEPTH - 8)) / (double)(m_count[compIdx][typeIdx][classIdx] << saoBitIncrease));
            m_offset[compIdx][typeIdx][classIdx] = Clip3(-saoOffsetTh + 1, saoOffsetTh - 1, (int)m_offset[compIdx][typeIdx][classIdx]);
            if (typeIdx < 4)
            {
                if (m_offset[compIdx][typeIdx][classIdx] < 0 && classIdx < 3)
                    m_offset[compIdx][typeIdx][classIdx] = 0;
                if (m_offset[compIdx][typeIdx][classIdx] > 0 && classIdx >= 3)
                    m_offset[compIdx][typeIdx][classIdx] = 0;
            }
            m_offset[compIdx][typeIdx][classIdx] = estIterOffset(typeIdx, classIdx, lambda, m_offset[compIdx][typeIdx][classIdx], m_count[compIdx][typeIdx][classIdx], m_offsetOrg[compIdx][typeIdx][classIdx], shift, saoBitIncrease, currentDistortionTableBo, currentRdCostTableBo, saoOffsetTh);
        }
        else
        {
            m_offsetOrg[compIdx][typeIdx][classIdx] = 0;
            m_offset[compIdx][typeIdx][classIdx] = 0;
        }
        if (typeIdx != SAO_BO)
            estDist += estSaoDist(m_count[compIdx][typeIdx][classIdx], m_offset[compIdx][typeIdx][classIdx] << saoBitIncrease, m_offsetOrg[compIdx][typeIdx][classIdx], shift);
    }

    return estDist;
}

inline int64_t TEncSampleAdaptiveOffset::estSaoDist(int64_t count, int64_t offset, int64_t offsetOrg, int shift)
{
    return (count * offset * offset - offsetOrg * offset * 2) >> shift;
}

inline int64_t TEncSampleAdaptiveOffset::estIterOffset(int typeIdx, int classIdx, double lambda, int64_t offsetInput, int64_t count, int64_t offsetOrg, int shift, int bitIncrease, int32_t *currentDistortionTableBo, double *currentRdCostTableBo, int offsetTh)
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

void TEncSampleAdaptiveOffset::saoComponentParamDist(int allowMergeLeft, int allowMergeUp, SAOParam *saoParam, int addr, int addrUp, int addrLeft, int yCbCr, double lambda, SaoLcuParam *compSaoParam, double *compDistortion)
{
    int typeIdx;

    int64_t estDist;
    int classIdx;
    int shift = 2 * DISTORTION_PRECISION_ADJUSTMENT(X265_DEPTH - 8);
    int64_t bestDist;

    SaoLcuParam* saoLcuParam = &(saoParam->saoLcuParam[yCbCr][addr]);
    SaoLcuParam* saoLcuParamNeighbor = NULL;

    resetSaoUnit(saoLcuParam);
    resetSaoUnit(&compSaoParam[0]);
    resetSaoUnit(&compSaoParam[1]);

    double dCostPartBest = MAX_DOUBLE;

    double  bestRDCostTableBo = MAX_DOUBLE;
    int     bestClassTableBo  = 0;
    int     currentDistortionTableBo[MAX_NUM_SAO_CLASS];
    double  currentRdCostTableBo[MAX_NUM_SAO_CLASS];

    SaoLcuParam saoLcuParamRdo;
    double estRate = 0;

    resetSaoUnit(&saoLcuParamRdo);

    m_entropyCoder->load(m_rdEntropyCoders[0][CI_TEMP_BEST]);
    m_entropyCoder->resetBits();
    m_entropyCoder->codeSaoOffset(&saoLcuParamRdo, yCbCr);
    dCostPartBest = m_entropyCoder->getNumberOfWrittenBits() * lambda;
    copySaoUnit(saoLcuParam, &saoLcuParamRdo);
    bestDist = 0;

    for (typeIdx = 0; typeIdx < MAX_NUM_SAO_TYPE; typeIdx++)
    {
        estDist = estSaoTypeDist(yCbCr, typeIdx, shift, lambda, currentDistortionTableBo, currentRdCostTableBo);

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
        saoLcuParamRdo.length = m_numClass[typeIdx];
        saoLcuParamRdo.typeIdx = typeIdx;
        saoLcuParamRdo.mergeLeftFlag = 0;
        saoLcuParamRdo.mergeUpFlag   = 0;
        saoLcuParamRdo.subTypeIdx = (typeIdx == SAO_BO) ? bestClassTableBo : 0;
        for (classIdx = 0; classIdx < saoLcuParamRdo.length; classIdx++)
            saoLcuParamRdo.offset[classIdx] = (int)m_offset[yCbCr][typeIdx][classIdx + saoLcuParamRdo.subTypeIdx + 1];

        m_entropyCoder->load(m_rdEntropyCoders[0][CI_TEMP_BEST]);
        m_entropyCoder->resetBits();
        m_entropyCoder->codeSaoOffset(&saoLcuParamRdo, yCbCr);

        estRate = m_entropyCoder->getNumberOfWrittenBits();
        m_cost[yCbCr][typeIdx] = (double)((double)estDist + lambda * (double)estRate);

        if (m_cost[yCbCr][typeIdx] < dCostPartBest)
        {
            dCostPartBest = m_cost[yCbCr][typeIdx];
            copySaoUnit(saoLcuParam, &saoLcuParamRdo);
            bestDist = estDist;
        }
    }

    compDistortion[0] += ((double)bestDist / lambda);
    m_entropyCoder->load(m_rdEntropyCoders[0][CI_TEMP_BEST]);
    m_entropyCoder->codeSaoOffset(saoLcuParam, yCbCr);
    m_entropyCoder->store(m_rdEntropyCoders[0][CI_TEMP_BEST]);

    // merge left or merge up

    for (int idxNeighbor = 0; idxNeighbor < 2; idxNeighbor++)
    {
        saoLcuParamNeighbor = NULL;
        if (allowMergeLeft && addrLeft >= 0 && idxNeighbor == 0)
            saoLcuParamNeighbor = &(saoParam->saoLcuParam[yCbCr][addrLeft]);
        else if (allowMergeUp && addrUp >= 0 && idxNeighbor == 1)
            saoLcuParamNeighbor = &(saoParam->saoLcuParam[yCbCr][addrUp]);
        if (saoLcuParamNeighbor != NULL)
        {
            estDist = 0;
            typeIdx = saoLcuParamNeighbor->typeIdx;
            if (typeIdx >= 0)
            {
                int mergeBandPosition = (typeIdx == SAO_BO) ? saoLcuParamNeighbor->subTypeIdx : 0;
                int mergeOffset;
                for (classIdx = 0; classIdx < m_numClass[typeIdx]; classIdx++)
                {
                    mergeOffset = saoLcuParamNeighbor->offset[classIdx];
                    estDist += estSaoDist(m_count[yCbCr][typeIdx][classIdx + mergeBandPosition + 1], mergeOffset, m_offsetOrg[yCbCr][typeIdx][classIdx + mergeBandPosition + 1],  shift);
                }
            }
            else
                estDist = 0;

            copySaoUnit(&compSaoParam[idxNeighbor], saoLcuParamNeighbor);
            compSaoParam[idxNeighbor].mergeUpFlag   = !!idxNeighbor;
            compSaoParam[idxNeighbor].mergeLeftFlag = !idxNeighbor;

            compDistortion[idxNeighbor + 1] += ((double)estDist / lambda);
        }
    }
}

void TEncSampleAdaptiveOffset::sao2ChromaParamDist(int allowMergeLeft, int allowMergeUp, SAOParam *saoParam, int addr, int addrUp, int addrLeft, double lambda, SaoLcuParam *crSaoParam, SaoLcuParam *cbSaoParam, double *distortion)
{
    int typeIdx;

    int64_t estDist[2];
    int classIdx;
    int shift = 2 * DISTORTION_PRECISION_ADJUSTMENT(X265_DEPTH - 8);
    int64_t bestDist = 0;

    SaoLcuParam*  saoLcuParam[2] = { &(saoParam->saoLcuParam[1][addr]), &(saoParam->saoLcuParam[2][addr]) };
    SaoLcuParam*  saoLcuParamNeighbor[2] = { NULL, NULL };
    SaoLcuParam*  saoMergeParam[2][2];

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

    double  bestRDCostTableBo;
    int     bestClassTableBo[2] = { 0, 0 };
    int     currentDistortionTableBo[MAX_NUM_SAO_CLASS];
    double  currentRdCostTableBo[MAX_NUM_SAO_CLASS];

    SaoLcuParam saoLcuParamRdo[2];
    double estRate = 0;

    resetSaoUnit(&saoLcuParamRdo[0]);
    resetSaoUnit(&saoLcuParamRdo[1]);

    m_entropyCoder->load(m_rdEntropyCoders[0][CI_TEMP_BEST]);
    m_entropyCoder->resetBits();
    m_entropyCoder->codeSaoOffset(&saoLcuParamRdo[0], 1);
    m_entropyCoder->codeSaoOffset(&saoLcuParamRdo[1], 2);

    costPartBest = m_entropyCoder->getNumberOfWrittenBits() * lambda;
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
                estDist[compIdx] = estSaoTypeDist(compIdx + 1, typeIdx, shift, lambda, currentDistortionTableBo, currentRdCostTableBo);
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
            estDist[0] = estSaoTypeDist(1, typeIdx, shift, lambda, currentDistortionTableBo, currentRdCostTableBo);
            estDist[1] = estSaoTypeDist(2, typeIdx, shift, lambda, currentDistortionTableBo, currentRdCostTableBo);
        }

        m_entropyCoder->load(m_rdEntropyCoders[0][CI_TEMP_BEST]);
        m_entropyCoder->resetBits();

        for (int compIdx = 0; compIdx < 2; compIdx++)
        {
            resetSaoUnit(&saoLcuParamRdo[compIdx]);
            saoLcuParamRdo[compIdx].length = m_numClass[typeIdx];
            saoLcuParamRdo[compIdx].typeIdx = typeIdx;
            saoLcuParamRdo[compIdx].mergeLeftFlag = 0;
            saoLcuParamRdo[compIdx].mergeUpFlag   = 0;
            saoLcuParamRdo[compIdx].subTypeIdx = (typeIdx == SAO_BO) ? bestClassTableBo[compIdx] : 0;
            for (classIdx = 0; classIdx < saoLcuParamRdo[compIdx].length; classIdx++)
                saoLcuParamRdo[compIdx].offset[classIdx] = (int)m_offset[compIdx + 1][typeIdx][classIdx + saoLcuParamRdo[compIdx].subTypeIdx + 1];

            m_entropyCoder->codeSaoOffset(&saoLcuParamRdo[compIdx], compIdx + 1);
        }

        estRate = m_entropyCoder->getNumberOfWrittenBits();
        m_cost[1][typeIdx] = (double)((double)(estDist[0] + estDist[1])  + lambda * (double)estRate);

        if (m_cost[1][typeIdx] < costPartBest)
        {
            costPartBest = m_cost[1][typeIdx];
            copySaoUnit(saoLcuParam[0], &saoLcuParamRdo[0]);
            copySaoUnit(saoLcuParam[1], &saoLcuParamRdo[1]);
            bestDist = (estDist[0] + estDist[1]);
        }
    }

    distortion[0] += ((double)bestDist / lambda);
    m_entropyCoder->load(m_rdEntropyCoders[0][CI_TEMP_BEST]);
    m_entropyCoder->codeSaoOffset(saoLcuParam[0], 1);
    m_entropyCoder->codeSaoOffset(saoLcuParam[1], 2);
    m_entropyCoder->store(m_rdEntropyCoders[0][CI_TEMP_BEST]);

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
                    int mergeOffset;
                    for (classIdx = 0; classIdx < m_numClass[typeIdx]; classIdx++)
                    {
                        mergeOffset = saoLcuParamNeighbor[compIdx]->offset[classIdx];
                        estDist[compIdx] += estSaoDist(m_count[compIdx + 1][typeIdx][classIdx + mergeBandPosition + 1], mergeOffset, m_offsetOrg[compIdx + 1][typeIdx][classIdx + mergeBandPosition + 1],  shift);
                    }
                }
                else
                    estDist[compIdx] = 0;

                copySaoUnit(saoMergeParam[compIdx][idxNeighbor], saoLcuParamNeighbor[compIdx]);
                saoMergeParam[compIdx][idxNeighbor]->mergeUpFlag   = !!idxNeighbor;
                saoMergeParam[compIdx][idxNeighbor]->mergeLeftFlag = !idxNeighbor;
                distortion[idxNeighbor + 1] += ((double)estDist[compIdx] / lambda);
            }
        }
    }
}
