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

TEncSearch::TEncSearch()
{
    m_qtTempCoeffY  = NULL;
    m_qtTempCoeffCb = NULL;
    m_qtTempCoeffCr = NULL;
    m_qtTempTrIdx = NULL;
    m_qtTempTComYuv = NULL;
    m_qtTempTUCoeffY  = NULL;
    m_qtTempTUCoeffCb = NULL;
    m_qtTempTUCoeffCr = NULL;
    for (int i = 0; i < 3; i++)
    {
        m_sharedPredTransformSkip[i] = NULL;
        m_qtTempCbf[i] = NULL;
        m_qtTempTransformSkipFlag[i] = NULL;
    }

    m_cfg = NULL;
    m_rdCost  = NULL;
    m_trQuant = NULL;
    m_tempPel = NULL;
    m_entropyCoder = NULL;
    m_rdSbacCoders    = NULL;
    m_rdGoOnSbacCoder = NULL;
    setWpScalingDistParam(NULL, -1, REF_PIC_LIST_X);
}

TEncSearch::~TEncSearch()
{
    delete [] m_tempPel;

    if (m_cfg)
    {
        const UInt numLayersToAllocate = m_cfg->getQuadtreeTULog2MaxSize() - m_cfg->getQuadtreeTULog2MinSize() + 1;
        for (UInt i = 0; i < numLayersToAllocate; ++i)
        {
            delete[] m_qtTempCoeffY[i];
            delete[] m_qtTempCoeffCb[i];
            delete[] m_qtTempCoeffCr[i];
            m_qtTempTComYuv[i].destroy();
        }
    }
    delete[] m_qtTempCoeffY;
    delete[] m_qtTempCoeffCb;
    delete[] m_qtTempCoeffCr;
    delete[] m_qtTempTrIdx;
    delete[] m_qtTempTComYuv;
    delete[] m_qtTempTUCoeffY;
    delete[] m_qtTempTUCoeffCb;
    delete[] m_qtTempTUCoeffCr;
    for (UInt i = 0; i < 3; ++i)
    {
        delete[] m_qtTempCbf[i];
        delete[] m_sharedPredTransformSkip[i];
        delete[] m_qtTempTransformSkipFlag[i];
    }

    m_qtTempTransformSkipTComYuv.destroy();
    m_tmpYuvPred.destroy();
}

void TEncSearch::init(TEncCfg* cfg, TComRdCost* rdCost, TComTrQuant* trQuant)
{
    m_cfg     = cfg;
    m_trQuant = trQuant;
    m_rdCost  = rdCost;

    m_me.setSearchMethod(cfg->param.searchMethod);
    m_me.setSubpelRefine(cfg->param.subpelRefine);

    /* When frame parallelism is active, only 'refLagPixels' of reference frames will be guaranteed
     * available for motion reference.  See refLagRows in FrameEncoder::compressCTURows() */
    m_refLagPixels = cfg->param.frameNumThreads > 1 ? cfg->param.searchRange : cfg->param.sourceHeight;

    // default to no adaptive range
    for (int dir = 0; dir < 2; dir++)
    {
        for (int ref = 0; ref < 33; ref++)
        {
            m_adaptiveRange[dir][ref] = cfg->param.searchRange;
        }
    }

    // initialize motion cost
    for (int num = 0; num < AMVP_MAX_NUM_CANDS + 1; num++)
    {
        for (int idx = 0; idx < AMVP_MAX_NUM_CANDS; idx++)
        {
            if (idx < num)
                m_mvpIdxCost[idx][num] = xGetMvpIdxBits(idx, num);
            else
                m_mvpIdxCost[idx][num] = MAX_INT;
        }
    }

    initTempBuff();

    m_tempPel = new Pel[g_maxCUWidth * g_maxCUHeight];

    const UInt numLayersToAllocate = cfg->getQuadtreeTULog2MaxSize() - cfg->getQuadtreeTULog2MinSize() + 1;
    m_qtTempCoeffY  = new TCoeff*[numLayersToAllocate];
    m_qtTempCoeffCb = new TCoeff*[numLayersToAllocate];
    m_qtTempCoeffCr = new TCoeff*[numLayersToAllocate];

    const UInt numPartitions = 1 << (g_maxCUDepth << 1);
    m_qtTempTrIdx   = new UChar[numPartitions];
    m_qtTempCbf[0]  = new UChar[numPartitions];
    m_qtTempCbf[1]  = new UChar[numPartitions];
    m_qtTempCbf[2]  = new UChar[numPartitions];
    m_qtTempTComYuv  = new TShortYUV[numLayersToAllocate];
    for (UInt i = 0; i < numLayersToAllocate; ++i)
    {
        m_qtTempCoeffY[i]  = new TCoeff[g_maxCUWidth * g_maxCUHeight];
        m_qtTempCoeffCb[i] = new TCoeff[g_maxCUWidth * g_maxCUHeight >> 2];
        m_qtTempCoeffCr[i] = new TCoeff[g_maxCUWidth * g_maxCUHeight >> 2];
        m_qtTempTComYuv[i].create(g_maxCUWidth, g_maxCUHeight);
    }

    m_sharedPredTransformSkip[0] = new Pel[MAX_TS_WIDTH * MAX_TS_HEIGHT];
    m_sharedPredTransformSkip[1] = new Pel[MAX_TS_WIDTH * MAX_TS_HEIGHT];
    m_sharedPredTransformSkip[2] = new Pel[MAX_TS_WIDTH * MAX_TS_HEIGHT];
    m_qtTempTUCoeffY  = new TCoeff[MAX_TS_WIDTH * MAX_TS_HEIGHT];
    m_qtTempTUCoeffCb = new TCoeff[MAX_TS_WIDTH * MAX_TS_HEIGHT];
    m_qtTempTUCoeffCr = new TCoeff[MAX_TS_WIDTH * MAX_TS_HEIGHT];
    m_qtTempTransformSkipTComYuv.create(g_maxCUWidth, g_maxCUHeight);

    m_qtTempTransformSkipFlag[0] = new UChar[numPartitions];
    m_qtTempTransformSkipFlag[1] = new UChar[numPartitions];
    m_qtTempTransformSkipFlag[2] = new UChar[numPartitions];
    m_tmpYuvPred.create(MAX_CU_SIZE, MAX_CU_SIZE);
}

void TEncSearch::setQPLambda(int QP, double lambdaLuma, double lambdaChroma)
{
    m_trQuant->setLambda(lambdaLuma, lambdaChroma);
    m_me.setQP(QP);
}

void TEncSearch::xEncSubdivCbfQT(TComDataCU* cu, UInt trDepth, UInt absPartIdx, bool bLuma, bool bChroma)
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
            m_entropyCoder->encodeTransformSubdivFlag(subdiv, 5 - trSizeLog2);
        }
    }

    if (bChroma)
    {
        if (trSizeLog2 > 2)
        {
            if (trDepth == 0 || cu->getCbf(absPartIdx, TEXT_CHROMA_U, trDepth - 1))
                m_entropyCoder->encodeQtCbf(cu, absPartIdx, TEXT_CHROMA_U, trDepth);
            if (trDepth == 0 || cu->getCbf(absPartIdx, TEXT_CHROMA_V, trDepth - 1))
                m_entropyCoder->encodeQtCbf(cu, absPartIdx, TEXT_CHROMA_V, trDepth);
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
        m_entropyCoder->encodeQtCbf(cu, absPartIdx, TEXT_LUMA, trMode);
    }
}

void TEncSearch::xEncCoeffQT(TComDataCU* cu, UInt trDepth, UInt absPartIdx, TextType ttype)
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
        bool bFirstQ = ((absPartIdx % qpdiv) == 0);
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
    case TEXT_LUMA:     coeff = m_qtTempCoeffY[qtLayer];
        break;
    case TEXT_CHROMA_U: coeff = m_qtTempCoeffCb[qtLayer];
        break;
    case TEXT_CHROMA_V: coeff = m_qtTempCoeffCr[qtLayer];
        break;
    default: assert(0);
    }

    coeff += coeffOffset;

    m_entropyCoder->encodeCoeffNxN(cu, coeff, absPartIdx, width, height, fullDepth, ttype);
}

void TEncSearch::xEncIntraHeader(TComDataCU* cu, UInt trDepth, UInt absPartIdx, bool bLuma, bool bChroma)
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
                    m_entropyCoder->encodeCUTransquantBypassFlag(cu, 0, true);
                }
                m_entropyCoder->encodeSkipFlag(cu, 0, true);
                m_entropyCoder->encodePredMode(cu, 0, true);
            }

            m_entropyCoder->encodePartSize(cu, 0, cu->getDepth(0), true);

            if (cu->isIntra(0) && cu->getPartitionSize(0) == SIZE_2Nx2N)
            {
                m_entropyCoder->encodeIPCMInfo(cu, 0, true);

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
                m_entropyCoder->encodeIntraDirModeLuma(cu, 0);
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
                    m_entropyCoder->encodeIntraDirModeLuma(cu, part * qtNumParts);
                }
            }
            else if ((absPartIdx % qtNumParts) == 0)
            {
                m_entropyCoder->encodeIntraDirModeLuma(cu, absPartIdx);
            }
        }
    }
    if (bChroma)
    {
        // chroma prediction mode
        if (absPartIdx == 0)
        {
            m_entropyCoder->encodeIntraDirModeChroma(cu, 0, true);
        }
    }
}

UInt TEncSearch::xGetIntraBitsQT(TComDataCU* cu, UInt trDepth, UInt absPartIdx, bool bLuma, bool bChroma)
{
    m_entropyCoder->resetBits();
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
    return m_entropyCoder->getNumberOfWrittenBits();
}

UInt TEncSearch::xGetIntraBitsQTChroma(TComDataCU* cu, UInt trDepth, UInt absPartIdx, UInt chromaId)
{
    m_entropyCoder->resetBits();
    if (chromaId == TEXT_CHROMA_U)
    {
        xEncCoeffQT(cu, trDepth, absPartIdx, TEXT_CHROMA_U);
    }
    else if (chromaId == TEXT_CHROMA_V)
    {
        xEncCoeffQT(cu, trDepth, absPartIdx, TEXT_CHROMA_V);
    }
    return m_entropyCoder->getNumberOfWrittenBits();
}

void TEncSearch::xIntraCodingLumaBlk(TComDataCU* cu,
                                     UInt        trDepth,
                                     UInt        absPartIdx,
                                     TComYuv*    fencYuv,
                                     TComYuv*    predYuv,
                                     TShortYUV*  resiYuv,
                                     UInt&       outDist,
                                     int         default0Save1Load2)
{
    UInt    lumaPredMode = cu->getLumaIntraDir(absPartIdx);
    UInt    fullDepth    = cu->getDepth(0)  + trDepth;
    UInt    width        = cu->getWidth(0) >> trDepth;
    UInt    height       = cu->getHeight(0) >> trDepth;
    UInt    stride       = fencYuv->getStride();
    Pel*    fenc         = fencYuv->getLumaAddr(absPartIdx);
    Pel*    pred         = predYuv->getLumaAddr(absPartIdx);
    short*  residual     = resiYuv->getLumaAddr(absPartIdx);
    Pel*    recon        = predYuv->getLumaAddr(absPartIdx);

    UInt    trSizeLog2     = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> fullDepth] + 2;
    UInt    qtLayer        = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;
    UInt    numCoeffPerInc = cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight() >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1);
    TCoeff* coeff          = m_qtTempCoeffY[qtLayer] + numCoeffPerInc * absPartIdx;

    short*  reconQt        = m_qtTempTComYuv[qtLayer].getLumaAddr(absPartIdx);
    UInt    reconQtStride  = m_qtTempTComYuv[qtLayer].m_width;

    UInt    zorder           = cu->getZorderIdxInCU() + absPartIdx;
    Pel*    reconIPred       = cu->getPic()->getPicYuvRec()->getLumaAddr(cu->getAddr(), zorder);
    UInt    reconIPredStride = cu->getPic()->getPicYuvRec()->getStride();
    bool    useTransformSkip = cu->getTransformSkip(absPartIdx, TEXT_LUMA);

    //===== init availability pattern =====

    if (default0Save1Load2 != 2)
    {
        cu->getPattern()->initPattern(cu, trDepth, absPartIdx);
        cu->getPattern()->initAdiPattern(cu, absPartIdx, trDepth, m_predBuf, m_predBufStride, m_predBufHeight, refAbove, refLeft, refAboveFlt, refLeftFlt);
        //===== get prediction signal =====
        predIntraLumaAng(lumaPredMode, pred, stride, width);
        // save prediction
        if (default0Save1Load2 == 1)
        {
            primitives.blockcpy_pp(width, height, m_sharedPredTransformSkip[0], width, pred, stride);
        }
    }
    else
    {
        primitives.blockcpy_pp(width, height, pred, stride, m_sharedPredTransformSkip[0], width);
    }

    //===== get residual signal =====

    primitives.calcresidual[(int)g_convertToBit[width]](fenc, pred, residual, stride);

    //===== transform and quantization =====
    //--- init rate estimation arrays for RDOQ ---
    if (useTransformSkip ? m_cfg->param.bEnableRDOQTS : m_cfg->param.bEnableRDOQ)
    {
        m_entropyCoder->estimateBit(m_trQuant->m_estBitsSbac, width, width, TEXT_LUMA);
    }

    //--- transform and quantization ---
    UInt absSum = 0;
    int lastPos = -1;
    cu->setTrIdxSubParts(trDepth, absPartIdx, fullDepth);

    m_trQuant->setQPforQuant(cu->getQP(0), TEXT_LUMA, cu->getSlice()->getSPS()->getQpBDOffsetY(), 0);
    m_trQuant->selectLambda(TEXT_LUMA);

    absSum = m_trQuant->transformNxN(cu, residual, stride, coeff, width, height, TEXT_LUMA, absPartIdx, &lastPos, useTransformSkip);

    //--- set coded block flag ---
    cu->setCbfSubParts((absSum ? 1 : 0) << trDepth, TEXT_LUMA, absPartIdx, fullDepth);

    //--- inverse transform ---
    int size = g_convertToBit[width];
    if (absSum)
    {
        int scalingListType = 0 + g_eTTable[(int)TEXT_LUMA];
        assert(scalingListType < 6);
        m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdx), cu->getLumaIntraDir(absPartIdx), residual, stride, coeff, width, height, scalingListType, useTransformSkip, lastPos);
    }
    else
    {
        short* resiTmp = residual;
        memset(coeff, 0, sizeof(TCoeff) * width * height);
        primitives.blockfil_s[size](resiTmp, stride, 0);
    }

    //===== reconstruction =====
    primitives.calcrecon[size](pred, residual, recon, reconQt, reconIPred, stride, reconQtStride, reconIPredStride);

    //===== update distortion =====
    int part = PartitionFromSizes(width, height);
    outDist += primitives.sse_pp[part](fenc, stride, recon, stride);
}

void TEncSearch::xIntraCodingChromaBlk(TComDataCU* cu,
                                       UInt        trDepth,
                                       UInt        absPartIdx,
                                       TComYuv*    fencYuv,
                                       TComYuv*    predYuv,
                                       TShortYUV*  resiYuv,
                                       UInt&       outDist,
                                       UInt        chromaId,
                                       int         default0Save1Load2)
{
    UInt origTrDepth = trDepth;
    UInt fullDepth   = cu->getDepth(0) + trDepth;
    UInt trSizeLog2  = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> fullDepth] + 2;

    if (trSizeLog2 == 2)
    {
        assert(trDepth > 0);
        trDepth--;
        UInt qpdiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trDepth) << 1);
        bool bFirstQ = ((absPartIdx % qpdiv) == 0);
        if (!bFirstQ)
        {
            return;
        }
    }

    TextType ttype          = (chromaId > 0 ? TEXT_CHROMA_V : TEXT_CHROMA_U);
    UInt     chromaPredMode = cu->getChromaIntraDir(absPartIdx);
    UInt     width          = cu->getWidth(0) >> (trDepth + 1);
    UInt     height         = cu->getHeight(0) >> (trDepth + 1);
    UInt     stride         = fencYuv->getCStride();
    Pel*     fenc           = (chromaId > 0 ? fencYuv->getCrAddr(absPartIdx) : fencYuv->getCbAddr(absPartIdx));
    Pel*     pred           = (chromaId > 0 ? predYuv->getCrAddr(absPartIdx) : predYuv->getCbAddr(absPartIdx));
    short*   residual       = (chromaId > 0 ? resiYuv->getCrAddr(absPartIdx) : resiYuv->getCbAddr(absPartIdx));
    Pel*     recon          = (chromaId > 0 ? predYuv->getCrAddr(absPartIdx) : predYuv->getCbAddr(absPartIdx));

    UInt    qtlayer        = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;
    UInt    numCoeffPerInc = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight() >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1)) >> 2;
    TCoeff* coeff          = (chromaId > 0 ? m_qtTempCoeffCr[qtlayer] : m_qtTempCoeffCb[qtlayer]) + numCoeffPerInc * absPartIdx;
    short*  reconQt        = (chromaId > 0 ? m_qtTempTComYuv[qtlayer].getCrAddr(absPartIdx) : m_qtTempTComYuv[qtlayer].getCbAddr(absPartIdx));
    UInt    reconQtStride  = m_qtTempTComYuv[qtlayer].m_cwidth;

    UInt    zorder           = cu->getZorderIdxInCU() + absPartIdx;
    Pel*    reconIPred       = (chromaId > 0 ? cu->getPic()->getPicYuvRec()->getCrAddr(cu->getAddr(), zorder) : cu->getPic()->getPicYuvRec()->getCbAddr(cu->getAddr(), zorder));
    UInt    reconIPredStride = cu->getPic()->getPicYuvRec()->getCStride();
    bool    useTransformSkipChroma = cu->getTransformSkip(absPartIdx, ttype);

    //===== update chroma mode =====
    if (chromaPredMode == DM_CHROMA_IDX)
    {
        chromaPredMode = cu->getLumaIntraDir(0);
    }

    //===== init availability pattern =====
    if (default0Save1Load2 != 2)
    {
        cu->getPattern()->initPattern(cu, trDepth, absPartIdx);

        cu->getPattern()->initAdiPatternChroma(cu, absPartIdx, trDepth, m_predBuf, m_predBufStride, m_predBufHeight);
        Pel* chromaPred = (chromaId > 0 ? cu->getPattern()->getAdiCrBuf(width, height, m_predBuf) : cu->getPattern()->getAdiCbBuf(width, height, m_predBuf));

        //===== get prediction signal =====
        predIntraChromaAng(chromaPred, chromaPredMode, pred, stride, width);

        // save prediction
        if (default0Save1Load2 == 1)
        {
            Pel* predbuf = m_sharedPredTransformSkip[1 + chromaId];
            primitives.blockcpy_pp(width, height, predbuf, width, pred, stride);
        }
    }
    else
    {
        // load prediction
        Pel* predbuf = m_sharedPredTransformSkip[1 + chromaId];
        primitives.blockcpy_pp(width, height, pred, stride, predbuf, width);
    }

    //===== get residual signal =====
    int size = g_convertToBit[width];
    primitives.calcresidual[size](fenc, pred, residual, stride);

    //===== transform and quantization =====
    {
        //--- init rate estimation arrays for RDOQ ---
        if (useTransformSkipChroma ? m_cfg->param.bEnableRDOQTS : m_cfg->param.bEnableRDOQ)
        {
            m_entropyCoder->estimateBit(m_trQuant->m_estBitsSbac, width, width, ttype);
        }
        //--- transform and quantization ---
        UInt absSum = 0;
        int lastPos = -1;

        int curChromaQpOffset;
        if (ttype == TEXT_CHROMA_U)
        {
            curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCbQpOffset() + cu->getSlice()->getSliceQpDeltaCb();
        }
        else
        {
            curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCrQpOffset() + cu->getSlice()->getSliceQpDeltaCr();
        }
        m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset);

        m_trQuant->selectLambda(TEXT_CHROMA);

        absSum = m_trQuant->transformNxN(cu, residual, stride, coeff, width, height, ttype, absPartIdx, &lastPos, useTransformSkipChroma);

        //--- set coded block flag ---
        cu->setCbfSubParts((absSum ? 1 : 0) << origTrDepth, ttype, absPartIdx, cu->getDepth(0) + trDepth);

        //--- inverse transform ---
        if (absSum)
        {
            int scalingListType = 0 + g_eTTable[(int)ttype];
            assert(scalingListType < 6);
            m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdx), REG_DCT, residual, stride, coeff, width, height, scalingListType, useTransformSkipChroma, lastPos);
        }
        else
        {
            short* resiTmp = residual;
            memset(coeff, 0, sizeof(TCoeff) * width * height);
            primitives.blockfil_s[size](resiTmp, stride, 0);
        }
    }

    //===== reconstruction =====
    primitives.calcrecon[size](pred, residual, recon, reconQt, reconIPred, stride, reconQtStride, reconIPredStride);

    //===== update distortion =====
    int part = PartitionFromSizes(width, height);
    UInt dist = primitives.sse_pp[part](fenc, stride, recon, stride);
    if (ttype == TEXT_CHROMA_U)
    {
        outDist += m_rdCost->scaleChromaDistCb(dist);
    }
    else if (ttype == TEXT_CHROMA_V)
    {
        outDist += m_rdCost->scaleChromaDistCr(dist);
    }
    else
    {
        outDist += dist;
    }
}

