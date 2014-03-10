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
#include "encoder.h"

#include "common.h"
#include "primitives.h"
#include "PPA/ppa.h"

using namespace x265;

DECLARE_CYCLE_COUNTER(ME);

//! \ingroup TLibEncoder
//! \{

TEncSearch::TEncSearch()
{
    m_qtTempCoeffY  = NULL;
    m_qtTempCoeffCb = NULL;
    m_qtTempCoeffCr = NULL;
    m_qtTempTrIdx = NULL;
    m_qtTempShortYuv = NULL;
    m_qtTempTUCoeffY  = NULL;
    m_qtTempTUCoeffCb = NULL;
    m_qtTempTUCoeffCr = NULL;
    for (int i = 0; i < 3; i++)
    {
        m_qtTempTransformSkipFlag[i] = NULL;
        m_qtTempCbf[i] = NULL;
    }

    m_cfg = NULL;
    m_rdCost  = NULL;
    m_trQuant = NULL;
    m_entropyCoder = NULL;
    m_rdSbacCoders    = NULL;
    m_rdGoOnSbacCoder = NULL;
}

TEncSearch::~TEncSearch()
{
    if (m_cfg)
    {
        const uint32_t numLayersToAllocate = m_cfg->m_quadtreeTULog2MaxSize - m_cfg->m_quadtreeTULog2MinSize + 1;
        for (uint32_t i = 0; i < numLayersToAllocate; ++i)
        {
            X265_FREE(m_qtTempCoeffY[i]);
            X265_FREE(m_qtTempCoeffCb[i]);
            X265_FREE(m_qtTempCoeffCr[i]);
            m_qtTempShortYuv[i].destroy();
        }
    }
    X265_FREE(m_qtTempTUCoeffY);
    X265_FREE(m_qtTempTUCoeffCb);
    X265_FREE(m_qtTempTUCoeffCr);
    X265_FREE(m_qtTempTrIdx);
    for (uint32_t i = 0; i < 3; ++i)
    {
        X265_FREE(m_qtTempCbf[i]);
        X265_FREE(m_qtTempTransformSkipFlag[i]);
    }

    delete[] m_qtTempCoeffY;
    delete[] m_qtTempCoeffCb;
    delete[] m_qtTempCoeffCr;
    delete[] m_qtTempShortYuv;
    m_qtTempTransformSkipYuv.destroy();
    m_tmpYuvPred.destroy();
}

bool TEncSearch::init(Encoder* cfg, TComRdCost* rdCost, TComTrQuant* trQuant)
{
    m_cfg     = cfg;
    m_trQuant = trQuant;
    m_rdCost  = rdCost;

    initTempBuff(cfg->param->internalCsp);
    m_me.setSearchMethod(cfg->param->searchMethod);
    m_me.setSubpelRefine(cfg->param->subpelRefine);

    /* When frame parallelism is active, only 'refLagPixels' of reference frames will be guaranteed
     * available for motion reference.  See refLagRows in FrameEncoder::compressCTURows() */
    m_refLagPixels = cfg->param->frameNumThreads > 1 ? cfg->param->searchRange : cfg->param->sourceHeight;

    // default to no adaptive range
    for (int dir = 0; dir < 2; dir++)
    {
        for (int ref = 0; ref < MAX_NUM_REF; ref++)
        {
            m_adaptiveRange[dir][ref] = cfg->param->searchRange;
        }
    }

    const uint32_t numLayersToAllocate = cfg->m_quadtreeTULog2MaxSize - cfg->m_quadtreeTULog2MinSize + 1;
    m_qtTempCoeffY  = new TCoeff*[numLayersToAllocate];
    m_qtTempCoeffCb = new TCoeff*[numLayersToAllocate];
    m_qtTempCoeffCr = new TCoeff*[numLayersToAllocate];
    m_qtTempShortYuv = new ShortYuv[numLayersToAllocate];
    for (uint32_t i = 0; i < numLayersToAllocate; ++i)
    {
        m_qtTempCoeffY[i]  = X265_MALLOC(TCoeff, g_maxCUSize * g_maxCUSize);
        m_qtTempCoeffCb[i] = X265_MALLOC(TCoeff, (g_maxCUSize >> m_hChromaShift) * (g_maxCUSize >> m_vChromaShift));
        m_qtTempCoeffCr[i] = X265_MALLOC(TCoeff, (g_maxCUSize >> m_hChromaShift) * (g_maxCUSize >> m_vChromaShift));
        m_qtTempShortYuv[i].create(MAX_CU_SIZE, MAX_CU_SIZE, cfg->param->internalCsp);
    }

    const uint32_t numPartitions = 1 << (g_maxCUDepth << 1);
    CHECKED_MALLOC(m_qtTempTrIdx, uint8_t, numPartitions);
    CHECKED_MALLOC(m_qtTempCbf[0], uint8_t, numPartitions);
    CHECKED_MALLOC(m_qtTempCbf[1], uint8_t, numPartitions);
    CHECKED_MALLOC(m_qtTempCbf[2], uint8_t, numPartitions);
    CHECKED_MALLOC(m_qtTempTransformSkipFlag[0], uint8_t, numPartitions);
    CHECKED_MALLOC(m_qtTempTransformSkipFlag[1], uint8_t, numPartitions);
    CHECKED_MALLOC(m_qtTempTransformSkipFlag[2], uint8_t, numPartitions);

    CHECKED_MALLOC(m_qtTempTUCoeffY, TCoeff, MAX_TS_WIDTH * MAX_TS_HEIGHT);
    CHECKED_MALLOC(m_qtTempTUCoeffCb, TCoeff, MAX_TS_WIDTH * MAX_TS_HEIGHT);
    CHECKED_MALLOC(m_qtTempTUCoeffCr, TCoeff, MAX_TS_WIDTH * MAX_TS_HEIGHT);

    return m_qtTempTransformSkipYuv.create(g_maxCUSize, g_maxCUSize, cfg->param->internalCsp) &&
           m_tmpYuvPred.create(MAX_CU_SIZE, MAX_CU_SIZE, cfg->param->internalCsp);

fail:
    return false;
}

void TEncSearch::setQPLambda(int QP, double lambdaLuma, double lambdaChroma)
{
    m_trQuant->setLambda(lambdaLuma, lambdaChroma);
    m_me.setQP(QP);
}

void TEncSearch::xEncSubdivCbfQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, bool bLuma, bool bChroma)
{
    uint32_t fullDepth  = cu->getDepth(0) + trDepth;
    uint32_t trMode     = cu->getTransformIdx(absPartIdx);
    uint32_t subdiv     = (trMode > trDepth ? 1 : 0);
    uint32_t trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUSize()] + 2 - fullDepth;
    int      chFmt      = cu->getChromaFormat();

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
        if ((trSizeLog2 > 2) && !(chFmt == CHROMA_444))
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
    uint32_t trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUSize()] + 2 - fullDepth;
    uint32_t chroma     = (ttype != TEXT_LUMA ? 1 : 0);
    int chFmt           = cu->getChromaFormat();

    if (subdiv)
    {
        uint32_t qtPartNum = cu->getPic()->getNumPartInCU() >> ((fullDepth + 1) << 1);
        for (uint32_t part = 0; part < 4; part++)
        {
            xEncCoeffQT(cu, trDepth + 1, absPartIdx + part * qtPartNum, ttype);
        }

        return;
    }

    if ((ttype != TEXT_LUMA) && (trSizeLog2 == 2) && !(chFmt == CHROMA_444))
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
    int cspx = chroma ? m_hChromaShift : 0;
    int cspy = chroma ? m_vChromaShift : 0;
    uint32_t width = cu->getCUSize(0) >> (trDepth + cspx);
    uint32_t height = cu->getCUSize(0) >> (trDepth + cspy);
    uint32_t coeffOffset = (cu->getPic()->getMinCUSize() >> cspx) * (cu->getPic()->getMinCUSize() >> cspy) * absPartIdx;
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
        if ((cu->getPartitionSize(0) == SIZE_2Nx2N) || !(cu->getChromaFormat() == CHROMA_444))
        {
            if (absPartIdx == 0)
            {
                m_entropyCoder->encodeIntraDirModeChroma(cu, absPartIdx, true);
            }
        }
        else
        {
            uint32_t qtNumParts = cu->getTotalNumPart() >> 2;
            assert(trDepth > 0);
            if ((absPartIdx % qtNumParts) == 0)
                m_entropyCoder->encodeIntraDirModeChroma(cu, absPartIdx, true);
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
                                     uint32_t    trDepth,
                                     uint32_t    absPartIdx,
                                     TComYuv*    fencYuv,
                                     TComYuv*    predYuv,
                                     ShortYuv*   resiYuv,
                                     uint32_t&   outDist,
                                     bool        bReusePred)
{
    uint32_t fullDepth    = cu->getDepth(0)  + trDepth;
    uint32_t width        = cu->getCUSize(0) >> trDepth;
    uint32_t height       = cu->getCUSize(0) >> trDepth;
    uint32_t stride       = fencYuv->getStride();
    Pel*     fenc         = fencYuv->getLumaAddr(absPartIdx);
    Pel*     pred         = predYuv->getLumaAddr(absPartIdx);
    int16_t* residual     = resiYuv->getLumaAddr(absPartIdx);
    int      chFmt        = cu->getChromaFormat();
    int      part         = partitionFromSizes(width, height);

    uint32_t trSizeLog2     = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUSize() >> fullDepth] + 2;
    uint32_t qtLayer        = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;
    uint32_t numCoeffPerInc = cu->getSlice()->getSPS()->getMaxCUSize() * cu->getSlice()->getSPS()->getMaxCUSize() >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1);
    TCoeff*  coeff          = m_qtTempCoeffY[qtLayer] + numCoeffPerInc * absPartIdx;

    int16_t* reconQt        = m_qtTempShortYuv[qtLayer].getLumaAddr(absPartIdx);

    assert(m_qtTempShortYuv[qtLayer].m_width == MAX_CU_SIZE);

    uint32_t zorder           = cu->getZorderIdxInCU() + absPartIdx;
    Pel*     reconIPred       = cu->getPic()->getPicYuvRec()->getLumaAddr(cu->getAddr(), zorder);
    uint32_t reconIPredStride = cu->getPic()->getPicYuvRec()->getStride();
    bool     useTransformSkip = cu->getTransformSkip(absPartIdx, TEXT_LUMA);

    if (!bReusePred)
    {
        //===== init availability pattern =====
        cu->getPattern()->initAdiPattern(cu, absPartIdx, trDepth, m_predBuf, m_predBufStride, m_predBufHeight, m_refAbove, m_refLeft, m_refAboveFlt, m_refLeftFlt);
        uint32_t lumaPredMode = cu->getLumaIntraDir(absPartIdx);
        //===== get prediction signal =====
        predIntraLumaAng(lumaPredMode, pred, stride, width);
    }

    //===== get residual signal =====
    assert(!((uint32_t)(size_t)fenc & (width - 1)));
    assert(!((uint32_t)(size_t)pred & (width - 1)));
    assert(!((uint32_t)(size_t)residual & (width - 1)));
    primitives.calcresidual[(int)g_convertToBit[width]](fenc, pred, residual, stride);

    //===== transform and quantization =====
    //--- init rate estimation arrays for RDOQ ---
    if (useTransformSkip ? m_cfg->bEnableRDOQTS : m_cfg->bEnableRDOQ)
    {
        m_entropyCoder->estimateBit(m_trQuant->m_estBitsSbac, width, TEXT_LUMA);
    }

    //--- transform and quantization ---
    uint32_t absSum = 0;
    int lastPos = -1;
    cu->setTrIdxSubParts(trDepth, absPartIdx, fullDepth);

    m_trQuant->setQPforQuant(cu->getQP(0), TEXT_LUMA, cu->getSlice()->getSPS()->getQpBDOffsetY(), 0, chFmt);
    m_trQuant->selectLambda(TEXT_LUMA);

    absSum = m_trQuant->transformNxN(cu, residual, stride, coeff, width, TEXT_LUMA, absPartIdx, &lastPos, useTransformSkip);

    //--- set coded block flag ---
    cu->setCbfSubParts((absSum ? 1 : 0) << trDepth, TEXT_LUMA, absPartIdx, fullDepth);

    //--- inverse transform ---
    int size = g_convertToBit[width];
    if (absSum)
    {
        int scalingListType = 0 + TEXT_LUMA;
        assert(scalingListType < 6);
        m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdx), cu->getLumaIntraDir(absPartIdx), residual, stride, coeff, width, scalingListType, useTransformSkip, lastPos);
    }
    else
    {
        int16_t* resiTmp = residual;
        memset(coeff, 0, sizeof(TCoeff) * width * height);
        primitives.blockfill_s[size](resiTmp, stride, 0);
    }

    assert(width <= 32);
#if NEW_CALCRECON
    //===== reconstruction =====
    primitives.calcrecon[size](pred, residual, 0, reconQt, reconIPred, stride, MAX_CU_SIZE, reconIPredStride);
    //===== update distortion =====
    outDist += primitives.sse_sp[part](reconQt, MAX_CU_SIZE, fenc, stride);
#else
    ALIGN_VAR_32(pixel, recon[MAX_CU_SIZE * MAX_CU_SIZE]);
    //===== reconstruction =====
    primitives.calcrecon[size](pred, residual, recon, reconQt, reconIPred, stride, MAX_CU_SIZE, reconIPredStride);
    //===== update distortion =====
    outDist += primitives.sse_pp[part](fenc, stride, recon, stride);
#endif
}

