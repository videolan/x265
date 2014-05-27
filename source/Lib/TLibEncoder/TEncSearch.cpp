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
#include "rdcost.h"
#include "encoder.h"

#include "common.h"
#include "primitives.h"
#include "PPA/ppa.h"

using namespace x265;

ALIGN_VAR_32(const pixel, RDCost::zeroPel[MAX_CU_SIZE * MAX_CU_SIZE]) = { 0 };

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
            m_qtTempShortYuv[i].destroy();
        }
    }
    X265_FREE(m_qtTempTUCoeffY);
    X265_FREE(m_qtTempTrIdx);
    X265_FREE(m_qtTempCbf[0]);
    X265_FREE(m_qtTempTransformSkipFlag[0]);

    delete[] m_qtTempCoeffY;
    delete[] m_qtTempShortYuv;
    m_qtTempTransformSkipYuv.destroy();
}

bool TEncSearch::init(Encoder* cfg, RDCost* rdCost, TComTrQuant* trQuant)
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

    const uint32_t numLayersToAllocate = cfg->m_quadtreeTULog2MaxSize - cfg->m_quadtreeTULog2MinSize + 1;
    m_qtTempCoeffY   = new coeff_t*[numLayersToAllocate * 3];
    m_qtTempCoeffCb  = m_qtTempCoeffY + numLayersToAllocate;
    m_qtTempCoeffCr  = m_qtTempCoeffY + numLayersToAllocate * 2;
    m_qtTempShortYuv = new ShortYuv[numLayersToAllocate];
    uint32_t sizeL = g_maxCUSize * g_maxCUSize;
    uint32_t sizeC = sizeL >> (m_hChromaShift + m_vChromaShift);
    for (uint32_t i = 0; i < numLayersToAllocate; ++i)
    {
        m_qtTempCoeffY[i]  = X265_MALLOC(coeff_t, sizeL + sizeC * 2);
        m_qtTempCoeffCb[i] = m_qtTempCoeffY[i] + sizeL;
        m_qtTempCoeffCr[i] = m_qtTempCoeffY[i] + sizeL + sizeC;
        m_qtTempShortYuv[i].create(MAX_CU_SIZE, MAX_CU_SIZE, cfg->param->internalCsp);
    }

    const uint32_t numPartitions = 1 << (g_maxCUDepth << 1);
    CHECKED_MALLOC(m_qtTempTrIdx, uint8_t, numPartitions);
    CHECKED_MALLOC(m_qtTempCbf[0], uint8_t, numPartitions * 3);
    m_qtTempCbf[1] = m_qtTempCbf[0] + numPartitions;
    m_qtTempCbf[2] = m_qtTempCbf[0] + numPartitions * 2;
    CHECKED_MALLOC(m_qtTempTransformSkipFlag[0], uint8_t, numPartitions * 3);
    m_qtTempTransformSkipFlag[1] = m_qtTempTransformSkipFlag[0] + numPartitions;
    m_qtTempTransformSkipFlag[2] = m_qtTempTransformSkipFlag[0] + numPartitions * 2;

    CHECKED_MALLOC(m_qtTempTUCoeffY, coeff_t, MAX_TS_SIZE * MAX_TS_SIZE * 3);
    m_qtTempTUCoeffCb = m_qtTempTUCoeffY + MAX_TS_SIZE * MAX_TS_SIZE;
    m_qtTempTUCoeffCr = m_qtTempTUCoeffY + MAX_TS_SIZE * MAX_TS_SIZE * 2;

    return m_qtTempTransformSkipYuv.create(g_maxCUSize, g_maxCUSize, cfg->param->internalCsp);

fail:
    return false;
}

void TEncSearch::setQP(int qp, double crWeight, double cbWeight)
{
    double lambda2 = x265_lambda2_tab[qp];
    double chromaLambda = lambda2 / crWeight;

    m_me.setQP(qp);
    m_trQuant->setLambda(lambda2, chromaLambda);
    m_rdCost->setLambda(lambda2, x265_lambda_tab[qp]);
    m_rdCost->setCbDistortionWeight(cbWeight);
    m_rdCost->setCrDistortionWeight(crWeight);
}

void TEncSearch::xEncSubdivCbfQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, uint32_t absPartIdxStep, uint32_t width, uint32_t height, bool bLuma, bool bChroma)
{
    uint32_t fullDepth  = cu->getDepth(0) + trDepth;
    uint32_t trMode     = cu->getTransformIdx(absPartIdx);
    uint32_t subdiv     = (trMode > trDepth ? 1 : 0);
    uint32_t trSizeLog2 = cu->getSlice()->getSPS()->getLog2MaxCodingBlockSize() - fullDepth;

    if (cu->getPredictionMode(0) == MODE_INTRA && cu->getPartitionSize(0) == SIZE_NxN && trDepth == 0)
    {
        X265_CHECK(subdiv, "subdivision not present\n");
    }
    else if (trSizeLog2 > cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize())
    {
        X265_CHECK(subdiv, "subdivision not present\n");
    }
    else if (trSizeLog2 == cu->getSlice()->getSPS()->getQuadtreeTULog2MinSize())
    {
        X265_CHECK(!subdiv, "subdivision present\n");
    }
    else if (trSizeLog2 == cu->getQuadtreeTULog2MinSizeInCU(absPartIdx))
    {
        X265_CHECK(!subdiv, "subdivision present\n");
    }
    else
    {
        X265_CHECK(trSizeLog2 > cu->getQuadtreeTULog2MinSizeInCU(absPartIdx), "transform size too small\n");
        if (bLuma)
        {
            m_entropyCoder->encodeTransformSubdivFlag(subdiv, 5 - trSizeLog2);
        }
    }

    if (bChroma)
    {
        int      chFmt      = cu->getChromaFormat();
        if ((trSizeLog2 > 2) && !(chFmt == CHROMA_444))
        {
            if (trDepth == 0 || cu->getCbf(absPartIdx, TEXT_CHROMA_U, trDepth - 1))
                m_entropyCoder->encodeQtCbf(cu, absPartIdx, absPartIdxStep, (width >> m_hChromaShift), (height >> m_vChromaShift), TEXT_CHROMA_U, trDepth, (subdiv == 0));

            if (trDepth == 0 || cu->getCbf(absPartIdx, TEXT_CHROMA_V, trDepth - 1))
                m_entropyCoder->encodeQtCbf(cu, absPartIdx, absPartIdxStep, (width >> m_hChromaShift), (height >> m_vChromaShift), TEXT_CHROMA_V, trDepth, (subdiv == 0));
        }
    }

    if (subdiv)
    {
        TComTURecurse tuIterator;
        initSection(&tuIterator, QUAD_SPLIT, absPartIdxStep);
        width  >>= 1;
        height >>= 1;

        uint32_t qtPartNum = cu->getPic()->getNumPartInCU() >> ((fullDepth + 1) << 1);
        for (uint32_t part = 0; part < 4; part++)
        {
            xEncSubdivCbfQT(cu, trDepth + 1, absPartIdx + part * qtPartNum, tuIterator.m_absPartIdxStep, width, height, bLuma, bChroma);
        }

        return;
    }

    //===== Cbfs =====
    if (bLuma)
    {
        m_entropyCoder->encodeQtCbf(cu, absPartIdx, absPartIdxStep, width, height, TEXT_LUMA, trMode, (subdiv == 0));
    }
}

void TEncSearch::xEncCoeffQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, TextType ttype, const bool splitIntoSubTUs)
{
    if (!cu->getCbf(absPartIdx, ttype, trDepth))
        return;

    uint32_t fullDepth  = cu->getDepth(0) + trDepth;
    uint32_t trMode     = cu->getTransformIdx(absPartIdx);
    uint32_t subdiv     = (trMode > trDepth ? 1 : 0);

    if (subdiv)
    {
        uint32_t qtPartNum = cu->getPic()->getNumPartInCU() >> ((fullDepth + 1) << 1);
        for (uint32_t part = 0; part < 4; part++)
        {
            xEncCoeffQT(cu, trDepth + 1, absPartIdx + part * qtPartNum, ttype, splitIntoSubTUs);
        }

        return;
    }

    uint32_t origTrDepth = trDepth;

    uint32_t trSizeLog2 = cu->getSlice()->getSPS()->getLog2MaxCodingBlockSize() - fullDepth;
    int chFmt           = cu->getChromaFormat();
    if ((ttype != TEXT_LUMA) && (trSizeLog2 == 2) && !(chFmt == CHROMA_444))
    {
        X265_CHECK(trDepth > 0, "transform size too small\n");
        trDepth--;
        uint32_t qpdiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trDepth) << 1);
        bool bFirstQ = ((absPartIdx & (qpdiv - 1)) == 0);
        if (!bFirstQ)
        {
            return;
        }
    }

    //===== coefficients =====
    uint32_t chroma     = (ttype != TEXT_LUMA ? 1 : 0);
    int cspx = chroma ? m_hChromaShift : 0;
    int cspy = chroma ? m_vChromaShift : 0;
    uint32_t width = cu->getCUSize(0) >> (trDepth + cspx);
    uint32_t height = cu->getCUSize(0) >> (trDepth + cspy);
    height = splitIntoSubTUs ? height >> 1 : height;
    uint32_t coeffOffset = absPartIdx << (cu->getPic()->getLog2UnitSize() * 2 - (cspx + cspy));
    uint32_t qtLayer = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;
    coeff_t* coeff = 0;
    switch (ttype)
    {
    case TEXT_LUMA:     coeff = m_qtTempCoeffY[qtLayer];
        break;
    case TEXT_CHROMA_U: coeff = m_qtTempCoeffCb[qtLayer];
        break;
    case TEXT_CHROMA_V: coeff = m_qtTempCoeffCr[qtLayer];
        break;
    default: X265_CHECK(0, "invalid texture type\n");
    }

    coeff += coeffOffset;

    if (width == height)
    {
        m_entropyCoder->encodeCoeffNxN(cu, coeff, absPartIdx, width, ttype);
    }
    else
    {
        uint32_t subTUSize = width * width;
        uint32_t partIdxesPerSubTU  = cu->getPic()->getNumPartInCU() >> (((cu->getDepth(absPartIdx) + trDepth) << 1) + 1);

        if (cu->getCbf(absPartIdx, ttype, origTrDepth + 1))
            m_entropyCoder->encodeCoeffNxN(cu, coeff, absPartIdx, width, ttype);
        if (cu->getCbf(absPartIdx + partIdxesPerSubTU, ttype, origTrDepth + 1))
            m_entropyCoder->encodeCoeffNxN(cu, coeff + subTUSize, absPartIdx + partIdxesPerSubTU, width, ttype);
    }
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
                    m_entropyCoder->encodeCUTransquantBypassFlag(cu, 0);
                }
                m_entropyCoder->encodeSkipFlag(cu, 0);
                m_entropyCoder->encodePredMode(cu, 0);
            }

            m_entropyCoder->encodePartSize(cu, 0, cu->getDepth(0));

            if (cu->isIntra(0) && cu->getPartitionSize(0) == SIZE_2Nx2N)
            {
                m_entropyCoder->encodeIPCMInfo(cu, 0);

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
                X265_CHECK(absPartIdx == 0, "unexpected absPartIdx %d\n", absPartIdx);
                for (uint32_t part = 0; part < 4; part++)
                {
                    m_entropyCoder->encodeIntraDirModeLuma(cu, part * qtNumParts);
                }
            }
            else if ((absPartIdx & (qtNumParts - 1)) == 0)
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
                m_entropyCoder->encodeIntraDirModeChroma(cu, absPartIdx);
            }
        }
        else
        {
            uint32_t qtNumParts = cu->getTotalNumPart() >> 2;
            X265_CHECK(trDepth > 0, "unexpected trDepth %d\n", trDepth);
            if ((absPartIdx & (qtNumParts - 1)) == 0)
                m_entropyCoder->encodeIntraDirModeChroma(cu, absPartIdx);
        }
    }
}

uint32_t TEncSearch::xGetIntraBitsQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, uint32_t absPartIdxStep, bool bLuma, bool bChroma)
{
    m_entropyCoder->resetBits();
    xEncIntraHeader(cu, trDepth, absPartIdx, bLuma, bChroma);
    xEncSubdivCbfQT(cu, trDepth, absPartIdx, absPartIdxStep, cu->getCUSize(absPartIdx), cu->getCUSize(absPartIdx), bLuma, bChroma);

    if (bLuma)
    {
        xEncCoeffQT(cu, trDepth, absPartIdx, TEXT_LUMA, false);
    }
    if (bChroma)
    {
        xEncCoeffQT(cu, trDepth, absPartIdx, TEXT_CHROMA_U, false);
        xEncCoeffQT(cu, trDepth, absPartIdx, TEXT_CHROMA_V, false);
    }
    return m_entropyCoder->getNumberOfWrittenBits();
}

uint32_t TEncSearch::xGetIntraBitsQTChroma(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, uint32_t chromaId, const bool splitIntoSubTUs)
{
    m_entropyCoder->resetBits();
    if (chromaId == TEXT_CHROMA_U)
    {
        xEncCoeffQT(cu, trDepth, absPartIdx, TEXT_CHROMA_U, splitIntoSubTUs);
    }
    else if (chromaId == TEXT_CHROMA_V)
    {
        xEncCoeffQT(cu, trDepth, absPartIdx, TEXT_CHROMA_V, splitIntoSubTUs);
    }
    return m_entropyCoder->getNumberOfWrittenBits();
}

void TEncSearch::xIntraCodingLumaBlk(TComDataCU* cu,
                                     uint32_t    trDepth,
                                     uint32_t    absPartIdx,
                                     TComYuv*    fencYuv,
                                     TComYuv*    predYuv,
                                     ShortYuv*   resiYuv,
                                     uint32_t&   outDist)
{
    uint32_t fullDepth    = cu->getDepth(0)  + trDepth;
    uint32_t tuSize       = cu->getCUSize(0) >> trDepth;
    uint32_t stride       = fencYuv->getStride();
    pixel*   fenc         = fencYuv->getLumaAddr(absPartIdx);
    pixel*   pred         = predYuv->getLumaAddr(absPartIdx);
    int16_t* residual     = resiYuv->getLumaAddr(absPartIdx);
    int      part         = partitionFromSize(tuSize);
    int      sizeIdx      = g_convertToBit[tuSize];

    uint32_t trSizeLog2     = cu->getSlice()->getSPS()->getLog2MaxCodingBlockSize() - fullDepth;
    uint32_t qtLayer        = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;
    uint32_t coeffOffsetY   = absPartIdx << cu->getPic()->getLog2UnitSize() * 2;
    coeff_t* coeff          = m_qtTempCoeffY[qtLayer] + coeffOffsetY;

    int16_t* reconQt        = m_qtTempShortYuv[qtLayer].getLumaAddr(absPartIdx);

    X265_CHECK(m_qtTempShortYuv[qtLayer].m_width == MAX_CU_SIZE, "width is not max CU size\n");

    uint32_t zorder           = cu->getZorderIdxInCU() + absPartIdx;
    pixel*   reconIPred       = cu->getPic()->getPicYuvRec()->getLumaAddr(cu->getAddr(), zorder);
    uint32_t reconIPredStride = cu->getPic()->getPicYuvRec()->getStride();
    bool     useTransformSkip = !!cu->getTransformSkip(absPartIdx, TEXT_LUMA);

    //===== get residual signal =====
    X265_CHECK(!((intptr_t)fenc & (tuSize - 1)), "fenc alignment check fail\n");
    X265_CHECK(!((intptr_t)pred & (tuSize - 1)), "pred alignment check fail\n");
    X265_CHECK(!((intptr_t)residual & (tuSize - 1)), "residual alignment check fail\n");
    primitives.calcresidual[sizeIdx](fenc, pred, residual, stride);

    //===== transform and quantization =====
    //--- init rate estimation arrays for RDOQ ---
    if (useTransformSkip ? m_cfg->bEnableRDOQTS : m_cfg->bEnableRDOQ)
    {
        m_entropyCoder->estimateBit(m_trQuant->m_estBitsSbac, tuSize, TEXT_LUMA);
    }

    //--- transform and quantization ---
    uint32_t absSum = 0;
    int lastPos = -1;
    cu->setTrIdxSubParts(trDepth, absPartIdx, fullDepth);

    int      chFmt        = cu->getChromaFormat();
    m_trQuant->setQPforQuant(cu->getQP(0), TEXT_LUMA, QP_BD_OFFSET, 0, chFmt);
    m_trQuant->selectLambda(TEXT_LUMA);

    absSum = m_trQuant->transformNxN(cu, residual, stride, coeff, tuSize, TEXT_LUMA, absPartIdx, &lastPos, useTransformSkip);

    //--- set coded block flag ---
    cu->setCbfSubParts((absSum ? 1 : 0) << trDepth, TEXT_LUMA, absPartIdx, fullDepth);

    //--- inverse transform ---
    if (absSum)
    {
        int scalingListType = 0 + TEXT_LUMA;
        X265_CHECK(scalingListType < 6, "scalingListType is too large %d\n", scalingListType);
        m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdx), cu->getLumaIntraDir(absPartIdx), residual, stride, coeff, tuSize, scalingListType, useTransformSkip, lastPos);
    }
    else
    {
        int16_t* resiTmp = residual;
        memset(coeff, 0, sizeof(coeff_t) * tuSize * tuSize);
        primitives.blockfill_s[sizeIdx](resiTmp, stride, 0);
    }

    X265_CHECK(tuSize <= 32, "tuSize is too large %d\n", tuSize);
    //===== reconstruction =====
    primitives.calcrecon[sizeIdx](pred, residual, reconQt, reconIPred, stride, MAX_CU_SIZE, reconIPredStride);
    //===== update distortion =====
    outDist += primitives.sse_sp[part](reconQt, MAX_CU_SIZE, fenc, stride);
}