void TEncSearch::xRecurIntraCodingQT(TComDataCU* cu,
                                     UInt        trDepth,
                                     UInt        absPartIdx,
                                     bool        bLumaOnly,
                                     TComYuv*    fencYuv,
                                     TComYuv*    predYuv,
                                     TShortYUV*  resiYuv,
                                     UInt&       outDistY,
                                     UInt&       outDistC,
                                     bool        bCheckFirst,
                                     UInt64&     rdCost)
{
    UInt    fullDepth   = cu->getDepth(0) +  trDepth;
    UInt    trSizeLog2  = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> fullDepth] + 2;
    bool    bCheckFull  = (trSizeLog2 <= cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize());
    bool    bCheckSplit = (trSizeLog2 > cu->getQuadtreeTULog2MinSizeInCU(absPartIdx));

    int maxTuSize = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize();
    int isIntraSlice = (cu->getSlice()->getSliceType() == I_SLICE);

    // don't check split if TU size is less or equal to max TU size
    bool noSplitIntraMaxTuSize = bCheckFull;

    if (m_cfg->param.rdPenalty && !isIntraSlice)
    {
        // in addition don't check split if TU size is less or equal to 16x16 TU size for non-intra slice
        noSplitIntraMaxTuSize = (trSizeLog2 <= X265_MIN(maxTuSize, 4));

        // if maximum RD-penalty don't check TU size 32x32
        if (m_cfg->param.rdPenalty == 2)
        {
            bCheckFull = (trSizeLog2 <= X265_MIN(maxTuSize, 4));
        }
    }
    if (bCheckFirst && noSplitIntraMaxTuSize)
    {
        bCheckSplit = false;
    }

    UInt64 singleCost  = MAX_INT64;
    UInt   singleDistY = 0;
    UInt   singleDistC = 0;
    UInt   singleCbfY  = 0;
    UInt   singleCbfU  = 0;
    UInt   singleCbfV  = 0;
    bool   checkTransformSkip  = cu->getSlice()->getPPS()->getUseTransformSkip();
    UInt   widthTransformSkip  = cu->getWidth(0) >> trDepth;
    UInt   heightTransformSkip = cu->getHeight(0) >> trDepth;
    int    bestModeId    = 0;
    int    bestModeIdUV[2] = { 0, 0 };

    checkTransformSkip &= (widthTransformSkip == 4 && heightTransformSkip == 4);
    checkTransformSkip &= (!cu->getCUTransquantBypass(0));
    checkTransformSkip &= (!((cu->getQP(0) == 0) && (cu->getSlice()->getSPS()->getUseLossless())));
    if (m_cfg->param.bEnableTSkipFast)
    {
        checkTransformSkip &= (cu->getPartitionSize(absPartIdx) == SIZE_NxN);
    }

    if (bCheckFull)
    {
        if (checkTransformSkip == true)
        {
            //----- store original entropy coding status -----
            m_rdGoOnSbacCoder->store(m_rdSbacCoders[fullDepth][CI_QT_TRAFO_ROOT]);

            UInt   singleDistYTmp     = 0;
            UInt   singleDistCTmp     = 0;
            UInt   singleCbfYTmp      = 0;
            UInt   singleCbfUTmp      = 0;
            UInt   singleCbfVTmp      = 0;
            UInt64 singleCostTmp      = 0;
            int    default0Save1Load2 = 0;
            int    firstCheckId       = 0;

            UInt   qpdiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + (trDepth - 1)) << 1);
            bool   bFirstQ = ((absPartIdx % qpdiv) == 0);

            for (int modeId = firstCheckId; modeId < 2; modeId++)
            {
                singleDistYTmp = 0;
                singleDistCTmp = 0;
                cu->setTransformSkipSubParts(modeId, TEXT_LUMA, absPartIdx, fullDepth);
                if (modeId == firstCheckId)
                {
                    default0Save1Load2 = 1;
                }
                else
                {
                    default0Save1Load2 = 2;
                }
                //----- code luma block with given intra prediction mode and store Cbf-----
                xIntraCodingLumaBlk(cu, trDepth, absPartIdx, fencYuv, predYuv, resiYuv, singleDistYTmp, default0Save1Load2);
                singleCbfYTmp = cu->getCbf(absPartIdx, TEXT_LUMA, trDepth);
                //----- code chroma blocks with given intra prediction mode and store Cbf-----
                if (!bLumaOnly)
                {
                    if (bFirstQ)
                    {
                        cu->setTransformSkipSubParts(modeId, TEXT_CHROMA_U, absPartIdx, fullDepth);
                        cu->setTransformSkipSubParts(modeId, TEXT_CHROMA_V, absPartIdx, fullDepth);
                    }
                    xIntraCodingChromaBlk(cu, trDepth, absPartIdx, fencYuv, predYuv, resiYuv, singleDistCTmp, 0, default0Save1Load2);
                    xIntraCodingChromaBlk(cu, trDepth, absPartIdx, fencYuv, predYuv, resiYuv, singleDistCTmp, 1, default0Save1Load2);
                    singleCbfUTmp = cu->getCbf(absPartIdx, TEXT_CHROMA_U, trDepth);
                    singleCbfVTmp = cu->getCbf(absPartIdx, TEXT_CHROMA_V, trDepth);
                }
                //----- determine rate and r-d cost -----
                if (modeId == 1 && singleCbfYTmp == 0)
                {
                    //In order not to code TS flag when cbf is zero, the case for TS with cbf being zero is forbidden.
                    singleCostTmp = MAX_INT64;
                }
                else
                {
                    UInt singleBits = xGetIntraBitsQT(cu, trDepth, absPartIdx, true, !bLumaOnly);
                    singleCostTmp = m_rdCost->calcRdCost(singleDistYTmp + singleDistCTmp, singleBits);
                }

                if (singleCostTmp < singleCost)
                {
                    singleCost   = singleCostTmp;
                    singleDistY = singleDistYTmp;
                    singleDistC = singleDistCTmp;
                    singleCbfY  = singleCbfYTmp;
                    singleCbfU  = singleCbfUTmp;
                    singleCbfV  = singleCbfVTmp;
                    bestModeId    = modeId;
                    if (bestModeId == firstCheckId)
                    {
                        xStoreIntraResultQT(cu, trDepth, absPartIdx, bLumaOnly);
                        m_rdGoOnSbacCoder->store(m_rdSbacCoders[fullDepth][CI_TEMP_BEST]);
                    }
                }
                if (modeId == firstCheckId)
                {
                    m_rdGoOnSbacCoder->load(m_rdSbacCoders[fullDepth][CI_QT_TRAFO_ROOT]);
                }
            }

            cu->setTransformSkipSubParts(bestModeId, TEXT_LUMA, absPartIdx, fullDepth);

            if (bestModeId == firstCheckId)
            {
                xLoadIntraResultQT(cu, trDepth, absPartIdx, bLumaOnly);
                cu->setCbfSubParts(singleCbfY << trDepth, TEXT_LUMA, absPartIdx, fullDepth);
                if (!bLumaOnly)
                {
                    if (bFirstQ)
                    {
                        cu->setCbfSubParts(singleCbfU << trDepth, TEXT_CHROMA_U, absPartIdx, cu->getDepth(0) + trDepth - 1);
                        cu->setCbfSubParts(singleCbfV << trDepth, TEXT_CHROMA_V, absPartIdx, cu->getDepth(0) + trDepth - 1);
                    }
                }
                m_rdGoOnSbacCoder->load(m_rdSbacCoders[fullDepth][CI_TEMP_BEST]);
            }

            if (!bLumaOnly)
            {
                bestModeIdUV[0] = bestModeIdUV[1] = bestModeId;
                if (bFirstQ && bestModeId == 1)
                {
                    //In order not to code TS flag when cbf is zero, the case for TS with cbf being zero is forbidden.
                    if (singleCbfU == 0)
                    {
                        cu->setTransformSkipSubParts(0, TEXT_CHROMA_U, absPartIdx, fullDepth);
                        bestModeIdUV[0] = 0;
                    }
                    if (singleCbfV == 0)
                    {
                        cu->setTransformSkipSubParts(0, TEXT_CHROMA_V, absPartIdx, fullDepth);
                        bestModeIdUV[1] = 0;
                    }
                }
            }
        }
        else
        {
            cu->setTransformSkipSubParts(0, TEXT_LUMA, absPartIdx, fullDepth);

            //----- store original entropy coding status -----
            m_rdGoOnSbacCoder->store(m_rdSbacCoders[fullDepth][CI_QT_TRAFO_ROOT]);

            //----- code luma block with given intra prediction mode and store Cbf-----
            singleCost = 0;
            xIntraCodingLumaBlk(cu, trDepth, absPartIdx, fencYuv, predYuv, resiYuv, singleDistY);
            if (bCheckSplit)
            {
                singleCbfY = cu->getCbf(absPartIdx, TEXT_LUMA, trDepth);
            }
            //----- code chroma blocks with given intra prediction mode and store Cbf-----
            if (!bLumaOnly)
            {
                cu->setTransformSkipSubParts(0, TEXT_CHROMA_U, absPartIdx, fullDepth);
                cu->setTransformSkipSubParts(0, TEXT_CHROMA_V, absPartIdx, fullDepth);
                xIntraCodingChromaBlk(cu, trDepth, absPartIdx, fencYuv, predYuv, resiYuv, singleDistC, 0);
                xIntraCodingChromaBlk(cu, trDepth, absPartIdx, fencYuv, predYuv, resiYuv, singleDistC, 1);
                if (bCheckSplit)
                {
                    singleCbfU = cu->getCbf(absPartIdx, TEXT_CHROMA_U, trDepth);
                    singleCbfV = cu->getCbf(absPartIdx, TEXT_CHROMA_V, trDepth);
                }
            }
            //----- determine rate and r-d cost -----
            UInt singleBits = xGetIntraBitsQT(cu, trDepth, absPartIdx, true, !bLumaOnly);
            if (m_cfg->param.rdPenalty && (trSizeLog2 == 5) && !isIntraSlice)
            {
                singleBits = singleBits * 4;
            }
            singleCost = m_rdCost->calcRdCost(singleDistY + singleDistC, singleBits);
        }
    }

    if (bCheckSplit)
    {
        //----- store full entropy coding status, load original entropy coding status -----
        if (bCheckFull)
        {
            m_rdGoOnSbacCoder->store(m_rdSbacCoders[fullDepth][CI_QT_TRAFO_TEST]);
            m_rdGoOnSbacCoder->load(m_rdSbacCoders[fullDepth][CI_QT_TRAFO_ROOT]);
        }
        else
        {
            m_rdGoOnSbacCoder->store(m_rdSbacCoders[fullDepth][CI_QT_TRAFO_ROOT]);
        }

        //----- code splitted block -----
        UInt64 splitCost     = 0;
        UInt   splitDistY    = 0;
        UInt   splitDistC    = 0;
        UInt   qPartsDiv     = cu->getPic()->getNumPartInCU() >> ((fullDepth + 1) << 1);
        UInt   absPartIdxSub = absPartIdx;

        UInt   splitCbfY = 0;
        UInt   splitCbfU = 0;
        UInt   splitCbfV = 0;

        for (UInt part = 0; part < 4; part++, absPartIdxSub += qPartsDiv)
        {
            xRecurIntraCodingQT(cu, trDepth + 1, absPartIdxSub, bLumaOnly, fencYuv, predYuv, resiYuv, splitDistY, splitDistC, bCheckFirst, splitCost);

            splitCbfY |= cu->getCbf(absPartIdxSub, TEXT_LUMA, trDepth + 1);
            if (!bLumaOnly)
            {
                splitCbfU |= cu->getCbf(absPartIdxSub, TEXT_CHROMA_U, trDepth + 1);
                splitCbfV |= cu->getCbf(absPartIdxSub, TEXT_CHROMA_V, trDepth + 1);
            }
        }

        for (UInt offs = 0; offs < 4 * qPartsDiv; offs++)
        {
            cu->getCbf(TEXT_LUMA)[absPartIdx + offs] |= (splitCbfY << trDepth);
        }

        if (!bLumaOnly)
        {
            for (UInt offs = 0; offs < 4 * qPartsDiv; offs++)
            {
                cu->getCbf(TEXT_CHROMA_U)[absPartIdx + offs] |= (splitCbfU << trDepth);
                cu->getCbf(TEXT_CHROMA_V)[absPartIdx + offs] |= (splitCbfV << trDepth);
            }
        }
        //----- restore context states -----
        m_rdGoOnSbacCoder->load(m_rdSbacCoders[fullDepth][CI_QT_TRAFO_ROOT]);

        //----- determine rate and r-d cost -----
        UInt splitBits = xGetIntraBitsQT(cu, trDepth, absPartIdx, true, !bLumaOnly);
        splitCost = m_rdCost->calcRdCost(splitDistY + splitDistC, splitBits);

        //===== compare and set best =====
        if (splitCost < singleCost)
        {
            //--- update cost ---
            outDistY += splitDistY;
            outDistC += splitDistC;
            rdCost   += splitCost;
            return;
        }

        //----- set entropy coding status -----
        m_rdGoOnSbacCoder->load(m_rdSbacCoders[fullDepth][CI_QT_TRAFO_TEST]);

        //--- set transform index and Cbf values ---
        cu->setTrIdxSubParts(trDepth, absPartIdx, fullDepth);
        cu->setCbfSubParts(singleCbfY << trDepth, TEXT_LUMA, absPartIdx, fullDepth);
        cu->setTransformSkipSubParts(bestModeId, TEXT_LUMA, absPartIdx, fullDepth);
        if (!bLumaOnly)
        {
            cu->setCbfSubParts(singleCbfU << trDepth, TEXT_CHROMA_U, absPartIdx, fullDepth);
            cu->setCbfSubParts(singleCbfV << trDepth, TEXT_CHROMA_V, absPartIdx, fullDepth);
            cu->setTransformSkipSubParts(bestModeIdUV[0], TEXT_CHROMA_U, absPartIdx, fullDepth);
            cu->setTransformSkipSubParts(bestModeIdUV[1], TEXT_CHROMA_V, absPartIdx, fullDepth);
        }

        //--- set reconstruction for next intra prediction blocks ---
        UInt  width     = cu->getWidth(0) >> trDepth;
        UInt  height    = cu->getHeight(0) >> trDepth;
        UInt  qtLayer   = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;
        UInt  zorder    = cu->getZorderIdxInCU() + absPartIdx;
        short* src      = m_qtTempTComYuv[qtLayer].getLumaAddr(absPartIdx);
        UInt  srcstride = m_qtTempTComYuv[qtLayer].m_width;
        Pel*  dst       = cu->getPic()->getPicYuvRec()->getLumaAddr(cu->getAddr(), zorder);
        UInt  dststride = cu->getPic()->getPicYuvRec()->getStride();
        primitives.blockcpy_ps(width, height, dst, dststride, src, srcstride);

        if (!bLumaOnly)
        {
            width >>= 1;
            height >>= 1;
            src       = m_qtTempTComYuv[qtLayer].getCbAddr(absPartIdx);
            srcstride = m_qtTempTComYuv[qtLayer].m_cwidth;
            dst       = cu->getPic()->getPicYuvRec()->getCbAddr(cu->getAddr(), zorder);
            dststride = cu->getPic()->getPicYuvRec()->getCStride();
            primitives.blockcpy_ps(width, height, dst, dststride, src, srcstride);

            src = m_qtTempTComYuv[qtLayer].getCrAddr(absPartIdx);
            dst = cu->getPic()->getPicYuvRec()->getCrAddr(cu->getAddr(), zorder);
            primitives.blockcpy_ps(width, height, dst, dststride, src, srcstride);
        }
    }

    outDistY += singleDistY;
    outDistC += singleDistC;
    rdCost   += singleCost;
}

void TEncSearch::xSetIntraResultQT(TComDataCU* cu, UInt trDepth, UInt absPartIdx, bool bLumaOnly, TComYuv* reconYuv)
{
    UInt fullDepth = cu->getDepth(0) + trDepth;
    UInt trMode    = cu->getTransformIdx(absPartIdx);

    if (trMode == trDepth)
    {
        UInt trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> fullDepth] + 2;
        UInt qtlayer    = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;

        bool bSkipChroma = false;
        bool bChromaSame = false;
        if (!bLumaOnly && trSizeLog2 == 2)
        {
            assert(trDepth > 0);
            UInt qpdiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trDepth - 1) << 1);
            bSkipChroma = ((absPartIdx % qpdiv) != 0);
            bChromaSame = true;
        }

        //===== copy transform coefficients =====
        UInt numCoeffY    = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> (fullDepth << 1);
        UInt numCoeffIncY = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1);
        TCoeff* coeffSrcY = m_qtTempCoeffY[qtlayer] + (numCoeffIncY * absPartIdx);
        TCoeff* coeffDestY = cu->getCoeffY()        + (numCoeffIncY * absPartIdx);
        ::memcpy(coeffDestY, coeffSrcY, sizeof(TCoeff) * numCoeffY);

        if (!bLumaOnly && !bSkipChroma)
        {
            UInt numCoeffC    = (bChromaSame ? numCoeffY    : numCoeffY    >> 2);
            UInt numCoeffIncC = numCoeffIncY >> 2;
            TCoeff* coeffSrcU = m_qtTempCoeffCb[qtlayer] + (numCoeffIncC * absPartIdx);
            TCoeff* coeffSrcV = m_qtTempCoeffCr[qtlayer] + (numCoeffIncC * absPartIdx);
            TCoeff* coeffDstU = cu->getCoeffCb()            + (numCoeffIncC * absPartIdx);
            TCoeff* coeffDstV = cu->getCoeffCr()            + (numCoeffIncC * absPartIdx);
            ::memcpy(coeffDstU, coeffSrcU, sizeof(TCoeff) * numCoeffC);
            ::memcpy(coeffDstV, coeffSrcV, sizeof(TCoeff) * numCoeffC);
        }

        //===== copy reconstruction =====
        m_qtTempTComYuv[qtlayer].copyPartToPartLuma(reconYuv, absPartIdx, 1 << trSizeLog2, 1 << trSizeLog2);
        if (!bLumaOnly && !bSkipChroma)
        {
            UInt trSizeCLog2 = (bChromaSame ? trSizeLog2 : trSizeLog2 - 1);
            m_qtTempTComYuv[qtlayer].copyPartToPartChroma(reconYuv, absPartIdx, 1 << trSizeCLog2, 1 << trSizeCLog2);
        }
    }
    else
    {
        UInt numQPart = cu->getPic()->getNumPartInCU() >> ((fullDepth + 1) << 1);
        for (UInt part = 0; part < 4; part++)
        {
            xSetIntraResultQT(cu, trDepth + 1, absPartIdx + part * numQPart, bLumaOnly, reconYuv);
        }
    }
}

void TEncSearch::xStoreIntraResultQT(TComDataCU* cu, UInt trDepth, UInt absPartIdx, bool bLumaOnly)
{
    UInt fullMode = cu->getDepth(0) + trDepth;

    UInt trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> fullMode] + 2;
    UInt qtlayer    = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;

    bool bSkipChroma  = false;
    bool bChromaSame  = false;

    if (!bLumaOnly && trSizeLog2 == 2)
    {
        assert(trDepth > 0);
        UInt qpdiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trDepth - 1) << 1);
        bSkipChroma  = ((absPartIdx % qpdiv) != 0);
        bChromaSame  = true;
    }

    //===== copy transform coefficients =====
    UInt numCoeffY    = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> (fullMode << 1);
    UInt numCoeffIncY = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1);
    TCoeff* coeffSrcY = m_qtTempCoeffY[qtlayer] + (numCoeffIncY * absPartIdx);
    TCoeff* coeffDstY = m_qtTempTUCoeffY;
    ::memcpy(coeffDstY, coeffSrcY, sizeof(TCoeff) * numCoeffY);

    if (!bLumaOnly && !bSkipChroma)
    {
        UInt numCoeffC    = (bChromaSame ? numCoeffY : numCoeffY >> 2);
        UInt numCoeffIncC = numCoeffIncY >> 2;
        TCoeff* coeffSrcU = m_qtTempCoeffCb[qtlayer] + (numCoeffIncC * absPartIdx);
        TCoeff* coeffSrcV = m_qtTempCoeffCr[qtlayer] + (numCoeffIncC * absPartIdx);
        TCoeff* coeffDstU = m_qtTempTUCoeffCb;
        TCoeff* coeffDstV = m_qtTempTUCoeffCr;
        ::memcpy(coeffDstU, coeffSrcU, sizeof(TCoeff) * numCoeffC);
        ::memcpy(coeffDstV, coeffSrcV, sizeof(TCoeff) * numCoeffC);
    }

    //===== copy reconstruction =====
    m_qtTempTComYuv[qtlayer].copyPartToPartLuma(&m_qtTempTransformSkipTComYuv, absPartIdx, 1 << trSizeLog2, 1 << trSizeLog2);

    if (!bLumaOnly && !bSkipChroma)
    {
        UInt trSizeCLog2 = (bChromaSame ? trSizeLog2 : trSizeLog2 - 1);
        m_qtTempTComYuv[qtlayer].copyPartToPartChroma(&m_qtTempTransformSkipTComYuv, absPartIdx, 1 << trSizeCLog2, 1 << trSizeCLog2);
    }
}

