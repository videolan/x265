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
Double    meCost;
#endif
//! \ingroup TLibEncoder
//! \{

static const TComMv s_acMvRefineH[9] =
{
    TComMv(0,  0),  // 0
    TComMv(0, -1),  // 1
    TComMv(0,  1),  // 2
    TComMv(-1,  0), // 3
    TComMv(1,  0),  // 4
    TComMv(-1, -1), // 5
    TComMv(1, -1),  // 6
    TComMv(-1,  1), // 7
    TComMv(1,  1)   // 8
};

static const TComMv s_acMvRefineQ[9] =
{
    TComMv(0,  0),  // 0
    TComMv(0, -1),  // 1
    TComMv(0,  1),  // 2
    TComMv(-1, -1), // 5
    TComMv(1, -1),  // 6
    TComMv(-1,  0), // 3
    TComMv(1,  0),  // 4
    TComMv(-1,  1), // 7
    TComMv(1,  1)   // 8
};

static const UInt s_auiDFilter[9] =
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

Void TEncSearch::init(TEncCfg* pcEncCfg, TComRdCost* pcRdCost)
{
    m_pcEncCfg             = pcEncCfg;
    m_pcTrQuant            = NULL;
    m_iSearchRange         = pcEncCfg->getSearchRange();
    m_bipredSearchRange    = pcEncCfg->getBipredSearchRange();
    m_iSearchMethod        = pcEncCfg->getSearchMethod();
    m_pcEntropyCoder       = NULL;
    m_pcRdCost             = pcRdCost;

    m_me.setSearchMethod(m_iSearchMethod);

    m_pppcRDSbacCoder     = NULL;
    m_pcRDGoOnSbacCoder   = NULL;

    for (Int iDir = 0; iDir < 2; iDir++)
    {
        for (Int iRefIdx = 0; iRefIdx < 33; iRefIdx++)
        {
            m_aaiAdaptSR[iDir][iRefIdx] = m_iSearchRange;
        }
    }

    m_puiDFilter = s_auiDFilter + 4;

    // initialize motion cost
    for (Int iNum = 0; iNum < AMVP_MAX_NUM_CANDS + 1; iNum++)
    {
        for (Int iIdx = 0; iIdx < AMVP_MAX_NUM_CANDS; iIdx++)
        {
            if (iIdx < iNum)
                m_auiMVPIdxCost[iIdx][iNum] = xGetMvpIdxBits(iIdx, iNum);
            else
                m_auiMVPIdxCost[iIdx][iNum] = MAX_INT;
        }
    }

    initTempBuff();

    m_pTempPel = new Pel[g_uiMaxCUWidth * g_uiMaxCUHeight];

    const UInt uiNumLayersToAllocate = pcEncCfg->getQuadtreeTULog2MaxSize() - pcEncCfg->getQuadtreeTULog2MinSize() + 1;
    m_ppcQTTempCoeffY  = new TCoeff*[uiNumLayersToAllocate];
    m_ppcQTTempCoeffCb = new TCoeff*[uiNumLayersToAllocate];
    m_ppcQTTempCoeffCr = new TCoeff*[uiNumLayersToAllocate];
    m_pcQTTempCoeffY   = new TCoeff[g_uiMaxCUWidth * g_uiMaxCUHeight];
    m_pcQTTempCoeffCb  = new TCoeff[g_uiMaxCUWidth * g_uiMaxCUHeight >> 2];
    m_pcQTTempCoeffCr  = new TCoeff[g_uiMaxCUWidth * g_uiMaxCUHeight >> 2];
    m_ppcQTTempArlCoeffY  = new Int*[uiNumLayersToAllocate];
    m_ppcQTTempArlCoeffCb = new Int*[uiNumLayersToAllocate];
    m_ppcQTTempArlCoeffCr = new Int*[uiNumLayersToAllocate];
    m_pcQTTempArlCoeffY   = new Int[g_uiMaxCUWidth * g_uiMaxCUHeight];
    m_pcQTTempArlCoeffCb  = new Int[g_uiMaxCUWidth * g_uiMaxCUHeight >> 2];
    m_pcQTTempArlCoeffCr  = new Int[g_uiMaxCUWidth * g_uiMaxCUHeight >> 2];

    const UInt uiNumPartitions = 1 << (g_uiMaxCUDepth << 1);
    m_puhQTTempTrIdx   = new UChar[uiNumPartitions];
    m_puhQTTempCbf[0]  = new UChar[uiNumPartitions];
    m_puhQTTempCbf[1]  = new UChar[uiNumPartitions];
    m_puhQTTempCbf[2]  = new UChar[uiNumPartitions];
    m_pcQTTempTComYuv  = new TShortYUV[uiNumLayersToAllocate];
    for (UInt ui = 0; ui < uiNumLayersToAllocate; ++ui)
    {
        m_ppcQTTempCoeffY[ui]  = new TCoeff[g_uiMaxCUWidth * g_uiMaxCUHeight];
        m_ppcQTTempCoeffCb[ui] = new TCoeff[g_uiMaxCUWidth * g_uiMaxCUHeight >> 2];
        m_ppcQTTempCoeffCr[ui] = new TCoeff[g_uiMaxCUWidth * g_uiMaxCUHeight >> 2];
        m_ppcQTTempArlCoeffY[ui]  = new Int[g_uiMaxCUWidth * g_uiMaxCUHeight];
        m_ppcQTTempArlCoeffCb[ui] = new Int[g_uiMaxCUWidth * g_uiMaxCUHeight >> 2];
        m_ppcQTTempArlCoeffCr[ui] = new Int[g_uiMaxCUWidth * g_uiMaxCUHeight >> 2];
        m_pcQTTempTComYuv[ui].create(g_uiMaxCUWidth, g_uiMaxCUHeight);
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
    m_pcQTTempTransformSkipTComYuv.create(g_uiMaxCUWidth, g_uiMaxCUHeight);

    m_puhQTTempTransformSkipFlag[0] = new UChar[uiNumPartitions];
    m_puhQTTempTransformSkipFlag[1] = new UChar[uiNumPartitions];
    m_puhQTTempTransformSkipFlag[2] = new UChar[uiNumPartitions];
    m_tmpYuvPred.create(MAX_CU_SIZE, MAX_CU_SIZE);
}

#if FASTME_SMOOTHER_MV
#define FIRSTSEARCHSTOP     1
#else
#define FIRSTSEARCHSTOP     0
#endif

#define TZ_SEARCH_CONFIGURATION                                                                                 \
    const Int  iRaster                  = 5; /* TZ soll von aussen ?ergeben werden */                            \
    const Bool bTestOtherPredictedMV    = 0;                                                                      \
    const Bool bTestZeroVector          = 1;                                                                      \
    const Bool bTestZeroVectorStart     = 0;                                                                      \
    const Bool bTestZeroVectorStop      = 0;                                                                      \
    const Bool bFirstSearchStop         = FIRSTSEARCHSTOP;                                                        \
    const UInt uiFirstSearchRounds      = 3; /* first search stop X rounds after best match (must be >=1) */     \
    const Bool bEnableRasterSearch      = 1;                                                                      \
    const Bool bAlwaysRasterSearch      = 0; /* ===== 1: BETTER but factor 2 slower ===== */                     \
    const Bool bRasterRefinementEnable  = 0; /* enable either raster refinement or star refinement */            \
    const Bool bStarRefinementEnable    = 1; /* enable either star refinement or raster refinement */            \
    const Bool bStarRefinementStop      = 0;                                                                      \
    const UInt uiStarRefinementRounds   = 2; /* star refinement stop X rounds after best match (must be >=1) */  \


__inline Void TEncSearch::xTZSearchHelp(TComPattern* pcPatternKey, IntTZSearchStruct& rcStruct, const Int iSearchX, const Int iSearchY, const UChar ucPointNr, const UInt uiDistance)
{
    UInt  uiSad;

    Pel*  piRefSrch = rcStruct.piRefY + iSearchY * rcStruct.iYStride + iSearchX;
    
    m_pcRdCost->setDistParam(pcPatternKey, piRefSrch, rcStruct.iYStride, m_cDistParam);

    if (m_cDistParam.iRows > 12)
    {
        // fast encoder decision: use subsampled SAD when rows > 12 for integer ME
        m_cDistParam.iSubShift = 1;
    }
    setDistParamComp(0);

    // distortion
    m_cDistParam.bitDepth = g_bitDepthY;
    uiSad = m_cDistParam.DistFunc(&m_cDistParam);
    
    // motion cost
    uiSad += m_bc.mvcost(x265::MV(iSearchX, iSearchY) << m_pcRdCost->m_iCostScale);

    if (uiSad < rcStruct.uiBestSad)
    {
        rcStruct.uiBestSad      = uiSad;
        rcStruct.iBestX         = iSearchX;
        rcStruct.iBestY         = iSearchY;
        rcStruct.uiBestDistance = uiDistance;
        rcStruct.uiBestRound    = 0;
        rcStruct.ucPointNr      = ucPointNr;
    }
}

__inline Void TEncSearch::xTZ2PointSearch(TComPattern* pcPatternKey, IntTZSearchStruct& rcStruct, TComMv* pcMvSrchRngLT, TComMv* pcMvSrchRngRB)
{
    Int   iSrchRngHorLeft   = pcMvSrchRngLT->getHor();
    Int   iSrchRngHorRight  = pcMvSrchRngRB->getHor();
    Int   iSrchRngVerTop    = pcMvSrchRngLT->getVer();
    Int   iSrchRngVerBottom = pcMvSrchRngRB->getVer();

    // 2 point search,                   //   1 2 3
    // check only the 2 untested points  //   4 0 5
    // around the start point            //   6 7 8
    Int iStartX = rcStruct.iBestX;
    Int iStartY = rcStruct.iBestY;

    switch (rcStruct.ucPointNr)
    {
    case 1:
    {
        if ((iStartX - 1) >= iSrchRngHorLeft)
        {
            xTZSearchHelp(pcPatternKey, rcStruct, iStartX - 1, iStartY, 0, 2);
        }
        if ((iStartY - 1) >= iSrchRngVerTop)
        {
            xTZSearchHelp(pcPatternKey, rcStruct, iStartX, iStartY - 1, 0, 2);
        }
    }
    break;
    case 2:
    {
        if ((iStartY - 1) >= iSrchRngVerTop)
        {
            if ((iStartX - 1) >= iSrchRngHorLeft)
            {
                xTZSearchHelp(pcPatternKey, rcStruct, iStartX - 1, iStartY - 1, 0, 2);
            }
            if ((iStartX + 1) <= iSrchRngHorRight)
            {
                xTZSearchHelp(pcPatternKey, rcStruct, iStartX + 1, iStartY - 1, 0, 2);
            }
        }
    }
    break;
    case 3:
    {
        if ((iStartY - 1) >= iSrchRngVerTop)
        {
            xTZSearchHelp(pcPatternKey, rcStruct, iStartX, iStartY - 1, 0, 2);
        }
        if ((iStartX + 1) <= iSrchRngHorRight)
        {
            xTZSearchHelp(pcPatternKey, rcStruct, iStartX + 1, iStartY, 0, 2);
        }
    }
    break;
    case 4:
    {
        if ((iStartX - 1) >= iSrchRngHorLeft)
        {
            if ((iStartY + 1) <= iSrchRngVerBottom)
            {
                xTZSearchHelp(pcPatternKey, rcStruct, iStartX - 1, iStartY + 1, 0, 2);
            }
            if ((iStartY - 1) >= iSrchRngVerTop)
            {
                xTZSearchHelp(pcPatternKey, rcStruct, iStartX - 1, iStartY - 1, 0, 2);
            }
        }
    }
    break;
    case 5:
    {
        if ((iStartX + 1) <= iSrchRngHorRight)
        {
            if ((iStartY - 1) >= iSrchRngVerTop)
            {
                xTZSearchHelp(pcPatternKey, rcStruct, iStartX + 1, iStartY - 1, 0, 2);
            }
            if ((iStartY + 1) <= iSrchRngVerBottom)
            {
                xTZSearchHelp(pcPatternKey, rcStruct, iStartX + 1, iStartY + 1, 0, 2);
            }
        }
    }
    break;
    case 6:
    {
        if ((iStartX - 1) >= iSrchRngHorLeft)
        {
            xTZSearchHelp(pcPatternKey, rcStruct, iStartX - 1, iStartY, 0, 2);
        }
        if ((iStartY + 1) <= iSrchRngVerBottom)
        {
            xTZSearchHelp(pcPatternKey, rcStruct, iStartX, iStartY + 1, 0, 2);
        }
    }
    break;
    case 7:
    {
        if ((iStartY + 1) <= iSrchRngVerBottom)
        {
            if ((iStartX - 1) >= iSrchRngHorLeft)
            {
                xTZSearchHelp(pcPatternKey, rcStruct, iStartX - 1, iStartY + 1, 0, 2);
            }
            if ((iStartX + 1) <= iSrchRngHorRight)
            {
                xTZSearchHelp(pcPatternKey, rcStruct, iStartX + 1, iStartY + 1, 0, 2);
            }
        }
    }
    break;
    case 8:
    {
        if ((iStartX + 1) <= iSrchRngHorRight)
        {
            xTZSearchHelp(pcPatternKey, rcStruct, iStartX + 1, iStartY, 0, 2);
        }
        if ((iStartY + 1) <= iSrchRngVerBottom)
        {
            xTZSearchHelp(pcPatternKey, rcStruct, iStartX, iStartY + 1, 0, 2);
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

__inline Void TEncSearch::xTZ8PointDiamondSearch(TComPattern* pcPatternKey, IntTZSearchStruct& rcStruct, TComMv* pcMvSrchRngLT, TComMv* pcMvSrchRngRB, const Int iStartX, const Int iStartY, const Int iDist)
{
    Int   iSrchRngHorLeft   = pcMvSrchRngLT->getHor();
    Int   iSrchRngHorRight  = pcMvSrchRngRB->getHor();
    Int   iSrchRngVerTop    = pcMvSrchRngLT->getVer();
    Int   iSrchRngVerBottom = pcMvSrchRngRB->getVer();

    assert(iDist != 0);
    const Int iTop        = iStartY - iDist;
    const Int iBottom     = iStartY + iDist;
    const Int iLeft       = iStartX - iDist;
    const Int iRight      = iStartX + iDist;
    rcStruct.uiBestRound += 1;

    if (iDist == 1) // iDist == 1
    {
        if (iTop >= iSrchRngVerTop) // check top
        {
            xTZSearchHelp(pcPatternKey, rcStruct, iStartX, iTop, 2, iDist);
        }
        if (iLeft >= iSrchRngHorLeft) // check middle left
        {
            xTZSearchHelp(pcPatternKey, rcStruct, iLeft, iStartY, 4, iDist);
        }
        if (iRight <= iSrchRngHorRight) // check middle right
        {
            xTZSearchHelp(pcPatternKey, rcStruct, iRight, iStartY, 5, iDist);
        }
        if (iBottom <= iSrchRngVerBottom) // check bottom
        {
            xTZSearchHelp(pcPatternKey, rcStruct, iStartX, iBottom, 7, iDist);
        }
    }
    else if (iDist <= 8)
    {
        const Int iTop_2      = iStartY - (iDist >> 1);
        const Int iBottom_2   = iStartY + (iDist >> 1);
        const Int iLeft_2     = iStartX - (iDist >> 1);
        const Int iRight_2    = iStartX + (iDist >> 1);

        if (iTop >= iSrchRngVerTop && iLeft >= iSrchRngHorLeft &&
            iRight <= iSrchRngHorRight && iBottom <= iSrchRngVerBottom) // check border
        {
            xTZSearchHelp(pcPatternKey, rcStruct, iStartX,  iTop,      2, iDist);
            xTZSearchHelp(pcPatternKey, rcStruct, iLeft_2,  iTop_2,    1, iDist >> 1);
            xTZSearchHelp(pcPatternKey, rcStruct, iRight_2, iTop_2,    3, iDist >> 1);
            xTZSearchHelp(pcPatternKey, rcStruct, iLeft,    iStartY,   4, iDist);
            xTZSearchHelp(pcPatternKey, rcStruct, iRight,   iStartY,   5, iDist);
            xTZSearchHelp(pcPatternKey, rcStruct, iLeft_2,  iBottom_2, 6, iDist >> 1);
            xTZSearchHelp(pcPatternKey, rcStruct, iRight_2, iBottom_2, 8, iDist >> 1);
            xTZSearchHelp(pcPatternKey, rcStruct, iStartX,  iBottom,   7, iDist);
        }
        else // check border for each mv
        {
            if (iTop >= iSrchRngVerTop) // check top
            {
                xTZSearchHelp(pcPatternKey, rcStruct, iStartX, iTop, 2, iDist);
            }
            if (iTop_2 >= iSrchRngVerTop) // check half top
            {
                if (iLeft_2 >= iSrchRngHorLeft) // check half left
                {
                    xTZSearchHelp(pcPatternKey, rcStruct, iLeft_2, iTop_2, 1, (iDist >> 1));
                }
                if (iRight_2 <= iSrchRngHorRight) // check half right
                {
                    xTZSearchHelp(pcPatternKey, rcStruct, iRight_2, iTop_2, 3, (iDist >> 1));
                }
            } // check half top
            if (iLeft >= iSrchRngHorLeft) // check left
            {
                xTZSearchHelp(pcPatternKey, rcStruct, iLeft, iStartY, 4, iDist);
            }
            if (iRight <= iSrchRngHorRight) // check right
            {
                xTZSearchHelp(pcPatternKey, rcStruct, iRight, iStartY, 5, iDist);
            }
            if (iBottom_2 <= iSrchRngVerBottom) // check half bottom
            {
                if (iLeft_2 >= iSrchRngHorLeft) // check half left
                {
                    xTZSearchHelp(pcPatternKey, rcStruct, iLeft_2, iBottom_2, 6, (iDist >> 1));
                }
                if (iRight_2 <= iSrchRngHorRight) // check half right
                {
                    xTZSearchHelp(pcPatternKey, rcStruct, iRight_2, iBottom_2, 8, (iDist >> 1));
                }
            } // check half bottom
            if (iBottom <= iSrchRngVerBottom) // check bottom
            {
                xTZSearchHelp(pcPatternKey, rcStruct, iStartX, iBottom, 7, iDist);
            }
        } // check border for each mv
    }
    else // iDist > 8
    {
        if (iTop >= iSrchRngVerTop && iLeft >= iSrchRngHorLeft &&
            iRight <= iSrchRngHorRight && iBottom <= iSrchRngVerBottom) // check border
        {
            xTZSearchHelp(pcPatternKey, rcStruct, iStartX, iTop,    0, iDist);
            xTZSearchHelp(pcPatternKey, rcStruct, iLeft,   iStartY, 0, iDist);
            xTZSearchHelp(pcPatternKey, rcStruct, iRight,  iStartY, 0, iDist);
            xTZSearchHelp(pcPatternKey, rcStruct, iStartX, iBottom, 0, iDist);
            for (Int index = 1; index < 4; index++)
            {
                Int iPosYT = iTop    + ((iDist >> 2) * index);
                Int iPosYB = iBottom - ((iDist >> 2) * index);
                Int iPosXL = iStartX - ((iDist >> 2) * index);
                Int iPosXR = iStartX + ((iDist >> 2) * index);
                xTZSearchHelp(pcPatternKey, rcStruct, iPosXL, iPosYT, 0, iDist);
                xTZSearchHelp(pcPatternKey, rcStruct, iPosXR, iPosYT, 0, iDist);
                xTZSearchHelp(pcPatternKey, rcStruct, iPosXL, iPosYB, 0, iDist);
                xTZSearchHelp(pcPatternKey, rcStruct, iPosXR, iPosYB, 0, iDist);
            }
        }
        else // check border for each mv
        {
            if (iTop >= iSrchRngVerTop) // check top
            {
                xTZSearchHelp(pcPatternKey, rcStruct, iStartX, iTop, 0, iDist);
            }
            if (iLeft >= iSrchRngHorLeft) // check left
            {
                xTZSearchHelp(pcPatternKey, rcStruct, iLeft, iStartY, 0, iDist);
            }
            if (iRight <= iSrchRngHorRight) // check right
            {
                xTZSearchHelp(pcPatternKey, rcStruct, iRight, iStartY, 0, iDist);
            }
            if (iBottom <= iSrchRngVerBottom) // check bottom
            {
                xTZSearchHelp(pcPatternKey, rcStruct, iStartX, iBottom, 0, iDist);
            }
            for (Int index = 1; index < 4; index++)
            {
                Int iPosYT = iTop    + ((iDist >> 2) * index);
                Int iPosYB = iBottom - ((iDist >> 2) * index);
                Int iPosXL = iStartX - ((iDist >> 2) * index);
                Int iPosXR = iStartX + ((iDist >> 2) * index);

                if (iPosYT >= iSrchRngVerTop) // check top
                {
                    if (iPosXL >= iSrchRngHorLeft) // check left
                    {
                        xTZSearchHelp(pcPatternKey, rcStruct, iPosXL, iPosYT, 0, iDist);
                    }
                    if (iPosXR <= iSrchRngHorRight) // check right
                    {
                        xTZSearchHelp(pcPatternKey, rcStruct, iPosXR, iPosYT, 0, iDist);
                    }
                } // check top
                if (iPosYB <= iSrchRngVerBottom) // check bottom
                {
                    if (iPosXL >= iSrchRngHorLeft) // check left
                    {
                        xTZSearchHelp(pcPatternKey, rcStruct, iPosXL, iPosYB, 0, iDist);
                    }
                    if (iPosXR <= iSrchRngHorRight) // check right
                    {
                        xTZSearchHelp(pcPatternKey, rcStruct, iPosXR, iPosYB, 0, iDist);
                    }
                } // check bottom
            } // for ...
        } // check border for each mv
    } // iDist > 8
}

//<--

UInt TEncSearch::xPatternRefinement(TComPattern* pcPatternKey,
                                    TComMv baseRefMv,
                                    Int iFrac, TComMv& rcMvFrac, TComPicYuv* refPic, Int offset, TComDataCU* pcCU, UInt uiPartAddr)
{
    UInt  uiDist;
    UInt  uiDistBest  = MAX_UINT;
    UInt  uiDirecBest = 0;
    Pel*  piRefPos;
    Int iRefStride = refPic->getStride();

    m_pcRdCost->setDistParam(pcPatternKey, refPic->getLumaFilterBlock(0, 0, pcCU->getAddr(), pcCU->getZorderIdxInCU() + uiPartAddr) + offset, iRefStride, 1, m_cDistParam, m_pcEncCfg->getUseHADME());

    const TComMv* pcMvRefine = (iFrac == 2 ? s_acMvRefineH : s_acMvRefineQ);

    for (UInt i = 0; i < 9; i++)
    {
        TComMv cMvTest = pcMvRefine[i];
        cMvTest += baseRefMv;

        Int horVal = cMvTest.getHor() * iFrac;
        Int verVal = cMvTest.getVer() * iFrac;
        piRefPos =  refPic->getLumaFilterBlock(verVal & 3, horVal & 3, pcCU->getAddr(), pcCU->getZorderIdxInCU() + uiPartAddr) + offset;
        if (horVal < 0)
            piRefPos -= 1;
        if (verVal < 0)
        {
            piRefPos -= iRefStride;
        }

        cMvTest = pcMvRefine[i];
        cMvTest += rcMvFrac;

        setDistParamComp(0); // Y component

        m_cDistParam.pCur = piRefPos;
        m_cDistParam.bitDepth = g_bitDepthY;
        uiDist = m_cDistParam.DistFunc(&m_cDistParam);
        uiDist += m_bc.mvcost(cMvTest * iFrac);

        if (uiDist < uiDistBest)
        {
            uiDistBest  = uiDist;
            uiDirecBest = i;
        }
    }

    rcMvFrac = pcMvRefine[uiDirecBest];

    return uiDistBest;
}

Void TEncSearch::xEncSubdivCbfQT(TComDataCU* pcCU,
                                 UInt        uiTrDepth,
                                 UInt        uiAbsPartIdx,
                                 Bool        bLuma,
                                 Bool        bChroma)
{
    UInt  uiFullDepth     = pcCU->getDepth(0) + uiTrDepth;
    UInt  uiTrMode        = pcCU->getTransformIdx(uiAbsPartIdx);
    UInt  uiSubdiv        = (uiTrMode > uiTrDepth ? 1 : 0);
    UInt  uiLog2TrafoSize = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth()] + 2 - uiFullDepth;

    if (pcCU->getPredictionMode(0) == MODE_INTRA && pcCU->getPartitionSize(0) == SIZE_NxN && uiTrDepth == 0)
    {
        assert(uiSubdiv);
    }
    else if (uiLog2TrafoSize > pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize())
    {
        assert(uiSubdiv);
    }
    else if (uiLog2TrafoSize == pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize())
    {
        assert(!uiSubdiv);
    }
    else if (uiLog2TrafoSize == pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx))
    {
        assert(!uiSubdiv);
    }
    else
    {
        assert(uiLog2TrafoSize > pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx));
        if (bLuma)
        {
            m_pcEntropyCoder->encodeTransformSubdivFlag(uiSubdiv, 5 - uiLog2TrafoSize);
        }
    }

    if (bChroma)
    {
        if (uiLog2TrafoSize > 2)
        {
            if (uiTrDepth == 0 || pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepth - 1))
                m_pcEntropyCoder->encodeQtCbf(pcCU, uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepth);
            if (uiTrDepth == 0 || pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepth - 1))
                m_pcEntropyCoder->encodeQtCbf(pcCU, uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepth);
        }
    }

    if (uiSubdiv)
    {
        UInt uiQPartNum = pcCU->getPic()->getNumPartInCU() >> ((uiFullDepth + 1) << 1);
        for (UInt uiPart = 0; uiPart < 4; uiPart++)
        {
            xEncSubdivCbfQT(pcCU, uiTrDepth + 1, uiAbsPartIdx + uiPart * uiQPartNum, bLuma, bChroma);
        }

        return;
    }

    //===== Cbfs =====
    if (bLuma)
    {
        m_pcEntropyCoder->encodeQtCbf(pcCU, uiAbsPartIdx, TEXT_LUMA,     uiTrMode);
    }
}

Void TEncSearch::xEncCoeffQT(TComDataCU* pcCU,
                             UInt        uiTrDepth,
                             UInt        uiAbsPartIdx,
                             TextType    eTextType,
                             Bool        bRealCoeff)
{
    UInt  uiFullDepth     = pcCU->getDepth(0) + uiTrDepth;
    UInt  uiTrMode        = pcCU->getTransformIdx(uiAbsPartIdx);
    UInt  uiSubdiv        = (uiTrMode > uiTrDepth ? 1 : 0);
    UInt  uiLog2TrafoSize = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth()] + 2 - uiFullDepth;
    UInt  uiChroma        = (eTextType != TEXT_LUMA ? 1 : 0);

    if (uiSubdiv)
    {
        UInt uiQPartNum = pcCU->getPic()->getNumPartInCU() >> ((uiFullDepth + 1) << 1);
        for (UInt uiPart = 0; uiPart < 4; uiPart++)
        {
            xEncCoeffQT(pcCU, uiTrDepth + 1, uiAbsPartIdx + uiPart * uiQPartNum, eTextType, bRealCoeff);
        }

        return;
    }

    if (eTextType != TEXT_LUMA && uiLog2TrafoSize == 2)
    {
        assert(uiTrDepth > 0);
        uiTrDepth--;
        UInt uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ((pcCU->getDepth(0) + uiTrDepth) << 1);
        Bool bFirstQ = ((uiAbsPartIdx % uiQPDiv) == 0);
        if (!bFirstQ)
        {
            return;
        }
    }

    //===== coefficients =====
    UInt    uiWidth         = pcCU->getWidth(0) >> (uiTrDepth + uiChroma);
    UInt    uiHeight        = pcCU->getHeight(0) >> (uiTrDepth + uiChroma);
    UInt    uiCoeffOffset   = (pcCU->getPic()->getMinCUWidth() * pcCU->getPic()->getMinCUHeight() * uiAbsPartIdx) >> (uiChroma << 1);
    UInt    uiQTLayer       = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrafoSize;
    TCoeff* pcCoeff         = 0;
    switch (eTextType)
    {
    case TEXT_LUMA:     pcCoeff = (bRealCoeff ? pcCU->getCoeffY() : m_ppcQTTempCoeffY[uiQTLayer]);
        break;
    case TEXT_CHROMA_U: pcCoeff = (bRealCoeff ? pcCU->getCoeffCb() : m_ppcQTTempCoeffCb[uiQTLayer]);
        break;
    case TEXT_CHROMA_V: pcCoeff = (bRealCoeff ? pcCU->getCoeffCr() : m_ppcQTTempCoeffCr[uiQTLayer]);
        break;
    default:            assert(0);
    }

    pcCoeff += uiCoeffOffset;

    m_pcEntropyCoder->encodeCoeffNxN(pcCU, pcCoeff, uiAbsPartIdx, uiWidth, uiHeight, uiFullDepth, eTextType);
}