void TEncSearch::xIntraCodingChromaBlk(TComDataCU* cu,
                                       uint32_t    trDepth,
                                       uint32_t    absPartIdx,
                                       TComYuv*    fencYuv,
                                       TComYuv*    predYuv,
                                       ShortYuv*   resiYuv,
                                       uint32_t&   outDist,
                                       uint32_t    chromaId,
                                       bool        bReusePred)
{
    uint32_t origTrDepth = trDepth;
    uint32_t fullDepth   = cu->getDepth(0) + trDepth;
    uint32_t trSizeLog2  = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUSize() >> fullDepth] + 2;
    int      chFmt       = cu->getChromaFormat();

    if ((trSizeLog2 == 2) && !(chFmt == CHROMA_444))
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
    uint32_t width          = cu->getCUSize(0) >> (trDepth + m_hChromaShift);
    uint32_t height         = cu->getCUSize(0) >> (trDepth + m_vChromaShift);
    uint32_t stride         = fencYuv->getCStride();
    Pel*     fenc           = (chromaId > 0 ? fencYuv->getCrAddr(absPartIdx) : fencYuv->getCbAddr(absPartIdx));
    Pel*     pred           = (chromaId > 0 ? predYuv->getCrAddr(absPartIdx) : predYuv->getCbAddr(absPartIdx));
    int16_t* residual       = (chromaId > 0 ? resiYuv->getCrAddr(absPartIdx) : resiYuv->getCbAddr(absPartIdx));

    uint32_t qtlayer        = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;
    uint32_t numCoeffPerInc = (cu->getSlice()->getSPS()->getMaxCUSize() * cu->getSlice()->getSPS()->getMaxCUSize() >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1)) >> (m_hChromaShift + m_vChromaShift);
    TCoeff*  coeff          = (chromaId > 0 ? m_qtTempCoeffCr[qtlayer] : m_qtTempCoeffCb[qtlayer]) + numCoeffPerInc * absPartIdx;
    int16_t* reconQt        = (chromaId > 0 ? m_qtTempShortYuv[qtlayer].getCrAddr(absPartIdx) : m_qtTempShortYuv[qtlayer].getCbAddr(absPartIdx));
    uint32_t reconQtStride  = m_qtTempShortYuv[qtlayer].m_cwidth;
    uint32_t zorder           = cu->getZorderIdxInCU() + absPartIdx;
    Pel*     reconIPred       = (chromaId > 0 ? cu->getPic()->getPicYuvRec()->getCrAddr(cu->getAddr(), zorder) : cu->getPic()->getPicYuvRec()->getCbAddr(cu->getAddr(), zorder));
    uint32_t reconIPredStride = cu->getPic()->getPicYuvRec()->getCStride();
    bool     useTransformSkipChroma = cu->getTransformSkip(absPartIdx, ttype);
    int      part = partitionFromSizes(width, height);

    if (!bReusePred)
    {
        //===== init availability pattern =====
        cu->getPattern()->initAdiPatternChroma(cu, absPartIdx, trDepth, m_predBuf, m_predBufStride, m_predBufHeight, chromaId);
        Pel* chromaPred = TComPattern::getAdiChromaBuf(chromaId, height, m_predBuf);

        uint32_t chromaPredMode = cu->getChromaIntraDir(absPartIdx);
        //===== update chroma mode =====
        if (chromaPredMode == DM_CHROMA_IDX)
        {
            chromaPredMode = cu->getLumaIntraDir(absPartIdx);
        }

        //===== get prediction signal =====
        predIntraChromaAng(chromaPred, chromaPredMode, pred, stride, width, height, chFmt);
    }

    //===== get residual signal =====
    assert(!((uint32_t)(size_t)fenc & (width - 1)));
    assert(!((uint32_t)(size_t)pred & (width - 1)));
    assert(!((uint32_t)(size_t)residual & (width - 1)));
    int size = g_convertToBit[width];
    primitives.calcresidual[size](fenc, pred, residual, stride);

    //===== transform and quantization =====
    {
        //--- init rate estimation arrays for RDOQ ---
        if (useTransformSkipChroma ? m_cfg->bEnableRDOQTS : m_cfg->bEnableRDOQ)
        {
            m_entropyCoder->estimateBit(m_trQuant->m_estBitsSbac, width, ttype);
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
        m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset, chFmt);

        m_trQuant->selectLambda(TEXT_CHROMA);

        absSum = m_trQuant->transformNxN(cu, residual, stride, coeff, width, ttype, absPartIdx, &lastPos, useTransformSkipChroma);

        //--- set coded block flag ---
        cu->setCbfSubParts((absSum ? 1 : 0) << origTrDepth, ttype, absPartIdx, cu->getDepth(0) + trDepth);

        //--- inverse transform ---
        if (absSum)
        {
            int scalingListType = 0 + ttype;
            assert(scalingListType < 6);
            m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdx), REG_DCT, residual, stride, coeff, width, scalingListType, useTransformSkipChroma, lastPos);
        }
        else
        {
            int16_t* resiTmp = residual;
            memset(coeff, 0, sizeof(TCoeff) * width * height);
            primitives.blockfill_s[size](resiTmp, stride, 0);
        }
    }

    assert(((intptr_t)residual & (width - 1)) == 0);
    assert(width <= 32);
#if NEW_CALCRECON
    //===== reconstruction =====
    primitives.calcrecon[size](pred, residual, 0, reconQt, reconIPred, stride, reconQtStride, reconIPredStride);
    //===== update distortion =====
    uint32_t dist = primitives.sse_sp[part](reconQt, reconQtStride, fenc, stride);
#else
    ALIGN_VAR_32(pixel, recon[MAX_CU_SIZE * MAX_CU_SIZE]);
    //===== reconstruction =====
    primitives.calcrecon[size](pred, residual, recon, reconQt, reconIPred, stride, reconQtStride, reconIPredStride);
    //===== update distortion =====
    uint32_t dist = primitives.sse_pp[part](fenc, stride, recon, stride);
#endif
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
                                     uint32_t    trDepth,
                                     uint32_t    absPartIdx,
                                     TComYuv*    fencYuv,
                                     TComYuv*    predYuv,
                                     ShortYuv*   resiYuv,
                                     uint32_t&   outDistY,
                                     bool        bCheckFirst,
                                     uint64_t&   rdCost)
{
    uint32_t fullDepth   = cu->getDepth(0) +  trDepth;
    uint32_t trSizeLog2  = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUSize() >> fullDepth] + 2;
    bool     bCheckFull  = (trSizeLog2 <= cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize());
    bool     bCheckSplit = (trSizeLog2 > cu->getQuadtreeTULog2MinSizeInCU(absPartIdx));

    int maxTuSize = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize();
    int isIntraSlice = (cu->getSlice()->getSliceType() == I_SLICE);

    // don't check split if TU size is less or equal to max TU size
    bool noSplitIntraMaxTuSize = bCheckFull;

    if (m_cfg->param->rdPenalty && !isIntraSlice)
    {
        // in addition don't check split if TU size is less or equal to 16x16 TU size for non-intra slice
        noSplitIntraMaxTuSize = (trSizeLog2 <= X265_MIN(maxTuSize, 4));

        // if maximum RD-penalty don't check TU size 32x32
        if (m_cfg->param->rdPenalty == 2)
        {
            bCheckFull = (trSizeLog2 <= X265_MIN(maxTuSize, 4));
        }
    }
    if (bCheckFirst && noSplitIntraMaxTuSize)
    {
        bCheckSplit = false;
    }

    uint64_t singleCost  = MAX_INT64;
    uint32_t singleDistY = 0;
    uint32_t singleCbfY  = 0;
    bool   checkTransformSkip  = cu->getSlice()->getPPS()->getUseTransformSkip();
    uint32_t widthTransformSkip  = cu->getCUSize(0) >> trDepth;
    uint32_t heightTransformSkip = cu->getCUSize(0) >> trDepth;
    int    bestModeId    = 0;

    checkTransformSkip &= (widthTransformSkip == 4 && heightTransformSkip == 4);
    checkTransformSkip &= (!cu->getCUTransquantBypass(0));
    checkTransformSkip &= (!((cu->getQP(0) == 0) && (cu->getSlice()->getSPS()->getUseLossless())));
    if (m_cfg->param->bEnableTSkipFast)
    {
        checkTransformSkip &= (cu->getPartitionSize(absPartIdx) == SIZE_NxN);
    }

    if (bCheckFull)
    {
        if (checkTransformSkip == true)
        {
            //----- store original entropy coding status -----
            m_rdGoOnSbacCoder->store(m_rdSbacCoders[fullDepth][CI_QT_TRAFO_ROOT]);

            uint32_t singleDistYTmp     = 0;
            uint32_t singleCbfYTmp      = 0;
            uint64_t singleCostTmp      = 0;
            const int firstCheckId      = 0;

            for (int modeId = firstCheckId; modeId < 2; modeId++)
            {
                singleDistYTmp = 0;
                cu->setTransformSkipSubParts(modeId, TEXT_LUMA, absPartIdx, fullDepth);
                //----- code luma block with given intra prediction mode and store Cbf-----
                bool bReusePred = modeId != firstCheckId;
                xIntraCodingLumaBlk(cu, trDepth, absPartIdx, fencYuv, predYuv, resiYuv, singleDistYTmp, bReusePred);
                singleCbfYTmp = cu->getCbf(absPartIdx, TEXT_LUMA, trDepth);
                //----- determine rate and r-d cost -----
                if (modeId == 1 && singleCbfYTmp == 0)
                {
                    //In order not to code TS flag when cbf is zero, the case for TS with cbf being zero is forbidden.
                    singleCostTmp = MAX_INT64;
                }
                else
                {
                    uint32_t singleBits = xGetIntraBitsQT(cu, trDepth, absPartIdx, true, false);
                    singleCostTmp = m_rdCost->calcRdCost(singleDistYTmp, singleBits);
                }

                if (singleCostTmp < singleCost)
                {
                    singleCost   = singleCostTmp;
                    singleDistY = singleDistYTmp;
                    singleCbfY  = singleCbfYTmp;
                    bestModeId    = modeId;
                    if (bestModeId == firstCheckId)
                    {
                        xStoreIntraResultQT(cu, trDepth, absPartIdx);
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
                xLoadIntraResultQT(cu, trDepth, absPartIdx);
                cu->setCbfSubParts(singleCbfY << trDepth, TEXT_LUMA, absPartIdx, fullDepth);
                m_rdGoOnSbacCoder->load(m_rdSbacCoders[fullDepth][CI_TEMP_BEST]);
            }
        }
        else
        {
            cu->setTransformSkipSubParts(0, TEXT_LUMA, absPartIdx, fullDepth);

            //----- store original entropy coding status -----
            m_rdGoOnSbacCoder->store(m_rdSbacCoders[fullDepth][CI_QT_TRAFO_ROOT]);

            //----- code luma block with given intra prediction mode and store Cbf-----
            xIntraCodingLumaBlk(cu, trDepth, absPartIdx, fencYuv, predYuv, resiYuv, singleDistY);
            if (bCheckSplit)
            {
                singleCbfY = cu->getCbf(absPartIdx, TEXT_LUMA, trDepth);
            }
            //----- determine rate and r-d cost -----
            uint32_t singleBits = xGetIntraBitsQT(cu, trDepth, absPartIdx, true, false);
            if (m_cfg->param->rdPenalty && (trSizeLog2 == 5) && !isIntraSlice)
            {
                singleBits = singleBits * 4;
            }
            singleCost = m_rdCost->calcRdCost(singleDistY, singleBits);
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
        uint64_t splitCost     = 0;
        uint32_t splitDistY    = 0;
        uint32_t qPartsDiv     = cu->getPic()->getNumPartInCU() >> ((fullDepth + 1) << 1);
        uint32_t absPartIdxSub = absPartIdx;

        uint32_t splitCbfY = 0;

        for (uint32_t part = 0; part < 4; part++, absPartIdxSub += qPartsDiv)
        {
            xRecurIntraCodingQT(cu, trDepth + 1, absPartIdxSub, fencYuv, predYuv, resiYuv, splitDistY, bCheckFirst, splitCost);

            splitCbfY |= cu->getCbf(absPartIdxSub, TEXT_LUMA, trDepth + 1);
        }

        for (uint32_t offs = 0; offs < 4 * qPartsDiv; offs++)
        {
            cu->getCbf(TEXT_LUMA)[absPartIdx + offs] |= (splitCbfY << trDepth);
        }

        //----- restore context states -----
        m_rdGoOnSbacCoder->load(m_rdSbacCoders[fullDepth][CI_QT_TRAFO_ROOT]);

        //----- determine rate and r-d cost -----
        uint32_t splitBits = xGetIntraBitsQT(cu, trDepth, absPartIdx, true, false);
        splitCost = m_rdCost->calcRdCost(splitDistY, splitBits);

        //===== compare and set best =====
        if (splitCost < singleCost)
        {
            //--- update cost ---
            outDistY += splitDistY;
            rdCost   += splitCost;
            return;
        }

        //----- set entropy coding status -----
        m_rdGoOnSbacCoder->load(m_rdSbacCoders[fullDepth][CI_QT_TRAFO_TEST]);

        //--- set transform index and Cbf values ---
        cu->setTrIdxSubParts(trDepth, absPartIdx, fullDepth);
        cu->setCbfSubParts(singleCbfY << trDepth, TEXT_LUMA, absPartIdx, fullDepth);
        cu->setTransformSkipSubParts(bestModeId, TEXT_LUMA, absPartIdx, fullDepth);

        //--- set reconstruction for next intra prediction blocks ---
        uint32_t width     = cu->getCUSize(0) >> trDepth;
        uint32_t height    = cu->getCUSize(0) >> trDepth;
        uint32_t qtLayer   = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;
        uint32_t zorder    = cu->getZorderIdxInCU() + absPartIdx;
        int16_t* src       = m_qtTempShortYuv[qtLayer].getLumaAddr(absPartIdx);
        assert(m_qtTempShortYuv[qtLayer].m_width == MAX_CU_SIZE);
        Pel*     dst       = cu->getPic()->getPicYuvRec()->getLumaAddr(cu->getAddr(), zorder);
        uint32_t dststride = cu->getPic()->getPicYuvRec()->getStride();
        primitives.blockcpy_ps(width, height, dst, dststride, src, MAX_CU_SIZE);
    }

    outDistY += singleDistY;
    rdCost   += singleCost;
}

void TEncSearch::residualTransformQuantIntra(TComDataCU* cu,
                                             uint32_t    trDepth,
                                             uint32_t    absPartIdx,
                                             TComYuv*    fencYuv,
                                             TComYuv*    predYuv,
                                             ShortYuv*   resiYuv,
                                             TComYuv*    reconYuv)
{
    uint32_t fullDepth   = cu->getDepth(0) +  trDepth;
    uint32_t trSizeLog2  = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUSize() >> fullDepth] + 2;
    bool     bCheckFull  = (trSizeLog2 <= cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize());
    bool     bCheckSplit = (trSizeLog2 > cu->getQuadtreeTULog2MinSizeInCU(absPartIdx));

    int maxTuSize = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize();
    int isIntraSlice = (cu->getSlice()->getSliceType() == I_SLICE);

    if (m_cfg->param->rdPenalty && !isIntraSlice)
    {
        // if maximum RD-penalty don't check TU size 32x32
        if (m_cfg->param->rdPenalty == 2)
        {
            bCheckFull = (trSizeLog2 <= X265_MIN(maxTuSize, 4));
        }
    }
    if (bCheckFull)
    {
        cu->setTransformSkipSubParts(0, TEXT_LUMA, absPartIdx, fullDepth);

        //----- code luma block with given intra prediction mode and store Cbf-----
        uint32_t lumaPredMode = cu->getLumaIntraDir(absPartIdx);
        uint32_t width        = cu->getCUSize(0) >> trDepth;
        uint32_t height       = cu->getCUSize(0) >> trDepth;
        int      chFmt        = cu->getChromaFormat();
        uint32_t stride       = fencYuv->getStride();
        Pel*     fenc         = fencYuv->getLumaAddr(absPartIdx);
        Pel*     pred         = predYuv->getLumaAddr(absPartIdx);
        int16_t* residual     = resiYuv->getLumaAddr(absPartIdx);
        Pel*     recon        = reconYuv->getLumaAddr(absPartIdx);

        uint32_t numCoeffPerInc = cu->getSlice()->getSPS()->getMaxCUSize() * cu->getSlice()->getSPS()->getMaxCUSize() >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1);
        TCoeff*  coeff          = cu->getCoeffY() + numCoeffPerInc * absPartIdx;

        uint32_t zorder           = cu->getZorderIdxInCU() + absPartIdx;
        Pel*     reconIPred       = cu->getPic()->getPicYuvRec()->getLumaAddr(cu->getAddr(), zorder);
        uint32_t reconIPredStride = cu->getPic()->getPicYuvRec()->getStride();

        bool     useTransformSkip = cu->getTransformSkip(absPartIdx, TEXT_LUMA);

        //===== init availability pattern =====

        cu->getPattern()->initAdiPattern(cu, absPartIdx, trDepth, m_predBuf, m_predBufStride, m_predBufHeight, m_refAbove, m_refLeft, m_refAboveFlt, m_refLeftFlt);
        //===== get prediction signal =====
        predIntraLumaAng(lumaPredMode, pred, stride, width);

        //===== get residual signal =====
        assert(!((uint32_t)(size_t)fenc & (width - 1)));
        assert(!((uint32_t)(size_t)pred & (width - 1)));
        assert(!((uint32_t)(size_t)residual & (width - 1)));
        primitives.calcresidual[(int)g_convertToBit[width]](fenc, pred, residual, stride);

        //===== transform and quantization =====
        uint32_t absSum = 0;
        int lastPos = -1;
        cu->setTrIdxSubParts(trDepth, absPartIdx, fullDepth);

        m_trQuant->setQPforQuant(cu->getQP(0), TEXT_LUMA, cu->getSlice()->getSPS()->getQpBDOffsetY(), 0, chFmt);
        m_trQuant->selectLambda(TEXT_LUMA);
        absSum = m_trQuant->transformNxN(cu, residual, stride, coeff, width, TEXT_LUMA, absPartIdx, &lastPos, useTransformSkip);

        //--- set coded block flag ---
        cu->setCbfSubParts((absSum ? 1 : 0) << trDepth, TEXT_LUMA, absPartIdx, fullDepth);

        //--- inverse transform ---
        int size = g_convertToBit[width];
        if (absSum)
        {
            int scalingListType = 0 + TEXT_LUMA;
            assert(scalingListType < 6);
            m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdx), cu->getLumaIntraDir(absPartIdx), residual, stride, coeff, width, scalingListType, useTransformSkip, lastPos);
        }
        else
        {
            int16_t* resiTmp = residual;
            memset(coeff, 0, sizeof(TCoeff) * width * height);
            primitives.blockfill_s[size](resiTmp, stride, 0);
        }

        //Generate Recon
        assert(width <= 32);
        int part = partitionFromSizes(width, height);
        primitives.luma_add_ps[part](recon, stride, pred, residual, stride, stride);
        primitives.blockcpy_pp(width, height, reconIPred, reconIPredStride, recon, stride);
    }

    if (bCheckSplit && !bCheckFull)
    {
        //----- code splitted block -----

        uint32_t qPartsDiv     = cu->getPic()->getNumPartInCU() >> ((fullDepth + 1) << 1);
        uint32_t absPartIdxSub = absPartIdx;
        uint32_t splitCbfY = 0;

        for (uint32_t part = 0; part < 4; part++, absPartIdxSub += qPartsDiv)
        {
            residualTransformQuantIntra(cu, trDepth + 1, absPartIdxSub, fencYuv, predYuv, resiYuv, reconYuv);
            splitCbfY |= cu->getCbf(absPartIdxSub, TEXT_LUMA, trDepth + 1);
        }

        for (uint32_t offs = 0; offs < 4 * qPartsDiv; offs++)
        {
            cu->getCbf(TEXT_LUMA)[absPartIdx + offs] |= (splitCbfY << trDepth);
        }

        return;
    }
}

