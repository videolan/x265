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

/** \file     TEncSearch.cpp
 \brief    encoder search class
 */

#include "TLibCommon/TypeDef.h"
#include "TLibCommon/TComRom.h"
#include "TLibCommon/TComMotionInfo.h"
#include "TEncSearch.h"
#include "primitives.h"
#include "common.h"
#include "PPA/ppa.h"
#include <math.h>

using namespace x265;

#if CU_STAT_LOGFILE
extern FILE *fp1;
extern bool mergeFlag;
UInt64      meCost;
#endif
DECLARE_CYCLE_COUNTER(ME);

//! \ingroup TLibEncoder
//! \{

static const MV s_mvRefineHpel[9] =
{
    MV(0,  0),  // 0
    MV(0, -1),  // 1
    MV(0,  1),  // 2
    MV(-1,  0), // 3
    MV(1,  0),  // 4
    MV(-1, -1), // 5
    MV(1, -1),  // 6
    MV(-1,  1), // 7
    MV(1,  1)   // 8
};

static const MV s_mvRefineQPel[9] =
{
    MV(0,  0),  // 0
    MV(0, -1),  // 1
    MV(0,  1),  // 2
    MV(-1, -1), // 5
    MV(1, -1),  // 6
    MV(-1,  0), // 3
    MV(1,  0),  // 4
    MV(-1,  1), // 7
    MV(1,  1)   // 8
};

static const UInt s_dFilter[9] =
{
    0, 1, 0,
    2, 3, 2,
    0, 1, 0
};

TEncSearch::TEncSearch()
{
    m_ppcQTTempCoeffY  = NULL;
    m_ppcQTTempCoeffCb = NULL;
    m_ppcQTTempCoeffCr = NULL;
    m_pcQTTempCoeffY   = NULL;
    m_pcQTTempCoeffCb  = NULL;
    m_pcQTTempCoeffCr  = NULL;
    m_ppcQTTempArlCoeffY  = NULL;
    m_ppcQTTempArlCoeffCb = NULL;
    m_ppcQTTempArlCoeffCr = NULL;
    m_pcQTTempArlCoeffY   = NULL;
    m_pcQTTempArlCoeffCb  = NULL;
    m_pcQTTempArlCoeffCr  = NULL;
    m_puhQTTempTrIdx   = NULL;
    m_puhQTTempCbf[0] = m_puhQTTempCbf[1] = m_puhQTTempCbf[2] = NULL;
    m_pcQTTempTComYuv  = NULL;
    m_pcEncCfg = NULL;
    m_pcEntropyCoder = NULL;
    m_pTempPel = NULL;
    m_pSharedPredTransformSkip[0] = m_pSharedPredTransformSkip[1] = m_pSharedPredTransformSkip[2] = NULL;
    m_pcQTTempTUCoeffY   = NULL;
    m_pcQTTempTUCoeffCb  = NULL;
    m_pcQTTempTUCoeffCr  = NULL;
    m_ppcQTTempTUArlCoeffY  = NULL;
    m_ppcQTTempTUArlCoeffCb = NULL;
    m_ppcQTTempTUArlCoeffCr = NULL;
    m_puhQTTempTransformSkipFlag[0] = NULL;
    m_puhQTTempTransformSkipFlag[1] = NULL;
    m_puhQTTempTransformSkipFlag[2] = NULL;
    setWpScalingDistParam(NULL, -1, REF_PIC_LIST_X);
}

TEncSearch::~TEncSearch()
{
    if (m_pTempPel)
    {
        delete [] m_pTempPel;
        m_pTempPel = NULL;
    }

    if (m_pcEncCfg)
    {
        const UInt uiNumLayersAllocated = m_pcEncCfg->getQuadtreeTULog2MaxSize() - m_pcEncCfg->getQuadtreeTULog2MinSize() + 1;
        for (UInt ui = 0; ui < uiNumLayersAllocated; ++ui)
        {
            delete[] m_ppcQTTempCoeffY[ui];
            delete[] m_ppcQTTempCoeffCb[ui];
            delete[] m_ppcQTTempCoeffCr[ui];
            delete[] m_ppcQTTempArlCoeffY[ui];
            delete[] m_ppcQTTempArlCoeffCb[ui];
            delete[] m_ppcQTTempArlCoeffCr[ui];
            m_pcQTTempTComYuv[ui].destroy();
        }
    }
    delete[] m_ppcQTTempCoeffY;
    delete[] m_ppcQTTempCoeffCb;
    delete[] m_ppcQTTempCoeffCr;
    delete[] m_pcQTTempCoeffY;
    delete[] m_pcQTTempCoeffCb;
    delete[] m_pcQTTempCoeffCr;
    delete[] m_ppcQTTempArlCoeffY;
    delete[] m_ppcQTTempArlCoeffCb;
    delete[] m_ppcQTTempArlCoeffCr;
    delete[] m_pcQTTempArlCoeffY;
    delete[] m_pcQTTempArlCoeffCb;
    delete[] m_pcQTTempArlCoeffCr;
    delete[] m_puhQTTempTrIdx;
    delete[] m_puhQTTempCbf[0];
    delete[] m_puhQTTempCbf[1];
    delete[] m_puhQTTempCbf[2];
    delete[] m_pcQTTempTComYuv;
    delete[] m_pSharedPredTransformSkip[0];
    delete[] m_pSharedPredTransformSkip[1];
    delete[] m_pSharedPredTransformSkip[2];
    delete[] m_pcQTTempTUCoeffY;
    delete[] m_pcQTTempTUCoeffCb;
    delete[] m_pcQTTempTUCoeffCr;
    delete[] m_ppcQTTempTUArlCoeffY;
    delete[] m_ppcQTTempTUArlCoeffCb;
    delete[] m_ppcQTTempTUArlCoeffCr;
    delete[] m_puhQTTempTransformSkipFlag[0];
    delete[] m_puhQTTempTransformSkipFlag[1];
    delete[] m_puhQTTempTransformSkipFlag[2];
    m_pcQTTempTransformSkipTComYuv.destroy();
    m_tmpYuvPred.destroy();
}

Void TEncSearch::init(TEncCfg* pcEncCfg, TComRdCost* pcRdCost, TComTrQuant* pcTrQuant)
{
    m_pcEncCfg          = pcEncCfg;
    m_pcTrQuant         = pcTrQuant;
    m_pcRdCost          = pcRdCost;
    m_iSearchRange      = pcEncCfg->getSearchRange();
    m_bipredSearchRange = pcEncCfg->getBipredSearchRange();
    m_iSearchMethod     = pcEncCfg->getSearchMethod();
    m_pcEntropyCoder    = NULL;
    m_pppcRDSbacCoder   = NULL;
    m_pcRDGoOnSbacCoder = NULL;
    m_me.setSearchMethod(m_iSearchMethod);

    for (Int dir = 0; dir < 2; dir++)
    {
        for (Int ref = 0; ref < 33; ref++)
        {
            m_adaptiveRange[dir][ref] = m_iSearchRange;
        }
    }

    m_puiDFilter = s_dFilter + 4;

    // initialize motion cost
    for (Int iNum = 0; iNum < AMVP_MAX_NUM_CANDS + 1; iNum++)
    {
        for (Int iIdx = 0; iIdx < AMVP_MAX_NUM_CANDS; iIdx++)
        {
            if (iIdx < iNum)
                m_mvpIdxCost[iIdx][iNum] = xGetMvpIdxBits(iIdx, iNum);
            else
                m_mvpIdxCost[iIdx][iNum] = MAX_INT;
        }
    }

    initTempBuff();

    m_pTempPel = new Pel[g_maxCUWidth * g_maxCUHeight];

    const UInt uiNumLayersToAllocate = pcEncCfg->getQuadtreeTULog2MaxSize() - pcEncCfg->getQuadtreeTULog2MinSize() + 1;
    m_ppcQTTempCoeffY  = new TCoeff*[uiNumLayersToAllocate];
    m_ppcQTTempCoeffCb = new TCoeff*[uiNumLayersToAllocate];
    m_ppcQTTempCoeffCr = new TCoeff*[uiNumLayersToAllocate];
    m_pcQTTempCoeffY   = new TCoeff[g_maxCUWidth * g_maxCUHeight];
    m_pcQTTempCoeffCb  = new TCoeff[g_maxCUWidth * g_maxCUHeight >> 2];
    m_pcQTTempCoeffCr  = new TCoeff[g_maxCUWidth * g_maxCUHeight >> 2];
    m_ppcQTTempArlCoeffY  = new Int*[uiNumLayersToAllocate];
    m_ppcQTTempArlCoeffCb = new Int*[uiNumLayersToAllocate];
    m_ppcQTTempArlCoeffCr = new Int*[uiNumLayersToAllocate];
    m_pcQTTempArlCoeffY   = new Int[g_maxCUWidth * g_maxCUHeight];
    m_pcQTTempArlCoeffCb  = new Int[g_maxCUWidth * g_maxCUHeight >> 2];
    m_pcQTTempArlCoeffCr  = new Int[g_maxCUWidth * g_maxCUHeight >> 2];

    const UInt uiNumPartitions = 1 << (g_maxCUDepth << 1);
    m_puhQTTempTrIdx   = new UChar[uiNumPartitions];
    m_puhQTTempCbf[0]  = new UChar[uiNumPartitions];
    m_puhQTTempCbf[1]  = new UChar[uiNumPartitions];
    m_puhQTTempCbf[2]  = new UChar[uiNumPartitions];
    m_pcQTTempTComYuv  = new TShortYUV[uiNumLayersToAllocate];
    for (UInt ui = 0; ui < uiNumLayersToAllocate; ++ui)
    {
        m_ppcQTTempCoeffY[ui]  = new TCoeff[g_maxCUWidth * g_maxCUHeight];
        m_ppcQTTempCoeffCb[ui] = new TCoeff[g_maxCUWidth * g_maxCUHeight >> 2];
        m_ppcQTTempCoeffCr[ui] = new TCoeff[g_maxCUWidth * g_maxCUHeight >> 2];
        m_ppcQTTempArlCoeffY[ui]  = new Int[g_maxCUWidth * g_maxCUHeight];
        m_ppcQTTempArlCoeffCb[ui] = new Int[g_maxCUWidth * g_maxCUHeight >> 2];
        m_ppcQTTempArlCoeffCr[ui] = new Int[g_maxCUWidth * g_maxCUHeight >> 2];
        m_pcQTTempTComYuv[ui].create(g_maxCUWidth, g_maxCUHeight);
    }

    m_pSharedPredTransformSkip[0] = new Pel[MAX_TS_WIDTH * MAX_TS_HEIGHT];
    m_pSharedPredTransformSkip[1] = new Pel[MAX_TS_WIDTH * MAX_TS_HEIGHT];
    m_pSharedPredTransformSkip[2] = new Pel[MAX_TS_WIDTH * MAX_TS_HEIGHT];
    m_pcQTTempTUCoeffY  = new TCoeff[MAX_TS_WIDTH * MAX_TS_HEIGHT];
    m_pcQTTempTUCoeffCb = new TCoeff[MAX_TS_WIDTH * MAX_TS_HEIGHT];
    m_pcQTTempTUCoeffCr = new TCoeff[MAX_TS_WIDTH * MAX_TS_HEIGHT];
    m_ppcQTTempTUArlCoeffY  = new Int[MAX_TS_WIDTH * MAX_TS_HEIGHT];
    m_ppcQTTempTUArlCoeffCb = new Int[MAX_TS_WIDTH * MAX_TS_HEIGHT];
    m_ppcQTTempTUArlCoeffCr = new Int[MAX_TS_WIDTH * MAX_TS_HEIGHT];
    m_pcQTTempTransformSkipTComYuv.create(g_maxCUWidth, g_maxCUHeight);

    m_puhQTTempTransformSkipFlag[0] = new UChar[uiNumPartitions];
    m_puhQTTempTransformSkipFlag[1] = new UChar[uiNumPartitions];
    m_puhQTTempTransformSkipFlag[2] = new UChar[uiNumPartitions];
    m_tmpYuvPred.create(MAX_CU_SIZE, MAX_CU_SIZE);
}

Void TEncSearch::setQPLambda(Int QP, Double lambdaLuma, Double lambdaChroma)
{
    m_me.setQP(QP, lambdaLuma);
    m_bc.setQP(QP, lambdaLuma);
    m_pcRdCost->setLambda(lambdaLuma);
    m_pcTrQuant->setLambda(lambdaLuma, lambdaChroma);
}

inline Void TEncSearch::xTZSearchHelp(TComPattern* patternKey, IntTZSearchStruct& data, Int searchX, Int searchY, UChar pointDir, UInt distance)
{
    Pel*  fref = data.fref + searchY * data.lumaStride + searchX;
    m_pcRdCost->setDistParam(patternKey, fref, data.lumaStride, m_cDistParam);

    if (m_cDistParam.iRows > 12)
    {
        // fast encoder decision: use subsampled SAD when rows > 12 for integer ME
        m_cDistParam.iSubShift = 1;
    }

    // distortion
    m_cDistParam.bitDepth = g_bitDepthY;
    UInt sad = m_cDistParam.DistFunc(&m_cDistParam);

    // motion cost
    sad += m_bc.mvcost(MV(searchX, searchY) << m_pcRdCost->m_iCostScale);

    if (sad < data.bcost)
    {
        data.bcost        = sad;
        data.bestx        = searchX;
        data.besty        = searchY;
        data.bestDistance = distance;
        data.bestRound    = 0;
        data.bestPointDir = pointDir;
    }
}

inline Void TEncSearch::xTZ2PointSearch(TComPattern* patternKey, IntTZSearchStruct& data, MV* mvmin, MV* mvmax)
{
    Int srchRngHorLeft   = mvmin->x;
    Int srchRngHorRight  = mvmax->x;
    Int srchRngVerTop    = mvmin->y;
    Int srchRngVerBottom = mvmax->y;

    // 2 point search,                   //   1 2 3
    // check only the 2 untested points  //   4 0 5
    // around the start point            //   6 7 8
    Int startX = data.bestx;
    Int startY = data.besty;

    switch (data.bestPointDir)
    {
    case 1:
    {
        if ((startX - 1) >= srchRngHorLeft)
        {
            xTZSearchHelp(patternKey, data, startX - 1, startY, 0, 2);
        }
        if ((startY - 1) >= srchRngVerTop)
        {
            xTZSearchHelp(patternKey, data, startX, startY - 1, 0, 2);
        }
    }
    break;
    case 2:
    {
        if ((startY - 1) >= srchRngVerTop)
        {
            if ((startX - 1) >= srchRngHorLeft)
            {
                xTZSearchHelp(patternKey, data, startX - 1, startY - 1, 0, 2);
            }
            if ((startX + 1) <= srchRngHorRight)
            {
                xTZSearchHelp(patternKey, data, startX + 1, startY - 1, 0, 2);
            }
        }
    }
    break;
    case 3:
    {
        if ((startY - 1) >= srchRngVerTop)
        {
            xTZSearchHelp(patternKey, data, startX, startY - 1, 0, 2);
        }
        if ((startX + 1) <= srchRngHorRight)
        {
            xTZSearchHelp(patternKey, data, startX + 1, startY, 0, 2);
        }
    }
    break;
    case 4:
    {
        if ((startX - 1) >= srchRngHorLeft)
        {
            if ((startY + 1) <= srchRngVerBottom)
            {
                xTZSearchHelp(patternKey, data, startX - 1, startY + 1, 0, 2);
            }
            if ((startY - 1) >= srchRngVerTop)
            {
                xTZSearchHelp(patternKey, data, startX - 1, startY - 1, 0, 2);
            }
        }
    }
    break;
    case 5:
    {
        if ((startX + 1) <= srchRngHorRight)
        {
            if ((startY - 1) >= srchRngVerTop)
            {
                xTZSearchHelp(patternKey, data, startX + 1, startY - 1, 0, 2);
            }
            if ((startY + 1) <= srchRngVerBottom)
            {
                xTZSearchHelp(patternKey, data, startX + 1, startY + 1, 0, 2);
            }
        }
    }
    break;
    case 6:
    {
        if ((startX - 1) >= srchRngHorLeft)
        {
            xTZSearchHelp(patternKey, data, startX - 1, startY, 0, 2);
        }
        if ((startY + 1) <= srchRngVerBottom)
        {
            xTZSearchHelp(patternKey, data, startX, startY + 1, 0, 2);
        }
    }
    break;
    case 7:
    {
        if ((startY + 1) <= srchRngVerBottom)
        {
            if ((startX - 1) >= srchRngHorLeft)
            {
                xTZSearchHelp(patternKey, data, startX - 1, startY + 1, 0, 2);
            }
            if ((startX + 1) <= srchRngHorRight)
            {
                xTZSearchHelp(patternKey, data, startX + 1, startY + 1, 0, 2);
            }
        }
    }
    break;
    case 8:
    {
        if ((startX + 1) <= srchRngHorRight)
        {
            xTZSearchHelp(patternKey, data, startX + 1, startY, 0, 2);
        }
        if ((startY + 1) <= srchRngVerBottom)
        {
            xTZSearchHelp(patternKey, data, startX, startY + 1, 0, 2);
        }
    }
    break;
    default:
    {
        assert(false);
    }
    break;
    } // switch( rcStruct.ucPointNr )
}

__inline Void TEncSearch::xTZ8PointDiamondSearch(TComPattern* patternKey, IntTZSearchStruct& data, MV* mvmin, MV* mvmax, Int startX, Int startY, Int distance)
{
    assert(distance != 0);
    Int srchRngHorLeft   = mvmin->x;
    Int srchRngHorRight  = mvmax->x;
    Int srchRngVerTop    = mvmin->y;
    Int srchRngVerBottom = mvmax->y;
    const Int top        = startY - distance;
    const Int bottom     = startY + distance;
    const Int left       = startX - distance;
    const Int right      = startX + distance;
    data.bestRound += 1;

    if (distance == 1) // iDist == 1
    {
        if (top >= srchRngVerTop) // check top
        {
            xTZSearchHelp(patternKey, data, startX, top, 2, distance);
        }
        if (left >= srchRngHorLeft) // check middle left
        {
            xTZSearchHelp(patternKey, data, left, startY, 4, distance);
        }
        if (right <= srchRngHorRight) // check middle right
        {
            xTZSearchHelp(patternKey, data, right, startY, 5, distance);
        }
        if (bottom <= srchRngVerBottom) // check bottom
        {
            xTZSearchHelp(patternKey, data, startX, bottom, 7, distance);
        }
    }
    else if (distance <= 8)
    {
        const Int top2      = startY - (distance >> 1);
        const Int bot2   = startY + (distance >> 1);
        const Int left2     = startX - (distance >> 1);
        const Int right2    = startX + (distance >> 1);

        if (top >= srchRngVerTop && left >= srchRngHorLeft &&
            right <= srchRngHorRight && bottom <= srchRngVerBottom) // check border
        {
            xTZSearchHelp(patternKey, data, startX,   top, 2, distance);
            xTZSearchHelp(patternKey, data, left2,   top2, 1, distance >> 1);
            xTZSearchHelp(patternKey, data, right2,  top2, 3, distance >> 1);
            xTZSearchHelp(patternKey, data, left,  startY, 4, distance);
            xTZSearchHelp(patternKey, data, right, startY, 5, distance);
            xTZSearchHelp(patternKey, data, left2,   bot2, 6, distance >> 1);
            xTZSearchHelp(patternKey, data, right2,  bot2, 8, distance >> 1);
            xTZSearchHelp(patternKey, data, startX, bottom, 7, distance);
        }
        else // check border for each mv
        {
            if (top >= srchRngVerTop) // check top
            {
                xTZSearchHelp(patternKey, data, startX, top, 2, distance);
            }
            if (top2 >= srchRngVerTop) // check half top
            {
                if (left2 >= srchRngHorLeft) // check half left
                {
                    xTZSearchHelp(patternKey, data, left2, top2, 1, (distance >> 1));
                }
                if (right2 <= srchRngHorRight) // check half right
                {
                    xTZSearchHelp(patternKey, data, right2, top2, 3, (distance >> 1));
                }
            } // check half top
            if (left >= srchRngHorLeft) // check left
            {
                xTZSearchHelp(patternKey, data, left, startY, 4, distance);
            }
            if (right <= srchRngHorRight) // check right
            {
                xTZSearchHelp(patternKey, data, right, startY, 5, distance);
            }
            if (bot2 <= srchRngVerBottom) // check half bottom
            {
                if (left2 >= srchRngHorLeft) // check half left
                {
                    xTZSearchHelp(patternKey, data, left2, bot2, 6, (distance >> 1));
                }
                if (right2 <= srchRngHorRight) // check half right
                {
                    xTZSearchHelp(patternKey, data, right2, bot2, 8, (distance >> 1));
                }
            } // check half bottom
            if (bottom <= srchRngVerBottom) // check bottom
            {
                xTZSearchHelp(patternKey, data, startX, bottom, 7, distance);
            }
        } // check border for each mv
    }
    else // iDist > 8
    {
        if (top >= srchRngVerTop && left >= srchRngHorLeft &&
            right <= srchRngHorRight && bottom <= srchRngVerBottom) // check border
        {
            xTZSearchHelp(patternKey, data, startX, top,    0, distance);
            xTZSearchHelp(patternKey, data, left,   startY, 0, distance);
            xTZSearchHelp(patternKey, data, right,  startY, 0, distance);
            xTZSearchHelp(patternKey, data, startX, bottom, 0, distance);
            for (Int index = 1; index < 4; index++)
            {
                Int posYT = top    + ((distance >> 2) * index);
                Int posYB = bottom - ((distance >> 2) * index);
                Int posXL = startX - ((distance >> 2) * index);
                Int PosXR = startX + ((distance >> 2) * index);
                xTZSearchHelp(patternKey, data, posXL, posYT, 0, distance);
                xTZSearchHelp(patternKey, data, PosXR, posYT, 0, distance);
                xTZSearchHelp(patternKey, data, posXL, posYB, 0, distance);
                xTZSearchHelp(patternKey, data, PosXR, posYB, 0, distance);
            }
        }
        else // check border for each mv
        {
            if (top >= srchRngVerTop) // check top
            {
                xTZSearchHelp(patternKey, data, startX, top, 0, distance);
            }
            if (left >= srchRngHorLeft) // check left
            {
                xTZSearchHelp(patternKey, data, left, startY, 0, distance);
            }
            if (right <= srchRngHorRight) // check right
            {
                xTZSearchHelp(patternKey, data, right, startY, 0, distance);
            }
            if (bottom <= srchRngVerBottom) // check bottom
            {
                xTZSearchHelp(patternKey, data, startX, bottom, 0, distance);
            }
            for (Int index = 1; index < 4; index++)
            {
                Int posYT = top    + ((distance >> 2) * index);
                Int posYB = bottom - ((distance >> 2) * index);
                Int posXL = startX - ((distance >> 2) * index);
                Int posXR = startX + ((distance >> 2) * index);

                if (posYT >= srchRngVerTop) // check top
                {
                    if (posXL >= srchRngHorLeft) // check left
                    {
                        xTZSearchHelp(patternKey, data, posXL, posYT, 0, distance);
                    }
                    if (posXR <= srchRngHorRight) // check right
                    {
                        xTZSearchHelp(patternKey, data, posXR, posYT, 0, distance);
                    }
                } // check top
                if (posYB <= srchRngVerBottom) // check bottom
                {
                    if (posXL >= srchRngHorLeft) // check left
                    {
                        xTZSearchHelp(patternKey, data, posXL, posYB, 0, distance);
                    }
                    if (posXR <= srchRngHorRight) // check right
                    {
                        xTZSearchHelp(patternKey, data, posXR, posYB, 0, distance);
                    }
                } // check bottom
            } // for ...
        } // check border for each mv
    } // iDist > 8
}

//<--

UInt TEncSearch::xPatternRefinement(TComPattern* patternKey, MV baseRefMv, Int fracBits, MV& outFracMv, TComPicYuv* refPic, Int offset, TComDataCU* cu, UInt partAddr)
{
    UInt  cost;
    UInt  bcost = MAX_UINT;
    Pel*  fref;
    UInt  bestDir = 0;
    Int   stride = refPic->getStride();

    m_pcRdCost->setDistParam(patternKey, refPic->getLumaFilterBlock(0, 0, cu->getAddr(), cu->getZorderIdxInCU() + partAddr) + offset, stride, 1, m_cDistParam, true);
    m_cDistParam.bitDepth = g_bitDepthY;

    const MV* mvRefine = (fracBits == 2 ? s_mvRefineHpel : s_mvRefineQPel);
    for (int i = 0; i < 9; i++)
    {
        // TODO: this is overly complicated, but it mainly needs to be deleted
        MV cMvTest = baseRefMv + mvRefine[i];

        Int horVal = cMvTest.x * fracBits;
        Int verVal = cMvTest.y * fracBits;
        fref =  refPic->getLumaFilterBlock(verVal & 3, horVal & 3, cu->getAddr(), cu->getZorderIdxInCU() + partAddr) + offset;
        if (horVal < 0)
            fref -= 1;
        if (verVal < 0)
        {
            fref -= stride;
        }
        m_cDistParam.pCur = fref;

        cMvTest = outFracMv + mvRefine[i];
        cost = m_cDistParam.DistFunc(&m_cDistParam) + m_bc.mvcost(cMvTest * fracBits);

        if (cost < bcost)
        {
            bcost  = cost;
            bestDir = i;
        }
    }

    outFracMv = mvRefine[bestDir];
    return bcost;
}

Void TEncSearch::xEncSubdivCbfQT(TComDataCU* cu, UInt trDepth, UInt absPartIdx, Bool bLuma, Bool bChroma)
{
    UInt  fullDepth  = cu->getDepth(0) + trDepth;
    UInt  trMode     = cu->getTransformIdx(absPartIdx);
    UInt  subdiv     = (trMode > trDepth ? 1 : 0);
    UInt  trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth()] + 2 - fullDepth;

    if (cu->getPredictionMode(0) == MODE_INTRA && cu->getPartitionSize(0) == SIZE_NxN && trDepth == 0)
    {
        assert(subdiv);
    }
    else if (trSizeLog2 > cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize())
    {
        assert(subdiv);
    }
    else if (trSizeLog2 == cu->getSlice()->getSPS()->getQuadtreeTULog2MinSize())
    {
        assert(!subdiv);
    }
    else if (trSizeLog2 == cu->getQuadtreeTULog2MinSizeInCU(absPartIdx))
    {
        assert(!subdiv);
    }
    else
    {
        assert(trSizeLog2 > cu->getQuadtreeTULog2MinSizeInCU(absPartIdx));
        if (bLuma)
        {
            m_pcEntropyCoder->encodeTransformSubdivFlag(subdiv, 5 - trSizeLog2);
        }
    }

    if (bChroma)
    {
        if (trSizeLog2 > 2)
        {
            if (trDepth == 0 || cu->getCbf(absPartIdx, TEXT_CHROMA_U, trDepth - 1))
                m_pcEntropyCoder->encodeQtCbf(cu, absPartIdx, TEXT_CHROMA_U, trDepth);
            if (trDepth == 0 || cu->getCbf(absPartIdx, TEXT_CHROMA_V, trDepth - 1))
                m_pcEntropyCoder->encodeQtCbf(cu, absPartIdx, TEXT_CHROMA_V, trDepth);
        }
    }

    if (subdiv)
    {
        UInt qtPartNum = cu->getPic()->getNumPartInCU() >> ((fullDepth + 1) << 1);
        for (UInt part = 0; part < 4; part++)
        {
            xEncSubdivCbfQT(cu, trDepth + 1, absPartIdx + part * qtPartNum, bLuma, bChroma);
        }

        return;
    }

    //===== Cbfs =====
    if (bLuma)
    {
        m_pcEntropyCoder->encodeQtCbf(cu, absPartIdx, TEXT_LUMA, trMode);
    }
}

Void TEncSearch::xEncCoeffQT(TComDataCU* cu, UInt trDepth, UInt absPartIdx, TextType ttype)
{
    UInt fullDepth  = cu->getDepth(0) + trDepth;
    UInt trMode     = cu->getTransformIdx(absPartIdx);
    UInt subdiv     = (trMode > trDepth ? 1 : 0);
    UInt trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth()] + 2 - fullDepth;
    UInt chroma     = (ttype != TEXT_LUMA ? 1 : 0);

    if (subdiv)
    {
        UInt qtPartNum = cu->getPic()->getNumPartInCU() >> ((fullDepth + 1) << 1);
        for (UInt part = 0; part < 4; part++)
        {
            xEncCoeffQT(cu, trDepth + 1, absPartIdx + part * qtPartNum, ttype);
        }

        return;
    }

    if (ttype != TEXT_LUMA && trSizeLog2 == 2)
    {
        assert(trDepth > 0);
        trDepth--;
        UInt qpdiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trDepth) << 1);
        Bool bFirstQ = ((absPartIdx % qpdiv) == 0);
        if (!bFirstQ)
        {
            return;
        }
    }

    //===== coefficients =====
    UInt width = cu->getWidth(0) >> (trDepth + chroma);
    UInt height = cu->getHeight(0) >> (trDepth + chroma);
    UInt coeffOffset = (cu->getPic()->getMinCUWidth() * cu->getPic()->getMinCUHeight() * absPartIdx) >> (chroma << 1);
    UInt qtLayer = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;
    TCoeff* coeff = 0;
    switch (ttype)
    {
    case TEXT_LUMA:     coeff = m_ppcQTTempCoeffY[qtLayer];  break;
    case TEXT_CHROMA_U: coeff = m_ppcQTTempCoeffCb[qtLayer]; break;
    case TEXT_CHROMA_V: coeff = m_ppcQTTempCoeffCr[qtLayer]; break;
    default: assert(0);
    }

    coeff += coeffOffset;

    m_pcEntropyCoder->encodeCoeffNxN(cu, coeff, absPartIdx, width, height, fullDepth, ttype);
}

Void TEncSearch::xEncIntraHeader(TComDataCU* cu, UInt trDepth, UInt absPartIdx, Bool bLuma, Bool bChroma)
{
    if (bLuma)
    {
        // CU header
        if (absPartIdx == 0)
        {
            if (!cu->getSlice()->isIntra())
            {
                if (cu->getSlice()->getPPS()->getTransquantBypassEnableFlag())
                {
                    m_pcEntropyCoder->encodeCUTransquantBypassFlag(cu, 0, true);
                }
                m_pcEntropyCoder->encodeSkipFlag(cu, 0, true);
                m_pcEntropyCoder->encodePredMode(cu, 0, true);
            }

            m_pcEntropyCoder->encodePartSize(cu, 0, cu->getDepth(0), true);

            if (cu->isIntra(0) && cu->getPartitionSize(0) == SIZE_2Nx2N)
            {
                m_pcEntropyCoder->encodeIPCMInfo(cu, 0, true);

                if (cu->getIPCMFlag(0))
                {
                    return;
                }
            }
        }
        // luma prediction mode
        if (cu->getPartitionSize(0) == SIZE_2Nx2N)
        {
            if (absPartIdx == 0)
            {
                m_pcEntropyCoder->encodeIntraDirModeLuma(cu, 0);
            }
        }
        else
        {
            UInt qtNumParts = cu->getTotalNumPart() >> 2;
            if (trDepth == 0)
            {
                assert(absPartIdx == 0);
                for (UInt part = 0; part < 4; part++)
                {
                    m_pcEntropyCoder->encodeIntraDirModeLuma(cu, part * qtNumParts);
                }
            }
            else if ((absPartIdx % qtNumParts) == 0)
            {
                m_pcEntropyCoder->encodeIntraDirModeLuma(cu, absPartIdx);
            }
        }
    }
    if (bChroma)
    {
        // chroma prediction mode
        if (absPartIdx == 0)
        {
            m_pcEntropyCoder->encodeIntraDirModeChroma(cu, 0, true);
        }
    }
}