Void TEncSearch::xEncIntraHeader(TComDataCU* pcCU,
                                 UInt        uiTrDepth,
                                 UInt        uiAbsPartIdx,
                                 Bool        bLuma,
                                 Bool        bChroma)
{
    if (bLuma)
    {
        // CU header
        if (uiAbsPartIdx == 0)
        {
            if (!pcCU->getSlice()->isIntra())
            {
                if (pcCU->getSlice()->getPPS()->getTransquantBypassEnableFlag())
                {
                    m_pcEntropyCoder->encodeCUTransquantBypassFlag(pcCU, 0, true);
                }
                m_pcEntropyCoder->encodeSkipFlag(pcCU, 0, true);
                m_pcEntropyCoder->encodePredMode(pcCU, 0, true);
            }

            m_pcEntropyCoder->encodePartSize(pcCU, 0, pcCU->getDepth(0), true);

            if (pcCU->isIntra(0) && pcCU->getPartitionSize(0) == SIZE_2Nx2N)
            {
                m_pcEntropyCoder->encodeIPCMInfo(pcCU, 0, true);

                if (pcCU->getIPCMFlag(0))
                {
                    return;
                }
            }
        }
        // luma prediction mode
        if (pcCU->getPartitionSize(0) == SIZE_2Nx2N)
        {
            if (uiAbsPartIdx == 0)
            {
                m_pcEntropyCoder->encodeIntraDirModeLuma(pcCU, 0);
            }
        }
        else
        {
            UInt uiQNumParts = pcCU->getTotalNumPart() >> 2;
            if (uiTrDepth == 0)
            {
                assert(uiAbsPartIdx == 0);
                for (UInt uiPart = 0; uiPart < 4; uiPart++)
                {
                    m_pcEntropyCoder->encodeIntraDirModeLuma(pcCU, uiPart * uiQNumParts);
                }
            }
            else if ((uiAbsPartIdx % uiQNumParts) == 0)
            {
                m_pcEntropyCoder->encodeIntraDirModeLuma(pcCU, uiAbsPartIdx);
            }
        }
    }
    if (bChroma)
    {
        // chroma prediction mode
        if (uiAbsPartIdx == 0)
        {
            m_pcEntropyCoder->encodeIntraDirModeChroma(pcCU, 0, true);
        }
    }
}

UInt TEncSearch::xGetIntraBitsQT(TComDataCU* pcCU,
                                 UInt        uiTrDepth,
                                 UInt        uiAbsPartIdx,
                                 Bool        bLuma,
                                 Bool        bChroma,
                                 Bool        bRealCoeff /* just for test */)
{
    m_pcEntropyCoder->resetBits();
    xEncIntraHeader(pcCU, uiTrDepth, uiAbsPartIdx, bLuma, bChroma);
    xEncSubdivCbfQT(pcCU, uiTrDepth, uiAbsPartIdx, bLuma, bChroma);

    if (bLuma)
    {
        xEncCoeffQT(pcCU, uiTrDepth, uiAbsPartIdx, TEXT_LUMA,      bRealCoeff);
    }
    if (bChroma)
    {
        xEncCoeffQT(pcCU, uiTrDepth, uiAbsPartIdx, TEXT_CHROMA_U,  bRealCoeff);
        xEncCoeffQT(pcCU, uiTrDepth, uiAbsPartIdx, TEXT_CHROMA_V,  bRealCoeff);
    }
    UInt   uiBits = m_pcEntropyCoder->getNumberOfWrittenBits();
    return uiBits;
}

UInt TEncSearch::xGetIntraBitsQTChroma(TComDataCU* pcCU,
                                       UInt        uiTrDepth,
                                       UInt        uiAbsPartIdx,
                                       UInt        uiChromaId,
                                       Bool        bRealCoeff /* just for test */)
{
    m_pcEntropyCoder->resetBits();
    if (uiChromaId == TEXT_CHROMA_U)
    {
        xEncCoeffQT(pcCU, uiTrDepth, uiAbsPartIdx, TEXT_CHROMA_U,  bRealCoeff);
    }
    else if (uiChromaId == TEXT_CHROMA_V)
    {
        xEncCoeffQT(pcCU, uiTrDepth, uiAbsPartIdx, TEXT_CHROMA_V,  bRealCoeff);
    }

    UInt   uiBits = m_pcEntropyCoder->getNumberOfWrittenBits();
    return uiBits;
}

Void TEncSearch::xIntraCodingLumaBlk(TComDataCU* pcCU,
                                     UInt        uiTrDepth,
                                     UInt        uiAbsPartIdx,
                                     TComYuv*    pcOrgYuv,
                                     TComYuv*    pcPredYuv,
                                     TShortYUV*  pcResiYuv,
                                     UInt&       ruiDist,
                                     Int         default0Save1Load2)
{
    UInt    uiLumaPredMode    = pcCU->getLumaIntraDir(uiAbsPartIdx);
    UInt    uiFullDepth       = pcCU->getDepth(0)  + uiTrDepth;
    UInt    uiWidth           = pcCU->getWidth(0) >> uiTrDepth;
    UInt    uiHeight          = pcCU->getHeight(0) >> uiTrDepth;
    UInt    uiStride          = pcOrgYuv->getStride();
    Pel*    piOrg             = pcOrgYuv->getLumaAddr(uiAbsPartIdx);
    Pel*    piPred            = pcPredYuv->getLumaAddr(uiAbsPartIdx);
    Short*  piResi            = pcResiYuv->getLumaAddr(uiAbsPartIdx);
    Pel*    piReco            = pcPredYuv->getLumaAddr(uiAbsPartIdx);

    UInt    uiLog2TrSize      = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth] + 2;
    UInt    uiQTLayer         = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;
    UInt    uiNumCoeffPerInc  = pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() >> (pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1);
    TCoeff* pcCoeff           = m_ppcQTTempCoeffY[uiQTLayer] + uiNumCoeffPerInc * uiAbsPartIdx;

    Int*    pcArlCoeff        = m_ppcQTTempArlCoeffY[uiQTLayer] + uiNumCoeffPerInc * uiAbsPartIdx;
    Short*  piRecQt           = m_pcQTTempTComYuv[uiQTLayer].getLumaAddr(uiAbsPartIdx);
    UInt    uiRecQtStride     = m_pcQTTempTComYuv[uiQTLayer].getStride();

    UInt    uiZOrder          = pcCU->getZorderIdxInCU() + uiAbsPartIdx;
    Pel*    piRecIPred        = pcCU->getPic()->getPicYuvRec()->getLumaAddr(pcCU->getAddr(), uiZOrder);
    UInt    uiRecIPredStride  = pcCU->getPic()->getPicYuvRec()->getStride();
    Bool    useTransformSkip  = pcCU->getTransformSkip(uiAbsPartIdx, TEXT_LUMA);
    //===== init availability pattern =====
    Bool  bAboveAvail = false;
    Bool  bLeftAvail  = false;

    if (default0Save1Load2 != 2)
    {
        pcCU->getPattern()->initPattern(pcCU, uiTrDepth, uiAbsPartIdx);
        pcCU->getPattern()->initAdiPattern(pcCU, uiAbsPartIdx, uiTrDepth, m_piPredBuf, m_iPredBufStride, m_iPredBufHeight, bAboveAvail, bLeftAvail, refAbove, refLeft, refAboveFlt, refLeftFlt);
        //===== get prediction signal =====
        predIntraLumaAng(pcCU->getPattern(), uiLumaPredMode, piPred, uiStride, uiWidth, uiHeight, bAboveAvail, bLeftAvail);
        // save prediction
        if (default0Save1Load2 == 1)
        {
            primitives.cpyblock((int)uiWidth, uiHeight, (pixel *)m_pSharedPredTransformSkip[0], uiWidth, (pixel *)piPred, uiStride);
        }
    }
    else
    {
        primitives.cpyblock((int)uiWidth, uiHeight, (pixel *)piPred, uiStride, (pixel *)m_pSharedPredTransformSkip[0], uiWidth);
    }

    //===== get residual signal =====

    primitives.calcresidual[(Int)g_aucConvertToBit[uiWidth]]((pixel*)piOrg, (pixel*)piPred, piResi, uiStride);

    //===== transform and quantization =====
    //--- init rate estimation arrays for RDOQ ---
    if (useTransformSkip ? m_pcEncCfg->getUseRDOQTS() : m_pcEncCfg->getUseRDOQ())
    {
        m_pcEntropyCoder->estimateBit(m_pcTrQuant->m_pcEstBitsSbac, uiWidth, uiWidth, TEXT_LUMA);
    }
    //--- transform and quantization ---
    UInt uiAbsSum = 0;
    pcCU->setTrIdxSubParts(uiTrDepth, uiAbsPartIdx, uiFullDepth);

    m_pcTrQuant->setQPforQuant(pcCU->getQP(0), TEXT_LUMA, pcCU->getSlice()->getSPS()->getQpBDOffsetY(), 0);

    m_pcTrQuant->selectLambda(TEXT_LUMA);

    m_pcTrQuant->transformNxN(pcCU, piResi, uiStride, pcCoeff, pcArlCoeff, uiWidth, uiHeight, uiAbsSum, TEXT_LUMA, uiAbsPartIdx, useTransformSkip);

    //--- set coded block flag ---
    pcCU->setCbfSubParts((uiAbsSum ? 1 : 0) << uiTrDepth, TEXT_LUMA, uiAbsPartIdx, uiFullDepth);
    //--- inverse transform ---
    if (uiAbsSum)
    {
        Int scalingListType = 0 + g_eTTable[(Int)TEXT_LUMA];
        assert(scalingListType < 6);
        m_pcTrQuant->invtransformNxN(pcCU->getCUTransquantBypass(uiAbsPartIdx), TEXT_LUMA, pcCU->getLumaIntraDir(uiAbsPartIdx), piResi, uiStride, pcCoeff, uiWidth, uiHeight, scalingListType, useTransformSkip);
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

    primitives.calcrecon[(Int)g_aucConvertToBit[uiWidth]]((pixel*)piPred, piResi, (pixel*)piReco, piRecQt, (pixel*)piRecIPred, uiStride, uiRecQtStride, uiRecIPredStride);

    //===== update distortion =====
    int Part = PartitionFromSizes(uiWidth, uiHeight);
    ruiDist += primitives.sse_pp[Part]((pixel*)piOrg, (intptr_t)uiStride, (pixel*)piReco, uiStride);
}

Void TEncSearch::xIntraCodingChromaBlk(TComDataCU* pcCU,
                                       UInt        uiTrDepth,
                                       UInt        uiAbsPartIdx,
                                       TComYuv*    pcOrgYuv,
                                       TComYuv*    pcPredYuv,
                                       TShortYUV*  pcResiYuv,
                                       UInt&       ruiDist,
                                       UInt        uiChromaId,
                                       Int         default0Save1Load2)
{
    UInt uiOrgTrDepth = uiTrDepth;
    UInt uiFullDepth  = pcCU->getDepth(0) + uiTrDepth;
    UInt uiLog2TrSize = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth] + 2;

    if (uiLog2TrSize == 2)
    {
        assert(uiTrDepth > 0);
        uiTrDepth--;
        UInt uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ((pcCU->getDepth(0) + uiTrDepth) << 1);
        Bool bFirstQ = ((uiAbsPartIdx % uiQPDiv) == 0);
        if (!bFirstQ)
        {
            return;
        }
    }

    TextType  eText             = (uiChromaId > 0 ? TEXT_CHROMA_V : TEXT_CHROMA_U);
    UInt      uiChromaPredMode  = pcCU->getChromaIntraDir(uiAbsPartIdx);
    UInt      uiWidth           = pcCU->getWidth(0) >> (uiTrDepth + 1);
    UInt      uiHeight          = pcCU->getHeight(0) >> (uiTrDepth + 1);
    UInt      uiStride          = pcOrgYuv->getCStride();
    Pel*      piOrg             = (uiChromaId > 0 ? pcOrgYuv->getCrAddr(uiAbsPartIdx) : pcOrgYuv->getCbAddr(uiAbsPartIdx));
    Pel*      piPred            = (uiChromaId > 0 ? pcPredYuv->getCrAddr(uiAbsPartIdx) : pcPredYuv->getCbAddr(uiAbsPartIdx));
    Short*      piResi            = (uiChromaId > 0 ? pcResiYuv->getCrAddr(uiAbsPartIdx) : pcResiYuv->getCbAddr(uiAbsPartIdx));
    Pel*      piReco            = (uiChromaId > 0 ? pcPredYuv->getCrAddr(uiAbsPartIdx) : pcPredYuv->getCbAddr(uiAbsPartIdx));

    UInt      uiQTLayer         = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;
    UInt      uiNumCoeffPerInc  = (pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() >> (pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1)) >> 2;
    TCoeff*   pcCoeff           = (uiChromaId > 0 ? m_ppcQTTempCoeffCr[uiQTLayer] : m_ppcQTTempCoeffCb[uiQTLayer]) + uiNumCoeffPerInc * uiAbsPartIdx;
    Int*      pcArlCoeff        = (uiChromaId > 0 ? m_ppcQTTempArlCoeffCr[uiQTLayer] : m_ppcQTTempArlCoeffCb[uiQTLayer]) + uiNumCoeffPerInc * uiAbsPartIdx;
    Short*      piRecQt           = (uiChromaId > 0 ? m_pcQTTempTComYuv[uiQTLayer].getCrAddr(uiAbsPartIdx) : m_pcQTTempTComYuv[uiQTLayer].getCbAddr(uiAbsPartIdx));
    UInt      uiRecQtStride     = m_pcQTTempTComYuv[uiQTLayer].getCStride();

    UInt      uiZOrder          = pcCU->getZorderIdxInCU() + uiAbsPartIdx;
    Pel*      piRecIPred        = (uiChromaId > 0 ? pcCU->getPic()->getPicYuvRec()->getCrAddr(pcCU->getAddr(), uiZOrder) : pcCU->getPic()->getPicYuvRec()->getCbAddr(pcCU->getAddr(), uiZOrder));
    UInt      uiRecIPredStride  = pcCU->getPic()->getPicYuvRec()->getCStride();
    Bool      useTransformSkipChroma       = pcCU->getTransformSkip(uiAbsPartIdx, eText);
    //===== update chroma mode =====
    if (uiChromaPredMode == DM_CHROMA_IDX)
    {
        uiChromaPredMode          = pcCU->getLumaIntraDir(0);
    }

    //===== init availability pattern =====
    Bool  bAboveAvail = false;
    Bool  bLeftAvail  = false;
    if (default0Save1Load2 != 2)
    {
        pcCU->getPattern()->initPattern(pcCU, uiTrDepth, uiAbsPartIdx);

        pcCU->getPattern()->initAdiPatternChroma(pcCU, uiAbsPartIdx, uiTrDepth, m_piPredBuf, m_iPredBufStride, m_iPredBufHeight, bAboveAvail, bLeftAvail);
        Pel*  pPatChroma  = (uiChromaId > 0 ? pcCU->getPattern()->getAdiCrBuf(uiWidth, uiHeight, m_piPredBuf) : pcCU->getPattern()->getAdiCbBuf(uiWidth, uiHeight, m_piPredBuf));

        //===== get prediction signal =====
        {
            predIntraChromaAng(pPatChroma, uiChromaPredMode, piPred, uiStride, uiWidth, uiHeight, bAboveAvail, bLeftAvail);
        }
        // save prediction
        if (default0Save1Load2 == 1)
        {
            Pel*  pPred   = piPred;
            Pel*  pPredBuf = m_pSharedPredTransformSkip[1 + uiChromaId];            
            
            primitives.cpyblock((int)uiWidth,uiHeight,(pixel *)pPredBuf,uiWidth, (pixel *)pPred, uiStride);
        }
    }
    else
    {
        // load prediction
        Pel*  pPred   = piPred;
        Pel*  pPredBuf = m_pSharedPredTransformSkip[1 + uiChromaId];
        
        primitives.cpyblock((int)uiWidth,uiHeight,(pixel *)pPred,uiStride,(pixel *)pPredBuf,uiWidth);
    }
    //===== get residual signal =====    

    primitives.calcresidual[(Int)g_aucConvertToBit[uiWidth]]((pixel*)piOrg,(pixel*)piPred,piResi, uiStride);

    //===== transform and quantization =====
    {
        //--- init rate estimation arrays for RDOQ ---
        if (useTransformSkipChroma ? m_pcEncCfg->getUseRDOQTS() : m_pcEncCfg->getUseRDOQ())
        {
            m_pcEntropyCoder->estimateBit(m_pcTrQuant->m_pcEstBitsSbac, uiWidth, uiWidth, eText);
        }
        //--- transform and quantization ---
        UInt uiAbsSum = 0;

        Int curChromaQpOffset;
        if (eText == TEXT_CHROMA_U)
        {
            curChromaQpOffset = pcCU->getSlice()->getPPS()->getChromaCbQpOffset() + pcCU->getSlice()->getSliceQpDeltaCb();
        }
        else
        {
            curChromaQpOffset = pcCU->getSlice()->getPPS()->getChromaCrQpOffset() + pcCU->getSlice()->getSliceQpDeltaCr();
        }
        m_pcTrQuant->setQPforQuant(pcCU->getQP(0), TEXT_CHROMA, pcCU->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset);

        m_pcTrQuant->selectLambda(TEXT_CHROMA);

        m_pcTrQuant->transformNxN(pcCU, piResi, uiStride, pcCoeff, pcArlCoeff, uiWidth, uiHeight, uiAbsSum, eText, uiAbsPartIdx, useTransformSkipChroma);
        //--- set coded block flag ---
        pcCU->setCbfSubParts((uiAbsSum ? 1 : 0) << uiOrgTrDepth, eText, uiAbsPartIdx, pcCU->getDepth(0) + uiTrDepth);
        //--- inverse transform ---
        if (uiAbsSum)
        {
            Int scalingListType = 0 + g_eTTable[(Int)eText];
            assert(scalingListType < 6);
            m_pcTrQuant->invtransformNxN(pcCU->getCUTransquantBypass(uiAbsPartIdx), TEXT_CHROMA, REG_DCT, piResi, uiStride, pcCoeff, uiWidth, uiHeight, scalingListType, useTransformSkipChroma);
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
    
    primitives.calcrecon[(Int)g_aucConvertToBit[uiWidth]]((pixel*)piPred, piResi, (pixel*)piReco, piRecQt, (pixel*)piRecIPred, uiStride, uiRecQtStride, uiRecIPredStride);

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

Void TEncSearch::xRecurIntraCodingQT(TComDataCU* pcCU,
                                     UInt        uiTrDepth,
                                     UInt        uiAbsPartIdx,
                                     Bool        bLumaOnly,
                                     TComYuv*    pcOrgYuv,
                                     TComYuv*    pcPredYuv,
                                     TShortYUV*  pcResiYuv,
                                     UInt&       ruiDistY,
                                     UInt&       ruiDistC,
                                     Bool        bCheckFirst,
                                     UInt64&     dRDCost)
{
    UInt    uiFullDepth   = pcCU->getDepth(0) +  uiTrDepth;
    UInt    uiLog2TrSize  = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth] + 2;
    Bool    bCheckFull    = (uiLog2TrSize  <= pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize());
    Bool    bCheckSplit   = (uiLog2TrSize  >  pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx));

    Int maxTuSize = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize();
    Int isIntraSlice = (pcCU->getSlice()->getSliceType() == I_SLICE);
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
    Bool    checkTransformSkip  = pcCU->getSlice()->getPPS()->getUseTransformSkip();
    UInt    widthTransformSkip  = pcCU->getWidth(0) >> uiTrDepth;
    UInt    heightTransformSkip = pcCU->getHeight(0) >> uiTrDepth;
    Int     bestModeId    = 0;
    Int     bestModeIdUV[2] = { 0, 0 };

    checkTransformSkip &= (widthTransformSkip == 4 && heightTransformSkip == 4);
    checkTransformSkip &= (!pcCU->getCUTransquantBypass(0));
    checkTransformSkip &= (!((pcCU->getQP(0) == 0) && (pcCU->getSlice()->getSPS()->getUseLossless())));
    if (m_pcEncCfg->getUseTransformSkipFast())
    {
        checkTransformSkip &= (pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_NxN);
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

            UInt   uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ((pcCU->getDepth(0) + (uiTrDepth - 1)) << 1);
            Bool   bFirstQ = ((uiAbsPartIdx % uiQPDiv) == 0);

            for (Int modeId = firstCheckId; modeId < 2; modeId++)
            {
                singleDistYTmp = 0;
                singleDistCTmp = 0;
                pcCU->setTransformSkipSubParts(modeId, TEXT_LUMA, uiAbsPartIdx, uiFullDepth);
                if (modeId == firstCheckId)
                {
                    default0Save1Load2 = 1;
                }
                else
                {
                    default0Save1Load2 = 2;
                }
                //----- code luma block with given intra prediction mode and store Cbf-----
                xIntraCodingLumaBlk(pcCU, uiTrDepth, uiAbsPartIdx, pcOrgYuv, pcPredYuv, pcResiYuv, singleDistYTmp, default0Save1Load2);
                singleCbfYTmp = pcCU->getCbf(uiAbsPartIdx, TEXT_LUMA, uiTrDepth);
                //----- code chroma blocks with given intra prediction mode and store Cbf-----
                if (!bLumaOnly)
                {
                    if (bFirstQ)
                    {
                        pcCU->setTransformSkipSubParts(modeId, TEXT_CHROMA_U, uiAbsPartIdx, uiFullDepth);
                        pcCU->setTransformSkipSubParts(modeId, TEXT_CHROMA_V, uiAbsPartIdx, uiFullDepth);
                    }
                    xIntraCodingChromaBlk(pcCU, uiTrDepth, uiAbsPartIdx, pcOrgYuv, pcPredYuv, pcResiYuv, singleDistCTmp, 0, default0Save1Load2);
                    xIntraCodingChromaBlk(pcCU, uiTrDepth, uiAbsPartIdx, pcOrgYuv, pcPredYuv, pcResiYuv, singleDistCTmp, 1, default0Save1Load2);
                    singleCbfUTmp = pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepth);
                    singleCbfVTmp = pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepth);
                }
                //----- determine rate and r-d cost -----
                if (modeId == 1 && singleCbfYTmp == 0)
                {
                    //In order not to code TS flag when cbf is zero, the case for TS with cbf being zero is forbidden.
                    singleCostTmp = MAX_INT64;
                }
                else
                {
                    UInt uiSingleBits = xGetIntraBitsQT(pcCU, uiTrDepth, uiAbsPartIdx, true, !bLumaOnly, false);
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
                        xStoreIntraResultQT(pcCU, uiTrDepth, uiAbsPartIdx, bLumaOnly);
                        m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiFullDepth][CI_TEMP_BEST]);
                    }
                }
                if (modeId == firstCheckId)
                {
                    m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiFullDepth][CI_QT_TRAFO_ROOT]);
                }
            }

            pcCU->setTransformSkipSubParts(bestModeId, TEXT_LUMA, uiAbsPartIdx, uiFullDepth);

            if (bestModeId == firstCheckId)
            {
                xLoadIntraResultQT(pcCU, uiTrDepth, uiAbsPartIdx, bLumaOnly);
                pcCU->setCbfSubParts(uiSingleCbfY << uiTrDepth, TEXT_LUMA, uiAbsPartIdx, uiFullDepth);
                if (!bLumaOnly)
                {
                    if (bFirstQ)
                    {
                        pcCU->setCbfSubParts(uiSingleCbfU << uiTrDepth, TEXT_CHROMA_U, uiAbsPartIdx, pcCU->getDepth(0) + uiTrDepth - 1);
                        pcCU->setCbfSubParts(uiSingleCbfV << uiTrDepth, TEXT_CHROMA_V, uiAbsPartIdx, pcCU->getDepth(0) + uiTrDepth - 1);
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
                        pcCU->setTransformSkipSubParts(0, TEXT_CHROMA_U, uiAbsPartIdx, uiFullDepth);
                        bestModeIdUV[0] = 0;
                    }
                    if (uiSingleCbfV == 0)
                    {
                        pcCU->setTransformSkipSubParts(0, TEXT_CHROMA_V, uiAbsPartIdx, uiFullDepth);
                        bestModeIdUV[1] = 0;
                    }
                }
            }
        }
        else
        {
            pcCU->setTransformSkipSubParts(0, TEXT_LUMA, uiAbsPartIdx, uiFullDepth);

            //----- store original entropy coding status -----
            m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiFullDepth][CI_QT_TRAFO_ROOT]);

            //----- code luma block with given intra prediction mode and store Cbf-----
            dSingleCost   = 0.0;
            xIntraCodingLumaBlk(pcCU, uiTrDepth, uiAbsPartIdx, pcOrgYuv, pcPredYuv, pcResiYuv, uiSingleDistY);
            if (bCheckSplit)
            {
                uiSingleCbfY = pcCU->getCbf(uiAbsPartIdx, TEXT_LUMA, uiTrDepth);
            }
            //----- code chroma blocks with given intra prediction mode and store Cbf-----
            if (!bLumaOnly)
            {
                pcCU->setTransformSkipSubParts(0, TEXT_CHROMA_U, uiAbsPartIdx, uiFullDepth);
                pcCU->setTransformSkipSubParts(0, TEXT_CHROMA_V, uiAbsPartIdx, uiFullDepth);
                xIntraCodingChromaBlk(pcCU, uiTrDepth, uiAbsPartIdx, pcOrgYuv, pcPredYuv, pcResiYuv, uiSingleDistC, 0);
                xIntraCodingChromaBlk(pcCU, uiTrDepth, uiAbsPartIdx, pcOrgYuv, pcPredYuv, pcResiYuv, uiSingleDistC, 1);
                if (bCheckSplit)
                {
                    uiSingleCbfU = pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepth);
                    uiSingleCbfV = pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepth);
                }
            }
            //----- determine rate and r-d cost -----
            UInt uiSingleBits = xGetIntraBitsQT(pcCU, uiTrDepth, uiAbsPartIdx, true, !bLumaOnly, false);
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
        UInt    uiQPartsDiv     = pcCU->getPic()->getNumPartInCU() >> ((uiFullDepth + 1) << 1);
        UInt    uiAbsPartIdxSub = uiAbsPartIdx;

        UInt    uiSplitCbfY = 0;
        UInt    uiSplitCbfU = 0;
        UInt    uiSplitCbfV = 0;

        for (UInt uiPart = 0; uiPart < 4; uiPart++, uiAbsPartIdxSub += uiQPartsDiv)
        {
            xRecurIntraCodingQT(pcCU, uiTrDepth + 1, uiAbsPartIdxSub, bLumaOnly, pcOrgYuv, pcPredYuv, pcResiYuv, uiSplitDistY, uiSplitDistC, bCheckFirst, dSplitCost);

            uiSplitCbfY |= pcCU->getCbf(uiAbsPartIdxSub, TEXT_LUMA, uiTrDepth + 1);
            if (!bLumaOnly)
            {
                uiSplitCbfU |= pcCU->getCbf(uiAbsPartIdxSub, TEXT_CHROMA_U, uiTrDepth + 1);
                uiSplitCbfV |= pcCU->getCbf(uiAbsPartIdxSub, TEXT_CHROMA_V, uiTrDepth + 1);
            }
        }

        for (UInt uiOffs = 0; uiOffs < 4 * uiQPartsDiv; uiOffs++)
        {
            pcCU->getCbf(TEXT_LUMA)[uiAbsPartIdx + uiOffs] |= (uiSplitCbfY << uiTrDepth);
        }

        if (!bLumaOnly)
        {
            for (UInt uiOffs = 0; uiOffs < 4 * uiQPartsDiv; uiOffs++)
            {
                pcCU->getCbf(TEXT_CHROMA_U)[uiAbsPartIdx + uiOffs] |= (uiSplitCbfU << uiTrDepth);
                pcCU->getCbf(TEXT_CHROMA_V)[uiAbsPartIdx + uiOffs] |= (uiSplitCbfV << uiTrDepth);
            }
        }
        //----- restore context states -----
        m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiFullDepth][CI_QT_TRAFO_ROOT]);

        //----- determine rate and r-d cost -----
        UInt uiSplitBits = xGetIntraBitsQT(pcCU, uiTrDepth, uiAbsPartIdx, true, !bLumaOnly, false);
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
        pcCU->setTrIdxSubParts(uiTrDepth, uiAbsPartIdx, uiFullDepth);
        pcCU->setCbfSubParts(uiSingleCbfY << uiTrDepth, TEXT_LUMA, uiAbsPartIdx, uiFullDepth);
        pcCU->setTransformSkipSubParts(bestModeId, TEXT_LUMA, uiAbsPartIdx, uiFullDepth);
        if (!bLumaOnly)
        {
            pcCU->setCbfSubParts(uiSingleCbfU << uiTrDepth, TEXT_CHROMA_U, uiAbsPartIdx, uiFullDepth);
            pcCU->setCbfSubParts(uiSingleCbfV << uiTrDepth, TEXT_CHROMA_V, uiAbsPartIdx, uiFullDepth);
            pcCU->setTransformSkipSubParts(bestModeIdUV[0], TEXT_CHROMA_U, uiAbsPartIdx, uiFullDepth);
            pcCU->setTransformSkipSubParts(bestModeIdUV[1], TEXT_CHROMA_V, uiAbsPartIdx, uiFullDepth);
        }

        //--- set reconstruction for next intra prediction blocks ---
        UInt  uiWidth     = pcCU->getWidth(0) >> uiTrDepth;
        UInt  uiHeight    = pcCU->getHeight(0) >> uiTrDepth;
        UInt  uiQTLayer   = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;
        UInt  uiZOrder    = pcCU->getZorderIdxInCU() + uiAbsPartIdx;
        Short*  piSrc     = m_pcQTTempTComYuv[uiQTLayer].getLumaAddr(uiAbsPartIdx);
        UInt  uiSrcStride = m_pcQTTempTComYuv[uiQTLayer].getStride();
        Pel*  piDes       = pcCU->getPic()->getPicYuvRec()->getLumaAddr(pcCU->getAddr(), uiZOrder);
        UInt  uiDesStride = pcCU->getPic()->getPicYuvRec()->getStride();
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
            uiSrcStride = m_pcQTTempTComYuv[uiQTLayer].getCStride();
            piDes       = pcCU->getPic()->getPicYuvRec()->getCbAddr(pcCU->getAddr(), uiZOrder);
            uiDesStride = pcCU->getPic()->getPicYuvRec()->getCStride();
            for (UInt uiY = 0; uiY < uiHeight; uiY++, piSrc += uiSrcStride, piDes += uiDesStride)
            {
                for (UInt uiX = 0; uiX < uiWidth; uiX++)
                {
                    piDes[uiX] = piSrc[uiX];
                }
            }

            piSrc = m_pcQTTempTComYuv[uiQTLayer].getCrAddr(uiAbsPartIdx);
            piDes = pcCU->getPic()->getPicYuvRec()->getCrAddr(pcCU->getAddr(), uiZOrder);
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