void TEncSearch::xLoadIntraResultQT(TComDataCU* cu, UInt trDepth, UInt absPartIdx, bool bLumaOnly)
{
    UInt fullDepth = cu->getDepth(0) + trDepth;

    UInt trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> fullDepth] + 2;
    UInt qtlayer    = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;

    bool bSkipChroma = false;
    bool bChromaSame = false;

    if (!bLumaOnly && trSizeLog2 == 2)
    {
        assert(trDepth > 0);
        UInt qpdiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trDepth - 1) << 1);
        bSkipChroma = ((absPartIdx % qpdiv) != 0);
        bChromaSame = true;
    }

    //===== copy transform coefficients =====
    UInt numCoeffY    = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> (fullDepth << 1);
    UInt numCoeffIncY = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1);
    TCoeff* coeffDstY = m_qtTempCoeffY[qtlayer] + (numCoeffIncY * absPartIdx);
    TCoeff* coeffSrcY = m_qtTempTUCoeffY;
    ::memcpy(coeffDstY, coeffSrcY, sizeof(TCoeff) * numCoeffY);

    if (!bLumaOnly && !bSkipChroma)
    {
        UInt numCoeffC    = (bChromaSame ? numCoeffY : numCoeffY >> 2);
        UInt numCoeffIncC = numCoeffIncY >> 2;
        TCoeff* coeffDstU = m_qtTempCoeffCb[qtlayer] + (numCoeffIncC * absPartIdx);
        TCoeff* coeffDstV = m_qtTempCoeffCr[qtlayer] + (numCoeffIncC * absPartIdx);
        TCoeff* coeffSrcU = m_qtTempTUCoeffCb;
        TCoeff* coeffSrcV = m_qtTempTUCoeffCr;
        ::memcpy(coeffDstU, coeffSrcU, sizeof(TCoeff) * numCoeffC);
        ::memcpy(coeffDstV, coeffSrcV, sizeof(TCoeff) * numCoeffC);
    }

    //===== copy reconstruction =====
    m_qtTempTransformSkipTComYuv.copyPartToPartLuma(&m_qtTempTComYuv[qtlayer], absPartIdx, 1 << trSizeLog2, 1 << trSizeLog2);

    if (!bLumaOnly && !bSkipChroma)
    {
        UInt trSizeCLog2 = (bChromaSame ? trSizeLog2 : trSizeLog2 - 1);
        m_qtTempTransformSkipTComYuv.copyPartToPartChroma(&m_qtTempTComYuv[qtlayer], absPartIdx, 1 << trSizeCLog2, 1 << trSizeCLog2);
    }

    UInt   zOrder           = cu->getZorderIdxInCU() + absPartIdx;
    Pel*   reconIPred       = cu->getPic()->getPicYuvRec()->getLumaAddr(cu->getAddr(), zOrder);
    UInt   reconIPredStride = cu->getPic()->getPicYuvRec()->getStride();
    short* reconQt          = m_qtTempTComYuv[qtlayer].getLumaAddr(absPartIdx);
    UInt   reconQtStride    = m_qtTempTComYuv[qtlayer].m_width;
    UInt   width            = cu->getWidth(0) >> trDepth;
    UInt   height           = cu->getHeight(0) >> trDepth;
    primitives.blockcpy_ps(width, height, reconIPred, reconIPredStride, reconQt, reconQtStride);

    if (!bLumaOnly && !bSkipChroma)
    {
        width >>= 1;
        height >>= 1;
        reconIPred = cu->getPic()->getPicYuvRec()->getCbAddr(cu->getAddr(), zOrder);
        reconIPredStride = cu->getPic()->getPicYuvRec()->getCStride();
        reconQt = m_qtTempTComYuv[qtlayer].getCbAddr(absPartIdx);
        reconQtStride = m_qtTempTComYuv[qtlayer].m_cwidth;
        primitives.blockcpy_ps(width, height, reconIPred, reconIPredStride, reconQt, reconQtStride);

        reconIPred = cu->getPic()->getPicYuvRec()->getCrAddr(cu->getAddr(), zOrder);
        reconQt    = m_qtTempTComYuv[qtlayer].getCrAddr(absPartIdx);
        primitives.blockcpy_ps(width, height, reconIPred, reconIPredStride, reconQt, reconQtStride);
    }
}

void TEncSearch::xStoreIntraResultChromaQT(TComDataCU* cu, UInt trDepth, UInt absPartIdx, UInt stateU0V1Both2)
{
    UInt fullDepth = cu->getDepth(0) + trDepth;
    UInt trMode    = cu->getTransformIdx(absPartIdx);

    if (trMode == trDepth)
    {
        UInt trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> fullDepth] + 2;
        UInt qtlayer    = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;

        bool bChromaSame = false;
        if (trSizeLog2 == 2)
        {
            assert(trDepth > 0);
            trDepth--;
            UInt qpdiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trDepth) << 1);
            if ((absPartIdx % qpdiv) != 0)
            {
                return;
            }
            bChromaSame = true;
        }

        //===== copy transform coefficients =====
        UInt numCoeffC = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> (fullDepth << 1);
        if (!bChromaSame)
        {
            numCoeffC >>= 2;
        }
        UInt numCoeffIncC = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> ((cu->getSlice()->getSPS()->getMaxCUDepth() << 1) + 2);
        if (stateU0V1Both2 == 0 || stateU0V1Both2 == 2)
        {
            TCoeff* coeffSrcU = m_qtTempCoeffCb[qtlayer] + (numCoeffIncC * absPartIdx);
            TCoeff* coeffDstU = m_qtTempTUCoeffCb;
            ::memcpy(coeffDstU, coeffSrcU, sizeof(TCoeff) * numCoeffC);
        }
        if (stateU0V1Both2 == 1 || stateU0V1Both2 == 2)
        {
            TCoeff* coeffSrcV = m_qtTempCoeffCr[qtlayer] + (numCoeffIncC * absPartIdx);
            TCoeff* coeffDstV = m_qtTempTUCoeffCr;
            ::memcpy(coeffDstV, coeffSrcV, sizeof(TCoeff) * numCoeffC);
        }

        //===== copy reconstruction =====
        UInt trSizeCLog2 = (bChromaSame ? trSizeLog2 : trSizeLog2 - 1);
        m_qtTempTComYuv[qtlayer].copyPartToPartChroma(&m_qtTempTransformSkipTComYuv, absPartIdx, 1 << trSizeCLog2, 1 << trSizeCLog2, stateU0V1Both2);
    }
}

void TEncSearch::xLoadIntraResultChromaQT(TComDataCU* cu, UInt trDepth, UInt absPartIdx, UInt stateU0V1Both2)
{
    UInt fullDepth = cu->getDepth(0) + trDepth;
    UInt trMode    = cu->getTransformIdx(absPartIdx);

    if (trMode == trDepth)
    {
        UInt trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> fullDepth] + 2;
        UInt qtlayer    = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;

        bool bChromaSame = false;
        if (trSizeLog2 == 2)
        {
            assert(trDepth > 0);
            trDepth--;
            UInt qpdiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trDepth) << 1);
            if ((absPartIdx % qpdiv) != 0)
            {
                return;
            }
            bChromaSame = true;
        }

        //===== copy transform coefficients =====
        UInt numCoeffC = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> (fullDepth << 1);
        if (!bChromaSame)
        {
            numCoeffC >>= 2;
        }
        UInt numCoeffIncC = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> ((cu->getSlice()->getSPS()->getMaxCUDepth() << 1) + 2);

        if (stateU0V1Both2 == 0 || stateU0V1Both2 == 2)
        {
            TCoeff* coeffDstU = m_qtTempCoeffCb[qtlayer] + (numCoeffIncC * absPartIdx);
            TCoeff* coeffSrcU = m_qtTempTUCoeffCb;
            ::memcpy(coeffDstU, coeffSrcU, sizeof(TCoeff) * numCoeffC);
        }
        if (stateU0V1Both2 == 1 || stateU0V1Both2 == 2)
        {
            TCoeff* coeffDstV = m_qtTempCoeffCr[qtlayer] + (numCoeffIncC * absPartIdx);
            TCoeff* coeffSrcV = m_qtTempTUCoeffCr;
            ::memcpy(coeffDstV, coeffSrcV, sizeof(TCoeff) * numCoeffC);
        }

        //===== copy reconstruction =====
        UInt trSizeCLog2 = (bChromaSame ? trSizeLog2 : trSizeLog2 - 1);
        m_qtTempTransformSkipTComYuv.copyPartToPartChroma(&m_qtTempTComYuv[qtlayer], absPartIdx, 1 << trSizeCLog2, 1 << trSizeCLog2, stateU0V1Both2);

        UInt zorder           = cu->getZorderIdxInCU() + absPartIdx;
        UInt width            = cu->getWidth(0) >> (trDepth + 1);
        UInt height           = cu->getHeight(0) >> (trDepth + 1);
        UInt reconQtStride    = m_qtTempTComYuv[qtlayer].m_cwidth;
        UInt reconIPredStride = cu->getPic()->getPicYuvRec()->getCStride();

        if (stateU0V1Both2 == 0 || stateU0V1Both2 == 2)
        {
            Pel* reconIPred = cu->getPic()->getPicYuvRec()->getCbAddr(cu->getAddr(), zorder);
            short* reconQt  = m_qtTempTComYuv[qtlayer].getCbAddr(absPartIdx);
            primitives.blockcpy_ps(width, height, reconIPred, reconIPredStride, reconQt, reconQtStride);
        }
        if (stateU0V1Both2 == 1 || stateU0V1Both2 == 2)
        {
            Pel* reconIPred = cu->getPic()->getPicYuvRec()->getCrAddr(cu->getAddr(), zorder);
            short* reconQt  = m_qtTempTComYuv[qtlayer].getCrAddr(absPartIdx);
            primitives.blockcpy_ps(width, height, reconIPred, reconIPredStride, reconQt, reconQtStride);
        }
    }
}

void TEncSearch::xRecurIntraChromaCodingQT(TComDataCU* cu,
                                           UInt        trDepth,
                                           UInt        absPartIdx,
                                           TComYuv*    fencYuv,
                                           TComYuv*    predYuv,
                                           TShortYUV*  resiYuv,
                                           UInt&       outDist)
{
    UInt fullDepth = cu->getDepth(0) + trDepth;
    UInt trMode    = cu->getTransformIdx(absPartIdx);

    if (trMode == trDepth)
    {
        bool checkTransformSkip = cu->getSlice()->getPPS()->getUseTransformSkip();
        UInt trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> fullDepth] + 2;

        UInt actualTrDepth = trDepth;
        if (trSizeLog2 == 2)
        {
            assert(trDepth > 0);
            actualTrDepth--;
            UInt qpdiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + actualTrDepth) << 1);
            bool bFirstQ = ((absPartIdx % qpdiv) == 0);
            if (!bFirstQ)
            {
                return;
            }
        }

        checkTransformSkip &= (trSizeLog2 <= 3);
        if (m_cfg->param.bEnableTSkipFast)
        {
            checkTransformSkip &= (trSizeLog2 < 3);
            if (checkTransformSkip)
            {
                int nbLumaSkip = 0;
                for (UInt absPartIdxSub = absPartIdx; absPartIdxSub < absPartIdx + 4; absPartIdxSub++)
                {
                    nbLumaSkip += cu->getTransformSkip(absPartIdxSub, TEXT_LUMA);
                }

                checkTransformSkip &= (nbLumaSkip > 0);
            }
        }

        if (checkTransformSkip)
        {
            // use RDO to decide whether Cr/Cb takes TS
            m_rdGoOnSbacCoder->store(m_rdSbacCoders[fullDepth][CI_QT_TRAFO_ROOT]);

            for (int chromaId = 0; chromaId < 2; chromaId++)
            {
                UInt64  singleCost     = MAX_INT64;
                int     bestModeId     = 0;
                UInt    singleDistC    = 0;
                UInt    singleCbfC     = 0;
                UInt    singleDistCTmp = 0;
                UInt64  singleCostTmp  = 0;
                UInt    singleCbfCTmp  = 0;

                int     default0Save1Load2 = 0;
                int     firstCheckId       = 0;

                for (int chromaModeId = firstCheckId; chromaModeId < 2; chromaModeId++)
                {
                    cu->setTransformSkipSubParts(chromaModeId, (TextType)(chromaId + 2), absPartIdx, cu->getDepth(0) + actualTrDepth);
                    if (chromaModeId == firstCheckId)
                    {
                        default0Save1Load2 = 1;
                    }
                    else
                    {
                        default0Save1Load2 = 2;
                    }
                    singleDistCTmp = 0;
                    xIntraCodingChromaBlk(cu, trDepth, absPartIdx, fencYuv, predYuv, resiYuv, singleDistCTmp, chromaId, default0Save1Load2);
                    singleCbfCTmp = cu->getCbf(absPartIdx, (TextType)(chromaId + 2), trDepth);

                    if (chromaModeId == 1 && singleCbfCTmp == 0)
                    {
                        //In order not to code TS flag when cbf is zero, the case for TS with cbf being zero is forbidden.
                        singleCostTmp = MAX_INT64;
                    }
                    else
                    {
                        UInt bitsTmp = xGetIntraBitsQTChroma(cu, trDepth, absPartIdx, chromaId + 2);
                        singleCostTmp = m_rdCost->calcRdCost(singleDistCTmp, bitsTmp);
                    }

                    if (singleCostTmp < singleCost)
                    {
                        singleCost  = singleCostTmp;
                        singleDistC = singleDistCTmp;
                        bestModeId  = chromaModeId;
                        singleCbfC  = singleCbfCTmp;

                        if (bestModeId == firstCheckId)
                        {
                            xStoreIntraResultChromaQT(cu, trDepth, absPartIdx, chromaId);
                            m_rdGoOnSbacCoder->store(m_rdSbacCoders[fullDepth][CI_TEMP_BEST]);
                        }
                    }
                    if (chromaModeId == firstCheckId)
                    {
                        m_rdGoOnSbacCoder->load(m_rdSbacCoders[fullDepth][CI_QT_TRAFO_ROOT]);
                    }
                }

                if (bestModeId == firstCheckId)
                {
                    xLoadIntraResultChromaQT(cu, trDepth, absPartIdx, chromaId);
                    cu->setCbfSubParts(singleCbfC << trDepth, (TextType)(chromaId + 2), absPartIdx, cu->getDepth(0) + actualTrDepth);
                    m_rdGoOnSbacCoder->load(m_rdSbacCoders[fullDepth][CI_TEMP_BEST]);
                }
                cu->setTransformSkipSubParts(bestModeId, (TextType)(chromaId + 2), absPartIdx, cu->getDepth(0) +  actualTrDepth);
                outDist += singleDistC;

                if (chromaId == 0)
                {
                    m_rdGoOnSbacCoder->store(m_rdSbacCoders[fullDepth][CI_QT_TRAFO_ROOT]);
                }
            }
        }
        else
        {
            cu->setTransformSkipSubParts(0, TEXT_CHROMA_U, absPartIdx, cu->getDepth(0) +  actualTrDepth);
            cu->setTransformSkipSubParts(0, TEXT_CHROMA_V, absPartIdx, cu->getDepth(0) +  actualTrDepth);
            xIntraCodingChromaBlk(cu, trDepth, absPartIdx, fencYuv, predYuv, resiYuv, outDist, 0);
            xIntraCodingChromaBlk(cu, trDepth, absPartIdx, fencYuv, predYuv, resiYuv, outDist, 1);
        }
    }
    else
    {
        UInt splitCbfU     = 0;
        UInt splitCbfV     = 0;
        UInt qPartsDiv     = cu->getPic()->getNumPartInCU() >> ((fullDepth + 1) << 1);
        UInt absPartIdxSub = absPartIdx;
        for (UInt part = 0; part < 4; part++, absPartIdxSub += qPartsDiv)
        {
            xRecurIntraChromaCodingQT(cu, trDepth + 1, absPartIdxSub, fencYuv, predYuv, resiYuv, outDist);
            splitCbfU |= cu->getCbf(absPartIdxSub, TEXT_CHROMA_U, trDepth + 1);
            splitCbfV |= cu->getCbf(absPartIdxSub, TEXT_CHROMA_V, trDepth + 1);
        }

        for (UInt offs = 0; offs < 4 * qPartsDiv; offs++)
        {
            cu->getCbf(TEXT_CHROMA_U)[absPartIdx + offs] |= (splitCbfU << trDepth);
            cu->getCbf(TEXT_CHROMA_V)[absPartIdx + offs] |= (splitCbfV << trDepth);
        }
    }
}

void TEncSearch::xSetIntraResultChromaQT(TComDataCU* cu, UInt trDepth, UInt absPartIdx, TComYuv* reconYuv)
{
    UInt fullDepth = cu->getDepth(0) + trDepth;
    UInt trMode    = cu->getTransformIdx(absPartIdx);

    if (trMode == trDepth)
    {
        UInt trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> fullDepth] + 2;
        UInt qtlayer    = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;

        bool bChromaSame  = false;
        if (trSizeLog2 == 2)
        {
            assert(trDepth > 0);
            UInt qpdiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trDepth - 1) << 1);
            if ((absPartIdx % qpdiv) != 0)
            {
                return;
            }
            bChromaSame = true;
        }

        //===== copy transform coefficients =====
        UInt numCoeffC = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> (fullDepth << 1);
        if (!bChromaSame)
        {
            numCoeffC >>= 2;
        }
        UInt numCoeffIncC = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> ((cu->getSlice()->getSPS()->getMaxCUDepth() << 1) + 2);
        TCoeff* coeffSrcU = m_qtTempCoeffCb[qtlayer] + (numCoeffIncC * absPartIdx);
        TCoeff* coeffSrcV = m_qtTempCoeffCr[qtlayer] + (numCoeffIncC * absPartIdx);
        TCoeff* coeffDstU = cu->getCoeffCb()         + (numCoeffIncC * absPartIdx);
        TCoeff* coeffDstV = cu->getCoeffCr()         + (numCoeffIncC * absPartIdx);
        ::memcpy(coeffDstU, coeffSrcU, sizeof(TCoeff) * numCoeffC);
        ::memcpy(coeffDstV, coeffSrcV, sizeof(TCoeff) * numCoeffC);

        //===== copy reconstruction =====
        UInt trSizeCLog2 = (bChromaSame ? trSizeLog2 : trSizeLog2 - 1);
        m_qtTempTComYuv[qtlayer].copyPartToPartChroma(reconYuv, absPartIdx, 1 << trSizeCLog2, 1 << trSizeCLog2);
    }
    else
    {
        UInt numQPart = cu->getPic()->getNumPartInCU() >> ((fullDepth + 1) << 1);
        for (UInt part = 0; part < 4; part++)
        {
            xSetIntraResultChromaQT(cu, trDepth + 1, absPartIdx + part * numQPart, reconYuv);
        }
    }
}

void TEncSearch::preestChromaPredMode(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv)
{
    UInt width  = cu->getWidth(0) >> 1;
    UInt height = cu->getHeight(0) >> 1;
    UInt stride = fencYuv->getCStride();
    Pel* fencU  = fencYuv->getCbAddr(0);
    Pel* fencV  = fencYuv->getCrAddr(0);
    Pel* predU  = predYuv->getCbAddr(0);
    Pel* predV  = predYuv->getCrAddr(0);

    //===== init pattern =====
    assert(width == height);
    cu->getPattern()->initPattern(cu, 0, 0);
    cu->getPattern()->initAdiPatternChroma(cu, 0, 0, m_predBuf, m_predBufStride, m_predBufHeight);
    Pel* patChromaU = cu->getPattern()->getAdiCbBuf(width, height, m_predBuf);
    Pel* patChromaV = cu->getPattern()->getAdiCrBuf(width, height, m_predBuf);

    //===== get best prediction modes (using SAD) =====
    UInt minMode  = 0;
    UInt maxMode  = 4;
    UInt bestMode = MAX_UINT;
    UInt minSAD   = MAX_UINT;
    pixelcmp_t sa8d = primitives.sa8d[(int)g_convertToBit[width]];
    for (UInt mode = minMode; mode < maxMode; mode++)
    {
        //--- get prediction ---
        predIntraChromaAng(patChromaU, mode, predU, stride, width);
        predIntraChromaAng(patChromaV, mode, predV, stride, width);

        //--- get SAD ---
        UInt sad = sa8d(fencU, stride, predU, stride) + sa8d(fencV, stride, predV, stride);

        //--- check ---
        if (sad < minSAD)
        {
            minSAD   = sad;
            bestMode = mode;
        }
    }

    x265_emms();

    //===== set chroma pred mode =====
    cu->setChromIntraDirSubParts(bestMode, 0, cu->getDepth(0));
}