UInt TEncSearch::xGetIntraBitsQT(TComDataCU* cu, UInt trDepth, UInt absPartIdx, Bool bLuma, Bool bChroma)
{
    m_pcEntropyCoder->resetBits();
    xEncIntraHeader(cu, trDepth, absPartIdx, bLuma, bChroma);
    xEncSubdivCbfQT(cu, trDepth, absPartIdx, bLuma, bChroma);

    if (bLuma)
    {
        xEncCoeffQT(cu, trDepth, absPartIdx, TEXT_LUMA);
    }
    if (bChroma)
    {
        xEncCoeffQT(cu, trDepth, absPartIdx, TEXT_CHROMA_U);
        xEncCoeffQT(cu, trDepth, absPartIdx, TEXT_CHROMA_V);
    }
    return m_pcEntropyCoder->getNumberOfWrittenBits();
}

UInt TEncSearch::xGetIntraBitsQTChroma(TComDataCU* cu, UInt trDepth, UInt absPartIdx, UInt chromaId)
{
    m_pcEntropyCoder->resetBits();
    if (chromaId == TEXT_CHROMA_U)
    {
        xEncCoeffQT(cu, trDepth, absPartIdx, TEXT_CHROMA_U);
    }
    else if (chromaId == TEXT_CHROMA_V)
    {
        xEncCoeffQT(cu, trDepth, absPartIdx, TEXT_CHROMA_V);
    }
    return m_pcEntropyCoder->getNumberOfWrittenBits();
}

Void TEncSearch::xIntraCodingLumaBlk(TComDataCU* cu,
                                     UInt        trDepth,
                                     UInt        uiAbsPartIdx,
                                     TComYuv*    fencYuv,
                                     TComYuv*    pcPredYuv,
                                     TShortYUV*  pcResiYuv,
                                     UInt&       ruiDist,
                                     Int         default0Save1Load2)
{
    UInt    uiLumaPredMode    = cu->getLumaIntraDir(uiAbsPartIdx);
    UInt    uiFullDepth       = cu->getDepth(0)  + trDepth;
    UInt    uiWidth           = cu->getWidth(0) >> trDepth;
    UInt    uiHeight          = cu->getHeight(0) >> trDepth;
    UInt    uiStride          = fencYuv->getStride();
    Pel*    piOrg             = fencYuv->getLumaAddr(uiAbsPartIdx);
    Pel*    piPred            = pcPredYuv->getLumaAddr(uiAbsPartIdx);
    Short*  piResi            = pcResiYuv->getLumaAddr(uiAbsPartIdx);
    Pel*    piReco            = pcPredYuv->getLumaAddr(uiAbsPartIdx);

    UInt    uiLog2TrSize      = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth] + 2;
    UInt    uiQTLayer         = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;
    UInt    uiNumCoeffPerInc  = cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight() >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1);
    TCoeff* pcCoeff           = m_ppcQTTempCoeffY[uiQTLayer] + uiNumCoeffPerInc * uiAbsPartIdx;

    Int*    pcArlCoeff        = m_ppcQTTempArlCoeffY[uiQTLayer] + uiNumCoeffPerInc * uiAbsPartIdx;
    Short*  piRecQt           = m_pcQTTempTComYuv[uiQTLayer].getLumaAddr(uiAbsPartIdx);
    UInt    uiRecQtStride     = m_pcQTTempTComYuv[uiQTLayer].width;

    UInt    uiZOrder          = cu->getZorderIdxInCU() + uiAbsPartIdx;
    Pel*    piRecIPred        = cu->getPic()->getPicYuvRec()->getLumaAddr(cu->getAddr(), uiZOrder);
    UInt    uiRecIPredStride  = cu->getPic()->getPicYuvRec()->getStride();
    Bool    useTransformSkip  = cu->getTransformSkip(uiAbsPartIdx, TEXT_LUMA);

    //===== init availability pattern =====

    if (default0Save1Load2 != 2)
    {
        cu->getPattern()->initPattern(cu, trDepth, uiAbsPartIdx);
        cu->getPattern()->initAdiPattern(cu, uiAbsPartIdx, trDepth, m_piPredBuf, m_iPredBufStride, m_iPredBufHeight, refAbove, refLeft, refAboveFlt, refLeftFlt);
        //===== get prediction signal =====
        predIntraLumaAng(cu->getPattern(), uiLumaPredMode, piPred, uiStride, uiWidth);
        // save prediction
        if (default0Save1Load2 == 1)
        {
            primitives.blockcpy_pp((int)uiWidth, uiHeight, (pixel*)m_pSharedPredTransformSkip[0], uiWidth, (pixel*)piPred, uiStride);
        }
    }
    else
    {
        primitives.blockcpy_pp((int)uiWidth, uiHeight, (pixel*)piPred, uiStride, (pixel*)m_pSharedPredTransformSkip[0], uiWidth);
    }

    //===== get residual signal =====

    primitives.calcresidual[(Int)g_convertToBit[uiWidth]]((pixel*)piOrg, (pixel*)piPred, piResi, uiStride);

    //===== transform and quantization =====
    //--- init rate estimation arrays for RDOQ ---
    if (useTransformSkip ? m_pcEncCfg->getUseRDOQTS() : m_pcEncCfg->getUseRDOQ())
    {
        m_pcEntropyCoder->estimateBit(m_pcTrQuant->m_estBitsSbac, uiWidth, uiWidth, TEXT_LUMA);
    }
    //--- transform and quantization ---
    UInt uiAbsSum = 0;
    cu->setTrIdxSubParts(trDepth, uiAbsPartIdx, uiFullDepth);

    m_pcTrQuant->setQPforQuant(cu->getQP(0), TEXT_LUMA, cu->getSlice()->getSPS()->getQpBDOffsetY(), 0);

    m_pcTrQuant->selectLambda(TEXT_LUMA);

    uiAbsSum = m_pcTrQuant->transformNxN(cu, piResi, uiStride, pcCoeff, pcArlCoeff, uiWidth, uiHeight, TEXT_LUMA, uiAbsPartIdx, useTransformSkip);

    //--- set coded block flag ---
    cu->setCbfSubParts((uiAbsSum ? 1 : 0) << trDepth, TEXT_LUMA, uiAbsPartIdx, uiFullDepth);
    //--- inverse transform ---
    if (uiAbsSum)
    {
        Int scalingListType = 0 + g_eTTable[(Int)TEXT_LUMA];
        assert(scalingListType < 6);
        m_pcTrQuant->invtransformNxN(cu->getCUTransquantBypass(uiAbsPartIdx), TEXT_LUMA, cu->getLumaIntraDir(uiAbsPartIdx), piResi, uiStride, pcCoeff, uiWidth, uiHeight, scalingListType, useTransformSkip);
    }
    else
    {
        Short* pResi = piResi;
        memset(pcCoeff, 0, sizeof(TCoeff) * uiWidth * uiHeight);
        for (UInt uiY = 0; uiY < uiHeight; uiY++)
        {
            memset(pResi, 0, sizeof(Short) * uiWidth);
            pResi += uiStride;
        }
    }

    //===== reconstruction =====

    primitives.calcrecon[(Int)g_convertToBit[uiWidth]]((pixel*)piPred, piResi, (pixel*)piReco, piRecQt, (pixel*)piRecIPred, uiStride, uiRecQtStride, uiRecIPredStride);

    //===== update distortion =====
    int Part = PartitionFromSizes(uiWidth, uiHeight);
    ruiDist += primitives.sse_pp[Part]((pixel*)piOrg, (intptr_t)uiStride, (pixel*)piReco, uiStride);
}

Void TEncSearch::xIntraCodingChromaBlk(TComDataCU* cu,
                                       UInt        trDepth,
                                       UInt        uiAbsPartIdx,
                                       TComYuv*    fencYuv,
                                       TComYuv*    pcPredYuv,
                                       TShortYUV*  pcResiYuv,
                                       UInt&       ruiDist,
                                       UInt        uiChromaId,
                                       Int         default0Save1Load2)
{
    UInt uiOrgTrDepth = trDepth;
    UInt uiFullDepth  = cu->getDepth(0) + trDepth;
    UInt uiLog2TrSize = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth] + 2;

    if (uiLog2TrSize == 2)
    {
        assert(trDepth > 0);
        trDepth--;
        UInt uiQPDiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trDepth) << 1);
        Bool bFirstQ = ((uiAbsPartIdx % uiQPDiv) == 0);
        if (!bFirstQ)
        {
            return;
        }
    }

    TextType  eText             = (uiChromaId > 0 ? TEXT_CHROMA_V : TEXT_CHROMA_U);
    UInt      uiChromaPredMode  = cu->getChromaIntraDir(uiAbsPartIdx);
    UInt      uiWidth           = cu->getWidth(0) >> (trDepth + 1);
    UInt      uiHeight          = cu->getHeight(0) >> (trDepth + 1);
    UInt      uiStride          = fencYuv->getCStride();
    Pel*      piOrg             = (uiChromaId > 0 ? fencYuv->getCrAddr(uiAbsPartIdx) : fencYuv->getCbAddr(uiAbsPartIdx));
    Pel*      piPred            = (uiChromaId > 0 ? pcPredYuv->getCrAddr(uiAbsPartIdx) : pcPredYuv->getCbAddr(uiAbsPartIdx));
    Short*      piResi            = (uiChromaId > 0 ? pcResiYuv->getCrAddr(uiAbsPartIdx) : pcResiYuv->getCbAddr(uiAbsPartIdx));
    Pel*      piReco            = (uiChromaId > 0 ? pcPredYuv->getCrAddr(uiAbsPartIdx) : pcPredYuv->getCbAddr(uiAbsPartIdx));

    UInt      uiQTLayer         = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;
    UInt      uiNumCoeffPerInc  = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight() >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1)) >> 2;
    TCoeff*   pcCoeff           = (uiChromaId > 0 ? m_ppcQTTempCoeffCr[uiQTLayer] : m_ppcQTTempCoeffCb[uiQTLayer]) + uiNumCoeffPerInc * uiAbsPartIdx;
    Int*      pcArlCoeff        = (uiChromaId > 0 ? m_ppcQTTempArlCoeffCr[uiQTLayer] : m_ppcQTTempArlCoeffCb[uiQTLayer]) + uiNumCoeffPerInc * uiAbsPartIdx;
    Short*      piRecQt           = (uiChromaId > 0 ? m_pcQTTempTComYuv[uiQTLayer].getCrAddr(uiAbsPartIdx) : m_pcQTTempTComYuv[uiQTLayer].getCbAddr(uiAbsPartIdx));
    UInt      uiRecQtStride     = m_pcQTTempTComYuv[uiQTLayer].Cwidth;

    UInt      uiZOrder          = cu->getZorderIdxInCU() + uiAbsPartIdx;
    Pel*      piRecIPred        = (uiChromaId > 0 ? cu->getPic()->getPicYuvRec()->getCrAddr(cu->getAddr(), uiZOrder) : cu->getPic()->getPicYuvRec()->getCbAddr(cu->getAddr(), uiZOrder));
    UInt      uiRecIPredStride  = cu->getPic()->getPicYuvRec()->getCStride();
    Bool      useTransformSkipChroma       = cu->getTransformSkip(uiAbsPartIdx, eText);
    //===== update chroma mode =====
    if (uiChromaPredMode == DM_CHROMA_IDX)
    {
        uiChromaPredMode          = cu->getLumaIntraDir(0);
    }

    //===== init availability pattern =====
    if (default0Save1Load2 != 2)
    {
        cu->getPattern()->initPattern(cu, trDepth, uiAbsPartIdx);

        cu->getPattern()->initAdiPatternChroma(cu, uiAbsPartIdx, trDepth, m_piPredBuf, m_iPredBufStride, m_iPredBufHeight);
        Pel*  pPatChroma  = (uiChromaId > 0 ? cu->getPattern()->getAdiCrBuf(uiWidth, uiHeight, m_piPredBuf) : cu->getPattern()->getAdiCbBuf(uiWidth, uiHeight, m_piPredBuf));

        //===== get prediction signal =====
        predIntraChromaAng(pPatChroma, uiChromaPredMode, piPred, uiStride, uiWidth);
        // save prediction
        if (default0Save1Load2 == 1)
        {
            Pel*  pPred   = piPred;
            Pel*  pPredBuf = m_pSharedPredTransformSkip[1 + uiChromaId];

            primitives.blockcpy_pp((int)uiWidth, uiHeight, (pixel*)pPredBuf, uiWidth, (pixel*)pPred, uiStride);
        }
    }
    else
    {
        // load prediction
        Pel*  pPred   = piPred;
        Pel*  pPredBuf = m_pSharedPredTransformSkip[1 + uiChromaId];

        primitives.blockcpy_pp((int)uiWidth, uiHeight, (pixel*)pPred, uiStride, (pixel*)pPredBuf, uiWidth);
    }
    //===== get residual signal =====

    primitives.calcresidual[(Int)g_convertToBit[uiWidth]]((pixel*)piOrg, (pixel*)piPred, piResi, uiStride);

    //===== transform and quantization =====
    {
        //--- init rate estimation arrays for RDOQ ---
        if (useTransformSkipChroma ? m_pcEncCfg->getUseRDOQTS() : m_pcEncCfg->getUseRDOQ())
        {
            m_pcEntropyCoder->estimateBit(m_pcTrQuant->m_estBitsSbac, uiWidth, uiWidth, eText);
        }
        //--- transform and quantization ---
        UInt uiAbsSum = 0;

        Int curChromaQpOffset;
        if (eText == TEXT_CHROMA_U)
        {
            curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCbQpOffset() + cu->getSlice()->getSliceQpDeltaCb();
        }
        else
        {
            curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCrQpOffset() + cu->getSlice()->getSliceQpDeltaCr();
        }
        m_pcTrQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset);

        m_pcTrQuant->selectLambda(TEXT_CHROMA);

        uiAbsSum = m_pcTrQuant->transformNxN(cu, piResi, uiStride, pcCoeff, pcArlCoeff, uiWidth, uiHeight, eText, uiAbsPartIdx, useTransformSkipChroma);
        //--- set coded block flag ---
        cu->setCbfSubParts((uiAbsSum ? 1 : 0) << uiOrgTrDepth, eText, uiAbsPartIdx, cu->getDepth(0) + trDepth);
        //--- inverse transform ---
        if (uiAbsSum)
        {
            Int scalingListType = 0 + g_eTTable[(Int)eText];
            assert(scalingListType < 6);
            m_pcTrQuant->invtransformNxN(cu->getCUTransquantBypass(uiAbsPartIdx), TEXT_CHROMA, REG_DCT, piResi, uiStride, pcCoeff, uiWidth, uiHeight, scalingListType, useTransformSkipChroma);
        }
        else
        {
            Short* pResi = piResi;
            memset(pcCoeff, 0, sizeof(TCoeff) * uiWidth * uiHeight);
            for (UInt uiY = 0; uiY < uiHeight; uiY++)
            {
                memset(pResi, 0, sizeof(Short) * uiWidth);
                pResi += uiStride;
            }
        }
    }

    //===== reconstruction =====

    primitives.calcrecon[(Int)g_convertToBit[uiWidth]]((pixel*)piPred, piResi, (pixel*)piReco, piRecQt, (pixel*)piRecIPred, uiStride, uiRecQtStride, uiRecIPredStride);

    //===== update distortion =====
    int Part = x265::PartitionFromSizes(uiWidth, uiHeight);
    UInt Cost = primitives.sse_pp[Part]((pixel*)piOrg, (intptr_t)uiStride, (pixel*)piReco, uiStride);
    if (eText == TEXT_CHROMA_U)
    {
        ruiDist += m_pcRdCost->scaleChromaDistCb(Cost);
    }
    else if (eText == TEXT_CHROMA_V)
    {
        ruiDist += m_pcRdCost->scaleChromaDistCr(Cost);
    }
    else
    {
        ruiDist += Cost;
    }
}

Void TEncSearch::xRecurIntraCodingQT(TComDataCU* cu,
                                     UInt        trDepth,
                                     UInt        uiAbsPartIdx,
                                     Bool        bLumaOnly,
                                     TComYuv*    fencYuv,
                                     TComYuv*    pcPredYuv,
                                     TShortYUV*  pcResiYuv,
                                     UInt&       ruiDistY,
                                     UInt&       ruiDistC,
                                     Bool        bCheckFirst,
                                     UInt64&     dRDCost)
{
    UInt    uiFullDepth   = cu->getDepth(0) +  trDepth;
    UInt    uiLog2TrSize  = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth] + 2;
    Bool    bCheckFull    = (uiLog2TrSize  <= cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize());
    Bool    bCheckSplit   = (uiLog2TrSize  >  cu->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx));

    Int maxTuSize = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize();
    Int isIntraSlice = (cu->getSlice()->getSliceType() == I_SLICE);
    // don't check split if TU size is less or equal to max TU size
    Bool noSplitIntraMaxTuSize = bCheckFull;

    if (m_pcEncCfg->getRDpenalty() && !isIntraSlice)
    {
        // in addition don't check split if TU size is less or equal to 16x16 TU size for non-intra slice
        noSplitIntraMaxTuSize = (uiLog2TrSize  <= min(maxTuSize, 4));

        // if maximum RD-penalty don't check TU size 32x32
        if (m_pcEncCfg->getRDpenalty() == 2)
        {
            bCheckFull = (uiLog2TrSize  <= min(maxTuSize, 4));
        }
    }
    if (bCheckFirst && noSplitIntraMaxTuSize)
    {
        bCheckSplit = false;
    }

    UInt64  dSingleCost   = MAX_INT64;
    UInt    uiSingleDistY = 0;
    UInt    uiSingleDistC = 0;
    UInt    uiSingleCbfY  = 0;
    UInt    uiSingleCbfU  = 0;
    UInt    uiSingleCbfV  = 0;
    Bool    checkTransformSkip  = cu->getSlice()->getPPS()->getUseTransformSkip();
    UInt    widthTransformSkip  = cu->getWidth(0) >> trDepth;
    UInt    heightTransformSkip = cu->getHeight(0) >> trDepth;
    Int     bestModeId    = 0;
    Int     bestModeIdUV[2] = { 0, 0 };

    checkTransformSkip &= (widthTransformSkip == 4 && heightTransformSkip == 4);
    checkTransformSkip &= (!cu->getCUTransquantBypass(0));
    checkTransformSkip &= (!((cu->getQP(0) == 0) && (cu->getSlice()->getSPS()->getUseLossless())));
    if (m_pcEncCfg->getUseTransformSkipFast())
    {
        checkTransformSkip &= (cu->getPartitionSize(uiAbsPartIdx) == SIZE_NxN);
    }
    if (bCheckFull)
    {
        if (checkTransformSkip == true)
        {
            //----- store original entropy coding status -----
            m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiFullDepth][CI_QT_TRAFO_ROOT]);

            UInt   singleDistYTmp     = 0;
            UInt   singleDistCTmp     = 0;
            UInt   singleCbfYTmp      = 0;
            UInt   singleCbfUTmp      = 0;
            UInt   singleCbfVTmp      = 0;
            UInt64 singleCostTmp      = 0;
            Int    default0Save1Load2 = 0;
            Int    firstCheckId       = 0;

            UInt   uiQPDiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + (trDepth - 1)) << 1);
            Bool   bFirstQ = ((uiAbsPartIdx % uiQPDiv) == 0);

            for (Int modeId = firstCheckId; modeId < 2; modeId++)
            {
                singleDistYTmp = 0;
                singleDistCTmp = 0;
                cu->setTransformSkipSubParts(modeId, TEXT_LUMA, uiAbsPartIdx, uiFullDepth);
                if (modeId == firstCheckId)
                {
                    default0Save1Load2 = 1;
                }
                else
                {
                    default0Save1Load2 = 2;
                }
                //----- code luma block with given intra prediction mode and store Cbf-----
                xIntraCodingLumaBlk(cu, trDepth, uiAbsPartIdx, fencYuv, pcPredYuv, pcResiYuv, singleDistYTmp, default0Save1Load2);
                singleCbfYTmp = cu->getCbf(uiAbsPartIdx, TEXT_LUMA, trDepth);
                //----- code chroma blocks with given intra prediction mode and store Cbf-----
                if (!bLumaOnly)
                {
                    if (bFirstQ)
                    {
                        cu->setTransformSkipSubParts(modeId, TEXT_CHROMA_U, uiAbsPartIdx, uiFullDepth);
                        cu->setTransformSkipSubParts(modeId, TEXT_CHROMA_V, uiAbsPartIdx, uiFullDepth);
                    }
                    xIntraCodingChromaBlk(cu, trDepth, uiAbsPartIdx, fencYuv, pcPredYuv, pcResiYuv, singleDistCTmp, 0, default0Save1Load2);
                    xIntraCodingChromaBlk(cu, trDepth, uiAbsPartIdx, fencYuv, pcPredYuv, pcResiYuv, singleDistCTmp, 1, default0Save1Load2);
                    singleCbfUTmp = cu->getCbf(uiAbsPartIdx, TEXT_CHROMA_U, trDepth);
                    singleCbfVTmp = cu->getCbf(uiAbsPartIdx, TEXT_CHROMA_V, trDepth);
                }
                //----- determine rate and r-d cost -----
                if (modeId == 1 && singleCbfYTmp == 0)
                {
                    //In order not to code TS flag when cbf is zero, the case for TS with cbf being zero is forbidden.
                    singleCostTmp = MAX_INT64;
                }
                else
                {
                    UInt uiSingleBits = xGetIntraBitsQT(cu, trDepth, uiAbsPartIdx, true, !bLumaOnly);
                    singleCostTmp = m_pcRdCost->calcRdCost(singleDistYTmp + singleDistCTmp, uiSingleBits);
                }

                if (singleCostTmp < dSingleCost)
                {
                    dSingleCost   = singleCostTmp;
                    uiSingleDistY = singleDistYTmp;
                    uiSingleDistC = singleDistCTmp;
                    uiSingleCbfY  = singleCbfYTmp;
                    uiSingleCbfU  = singleCbfUTmp;
                    uiSingleCbfV  = singleCbfVTmp;
                    bestModeId    = modeId;
                    if (bestModeId == firstCheckId)
                    {
                        xStoreIntraResultQT(cu, trDepth, uiAbsPartIdx, bLumaOnly);
                        m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiFullDepth][CI_TEMP_BEST]);
                    }
                }
                if (modeId == firstCheckId)
                {
                    m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiFullDepth][CI_QT_TRAFO_ROOT]);
                }
            }

            cu->setTransformSkipSubParts(bestModeId, TEXT_LUMA, uiAbsPartIdx, uiFullDepth);

            if (bestModeId == firstCheckId)
            {
                xLoadIntraResultQT(cu, trDepth, uiAbsPartIdx, bLumaOnly);
                cu->setCbfSubParts(uiSingleCbfY << trDepth, TEXT_LUMA, uiAbsPartIdx, uiFullDepth);
                if (!bLumaOnly)
                {
                    if (bFirstQ)
                    {
                        cu->setCbfSubParts(uiSingleCbfU << trDepth, TEXT_CHROMA_U, uiAbsPartIdx, cu->getDepth(0) + trDepth - 1);
                        cu->setCbfSubParts(uiSingleCbfV << trDepth, TEXT_CHROMA_V, uiAbsPartIdx, cu->getDepth(0) + trDepth - 1);
                    }
                }
                m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiFullDepth][CI_TEMP_BEST]);
            }

            if (!bLumaOnly)
            {
                bestModeIdUV[0] = bestModeIdUV[1] = bestModeId;
                if (bFirstQ && bestModeId == 1)
                {
                    //In order not to code TS flag when cbf is zero, the case for TS with cbf being zero is forbidden.
                    if (uiSingleCbfU == 0)
                    {
                        cu->setTransformSkipSubParts(0, TEXT_CHROMA_U, uiAbsPartIdx, uiFullDepth);
                        bestModeIdUV[0] = 0;
                    }
                    if (uiSingleCbfV == 0)
                    {
                        cu->setTransformSkipSubParts(0, TEXT_CHROMA_V, uiAbsPartIdx, uiFullDepth);
                        bestModeIdUV[1] = 0;
                    }
                }
            }
        }
        else
        {
            cu->setTransformSkipSubParts(0, TEXT_LUMA, uiAbsPartIdx, uiFullDepth);

            //----- store original entropy coding status -----
            m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiFullDepth][CI_QT_TRAFO_ROOT]);

            //----- code luma block with given intra prediction mode and store Cbf-----
            dSingleCost   = 0.0;
            xIntraCodingLumaBlk(cu, trDepth, uiAbsPartIdx, fencYuv, pcPredYuv, pcResiYuv, uiSingleDistY);
            if (bCheckSplit)
            {
                uiSingleCbfY = cu->getCbf(uiAbsPartIdx, TEXT_LUMA, trDepth);
            }
            //----- code chroma blocks with given intra prediction mode and store Cbf-----
            if (!bLumaOnly)
            {
                cu->setTransformSkipSubParts(0, TEXT_CHROMA_U, uiAbsPartIdx, uiFullDepth);
                cu->setTransformSkipSubParts(0, TEXT_CHROMA_V, uiAbsPartIdx, uiFullDepth);
                xIntraCodingChromaBlk(cu, trDepth, uiAbsPartIdx, fencYuv, pcPredYuv, pcResiYuv, uiSingleDistC, 0);
                xIntraCodingChromaBlk(cu, trDepth, uiAbsPartIdx, fencYuv, pcPredYuv, pcResiYuv, uiSingleDistC, 1);
                if (bCheckSplit)
                {
                    uiSingleCbfU = cu->getCbf(uiAbsPartIdx, TEXT_CHROMA_U, trDepth);
                    uiSingleCbfV = cu->getCbf(uiAbsPartIdx, TEXT_CHROMA_V, trDepth);
                }
            }
            //----- determine rate and r-d cost -----
            UInt uiSingleBits = xGetIntraBitsQT(cu, trDepth, uiAbsPartIdx, true, !bLumaOnly);
            if (m_pcEncCfg->getRDpenalty() && (uiLog2TrSize == 5) && !isIntraSlice)
            {
                uiSingleBits = uiSingleBits * 4;
            }
            dSingleCost = m_pcRdCost->calcRdCost(uiSingleDistY + uiSingleDistC, uiSingleBits);
        }
    }

    if (bCheckSplit)
    {
        //----- store full entropy coding status, load original entropy coding status -----
        if (bCheckFull)
        {
            m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiFullDepth][CI_QT_TRAFO_TEST]);
            m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiFullDepth][CI_QT_TRAFO_ROOT]);
        }
        else
        {
            m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiFullDepth][CI_QT_TRAFO_ROOT]);
        }

        //----- code splitted block -----
        UInt64  dSplitCost      = 0.0;
        UInt    uiSplitDistY    = 0;
        UInt    uiSplitDistC    = 0;
        UInt    uiQPartsDiv     = cu->getPic()->getNumPartInCU() >> ((uiFullDepth + 1) << 1);
        UInt    uiAbsPartIdxSub = uiAbsPartIdx;

        UInt    uiSplitCbfY = 0;
        UInt    uiSplitCbfU = 0;
        UInt    uiSplitCbfV = 0;

        for (UInt uiPart = 0; uiPart < 4; uiPart++, uiAbsPartIdxSub += uiQPartsDiv)
        {
            xRecurIntraCodingQT(cu, trDepth + 1, uiAbsPartIdxSub, bLumaOnly, fencYuv, pcPredYuv, pcResiYuv, uiSplitDistY, uiSplitDistC, bCheckFirst, dSplitCost);

            uiSplitCbfY |= cu->getCbf(uiAbsPartIdxSub, TEXT_LUMA, trDepth + 1);
            if (!bLumaOnly)
            {
                uiSplitCbfU |= cu->getCbf(uiAbsPartIdxSub, TEXT_CHROMA_U, trDepth + 1);
                uiSplitCbfV |= cu->getCbf(uiAbsPartIdxSub, TEXT_CHROMA_V, trDepth + 1);
            }
        }

        for (UInt uiOffs = 0; uiOffs < 4 * uiQPartsDiv; uiOffs++)
        {
            cu->getCbf(TEXT_LUMA)[uiAbsPartIdx + uiOffs] |= (uiSplitCbfY << trDepth);
        }

        if (!bLumaOnly)
        {
            for (UInt uiOffs = 0; uiOffs < 4 * uiQPartsDiv; uiOffs++)
            {
                cu->getCbf(TEXT_CHROMA_U)[uiAbsPartIdx + uiOffs] |= (uiSplitCbfU << trDepth);
                cu->getCbf(TEXT_CHROMA_V)[uiAbsPartIdx + uiOffs] |= (uiSplitCbfV << trDepth);
            }
        }
        //----- restore context states -----
        m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiFullDepth][CI_QT_TRAFO_ROOT]);

        //----- determine rate and r-d cost -----
        UInt uiSplitBits = xGetIntraBitsQT(cu, trDepth, uiAbsPartIdx, true, !bLumaOnly);
        dSplitCost       = m_pcRdCost->calcRdCost(uiSplitDistY + uiSplitDistC, uiSplitBits);

        //===== compare and set best =====
        if (dSplitCost < dSingleCost)
        {
            //--- update cost ---
            ruiDistY += uiSplitDistY;
            ruiDistC += uiSplitDistC;
            dRDCost  += dSplitCost;
            return;
        }

        //----- set entropy coding status -----
        m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiFullDepth][CI_QT_TRAFO_TEST]);

        //--- set transform index and Cbf values ---
        cu->setTrIdxSubParts(trDepth, uiAbsPartIdx, uiFullDepth);
        cu->setCbfSubParts(uiSingleCbfY << trDepth, TEXT_LUMA, uiAbsPartIdx, uiFullDepth);
        cu->setTransformSkipSubParts(bestModeId, TEXT_LUMA, uiAbsPartIdx, uiFullDepth);
        if (!bLumaOnly)
        {
            cu->setCbfSubParts(uiSingleCbfU << trDepth, TEXT_CHROMA_U, uiAbsPartIdx, uiFullDepth);
            cu->setCbfSubParts(uiSingleCbfV << trDepth, TEXT_CHROMA_V, uiAbsPartIdx, uiFullDepth);
            cu->setTransformSkipSubParts(bestModeIdUV[0], TEXT_CHROMA_U, uiAbsPartIdx, uiFullDepth);
            cu->setTransformSkipSubParts(bestModeIdUV[1], TEXT_CHROMA_V, uiAbsPartIdx, uiFullDepth);
        }

        //--- set reconstruction for next intra prediction blocks ---
        UInt  uiWidth     = cu->getWidth(0) >> trDepth;
        UInt  uiHeight    = cu->getHeight(0) >> trDepth;
        UInt  uiQTLayer   = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;
        UInt  uiZOrder    = cu->getZorderIdxInCU() + uiAbsPartIdx;
        Short*  piSrc     = m_pcQTTempTComYuv[uiQTLayer].getLumaAddr(uiAbsPartIdx);
        UInt  uiSrcStride = m_pcQTTempTComYuv[uiQTLayer].width;
        Pel*  piDes       = cu->getPic()->getPicYuvRec()->getLumaAddr(cu->getAddr(), uiZOrder);
        UInt  uiDesStride = cu->getPic()->getPicYuvRec()->getStride();
        for (UInt uiY = 0; uiY < uiHeight; uiY++, piSrc += uiSrcStride, piDes += uiDesStride)
        {
            for (UInt uiX = 0; uiX < uiWidth; uiX++)
            {
                piDes[uiX] = (Pel)(piSrc[uiX]);
            }
        }

        if (!bLumaOnly)
        {
            uiWidth   >>= 1;
            uiHeight  >>= 1;
            piSrc       = m_pcQTTempTComYuv[uiQTLayer].getCbAddr(uiAbsPartIdx);
            uiSrcStride = m_pcQTTempTComYuv[uiQTLayer].Cwidth;
            piDes       = cu->getPic()->getPicYuvRec()->getCbAddr(cu->getAddr(), uiZOrder);
            uiDesStride = cu->getPic()->getPicYuvRec()->getCStride();
            for (UInt uiY = 0; uiY < uiHeight; uiY++, piSrc += uiSrcStride, piDes += uiDesStride)
            {
                for (UInt uiX = 0; uiX < uiWidth; uiX++)
                {
                    piDes[uiX] = piSrc[uiX];
                }
            }

            piSrc = m_pcQTTempTComYuv[uiQTLayer].getCrAddr(uiAbsPartIdx);
            piDes = cu->getPic()->getPicYuvRec()->getCrAddr(cu->getAddr(), uiZOrder);
            for (UInt uiY = 0; uiY < uiHeight; uiY++, piSrc += uiSrcStride, piDes += uiDesStride)
            {
                for (UInt uiX = 0; uiX < uiWidth; uiX++)
                {
                    piDes[uiX] = piSrc[uiX];
                }
            }
        }
    }
    ruiDistY += uiSingleDistY;
    ruiDistC += uiSingleDistC;
    dRDCost  += dSingleCost;
}

