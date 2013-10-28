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
        const uint32_t numLayersToAllocate = m_cfg->getQuadtreeTULog2MaxSize() - m_cfg->getQuadtreeTULog2MinSize() + 1;
        for (uint32_t i = 0; i < numLayersToAllocate; ++i)
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
    for (uint32_t i = 0; i < 3; ++i)
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

    const uint32_t numLayersToAllocate = cfg->getQuadtreeTULog2MaxSize() - cfg->getQuadtreeTULog2MinSize() + 1;
    m_qtTempCoeffY  = new TCoeff*[numLayersToAllocate];
    m_qtTempCoeffCb = new TCoeff*[numLayersToAllocate];
    m_qtTempCoeffCr = new TCoeff*[numLayersToAllocate];

    const uint32_t numPartitions = 1 << (g_maxCUDepth << 1);
    m_qtTempTrIdx   = new UChar[numPartitions];
    m_qtTempCbf[0]  = new UChar[numPartitions];
    m_qtTempCbf[1]  = new UChar[numPartitions];
    m_qtTempCbf[2]  = new UChar[numPartitions];
    m_qtTempTComYuv  = new TShortYUV[numLayersToAllocate];
    for (uint32_t i = 0; i < numLayersToAllocate; ++i)
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

void TEncSearch::xEncSubdivCbfQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, bool bLuma, bool bChroma)
{
    uint32_t  fullDepth  = cu->getDepth(0) + trDepth;
    uint32_t  trMode     = cu->getTransformIdx(absPartIdx);
    uint32_t  subdiv     = (trMode > trDepth ? 1 : 0);
    uint32_t  trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth()] + 2 - fullDepth;

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
        uint32_t qtPartNum = cu->getPic()->getNumPartInCU() >> ((fullDepth + 1) << 1);
        for (uint32_t part = 0; part < 4; part++)
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

void TEncSearch::xEncCoeffQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, TextType ttype)
{
    uint32_t fullDepth  = cu->getDepth(0) + trDepth;
    uint32_t trMode     = cu->getTransformIdx(absPartIdx);
    uint32_t subdiv     = (trMode > trDepth ? 1 : 0);
    uint32_t trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth()] + 2 - fullDepth;
    uint32_t chroma     = (ttype != TEXT_LUMA ? 1 : 0);

    if (subdiv)
    {
        uint32_t qtPartNum = cu->getPic()->getNumPartInCU() >> ((fullDepth + 1) << 1);
        for (uint32_t part = 0; part < 4; part++)
        {
            xEncCoeffQT(cu, trDepth + 1, absPartIdx + part * qtPartNum, ttype);
        }

        return;
    }

    if (ttype != TEXT_LUMA && trSizeLog2 == 2)
    {
        assert(trDepth > 0);
        trDepth--;
        uint32_t qpdiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trDepth) << 1);
        bool bFirstQ = ((absPartIdx % qpdiv) == 0);
        if (!bFirstQ)
        {
            return;
        }
    }

    //===== coefficients =====
    uint32_t width = cu->getWidth(0) >> (trDepth + chroma);
    uint32_t height = cu->getHeight(0) >> (trDepth + chroma);
    uint32_t coeffOffset = (cu->getPic()->getMinCUWidth() * cu->getPic()->getMinCUHeight() * absPartIdx) >> (chroma << 1);
    uint32_t qtLayer = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;
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

void TEncSearch::xEncIntraHeader(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, bool bLuma, bool bChroma)
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
            uint32_t qtNumParts = cu->getTotalNumPart() >> 2;
            if (trDepth == 0)
            {
                assert(absPartIdx == 0);
                for (uint32_t part = 0; part < 4; part++)
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

uint32_t TEncSearch::xGetIntraBitsQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, bool bLuma, bool bChroma)
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

uint32_t TEncSearch::xGetIntraBitsQTChroma(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, uint32_t chromaId)
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
                                     uint32_t        trDepth,
                                     uint32_t        absPartIdx,
                                     TComYuv*    fencYuv,
                                     TComYuv*    predYuv,
                                     TShortYUV*  resiYuv,
                                     uint32_t&       outDist,
                                     int         default0Save1Load2)
{
    uint32_t    lumaPredMode = cu->getLumaIntraDir(absPartIdx);
    uint32_t    fullDepth    = cu->getDepth(0)  + trDepth;
    uint32_t    width        = cu->getWidth(0) >> trDepth;
    uint32_t    height       = cu->getHeight(0) >> trDepth;
    uint32_t    stride       = fencYuv->getStride();
    Pel*    fenc         = fencYuv->getLumaAddr(absPartIdx);
    Pel*    pred         = predYuv->getLumaAddr(absPartIdx);
    int16_t*  residual     = resiYuv->getLumaAddr(absPartIdx);
    Pel*    recon        = predYuv->getLumaAddr(absPartIdx);

    uint32_t    trSizeLog2     = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> fullDepth] + 2;
    uint32_t    qtLayer        = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;
    uint32_t    numCoeffPerInc = cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight() >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1);
    TCoeff* coeff          = m_qtTempCoeffY[qtLayer] + numCoeffPerInc * absPartIdx;

    int16_t*  reconQt        = m_qtTempTComYuv[qtLayer].getLumaAddr(absPartIdx);
    uint32_t    reconQtStride  = m_qtTempTComYuv[qtLayer].m_width;

    uint32_t    zorder           = cu->getZorderIdxInCU() + absPartIdx;
    Pel*    reconIPred       = cu->getPic()->getPicYuvRec()->getLumaAddr(cu->getAddr(), zorder);
    uint32_t    reconIPredStride = cu->getPic()->getPicYuvRec()->getStride();
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
    uint32_t absSum = 0;
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
        int16_t* resiTmp = residual;
        memset(coeff, 0, sizeof(TCoeff) * width * height);
        primitives.blockfill_s[size](resiTmp, stride, 0);
    }

    //===== reconstruction =====
    primitives.calcrecon[size](pred, residual, recon, reconQt, reconIPred, stride, reconQtStride, reconIPredStride);

    //===== update distortion =====
    int part = partitionFromSizes(width, height);
    outDist += primitives.sse_pp[part](fenc, stride, recon, stride);
}