Void TEncSearch::xSetIntraResultQT(TComDataCU* pcCU,
                                   UInt        uiTrDepth,
                                   UInt        uiAbsPartIdx,
                                   Bool        bLumaOnly,
                                   TComYuv*    pcRecoYuv)
{
    UInt uiFullDepth  = pcCU->getDepth(0) + uiTrDepth;
    UInt uiTrMode     = pcCU->getTransformIdx(uiAbsPartIdx);

    if (uiTrMode == uiTrDepth)
    {
        UInt uiLog2TrSize = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth] + 2;
        UInt uiQTLayer    = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;

        Bool bSkipChroma  = false;
        Bool bChromaSame  = false;
        if (!bLumaOnly && uiLog2TrSize == 2)
        {
            assert(uiTrDepth > 0);
            UInt uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ((pcCU->getDepth(0) + uiTrDepth - 1) << 1);
            bSkipChroma  = ((uiAbsPartIdx % uiQPDiv) != 0);
            bChromaSame  = true;
        }

        //===== copy transform coefficients =====
        UInt uiNumCoeffY    = (pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight()) >> (uiFullDepth << 1);
        UInt uiNumCoeffIncY = (pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight()) >> (pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1);
        TCoeff* pcCoeffSrcY = m_ppcQTTempCoeffY[uiQTLayer] + (uiNumCoeffIncY * uiAbsPartIdx);
        TCoeff* pcCoeffDstY = pcCU->getCoeffY()              + (uiNumCoeffIncY * uiAbsPartIdx);
        ::memcpy(pcCoeffDstY, pcCoeffSrcY, sizeof(TCoeff) * uiNumCoeffY);
        Int* pcArlCoeffSrcY = m_ppcQTTempArlCoeffY[uiQTLayer] + (uiNumCoeffIncY * uiAbsPartIdx);
        Int* pcArlCoeffDstY = pcCU->getArlCoeffY()              + (uiNumCoeffIncY * uiAbsPartIdx);
        ::memcpy(pcArlCoeffDstY, pcArlCoeffSrcY, sizeof(Int) * uiNumCoeffY);
        if (!bLumaOnly && !bSkipChroma)
        {
            UInt uiNumCoeffC    = (bChromaSame ? uiNumCoeffY    : uiNumCoeffY    >> 2);
            UInt uiNumCoeffIncC = uiNumCoeffIncY >> 2;
            TCoeff* pcCoeffSrcU = m_ppcQTTempCoeffCb[uiQTLayer] + (uiNumCoeffIncC * uiAbsPartIdx);
            TCoeff* pcCoeffSrcV = m_ppcQTTempCoeffCr[uiQTLayer] + (uiNumCoeffIncC * uiAbsPartIdx);
            TCoeff* pcCoeffDstU = pcCU->getCoeffCb()              + (uiNumCoeffIncC * uiAbsPartIdx);
            TCoeff* pcCoeffDstV = pcCU->getCoeffCr()              + (uiNumCoeffIncC * uiAbsPartIdx);
            ::memcpy(pcCoeffDstU, pcCoeffSrcU, sizeof(TCoeff) * uiNumCoeffC);
            ::memcpy(pcCoeffDstV, pcCoeffSrcV, sizeof(TCoeff) * uiNumCoeffC);
            Int* pcArlCoeffSrcU = m_ppcQTTempArlCoeffCb[uiQTLayer] + (uiNumCoeffIncC * uiAbsPartIdx);
            Int* pcArlCoeffSrcV = m_ppcQTTempArlCoeffCr[uiQTLayer] + (uiNumCoeffIncC * uiAbsPartIdx);
            Int* pcArlCoeffDstU = pcCU->getArlCoeffCb()              + (uiNumCoeffIncC * uiAbsPartIdx);
            Int* pcArlCoeffDstV = pcCU->getArlCoeffCr()              + (uiNumCoeffIncC * uiAbsPartIdx);
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
        UInt uiNumQPart  = pcCU->getPic()->getNumPartInCU() >> ((uiFullDepth + 1) << 1);
        for (UInt uiPart = 0; uiPart < 4; uiPart++)
        {
            xSetIntraResultQT(pcCU, uiTrDepth + 1, uiAbsPartIdx + uiPart * uiNumQPart, bLumaOnly, pcRecoYuv);
        }
    }
}

Void TEncSearch::xStoreIntraResultQT(TComDataCU* pcCU,
                                     UInt        uiTrDepth,
                                     UInt        uiAbsPartIdx,
                                     Bool        bLumaOnly)
{
    UInt uiFullDepth  = pcCU->getDepth(0) + uiTrDepth;
    UInt uiTrMode     = pcCU->getTransformIdx(uiAbsPartIdx);

    assert(uiTrMode == uiTrDepth);
    UInt uiLog2TrSize = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth] + 2;
    UInt uiQTLayer    = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;

    Bool bSkipChroma  = false;
    Bool bChromaSame  = false;
    if (!bLumaOnly && uiLog2TrSize == 2)
    {
        assert(uiTrDepth > 0);
        UInt uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ((pcCU->getDepth(0) + uiTrDepth - 1) << 1);
        bSkipChroma  = ((uiAbsPartIdx % uiQPDiv) != 0);
        bChromaSame  = true;
    }

    //===== copy transform coefficients =====
    UInt uiNumCoeffY    = (pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight()) >> (uiFullDepth << 1);
    UInt uiNumCoeffIncY = (pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight()) >> (pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1);
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

Void TEncSearch::xLoadIntraResultQT(TComDataCU* pcCU,
                                    UInt        uiTrDepth,
                                    UInt        uiAbsPartIdx,
                                    Bool        bLumaOnly)
{
    UInt uiFullDepth  = pcCU->getDepth(0) + uiTrDepth;
    UInt uiTrMode     = pcCU->getTransformIdx(uiAbsPartIdx);

    assert(uiTrMode == uiTrDepth);
    UInt uiLog2TrSize = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth] + 2;
    UInt uiQTLayer    = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;

    Bool bSkipChroma  = false;
    Bool bChromaSame  = false;
    if (!bLumaOnly && uiLog2TrSize == 2)
    {
        assert(uiTrDepth > 0);
        UInt uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ((pcCU->getDepth(0) + uiTrDepth - 1) << 1);
        bSkipChroma  = ((uiAbsPartIdx % uiQPDiv) != 0);
        bChromaSame  = true;
    }

    //===== copy transform coefficients =====
    UInt uiNumCoeffY    = (pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight()) >> (uiFullDepth << 1);
    UInt uiNumCoeffIncY = (pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight()) >> (pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1);
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

    UInt    uiZOrder          = pcCU->getZorderIdxInCU() + uiAbsPartIdx;
    Pel*    piRecIPred        = pcCU->getPic()->getPicYuvRec()->getLumaAddr(pcCU->getAddr(), uiZOrder);
    UInt    uiRecIPredStride  = pcCU->getPic()->getPicYuvRec()->getStride();
    Short*    piRecQt           = m_pcQTTempTComYuv[uiQTLayer].getLumaAddr(uiAbsPartIdx);
    UInt    uiRecQtStride     = m_pcQTTempTComYuv[uiQTLayer].getStride();
    UInt    uiWidth           = pcCU->getWidth(0) >> uiTrDepth;
    UInt    uiHeight          = pcCU->getHeight(0) >> uiTrDepth;
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
        piRecIPred = pcCU->getPic()->getPicYuvRec()->getCbAddr(pcCU->getAddr(), uiZOrder);
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

        piRecIPred = pcCU->getPic()->getPicYuvRec()->getCrAddr(pcCU->getAddr(), uiZOrder);
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

Void TEncSearch::xStoreIntraResultChromaQT(TComDataCU* pcCU,
                                           UInt        uiTrDepth,
                                           UInt        uiAbsPartIdx,
                                           UInt        stateU0V1Both2)
{
    UInt uiFullDepth = pcCU->getDepth(0) + uiTrDepth;
    UInt uiTrMode    = pcCU->getTransformIdx(uiAbsPartIdx);

    if (uiTrMode == uiTrDepth)
    {
        UInt uiLog2TrSize = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth] + 2;
        UInt uiQTLayer    = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;

        Bool bChromaSame = false;
        if (uiLog2TrSize == 2)
        {
            assert(uiTrDepth > 0);
            uiTrDepth--;
            UInt uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ((pcCU->getDepth(0) + uiTrDepth) << 1);
            if ((uiAbsPartIdx % uiQPDiv) != 0)
            {
                return;
            }
            bChromaSame = true;
        }

        //===== copy transform coefficients =====
        UInt uiNumCoeffC    = (pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight()) >> (uiFullDepth << 1);
        if (!bChromaSame)
        {
            uiNumCoeffC     >>= 2;
        }
        UInt uiNumCoeffIncC = (pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight()) >> ((pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1) + 2);
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

Void TEncSearch::xLoadIntraResultChromaQT(TComDataCU* pcCU,
                                          UInt        uiTrDepth,
                                          UInt        uiAbsPartIdx,
                                          UInt        stateU0V1Both2)
{
    UInt uiFullDepth = pcCU->getDepth(0) + uiTrDepth;
    UInt uiTrMode    = pcCU->getTransformIdx(uiAbsPartIdx);

    if (uiTrMode == uiTrDepth)
    {
        UInt uiLog2TrSize = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth] + 2;
        UInt uiQTLayer    = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;

        Bool bChromaSame = false;
        if (uiLog2TrSize == 2)
        {
            assert(uiTrDepth > 0);
            uiTrDepth--;
            UInt uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ((pcCU->getDepth(0) + uiTrDepth) << 1);
            if ((uiAbsPartIdx % uiQPDiv) != 0)
            {
                return;
            }
            bChromaSame = true;
        }

        //===== copy transform coefficients =====
        UInt uiNumCoeffC = (pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight()) >> (uiFullDepth << 1);
        if (!bChromaSame)
        {
            uiNumCoeffC >>= 2;
        }
        UInt uiNumCoeffIncC = (pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight()) >> ((pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1) + 2);

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

        UInt    uiZOrder          = pcCU->getZorderIdxInCU() + uiAbsPartIdx;
        UInt    uiWidth           = pcCU->getWidth(0) >> (uiTrDepth + 1);
        UInt    uiHeight          = pcCU->getHeight(0) >> (uiTrDepth + 1);
        UInt    uiRecQtStride     = m_pcQTTempTComYuv[uiQTLayer].getCStride();
        UInt    uiRecIPredStride  = pcCU->getPic()->getPicYuvRec()->getCStride();

        if (stateU0V1Both2 == 0 || stateU0V1Both2 == 2)
        {
            Pel* piRecIPred = pcCU->getPic()->getPicYuvRec()->getCbAddr(pcCU->getAddr(), uiZOrder);
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
            Pel* piRecIPred = pcCU->getPic()->getPicYuvRec()->getCrAddr(pcCU->getAddr(), uiZOrder);
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

Void TEncSearch::xRecurIntraChromaCodingQT(TComDataCU* pcCU,
                                           UInt        uiTrDepth,
                                           UInt        uiAbsPartIdx,
                                           TComYuv*    pcOrgYuv,
                                           TComYuv*    pcPredYuv,
                                           TShortYUV*  pcResiYuv,
                                           UInt&       ruiDist)
{
    UInt uiFullDepth = pcCU->getDepth(0) +  uiTrDepth;
    UInt uiTrMode    = pcCU->getTransformIdx(uiAbsPartIdx);

    if (uiTrMode == uiTrDepth)
    {
        Bool checkTransformSkip = pcCU->getSlice()->getPPS()->getUseTransformSkip();
        UInt uiLog2TrSize = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth] + 2;

        UInt actualTrDepth = uiTrDepth;
        if (uiLog2TrSize == 2)
        {
            assert(uiTrDepth > 0);
            actualTrDepth--;
            UInt uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ((pcCU->getDepth(0) + actualTrDepth) << 1);
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
                    nbLumaSkip += pcCU->getTransformSkip(absPartIdxSub, TEXT_LUMA);
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
                    pcCU->setTransformSkipSubParts(chromaModeId, (TextType)(chromaId + 2), uiAbsPartIdx, pcCU->getDepth(0) +  actualTrDepth);
                    if (chromaModeId == firstCheckId)
                    {
                        default0Save1Load2 = 1;
                    }
                    else
                    {
                        default0Save1Load2 = 2;
                    }
                    singleDistCTmp = 0;
                    xIntraCodingChromaBlk(pcCU, uiTrDepth, uiAbsPartIdx, pcOrgYuv, pcPredYuv, pcResiYuv, singleDistCTmp, chromaId, default0Save1Load2);
                    singleCbfCTmp = pcCU->getCbf(uiAbsPartIdx, (TextType)(chromaId + 2), uiTrDepth);

                    if (chromaModeId == 1 && singleCbfCTmp == 0)
                    {
                        //In order not to code TS flag when cbf is zero, the case for TS with cbf being zero is forbidden.
                        singleCostTmp = MAX_INT64;
                    }
                    else
                    {
                        UInt bitsTmp = xGetIntraBitsQTChroma(pcCU, uiTrDepth, uiAbsPartIdx, chromaId + 2, false);
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
                            xStoreIntraResultChromaQT(pcCU, uiTrDepth, uiAbsPartIdx, chromaId);
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
                    xLoadIntraResultChromaQT(pcCU, uiTrDepth, uiAbsPartIdx, chromaId);
                    pcCU->setCbfSubParts(singleCbfC << uiTrDepth, (TextType)(chromaId + 2), uiAbsPartIdx, pcCU->getDepth(0) + actualTrDepth);
                    m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiFullDepth][CI_TEMP_BEST]);
                }
                pcCU->setTransformSkipSubParts(bestModeId, (TextType)(chromaId + 2), uiAbsPartIdx, pcCU->getDepth(0) +  actualTrDepth);
                ruiDist += singleDistC;

                if (chromaId == 0)
                {
                    m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiFullDepth][CI_QT_TRAFO_ROOT]);
                }
            }
        }
        else
        {
            pcCU->setTransformSkipSubParts(0, TEXT_CHROMA_U, uiAbsPartIdx, pcCU->getDepth(0) +  actualTrDepth);
            pcCU->setTransformSkipSubParts(0, TEXT_CHROMA_V, uiAbsPartIdx, pcCU->getDepth(0) +  actualTrDepth);
            xIntraCodingChromaBlk(pcCU, uiTrDepth, uiAbsPartIdx, pcOrgYuv, pcPredYuv, pcResiYuv, ruiDist, 0);
            xIntraCodingChromaBlk(pcCU, uiTrDepth, uiAbsPartIdx, pcOrgYuv, pcPredYuv, pcResiYuv, ruiDist, 1);
        }
    }
    else
    {
        UInt uiSplitCbfU     = 0;
        UInt uiSplitCbfV     = 0;
        UInt uiQPartsDiv     = pcCU->getPic()->getNumPartInCU() >> ((uiFullDepth + 1) << 1);
        UInt uiAbsPartIdxSub = uiAbsPartIdx;
        for (UInt uiPart = 0; uiPart < 4; uiPart++, uiAbsPartIdxSub += uiQPartsDiv)
        {
            xRecurIntraChromaCodingQT(pcCU, uiTrDepth + 1, uiAbsPartIdxSub, pcOrgYuv, pcPredYuv, pcResiYuv, ruiDist);
            uiSplitCbfU |= pcCU->getCbf(uiAbsPartIdxSub, TEXT_CHROMA_U, uiTrDepth + 1);
            uiSplitCbfV |= pcCU->getCbf(uiAbsPartIdxSub, TEXT_CHROMA_V, uiTrDepth + 1);
        }

        for (UInt uiOffs = 0; uiOffs < 4 * uiQPartsDiv; uiOffs++)
        {
            pcCU->getCbf(TEXT_CHROMA_U)[uiAbsPartIdx + uiOffs] |= (uiSplitCbfU << uiTrDepth);
            pcCU->getCbf(TEXT_CHROMA_V)[uiAbsPartIdx + uiOffs] |= (uiSplitCbfV << uiTrDepth);
        }
    }
}

Void TEncSearch::xSetIntraResultChromaQT(TComDataCU* pcCU,
                                         UInt        uiTrDepth,
                                         UInt        uiAbsPartIdx,
                                         TComYuv*    pcRecoYuv)
{
    UInt uiFullDepth  = pcCU->getDepth(0) + uiTrDepth;
    UInt uiTrMode     = pcCU->getTransformIdx(uiAbsPartIdx);

    if (uiTrMode == uiTrDepth)
    {
        UInt uiLog2TrSize = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth] + 2;
        UInt uiQTLayer    = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;

        Bool bChromaSame  = false;
        if (uiLog2TrSize == 2)
        {
            assert(uiTrDepth > 0);
            UInt uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ((pcCU->getDepth(0) + uiTrDepth - 1) << 1);
            if ((uiAbsPartIdx % uiQPDiv) != 0)
            {
                return;
            }
            bChromaSame     = true;
        }

        //===== copy transform coefficients =====
        UInt uiNumCoeffC    = (pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight()) >> (uiFullDepth << 1);
        if (!bChromaSame)
        {
            uiNumCoeffC     >>= 2;
        }
        UInt uiNumCoeffIncC = (pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight()) >> ((pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1) + 2);
        TCoeff* pcCoeffSrcU = m_ppcQTTempCoeffCb[uiQTLayer] + (uiNumCoeffIncC * uiAbsPartIdx);
        TCoeff* pcCoeffSrcV = m_ppcQTTempCoeffCr[uiQTLayer] + (uiNumCoeffIncC * uiAbsPartIdx);
        TCoeff* pcCoeffDstU = pcCU->getCoeffCb()              + (uiNumCoeffIncC * uiAbsPartIdx);
        TCoeff* pcCoeffDstV = pcCU->getCoeffCr()              + (uiNumCoeffIncC * uiAbsPartIdx);
        ::memcpy(pcCoeffDstU, pcCoeffSrcU, sizeof(TCoeff) * uiNumCoeffC);
        ::memcpy(pcCoeffDstV, pcCoeffSrcV, sizeof(TCoeff) * uiNumCoeffC);
        Int* pcArlCoeffSrcU = m_ppcQTTempArlCoeffCb[uiQTLayer] + (uiNumCoeffIncC * uiAbsPartIdx);
        Int* pcArlCoeffSrcV = m_ppcQTTempArlCoeffCr[uiQTLayer] + (uiNumCoeffIncC * uiAbsPartIdx);
        Int* pcArlCoeffDstU = pcCU->getArlCoeffCb()              + (uiNumCoeffIncC * uiAbsPartIdx);
        Int* pcArlCoeffDstV = pcCU->getArlCoeffCr()              + (uiNumCoeffIncC * uiAbsPartIdx);
        ::memcpy(pcArlCoeffDstU, pcArlCoeffSrcU, sizeof(Int) * uiNumCoeffC);
        ::memcpy(pcArlCoeffDstV, pcArlCoeffSrcV, sizeof(Int) * uiNumCoeffC);

        //===== copy reconstruction =====
        UInt uiLog2TrSizeChroma = (bChromaSame ? uiLog2TrSize : uiLog2TrSize - 1);
        m_pcQTTempTComYuv[uiQTLayer].copyPartToPartChroma(pcRecoYuv, uiAbsPartIdx, 1 << uiLog2TrSizeChroma, 1 << uiLog2TrSizeChroma);
    }
    else
    {
        UInt uiNumQPart  = pcCU->getPic()->getNumPartInCU() >> ((uiFullDepth + 1) << 1);
        for (UInt uiPart = 0; uiPart < 4; uiPart++)
        {
            xSetIntraResultChromaQT(pcCU, uiTrDepth + 1, uiAbsPartIdx + uiPart * uiNumQPart, pcRecoYuv);
        }
    }
}