Void TEncSearch::xSetIntraResultQT(TComDataCU* cu,
                                   UInt        trDepth,
                                   UInt        uiAbsPartIdx,
                                   Bool        bLumaOnly,
                                   TComYuv*    pcRecoYuv)
{
    UInt uiFullDepth  = cu->getDepth(0) + trDepth;
    UInt uiTrMode     = cu->getTransformIdx(uiAbsPartIdx);

    if (uiTrMode == trDepth)
    {
        UInt uiLog2TrSize = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth] + 2;
        UInt uiQTLayer    = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;

        Bool bSkipChroma  = false;
        Bool bChromaSame  = false;
        if (!bLumaOnly && uiLog2TrSize == 2)
        {
            assert(trDepth > 0);
            UInt uiQPDiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trDepth - 1) << 1);
            bSkipChroma  = ((uiAbsPartIdx % uiQPDiv) != 0);
            bChromaSame  = true;
        }

        //===== copy transform coefficients =====
        UInt uiNumCoeffY    = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> (uiFullDepth << 1);
        UInt uiNumCoeffIncY = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1);
        TCoeff* pcCoeffSrcY = m_ppcQTTempCoeffY[uiQTLayer] + (uiNumCoeffIncY * uiAbsPartIdx);
        TCoeff* pcCoeffDstY = cu->getCoeffY()              + (uiNumCoeffIncY * uiAbsPartIdx);
        ::memcpy(pcCoeffDstY, pcCoeffSrcY, sizeof(TCoeff) * uiNumCoeffY);
        Int* pcArlCoeffSrcY = m_ppcQTTempArlCoeffY[uiQTLayer] + (uiNumCoeffIncY * uiAbsPartIdx);
        Int* pcArlCoeffDstY = cu->getArlCoeffY()              + (uiNumCoeffIncY * uiAbsPartIdx);
        ::memcpy(pcArlCoeffDstY, pcArlCoeffSrcY, sizeof(Int) * uiNumCoeffY);
        if (!bLumaOnly && !bSkipChroma)
        {
            UInt uiNumCoeffC    = (bChromaSame ? uiNumCoeffY    : uiNumCoeffY    >> 2);
            UInt uiNumCoeffIncC = uiNumCoeffIncY >> 2;
            TCoeff* pcCoeffSrcU = m_ppcQTTempCoeffCb[uiQTLayer] + (uiNumCoeffIncC * uiAbsPartIdx);
            TCoeff* pcCoeffSrcV = m_ppcQTTempCoeffCr[uiQTLayer] + (uiNumCoeffIncC * uiAbsPartIdx);
            TCoeff* pcCoeffDstU = cu->getCoeffCb()              + (uiNumCoeffIncC * uiAbsPartIdx);
            TCoeff* pcCoeffDstV = cu->getCoeffCr()              + (uiNumCoeffIncC * uiAbsPartIdx);
            ::memcpy(pcCoeffDstU, pcCoeffSrcU, sizeof(TCoeff) * uiNumCoeffC);
            ::memcpy(pcCoeffDstV, pcCoeffSrcV, sizeof(TCoeff) * uiNumCoeffC);
            Int* pcArlCoeffSrcU = m_ppcQTTempArlCoeffCb[uiQTLayer] + (uiNumCoeffIncC * uiAbsPartIdx);
            Int* pcArlCoeffSrcV = m_ppcQTTempArlCoeffCr[uiQTLayer] + (uiNumCoeffIncC * uiAbsPartIdx);
            Int* pcArlCoeffDstU = cu->getArlCoeffCb()              + (uiNumCoeffIncC * uiAbsPartIdx);
            Int* pcArlCoeffDstV = cu->getArlCoeffCr()              + (uiNumCoeffIncC * uiAbsPartIdx);
            ::memcpy(pcArlCoeffDstU, pcArlCoeffSrcU, sizeof(Int) * uiNumCoeffC);
            ::memcpy(pcArlCoeffDstV, pcArlCoeffSrcV, sizeof(Int) * uiNumCoeffC);
        }

        //===== copy reconstruction =====
        m_pcQTTempTComYuv[uiQTLayer].copyPartToPartLuma(pcRecoYuv, uiAbsPartIdx, 1 << uiLog2TrSize, 1 << uiLog2TrSize);
        if (!bLumaOnly && !bSkipChroma)
        {
            UInt uiLog2TrSizeChroma = (bChromaSame ? uiLog2TrSize : uiLog2TrSize - 1);
            m_pcQTTempTComYuv[uiQTLayer].copyPartToPartChroma(pcRecoYuv, uiAbsPartIdx, 1 << uiLog2TrSizeChroma, 1 << uiLog2TrSizeChroma);
        }
    }
    else
    {
        UInt uiNumQPart  = cu->getPic()->getNumPartInCU() >> ((uiFullDepth + 1) << 1);
        for (UInt uiPart = 0; uiPart < 4; uiPart++)
        {
            xSetIntraResultQT(cu, trDepth + 1, uiAbsPartIdx + uiPart * uiNumQPart, bLumaOnly, pcRecoYuv);
        }
    }
}

Void TEncSearch::xStoreIntraResultQT(TComDataCU* cu,
                                     UInt        trDepth,
                                     UInt        uiAbsPartIdx,
                                     Bool        bLumaOnly)
{
    UInt uiFullDepth  = cu->getDepth(0) + trDepth;
    UInt uiTrMode     = cu->getTransformIdx(uiAbsPartIdx);

    assert(uiTrMode == trDepth);
    UInt uiLog2TrSize = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth] + 2;
    UInt uiQTLayer    = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;

    Bool bSkipChroma  = false;
    Bool bChromaSame  = false;
    if (!bLumaOnly && uiLog2TrSize == 2)
    {
        assert(trDepth > 0);
        UInt uiQPDiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trDepth - 1) << 1);
        bSkipChroma  = ((uiAbsPartIdx % uiQPDiv) != 0);
        bChromaSame  = true;
    }

    //===== copy transform coefficients =====
    UInt uiNumCoeffY    = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> (uiFullDepth << 1);
    UInt uiNumCoeffIncY = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1);
    TCoeff* pcCoeffSrcY = m_ppcQTTempCoeffY[uiQTLayer] + (uiNumCoeffIncY * uiAbsPartIdx);
    TCoeff* pcCoeffDstY = m_pcQTTempTUCoeffY;

    ::memcpy(pcCoeffDstY, pcCoeffSrcY, sizeof(TCoeff) * uiNumCoeffY);
    Int* pcArlCoeffSrcY = m_ppcQTTempArlCoeffY[uiQTLayer] + (uiNumCoeffIncY * uiAbsPartIdx);
    Int* pcArlCoeffDstY = m_ppcQTTempTUArlCoeffY;
    ::memcpy(pcArlCoeffDstY, pcArlCoeffSrcY, sizeof(Int) * uiNumCoeffY);
    if (!bLumaOnly && !bSkipChroma)
    {
        UInt uiNumCoeffC    = (bChromaSame ? uiNumCoeffY    : uiNumCoeffY    >> 2);
        UInt uiNumCoeffIncC = uiNumCoeffIncY >> 2;
        TCoeff* pcCoeffSrcU = m_ppcQTTempCoeffCb[uiQTLayer] + (uiNumCoeffIncC * uiAbsPartIdx);
        TCoeff* pcCoeffSrcV = m_ppcQTTempCoeffCr[uiQTLayer] + (uiNumCoeffIncC * uiAbsPartIdx);
        TCoeff* pcCoeffDstU = m_pcQTTempTUCoeffCb;
        TCoeff* pcCoeffDstV = m_pcQTTempTUCoeffCr;
        ::memcpy(pcCoeffDstU, pcCoeffSrcU, sizeof(TCoeff) * uiNumCoeffC);
        ::memcpy(pcCoeffDstV, pcCoeffSrcV, sizeof(TCoeff) * uiNumCoeffC);
        Int* pcArlCoeffSrcU = m_ppcQTTempArlCoeffCb[uiQTLayer] + (uiNumCoeffIncC * uiAbsPartIdx);
        Int* pcArlCoeffSrcV = m_ppcQTTempArlCoeffCr[uiQTLayer] + (uiNumCoeffIncC * uiAbsPartIdx);
        Int* pcArlCoeffDstU = m_ppcQTTempTUArlCoeffCb;
        Int* pcArlCoeffDstV = m_ppcQTTempTUArlCoeffCr;
        ::memcpy(pcArlCoeffDstU, pcArlCoeffSrcU, sizeof(Int) * uiNumCoeffC);
        ::memcpy(pcArlCoeffDstV, pcArlCoeffSrcV, sizeof(Int) * uiNumCoeffC);
    }

    //===== copy reconstruction =====
    m_pcQTTempTComYuv[uiQTLayer].copyPartToPartLuma(&m_pcQTTempTransformSkipTComYuv, uiAbsPartIdx, 1 << uiLog2TrSize, 1 << uiLog2TrSize);

    if (!bLumaOnly && !bSkipChroma)
    {
        UInt uiLog2TrSizeChroma = (bChromaSame ? uiLog2TrSize : uiLog2TrSize - 1);
        m_pcQTTempTComYuv[uiQTLayer].copyPartToPartChroma(&m_pcQTTempTransformSkipTComYuv, uiAbsPartIdx, 1 << uiLog2TrSizeChroma, 1 << uiLog2TrSizeChroma);
    }
}

Void TEncSearch::xLoadIntraResultQT(TComDataCU* cu,
                                    UInt        trDepth,
                                    UInt        uiAbsPartIdx,
                                    Bool        bLumaOnly)
{
    UInt uiFullDepth  = cu->getDepth(0) + trDepth;
    UInt uiTrMode     = cu->getTransformIdx(uiAbsPartIdx);

    assert(uiTrMode == trDepth);
    UInt uiLog2TrSize = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth] + 2;
    UInt uiQTLayer    = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;

    Bool bSkipChroma  = false;
    Bool bChromaSame  = false;
    if (!bLumaOnly && uiLog2TrSize == 2)
    {
        assert(trDepth > 0);
        UInt uiQPDiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trDepth - 1) << 1);
        bSkipChroma  = ((uiAbsPartIdx % uiQPDiv) != 0);
        bChromaSame  = true;
    }

    //===== copy transform coefficients =====
    UInt uiNumCoeffY    = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> (uiFullDepth << 1);
    UInt uiNumCoeffIncY = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1);
    TCoeff* pcCoeffDstY = m_ppcQTTempCoeffY[uiQTLayer] + (uiNumCoeffIncY * uiAbsPartIdx);
    TCoeff* pcCoeffSrcY = m_pcQTTempTUCoeffY;

    ::memcpy(pcCoeffDstY, pcCoeffSrcY, sizeof(TCoeff) * uiNumCoeffY);
    Int* pcArlCoeffDstY = m_ppcQTTempArlCoeffY[uiQTLayer] + (uiNumCoeffIncY * uiAbsPartIdx);
    Int* pcArlCoeffSrcY = m_ppcQTTempTUArlCoeffY;
    ::memcpy(pcArlCoeffDstY, pcArlCoeffSrcY, sizeof(Int) * uiNumCoeffY);
    if (!bLumaOnly && !bSkipChroma)
    {
        UInt uiNumCoeffC    = (bChromaSame ? uiNumCoeffY    : uiNumCoeffY    >> 2);
        UInt uiNumCoeffIncC = uiNumCoeffIncY >> 2;
        TCoeff* pcCoeffDstU = m_ppcQTTempCoeffCb[uiQTLayer] + (uiNumCoeffIncC * uiAbsPartIdx);
        TCoeff* pcCoeffDstV = m_ppcQTTempCoeffCr[uiQTLayer] + (uiNumCoeffIncC * uiAbsPartIdx);
        TCoeff* pcCoeffSrcU = m_pcQTTempTUCoeffCb;
        TCoeff* pcCoeffSrcV = m_pcQTTempTUCoeffCr;
        ::memcpy(pcCoeffDstU, pcCoeffSrcU, sizeof(TCoeff) * uiNumCoeffC);
        ::memcpy(pcCoeffDstV, pcCoeffSrcV, sizeof(TCoeff) * uiNumCoeffC);
        Int* pcArlCoeffDstU = m_ppcQTTempArlCoeffCb[uiQTLayer] + (uiNumCoeffIncC * uiAbsPartIdx);
        Int* pcArlCoeffDstV = m_ppcQTTempArlCoeffCr[uiQTLayer] + (uiNumCoeffIncC * uiAbsPartIdx);
        Int* pcArlCoeffSrcU = m_ppcQTTempTUArlCoeffCb;
        Int* pcArlCoeffSrcV = m_ppcQTTempTUArlCoeffCr;
        ::memcpy(pcArlCoeffDstU, pcArlCoeffSrcU, sizeof(Int) * uiNumCoeffC);
        ::memcpy(pcArlCoeffDstV, pcArlCoeffSrcV, sizeof(Int) * uiNumCoeffC);
    }

    //===== copy reconstruction =====
    m_pcQTTempTransformSkipTComYuv.copyPartToPartLuma(&m_pcQTTempTComYuv[uiQTLayer], uiAbsPartIdx, 1 << uiLog2TrSize, 1 << uiLog2TrSize);

    if (!bLumaOnly && !bSkipChroma)
    {
        UInt uiLog2TrSizeChroma = (bChromaSame ? uiLog2TrSize : uiLog2TrSize - 1);
        m_pcQTTempTransformSkipTComYuv.copyPartToPartChroma(&m_pcQTTempTComYuv[uiQTLayer], uiAbsPartIdx, 1 << uiLog2TrSizeChroma, 1 << uiLog2TrSizeChroma);
    }

    UInt    uiZOrder          = cu->getZorderIdxInCU() + uiAbsPartIdx;
    Pel*    piRecIPred        = cu->getPic()->getPicYuvRec()->getLumaAddr(cu->getAddr(), uiZOrder);
    UInt    uiRecIPredStride  = cu->getPic()->getPicYuvRec()->getStride();
    Short*    piRecQt           = m_pcQTTempTComYuv[uiQTLayer].getLumaAddr(uiAbsPartIdx);
    UInt    uiRecQtStride     = m_pcQTTempTComYuv[uiQTLayer].width;
    UInt    uiWidth           = cu->getWidth(0) >> trDepth;
    UInt    uiHeight          = cu->getHeight(0) >> trDepth;
    Short* pRecQt     = piRecQt;
    Pel* pRecIPred  = piRecIPred;
    for (UInt uiY = 0; uiY < uiHeight; uiY++)
    {
        for (UInt uiX = 0; uiX < uiWidth; uiX++)
        {
            pRecIPred[uiX] = (Pel)pRecQt[uiX];
        }

        pRecQt    += uiRecQtStride;
        pRecIPred += uiRecIPredStride;
    }

    if (!bLumaOnly && !bSkipChroma)
    {
        piRecIPred = cu->getPic()->getPicYuvRec()->getCbAddr(cu->getAddr(), uiZOrder);
        piRecQt    = m_pcQTTempTComYuv[uiQTLayer].getCbAddr(uiAbsPartIdx);
        pRecQt     = piRecQt;
        pRecIPred  = piRecIPred;
        for (UInt uiY = 0; uiY < uiHeight; uiY++)
        {
            for (UInt uiX = 0; uiX < uiWidth; uiX++)
            {
                pRecIPred[uiX] = (Pel)pRecQt[uiX];
            }

            pRecQt    += uiRecQtStride;
            pRecIPred += uiRecIPredStride;
        }

        piRecIPred = cu->getPic()->getPicYuvRec()->getCrAddr(cu->getAddr(), uiZOrder);
        piRecQt    = m_pcQTTempTComYuv[uiQTLayer].getCrAddr(uiAbsPartIdx);
        pRecQt     = piRecQt;
        pRecIPred  = piRecIPred;
        for (UInt uiY = 0; uiY < uiHeight; uiY++)
        {
            for (UInt uiX = 0; uiX < uiWidth; uiX++)
            {
                pRecIPred[uiX] = (Pel)pRecQt[uiX];
            }

            pRecQt    += uiRecQtStride;
            pRecIPred += uiRecIPredStride;
        }
    }
}

Void TEncSearch::xStoreIntraResultChromaQT(TComDataCU* cu,
                                           UInt        trDepth,
                                           UInt        uiAbsPartIdx,
                                           UInt        stateU0V1Both2)
{
    UInt uiFullDepth = cu->getDepth(0) + trDepth;
    UInt uiTrMode    = cu->getTransformIdx(uiAbsPartIdx);

    if (uiTrMode == trDepth)
    {
        UInt uiLog2TrSize = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth] + 2;
        UInt uiQTLayer    = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;

        Bool bChromaSame = false;
        if (uiLog2TrSize == 2)
        {
            assert(trDepth > 0);
            trDepth--;
            UInt uiQPDiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trDepth) << 1);
            if ((uiAbsPartIdx % uiQPDiv) != 0)
            {
                return;
            }
            bChromaSame = true;
        }

        //===== copy transform coefficients =====
        UInt uiNumCoeffC    = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> (uiFullDepth << 1);
        if (!bChromaSame)
        {
            uiNumCoeffC     >>= 2;
        }
        UInt uiNumCoeffIncC = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> ((cu->getSlice()->getSPS()->getMaxCUDepth() << 1) + 2);
        if (stateU0V1Both2 == 0 || stateU0V1Both2 == 2)
        {
            TCoeff* pcCoeffSrcU = m_ppcQTTempCoeffCb[uiQTLayer] + (uiNumCoeffIncC * uiAbsPartIdx);
            TCoeff* pcCoeffDstU = m_pcQTTempTUCoeffCb;
            ::memcpy(pcCoeffDstU, pcCoeffSrcU, sizeof(TCoeff) * uiNumCoeffC);

            Int* pcArlCoeffSrcU = m_ppcQTTempArlCoeffCb[uiQTLayer] + (uiNumCoeffIncC * uiAbsPartIdx);
            Int* pcArlCoeffDstU = m_ppcQTTempTUArlCoeffCb;
            ::memcpy(pcArlCoeffDstU, pcArlCoeffSrcU, sizeof(Int) * uiNumCoeffC);
        }
        if (stateU0V1Both2 == 1 || stateU0V1Both2 == 2)
        {
            TCoeff* pcCoeffSrcV = m_ppcQTTempCoeffCr[uiQTLayer] + (uiNumCoeffIncC * uiAbsPartIdx);
            TCoeff* pcCoeffDstV = m_pcQTTempTUCoeffCr;
            ::memcpy(pcCoeffDstV, pcCoeffSrcV, sizeof(TCoeff) * uiNumCoeffC);
            Int* pcArlCoeffSrcV = m_ppcQTTempArlCoeffCr[uiQTLayer] + (uiNumCoeffIncC * uiAbsPartIdx);
            Int* pcArlCoeffDstV = m_ppcQTTempTUArlCoeffCr;
            ::memcpy(pcArlCoeffDstV, pcArlCoeffSrcV, sizeof(Int) * uiNumCoeffC);
        }

        //===== copy reconstruction =====
        UInt uiLog2TrSizeChroma = (bChromaSame ? uiLog2TrSize : uiLog2TrSize - 1);
        m_pcQTTempTComYuv[uiQTLayer].copyPartToPartChroma(&m_pcQTTempTransformSkipTComYuv, uiAbsPartIdx, 1 << uiLog2TrSizeChroma, 1 << uiLog2TrSizeChroma, stateU0V1Both2);
    }
}

Void TEncSearch::xLoadIntraResultChromaQT(TComDataCU* cu,
                                          UInt        trDepth,
                                          UInt        uiAbsPartIdx,
                                          UInt        stateU0V1Both2)
{
    UInt uiFullDepth = cu->getDepth(0) + trDepth;
    UInt uiTrMode    = cu->getTransformIdx(uiAbsPartIdx);

    if (uiTrMode == trDepth)
    {
        UInt uiLog2TrSize = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth] + 2;
        UInt uiQTLayer    = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;

        Bool bChromaSame = false;
        if (uiLog2TrSize == 2)
        {
            assert(trDepth > 0);
            trDepth--;
            UInt uiQPDiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trDepth) << 1);
            if ((uiAbsPartIdx % uiQPDiv) != 0)
            {
                return;
            }
            bChromaSame = true;
        }

        //===== copy transform coefficients =====
        UInt uiNumCoeffC = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> (uiFullDepth << 1);
        if (!bChromaSame)
        {
            uiNumCoeffC >>= 2;
        }
        UInt uiNumCoeffIncC = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> ((cu->getSlice()->getSPS()->getMaxCUDepth() << 1) + 2);

        if (stateU0V1Both2 == 0 || stateU0V1Both2 == 2)
        {
            TCoeff* pcCoeffDstU = m_ppcQTTempCoeffCb[uiQTLayer] + (uiNumCoeffIncC * uiAbsPartIdx);
            TCoeff* pcCoeffSrcU = m_pcQTTempTUCoeffCb;
            ::memcpy(pcCoeffDstU, pcCoeffSrcU, sizeof(TCoeff) * uiNumCoeffC);
            Int* pcArlCoeffDstU = m_ppcQTTempArlCoeffCb[uiQTLayer] + (uiNumCoeffIncC * uiAbsPartIdx);
            Int* pcArlCoeffSrcU = m_ppcQTTempTUArlCoeffCb;
            ::memcpy(pcArlCoeffDstU, pcArlCoeffSrcU, sizeof(Int) * uiNumCoeffC);
        }
        if (stateU0V1Both2 == 1 || stateU0V1Both2 == 2)
        {
            TCoeff* pcCoeffDstV = m_ppcQTTempCoeffCr[uiQTLayer] + (uiNumCoeffIncC * uiAbsPartIdx);
            TCoeff* pcCoeffSrcV = m_pcQTTempTUCoeffCr;
            ::memcpy(pcCoeffDstV, pcCoeffSrcV, sizeof(TCoeff) * uiNumCoeffC);
            Int* pcArlCoeffDstV = m_ppcQTTempArlCoeffCr[uiQTLayer] + (uiNumCoeffIncC * uiAbsPartIdx);
            Int* pcArlCoeffSrcV = m_ppcQTTempTUArlCoeffCr;
            ::memcpy(pcArlCoeffDstV, pcArlCoeffSrcV, sizeof(Int) * uiNumCoeffC);
        }

        //===== copy reconstruction =====
        UInt uiLog2TrSizeChroma = (bChromaSame ? uiLog2TrSize : uiLog2TrSize - 1);
        m_pcQTTempTransformSkipTComYuv.copyPartToPartChroma(&m_pcQTTempTComYuv[uiQTLayer], uiAbsPartIdx, 1 << uiLog2TrSizeChroma, 1 << uiLog2TrSizeChroma, stateU0V1Both2);

        UInt    uiZOrder          = cu->getZorderIdxInCU() + uiAbsPartIdx;
        UInt    uiWidth           = cu->getWidth(0) >> (trDepth + 1);
        UInt    uiHeight          = cu->getHeight(0) >> (trDepth + 1);
        UInt    uiRecQtStride     = m_pcQTTempTComYuv[uiQTLayer].Cwidth;
        UInt    uiRecIPredStride  = cu->getPic()->getPicYuvRec()->getCStride();

        if (stateU0V1Both2 == 0 || stateU0V1Both2 == 2)
        {
            Pel* piRecIPred = cu->getPic()->getPicYuvRec()->getCbAddr(cu->getAddr(), uiZOrder);
            Short* piRecQt    = m_pcQTTempTComYuv[uiQTLayer].getCbAddr(uiAbsPartIdx);
            Short* pRecQt     = piRecQt;
            Pel* pRecIPred  = piRecIPred;
            for (UInt uiY = 0; uiY < uiHeight; uiY++)
            {
                for (UInt uiX = 0; uiX < uiWidth; uiX++)
                {
                    pRecIPred[uiX] = (Pel)pRecQt[uiX];
                }

                pRecQt    += uiRecQtStride;
                pRecIPred += uiRecIPredStride;
            }
        }
        if (stateU0V1Both2 == 1 || stateU0V1Both2 == 2)
        {
            Pel* piRecIPred = cu->getPic()->getPicYuvRec()->getCrAddr(cu->getAddr(), uiZOrder);
            Short* piRecQt    = m_pcQTTempTComYuv[uiQTLayer].getCrAddr(uiAbsPartIdx);
            Short* pRecQt     = piRecQt;
            Pel* pRecIPred  = piRecIPred;
            for (UInt uiY = 0; uiY < uiHeight; uiY++)
            {
                for (UInt uiX = 0; uiX < uiWidth; uiX++)
                {
                    pRecIPred[uiX] = (Pel)pRecQt[uiX];
                }

                pRecQt    += uiRecQtStride;
                pRecIPred += uiRecIPredStride;
            }
        }
    }
}

Void TEncSearch::xRecurIntraChromaCodingQT(TComDataCU* cu,
                                           UInt        trDepth,
                                           UInt        uiAbsPartIdx,
                                           TComYuv*    fencYuv,
                                           TComYuv*    pcPredYuv,
                                           TShortYUV*  pcResiYuv,
                                           UInt&       ruiDist)
{
    UInt uiFullDepth = cu->getDepth(0) +  trDepth;
    UInt uiTrMode    = cu->getTransformIdx(uiAbsPartIdx);

    if (uiTrMode == trDepth)
    {
        Bool checkTransformSkip = cu->getSlice()->getPPS()->getUseTransformSkip();
        UInt uiLog2TrSize = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth] + 2;

        UInt actualTrDepth = trDepth;
        if (uiLog2TrSize == 2)
        {
            assert(trDepth > 0);
            actualTrDepth--;
            UInt uiQPDiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + actualTrDepth) << 1);
            Bool bFirstQ = ((uiAbsPartIdx % uiQPDiv) == 0);
            if (!bFirstQ)
            {
                return;
            }
        }

        checkTransformSkip &= (uiLog2TrSize <= 3);
        if (m_pcEncCfg->getUseTransformSkipFast())
        {
            checkTransformSkip &= (uiLog2TrSize < 3);
            if (checkTransformSkip)
            {
                Int nbLumaSkip = 0;
                for (UInt absPartIdxSub = uiAbsPartIdx; absPartIdxSub < uiAbsPartIdx + 4; absPartIdxSub++)
                {
                    nbLumaSkip += cu->getTransformSkip(absPartIdxSub, TEXT_LUMA);
                }

                checkTransformSkip &= (nbLumaSkip > 0);
            }
        }

        if (checkTransformSkip)
        {
            // use RDO to decide whether Cr/Cb takes TS
            m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiFullDepth][CI_QT_TRAFO_ROOT]);

            for (Int chromaId = 0; chromaId < 2; chromaId++)
            {
                UInt64  dSingleCost    = MAX_INT64;
                Int     bestModeId     = 0;
                UInt    singleDistC    = 0;
                UInt    singleCbfC     = 0;
                UInt    singleDistCTmp = 0;
                UInt64  singleCostTmp  = 0;
                UInt    singleCbfCTmp  = 0;

                Int     default0Save1Load2 = 0;
                Int     firstCheckId       = 0;

                for (Int chromaModeId = firstCheckId; chromaModeId < 2; chromaModeId++)
                {
                    cu->setTransformSkipSubParts(chromaModeId, (TextType)(chromaId + 2), uiAbsPartIdx, cu->getDepth(0) +  actualTrDepth);
                    if (chromaModeId == firstCheckId)
                    {
                        default0Save1Load2 = 1;
                    }
                    else
                    {
                        default0Save1Load2 = 2;
                    }
                    singleDistCTmp = 0;
                    xIntraCodingChromaBlk(cu, trDepth, uiAbsPartIdx, fencYuv, pcPredYuv, pcResiYuv, singleDistCTmp, chromaId, default0Save1Load2);
                    singleCbfCTmp = cu->getCbf(uiAbsPartIdx, (TextType)(chromaId + 2), trDepth);

                    if (chromaModeId == 1 && singleCbfCTmp == 0)
                    {
                        //In order not to code TS flag when cbf is zero, the case for TS with cbf being zero is forbidden.
                        singleCostTmp = MAX_INT64;
                    }
                    else
                    {
                        UInt bitsTmp = xGetIntraBitsQTChroma(cu, trDepth, uiAbsPartIdx, chromaId + 2);
                        singleCostTmp = m_pcRdCost->calcRdCost(singleDistCTmp, bitsTmp);
                    }

                    if (singleCostTmp < dSingleCost)
                    {
                        dSingleCost = singleCostTmp;
                        singleDistC = singleDistCTmp;
                        bestModeId  = chromaModeId;
                        singleCbfC  = singleCbfCTmp;

                        if (bestModeId == firstCheckId)
                        {
                            xStoreIntraResultChromaQT(cu, trDepth, uiAbsPartIdx, chromaId);
                            m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiFullDepth][CI_TEMP_BEST]);
                        }
                    }
                    if (chromaModeId == firstCheckId)
                    {
                        m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiFullDepth][CI_QT_TRAFO_ROOT]);
                    }
                }

                if (bestModeId == firstCheckId)
                {
                    xLoadIntraResultChromaQT(cu, trDepth, uiAbsPartIdx, chromaId);
                    cu->setCbfSubParts(singleCbfC << trDepth, (TextType)(chromaId + 2), uiAbsPartIdx, cu->getDepth(0) + actualTrDepth);
                    m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiFullDepth][CI_TEMP_BEST]);
                }
                cu->setTransformSkipSubParts(bestModeId, (TextType)(chromaId + 2), uiAbsPartIdx, cu->getDepth(0) +  actualTrDepth);
                ruiDist += singleDistC;

                if (chromaId == 0)
                {
                    m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiFullDepth][CI_QT_TRAFO_ROOT]);
                }
            }
        }
        else
        {
            cu->setTransformSkipSubParts(0, TEXT_CHROMA_U, uiAbsPartIdx, cu->getDepth(0) +  actualTrDepth);
            cu->setTransformSkipSubParts(0, TEXT_CHROMA_V, uiAbsPartIdx, cu->getDepth(0) +  actualTrDepth);
            xIntraCodingChromaBlk(cu, trDepth, uiAbsPartIdx, fencYuv, pcPredYuv, pcResiYuv, ruiDist, 0);
            xIntraCodingChromaBlk(cu, trDepth, uiAbsPartIdx, fencYuv, pcPredYuv, pcResiYuv, ruiDist, 1);
        }
    }
    else
    {
        UInt uiSplitCbfU     = 0;
        UInt uiSplitCbfV     = 0;
        UInt uiQPartsDiv     = cu->getPic()->getNumPartInCU() >> ((uiFullDepth + 1) << 1);
        UInt uiAbsPartIdxSub = uiAbsPartIdx;
        for (UInt uiPart = 0; uiPart < 4; uiPart++, uiAbsPartIdxSub += uiQPartsDiv)
        {
            xRecurIntraChromaCodingQT(cu, trDepth + 1, uiAbsPartIdxSub, fencYuv, pcPredYuv, pcResiYuv, ruiDist);
            uiSplitCbfU |= cu->getCbf(uiAbsPartIdxSub, TEXT_CHROMA_U, trDepth + 1);
            uiSplitCbfV |= cu->getCbf(uiAbsPartIdxSub, TEXT_CHROMA_V, trDepth + 1);
        }

        for (UInt uiOffs = 0; uiOffs < 4 * uiQPartsDiv; uiOffs++)
        {
            cu->getCbf(TEXT_CHROMA_U)[uiAbsPartIdx + uiOffs] |= (uiSplitCbfU << trDepth);
            cu->getCbf(TEXT_CHROMA_V)[uiAbsPartIdx + uiOffs] |= (uiSplitCbfV << trDepth);
        }
    }
}

