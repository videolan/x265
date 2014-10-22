/*****************************************************************************
* Copyright (C) 2013 x265 project
*
* Authors: Steve Borho <steve@borho.org>
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
#include "primitives.h"
#include "picyuv.h"
#include "cudata.h"

#include "TLibCommon/TComRom.h"
#include "TLibCommon/TComMotionInfo.h"

#include "search.h"
#include "entropy.h"
#include "rdcost.h"

using namespace x265;

#if _MSC_VER
#pragma warning(disable: 4800) // 'uint8_t' : forcing value to bool 'true' or 'false' (performance warning)
#pragma warning(disable: 4244) // '=' : conversion from 'int' to 'uint8_t', possible loss of data)
#endif

ALIGN_VAR_32(const pixel, Search::zeroPixel[MAX_CU_SIZE]) = { 0 };
ALIGN_VAR_32(const int16_t, Search::zeroShort[MAX_CU_SIZE]) = { 0 };

Search::Search() : JobProvider(NULL)
{
    memset(m_rqt, 0, sizeof(m_rqt));

    for (int i = 0; i < 3; i++)
    {
        m_qtTempTransformSkipFlag[i] = NULL;
        m_qtTempCbf[i] = NULL;
    }

    m_numLayers = 0;
    m_param = NULL;
    m_slice = NULL;
    m_frame = NULL;
    m_bJobsQueued = false;
    m_totalNumME = m_numAcquiredME = m_numCompletedME = 0;
}

bool Search::initSearch(x265_param *param, ScalingList& scalingList)
{
    m_param = param;
    m_bEnableRDOQ = param->rdLevel >= 4;
    m_bFrameParallel = param->frameNumThreads > 1;
    m_numLayers = g_log2Size[param->maxCUSize] - 2;

    m_rdCost.setPsyRdScale(param->psyRd);
    m_me.setSearchMethod(param->searchMethod);
    m_me.setSubpelRefine(param->subpelRefine);

    bool ok = m_quant.init(m_bEnableRDOQ, param->psyRdoq, scalingList, m_entropyCoder);
    ok &= Predict::allocBuffers(param->internalCsp);

    /* When frame parallelism is active, only 'refLagPixels' of reference frames will be guaranteed
     * available for motion reference.  See refLagRows in FrameEncoder::compressCTURows() */
    m_refLagPixels = m_bFrameParallel ? param->searchRange : param->sourceHeight;

    uint32_t sizeL = 1 << (g_maxLog2CUSize * 2);
    uint32_t sizeC = sizeL >> (CHROMA_H_SHIFT(m_csp) + CHROMA_V_SHIFT(m_csp));
    uint32_t numPartitions = 1 << (g_maxFullDepth * 2);

    for (int i = 0; i < m_numLayers; i++)
    {
        CHECKED_MALLOC(m_rqt[i].tmpCoeff[0], coeff_t, sizeL + sizeC * 2);
        m_rqt[i].tmpCoeff[1] = m_rqt[i].tmpCoeff[0] + sizeL;
        m_rqt[i].tmpCoeff[2] = m_rqt[i].tmpCoeff[0] + sizeL + sizeC;

        int cuSize = g_maxCUSize >> i;
        ok &= m_rqt[i].tmpShortYuv.create(MAX_CU_SIZE, param->internalCsp); // TODO: why not size this per depth?
        ok &= m_rqt[i].tmpResiYuv.create(cuSize, m_param->internalCsp);
        ok &= m_rqt[i].tmpReconYuv.create(cuSize, m_param->internalCsp);
        ok &= m_rqt[i].tmpPredYuv.create(cuSize, param->internalCsp);
        ok &= m_rqt[i].bidirPredYuv[0].create(cuSize, m_param->internalCsp);
        ok &= m_rqt[i].bidirPredYuv[1].create(cuSize, m_param->internalCsp);
    }

    CHECKED_MALLOC(m_qtTempCbf[0], uint8_t, numPartitions * 3);
    m_qtTempCbf[1] = m_qtTempCbf[0] + numPartitions;
    m_qtTempCbf[2] = m_qtTempCbf[0] + numPartitions * 2;
    CHECKED_MALLOC(m_qtTempTransformSkipFlag[0], uint8_t, numPartitions * 3);
    m_qtTempTransformSkipFlag[1] = m_qtTempTransformSkipFlag[0] + numPartitions;
    m_qtTempTransformSkipFlag[2] = m_qtTempTransformSkipFlag[0] + numPartitions * 2;

    return ok;

fail:
    return false;
}

Search::~Search()
{
    for (int i = 0; i < m_numLayers; i++)
    {
        X265_FREE(m_rqt[i].tmpCoeff[0]);
        m_rqt[i].tmpShortYuv.destroy();
        m_rqt[i].tmpResiYuv.destroy();
        m_rqt[i].tmpReconYuv.destroy();
        m_rqt[i].tmpPredYuv.destroy();
        m_rqt[i].bidirPredYuv[0].destroy();
        m_rqt[i].bidirPredYuv[1].destroy();
    }

    X265_FREE(m_qtTempCbf[0]);
    X265_FREE(m_qtTempTransformSkipFlag[0]);
}

void Search::setQP(const Slice& slice, int qp)
{
    m_me.setQP(qp);
    m_rdCost.setQP(slice, qp);
}

#if CHECKED_BUILD || _DEBUG
void Search::invalidateContexts(int fromDepth)
{
    /* catch reads without previous writes */
    for (int d = fromDepth; d < NUM_FULL_DEPTH; d++)
    {
        m_rqt[d].cur.markInvalid();
        m_rqt[d].rqtTemp.markInvalid();
        m_rqt[d].rqtRoot.markInvalid();
        m_rqt[d].rqtTest.markInvalid();
    }
}
#else
void Search::invalidateContexts(int) {}
#endif

void Search::updateModeCost(Mode& m) const
{
    if (m_rdCost.m_psyRd)
        m.rdCost = m_rdCost.calcPsyRdCost(m.distortion, m.totalBits, m.psyEnergy);
    else
        m.rdCost = m_rdCost.calcRdCost(m.distortion, m.totalBits);
}

void Search::xEncSubdivCbfQTChroma(const CUData& cu, uint32_t trDepth, uint32_t absPartIdx, uint32_t absPartIdxStep, uint32_t width, uint32_t height)
{
    uint32_t fullDepth  = cu.m_depth[0] + trDepth;
    uint32_t trMode     = cu.m_trIdx[absPartIdx];
    uint32_t subdiv     = (trMode > trDepth ? 1 : 0);
    uint32_t log2TrSize = g_maxLog2CUSize - fullDepth;

    int hChromaShift = CHROMA_H_SHIFT(m_csp);
    int vChromaShift = CHROMA_V_SHIFT(m_csp);

    if ((log2TrSize > 2) && !(m_csp == X265_CSP_I444))
    {
        if (!trDepth || cu.getCbf(absPartIdx, TEXT_CHROMA_U, trDepth - 1))
            m_entropyCoder.codeQtCbf(cu, absPartIdx, absPartIdxStep, (width >> hChromaShift), (height >> vChromaShift), TEXT_CHROMA_U, trDepth, !subdiv);

        if (!trDepth || cu.getCbf(absPartIdx, TEXT_CHROMA_V, trDepth - 1))
            m_entropyCoder.codeQtCbf(cu, absPartIdx, absPartIdxStep, (width >> hChromaShift), (height >> vChromaShift), TEXT_CHROMA_V, trDepth, !subdiv);
    }

    if (subdiv)
    {
        absPartIdxStep >>= 2;
        width  >>= 1;
        height >>= 1;

        uint32_t qtPartNum = NUM_CU_PARTITIONS >> ((fullDepth + 1) << 1);
        for (uint32_t part = 0; part < 4; part++)
            xEncSubdivCbfQTChroma(cu, trDepth + 1, absPartIdx + part * qtPartNum, absPartIdxStep, width, height);
    }
}

void Search::xEncCoeffQTChroma(const CUData& cu, uint32_t trDepth, uint32_t absPartIdx, TextType ttype)
{
    if (!cu.getCbf(absPartIdx, ttype, trDepth))
        return;

    uint32_t fullDepth = cu.m_depth[0] + trDepth;
    uint32_t trMode    = cu.m_trIdx[absPartIdx];

    if (trMode > trDepth)
    {
        uint32_t qtPartNum = NUM_CU_PARTITIONS >> ((fullDepth + 1) << 1);
        for (uint32_t part = 0; part < 4; part++)
            xEncCoeffQTChroma(cu, trDepth + 1, absPartIdx + part * qtPartNum, ttype);

        return;
    }

    uint32_t log2TrSize = g_maxLog2CUSize - fullDepth;

    uint32_t trDepthC = trDepth;
    int hChromaShift = CHROMA_H_SHIFT(m_csp);
    uint32_t log2TrSizeC = log2TrSize - hChromaShift;
    
    if ((log2TrSize == 2) && !(m_csp == X265_CSP_I444))
    {
        X265_CHECK(trDepth > 0, "transform size too small\n");
        trDepthC--;
        log2TrSizeC++;
        uint32_t qpdiv = NUM_CU_PARTITIONS >> ((cu.m_depth[0] + trDepthC) << 1);
        bool bFirstQ = ((absPartIdx & (qpdiv - 1)) == 0);
        if (!bFirstQ)
            return;
    }

    uint32_t qtLayer = log2TrSize - 2;

    if (m_csp != X265_CSP_I422)
    {
        uint32_t shift = (m_csp == X265_CSP_I420) ? 2 : 0;
        uint32_t coeffOffset = absPartIdx << (LOG2_UNIT_SIZE * 2 - shift);
        coeff_t* coeff = m_rqt[qtLayer].tmpCoeff[ttype] + coeffOffset;
        m_entropyCoder.codeCoeffNxN(cu, coeff, absPartIdx, log2TrSizeC, ttype);
    }
    else
    {
        uint32_t coeffOffset = absPartIdx << (LOG2_UNIT_SIZE * 2 - 1);
        coeff_t* coeff = m_rqt[qtLayer].tmpCoeff[ttype] + coeffOffset;
        uint32_t subTUSize = 1 << (log2TrSizeC * 2);
        uint32_t partIdxesPerSubTU  = NUM_CU_PARTITIONS >> (((cu.m_depth[absPartIdx] + trDepthC) << 1) + 1);
        if (cu.getCbf(absPartIdx, ttype, trDepth + 1))
            m_entropyCoder.codeCoeffNxN(cu, coeff, absPartIdx, log2TrSizeC, ttype);
        if (cu.getCbf(absPartIdx + partIdxesPerSubTU, ttype, trDepth + 1))
            m_entropyCoder.codeCoeffNxN(cu, coeff + subTUSize, absPartIdx + partIdxesPerSubTU, log2TrSizeC, ttype);
    }
}

uint32_t Search::calcIntraChromaRecon(Mode& mode, const CUGeom& cuGeom, uint32_t absPartIdx, uint32_t chromaId, uint32_t log2TrSizeC,
                                      int16_t* reconQt, uint32_t reconQtStride, coeff_t* coeff, uint32_t& cbf)
{
    CUData* cu = &mode.cu;
    Yuv* predYuv = &mode.predYuv;
    ShortYuv* resiYuv = &mode.resiYuv;
    const Yuv* fencYuv = mode.fencYuv;

    TextType ttype        = (TextType)chromaId;
    uint32_t stride       = fencYuv->m_csize;
    pixel*   fenc         = const_cast<pixel*>(fencYuv->getChromaAddr(chromaId, absPartIdx));
    pixel*   pred         = predYuv->getChromaAddr(chromaId, absPartIdx);
    int16_t* residual     = resiYuv->getChromaAddr(chromaId, absPartIdx);

    uint32_t zorder           = cuGeom.encodeIdx + absPartIdx;
    pixel*   reconIPred       = m_frame->m_reconPicYuv->getChromaAddr(chromaId, cu->m_cuAddr, zorder);
    uint32_t reconIPredStride = m_frame->m_reconPicYuv->m_strideC;
    bool     useTransformSkipC = !!cu->m_transformSkip[ttype][absPartIdx];
    int      part = partitionFromLog2Size(log2TrSizeC);
    int      sizeIdxC = log2TrSizeC - 2;
    uint32_t dist;

#if CHECKED_BUILD || _DEBUG
    uint32_t tuSize       = 1 << log2TrSizeC;
    X265_CHECK(!((intptr_t)fenc & (tuSize - 1)), "fenc alignment check fail\n");
    X265_CHECK(!((intptr_t)pred & (tuSize - 1)), "pred alignment check fail\n");
    X265_CHECK(!((intptr_t)residual & (tuSize - 1)), "residual alignment check fail\n");
#endif
    primitives.calcresidual[sizeIdxC](fenc, pred, residual, stride);

    // init rate estimation arrays for RDOQ
    if (m_bEnableRDOQ)
        m_entropyCoder.estBit(m_entropyCoder.m_estBitsSbac, log2TrSizeC, false);

    uint32_t numSig = m_quant.transformNxN(cu, fenc, stride, residual, stride, coeff, log2TrSizeC, ttype, absPartIdx, useTransformSkipC);

    if (numSig)
    {
        X265_CHECK(log2TrSizeC <= 5, "log2TrSizeC is too large %d\n", log2TrSizeC);
        m_quant.invtransformNxN(cu->m_tqBypass[absPartIdx], residual, stride, coeff, log2TrSizeC, ttype, true, useTransformSkipC, numSig);
        primitives.calcrecon[sizeIdxC](pred, residual, reconQt, reconIPred, stride, reconQtStride, reconIPredStride);
        cbf = 1;
        dist = primitives.sse_sp[part](reconQt, reconQtStride, fenc, stride);
    }
    else
    {
#if CHECKED_BUILD || _DEBUG
        memset(coeff, 0, sizeof(coeff_t) << (log2TrSizeC * 2));
#endif
        // reconstruction
        primitives.square_copy_ps[sizeIdxC](reconQt,    reconQtStride,    pred, stride);
        primitives.square_copy_pp[sizeIdxC](reconIPred, reconIPredStride, pred, stride);
        cbf = 0;
        dist = primitives.sse_pp[part](pred, stride, fenc, stride);
    }

    X265_CHECK(ttype == TEXT_CHROMA_U || ttype == TEXT_CHROMA_V, "invalid ttype\n");
    return (ttype == TEXT_CHROMA_U) ? m_rdCost.scaleChromaDistCb(dist) : m_rdCost.scaleChromaDistCr(dist);
}

void Search::codeIntraLumaQT(Mode& mode, const CUGeom& cuGeom, uint32_t trDepth, uint32_t absPartIdx, bool bAllowSplit, Cost& outCost, uint32_t depthRange[2])
{
    uint32_t fullDepth  = mode.cu.m_depth[0] + trDepth;
    uint32_t log2TrSize = g_maxLog2CUSize - fullDepth;

    bool mightNotSplit  = log2TrSize <= depthRange[1];
    bool mightSplit     = (log2TrSize > depthRange[0]) && (bAllowSplit || !mightNotSplit);

    /* If maximum RD penalty, force spits at TU size 32x32 if SPS allows TUs of 16x16 */
    if (m_param->rdPenalty == 2 && m_slice->m_sliceType != I_SLICE && log2TrSize == 5 &&
        m_slice->m_sps->quadtreeTULog2MaxSize >= 4)
    {
        mightNotSplit = false;
        mightSplit = true;
    }

    CUData& cu = mode.cu;
    Yuv* predYuv = &mode.predYuv;
    const Yuv* fencYuv = mode.fencYuv;

    Cost fullCost;
    fullCost.rdcost = MAX_INT64;
    int      bTSkip = 0;
    uint32_t bCBF   = 0;

    if (mightNotSplit)
    {
        uint32_t tuSize = 1 << log2TrSize;

        pixel*   fenc     = const_cast<pixel*>(fencYuv->getLumaAddr(absPartIdx));
        pixel*   pred     = predYuv->getLumaAddr(absPartIdx);
        int16_t* residual = mode.resiYuv.getLumaAddr(absPartIdx);
        uint32_t stride   = fencYuv->m_size;
        int      sizeIdx  = log2TrSize - 2;

        // init availability pattern
        uint32_t lumaPredMode = cu.m_lumaIntraDir[absPartIdx];
        initAdiPattern(cu, cuGeom, absPartIdx, trDepth, lumaPredMode);

        // get prediction signal
        predIntraLumaAng(lumaPredMode, pred, stride, log2TrSize);

        cu.setTrIdxSubParts(trDepth, absPartIdx, fullDepth);

        uint32_t qtLayer      = log2TrSize - 2;
        uint32_t coeffOffsetY = absPartIdx << (LOG2_UNIT_SIZE * 2);
        coeff_t* coeffY       = m_rqt[qtLayer].tmpCoeff[0] + coeffOffsetY;
        int16_t* reconQt      = m_rqt[qtLayer].tmpShortYuv.getLumaAddr(absPartIdx);
        X265_CHECK(m_rqt[qtLayer].tmpShortYuv.m_size == MAX_CU_SIZE, "width is not max CU size\n");
        const uint32_t reconQtStride = MAX_CU_SIZE;

        pixel*   recon     = m_frame->m_reconPicYuv->getLumaAddr(cu.m_cuAddr, cuGeom.encodeIdx + absPartIdx);
        uint32_t picStride = m_frame->m_reconPicYuv->m_stride;

        // store original entropy coding status
        if (m_bEnableRDOQ)
            m_entropyCoder.estBit(m_entropyCoder.m_estBitsSbac, log2TrSize, true);

        m_entropyCoder.store(m_rqt[fullDepth].rqtRoot);

        ALIGN_VAR_32(coeff_t, tsCoeffY[32 * 32]);
        ALIGN_VAR_32(int16_t, tsReconY[32 * 32]);

        int checkTransformSkip = m_slice->m_pps->bTransformSkipEnabled && log2TrSize <= MAX_LOG2_TS_SIZE && !cu.m_tqBypass[0];
        if (m_param->bEnableTSkipFast)
            checkTransformSkip &= cu.m_partSize[absPartIdx] == SIZE_NxN;

        for (int useTSkip = 0; useTSkip <= checkTransformSkip; useTSkip++)
        {
            uint64_t tmpCost;
            uint32_t tmpEnergy = 0, tmpDist;

            coeff_t* coeff = (useTSkip ? tsCoeffY : coeffY);
            int16_t* reconShort = (useTSkip ? tsReconY : reconQt);
            uint32_t reconStride = (useTSkip ? tuSize : reconQtStride);

            primitives.calcresidual[sizeIdx](fenc, pred, residual, stride);

            uint32_t numSig = m_quant.transformNxN(&cu, fenc, stride, residual, stride, coeff, log2TrSize, TEXT_LUMA, absPartIdx, useTSkip);
            if (numSig)
            {
                m_quant.invtransformNxN(cu.m_tqBypass[0], residual, stride, coeff, log2TrSize, TEXT_LUMA, true, useTSkip, numSig);
                primitives.calcrecon[sizeIdx](pred, residual, reconShort, recon, stride, reconStride, picStride);
                tmpDist = primitives.sse_sp[sizeIdx](reconQt, reconQtStride, fenc, stride);
            }
            else
            {
                // no CBF reconstruction
                primitives.square_copy_ps[sizeIdx](reconShort, reconStride, pred, stride);
                primitives.square_copy_pp[sizeIdx](recon, picStride, pred, stride);
                tmpDist = primitives.sse_pp[sizeIdx](pred, stride, fenc, stride);
            }

            if (useTSkip && !numSig)
            {
                /* do not allow tskip if CBF=0, pretend we did not try tskip */
                checkTransformSkip = 0;
                break;
            }

            cu.setTransformSkipSubParts(useTSkip, TEXT_LUMA, absPartIdx, fullDepth);
            cu.setCbfSubParts((!!numSig) << trDepth, TEXT_LUMA, absPartIdx, fullDepth);

            if (useTSkip)
                m_entropyCoder.load(m_rqt[fullDepth].rqtRoot);

            m_entropyCoder.resetBits();
            if (!absPartIdx)
            {
                if (!cu.m_slice->isIntra())
                {
                    if (cu.m_slice->m_pps->bTransquantBypassEnabled)
                        m_entropyCoder.codeCUTransquantBypassFlag(cu.m_tqBypass[0]);
                    m_entropyCoder.codeSkipFlag(cu, 0);
                    m_entropyCoder.codePredMode(cu.m_predMode[0]);
                }

                m_entropyCoder.codePartSize(cu, 0, cu.m_depth[0]);
            }
            if (cu.m_partSize[0] == SIZE_2Nx2N)
            {
                if (!absPartIdx)
                    m_entropyCoder.codeIntraDirLumaAng(cu, 0, false);
            }
            else
            {
                uint32_t qtNumParts = cuGeom.numPartitions >> 2;
                if (!trDepth)
                {
                    for (uint32_t part = 0; part < 4; part++)
                        m_entropyCoder.codeIntraDirLumaAng(cu, part * qtNumParts, false);
                }
                else if (!(absPartIdx & (qtNumParts - 1)))
                    m_entropyCoder.codeIntraDirLumaAng(cu, absPartIdx, false);
            }
            if (log2TrSize != *depthRange)
                m_entropyCoder.codeTransformSubdivFlag(0, 5 - log2TrSize);

            m_entropyCoder.codeQtCbf(cu, absPartIdx, TEXT_LUMA, cu.m_trIdx[absPartIdx]);

            if (cu.getCbf(absPartIdx, TEXT_LUMA, trDepth))
                m_entropyCoder.codeCoeffNxN(cu, coeff, absPartIdx, log2TrSize, TEXT_LUMA);

            uint32_t tmpBits = m_entropyCoder.getNumberOfWrittenBits();

            if (m_param->rdPenalty && log2TrSize == 5 && m_slice->m_sliceType != I_SLICE)
                tmpBits *= 4;

            if (!useTSkip && checkTransformSkip)
                m_entropyCoder.store(m_rqt[fullDepth].rqtTemp);

            if (m_rdCost.m_psyRd)
            {
                uint32_t zorder = cuGeom.encodeIdx + absPartIdx;
                tmpEnergy = m_rdCost.psyCost(log2TrSize - 2, fenc, fencYuv->m_size,
                    m_frame->m_reconPicYuv->getLumaAddr(cu.m_cuAddr, zorder), m_frame->m_reconPicYuv->m_stride);
                tmpCost = m_rdCost.calcPsyRdCost(tmpDist, tmpBits, tmpEnergy);
            }
            else
                tmpCost = m_rdCost.calcRdCost(tmpDist, tmpBits);

            if (tmpCost < fullCost.rdcost)
            {
                bTSkip = useTSkip;
                bCBF   = !!numSig;
                fullCost.rdcost     = tmpCost;
                fullCost.distortion = tmpDist;
                fullCost.bits       = tmpBits;
                fullCost.energy     = tmpEnergy;
            }
        }

        if (bTSkip)
        {
            memcpy(coeffY, tsCoeffY, sizeof(coeff_t) << (log2TrSize * 2));
            primitives.square_copy_ss[sizeIdx](reconQt, reconQtStride, tsReconY, tuSize);
        }
        else
        {
            /* copy from int16 recon buffer to reconPic (bad on two counts) */
            pixel*   reconIPred = m_frame->m_reconPicYuv->getLumaAddr(cu.m_cuAddr, cuGeom.encodeIdx + absPartIdx);
            uint32_t reconIPredStride = m_frame->m_reconPicYuv->m_stride;
            primitives.square_copy_sp[sizeIdx](reconIPred, reconIPredStride, reconQt, reconQtStride);

            if (checkTransformSkip)
            {
                cu.setTransformSkipSubParts(false, TEXT_LUMA, absPartIdx, fullDepth);
                cu.setCbfSubParts(bCBF << trDepth, TEXT_LUMA, absPartIdx, fullDepth);
                m_entropyCoder.load(m_rqt[fullDepth].rqtTemp);
            }
        }
    }

    if (mightSplit)
    {
        if (mightNotSplit)
        {
            m_entropyCoder.store(m_rqt[fullDepth].rqtTest);  // save state after full TU encode
            m_entropyCoder.load(m_rqt[fullDepth].rqtRoot);   // prep state of split encode
        }

        // code splitted block
        uint32_t qPartsDiv = NUM_CU_PARTITIONS >> ((fullDepth + 1) << 1);
        uint32_t absPartIdxSub = absPartIdx;

        Cost splitCost;

        uint32_t cbf = 0;
        for (uint32_t part = 0; part < 4; part++, absPartIdxSub += qPartsDiv)
        {
            codeIntraLumaQT(mode, cuGeom, trDepth + 1, absPartIdxSub, bAllowSplit, splitCost, depthRange);
            cbf |= cu.getCbf(absPartIdxSub, TEXT_LUMA, trDepth + 1);
        }
        for (uint32_t offs = 0; offs < 4 * qPartsDiv; offs++)
            cu.m_cbf[0][absPartIdx + offs] |= (cbf << trDepth);

        /* TODO: can't we do this before the splits? */
        if (mightNotSplit && log2TrSize != depthRange[0])
        {
            /* If we could have coded this TU depth, include cost of subdiv flag */
            m_entropyCoder.resetBits();
            m_entropyCoder.codeTransformSubdivFlag(1, 5 - log2TrSize);
            splitCost.bits += m_entropyCoder.getNumberOfWrittenBits();

            if (m_rdCost.m_psyRd)
                splitCost.rdcost = m_rdCost.calcPsyRdCost(splitCost.distortion, splitCost.bits, splitCost.energy);
            else
                splitCost.rdcost = m_rdCost.calcRdCost(splitCost.distortion, splitCost.bits);
        }

        if (splitCost.rdcost < fullCost.rdcost)
        {
            outCost.rdcost     += splitCost.rdcost;
            outCost.distortion += splitCost.distortion;
            outCost.bits       += splitCost.bits;
            outCost.energy     += splitCost.energy;
            return;
        }
        else
        {
            // recover state of full-size TU encode
            m_entropyCoder.load(m_rqt[fullDepth].rqtTest);

            // recover transform index and Cbf values
            cu.setTrIdxSubParts(trDepth, absPartIdx, fullDepth);
            cu.setCbfSubParts(bCBF << trDepth, TEXT_LUMA, absPartIdx, fullDepth);
            cu.setTransformSkipSubParts(bTSkip, TEXT_LUMA, absPartIdx, fullDepth);

            // set reconstruction for next intra prediction blocks
            uint32_t qtLayer = log2TrSize - 2;
            uint32_t zorder  = cuGeom.encodeIdx + absPartIdx;
            int16_t* reconQt = m_rqt[qtLayer].tmpShortYuv.getLumaAddr(absPartIdx);
            X265_CHECK(m_rqt[qtLayer].tmpShortYuv.m_size == MAX_CU_SIZE, "width is not max CU size\n");
            const uint32_t reconQtStride = MAX_CU_SIZE;

            pixel*   dst       = m_frame->m_reconPicYuv->getLumaAddr(cu.m_cuAddr, zorder);
            uint32_t dststride = m_frame->m_reconPicYuv->m_stride;
            int sizeIdx = log2TrSize - 2;
            primitives.square_copy_sp[sizeIdx](dst, dststride, reconQt, reconQtStride);
        }
    }

    outCost.rdcost     += fullCost.rdcost;
    outCost.distortion += fullCost.distortion;
    outCost.bits       += fullCost.bits;
    outCost.energy     += fullCost.energy;
}