void TEncSearch::estIntraPredQT(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, TShortYUV* resiYuv, TComYuv* reconYuv, UInt& outDistC, bool bLumaOnly)
{
    UInt depth        = cu->getDepth(0);
    UInt numPU        = cu->getNumPartInter();
    UInt initTrDepth  = cu->getPartitionSize(0) == SIZE_2Nx2N ? 0 : 1;
    UInt width        = cu->getWidth(0) >> initTrDepth;
    UInt qNumParts    = cu->getTotalNumPart() >> 2;
    UInt widthBit     = cu->getIntraSizeIdx(0);
    UInt overallDistY = 0;
    UInt overallDistC = 0;
    UInt candNum;
    UInt64 candCostList[FAST_UDI_MAX_RDMODE_NUM];

    //===== set QP and clear Cbf =====
    if (cu->getSlice()->getPPS()->getUseDQP() == true)
    {
        cu->setQPSubParts(cu->getQP(0), 0, depth);
    }
    else
    {
        cu->setQPSubParts(cu->getSlice()->getSliceQp(), 0, depth);
    }

    //===== loop over partitions =====
    UInt partOffset = 0;
    for (UInt pu = 0; pu < numPU; pu++, partOffset += qNumParts)
    {
        //===== init pattern for luma prediction =====
        cu->getPattern()->initPattern(cu, initTrDepth, partOffset);

        // Reference sample smoothing
        cu->getPattern()->initAdiPattern(cu, partOffset, initTrDepth, m_predBuf, m_predBufStride, m_predBufHeight, refAbove, refLeft, refAboveFlt, refLeftFlt);

        //===== determine set of modes to be tested (using prediction signal only) =====
        int numModesAvailable = 35; //total number of Intra modes
        Pel* fenc   = fencYuv->getLumaAddr(pu, width);
        Pel* pred   = predYuv->getLumaAddr(pu, width);
        UInt stride = predYuv->getStride();
        UInt rdModeList[FAST_UDI_MAX_RDMODE_NUM];
        int numModesForFullRD = g_intraModeNumFast[widthBit];
        int log2SizeMinus2 = g_convertToBit[width];
        pixelcmp_t sa8d = primitives.sa8d[log2SizeMinus2];

        bool doFastSearch = (numModesForFullRD != numModesAvailable);
        if (doFastSearch)
        {
            assert(numModesForFullRD < numModesAvailable);

            for (int i = 0; i < numModesForFullRD; i++)
            {
                candCostList[i] = MAX_INT64;
            }

            candNum = 0;
            UInt modeCosts[35];

            Pel *pAbove0 = refAbove    + width - 1;
            Pel *pAbove1 = refAboveFlt + width - 1;
            Pel *pLeft0  = refLeft     + width - 1;
            Pel *pLeft1  = refLeftFlt  + width - 1;

            // 33 Angle modes once
            ALIGN_VAR_32(Pel, buf_trans[32 * 32]);
            ALIGN_VAR_32(Pel, tmp[33 * 32 * 32]);

            if (width <= 32)
            {
                // 1
                primitives.intra_pred_dc(pAbove0 + 1, pLeft0 + 1, pred, stride, width, (width <= 16));
                modeCosts[DC_IDX] = sa8d(fenc, stride, pred, stride);

                // 0
                Pel *above   = pAbove0;
                Pel *left    = pLeft0;
                if (width >= 8 && width <= 32)
                {
                    above = pAbove1;
                    left  = pLeft1;
                }
                primitives.intra_pred_planar((pixel*)above + 1, (pixel*)left + 1, pred, stride, width);
                modeCosts[PLANAR_IDX] = sa8d(fenc, stride, pred, stride);

                // Transpose NxN
                primitives.transpose[log2SizeMinus2](buf_trans, (pixel*)fenc, stride);

                primitives.intra_pred_allangs[log2SizeMinus2](tmp, pAbove0, pLeft0, pAbove1, pLeft1, (width <= 16));

                // TODO: We need SATD_x4 here
                for (UInt mode = 2; mode < numModesAvailable; mode++)
                {
                    bool modeHor = (mode < 18);
                    Pel *cmp = (modeHor ? buf_trans : fenc);
                    intptr_t srcStride = (modeHor ? width : stride);
                    modeCosts[mode] = sa8d(cmp, srcStride, &tmp[(mode - 2) * (width * width)], width);
                }
            }
            else
            {
                // origin is 64x64, we scale to 32x32
                // TODO: cli option to chose
#if 1
                ALIGN_VAR_32(Pel, buf_scale[32 * 32]);
                primitives.scale2D_64to32(buf_scale, fenc, stride);
                primitives.transpose[3](buf_trans, buf_scale, 32);

                // reserve space in case primitives need to store data in above
                // or left buffers
                Pel _above[4 * 32 + 1];
                Pel _left[4 * 32 + 1];
                Pel *const above = _above + 2 * 32;
                Pel *const left = _left + 2 * 32;

                above[0] = left[0] = pAbove0[0];
                primitives.scale1D_128to64(above + 1, pAbove0 + 1, 0);
                primitives.scale1D_128to64(left + 1, pLeft0 + 1, 0);

                // 1
                primitives.intra_pred_dc(above + 1, left + 1, tmp, 32, 32, false);
                modeCosts[DC_IDX] = 4 * primitives.sa8d[3](buf_scale, 32, tmp, 32);

                // 0
                primitives.intra_pred_planar((pixel*)above + 1, (pixel*)left + 1, tmp, 32, 32);
                modeCosts[PLANAR_IDX] = 4 * primitives.sa8d[3](buf_scale, 32, tmp, 32);

                primitives.intra_pred_allangs[3](tmp, above, left, above, left, false);

                // TODO: I use 4 of SATD32x32 to replace real 64x64
                for (UInt mode = 2; mode < numModesAvailable; mode++)
                {
                    bool modeHor = (mode < 18);
                    Pel *cmp_buf = (modeHor ? buf_trans : buf_scale);
                    modeCosts[mode] = 4 * primitives.sa8d[3]((pixel*)cmp_buf, 32, (pixel*)&tmp[(mode - 2) * (32 * 32)], 32);
                }

#else // if 1
                // 1
                primitives.intra_pred_dc(pAbove0 + 1, pLeft0 + 1, pred, stride, width, false);
                modeCosts[DC_IDX] = sa8d(fenc, stride, pred, stride);

                // 0
                primitives.intra_pred_planar((pixel*)pAbove0 + 1, (pixel*)pLeft0 + 1, pred, stride, width);
                modeCosts[PLANAR_IDX] = sa8d(fenc, stride, pred, stride);

                for (UInt mode = 2; mode < numModesAvailable; mode++)
                {
                    predIntraLumaAng(mode, pred, stride, width);
                    modeCosts[mode] = sa8d(fenc, stride, pred, stride);
                }

#endif // if 1
            }

            // Find N least cost modes. N = numModesForFullRD
            for (UInt mode = 0; mode < numModesAvailable; mode++)
            {
                UInt sad = modeCosts[mode];
                UInt bits = xModeBitsIntra(cu, mode, partOffset, depth, initTrDepth);
                UInt64 cost = m_rdCost->calcRdSADCost(sad, bits);
                candNum += xUpdateCandList(mode, cost, numModesForFullRD, rdModeList, candCostList);
            }

            int preds[3] = { -1, -1, -1 };
            int mode = -1;
            int numCand = cu->getIntraDirLumaPredictor(partOffset, preds, &mode);
            if (mode >= 0)
            {
                numCand = mode;
            }

            for (int j = 0; j < numCand; j++)
            {
                bool mostProbableModeIncluded = false;
                int mostProbableMode = preds[j];

                for (int i = 0; i < numModesForFullRD; i++)
                {
                    mostProbableModeIncluded |= (mostProbableMode == rdModeList[i]);
                }

                if (!mostProbableModeIncluded)
                {
                    rdModeList[numModesForFullRD++] = mostProbableMode;
                }
            }
        }
        else
        {
            for (int i = 0; i < numModesForFullRD; i++)
            {
                rdModeList[i] = i;
            }
        }
        x265_emms();

        //===== check modes (using r-d costs) =====
        UInt    bestPUMode  = 0;
        UInt    bestPUDistY = 0;
        UInt    bestPUDistC = 0;
        UInt64  bestPUCost  = MAX_INT64;
        for (UInt mode = 0; mode < numModesForFullRD; mode++)
        {
            // set luma prediction mode
            UInt origMode = rdModeList[mode];

            cu->setLumaIntraDirSubParts(origMode, partOffset, depth + initTrDepth);

            // set context models
            m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_CURR_BEST]);

            // determine residual for partition
            UInt   puDistY = 0;
            UInt   puDistC = 0;
            UInt64 puCost  = 0;
            xRecurIntraCodingQT(cu, initTrDepth, partOffset, bLumaOnly, fencYuv, predYuv, resiYuv, puDistY, puDistC, true, puCost);

            // check r-d cost
            if (puCost < bestPUCost)
            {
                bestPUMode  = origMode;
                bestPUDistY = puDistY;
                bestPUDistC = puDistC;
                bestPUCost  = puCost;

                xSetIntraResultQT(cu, initTrDepth, partOffset, bLumaOnly, reconYuv);

                UInt qPartNum = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + initTrDepth) << 1);
                ::memcpy(m_qtTempTrIdx,  cu->getTransformIdx()     + partOffset, qPartNum * sizeof(UChar));
                ::memcpy(m_qtTempCbf[0], cu->getCbf(TEXT_LUMA)     + partOffset, qPartNum * sizeof(UChar));
                ::memcpy(m_qtTempCbf[1], cu->getCbf(TEXT_CHROMA_U) + partOffset, qPartNum * sizeof(UChar));
                ::memcpy(m_qtTempCbf[2], cu->getCbf(TEXT_CHROMA_V) + partOffset, qPartNum * sizeof(UChar));
                ::memcpy(m_qtTempTransformSkipFlag[0], cu->getTransformSkip(TEXT_LUMA)     + partOffset, qPartNum * sizeof(UChar));
                ::memcpy(m_qtTempTransformSkipFlag[1], cu->getTransformSkip(TEXT_CHROMA_U) + partOffset, qPartNum * sizeof(UChar));
                ::memcpy(m_qtTempTransformSkipFlag[2], cu->getTransformSkip(TEXT_CHROMA_V) + partOffset, qPartNum * sizeof(UChar));
            }
        } // Mode loop

        {
            UInt origMode = bestPUMode;

            cu->setLumaIntraDirSubParts(origMode, partOffset, depth + initTrDepth);

            // set context models
            m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_CURR_BEST]);

            // determine residual for partition
            UInt   puDistY = 0;
            UInt   puDistC = 0;
            UInt64 puCost  = 0;
            xRecurIntraCodingQT(cu, initTrDepth, partOffset, bLumaOnly, fencYuv, predYuv, resiYuv, puDistY, puDistC, false, puCost);

            // check r-d cost
            if (puCost < bestPUCost)
            {
                bestPUMode  = origMode;
                bestPUDistY = puDistY;
                bestPUDistC = puDistC;
                bestPUCost  = puCost;

                xSetIntraResultQT(cu, initTrDepth, partOffset, bLumaOnly, reconYuv);

                UInt qPartNum = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + initTrDepth) << 1);
                ::memcpy(m_qtTempTrIdx,  cu->getTransformIdx()     + partOffset, qPartNum * sizeof(UChar));
                ::memcpy(m_qtTempCbf[0], cu->getCbf(TEXT_LUMA)     + partOffset, qPartNum * sizeof(UChar));
                ::memcpy(m_qtTempCbf[1], cu->getCbf(TEXT_CHROMA_U) + partOffset, qPartNum * sizeof(UChar));
                ::memcpy(m_qtTempCbf[2], cu->getCbf(TEXT_CHROMA_V) + partOffset, qPartNum * sizeof(UChar));
                ::memcpy(m_qtTempTransformSkipFlag[0], cu->getTransformSkip(TEXT_LUMA)     + partOffset, qPartNum * sizeof(UChar));
                ::memcpy(m_qtTempTransformSkipFlag[1], cu->getTransformSkip(TEXT_CHROMA_U) + partOffset, qPartNum * sizeof(UChar));
                ::memcpy(m_qtTempTransformSkipFlag[2], cu->getTransformSkip(TEXT_CHROMA_V) + partOffset, qPartNum * sizeof(UChar));
            }
        } // Mode loop

        //--- update overall distortion ---
        overallDistY += bestPUDistY;
        overallDistC += bestPUDistC;

        //--- update transform index and cbf ---
        UInt qPartNum = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + initTrDepth) << 1);
        ::memcpy(cu->getTransformIdx()     + partOffset, m_qtTempTrIdx,  qPartNum * sizeof(UChar));
        ::memcpy(cu->getCbf(TEXT_LUMA)     + partOffset, m_qtTempCbf[0], qPartNum * sizeof(UChar));
        ::memcpy(cu->getCbf(TEXT_CHROMA_U) + partOffset, m_qtTempCbf[1], qPartNum * sizeof(UChar));
        ::memcpy(cu->getCbf(TEXT_CHROMA_V) + partOffset, m_qtTempCbf[2], qPartNum * sizeof(UChar));
        ::memcpy(cu->getTransformSkip(TEXT_LUMA)     + partOffset, m_qtTempTransformSkipFlag[0], qPartNum * sizeof(UChar));
        ::memcpy(cu->getTransformSkip(TEXT_CHROMA_U) + partOffset, m_qtTempTransformSkipFlag[1], qPartNum * sizeof(UChar));
        ::memcpy(cu->getTransformSkip(TEXT_CHROMA_V) + partOffset, m_qtTempTransformSkipFlag[2], qPartNum * sizeof(UChar));
        //--- set reconstruction for next intra prediction blocks ---
        if (pu != numPU - 1)
        {
            bool bSkipChroma  = false;
            bool bChromaSame  = false;
            UInt trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> (cu->getDepth(0) + initTrDepth)] + 2;
            if (!bLumaOnly && trSizeLog2 == 2)
            {
                assert(initTrDepth  > 0);
                bSkipChroma  = (pu != 0);
                bChromaSame  = true;
            }

            UInt compWidth   = cu->getWidth(0) >> initTrDepth;
            UInt compHeight  = cu->getHeight(0) >> initTrDepth;
            UInt zorder      = cu->getZorderIdxInCU() + partOffset;
            Pel* dst         = cu->getPic()->getPicYuvRec()->getLumaAddr(cu->getAddr(), zorder);
            UInt dststride   = cu->getPic()->getPicYuvRec()->getStride();
            Pel* src         = reconYuv->getLumaAddr(partOffset);
            UInt srcstride   = reconYuv->getStride();
            primitives.blockcpy_pp(compWidth, compHeight, dst, dststride, src, srcstride);

            if (!bLumaOnly && !bSkipChroma)
            {
                if (!bChromaSame)
                {
                    compWidth   >>= 1;
                    compHeight  >>= 1;
                }
                dst         = cu->getPic()->getPicYuvRec()->getCbAddr(cu->getAddr(), zorder);
                dststride   = cu->getPic()->getPicYuvRec()->getCStride();
                src         = reconYuv->getCbAddr(partOffset);
                srcstride   = reconYuv->getCStride();
                primitives.blockcpy_pp(compWidth, compHeight, dst, dststride, src, srcstride);

                dst         = cu->getPic()->getPicYuvRec()->getCrAddr(cu->getAddr(), zorder);
                src         = reconYuv->getCrAddr(partOffset);
                primitives.blockcpy_pp(compWidth, compHeight, dst, dststride, src, srcstride);
            }
        }

        //=== update PU data ====
        cu->setLumaIntraDirSubParts(bestPUMode, partOffset, depth + initTrDepth);
        cu->copyToPic(depth, pu, initTrDepth);
    } // PU loop

    if (numPU > 1)
    { // set Cbf for all blocks
        UInt combCbfY = 0;
        UInt combCbfU = 0;
        UInt combCbfV = 0;
        UInt partIdx  = 0;
        for (UInt part = 0; part < 4; part++, partIdx += qNumParts)
        {
            combCbfY |= cu->getCbf(partIdx, TEXT_LUMA,     1);
            combCbfU |= cu->getCbf(partIdx, TEXT_CHROMA_U, 1);
            combCbfV |= cu->getCbf(partIdx, TEXT_CHROMA_V, 1);
        }

        for (UInt offs = 0; offs < 4 * qNumParts; offs++)
        {
            cu->getCbf(TEXT_LUMA)[offs] |= combCbfY;
            cu->getCbf(TEXT_CHROMA_U)[offs] |= combCbfU;
            cu->getCbf(TEXT_CHROMA_V)[offs] |= combCbfV;
        }
    }

    //===== reset context models =====
    m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_CURR_BEST]);

    //===== set distortion (rate and r-d costs are determined later) =====
    outDistC                 = overallDistC;
    cu->m_totalDistortion = overallDistY + overallDistC;
}

void TEncSearch::estIntraPredChromaQT(TComDataCU* cu,
                                      TComYuv*    fencYuv,
                                      TComYuv*    predYuv,
                                      TShortYUV*  resiYuv,
                                      TComYuv*    reconYuv,
                                      UInt        preCalcDistC)
{
    UInt   depth     = cu->getDepth(0);
    UInt   bestMode  = 0;
    UInt   bestDist  = 0;
    UInt64 bestCost  = MAX_INT64;

    //----- init mode list -----
    UInt minMode = 0;
    UInt maxMode = NUM_CHROMA_MODE;
    UInt modeList[NUM_CHROMA_MODE];

    cu->getAllowedChromaDir(0, modeList);

    //----- check chroma modes -----
    for (UInt mode = minMode; mode < maxMode; mode++)
    {
        //----- restore context models -----
        m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_CURR_BEST]);

        //----- chroma coding -----
        UInt dist = 0;
        cu->setChromIntraDirSubParts(modeList[mode], 0, depth);
        xRecurIntraChromaCodingQT(cu, 0, 0, fencYuv, predYuv, resiYuv, dist);
        if (cu->getSlice()->getPPS()->getUseTransformSkip())
        {
            m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_CURR_BEST]);
        }

        UInt   bits = xGetIntraBitsQT(cu, 0, 0, false, true);
        UInt64 cost  = m_rdCost->calcRdCost(dist, bits);

        //----- compare -----
        if (cost < bestCost)
        {
            bestCost = cost;
            bestDist = dist;
            bestMode = modeList[mode];
            UInt qpn = cu->getPic()->getNumPartInCU() >> (depth << 1);
            xSetIntraResultChromaQT(cu, 0, 0, reconYuv);
            ::memcpy(m_qtTempCbf[1], cu->getCbf(TEXT_CHROMA_U), qpn * sizeof(UChar));
            ::memcpy(m_qtTempCbf[2], cu->getCbf(TEXT_CHROMA_V), qpn * sizeof(UChar));
            ::memcpy(m_qtTempTransformSkipFlag[1], cu->getTransformSkip(TEXT_CHROMA_U), qpn * sizeof(UChar));
            ::memcpy(m_qtTempTransformSkipFlag[2], cu->getTransformSkip(TEXT_CHROMA_V), qpn * sizeof(UChar));
        }
    }

    //----- set data -----
    UInt qpn = cu->getPic()->getNumPartInCU() >> (depth << 1);
    ::memcpy(cu->getCbf(TEXT_CHROMA_U), m_qtTempCbf[1], qpn * sizeof(UChar));
    ::memcpy(cu->getCbf(TEXT_CHROMA_V), m_qtTempCbf[2], qpn * sizeof(UChar));
    ::memcpy(cu->getTransformSkip(TEXT_CHROMA_U), m_qtTempTransformSkipFlag[1], qpn * sizeof(UChar));
    ::memcpy(cu->getTransformSkip(TEXT_CHROMA_V), m_qtTempTransformSkipFlag[2], qpn * sizeof(UChar));
    cu->setChromIntraDirSubParts(bestMode, 0, depth);
    cu->m_totalDistortion += bestDist - preCalcDistC;

    //----- restore context models -----
    m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_CURR_BEST]);
}

/** Function for encoding and reconstructing luma/chroma samples of a PCM mode CU.
 * \param cu pointer to current CU
 * \param absPartIdx part index
 * \param fenc pointer to original sample arrays
 * \param pcm pointer to PCM code arrays
 * \param pred pointer to prediction signal arrays
 * \param resi pointer to residual signal arrays
 * \param reco pointer to reconstructed sample arrays
 * \param stride stride of the original/prediction/residual sample arrays
 * \param width block width
 * \param height block height
 * \param ttText texture component type
 * \returns void
 */
void TEncSearch::xEncPCM(TComDataCU* cu, UInt absPartIdx, Pel* fenc, Pel* pcm, Pel* pred, short* resi, Pel* recon, UInt stride, UInt width, UInt height, TextType eText)
{
    UInt x, y;
    UInt reconStride;
    Pel* pcmTmp = pcm;
    Pel* reconPic;
    int shiftPcm;

    if (eText == TEXT_LUMA)
    {
        reconStride = cu->getPic()->getPicYuvRec()->getStride();
        reconPic    = cu->getPic()->getPicYuvRec()->getLumaAddr(cu->getAddr(), cu->getZorderIdxInCU() + absPartIdx);
        shiftPcm = X265_DEPTH - cu->getSlice()->getSPS()->getPCMBitDepthLuma();
    }
    else
    {
        reconStride = cu->getPic()->getPicYuvRec()->getCStride();
        if (eText == TEXT_CHROMA_U)
        {
            reconPic = cu->getPic()->getPicYuvRec()->getCbAddr(cu->getAddr(), cu->getZorderIdxInCU() + absPartIdx);
        }
        else
        {
            reconPic = cu->getPic()->getPicYuvRec()->getCrAddr(cu->getAddr(), cu->getZorderIdxInCU() + absPartIdx);
        }
        shiftPcm = X265_DEPTH - cu->getSlice()->getSPS()->getPCMBitDepthChroma();
    }

    // zero prediction and residual
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            pred[x] = resi[x] = 0;
        }

        pred += stride;
        resi += stride;
    }

    // Encode
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            pcmTmp[x] = fenc[x] >> shiftPcm;
        }

        pcmTmp += width;
        fenc += stride;
    }

    pcmTmp = pcm;

    // Reconstruction
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            recon[x] = pcmTmp[x] << shiftPcm;
            reconPic[x] = recon[x];
        }

        pcmTmp   += width;
        recon    += stride;
        reconPic += reconStride;
    }
}

/**  Function for PCM mode estimation.
 * \param cu
 * \param fencYuv
 * \param rpcPredYuv
 * \param rpcResiYuv
 * \param rpcRecoYuv
 * \returns void
 */
void TEncSearch::IPCMSearch(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, TShortYUV* resiYuv, TComYuv* reconYuv)
{
    UInt depth      = cu->getDepth(0);
    UInt width      = cu->getWidth(0);
    UInt height     = cu->getHeight(0);
    UInt stride     = predYuv->getStride();
    UInt strideC    = predYuv->getCStride();
    UInt widthC     = width  >> 1;
    UInt heightC    = height >> 1;
    UInt distortion = 0;
    UInt bits;
    UInt64 cost;

    UInt absPartIdx = 0;
    UInt minCoeffSize = cu->getPic()->getMinCUWidth() * cu->getPic()->getMinCUHeight();
    UInt lumaOffset   = minCoeffSize * absPartIdx;
    UInt chromaOffset = lumaOffset >> 2;

    // Luminance
    Pel*   fenc = fencYuv->getLumaAddr(0, width);
    short* resi = resiYuv->getLumaAddr(0, width);
    Pel*   pred = predYuv->getLumaAddr(0, width);
    Pel*   recon = reconYuv->getLumaAddr(0, width);
    Pel*   pcm  = cu->getPCMSampleY() + lumaOffset;

    xEncPCM(cu, 0, fenc, pcm, pred, resi, recon, stride, width, height, TEXT_LUMA);

    // Chroma U
    fenc = fencYuv->getCbAddr();
    resi = resiYuv->getCbAddr();
    pred = predYuv->getCbAddr();
    recon = reconYuv->getCbAddr();
    pcm  = cu->getPCMSampleCb() + chromaOffset;

    xEncPCM(cu, 0, fenc, pcm, pred, resi, recon, strideC, widthC, heightC, TEXT_CHROMA_U);

    // Chroma V
    fenc = fencYuv->getCrAddr();
    resi = resiYuv->getCrAddr();
    pred = predYuv->getCrAddr();
    recon = reconYuv->getCrAddr();
    pcm  = cu->getPCMSampleCr() + chromaOffset;

    xEncPCM(cu, 0, fenc, pcm, pred, resi, recon, strideC, widthC, heightC, TEXT_CHROMA_V);

    m_entropyCoder->resetBits();
    xEncIntraHeader(cu, depth, absPartIdx, true, false);
    bits = m_entropyCoder->getNumberOfWrittenBits();
    cost = m_rdCost->calcRdCost(distortion, bits);

    m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_CURR_BEST]);

    cu->m_totalBits       = bits;
    cu->m_totalCost       = cost;
    cu->m_totalDistortion = distortion;

    cu->copyToPic(depth, 0, 0);
}