Void TEncSearch::xSetIntraResultChromaQT(TComDataCU* cu,
                                         UInt        trDepth,
                                         UInt        uiAbsPartIdx,
                                         TComYuv*    pcRecoYuv)
{
    UInt uiFullDepth  = cu->getDepth(0) + trDepth;
    UInt uiTrMode     = cu->getTransformIdx(uiAbsPartIdx);

    if (uiTrMode == trDepth)
    {
        UInt uiLog2TrSize = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth] + 2;
        UInt uiQTLayer    = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;

        Bool bChromaSame  = false;
        if (uiLog2TrSize == 2)
        {
            assert(trDepth > 0);
            UInt uiQPDiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trDepth - 1) << 1);
            if ((uiAbsPartIdx % uiQPDiv) != 0)
            {
                return;
            }
            bChromaSame     = true;
        }

        //===== copy transform coefficients =====
        UInt uiNumCoeffC    = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> (uiFullDepth << 1);
        if (!bChromaSame)
        {
            uiNumCoeffC     >>= 2;
        }
        UInt uiNumCoeffIncC = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> ((cu->getSlice()->getSPS()->getMaxCUDepth() << 1) + 2);
        TCoeff* pcCoeffSrcU = m_ppcQTTempCoeffCb[uiQTLayer] + (uiNumCoeffIncC * uiAbsPartIdx);
        TCoeff* pcCoeffSrcV = m_ppcQTTempCoeffCr[uiQTLayer] + (uiNumCoeffIncC * uiAbsPartIdx);
        TCoeff* pcCoeffDstU = cu->getCoeffCb()              + (uiNumCoeffIncC * uiAbsPartIdx);
        TCoeff* pcCoeffDstV = cu->getCoeffCr()              + (uiNumCoeffIncC * uiAbsPartIdx);
        ::memcpy(pcCoeffDstU, pcCoeffSrcU, sizeof(TCoeff) * uiNumCoeffC);
        ::memcpy(pcCoeffDstV, pcCoeffSrcV, sizeof(TCoeff) * uiNumCoeffC);
        Int* pcArlCoeffSrcU = m_ppcQTTempArlCoeffCb[uiQTLayer] + (uiNumCoeffIncC * uiAbsPartIdx);
        Int* pcArlCoeffSrcV = m_ppcQTTempArlCoeffCr[uiQTLayer] + (uiNumCoeffIncC * uiAbsPartIdx);
        Int* pcArlCoeffDstU = cu->getArlCoeffCb()              + (uiNumCoeffIncC * uiAbsPartIdx);
        Int* pcArlCoeffDstV = cu->getArlCoeffCr()              + (uiNumCoeffIncC * uiAbsPartIdx);
        ::memcpy(pcArlCoeffDstU, pcArlCoeffSrcU, sizeof(Int) * uiNumCoeffC);
        ::memcpy(pcArlCoeffDstV, pcArlCoeffSrcV, sizeof(Int) * uiNumCoeffC);

        //===== copy reconstruction =====
        UInt uiLog2TrSizeChroma = (bChromaSame ? uiLog2TrSize : uiLog2TrSize - 1);
        m_pcQTTempTComYuv[uiQTLayer].copyPartToPartChroma(pcRecoYuv, uiAbsPartIdx, 1 << uiLog2TrSizeChroma, 1 << uiLog2TrSizeChroma);
    }
    else
    {
        UInt uiNumQPart  = cu->getPic()->getNumPartInCU() >> ((uiFullDepth + 1) << 1);
        for (UInt uiPart = 0; uiPart < 4; uiPart++)
        {
            xSetIntraResultChromaQT(cu, trDepth + 1, uiAbsPartIdx + uiPart * uiNumQPart, pcRecoYuv);
        }
    }
}

Void TEncSearch::preestChromaPredMode(TComDataCU* cu,
                                      TComYuv*    fencYuv,
                                      TComYuv*    pcPredYuv)
{
    UInt  uiWidth     = cu->getWidth(0) >> 1;
    UInt  uiHeight    = cu->getHeight(0) >> 1;
    UInt  uiStride    = fencYuv->getCStride();
    Pel*  piOrgU      = fencYuv->getCbAddr(0);
    Pel*  piOrgV      = fencYuv->getCrAddr(0);
    Pel*  piPredU     = pcPredYuv->getCbAddr(0);
    Pel*  piPredV     = pcPredYuv->getCrAddr(0);

    //===== init pattern =====
    assert(uiWidth == uiHeight);
    cu->getPattern()->initPattern(cu, 0, 0);
    cu->getPattern()->initAdiPatternChroma(cu, 0, 0, m_piPredBuf, m_iPredBufStride, m_iPredBufHeight);
    Pel*  pPatChromaU = cu->getPattern()->getAdiCbBuf(uiWidth, uiHeight, m_piPredBuf);
    Pel*  pPatChromaV = cu->getPattern()->getAdiCrBuf(uiWidth, uiHeight, m_piPredBuf);

    //===== get best prediction modes (using SAD) =====
    UInt  uiMinMode   = 0;
    UInt  uiMaxMode   = 4;
    UInt  uiBestMode  = MAX_UINT;
    UInt  uiMinSAD    = MAX_UINT;
    x265::pixelcmp_t sa8d = x265::primitives.sa8d[(int)g_convertToBit[uiWidth]];
    for (UInt uiMode  = uiMinMode; uiMode < uiMaxMode; uiMode++)
    {
        //--- get prediction ---
        predIntraChromaAng(pPatChromaU, uiMode, piPredU, uiStride, uiWidth);
        predIntraChromaAng(pPatChromaV, uiMode, piPredV, uiStride, uiWidth);

        //--- get SAD ---
        UInt uiSAD = sa8d((pixel*)piOrgU, uiStride, (pixel*)piPredU, uiStride) +
            sa8d((pixel*)piOrgV, uiStride, (pixel*)piPredV, uiStride);

        //--- check ---
        if (uiSAD < uiMinSAD)
        {
            uiMinSAD   = uiSAD;
            uiBestMode = uiMode;
        }
    }

    x265_emms();

    //===== set chroma pred mode =====
    cu->setChromIntraDirSubParts(uiBestMode, 0, cu->getDepth(0));
}

Void TEncSearch::estIntraPredQT(TComDataCU* cu,
                                TComYuv*    fencYuv,
                                TComYuv*    pcPredYuv,
                                TShortYUV*  pcResiYuv,
                                TComYuv*    pcRecoYuv,
                                UInt&       ruiDistC,
                                Bool        bLumaOnly)
{
    UInt    uiDepth        = cu->getDepth(0);
    UInt    uiNumPU        = cu->getNumPartInter();
    UInt    uiInitTrDepth  = cu->getPartitionSize(0) == SIZE_2Nx2N ? 0 : 1;
    UInt    uiWidth        = cu->getWidth(0) >> uiInitTrDepth;
    UInt    uiHeight       = cu->getHeight(0) >> uiInitTrDepth;
    UInt    uiQNumParts    = cu->getTotalNumPart() >> 2;
    UInt    uiWidthBit     = cu->getIntraSizeIdx(0);
    UInt    uiOverallDistY = 0;
    UInt    uiOverallDistC = 0;
    UInt    CandNum;
    UInt64  CandCostList[FAST_UDI_MAX_RDMODE_NUM];

    assert(uiWidth == uiHeight);

    //===== set QP and clear Cbf =====
    if (cu->getSlice()->getPPS()->getUseDQP() == true)
    {
        cu->setQPSubParts(cu->getQP(0), 0, uiDepth);
    }
    else
    {
        cu->setQPSubParts(cu->getSlice()->getSliceQp(), 0, uiDepth);
    }

    //===== loop over partitions =====
    UInt uiPartOffset = 0;
    for (UInt uiPU = 0; uiPU < uiNumPU; uiPU++, uiPartOffset += uiQNumParts)
    {
        //===== init pattern for luma prediction =====
        cu->getPattern()->initPattern(cu, uiInitTrDepth, uiPartOffset);
        // Reference sample smoothing
        cu->getPattern()->initAdiPattern(cu, uiPartOffset, uiInitTrDepth, m_piPredBuf, m_iPredBufStride, m_iPredBufHeight, refAbove, refLeft, refAboveFlt, refLeftFlt);

        //===== determine set of modes to be tested (using prediction signal only) =====
        Int numModesAvailable = 35; //total number of Intra modes
        Pel* piOrg         = fencYuv->getLumaAddr(uiPU, uiWidth);
        Pel* piPred        = pcPredYuv->getLumaAddr(uiPU, uiWidth);
        UInt uiStride      = pcPredYuv->getStride();
        UInt uiRdModeList[FAST_UDI_MAX_RDMODE_NUM];
        Int numModesForFullRD = g_intraModeNumFast[uiWidthBit];
        Int nLog2SizeMinus2 = g_convertToBit[uiWidth];
        x265::pixelcmp_t sa8d = x265::primitives.sa8d[nLog2SizeMinus2];

        Bool doFastSearch = (numModesForFullRD != numModesAvailable);
        if (doFastSearch)
        {
            assert(numModesForFullRD < numModesAvailable);

            for (Int i = 0; i < numModesForFullRD; i++)
            {
                CandCostList[i] = MAX_INT64;
            }

            CandNum = 0;
            UInt uiSads[35];
            Bool bFilter = (uiWidth <= 16);
            Pel *ptrSrc = m_piPredBuf;

            // 1
            primitives.intra_pred_dc((pixel*)ptrSrc + ADI_BUF_STRIDE + 1, ADI_BUF_STRIDE, (pixel*)piPred, uiStride, uiWidth, bFilter);
            uiSads[DC_IDX] = sa8d((pixel*)piOrg, uiStride, (pixel*)piPred, uiStride);

            // 0
            if (uiWidth >= 8 && uiWidth <= 32)
            {
                ptrSrc += ADI_BUF_STRIDE * (2 * uiWidth + 1);
            }
            primitives.intra_pred_planar((pixel*)ptrSrc + ADI_BUF_STRIDE + 1, ADI_BUF_STRIDE, (pixel*)piPred, uiStride, uiWidth);
            uiSads[PLANAR_IDX] = sa8d((pixel*)piOrg, uiStride, (pixel*)piPred, uiStride);

            // 33 Angle modes once
            if (uiWidth <= 32)
            {
                ALIGN_VAR_32(Pel, buf1[MAX_CU_SIZE * MAX_CU_SIZE]);
                ALIGN_VAR_32(Pel, tmp[33 * MAX_CU_SIZE * MAX_CU_SIZE]);

                // Transpose NxN
                x265::primitives.transpose[nLog2SizeMinus2]((pixel*)buf1, (pixel*)piOrg, uiStride);

                Pel *pAbove0 = refAbove    + uiWidth - 1;
                Pel *pAbove1 = refAboveFlt + uiWidth - 1;
                Pel *pLeft0  = refLeft     + uiWidth - 1;
                Pel *pLeft1  = refLeftFlt  + uiWidth - 1;

                x265::primitives.intra_pred_allangs[nLog2SizeMinus2]((pixel*)tmp, (pixel*)pAbove0, (pixel*)pLeft0, (pixel*)pAbove1, (pixel*)pLeft1, (uiWidth <= 16));

                // TODO: We need SATD_x4 here
                for (UInt uiMode = 2; uiMode < numModesAvailable; uiMode++)
                {
                    bool modeHor = (uiMode < 18);
                    Pel *pSrc = (modeHor ? buf1 : piOrg);
                    intptr_t srcStride = (modeHor ? uiWidth : uiStride);

                    // use hadamard transform here
                    UInt uiSad = sa8d((pixel*)pSrc, srcStride, (pixel*)&tmp[(uiMode - 2) * (uiWidth * uiWidth)], uiWidth);
                    uiSads[uiMode] = uiSad;
                }
            }
            else
            {
                for (UInt uiMode = 2; uiMode < numModesAvailable; uiMode++)
                {
                    predIntraLumaAng(cu->getPattern(), uiMode, piPred, uiStride, uiWidth);

                    // use hadamard transform here
                    UInt uiSad = sa8d((pixel*)piOrg, uiStride, (pixel*)piPred, uiStride);
                    uiSads[uiMode] = uiSad;
                }
            }

            for (UInt uiMode = 0; uiMode < numModesAvailable; uiMode++)
            {
                UInt uiSad = uiSads[uiMode];
                UInt iModeBits = xModeBitsIntra(cu, uiMode, uiPU, uiPartOffset, uiDepth, uiInitTrDepth);
                UInt64 cost = m_pcRdCost->calcRdSADCost(uiSad, iModeBits);
                CandNum += xUpdateCandList(uiMode, cost, numModesForFullRD, uiRdModeList, CandCostList);    //Find N least cost  modes. N = numModesForFullRD
            }

            Int uiPreds[3] = { -1, -1, -1 };
            Int iMode = -1;
            Int numCand = cu->getIntraDirLumaPredictor(uiPartOffset, uiPreds, &iMode);
            if (iMode >= 0)
            {
                numCand = iMode;
            }

            for (Int j = 0; j < numCand; j++)
            {
                Bool mostProbableModeIncluded = false;
                Int mostProbableMode = uiPreds[j];

                for (Int i = 0; i < numModesForFullRD; i++)
                {
                    mostProbableModeIncluded |= (mostProbableMode == uiRdModeList[i]);
                }

                if (!mostProbableModeIncluded)
                {
                    uiRdModeList[numModesForFullRD++] = mostProbableMode;
                }
            }
        }
        else
        {
            for (Int i = 0; i < numModesForFullRD; i++)
            {
                uiRdModeList[i] = i;
            }
        }
        x265_emms();

        //===== check modes (using r-d costs) =====
        UInt    uiBestPUMode  = 0;
        UInt    uiBestPUDistY = 0;
        UInt    uiBestPUDistC = 0;
        UInt64  dBestPUCost   = MAX_INT64;
        for (UInt uiMode = 0; uiMode < numModesForFullRD; uiMode++)
        {
            // set luma prediction mode
            UInt uiOrgMode = uiRdModeList[uiMode];

            cu->setLumaIntraDirSubParts(uiOrgMode, uiPartOffset, uiDepth + uiInitTrDepth);

            // set context models
            m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);

            // determine residual for partition
            UInt   uiPUDistY = 0;
            UInt   uiPUDistC = 0;
            UInt64 dPUCost   = 0.0;
            xRecurIntraCodingQT(cu, uiInitTrDepth, uiPartOffset, bLumaOnly, fencYuv, pcPredYuv, pcResiYuv, uiPUDistY, uiPUDistC, true, dPUCost);

            // check r-d cost
            if (dPUCost < dBestPUCost)
            {
                uiBestPUMode  = uiOrgMode;
                uiBestPUDistY = uiPUDistY;
                uiBestPUDistC = uiPUDistC;
                dBestPUCost   = dPUCost;

                xSetIntraResultQT(cu, uiInitTrDepth, uiPartOffset, bLumaOnly, pcRecoYuv);

                UInt uiQPartNum = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + uiInitTrDepth) << 1);
                ::memcpy(m_puhQTTempTrIdx,  cu->getTransformIdx() + uiPartOffset, uiQPartNum * sizeof(UChar));
                ::memcpy(m_puhQTTempCbf[0], cu->getCbf(TEXT_LUMA) + uiPartOffset, uiQPartNum * sizeof(UChar));
                ::memcpy(m_puhQTTempCbf[1], cu->getCbf(TEXT_CHROMA_U) + uiPartOffset, uiQPartNum * sizeof(UChar));
                ::memcpy(m_puhQTTempCbf[2], cu->getCbf(TEXT_CHROMA_V) + uiPartOffset, uiQPartNum * sizeof(UChar));
                ::memcpy(m_puhQTTempTransformSkipFlag[0], cu->getTransformSkip(TEXT_LUMA)     + uiPartOffset, uiQPartNum * sizeof(UChar));
                ::memcpy(m_puhQTTempTransformSkipFlag[1], cu->getTransformSkip(TEXT_CHROMA_U) + uiPartOffset, uiQPartNum * sizeof(UChar));
                ::memcpy(m_puhQTTempTransformSkipFlag[2], cu->getTransformSkip(TEXT_CHROMA_V) + uiPartOffset, uiQPartNum * sizeof(UChar));
            }
        } // Mode loop

        {
            UInt uiOrgMode = uiBestPUMode;

            cu->setLumaIntraDirSubParts(uiOrgMode, uiPartOffset, uiDepth + uiInitTrDepth);

            // set context models
            m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);

            // determine residual for partition
            UInt   uiPUDistY = 0;
            UInt   uiPUDistC = 0;
            UInt64 dPUCost   = 0;
            xRecurIntraCodingQT(cu, uiInitTrDepth, uiPartOffset, bLumaOnly, fencYuv, pcPredYuv, pcResiYuv, uiPUDistY, uiPUDistC, false, dPUCost);

            // check r-d cost
            if (dPUCost < dBestPUCost)
            {
                uiBestPUMode  = uiOrgMode;
                uiBestPUDistY = uiPUDistY;
                uiBestPUDistC = uiPUDistC;
                dBestPUCost   = dPUCost;

                xSetIntraResultQT(cu, uiInitTrDepth, uiPartOffset, bLumaOnly, pcRecoYuv);

                UInt uiQPartNum = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + uiInitTrDepth) << 1);
                ::memcpy(m_puhQTTempTrIdx,  cu->getTransformIdx()       + uiPartOffset, uiQPartNum * sizeof(UChar));
                ::memcpy(m_puhQTTempCbf[0], cu->getCbf(TEXT_LUMA) + uiPartOffset, uiQPartNum * sizeof(UChar));
                ::memcpy(m_puhQTTempCbf[1], cu->getCbf(TEXT_CHROMA_U) + uiPartOffset, uiQPartNum * sizeof(UChar));
                ::memcpy(m_puhQTTempCbf[2], cu->getCbf(TEXT_CHROMA_V) + uiPartOffset, uiQPartNum * sizeof(UChar));
                ::memcpy(m_puhQTTempTransformSkipFlag[0], cu->getTransformSkip(TEXT_LUMA)     + uiPartOffset, uiQPartNum * sizeof(UChar));
                ::memcpy(m_puhQTTempTransformSkipFlag[1], cu->getTransformSkip(TEXT_CHROMA_U) + uiPartOffset, uiQPartNum * sizeof(UChar));
                ::memcpy(m_puhQTTempTransformSkipFlag[2], cu->getTransformSkip(TEXT_CHROMA_V) + uiPartOffset, uiQPartNum * sizeof(UChar));
            }
        } // Mode loop

        //--- update overall distortion ---
        uiOverallDistY += uiBestPUDistY;
        uiOverallDistC += uiBestPUDistC;

        //--- update transform index and cbf ---
        UInt uiQPartNum = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + uiInitTrDepth) << 1);
        ::memcpy(cu->getTransformIdx()       + uiPartOffset, m_puhQTTempTrIdx,  uiQPartNum * sizeof(UChar));
        ::memcpy(cu->getCbf(TEXT_LUMA) + uiPartOffset, m_puhQTTempCbf[0], uiQPartNum * sizeof(UChar));
        ::memcpy(cu->getCbf(TEXT_CHROMA_U) + uiPartOffset, m_puhQTTempCbf[1], uiQPartNum * sizeof(UChar));
        ::memcpy(cu->getCbf(TEXT_CHROMA_V) + uiPartOffset, m_puhQTTempCbf[2], uiQPartNum * sizeof(UChar));
        ::memcpy(cu->getTransformSkip(TEXT_LUMA)     + uiPartOffset, m_puhQTTempTransformSkipFlag[0], uiQPartNum * sizeof(UChar));
        ::memcpy(cu->getTransformSkip(TEXT_CHROMA_U) + uiPartOffset, m_puhQTTempTransformSkipFlag[1], uiQPartNum * sizeof(UChar));
        ::memcpy(cu->getTransformSkip(TEXT_CHROMA_V) + uiPartOffset, m_puhQTTempTransformSkipFlag[2], uiQPartNum * sizeof(UChar));
        //--- set reconstruction for next intra prediction blocks ---
        if (uiPU != uiNumPU - 1)
        {
            Bool bSkipChroma  = false;
            Bool bChromaSame  = false;
            UInt uiLog2TrSize = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> (cu->getDepth(0) + uiInitTrDepth)] + 2;
            if (!bLumaOnly && uiLog2TrSize == 2)
            {
                assert(uiInitTrDepth  > 0);
                bSkipChroma  = (uiPU != 0);
                bChromaSame  = true;
            }

            UInt    uiCompWidth   = cu->getWidth(0) >> uiInitTrDepth;
            UInt    uiCompHeight  = cu->getHeight(0) >> uiInitTrDepth;
            UInt    uiZOrder      = cu->getZorderIdxInCU() + uiPartOffset;
            Pel*    piDes         = cu->getPic()->getPicYuvRec()->getLumaAddr(cu->getAddr(), uiZOrder);
            UInt    uiDesStride   = cu->getPic()->getPicYuvRec()->getStride();
            Pel*    piSrc         = pcRecoYuv->getLumaAddr(uiPartOffset);
            UInt    uiSrcStride   = pcRecoYuv->getStride();
            for (UInt uiY = 0; uiY < uiCompHeight; uiY++, piSrc += uiSrcStride, piDes += uiDesStride)
            {
                for (UInt uiX = 0; uiX < uiCompWidth; uiX++)
                {
                    piDes[uiX] = piSrc[uiX];
                }
            }

            if (!bLumaOnly && !bSkipChroma)
            {
                if (!bChromaSame)
                {
                    uiCompWidth   >>= 1;
                    uiCompHeight  >>= 1;
                }
                piDes         = cu->getPic()->getPicYuvRec()->getCbAddr(cu->getAddr(), uiZOrder);
                uiDesStride   = cu->getPic()->getPicYuvRec()->getCStride();
                piSrc         = pcRecoYuv->getCbAddr(uiPartOffset);
                uiSrcStride   = pcRecoYuv->getCStride();
                for (UInt uiY = 0; uiY < uiCompHeight; uiY++, piSrc += uiSrcStride, piDes += uiDesStride)
                {
                    for (UInt uiX = 0; uiX < uiCompWidth; uiX++)
                    {
                        piDes[uiX] = piSrc[uiX];
                    }
                }

                piDes         = cu->getPic()->getPicYuvRec()->getCrAddr(cu->getAddr(), uiZOrder);
                piSrc         = pcRecoYuv->getCrAddr(uiPartOffset);
                for (UInt uiY = 0; uiY < uiCompHeight; uiY++, piSrc += uiSrcStride, piDes += uiDesStride)
                {
                    for (UInt uiX = 0; uiX < uiCompWidth; uiX++)
                    {
                        piDes[uiX] = piSrc[uiX];
                    }
                }
            }
        }

        //=== update PU data ====
        cu->setLumaIntraDirSubParts(uiBestPUMode, uiPartOffset, uiDepth + uiInitTrDepth);
        cu->copyToPic(uiDepth, uiPU, uiInitTrDepth);
    } // PU loop

    if (uiNumPU > 1)
    { // set Cbf for all blocks
        UInt uiCombCbfY = 0;
        UInt uiCombCbfU = 0;
        UInt uiCombCbfV = 0;
        UInt uiPartIdx  = 0;
        for (UInt uiPart = 0; uiPart < 4; uiPart++, uiPartIdx += uiQNumParts)
        {
            uiCombCbfY |= cu->getCbf(uiPartIdx, TEXT_LUMA,     1);
            uiCombCbfU |= cu->getCbf(uiPartIdx, TEXT_CHROMA_U, 1);
            uiCombCbfV |= cu->getCbf(uiPartIdx, TEXT_CHROMA_V, 1);
        }

        for (UInt uiOffs = 0; uiOffs < 4 * uiQNumParts; uiOffs++)
        {
            cu->getCbf(TEXT_LUMA)[uiOffs] |= uiCombCbfY;
            cu->getCbf(TEXT_CHROMA_U)[uiOffs] |= uiCombCbfU;
            cu->getCbf(TEXT_CHROMA_V)[uiOffs] |= uiCombCbfV;
        }
    }

    //===== reset context models =====
    m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);

    //===== set distortion (rate and r-d costs are determined later) =====
    ruiDistC                   = uiOverallDistC;
    cu->getTotalDistortion() = uiOverallDistY + uiOverallDistC;
}

Void TEncSearch::estIntraPredChromaQT(TComDataCU* cu,
                                      TComYuv*    fencYuv,
                                      TComYuv*    pcPredYuv,
                                      TShortYUV*  pcResiYuv,
                                      TComYuv*    pcRecoYuv,
                                      UInt        uiPreCalcDistC)
{
    UInt    uiDepth     = cu->getDepth(0);
    UInt    uiBestMode  = 0;
    UInt    uiBestDist  = 0;
    UInt64  uiBestCost  = MAX_INT64;

    //----- init mode list -----
    UInt  uiMinMode = 0;
    UInt  uiModeList[NUM_CHROMA_MODE];

    cu->getAllowedChromaDir(0, uiModeList);
    UInt  uiMaxMode = NUM_CHROMA_MODE;

    //----- check chroma modes -----
    for (UInt uiMode = uiMinMode; uiMode < uiMaxMode; uiMode++)
    {
        //----- restore context models -----
        m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);

        //----- chroma coding -----
        UInt    uiDist = 0;
        cu->setChromIntraDirSubParts(uiModeList[uiMode], 0, uiDepth);
        xRecurIntraChromaCodingQT(cu,   0, 0, fencYuv, pcPredYuv, pcResiYuv, uiDist);
        if (cu->getSlice()->getPPS()->getUseTransformSkip())
        {
            m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);
        }

        UInt    uiBits = xGetIntraBitsQT(cu,   0, 0, false, true);
        UInt64  uiCost  = m_pcRdCost->calcRdCost(uiDist, uiBits);

        //----- compare -----
        if (uiCost < uiBestCost)
        {
            uiBestCost  = uiCost;
            uiBestDist  = uiDist;
            uiBestMode  = uiModeList[uiMode];
            UInt  uiQPN = cu->getPic()->getNumPartInCU() >> (uiDepth << 1);
            xSetIntraResultChromaQT(cu, 0, 0, pcRecoYuv);
            ::memcpy(m_puhQTTempCbf[1], cu->getCbf(TEXT_CHROMA_U), uiQPN * sizeof(UChar));
            ::memcpy(m_puhQTTempCbf[2], cu->getCbf(TEXT_CHROMA_V), uiQPN * sizeof(UChar));
            ::memcpy(m_puhQTTempTransformSkipFlag[1], cu->getTransformSkip(TEXT_CHROMA_U), uiQPN * sizeof(UChar));
            ::memcpy(m_puhQTTempTransformSkipFlag[2], cu->getTransformSkip(TEXT_CHROMA_V), uiQPN * sizeof(UChar));
        }
    }

    //----- set data -----
    UInt  uiQPN = cu->getPic()->getNumPartInCU() >> (uiDepth << 1);
    ::memcpy(cu->getCbf(TEXT_CHROMA_U), m_puhQTTempCbf[1], uiQPN * sizeof(UChar));
    ::memcpy(cu->getCbf(TEXT_CHROMA_V), m_puhQTTempCbf[2], uiQPN * sizeof(UChar));
    ::memcpy(cu->getTransformSkip(TEXT_CHROMA_U), m_puhQTTempTransformSkipFlag[1], uiQPN * sizeof(UChar));
    ::memcpy(cu->getTransformSkip(TEXT_CHROMA_V), m_puhQTTempTransformSkipFlag[2], uiQPN * sizeof(UChar));
    cu->setChromIntraDirSubParts(uiBestMode, 0, uiDepth);
    cu->getTotalDistortion() += uiBestDist - uiPreCalcDistC;

    //----- restore context models -----
    m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);
}

/** Function for encoding and reconstructing luma/chroma samples of a PCM mode CU.
 * \param cu pointer to current CU
 * \param uiAbsPartIdx part index
 * \param piOrg pointer to original sample arrays
 * \param piPCM pointer to PCM code arrays
 * \param piPred pointer to prediction signal arrays
 * \param piResi pointer to residual signal arrays
 * \param piReco pointer to reconstructed sample arrays
 * \param uiStride stride of the original/prediction/residual sample arrays
 * \param uiWidth block width
 * \param uiHeight block height
 * \param ttText texture component type
 * \returns Void
 */
Void TEncSearch::xEncPCM(TComDataCU* cu, UInt uiAbsPartIdx, Pel* piOrg, Pel* piPCM, Pel* piPred, Short* piResi, Pel* piReco, UInt uiStride, UInt uiWidth, UInt uiHeight, TextType eText)
{
    UInt uiX, uiY;
    UInt uiReconStride;
    Pel* pOrg  = piOrg;
    Pel* pPCM  = piPCM;
    Pel* pPred = piPred;
    Short* pResi = piResi;
    Pel* pReco = piReco;
    Pel* pRecoPic;
    Int shiftPcm;

    if (eText == TEXT_LUMA)
    {
        uiReconStride = cu->getPic()->getPicYuvRec()->getStride();
        pRecoPic      = cu->getPic()->getPicYuvRec()->getLumaAddr(cu->getAddr(), cu->getZorderIdxInCU() + uiAbsPartIdx);
        shiftPcm = g_bitDepthY - cu->getSlice()->getSPS()->getPCMBitDepthLuma();
    }
    else
    {
        uiReconStride = cu->getPic()->getPicYuvRec()->getCStride();

        if (eText == TEXT_CHROMA_U)
        {
            pRecoPic = cu->getPic()->getPicYuvRec()->getCbAddr(cu->getAddr(), cu->getZorderIdxInCU() + uiAbsPartIdx);
        }
        else
        {
            pRecoPic = cu->getPic()->getPicYuvRec()->getCrAddr(cu->getAddr(), cu->getZorderIdxInCU() + uiAbsPartIdx);
        }
        shiftPcm = g_bitDepthC - cu->getSlice()->getSPS()->getPCMBitDepthChroma();
    }

    // Reset pred and residual
    for (uiY = 0; uiY < uiHeight; uiY++)
    {
        for (uiX = 0; uiX < uiWidth; uiX++)
        {
            pPred[uiX] = 0;
            pResi[uiX] = 0;
        }

        pPred += uiStride;
        pResi += uiStride;
    }

    // Encode
    for (uiY = 0; uiY < uiHeight; uiY++)
    {
        for (uiX = 0; uiX < uiWidth; uiX++)
        {
            pPCM[uiX] = pOrg[uiX] >> shiftPcm;
        }

        pPCM += uiWidth;
        pOrg += uiStride;
    }

    pPCM  = piPCM;

    // Reconstruction
    for (uiY = 0; uiY < uiHeight; uiY++)
    {
        for (uiX = 0; uiX < uiWidth; uiX++)
        {
            pReco[uiX] = pPCM[uiX] << shiftPcm;
            pRecoPic[uiX] = pReco[uiX];
        }

        pPCM += uiWidth;
        pReco += uiStride;
        pRecoPic += uiReconStride;
    }
}

/**  Function for PCM mode estimation.
 * \param cu
 * \param fencYuv
 * \param rpcPredYuv
 * \param rpcResiYuv
 * \param rpcRecoYuv
 * \returns Void
 */