void Search::residualTransformQuantIntra(Mode& mode, const CUGeom& cuGeom, uint32_t trDepth, uint32_t absPartIdx, uint32_t depthRange[2])
{
    CUData* cu = &mode.cu;
    Yuv* predYuv = &mode.predYuv;
    Yuv* reconYuv = &mode.reconYuv;
    ShortYuv* resiYuv = &mode.resiYuv;
    const Yuv* fencYuv = mode.fencYuv;

    uint32_t fullDepth = cu->m_depth[0] + trDepth;
    uint32_t log2TrSize  = g_maxLog2CUSize - fullDepth;
    bool     bCheckFull  = log2TrSize <= depthRange[1];
    bool     bCheckSplit = log2TrSize > depthRange[0];

    if (m_param->rdPenalty == 2 && m_slice->m_sliceType != I_SLICE && log2TrSize == 5 &&
        m_slice->m_sps->quadtreeTULog2MaxSize >= 4)
    {
        bCheckFull = false;
        bCheckSplit = true;
    }

    if (bCheckFull)
    {
        cu->setTransformSkipSubParts(0, TEXT_LUMA, absPartIdx, fullDepth);

        // code luma block with given intra prediction mode and store Cbf
        uint32_t lumaPredMode = cu->m_lumaIntraDir[absPartIdx];
        uint32_t stride       = fencYuv->m_size;
        pixel*   fenc         = const_cast<pixel*>(fencYuv->getLumaAddr(absPartIdx));
        pixel*   pred         = predYuv->getLumaAddr(absPartIdx);
        int16_t* residual     = resiYuv->getLumaAddr(absPartIdx);
        pixel*   recon        = reconYuv->getLumaAddr(absPartIdx);
        uint32_t coeffOffsetY = absPartIdx << (LOG2_UNIT_SIZE * 2);
        coeff_t* coeff        = cu->m_trCoeff[0] + coeffOffsetY;

        uint32_t zorder           = cuGeom.encodeIdx + absPartIdx;
        pixel*   reconIPred       = m_frame->m_reconPicYuv->getLumaAddr(cu->m_cuAddr, zorder);
        uint32_t reconIPredStride = m_frame->m_reconPicYuv->m_stride;

        bool     useTransformSkip = !!cu->m_transformSkip[0][absPartIdx];

        initAdiPattern(*cu, cuGeom, absPartIdx, trDepth, lumaPredMode);
        predIntraLumaAng(lumaPredMode, pred, stride, log2TrSize);

        cu->setTrIdxSubParts(trDepth, absPartIdx, fullDepth);

#if CHECKED_BUILD || _DEBUG
        uint32_t tuSize       = 1 << log2TrSize;
        X265_CHECK(!((intptr_t)fenc & (tuSize - 1)), "fenc alignment failure\n");
        X265_CHECK(!((intptr_t)pred & (tuSize - 1)), "pred alignment failure\n");
        X265_CHECK(!((intptr_t)residual & (tuSize - 1)), "residual alignment failure\n");
#endif
        int sizeIdx = log2TrSize - 2;
        primitives.calcresidual[sizeIdx](fenc, pred, residual, stride);
        uint32_t numSig = m_quant.transformNxN(cu, fenc, stride, residual, stride, coeff, log2TrSize, TEXT_LUMA, absPartIdx, useTransformSkip);

        // set coded block flag
        cu->setCbfSubParts((!!numSig) << trDepth, TEXT_LUMA, absPartIdx, fullDepth);

        if (numSig)
        {
            m_quant.invtransformNxN(cu->m_tqBypass[absPartIdx], residual, stride, coeff, log2TrSize, TEXT_LUMA, true, useTransformSkip, numSig);

            // Generate Recon
            primitives.luma_add_ps[sizeIdx](recon, stride, pred, residual, stride, stride);
            primitives.square_copy_pp[sizeIdx](reconIPred, reconIPredStride, recon, stride);
        }
        else
        {
#if CHECKED_BUILD || _DEBUG
            memset(coeff, 0, sizeof(coeff_t) << (log2TrSize * 2));
#endif

            // Generate Recon
            primitives.square_copy_pp[sizeIdx](recon,      stride,           pred, stride);
            primitives.square_copy_pp[sizeIdx](reconIPred, reconIPredStride, pred, stride);
        }
    }

    if (bCheckSplit && !bCheckFull)
    {
        // code splitted block

        uint32_t qPartsDiv     = NUM_CU_PARTITIONS >> ((fullDepth + 1) << 1);
        uint32_t absPartIdxSub = absPartIdx;
        uint32_t splitCbfY     = 0;

        for (uint32_t part = 0; part < 4; part++, absPartIdxSub += qPartsDiv)
        {
            residualTransformQuantIntra(mode, cuGeom, trDepth + 1, absPartIdxSub, depthRange);
            splitCbfY |= cu->getCbf(absPartIdxSub, TEXT_LUMA, trDepth + 1);
        }

        for (uint32_t offs = 0; offs < 4 * qPartsDiv; offs++)
            cu->m_cbf[0][absPartIdx + offs] |= (splitCbfY << trDepth);
    }
}

void Search::xSetIntraResultQT(CUData* cu, uint32_t trDepth, uint32_t absPartIdx, Yuv* reconYuv)
{
    uint32_t fullDepth = cu->m_depth[0] + trDepth;
    uint32_t trMode    = cu->m_trIdx[absPartIdx];

    if (trMode == trDepth)
    {
        uint32_t log2TrSize = g_maxLog2CUSize - fullDepth;
        uint32_t qtLayer    = log2TrSize - 2;

        // copy transform coefficients
        uint32_t coeffOffsetY = absPartIdx << (LOG2_UNIT_SIZE * 2);
        coeff_t* coeffSrcY    = m_rqt[qtLayer].tmpCoeff[0] + coeffOffsetY;
        coeff_t* coeffDestY   = cu->m_trCoeff[0]           + coeffOffsetY;
        ::memcpy(coeffDestY, coeffSrcY, sizeof(coeff_t) << (log2TrSize * 2));

        // copy reconstruction
        m_rqt[qtLayer].tmpShortYuv.copyPartToPartLuma(*reconYuv, absPartIdx, log2TrSize);
    }
    else
    {
        uint32_t numQPart = NUM_CU_PARTITIONS >> ((fullDepth + 1) << 1);
        for (uint32_t part = 0; part < 4; part++)
            xSetIntraResultQT(cu, trDepth + 1, absPartIdx + part * numQPart, reconYuv);
    }
}