UInt TEncSearch::xGetInterPredictionError(TComDataCU* cu, int partIdx)
{
    UInt absPartIdx;
    int width, height;

    motionCompensation(cu, &m_tmpYuvPred, REF_PIC_LIST_X, partIdx);
    cu->getPartIndexAndSize(partIdx, absPartIdx, width, height);
    UInt cost = m_me.bufSA8D(m_tmpYuvPred.getLumaAddr(absPartIdx), m_tmpYuvPred.getStride());
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
 * \returns void
 */
void TEncSearch::xMergeEstimation(TComDataCU* cu, int puIdx, UInt& interDir, TComMvField* mvField, UInt& mergeIndex, UInt& outCost, TComMvField* mvFieldNeighbours, UChar* interDirNeighbours, int& numValidMergeCand)
{
    UInt absPartIdx = 0;
    int width = 0;
    int height = 0;

    cu->getPartIndexAndSize(puIdx, absPartIdx, width, height);
    UInt depth = cu->getDepth(absPartIdx);
    PartSize partSize = cu->getPartitionSize(0);
    if (cu->getSlice()->getPPS()->getLog2ParallelMergeLevelMinus2() && partSize != SIZE_2Nx2N && cu->getWidth(0) <= 8)
    {
        cu->setPartSizeSubParts(SIZE_2Nx2N, 0, depth);
        if (puIdx == 0)
        {
            cu->getInterMergeCandidates(0, 0, mvFieldNeighbours, interDirNeighbours, numValidMergeCand);
        }
        cu->setPartSizeSubParts(partSize, 0, depth);
    }
    else
    {
        cu->getInterMergeCandidates(absPartIdx, puIdx, mvFieldNeighbours, interDirNeighbours, numValidMergeCand);
    }
    xRestrictBipredMergeCand(cu, puIdx, mvFieldNeighbours, interDirNeighbours, numValidMergeCand);

    outCost = MAX_UINT;
    for (UInt mergeCand = 0; mergeCand < numValidMergeCand; ++mergeCand)
    {
        UInt costCand = MAX_UINT;
        UInt bitsCand = 0;

        PartSize size = cu->getPartitionSize(0);

        cu->getCUMvField(REF_PIC_LIST_0)->setAllMvField(mvFieldNeighbours[0 + 2 * mergeCand], size, absPartIdx, 0, puIdx);
        cu->getCUMvField(REF_PIC_LIST_1)->setAllMvField(mvFieldNeighbours[1 + 2 * mergeCand], size, absPartIdx, 0, puIdx);

        costCand = xGetInterPredictionError(cu, puIdx);
        bitsCand = mergeCand + 1;
        if (mergeCand == m_cfg->param.maxNumMergeCand - 1)
        {
            bitsCand--;
        }
        costCand = costCand + m_rdCost->getCost(bitsCand);
        if (costCand < outCost)
        {
            outCost = costCand;
            mvField[0] = mvFieldNeighbours[0 + 2 * mergeCand];
            mvField[1] = mvFieldNeighbours[1 + 2 * mergeCand];
            interDir = interDirNeighbours[mergeCand];
            mergeIndex = mergeCand;
        }
    }
}

/** convert bi-pred merge candidates to uni-pred
 * \param cu
 * \param puIdx
 * \param mvFieldNeighbours
 * \param interDirNeighbours
 * \param numValidMergeCand
 * \returns void
 */
void TEncSearch::xRestrictBipredMergeCand(TComDataCU* cu, UInt puIdx, TComMvField* mvFieldNeighbours, UChar* interDirNeighbours, int numValidMergeCand)
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
 * \returns void
 */
void TEncSearch::predInterSearch(TComDataCU* cu, TComYuv* predYuv, bool bUseMRG)
{
    m_predYuv[0].clear();
    m_predYuv[1].clear();
    m_predTempYuv.clear();
    predYuv->clear();

    MV mvmin;
    MV mvmax;
    MV mvzero(0, 0);
    MV mvtmp;
    MV mv[2];
    MV mvBidir[2];
    MV mvTemp[2][33];
    MV mvPred[2][33];
    MV mvPredBi[2][33];

    int mvpIdxBi[2][33];
    int mvpIdx[2][33];
    int mvpNum[2][33];
    AMVPInfo amvpInfo[2][33];

    UInt mbBits[3] = { 1, 1, 0 };
    int refIdx[2] = { 0, 0 }; // If un-initialized, may cause SEGV in bi-directional prediction iterative stage.
    int refIdxBidir[2] = { 0, 0 };

    UInt partAddr;
    int  roiWidth, roiHeight;

    PartSize partSize = cu->getPartitionSize(0);
    UInt lastMode = 0;
    int numPart = cu->getNumPartInter();
    int numPredDir = cu->getSlice()->isInterP() ? 1 : 2;
    UInt biPDistTemp = MAX_INT;

    /* TODO: this could be as high as FrameEncoder::prepareEncode() */
    TComPicYuv *fenc = cu->getSlice()->getPic()->getPicYuvOrg();
    m_me.setSourcePlane(fenc->getLumaAddr(), fenc->getStride());

    TComMvField mvFieldNeighbours[MRG_MAX_NUM_CANDS << 1]; // double length for mv of both lists
    UChar interDirNeighbours[MRG_MAX_NUM_CANDS];
    int numValidMergeCand = 0;

    if (!m_cfg->param.bEnableRDO)
        cu->m_totalCost = 0;

    for (int partIdx = 0; partIdx < numPart; partIdx++)
    {
        UInt bitsTempL0[MAX_NUM_REF];
        UInt listCost[2] = { MAX_UINT, MAX_UINT };
        UInt bits[3];
        UInt costbi = MAX_UINT;
        UInt costTemp = 0;
        UInt bitsTemp;
        UInt bestBiPDist = MAX_INT;
        MV   mvValidList1;
        int  refIdxValidList1 = 0;
        UInt bitsValidList1 = MAX_UINT;
        UInt costValidList1 = MAX_UINT;
        UInt costTempL0[MAX_NUM_REF];
        for (int ref = 0; ref < MAX_NUM_REF; ref++)
        {
            costTempL0[ref] = MAX_UINT;
        }

        xGetBlkBits(partSize, cu->getSlice()->isInterP(), partIdx, lastMode, mbBits);
        cu->getPartIndexAndSize(partIdx, partAddr, roiWidth, roiHeight);

        Pel* pu = fenc->getLumaAddr(cu->getAddr(), cu->getZorderIdxInCU() + partAddr);
        m_me.setSourcePU(pu - fenc->getLumaAddr(), roiWidth, roiHeight);

        cu->getMvPredLeft(m_mvPredictors[0]);
        cu->getMvPredAbove(m_mvPredictors[1]);
        cu->getMvPredAboveRight(m_mvPredictors[2]);

        bool bTestNormalMC = true;

        if (bUseMRG && cu->getWidth(0) > 8 && numPart == 2)
        {
            bTestNormalMC = false;
        }

        if (bTestNormalMC)
        {
            // Uni-directional prediction
            for (int refList = 0; refList < numPredDir; refList++)
            {
                RefPicList picList = (refList ? REF_PIC_LIST_1 : REF_PIC_LIST_0);

                for (int refIdxTmp = 0; refIdxTmp < cu->getSlice()->getNumRefIdx(picList); refIdxTmp++)
                {
                    bitsTemp = mbBits[refList];
                    if (cu->getSlice()->getNumRefIdx(picList) > 1)
                    {
                        bitsTemp += refIdxTmp + 1;
                        if (refIdxTmp == cu->getSlice()->getNumRefIdx(picList) - 1) bitsTemp--;
                    }
                    xEstimateMvPredAMVP(cu, partIdx, picList, refIdxTmp, mvPred[refList][refIdxTmp], &biPDistTemp);
                    mvpIdx[refList][refIdxTmp] = cu->getMVPIdx(picList, partAddr);
                    mvpNum[refList][refIdxTmp] = cu->getMVPNum(picList, partAddr);

                    if (cu->getSlice()->getMvdL1ZeroFlag() && refList == 1 && biPDistTemp < bestBiPDist)
                    {
                        bestBiPDist = biPDistTemp;
                    }

                    bitsTemp += m_mvpIdxCost[mvpIdx[refList][refIdxTmp]][AMVP_MAX_NUM_CANDS];

                    if (refList == 1) // list 1
                    {
                        if (cu->getSlice()->getList1IdxToList0Idx(refIdxTmp) >= 0)
                        {
                            mvTemp[1][refIdxTmp] = mvTemp[0][cu->getSlice()->getList1IdxToList0Idx(refIdxTmp)];
                            costTemp = costTempL0[cu->getSlice()->getList1IdxToList0Idx(refIdxTmp)];

                            /* first subtract the bit-rate part of the cost of the other list */
                            costTemp -= m_rdCost->getCost(bitsTempL0[cu->getSlice()->getList1IdxToList0Idx(refIdxTmp)]);

                            /* correct the bit-rate part of the current ref */
                            m_me.setMVP(mvPred[refList][refIdxTmp]);
                            bitsTemp += m_me.bitcost(mvTemp[1][refIdxTmp]);

                            /* calculate the correct cost */
                            costTemp += m_rdCost->getCost(bitsTemp);
                        }
                        else
                        {
                            CYCLE_COUNTER_START(ME);
                            int merange = m_adaptiveRange[picList][refIdxTmp];
                            MV& mvp = mvPred[refList][refIdxTmp];
                            MV& outmv = mvTemp[refList][refIdxTmp];
                            xSetSearchRange(cu, mvp, merange, mvmin, mvmax);
                            int satdCost = m_me.motionEstimate(cu->getSlice()->m_mref[picList][refIdxTmp],
                                                               mvmin, mvmax, mvp, 3, m_mvPredictors, merange, outmv);

                            /* Get total cost of partition, but only include MV bit cost once */
                            bitsTemp += m_me.bitcost(outmv);
                            costTemp = (satdCost - m_me.mvcost(outmv)) + m_rdCost->getCost(bitsTemp);
                            CYCLE_COUNTER_STOP(ME);
                        }
                    }
                    else
                    {
                        CYCLE_COUNTER_START(ME);
                        int merange = m_adaptiveRange[picList][refIdxTmp];
                        MV& mvp = mvPred[refList][refIdxTmp];
                        MV& outmv = mvTemp[refList][refIdxTmp];
                        xSetSearchRange(cu, mvp, merange, mvmin, mvmax);
                        int satdCost = m_me.motionEstimate(cu->getSlice()->m_mref[picList][refIdxTmp],
                                                            mvmin, mvmax, mvp, 3, m_mvPredictors, merange, outmv);

                        /* Get total cost of partition, but only include MV bit cost once */
                        bitsTemp += m_me.bitcost(outmv);
                        costTemp = (satdCost - m_me.mvcost(outmv)) + m_rdCost->getCost(bitsTemp);
                        CYCLE_COUNTER_STOP(ME);
                    }
                    xCopyAMVPInfo(cu->getCUMvField(picList)->getAMVPInfo(), &amvpInfo[refList][refIdxTmp]); // must always be done ( also when AMVP_MODE = AM_NONE )
                    xCheckBestMVP(cu, picList, mvTemp[refList][refIdxTmp], mvPred[refList][refIdxTmp], mvpIdx[refList][refIdxTmp], bitsTemp, costTemp);

                    if (refList == 0)
                    {
                        costTempL0[refIdxTmp] = costTemp;
                        bitsTempL0[refIdxTmp] = bitsTemp;
                    }
                    if (costTemp < listCost[refList])
                    {
                        listCost[refList] = costTemp;
                        bits[refList] = bitsTemp; // storing for bi-prediction

                        // set motion
                        mv[refList] = mvTemp[refList][refIdxTmp];
                        refIdx[refList] = refIdxTmp;
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
                refIdxBidir[0] = refIdx[0];
                refIdxBidir[1] = refIdx[1];

                ::memcpy(mvPredBi, mvPred, sizeof(mvPred));
                ::memcpy(mvpIdxBi, mvpIdx, sizeof(mvpIdx));

                pixel *ref0, *ref1;

                //Generate reference subpels
                xPredInterLumaBlk(cu, cu->getSlice()->m_mref[0][refIdx[0]], partAddr, &mv[0], roiWidth, roiHeight, &m_predYuv[0]);
                xPredInterLumaBlk(cu, cu->getSlice()->m_mref[1][refIdx[1]], partAddr, &mv[1], roiWidth, roiHeight, &m_predYuv[1]);

                ref0 = m_predYuv[0].getLumaAddr(partAddr);
                ref1 = m_predYuv[1].getLumaAddr(partAddr);

                pixel avg[MAX_CU_SIZE * MAX_CU_SIZE];

                int partEnum = PartitionFromSizes(roiWidth, roiHeight);
                primitives.pixelavg_pp[partEnum](avg, roiWidth, ref0, ref1, m_predYuv[0].getStride(), m_predYuv[1].getStride());

                int satdCost = primitives.satd[partEnum](pu, fenc->getStride(), avg, roiWidth);
                bits[2] = bits[0] + bits[1] - mbBits[0] - mbBits[1] + mbBits[2];
                costbi =  satdCost + m_rdCost->getCost(bits[2]);

                if (mv[0].notZero() || mv[1].notZero())
                {
                    ref0 = cu->getSlice()->m_mref[0][refIdx[0]]->fpelPlane + (pu - fenc->getLumaAddr());  //MV(0,0) of ref0
                    ref1 = cu->getSlice()->m_mref[1][refIdx[1]]->fpelPlane + (pu - fenc->getLumaAddr());  //MV(0,0) of ref1
                    intptr_t refStride = cu->getSlice()->m_mref[0][refIdx[0]]->lumaStride;

                    partEnum = PartitionFromSizes(roiWidth, roiHeight);
                    primitives.pixelavg_pp[partEnum](avg, roiWidth, ref0, ref1, refStride, refStride);

                    satdCost = primitives.satd[partEnum](pu, fenc->getStride(), avg, roiWidth);

                    unsigned int bitsZero0, bitsZero1;
                    m_me.setMVP(mvPredBi[0][refIdxBidir[0]]);
                    bitsZero0 = bits[0] - m_me.bitcost(mv[0]) + m_me.bitcost(mvzero);

                    m_me.setMVP(mvPredBi[1][refIdxBidir[1]]);
                    bitsZero1 = bits[1] - m_me.bitcost(mv[1]) + m_me.bitcost(mvzero);

                    xCheckBestMVP(cu, REF_PIC_LIST_0, mvzero, mvPredBi[0][refIdxBidir[0]], mvpIdxBi[0][refIdxBidir[0]], bitsZero0, costTemp);
                    xCheckBestMVP(cu, REF_PIC_LIST_1, mvzero, mvPredBi[1][refIdxBidir[1]], mvpIdxBi[1][refIdxBidir[1]], bitsZero1, costTemp);

                    int costZero =  satdCost + m_rdCost->getCost(bitsZero0) + m_rdCost->getCost(bitsZero1);
                    if (costZero < costbi)
                    {
                        costbi = costZero;
                        mvBidir[0].x = mvBidir[0].y = 0;
                        mvBidir[1].x = mvBidir[1].y = 0;
                        bits[2] = bitsZero0 + bitsZero1 - mbBits[0] - mbBits[1] + mbBits[2];
                    }
                }
            } // if (B_SLICE)
        } //end if bTestNormalMC

        //  Clear Motion Field
        cu->getCUMvField(REF_PIC_LIST_0)->setAllMvField(TComMvField(), partSize, partAddr, 0, partIdx);
        cu->getCUMvField(REF_PIC_LIST_1)->setAllMvField(TComMvField(), partSize, partAddr, 0, partIdx);
        cu->getCUMvField(REF_PIC_LIST_0)->setAllMvd(mvzero, partSize, partAddr, 0, partIdx);
        cu->getCUMvField(REF_PIC_LIST_1)->setAllMvd(mvzero, partSize, partAddr, 0, partIdx);

        cu->setMVPIdxSubParts(-1, REF_PIC_LIST_0, partAddr, partIdx, cu->getDepth(partAddr));
        cu->setMVPNumSubParts(-1, REF_PIC_LIST_0, partAddr, partIdx, cu->getDepth(partAddr));
        cu->setMVPIdxSubParts(-1, REF_PIC_LIST_1, partAddr, partIdx, cu->getDepth(partAddr));
        cu->setMVPNumSubParts(-1, REF_PIC_LIST_1, partAddr, partIdx, cu->getDepth(partAddr));

        UInt mebits = 0;
        // Set Motion Field_
        mv[1] = mvValidList1;
        refIdx[1] = refIdxValidList1;
        bits[1] = bitsValidList1;
        listCost[1] = costValidList1;

        if (bTestNormalMC)
        {
            if (costbi <= listCost[0] && costbi <= listCost[1])
            {
                lastMode = 2;
                {
                    cu->getCUMvField(REF_PIC_LIST_0)->setAllMv(mvBidir[0], partSize, partAddr, 0, partIdx);
                    cu->getCUMvField(REF_PIC_LIST_0)->setAllRefIdx(refIdxBidir[0], partSize, partAddr, 0, partIdx);
                    cu->getCUMvField(REF_PIC_LIST_1)->setAllMv(mvBidir[1], partSize, partAddr, 0, partIdx);
                    cu->getCUMvField(REF_PIC_LIST_1)->setAllRefIdx(refIdxBidir[1], partSize, partAddr, 0, partIdx);
                }
                {
                    mvtmp = mvBidir[0] - mvPredBi[0][refIdxBidir[0]];
                    cu->getCUMvField(REF_PIC_LIST_0)->setAllMvd(mvtmp, partSize, partAddr, 0, partIdx);
                }
                {
                    mvtmp = mvBidir[1] - mvPredBi[1][refIdxBidir[1]];
                    cu->getCUMvField(REF_PIC_LIST_1)->setAllMvd(mvtmp, partSize, partAddr, 0, partIdx);
                }

                cu->setInterDirSubParts(3, partAddr, partIdx, cu->getDepth(0));

                cu->setMVPIdxSubParts(mvpIdxBi[0][refIdxBidir[0]], REF_PIC_LIST_0, partAddr, partIdx, cu->getDepth(partAddr));
                cu->setMVPNumSubParts(mvpNum[0][refIdxBidir[0]], REF_PIC_LIST_0, partAddr, partIdx, cu->getDepth(partAddr));
                cu->setMVPIdxSubParts(mvpIdxBi[1][refIdxBidir[1]], REF_PIC_LIST_1, partAddr, partIdx, cu->getDepth(partAddr));
                cu->setMVPNumSubParts(mvpNum[1][refIdxBidir[1]], REF_PIC_LIST_1, partAddr, partIdx, cu->getDepth(partAddr));

                mebits = bits[2];
            }
            else if (listCost[0] <= listCost[1])
            {
                lastMode = 0;
                cu->getCUMvField(REF_PIC_LIST_0)->setAllMv(mv[0], partSize, partAddr, 0, partIdx);
                cu->getCUMvField(REF_PIC_LIST_0)->setAllRefIdx(refIdx[0], partSize, partAddr, 0, partIdx);
                {
                    mvtmp = mv[0] - mvPred[0][refIdx[0]];
                    cu->getCUMvField(REF_PIC_LIST_0)->setAllMvd(mvtmp, partSize, partAddr, 0, partIdx);
                }
                cu->setInterDirSubParts(1, partAddr, partIdx, cu->getDepth(0));

                cu->setMVPIdxSubParts(mvpIdx[0][refIdx[0]], REF_PIC_LIST_0, partAddr, partIdx, cu->getDepth(partAddr));
                cu->setMVPNumSubParts(mvpNum[0][refIdx[0]], REF_PIC_LIST_0, partAddr, partIdx, cu->getDepth(partAddr));

                mebits = bits[0];
            }
            else
            {
                lastMode = 1;
                cu->getCUMvField(REF_PIC_LIST_1)->setAllMv(mv[1], partSize, partAddr, 0, partIdx);
                cu->getCUMvField(REF_PIC_LIST_1)->setAllRefIdx(refIdx[1], partSize, partAddr, 0, partIdx);
                {
                    mvtmp = mv[1] - mvPred[1][refIdx[1]];
                    cu->getCUMvField(REF_PIC_LIST_1)->setAllMvd(mvtmp, partSize, partAddr, 0, partIdx);
                }
                cu->setInterDirSubParts(2, partAddr, partIdx, cu->getDepth(0));

                cu->setMVPIdxSubParts(mvpIdx[1][refIdx[1]], REF_PIC_LIST_1, partAddr, partIdx, cu->getDepth(partAddr));
                cu->setMVPNumSubParts(mvpNum[1][refIdx[1]], REF_PIC_LIST_1, partAddr, partIdx, cu->getDepth(partAddr));

                mebits = bits[1];
            }
#if CU_STAT_LOGFILE
            meCost += listCost[0];
#endif
        } // end if bTestNormalMC

        if (cu->getPartitionSize(partAddr) != SIZE_2Nx2N)
        {
            UInt msgInterDir = 0;
            TComMvField mrgMvField[2];
            UInt msgIndex = 0;

            UInt meInterDir = 0;
            TComMvField meMvField[2];

            // calculate ME cost
            UInt meError = MAX_UINT;
            UInt meCost = MAX_UINT;

            if (bTestNormalMC)
            {
                meError = xGetInterPredictionError(cu, partIdx);
                meCost = meError + m_rdCost->getCost(mebits);
            }

            // save ME result.
            meInterDir = cu->getInterDir(partAddr);
            cu->getMvField(cu, partAddr, REF_PIC_LIST_0, meMvField[0]);
            cu->getMvField(cu, partAddr, REF_PIC_LIST_1, meMvField[1]);

            // find Merge result
            UInt mrgCost = MAX_UINT;
            xMergeEstimation(cu, partIdx, msgInterDir, mrgMvField, msgIndex, mrgCost, mvFieldNeighbours, interDirNeighbours, numValidMergeCand);
            if (mrgCost < meCost)
            {
                // set Merge result
                cu->setMergeFlagSubParts(true, partAddr, partIdx, cu->getDepth(partAddr));
                cu->setMergeIndexSubParts(msgIndex, partAddr, partIdx, cu->getDepth(partAddr));
                cu->setInterDirSubParts(msgInterDir, partAddr, partIdx, cu->getDepth(partAddr));
                {
                    cu->getCUMvField(REF_PIC_LIST_0)->setAllMvField(mrgMvField[0], partSize, partAddr, 0, partIdx);
                    cu->getCUMvField(REF_PIC_LIST_1)->setAllMvField(mrgMvField[1], partSize, partAddr, 0, partIdx);
                }

                cu->getCUMvField(REF_PIC_LIST_0)->setAllMvd(mvzero, partSize, partAddr, 0, partIdx);
                cu->getCUMvField(REF_PIC_LIST_1)->setAllMvd(mvzero, partSize, partAddr, 0, partIdx);

                cu->setMVPIdxSubParts(-1, REF_PIC_LIST_0, partAddr, partIdx, cu->getDepth(partAddr));
                cu->setMVPNumSubParts(-1, REF_PIC_LIST_0, partAddr, partIdx, cu->getDepth(partAddr));
                cu->setMVPIdxSubParts(-1, REF_PIC_LIST_1, partAddr, partIdx, cu->getDepth(partAddr));
                cu->setMVPNumSubParts(-1, REF_PIC_LIST_1, partAddr, partIdx, cu->getDepth(partAddr));
#if CU_STAT_LOGFILE
                meCost += mrgCost;
#endif
                if (!m_cfg->param.bEnableRDO)
                    cu->m_totalCost += mrgCost;
            }
            else
            {
                // set ME result
                cu->setMergeFlagSubParts(false, partAddr, partIdx, cu->getDepth(partAddr));
                cu->setInterDirSubParts(meInterDir, partAddr, partIdx, cu->getDepth(partAddr));
                {
                    cu->getCUMvField(REF_PIC_LIST_0)->setAllMvField(meMvField[0], partSize, partAddr, 0, partIdx);
                    cu->getCUMvField(REF_PIC_LIST_1)->setAllMvField(meMvField[1], partSize, partAddr, 0, partIdx);
                }
#if CU_STAT_LOGFILE
                meCost += meCost;
#endif
                if (!m_cfg->param.bEnableRDO)
                    cu->m_totalCost += meCost;
            }
        }
        else
        {
            if (!m_cfg->param.bEnableRDO)
                cu->m_totalCost += costTemp;
        }
        motionCompensation(cu, predYuv, REF_PIC_LIST_X, partIdx);
    }

    setWpScalingDistParam(cu, -1, REF_PIC_LIST_X);
}

// AMVP
void TEncSearch::xEstimateMvPredAMVP(TComDataCU* cu, UInt partIdx, RefPicList picList, int refIdx, MV& mvPred, UInt* distBiP)
{
    AMVPInfo* amvpInfo = cu->getCUMvField(picList)->getAMVPInfo();

    MV   bestMv;
    int  bestIdx = 0;
    UInt bestCost = MAX_INT;
    UInt partAddr = 0;
    int  roiWidth, roiHeight;
    int  i;

    cu->getPartIndexAndSize(partIdx, partAddr, roiWidth, roiHeight);

    // Fill the MV Candidates
    cu->fillMvpCand(partIdx, partAddr, picList, refIdx, amvpInfo);

    bestMv = amvpInfo->m_mvCand[0];
    if (amvpInfo->m_num <= 1)
    {
        mvPred = bestMv;

        cu->setMVPIdxSubParts(bestIdx, picList, partAddr, partIdx, cu->getDepth(partAddr));
        cu->setMVPNumSubParts(amvpInfo->m_num, picList, partAddr, partIdx, cu->getDepth(partAddr));

        if (cu->getSlice()->getMvdL1ZeroFlag() && picList == REF_PIC_LIST_1)
        {
            (*distBiP) = xGetTemplateCost(cu, partAddr, &m_predTempYuv, mvPred, 0, AMVP_MAX_NUM_CANDS, picList, refIdx, roiWidth, roiHeight);
        }
        return;
    }

    m_predTempYuv.clear();

    //-- Check Minimum Cost.
    for (i = 0; i < amvpInfo->m_num; i++)
    {
        UInt cost = xGetTemplateCost(cu, partAddr, &m_predTempYuv, amvpInfo->m_mvCand[i], i, AMVP_MAX_NUM_CANDS, picList, refIdx, roiWidth, roiHeight);
        if (bestCost > cost)
        {
            bestCost = cost;
            bestMv   = amvpInfo->m_mvCand[i];
            bestIdx  = i;
            (*distBiP) = cost;
        }
    }

    m_predTempYuv.clear();

    // Setting Best MVP
    mvPred = bestMv;
    cu->setMVPIdxSubParts(bestIdx, picList, partAddr, partIdx, cu->getDepth(partAddr));
    cu->setMVPNumSubParts(amvpInfo->m_num, picList, partAddr, partIdx, cu->getDepth(partAddr));
}

UInt TEncSearch::xGetMvpIdxBits(int idx, int num)
{
    assert(idx >= 0 && num >= 0 && idx < num);

    if (num == 1)
        return 0;

    UInt length = 1;
    int temp = idx;
    if (temp == 0)
    {
        return length;
    }

    bool bCodeLast = (num - 1 > temp);

    length += (temp - 1);

    if (bCodeLast)
    {
        length++;
    }

    return length;
}

void TEncSearch::xGetBlkBits(PartSize cuMode, bool bPSlice, int partIdx, UInt lastMode, UInt blockBit[3])
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

void TEncSearch::xCopyAMVPInfo(AMVPInfo* src, AMVPInfo* dst)
{
    dst->m_num = src->m_num;
    for (int i = 0; i < src->m_num; i++)
    {
        dst->m_mvCand[i] = src->m_mvCand[i];
    }
}

/* Check if using an alternative MVP would result in a smaller MVD + signal bits */
void TEncSearch::xCheckBestMVP(TComDataCU* cu, RefPicList picList, MV mv, MV& mvPred, int& outMvpIdx, UInt& outBits, UInt& outCost)
{
    AMVPInfo* amvpInfo = cu->getCUMvField(picList)->getAMVPInfo();

    assert(amvpInfo->m_mvCand[outMvpIdx] == mvPred);
    if (amvpInfo->m_num < 2) return;

    m_me.setMVP(mvPred);
    int bestMvpIdx = outMvpIdx;
    int mvBitsOrig = m_me.bitcost(mv) + m_mvpIdxCost[outMvpIdx][AMVP_MAX_NUM_CANDS];
    int bestMvBits = mvBitsOrig;

    for (int mvpIdx = 0; mvpIdx < amvpInfo->m_num; mvpIdx++)
    {
        if (mvpIdx == outMvpIdx)
            continue;

        m_me.setMVP(amvpInfo->m_mvCand[mvpIdx]);
        int mvbits = m_me.bitcost(mv) + m_mvpIdxCost[mvpIdx][AMVP_MAX_NUM_CANDS];

        if (mvbits < bestMvBits)
        {
            bestMvBits = mvbits;
            bestMvpIdx = mvpIdx;
        }
    }

    if (bestMvpIdx != outMvpIdx) // if changed
    {
        mvPred = amvpInfo->m_mvCand[bestMvpIdx];

        outMvpIdx = bestMvpIdx;
        UInt origOutBits = outBits;
        outBits = origOutBits - mvBitsOrig + bestMvBits;
        outCost = (outCost - m_rdCost->getCost(origOutBits))  + m_rdCost->getCost(outBits);
    }
}

UInt TEncSearch::xGetTemplateCost(TComDataCU* cu, UInt partAddr, TComYuv* templateCand, MV mvCand, int mvpIdx,
                                  int mvpCandCount, RefPicList picList, int refIdx, int sizex, int sizey)
{
    // TODO: does it clip with m_referenceRowsAvailable?
    cu->clipMv(mvCand);

    // prediction pattern
    MotionReference* ref = cu->getSlice()->m_mref[picList][refIdx];
    xPredInterLumaBlk(cu, ref, partAddr, &mvCand, sizex, sizey, templateCand);

    // calc distortion
    UInt cost = m_me.bufSAD(templateCand->getLumaAddr(partAddr), templateCand->getStride());
    x265_emms();
    return m_rdCost->calcRdSADCost(cost, m_mvpIdxCost[mvpIdx][mvpCandCount]);
}

void TEncSearch::xSetSearchRange(TComDataCU* cu, MV mvp, int merange, MV& mvmin, MV& mvmax)
{
    cu->clipMv(mvp);

    MV dist(merange << 2, merange << 2);
    mvmin = mvp - dist;
    mvmax = mvp + dist;

    cu->clipMv(mvmin);
    cu->clipMv(mvmax);

    mvmin >>= 2;
    mvmax >>= 2;

    /* conditional clipping for frame parallelism */
    mvmin.y = X265_MIN(mvmin.y, m_refLagPixels);
    mvmax.y = X265_MIN(mvmax.y, m_refLagPixels);
}

/** encode residual and calculate rate-distortion for a CU block
 * \param cu
 * \param fencYuv
 * \param predYuv
 * \param outResiYuv
 * \param rpcYuvResiBest
 * \param outReconYuv
 * \param bSkipRes
 * \returns void
 */
void TEncSearch::encodeResAndCalcRdInterCU(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, TShortYUV* outResiYuv,
                                           TShortYUV* outBestResiYuv, TComYuv* outReconYuv, bool bSkipRes)
{
    if (cu->isIntra(0))
    {
        return;
    }

    bool bHighPass = cu->getSlice()->getSliceType() == B_SLICE;
    UInt bits = 0, bestBits = 0;
    UInt distortion = 0, bdist = 0;

    UInt width  = cu->getWidth(0);
    UInt height = cu->getHeight(0);

    //  No residual coding : SKIP mode
    if (bSkipRes)
    {
        cu->setSkipFlagSubParts(true, 0, cu->getDepth(0));

        outResiYuv->clear();

        predYuv->copyToPartYuv(outReconYuv, 0);

        int part = PartitionFromSizes(width, height);
        distortion = primitives.sse_pp[part](fencYuv->getLumaAddr(), fencYuv->getStride(), outReconYuv->getLumaAddr(), outReconYuv->getStride());
        part = PartitionFromSizes(width >> 1, height >> 1);
        distortion += m_rdCost->scaleChromaDistCb(primitives.sse_pp[part](fencYuv->getCbAddr(), fencYuv->getCStride(), outReconYuv->getCbAddr(), outReconYuv->getCStride()));
        distortion += m_rdCost->scaleChromaDistCr(primitives.sse_pp[part](fencYuv->getCrAddr(), fencYuv->getCStride(), outReconYuv->getCrAddr(), outReconYuv->getCStride()));

        m_rdGoOnSbacCoder->load(m_rdSbacCoders[cu->getDepth(0)][CI_CURR_BEST]);
        m_entropyCoder->resetBits();
        if (cu->getSlice()->getPPS()->getTransquantBypassEnableFlag())
        {
            m_entropyCoder->encodeCUTransquantBypassFlag(cu, 0, true);
        }
        m_entropyCoder->encodeSkipFlag(cu, 0, true);
        m_entropyCoder->encodeMergeIndex(cu, 0, true);

        bits = m_entropyCoder->getNumberOfWrittenBits();
        m_rdGoOnSbacCoder->store(m_rdSbacCoders[cu->getDepth(0)][CI_TEMP_BEST]);

        cu->m_totalBits       = bits;
        cu->m_totalDistortion = distortion;
        cu->m_totalCost       = m_rdCost->calcRdCost(distortion, bits);

        m_rdGoOnSbacCoder->store(m_rdSbacCoders[cu->getDepth(0)][CI_TEMP_BEST]);

        cu->setCbfSubParts(0, 0, 0, 0, cu->getDepth(0));
        cu->setTrIdxSubParts(0, 0, cu->getDepth(0));
        return;
    }

    //  Residual coding.
    int     qp, qpBest = 0;
    UInt64  cost, bcost = MAX_INT64;

    UInt trLevel = 0;
    if ((cu->getWidth(0) > cu->getSlice()->getSPS()->getMaxTrSize()))
    {
        while (cu->getWidth(0) > (cu->getSlice()->getSPS()->getMaxTrSize() << trLevel))
        {
            trLevel++;
        }
    }
    UInt maxTrLevel = 1 + trLevel;

    while ((width >> maxTrLevel) < (g_maxCUWidth >> g_maxCUDepth))
    {
        maxTrLevel--;
    }

    qp = bHighPass ? Clip3(-cu->getSlice()->getSPS()->getQpBDOffsetY(), MAX_QP, (int)cu->getQP(0)) : cu->getQP(0);

    outResiYuv->subtract(fencYuv, predYuv, 0, width);

    cost = 0;
    bits = 0;
    distortion = 0;
    m_rdGoOnSbacCoder->load(m_rdSbacCoders[cu->getDepth(0)][CI_CURR_BEST]);

    UInt zeroDistortion = 0;
    xEstimateResidualQT(cu, 0, 0, outResiYuv, cu->getDepth(0), cost, bits, distortion, &zeroDistortion);

    m_entropyCoder->resetBits();
    m_entropyCoder->encodeQtRootCbfZero(cu);
    UInt zeroResiBits = m_entropyCoder->getNumberOfWrittenBits();

    UInt64 zeroCost = m_rdCost->calcRdCost(zeroDistortion, zeroResiBits);
    if (cu->isLosslessCoded(0))
    {
        zeroCost = cost + 1;
    }
    if (zeroCost < cost)
    {
        cost       = zeroCost;
        bits       = 0;
        distortion = zeroDistortion;

        const UInt qpartnum = cu->getPic()->getNumPartInCU() >> (cu->getDepth(0) << 1);
        ::memset(cu->getTransformIdx(), 0, qpartnum * sizeof(UChar));
        ::memset(cu->getCbf(TEXT_LUMA), 0, qpartnum * sizeof(UChar));
        ::memset(cu->getCbf(TEXT_CHROMA_U), 0, qpartnum * sizeof(UChar));
        ::memset(cu->getCbf(TEXT_CHROMA_V), 0, qpartnum * sizeof(UChar));
        ::memset(cu->getCoeffY(), 0, width * height * sizeof(TCoeff));
        ::memset(cu->getCoeffCb(), 0, width * height * sizeof(TCoeff) >> 2);
        ::memset(cu->getCoeffCr(), 0, width * height * sizeof(TCoeff) >> 2);
        cu->setTransformSkipSubParts(0, 0, 0, 0, cu->getDepth(0));
    }
    else
    {
        xSetResidualQTData(cu, 0, 0, NULL, cu->getDepth(0), false);
    }

    m_rdGoOnSbacCoder->load(m_rdSbacCoders[cu->getDepth(0)][CI_CURR_BEST]);

    bits = xSymbolBitsInter(cu);

    UInt64 exactCost = m_rdCost->calcRdCost(distortion, bits);
    cost = exactCost;

    if (cost < bcost)
    {
        if (!cu->getQtRootCbf(0))
        {
            outBestResiYuv->clear();
        }
        else
        {
            xSetResidualQTData(cu, 0, 0, outBestResiYuv, cu->getDepth(0), true);
        }

        bestBits = bits;
        bdist    = distortion;
        bcost    = cost;
        qpBest   = qp;
        m_rdGoOnSbacCoder->store(m_rdSbacCoders[cu->getDepth(0)][CI_TEMP_BEST]);
    }

    assert(bcost != MAX_INT64);

    outReconYuv->addClip(predYuv, outBestResiYuv, 0, width);

    // update with clipped distortion and cost (qp estimation loop uses unclipped values)
    int part = PartitionFromSizes(width, height);
    bdist = primitives.sse_pp[part](fencYuv->getLumaAddr(), fencYuv->getStride(), outReconYuv->getLumaAddr(), outReconYuv->getStride());
    part = PartitionFromSizes(width >> 1, height >> 1);
    bdist += m_rdCost->scaleChromaDistCb(primitives.sse_pp[part](fencYuv->getCbAddr(), fencYuv->getCStride(), outReconYuv->getCbAddr(), outReconYuv->getCStride()));
    bdist += m_rdCost->scaleChromaDistCr(primitives.sse_pp[part](fencYuv->getCrAddr(), fencYuv->getCStride(), outReconYuv->getCrAddr(), outReconYuv->getCStride()));
    bcost = m_rdCost->calcRdCost(bdist, bestBits);

    cu->m_totalBits       = bestBits;
    cu->m_totalDistortion = bdist;
    cu->m_totalCost       = bcost;

    if (cu->isSkipped(0))
    {
        cu->setCbfSubParts(0, 0, 0, 0, cu->getDepth(0));
    }

    cu->setQPSubParts(qpBest, 0, cu->getDepth(0));
}

#if _MSC_VER
#pragma warning(disable: 4701) // potentially uninitialized local variable
#endif

void TEncSearch::xEstimateResidualQT(TComDataCU* cu,
                                     UInt        absPartIdx,
                                     UInt        absTUPartIdx,
                                     TShortYUV*  resiYuv,
                                     const UInt  depth,
                                     UInt64 &    rdCost,
                                     UInt &      outBits,
                                     UInt &      outDist,
                                     UInt *      outZeroDist)
{
    assert(cu->getDepth(0) == cu->getDepth(absPartIdx));
    const UInt trMode = depth - cu->getDepth(0);
    const UInt trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> depth] + 2;

    bool bSplitFlag = ((cu->getSlice()->getSPS()->getQuadtreeTUMaxDepthInter() == 1) && cu->getPredictionMode(absPartIdx) == MODE_INTER && (cu->getPartitionSize(absPartIdx) != SIZE_2Nx2N));
    bool bCheckFull;
    if (bSplitFlag && depth == cu->getDepth(absPartIdx) && (trSizeLog2 > cu->getQuadtreeTULog2MinSizeInCU(absPartIdx)))
        bCheckFull = false;
    else
        bCheckFull = (trSizeLog2 <= cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize());
    const bool bCheckSplit = (trSizeLog2 > cu->getQuadtreeTULog2MinSizeInCU(absPartIdx));
    assert(bCheckFull || bCheckSplit);

    bool  bCodeChroma = true;
    UInt  trModeC     = trMode;
    UInt  trSizeCLog2 = trSizeLog2 - 1;
    if (trSizeLog2 == 2)
    {
        trSizeCLog2++;
        trModeC--;
        UInt qpdiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trModeC) << 1);
        bCodeChroma = ((absPartIdx % qpdiv) == 0);
    }

    const UInt setCbf = 1 << trMode;
    // code full block
    UInt64 singleCost = MAX_INT64;
    UInt singleBits = 0;
    UInt singleDist = 0;
    UInt absSumY = 0, absSumU = 0, absSumV = 0;
    int lastPosY = -1, lastPosU = -1, lastPosV = -1;
    UInt bestTransformMode[3] = { 0 };

    m_rdGoOnSbacCoder->store(m_rdSbacCoders[depth][CI_QT_TRAFO_ROOT]);

    if (bCheckFull)
    {
        const UInt numCoeffPerAbsPartIdxIncrement = cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight() >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1);
        const UInt qtlayer = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;
        TCoeff *coeffCurY = m_qtTempCoeffY[qtlayer] + (numCoeffPerAbsPartIdxIncrement * absPartIdx);
        TCoeff *coeffCurU = m_qtTempCoeffCb[qtlayer] + (numCoeffPerAbsPartIdxIncrement * absPartIdx >> 2);
        TCoeff *coeffCurV = m_qtTempCoeffCr[qtlayer] + (numCoeffPerAbsPartIdxIncrement * absPartIdx >> 2);

        int trWidth = 0, trHeight = 0, trWidthC = 0, trHeightC = 0;
        UInt absTUPartIdxC = absPartIdx;

        trWidth  = trHeight  = 1 << trSizeLog2;
        trWidthC = trHeightC = 1 << trSizeCLog2;
        cu->setTrIdxSubParts(depth - cu->getDepth(0), absPartIdx, depth);
        UInt64 minCostY = MAX_INT64;
        UInt64 minCostU = MAX_INT64;
        UInt64 minCostV = MAX_INT64;
        bool checkTransformSkipY  = cu->getSlice()->getPPS()->getUseTransformSkip() && trWidth == 4 && trHeight == 4;
        bool checkTransformSkipUV = cu->getSlice()->getPPS()->getUseTransformSkip() && trWidthC == 4 && trHeightC == 4;

        checkTransformSkipY         &= (!cu->isLosslessCoded(0));
        checkTransformSkipUV        &= (!cu->isLosslessCoded(0));

        cu->setTransformSkipSubParts(0, TEXT_LUMA, absPartIdx, depth);
        if (bCodeChroma)
        {
            cu->setTransformSkipSubParts(0, TEXT_CHROMA_U, absPartIdx, cu->getDepth(0) + trModeC);
            cu->setTransformSkipSubParts(0, TEXT_CHROMA_V, absPartIdx, cu->getDepth(0) + trModeC);
        }

        if (m_cfg->param.bEnableRDOQ)
        {
            m_entropyCoder->estimateBit(m_trQuant->m_estBitsSbac, trWidth, trHeight, TEXT_LUMA);
        }

        m_trQuant->setQPforQuant(cu->getQP(0), TEXT_LUMA, cu->getSlice()->getSPS()->getQpBDOffsetY(), 0);
        m_trQuant->selectLambda(TEXT_LUMA);

        absSumY = m_trQuant->transformNxN(cu, resiYuv->getLumaAddr(absTUPartIdx), resiYuv->m_width, coeffCurY,
                                          trWidth, trHeight, TEXT_LUMA, absPartIdx, &lastPosY);

        cu->setCbfSubParts(absSumY ? setCbf : 0, TEXT_LUMA, absPartIdx, depth);

        if (bCodeChroma)
        {
            if (m_cfg->param.bEnableRDOQ)
            {
                m_entropyCoder->estimateBit(m_trQuant->m_estBitsSbac, trWidthC, trHeightC, TEXT_CHROMA);
            }

            int curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCbQpOffset() + cu->getSlice()->getSliceQpDeltaCb();
            m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset);

            m_trQuant->selectLambda(TEXT_CHROMA);

            absSumU = m_trQuant->transformNxN(cu, resiYuv->getCbAddr(absTUPartIdxC), resiYuv->m_cwidth, coeffCurU,
                                              trWidthC, trHeightC, TEXT_CHROMA_U, absPartIdx, &lastPosU);

            curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCrQpOffset() + cu->getSlice()->getSliceQpDeltaCr();
            m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset);
            absSumV = m_trQuant->transformNxN(cu, resiYuv->getCrAddr(absTUPartIdxC), resiYuv->m_cwidth, coeffCurV,
                                              trWidthC, trHeightC, TEXT_CHROMA_V, absPartIdx, &lastPosV);

            cu->setCbfSubParts(absSumU ? setCbf : 0, TEXT_CHROMA_U, absPartIdx, cu->getDepth(0) + trModeC);
            cu->setCbfSubParts(absSumV ? setCbf : 0, TEXT_CHROMA_V, absPartIdx, cu->getDepth(0) + trModeC);
        }

        m_entropyCoder->resetBits();
        m_entropyCoder->encodeQtCbf(cu, absPartIdx, TEXT_LUMA, trMode);
        m_entropyCoder->encodeCoeffNxN(cu, coeffCurY, absPartIdx,  trWidth,  trHeight, depth, TEXT_LUMA);
        const UInt uiSingleBitsY = m_entropyCoder->getNumberOfWrittenBits();

        UInt singleBitsU = 0;
        UInt singleBitsV = 0;
        if (bCodeChroma)
        {
            m_entropyCoder->encodeQtCbf(cu, absPartIdx, TEXT_CHROMA_U, trMode);
            m_entropyCoder->encodeCoeffNxN(cu, coeffCurU, absPartIdx, trWidthC, trHeightC, depth, TEXT_CHROMA_U);
            singleBitsU = m_entropyCoder->getNumberOfWrittenBits() - uiSingleBitsY;

            m_entropyCoder->encodeQtCbf(cu, absPartIdx, TEXT_CHROMA_V, trMode);
            m_entropyCoder->encodeCoeffNxN(cu, coeffCurV, absPartIdx, trWidthC, trHeightC, depth, TEXT_CHROMA_V);
            singleBitsV = m_entropyCoder->getNumberOfWrittenBits() - (uiSingleBitsY + singleBitsU);
        }

        const UInt numSamplesLuma = 1 << (trSizeLog2 << 1);
        const UInt numSamplesChroma = 1 << (trSizeCLog2 << 1);

        ::memset(m_tempPel, 0, sizeof(Pel) * numSamplesLuma); // not necessary needed for inside of recursion (only at the beginning)

        int partSize = PartitionFromSizes(trWidth, trHeight);
        UInt distY = primitives.sse_sp[partSize](resiYuv->getLumaAddr(absTUPartIdx), resiYuv->m_width, m_tempPel, trWidth);

        if (outZeroDist)
        {
            *outZeroDist += distY;
        }
        if (absSumY)
        {
            short *curResiY = m_qtTempTComYuv[qtlayer].getLumaAddr(absTUPartIdx);

            m_trQuant->setQPforQuant(cu->getQP(0), TEXT_LUMA, cu->getSlice()->getSPS()->getQpBDOffsetY(), 0);

            int scalingListType = 3 + g_eTTable[(int)TEXT_LUMA];
            assert(scalingListType < 6);
            m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdx), REG_DCT, curResiY, m_qtTempTComYuv[qtlayer].m_width,  coeffCurY, trWidth, trHeight, scalingListType, false, lastPosY); //this is for inter mode only

            const UInt nonZeroDistY = primitives.sse_ss[partSize](resiYuv->getLumaAddr(absTUPartIdx), resiYuv->m_width, m_qtTempTComYuv[qtlayer].getLumaAddr(absTUPartIdx),
                                                                  m_qtTempTComYuv[qtlayer].m_width);
            if (cu->isLosslessCoded(0))
            {
                distY = nonZeroDistY;
            }
            else
            {
                const UInt64 singleCostY = m_rdCost->calcRdCost(nonZeroDistY, uiSingleBitsY);
                m_entropyCoder->resetBits();
                m_entropyCoder->encodeQtCbfZero(cu, TEXT_LUMA,     trMode);
                const UInt nullBitsY = m_entropyCoder->getNumberOfWrittenBits();
                const UInt64 nullCostY = m_rdCost->calcRdCost(distY, nullBitsY);
                if (nullCostY < singleCostY)
                {
                    absSumY = 0;
                    ::memset(coeffCurY, 0, sizeof(TCoeff) * numSamplesLuma);
                    if (checkTransformSkipY)
                    {
                        minCostY = nullCostY;
                    }
                }
                else
                {
                    distY = nonZeroDistY;
                    if (checkTransformSkipY)
                    {
                        minCostY = singleCostY;
                    }
                }
            }
        }
        else if (checkTransformSkipY)
        {
            m_entropyCoder->resetBits();
            m_entropyCoder->encodeQtCbfZero(cu, TEXT_LUMA, trMode);
            const UInt nullBitsY = m_entropyCoder->getNumberOfWrittenBits();
            minCostY = m_rdCost->calcRdCost(distY, nullBitsY);
        }

        if (!absSumY)
        {
            short *ptr =  m_qtTempTComYuv[qtlayer].getLumaAddr(absTUPartIdx);
            const UInt stride = m_qtTempTComYuv[qtlayer].m_width;

            assert(trWidth == trHeight);
            primitives.blockfil_s[(int)g_convertToBit[trWidth]](ptr, stride, 0);
        }

        UInt distU = 0;
        UInt distV = 0;

        int partSizeC = PartitionFromSizes(trWidthC, trHeightC);
        if (bCodeChroma)
        {
            distU = m_rdCost->scaleChromaDistCb(primitives.sse_sp[partSizeC](resiYuv->getCbAddr(absTUPartIdxC), resiYuv->m_cwidth, m_tempPel, trWidthC));

            if (outZeroDist)
            {
                *outZeroDist += distU;
            }
            if (absSumU)
            {
                short *pcResiCurrU = m_qtTempTComYuv[qtlayer].getCbAddr(absTUPartIdxC);

                int curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCbQpOffset() + cu->getSlice()->getSliceQpDeltaCb();
                m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset);

                int scalingListType = 3 + g_eTTable[(int)TEXT_CHROMA_U];
                assert(scalingListType < 6);
                m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdx), REG_DCT, pcResiCurrU, m_qtTempTComYuv[qtlayer].m_cwidth, coeffCurU, trWidthC, trHeightC, scalingListType, false, lastPosU);

                UInt dist = primitives.sse_ss[partSizeC](resiYuv->getCbAddr(absTUPartIdxC), resiYuv->m_cwidth,
                                                         m_qtTempTComYuv[qtlayer].getCbAddr(absTUPartIdxC),
                                                         m_qtTempTComYuv[qtlayer].m_cwidth);
                const UInt nonZeroDistU = m_rdCost->scaleChromaDistCb(dist);

                if (cu->isLosslessCoded(0))
                {
                    distU = nonZeroDistU;
                }
                else
                {
                    const UInt64 singleCostU = m_rdCost->calcRdCost(nonZeroDistU, singleBitsU);
                    m_entropyCoder->resetBits();
                    m_entropyCoder->encodeQtCbfZero(cu, TEXT_CHROMA_U, trMode);
                    const UInt nullBitsU = m_entropyCoder->getNumberOfWrittenBits();
                    const UInt64 nullCostU = m_rdCost->calcRdCost(distU, nullBitsU);
                    if (nullCostU < singleCostU)
                    {
                        absSumU = 0;
                        ::memset(coeffCurU, 0, sizeof(TCoeff) * numSamplesChroma);
                        if (checkTransformSkipUV)
                        {
                            minCostU = nullCostU;
                        }
                    }
                    else
                    {
                        distU = nonZeroDistU;
                        if (checkTransformSkipUV)
                        {
                            minCostU = singleCostU;
                        }
                    }
                }
            }
            else if (checkTransformSkipUV)
            {
                m_entropyCoder->resetBits();
                m_entropyCoder->encodeQtCbfZero(cu, TEXT_CHROMA_U, trModeC);
                const UInt nullBitsU = m_entropyCoder->getNumberOfWrittenBits();
                minCostU = m_rdCost->calcRdCost(distU, nullBitsU);
            }
            if (!absSumU)
            {
                short *ptr = m_qtTempTComYuv[qtlayer].getCbAddr(absTUPartIdxC);
                const UInt stride = m_qtTempTComYuv[qtlayer].m_cwidth;

                assert(trWidthC == trHeightC);
                primitives.blockfil_s[(int)g_convertToBit[trWidthC]](ptr, stride, 0);
            }

            distV = m_rdCost->scaleChromaDistCr(primitives.sse_sp[partSizeC](resiYuv->getCrAddr(absTUPartIdxC), resiYuv->m_cwidth, m_tempPel, trWidthC));
            if (outZeroDist)
            {
                *outZeroDist += distV;
            }
            if (absSumV)
            {
                short *curResiV = m_qtTempTComYuv[qtlayer].getCrAddr(absTUPartIdxC);
                int curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCrQpOffset() + cu->getSlice()->getSliceQpDeltaCr();
                m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset);

                int scalingListType = 3 + g_eTTable[(int)TEXT_CHROMA_V];
                assert(scalingListType < 6);
                m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdx), REG_DCT, curResiV, m_qtTempTComYuv[qtlayer].m_cwidth, coeffCurV, trWidthC, trHeightC, scalingListType, false, lastPosV);

                UInt dist = primitives.sse_ss[partSizeC](resiYuv->getCrAddr(absTUPartIdxC), resiYuv->m_cwidth,
                                                         m_qtTempTComYuv[qtlayer].getCrAddr(absTUPartIdxC),
                                                         m_qtTempTComYuv[qtlayer].m_cwidth);
                const UInt nonZeroDistV = m_rdCost->scaleChromaDistCr(dist);

                if (cu->isLosslessCoded(0))
                {
                    distV = nonZeroDistV;
                }
                else
                {
                    const UInt64 singleCostV = m_rdCost->calcRdCost(nonZeroDistV, singleBitsV);
                    m_entropyCoder->resetBits();
                    m_entropyCoder->encodeQtCbfZero(cu, TEXT_CHROMA_V, trMode);
                    const UInt nullBitsV = m_entropyCoder->getNumberOfWrittenBits();
                    const UInt64 nullCostV = m_rdCost->calcRdCost(distV, nullBitsV);
                    if (nullCostV < singleCostV)
                    {
                        absSumV = 0;
                        ::memset(coeffCurV, 0, sizeof(TCoeff) * numSamplesChroma);
                        if (checkTransformSkipUV)
                        {
                            minCostV = nullCostV;
                        }
                    }
                    else
                    {
                        distV = nonZeroDistV;
                        if (checkTransformSkipUV)
                        {
                            minCostV = singleCostV;
                        }
                    }
                }
            }
            else if (checkTransformSkipUV)
            {
                m_entropyCoder->resetBits();
                m_entropyCoder->encodeQtCbfZero(cu, TEXT_CHROMA_V, trModeC);
                const UInt nullBitsV = m_entropyCoder->getNumberOfWrittenBits();
                minCostV = m_rdCost->calcRdCost(distV, nullBitsV);
            }
            if (!absSumV)
            {
                short *ptr =  m_qtTempTComYuv[qtlayer].getCrAddr(absTUPartIdxC);
                const UInt stride = m_qtTempTComYuv[qtlayer].m_cwidth;

                assert(trWidthC == trHeightC);
                primitives.blockfil_s[(int)g_convertToBit[trWidthC]](ptr, stride, 0);
            }
        }
        cu->setCbfSubParts(absSumY ? setCbf : 0, TEXT_LUMA, absPartIdx, depth);
        if (bCodeChroma)
        {
            cu->setCbfSubParts(absSumU ? setCbf : 0, TEXT_CHROMA_U, absPartIdx, cu->getDepth(0) + trModeC);
            cu->setCbfSubParts(absSumV ? setCbf : 0, TEXT_CHROMA_V, absPartIdx, cu->getDepth(0) + trModeC);
        }

        if (checkTransformSkipY)
        {
            UInt nonZeroDistY = 0, absSumTransformSkipY;
            int lastPosTransformSkipY = -1;
            UInt64 singleCostY = MAX_INT64;

            short *curResiY = m_qtTempTComYuv[qtlayer].getLumaAddr(absTUPartIdx);
            UInt resiStride = m_qtTempTComYuv[qtlayer].m_width;

            TCoeff bestCoeffY[32 * 32];
            memcpy(bestCoeffY, coeffCurY, sizeof(TCoeff) * numSamplesLuma);

            short bestResiY[32 * 32];
            for (int i = 0; i < trHeight; ++i)
            {
                memcpy(bestResiY + i * trWidth, curResiY + i * resiStride, sizeof(short) * trWidth);
            }

            m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_QT_TRAFO_ROOT]);

            cu->setTransformSkipSubParts(1, TEXT_LUMA, absPartIdx, depth);

            if (m_cfg->param.bEnableRDOQTS)
            {
                m_entropyCoder->estimateBit(m_trQuant->m_estBitsSbac, trWidth, trHeight, TEXT_LUMA);
            }

            m_trQuant->setQPforQuant(cu->getQP(0), TEXT_LUMA, cu->getSlice()->getSPS()->getQpBDOffsetY(), 0);

            m_trQuant->selectLambda(TEXT_LUMA);
            absSumTransformSkipY = m_trQuant->transformNxN(cu, resiYuv->getLumaAddr(absTUPartIdx), resiYuv->m_width, coeffCurY,
                                                           trWidth, trHeight, TEXT_LUMA, absPartIdx, &lastPosTransformSkipY, true);
            cu->setCbfSubParts(absSumTransformSkipY ? setCbf : 0, TEXT_LUMA, absPartIdx, depth);

            if (absSumTransformSkipY != 0)
            {
                m_entropyCoder->resetBits();
                m_entropyCoder->encodeQtCbf(cu, absPartIdx, TEXT_LUMA, trMode);
                m_entropyCoder->encodeCoeffNxN(cu, coeffCurY, absPartIdx, trWidth, trHeight, depth, TEXT_LUMA);
                const UInt skipSingleBitsY = m_entropyCoder->getNumberOfWrittenBits();

                m_trQuant->setQPforQuant(cu->getQP(0), TEXT_LUMA, cu->getSlice()->getSPS()->getQpBDOffsetY(), 0);

                int scalingListType = 3 + g_eTTable[(int)TEXT_LUMA];
                assert(scalingListType < 6);

                m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdx), REG_DCT, curResiY, m_qtTempTComYuv[qtlayer].m_width,  coeffCurY, trWidth, trHeight, scalingListType, true, lastPosTransformSkipY);

                nonZeroDistY = primitives.sse_ss[partSize](resiYuv->getLumaAddr(absTUPartIdx), resiYuv->m_width,
                                                           m_qtTempTComYuv[qtlayer].getLumaAddr(absTUPartIdx),
                                                           m_qtTempTComYuv[qtlayer].m_width);

                singleCostY = m_rdCost->calcRdCost(nonZeroDistY, skipSingleBitsY);
            }

            if (!absSumTransformSkipY || minCostY < singleCostY)
            {
                cu->setTransformSkipSubParts(0, TEXT_LUMA, absPartIdx, depth);
                memcpy(coeffCurY, bestCoeffY, sizeof(TCoeff) * numSamplesLuma);
                for (int i = 0; i < trHeight; ++i)
                {
                    memcpy(curResiY + i * resiStride, &bestResiY[i * trWidth], sizeof(short) * trWidth);
                }
            }
            else
            {
                distY = nonZeroDistY;
                absSumY = absSumTransformSkipY;
                bestTransformMode[0] = 1;
            }

            cu->setCbfSubParts(absSumY ? setCbf : 0, TEXT_LUMA, absPartIdx, depth);
        }

        if (bCodeChroma && checkTransformSkipUV)
        {
            UInt nonZeroDistU = 0, nonZeroDistV = 0, absSumTransformSkipU, absSumTransformSkipV;
            int lastPosTransformSkipU = -1, lastPosTransformSkipV = -1;
            UInt64 singleCostU = MAX_INT64;
            UInt64 singleCostV = MAX_INT64;

            short *curResiU = m_qtTempTComYuv[qtlayer].getCbAddr(absTUPartIdxC);
            short *curResiV = m_qtTempTComYuv[qtlayer].getCrAddr(absTUPartIdxC);
            UInt stride = m_qtTempTComYuv[qtlayer].m_cwidth;

            TCoeff bestCoeffU[32 * 32], bestCoeffV[32 * 32];
            memcpy(bestCoeffU, coeffCurU, sizeof(TCoeff) * numSamplesChroma);
            memcpy(bestCoeffV, coeffCurV, sizeof(TCoeff) * numSamplesChroma);

            short bestResiU[32 * 32], bestResiV[32 * 32];
            for (int i = 0; i < trHeightC; ++i)
            {
                memcpy(&bestResiU[i * trWidthC], curResiU + i * stride, sizeof(short) * trWidthC);
                memcpy(&bestResiV[i * trWidthC], curResiV + i * stride, sizeof(short) * trWidthC);
            }

            m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_QT_TRAFO_ROOT]);

            cu->setTransformSkipSubParts(1, TEXT_CHROMA_U, absPartIdx, cu->getDepth(0) + trModeC);
            cu->setTransformSkipSubParts(1, TEXT_CHROMA_V, absPartIdx, cu->getDepth(0) + trModeC);

            if (m_cfg->param.bEnableRDOQTS)
            {
                m_entropyCoder->estimateBit(m_trQuant->m_estBitsSbac, trWidthC, trHeightC, TEXT_CHROMA);
            }

            int curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCbQpOffset() + cu->getSlice()->getSliceQpDeltaCb();
            m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset);
            m_trQuant->selectLambda(TEXT_CHROMA);

            absSumTransformSkipU = m_trQuant->transformNxN(cu, resiYuv->getCbAddr(absTUPartIdxC), resiYuv->m_cwidth, coeffCurU,
                                                           trWidthC, trHeightC, TEXT_CHROMA_U, absPartIdx, &lastPosTransformSkipU, true);
            curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCrQpOffset() + cu->getSlice()->getSliceQpDeltaCr();
            m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset);
            absSumTransformSkipV = m_trQuant->transformNxN(cu, resiYuv->getCrAddr(absTUPartIdxC), resiYuv->m_cwidth, coeffCurV,
                                                           trWidthC, trHeightC, TEXT_CHROMA_V, absPartIdx, &lastPosTransformSkipV, true);

            cu->setCbfSubParts(absSumTransformSkipU ? setCbf : 0, TEXT_CHROMA_U, absPartIdx, cu->getDepth(0) + trModeC);
            cu->setCbfSubParts(absSumTransformSkipV ? setCbf : 0, TEXT_CHROMA_V, absPartIdx, cu->getDepth(0) + trModeC);

            m_entropyCoder->resetBits();
            singleBitsU = 0;
            singleBitsV = 0;

            if (absSumTransformSkipU)
            {
                m_entropyCoder->encodeQtCbf(cu, absPartIdx, TEXT_CHROMA_U, trMode);
                m_entropyCoder->encodeCoeffNxN(cu, coeffCurU, absPartIdx, trWidthC, trHeightC, depth, TEXT_CHROMA_U);
                singleBitsU = m_entropyCoder->getNumberOfWrittenBits();

                curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCbQpOffset() + cu->getSlice()->getSliceQpDeltaCb();
                m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset);

                int scalingListType = 3 + g_eTTable[(int)TEXT_CHROMA_U];
                assert(scalingListType < 6);

                m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdx), REG_DCT, curResiU, m_qtTempTComYuv[qtlayer].m_cwidth, coeffCurU, trWidthC, trHeightC, scalingListType, true, lastPosTransformSkipU);

                UInt dist = primitives.sse_ss[partSizeC](resiYuv->getCbAddr(absTUPartIdxC), resiYuv->m_cwidth,
                                                         m_qtTempTComYuv[qtlayer].getCbAddr(absTUPartIdxC),
                                                         m_qtTempTComYuv[qtlayer].m_cwidth);
                nonZeroDistU = m_rdCost->scaleChromaDistCb(dist);
                singleCostU = m_rdCost->calcRdCost(nonZeroDistU, singleBitsU);
            }

            if (!absSumTransformSkipU || minCostU < singleCostU)
            {
                cu->setTransformSkipSubParts(0, TEXT_CHROMA_U, absPartIdx, cu->getDepth(0) + trModeC);

                memcpy(coeffCurU, bestCoeffU, sizeof(TCoeff) * numSamplesChroma);
                for (int i = 0; i < trHeightC; ++i)
                {
                    memcpy(curResiU + i * stride, &bestResiU[i * trWidthC], sizeof(short) * trWidthC);
                }
            }
            else
            {
                distU = nonZeroDistU;
                absSumU = absSumTransformSkipU;
                bestTransformMode[1] = 1;
            }

            if (absSumTransformSkipV)
            {
                m_entropyCoder->encodeQtCbf(cu, absPartIdx, TEXT_CHROMA_V, trMode);
                m_entropyCoder->encodeCoeffNxN(cu, coeffCurV, absPartIdx, trWidthC, trHeightC, depth, TEXT_CHROMA_V);
                singleBitsV = m_entropyCoder->getNumberOfWrittenBits() - singleBitsU;

                curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCrQpOffset() + cu->getSlice()->getSliceQpDeltaCr();
                m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset);

                int scalingListType = 3 + g_eTTable[(int)TEXT_CHROMA_V];
                assert(scalingListType < 6);

                m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdx), REG_DCT, curResiV, m_qtTempTComYuv[qtlayer].m_cwidth, coeffCurV, trWidthC, trHeightC, scalingListType, true, lastPosTransformSkipV);

                UInt dist = primitives.sse_ss[partSizeC](resiYuv->getCrAddr(absTUPartIdxC), resiYuv->m_cwidth,
                                                         m_qtTempTComYuv[qtlayer].getCrAddr(absTUPartIdxC),
                                                         m_qtTempTComYuv[qtlayer].m_cwidth);
                nonZeroDistV = m_rdCost->scaleChromaDistCr(dist);
                singleCostV = m_rdCost->calcRdCost(nonZeroDistV, singleBitsV);
            }

            if (!absSumTransformSkipV || minCostV < singleCostV)
            {
                cu->setTransformSkipSubParts(0, TEXT_CHROMA_V, absPartIdx, cu->getDepth(0) + trModeC);

                memcpy(coeffCurV, bestCoeffV, sizeof(TCoeff) * numSamplesChroma);
                for (int i = 0; i < trHeightC; ++i)
                {
                    memcpy(curResiV + i * stride, &bestResiV[i * trWidthC], sizeof(short) * trWidthC);
                }
            }
            else
            {
                distV = nonZeroDistV;
                absSumV = absSumTransformSkipV;
                bestTransformMode[2] = 1;
            }

            cu->setCbfSubParts(absSumU ? setCbf : 0, TEXT_CHROMA_U, absPartIdx, cu->getDepth(0) + trModeC);
            cu->setCbfSubParts(absSumV ? setCbf : 0, TEXT_CHROMA_V, absPartIdx, cu->getDepth(0) + trModeC);
        }

        m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_QT_TRAFO_ROOT]);

        m_entropyCoder->resetBits();

        if (trSizeLog2 > cu->getQuadtreeTULog2MinSizeInCU(absPartIdx))
        {
            m_entropyCoder->encodeTransformSubdivFlag(0, 5 - trSizeLog2);
        }

        if (bCodeChroma)
        {
            m_entropyCoder->encodeQtCbf(cu, absPartIdx, TEXT_CHROMA_U, trMode);
            m_entropyCoder->encodeQtCbf(cu, absPartIdx, TEXT_CHROMA_V, trMode);
        }

        m_entropyCoder->encodeQtCbf(cu, absPartIdx, TEXT_LUMA,     trMode);
        m_entropyCoder->encodeCoeffNxN(cu, coeffCurY, absPartIdx, trWidth, trHeight, depth, TEXT_LUMA);

        if (bCodeChroma)
        {
            m_entropyCoder->encodeCoeffNxN(cu, coeffCurU, absPartIdx, trWidthC, trHeightC, depth, TEXT_CHROMA_U);
            m_entropyCoder->encodeCoeffNxN(cu, coeffCurV, absPartIdx, trWidthC, trHeightC, depth, TEXT_CHROMA_V);
        }

        singleBits = m_entropyCoder->getNumberOfWrittenBits();
        singleDist = distY + distU + distV;
        singleCost = m_rdCost->calcRdCost(singleDist, singleBits);
    }

    // code sub-blocks
    if (bCheckSplit)
    {
        if (bCheckFull)
        {
            m_rdGoOnSbacCoder->store(m_rdSbacCoders[depth][CI_QT_TRAFO_TEST]);
            m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_QT_TRAFO_ROOT]);
        }
        UInt subdivDist = 0;
        UInt subdivBits = 0;
        UInt64 subDivCost = 0;

        const UInt qPartNumSubdiv = cu->getPic()->getNumPartInCU() >> ((depth + 1) << 1);
        for (UInt i = 0; i < 4; ++i)
        {
            UInt nsAddr = absPartIdx + i * qPartNumSubdiv;
            xEstimateResidualQT(cu, absPartIdx + i * qPartNumSubdiv, nsAddr, resiYuv, depth + 1, subDivCost, subdivBits, subdivDist, bCheckFull ? NULL : outZeroDist);
        }

        UInt ycbf = 0;
        UInt ucbf = 0;
        UInt vcbf = 0;
        for (UInt i = 0; i < 4; ++i)
        {
            ycbf |= cu->getCbf(absPartIdx + i * qPartNumSubdiv, TEXT_LUMA,     trMode + 1);
            ucbf |= cu->getCbf(absPartIdx + i * qPartNumSubdiv, TEXT_CHROMA_U, trMode + 1);
            vcbf |= cu->getCbf(absPartIdx + i * qPartNumSubdiv, TEXT_CHROMA_V, trMode + 1);
        }

        for (UInt i = 0; i < 4 * qPartNumSubdiv; ++i)
        {
            cu->getCbf(TEXT_LUMA)[absPartIdx + i] |= ycbf << trMode;
            cu->getCbf(TEXT_CHROMA_U)[absPartIdx + i] |= ucbf << trMode;
            cu->getCbf(TEXT_CHROMA_V)[absPartIdx + i] |= vcbf << trMode;
        }

        m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_QT_TRAFO_ROOT]);
        m_entropyCoder->resetBits();

        xEncodeResidualQT(cu, absPartIdx, depth, true,  TEXT_LUMA);
        xEncodeResidualQT(cu, absPartIdx, depth, false, TEXT_LUMA);
        xEncodeResidualQT(cu, absPartIdx, depth, false, TEXT_CHROMA_U);
        xEncodeResidualQT(cu, absPartIdx, depth, false, TEXT_CHROMA_V);

        subdivBits = m_entropyCoder->getNumberOfWrittenBits();
        subDivCost  = m_rdCost->calcRdCost(subdivDist, subdivBits);

        if (ycbf || ucbf || vcbf || !bCheckFull)
        {
            if (subDivCost < singleCost)
            {
                rdCost += subDivCost;
                outBits += subdivBits;
                outDist += subdivDist;
                return;
            }
        }

        cu->setTransformSkipSubParts(bestTransformMode[0], TEXT_LUMA, absPartIdx, depth);
        if (bCodeChroma)
        {
            cu->setTransformSkipSubParts(bestTransformMode[1], TEXT_CHROMA_U, absPartIdx, cu->getDepth(0) + trModeC);
            cu->setTransformSkipSubParts(bestTransformMode[2], TEXT_CHROMA_V, absPartIdx, cu->getDepth(0) + trModeC);
        }
        assert(bCheckFull);
        m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_QT_TRAFO_TEST]);
    }

    rdCost += singleCost;
    outBits += singleBits;
    outDist += singleDist;

    cu->setTrIdxSubParts(trMode, absPartIdx, depth);
    cu->setCbfSubParts(absSumY ? setCbf : 0, TEXT_LUMA, absPartIdx, depth);

    if (bCodeChroma)
    {
        cu->setCbfSubParts(absSumU ? setCbf : 0, TEXT_CHROMA_U, absPartIdx, cu->getDepth(0) + trModeC);
        cu->setCbfSubParts(absSumV ? setCbf : 0, TEXT_CHROMA_V, absPartIdx, cu->getDepth(0) + trModeC);
    }
}