Void TEncSearch::IPCMSearch(TComDataCU* cu, TComYuv* fencYuv, TComYuv*& rpcPredYuv, TShortYUV*& rpcResiYuv, TComYuv*& rpcRecoYuv)
{
    UInt   uiDepth        = cu->getDepth(0);
    UInt   uiWidth        = cu->getWidth(0);
    UInt   uiHeight       = cu->getHeight(0);
    UInt   uiStride       = rpcPredYuv->getStride();
    UInt   uiStrideC      = rpcPredYuv->getCStride();
    UInt   uiWidthC       = uiWidth  >> 1;
    UInt   uiHeightC      = uiHeight >> 1;
    UInt   uiDistortion = 0;
    UInt   uiBits;

    UInt64 uiCost;

    Pel*    pOrig;
    Short*  pResi;
    Pel*    pReco;
    Pel*    pPred;
    Pel*    pPCM;

    UInt uiAbsPartIdx = 0;

    UInt uiMinCoeffSize = cu->getPic()->getMinCUWidth() * cu->getPic()->getMinCUHeight();
    UInt uiLumaOffset   = uiMinCoeffSize * uiAbsPartIdx;
    UInt uiChromaOffset = uiLumaOffset >> 2;

    // Luminance
    pOrig    = fencYuv->getLumaAddr(0, uiWidth);
    pResi    = rpcResiYuv->getLumaAddr(0, uiWidth);
    pPred    = rpcPredYuv->getLumaAddr(0, uiWidth);
    pReco    = rpcRecoYuv->getLumaAddr(0, uiWidth);
    pPCM     = cu->getPCMSampleY() + uiLumaOffset;

    xEncPCM(cu, 0, pOrig, pPCM, pPred, pResi, pReco, uiStride, uiWidth, uiHeight, TEXT_LUMA);

    // Chroma U
    pOrig    = fencYuv->getCbAddr();
    pResi    = rpcResiYuv->getCbAddr();
    pPred    = rpcPredYuv->getCbAddr();
    pReco    = rpcRecoYuv->getCbAddr();
    pPCM     = cu->getPCMSampleCb() + uiChromaOffset;

    xEncPCM(cu, 0, pOrig, pPCM, pPred, pResi, pReco, uiStrideC, uiWidthC, uiHeightC, TEXT_CHROMA_U);

    // Chroma V
    pOrig    = fencYuv->getCrAddr();
    pResi    = rpcResiYuv->getCrAddr();
    pPred    = rpcPredYuv->getCrAddr();
    pReco    = rpcRecoYuv->getCrAddr();
    pPCM     = cu->getPCMSampleCr() + uiChromaOffset;

    xEncPCM(cu, 0, pOrig, pPCM, pPred, pResi, pReco, uiStrideC, uiWidthC, uiHeightC, TEXT_CHROMA_V);

    m_pcEntropyCoder->resetBits();
    xEncIntraHeader(cu, uiDepth, uiAbsPartIdx, true, false);
    uiBits = m_pcEntropyCoder->getNumberOfWrittenBits();

    uiCost = m_pcRdCost->calcRdCost(uiDistortion, uiBits);

    m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);

    cu->getTotalBits()       = uiBits;
    cu->getTotalCost()       = uiCost;
    cu->getTotalDistortion() = uiDistortion;

    cu->copyToPic(uiDepth, 0, 0);
}

UInt TEncSearch::xGetInterPredictionError(TComDataCU* cu, TComYuv* fencYuv, Int partIdx)
{
    UInt absPartIdx;
    Int width, height;

    motionCompensation(cu, &m_tmpYuvPred, REF_PIC_LIST_X, partIdx);
    cu->getPartIndexAndSize(partIdx, absPartIdx, width, height);
    UInt cost = m_me.bufSA8D((pixel*)m_tmpYuvPred.getLumaAddr(absPartIdx), m_tmpYuvPred.getStride());
    x265_emms();
    return cost;
}

/** estimation of best merge coding
 * \param cu
 * \param fencYuv
 * \param iPUIdx
 * \param uiInterDir
 * \param pacMvField
 * \param uiMergeIndex
 * \param outCost
 * \param outBits
 * \param puhNeighCands
 * \param bValid
 * \returns Void
 */
Void TEncSearch::xMergeEstimation(TComDataCU* cu, TComYuv* fencYuv, Int iPUIdx, UInt& uiInterDir, TComMvField* pacMvField, UInt& uiMergeIndex, UInt& outCost, TComMvField* cMvFieldNeighbours, UChar* uhInterDirNeighbours, Int& numValidMergeCand)
{
    UInt uiAbsPartIdx = 0;
    Int iWidth = 0;
    Int iHeight = 0;

    cu->getPartIndexAndSize(iPUIdx, uiAbsPartIdx, iWidth, iHeight);
    UInt uiDepth = cu->getDepth(uiAbsPartIdx);
    PartSize partSize = cu->getPartitionSize(0);
    if (cu->getSlice()->getPPS()->getLog2ParallelMergeLevelMinus2() && partSize != SIZE_2Nx2N && cu->getWidth(0) <= 8)
    {
        cu->setPartSizeSubParts(SIZE_2Nx2N, 0, uiDepth);
        if (iPUIdx == 0)
        {
            cu->getInterMergeCandidates(0, 0, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand);
        }
        cu->setPartSizeSubParts(partSize, 0, uiDepth);
    }
    else
    {
        cu->getInterMergeCandidates(uiAbsPartIdx, iPUIdx, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand);
    }
    xRestrictBipredMergeCand(cu, iPUIdx, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand);

    outCost = MAX_UINT;
    for (UInt uiMergeCand = 0; uiMergeCand < numValidMergeCand; ++uiMergeCand)
    {
        UInt uiCostCand = MAX_UINT;
        UInt uiBitsCand = 0;

        PartSize ePartSize = cu->getPartitionSize(0);

        cu->getCUMvField(REF_PIC_LIST_0)->setAllMvField(cMvFieldNeighbours[0 + 2 * uiMergeCand], ePartSize, uiAbsPartIdx, 0, iPUIdx);
        cu->getCUMvField(REF_PIC_LIST_1)->setAllMvField(cMvFieldNeighbours[1 + 2 * uiMergeCand], ePartSize, uiAbsPartIdx, 0, iPUIdx);

        uiCostCand = xGetInterPredictionError(cu, fencYuv, iPUIdx);
        uiBitsCand = uiMergeCand + 1;
        if (uiMergeCand == m_pcEncCfg->getMaxNumMergeCand() - 1)
        {
            uiBitsCand--;
        }
        uiCostCand = uiCostCand + m_pcRdCost->getCost(uiBitsCand);
        if (uiCostCand < outCost)
        {
            outCost = uiCostCand;
            pacMvField[0] = cMvFieldNeighbours[0 + 2 * uiMergeCand];
            pacMvField[1] = cMvFieldNeighbours[1 + 2 * uiMergeCand];
            uiInterDir = uhInterDirNeighbours[uiMergeCand];
            uiMergeIndex = uiMergeCand;
        }
    }
}

/** convert bi-pred merge candidates to uni-pred
 * \param cu
 * \param puIdx
 * \param mvFieldNeighbours
 * \param interDirNeighbours
 * \param numValidMergeCand
 * \returns Void
 */
Void TEncSearch::xRestrictBipredMergeCand(TComDataCU* cu, UInt puIdx, TComMvField* mvFieldNeighbours, UChar* interDirNeighbours, Int numValidMergeCand)
{
    if (cu->isBipredRestriction(puIdx))
    {
        for (UInt mergeCand = 0; mergeCand < numValidMergeCand; ++mergeCand)
        {
            if (interDirNeighbours[mergeCand] == 3)
            {
                interDirNeighbours[mergeCand] = 1;
                mvFieldNeighbours[(mergeCand << 1) + 1].setMvField(MV(0, 0), -1);
            }
        }
    }
}

/** search of the best candidate for inter prediction
 * \param cu
 * \param fencYuv
 * \param rpcPredYuv
 * \param rpcResiYuv
 * \param rpcRecoYuv
 * \param bUseRes
 * \returns Void
 */
Void TEncSearch::predInterSearch(TComDataCU* cu, TComYuv* fencYuv, TComYuv*& rpcPredYuv, Bool bUseMRG)
{
    m_acYuvPred[0].clear();
    m_acYuvPred[1].clear();
    m_cYuvPredTemp.clear();
    rpcPredYuv->clear();

    MV        cMvSrchRngLT;
    MV        cMvSrchRngRB;

    MV        cMvZero;
    MV        TempMv; //kolya

    MV        mv[2];
    MV        mvBidir[2];
    MV        mvTemp[2][33];

    Int       iNumPart    = cu->getNumPartInter();
    Int       iNumPredDir = cu->getSlice()->isInterP() ? 1 : 2;

    MV        mvPred[2][33];

    MV        cMvPredBi[2][33];
    Int       aaiMvpIdxBi[2][33];

    Int       aaiMvpIdx[2][33];
    Int       aaiMvpNum[2][33];

    AMVPInfo  aacAMVPInfo[2][33];

    Int       refIfx[2] = { 0, 0 }; //If un-initialized, may cause SEGV in bi-directional prediction iterative stage.
    Int       refIdxBidir[2];

    UInt      partAddr;
    Int       iRoiWidth, iRoiHeight;

    UInt      mbBits[3] = { 1, 1, 0 };

    UInt      uiLastMode = 0;
    Int       iRefStart, iRefEnd;

    PartSize  partSize = cu->getPartitionSize(0);

    Int       bestBiPRefIdxL1 = 0;
    Int       bestBiPMvpL1 = 0;
    UInt      biPDistTemp = MAX_INT;

    /* TODO: this could be as high as TEncSlice::compressSlice() */
    TComPicYuv *fenc = cu->getSlice()->getPic()->getPicYuvOrg();
    m_me.setSourcePlane((pixel*)fenc->getLumaAddr(), fenc->getStride());

    TComMvField cMvFieldNeighbours[MRG_MAX_NUM_CANDS << 1]; // double length for mv of both lists
    UChar uhInterDirNeighbours[MRG_MAX_NUM_CANDS];
    Int numValidMergeCand = 0;
    if (!m_pcEncCfg->getUseRDO())
        cu->getTotalCost() = 0;

    for (Int partIdx = 0; partIdx < iNumPart; partIdx++)
    {
        UInt          uiCost[2] = { MAX_UINT, MAX_UINT };
        UInt          uiCostBi  = MAX_UINT;
        UInt          costTemp;

        UInt          bits[3];
        UInt          bitsTemp;
        UInt          bestBiPDist = MAX_INT;

        UInt          uiCostTempL0[MAX_NUM_REF];
        for (Int iNumRef = 0; iNumRef < MAX_NUM_REF; iNumRef++)
        {
            uiCostTempL0[iNumRef] = MAX_UINT;
        }

        UInt          uiBitsTempL0[MAX_NUM_REF];

        MV            mvValidList1;
        Int           refIdxValidList1 = 0;
        UInt          bitsValidList1 = MAX_UINT;
        UInt          costValidList1 = MAX_UINT;

        //PPAScopeEvent(TEncSearch_xMotionEstimation + cu->getDepth(partIdx));
        xGetBlkBits(partSize, cu->getSlice()->isInterP(), partIdx, uiLastMode, mbBits);

        cu->getPartIndexAndSize(partIdx, partAddr, iRoiWidth, iRoiHeight);

        Pel* PU = fenc->getLumaAddr(cu->getAddr(), cu->getZorderIdxInCU() + partAddr);
        m_me.setSourcePU(PU - fenc->getLumaAddr(), iRoiWidth, iRoiHeight);

        cu->getMvPredLeft(m_mvPredictors[0]);
        cu->getMvPredAbove(m_mvPredictors[1]);
        cu->getMvPredAboveRight(m_mvPredictors[2]);

        Bool bTestNormalMC = true;

        if (bUseMRG && cu->getWidth(0) > 8 && iNumPart == 2)
        {
            bTestNormalMC = false;
        }

        if (bTestNormalMC)
        {
            //  Uni-directional prediction
            for (Int refList = 0; refList < iNumPredDir; refList++)
            {
                RefPicList  picList = (refList ? REF_PIC_LIST_1 : REF_PIC_LIST_0);

                for (Int refIdxTmp = 0; refIdxTmp < cu->getSlice()->getNumRefIdx(picList); refIdxTmp++)
                {
                    bitsTemp = mbBits[refList];
                    if (cu->getSlice()->getNumRefIdx(picList) > 1)
                    {
                        bitsTemp += refIdxTmp + 1;
                        if (refIdxTmp == cu->getSlice()->getNumRefIdx(picList) - 1) bitsTemp--;
                    }
                    xEstimateMvPredAMVP(cu, fencYuv, partIdx, picList, refIdxTmp, mvPred[refList][refIdxTmp], false, &biPDistTemp);
                    aaiMvpIdx[refList][refIdxTmp] = cu->getMVPIdx(picList, partAddr);
                    aaiMvpNum[refList][refIdxTmp] = cu->getMVPNum(picList, partAddr);

                    if (cu->getSlice()->getMvdL1ZeroFlag() && refList == 1 && biPDistTemp < bestBiPDist)
                    {
                        bestBiPDist = biPDistTemp;
                        bestBiPMvpL1 = aaiMvpIdx[refList][refIdxTmp];
                        bestBiPRefIdxL1 = refIdxTmp;
                    }

                    bitsTemp += m_mvpIdxCost[aaiMvpIdx[refList][refIdxTmp]][AMVP_MAX_NUM_CANDS];

                    if (refList == 1) // list 1
                    {
                        if (cu->getSlice()->getList1IdxToList0Idx(refIdxTmp) >= 0)
                        {
                            mvTemp[1][refIdxTmp] = mvTemp[0][cu->getSlice()->getList1IdxToList0Idx(refIdxTmp)];
                            costTemp = uiCostTempL0[cu->getSlice()->getList1IdxToList0Idx(refIdxTmp)];

                            /* first subtract the bit-rate part of the cost of the other list */
                            costTemp -= m_pcRdCost->getCost(uiBitsTempL0[cu->getSlice()->getList1IdxToList0Idx(refIdxTmp)]);

                            /* correct the bit-rate part of the current ref */
                            m_me.setMVP(mvPred[refList][refIdxTmp]);
                            bitsTemp += m_me.bitcost(mvTemp[1][refIdxTmp]);

                            /* calculate the correct cost */
                            costTemp += m_pcRdCost->getCost(bitsTemp);
                        }
                        else
                        {
                            if (m_iSearchMethod < X265_ORIG_SEARCH)
                            {
                                CYCLE_COUNTER_START(ME);
                                int merange = m_adaptiveRange[picList][refIdxTmp];
                                MV& mvp = mvPred[refList][refIdxTmp];
                                MV& outmv = mvTemp[refList][refIdxTmp];
                                MV mvmin, mvmax;
                                xSetSearchRange(cu, mvp, merange, mvmin, mvmax);
                                TComPicYuv *refRecon = cu->getSlice()->getRefPic(picList, refIdxTmp)->getPicYuvRec();
                                int satdCost = m_me.motionEstimate(refRecon->getMotionReference(0),
                                                                   mvmin, mvmax, mvp, 3, m_mvPredictors, merange, outmv);

                                /* Get total cost of partition, but only include MV bit cost once */
                                bitsTemp += m_me.bitcost(outmv);
                                costTemp = (satdCost - m_me.mvcost(outmv)) + m_pcRdCost->getCost(bitsTemp);
                                CYCLE_COUNTER_STOP(ME);
                            }
                            else
                                xMotionEstimation(cu, fencYuv, partIdx, picList, &mvPred[refList][refIdxTmp], refIdxTmp, mvTemp[refList][refIdxTmp], bitsTemp, costTemp);
                        }
                    }
                    else
                    {
                        if (m_iSearchMethod < X265_ORIG_SEARCH)
                        {
                            CYCLE_COUNTER_START(ME);
                            int merange = m_adaptiveRange[picList][refIdxTmp];
                            MV& mvp = mvPred[refList][refIdxTmp];
                            MV& outmv = mvTemp[refList][refIdxTmp];
                            MV mvmin, mvmax;
                            xSetSearchRange(cu, mvp, merange, mvmin, mvmax);
                            TComPicYuv *refRecon = cu->getSlice()->getRefPic(picList, refIdxTmp)->getPicYuvRec();
                            int satdCost = m_me.motionEstimate(refRecon->getMotionReference(0),
                                                               mvmin, mvmax, mvp, 3, m_mvPredictors, merange, outmv);

                            /* Get total cost of partition, but only include MV bit cost once */
                            bitsTemp += m_me.bitcost(outmv);
                            costTemp = (satdCost - m_me.mvcost(outmv)) + m_pcRdCost->getCost(bitsTemp);
                            CYCLE_COUNTER_STOP(ME);
                        }
                        else
                            xMotionEstimation(cu, fencYuv, partIdx, picList, &mvPred[refList][refIdxTmp], refIdxTmp, mvTemp[refList][refIdxTmp], bitsTemp, costTemp);
                    }
                    xCopyAMVPInfo(cu->getCUMvField(picList)->getAMVPInfo(), &aacAMVPInfo[refList][refIdxTmp]); // must always be done ( also when AMVP_MODE = AM_NONE )
                    xCheckBestMVP(cu, picList, mvTemp[refList][refIdxTmp], mvPred[refList][refIdxTmp], aaiMvpIdx[refList][refIdxTmp], bitsTemp, costTemp);

                    if (refList == 0)
                    {
                        uiCostTempL0[refIdxTmp] = costTemp;
                        uiBitsTempL0[refIdxTmp] = bitsTemp;
                    }
                    if (costTemp < uiCost[refList])
                    {
                        uiCost[refList] = costTemp;
                        bits[refList] = bitsTemp; // storing for bi-prediction

                        // set motion
                        mv[refList]     = mvTemp[refList][refIdxTmp];
                        refIfx[refList] = refIdxTmp;
                    }

                    if (refList == 1 && costTemp < costValidList1 && cu->getSlice()->getList1IdxToList0Idx(refIdxTmp) < 0)
                    {
                        costValidList1 = costTemp;
                        bitsValidList1 = bitsTemp;

                        // set motion
                        mvValidList1     = mvTemp[refList][refIdxTmp];
                        refIdxValidList1 = refIdxTmp;
                    }
                }
            }

            //  Bi-directional prediction
            if ((cu->getSlice()->isInterB()) && (cu->isBipredRestriction(partIdx) == false))
            {
                mvBidir[0] = mv[0];
                mvBidir[1] = mv[1];
                refIdxBidir[0] = refIfx[0];
                refIdxBidir[1] = refIfx[1];

                ::memcpy(cMvPredBi, mvPred, sizeof(mvPred));
                ::memcpy(aaiMvpIdxBi, aaiMvpIdx, sizeof(aaiMvpIdx));

                UInt motBits[2];

                if (cu->getSlice()->getMvdL1ZeroFlag())
                {
                    xCopyAMVPInfo(&aacAMVPInfo[1][bestBiPRefIdxL1], cu->getCUMvField(REF_PIC_LIST_1)->getAMVPInfo());
                    cu->setMVPIdxSubParts(bestBiPMvpL1, REF_PIC_LIST_1, partAddr, partIdx, cu->getDepth(partAddr));
                    aaiMvpIdxBi[1][bestBiPRefIdxL1] = bestBiPMvpL1;
                    cMvPredBi[1][bestBiPRefIdxL1] = cu->getCUMvField(REF_PIC_LIST_1)->getAMVPInfo()->m_acMvCand[bestBiPMvpL1];

                    mvBidir[1] = cMvPredBi[1][bestBiPRefIdxL1];
                    refIdxBidir[1] = bestBiPRefIdxL1;
                    cu->getCUMvField(REF_PIC_LIST_1)->setAllMv(mvBidir[1], partSize, partAddr, 0, partIdx);
                    cu->getCUMvField(REF_PIC_LIST_1)->setAllRefIdx(refIdxBidir[1], partSize, partAddr, 0, partIdx);
                    TComYuv* predYuv = &m_acYuvPred[1];
                    motionCompensation(cu, predYuv, REF_PIC_LIST_1, partIdx);

                    motBits[0] = bits[0] - mbBits[0];
                    motBits[1] = mbBits[1];

                    if (cu->getSlice()->getNumRefIdx(REF_PIC_LIST_1) > 1)
                    {
                        motBits[1] += bestBiPRefIdxL1 + 1;
                        if (bestBiPRefIdxL1 == cu->getSlice()->getNumRefIdx(REF_PIC_LIST_1) - 1) motBits[1]--;
                    }

                    motBits[1] += m_mvpIdxCost[aaiMvpIdxBi[1][bestBiPRefIdxL1]][AMVP_MAX_NUM_CANDS];

                    bits[2] = mbBits[2] + motBits[0] + motBits[1];

                    mvTemp[1][bestBiPRefIdxL1] = mvBidir[1];
                }
                else
                {
                    motBits[0] = bits[0] - mbBits[0];
                    motBits[1] = bits[1] - mbBits[1];
                    bits[2] = mbBits[2] + motBits[0] + motBits[1];
                }

                Int refList = 0;
                if (uiCost[0] <= uiCost[1])
                {
                    refList = 1;
                }
                else
                {
                    refList = 0;
                }
                if (!cu->getSlice()->getMvdL1ZeroFlag())
                {
                    cu->getCUMvField(RefPicList(1 - refList))->setAllMv(mv[1 - refList], partSize, partAddr, 0, partIdx);
                    cu->getCUMvField(RefPicList(1 - refList))->setAllRefIdx(refIfx[1 - refList], partSize, partAddr, 0, partIdx);
                    TComYuv*  predYuv = &m_acYuvPred[1 - refList];
                    motionCompensation(cu, predYuv, RefPicList(1 - refList), partIdx);
                }
                RefPicList  picList = (refList ? REF_PIC_LIST_1 : REF_PIC_LIST_0);

                if (cu->getSlice()->getMvdL1ZeroFlag())
                {
                    refList = 0;
                    picList = REF_PIC_LIST_0;
                }

                Bool bChanged = false;

                iRefStart = 0;
                iRefEnd   = cu->getSlice()->getNumRefIdx(picList) - 1;

                for (Int refIdxTmp = iRefStart; refIdxTmp <= iRefEnd; refIdxTmp++)
                {
                    bitsTemp = mbBits[2] + motBits[1 - refList];
                    if (cu->getSlice()->getNumRefIdx(picList) > 1)
                    {
                        bitsTemp += refIdxTmp + 1;
                        if (refIdxTmp == cu->getSlice()->getNumRefIdx(picList) - 1) bitsTemp--;
                    }
                    bitsTemp += m_mvpIdxCost[aaiMvpIdxBi[refList][refIdxTmp]][AMVP_MAX_NUM_CANDS];
                    // call bidir ME
                    xMotionEstimation(cu, fencYuv, partIdx, picList, &cMvPredBi[refList][refIdxTmp], refIdxTmp, mvTemp[refList][refIdxTmp], bitsTemp, costTemp, true);
                    xCopyAMVPInfo(&aacAMVPInfo[refList][refIdxTmp], cu->getCUMvField(picList)->getAMVPInfo());
                    xCheckBestMVP(cu, picList, mvTemp[refList][refIdxTmp], cMvPredBi[refList][refIdxTmp], aaiMvpIdxBi[refList][refIdxTmp], bitsTemp, costTemp);

                    if (costTemp < uiCostBi)
                    {
                        bChanged = true;

                        mvBidir[refList]     = mvTemp[refList][refIdxTmp];
                        refIdxBidir[refList] = refIdxTmp;

                        uiCostBi         = costTemp;
                        motBits[refList] = bitsTemp - mbBits[2] - motBits[1 - refList];
                        bits[2]          = bitsTemp;
                    }
                } // for loop-refIdxTmp

                if (!bChanged)
                {
                    if (uiCostBi <= uiCost[0] && uiCostBi <= uiCost[1])
                    {
                        xCopyAMVPInfo(&aacAMVPInfo[0][refIdxBidir[0]], cu->getCUMvField(REF_PIC_LIST_0)->getAMVPInfo());
                        xCheckBestMVP(cu, REF_PIC_LIST_0, mvBidir[0], cMvPredBi[0][refIdxBidir[0]], aaiMvpIdxBi[0][refIdxBidir[0]], bits[2], uiCostBi);
                        if (!cu->getSlice()->getMvdL1ZeroFlag())
                        {
                            xCopyAMVPInfo(&aacAMVPInfo[1][refIdxBidir[1]], cu->getCUMvField(REF_PIC_LIST_1)->getAMVPInfo());
                            xCheckBestMVP(cu, REF_PIC_LIST_1, mvBidir[1], cMvPredBi[1][refIdxBidir[1]], aaiMvpIdxBi[1][refIdxBidir[1]], bits[2], uiCostBi);
                        }
                    }
                }
            } // if (B_SLICE)
        } //end if bTestNormalMC

        //  Clear Motion Field
        cu->getCUMvField(REF_PIC_LIST_0)->setAllMvField(TComMvField(), partSize, partAddr, 0, partIdx);
        cu->getCUMvField(REF_PIC_LIST_1)->setAllMvField(TComMvField(), partSize, partAddr, 0, partIdx);
        cu->getCUMvField(REF_PIC_LIST_0)->setAllMvd(cMvZero, partSize, partAddr, 0, partIdx);
        cu->getCUMvField(REF_PIC_LIST_1)->setAllMvd(cMvZero, partSize, partAddr, 0, partIdx);

        cu->setMVPIdxSubParts(-1, REF_PIC_LIST_0, partAddr, partIdx, cu->getDepth(partAddr));
        cu->setMVPNumSubParts(-1, REF_PIC_LIST_0, partAddr, partIdx, cu->getDepth(partAddr));
        cu->setMVPIdxSubParts(-1, REF_PIC_LIST_1, partAddr, partIdx, cu->getDepth(partAddr));
        cu->setMVPNumSubParts(-1, REF_PIC_LIST_1, partAddr, partIdx, cu->getDepth(partAddr));

        UInt uiMEBits = 0;
        // Set Motion Field_
        mv[1] = mvValidList1;
        refIfx[1] = refIdxValidList1;
        bits[1] = bitsValidList1;
        uiCost[1] = costValidList1;

        if (bTestNormalMC)
        {
            if (uiCostBi <= uiCost[0] && uiCostBi <= uiCost[1])
            {
                uiLastMode = 2;
                {
                    cu->getCUMvField(REF_PIC_LIST_0)->setAllMv(mvBidir[0], partSize, partAddr, 0, partIdx);
                    cu->getCUMvField(REF_PIC_LIST_0)->setAllRefIdx(refIdxBidir[0], partSize, partAddr, 0, partIdx);
                    cu->getCUMvField(REF_PIC_LIST_1)->setAllMv(mvBidir[1], partSize, partAddr, 0, partIdx);
                    cu->getCUMvField(REF_PIC_LIST_1)->setAllRefIdx(refIdxBidir[1], partSize, partAddr, 0, partIdx);
                }
                {
                    TempMv = mvBidir[0] - cMvPredBi[0][refIdxBidir[0]];
                    cu->getCUMvField(REF_PIC_LIST_0)->setAllMvd(TempMv, partSize, partAddr, 0, partIdx);
                }
                {
                    TempMv = mvBidir[1] - cMvPredBi[1][refIdxBidir[1]];
                    cu->getCUMvField(REF_PIC_LIST_1)->setAllMvd(TempMv, partSize, partAddr, 0, partIdx);
                }

                cu->setInterDirSubParts(3, partAddr, partIdx, cu->getDepth(0));

                cu->setMVPIdxSubParts(aaiMvpIdxBi[0][refIdxBidir[0]], REF_PIC_LIST_0, partAddr, partIdx, cu->getDepth(partAddr));
                cu->setMVPNumSubParts(aaiMvpNum[0][refIdxBidir[0]], REF_PIC_LIST_0, partAddr, partIdx, cu->getDepth(partAddr));
                cu->setMVPIdxSubParts(aaiMvpIdxBi[1][refIdxBidir[1]], REF_PIC_LIST_1, partAddr, partIdx, cu->getDepth(partAddr));
                cu->setMVPNumSubParts(aaiMvpNum[1][refIdxBidir[1]], REF_PIC_LIST_1, partAddr, partIdx, cu->getDepth(partAddr));

                uiMEBits = bits[2];
            }
            else if (uiCost[0] <= uiCost[1])
            {
                uiLastMode = 0;
                cu->getCUMvField(REF_PIC_LIST_0)->setAllMv(mv[0], partSize, partAddr, 0, partIdx);
                cu->getCUMvField(REF_PIC_LIST_0)->setAllRefIdx(refIfx[0], partSize, partAddr, 0, partIdx);
                {
                    TempMv = mv[0] - mvPred[0][refIfx[0]];
                    cu->getCUMvField(REF_PIC_LIST_0)->setAllMvd(TempMv, partSize, partAddr, 0, partIdx);
                }
                cu->setInterDirSubParts(1, partAddr, partIdx, cu->getDepth(0));

                cu->setMVPIdxSubParts(aaiMvpIdx[0][refIfx[0]], REF_PIC_LIST_0, partAddr, partIdx, cu->getDepth(partAddr));
                cu->setMVPNumSubParts(aaiMvpNum[0][refIfx[0]], REF_PIC_LIST_0, partAddr, partIdx, cu->getDepth(partAddr));

                uiMEBits = bits[0];
            }
            else
            {
                uiLastMode = 1;
                cu->getCUMvField(REF_PIC_LIST_1)->setAllMv(mv[1], partSize, partAddr, 0, partIdx);
                cu->getCUMvField(REF_PIC_LIST_1)->setAllRefIdx(refIfx[1], partSize, partAddr, 0, partIdx);
                {
                    TempMv = mv[1] - mvPred[1][refIfx[1]];
                    cu->getCUMvField(REF_PIC_LIST_1)->setAllMvd(TempMv, partSize, partAddr, 0, partIdx);
                }
                cu->setInterDirSubParts(2, partAddr, partIdx, cu->getDepth(0));

                cu->setMVPIdxSubParts(aaiMvpIdx[1][refIfx[1]], REF_PIC_LIST_1, partAddr, partIdx, cu->getDepth(partAddr));
                cu->setMVPNumSubParts(aaiMvpNum[1][refIfx[1]], REF_PIC_LIST_1, partAddr, partIdx, cu->getDepth(partAddr));

                uiMEBits = bits[1];
            }
#if CU_STAT_LOGFILE
            meCost += uiCost[0];
#endif
        } // end if bTestNormalMC

        if (cu->getPartitionSize(partAddr) != SIZE_2Nx2N)
        {
            UInt uiMRGInterDir = 0;
            TComMvField cMRGMvField[2];
            UInt uiMRGIndex = 0;

            UInt uiMEInterDir = 0;
            TComMvField cMEMvField[2];

            // calculate ME cost
            UInt uiMEError = MAX_UINT;
            UInt uiMECost = MAX_UINT;

            if (bTestNormalMC)
            {
                uiMEError = xGetInterPredictionError(cu, fencYuv, partIdx);
                uiMECost = uiMEError + m_pcRdCost->getCost(uiMEBits);
            }
            // save ME result.
            uiMEInterDir = cu->getInterDir(partAddr);
            cu->getMvField(cu, partAddr, REF_PIC_LIST_0, cMEMvField[0]);
            cu->getMvField(cu, partAddr, REF_PIC_LIST_1, cMEMvField[1]);

            // find Merge result
            UInt uiMRGCost = MAX_UINT;
            xMergeEstimation(cu, fencYuv, partIdx, uiMRGInterDir, cMRGMvField, uiMRGIndex, uiMRGCost, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand);
            if (uiMRGCost < uiMECost)
            {
                // set Merge result
                cu->setMergeFlagSubParts(true, partAddr, partIdx, cu->getDepth(partAddr));
                cu->setMergeIndexSubParts(uiMRGIndex, partAddr, partIdx, cu->getDepth(partAddr));
                cu->setInterDirSubParts(uiMRGInterDir, partAddr, partIdx, cu->getDepth(partAddr));
                {
                    cu->getCUMvField(REF_PIC_LIST_0)->setAllMvField(cMRGMvField[0], partSize, partAddr, 0, partIdx);
                    cu->getCUMvField(REF_PIC_LIST_1)->setAllMvField(cMRGMvField[1], partSize, partAddr, 0, partIdx);
                }

                cu->getCUMvField(REF_PIC_LIST_0)->setAllMvd(cMvZero, partSize, partAddr, 0, partIdx);
                cu->getCUMvField(REF_PIC_LIST_1)->setAllMvd(cMvZero, partSize, partAddr, 0, partIdx);

                cu->setMVPIdxSubParts(-1, REF_PIC_LIST_0, partAddr, partIdx, cu->getDepth(partAddr));
                cu->setMVPNumSubParts(-1, REF_PIC_LIST_0, partAddr, partIdx, cu->getDepth(partAddr));
                cu->setMVPIdxSubParts(-1, REF_PIC_LIST_1, partAddr, partIdx, cu->getDepth(partAddr));
                cu->setMVPNumSubParts(-1, REF_PIC_LIST_1, partAddr, partIdx, cu->getDepth(partAddr));
#if CU_STAT_LOGFILE
                meCost += uiMRGCost;
#endif
                if (!m_pcEncCfg->getUseRDO())
                    cu->getTotalCost() += uiMRGCost;
            }
            else
            {
                // set ME result
                cu->setMergeFlagSubParts(false, partAddr, partIdx, cu->getDepth(partAddr));
                cu->setInterDirSubParts(uiMEInterDir, partAddr, partIdx, cu->getDepth(partAddr));
                {
                    cu->getCUMvField(REF_PIC_LIST_0)->setAllMvField(cMEMvField[0], partSize, partAddr, 0, partIdx);
                    cu->getCUMvField(REF_PIC_LIST_1)->setAllMvField(cMEMvField[1], partSize, partAddr, 0, partIdx);
                }
#if CU_STAT_LOGFILE
                meCost += uiMECost;
#endif
                if (!m_pcEncCfg->getUseRDO())
                    cu->getTotalCost() += uiMECost;
            }
        }
        else
        {
            if (!m_pcEncCfg->getUseRDO())
                cu->getTotalCost() += costTemp;
        }
        motionCompensation(cu, rpcPredYuv, REF_PIC_LIST_X, partIdx);
    }

    setWpScalingDistParam(cu, -1, REF_PIC_LIST_X);
}