Void TEncSearch::preestChromaPredMode(TComDataCU* pcCU,
                                      TComYuv*    pcOrgYuv,
                                      TComYuv*    pcPredYuv)
{
    UInt  uiWidth     = pcCU->getWidth(0) >> 1;
    UInt  uiHeight    = pcCU->getHeight(0) >> 1;
    UInt  uiStride    = pcOrgYuv->getCStride();
    Pel*  piOrgU      = pcOrgYuv->getCbAddr(0);
    Pel*  piOrgV      = pcOrgYuv->getCrAddr(0);
    Pel*  piPredU     = pcPredYuv->getCbAddr(0);
    Pel*  piPredV     = pcPredYuv->getCrAddr(0);

    //===== init pattern =====
    Bool  bAboveAvail = false;
    Bool  bLeftAvail  = false;

    x265::pixelcmp sa8d;

    switch (uiWidth)
    {
    case 32:
        sa8d = x265::primitives.sa8d_32x32;
        break;
    case 16:
        sa8d = x265::primitives.sa8d_16x16;
        break;
    case 8:
        sa8d = x265::primitives.sa8d_8x8;
        break;
    case 4:
        sa8d = x265::primitives.satd[PARTITION_4x4];
        break;
    default:
        assert(!"invalid intra block width");
        sa8d = x265::primitives.sad[0];
        break;
    }

    assert(uiWidth == uiHeight);

    pcCU->getPattern()->initPattern(pcCU, 0, 0);
    pcCU->getPattern()->initAdiPatternChroma(pcCU, 0, 0, m_piPredBuf, m_iPredBufStride, m_iPredBufHeight, bAboveAvail, bLeftAvail);
    Pel*  pPatChromaU = pcCU->getPattern()->getAdiCbBuf(uiWidth, uiHeight, m_piPredBuf);
    Pel*  pPatChromaV = pcCU->getPattern()->getAdiCrBuf(uiWidth, uiHeight, m_piPredBuf);

    //===== get best prediction modes (using SAD) =====
    UInt  uiMinMode   = 0;
    UInt  uiMaxMode   = 4;
    UInt  uiBestMode  = MAX_UINT;
    UInt  uiMinSAD    = MAX_UINT;
    for (UInt uiMode  = uiMinMode; uiMode < uiMaxMode; uiMode++)
    {
        //--- get prediction ---
        predIntraChromaAng(pPatChromaU, uiMode, piPredU, uiStride, uiWidth, uiHeight, bAboveAvail, bLeftAvail);
        predIntraChromaAng(pPatChromaV, uiMode, piPredV, uiStride, uiWidth, uiHeight, bAboveAvail, bLeftAvail);

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
    pcCU->setChromIntraDirSubParts(uiBestMode, 0, pcCU->getDepth(0));
}

Void TEncSearch::estIntraPredQT(TComDataCU* pcCU,
                                TComYuv*    pcOrgYuv,
                                TComYuv*    pcPredYuv,
                                TShortYUV*  pcResiYuv,
                                TComYuv*    pcRecoYuv,
                                UInt&       ruiDistC,
                                Bool        bLumaOnly)
{
    UInt    uiDepth        = pcCU->getDepth(0);
    UInt    uiNumPU        = pcCU->getNumPartInter();
    UInt    uiInitTrDepth  = pcCU->getPartitionSize(0) == SIZE_2Nx2N ? 0 : 1;
    UInt    uiWidth        = pcCU->getWidth(0) >> uiInitTrDepth;
    UInt    uiHeight       = pcCU->getHeight(0) >> uiInitTrDepth;
    UInt    uiQNumParts    = pcCU->getTotalNumPart() >> 2;
    UInt    uiWidthBit     = pcCU->getIntraSizeIdx(0);
    UInt    uiOverallDistY = 0;
    UInt    uiOverallDistC = 0;
    UInt    CandNum;
    UInt64  CandCostList[FAST_UDI_MAX_RDMODE_NUM];

    x265::pixelcmp sa8d;

    // TODO: Use a table lookup here
    switch (uiWidth)
    {
    case 64:
        sa8d = x265::primitives.sa8d_64x64;
        break;
    case 32:
        sa8d = x265::primitives.sa8d_32x32;
        break;
    case 16:
        sa8d = x265::primitives.sa8d_16x16;
        break;
    case 8:
        sa8d = x265::primitives.sa8d_8x8;
        break;
    case 4:
        sa8d = x265::primitives.satd[PARTITION_4x4];
        break;
    default:
        assert(!"invalid intra block width");
        sa8d = x265::primitives.sad[0];
        break;
    }

    assert(uiWidth == uiHeight);

    //===== set QP and clear Cbf =====
    if (pcCU->getSlice()->getPPS()->getUseDQP() == true)
    {
        pcCU->setQPSubParts(pcCU->getQP(0), 0, uiDepth);
    }
    else
    {
        pcCU->setQPSubParts(pcCU->getSlice()->getSliceQp(), 0, uiDepth);
    }

    //===== loop over partitions =====
    UInt uiPartOffset = 0;
    for (UInt uiPU = 0; uiPU < uiNumPU; uiPU++, uiPartOffset += uiQNumParts)
    {
        //===== init pattern for luma prediction =====
        Bool bAboveAvail = false;
        Bool bLeftAvail  = false;
        pcCU->getPattern()->initPattern(pcCU, uiInitTrDepth, uiPartOffset);
        // Reference sample smoothing
        pcCU->getPattern()->initAdiPattern(pcCU, uiPartOffset, uiInitTrDepth, m_piPredBuf, m_iPredBufStride, m_iPredBufHeight, bAboveAvail, bLeftAvail, refAbove, refLeft, refAboveFlt, refLeftFlt);

        //===== determine set of modes to be tested (using prediction signal only) =====
        Int numModesAvailable     = 35; //total number of Intra modes
        Pel* piOrg         = pcOrgYuv->getLumaAddr(uiPU, uiWidth);
        Pel* piPred        = pcPredYuv->getLumaAddr(uiPU, uiWidth);
        UInt uiStride      = pcPredYuv->getStride();
        UInt uiRdModeList[FAST_UDI_MAX_RDMODE_NUM];
        Int numModesForFullRD = g_aucIntraModeNumFast[uiWidthBit];

        Bool doFastSearch = (numModesForFullRD != numModesAvailable);
        if (doFastSearch)
        {
            assert(numModesForFullRD < numModesAvailable);

            for (Int i = 0; i < numModesForFullRD; i++)
            {
                CandCostList[i] = MAX_INT64;
            }

            CandNum = 0;

            for (Int modeIdx = 0; modeIdx < numModesAvailable; modeIdx++)
            {
                UInt uiMode = modeIdx;

                predIntraLumaAng(pcCU->getPattern(), uiMode, piPred, uiStride, uiWidth, uiHeight, bAboveAvail, bLeftAvail);

                // use hadamard transform here
                UInt uiSad = sa8d((pixel*)piOrg, uiStride, (pixel*)piPred, uiStride);
                x265_emms();

                UInt   iModeBits = xModeBitsIntra(pcCU, uiMode, uiPU, uiPartOffset, uiDepth, uiInitTrDepth);
                UInt64 sqrtLambda = m_pcRdCost->getSqrtLambda()*256;
                UInt64 cost      = uiSad + ((iModeBits * sqrtLambda +128 )>>8);

                CandNum += xUpdateCandList(uiMode, cost, numModesForFullRD, uiRdModeList, CandCostList);    //Find N least cost  modes. N = numModesForFullRD
            }

            Int uiPreds[3] = { -1, -1, -1 };
            Int iMode = -1;
            Int numCand = pcCU->getIntraDirLumaPredictor(uiPartOffset, uiPreds, &iMode);
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

        //===== check modes (using r-d costs) =====
        UInt    uiBestPUMode  = 0;
        UInt    uiBestPUDistY = 0;
        UInt    uiBestPUDistC = 0;
        UInt64  dBestPUCost   = MAX_INT64;
        for (UInt uiMode = 0; uiMode < numModesForFullRD; uiMode++)
        {
            // set luma prediction mode
            UInt uiOrgMode = uiRdModeList[uiMode];

            pcCU->setLumaIntraDirSubParts(uiOrgMode, uiPartOffset, uiDepth + uiInitTrDepth);

            // set context models
            m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);

            // determine residual for partition
            UInt   uiPUDistY = 0;
            UInt   uiPUDistC = 0;
            UInt64 dPUCost   = 0.0;
            xRecurIntraCodingQT(pcCU, uiInitTrDepth, uiPartOffset, bLumaOnly, pcOrgYuv, pcPredYuv, pcResiYuv, uiPUDistY, uiPUDistC, true, dPUCost);

            // check r-d cost
            if (dPUCost < dBestPUCost)
            {
                uiBestPUMode  = uiOrgMode;
                uiBestPUDistY = uiPUDistY;
                uiBestPUDistC = uiPUDistC;
                dBestPUCost   = dPUCost;

                xSetIntraResultQT(pcCU, uiInitTrDepth, uiPartOffset, bLumaOnly, pcRecoYuv);

                UInt uiQPartNum = pcCU->getPic()->getNumPartInCU() >> ((pcCU->getDepth(0) + uiInitTrDepth) << 1);
                ::memcpy(m_puhQTTempTrIdx,  pcCU->getTransformIdx()       + uiPartOffset, uiQPartNum * sizeof(UChar));
                ::memcpy(m_puhQTTempCbf[0], pcCU->getCbf(TEXT_LUMA) + uiPartOffset, uiQPartNum * sizeof(UChar));
                ::memcpy(m_puhQTTempCbf[1], pcCU->getCbf(TEXT_CHROMA_U) + uiPartOffset, uiQPartNum * sizeof(UChar));
                ::memcpy(m_puhQTTempCbf[2], pcCU->getCbf(TEXT_CHROMA_V) + uiPartOffset, uiQPartNum * sizeof(UChar));
                ::memcpy(m_puhQTTempTransformSkipFlag[0], pcCU->getTransformSkip(TEXT_LUMA)     + uiPartOffset, uiQPartNum * sizeof(UChar));
                ::memcpy(m_puhQTTempTransformSkipFlag[1], pcCU->getTransformSkip(TEXT_CHROMA_U) + uiPartOffset, uiQPartNum * sizeof(UChar));
                ::memcpy(m_puhQTTempTransformSkipFlag[2], pcCU->getTransformSkip(TEXT_CHROMA_V) + uiPartOffset, uiQPartNum * sizeof(UChar));
            }
        } // Mode loop

        {
            UInt uiOrgMode = uiBestPUMode;

            pcCU->setLumaIntraDirSubParts(uiOrgMode, uiPartOffset, uiDepth + uiInitTrDepth);

            // set context models
            m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);

            // determine residual for partition
            UInt   uiPUDistY = 0;
            UInt   uiPUDistC = 0;
            UInt64 dPUCost   = 0;
            xRecurIntraCodingQT(pcCU, uiInitTrDepth, uiPartOffset, bLumaOnly, pcOrgYuv, pcPredYuv, pcResiYuv, uiPUDistY, uiPUDistC, false, dPUCost);

            // check r-d cost
            if (dPUCost < dBestPUCost)
            {
                uiBestPUMode  = uiOrgMode;
                uiBestPUDistY = uiPUDistY;
                uiBestPUDistC = uiPUDistC;
                dBestPUCost   = dPUCost;

                xSetIntraResultQT(pcCU, uiInitTrDepth, uiPartOffset, bLumaOnly, pcRecoYuv);

                UInt uiQPartNum = pcCU->getPic()->getNumPartInCU() >> ((pcCU->getDepth(0) + uiInitTrDepth) << 1);
                ::memcpy(m_puhQTTempTrIdx,  pcCU->getTransformIdx()       + uiPartOffset, uiQPartNum * sizeof(UChar));
                ::memcpy(m_puhQTTempCbf[0], pcCU->getCbf(TEXT_LUMA) + uiPartOffset, uiQPartNum * sizeof(UChar));
                ::memcpy(m_puhQTTempCbf[1], pcCU->getCbf(TEXT_CHROMA_U) + uiPartOffset, uiQPartNum * sizeof(UChar));
                ::memcpy(m_puhQTTempCbf[2], pcCU->getCbf(TEXT_CHROMA_V) + uiPartOffset, uiQPartNum * sizeof(UChar));
                ::memcpy(m_puhQTTempTransformSkipFlag[0], pcCU->getTransformSkip(TEXT_LUMA)     + uiPartOffset, uiQPartNum * sizeof(UChar));
                ::memcpy(m_puhQTTempTransformSkipFlag[1], pcCU->getTransformSkip(TEXT_CHROMA_U) + uiPartOffset, uiQPartNum * sizeof(UChar));
                ::memcpy(m_puhQTTempTransformSkipFlag[2], pcCU->getTransformSkip(TEXT_CHROMA_V) + uiPartOffset, uiQPartNum * sizeof(UChar));
            }
        } // Mode loop

        //--- update overall distortion ---
        uiOverallDistY += uiBestPUDistY;
        uiOverallDistC += uiBestPUDistC;

        //--- update transform index and cbf ---
        UInt uiQPartNum = pcCU->getPic()->getNumPartInCU() >> ((pcCU->getDepth(0) + uiInitTrDepth) << 1);
        ::memcpy(pcCU->getTransformIdx()       + uiPartOffset, m_puhQTTempTrIdx,  uiQPartNum * sizeof(UChar));
        ::memcpy(pcCU->getCbf(TEXT_LUMA) + uiPartOffset, m_puhQTTempCbf[0], uiQPartNum * sizeof(UChar));
        ::memcpy(pcCU->getCbf(TEXT_CHROMA_U) + uiPartOffset, m_puhQTTempCbf[1], uiQPartNum * sizeof(UChar));
        ::memcpy(pcCU->getCbf(TEXT_CHROMA_V) + uiPartOffset, m_puhQTTempCbf[2], uiQPartNum * sizeof(UChar));
        ::memcpy(pcCU->getTransformSkip(TEXT_LUMA)     + uiPartOffset, m_puhQTTempTransformSkipFlag[0], uiQPartNum * sizeof(UChar));
        ::memcpy(pcCU->getTransformSkip(TEXT_CHROMA_U) + uiPartOffset, m_puhQTTempTransformSkipFlag[1], uiQPartNum * sizeof(UChar));
        ::memcpy(pcCU->getTransformSkip(TEXT_CHROMA_V) + uiPartOffset, m_puhQTTempTransformSkipFlag[2], uiQPartNum * sizeof(UChar));
        //--- set reconstruction for next intra prediction blocks ---
        if (uiPU != uiNumPU - 1)
        {
            Bool bSkipChroma  = false;
            Bool bChromaSame  = false;
            UInt uiLog2TrSize = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth() >> (pcCU->getDepth(0) + uiInitTrDepth)] + 2;
            if (!bLumaOnly && uiLog2TrSize == 2)
            {
                assert(uiInitTrDepth  > 0);
                bSkipChroma  = (uiPU != 0);
                bChromaSame  = true;
            }

            UInt    uiCompWidth   = pcCU->getWidth(0) >> uiInitTrDepth;
            UInt    uiCompHeight  = pcCU->getHeight(0) >> uiInitTrDepth;
            UInt    uiZOrder      = pcCU->getZorderIdxInCU() + uiPartOffset;
            Pel*    piDes         = pcCU->getPic()->getPicYuvRec()->getLumaAddr(pcCU->getAddr(), uiZOrder);
            UInt    uiDesStride   = pcCU->getPic()->getPicYuvRec()->getStride();
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
                piDes         = pcCU->getPic()->getPicYuvRec()->getCbAddr(pcCU->getAddr(), uiZOrder);
                uiDesStride   = pcCU->getPic()->getPicYuvRec()->getCStride();
                piSrc         = pcRecoYuv->getCbAddr(uiPartOffset);
                uiSrcStride   = pcRecoYuv->getCStride();
                for (UInt uiY = 0; uiY < uiCompHeight; uiY++, piSrc += uiSrcStride, piDes += uiDesStride)
                {
                    for (UInt uiX = 0; uiX < uiCompWidth; uiX++)
                    {
                        piDes[uiX] = piSrc[uiX];
                    }
                }

                piDes         = pcCU->getPic()->getPicYuvRec()->getCrAddr(pcCU->getAddr(), uiZOrder);
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
        pcCU->setLumaIntraDirSubParts(uiBestPUMode, uiPartOffset, uiDepth + uiInitTrDepth);
        pcCU->copyToPic(uiDepth, uiPU, uiInitTrDepth);
    } // PU loop

    if (uiNumPU > 1)
    { // set Cbf for all blocks
        UInt uiCombCbfY = 0;
        UInt uiCombCbfU = 0;
        UInt uiCombCbfV = 0;
        UInt uiPartIdx  = 0;
        for (UInt uiPart = 0; uiPart < 4; uiPart++, uiPartIdx += uiQNumParts)
        {
            uiCombCbfY |= pcCU->getCbf(uiPartIdx, TEXT_LUMA,     1);
            uiCombCbfU |= pcCU->getCbf(uiPartIdx, TEXT_CHROMA_U, 1);
            uiCombCbfV |= pcCU->getCbf(uiPartIdx, TEXT_CHROMA_V, 1);
        }

        for (UInt uiOffs = 0; uiOffs < 4 * uiQNumParts; uiOffs++)
        {
            pcCU->getCbf(TEXT_LUMA)[uiOffs] |= uiCombCbfY;
            pcCU->getCbf(TEXT_CHROMA_U)[uiOffs] |= uiCombCbfU;
            pcCU->getCbf(TEXT_CHROMA_V)[uiOffs] |= uiCombCbfV;
        }
    }

    //===== reset context models =====
    m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);

    //===== set distortion (rate and r-d costs are determined later) =====
    ruiDistC                   = uiOverallDistC;
    pcCU->getTotalDistortion() = uiOverallDistY + uiOverallDistC;
}

Void TEncSearch::estIntraPredChromaQT(TComDataCU* pcCU,
                                      TComYuv*    pcOrgYuv,
                                      TComYuv*    pcPredYuv,
                                      TShortYUV*  pcResiYuv,
                                      TComYuv*    pcRecoYuv,
                                      UInt        uiPreCalcDistC)
{
    UInt    uiDepth     = pcCU->getDepth(0);
    UInt    uiBestMode  = 0;
    UInt    uiBestDist  = 0;
    UInt64  dBestCost   = MAX_INT64;

    //----- init mode list -----
    UInt  uiMinMode = 0;
    UInt  uiModeList[NUM_CHROMA_MODE];

    pcCU->getAllowedChromaDir(0, uiModeList);
    UInt  uiMaxMode = NUM_CHROMA_MODE;

    //----- check chroma modes -----
    for (UInt uiMode = uiMinMode; uiMode < uiMaxMode; uiMode++)
    {
        //----- restore context models -----
        m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);

        //----- chroma coding -----
        UInt    uiDist = 0;
        pcCU->setChromIntraDirSubParts(uiModeList[uiMode], 0, uiDepth);
        xRecurIntraChromaCodingQT(pcCU,   0, 0, pcOrgYuv, pcPredYuv, pcResiYuv, uiDist);
        if (pcCU->getSlice()->getPPS()->getUseTransformSkip())
        {
            m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);
        }

        UInt    uiBits = xGetIntraBitsQT(pcCU,   0, 0, false, true, false);
        UInt64  dCost  = m_pcRdCost->calcRdCost(uiDist, uiBits);

        //----- compare -----
        if (dCost < dBestCost)
        {
            dBestCost   = dCost;
            uiBestDist  = uiDist;
            uiBestMode  = uiModeList[uiMode];
            UInt  uiQPN = pcCU->getPic()->getNumPartInCU() >> (uiDepth << 1);
            xSetIntraResultChromaQT(pcCU, 0, 0, pcRecoYuv);
            ::memcpy(m_puhQTTempCbf[1], pcCU->getCbf(TEXT_CHROMA_U), uiQPN * sizeof(UChar));
            ::memcpy(m_puhQTTempCbf[2], pcCU->getCbf(TEXT_CHROMA_V), uiQPN * sizeof(UChar));
            ::memcpy(m_puhQTTempTransformSkipFlag[1], pcCU->getTransformSkip(TEXT_CHROMA_U), uiQPN * sizeof(UChar));
            ::memcpy(m_puhQTTempTransformSkipFlag[2], pcCU->getTransformSkip(TEXT_CHROMA_V), uiQPN * sizeof(UChar));
        }
    }

    //----- set data -----
    UInt  uiQPN = pcCU->getPic()->getNumPartInCU() >> (uiDepth << 1);
    ::memcpy(pcCU->getCbf(TEXT_CHROMA_U), m_puhQTTempCbf[1], uiQPN * sizeof(UChar));
    ::memcpy(pcCU->getCbf(TEXT_CHROMA_V), m_puhQTTempCbf[2], uiQPN * sizeof(UChar));
    ::memcpy(pcCU->getTransformSkip(TEXT_CHROMA_U), m_puhQTTempTransformSkipFlag[1], uiQPN * sizeof(UChar));
    ::memcpy(pcCU->getTransformSkip(TEXT_CHROMA_V), m_puhQTTempTransformSkipFlag[2], uiQPN * sizeof(UChar));
    pcCU->setChromIntraDirSubParts(uiBestMode, 0, uiDepth);
    pcCU->getTotalDistortion() += uiBestDist - uiPreCalcDistC;

    //----- restore context models -----
    m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);
}

/** Function for encoding and reconstructing luma/chroma samples of a PCM mode CU.
 * \param pcCU pointer to current CU
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
Void TEncSearch::xEncPCM(TComDataCU* pcCU, UInt uiAbsPartIdx, Pel* piOrg, Pel* piPCM, Pel* piPred, Short* piResi, Pel* piReco, UInt uiStride, UInt uiWidth, UInt uiHeight, TextType eText)
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
        uiReconStride = pcCU->getPic()->getPicYuvRec()->getStride();
        pRecoPic      = pcCU->getPic()->getPicYuvRec()->getLumaAddr(pcCU->getAddr(), pcCU->getZorderIdxInCU() + uiAbsPartIdx);
        shiftPcm = g_bitDepthY - pcCU->getSlice()->getSPS()->getPCMBitDepthLuma();
    }
    else
    {
        uiReconStride = pcCU->getPic()->getPicYuvRec()->getCStride();

        if (eText == TEXT_CHROMA_U)
        {
            pRecoPic = pcCU->getPic()->getPicYuvRec()->getCbAddr(pcCU->getAddr(), pcCU->getZorderIdxInCU() + uiAbsPartIdx);
        }
        else
        {
            pRecoPic = pcCU->getPic()->getPicYuvRec()->getCrAddr(pcCU->getAddr(), pcCU->getZorderIdxInCU() + uiAbsPartIdx);
        }
        shiftPcm = g_bitDepthC - pcCU->getSlice()->getSPS()->getPCMBitDepthChroma();
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
 * \param pcCU
 * \param pcOrgYuv
 * \param rpcPredYuv
 * \param rpcResiYuv
 * \param rpcRecoYuv
 * \returns Void
 */
Void TEncSearch::IPCMSearch(TComDataCU* pcCU, TComYuv* pcOrgYuv, TComYuv*& rpcPredYuv, TShortYUV*& rpcResiYuv, TComYuv*& rpcRecoYuv)
{
    UInt   uiDepth        = pcCU->getDepth(0);
    UInt   uiWidth        = pcCU->getWidth(0);
    UInt   uiHeight       = pcCU->getHeight(0);
    UInt   uiStride       = rpcPredYuv->getStride();
    UInt   uiStrideC      = rpcPredYuv->getCStride();
    UInt   uiWidthC       = uiWidth  >> 1;
    UInt   uiHeightC      = uiHeight >> 1;
    UInt   uiDistortion = 0;
    UInt   uiBits;

    Double dCost;

    Pel*    pOrig;
    Short*    pResi;
    Pel*    pReco;
    Pel*    pPred;
    Pel*    pPCM;

    UInt uiAbsPartIdx = 0;

    UInt uiMinCoeffSize = pcCU->getPic()->getMinCUWidth() * pcCU->getPic()->getMinCUHeight();
    UInt uiLumaOffset   = uiMinCoeffSize * uiAbsPartIdx;
    UInt uiChromaOffset = uiLumaOffset >> 2;

    // Luminance
    pOrig    = pcOrgYuv->getLumaAddr(0, uiWidth);
    pResi    = rpcResiYuv->getLumaAddr(0, uiWidth);
    pPred    = rpcPredYuv->getLumaAddr(0, uiWidth);
    pReco    = rpcRecoYuv->getLumaAddr(0, uiWidth);
    pPCM     = pcCU->getPCMSampleY() + uiLumaOffset;

    xEncPCM(pcCU, 0, pOrig, pPCM, pPred, pResi, pReco, uiStride, uiWidth, uiHeight, TEXT_LUMA);

    // Chroma U
    pOrig    = pcOrgYuv->getCbAddr();
    pResi    = rpcResiYuv->getCbAddr();
    pPred    = rpcPredYuv->getCbAddr();
    pReco    = rpcRecoYuv->getCbAddr();
    pPCM     = pcCU->getPCMSampleCb() + uiChromaOffset;

    xEncPCM(pcCU, 0, pOrig, pPCM, pPred, pResi, pReco, uiStrideC, uiWidthC, uiHeightC, TEXT_CHROMA_U);

    // Chroma V
    pOrig    = pcOrgYuv->getCrAddr();
    pResi    = rpcResiYuv->getCrAddr();
    pPred    = rpcPredYuv->getCrAddr();
    pReco    = rpcRecoYuv->getCrAddr();
    pPCM     = pcCU->getPCMSampleCr() + uiChromaOffset;

    xEncPCM(pcCU, 0, pOrig, pPCM, pPred, pResi, pReco, uiStrideC, uiWidthC, uiHeightC, TEXT_CHROMA_V);

    m_pcEntropyCoder->resetBits();
    xEncIntraHeader(pcCU, uiDepth, uiAbsPartIdx, true, false);
    uiBits = m_pcEntropyCoder->getNumberOfWrittenBits();

    dCost = m_pcRdCost->calcRdCost(uiDistortion, uiBits);

    m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);

    pcCU->getTotalBits()       = uiBits;
    pcCU->getTotalCost()       = dCost;
    pcCU->getTotalDistortion() = uiDistortion;

    pcCU->copyToPic(uiDepth, 0, 0);
}

Void TEncSearch::xGetInterPredictionError(TComDataCU* pcCU, TComYuv* pcYuvOrg, Int iPartIdx, UInt& ruiErr, Bool bHadamard)
{
    motionCompensation(pcCU, &m_tmpYuvPred, REF_PIC_LIST_X, iPartIdx);
    UInt uiAbsPartIdx = 0;
    Int iWidth = 0;
    Int iHeight = 0;
    pcCU->getPartIndexAndSize(iPartIdx, uiAbsPartIdx, iWidth, iHeight);

    DistParam cDistParam;

    cDistParam.bApplyWeight = false;

    m_pcRdCost->setDistParam(cDistParam, g_bitDepthY,
                             pcYuvOrg->getLumaAddr(uiAbsPartIdx), pcYuvOrg->getStride(),
                             m_tmpYuvPred.getLumaAddr(uiAbsPartIdx), m_tmpYuvPred.getStride(),
                             iWidth, iHeight, m_pcEncCfg->getUseHADME());
    ruiErr = cDistParam.DistFunc(&cDistParam);
}

/** estimation of best merge coding
 * \param pcCU
 * \param pcYuvOrg
 * \param iPUIdx
 * \param uiInterDir
 * \param pacMvField
 * \param uiMergeIndex
 * \param ruiCost
 * \param ruiBits
 * \param puhNeighCands
 * \param bValid
 * \returns Void
 */
Void TEncSearch::xMergeEstimation(TComDataCU* pcCU, TComYuv* pcYuvOrg, Int iPUIdx, UInt& uiInterDir, TComMvField* pacMvField, UInt& uiMergeIndex, UInt& ruiCost, TComMvField* cMvFieldNeighbours, UChar* uhInterDirNeighbours, Int& numValidMergeCand)
{
    UInt uiAbsPartIdx = 0;
    Int iWidth = 0;
    Int iHeight = 0;

    pcCU->getPartIndexAndSize(iPUIdx, uiAbsPartIdx, iWidth, iHeight);
    UInt uiDepth = pcCU->getDepth(uiAbsPartIdx);
    PartSize partSize = pcCU->getPartitionSize(0);
    if (pcCU->getSlice()->getPPS()->getLog2ParallelMergeLevelMinus2() && partSize != SIZE_2Nx2N && pcCU->getWidth(0) <= 8)
    {
        pcCU->setPartSizeSubParts(SIZE_2Nx2N, 0, uiDepth);
        if (iPUIdx == 0)
        {
            pcCU->getInterMergeCandidates(0, 0, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand);
        }
        pcCU->setPartSizeSubParts(partSize, 0, uiDepth);
    }
    else
    {
        pcCU->getInterMergeCandidates(uiAbsPartIdx, iPUIdx, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand);
    }
    xRestrictBipredMergeCand(pcCU, iPUIdx, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand);

    ruiCost = MAX_UINT;
    for (UInt uiMergeCand = 0; uiMergeCand < numValidMergeCand; ++uiMergeCand)
    {
        {
            UInt uiCostCand = MAX_UINT;
            UInt uiBitsCand = 0;

            PartSize ePartSize = pcCU->getPartitionSize(0);

            pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvField(cMvFieldNeighbours[0 + 2 * uiMergeCand], ePartSize, uiAbsPartIdx, 0, iPUIdx);
            pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvField(cMvFieldNeighbours[1 + 2 * uiMergeCand], ePartSize, uiAbsPartIdx, 0, iPUIdx);

            xGetInterPredictionError(pcCU, pcYuvOrg, iPUIdx, uiCostCand, m_pcEncCfg->getUseHADME());
            uiBitsCand = uiMergeCand + 1;
            if (uiMergeCand == m_pcEncCfg->getMaxNumMergeCand() - 1)
            {
                uiBitsCand--;
            }
            uiCostCand = uiCostCand + m_pcRdCost->getCost(uiBitsCand);
            if (uiCostCand < ruiCost)
            {
                ruiCost = uiCostCand;
                pacMvField[0] = cMvFieldNeighbours[0 + 2 * uiMergeCand];
                pacMvField[1] = cMvFieldNeighbours[1 + 2 * uiMergeCand];
                uiInterDir = uhInterDirNeighbours[uiMergeCand];
                uiMergeIndex = uiMergeCand;
            }
        }
    }
}

/** convert bi-pred merge candidates to uni-pred
 * \param pcCU
 * \param puIdx
 * \param mvFieldNeighbours
 * \param interDirNeighbours
 * \param numValidMergeCand
 * \returns Void
 */
Void TEncSearch::xRestrictBipredMergeCand(TComDataCU* pcCU, UInt puIdx, TComMvField* mvFieldNeighbours, UChar* interDirNeighbours, Int numValidMergeCand)
{
    if (pcCU->isBipredRestriction(puIdx))
    {
        for (UInt mergeCand = 0; mergeCand < numValidMergeCand; ++mergeCand)
        {
            if (interDirNeighbours[mergeCand] == 3)
            {
                interDirNeighbours[mergeCand] = 1;
                mvFieldNeighbours[(mergeCand << 1) + 1].setMvField(TComMv(0, 0), -1);
            }
        }
    }
}

/** search of the best candidate for inter prediction
 * \param pcCU
 * \param pcOrgYuv
 * \param rpcPredYuv
 * \param rpcResiYuv
 * \param rpcRecoYuv
 * \param bUseRes
 * \returns Void
 */