void TEncSearch::xEncodeResidualQT(TComDataCU* cu, UInt absPartIdx, const UInt depth, bool bSubdivAndCbf, TextType ttype)
{
    assert(cu->getDepth(0) == cu->getDepth(absPartIdx));
    const UInt curTrMode = depth - cu->getDepth(0);
    const UInt trMode = cu->getTransformIdx(absPartIdx);
    const bool bSubdiv = curTrMode != trMode;
    const UInt trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> depth] + 2;

    if (bSubdivAndCbf && trSizeLog2 <= cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() && trSizeLog2 > cu->getQuadtreeTULog2MinSizeInCU(absPartIdx))
    {
        m_entropyCoder->encodeTransformSubdivFlag(bSubdiv, 5 - trSizeLog2);
    }

    assert(cu->getPredictionMode(absPartIdx) != MODE_INTRA);
    if (bSubdivAndCbf)
    {
        const bool bFirstCbfOfCU = curTrMode == 0;
        if (bFirstCbfOfCU || trSizeLog2 > 2)
        {
            if (bFirstCbfOfCU || cu->getCbf(absPartIdx, TEXT_CHROMA_U, curTrMode - 1))
            {
                m_entropyCoder->encodeQtCbf(cu, absPartIdx, TEXT_CHROMA_U, curTrMode);
            }
            if (bFirstCbfOfCU || cu->getCbf(absPartIdx, TEXT_CHROMA_V, curTrMode - 1))
            {
                m_entropyCoder->encodeQtCbf(cu, absPartIdx, TEXT_CHROMA_V, curTrMode);
            }
        }
        else if (trSizeLog2 == 2)
        {
            assert(cu->getCbf(absPartIdx, TEXT_CHROMA_U, curTrMode) == cu->getCbf(absPartIdx, TEXT_CHROMA_U, curTrMode - 1));
            assert(cu->getCbf(absPartIdx, TEXT_CHROMA_V, curTrMode) == cu->getCbf(absPartIdx, TEXT_CHROMA_V, curTrMode - 1));
        }
    }

    if (!bSubdiv)
    {
        const UInt numCoeffPerAbsPartIdxIncrement = cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight() >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1);
        //assert( 16 == uiNumCoeffPerAbsPartIdxIncrement ); // check
        const UInt qtlayer = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;
        TCoeff *coeffCurY = m_qtTempCoeffY[qtlayer] +  numCoeffPerAbsPartIdxIncrement * absPartIdx;
        TCoeff *coeffCurU = m_qtTempCoeffCb[qtlayer] + (numCoeffPerAbsPartIdxIncrement * absPartIdx >> 2);
        TCoeff *coeffCurV = m_qtTempCoeffCr[qtlayer] + (numCoeffPerAbsPartIdxIncrement * absPartIdx >> 2);

        bool  bCodeChroma = true;
        UInt  trModeC     = trMode;
        UInt  trSizeCLog2 = trSizeLog2 - 1;
        if (trSizeLog2 == 2)
        {
            trSizeCLog2++;
            trModeC--;
            UInt qpdiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trModeC) << 1);
            bCodeChroma = ((absPartIdx % qpdiv) == 0);
        }

        if (bSubdivAndCbf)
        {
            m_entropyCoder->encodeQtCbf(cu, absPartIdx, TEXT_LUMA, trMode);
        }
        else
        {
            if (ttype == TEXT_LUMA && cu->getCbf(absPartIdx, TEXT_LUMA, trMode))
            {
                int trWidth  = 1 << trSizeLog2;
                int trHeight = 1 << trSizeLog2;
                m_entropyCoder->encodeCoeffNxN(cu, coeffCurY, absPartIdx, trWidth, trHeight, depth, TEXT_LUMA);
            }
            if (bCodeChroma)
            {
                int trWidth  = 1 << trSizeCLog2;
                int trHeight = 1 << trSizeCLog2;
                if (ttype == TEXT_CHROMA_U && cu->getCbf(absPartIdx, TEXT_CHROMA_U, trMode))
                {
                    m_entropyCoder->encodeCoeffNxN(cu, coeffCurU, absPartIdx, trWidth, trHeight, depth, TEXT_CHROMA_U);
                }
                if (ttype == TEXT_CHROMA_V && cu->getCbf(absPartIdx, TEXT_CHROMA_V, trMode))
                {
                    m_entropyCoder->encodeCoeffNxN(cu, coeffCurV, absPartIdx, trWidth, trHeight, depth, TEXT_CHROMA_V);
                }
            }
        }
    }
    else
    {
        if (bSubdivAndCbf || cu->getCbf(absPartIdx, ttype, curTrMode))
        {
            const UInt qpartNumSubdiv = cu->getPic()->getNumPartInCU() >> ((depth + 1) << 1);
            for (UInt i = 0; i < 4; ++i)
            {
                xEncodeResidualQT(cu, absPartIdx + i * qpartNumSubdiv, depth + 1, bSubdivAndCbf, ttype);
            }
        }
    }
}

