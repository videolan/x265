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

#include "search.h"
#include "entropy.h"
#include "rdcost.h"

#include "analysis.h"  // TLD
#include "framedata.h"

using namespace x265;

#if _MSC_VER
#pragma warning(disable: 4800) // 'uint8_t' : forcing value to bool 'true' or 'false' (performance warning)
#pragma warning(disable: 4244) // '=' : conversion from 'int' to 'uint8_t', possible loss of data)
#pragma warning(disable: 4127) // conditional expression is constant
#endif

#define MVP_IDX_BITS 1

ALIGN_VAR_32(const int16_t, Search::zeroShort[MAX_CU_SIZE]) = { 0 };

Search::Search()
{
    memset(m_rqt, 0, sizeof(m_rqt));

    for (int i = 0; i < 3; i++)
    {
        m_qtTempTransformSkipFlag[i] = NULL;
        m_qtTempCbf[i] = NULL;
    }

    m_numLayers = 0;
    m_intraPred = NULL;
    m_intraPredAngs = NULL;
    m_fencScaled = NULL;
    m_fencTransposed = NULL;
    m_tsCoeff = NULL;
    m_tsResidual = NULL;
    m_tsRecon = NULL;
    m_param = NULL;
    m_slice = NULL;
    m_frame = NULL;
}

bool Search::initSearch(const x265_param& param, ScalingList& scalingList)
{
    uint32_t maxLog2CUSize = g_log2Size[param.maxCUSize];
    m_param = &param;
    m_bEnableRDOQ = !!param.rdoqLevel;
    m_bFrameParallel = param.frameNumThreads > 1;
    m_numLayers = g_log2Size[param.maxCUSize] - 2;

    m_rdCost.setPsyRdScale(param.psyRd);
    m_me.init(param.searchMethod, param.subpelRefine, param.internalCsp);

    bool ok = m_quant.init(param.rdoqLevel, param.psyRdoq, scalingList, m_entropyCoder);
    if (m_param->noiseReductionIntra || m_param->noiseReductionInter)
        ok &= m_quant.allocNoiseReduction(param);

    ok &= Predict::allocBuffers(param.internalCsp); /* sets m_hChromaShift & m_vChromaShift */

    /* When frame parallelism is active, only 'refLagPixels' of reference frames will be guaranteed
     * available for motion reference.  See refLagRows in FrameEncoder::compressCTURows() */
    m_refLagPixels = m_bFrameParallel ? param.searchRange : param.sourceHeight;

    uint32_t sizeL = 1 << (maxLog2CUSize * 2);
    uint32_t sizeC = sizeL >> (m_hChromaShift + m_vChromaShift);
    uint32_t numPartitions = 1 << (maxLog2CUSize - LOG2_UNIT_SIZE) * 2;

    /* these are indexed by qtLayer (log2size - 2) so nominally 0=4x4, 1=8x8, 2=16x16, 3=32x32
     * the coeffRQT and reconQtYuv are allocated to the max CU size at every depth. The parts
     * which are reconstructed at each depth are valid. At the end, the transform depth table
     * is walked and the coeff and recon at the correct depths are collected */
    for (uint32_t i = 0; i <= m_numLayers; i++)
    {
        CHECKED_MALLOC(m_rqt[i].coeffRQT[0], coeff_t, sizeL + sizeC * 2);
        m_rqt[i].coeffRQT[1] = m_rqt[i].coeffRQT[0] + sizeL;
        m_rqt[i].coeffRQT[2] = m_rqt[i].coeffRQT[0] + sizeL + sizeC;
        ok &= m_rqt[i].reconQtYuv.create(g_maxCUSize, param.internalCsp);
        ok &= m_rqt[i].resiQtYuv.create(g_maxCUSize, param.internalCsp);
    }

    /* the rest of these buffers are indexed per-depth */
    for (uint32_t i = 0; i <= g_maxCUDepth; i++)
    {
        int cuSize = g_maxCUSize >> i;
        ok &= m_rqt[i].tmpResiYuv.create(cuSize, param.internalCsp);
        ok &= m_rqt[i].tmpPredYuv.create(cuSize, param.internalCsp);
        ok &= m_rqt[i].bidirPredYuv[0].create(cuSize, param.internalCsp);
        ok &= m_rqt[i].bidirPredYuv[1].create(cuSize, param.internalCsp);
    }

    CHECKED_MALLOC(m_qtTempCbf[0], uint8_t, numPartitions * 3);
    m_qtTempCbf[1] = m_qtTempCbf[0] + numPartitions;
    m_qtTempCbf[2] = m_qtTempCbf[0] + numPartitions * 2;
    CHECKED_MALLOC(m_qtTempTransformSkipFlag[0], uint8_t, numPartitions * 3);
    m_qtTempTransformSkipFlag[1] = m_qtTempTransformSkipFlag[0] + numPartitions;
    m_qtTempTransformSkipFlag[2] = m_qtTempTransformSkipFlag[0] + numPartitions * 2;

    CHECKED_MALLOC(m_intraPred, pixel, (32 * 32) * (33 + 3));
    m_fencScaled = m_intraPred + 32 * 32;
    m_fencTransposed = m_fencScaled + 32 * 32;
    m_intraPredAngs = m_fencTransposed + 32 * 32;

    CHECKED_MALLOC(m_tsCoeff,    coeff_t, MAX_TS_SIZE * MAX_TS_SIZE);
    CHECKED_MALLOC(m_tsResidual, int16_t, MAX_TS_SIZE * MAX_TS_SIZE);
    CHECKED_MALLOC(m_tsRecon,    pixel,   MAX_TS_SIZE * MAX_TS_SIZE);

    return ok;

fail:
    return false;
}

Search::~Search()
{
    for (uint32_t i = 0; i <= m_numLayers; i++)
    {
        X265_FREE(m_rqt[i].coeffRQT[0]);
        m_rqt[i].reconQtYuv.destroy();
        m_rqt[i].resiQtYuv.destroy();
    }

    for (uint32_t i = 0; i <= g_maxCUDepth; i++)
    {
        m_rqt[i].tmpResiYuv.destroy();
        m_rqt[i].tmpPredYuv.destroy();
        m_rqt[i].bidirPredYuv[0].destroy();
        m_rqt[i].bidirPredYuv[1].destroy();
    }

    X265_FREE(m_qtTempCbf[0]);
    X265_FREE(m_qtTempTransformSkipFlag[0]);
    X265_FREE(m_intraPred);
    X265_FREE(m_tsCoeff);
    X265_FREE(m_tsResidual);
    X265_FREE(m_tsRecon);
}

void Search::setQP(const Slice& slice, int qp)
{
    x265_emms(); /* TODO: if the lambda tables were ints, this would not be necessary */
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

void Search::codeSubdivCbfQTChroma(const CUData& cu, uint32_t tuDepth, uint32_t absPartIdx)
{
    uint32_t subdiv     = tuDepth < cu.m_tuDepth[absPartIdx];
    uint32_t log2TrSize = cu.m_log2CUSize[0] - tuDepth;

    if (!(log2TrSize - m_hChromaShift < 2))
    {
        if (!tuDepth || cu.getCbf(absPartIdx, TEXT_CHROMA_U, tuDepth - 1))
            m_entropyCoder.codeQtCbfChroma(cu, absPartIdx, TEXT_CHROMA_U, tuDepth, !subdiv);
        if (!tuDepth || cu.getCbf(absPartIdx, TEXT_CHROMA_V, tuDepth - 1))
            m_entropyCoder.codeQtCbfChroma(cu, absPartIdx, TEXT_CHROMA_V, tuDepth, !subdiv);
    }

    if (subdiv)
    {
        uint32_t qNumParts = 1 << (log2TrSize - 1 - LOG2_UNIT_SIZE) * 2;
        for (uint32_t qIdx = 0; qIdx < 4; ++qIdx, absPartIdx += qNumParts)
            codeSubdivCbfQTChroma(cu, tuDepth + 1, absPartIdx);
    }
}

void Search::codeCoeffQTChroma(const CUData& cu, uint32_t tuDepth, uint32_t absPartIdx, TextType ttype)
{
    if (!cu.getCbf(absPartIdx, ttype, tuDepth))
        return;

    uint32_t log2TrSize = cu.m_log2CUSize[0] - tuDepth;

    if (tuDepth < cu.m_tuDepth[absPartIdx])
    {
        uint32_t qNumParts = 1 << (log2TrSize - 1 - LOG2_UNIT_SIZE) * 2;
        for (uint32_t qIdx = 0; qIdx < 4; ++qIdx, absPartIdx += qNumParts)
            codeCoeffQTChroma(cu, tuDepth + 1, absPartIdx, ttype);

        return;
    }

    uint32_t tuDepthC = tuDepth;
    uint32_t log2TrSizeC = log2TrSize - m_hChromaShift;

    if (log2TrSizeC < 2)
    {
        X265_CHECK(log2TrSize == 2 && m_csp != X265_CSP_I444 && tuDepth, "invalid tuDepth\n");
        if (absPartIdx & 3)
            return;
        log2TrSizeC = 2;
        tuDepthC--;
    }

    uint32_t qtLayer = log2TrSize - 2;

    if (m_csp != X265_CSP_I422)
    {
        uint32_t shift = (m_csp == X265_CSP_I420) ? 2 : 0;
        uint32_t coeffOffset = absPartIdx << (LOG2_UNIT_SIZE * 2 - shift);
        coeff_t* coeff = m_rqt[qtLayer].coeffRQT[ttype] + coeffOffset;
        m_entropyCoder.codeCoeffNxN(cu, coeff, absPartIdx, log2TrSizeC, ttype);
    }
    else
    {
        uint32_t coeffOffset = absPartIdx << (LOG2_UNIT_SIZE * 2 - 1);
        coeff_t* coeff = m_rqt[qtLayer].coeffRQT[ttype] + coeffOffset;
        uint32_t subTUSize = 1 << (log2TrSizeC * 2);
        uint32_t tuNumParts = 2 << ((log2TrSizeC - LOG2_UNIT_SIZE) * 2);
        if (cu.getCbf(absPartIdx, ttype, tuDepth + 1))
            m_entropyCoder.codeCoeffNxN(cu, coeff, absPartIdx, log2TrSizeC, ttype);
        if (cu.getCbf(absPartIdx + tuNumParts, ttype, tuDepth + 1))
            m_entropyCoder.codeCoeffNxN(cu, coeff + subTUSize, absPartIdx + tuNumParts, log2TrSizeC, ttype);
    }
}

void Search::codeIntraLumaQT(Mode& mode, const CUGeom& cuGeom, uint32_t tuDepth, uint32_t absPartIdx, bool bAllowSplit, Cost& outCost, const uint32_t depthRange[2])
{
    CUData& cu = mode.cu;
    uint32_t fullDepth  = cuGeom.depth + tuDepth;
    uint32_t log2TrSize = cuGeom.log2CUSize - tuDepth;
    uint32_t qtLayer    = log2TrSize - 2;
    uint32_t sizeIdx    = log2TrSize - 2;
    bool mightNotSplit  = log2TrSize <= depthRange[1];
    bool mightSplit     = (log2TrSize > depthRange[0]) && (bAllowSplit || !mightNotSplit);

    /* If maximum RD penalty, force spits at TU size 32x32 if SPS allows TUs of 16x16 */
    if (m_param->rdPenalty == 2 && m_slice->m_sliceType != I_SLICE && log2TrSize == 5 && depthRange[0] <= 4)
    {
        mightNotSplit = false;
        mightSplit = true;
    }

    Cost fullCost;
    uint32_t bCBF = 0;

    pixel*   reconQt = m_rqt[qtLayer].reconQtYuv.getLumaAddr(absPartIdx);
    uint32_t reconQtStride = m_rqt[qtLayer].reconQtYuv.m_size;

    if (mightNotSplit)
    {
        if (mightSplit)
            m_entropyCoder.store(m_rqt[fullDepth].rqtRoot);

        const pixel* fenc = mode.fencYuv->getLumaAddr(absPartIdx);
        pixel*   pred     = mode.predYuv.getLumaAddr(absPartIdx);
        int16_t* residual = m_rqt[cuGeom.depth].tmpResiYuv.getLumaAddr(absPartIdx);
        uint32_t stride   = mode.fencYuv->m_size;

        // init availability pattern
        uint32_t lumaPredMode = cu.m_lumaIntraDir[absPartIdx];
        IntraNeighbors intraNeighbors;
        initIntraNeighbors(cu, absPartIdx, tuDepth, true, &intraNeighbors);
        initAdiPattern(cu, cuGeom, absPartIdx, intraNeighbors, lumaPredMode);

        // get prediction signal
        predIntraLumaAng(lumaPredMode, pred, stride, log2TrSize);

        cu.setTransformSkipSubParts(0, TEXT_LUMA, absPartIdx, fullDepth);
        cu.setTUDepthSubParts(tuDepth, absPartIdx, fullDepth);

        uint32_t coeffOffsetY = absPartIdx << (LOG2_UNIT_SIZE * 2);
        coeff_t* coeffY       = m_rqt[qtLayer].coeffRQT[0] + coeffOffsetY;

        // store original entropy coding status
        if (m_bEnableRDOQ)
            m_entropyCoder.estBit(m_entropyCoder.m_estBitsSbac, log2TrSize, true);

        primitives.cu[sizeIdx].calcresidual(fenc, pred, residual, stride);

        uint32_t numSig = m_quant.transformNxN(cu, fenc, stride, residual, stride, coeffY, log2TrSize, TEXT_LUMA, absPartIdx, false);
        if (numSig)
        {
            m_quant.invtransformNxN(residual, stride, coeffY, log2TrSize, TEXT_LUMA, true, false, numSig);
            primitives.cu[sizeIdx].add_ps(reconQt, reconQtStride, pred, residual, stride, stride);
        }
        else
            // no coded residual, recon = pred
            primitives.cu[sizeIdx].copy_pp(reconQt, reconQtStride, pred, stride);

        bCBF = !!numSig << tuDepth;
        cu.setCbfSubParts(bCBF, TEXT_LUMA, absPartIdx, fullDepth);
        fullCost.distortion = primitives.cu[sizeIdx].sse_pp(reconQt, reconQtStride, fenc, stride);

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

            m_entropyCoder.codePartSize(cu, 0, cuGeom.depth);
        }
        if (cu.m_partSize[0] == SIZE_2Nx2N)
        {
            if (!absPartIdx)
                m_entropyCoder.codeIntraDirLumaAng(cu, 0, false);
        }
        else
        {
            uint32_t qNumParts = cuGeom.numPartitions >> 2;
            if (!tuDepth)
            {
                for (uint32_t qIdx = 0; qIdx < 4; ++qIdx)
                    m_entropyCoder.codeIntraDirLumaAng(cu, qIdx * qNumParts, false);
            }
            else if (!(absPartIdx & (qNumParts - 1)))
                m_entropyCoder.codeIntraDirLumaAng(cu, absPartIdx, false);
        }
        if (log2TrSize != depthRange[0])
            m_entropyCoder.codeTransformSubdivFlag(0, 5 - log2TrSize);

        m_entropyCoder.codeQtCbfLuma(!!numSig, tuDepth);

        if (cu.getCbf(absPartIdx, TEXT_LUMA, tuDepth))
            m_entropyCoder.codeCoeffNxN(cu, coeffY, absPartIdx, log2TrSize, TEXT_LUMA);

        fullCost.bits = m_entropyCoder.getNumberOfWrittenBits();

        if (m_param->rdPenalty && log2TrSize == 5 && m_slice->m_sliceType != I_SLICE)
            fullCost.bits *= 4;

        if (m_rdCost.m_psyRd)
        {
            fullCost.energy = m_rdCost.psyCost(sizeIdx, fenc, mode.fencYuv->m_size, reconQt, reconQtStride);
            fullCost.rdcost = m_rdCost.calcPsyRdCost(fullCost.distortion, fullCost.bits, fullCost.energy);
        }
        else
            fullCost.rdcost = m_rdCost.calcRdCost(fullCost.distortion, fullCost.bits);
    }
    else
        fullCost.rdcost = MAX_INT64;

    if (mightSplit)
    {
        if (mightNotSplit)
        {
            m_entropyCoder.store(m_rqt[fullDepth].rqtTest);  // save state after full TU encode
            m_entropyCoder.load(m_rqt[fullDepth].rqtRoot);   // prep state of split encode
        }

        /* code split block */
        uint32_t qNumParts = 1 << (log2TrSize - 1 - LOG2_UNIT_SIZE) * 2;

        int checkTransformSkip = m_slice->m_pps->bTransformSkipEnabled && (log2TrSize - 1) <= MAX_LOG2_TS_SIZE && !cu.m_tqBypass[0];
        if (m_param->bEnableTSkipFast)
            checkTransformSkip &= cu.m_partSize[0] != SIZE_2Nx2N;

        Cost splitCost;
        uint32_t cbf = 0;
        for (uint32_t qIdx = 0, qPartIdx = absPartIdx; qIdx < 4; ++qIdx, qPartIdx += qNumParts)
        {
            if (checkTransformSkip)
                codeIntraLumaTSkip(mode, cuGeom, tuDepth + 1, qPartIdx, splitCost);
            else
                codeIntraLumaQT(mode, cuGeom, tuDepth + 1, qPartIdx, bAllowSplit, splitCost, depthRange);

            cbf |= cu.getCbf(qPartIdx, TEXT_LUMA, tuDepth + 1);
        }
        for (uint32_t offs = 0; offs < 4 * qNumParts; offs++)
            cu.m_cbf[0][absPartIdx + offs] |= (cbf << tuDepth);

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
            // recover entropy state of full-size TU encode
            m_entropyCoder.load(m_rqt[fullDepth].rqtTest);

            // recover transform index and Cbf values
            cu.setTUDepthSubParts(tuDepth, absPartIdx, fullDepth);
            cu.setCbfSubParts(bCBF, TEXT_LUMA, absPartIdx, fullDepth);
            cu.setTransformSkipSubParts(0, TEXT_LUMA, absPartIdx, fullDepth);
        }
    }

    // set reconstruction for next intra prediction blocks if full TU prediction won
    pixel*   picReconY = m_frame->m_reconPic->getLumaAddr(cu.m_cuAddr, cuGeom.absPartIdx + absPartIdx);
    intptr_t picStride = m_frame->m_reconPic->m_stride;
    primitives.cu[sizeIdx].copy_pp(picReconY, picStride, reconQt, reconQtStride);

    outCost.rdcost     += fullCost.rdcost;
    outCost.distortion += fullCost.distortion;
    outCost.bits       += fullCost.bits;
    outCost.energy     += fullCost.energy;
}