void TEncSearch::xIntraCodingChromaBlk(TComDataCU* cu,
                                       uint32_t    trDepth,
                                       uint32_t    absPartIdx,
                                       uint32_t    absPartIdxStep,
                                       TComYuv*    fencYuv,
                                       TComYuv*    predYuv,
                                       ShortYuv*   resiYuv,
                                       uint32_t&   outDist,
                                       uint32_t    chromaId)
{
    uint32_t fullDepth   = cu->getDepth(0) + trDepth;
    uint32_t trSizeLog2  = cu->getSlice()->getSPS()->getLog2MaxCodingBlockSize() - fullDepth;
    int      chFmt       = cu->getChromaFormat();

    uint32_t origTrDepth = trDepth;

    if ((trSizeLog2 == 2) && !(chFmt == CHROMA_444))
    {
        X265_CHECK(trDepth > 0, "trDepth should be non-zero\n");
        trDepth--;
        uint32_t qpdiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trDepth) << 1);
        bool bFirstQ = ((absPartIdx & (qpdiv - 1)) == 0);
        bool bSecondQ = (chFmt == CHROMA_422) ? ((absPartIdx & (qpdiv - 1)) == 2) : false;
        if ((!bFirstQ) && (!bSecondQ))
        {
            return;
        }
    }

    TextType ttype          = (chromaId == 1) ? TEXT_CHROMA_U : TEXT_CHROMA_V;
    uint32_t tuSize         = cu->getCUSize(0) >> (trDepth + m_hChromaShift);
    uint32_t stride         = fencYuv->getCStride();
    pixel*   fenc           = (chromaId == 1) ? fencYuv->getCbAddr(absPartIdx) : fencYuv->getCrAddr(absPartIdx);
    pixel*   pred           = (chromaId == 1) ? predYuv->getCbAddr(absPartIdx) : predYuv->getCrAddr(absPartIdx);
    int16_t* residual       = (chromaId == 1) ? resiYuv->getCbAddr(absPartIdx) : resiYuv->getCrAddr(absPartIdx);

    uint32_t qtlayer        = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;
    uint32_t coeffOffsetC   = absPartIdx << (cu->getPic()->getLog2UnitSize() * 2 - (m_hChromaShift + m_vChromaShift));
    coeff_t* coeff          = (chromaId == 1 ? m_qtTempCoeffCb[qtlayer] : m_qtTempCoeffCr[qtlayer]) + coeffOffsetC;
    int16_t* reconQt        = (chromaId == 1) ? m_qtTempShortYuv[qtlayer].getCbAddr(absPartIdx) : m_qtTempShortYuv[qtlayer].getCrAddr(absPartIdx);
    uint32_t reconQtStride  = m_qtTempShortYuv[qtlayer].m_cwidth;
    uint32_t zorder           = cu->getZorderIdxInCU() + absPartIdx;
    pixel*   reconIPred       = (chromaId == 1) ? cu->getPic()->getPicYuvRec()->getCbAddr(cu->getAddr(), zorder) : cu->getPic()->getPicYuvRec()->getCrAddr(cu->getAddr(), zorder);
    uint32_t reconIPredStride = cu->getPic()->getPicYuvRec()->getCStride();
    bool     useTransformSkipChroma = !!cu->getTransformSkip(absPartIdx, ttype);
    int      part = partitionFromSize(tuSize);
    int      sizeIdx = g_convertToBit[tuSize];

    //===== get residual signal =====
    X265_CHECK(!((intptr_t)fenc & (tuSize - 1)), "fenc alignment check fail\n");
    X265_CHECK(!((intptr_t)pred & (tuSize - 1)), "pred alignment check fail\n");
    X265_CHECK(!((intptr_t)residual & (tuSize - 1)), "residual alignment check fail\n");
    primitives.calcresidual[sizeIdx](fenc, pred, residual, stride);

    //===== transform and quantization =====
    {
        //--- init rate estimation arrays for RDOQ ---
        if (useTransformSkipChroma ? m_cfg->bEnableRDOQTS : m_cfg->bEnableRDOQ)
        {
            m_entropyCoder->estimateBit(m_trQuant->m_estBitsSbac, tuSize, ttype);
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

        absSum = m_trQuant->transformNxN(cu, residual, stride, coeff, tuSize, ttype, absPartIdx, &lastPos, useTransformSkipChroma);

        //--- set coded block flag ---
        cu->setCbfPartRange((((absSum > 0) ? 1 : 0) << origTrDepth), ttype, absPartIdx, absPartIdxStep);

        //--- inverse transform ---
        if (absSum)
        {
            int scalingListType = 0 + ttype;
            X265_CHECK(scalingListType < 6, "scalingListType invalid %d\n", scalingListType);
            m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdx), REG_DCT, residual, stride, coeff, tuSize, scalingListType, useTransformSkipChroma, lastPos);
        }
        else
        {
            int16_t* resiTmp = residual;
            memset(coeff, 0, sizeof(coeff_t) * tuSize * tuSize);
            primitives.blockfill_s[sizeIdx](resiTmp, stride, 0);
        }
    }

    X265_CHECK(((intptr_t)residual & (tuSize - 1)) == 0, "residual alignment check failure\n");
    X265_CHECK(tuSize <= 32, "tuSize invalud\n");
    //===== reconstruction =====
    primitives.calcrecon[sizeIdx](pred, residual, reconQt, reconIPred, stride, reconQtStride, reconIPredStride);
    //===== update distortion =====
    uint32_t dist = primitives.sse_sp[part](reconQt, reconQtStride, fenc, stride);
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
    uint32_t trSizeLog2  = cu->getSlice()->getSPS()->getLog2MaxCodingBlockSize() - fullDepth;
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
    int    bestModeId    = 0;

    if (bCheckFull)
    {
        uint32_t tuSize = 1 << trSizeLog2;

        bool checkTransformSkip = (cu->getSlice()->getPPS()->getUseTransformSkip() &&
                                   trSizeLog2 <= LOG2_MAX_TS_SIZE &&
                                   !cu->getCUTransquantBypass(0));
        if (checkTransformSkip)
        {
            checkTransformSkip &= (!((cu->getQP(0) == 0) && (cu->getSlice()->getSPS()->getUseLossless())));
            if (m_cfg->param->bEnableTSkipFast)
            {
                checkTransformSkip &= (cu->getPartitionSize(absPartIdx) == SIZE_NxN);
            }
        }

        uint32_t stride = fencYuv->getStride();
        pixel*   pred   = predYuv->getLumaAddr(absPartIdx);

        //===== init availability pattern =====
        uint32_t lumaPredMode = cu->getLumaIntraDir(absPartIdx);
        TComPattern::initAdiPattern(cu, absPartIdx, trDepth, m_predBuf, m_refAbove, m_refLeft, m_refAboveFlt, m_refLeftFlt, lumaPredMode);

        //===== get prediction signal =====
        predIntraLumaAng(lumaPredMode, pred, stride, tuSize);

        if (checkTransformSkip)
        {
            //----- store original entropy coding status -----
            m_rdGoOnSbacCoder->store(m_rdSbacCoders[fullDepth][CI_QT_TRAFO_ROOT]);

            uint32_t singleDistYTmp = 0;
            uint32_t singleCbfYTmp  = 0;
            uint64_t singleCostTmp  = 0;
            const int firstCheckId  = 0;

            for (int modeId = firstCheckId; modeId < 2; modeId++)
            {
                singleDistYTmp = 0;
                cu->setTransformSkipSubParts(modeId, TEXT_LUMA, absPartIdx, fullDepth);

                //----- code luma block with given intra prediction mode and store Cbf-----
                xIntraCodingLumaBlk(cu, trDepth, absPartIdx, fencYuv, predYuv, resiYuv, singleDistYTmp);
                singleCbfYTmp = cu->getCbf(absPartIdx, TEXT_LUMA, trDepth);

                if (modeId == 1 && singleCbfYTmp == 0)
                {
                    // In order not to code TS flag when cbf is zero, the case for TS with cbf being zero is forbidden.
                    singleCostTmp = MAX_INT64;
                }
                else
                {
                    uint32_t singleBits = xGetIntraBitsQT(cu, trDepth, absPartIdx, 0, true, false);
                    singleCostTmp = m_rdCost->calcRdCost(singleDistYTmp, singleBits);
                }

                if (singleCostTmp < singleCost)
                {
                    singleCost  = singleCostTmp;
                    singleDistY = singleDistYTmp;
                    singleCbfY  = singleCbfYTmp;
                    bestModeId  = modeId;
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
            m_rdGoOnSbacCoder->store(m_rdSbacCoders[fullDepth][CI_QT_TRAFO_ROOT]);

            //----- code luma block with given intra prediction mode and store Cbf-----
            cu->setTransformSkipSubParts(0, TEXT_LUMA, absPartIdx, fullDepth);
            xIntraCodingLumaBlk(cu, trDepth, absPartIdx, fencYuv, predYuv, resiYuv, singleDistY);

            if (bCheckSplit)
                singleCbfY = cu->getCbf(absPartIdx, TEXT_LUMA, trDepth);

            uint32_t singleBits = xGetIntraBitsQT(cu, trDepth, absPartIdx, 0, true, false);
            if (m_cfg->param->rdPenalty && (trSizeLog2 == 5) && !isIntraSlice)
                singleBits *= 4;

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
        uint32_t splitBits = xGetIntraBitsQT(cu, trDepth, absPartIdx, 0, true, false);
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
        X265_CHECK(m_qtTempShortYuv[qtLayer].m_width == MAX_CU_SIZE, "width is not max CU size\n");
        pixel*   dst       = cu->getPic()->getPicYuvRec()->getLumaAddr(cu->getAddr(), zorder);
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
    uint32_t trSizeLog2  = cu->getSlice()->getSPS()->getLog2MaxCodingBlockSize() - fullDepth;
    bool     bCheckFull  = (trSizeLog2 <= cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize());
    bool     bCheckSplit = (trSizeLog2 > cu->getQuadtreeTULog2MinSizeInCU(absPartIdx));

    int maxTuSize = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize();
    int isIntraSlice = (cu->getSlice()->getSliceType() == I_SLICE);

    if (m_cfg->param->rdPenalty == 2 && !isIntraSlice)
    {
        // if maximum RD-penalty don't check TU size 32x32
        bCheckFull = (trSizeLog2 <= X265_MIN(maxTuSize, 4));
    }
    if (bCheckFull)
    {
        cu->setTransformSkipSubParts(0, TEXT_LUMA, absPartIdx, fullDepth);

        //----- code luma block with given intra prediction mode and store Cbf-----
        uint32_t lumaPredMode = cu->getLumaIntraDir(absPartIdx);
        uint32_t tuSize       = cu->getCUSize(0) >> trDepth;
        int      chFmt        = cu->getChromaFormat();
        uint32_t stride       = fencYuv->getStride();
        pixel*   fenc         = fencYuv->getLumaAddr(absPartIdx);
        pixel*   pred         = predYuv->getLumaAddr(absPartIdx);
        int16_t* residual     = resiYuv->getLumaAddr(absPartIdx);
        pixel*   recon        = reconYuv->getLumaAddr(absPartIdx);
        uint32_t coeffOffsetY = absPartIdx << cu->getPic()->getLog2UnitSize() * 2;
        coeff_t* coeff        = cu->getCoeffY() + coeffOffsetY;

        uint32_t zorder           = cu->getZorderIdxInCU() + absPartIdx;
        pixel*   reconIPred       = cu->getPic()->getPicYuvRec()->getLumaAddr(cu->getAddr(), zorder);
        uint32_t reconIPredStride = cu->getPic()->getPicYuvRec()->getStride();

        bool     useTransformSkip = !!cu->getTransformSkip(absPartIdx, TEXT_LUMA);

        //===== init availability pattern =====
        TComPattern::initAdiPattern(cu, absPartIdx, trDepth, m_predBuf, m_refAbove, m_refLeft, m_refAboveFlt, m_refLeftFlt, lumaPredMode);
        //===== get prediction signal =====
        predIntraLumaAng(lumaPredMode, pred, stride, tuSize);

        //===== get residual signal =====
        X265_CHECK(!((intptr_t)fenc & (tuSize - 1)), "fenc alignment failure\n");
        X265_CHECK(!((intptr_t)pred & (tuSize - 1)), "pred alignment failure\n");
        X265_CHECK(!((intptr_t)residual & (tuSize - 1)), "residual alignment failure\n");
        int sizeIdx = g_convertToBit[tuSize];
        primitives.calcresidual[sizeIdx](fenc, pred, residual, stride);

        //===== transform and quantization =====
        uint32_t absSum = 0;
        int lastPos = -1;
        cu->setTrIdxSubParts(trDepth, absPartIdx, fullDepth);

        m_trQuant->setQPforQuant(cu->getQP(0), TEXT_LUMA, QP_BD_OFFSET, 0, chFmt);
        m_trQuant->selectLambda(TEXT_LUMA);
        absSum = m_trQuant->transformNxN(cu, residual, stride, coeff, tuSize, TEXT_LUMA, absPartIdx, &lastPos, useTransformSkip);

        //--- set coded block flag ---
        cu->setCbfSubParts((absSum ? 1 : 0) << trDepth, TEXT_LUMA, absPartIdx, fullDepth);

        //--- inverse transform ---
        if (absSum)
        {
            int scalingListType = 0 + TEXT_LUMA;
            X265_CHECK(scalingListType < 6, "scalingListType %d\n", scalingListType);
            m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdx), cu->getLumaIntraDir(absPartIdx), residual, stride, coeff, tuSize, scalingListType, useTransformSkip, lastPos);
        }
        else
        {
            int16_t* resiTmp = residual;
            memset(coeff, 0, sizeof(coeff_t) * tuSize * tuSize);
            primitives.blockfill_s[sizeIdx](resiTmp, stride, 0);
        }

        //Generate Recon
        X265_CHECK(tuSize <= 32, "tuSize is too large\n");
        int part = partitionFromSize(tuSize);
        primitives.luma_add_ps[part](recon, stride, pred, residual, stride, stride);
        primitives.blockcpy_pp(tuSize, tuSize, reconIPred, reconIPredStride, recon, stride);
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
        uint32_t trSizeLog2 = cu->getSlice()->getSPS()->getLog2MaxCodingBlockSize() - fullDepth;
        uint32_t qtlayer    = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;

        //===== copy transform coefficients =====
        uint32_t numCoeffY    = 1 << (trSizeLog2 * 2);
        uint32_t coeffOffsetY = absPartIdx << cu->getPic()->getLog2UnitSize() * 2;
        coeff_t* coeffSrcY    = m_qtTempCoeffY[qtlayer] + coeffOffsetY;
        coeff_t* coeffDestY   = cu->getCoeffY()         + coeffOffsetY;
        ::memcpy(coeffDestY, coeffSrcY, sizeof(coeff_t) * numCoeffY);

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
    uint32_t fullDepth = cu->getDepth(0) + trDepth;
    uint32_t trSizeLog2 = cu->getSlice()->getSPS()->getLog2MaxCodingBlockSize() - fullDepth;
    uint32_t qtlayer    = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;

    //===== copy transform coefficients =====
    uint32_t numCoeffY    = 1 << (trSizeLog2 * 2);
    uint32_t coeffOffsetY = absPartIdx << cu->getPic()->getLog2UnitSize() * 2;
    coeff_t* coeffSrcY = m_qtTempCoeffY[qtlayer] + coeffOffsetY;
    coeff_t* coeffDstY = m_qtTempTUCoeffY;

    ::memcpy(coeffDstY, coeffSrcY, sizeof(coeff_t) * numCoeffY);

    //===== copy reconstruction =====
    m_qtTempShortYuv[qtlayer].copyPartToPartLuma(&m_qtTempTransformSkipYuv, absPartIdx, 1 << trSizeLog2, 1 << trSizeLog2);
}

void TEncSearch::xLoadIntraResultQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx)
{
    uint32_t fullDepth = cu->getDepth(0) + trDepth;
    uint32_t trSizeLog2 = cu->getSlice()->getSPS()->getLog2MaxCodingBlockSize() - fullDepth;
    uint32_t qtlayer    = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;

    //===== copy transform coefficients =====
    uint32_t numCoeffY    = 1 << (trSizeLog2 * 2);
    uint32_t coeffOffsetY = absPartIdx << cu->getPic()->getLog2UnitSize() * 2;
    coeff_t* coeffDstY = m_qtTempCoeffY[qtlayer] + coeffOffsetY;
    coeff_t* coeffSrcY = m_qtTempTUCoeffY;

    ::memcpy(coeffDstY, coeffSrcY, sizeof(coeff_t) * numCoeffY);

    //===== copy reconstruction =====
    uint32_t trSize = 1 << trSizeLog2;
    m_qtTempTransformSkipYuv.copyPartToPartLuma(&m_qtTempShortYuv[qtlayer], absPartIdx, trSize);

    uint32_t   zOrder           = cu->getZorderIdxInCU() + absPartIdx;
    pixel*     reconIPred       = cu->getPic()->getPicYuvRec()->getLumaAddr(cu->getAddr(), zOrder);
    uint32_t   reconIPredStride = cu->getPic()->getPicYuvRec()->getStride();
    int16_t*   reconQt          = m_qtTempShortYuv[qtlayer].getLumaAddr(absPartIdx);
    primitives.blockcpy_ps(trSize, trSize, reconIPred, reconIPredStride, reconQt, MAX_CU_SIZE);
    X265_CHECK(m_qtTempShortYuv[qtlayer].m_width == MAX_CU_SIZE, "width is not max CU size\n");
}

void TEncSearch::xStoreIntraResultChromaQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, uint32_t chromaId, const bool splitIntoSubTUs)
{
    assert(chromaId == 1 || chromaId == 2);

    uint32_t fullDepth = cu->getDepth(0) + trDepth;
    uint32_t trMode    = cu->getTransformIdx(absPartIdx);

    if (trMode == trDepth)
    {
        uint32_t trSizeLog2 = cu->getSlice()->getSPS()->getLog2MaxCodingBlockSize() - fullDepth;
        uint32_t qtlayer    = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;
        int      chFmt      = cu->getChromaFormat();

        bool bChromaSame = false;
        if (trSizeLog2 == 2 && !(chFmt == CHROMA_444))
        {
            X265_CHECK(trDepth > 0, "invalid trDepth\n");
            trDepth--;
            uint32_t qpdiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trDepth) << 1);
            bool bFirstQ = ((absPartIdx & (qpdiv - 1)) == 0);
            bool bSecondQ = (chFmt == CHROMA_422) ? ((absPartIdx & (qpdiv - 1)) == 2) : false;
            if ((!bFirstQ) && (!bSecondQ))
            {
                return;
            }
            bChromaSame = true;
        }
        uint32_t width  = cu->getCUSize(absPartIdx) >> (trDepth + m_hChromaShift);
        uint32_t height = cu->getCUSize(absPartIdx) >> (trDepth + m_vChromaShift);
        height = splitIntoSubTUs ? height >> 1 : height;
        uint32_t numCoeffC = width * height;
        uint32_t coeffOffsetC = absPartIdx << (cu->getPic()->getLog2UnitSize() * 2 - (m_hChromaShift + m_vChromaShift));

        if (chromaId == 1)
        {
            coeff_t* coeffSrcU = m_qtTempCoeffCb[qtlayer] + coeffOffsetC;
            coeff_t* coeffDstU = m_qtTempTUCoeffCb;
            ::memcpy(coeffDstU, coeffSrcU, sizeof(coeff_t) * numCoeffC);
        }
        if (chromaId == 2)
        {
            coeff_t* coeffSrcV = m_qtTempCoeffCr[qtlayer] + coeffOffsetC;
            coeff_t* coeffDstV = m_qtTempTUCoeffCr;
            ::memcpy(coeffDstV, coeffSrcV, sizeof(coeff_t) * numCoeffC);
        }

        //===== copy reconstruction =====
        uint32_t lumaSize = 1 << (bChromaSame ? trSizeLog2 + 1 : trSizeLog2);
        m_qtTempShortYuv[qtlayer].copyPartToPartYuvChroma(&m_qtTempTransformSkipYuv, absPartIdx, lumaSize, chromaId, splitIntoSubTUs);
    }
}

void TEncSearch::xLoadIntraResultChromaQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, uint32_t chromaId, const bool splitIntoSubTUs)
{
    assert(chromaId == 1 || chromaId == 2);

    uint32_t fullDepth = cu->getDepth(0) + trDepth;
    uint32_t trMode    = cu->getTransformIdx(absPartIdx);

    if (trMode == trDepth)
    {
        uint32_t trSizeLog2 = cu->getSlice()->getSPS()->getLog2MaxCodingBlockSize() - fullDepth;
        uint32_t qtlayer    = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;
        int      chFmt      = cu->getChromaFormat();

        bool bChromaSame = false;
        if (trSizeLog2 == 2 && !(chFmt == CHROMA_444))
        {
            X265_CHECK(trDepth > 0, "invalid trDepth\n");
            trDepth--;
            uint32_t qpdiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trDepth) << 1);
            bool bFirstQ = ((absPartIdx & (qpdiv - 1)) == 0);
            bool bSecondQ = (chFmt == CHROMA_422) ? ((absPartIdx & (qpdiv - 1)) == 2) : false;
            if ((!bFirstQ) && (!bSecondQ))
            {
                return;
            }
            bChromaSame = true;
        }

        //===== copy transform coefficients =====
        uint32_t trWidthC  = cu->getCUSize(absPartIdx) >> (trDepth + m_hChromaShift);
        uint32_t trHeightC = cu->getCUSize(absPartIdx) >> (trDepth + m_vChromaShift);
        trHeightC = splitIntoSubTUs ? trHeightC >> 1 : trHeightC;
        uint32_t numCoeffC = trWidthC * trHeightC;
        uint32_t coeffOffsetC = absPartIdx << (cu->getPic()->getLog2UnitSize() * 2 - (m_hChromaShift + m_vChromaShift));

        if (chromaId == 1)
        {
            coeff_t* coeffDstU = m_qtTempCoeffCb[qtlayer] + coeffOffsetC;
            coeff_t* coeffSrcU = m_qtTempTUCoeffCb;
            ::memcpy(coeffDstU, coeffSrcU, sizeof(coeff_t) * numCoeffC);
        }
        if (chromaId == 2)
        {
            coeff_t* coeffDstV = m_qtTempCoeffCr[qtlayer] + coeffOffsetC;
            coeff_t* coeffSrcV = m_qtTempTUCoeffCr;
            ::memcpy(coeffDstV, coeffSrcV, sizeof(coeff_t) * numCoeffC);
        }

        //===== copy reconstruction =====
        uint32_t lumaSize = 1 << (bChromaSame ? trSizeLog2 + 1 : trSizeLog2);
        m_qtTempTransformSkipYuv.copyPartToPartChroma(&m_qtTempShortYuv[qtlayer], absPartIdx, lumaSize, chromaId, splitIntoSubTUs);

        uint32_t zorder           = cu->getZorderIdxInCU() + absPartIdx;
        uint32_t reconQtStride    = m_qtTempShortYuv[qtlayer].m_cwidth;
        uint32_t reconIPredStride = cu->getPic()->getPicYuvRec()->getCStride();

        if (chromaId == 1)
        {
            pixel* reconIPred = cu->getPic()->getPicYuvRec()->getCbAddr(cu->getAddr(), zorder);
            int16_t* reconQt  = m_qtTempShortYuv[qtlayer].getCbAddr(absPartIdx);
            primitives.blockcpy_ps(trWidthC, trHeightC, reconIPred, reconIPredStride, reconQt, reconQtStride);
        }
        if (chromaId == 2)
        {
            pixel* reconIPred = cu->getPic()->getPicYuvRec()->getCrAddr(cu->getAddr(), zorder);
            int16_t* reconQt  = m_qtTempShortYuv[qtlayer].getCrAddr(absPartIdx);
            primitives.blockcpy_ps(trWidthC, trHeightC, reconIPred, reconIPredStride, reconQt, reconQtStride);
        }
    }
}