Void TEncSearch::predInterSearch(TComDataCU* pcCU, TComYuv* pcOrgYuv, TComYuv*& rpcPredYuv, Bool bUseMRG)
{
    m_acYuvPred[0].clear();
    m_acYuvPred[1].clear();
    m_cYuvPredTemp.clear();
    rpcPredYuv->clear();

    TComMv        cMvSrchRngLT;
    TComMv        cMvSrchRngRB;

    TComMv        cMvZero;
    TComMv        TempMv; //kolya

    TComMv        cMv[2];
    TComMv        cMvBi[2];
    TComMv        cMvTemp[2][33];

    Int           iNumPart    = pcCU->getNumPartInter();
    Int           iNumPredDir = pcCU->getSlice()->isInterP() ? 1 : 2;

    TComMv        cMvPred[2][33];

    TComMv        cMvPredBi[2][33];
    Int           aaiMvpIdxBi[2][33];

    Int           aaiMvpIdx[2][33];
    Int           aaiMvpNum[2][33];

    AMVPInfo aacAMVPInfo[2][33];

    Int           iRefIdx[2] = { 0, 0 }; //If un-initialized, may cause SEGV in bi-directional prediction iterative stage.
    Int           iRefIdxBi[2];

    UInt          uiPartAddr;
    Int           iRoiWidth, iRoiHeight;

    UInt          uiMbBits[3] = { 1, 1, 0 };

    UInt          uiLastMode = 0;
    Int           iRefStart, iRefEnd;

    PartSize      ePartSize = pcCU->getPartitionSize(0);

    Int           bestBiPRefIdxL1 = 0;
    Int           bestBiPMvpL1 = 0;
    UInt          biPDistTemp = MAX_INT;

    /* TODO: this could be as high as TEncSlice::compressSlice() */
    TComPicYuv *fenc = pcCU->getSlice()->getPic()->getPicYuvOrg();
    m_me.setSourcePlane((pixel*)fenc->getLumaAddr(), fenc->getStride());

    TComMvField cMvFieldNeighbours[MRG_MAX_NUM_CANDS << 1]; // double length for mv of both lists
    UChar uhInterDirNeighbours[MRG_MAX_NUM_CANDS];
    Int numValidMergeCand = 0;
#if FAST_MODE_DECISION
    pcCU->getTotalCost() = 0;
#endif
    for (Int iPartIdx = 0; iPartIdx < iNumPart; iPartIdx++)
    {
        UInt          uiCost[2] = { MAX_UINT, MAX_UINT };
        UInt          uiCostBi  = MAX_UINT;
        UInt          uiCostTemp;

        UInt          uiBits[3];
        UInt          uiBitsTemp;
        UInt          bestBiPDist = MAX_INT;

        UInt          uiCostTempL0[MAX_NUM_REF];
        for (Int iNumRef = 0; iNumRef < MAX_NUM_REF; iNumRef++)
        {
            uiCostTempL0[iNumRef] = MAX_UINT;
        }

        UInt          uiBitsTempL0[MAX_NUM_REF];

        TComMv        mvValidList1;
        Int           refIdxValidList1 = 0;
        UInt          bitsValidList1 = MAX_UINT;
        UInt          costValidList1 = MAX_UINT;

        xGetBlkBits(ePartSize, pcCU->getSlice()->isInterP(), iPartIdx, uiLastMode, uiMbBits);

        pcCU->getPartIndexAndSize(iPartIdx, uiPartAddr, iRoiWidth, iRoiHeight);

        Pel* PU = fenc->getLumaAddr(pcCU->getAddr(), pcCU->getZorderIdxInCU() + uiPartAddr);
        m_me.setSourcePU(PU - fenc->getLumaAddr(), iRoiWidth, iRoiHeight);
        m_me.setQP(pcCU->getQP(0), m_pcRdCost->getSqrtLambda());

        Bool bTestNormalMC = true;

        if (bUseMRG && pcCU->getWidth(0) > 8 && iNumPart == 2)
        {
            bTestNormalMC = false;
        }

        if (bTestNormalMC)
        {
            //  Uni-directional prediction
            for (Int iRefList = 0; iRefList < iNumPredDir; iRefList++)
            {
                RefPicList  eRefPicList = (iRefList ? REF_PIC_LIST_1 : REF_PIC_LIST_0);

                for (Int iRefIdxTemp = 0; iRefIdxTemp < pcCU->getSlice()->getNumRefIdx(eRefPicList); iRefIdxTemp++)
                {
                    uiBitsTemp = uiMbBits[iRefList];
                    if (pcCU->getSlice()->getNumRefIdx(eRefPicList) > 1)
                    {
                        uiBitsTemp += iRefIdxTemp + 1;
                        if (iRefIdxTemp == pcCU->getSlice()->getNumRefIdx(eRefPicList) - 1) uiBitsTemp--;
                    }
                    xEstimateMvPredAMVP(pcCU, pcOrgYuv, iPartIdx, eRefPicList, iRefIdxTemp, cMvPred[iRefList][iRefIdxTemp], false, &biPDistTemp);
                    aaiMvpIdx[iRefList][iRefIdxTemp] = pcCU->getMVPIdx(eRefPicList, uiPartAddr);
                    aaiMvpNum[iRefList][iRefIdxTemp] = pcCU->getMVPNum(eRefPicList, uiPartAddr);

                    if (pcCU->getSlice()->getMvdL1ZeroFlag() && iRefList == 1 && biPDistTemp < bestBiPDist)
                    {
                        bestBiPDist = biPDistTemp;
                        bestBiPMvpL1 = aaiMvpIdx[iRefList][iRefIdxTemp];
                        bestBiPRefIdxL1 = iRefIdxTemp;
                    }

                    uiBitsTemp += m_auiMVPIdxCost[aaiMvpIdx[iRefList][iRefIdxTemp]][AMVP_MAX_NUM_CANDS];

                    if (iRefList == 1) // list 1
                    {
                        if (pcCU->getSlice()->getList1IdxToList0Idx(iRefIdxTemp) >= 0)
                        {
                            cMvTemp[1][iRefIdxTemp] = cMvTemp[0][pcCU->getSlice()->getList1IdxToList0Idx(iRefIdxTemp)];
                            uiCostTemp = uiCostTempL0[pcCU->getSlice()->getList1IdxToList0Idx(iRefIdxTemp)];

                            /* first subtract the bit-rate part of the cost of the other list */
                            uiCostTemp -= m_pcRdCost->getCost(uiBitsTempL0[pcCU->getSlice()->getList1IdxToList0Idx(iRefIdxTemp)]);

                            /* correct the bit-rate part of the current ref */
                            m_me.setMVP(cMvPred[iRefList][iRefIdxTemp]);
                            uiBitsTemp += m_me.bitcost(x265::MV(cMvTemp[1][iRefIdxTemp].getHor(), cMvTemp[1][iRefIdxTemp].getVer()));

                            /* calculate the correct cost */
                            uiCostTemp += m_pcRdCost->getCost(uiBitsTemp);
                        }
                        else
                        {
                            xMotionEstimation(pcCU, pcOrgYuv, iPartIdx, eRefPicList, &cMvPred[iRefList][iRefIdxTemp], iRefIdxTemp, cMvTemp[iRefList][iRefIdxTemp], uiBitsTemp, uiCostTemp);
                        }
                    }
                    else
                    {
                        xMotionEstimation(pcCU, pcOrgYuv, iPartIdx, eRefPicList, &cMvPred[iRefList][iRefIdxTemp], iRefIdxTemp, cMvTemp[iRefList][iRefIdxTemp], uiBitsTemp, uiCostTemp);
                    }
                    xCopyAMVPInfo(pcCU->getCUMvField(eRefPicList)->getAMVPInfo(), &aacAMVPInfo[iRefList][iRefIdxTemp]); // must always be done ( also when AMVP_MODE = AM_NONE )
                    xCheckBestMVP(pcCU, eRefPicList, cMvTemp[iRefList][iRefIdxTemp], cMvPred[iRefList][iRefIdxTemp], aaiMvpIdx[iRefList][iRefIdxTemp], uiBitsTemp, uiCostTemp);

                    if (iRefList == 0)
                    {
                        uiCostTempL0[iRefIdxTemp] = uiCostTemp;
                        uiBitsTempL0[iRefIdxTemp] = uiBitsTemp;
                    }
                    if (uiCostTemp < uiCost[iRefList])
                    {
                        uiCost[iRefList] = uiCostTemp;
                        uiBits[iRefList] = uiBitsTemp; // storing for bi-prediction

                        // set motion
                        cMv[iRefList]     = cMvTemp[iRefList][iRefIdxTemp];
                        iRefIdx[iRefList] = iRefIdxTemp;
                    }

                    if (iRefList == 1 && uiCostTemp < costValidList1 && pcCU->getSlice()->getList1IdxToList0Idx(iRefIdxTemp) < 0)
                    {
                        costValidList1 = uiCostTemp;
                        bitsValidList1 = uiBitsTemp;

                        // set motion
                        mvValidList1     = cMvTemp[iRefList][iRefIdxTemp];
                        refIdxValidList1 = iRefIdxTemp;
                    }
                }
            }

            //  Bi-directional prediction
            if ((pcCU->getSlice()->isInterB()) && (pcCU->isBipredRestriction(iPartIdx) == false))
            {
                cMvBi[0] = cMv[0];
                cMvBi[1] = cMv[1];
                iRefIdxBi[0] = iRefIdx[0];
                iRefIdxBi[1] = iRefIdx[1];

                ::memcpy(cMvPredBi, cMvPred, sizeof(cMvPred));
                ::memcpy(aaiMvpIdxBi, aaiMvpIdx, sizeof(aaiMvpIdx));

                UInt uiMotBits[2];

                if (pcCU->getSlice()->getMvdL1ZeroFlag())
                {
                    xCopyAMVPInfo(&aacAMVPInfo[1][bestBiPRefIdxL1], pcCU->getCUMvField(REF_PIC_LIST_1)->getAMVPInfo());
                    pcCU->setMVPIdxSubParts(bestBiPMvpL1, REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
                    aaiMvpIdxBi[1][bestBiPRefIdxL1] = bestBiPMvpL1;
                    cMvPredBi[1][bestBiPRefIdxL1]   = pcCU->getCUMvField(REF_PIC_LIST_1)->getAMVPInfo()->m_acMvCand[bestBiPMvpL1];

                    cMvBi[1] = cMvPredBi[1][bestBiPRefIdxL1];
                    iRefIdxBi[1] = bestBiPRefIdxL1;
                    pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMv(cMvBi[1], ePartSize, uiPartAddr, 0, iPartIdx);
                    pcCU->getCUMvField(REF_PIC_LIST_1)->setAllRefIdx(iRefIdxBi[1], ePartSize, uiPartAddr, 0, iPartIdx);
                    TComYuv* pcYuvPred = &m_acYuvPred[1];
                    motionCompensation(pcCU, pcYuvPred, REF_PIC_LIST_1, iPartIdx);

                    uiMotBits[0] = uiBits[0] - uiMbBits[0];
                    uiMotBits[1] = uiMbBits[1];

                    if (pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_1) > 1)
                    {
                        uiMotBits[1] += bestBiPRefIdxL1 + 1;
                        if (bestBiPRefIdxL1 == pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_1) - 1) uiMotBits[1]--;
                    }

                    uiMotBits[1] += m_auiMVPIdxCost[aaiMvpIdxBi[1][bestBiPRefIdxL1]][AMVP_MAX_NUM_CANDS];

                    uiBits[2] = uiMbBits[2] + uiMotBits[0] + uiMotBits[1];

                    cMvTemp[1][bestBiPRefIdxL1] = cMvBi[1];
                }
                else
                {
                    uiMotBits[0] = uiBits[0] - uiMbBits[0];
                    uiMotBits[1] = uiBits[1] - uiMbBits[1];
                    uiBits[2] = uiMbBits[2] + uiMotBits[0] + uiMotBits[1];
                }

                // 4-times iteration (default)
                Int iNumIter = 4;

                // fast encoder setting: only one iteration
                if (1 || pcCU->getSlice()->getMvdL1ZeroFlag())
                {
                    iNumIter = 1;
                }

                for (Int iIter = 0; iIter < iNumIter; iIter++)
                {
                    Int         iRefList    = iIter % 2;
                    if (1) // fast encode
                    {
                        if (uiCost[0] <= uiCost[1])
                        {
                            iRefList = 1;
                        }
                        else
                        {
                            iRefList = 0;
                        }
                    }
                    else if (iIter == 0)
                    {
                        iRefList = 0;
                    }
                    if (iIter == 0 && !pcCU->getSlice()->getMvdL1ZeroFlag())
                    {
                        pcCU->getCUMvField(RefPicList(1 - iRefList))->setAllMv(cMv[1 - iRefList], ePartSize, uiPartAddr, 0, iPartIdx);
                        pcCU->getCUMvField(RefPicList(1 - iRefList))->setAllRefIdx(iRefIdx[1 - iRefList], ePartSize, uiPartAddr, 0, iPartIdx);
                        TComYuv*  pcYuvPred = &m_acYuvPred[1 - iRefList];
                        motionCompensation(pcCU, pcYuvPred, RefPicList(1 - iRefList), iPartIdx);
                    }
                    RefPicList  eRefPicList = (iRefList ? REF_PIC_LIST_1 : REF_PIC_LIST_0);

                    if (pcCU->getSlice()->getMvdL1ZeroFlag())
                    {
                        iRefList = 0;
                        eRefPicList = REF_PIC_LIST_0;
                    }

                    Bool bChanged = false;

                    iRefStart = 0;
                    iRefEnd   = pcCU->getSlice()->getNumRefIdx(eRefPicList) - 1;

                    for (Int iRefIdxTemp = iRefStart; iRefIdxTemp <= iRefEnd; iRefIdxTemp++)
                    {
                        uiBitsTemp = uiMbBits[2] + uiMotBits[1 - iRefList];
                        if (pcCU->getSlice()->getNumRefIdx(eRefPicList) > 1)
                        {
                            uiBitsTemp += iRefIdxTemp + 1;
                            if (iRefIdxTemp == pcCU->getSlice()->getNumRefIdx(eRefPicList) - 1) uiBitsTemp--;
                        }
                        uiBitsTemp += m_auiMVPIdxCost[aaiMvpIdxBi[iRefList][iRefIdxTemp]][AMVP_MAX_NUM_CANDS];
                        // call ME
                        xMotionEstimation(pcCU, pcOrgYuv, iPartIdx, eRefPicList, &cMvPredBi[iRefList][iRefIdxTemp], iRefIdxTemp, cMvTemp[iRefList][iRefIdxTemp], uiBitsTemp, uiCostTemp, true);
                        xCopyAMVPInfo(&aacAMVPInfo[iRefList][iRefIdxTemp], pcCU->getCUMvField(eRefPicList)->getAMVPInfo());
                        xCheckBestMVP(pcCU, eRefPicList, cMvTemp[iRefList][iRefIdxTemp], cMvPredBi[iRefList][iRefIdxTemp], aaiMvpIdxBi[iRefList][iRefIdxTemp], uiBitsTemp, uiCostTemp);

                        if (uiCostTemp < uiCostBi)
                        {
                            bChanged = true;

                            cMvBi[iRefList]     = cMvTemp[iRefList][iRefIdxTemp];
                            iRefIdxBi[iRefList] = iRefIdxTemp;

                            uiCostBi            = uiCostTemp;
                            uiMotBits[iRefList] = uiBitsTemp - uiMbBits[2] - uiMotBits[1 - iRefList];
                            uiBits[2]           = uiBitsTemp;

                            if (iNumIter != 1)
                            {
                                //  Set motion
                                pcCU->getCUMvField(eRefPicList)->setAllMv(cMvBi[iRefList], ePartSize, uiPartAddr, 0, iPartIdx);
                                pcCU->getCUMvField(eRefPicList)->setAllRefIdx(iRefIdxBi[iRefList], ePartSize, uiPartAddr, 0, iPartIdx);

                                TComYuv* pcYuvPred = &m_acYuvPred[iRefList];
                                motionCompensation(pcCU, pcYuvPred, eRefPicList, iPartIdx);
                            }
                        }
                    } // for loop-iRefIdxTemp

                    if (!bChanged)
                    {
                        if (uiCostBi <= uiCost[0] && uiCostBi <= uiCost[1])
                        {
                            xCopyAMVPInfo(&aacAMVPInfo[0][iRefIdxBi[0]], pcCU->getCUMvField(REF_PIC_LIST_0)->getAMVPInfo());
                            xCheckBestMVP(pcCU, REF_PIC_LIST_0, cMvBi[0], cMvPredBi[0][iRefIdxBi[0]], aaiMvpIdxBi[0][iRefIdxBi[0]], uiBits[2], uiCostBi);
                            if (!pcCU->getSlice()->getMvdL1ZeroFlag())
                            {
                                xCopyAMVPInfo(&aacAMVPInfo[1][iRefIdxBi[1]], pcCU->getCUMvField(REF_PIC_LIST_1)->getAMVPInfo());
                                xCheckBestMVP(pcCU, REF_PIC_LIST_1, cMvBi[1], cMvPredBi[1][iRefIdxBi[1]], aaiMvpIdxBi[1][iRefIdxBi[1]], uiBits[2], uiCostBi);
                            }
                        }
                        break;
                    }
                } // for loop-iter
            } // if (B_SLICE)
        } //end if bTestNormalMC

        //  Clear Motion Field
        pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvField(TComMvField(), ePartSize, uiPartAddr, 0, iPartIdx);
        pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvField(TComMvField(), ePartSize, uiPartAddr, 0, iPartIdx);
        pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvd(cMvZero, ePartSize, uiPartAddr, 0, iPartIdx);
        pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvd(cMvZero, ePartSize, uiPartAddr, 0, iPartIdx);

        pcCU->setMVPIdxSubParts(-1, REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
        pcCU->setMVPNumSubParts(-1, REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
        pcCU->setMVPIdxSubParts(-1, REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
        pcCU->setMVPNumSubParts(-1, REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));

        UInt uiMEBits = 0;
        // Set Motion Field_
        cMv[1] = mvValidList1;
        iRefIdx[1] = refIdxValidList1;
        uiBits[1] = bitsValidList1;
        uiCost[1] = costValidList1;

        if (bTestNormalMC)
        {
            if (uiCostBi <= uiCost[0] && uiCostBi <= uiCost[1])
            {
                uiLastMode = 2;
                {
                    pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMv(cMvBi[0], ePartSize, uiPartAddr, 0, iPartIdx);
                    pcCU->getCUMvField(REF_PIC_LIST_0)->setAllRefIdx(iRefIdxBi[0], ePartSize, uiPartAddr, 0, iPartIdx);
                    pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMv(cMvBi[1], ePartSize, uiPartAddr, 0, iPartIdx);
                    pcCU->getCUMvField(REF_PIC_LIST_1)->setAllRefIdx(iRefIdxBi[1], ePartSize, uiPartAddr, 0, iPartIdx);
                }
                {
                    TempMv = cMvBi[0] - cMvPredBi[0][iRefIdxBi[0]];
                    pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvd(TempMv, ePartSize, uiPartAddr, 0, iPartIdx);
                }
                {
                    TempMv = cMvBi[1] - cMvPredBi[1][iRefIdxBi[1]];
                    pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvd(TempMv, ePartSize, uiPartAddr, 0, iPartIdx);
                }

                pcCU->setInterDirSubParts(3, uiPartAddr, iPartIdx, pcCU->getDepth(0));

                pcCU->setMVPIdxSubParts(aaiMvpIdxBi[0][iRefIdxBi[0]], REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
                pcCU->setMVPNumSubParts(aaiMvpNum[0][iRefIdxBi[0]], REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
                pcCU->setMVPIdxSubParts(aaiMvpIdxBi[1][iRefIdxBi[1]], REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
                pcCU->setMVPNumSubParts(aaiMvpNum[1][iRefIdxBi[1]], REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));

                uiMEBits = uiBits[2];
            }
            else if (uiCost[0] <= uiCost[1])
            {
                uiLastMode = 0;
                pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMv(cMv[0], ePartSize, uiPartAddr, 0, iPartIdx);
                pcCU->getCUMvField(REF_PIC_LIST_0)->setAllRefIdx(iRefIdx[0], ePartSize, uiPartAddr, 0, iPartIdx);
                {
                    TempMv = cMv[0] - cMvPred[0][iRefIdx[0]];
                    pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvd(TempMv, ePartSize, uiPartAddr, 0, iPartIdx);
                }
                pcCU->setInterDirSubParts(1, uiPartAddr, iPartIdx, pcCU->getDepth(0));

                pcCU->setMVPIdxSubParts(aaiMvpIdx[0][iRefIdx[0]], REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
                pcCU->setMVPNumSubParts(aaiMvpNum[0][iRefIdx[0]], REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));

                uiMEBits = uiBits[0];
            }
            else
            {
                uiLastMode = 1;
                pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMv(cMv[1], ePartSize, uiPartAddr, 0, iPartIdx);
                pcCU->getCUMvField(REF_PIC_LIST_1)->setAllRefIdx(iRefIdx[1], ePartSize, uiPartAddr, 0, iPartIdx);
                {
                    TempMv = cMv[1] - cMvPred[1][iRefIdx[1]];
                    pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvd(TempMv, ePartSize, uiPartAddr, 0, iPartIdx);
                }
                pcCU->setInterDirSubParts(2, uiPartAddr, iPartIdx, pcCU->getDepth(0));

                pcCU->setMVPIdxSubParts(aaiMvpIdx[1][iRefIdx[1]], REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
                pcCU->setMVPNumSubParts(aaiMvpNum[1][iRefIdx[1]], REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));

                uiMEBits = uiBits[1];
            }
#if CU_STAT_LOGFILE
            meCost += uiCost[0];
#endif
        } // end if bTestNormalMC

        if (pcCU->getPartitionSize(uiPartAddr) != SIZE_2Nx2N)
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
                xGetInterPredictionError(pcCU, pcOrgYuv, iPartIdx, uiMEError, m_pcEncCfg->getUseHADME());
                uiMECost = uiMEError + m_pcRdCost->getCost(uiMEBits);
            }
            // save ME result.
            uiMEInterDir = pcCU->getInterDir(uiPartAddr);
            pcCU->getMvField(pcCU, uiPartAddr, REF_PIC_LIST_0, cMEMvField[0]);
            pcCU->getMvField(pcCU, uiPartAddr, REF_PIC_LIST_1, cMEMvField[1]);

            // find Merge result
            UInt uiMRGCost = MAX_UINT;
            xMergeEstimation(pcCU, pcOrgYuv, iPartIdx, uiMRGInterDir, cMRGMvField, uiMRGIndex, uiMRGCost, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand);
            if (uiMRGCost < uiMECost)
            {
                // set Merge result
                pcCU->setMergeFlagSubParts(true, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
                pcCU->setMergeIndexSubParts(uiMRGIndex, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
                pcCU->setInterDirSubParts(uiMRGInterDir, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
                {
                    pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvField(cMRGMvField[0], ePartSize, uiPartAddr, 0, iPartIdx);
                    pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvField(cMRGMvField[1], ePartSize, uiPartAddr, 0, iPartIdx);
                }

                pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvd(cMvZero, ePartSize, uiPartAddr, 0, iPartIdx);
                pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvd(cMvZero, ePartSize, uiPartAddr, 0, iPartIdx);

                pcCU->setMVPIdxSubParts(-1, REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
                pcCU->setMVPNumSubParts(-1, REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
                pcCU->setMVPIdxSubParts(-1, REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
                pcCU->setMVPNumSubParts(-1, REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
#if CU_STAT_LOGFILE
                meCost += uiMRGCost;
#endif
#if FAST_MODE_DECISION
                pcCU->getTotalCost() += uiMRGCost;
#endif
            }
            else
            {
                // set ME result
                pcCU->setMergeFlagSubParts(false, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
                pcCU->setInterDirSubParts(uiMEInterDir, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
                {
                    pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvField(cMEMvField[0], ePartSize, uiPartAddr, 0, iPartIdx);
                    pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvField(cMEMvField[1], ePartSize, uiPartAddr, 0, iPartIdx);
                }
#if CU_STAT_LOGFILE
                meCost += uiMECost;
#endif
#if FAST_MODE_DECISION
                pcCU->getTotalCost() += uiMECost;
#endif
            }
        }
#if FAST_MODE_DECISION
        else
            pcCU->getTotalCost() += uiCostTemp;
#endif
        motionCompensation(pcCU, rpcPredYuv, REF_PIC_LIST_X, iPartIdx);
        
    }

    setWpScalingDistParam(pcCU, -1, REF_PIC_LIST_X);
}

// AMVP
Void TEncSearch::xEstimateMvPredAMVP(TComDataCU* pcCU, TComYuv* pcOrgYuv, UInt uiPartIdx, RefPicList eRefPicList, Int iRefIdx, TComMv& rcMvPred, Bool bFilled, UInt* puiDistBiP)
{
    AMVPInfo* pcAMVPInfo = pcCU->getCUMvField(eRefPicList)->getAMVPInfo();

    TComMv  cBestMv;
    Int     iBestIdx = 0;
    TComMv  cZeroMv;
    TComMv  cMvPred;
    UInt    uiBestCost = MAX_INT;
    UInt    uiPartAddr = 0;
    Int     iRoiWidth, iRoiHeight;
    Int     i;

    pcCU->getPartIndexAndSize(uiPartIdx, uiPartAddr, iRoiWidth, iRoiHeight);
    // Fill the MV Candidates
    if (!bFilled)
    {
        pcCU->fillMvpCand(uiPartIdx, uiPartAddr, eRefPicList, iRefIdx, pcAMVPInfo);
    }

    // initialize Mvp index & Mvp
    iBestIdx = 0;
    cBestMv  = pcAMVPInfo->m_acMvCand[0];
    if (pcAMVPInfo->iN <= 1)
    {
        rcMvPred = cBestMv;

        pcCU->setMVPIdxSubParts(iBestIdx, eRefPicList, uiPartAddr, uiPartIdx, pcCU->getDepth(uiPartAddr));
        pcCU->setMVPNumSubParts(pcAMVPInfo->iN, eRefPicList, uiPartAddr, uiPartIdx, pcCU->getDepth(uiPartAddr));

        if (pcCU->getSlice()->getMvdL1ZeroFlag() && eRefPicList == REF_PIC_LIST_1)
        {
            (*puiDistBiP) = xGetTemplateCost(pcCU, uiPartIdx, uiPartAddr, pcOrgYuv, &m_cYuvPredTemp, rcMvPred, 0, AMVP_MAX_NUM_CANDS, eRefPicList, iRefIdx, iRoiWidth, iRoiHeight);
        }
        return;
    }
    if (bFilled)
    {
        assert(pcCU->getMVPIdx(eRefPicList, uiPartAddr) >= 0);
        rcMvPred = pcAMVPInfo->m_acMvCand[pcCU->getMVPIdx(eRefPicList, uiPartAddr)];
        return;
    }

    m_cYuvPredTemp.clear();

    //-- Check Minimum Cost.
    for (i = 0; i < pcAMVPInfo->iN; i++)
    {
        UInt uiTmpCost;
        uiTmpCost = xGetTemplateCost(pcCU, uiPartIdx, uiPartAddr, pcOrgYuv, &m_cYuvPredTemp, pcAMVPInfo->m_acMvCand[i], i, AMVP_MAX_NUM_CANDS, eRefPicList, iRefIdx, iRoiWidth, iRoiHeight);
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
    rcMvPred = cBestMv;
    pcCU->setMVPIdxSubParts(iBestIdx, eRefPicList, uiPartAddr, uiPartIdx, pcCU->getDepth(uiPartAddr));
    pcCU->setMVPNumSubParts(pcAMVPInfo->iN, eRefPicList, uiPartAddr, uiPartIdx, pcCU->getDepth(uiPartAddr));
}

UInt TEncSearch::xGetMvpIdxBits(Int iIdx, Int iNum)
{
    assert(iIdx >= 0 && iNum >= 0 && iIdx < iNum);

    if (iNum == 1)
        return 0;

    UInt uiLength = 1;
    Int iTemp = iIdx;
    if (iTemp == 0)
    {
        return uiLength;
    }

    Bool bCodeLast = (iNum - 1 > iTemp);

    uiLength += (iTemp - 1);

    if (bCodeLast)
    {
        uiLength++;
    }

    return uiLength;
}

Void TEncSearch::xGetBlkBits(PartSize eCUMode, Bool bPSlice, Int iPartIdx, UInt uiLastMode, UInt uiBlkBit[3])
{
    if (eCUMode == SIZE_2Nx2N)
    {
        uiBlkBit[0] = (!bPSlice) ? 3 : 1;
        uiBlkBit[1] = 3;
        uiBlkBit[2] = 5;
    }
    else if ((eCUMode == SIZE_2NxN || eCUMode == SIZE_2NxnU) || eCUMode == SIZE_2NxnD)
    {
        UInt aauiMbBits[2][3][3] = { { { 0, 0, 3 }, { 0, 0, 0 }, { 0, 0, 0 } }, { { 5, 7, 7 }, { 7, 5, 7 }, { 9 - 3, 9 - 3, 9 - 3 } } };
        if (bPSlice)
        {
            uiBlkBit[0] = 3;
            uiBlkBit[1] = 0;
            uiBlkBit[2] = 0;
        }
        else
        {
            ::memcpy(uiBlkBit, aauiMbBits[iPartIdx][uiLastMode], 3 * sizeof(UInt));
        }
    }
    else if ((eCUMode == SIZE_Nx2N || eCUMode == SIZE_nLx2N) || eCUMode == SIZE_nRx2N)
    {
        UInt aauiMbBits[2][3][3] = { { { 0, 2, 3 }, { 0, 0, 0 }, { 0, 0, 0 } }, { { 5, 7, 7 }, { 7 - 2, 7 - 2, 9 - 2 }, { 9 - 3, 9 - 3, 9 - 3 } } };
        if (bPSlice)
        {
            uiBlkBit[0] = 3;
            uiBlkBit[1] = 0;
            uiBlkBit[2] = 0;
        }
        else
        {
            ::memcpy(uiBlkBit, aauiMbBits[iPartIdx][uiLastMode], 3 * sizeof(UInt));
        }
    }
    else if (eCUMode == SIZE_NxN)
    {
        uiBlkBit[0] = (!bPSlice) ? 3 : 1;
        uiBlkBit[1] = 3;
        uiBlkBit[2] = 5;
    }
    else
    {
        printf("Wrong!\n");
        assert(0);
    }
}

Void TEncSearch::xCopyAMVPInfo(AMVPInfo* pSrc, AMVPInfo* pDst)
{
    pDst->iN = pSrc->iN;
    for (Int i = 0; i < pSrc->iN; i++)
    {
        pDst->m_acMvCand[i] = pSrc->m_acMvCand[i];
    }
}

/* Check if using an alternative MVP would result in a smaller MVD + signal bits */
Void TEncSearch::xCheckBestMVP(TComDataCU* pcCU, RefPicList eRefPicList, TComMv cMv, TComMv& rcMvPred, Int& riMVPIdx, UInt& ruiBits, UInt& ruiCost)
{
    AMVPInfo* pcAMVPInfo = pcCU->getCUMvField(eRefPicList)->getAMVPInfo();

    assert(pcAMVPInfo->m_acMvCand[riMVPIdx] == rcMvPred);

    if (pcAMVPInfo->iN < 2) return;

    m_pcRdCost->setCostScale(0);

    Int iBestMVPIdx = riMVPIdx;

    m_me.setMVP(rcMvPred);
    Int iOrgMvBits = m_me.bitcost(cMv) + m_auiMVPIdxCost[riMVPIdx][AMVP_MAX_NUM_CANDS];
    Int iBestMvBits = iOrgMvBits;

    for (Int iMVPIdx = 0; iMVPIdx < pcAMVPInfo->iN; iMVPIdx++)
    {
        if (iMVPIdx == riMVPIdx)
            continue;

        m_me.setMVP(pcAMVPInfo->m_acMvCand[iMVPIdx]);
        Int iMvBits = m_me.bitcost(cMv) + m_auiMVPIdxCost[iMVPIdx][AMVP_MAX_NUM_CANDS];

        if (iMvBits < iBestMvBits)
        {
            iBestMvBits = iMvBits;
            iBestMVPIdx = iMVPIdx;
        }
    }

    if (iBestMVPIdx != riMVPIdx) // if changed
    {
        rcMvPred = pcAMVPInfo->m_acMvCand[iBestMVPIdx];

        riMVPIdx = iBestMVPIdx;
        UInt uiOrgBits = ruiBits;
        ruiBits = uiOrgBits - iOrgMvBits + iBestMvBits;
        ruiCost = (ruiCost - m_pcRdCost->getCost(uiOrgBits))  + m_pcRdCost->getCost(ruiBits);
    }
}

UInt TEncSearch::xGetTemplateCost(TComDataCU* pcCU,
                                  UInt        uiPartIdx,
                                  UInt        uiPartAddr,
                                  TComYuv*    pcOrgYuv,
                                  TComYuv*    pcTemplateCand,
                                  TComMv      cMvCand,
                                  Int         iMVPIdx,
                                  Int         iMVPNum,
                                  RefPicList  eRefPicList,
                                  Int         iRefIdx,
                                  Int         iSizeX,
                                  Int         iSizeY)
{
    UInt uiCost  = MAX_INT;

    TComPicYuv* pcPicYuvRef = pcCU->getSlice()->getRefPic(eRefPicList, iRefIdx)->getPicYuvRec();

    pcCU->clipMv(cMvCand);

    // prediction pattern
    if (pcCU->getSlice()->getPPS()->getUseWP() && pcCU->getSlice()->getSliceType() == P_SLICE)
    {
        xPredInterLumaBlk(pcCU, pcPicYuvRef, uiPartAddr, &cMvCand, iSizeX, iSizeY, pcTemplateCand, true);
    }
    else
    {
        xPredInterLumaBlk(pcCU, pcPicYuvRef, uiPartAddr, &cMvCand, iSizeX, iSizeY, pcTemplateCand, false);
    }

    if (pcCU->getSlice()->getPPS()->getUseWP() && pcCU->getSlice()->getSliceType() == P_SLICE)
    {
        xWeightedPredictionUni(pcCU, pcTemplateCand, uiPartAddr, iSizeX, iSizeY, eRefPicList, pcTemplateCand, iRefIdx);
    }

    // calc distortion
    uiCost = m_me.bufSAD((pixel*)pcTemplateCand->getLumaAddr(uiPartAddr), pcTemplateCand->getStride());
    x265_emms();
    return m_pcRdCost->calcRdSADCost(uiCost, m_auiMVPIdxCost[iMVPIdx][iMVPNum]);
}

DECLARE_CYCLE_COUNTER(ME);

Void TEncSearch::xMotionEstimation(TComDataCU* pcCU, TComYuv* pcYuvOrg, Int iPartIdx, RefPicList eRefPicList, TComMv* pcMvPred, Int iRefIdxPred, TComMv& rcMv, UInt& ruiBits, UInt& ruiCost, Bool bBi)
{
    UInt          uiPartAddr;
    Int           iRoiWidth;
    Int           iRoiHeight;

    TComMv        cMvHalf, cMvQter;
    TComMv        cMvSrchRngLT;
    TComMv        cMvSrchRngRB;

    TComYuv*      pcYuv = pcYuvOrg;

    m_iSearchRange = m_aaiAdaptSR[eRefPicList][iRefIdxPred];

    Int           iSrchRng      = (bBi ? m_bipredSearchRange : m_iSearchRange);
    TComPattern*  pcPatternKey  = pcCU->getPattern();

    Double        fWeight       = 1.0;

    pcCU->getPartIndexAndSize(iPartIdx, uiPartAddr, iRoiWidth, iRoiHeight);
    //PPAScopeEvent(TEncSearch_xMotionEstimation + pcCU->getDepth(iPartIdx));

    if (bBi)
    {
        TComYuv*  pcYuvOther = &m_acYuvPred[1 - (Int)eRefPicList];
        pcYuv                = &m_cYuvPredTemp;

        pcYuvOrg->copyPartToPartYuv(pcYuv, uiPartAddr, iRoiWidth, iRoiHeight);

        pcYuv->removeHighFreq(pcYuvOther, uiPartAddr, iRoiWidth, iRoiHeight);

        fWeight = 0.5;
    }

    //  Search key pattern initialization
    pcPatternKey->initPattern(pcYuv->getLumaAddr(uiPartAddr),
                              pcYuv->getCbAddr(uiPartAddr),
                              pcYuv->getCrAddr(uiPartAddr),
                              iRoiWidth,
                              iRoiHeight,
                              pcYuv->getStride(),
                              0, 0);

    Pel*        piRefY      = pcCU->getSlice()->getRefPic(eRefPicList, iRefIdxPred)->getPicYuvRec()->getLumaAddr(pcCU->getAddr(), pcCU->getZorderIdxInCU() + uiPartAddr);
    Int         iRefStride  = pcCU->getSlice()->getRefPic(eRefPicList, iRefIdxPred)->getPicYuvRec()->getStride();

    TComPicYuv* refPic = pcCU->getSlice()->getRefPic(eRefPicList, iRefIdxPred)->getPicYuvRec(); //For new xPatternSearchFracDiff

    TComMv      cMvPred = *pcMvPred;

    if (bBi)
        xSetSearchRange(pcCU, rcMv, iSrchRng, cMvSrchRngLT, cMvSrchRngRB);
    else
        xSetSearchRange(pcCU, cMvPred, iSrchRng, cMvSrchRngLT, cMvSrchRngRB);

    CYCLE_COUNTER_START(ME);
    if (m_iSearchMethod != X265_ORIG_SEARCH && m_cDistParam.bApplyWeight == false && !bBi)
    {
        TComPicYuv *refRecon = pcCU->getSlice()->getRefPic(eRefPicList, iRefIdxPred)->getPicYuvRec();
        int satdCost = m_me.motionEstimate(refRecon->getMotionReference(0),
                                           cMvSrchRngLT,
                                           cMvSrchRngRB,
                                           cMvPred,
                                           3,
                                           m_acMvPredictors,
                                           iSrchRng,
                                           rcMv);

        /* Get total cost of PU, but only include MV bit cost once */
        ruiBits += m_me.bitcost(rcMv);
        ruiCost = (satdCost - m_me.mvcost(rcMv)) + m_pcRdCost->getCost(ruiBits);
        CYCLE_COUNTER_STOP(ME);
        return;
    }

    m_pcRdCost->setCostScale(2);

    // Configure the MV bit cost calculator
    m_bc.setQP(pcCU->getQP(0), m_pcRdCost->getSqrtLambda());
    m_bc.setMVP(*pcMvPred);

    setWpScalingDistParam(pcCU, iRefIdxPred, eRefPicList);

    //  Do integer search
    if (bBi)
    {
        xPatternSearch(pcPatternKey, piRefY, iRefStride, &cMvSrchRngLT, &cMvSrchRngRB, rcMv, ruiCost);
    }
    else
    {
        rcMv = *pcMvPred;
        xPatternSearchFast(pcCU, pcPatternKey, piRefY, iRefStride, &cMvSrchRngLT, &cMvSrchRngRB, rcMv, ruiCost);
    }

    m_pcRdCost->setCostScale(1);

    xPatternSearchFracDIF(pcCU, pcPatternKey, piRefY, iRefStride, &rcMv, cMvHalf, cMvQter, ruiCost, bBi, refPic, uiPartAddr);

    m_pcRdCost->setCostScale(0);
    rcMv <<= 2;
    rcMv += (cMvHalf <<= 1);
    rcMv += cMvQter;

    UInt uiMvBits = m_bc.mvcost(x265::MV(rcMv.getHor(), rcMv.getVer()));

    ruiBits += uiMvBits;
    if (bBi)
        ruiCost = (UInt)(floor(fWeight * ((Double)ruiCost - (Double)m_pcRdCost->getCost(uiMvBits))) + (Double)m_pcRdCost->getCost(ruiBits));
    else
        ruiCost = (ruiCost - m_pcRdCost->getCost(uiMvBits)) + m_pcRdCost->getCost(ruiBits);

    CYCLE_COUNTER_STOP(ME);
}

Void TEncSearch::xSetSearchRange(TComDataCU* pcCU, TComMv& cMvPred, Int iSrchRng, TComMv& rcMvSrchRngLT, TComMv& rcMvSrchRngRB)
{
    Int  iMvShift = 2;
    TComMv cTmpMvPred = cMvPred;

    pcCU->clipMv(cTmpMvPred);

    rcMvSrchRngLT.setHor(cTmpMvPred.getHor() - (iSrchRng << iMvShift));
    rcMvSrchRngLT.setVer(cTmpMvPred.getVer() - (iSrchRng << iMvShift));

    rcMvSrchRngRB.setHor(cTmpMvPred.getHor() + (iSrchRng << iMvShift));
    rcMvSrchRngRB.setVer(cTmpMvPred.getVer() + (iSrchRng << iMvShift));
    pcCU->clipMv(rcMvSrchRngLT);
    pcCU->clipMv(rcMvSrchRngRB);

    rcMvSrchRngLT >>= iMvShift;
    rcMvSrchRngRB >>= iMvShift;
}

Void TEncSearch::xPatternSearch(TComPattern* pcPatternKey, Pel* piRefY, Int iRefStride, TComMv* pcMvSrchRngLT, TComMv* pcMvSrchRngRB, TComMv& rcMv, UInt& ruiSAD)
{
    Int   iSrchRngHorLeft   = pcMvSrchRngLT->getHor();
    Int   iSrchRngHorRight  = pcMvSrchRngRB->getHor();
    Int   iSrchRngVerTop    = pcMvSrchRngLT->getVer();
    Int   iSrchRngVerBottom = pcMvSrchRngRB->getVer();

    UInt  uiSad;
    UInt  uiSadBest         = MAX_UINT;
    Int   iBestX = 0;
    Int   iBestY = 0;

    Pel*  piRefSrch;

    //-- jclee for using the SAD function pointer
    m_pcRdCost->setDistParam(pcPatternKey, piRefY, iRefStride,  m_cDistParam);

    // fast encoder decision: use subsampled SAD for integer ME
    if (0)
    {
        if (m_cDistParam.iRows > 12)
        {
            m_cDistParam.iSubShift = 1;
        }
    }

    piRefY += (iSrchRngVerTop * iRefStride);
    for (Int y = iSrchRngVerTop; y <= iSrchRngVerBottom; y++)
    {
        for (Int x = iSrchRngHorLeft; x <= iSrchRngHorRight; x++)
        {
            //  find min. distortion position
            piRefSrch = piRefY + x;
            m_cDistParam.pCur = piRefSrch;

            setDistParamComp(0);

            m_cDistParam.bitDepth = g_bitDepthY;
            uiSad = m_cDistParam.DistFunc(&m_cDistParam);
            uiSad += m_bc.mvcost(MV(x, y) << 2);

            if (uiSad < uiSadBest)
            {
                uiSadBest = uiSad;
                iBestX    = x;
                iBestY    = y;
            }
        }

        piRefY += iRefStride;
    }

    rcMv.set(iBestX, iBestY);

    ruiSAD = uiSadBest - m_bc.mvcost(rcMv << 2);
}

Void TEncSearch::xPatternSearchFast(TComDataCU* pcCU, TComPattern* pcPatternKey, Pel* piRefY, Int iRefStride, TComMv* pcMvSrchRngLT, TComMv* pcMvSrchRngRB, TComMv& rcMv, UInt& ruiSAD)
{
    pcCU->getMvPredLeft(m_acMvPredictors[0]);
    pcCU->getMvPredAbove(m_acMvPredictors[1]);
    pcCU->getMvPredAboveRight(m_acMvPredictors[2]);
    xTZSearch(pcCU, pcPatternKey, piRefY, iRefStride, pcMvSrchRngLT, pcMvSrchRngRB, rcMv, ruiSAD);
}

Void TEncSearch::xTZSearch(TComDataCU* pcCU, TComPattern* pcPatternKey, Pel* piRefY, Int iRefStride, TComMv* pcMvSrchRngLT, TComMv* pcMvSrchRngRB, TComMv& rcMv, UInt& ruiSAD)
{
    Int   iSrchRngHorLeft   = pcMvSrchRngLT->getHor();
    Int   iSrchRngHorRight  = pcMvSrchRngRB->getHor();
    Int   iSrchRngVerTop    = pcMvSrchRngLT->getVer();
    Int   iSrchRngVerBottom = pcMvSrchRngRB->getVer();

    TZ_SEARCH_CONFIGURATION

    UInt uiSearchRange = m_iSearchRange;

    pcCU->clipMv(rcMv);
    rcMv >>= 2;
    // init TZSearchStruct
    IntTZSearchStruct cStruct;
    cStruct.iYStride    = iRefStride;
    cStruct.piRefY      = piRefY;
    cStruct.uiBestSad   = MAX_UINT;

    // set rcMv (Median predictor) as start point and as best point
    xTZSearchHelp(pcPatternKey, cStruct, rcMv.getHor(), rcMv.getVer(), 0, 0);

    // test whether one of PRED_A, PRED_B, PRED_C MV is better start point than Median predictor
    if (bTestOtherPredictedMV)
    {
        for (UInt index = 0; index < 3; index++)
        {
            TComMv cMv = m_acMvPredictors[index];
            pcCU->clipMv(cMv);
            cMv >>= 2;
            xTZSearchHelp(pcPatternKey, cStruct, cMv.getHor(), cMv.getVer(), 0, 0);
        }
    }

    // test whether zero Mv is better start point than Median predictor
    if (bTestZeroVector)
    {
        xTZSearchHelp(pcPatternKey, cStruct, 0, 0, 0, 0);
    }

    // start search
    Int  iDist = 0;
    Int  iStartX = cStruct.iBestX;
    Int  iStartY = cStruct.iBestY;

    // first search
    for (iDist = 1; iDist <= (Int)uiSearchRange; iDist *= 2)
    {
        xTZ8PointDiamondSearch(pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB, iStartX, iStartY, iDist);

        if (bFirstSearchStop && (cStruct.uiBestRound >= uiFirstSearchRounds)) // stop criterion
        {
            break;
        }
    }

    // test whether zero Mv is a better start point than Median predictor
    if (bTestZeroVectorStart && ((cStruct.iBestX != 0) || (cStruct.iBestY != 0)))
    {
        xTZSearchHelp(pcPatternKey, cStruct, 0, 0, 0, 0);
        if ((cStruct.iBestX == 0) && (cStruct.iBestY == 0))
        {
            // test its neighborhood
            for (iDist = 1; iDist <= (Int)uiSearchRange; iDist *= 2)
            {
                xTZ8PointDiamondSearch(pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB, 0, 0, iDist);
                if (bTestZeroVectorStop && (cStruct.uiBestRound > 0)) // stop criterion
                {
                    break;
                }
            }
        }
    }

    // calculate only 2 missing points instead 8 points if cStruct.uiBestDistance == 1
    if (cStruct.uiBestDistance == 1)
    {
        cStruct.uiBestDistance = 0;
        xTZ2PointSearch(pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB);
    }

    // raster search if distance is too big
    if (bEnableRasterSearch && (((Int)(cStruct.uiBestDistance) > iRaster) || bAlwaysRasterSearch))
    {
        cStruct.uiBestDistance = iRaster;
        for (iStartY = iSrchRngVerTop; iStartY <= iSrchRngVerBottom; iStartY += iRaster)
        {
            for (iStartX = iSrchRngHorLeft; iStartX <= iSrchRngHorRight; iStartX += iRaster)
            {
                xTZSearchHelp(pcPatternKey, cStruct, iStartX, iStartY, 0, iRaster);
            }
        }
    }

    // raster refinement
    if (bRasterRefinementEnable && cStruct.uiBestDistance > 0)
    {
        while (cStruct.uiBestDistance > 0)
        {
            iStartX = cStruct.iBestX;
            iStartY = cStruct.iBestY;
            if (cStruct.uiBestDistance > 1)
            {
                iDist = cStruct.uiBestDistance >>= 1;
                xTZ8PointDiamondSearch(pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB, iStartX, iStartY, iDist);
            }

            // calculate only 2 missing points instead 8 points if cStruct.uiBestDistance == 1
            if (cStruct.uiBestDistance == 1)
            {
                cStruct.uiBestDistance = 0;
                if (cStruct.ucPointNr != 0)
                {
                    xTZ2PointSearch(pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB);
                }
            }
        }
    }

    // start refinement
    if (bStarRefinementEnable && cStruct.uiBestDistance > 0)
    {
        while (cStruct.uiBestDistance > 0)
        {
            iStartX = cStruct.iBestX;
            iStartY = cStruct.iBestY;
            cStruct.uiBestDistance = 0;
            cStruct.ucPointNr = 0;
            for (iDist = 1; iDist < (Int)uiSearchRange + 1; iDist *= 2)
            {
                xTZ8PointDiamondSearch(pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB, iStartX, iStartY, iDist);
                if (bStarRefinementStop && (cStruct.uiBestRound >= uiStarRefinementRounds)) // stop criterion
                {
                    break;
                }
            }

            // calculate only 2 missing points instead 8 points if cStrukt.uiBestDistance == 1
            if (cStruct.uiBestDistance == 1)
            {
                cStruct.uiBestDistance = 0;
                if (cStruct.ucPointNr != 0)
                {
                    xTZ2PointSearch(pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB);
                }
            }
        }
    }

    // write out best match
    rcMv.set(cStruct.iBestX, cStruct.iBestY);
    ruiSAD = cStruct.uiBestSad - m_bc.mvcost(rcMv << 2);
}

Void TEncSearch::xPatternSearchFracDIF(TComDataCU*  pcCU,
                                       TComPattern* pcPatternKey,
                                       Pel*         piRefY,
                                       Int          iRefStride,
                                       TComMv*      pcMvInt,
                                       TComMv&      rcMvHalf,
                                       TComMv&      rcMvQter,
                                       UInt&        ruiCost,
                                       Bool         biPred,
                                       TComPicYuv * refPic,
                                       UInt         uiPartAddr
                                       )
{
    Int         iOffset    = pcMvInt->getHor() + pcMvInt->getVer() * iRefStride;

    TComMv baseRefMv(0, 0);

    rcMvHalf = *pcMvInt;
    rcMvHalf <<= 1;

    ruiCost =  xPatternRefinement(pcPatternKey, baseRefMv, 2, rcMvHalf, refPic, iOffset, pcCU, uiPartAddr);
    m_pcRdCost->setCostScale(0);

    baseRefMv = rcMvHalf;
    baseRefMv <<= 1;

    rcMvQter = *pcMvInt;
    rcMvQter <<= 1;                         // for mv-cost
    rcMvQter += rcMvHalf;
    rcMvQter <<= 1;

    ruiCost = xPatternRefinement(pcPatternKey, baseRefMv, 1, rcMvQter, refPic, iOffset, pcCU, uiPartAddr);
}

/** encode residual and calculate rate-distortion for a CU block
 * \param pcCU
 * \param pcYuvOrg
 * \param pcYuvPred
 * \param rpcYuvResi
 * \param rpcYuvResiBest
 * \param rpcYuvRec
 * \param bSkipRes
 * \returns Void
 */
Void TEncSearch::encodeResAndCalcRdInterCU(TComDataCU* pcCU, TComYuv* pcYuvOrg, TComYuv* pcYuvPred, TShortYUV*& rpcYuvResi, TShortYUV*& rpcYuvResiBest, TComYuv*& rpcYuvRec, Bool bSkipRes)
{
    if (pcCU->isIntra(0))
    {
        return;
    }

    Bool      bHighPass    = pcCU->getSlice()->getDepth() ? true : false;
    UInt      uiBits       = 0, uiBitsBest = 0;
    UInt      uiDistortion = 0, uiDistortionBest = 0;

    UInt      uiWidth      = pcCU->getWidth(0);
    UInt      uiHeight     = pcCU->getHeight(0);

    //  No residual coding : SKIP mode
    if (bSkipRes)
    {
        pcCU->setSkipFlagSubParts(true, 0, pcCU->getDepth(0));

        rpcYuvResi->clear();

        pcYuvPred->copyToPartYuv(rpcYuvRec, 0);

        uiDistortion = primitives.sse_pp[PartitionFromSizes(uiWidth, uiHeight)]((pixel *)pcYuvOrg->getLumaAddr(), (intptr_t)pcYuvOrg->getStride(), (pixel *)rpcYuvRec->getLumaAddr(), rpcYuvRec->getStride());
        uiDistortion += m_pcRdCost->scaleChromaDistCb(primitives.sse_pp[PartitionFromSizes(uiWidth >> 1, uiHeight >> 1)]((pixel *)pcYuvOrg->getCbAddr(), (intptr_t)pcYuvOrg->getCStride(), (pixel *)rpcYuvRec->getCbAddr(), rpcYuvRec->getCStride()));
        uiDistortion += m_pcRdCost->scaleChromaDistCr(primitives.sse_pp[PartitionFromSizes(uiWidth >> 1, uiHeight >> 1)]((pixel *)pcYuvOrg->getCrAddr(), (intptr_t)pcYuvOrg->getCStride(), (pixel *)rpcYuvRec->getCrAddr(), rpcYuvRec->getCStride()));

        m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[pcCU->getDepth(0)][CI_CURR_BEST]);
        m_pcEntropyCoder->resetBits();
        if (pcCU->getSlice()->getPPS()->getTransquantBypassEnableFlag())
        {
            m_pcEntropyCoder->encodeCUTransquantBypassFlag(pcCU, 0, true);
        }
        m_pcEntropyCoder->encodeSkipFlag(pcCU, 0, true);
        m_pcEntropyCoder->encodeMergeIndex(pcCU, 0, true);

        uiBits = m_pcEntropyCoder->getNumberOfWrittenBits();
        m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[pcCU->getDepth(0)][CI_TEMP_BEST]);

        pcCU->getTotalBits()       = uiBits;
        pcCU->getTotalDistortion() = uiDistortion;
        pcCU->getTotalCost()       = m_pcRdCost->calcRdCost(uiDistortion, uiBits);

        m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[pcCU->getDepth(0)][CI_TEMP_BEST]);

        pcCU->setCbfSubParts(0, 0, 0, 0, pcCU->getDepth(0));
        pcCU->setTrIdxSubParts(0, 0, pcCU->getDepth(0));

        return;
    }

    //  Residual coding.
    Int     qp, qpBest = 0;
    UInt64  dCost, dCostBest = MAX_INT64;

    UInt uiTrLevel = 0;
    if ((pcCU->getWidth(0) > pcCU->getSlice()->getSPS()->getMaxTrSize()))
    {
        while (pcCU->getWidth(0) > (pcCU->getSlice()->getSPS()->getMaxTrSize() << uiTrLevel))
        {
            uiTrLevel++;
        }
    }
    UInt uiMaxTrMode = 1 + uiTrLevel;

    while ((uiWidth >> uiMaxTrMode) < (g_uiMaxCUWidth >> g_uiMaxCUDepth))
    {
        uiMaxTrMode--;
    }

    qp = bHighPass ? Clip3(-pcCU->getSlice()->getSPS()->getQpBDOffsetY(), MAX_QP, (Int)pcCU->getQP(0)) : pcCU->getQP(0);

    rpcYuvResi->subtract(pcYuvOrg, pcYuvPred, 0, uiWidth);

    dCost = 0.;
    uiBits = 0;
    uiDistortion = 0;
    m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[pcCU->getDepth(0)][CI_CURR_BEST]);

    UInt uiZeroDistortion = 0;
    xEstimateResidualQT(pcCU, 0, 0, 0, rpcYuvResi,  pcCU->getDepth(0), dCost, uiBits, uiDistortion, &uiZeroDistortion);

    UInt zeroResiBits;

    m_pcEntropyCoder->resetBits();
    m_pcEntropyCoder->encodeQtRootCbfZero(pcCU);
    zeroResiBits = m_pcEntropyCoder->getNumberOfWrittenBits();

    UInt64 dZeroCost = m_pcRdCost->calcRdCost(uiZeroDistortion, zeroResiBits);
    if (pcCU->isLosslessCoded(0))
    {
        dZeroCost = dCost + 1;
    }
    if (dZeroCost < dCost)
    {
        dCost        = dZeroCost;
        uiBits       = 0;
        uiDistortion = uiZeroDistortion;

        const UInt uiQPartNum = pcCU->getPic()->getNumPartInCU() >> (pcCU->getDepth(0) << 1);
        ::memset(pcCU->getTransformIdx(), 0, uiQPartNum * sizeof(UChar));
        ::memset(pcCU->getCbf(TEXT_LUMA), 0, uiQPartNum * sizeof(UChar));
        ::memset(pcCU->getCbf(TEXT_CHROMA_U), 0, uiQPartNum * sizeof(UChar));
        ::memset(pcCU->getCbf(TEXT_CHROMA_V), 0, uiQPartNum * sizeof(UChar));
        ::memset(pcCU->getCoeffY(), 0, uiWidth * uiHeight * sizeof(TCoeff));
        ::memset(pcCU->getCoeffCb(), 0, uiWidth * uiHeight * sizeof(TCoeff) >> 2);
        ::memset(pcCU->getCoeffCr(), 0, uiWidth * uiHeight * sizeof(TCoeff) >> 2);
        pcCU->setTransformSkipSubParts(0, 0, 0, 0, pcCU->getDepth(0));
    }
    else
    {
        xSetResidualQTData(pcCU, 0, 0, 0, NULL, pcCU->getDepth(0), false);
    }

    m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[pcCU->getDepth(0)][CI_CURR_BEST]);

    uiBits = 0;
    {
        TShortYUV *pDummy = NULL;
        xAddSymbolBitsInter(pcCU, 0, 0, uiBits, pDummy, NULL, pDummy);
    }

    UInt64 dExactCost = m_pcRdCost->calcRdCost(uiDistortion, uiBits);
    dCost = dExactCost;

    if (dCost < dCostBest)
    {
        if (!pcCU->getQtRootCbf(0))
        {
            rpcYuvResiBest->clear();
        }
        else
        {
            xSetResidualQTData(pcCU, 0, 0, 0, rpcYuvResiBest, pcCU->getDepth(0), true);
        }

        uiBitsBest       = uiBits;
        uiDistortionBest = uiDistortion;
        dCostBest        = dCost;
        qpBest           = qp;
        m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[pcCU->getDepth(0)][CI_TEMP_BEST]);
    }

    assert(dCostBest != MAX_INT64);

    rpcYuvRec->addClip(pcYuvPred, rpcYuvResiBest, 0, uiWidth);

    // update with clipped distortion and cost (qp estimation loop uses unclipped values)
    uiDistortionBest = primitives.sse_pp[PartitionFromSizes(uiWidth, uiHeight)]((pixel *)pcYuvOrg->getLumaAddr(), (intptr_t)pcYuvOrg->getStride(), (pixel *)rpcYuvRec->getLumaAddr(), rpcYuvRec->getStride());
    uiDistortionBest += m_pcRdCost->scaleChromaDistCb(primitives.sse_pp[PartitionFromSizes(uiWidth >> 1, uiHeight >> 1)]((pixel *)pcYuvOrg->getCbAddr(), (intptr_t)pcYuvOrg->getCStride(), (pixel *)rpcYuvRec->getCbAddr(), rpcYuvRec->getCStride()));
    uiDistortionBest += m_pcRdCost->scaleChromaDistCr(primitives.sse_pp[PartitionFromSizes(uiWidth >> 1, uiHeight >> 1)]((pixel *)pcYuvOrg->getCrAddr(), (intptr_t)pcYuvOrg->getCStride(), (pixel *)rpcYuvRec->getCrAddr(), rpcYuvRec->getCStride()));
    dCostBest = m_pcRdCost->calcRdCost(uiDistortionBest, uiBitsBest);

#if !FAST_MODE_DECISION
    pcCU->getTotalBits()       = uiBitsBest;
    pcCU->getTotalDistortion() = uiDistortionBest;
    pcCU->getTotalCost()       = dCostBest;
#endif

    if (pcCU->isSkipped(0))
    {
        pcCU->setCbfSubParts(0, 0, 0, 0, pcCU->getDepth(0));
    }

    pcCU->setQPSubParts(qpBest, 0, pcCU->getDepth(0));
}

#if _MSC_VER
#pragma warning(disable: 4701) // potentially uninitialized local variable
#endif

Void TEncSearch::xEstimateResidualQT(TComDataCU* pcCU,
                                     UInt        uiQuadrant,
                                     UInt        uiAbsPartIdx,
                                     UInt        absTUPartIdx,
                                     TShortYUV*  pcResi,
                                     const UInt  uiDepth,
                                     UInt64 &    rdCost,
                                     UInt &      ruiBits,
                                     UInt &      ruiDist,
                                     UInt *      puiZeroDist)
{
    const UInt uiTrMode = uiDepth - pcCU->getDepth(0);

    assert(pcCU->getDepth(0) == pcCU->getDepth(uiAbsPartIdx));
    const UInt uiLog2TrSize = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiDepth] + 2;

    UInt SplitFlag = ((pcCU->getSlice()->getSPS()->getQuadtreeTUMaxDepthInter() == 1) && pcCU->getPredictionMode(uiAbsPartIdx) == MODE_INTER && (pcCU->getPartitionSize(uiAbsPartIdx) != SIZE_2Nx2N));
    Bool bCheckFull;
    if (SplitFlag && uiDepth == pcCU->getDepth(uiAbsPartIdx) && (uiLog2TrSize >  pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx)))
        bCheckFull = false;
    else
        bCheckFull =  (uiLog2TrSize <= pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize());

    const Bool bCheckSplit  = (uiLog2TrSize >  pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx));

    assert(bCheckFull || bCheckSplit);

    Bool  bCodeChroma   = true;
    UInt  uiTrModeC     = uiTrMode;
    UInt  uiLog2TrSizeC = uiLog2TrSize - 1;
    if (uiLog2TrSize == 2)
    {
        uiLog2TrSizeC++;
        uiTrModeC--;
        UInt  uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ((pcCU->getDepth(0) + uiTrModeC) << 1);
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
        const UInt uiNumCoeffPerAbsPartIdxIncrement = pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() >> (pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1);
        const UInt uiQTTempAccessLayer = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;
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
        pcCU->setTrIdxSubParts(uiDepth - pcCU->getDepth(0), uiAbsPartIdx, uiDepth);
        UInt64 minCostY = MAX_INT64;
        UInt64 minCostU = MAX_INT64;
        UInt64 minCostV = MAX_INT64;
        Bool checkTransformSkipY  = pcCU->getSlice()->getPPS()->getUseTransformSkip() && trWidth == 4 && trHeight == 4;
        Bool checkTransformSkipUV = pcCU->getSlice()->getPPS()->getUseTransformSkip() && trWidthC == 4 && trHeightC == 4;

        checkTransformSkipY         &= (!pcCU->isLosslessCoded(0));
        checkTransformSkipUV        &= (!pcCU->isLosslessCoded(0));

        pcCU->setTransformSkipSubParts(0, TEXT_LUMA, uiAbsPartIdx, uiDepth);
        if (bCodeChroma)
        {
            pcCU->setTransformSkipSubParts(0, TEXT_CHROMA_U, uiAbsPartIdx, pcCU->getDepth(0) + uiTrModeC);
            pcCU->setTransformSkipSubParts(0, TEXT_CHROMA_V, uiAbsPartIdx, pcCU->getDepth(0) + uiTrModeC);
        }

        if (m_pcEncCfg->getUseRDOQ())
        {
            m_pcEntropyCoder->estimateBit(m_pcTrQuant->m_pcEstBitsSbac, trWidth, trHeight, TEXT_LUMA);
        }

        m_pcTrQuant->setQPforQuant(pcCU->getQP(0), TEXT_LUMA, pcCU->getSlice()->getSPS()->getQpBDOffsetY(), 0);

        m_pcTrQuant->selectLambda(TEXT_LUMA);

        m_pcTrQuant->transformNxN(pcCU, pcResi->getLumaAddr(absTUPartIdx), pcResi->getStride(), pcCoeffCurrY,
                                  pcArlCoeffCurrY, trWidth, trHeight, uiAbsSumY, TEXT_LUMA, uiAbsPartIdx);

        pcCU->setCbfSubParts(uiAbsSumY ? uiSetCbf : 0, TEXT_LUMA, uiAbsPartIdx, uiDepth);

        if (bCodeChroma)
        {
            if (m_pcEncCfg->getUseRDOQ())
            {
                m_pcEntropyCoder->estimateBit(m_pcTrQuant->m_pcEstBitsSbac, trWidthC, trHeightC, TEXT_CHROMA);
            }

            Int curChromaQpOffset = pcCU->getSlice()->getPPS()->getChromaCbQpOffset() + pcCU->getSlice()->getSliceQpDeltaCb();
            m_pcTrQuant->setQPforQuant(pcCU->getQP(0), TEXT_CHROMA, pcCU->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset);

            m_pcTrQuant->selectLambda(TEXT_CHROMA);

            m_pcTrQuant->transformNxN(pcCU, pcResi->getCbAddr(absTUPartIdxC), pcResi->getCStride(), pcCoeffCurrU,
                                      pcArlCoeffCurrU, trWidthC, trHeightC, uiAbsSumU, TEXT_CHROMA_U, uiAbsPartIdx);

            curChromaQpOffset = pcCU->getSlice()->getPPS()->getChromaCrQpOffset() + pcCU->getSlice()->getSliceQpDeltaCr();
            m_pcTrQuant->setQPforQuant(pcCU->getQP(0), TEXT_CHROMA, pcCU->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset);
            m_pcTrQuant->transformNxN(pcCU, pcResi->getCrAddr(absTUPartIdxC), pcResi->getCStride(), pcCoeffCurrV,
                                      pcArlCoeffCurrV, trWidthC, trHeightC, uiAbsSumV, TEXT_CHROMA_V, uiAbsPartIdx);

            pcCU->setCbfSubParts(uiAbsSumU ? uiSetCbf : 0, TEXT_CHROMA_U, uiAbsPartIdx, pcCU->getDepth(0) + uiTrModeC);
            pcCU->setCbfSubParts(uiAbsSumV ? uiSetCbf : 0, TEXT_CHROMA_V, uiAbsPartIdx, pcCU->getDepth(0) + uiTrModeC);
        }

        m_pcEntropyCoder->resetBits();

        {
            m_pcEntropyCoder->encodeQtCbf(pcCU, uiAbsPartIdx, TEXT_LUMA,     uiTrMode);
        }

        m_pcEntropyCoder->encodeCoeffNxN(pcCU, pcCoeffCurrY, uiAbsPartIdx,  trWidth,  trHeight,    uiDepth, TEXT_LUMA);
        const UInt uiSingleBitsY = m_pcEntropyCoder->getNumberOfWrittenBits();

        UInt uiSingleBitsU = 0;
        UInt uiSingleBitsV = 0;
        if (bCodeChroma)
        {
            {
                m_pcEntropyCoder->encodeQtCbf(pcCU, uiAbsPartIdx, TEXT_CHROMA_U, uiTrMode);
            }
            m_pcEntropyCoder->encodeCoeffNxN(pcCU, pcCoeffCurrU, uiAbsPartIdx, trWidthC, trHeightC, uiDepth, TEXT_CHROMA_U);
            uiSingleBitsU = m_pcEntropyCoder->getNumberOfWrittenBits() - uiSingleBitsY;

            {
                m_pcEntropyCoder->encodeQtCbf(pcCU, uiAbsPartIdx, TEXT_CHROMA_V, uiTrMode);
            }
            m_pcEntropyCoder->encodeCoeffNxN(pcCU, pcCoeffCurrV, uiAbsPartIdx, trWidthC, trHeightC, uiDepth, TEXT_CHROMA_V);
            uiSingleBitsV = m_pcEntropyCoder->getNumberOfWrittenBits() - (uiSingleBitsY + uiSingleBitsU);
        }

        const UInt uiNumSamplesLuma = 1 << (uiLog2TrSize << 1);
        const UInt uiNumSamplesChro = 1 << (uiLog2TrSizeC << 1);

        ::memset(m_pTempPel, 0, sizeof(Pel) * uiNumSamplesLuma); // not necessary needed for inside of recursion (only at the beginning)

        int Part = PartitionFromSizes(trWidth, trHeight);
        UInt uiDistY = primitives.sse_sp[Part](pcResi->getLumaAddr(absTUPartIdx), (intptr_t)pcResi->getStride(), (pixel *)m_pTempPel, trWidth);

        if (puiZeroDist)
        {
            *puiZeroDist += uiDistY;
        }
        if (uiAbsSumY)
        {
            Short *pcResiCurrY = m_pcQTTempTComYuv[uiQTTempAccessLayer].getLumaAddr(absTUPartIdx);

            m_pcTrQuant->setQPforQuant(pcCU->getQP(0), TEXT_LUMA, pcCU->getSlice()->getSPS()->getQpBDOffsetY(), 0);

            Int scalingListType = 3 + g_eTTable[(Int)TEXT_LUMA];
            assert(scalingListType < 6);
            m_pcTrQuant->invtransformNxN(pcCU->getCUTransquantBypass(uiAbsPartIdx), TEXT_LUMA, REG_DCT, pcResiCurrY, m_pcQTTempTComYuv[uiQTTempAccessLayer].getStride(),  pcCoeffCurrY, trWidth, trHeight, scalingListType); //this is for inter mode only

            const UInt uiNonzeroDistY = primitives.sse_ss[Part](pcResi->getLumaAddr(absTUPartIdx), pcResi->getStride(), m_pcQTTempTComYuv[uiQTTempAccessLayer].getLumaAddr(absTUPartIdx),
                                                                m_pcQTTempTComYuv[uiQTTempAccessLayer].getStride());
            if (pcCU->isLosslessCoded(0))
            {
                uiDistY = uiNonzeroDistY;
            }
            else
            {
                const UInt64 singleCostY = m_pcRdCost->calcRdCost(uiNonzeroDistY, uiSingleBitsY);
                m_pcEntropyCoder->resetBits();
                m_pcEntropyCoder->encodeQtCbfZero(pcCU, TEXT_LUMA,     uiTrMode);
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
            m_pcEntropyCoder->encodeQtCbfZero(pcCU, TEXT_LUMA, uiTrMode);
            const UInt uiNullBitsY = m_pcEntropyCoder->getNumberOfWrittenBits();
            minCostY = m_pcRdCost->calcRdCost(uiDistY, uiNullBitsY);
        }

        if (!uiAbsSumY)
        {
            Short *pcPtr =  m_pcQTTempTComYuv[uiQTTempAccessLayer].getLumaAddr(absTUPartIdx);
            const UInt uiStride = m_pcQTTempTComYuv[uiQTTempAccessLayer].getStride();
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
            uiDistU = m_pcRdCost->scaleChromaDistCb(primitives.sse_sp[PartC](pcResi->getCbAddr(absTUPartIdxC), (intptr_t)pcResi->getCStride(), (pixel *)m_pTempPel, trWidthC));

            if (puiZeroDist)
            {
                *puiZeroDist += uiDistU;
            }
            if (uiAbsSumU)
            {
                Short *pcResiCurrU = m_pcQTTempTComYuv[uiQTTempAccessLayer].getCbAddr(absTUPartIdxC);

                Int curChromaQpOffset = pcCU->getSlice()->getPPS()->getChromaCbQpOffset() + pcCU->getSlice()->getSliceQpDeltaCb();
                m_pcTrQuant->setQPforQuant(pcCU->getQP(0), TEXT_CHROMA, pcCU->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset);

                Int scalingListType = 3 + g_eTTable[(Int)TEXT_CHROMA_U];
                assert(scalingListType < 6);
                m_pcTrQuant->invtransformNxN(pcCU->getCUTransquantBypass(uiAbsPartIdx), TEXT_CHROMA, REG_DCT, pcResiCurrU, m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride(), pcCoeffCurrU, trWidthC, trHeightC, scalingListType);

                const UInt uiNonzeroDistU = m_pcRdCost->scaleChromaDistCb(primitives.sse_ss[PartC](pcResi->getCbAddr(absTUPartIdxC), pcResi->getCStride(),
                                                                                                   m_pcQTTempTComYuv[uiQTTempAccessLayer].getCbAddr(absTUPartIdxC), m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride()));
                if (pcCU->isLosslessCoded(0))
                {
                    uiDistU = uiNonzeroDistU;
                }
                else
                {
                    const UInt64 dSingleCostU = m_pcRdCost->calcRdCost(uiNonzeroDistU, uiSingleBitsU);
                    m_pcEntropyCoder->resetBits();
                    m_pcEntropyCoder->encodeQtCbfZero(pcCU, TEXT_CHROMA_U,     uiTrMode);
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
                m_pcEntropyCoder->encodeQtCbfZero(pcCU, TEXT_CHROMA_U, uiTrModeC);
                const UInt uiNullBitsU = m_pcEntropyCoder->getNumberOfWrittenBits();
                minCostU = m_pcRdCost->calcRdCost(uiDistU, uiNullBitsU);
            }
            if (!uiAbsSumU)
            {
                Short *pcPtr =  m_pcQTTempTComYuv[uiQTTempAccessLayer].getCbAddr(absTUPartIdxC);
                const UInt uiStride = m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride();
                for (UInt uiY = 0; uiY < trHeightC; ++uiY)
                {
                    ::memset(pcPtr, 0, sizeof(Short) * trWidthC);
                    pcPtr += uiStride;
                }
            }

            uiDistV = m_pcRdCost->scaleChromaDistCr(primitives.sse_sp[PartC](pcResi->getCrAddr(absTUPartIdxC), (intptr_t)pcResi->getCStride(), (pixel *)m_pTempPel, trWidthC));
            if (puiZeroDist)
            {
                *puiZeroDist += uiDistV;
            }
            if (uiAbsSumV)
            {
                Short *pcResiCurrV = m_pcQTTempTComYuv[uiQTTempAccessLayer].getCrAddr(absTUPartIdxC);
                Int curChromaQpOffset = pcCU->getSlice()->getPPS()->getChromaCrQpOffset() + pcCU->getSlice()->getSliceQpDeltaCr();
                m_pcTrQuant->setQPforQuant(pcCU->getQP(0), TEXT_CHROMA, pcCU->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset);

                Int scalingListType = 3 + g_eTTable[(Int)TEXT_CHROMA_V];
                assert(scalingListType < 6);
                m_pcTrQuant->invtransformNxN(pcCU->getCUTransquantBypass(uiAbsPartIdx), TEXT_CHROMA, REG_DCT, pcResiCurrV, m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride(), pcCoeffCurrV, trWidthC, trHeightC, scalingListType);

                const UInt uiNonzeroDistV = m_pcRdCost->scaleChromaDistCr(primitives.sse_ss[PartC](pcResi->getCrAddr(absTUPartIdxC), pcResi->getCStride(),
                                                                                                   m_pcQTTempTComYuv[uiQTTempAccessLayer].getCrAddr(absTUPartIdxC), m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride()));

                if (pcCU->isLosslessCoded(0))
                {
                    uiDistV = uiNonzeroDistV;
                }
                else
                {
                    const UInt64 dSingleCostV = m_pcRdCost->calcRdCost(uiNonzeroDistV, uiSingleBitsV);
                    m_pcEntropyCoder->resetBits();
                    m_pcEntropyCoder->encodeQtCbfZero(pcCU, TEXT_CHROMA_V,     uiTrMode);
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
                m_pcEntropyCoder->encodeQtCbfZero(pcCU, TEXT_CHROMA_V, uiTrModeC);
                const UInt uiNullBitsV = m_pcEntropyCoder->getNumberOfWrittenBits();
                minCostV = m_pcRdCost->calcRdCost(uiDistV, uiNullBitsV);
            }
            if (!uiAbsSumV)
            {
                Short *pcPtr =  m_pcQTTempTComYuv[uiQTTempAccessLayer].getCrAddr(absTUPartIdxC);
                const UInt uiStride = m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride();
                for (UInt uiY = 0; uiY < trHeightC; ++uiY)
                {
                    ::memset(pcPtr, 0, sizeof(Short) * trWidthC);
                    pcPtr += uiStride;
                }
            }
        }
        pcCU->setCbfSubParts(uiAbsSumY ? uiSetCbf : 0, TEXT_LUMA, uiAbsPartIdx, uiDepth);
        if (bCodeChroma)
        {
            pcCU->setCbfSubParts(uiAbsSumU ? uiSetCbf : 0, TEXT_CHROMA_U, uiAbsPartIdx, pcCU->getDepth(0) + uiTrModeC);
            pcCU->setCbfSubParts(uiAbsSumV ? uiSetCbf : 0, TEXT_CHROMA_V, uiAbsPartIdx, pcCU->getDepth(0) + uiTrModeC);
        }

        if (checkTransformSkipY)
        {
            UInt uiNonzeroDistY = 0, uiAbsSumTransformSkipY;
            UInt64 dSingleCostY = MAX_INT64;

            Short *pcResiCurrY = m_pcQTTempTComYuv[uiQTTempAccessLayer].getLumaAddr(absTUPartIdx);
            UInt resiYStride = m_pcQTTempTComYuv[uiQTTempAccessLayer].getStride();

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

            pcCU->setTransformSkipSubParts(1, TEXT_LUMA, uiAbsPartIdx, uiDepth);

            if (m_pcEncCfg->getUseRDOQTS())
            {
                m_pcEntropyCoder->estimateBit(m_pcTrQuant->m_pcEstBitsSbac, trWidth, trHeight, TEXT_LUMA);
            }

            m_pcTrQuant->setQPforQuant(pcCU->getQP(0), TEXT_LUMA, pcCU->getSlice()->getSPS()->getQpBDOffsetY(), 0);

            m_pcTrQuant->selectLambda(TEXT_LUMA);
            m_pcTrQuant->transformNxN(pcCU, pcResi->getLumaAddr(absTUPartIdx), pcResi->getStride(), pcCoeffCurrY,
                                      pcArlCoeffCurrY, trWidth, trHeight, uiAbsSumTransformSkipY, TEXT_LUMA, uiAbsPartIdx, true);
            pcCU->setCbfSubParts(uiAbsSumTransformSkipY ? uiSetCbf : 0, TEXT_LUMA, uiAbsPartIdx, uiDepth);

            if (uiAbsSumTransformSkipY != 0)
            {
                m_pcEntropyCoder->resetBits();
                m_pcEntropyCoder->encodeQtCbf(pcCU, uiAbsPartIdx, TEXT_LUMA, uiTrMode);
                m_pcEntropyCoder->encodeCoeffNxN(pcCU, pcCoeffCurrY, uiAbsPartIdx, trWidth, trHeight, uiDepth, TEXT_LUMA);
                const UInt uiTsSingleBitsY = m_pcEntropyCoder->getNumberOfWrittenBits();

                m_pcTrQuant->setQPforQuant(pcCU->getQP(0), TEXT_LUMA, pcCU->getSlice()->getSPS()->getQpBDOffsetY(), 0);

                Int scalingListType = 3 + g_eTTable[(Int)TEXT_LUMA];
                assert(scalingListType < 6);

                m_pcTrQuant->invtransformNxN(pcCU->getCUTransquantBypass(uiAbsPartIdx), TEXT_LUMA, REG_DCT, pcResiCurrY, m_pcQTTempTComYuv[uiQTTempAccessLayer].getStride(),  pcCoeffCurrY, trWidth, trHeight, scalingListType, true);

                uiNonzeroDistY = primitives.sse_ss[Part](pcResi->getLumaAddr(absTUPartIdx), pcResi->getStride(),
                                                         m_pcQTTempTComYuv[uiQTTempAccessLayer].getLumaAddr(absTUPartIdx), m_pcQTTempTComYuv[uiQTTempAccessLayer].getStride());

                dSingleCostY = m_pcRdCost->calcRdCost(uiNonzeroDistY, uiTsSingleBitsY);
            }

            if (!uiAbsSumTransformSkipY || minCostY < dSingleCostY)
            {
                pcCU->setTransformSkipSubParts(0, TEXT_LUMA, uiAbsPartIdx, uiDepth);
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

            pcCU->setCbfSubParts(uiAbsSumY ? uiSetCbf : 0, TEXT_LUMA, uiAbsPartIdx, uiDepth);
        }

        if (bCodeChroma && checkTransformSkipUV)
        {
            UInt uiNonzeroDistU = 0, uiNonzeroDistV = 0, uiAbsSumTransformSkipU, uiAbsSumTransformSkipV;
            UInt64 dSingleCostU = MAX_INT64;
            UInt64 dSingleCostV = MAX_INT64;

            Short *pcResiCurrU = m_pcQTTempTComYuv[uiQTTempAccessLayer].getCbAddr(absTUPartIdxC);
            Short *pcResiCurrV = m_pcQTTempTComYuv[uiQTTempAccessLayer].getCrAddr(absTUPartIdxC);
            UInt resiCStride = m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride();

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

            pcCU->setTransformSkipSubParts(1, TEXT_CHROMA_U, uiAbsPartIdx, pcCU->getDepth(0) + uiTrModeC);
            pcCU->setTransformSkipSubParts(1, TEXT_CHROMA_V, uiAbsPartIdx, pcCU->getDepth(0) + uiTrModeC);

            if (m_pcEncCfg->getUseRDOQTS())
            {
                m_pcEntropyCoder->estimateBit(m_pcTrQuant->m_pcEstBitsSbac, trWidthC, trHeightC, TEXT_CHROMA);
            }

            Int curChromaQpOffset = pcCU->getSlice()->getPPS()->getChromaCbQpOffset() + pcCU->getSlice()->getSliceQpDeltaCb();
            m_pcTrQuant->setQPforQuant(pcCU->getQP(0), TEXT_CHROMA, pcCU->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset);

            m_pcTrQuant->selectLambda(TEXT_CHROMA);

            m_pcTrQuant->transformNxN(pcCU, pcResi->getCbAddr(absTUPartIdxC), pcResi->getCStride(), pcCoeffCurrU,
                                      pcArlCoeffCurrU, trWidthC, trHeightC, uiAbsSumTransformSkipU, TEXT_CHROMA_U, uiAbsPartIdx, true);
            curChromaQpOffset = pcCU->getSlice()->getPPS()->getChromaCrQpOffset() + pcCU->getSlice()->getSliceQpDeltaCr();
            m_pcTrQuant->setQPforQuant(pcCU->getQP(0), TEXT_CHROMA, pcCU->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset);
            m_pcTrQuant->transformNxN(pcCU, pcResi->getCrAddr(absTUPartIdxC), pcResi->getCStride(), pcCoeffCurrV,
                                      pcArlCoeffCurrV, trWidthC, trHeightC, uiAbsSumTransformSkipV, TEXT_CHROMA_V, uiAbsPartIdx, true);

            pcCU->setCbfSubParts(uiAbsSumTransformSkipU ? uiSetCbf : 0, TEXT_CHROMA_U, uiAbsPartIdx, pcCU->getDepth(0) + uiTrModeC);
            pcCU->setCbfSubParts(uiAbsSumTransformSkipV ? uiSetCbf : 0, TEXT_CHROMA_V, uiAbsPartIdx, pcCU->getDepth(0) + uiTrModeC);

            m_pcEntropyCoder->resetBits();
            uiSingleBitsU = 0;
            uiSingleBitsV = 0;

            if (uiAbsSumTransformSkipU)
            {
                m_pcEntropyCoder->encodeQtCbf(pcCU, uiAbsPartIdx, TEXT_CHROMA_U, uiTrMode);
                m_pcEntropyCoder->encodeCoeffNxN(pcCU, pcCoeffCurrU, uiAbsPartIdx, trWidthC, trHeightC, uiDepth, TEXT_CHROMA_U);
                uiSingleBitsU = m_pcEntropyCoder->getNumberOfWrittenBits();

                curChromaQpOffset = pcCU->getSlice()->getPPS()->getChromaCbQpOffset() + pcCU->getSlice()->getSliceQpDeltaCb();
                m_pcTrQuant->setQPforQuant(pcCU->getQP(0), TEXT_CHROMA, pcCU->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset);

                Int scalingListType = 3 + g_eTTable[(Int)TEXT_CHROMA_U];
                assert(scalingListType < 6);

                m_pcTrQuant->invtransformNxN(pcCU->getCUTransquantBypass(uiAbsPartIdx), TEXT_CHROMA, REG_DCT, pcResiCurrU, m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride(), pcCoeffCurrU, trWidthC, trHeightC, scalingListType, true);

                uiNonzeroDistU = m_pcRdCost->scaleChromaDistCb(primitives.sse_ss[PartC](pcResi->getCbAddr(absTUPartIdxC), pcResi->getCStride(),
                                                                                        m_pcQTTempTComYuv[uiQTTempAccessLayer].getCbAddr(absTUPartIdxC), m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride()));
                dSingleCostU = m_pcRdCost->calcRdCost(uiNonzeroDistU, uiSingleBitsU);
            }

            if (!uiAbsSumTransformSkipU || minCostU < dSingleCostU)
            {
                pcCU->setTransformSkipSubParts(0, TEXT_CHROMA_U, uiAbsPartIdx, pcCU->getDepth(0) + uiTrModeC);

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
                m_pcEntropyCoder->encodeQtCbf(pcCU, uiAbsPartIdx, TEXT_CHROMA_V, uiTrMode);
                m_pcEntropyCoder->encodeCoeffNxN(pcCU, pcCoeffCurrV, uiAbsPartIdx, trWidthC, trHeightC, uiDepth, TEXT_CHROMA_V);
                uiSingleBitsV = m_pcEntropyCoder->getNumberOfWrittenBits() - uiSingleBitsU;

                curChromaQpOffset = pcCU->getSlice()->getPPS()->getChromaCrQpOffset() + pcCU->getSlice()->getSliceQpDeltaCr();
                m_pcTrQuant->setQPforQuant(pcCU->getQP(0), TEXT_CHROMA, pcCU->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset);

                Int scalingListType = 3 + g_eTTable[(Int)TEXT_CHROMA_V];
                assert(scalingListType < 6);

                m_pcTrQuant->invtransformNxN(pcCU->getCUTransquantBypass(uiAbsPartIdx), TEXT_CHROMA, REG_DCT, pcResiCurrV, m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride(), pcCoeffCurrV, trWidthC, trHeightC, scalingListType, true);

                uiNonzeroDistV = m_pcRdCost->scaleChromaDistCr(primitives.sse_ss[PartC](pcResi->getCrAddr(absTUPartIdxC), pcResi->getCStride(),
                                                                                        m_pcQTTempTComYuv[uiQTTempAccessLayer].getCrAddr(absTUPartIdxC), m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride()));
                dSingleCostV = m_pcRdCost->calcRdCost(uiNonzeroDistV, uiSingleBitsV);
            }

            if (!uiAbsSumTransformSkipV || minCostV < dSingleCostV)
            {
                pcCU->setTransformSkipSubParts(0, TEXT_CHROMA_V, uiAbsPartIdx, pcCU->getDepth(0) + uiTrModeC);

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

            pcCU->setCbfSubParts(uiAbsSumU ? uiSetCbf : 0, TEXT_CHROMA_U, uiAbsPartIdx, pcCU->getDepth(0) + uiTrModeC);
            pcCU->setCbfSubParts(uiAbsSumV ? uiSetCbf : 0, TEXT_CHROMA_V, uiAbsPartIdx, pcCU->getDepth(0) + uiTrModeC);
        }

        m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_QT_TRAFO_ROOT]);

        m_pcEntropyCoder->resetBits();

        {
            if (uiLog2TrSize > pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx))
            {
                m_pcEntropyCoder->encodeTransformSubdivFlag(0, 5 - uiLog2TrSize);
            }
        }

        {
            if (bCodeChroma)
            {
                m_pcEntropyCoder->encodeQtCbf(pcCU, uiAbsPartIdx, TEXT_CHROMA_U, uiTrMode);
                m_pcEntropyCoder->encodeQtCbf(pcCU, uiAbsPartIdx, TEXT_CHROMA_V, uiTrMode);
            }

            m_pcEntropyCoder->encodeQtCbf(pcCU, uiAbsPartIdx, TEXT_LUMA,     uiTrMode);
        }

        m_pcEntropyCoder->encodeCoeffNxN(pcCU, pcCoeffCurrY, uiAbsPartIdx, trWidth, trHeight,    uiDepth, TEXT_LUMA);

        if (bCodeChroma)
        {
            m_pcEntropyCoder->encodeCoeffNxN(pcCU, pcCoeffCurrU, uiAbsPartIdx, trWidthC, trHeightC, uiDepth, TEXT_CHROMA_U);
            m_pcEntropyCoder->encodeCoeffNxN(pcCU, pcCoeffCurrV, uiAbsPartIdx, trWidthC, trHeightC, uiDepth, TEXT_CHROMA_V);
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

        const UInt uiQPartNumSubdiv = pcCU->getPic()->getNumPartInCU() >> ((uiDepth + 1) << 1);
        for (UInt ui = 0; ui < 4; ++ui)
        {
            UInt nsAddr = uiAbsPartIdx + ui * uiQPartNumSubdiv;
            xEstimateResidualQT(pcCU, ui, uiAbsPartIdx + ui * uiQPartNumSubdiv, nsAddr, pcResi, uiDepth + 1, dSubdivCost, uiSubdivBits, uiSubdivDist, bCheckFull ? NULL : puiZeroDist);
        }

        UInt uiYCbf = 0;
        UInt uiUCbf = 0;
        UInt uiVCbf = 0;
        for (UInt ui = 0; ui < 4; ++ui)
        {
            uiYCbf |= pcCU->getCbf(uiAbsPartIdx + ui * uiQPartNumSubdiv, TEXT_LUMA,     uiTrMode + 1);
            uiUCbf |= pcCU->getCbf(uiAbsPartIdx + ui * uiQPartNumSubdiv, TEXT_CHROMA_U, uiTrMode + 1);
            uiVCbf |= pcCU->getCbf(uiAbsPartIdx + ui * uiQPartNumSubdiv, TEXT_CHROMA_V, uiTrMode + 1);
        }

        for (UInt ui = 0; ui < 4 * uiQPartNumSubdiv; ++ui)
        {
            pcCU->getCbf(TEXT_LUMA)[uiAbsPartIdx + ui] |= uiYCbf << uiTrMode;
            pcCU->getCbf(TEXT_CHROMA_U)[uiAbsPartIdx + ui] |= uiUCbf << uiTrMode;
            pcCU->getCbf(TEXT_CHROMA_V)[uiAbsPartIdx + ui] |= uiVCbf << uiTrMode;
        }

        m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_QT_TRAFO_ROOT]);
        m_pcEntropyCoder->resetBits();

        {
            xEncodeResidualQT(pcCU, uiAbsPartIdx, uiDepth, true,  TEXT_LUMA);
            xEncodeResidualQT(pcCU, uiAbsPartIdx, uiDepth, false, TEXT_LUMA);
            xEncodeResidualQT(pcCU, uiAbsPartIdx, uiDepth, false, TEXT_CHROMA_U);
            xEncodeResidualQT(pcCU, uiAbsPartIdx, uiDepth, false, TEXT_CHROMA_V);
        }

        uiSubdivBits = m_pcEntropyCoder->getNumberOfWrittenBits();
        dSubdivCost  = m_pcRdCost->calcRdCost(uiSubdivDist, uiSubdivBits);

        if (uiYCbf || uiUCbf || uiVCbf || !bCheckFull)
        {
            if (dSubdivCost < dSingleCost)
            {
                rdCost += dSubdivCost;
                ruiBits += uiSubdivBits;
                ruiDist += uiSubdivDist;
                return;
            }
        }
        pcCU->setTransformSkipSubParts(uiBestTransformMode[0], TEXT_LUMA, uiAbsPartIdx, uiDepth);
        if (bCodeChroma)
        {
            pcCU->setTransformSkipSubParts(uiBestTransformMode[1], TEXT_CHROMA_U, uiAbsPartIdx, pcCU->getDepth(0) + uiTrModeC);
            pcCU->setTransformSkipSubParts(uiBestTransformMode[2], TEXT_CHROMA_V, uiAbsPartIdx, pcCU->getDepth(0) + uiTrModeC);
        }
        assert(bCheckFull);
        m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_QT_TRAFO_TEST]);
    }
    rdCost += dSingleCost;
    ruiBits += uiSingleBits;
    ruiDist += uiSingleDist;

    pcCU->setTrIdxSubParts(uiTrMode, uiAbsPartIdx, uiDepth);

    pcCU->setCbfSubParts(uiAbsSumY ? uiSetCbf : 0, TEXT_LUMA, uiAbsPartIdx, uiDepth);
    if (bCodeChroma)
    {
        pcCU->setCbfSubParts(uiAbsSumU ? uiSetCbf : 0, TEXT_CHROMA_U, uiAbsPartIdx, pcCU->getDepth(0) + uiTrModeC);
        pcCU->setCbfSubParts(uiAbsSumV ? uiSetCbf : 0, TEXT_CHROMA_V, uiAbsPartIdx, pcCU->getDepth(0) + uiTrModeC);
    }
}

Void TEncSearch::xEncodeResidualQT(TComDataCU* pcCU, UInt uiAbsPartIdx, const UInt uiDepth, Bool bSubdivAndCbf, TextType eType)
{
    assert(pcCU->getDepth(0) == pcCU->getDepth(uiAbsPartIdx));
    const UInt uiCurrTrMode = uiDepth - pcCU->getDepth(0);
    const UInt uiTrMode = pcCU->getTransformIdx(uiAbsPartIdx);

    const Bool bSubdiv = uiCurrTrMode != uiTrMode;

    const UInt uiLog2TrSize = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiDepth] + 2;

    {
        if (bSubdivAndCbf && uiLog2TrSize <= pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() && uiLog2TrSize > pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx))
        {
            m_pcEntropyCoder->encodeTransformSubdivFlag(bSubdiv, 5 - uiLog2TrSize);
        }
    }

    {
        assert(pcCU->getPredictionMode(uiAbsPartIdx) != MODE_INTRA);
        if (bSubdivAndCbf)
        {
            const Bool bFirstCbfOfCU = uiCurrTrMode == 0;
            if (bFirstCbfOfCU || uiLog2TrSize > 2)
            {
                if (bFirstCbfOfCU || pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_U, uiCurrTrMode - 1))
                {
                    m_pcEntropyCoder->encodeQtCbf(pcCU, uiAbsPartIdx, TEXT_CHROMA_U, uiCurrTrMode);
                }
                if (bFirstCbfOfCU || pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_V, uiCurrTrMode - 1))
                {
                    m_pcEntropyCoder->encodeQtCbf(pcCU, uiAbsPartIdx, TEXT_CHROMA_V, uiCurrTrMode);
                }
            }
            else if (uiLog2TrSize == 2)
            {
                assert(pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_U, uiCurrTrMode) == pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_U, uiCurrTrMode - 1));
                assert(pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_V, uiCurrTrMode) == pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_V, uiCurrTrMode - 1));
            }
        }
    }

    if (!bSubdiv)
    {
        const UInt uiNumCoeffPerAbsPartIdxIncrement = pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() >> (pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1);
        //assert( 16 == uiNumCoeffPerAbsPartIdxIncrement ); // check
        const UInt uiQTTempAccessLayer = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;
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
            UInt  uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ((pcCU->getDepth(0) + uiTrModeC) << 1);
            bCodeChroma   = ((uiAbsPartIdx % uiQPDiv) == 0);
        }

        if (bSubdivAndCbf)
        {
            {
                m_pcEntropyCoder->encodeQtCbf(pcCU, uiAbsPartIdx, TEXT_LUMA,     uiTrMode);
            }
        }
        else
        {
            if (eType == TEXT_LUMA     && pcCU->getCbf(uiAbsPartIdx, TEXT_LUMA,     uiTrMode))
            {
                Int trWidth  = 1 << uiLog2TrSize;
                Int trHeight = 1 << uiLog2TrSize;
                m_pcEntropyCoder->encodeCoeffNxN(pcCU, pcCoeffCurrY, uiAbsPartIdx, trWidth, trHeight,    uiDepth, TEXT_LUMA);
            }
            if (bCodeChroma)
            {
                Int trWidth  = 1 << uiLog2TrSizeC;
                Int trHeight = 1 << uiLog2TrSizeC;
                if (eType == TEXT_CHROMA_U && pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_U, uiTrMode))
                {
                    m_pcEntropyCoder->encodeCoeffNxN(pcCU, pcCoeffCurrU, uiAbsPartIdx, trWidth, trHeight, uiDepth, TEXT_CHROMA_U);
                }
                if (eType == TEXT_CHROMA_V && pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_V, uiTrMode))
                {
                    m_pcEntropyCoder->encodeCoeffNxN(pcCU, pcCoeffCurrV, uiAbsPartIdx, trWidth, trHeight, uiDepth, TEXT_CHROMA_V);
                }
            }
        }
    }
    else
    {
        if (bSubdivAndCbf || pcCU->getCbf(uiAbsPartIdx, eType, uiCurrTrMode))
        {
            const UInt uiQPartNumSubdiv = pcCU->getPic()->getNumPartInCU() >> ((uiDepth + 1) << 1);
            for (UInt ui = 0; ui < 4; ++ui)
            {
                xEncodeResidualQT(pcCU, uiAbsPartIdx + ui * uiQPartNumSubdiv, uiDepth + 1, bSubdivAndCbf, eType);
            }
        }
    }
}