void Search::codeIntraLumaTSkip(Mode& mode, const CUGeom& cuGeom, uint32_t tuDepth, uint32_t absPartIdx, Cost& outCost)
{
    uint32_t fullDepth = cuGeom.depth + tuDepth;
    uint32_t log2TrSize = cuGeom.log2CUSize - tuDepth;
    uint32_t tuSize = 1 << log2TrSize;

    X265_CHECK(tuSize <= MAX_TS_SIZE, "transform skip is only possible at 4x4 TUs\n");

    CUData& cu = mode.cu;
    Yuv* predYuv = &mode.predYuv;
    const Yuv* fencYuv = mode.fencYuv;

    Cost fullCost;
    fullCost.rdcost = MAX_INT64;
    int      bTSkip = 0;
    uint32_t bCBF = 0;

    const pixel* fenc = fencYuv->getLumaAddr(absPartIdx);
    pixel*   pred = predYuv->getLumaAddr(absPartIdx);
    int16_t* residual = m_rqt[cuGeom.depth].tmpResiYuv.getLumaAddr(absPartIdx);
    uint32_t stride = fencYuv->m_size;
    uint32_t sizeIdx = log2TrSize - 2;

    // init availability pattern
    uint32_t lumaPredMode = cu.m_lumaIntraDir[absPartIdx];
    IntraNeighbors intraNeighbors;
    initIntraNeighbors(cu, absPartIdx, tuDepth, true, &intraNeighbors);
    initAdiPattern(cu, cuGeom, absPartIdx, intraNeighbors, lumaPredMode);

    // get prediction signal
    predIntraLumaAng(lumaPredMode, pred, stride, log2TrSize);

    cu.setTUDepthSubParts(tuDepth, absPartIdx, fullDepth);

    uint32_t qtLayer = log2TrSize - 2;
    uint32_t coeffOffsetY = absPartIdx << (LOG2_UNIT_SIZE * 2);
    coeff_t* coeffY = m_rqt[qtLayer].coeffRQT[0] + coeffOffsetY;
    pixel*   reconQt = m_rqt[qtLayer].reconQtYuv.getLumaAddr(absPartIdx);
    uint32_t reconQtStride = m_rqt[qtLayer].reconQtYuv.m_size;

    // store original entropy coding status
    m_entropyCoder.store(m_rqt[fullDepth].rqtRoot);

    if (m_bEnableRDOQ)
        m_entropyCoder.estBit(m_entropyCoder.m_estBitsSbac, log2TrSize, true);

    int checkTransformSkip = 1;
    for (int useTSkip = 0; useTSkip <= checkTransformSkip; useTSkip++)
    {
        uint64_t tmpCost;
        uint32_t tmpEnergy = 0;

        coeff_t* coeff = (useTSkip ? m_tsCoeff : coeffY);
        pixel*   tmpRecon = (useTSkip ? m_tsRecon : reconQt);
        uint32_t tmpReconStride = (useTSkip ? MAX_TS_SIZE : reconQtStride);

        primitives.cu[sizeIdx].calcresidual(fenc, pred, residual, stride);

        uint32_t numSig = m_quant.transformNxN(cu, fenc, stride, residual, stride, coeff, log2TrSize, TEXT_LUMA, absPartIdx, useTSkip);
        if (numSig)
        {
            m_quant.invtransformNxN(residual, stride, coeff, log2TrSize, TEXT_LUMA, true, useTSkip, numSig);
            primitives.cu[sizeIdx].add_ps(tmpRecon, tmpReconStride, pred, residual, stride, stride);
        }
        else if (useTSkip)
        {
            /* do not allow tskip if CBF=0, pretend we did not try tskip */
            checkTransformSkip = 0;
            break;
        }
        else
            // no residual coded, recon = pred
            primitives.cu[sizeIdx].copy_pp(tmpRecon, tmpReconStride, pred, stride);

        uint32_t tmpDist = primitives.cu[sizeIdx].sse_pp(tmpRecon, tmpReconStride, fenc, stride);

        cu.setTransformSkipSubParts(useTSkip, TEXT_LUMA, absPartIdx, fullDepth);
        cu.setCbfSubParts((!!numSig) << tuDepth, TEXT_LUMA, absPartIdx, fullDepth);

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

            m_entropyCoder.codePartSize(cu, 0, cuGeom.depth);
        }
        if (cu.m_partSize[0] == SIZE_2Nx2N)
        {
            if (!absPartIdx)
                m_entropyCoder.codeIntraDirLumaAng(cu, 0, false);
        }
        else
        {
            uint32_t qNumParts = cuGeom.numPartitions >> 2;
            if (!tuDepth)
            {
                for (uint32_t qIdx = 0; qIdx < 4; ++qIdx)
                    m_entropyCoder.codeIntraDirLumaAng(cu, qIdx * qNumParts, false);
            }
            else if (!(absPartIdx & (qNumParts - 1)))
                m_entropyCoder.codeIntraDirLumaAng(cu, absPartIdx, false);
        }
        m_entropyCoder.codeTransformSubdivFlag(0, 5 - log2TrSize);

        m_entropyCoder.codeQtCbfLuma(!!numSig, tuDepth);

        if (cu.getCbf(absPartIdx, TEXT_LUMA, tuDepth))
            m_entropyCoder.codeCoeffNxN(cu, coeff, absPartIdx, log2TrSize, TEXT_LUMA);

        uint32_t tmpBits = m_entropyCoder.getNumberOfWrittenBits();

        if (!useTSkip)
            m_entropyCoder.store(m_rqt[fullDepth].rqtTemp);

        if (m_rdCost.m_psyRd)
        {
            tmpEnergy = m_rdCost.psyCost(sizeIdx, fenc, fencYuv->m_size, tmpRecon, tmpReconStride);
            tmpCost = m_rdCost.calcPsyRdCost(tmpDist, tmpBits, tmpEnergy);
        }
        else
            tmpCost = m_rdCost.calcRdCost(tmpDist, tmpBits);

        if (tmpCost < fullCost.rdcost)
        {
            bTSkip = useTSkip;
            bCBF = !!numSig;
            fullCost.rdcost = tmpCost;
            fullCost.distortion = tmpDist;
            fullCost.bits = tmpBits;
            fullCost.energy = tmpEnergy;
        }
    }

    if (bTSkip)
    {
        memcpy(coeffY, m_tsCoeff, sizeof(coeff_t) << (log2TrSize * 2));
        primitives.cu[sizeIdx].copy_pp(reconQt, reconQtStride, m_tsRecon, tuSize);
    }
    else if (checkTransformSkip)
    {
        cu.setTransformSkipSubParts(0, TEXT_LUMA, absPartIdx, fullDepth);
        cu.setCbfSubParts(bCBF << tuDepth, TEXT_LUMA, absPartIdx, fullDepth);
        m_entropyCoder.load(m_rqt[fullDepth].rqtTemp);
    }

    // set reconstruction for next intra prediction blocks
    pixel*   picReconY = m_frame->m_reconPic->getLumaAddr(cu.m_cuAddr, cuGeom.absPartIdx + absPartIdx);
    intptr_t picStride = m_frame->m_reconPic->m_stride;
    primitives.cu[sizeIdx].copy_pp(picReconY, picStride, reconQt, reconQtStride);

    outCost.rdcost += fullCost.rdcost;
    outCost.distortion += fullCost.distortion;
    outCost.bits += fullCost.bits;
    outCost.energy += fullCost.energy;
}

/* fast luma intra residual generation. Only perform the minimum number of TU splits required by the CU size */
void Search::residualTransformQuantIntra(Mode& mode, const CUGeom& cuGeom, uint32_t absPartIdx, uint32_t tuDepth, const uint32_t depthRange[2])
{
    CUData& cu = mode.cu;
    uint32_t fullDepth  = cuGeom.depth + tuDepth;
    uint32_t log2TrSize = cuGeom.log2CUSize - tuDepth;
    bool     bCheckFull = log2TrSize <= depthRange[1];

    X265_CHECK(m_slice->m_sliceType != I_SLICE, "residualTransformQuantIntra not intended for I slices\n");

    /* we still respect rdPenalty == 2, we can forbid 32x32 intra TU. rdPenalty = 1 is impossible
     * since we are not measuring RD cost */
    if (m_param->rdPenalty == 2 && log2TrSize == 5 && depthRange[0] <= 4)
        bCheckFull = false;

    if (bCheckFull)
    {
        const pixel* fenc = mode.fencYuv->getLumaAddr(absPartIdx);
        pixel*   pred     = mode.predYuv.getLumaAddr(absPartIdx);
        int16_t* residual = m_rqt[cuGeom.depth].tmpResiYuv.getLumaAddr(absPartIdx);
        uint32_t stride   = mode.fencYuv->m_size;

        // init availability pattern
        uint32_t lumaPredMode = cu.m_lumaIntraDir[absPartIdx];
        IntraNeighbors intraNeighbors;
        initIntraNeighbors(cu, absPartIdx, tuDepth, true, &intraNeighbors);
        initAdiPattern(cu, cuGeom, absPartIdx, intraNeighbors, lumaPredMode);

        // get prediction signal
        predIntraLumaAng(lumaPredMode, pred, stride, log2TrSize);

        X265_CHECK(!cu.m_transformSkip[TEXT_LUMA][absPartIdx], "unexpected tskip flag in residualTransformQuantIntra\n");
        cu.setTUDepthSubParts(tuDepth, absPartIdx, fullDepth);

        uint32_t coeffOffsetY = absPartIdx << (LOG2_UNIT_SIZE * 2);
        coeff_t* coeffY       = cu.m_trCoeff[0] + coeffOffsetY;

        uint32_t sizeIdx   = log2TrSize - 2;
        primitives.cu[sizeIdx].calcresidual(fenc, pred, residual, stride);

        pixel*   picReconY = m_frame->m_reconPic->getLumaAddr(cu.m_cuAddr, cuGeom.absPartIdx + absPartIdx);
        intptr_t picStride = m_frame->m_reconPic->m_stride;

        uint32_t numSig = m_quant.transformNxN(cu, fenc, stride, residual, stride, coeffY, log2TrSize, TEXT_LUMA, absPartIdx, false);
        if (numSig)
        {
            m_quant.invtransformNxN(residual, stride, coeffY, log2TrSize, TEXT_LUMA, true, false, numSig);
            primitives.cu[sizeIdx].add_ps(picReconY, picStride, pred, residual, stride, stride);
            cu.setCbfSubParts(1 << tuDepth, TEXT_LUMA, absPartIdx, fullDepth);
        }
        else
        {
            primitives.cu[sizeIdx].copy_pp(picReconY, picStride, pred, stride);
            cu.setCbfSubParts(0, TEXT_LUMA, absPartIdx, fullDepth);
        }
    }
    else
    {
        X265_CHECK(log2TrSize > depthRange[0], "intra luma split state failure\n");

        /* code split block */
        uint32_t qNumParts = 1 << (log2TrSize - 1 - LOG2_UNIT_SIZE) * 2;
        uint32_t cbf = 0;
        for (uint32_t qIdx = 0, qPartIdx = absPartIdx; qIdx < 4; ++qIdx, qPartIdx += qNumParts)
        {
            residualTransformQuantIntra(mode, cuGeom, qPartIdx, tuDepth + 1, depthRange);
            cbf |= cu.getCbf(qPartIdx, TEXT_LUMA, tuDepth + 1);
        }
        for (uint32_t offs = 0; offs < 4 * qNumParts; offs++)
            cu.m_cbf[0][absPartIdx + offs] |= (cbf << tuDepth);
    }
}

void Search::extractIntraResultQT(CUData& cu, Yuv& reconYuv, uint32_t tuDepth, uint32_t absPartIdx)
{
    uint32_t log2TrSize = cu.m_log2CUSize[0] - tuDepth;

    if (tuDepth == cu.m_tuDepth[absPartIdx])
    {
        uint32_t qtLayer    = log2TrSize - 2;

        // copy transform coefficients
        uint32_t coeffOffsetY = absPartIdx << (LOG2_UNIT_SIZE * 2);
        coeff_t* coeffSrcY    = m_rqt[qtLayer].coeffRQT[0] + coeffOffsetY;
        coeff_t* coeffDestY   = cu.m_trCoeff[0]            + coeffOffsetY;
        memcpy(coeffDestY, coeffSrcY, sizeof(coeff_t) << (log2TrSize * 2));

        // copy reconstruction
        m_rqt[qtLayer].reconQtYuv.copyPartToPartLuma(reconYuv, absPartIdx, log2TrSize);
    }
    else
    {
        uint32_t qNumParts = 1 << (log2TrSize - 1 - LOG2_UNIT_SIZE) * 2;
        for (uint32_t qIdx = 0; qIdx < 4; ++qIdx, absPartIdx += qNumParts)
            extractIntraResultQT(cu, reconYuv, tuDepth + 1, absPartIdx);
    }
}

inline void offsetCBFs(uint8_t subTUCBF[2])
{
    uint8_t combinedCBF = subTUCBF[0] | subTUCBF[1];
    subTUCBF[0] = subTUCBF[0] << 1 | combinedCBF;
    subTUCBF[1] = subTUCBF[1] << 1 | combinedCBF;
}

/* 4:2:2 post-TU split processing */
void Search::offsetSubTUCBFs(CUData& cu, TextType ttype, uint32_t tuDepth, uint32_t absPartIdx)
{
    uint32_t log2TrSize = cu.m_log2CUSize[0] - tuDepth;

    if (log2TrSize == 2)
    {
        X265_CHECK(m_csp != X265_CSP_I444 && tuDepth, "invalid tuDepth\n");
        ++log2TrSize;
    }

    uint32_t tuNumParts = 1 << ((log2TrSize - LOG2_UNIT_SIZE) * 2 - 1);

    // move the CBFs down a level and set the parent CBF
    uint8_t subTUCBF[2];
    subTUCBF[0] = cu.getCbf(absPartIdx            , ttype, tuDepth);
    subTUCBF[1] = cu.getCbf(absPartIdx+ tuNumParts, ttype, tuDepth);
    offsetCBFs(subTUCBF);

    cu.setCbfPartRange(subTUCBF[0] << tuDepth, ttype, absPartIdx             , tuNumParts);
    cu.setCbfPartRange(subTUCBF[1] << tuDepth, ttype, absPartIdx + tuNumParts, tuNumParts);
}

/* returns distortion */
uint32_t Search::codeIntraChromaQt(Mode& mode, const CUGeom& cuGeom, uint32_t tuDepth, uint32_t absPartIdx, uint32_t& psyEnergy)
{
    CUData& cu = mode.cu;
    uint32_t log2TrSize = cuGeom.log2CUSize - tuDepth;

    if (tuDepth < cu.m_tuDepth[absPartIdx])
    {
        uint32_t qNumParts = 1 << (log2TrSize - 1 - LOG2_UNIT_SIZE) * 2;
        uint32_t outDist = 0, splitCbfU = 0, splitCbfV = 0;
        for (uint32_t qIdx = 0, qPartIdx = absPartIdx; qIdx < 4; ++qIdx, qPartIdx += qNumParts)
        {
            outDist += codeIntraChromaQt(mode, cuGeom, tuDepth + 1, qPartIdx, psyEnergy);
            splitCbfU |= cu.getCbf(qPartIdx, TEXT_CHROMA_U, tuDepth + 1);
            splitCbfV |= cu.getCbf(qPartIdx, TEXT_CHROMA_V, tuDepth + 1);
        }
        for (uint32_t offs = 0; offs < 4 * qNumParts; offs++)
        {
            cu.m_cbf[1][absPartIdx + offs] |= (splitCbfU << tuDepth);
            cu.m_cbf[2][absPartIdx + offs] |= (splitCbfV << tuDepth);
        }

        return outDist;
    }

    uint32_t log2TrSizeC = log2TrSize - m_hChromaShift;
    uint32_t tuDepthC = tuDepth;
    if (log2TrSizeC < 2)
    {
        X265_CHECK(log2TrSize == 2 && m_csp != X265_CSP_I444 && tuDepth, "invalid tuDepth\n");
        if (absPartIdx & 3)
            return 0;
        log2TrSizeC = 2;
        tuDepthC--;
    }

    if (m_bEnableRDOQ)
        m_entropyCoder.estBit(m_entropyCoder.m_estBitsSbac, log2TrSizeC, false);

    bool checkTransformSkip = m_slice->m_pps->bTransformSkipEnabled && log2TrSizeC <= MAX_LOG2_TS_SIZE && !cu.m_tqBypass[0];
    checkTransformSkip &= !m_param->bEnableTSkipFast || (log2TrSize <= MAX_LOG2_TS_SIZE && cu.m_transformSkip[TEXT_LUMA][absPartIdx]);
    if (checkTransformSkip)
        return codeIntraChromaTSkip(mode, cuGeom, tuDepth, tuDepthC, absPartIdx, psyEnergy);

    ShortYuv& resiYuv = m_rqt[cuGeom.depth].tmpResiYuv;
    uint32_t qtLayer = log2TrSize - 2;
    uint32_t stride = mode.fencYuv->m_csize;
    const uint32_t sizeIdxC = log2TrSizeC - 2;
    uint32_t outDist = 0;

    uint32_t curPartNum = cuGeom.numPartitions >> tuDepthC * 2;
    const SplitType splitType = (m_csp == X265_CSP_I422) ? VERTICAL_SPLIT : DONT_SPLIT;

    TURecurse tuIterator(splitType, curPartNum, absPartIdx);
    do
    {
        uint32_t absPartIdxC = tuIterator.absPartIdxTURelCU;

        IntraNeighbors intraNeighbors;
        initIntraNeighbors(cu, absPartIdxC, tuDepthC, false, &intraNeighbors);

        for (uint32_t chromaId = TEXT_CHROMA_U; chromaId <= TEXT_CHROMA_V; chromaId++)
        {
            TextType ttype = (TextType)chromaId;

            const pixel* fenc = mode.fencYuv->getChromaAddr(chromaId, absPartIdxC);
            pixel*   pred     = mode.predYuv.getChromaAddr(chromaId, absPartIdxC);
            int16_t* residual = resiYuv.getChromaAddr(chromaId, absPartIdxC);
            uint32_t coeffOffsetC  = absPartIdxC << (LOG2_UNIT_SIZE * 2 - (m_hChromaShift + m_vChromaShift));
            coeff_t* coeffC        = m_rqt[qtLayer].coeffRQT[chromaId] + coeffOffsetC;
            pixel*   reconQt       = m_rqt[qtLayer].reconQtYuv.getChromaAddr(chromaId, absPartIdxC);
            uint32_t reconQtStride = m_rqt[qtLayer].reconQtYuv.m_csize;
            pixel*   picReconC = m_frame->m_reconPic->getChromaAddr(chromaId, cu.m_cuAddr, cuGeom.absPartIdx + absPartIdxC);
            intptr_t picStride = m_frame->m_reconPic->m_strideC;

            uint32_t chromaPredMode = cu.m_chromaIntraDir[absPartIdxC];
            if (chromaPredMode == DM_CHROMA_IDX)
                chromaPredMode = cu.m_lumaIntraDir[(m_csp == X265_CSP_I444) ? absPartIdxC : 0];
            if (m_csp == X265_CSP_I422)
                chromaPredMode = g_chroma422IntraAngleMappingTable[chromaPredMode];

            // init availability pattern
            initAdiPatternChroma(cu, cuGeom, absPartIdxC, intraNeighbors, chromaId);

            // get prediction signal
            predIntraChromaAng(chromaPredMode, pred, stride, log2TrSizeC);
            cu.setTransformSkipPartRange(0, ttype, absPartIdxC, tuIterator.absPartIdxStep);

            primitives.cu[sizeIdxC].calcresidual(fenc, pred, residual, stride);
            uint32_t numSig = m_quant.transformNxN(cu, fenc, stride, residual, stride, coeffC, log2TrSizeC, ttype, absPartIdxC, false);
            if (numSig)
            {
                m_quant.invtransformNxN(residual, stride, coeffC, log2TrSizeC, ttype, true, false, numSig);
                primitives.cu[sizeIdxC].add_ps(reconQt, reconQtStride, pred, residual, stride, stride);
                cu.setCbfPartRange(1 << tuDepth, ttype, absPartIdxC, tuIterator.absPartIdxStep);
            }
            else
            {
                // no coded residual, recon = pred
                primitives.cu[sizeIdxC].copy_pp(reconQt, reconQtStride, pred, stride);
                cu.setCbfPartRange(0, ttype, absPartIdxC, tuIterator.absPartIdxStep);
            }

            outDist += m_rdCost.scaleChromaDist(chromaId, primitives.cu[sizeIdxC].sse_pp(reconQt, reconQtStride, fenc, stride));

            if (m_rdCost.m_psyRd)
                psyEnergy += m_rdCost.psyCost(sizeIdxC, fenc, stride, reconQt, reconQtStride);

            primitives.cu[sizeIdxC].copy_pp(picReconC, picStride, reconQt, reconQtStride);
        }
    }
    while (tuIterator.isNextSection());

    if (splitType == VERTICAL_SPLIT)
    {
        offsetSubTUCBFs(cu, TEXT_CHROMA_U, tuDepth, absPartIdx);
        offsetSubTUCBFs(cu, TEXT_CHROMA_V, tuDepth, absPartIdx);
    }

    return outDist;
}