void Search::offsetSubTUCBFs(CUData* cu, TextType ttype, uint32_t trDepth, uint32_t absPartIdx)
{
    uint32_t depth = cu->m_depth[0];
    uint32_t fullDepth = depth + trDepth;
    uint32_t log2TrSize = g_maxLog2CUSize - fullDepth;

    uint32_t trDepthC = trDepth;
    if (log2TrSize == 2 && cu->m_chromaFormat != X265_CSP_I444)
    {
        X265_CHECK(trDepthC > 0, "trDepthC invalid\n");
        trDepthC--;
    }

    uint32_t partIdxesPerSubTU = (NUM_CU_PARTITIONS >> ((depth + trDepthC) << 1)) >> 1;

    // move the CBFs down a level and set the parent CBF
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

/* returns distortion */
uint32_t Search::codeIntraChromaQt(Mode& mode, const CUGeom& cuGeom, uint32_t trDepth, uint32_t absPartIdx, uint32_t& psyEnergy)
{
    CUData* cu = &mode.cu;
    Yuv* predYuv = &mode.predYuv;
    const Yuv* fencYuv = mode.fencYuv;

    uint32_t fullDepth = cu->m_depth[0] + trDepth;
    uint32_t trMode    = cu->m_trIdx[absPartIdx];
    int hChromaShift = CHROMA_H_SHIFT(m_csp);
    int vChromaShift = CHROMA_V_SHIFT(m_csp);
    uint32_t outDist = 0;

    if (trMode > trDepth)
    {
        uint32_t splitCbfU = 0;
        uint32_t splitCbfV = 0;
        uint32_t splitPsyEnergy = 0;
        uint32_t qPartsDiv = NUM_CU_PARTITIONS >> ((fullDepth + 1) << 1);
        uint32_t absPartIdxSub = absPartIdx;

        for (uint32_t part = 0; part < 4; part++, absPartIdxSub += qPartsDiv)
        {
            uint32_t psyEnergyTemp = 0;
            outDist += codeIntraChromaQt(mode, cuGeom, trDepth + 1, absPartIdxSub, psyEnergyTemp);
            splitPsyEnergy += psyEnergyTemp;
            splitCbfU |= cu->getCbf(absPartIdxSub, TEXT_CHROMA_U, trDepth + 1);
            splitCbfV |= cu->getCbf(absPartIdxSub, TEXT_CHROMA_V, trDepth + 1);
        }

        for (uint32_t offs = 0; offs < 4 * qPartsDiv; offs++)
        {
            cu->m_cbf[1][absPartIdx + offs] |= (splitCbfU << trDepth);
            cu->m_cbf[2][absPartIdx + offs] |= (splitCbfV << trDepth);
        }

        psyEnergy = splitPsyEnergy;
        return outDist;
    }

    uint32_t log2TrSize = g_maxLog2CUSize - fullDepth;
    uint32_t log2TrSizeC = log2TrSize - hChromaShift;

    uint32_t trDepthC = trDepth;
    if (log2TrSize == 2 && m_csp != X265_CSP_I444)
    {
        X265_CHECK(trDepth > 0, "invalid trDepth\n");
        trDepthC--;
        log2TrSizeC++;
        uint32_t qpdiv = NUM_CU_PARTITIONS >> ((cu->m_depth[0] + trDepthC) << 1);
        bool bFirstQ = ((absPartIdx & (qpdiv - 1)) == 0);
        if (!bFirstQ)
            return outDist;
    }

    uint32_t qtLayer = log2TrSize - 2;
    uint32_t tuSize = 1 << log2TrSizeC;
    uint32_t stride = fencYuv->m_csize;
    const bool splitIntoSubTUs = (m_csp == X265_CSP_I422);

    bool checkTransformSkip = m_slice->m_pps->bTransformSkipEnabled && log2TrSizeC <= MAX_LOG2_TS_SIZE && !cu->m_tqBypass[0];

    if (m_param->bEnableTSkipFast)
    {
        checkTransformSkip &= (log2TrSize <= MAX_LOG2_TS_SIZE);
        if (checkTransformSkip)
        {
            int nbLumaSkip = 0;
            for (uint32_t absPartIdxSub = absPartIdx; absPartIdxSub < absPartIdx + 4; absPartIdxSub++)
                nbLumaSkip += cu->m_transformSkip[0][absPartIdxSub];

            checkTransformSkip &= (nbLumaSkip > 0);
        }
    }

    uint32_t singlePsyEnergy = 0;
    for (uint32_t chromaId = TEXT_CHROMA_U; chromaId <= TEXT_CHROMA_V; chromaId++)
    {
        uint32_t curPartNum = NUM_CU_PARTITIONS >> ((cu->m_depth[0] + trDepthC) << 1);
        TURecurse tuIterator(splitIntoSubTUs ? VERTICAL_SPLIT : DONT_SPLIT, curPartNum, absPartIdx);

        do
        {
            uint32_t absPartIdxC = tuIterator.absPartIdxTURelCU;
            pixel*   pred        = predYuv->getChromaAddr(chromaId, absPartIdxC);

            // init availability pattern
            initAdiPatternChroma(*cu, cuGeom, absPartIdxC, trDepthC, chromaId);
            pixel* chromaPred = getAdiChromaBuf(chromaId, tuSize);

            uint32_t chromaPredMode = cu->m_chromaIntraDir[absPartIdxC];
            if (chromaPredMode == DM_CHROMA_IDX)
                chromaPredMode = cu->m_lumaIntraDir[(m_csp == X265_CSP_I444) ? absPartIdxC : 0];
            if (m_csp == X265_CSP_I422)
                chromaPredMode = g_chroma422IntraAngleMappingTable[chromaPredMode];

            // get prediction signal
            predIntraChromaAng(chromaPred, chromaPredMode, pred, stride, log2TrSizeC, m_csp);

            uint32_t singleCbfC     = 0;
            uint32_t singlePsyEnergyTmp = 0;

            int16_t* reconQt       = m_rqt[qtLayer].tmpShortYuv.getChromaAddr(chromaId, absPartIdxC);
            uint32_t reconQtStride = m_rqt[qtLayer].tmpShortYuv.m_csize;
            uint32_t coeffOffsetC  = absPartIdxC << (LOG2_UNIT_SIZE * 2 - (hChromaShift + vChromaShift));
            coeff_t* coeffC        = m_rqt[qtLayer].tmpCoeff[chromaId] + coeffOffsetC;

            if (checkTransformSkip)
            {
                // use RDO to decide whether Cr/Cb takes TS
                m_entropyCoder.store(m_rqt[fullDepth].rqtRoot);

                uint64_t singleCost     = MAX_INT64;
                int      bestModeId     = 0;
                uint32_t singleDistC    = 0;
                uint32_t singleDistCTmp = 0;
                uint64_t singleCostTmp  = 0;
                uint32_t singleCbfCTmp  = 0;

                const int firstCheckId  = 0;

                ALIGN_VAR_32(coeff_t, tsCoeffC[MAX_TS_SIZE * MAX_TS_SIZE]);
                ALIGN_VAR_32(int16_t, tsReconC[MAX_TS_SIZE * MAX_TS_SIZE]);

                for (int chromaModeId = firstCheckId; chromaModeId < 2; chromaModeId++)
                {
                    coeff_t* coeff = (chromaModeId ? tsCoeffC : coeffC);
                    int16_t* recon = (chromaModeId ? tsReconC : reconQt);
                    uint32_t reconStride = (chromaModeId ? tuSize : reconQtStride);

                    cu->setTransformSkipPartRange(chromaModeId, (TextType)chromaId, absPartIdxC, tuIterator.absPartIdxStep);

                    singleDistCTmp = calcIntraChromaRecon(mode, cuGeom, absPartIdxC, chromaId, log2TrSizeC, recon, reconStride, coeff, singleCbfCTmp);
                    cu->setCbfPartRange(singleCbfCTmp << trDepth, (TextType)chromaId, absPartIdxC, tuIterator.absPartIdxStep);

                    if (chromaModeId == 1 && !singleCbfCTmp)
                        //In order not to code TS flag when cbf is zero, the case for TS with cbf being zero is forbidden.
                        break;
                    else
                    {
                        uint32_t bitsTmp;
                        if (singleCbfCTmp)
                        {
                            m_entropyCoder.resetBits();
                            m_entropyCoder.codeCoeffNxN(*cu, coeff, absPartIdx, log2TrSizeC, (TextType)chromaId);
                            bitsTmp = m_entropyCoder.getNumberOfWrittenBits();
                        }
                        else
                            bitsTmp = 0;

                        if (m_rdCost.m_psyRd)
                        {
                            uint32_t zorder = cuGeom.encodeIdx + absPartIdxC;
                            pixel*   fenc = const_cast<pixel*>(fencYuv->getChromaAddr(chromaId, absPartIdxC));
                            singlePsyEnergyTmp = m_rdCost.psyCost(log2TrSizeC - 2, fenc, fencYuv->m_csize,
                                m_frame->m_reconPicYuv->getChromaAddr(chromaId, cu->m_cuAddr, zorder), m_frame->m_reconPicYuv->m_strideC);
                            singleCostTmp = m_rdCost.calcPsyRdCost(singleDistCTmp, bitsTmp, singlePsyEnergyTmp);
                        }
                        else
                            singleCostTmp = m_rdCost.calcRdCost(singleDistCTmp, bitsTmp);
                    }

                    if (singleCostTmp < singleCost)
                    {
                        singleCost  = singleCostTmp;
                        singleDistC = singleDistCTmp;
                        bestModeId  = chromaModeId;
                        singleCbfC  = singleCbfCTmp;
                        singlePsyEnergy = singlePsyEnergyTmp;
                        if (bestModeId == firstCheckId)
                            m_entropyCoder.store(m_rqt[fullDepth].rqtTemp);
                    }
                    if (chromaModeId == firstCheckId)
                        m_entropyCoder.load(m_rqt[fullDepth].rqtRoot);
                }

                if (bestModeId == firstCheckId)
                {
                    /* copy from int16 recon buffer to reconPic (bad on two counts) */
                    pixel*   reconIPred = m_frame->m_reconPicYuv->getChromaAddr(chromaId, cu->m_cuAddr, cuGeom.encodeIdx + absPartIdxC);
                    uint32_t reconIPredStride = m_frame->m_reconPicYuv->m_strideC;
                    primitives.square_copy_sp[log2TrSizeC - 2](reconIPred, reconIPredStride, reconQt, reconQtStride);

                    cu->setCbfPartRange(singleCbfC << trDepth, (TextType)chromaId, absPartIdxC, tuIterator.absPartIdxStep);
                    m_entropyCoder.load(m_rqt[fullDepth].rqtTemp);
                }
                else
                {
                    ::memcpy(coeffC, tsCoeffC, sizeof(coeff_t) << (log2TrSizeC * 2));
                    int sizeIdxC = log2TrSizeC - 2;
                    primitives.square_copy_ss[sizeIdxC](reconQt, reconQtStride, tsReconC, tuSize);
                }

                cu->setTransformSkipPartRange(bestModeId, (TextType)chromaId, absPartIdxC, tuIterator.absPartIdxStep);

                outDist += singleDistC;

                if (chromaId == 1)
                    m_entropyCoder.store(m_rqt[fullDepth].rqtRoot);
            }
            else
            {
                cu->setTransformSkipPartRange(0, (TextType)chromaId, absPartIdxC, tuIterator.absPartIdxStep);
                outDist += calcIntraChromaRecon(mode, cuGeom, absPartIdxC, chromaId, log2TrSizeC, reconQt, reconQtStride, coeffC, singleCbfC);
                if (m_rdCost.m_psyRd)
                {
                    uint32_t zorder = cuGeom.encodeIdx + absPartIdxC;
                    pixel *fenc = const_cast<pixel*>(fencYuv->getChromaAddr(chromaId, absPartIdxC));
                    singlePsyEnergyTmp = m_rdCost.psyCost(log2TrSizeC - 2, fenc, fencYuv->m_csize,
                        m_frame->m_reconPicYuv->getChromaAddr(chromaId, cu->m_cuAddr, zorder), m_frame->m_reconPicYuv->m_strideC);
                }
                cu->setCbfPartRange(singleCbfC << trDepth, (TextType)chromaId, absPartIdxC, tuIterator.absPartIdxStep);
            }
            singlePsyEnergy += singlePsyEnergyTmp;
        }
        while (tuIterator.isNextSection());

        if (splitIntoSubTUs)
            offsetSubTUCBFs(cu, (TextType)chromaId, trDepth, absPartIdx);
    }

    psyEnergy = singlePsyEnergy;
    return outDist;
}

void Search::xSetIntraResultChromaQT(CUData* cu, uint32_t trDepth, uint32_t absPartIdx, Yuv* reconYuv)
{
    uint32_t fullDepth = cu->m_depth[0] + trDepth;
    uint32_t trMode    = cu->m_trIdx[absPartIdx];
    int hChromaShift = CHROMA_H_SHIFT(m_csp);
    int vChromaShift = CHROMA_V_SHIFT(m_csp);

    if (trMode == trDepth)
    {
        uint32_t log2TrSize = g_maxLog2CUSize - fullDepth;
        uint32_t log2TrSizeC = log2TrSize - hChromaShift;

        if (log2TrSize == 2 && m_csp != X265_CSP_I444)
        {
            X265_CHECK(trDepth > 0, "invalid trDepth\n");
            trDepth--;
            log2TrSizeC++;
            uint32_t qpdiv = NUM_CU_PARTITIONS >> ((cu->m_depth[0] + trDepth) << 1);
            if (absPartIdx & (qpdiv - 1))
                return;
        }

        // copy transform coefficients

        uint32_t numCoeffC = 1 << (log2TrSizeC * 2 + (m_csp == X265_CSP_I422));
        uint32_t coeffOffsetC = absPartIdx << (LOG2_UNIT_SIZE * 2 - (hChromaShift + vChromaShift));

        uint32_t qtLayer   = log2TrSize - 2;
        coeff_t* coeffSrcU = m_rqt[qtLayer].tmpCoeff[1] + coeffOffsetC;
        coeff_t* coeffSrcV = m_rqt[qtLayer].tmpCoeff[2] + coeffOffsetC;
        coeff_t* coeffDstU = cu->m_trCoeff[1]           + coeffOffsetC;
        coeff_t* coeffDstV = cu->m_trCoeff[2]           + coeffOffsetC;
        memcpy(coeffDstU, coeffSrcU, sizeof(coeff_t) * numCoeffC);
        memcpy(coeffDstV, coeffSrcV, sizeof(coeff_t) * numCoeffC);

        // copy reconstruction
        m_rqt[qtLayer].tmpShortYuv.copyPartToPartChroma(*reconYuv, absPartIdx, log2TrSizeC + hChromaShift);
    }
    else
    {
        uint32_t numQPart = NUM_CU_PARTITIONS >> ((fullDepth + 1) << 1);
        for (uint32_t part = 0; part < 4; part++)
            xSetIntraResultChromaQT(cu, trDepth + 1, absPartIdx + part * numQPart, reconYuv);
    }
}

void Search::residualQTIntraChroma(Mode& mode, const CUGeom& cuGeom, uint32_t trDepth, uint32_t absPartIdx)
{
    CUData* cu = &mode.cu;
    Yuv* predYuv = &mode.predYuv;
    ShortYuv* resiYuv = &mode.resiYuv;
    Yuv* reconYuv = &mode.reconYuv;
    const Yuv* fencYuv = mode.fencYuv;

    uint32_t fullDepth = cu->m_depth[0] + trDepth;
    uint32_t trMode    = cu->m_trIdx[absPartIdx];
    int hChromaShift = CHROMA_H_SHIFT(m_csp);
    int vChromaShift = CHROMA_V_SHIFT(m_csp);
    
    if (trMode == trDepth)
    {
        uint32_t log2TrSize = g_maxLog2CUSize - fullDepth;
        uint32_t log2TrSizeC = log2TrSize - hChromaShift;
        uint32_t trDepthC = trDepth;
        if (log2TrSize == 2 && m_csp != X265_CSP_I444)
        {
            X265_CHECK(trDepth > 0, "invalid trDepth\n");
            trDepthC--;
            log2TrSizeC++;
            uint32_t qpdiv = NUM_CU_PARTITIONS >> ((cu->m_depth[0] + trDepthC) << 1);
            bool bFirstQ = ((absPartIdx & (qpdiv - 1)) == 0);
            if (!bFirstQ)
                return;
        }

        uint32_t tuSize = 1 << log2TrSizeC;
        uint32_t stride = fencYuv->m_csize;
        const bool splitIntoSubTUs = (m_csp == X265_CSP_I422);
        const int sizeIdxC = log2TrSizeC - 2;

        for (uint32_t chromaId = TEXT_CHROMA_U; chromaId <= TEXT_CHROMA_V; chromaId++)
        {
            uint32_t curPartNum = NUM_CU_PARTITIONS >> ((cu->m_depth[0] + trDepthC) << 1);
            TURecurse tuIterator(splitIntoSubTUs ? VERTICAL_SPLIT : DONT_SPLIT, curPartNum, absPartIdx);

            do
            {
                uint32_t absPartIdxC = tuIterator.absPartIdxTURelCU;

                TextType ttype          = (TextType)chromaId;
                pixel*   fenc           = const_cast<pixel*>(fencYuv->getChromaAddr(chromaId, absPartIdxC));
                pixel*   pred           = predYuv->getChromaAddr(chromaId, absPartIdxC);
                int16_t* residual       = resiYuv->getChromaAddr(chromaId, absPartIdxC);
                pixel*   recon          = reconYuv->getChromaAddr(chromaId, absPartIdxC);
                uint32_t coeffOffsetC   = absPartIdxC << (LOG2_UNIT_SIZE * 2 - (hChromaShift + vChromaShift));
                coeff_t* coeff          = cu->m_trCoeff[ttype] + coeffOffsetC;
                uint32_t zorder         = cuGeom.encodeIdx + absPartIdxC;
                pixel*   reconIPred     = m_frame->m_reconPicYuv->getChromaAddr(chromaId, cu->m_cuAddr, zorder);
                uint32_t reconIPredStride = m_frame->m_reconPicYuv->m_strideC;

                const bool useTransformSkipC = !!cu->m_transformSkip[ttype][absPartIdxC];
                cu->setTransformSkipPartRange(0, ttype, absPartIdxC, tuIterator.absPartIdxStep);

                uint32_t chromaPredMode = cu->m_chromaIntraDir[absPartIdxC];

                // update chroma mode
                if (chromaPredMode == DM_CHROMA_IDX)
                    chromaPredMode = cu->m_lumaIntraDir[(m_csp == X265_CSP_I444) ? absPartIdxC : 0];
                chromaPredMode = (m_csp == X265_CSP_I422) ? g_chroma422IntraAngleMappingTable[chromaPredMode] : chromaPredMode;
                initAdiPatternChroma(*cu, cuGeom, absPartIdxC, trDepthC, chromaId);
                pixel* chromaPred = getAdiChromaBuf(chromaId, tuSize);

                predIntraChromaAng(chromaPred, chromaPredMode, pred, stride, log2TrSizeC, m_csp);

                X265_CHECK(!((intptr_t)fenc & (tuSize - 1)), "fenc alignment failure\n");
                X265_CHECK(!((intptr_t)pred & (tuSize - 1)), "pred alignment failure\n");
                X265_CHECK(!((intptr_t)residual & (tuSize - 1)), "residual alignment failure\n");
                primitives.calcresidual[sizeIdxC](fenc, pred, residual, stride);

                uint32_t numSig = m_quant.transformNxN(cu, fenc, stride, residual, stride, coeff, log2TrSizeC, ttype, absPartIdxC, useTransformSkipC);

                cu->setCbfPartRange((!!numSig) << trDepth, ttype, absPartIdxC, tuIterator.absPartIdxStep);

                if (numSig)
                {
                    // inverse transform
                    m_quant.invtransformNxN(cu->m_tqBypass[absPartIdxC], residual, stride, coeff, log2TrSizeC, ttype, true, useTransformSkipC, numSig);

                    // reconstruction
                    primitives.chroma[X265_CSP_I444].add_ps[sizeIdxC](recon, stride, pred, residual, stride, stride);
                    primitives.square_copy_pp[sizeIdxC](reconIPred, reconIPredStride, recon, stride);
                }
                else
                {
#if CHECKED_BUILD || _DEBUG
                    memset(coeff, 0, sizeof(coeff_t) * tuSize * tuSize);
#endif
                    primitives.square_copy_pp[sizeIdxC](recon,      stride,           pred, stride);
                    primitives.square_copy_pp[sizeIdxC](reconIPred, reconIPredStride, pred, stride);
                }
            }
            while (tuIterator.isNextSection());

            if (splitIntoSubTUs)
                offsetSubTUCBFs(cu, (TextType)chromaId, trDepth, absPartIdx);
        }
    }
    else
    {
        uint32_t splitCbfU   = 0;
        uint32_t splitCbfV   = 0;
        uint32_t qPartsDiv   = NUM_CU_PARTITIONS >> ((fullDepth + 1) << 1);
        uint32_t absPartIdxC = absPartIdx;
        for (uint32_t part = 0; part < 4; part++, absPartIdxC += qPartsDiv)
        {
            residualQTIntraChroma(mode, cuGeom, trDepth + 1, absPartIdxC);
            splitCbfU |= cu->getCbf(absPartIdxC, TEXT_CHROMA_U, trDepth + 1);
            splitCbfV |= cu->getCbf(absPartIdxC, TEXT_CHROMA_V, trDepth + 1);
        }

        for (uint32_t offs = 0; offs < 4 * qPartsDiv; offs++)
        {
            cu->m_cbf[1][absPartIdx + offs] |= (splitCbfU << trDepth);
            cu->m_cbf[2][absPartIdx + offs] |= (splitCbfV << trDepth);
        }
    }
}

void Search::checkIntra(Mode& intraMode, const CUGeom& cuGeom, PartSize partSize, uint8_t* sharedModes)
{
    uint32_t depth = cuGeom.depth;
    CUData& cu = intraMode.cu;

    cu.setPartSizeSubParts(partSize);
    cu.setPredModeSubParts(MODE_INTRA);

    uint32_t tuDepthRange[2];
    cu.getQuadtreeTULog2MinSizeInCU(tuDepthRange, 0);

    intraMode.initCosts();
    intraMode.distortion += estIntraPredQT(intraMode, cuGeom, tuDepthRange, sharedModes);
    intraMode.distortion += estIntraPredChromaQT(intraMode, cuGeom);

    m_entropyCoder.resetBits();
    if (m_slice->m_pps->bTransquantBypassEnabled)
        m_entropyCoder.codeCUTransquantBypassFlag(cu.m_tqBypass[0]);

    if (!m_slice->isIntra())
    {
        m_entropyCoder.codeSkipFlag(cu, 0);
        m_entropyCoder.codePredMode(cu.m_predMode[0]);
    }

    m_entropyCoder.codePartSize(cu, 0, depth);
    m_entropyCoder.codePredInfo(cu, 0);
    intraMode.mvBits = m_entropyCoder.getNumberOfWrittenBits();

    bool bCodeDQP = m_slice->m_pps->bUseDQP;
    m_entropyCoder.codeCoeff(cu, 0, depth, bCodeDQP, tuDepthRange);
    m_entropyCoder.store(intraMode.contexts);
    intraMode.totalBits = m_entropyCoder.getNumberOfWrittenBits();
    intraMode.coeffBits = intraMode.totalBits - intraMode.mvBits;
    if (m_rdCost.m_psyRd)
        intraMode.psyEnergy = m_rdCost.psyCost(cuGeom.log2CUSize - 2, intraMode.fencYuv->m_buf[0], intraMode.fencYuv->m_size, intraMode.reconYuv.m_buf[0], intraMode.reconYuv.m_size);

    updateModeCost(intraMode);
}

uint32_t Search::estIntraPredQT(Mode &intraMode, const CUGeom& cuGeom, uint32_t depthRange[2], uint8_t* sharedModes)
{
    CUData* cu = &intraMode.cu;
    Yuv* reconYuv = &intraMode.reconYuv;
    Yuv* predYuv = &intraMode.predYuv;
    const Yuv* fencYuv = intraMode.fencYuv;

    uint32_t depth        = cu->m_depth[0];
    uint32_t initTrDepth  = cu->m_partSize[0] == SIZE_2Nx2N ? 0 : 1;
    uint32_t numPU        = 1 << (2 * initTrDepth);
    uint32_t log2TrSize   = cu->m_log2CUSize[0] - initTrDepth;
    uint32_t tuSize       = 1 << log2TrSize;
    uint32_t qNumParts    = cuGeom.numPartitions >> 2;
    uint32_t sizeIdx      = log2TrSize - 2;
    uint32_t absPartIdx   = 0;
    uint32_t totalDistortion = 0;

    // loop over partitions
    for (uint32_t pu = 0; pu < numPU; pu++, absPartIdx += qNumParts)
    {
        uint32_t bmode = 0;

        if (sharedModes)
            bmode = sharedModes[pu];
        else
        {
            // Reference sample smoothing
            initAdiPattern(*cu, cuGeom, absPartIdx, initTrDepth, ALL_IDX);

            // determine set of modes to be tested (using prediction signal only)
            pixel*   fenc = const_cast<pixel*>(fencYuv->getLumaAddr(absPartIdx));
            uint32_t stride = predYuv->m_size;

            pixel *above = m_refAbove + tuSize - 1;
            pixel *aboveFiltered = m_refAboveFlt + tuSize - 1;
            pixel *left = m_refLeft + tuSize - 1;
            pixel *leftFiltered = m_refLeftFlt + tuSize - 1;

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
                pixel *aboveScale = _above + 2 * 32;
                pixel *leftScale = _left + 2 * 32;

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
                sizeIdx = 5 - 2; // log2(scaleTuSize) - 2

                // Filtered and Unfiltered refAbove and refLeft pointing to above and left.
                above = aboveScale;
                left = leftScale;
                aboveFiltered = aboveScale;
                leftFiltered = leftScale;
            }

            uint32_t preds[3];
            cu->getIntraDirLumaPredictor(absPartIdx, preds);

            uint64_t mpms;
            uint32_t rbits = getIntraRemModeBits(*cu, absPartIdx, depth, preds, mpms);

            pixelcmp_t sa8d = primitives.sa8d[sizeIdx];
            uint64_t modeCosts[35];
            uint64_t bcost;

            // DC
            primitives.intra_pred[DC_IDX][sizeIdx](tmp, scaleStride, left, above, 0, (scaleTuSize <= 16));
            uint32_t bits = (mpms & ((uint64_t)1 << DC_IDX)) ? getIntraModeBits(*cu, DC_IDX, absPartIdx, depth) : rbits;
            uint32_t sad = sa8d(fenc, scaleStride, tmp, scaleStride) << costShift;
            modeCosts[DC_IDX] = bcost = m_rdCost.calcRdSADCost(sad, bits);

            // PLANAR
            pixel *abovePlanar = above;
            pixel *leftPlanar = left;
            if (tuSize >= 8 && tuSize <= 32)
            {
                abovePlanar = aboveFiltered;
                leftPlanar = leftFiltered;
            }
            primitives.intra_pred[PLANAR_IDX][sizeIdx](tmp, scaleStride, leftPlanar, abovePlanar, 0, 0);
            bits = (mpms & ((uint64_t)1 << PLANAR_IDX)) ? getIntraModeBits(*cu, PLANAR_IDX, absPartIdx, depth) : rbits;
            sad = sa8d(fenc, scaleStride, tmp, scaleStride) << costShift;
            modeCosts[PLANAR_IDX] = m_rdCost.calcRdSADCost(sad, bits);
            COPY1_IF_LT(bcost, modeCosts[PLANAR_IDX]);

            // angular predictions
            primitives.intra_pred_allangs[sizeIdx](tmp, above, left, aboveFiltered, leftFiltered, (scaleTuSize <= 16));

            primitives.transpose[sizeIdx](buf_trans, fenc, scaleStride);
            for (int mode = 2; mode < 35; mode++)
            {
                bool modeHor = (mode < 18);
                pixel *cmp = (modeHor ? buf_trans : fenc);
                intptr_t srcStride = (modeHor ? scaleTuSize : scaleStride);
                bits = (mpms & ((uint64_t)1 << mode)) ? getIntraModeBits(*cu, mode, absPartIdx, depth) : rbits;
                sad = sa8d(cmp, srcStride, &tmp[(mode - 2) * (scaleTuSize * scaleTuSize)], scaleTuSize) << costShift;
                modeCosts[mode] = m_rdCost.calcRdSADCost(sad, bits);
                COPY1_IF_LT(bcost, modeCosts[mode]);
            }

            /* Find the top maxCandCount candidate modes with cost within 25% of best
             * or among the most probable modes. maxCandCount is derived from the
             * rdLevel and depth. In general we want to try more modes at slower RD
             * levels and at higher depths */
            uint64_t candCostList[MAX_RD_INTRA_MODES];
            uint32_t rdModeList[MAX_RD_INTRA_MODES];
            int maxCandCount = 2 + m_param->rdLevel + ((depth + initTrDepth) >> 1);
            for (int i = 0; i < maxCandCount; i++)
                candCostList[i] = MAX_INT64;

            uint64_t paddedBcost = bcost + (bcost >> 3); // 1.12%
            for (int mode = 0; mode < 35; mode++)
                if (modeCosts[mode] < paddedBcost || (mpms & ((uint64_t)1 << mode)))
                    updateCandList(mode, modeCosts[mode], maxCandCount, rdModeList, candCostList);

            /* measure best candidates using simple RDO (no TU splits) */
            bcost = MAX_INT64;
            for (int i = 0; i < maxCandCount; i++)
            {
                if (candCostList[i] == MAX_INT64)
                    break;
                m_entropyCoder.load(m_rqt[depth].cur);
                cu->setLumaIntraDirSubParts(rdModeList[i], absPartIdx, depth + initTrDepth);

                Cost icosts;
                codeIntraLumaQT(intraMode, cuGeom, initTrDepth, absPartIdx, false, icosts, depthRange);
                COPY2_IF_LT(bcost, icosts.rdcost, bmode, rdModeList[i]);
            }
        }

        /* remeasure best mode, allowing TU splits */
        cu->setLumaIntraDirSubParts(bmode, absPartIdx, depth + initTrDepth);
        m_entropyCoder.load(m_rqt[depth].cur);

        Cost icosts;
        codeIntraLumaQT(intraMode, cuGeom, initTrDepth, absPartIdx, true, icosts, depthRange);
        totalDistortion += icosts.distortion;

        xSetIntraResultQT(cu, initTrDepth, absPartIdx, reconYuv);

        // set reconstruction for next intra prediction blocks
        if (pu != numPU - 1)
        {
            /* This has important implications for parallelism and RDO.  It is writing intermediate results into the
             * output recon picture, so it cannot proceed in parallel with anything else when doing INTRA_NXN. Also
             * it is not updating m_rdContexts[depth].cur for the later PUs which I suspect is slightly wrong. I think
             * that the contexts should be tracked through each PU */
            pixel*   dst         = m_frame->m_reconPicYuv->getLumaAddr(cu->m_cuAddr, cuGeom.encodeIdx + absPartIdx);
            uint32_t dststride   = m_frame->m_reconPicYuv->m_stride;
            pixel*   src         = reconYuv->getLumaAddr(absPartIdx);
            uint32_t srcstride   = reconYuv->m_size;
            primitives.square_copy_pp[log2TrSize - 2](dst, dststride, src, srcstride);
        }
    }

    if (numPU > 1)
    {
        uint32_t combCbfY = 0;
        uint32_t partIdx  = 0;
        for (uint32_t part = 0; part < 4; part++, partIdx += qNumParts)
            combCbfY |= cu->getCbf(partIdx, TEXT_LUMA, 1);

        for (uint32_t offs = 0; offs < 4 * qNumParts; offs++)
            cu->m_cbf[0][offs] |= combCbfY;
    }

    // TODO: remove these two lines
    m_entropyCoder.load(m_rqt[depth].cur);
    x265_emms();

    return totalDistortion;
}

void Search::getBestIntraModeChroma(Mode& intraMode, const CUGeom& cuGeom)
{
    CUData* cu = &intraMode.cu;
    const Yuv* fencYuv = intraMode.fencYuv;
    Yuv* predYuv = &intraMode.predYuv;

    uint32_t bestMode  = 0;
    uint64_t bestCost  = MAX_INT64;
    uint32_t modeList[NUM_CHROMA_MODE];

    uint32_t log2TrSizeC = cu->m_log2CUSize[0] - CHROMA_H_SHIFT(m_csp);
    uint32_t tuSize = 1 << log2TrSizeC;
    int32_t scaleTuSize = tuSize;
    int32_t costShift = 0;

    if (tuSize > 32)
    {
        scaleTuSize = 32;
        costShift = 2;
        log2TrSizeC = 5;
    }

    Predict::initAdiPatternChroma(*cu, cuGeom, 0, 0, 1);
    Predict::initAdiPatternChroma(*cu, cuGeom, 0, 0, 2);
    cu->getAllowedChromaDir(0, modeList);

    // check chroma modes
    for (uint32_t mode = 0; mode < NUM_CHROMA_MODE; mode++)
    {
        uint32_t chromaPredMode = modeList[mode];
        if (chromaPredMode == DM_CHROMA_IDX)
            chromaPredMode = cu->m_lumaIntraDir[0];
        if (m_csp == X265_CSP_I422)
            chromaPredMode = g_chroma422IntraAngleMappingTable[chromaPredMode];

        uint64_t cost = 0;
        for (uint32_t chromaId = TEXT_CHROMA_U; chromaId <= TEXT_CHROMA_V; chromaId++)
        {
            pixel* fenc = fencYuv->m_buf[chromaId];
            pixel* pred = predYuv->m_buf[chromaId];
            pixel* chromaPred = getAdiChromaBuf(chromaId, scaleTuSize);

            // get prediction signal
            predIntraChromaAng(chromaPred, chromaPredMode, pred, fencYuv->m_csize, log2TrSizeC, m_csp);
            cost += primitives.sa8d[log2TrSizeC - 2](fenc, predYuv->m_csize, pred, fencYuv->m_csize) << costShift;
        }

        if (cost < bestCost)
        {
            bestCost = cost;
            bestMode = modeList[mode];
        }
    }

    cu->setChromIntraDirSubParts(bestMode, 0, cu->m_depth[0]);
}