void TEncSearch::xIntraCodingChromaBlk(TComDataCU* cu,
                                       uint32_t        trDepth,
                                       uint32_t        absPartIdx,
                                       TComYuv*    fencYuv,
                                       TComYuv*    predYuv,
                                       TShortYUV*  resiYuv,
                                       uint32_t&       outDist,
                                       uint32_t        chromaId,
                                       int         default0Save1Load2)
{
    uint32_t origTrDepth = trDepth;
    uint32_t fullDepth   = cu->getDepth(0) + trDepth;
    uint32_t trSizeLog2  = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> fullDepth] + 2;

    if (trSizeLog2 == 2)
    {
        assert(trDepth > 0);
        trDepth--;
        uint32_t qpdiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trDepth) << 1);
        bool bFirstQ = ((absPartIdx % qpdiv) == 0);
        if (!bFirstQ)
        {
            return;
        }
    }

    TextType ttype          = (chromaId > 0 ? TEXT_CHROMA_V : TEXT_CHROMA_U);
    uint32_t     chromaPredMode = cu->getChromaIntraDir(absPartIdx);
    uint32_t     width          = cu->getWidth(0) >> (trDepth + 1);
    uint32_t     height         = cu->getHeight(0) >> (trDepth + 1);
    uint32_t     stride         = fencYuv->getCStride();
    Pel*     fenc           = (chromaId > 0 ? fencYuv->getCrAddr(absPartIdx) : fencYuv->getCbAddr(absPartIdx));
    Pel*     pred           = (chromaId > 0 ? predYuv->getCrAddr(absPartIdx) : predYuv->getCbAddr(absPartIdx));
    int16_t*   residual       = (chromaId > 0 ? resiYuv->getCrAddr(absPartIdx) : resiYuv->getCbAddr(absPartIdx));
    Pel*     recon          = (chromaId > 0 ? predYuv->getCrAddr(absPartIdx) : predYuv->getCbAddr(absPartIdx));

    uint32_t    qtlayer        = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;
    uint32_t    numCoeffPerInc = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight() >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1)) >> 2;
    TCoeff* coeff          = (chromaId > 0 ? m_qtTempCoeffCr[qtlayer] : m_qtTempCoeffCb[qtlayer]) + numCoeffPerInc * absPartIdx;
    int16_t*  reconQt        = (chromaId > 0 ? m_qtTempTComYuv[qtlayer].getCrAddr(absPartIdx) : m_qtTempTComYuv[qtlayer].getCbAddr(absPartIdx));
    uint32_t    reconQtStride  = m_qtTempTComYuv[qtlayer].m_cwidth;

    uint32_t    zorder           = cu->getZorderIdxInCU() + absPartIdx;
    Pel*    reconIPred       = (chromaId > 0 ? cu->getPic()->getPicYuvRec()->getCrAddr(cu->getAddr(), zorder) : cu->getPic()->getPicYuvRec()->getCbAddr(cu->getAddr(), zorder));
    uint32_t    reconIPredStride = cu->getPic()->getPicYuvRec()->getCStride();
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
        uint32_t absSum = 0;
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
            int16_t* resiTmp = residual;
            memset(coeff, 0, sizeof(TCoeff) * width * height);
            primitives.blockfill_s[size](resiTmp, stride, 0);
        }
    }

    //===== reconstruction =====
    primitives.calcrecon[size](pred, residual, recon, reconQt, reconIPred, stride, reconQtStride, reconIPredStride);

    //===== update distortion =====
    int part = partitionFromSizes(width, height);
    uint32_t dist = primitives.sse_pp[part](fenc, stride, recon, stride);
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
                                     uint32_t        trDepth,
                                     uint32_t        absPartIdx,
                                     bool        bLumaOnly,
                                     TComYuv*    fencYuv,
                                     TComYuv*    predYuv,
                                     TShortYUV*  resiYuv,
                                     uint32_t&       outDistY,
                                     uint32_t&       outDistC,
                                     bool        bCheckFirst,
                                     UInt64&     rdCost)
{
    uint32_t    fullDepth   = cu->getDepth(0) +  trDepth;
    uint32_t    trSizeLog2  = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> fullDepth] + 2;
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
    uint32_t   singleDistY = 0;
    uint32_t   singleDistC = 0;
    uint32_t   singleCbfY  = 0;
    uint32_t   singleCbfU  = 0;
    uint32_t   singleCbfV  = 0;
    bool   checkTransformSkip  = cu->getSlice()->getPPS()->getUseTransformSkip();
    uint32_t   widthTransformSkip  = cu->getWidth(0) >> trDepth;
    uint32_t   heightTransformSkip = cu->getHeight(0) >> trDepth;
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

            uint32_t   singleDistYTmp     = 0;
            uint32_t   singleDistCTmp     = 0;
            uint32_t   singleCbfYTmp      = 0;
            uint32_t   singleCbfUTmp      = 0;
            uint32_t   singleCbfVTmp      = 0;
            UInt64 singleCostTmp      = 0;
            int    default0Save1Load2 = 0;
            int    firstCheckId       = 0;

            uint32_t   qpdiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + (trDepth - 1)) << 1);
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
                    uint32_t singleBits = xGetIntraBitsQT(cu, trDepth, absPartIdx, true, !bLumaOnly);
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
            uint32_t singleBits = xGetIntraBitsQT(cu, trDepth, absPartIdx, true, !bLumaOnly);
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
        uint32_t   splitDistY    = 0;
        uint32_t   splitDistC    = 0;
        uint32_t   qPartsDiv     = cu->getPic()->getNumPartInCU() >> ((fullDepth + 1) << 1);
        uint32_t   absPartIdxSub = absPartIdx;

        uint32_t   splitCbfY = 0;
        uint32_t   splitCbfU = 0;
        uint32_t   splitCbfV = 0;

        for (uint32_t part = 0; part < 4; part++, absPartIdxSub += qPartsDiv)
        {
            xRecurIntraCodingQT(cu, trDepth + 1, absPartIdxSub, bLumaOnly, fencYuv, predYuv, resiYuv, splitDistY, splitDistC, bCheckFirst, splitCost);

            splitCbfY |= cu->getCbf(absPartIdxSub, TEXT_LUMA, trDepth + 1);
            if (!bLumaOnly)
            {
                splitCbfU |= cu->getCbf(absPartIdxSub, TEXT_CHROMA_U, trDepth + 1);
                splitCbfV |= cu->getCbf(absPartIdxSub, TEXT_CHROMA_V, trDepth + 1);
            }
        }

        for (uint32_t offs = 0; offs < 4 * qPartsDiv; offs++)
        {
            cu->getCbf(TEXT_LUMA)[absPartIdx + offs] |= (splitCbfY << trDepth);
        }

        if (!bLumaOnly)
        {
            for (uint32_t offs = 0; offs < 4 * qPartsDiv; offs++)
            {
                cu->getCbf(TEXT_CHROMA_U)[absPartIdx + offs] |= (splitCbfU << trDepth);
                cu->getCbf(TEXT_CHROMA_V)[absPartIdx + offs] |= (splitCbfV << trDepth);
            }
        }
        //----- restore context states -----
        m_rdGoOnSbacCoder->load(m_rdSbacCoders[fullDepth][CI_QT_TRAFO_ROOT]);

        //----- determine rate and r-d cost -----
        uint32_t splitBits = xGetIntraBitsQT(cu, trDepth, absPartIdx, true, !bLumaOnly);
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
        uint32_t  width     = cu->getWidth(0) >> trDepth;
        uint32_t  height    = cu->getHeight(0) >> trDepth;
        uint32_t  qtLayer   = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;
        uint32_t  zorder    = cu->getZorderIdxInCU() + absPartIdx;
        int16_t* src      = m_qtTempTComYuv[qtLayer].getLumaAddr(absPartIdx);
        uint32_t  srcstride = m_qtTempTComYuv[qtLayer].m_width;
        Pel*  dst       = cu->getPic()->getPicYuvRec()->getLumaAddr(cu->getAddr(), zorder);
        uint32_t  dststride = cu->getPic()->getPicYuvRec()->getStride();
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

void TEncSearch::xSetIntraResultQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, bool bLumaOnly, TComYuv* reconYuv)
{
    uint32_t fullDepth = cu->getDepth(0) + trDepth;
    uint32_t trMode    = cu->getTransformIdx(absPartIdx);

    if (trMode == trDepth)
    {
        uint32_t trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> fullDepth] + 2;
        uint32_t qtlayer    = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;

        bool bSkipChroma = false;
        bool bChromaSame = false;
        if (!bLumaOnly && trSizeLog2 == 2)
        {
            assert(trDepth > 0);
            uint32_t qpdiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trDepth - 1) << 1);
            bSkipChroma = ((absPartIdx % qpdiv) != 0);
            bChromaSame = true;
        }

        //===== copy transform coefficients =====
        uint32_t numCoeffY    = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> (fullDepth << 1);
        uint32_t numCoeffIncY = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1);
        TCoeff* coeffSrcY = m_qtTempCoeffY[qtlayer] + (numCoeffIncY * absPartIdx);
        TCoeff* coeffDestY = cu->getCoeffY()        + (numCoeffIncY * absPartIdx);
        ::memcpy(coeffDestY, coeffSrcY, sizeof(TCoeff) * numCoeffY);

        if (!bLumaOnly && !bSkipChroma)
        {
            uint32_t numCoeffC    = (bChromaSame ? numCoeffY    : numCoeffY    >> 2);
            uint32_t numCoeffIncC = numCoeffIncY >> 2;
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
            uint32_t trSizeCLog2 = (bChromaSame ? trSizeLog2 : trSizeLog2 - 1);
            m_qtTempTComYuv[qtlayer].copyPartToPartChroma(reconYuv, absPartIdx, 1 << trSizeCLog2, 1 << trSizeCLog2);
        }
    }
    else
    {
        uint32_t numQPart = cu->getPic()->getNumPartInCU() >> ((fullDepth + 1) << 1);
        for (uint32_t part = 0; part < 4; part++)
        {
            xSetIntraResultQT(cu, trDepth + 1, absPartIdx + part * numQPart, bLumaOnly, reconYuv);
        }
    }
}

void TEncSearch::xStoreIntraResultQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, bool bLumaOnly)
{
    uint32_t fullMode = cu->getDepth(0) + trDepth;

    uint32_t trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> fullMode] + 2;
    uint32_t qtlayer    = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;

    bool bSkipChroma  = false;
    bool bChromaSame  = false;

    if (!bLumaOnly && trSizeLog2 == 2)
    {
        assert(trDepth > 0);
        uint32_t qpdiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trDepth - 1) << 1);
        bSkipChroma  = ((absPartIdx % qpdiv) != 0);
        bChromaSame  = true;
    }

    //===== copy transform coefficients =====
    uint32_t numCoeffY    = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> (fullMode << 1);
    uint32_t numCoeffIncY = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1);
    TCoeff* coeffSrcY = m_qtTempCoeffY[qtlayer] + (numCoeffIncY * absPartIdx);
    TCoeff* coeffDstY = m_qtTempTUCoeffY;
    ::memcpy(coeffDstY, coeffSrcY, sizeof(TCoeff) * numCoeffY);

    if (!bLumaOnly && !bSkipChroma)
    {
        uint32_t numCoeffC    = (bChromaSame ? numCoeffY : numCoeffY >> 2);
        uint32_t numCoeffIncC = numCoeffIncY >> 2;
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
        uint32_t trSizeCLog2 = (bChromaSame ? trSizeLog2 : trSizeLog2 - 1);
        m_qtTempTComYuv[qtlayer].copyPartToPartChroma(&m_qtTempTransformSkipTComYuv, absPartIdx, 1 << trSizeCLog2, 1 << trSizeCLog2);
    }
}

void TEncSearch::xLoadIntraResultQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, bool bLumaOnly)
{
    uint32_t fullDepth = cu->getDepth(0) + trDepth;

    uint32_t trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> fullDepth] + 2;
    uint32_t qtlayer    = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;

    bool bSkipChroma = false;
    bool bChromaSame = false;

    if (!bLumaOnly && trSizeLog2 == 2)
    {
        assert(trDepth > 0);
        uint32_t qpdiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trDepth - 1) << 1);
        bSkipChroma = ((absPartIdx % qpdiv) != 0);
        bChromaSame = true;
    }

    //===== copy transform coefficients =====
    uint32_t numCoeffY    = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> (fullDepth << 1);
    uint32_t numCoeffIncY = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1);
    TCoeff* coeffDstY = m_qtTempCoeffY[qtlayer] + (numCoeffIncY * absPartIdx);
    TCoeff* coeffSrcY = m_qtTempTUCoeffY;
    ::memcpy(coeffDstY, coeffSrcY, sizeof(TCoeff) * numCoeffY);

    if (!bLumaOnly && !bSkipChroma)
    {
        uint32_t numCoeffC    = (bChromaSame ? numCoeffY : numCoeffY >> 2);
        uint32_t numCoeffIncC = numCoeffIncY >> 2;
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
        uint32_t trSizeCLog2 = (bChromaSame ? trSizeLog2 : trSizeLog2 - 1);
        m_qtTempTransformSkipTComYuv.copyPartToPartChroma(&m_qtTempTComYuv[qtlayer], absPartIdx, 1 << trSizeCLog2, 1 << trSizeCLog2);
    }

    uint32_t   zOrder           = cu->getZorderIdxInCU() + absPartIdx;
    Pel*   reconIPred       = cu->getPic()->getPicYuvRec()->getLumaAddr(cu->getAddr(), zOrder);
    uint32_t   reconIPredStride = cu->getPic()->getPicYuvRec()->getStride();
    int16_t* reconQt          = m_qtTempTComYuv[qtlayer].getLumaAddr(absPartIdx);
    uint32_t   reconQtStride    = m_qtTempTComYuv[qtlayer].m_width;
    uint32_t   width            = cu->getWidth(0) >> trDepth;
    uint32_t   height           = cu->getHeight(0) >> trDepth;
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

void TEncSearch::xStoreIntraResultChromaQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, uint32_t stateU0V1Both2)
{
    uint32_t fullDepth = cu->getDepth(0) + trDepth;
    uint32_t trMode    = cu->getTransformIdx(absPartIdx);

    if (trMode == trDepth)
    {
        uint32_t trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> fullDepth] + 2;
        uint32_t qtlayer    = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;

        bool bChromaSame = false;
        if (trSizeLog2 == 2)
        {
            assert(trDepth > 0);
            trDepth--;
            uint32_t qpdiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trDepth) << 1);
            if ((absPartIdx % qpdiv) != 0)
            {
                return;
            }
            bChromaSame = true;
        }

        //===== copy transform coefficients =====
        uint32_t numCoeffC = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> (fullDepth << 1);
        if (!bChromaSame)
        {
            numCoeffC >>= 2;
        }
        uint32_t numCoeffIncC = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> ((cu->getSlice()->getSPS()->getMaxCUDepth() << 1) + 2);
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
        uint32_t trSizeCLog2 = (bChromaSame ? trSizeLog2 : trSizeLog2 - 1);
        m_qtTempTComYuv[qtlayer].copyPartToPartChroma(&m_qtTempTransformSkipTComYuv, absPartIdx, 1 << trSizeCLog2, 1 << trSizeCLog2, stateU0V1Both2);
    }
}

void TEncSearch::xLoadIntraResultChromaQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, uint32_t stateU0V1Both2)
{
    uint32_t fullDepth = cu->getDepth(0) + trDepth;
    uint32_t trMode    = cu->getTransformIdx(absPartIdx);

    if (trMode == trDepth)
    {
        uint32_t trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> fullDepth] + 2;
        uint32_t qtlayer    = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;

        bool bChromaSame = false;
        if (trSizeLog2 == 2)
        {
            assert(trDepth > 0);
            trDepth--;
            uint32_t qpdiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trDepth) << 1);
            if ((absPartIdx % qpdiv) != 0)
            {
                return;
            }
            bChromaSame = true;
        }

        //===== copy transform coefficients =====
        uint32_t numCoeffC = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> (fullDepth << 1);
        if (!bChromaSame)
        {
            numCoeffC >>= 2;
        }
        uint32_t numCoeffIncC = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> ((cu->getSlice()->getSPS()->getMaxCUDepth() << 1) + 2);

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
        uint32_t trSizeCLog2 = (bChromaSame ? trSizeLog2 : trSizeLog2 - 1);
        m_qtTempTransformSkipTComYuv.copyPartToPartChroma(&m_qtTempTComYuv[qtlayer], absPartIdx, 1 << trSizeCLog2, 1 << trSizeCLog2, stateU0V1Both2);

        uint32_t zorder           = cu->getZorderIdxInCU() + absPartIdx;
        uint32_t width            = cu->getWidth(0) >> (trDepth + 1);
        uint32_t height           = cu->getHeight(0) >> (trDepth + 1);
        uint32_t reconQtStride    = m_qtTempTComYuv[qtlayer].m_cwidth;
        uint32_t reconIPredStride = cu->getPic()->getPicYuvRec()->getCStride();

        if (stateU0V1Both2 == 0 || stateU0V1Both2 == 2)
        {
            Pel* reconIPred = cu->getPic()->getPicYuvRec()->getCbAddr(cu->getAddr(), zorder);
            int16_t* reconQt  = m_qtTempTComYuv[qtlayer].getCbAddr(absPartIdx);
            primitives.blockcpy_ps(width, height, reconIPred, reconIPredStride, reconQt, reconQtStride);
        }
        if (stateU0V1Both2 == 1 || stateU0V1Both2 == 2)
        {
            Pel* reconIPred = cu->getPic()->getPicYuvRec()->getCrAddr(cu->getAddr(), zorder);
            int16_t* reconQt  = m_qtTempTComYuv[qtlayer].getCrAddr(absPartIdx);
            primitives.blockcpy_ps(width, height, reconIPred, reconIPredStride, reconQt, reconQtStride);
        }
    }
}

void TEncSearch::xRecurIntraChromaCodingQT(TComDataCU* cu,
                                           uint32_t        trDepth,
                                           uint32_t        absPartIdx,
                                           TComYuv*    fencYuv,
                                           TComYuv*    predYuv,
                                           TShortYUV*  resiYuv,
                                           uint32_t&       outDist)
{
    uint32_t fullDepth = cu->getDepth(0) + trDepth;
    uint32_t trMode    = cu->getTransformIdx(absPartIdx);

    if (trMode == trDepth)
    {
        bool checkTransformSkip = cu->getSlice()->getPPS()->getUseTransformSkip();
        uint32_t trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> fullDepth] + 2;

        uint32_t actualTrDepth = trDepth;
        if (trSizeLog2 == 2)
        {
            assert(trDepth > 0);
            actualTrDepth--;
            uint32_t qpdiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + actualTrDepth) << 1);
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
                for (uint32_t absPartIdxSub = absPartIdx; absPartIdxSub < absPartIdx + 4; absPartIdxSub++)
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
                uint32_t    singleDistC    = 0;
                uint32_t    singleCbfC     = 0;
                uint32_t    singleDistCTmp = 0;
                UInt64  singleCostTmp  = 0;
                uint32_t    singleCbfCTmp  = 0;

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
                        uint32_t bitsTmp = xGetIntraBitsQTChroma(cu, trDepth, absPartIdx, chromaId + 2);
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
        uint32_t splitCbfU     = 0;
        uint32_t splitCbfV     = 0;
        uint32_t qPartsDiv     = cu->getPic()->getNumPartInCU() >> ((fullDepth + 1) << 1);
        uint32_t absPartIdxSub = absPartIdx;
        for (uint32_t part = 0; part < 4; part++, absPartIdxSub += qPartsDiv)
        {
            xRecurIntraChromaCodingQT(cu, trDepth + 1, absPartIdxSub, fencYuv, predYuv, resiYuv, outDist);
            splitCbfU |= cu->getCbf(absPartIdxSub, TEXT_CHROMA_U, trDepth + 1);
            splitCbfV |= cu->getCbf(absPartIdxSub, TEXT_CHROMA_V, trDepth + 1);
        }

        for (uint32_t offs = 0; offs < 4 * qPartsDiv; offs++)
        {
            cu->getCbf(TEXT_CHROMA_U)[absPartIdx + offs] |= (splitCbfU << trDepth);
            cu->getCbf(TEXT_CHROMA_V)[absPartIdx + offs] |= (splitCbfV << trDepth);
        }
    }
}

void TEncSearch::xSetIntraResultChromaQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, TComYuv* reconYuv)
{
    uint32_t fullDepth = cu->getDepth(0) + trDepth;
    uint32_t trMode    = cu->getTransformIdx(absPartIdx);

    if (trMode == trDepth)
    {
        uint32_t trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> fullDepth] + 2;
        uint32_t qtlayer    = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;

        bool bChromaSame  = false;
        if (trSizeLog2 == 2)
        {
            assert(trDepth > 0);
            uint32_t qpdiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trDepth - 1) << 1);
            if ((absPartIdx % qpdiv) != 0)
            {
                return;
            }
            bChromaSame = true;
        }

        //===== copy transform coefficients =====
        uint32_t numCoeffC = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> (fullDepth << 1);
        if (!bChromaSame)
        {
            numCoeffC >>= 2;
        }
        uint32_t numCoeffIncC = (cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight()) >> ((cu->getSlice()->getSPS()->getMaxCUDepth() << 1) + 2);
        TCoeff* coeffSrcU = m_qtTempCoeffCb[qtlayer] + (numCoeffIncC * absPartIdx);
        TCoeff* coeffSrcV = m_qtTempCoeffCr[qtlayer] + (numCoeffIncC * absPartIdx);
        TCoeff* coeffDstU = cu->getCoeffCb()         + (numCoeffIncC * absPartIdx);
        TCoeff* coeffDstV = cu->getCoeffCr()         + (numCoeffIncC * absPartIdx);
        ::memcpy(coeffDstU, coeffSrcU, sizeof(TCoeff) * numCoeffC);
        ::memcpy(coeffDstV, coeffSrcV, sizeof(TCoeff) * numCoeffC);

        //===== copy reconstruction =====
        uint32_t trSizeCLog2 = (bChromaSame ? trSizeLog2 : trSizeLog2 - 1);
        m_qtTempTComYuv[qtlayer].copyPartToPartChroma(reconYuv, absPartIdx, 1 << trSizeCLog2, 1 << trSizeCLog2);
    }
    else
    {
        uint32_t numQPart = cu->getPic()->getNumPartInCU() >> ((fullDepth + 1) << 1);
        for (uint32_t part = 0; part < 4; part++)
        {
            xSetIntraResultChromaQT(cu, trDepth + 1, absPartIdx + part * numQPart, reconYuv);
        }
    }
}