/* returns distortion */
uint32_t Search::codeIntraChromaTSkip(Mode& mode, const CUGeom& cuGeom, uint32_t tuDepth, uint32_t tuDepthC, uint32_t absPartIdx, uint32_t& psyEnergy)
{
    CUData& cu = mode.cu;
    uint32_t fullDepth  = cuGeom.depth + tuDepth;
    uint32_t log2TrSize = cuGeom.log2CUSize - tuDepth;
    const uint32_t log2TrSizeC = 2;
    uint32_t qtLayer = log2TrSize - 2;
    uint32_t outDist = 0;

    /* At the TU layers above this one, no RDO is performed, only distortion is being measured,
     * so the entropy coder is not very accurate. The best we can do is return it in the same
     * condition as it arrived, and to do all bit estimates from the same state. */
    m_entropyCoder.store(m_rqt[fullDepth].rqtRoot);

    uint32_t curPartNum = cuGeom.numPartitions >> tuDepthC * 2;
    const SplitType splitType = (m_csp == X265_CSP_I422) ? VERTICAL_SPLIT : DONT_SPLIT;

    TURecurse tuIterator(splitType, curPartNum, absPartIdx);
    do
    {
        uint32_t absPartIdxC = tuIterator.absPartIdxTURelCU;

        IntraNeighbors intraNeighbors;
        initIntraNeighbors(cu, absPartIdxC, tuDepthC, false, &intraNeighbors);

        for (uint32_t chromaId = TEXT_CHROMA_U; chromaId <= TEXT_CHROMA_V; chromaId++)
        {
            TextType ttype = (TextType)chromaId;

            const pixel* fenc = mode.fencYuv->getChromaAddr(chromaId, absPartIdxC);
            pixel*   pred = mode.predYuv.getChromaAddr(chromaId, absPartIdxC);
            int16_t* residual = m_rqt[cuGeom.depth].tmpResiYuv.getChromaAddr(chromaId, absPartIdxC);
            uint32_t stride = mode.fencYuv->m_csize;
            const uint32_t sizeIdxC = log2TrSizeC - 2;

            uint32_t coeffOffsetC = absPartIdxC << (LOG2_UNIT_SIZE * 2 - (m_hChromaShift + m_vChromaShift));
            coeff_t* coeffC = m_rqt[qtLayer].coeffRQT[chromaId] + coeffOffsetC;
            pixel*   reconQt = m_rqt[qtLayer].reconQtYuv.getChromaAddr(chromaId, absPartIdxC);
            uint32_t reconQtStride = m_rqt[qtLayer].reconQtYuv.m_csize;

            // init availability pattern
            initAdiPatternChroma(cu, cuGeom, absPartIdxC, intraNeighbors, chromaId);

            uint32_t chromaPredMode = cu.m_chromaIntraDir[absPartIdxC];
            if (chromaPredMode == DM_CHROMA_IDX)
                chromaPredMode = cu.m_lumaIntraDir[(m_csp == X265_CSP_I444) ? absPartIdxC : 0];
            if (m_csp == X265_CSP_I422)
                chromaPredMode = g_chroma422IntraAngleMappingTable[chromaPredMode];

            // get prediction signal
            predIntraChromaAng(chromaPredMode, pred, stride, log2TrSizeC);

            uint64_t bCost = MAX_INT64;
            uint32_t bDist = 0;
            uint32_t bCbf = 0;
            uint32_t bEnergy = 0;
            int      bTSkip = 0;

            int checkTransformSkip = 1;
            for (int useTSkip = 0; useTSkip <= checkTransformSkip; useTSkip++)
            {
                coeff_t* coeff = (useTSkip ? m_tsCoeff : coeffC);
                pixel*   recon = (useTSkip ? m_tsRecon : reconQt);
                uint32_t reconStride = (useTSkip ? MAX_TS_SIZE : reconQtStride);

                primitives.cu[sizeIdxC].calcresidual(fenc, pred, residual, stride);

                uint32_t numSig = m_quant.transformNxN(cu, fenc, stride, residual, stride, coeff, log2TrSizeC, ttype, absPartIdxC, useTSkip);
                if (numSig)
                {
                    m_quant.invtransformNxN(residual, stride, coeff, log2TrSizeC, ttype, true, useTSkip, numSig);
                    primitives.cu[sizeIdxC].add_ps(recon, reconStride, pred, residual, stride, stride);
                    cu.setCbfPartRange(1 << tuDepth, ttype, absPartIdxC, tuIterator.absPartIdxStep);
                }
                else if (useTSkip)
                {
                    checkTransformSkip = 0;
                    break;
                }
                else
                {
                    primitives.cu[sizeIdxC].copy_pp(recon, reconStride, pred, stride);
                    cu.setCbfPartRange(0, ttype, absPartIdxC, tuIterator.absPartIdxStep);
                }
                uint32_t tmpDist = primitives.cu[sizeIdxC].sse_pp(recon, reconStride, fenc, stride);
                tmpDist = m_rdCost.scaleChromaDist(chromaId, tmpDist);

                cu.setTransformSkipPartRange(useTSkip, ttype, absPartIdxC, tuIterator.absPartIdxStep);

                uint32_t tmpBits = 0, tmpEnergy = 0;
                if (numSig)
                {
                    m_entropyCoder.load(m_rqt[fullDepth].rqtRoot);
                    m_entropyCoder.resetBits();
                    m_entropyCoder.codeCoeffNxN(cu, coeff, absPartIdxC, log2TrSizeC, (TextType)chromaId);
                    tmpBits = m_entropyCoder.getNumberOfWrittenBits();
                }

                uint64_t tmpCost;
                if (m_rdCost.m_psyRd)
                {
                    tmpEnergy = m_rdCost.psyCost(sizeIdxC, fenc, stride, reconQt, reconQtStride);
                    tmpCost = m_rdCost.calcPsyRdCost(tmpDist, tmpBits, tmpEnergy);
                }
                else
                    tmpCost = m_rdCost.calcRdCost(tmpDist, tmpBits);

                if (tmpCost < bCost)
                {
                    bCost = tmpCost;
                    bDist = tmpDist;
                    bTSkip = useTSkip;
                    bCbf = !!numSig;
                    bEnergy = tmpEnergy;
                }
            }

            if (bTSkip)
            {
                memcpy(coeffC, m_tsCoeff, sizeof(coeff_t) << (log2TrSizeC * 2));
                primitives.cu[sizeIdxC].copy_pp(reconQt, reconQtStride, m_tsRecon, MAX_TS_SIZE);
            }

            cu.setCbfPartRange(bCbf << tuDepth, ttype, absPartIdxC, tuIterator.absPartIdxStep);
            cu.setTransformSkipPartRange(bTSkip, ttype, absPartIdxC, tuIterator.absPartIdxStep);

            pixel*   reconPicC = m_frame->m_reconPic->getChromaAddr(chromaId, cu.m_cuAddr, cuGeom.absPartIdx + absPartIdxC);
            intptr_t picStride = m_frame->m_reconPic->m_strideC;
            primitives.cu[sizeIdxC].copy_pp(reconPicC, picStride, reconQt, reconQtStride);

            outDist += bDist;
            psyEnergy += bEnergy;
        }
    }
    while (tuIterator.isNextSection());

    if (splitType == VERTICAL_SPLIT)
    {
        offsetSubTUCBFs(cu, TEXT_CHROMA_U, tuDepth, absPartIdx);
        offsetSubTUCBFs(cu, TEXT_CHROMA_V, tuDepth, absPartIdx);
    }

    m_entropyCoder.load(m_rqt[fullDepth].rqtRoot);
    return outDist;
}

void Search::extractIntraResultChromaQT(CUData& cu, Yuv& reconYuv, uint32_t absPartIdx, uint32_t tuDepth)
{
    uint32_t tuDepthL  = cu.m_tuDepth[absPartIdx];
    uint32_t log2TrSize = cu.m_log2CUSize[0] - tuDepth;
    uint32_t log2TrSizeC = log2TrSize - m_hChromaShift;

    if (tuDepthL == tuDepth || log2TrSizeC == 2)
    {
        // copy transform coefficients
        uint32_t numCoeffC = 1 << (log2TrSizeC * 2 + (m_csp == X265_CSP_I422));
        uint32_t coeffOffsetC = absPartIdx << (LOG2_UNIT_SIZE * 2 - (m_hChromaShift + m_vChromaShift));

        uint32_t qtLayer   = log2TrSize - 2 - (tuDepthL - tuDepth);
        coeff_t* coeffSrcU = m_rqt[qtLayer].coeffRQT[1] + coeffOffsetC;
        coeff_t* coeffSrcV = m_rqt[qtLayer].coeffRQT[2] + coeffOffsetC;
        coeff_t* coeffDstU = cu.m_trCoeff[1]           + coeffOffsetC;
        coeff_t* coeffDstV = cu.m_trCoeff[2]           + coeffOffsetC;
        memcpy(coeffDstU, coeffSrcU, sizeof(coeff_t) * numCoeffC);
        memcpy(coeffDstV, coeffSrcV, sizeof(coeff_t) * numCoeffC);

        // copy reconstruction
        m_rqt[qtLayer].reconQtYuv.copyPartToPartChroma(reconYuv, absPartIdx, log2TrSizeC + m_hChromaShift);
    }
    else
    {
        uint32_t qNumParts = 1 << (log2TrSize - 1 - LOG2_UNIT_SIZE) * 2;
        for (uint32_t qIdx = 0; qIdx < 4; ++qIdx, absPartIdx += qNumParts)
            extractIntraResultChromaQT(cu, reconYuv, absPartIdx, tuDepth + 1);
    }
}

void Search::residualQTIntraChroma(Mode& mode, const CUGeom& cuGeom, uint32_t absPartIdx, uint32_t tuDepth)
{
    CUData& cu = mode.cu;
    uint32_t log2TrSize = cu.m_log2CUSize[absPartIdx] - tuDepth;

    if (tuDepth < cu.m_tuDepth[absPartIdx])
    {
        uint32_t qNumParts = 1 << (log2TrSize - 1 - LOG2_UNIT_SIZE) * 2;
        uint32_t splitCbfU = 0, splitCbfV = 0;
        for (uint32_t qIdx = 0, qPartIdx = absPartIdx; qIdx < 4; ++qIdx, qPartIdx += qNumParts)
        {
            residualQTIntraChroma(mode, cuGeom, qPartIdx, tuDepth + 1);
            splitCbfU |= cu.getCbf(qPartIdx, TEXT_CHROMA_U, tuDepth + 1);
            splitCbfV |= cu.getCbf(qPartIdx, TEXT_CHROMA_V, tuDepth + 1);
        }
        for (uint32_t offs = 0; offs < 4 * qNumParts; offs++)
        {
            cu.m_cbf[1][absPartIdx + offs] |= (splitCbfU << tuDepth);
            cu.m_cbf[2][absPartIdx + offs] |= (splitCbfV << tuDepth);
        }

        return;
    }

    uint32_t log2TrSizeC = log2TrSize - m_hChromaShift;
    uint32_t tuDepthC = tuDepth;
    if (log2TrSizeC < 2)
    {
        X265_CHECK(log2TrSize == 2 && m_csp != X265_CSP_I444 && tuDepth, "invalid tuDepth\n");
        if (absPartIdx & 3)
            return;
        log2TrSizeC = 2;
        tuDepthC--;
    }

    ShortYuv& resiYuv = m_rqt[cuGeom.depth].tmpResiYuv;
    uint32_t stride = mode.fencYuv->m_csize;
    const uint32_t sizeIdxC = log2TrSizeC - 2;

    uint32_t curPartNum = cuGeom.numPartitions >> tuDepthC * 2;
    const SplitType splitType = (m_csp == X265_CSP_I422) ? VERTICAL_SPLIT : DONT_SPLIT;

    TURecurse tuIterator(splitType, curPartNum, absPartIdx);
    do
    {
        uint32_t absPartIdxC = tuIterator.absPartIdxTURelCU;

        IntraNeighbors intraNeighbors;
        initIntraNeighbors(cu, absPartIdxC, tuDepthC, false, &intraNeighbors);

        for (uint32_t chromaId = TEXT_CHROMA_U; chromaId <= TEXT_CHROMA_V; chromaId++)
        {
            TextType ttype = (TextType)chromaId;

            const pixel* fenc = mode.fencYuv->getChromaAddr(chromaId, absPartIdxC);
            pixel*   pred     = mode.predYuv.getChromaAddr(chromaId, absPartIdxC);
            int16_t* residual = resiYuv.getChromaAddr(chromaId, absPartIdxC);
            uint32_t coeffOffsetC  = absPartIdxC << (LOG2_UNIT_SIZE * 2 - (m_hChromaShift + m_vChromaShift));
            coeff_t* coeffC        = cu.m_trCoeff[ttype] + coeffOffsetC;
            pixel*   picReconC = m_frame->m_reconPic->getChromaAddr(chromaId, cu.m_cuAddr, cuGeom.absPartIdx + absPartIdxC);
            intptr_t picStride = m_frame->m_reconPic->m_strideC;

            uint32_t chromaPredMode = cu.m_chromaIntraDir[absPartIdxC];
            if (chromaPredMode == DM_CHROMA_IDX)
                chromaPredMode = cu.m_lumaIntraDir[(m_csp == X265_CSP_I444) ? absPartIdxC : 0];
            if (m_csp == X265_CSP_I422)
                chromaPredMode = g_chroma422IntraAngleMappingTable[chromaPredMode];

            // init availability pattern
            initAdiPatternChroma(cu, cuGeom, absPartIdxC, intraNeighbors, chromaId);

            // get prediction signal
            predIntraChromaAng(chromaPredMode, pred, stride, log2TrSizeC);

            X265_CHECK(!cu.m_transformSkip[ttype][0], "transform skip not supported at low RD levels\n");

            primitives.cu[sizeIdxC].calcresidual(fenc, pred, residual, stride);
            uint32_t numSig = m_quant.transformNxN(cu, fenc, stride, residual, stride, coeffC, log2TrSizeC, ttype, absPartIdxC, false);
            if (numSig)
            {
                m_quant.invtransformNxN(residual, stride, coeffC, log2TrSizeC, ttype, true, false, numSig);
                primitives.cu[sizeIdxC].add_ps(picReconC, picStride, pred, residual, stride, stride);
                cu.setCbfPartRange(1 << tuDepth, ttype, absPartIdxC, tuIterator.absPartIdxStep);
            }
            else
            {
                // no coded residual, recon = pred
                primitives.cu[sizeIdxC].copy_pp(picReconC, picStride, pred, stride);
                cu.setCbfPartRange(0, ttype, absPartIdxC, tuIterator.absPartIdxStep);
            }
        }
    }
    while (tuIterator.isNextSection());

    if (splitType == VERTICAL_SPLIT)
    {
        offsetSubTUCBFs(cu, TEXT_CHROMA_U, tuDepth, absPartIdx);
        offsetSubTUCBFs(cu, TEXT_CHROMA_V, tuDepth, absPartIdx);
    }
}

void Search::checkIntra(Mode& intraMode, const CUGeom& cuGeom, PartSize partSize, uint8_t* sharedModes, uint8_t* sharedChromaModes)
{
    CUData& cu = intraMode.cu;

    cu.setPartSizeSubParts(partSize);
    cu.setPredModeSubParts(MODE_INTRA);
    m_quant.m_tqBypass = !!cu.m_tqBypass[0];

    uint32_t tuDepthRange[2];
    cu.getIntraTUQtDepthRange(tuDepthRange, 0);

    intraMode.initCosts();
    intraMode.distortion += estIntraPredQT(intraMode, cuGeom, tuDepthRange, sharedModes);
    intraMode.distortion += estIntraPredChromaQT(intraMode, cuGeom, sharedChromaModes);

    m_entropyCoder.resetBits();
    if (m_slice->m_pps->bTransquantBypassEnabled)
        m_entropyCoder.codeCUTransquantBypassFlag(cu.m_tqBypass[0]);

    if (!m_slice->isIntra())
    {
        m_entropyCoder.codeSkipFlag(cu, 0);
        m_entropyCoder.codePredMode(cu.m_predMode[0]);
    }

    m_entropyCoder.codePartSize(cu, 0, cuGeom.depth);
    m_entropyCoder.codePredInfo(cu, 0);
    intraMode.mvBits = m_entropyCoder.getNumberOfWrittenBits();

    bool bCodeDQP = m_slice->m_pps->bUseDQP;
    m_entropyCoder.codeCoeff(cu, 0, bCodeDQP, tuDepthRange);
    m_entropyCoder.store(intraMode.contexts);
    intraMode.totalBits = m_entropyCoder.getNumberOfWrittenBits();
    intraMode.coeffBits = intraMode.totalBits - intraMode.mvBits;
    if (m_rdCost.m_psyRd)
    {
        const Yuv* fencYuv = intraMode.fencYuv;
        intraMode.psyEnergy = m_rdCost.psyCost(cuGeom.log2CUSize - 2, fencYuv->m_buf[0], fencYuv->m_size, intraMode.reconYuv.m_buf[0], intraMode.reconYuv.m_size);
    }
    updateModeCost(intraMode);
    checkDQP(cu, cuGeom);
}

/* Note that this function does not save the best intra prediction, it must
 * be generated later. It records the best mode in the cu */
void Search::checkIntraInInter(Mode& intraMode, const CUGeom& cuGeom)
{
    ProfileCUScope(intraMode.cu, intraAnalysisElapsedTime, countIntraAnalysis);

    CUData& cu = intraMode.cu;
    uint32_t depth = cuGeom.depth;

    cu.setPartSizeSubParts(SIZE_2Nx2N);
    cu.setPredModeSubParts(MODE_INTRA);

    const uint32_t initTuDepth = 0;
    uint32_t log2TrSize = cuGeom.log2CUSize - initTuDepth;
    uint32_t tuSize = 1 << log2TrSize;
    const uint32_t absPartIdx = 0;

    // Reference sample smoothing
    IntraNeighbors intraNeighbors;
    initIntraNeighbors(cu, absPartIdx, initTuDepth, true, &intraNeighbors);
    initAdiPattern(cu, cuGeom, absPartIdx, intraNeighbors, ALL_IDX);

    const pixel* fenc = intraMode.fencYuv->m_buf[0];
    uint32_t stride = intraMode.fencYuv->m_size;

    int sad, bsad;
    uint32_t bits, bbits, mode, bmode;
    uint64_t cost, bcost;

    // 33 Angle modes once
    int scaleTuSize = tuSize;
    int scaleStride = stride;
    int costShift = 0;
    int sizeIdx = log2TrSize - 2;

    if (tuSize > 32)
    {
        // CU is 64x64, we scale to 32x32 and adjust required parameters
        primitives.scale2D_64to32(m_fencScaled, fenc, stride);
        fenc = m_fencScaled;

        pixel nScale[129];
        intraNeighbourBuf[1][0] = intraNeighbourBuf[0][0];
        primitives.scale1D_128to64(nScale + 1, intraNeighbourBuf[0] + 1);

        // we do not estimate filtering for downscaled samples
        memcpy(&intraNeighbourBuf[0][1], &nScale[1], 2 * 64 * sizeof(pixel));   // Top & Left pixels
        memcpy(&intraNeighbourBuf[1][1], &nScale[1], 2 * 64 * sizeof(pixel));

        scaleTuSize = 32;
        scaleStride = 32;
        costShift = 2;
        sizeIdx = 5 - 2; // log2(scaleTuSize) - 2
    }

    pixelcmp_t sa8d = primitives.cu[sizeIdx].sa8d;
    int predsize = scaleTuSize * scaleTuSize;

    m_entropyCoder.loadIntraDirModeLuma(m_rqt[depth].cur);

    /* there are three cost tiers for intra modes:
     *  pred[0]          - mode probable, least cost
     *  pred[1], pred[2] - less probable, slightly more cost
     *  non-mpm modes    - all cost the same (rbits) */
    uint64_t mpms;
    uint32_t mpmModes[3];
    uint32_t rbits = getIntraRemModeBits(cu, absPartIdx, mpmModes, mpms);

    // DC
    primitives.cu[sizeIdx].intra_pred[DC_IDX](m_intraPredAngs, scaleStride, intraNeighbourBuf[0], 0, (scaleTuSize <= 16));
    bsad = sa8d(fenc, scaleStride, m_intraPredAngs, scaleStride) << costShift;
    bmode = mode = DC_IDX;
    bbits = (mpms & ((uint64_t)1 << mode)) ? m_entropyCoder.bitsIntraModeMPM(mpmModes, mode) : rbits;
    bcost = m_rdCost.calcRdSADCost(bsad, bbits);

    // PLANAR
    pixel* planar = intraNeighbourBuf[0];
    if (tuSize & (8 | 16 | 32))
        planar = intraNeighbourBuf[1];

    primitives.cu[sizeIdx].intra_pred[PLANAR_IDX](m_intraPredAngs, scaleStride, planar, 0, 0);
    sad = sa8d(fenc, scaleStride, m_intraPredAngs, scaleStride) << costShift;
    mode = PLANAR_IDX;
    bits = (mpms & ((uint64_t)1 << mode)) ? m_entropyCoder.bitsIntraModeMPM(mpmModes, mode) : rbits;
    cost = m_rdCost.calcRdSADCost(sad, bits);
    COPY4_IF_LT(bcost, cost, bmode, mode, bsad, sad, bbits, bits);

    bool allangs = true;
    if (primitives.cu[sizeIdx].intra_pred_allangs)
    {
        primitives.cu[sizeIdx].transpose(m_fencTransposed, fenc, scaleStride);
        primitives.cu[sizeIdx].intra_pred_allangs(m_intraPredAngs, intraNeighbourBuf[0], intraNeighbourBuf[1], (scaleTuSize <= 16)); 
    }
    else
        allangs = false;

#define TRY_ANGLE(angle) \
    if (allangs) { \
        if (angle < 18) \
            sad = sa8d(m_fencTransposed, scaleTuSize, &m_intraPredAngs[(angle - 2) * predsize], scaleTuSize) << costShift; \
        else \
            sad = sa8d(fenc, scaleStride, &m_intraPredAngs[(angle - 2) * predsize], scaleTuSize) << costShift; \
        bits = (mpms & ((uint64_t)1 << angle)) ? m_entropyCoder.bitsIntraModeMPM(mpmModes, angle) : rbits; \
        cost = m_rdCost.calcRdSADCost(sad, bits); \
    } else { \
        int filter = !!(g_intraFilterFlags[angle] & scaleTuSize); \
        primitives.cu[sizeIdx].intra_pred[angle](m_intraPredAngs, scaleTuSize, intraNeighbourBuf[filter], angle, scaleTuSize <= 16); \
        sad = sa8d(fenc, scaleStride, m_intraPredAngs, scaleTuSize) << costShift; \
        bits = (mpms & ((uint64_t)1 << angle)) ? m_entropyCoder.bitsIntraModeMPM(mpmModes, angle) : rbits; \
        cost = m_rdCost.calcRdSADCost(sad, bits); \
    }

    if (m_param->bEnableFastIntra)
    {
        int asad = 0;
        uint32_t lowmode, highmode, amode = 5, abits = 0;
        uint64_t acost = MAX_INT64;

        /* pick the best angle, sampling at distance of 5 */
        for (mode = 5; mode < 35; mode += 5)
        {
            TRY_ANGLE(mode);
            COPY4_IF_LT(acost, cost, amode, mode, asad, sad, abits, bits);
        }

        /* refine best angle at distance 2, then distance 1 */
        for (uint32_t dist = 2; dist >= 1; dist--)
        {
            lowmode = amode - dist;
            highmode = amode + dist;

            X265_CHECK(lowmode >= 2 && lowmode <= 34, "low intra mode out of range\n");
            TRY_ANGLE(lowmode);
            COPY4_IF_LT(acost, cost, amode, lowmode, asad, sad, abits, bits);

            X265_CHECK(highmode >= 2 && highmode <= 34, "high intra mode out of range\n");
            TRY_ANGLE(highmode);
            COPY4_IF_LT(acost, cost, amode, highmode, asad, sad, abits, bits);
        }

        if (amode == 33)
        {
            TRY_ANGLE(34);
            COPY4_IF_LT(acost, cost, amode, 34, asad, sad, abits, bits);
        }

        COPY4_IF_LT(bcost, acost, bmode, amode, bsad, asad, bbits, abits);
    }
    else // calculate and search all intra prediction angles for lowest cost
    {
        for (mode = 2; mode < 35; mode++)
        {
            TRY_ANGLE(mode);
            COPY4_IF_LT(bcost, cost, bmode, mode, bsad, sad, bbits, bits);
        }
    }

    cu.setLumaIntraDirSubParts((uint8_t)bmode, absPartIdx, depth + initTuDepth);
    intraMode.initCosts();
    intraMode.totalBits = bbits;
    intraMode.distortion = bsad;
    intraMode.sa8dCost = bcost;
    intraMode.sa8dBits = bbits;
    X265_CHECK(intraMode.ok(), "intra mode is not ok");
}