void TEncSearch::xSetIntraResultQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, TComYuv* reconYuv)
{
    uint32_t fullDepth = cu->getDepth(0) + trDepth;
    uint32_t trMode    = cu->getTransformIdx(absPartIdx);

    if (trMode == trDepth)
    {
        uint32_t trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUSize() >> fullDepth] + 2;
        uint32_t qtlayer    = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;

        //===== copy transform coefficients =====
        uint32_t numCoeffY    = (cu->getSlice()->getSPS()->getMaxCUSize() * cu->getSlice()->getSPS()->getMaxCUSize()) >> (fullDepth << 1);
        uint32_t numCoeffIncY = (cu->getSlice()->getSPS()->getMaxCUSize() * cu->getSlice()->getSPS()->getMaxCUSize()) >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1);
        TCoeff* coeffSrcY = m_qtTempCoeffY[qtlayer] + (numCoeffIncY * absPartIdx);
        TCoeff* coeffDestY = cu->getCoeffY()        + (numCoeffIncY * absPartIdx);
        ::memcpy(coeffDestY, coeffSrcY, sizeof(TCoeff) * numCoeffY);

        //===== copy reconstruction =====
        m_qtTempShortYuv[qtlayer].copyPartToPartLuma(reconYuv, absPartIdx, 1 << trSizeLog2, 1 << trSizeLog2);
    }
    else
    {
        uint32_t numQPart = cu->getPic()->getNumPartInCU() >> ((fullDepth + 1) << 1);
        for (uint32_t part = 0; part < 4; part++)
        {
            xSetIntraResultQT(cu, trDepth + 1, absPartIdx + part * numQPart, reconYuv);
        }
    }
}

void TEncSearch::xStoreIntraResultQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx)
{
    uint32_t fullMode = cu->getDepth(0) + trDepth;

    uint32_t trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUSize() >> fullMode] + 2;
    uint32_t qtlayer    = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;

    //===== copy transform coefficients =====
    uint32_t numCoeffY    = (cu->getSlice()->getSPS()->getMaxCUSize() * cu->getSlice()->getSPS()->getMaxCUSize()) >> (fullMode << 1);
    uint32_t numCoeffIncY = (cu->getSlice()->getSPS()->getMaxCUSize() * cu->getSlice()->getSPS()->getMaxCUSize()) >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1);
    TCoeff* coeffSrcY = m_qtTempCoeffY[qtlayer] + (numCoeffIncY * absPartIdx);
    TCoeff* coeffDstY = m_qtTempTUCoeffY;
    ::memcpy(coeffDstY, coeffSrcY, sizeof(TCoeff) * numCoeffY);

    //===== copy reconstruction =====
    m_qtTempShortYuv[qtlayer].copyPartToPartLuma(&m_qtTempTransformSkipYuv, absPartIdx, 1 << trSizeLog2, 1 << trSizeLog2);
}

void TEncSearch::xLoadIntraResultQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx)
{
    uint32_t fullDepth = cu->getDepth(0) + trDepth;

    uint32_t trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUSize() >> fullDepth] + 2;
    uint32_t qtlayer    = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;

    //===== copy transform coefficients =====
    uint32_t numCoeffY    = (cu->getSlice()->getSPS()->getMaxCUSize() * cu->getSlice()->getSPS()->getMaxCUSize()) >> (fullDepth << 1);
    uint32_t numCoeffIncY = (cu->getSlice()->getSPS()->getMaxCUSize() * cu->getSlice()->getSPS()->getMaxCUSize()) >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1);
    TCoeff* coeffDstY = m_qtTempCoeffY[qtlayer] + (numCoeffIncY * absPartIdx);
    TCoeff* coeffSrcY = m_qtTempTUCoeffY;
    ::memcpy(coeffDstY, coeffSrcY, sizeof(TCoeff) * numCoeffY);

    //===== copy reconstruction =====
    uint32_t trSize = 1 << trSizeLog2;
    m_qtTempTransformSkipYuv.copyPartToPartLuma(&m_qtTempShortYuv[qtlayer], absPartIdx, trSize);

    uint32_t   zOrder           = cu->getZorderIdxInCU() + absPartIdx;
    pixel*     reconIPred       = cu->getPic()->getPicYuvRec()->getLumaAddr(cu->getAddr(), zOrder);
    uint32_t   reconIPredStride = cu->getPic()->getPicYuvRec()->getStride();
    int16_t*   reconQt          = m_qtTempShortYuv[qtlayer].getLumaAddr(absPartIdx);
    primitives.blockcpy_ps(trSize, trSize, reconIPred, reconIPredStride, reconQt, MAX_CU_SIZE);
    assert(m_qtTempShortYuv[qtlayer].m_width == MAX_CU_SIZE);
}

void TEncSearch::xStoreIntraResultChromaQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, uint32_t stateU0V1Both2)
{
    uint32_t fullDepth = cu->getDepth(0) + trDepth;
    uint32_t trMode    = cu->getTransformIdx(absPartIdx);

    if (trMode == trDepth)
    {
        uint32_t trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUSize() >> fullDepth] + 2;
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
        uint32_t numCoeffC = (cu->getSlice()->getSPS()->getMaxCUSize() * cu->getSlice()->getSPS()->getMaxCUSize()) >> (fullDepth << 1);
        if (!bChromaSame)
        {
            numCoeffC >>= 2;
        }
        uint32_t numCoeffIncC = (cu->getSlice()->getSPS()->getMaxCUSize() * cu->getSlice()->getSPS()->getMaxCUSize()) >> ((cu->getSlice()->getSPS()->getMaxCUDepth() << 1) + 2);
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
        uint32_t lumaSize = 1 << (bChromaSame ? trSizeLog2 + 1 : trSizeLog2);
        m_qtTempShortYuv[qtlayer].copyPartToPartYuvChroma(&m_qtTempTransformSkipYuv, absPartIdx, lumaSize, stateU0V1Both2);
    }
}

void TEncSearch::xLoadIntraResultChromaQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, uint32_t stateU0V1Both2)
{
    uint32_t fullDepth = cu->getDepth(0) + trDepth;
    uint32_t trMode    = cu->getTransformIdx(absPartIdx);

    if (trMode == trDepth)
    {
        uint32_t trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUSize() >> fullDepth] + 2;
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
        uint32_t numCoeffC = (cu->getSlice()->getSPS()->getMaxCUSize() * cu->getSlice()->getSPS()->getMaxCUSize()) >> (fullDepth << 1);
        if (!bChromaSame)
        {
            numCoeffC >>= 2;
        }
        uint32_t numCoeffIncC = (cu->getSlice()->getSPS()->getMaxCUSize() * cu->getSlice()->getSPS()->getMaxCUSize()) >> ((cu->getSlice()->getSPS()->getMaxCUDepth() << 1) + 2);

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
        uint32_t lumaSize = 1 << (bChromaSame ? trSizeLog2 + 1 : trSizeLog2);
        m_qtTempTransformSkipYuv.copyPartToPartChroma(&m_qtTempShortYuv[qtlayer], absPartIdx, lumaSize, stateU0V1Both2);

        uint32_t zorder           = cu->getZorderIdxInCU() + absPartIdx;
        uint32_t width            = cu->getCUSize(0) >> (trDepth + 1);
        uint32_t height           = cu->getCUSize(0) >> (trDepth + 1);
        uint32_t reconQtStride    = m_qtTempShortYuv[qtlayer].m_cwidth;
        uint32_t reconIPredStride = cu->getPic()->getPicYuvRec()->getCStride();

        if (stateU0V1Both2 == 0 || stateU0V1Both2 == 2)
        {
            Pel* reconIPred = cu->getPic()->getPicYuvRec()->getCbAddr(cu->getAddr(), zorder);
            int16_t* reconQt  = m_qtTempShortYuv[qtlayer].getCbAddr(absPartIdx);
            primitives.blockcpy_ps(width, height, reconIPred, reconIPredStride, reconQt, reconQtStride);
        }
        if (stateU0V1Both2 == 1 || stateU0V1Both2 == 2)
        {
            Pel* reconIPred = cu->getPic()->getPicYuvRec()->getCrAddr(cu->getAddr(), zorder);
            int16_t* reconQt  = m_qtTempShortYuv[qtlayer].getCrAddr(absPartIdx);
            primitives.blockcpy_ps(width, height, reconIPred, reconIPredStride, reconQt, reconQtStride);
        }
    }
}

void TEncSearch::xRecurIntraChromaCodingQT(TComDataCU* cu,
                                           uint32_t    trDepth,
                                           uint32_t    absPartIdx,
                                           TComYuv*    fencYuv,
                                           TComYuv*    predYuv,
                                           ShortYuv*  resiYuv,
                                           uint32_t&   outDist)
{
    uint32_t fullDepth = cu->getDepth(0) + trDepth;
    uint32_t trMode    = cu->getTransformIdx(absPartIdx);

    if (trMode == trDepth)
    {
        bool checkTransformSkip = cu->getSlice()->getPPS()->getUseTransformSkip();
        uint32_t trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUSize() >> fullDepth] + 2;

        uint32_t actualTrDepth = trDepth;
        if ((trSizeLog2 == 2) && !(cu->getChromaFormat() == CHROMA_444))
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
        if (m_cfg->param->bEnableTSkipFast)
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
                uint64_t singleCost     = MAX_INT64;
                int      bestModeId     = 0;
                uint32_t singleDistC    = 0;
                uint32_t singleCbfC     = 0;
                uint32_t singleDistCTmp = 0;
                uint64_t singleCostTmp  = 0;
                uint32_t singleCbfCTmp  = 0;

                const int firstCheckId  = 0;

                for (int chromaModeId = firstCheckId; chromaModeId < 2; chromaModeId++)
                {
                    cu->setTransformSkipSubParts(chromaModeId, (TextType)(chromaId + TEXT_CHROMA_U), absPartIdx, cu->getDepth(0) + actualTrDepth);
                    singleDistCTmp = 0;
                    bool bReusePred = chromaModeId != firstCheckId;
                    xIntraCodingChromaBlk(cu, trDepth, absPartIdx, fencYuv, predYuv, resiYuv, singleDistCTmp, chromaId, bReusePred);
                    singleCbfCTmp = cu->getCbf(absPartIdx, (TextType)(chromaId + TEXT_CHROMA_U), trDepth);

                    if (chromaModeId == 1 && singleCbfCTmp == 0)
                    {
                        //In order not to code TS flag when cbf is zero, the case for TS with cbf being zero is forbidden.
                        singleCostTmp = MAX_INT64;
                    }
                    else
                    {
                        uint32_t bitsTmp = xGetIntraBitsQTChroma(cu, trDepth, absPartIdx, chromaId + TEXT_CHROMA_U);
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
                    cu->setCbfSubParts(singleCbfC << trDepth, (TextType)(chromaId + TEXT_CHROMA_U), absPartIdx, cu->getDepth(0) + actualTrDepth);
                    m_rdGoOnSbacCoder->load(m_rdSbacCoders[fullDepth][CI_TEMP_BEST]);
                }
                cu->setTransformSkipSubParts(bestModeId, (TextType)(chromaId + TEXT_CHROMA_U), absPartIdx, cu->getDepth(0) + actualTrDepth);
                outDist += singleDistC;

                if (chromaId == 0)
                {
                    m_rdGoOnSbacCoder->store(m_rdSbacCoders[fullDepth][CI_QT_TRAFO_ROOT]);
                }
            }
        }
        else
        {
            cu->setTransformSkipSubParts(0, TEXT_CHROMA_U, absPartIdx, cu->getDepth(0) + actualTrDepth);
            cu->setTransformSkipSubParts(0, TEXT_CHROMA_V, absPartIdx, cu->getDepth(0) + actualTrDepth);
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
    int      chFmt     = cu->getChromaFormat();

    if (trMode == trDepth)
    {
        uint32_t trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUSize() >> fullDepth] + 2;
        uint32_t qtlayer    = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;

        bool bChromaSame  = false;
        if ((trSizeLog2 == 2) && !(chFmt == CHROMA_444))
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
        uint32_t numCoeffC = (cu->getSlice()->getSPS()->getMaxCUSize() * cu->getSlice()->getSPS()->getMaxCUSize()) >> (fullDepth << 1);
        if (!bChromaSame)
        {
            numCoeffC = ((cu->getSlice()->getSPS()->getMaxCUSize() >> m_hChromaShift) * (cu->getSlice()->getSPS()->getMaxCUSize() >> m_vChromaShift)) >> (fullDepth << 1);
        }

        uint32_t numCoeffIncC = ((cu->getSlice()->getSPS()->getMaxCUSize() >> m_hChromaShift) * (cu->getSlice()->getSPS()->getMaxCUSize() >> m_vChromaShift)) >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1);

        TCoeff* coeffSrcU = m_qtTempCoeffCb[qtlayer] + (numCoeffIncC * absPartIdx);
        TCoeff* coeffSrcV = m_qtTempCoeffCr[qtlayer] + (numCoeffIncC * absPartIdx);
        TCoeff* coeffDstU = cu->getCoeffCb()         + (numCoeffIncC * absPartIdx);
        TCoeff* coeffDstV = cu->getCoeffCr()         + (numCoeffIncC * absPartIdx);
        ::memcpy(coeffDstU, coeffSrcU, sizeof(TCoeff) * numCoeffC);
        ::memcpy(coeffDstV, coeffSrcV, sizeof(TCoeff) * numCoeffC);

        //===== copy reconstruction =====
        m_qtTempShortYuv[qtlayer].copyPartToPartChroma(reconYuv, absPartIdx, 1 << trSizeLog2, bChromaSame);
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