void TEncSearch::preestChromaPredMode(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv)
{
    uint32_t width  = cu->getWidth(0) >> 1;
    uint32_t height = cu->getHeight(0) >> 1;
    uint32_t stride = fencYuv->getCStride();
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
    uint32_t minMode  = 0;
    uint32_t maxMode  = 4;
    uint32_t bestMode = MAX_UINT;
    uint32_t minSAD   = MAX_UINT;
    pixelcmp_t sa8d = primitives.sa8d[(int)g_convertToBit[width]];
    for (uint32_t mode = minMode; mode < maxMode; mode++)
    {
        //--- get prediction ---
        predIntraChromaAng(patChromaU, mode, predU, stride, width);
        predIntraChromaAng(patChromaV, mode, predV, stride, width);

        //--- get SAD ---
        uint32_t sad = sa8d(fencU, stride, predU, stride) + sa8d(fencV, stride, predV, stride);

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

void TEncSearch::estIntraPredQT(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, TShortYUV* resiYuv, TComYuv* reconYuv, uint32_t& outDistC, bool bLumaOnly)
{
    uint32_t depth        = cu->getDepth(0);
    uint32_t numPU        = cu->getNumPartInter();
    uint32_t initTrDepth  = cu->getPartitionSize(0) == SIZE_2Nx2N ? 0 : 1;
    uint32_t width        = cu->getWidth(0) >> initTrDepth;
    uint32_t qNumParts    = cu->getTotalNumPart() >> 2;
    uint32_t widthBit     = cu->getIntraSizeIdx(0);
    uint32_t overallDistY = 0;
    uint32_t overallDistC = 0;
    uint32_t candNum;
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
    uint32_t partOffset = 0;
    for (uint32_t pu = 0; pu < numPU; pu++, partOffset += qNumParts)
    {
        //===== init pattern for luma prediction =====
        cu->getPattern()->initPattern(cu, initTrDepth, partOffset);

        // Reference sample smoothing
        cu->getPattern()->initAdiPattern(cu, partOffset, initTrDepth, m_predBuf, m_predBufStride, m_predBufHeight, refAbove, refLeft, refAboveFlt, refLeftFlt);

        //===== determine set of modes to be tested (using prediction signal only) =====
        int numModesAvailable = 35; //total number of Intra modes
        Pel* fenc   = fencYuv->getLumaAddr(pu, width);
        Pel* pred   = predYuv->getLumaAddr(pu, width);
        uint32_t stride = predYuv->getStride();
        uint32_t rdModeList[FAST_UDI_MAX_RDMODE_NUM];
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
            uint32_t modeCosts[35];

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
                for (uint32_t mode = 2; mode < numModesAvailable; mode++)
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
                for (uint32_t mode = 2; mode < numModesAvailable; mode++)
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

                for (uint32_t mode = 2; mode < numModesAvailable; mode++)
                {
                    predIntraLumaAng(mode, pred, stride, width);
                    modeCosts[mode] = sa8d(fenc, stride, pred, stride);
                }

#endif // if 1
            }

            // Find N least cost modes. N = numModesForFullRD
            for (uint32_t mode = 0; mode < numModesAvailable; mode++)
            {
                uint32_t sad = modeCosts[mode];
                uint32_t bits = xModeBitsIntra(cu, mode, partOffset, depth, initTrDepth);
                UInt64 cost = m_rdCost->calcRdSADCost(sad, bits);
                candNum += xUpdateCandList(mode, cost, numModesForFullRD, rdModeList, candCostList);
            }

            int preds[3] = { -1, -1, -1 };
            int mode = -1;
            int numCand = 3;
            cu->getIntraDirLumaPredictor(partOffset, preds, &mode);
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
        uint32_t    bestPUMode  = 0;
        uint32_t    bestPUDistY = 0;
        uint32_t    bestPUDistC = 0;
        UInt64  bestPUCost  = MAX_INT64;
        for (uint32_t mode = 0; mode < numModesForFullRD; mode++)
        {
            // set luma prediction mode
            uint32_t origMode = rdModeList[mode];

            cu->setLumaIntraDirSubParts(origMode, partOffset, depth + initTrDepth);

            // set context models
            m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_CURR_BEST]);

            // determine residual for partition
            uint32_t   puDistY = 0;
            uint32_t   puDistC = 0;
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

                uint32_t qPartNum = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + initTrDepth) << 1);
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
            uint32_t origMode = bestPUMode;

            cu->setLumaIntraDirSubParts(origMode, partOffset, depth + initTrDepth);

            // set context models
            m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_CURR_BEST]);

            // determine residual for partition
            uint32_t   puDistY = 0;
            uint32_t   puDistC = 0;
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

                uint32_t qPartNum = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + initTrDepth) << 1);
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
        uint32_t qPartNum = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + initTrDepth) << 1);
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
            uint32_t trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> (cu->getDepth(0) + initTrDepth)] + 2;
            if (!bLumaOnly && trSizeLog2 == 2)
            {
                assert(initTrDepth  > 0);
                bSkipChroma  = (pu != 0);
                bChromaSame  = true;
            }

            uint32_t compWidth   = cu->getWidth(0) >> initTrDepth;
            uint32_t compHeight  = cu->getHeight(0) >> initTrDepth;
            uint32_t zorder      = cu->getZorderIdxInCU() + partOffset;
            Pel* dst         = cu->getPic()->getPicYuvRec()->getLumaAddr(cu->getAddr(), zorder);
            uint32_t dststride   = cu->getPic()->getPicYuvRec()->getStride();
            Pel* src         = reconYuv->getLumaAddr(partOffset);
            uint32_t srcstride   = reconYuv->getStride();
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
        uint32_t combCbfY = 0;
        uint32_t combCbfU = 0;
        uint32_t combCbfV = 0;
        uint32_t partIdx  = 0;
        for (uint32_t part = 0; part < 4; part++, partIdx += qNumParts)
        {
            combCbfY |= cu->getCbf(partIdx, TEXT_LUMA,     1);
            combCbfU |= cu->getCbf(partIdx, TEXT_CHROMA_U, 1);
            combCbfV |= cu->getCbf(partIdx, TEXT_CHROMA_V, 1);
        }

        for (uint32_t offs = 0; offs < 4 * qNumParts; offs++)
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
                                      uint32_t        preCalcDistC)
{
    uint32_t   depth     = cu->getDepth(0);
    uint32_t   bestMode  = 0;
    uint32_t   bestDist  = 0;
    UInt64 bestCost  = MAX_INT64;

    //----- init mode list -----
    uint32_t minMode = 0;
    uint32_t maxMode = NUM_CHROMA_MODE;
    uint32_t modeList[NUM_CHROMA_MODE];

    cu->getAllowedChromaDir(0, modeList);

    //----- check chroma modes -----
    for (uint32_t mode = minMode; mode < maxMode; mode++)
    {
        //----- restore context models -----
        m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_CURR_BEST]);

        //----- chroma coding -----
        uint32_t dist = 0;
        cu->setChromIntraDirSubParts(modeList[mode], 0, depth);
        xRecurIntraChromaCodingQT(cu, 0, 0, fencYuv, predYuv, resiYuv, dist);
        if (cu->getSlice()->getPPS()->getUseTransformSkip())
        {
            m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_CURR_BEST]);
        }

        uint32_t   bits = xGetIntraBitsQT(cu, 0, 0, false, true);
        UInt64 cost  = m_rdCost->calcRdCost(dist, bits);

        //----- compare -----
        if (cost < bestCost)
        {
            bestCost = cost;
            bestDist = dist;
            bestMode = modeList[mode];
            uint32_t qpn = cu->getPic()->getNumPartInCU() >> (depth << 1);
            xSetIntraResultChromaQT(cu, 0, 0, reconYuv);
            ::memcpy(m_qtTempCbf[1], cu->getCbf(TEXT_CHROMA_U), qpn * sizeof(UChar));
            ::memcpy(m_qtTempCbf[2], cu->getCbf(TEXT_CHROMA_V), qpn * sizeof(UChar));
            ::memcpy(m_qtTempTransformSkipFlag[1], cu->getTransformSkip(TEXT_CHROMA_U), qpn * sizeof(UChar));
            ::memcpy(m_qtTempTransformSkipFlag[2], cu->getTransformSkip(TEXT_CHROMA_V), qpn * sizeof(UChar));
        }
    }

    //----- set data -----
    uint32_t qpn = cu->getPic()->getNumPartInCU() >> (depth << 1);
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
void TEncSearch::xEncPCM(TComDataCU* cu, uint32_t absPartIdx, Pel* fenc, Pel* pcm, Pel* pred, int16_t* resi, Pel* recon, uint32_t stride, uint32_t width, uint32_t height, TextType eText)
{
    uint32_t x, y;
    uint32_t reconStride;
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
    uint32_t depth      = cu->getDepth(0);
    uint32_t width      = cu->getWidth(0);
    uint32_t height     = cu->getHeight(0);
    uint32_t stride     = predYuv->getStride();
    uint32_t strideC    = predYuv->getCStride();
    uint32_t widthC     = width  >> 1;
    uint32_t heightC    = height >> 1;
    uint32_t distortion = 0;
    uint32_t bits;
    UInt64 cost;

    uint32_t absPartIdx = 0;
    uint32_t minCoeffSize = cu->getPic()->getMinCUWidth() * cu->getPic()->getMinCUHeight();
    uint32_t lumaOffset   = minCoeffSize * absPartIdx;
    uint32_t chromaOffset = lumaOffset >> 2;

    // Luminance
    Pel*   fenc = fencYuv->getLumaAddr(0, width);
    int16_t* resi = resiYuv->getLumaAddr(0, width);
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

uint32_t TEncSearch::xGetInterPredictionError(TComDataCU* cu, int partIdx)
{
    uint32_t absPartIdx;
    int width, height;

    motionCompensation(cu, &m_tmpYuvPred, REF_PIC_LIST_X, partIdx, true, false);
    cu->getPartIndexAndSize(partIdx, absPartIdx, width, height);
    uint32_t cost = m_me.bufSA8D(m_tmpYuvPred.getLumaAddr(absPartIdx), m_tmpYuvPred.getStride());
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
void TEncSearch::xMergeEstimation(TComDataCU* cu, int puIdx, uint32_t& interDir, TComMvField* mvField, uint32_t& mergeIndex, uint32_t& outCost, TComMvField* mvFieldNeighbours, UChar* interDirNeighbours, int& numValidMergeCand)
{
    uint32_t absPartIdx = 0;
    int width = 0;
    int height = 0;

    cu->getPartIndexAndSize(puIdx, absPartIdx, width, height);
    uint32_t depth = cu->getDepth(absPartIdx);
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
    for (uint32_t mergeCand = 0; mergeCand < numValidMergeCand; ++mergeCand)
    {
        uint32_t costCand = MAX_UINT;
        uint32_t bitsCand = 0;

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
void TEncSearch::xRestrictBipredMergeCand(TComDataCU* cu, uint32_t puIdx, TComMvField* mvFieldNeighbours, UChar* interDirNeighbours, int numValidMergeCand)
{
    if (cu->isBipredRestriction(puIdx))
    {
        for (uint32_t mergeCand = 0; mergeCand < numValidMergeCand; ++mergeCand)
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
 * \param predYuv
 * \param bUseMRG
 * \returns void
 */
void TEncSearch::predInterSearch(TComDataCU* cu, TComYuv* predYuv, bool bUseMRG, bool bLuma, bool bChroma)
{
    MV mvzero(0, 0);
    MV mv[2];
    MV mvBidir[2];
    MV mvTemp[2][33];
    MV mvPred[2][33];
    MV mvPredBi[2][33];

    int mvpIdxBi[2][33];
    int mvpIdx[2][33];
    int mvpNum[2][33];
    AMVPInfo amvpInfo[2][33];

    uint32_t mbBits[3] = { 1, 1, 0 };
    int refIdx[2] = { 0, 0 }; // If un-initialized, may cause SEGV in bi-directional prediction iterative stage.
    int refIdxBidir[2] = { 0, 0 };

    PartSize partSize = cu->getPartitionSize(0);
    uint32_t lastMode = 0;
    int numPart = cu->getNumPartInter();
    int numPredDir = cu->getSlice()->isInterP() ? 1 : 2;
    uint32_t biPDistTemp = MAX_INT;

    TComPicYuv *fenc = cu->getSlice()->getPic()->getPicYuvOrg();
    TComMvField mvFieldNeighbours[MRG_MAX_NUM_CANDS << 1]; // double length for mv of both lists
    UChar interDirNeighbours[MRG_MAX_NUM_CANDS];
    int numValidMergeCand = 0;

    for (int partIdx = 0; partIdx < numPart; partIdx++)
    {
        uint32_t listCost[2] = { MAX_UINT, MAX_UINT };
        uint32_t bits[3];
        uint32_t costbi = MAX_UINT;
        uint32_t costTemp = 0;
        uint32_t bitsTemp;
        MV   mvValidList1;
        int  refIdxValidList1 = 0;
        uint32_t bitsValidList1 = MAX_UINT;
        uint32_t costValidList1 = MAX_UINT;

        uint32_t partAddr;
        int  roiWidth, roiHeight;
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
            for (int list = 0; list < numPredDir; list++)
            {
                for (int idx = 0; idx < cu->getSlice()->getNumRefIdx(list); idx++)
                {
                    bitsTemp = mbBits[list];
                    if (cu->getSlice()->getNumRefIdx(list) > 1)
                    {
                        bitsTemp += idx + 1;
                        if (idx == cu->getSlice()->getNumRefIdx(list) - 1) bitsTemp--;
                    }
                    xEstimateMvPredAMVP(cu, partIdx, list, idx, mvPred[list][idx], &biPDistTemp);
                    mvpIdx[list][idx] = cu->getMVPIdx(list, partAddr);
                    mvpNum[list][idx] = cu->getMVPNum(list, partAddr);

                    bitsTemp += m_mvpIdxCost[mvpIdx[list][idx]][AMVP_MAX_NUM_CANDS];
                    int merange = m_adaptiveRange[list][idx];
                    MV& mvp = mvPred[list][idx];
                    MV& outmv = mvTemp[list][idx];

                    MV mvmin, mvmax;
                    xSetSearchRange(cu, mvp, merange, mvmin, mvmax);
                    int satdCost = m_me.motionEstimate(m_mref[list][idx],
                                                       mvmin, mvmax, mvp, 3, m_mvPredictors, merange, outmv);

                    /* Get total cost of partition, but only include MV bit cost once */
                    bitsTemp += m_me.bitcost(outmv);
                    costTemp = (satdCost - m_me.mvcost(outmv)) + m_rdCost->getCost(bitsTemp);

                    xCopyAMVPInfo(cu->getCUMvField(list)->getAMVPInfo(), &amvpInfo[list][idx]); // must always be done ( also when AMVP_MODE = AM_NONE )
                    xCheckBestMVP(cu, list, mvTemp[list][idx], mvPred[list][idx], mvpIdx[list][idx], bitsTemp, costTemp);

                    if (costTemp < listCost[list])
                    {
                        listCost[list] = costTemp;
                        bits[list] = bitsTemp; // storing for bi-prediction

                        // set motion
                        mv[list] = mvTemp[list][idx];
                        refIdx[list] = idx;
                    }

                    if (list == 1 && costTemp < costValidList1)
                    {
                        costValidList1 = costTemp;
                        bitsValidList1 = bitsTemp;

                        // set motion
                        mvValidList1     = mvTemp[list][idx];
                        refIdxValidList1 = idx;
                    }
                }
            }

            // Bi-directional prediction
            if ((cu->getSlice()->isInterB()) && (cu->isBipredRestriction(partIdx) == false))
            {
                mvBidir[0] = mv[0];
                mvBidir[1] = mv[1];
                refIdxBidir[0] = refIdx[0];
                refIdxBidir[1] = refIdx[1];

                ::memcpy(mvPredBi, mvPred, sizeof(mvPred));
                ::memcpy(mvpIdxBi, mvpIdx, sizeof(mvpIdx));

                // Generate reference subpels
                xPredInterLumaBlk(cu, cu->getSlice()->getRefPic(REF_PIC_LIST_0, refIdx[0])->getPicYuvRec(), partAddr, &mv[0], roiWidth, roiHeight, &m_predYuv[0]);
                xPredInterLumaBlk(cu, cu->getSlice()->getRefPic(REF_PIC_LIST_1, refIdx[1])->getPicYuvRec(), partAddr, &mv[1], roiWidth, roiHeight, &m_predYuv[1]);

                pixel *ref0 = m_predYuv[0].getLumaAddr(partAddr);
                pixel *ref1 = m_predYuv[1].getLumaAddr(partAddr);

                ALIGN_VAR_32(pixel, avg[MAX_CU_SIZE * MAX_CU_SIZE]);

                int partEnum = partitionFromSizes(roiWidth, roiHeight);
                primitives.pixelavg_pp[partEnum](avg, roiWidth, ref0, m_predYuv[0].getStride(), ref1, m_predYuv[1].getStride(), 32);
                int satdCost = primitives.satd[partEnum](pu, fenc->getStride(), avg, roiWidth);
                x265_emms();
                bits[2] = bits[0] + bits[1] - mbBits[0] - mbBits[1] + mbBits[2];
                costbi =  satdCost + m_rdCost->getCost(bits[2]);

                if (mv[0].notZero() || mv[1].notZero())
                {
                    ref0 = m_mref[0][refIdx[0]]->fpelPlane + (pu - fenc->getLumaAddr());  //MV(0,0) of ref0
                    ref1 = m_mref[1][refIdx[1]]->fpelPlane + (pu - fenc->getLumaAddr());  //MV(0,0) of ref1
                    intptr_t refStride = m_mref[0][refIdx[0]]->lumaStride;

                    primitives.pixelavg_pp[partEnum](avg, roiWidth, ref0, refStride, ref1, refStride, 32);
                    satdCost = primitives.satd[partEnum](pu, fenc->getStride(), avg, roiWidth);
                    x265_emms();

                    unsigned int bitsZero0, bitsZero1;
                    m_me.setMVP(mvPredBi[0][refIdxBidir[0]]);
                    bitsZero0 = bits[0] - m_me.bitcost(mv[0]) + m_me.bitcost(mvzero);

                    m_me.setMVP(mvPredBi[1][refIdxBidir[1]]);
                    bitsZero1 = bits[1] - m_me.bitcost(mv[1]) + m_me.bitcost(mvzero);

                    uint32_t costZero = satdCost + m_rdCost->getCost(bitsZero0) + m_rdCost->getCost(bitsZero1);

                    MV mvpZero[2];
                    int mvpidxZero[2];
                    mvpZero[0] = mvPredBi[0][refIdxBidir[0]];
                    mvpidxZero[0] = mvpIdxBi[0][refIdxBidir[0]];
                    xCopyAMVPInfo(&amvpInfo[0][refIdxBidir[0]], cu->getCUMvField(REF_PIC_LIST_0)->getAMVPInfo());
                    xCheckBestMVP(cu, REF_PIC_LIST_0, mvzero, mvpZero[0], mvpidxZero[0], bitsZero0, costZero);
                    mvpZero[1] = mvPredBi[1][refIdxBidir[1]];
                    mvpidxZero[1] = mvpIdxBi[1][refIdxBidir[1]];
                    xCopyAMVPInfo(&amvpInfo[1][refIdxBidir[1]], cu->getCUMvField(REF_PIC_LIST_1)->getAMVPInfo());
                    xCheckBestMVP(cu, REF_PIC_LIST_1, mvzero, mvpZero[1], mvpidxZero[1], bitsZero1, costZero);

                    if (costZero < costbi)
                    {
                        costbi = costZero;
                        mvBidir[0].x = mvBidir[0].y = 0;
                        mvBidir[1].x = mvBidir[1].y = 0;
                        mvPredBi[0][refIdxBidir[0]] = mvpZero[0];
                        mvPredBi[1][refIdxBidir[1]] = mvpZero[1];
                        mvpIdxBi[0][refIdxBidir[0]] = mvpidxZero[0];
                        mvpIdxBi[1][refIdxBidir[1]] = mvpidxZero[1];
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

        uint32_t mebits = 0;
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
                    MV mvtmp = mvBidir[0] - mvPredBi[0][refIdxBidir[0]];
                    cu->getCUMvField(REF_PIC_LIST_0)->setAllMvd(mvtmp, partSize, partAddr, 0, partIdx);
                }
                {
                    MV mvtmp = mvBidir[1] - mvPredBi[1][refIdxBidir[1]];
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
                    MV mvtmp = mv[0] - mvPred[0][refIdx[0]];
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
                    MV mvtmp = mv[1] - mvPred[1][refIdx[1]];
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
            uint32_t mrgInterDir = 0;
            TComMvField mrgMvField[2];
            uint32_t mrgIndex = 0;

            uint32_t meInterDir = 0;
            TComMvField meMvField[2];

            // calculate ME cost
            uint32_t meError = MAX_UINT;
            uint32_t meCost = MAX_UINT;

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
            uint32_t mrgCost = MAX_UINT;
            xMergeEstimation(cu, partIdx, mrgInterDir, mrgMvField, mrgIndex, mrgCost, mvFieldNeighbours, interDirNeighbours, numValidMergeCand);
            if (mrgCost < meCost)
            {
                // set Merge result
                cu->setMergeFlagSubParts(true, partAddr, partIdx, cu->getDepth(partAddr));
                cu->setMergeIndexSubParts(mrgIndex, partAddr, partIdx, cu->getDepth(partAddr));
                cu->setInterDirSubParts(mrgInterDir, partAddr, partIdx, cu->getDepth(partAddr));
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
            }
        }
        motionCompensation(cu, predYuv, REF_PIC_LIST_X, partIdx, bLuma, bChroma);
    }

    setWpScalingDistParam(cu, -1, REF_PIC_LIST_X);
}

// AMVP
void TEncSearch::xEstimateMvPredAMVP(TComDataCU* cu, uint32_t partIdx, int list, int refIdx, MV& mvPred, uint32_t* distBiP)
{
    AMVPInfo* amvpInfo = cu->getCUMvField(list)->getAMVPInfo();

    MV   bestMv;
    int  bestIdx = 0;
    uint32_t bestCost = MAX_INT;
    uint32_t partAddr = 0;
    int  roiWidth, roiHeight;
    int  i;

    cu->getPartIndexAndSize(partIdx, partAddr, roiWidth, roiHeight);

    // Fill the MV Candidates
    cu->fillMvpCand(partIdx, partAddr, list, refIdx, amvpInfo);

    bestMv = amvpInfo->m_mvCand[0];
    if (amvpInfo->m_num <= 1)
    {
        mvPred = bestMv;

        cu->setMVPIdxSubParts(bestIdx, list, partAddr, partIdx, cu->getDepth(partAddr));
        cu->setMVPNumSubParts(amvpInfo->m_num, list, partAddr, partIdx, cu->getDepth(partAddr));

        if (cu->getSlice()->getMvdL1ZeroFlag() && list == REF_PIC_LIST_1)
        {
            (*distBiP) = xGetTemplateCost(cu, partAddr, &m_predTempYuv, mvPred, 0, AMVP_MAX_NUM_CANDS, list, refIdx, roiWidth, roiHeight);
        }
        return;
    }

    m_predTempYuv.clear();

    //-- Check Minimum Cost.
    for (i = 0; i < amvpInfo->m_num; i++)
    {
        uint32_t cost = xGetTemplateCost(cu, partAddr, &m_predTempYuv, amvpInfo->m_mvCand[i], i, AMVP_MAX_NUM_CANDS, list, refIdx, roiWidth, roiHeight);
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
    cu->setMVPIdxSubParts(bestIdx, list, partAddr, partIdx, cu->getDepth(partAddr));
    cu->setMVPNumSubParts(amvpInfo->m_num, list, partAddr, partIdx, cu->getDepth(partAddr));
}

uint32_t TEncSearch::xGetMvpIdxBits(int idx, int num)
{
    assert(idx >= 0 && num >= 0 && idx < num);

    if (num == 1)
        return 0;

    uint32_t length = 1;
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

void TEncSearch::xGetBlkBits(PartSize cuMode, bool bPSlice, int partIdx, uint32_t lastMode, uint32_t blockBit[3])
{
    if (cuMode == SIZE_2Nx2N)
    {
        blockBit[0] = (!bPSlice) ? 3 : 1;
        blockBit[1] = 3;
        blockBit[2] = 5;
    }
    else if ((cuMode == SIZE_2NxN || cuMode == SIZE_2NxnU) || cuMode == SIZE_2NxnD)
    {
        uint32_t aauiMbBits[2][3][3] = { { { 0, 0, 3 }, { 0, 0, 0 }, { 0, 0, 0 } }, { { 5, 7, 7 }, { 7, 5, 7 }, { 9 - 3, 9 - 3, 9 - 3 } } };
        if (bPSlice)
        {
            blockBit[0] = 3;
            blockBit[1] = 0;
            blockBit[2] = 0;
        }
        else
        {
            ::memcpy(blockBit, aauiMbBits[partIdx][lastMode], 3 * sizeof(uint32_t));
        }
    }
    else if ((cuMode == SIZE_Nx2N || cuMode == SIZE_nLx2N) || cuMode == SIZE_nRx2N)
    {
        uint32_t aauiMbBits[2][3][3] = { { { 0, 2, 3 }, { 0, 0, 0 }, { 0, 0, 0 } }, { { 5, 7, 7 }, { 7 - 2, 7 - 2, 9 - 2 }, { 9 - 3, 9 - 3, 9 - 3 } } };
        if (bPSlice)
        {
            blockBit[0] = 3;
            blockBit[1] = 0;
            blockBit[2] = 0;
        }
        else
        {
            ::memcpy(blockBit, aauiMbBits[partIdx][lastMode], 3 * sizeof(uint32_t));
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
void TEncSearch::xCheckBestMVP(TComDataCU* cu, int list, MV mv, MV& mvPred, int& outMvpIdx, uint32_t& outBits, uint32_t& outCost)
{
    AMVPInfo* amvpInfo = cu->getCUMvField(list)->getAMVPInfo();

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
        uint32_t origOutBits = outBits;
        outBits = origOutBits - mvBitsOrig + bestMvBits;
        outCost = (outCost - m_rdCost->getCost(origOutBits))  + m_rdCost->getCost(outBits);
    }
}

uint32_t TEncSearch::xGetTemplateCost(TComDataCU* cu, uint32_t partAddr, TComYuv* templateCand, MV mvCand, int mvpIdx,
                                  int mvpCandCount, int list, int refIdx, int sizex, int sizey)
{
    // TODO: does it clip with m_referenceRowsAvailable?
    cu->clipMv(mvCand);

    // prediction pattern
    xPredInterLumaBlk(cu, cu->getSlice()->getRefPic(list, refIdx)->getPicYuvRec(), partAddr, &mvCand, sizex, sizey, templateCand);

    // calc distortion
    uint32_t cost = m_me.bufSAD(templateCand->getLumaAddr(partAddr), templateCand->getStride());
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
    uint32_t bits = 0, bestBits = 0;
    uint32_t distortion = 0, bdist = 0;

    uint32_t width  = cu->getWidth(0);
    uint32_t height = cu->getHeight(0);

    //  No residual coding : SKIP mode
    if (bSkipRes)
    {
        cu->setSkipFlagSubParts(true, 0, cu->getDepth(0));

        outResiYuv->clear();

        predYuv->copyToPartYuv(outReconYuv, 0);

        int part = partitionFromSizes(width, height);
        distortion = primitives.sse_pp[part](fencYuv->getLumaAddr(), fencYuv->getStride(), outReconYuv->getLumaAddr(), outReconYuv->getStride());
        part = partitionFromSizes(width >> 1, height >> 1);
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

    uint32_t trLevel = 0;
    if ((cu->getWidth(0) > cu->getSlice()->getSPS()->getMaxTrSize()))
    {
        while (cu->getWidth(0) > (cu->getSlice()->getSPS()->getMaxTrSize() << trLevel))
        {
            trLevel++;
        }
    }
    uint32_t maxTrLevel = 1 + trLevel;

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

    uint32_t zeroDistortion = 0;
    xEstimateResidualQT(cu, 0, 0, outResiYuv, cu->getDepth(0), cost, bits, distortion, &zeroDistortion);

    m_entropyCoder->resetBits();
    m_entropyCoder->encodeQtRootCbfZero(cu);
    uint32_t zeroResiBits = m_entropyCoder->getNumberOfWrittenBits();

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

        const uint32_t qpartnum = cu->getPic()->getNumPartInCU() >> (cu->getDepth(0) << 1);
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
    int part = partitionFromSizes(width, height);
    bdist = primitives.sse_pp[part](fencYuv->getLumaAddr(), fencYuv->getStride(), outReconYuv->getLumaAddr(), outReconYuv->getStride());
    part = partitionFromSizes(width >> 1, height >> 1);
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
                                     uint32_t        absPartIdx,
                                     uint32_t        absTUPartIdx,
                                     TShortYUV*  resiYuv,
                                     const uint32_t  depth,
                                     UInt64 &    rdCost,
                                     uint32_t &      outBits,
                                     uint32_t &      outDist,
                                     uint32_t *      outZeroDist)
{
    assert(cu->getDepth(0) == cu->getDepth(absPartIdx));
    const uint32_t trMode = depth - cu->getDepth(0);
    const uint32_t trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> depth] + 2;

    bool bSplitFlag = ((cu->getSlice()->getSPS()->getQuadtreeTUMaxDepthInter() == 1) && cu->getPredictionMode(absPartIdx) == MODE_INTER && (cu->getPartitionSize(absPartIdx) != SIZE_2Nx2N));
    bool bCheckFull;
    if (bSplitFlag && depth == cu->getDepth(absPartIdx) && (trSizeLog2 > cu->getQuadtreeTULog2MinSizeInCU(absPartIdx)))
        bCheckFull = false;
    else
        bCheckFull = (trSizeLog2 <= cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize());
    const bool bCheckSplit = (trSizeLog2 > cu->getQuadtreeTULog2MinSizeInCU(absPartIdx));
    assert(bCheckFull || bCheckSplit);

    bool  bCodeChroma = true;
    uint32_t  trModeC     = trMode;
    uint32_t  trSizeCLog2 = trSizeLog2 - 1;
    if (trSizeLog2 == 2)
    {
        trSizeCLog2++;
        trModeC--;
        uint32_t qpdiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trModeC) << 1);
        bCodeChroma = ((absPartIdx % qpdiv) == 0);
    }

    const uint32_t setCbf = 1 << trMode;
    // code full block
    UInt64 singleCost = MAX_INT64;
    uint32_t singleBits = 0;
    uint32_t singleDist = 0;
    uint32_t absSumY = 0, absSumU = 0, absSumV = 0;
    int lastPosY = -1, lastPosU = -1, lastPosV = -1;
    uint32_t bestTransformMode[3] = { 0 };

    m_rdGoOnSbacCoder->store(m_rdSbacCoders[depth][CI_QT_TRAFO_ROOT]);

    if (bCheckFull)
    {
        const uint32_t numCoeffPerAbsPartIdxIncrement = cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight() >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1);
        const uint32_t qtlayer = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;
        TCoeff *coeffCurY = m_qtTempCoeffY[qtlayer] + (numCoeffPerAbsPartIdxIncrement * absPartIdx);
        TCoeff *coeffCurU = m_qtTempCoeffCb[qtlayer] + (numCoeffPerAbsPartIdxIncrement * absPartIdx >> 2);
        TCoeff *coeffCurV = m_qtTempCoeffCr[qtlayer] + (numCoeffPerAbsPartIdxIncrement * absPartIdx >> 2);

        int trWidth = 0, trHeight = 0, trWidthC = 0, trHeightC = 0;
        uint32_t absTUPartIdxC = absPartIdx;

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
        const uint32_t uiSingleBitsY = m_entropyCoder->getNumberOfWrittenBits();

        uint32_t singleBitsU = 0;
        uint32_t singleBitsV = 0;
        if (bCodeChroma)
        {
            m_entropyCoder->encodeQtCbf(cu, absPartIdx, TEXT_CHROMA_U, trMode);
            m_entropyCoder->encodeCoeffNxN(cu, coeffCurU, absPartIdx, trWidthC, trHeightC, depth, TEXT_CHROMA_U);
            singleBitsU = m_entropyCoder->getNumberOfWrittenBits() - uiSingleBitsY;

            m_entropyCoder->encodeQtCbf(cu, absPartIdx, TEXT_CHROMA_V, trMode);
            m_entropyCoder->encodeCoeffNxN(cu, coeffCurV, absPartIdx, trWidthC, trHeightC, depth, TEXT_CHROMA_V);
            singleBitsV = m_entropyCoder->getNumberOfWrittenBits() - (uiSingleBitsY + singleBitsU);
        }

        const uint32_t numSamplesLuma = 1 << (trSizeLog2 << 1);
        const uint32_t numSamplesChroma = 1 << (trSizeCLog2 << 1);

        ::memset(m_tempPel, 0, sizeof(Pel) * numSamplesLuma); // not necessary needed for inside of recursion (only at the beginning)

        int partSize = partitionFromSizes(trWidth, trHeight);
        uint32_t distY = primitives.sse_sp[partSize](resiYuv->getLumaAddr(absTUPartIdx), resiYuv->m_width, m_tempPel, trWidth);

        if (outZeroDist)
        {
            *outZeroDist += distY;
        }
        if (absSumY)
        {
            int16_t *curResiY = m_qtTempTComYuv[qtlayer].getLumaAddr(absTUPartIdx);

            m_trQuant->setQPforQuant(cu->getQP(0), TEXT_LUMA, cu->getSlice()->getSPS()->getQpBDOffsetY(), 0);

            int scalingListType = 3 + g_eTTable[(int)TEXT_LUMA];
            assert(scalingListType < 6);
            m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdx), REG_DCT, curResiY, m_qtTempTComYuv[qtlayer].m_width,  coeffCurY, trWidth, trHeight, scalingListType, false, lastPosY); //this is for inter mode only

            const uint32_t nonZeroDistY = primitives.sse_ss[partSize](resiYuv->getLumaAddr(absTUPartIdx), resiYuv->m_width, m_qtTempTComYuv[qtlayer].getLumaAddr(absTUPartIdx),
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
                const uint32_t nullBitsY = m_entropyCoder->getNumberOfWrittenBits();
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
            const uint32_t nullBitsY = m_entropyCoder->getNumberOfWrittenBits();
            minCostY = m_rdCost->calcRdCost(distY, nullBitsY);
        }

        if (!absSumY)
        {
            int16_t *ptr =  m_qtTempTComYuv[qtlayer].getLumaAddr(absTUPartIdx);
            const uint32_t stride = m_qtTempTComYuv[qtlayer].m_width;

            assert(trWidth == trHeight);
            primitives.blockfill_s[(int)g_convertToBit[trWidth]](ptr, stride, 0);
        }

        uint32_t distU = 0;
        uint32_t distV = 0;

        int partSizeC = partitionFromSizes(trWidthC, trHeightC);
        if (bCodeChroma)
        {
            distU = m_rdCost->scaleChromaDistCb(primitives.sse_sp[partSizeC](resiYuv->getCbAddr(absTUPartIdxC), resiYuv->m_cwidth, m_tempPel, trWidthC));

            if (outZeroDist)
            {
                *outZeroDist += distU;
            }
            if (absSumU)
            {
                int16_t *pcResiCurrU = m_qtTempTComYuv[qtlayer].getCbAddr(absTUPartIdxC);

                int curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCbQpOffset() + cu->getSlice()->getSliceQpDeltaCb();
                m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset);

                int scalingListType = 3 + g_eTTable[(int)TEXT_CHROMA_U];
                assert(scalingListType < 6);
                m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdx), REG_DCT, pcResiCurrU, m_qtTempTComYuv[qtlayer].m_cwidth, coeffCurU, trWidthC, trHeightC, scalingListType, false, lastPosU);

                uint32_t dist = primitives.sse_ss[partSizeC](resiYuv->getCbAddr(absTUPartIdxC), resiYuv->m_cwidth,
                                                         m_qtTempTComYuv[qtlayer].getCbAddr(absTUPartIdxC),
                                                         m_qtTempTComYuv[qtlayer].m_cwidth);
                const uint32_t nonZeroDistU = m_rdCost->scaleChromaDistCb(dist);

                if (cu->isLosslessCoded(0))
                {
                    distU = nonZeroDistU;
                }
                else
                {
                    const UInt64 singleCostU = m_rdCost->calcRdCost(nonZeroDistU, singleBitsU);
                    m_entropyCoder->resetBits();
                    m_entropyCoder->encodeQtCbfZero(cu, TEXT_CHROMA_U, trMode);
                    const uint32_t nullBitsU = m_entropyCoder->getNumberOfWrittenBits();
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
                const uint32_t nullBitsU = m_entropyCoder->getNumberOfWrittenBits();
                minCostU = m_rdCost->calcRdCost(distU, nullBitsU);
            }
            if (!absSumU)
            {
                int16_t *ptr = m_qtTempTComYuv[qtlayer].getCbAddr(absTUPartIdxC);
                const uint32_t stride = m_qtTempTComYuv[qtlayer].m_cwidth;

                assert(trWidthC == trHeightC);
                primitives.blockfill_s[(int)g_convertToBit[trWidthC]](ptr, stride, 0);
            }

            distV = m_rdCost->scaleChromaDistCr(primitives.sse_sp[partSizeC](resiYuv->getCrAddr(absTUPartIdxC), resiYuv->m_cwidth, m_tempPel, trWidthC));
            if (outZeroDist)
            {
                *outZeroDist += distV;
            }
            if (absSumV)
            {
                int16_t *curResiV = m_qtTempTComYuv[qtlayer].getCrAddr(absTUPartIdxC);
                int curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCrQpOffset() + cu->getSlice()->getSliceQpDeltaCr();
                m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset);

                int scalingListType = 3 + g_eTTable[(int)TEXT_CHROMA_V];
                assert(scalingListType < 6);
                m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdx), REG_DCT, curResiV, m_qtTempTComYuv[qtlayer].m_cwidth, coeffCurV, trWidthC, trHeightC, scalingListType, false, lastPosV);

                uint32_t dist = primitives.sse_ss[partSizeC](resiYuv->getCrAddr(absTUPartIdxC), resiYuv->m_cwidth,
                                                         m_qtTempTComYuv[qtlayer].getCrAddr(absTUPartIdxC),
                                                         m_qtTempTComYuv[qtlayer].m_cwidth);
                const uint32_t nonZeroDistV = m_rdCost->scaleChromaDistCr(dist);

                if (cu->isLosslessCoded(0))
                {
                    distV = nonZeroDistV;
                }
                else
                {
                    const UInt64 singleCostV = m_rdCost->calcRdCost(nonZeroDistV, singleBitsV);
                    m_entropyCoder->resetBits();
                    m_entropyCoder->encodeQtCbfZero(cu, TEXT_CHROMA_V, trMode);
                    const uint32_t nullBitsV = m_entropyCoder->getNumberOfWrittenBits();
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
                const uint32_t nullBitsV = m_entropyCoder->getNumberOfWrittenBits();
                minCostV = m_rdCost->calcRdCost(distV, nullBitsV);
            }
            if (!absSumV)
            {
                int16_t *ptr =  m_qtTempTComYuv[qtlayer].getCrAddr(absTUPartIdxC);
                const uint32_t stride = m_qtTempTComYuv[qtlayer].m_cwidth;

                assert(trWidthC == trHeightC);
                primitives.blockfill_s[(int)g_convertToBit[trWidthC]](ptr, stride, 0);
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
            uint32_t nonZeroDistY = 0, absSumTransformSkipY;
            int lastPosTransformSkipY = -1;
            UInt64 singleCostY = MAX_INT64;

            int16_t *curResiY = m_qtTempTComYuv[qtlayer].getLumaAddr(absTUPartIdx);
            uint32_t resiStride = m_qtTempTComYuv[qtlayer].m_width;

            TCoeff bestCoeffY[32 * 32];
            memcpy(bestCoeffY, coeffCurY, sizeof(TCoeff) * numSamplesLuma);

            int16_t bestResiY[32 * 32];
            for (int i = 0; i < trHeight; ++i)
            {
                memcpy(bestResiY + i * trWidth, curResiY + i * resiStride, sizeof(int16_t) * trWidth);
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
                const uint32_t skipSingleBitsY = m_entropyCoder->getNumberOfWrittenBits();

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
                    memcpy(curResiY + i * resiStride, &bestResiY[i * trWidth], sizeof(int16_t) * trWidth);
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
            uint32_t nonZeroDistU = 0, nonZeroDistV = 0, absSumTransformSkipU, absSumTransformSkipV;
            int lastPosTransformSkipU = -1, lastPosTransformSkipV = -1;
            UInt64 singleCostU = MAX_INT64;
            UInt64 singleCostV = MAX_INT64;

            int16_t *curResiU = m_qtTempTComYuv[qtlayer].getCbAddr(absTUPartIdxC);
            int16_t *curResiV = m_qtTempTComYuv[qtlayer].getCrAddr(absTUPartIdxC);
            uint32_t stride = m_qtTempTComYuv[qtlayer].m_cwidth;

            TCoeff bestCoeffU[32 * 32], bestCoeffV[32 * 32];
            memcpy(bestCoeffU, coeffCurU, sizeof(TCoeff) * numSamplesChroma);
            memcpy(bestCoeffV, coeffCurV, sizeof(TCoeff) * numSamplesChroma);

            int16_t bestResiU[32 * 32], bestResiV[32 * 32];
            for (int i = 0; i < trHeightC; ++i)
            {
                memcpy(&bestResiU[i * trWidthC], curResiU + i * stride, sizeof(int16_t) * trWidthC);
                memcpy(&bestResiV[i * trWidthC], curResiV + i * stride, sizeof(int16_t) * trWidthC);
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

                uint32_t dist = primitives.sse_ss[partSizeC](resiYuv->getCbAddr(absTUPartIdxC), resiYuv->m_cwidth,
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
                    memcpy(curResiU + i * stride, &bestResiU[i * trWidthC], sizeof(int16_t) * trWidthC);
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

                uint32_t dist = primitives.sse_ss[partSizeC](resiYuv->getCrAddr(absTUPartIdxC), resiYuv->m_cwidth,
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
                    memcpy(curResiV + i * stride, &bestResiV[i * trWidthC], sizeof(int16_t) * trWidthC);
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
        uint32_t subdivDist = 0;
        uint32_t subdivBits = 0;
        UInt64 subDivCost = 0;

        const uint32_t qPartNumSubdiv = cu->getPic()->getNumPartInCU() >> ((depth + 1) << 1);
        for (uint32_t i = 0; i < 4; ++i)
        {
            uint32_t nsAddr = absPartIdx + i * qPartNumSubdiv;
            xEstimateResidualQT(cu, absPartIdx + i * qPartNumSubdiv, nsAddr, resiYuv, depth + 1, subDivCost, subdivBits, subdivDist, bCheckFull ? NULL : outZeroDist);
        }

        uint32_t ycbf = 0;
        uint32_t ucbf = 0;
        uint32_t vcbf = 0;
        for (uint32_t i = 0; i < 4; ++i)
        {
            ycbf |= cu->getCbf(absPartIdx + i * qPartNumSubdiv, TEXT_LUMA,     trMode + 1);
            ucbf |= cu->getCbf(absPartIdx + i * qPartNumSubdiv, TEXT_CHROMA_U, trMode + 1);
            vcbf |= cu->getCbf(absPartIdx + i * qPartNumSubdiv, TEXT_CHROMA_V, trMode + 1);
        }

        for (uint32_t i = 0; i < 4 * qPartNumSubdiv; ++i)
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

void TEncSearch::xEncodeResidualQT(TComDataCU* cu, uint32_t absPartIdx, const uint32_t depth, bool bSubdivAndCbf, TextType ttype)
{
    assert(cu->getDepth(0) == cu->getDepth(absPartIdx));
    const uint32_t curTrMode = depth - cu->getDepth(0);
    const uint32_t trMode = cu->getTransformIdx(absPartIdx);
    const bool bSubdiv = curTrMode != trMode;
    const uint32_t trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> depth] + 2;

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
        const uint32_t numCoeffPerAbsPartIdxIncrement = cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight() >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1);
        //assert( 16 == uiNumCoeffPerAbsPartIdxIncrement ); // check
        const uint32_t qtlayer = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;
        TCoeff *coeffCurY = m_qtTempCoeffY[qtlayer] +  numCoeffPerAbsPartIdxIncrement * absPartIdx;
        TCoeff *coeffCurU = m_qtTempCoeffCb[qtlayer] + (numCoeffPerAbsPartIdxIncrement * absPartIdx >> 2);
        TCoeff *coeffCurV = m_qtTempCoeffCr[qtlayer] + (numCoeffPerAbsPartIdxIncrement * absPartIdx >> 2);

        bool  bCodeChroma = true;
        uint32_t  trModeC     = trMode;
        uint32_t  trSizeCLog2 = trSizeLog2 - 1;
        if (trSizeLog2 == 2)
        {
            trSizeCLog2++;
            trModeC--;
            uint32_t qpdiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trModeC) << 1);
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
            const uint32_t qpartNumSubdiv = cu->getPic()->getNumPartInCU() >> ((depth + 1) << 1);
            for (uint32_t i = 0; i < 4; ++i)
            {
                xEncodeResidualQT(cu, absPartIdx + i * qpartNumSubdiv, depth + 1, bSubdivAndCbf, ttype);
            }
        }
    }
}

void TEncSearch::xSetResidualQTData(TComDataCU* cu, uint32_t absPartIdx, uint32_t absTUPartIdx, TShortYUV* resiYuv, uint32_t depth, bool bSpatial)
{
    assert(cu->getDepth(0) == cu->getDepth(absPartIdx));
    const uint32_t curTrMode = depth - cu->getDepth(0);
    const uint32_t trMode = cu->getTransformIdx(absPartIdx);

    if (curTrMode == trMode)
    {
        const uint32_t trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUWidth() >> depth] + 2;
        const uint32_t qtlayer = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;

        bool  bCodeChroma   = true;
        uint32_t  trModeC     = trMode;
        uint32_t  trSizeCLog2 = trSizeLog2 - 1;
        if (trSizeLog2 == 2)
        {
            trSizeCLog2++;
            trModeC--;
            uint32_t qpdiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trModeC) << 1);
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
            uint32_t uiNumCoeffPerAbsPartIdxIncrement = cu->getSlice()->getSPS()->getMaxCUWidth() * cu->getSlice()->getSPS()->getMaxCUHeight() >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1);
            uint32_t uiNumCoeffY = (1 << (trSizeLog2 << 1));
            TCoeff* pcCoeffSrcY = m_qtTempCoeffY[qtlayer] +  uiNumCoeffPerAbsPartIdxIncrement * absPartIdx;
            TCoeff* pcCoeffDstY = cu->getCoeffY() + uiNumCoeffPerAbsPartIdxIncrement * absPartIdx;
            ::memcpy(pcCoeffDstY, pcCoeffSrcY, sizeof(TCoeff) * uiNumCoeffY);
            if (bCodeChroma)
            {
                uint32_t    uiNumCoeffC = (1 << (trSizeCLog2 << 1));
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
        const uint32_t qPartNumSubdiv = cu->getPic()->getNumPartInCU() >> ((depth + 1) << 1);
        for (uint32_t i = 0; i < 4; ++i)
        {
            uint32_t nsAddr = absPartIdx + i * qPartNumSubdiv;
            xSetResidualQTData(cu, absPartIdx + i * qPartNumSubdiv, nsAddr, resiYuv, depth + 1, bSpatial);
        }
    }
}

uint32_t TEncSearch::xModeBitsIntra(TComDataCU* cu, uint32_t mode, uint32_t partOffset, uint32_t depth, uint32_t initTrDepth)
{
    // Reload only contexts required for coding intra mode information
    m_rdGoOnSbacCoder->loadIntraDirModeLuma(m_rdSbacCoders[depth][CI_CURR_BEST]);

    cu->setLumaIntraDirSubParts(mode, partOffset, depth + initTrDepth);

    m_entropyCoder->resetBits();
    m_entropyCoder->encodeIntraDirModeLuma(cu, partOffset);

    return m_entropyCoder->getNumberOfWrittenBits();
}

uint32_t TEncSearch::xUpdateCandList(uint32_t mode, UInt64 cost, uint32_t fastCandNum, uint32_t* CandModeList, UInt64* CandCostList)
{
    uint32_t i;
    uint32_t shift = 0;

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
uint32_t TEncSearch::xSymbolBitsInter(TComDataCU* cu)
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
uint32_t  TEncSearch::estimateHeaderBits(TComDataCU* cu, uint32_t absPartIdx)
{
    uint32_t bits = 0;

    m_entropyCoder->resetBits();

    uint32_t lpelx = cu->getCUPelX() + g_rasterToPelX[g_zscanToRaster[absPartIdx]];
    uint32_t rpelx = lpelx + (g_maxCUWidth >> cu->getDepth(0))  - 1;
    uint32_t tpely = cu->getCUPelY() + g_rasterToPelY[g_zscanToRaster[absPartIdx]];
    uint32_t bpely = tpely + (g_maxCUHeight >>  cu->getDepth(0)) - 1;

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

void  TEncSearch::setWpScalingDistParam(TComDataCU*, int, int)
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

    int refIdx0 = (list == REF_PIC_LIST_0) ? refIdx : (-1);
    int refIdx1 = (list == REF_PIC_LIST_1) ? refIdx : (-1);

    getWpScaling(cu, refIdx0, refIdx1, wp0, wp1);

    if (refIdx0 < 0) wp0 = NULL;
    if (refIdx1 < 0) wp1 = NULL;

    m_distParam.wpCur  = NULL;

    if (list == REF_PIC_LIST_0)
    {
        m_distParam.wpCur = wp0;
    }
    else
    {
        m_distParam.wpCur = wp1;
    }
#endif // if 0
}

//! \}