// AMVP
Void TEncSearch::xEstimateMvPredAMVP(TComDataCU* cu, TComYuv* fencYuv, UInt uiPartIdx, RefPicList picList, Int refIfx, MV& mvPred, Bool bFilled, UInt* puiDistBiP)
{
    AMVPInfo* pcAMVPInfo = cu->getCUMvField(picList)->getAMVPInfo();

    MV      cBestMv;
    Int     iBestIdx = 0;
    MV      cZeroMv;
    MV      cMvPred;
    UInt    uiBestCost = MAX_INT;
    UInt    partAddr = 0;
    Int     iRoiWidth, iRoiHeight;
    Int     i;

    cu->getPartIndexAndSize(uiPartIdx, partAddr, iRoiWidth, iRoiHeight);
    // Fill the MV Candidates
    if (!bFilled)
    {
        cu->fillMvpCand(uiPartIdx, partAddr, picList, refIfx, pcAMVPInfo);
    }

    // initialize Mvp index & Mvp
    iBestIdx = 0;
    cBestMv  = pcAMVPInfo->m_acMvCand[0];
    if (pcAMVPInfo->iN <= 1)
    {
        mvPred = cBestMv;

        cu->setMVPIdxSubParts(iBestIdx, picList, partAddr, uiPartIdx, cu->getDepth(partAddr));
        cu->setMVPNumSubParts(pcAMVPInfo->iN, picList, partAddr, uiPartIdx, cu->getDepth(partAddr));

        if (cu->getSlice()->getMvdL1ZeroFlag() && picList == REF_PIC_LIST_1)
        {
            (*puiDistBiP) = xGetTemplateCost(cu, uiPartIdx, partAddr, fencYuv, &m_cYuvPredTemp, mvPred, 0, AMVP_MAX_NUM_CANDS, picList, refIfx, iRoiWidth, iRoiHeight);
        }
        return;
    }
    if (bFilled)
    {
        assert(cu->getMVPIdx(picList, partAddr) >= 0);
        mvPred = pcAMVPInfo->m_acMvCand[cu->getMVPIdx(picList, partAddr)];
        return;
    }

    m_cYuvPredTemp.clear();

    //-- Check Minimum Cost.
    for (i = 0; i < pcAMVPInfo->iN; i++)
    {
        UInt uiTmpCost;
        uiTmpCost = xGetTemplateCost(cu, uiPartIdx, partAddr, fencYuv, &m_cYuvPredTemp, pcAMVPInfo->m_acMvCand[i], i, AMVP_MAX_NUM_CANDS, picList, refIfx, iRoiWidth, iRoiHeight);
        if (uiBestCost > uiTmpCost)
        {
            uiBestCost = uiTmpCost;
            cBestMv   = pcAMVPInfo->m_acMvCand[i];
            iBestIdx  = i;
            (*puiDistBiP) = uiTmpCost;
        }
    }

    m_cYuvPredTemp.clear();

    // Setting Best MVP
    mvPred = cBestMv;
    cu->setMVPIdxSubParts(iBestIdx, picList, partAddr, uiPartIdx, cu->getDepth(partAddr));
    cu->setMVPNumSubParts(pcAMVPInfo->iN, picList, partAddr, uiPartIdx, cu->getDepth(partAddr));
}

UInt TEncSearch::xGetMvpIdxBits(Int idx, Int num)
{
    assert(idx >= 0 && num >= 0 && idx < num);

    if (num == 1)
        return 0;

    UInt uiLength = 1;
    Int iTemp = idx;
    if (iTemp == 0)
    {
        return uiLength;
    }

    Bool bCodeLast = (num - 1 > iTemp);

    uiLength += (iTemp - 1);

    if (bCodeLast)
    {
        uiLength++;
    }

    return uiLength;
}

Void TEncSearch::xGetBlkBits(PartSize cuMode, Bool bPSlice, Int partIdx, UInt lastMode, UInt blockBit[3])
{
    if (cuMode == SIZE_2Nx2N)
    {
        blockBit[0] = (!bPSlice) ? 3 : 1;
        blockBit[1] = 3;
        blockBit[2] = 5;
    }
    else if ((cuMode == SIZE_2NxN || cuMode == SIZE_2NxnU) || cuMode == SIZE_2NxnD)
    {
        UInt aauiMbBits[2][3][3] = { { { 0, 0, 3 }, { 0, 0, 0 }, { 0, 0, 0 } }, { { 5, 7, 7 }, { 7, 5, 7 }, { 9 - 3, 9 - 3, 9 - 3 } } };
        if (bPSlice)
        {
            blockBit[0] = 3;
            blockBit[1] = 0;
            blockBit[2] = 0;
        }
        else
        {
            ::memcpy(blockBit, aauiMbBits[partIdx][lastMode], 3 * sizeof(UInt));
        }
    }
    else if ((cuMode == SIZE_Nx2N || cuMode == SIZE_nLx2N) || cuMode == SIZE_nRx2N)
    {
        UInt aauiMbBits[2][3][3] = { { { 0, 2, 3 }, { 0, 0, 0 }, { 0, 0, 0 } }, { { 5, 7, 7 }, { 7 - 2, 7 - 2, 9 - 2 }, { 9 - 3, 9 - 3, 9 - 3 } } };
        if (bPSlice)
        {
            blockBit[0] = 3;
            blockBit[1] = 0;
            blockBit[2] = 0;
        }
        else
        {
            ::memcpy(blockBit, aauiMbBits[partIdx][lastMode], 3 * sizeof(UInt));
        }
    }
    else if (cuMode == SIZE_NxN)
    {
        blockBit[0] = (!bPSlice) ? 3 : 1;
        blockBit[1] = 3;
        blockBit[2] = 5;
    }
    else
    {
        printf("Wrong!\n");
        assert(0);
    }
}

Void TEncSearch::xCopyAMVPInfo(AMVPInfo* src, AMVPInfo* dst)
{
    dst->iN = src->iN;
    for (Int i = 0; i < src->iN; i++)
    {
        dst->m_acMvCand[i] = src->m_acMvCand[i];
    }
}

/* Check if using an alternative MVP would result in a smaller MVD + signal bits */
Void TEncSearch::xCheckBestMVP(TComDataCU* cu, RefPicList picList, MV mv, MV& mvPred, Int& outMvpIdx, UInt& outBits, UInt& outCost)
{
    AMVPInfo* amvpInfo = cu->getCUMvField(picList)->getAMVPInfo();
    assert(amvpInfo->m_acMvCand[outMvpIdx] == mvPred);
    if (amvpInfo->iN < 2) return;

    m_me.setMVP(mvPred);
    Int bestMvpIdx = outMvpIdx;
    Int mvBitsOrig = m_me.bitcost(mv) + m_mvpIdxCost[outMvpIdx][AMVP_MAX_NUM_CANDS];
    Int bestMvBits = mvBitsOrig;

    for (Int mvpIdx = 0; mvpIdx < amvpInfo->iN; mvpIdx++)
    {
        if (mvpIdx == outMvpIdx)
            continue;

        m_me.setMVP(amvpInfo->m_acMvCand[mvpIdx]);
        Int mvbits = m_me.bitcost(mv) + m_mvpIdxCost[mvpIdx][AMVP_MAX_NUM_CANDS];

        if (mvbits < bestMvBits)
        {
            bestMvBits = mvbits;
            bestMvpIdx = mvpIdx;
        }
    }

    if (bestMvpIdx != outMvpIdx) // if changed
    {
        mvPred = amvpInfo->m_acMvCand[bestMvpIdx];

        outMvpIdx = bestMvpIdx;
        UInt origOutBits = outBits;
        outBits = origOutBits - mvBitsOrig + bestMvBits;
        outCost = (outCost - m_pcRdCost->getCost(origOutBits))  + m_pcRdCost->getCost(outBits);
    }
}

UInt TEncSearch::xGetTemplateCost(TComDataCU* cu,
                                  UInt        uiPartIdx,
                                  UInt        partAddr,
                                  TComYuv*    fencYuv,
                                  TComYuv*    pcTemplateCand,
                                  MV          cMvCand,
                                  Int         iMVPIdx,
                                  Int         iMVPNum,
                                  RefPicList  picList,
                                  Int         refIfx,
                                  Int         iSizeX,
                                  Int         iSizeY)
{
    UInt uiCost  = MAX_INT;

    TComPicYuv* pcPicYuvRef = cu->getSlice()->getRefPic(picList, refIfx)->getPicYuvRec();

    cu->clipMv(cMvCand);

    // prediction pattern
    if (cu->getSlice()->getPPS()->getUseWP() && cu->getSlice()->getSliceType() == P_SLICE)
    {
        TShortYUV *pcMbYuv = &m_acShortPred[0];
        xPredInterLumaBlk(cu, pcPicYuvRef, partAddr, &cMvCand, iSizeX, iSizeY, pcMbYuv, true);
        xWeightedPredictionUni(cu, pcMbYuv, partAddr, iSizeX, iSizeY, picList, pcTemplateCand, refIfx);
    }
    else
    {
        xPredInterLumaBlk(cu, pcPicYuvRef, partAddr, &cMvCand, iSizeX, iSizeY, pcTemplateCand, false);
    }

    // calc distortion
    uiCost = m_me.bufSAD((pixel*)pcTemplateCand->getLumaAddr(partAddr), pcTemplateCand->getStride());
    x265_emms();
    return m_pcRdCost->calcRdSADCost(uiCost, m_mvpIdxCost[iMVPIdx][iMVPNum]);
}

Void TEncSearch::xMotionEstimation(TComDataCU* cu, TComYuv* fencYuv, Int PartIdx, RefPicList picList, MV* pcMvPred, Int RefIdxPred, MV& rcMv, UInt& outBits, UInt& outCost, Bool Bi)
{
    CYCLE_COUNTER_START(ME);
    int cost_shift = 0;
    UInt PartAddr;
    Int RoiWidth;
    Int RoiHeight;

    MV cMvHalf, cMvQter;
    MV cMvSrchRngLT;
    MV cMvSrchRngRB;

    TComYuv* pcYuv = fencYuv;
    m_iSearchRange = m_adaptiveRange[picList][RefIdxPred];
    Int SrchRng = (Bi ? m_bipredSearchRange : m_iSearchRange);
    TComPattern*  pcPatternKey  = cu->getPattern();
    cu->getPartIndexAndSize(PartIdx, PartAddr, RoiWidth, RoiHeight);

    if (Bi)
    {
        TComYuv*  pcYuvOther = &m_acYuvPred[1 - (Int)picList];
        pcYuv  = &m_cYuvPredTemp;
        fencYuv->copyPartToPartYuv(pcYuv, PartAddr, RoiWidth, RoiHeight);
        pcYuv->removeHighFreq(pcYuvOther, PartAddr, RoiWidth, RoiHeight);
        cost_shift = 1;
    }

    //  Search key pattern initialization
    pcPatternKey->initPattern(pcYuv->getLumaAddr(PartAddr), pcYuv->getCbAddr(PartAddr), pcYuv->getCrAddr(PartAddr), RoiWidth,  RoiHeight, pcYuv->getStride(), 0, 0);

    Pel* piRefY = cu->getSlice()->getRefPic(picList, RefIdxPred)->getPicYuvRec()->getLumaAddr(cu->getAddr(), cu->getZorderIdxInCU() + PartAddr);
    Int  RefStride  = cu->getSlice()->getRefPic(picList, RefIdxPred)->getPicYuvRec()->getStride();
    TComPicYuv* refPic = cu->getSlice()->getRefPic(picList, RefIdxPred)->getPicYuvRec(); //For new xPatternSearchFracDiff

    MV cMvPred = *pcMvPred;

    if (Bi)
        xSetSearchRange(cu, rcMv, SrchRng, cMvSrchRngLT, cMvSrchRngRB);
    else
        xSetSearchRange(cu, cMvPred, SrchRng, cMvSrchRngLT, cMvSrchRngRB);

    // Configure the MV bit cost calculator
    m_bc.setMVP(*pcMvPred);

    setWpScalingDistParam(cu, RefIdxPred, picList);

    // Do integer search
    m_pcRdCost->setCostScale(2);
    if (Bi || m_iSearchMethod == X265_FULL_SEARCH)
    {
        xPatternSearch(pcPatternKey, piRefY, RefStride, &cMvSrchRngLT, &cMvSrchRngRB, rcMv, outCost);
    }
    else
    {
        rcMv = *pcMvPred;
        xPatternSearchFast(cu, pcPatternKey, piRefY, RefStride, &cMvSrchRngLT, &cMvSrchRngRB, rcMv, outCost);
    }

    m_pcRdCost->setCostScale(1);
    xPatternSearchFracDIF(cu, pcPatternKey, piRefY, RefStride, &rcMv, cMvHalf, cMvQter, outCost, Bi, refPic, PartAddr);
    m_pcRdCost->setCostScale(0);

    rcMv <<= 2;
    rcMv += (cMvHalf <<= 1);
    rcMv += cMvQter;

    UInt MvBits = m_bc.bitcost(rcMv);

    outBits += MvBits;
    outCost = ((outCost - m_pcRdCost->getCost(MvBits)) >> cost_shift) + m_pcRdCost->getCost(outBits);

    CYCLE_COUNTER_STOP(ME);
}

Void TEncSearch::xSetSearchRange(TComDataCU* cu, MV mvp, Int merange, MV& mvmin, MV& mvmax)
{
    cu->clipMv(mvp);

    MV dist(merange << 2, merange << 2);
    mvmin = mvp - dist;
    mvmax = mvp + dist;

    cu->clipMv(mvmin);
    cu->clipMv(mvmax);

    mvmin >>= 2;
    mvmax >>= 2;
}

Void TEncSearch::xPatternSearch(TComPattern* pcPatternKey, Pel* RefY, Int RefStride, MV* pcMvSrchRngLT, MV* pcMvSrchRngRB, MV& rcMv, UInt& ruiSAD)
{
    Int SrchRngHorLeft   = pcMvSrchRngLT->x;
    Int SrchRngHorRight  = pcMvSrchRngRB->x;
    Int SrchRngVerTop    = pcMvSrchRngLT->y;
    Int SrchRngVerBottom = pcMvSrchRngRB->y;

    m_pcRdCost->setDistParam(pcPatternKey, RefY, RefStride,  m_cDistParam);
    m_cDistParam.bitDepth = g_bitDepthY;
    RefY += (SrchRngVerTop * RefStride);

    // find min. distortion position
    UInt SadBest = MAX_UINT;
    for (Int y = SrchRngVerTop; y <= SrchRngVerBottom; y++)
    {
        for (Int x = SrchRngHorLeft; x <= SrchRngHorRight; x++)
        {
            MV mv(x, y);
            m_cDistParam.pCur = RefY + x;
            UInt Sad = m_cDistParam.DistFunc(&m_cDistParam) + m_bc.mvcost(mv << 2);

            if (Sad < SadBest)
            {
                SadBest = Sad;
                rcMv = mv;
            }
        }
        RefY += RefStride;
    }
    ruiSAD = SadBest - m_bc.mvcost(rcMv << 2);
}

Void TEncSearch::xPatternSearchFast(TComDataCU* cu, TComPattern* pcPatternKey, Pel* RefY, Int RefStride, MV* pcMvSrchRngLT, MV* pcMvSrchRngRB, MV& rcMv, UInt& ruiSAD)
{
    const Int  Raster = 5;
	const Bool AlwaysRasterSearch = 0;
    const Bool TestZeroVector = 1;
    const Bool FirstSearchStop = 1;
    const UInt FirstSearchRounds = 3; /* first search stop X rounds after best match (must be >=1) */
    const Bool EnableRasterSearch = 1;
    const Bool StarRefinementEnable = 1; /* enable either star refinement or raster refinement */
    const UInt StarRefinementRounds = 2; /* star refinement stop X rounds after best match (must be >=1) */

    Int SrchRngHorLeft   = pcMvSrchRngLT->x;
    Int SrchRngHorRight  = pcMvSrchRngRB->x;
    Int SrchRngVerTop    = pcMvSrchRngLT->y;
    Int SrchRngVerBottom = pcMvSrchRngRB->y;
    UInt SearchRange = m_iSearchRange;

    cu->clipMv(rcMv);
    rcMv >>= 2;

    // init TZSearchStruct
    IntTZSearchStruct cStruct;
    cStruct.lumaStride    = RefStride;
    cStruct.fref      = RefY;
    cStruct.bcost   = MAX_UINT;

    // set rcMv (Median predictor) as start point and as best point
    xTZSearchHelp(pcPatternKey, cStruct, rcMv.x, rcMv.y, 0, 0);

    // test whether zero Mv is better start point than Median predictor
    if (TestZeroVector)
    {
        xTZSearchHelp(pcPatternKey, cStruct, 0, 0, 0, 0);
    }

    // start search
    Int Dist = 0;
    Int StartX = cStruct.bestx;
    Int StartY = cStruct.besty;

    // first search
    for (Dist = 1; Dist <= (Int)SearchRange; Dist *= 2)
    {
        xTZ8PointDiamondSearch(pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB, StartX, StartY, Dist);

        if (FirstSearchStop && (cStruct.bestRound >= FirstSearchRounds)) // stop criterion
        {
            break;
        }
    }

    // calculate only 2 missing points instead 8 points if cStruct.uiBestDistance == 1
    if (cStruct.bestDistance == 1)
    {
        cStruct.bestDistance = 0;
        xTZ2PointSearch(pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB);
    }

    // raster search if distance is too big
    if (EnableRasterSearch && (((Int)(cStruct.bestDistance) > Raster) || AlwaysRasterSearch))
    {
        cStruct.bestDistance = Raster;
        for (StartY = SrchRngVerTop; StartY <= SrchRngVerBottom; StartY += Raster)
        {
            for (StartX = SrchRngHorLeft; StartX <= SrchRngHorRight; StartX += Raster)
            {
                xTZSearchHelp(pcPatternKey, cStruct, StartX, StartY, 0, Raster);
            }
        }
    }

    // start refinement
    if (StarRefinementEnable && cStruct.bestDistance > 0)
    {
        while (cStruct.bestDistance > 0)
        {
            StartX = cStruct.bestx;
            StartY = cStruct.besty;
            cStruct.bestDistance = 0;
            cStruct.bestPointDir = 0;
            for (Dist = 1; Dist < (Int)SearchRange + 1; Dist *= 2)
            {
                xTZ8PointDiamondSearch(pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB, StartX, StartY, Dist);
            }

            // calculate only 2 missing points instead 8 points if cStrukt.uiBestDistance == 1
            if (cStruct.bestDistance == 1)
            {
                cStruct.bestDistance = 0;
                if (cStruct.bestPointDir != 0)
                {
                    xTZ2PointSearch(pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB);
                }
            }
        }
    }

    // write out best match
    rcMv = MV(cStruct.bestx, cStruct.besty);
    ruiSAD = cStruct.bcost - m_bc.mvcost(rcMv << 2);
}

Void TEncSearch::xPatternSearchFracDIF(TComDataCU*  cu,
                                       TComPattern* pcPatternKey,
                                       Pel*         piRefY,
                                       Int          iRefStride,
                                       MV*          pcMvInt,
                                       MV&          rcMvHalf,
                                       MV&          rcMvQter,
                                       UInt&        outCost,
                                       Bool         biPred,
                                       TComPicYuv * refPic,
                                       UInt         partAddr)
{
    Int         iOffset    = pcMvInt->x + pcMvInt->y * iRefStride;

    MV baseRefMv(0, 0);

    rcMvHalf = *pcMvInt;
    rcMvHalf <<= 1;

    outCost =  xPatternRefinement(pcPatternKey, baseRefMv, 2, rcMvHalf, refPic, iOffset, cu, partAddr);
    m_pcRdCost->setCostScale(0);

    baseRefMv = rcMvHalf;
    baseRefMv <<= 1;

    rcMvQter = *pcMvInt;
    rcMvQter <<= 1;                         // for mv-cost
    rcMvQter += rcMvHalf;
    rcMvQter <<= 1;

    outCost = xPatternRefinement(pcPatternKey, baseRefMv, 1, rcMvQter, refPic, iOffset, cu, partAddr);
}

/** encode residual and calculate rate-distortion for a CU block
 * \param cu
 * \param fencYuv
 * \param predYuv
 * \param outResiYuv
 * \param rpcYuvResiBest
 * \param rpcYuvRec
 * \param bSkipRes
 * \returns Void
 */
Void TEncSearch::encodeResAndCalcRdInterCU(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, TShortYUV*& outResiYuv, TShortYUV*& rpcYuvResiBest, TComYuv*& rpcYuvRec, Bool bSkipRes)
{
    if (cu->isIntra(0))
    {
        return;
    }

    Bool      bHighPass    = cu->getSlice()->getDepth() ? true : false;
    UInt      uiBits       = 0, uiBitsBest = 0;
    UInt      uiDistortion = 0, uiDistortionBest = 0;

    UInt      uiWidth      = cu->getWidth(0);
    UInt      uiHeight     = cu->getHeight(0);

    //  No residual coding : SKIP mode
    if (bSkipRes)
    {
        cu->setSkipFlagSubParts(true, 0, cu->getDepth(0));

        outResiYuv->clear();

        predYuv->copyToPartYuv(rpcYuvRec, 0);

        uiDistortion = primitives.sse_pp[PartitionFromSizes(uiWidth, uiHeight)]((pixel*)fencYuv->getLumaAddr(), (intptr_t)fencYuv->getStride(), (pixel*)rpcYuvRec->getLumaAddr(), rpcYuvRec->getStride());
        uiDistortion += m_pcRdCost->scaleChromaDistCb(primitives.sse_pp[PartitionFromSizes(uiWidth >> 1, uiHeight >> 1)]((pixel*)fencYuv->getCbAddr(), (intptr_t)fencYuv->getCStride(), (pixel*)rpcYuvRec->getCbAddr(), rpcYuvRec->getCStride()));
        uiDistortion += m_pcRdCost->scaleChromaDistCr(primitives.sse_pp[PartitionFromSizes(uiWidth >> 1, uiHeight >> 1)]((pixel*)fencYuv->getCrAddr(), (intptr_t)fencYuv->getCStride(), (pixel*)rpcYuvRec->getCrAddr(), rpcYuvRec->getCStride()));

        m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[cu->getDepth(0)][CI_CURR_BEST]);
        m_pcEntropyCoder->resetBits();
        if (cu->getSlice()->getPPS()->getTransquantBypassEnableFlag())
        {
            m_pcEntropyCoder->encodeCUTransquantBypassFlag(cu, 0, true);
        }
        m_pcEntropyCoder->encodeSkipFlag(cu, 0, true);
        m_pcEntropyCoder->encodeMergeIndex(cu, 0, true);

        uiBits = m_pcEntropyCoder->getNumberOfWrittenBits();
        m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[cu->getDepth(0)][CI_TEMP_BEST]);

        cu->getTotalBits()       = uiBits;
        cu->getTotalDistortion() = uiDistortion;
        cu->getTotalCost()       = m_pcRdCost->calcRdCost(uiDistortion, uiBits);

        m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[cu->getDepth(0)][CI_TEMP_BEST]);

        cu->setCbfSubParts(0, 0, 0, 0, cu->getDepth(0));
        cu->setTrIdxSubParts(0, 0, cu->getDepth(0));

        return;
    }

    //  Residual coding.
    Int     qp, qpBest = 0;
    UInt64  uiCost, uiCostBest = MAX_INT64;

    UInt uiTrLevel = 0;
    if ((cu->getWidth(0) > cu->getSlice()->getSPS()->getMaxTrSize()))
    {
        while (cu->getWidth(0) > (cu->getSlice()->getSPS()->getMaxTrSize() << uiTrLevel))
        {
            uiTrLevel++;
        }
    }
    UInt uiMaxTrMode = 1 + uiTrLevel;

    while ((uiWidth >> uiMaxTrMode) < (g_maxCUWidth >> g_maxCUDepth))
    {
        uiMaxTrMode--;
    }

    qp = bHighPass ? Clip3(-cu->getSlice()->getSPS()->getQpBDOffsetY(), MAX_QP, (Int)cu->getQP(0)) : cu->getQP(0);

    outResiYuv->subtract(fencYuv, predYuv, 0, uiWidth);

    uiCost = 0;
    uiBits = 0;
    uiDistortion = 0;
    m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[cu->getDepth(0)][CI_CURR_BEST]);

    UInt uiZeroDistortion = 0;
    xEstimateResidualQT(cu, 0, 0, 0, outResiYuv,  cu->getDepth(0), uiCost, uiBits, uiDistortion, &uiZeroDistortion);

    UInt zeroResiBits;

    m_pcEntropyCoder->resetBits();
    m_pcEntropyCoder->encodeQtRootCbfZero(cu);
    zeroResiBits = m_pcEntropyCoder->getNumberOfWrittenBits();

    UInt64 uiZeroCost = m_pcRdCost->calcRdCost(uiZeroDistortion, zeroResiBits);
    if (cu->isLosslessCoded(0))
    {
        uiZeroCost = uiCost + 1;
    }
    if (uiZeroCost < uiCost)
    {
        uiCost       = uiZeroCost;
        uiBits       = 0;
        uiDistortion = uiZeroDistortion;

        const UInt uiQPartNum = cu->getPic()->getNumPartInCU() >> (cu->getDepth(0) << 1);
        ::memset(cu->getTransformIdx(), 0, uiQPartNum * sizeof(UChar));
        ::memset(cu->getCbf(TEXT_LUMA), 0, uiQPartNum * sizeof(UChar));
        ::memset(cu->getCbf(TEXT_CHROMA_U), 0, uiQPartNum * sizeof(UChar));
        ::memset(cu->getCbf(TEXT_CHROMA_V), 0, uiQPartNum * sizeof(UChar));
        ::memset(cu->getCoeffY(), 0, uiWidth * uiHeight * sizeof(TCoeff));
        ::memset(cu->getCoeffCb(), 0, uiWidth * uiHeight * sizeof(TCoeff) >> 2);
        ::memset(cu->getCoeffCr(), 0, uiWidth * uiHeight * sizeof(TCoeff) >> 2);
        cu->setTransformSkipSubParts(0, 0, 0, 0, cu->getDepth(0));
    }
    else
    {
        xSetResidualQTData(cu, 0, 0, 0, NULL, cu->getDepth(0), false);
    }

    m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[cu->getDepth(0)][CI_CURR_BEST]);

    uiBits = 0;
    {
        TShortYUV *pDummy = NULL;
        xAddSymbolBitsInter(cu, 0, 0, uiBits, pDummy, NULL, pDummy);
    }

    UInt64 dExactCost = m_pcRdCost->calcRdCost(uiDistortion, uiBits);
    uiCost = dExactCost;

    if (uiCost < uiCostBest)
    {
        if (!cu->getQtRootCbf(0))
        {
            rpcYuvResiBest->clear();
        }
        else
        {
            xSetResidualQTData(cu, 0, 0, 0, rpcYuvResiBest, cu->getDepth(0), true);
        }

        uiBitsBest       = uiBits;
        uiDistortionBest = uiDistortion;
        uiCostBest       = uiCost;
        qpBest           = qp;
        m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[cu->getDepth(0)][CI_TEMP_BEST]);
    }

    assert(uiCostBest != MAX_INT64);

    rpcYuvRec->addClip(predYuv, rpcYuvResiBest, 0, uiWidth);

    // update with clipped distortion and cost (qp estimation loop uses unclipped values)
    uiDistortionBest = primitives.sse_pp[PartitionFromSizes(uiWidth, uiHeight)]((pixel*)fencYuv->getLumaAddr(), (intptr_t)fencYuv->getStride(), (pixel*)rpcYuvRec->getLumaAddr(), rpcYuvRec->getStride());
    uiDistortionBest += m_pcRdCost->scaleChromaDistCb(primitives.sse_pp[PartitionFromSizes(uiWidth >> 1, uiHeight >> 1)]((pixel*)fencYuv->getCbAddr(), (intptr_t)fencYuv->getCStride(), (pixel*)rpcYuvRec->getCbAddr(), rpcYuvRec->getCStride()));
    uiDistortionBest += m_pcRdCost->scaleChromaDistCr(primitives.sse_pp[PartitionFromSizes(uiWidth >> 1, uiHeight >> 1)]((pixel*)fencYuv->getCrAddr(), (intptr_t)fencYuv->getCStride(), (pixel*)rpcYuvRec->getCrAddr(), rpcYuvRec->getCStride()));
    uiCostBest = m_pcRdCost->calcRdCost(uiDistortionBest, uiBitsBest);

    cu->getTotalBits()       = uiBitsBest;
    cu->getTotalDistortion() = uiDistortionBest;
    cu->getTotalCost()       = uiCostBest;

    if (cu->isSkipped(0))
    {
        cu->setCbfSubParts(0, 0, 0, 0, cu->getDepth(0));
    }

    cu->setQPSubParts(qpBest, 0, cu->getDepth(0));
}

#if _MSC_VER
#pragma warning(disable: 4701) // potentially uninitialized local variable
#endif