void Search::encodeIntraInInter(Mode& intraMode, const CUGeom& cuGeom)
{
    ProfileCUScope(intraMode.cu, intraRDOElapsedTime[cuGeom.depth], countIntraRDO[cuGeom.depth]);

    CUData& cu = intraMode.cu;
    Yuv* reconYuv = &intraMode.reconYuv;

    X265_CHECK(cu.m_partSize[0] == SIZE_2Nx2N, "encodeIntraInInter does not expect NxN intra\n");
    X265_CHECK(!m_slice->isIntra(), "encodeIntraInInter does not expect to be used in I slices\n");

    m_quant.setQPforQuant(cu);

    uint32_t tuDepthRange[2];
    cu.getIntraTUQtDepthRange(tuDepthRange, 0);

    m_entropyCoder.load(m_rqt[cuGeom.depth].cur);

    Cost icosts;
    codeIntraLumaQT(intraMode, cuGeom, 0, 0, false, icosts, tuDepthRange);
    extractIntraResultQT(cu, *reconYuv, 0, 0);

    intraMode.distortion = icosts.distortion;
    intraMode.distortion += estIntraPredChromaQT(intraMode, cuGeom, NULL);

    m_entropyCoder.resetBits();
    if (m_slice->m_pps->bTransquantBypassEnabled)
        m_entropyCoder.codeCUTransquantBypassFlag(cu.m_tqBypass[0]);
    m_entropyCoder.codeSkipFlag(cu, 0);
    m_entropyCoder.codePredMode(cu.m_predMode[0]);
    m_entropyCoder.codePartSize(cu, 0, cuGeom.depth);
    m_entropyCoder.codePredInfo(cu, 0);
    intraMode.mvBits += m_entropyCoder.getNumberOfWrittenBits();

    bool bCodeDQP = m_slice->m_pps->bUseDQP;
    m_entropyCoder.codeCoeff(cu, 0, bCodeDQP, tuDepthRange);

    intraMode.totalBits = m_entropyCoder.getNumberOfWrittenBits();
    intraMode.coeffBits = intraMode.totalBits - intraMode.mvBits;
    if (m_rdCost.m_psyRd)
    {
        const Yuv* fencYuv = intraMode.fencYuv;
        intraMode.psyEnergy = m_rdCost.psyCost(cuGeom.log2CUSize - 2, fencYuv->m_buf[0], fencYuv->m_size, reconYuv->m_buf[0], reconYuv->m_size);
    }

    m_entropyCoder.store(intraMode.contexts);
    updateModeCost(intraMode);
    checkDQP(intraMode.cu, cuGeom);
}

uint32_t Search::estIntraPredQT(Mode &intraMode, const CUGeom& cuGeom, const uint32_t depthRange[2], uint8_t* sharedModes)
{
    CUData& cu = intraMode.cu;
    Yuv* reconYuv = &intraMode.reconYuv;
    Yuv* predYuv = &intraMode.predYuv;
    const Yuv* fencYuv = intraMode.fencYuv;

    uint32_t depth        = cuGeom.depth;
    uint32_t initTuDepth  = cu.m_partSize[0] != SIZE_2Nx2N;
    uint32_t numPU        = 1 << (2 * initTuDepth);
    uint32_t log2TrSize   = cuGeom.log2CUSize - initTuDepth;
    uint32_t tuSize       = 1 << log2TrSize;
    uint32_t qNumParts    = cuGeom.numPartitions >> 2;
    uint32_t sizeIdx      = log2TrSize - 2;
    uint32_t absPartIdx   = 0;
    uint32_t totalDistortion = 0;

    int checkTransformSkip = m_slice->m_pps->bTransformSkipEnabled && !cu.m_tqBypass[0] && cu.m_partSize[0] != SIZE_2Nx2N;

    // loop over partitions
    for (uint32_t puIdx = 0; puIdx < numPU; puIdx++, absPartIdx += qNumParts)
    {
        uint32_t bmode = 0;

        if (sharedModes)
            bmode = sharedModes[puIdx];
        else
        {
            uint64_t candCostList[MAX_RD_INTRA_MODES];
            uint32_t rdModeList[MAX_RD_INTRA_MODES];
            uint64_t bcost;
            int maxCandCount = 2 + m_param->rdLevel + ((depth + initTuDepth) >> 1);

            {
                ProfileCUScope(intraMode.cu, intraAnalysisElapsedTime, countIntraAnalysis);

                // Reference sample smoothing
                IntraNeighbors intraNeighbors;
                initIntraNeighbors(cu, absPartIdx, initTuDepth, true, &intraNeighbors);
                initAdiPattern(cu, cuGeom, absPartIdx, intraNeighbors, ALL_IDX);

                // determine set of modes to be tested (using prediction signal only)
                const pixel* fenc = fencYuv->getLumaAddr(absPartIdx);
                uint32_t stride = predYuv->m_size;

                int scaleTuSize = tuSize;
                int scaleStride = stride;
                int costShift = 0;

                if (tuSize > 32)
                {
                    // origin is 64x64, we scale to 32x32 and setup required parameters
                    primitives.scale2D_64to32(m_fencScaled, fenc, stride);
                    fenc = m_fencScaled;

                    pixel nScale[129];
                    intraNeighbourBuf[1][0] = intraNeighbourBuf[0][0];
                    primitives.scale1D_128to64(nScale + 1, intraNeighbourBuf[0] + 1);

                    memcpy(&intraNeighbourBuf[0][1], &nScale[1], 2 * 64 * sizeof(pixel));
                    memcpy(&intraNeighbourBuf[1][1], &nScale[1], 2 * 64 * sizeof(pixel));

                    scaleTuSize = 32;
                    scaleStride = 32;
                    costShift = 2;
                    sizeIdx = 5 - 2; // log2(scaleTuSize) - 2
                }

                m_entropyCoder.loadIntraDirModeLuma(m_rqt[depth].cur);

                /* there are three cost tiers for intra modes:
                *  pred[0]          - mode probable, least cost
                *  pred[1], pred[2] - less probable, slightly more cost
                *  non-mpm modes    - all cost the same (rbits) */
                uint64_t mpms;
                uint32_t mpmModes[3];
                uint32_t rbits = getIntraRemModeBits(cu, absPartIdx, mpmModes, mpms);

                pixelcmp_t sa8d = primitives.cu[sizeIdx].sa8d;
                uint64_t modeCosts[35];

                // DC
                primitives.cu[sizeIdx].intra_pred[DC_IDX](m_intraPred, scaleStride, intraNeighbourBuf[0], 0, (scaleTuSize <= 16));
                uint32_t bits = (mpms & ((uint64_t)1 << DC_IDX)) ? m_entropyCoder.bitsIntraModeMPM(mpmModes, DC_IDX) : rbits;
                uint32_t sad = sa8d(fenc, scaleStride, m_intraPred, scaleStride) << costShift;
                modeCosts[DC_IDX] = bcost = m_rdCost.calcRdSADCost(sad, bits);

                // PLANAR
                pixel* planar = intraNeighbourBuf[0];
                if (tuSize >= 8 && tuSize <= 32)
                    planar = intraNeighbourBuf[1];

                primitives.cu[sizeIdx].intra_pred[PLANAR_IDX](m_intraPred, scaleStride, planar, 0, 0);
                bits = (mpms & ((uint64_t)1 << PLANAR_IDX)) ? m_entropyCoder.bitsIntraModeMPM(mpmModes, PLANAR_IDX) : rbits;
                sad = sa8d(fenc, scaleStride, m_intraPred, scaleStride) << costShift;
                modeCosts[PLANAR_IDX] = m_rdCost.calcRdSADCost(sad, bits);
                COPY1_IF_LT(bcost, modeCosts[PLANAR_IDX]);

                // angular predictions
                if (primitives.cu[sizeIdx].intra_pred_allangs)
                {
                    primitives.cu[sizeIdx].transpose(m_fencTransposed, fenc, scaleStride);
                    primitives.cu[sizeIdx].intra_pred_allangs(m_intraPredAngs, intraNeighbourBuf[0], intraNeighbourBuf[1], (scaleTuSize <= 16));
                    for (int mode = 2; mode < 35; mode++)
                    {
                        bits = (mpms & ((uint64_t)1 << mode)) ? m_entropyCoder.bitsIntraModeMPM(mpmModes, mode) : rbits;
                        if (mode < 18)
                            sad = sa8d(m_fencTransposed, scaleTuSize, &m_intraPredAngs[(mode - 2) * (scaleTuSize * scaleTuSize)], scaleTuSize) << costShift;
                        else
                            sad = sa8d(fenc, scaleStride, &m_intraPredAngs[(mode - 2) * (scaleTuSize * scaleTuSize)], scaleTuSize) << costShift;
                        modeCosts[mode] = m_rdCost.calcRdSADCost(sad, bits);
                        COPY1_IF_LT(bcost, modeCosts[mode]);
                    }
                }
                else
                {
                    for (int mode = 2; mode < 35; mode++)
                    {
                        bits = (mpms & ((uint64_t)1 << mode)) ? m_entropyCoder.bitsIntraModeMPM(mpmModes, mode) : rbits;
                        int filter = !!(g_intraFilterFlags[mode] & scaleTuSize);
                        primitives.cu[sizeIdx].intra_pred[mode](m_intraPred, scaleTuSize, intraNeighbourBuf[filter], mode, scaleTuSize <= 16);
                        sad = sa8d(fenc, scaleStride, m_intraPred, scaleTuSize) << costShift;
                        modeCosts[mode] = m_rdCost.calcRdSADCost(sad, bits);
                        COPY1_IF_LT(bcost, modeCosts[mode]);
                    }
                }

                /* Find the top maxCandCount candidate modes with cost within 25% of best
                * or among the most probable modes. maxCandCount is derived from the
                * rdLevel and depth. In general we want to try more modes at slower RD
                * levels and at higher depths */
                for (int i = 0; i < maxCandCount; i++)
                    candCostList[i] = MAX_INT64;

                uint64_t paddedBcost = bcost + (bcost >> 3); // 1.12%
                for (int mode = 0; mode < 35; mode++)
                    if (modeCosts[mode] < paddedBcost || (mpms & ((uint64_t)1 << mode)))
                        updateCandList(mode, modeCosts[mode], maxCandCount, rdModeList, candCostList);
            }

            /* measure best candidates using simple RDO (no TU splits) */
            bcost = MAX_INT64;
            for (int i = 0; i < maxCandCount; i++)
            {
                if (candCostList[i] == MAX_INT64)
                    break;

                ProfileCUScope(intraMode.cu, intraRDOElapsedTime[cuGeom.depth], countIntraRDO[cuGeom.depth]);

                m_entropyCoder.load(m_rqt[depth].cur);
                cu.setLumaIntraDirSubParts(rdModeList[i], absPartIdx, depth + initTuDepth);

                Cost icosts;
                if (checkTransformSkip)
                    codeIntraLumaTSkip(intraMode, cuGeom, initTuDepth, absPartIdx, icosts);
                else
                    codeIntraLumaQT(intraMode, cuGeom, initTuDepth, absPartIdx, false, icosts, depthRange);
                COPY2_IF_LT(bcost, icosts.rdcost, bmode, rdModeList[i]);
            }
        }

        ProfileCUScope(intraMode.cu, intraRDOElapsedTime[cuGeom.depth], countIntraRDO[cuGeom.depth]);

        /* remeasure best mode, allowing TU splits */
        cu.setLumaIntraDirSubParts(bmode, absPartIdx, depth + initTuDepth);
        m_entropyCoder.load(m_rqt[depth].cur);

        Cost icosts;
        if (checkTransformSkip)
            codeIntraLumaTSkip(intraMode, cuGeom, initTuDepth, absPartIdx, icosts);
        else
            codeIntraLumaQT(intraMode, cuGeom, initTuDepth, absPartIdx, true, icosts, depthRange);
        totalDistortion += icosts.distortion;

        extractIntraResultQT(cu, *reconYuv, initTuDepth, absPartIdx);

        // set reconstruction for next intra prediction blocks
        if (puIdx != numPU - 1)
        {
            /* This has important implications for parallelism and RDO.  It is writing intermediate results into the
             * output recon picture, so it cannot proceed in parallel with anything else when doing INTRA_NXN. Also
             * it is not updating m_rdContexts[depth].cur for the later PUs which I suspect is slightly wrong. I think
             * that the contexts should be tracked through each PU */
            pixel*   dst         = m_frame->m_reconPic->getLumaAddr(cu.m_cuAddr, cuGeom.absPartIdx + absPartIdx);
            uint32_t dststride   = m_frame->m_reconPic->m_stride;
            const pixel*   src   = reconYuv->getLumaAddr(absPartIdx);
            uint32_t srcstride   = reconYuv->m_size;
            primitives.cu[log2TrSize - 2].copy_pp(dst, dststride, src, srcstride);
        }
    }

    if (numPU > 1)
    {
        uint32_t combCbfY = 0;
        for (uint32_t qIdx = 0, qPartIdx = 0; qIdx < 4; ++qIdx, qPartIdx += qNumParts)
            combCbfY |= cu.getCbf(qPartIdx, TEXT_LUMA, 1);

        for (uint32_t offs = 0; offs < 4 * qNumParts; offs++)
            cu.m_cbf[0][offs] |= combCbfY;
    }

    // TODO: remove this
    m_entropyCoder.load(m_rqt[depth].cur);

    return totalDistortion;
}

void Search::getBestIntraModeChroma(Mode& intraMode, const CUGeom& cuGeom)
{
    CUData& cu = intraMode.cu;
    const Yuv* fencYuv = intraMode.fencYuv;
    Yuv* predYuv = &intraMode.predYuv;

    uint32_t bestMode  = 0;
    uint64_t bestCost  = MAX_INT64;
    uint32_t modeList[NUM_CHROMA_MODE];

    uint32_t log2TrSizeC = cu.m_log2CUSize[0] - m_hChromaShift;
    uint32_t tuSize = 1 << log2TrSizeC;
    uint32_t tuDepth = 0;
    int32_t costShift = 0;

    if (tuSize > 32)
    {
        tuDepth = 1;
        costShift = 2;
        log2TrSizeC = 5;
    }

    IntraNeighbors intraNeighbors;
    initIntraNeighbors(cu, 0, tuDepth, false, &intraNeighbors);
    cu.getAllowedChromaDir(0, modeList);

    // check chroma modes
    for (uint32_t mode = 0; mode < NUM_CHROMA_MODE; mode++)
    {
        uint32_t chromaPredMode = modeList[mode];
        if (chromaPredMode == DM_CHROMA_IDX)
            chromaPredMode = cu.m_lumaIntraDir[0];
        if (m_csp == X265_CSP_I422)
            chromaPredMode = g_chroma422IntraAngleMappingTable[chromaPredMode];

        uint64_t cost = 0;
        for (uint32_t chromaId = TEXT_CHROMA_U; chromaId <= TEXT_CHROMA_V; chromaId++)
        {
            const pixel* fenc = fencYuv->m_buf[chromaId];
            pixel* pred = predYuv->m_buf[chromaId];
            Predict::initAdiPatternChroma(cu, cuGeom, 0, intraNeighbors, chromaId);
            // get prediction signal
            predIntraChromaAng(chromaPredMode, pred, fencYuv->m_csize, log2TrSizeC);
            cost += primitives.cu[log2TrSizeC - 2].sa8d(fenc, predYuv->m_csize, pred, fencYuv->m_csize) << costShift;
        }

        if (cost < bestCost)
        {
            bestCost = cost;
            bestMode = modeList[mode];
        }
    }

    cu.setChromIntraDirSubParts(bestMode, 0, cuGeom.depth);
}

uint32_t Search::estIntraPredChromaQT(Mode &intraMode, const CUGeom& cuGeom, uint8_t* sharedChromaModes)
{
    CUData& cu = intraMode.cu;
    Yuv& reconYuv = intraMode.reconYuv;

    uint32_t depth       = cuGeom.depth;
    uint32_t initTuDepth = cu.m_partSize[0] != SIZE_2Nx2N && m_csp == X265_CSP_I444;
    uint32_t log2TrSize  = cuGeom.log2CUSize - initTuDepth;
    uint32_t absPartStep = cuGeom.numPartitions;
    uint32_t totalDistortion = 0;

    int size = partitionFromLog2Size(log2TrSize);

    TURecurse tuIterator((initTuDepth == 0) ? DONT_SPLIT : QUAD_SPLIT, absPartStep, 0);

    do
    {
        uint32_t absPartIdxC = tuIterator.absPartIdxTURelCU;

        uint32_t bestMode = 0;
        uint32_t bestDist = 0;
        uint64_t bestCost = MAX_INT64;

        // init mode list
        uint32_t minMode = 0;
        uint32_t maxMode = NUM_CHROMA_MODE;
        uint32_t modeList[NUM_CHROMA_MODE];

        if (sharedChromaModes && !initTuDepth)
        {
            for (uint32_t l = 0; l < NUM_CHROMA_MODE; l++)
                modeList[l] = sharedChromaModes[0];
            maxMode = 1;
        }
        else
            cu.getAllowedChromaDir(absPartIdxC, modeList);

        // check chroma modes
        for (uint32_t mode = minMode; mode < maxMode; mode++)
        {
            // restore context models
            m_entropyCoder.load(m_rqt[depth].cur);

            cu.setChromIntraDirSubParts(modeList[mode], absPartIdxC, depth + initTuDepth);
            uint32_t psyEnergy = 0;
            uint32_t dist = codeIntraChromaQt(intraMode, cuGeom, initTuDepth, absPartIdxC, psyEnergy);

            if (m_slice->m_pps->bTransformSkipEnabled)
                m_entropyCoder.load(m_rqt[depth].cur);

            m_entropyCoder.resetBits();
            // chroma prediction mode
            if (cu.m_partSize[0] == SIZE_2Nx2N || m_csp != X265_CSP_I444)
            {
                if (!absPartIdxC)
                    m_entropyCoder.codeIntraDirChroma(cu, absPartIdxC, modeList);
            }
            else
            {
                uint32_t qNumParts = cuGeom.numPartitions >> 2;
                if (!(absPartIdxC & (qNumParts - 1)))
                    m_entropyCoder.codeIntraDirChroma(cu, absPartIdxC, modeList);
            }

            codeSubdivCbfQTChroma(cu, initTuDepth, absPartIdxC);
            codeCoeffQTChroma(cu, initTuDepth, absPartIdxC, TEXT_CHROMA_U);
            codeCoeffQTChroma(cu, initTuDepth, absPartIdxC, TEXT_CHROMA_V);
            uint32_t bits = m_entropyCoder.getNumberOfWrittenBits();
            uint64_t cost = m_rdCost.m_psyRd ? m_rdCost.calcPsyRdCost(dist, bits, psyEnergy) : m_rdCost.calcRdCost(dist, bits);

            if (cost < bestCost)
            {
                bestCost = cost;
                bestDist = dist;
                bestMode = modeList[mode];
                extractIntraResultChromaQT(cu, reconYuv, absPartIdxC, initTuDepth);
                memcpy(m_qtTempCbf[1], cu.m_cbf[1] + absPartIdxC, tuIterator.absPartIdxStep * sizeof(uint8_t));
                memcpy(m_qtTempCbf[2], cu.m_cbf[2] + absPartIdxC, tuIterator.absPartIdxStep * sizeof(uint8_t));
                memcpy(m_qtTempTransformSkipFlag[1], cu.m_transformSkip[1] + absPartIdxC, tuIterator.absPartIdxStep * sizeof(uint8_t));
                memcpy(m_qtTempTransformSkipFlag[2], cu.m_transformSkip[2] + absPartIdxC, tuIterator.absPartIdxStep * sizeof(uint8_t));
            }
        }

        if (!tuIterator.isLastSection())
        {
            uint32_t zorder    = cuGeom.absPartIdx + absPartIdxC;
            uint32_t dststride = m_frame->m_reconPic->m_strideC;
            const pixel* src;
            pixel* dst;

            dst = m_frame->m_reconPic->getCbAddr(cu.m_cuAddr, zorder);
            src = reconYuv.getCbAddr(absPartIdxC);
            primitives.chroma[m_csp].cu[size].copy_pp(dst, dststride, src, reconYuv.m_csize);

            dst = m_frame->m_reconPic->getCrAddr(cu.m_cuAddr, zorder);
            src = reconYuv.getCrAddr(absPartIdxC);
            primitives.chroma[m_csp].cu[size].copy_pp(dst, dststride, src, reconYuv.m_csize);
        }

        memcpy(cu.m_cbf[1] + absPartIdxC, m_qtTempCbf[1], tuIterator.absPartIdxStep * sizeof(uint8_t));
        memcpy(cu.m_cbf[2] + absPartIdxC, m_qtTempCbf[2], tuIterator.absPartIdxStep * sizeof(uint8_t));
        memcpy(cu.m_transformSkip[1] + absPartIdxC, m_qtTempTransformSkipFlag[1], tuIterator.absPartIdxStep * sizeof(uint8_t));
        memcpy(cu.m_transformSkip[2] + absPartIdxC, m_qtTempTransformSkipFlag[2], tuIterator.absPartIdxStep * sizeof(uint8_t));
        cu.setChromIntraDirSubParts(bestMode, absPartIdxC, depth + initTuDepth);
        totalDistortion += bestDist;
    }
    while (tuIterator.isNextSection());

    if (initTuDepth != 0)
    {
        uint32_t combCbfU = 0;
        uint32_t combCbfV = 0;
        uint32_t qNumParts = tuIterator.absPartIdxStep;
        for (uint32_t qIdx = 0, qPartIdx = 0; qIdx < 4; ++qIdx, qPartIdx += qNumParts)
        {
            combCbfU |= cu.getCbf(qPartIdx, TEXT_CHROMA_U, 1);
            combCbfV |= cu.getCbf(qPartIdx, TEXT_CHROMA_V, 1);
        }

        for (uint32_t offs = 0; offs < 4 * qNumParts; offs++)
        {
            cu.m_cbf[1][offs] |= combCbfU;
            cu.m_cbf[2][offs] |= combCbfV;
        }
    }

    /* TODO: remove this */
    m_entropyCoder.load(m_rqt[depth].cur);
    return totalDistortion;
}