Void TEncSearch::xSetResidualQTData(TComDataCU* pcCU, UInt uiQuadrant, UInt uiAbsPartIdx, UInt absTUPartIdx, TShortYUV* pcResi, UInt uiDepth, Bool bSpatial)
{
    assert(pcCU->getDepth(0) == pcCU->getDepth(uiAbsPartIdx));
    const UInt uiCurrTrMode = uiDepth - pcCU->getDepth(0);
    const UInt uiTrMode = pcCU->getTransformIdx(uiAbsPartIdx);

    if (uiCurrTrMode == uiTrMode)
    {
        const UInt uiLog2TrSize = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiDepth] + 2;
        const UInt uiQTTempAccessLayer = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;

        Bool  bCodeChroma   = true;
        UInt  uiTrModeC     = uiTrMode;
        UInt  uiLog2TrSizeC = uiLog2TrSize - 1;
        if (uiLog2TrSize == 2)
        {
            uiLog2TrSizeC++;
            uiTrModeC--;
            UInt  uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ((pcCU->getDepth(0) + uiTrModeC) << 1);
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
            UInt    uiNumCoeffPerAbsPartIdxIncrement = pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() >> (pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1);
            UInt    uiNumCoeffY = (1 << (uiLog2TrSize << 1));
            TCoeff* pcCoeffSrcY = m_ppcQTTempCoeffY[uiQTTempAccessLayer] +  uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx;
            TCoeff* pcCoeffDstY = pcCU->getCoeffY() + uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx;
            ::memcpy(pcCoeffDstY, pcCoeffSrcY, sizeof(TCoeff) * uiNumCoeffY);
            Int* pcArlCoeffSrcY = m_ppcQTTempArlCoeffY[uiQTTempAccessLayer] +  uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx;
            Int* pcArlCoeffDstY = pcCU->getArlCoeffY() + uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx;
            ::memcpy(pcArlCoeffDstY, pcArlCoeffSrcY, sizeof(Int) * uiNumCoeffY);
            if (bCodeChroma)
            {
                UInt    uiNumCoeffC = (1 << (uiLog2TrSizeC << 1));
                TCoeff* pcCoeffSrcU = m_ppcQTTempCoeffCb[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx >> 2);
                TCoeff* pcCoeffSrcV = m_ppcQTTempCoeffCr[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx >> 2);
                TCoeff* pcCoeffDstU = pcCU->getCoeffCb() + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx >> 2);
                TCoeff* pcCoeffDstV = pcCU->getCoeffCr() + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx >> 2);
                ::memcpy(pcCoeffDstU, pcCoeffSrcU, sizeof(TCoeff) * uiNumCoeffC);
                ::memcpy(pcCoeffDstV, pcCoeffSrcV, sizeof(TCoeff) * uiNumCoeffC);
                Int* pcArlCoeffSrcU = m_ppcQTTempArlCoeffCb[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx >> 2);
                Int* pcArlCoeffSrcV = m_ppcQTTempArlCoeffCr[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx >> 2);
                Int* pcArlCoeffDstU = pcCU->getArlCoeffCb() + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx >> 2);
                Int* pcArlCoeffDstV = pcCU->getArlCoeffCr() + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx >> 2);
                ::memcpy(pcArlCoeffDstU, pcArlCoeffSrcU, sizeof(Int) * uiNumCoeffC);
                ::memcpy(pcArlCoeffDstV, pcArlCoeffSrcV, sizeof(Int) * uiNumCoeffC);
            }
        }
    }
    else
    {
        const UInt uiQPartNumSubdiv = pcCU->getPic()->getNumPartInCU() >> ((uiDepth + 1) << 1);
        for (UInt ui = 0; ui < 4; ++ui)
        {
            UInt nsAddr = uiAbsPartIdx + ui * uiQPartNumSubdiv;
            xSetResidualQTData(pcCU, ui, uiAbsPartIdx + ui * uiQPartNumSubdiv, nsAddr, pcResi, uiDepth + 1, bSpatial);
        }
    }
}