void TEncSearch::residualQTIntrachroma(TComDataCU* cu,
                                       uint32_t    trDepth,
                                       uint32_t    absPartIdx,
                                       TComYuv*    fencYuv,
                                       TComYuv*    predYuv,
                                       ShortYuv*  resiYuv,
                                       TComYuv*    reconYuv)
{
    uint32_t fullDepth = cu->getDepth(0) + trDepth;
    uint32_t trMode    = cu->getTransformIdx(absPartIdx);
    int      chFmt     = cu->getChromaFormat();

    if (trMode == trDepth)
    {
        uint32_t trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUSize() >> fullDepth] + 2;
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

        cu->setTransformSkipSubParts(0, TEXT_CHROMA_U, absPartIdx, cu->getDepth(0) +  actualTrDepth);
        cu->setTransformSkipSubParts(0, TEXT_CHROMA_V, absPartIdx, cu->getDepth(0) +  actualTrDepth);
        uint32_t width          = cu->getCUSize(0) >> (trDepth + m_hChromaShift);
        uint32_t height         = cu->getCUSize(0) >> (trDepth + m_vChromaShift);
        uint32_t stride         = fencYuv->getCStride();

        for (uint32_t chromaId = 0; chromaId < 2; chromaId++)
        {
            TextType ttype          = (chromaId > 0 ? TEXT_CHROMA_V : TEXT_CHROMA_U);
            uint32_t chromaPredMode = cu->getChromaIntraDir(absPartIdx);
            Pel*     fenc           = (chromaId > 0 ? fencYuv->getCrAddr(absPartIdx) : fencYuv->getCbAddr(absPartIdx));
            Pel*     pred           = (chromaId > 0 ? predYuv->getCrAddr(absPartIdx) : predYuv->getCbAddr(absPartIdx));
            int16_t* residual       = (chromaId > 0 ? resiYuv->getCrAddr(absPartIdx) : resiYuv->getCbAddr(absPartIdx));
            Pel*     recon          = (chromaId > 0 ? reconYuv->getCrAddr(absPartIdx) : reconYuv->getCbAddr(absPartIdx));
            uint32_t numCoeffPerInc = (cu->getSlice()->getSPS()->getMaxCUSize() * cu->getSlice()->getSPS()->getMaxCUSize() >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1)) >> 2;
            TCoeff*  coeff          = (chromaId > 0 ? cu->getCoeffCr() : cu->getCoeffCb()) + numCoeffPerInc * absPartIdx;

            uint32_t zorder           = cu->getZorderIdxInCU() + absPartIdx;
            Pel*     reconIPred       = (chromaId > 0 ? cu->getPic()->getPicYuvRec()->getCrAddr(cu->getAddr(), zorder) : cu->getPic()->getPicYuvRec()->getCbAddr(cu->getAddr(), zorder));
            uint32_t reconIPredStride = cu->getPic()->getPicYuvRec()->getCStride();
            bool     useTransformSkipChroma = cu->getTransformSkip(absPartIdx, ttype);
            //===== update chroma mode =====
            if (chromaPredMode == DM_CHROMA_IDX)
            {
                chromaPredMode = cu->getLumaIntraDir(0);
            }
            //===== init availability pattern =====
            cu->getPattern()->initAdiPatternChroma(cu, absPartIdx, trDepth, m_predBuf, m_predBufStride, m_predBufHeight, chromaId);
            Pel* chromaPred = TComPattern::getAdiChromaBuf(chromaId, height, m_predBuf);

            //===== get prediction signal =====
            predIntraChromaAng(chromaPred, chromaPredMode, pred, stride, width, height, chFmt);

            //===== get residual signal =====
            assert(!((uint32_t)(size_t)fenc & (width - 1)));
            assert(!((uint32_t)(size_t)pred & (width - 1)));
            assert(!((uint32_t)(size_t)residual & (width - 1)));
            int size = g_convertToBit[width];
            primitives.calcresidual[size](fenc, pred, residual, stride);

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
            m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset, chFmt);

            m_trQuant->selectLambda(TEXT_CHROMA);

            absSum = m_trQuant->transformNxN(cu, residual, stride, coeff, width, ttype, absPartIdx, &lastPos, useTransformSkipChroma);

            //--- set coded block flag ---
            cu->setCbfSubParts((absSum ? 1 : 0) << trDepth, ttype, absPartIdx, cu->getDepth(0) + trDepth);

            //--- inverse transform ---
            if (absSum)
            {
                int scalingListType = 0 + ttype;
                assert(scalingListType < 6);
                m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdx), REG_DCT, residual, stride, coeff, width, scalingListType, useTransformSkipChroma, lastPos);
            }
            else
            {
                int16_t* resiTmp = residual;
                memset(coeff, 0, sizeof(TCoeff) * width * height);
                primitives.blockfill_s[size](resiTmp, stride, 0);
            }

            //===== reconstruction =====
            assert(((uint32_t)(size_t)residual & (width - 1)) == 0);
            assert(width <= 32);
            int part = partitionFromSizes(cu->getCUSize(0) >> (trDepth), cu->getCUSize(0) >> (trDepth));
            primitives.chroma[m_cfg->param->internalCsp].add_ps[part](recon, stride, pred, residual, stride, stride);
            primitives.chroma[m_cfg->param->internalCsp].copy_pp[part](reconIPred, reconIPredStride, recon, stride);
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
            residualQTIntrachroma(cu, trDepth + 1, absPartIdxSub, fencYuv, predYuv, resiYuv, reconYuv);
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

void TEncSearch::estIntraPredQT(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, ShortYuv* resiYuv, TComYuv* reconYuv)
{
    uint32_t depth        = cu->getDepth(0);
    uint32_t initTrDepth  = cu->getPartitionSize(0) == SIZE_2Nx2N ? 0 : 1;
    uint32_t numPU        = 1 << (2 * initTrDepth);
    uint32_t puSize       = cu->getCUSize(0) >> initTrDepth;
    uint32_t qNumParts    = cu->getTotalNumPart() >> 2;
    uint32_t qPartNum     = cu->getPic()->getNumPartInCU() >> ((depth + initTrDepth) << 1);
    uint32_t overallDistY = 0;
    uint32_t candNum;
    uint64_t candCostList[FAST_UDI_MAX_RDMODE_NUM];
    uint32_t puSizeIdx    = g_convertToBit[puSize]; // log2(puSize) - 2
    static const UChar intraModeNumFast[] = { 8, 8, 3, 3, 3 }; // 4x4, 8x8, 16x16, 32x32, 64x64

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
        // Reference sample smoothing
        cu->getPattern()->initAdiPattern(cu, partOffset, initTrDepth, m_predBuf, m_predBufStride, m_predBufHeight, m_refAbove, m_refLeft, m_refAboveFlt, m_refLeftFlt);

        //===== determine set of modes to be tested (using prediction signal only) =====
        const int numModesAvailable = 35; //total number of Intra modes
        Pel* fenc   = fencYuv->getLumaAddr(pu, puSize);
        uint32_t stride = predYuv->getStride();
        uint32_t rdModeList[FAST_UDI_MAX_RDMODE_NUM];
        int numModesForFullRD = intraModeNumFast[puSizeIdx];

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

            Pel *above         = m_refAbove    + puSize - 1;
            Pel *aboveFiltered = m_refAboveFlt + puSize - 1;
            Pel *left          = m_refLeft     + puSize - 1;
            Pel *leftFiltered  = m_refLeftFlt  + puSize - 1;

            // 33 Angle modes once
            ALIGN_VAR_32(Pel, buf_trans[32 * 32]);
            ALIGN_VAR_32(Pel, tmp[33 * 32 * 32]);
            int scaleSize = puSize;
            int scaleStride = stride;
            int costShift = 0;

            if (puSize > 32)
            {
                // origin is 64x64, we scale to 32x32 and setup required parameters
                ALIGN_VAR_32(Pel, bufScale[32 * 32]);
                primitives.scale2D_64to32(bufScale, fenc, stride);
                fenc = bufScale;

                // reserve space in case primitives need to store data in above
                // or left buffers
                Pel _above[4 * 32 + 1];
                Pel _left[4 * 32 + 1];
                Pel *aboveScale  = _above + 2 * 32;
                Pel *leftScale   = _left + 2 * 32;
                aboveScale[0] = leftScale[0] = above[0];
                primitives.scale1D_128to64(aboveScale + 1, above + 1, 0);
                primitives.scale1D_128to64(leftScale + 1, left + 1, 0);

                scaleSize = 32;
                scaleStride = 32;
                costShift = 2;

                // Filtered and Unfiltered refAbove and refLeft pointing to above and left.
                above         = aboveScale;
                left          = leftScale;
                aboveFiltered = aboveScale;
                leftFiltered  = leftScale;
            }

            int log2SizeMinus2 = g_convertToBit[scaleSize];
            pixelcmp_t sa8d = primitives.sa8d[log2SizeMinus2];

            // DC
            primitives.intra_pred[log2SizeMinus2][DC_IDX](tmp, scaleStride, left, above, 0, (scaleSize <= 16));
            modeCosts[DC_IDX] = sa8d(fenc, scaleStride, tmp, scaleStride) << costShift;

            Pel *abovePlanar   = above;
            Pel *leftPlanar    = left;

            if (puSize >= 8 && puSize <= 32)
            {
                abovePlanar = aboveFiltered;
                leftPlanar  = leftFiltered;
            }

            // PLANAR
            primitives.intra_pred[log2SizeMinus2][PLANAR_IDX](tmp, scaleStride, leftPlanar, abovePlanar, 0, 0);
            modeCosts[PLANAR_IDX] = sa8d(fenc, scaleStride, tmp, scaleStride) << costShift;

            // Transpose NxN
            primitives.transpose[log2SizeMinus2](buf_trans, fenc, scaleStride);

            primitives.intra_pred_allangs[log2SizeMinus2](tmp, above, left, aboveFiltered, leftFiltered, (scaleSize <= 16));

            for (uint32_t mode = 2; mode < numModesAvailable; mode++)
            {
                bool modeHor = (mode < 18);
                Pel *cmp = (modeHor ? buf_trans : fenc);
                intptr_t srcStride = (modeHor ? scaleSize : scaleStride);
                modeCosts[mode] = sa8d(cmp, srcStride, &tmp[(mode - 2) * (scaleSize * scaleSize)], scaleSize) << costShift;
            }

            // Find N least cost modes. N = numModesForFullRD
            for (uint32_t mode = 0; mode < numModesAvailable; mode++)
            {
                uint32_t sad = modeCosts[mode];
                uint32_t bits = xModeBitsIntra(cu, mode, partOffset, depth, initTrDepth);
                uint64_t cost = m_rdCost->calcRdSADCost(sad, bits);
                candNum += xUpdateCandList(mode, cost, numModesForFullRD, rdModeList, candCostList);
            }

            int preds[3];
            int numCand = cu->getIntraDirLumaPredictor(partOffset, preds);

            for (int j = 0; j < numCand; j++)
            {
                bool mostProbableModeIncluded = false;
                int mostProbableMode = preds[j];

                for (int i = 0; i < numModesForFullRD; i++)
                {
                    if (mostProbableMode == rdModeList[i])
                    {
                        mostProbableModeIncluded = true;
                        break;
                    }
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
        uint32_t bestPUMode  = 0;
        uint32_t bestPUDistY = 0;
        uint64_t bestPUCost  = MAX_INT64;
        for (uint32_t mode = 0; mode < numModesForFullRD; mode++)
        {
            // set luma prediction mode
            uint32_t origMode = rdModeList[mode];

            cu->setLumaIntraDirSubParts(origMode, partOffset, depth + initTrDepth);

            // set context models
            m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_CURR_BEST]);

            // determine residual for partition
            uint32_t puDistY = 0;
            uint64_t puCost  = 0;
            xRecurIntraCodingQT(cu, initTrDepth, partOffset, fencYuv, predYuv, resiYuv, puDistY, true, puCost);

            // check r-d cost
            if (puCost < bestPUCost)
            {
                bestPUMode  = origMode;
                bestPUDistY = puDistY;
                bestPUCost  = puCost;

                xSetIntraResultQT(cu, initTrDepth, partOffset, reconYuv);

                ::memcpy(m_qtTempTrIdx,  cu->getTransformIdx()     + partOffset, qPartNum * sizeof(UChar));
                ::memcpy(m_qtTempCbf[0], cu->getCbf(TEXT_LUMA)     + partOffset, qPartNum * sizeof(UChar));
                ::memcpy(m_qtTempTransformSkipFlag[0], cu->getTransformSkip(TEXT_LUMA)     + partOffset, qPartNum * sizeof(UChar));
            }
        } // Mode loop

        {
            uint32_t origMode = bestPUMode;

            cu->setLumaIntraDirSubParts(origMode, partOffset, depth + initTrDepth);

            // set context models
            m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_CURR_BEST]);

            // determine residual for partition
            uint32_t puDistY = 0;
            uint64_t puCost  = 0;
            xRecurIntraCodingQT(cu, initTrDepth, partOffset, fencYuv, predYuv, resiYuv, puDistY, false, puCost);

            // check r-d cost
            if (puCost < bestPUCost)
            {
                bestPUMode  = origMode;
                bestPUDistY = puDistY;

                xSetIntraResultQT(cu, initTrDepth, partOffset, reconYuv);

                ::memcpy(m_qtTempTrIdx,  cu->getTransformIdx()     + partOffset, qPartNum * sizeof(UChar));
                ::memcpy(m_qtTempCbf[0], cu->getCbf(TEXT_LUMA)     + partOffset, qPartNum * sizeof(UChar));
                ::memcpy(m_qtTempTransformSkipFlag[0], cu->getTransformSkip(TEXT_LUMA)     + partOffset, qPartNum * sizeof(UChar));
            }
        } // Mode loop

        //--- update overall distortion ---
        overallDistY += bestPUDistY;

        //--- update transform index and cbf ---
        ::memcpy(cu->getTransformIdx()     + partOffset, m_qtTempTrIdx,  qPartNum * sizeof(UChar));
        ::memcpy(cu->getCbf(TEXT_LUMA)     + partOffset, m_qtTempCbf[0], qPartNum * sizeof(UChar));
        ::memcpy(cu->getTransformSkip(TEXT_LUMA)     + partOffset, m_qtTempTransformSkipFlag[0], qPartNum * sizeof(UChar));
        //--- set reconstruction for next intra prediction blocks ---
        if (pu != numPU - 1)
        {
            uint32_t zorder      = cu->getZorderIdxInCU() + partOffset;
            int      part        = partitionFromSizes(puSize, puSize);
            Pel*     dst         = cu->getPic()->getPicYuvRec()->getLumaAddr(cu->getAddr(), zorder);
            uint32_t dststride   = cu->getPic()->getPicYuvRec()->getStride();
            Pel*     src         = reconYuv->getLumaAddr(partOffset);
            uint32_t srcstride   = reconYuv->getStride();
            primitives.luma_copy_pp[part](dst, dststride, src, srcstride);
        }

        //=== update PU data ====
        cu->setLumaIntraDirSubParts(bestPUMode, partOffset, depth + initTrDepth);
        cu->copyToPic(depth, pu, initTrDepth);
    } // PU loop

    if (numPU > 1)
    { // set Cbf for all blocks
        uint32_t combCbfY = 0;
        uint32_t partIdx  = 0;
        for (uint32_t part = 0; part < 4; part++, partIdx += qNumParts)
        {
            combCbfY |= cu->getCbf(partIdx, TEXT_LUMA,     1);
        }

        for (uint32_t offs = 0; offs < 4 * qNumParts; offs++)
        {
            cu->getCbf(TEXT_LUMA)[offs] |= combCbfY;
        }
    }

    //===== reset context models =====
    m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_CURR_BEST]);

    //===== set distortion (rate and r-d costs are determined later) =====
    cu->m_totalDistortion = overallDistY;
}