/* estimation of best merge coding of an inter PU (2Nx2N merge PUs are evaluated as their own mode) */
uint32_t Search::mergeEstimation(CUData& cu, const CUGeom& cuGeom, const PredictionUnit& pu, int puIdx, MergeData& m)
{
    X265_CHECK(cu.m_partSize[0] != SIZE_2Nx2N, "mergeEstimation() called for 2Nx2N\n");

    MVField  candMvField[MRG_MAX_NUM_CANDS][2];
    uint8_t  candDir[MRG_MAX_NUM_CANDS];
    uint32_t numMergeCand = cu.getInterMergeCandidates(pu.puAbsPartIdx, puIdx, candMvField, candDir);

    if (cu.isBipredRestriction())
    {
        /* do not allow bidir merge candidates if PU is smaller than 8x8, drop L1 reference */
        for (uint32_t mergeCand = 0; mergeCand < numMergeCand; ++mergeCand)
        {
            if (candDir[mergeCand] == 3)
            {
                candDir[mergeCand] = 1;
                candMvField[mergeCand][1].refIdx = REF_NOT_VALID;
            }
        }
    }

    Yuv& tempYuv = m_rqt[cuGeom.depth].tmpPredYuv;

    uint32_t outCost = MAX_UINT;
    for (uint32_t mergeCand = 0; mergeCand < numMergeCand; ++mergeCand)
    {
        /* Prevent TMVP candidates from using unavailable reference pixels */
        if (m_bFrameParallel &&
            (candMvField[mergeCand][0].mv.y >= (m_param->searchRange + 1) * 4 ||
             candMvField[mergeCand][1].mv.y >= (m_param->searchRange + 1) * 4))
            continue;

        cu.m_mv[0][pu.puAbsPartIdx] = candMvField[mergeCand][0].mv;
        cu.m_refIdx[0][pu.puAbsPartIdx] = (int8_t)candMvField[mergeCand][0].refIdx;
        cu.m_mv[1][pu.puAbsPartIdx] = candMvField[mergeCand][1].mv;
        cu.m_refIdx[1][pu.puAbsPartIdx] = (int8_t)candMvField[mergeCand][1].refIdx;

        motionCompensation(cu, pu, tempYuv, true, m_me.bChromaSATD);

        uint32_t costCand = m_me.bufSATD(tempYuv.getLumaAddr(pu.puAbsPartIdx), tempYuv.m_size);
        if (m_me.bChromaSATD)
            costCand += m_me.bufChromaSATD(tempYuv, pu.puAbsPartIdx);

        uint32_t bitsCand = getTUBits(mergeCand, numMergeCand);
        costCand = costCand + m_rdCost.getCost(bitsCand);
        if (costCand < outCost)
        {
            outCost = costCand;
            m.bits = bitsCand;
            m.index = mergeCand;
        }
    }

    m.mvField[0] = candMvField[m.index][0];
    m.mvField[1] = candMvField[m.index][1];
    m.dir = candDir[m.index];

    return outCost;
}

void Search::PME::processTasks(int workerThreadId)
{
#if DETAILED_CU_STATS
    int fe = mode.cu.m_encData->m_frameEncoderID;
    master.m_stats[fe].countPMETasks++;
    ScopedElapsedTime pmeTime(master.m_stats[fe].pmeTime);
#endif
    ProfileScopeEvent(pme);
    master.processPME(*this, master.m_tld[workerThreadId].analysis);
}

void Search::processPME(PME& pme, Search& slave)
{
    /* acquire a motion estimation job, else exit early */
    int meId;
    pme.m_lock.acquire();
    if (pme.m_jobTotal > pme.m_jobAcquired)
    {
        meId = pme.m_jobAcquired++;
        pme.m_lock.release();
    }
    else
    {
        pme.m_lock.release();
        return;
    }

    /* Setup slave Search instance for ME for master's CU */
    if (&slave != this)
    {
        slave.setQP(*m_slice, m_rdCost.m_qp);
        slave.m_slice = m_slice;
        slave.m_frame = m_frame;

        slave.m_me.setSourcePU(*pme.mode.fencYuv, pme.pu.ctuAddr, pme.pu.cuAbsPartIdx, pme.pu.puAbsPartIdx, pme.pu.width, pme.pu.height);
    }

    /* Perform ME, repeat until no more work is available */
    do
    {
        if (meId < m_slice->m_numRefIdx[0])
            slave.singleMotionEstimation(*this, pme.mode, pme.cuGeom, pme.pu, pme.puIdx, 0, meId);
        else
            slave.singleMotionEstimation(*this, pme.mode, pme.cuGeom, pme.pu, pme.puIdx, 1, meId - m_slice->m_numRefIdx[0]);

        meId = -1;
        pme.m_lock.acquire();
        if (pme.m_jobTotal > pme.m_jobAcquired)
            meId = pme.m_jobAcquired++;
        pme.m_lock.release();
    }
    while (meId >= 0);
}

void Search::singleMotionEstimation(Search& master, Mode& interMode, const CUGeom& cuGeom, const PredictionUnit& pu,
                                    int part, int list, int ref)
{
    uint32_t bits = master.m_listSelBits[list] + MVP_IDX_BITS;
    bits += getTUBits(ref, m_slice->m_numRefIdx[list]);

    MV mvc[(MD_ABOVE_LEFT + 1) * 2 + 1];
    int numMvc = interMode.cu.getPMV(interMode.interNeighbours, list, ref, interMode.amvpCand[list][ref], mvc);

    int mvpIdx = 0;
    int merange = m_param->searchRange;
    MotionData* bestME = interMode.bestME[part];

    if (interMode.amvpCand[list][ref][0] != interMode.amvpCand[list][ref][1])
    {
        uint32_t bestCost = MAX_INT;
        for (int i = 0; i < AMVP_NUM_CANDS; i++)
        {
            MV mvCand = interMode.amvpCand[list][ref][i];

            // NOTE: skip mvCand if Y is > merange and -FN>1
            if (m_bFrameParallel && (mvCand.y >= (merange + 1) * 4))
                continue;

            interMode.cu.clipMv(mvCand);

            Yuv& tmpPredYuv = m_rqt[cuGeom.depth].tmpPredYuv;
            predInterLumaPixel(pu, tmpPredYuv, *m_slice->m_refPicList[list][ref]->m_reconPic, mvCand);
            uint32_t cost = m_me.bufSAD(tmpPredYuv.getLumaAddr(pu.puAbsPartIdx), tmpPredYuv.m_size);

            if (bestCost > cost)
            {
                bestCost = cost;
                mvpIdx = i;
            }
        }
    }

    MV mvmin, mvmax, outmv, mvp = interMode.amvpCand[list][ref][mvpIdx];
    setSearchRange(interMode.cu, mvp, merange, mvmin, mvmax);

    int satdCost = m_me.motionEstimate(&m_slice->m_mref[list][ref], mvmin, mvmax, mvp, numMvc, mvc, merange, outmv);

    /* Get total cost of partition, but only include MV bit cost once */
    bits += m_me.bitcost(outmv);
    uint32_t cost = (satdCost - m_me.mvcost(outmv)) + m_rdCost.getCost(bits);

    /* Refine MVP selection, updates: mvp, mvpIdx, bits, cost */
    checkBestMVP(interMode.amvpCand[list][ref], outmv, mvp, mvpIdx, bits, cost);

    /* tie goes to the smallest ref ID, just like --no-pme */
    ScopedLock _lock(master.m_meLock);
    if (cost < bestME[list].cost ||
       (cost == bestME[list].cost && ref < bestME[list].ref))
    {
        bestME[list].mv = outmv;
        bestME[list].mvp = mvp;
        bestME[list].mvpIdx = mvpIdx;
        bestME[list].ref = ref;
        bestME[list].cost = cost;
        bestME[list].bits = bits;
    }
}