uint32_t Search::estIntraPredChromaQT(Mode &intraMode, const CUGeom& cuGeom)
{
    CUData* cu = &intraMode.cu;
    Yuv* reconYuv = &intraMode.reconYuv;

    uint32_t depth       = cu->m_depth[0];
    uint32_t initTrDepth = (cu->m_partSize[0] != SIZE_2Nx2N) && (cu->m_chromaFormat == X265_CSP_I444 ? 1 : 0);
    uint32_t log2TrSize  = cu->m_log2CUSize[0] - initTrDepth;
    uint32_t absPartIdx  = (NUM_CU_PARTITIONS >> (depth << 1));
    uint32_t totalDistortion = 0;

    int part = partitionFromLog2Size(log2TrSize);

    TURecurse tuIterator((initTrDepth == 0) ? DONT_SPLIT : QUAD_SPLIT, absPartIdx, 0);

    do
    {
        uint32_t absPartIdxC = tuIterator.absPartIdxTURelCU;
        int cuSize = 1 << cu->m_log2CUSize[absPartIdxC];

        uint32_t bestMode = 0;
        uint32_t bestDist = 0;
        uint64_t bestCost = MAX_INT64;

        // init mode list
        uint32_t minMode = 0;
        uint32_t maxMode = NUM_CHROMA_MODE;
        uint32_t modeList[NUM_CHROMA_MODE];

        cu->getAllowedChromaDir(absPartIdxC, modeList);

        // check chroma modes
        for (uint32_t mode = minMode; mode < maxMode; mode++)
        {
            // restore context models
            m_entropyCoder.load(m_rqt[depth].cur);

            cu->setChromIntraDirSubParts(modeList[mode], absPartIdxC, depth + initTrDepth);
            uint32_t psyEnergy = 0;
            uint32_t dist = codeIntraChromaQt(intraMode, cuGeom, initTrDepth, absPartIdxC, psyEnergy);

            if (m_slice->m_pps->bTransformSkipEnabled)
                m_entropyCoder.load(m_rqt[depth].cur);

            m_entropyCoder.resetBits();
            // chroma prediction mode
            if (cu->m_partSize[0] == SIZE_2Nx2N || cu->m_chromaFormat != X265_CSP_I444)
            {
                if (!absPartIdxC)
                    m_entropyCoder.codeIntraDirChroma(*cu, absPartIdxC, modeList);
            }
            else
            {
                uint32_t qtNumParts = cuGeom.numPartitions >> 2;
                if (!(absPartIdxC & (qtNumParts - 1)))
                    m_entropyCoder.codeIntraDirChroma(*cu, absPartIdxC, modeList);
            }

            xEncSubdivCbfQTChroma(*cu, initTrDepth, absPartIdxC, tuIterator.absPartIdxStep, cuSize, cuSize);
            xEncCoeffQTChroma(*cu, initTrDepth, absPartIdxC, TEXT_CHROMA_U);
            xEncCoeffQTChroma(*cu, initTrDepth, absPartIdxC, TEXT_CHROMA_V);
            uint32_t bits = m_entropyCoder.getNumberOfWrittenBits();
            uint64_t cost = m_rdCost.m_psyRd ? m_rdCost.calcPsyRdCost(dist, bits, psyEnergy) : m_rdCost.calcRdCost(dist, bits);

            if (cost < bestCost)
            {
                bestCost = cost;
                bestDist = dist;
                bestMode = modeList[mode];
                xSetIntraResultChromaQT(cu, initTrDepth, absPartIdxC, reconYuv);
                memcpy(m_qtTempCbf[1], cu->m_cbf[1] + absPartIdxC, tuIterator.absPartIdxStep * sizeof(uint8_t));
                memcpy(m_qtTempCbf[2], cu->m_cbf[2] + absPartIdxC, tuIterator.absPartIdxStep * sizeof(uint8_t));
                memcpy(m_qtTempTransformSkipFlag[1], cu->m_transformSkip[1] + absPartIdxC, tuIterator.absPartIdxStep * sizeof(uint8_t));
                memcpy(m_qtTempTransformSkipFlag[2], cu->m_transformSkip[2] + absPartIdxC, tuIterator.absPartIdxStep * sizeof(uint8_t));
            }
        }

        if (!tuIterator.isLastSection())
        {
            uint32_t zorder    = cuGeom.encodeIdx + absPartIdxC;
            uint32_t dststride = m_frame->m_reconPicYuv->m_strideC;
            pixel *src, *dst;

            dst = m_frame->m_reconPicYuv->getCbAddr(cu->m_cuAddr, zorder);
            src = reconYuv->getCbAddr(absPartIdxC);
            primitives.chroma[m_csp].copy_pp[part](dst, dststride, src, reconYuv->m_csize);

            dst = m_frame->m_reconPicYuv->getCrAddr(cu->m_cuAddr, zorder);
            src = reconYuv->getCrAddr(absPartIdxC);
            primitives.chroma[m_csp].copy_pp[part](dst, dststride, src, reconYuv->m_csize);
        }

        memcpy(cu->m_cbf[1] + absPartIdxC, m_qtTempCbf[1], tuIterator.absPartIdxStep * sizeof(uint8_t));
        memcpy(cu->m_cbf[2] + absPartIdxC, m_qtTempCbf[2], tuIterator.absPartIdxStep * sizeof(uint8_t));
        memcpy(cu->m_transformSkip[1] + absPartIdxC, m_qtTempTransformSkipFlag[1], tuIterator.absPartIdxStep * sizeof(uint8_t));
        memcpy(cu->m_transformSkip[2] + absPartIdxC, m_qtTempTransformSkipFlag[2], tuIterator.absPartIdxStep * sizeof(uint8_t));
        cu->setChromIntraDirSubParts(bestMode, absPartIdxC, depth + initTrDepth);
        totalDistortion += bestDist;
    }
    while (tuIterator.isNextSection());

    if (initTrDepth != 0)
    {
        uint32_t combCbfU = 0;
        uint32_t combCbfV = 0;
        uint32_t partIdx  = 0;
        for (uint32_t p = 0; p < 4; p++, partIdx += tuIterator.absPartIdxStep)
        {
            combCbfU |= cu->getCbf(partIdx, TEXT_CHROMA_U, 1);
            combCbfV |= cu->getCbf(partIdx, TEXT_CHROMA_V, 1);
        }

        for (uint32_t offs = 0; offs < 4 * tuIterator.absPartIdxStep; offs++)
        {
            cu->m_cbf[1][offs] |= combCbfU;
            cu->m_cbf[2][offs] |= combCbfV;
        }
    }

    /* TODO: remove this */
    m_entropyCoder.load(m_rqt[depth].cur);
    return totalDistortion;
}