UInt TEncSearch::xModeBitsIntra(TComDataCU* pcCU, UInt uiMode, UInt uiPU, UInt uiPartOffset, UInt uiDepth, UInt uiInitTrDepth)
{
    // Reload only contexts required for coding intra mode information
    m_pcRDGoOnSbacCoder->loadIntraDirModeLuma(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);

    pcCU->setLumaIntraDirSubParts(uiMode, uiPartOffset, uiDepth + uiInitTrDepth);

    m_pcEntropyCoder->resetBits();
    m_pcEntropyCoder->encodeIntraDirModeLuma(pcCU, uiPartOffset);

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
 * \param pcCU
 * \param uiQp
 * \param uiTrMode
 * \param ruiBits
 * \param rpcYuvRec
 * \param pcYuvPred
 * \param rpcYuvResi
 * \returns Void
 */
Void  TEncSearch::xAddSymbolBitsInter(TComDataCU* pcCU, UInt uiQp, UInt uiTrMode, UInt& ruiBits, TShortYUV*& rpcYuvRec, TComYuv*pcYuvPred, TShortYUV*& rpcYuvResi)
{
    if (pcCU->getMergeFlag(0) && pcCU->getPartitionSize(0) == SIZE_2Nx2N && !pcCU->getQtRootCbf(0))
    {
        pcCU->setSkipFlagSubParts(true, 0, pcCU->getDepth(0));

        m_pcEntropyCoder->resetBits();
        if (pcCU->getSlice()->getPPS()->getTransquantBypassEnableFlag())
        {
            m_pcEntropyCoder->encodeCUTransquantBypassFlag(pcCU, 0, true);
        }
        m_pcEntropyCoder->encodeSkipFlag(pcCU, 0, true);
        m_pcEntropyCoder->encodeMergeIndex(pcCU, 0, true);
        ruiBits += m_pcEntropyCoder->getNumberOfWrittenBits();
    }
    else
    {
        m_pcEntropyCoder->resetBits();
        if (pcCU->getSlice()->getPPS()->getTransquantBypassEnableFlag())
        {
            m_pcEntropyCoder->encodeCUTransquantBypassFlag(pcCU, 0, true);
        }
        m_pcEntropyCoder->encodeSkipFlag(pcCU, 0, true);
        m_pcEntropyCoder->encodePredMode(pcCU, 0, true);
        m_pcEntropyCoder->encodePartSize(pcCU, 0, pcCU->getDepth(0), true);
        m_pcEntropyCoder->encodePredInfo(pcCU, 0, true);
        Bool bDummy = false;
        m_pcEntropyCoder->encodeCoeff(pcCU, 0, pcCU->getDepth(0), pcCU->getWidth(0), pcCU->getHeight(0), bDummy);

        ruiBits += m_pcEntropyCoder->getNumberOfWrittenBits();
    }
}

/**** Function to estimate the header bits ************/
UInt  TEncSearch::estimateHeaderBits(TComDataCU* pcCU, UInt uiAbsPartIdx)
{
    UInt bits = 0;

    m_pcEntropyCoder->resetBits();

    UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[g_auiZscanToRaster[uiAbsPartIdx]];
    UInt uiRPelX   = uiLPelX + (g_uiMaxCUWidth >> pcCU->getDepth(0))  - 1;
    UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[g_auiZscanToRaster[uiAbsPartIdx]];
    UInt uiBPelY   = uiTPelY + (g_uiMaxCUHeight >>  pcCU->getDepth(0)) - 1;

    TComSlice * pcSlice = pcCU->getPic()->getSlice();
    if ((uiRPelX < pcSlice->getSPS()->getPicWidthInLumaSamples()) && (uiBPelY < pcSlice->getSPS()->getPicHeightInLumaSamples()))
    {
        m_pcEntropyCoder->encodeSplitFlag(pcCU, uiAbsPartIdx,  pcCU->getDepth(0));
    }

    if (pcCU->getMergeFlag(0) && pcCU->getPartitionSize(0) == SIZE_2Nx2N && !pcCU->getQtRootCbf(0))
    {
        m_pcEntropyCoder->encodeMergeFlag(pcCU, 0);
        m_pcEntropyCoder->encodeMergeIndex(pcCU, 0, true);
    }

    if (pcCU->getSlice()->getPPS()->getTransquantBypassEnableFlag())
    {
        m_pcEntropyCoder->encodeCUTransquantBypassFlag(pcCU, 0, true);
    }

    if (!pcCU->getSlice()->isIntra())
    {
        m_pcEntropyCoder->encodeSkipFlag(pcCU, 0, true);
    }
       
    m_pcEntropyCoder->encodePredMode(pcCU, 0, true);
        
    m_pcEntropyCoder->encodePartSize(pcCU, 0, pcCU->getDepth(0), true);
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

    Int intStride = filteredBlockTmp[0].getWidth();
    Int dstStride = m_filteredBlock[0][0].getStride();

    Pel *srcPtr;    //Contains raw pixels
    Short *intPtr;  // Intermediate results in short
    Pel *dstPtr;    // Final filtered blocks in Pel

    Int filterSize = NTAPS_LUMA;
    Int halfFilterSize = (filterSize >> 1);

    srcPtr = (Pel*)pattern->getROIY() - halfFilterSize * srcStride - 1;

    dstPtr = m_filteredBlock[0][0].getLumaAddr();

    primitives.cpyblock(width, height, (pixel*)dstPtr, dstStride, (pixel*)(srcPtr + halfFilterSize * srcStride + 1), srcStride);

    intPtr = filteredBlockTmp[0].getLumaAddr();
    primitives.ipfilterConvert_p_s(g_bitDepthY, (pixel*)srcPtr, srcStride, intPtr,
                                   intStride, width + 1, height + filterSize);

    intPtr = filteredBlockTmp[0].getLumaAddr() + (halfFilterSize - 1) * intStride + 1;
    dstPtr = m_filteredBlock[2][0].getLumaAddr();
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr,
                                            dstStride, width, height + 1, m_if.m_lumaFilter[2]);

    intPtr = filteredBlockTmp[2].getLumaAddr();
    primitives.ipFilter_p_s[FILTER_H_P_S_8](g_bitDepthY, (pixel*)srcPtr, srcStride, intPtr, intStride, width + 1, height + filterSize,  m_if.m_lumaFilter[2]);

    intPtr = filteredBlockTmp[2].getLumaAddr() + halfFilterSize * intStride;
    dstPtr = m_filteredBlock[0][2].getLumaAddr();
    primitives.ipfilterConvert_s_p(g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, dstStride, width + 1, height + 0);

    intPtr = filteredBlockTmp[2].getLumaAddr() + (halfFilterSize - 1) * intStride;
    dstPtr = m_filteredBlock[2][2].getLumaAddr();
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, dstStride, width + 1, height + 1, m_if.m_lumaFilter[2]);
}