void TEncSearch::offsetSubTUCBFs(TComDataCU* cu, TextType ttype, uint32_t trDepth, uint32_t absPartIdx)
{
    uint32_t depth = cu->getDepth(0);
    uint32_t fullDepth = depth + trDepth;
    uint32_t trSizeLog2 = cu->getSlice()->getSPS()->getLog2MaxCodingBlockSize() - fullDepth;

    uint32_t actualTrDepth = trDepth;

    if ((trSizeLog2 == 2) && !(cu->getChromaFormat() == CHROMA_444))
    {
        X265_CHECK(actualTrDepth > 0, "actualTrDepth invalid\n");
        actualTrDepth--;
    }

    uint32_t partIdxesPerSubTU     = (cu->getPic()->getNumPartInCU() >> ((depth + actualTrDepth) << 1)) >> 1;

    //move the CBFs down a level and set the parent CBF
    uint8_t subTUCBF[2];
    uint8_t combinedSubTUCBF = 0;

    for (uint32_t subTU = 0; subTU < 2; subTU++)
    {
        const uint32_t subTUAbsPartIdx = absPartIdx + (subTU * partIdxesPerSubTU);

        subTUCBF[subTU]   = cu->getCbf(subTUAbsPartIdx, ttype, trDepth);
        combinedSubTUCBF |= subTUCBF[subTU];
    }

    for (uint32_t subTU = 0; subTU < 2; subTU++)
    {
        const uint32_t subTUAbsPartIdx = absPartIdx + (subTU * partIdxesPerSubTU);
        const uint8_t compositeCBF = (subTUCBF[subTU] << 1) | combinedSubTUCBF;

        cu->setCbfPartRange((compositeCBF << trDepth), ttype, subTUAbsPartIdx, partIdxesPerSubTU);
    }
}