/* estimation of best merge coding */
uint32_t Search::mergeEstimation(CUData* cu, const CUGeom& cuGeom, int puIdx, MergeData& m)
{
    X265_CHECK(cu->m_partSize[0] != SIZE_2Nx2N, "merge tested on non-2Nx2N partition\n");

    m.maxNumMergeCand = cu->getInterMergeCandidates(m.absPartIdx, puIdx, m.mvFieldNeighbours, m.interDirNeighbours);

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

    Yuv& tempYuv = m_rqt[cuGeom.depth].tmpPredYuv;

    uint32_t outCost = MAX_UINT;
    for (uint32_t mergeCand = 0; mergeCand < m.maxNumMergeCand; ++mergeCand)
    {
        /* Prevent TMVP candidates from using unavailable reference pixels */
        if (m_bFrameParallel &&
            (m.mvFieldNeighbours[mergeCand][0].mv.y >= (m_param->searchRange + 1) * 4 ||
             m.mvFieldNeighbours[mergeCand][1].mv.y >= (m_param->searchRange + 1) * 4))
            continue;

        /* TODO: merge this logic with merge functions in analysis.cpp */
        cu->m_cuMvField[0].mv[m.absPartIdx] = m.mvFieldNeighbours[mergeCand][0].mv;
        cu->m_cuMvField[0].refIdx[m.absPartIdx] = (char)m.mvFieldNeighbours[mergeCand][0].refIdx;
        cu->m_cuMvField[1].mv[m.absPartIdx] = m.mvFieldNeighbours[mergeCand][1].mv;
        cu->m_cuMvField[1].refIdx[m.absPartIdx] = (char)m.mvFieldNeighbours[mergeCand][1].refIdx;

        prepMotionCompensation(cu, cuGeom, puIdx);
        motionCompensation(&tempYuv, true, false);
        uint32_t costCand = m_me.bufSATD(tempYuv.getLumaAddr(m.absPartIdx), tempYuv.m_size);
        uint32_t bitsCand = getTUBits(mergeCand, m.maxNumMergeCand);
        costCand = costCand + m_rdCost.getCost(bitsCand);
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

void Search::singleMotionEstimation(Search& master, const CUData& cu, const CUGeom& cuGeom, int part, int list, int ref)
{
    PicYuv*  fencPic = m_frame->m_origPicYuv;

    uint32_t partAddr;
    int      puWidth, puHeight;
    cu.getPartIndexAndSize(part, partAddr, puWidth, puHeight);
    prepMotionCompensation(&cu, cuGeom, part);

    pixel* pu = fencPic->getLumaAddr(cu.m_cuAddr, cuGeom.encodeIdx + partAddr);
    m_me.setSourcePU(pu - fencPic->m_picOrg[0], puWidth, puHeight);

    uint32_t bits = master.m_listSelBits[list] + MVP_IDX_BITS;
    bits += getTUBits(ref, m_slice->m_numRefIdx[list]);

    MV amvpCand[AMVP_NUM_CANDS];
    MV mvc[(MD_ABOVE_LEFT + 1) * 2 + 1];
    int numMvc = cu.fillMvpCand(part, partAddr, list, ref, amvpCand, mvc);

    Yuv& tmpPredYuv = m_rqt[cuGeom.depth].tmpPredYuv;

    uint32_t bestCost = MAX_INT;
    int mvpIdx = 0;
    int merange = m_param->searchRange;
    for (int i = 0; i < AMVP_NUM_CANDS; i++)
    {
        MV mvCand = amvpCand[i];

        // NOTE: skip mvCand if Y is > merange and -FN>1
        if (m_bFrameParallel && (mvCand.y >= (merange + 1) * 4))
            continue;

        cu.clipMv(mvCand);

        predInterLumaBlk(m_slice->m_refPicList[list][ref]->m_reconPicYuv, &tmpPredYuv, &mvCand);
        uint32_t cost = m_me.bufSAD(tmpPredYuv.getLumaAddr(partAddr), tmpPredYuv.m_size);

        if (bestCost > cost)
        {
            bestCost = cost;
            mvpIdx = i;
        }
    }

    MV mvmin, mvmax, outmv, mvp = amvpCand[mvpIdx];
    setSearchRange(cu, mvp, merange, mvmin, mvmax);

    int satdCost = m_me.motionEstimate(&m_slice->m_mref[list][ref], mvmin, mvmax, mvp, numMvc, mvc, merange, outmv);

    /* Get total cost of partition, but only include MV bit cost once */
    bits += m_me.bitcost(outmv);
    uint32_t cost = (satdCost - m_me.mvcost(outmv)) + m_rdCost.getCost(bits);

    /* Refine MVP selection, updates: mvp, mvpIdx, bits, cost */
    checkBestMVP(amvpCand, outmv, mvp, mvpIdx, bits, cost);

    /* tie goes to the smallest ref ID, just like --no-pme */
    ScopedLock _lock(master.m_outputLock);
    if (cost < master.m_bestME[list].cost ||
       (cost == master.m_bestME[list].cost && ref < master.m_bestME[list].ref))
    {
        master.m_bestME[list].mv = outmv;
        master.m_bestME[list].mvp = mvp;
        master.m_bestME[list].mvpIdx = mvpIdx;
        master.m_bestME[list].ref = ref;
        master.m_bestME[list].cost = cost;
        master.m_bestME[list].bits = bits;
    }
}

void Search::parallelInterSearch(Mode& interMode, const CUGeom& cuGeom, bool bChroma)
{
    CUData* cu = &interMode.cu;
    const Slice *slice = m_slice;
    PicYuv *fencPic = slice->m_frame->m_origPicYuv;
    PartSize partSize = (PartSize)cu->m_partSize[0];
    m_curMECu = cu;
    m_curGeom = &cuGeom;

    MergeData merge;
    memset(&merge, 0, sizeof(merge));

    uint32_t lastMode = 0;
    int numPart = cu->getNumPartInter();

    for (int puIdx = 0; puIdx < numPart; puIdx++)
    {
        uint32_t absPartIdx;
        int      puWidth, puHeight;
        cu->getPartIndexAndSize(puIdx, absPartIdx, puWidth, puHeight);

        getBlkBits(partSize, slice->isInterP(), puIdx, lastMode, m_listSelBits);
        prepMotionCompensation(cu, cuGeom, puIdx);

        pixel* pu = fencPic->getLumaAddr(cu->m_cuAddr, cuGeom.encodeIdx + absPartIdx);
        m_me.setSourcePU(pu - fencPic->m_picOrg[0], puWidth, puHeight);

        m_bestME[0].cost = MAX_UINT;
        m_bestME[1].cost = MAX_UINT;

        /* this worker might already be enqueued, so other threads might be looking at the ME job counts
        * at any time, do these sets in a safe order */
        m_curPart = puIdx;
        m_totalNumME = 0;
        m_numAcquiredME = 0;
        m_numCompletedME = 0;
        m_totalNumME = slice->m_numRefIdx[0] + slice->m_numRefIdx[1];

        if (!m_bJobsQueued)
            JobProvider::enqueue();

        for (int i = 0; i < m_totalNumME; i++)
            m_pool->pokeIdleThread();

        MotionData bidir[2];
        uint32_t mrgCost = MAX_UINT;
        uint32_t bidirCost = MAX_UINT;
        int bidirBits = 0;

        /* the master thread does merge estimation */
        if (partSize != SIZE_2Nx2N)
        {
            merge.absPartIdx = absPartIdx;
            merge.width = puWidth;
            merge.height = puHeight;
            mrgCost = mergeEstimation(cu, cuGeom, puIdx, merge);
        }

        /* Participate in unidir motion searches */
        while (m_totalNumME > m_numAcquiredME)
        {
            int id = ATOMIC_INC(&m_numAcquiredME);
            if (m_totalNumME >= id)
            {
                id -= 1;
                if (id < m_slice->m_numRefIdx[0])
                    singleMotionEstimation(*this, *cu, cuGeom, puIdx, 0, id);
                else
                    singleMotionEstimation(*this, *cu, cuGeom, puIdx, 1, id - m_slice->m_numRefIdx[0]);

                if (ATOMIC_INC(&m_numCompletedME) == m_totalNumME)
                    m_meCompletionEvent.trigger();
            }
        }
        if (!m_bJobsQueued)
            JobProvider::dequeue();
        m_meCompletionEvent.wait();

        /* the master thread does bidir estimation */
        if (slice->isInterB() && !cu->isBipredRestriction() && m_bestME[0].cost != MAX_UINT && m_bestME[1].cost != MAX_UINT)
        {
            ALIGN_VAR_32(pixel, avg[MAX_CU_SIZE * MAX_CU_SIZE]);

            bidir[0] = m_bestME[0];
            bidir[1] = m_bestME[1];

            // Generate reference subpels
            PicYuv *refPic0 = slice->m_refPicList[0][m_bestME[0].ref]->m_reconPicYuv;
            PicYuv *refPic1 = slice->m_refPicList[1][m_bestME[1].ref]->m_reconPicYuv;

            Yuv* bidirYuv = m_rqt[cuGeom.depth].bidirPredYuv;

            prepMotionCompensation(cu, cuGeom, puIdx);
            predInterLumaBlk(refPic0, &bidirYuv[0], &m_bestME[0].mv);
            predInterLumaBlk(refPic1, &bidirYuv[1], &m_bestME[1].mv);

            pixel *pred0 = bidirYuv[0].getLumaAddr(absPartIdx);
            pixel *pred1 = bidirYuv[1].getLumaAddr(absPartIdx);

            int partEnum = partitionFromSizes(puWidth, puHeight);
            primitives.pixelavg_pp[partEnum](avg, puWidth, pred0, bidirYuv[0].m_size, pred1, bidirYuv[1].m_size, 32);
            int satdCost = m_me.bufSATD(avg, puWidth);

            bidirBits = m_bestME[0].bits + m_bestME[1].bits + m_listSelBits[2] - (m_listSelBits[0] + m_listSelBits[1]);
            bidirCost = satdCost + m_rdCost.getCost(bidirBits);

            MV mvzero(0, 0);
            bool bTryZero = m_bestME[0].mv.notZero() || m_bestME[1].mv.notZero();
            if (bTryZero)
            {
                /* Do not try zero MV if unidir motion predictors are beyond
                * valid search area */
                MV mvmin, mvmax;
                int merange = X265_MAX(m_param->sourceWidth, m_param->sourceHeight);
                setSearchRange(*cu, mvzero, merange, mvmin, mvmax);
                mvmax.y += 2; // there is some pad for subpel refine
                mvmin <<= 2;
                mvmax <<= 2;

                bTryZero &= m_bestME[0].mvp.checkRange(mvmin, mvmax);
                bTryZero &= m_bestME[1].mvp.checkRange(mvmin, mvmax);
            }
            if (bTryZero)
            {
                // coincident blocks of the two reference pictures
                pixel *ref0 = slice->m_mref[0][m_bestME[0].ref].fpelPlane + (pu - fencPic->m_picOrg[0]);
                pixel *ref1 = slice->m_mref[1][m_bestME[1].ref].fpelPlane + (pu - fencPic->m_picOrg[0]);
                intptr_t refStride = slice->m_mref[0][0].lumaStride;

                primitives.pixelavg_pp[partEnum](avg, puWidth, ref0, refStride, ref1, refStride, 32);
                satdCost = m_me.bufSATD(avg, puWidth);

                MV mvp0 = m_bestME[0].mvp;
                int mvpIdx0 = m_bestME[0].mvpIdx;
                uint32_t bits0 = m_bestME[0].bits - m_me.bitcost(m_bestME[0].mv, mvp0) + m_me.bitcost(mvzero, mvp0);

                MV mvp1 = m_bestME[1].mvp;
                int mvpIdx1 = m_bestME[1].mvpIdx;
                uint32_t bits1 = m_bestME[1].bits - m_me.bitcost(m_bestME[1].mv, mvp1) + m_me.bitcost(mvzero, mvp1);

                uint32_t cost = satdCost + m_rdCost.getCost(bits0) + m_rdCost.getCost(bits1);

                MV amvpCand[AMVP_NUM_CANDS];
                MV mvc[(MD_ABOVE_LEFT + 1) * 2 + 1];
                cu->fillMvpCand(puIdx, absPartIdx, 0, m_bestME[0].ref, amvpCand, mvc);
                checkBestMVP(amvpCand, mvzero, mvp0, mvpIdx0, bits0, cost);

                cu->fillMvpCand(puIdx, absPartIdx, 1, m_bestME[1].ref, amvpCand, mvc);
                checkBestMVP(amvpCand, mvzero, mvp1, mvpIdx1, bits1, cost);

                if (cost < bidirCost)
                {
                    bidir[0].mv = mvzero;
                    bidir[1].mv = mvzero;
                    bidir[0].mvp = mvp0;
                    bidir[1].mvp = mvp1;
                    bidir[0].mvpIdx = mvpIdx0;
                    bidir[1].mvpIdx = mvpIdx1;
                    bidirCost = cost;
                    bidirBits = bits0 + bits1 + m_listSelBits[2] - (m_listSelBits[0] + m_listSelBits[1]);
                }
            }
        }

        /* select best option and store into CU */
        cu->m_cuMvField[REF_PIC_LIST_0].setAllMvField(TComMvField(), partSize, absPartIdx, 0, puIdx);
        cu->m_cuMvField[REF_PIC_LIST_1].setAllMvField(TComMvField(), partSize, absPartIdx, 0, puIdx);

        if (mrgCost < bidirCost && mrgCost < m_bestME[0].cost && mrgCost < m_bestME[1].cost)
        {
            cu->m_mergeFlag[absPartIdx] = true;
            cu->m_mvpIdx[REF_PIC_LIST_0][absPartIdx] = merge.index; // merge candidate ID is stored in L0 MVP idx
            cu->setInterDirSubParts(merge.interDir, absPartIdx, puIdx, cu->m_depth[absPartIdx]);
            cu->m_cuMvField[REF_PIC_LIST_0].setAllMvField(merge.mvField[0], partSize, absPartIdx, 0, puIdx);
            cu->m_cuMvField[REF_PIC_LIST_1].setAllMvField(merge.mvField[1], partSize, absPartIdx, 0, puIdx);

            interMode.sa8dBits += merge.bits;
        }
        else if (bidirCost < m_bestME[0].cost && bidirCost < m_bestME[1].cost)
        {
            lastMode = 2;

            cu->m_mergeFlag[absPartIdx] = false;
            cu->setInterDirSubParts(3, absPartIdx, puIdx, cu->m_depth[0]);
            cu->m_cuMvField[REF_PIC_LIST_0].setAllMv(bidir[0].mv, partSize, absPartIdx, 0, puIdx);
            cu->m_cuMvField[REF_PIC_LIST_0].setAllRefIdx(m_bestME[0].ref, partSize, absPartIdx, 0, puIdx);
            cu->m_cuMvField[REF_PIC_LIST_0].mvd[absPartIdx] = bidir[0].mv - bidir[0].mvp;
            cu->m_mvpIdx[REF_PIC_LIST_0][absPartIdx] = bidir[0].mvpIdx;

            cu->m_cuMvField[REF_PIC_LIST_1].setAllMv(bidir[1].mv, partSize, absPartIdx, 0, puIdx);
            cu->m_cuMvField[REF_PIC_LIST_1].setAllRefIdx(m_bestME[1].ref, partSize, absPartIdx, 0, puIdx);
            cu->m_cuMvField[REF_PIC_LIST_1].mvd[absPartIdx] = bidir[1].mv - bidir[1].mvp;
            cu->m_mvpIdx[REF_PIC_LIST_1][absPartIdx] = bidir[1].mvpIdx;

            interMode.sa8dBits += bidirBits;
        }
        else if (m_bestME[0].cost <= m_bestME[1].cost)
        {
            lastMode = 0;

            cu->m_mergeFlag[absPartIdx] = false;
            cu->setInterDirSubParts(1, absPartIdx, puIdx, cu->m_depth[0]);
            cu->m_cuMvField[REF_PIC_LIST_0].setAllMv(m_bestME[0].mv, partSize, absPartIdx, 0, puIdx);
            cu->m_cuMvField[REF_PIC_LIST_0].setAllRefIdx(m_bestME[0].ref, partSize, absPartIdx, 0, puIdx);
            cu->m_cuMvField[REF_PIC_LIST_0].mvd[absPartIdx] = m_bestME[0].mv - m_bestME[0].mvp;
            cu->m_mvpIdx[REF_PIC_LIST_0][absPartIdx] = m_bestME[0].mvpIdx;

            interMode.sa8dBits += m_bestME[0].bits;
        }
        else
        {
            lastMode = 1;

            cu->m_mergeFlag[absPartIdx] = false;
            cu->setInterDirSubParts(2, absPartIdx, puIdx, cu->m_depth[0]);
            cu->m_cuMvField[REF_PIC_LIST_1].setAllMv(m_bestME[1].mv, partSize, absPartIdx, 0, puIdx);
            cu->m_cuMvField[REF_PIC_LIST_1].setAllRefIdx(m_bestME[1].ref, partSize, absPartIdx, 0, puIdx);
            cu->m_cuMvField[REF_PIC_LIST_1].mvd[absPartIdx] = m_bestME[1].mv - m_bestME[1].mvp;
            cu->m_mvpIdx[REF_PIC_LIST_1][absPartIdx] = m_bestME[1].mvpIdx;

            interMode.sa8dBits += m_bestME[1].bits;
        }

        prepMotionCompensation(cu, cuGeom, puIdx);
        motionCompensation(&interMode.predYuv, true, bChroma);

        if (partSize == SIZE_2Nx2N)
            return;
    }
}

/* search of the best candidate for inter prediction
 * returns true if predYuv was filled with a motion compensated prediction */
bool Search::predInterSearch(Mode& interMode, const CUGeom& cuGeom, bool bMergeOnly, bool bChroma)
{
    CUData* cu = &interMode.cu;
    Yuv* predYuv = &interMode.predYuv;

    MV amvpCand[2][MAX_NUM_REF][AMVP_NUM_CANDS];
    MV mvc[(MD_ABOVE_LEFT + 1) * 2 + 1];

    const Slice *slice  = m_slice;
    PicYuv *fencPic     = slice->m_frame->m_origPicYuv;
    PartSize partSize   = (PartSize)cu->m_partSize[0];
    int      numPart    = cu->getNumPartInter();
    int      numPredDir = slice->isInterP() ? 1 : 2;
    uint32_t lastMode = 0;
    int      totalmebits = 0;

    const int* numRefIdx = slice->m_numRefIdx;

    MergeData merge;

    memset(&merge, 0, sizeof(merge));

    for (int puIdx = 0; puIdx < numPart; puIdx++)
    {
        uint32_t absPartIdx;
        int      puWidth, puHeight;
        cu->getPartIndexAndSize(puIdx, absPartIdx, puWidth, puHeight);

        prepMotionCompensation(cu, cuGeom, puIdx);

        pixel* pu = fencPic->getLumaAddr(cu->m_cuAddr, cuGeom.encodeIdx + absPartIdx);
        m_me.setSourcePU(pu - fencPic->m_picOrg[0], puWidth, puHeight);

        uint32_t mrgCost = MAX_UINT;

        /* find best cost merge candidate */
        if (cu->m_partSize[absPartIdx] != SIZE_2Nx2N)
        {
            merge.absPartIdx = absPartIdx;
            merge.width = puWidth;
            merge.height = puHeight;
            mrgCost = mergeEstimation(cu, cuGeom, puIdx, merge);

            if (bMergeOnly && cu->m_log2CUSize[0] > 3)
            {
                if (mrgCost == MAX_UINT)
                {
                    /* No valid merge modes were found, there is no possible way to
                     * perform a valid motion compensation prediction, so early-exit */
                    return false;
                }
                // set merge result
                cu->m_mergeFlag[absPartIdx] = true;
                cu->m_mvpIdx[REF_PIC_LIST_0][absPartIdx] = merge.index; // merge candidate ID is stored in L0 MVP idx
                cu->setInterDirSubParts(merge.interDir, absPartIdx, puIdx, cu->m_depth[absPartIdx]);
                cu->m_cuMvField[REF_PIC_LIST_0].setAllMvField(merge.mvField[0], partSize, absPartIdx, 0, puIdx);
                cu->m_cuMvField[REF_PIC_LIST_1].setAllMvField(merge.mvField[1], partSize, absPartIdx, 0, puIdx);
                totalmebits += merge.bits;

                prepMotionCompensation(cu, cuGeom, puIdx);
                motionCompensation(predYuv, true, bChroma);
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

        getBlkBits(partSize, slice->isInterP(), puIdx, lastMode, listSelBits);

        // Uni-directional prediction
        for (int l = 0; l < numPredDir; l++)
        {
            for (int ref = 0; ref < numRefIdx[l]; ref++)
            {
                uint32_t bits = listSelBits[l] + MVP_IDX_BITS;
                bits += getTUBits(ref, numRefIdx[l]);

                int numMvc = cu->fillMvpCand(puIdx, absPartIdx, l, ref, amvpCand[l][ref], mvc);

                // Pick the best possible MVP from AMVP candidates based on least residual
                uint32_t bestCost = MAX_INT;
                int mvpIdx = 0;
                int merange = m_param->searchRange;

                Yuv& tmpPredYuv = m_rqt[cuGeom.depth].tmpPredYuv;

                for (int i = 0; i < AMVP_NUM_CANDS; i++)
                {
                    MV mvCand = amvpCand[l][ref][i];

                    // NOTE: skip mvCand if Y is > merange and -FN>1
                    if (m_bFrameParallel && (mvCand.y >= (merange + 1) * 4))
                        continue;

                    cu->clipMv(mvCand);

                    predInterLumaBlk(slice->m_refPicList[l][ref]->m_reconPicYuv, &tmpPredYuv, &mvCand);
                    uint32_t cost = m_me.bufSAD(tmpPredYuv.getLumaAddr(absPartIdx), tmpPredYuv.m_size);

                    if (bestCost > cost)
                    {
                        bestCost = cost;
                        mvpIdx  = i;
                    }
                }

                MV mvmin, mvmax, outmv, mvp = amvpCand[l][ref][mvpIdx];

                setSearchRange(*cu, mvp, merange, mvmin, mvmax);
                int satdCost = m_me.motionEstimate(&slice->m_mref[l][ref], mvmin, mvmax, mvp, numMvc, mvc, merange, outmv);

                /* Get total cost of partition, but only include MV bit cost once */
                bits += m_me.bitcost(outmv);
                uint32_t cost = (satdCost - m_me.mvcost(outmv)) + m_rdCost.getCost(bits);

                /* Refine MVP selection, updates: mvp, mvpIdx, bits, cost */
                checkBestMVP(amvpCand[l][ref], outmv, mvp, mvpIdx, bits, cost);

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
        if (slice->isInterB() && !cu->isBipredRestriction() && list[0].cost != MAX_UINT && list[1].cost != MAX_UINT)
        {
            ALIGN_VAR_32(pixel, avg[MAX_CU_SIZE * MAX_CU_SIZE]);

            bidir[0] = list[0];
            bidir[1] = list[1];

            // Generate reference subpels
            PicYuv *refPic0 = slice->m_refPicList[0][list[0].ref]->m_reconPicYuv;
            PicYuv *refPic1 = slice->m_refPicList[1][list[1].ref]->m_reconPicYuv;
            
            Yuv* bidirYuv = m_rqt[cuGeom.depth].bidirPredYuv;

            predInterLumaBlk(refPic0, &bidirYuv[0], &list[0].mv);
            predInterLumaBlk(refPic1, &bidirYuv[1], &list[1].mv);

            pixel *pred0 = bidirYuv[0].getLumaAddr(absPartIdx);
            pixel *pred1 = bidirYuv[1].getLumaAddr(absPartIdx);

            int partEnum = partitionFromSizes(puWidth, puHeight);
            primitives.pixelavg_pp[partEnum](avg, puWidth, pred0, bidirYuv[0].m_size, pred1, bidirYuv[1].m_size, 32);
            int satdCost = m_me.bufSATD(avg, puWidth);

            bidirBits = list[0].bits + list[1].bits + listSelBits[2] - (listSelBits[0] + listSelBits[1]);
            bidirCost = satdCost + m_rdCost.getCost(bidirBits);

            MV mvzero(0, 0);
            bool bTryZero = list[0].mv.notZero() || list[1].mv.notZero();
            if (bTryZero)
            {
                /* Do not try zero MV if unidir motion predictors are beyond
                 * valid search area */
                MV mvmin, mvmax;
                int merange = X265_MAX(m_param->sourceWidth, m_param->sourceHeight);
                setSearchRange(*cu, mvzero, merange, mvmin, mvmax);
                mvmax.y += 2; // there is some pad for subpel refine
                mvmin <<= 2;
                mvmax <<= 2;

                bTryZero &= list[0].mvp.checkRange(mvmin, mvmax);
                bTryZero &= list[1].mvp.checkRange(mvmin, mvmax);
            }
            if (bTryZero)
            {
                // coincident blocks of the two reference pictures
                pixel *ref0 = slice->m_mref[0][list[0].ref].fpelPlane + (pu - fencPic->m_picOrg[0]);
                pixel *ref1 = slice->m_mref[1][list[1].ref].fpelPlane + (pu - fencPic->m_picOrg[0]);
                intptr_t refStride = slice->m_mref[0][0].lumaStride;

                primitives.pixelavg_pp[partEnum](avg, puWidth, ref0, refStride, ref1, refStride, 32);
                satdCost = m_me.bufSATD(avg, puWidth);

                MV mvp0 = list[0].mvp;
                int mvpIdx0 = list[0].mvpIdx;
                uint32_t bits0 = list[0].bits - m_me.bitcost(list[0].mv, mvp0) + m_me.bitcost(mvzero, mvp0);

                MV mvp1 = list[1].mvp;
                int mvpIdx1 = list[1].mvpIdx;
                uint32_t bits1 = list[1].bits - m_me.bitcost(list[1].mv, mvp1) + m_me.bitcost(mvzero, mvp1);

                uint32_t cost = satdCost + m_rdCost.getCost(bits0) + m_rdCost.getCost(bits1);

                /* refine MVP selection for zero mv, updates: mvp, mvpidx, bits, cost */
                checkBestMVP(amvpCand[0][list[0].ref], mvzero, mvp0, mvpIdx0, bits0, cost);
                checkBestMVP(amvpCand[1][list[1].ref], mvzero, mvp1, mvpIdx1, bits1, cost);

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
        cu->m_cuMvField[REF_PIC_LIST_0].setAllMvField(TComMvField(), partSize, absPartIdx, 0, puIdx);
        cu->m_cuMvField[REF_PIC_LIST_1].setAllMvField(TComMvField(), partSize, absPartIdx, 0, puIdx);

        if (mrgCost < bidirCost && mrgCost < list[0].cost && mrgCost < list[1].cost)
        {
            cu->m_mergeFlag[absPartIdx] = true;
            cu->m_mvpIdx[REF_PIC_LIST_0][absPartIdx] = merge.index; // merge candidate ID is stored in L0 MVP idx
            cu->setInterDirSubParts(merge.interDir, absPartIdx, puIdx, cu->m_depth[absPartIdx]);
            cu->m_cuMvField[REF_PIC_LIST_0].setAllMvField(merge.mvField[0], partSize, absPartIdx, 0, puIdx);
            cu->m_cuMvField[REF_PIC_LIST_1].setAllMvField(merge.mvField[1], partSize, absPartIdx, 0, puIdx);

            totalmebits += merge.bits;
        }
        else if (bidirCost < list[0].cost && bidirCost < list[1].cost)
        {
            lastMode = 2;

            cu->m_mergeFlag[absPartIdx] = false;
            cu->setInterDirSubParts(3, absPartIdx, puIdx, cu->m_depth[0]);
            cu->m_cuMvField[REF_PIC_LIST_0].setAllMv(bidir[0].mv, partSize, absPartIdx, 0, puIdx);
            cu->m_cuMvField[REF_PIC_LIST_0].setAllRefIdx(list[0].ref, partSize, absPartIdx, 0, puIdx);
            cu->m_cuMvField[REF_PIC_LIST_0].mvd[absPartIdx] = bidir[0].mv - bidir[0].mvp;
            cu->m_mvpIdx[REF_PIC_LIST_0][absPartIdx] = bidir[0].mvpIdx;

            cu->m_cuMvField[REF_PIC_LIST_1].setAllMv(bidir[1].mv, partSize, absPartIdx, 0, puIdx);
            cu->m_cuMvField[REF_PIC_LIST_1].setAllRefIdx(list[1].ref, partSize, absPartIdx, 0, puIdx);
            cu->m_cuMvField[REF_PIC_LIST_1].mvd[absPartIdx] = bidir[1].mv - bidir[1].mvp;
            cu->m_mvpIdx[REF_PIC_LIST_1][absPartIdx] = bidir[1].mvpIdx;

            totalmebits += bidirBits;
        }
        else if (list[0].cost <= list[1].cost)
        {
            lastMode = 0;

            cu->m_mergeFlag[absPartIdx] = false;
            cu->setInterDirSubParts(1, absPartIdx, puIdx, cu->m_depth[0]);
            cu->m_cuMvField[REF_PIC_LIST_0].setAllMv(list[0].mv, partSize, absPartIdx, 0, puIdx);
            cu->m_cuMvField[REF_PIC_LIST_0].setAllRefIdx(list[0].ref, partSize, absPartIdx, 0, puIdx);
            cu->m_cuMvField[REF_PIC_LIST_0].mvd[absPartIdx] = list[0].mv - list[0].mvp;
            cu->m_mvpIdx[REF_PIC_LIST_0][absPartIdx] = list[0].mvpIdx;

            totalmebits += list[0].bits;
        }
        else
        {
            lastMode = 1;

            cu->m_mergeFlag[absPartIdx] = false;
            cu->setInterDirSubParts(2, absPartIdx, puIdx, cu->m_depth[0]);
            cu->m_cuMvField[REF_PIC_LIST_1].setAllMv(list[1].mv, partSize, absPartIdx, 0, puIdx);
            cu->m_cuMvField[REF_PIC_LIST_1].setAllRefIdx(list[1].ref, partSize, absPartIdx, 0, puIdx);
            cu->m_cuMvField[REF_PIC_LIST_1].mvd[absPartIdx] = list[1].mv - list[1].mvp;
            cu->m_mvpIdx[REF_PIC_LIST_1][absPartIdx] = list[1].mvpIdx;

            totalmebits += list[1].bits;
        }

        prepMotionCompensation(cu, cuGeom, puIdx);
        motionCompensation(predYuv, true, bChroma);
    }

    x265_emms();
    interMode.sa8dBits += totalmebits;
    return true;
}

void Search::getBlkBits(PartSize cuMode, bool bPSlice, int partIdx, uint32_t lastMode, uint32_t blockBit[3])
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
            ::memcpy(blockBit, listBits[partIdx][lastMode], 3 * sizeof(uint32_t));
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
            ::memcpy(blockBit, listBits[partIdx][lastMode], 3 * sizeof(uint32_t));
    }
    else if (cuMode == SIZE_NxN)
    {
        blockBit[0] = (!bPSlice) ? 3 : 1;
        blockBit[1] = 3;
        blockBit[2] = 5;
    }
    else
    {
        X265_CHECK(0, "getBlkBits: unknown cuMode\n");
    }
}

/* Check if using an alternative MVP would result in a smaller MVD + signal bits */
void Search::checkBestMVP(MV* amvpCand, MV mv, MV& mvPred, int& outMvpIdx, uint32_t& outBits, uint32_t& outCost) const
{
    X265_CHECK(amvpCand[outMvpIdx] == mvPred, "checkBestMVP: unexpected mvPred\n");

    int mvpIdx = !outMvpIdx;
    MV mvp = amvpCand[mvpIdx];
    int diffBits = m_me.bitcost(mv, mvp) - m_me.bitcost(mv, mvPred);
    if (diffBits < 0)
    {
        outMvpIdx = mvpIdx;
        mvPred = mvp;
        uint32_t origOutBits = outBits;
        outBits = origOutBits + diffBits;
        outCost = (outCost - m_rdCost.getCost(origOutBits)) + m_rdCost.getCost(outBits);
    }
}

void Search::setSearchRange(const CUData& cu, MV mvp, int merange, MV& mvmin, MV& mvmax) const
{
    cu.clipMv(mvp);

    MV dist((int16_t)merange << 2, (int16_t)merange << 2);
    mvmin = mvp - dist;
    mvmax = mvp + dist;

    cu.clipMv(mvmin);
    cu.clipMv(mvmax);

    /* Clip search range to signaled maximum MV length.
     * We do not support this VUI field being changed from the default */
    const int maxMvLen = (1 << 15) - 1;
    mvmin.x = X265_MAX(mvmin.x, -maxMvLen);
    mvmin.y = X265_MAX(mvmin.y, -maxMvLen);
    mvmax.x = X265_MIN(mvmax.x, maxMvLen);
    mvmax.y = X265_MIN(mvmax.y, maxMvLen);

    mvmin >>= 2;
    mvmax >>= 2;

    /* conditional clipping for frame parallelism */
    mvmin.y = X265_MIN(mvmin.y, (int16_t)m_refLagPixels);
    mvmax.y = X265_MIN(mvmax.y, (int16_t)m_refLagPixels);
}

/* Note: this function overwrites the RD cost variables of interMode, but leaves the sa8d cost unharmed */
void Search::encodeResAndCalcRdSkipCU(Mode& interMode)
{
    CUData* cu = &interMode.cu;
    Yuv* reconYuv = &interMode.reconYuv;
    const Yuv* fencYuv = interMode.fencYuv;

    X265_CHECK(!cu->isIntra(0), "intra CU not expected\n");

    uint32_t cuSize = 1 << cu->m_log2CUSize[0];
    uint32_t depth  = cu->m_depth[0];

    int hChromaShift = CHROMA_H_SHIFT(m_csp);
    int vChromaShift = CHROMA_V_SHIFT(m_csp);

    // No residual coding : SKIP mode

    cu->setSkipFlagSubParts(true);
    cu->setTrIdxSubParts(0, 0, depth);
    cu->clearCbf(0, depth);

    reconYuv->copyFromYuv(interMode.predYuv);

    // Luma
    int part = partitionFromLog2Size(cu->m_log2CUSize[0]);
    interMode.distortion = primitives.sse_pp[part](fencYuv->m_buf[0], fencYuv->m_size, reconYuv->m_buf[0], reconYuv->m_size);
    // Chroma
    part = partitionFromSizes(cuSize >> hChromaShift, cuSize >> vChromaShift);
    interMode.distortion += m_rdCost.scaleChromaDistCb(primitives.sse_pp[part](fencYuv->m_buf[1], fencYuv->m_csize, reconYuv->m_buf[1], reconYuv->m_csize));
    interMode.distortion += m_rdCost.scaleChromaDistCr(primitives.sse_pp[part](fencYuv->m_buf[2], fencYuv->m_csize, reconYuv->m_buf[2], reconYuv->m_csize));

    m_entropyCoder.load(m_rqt[depth].cur);
    m_entropyCoder.resetBits();
    if (m_slice->m_pps->bTransquantBypassEnabled)
        m_entropyCoder.codeCUTransquantBypassFlag(cu->m_tqBypass[0]);
    m_entropyCoder.codeSkipFlag(*cu, 0);
    m_entropyCoder.codeMergeIndex(*cu, 0);

    interMode.mvBits = m_entropyCoder.getNumberOfWrittenBits();
    interMode.coeffBits = 0;
    interMode.totalBits = interMode.mvBits;
    if (m_rdCost.m_psyRd)
        interMode.psyEnergy = m_rdCost.psyCost(cu->m_log2CUSize[0] - 2, fencYuv->m_buf[0], fencYuv->m_size, reconYuv->m_buf[0], reconYuv->m_size);

    updateModeCost(interMode);
    m_entropyCoder.store(interMode.contexts);
}

/* encode residual and calculate rate-distortion for a CU block.
 * Note: this function overwrites the RD cost variables of interMode, but leaves the sa8d cost unharmed */
void Search::encodeResAndCalcRdInterCU(Mode& interMode, const CUGeom& cuGeom)
{
    CUData* cu = &interMode.cu;
    Yuv* reconYuv = &interMode.reconYuv;
    Yuv* predYuv = &interMode.predYuv;
    ShortYuv* resiYuv = &interMode.resiYuv;
    const Yuv* fencYuv = interMode.fencYuv;

    X265_CHECK(!cu->isIntra(0), "intra CU not expected\n");

    uint32_t log2CUSize = cu->m_log2CUSize[0];
    uint32_t cuSize = 1 << log2CUSize;
    uint32_t depth  = cu->m_depth[0];

    int hChromaShift = CHROMA_H_SHIFT(m_csp);
    int vChromaShift = CHROMA_V_SHIFT(m_csp);
    int part = partitionFromLog2Size(log2CUSize);
    int cpart = partitionFromSizes(cuSize >> hChromaShift, cuSize >> vChromaShift);

    m_quant.setQPforQuant(interMode.cu);

    ShortYuv* inputResiYuv = &m_rqt[cuGeom.depth].tmpResiYuv;
    inputResiYuv->subtract(*fencYuv, *predYuv, log2CUSize);

    uint32_t tuDepthRange[2];
    cu->getQuadtreeTULog2MinSizeInCU(tuDepthRange, 0);

    m_entropyCoder.load(m_rqt[depth].cur);

    uint64_t cost = 0;
    uint32_t bits = 0;
    xEstimateResidualQT(interMode, cuGeom, 0, inputResiYuv, depth, cost, bits, tuDepthRange);

    if (!cu->m_tqBypass[0])
    {
        uint32_t cbf0Dist = primitives.sse_pp[part](fencYuv->m_buf[0], fencYuv->m_size, predYuv->m_buf[0], predYuv->m_size);
        cbf0Dist += m_rdCost.scaleChromaDistCb(primitives.sse_pp[cpart](fencYuv->m_buf[1], predYuv->m_csize, predYuv->m_buf[1], predYuv->m_csize));
        cbf0Dist += m_rdCost.scaleChromaDistCr(primitives.sse_pp[cpart](fencYuv->m_buf[2], predYuv->m_csize, predYuv->m_buf[2], predYuv->m_csize));

        /* Consider the RD cost of not signaling any residual */
        m_entropyCoder.load(m_rqt[depth].cur);
        m_entropyCoder.resetBits();
        m_entropyCoder.codeQtRootCbfZero();
        uint32_t cbf0Bits = m_entropyCoder.getNumberOfWrittenBits();

        uint64_t cbf0Cost;
        uint32_t cbf0Energy;
        if (m_rdCost.m_psyRd)
        {
            cbf0Energy = m_rdCost.psyCost(log2CUSize - 2, fencYuv->m_buf[0], fencYuv->m_size, predYuv->m_buf[0], predYuv->m_size);
            cbf0Cost = m_rdCost.calcPsyRdCost(cbf0Dist, cbf0Bits, cbf0Energy);
        }
        else
            cbf0Cost = m_rdCost.calcRdCost(cbf0Dist, cbf0Bits);

        if (cbf0Cost < cost)
        {
            cu->clearCbf(0, depth);
            cu->setTransformSkipSubParts(0);
            const uint32_t qpartnum = NUM_CU_PARTITIONS >> (depth << 1); /* TODO is this necessary with CBF=0? */
            memset(cu->m_trIdx, 0, qpartnum * sizeof(uint8_t));
        }
    }

    if (cu->getQtRootCbf(0))
        saveResidualQTData(*cu, interMode.resiYuv, 0, depth);

    /* calculate signal bits for inter/merge/skip coded CU */
    m_entropyCoder.load(m_rqt[depth].cur);

    uint32_t coeffBits;
    if (cu->m_mergeFlag[0] && cu->m_partSize[0] == SIZE_2Nx2N && !cu->getQtRootCbf(0))
    {
        cu->setSkipFlagSubParts(true);

        /* Merge/Skip */
        m_entropyCoder.resetBits();
        if (m_slice->m_pps->bTransquantBypassEnabled)
            m_entropyCoder.codeCUTransquantBypassFlag(cu->m_tqBypass[0]);
        m_entropyCoder.codeSkipFlag(*cu, 0);
        m_entropyCoder.codeMergeIndex(*cu, 0);
        coeffBits = 0;
        bits = m_entropyCoder.getNumberOfWrittenBits();
    }
    else
    {
        m_entropyCoder.resetBits();
        if (m_slice->m_pps->bTransquantBypassEnabled)
            m_entropyCoder.codeCUTransquantBypassFlag(cu->m_tqBypass[0]);
        m_entropyCoder.codeSkipFlag(*cu, 0);
        m_entropyCoder.codePredMode(cu->m_predMode[0]);
        m_entropyCoder.codePartSize(*cu, 0, cu->m_depth[0]);
        m_entropyCoder.codePredInfo(*cu, 0);
        uint32_t mvBits = m_entropyCoder.getNumberOfWrittenBits();

        bool bCodeDQP = m_slice->m_pps->bUseDQP;
        m_entropyCoder.codeCoeff(*cu, 0, cu->m_depth[0], bCodeDQP, tuDepthRange);
        bits = m_entropyCoder.getNumberOfWrittenBits();

        coeffBits = bits - mvBits;
    }

    m_entropyCoder.store(interMode.contexts);

    if (cu->getQtRootCbf(0))
        reconYuv->addClip(*predYuv, *resiYuv, log2CUSize);
    else
        reconYuv->copyFromYuv(*predYuv);

    // update with clipped distortion and cost (qp estimation loop uses unclipped values)
    uint32_t bestDist = primitives.sse_pp[part](fencYuv->m_buf[0], fencYuv->m_size, reconYuv->m_buf[0], reconYuv->m_size);
    bestDist += m_rdCost.scaleChromaDistCb(primitives.sse_pp[cpart](fencYuv->m_buf[1], fencYuv->m_csize, reconYuv->m_buf[1], reconYuv->m_csize));
    bestDist += m_rdCost.scaleChromaDistCr(primitives.sse_pp[cpart](fencYuv->m_buf[2], fencYuv->m_csize, reconYuv->m_buf[2], reconYuv->m_csize));
    if (m_rdCost.m_psyRd)
        interMode.psyEnergy = m_rdCost.psyCost(log2CUSize - 2, fencYuv->m_buf[0], fencYuv->m_size, reconYuv->m_buf[0], reconYuv->m_size);

    interMode.totalBits = bits;
    interMode.distortion = bestDist;
    interMode.coeffBits = coeffBits;
    interMode.mvBits = bits - coeffBits;
    updateModeCost(interMode);
}

void Search::generateCoeffRecon(Mode& mode, const CUGeom& cuGeom)
{
    CUData* cu = &mode.cu;

    m_quant.setQPforQuant(mode.cu);

    uint32_t tuDepthRange[2];
    cu->getQuadtreeTULog2MinSizeInCU(tuDepthRange, 0);

    if (cu->m_predMode[0] == MODE_INTER)
    {
        residualTransformQuantInter(mode, cuGeom, 0, cu->m_depth[0], tuDepthRange);
        if (cu->getQtRootCbf(0))
            mode.reconYuv.addClip(mode.predYuv, mode.resiYuv, cu->m_log2CUSize[0]);
        else
        {
            mode.reconYuv.copyFromYuv(mode.predYuv);
            if (cu->m_mergeFlag[0] && cu->m_partSize[0] == SIZE_2Nx2N)
                cu->setSkipFlagSubParts(true);
        }
    }
    else if (cu->m_predMode[0] == MODE_INTRA)
    {
        uint32_t initTrDepth = cu->m_partSize[0] == SIZE_2Nx2N ? 0 : 1;
        residualTransformQuantIntra(mode, cuGeom, initTrDepth, 0, tuDepthRange);
        getBestIntraModeChroma(mode, cuGeom);
        residualQTIntraChroma(mode, cuGeom, 0, 0);
    }
}

void Search::residualTransformQuantInter(Mode& mode, const CUGeom& cuGeom, uint32_t absPartIdx, const uint32_t depth, uint32_t depthRange[2])
{
    CUData* cu = &mode.cu;
    ShortYuv* resiYuv = &mode.resiYuv;
    const Yuv* fencYuv = mode.fencYuv;

    X265_CHECK(cu->m_depth[0] == cu->m_depth[absPartIdx], "invalid depth\n");
    const uint32_t trMode = depth - cu->m_depth[0];
    const uint32_t log2TrSize = g_maxLog2CUSize - depth;
    const uint32_t setCbf     = 1 << trMode;
    int hChromaShift = CHROMA_H_SHIFT(m_csp);
    int vChromaShift = CHROMA_V_SHIFT(m_csp);

    bool bSplitFlag = ((m_slice->m_sps->quadtreeTUMaxDepthInter == 1) && cu->m_predMode[absPartIdx] == MODE_INTER && (cu->m_partSize[absPartIdx] != SIZE_2Nx2N));
    bool bCheckFull;
    if (bSplitFlag && depth == cu->m_depth[absPartIdx] && log2TrSize > depthRange[0])
        bCheckFull = false;
    else
        bCheckFull = log2TrSize <= depthRange[1];
    const bool bCheckSplit = log2TrSize > depthRange[0];
    X265_CHECK(bCheckFull || bCheckSplit, "check-full or check-split must be set\n");

    // code full block
    if (bCheckFull)
    {
        uint32_t log2TrSizeC = log2TrSize - hChromaShift;
        bool bCodeChroma = true;
        uint32_t trModeC = trMode;
        if ((log2TrSize == 2) && !(m_csp == X265_CSP_I444))
        {
            log2TrSizeC++;
            trModeC--;
            uint32_t qpdiv = NUM_CU_PARTITIONS >> ((depth - 1) << 1);
            bCodeChroma = ((absPartIdx & (qpdiv - 1)) == 0);
        }

        const bool splitIntoSubTUs = (m_csp == X265_CSP_I422);
        uint32_t absPartIdxStep = NUM_CU_PARTITIONS >> ((cu->m_depth[0] +  trModeC) << 1);

        uint32_t coeffOffsetY = absPartIdx << (LOG2_UNIT_SIZE * 2);
        uint32_t coeffOffsetC = coeffOffsetY >> (hChromaShift + vChromaShift);
        coeff_t *coeffCurY = cu->m_trCoeff[0]  + coeffOffsetY;
        coeff_t *coeffCurU = cu->m_trCoeff[1] + coeffOffsetC;
        coeff_t *coeffCurV = cu->m_trCoeff[2] + coeffOffsetC;

        uint32_t sizeIdx  = log2TrSize  - 2;
        uint32_t sizeIdxC = log2TrSizeC - 2;
        cu->setTrIdxSubParts(depth - cu->m_depth[0], absPartIdx, depth);

        cu->setTransformSkipSubParts(0, TEXT_LUMA, absPartIdx, depth);

        int16_t *curResiY = resiYuv->getLumaAddr(absPartIdx);
        const uint32_t strideResiY = resiYuv->m_size;
        const uint32_t strideResiC = resiYuv->m_csize;

        pixel *fenc = const_cast<pixel*>(fencYuv->getLumaAddr(absPartIdx));
        uint32_t numSigY = m_quant.transformNxN(cu, fenc, fencYuv->m_size, curResiY, strideResiY, coeffCurY, log2TrSize, TEXT_LUMA, absPartIdx, false);

        cu->setCbfSubParts(numSigY ? setCbf : 0, TEXT_LUMA, absPartIdx, depth);

        if (numSigY)
            m_quant.invtransformNxN(cu->m_tqBypass[absPartIdx], curResiY, strideResiY, coeffCurY, log2TrSize, TEXT_LUMA, false, false, numSigY);
        else
            primitives.blockfill_s[sizeIdx](curResiY, strideResiY, 0);

        if (bCodeChroma)
        {
            TURecurse tuIterator(splitIntoSubTUs ? VERTICAL_SPLIT : DONT_SPLIT, absPartIdxStep, absPartIdx);

            do
            {
                uint32_t absPartIdxC = tuIterator.absPartIdxTURelCU;
                uint32_t subTUOffset = tuIterator.section << (log2TrSizeC * 2);

                int16_t *curResiU = resiYuv->getCbAddr(absPartIdxC);
                int16_t *curResiV = resiYuv->getCrAddr(absPartIdxC);

                cu->setTransformSkipPartRange(0, TEXT_CHROMA_U, absPartIdxC, tuIterator.absPartIdxStep);
                cu->setTransformSkipPartRange(0, TEXT_CHROMA_V, absPartIdxC, tuIterator.absPartIdxStep);

                pixel *fencCb = const_cast<pixel*>(fencYuv->getCbAddr(absPartIdxC));
                pixel *fencCr = const_cast<pixel*>(fencYuv->getCrAddr(absPartIdxC));
                uint32_t numSigU = m_quant.transformNxN(cu, fencCb, fencYuv->m_csize, curResiU, strideResiC, coeffCurU + subTUOffset, log2TrSizeC, TEXT_CHROMA_U, absPartIdxC, false);
                uint32_t numSigV = m_quant.transformNxN(cu, fencCr, fencYuv->m_csize, curResiV, strideResiC, coeffCurV + subTUOffset, log2TrSizeC, TEXT_CHROMA_V, absPartIdxC, false);

                cu->setCbfPartRange(numSigU ? setCbf : 0, TEXT_CHROMA_U, absPartIdxC, tuIterator.absPartIdxStep);
                cu->setCbfPartRange(numSigV ? setCbf : 0, TEXT_CHROMA_V, absPartIdxC, tuIterator.absPartIdxStep);

                if (numSigU)
                    m_quant.invtransformNxN(cu->m_tqBypass[absPartIdxC], curResiU, strideResiC, coeffCurU + subTUOffset, log2TrSizeC, TEXT_CHROMA_U, false, false, numSigU);
                else
                    primitives.blockfill_s[sizeIdxC](curResiU, strideResiC, 0);

                if (numSigV)
                    m_quant.invtransformNxN(cu->m_tqBypass[absPartIdxC], curResiV, strideResiC, coeffCurV + subTUOffset, log2TrSizeC, TEXT_CHROMA_V, false, false, numSigV);
                else
                    primitives.blockfill_s[sizeIdxC](curResiV, strideResiC, 0);
            }
            while (tuIterator.isNextSection());

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
        const uint32_t qPartNumSubdiv = NUM_CU_PARTITIONS >> ((depth + 1) << 1);
        for (uint32_t i = 0; i < 4; ++i)
            residualTransformQuantInter(mode, cuGeom, absPartIdx + i * qPartNumSubdiv, depth + 1, depthRange);

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
            cu->m_cbf[0][absPartIdx + i] |= ycbf << trMode;
            cu->m_cbf[1][absPartIdx + i] |= ucbf << trMode;
            cu->m_cbf[2][absPartIdx + i] |= vcbf << trMode;
        }
    }
}

uint32_t Search::xEstimateResidualQT(Mode& mode, const CUGeom& cuGeom, uint32_t absPartIdx, ShortYuv* resiYuv, uint32_t depth, uint64_t& rdCost,
                                     uint32_t& outBits, uint32_t depthRange[2])
{
    CUData* cu = &mode.cu;
    const Yuv* fencYuv = mode.fencYuv;

    X265_CHECK(cu->m_depth[0] == cu->m_depth[absPartIdx], "depth not matching\n");
    const uint32_t trMode = depth - cu->m_depth[0];
    const uint32_t log2TrSize = g_maxLog2CUSize - depth;
    const uint32_t subTUDepth = trMode + 1;
    const uint32_t setCbf     = 1 << trMode;
    uint32_t outDist = 0;
    int hChromaShift = CHROMA_H_SHIFT(m_csp);
    int vChromaShift = CHROMA_V_SHIFT(m_csp);

    bool bSplitFlag = ((m_slice->m_sps->quadtreeTUMaxDepthInter == 1) && cu->m_predMode[absPartIdx] == MODE_INTER && (cu->m_partSize[absPartIdx] != SIZE_2Nx2N));
    bool bCheckFull;
    if (bSplitFlag && depth == cu->m_depth[absPartIdx] && log2TrSize > depthRange[0])
        bCheckFull = false;
    else
        bCheckFull = log2TrSize <= depthRange[1];
    const bool bCheckSplit = log2TrSize > depthRange[0];

    X265_CHECK(bCheckFull || bCheckSplit, "check-full or check-split must be set\n");

    uint32_t log2TrSizeC = log2TrSize - hChromaShift;
    bool bCodeChroma = true;
    uint32_t trModeC = trMode;
    if ((log2TrSize == 2) && !(m_csp == X265_CSP_I444))
    {
        log2TrSizeC++;
        trModeC--;
        uint32_t qpdiv = NUM_CU_PARTITIONS >> ((depth - 1) << 1);
        bCodeChroma = ((absPartIdx & (qpdiv - 1)) == 0);
    }

    // code full block
    uint64_t singleCost = MAX_INT64;
    uint32_t singleBits = 0;
    uint32_t singleDist = 0;
    uint32_t singlePsyEnergy = 0;
    uint32_t singleBitsComp[MAX_NUM_COMPONENT][2 /*0 = top (or whole TU for non-4:2:2) sub-TU, 1 = bottom sub-TU*/] = { { 0, 0 }, { 0, 0 }, { 0, 0 } };
    uint32_t singleDistComp[MAX_NUM_COMPONENT][2 /*0 = top (or whole TU for non-4:2:2) sub-TU, 1 = bottom sub-TU*/] = { { 0, 0 }, { 0, 0 }, { 0, 0 } };
    uint32_t singlePsyEnergyComp[MAX_NUM_COMPONENT][2] = { { 0, 0 }, { 0, 0 }, { 0, 0 } };
    uint32_t numSigY = 0;
    uint32_t bestTransformMode[MAX_NUM_COMPONENT][2 /*0 = top (or whole TU for non-4:2:2) sub-TU, 1 = bottom sub-TU*/] = { { 0, 0 }, { 0, 0 }, { 0, 0 } };
    uint64_t minCost[MAX_NUM_COMPONENT][2 /*0 = top (or whole TU for non-4:2:2) sub-TU, 1 = bottom sub-TU*/];

    uint32_t bestCBF[MAX_NUM_COMPONENT];
    uint32_t bestsubTUCBF[MAX_NUM_COMPONENT][2];
    m_entropyCoder.store(m_rqt[depth].rqtRoot);

    uint32_t trSize = 1 << log2TrSize;
    const bool splitIntoSubTUs = (m_csp == X265_CSP_I422);
    uint32_t absPartIdxStep = NUM_CU_PARTITIONS >> ((cu->m_depth[0] +  trModeC) << 1);

    // code full block
    if (bCheckFull)
    {
        uint32_t numSigU[2 /*0 = top (or whole TU for non-4:2:2) sub-TU, 1 = bottom sub-TU*/] = { 0, 0 };
        uint32_t numSigV[2 /*0 = top (or whole TU for non-4:2:2) sub-TU, 1 = bottom sub-TU*/] = { 0, 0 };
        uint32_t trSizeC = 1 << log2TrSizeC;
        int partSize  = partitionFromLog2Size(log2TrSize);
        int partSizeC = partitionFromLog2Size(log2TrSizeC);
        const uint32_t qtLayer = log2TrSize - 2;
        uint32_t coeffOffsetY = absPartIdx << (LOG2_UNIT_SIZE * 2);
        uint32_t coeffOffsetC = coeffOffsetY >> (hChromaShift + vChromaShift);
        coeff_t* coeffCurY = m_rqt[qtLayer].tmpCoeff[0] + coeffOffsetY;
        coeff_t* coeffCurU = m_rqt[qtLayer].tmpCoeff[1] + coeffOffsetC;
        coeff_t* coeffCurV = m_rqt[qtLayer].tmpCoeff[2] + coeffOffsetC;

        cu->setTrIdxSubParts(depth - cu->m_depth[0], absPartIdx, depth);
        bool checkTransformSkip   = m_slice->m_pps->bTransformSkipEnabled && !cu->m_tqBypass[0];
        bool checkTransformSkipY  = checkTransformSkip && log2TrSize  <= MAX_LOG2_TS_SIZE;
        bool checkTransformSkipUV = checkTransformSkip && log2TrSizeC <= MAX_LOG2_TS_SIZE;

        cu->setTransformSkipSubParts(0, TEXT_LUMA, absPartIdx, depth);

        if (m_bEnableRDOQ)
            m_entropyCoder.estBit(m_entropyCoder.m_estBitsSbac, log2TrSize, true);

        pixel *fenc = const_cast<pixel*>(fencYuv->getLumaAddr(absPartIdx));
        int16_t *resi = resiYuv->getLumaAddr(absPartIdx);
        numSigY = m_quant.transformNxN(cu, fenc, fencYuv->m_size, resi, resiYuv->m_size, coeffCurY, log2TrSize, TEXT_LUMA, absPartIdx, false);

        cu->setCbfSubParts(numSigY ? setCbf : 0, TEXT_LUMA, absPartIdx, depth);

        m_entropyCoder.resetBits();
        m_entropyCoder.codeQtCbf(*cu, absPartIdx, TEXT_LUMA, trMode);
        if (numSigY)
            m_entropyCoder.codeCoeffNxN(*cu, coeffCurY, absPartIdx, log2TrSize, TEXT_LUMA);
        singleBitsComp[TEXT_LUMA][0] = m_entropyCoder.getNumberOfWrittenBits();

        uint32_t singleBitsPrev = singleBitsComp[TEXT_LUMA][0];

        if (bCodeChroma)
        {
            TURecurse tuIterator(splitIntoSubTUs ? VERTICAL_SPLIT : DONT_SPLIT, absPartIdxStep, absPartIdx);

            do
            {
                uint32_t absPartIdxC = tuIterator.absPartIdxTURelCU;
                uint32_t subTUOffset = tuIterator.section << (log2TrSizeC * 2);

                cu->setTransformSkipPartRange(0, TEXT_CHROMA_U, absPartIdxC, tuIterator.absPartIdxStep);
                cu->setTransformSkipPartRange(0, TEXT_CHROMA_V, absPartIdxC, tuIterator.absPartIdxStep);

                if (m_bEnableRDOQ)
                    m_entropyCoder.estBit(m_entropyCoder.m_estBitsSbac, log2TrSizeC, false);

                fenc = const_cast<pixel*>(fencYuv->getCbAddr(absPartIdxC));
                resi = resiYuv->getCbAddr(absPartIdxC);
                numSigU[tuIterator.section] = m_quant.transformNxN(cu, fenc, fencYuv->m_csize, resi, resiYuv->m_csize, coeffCurU + subTUOffset, log2TrSizeC, TEXT_CHROMA_U, absPartIdxC, false);

                fenc = const_cast<pixel*>(fencYuv->getCrAddr(absPartIdxC));
                resi = resiYuv->getCrAddr(absPartIdxC);
                numSigV[tuIterator.section] = m_quant.transformNxN(cu, fenc, fencYuv->m_csize, resi, resiYuv->m_csize, coeffCurV + subTUOffset, log2TrSizeC, TEXT_CHROMA_V, absPartIdxC, false);

                cu->setCbfPartRange(numSigU[tuIterator.section] ? setCbf : 0, TEXT_CHROMA_U, absPartIdxC, tuIterator.absPartIdxStep);
                cu->setCbfPartRange(numSigV[tuIterator.section] ? setCbf : 0, TEXT_CHROMA_V, absPartIdxC, tuIterator.absPartIdxStep);

                m_entropyCoder.codeQtCbf(*cu, absPartIdxC, TEXT_CHROMA_U, trMode);
                if (numSigU[tuIterator.section])
                    m_entropyCoder.codeCoeffNxN(*cu, coeffCurU + subTUOffset, absPartIdxC, log2TrSizeC, TEXT_CHROMA_U);
                singleBitsComp[TEXT_CHROMA_U][tuIterator.section] = m_entropyCoder.getNumberOfWrittenBits() - singleBitsPrev;

                m_entropyCoder.codeQtCbf(*cu, absPartIdxC, TEXT_CHROMA_V, trMode);
                if (numSigV[tuIterator.section])
                    m_entropyCoder.codeCoeffNxN(*cu, coeffCurV + subTUOffset, absPartIdxC, log2TrSizeC, TEXT_CHROMA_V);

                uint32_t newBits = m_entropyCoder.getNumberOfWrittenBits();
                singleBitsComp[TEXT_CHROMA_V][tuIterator.section] = newBits - (singleBitsPrev + singleBitsComp[TEXT_CHROMA_U][tuIterator.section]);

                singleBitsPrev = newBits;
            }
            while (tuIterator.isNextSection());
        }

        const uint32_t numCoeffY = 1 << (log2TrSize * 2);
        const uint32_t numCoeffC = 1 << (log2TrSizeC * 2);

        for (uint32_t subTUIndex = 0; subTUIndex < 2; subTUIndex++)
        {
            minCost[TEXT_LUMA][subTUIndex]     = MAX_INT64;
            minCost[TEXT_CHROMA_U][subTUIndex] = MAX_INT64;
            minCost[TEXT_CHROMA_V][subTUIndex] = MAX_INT64;
        }

        X265_CHECK(log2TrSize <= 5, "log2TrSize is too large\n");
        uint32_t distY = primitives.ssd_s[partSize](resiYuv->getLumaAddr(absPartIdx), resiYuv->m_size);
        uint32_t psyEnergyY = 0;
        if (m_rdCost.m_psyRd)
            psyEnergyY = m_rdCost.psyCost(partSize, resiYuv->getLumaAddr(absPartIdx), resiYuv->m_size, (int16_t*)zeroShort, 0);

        int16_t *curResiY = m_rqt[qtLayer].tmpShortYuv.getLumaAddr(absPartIdx);
        X265_CHECK(m_rqt[qtLayer].tmpShortYuv.m_size == MAX_CU_SIZE, "width not full CU\n");
        const uint32_t strideResiY = MAX_CU_SIZE;
        const uint32_t strideResiC = m_rqt[qtLayer].tmpShortYuv.m_csize;

        if (numSigY)
        {
            m_quant.invtransformNxN(cu->m_tqBypass[absPartIdx], curResiY, strideResiY, coeffCurY, log2TrSize, TEXT_LUMA, false, false, numSigY); //this is for inter mode only

            const uint32_t nonZeroDistY = primitives.sse_ss[partSize](resiYuv->getLumaAddr(absPartIdx), resiYuv->m_size, curResiY, strideResiY);
            uint32_t nonZeroPsyEnergyY = 0;
            if (m_rdCost.m_psyRd)
                nonZeroPsyEnergyY = m_rdCost.psyCost(partSize, resiYuv->getLumaAddr(absPartIdx), resiYuv->m_size, curResiY, strideResiY);

            if (cu->m_tqBypass[0])
            {
                distY = nonZeroDistY;
                psyEnergyY = nonZeroPsyEnergyY;
            }
            else
            {
                uint64_t singleCostY = 0;
                if (m_rdCost.m_psyRd)
                    singleCostY = m_rdCost.calcPsyRdCost(nonZeroDistY, singleBitsComp[TEXT_LUMA][0], nonZeroPsyEnergyY);
                else
                    singleCostY = m_rdCost.calcRdCost(nonZeroDistY, singleBitsComp[TEXT_LUMA][0]);
                m_entropyCoder.resetBits();
                m_entropyCoder.codeQtCbfZero(TEXT_LUMA, trMode);
                const uint32_t nullBitsY = m_entropyCoder.getNumberOfWrittenBits();
                uint64_t nullCostY = 0;
                if (m_rdCost.m_psyRd)
                    nullCostY = m_rdCost.calcPsyRdCost(distY, nullBitsY, psyEnergyY);
                else
                    nullCostY = m_rdCost.calcRdCost(distY, nullBitsY);
                if (nullCostY < singleCostY)
                {
                    numSigY = 0;
#if CHECKED_BUILD || _DEBUG
                    ::memset(coeffCurY, 0, sizeof(coeff_t) * numCoeffY);
#endif
                    if (checkTransformSkipY)
                        minCost[TEXT_LUMA][0] = nullCostY;
                }
                else
                {
                    distY = nonZeroDistY;
                    psyEnergyY = nonZeroPsyEnergyY;
                    if (checkTransformSkipY)
                        minCost[TEXT_LUMA][0] = singleCostY;
                }
            }
        }
        else if (checkTransformSkipY)
        {
            m_entropyCoder.resetBits();
            m_entropyCoder.codeQtCbfZero(TEXT_LUMA, trMode);
            const uint32_t nullBitsY = m_entropyCoder.getNumberOfWrittenBits();
            if (m_rdCost.m_psyRd)
                minCost[TEXT_LUMA][0] = m_rdCost.calcPsyRdCost(distY, nullBitsY, psyEnergyY);
            else
                minCost[TEXT_LUMA][0] = m_rdCost.calcRdCost(distY, nullBitsY);
        }

        singleDistComp[TEXT_LUMA][0] = distY;
        singlePsyEnergyComp[TEXT_LUMA][0] = psyEnergyY;
        if (!numSigY)
            primitives.blockfill_s[partSize](curResiY, strideResiY, 0);
        cu->setCbfSubParts(numSigY ? setCbf : 0, TEXT_LUMA, absPartIdx, depth);

        uint32_t distU = 0;
        uint32_t distV = 0;
        uint32_t psyEnergyU = 0;
        uint32_t psyEnergyV = 0;
        if (bCodeChroma)
        {
            TURecurse tuIterator(splitIntoSubTUs ? VERTICAL_SPLIT : DONT_SPLIT, absPartIdxStep, absPartIdx);

            do
            {
                uint32_t absPartIdxC = tuIterator.absPartIdxTURelCU;
                uint32_t subTUOffset = tuIterator.section << (log2TrSizeC * 2);

                int16_t *curResiU = m_rqt[qtLayer].tmpShortYuv.getCbAddr(absPartIdxC);
                int16_t *curResiV = m_rqt[qtLayer].tmpShortYuv.getCrAddr(absPartIdxC);

                distU = m_rdCost.scaleChromaDistCb(primitives.ssd_s[log2TrSizeC - 2](resiYuv->getCbAddr(absPartIdxC), resiYuv->m_csize));

                if (numSigU[tuIterator.section])
                {
                    m_quant.invtransformNxN(cu->m_tqBypass[absPartIdxC], curResiU, strideResiC, coeffCurU + subTUOffset,
                                            log2TrSizeC, TEXT_CHROMA_U, false, false, numSigU[tuIterator.section]);
                    uint32_t dist = primitives.sse_ss[partSizeC](resiYuv->getCbAddr(absPartIdxC), resiYuv->m_csize, curResiU, strideResiC);
                    const uint32_t nonZeroDistU = m_rdCost.scaleChromaDistCb(dist);
                    uint32_t nonZeroPsyEnergyU = 0;
                    if (m_rdCost.m_psyRd)
                        nonZeroPsyEnergyU = m_rdCost.psyCost(partSizeC, resiYuv->getCbAddr(absPartIdxC), resiYuv->m_csize, curResiU, strideResiC);

                    if (cu->m_tqBypass[0])
                    {
                        distU = nonZeroDistU;
                        psyEnergyU = nonZeroPsyEnergyU;
                    }
                    else
                    {
                        uint64_t singleCostU = 0;
                        if (m_rdCost.m_psyRd)
                            singleCostU = m_rdCost.calcPsyRdCost(nonZeroDistU, singleBitsComp[TEXT_CHROMA_U][tuIterator.section], nonZeroPsyEnergyU);
                        else
                            singleCostU = m_rdCost.calcRdCost(nonZeroDistU, singleBitsComp[TEXT_CHROMA_U][tuIterator.section]);
                        m_entropyCoder.resetBits();
                        m_entropyCoder.codeQtCbfZero(TEXT_CHROMA_U, trMode);
                        const uint32_t nullBitsU = m_entropyCoder.getNumberOfWrittenBits();
                        uint64_t nullCostU = 0;
                        if (m_rdCost.m_psyRd)
                            nullCostU = m_rdCost.calcPsyRdCost(distU, nullBitsU, psyEnergyU);
                        else
                            nullCostU = m_rdCost.calcRdCost(distU, nullBitsU);
                        if (nullCostU < singleCostU)
                        {
                            numSigU[tuIterator.section] = 0;
#if CHECKED_BUILD || _DEBUG
                            ::memset(coeffCurU + subTUOffset, 0, sizeof(coeff_t) * numCoeffC);
#endif
                            if (checkTransformSkipUV)
                                minCost[TEXT_CHROMA_U][tuIterator.section] = nullCostU;
                        }
                        else
                        {
                            distU = nonZeroDistU;
                            psyEnergyU = nonZeroPsyEnergyU;
                            if (checkTransformSkipUV)
                                minCost[TEXT_CHROMA_U][tuIterator.section] = singleCostU;
                        }
                    }
                }
                else if (checkTransformSkipUV)
                {
                    m_entropyCoder.resetBits();
                    m_entropyCoder.codeQtCbfZero(TEXT_CHROMA_U, trModeC);
                    const uint32_t nullBitsU = m_entropyCoder.getNumberOfWrittenBits();
                    if (m_rdCost.m_psyRd)
                        minCost[TEXT_CHROMA_U][tuIterator.section] = m_rdCost.calcPsyRdCost(distU, nullBitsU, psyEnergyU);
                    else
                        minCost[TEXT_CHROMA_U][tuIterator.section] = m_rdCost.calcRdCost(distU, nullBitsU);
                }

                singleDistComp[TEXT_CHROMA_U][tuIterator.section] = distU;
                singlePsyEnergyComp[TEXT_CHROMA_U][tuIterator.section] = psyEnergyU;

                if (!numSigU[tuIterator.section])
                    primitives.blockfill_s[partSizeC](curResiU, strideResiC, 0);

                distV = m_rdCost.scaleChromaDistCr(primitives.ssd_s[partSizeC](resiYuv->getCrAddr(absPartIdxC), resiYuv->m_csize));

                if (numSigV[tuIterator.section])
                {
                    m_quant.invtransformNxN(cu->m_tqBypass[absPartIdxC], curResiV, strideResiC, coeffCurV + subTUOffset,
                                            log2TrSizeC, TEXT_CHROMA_V, false, false, numSigV[tuIterator.section]);
                    uint32_t dist = primitives.sse_ss[partSizeC](resiYuv->getCrAddr(absPartIdxC), resiYuv->m_csize, curResiV, strideResiC);
                    const uint32_t nonZeroDistV = m_rdCost.scaleChromaDistCr(dist);
                    uint32_t nonZeroPsyEnergyV = 0;
                    if (m_rdCost.m_psyRd)
                        nonZeroPsyEnergyV = m_rdCost.psyCost(partSizeC, resiYuv->getCrAddr(absPartIdxC), resiYuv->m_csize, curResiV, strideResiC);

                    if (cu->m_tqBypass[0])
                    {
                        distV = nonZeroDistV;
                        psyEnergyV = nonZeroPsyEnergyV;
                    }
                    else
                    {
                        uint64_t singleCostV = 0;
                        if (m_rdCost.m_psyRd)
                            singleCostV = m_rdCost.calcPsyRdCost(nonZeroDistV, singleBitsComp[TEXT_CHROMA_V][tuIterator.section], nonZeroPsyEnergyV);
                        else
                            singleCostV = m_rdCost.calcRdCost(nonZeroDistV, singleBitsComp[TEXT_CHROMA_V][tuIterator.section]);
                        m_entropyCoder.resetBits();
                        m_entropyCoder.codeQtCbfZero(TEXT_CHROMA_V, trMode);
                        const uint32_t nullBitsV = m_entropyCoder.getNumberOfWrittenBits();
                        uint64_t nullCostV = 0;
                        if (m_rdCost.m_psyRd)
                            nullCostV = m_rdCost.calcPsyRdCost(distV, nullBitsV, psyEnergyV);
                        else
                            nullCostV = m_rdCost.calcRdCost(distV, nullBitsV);
                        if (nullCostV < singleCostV)
                        {
                            numSigV[tuIterator.section] = 0;
#if CHECKED_BUILD || _DEBUG
                            ::memset(coeffCurV + subTUOffset, 0, sizeof(coeff_t) * numCoeffC);
#endif
                            if (checkTransformSkipUV)
                                minCost[TEXT_CHROMA_V][tuIterator.section] = nullCostV;
                        }
                        else
                        {
                            distV = nonZeroDistV;
                            psyEnergyV = nonZeroPsyEnergyV;
                            if (checkTransformSkipUV)
                                minCost[TEXT_CHROMA_V][tuIterator.section] = singleCostV;
                        }
                    }
                }
                else if (checkTransformSkipUV)
                {
                    m_entropyCoder.resetBits();
                    m_entropyCoder.codeQtCbfZero(TEXT_CHROMA_V, trModeC);
                    const uint32_t nullBitsV = m_entropyCoder.getNumberOfWrittenBits();
                    if (m_rdCost.m_psyRd)
                        minCost[TEXT_CHROMA_V][tuIterator.section] = m_rdCost.calcPsyRdCost(distV, nullBitsV, psyEnergyV);
                    else
                        minCost[TEXT_CHROMA_V][tuIterator.section] = m_rdCost.calcRdCost(distV, nullBitsV);
                }

                singleDistComp[TEXT_CHROMA_V][tuIterator.section] = distV;
                singlePsyEnergyComp[TEXT_CHROMA_V][tuIterator.section] = psyEnergyV;

                if (!numSigV[tuIterator.section])
                    primitives.blockfill_s[partSizeC](curResiV, strideResiC, 0);

                cu->setCbfPartRange(numSigU[tuIterator.section] ? setCbf : 0, TEXT_CHROMA_U, absPartIdxC, tuIterator.absPartIdxStep);
                cu->setCbfPartRange(numSigV[tuIterator.section] ? setCbf : 0, TEXT_CHROMA_V, absPartIdxC, tuIterator.absPartIdxStep);
            }
            while (tuIterator.isNextSection());
        }

        if (checkTransformSkipY)
        {
            uint32_t nonZeroDistY = 0;
            uint32_t nonZeroPsyEnergyY = 0;
            uint64_t singleCostY = MAX_INT64;

            ALIGN_VAR_32(coeff_t, tsCoeffY[MAX_TS_SIZE * MAX_TS_SIZE]);
            ALIGN_VAR_32(int16_t, tsResiY[MAX_TS_SIZE * MAX_TS_SIZE]);

            m_entropyCoder.load(m_rqt[depth].rqtRoot);

            cu->setTransformSkipSubParts(1, TEXT_LUMA, absPartIdx, depth);

            if (m_bEnableRDOQ)
                m_entropyCoder.estBit(m_entropyCoder.m_estBitsSbac, log2TrSize, true);

            fenc = const_cast<pixel*>(fencYuv->getLumaAddr(absPartIdx));
            resi = resiYuv->getLumaAddr(absPartIdx);
            uint32_t numSigTSkipY = m_quant.transformNxN(cu, fenc, fencYuv->m_size, resi, resiYuv->m_size, tsCoeffY, log2TrSize, TEXT_LUMA, absPartIdx, true);
            cu->setCbfSubParts(numSigTSkipY ? setCbf : 0, TEXT_LUMA, absPartIdx, depth);

            if (numSigTSkipY)
            {
                m_entropyCoder.resetBits();
                m_entropyCoder.codeQtCbf(*cu, absPartIdx, TEXT_LUMA, trMode);
                m_entropyCoder.codeCoeffNxN(*cu, tsCoeffY, absPartIdx, log2TrSize, TEXT_LUMA);
                const uint32_t skipSingleBitsY = m_entropyCoder.getNumberOfWrittenBits();

                m_quant.invtransformNxN(cu->m_tqBypass[absPartIdx], tsResiY, trSize, tsCoeffY, log2TrSize, TEXT_LUMA, false, true, numSigTSkipY);

                nonZeroDistY = primitives.sse_ss[partSize](resiYuv->getLumaAddr(absPartIdx), resiYuv->m_size, tsResiY, trSize);

                if (m_rdCost.m_psyRd)
                {
                    nonZeroPsyEnergyY = m_rdCost.psyCost(partSize, resiYuv->getLumaAddr(absPartIdx), resiYuv->m_size, tsResiY, trSize);
                    singleCostY = m_rdCost.calcPsyRdCost(nonZeroDistY, skipSingleBitsY, nonZeroPsyEnergyY);
                }
                else
                    singleCostY = m_rdCost.calcRdCost(nonZeroDistY, skipSingleBitsY);
            }

            if (!numSigTSkipY || minCost[TEXT_LUMA][0] < singleCostY)
                cu->setTransformSkipSubParts(0, TEXT_LUMA, absPartIdx, depth);
            else
            {
                singleDistComp[TEXT_LUMA][0] = nonZeroDistY;
                singlePsyEnergyComp[TEXT_LUMA][0] = nonZeroPsyEnergyY;
                numSigY = numSigTSkipY;
                bestTransformMode[TEXT_LUMA][0] = 1;
                memcpy(coeffCurY, tsCoeffY, sizeof(coeff_t) * numCoeffY);
                primitives.square_copy_ss[partSize](curResiY, strideResiY, tsResiY, trSize);
            }

            cu->setCbfSubParts(numSigY ? setCbf : 0, TEXT_LUMA, absPartIdx, depth);
        }

        if (bCodeChroma && checkTransformSkipUV)
        {
            uint32_t nonZeroDistU = 0, nonZeroDistV = 0;
            uint32_t nonZeroPsyEnergyU = 0, nonZeroPsyEnergyV = 0;
            uint64_t singleCostU = MAX_INT64;
            uint64_t singleCostV = MAX_INT64;

            m_entropyCoder.load(m_rqt[depth].rqtRoot);

            TURecurse tuIterator(splitIntoSubTUs ? VERTICAL_SPLIT : DONT_SPLIT, absPartIdxStep, absPartIdx);

            do
            {
                uint32_t absPartIdxC = tuIterator.absPartIdxTURelCU;
                uint32_t subTUOffset = tuIterator.section << (log2TrSizeC * 2);

                int16_t *curResiU = m_rqt[qtLayer].tmpShortYuv.getCbAddr(absPartIdxC);
                int16_t *curResiV = m_rqt[qtLayer].tmpShortYuv.getCrAddr(absPartIdxC);

                ALIGN_VAR_32(coeff_t, tsCoeffU[MAX_TS_SIZE * MAX_TS_SIZE]);
                ALIGN_VAR_32(int16_t, tsResiU[MAX_TS_SIZE * MAX_TS_SIZE]);
                ALIGN_VAR_32(coeff_t, tsCoeffV[MAX_TS_SIZE * MAX_TS_SIZE]);
                ALIGN_VAR_32(int16_t, tsResiV[MAX_TS_SIZE * MAX_TS_SIZE]);

                cu->setTransformSkipPartRange(1, TEXT_CHROMA_U, absPartIdxC, tuIterator.absPartIdxStep);
                cu->setTransformSkipPartRange(1, TEXT_CHROMA_V, absPartIdxC, tuIterator.absPartIdxStep);

                if (m_bEnableRDOQ)
                    m_entropyCoder.estBit(m_entropyCoder.m_estBitsSbac, log2TrSizeC, false);

                fenc = const_cast<pixel*>(fencYuv->getCbAddr(absPartIdxC));
                resi = resiYuv->getCbAddr(absPartIdxC);
                uint32_t numSigTSkipU = m_quant.transformNxN(cu, fenc, fencYuv->m_csize, resi, resiYuv->m_csize, tsCoeffU, log2TrSizeC, TEXT_CHROMA_U, absPartIdxC, true);

                fenc = const_cast<pixel*>(fencYuv->getCrAddr(absPartIdxC));
                resi = resiYuv->getCrAddr(absPartIdxC);
                uint32_t numSigTSkipV = m_quant.transformNxN(cu, fenc, fencYuv->m_csize, resi, resiYuv->m_csize, tsCoeffV, log2TrSizeC, TEXT_CHROMA_V, absPartIdxC, true);

                cu->setCbfPartRange(numSigTSkipU ? setCbf : 0, TEXT_CHROMA_U, absPartIdxC, tuIterator.absPartIdxStep);
                cu->setCbfPartRange(numSigTSkipV ? setCbf : 0, TEXT_CHROMA_V, absPartIdxC, tuIterator.absPartIdxStep);

                m_entropyCoder.resetBits();
                singleBitsComp[TEXT_CHROMA_U][tuIterator.section] = 0;

                if (numSigTSkipU)
                {
                    m_entropyCoder.codeQtCbf(*cu, absPartIdxC, TEXT_CHROMA_U, trMode);
                    m_entropyCoder.codeCoeffNxN(*cu, tsCoeffU, absPartIdxC, log2TrSizeC, TEXT_CHROMA_U);
                    singleBitsComp[TEXT_CHROMA_U][tuIterator.section] = m_entropyCoder.getNumberOfWrittenBits();

                    m_quant.invtransformNxN(cu->m_tqBypass[absPartIdxC], tsResiU, trSizeC, tsCoeffU,
                                            log2TrSizeC, TEXT_CHROMA_U, false, true, numSigTSkipU);
                    uint32_t dist = primitives.sse_ss[partSizeC](resiYuv->getCbAddr(absPartIdxC), resiYuv->m_csize, tsResiU, trSizeC);
                    nonZeroDistU = m_rdCost.scaleChromaDistCb(dist);
                    if (m_rdCost.m_psyRd)
                    {
                        nonZeroPsyEnergyU = m_rdCost.psyCost(partSizeC, resiYuv->getCbAddr(absPartIdxC), resiYuv->m_csize, tsResiU, trSizeC);
                        singleCostU = m_rdCost.calcPsyRdCost(nonZeroDistU, singleBitsComp[TEXT_CHROMA_U][tuIterator.section], nonZeroPsyEnergyU);
                    }
                    else
                        singleCostU = m_rdCost.calcRdCost(nonZeroDistU, singleBitsComp[TEXT_CHROMA_U][tuIterator.section]);
                }

                if (!numSigTSkipU || minCost[TEXT_CHROMA_U][tuIterator.section] < singleCostU)
                    cu->setTransformSkipPartRange(0, TEXT_CHROMA_U, absPartIdxC, tuIterator.absPartIdxStep);
                else
                {
                    singleDistComp[TEXT_CHROMA_U][tuIterator.section] = nonZeroDistU;
                    singlePsyEnergyComp[TEXT_CHROMA_U][tuIterator.section] = nonZeroPsyEnergyU;
                    numSigU[tuIterator.section] = numSigTSkipU;
                    bestTransformMode[TEXT_CHROMA_U][tuIterator.section] = 1;
                    memcpy(coeffCurU + subTUOffset, tsCoeffU, sizeof(coeff_t) * numCoeffC);
                    primitives.square_copy_ss[partSizeC](curResiU, strideResiC, tsResiU, trSizeC);
                }

                if (numSigTSkipV)
                {
                    m_entropyCoder.codeQtCbf(*cu, absPartIdxC, TEXT_CHROMA_V, trMode);
                    m_entropyCoder.codeCoeffNxN(*cu, tsCoeffV, absPartIdxC, log2TrSizeC, TEXT_CHROMA_V);
                    singleBitsComp[TEXT_CHROMA_V][tuIterator.section] = m_entropyCoder.getNumberOfWrittenBits() - singleBitsComp[TEXT_CHROMA_U][tuIterator.section];

                    m_quant.invtransformNxN(cu->m_tqBypass[absPartIdxC], tsResiV, trSizeC, tsCoeffV,
                                            log2TrSizeC, TEXT_CHROMA_V, false, true, numSigTSkipV);
                    uint32_t dist = primitives.sse_ss[partSizeC](resiYuv->getCrAddr(absPartIdxC), resiYuv->m_csize, tsResiV, trSizeC);
                    nonZeroDistV = m_rdCost.scaleChromaDistCr(dist);
                    if (m_rdCost.m_psyRd)
                    {
                        nonZeroPsyEnergyV = m_rdCost.psyCost(partSizeC, resiYuv->getCrAddr(absPartIdxC), resiYuv->m_csize, tsResiV, trSizeC);
                        singleCostV = m_rdCost.calcPsyRdCost(nonZeroDistV, singleBitsComp[TEXT_CHROMA_V][tuIterator.section], nonZeroPsyEnergyV);
                    }
                    else
                        singleCostV = m_rdCost.calcRdCost(nonZeroDistV, singleBitsComp[TEXT_CHROMA_V][tuIterator.section]);
                }

                if (!numSigTSkipV || minCost[TEXT_CHROMA_V][tuIterator.section] < singleCostV)
                    cu->setTransformSkipPartRange(0, TEXT_CHROMA_V, absPartIdxC, tuIterator.absPartIdxStep);
                else
                {
                    singleDistComp[TEXT_CHROMA_V][tuIterator.section] = nonZeroDistV;
                    singlePsyEnergyComp[TEXT_CHROMA_V][tuIterator.section] = nonZeroPsyEnergyV;
                    numSigV[tuIterator.section] = numSigTSkipV;
                    bestTransformMode[TEXT_CHROMA_V][tuIterator.section] = 1;
                    memcpy(coeffCurV + subTUOffset, tsCoeffV, sizeof(coeff_t) * numCoeffC);
                    primitives.square_copy_ss[partSizeC](curResiV, strideResiC, tsResiV, trSizeC);
                }

                cu->setCbfPartRange(numSigU[tuIterator.section] ? setCbf : 0, TEXT_CHROMA_U, absPartIdxC, tuIterator.absPartIdxStep);
                cu->setCbfPartRange(numSigV[tuIterator.section] ? setCbf : 0, TEXT_CHROMA_V, absPartIdxC, tuIterator.absPartIdxStep);
            }
            while (tuIterator.isNextSection());
        }

        m_entropyCoder.load(m_rqt[depth].rqtRoot);

        m_entropyCoder.resetBits();

        if (log2TrSize > depthRange[0])
            m_entropyCoder.codeTransformSubdivFlag(0, 5 - log2TrSize);

        if (bCodeChroma)
        {
            if (splitIntoSubTUs)
            {
                offsetSubTUCBFs(cu, TEXT_CHROMA_U, trMode, absPartIdx);
                offsetSubTUCBFs(cu, TEXT_CHROMA_V, trMode, absPartIdx);
            }

            uint32_t trHeightC = (m_csp == X265_CSP_I422) ? (trSizeC << 1) : trSizeC;
            m_entropyCoder.codeQtCbf(*cu, absPartIdx, absPartIdxStep, trSizeC, trHeightC, TEXT_CHROMA_U, trMode, true);
            m_entropyCoder.codeQtCbf(*cu, absPartIdx, absPartIdxStep, trSizeC, trHeightC, TEXT_CHROMA_V, trMode, true);
        }

        m_entropyCoder.codeQtCbf(*cu, absPartIdx, TEXT_LUMA, trMode);
        if (numSigY)
            m_entropyCoder.codeCoeffNxN(*cu, coeffCurY, absPartIdx, log2TrSize, TEXT_LUMA);

        if (bCodeChroma)
        {
            if (!splitIntoSubTUs)
            {
                if (numSigU[0])
                    m_entropyCoder.codeCoeffNxN(*cu, coeffCurU, absPartIdx, log2TrSizeC, TEXT_CHROMA_U);
                if (numSigV[0])
                    m_entropyCoder.codeCoeffNxN(*cu, coeffCurV, absPartIdx, log2TrSizeC, TEXT_CHROMA_V);
            }
            else
            {
                uint32_t subTUSize = 1 << (log2TrSizeC * 2);
                uint32_t partIdxesPerSubTU = absPartIdxStep >> 1;

                if (numSigU[0])
                    m_entropyCoder.codeCoeffNxN(*cu, coeffCurU, absPartIdx, log2TrSizeC, TEXT_CHROMA_U);
                if (numSigU[1])
                    m_entropyCoder.codeCoeffNxN(*cu, coeffCurU + subTUSize, absPartIdx + partIdxesPerSubTU, log2TrSizeC, TEXT_CHROMA_U);
                if (numSigV[0])
                    m_entropyCoder.codeCoeffNxN(*cu, coeffCurV, absPartIdx, log2TrSizeC, TEXT_CHROMA_V);
                if (numSigV[1])
                    m_entropyCoder.codeCoeffNxN(*cu, coeffCurV + subTUSize, absPartIdx + partIdxesPerSubTU, log2TrSizeC, TEXT_CHROMA_V);
            }
        }

        singleDist += singleDistComp[TEXT_LUMA][0];
        singlePsyEnergy += singlePsyEnergyComp[TEXT_LUMA][0];// need to check we need to add chroma also
        for (uint32_t subTUIndex = 0; subTUIndex < 2; subTUIndex++)
        {
            singleDist += singleDistComp[TEXT_CHROMA_U][subTUIndex];
            singleDist += singleDistComp[TEXT_CHROMA_V][subTUIndex];
        }

        singleBits = m_entropyCoder.getNumberOfWrittenBits();
        if (m_rdCost.m_psyRd)
            singleCost = m_rdCost.calcPsyRdCost(singleDist, singleBits, singlePsyEnergy);
        else
            singleCost = m_rdCost.calcRdCost(singleDist, singleBits);

        bestCBF[TEXT_LUMA] = cu->getCbf(absPartIdx, TEXT_LUMA, trMode);
        if (bCodeChroma)
        {
            for (uint32_t chromId = TEXT_CHROMA_U; chromId <= TEXT_CHROMA_V; chromId++)
            {
                bestCBF[chromId] = cu->getCbf(absPartIdx, (TextType)chromId, trMode);
                if (splitIntoSubTUs)
                {
                    uint32_t partIdxesPerSubTU = absPartIdxStep >> 1;
                    for (uint32_t subTU = 0; subTU < 2; subTU++)
                        bestsubTUCBF[chromId][subTU] = cu->getCbf((absPartIdx + (subTU * partIdxesPerSubTU)), (TextType)chromId, subTUDepth);
                }
            }
        }
    }

    // code sub-blocks
    if (bCheckSplit)
    {
        if (bCheckFull)
        {
            m_entropyCoder.store(m_rqt[depth].rqtTest);
            m_entropyCoder.load(m_rqt[depth].rqtRoot);
        }
        uint32_t subdivDist = 0;
        uint32_t subdivBits = 0;
        uint64_t subDivCost = 0;
        uint32_t subDivPsyEnergy = 0;
        bestCBF[TEXT_LUMA] = cu->getCbf(absPartIdx, TEXT_LUMA, trMode);
        if (bCodeChroma)
        {
            for (uint32_t chromId = TEXT_CHROMA_U; chromId <= TEXT_CHROMA_V; chromId++)
            {
                bestCBF[chromId] = cu->getCbf(absPartIdx, (TextType)chromId, trMode);
                if (splitIntoSubTUs)
                {
                    uint32_t partIdxesPerSubTU = absPartIdxStep >> 1;
                    for (uint32_t subTU = 0; subTU < 2; subTU++)
                        bestsubTUCBF[chromId][subTU] = cu->getCbf((absPartIdx + (subTU * partIdxesPerSubTU)), (TextType)chromId, subTUDepth);
                }
            }
        }

        const uint32_t qPartNumSubdiv = NUM_CU_PARTITIONS >> ((depth + 1) << 1);
        for (uint32_t i = 0; i < 4; ++i)
        {
            mode.psyEnergy = 0;
            subdivDist += xEstimateResidualQT(mode, cuGeom, absPartIdx + i * qPartNumSubdiv, resiYuv, depth + 1, subDivCost, subdivBits, depthRange);
            subDivPsyEnergy += mode.psyEnergy;
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
            cu->m_cbf[0][absPartIdx + i] |= ycbf << trMode;
            cu->m_cbf[1][absPartIdx + i] |= ucbf << trMode;
            cu->m_cbf[2][absPartIdx + i] |= vcbf << trMode;
        }

        m_entropyCoder.load(m_rqt[depth].rqtRoot);
        m_entropyCoder.resetBits();

        xEncodeResidualQT(cu, absPartIdx, depth, true,  TEXT_LUMA, depthRange);
        xEncodeResidualQT(cu, absPartIdx, depth, false, TEXT_LUMA, depthRange);
        xEncodeResidualQT(cu, absPartIdx, depth, false, TEXT_CHROMA_U, depthRange);
        xEncodeResidualQT(cu, absPartIdx, depth, false, TEXT_CHROMA_V, depthRange);

        subdivBits = m_entropyCoder.getNumberOfWrittenBits();

        if (m_rdCost.m_psyRd)
            subDivCost = m_rdCost.calcPsyRdCost(subdivDist, subdivBits, subDivPsyEnergy);
        else
            subDivCost = m_rdCost.calcRdCost(subdivDist, subdivBits);

        if (ycbf || ucbf || vcbf || !bCheckFull)
        {
            if (subDivCost < singleCost)
            {
                rdCost += subDivCost;
                outBits += subdivBits;
                outDist += subdivDist;
                mode.psyEnergy = subDivPsyEnergy;
                return outDist;
            }
            else
                mode.psyEnergy = singlePsyEnergy;
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
        m_entropyCoder.load(m_rqt[depth].rqtTest);
    }

    rdCost += singleCost;
    outBits += singleBits;
    outDist += singleDist;
    mode.psyEnergy = singlePsyEnergy;

    cu->setTrIdxSubParts(trMode, absPartIdx, depth);
    cu->setCbfSubParts(numSigY ? setCbf : 0, TEXT_LUMA, absPartIdx, depth);

    if (bCodeChroma)
    {
        const uint32_t numberOfSections  = splitIntoSubTUs ? 2 : 1;
        uint32_t partIdxesPerSubTU  = absPartIdxStep >> (splitIntoSubTUs ? 1 : 0);

        for (uint32_t chromaId = TEXT_CHROMA_U; chromaId <= TEXT_CHROMA_V; chromaId++)
        {
            for (uint32_t subTUIndex = 0; subTUIndex < numberOfSections; subTUIndex++)
            {
                const uint32_t  subTUPartIdx = absPartIdx + (subTUIndex * partIdxesPerSubTU);

                if (splitIntoSubTUs)
                {
                    const uint8_t combinedCBF = (uint8_t)((bestsubTUCBF[chromaId][subTUIndex] << subTUDepth) | (bestCBF[chromaId] << trMode));
                    cu->setCbfPartRange(combinedCBF, (TextType)chromaId, subTUPartIdx, partIdxesPerSubTU);
                }
                else
                    cu->setCbfPartRange((bestCBF[chromaId] << trMode), (TextType)chromaId, subTUPartIdx, partIdxesPerSubTU);
            }
        }
    }

    return outDist;
}

void Search::xEncodeResidualQT(CUData* cu, uint32_t absPartIdx, const uint32_t depth, bool bSubdivAndCbf, TextType ttype, uint32_t depthRange[2])
{
    X265_CHECK(cu->m_depth[0] == cu->m_depth[absPartIdx], "depth not matching\n");
    const uint32_t curTrMode   = depth - cu->m_depth[0];
    const uint32_t trMode      = cu->m_trIdx[absPartIdx];
    const bool     bSubdiv     = curTrMode != trMode;
    const uint32_t log2TrSize  = g_maxLog2CUSize - depth;
    int hChromaShift = CHROMA_H_SHIFT(m_csp);
    int vChromaShift = CHROMA_V_SHIFT(m_csp);

    uint32_t log2TrSizeC = log2TrSize - hChromaShift;
    
    const bool splitIntoSubTUs = (m_csp == X265_CSP_I422);

    if (bSubdivAndCbf && log2TrSize <= depthRange[1] && log2TrSize > depthRange[0])
        m_entropyCoder.codeTransformSubdivFlag(bSubdiv, 5 - log2TrSize);

    X265_CHECK(cu->m_predMode[absPartIdx] != MODE_INTRA, "xEncodeResidualQT() with intra block\n");

    bool mCodeAll = true;
    uint32_t trWidthC  = 1 << log2TrSizeC;
    uint32_t trHeightC = splitIntoSubTUs ? (trWidthC << 1) : trWidthC;

    const uint32_t numPels = trWidthC * trHeightC;
    if (numPels < (MIN_TU_SIZE * MIN_TU_SIZE))
        mCodeAll = false;

    if (bSubdivAndCbf)
    {
        const bool bFirstCbfOfCU = curTrMode == 0;
        if (bFirstCbfOfCU || mCodeAll)
        {
            uint32_t absPartIdxStep = NUM_CU_PARTITIONS >> ((cu->m_depth[0] +  curTrMode) << 1);
            if (bFirstCbfOfCU || cu->getCbf(absPartIdx, TEXT_CHROMA_U, curTrMode - 1))
                m_entropyCoder.codeQtCbf(*cu, absPartIdx, absPartIdxStep, trWidthC, trHeightC, TEXT_CHROMA_U, curTrMode, !bSubdiv);
            if (bFirstCbfOfCU || cu->getCbf(absPartIdx, TEXT_CHROMA_V, curTrMode - 1))
                m_entropyCoder.codeQtCbf(*cu, absPartIdx, absPartIdxStep, trWidthC, trHeightC, TEXT_CHROMA_V, curTrMode, !bSubdiv);
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
        const uint32_t qtLayer = log2TrSize - 2;
        uint32_t coeffOffsetY = absPartIdx << (LOG2_UNIT_SIZE * 2);
        coeff_t* coeffCurY = m_rqt[qtLayer].tmpCoeff[0] + coeffOffsetY;

        //Chroma
        bool bCodeChroma = true;
        uint32_t trModeC = trMode;
        if ((log2TrSize == 2) && !(m_csp == X265_CSP_I444))
        {
            log2TrSizeC++;
            trModeC--;
            uint32_t qpdiv = NUM_CU_PARTITIONS >> ((depth - 1) << 1);
            bCodeChroma = ((absPartIdx & (qpdiv - 1)) == 0);
        }

        if (bSubdivAndCbf)
            m_entropyCoder.codeQtCbf(*cu, absPartIdx, TEXT_LUMA, trMode);
        else
        {
            if (ttype == TEXT_LUMA && cu->getCbf(absPartIdx, TEXT_LUMA, trMode))
                m_entropyCoder.codeCoeffNxN(*cu, coeffCurY, absPartIdx, log2TrSize, TEXT_LUMA);

            if (bCodeChroma)
            {
                uint32_t coeffOffsetC = coeffOffsetY >> (hChromaShift + vChromaShift);
                coeff_t* coeffCurU = m_rqt[qtLayer].tmpCoeff[1] + coeffOffsetC;
                coeff_t* coeffCurV = m_rqt[qtLayer].tmpCoeff[2] + coeffOffsetC;

                if (!splitIntoSubTUs)
                {
                    if (ttype == TEXT_CHROMA_U && cu->getCbf(absPartIdx, TEXT_CHROMA_U, trMode))
                        m_entropyCoder.codeCoeffNxN(*cu, coeffCurU, absPartIdx, log2TrSizeC, TEXT_CHROMA_U);
                    if (ttype == TEXT_CHROMA_V && cu->getCbf(absPartIdx, TEXT_CHROMA_V, trMode))
                        m_entropyCoder.codeCoeffNxN(*cu, coeffCurV, absPartIdx, log2TrSizeC, TEXT_CHROMA_V);
                }
                else
                {
                    uint32_t partIdxesPerSubTU  = NUM_CU_PARTITIONS >> (((cu->m_depth[absPartIdx] + trModeC) << 1) + 1);
                    uint32_t subTUSize = 1 << (log2TrSizeC * 2);
                    if (ttype == TEXT_CHROMA_U && cu->getCbf(absPartIdx, TEXT_CHROMA_U, trMode))
                    {
                        if (cu->getCbf(absPartIdx, ttype, trMode + 1))
                            m_entropyCoder.codeCoeffNxN(*cu, coeffCurU, absPartIdx, log2TrSizeC, TEXT_CHROMA_U);
                        if (cu->getCbf(absPartIdx + partIdxesPerSubTU, ttype, trMode + 1))
                            m_entropyCoder.codeCoeffNxN(*cu, coeffCurU + subTUSize, absPartIdx + partIdxesPerSubTU, log2TrSizeC, TEXT_CHROMA_U);
                    }
                    if (ttype == TEXT_CHROMA_V && cu->getCbf(absPartIdx, TEXT_CHROMA_V, trMode))
                    {
                        if (cu->getCbf(absPartIdx, ttype, trMode + 1))
                            m_entropyCoder.codeCoeffNxN(*cu, coeffCurV, absPartIdx, log2TrSizeC, TEXT_CHROMA_V);
                        if (cu->getCbf(absPartIdx + partIdxesPerSubTU, ttype, trMode + 1))
                            m_entropyCoder.codeCoeffNxN(*cu, coeffCurV + subTUSize, absPartIdx + partIdxesPerSubTU, log2TrSizeC, TEXT_CHROMA_V);
                    }
                }
            }
        }
    }
    else
    {
        if (bSubdivAndCbf || cu->getCbf(absPartIdx, ttype, curTrMode))
        {
            const uint32_t qpartNumSubdiv = NUM_CU_PARTITIONS >> ((depth + 1) << 1);
            for (uint32_t i = 0; i < 4; ++i)
                xEncodeResidualQT(cu, absPartIdx + i * qpartNumSubdiv, depth + 1, bSubdivAndCbf, ttype, depthRange);
        }
    }
}

void Search::saveResidualQTData(CUData& cu, ShortYuv& resiYuv, uint32_t absPartIdx, uint32_t depth)
{
    X265_CHECK(cu.m_depth[0] == cu.m_depth[absPartIdx], "depth not matching\n");
    const uint32_t curTrMode = depth - cu.m_depth[0];
    const uint32_t trMode    = cu.m_trIdx[absPartIdx];

    if (curTrMode < trMode)
    {
        uint32_t qPartNumSubdiv = NUM_CU_PARTITIONS >> ((depth + 1) << 1);
        for (uint32_t i = 0; i < 4; i++, absPartIdx += qPartNumSubdiv)
            saveResidualQTData(cu, resiYuv, absPartIdx, depth + 1);
        return;
    }

    int hChromaShift = CHROMA_H_SHIFT(m_csp);
    int vChromaShift = CHROMA_V_SHIFT(m_csp);

    const uint32_t log2TrSize = g_maxLog2CUSize - depth;
    const uint32_t qtLayer = log2TrSize - 2;

    uint32_t log2TrSizeC = log2TrSize - hChromaShift;
    bool bCodeChroma = true;
    uint32_t trModeC = trMode;
    if (log2TrSize == 2 && m_csp != X265_CSP_I444)
    {
        log2TrSizeC++;
        trModeC--;
        uint32_t qpdiv = NUM_CU_PARTITIONS >> ((cu.m_depth[0] + trModeC) << 1);
        bCodeChroma = ((absPartIdx & (qpdiv - 1)) == 0);
    }

    m_rqt[qtLayer].tmpShortYuv.copyPartToPartLuma(resiYuv, absPartIdx, log2TrSize);

    uint32_t numCoeffY = 1 << (log2TrSize * 2);
    uint32_t coeffOffsetY = absPartIdx << LOG2_UNIT_SIZE * 2;
    coeff_t* coeffSrcY = m_rqt[qtLayer].tmpCoeff[0] + coeffOffsetY;
    coeff_t* coeffDstY = cu.m_trCoeff[0] + coeffOffsetY;
    memcpy(coeffDstY, coeffSrcY, sizeof(coeff_t) * numCoeffY);

    if (bCodeChroma)
    {
        m_rqt[qtLayer].tmpShortYuv.copyPartToPartChroma(resiYuv, absPartIdx, log2TrSizeC + hChromaShift);

        uint32_t numCoeffC = 1 << (log2TrSizeC * 2 + (m_csp == X265_CSP_I422));
        uint32_t coeffOffsetC = coeffOffsetY >> (hChromaShift + vChromaShift);

        coeff_t* coeffSrcU = m_rqt[qtLayer].tmpCoeff[1] + coeffOffsetC;
        coeff_t* coeffSrcV = m_rqt[qtLayer].tmpCoeff[2] + coeffOffsetC;
        coeff_t* coeffDstU = cu.m_trCoeff[1] + coeffOffsetC;
        coeff_t* coeffDstV = cu.m_trCoeff[2] + coeffOffsetC;
        memcpy(coeffDstU, coeffSrcU, sizeof(coeff_t) * numCoeffC);
        memcpy(coeffDstV, coeffSrcV, sizeof(coeff_t) * numCoeffC);
    }
}

uint32_t Search::getIntraModeBits(CUData& cu, uint32_t mode, uint32_t absPartIdx, uint32_t depth)
{
    cu.m_lumaIntraDir[absPartIdx] = (uint8_t)mode;

    // Reload only contexts required for coding intra mode information
    m_entropyCoder.loadIntraDirModeLuma(m_rqt[depth].cur);
    m_entropyCoder.resetBits();
    m_entropyCoder.codeIntraDirLumaAng(cu, absPartIdx, false); /* TODO: Pass mode here so this func can take const cu ref */
    return m_entropyCoder.getNumberOfWrittenBits();
}

/* returns the number of bits required to signal a non-most-probable mode.
 * on return mpms contains bitmap of most probable modes */
uint32_t Search::getIntraRemModeBits(CUData& cu, uint32_t absPartIdx, uint32_t depth, uint32_t preds[3], uint64_t& mpms)
{
    mpms = 0;
    for (int i = 0; i < 3; ++i)
        mpms |= ((uint64_t)1 << preds[i]);

    uint32_t mode = 34;
    while (mpms & ((uint64_t)1 << mode))
        --mode;

    return getIntraModeBits(cu, mode, absPartIdx, depth);
}

/* swap the current mode/cost with the mode with the highest cost in the
 * current candidate list, if its cost is better (maintain a top N list) */
void Search::updateCandList(uint32_t mode, uint64_t cost, int maxCandCount, uint32_t* candModeList, uint64_t* candCostList)
{
    uint32_t maxIndex = 0;
    uint64_t maxValue = 0;

    for (int i = 0; i < maxCandCount; i++)
    {
        if (maxValue < candCostList[i])
        {
            maxValue = candCostList[i];
            maxIndex = i;
        }
    }

    if (cost < maxValue)
    {
        candCostList[maxIndex] = cost;
        candModeList[maxIndex] = mode;
    }
}