void TEncSearch::getBestIntraModeChroma(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv)
{
    uint32_t depth     = cu->getDepth(0);
    uint32_t trDepth = 0;
    uint32_t absPartIdx = 0;
    uint32_t bestMode  = 0;
    uint64_t bestCost  = MAX_INT64;
    //----- init mode list -----
    uint32_t minMode = 0;
    uint32_t maxMode = NUM_CHROMA_MODE;
    uint32_t modeList[NUM_CHROMA_MODE];

    uint32_t width          = cu->getCUSize(0) >> (trDepth + m_hChromaShift);
    uint32_t height         = cu->getCUSize(0) >> (trDepth + m_vChromaShift);
    int      chFmt          = cu->getChromaFormat();
    uint32_t stride         = fencYuv->getCStride();
    int scaleWidth = width;
    int scaleStride = stride;
    int costShift = 0;

    if (width > 32)
    {
        scaleWidth = 32;
        scaleStride = 32;
        costShift = 2;
    }

    cu->getPattern()->initAdiPatternChroma(cu, absPartIdx, trDepth, m_predBuf, m_predBufStride, m_predBufHeight, 0);
    cu->getPattern()->initAdiPatternChroma(cu, absPartIdx, trDepth, m_predBuf, m_predBufStride, m_predBufHeight, 1);
    cu->getAllowedChromaDir(0, modeList);
    //----- check chroma modes -----
    for (uint32_t mode = minMode; mode < maxMode; mode++)
    {
        uint64_t cost = 0;
        for (int chromaId = 0; chromaId < 2; chromaId++)
        {
            int sad = 0;
            uint32_t chromaPredMode = modeList[mode];
            if (chromaPredMode == DM_CHROMA_IDX)
                chromaPredMode = cu->getLumaIntraDir(0);
            Pel*     fenc           = (chromaId > 0 ? fencYuv->getCrAddr(absPartIdx) : fencYuv->getCbAddr(absPartIdx));
            Pel*     pred           = (chromaId > 0 ? predYuv->getCrAddr(absPartIdx) : predYuv->getCbAddr(absPartIdx));

            Pel* chromaPred = TComPattern::getAdiChromaBuf(chromaId, height, m_predBuf);

            //===== get prediction signal =====
            predIntraChromaAng(chromaPred, chromaPredMode, pred, stride, width, height, chFmt);
            int log2SizeMinus2 = g_convertToBit[scaleWidth];
            pixelcmp_t sa8d = primitives.sa8d[log2SizeMinus2];
            sad = sa8d(fenc, scaleStride, pred, scaleStride) << costShift;
            cost += sad;
        }

        //----- compare -----
        if (cost < bestCost)
        {
            bestCost = cost;
            bestMode = modeList[mode];
        }
    }

    cu->setChromIntraDirSubParts(bestMode, 0, depth);
}

bool TEncSearch::isNextSection()
{
    if (m_splitMode == DONT_SPLIT)
    {
        m_section++;
        return false;
    }
    else
    {
        m_absPartIdxTURelCU += m_absPartIdxStep;

        m_section++;
        return m_section < (1 << m_splitMode);
    }
}

bool TEncSearch::isLastSection()
{
    return (m_section + 1) >= (1 << m_splitMode);
}

void TEncSearch::estIntraPredChromaQT(TComDataCU* cu,
                                      TComYuv*    fencYuv,
                                      TComYuv*    predYuv,
                                      ShortYuv*   resiYuv,
                                      TComYuv*    reconYuv)
{
    uint32_t depth              = cu->getDepth(0);
    uint32_t initTrDepth        = (cu->getPartitionSize(0) != SIZE_2Nx2N) && (cu->getChromaFormat() == CHROMA_444 ? 1 : 0);

    m_splitMode                 = (initTrDepth == 0) ? DONT_SPLIT : QUAD_SPLIT;
    m_absPartIdxStep            = (cu->getPic()->getNumPartInCU() >> (depth << 1)) >> partIdxStepShift[m_splitMode];
    m_partOffset                = 0;
    m_section                   = 0;
    m_absPartIdxTURelCU         = 0;

    do
    {
        uint32_t bestMode           = 0;
        uint32_t bestDist           = 0;
        uint64_t bestCost           = MAX_INT64;

        //----- init mode list -----
        uint32_t minMode = 0;
        uint32_t maxMode = NUM_CHROMA_MODE;
        uint32_t modeList[NUM_CHROMA_MODE];

        m_partOffset = m_absPartIdxTURelCU;

        cu->getAllowedChromaDir(m_partOffset, modeList);

        //----- check chroma modes -----
        for (uint32_t mode = minMode; mode < maxMode; mode++)
        {
            //----- restore context models -----
            m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_CURR_BEST]);

            //----- chroma coding -----
            uint32_t dist = 0;

            cu->setChromIntraDirSubParts(modeList[mode], m_partOffset, depth + initTrDepth);

            xRecurIntraChromaCodingQT(cu, initTrDepth, m_absPartIdxTURelCU, fencYuv, predYuv, resiYuv, dist);

            if (cu->getSlice()->getPPS()->getUseTransformSkip())
            {
                m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_CURR_BEST]);
            }

            uint32_t bits = xGetIntraBitsQT(cu, initTrDepth, m_absPartIdxTURelCU, false, true);
            uint64_t cost = m_rdCost->calcRdCost(dist, bits);

            //----- compare -----
            if (cost < bestCost)
            {
                bestCost = cost;
                bestDist = dist;
                bestMode = modeList[mode];
                xSetIntraResultChromaQT(cu, initTrDepth, m_absPartIdxTURelCU, reconYuv);
                ::memcpy(m_qtTempCbf[1], cu->getCbf(TEXT_CHROMA_U) + m_partOffset, m_absPartIdxStep * sizeof(UChar));
                ::memcpy(m_qtTempCbf[2], cu->getCbf(TEXT_CHROMA_V) + m_partOffset, m_absPartIdxStep * sizeof(UChar));
                ::memcpy(m_qtTempTransformSkipFlag[1], cu->getTransformSkip(TEXT_CHROMA_U) + m_partOffset, m_absPartIdxStep * sizeof(UChar));
                ::memcpy(m_qtTempTransformSkipFlag[2], cu->getTransformSkip(TEXT_CHROMA_V) + m_partOffset, m_absPartIdxStep * sizeof(UChar));
            }
        }

        if (!isLastSection())
        {
            uint32_t compWidth   = (cu->getCUSize(0) >> m_hChromaShift)  >> initTrDepth;
            uint32_t compHeight  = (cu->getCUSize(0) >> m_vChromaShift) >> initTrDepth;
            uint32_t zorder      = cu->getZorderIdxInCU() + m_partOffset;
            Pel*     dst         = cu->getPic()->getPicYuvRec()->getCbAddr(cu->getAddr(), zorder);
            uint32_t dststride   = cu->getPic()->getPicYuvRec()->getCStride();
            Pel*     src         = reconYuv->getCbAddr(m_partOffset);
            uint32_t srcstride   = reconYuv->getCStride();

            primitives.blockcpy_pp(compWidth, compHeight, dst, dststride, src, srcstride);

            dst                 = cu->getPic()->getPicYuvRec()->getCrAddr(cu->getAddr(), zorder);
            src                 = reconYuv->getCrAddr(m_partOffset);
            primitives.blockcpy_pp(compWidth, compHeight, dst, dststride, src, srcstride);
        }

        //----- set data -----
        ::memcpy(cu->getCbf(TEXT_CHROMA_U) + m_partOffset, m_qtTempCbf[1], m_absPartIdxStep * sizeof(UChar));
        ::memcpy(cu->getCbf(TEXT_CHROMA_V) + m_partOffset, m_qtTempCbf[2], m_absPartIdxStep * sizeof(UChar));
        ::memcpy(cu->getTransformSkip(TEXT_CHROMA_U) + m_partOffset, m_qtTempTransformSkipFlag[1], m_absPartIdxStep * sizeof(UChar));
        ::memcpy(cu->getTransformSkip(TEXT_CHROMA_V) + m_partOffset, m_qtTempTransformSkipFlag[2], m_absPartIdxStep * sizeof(UChar));
        cu->setChromIntraDirSubParts(bestMode, m_partOffset, depth + initTrDepth);
        cu->m_totalDistortion += bestDist;
    }
    while (isNextSection());

    //----- restore context models -----
    if (initTrDepth != 0)
    {   // set Cbf for all blocks
        uint32_t uiCombCbfU = 0;
        uint32_t uiCombCbfV = 0;
        uint32_t uiPartIdx  = 0;
        for (uint32_t uiPart = 0; uiPart < 4; uiPart++, uiPartIdx += m_absPartIdxStep)
        {
            uiCombCbfU |= cu->getCbf(uiPartIdx, TEXT_CHROMA_U, 1);
            uiCombCbfV |= cu->getCbf(uiPartIdx, TEXT_CHROMA_V, 1);
        }

        for (uint32_t uiOffs = 0; uiOffs < 4 * m_absPartIdxStep; uiOffs++)
        {
            cu->getCbf(TEXT_CHROMA_U)[uiOffs] |= uiCombCbfU;
            cu->getCbf(TEXT_CHROMA_V)[uiOffs] |= uiCombCbfV;
        }
    }
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
void TEncSearch::IPCMSearch(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, ShortYuv* resiYuv, TComYuv* reconYuv)
{
    uint32_t depth      = cu->getDepth(0);
    uint32_t width      = cu->getCUSize(0);
    uint32_t height     = cu->getCUSize(0);
    uint32_t stride     = predYuv->getStride();
    uint32_t strideC    = predYuv->getCStride();
    uint32_t widthC     = width  >> 1;
    uint32_t heightC    = height >> 1;
    uint32_t distortion = 0;
    uint32_t bits;
    uint64_t cost;

    uint32_t absPartIdx = 0;
    uint32_t minCoeffSize = cu->getPic()->getMinCUSize() * cu->getPic()->getMinCUSize();
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
void TEncSearch::xMergeEstimation(TComDataCU* cu, int puIdx, uint32_t& interDir, TComMvField* mvField, uint32_t& mergeIndex, uint32_t& outCost, uint32_t& outbits, TComMvField* mvFieldNeighbours, UChar* interDirNeighbours, int& numValidMergeCand)
{
    uint32_t absPartIdx = 0;
    int width = 0;
    int height = 0;

    cu->getPartIndexAndSize(puIdx, absPartIdx, width, height);
    uint32_t depth = cu->getDepth(absPartIdx);
    PartSize partSize = cu->getPartitionSize(0);
    if (cu->getSlice()->getPPS()->getLog2ParallelMergeLevelMinus2() && partSize != SIZE_2Nx2N && cu->getCUSize(0) <= 8)
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
    xRestrictBipredMergeCand(cu, mvFieldNeighbours, interDirNeighbours, numValidMergeCand);

    outCost = MAX_UINT;
    for (uint32_t mergeCand = 0; mergeCand < numValidMergeCand; ++mergeCand)
    {
        /* Prevent TMVP candidates from using unavailable reference pixels */
        if (m_cfg->param->frameNumThreads > 1 &&
            (mvFieldNeighbours[0 + 2 * mergeCand].mv.y >= (m_cfg->param->searchRange + 1) * 4 ||
             mvFieldNeighbours[1 + 2 * mergeCand].mv.y >= (m_cfg->param->searchRange + 1) * 4))
        {
            continue;
        }

        cu->getCUMvField(REF_PIC_LIST_0)->m_mv[absPartIdx] = mvFieldNeighbours[0 + 2 * mergeCand].mv;
        cu->getCUMvField(REF_PIC_LIST_0)->m_refIdx[absPartIdx] = mvFieldNeighbours[0 + 2 * mergeCand].refIdx;
        cu->getCUMvField(REF_PIC_LIST_1)->m_mv[absPartIdx] = mvFieldNeighbours[1 + 2 * mergeCand].mv;
        cu->getCUMvField(REF_PIC_LIST_1)->m_refIdx[absPartIdx] = mvFieldNeighbours[1 + 2 * mergeCand].refIdx;

        uint32_t costCand = xGetInterPredictionError(cu, puIdx);
        uint32_t bitsCand = mergeCand + 1;
        if (mergeCand == m_cfg->param->maxNumMergeCand - 1)
        {
            bitsCand--;
        }
        costCand = costCand + m_rdCost->getCost(bitsCand);
        if (costCand < outCost)
        {
            outCost = costCand;
            outbits = bitsCand;
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
void TEncSearch::xRestrictBipredMergeCand(TComDataCU* cu, TComMvField* mvFieldNeighbours, UChar* interDirNeighbours, int numValidMergeCand)
{
    if (cu->isBipredRestriction())
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
 * \param predYuv    - output buffer for motion compensation prediction
 * \param bMergeOnly - try merge predictions only, do not perform motion estimation
 * \param bChroma    - generate a chroma prediction
 * \returns true if predYuv was filled with a motion compensated prediction
 */
bool TEncSearch::predInterSearch(TComDataCU* cu, TComYuv* predYuv, bool bMergeOnly, bool bChroma)
{
    MV mvzero(0, 0);
    MV mv[2];
    MV mvBidir[2];
    MV mvTemp[2][MAX_NUM_REF];
    MV mvPred[2][MAX_NUM_REF];
    MV mvPredBi[2][MAX_NUM_REF];

    int mvpIdxBi[2][MAX_NUM_REF];
    int mvpIdx[2][MAX_NUM_REF];
    AMVPInfo amvpInfo[2][MAX_NUM_REF];

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

    int totalmebits = 0;

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

        // force ME for the smallest rect/AMP sizes (Why? the HM did this)
        if (cu->getCUSize(0) <= 8 || numPart != 2)
            bMergeOnly = false;

        if (!bMergeOnly)
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
                    xEstimateMvPredAMVP(cu, partIdx, list, idx, mvPred[list][idx], &amvpInfo[list][idx], &biPDistTemp);
                    mvpIdx[list][idx] = cu->getMVPIdx(list, partAddr);

                    bitsTemp += MVP_IDX_BITS;
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

                    xCheckBestMVP(&amvpInfo[list][idx], mvTemp[list][idx], mvPred[list][idx], mvpIdx[list][idx], bitsTemp, costTemp);

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
            if ((cu->getSlice()->isInterB()) && (cu->isBipredRestriction() == false))
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
                    xCheckBestMVP(&amvpInfo[0][refIdxBidir[0]], mvzero, mvpZero[0], mvpidxZero[0], bitsZero0, costZero);
                    mvpZero[1] = mvPredBi[1][refIdxBidir[1]];
                    mvpidxZero[1] = mvpIdxBi[1][refIdxBidir[1]];
                    xCheckBestMVP(&amvpInfo[1][refIdxBidir[1]], mvzero, mvpZero[1], mvpidxZero[1], bitsZero1, costZero);

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
        }

        //  Clear Motion Field
        cu->getCUMvField(REF_PIC_LIST_0)->setAllMvField(TComMvField(), partSize, partAddr, 0, partIdx);
        cu->getCUMvField(REF_PIC_LIST_1)->setAllMvField(TComMvField(), partSize, partAddr, 0, partIdx);

        uint32_t mebits = 0;
        // Set Motion Field_
        mv[1] = mvValidList1;
        refIdx[1] = refIdxValidList1;
        bits[1] = bitsValidList1;
        listCost[1] = costValidList1;

        if (!bMergeOnly)
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
                    cu->getCUMvField(REF_PIC_LIST_0)->setMvd(partAddr, mvtmp);
                }
                {
                    MV mvtmp = mvBidir[1] - mvPredBi[1][refIdxBidir[1]];
                    cu->getCUMvField(REF_PIC_LIST_1)->setMvd(partAddr, mvtmp);
                }

                cu->setInterDirSubParts(3, partAddr, partIdx, cu->getDepth(0));

                cu->setMVPIdx(REF_PIC_LIST_0, partAddr, mvpIdxBi[0][refIdxBidir[0]]);
                cu->setMVPIdx(REF_PIC_LIST_1, partAddr, mvpIdxBi[1][refIdxBidir[1]]);

                mebits = bits[2];
            }
            else if (listCost[0] <= listCost[1])
            {
                lastMode = 0;
                cu->getCUMvField(REF_PIC_LIST_0)->setAllMv(mv[0], partSize, partAddr, 0, partIdx);
                cu->getCUMvField(REF_PIC_LIST_0)->setAllRefIdx(refIdx[0], partSize, partAddr, 0, partIdx);
                {
                    MV mvtmp = mv[0] - mvPred[0][refIdx[0]];
                    cu->getCUMvField(REF_PIC_LIST_0)->setMvd(partAddr, mvtmp);
                }
                cu->setInterDirSubParts(1, partAddr, partIdx, cu->getDepth(0));

                cu->setMVPIdx(REF_PIC_LIST_0, partAddr, mvpIdx[0][refIdx[0]]);

                mebits = bits[0];
            }
            else
            {
                lastMode = 1;
                cu->getCUMvField(REF_PIC_LIST_1)->setAllMv(mv[1], partSize, partAddr, 0, partIdx);
                cu->getCUMvField(REF_PIC_LIST_1)->setAllRefIdx(refIdx[1], partSize, partAddr, 0, partIdx);
                {
                    MV mvtmp = mv[1] - mvPred[1][refIdx[1]];
                    cu->getCUMvField(REF_PIC_LIST_1)->setMvd(partAddr, mvtmp);
                }
                cu->setInterDirSubParts(2, partAddr, partIdx, cu->getDepth(0));

                cu->setMVPIdx(REF_PIC_LIST_1, partAddr, mvpIdx[1][refIdx[1]]);

                mebits = bits[1];
            }
        }

        if (cu->getPartitionSize(partAddr) != SIZE_2Nx2N)
        {
            uint32_t mrgInterDir = 0;
            TComMvField mrgMvField[2];

            uint32_t meInterDir = 0;
            TComMvField meMvField[2];

            // calculate ME cost
            uint32_t meError = MAX_UINT;
            uint32_t meCost = MAX_UINT;

            if (!bMergeOnly)
            {
                meError = xGetInterPredictionError(cu, partIdx);
                meCost = meError + m_rdCost->getCost(mebits);
            }

            // save ME result.
            meInterDir = cu->getInterDir(partAddr);
            cu->getMvField(cu, partAddr, REF_PIC_LIST_0, meMvField[0]);
            cu->getMvField(cu, partAddr, REF_PIC_LIST_1, meMvField[1]);

            // find Merge result
            uint32_t mrgIndex = UINT_MAX;
            uint32_t mrgCost = MAX_UINT;
            uint32_t mrgBits = 0;
            xMergeEstimation(cu, partIdx, mrgInterDir, mrgMvField, mrgIndex, mrgCost, mrgBits, mvFieldNeighbours, interDirNeighbours, numValidMergeCand);
            if (mrgCost == MAX_UINT && bMergeOnly)
            {
                /* No valid merge modes were found, there is no possible way to
                 * perform a valid motion compensation prediction, so early-exit */
                return false;
            }
            if (mrgCost < meCost)
            {
                // set Merge result
                cu->setMergeFlag(partAddr, true);
                cu->setMergeIndex(partAddr, mrgIndex);
                cu->setInterDirSubParts(mrgInterDir, partAddr, partIdx, cu->getDepth(partAddr));
                {
                    cu->getCUMvField(REF_PIC_LIST_0)->setAllMvField(mrgMvField[0], partSize, partAddr, 0, partIdx);
                    cu->getCUMvField(REF_PIC_LIST_1)->setAllMvField(mrgMvField[1], partSize, partAddr, 0, partIdx);
                }
                totalmebits += mrgBits;
            }
            else
            {
                // set ME result
                cu->setMergeFlag(partAddr, false);
                cu->setInterDirSubParts(meInterDir, partAddr, partIdx, cu->getDepth(partAddr));
                {
                    cu->getCUMvField(REF_PIC_LIST_0)->setAllMvField(meMvField[0], partSize, partAddr, 0, partIdx);
                    cu->getCUMvField(REF_PIC_LIST_1)->setAllMvField(meMvField[1], partSize, partAddr, 0, partIdx);
                }
                totalmebits += mebits;
            }
        }
        else
        {
            totalmebits += mebits;
        }
        motionCompensation(cu, predYuv, REF_PIC_LIST_X, partIdx, true, bChroma);
    }

    cu->m_totalBits = totalmebits;
    return true;
}