void TEncSearch::xSetResidualQTData(TComDataCU* cu, UInt absPartIdx, UInt absTUPartIdx, TShortYUV* resiYuv, UInt depth, bool bSpatial)
{
    assert(cu->getDepth(0) == cu->getDepth(absPartIdx));
    const UInt curTrMode = depth - cu->getDepth(0);
    const UInt trMode = cu->getTransformIdx(absPartIdx);

    if (curTrMode == trMode)
    {
        const UInt trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> depth] + 2;
        const UInt qtlayer = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;

        bool  bCodeChroma   = true;
        UInt  trModeC     = trMode;
        UInt  trSizeCLog2 = trSizeLog2 - 1;
        if (trSizeLog2 == 2)
        {
            trSizeCLog2++;
            trModeC--;
            UInt qpdiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trModeC) << 1);
            bCodeChroma  = ((absPartIdx % qpdiv) == 0);
        }

        if (bSpatial)
        {
            int trWidth  = 1 << trSizeLog2;
            int trHeight = 1 << trSizeLog2;
            m_qtTempTComYuv[qtlayer].copyPartToPartLuma(resiYuv, absTUPartIdx, trWidth, trHeight);

            if (bCodeChroma)
            {
                m_qtTempTComYuv[qtlayer].copyPartToPartChroma(resiYuv, absPartIdx, 1 << trSizeCLog2, 1 << trSizeCLog2);
            }
        }
        else
        {
            UInt uiNumCoeffPerAbsPartIdxIncrement = cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight() >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1);
            UInt uiNumCoeffY = (1 << (trSizeLog2 << 1));
            TCoeff* pcCoeffSrcY = m_qtTempCoeffY[qtlayer] +  uiNumCoeffPerAbsPartIdxIncrement * absPartIdx;
            TCoeff* pcCoeffDstY = cu->getCoeffY() + uiNumCoeffPerAbsPartIdxIncrement * absPartIdx;
            ::memcpy(pcCoeffDstY, pcCoeffSrcY, sizeof(TCoeff) * uiNumCoeffY);
            if (bCodeChroma)
            {
                UInt    uiNumCoeffC = (1 << (trSizeCLog2 << 1));
                TCoeff* pcCoeffSrcU = m_qtTempCoeffCb[qtlayer] + (uiNumCoeffPerAbsPartIdxIncrement * absPartIdx >> 2);
                TCoeff* pcCoeffSrcV = m_qtTempCoeffCr[qtlayer] + (uiNumCoeffPerAbsPartIdxIncrement * absPartIdx >> 2);
                TCoeff* pcCoeffDstU = cu->getCoeffCb() + (uiNumCoeffPerAbsPartIdxIncrement * absPartIdx >> 2);
                TCoeff* pcCoeffDstV = cu->getCoeffCr() + (uiNumCoeffPerAbsPartIdxIncrement * absPartIdx >> 2);
                ::memcpy(pcCoeffDstU, pcCoeffSrcU, sizeof(TCoeff) * uiNumCoeffC);
                ::memcpy(pcCoeffDstV, pcCoeffSrcV, sizeof(TCoeff) * uiNumCoeffC);
            }
        }
    }
    else
    {
        const UInt qPartNumSubdiv = cu->getPic()->getNumPartInCU() >> ((depth + 1) << 1);
        for (UInt i = 0; i < 4; ++i)
        {
            UInt nsAddr = absPartIdx + i * qPartNumSubdiv;
            xSetResidualQTData(cu, absPartIdx + i * qPartNumSubdiv, nsAddr, resiYuv, depth + 1, bSpatial);
        }
    }
}