Void TEncSearch::xEstimateResidualQT(TComDataCU* cu,
                                     UInt        uiQuadrant,
                                     UInt        uiAbsPartIdx,
                                     UInt        absTUPartIdx,
                                     TShortYUV*  pcResi,
                                     const UInt  uiDepth,
                                     UInt64 &    rdCost,
                                     UInt &      outBits,
                                     UInt &      ruiDist,
                                     UInt *      puiZeroDist)
{
    const UInt uiTrMode = uiDepth - cu->getDepth(0);

    assert(cu->getDepth(0) == cu->getDepth(uiAbsPartIdx));
    const UInt uiLog2TrSize = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> uiDepth] + 2;

    UInt SplitFlag = ((cu->getSlice()->getSPS()->getQuadtreeTUMaxDepthInter() == 1) && cu->getPredictionMode(uiAbsPartIdx) == MODE_INTER && (cu->getPartitionSize(uiAbsPartIdx) != SIZE_2Nx2N));
    Bool bCheckFull;
    if (SplitFlag && uiDepth == cu->getDepth(uiAbsPartIdx) && (uiLog2TrSize > cu->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx)))
        bCheckFull = false;
    else
        bCheckFull =  (uiLog2TrSize <= cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize());

    const Bool bCheckSplit  = (uiLog2TrSize > cu->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx));

    assert(bCheckFull || bCheckSplit);

    Bool  bCodeChroma   = true;
    UInt  uiTrModeC     = uiTrMode;
    UInt  uiLog2TrSizeC = uiLog2TrSize - 1;
    if (uiLog2TrSize == 2)
    {
        uiLog2TrSizeC++;
        uiTrModeC--;
        UInt  uiQPDiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + uiTrModeC) << 1);
        bCodeChroma   = ((uiAbsPartIdx % uiQPDiv) == 0);
    }

    const UInt uiSetCbf = 1 << uiTrMode;
    // code full block
    UInt64 dSingleCost = MAX_INT64;
    UInt uiSingleBits = 0;
    UInt uiSingleDist = 0;
    UInt uiAbsSumY = 0, uiAbsSumU = 0, uiAbsSumV = 0;
    UInt uiBestTransformMode[3] = { 0 };

    m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiDepth][CI_QT_TRAFO_ROOT]);

    if (bCheckFull)
    {
        const UInt uiNumCoeffPerAbsPartIdxIncrement = cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight() >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1);
        const UInt uiQTTempAccessLayer = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;
        TCoeff *pcCoeffCurrY = m_ppcQTTempCoeffY[uiQTTempAccessLayer] +  uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx;
        TCoeff *pcCoeffCurrU = m_ppcQTTempCoeffCb[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx >> 2);
        TCoeff *pcCoeffCurrV = m_ppcQTTempCoeffCr[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx >> 2);
        Int *pcArlCoeffCurrY = m_ppcQTTempArlCoeffY[uiQTTempAccessLayer] +  uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx;
        Int *pcArlCoeffCurrU = m_ppcQTTempArlCoeffCb[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx >> 2);
        Int *pcArlCoeffCurrV = m_ppcQTTempArlCoeffCr[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx >> 2);

        Int trWidth = 0, trHeight = 0, trWidthC = 0, trHeightC = 0;
        UInt absTUPartIdxC = uiAbsPartIdx;

        trWidth  = trHeight  = 1 << uiLog2TrSize;
        trWidthC = trHeightC = 1 << uiLog2TrSizeC;
        cu->setTrIdxSubParts(uiDepth - cu->getDepth(0), uiAbsPartIdx, uiDepth);
        UInt64 minCostY = MAX_INT64;
        UInt64 minCostU = MAX_INT64;
        UInt64 minCostV = MAX_INT64;
        Bool checkTransformSkipY  = cu->getSlice()->getPPS()->getUseTransformSkip() && trWidth == 4 && trHeight == 4;
        Bool checkTransformSkipUV = cu->getSlice()->getPPS()->getUseTransformSkip() && trWidthC == 4 && trHeightC == 4;

        checkTransformSkipY         &= (!cu->isLosslessCoded(0));
        checkTransformSkipUV        &= (!cu->isLosslessCoded(0));

        cu->setTransformSkipSubParts(0, TEXT_LUMA, uiAbsPartIdx, uiDepth);
        if (bCodeChroma)
        {
            cu->setTransformSkipSubParts(0, TEXT_CHROMA_U, uiAbsPartIdx, cu->getDepth(0) + uiTrModeC);
            cu->setTransformSkipSubParts(0, TEXT_CHROMA_V, uiAbsPartIdx, cu->getDepth(0) + uiTrModeC);
        }

        if (m_pcEncCfg->getUseRDOQ())
        {
            m_pcEntropyCoder->estimateBit(m_pcTrQuant->m_estBitsSbac, trWidth, trHeight, TEXT_LUMA);
        }

        m_pcTrQuant->setQPforQuant(cu->getQP(0), TEXT_LUMA, cu->getSlice()->getSPS()->getQpBDOffsetY(), 0);

        m_pcTrQuant->selectLambda(TEXT_LUMA);

        uiAbsSumY = m_pcTrQuant->transformNxN(cu, pcResi->getLumaAddr(absTUPartIdx), pcResi->width, pcCoeffCurrY,
                                              pcArlCoeffCurrY, trWidth, trHeight, TEXT_LUMA, uiAbsPartIdx);

        cu->setCbfSubParts(uiAbsSumY ? uiSetCbf : 0, TEXT_LUMA, uiAbsPartIdx, uiDepth);

        if (bCodeChroma)
        {
            if (m_pcEncCfg->getUseRDOQ())
            {
                m_pcEntropyCoder->estimateBit(m_pcTrQuant->m_estBitsSbac, trWidthC, trHeightC, TEXT_CHROMA);
            }

            Int curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCbQpOffset() + cu->getSlice()->getSliceQpDeltaCb();
            m_pcTrQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset);

            m_pcTrQuant->selectLambda(TEXT_CHROMA);

            uiAbsSumU = m_pcTrQuant->transformNxN(cu, pcResi->getCbAddr(absTUPartIdxC), pcResi->Cwidth, pcCoeffCurrU,
                                                  pcArlCoeffCurrU, trWidthC, trHeightC, TEXT_CHROMA_U, uiAbsPartIdx);

            curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCrQpOffset() + cu->getSlice()->getSliceQpDeltaCr();
            m_pcTrQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset);
            uiAbsSumV = m_pcTrQuant->transformNxN(cu, pcResi->getCrAddr(absTUPartIdxC), pcResi->Cwidth, pcCoeffCurrV,
                                                  pcArlCoeffCurrV, trWidthC, trHeightC, TEXT_CHROMA_V, uiAbsPartIdx);

            cu->setCbfSubParts(uiAbsSumU ? uiSetCbf : 0, TEXT_CHROMA_U, uiAbsPartIdx, cu->getDepth(0) + uiTrModeC);
            cu->setCbfSubParts(uiAbsSumV ? uiSetCbf : 0, TEXT_CHROMA_V, uiAbsPartIdx, cu->getDepth(0) + uiTrModeC);
        }

        m_pcEntropyCoder->resetBits();

        {
            m_pcEntropyCoder->encodeQtCbf(cu, uiAbsPartIdx, TEXT_LUMA,     uiTrMode);
        }

        m_pcEntropyCoder->encodeCoeffNxN(cu, pcCoeffCurrY, uiAbsPartIdx,  trWidth,  trHeight,    uiDepth, TEXT_LUMA);
        const UInt uiSingleBitsY = m_pcEntropyCoder->getNumberOfWrittenBits();

        UInt uiSingleBitsU = 0;
        UInt uiSingleBitsV = 0;
        if (bCodeChroma)
        {
            {
                m_pcEntropyCoder->encodeQtCbf(cu, uiAbsPartIdx, TEXT_CHROMA_U, uiTrMode);
            }
            m_pcEntropyCoder->encodeCoeffNxN(cu, pcCoeffCurrU, uiAbsPartIdx, trWidthC, trHeightC, uiDepth, TEXT_CHROMA_U);
            uiSingleBitsU = m_pcEntropyCoder->getNumberOfWrittenBits() - uiSingleBitsY;

            {
                m_pcEntropyCoder->encodeQtCbf(cu, uiAbsPartIdx, TEXT_CHROMA_V, uiTrMode);
            }
            m_pcEntropyCoder->encodeCoeffNxN(cu, pcCoeffCurrV, uiAbsPartIdx, trWidthC, trHeightC, uiDepth, TEXT_CHROMA_V);
            uiSingleBitsV = m_pcEntropyCoder->getNumberOfWrittenBits() - (uiSingleBitsY + uiSingleBitsU);
        }

        const UInt uiNumSamplesLuma = 1 << (uiLog2TrSize << 1);
        const UInt uiNumSamplesChro = 1 << (uiLog2TrSizeC << 1);

        ::memset(m_pTempPel, 0, sizeof(Pel) * uiNumSamplesLuma); // not necessary needed for inside of recursion (only at the beginning)

        int Part = PartitionFromSizes(trWidth, trHeight);
        UInt uiDistY = primitives.sse_sp[Part](pcResi->getLumaAddr(absTUPartIdx), (intptr_t)pcResi->width, (pixel*)m_pTempPel, trWidth);

        if (puiZeroDist)
        {
            *puiZeroDist += uiDistY;
        }
        if (uiAbsSumY)
        {
            Short *pcResiCurrY = m_pcQTTempTComYuv[uiQTTempAccessLayer].getLumaAddr(absTUPartIdx);

            m_pcTrQuant->setQPforQuant(cu->getQP(0), TEXT_LUMA, cu->getSlice()->getSPS()->getQpBDOffsetY(), 0);

            Int scalingListType = 3 + g_eTTable[(Int)TEXT_LUMA];
            assert(scalingListType < 6);
            m_pcTrQuant->invtransformNxN(cu->getCUTransquantBypass(uiAbsPartIdx), TEXT_LUMA, REG_DCT, pcResiCurrY, m_pcQTTempTComYuv[uiQTTempAccessLayer].width,  pcCoeffCurrY, trWidth, trHeight, scalingListType); //this is for inter mode only

            const UInt uiNonzeroDistY = primitives.sse_ss[Part](pcResi->getLumaAddr(absTUPartIdx), pcResi->width, m_pcQTTempTComYuv[uiQTTempAccessLayer].getLumaAddr(absTUPartIdx),
                                                                m_pcQTTempTComYuv[uiQTTempAccessLayer].width);
            if (cu->isLosslessCoded(0))
            {
                uiDistY = uiNonzeroDistY;
            }
            else
            {
                const UInt64 singleCostY = m_pcRdCost->calcRdCost(uiNonzeroDistY, uiSingleBitsY);
                m_pcEntropyCoder->resetBits();
                m_pcEntropyCoder->encodeQtCbfZero(cu, TEXT_LUMA,     uiTrMode);
                const UInt uiNullBitsY = m_pcEntropyCoder->getNumberOfWrittenBits();
                const UInt64 nullCostY = m_pcRdCost->calcRdCost(uiDistY, uiNullBitsY);
                if (nullCostY < singleCostY)
                {
                    uiAbsSumY = 0;
                    ::memset(pcCoeffCurrY, 0, sizeof(TCoeff) * uiNumSamplesLuma);
                    if (checkTransformSkipY)
                    {
                        minCostY = nullCostY;
                    }
                }
                else
                {
                    uiDistY = uiNonzeroDistY;
                    if (checkTransformSkipY)
                    {
                        minCostY = singleCostY;
                    }
                }
            }
        }
        else if (checkTransformSkipY)
        {
            m_pcEntropyCoder->resetBits();
            m_pcEntropyCoder->encodeQtCbfZero(cu, TEXT_LUMA, uiTrMode);
            const UInt uiNullBitsY = m_pcEntropyCoder->getNumberOfWrittenBits();
            minCostY = m_pcRdCost->calcRdCost(uiDistY, uiNullBitsY);
        }

        if (!uiAbsSumY)
        {
            Short *pcPtr =  m_pcQTTempTComYuv[uiQTTempAccessLayer].getLumaAddr(absTUPartIdx);
            const UInt uiStride = m_pcQTTempTComYuv[uiQTTempAccessLayer].width;
            for (UInt uiY = 0; uiY < trHeight; ++uiY)
            {
                ::memset(pcPtr, 0, sizeof(Short) * trWidth);
                pcPtr += uiStride;
            }
        }

        UInt uiDistU = 0;
        UInt uiDistV = 0;

        int PartC = x265::PartitionFromSizes(trWidthC, trHeightC);
        if (bCodeChroma)
        {
            uiDistU = m_pcRdCost->scaleChromaDistCb(primitives.sse_sp[PartC](pcResi->getCbAddr(absTUPartIdxC), (intptr_t)pcResi->Cwidth, (pixel*)m_pTempPel, trWidthC));

            if (puiZeroDist)
            {
                *puiZeroDist += uiDistU;
            }
            if (uiAbsSumU)
            {
                Short *pcResiCurrU = m_pcQTTempTComYuv[uiQTTempAccessLayer].getCbAddr(absTUPartIdxC);

                Int curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCbQpOffset() + cu->getSlice()->getSliceQpDeltaCb();
                m_pcTrQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset);

                Int scalingListType = 3 + g_eTTable[(Int)TEXT_CHROMA_U];
                assert(scalingListType < 6);
                m_pcTrQuant->invtransformNxN(cu->getCUTransquantBypass(uiAbsPartIdx), TEXT_CHROMA, REG_DCT, pcResiCurrU, m_pcQTTempTComYuv[uiQTTempAccessLayer].Cwidth, pcCoeffCurrU, trWidthC, trHeightC, scalingListType);

                const UInt uiNonzeroDistU = m_pcRdCost->scaleChromaDistCb(primitives.sse_ss[PartC](pcResi->getCbAddr(absTUPartIdxC), pcResi->Cwidth,
                                                                                                   m_pcQTTempTComYuv[uiQTTempAccessLayer].getCbAddr(absTUPartIdxC), m_pcQTTempTComYuv[uiQTTempAccessLayer].Cwidth));
                if (cu->isLosslessCoded(0))
                {
                    uiDistU = uiNonzeroDistU;
                }
                else
                {
                    const UInt64 dSingleCostU = m_pcRdCost->calcRdCost(uiNonzeroDistU, uiSingleBitsU);
                    m_pcEntropyCoder->resetBits();
                    m_pcEntropyCoder->encodeQtCbfZero(cu, TEXT_CHROMA_U,     uiTrMode);
                    const UInt uiNullBitsU = m_pcEntropyCoder->getNumberOfWrittenBits();
                    const UInt64 dNullCostU = m_pcRdCost->calcRdCost(uiDistU, uiNullBitsU);
                    if (dNullCostU < dSingleCostU)
                    {
                        uiAbsSumU = 0;
                        ::memset(pcCoeffCurrU, 0, sizeof(TCoeff) * uiNumSamplesChro);
                        if (checkTransformSkipUV)
                        {
                            minCostU = dNullCostU;
                        }
                    }
                    else
                    {
                        uiDistU = uiNonzeroDistU;
                        if (checkTransformSkipUV)
                        {
                            minCostU = dSingleCostU;
                        }
                    }
                }
            }
            else if (checkTransformSkipUV)
            {
                m_pcEntropyCoder->resetBits();
                m_pcEntropyCoder->encodeQtCbfZero(cu, TEXT_CHROMA_U, uiTrModeC);
                const UInt uiNullBitsU = m_pcEntropyCoder->getNumberOfWrittenBits();
                minCostU = m_pcRdCost->calcRdCost(uiDistU, uiNullBitsU);
            }
            if (!uiAbsSumU)
            {
                Short *pcPtr =  m_pcQTTempTComYuv[uiQTTempAccessLayer].getCbAddr(absTUPartIdxC);
                const UInt uiStride = m_pcQTTempTComYuv[uiQTTempAccessLayer].Cwidth;
                for (UInt uiY = 0; uiY < trHeightC; ++uiY)
                {
                    ::memset(pcPtr, 0, sizeof(Short) * trWidthC);
                    pcPtr += uiStride;
                }
            }

            uiDistV = m_pcRdCost->scaleChromaDistCr(primitives.sse_sp[PartC](pcResi->getCrAddr(absTUPartIdxC), (intptr_t)pcResi->Cwidth, (pixel*)m_pTempPel, trWidthC));
            if (puiZeroDist)
            {
                *puiZeroDist += uiDistV;
            }
            if (uiAbsSumV)
            {
                Short *pcResiCurrV = m_pcQTTempTComYuv[uiQTTempAccessLayer].getCrAddr(absTUPartIdxC);
                Int curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCrQpOffset() + cu->getSlice()->getSliceQpDeltaCr();
                m_pcTrQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset);

                Int scalingListType = 3 + g_eTTable[(Int)TEXT_CHROMA_V];
                assert(scalingListType < 6);
                m_pcTrQuant->invtransformNxN(cu->getCUTransquantBypass(uiAbsPartIdx), TEXT_CHROMA, REG_DCT, pcResiCurrV, m_pcQTTempTComYuv[uiQTTempAccessLayer].Cwidth, pcCoeffCurrV, trWidthC, trHeightC, scalingListType);

                const UInt uiNonzeroDistV = m_pcRdCost->scaleChromaDistCr(primitives.sse_ss[PartC](pcResi->getCrAddr(absTUPartIdxC), pcResi->Cwidth,
                                                                                                   m_pcQTTempTComYuv[uiQTTempAccessLayer].getCrAddr(absTUPartIdxC), m_pcQTTempTComYuv[uiQTTempAccessLayer].Cwidth));

                if (cu->isLosslessCoded(0))
                {
                    uiDistV = uiNonzeroDistV;
                }
                else
                {
                    const UInt64 dSingleCostV = m_pcRdCost->calcRdCost(uiNonzeroDistV, uiSingleBitsV);
                    m_pcEntropyCoder->resetBits();
                    m_pcEntropyCoder->encodeQtCbfZero(cu, TEXT_CHROMA_V,     uiTrMode);
                    const UInt uiNullBitsV = m_pcEntropyCoder->getNumberOfWrittenBits();
                    const UInt64 dNullCostV = m_pcRdCost->calcRdCost(uiDistV, uiNullBitsV);
                    if (dNullCostV < dSingleCostV)
                    {
                        uiAbsSumV = 0;
                        ::memset(pcCoeffCurrV, 0, sizeof(TCoeff) * uiNumSamplesChro);
                        if (checkTransformSkipUV)
                        {
                            minCostV = dNullCostV;
                        }
                    }
                    else
                    {
                        uiDistV = uiNonzeroDistV;
                        if (checkTransformSkipUV)
                        {
                            minCostV = dSingleCostV;
                        }
                    }
                }
            }
            else if (checkTransformSkipUV)
            {
                m_pcEntropyCoder->resetBits();
                m_pcEntropyCoder->encodeQtCbfZero(cu, TEXT_CHROMA_V, uiTrModeC);
                const UInt uiNullBitsV = m_pcEntropyCoder->getNumberOfWrittenBits();
                minCostV = m_pcRdCost->calcRdCost(uiDistV, uiNullBitsV);
            }
            if (!uiAbsSumV)
            {
                Short *pcPtr =  m_pcQTTempTComYuv[uiQTTempAccessLayer].getCrAddr(absTUPartIdxC);
                const UInt uiStride = m_pcQTTempTComYuv[uiQTTempAccessLayer].Cwidth;
                for (UInt uiY = 0; uiY < trHeightC; ++uiY)
                {
                    ::memset(pcPtr, 0, sizeof(Short) * trWidthC);
                    pcPtr += uiStride;
                }
            }
        }
        cu->setCbfSubParts(uiAbsSumY ? uiSetCbf : 0, TEXT_LUMA, uiAbsPartIdx, uiDepth);
        if (bCodeChroma)
        {
            cu->setCbfSubParts(uiAbsSumU ? uiSetCbf : 0, TEXT_CHROMA_U, uiAbsPartIdx, cu->getDepth(0) + uiTrModeC);
            cu->setCbfSubParts(uiAbsSumV ? uiSetCbf : 0, TEXT_CHROMA_V, uiAbsPartIdx, cu->getDepth(0) + uiTrModeC);
        }

        if (checkTransformSkipY)
        {
            UInt uiNonzeroDistY = 0, uiAbsSumTransformSkipY;
            UInt64 dSingleCostY = MAX_INT64;

            Short *pcResiCurrY = m_pcQTTempTComYuv[uiQTTempAccessLayer].getLumaAddr(absTUPartIdx);
            UInt resiYStride = m_pcQTTempTComYuv[uiQTTempAccessLayer].width;

            TCoeff bestCoeffY[32 * 32];
            memcpy(bestCoeffY, pcCoeffCurrY, sizeof(TCoeff) * uiNumSamplesLuma);

            TCoeff bestArlCoeffY[32 * 32];
            memcpy(bestArlCoeffY, pcArlCoeffCurrY, sizeof(TCoeff) * uiNumSamplesLuma);

            Short bestResiY[32 * 32];
            for (Int i = 0; i < trHeight; ++i)
            {
                memcpy(bestResiY + i * trWidth, pcResiCurrY + i * resiYStride, sizeof(Short) * trWidth);
            }

            m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_QT_TRAFO_ROOT]);

            cu->setTransformSkipSubParts(1, TEXT_LUMA, uiAbsPartIdx, uiDepth);

            if (m_pcEncCfg->getUseRDOQTS())
            {
                m_pcEntropyCoder->estimateBit(m_pcTrQuant->m_estBitsSbac, trWidth, trHeight, TEXT_LUMA);
            }

            m_pcTrQuant->setQPforQuant(cu->getQP(0), TEXT_LUMA, cu->getSlice()->getSPS()->getQpBDOffsetY(), 0);

            m_pcTrQuant->selectLambda(TEXT_LUMA);
            uiAbsSumTransformSkipY = m_pcTrQuant->transformNxN(cu, pcResi->getLumaAddr(absTUPartIdx), pcResi->width, pcCoeffCurrY,
                                                               pcArlCoeffCurrY, trWidth, trHeight, TEXT_LUMA, uiAbsPartIdx, true);
            cu->setCbfSubParts(uiAbsSumTransformSkipY ? uiSetCbf : 0, TEXT_LUMA, uiAbsPartIdx, uiDepth);

            if (uiAbsSumTransformSkipY != 0)
            {
                m_pcEntropyCoder->resetBits();
                m_pcEntropyCoder->encodeQtCbf(cu, uiAbsPartIdx, TEXT_LUMA, uiTrMode);
                m_pcEntropyCoder->encodeCoeffNxN(cu, pcCoeffCurrY, uiAbsPartIdx, trWidth, trHeight, uiDepth, TEXT_LUMA);
                const UInt uiTsSingleBitsY = m_pcEntropyCoder->getNumberOfWrittenBits();

                m_pcTrQuant->setQPforQuant(cu->getQP(0), TEXT_LUMA, cu->getSlice()->getSPS()->getQpBDOffsetY(), 0);

                Int scalingListType = 3 + g_eTTable[(Int)TEXT_LUMA];
                assert(scalingListType < 6);

                m_pcTrQuant->invtransformNxN(cu->getCUTransquantBypass(uiAbsPartIdx), TEXT_LUMA, REG_DCT, pcResiCurrY, m_pcQTTempTComYuv[uiQTTempAccessLayer].width,  pcCoeffCurrY, trWidth, trHeight, scalingListType, true);

                uiNonzeroDistY = primitives.sse_ss[Part](pcResi->getLumaAddr(absTUPartIdx), pcResi->width,
                                                         m_pcQTTempTComYuv[uiQTTempAccessLayer].getLumaAddr(absTUPartIdx), m_pcQTTempTComYuv[uiQTTempAccessLayer].width);

                dSingleCostY = m_pcRdCost->calcRdCost(uiNonzeroDistY, uiTsSingleBitsY);
            }

            if (!uiAbsSumTransformSkipY || minCostY < dSingleCostY)
            {
                cu->setTransformSkipSubParts(0, TEXT_LUMA, uiAbsPartIdx, uiDepth);
                memcpy(pcCoeffCurrY, bestCoeffY, sizeof(TCoeff) * uiNumSamplesLuma);
                memcpy(pcArlCoeffCurrY, bestArlCoeffY, sizeof(TCoeff) * uiNumSamplesLuma);
                for (Int i = 0; i < trHeight; ++i)
                {
                    memcpy(pcResiCurrY + i * resiYStride, &bestResiY[i * trWidth], sizeof(Short) * trWidth);
                }
            }
            else
            {
                uiDistY = uiNonzeroDistY;
                uiAbsSumY = uiAbsSumTransformSkipY;
                uiBestTransformMode[0] = 1;
            }

            cu->setCbfSubParts(uiAbsSumY ? uiSetCbf : 0, TEXT_LUMA, uiAbsPartIdx, uiDepth);
        }

        if (bCodeChroma && checkTransformSkipUV)
        {
            UInt uiNonzeroDistU = 0, uiNonzeroDistV = 0, uiAbsSumTransformSkipU, uiAbsSumTransformSkipV;
            UInt64 dSingleCostU = MAX_INT64;
            UInt64 dSingleCostV = MAX_INT64;

            Short *pcResiCurrU = m_pcQTTempTComYuv[uiQTTempAccessLayer].getCbAddr(absTUPartIdxC);
            Short *pcResiCurrV = m_pcQTTempTComYuv[uiQTTempAccessLayer].getCrAddr(absTUPartIdxC);
            UInt resiCStride = m_pcQTTempTComYuv[uiQTTempAccessLayer].Cwidth;

            TCoeff bestCoeffU[32 * 32], bestCoeffV[32 * 32];
            memcpy(bestCoeffU, pcCoeffCurrU, sizeof(TCoeff) * uiNumSamplesChro);
            memcpy(bestCoeffV, pcCoeffCurrV, sizeof(TCoeff) * uiNumSamplesChro);

            TCoeff bestArlCoeffU[32 * 32], bestArlCoeffV[32 * 32];
            memcpy(bestArlCoeffU, pcArlCoeffCurrU, sizeof(TCoeff) * uiNumSamplesChro);
            memcpy(bestArlCoeffV, pcArlCoeffCurrV, sizeof(TCoeff) * uiNumSamplesChro);

            Short bestResiU[32 * 32], bestResiV[32 * 32];
            for (Int i = 0; i < trHeightC; ++i)
            {
                memcpy(&bestResiU[i * trWidthC], pcResiCurrU + i * resiCStride, sizeof(Short) * trWidthC);
                memcpy(&bestResiV[i * trWidthC], pcResiCurrV + i * resiCStride, sizeof(Short) * trWidthC);
            }

            m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_QT_TRAFO_ROOT]);

            cu->setTransformSkipSubParts(1, TEXT_CHROMA_U, uiAbsPartIdx, cu->getDepth(0) + uiTrModeC);
            cu->setTransformSkipSubParts(1, TEXT_CHROMA_V, uiAbsPartIdx, cu->getDepth(0) + uiTrModeC);

            if (m_pcEncCfg->getUseRDOQTS())
            {
                m_pcEntropyCoder->estimateBit(m_pcTrQuant->m_estBitsSbac, trWidthC, trHeightC, TEXT_CHROMA);
            }

            Int curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCbQpOffset() + cu->getSlice()->getSliceQpDeltaCb();
            m_pcTrQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset);

            m_pcTrQuant->selectLambda(TEXT_CHROMA);

            uiAbsSumTransformSkipU = m_pcTrQuant->transformNxN(cu, pcResi->getCbAddr(absTUPartIdxC), pcResi->Cwidth, pcCoeffCurrU,
                                                               pcArlCoeffCurrU, trWidthC, trHeightC, TEXT_CHROMA_U, uiAbsPartIdx, true);
            curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCrQpOffset() + cu->getSlice()->getSliceQpDeltaCr();
            m_pcTrQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset);
            uiAbsSumTransformSkipV = m_pcTrQuant->transformNxN(cu, pcResi->getCrAddr(absTUPartIdxC), pcResi->Cwidth, pcCoeffCurrV,
                                                               pcArlCoeffCurrV, trWidthC, trHeightC, TEXT_CHROMA_V, uiAbsPartIdx, true);

            cu->setCbfSubParts(uiAbsSumTransformSkipU ? uiSetCbf : 0, TEXT_CHROMA_U, uiAbsPartIdx, cu->getDepth(0) + uiTrModeC);
            cu->setCbfSubParts(uiAbsSumTransformSkipV ? uiSetCbf : 0, TEXT_CHROMA_V, uiAbsPartIdx, cu->getDepth(0) + uiTrModeC);

            m_pcEntropyCoder->resetBits();
            uiSingleBitsU = 0;
            uiSingleBitsV = 0;

            if (uiAbsSumTransformSkipU)
            {
                m_pcEntropyCoder->encodeQtCbf(cu, uiAbsPartIdx, TEXT_CHROMA_U, uiTrMode);
                m_pcEntropyCoder->encodeCoeffNxN(cu, pcCoeffCurrU, uiAbsPartIdx, trWidthC, trHeightC, uiDepth, TEXT_CHROMA_U);
                uiSingleBitsU = m_pcEntropyCoder->getNumberOfWrittenBits();

                curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCbQpOffset() + cu->getSlice()->getSliceQpDeltaCb();
                m_pcTrQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset);

                Int scalingListType = 3 + g_eTTable[(Int)TEXT_CHROMA_U];
                assert(scalingListType < 6);

                m_pcTrQuant->invtransformNxN(cu->getCUTransquantBypass(uiAbsPartIdx), TEXT_CHROMA, REG_DCT, pcResiCurrU, m_pcQTTempTComYuv[uiQTTempAccessLayer].Cwidth, pcCoeffCurrU, trWidthC, trHeightC, scalingListType, true);

                uiNonzeroDistU = m_pcRdCost->scaleChromaDistCb(primitives.sse_ss[PartC](pcResi->getCbAddr(absTUPartIdxC), pcResi->Cwidth,
                                                                                        m_pcQTTempTComYuv[uiQTTempAccessLayer].getCbAddr(absTUPartIdxC), m_pcQTTempTComYuv[uiQTTempAccessLayer].Cwidth));
                dSingleCostU = m_pcRdCost->calcRdCost(uiNonzeroDistU, uiSingleBitsU);
            }

            if (!uiAbsSumTransformSkipU || minCostU < dSingleCostU)
            {
                cu->setTransformSkipSubParts(0, TEXT_CHROMA_U, uiAbsPartIdx, cu->getDepth(0) + uiTrModeC);

                memcpy(pcCoeffCurrU, bestCoeffU, sizeof(TCoeff) * uiNumSamplesChro);
                memcpy(pcArlCoeffCurrU, bestArlCoeffU, sizeof(TCoeff) * uiNumSamplesChro);
                for (Int i = 0; i < trHeightC; ++i)
                {
                    memcpy(pcResiCurrU + i * resiCStride, &bestResiU[i * trWidthC], sizeof(Short) * trWidthC);
                }
            }
            else
            {
                uiDistU = uiNonzeroDistU;
                uiAbsSumU = uiAbsSumTransformSkipU;
                uiBestTransformMode[1] = 1;
            }

            if (uiAbsSumTransformSkipV)
            {
                m_pcEntropyCoder->encodeQtCbf(cu, uiAbsPartIdx, TEXT_CHROMA_V, uiTrMode);
                m_pcEntropyCoder->encodeCoeffNxN(cu, pcCoeffCurrV, uiAbsPartIdx, trWidthC, trHeightC, uiDepth, TEXT_CHROMA_V);
                uiSingleBitsV = m_pcEntropyCoder->getNumberOfWrittenBits() - uiSingleBitsU;

                curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCrQpOffset() + cu->getSlice()->getSliceQpDeltaCr();
                m_pcTrQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset);

                Int scalingListType = 3 + g_eTTable[(Int)TEXT_CHROMA_V];
                assert(scalingListType < 6);

                m_pcTrQuant->invtransformNxN(cu->getCUTransquantBypass(uiAbsPartIdx), TEXT_CHROMA, REG_DCT, pcResiCurrV, m_pcQTTempTComYuv[uiQTTempAccessLayer].Cwidth, pcCoeffCurrV, trWidthC, trHeightC, scalingListType, true);

                uiNonzeroDistV = m_pcRdCost->scaleChromaDistCr(primitives.sse_ss[PartC](pcResi->getCrAddr(absTUPartIdxC), pcResi->Cwidth,
                                                                                        m_pcQTTempTComYuv[uiQTTempAccessLayer].getCrAddr(absTUPartIdxC), m_pcQTTempTComYuv[uiQTTempAccessLayer].Cwidth));
                dSingleCostV = m_pcRdCost->calcRdCost(uiNonzeroDistV, uiSingleBitsV);
            }

            if (!uiAbsSumTransformSkipV || minCostV < dSingleCostV)
            {
                cu->setTransformSkipSubParts(0, TEXT_CHROMA_V, uiAbsPartIdx, cu->getDepth(0) + uiTrModeC);

                memcpy(pcCoeffCurrV, bestCoeffV, sizeof(TCoeff) * uiNumSamplesChro);
                memcpy(pcArlCoeffCurrV, bestArlCoeffV, sizeof(TCoeff) * uiNumSamplesChro);
                for (Int i = 0; i < trHeightC; ++i)
                {
                    memcpy(pcResiCurrV + i * resiCStride, &bestResiV[i * trWidthC], sizeof(Short) * trWidthC);
                }
            }
            else
            {
                uiDistV = uiNonzeroDistV;
                uiAbsSumV = uiAbsSumTransformSkipV;
                uiBestTransformMode[2] = 1;
            }

            cu->setCbfSubParts(uiAbsSumU ? uiSetCbf : 0, TEXT_CHROMA_U, uiAbsPartIdx, cu->getDepth(0) + uiTrModeC);
            cu->setCbfSubParts(uiAbsSumV ? uiSetCbf : 0, TEXT_CHROMA_V, uiAbsPartIdx, cu->getDepth(0) + uiTrModeC);
        }

        m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_QT_TRAFO_ROOT]);

        m_pcEntropyCoder->resetBits();

        {
            if (uiLog2TrSize > cu->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx))
            {
                m_pcEntropyCoder->encodeTransformSubdivFlag(0, 5 - uiLog2TrSize);
            }
        }

        {
            if (bCodeChroma)
            {
                m_pcEntropyCoder->encodeQtCbf(cu, uiAbsPartIdx, TEXT_CHROMA_U, uiTrMode);
                m_pcEntropyCoder->encodeQtCbf(cu, uiAbsPartIdx, TEXT_CHROMA_V, uiTrMode);
            }

            m_pcEntropyCoder->encodeQtCbf(cu, uiAbsPartIdx, TEXT_LUMA,     uiTrMode);
        }

        m_pcEntropyCoder->encodeCoeffNxN(cu, pcCoeffCurrY, uiAbsPartIdx, trWidth, trHeight,    uiDepth, TEXT_LUMA);

        if (bCodeChroma)
        {
            m_pcEntropyCoder->encodeCoeffNxN(cu, pcCoeffCurrU, uiAbsPartIdx, trWidthC, trHeightC, uiDepth, TEXT_CHROMA_U);
            m_pcEntropyCoder->encodeCoeffNxN(cu, pcCoeffCurrV, uiAbsPartIdx, trWidthC, trHeightC, uiDepth, TEXT_CHROMA_V);
        }

        uiSingleBits = m_pcEntropyCoder->getNumberOfWrittenBits();

        uiSingleDist = uiDistY + uiDistU + uiDistV;
        dSingleCost = m_pcRdCost->calcRdCost(uiSingleDist, uiSingleBits);
    }

    // code sub-blocks
    if (bCheckSplit)
    {
        if (bCheckFull)
        {
            m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiDepth][CI_QT_TRAFO_TEST]);
            m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_QT_TRAFO_ROOT]);
        }
        UInt uiSubdivDist = 0;
        UInt uiSubdivBits = 0;
        UInt64 dSubdivCost = 0.0;

        const UInt uiQPartNumSubdiv = cu->getPic()->getNumPartInCU() >> ((uiDepth + 1) << 1);
        for (UInt ui = 0; ui < 4; ++ui)
        {
            UInt nsAddr = uiAbsPartIdx + ui * uiQPartNumSubdiv;
            xEstimateResidualQT(cu, ui, uiAbsPartIdx + ui * uiQPartNumSubdiv, nsAddr, pcResi, uiDepth + 1, dSubdivCost, uiSubdivBits, uiSubdivDist, bCheckFull ? NULL : puiZeroDist);
        }

        UInt uiYCbf = 0;
        UInt uiUCbf = 0;
        UInt uiVCbf = 0;
        for (UInt ui = 0; ui < 4; ++ui)
        {
            uiYCbf |= cu->getCbf(uiAbsPartIdx + ui * uiQPartNumSubdiv, TEXT_LUMA,     uiTrMode + 1);
            uiUCbf |= cu->getCbf(uiAbsPartIdx + ui * uiQPartNumSubdiv, TEXT_CHROMA_U, uiTrMode + 1);
            uiVCbf |= cu->getCbf(uiAbsPartIdx + ui * uiQPartNumSubdiv, TEXT_CHROMA_V, uiTrMode + 1);
        }

        for (UInt ui = 0; ui < 4 * uiQPartNumSubdiv; ++ui)
        {
            cu->getCbf(TEXT_LUMA)[uiAbsPartIdx + ui] |= uiYCbf << uiTrMode;
            cu->getCbf(TEXT_CHROMA_U)[uiAbsPartIdx + ui] |= uiUCbf << uiTrMode;
            cu->getCbf(TEXT_CHROMA_V)[uiAbsPartIdx + ui] |= uiVCbf << uiTrMode;
        }

        m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_QT_TRAFO_ROOT]);
        m_pcEntropyCoder->resetBits();

        {
            xEncodeResidualQT(cu, uiAbsPartIdx, uiDepth, true,  TEXT_LUMA);
            xEncodeResidualQT(cu, uiAbsPartIdx, uiDepth, false, TEXT_LUMA);
            xEncodeResidualQT(cu, uiAbsPartIdx, uiDepth, false, TEXT_CHROMA_U);
            xEncodeResidualQT(cu, uiAbsPartIdx, uiDepth, false, TEXT_CHROMA_V);
        }

        uiSubdivBits = m_pcEntropyCoder->getNumberOfWrittenBits();
        dSubdivCost  = m_pcRdCost->calcRdCost(uiSubdivDist, uiSubdivBits);

        if (uiYCbf || uiUCbf || uiVCbf || !bCheckFull)
        {
            if (dSubdivCost < dSingleCost)
            {
                rdCost += dSubdivCost;
                outBits += uiSubdivBits;
                ruiDist += uiSubdivDist;
                return;
            }
        }
        cu->setTransformSkipSubParts(uiBestTransformMode[0], TEXT_LUMA, uiAbsPartIdx, uiDepth);
        if (bCodeChroma)
        {
            cu->setTransformSkipSubParts(uiBestTransformMode[1], TEXT_CHROMA_U, uiAbsPartIdx, cu->getDepth(0) + uiTrModeC);
            cu->setTransformSkipSubParts(uiBestTransformMode[2], TEXT_CHROMA_V, uiAbsPartIdx, cu->getDepth(0) + uiTrModeC);
        }
        assert(bCheckFull);
        m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_QT_TRAFO_TEST]);
    }
    rdCost += dSingleCost;
    outBits += uiSingleBits;
    ruiDist += uiSingleDist;

    cu->setTrIdxSubParts(uiTrMode, uiAbsPartIdx, uiDepth);

    cu->setCbfSubParts(uiAbsSumY ? uiSetCbf : 0, TEXT_LUMA, uiAbsPartIdx, uiDepth);
    if (bCodeChroma)
    {
        cu->setCbfSubParts(uiAbsSumU ? uiSetCbf : 0, TEXT_CHROMA_U, uiAbsPartIdx, cu->getDepth(0) + uiTrModeC);
        cu->setCbfSubParts(uiAbsSumV ? uiSetCbf : 0, TEXT_CHROMA_V, uiAbsPartIdx, cu->getDepth(0) + uiTrModeC);
    }
}