// AMVP
void TEncSearch::xEstimateMvPredAMVP(TComDataCU* cu, uint32_t partIdx, int list, int refIdx, MV& mvPred, AMVPInfo* amvpInfo, uint32_t* distBiP)
{
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

    //-- Check Minimum Cost.
    for (i = 0; i < AMVP_MAX_NUM_CANDS; i++)
    {
        uint32_t cost = xGetTemplateCost(cu, partAddr, &m_predTempYuv, amvpInfo->m_mvCand[i], list, refIdx, roiWidth, roiHeight);
        if (bestCost > cost)
        {
            bestCost = cost;
            bestMv   = amvpInfo->m_mvCand[i];
            bestIdx  = i;
            (*distBiP) = cost;
        }
    }

    // Setting Best MVP
    mvPred = bestMv;
    cu->setMVPIdx(list, partAddr, bestIdx);
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
        static const uint32_t aauiMbBits[2][3][3] = { { { 0, 0, 3 }, { 0, 0, 0 }, { 0, 0, 0 } }, { { 5, 7, 7 }, { 7, 5, 7 }, { 9 - 3, 9 - 3, 9 - 3 } } };
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
        static const uint32_t aauiMbBits[2][3][3] = { { { 0, 2, 3 }, { 0, 0, 0 }, { 0, 0, 0 } }, { { 5, 7, 7 }, { 7 - 2, 7 - 2, 9 - 2 }, { 9 - 3, 9 - 3, 9 - 3 } } };
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

/* Check if using an alternative MVP would result in a smaller MVD + signal bits */
void TEncSearch::xCheckBestMVP(AMVPInfo* amvpInfo, MV mv, MV& mvPred, int& outMvpIdx, uint32_t& outBits, uint32_t& outCost)
{
    assert(amvpInfo->m_mvCand[outMvpIdx] == mvPred);

    m_me.setMVP(mvPred);
    int bestMvpIdx = outMvpIdx;
    int mvBitsOrig = m_me.bitcost(mv) + MVP_IDX_BITS;
    int bestMvBits = mvBitsOrig;

    for (int mvpIdx = 0; mvpIdx < AMVP_MAX_NUM_CANDS; mvpIdx++)
    {
        if (mvpIdx == outMvpIdx)
            continue;

        m_me.setMVP(amvpInfo->m_mvCand[mvpIdx]);
        int mvbits = m_me.bitcost(mv) + MVP_IDX_BITS;

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

uint32_t TEncSearch::xGetTemplateCost(TComDataCU* cu, uint32_t partAddr, TComYuv* templateCand, MV mvCand,
                                      int list, int refIdx, int sizex, int sizey)
{
    // TODO: does it clip with m_referenceRowsAvailable?
    cu->clipMv(mvCand);

    // prediction pattern
    xPredInterLumaBlk(cu, cu->getSlice()->getRefPic(list, refIdx)->getPicYuvRec(), partAddr, &mvCand, sizex, sizey, templateCand);

    // calc distortion
    uint32_t cost = m_me.bufSAD(templateCand->getLumaAddr(partAddr), templateCand->getStride());
    x265_emms();
    return m_rdCost->calcRdSADCost(cost, MVP_IDX_BITS);
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
void TEncSearch::encodeResAndCalcRdInterCU(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, ShortYuv* outResiYuv,
                                           ShortYuv* outBestResiYuv, TComYuv* outReconYuv, bool bSkipRes, bool curUseRDOQ)
{
    if (cu->isIntra(0))
    {
        return;
    }

    bool bHighPass = cu->getSlice()->getSliceType() == B_SLICE;
    uint32_t bits = 0, bestBits = 0;
    uint32_t distortion = 0, bdist = 0;

    uint32_t width  = cu->getCUSize(0);
    uint32_t height = cu->getCUSize(0);

    //  No residual coding : SKIP mode
    if (bSkipRes)
    {
        cu->setSkipFlagSubParts(true, 0, cu->getDepth(0));

        outResiYuv->clear();

        predYuv->copyToPartYuv(outReconYuv, 0);
        //Luma
        int part = partitionFromSizes(width, height);
        distortion = primitives.sse_pp[part](fencYuv->getLumaAddr(), fencYuv->getStride(), outReconYuv->getLumaAddr(), outReconYuv->getStride());
        //Chroma
        part = partitionFromSizes(width >> m_hChromaShift, height >> m_vChromaShift);
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
    int      qp, qpBest = 0;
    uint64_t cost, bcost = MAX_INT64;

    qp = bHighPass ? Clip3(-cu->getSlice()->getSPS()->getQpBDOffsetY(), MAX_QP, (int)cu->getQP(0)) : cu->getQP(0);

    outResiYuv->subtract(fencYuv, predYuv, 0, width);

    cost = 0;
    bits = 0;
    distortion = 0;
    m_rdGoOnSbacCoder->load(m_rdSbacCoders[cu->getDepth(0)][CI_CURR_BEST]);

    uint32_t zeroDistortion = 0;
    xEstimateResidualQT(cu, 0, 0, outResiYuv, cu->getDepth(0), cost, bits, distortion, &zeroDistortion, curUseRDOQ);

    m_entropyCoder->resetBits();
    m_entropyCoder->encodeQtRootCbfZero(cu);
    uint32_t zeroResiBits = m_entropyCoder->getNumberOfWrittenBits();

    uint64_t zeroCost = m_rdCost->calcRdCost(zeroDistortion, zeroResiBits);
    if (cu->isLosslessCoded(0))
    {
        zeroCost = cost + 1;
    }
    if (zeroCost < cost)
    {
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

    cost = m_rdCost->calcRdCost(distortion, bits);

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
        bcost    = cost;
        qpBest   = qp;
        m_rdGoOnSbacCoder->store(m_rdSbacCoders[cu->getDepth(0)][CI_TEMP_BEST]);
    }

    assert(bcost != MAX_INT64);

    if (cu->getQtRootCbf(0))
    {
        outReconYuv->addClip(predYuv, outBestResiYuv, 0, width);
    }
    else
    {
        predYuv->copyToPartYuv(outReconYuv, 0);
    }

    // update with clipped distortion and cost (qp estimation loop uses unclipped values)
    int part = partitionFromSizes(width, height);
    bdist = primitives.sse_pp[part](fencYuv->getLumaAddr(), fencYuv->getStride(), outReconYuv->getLumaAddr(), outReconYuv->getStride());
    part = partitionFromSizes(width >> cu->getHorzChromaShift(), height >> cu->getVertChromaShift());
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

void TEncSearch::generateCoeffRecon(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, ShortYuv* resiYuv, TComYuv* reconYuv, bool skipRes)
{
    if (skipRes && cu->getPredictionMode(0) == MODE_INTER && cu->getMergeFlag(0) && cu->getPartitionSize(0) == SIZE_2Nx2N)
    {
        predYuv->copyToPartYuv(reconYuv, 0);
        cu->setCbfSubParts(0, TEXT_LUMA, 0, 0, cu->getDepth(0));
        cu->setCbfSubParts(0, TEXT_CHROMA_U, 0, 0, cu->getDepth(0));
        cu->setCbfSubParts(0, TEXT_CHROMA_V, 0, 0, cu->getDepth(0));
        return;
    }
    if (cu->getPredictionMode(0) == MODE_INTER)
    {
        residualTransformQuantInter(cu, 0, 0, resiYuv, cu->getDepth(0), true);
        uint32_t width  = cu->getCUSize(0);
        if (cu->getQtRootCbf(0))
        {
            reconYuv->addClip(predYuv, resiYuv, 0, width);
        }
        else
        {
            predYuv->copyToPartYuv(reconYuv, 0);
            if (cu->getMergeFlag(0) && cu->getPartitionSize(0) == SIZE_2Nx2N)
            {
                cu->setSkipFlagSubParts(true, 0, cu->getDepth(0));
            }
        }
    }
    else if (cu->getPredictionMode(0) == MODE_INTRA)
    {
        uint32_t initTrDepth = cu->getPartitionSize(0) == SIZE_2Nx2N ? 0 : 1;
        residualTransformQuantIntra(cu, initTrDepth, 0, fencYuv, predYuv, resiYuv, reconYuv);
        getBestIntraModeChroma(cu, fencYuv, predYuv);
        residualQTIntrachroma(cu, 0, 0, fencYuv, predYuv, resiYuv, reconYuv);
    }
}

#if _MSC_VER
#pragma warning(disable: 4701) // potentially uninitialized local variable
#endif

void TEncSearch::residualTransformQuantInter(TComDataCU* cu, uint32_t absPartIdx, uint32_t absTUPartIdx, ShortYuv* resiYuv, const uint32_t depth, bool curuseRDOQ)
{
    assert(cu->getDepth(0) == cu->getDepth(absPartIdx));
    const uint32_t trMode = depth - cu->getDepth(0);
    const uint32_t trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUSize() >> depth] + 2;
    int chFmt                 = cu->getChromaFormat();

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
    uint32_t absSumY = 0, absSumU = 0, absSumV = 0;
    int lastPosY = -1, lastPosU = -1, lastPosV = -1;
    if (bCheckFull)
    {
        const uint32_t numCoeffPerAbsPartIdxIncrement = cu->getSlice()->getSPS()->getMaxCUSize() * cu->getSlice()->getSPS()->getMaxCUSize() >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1);

        TCoeff *coeffCurY = cu->getCoeffY() + (numCoeffPerAbsPartIdxIncrement * absPartIdx);
        TCoeff *coeffCurU = cu->getCoeffCb() + (numCoeffPerAbsPartIdxIncrement * absPartIdx >> 2);
        TCoeff *coeffCurV = cu->getCoeffCr() + (numCoeffPerAbsPartIdxIncrement * absPartIdx >> 2);

        int trWidth = 0, trHeight = 0, trWidthC = 0, trHeightC = 0;
        uint32_t absTUPartIdxC = absPartIdx;

        trWidth  = trHeight  = 1 << trSizeLog2;
        trWidthC = trHeightC = 1 << trSizeCLog2;
        cu->setTrIdxSubParts(depth - cu->getDepth(0), absPartIdx, depth);

        cu->setTransformSkipSubParts(0, TEXT_LUMA, absPartIdx, depth);
        if (bCodeChroma)
        {
            cu->setTransformSkipSubParts(0, TEXT_CHROMA_U, absPartIdx, cu->getDepth(0) + trModeC);
            cu->setTransformSkipSubParts(0, TEXT_CHROMA_V, absPartIdx, cu->getDepth(0) + trModeC);
        }

        m_trQuant->setQPforQuant(cu->getQP(0), TEXT_LUMA, cu->getSlice()->getSPS()->getQpBDOffsetY(), 0, chFmt);
        m_trQuant->selectLambda(TEXT_LUMA);

        absSumY = m_trQuant->transformNxN(cu, resiYuv->getLumaAddr(absTUPartIdx), resiYuv->m_width, coeffCurY,
                                          trWidth, TEXT_LUMA, absPartIdx, &lastPosY, false, curuseRDOQ);

        cu->setCbfSubParts(absSumY ? setCbf : 0, TEXT_LUMA, absPartIdx, depth);

        if (bCodeChroma)
        {
            int curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCbQpOffset() + cu->getSlice()->getSliceQpDeltaCb();
            m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset, chFmt);

            m_trQuant->selectLambda(TEXT_CHROMA);

            absSumU = m_trQuant->transformNxN(cu, resiYuv->getCbAddr(absTUPartIdxC), resiYuv->m_cwidth, coeffCurU,
                                              trWidthC, TEXT_CHROMA_U, absPartIdx, &lastPosU, false, curuseRDOQ);

            curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCrQpOffset() + cu->getSlice()->getSliceQpDeltaCr();
            m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset, chFmt);
            absSumV = m_trQuant->transformNxN(cu, resiYuv->getCrAddr(absTUPartIdxC), resiYuv->m_cwidth, coeffCurV,
                                              trWidthC, TEXT_CHROMA_V, absPartIdx, &lastPosV, false, curuseRDOQ);

            cu->setCbfSubParts(absSumU ? setCbf : 0, TEXT_CHROMA_U, absPartIdx, cu->getDepth(0) + trModeC);
            cu->setCbfSubParts(absSumV ? setCbf : 0, TEXT_CHROMA_V, absPartIdx, cu->getDepth(0) + trModeC);
        }

        if (absSumY)
        {
            int16_t *curResiY = resiYuv->getLumaAddr(absTUPartIdx);

            m_trQuant->setQPforQuant(cu->getQP(0), TEXT_LUMA, cu->getSlice()->getSPS()->getQpBDOffsetY(), 0, chFmt);

            int scalingListType = 3 + TEXT_LUMA;
            assert(scalingListType < 6);
            m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdx), REG_DCT, curResiY, resiYuv->m_width,  coeffCurY, trWidth, scalingListType, false, lastPosY); //this is for inter mode only
        }
        else
        {
            int16_t *ptr =  resiYuv->getLumaAddr(absTUPartIdx);
            assert(trWidth == trHeight);
            primitives.blockfill_s[(int)g_convertToBit[trWidth]](ptr, resiYuv->m_width, 0);
        }

        if (bCodeChroma)
        {
            if (absSumU)
            {
                int16_t *pcResiCurrU = resiYuv->getCbAddr(absTUPartIdxC);

                int curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCbQpOffset() + cu->getSlice()->getSliceQpDeltaCb();
                m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset, chFmt);

                int scalingListType = 3 + TEXT_CHROMA_U;
                assert(scalingListType < 6);
                m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdx), REG_DCT, pcResiCurrU, resiYuv->m_cwidth, coeffCurU, trWidthC, scalingListType, false, lastPosU);
            }
            else
            {
                int16_t *ptr = resiYuv->getCbAddr(absTUPartIdxC);
                assert(trWidthC == trHeightC);
                primitives.blockfill_s[(int)g_convertToBit[trWidthC]](ptr, resiYuv->m_cwidth, 0);
            }
            if (absSumV)
            {
                int16_t *curResiV = resiYuv->getCrAddr(absTUPartIdxC);
                int curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCrQpOffset() + cu->getSlice()->getSliceQpDeltaCr();
                m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset, chFmt);

                int scalingListType = 3 + TEXT_CHROMA_V;
                assert(scalingListType < 6);
                m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdx), REG_DCT, curResiV, resiYuv->m_cwidth, coeffCurV, trWidthC, scalingListType, false, lastPosV);
            }
            else
            {
                int16_t *ptr =  resiYuv->getCrAddr(absTUPartIdxC);
                assert(trWidthC == trHeightC);
                primitives.blockfill_s[(int)g_convertToBit[trWidthC]](ptr, resiYuv->m_cwidth, 0);
            }
        }
        cu->setCbfSubParts(absSumY ? setCbf : 0, TEXT_LUMA, absPartIdx, depth);
        if (bCodeChroma)
        {
            cu->setCbfSubParts(absSumU ? setCbf : 0, TEXT_CHROMA_U, absPartIdx, cu->getDepth(0) + trModeC);
            cu->setCbfSubParts(absSumV ? setCbf : 0, TEXT_CHROMA_V, absPartIdx, cu->getDepth(0) + trModeC);
        }
    }

    // code sub-blocks
    if (bCheckSplit && !bCheckFull)
    {
        const uint32_t qPartNumSubdiv = cu->getPic()->getNumPartInCU() >> ((depth + 1) << 1);
        for (uint32_t i = 0; i < 4; ++i)
        {
            uint32_t nsAddr = absPartIdx + i * qPartNumSubdiv;
            residualTransformQuantInter(cu, absPartIdx + i * qPartNumSubdiv, nsAddr, resiYuv, depth + 1, curuseRDOQ);
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

        return;
    }

    cu->setTrIdxSubParts(trMode, absPartIdx, depth);
    cu->setCbfSubParts(absSumY ? setCbf : 0, TEXT_LUMA, absPartIdx, depth);

    if (bCodeChroma)
    {
        cu->setCbfSubParts(absSumU ? setCbf : 0, TEXT_CHROMA_U, absPartIdx, cu->getDepth(0) + trModeC);
        cu->setCbfSubParts(absSumV ? setCbf : 0, TEXT_CHROMA_V, absPartIdx, cu->getDepth(0) + trModeC);
    }
}