void TEncSearch::xRecurIntraChromaCodingQT(TComDataCU* cu,
                                           uint32_t    trDepth,
                                           uint32_t    absPartIdx,
                                           TComYuv*    fencYuv,
                                           TComYuv*    predYuv,
                                           ShortYuv*   resiYuv,
                                           uint32_t&   outDist)
{
    uint32_t fullDepth = cu->getDepth(0) + trDepth;
    uint32_t trMode    = cu->getTransformIdx(absPartIdx);

    if (trMode == trDepth)
    {
        int chFmt = cu->getChromaFormat();
        uint32_t trSizeLog2 = cu->getSlice()->getSPS()->getLog2MaxCodingBlockSize() - fullDepth;
        uint32_t trSizeCLog2 = trSizeLog2 - m_hChromaShift;
        uint32_t actualTrDepth = trDepth;
        if ((trSizeLog2 == 2) && !(chFmt == CHROMA_444))
        {
            X265_CHECK(trDepth > 0, "invalid trDepth\n");
            actualTrDepth--;
            trSizeCLog2++;
            uint32_t qpdiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + actualTrDepth) << 1);
            bool bFirstQ = ((absPartIdx & (qpdiv - 1)) == 0);
            if (!bFirstQ)
            {
                return;
            }
        }

        uint32_t tuSize = cu->getCUSize(0) >> (actualTrDepth + m_hChromaShift);
        uint32_t stride = fencYuv->getCStride();
        const bool splitIntoSubTUs = (chFmt == CHROMA_422);

        bool checkTransformSkip = (cu->getSlice()->getPPS()->getUseTransformSkip() &&
                                   trSizeCLog2 <= LOG2_MAX_TS_SIZE &&
                                   !cu->getCUTransquantBypass(0));

        if (m_cfg->param->bEnableTSkipFast)
        {
            checkTransformSkip &= ((cu->getCUSize(0) >> trDepth) <= 4);
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

        for (int chromaId = TEXT_CHROMA; chromaId < MAX_NUM_COMPONENT; chromaId++)
        {
            TComTURecurse tuIterator;
            uint32_t curPartNum = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) +  actualTrDepth) << 1);
            initSection(&tuIterator, splitIntoSubTUs ? VERTICAL_SPLIT : DONT_SPLIT, curPartNum, absPartIdx);

            do
            {
                uint32_t absPartIdxC = tuIterator.m_absPartIdxTURelCU;
                pixel*   pred        = (chromaId == 1) ? predYuv->getCbAddr(absPartIdxC) : predYuv->getCrAddr(absPartIdxC);

                //===== init availability pattern =====
                TComPattern::initAdiPatternChroma(cu, absPartIdxC, actualTrDepth, m_predBuf, chromaId);
                pixel* chromaPred = TComPattern::getAdiChromaBuf(chromaId, tuSize, m_predBuf);

                uint32_t chromaPredMode = cu->getChromaIntraDir(absPartIdxC);
                //===== update chroma mode =====
                if (chromaPredMode == DM_CHROMA_IDX)
                {
                    uint32_t lumaLCUIdx  = (chFmt == CHROMA_444) ? absPartIdxC : absPartIdxC & (~((1 << (2 * g_addCUDepth)) - 1));
                    chromaPredMode = cu->getLumaIntraDir(lumaLCUIdx);
                }
                chromaPredMode = (chFmt == CHROMA_422) ? g_chroma422IntraAngleMappingTable[chromaPredMode] : chromaPredMode;
                //===== get prediction signal =====
                predIntraChromaAng(chromaPred, chromaPredMode, pred, stride, tuSize, chFmt);

                if (checkTransformSkip)
                {
                    // use RDO to decide whether Cr/Cb takes TS
                    m_rdGoOnSbacCoder->store(m_rdSbacCoders[fullDepth][CI_QT_TRAFO_ROOT]);

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
                        cu->setTransformSkipPartRange(chromaModeId, (TextType)chromaId, absPartIdxC, tuIterator.m_absPartIdxStep);

                        singleDistCTmp = 0;
                        xIntraCodingChromaBlk(cu, trDepth, absPartIdxC, tuIterator.m_absPartIdxStep, fencYuv, predYuv, resiYuv, singleDistCTmp, chromaId);

                        singleCbfCTmp = cu->getCbf(absPartIdxC, (TextType)chromaId, trDepth);

                        if (chromaModeId == 1 && singleCbfCTmp == 0)
                        {
                            //In order not to code TS flag when cbf is zero, the case for TS with cbf being zero is forbidden.
                            singleCostTmp = MAX_INT64;
                        }
                        else
                        {
                            uint32_t bitsTmp = xGetIntraBitsQTChroma(cu, trDepth, absPartIdxC, chromaId, splitIntoSubTUs);
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
                                xStoreIntraResultChromaQT(cu, trDepth, absPartIdxC, chromaId, splitIntoSubTUs);
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
                        xLoadIntraResultChromaQT(cu, trDepth, absPartIdxC, chromaId, splitIntoSubTUs);
                        cu->setCbfPartRange(singleCbfC << trDepth, (TextType)chromaId, absPartIdxC, tuIterator.m_absPartIdxStep);

                        m_rdGoOnSbacCoder->load(m_rdSbacCoders[fullDepth][CI_TEMP_BEST]);
                    }

                    cu->setTransformSkipPartRange(bestModeId, (TextType)chromaId, absPartIdxC, tuIterator.m_absPartIdxStep);

                    outDist += singleDistC;

                    if (chromaId == 1)
                    {
                        m_rdGoOnSbacCoder->store(m_rdSbacCoders[fullDepth][CI_QT_TRAFO_ROOT]);
                    }
                }
                else
                {
                    cu->setTransformSkipPartRange(0, (TextType)chromaId, absPartIdxC, tuIterator.m_absPartIdxStep);
                    xIntraCodingChromaBlk(cu, trDepth, absPartIdxC, tuIterator.m_absPartIdxStep, fencYuv, predYuv, resiYuv, outDist, chromaId);
                }
            }
            while (isNextSection(&tuIterator));

            if (splitIntoSubTUs)
            {
                offsetSubTUCBFs(cu, (TextType)chromaId, trDepth, absPartIdx);
            }
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
        int      chFmt      = cu->getChromaFormat();
        uint32_t trSizeLog2 = cu->getSlice()->getSPS()->getLog2MaxCodingBlockSize() - fullDepth;
        uint32_t qtlayer    = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;

        bool bChromaSame = false;
        if ((trSizeLog2 == 2) && !(chFmt == CHROMA_444))
        {
            X265_CHECK(trDepth > 0, "invalid trDepth\n");
            trDepth--;
            uint32_t qpdiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trDepth) << 1);
            if ((absPartIdx & (qpdiv - 1)) != 0)
            {
                return;
            }
            bChromaSame = true;
        }

        //===== copy transform coefficients =====

        uint32_t width     = cu->getCUSize(absPartIdx) >> (trDepth + m_hChromaShift);
        uint32_t height    = cu->getCUSize(absPartIdx) >> (trDepth + m_vChromaShift);
        uint32_t numCoeffC = width * height;
        uint32_t coeffOffsetC = absPartIdx << (cu->getPic()->getLog2UnitSize() * 2 - (m_hChromaShift + m_vChromaShift));

        coeff_t* coeffSrcU = m_qtTempCoeffCb[qtlayer] + coeffOffsetC;
        coeff_t* coeffSrcV = m_qtTempCoeffCr[qtlayer] + coeffOffsetC;
        coeff_t* coeffDstU = cu->getCoeffCb()         + coeffOffsetC;
        coeff_t* coeffDstV = cu->getCoeffCr()         + coeffOffsetC;
        ::memcpy(coeffDstU, coeffSrcU, sizeof(coeff_t) * numCoeffC);
        ::memcpy(coeffDstV, coeffSrcV, sizeof(coeff_t) * numCoeffC);

        //===== copy reconstruction =====
        m_qtTempShortYuv[qtlayer].copyPartToPartChroma(reconYuv, absPartIdx, 1 << trSizeLog2, (bChromaSame && (chFmt != CHROMA_422)));
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
                                       ShortYuv*   resiYuv,
                                       TComYuv*    reconYuv)
{
    uint32_t fullDepth = cu->getDepth(0) + trDepth;
    uint32_t trMode    = cu->getTransformIdx(absPartIdx);

    if (trMode == trDepth)
    {
        int      chFmt     = cu->getChromaFormat();
        uint32_t trSizeLog2 = cu->getSlice()->getSPS()->getLog2MaxCodingBlockSize() - fullDepth;
        uint32_t origTrDepth = trDepth;
        uint32_t actualTrDepth = trDepth;
        if ((trSizeLog2 == 2) && !(chFmt == CHROMA_444))
        {
            X265_CHECK(trDepth > 0, "invalid trDepth\n");
            actualTrDepth--;
            uint32_t qpdiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + actualTrDepth) << 1);
            bool bFirstQ = ((absPartIdx & (qpdiv - 1)) == 0);
            if (!bFirstQ)
            {
                return;
            }
        }

        uint32_t tuSize = cu->getCUSize(0) >> (actualTrDepth + m_hChromaShift);
        uint32_t stride = fencYuv->getCStride();
        const bool splitIntoSubTUs = (chFmt == CHROMA_422);
        int sizeIdx = g_convertToBit[tuSize];

        for (int chromaId = TEXT_CHROMA; chromaId < MAX_NUM_COMPONENT; chromaId++)
        {
            TComTURecurse tuIterator;
            uint32_t curPartNum = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) +  actualTrDepth) << 1);
            initSection(&tuIterator, splitIntoSubTUs ? VERTICAL_SPLIT : DONT_SPLIT, curPartNum, absPartIdx);

            do
            {
                uint32_t absPartIdxC = tuIterator.m_absPartIdxTURelCU;
                cu->setTransformSkipPartRange(0, (TextType)chromaId, absPartIdxC, tuIterator.m_absPartIdxStep);

                TextType ttype          = (chromaId == 1) ? TEXT_CHROMA_U : TEXT_CHROMA_V;
                pixel*   fenc           = (chromaId == 1) ? fencYuv->getCbAddr(absPartIdxC) : fencYuv->getCrAddr(absPartIdxC);
                pixel*   pred           = (chromaId == 1) ? predYuv->getCbAddr(absPartIdxC) : predYuv->getCrAddr(absPartIdxC);
                int16_t* residual       = (chromaId == 1) ? resiYuv->getCbAddr(absPartIdxC) : resiYuv->getCrAddr(absPartIdxC);
                pixel*   recon          = (chromaId == 1) ? reconYuv->getCbAddr(absPartIdxC) : reconYuv->getCrAddr(absPartIdxC);
                uint32_t coeffOffsetC   = absPartIdxC << (cu->getPic()->getLog2UnitSize() * 2 - (m_hChromaShift + m_vChromaShift));
                coeff_t* coeff          = (chromaId == 1 ? cu->getCoeffCb() : cu->getCoeffCr()) + coeffOffsetC;
                uint32_t zorder         = cu->getZorderIdxInCU() + absPartIdxC;
                pixel*   reconIPred     = (chromaId == 1) ? cu->getPic()->getPicYuvRec()->getCbAddr(cu->getAddr(), zorder) : cu->getPic()->getPicYuvRec()->getCrAddr(cu->getAddr(), zorder);
                uint32_t reconIPredStride = cu->getPic()->getPicYuvRec()->getCStride();
                //bool     useTransformSkipChroma = cu->getTransformSkip(absPartIdxC, ttype);
                const bool useTransformSkipChroma = false;

                uint32_t chromaPredMode = cu->getChromaIntraDir(absPartIdxC);
                //===== update chroma mode =====
                if (chromaPredMode == DM_CHROMA_IDX)
                {
                    uint32_t lumaLCUIdx  = (chFmt == CHROMA_444) ? absPartIdxC : absPartIdxC & (~((1 << (2 * g_addCUDepth)) - 1));
                    chromaPredMode = cu->getLumaIntraDir(lumaLCUIdx);
                }
                chromaPredMode = (chFmt == CHROMA_422) ? g_chroma422IntraAngleMappingTable[chromaPredMode] : chromaPredMode;
                //===== init availability pattern =====
                TComPattern::initAdiPatternChroma(cu, absPartIdxC, actualTrDepth, m_predBuf, chromaId);
                pixel* chromaPred = TComPattern::getAdiChromaBuf(chromaId, tuSize, m_predBuf);

                //===== get prediction signal =====
                predIntraChromaAng(chromaPred, chromaPredMode, pred, stride, tuSize, chFmt);

                //===== get residual signal =====
                X265_CHECK(!((intptr_t)fenc & (tuSize - 1)), "fenc alignment failure\n");
                X265_CHECK(!((intptr_t)pred & (tuSize - 1)), "pred alignment failure\n");
                X265_CHECK(!((intptr_t)residual & (tuSize - 1)), "residual alignment failure\n");
                primitives.calcresidual[sizeIdx](fenc, pred, residual, stride);

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

                absSum = m_trQuant->transformNxN(cu, residual, stride, coeff, tuSize, ttype, absPartIdxC, &lastPos, useTransformSkipChroma);

                //--- set coded block flag ---
                cu->setCbfPartRange((((absSum > 0) ? 1 : 0) << origTrDepth), ttype, absPartIdxC, tuIterator.m_absPartIdxStep);

                //--- inverse transform ---
                if (absSum)
                {
                    int scalingListType = 0 + ttype;
                    X265_CHECK(scalingListType < 6, "scalingListType too large %d\n", scalingListType);
                    m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdxC), REG_DCT, residual, stride, coeff, tuSize, scalingListType, useTransformSkipChroma, lastPos);
                }
                else
                {
                    int16_t* resiTmp = residual;
                    memset(coeff, 0, sizeof(coeff_t) * tuSize * tuSize);
                    primitives.blockfill_s[sizeIdx](resiTmp, stride, 0);
                }

                //===== reconstruction =====
                X265_CHECK(((intptr_t)residual & (tuSize - 1)) == 0, "residual alignment check failed\n");
                X265_CHECK(tuSize <= 32, "tuSize out of range\n");

                // use square primitive
                int part = partitionFromSize(tuSize);
                primitives.chroma[CHROMA_444].add_ps[part](recon, stride, pred, residual, stride, stride);
                primitives.chroma[CHROMA_444].copy_pp[part](reconIPred, reconIPredStride, recon, stride);
            }
            while (isNextSection(&tuIterator));

            if (splitIntoSubTUs)
            {
                offsetSubTUCBFs(cu, (TextType)chromaId, trDepth, absPartIdx);
            }
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
    uint32_t tuSize       = cu->getCUSize(0) >> initTrDepth;
    uint32_t qNumParts    = cu->getTotalNumPart() >> 2;
    uint32_t qPartNum     = cu->getPic()->getNumPartInCU() >> ((depth + initTrDepth) << 1);
    uint32_t overallDistY = 0;
    uint32_t candNum;
    uint64_t candCostList[FAST_UDI_MAX_RDMODE_NUM];
    uint32_t sizeIdx      = g_convertToBit[tuSize]; // log2(tuSize) - 2
    static const uint8_t intraModeNumFast[] = { 8, 8, 3, 3, 3 }; // 4x4, 8x8, 16x16, 32x32, 64x64

    //===== loop over partitions =====
    uint32_t partOffset = 0;

    for (uint32_t pu = 0; pu < numPU; pu++, partOffset += qNumParts)
    {
        // Reference sample smoothing
        TComPattern::initAdiPattern(cu, partOffset, initTrDepth, m_predBuf, m_refAbove, m_refLeft, m_refAboveFlt, m_refLeftFlt, ALL_IDX);

        //===== determine set of modes to be tested (using prediction signal only) =====
        const int numModesAvailable = 35; //total number of Intra modes
        pixel*   fenc   = fencYuv->getLumaAddr(pu, tuSize);
        uint32_t stride = predYuv->getStride();
        uint32_t rdModeList[FAST_UDI_MAX_RDMODE_NUM];
        int numModesForFullRD = intraModeNumFast[sizeIdx];

        bool doFastSearch = (numModesForFullRD != numModesAvailable);
        if (doFastSearch)
        {
            X265_CHECK(numModesForFullRD < numModesAvailable, "numModesAvailable too large\n");

            for (int i = 0; i < numModesForFullRD; i++)
            {
                candCostList[i] = MAX_INT64;
            }

            candNum = 0;
            uint32_t modeCosts[35];

            pixel *above         = m_refAbove    + tuSize - 1;
            pixel *aboveFiltered = m_refAboveFlt + tuSize - 1;
            pixel *left          = m_refLeft     + tuSize - 1;
            pixel *leftFiltered  = m_refLeftFlt  + tuSize - 1;

            // 33 Angle modes once
            ALIGN_VAR_32(pixel, buf_trans[32 * 32]);
            ALIGN_VAR_32(pixel, tmp[33 * 32 * 32]);
            ALIGN_VAR_32(pixel, bufScale[32 * 32]);
            pixel _above[4 * 32 + 1];
            pixel _left[4 * 32 + 1];
            int scaleTuSize = tuSize;
            int scaleStride = stride;
            int costShift = 0;

            if (tuSize > 32)
            {
                pixel *aboveScale  = _above + 2 * 32;
                pixel *leftScale   = _left + 2 * 32;

                // origin is 64x64, we scale to 32x32 and setup required parameters
                primitives.scale2D_64to32(bufScale, fenc, stride);
                fenc = bufScale;

                // reserve space in case primitives need to store data in above
                // or left buffers
                aboveScale[0] = leftScale[0] = above[0];
                primitives.scale1D_128to64(aboveScale + 1, above + 1, 0);
                primitives.scale1D_128to64(leftScale + 1, left + 1, 0);

                scaleTuSize = 32;
                scaleStride = 32;
                costShift = 2;
                sizeIdx = 5 - 2; // g_convertToBit[scaleTuSize];

                // Filtered and Unfiltered refAbove and refLeft pointing to above and left.
                above         = aboveScale;
                left          = leftScale;
                aboveFiltered = aboveScale;
                leftFiltered  = leftScale;
            }

            pixelcmp_t sa8d = primitives.sa8d[sizeIdx];

            // DC
            primitives.intra_pred[sizeIdx][DC_IDX](tmp, scaleStride, left, above, 0, (scaleTuSize <= 16));
            modeCosts[DC_IDX] = sa8d(fenc, scaleStride, tmp, scaleStride) << costShift;

            pixel *abovePlanar   = above;
            pixel *leftPlanar    = left;

            if (tuSize >= 8 && tuSize <= 32)
            {
                abovePlanar = aboveFiltered;
                leftPlanar  = leftFiltered;
            }

            // PLANAR
            primitives.intra_pred[sizeIdx][PLANAR_IDX](tmp, scaleStride, leftPlanar, abovePlanar, 0, 0);
            modeCosts[PLANAR_IDX] = sa8d(fenc, scaleStride, tmp, scaleStride) << costShift;

            // Transpose NxN
            primitives.transpose[sizeIdx](buf_trans, fenc, scaleStride);

            primitives.intra_pred_allangs[sizeIdx](tmp, above, left, aboveFiltered, leftFiltered, (scaleTuSize <= 16));

            for (uint32_t mode = 2; mode < numModesAvailable; mode++)
            {
                bool modeHor = (mode < 18);
                pixel *cmp = (modeHor ? buf_trans : fenc);
                intptr_t srcStride = (modeHor ? scaleTuSize : scaleStride);
                modeCosts[mode] = sa8d(cmp, srcStride, &tmp[(mode - 2) * (scaleTuSize * scaleTuSize)], scaleTuSize) << costShift;
            }

            uint32_t preds[3];
            int numCand = cu->getIntraDirLumaPredictor(partOffset, preds);

            uint64_t mpms;
            uint32_t rbits = xModeBitsRemIntra(cu, partOffset, depth, preds, mpms);

            // Find N least cost modes. N = numModesForFullRD
            for (uint32_t mode = 0; mode < numModesAvailable; mode++)
            {
                uint32_t sad = modeCosts[mode];
                uint32_t bits = !(mpms & ((uint64_t)1 << mode)) ? rbits : xModeBitsIntra(cu, mode, partOffset, depth);
                uint64_t cost = m_rdCost->calcRdSADCost(sad, bits);
                candNum += xUpdateCandList(mode, cost, numModesForFullRD, rdModeList, candCostList);
            }

            for (int j = 0; j < numCand; j++)
            {
                bool mostProbableModeIncluded = false;
                uint32_t mostProbableMode = preds[j];

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

                ::memcpy(m_qtTempTrIdx,  cu->getTransformIdx()     + partOffset, qPartNum * sizeof(uint8_t));
                ::memcpy(m_qtTempCbf[0], cu->getCbf(TEXT_LUMA)     + partOffset, qPartNum * sizeof(uint8_t));
                ::memcpy(m_qtTempTransformSkipFlag[0], cu->getTransformSkip(TEXT_LUMA)     + partOffset, qPartNum * sizeof(uint8_t));
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

                ::memcpy(m_qtTempTrIdx,  cu->getTransformIdx()     + partOffset, qPartNum * sizeof(uint8_t));
                ::memcpy(m_qtTempCbf[0], cu->getCbf(TEXT_LUMA)     + partOffset, qPartNum * sizeof(uint8_t));
                ::memcpy(m_qtTempTransformSkipFlag[0], cu->getTransformSkip(TEXT_LUMA)     + partOffset, qPartNum * sizeof(uint8_t));
            }
        } // Mode loop

        //--- update overall distortion ---
        overallDistY += bestPUDistY;

        //--- update transform index and cbf ---
        ::memcpy(cu->getTransformIdx()     + partOffset, m_qtTempTrIdx,  qPartNum * sizeof(uint8_t));
        ::memcpy(cu->getCbf(TEXT_LUMA)     + partOffset, m_qtTempCbf[0], qPartNum * sizeof(uint8_t));
        ::memcpy(cu->getTransformSkip(TEXT_LUMA)     + partOffset, m_qtTempTransformSkipFlag[0], qPartNum * sizeof(uint8_t));
        //--- set reconstruction for next intra prediction blocks ---
        if (pu != numPU - 1)
        {
            uint32_t zorder      = cu->getZorderIdxInCU() + partOffset;
            int      part        = partitionFromSize(tuSize);
            pixel*   dst         = cu->getPic()->getPicYuvRec()->getLumaAddr(cu->getAddr(), zorder);
            uint32_t dststride   = cu->getPic()->getPicYuvRec()->getStride();
            pixel*   src         = reconYuv->getLumaAddr(partOffset);
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

    uint32_t tuSize         = cu->getCUSize(0) >> (trDepth + m_hChromaShift);
    int      chFmt          = cu->getChromaFormat();
    uint32_t stride         = fencYuv->getCStride();
    int scaleTuSize = tuSize;
    int costShift = 0;

    if (tuSize > 32)
    {
        scaleTuSize = 32;
        costShift = 2;
    }
    int sizeIdx = g_convertToBit[scaleTuSize];
    pixelcmp_t sa8d = primitives.sa8d[sizeIdx];

    TComPattern::initAdiPatternChroma(cu, absPartIdx, trDepth, m_predBuf, 1);
    TComPattern::initAdiPatternChroma(cu, absPartIdx, trDepth, m_predBuf, 2);
    cu->getAllowedChromaDir(0, modeList);
    //----- check chroma modes -----
    for (uint32_t mode = minMode; mode < maxMode; mode++)
    {
        uint32_t chromaPredMode = modeList[mode];
        if (chromaPredMode == DM_CHROMA_IDX)
        {
            chromaPredMode = cu->getLumaIntraDir(0);
        }
        chromaPredMode = (chFmt == CHROMA_422) ? g_chroma422IntraAngleMappingTable[chromaPredMode] : chromaPredMode;
        uint64_t cost = 0;
        for (int chromaId = 0; chromaId < 2; chromaId++)
        {
            pixel* fenc = (chromaId > 0 ? fencYuv->getCrAddr(absPartIdx) : fencYuv->getCbAddr(absPartIdx));
            pixel* pred = (chromaId > 0 ? predYuv->getCrAddr(absPartIdx) : predYuv->getCbAddr(absPartIdx));
            pixel* chromaPred = TComPattern::getAdiChromaBuf(chromaId + 1, tuSize, m_predBuf);

            //===== get prediction signal =====
            predIntraChromaAng(chromaPred, chromaPredMode, pred, stride, scaleTuSize, chFmt);
            cost += sa8d(fenc, stride, pred, stride) << costShift;
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

bool TEncSearch::isNextSection(TComTURecurse *tuIterator)
{
    if (tuIterator->m_splitMode == DONT_SPLIT)
    {
        tuIterator->m_section++;
        return false;
    }
    else
    {
        tuIterator->m_absPartIdxTURelCU += tuIterator->m_absPartIdxStep;

        tuIterator->m_section++;
        return tuIterator->m_section < (1 << tuIterator->m_splitMode);
    }
}

bool TEncSearch::isLastSection(TComTURecurse *tuIterator)
{
    return (tuIterator->m_section + 1) >= (1 << tuIterator->m_splitMode);
}

void TEncSearch::initSection(TComTURecurse *tuIterator, uint32_t splitMode, uint32_t absPartIdxStep, uint32_t m_absPartIdxTU)
{
    tuIterator->m_partOffset        = 0;
    tuIterator->m_section           = 0;
    tuIterator->m_absPartIdxTURelCU = m_absPartIdxTU;
    tuIterator->m_splitMode         = splitMode;
    tuIterator->m_absPartIdxStep    = absPartIdxStep >> partIdxStepShift[splitMode];
}

void TEncSearch::estIntraPredChromaQT(TComDataCU* cu,
                                      TComYuv*    fencYuv,
                                      TComYuv*    predYuv,
                                      ShortYuv*   resiYuv,
                                      TComYuv*    reconYuv)
{
    uint32_t depth              = cu->getDepth(0);
    uint32_t initTrDepth        = (cu->getPartitionSize(0) != SIZE_2Nx2N) && (cu->getChromaFormat() == CHROMA_444 ? 1 : 0);

    uint32_t splitMode          = (initTrDepth == 0) ? DONT_SPLIT : QUAD_SPLIT;
    uint32_t absPartIdx         = (cu->getPic()->getNumPartInCU() >> (depth << 1));

    TComTURecurse tuIterator;

    initSection(&tuIterator, splitMode, absPartIdx);

    do
    {
        uint32_t bestMode           = 0;
        uint32_t bestDist           = 0;
        uint64_t bestCost           = MAX_INT64;

        //----- init mode list -----
        uint32_t minMode = 0;
        uint32_t maxMode = NUM_CHROMA_MODE;
        uint32_t modeList[NUM_CHROMA_MODE];

        tuIterator.m_partOffset = tuIterator.m_absPartIdxTURelCU;

        cu->getAllowedChromaDir(tuIterator.m_partOffset, modeList);

        //----- check chroma modes -----
        for (uint32_t mode = minMode; mode < maxMode; mode++)
        {
            //----- restore context models -----
            m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_CURR_BEST]);

            //----- chroma coding -----
            uint32_t dist = 0;

            cu->setChromIntraDirSubParts(modeList[mode], tuIterator.m_partOffset, depth + initTrDepth);

            xRecurIntraChromaCodingQT(cu, initTrDepth, tuIterator.m_absPartIdxTURelCU, fencYuv, predYuv, resiYuv, dist);

            if (cu->getSlice()->getPPS()->getUseTransformSkip())
            {
                m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_CURR_BEST]);
            }

            uint32_t   bits = xGetIntraBitsQT(cu, initTrDepth, tuIterator.m_absPartIdxTURelCU, tuIterator.m_absPartIdxStep, false, true);
            uint64_t cost  = m_rdCost->calcRdCost(dist, bits);

            //----- compare -----
            if (cost < bestCost)
            {
                bestCost = cost;
                bestDist = dist;
                bestMode = modeList[mode];
                xSetIntraResultChromaQT(cu, initTrDepth, tuIterator.m_absPartIdxTURelCU, reconYuv);
                ::memcpy(m_qtTempCbf[1], cu->getCbf(TEXT_CHROMA_U) + tuIterator.m_partOffset, tuIterator.m_absPartIdxStep * sizeof(uint8_t));
                ::memcpy(m_qtTempCbf[2], cu->getCbf(TEXT_CHROMA_V) + tuIterator.m_partOffset, tuIterator.m_absPartIdxStep * sizeof(uint8_t));
                ::memcpy(m_qtTempTransformSkipFlag[1], cu->getTransformSkip(TEXT_CHROMA_U) + tuIterator.m_partOffset, tuIterator.m_absPartIdxStep * sizeof(uint8_t));
                ::memcpy(m_qtTempTransformSkipFlag[2], cu->getTransformSkip(TEXT_CHROMA_V) + tuIterator.m_partOffset, tuIterator.m_absPartIdxStep * sizeof(uint8_t));
            }
        }

        if (!isLastSection(&tuIterator))
        {
            uint32_t compWidth   = (cu->getCUSize(0) >> m_hChromaShift) >> initTrDepth;
            uint32_t compHeight  = (cu->getCUSize(0) >> m_vChromaShift) >> initTrDepth;
            uint32_t zorder      = cu->getZorderIdxInCU() + tuIterator.m_partOffset;
            pixel*     dst         = cu->getPic()->getPicYuvRec()->getCbAddr(cu->getAddr(), zorder);
            uint32_t dststride   = cu->getPic()->getPicYuvRec()->getCStride();
            pixel*     src         = reconYuv->getCbAddr(tuIterator.m_partOffset);
            uint32_t srcstride   = reconYuv->getCStride();

            primitives.blockcpy_pp(compWidth, compHeight, dst, dststride, src, srcstride);

            dst                 = cu->getPic()->getPicYuvRec()->getCrAddr(cu->getAddr(), zorder);
            src                 = reconYuv->getCrAddr(tuIterator.m_partOffset);
            primitives.blockcpy_pp(compWidth, compHeight, dst, dststride, src, srcstride);
        }

        //----- set data -----
        ::memcpy(cu->getCbf(TEXT_CHROMA_U) + tuIterator.m_partOffset, m_qtTempCbf[1], tuIterator.m_absPartIdxStep * sizeof(uint8_t));
        ::memcpy(cu->getCbf(TEXT_CHROMA_V) + tuIterator.m_partOffset, m_qtTempCbf[2], tuIterator.m_absPartIdxStep * sizeof(uint8_t));
        ::memcpy(cu->getTransformSkip(TEXT_CHROMA_U) + tuIterator.m_partOffset, m_qtTempTransformSkipFlag[1], tuIterator.m_absPartIdxStep * sizeof(uint8_t));
        ::memcpy(cu->getTransformSkip(TEXT_CHROMA_V) + tuIterator.m_partOffset, m_qtTempTransformSkipFlag[2], tuIterator.m_absPartIdxStep * sizeof(uint8_t));
        cu->setChromIntraDirSubParts(bestMode, tuIterator.m_partOffset, depth + initTrDepth);
        cu->m_totalDistortion += bestDist;
    }
    while (isNextSection(&tuIterator));

    //----- restore context models -----
    if (initTrDepth != 0)
    {   // set Cbf for all blocks
        uint32_t combCbfU = 0;
        uint32_t combCbfV = 0;
        uint32_t partIdx  = 0;
        for (uint32_t part = 0; part < 4; part++, partIdx += tuIterator.m_absPartIdxStep)
        {
            combCbfU |= cu->getCbf(partIdx, TEXT_CHROMA_U, 1);
            combCbfV |= cu->getCbf(partIdx, TEXT_CHROMA_V, 1);
        }

        for (uint32_t offs = 0; offs < 4 * tuIterator.m_absPartIdxStep; offs++)
        {
            cu->getCbf(TEXT_CHROMA_U)[offs] |= combCbfU;
            cu->getCbf(TEXT_CHROMA_V)[offs] |= combCbfV;
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
void TEncSearch::xEncPCM(TComDataCU* cu, uint32_t absPartIdx, pixel* fenc, pixel* pcm, pixel* pred, int16_t* resi, pixel* recon, uint32_t stride, uint32_t width, uint32_t height, TextType eText)
{
    uint32_t x, y;
    uint32_t reconStride;
    pixel* pcmTmp = pcm;
    pixel* reconPic;
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
    uint32_t lumaOffset   = absPartIdx << cu->getPic()->getLog2UnitSize() * 2;
    uint32_t chromaOffset = lumaOffset >> (m_hChromaShift + m_vChromaShift);

    // Luminance
    pixel*   fenc = fencYuv->getLumaAddr();
    int16_t* resi = resiYuv->getLumaAddr();
    pixel*   pred = predYuv->getLumaAddr();
    pixel*   recon = reconYuv->getLumaAddr();
    pixel*   pcm  = cu->getPCMSampleY() + lumaOffset;

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
    cu->m_totalRDCost       = cost;
    cu->m_totalDistortion = distortion;

    cu->copyToPic(depth, 0, 0);
}

/** estimation of best merge coding
 * \param cu
 * \param puIdx
 * \param m
 * \returns void
 */
uint32_t TEncSearch::xMergeEstimation(TComDataCU* cu, int puIdx, MergeData& m)
{
    assert(cu->getPartitionSize(0) != SIZE_2Nx2N);

    if (cu->getCUSize(0) <= 8 && cu->getSlice()->getPPS()->getLog2ParallelMergeLevelMinus2())
    {
        if (puIdx == 0)
        {
            PartSize partSize = cu->getPartitionSize(0);
            cu->getPartitionSize()[0] = SIZE_2Nx2N;
            cu->getInterMergeCandidates(0, 0, m.mvFieldNeighbours, m.interDirNeighbours, m.maxNumMergeCand);
            cu->getPartitionSize()[0] = partSize;
        }
    }
    else
    {
        cu->getInterMergeCandidates(m.absPartIdx, puIdx, m.mvFieldNeighbours, m.interDirNeighbours, m.maxNumMergeCand);
    }

    /* convert bidir merge candidates into unidir
     * TODO: why did the HM do this?, why use MV pairs below? */
    if (cu->isBipredRestriction())
    {
        for (uint32_t mergeCand = 0; mergeCand < m.maxNumMergeCand; ++mergeCand)
        {
            if (m.interDirNeighbours[mergeCand] == 3)
            {
                m.interDirNeighbours[mergeCand] = 1;
                m.mvFieldNeighbours[mergeCand][1].refIdx = NOT_VALID;
            }
        }
    }

    uint32_t outCost = MAX_UINT;
    for (uint32_t mergeCand = 0; mergeCand < m.maxNumMergeCand; ++mergeCand)
    {
        /* Prevent TMVP candidates from using unavailable reference pixels */
        if (m_cfg->param->frameNumThreads > 1 &&
            (m.mvFieldNeighbours[mergeCand][0].mv.y >= (m_cfg->param->searchRange + 1) * 4 ||
             m.mvFieldNeighbours[mergeCand][1].mv.y >= (m_cfg->param->searchRange + 1) * 4))
        {
            continue;
        }

        cu->getCUMvField(REF_PIC_LIST_0)->m_mv[m.absPartIdx] = m.mvFieldNeighbours[mergeCand][0].mv;
        cu->getCUMvField(REF_PIC_LIST_0)->m_refIdx[m.absPartIdx] = m.mvFieldNeighbours[mergeCand][0].refIdx;
        cu->getCUMvField(REF_PIC_LIST_1)->m_mv[m.absPartIdx] = m.mvFieldNeighbours[mergeCand][1].mv;
        cu->getCUMvField(REF_PIC_LIST_1)->m_refIdx[m.absPartIdx] = m.mvFieldNeighbours[mergeCand][1].refIdx;

        motionCompensation(cu, &m_predTempYuv, REF_PIC_LIST_X, puIdx, true, false);
        uint32_t costCand = m_me.bufSATD(m_predTempYuv.getLumaAddr(m.absPartIdx), m_predTempYuv.getStride());
        uint32_t bitsCand = getTUBits(mergeCand, m.maxNumMergeCand);
        costCand = costCand + m_rdCost->getCost(bitsCand);
        if (costCand < outCost)
        {
            outCost = costCand;
            m.bits = bitsCand;
            m.index = mergeCand;
        }
    }

    m.mvField[0] = m.mvFieldNeighbours[m.index][0];
    m.mvField[1] = m.mvFieldNeighbours[m.index][1];
    m.interDir = m.interDirNeighbours[m.index];

    return outCost;
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
    AMVPInfo amvpInfo[2][MAX_NUM_REF];

    TComPicYuv *fenc    = cu->getSlice()->getPic()->getPicYuvOrg();
    PartSize partSize   = cu->getPartitionSize(0);
    int      numPart    = cu->getNumPartInter();
    int      numPredDir = cu->getSlice()->isInterP() ? 1 : 2;
    uint32_t lastMode = 0;
    int      totalmebits = 0;

    const int* numRefIdx = cu->getSlice()->getNumRefIdx();

    MergeData merge;

    memset(&merge, 0, sizeof(merge));

    for (int partIdx = 0; partIdx < numPart; partIdx++)
    {
        uint32_t partAddr;
        int      roiWidth, roiHeight;
        cu->getPartIndexAndSize(partIdx, partAddr, roiWidth, roiHeight);

        pixel* pu = fenc->getLumaAddr(cu->getAddr(), cu->getZorderIdxInCU() + partAddr);
        m_me.setSourcePU(pu - fenc->getLumaAddr(), roiWidth, roiHeight);

        uint32_t mrgCost = MAX_UINT;

        /* find best cost merge candidate */
        if (cu->getPartitionSize(partAddr) != SIZE_2Nx2N)
        {
            merge.absPartIdx = partAddr;
            merge.width = roiWidth;
            merge.height = roiHeight;
            mrgCost = xMergeEstimation(cu, partIdx, merge);

            if (bMergeOnly && cu->getCUSize(0) > 8)
            {
                if (mrgCost == MAX_UINT)
                {
                    /* No valid merge modes were found, there is no possible way to
                     * perform a valid motion compensation prediction, so early-exit */
                    return false;
                }
                // set merge result
                cu->setMergeFlag(partAddr, true);
                cu->setMergeIndex(partAddr, merge.index);
                cu->setInterDirSubParts(merge.interDir, partAddr, partIdx, cu->getDepth(partAddr));
                cu->getCUMvField(REF_PIC_LIST_0)->setAllMvField(merge.mvField[0], partSize, partAddr, 0, partIdx);
                cu->getCUMvField(REF_PIC_LIST_1)->setAllMvField(merge.mvField[1], partSize, partAddr, 0, partIdx);
                totalmebits += merge.bits;

                motionCompensation(cu, predYuv, REF_PIC_LIST_X, partIdx, true, bChroma);
                continue;
            }
        }

        MotionData list[2];
        MotionData bidir[2];
        uint32_t listSelBits[3]; // cost in bits of selecting a particular ref list
        uint32_t bidirCost = MAX_UINT;
        int bidirBits = 0;

        list[0].cost = MAX_UINT;
        list[1].cost = MAX_UINT;

        xGetBlkBits(partSize, cu->getSlice()->isInterP(), partIdx, lastMode, listSelBits);

        // Uni-directional prediction
        for (int l = 0; l < numPredDir; l++)
        {
            for (int ref = 0; ref < numRefIdx[l]; ref++)
            {
                uint32_t bits = listSelBits[l] + MVP_IDX_BITS;
                bits += getTUBits(ref, numRefIdx[l]);

                MV mvc[(MD_ABOVE_LEFT + 1) * 2 + 1];
                int numMvc = cu->fillMvpCand(partIdx, partAddr, l, ref, &amvpInfo[l][ref], mvc);

                // Pick the best possible MVP from AMVP candidates based on least residual
                uint32_t bestCost = MAX_INT;
                int mvpIdx = 0;

                for (int i = 0; i < amvpInfo[l][ref].m_num; i++)
                {
                    MV mvCand = amvpInfo[l][ref].m_mvCand[i];

                    // TODO: skip mvCand if Y is > merange and -FN>1
                    cu->clipMv(mvCand);

                    xPredInterLumaBlk(cu, cu->getSlice()->getRefPic(l, ref)->getPicYuvRec(), partAddr, &mvCand, roiWidth, roiHeight, &m_predTempYuv);
                    uint32_t cost = m_me.bufSAD(m_predTempYuv.getLumaAddr(partAddr), m_predTempYuv.getStride());
                    cost = m_rdCost->calcRdSADCost(cost, MVP_IDX_BITS);

                    if (bestCost > cost)
                    {
                        bestCost = cost;
                        mvpIdx  = i;
                    }
                }

                MV mvmin, mvmax, outmv, mvp = amvpInfo[l][ref].m_mvCand[mvpIdx];

                int merange = m_cfg->param->searchRange;
                xSetSearchRange(cu, mvp, merange, mvmin, mvmax);
                int satdCost = m_me.motionEstimate(m_mref[l][ref], mvmin, mvmax, mvp, numMvc, mvc, merange, outmv);

                /* Get total cost of partition, but only include MV bit cost once */
                bits += m_me.bitcost(outmv);
                uint32_t cost = (satdCost - m_me.mvcost(outmv)) + m_rdCost->getCost(bits);

                /* Refine MVP selection, updates: mvp, mvpIdx, bits, cost */
                xCheckBestMVP(&amvpInfo[l][ref], outmv, mvp, mvpIdx, bits, cost);

                if (cost < list[l].cost)
                {
                    list[l].mv = outmv;
                    list[l].mvp = mvp;
                    list[l].mvpIdx = mvpIdx;
                    list[l].ref = ref;
                    list[l].cost = cost;
                    list[l].bits = bits;
                }
            }
        }

        // Bi-directional prediction
        if (cu->getSlice()->isInterB() && !cu->isBipredRestriction() && list[0].cost != MAX_UINT && list[1].cost != MAX_UINT)
        {
            ALIGN_VAR_32(pixel, avg[MAX_CU_SIZE * MAX_CU_SIZE]);

            bidir[0] = list[0];
            bidir[1] = list[1];

            // Generate reference subpels
            TComPicYuv *refPic0 = cu->getSlice()->getRefPic(REF_PIC_LIST_0, list[0].ref)->getPicYuvRec();
            TComPicYuv *refPic1 = cu->getSlice()->getRefPic(REF_PIC_LIST_1, list[1].ref)->getPicYuvRec();
            xPredInterLumaBlk(cu, refPic0, partAddr, &list[0].mv, roiWidth, roiHeight, &m_predYuv[0]);
            xPredInterLumaBlk(cu, refPic1, partAddr, &list[1].mv, roiWidth, roiHeight, &m_predYuv[1]);

            pixel *pred0 = m_predYuv[0].getLumaAddr(partAddr);
            pixel *pred1 = m_predYuv[1].getLumaAddr(partAddr);

            int partEnum = partitionFromSizes(roiWidth, roiHeight);
            primitives.pixelavg_pp[partEnum](avg, roiWidth, pred0, m_predYuv[0].getStride(), pred1, m_predYuv[1].getStride(), 32);
            int satdCost = m_me.bufSATD(avg, roiWidth);

            bidirBits = list[0].bits + list[1].bits + listSelBits[2] - (listSelBits[0] + listSelBits[1]);
            bidirCost = satdCost + m_rdCost->getCost(bidirBits);

            MV mvzero(0, 0);
            bool bTryZero = list[0].mv.notZero() || list[1].mv.notZero();
            if (bTryZero)
            {
                /* Do not try zero MV if unidir motion predictors are beyond
                 * valid search area */
                MV mvmin, mvmax;
                int merange = X265_MAX(m_cfg->param->sourceWidth, m_cfg->param->sourceHeight);
                xSetSearchRange(cu, mvzero, merange, mvmin, mvmax);
                mvmax.y += 2; // there is some pad for subpel refine
                mvmin <<= 2;
                mvmax <<= 2;

                bTryZero &= list[0].mvp.checkRange(mvmin, mvmax);
                bTryZero &= list[1].mvp.checkRange(mvmin, mvmax);
            }
            if (bTryZero)
            {
                // coincident blocks of the two reference pictures
                pixel *ref0 = m_mref[0][list[0].ref]->fpelPlane + (pu - fenc->getLumaAddr());
                pixel *ref1 = m_mref[1][list[1].ref]->fpelPlane + (pu - fenc->getLumaAddr());
                intptr_t refStride = m_mref[0][0]->lumaStride;

                primitives.pixelavg_pp[partEnum](avg, roiWidth, ref0, refStride, ref1, refStride, 32);
                satdCost = m_me.bufSATD(avg, roiWidth);

                MV mvp0 = list[0].mvp;
                int mvpIdx0 = list[0].mvpIdx;
                m_me.setMVP(mvp0);
                uint32_t bits0 = list[0].bits - m_me.bitcost(list[0].mv) + m_me.bitcost(mvzero);

                MV mvp1 = list[1].mvp;
                int mvpIdx1 = list[1].mvpIdx;
                m_me.setMVP(mvp1);
                uint32_t bits1 = list[1].bits - m_me.bitcost(list[1].mv) + m_me.bitcost(mvzero);

                uint32_t cost = satdCost + m_rdCost->getCost(bits0) + m_rdCost->getCost(bits1);

                /* refine MVP selection for zero mv, updates: mvp, mvpidx, bits, cost */
                xCheckBestMVP(&amvpInfo[0][list[0].ref], mvzero, mvp0, mvpIdx0, bits0, cost);
                xCheckBestMVP(&amvpInfo[1][list[1].ref], mvzero, mvp1, mvpIdx1, bits1, cost);

                if (cost < bidirCost)
                {
                    bidir[0].mv = mvzero;
                    bidir[1].mv = mvzero;
                    bidir[0].mvp = mvp0;
                    bidir[1].mvp = mvp1;
                    bidir[0].mvpIdx = mvpIdx0;
                    bidir[1].mvpIdx = mvpIdx1;
                    bidirCost = cost;
                    bidirBits = bits0 + bits1 + listSelBits[2] - (listSelBits[0] + listSelBits[1]);
                }
            }
        }

        /* select best option and store into CU */
        cu->getCUMvField(REF_PIC_LIST_0)->setAllMvField(TComMvField(), partSize, partAddr, 0, partIdx);
        cu->getCUMvField(REF_PIC_LIST_1)->setAllMvField(TComMvField(), partSize, partAddr, 0, partIdx);

        if (mrgCost < bidirCost && mrgCost < list[0].cost && mrgCost < list[1].cost)
        {
            cu->setMergeFlag(partAddr, true);
            cu->setMergeIndex(partAddr, merge.index);
            cu->setInterDirSubParts(merge.interDir, partAddr, partIdx, cu->getDepth(partAddr));
            cu->getCUMvField(REF_PIC_LIST_0)->setAllMvField(merge.mvField[0], partSize, partAddr, 0, partIdx);
            cu->getCUMvField(REF_PIC_LIST_1)->setAllMvField(merge.mvField[1], partSize, partAddr, 0, partIdx);

            totalmebits += merge.bits;
        }
        else if (bidirCost < list[0].cost && bidirCost < list[1].cost)
        {
            lastMode = 2;

            cu->setMergeFlag(partAddr, false);
            cu->setInterDirSubParts(3, partAddr, partIdx, cu->getDepth(0));
            cu->getCUMvField(REF_PIC_LIST_0)->setAllMv(bidir[0].mv, partSize, partAddr, 0, partIdx);
            cu->getCUMvField(REF_PIC_LIST_0)->setAllRefIdx(list[0].ref, partSize, partAddr, 0, partIdx);
            cu->getCUMvField(REF_PIC_LIST_0)->setMvd(partAddr, bidir[0].mv - bidir[0].mvp);
            cu->setMVPIdx(REF_PIC_LIST_0, partAddr, bidir[0].mvpIdx);

            cu->getCUMvField(REF_PIC_LIST_1)->setAllMv(bidir[1].mv, partSize, partAddr, 0, partIdx);
            cu->getCUMvField(REF_PIC_LIST_1)->setAllRefIdx(list[1].ref, partSize, partAddr, 0, partIdx);
            cu->getCUMvField(REF_PIC_LIST_1)->setMvd(partAddr, bidir[1].mv - bidir[1].mvp);
            cu->setMVPIdx(REF_PIC_LIST_1, partAddr, bidir[1].mvpIdx);

            totalmebits += bidirBits;
        }
        else if (list[0].cost <= list[1].cost)
        {
            lastMode = 0;

            cu->setMergeFlag(partAddr, false);
            cu->setInterDirSubParts(1, partAddr, partIdx, cu->getDepth(0));
            cu->getCUMvField(REF_PIC_LIST_0)->setAllMv(list[0].mv, partSize, partAddr, 0, partIdx);
            cu->getCUMvField(REF_PIC_LIST_0)->setAllRefIdx(list[0].ref, partSize, partAddr, 0, partIdx);
            cu->getCUMvField(REF_PIC_LIST_0)->setMvd(partAddr, list[0].mv - list[0].mvp);
            cu->setMVPIdx(REF_PIC_LIST_0, partAddr, list[0].mvpIdx);

            totalmebits += list[0].bits;
        }
        else
        {
            lastMode = 1;

            cu->setMergeFlag(partAddr, false);
            cu->setInterDirSubParts(2, partAddr, partIdx, cu->getDepth(0));
            cu->getCUMvField(REF_PIC_LIST_1)->setAllMv(list[1].mv, partSize, partAddr, 0, partIdx);
            cu->getCUMvField(REF_PIC_LIST_1)->setAllRefIdx(list[1].ref, partSize, partAddr, 0, partIdx);
            cu->getCUMvField(REF_PIC_LIST_1)->setMvd(partAddr, list[1].mv - list[1].mvp);
            cu->setMVPIdx(REF_PIC_LIST_1, partAddr, list[1].mvpIdx);

            totalmebits += list[1].bits;
        }

        motionCompensation(cu, predYuv, REF_PIC_LIST_X, partIdx, true, bChroma);
    }

    x265_emms();
    cu->m_totalBits = totalmebits;
    return true;
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
        static const uint32_t listBits[2][3][3] =
        {
            { { 0, 0, 3 }, { 0, 0, 0 }, { 0, 0, 0 } },
            { { 5, 7, 7 }, { 7, 5, 7 }, { 9 - 3, 9 - 3, 9 - 3 } }
        };
        if (bPSlice)
        {
            blockBit[0] = 3;
            blockBit[1] = 0;
            blockBit[2] = 0;
        }
        else
        {
            ::memcpy(blockBit, listBits[partIdx][lastMode], 3 * sizeof(uint32_t));
        }
    }
    else if ((cuMode == SIZE_Nx2N || cuMode == SIZE_nLx2N) || cuMode == SIZE_nRx2N)
    {
        static const uint32_t listBits[2][3][3] =
        {
            { { 0, 2, 3 }, { 0, 0, 0 }, { 0, 0, 0 } },
            { { 5, 7, 7 }, { 7 - 2, 7 - 2, 9 - 2 }, { 9 - 3, 9 - 3, 9 - 3 } }
        };
        if (bPSlice)
        {
            blockBit[0] = 3;
            blockBit[1] = 0;
            blockBit[2] = 0;
        }
        else
        {
            ::memcpy(blockBit, listBits[partIdx][lastMode], 3 * sizeof(uint32_t));
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
        X265_CHECK(0, "xGetBlkBits: unknown cuMode\n");
    }
}

/* Check if using an alternative MVP would result in a smaller MVD + signal bits */
void TEncSearch::xCheckBestMVP(AMVPInfo* amvpInfo, MV mv, MV& mvPred, int& outMvpIdx, uint32_t& outBits, uint32_t& outCost)
{
    X265_CHECK(amvpInfo->m_mvCand[outMvpIdx] == mvPred, "xCheckBestMVP: unexpected mvPred\n");

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

    uint32_t bits = 0, bestBits = 0;
    uint32_t distortion = 0, bdist = 0;

    uint32_t cuSize = cu->getCUSize(0);

    // No residual coding : SKIP mode
    if (bSkipRes)
    {
        cu->setSkipFlagSubParts(true, 0, cu->getDepth(0));

        predYuv->copyToPartYuv(outReconYuv, 0);
        // Luma
        int part = partitionFromSize(cuSize);
        distortion = primitives.sse_pp[part](fencYuv->getLumaAddr(), fencYuv->getStride(), outReconYuv->getLumaAddr(), outReconYuv->getStride());
        // Chroma
        part = partitionFromSizes(cuSize >> m_hChromaShift, cuSize >> m_vChromaShift);
        distortion += m_rdCost->scaleChromaDistCb(primitives.sse_pp[part](fencYuv->getCbAddr(), fencYuv->getCStride(), outReconYuv->getCbAddr(), outReconYuv->getCStride()));
        distortion += m_rdCost->scaleChromaDistCr(primitives.sse_pp[part](fencYuv->getCrAddr(), fencYuv->getCStride(), outReconYuv->getCrAddr(), outReconYuv->getCStride()));

        m_rdGoOnSbacCoder->load(m_rdSbacCoders[cu->getDepth(0)][CI_CURR_BEST]);
        m_entropyCoder->resetBits();
        if (cu->getSlice()->getPPS()->getTransquantBypassEnableFlag())
        {
            m_entropyCoder->encodeCUTransquantBypassFlag(cu, 0);
        }
        m_entropyCoder->encodeSkipFlag(cu, 0);
        m_entropyCoder->encodeMergeIndex(cu, 0);

        bits = m_entropyCoder->getNumberOfWrittenBits();
        m_rdGoOnSbacCoder->store(m_rdSbacCoders[cu->getDepth(0)][CI_TEMP_BEST]);

        cu->m_totalBits       = bits;
        cu->m_totalDistortion = distortion;
        if (m_rdCost->psyRdEnabled())
        {
            int size = g_convertToBit[cuSize];
            cu->m_psyEnergy = m_rdCost->psyCost(size, fencYuv->getLumaAddr(), fencYuv->getStride(),
                                                   outReconYuv->getLumaAddr(), outReconYuv->getStride());
            cu->m_totalPsyCost = m_rdCost->calcPsyRdCost(cu->m_totalDistortion, cu->m_totalBits, cu->m_psyEnergy);
        }
        else
        {
            cu->m_totalRDCost = m_rdCost->calcRdCost(cu->m_totalDistortion, cu->m_totalBits);
        }

        m_rdGoOnSbacCoder->store(m_rdSbacCoders[cu->getDepth(0)][CI_TEMP_BEST]);

        cu->clearCbf(0, cu->getDepth(0));
        cu->setTrIdxSubParts(0, 0, cu->getDepth(0));
        return;
    }

    // Residual coding.
    uint64_t cost = 0, bcost = MAX_INT64;
    uint32_t zeroDistortion = 0;
    bits = 0;
    distortion = 0;

    outResiYuv->subtract(fencYuv, predYuv, cuSize);
    m_rdGoOnSbacCoder->load(m_rdSbacCoders[cu->getDepth(0)][CI_CURR_BEST]);
    xEstimateResidualQT(cu, 0, outResiYuv, cu->getDepth(0), cost, bits, distortion, &zeroDistortion, curUseRDOQ);

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
        ::memset(cu->getTransformIdx(), 0, qpartnum * sizeof(uint8_t));
        ::memset(cu->getCbf(TEXT_LUMA), 0, qpartnum * sizeof(uint8_t));
        ::memset(cu->getCbf(TEXT_CHROMA_U), 0, qpartnum * sizeof(uint8_t));
        ::memset(cu->getCbf(TEXT_CHROMA_V), 0, qpartnum * sizeof(uint8_t));
        ::memset(cu->getCoeffY(), 0, cuSize * cuSize * sizeof(coeff_t));
        ::memset(cu->getCoeffCb(), 0, cuSize * cuSize * sizeof(coeff_t) >> (m_hChromaShift + m_vChromaShift));
        ::memset(cu->getCoeffCr(), 0, cuSize * cuSize * sizeof(coeff_t) >> (m_hChromaShift + m_vChromaShift));
        cu->setTransformSkipSubParts(0, 0, 0, 0, cu->getDepth(0));
    }
    else
    {
        xSetResidualQTData(cu, 0, NULL, cu->getDepth(0), false);
    }

    m_rdGoOnSbacCoder->load(m_rdSbacCoders[cu->getDepth(0)][CI_CURR_BEST]);

    bits = xSymbolBitsInter(cu);

    cost = m_rdCost->calcRdCost(distortion, bits);

    if (cost < bcost)
    {
        if (cu->getQtRootCbf(0))
        {
            xSetResidualQTData(cu, 0, outBestResiYuv, cu->getDepth(0), true);
        }

        bestBits = bits;
        bcost    = cost;
        m_rdGoOnSbacCoder->store(m_rdSbacCoders[cu->getDepth(0)][CI_TEMP_BEST]);
    }

    X265_CHECK(bcost != MAX_INT64, "no best cost\n");

    if (cu->getQtRootCbf(0))
    {
        outReconYuv->addClip(predYuv, outBestResiYuv, cuSize);
    }
    else
    {
        predYuv->copyToPartYuv(outReconYuv, 0);
    }

    // update with clipped distortion and cost (qp estimation loop uses unclipped values)
    int part = partitionFromSize(cuSize);
    bdist = primitives.sse_pp[part](fencYuv->getLumaAddr(), fencYuv->getStride(), outReconYuv->getLumaAddr(), outReconYuv->getStride());
    part = partitionFromSizes(cuSize >> m_hChromaShift, cuSize >> m_vChromaShift);
    bdist += m_rdCost->scaleChromaDistCb(primitives.sse_pp[part](fencYuv->getCbAddr(), fencYuv->getCStride(), outReconYuv->getCbAddr(), outReconYuv->getCStride()));
    bdist += m_rdCost->scaleChromaDistCr(primitives.sse_pp[part](fencYuv->getCrAddr(), fencYuv->getCStride(), outReconYuv->getCrAddr(), outReconYuv->getCStride()));
    if (m_rdCost->psyRdEnabled())
    {
        int size = g_convertToBit[cuSize];
        cu->m_psyEnergy = m_rdCost->psyCost(size, fencYuv->getLumaAddr(), fencYuv->getStride(),
                                               outReconYuv->getLumaAddr(), outReconYuv->getStride());
        cu->m_totalPsyCost = m_rdCost->calcPsyRdCost(bdist, bestBits, cu->m_psyEnergy);
    }
    else
    {
        cu->m_totalRDCost = m_rdCost->calcRdCost(bdist, bestBits);
    }
    cu->m_totalBits       = bestBits;
    cu->m_totalDistortion = bdist;
    
    if (cu->isSkipped(0))
    {
        cu->clearCbf(0, cu->getDepth(0));
    }
}

void TEncSearch::generateCoeffRecon(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, ShortYuv* resiYuv, TComYuv* reconYuv, bool skipRes)
{
    if (skipRes && cu->getPredictionMode(0) == MODE_INTER && cu->getMergeFlag(0) && cu->getPartitionSize(0) == SIZE_2Nx2N)
    {
        predYuv->copyToPartYuv(reconYuv, 0);
        cu->clearCbf(0, cu->getDepth(0));
        return;
    }
    if (cu->getPredictionMode(0) == MODE_INTER)
    {
        residualTransformQuantInter(cu, 0, resiYuv, cu->getDepth(0), true);
        uint32_t width  = cu->getCUSize(0);
        if (cu->getQtRootCbf(0))
        {
            reconYuv->addClip(predYuv, resiYuv, width);
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

void TEncSearch::residualTransformQuantInter(TComDataCU* cu, uint32_t absPartIdx, ShortYuv* resiYuv, const uint32_t depth, bool curuseRDOQ)
{
    X265_CHECK(cu->getDepth(0) == cu->getDepth(absPartIdx), "invalid depth\n");
    const uint32_t trMode = depth - cu->getDepth(0);
    const uint32_t trSizeLog2 = cu->getSlice()->getSPS()->getLog2MaxCodingBlockSize() - depth;
    const uint32_t setCbf     = 1 << trMode;
    int chFmt                 = cu->getChromaFormat();

    bool bSplitFlag = ((cu->getSlice()->getSPS()->getQuadtreeTUMaxDepthInter() == 1) && cu->getPredictionMode(absPartIdx) == MODE_INTER && (cu->getPartitionSize(absPartIdx) != SIZE_2Nx2N));
    bool bCheckFull;
    if (bSplitFlag && depth == cu->getDepth(absPartIdx) && (trSizeLog2 > cu->getQuadtreeTULog2MinSizeInCU(absPartIdx)))
        bCheckFull = false;
    else
        bCheckFull = (trSizeLog2 <= cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize());
    const bool bCheckSplit = (trSizeLog2 > cu->getQuadtreeTULog2MinSizeInCU(absPartIdx));
    X265_CHECK(bCheckFull || bCheckSplit, "check-full or check-split must be set\n");

    // code full block
    uint32_t absSumY = 0, absSumU = 0, absSumV = 0;
    int lastPosY = -1, lastPosU = -1, lastPosV = -1;
    if (bCheckFull)
    {
        uint32_t trSizeCLog2 = trSizeLog2 - m_hChromaShift;
        bool bCodeChroma = true;
        uint32_t trModeC = trMode;
        if ((trSizeLog2 == 2) && !(chFmt == CHROMA_444))
        {
            trSizeCLog2++;
            trModeC--;
            uint32_t qpdiv = cu->getPic()->getNumPartInCU() >> ((depth - 1) << 1);
            bCodeChroma = ((absPartIdx & (qpdiv - 1)) == 0);
        }

        const bool splitIntoSubTUs = (chFmt == CHROMA_422);
        uint32_t absPartIdxStep = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) +  trModeC) << 1);

        uint32_t coeffOffsetY = absPartIdx << cu->getPic()->getLog2UnitSize() * 2;
        uint32_t coeffOffsetC = coeffOffsetY >> (m_hChromaShift + m_vChromaShift);
        coeff_t *coeffCurY = cu->getCoeffY() + coeffOffsetY;
        coeff_t *coeffCurU = cu->getCoeffCb() + coeffOffsetC;
        coeff_t *coeffCurV = cu->getCoeffCr() + coeffOffsetC;

        uint32_t trSize   = 1 << trSizeLog2;
        uint32_t trSizeC  = 1 << trSizeCLog2;
        uint32_t sizeIdx  = trSizeLog2  - 2;
        uint32_t sizeIdxC = trSizeCLog2 - 2;
        cu->setTrIdxSubParts(depth - cu->getDepth(0), absPartIdx, depth);

        cu->setTransformSkipSubParts(0, TEXT_LUMA, absPartIdx, depth);

        m_trQuant->setQPforQuant(cu->getQP(0), TEXT_LUMA, QP_BD_OFFSET, 0, chFmt);
        m_trQuant->selectLambda(TEXT_LUMA);

        absSumY = m_trQuant->transformNxN(cu, resiYuv->getLumaAddr(absPartIdx), resiYuv->m_width, coeffCurY,
                                          trSize, TEXT_LUMA, absPartIdx, &lastPosY, false, curuseRDOQ);

        cu->setCbfSubParts(absSumY ? setCbf : 0, TEXT_LUMA, absPartIdx, depth);

        if (absSumY)
        {
            int16_t *curResiY = resiYuv->getLumaAddr(absPartIdx);

            m_trQuant->setQPforQuant(cu->getQP(0), TEXT_LUMA, QP_BD_OFFSET, 0, chFmt);

            int scalingListType = 3 + TEXT_LUMA;
            X265_CHECK(scalingListType < 6, "scalingListType too large %d\n", scalingListType);
            m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdx), REG_DCT, curResiY, resiYuv->m_width,  coeffCurY, trSize, scalingListType, false, lastPosY); //this is for inter mode only
        }
        else
        {
            int16_t *ptr = resiYuv->getLumaAddr(absPartIdx);
            primitives.blockfill_s[sizeIdx](ptr, resiYuv->m_width, 0);
        }
        cu->setCbfSubParts(absSumY ? setCbf : 0, TEXT_LUMA, absPartIdx, depth);

        if (bCodeChroma)
        {
            TComTURecurse tuIterator;
            initSection(&tuIterator, splitIntoSubTUs ? VERTICAL_SPLIT : DONT_SPLIT, absPartIdxStep, absPartIdx);

            do
            {
                uint32_t absPartIdxC = tuIterator.m_absPartIdxTURelCU;
                uint32_t subTUBufferOffset = trSizeC * trSizeC * tuIterator.m_section;

                cu->setTransformSkipPartRange(0, TEXT_CHROMA_U, absPartIdxC, tuIterator.m_absPartIdxStep);
                cu->setTransformSkipPartRange(0, TEXT_CHROMA_V, absPartIdxC, tuIterator.m_absPartIdxStep);

                int curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCbQpOffset() + cu->getSlice()->getSliceQpDeltaCb();
                m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset, chFmt);

                m_trQuant->selectLambda(TEXT_CHROMA);

                absSumU = m_trQuant->transformNxN(cu, resiYuv->getCbAddr(absPartIdxC), resiYuv->m_cwidth, coeffCurU + subTUBufferOffset,
                                                  trSizeC, TEXT_CHROMA_U, absPartIdxC, &lastPosU, false, curuseRDOQ);

                curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCrQpOffset() + cu->getSlice()->getSliceQpDeltaCr();
                m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset, chFmt);
                absSumV = m_trQuant->transformNxN(cu, resiYuv->getCrAddr(absPartIdxC), resiYuv->m_cwidth, coeffCurV + subTUBufferOffset,
                                                  trSizeC, TEXT_CHROMA_V, absPartIdxC, &lastPosV, false, curuseRDOQ);

                cu->setCbfPartRange(absSumU ? setCbf : 0, TEXT_CHROMA_U, absPartIdxC, tuIterator.m_absPartIdxStep);
                cu->setCbfPartRange(absSumV ? setCbf : 0, TEXT_CHROMA_V, absPartIdxC, tuIterator.m_absPartIdxStep);

                if (absSumU)
                {
                    int16_t *pcResiCurrU = resiYuv->getCbAddr(absPartIdxC);

                    curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCbQpOffset() + cu->getSlice()->getSliceQpDeltaCb();
                    m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset, chFmt);

                    int scalingListType = 3 + TEXT_CHROMA_U;
                    X265_CHECK(scalingListType < 6, "scalingListType too large %d\n", scalingListType);
                    m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdxC), REG_DCT, pcResiCurrU, resiYuv->m_cwidth, coeffCurU + subTUBufferOffset, trSizeC, scalingListType, false, lastPosU);
                }
                else
                {
                    int16_t *ptr = resiYuv->getCbAddr(absPartIdxC);
                    primitives.blockfill_s[sizeIdxC](ptr, resiYuv->m_cwidth, 0);
                }
                if (absSumV)
                {
                    int16_t *curResiV = resiYuv->getCrAddr(absPartIdxC);
                    curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCrQpOffset() + cu->getSlice()->getSliceQpDeltaCr();
                    m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset, chFmt);

                    int scalingListType = 3 + TEXT_CHROMA_V;
                    X265_CHECK(scalingListType < 6, "scalingListType too large %d\n", scalingListType);
                    m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdxC), REG_DCT, curResiV, resiYuv->m_cwidth, coeffCurV + subTUBufferOffset, trSizeC, scalingListType, false, lastPosV);
                }
                else
                {
                    int16_t *ptr = resiYuv->getCrAddr(absPartIdxC);
                    primitives.blockfill_s[sizeIdxC](ptr, resiYuv->m_cwidth, 0);
                }
                cu->setCbfPartRange(absSumU ? setCbf : 0, TEXT_CHROMA_U, absPartIdxC, tuIterator.m_absPartIdxStep);
                cu->setCbfPartRange(absSumV ? setCbf : 0, TEXT_CHROMA_V, absPartIdxC, tuIterator.m_absPartIdxStep);
            }
            while (isNextSection(&tuIterator));

            if (splitIntoSubTUs)
            {
                offsetSubTUCBFs(cu, TEXT_CHROMA_U, trMode, absPartIdx);
                offsetSubTUCBFs(cu, TEXT_CHROMA_V, trMode, absPartIdx);
            }
        }
        return;
    }

    // code sub-blocks
    if (bCheckSplit && !bCheckFull)
    {
        const uint32_t qPartNumSubdiv = cu->getPic()->getNumPartInCU() >> ((depth + 1) << 1);
        for (uint32_t i = 0; i < 4; ++i)
        {
            residualTransformQuantInter(cu, absPartIdx + i * qPartNumSubdiv, resiYuv, depth + 1, curuseRDOQ);
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
}

void TEncSearch::xEstimateResidualQT(TComDataCU*    cu,
                                     uint32_t       absPartIdx,
                                     ShortYuv*      resiYuv,
                                     const uint32_t depth,
                                     uint64_t &     rdCost,
                                     uint32_t &     outBits,
                                     uint32_t &     outDist,
                                     uint32_t *     outZeroDist,
                                     bool           curuseRDOQ)
{
    X265_CHECK(cu->getDepth(0) == cu->getDepth(absPartIdx), "depth not matching\n");
    const uint32_t trMode = depth - cu->getDepth(0);
    const uint32_t trSizeLog2 = cu->getSlice()->getSPS()->getLog2MaxCodingBlockSize() - depth;
    const uint32_t subTUDepth = trMode + 1;
    const uint32_t setCbf     = 1 << trMode;
    int chFmt                 = cu->getChromaFormat();

    bool bSplitFlag = ((cu->getSlice()->getSPS()->getQuadtreeTUMaxDepthInter() == 1) && cu->getPredictionMode(absPartIdx) == MODE_INTER && (cu->getPartitionSize(absPartIdx) != SIZE_2Nx2N));
    bool bCheckFull;
    if (bSplitFlag && depth == cu->getDepth(absPartIdx) && (trSizeLog2 > cu->getQuadtreeTULog2MinSizeInCU(absPartIdx)))
        bCheckFull = false;
    else
        bCheckFull = (trSizeLog2 <= cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize());
    const bool bCheckSplit = (trSizeLog2 > cu->getQuadtreeTULog2MinSizeInCU(absPartIdx));
    X265_CHECK(bCheckFull || bCheckSplit, "check-full or check-split must be set\n");

    uint32_t trSizeCLog2 = trSizeLog2 - m_hChromaShift;
    bool bCodeChroma = true;
    uint32_t trModeC = trMode;
    if ((trSizeLog2 == 2) && !(chFmt == CHROMA_444))
    {
        trSizeCLog2++;
        trModeC--;
        uint32_t qpdiv = cu->getPic()->getNumPartInCU() >> ((depth - 1) << 1);
        bCodeChroma = ((absPartIdx & (qpdiv - 1)) == 0);
    }

    // code full block
    uint64_t singleCost = MAX_INT64;
    uint32_t singleBits = 0;
    uint32_t singleDist = 0;
    uint32_t singleBitsComp[MAX_NUM_COMPONENT][2 /*0 = top (or whole TU for non-4:2:2) sub-TU, 1 = bottom sub-TU*/] = { { 0, 0 }, { 0, 0 }, { 0, 0 } };
    uint32_t singleDistComp[MAX_NUM_COMPONENT][2 /*0 = top (or whole TU for non-4:2:2) sub-TU, 1 = bottom sub-TU*/] = { { 0, 0 }, { 0, 0 }, { 0, 0 } };
    uint32_t absSum[MAX_NUM_COMPONENT][2 /*0 = top (or whole TU for non-4:2:2) sub-TU, 1 = bottom sub-TU*/] = { { 0, 0 }, { 0, 0 }, { 0, 0 } };
    uint32_t bestTransformMode[MAX_NUM_COMPONENT][2 /*0 = top (or whole TU for non-4:2:2) sub-TU, 1 = bottom sub-TU*/] = { { 0, 0 }, { 0, 0 }, { 0, 0 } };
    int      lastPos[MAX_NUM_COMPONENT][2 /*0 = top (or whole TU for non-4:2:2) sub-TU, 1 = bottom sub-TU*/] = { { -1, -1 }, { -1, -1 }, { -1, -1 } };
    uint64_t minCost[MAX_NUM_COMPONENT][2 /*0 = top (or whole TU for non-4:2:2) sub-TU, 1 = bottom sub-TU*/];

    uint32_t bestCBF[MAX_NUM_COMPONENT];
    uint32_t bestsubTUCBF[MAX_NUM_COMPONENT][2];
    m_rdGoOnSbacCoder->store(m_rdSbacCoders[depth][CI_QT_TRAFO_ROOT]);

    uint32_t trSize = 1 << trSizeLog2;
    const bool splitIntoSubTUs = (chFmt == CHROMA_422);
    uint32_t absPartIdxStep = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) +  trModeC) << 1);

    // code full block
    if (bCheckFull)
    {
        uint32_t trSizeC = 1 << trSizeCLog2;
        const uint32_t qtlayer = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;
        uint32_t coeffOffsetY = absPartIdx << cu->getPic()->getLog2UnitSize() * 2;
        uint32_t coeffOffsetC = coeffOffsetY >> (m_hChromaShift + m_vChromaShift);
        coeff_t *coeffCurY = m_qtTempCoeffY[qtlayer] + coeffOffsetY;
        coeff_t *coeffCurU = m_qtTempCoeffCb[qtlayer] + coeffOffsetC;
        coeff_t *coeffCurV = m_qtTempCoeffCr[qtlayer] + coeffOffsetC;

        cu->setTrIdxSubParts(depth - cu->getDepth(0), absPartIdx, depth);
        bool checkTransformSkip   = cu->getSlice()->getPPS()->getUseTransformSkip() && !cu->getCUTransquantBypass(0);
        bool checkTransformSkipY  = checkTransformSkip && trSizeLog2  <= LOG2_MAX_TS_SIZE;
        bool checkTransformSkipUV = checkTransformSkip && trSizeCLog2 <= LOG2_MAX_TS_SIZE;

        cu->setTransformSkipSubParts(0, TEXT_LUMA, absPartIdx, depth);

        if (m_cfg->bEnableRDOQ && curuseRDOQ)
        {
            m_entropyCoder->estimateBit(m_trQuant->m_estBitsSbac, trSize, TEXT_LUMA);
        }

        m_trQuant->setQPforQuant(cu->getQP(0), TEXT_LUMA, QP_BD_OFFSET, 0, chFmt);
        m_trQuant->selectLambda(TEXT_LUMA);

        absSum[TEXT_LUMA][0] = m_trQuant->transformNxN(cu, resiYuv->getLumaAddr(absPartIdx), resiYuv->m_width, coeffCurY,
                                                       trSize, TEXT_LUMA, absPartIdx, &lastPos[TEXT_LUMA][0], false, curuseRDOQ);

        cu->setCbfSubParts(absSum[TEXT_LUMA][0] ? setCbf : 0, TEXT_LUMA, absPartIdx, depth);

        m_entropyCoder->resetBits();
        m_entropyCoder->encodeQtCbf(cu, absPartIdx, 0, trSize, trSize, TEXT_LUMA, trMode, true);
        if (absSum[TEXT_LUMA][0])
            m_entropyCoder->encodeCoeffNxN(cu, coeffCurY, absPartIdx,  trSize, TEXT_LUMA);
        singleBitsComp[TEXT_LUMA][0] = m_entropyCoder->getNumberOfWrittenBits();

        uint32_t singleBitsPrev = singleBitsComp[TEXT_LUMA][0];

        if (bCodeChroma)
        {
            TComTURecurse tuIterator;
            initSection(&tuIterator, splitIntoSubTUs ? VERTICAL_SPLIT : DONT_SPLIT, absPartIdxStep, absPartIdx);

            do
            {
                uint32_t absPartIdxC = tuIterator.m_absPartIdxTURelCU;
                uint32_t subTUBufferOffset    = trSizeC * trSizeC * tuIterator.m_section;

                cu->setTransformSkipPartRange(0, TEXT_CHROMA_U, absPartIdxC, tuIterator.m_absPartIdxStep);
                cu->setTransformSkipPartRange(0, TEXT_CHROMA_V, absPartIdxC, tuIterator.m_absPartIdxStep);

                if (m_cfg->bEnableRDOQ && curuseRDOQ)
                {
                    m_entropyCoder->estimateBit(m_trQuant->m_estBitsSbac, trSizeC, TEXT_CHROMA);
                }
                //Cb transform
                int curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCbQpOffset() + cu->getSlice()->getSliceQpDeltaCb();
                m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset, chFmt);

                m_trQuant->selectLambda(TEXT_CHROMA);

                absSum[TEXT_CHROMA_U][tuIterator.m_section] = m_trQuant->transformNxN(cu, resiYuv->getCbAddr(absPartIdxC), resiYuv->m_cwidth, coeffCurU + subTUBufferOffset,
                                                                                      trSizeC, TEXT_CHROMA_U, absPartIdxC, &lastPos[TEXT_CHROMA_U][tuIterator.m_section], false, curuseRDOQ);
                //Cr transform
                curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCrQpOffset() + cu->getSlice()->getSliceQpDeltaCr();
                m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset, chFmt);
                absSum[TEXT_CHROMA_V][tuIterator.m_section] = m_trQuant->transformNxN(cu, resiYuv->getCrAddr(absPartIdxC), resiYuv->m_cwidth, coeffCurV + subTUBufferOffset,
                                                                                      trSizeC, TEXT_CHROMA_V, absPartIdxC, &lastPos[TEXT_CHROMA_V][tuIterator.m_section], false, curuseRDOQ);

                cu->setCbfPartRange(absSum[TEXT_CHROMA_U][tuIterator.m_section] ? setCbf : 0, TEXT_CHROMA_U, absPartIdxC, tuIterator.m_absPartIdxStep);
                cu->setCbfPartRange(absSum[TEXT_CHROMA_V][tuIterator.m_section] ? setCbf : 0, TEXT_CHROMA_V, absPartIdxC, tuIterator.m_absPartIdxStep);

                m_entropyCoder->encodeQtCbf(cu, absPartIdxC, tuIterator.m_absPartIdxStep, trSizeC, trSizeC, TEXT_CHROMA_U, trMode, true);
                if (absSum[TEXT_CHROMA_U][tuIterator.m_section])
                    m_entropyCoder->encodeCoeffNxN(cu, coeffCurU + subTUBufferOffset, absPartIdxC, trSizeC, TEXT_CHROMA_U);
                singleBitsComp[TEXT_CHROMA_U][tuIterator.m_section] = m_entropyCoder->getNumberOfWrittenBits() - singleBitsPrev;

                m_entropyCoder->encodeQtCbf(cu, absPartIdxC, tuIterator.m_absPartIdxStep, trSizeC, trSizeC, TEXT_CHROMA_V, trMode, true);
                if (absSum[TEXT_CHROMA_V][tuIterator.m_section])
                    m_entropyCoder->encodeCoeffNxN(cu, coeffCurV + subTUBufferOffset, absPartIdxC, trSizeC, TEXT_CHROMA_V);
                uint32_t newBits = m_entropyCoder->getNumberOfWrittenBits();
                singleBitsComp[TEXT_CHROMA_V][tuIterator.m_section] = newBits - (singleBitsPrev + singleBitsComp[TEXT_CHROMA_U][tuIterator.m_section]);

                singleBitsPrev = newBits;
            }
            while (isNextSection(&tuIterator));
        }

        const uint32_t numSamplesLuma = 1 << (trSizeLog2 << 1);

        for (uint32_t subTUIndex = 0; subTUIndex < 2; subTUIndex++)
        {
            minCost[TEXT_LUMA][subTUIndex]     = MAX_INT64;
            minCost[TEXT_CHROMA_U][subTUIndex] = MAX_INT64;
            minCost[TEXT_CHROMA_V][subTUIndex] = MAX_INT64;
        }

        int partSize = partitionFromSize(trSize);
        uint32_t distY = primitives.sse_sp[partSize](resiYuv->getLumaAddr(absPartIdx), resiYuv->m_width, (pixel*)RDCost::zeroPel, trSize);

        if (outZeroDist)
        {
            *outZeroDist += distY;
        }
        if (absSum[TEXT_LUMA][0])
        {
            int16_t *curResiY = m_qtTempShortYuv[qtlayer].getLumaAddr(absPartIdx);

            m_trQuant->setQPforQuant(cu->getQP(0), TEXT_LUMA, QP_BD_OFFSET, 0, chFmt);

            int scalingListType = 3 + TEXT_LUMA;
            X265_CHECK(scalingListType < 6, "scalingListType too large %d\n", scalingListType);
            X265_CHECK(m_qtTempShortYuv[qtlayer].m_width == MAX_CU_SIZE, "width not full CU\n");
            m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdx), REG_DCT, curResiY, MAX_CU_SIZE,  coeffCurY, trSize, scalingListType, false, lastPos[TEXT_LUMA][0]); //this is for inter mode only

            const uint32_t nonZeroDistY = primitives.sse_ss[partSize](resiYuv->getLumaAddr(absPartIdx), resiYuv->m_width, m_qtTempShortYuv[qtlayer].getLumaAddr(absPartIdx), MAX_CU_SIZE);
            if (cu->isLosslessCoded(0))
            {
                distY = nonZeroDistY;
            }
            else
            {
                const uint64_t singleCostY = m_rdCost->calcRdCost(nonZeroDistY, singleBitsComp[TEXT_LUMA][0]);
                m_entropyCoder->resetBits();
                m_entropyCoder->encodeQtCbfZero(cu, TEXT_LUMA, trMode);
                const uint32_t nullBitsY = m_entropyCoder->getNumberOfWrittenBits();
                const uint64_t nullCostY = m_rdCost->calcRdCost(distY, nullBitsY);
                if (nullCostY < singleCostY)
                {
                    absSum[TEXT_LUMA][0] = 0;
                    ::memset(coeffCurY, 0, sizeof(coeff_t) * numSamplesLuma);
                    if (checkTransformSkipY)
                    {
                        minCost[TEXT_LUMA][0] = nullCostY;
                    }
                }
                else
                {
                    distY = nonZeroDistY;
                    if (checkTransformSkipY)
                    {
                        minCost[TEXT_LUMA][0] = singleCostY;
                    }
                }
            }
        }
        else if (checkTransformSkipY)
        {
            m_entropyCoder->resetBits();
            m_entropyCoder->encodeQtCbfZero(cu, TEXT_LUMA, trMode);
            const uint32_t nullBitsY = m_entropyCoder->getNumberOfWrittenBits();
            minCost[TEXT_LUMA][0] = m_rdCost->calcRdCost(distY, nullBitsY);
        }

        singleDistComp[TEXT_LUMA][0] = distY;

        if (!absSum[TEXT_LUMA][0])
        {
            int16_t *ptr =  m_qtTempShortYuv[qtlayer].getLumaAddr(absPartIdx);
            X265_CHECK(m_qtTempShortYuv[qtlayer].m_width == MAX_CU_SIZE, "width not full CU\n");
            int sizeIdx = trSizeLog2 - 2;
            primitives.blockfill_s[sizeIdx](ptr, MAX_CU_SIZE, 0);
        }
        cu->setCbfSubParts(absSum[TEXT_LUMA][0] ? setCbf : 0, TEXT_LUMA, absPartIdx, depth);

        uint32_t distU = 0;
        uint32_t distV = 0;
        if (bCodeChroma)
        {
            TComTURecurse tuIterator;
            initSection(&tuIterator, splitIntoSubTUs ? VERTICAL_SPLIT : DONT_SPLIT, absPartIdxStep, absPartIdx);

            int partSizeC = partitionFromSize(trSizeC);
            const uint32_t numSamplesChroma = trSizeC * trSizeC;

            do
            {
                uint32_t absPartIdxC = tuIterator.m_absPartIdxTURelCU;
                uint32_t subTUBufferOffset = trSizeC * trSizeC * tuIterator.m_section;

                distU = m_rdCost->scaleChromaDistCb(primitives.sse_sp[partSizeC](resiYuv->getCbAddr(absPartIdxC), resiYuv->m_cwidth, (pixel*)RDCost::zeroPel, trSizeC));

                if (outZeroDist)
                {
                    *outZeroDist += distU;
                }
                if (absSum[TEXT_CHROMA_U][tuIterator.m_section])
                {
                    int16_t *pcResiCurrU = m_qtTempShortYuv[qtlayer].getCbAddr(absPartIdxC);

                    int curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCbQpOffset() + cu->getSlice()->getSliceQpDeltaCb();
                    m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset, chFmt);

                    int scalingListType = 3 + TEXT_CHROMA_U;
                    X265_CHECK(scalingListType < 6, "scalingListType too large %d\n", scalingListType);
                    m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdxC), REG_DCT, pcResiCurrU, m_qtTempShortYuv[qtlayer].m_cwidth, coeffCurU + subTUBufferOffset,
                                               trSizeC, scalingListType, false, lastPos[TEXT_CHROMA_U][tuIterator.m_section]);
                    uint32_t dist = primitives.sse_ss[partSizeC](resiYuv->getCbAddr(absPartIdxC), resiYuv->m_cwidth,
                                                                 m_qtTempShortYuv[qtlayer].getCbAddr(absPartIdxC),
                                                                 m_qtTempShortYuv[qtlayer].m_cwidth);
                    const uint32_t nonZeroDistU = m_rdCost->scaleChromaDistCb(dist);

                    if (cu->isLosslessCoded(0))
                    {
                        distU = nonZeroDistU;
                    }
                    else
                    {
                        const uint64_t singleCostU = m_rdCost->calcRdCost(nonZeroDistU, singleBitsComp[TEXT_CHROMA_U][tuIterator.m_section]);
                        m_entropyCoder->resetBits();
                        m_entropyCoder->encodeQtCbfZero(cu, TEXT_CHROMA_U, trMode);
                        const uint32_t nullBitsU = m_entropyCoder->getNumberOfWrittenBits();
                        const uint64_t nullCostU = m_rdCost->calcRdCost(distU, nullBitsU);
                        if (nullCostU < singleCostU)
                        {
                            absSum[TEXT_CHROMA_U][tuIterator.m_section] = 0;
                            ::memset(coeffCurU + subTUBufferOffset, 0, sizeof(coeff_t) * numSamplesChroma);
                            if (checkTransformSkipUV)
                            {
                                minCost[TEXT_CHROMA_U][tuIterator.m_section] = nullCostU;
                            }
                        }
                        else
                        {
                            distU = nonZeroDistU;
                            if (checkTransformSkipUV)
                            {
                                minCost[TEXT_CHROMA_U][tuIterator.m_section] = singleCostU;
                            }
                        }
                    }
                }
                else if (checkTransformSkipUV)
                {
                    m_entropyCoder->resetBits();
                    m_entropyCoder->encodeQtCbfZero(cu, TEXT_CHROMA_U, trModeC);
                    const uint32_t nullBitsU = m_entropyCoder->getNumberOfWrittenBits();
                    minCost[TEXT_CHROMA_U][tuIterator.m_section] = m_rdCost->calcRdCost(distU, nullBitsU);
                }

                singleDistComp[TEXT_CHROMA_U][tuIterator.m_section] = distU;

                if (!absSum[TEXT_CHROMA_U][tuIterator.m_section])
                {
                    int16_t *ptr = m_qtTempShortYuv[qtlayer].getCbAddr(absPartIdxC);
                    const uint32_t stride = m_qtTempShortYuv[qtlayer].m_cwidth;
                    int sizeIdxC = trSizeCLog2 - 2;
                    primitives.blockfill_s[sizeIdxC](ptr, stride, 0);
                }

                distV = m_rdCost->scaleChromaDistCr(primitives.sse_sp[partSizeC](resiYuv->getCrAddr(absPartIdxC), resiYuv->m_cwidth, (pixel*)RDCost::zeroPel, trSizeC));
                if (outZeroDist)
                {
                    *outZeroDist += distV;
                }
                if (absSum[TEXT_CHROMA_V][tuIterator.m_section])
                {
                    int16_t *curResiV = m_qtTempShortYuv[qtlayer].getCrAddr(absPartIdxC);
                    int curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCrQpOffset() + cu->getSlice()->getSliceQpDeltaCr();
                    m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset, chFmt);

                    int scalingListType = 3 + TEXT_CHROMA_V;
                    X265_CHECK(scalingListType < 6, "scalingListType too large %d\n", scalingListType);
                    m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdxC), REG_DCT, curResiV, m_qtTempShortYuv[qtlayer].m_cwidth, coeffCurV + subTUBufferOffset,
                                               trSizeC, scalingListType, false, lastPos[TEXT_CHROMA_V][tuIterator.m_section]);
                    uint32_t dist = primitives.sse_ss[partSizeC](resiYuv->getCrAddr(absPartIdxC), resiYuv->m_cwidth,
                                                                 m_qtTempShortYuv[qtlayer].getCrAddr(absPartIdxC),
                                                                 m_qtTempShortYuv[qtlayer].m_cwidth);
                    const uint32_t nonZeroDistV = m_rdCost->scaleChromaDistCr(dist);

                    if (cu->isLosslessCoded(0))
                    {
                        distV = nonZeroDistV;
                    }
                    else
                    {
                        const uint64_t singleCostV = m_rdCost->calcRdCost(nonZeroDistV, singleBitsComp[TEXT_CHROMA_V][tuIterator.m_section]);
                        m_entropyCoder->resetBits();
                        m_entropyCoder->encodeQtCbfZero(cu, TEXT_CHROMA_V, trMode);
                        const uint32_t nullBitsV = m_entropyCoder->getNumberOfWrittenBits();
                        const uint64_t nullCostV = m_rdCost->calcRdCost(distV, nullBitsV);
                        if (nullCostV < singleCostV)
                        {
                            absSum[TEXT_CHROMA_V][tuIterator.m_section] = 0;
                            ::memset(coeffCurV + subTUBufferOffset, 0, sizeof(coeff_t) * numSamplesChroma);
                            if (checkTransformSkipUV)
                            {
                                minCost[TEXT_CHROMA_V][tuIterator.m_section] = nullCostV;
                            }
                        }
                        else
                        {
                            distV = nonZeroDistV;
                            if (checkTransformSkipUV)
                            {
                                minCost[TEXT_CHROMA_V][tuIterator.m_section] = singleCostV;
                            }
                        }
                    }
                }
                else if (checkTransformSkipUV)
                {
                    m_entropyCoder->resetBits();
                    m_entropyCoder->encodeQtCbfZero(cu, TEXT_CHROMA_V, trModeC);
                    const uint32_t nullBitsV = m_entropyCoder->getNumberOfWrittenBits();
                    minCost[TEXT_CHROMA_V][tuIterator.m_section] = m_rdCost->calcRdCost(distV, nullBitsV);
                }

                singleDistComp[TEXT_CHROMA_V][tuIterator.m_section] = distV;

                if (!absSum[TEXT_CHROMA_V][tuIterator.m_section])
                {
                    int16_t *ptr =  m_qtTempShortYuv[qtlayer].getCrAddr(absPartIdxC);
                    const uint32_t stride = m_qtTempShortYuv[qtlayer].m_cwidth;
                    int sizeIdxC = trSizeCLog2 - 2;
                    primitives.blockfill_s[sizeIdxC](ptr, stride, 0);
                }

                cu->setCbfPartRange(absSum[TEXT_CHROMA_U][tuIterator.m_section] ? setCbf : 0, TEXT_CHROMA_U, absPartIdxC, tuIterator.m_absPartIdxStep);
                cu->setCbfPartRange(absSum[TEXT_CHROMA_V][tuIterator.m_section] ? setCbf : 0, TEXT_CHROMA_V, absPartIdxC, tuIterator.m_absPartIdxStep);
            }
            while (isNextSection(&tuIterator));
        }

        int lastPosTransformSkip[MAX_NUM_COMPONENT][2 /*0 = top (or whole TU for non-4:2:2) sub-TU, 1 = bottom sub-TU*/] = { { -1, -1 }, { -1, -1 }, { -1, -1 } };
        if (checkTransformSkipY)
        {
            uint32_t nonZeroDistY = 0, absSumTransformSkipY;
            uint64_t singleCostY = MAX_INT64;

            int16_t *curResiY = m_qtTempShortYuv[qtlayer].getLumaAddr(absPartIdx);
            X265_CHECK(m_qtTempShortYuv[qtlayer].m_width == MAX_CU_SIZE, "width not full CU\n");

            coeff_t bestCoeffY[32 * 32];
            memcpy(bestCoeffY, coeffCurY, sizeof(coeff_t) * numSamplesLuma);

            int16_t bestResiY[32 * 32];
            for (int i = 0; i < trSize; ++i)
            {
                memcpy(bestResiY + i * trSize, curResiY + i * MAX_CU_SIZE, sizeof(int16_t) * trSize);
            }

            m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_QT_TRAFO_ROOT]);

            cu->setTransformSkipSubParts(1, TEXT_LUMA, absPartIdx, depth);

            if (m_cfg->bEnableRDOQTS)
            {
                m_entropyCoder->estimateBit(m_trQuant->m_estBitsSbac, trSize, TEXT_LUMA);
            }

            m_trQuant->setQPforQuant(cu->getQP(0), TEXT_LUMA, QP_BD_OFFSET, 0, chFmt);

            m_trQuant->selectLambda(TEXT_LUMA);
            absSumTransformSkipY = m_trQuant->transformNxN(cu, resiYuv->getLumaAddr(absPartIdx), resiYuv->m_width, coeffCurY,
                                                           trSize, TEXT_LUMA, absPartIdx, &lastPosTransformSkip[TEXT_LUMA][0], true, curuseRDOQ);
            cu->setCbfSubParts(absSumTransformSkipY ? setCbf : 0, TEXT_LUMA, absPartIdx, depth);

            if (absSumTransformSkipY)
            {
                m_entropyCoder->resetBits();
                m_entropyCoder->encodeQtCbf(cu, absPartIdx, 0, trSize, trSize, TEXT_LUMA, trMode, true);
                m_entropyCoder->encodeCoeffNxN(cu, coeffCurY, absPartIdx, trSize, TEXT_LUMA);
                const uint32_t skipSingleBitsY = m_entropyCoder->getNumberOfWrittenBits();

                m_trQuant->setQPforQuant(cu->getQP(0), TEXT_LUMA, QP_BD_OFFSET, 0, chFmt);

                int scalingListType = 3 + TEXT_LUMA;
                X265_CHECK(scalingListType < 6, "scalingListType too large %d\n", scalingListType);
                X265_CHECK(m_qtTempShortYuv[qtlayer].m_width == MAX_CU_SIZE, "width not full CU\n");

                m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdx), REG_DCT, curResiY, MAX_CU_SIZE,  coeffCurY, trSize, scalingListType, true, lastPosTransformSkip[TEXT_LUMA][0]);

                nonZeroDistY = primitives.sse_ss[partSize](resiYuv->getLumaAddr(absPartIdx), resiYuv->m_width,
                                                           m_qtTempShortYuv[qtlayer].getLumaAddr(absPartIdx),
                                                           MAX_CU_SIZE);

                singleCostY = m_rdCost->calcRdCost(nonZeroDistY, skipSingleBitsY);
            }

            if (!absSumTransformSkipY || minCost[TEXT_LUMA][0] < singleCostY)
            {
                cu->setTransformSkipSubParts(0, TEXT_LUMA, absPartIdx, depth);
                memcpy(coeffCurY, bestCoeffY, sizeof(coeff_t) * numSamplesLuma);
                for (int i = 0; i < trSize; ++i)
                {
                    memcpy(curResiY + i * MAX_CU_SIZE, &bestResiY[i * trSize], sizeof(int16_t) * trSize);
                }
            }
            else
            {
                singleDistComp[TEXT_LUMA][0] = nonZeroDistY;
                absSum[TEXT_LUMA][0] = absSumTransformSkipY;
                bestTransformMode[TEXT_LUMA][0] = 1;
            }

            cu->setCbfSubParts(absSum[TEXT_LUMA][0] ? setCbf : 0, TEXT_LUMA, absPartIdx, depth);
        }

        if (bCodeChroma && checkTransformSkipUV)
        {
            uint32_t nonZeroDistU = 0, nonZeroDistV = 0, absSumTransformSkipU, absSumTransformSkipV;
            uint64_t singleCostU = MAX_INT64;
            uint64_t singleCostV = MAX_INT64;

            m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_QT_TRAFO_ROOT]);

            TComTURecurse tuIterator;
            initSection(&tuIterator, splitIntoSubTUs ? VERTICAL_SPLIT : DONT_SPLIT, absPartIdxStep, absPartIdx);

            int partSizeC = partitionFromSize(trSizeC);
            const uint32_t numSamplesChroma = trSizeC * trSizeC;

            do
            {
                uint32_t absPartIdxC = tuIterator.m_absPartIdxTURelCU;
                uint32_t subTUBufferOffset = trSizeC * trSizeC * tuIterator.m_section;

                int16_t *curResiU = m_qtTempShortYuv[qtlayer].getCbAddr(absPartIdxC);
                int16_t *curResiV = m_qtTempShortYuv[qtlayer].getCrAddr(absPartIdxC);
                uint32_t stride = m_qtTempShortYuv[qtlayer].m_cwidth;

                coeff_t bestCoeffU[32 * 32], bestCoeffV[32 * 32];
                memcpy(bestCoeffU, coeffCurU + subTUBufferOffset, sizeof(coeff_t) * numSamplesChroma);
                memcpy(bestCoeffV, coeffCurV + subTUBufferOffset, sizeof(coeff_t) * numSamplesChroma);

                int16_t bestResiU[32 * 32], bestResiV[32 * 32];
                for (int i = 0; i < trSizeC; ++i)
                {
                    memcpy(&bestResiU[i * trSizeC], curResiU + i * stride, sizeof(int16_t) * trSizeC);
                    memcpy(&bestResiV[i * trSizeC], curResiV + i * stride, sizeof(int16_t) * trSizeC);
                }

                cu->setTransformSkipPartRange(1, TEXT_CHROMA_U, absPartIdxC, tuIterator.m_absPartIdxStep);
                cu->setTransformSkipPartRange(1, TEXT_CHROMA_V, absPartIdxC, tuIterator.m_absPartIdxStep);

                if (m_cfg->bEnableRDOQTS)
                {
                    m_entropyCoder->estimateBit(m_trQuant->m_estBitsSbac, trSizeC, TEXT_CHROMA);
                }

                int curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCbQpOffset() + cu->getSlice()->getSliceQpDeltaCb();
                m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset, chFmt);
                m_trQuant->selectLambda(TEXT_CHROMA);

                absSumTransformSkipU = m_trQuant->transformNxN(cu, resiYuv->getCbAddr(absPartIdxC), resiYuv->m_cwidth, coeffCurU + subTUBufferOffset,
                                                               trSizeC, TEXT_CHROMA_U, absPartIdxC, &lastPosTransformSkip[TEXT_CHROMA_U][tuIterator.m_section], true, curuseRDOQ);
                curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCrQpOffset() + cu->getSlice()->getSliceQpDeltaCr();
                m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset, chFmt);
                absSumTransformSkipV = m_trQuant->transformNxN(cu, resiYuv->getCrAddr(absPartIdxC), resiYuv->m_cwidth, coeffCurV + subTUBufferOffset,
                                                               trSizeC, TEXT_CHROMA_V, absPartIdxC, &lastPosTransformSkip[TEXT_CHROMA_V][tuIterator.m_section], true, curuseRDOQ);

                cu->setCbfPartRange(absSumTransformSkipU ? setCbf : 0, TEXT_CHROMA_U, absPartIdxC, tuIterator.m_absPartIdxStep);
                cu->setCbfPartRange(absSumTransformSkipV ? setCbf : 0, TEXT_CHROMA_V, absPartIdxC, tuIterator.m_absPartIdxStep);

                m_entropyCoder->resetBits();
                singleBitsComp[TEXT_CHROMA_U][tuIterator.m_section] = 0;

                if (absSumTransformSkipU)
                {
                    m_entropyCoder->encodeQtCbf(cu, absPartIdxC, tuIterator.m_absPartIdxStep, trSizeC, trSizeC, TEXT_CHROMA_U, trMode, true);
                    m_entropyCoder->encodeCoeffNxN(cu, coeffCurU + subTUBufferOffset, absPartIdxC, trSizeC, TEXT_CHROMA_U);
                    singleBitsComp[TEXT_CHROMA_U][tuIterator.m_section] = m_entropyCoder->getNumberOfWrittenBits();

                    curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCbQpOffset() + cu->getSlice()->getSliceQpDeltaCb();
                    m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset, chFmt);

                    int scalingListType = 3 + TEXT_CHROMA_U;
                    X265_CHECK(scalingListType < 6, "scalingListType too large %d\n", scalingListType);
                    m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdxC), REG_DCT, curResiU, m_qtTempShortYuv[qtlayer].m_cwidth, coeffCurU + subTUBufferOffset,
                                               trSizeC, scalingListType, true, lastPosTransformSkip[TEXT_CHROMA_U][tuIterator.m_section]);
                    uint32_t dist = primitives.sse_ss[partSizeC](resiYuv->getCbAddr(absPartIdxC), resiYuv->m_cwidth,
                                                                 m_qtTempShortYuv[qtlayer].getCbAddr(absPartIdxC),
                                                                 m_qtTempShortYuv[qtlayer].m_cwidth);
                    nonZeroDistU = m_rdCost->scaleChromaDistCb(dist);
                    singleCostU = m_rdCost->calcRdCost(nonZeroDistU, singleBitsComp[TEXT_CHROMA_U][tuIterator.m_section]);
                }

                if (!absSumTransformSkipU || minCost[TEXT_CHROMA_U][tuIterator.m_section] < singleCostU)
                {
                    cu->setTransformSkipPartRange(0, TEXT_CHROMA_U, absPartIdxC, tuIterator.m_absPartIdxStep);

                    memcpy(coeffCurU + subTUBufferOffset, bestCoeffU, sizeof(coeff_t) * numSamplesChroma);
                    for (int i = 0; i < trSizeC; ++i)
                    {
                        memcpy(curResiU + i * stride, &bestResiU[i * trSizeC], sizeof(int16_t) * trSizeC);
                    }
                }
                else
                {
                    singleDistComp[TEXT_CHROMA_U][tuIterator.m_section] = nonZeroDistU;
                    absSum[TEXT_CHROMA_U][tuIterator.m_section] = absSumTransformSkipU;
                    bestTransformMode[TEXT_CHROMA_U][tuIterator.m_section] = 1;
                }

                if (absSumTransformSkipV)
                {
                    m_entropyCoder->encodeQtCbf(cu, absPartIdxC, tuIterator.m_absPartIdxStep, trSizeC, trSizeC, TEXT_CHROMA_V, trMode, true);
                    m_entropyCoder->encodeCoeffNxN(cu, coeffCurV + subTUBufferOffset, absPartIdxC, trSizeC, TEXT_CHROMA_V);
                    singleBitsComp[TEXT_CHROMA_V][tuIterator.m_section] = m_entropyCoder->getNumberOfWrittenBits() - singleBitsComp[TEXT_CHROMA_U][tuIterator.m_section];

                    curChromaQpOffset = cu->getSlice()->getPPS()->getChromaCrQpOffset() + cu->getSlice()->getSliceQpDeltaCr();
                    m_trQuant->setQPforQuant(cu->getQP(0), TEXT_CHROMA, cu->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset, chFmt);

                    int scalingListType = 3 + TEXT_CHROMA_V;
                    X265_CHECK(scalingListType < 6, "scalingListType too large %d\n", scalingListType);
                    m_trQuant->invtransformNxN(cu->getCUTransquantBypass(absPartIdxC), REG_DCT, curResiV, m_qtTempShortYuv[qtlayer].m_cwidth, coeffCurV + subTUBufferOffset,
                                               trSizeC, scalingListType, true, lastPosTransformSkip[TEXT_CHROMA_V][tuIterator.m_section]);
                    uint32_t dist = primitives.sse_ss[partSizeC](resiYuv->getCrAddr(absPartIdxC), resiYuv->m_cwidth,
                                                                 m_qtTempShortYuv[qtlayer].getCrAddr(absPartIdxC),
                                                                 m_qtTempShortYuv[qtlayer].m_cwidth);
                    nonZeroDistV = m_rdCost->scaleChromaDistCr(dist);
                    singleCostV = m_rdCost->calcRdCost(nonZeroDistV, singleBitsComp[TEXT_CHROMA_V][tuIterator.m_section]);
                }

                if (!absSumTransformSkipV || minCost[TEXT_CHROMA_V][tuIterator.m_section] < singleCostV)
                {
                    cu->setTransformSkipPartRange(0, TEXT_CHROMA_V, absPartIdxC, tuIterator.m_absPartIdxStep);

                    memcpy(coeffCurV + subTUBufferOffset, bestCoeffV, sizeof(coeff_t) * numSamplesChroma);
                    for (int i = 0; i < trSizeC; ++i)
                    {
                        memcpy(curResiV + i * stride, &bestResiV[i * trSizeC], sizeof(int16_t) * trSizeC);
                    }
                }
                else
                {
                    singleDistComp[TEXT_CHROMA_V][tuIterator.m_section] = nonZeroDistV;
                    absSum[TEXT_CHROMA_V][tuIterator.m_section] = absSumTransformSkipV;
                    bestTransformMode[TEXT_CHROMA_V][tuIterator.m_section] = 1;
                }

                cu->setCbfPartRange(absSum[TEXT_CHROMA_U][tuIterator.m_section] ? setCbf : 0, TEXT_CHROMA_U, absPartIdxC, tuIterator.m_absPartIdxStep);
                cu->setCbfPartRange(absSum[TEXT_CHROMA_V][tuIterator.m_section] ? setCbf : 0, TEXT_CHROMA_V, absPartIdxC, tuIterator.m_absPartIdxStep);
            }
            while (isNextSection(&tuIterator));
        }

        m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_QT_TRAFO_ROOT]);

        m_entropyCoder->resetBits();

        if (trSizeLog2 > cu->getQuadtreeTULog2MinSizeInCU(absPartIdx))
        {
            m_entropyCoder->encodeTransformSubdivFlag(0, 5 - trSizeLog2);
        }

        if (bCodeChroma)
        {
            if (splitIntoSubTUs)
            {
                offsetSubTUCBFs(cu, TEXT_CHROMA_U, trMode, absPartIdx);
                offsetSubTUCBFs(cu, TEXT_CHROMA_V, trMode, absPartIdx);
            }

            uint32_t trHeightC = (chFmt == CHROMA_422) ? (trSizeC << 1) : trSizeC;
            m_entropyCoder->encodeQtCbf(cu, absPartIdx, absPartIdxStep, trSizeC, trHeightC, TEXT_CHROMA_U, trMode, true);
            m_entropyCoder->encodeQtCbf(cu, absPartIdx, absPartIdxStep, trSizeC, trHeightC, TEXT_CHROMA_V, trMode, true);
        }

        m_entropyCoder->encodeQtCbf(cu, absPartIdx, 0, trSize, trSize, TEXT_LUMA,     trMode, true);
        if (absSum[TEXT_LUMA][0])
            m_entropyCoder->encodeCoeffNxN(cu, coeffCurY, absPartIdx, trSize, TEXT_LUMA);

        if (bCodeChroma)
        {
            if (!splitIntoSubTUs)
            {
                if (absSum[TEXT_CHROMA_U][0])
                    m_entropyCoder->encodeCoeffNxN(cu, coeffCurU, absPartIdx, trSizeC, TEXT_CHROMA_U);
                if (absSum[TEXT_CHROMA_V][0])
                    m_entropyCoder->encodeCoeffNxN(cu, coeffCurV, absPartIdx, trSizeC, TEXT_CHROMA_V);
            }
            else
            {
                uint32_t subTUSize = trSizeC * trSizeC;
                uint32_t partIdxesPerSubTU = absPartIdxStep >> 1;

                if (absSum[TEXT_CHROMA_U][0])
                    m_entropyCoder->encodeCoeffNxN(cu, coeffCurU, absPartIdx, trSizeC, TEXT_CHROMA_U);
                if (absSum[TEXT_CHROMA_U][1])
                    m_entropyCoder->encodeCoeffNxN(cu, coeffCurU + subTUSize, absPartIdx + partIdxesPerSubTU, trSizeC, TEXT_CHROMA_U);
                if (absSum[TEXT_CHROMA_V][0])
                    m_entropyCoder->encodeCoeffNxN(cu, coeffCurV, absPartIdx, trSizeC, TEXT_CHROMA_V);
                if (absSum[TEXT_CHROMA_V][1])
                    m_entropyCoder->encodeCoeffNxN(cu, coeffCurV + subTUSize, absPartIdx + partIdxesPerSubTU, trSizeC, TEXT_CHROMA_V);
            }
        }

        singleDist += singleDistComp[TEXT_LUMA][0];
        for (uint32_t subTUIndex = 0; subTUIndex < 2; subTUIndex++)
        {
            singleDist += singleDistComp[TEXT_CHROMA_U][subTUIndex];
            singleDist += singleDistComp[TEXT_CHROMA_V][subTUIndex];
        }

        singleBits = m_entropyCoder->getNumberOfWrittenBits();
        singleCost = m_rdCost->calcRdCost(singleDist, singleBits);

        bestCBF[TEXT_LUMA] = cu->getCbf(absPartIdx, TEXT_LUMA, trMode);
        if (bCodeChroma)
        {
            for (uint32_t chromId = TEXT_CHROMA_U; chromId < MAX_NUM_COMPONENT; chromId++)
            {
                bestCBF[chromId] = cu->getCbf(absPartIdx, (TextType)chromId, trMode);
                if (splitIntoSubTUs)
                {
                    uint32_t partIdxesPerSubTU = absPartIdxStep >> 1;
                    for (uint32_t subTU = 0; subTU < 2; subTU++)
                    {
                        bestsubTUCBF[chromId][subTU] = cu->getCbf((absPartIdx + (subTU * partIdxesPerSubTU)), (TextType)chromId, subTUDepth);
                    }
                }
            }
        }
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

        bestCBF[TEXT_LUMA] = cu->getCbf(absPartIdx, TEXT_LUMA, trMode);
        if (bCodeChroma)
        {
            for (uint32_t chromId = TEXT_CHROMA_U; chromId < MAX_NUM_COMPONENT; chromId++)
            {
                bestCBF[chromId] = cu->getCbf(absPartIdx, (TextType)chromId, trMode);
                if (splitIntoSubTUs)
                {
                    uint32_t partIdxesPerSubTU     = absPartIdxStep >> 1;
                    for (uint32_t subTU = 0; subTU < 2; subTU++)
                    {
                        bestsubTUCBF[chromId][subTU] = cu->getCbf((absPartIdx + (subTU * partIdxesPerSubTU)), (TextType)chromId, subTUDepth);
                    }
                }
            }
        }

        const uint32_t qPartNumSubdiv = cu->getPic()->getNumPartInCU() >> ((depth + 1) << 1);
        for (uint32_t i = 0; i < 4; ++i)
        {
            xEstimateResidualQT(cu, absPartIdx + i * qPartNumSubdiv, resiYuv, depth + 1, subDivCost, subdivBits, subdivDist, bCheckFull ? NULL : outZeroDist);
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
            cu->getCbf(TEXT_LUMA)[absPartIdx + i]     |= ycbf << trMode;
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

        cu->setTransformSkipSubParts(bestTransformMode[TEXT_LUMA][0], TEXT_LUMA, absPartIdx, depth);
        if (bCodeChroma)
        {
            const uint32_t numberOfSections  = splitIntoSubTUs ? 2 : 1;

            uint32_t partIdxesPerSubTU  = absPartIdxStep >> (splitIntoSubTUs ? 1 : 0);
            for (uint32_t subTUIndex = 0; subTUIndex < numberOfSections; subTUIndex++)
            {
                const uint32_t  subTUPartIdx = absPartIdx + (subTUIndex * partIdxesPerSubTU);

                cu->setTransformSkipPartRange(bestTransformMode[TEXT_CHROMA_U][subTUIndex], TEXT_CHROMA_U, subTUPartIdx, partIdxesPerSubTU);
                cu->setTransformSkipPartRange(bestTransformMode[TEXT_CHROMA_V][subTUIndex], TEXT_CHROMA_V, subTUPartIdx, partIdxesPerSubTU);
            }
        }
        X265_CHECK(bCheckFull, "check-full must be set\n");
        m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_QT_TRAFO_TEST]);
    }

    rdCost += singleCost;
    outBits += singleBits;
    outDist += singleDist;

    cu->setTrIdxSubParts(trMode, absPartIdx, depth);
    cu->setCbfSubParts(absSum[TEXT_LUMA][0] ? setCbf : 0, TEXT_LUMA, absPartIdx, depth);

    if (bCodeChroma)
    {
        const uint32_t numberOfSections  = splitIntoSubTUs ? 2 : 1;
        uint32_t partIdxesPerSubTU  = absPartIdxStep >> (splitIntoSubTUs ? 1 : 0);

        for (uint32_t chromId = TEXT_CHROMA_U; chromId < MAX_NUM_COMPONENT; chromId++)
        {
            for (uint32_t subTUIndex = 0; subTUIndex < numberOfSections; subTUIndex++)
            {
                const uint32_t  subTUPartIdx = absPartIdx + (subTUIndex * partIdxesPerSubTU);

                if (splitIntoSubTUs)
                {
                    const uint8_t combinedCBF = (bestsubTUCBF[chromId][subTUIndex] << subTUDepth) | (bestCBF[chromId] << trMode);
                    cu->setCbfPartRange(combinedCBF, (TextType)chromId, subTUPartIdx, partIdxesPerSubTU);
                }
                else
                {
                    cu->setCbfPartRange((bestCBF[chromId] << trMode), (TextType)chromId, subTUPartIdx, partIdxesPerSubTU);
                }
            }
        }
    }
}