/* find the best inter prediction for each PU of specified mode */
void Search::predInterSearch(Mode& interMode, const CUGeom& cuGeom, bool bMergeOnly, bool bChromaSA8D)
{
    ProfileCUScope(interMode.cu, motionEstimationElapsedTime, countMotionEstimate);

    CUData& cu = interMode.cu;
    Yuv* predYuv = &interMode.predYuv;

    MV mvc[(MD_ABOVE_LEFT + 1) * 2 + 1];

    const Slice *slice = m_slice;
    int numPart     = cu.getNumPartInter();
    int numPredDir  = slice->isInterP() ? 1 : 2;
    const int* numRefIdx = slice->m_numRefIdx;
    uint32_t lastMode = 0;
    int      totalmebits = 0;
    int      numME = numRefIdx[0] + numRefIdx[1];
    bool     bTryDistributed = m_param->bDistributeMotionEstimation && numME > 2;
    MV       mvzero(0, 0);
    Yuv&     tmpPredYuv = m_rqt[cuGeom.depth].tmpPredYuv;

    MergeData merge;
    uint32_t mrgCost;
    memset(&merge, 0, sizeof(merge));

    for (int puIdx = 0; puIdx < numPart; puIdx++)
    {
        MotionData* bestME = interMode.bestME[puIdx];
        PredictionUnit pu(cu, cuGeom, puIdx);

        m_me.setSourcePU(*interMode.fencYuv, pu.ctuAddr, pu.cuAbsPartIdx, pu.puAbsPartIdx, pu.width, pu.height);

        /* find best cost merge candidate. note: 2Nx2N merge and bidir are handled as separate modes */
        if (cu.m_partSize[0] != SIZE_2Nx2N)
        {
            mrgCost = mergeEstimation(cu, cuGeom, pu, puIdx, merge);

            if (bMergeOnly && mrgCost != MAX_UINT)
            {
                cu.m_mergeFlag[pu.puAbsPartIdx] = true;
                cu.m_mvpIdx[0][pu.puAbsPartIdx] = merge.index; // merge candidate ID is stored in L0 MVP idx
                cu.setPUInterDir(merge.dir, pu.puAbsPartIdx, puIdx);
                cu.setPUMv(0, merge.mvField[0].mv, pu.puAbsPartIdx, puIdx);
                cu.setPURefIdx(0, merge.mvField[0].refIdx, pu.puAbsPartIdx, puIdx);
                cu.setPUMv(1, merge.mvField[1].mv, pu.puAbsPartIdx, puIdx);
                cu.setPURefIdx(1, merge.mvField[1].refIdx, pu.puAbsPartIdx, puIdx);
                totalmebits += merge.bits;

                motionCompensation(cu, pu, *predYuv, true, bChromaSA8D);
                continue;
            }
        }
        else
            mrgCost = MAX_UINT;

        bestME[0].cost = MAX_UINT;
        bestME[1].cost = MAX_UINT;

        getBlkBits((PartSize)cu.m_partSize[0], slice->isInterP(), puIdx, lastMode, m_listSelBits);
        bool bDoUnidir = true;

        cu.getNeighbourMV(puIdx, pu.puAbsPartIdx, interMode.interNeighbours);

        /* Uni-directional prediction */
        if (m_param->analysisMode == X265_ANALYSIS_LOAD && bestME[0].ref >= 0)
        {
            for (int list = 0; list < numPredDir; list++)
            {
                int ref = bestME[list].ref;
                uint32_t bits = m_listSelBits[list] + MVP_IDX_BITS;
                bits += getTUBits(ref, numRefIdx[list]);

                int numMvc = cu.getPMV(interMode.interNeighbours, list, ref, interMode.amvpCand[list][ref], mvc);

                // Pick the best possible MVP from AMVP candidates based on least residual
                int mvpIdx = 0;
                int merange = m_param->searchRange;

                if (interMode.amvpCand[list][ref][0] != interMode.amvpCand[list][ref][1])
                {
                    uint32_t bestCost = MAX_INT;
                    for (int i = 0; i < AMVP_NUM_CANDS; i++)
                    {
                        MV mvCand = interMode.amvpCand[list][ref][i];

                        // NOTE: skip mvCand if Y is > merange and -FN>1
                        if (m_bFrameParallel && (mvCand.y >= (merange + 1) * 4))
                            continue;

                        cu.clipMv(mvCand);
                        predInterLumaPixel(pu, tmpPredYuv, *slice->m_refPicList[list][ref]->m_reconPic, mvCand);
                        uint32_t cost = m_me.bufSAD(tmpPredYuv.getLumaAddr(pu.puAbsPartIdx), tmpPredYuv.m_size);

                        if (bestCost > cost)
                        {
                            bestCost = cost;
                            mvpIdx = i;
                        }
                    }
                }

                MV mvmin, mvmax, outmv, mvp = interMode.amvpCand[list][ref][mvpIdx];

                int satdCost;
                setSearchRange(cu, mvp, merange, mvmin, mvmax);
                satdCost = m_me.motionEstimate(&slice->m_mref[list][ref], mvmin, mvmax, mvp, numMvc, mvc, merange, outmv);

                /* Get total cost of partition, but only include MV bit cost once */
                bits += m_me.bitcost(outmv);
                uint32_t cost = (satdCost - m_me.mvcost(outmv)) + m_rdCost.getCost(bits);

                /* Refine MVP selection, updates: mvp, mvpIdx, bits, cost */
                checkBestMVP(interMode.amvpCand[list][ref], outmv, mvp, mvpIdx, bits, cost);

                if (cost < bestME[list].cost)
                {
                    bestME[list].mv = outmv;
                    bestME[list].mvp = mvp;
                    bestME[list].mvpIdx = mvpIdx;
                    bestME[list].cost = cost;
                    bestME[list].bits = bits;
                }
            }
            bDoUnidir = false;
        }
        else if (bTryDistributed)
        {
            PME pme(*this, interMode, cuGeom, pu, puIdx);
            pme.m_jobTotal = numME;
            pme.m_jobAcquired = 1; /* reserve L0-0 */

            if (pme.tryBondPeers(*m_frame->m_encData->m_jobProvider, numME - 1))
            {
                processPME(pme, *this);

                singleMotionEstimation(*this, interMode, cuGeom, pu, puIdx, 0, 0); /* L0-0 */

                bDoUnidir = false;

                ProfileCUScopeNamed(pmeWaitScope, interMode.cu, pmeBlockTime, countPMEMasters);
                pme.waitForExit();
            }

            /* if no peer threads were bonded, fall back to doing unidirectional
             * searches ourselves without overhead of singleMotionEstimation() */
        }
        if (bDoUnidir)
        {
            for (int list = 0; list < numPredDir; list++)
            {
                for (int ref = 0; ref < numRefIdx[list]; ref++)
                {
                    uint32_t bits = m_listSelBits[list] + MVP_IDX_BITS;
                    bits += getTUBits(ref, numRefIdx[list]);

                    int numMvc = cu.getPMV(interMode.interNeighbours, list, ref, interMode.amvpCand[list][ref], mvc);

                    // Pick the best possible MVP from AMVP candidates based on least residual
                    int mvpIdx = 0;
                    int merange = m_param->searchRange;

                    if (interMode.amvpCand[list][ref][0] != interMode.amvpCand[list][ref][1])
                    {
                        uint32_t bestCost = MAX_INT;
                        for (int i = 0; i < AMVP_NUM_CANDS; i++)
                        {
                            MV mvCand = interMode.amvpCand[list][ref][i];

                            // NOTE: skip mvCand if Y is > merange and -FN>1
                            if (m_bFrameParallel && (mvCand.y >= (merange + 1) * 4))
                                continue;

                            cu.clipMv(mvCand);
                            predInterLumaPixel(pu, tmpPredYuv, *slice->m_refPicList[list][ref]->m_reconPic, mvCand);
                            uint32_t cost = m_me.bufSAD(tmpPredYuv.getLumaAddr(pu.puAbsPartIdx), tmpPredYuv.m_size);

                            if (bestCost > cost)
                            {
                                bestCost = cost;
                                mvpIdx = i;
                            }
                        }
                    }

                    MV mvmin, mvmax, outmv, mvp = interMode.amvpCand[list][ref][mvpIdx];

                    setSearchRange(cu, mvp, merange, mvmin, mvmax);
                    int satdCost = m_me.motionEstimate(&slice->m_mref[list][ref], mvmin, mvmax, mvp, numMvc, mvc, merange, outmv);

                    /* Get total cost of partition, but only include MV bit cost once */
                    bits += m_me.bitcost(outmv);
                    uint32_t cost = (satdCost - m_me.mvcost(outmv)) + m_rdCost.getCost(bits);

                    /* Refine MVP selection, updates: mvp, mvpIdx, bits, cost */
                    checkBestMVP(interMode.amvpCand[list][ref], outmv, mvp, mvpIdx, bits, cost);

                    if (cost < bestME[list].cost)
                    {
                        bestME[list].mv = outmv;
                        bestME[list].mvp = mvp;
                        bestME[list].mvpIdx = mvpIdx;
                        bestME[list].ref = ref;
                        bestME[list].cost = cost;
                        bestME[list].bits = bits;
                    }
                }
            }
        }

        /* Bi-directional prediction */
        MotionData bidir[2];
        uint32_t bidirCost = MAX_UINT;
        int bidirBits = 0;

        if (slice->isInterB() && !cu.isBipredRestriction() &&  /* biprediction is possible for this PU */
            cu.m_partSize[pu.puAbsPartIdx] != SIZE_2Nx2N &&    /* 2Nx2N biprediction is handled elsewhere */
            bestME[0].cost != MAX_UINT && bestME[1].cost != MAX_UINT)
        {
            bidir[0] = bestME[0];
            bidir[1] = bestME[1];

            int satdCost;

            if (m_me.bChromaSATD)
            {
                cu.m_mv[0][pu.puAbsPartIdx] = bidir[0].mv;
                cu.m_refIdx[0][pu.puAbsPartIdx] = (int8_t)bidir[0].ref;
                cu.m_mv[1][pu.puAbsPartIdx] = bidir[1].mv;
                cu.m_refIdx[1][pu.puAbsPartIdx] = (int8_t)bidir[1].ref;
                motionCompensation(cu, pu, tmpPredYuv, true, true);

                satdCost = m_me.bufSATD(tmpPredYuv.getLumaAddr(pu.puAbsPartIdx), tmpPredYuv.m_size) +
                           m_me.bufChromaSATD(tmpPredYuv, pu.puAbsPartIdx);
            }
            else
            {
                PicYuv* refPic0 = slice->m_refPicList[0][bestME[0].ref]->m_reconPic;
                PicYuv* refPic1 = slice->m_refPicList[1][bestME[1].ref]->m_reconPic;
                Yuv* bidirYuv = m_rqt[cuGeom.depth].bidirPredYuv;

                /* Generate reference subpels */
                predInterLumaPixel(pu, bidirYuv[0], *refPic0, bestME[0].mv);
                predInterLumaPixel(pu, bidirYuv[1], *refPic1, bestME[1].mv);

                primitives.pu[m_me.partEnum].pixelavg_pp(tmpPredYuv.m_buf[0], tmpPredYuv.m_size, bidirYuv[0].getLumaAddr(pu.puAbsPartIdx), bidirYuv[0].m_size,
                                                                                                 bidirYuv[1].getLumaAddr(pu.puAbsPartIdx), bidirYuv[1].m_size, 32);
                satdCost = m_me.bufSATD(tmpPredYuv.m_buf[0], tmpPredYuv.m_size);
            }

            bidirBits = bestME[0].bits + bestME[1].bits + m_listSelBits[2] - (m_listSelBits[0] + m_listSelBits[1]);
            bidirCost = satdCost + m_rdCost.getCost(bidirBits);

            bool bTryZero = bestME[0].mv.notZero() || bestME[1].mv.notZero();
            if (bTryZero)
            {
                /* Do not try zero MV if unidir motion predictors are beyond
                 * valid search area */
                MV mvmin, mvmax;
                int merange = X265_MAX(m_param->sourceWidth, m_param->sourceHeight);
                setSearchRange(cu, mvzero, merange, mvmin, mvmax);
                mvmax.y += 2; // there is some pad for subpel refine
                mvmin <<= 2;
                mvmax <<= 2;

                bTryZero &= bestME[0].mvp.checkRange(mvmin, mvmax);
                bTryZero &= bestME[1].mvp.checkRange(mvmin, mvmax);
            }
            if (bTryZero)
            {
                /* coincident blocks of the two reference pictures */
                if (m_me.bChromaSATD)
                {
                    cu.m_mv[0][pu.puAbsPartIdx] = mvzero;
                    cu.m_refIdx[0][pu.puAbsPartIdx] = (int8_t)bidir[0].ref;
                    cu.m_mv[1][pu.puAbsPartIdx] = mvzero;
                    cu.m_refIdx[1][pu.puAbsPartIdx] = (int8_t)bidir[1].ref;
                    motionCompensation(cu, pu, tmpPredYuv, true, true);

                    satdCost = m_me.bufSATD(tmpPredYuv.getLumaAddr(pu.puAbsPartIdx), tmpPredYuv.m_size) +
                               m_me.bufChromaSATD(tmpPredYuv, pu.puAbsPartIdx);
                }
                else
                {
                    const pixel* ref0 = m_slice->m_mref[0][bestME[0].ref].getLumaAddr(pu.ctuAddr, pu.cuAbsPartIdx + pu.puAbsPartIdx);
                    const pixel* ref1 = m_slice->m_mref[1][bestME[1].ref].getLumaAddr(pu.ctuAddr, pu.cuAbsPartIdx + pu.puAbsPartIdx);
                    intptr_t refStride = slice->m_mref[0][0].lumaStride;

                    primitives.pu[m_me.partEnum].pixelavg_pp(tmpPredYuv.m_buf[0], tmpPredYuv.m_size, ref0, refStride, ref1, refStride, 32);
                    satdCost = m_me.bufSATD(tmpPredYuv.m_buf[0], tmpPredYuv.m_size);
                }

                MV mvp0 = bestME[0].mvp;
                int mvpIdx0 = bestME[0].mvpIdx;
                uint32_t bits0 = bestME[0].bits - m_me.bitcost(bestME[0].mv, mvp0) + m_me.bitcost(mvzero, mvp0);

                MV mvp1 = bestME[1].mvp;
                int mvpIdx1 = bestME[1].mvpIdx;
                uint32_t bits1 = bestME[1].bits - m_me.bitcost(bestME[1].mv, mvp1) + m_me.bitcost(mvzero, mvp1);

                uint32_t cost = satdCost + m_rdCost.getCost(bits0) + m_rdCost.getCost(bits1);

                /* refine MVP selection for zero mv, updates: mvp, mvpidx, bits, cost */
                checkBestMVP(interMode.amvpCand[0][bestME[0].ref], mvzero, mvp0, mvpIdx0, bits0, cost);
                checkBestMVP(interMode.amvpCand[1][bestME[1].ref], mvzero, mvp1, mvpIdx1, bits1, cost);

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
        if (mrgCost < bidirCost && mrgCost < bestME[0].cost && mrgCost < bestME[1].cost)
        {
            cu.m_mergeFlag[pu.puAbsPartIdx] = true;
            cu.m_mvpIdx[0][pu.puAbsPartIdx] = merge.index; /* merge candidate ID is stored in L0 MVP idx */
            cu.setPUInterDir(merge.dir, pu.puAbsPartIdx, puIdx);
            cu.setPUMv(0, merge.mvField[0].mv, pu.puAbsPartIdx, puIdx);
            cu.setPURefIdx(0, merge.mvField[0].refIdx, pu.puAbsPartIdx, puIdx);
            cu.setPUMv(1, merge.mvField[1].mv, pu.puAbsPartIdx, puIdx);
            cu.setPURefIdx(1, merge.mvField[1].refIdx, pu.puAbsPartIdx, puIdx);

            totalmebits += merge.bits;
        }
        else if (bidirCost < bestME[0].cost && bidirCost < bestME[1].cost)
        {
            lastMode = 2;

            cu.m_mergeFlag[pu.puAbsPartIdx] = false;
            cu.setPUInterDir(3, pu.puAbsPartIdx, puIdx);
            cu.setPUMv(0, bidir[0].mv, pu.puAbsPartIdx, puIdx);
            cu.setPURefIdx(0, bestME[0].ref, pu.puAbsPartIdx, puIdx);
            cu.m_mvd[0][pu.puAbsPartIdx] = bidir[0].mv - bidir[0].mvp;
            cu.m_mvpIdx[0][pu.puAbsPartIdx] = bidir[0].mvpIdx;

            cu.setPUMv(1, bidir[1].mv, pu.puAbsPartIdx, puIdx);
            cu.setPURefIdx(1, bestME[1].ref, pu.puAbsPartIdx, puIdx);
            cu.m_mvd[1][pu.puAbsPartIdx] = bidir[1].mv - bidir[1].mvp;
            cu.m_mvpIdx[1][pu.puAbsPartIdx] = bidir[1].mvpIdx;

            totalmebits += bidirBits;
        }
        else if (bestME[0].cost <= bestME[1].cost)
        {
            lastMode = 0;

            cu.m_mergeFlag[pu.puAbsPartIdx] = false;
            cu.setPUInterDir(1, pu.puAbsPartIdx, puIdx);
            cu.setPUMv(0, bestME[0].mv, pu.puAbsPartIdx, puIdx);
            cu.setPURefIdx(0, bestME[0].ref, pu.puAbsPartIdx, puIdx);
            cu.m_mvd[0][pu.puAbsPartIdx] = bestME[0].mv - bestME[0].mvp;
            cu.m_mvpIdx[0][pu.puAbsPartIdx] = bestME[0].mvpIdx;

            cu.setPURefIdx(1, REF_NOT_VALID, pu.puAbsPartIdx, puIdx);
            cu.setPUMv(1, mvzero, pu.puAbsPartIdx, puIdx);

            totalmebits += bestME[0].bits;
        }
        else
        {
            lastMode = 1;

            cu.m_mergeFlag[pu.puAbsPartIdx] = false;
            cu.setPUInterDir(2, pu.puAbsPartIdx, puIdx);
            cu.setPUMv(1, bestME[1].mv, pu.puAbsPartIdx, puIdx);
            cu.setPURefIdx(1, bestME[1].ref, pu.puAbsPartIdx, puIdx);
            cu.m_mvd[1][pu.puAbsPartIdx] = bestME[1].mv - bestME[1].mvp;
            cu.m_mvpIdx[1][pu.puAbsPartIdx] = bestME[1].mvpIdx;

            cu.setPURefIdx(0, REF_NOT_VALID, pu.puAbsPartIdx, puIdx);
            cu.setPUMv(0, mvzero, pu.puAbsPartIdx, puIdx);

            totalmebits += bestME[1].bits;
        }

        motionCompensation(cu, pu, *predYuv, true, bChromaSA8D);
    }
    X265_CHECK(interMode.ok(), "inter mode is not ok");
    interMode.sa8dBits += totalmebits;
}

void Search::getBlkBits(PartSize cuMode, bool bPSlice, int partIdx, uint32_t lastMode, uint32_t blockBit[3])
{
    if (cuMode == SIZE_2Nx2N)
    {
        blockBit[0] = (!bPSlice) ? 3 : 1;
        blockBit[1] = 3;
        blockBit[2] = 5;
    }
    else if (cuMode == SIZE_2NxN || cuMode == SIZE_2NxnU || cuMode == SIZE_2NxnD)
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
            memcpy(blockBit, listBits[partIdx][lastMode], 3 * sizeof(uint32_t));
    }
    else if (cuMode == SIZE_Nx2N || cuMode == SIZE_nLx2N || cuMode == SIZE_nRx2N)
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
            memcpy(blockBit, listBits[partIdx][lastMode], 3 * sizeof(uint32_t));
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
    CUData& cu = interMode.cu;
    Yuv* reconYuv = &interMode.reconYuv;
    const Yuv* fencYuv = interMode.fencYuv;

    X265_CHECK(!cu.isIntra(0), "intra CU not expected\n");

    uint32_t depth  = cu.m_cuDepth[0];

    // No residual coding : SKIP mode

    cu.setPredModeSubParts(MODE_SKIP);
    cu.clearCbf();
    cu.setTUDepthSubParts(0, 0, depth);

    reconYuv->copyFromYuv(interMode.predYuv);

    // Luma
    int part = partitionFromLog2Size(cu.m_log2CUSize[0]);
    interMode.distortion = primitives.cu[part].sse_pp(fencYuv->m_buf[0], fencYuv->m_size, reconYuv->m_buf[0], reconYuv->m_size);
    // Chroma
    interMode.distortion += m_rdCost.scaleChromaDist(1, primitives.chroma[m_csp].cu[part].sse_pp(fencYuv->m_buf[1], fencYuv->m_csize, reconYuv->m_buf[1], reconYuv->m_csize));
    interMode.distortion += m_rdCost.scaleChromaDist(2, primitives.chroma[m_csp].cu[part].sse_pp(fencYuv->m_buf[2], fencYuv->m_csize, reconYuv->m_buf[2], reconYuv->m_csize));

    m_entropyCoder.load(m_rqt[depth].cur);
    m_entropyCoder.resetBits();
    if (m_slice->m_pps->bTransquantBypassEnabled)
        m_entropyCoder.codeCUTransquantBypassFlag(cu.m_tqBypass[0]);
    m_entropyCoder.codeSkipFlag(cu, 0);
    m_entropyCoder.codeMergeIndex(cu, 0);

    interMode.mvBits = m_entropyCoder.getNumberOfWrittenBits();
    interMode.coeffBits = 0;
    interMode.totalBits = interMode.mvBits;
    if (m_rdCost.m_psyRd)
        interMode.psyEnergy = m_rdCost.psyCost(part, fencYuv->m_buf[0], fencYuv->m_size, reconYuv->m_buf[0], reconYuv->m_size);

    updateModeCost(interMode);
    m_entropyCoder.store(interMode.contexts);
}

/* encode residual and calculate rate-distortion for a CU block.
 * Note: this function overwrites the RD cost variables of interMode, but leaves the sa8d cost unharmed */
void Search::encodeResAndCalcRdInterCU(Mode& interMode, const CUGeom& cuGeom)
{
    ProfileCUScope(interMode.cu, interRDOElapsedTime[cuGeom.depth], countInterRDO[cuGeom.depth]);

    CUData& cu = interMode.cu;
    Yuv* reconYuv = &interMode.reconYuv;
    Yuv* predYuv = &interMode.predYuv;
    uint32_t depth = cuGeom.depth;
    ShortYuv* resiYuv = &m_rqt[depth].tmpResiYuv;
    const Yuv* fencYuv = interMode.fencYuv;

    X265_CHECK(!cu.isIntra(0), "intra CU not expected\n");

    uint32_t log2CUSize = cuGeom.log2CUSize;
    int sizeIdx = log2CUSize - 2;

    uint32_t tqBypass = cu.m_tqBypass[0];
    m_quant.setQPforQuant(interMode.cu);

    resiYuv->subtract(*fencYuv, *predYuv, log2CUSize);

    uint32_t tuDepthRange[2];
    cu.getInterTUQtDepthRange(tuDepthRange, 0);

    m_entropyCoder.load(m_rqt[depth].cur);

    Cost costs;
    estimateResidualQT(interMode, cuGeom, 0, 0, *resiYuv, costs, tuDepthRange);

    if (!tqBypass)
    {
        uint32_t cbf0Dist = primitives.cu[sizeIdx].sse_pp(fencYuv->m_buf[0], fencYuv->m_size, predYuv->m_buf[0], predYuv->m_size);
        cbf0Dist += m_rdCost.scaleChromaDist(1, primitives.chroma[m_csp].cu[sizeIdx].sse_pp(fencYuv->m_buf[1], predYuv->m_csize, predYuv->m_buf[1], predYuv->m_csize));
        cbf0Dist += m_rdCost.scaleChromaDist(2, primitives.chroma[m_csp].cu[sizeIdx].sse_pp(fencYuv->m_buf[2], predYuv->m_csize, predYuv->m_buf[2], predYuv->m_csize));

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

        if (cbf0Cost < costs.rdcost)
        {
            cu.clearCbf();
            cu.setTUDepthSubParts(0, 0, depth);
        }
    }

    if (cu.getQtRootCbf(0))
        saveResidualQTData(cu, *resiYuv, 0, 0);

    /* calculate signal bits for inter/merge/skip coded CU */
    m_entropyCoder.load(m_rqt[depth].cur);

    m_entropyCoder.resetBits();
    if (m_slice->m_pps->bTransquantBypassEnabled)
        m_entropyCoder.codeCUTransquantBypassFlag(tqBypass);

    uint32_t coeffBits, bits;
    if (cu.m_mergeFlag[0] && cu.m_partSize[0] == SIZE_2Nx2N && !cu.getQtRootCbf(0))
    {
        cu.setPredModeSubParts(MODE_SKIP);

        /* Merge/Skip */
        m_entropyCoder.codeSkipFlag(cu, 0);
        m_entropyCoder.codeMergeIndex(cu, 0);
        coeffBits = 0;
        bits = m_entropyCoder.getNumberOfWrittenBits();
    }
    else
    {
        m_entropyCoder.codeSkipFlag(cu, 0);
        m_entropyCoder.codePredMode(cu.m_predMode[0]);
        m_entropyCoder.codePartSize(cu, 0, cuGeom.depth);
        m_entropyCoder.codePredInfo(cu, 0);
        uint32_t mvBits = m_entropyCoder.getNumberOfWrittenBits();

        bool bCodeDQP = m_slice->m_pps->bUseDQP;
        m_entropyCoder.codeCoeff(cu, 0, bCodeDQP, tuDepthRange);
        bits = m_entropyCoder.getNumberOfWrittenBits();

        coeffBits = bits - mvBits;
    }

    m_entropyCoder.store(interMode.contexts);

    if (cu.getQtRootCbf(0))
        reconYuv->addClip(*predYuv, *resiYuv, log2CUSize);
    else
        reconYuv->copyFromYuv(*predYuv);

    // update with clipped distortion and cost (qp estimation loop uses unclipped values)
    uint32_t bestDist = primitives.cu[sizeIdx].sse_pp(fencYuv->m_buf[0], fencYuv->m_size, reconYuv->m_buf[0], reconYuv->m_size);
    bestDist += m_rdCost.scaleChromaDist(1, primitives.chroma[m_csp].cu[sizeIdx].sse_pp(fencYuv->m_buf[1], fencYuv->m_csize, reconYuv->m_buf[1], reconYuv->m_csize));
    bestDist += m_rdCost.scaleChromaDist(2, primitives.chroma[m_csp].cu[sizeIdx].sse_pp(fencYuv->m_buf[2], fencYuv->m_csize, reconYuv->m_buf[2], reconYuv->m_csize));
    if (m_rdCost.m_psyRd)
        interMode.psyEnergy = m_rdCost.psyCost(sizeIdx, fencYuv->m_buf[0], fencYuv->m_size, reconYuv->m_buf[0], reconYuv->m_size);

    interMode.totalBits = bits;
    interMode.distortion = bestDist;
    interMode.coeffBits = coeffBits;
    interMode.mvBits = bits - coeffBits;
    updateModeCost(interMode);
    checkDQP(interMode.cu, cuGeom);
}

void Search::residualTransformQuantInter(Mode& mode, const CUGeom& cuGeom, uint32_t absPartIdx, uint32_t tuDepth, const uint32_t depthRange[2])
{
    uint32_t depth = cuGeom.depth + tuDepth;
    CUData& cu = mode.cu;
    uint32_t log2TrSize = cuGeom.log2CUSize - tuDepth;

    bool bCheckFull = log2TrSize <= depthRange[1];
    if (cu.m_partSize[0] != SIZE_2Nx2N && !tuDepth && log2TrSize > depthRange[0])
        bCheckFull = false;

    if (bCheckFull)
    {
        // code full block
        uint32_t log2TrSizeC = log2TrSize - m_hChromaShift;
        bool bCodeChroma = true;
        uint32_t tuDepthC = tuDepth;
        if (log2TrSizeC < 2)
        {
            X265_CHECK(log2TrSize == 2 && m_csp != X265_CSP_I444 && tuDepth, "invalid tuDepth\n");
            log2TrSizeC = 2;
            tuDepthC--;
            bCodeChroma = !(absPartIdx & 3);
        }

        uint32_t absPartIdxStep = cuGeom.numPartitions >> tuDepthC * 2;
        uint32_t setCbf = 1 << tuDepth;

        uint32_t coeffOffsetY = absPartIdx << (LOG2_UNIT_SIZE * 2);
        coeff_t* coeffCurY = cu.m_trCoeff[0] + coeffOffsetY;

        uint32_t sizeIdx  = log2TrSize  - 2;

        cu.setTUDepthSubParts(tuDepth, absPartIdx, depth);
        cu.setTransformSkipSubParts(0, TEXT_LUMA, absPartIdx, depth);

        ShortYuv& resiYuv = m_rqt[cuGeom.depth].tmpResiYuv;
        const Yuv* fencYuv = mode.fencYuv;

        int16_t* curResiY = resiYuv.getLumaAddr(absPartIdx);
        uint32_t strideResiY = resiYuv.m_size;

        const pixel* fenc = fencYuv->getLumaAddr(absPartIdx);
        uint32_t numSigY = m_quant.transformNxN(cu, fenc, fencYuv->m_size, curResiY, strideResiY, coeffCurY, log2TrSize, TEXT_LUMA, absPartIdx, false);

        if (numSigY)
        {
            m_quant.invtransformNxN(curResiY, strideResiY, coeffCurY, log2TrSize, TEXT_LUMA, false, false, numSigY);
            cu.setCbfSubParts(setCbf, TEXT_LUMA, absPartIdx, depth);
        }
        else
        {
            primitives.cu[sizeIdx].blockfill_s(curResiY, strideResiY, 0);
            cu.setCbfSubParts(0, TEXT_LUMA, absPartIdx, depth);
        }

        if (bCodeChroma)
        {
            uint32_t sizeIdxC = log2TrSizeC - 2;
            uint32_t strideResiC = resiYuv.m_csize;

            uint32_t coeffOffsetC = coeffOffsetY >> (m_hChromaShift + m_vChromaShift);
            coeff_t* coeffCurU = cu.m_trCoeff[1] + coeffOffsetC;
            coeff_t* coeffCurV = cu.m_trCoeff[2] + coeffOffsetC;
            bool splitIntoSubTUs = (m_csp == X265_CSP_I422);

            TURecurse tuIterator(splitIntoSubTUs ? VERTICAL_SPLIT : DONT_SPLIT, absPartIdxStep, absPartIdx);
            do
            {
                uint32_t absPartIdxC = tuIterator.absPartIdxTURelCU;
                uint32_t subTUOffset = tuIterator.section << (log2TrSizeC * 2);

                cu.setTransformSkipPartRange(0, TEXT_CHROMA_U, absPartIdxC, tuIterator.absPartIdxStep);
                cu.setTransformSkipPartRange(0, TEXT_CHROMA_V, absPartIdxC, tuIterator.absPartIdxStep);

                int16_t* curResiU = resiYuv.getCbAddr(absPartIdxC);
                const pixel* fencCb = fencYuv->getCbAddr(absPartIdxC);
                uint32_t numSigU = m_quant.transformNxN(cu, fencCb, fencYuv->m_csize, curResiU, strideResiC, coeffCurU + subTUOffset, log2TrSizeC, TEXT_CHROMA_U, absPartIdxC, false);
                if (numSigU)
                {
                    m_quant.invtransformNxN(curResiU, strideResiC, coeffCurU + subTUOffset, log2TrSizeC, TEXT_CHROMA_U, false, false, numSigU);
                    cu.setCbfPartRange(setCbf, TEXT_CHROMA_U, absPartIdxC, tuIterator.absPartIdxStep);
                }
                else
                {
                    primitives.cu[sizeIdxC].blockfill_s(curResiU, strideResiC, 0);
                    cu.setCbfPartRange(0, TEXT_CHROMA_U, absPartIdxC, tuIterator.absPartIdxStep);
                }

                int16_t* curResiV = resiYuv.getCrAddr(absPartIdxC);
                const pixel* fencCr = fencYuv->getCrAddr(absPartIdxC);
                uint32_t numSigV = m_quant.transformNxN(cu, fencCr, fencYuv->m_csize, curResiV, strideResiC, coeffCurV + subTUOffset, log2TrSizeC, TEXT_CHROMA_V, absPartIdxC, false);
                if (numSigV)
                {
                    m_quant.invtransformNxN(curResiV, strideResiC, coeffCurV + subTUOffset, log2TrSizeC, TEXT_CHROMA_V, false, false, numSigV);
                    cu.setCbfPartRange(setCbf, TEXT_CHROMA_V, absPartIdxC, tuIterator.absPartIdxStep);
                }
                else
                {
                    primitives.cu[sizeIdxC].blockfill_s(curResiV, strideResiC, 0);
                    cu.setCbfPartRange(0, TEXT_CHROMA_V, absPartIdxC, tuIterator.absPartIdxStep);
                }
            }
            while (tuIterator.isNextSection());

            if (splitIntoSubTUs)
            {
                offsetSubTUCBFs(cu, TEXT_CHROMA_U, tuDepth, absPartIdx);
                offsetSubTUCBFs(cu, TEXT_CHROMA_V, tuDepth, absPartIdx);
            }
        }
    }
    else
    {
        X265_CHECK(log2TrSize > depthRange[0], "residualTransformQuantInter recursion check failure\n");

        uint32_t qNumParts = 1 << (log2TrSize - 1 - LOG2_UNIT_SIZE) * 2;
        uint32_t ycbf = 0, ucbf = 0, vcbf = 0;
        for (uint32_t qIdx = 0, qPartIdx = absPartIdx; qIdx < 4; ++qIdx, qPartIdx += qNumParts)
        {
            residualTransformQuantInter(mode, cuGeom, qPartIdx, tuDepth + 1, depthRange);
            ycbf |= cu.getCbf(qPartIdx, TEXT_LUMA,     tuDepth + 1);
            ucbf |= cu.getCbf(qPartIdx, TEXT_CHROMA_U, tuDepth + 1);
            vcbf |= cu.getCbf(qPartIdx, TEXT_CHROMA_V, tuDepth + 1);
        }
        for (uint32_t i = 0; i < 4 * qNumParts; ++i)
        {
            cu.m_cbf[0][absPartIdx + i] |= ycbf << tuDepth;
            cu.m_cbf[1][absPartIdx + i] |= ucbf << tuDepth;
            cu.m_cbf[2][absPartIdx + i] |= vcbf << tuDepth;
        }
    }
}

uint64_t Search::estimateNullCbfCost(uint32_t &dist, uint32_t &psyEnergy, uint32_t tuDepth, TextType compId)
{
    uint32_t nullBits = m_entropyCoder.estimateCbfBits(0, compId, tuDepth);

    if (m_rdCost.m_psyRd)
        return m_rdCost.calcPsyRdCost(dist, nullBits, psyEnergy);
    else
        return m_rdCost.calcRdCost(dist, nullBits);
}

void Search::estimateResidualQT(Mode& mode, const CUGeom& cuGeom, uint32_t absPartIdx, uint32_t tuDepth, ShortYuv& resiYuv, Cost& outCosts, const uint32_t depthRange[2])
{
    CUData& cu = mode.cu;
    uint32_t depth = cuGeom.depth + tuDepth;
    uint32_t log2TrSize = cuGeom.log2CUSize - tuDepth;

    bool bCheckSplit = log2TrSize > depthRange[0];
    bool bCheckFull = log2TrSize <= depthRange[1];
    bool bSplitPresentFlag = bCheckSplit && bCheckFull;

    if (cu.m_partSize[0] != SIZE_2Nx2N && !tuDepth && bCheckSplit)
        bCheckFull = false;

    X265_CHECK(bCheckFull || bCheckSplit, "check-full or check-split must be set\n");

    uint32_t log2TrSizeC = log2TrSize - m_hChromaShift;
    bool bCodeChroma = true;
    uint32_t tuDepthC = tuDepth;
    if (log2TrSizeC < 2)
    {
        X265_CHECK(log2TrSize == 2 && m_csp != X265_CSP_I444 && tuDepth, "invalid tuDepth\n");
        log2TrSizeC = 2;
        tuDepthC--;
        bCodeChroma = !(absPartIdx & 3);
    }

    // code full block
    Cost fullCost;
    fullCost.rdcost = MAX_INT64;

    uint8_t  cbfFlag[MAX_NUM_COMPONENT][2 /*0 = top (or whole TU for non-4:2:2) sub-TU, 1 = bottom sub-TU*/] = { { 0, 0 }, {0, 0}, {0, 0} };
    uint32_t numSig[MAX_NUM_COMPONENT][2 /*0 = top (or whole TU for non-4:2:2) sub-TU, 1 = bottom sub-TU*/] = { { 0, 0 }, {0, 0}, {0, 0} };
    uint32_t singleBits[MAX_NUM_COMPONENT][2 /*0 = top (or whole TU for non-4:2:2) sub-TU, 1 = bottom sub-TU*/] = { { 0, 0 }, { 0, 0 }, { 0, 0 } };
    uint32_t singleDist[MAX_NUM_COMPONENT][2 /*0 = top (or whole TU for non-4:2:2) sub-TU, 1 = bottom sub-TU*/] = { { 0, 0 }, { 0, 0 }, { 0, 0 } };
    uint32_t singlePsyEnergy[MAX_NUM_COMPONENT][2 /*0 = top (or whole TU for non-4:2:2) sub-TU, 1 = bottom sub-TU*/] = { { 0, 0 }, { 0, 0 }, { 0, 0 } };
    uint32_t bestTransformMode[MAX_NUM_COMPONENT][2 /*0 = top (or whole TU for non-4:2:2) sub-TU, 1 = bottom sub-TU*/] = { { 0, 0 }, { 0, 0 }, { 0, 0 } };
    uint64_t minCost[MAX_NUM_COMPONENT][2 /*0 = top (or whole TU for non-4:2:2) sub-TU, 1 = bottom sub-TU*/] = { { MAX_INT64, MAX_INT64 }, {MAX_INT64, MAX_INT64}, {MAX_INT64, MAX_INT64} };

    m_entropyCoder.store(m_rqt[depth].rqtRoot);

    uint32_t trSize = 1 << log2TrSize;
    const bool splitIntoSubTUs = (m_csp == X265_CSP_I422);
    uint32_t absPartIdxStep = cuGeom.numPartitions >> tuDepthC * 2;
    const Yuv* fencYuv = mode.fencYuv;

    // code full block
    if (bCheckFull)
    {
        uint32_t trSizeC = 1 << log2TrSizeC;
        int partSize  = partitionFromLog2Size(log2TrSize);
        int partSizeC = partitionFromLog2Size(log2TrSizeC);
        const uint32_t qtLayer = log2TrSize - 2;
        uint32_t coeffOffsetY = absPartIdx << (LOG2_UNIT_SIZE * 2);
        coeff_t* coeffCurY = m_rqt[qtLayer].coeffRQT[0] + coeffOffsetY;

        bool checkTransformSkip   = m_slice->m_pps->bTransformSkipEnabled && !cu.m_tqBypass[0];
        bool checkTransformSkipY  = checkTransformSkip && log2TrSize  <= MAX_LOG2_TS_SIZE;
        bool checkTransformSkipC = checkTransformSkip && log2TrSizeC <= MAX_LOG2_TS_SIZE;

        cu.setTUDepthSubParts(tuDepth, absPartIdx, depth);
        cu.setTransformSkipSubParts(0, TEXT_LUMA, absPartIdx, depth);

        if (m_bEnableRDOQ)
            m_entropyCoder.estBit(m_entropyCoder.m_estBitsSbac, log2TrSize, true);

        const pixel* fenc = fencYuv->getLumaAddr(absPartIdx);
        int16_t* resi = resiYuv.getLumaAddr(absPartIdx);
        numSig[TEXT_LUMA][0] = m_quant.transformNxN(cu, fenc, fencYuv->m_size, resi, resiYuv.m_size, coeffCurY, log2TrSize, TEXT_LUMA, absPartIdx, false);
        cbfFlag[TEXT_LUMA][0] = !!numSig[TEXT_LUMA][0];

        m_entropyCoder.resetBits();

        if (bSplitPresentFlag && log2TrSize > depthRange[0])
            m_entropyCoder.codeTransformSubdivFlag(0, 5 - log2TrSize);
        fullCost.bits = m_entropyCoder.getNumberOfWrittenBits();

        // Coding luma cbf flag has been removed from here. The context for cbf flag is different for each depth.
        // So it is valid if we encode coefficients and then cbfs at least for analysis.
//        m_entropyCoder.codeQtCbfLuma(cbfFlag[TEXT_LUMA][0], tuDepth);
        if (cbfFlag[TEXT_LUMA][0])
            m_entropyCoder.codeCoeffNxN(cu, coeffCurY, absPartIdx, log2TrSize, TEXT_LUMA);

        uint32_t singleBitsPrev = m_entropyCoder.getNumberOfWrittenBits();
        singleBits[TEXT_LUMA][0] = singleBitsPrev - fullCost.bits;

        X265_CHECK(log2TrSize <= 5, "log2TrSize is too large\n");
        uint32_t distY = primitives.cu[partSize].ssd_s(resiYuv.getLumaAddr(absPartIdx), resiYuv.m_size);
        uint32_t psyEnergyY = 0;
        if (m_rdCost.m_psyRd)
            psyEnergyY = m_rdCost.psyCost(partSize, resiYuv.getLumaAddr(absPartIdx), resiYuv.m_size, (int16_t*)zeroShort, 0);

        int16_t* curResiY    = m_rqt[qtLayer].resiQtYuv.getLumaAddr(absPartIdx);
        uint32_t strideResiY = m_rqt[qtLayer].resiQtYuv.m_size;

        if (cbfFlag[TEXT_LUMA][0])
        {
            m_quant.invtransformNxN(curResiY, strideResiY, coeffCurY, log2TrSize, TEXT_LUMA, false, false, numSig[TEXT_LUMA][0]); //this is for inter mode only

            // non-zero cost calculation for luma - This is an approximation
            // finally we have to encode correct cbf after comparing with null cost
            const uint32_t nonZeroDistY = primitives.cu[partSize].sse_ss(resiYuv.getLumaAddr(absPartIdx), resiYuv.m_size, curResiY, strideResiY);
            uint32_t nzCbfBitsY = m_entropyCoder.estimateCbfBits(cbfFlag[TEXT_LUMA][0], TEXT_LUMA, tuDepth);
            uint32_t nonZeroPsyEnergyY = 0; uint64_t singleCostY = 0;
            if (m_rdCost.m_psyRd)
            {
                nonZeroPsyEnergyY = m_rdCost.psyCost(partSize, resiYuv.getLumaAddr(absPartIdx), resiYuv.m_size, curResiY, strideResiY);
                singleCostY = m_rdCost.calcPsyRdCost(nonZeroDistY, nzCbfBitsY + singleBits[TEXT_LUMA][0], nonZeroPsyEnergyY);
            }
            else
                singleCostY = m_rdCost.calcRdCost(nonZeroDistY, nzCbfBitsY + singleBits[TEXT_LUMA][0]);

            if (cu.m_tqBypass[0])
            {
                singleDist[TEXT_LUMA][0] = nonZeroDistY;
                singlePsyEnergy[TEXT_LUMA][0] = nonZeroPsyEnergyY;
            }
            else
            {
                // zero-cost calculation for luma. This is an approximation
                // Initial cost calculation was also an approximation. First resetting the bit counter and then encoding zero cbf.
                // Now encoding the zero cbf without writing into bitstream, keeping m_fracBits unchanged. The same is valid for chroma.
                uint64_t nullCostY = estimateNullCbfCost(distY, psyEnergyY, tuDepth, TEXT_LUMA);

                if (nullCostY < singleCostY)
                {
                    cbfFlag[TEXT_LUMA][0] = 0;
                    singleBits[TEXT_LUMA][0] = 0;
                    primitives.cu[partSize].blockfill_s(curResiY, strideResiY, 0);
#if CHECKED_BUILD || _DEBUG
                    uint32_t numCoeffY = 1 << (log2TrSize << 1);
                    memset(coeffCurY, 0, sizeof(coeff_t) * numCoeffY);
#endif
                    if (checkTransformSkipY)
                        minCost[TEXT_LUMA][0] = nullCostY;
                    singleDist[TEXT_LUMA][0] = distY;
                    singlePsyEnergy[TEXT_LUMA][0] = psyEnergyY;
                }
                else
                {
                    if (checkTransformSkipY)
                        minCost[TEXT_LUMA][0] = singleCostY;
                    singleDist[TEXT_LUMA][0] = nonZeroDistY;
                    singlePsyEnergy[TEXT_LUMA][0] = nonZeroPsyEnergyY;
                }
            }
        }
        else
        {
            if (checkTransformSkipY)
                minCost[TEXT_LUMA][0] = estimateNullCbfCost(distY, psyEnergyY, tuDepth, TEXT_LUMA);
            primitives.cu[partSize].blockfill_s(curResiY, strideResiY, 0);
            singleDist[TEXT_LUMA][0] = distY;
            singlePsyEnergy[TEXT_LUMA][0] = psyEnergyY;
        }

        cu.setCbfSubParts(cbfFlag[TEXT_LUMA][0] << tuDepth, TEXT_LUMA, absPartIdx, depth);

        if (bCodeChroma)
        {
            uint32_t coeffOffsetC = coeffOffsetY >> (m_hChromaShift + m_vChromaShift);
            uint32_t strideResiC  = m_rqt[qtLayer].resiQtYuv.m_csize;
            for (uint32_t chromaId = TEXT_CHROMA_U; chromaId <= TEXT_CHROMA_V; chromaId++)
            {
                uint32_t distC = 0, psyEnergyC = 0;
                coeff_t* coeffCurC = m_rqt[qtLayer].coeffRQT[chromaId] + coeffOffsetC;
                TURecurse tuIterator(splitIntoSubTUs ? VERTICAL_SPLIT : DONT_SPLIT, absPartIdxStep, absPartIdx);

                do
                {
                    uint32_t absPartIdxC = tuIterator.absPartIdxTURelCU;
                    uint32_t subTUOffset = tuIterator.section << (log2TrSizeC * 2);

                    cu.setTransformSkipPartRange(0, (TextType)chromaId, absPartIdxC, tuIterator.absPartIdxStep);

                    if (m_bEnableRDOQ && (chromaId != TEXT_CHROMA_V))
                        m_entropyCoder.estBit(m_entropyCoder.m_estBitsSbac, log2TrSizeC, false);

                    fenc = fencYuv->getChromaAddr(chromaId, absPartIdxC);
                    resi = resiYuv.getChromaAddr(chromaId, absPartIdxC);
                    numSig[chromaId][tuIterator.section] = m_quant.transformNxN(cu, fenc, fencYuv->m_csize, resi, resiYuv.m_csize, coeffCurC + subTUOffset, log2TrSizeC, (TextType)chromaId, absPartIdxC, false);
                    cbfFlag[chromaId][tuIterator.section] = !!numSig[chromaId][tuIterator.section];

                    if (cbfFlag[chromaId][tuIterator.section])
                        m_entropyCoder.codeCoeffNxN(cu, coeffCurC + subTUOffset, absPartIdxC, log2TrSizeC, (TextType)chromaId);
                    uint32_t newBits = m_entropyCoder.getNumberOfWrittenBits();
                    singleBits[chromaId][tuIterator.section] = newBits - singleBitsPrev;
                    singleBitsPrev = newBits;

                    int16_t* curResiC = m_rqt[qtLayer].resiQtYuv.getChromaAddr(chromaId, absPartIdxC);
                    distC = m_rdCost.scaleChromaDist(chromaId, primitives.cu[log2TrSizeC - 2].ssd_s(resiYuv.getChromaAddr(chromaId, absPartIdxC), resiYuv.m_csize));

                    if (cbfFlag[chromaId][tuIterator.section])
                    {
                        m_quant.invtransformNxN(curResiC, strideResiC, coeffCurC + subTUOffset,
                                                log2TrSizeC, (TextType)chromaId, false, false, numSig[chromaId][tuIterator.section]);

                        // non-zero cost calculation for luma, same as luma - This is an approximation
                        // finally we have to encode correct cbf after comparing with null cost
                        uint32_t dist = primitives.cu[partSizeC].sse_ss(resiYuv.getChromaAddr(chromaId, absPartIdxC), resiYuv.m_csize, curResiC, strideResiC);
                        uint32_t nzCbfBitsC = m_entropyCoder.estimateCbfBits(cbfFlag[chromaId][tuIterator.section], (TextType)chromaId, tuDepth);
                        uint32_t nonZeroDistC = m_rdCost.scaleChromaDist(chromaId, dist);
                        uint32_t nonZeroPsyEnergyC = 0; uint64_t singleCostC = 0;
                        if (m_rdCost.m_psyRd)
                        {
                            nonZeroPsyEnergyC = m_rdCost.psyCost(partSizeC, resiYuv.getChromaAddr(chromaId, absPartIdxC), resiYuv.m_csize, curResiC, strideResiC);
                            singleCostC = m_rdCost.calcPsyRdCost(nonZeroDistC, nzCbfBitsC + singleBits[chromaId][tuIterator.section], nonZeroPsyEnergyC);
                        }
                        else
                            singleCostC = m_rdCost.calcRdCost(nonZeroDistC, nzCbfBitsC + singleBits[chromaId][tuIterator.section]);

                        if (cu.m_tqBypass[0])
                        {
                            singleDist[chromaId][tuIterator.section] = nonZeroDistC;
                            singlePsyEnergy[chromaId][tuIterator.section] = nonZeroPsyEnergyC;
                        }
                        else
                        {
                            //zero-cost calculation for chroma. This is an approximation
                            uint64_t nullCostC = estimateNullCbfCost(distC, psyEnergyC, tuDepth, (TextType)chromaId);

                            if (nullCostC < singleCostC)
                            {
                                cbfFlag[chromaId][tuIterator.section] = 0;
                                singleBits[chromaId][tuIterator.section] = 0;
                                primitives.cu[partSizeC].blockfill_s(curResiC, strideResiC, 0);
#if CHECKED_BUILD || _DEBUG
                                uint32_t numCoeffC = 1 << (log2TrSizeC << 1);
                                memset(coeffCurC + subTUOffset, 0, sizeof(coeff_t) * numCoeffC);
#endif
                                if (checkTransformSkipC)
                                    minCost[chromaId][tuIterator.section] = nullCostC;
                                singleDist[chromaId][tuIterator.section] = distC;
                                singlePsyEnergy[chromaId][tuIterator.section] = psyEnergyC;
                            }
                            else
                            {
                                if (checkTransformSkipC)
                                    minCost[chromaId][tuIterator.section] = singleCostC;
                                singleDist[chromaId][tuIterator.section] = nonZeroDistC;
                                singlePsyEnergy[chromaId][tuIterator.section] = nonZeroPsyEnergyC;
                            }
                        }
                    }
                    else
                    {
                        if (checkTransformSkipC)
                            minCost[chromaId][tuIterator.section] = estimateNullCbfCost(distC, psyEnergyC, tuDepthC, (TextType)chromaId);
                        primitives.cu[partSizeC].blockfill_s(curResiC, strideResiC, 0);
                        singleDist[chromaId][tuIterator.section] = distC;
                        singlePsyEnergy[chromaId][tuIterator.section] = psyEnergyC;
                    }

                    cu.setCbfPartRange(cbfFlag[chromaId][tuIterator.section] << tuDepth, (TextType)chromaId, absPartIdxC, tuIterator.absPartIdxStep);
                }
                while (tuIterator.isNextSection());
            }
        }

        if (checkTransformSkipY)
        {
            uint32_t nonZeroDistY = 0;
            uint32_t nonZeroPsyEnergyY = 0;
            uint64_t singleCostY = MAX_INT64;

            m_entropyCoder.load(m_rqt[depth].rqtRoot);

            cu.setTransformSkipSubParts(1, TEXT_LUMA, absPartIdx, depth);

            if (m_bEnableRDOQ)
                m_entropyCoder.estBit(m_entropyCoder.m_estBitsSbac, log2TrSize, true);

            fenc = fencYuv->getLumaAddr(absPartIdx);
            resi = resiYuv.getLumaAddr(absPartIdx);
            uint32_t numSigTSkipY = m_quant.transformNxN(cu, fenc, fencYuv->m_size, resi, resiYuv.m_size, m_tsCoeff, log2TrSize, TEXT_LUMA, absPartIdx, true);

            if (numSigTSkipY)
            {
                m_entropyCoder.resetBits();
                m_entropyCoder.codeQtCbfLuma(!!numSigTSkipY, tuDepth);
                m_entropyCoder.codeCoeffNxN(cu, m_tsCoeff, absPartIdx, log2TrSize, TEXT_LUMA);
                const uint32_t skipSingleBitsY = m_entropyCoder.getNumberOfWrittenBits();

                m_quant.invtransformNxN(m_tsResidual, trSize, m_tsCoeff, log2TrSize, TEXT_LUMA, false, true, numSigTSkipY);

                nonZeroDistY = primitives.cu[partSize].sse_ss(resiYuv.getLumaAddr(absPartIdx), resiYuv.m_size, m_tsResidual, trSize);

                if (m_rdCost.m_psyRd)
                {
                    nonZeroPsyEnergyY = m_rdCost.psyCost(partSize, resiYuv.getLumaAddr(absPartIdx), resiYuv.m_size, m_tsResidual, trSize);
                    singleCostY = m_rdCost.calcPsyRdCost(nonZeroDistY, skipSingleBitsY, nonZeroPsyEnergyY);
                }
                else
                    singleCostY = m_rdCost.calcRdCost(nonZeroDistY, skipSingleBitsY);
            }

            if (!numSigTSkipY || minCost[TEXT_LUMA][0] < singleCostY)
                cu.setTransformSkipSubParts(0, TEXT_LUMA, absPartIdx, depth);
            else
            {
                singleDist[TEXT_LUMA][0] = nonZeroDistY;
                singlePsyEnergy[TEXT_LUMA][0] = nonZeroPsyEnergyY;
                cbfFlag[TEXT_LUMA][0] = !!numSigTSkipY;
                bestTransformMode[TEXT_LUMA][0] = 1;
                uint32_t numCoeffY = 1 << (log2TrSize << 1);
                memcpy(coeffCurY, m_tsCoeff, sizeof(coeff_t) * numCoeffY);
                primitives.cu[partSize].copy_ss(curResiY, strideResiY, m_tsResidual, trSize);
            }

            cu.setCbfSubParts(cbfFlag[TEXT_LUMA][0] << tuDepth, TEXT_LUMA, absPartIdx, depth);
        }

        if (bCodeChroma && checkTransformSkipC)
        {
            uint32_t nonZeroDistC = 0, nonZeroPsyEnergyC = 0;
            uint64_t singleCostC = MAX_INT64;
            uint32_t strideResiC = m_rqt[qtLayer].resiQtYuv.m_csize;
            uint32_t coeffOffsetC = coeffOffsetY >> (m_hChromaShift + m_vChromaShift);

            m_entropyCoder.load(m_rqt[depth].rqtRoot);

            for (uint32_t chromaId = TEXT_CHROMA_U; chromaId <= TEXT_CHROMA_V; chromaId++)
            {
                coeff_t* coeffCurC = m_rqt[qtLayer].coeffRQT[chromaId] + coeffOffsetC;
                TURecurse tuIterator(splitIntoSubTUs ? VERTICAL_SPLIT : DONT_SPLIT, absPartIdxStep, absPartIdx);

                do
                {
                    uint32_t absPartIdxC = tuIterator.absPartIdxTURelCU;
                    uint32_t subTUOffset = tuIterator.section << (log2TrSizeC * 2);

                    int16_t* curResiC = m_rqt[qtLayer].resiQtYuv.getChromaAddr(chromaId, absPartIdxC);

                    cu.setTransformSkipPartRange(1, (TextType)chromaId, absPartIdxC, tuIterator.absPartIdxStep);

                    if (m_bEnableRDOQ && (chromaId != TEXT_CHROMA_V))
                        m_entropyCoder.estBit(m_entropyCoder.m_estBitsSbac, log2TrSizeC, false);

                    fenc = fencYuv->getChromaAddr(chromaId, absPartIdxC);
                    resi = resiYuv.getChromaAddr(chromaId, absPartIdxC);
                    uint32_t numSigTSkipC = m_quant.transformNxN(cu, fenc, fencYuv->m_csize, resi, resiYuv.m_csize, m_tsCoeff, log2TrSizeC, (TextType)chromaId, absPartIdxC, true);

                    m_entropyCoder.resetBits();
                    singleBits[chromaId][tuIterator.section] = 0;

                    if (numSigTSkipC)
                    {
                        m_entropyCoder.codeQtCbfChroma(!!numSigTSkipC, tuDepth);
                        m_entropyCoder.codeCoeffNxN(cu, m_tsCoeff, absPartIdxC, log2TrSizeC, (TextType)chromaId);
                        singleBits[chromaId][tuIterator.section] = m_entropyCoder.getNumberOfWrittenBits();

                        m_quant.invtransformNxN(m_tsResidual, trSizeC, m_tsCoeff,
                                                log2TrSizeC, (TextType)chromaId, false, true, numSigTSkipC);
                        uint32_t dist = primitives.cu[partSizeC].sse_ss(resiYuv.getChromaAddr(chromaId, absPartIdxC), resiYuv.m_csize, m_tsResidual, trSizeC);
                        nonZeroDistC = m_rdCost.scaleChromaDist(chromaId, dist);
                        if (m_rdCost.m_psyRd)
                        {
                            nonZeroPsyEnergyC = m_rdCost.psyCost(partSizeC, resiYuv.getChromaAddr(chromaId, absPartIdxC), resiYuv.m_csize, m_tsResidual, trSizeC);
                            singleCostC = m_rdCost.calcPsyRdCost(nonZeroDistC, singleBits[chromaId][tuIterator.section], nonZeroPsyEnergyC);
                        }
                        else
                            singleCostC = m_rdCost.calcRdCost(nonZeroDistC, singleBits[chromaId][tuIterator.section]);
                    }

                    if (!numSigTSkipC || minCost[chromaId][tuIterator.section] < singleCostC)
                        cu.setTransformSkipPartRange(0, (TextType)chromaId, absPartIdxC, tuIterator.absPartIdxStep);
                    else
                    {
                        singleDist[chromaId][tuIterator.section] = nonZeroDistC;
                        singlePsyEnergy[chromaId][tuIterator.section] = nonZeroPsyEnergyC;
                        cbfFlag[chromaId][tuIterator.section] = !!numSigTSkipC;
                        bestTransformMode[chromaId][tuIterator.section] = 1;
                        uint32_t numCoeffC = 1 << (log2TrSizeC << 1);
                        memcpy(coeffCurC + subTUOffset, m_tsCoeff, sizeof(coeff_t) * numCoeffC);
                        primitives.cu[partSizeC].copy_ss(curResiC, strideResiC, m_tsResidual, trSizeC);
                    }

                    cu.setCbfPartRange(cbfFlag[chromaId][tuIterator.section] << tuDepth, (TextType)chromaId, absPartIdxC, tuIterator.absPartIdxStep);
                }
                while (tuIterator.isNextSection());
            }
        }

        // Here we were encoding cbfs and coefficients, after calculating distortion above.
        // Now I am encoding only cbfs, since I have encoded coefficients above. I have just collected
        // bits required for coefficients and added with number of cbf bits. As I tested the order does not
        // make any difference. But bit confused whether I should load the original context as below.
        m_entropyCoder.load(m_rqt[depth].rqtRoot);
        m_entropyCoder.resetBits();

        //Encode cbf flags
        if (bCodeChroma)
        {
            if (!splitIntoSubTUs)
            {
                m_entropyCoder.codeQtCbfChroma(cbfFlag[TEXT_CHROMA_U][0], tuDepth);
                m_entropyCoder.codeQtCbfChroma(cbfFlag[TEXT_CHROMA_V][0], tuDepth);
            }
            else
            {
                offsetSubTUCBFs(cu, TEXT_CHROMA_U, tuDepth, absPartIdx);
                offsetSubTUCBFs(cu, TEXT_CHROMA_V, tuDepth, absPartIdx);
                m_entropyCoder.codeQtCbfChroma(cbfFlag[TEXT_CHROMA_U][0], tuDepth);
                m_entropyCoder.codeQtCbfChroma(cbfFlag[TEXT_CHROMA_U][1], tuDepth);
                m_entropyCoder.codeQtCbfChroma(cbfFlag[TEXT_CHROMA_V][0], tuDepth);
                m_entropyCoder.codeQtCbfChroma(cbfFlag[TEXT_CHROMA_V][1], tuDepth);
            }
        }

        m_entropyCoder.codeQtCbfLuma(cbfFlag[TEXT_LUMA][0], tuDepth);

        uint32_t cbfBits = m_entropyCoder.getNumberOfWrittenBits();

        uint32_t coeffBits = 0;
        coeffBits = singleBits[TEXT_LUMA][0];
        for (uint32_t subTUIndex = 0; subTUIndex < 2; subTUIndex++)
        {
            coeffBits += singleBits[TEXT_CHROMA_U][subTUIndex];
            coeffBits += singleBits[TEXT_CHROMA_V][subTUIndex];
        }

        // In split mode, we need only coeffBits. The reason is encoding chroma cbfs is different from luma.
        // In case of chroma, if any one of the split block's cbf is 1, then we need to encode cbf 1, and then for
        // four split block's individual cbf value. This is not known before analysis of four split blocks.
        // For that reason, I am collecting individual coefficient bits only.
        fullCost.bits = bSplitPresentFlag ? cbfBits + coeffBits : coeffBits;

        fullCost.distortion += singleDist[TEXT_LUMA][0];
        fullCost.energy += singlePsyEnergy[TEXT_LUMA][0];// need to check we need to add chroma also
        for (uint32_t subTUIndex = 0; subTUIndex < 2; subTUIndex++)
        {
            fullCost.distortion += singleDist[TEXT_CHROMA_U][subTUIndex];
            fullCost.distortion += singleDist[TEXT_CHROMA_V][subTUIndex];
        }

        if (m_rdCost.m_psyRd)
            fullCost.rdcost = m_rdCost.calcPsyRdCost(fullCost.distortion, fullCost.bits, fullCost.energy);
        else
            fullCost.rdcost = m_rdCost.calcRdCost(fullCost.distortion, fullCost.bits);
    }

    // code sub-blocks
    if (bCheckSplit)
    {
        if (bCheckFull)
        {
            m_entropyCoder.store(m_rqt[depth].rqtTest);
            m_entropyCoder.load(m_rqt[depth].rqtRoot);
        }

        Cost splitCost;
        if (bSplitPresentFlag && (log2TrSize <= depthRange[1] && log2TrSize > depthRange[0]))
        {
            // Subdiv flag can be encoded at the start of analysis of split blocks.
            m_entropyCoder.resetBits();
            m_entropyCoder.codeTransformSubdivFlag(1, 5 - log2TrSize);
            splitCost.bits = m_entropyCoder.getNumberOfWrittenBits();
        }

        uint32_t qNumParts = 1 << (log2TrSize - 1 - LOG2_UNIT_SIZE) * 2;
        uint32_t ycbf = 0, ucbf = 0, vcbf = 0;
        for (uint32_t qIdx = 0, qPartIdx = absPartIdx; qIdx < 4; ++qIdx, qPartIdx += qNumParts)
        {
            estimateResidualQT(mode, cuGeom, qPartIdx, tuDepth + 1, resiYuv, splitCost, depthRange);
            ycbf |= cu.getCbf(qPartIdx, TEXT_LUMA,     tuDepth + 1);
            ucbf |= cu.getCbf(qPartIdx, TEXT_CHROMA_U, tuDepth + 1);
            vcbf |= cu.getCbf(qPartIdx, TEXT_CHROMA_V, tuDepth + 1);
        }
        for (uint32_t i = 0; i < 4 * qNumParts; ++i)
        {
            cu.m_cbf[0][absPartIdx + i] |= ycbf << tuDepth;
            cu.m_cbf[1][absPartIdx + i] |= ucbf << tuDepth;
            cu.m_cbf[2][absPartIdx + i] |= vcbf << tuDepth;
        }

        // Here we were encoding cbfs and coefficients for splitted blocks. Since I have collected coefficient bits
        // for each individual blocks, only encoding cbf values. As I mentioned encoding chroma cbfs is different then luma.
        // But have one doubt that if coefficients are encoded in context at depth 2 (for example) and cbfs are encoded in context
        // at depth 0 (for example).
        m_entropyCoder.load(m_rqt[depth].rqtRoot);
        m_entropyCoder.resetBits();

        codeInterSubdivCbfQT(cu, absPartIdx, tuDepth, depthRange);
        uint32_t splitCbfBits = m_entropyCoder.getNumberOfWrittenBits();
        splitCost.bits += splitCbfBits;

        if (m_rdCost.m_psyRd)
            splitCost.rdcost = m_rdCost.calcPsyRdCost(splitCost.distortion, splitCost.bits, splitCost.energy);
        else
            splitCost.rdcost = m_rdCost.calcRdCost(splitCost.distortion, splitCost.bits);

        if (ycbf || ucbf || vcbf || !bCheckFull)
        {
            if (splitCost.rdcost < fullCost.rdcost)
            {
                outCosts.distortion += splitCost.distortion;
                outCosts.rdcost     += splitCost.rdcost;
                outCosts.bits       += splitCost.bits;
                outCosts.energy     += splitCost.energy;
                return;
            }
            else
                outCosts.energy     += splitCost.energy;
        }

        cu.setTransformSkipSubParts(bestTransformMode[TEXT_LUMA][0], TEXT_LUMA, absPartIdx, depth);
        if (bCodeChroma)
        {
            if (!splitIntoSubTUs)
            {
                cu.setTransformSkipSubParts(bestTransformMode[TEXT_CHROMA_U][0], TEXT_CHROMA_U, absPartIdx, depth);
                cu.setTransformSkipSubParts(bestTransformMode[TEXT_CHROMA_V][0], TEXT_CHROMA_V, absPartIdx, depth);
            }
            else
            {
                uint32_t tuNumParts = absPartIdxStep >> 1;
                cu.setTransformSkipPartRange(bestTransformMode[TEXT_CHROMA_U][0], TEXT_CHROMA_U, absPartIdx             , tuNumParts);
                cu.setTransformSkipPartRange(bestTransformMode[TEXT_CHROMA_U][1], TEXT_CHROMA_U, absPartIdx + tuNumParts, tuNumParts);
                cu.setTransformSkipPartRange(bestTransformMode[TEXT_CHROMA_V][0], TEXT_CHROMA_V, absPartIdx             , tuNumParts);
                cu.setTransformSkipPartRange(bestTransformMode[TEXT_CHROMA_V][1], TEXT_CHROMA_V, absPartIdx + tuNumParts, tuNumParts);
            }
        }
        X265_CHECK(bCheckFull, "check-full must be set\n");
        m_entropyCoder.load(m_rqt[depth].rqtTest);
    }

    cu.setTUDepthSubParts(tuDepth, absPartIdx, depth);
    cu.setCbfSubParts(cbfFlag[TEXT_LUMA][0] << tuDepth, TEXT_LUMA, absPartIdx, depth);

    if (bCodeChroma)
    {
        if (!splitIntoSubTUs)
        {
            cu.setCbfSubParts(cbfFlag[TEXT_CHROMA_U][0] << tuDepth, TEXT_CHROMA_U, absPartIdx, depth);
            cu.setCbfSubParts(cbfFlag[TEXT_CHROMA_V][0] << tuDepth, TEXT_CHROMA_V, absPartIdx, depth);
        }
        else
        {
            uint32_t tuNumParts = absPartIdxStep >> 1;

            offsetCBFs(cbfFlag[TEXT_CHROMA_U]);
            offsetCBFs(cbfFlag[TEXT_CHROMA_V]);
            cu.setCbfPartRange(cbfFlag[TEXT_CHROMA_U][0] << tuDepth, TEXT_CHROMA_U, absPartIdx             , tuNumParts);
            cu.setCbfPartRange(cbfFlag[TEXT_CHROMA_U][1] << tuDepth, TEXT_CHROMA_U, absPartIdx + tuNumParts, tuNumParts);
            cu.setCbfPartRange(cbfFlag[TEXT_CHROMA_V][0] << tuDepth, TEXT_CHROMA_V, absPartIdx             , tuNumParts);
            cu.setCbfPartRange(cbfFlag[TEXT_CHROMA_V][1] << tuDepth, TEXT_CHROMA_V, absPartIdx + tuNumParts, tuNumParts);
        }
    }

    outCosts.distortion += fullCost.distortion;
    outCosts.rdcost     += fullCost.rdcost;
    outCosts.bits       += fullCost.bits;
    outCosts.energy     += fullCost.energy;
}

void Search::codeInterSubdivCbfQT(CUData& cu, uint32_t absPartIdx, const uint32_t tuDepth, const uint32_t depthRange[2])
{
    X265_CHECK(cu.isInter(absPartIdx), "codeInterSubdivCbfQT() with intra block\n");

    const bool bSubdiv  = tuDepth < cu.m_tuDepth[absPartIdx];
    uint32_t log2TrSize = cu.m_log2CUSize[0] - tuDepth;

    if (!(log2TrSize - m_hChromaShift < 2))
    {
        if (!tuDepth || cu.getCbf(absPartIdx, TEXT_CHROMA_U, tuDepth - 1))
            m_entropyCoder.codeQtCbfChroma(cu, absPartIdx, TEXT_CHROMA_U, tuDepth, !bSubdiv);
        if (!tuDepth || cu.getCbf(absPartIdx, TEXT_CHROMA_V, tuDepth - 1))
            m_entropyCoder.codeQtCbfChroma(cu, absPartIdx, TEXT_CHROMA_V, tuDepth, !bSubdiv);
    }
    else
    {
        X265_CHECK(cu.getCbf(absPartIdx, TEXT_CHROMA_U, tuDepth) == cu.getCbf(absPartIdx, TEXT_CHROMA_U, tuDepth - 1), "chroma CBF not matching\n");
        X265_CHECK(cu.getCbf(absPartIdx, TEXT_CHROMA_V, tuDepth) == cu.getCbf(absPartIdx, TEXT_CHROMA_V, tuDepth - 1), "chroma CBF not matching\n");
    }

    if (!bSubdiv)
    {
        m_entropyCoder.codeQtCbfLuma(cu, absPartIdx, tuDepth);
    }
    else
    {
        uint32_t qNumParts = 1 << (log2TrSize -1 - LOG2_UNIT_SIZE) * 2;
        for (uint32_t qIdx = 0; qIdx < 4; ++qIdx, absPartIdx += qNumParts)
            codeInterSubdivCbfQT(cu, absPartIdx, tuDepth + 1, depthRange);
    }
}

void Search::saveResidualQTData(CUData& cu, ShortYuv& resiYuv, uint32_t absPartIdx, uint32_t tuDepth)
{
    const uint32_t log2TrSize = cu.m_log2CUSize[0] - tuDepth;

    if (tuDepth < cu.m_tuDepth[absPartIdx])
    {
        uint32_t qNumParts = 1 << (log2TrSize - 1 - LOG2_UNIT_SIZE) * 2;
        for (uint32_t qIdx = 0; qIdx < 4; ++qIdx, absPartIdx += qNumParts)
            saveResidualQTData(cu, resiYuv, absPartIdx, tuDepth + 1);
        return;
    }

    const uint32_t qtLayer = log2TrSize - 2;

    uint32_t log2TrSizeC = log2TrSize - m_hChromaShift;
    bool bCodeChroma = true;
    uint32_t tuDepthC = tuDepth;
    if (log2TrSizeC < 2)
    {
        X265_CHECK(log2TrSize == 2 && m_csp != X265_CSP_I444 && tuDepth, "invalid tuDepth\n");
        log2TrSizeC = 2;
        tuDepthC--;
        bCodeChroma = !(absPartIdx & 3);
    }

    m_rqt[qtLayer].resiQtYuv.copyPartToPartLuma(resiYuv, absPartIdx, log2TrSize);

    uint32_t numCoeffY = 1 << (log2TrSize * 2);
    uint32_t coeffOffsetY = absPartIdx << LOG2_UNIT_SIZE * 2;
    coeff_t* coeffSrcY = m_rqt[qtLayer].coeffRQT[0] + coeffOffsetY;
    coeff_t* coeffDstY = cu.m_trCoeff[0] + coeffOffsetY;
    memcpy(coeffDstY, coeffSrcY, sizeof(coeff_t) * numCoeffY);

    if (bCodeChroma)
    {
        m_rqt[qtLayer].resiQtYuv.copyPartToPartChroma(resiYuv, absPartIdx, log2TrSizeC + m_hChromaShift);

        uint32_t numCoeffC = 1 << (log2TrSizeC * 2 + (m_csp == X265_CSP_I422));
        uint32_t coeffOffsetC = coeffOffsetY >> (m_hChromaShift + m_vChromaShift);

        coeff_t* coeffSrcU = m_rqt[qtLayer].coeffRQT[1] + coeffOffsetC;
        coeff_t* coeffSrcV = m_rqt[qtLayer].coeffRQT[2] + coeffOffsetC;
        coeff_t* coeffDstU = cu.m_trCoeff[1] + coeffOffsetC;
        coeff_t* coeffDstV = cu.m_trCoeff[2] + coeffOffsetC;
        memcpy(coeffDstU, coeffSrcU, sizeof(coeff_t) * numCoeffC);
        memcpy(coeffDstV, coeffSrcV, sizeof(coeff_t) * numCoeffC);
    }
}

/* returns the number of bits required to signal a non-most-probable mode.
 * on return mpms contains bitmap of most probable modes */
uint32_t Search::getIntraRemModeBits(CUData& cu, uint32_t absPartIdx, uint32_t mpmModes[3], uint64_t& mpms) const
{
    cu.getIntraDirLumaPredictor(absPartIdx, mpmModes);

    mpms = 0;
    for (int i = 0; i < 3; ++i)
        mpms |= ((uint64_t)1 << mpmModes[i]);

    return m_entropyCoder.bitsIntraModeNonMPM();
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

void Search::checkDQP(CUData& cu, const CUGeom& cuGeom)
{
    if (cu.m_slice->m_pps->bUseDQP && cuGeom.depth <= cu.m_slice->m_pps->maxCuDQPDepth)
    {
        if (cu.getQtRootCbf(0))
        {
            /* When analysing RDO with DQP bits, the entropy encoder should add the cost of DQP bits here
             * i.e Encode QP */
        }
        else
            cu.setQPSubParts(cu.getRefQP(0), 0, cuGeom.depth);
    }
}

void Search::checkDQPForSplitPred(CUData& cu, const CUGeom& cuGeom)
{
    if ((cuGeom.depth == cu.m_slice->m_pps->maxCuDQPDepth) && cu.m_slice->m_pps->bUseDQP)
    {
        bool hasResidual = false;

        /* Check if any sub-CU has a non-zero QP */
        for (uint32_t blkIdx = 0; blkIdx < cuGeom.numPartitions; blkIdx++)
        {
            if (cu.getQtRootCbf(blkIdx))
            {
                hasResidual = true;
                break;
            }
        }
        if (hasResidual)
            /* TODO: Encode QP, and recalculate RD cost of splitPred */
            /* For all zero CBF sub-CUs, reset QP to RefQP (so that deltaQP is not signalled).
            When the non-zero CBF sub-CU is found, stop */
            cu.setQPSubCUs(cu.getRefQP(0), 0, cuGeom.depth);
        else
            /* No residual within this CU or subCU, so reset QP to RefQP */
            cu.setQPSubParts(cu.getRefQP(0), 0, cuGeom.depth);
    }
}