void TEncSearch::xEstimateResidualQT(TComDataCU*    cu,
                                     uint32_t       absPartIdx,
                                     uint32_t       absTUPartIdx,
                                     ShortYuv*     resiYuv,
                                     const uint32_t depth,
                                     uint64_t &     rdCost,
                                     uint32_t &     outBits,
                                     uint32_t &     outDist,
                                     uint32_t *     outZeroDist,
                                     bool           curuseRDOQ)
{
    assert(cu->getDepth(0) == cu->getDepth(absPartIdx));
    const uint32_t trMode = depth - cu->getDepth(0);
    const uint32_t trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUSize() >> depth] + 2;
    uint32_t  trSizeCLog2     = g_convertToBit[(cu->getSlice()->getSPS()->getMaxCUSize() >> m_hChromaShift) >> depth] + 2;
    int chFmt                 = cu->getChromaFormat();

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
    if ((trSizeLog2 == 2) && !(chFmt == CHROMA_444))
    {
        trSizeCLog2++;
        trModeC--;
        uint32_t qpdiv = cu->getPic()->getNumPartInCU() >> ((depth - 1) << 1);
        bCodeChroma = ((absPartIdx % qpdiv) == 0);
    }

    const uint32_t setCbf = 1 << trMode;
    // code full block
    uint64_t singleCost = MAX_INT64;
    uint32_t singleBits = 0;
    uint32_t singleDist = 0;
    uint32_t absSumY = 0, absSumU = 0, absSumV = 0;
    int lastPosY = -1, lastPosU = -1, lastPosV = -1;
    uint32_t bestTransformMode[3] = { 0 };

    m_rdGoOnSbacCoder->store(m_rdSbacCoders[depth][CI_QT_TRAFO_ROOT]);

    if (bCheckFull)
    {
        const uint32_t numCoeffPerAbsPartIdxIncrement = cu->getSlice()->getSPS()->getMaxCUSize() * cu->getSlice()->getSPS()->getMaxCUSize() >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1);
        const uint32_t qtlayer = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;
        TCoeff *coeffCurY = m_qtTempCoeffY[qtlayer] + (numCoeffPerAbsPartIdxIncrement * absPartIdx);
        TCoeff *coeffCurU = m_qtTempCoeffCb[qtlayer] + (numCoeffPerAbsPartIdxIncrement * absPartIdx >> (m_hChromaShift + m_vChromaShift));
        TCoeff *coeffCurV = m_qtTempCoeffCr[qtlayer] + (numCoeffPerAbsPartIdxIncrement * absPartIdx >> (m_hChromaShift + m_vChromaShift));
        int trWidth = 0, trHeight = 0, trWidthC = 0, trHeightC = 0;
        uint32_t absTUPartIdxC = absPartIdx;

        trWidth  = trHeight  = 1 << trSizeLog2;
        trWidthC = trHeightC = 1 << trSizeCLog2;
        cu->setTrIdxSubParts(depth - cu->getDepth(0), absPartIdx, depth);
        uint64_t minCostY = MAX_INT64;
        uint64_t minCostU = MAX_INT64;
        uint64_t minCostV = MAX_INT64;
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

        if (m_cfg->bEnableRDOQ && curuseRDOQ)
        {
            m_entropyCoder->estimateBit(m_trQuant->m_estBitsSbac, trWidth, TEXT_LUMA);
        }

        m_trQuant->setQPforQuant(cu->getQP(0), TEXT_LUMA, cu->getSlice()->getSPS()->getQpBDOffsetY(), 0, chFmt);
        m_trQuant->selectLambda(TEXT_LUMA);

        absSumY = m_trQuant->transformNxN(cu, resiYuv->getLumaAddr(absTUPartIdx), resiYuv->m_width, coeffCurY,
                                          trWidth, TEXT_LUMA, absPartIdx, &lastPosY, false, curuseRDOQ);

        cu->setCbfSubParts(absSumY ? setCbf : 0, TEXT_LUMA, absPartIdx, depth);

        if (bCodeChroma)
        {
            if (m_cfg->bEnableRDOQ && curuseRDOQ)
            {
                m_entropyCoder->estimateBit(m_trQuant->m_estBitsSbac, trWidthC, TEXT_CHROMA);
            }
            //Cb transform
            int curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCbQpOffset() + cu->getSlice()->getSliceQpDeltaCb();
            m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset, chFmt);

            m_trQuant->selectLambda(TEXT_CHROMA);

            absSumU = m_trQuant->transformNxN(cu, resiYuv->getCbAddr(absTUPartIdxC), resiYuv->m_cwidth, coeffCurU,
                                              trWidthC, TEXT_CHROMA_U, absPartIdx, &lastPosU, false, curuseRDOQ);
            //Cr transform
            curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCrQpOffset() + cu->getSlice()->getSliceQpDeltaCr();
            m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset, chFmt);
            absSumV = m_trQuant->transformNxN(cu, resiYuv->getCrAddr(absTUPartIdxC), resiYuv->m_cwidth, coeffCurV,
                                              trWidthC, TEXT_CHROMA_V, absPartIdx, &lastPosV, false, curuseRDOQ);

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

        ALIGN_VAR_32(static const pixel, zeroPel[MAX_CU_SIZE * MAX_CU_SIZE]) = { 0 };

        int partSize = partitionFromSizes(trWidth, trHeight);
        uint32_t distY = primitives.sse_sp[partSize](resiYuv->getLumaAddr(absTUPartIdx), resiYuv->m_width, (pixel*)zeroPel, trWidth);

        if (outZeroDist)
        {
            *outZeroDist += distY;
        }
        if (absSumY)
        {
            int16_t *curResiY = m_qtTempShortYuv[qtlayer].getLumaAddr(absTUPartIdx);

            m_trQuant->setQPforQuant(cu->getQP(0), TEXT_LUMA, cu->getSlice()->getSPS()->getQpBDOffsetY(), 0, chFmt);

            int scalingListType = 3 + TEXT_LUMA;
            assert(scalingListType < 6);
            assert(m_qtTempShortYuv[qtlayer].m_width == MAX_CU_SIZE);
            m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdx), REG_DCT, curResiY, MAX_CU_SIZE,  coeffCurY, trWidth, scalingListType, false, lastPosY); //this is for inter mode only

            const uint32_t nonZeroDistY = primitives.sse_ss[partSize](resiYuv->getLumaAddr(absTUPartIdx), resiYuv->m_width, m_qtTempShortYuv[qtlayer].getLumaAddr(absTUPartIdx), MAX_CU_SIZE);
            if (cu->isLosslessCoded(0))
            {
                distY = nonZeroDistY;
            }
            else
            {
                const uint64_t singleCostY = m_rdCost->calcRdCost(nonZeroDistY, uiSingleBitsY);
                m_entropyCoder->resetBits();
                m_entropyCoder->encodeQtCbfZero(cu, TEXT_LUMA,     trMode);
                const uint32_t nullBitsY = m_entropyCoder->getNumberOfWrittenBits();
                const uint64_t nullCostY = m_rdCost->calcRdCost(distY, nullBitsY);
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
            int16_t *ptr =  m_qtTempShortYuv[qtlayer].getLumaAddr(absTUPartIdx);
            assert(m_qtTempShortYuv[qtlayer].m_width == MAX_CU_SIZE);

            assert(trWidth == trHeight);
            primitives.blockfill_s[(int)g_convertToBit[trWidth]](ptr, MAX_CU_SIZE, 0);
        }

        uint32_t distU = 0;
        uint32_t distV = 0;

        int partSizeC = partitionFromSizes(trWidthC, trHeightC);
        if (bCodeChroma)
        {
            distU = m_rdCost->scaleChromaDistCb(primitives.sse_sp[partSizeC](resiYuv->getCbAddr(absTUPartIdxC), resiYuv->m_cwidth, (pixel*)zeroPel, trWidthC));

            if (outZeroDist)
            {
                *outZeroDist += distU;
            }
            if (absSumU)
            {
                int16_t *pcResiCurrU = m_qtTempShortYuv[qtlayer].getCbAddr(absTUPartIdxC);

                int curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCbQpOffset() + cu->getSlice()->getSliceQpDeltaCb();
                m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset, chFmt);

                int scalingListType = 3 + TEXT_CHROMA_U;
                assert(scalingListType < 6);
                m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdx), REG_DCT, pcResiCurrU, m_qtTempShortYuv[qtlayer].m_cwidth, coeffCurU, trWidthC, scalingListType, false, lastPosU);
                uint32_t dist = primitives.sse_ss[partSizeC](resiYuv->getCbAddr(absTUPartIdxC), resiYuv->m_cwidth,
                                                             m_qtTempShortYuv[qtlayer].getCbAddr(absTUPartIdxC),
                                                             m_qtTempShortYuv[qtlayer].m_cwidth);
                const uint32_t nonZeroDistU = m_rdCost->scaleChromaDistCb(dist);

                if (cu->isLosslessCoded(0))
                {
                    distU = nonZeroDistU;
                }
                else
                {
                    const uint64_t singleCostU = m_rdCost->calcRdCost(nonZeroDistU, singleBitsU);
                    m_entropyCoder->resetBits();
                    m_entropyCoder->encodeQtCbfZero(cu, TEXT_CHROMA_U, trMode);
                    const uint32_t nullBitsU = m_entropyCoder->getNumberOfWrittenBits();
                    const uint64_t nullCostU = m_rdCost->calcRdCost(distU, nullBitsU);
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
                int16_t *ptr = m_qtTempShortYuv[qtlayer].getCbAddr(absTUPartIdxC);
                const uint32_t stride = m_qtTempShortYuv[qtlayer].m_cwidth;
                assert(trWidthC == trHeightC);
                primitives.blockfill_s[(int)g_convertToBit[trWidthC]](ptr, stride, 0);
            }

            distV = m_rdCost->scaleChromaDistCr(primitives.sse_sp[partSizeC](resiYuv->getCrAddr(absTUPartIdxC), resiYuv->m_cwidth, (pixel*)zeroPel, trWidthC));
            if (outZeroDist)
            {
                *outZeroDist += distV;
            }
            if (absSumV)
            {
                int16_t *curResiV = m_qtTempShortYuv[qtlayer].getCrAddr(absTUPartIdxC);
                int curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCrQpOffset() + cu->getSlice()->getSliceQpDeltaCr();
                m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset, chFmt);

                int scalingListType = 3 + TEXT_CHROMA_V;
                assert(scalingListType < 6);
                m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdx), REG_DCT, curResiV, m_qtTempShortYuv[qtlayer].m_cwidth, coeffCurV, trWidthC, scalingListType, false, lastPosV);
                uint32_t dist = primitives.sse_ss[partSizeC](resiYuv->getCrAddr(absTUPartIdxC), resiYuv->m_cwidth,
                                                             m_qtTempShortYuv[qtlayer].getCrAddr(absTUPartIdxC),
                                                             m_qtTempShortYuv[qtlayer].m_cwidth);
                const uint32_t nonZeroDistV = m_rdCost->scaleChromaDistCr(dist);

                if (cu->isLosslessCoded(0))
                {
                    distV = nonZeroDistV;
                }
                else
                {
                    const uint64_t singleCostV = m_rdCost->calcRdCost(nonZeroDistV, singleBitsV);
                    m_entropyCoder->resetBits();
                    m_entropyCoder->encodeQtCbfZero(cu, TEXT_CHROMA_V, trMode);
                    const uint32_t nullBitsV = m_entropyCoder->getNumberOfWrittenBits();
                    const uint64_t nullCostV = m_rdCost->calcRdCost(distV, nullBitsV);
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
                int16_t *ptr =  m_qtTempShortYuv[qtlayer].getCrAddr(absTUPartIdxC);
                const uint32_t stride = m_qtTempShortYuv[qtlayer].m_cwidth;
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
            uint64_t singleCostY = MAX_INT64;

            int16_t *curResiY = m_qtTempShortYuv[qtlayer].getLumaAddr(absTUPartIdx);
            assert(m_qtTempShortYuv[qtlayer].m_width == MAX_CU_SIZE);

            TCoeff bestCoeffY[32 * 32];
            memcpy(bestCoeffY, coeffCurY, sizeof(TCoeff) * numSamplesLuma);

            int16_t bestResiY[32 * 32];
            for (int i = 0; i < trHeight; ++i)
            {
                memcpy(bestResiY + i * trWidth, curResiY + i * MAX_CU_SIZE, sizeof(int16_t) * trWidth);
            }

            m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_QT_TRAFO_ROOT]);

            cu->setTransformSkipSubParts(1, TEXT_LUMA, absPartIdx, depth);

            if (m_cfg->bEnableRDOQTS)
            {
                m_entropyCoder->estimateBit(m_trQuant->m_estBitsSbac, trWidth, TEXT_LUMA);
            }

            m_trQuant->setQPforQuant(cu->getQP(0), TEXT_LUMA, cu->getSlice()->getSPS()->getQpBDOffsetY(), 0, chFmt);

            m_trQuant->selectLambda(TEXT_LUMA);
            absSumTransformSkipY = m_trQuant->transformNxN(cu, resiYuv->getLumaAddr(absTUPartIdx), resiYuv->m_width, coeffCurY,
                                                           trWidth, TEXT_LUMA, absPartIdx, &lastPosTransformSkipY, true, curuseRDOQ);
            cu->setCbfSubParts(absSumTransformSkipY ? setCbf : 0, TEXT_LUMA, absPartIdx, depth);

            if (absSumTransformSkipY != 0)
            {
                m_entropyCoder->resetBits();
                m_entropyCoder->encodeQtCbf(cu, absPartIdx, TEXT_LUMA, trMode);
                m_entropyCoder->encodeCoeffNxN(cu, coeffCurY, absPartIdx, trWidth, trHeight, depth, TEXT_LUMA);
                const uint32_t skipSingleBitsY = m_entropyCoder->getNumberOfWrittenBits();

                m_trQuant->setQPforQuant(cu->getQP(0), TEXT_LUMA, cu->getSlice()->getSPS()->getQpBDOffsetY(), 0, chFmt);

                int scalingListType = 3 + TEXT_LUMA;
                assert(scalingListType < 6);
                assert(m_qtTempShortYuv[qtlayer].m_width == MAX_CU_SIZE);

                m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdx), REG_DCT, curResiY, MAX_CU_SIZE,  coeffCurY, trWidth, scalingListType, true, lastPosTransformSkipY);

                nonZeroDistY = primitives.sse_ss[partSize](resiYuv->getLumaAddr(absTUPartIdx), resiYuv->m_width,
                                                           m_qtTempShortYuv[qtlayer].getLumaAddr(absTUPartIdx),
                                                           MAX_CU_SIZE);

                singleCostY = m_rdCost->calcRdCost(nonZeroDistY, skipSingleBitsY);
            }

            if (!absSumTransformSkipY || minCostY < singleCostY)
            {
                cu->setTransformSkipSubParts(0, TEXT_LUMA, absPartIdx, depth);
                memcpy(coeffCurY, bestCoeffY, sizeof(TCoeff) * numSamplesLuma);
                for (int i = 0; i < trHeight; ++i)
                {
                    memcpy(curResiY + i * MAX_CU_SIZE, &bestResiY[i * trWidth], sizeof(int16_t) * trWidth);
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
            uint64_t singleCostU = MAX_INT64;
            uint64_t singleCostV = MAX_INT64;

            int16_t *curResiU = m_qtTempShortYuv[qtlayer].getCbAddr(absTUPartIdxC);
            int16_t *curResiV = m_qtTempShortYuv[qtlayer].getCrAddr(absTUPartIdxC);
            uint32_t stride = m_qtTempShortYuv[qtlayer].m_cwidth;
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

            if (m_cfg->bEnableRDOQTS)
            {
                m_entropyCoder->estimateBit(m_trQuant->m_estBitsSbac, trWidthC, TEXT_CHROMA);
            }

            int curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCbQpOffset() + cu->getSlice()->getSliceQpDeltaCb();
            m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset, chFmt);
            m_trQuant->selectLambda(TEXT_CHROMA);

            absSumTransformSkipU = m_trQuant->transformNxN(cu, resiYuv->getCbAddr(absTUPartIdxC), resiYuv->m_cwidth, coeffCurU,
                                                           trWidthC, TEXT_CHROMA_U, absPartIdx, &lastPosTransformSkipU, true, curuseRDOQ);
            curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCrQpOffset() + cu->getSlice()->getSliceQpDeltaCr();
            m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset, chFmt);
            absSumTransformSkipV = m_trQuant->transformNxN(cu, resiYuv->getCrAddr(absTUPartIdxC), resiYuv->m_cwidth, coeffCurV,
                                                           trWidthC, TEXT_CHROMA_V, absPartIdx, &lastPosTransformSkipV, true, curuseRDOQ);

            cu->setCbfSubParts(absSumTransformSkipU ? setCbf : 0, TEXT_CHROMA_U, absPartIdx, cu->getDepth(0) + trModeC);
            cu->setCbfSubParts(absSumTransformSkipV ? setCbf : 0, TEXT_CHROMA_V, absPartIdx, cu->getDepth(0) + trModeC);

            m_entropyCoder->resetBits();
            singleBitsU = 0;

            if (absSumTransformSkipU)
            {
                m_entropyCoder->encodeQtCbf(cu, absPartIdx, TEXT_CHROMA_U, trMode);
                m_entropyCoder->encodeCoeffNxN(cu, coeffCurU, absPartIdx, trWidthC, trHeightC, depth, TEXT_CHROMA_U);
                singleBitsU = m_entropyCoder->getNumberOfWrittenBits();

                curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCbQpOffset() + cu->getSlice()->getSliceQpDeltaCb();
                m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset, chFmt);

                int scalingListType = 3 + TEXT_CHROMA_U;
                assert(scalingListType < 6);
                m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdx), REG_DCT, curResiU, m_qtTempShortYuv[qtlayer].m_cwidth, coeffCurU, trWidthC, scalingListType, true, lastPosTransformSkipU);
                uint32_t dist = primitives.sse_ss[partSizeC](resiYuv->getCbAddr(absTUPartIdxC), resiYuv->m_cwidth,
                                                             m_qtTempShortYuv[qtlayer].getCbAddr(absTUPartIdxC),
                                                             m_qtTempShortYuv[qtlayer].m_cwidth);
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
                m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset, chFmt);

                int scalingListType = 3 + TEXT_CHROMA_V;
                assert(scalingListType < 6);
                m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdx), REG_DCT, curResiV, m_qtTempShortYuv[qtlayer].m_cwidth, coeffCurV, trWidthC, scalingListType, true, lastPosTransformSkipV);
                uint32_t dist = primitives.sse_ss[partSizeC](resiYuv->getCrAddr(absTUPartIdxC), resiYuv->m_cwidth,
                                                             m_qtTempShortYuv[qtlayer].getCrAddr(absTUPartIdxC),
                                                             m_qtTempShortYuv[qtlayer].m_cwidth);
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
        uint64_t subDivCost = 0;

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
    const uint32_t curTrMode   = depth - cu->getDepth(0);
    const uint32_t trMode      = cu->getTransformIdx(absPartIdx);
    const bool     bSubdiv     = curTrMode != trMode;
    const uint32_t trSizeLog2  = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUSize() >> depth] + 2;
    uint32_t       trSizeCLog2 = g_convertToBit[(cu->getSlice()->getSPS()->getMaxCUSize() >> m_hChromaShift) >> depth] + 2;
    int            chFmt       = cu->getChromaFormat();

    if (bSubdivAndCbf && trSizeLog2 <= cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() && trSizeLog2 > cu->getQuadtreeTULog2MinSizeInCU(absPartIdx))
    {
        m_entropyCoder->encodeTransformSubdivFlag(bSubdiv, 5 - trSizeLog2);
    }

    assert(cu->getPredictionMode(absPartIdx) != MODE_INTRA);

    bool mCodeAll = true;
    int width  = 1 << trSizeLog2;
    int height = 1 << trSizeLog2;
    const uint32_t numPels = (width >> cu->getHorzChromaShift()) * (height >> cu->getHorzChromaShift());
    if (numPels < (MIN_TU_SIZE * MIN_TU_SIZE))
    {
        mCodeAll = false;
    }

    if (bSubdivAndCbf)
    {
        const bool bFirstCbfOfCU = curTrMode == 0;
        if (bFirstCbfOfCU || mCodeAll)
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
        else
        {
            assert(cu->getCbf(absPartIdx, TEXT_CHROMA_U, curTrMode) == cu->getCbf(absPartIdx, TEXT_CHROMA_U, curTrMode - 1));
            assert(cu->getCbf(absPartIdx, TEXT_CHROMA_V, curTrMode) == cu->getCbf(absPartIdx, TEXT_CHROMA_V, curTrMode - 1));
        }
    }

    if (!bSubdiv)
    {
        //Luma
        const uint32_t numCoeffPerAbsPartIdxIncrement = cu->getSlice()->getSPS()->getMaxCUSize() * cu->getSlice()->getSPS()->getMaxCUSize() >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1);
        //assert( 16 == uiNumCoeffPerAbsPartIdxIncrement ); // check
        const uint32_t qtlayer = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;
        TCoeff *coeffCurY = m_qtTempCoeffY[qtlayer] +  numCoeffPerAbsPartIdxIncrement * absPartIdx;

        //Chroma
        TCoeff *coeffCurU = m_qtTempCoeffCb[qtlayer] + (numCoeffPerAbsPartIdxIncrement * absPartIdx >> (m_hChromaShift + m_vChromaShift));
        TCoeff *coeffCurV = m_qtTempCoeffCr[qtlayer] + (numCoeffPerAbsPartIdxIncrement * absPartIdx >> (m_hChromaShift + m_vChromaShift));
        bool  bCodeChroma = true;
        if ((trSizeLog2 == 2) && !(chFmt == CHROMA_444))
        {
            trSizeCLog2++;
            uint32_t qpdiv = cu->getPic()->getNumPartInCU() >> ((depth - 1) << 1);
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
                m_entropyCoder->encodeCoeffNxN(cu, coeffCurY, absPartIdx, 1 << trSizeLog2, 1 << trSizeLog2, depth, TEXT_LUMA);
            }
            if (bCodeChroma)
            {
                if (ttype == TEXT_CHROMA_U && cu->getCbf(absPartIdx, TEXT_CHROMA_U, trMode))
                {
                    m_entropyCoder->encodeCoeffNxN(cu, coeffCurU, absPartIdx, 1 << trSizeCLog2, 1 << trSizeCLog2, depth, TEXT_CHROMA_U);
                }
                if (ttype == TEXT_CHROMA_V && cu->getCbf(absPartIdx, TEXT_CHROMA_V, trMode))
                {
                    m_entropyCoder->encodeCoeffNxN(cu, coeffCurV, absPartIdx, 1 << trSizeCLog2, 1 << trSizeCLog2, depth, TEXT_CHROMA_V);
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

void TEncSearch::xSetResidualQTData(TComDataCU* cu, uint32_t absPartIdx, uint32_t absTUPartIdx, ShortYuv* resiYuv, uint32_t depth, bool bSpatial)
{
    assert(cu->getDepth(0) == cu->getDepth(absPartIdx));
    int            chFmt     = cu->getChromaFormat();
    const uint32_t curTrMode = depth - cu->getDepth(0);
    const uint32_t trMode = cu->getTransformIdx(absPartIdx);

    if (curTrMode == trMode)
    {
        const uint32_t trSizeLog2 = g_convertToBit[cu->getSlice()->getSPS()->getMaxCUSize() >> depth] + 2;
        uint32_t  trSizeCLog2     = g_convertToBit[(cu->getSlice()->getSPS()->getMaxCUSize() >> cu->getHorzChromaShift()) >> depth] + 2;
        const uint32_t qtlayer    = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;

        bool  bCodeChroma   = true;
        bool bChromaSame = false;
        uint32_t  trModeC     = trMode;
        if ((trSizeLog2 == 2) && !(chFmt == CHROMA_444))
        {
            trSizeCLog2++;
            trModeC--;
            uint32_t qpdiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trModeC) << 1);
            bCodeChroma  = ((absPartIdx % qpdiv) == 0);
            bChromaSame = true;
        }

        if (bSpatial)
        {
            int trWidth  = 1 << trSizeLog2;
            int trHeight = 1 << trSizeLog2;
            m_qtTempShortYuv[qtlayer].copyPartToPartLuma(resiYuv, absTUPartIdx, trWidth, trHeight);

            if (bCodeChroma)
            {
                m_qtTempShortYuv[qtlayer].copyPartToPartChroma(resiYuv, absPartIdx, 1 << trSizeLog2, bChromaSame);
            }
        }
        else
        {
            uint32_t uiNumCoeffPerAbsPartIdxIncrement = cu->getSlice()->getSPS()->getMaxCUSize() * cu->getSlice()->getSPS()->getMaxCUSize() >> (cu->getSlice()->getSPS()->getMaxCUDepth() << 1);
            uint32_t uiNumCoeffY = (1 << (trSizeLog2 << 1));
            TCoeff* pcCoeffSrcY = m_qtTempCoeffY[qtlayer] +  uiNumCoeffPerAbsPartIdxIncrement * absPartIdx;
            TCoeff* pcCoeffDstY = cu->getCoeffY() + uiNumCoeffPerAbsPartIdxIncrement * absPartIdx;
            ::memcpy(pcCoeffDstY, pcCoeffSrcY, sizeof(TCoeff) * uiNumCoeffY);
            if (bCodeChroma)
            {
                uint32_t    uiNumCoeffC = (1 << (trSizeCLog2 << 1));
                TCoeff* pcCoeffSrcU = m_qtTempCoeffCb[qtlayer] + (uiNumCoeffPerAbsPartIdxIncrement * absPartIdx >> (m_hChromaShift + m_vChromaShift));
                TCoeff* pcCoeffSrcV = m_qtTempCoeffCr[qtlayer] + (uiNumCoeffPerAbsPartIdxIncrement * absPartIdx >> (m_hChromaShift + m_vChromaShift));
                TCoeff* pcCoeffDstU = cu->getCoeffCb() + (uiNumCoeffPerAbsPartIdxIncrement * absPartIdx >> (m_hChromaShift + m_vChromaShift));
                TCoeff* pcCoeffDstV = cu->getCoeffCr() + (uiNumCoeffPerAbsPartIdxIncrement * absPartIdx >> (m_hChromaShift + m_vChromaShift));
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

uint32_t TEncSearch::xUpdateCandList(uint32_t mode, uint64_t cost, uint32_t fastCandNum, uint32_t* CandModeList, uint64_t* CandCostList)
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
        m_entropyCoder->encodeCoeff(cu, 0, cu->getDepth(0), cu->getCUSize(0), cu->getCUSize(0), bDummy);
        return m_entropyCoder->getNumberOfWrittenBits();
    }
}

//! \}