Void TEncSearch::xEncodeResidualQT(TComDataCU* cu, UInt uiAbsPartIdx, const UInt uiDepth, Bool bSubdivAndCbf, TextType eType)
{
    assert(cu->getDepth(0) == cu->getDepth(uiAbsPartIdx));
    const UInt uiCurrTrMode = uiDepth - cu->getDepth(0);
    const UInt uiTrMode = cu->getTransformIdx(uiAbsPartIdx);

    const Bool bSubdiv = uiCurrTrMode != uiTrMode;

    const UInt uiLog2TrSize = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> uiDepth] + 2;

    {
        if (bSubdivAndCbf && uiLog2TrSize <= cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() && uiLog2TrSize > cu->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx))
        {
            m_pcEntropyCoder->encodeTransformSubdivFlag(bSubdiv, 5 - uiLog2TrSize);
        }
    }

    {
        assert(cu->getPredictionMode(uiAbsPartIdx) != MODE_INTRA);
        if (bSubdivAndCbf)
        {
            const Bool bFirstCbfOfCU = uiCurrTrMode == 0;
            if (bFirstCbfOfCU || uiLog2TrSize > 2)
            {
                if (bFirstCbfOfCU || cu->getCbf(uiAbsPartIdx, TEXT_CHROMA_U, uiCurrTrMode - 1))
                {
                    m_pcEntropyCoder->encodeQtCbf(cu, uiAbsPartIdx, TEXT_CHROMA_U, uiCurrTrMode);
                }
                if (bFirstCbfOfCU || cu->getCbf(uiAbsPartIdx, TEXT_CHROMA_V, uiCurrTrMode - 1))
                {
                    m_pcEntropyCoder->encodeQtCbf(cu, uiAbsPartIdx, TEXT_CHROMA_V, uiCurrTrMode);
                }
            }
            else if (uiLog2TrSize == 2)
            {
                assert(cu->getCbf(uiAbsPartIdx, TEXT_CHROMA_U, uiCurrTrMode) == cu->getCbf(uiAbsPartIdx, TEXT_CHROMA_U, uiCurrTrMode - 1));
                assert(cu->getCbf(uiAbsPartIdx, TEXT_CHROMA_V, uiCurrTrMode) == cu->getCbf(uiAbsPartIdx, TEXT_CHROMA_V, uiCurrTrMode - 1));
            }
        }
    }

    if (!bSubdiv)
    {
        const UInt uiNumCoeffPerAbsPartIdxIncrement = cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight() >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1);
        //assert( 16 == uiNumCoeffPerAbsPartIdxIncrement ); // check
        const UInt uiQTTempAccessLayer = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;
        TCoeff *pcCoeffCurrY = m_ppcQTTempCoeffY[uiQTTempAccessLayer] +  uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx;
        TCoeff *pcCoeffCurrU = m_ppcQTTempCoeffCb[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx >> 2);
        TCoeff *pcCoeffCurrV = m_ppcQTTempCoeffCr[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx >> 2);

        Bool  bCodeChroma   = true;
        UInt  uiTrModeC     = uiTrMode;
        UInt  uiLog2TrSizeC = uiLog2TrSize - 1;
        if (uiLog2TrSize == 2)
        {
            uiLog2TrSizeC++;
            uiTrModeC--;
            UInt  uiQPDiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + uiTrModeC) << 1);
            bCodeChroma   = ((uiAbsPartIdx % uiQPDiv) == 0);
        }

        if (bSubdivAndCbf)
        {
            {
                m_pcEntropyCoder->encodeQtCbf(cu, uiAbsPartIdx, TEXT_LUMA,     uiTrMode);
            }
        }
        else
        {
            if (eType == TEXT_LUMA     && cu->getCbf(uiAbsPartIdx, TEXT_LUMA,     uiTrMode))
            {
                Int trWidth  = 1 << uiLog2TrSize;
                Int trHeight = 1 << uiLog2TrSize;
                m_pcEntropyCoder->encodeCoeffNxN(cu, pcCoeffCurrY, uiAbsPartIdx, trWidth, trHeight,    uiDepth, TEXT_LUMA);
            }
            if (bCodeChroma)
            {
                Int trWidth  = 1 << uiLog2TrSizeC;
                Int trHeight = 1 << uiLog2TrSizeC;
                if (eType == TEXT_CHROMA_U && cu->getCbf(uiAbsPartIdx, TEXT_CHROMA_U, uiTrMode))
                {
                    m_pcEntropyCoder->encodeCoeffNxN(cu, pcCoeffCurrU, uiAbsPartIdx, trWidth, trHeight, uiDepth, TEXT_CHROMA_U);
                }
                if (eType == TEXT_CHROMA_V && cu->getCbf(uiAbsPartIdx, TEXT_CHROMA_V, uiTrMode))
                {
                    m_pcEntropyCoder->encodeCoeffNxN(cu, pcCoeffCurrV, uiAbsPartIdx, trWidth, trHeight, uiDepth, TEXT_CHROMA_V);
                }
            }
        }
    }
    else
    {
        if (bSubdivAndCbf || cu->getCbf(uiAbsPartIdx, eType, uiCurrTrMode))
        {
            const UInt uiQPartNumSubdiv = cu->getPic()->getNumPartInCU() >> ((uiDepth + 1) << 1);
            for (UInt ui = 0; ui < 4; ++ui)
            {
                xEncodeResidualQT(cu, uiAbsPartIdx + ui * uiQPartNumSubdiv, uiDepth + 1, bSubdivAndCbf, eType);
            }
        }
    }
}

Void TEncSearch::xSetResidualQTData(TComDataCU* cu, UInt uiQuadrant, UInt uiAbsPartIdx, UInt absTUPartIdx, TShortYUV* pcResi, UInt uiDepth, Bool bSpatial)
{
    assert(cu->getDepth(0) == cu->getDepth(uiAbsPartIdx));
    const UInt uiCurrTrMode = uiDepth - cu->getDepth(0);
    const UInt uiTrMode = cu->getTransformIdx(uiAbsPartIdx);

    if (uiCurrTrMode == uiTrMode)
    {
        const UInt uiLog2TrSize = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> uiDepth] + 2;
        const UInt uiQTTempAccessLayer = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;

        Bool  bCodeChroma   = true;
        UInt  uiTrModeC     = uiTrMode;
        UInt  uiLog2TrSizeC = uiLog2TrSize - 1;
        if (uiLog2TrSize == 2)
        {
            uiLog2TrSizeC++;
            uiTrModeC--;
            UInt  uiQPDiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + uiTrModeC) << 1);
            bCodeChroma   = ((uiAbsPartIdx % uiQPDiv) == 0);
        }

        if (bSpatial)
        {
            Int trWidth  = 1 << uiLog2TrSize;
            Int trHeight = 1 << uiLog2TrSize;
            m_pcQTTempTComYuv[uiQTTempAccessLayer].copyPartToPartLuma(pcResi, absTUPartIdx, trWidth, trHeight);

            if (bCodeChroma)
            {
                {
                    m_pcQTTempTComYuv[uiQTTempAccessLayer].copyPartToPartChroma(pcResi, uiAbsPartIdx, 1 << uiLog2TrSizeC, 1 << uiLog2TrSizeC);
                }
            }
        }
        else
        {
            UInt    uiNumCoeffPerAbsPartIdxIncrement = cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight() >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1);
            UInt    uiNumCoeffY = (1 << (uiLog2TrSize << 1));
            TCoeff* pcCoeffSrcY = m_ppcQTTempCoeffY[uiQTTempAccessLayer] +  uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx;
            TCoeff* pcCoeffDstY = cu->getCoeffY() + uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx;
            ::memcpy(pcCoeffDstY, pcCoeffSrcY, sizeof(TCoeff) * uiNumCoeffY);
            Int* pcArlCoeffSrcY = m_ppcQTTempArlCoeffY[uiQTTempAccessLayer] +  uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx;
            Int* pcArlCoeffDstY = cu->getArlCoeffY() + uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx;
            ::memcpy(pcArlCoeffDstY, pcArlCoeffSrcY, sizeof(Int) * uiNumCoeffY);
            if (bCodeChroma)
            {
                UInt    uiNumCoeffC = (1 << (uiLog2TrSizeC << 1));
                TCoeff* pcCoeffSrcU = m_ppcQTTempCoeffCb[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx >> 2);
                TCoeff* pcCoeffSrcV = m_ppcQTTempCoeffCr[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx >> 2);
                TCoeff* pcCoeffDstU = cu->getCoeffCb() + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx >> 2);
                TCoeff* pcCoeffDstV = cu->getCoeffCr() + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx >> 2);
                ::memcpy(pcCoeffDstU, pcCoeffSrcU, sizeof(TCoeff) * uiNumCoeffC);
                ::memcpy(pcCoeffDstV, pcCoeffSrcV, sizeof(TCoeff) * uiNumCoeffC);
                Int* pcArlCoeffSrcU = m_ppcQTTempArlCoeffCb[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx >> 2);
                Int* pcArlCoeffSrcV = m_ppcQTTempArlCoeffCr[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx >> 2);
                Int* pcArlCoeffDstU = cu->getArlCoeffCb() + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx >> 2);
                Int* pcArlCoeffDstV = cu->getArlCoeffCr() + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx >> 2);
                ::memcpy(pcArlCoeffDstU, pcArlCoeffSrcU, sizeof(Int) * uiNumCoeffC);
                ::memcpy(pcArlCoeffDstV, pcArlCoeffSrcV, sizeof(Int) * uiNumCoeffC);
            }
        }
    }
    else
    {
        const UInt uiQPartNumSubdiv = cu->getPic()->getNumPartInCU() >> ((uiDepth + 1) << 1);
        for (UInt ui = 0; ui < 4; ++ui)
        {
            UInt nsAddr = uiAbsPartIdx + ui * uiQPartNumSubdiv;
            xSetResidualQTData(cu, ui, uiAbsPartIdx + ui * uiQPartNumSubdiv, nsAddr, pcResi, uiDepth + 1, bSpatial);
        }
    }
}

UInt TEncSearch::xModeBitsIntra(TComDataCU* cu, UInt uiMode, UInt uiPU, UInt uiPartOffset, UInt uiDepth, UInt uiInitTrDepth)
{
    // Reload only contexts required for coding intra mode information
    m_pcRDGoOnSbacCoder->loadIntraDirModeLuma(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);

    cu->setLumaIntraDirSubParts(uiMode, uiPartOffset, uiDepth + uiInitTrDepth);

    m_pcEntropyCoder->resetBits();
    m_pcEntropyCoder->encodeIntraDirModeLuma(cu, uiPartOffset);

    return m_pcEntropyCoder->getNumberOfWrittenBits();
}

UInt TEncSearch::xUpdateCandList(UInt uiMode, UInt64 uiCost, UInt uiFastCandNum, UInt * CandModeList, UInt64 * CandCostList)
{
    UInt i;
    UInt shift = 0;

    while (shift < uiFastCandNum && uiCost < CandCostList[uiFastCandNum - 1 - shift])
    {
        shift++;
    }

    if (shift != 0)
    {
        for (i = 1; i < shift; i++)
        {
            CandModeList[uiFastCandNum - i] = CandModeList[uiFastCandNum - 1 - i];
            CandCostList[uiFastCandNum - i] = CandCostList[uiFastCandNum - 1 - i];
        }

        CandModeList[uiFastCandNum - shift] = uiMode;
        CandCostList[uiFastCandNum - shift] = uiCost;
        return 1;
    }

    return 0;
}

/** add inter-prediction syntax elements for a CU block
 * \param cu
 * \param uiQp
 * \param uiTrMode
 * \param outBits
 * \param rpcYuvRec
 * \param predYuv
 * \param outResiYuv
 * \returns Void
 */
Void  TEncSearch::xAddSymbolBitsInter(TComDataCU* cu, UInt uiQp, UInt uiTrMode, UInt& outBits, TShortYUV*& rpcYuvRec, TComYuv*predYuv, TShortYUV*& outResiYuv)
{
    if (cu->getMergeFlag(0) && cu->getPartitionSize(0) == SIZE_2Nx2N && !cu->getQtRootCbf(0))
    {
        cu->setSkipFlagSubParts(true, 0, cu->getDepth(0));

        m_pcEntropyCoder->resetBits();
        if (cu->getSlice()->getPPS()->getTransquantBypassEnableFlag())
        {
            m_pcEntropyCoder->encodeCUTransquantBypassFlag(cu, 0, true);
        }
        m_pcEntropyCoder->encodeSkipFlag(cu, 0, true);
        m_pcEntropyCoder->encodeMergeIndex(cu, 0, true);
        outBits += m_pcEntropyCoder->getNumberOfWrittenBits();
    }
    else
    {
        m_pcEntropyCoder->resetBits();
        if (cu->getSlice()->getPPS()->getTransquantBypassEnableFlag())
        {
            m_pcEntropyCoder->encodeCUTransquantBypassFlag(cu, 0, true);
        }
        m_pcEntropyCoder->encodeSkipFlag(cu, 0, true);
        m_pcEntropyCoder->encodePredMode(cu, 0, true);
        m_pcEntropyCoder->encodePartSize(cu, 0, cu->getDepth(0), true);
        m_pcEntropyCoder->encodePredInfo(cu, 0, true);
        Bool bDummy = false;
        m_pcEntropyCoder->encodeCoeff(cu, 0, cu->getDepth(0), cu->getWidth(0), cu->getHeight(0), bDummy);

        outBits += m_pcEntropyCoder->getNumberOfWrittenBits();
    }
}

/**** Function to estimate the header bits ************/
UInt  TEncSearch::estimateHeaderBits(TComDataCU* cu, UInt uiAbsPartIdx)
{
    UInt bits = 0;

    m_pcEntropyCoder->resetBits();

    UInt uiLPelX   = cu->getCUPelX() + g_rasterToPelX[g_zscanToRaster[uiAbsPartIdx]];
    UInt uiRPelX   = uiLPelX + (g_maxCUWidth >> cu->getDepth(0))  - 1;
    UInt uiTPelY   = cu->getCUPelY() + g_rasterToPelY[g_zscanToRaster[uiAbsPartIdx]];
    UInt uiBPelY   = uiTPelY + (g_maxCUHeight >>  cu->getDepth(0)) - 1;

    TComSlice * pcSlice = cu->getPic()->getSlice();
    if ((uiRPelX < pcSlice->getSPS()->getPicWidthInLumaSamples()) && (uiBPelY < pcSlice->getSPS()->getPicHeightInLumaSamples()))
    {
        m_pcEntropyCoder->encodeSplitFlag(cu, uiAbsPartIdx,  cu->getDepth(0));
    }

    if (cu->getMergeFlag(0) && cu->getPartitionSize(0) == SIZE_2Nx2N && !cu->getQtRootCbf(0))
    {
        m_pcEntropyCoder->encodeMergeFlag(cu, 0);
        m_pcEntropyCoder->encodeMergeIndex(cu, 0, true);
    }

    if (cu->getSlice()->getPPS()->getTransquantBypassEnableFlag())
    {
        m_pcEntropyCoder->encodeCUTransquantBypassFlag(cu, 0, true);
    }

    if (!cu->getSlice()->isIntra())
    {
        m_pcEntropyCoder->encodeSkipFlag(cu, 0, true);
    }

    m_pcEntropyCoder->encodePredMode(cu, 0, true);

    m_pcEntropyCoder->encodePartSize(cu, 0, cu->getDepth(0), true);
    bits += m_pcEntropyCoder->getNumberOfWrittenBits();

    return bits;
}

/**
 * \brief Generate half-sample interpolated block
 *
 * \param pattern Reference picture ROI
 * \param biPred    Flag indicating whether block is for biprediction
 */
Void TEncSearch::xExtDIFUpSamplingH(TComPattern* pattern, Bool biPred)
{
    assert(g_bitDepthY == 8);

    Int width      = pattern->getROIYWidth();
    Int height     = pattern->getROIYHeight();
    Int srcStride  = pattern->getPatternLStride();

    Int intStride = filteredBlockTmp[0].width;
    Int dstStride = m_filteredBlock[0][0].getStride();

    Pel *srcPtr;    //Contains raw pixels
    Short *intPtr;  // Intermediate results in short
    Pel *dstPtr;    // Final filtered blocks in Pel

    Int filterSize = NTAPS_LUMA;
    Int halfFilterSize = (filterSize >> 1);

    srcPtr = (Pel*)pattern->getROIY() - halfFilterSize * srcStride - 1;

    dstPtr = m_filteredBlock[0][0].getLumaAddr();

    primitives.blockcpy_pp(width, height, (pixel*)dstPtr, dstStride, (pixel*)(srcPtr + halfFilterSize * srcStride + 1), srcStride);

    intPtr = filteredBlockTmp[0].getLumaAddr();
    primitives.ipfilter_p2s(g_bitDepthY, (pixel*)srcPtr, srcStride, intPtr,
                            intStride, width + 1, height + filterSize);

    intPtr = filteredBlockTmp[0].getLumaAddr() + (halfFilterSize - 1) * intStride + 1;
    dstPtr = m_filteredBlock[2][0].getLumaAddr();
    primitives.ipfilter_sp[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr,
                                           dstStride, width, height + 1, m_lumaFilter[2]);

    intPtr = filteredBlockTmp[2].getLumaAddr();
    primitives.ipfilter_ps[FILTER_H_P_S_8](g_bitDepthY, (pixel*)srcPtr, srcStride, intPtr, intStride, width + 1, height + filterSize,  m_lumaFilter[2]);

    intPtr = filteredBlockTmp[2].getLumaAddr() + halfFilterSize * intStride;
    dstPtr = m_filteredBlock[0][2].getLumaAddr();
    primitives.ipfilter_s2p(g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, dstStride, width + 1, height + 0);

    intPtr = filteredBlockTmp[2].getLumaAddr() + (halfFilterSize - 1) * intStride;
    dstPtr = m_filteredBlock[2][2].getLumaAddr();
    primitives.ipfilter_sp[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, dstStride, width + 1, height + 1, m_lumaFilter[2]);
}

/**
 * \brief Generate quarter-sample interpolated blocks
 *
 * \param pattern    Reference picture ROI
 * \param halfPelRef Half-pel mv
 * \param biPred     Flag indicating whether block is for biprediction
 */
Void TEncSearch::xExtDIFUpSamplingQ(TComPattern* pattern, MV halfPelRef, Bool biPred)
{
    assert(g_bitDepthY == 8);

    Int width      = pattern->getROIYWidth();
    Int height     = pattern->getROIYHeight();
    Int srcStride  = pattern->getPatternLStride();

    Int intStride = filteredBlockTmp[0].width;
    Int dstStride = m_filteredBlock[0][0].getStride();

    Pel *srcPtr;    //Contains raw pixels
    Short *intPtr;  // Intermediate results in short
    Pel *dstPtr;    // Final filtered blocks in Pel

    Int filterSize = NTAPS_LUMA;

    Int halfFilterSize = (filterSize >> 1);

    Int extHeight = (halfPelRef.y == 0) ? height + filterSize : height + filterSize - 1;

    // Horizontal filter 1/4
    srcPtr = pattern->getROIY() - halfFilterSize * srcStride - 1;
    intPtr = filteredBlockTmp[1].getLumaAddr();
    if (halfPelRef.y > 0)
    {
        srcPtr += srcStride;
    }
    if (halfPelRef.x >= 0)
    {
        srcPtr += 1;
    }
    primitives.ipfilter_ps[FILTER_H_P_S_8](g_bitDepthY, (pixel*)srcPtr, srcStride, intPtr, intStride, width, extHeight, m_lumaFilter[1]);

    // Horizontal filter 3/4
    srcPtr = pattern->getROIY() - halfFilterSize * srcStride - 1;
    intPtr = filteredBlockTmp[3].getLumaAddr();
    if (halfPelRef.y > 0)
    {
        srcPtr += srcStride;
    }
    if (halfPelRef.x > 0)
    {
        srcPtr += 1;
    }
    primitives.ipfilter_ps[FILTER_H_P_S_8](g_bitDepthY, (pixel*)srcPtr, srcStride, intPtr, intStride, width, extHeight, m_lumaFilter[3]);

    // Generate @ 1,1
    intPtr = filteredBlockTmp[1].getLumaAddr() + (halfFilterSize - 1) * intStride;
    dstPtr = m_filteredBlock[1][1].getLumaAddr();
    if (halfPelRef.y == 0)
    {
        intPtr += intStride;
    }
    primitives.ipfilter_sp[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, dstStride, width, height, m_lumaFilter[1]);

    // Generate @ 3,1
    intPtr = filteredBlockTmp[1].getLumaAddr() + (halfFilterSize - 1) * intStride;
    dstPtr = m_filteredBlock[3][1].getLumaAddr();
    primitives.ipfilter_sp[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, dstStride, width, height, m_lumaFilter[3]);

    if (halfPelRef.y != 0)
    {
        // Generate @ 2,1
        intPtr = filteredBlockTmp[1].getLumaAddr() + (halfFilterSize - 1) * intStride;
        dstPtr = m_filteredBlock[2][1].getLumaAddr();
        if (halfPelRef.y == 0)
        {
            intPtr += intStride;
        }
        primitives.ipfilter_sp[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, dstStride, width, height, m_lumaFilter[2]);

        // Generate @ 2,3
        intPtr = filteredBlockTmp[3].getLumaAddr() + (halfFilterSize - 1) * intStride;
        dstPtr = m_filteredBlock[2][3].getLumaAddr();
        if (halfPelRef.y == 0)
        {
            intPtr += intStride;
        }
        primitives.ipfilter_sp[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, dstStride, width, height, m_lumaFilter[2]);
    }
    else
    {
        // Generate @ 0,1
        intPtr = filteredBlockTmp[1].getLumaAddr() + halfFilterSize * intStride;
        dstPtr = m_filteredBlock[0][1].getLumaAddr();
        primitives.ipfilter_s2p(g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, dstStride, width, height);

        // Generate @ 0,3
        intPtr = filteredBlockTmp[3].getLumaAddr() + halfFilterSize * intStride;
        dstPtr = m_filteredBlock[0][3].getLumaAddr();
        primitives.ipfilter_s2p(g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, dstStride, width, height);
    }

    if (halfPelRef.x != 0)
    {
        // Generate @ 1,2
        intPtr = filteredBlockTmp[2].getLumaAddr() + (halfFilterSize - 1) * intStride;
        dstPtr = m_filteredBlock[1][2].getLumaAddr();
        if (halfPelRef.x > 0)
        {
            intPtr += 1;
        }
        if (halfPelRef.y >= 0)
        {
            intPtr += intStride;
        }

        primitives.ipfilter_sp[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, dstStride, width, height, m_lumaFilter[1]);

        // Generate @ 3,2
        intPtr = filteredBlockTmp[2].getLumaAddr() + (halfFilterSize - 1) * intStride;
        dstPtr = m_filteredBlock[3][2].getLumaAddr();
        if (halfPelRef.x > 0)
        {
            intPtr += 1;
        }
        if (halfPelRef.y > 0)
        {
            intPtr += intStride;
        }
        primitives.ipfilter_sp[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, dstStride, width, height, m_lumaFilter[3]);
    }
    else
    {
        // Generate @ 1,0
        intPtr = filteredBlockTmp[0].getLumaAddr() + (halfFilterSize - 1) * intStride + 1;
        dstPtr = m_filteredBlock[1][0].getLumaAddr();
        if (halfPelRef.y >= 0)
        {
            intPtr += intStride;
        }
        primitives.ipfilter_sp[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, dstStride, width, height, m_lumaFilter[1]);

        // Generate @ 3,0
        intPtr = filteredBlockTmp[0].getLumaAddr() + (halfFilterSize - 1) * intStride + 1;
        dstPtr = (Pel*)m_filteredBlock[3][0].getLumaAddr();
        if (halfPelRef.y > 0)
        {
            intPtr += intStride;
        }
        primitives.ipfilter_sp[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, dstStride, width, height, m_lumaFilter[3]);
    }

    // Generate @ 1,3
    intPtr = filteredBlockTmp[3].getLumaAddr() + (halfFilterSize - 1) * intStride;
    dstPtr = m_filteredBlock[1][3].getLumaAddr();
    if (halfPelRef.y == 0)
    {
        intPtr += intStride;
    }
    primitives.ipfilter_sp[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, dstStride, width, height, m_lumaFilter[1]);

    // Generate @ 3,3
    intPtr = filteredBlockTmp[3].getLumaAddr() + (halfFilterSize - 1) * intStride;
    dstPtr = m_filteredBlock[3][3].getLumaAddr();
    primitives.ipfilter_sp[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, dstStride, width, height, m_lumaFilter[3]);
}

/** set wp tables
 * \param TComDataCU* cu
 * \param refIfx
 * \param eRefPicListCur
 * \returns Void
 */
Void  TEncSearch::setWpScalingDistParam(TComDataCU* cu, Int refIfx, RefPicList eRefPicListCur)
{
    if (refIfx < 0)
    {
        m_cDistParam.bApplyWeight = false;
        return;
    }

    TComSlice       *pcSlice  = cu->getSlice();
    TComPPS         *pps      = cu->getSlice()->getPPS();
    wpScalingParam  *wp0, *wp1;
    m_cDistParam.bApplyWeight = (pcSlice->getSliceType() == P_SLICE && pps->getUseWP()) || (pcSlice->getSliceType() == B_SLICE && pps->getWPBiPred());
    if (!m_cDistParam.bApplyWeight) return;

    Int iRefIdx0 = (eRefPicListCur == REF_PIC_LIST_0) ? refIfx : (-1);
    Int iRefIdx1 = (eRefPicListCur == REF_PIC_LIST_1) ? refIfx : (-1);

    getWpScaling(cu, iRefIdx0, iRefIdx1, wp0, wp1);

    if (iRefIdx0 < 0) wp0 = NULL;
    if (iRefIdx1 < 0) wp1 = NULL;

    m_cDistParam.wpCur  = NULL;

    if (eRefPicListCur == REF_PIC_LIST_0)
    {
        m_cDistParam.wpCur = wp0;
    }
    else
    {
        m_cDistParam.wpCur = wp1;
    }
}

//! \}