void TEncSearch::xEncodeResidualQT(TComDataCU* cu, uint32_t absPartIdx, const uint32_t depth, bool bSubdivAndCbf, TextType ttype)
{
    X265_CHECK(cu->getDepth(0) == cu->getDepth(absPartIdx), "depth not matching\n");
    const uint32_t curTrMode   = depth - cu->getDepth(0);
    const uint32_t trMode      = cu->getTransformIdx(absPartIdx);
    const bool     bSubdiv     = curTrMode != trMode;
    const uint32_t trSizeLog2  = cu->getSlice()->getSPS()->getLog2MaxCodingBlockSize() - depth;
    uint32_t       trSizeCLog2 = trSizeLog2 - m_hChromaShift;
    int            chFmt       = cu->getChromaFormat();
    const bool splitIntoSubTUs = (chFmt == CHROMA_422);

    if (bSubdivAndCbf && trSizeLog2 <= cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() && trSizeLog2 > cu->getQuadtreeTULog2MinSizeInCU(absPartIdx))
    {
        m_entropyCoder->encodeTransformSubdivFlag(bSubdiv, 5 - trSizeLog2);
    }

    X265_CHECK(cu->getPredictionMode(absPartIdx) != MODE_INTRA, "xEncodeResidualQT() with intra block\n");

    bool mCodeAll = true;
    uint32_t trSize    = 1 << trSizeLog2;
    uint32_t trWidthC  = 1 << trSizeCLog2;
    uint32_t trHeightC = splitIntoSubTUs ? (trWidthC << 1) : trWidthC;

    const uint32_t numPels = trWidthC * trHeightC;
    if (numPels < (MIN_TU_SIZE * MIN_TU_SIZE))
    {
        mCodeAll = false;
    }

    if (bSubdivAndCbf)
    {
        const bool bFirstCbfOfCU = curTrMode == 0;
        if (bFirstCbfOfCU || mCodeAll)
        {
            uint32_t absPartIdxStep = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) +  curTrMode) << 1);
            if (bFirstCbfOfCU || cu->getCbf(absPartIdx, TEXT_CHROMA_U, curTrMode - 1))
            {
                m_entropyCoder->encodeQtCbf(cu, absPartIdx, absPartIdxStep, trWidthC, trHeightC, TEXT_CHROMA_U, curTrMode, !bSubdiv);
            }
            if (bFirstCbfOfCU || cu->getCbf(absPartIdx, TEXT_CHROMA_V, curTrMode - 1))
            {
                m_entropyCoder->encodeQtCbf(cu, absPartIdx, absPartIdxStep, trWidthC, trHeightC, TEXT_CHROMA_V, curTrMode, !bSubdiv);
            }
        }
        else
        {
            X265_CHECK(cu->getCbf(absPartIdx, TEXT_CHROMA_U, curTrMode) == cu->getCbf(absPartIdx, TEXT_CHROMA_U, curTrMode - 1), "chroma CBF not matching\n");
            X265_CHECK(cu->getCbf(absPartIdx, TEXT_CHROMA_V, curTrMode) == cu->getCbf(absPartIdx, TEXT_CHROMA_V, curTrMode - 1), "chroma CBF not matching\n");
        }
    }

    if (!bSubdiv)
    {
        //Luma
        const uint32_t qtlayer = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;
        uint32_t coeffOffsetY = absPartIdx << cu->getPic()->getLog2UnitSize() * 2;
        coeff_t *coeffCurY = m_qtTempCoeffY[qtlayer] + coeffOffsetY;

        //Chroma
        bool bCodeChroma = true;
        uint32_t trModeC = trMode;
        if ((trSizeLog2 == 2) && !(chFmt == CHROMA_444))
        {
            trSizeCLog2++;
            trModeC--;
            uint32_t qpdiv = cu->getPic()->getNumPartInCU() >> ((depth - 1) << 1);
            bCodeChroma = ((absPartIdx & (qpdiv - 1)) == 0);
        }

        if (bSubdivAndCbf)
        {
            m_entropyCoder->encodeQtCbf(cu, absPartIdx, 0, trSize, trSize, TEXT_LUMA, trMode, true);
        }
        else
        {
            if (ttype == TEXT_LUMA && cu->getCbf(absPartIdx, TEXT_LUMA, trMode))
            {
                m_entropyCoder->encodeCoeffNxN(cu, coeffCurY, absPartIdx, trSize, TEXT_LUMA);
            }
            if (bCodeChroma)
            {
                uint32_t coeffOffsetC = coeffOffsetY >> (m_hChromaShift + m_vChromaShift);
                coeff_t *coeffCurU = m_qtTempCoeffCb[qtlayer] + coeffOffsetC;
                coeff_t *coeffCurV = m_qtTempCoeffCr[qtlayer] + coeffOffsetC;
                uint32_t trSizeC = 1 << trSizeCLog2;

                if (!splitIntoSubTUs)
                {
                    if (ttype == TEXT_CHROMA_U && cu->getCbf(absPartIdx, TEXT_CHROMA_U, trMode))
                    {
                        m_entropyCoder->encodeCoeffNxN(cu, coeffCurU, absPartIdx, trSizeC, TEXT_CHROMA_U);
                    }
                    if (ttype == TEXT_CHROMA_V && cu->getCbf(absPartIdx, TEXT_CHROMA_V, trMode))
                    {
                        m_entropyCoder->encodeCoeffNxN(cu, coeffCurV, absPartIdx, trSizeC, TEXT_CHROMA_V);
                    }
                }
                else
                {
                    uint32_t partIdxesPerSubTU  = cu->getPic()->getNumPartInCU() >> (((cu->getDepth(absPartIdx) + trModeC) << 1) + 1);
                    uint32_t subTUSize = trSizeC * trSizeC;
                    if (ttype == TEXT_CHROMA_U && cu->getCbf(absPartIdx, TEXT_CHROMA_U, trMode))
                    {
                        if (cu->getCbf(absPartIdx, ttype, trMode + 1))
                            m_entropyCoder->encodeCoeffNxN(cu, coeffCurU, absPartIdx, trSizeC, TEXT_CHROMA_U);
                        if (cu->getCbf(absPartIdx + partIdxesPerSubTU, ttype, trMode + 1))
                            m_entropyCoder->encodeCoeffNxN(cu, coeffCurU + subTUSize, absPartIdx + partIdxesPerSubTU, trSizeC, TEXT_CHROMA_U);
                    }
                    if (ttype == TEXT_CHROMA_V && cu->getCbf(absPartIdx, TEXT_CHROMA_V, trMode))
                    {
                        if (cu->getCbf(absPartIdx, ttype, trMode + 1))
                            m_entropyCoder->encodeCoeffNxN(cu, coeffCurV, absPartIdx, trSizeC, TEXT_CHROMA_V);
                        if (cu->getCbf(absPartIdx + partIdxesPerSubTU, ttype, trMode + 1))
                            m_entropyCoder->encodeCoeffNxN(cu, coeffCurV + subTUSize, absPartIdx + partIdxesPerSubTU, trSizeC, TEXT_CHROMA_V);
                    }
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

void TEncSearch::xSetResidualQTData(TComDataCU* cu, uint32_t absPartIdx, ShortYuv* resiYuv, uint32_t depth, bool bSpatial)
{
    X265_CHECK(cu->getDepth(0) == cu->getDepth(absPartIdx), "depth not matching\n");
    const uint32_t curTrMode = depth - cu->getDepth(0);
    const uint32_t trMode    = cu->getTransformIdx(absPartIdx);

    if (curTrMode == trMode)
    {
        int            chFmt      = cu->getChromaFormat();
        const uint32_t trSizeLog2 = cu->getSlice()->getSPS()->getLog2MaxCodingBlockSize() - depth;
        const uint32_t qtlayer    = cu->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - trSizeLog2;

        uint32_t trSizeCLog2 = trSizeLog2 - m_hChromaShift;
        bool bCodeChroma = true;
        bool bChromaSame = false;
        uint32_t trModeC = trMode;
        if ((trSizeLog2 == 2) && !(chFmt == CHROMA_444))
        {
            trSizeCLog2++;
            trModeC--;
            uint32_t qpdiv = cu->getPic()->getNumPartInCU() >> ((cu->getDepth(0) + trModeC) << 1);
            bCodeChroma = ((absPartIdx & (qpdiv - 1)) == 0);
            bChromaSame = true;
        }

        if (bSpatial)
        {
            uint32_t trSize = 1 << trSizeLog2;
            m_qtTempShortYuv[qtlayer].copyPartToPartLuma(resiYuv, absPartIdx, trSize, trSize);

            if (bCodeChroma)
            {
                m_qtTempShortYuv[qtlayer].copyPartToPartChroma(resiYuv, absPartIdx, 1 << trSizeLog2, (bChromaSame && (chFmt != CHROMA_422)));
            }
        }
        else
        {
            uint32_t numCoeffY = 1 << (trSizeLog2 * 2);
            uint32_t coeffOffsetY = absPartIdx << cu->getPic()->getLog2UnitSize() * 2;
            coeff_t* coeffSrcY = m_qtTempCoeffY[qtlayer] + coeffOffsetY;
            coeff_t* coeffDstY = cu->getCoeffY() + coeffOffsetY;
            ::memcpy(coeffDstY, coeffSrcY, sizeof(coeff_t) * numCoeffY);
            if (bCodeChroma)
            {
                uint32_t numCoeffC = 1 << (trSizeCLog2 * 2 + (chFmt == CHROMA_422));
                uint32_t coeffOffsetC = coeffOffsetY >> (m_hChromaShift + m_vChromaShift);

                coeff_t* coeffSrcU = m_qtTempCoeffCb[qtlayer] + coeffOffsetC;
                coeff_t* coeffSrcV = m_qtTempCoeffCr[qtlayer] + coeffOffsetC;
                coeff_t* coeffDstU = cu->getCoeffCb() + coeffOffsetC;
                coeff_t* coeffDstV = cu->getCoeffCr() + coeffOffsetC;
                ::memcpy(coeffDstU, coeffSrcU, sizeof(coeff_t) * numCoeffC);
                ::memcpy(coeffDstV, coeffSrcV, sizeof(coeff_t) * numCoeffC);
            }
        }
    }
    else
    {
        const uint32_t qPartNumSubdiv = cu->getPic()->getNumPartInCU() >> ((depth + 1) << 1);
        for (uint32_t i = 0; i < 4; ++i)
        {
            xSetResidualQTData(cu, absPartIdx + i * qPartNumSubdiv, resiYuv, depth + 1, bSpatial);
        }
    }
}

uint32_t TEncSearch::xModeBitsIntra(TComDataCU* cu, uint32_t mode, uint32_t partOffset, uint32_t depth)
{
    // Reload only contexts required for coding intra mode information
    m_rdGoOnSbacCoder->loadIntraDirModeLuma(m_rdSbacCoders[depth][CI_CURR_BEST]);

    cu->getLumaIntraDir()[partOffset] = (uint8_t)mode;

    m_entropyCoder->resetBits();
    m_entropyCoder->encodeIntraDirModeLuma(cu, partOffset);

    return m_entropyCoder->getNumberOfWrittenBits();
}

uint32_t TEncSearch::xModeBitsRemIntra(TComDataCU* cu, uint32_t partOffset, uint32_t depth, uint32_t preds[3], uint64_t& mpms)
{
    mpms = 0;
    for (int i = 0; i < 3; ++i)
    {
        mpms |= ((uint64_t)1 << preds[i]);
    }

    uint32_t mode = 34;
    while (mpms & ((uint64_t)1 << mode))
    {
        --mode;
    }

    return xModeBitsIntra(cu, mode, partOffset, depth);
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
            m_entropyCoder->encodeCUTransquantBypassFlag(cu, 0);
        }
        m_entropyCoder->encodeSkipFlag(cu, 0);
        m_entropyCoder->encodeMergeIndex(cu, 0);
        return m_entropyCoder->getNumberOfWrittenBits();
    }
    else
    {
        m_entropyCoder->resetBits();
        if (cu->getSlice()->getPPS()->getTransquantBypassEnableFlag())
        {
            m_entropyCoder->encodeCUTransquantBypassFlag(cu, 0);
        }
        m_entropyCoder->encodeSkipFlag(cu, 0);
        m_entropyCoder->encodePredMode(cu, 0);
        m_entropyCoder->encodePartSize(cu, 0, cu->getDepth(0));
        m_entropyCoder->encodePredInfo(cu, 0);
        bool bDummy = false;
        m_entropyCoder->encodeCoeff(cu, 0, cu->getDepth(0), cu->getCUSize(0), bDummy);
        return m_entropyCoder->getNumberOfWrittenBits();
    }
}

//! \}