UInt TEncSearch::xModeBitsIntra(TComDataCU* cu, UInt mode, UInt partOffset, UInt depth, UInt initTrDepth)
{
    // Reload only contexts required for coding intra mode information
    m_rdGoOnSbacCoder->loadIntraDirModeLuma(m_rdSbacCoders[depth][CI_CURR_BEST]);

    cu->setLumaIntraDirSubParts(mode, partOffset, depth + initTrDepth);

    m_entropyCoder->resetBits();
    m_entropyCoder->encodeIntraDirModeLuma(cu, partOffset);

    return m_entropyCoder->getNumberOfWrittenBits();
}

UInt TEncSearch::xUpdateCandList(UInt mode, UInt64 cost, UInt fastCandNum, UInt* CandModeList, UInt64* CandCostList)
{
    UInt i;
    UInt shift = 0;

    while (shift < fastCandNum && cost < CandCostList[fastCandNum - 1 - shift])
    {
        shift++;
    }

    if (shift != 0)
    {
        for (i = 1; i < shift; i++)
        {
            CandModeList[fastCandNum - i] = CandModeList[fastCandNum - 1 - i];
            CandCostList[fastCandNum - i] = CandCostList[fastCandNum - 1 - i];
        }

        CandModeList[fastCandNum - shift] = mode;
        CandCostList[fastCandNum - shift] = cost;
        return 1;
    }

    return 0;
}

/** add inter-prediction syntax elements for a CU block
 * \param cu
 */
UInt TEncSearch::xSymbolBitsInter(TComDataCU* cu)
{
    if (cu->getMergeFlag(0) && cu->getPartitionSize(0) == SIZE_2Nx2N && !cu->getQtRootCbf(0))
    {
        cu->setSkipFlagSubParts(true, 0, cu->getDepth(0));

        m_entropyCoder->resetBits();
        if (cu->getSlice()->getPPS()->getTransquantBypassEnableFlag())
        {
            m_entropyCoder->encodeCUTransquantBypassFlag(cu, 0, true);
        }
        m_entropyCoder->encodeSkipFlag(cu, 0, true);
        m_entropyCoder->encodeMergeIndex(cu, 0, true);
        return m_entropyCoder->getNumberOfWrittenBits();
    }
    else
    {
        m_entropyCoder->resetBits();
        if (cu->getSlice()->getPPS()->getTransquantBypassEnableFlag())
        {
            m_entropyCoder->encodeCUTransquantBypassFlag(cu, 0, true);
        }
        m_entropyCoder->encodeSkipFlag(cu, 0, true);
        m_entropyCoder->encodePredMode(cu, 0, true);
        m_entropyCoder->encodePartSize(cu, 0, cu->getDepth(0), true);
        m_entropyCoder->encodePredInfo(cu, 0, true);
        bool bDummy = false;
        m_entropyCoder->encodeCoeff(cu, 0, cu->getDepth(0), cu->getWidth(0), cu->getHeight(0), bDummy);
        return m_entropyCoder->getNumberOfWrittenBits();
    }
}

/**** Function to estimate the header bits ************/
UInt  TEncSearch::estimateHeaderBits(TComDataCU* cu, UInt absPartIdx)
{
    UInt bits = 0;

    m_entropyCoder->resetBits();

    UInt lpelx = cu->getCUPelX() + g_rasterToPelX[g_zscanToRaster[absPartIdx]];
    UInt rpelx = lpelx + (g_maxCUWidth >> cu->getDepth(0))  - 1;
    UInt tpely = cu->getCUPelY() + g_rasterToPelY[g_zscanToRaster[absPartIdx]];
    UInt bpely = tpely + (g_maxCUHeight >>  cu->getDepth(0)) - 1;

    TComSlice * slice = cu->getPic()->getSlice();
    if ((rpelx < slice->getSPS()->getPicWidthInLumaSamples()) && (bpely < slice->getSPS()->getPicHeightInLumaSamples()))
    {
        m_entropyCoder->encodeSplitFlag(cu, absPartIdx,  cu->getDepth(0));
    }

    if (cu->getMergeFlag(0) && cu->getPartitionSize(0) == SIZE_2Nx2N && !cu->getQtRootCbf(0))
    {
        m_entropyCoder->encodeMergeFlag(cu, 0);
        m_entropyCoder->encodeMergeIndex(cu, 0, true);
    }

    if (cu->getSlice()->getPPS()->getTransquantBypassEnableFlag())
    {
        m_entropyCoder->encodeCUTransquantBypassFlag(cu, 0, true);
    }

    if (!cu->getSlice()->isIntra())
    {
        m_entropyCoder->encodeSkipFlag(cu, 0, true);
    }

    m_entropyCoder->encodePredMode(cu, 0, true);

    m_entropyCoder->encodePartSize(cu, 0, cu->getDepth(0), true);
    bits += m_entropyCoder->getNumberOfWrittenBits();

    return bits;
}


void  TEncSearch::setWpScalingDistParam(TComDataCU*, int, RefPicList)
{
#if 0 // dead code
    if (refIdx < 0)
    {
        m_distParam.applyWeight = false;
        return;
    }

    TComSlice       *slice = cu->getSlice();
    TComPPS         *pps   = cu->getSlice()->getPPS();
    wpScalingParam  *wp0, *wp1;
    m_distParam.applyWeight = (slice->getSliceType() == P_SLICE && pps->getUseWP()) || (slice->getSliceType() == B_SLICE && pps->getWPBiPred());
    if (!m_distParam.applyWeight) return;

    int refIdx0 = (picList == REF_PIC_LIST_0) ? refIdx : (-1);
    int refIdx1 = (picList == REF_PIC_LIST_1) ? refIdx : (-1);

    getWpScaling(cu, refIdx0, refIdx1, wp0, wp1);

    if (refIdx0 < 0) wp0 = NULL;
    if (refIdx1 < 0) wp1 = NULL;

    m_distParam.wpCur  = NULL;

    if (picList == REF_PIC_LIST_0)
    {
        m_distParam.wpCur = wp0;
    }
    else
    {
        m_distParam.wpCur = wp1;
    }
#endif
}

//! \}