/**
 * \brief Generate quarter-sample interpolated blocks
 *
 * \param pattern    Reference picture ROI
 * \param halfPelRef Half-pel mv
 * \param biPred     Flag indicating whether block is for biprediction
 */
Void TEncSearch::xExtDIFUpSamplingQ(TComPattern* pattern, TComMv halfPelRef, Bool biPred)
{
    assert(g_bitDepthY == 8);

    Int width      = pattern->getROIYWidth();
    Int height     = pattern->getROIYHeight();
    Int srcStride  = pattern->getPatternLStride();

    Int intStride = filteredBlockTmp[0].getWidth();
    Int dstStride = m_filteredBlock[0][0].getStride();

    Pel *srcPtr;    //Contains raw pixels
    Short *intPtr;  // Intermediate results in short
    Pel *dstPtr;    // Final filtered blocks in Pel

    Int filterSize = NTAPS_LUMA;

    Int halfFilterSize = (filterSize >> 1);

    Int extHeight = (halfPelRef.getVer() == 0) ? height + filterSize : height + filterSize - 1;

    // Horizontal filter 1/4
    srcPtr = pattern->getROIY() - halfFilterSize * srcStride - 1;
    intPtr = filteredBlockTmp[1].getLumaAddr();
    if (halfPelRef.getVer() > 0)
    {
        srcPtr += srcStride;
    }
    if (halfPelRef.getHor() >= 0)
    {
        srcPtr += 1;
    }
    primitives.ipFilter_p_s[FILTER_H_P_S_8](g_bitDepthY, (pixel*)srcPtr, srcStride, intPtr, intStride, width, extHeight, m_if.m_lumaFilter[1]);

    // Horizontal filter 3/4
    srcPtr = pattern->getROIY() - halfFilterSize * srcStride - 1;
    intPtr = filteredBlockTmp[3].getLumaAddr();
    if (halfPelRef.getVer() > 0)
    {
        srcPtr += srcStride;
    }
    if (halfPelRef.getHor() > 0)
    {
        srcPtr += 1;
    }
    primitives.ipFilter_p_s[FILTER_H_P_S_8](g_bitDepthY, (pixel*)srcPtr, srcStride, intPtr, intStride, width, extHeight, m_if.m_lumaFilter[3]);

    // Generate @ 1,1
    intPtr = filteredBlockTmp[1].getLumaAddr() + (halfFilterSize - 1) * intStride;
    dstPtr = m_filteredBlock[1][1].getLumaAddr();
    if (halfPelRef.getVer() == 0)
    {
        intPtr += intStride;
    }
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, dstStride, width, height, m_if.m_lumaFilter[1]);

    // Generate @ 3,1
    intPtr = filteredBlockTmp[1].getLumaAddr() + (halfFilterSize - 1) * intStride;
    dstPtr = m_filteredBlock[3][1].getLumaAddr();
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, dstStride, width, height, m_if.m_lumaFilter[3]);

    if (halfPelRef.getVer() != 0)
    {
        // Generate @ 2,1
        intPtr = filteredBlockTmp[1].getLumaAddr() + (halfFilterSize - 1) * intStride;
        dstPtr = m_filteredBlock[2][1].getLumaAddr();
        if (halfPelRef.getVer() == 0)
        {
            intPtr += intStride;
        }
        primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, dstStride, width, height, m_if.m_lumaFilter[2]);

        // Generate @ 2,3
        intPtr = filteredBlockTmp[3].getLumaAddr() + (halfFilterSize - 1) * intStride;
        dstPtr = m_filteredBlock[2][3].getLumaAddr();
        if (halfPelRef.getVer() == 0)
        {
            intPtr += intStride;
        }
        primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, dstStride, width, height, m_if.m_lumaFilter[2]);
    }
    else
    {
        // Generate @ 0,1
        intPtr = filteredBlockTmp[1].getLumaAddr() + halfFilterSize * intStride;
        dstPtr = m_filteredBlock[0][1].getLumaAddr();
        primitives.ipfilterConvert_s_p(g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, dstStride, width, height);

        // Generate @ 0,3
        intPtr = filteredBlockTmp[3].getLumaAddr() + halfFilterSize * intStride;
        dstPtr = m_filteredBlock[0][3].getLumaAddr();
        primitives.ipfilterConvert_s_p(g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, dstStride, width, height);
    }

    if (halfPelRef.getHor() != 0)
    {
        // Generate @ 1,2
        intPtr = filteredBlockTmp[2].getLumaAddr() + (halfFilterSize - 1) * intStride;
        dstPtr = m_filteredBlock[1][2].getLumaAddr();
        if (halfPelRef.getHor() > 0)
        {
            intPtr += 1;
        }
        if (halfPelRef.getVer() >= 0)
        {
            intPtr += intStride;
        }

        primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, dstStride, width, height, m_if.m_lumaFilter[1]);

        // Generate @ 3,2
        intPtr = filteredBlockTmp[2].getLumaAddr() + (halfFilterSize - 1) * intStride;
        dstPtr = m_filteredBlock[3][2].getLumaAddr();
        if (halfPelRef.getHor() > 0)
        {
            intPtr += 1;
        }
        if (halfPelRef.getVer() > 0)
        {
            intPtr += intStride;
        }
        primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, dstStride, width, height, m_if.m_lumaFilter[3]);
    }
    else
    {
        // Generate @ 1,0
        intPtr = filteredBlockTmp[0].getLumaAddr() + (halfFilterSize - 1) * intStride + 1;
        dstPtr = m_filteredBlock[1][0].getLumaAddr();
        if (halfPelRef.getVer() >= 0)
        {
            intPtr += intStride;
        }
        primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, dstStride, width, height, m_if.m_lumaFilter[1]);

        // Generate @ 3,0
        intPtr = filteredBlockTmp[0].getLumaAddr() + (halfFilterSize - 1) * intStride + 1;
        dstPtr = (Pel*)m_filteredBlock[3][0].getLumaAddr();
        if (halfPelRef.getVer() > 0)
        {
            intPtr += intStride;
        }
        primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, dstStride, width, height, m_if.m_lumaFilter[3]);
    }

    // Generate @ 1,3
    intPtr = filteredBlockTmp[3].getLumaAddr() + (halfFilterSize - 1) * intStride;
    dstPtr = m_filteredBlock[1][3].getLumaAddr();
    if (halfPelRef.getVer() == 0)
    {
        intPtr += intStride;
    }
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, dstStride, width, height, m_if.m_lumaFilter[1]);

    // Generate @ 3,3
    intPtr = filteredBlockTmp[3].getLumaAddr() + (halfFilterSize - 1) * intStride;
    dstPtr = m_filteredBlock[3][3].getLumaAddr();
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, dstStride, width, height, m_if.m_lumaFilter[3]);
}

/** set wp tables
 * \param TComDataCU* pcCU
 * \param iRefIdx
 * \param eRefPicListCur
 * \returns Void
 */
Void  TEncSearch::setWpScalingDistParam(TComDataCU* pcCU, Int iRefIdx, RefPicList eRefPicListCur)
{
    if (iRefIdx < 0)
    {
        m_cDistParam.bApplyWeight = false;
        return;
    }

    TComSlice       *pcSlice  = pcCU->getSlice();
    TComPPS         *pps      = pcCU->getSlice()->getPPS();
    wpScalingParam  *wp0, *wp1;
    m_cDistParam.bApplyWeight = (pcSlice->getSliceType() == P_SLICE && pps->getUseWP()) || (pcSlice->getSliceType() == B_SLICE && pps->getWPBiPred());
    if (!m_cDistParam.bApplyWeight) return;

    Int iRefIdx0 = (eRefPicListCur == REF_PIC_LIST_0) ? iRefIdx : (-1);
    Int iRefIdx1 = (eRefPicListCur == REF_PIC_LIST_1) ? iRefIdx : (-1);

    getWpScaling(pcCU, iRefIdx0, iRefIdx1, wp0, wp1);

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
