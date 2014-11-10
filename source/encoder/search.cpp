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

using namespace x265;

#if _MSC_VER
#pragma warning(disable: 4800) // 'uint8_t' : forcing value to bool 'true' or 'false' (performance warning)
#pragma warning(disable: 4244) // '=' : conversion from 'int' to 'uint8_t', possible loss of data)
#endif

#define MVP_IDX_BITS 1

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

bool Search::initSearch(const x265_param& param, ScalingList& scalingList)
{
    m_param = &param;
    m_bEnableRDOQ = param.rdLevel >= 4;
    m_bFrameParallel = param.frameNumThreads > 1;
    m_numLayers = g_log2Size[param.maxCUSize] - 2;

    m_rdCost.setPsyRdScale(param.psyRd);
    m_me.searchMethod = param.searchMethod;
    m_me.subpelRefine = param.subpelRefine;

    bool ok = m_quant.init(m_bEnableRDOQ, param.psyRdoq, scalingList, m_entropyCoder);
    if (m_param->noiseReduction)
        ok &= m_quant.allocNoiseReduction(param);

    ok &= Predict::allocBuffers(param.internalCsp); /* sets m_hChromaShift & m_vChromaShift */

    /* When frame parallelism is active, only 'refLagPixels' of reference frames will be guaranteed
     * available for motion reference.  See refLagRows in FrameEncoder::compressCTURows() */
    m_refLagPixels = m_bFrameParallel ? param.searchRange : param.sourceHeight;

    uint32_t sizeL = 1 << (g_maxLog2CUSize * 2);
    uint32_t sizeC = sizeL >> (m_hChromaShift + m_vChromaShift);
    uint32_t numPartitions = NUM_CU_PARTITIONS;

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

void Search::codeSubdivCbfQTChroma(const CUData& cu, uint32_t trDepth, uint32_t absPartIdx, uint32_t absPartIdxStep, uint32_t width, uint32_t height)
{
    uint32_t fullDepth  = cu.m_cuDepth[0] + trDepth;
    uint32_t tuDepthL   = cu.m_tuDepth[absPartIdx];
    uint32_t subdiv     = tuDepthL > trDepth;
    uint32_t log2TrSize = g_maxLog2CUSize - fullDepth;

    bool mCodeAll = true;
    const uint32_t numPels = 1 << (log2TrSize * 2 - m_hChromaShift - m_vChromaShift);
    if (numPels < (MIN_TU_SIZE * MIN_TU_SIZE))
        mCodeAll = false;

    if (mCodeAll)
    {
        if (!trDepth || cu.getCbf(absPartIdx, TEXT_CHROMA_U, trDepth - 1))
            m_entropyCoder.codeQtCbf(cu, absPartIdx, absPartIdxStep, (width >> m_hChromaShift), (height >> m_vChromaShift), TEXT_CHROMA_U, trDepth, !subdiv);

        if (!trDepth || cu.getCbf(absPartIdx, TEXT_CHROMA_V, trDepth - 1))
            m_entropyCoder.codeQtCbf(cu, absPartIdx, absPartIdxStep, (width >> m_hChromaShift), (height >> m_vChromaShift), TEXT_CHROMA_V, trDepth, !subdiv);
    }

    if (subdiv)
    {
        absPartIdxStep >>= 2;
        width  >>= 1;
        height >>= 1;

        uint32_t qtPartNum = NUM_CU_PARTITIONS >> ((fullDepth + 1) << 1);
        for (uint32_t part = 0; part < 4; part++)
            codeSubdivCbfQTChroma(cu, trDepth + 1, absPartIdx + part * qtPartNum, absPartIdxStep, width, height);
    }
}

void Search::codeCoeffQTChroma(const CUData& cu, uint32_t trDepth, uint32_t absPartIdx, TextType ttype)
{
    if (!cu.getCbf(absPartIdx, ttype, trDepth))
        return;

    uint32_t fullDepth = cu.m_cuDepth[0] + trDepth;
    uint32_t tuDepthL  = cu.m_tuDepth[absPartIdx];

    if (tuDepthL > trDepth)
    {
        uint32_t qtPartNum = NUM_CU_PARTITIONS >> ((fullDepth + 1) << 1);
        for (uint32_t part = 0; part < 4; part++)
            codeCoeffQTChroma(cu, trDepth + 1, absPartIdx + part * qtPartNum, ttype);

        return;
    }

    uint32_t log2TrSize = g_maxLog2CUSize - fullDepth;

    uint32_t trDepthC = trDepth;
    uint32_t log2TrSizeC = log2TrSize - m_hChromaShift;
    
    if (log2TrSizeC == 1)
    {
        X265_CHECK(log2TrSize == 2 && m_csp != X265_CSP_I444 && trDepth, "transform size too small\n");
        trDepthC--;
        log2TrSizeC++;
        uint32_t qpdiv = NUM_CU_PARTITIONS >> ((cu.m_cuDepth[0] + trDepthC) << 1);
        bool bFirstQ = ((absPartIdx & (qpdiv - 1)) == 0);
        if (!bFirstQ)
            return;
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
        uint32_t partIdxesPerSubTU  = NUM_CU_PARTITIONS >> (((cu.m_cuDepth[absPartIdx] + trDepthC) << 1) + 1);
        if (cu.getCbf(absPartIdx, ttype, trDepth + 1))
            m_entropyCoder.codeCoeffNxN(cu, coeff, absPartIdx, log2TrSizeC, ttype);
        if (cu.getCbf(absPartIdx + partIdxesPerSubTU, ttype, trDepth + 1))
            m_entropyCoder.codeCoeffNxN(cu, coeff + subTUSize, absPartIdx + partIdxesPerSubTU, log2TrSizeC, ttype);
    }
}

void Search::codeIntraLumaQT(Mode& mode, const CUGeom& cuGeom, uint32_t trDepth, uint32_t absPartIdx, bool bAllowSplit, Cost& outCost, uint32_t depthRange[2])
{
    uint32_t fullDepth  = mode.cu.m_cuDepth[0] + trDepth;
    uint32_t log2TrSize = g_maxLog2CUSize - fullDepth;
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

    CUData& cu = mode.cu;

    Cost fullCost;
    uint32_t bCBF = 0;

    pixel*   reconQt = m_rqt[qtLayer].reconQtYuv.getLumaAddr(absPartIdx);
    uint32_t reconQtStride = m_rqt[qtLayer].reconQtYuv.m_size;

    if (mightNotSplit)
    {
        if (mightSplit)
            m_entropyCoder.store(m_rqt[fullDepth].rqtRoot);

        pixel*   fenc     = const_cast<pixel*>(mode.fencYuv->getLumaAddr(absPartIdx));
        pixel*   pred     = mode.predYuv.getLumaAddr(absPartIdx);
        int16_t* residual = m_rqt[cuGeom.depth].tmpResiYuv.getLumaAddr(absPartIdx);
        uint32_t stride   = mode.fencYuv->m_size;

        // init availability pattern
        uint32_t lumaPredMode = cu.m_lumaIntraDir[absPartIdx];
        initAdiPattern(cu, cuGeom, absPartIdx, trDepth, lumaPredMode);

        // get prediction signal
        predIntraLumaAng(lumaPredMode, pred, stride, log2TrSize);

        cu.setTransformSkipSubParts(0, TEXT_LUMA, absPartIdx, fullDepth);
        cu.setTUDepthSubParts(trDepth, absPartIdx, fullDepth);

        uint32_t coeffOffsetY = absPartIdx << (LOG2_UNIT_SIZE * 2);
        coeff_t* coeffY       = m_rqt[qtLayer].coeffRQT[0] + coeffOffsetY;

        // store original entropy coding status
        if (m_bEnableRDOQ)
            m_entropyCoder.estBit(m_entropyCoder.m_estBitsSbac, log2TrSize, true);

        primitives.calcresidual[sizeIdx](fenc, pred, residual, stride);

        uint32_t numSig = m_quant.transformNxN(cu, fenc, stride, residual, stride, coeffY, log2TrSize, TEXT_LUMA, absPartIdx, false);
        if (numSig)
        {
            m_quant.invtransformNxN(cu.m_tqBypass[0], residual, stride, coeffY, log2TrSize, TEXT_LUMA, true, false, numSig);
            primitives.luma_add_ps[sizeIdx](reconQt, reconQtStride, pred, residual, stride, stride);
        }
        else
            // no coded residual, recon = pred
            primitives.square_copy_pp[sizeIdx](reconQt, reconQtStride, pred, stride);

        bCBF = !!numSig << trDepth;
        cu.setCbfSubParts(bCBF, TEXT_LUMA, absPartIdx, fullDepth);
        fullCost.distortion = primitives.sse_pp[sizeIdx](reconQt, reconQtStride, fenc, stride);

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

            m_entropyCoder.codePartSize(cu, 0, cu.m_cuDepth[0]);
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
        if (log2TrSize != depthRange[0])
            m_entropyCoder.codeTransformSubdivFlag(0, 5 - log2TrSize);

        m_entropyCoder.codeQtCbf(cu, absPartIdx, TEXT_LUMA, cu.m_tuDepth[absPartIdx]);

        if (cu.getCbf(absPartIdx, TEXT_LUMA, trDepth))
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

        // code split block
        uint32_t qPartsDiv = NUM_CU_PARTITIONS >> ((fullDepth + 1) << 1);
        uint32_t absPartIdxSub = absPartIdx;

        int checkTransformSkip = m_slice->m_pps->bTransformSkipEnabled && (log2TrSize - 1) <= MAX_LOG2_TS_SIZE && !cu.m_tqBypass[0];
        if (m_param->bEnableTSkipFast)
            checkTransformSkip &= cu.m_partSize[0] != SIZE_2Nx2N;

        Cost splitCost;
        uint32_t cbf = 0;
        for (uint32_t subPartIdx = 0; subPartIdx < 4; subPartIdx++, absPartIdxSub += qPartsDiv)
        {
            if (checkTransformSkip)
                codeIntraLumaTSkip(mode, cuGeom, trDepth + 1, absPartIdxSub, splitCost);
            else
                codeIntraLumaQT(mode, cuGeom, trDepth + 1, absPartIdxSub, bAllowSplit, splitCost, depthRange);

            cbf |= cu.getCbf(absPartIdxSub, TEXT_LUMA, trDepth + 1);
        }
        for (uint32_t offs = 0; offs < 4 * qPartsDiv; offs++)
            cu.m_cbf[0][absPartIdx + offs] |= (cbf << trDepth);

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
            cu.setTUDepthSubParts(trDepth, absPartIdx, fullDepth);
            cu.setCbfSubParts(bCBF, TEXT_LUMA, absPartIdx, fullDepth);
            cu.setTransformSkipSubParts(0, TEXT_LUMA, absPartIdx, fullDepth);
        }
    }

    // set reconstruction for next intra prediction blocks if full TU prediction won
    pixel*   picReconY = m_frame->m_reconPic->getLumaAddr(cu.m_cuAddr, cuGeom.encodeIdx + absPartIdx);
    intptr_t picStride = m_frame->m_reconPic->m_stride;
    primitives.square_copy_pp[sizeIdx](picReconY, picStride, reconQt, reconQtStride);

    outCost.rdcost     += fullCost.rdcost;
    outCost.distortion += fullCost.distortion;
    outCost.bits       += fullCost.bits;
    outCost.energy     += fullCost.energy;
}

void Search::codeIntraLumaTSkip(Mode& mode, const CUGeom& cuGeom, uint32_t trDepth, uint32_t absPartIdx, Cost& outCost)
{
    uint32_t fullDepth = mode.cu.m_cuDepth[0] + trDepth;
    uint32_t log2TrSize = g_maxLog2CUSize - fullDepth;
    uint32_t tuSize = 1 << log2TrSize;

    X265_CHECK(tuSize == MAX_TS_SIZE, "transform skip is only possible at 4x4 TUs\n");

    CUData& cu = mode.cu;
    Yuv* predYuv = &mode.predYuv;
    const Yuv* fencYuv = mode.fencYuv;

    Cost fullCost;
    fullCost.rdcost = MAX_INT64;
    int      bTSkip = 0;
    uint32_t bCBF = 0;

    pixel*   fenc = const_cast<pixel*>(fencYuv->getLumaAddr(absPartIdx));
    pixel*   pred = predYuv->getLumaAddr(absPartIdx);
    int16_t* residual = m_rqt[cuGeom.depth].tmpResiYuv.getLumaAddr(absPartIdx);
    uint32_t stride = fencYuv->m_size;
    int      sizeIdx = log2TrSize - 2;

    // init availability pattern
    uint32_t lumaPredMode = cu.m_lumaIntraDir[absPartIdx];
    initAdiPattern(cu, cuGeom, absPartIdx, trDepth, lumaPredMode);

    // get prediction signal
    predIntraLumaAng(lumaPredMode, pred, stride, log2TrSize);

    cu.setTUDepthSubParts(trDepth, absPartIdx, fullDepth);

    uint32_t qtLayer = log2TrSize - 2;
    uint32_t coeffOffsetY = absPartIdx << (LOG2_UNIT_SIZE * 2);
    coeff_t* coeffY = m_rqt[qtLayer].coeffRQT[0] + coeffOffsetY;
    pixel*   reconQt = m_rqt[qtLayer].reconQtYuv.getLumaAddr(absPartIdx);
    uint32_t reconQtStride = m_rqt[qtLayer].reconQtYuv.m_size;

    // store original entropy coding status
    m_entropyCoder.store(m_rqt[fullDepth].rqtRoot);

    if (m_bEnableRDOQ)
        m_entropyCoder.estBit(m_entropyCoder.m_estBitsSbac, log2TrSize, true);

    ALIGN_VAR_32(coeff_t, tsCoeffY[MAX_TS_SIZE * MAX_TS_SIZE]);
    ALIGN_VAR_32(pixel,   tsReconY[MAX_TS_SIZE * MAX_TS_SIZE]);

    int checkTransformSkip = 1;
    for (int useTSkip = 0; useTSkip <= checkTransformSkip; useTSkip++)
    {
        uint64_t tmpCost;
        uint32_t tmpEnergy = 0;

        coeff_t* coeff = (useTSkip ? tsCoeffY : coeffY);
        pixel*   tmpRecon = (useTSkip ? tsReconY : reconQt);
        uint32_t tmpReconStride = (useTSkip ? MAX_TS_SIZE : reconQtStride);

        primitives.calcresidual[sizeIdx](fenc, pred, residual, stride);

        uint32_t numSig = m_quant.transformNxN(cu, fenc, stride, residual, stride, coeff, log2TrSize, TEXT_LUMA, absPartIdx, useTSkip);
        if (numSig)
        {
            m_quant.invtransformNxN(cu.m_tqBypass[0], residual, stride, coeff, log2TrSize, TEXT_LUMA, true, useTSkip, numSig);
            primitives.luma_add_ps[sizeIdx](tmpRecon, tmpReconStride, pred, residual, stride, stride);
        }
        else if (useTSkip)
        {
            /* do not allow tskip if CBF=0, pretend we did not try tskip */
            checkTransformSkip = 0;
            break;
        }
        else
            // no residual coded, recon = pred
            primitives.square_copy_pp[sizeIdx](tmpRecon, tmpReconStride, pred, stride);

        uint32_t tmpDist = primitives.sse_pp[sizeIdx](tmpRecon, tmpReconStride, fenc, stride);

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

            m_entropyCoder.codePartSize(cu, 0, cu.m_cuDepth[0]);
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
        m_entropyCoder.codeTransformSubdivFlag(0, 5 - log2TrSize);

        m_entropyCoder.codeQtCbf(cu, absPartIdx, TEXT_LUMA, cu.m_tuDepth[absPartIdx]);

        if (cu.getCbf(absPartIdx, TEXT_LUMA, trDepth))
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
        memcpy(coeffY, tsCoeffY, sizeof(coeff_t) << (log2TrSize * 2));
        primitives.square_copy_pp[sizeIdx](reconQt, reconQtStride, tsReconY, tuSize);
    }
    else if (checkTransformSkip)
    {
        cu.setTransformSkipSubParts(0, TEXT_LUMA, absPartIdx, fullDepth);
        cu.setCbfSubParts(bCBF << trDepth, TEXT_LUMA, absPartIdx, fullDepth);
        m_entropyCoder.load(m_rqt[fullDepth].rqtTemp);
    }

    // set reconstruction for next intra prediction blocks
    pixel*   picReconY = m_frame->m_reconPic->getLumaAddr(cu.m_cuAddr, cuGeom.encodeIdx + absPartIdx);
    intptr_t picStride = m_frame->m_reconPic->m_stride;
    primitives.square_copy_pp[sizeIdx](picReconY, picStride, reconQt, reconQtStride);

    outCost.rdcost += fullCost.rdcost;
    outCost.distortion += fullCost.distortion;
    outCost.bits += fullCost.bits;
    outCost.energy += fullCost.energy;
}

/* fast luma intra residual generation. Only perform the minimum number of TU splits required by the CU size */
void Search::residualTransformQuantIntra(Mode& mode, const CUGeom& cuGeom, uint32_t trDepth, uint32_t absPartIdx, uint32_t depthRange[2])
{
    CUData& cu = mode.cu;

    uint32_t fullDepth   = cu.m_cuDepth[0] + trDepth;
    uint32_t log2TrSize  = g_maxLog2CUSize - fullDepth;
    bool     bCheckFull  = log2TrSize <= depthRange[1];

    X265_CHECK(m_slice->m_sliceType != I_SLICE, "residualTransformQuantIntra not intended for I slices\n");

    /* we still respect rdPenalty == 2, we can forbid 32x32 intra TU. rdPenalty = 1 is impossible
     * since we are not measuring RD cost */
    if (m_param->rdPenalty == 2 && log2TrSize == 5 && depthRange[0] <= 4)
        bCheckFull = false;

    if (bCheckFull)
    {
        pixel*   fenc      = const_cast<pixel*>(mode.fencYuv->getLumaAddr(absPartIdx));
        pixel*   pred      = mode.predYuv.getLumaAddr(absPartIdx);
        int16_t* residual  = m_rqt[cuGeom.depth].tmpResiYuv.getLumaAddr(absPartIdx);
        pixel*   picReconY = m_frame->m_reconPic->getLumaAddr(cu.m_cuAddr, cuGeom.encodeIdx + absPartIdx);
        intptr_t picStride = m_frame->m_reconPic->m_stride;
        uint32_t stride    = mode.fencYuv->m_size;
        uint32_t sizeIdx   = log2TrSize - 2;
        uint32_t lumaPredMode = cu.m_lumaIntraDir[absPartIdx];
        uint32_t coeffOffsetY = absPartIdx << (LOG2_UNIT_SIZE * 2);
        coeff_t* coeff        = cu.m_trCoeff[TEXT_LUMA] + coeffOffsetY;

        initAdiPattern(cu, cuGeom, absPartIdx, trDepth, lumaPredMode);
        predIntraLumaAng(lumaPredMode, pred, stride, log2TrSize);

        X265_CHECK(!cu.m_transformSkip[TEXT_LUMA][absPartIdx], "unexpected tskip flag in residualTransformQuantIntra\n");
        cu.setTUDepthSubParts(trDepth, absPartIdx, fullDepth);

        primitives.calcresidual[sizeIdx](fenc, pred, residual, stride);
        uint32_t numSig = m_quant.transformNxN(cu, fenc, stride, residual, stride, coeff, log2TrSize, TEXT_LUMA, absPartIdx, false);
        if (numSig)
        {
            m_quant.invtransformNxN(cu.m_tqBypass[absPartIdx], residual, stride, coeff, log2TrSize, TEXT_LUMA, true, false, numSig);
            primitives.luma_add_ps[sizeIdx](picReconY, picStride, pred, residual, stride, stride);
            cu.setCbfSubParts(1 << trDepth, TEXT_LUMA, absPartIdx, fullDepth);
        }
        else
        {
            primitives.square_copy_pp[sizeIdx](picReconY, picStride, pred, stride);
            cu.setCbfSubParts(0, TEXT_LUMA, absPartIdx, fullDepth);
        }
    }
    else
    {
        X265_CHECK(log2TrSize > depthRange[0], "intra luma split state failure\n");
        
        /* code split block */
        uint32_t qPartsDiv = NUM_CU_PARTITIONS >> ((fullDepth + 1) << 1);
        uint32_t cbf = 0;
        for (uint32_t subPartIdx = 0, absPartIdxSub = absPartIdx; subPartIdx < 4; subPartIdx++, absPartIdxSub += qPartsDiv)
        {
            residualTransformQuantIntra(mode, cuGeom, trDepth + 1, absPartIdxSub, depthRange);
            cbf |= cu.getCbf(absPartIdxSub, TEXT_LUMA, trDepth + 1);
        }
        for (uint32_t offs = 0; offs < 4 * qPartsDiv; offs++)
            cu.m_cbf[TEXT_LUMA][absPartIdx + offs] |= (cbf << trDepth);
    }
}

void Search::extractIntraResultQT(CUData& cu, Yuv& reconYuv, uint32_t trDepth, uint32_t absPartIdx)
{
    uint32_t fullDepth = cu.m_cuDepth[0] + trDepth;
    uint32_t tuDepth   = cu.m_tuDepth[absPartIdx];

    if (tuDepth == trDepth)
    {
        uint32_t log2TrSize = g_maxLog2CUSize - fullDepth;
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
        uint32_t numQPart = NUM_CU_PARTITIONS >> ((fullDepth + 1) << 1);
        for (uint32_t subPartIdx = 0; subPartIdx < 4; subPartIdx++)
            extractIntraResultQT(cu, reconYuv, trDepth + 1, absPartIdx + subPartIdx * numQPart);
    }
}

/* 4:2:2 post-TU split processing */
void Search::offsetSubTUCBFs(CUData& cu, TextType ttype, uint32_t trDepth, uint32_t absPartIdx)
{
    uint32_t depth = cu.m_cuDepth[0];
    uint32_t fullDepth = depth + trDepth;
    uint32_t log2TrSize = g_maxLog2CUSize - fullDepth;

    uint32_t trDepthC = trDepth;
    if (log2TrSize == 2)
    {
        X265_CHECK(m_csp != X265_CSP_I444 && trDepthC, "trDepthC invalid\n");
        trDepthC--;
    }

    uint32_t partIdxesPerSubTU = (NUM_CU_PARTITIONS >> ((depth + trDepthC) << 1)) >> 1;

    // move the CBFs down a level and set the parent CBF
    uint8_t subTUCBF[2];
    uint8_t combinedSubTUCBF = 0;

    for (uint32_t subTU = 0; subTU < 2; subTU++)
    {
        const uint32_t subTUAbsPartIdx = absPartIdx + (subTU * partIdxesPerSubTU);

        subTUCBF[subTU]   = cu.getCbf(subTUAbsPartIdx, ttype, trDepth);
        combinedSubTUCBF |= subTUCBF[subTU];
    }

    for (uint32_t subTU = 0; subTU < 2; subTU++)
    {
        const uint32_t subTUAbsPartIdx = absPartIdx + (subTU * partIdxesPerSubTU);
        const uint8_t compositeCBF = (subTUCBF[subTU] << 1) | combinedSubTUCBF;

        cu.setCbfPartRange((compositeCBF << trDepth), ttype, subTUAbsPartIdx, partIdxesPerSubTU);
    }
}

/* returns distortion */
uint32_t Search::codeIntraChromaQt(Mode& mode, const CUGeom& cuGeom, uint32_t trDepth, uint32_t absPartIdx, uint32_t& psyEnergy)
{
    CUData& cu = mode.cu;
    uint32_t fullDepth = cu.m_cuDepth[0] + trDepth;
    uint32_t tuDepthL  = cu.m_tuDepth[absPartIdx];

    if (tuDepthL > trDepth)
    {
        uint32_t qPartsDiv = NUM_CU_PARTITIONS >> ((fullDepth + 1) << 1);
        uint32_t outDist = 0, splitCbfU = 0, splitCbfV = 0;
        for (uint32_t subPartIdx = 0, absPartIdxSub = absPartIdx; subPartIdx < 4; subPartIdx++, absPartIdxSub += qPartsDiv)
        {
            outDist += codeIntraChromaQt(mode, cuGeom, trDepth + 1, absPartIdxSub, psyEnergy);
            splitCbfU |= cu.getCbf(absPartIdxSub, TEXT_CHROMA_U, trDepth + 1);
            splitCbfV |= cu.getCbf(absPartIdxSub, TEXT_CHROMA_V, trDepth + 1);
        }
        for (uint32_t offs = 0; offs < 4 * qPartsDiv; offs++)
        {
            cu.m_cbf[TEXT_CHROMA_U][absPartIdx + offs] |= (splitCbfU << trDepth);
            cu.m_cbf[TEXT_CHROMA_V][absPartIdx + offs] |= (splitCbfV << trDepth);
        }

        return outDist;
    }

    uint32_t log2TrSize = g_maxLog2CUSize - fullDepth;
    uint32_t log2TrSizeC = log2TrSize - m_hChromaShift;

    uint32_t trDepthC = trDepth;
    if (log2TrSizeC == 1)
    {
        X265_CHECK(log2TrSize == 2 && m_csp != X265_CSP_I444 && trDepth, "invalid trDepth\n");
        trDepthC--;
        log2TrSizeC++;
        uint32_t qpdiv = NUM_CU_PARTITIONS >> ((cu.m_cuDepth[0] + trDepthC) << 1);
        bool bFirstQ = ((absPartIdx & (qpdiv - 1)) == 0);
        if (!bFirstQ)
            return 0;
    }

    if (m_bEnableRDOQ)
        m_entropyCoder.estBit(m_entropyCoder.m_estBitsSbac, log2TrSizeC, false);

    bool checkTransformSkip = m_slice->m_pps->bTransformSkipEnabled && log2TrSizeC <= MAX_LOG2_TS_SIZE && !cu.m_tqBypass[0];
    checkTransformSkip &= !m_param->bEnableTSkipFast || (log2TrSize <= MAX_LOG2_TS_SIZE && cu.m_transformSkip[TEXT_LUMA][absPartIdx]);
    if (checkTransformSkip)
        return codeIntraChromaTSkip(mode, cuGeom, trDepth, trDepthC, absPartIdx, psyEnergy);

    uint32_t qtLayer = log2TrSize - 2;
    uint32_t tuSize = 1 << log2TrSizeC;
    uint32_t outDist = 0;

    uint32_t curPartNum = NUM_CU_PARTITIONS >> ((cu.m_cuDepth[0] + trDepthC) << 1);
    const SplitType splitType = (m_csp == X265_CSP_I422) ? VERTICAL_SPLIT : DONT_SPLIT;

    for (uint32_t chromaId = TEXT_CHROMA_U; chromaId <= TEXT_CHROMA_V; chromaId++)
    {
        TextType ttype = (TextType)chromaId;

        TURecurse tuIterator(splitType, curPartNum, absPartIdx);
        do
        {
            uint32_t absPartIdxC = tuIterator.absPartIdxTURelCU;

            pixel*   fenc     = const_cast<Yuv*>(mode.fencYuv)->getChromaAddr(chromaId, absPartIdxC);
            pixel*   pred     = mode.predYuv.getChromaAddr(chromaId, absPartIdxC);
            int16_t* residual = m_rqt[cuGeom.depth].tmpResiYuv.getChromaAddr(chromaId, absPartIdxC);
            uint32_t stride   = mode.fencYuv->m_csize;
            uint32_t sizeIdxC = log2TrSizeC - 2;

            uint32_t coeffOffsetC  = absPartIdxC << (LOG2_UNIT_SIZE * 2 - (m_hChromaShift + m_vChromaShift));
            coeff_t* coeffC        = m_rqt[qtLayer].coeffRQT[chromaId] + coeffOffsetC;
            pixel*   reconQt       = m_rqt[qtLayer].reconQtYuv.getChromaAddr(chromaId, absPartIdxC);
            uint32_t reconQtStride = m_rqt[qtLayer].reconQtYuv.m_csize;

            pixel*   picReconC = m_frame->m_reconPic->getChromaAddr(chromaId, cu.m_cuAddr, cuGeom.encodeIdx + absPartIdxC);
            intptr_t picStride = m_frame->m_reconPic->m_strideC;

            // init availability pattern
            initAdiPatternChroma(cu, cuGeom, absPartIdxC, trDepthC, chromaId);
            pixel* chromaPred = getAdiChromaBuf(chromaId, tuSize);

            uint32_t chromaPredMode = cu.m_chromaIntraDir[absPartIdxC];
            if (chromaPredMode == DM_CHROMA_IDX)
                chromaPredMode = cu.m_lumaIntraDir[(m_csp == X265_CSP_I444) ? absPartIdxC : 0];
            if (m_csp == X265_CSP_I422)
                chromaPredMode = g_chroma422IntraAngleMappingTable[chromaPredMode];

            // get prediction signal
            predIntraChromaAng(chromaPred, chromaPredMode, pred, stride, log2TrSizeC, m_csp);

            cu.setTransformSkipPartRange(0, ttype, absPartIdxC, tuIterator.absPartIdxStep);

            primitives.calcresidual[sizeIdxC](fenc, pred, residual, stride);
            uint32_t numSig = m_quant.transformNxN(cu, fenc, stride, residual, stride, coeffC, log2TrSizeC, ttype, absPartIdxC, false);
            uint32_t tmpDist;
            if (numSig)
            {
                m_quant.invtransformNxN(cu.m_tqBypass[0], residual, stride, coeffC, log2TrSizeC, ttype, true, false, numSig);
                primitives.luma_add_ps[sizeIdxC](reconQt, reconQtStride, pred, residual, stride, stride);
                cu.setCbfPartRange(1 << trDepth, ttype, absPartIdxC, tuIterator.absPartIdxStep);
            }
            else
            {
                // no coded residual, recon = pred
                primitives.square_copy_pp[sizeIdxC](reconQt, reconQtStride, pred, stride);
                cu.setCbfPartRange(0, ttype, absPartIdxC, tuIterator.absPartIdxStep);
            }

            tmpDist = primitives.sse_pp[sizeIdxC](reconQt, reconQtStride, fenc, stride);
            outDist += (ttype == TEXT_CHROMA_U) ? m_rdCost.scaleChromaDistCb(tmpDist) : m_rdCost.scaleChromaDistCr(tmpDist);

            if (m_rdCost.m_psyRd)
                psyEnergy += m_rdCost.psyCost(sizeIdxC, fenc, stride, picReconC, picStride);

            primitives.square_copy_pp[sizeIdxC](picReconC, picStride, reconQt, reconQtStride);
        }
        while (tuIterator.isNextSection());

        if (splitType == VERTICAL_SPLIT)
            offsetSubTUCBFs(cu, ttype, trDepth, absPartIdx);
    }

    return outDist;
}

/* returns distortion */
uint32_t Search::codeIntraChromaTSkip(Mode& mode, const CUGeom& cuGeom, uint32_t trDepth, uint32_t trDepthC, uint32_t absPartIdx, uint32_t& psyEnergy)
{
    CUData& cu = mode.cu;
    uint32_t fullDepth = cu.m_cuDepth[0] + trDepth;
    uint32_t log2TrSize = g_maxLog2CUSize - fullDepth;
    uint32_t log2TrSizeC = 2;
    uint32_t tuSize = 4;
    uint32_t qtLayer = log2TrSize - 2;
    uint32_t outDist = 0;

    /* At the TU layers above this one, no RDO is performed, only distortion is being measured,
     * so the entropy coder is not very accurate. The best we can do is return it in the same
     * condition as it arrived, and to do all bit estimates from the same state. */
    m_entropyCoder.store(m_rqt[fullDepth].rqtRoot);

    ALIGN_VAR_32(coeff_t, tskipCoeffC[MAX_TS_SIZE * MAX_TS_SIZE]);
    ALIGN_VAR_32(pixel,   tskipReconC[MAX_TS_SIZE * MAX_TS_SIZE]);

    uint32_t curPartNum = NUM_CU_PARTITIONS >> ((cu.m_cuDepth[0] + trDepthC) << 1);
    const SplitType splitType = (m_csp == X265_CSP_I422) ? VERTICAL_SPLIT : DONT_SPLIT;

    for (uint32_t chromaId = TEXT_CHROMA_U; chromaId <= TEXT_CHROMA_V; chromaId++)
    {
        TextType ttype = (TextType)chromaId;

        TURecurse tuIterator(splitType, curPartNum, absPartIdx);
        do
        {
            uint32_t absPartIdxC = tuIterator.absPartIdxTURelCU;

            pixel*   fenc = const_cast<Yuv*>(mode.fencYuv)->getChromaAddr(chromaId, absPartIdxC);
            pixel*   pred = mode.predYuv.getChromaAddr(chromaId, absPartIdxC);
            int16_t* residual = m_rqt[cuGeom.depth].tmpResiYuv.getChromaAddr(chromaId, absPartIdxC);
            uint32_t stride = mode.fencYuv->m_csize;
            uint32_t sizeIdxC = log2TrSizeC - 2;

            uint32_t coeffOffsetC = absPartIdxC << (LOG2_UNIT_SIZE * 2 - (m_hChromaShift + m_vChromaShift));
            coeff_t* coeffC = m_rqt[qtLayer].coeffRQT[chromaId] + coeffOffsetC;
            pixel*   reconQt = m_rqt[qtLayer].reconQtYuv.getChromaAddr(chromaId, absPartIdxC);
            uint32_t reconQtStride = m_rqt[qtLayer].reconQtYuv.m_csize;

            // init availability pattern
            initAdiPatternChroma(cu, cuGeom, absPartIdxC, trDepthC, chromaId);
            pixel* chromaPred = getAdiChromaBuf(chromaId, tuSize);

            uint32_t chromaPredMode = cu.m_chromaIntraDir[absPartIdxC];
            if (chromaPredMode == DM_CHROMA_IDX)
                chromaPredMode = cu.m_lumaIntraDir[(m_csp == X265_CSP_I444) ? absPartIdxC : 0];
            if (m_csp == X265_CSP_I422)
                chromaPredMode = g_chroma422IntraAngleMappingTable[chromaPredMode];

            // get prediction signal
            predIntraChromaAng(chromaPred, chromaPredMode, pred, stride, log2TrSizeC, m_csp);

            uint64_t bCost = MAX_INT64;
            uint32_t bDist = 0;
            uint32_t bCbf = 0;
            uint32_t bEnergy = 0;
            int      bTSkip = 0;

            int checkTransformSkip = 1;
            for (int useTSkip = 0; useTSkip <= checkTransformSkip; useTSkip++)
            {
                coeff_t* coeff = (useTSkip ? tskipCoeffC : coeffC);
                pixel*   recon = (useTSkip ? tskipReconC : reconQt);
                uint32_t reconStride = (useTSkip ? MAX_TS_SIZE : reconQtStride);

                primitives.calcresidual[sizeIdxC](fenc, pred, residual, stride);

                uint32_t numSig = m_quant.transformNxN(cu, fenc, stride, residual, stride, coeff, log2TrSizeC, ttype, absPartIdxC, useTSkip);
                if (numSig)
                {
                    m_quant.invtransformNxN(cu.m_tqBypass[0], residual, stride, coeff, log2TrSizeC, ttype, true, useTSkip, numSig);
                    primitives.luma_add_ps[sizeIdxC](recon, reconStride, pred, residual, stride, stride);
                    cu.setCbfPartRange(1 << trDepth, ttype, absPartIdxC, tuIterator.absPartIdxStep);
                }
                else if (useTSkip)
                {
                    checkTransformSkip = 0;
                    break;
                }
                else
                {
                    primitives.square_copy_pp[sizeIdxC](recon, reconStride, pred, stride);
                    cu.setCbfPartRange(0, ttype, absPartIdxC, tuIterator.absPartIdxStep);
                }
                uint32_t tmpDist = primitives.sse_pp[sizeIdxC](recon, reconStride, fenc, stride);
                tmpDist = (ttype == TEXT_CHROMA_U) ? m_rdCost.scaleChromaDistCb(tmpDist) : m_rdCost.scaleChromaDistCr(tmpDist);

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
                memcpy(coeffC, tskipCoeffC, sizeof(coeff_t) << (log2TrSizeC * 2));
                primitives.square_copy_pp[sizeIdxC](reconQt, reconQtStride, tskipReconC, MAX_TS_SIZE);
            }

            cu.setCbfPartRange(bCbf << trDepth, ttype, absPartIdxC, tuIterator.absPartIdxStep);
            cu.setTransformSkipPartRange(bTSkip, ttype, absPartIdxC, tuIterator.absPartIdxStep);

            pixel*   reconPicC = m_frame->m_reconPic->getChromaAddr(chromaId, cu.m_cuAddr, cuGeom.encodeIdx + absPartIdxC);
            intptr_t picStride = m_frame->m_reconPic->m_strideC;
            primitives.square_copy_pp[sizeIdxC](reconPicC, picStride, reconQt, reconQtStride);

            outDist += bDist;
            psyEnergy += bEnergy;
        }
        while (tuIterator.isNextSection());

        if (splitType == VERTICAL_SPLIT)
            offsetSubTUCBFs(cu, ttype, trDepth, absPartIdx);
    }

    m_entropyCoder.load(m_rqt[fullDepth].rqtRoot);
    return outDist;
}

void Search::extractIntraResultChromaQT(CUData& cu, Yuv& reconYuv, uint32_t absPartIdx, uint32_t trDepth, bool tuQuad)
{
    uint32_t fullDepth = cu.m_cuDepth[0] + trDepth;
    uint32_t tuDepthL  = cu.m_tuDepth[absPartIdx];

    if (tuDepthL == trDepth)
    {
        uint32_t log2TrSize = g_maxLog2CUSize - fullDepth;
        uint32_t log2TrSizeC = log2TrSize - m_hChromaShift;

        if (tuQuad)
        {
            log2TrSizeC++; /* extract one 4x4 instead of 4 2x2 */
            trDepth--;     /* also adjust the number of coeff read */
        }

        // copy transform coefficients
        uint32_t numCoeffC = 1 << (log2TrSizeC * 2 + (m_csp == X265_CSP_I422));
        uint32_t coeffOffsetC = absPartIdx << (LOG2_UNIT_SIZE * 2 - (m_hChromaShift + m_vChromaShift));

        uint32_t qtLayer   = log2TrSize - 2;
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
        if (g_maxLog2CUSize - fullDepth - 1 == 2 && m_csp != X265_CSP_I444)
            /* no such thing as chroma 2x2, so extract one 4x4 instead of 4 2x2 */
            extractIntraResultChromaQT(cu, reconYuv, absPartIdx, trDepth + 1, true);
        else
        {
            uint32_t numQPart = NUM_CU_PARTITIONS >> ((fullDepth + 1) << 1);
            for (uint32_t subPartIdx = 0; subPartIdx < 4; subPartIdx++)
                extractIntraResultChromaQT(cu, reconYuv, absPartIdx + subPartIdx * numQPart, trDepth + 1, false);
        }
    }
}

void Search::residualQTIntraChroma(Mode& mode, const CUGeom& cuGeom, uint32_t trDepth, uint32_t absPartIdx)
{
    CUData& cu = mode.cu;
    uint32_t fullDepth = cu.m_cuDepth[0] + trDepth;
    uint32_t tuDepthL  = cu.m_tuDepth[absPartIdx];
    
    if (tuDepthL == trDepth)
    {
        uint32_t log2TrSize = g_maxLog2CUSize - fullDepth;
        uint32_t log2TrSizeC = log2TrSize - m_hChromaShift;
        uint32_t trDepthC = trDepth;
        if (log2TrSizeC == 1)
        {
            X265_CHECK(log2TrSize == 2 && m_csp != X265_CSP_I444 && trDepth > 0, "invalid trDepth\n");
            trDepthC--;
            log2TrSizeC++;
            uint32_t qpdiv = NUM_CU_PARTITIONS >> ((cu.m_cuDepth[0] + trDepthC) << 1);
            bool bFirstQ = ((absPartIdx & (qpdiv - 1)) == 0);
            if (!bFirstQ)
                return;
        }

        ShortYuv& resiYuv = m_rqt[cuGeom.depth].tmpResiYuv;
        uint32_t tuSize = 1 << log2TrSizeC;
        uint32_t stride = mode.fencYuv->m_csize;
        const int sizeIdxC = log2TrSizeC - 2;

        uint32_t curPartNum = NUM_CU_PARTITIONS >> ((cu.m_cuDepth[0] + trDepthC) << 1);
        const SplitType splitType = (m_csp == X265_CSP_I422) ? VERTICAL_SPLIT : DONT_SPLIT;

        for (uint32_t chromaId = TEXT_CHROMA_U; chromaId <= TEXT_CHROMA_V; chromaId++)
        {
            TextType ttype = (TextType)chromaId;

            TURecurse tuIterator(splitType, curPartNum, absPartIdx);
            do
            {
                uint32_t absPartIdxC = tuIterator.absPartIdxTURelCU;

                pixel*   fenc         = const_cast<pixel*>(mode.fencYuv->getChromaAddr(chromaId, absPartIdxC));
                pixel*   pred         = mode.predYuv.getChromaAddr(chromaId, absPartIdxC);
                int16_t* residual     = resiYuv.getChromaAddr(chromaId, absPartIdxC);
                pixel*   recon        = mode.reconYuv.getChromaAddr(chromaId, absPartIdxC); // TODO: needed?
                uint32_t coeffOffsetC = absPartIdxC << (LOG2_UNIT_SIZE * 2 - (m_hChromaShift + m_vChromaShift));
                coeff_t* coeff        = cu.m_trCoeff[ttype] + coeffOffsetC;
                pixel*   picReconC    = m_frame->m_reconPic->getChromaAddr(chromaId, cu.m_cuAddr, cuGeom.encodeIdx + absPartIdxC);
                uint32_t picStride    = m_frame->m_reconPic->m_strideC;

                uint32_t chromaPredMode = cu.m_chromaIntraDir[absPartIdxC];
                if (chromaPredMode == DM_CHROMA_IDX)
                    chromaPredMode = cu.m_lumaIntraDir[(m_csp == X265_CSP_I444) ? absPartIdxC : 0];
                chromaPredMode = (m_csp == X265_CSP_I422) ? g_chroma422IntraAngleMappingTable[chromaPredMode] : chromaPredMode;
                initAdiPatternChroma(cu, cuGeom, absPartIdxC, trDepthC, chromaId);
                pixel* chromaPred = getAdiChromaBuf(chromaId, tuSize);

                predIntraChromaAng(chromaPred, chromaPredMode, pred, stride, log2TrSizeC, m_csp);

                X265_CHECK(!cu.m_transformSkip[ttype][0], "transform skip not supported at low RD levels\n");

                primitives.calcresidual[sizeIdxC](fenc, pred, residual, stride);
                uint32_t numSig = m_quant.transformNxN(cu, fenc, stride, residual, stride, coeff, log2TrSizeC, ttype, absPartIdxC, false);
                if (numSig)
                {
                    m_quant.invtransformNxN(cu.m_tqBypass[absPartIdxC], residual, stride, coeff, log2TrSizeC, ttype, true, false, numSig);
                    primitives.luma_add_ps[sizeIdxC](recon, stride, pred, residual, stride, stride);
                    primitives.square_copy_pp[sizeIdxC](picReconC, picStride, recon, stride);
                    cu.setCbfPartRange(1 << trDepth, ttype, absPartIdxC, tuIterator.absPartIdxStep);
                }
                else
                {
                    primitives.square_copy_pp[sizeIdxC](recon, stride, pred, stride);
                    primitives.square_copy_pp[sizeIdxC](picReconC, picStride, pred, stride);
                    cu.setCbfPartRange(0, ttype, absPartIdxC, tuIterator.absPartIdxStep);
                }
            }
            while (tuIterator.isNextSection());

            if (splitType == VERTICAL_SPLIT)
                offsetSubTUCBFs(cu, (TextType)chromaId, trDepth, absPartIdx);
        }
    }
    else
    {
        uint32_t qPartsDiv = NUM_CU_PARTITIONS >> ((fullDepth + 1) << 1);
        uint32_t splitCbfU = 0, splitCbfV = 0;
        for (uint32_t subPartIdx = 0, absPartIdxC = absPartIdx; subPartIdx < 4; subPartIdx++, absPartIdxC += qPartsDiv)
        {
            residualQTIntraChroma(mode, cuGeom, trDepth + 1, absPartIdxC);
            splitCbfU |= cu.getCbf(absPartIdxC, TEXT_CHROMA_U, trDepth + 1);
            splitCbfV |= cu.getCbf(absPartIdxC, TEXT_CHROMA_V, trDepth + 1);
        }
        for (uint32_t offs = 0; offs < 4 * qPartsDiv; offs++)
        {
            cu.m_cbf[1][absPartIdx + offs] |= (splitCbfU << trDepth);
            cu.m_cbf[2][absPartIdx + offs] |= (splitCbfV << trDepth);
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
    cu.getIntraTUQtDepthRange(tuDepthRange, 0);

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

/* Note that this function does not save the best intra prediction, it must
 * be generated later. It records the best mode in the cu */
void Search::checkIntraInInter(Mode& intraMode, const CUGeom& cuGeom)
{
    CUData& cu = intraMode.cu;
    uint32_t depth = cu.m_cuDepth[0];

    cu.setPartSizeSubParts(SIZE_2Nx2N);
    cu.setPredModeSubParts(MODE_INTRA);

    const uint32_t initTrDepth = 0;
    uint32_t log2TrSize = cu.m_log2CUSize[0] - initTrDepth;
    uint32_t tuSize = 1 << log2TrSize;
    const uint32_t absPartIdx = 0;

    // Reference sample smoothing
    initAdiPattern(cu, cuGeom, absPartIdx, initTrDepth, ALL_IDX);

    pixel* fenc = intraMode.fencYuv->m_buf[0];
    uint32_t stride = intraMode.fencYuv->m_size;

    pixel *above = m_refAbove + tuSize - 1;
    pixel *aboveFiltered = m_refAboveFlt + tuSize - 1;
    pixel *left = m_refLeft + tuSize - 1;
    pixel *leftFiltered = m_refLeftFlt + tuSize - 1;
    int sad, bsad;
    uint32_t bits, bbits, mode, bmode;
    uint64_t cost, bcost;

    // 33 Angle modes once
    ALIGN_VAR_32(pixel, bufScale[32 * 32]);
    ALIGN_VAR_32(pixel, bufTrans[32 * 32]);
    ALIGN_VAR_32(pixel, tmp[33 * 32 * 32]);
    int scaleTuSize = tuSize;
    int scaleStride = stride;
    int costShift = 0;
    int sizeIdx = log2TrSize - 2;

    if (tuSize > 32)
    {
        // origin is 64x64, we scale to 32x32 and setup required parameters
        primitives.scale2D_64to32(bufScale, fenc, stride);
        fenc = bufScale;

        // reserve space in case primitives need to store data in above
        // or left buffers
        pixel _above[4 * 32 + 1];
        pixel _left[4 * 32 + 1];
        pixel *aboveScale = _above + 2 * 32;
        pixel *leftScale = _left + 2 * 32;
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

    pixelcmp_t sa8d = primitives.sa8d[sizeIdx];
    int predsize = scaleTuSize * scaleTuSize;

    m_entropyCoder.loadIntraDirModeLuma(m_rqt[depth].cur);

    /* there are three cost tiers for intra modes:
    *  pred[0]          - mode probable, least cost
    *  pred[1], pred[2] - less probable, slightly more cost
    *  non-mpm modes    - all cost the same (rbits) */
    uint64_t mpms;
    uint32_t preds[3];
    uint32_t rbits = getIntraRemModeBits(cu, absPartIdx, preds, mpms);

    // DC
    primitives.intra_pred[DC_IDX][sizeIdx](tmp, scaleStride, left, above, 0, (scaleTuSize <= 16));
    bsad = sa8d(fenc, scaleStride, tmp, scaleStride) << costShift;
    bmode = mode = DC_IDX;
    bbits = (mpms & ((uint64_t)1 << mode)) ? m_entropyCoder.bitsIntraModeMPM(preds, mode) : rbits;
    bcost = m_rdCost.calcRdSADCost(bsad, bbits);

    pixel *abovePlanar = above;
    pixel *leftPlanar = left;

    if (tuSize & (8 | 16 | 32))
    {
        abovePlanar = aboveFiltered;
        leftPlanar = leftFiltered;
    }

    // PLANAR
    primitives.intra_pred[PLANAR_IDX][sizeIdx](tmp, scaleStride, leftPlanar, abovePlanar, 0, 0);
    sad = sa8d(fenc, scaleStride, tmp, scaleStride) << costShift;
    mode = PLANAR_IDX;
    bits = (mpms & ((uint64_t)1 << mode)) ? m_entropyCoder.bitsIntraModeMPM(preds, mode) : rbits;
    cost = m_rdCost.calcRdSADCost(sad, bits);
    COPY4_IF_LT(bcost, cost, bmode, mode, bsad, sad, bbits, bits);

    // Transpose NxN
    primitives.transpose[sizeIdx](bufTrans, fenc, scaleStride);

    primitives.intra_pred_allangs[sizeIdx](tmp, above, left, aboveFiltered, leftFiltered, (scaleTuSize <= 16));

    bool modeHor;
    pixel *cmp;
    intptr_t srcStride;

#define TRY_ANGLE(angle) \
    modeHor = angle < 18; \
    cmp = modeHor ? bufTrans : fenc; \
    srcStride = modeHor ? scaleTuSize : scaleStride; \
    sad = sa8d(cmp, srcStride, &tmp[(angle - 2) * predsize], scaleTuSize) << costShift; \
    bits = (mpms & ((uint64_t)1 << angle)) ? m_entropyCoder.bitsIntraModeMPM(preds, angle) : rbits; \
    cost = m_rdCost.calcRdSADCost(sad, bits)

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

    cu.setLumaIntraDirSubParts((uint8_t)bmode, absPartIdx, depth + initTrDepth);
    intraMode.initCosts();
    intraMode.totalBits = bbits;
    intraMode.distortion = bsad;
    intraMode.sa8dCost = bcost;
    intraMode.sa8dBits = bbits;
}

void Search::encodeIntraInInter(Mode& intraMode, const CUGeom& cuGeom)
{
    CUData& cu = intraMode.cu;
    Yuv* reconYuv = &intraMode.reconYuv;
    const Yuv* fencYuv = intraMode.fencYuv;

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
    intraMode.distortion += estIntraPredChromaQT(intraMode, cuGeom);

    m_entropyCoder.resetBits();
    if (m_slice->m_pps->bTransquantBypassEnabled)
        m_entropyCoder.codeCUTransquantBypassFlag(cu.m_tqBypass[0]);
    m_entropyCoder.codeSkipFlag(cu, 0);
    m_entropyCoder.codePredMode(cu.m_predMode[0]);
    m_entropyCoder.codePartSize(cu, 0, cuGeom.depth);
    m_entropyCoder.codePredInfo(cu, 0);
    intraMode.mvBits += m_entropyCoder.getNumberOfWrittenBits();

    bool bCodeDQP = m_slice->m_pps->bUseDQP;
    m_entropyCoder.codeCoeff(cu, 0, cuGeom.depth, bCodeDQP, tuDepthRange);

    intraMode.totalBits = m_entropyCoder.getNumberOfWrittenBits();
    intraMode.coeffBits = intraMode.totalBits - intraMode.mvBits;
    if (m_rdCost.m_psyRd)
        intraMode.psyEnergy = m_rdCost.psyCost(cuGeom.log2CUSize - 2, fencYuv->m_buf[0], fencYuv->m_size, reconYuv->m_buf[0], reconYuv->m_size);

    m_entropyCoder.store(intraMode.contexts);
    updateModeCost(intraMode);
}

uint32_t Search::estIntraPredQT(Mode &intraMode, const CUGeom& cuGeom, uint32_t depthRange[2], uint8_t* sharedModes)
{
    CUData& cu = intraMode.cu;
    Yuv* reconYuv = &intraMode.reconYuv;
    Yuv* predYuv = &intraMode.predYuv;
    const Yuv* fencYuv = intraMode.fencYuv;

    uint32_t depth        = cu.m_cuDepth[0];
    uint32_t initTrDepth  = cu.m_partSize[0] != SIZE_2Nx2N;
    uint32_t numPU        = 1 << (2 * initTrDepth);
    uint32_t log2TrSize   = cu.m_log2CUSize[0] - initTrDepth;
    uint32_t tuSize       = 1 << log2TrSize;
    uint32_t qNumParts    = cuGeom.numPartitions >> 2;
    uint32_t sizeIdx      = log2TrSize - 2;
    uint32_t absPartIdx   = 0;
    uint32_t totalDistortion = 0;

    int checkTransformSkip = m_slice->m_pps->bTransformSkipEnabled && !cu.m_tqBypass[0] && cu.m_partSize[0] != SIZE_2Nx2N;

    // loop over partitions
    for (uint32_t pu = 0; pu < numPU; pu++, absPartIdx += qNumParts)
    {
        uint32_t bmode = 0;

        if (sharedModes)
            bmode = sharedModes[pu];
        else
        {
            // Reference sample smoothing
            initAdiPattern(cu, cuGeom, absPartIdx, initTrDepth, ALL_IDX);

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

            m_entropyCoder.loadIntraDirModeLuma(m_rqt[depth].cur);

            /* there are three cost tiers for intra modes:
             *  pred[0]          - mode probable, least cost
             *  pred[1], pred[2] - less probable, slightly more cost
             *  non-mpm modes    - all cost the same (rbits) */
            uint64_t mpms;
            uint32_t preds[3];
            uint32_t rbits = getIntraRemModeBits(cu, absPartIdx, preds, mpms);

            pixelcmp_t sa8d = primitives.sa8d[sizeIdx];
            uint64_t modeCosts[35];
            uint64_t bcost;

            // DC
            primitives.intra_pred[DC_IDX][sizeIdx](tmp, scaleStride, left, above, 0, (scaleTuSize <= 16));
            uint32_t bits = (mpms & ((uint64_t)1 << DC_IDX)) ? m_entropyCoder.bitsIntraModeMPM(preds, DC_IDX) : rbits;
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
            bits = (mpms & ((uint64_t)1 << PLANAR_IDX)) ? m_entropyCoder.bitsIntraModeMPM(preds, PLANAR_IDX) : rbits;
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
                bits = (mpms & ((uint64_t)1 << mode)) ? m_entropyCoder.bitsIntraModeMPM(preds, mode) : rbits;
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
                cu.setLumaIntraDirSubParts(rdModeList[i], absPartIdx, depth + initTrDepth);

                Cost icosts;
                if (checkTransformSkip)
                    codeIntraLumaTSkip(intraMode, cuGeom, initTrDepth, absPartIdx, icosts);
                else
                    codeIntraLumaQT(intraMode, cuGeom, initTrDepth, absPartIdx, false, icosts, depthRange);
                COPY2_IF_LT(bcost, icosts.rdcost, bmode, rdModeList[i]);
            }
        }

        /* remeasure best mode, allowing TU splits */
        cu.setLumaIntraDirSubParts(bmode, absPartIdx, depth + initTrDepth);
        m_entropyCoder.load(m_rqt[depth].cur);

        Cost icosts;
        if (checkTransformSkip)
            codeIntraLumaTSkip(intraMode, cuGeom, initTrDepth, absPartIdx, icosts);
        else
            codeIntraLumaQT(intraMode, cuGeom, initTrDepth, absPartIdx, true, icosts, depthRange);
        totalDistortion += icosts.distortion;

        extractIntraResultQT(cu, *reconYuv, initTrDepth, absPartIdx);

        // set reconstruction for next intra prediction blocks
        if (pu != numPU - 1)
        {
            /* This has important implications for parallelism and RDO.  It is writing intermediate results into the
             * output recon picture, so it cannot proceed in parallel with anything else when doing INTRA_NXN. Also
             * it is not updating m_rdContexts[depth].cur for the later PUs which I suspect is slightly wrong. I think
             * that the contexts should be tracked through each PU */
            pixel*   dst         = m_frame->m_reconPic->getLumaAddr(cu.m_cuAddr, cuGeom.encodeIdx + absPartIdx);
            uint32_t dststride   = m_frame->m_reconPic->m_stride;
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
            combCbfY |= cu.getCbf(partIdx, TEXT_LUMA, 1);

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
    int32_t scaleTuSize = tuSize;
    int32_t costShift = 0;

    if (tuSize > 32)
    {
        scaleTuSize = 32;
        costShift = 2;
        log2TrSizeC = 5;
    }

    Predict::initAdiPatternChroma(cu, cuGeom, 0, 0, 1);
    Predict::initAdiPatternChroma(cu, cuGeom, 0, 0, 2);
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

    cu.setChromIntraDirSubParts(bestMode, 0, cu.m_cuDepth[0]);
}

uint32_t Search::estIntraPredChromaQT(Mode &intraMode, const CUGeom& cuGeom)
{
    CUData& cu = intraMode.cu;
    Yuv& reconYuv = intraMode.reconYuv;

    uint32_t depth       = cu.m_cuDepth[0];
    uint32_t initTrDepth = cu.m_partSize[0] != SIZE_2Nx2N && m_csp == X265_CSP_I444;
    uint32_t log2TrSize  = cu.m_log2CUSize[0] - initTrDepth;
    uint32_t absPartStep = (NUM_CU_PARTITIONS >> (depth << 1));
    uint32_t totalDistortion = 0;

    int part = partitionFromLog2Size(log2TrSize);

    TURecurse tuIterator((initTrDepth == 0) ? DONT_SPLIT : QUAD_SPLIT, absPartStep, 0);

    do
    {
        uint32_t absPartIdxC = tuIterator.absPartIdxTURelCU;
        int cuSize = 1 << cu.m_log2CUSize[absPartIdxC];

        uint32_t bestMode = 0;
        uint32_t bestDist = 0;
        uint64_t bestCost = MAX_INT64;

        // init mode list
        uint32_t minMode = 0;
        uint32_t maxMode = NUM_CHROMA_MODE;
        uint32_t modeList[NUM_CHROMA_MODE];

        cu.getAllowedChromaDir(absPartIdxC, modeList);

        // check chroma modes
        for (uint32_t mode = minMode; mode < maxMode; mode++)
        {
            // restore context models
            m_entropyCoder.load(m_rqt[depth].cur);

            cu.setChromIntraDirSubParts(modeList[mode], absPartIdxC, depth + initTrDepth);
            uint32_t psyEnergy = 0;
            uint32_t dist = codeIntraChromaQt(intraMode, cuGeom, initTrDepth, absPartIdxC, psyEnergy);

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
                uint32_t qtNumParts = cuGeom.numPartitions >> 2;
                if (!(absPartIdxC & (qtNumParts - 1)))
                    m_entropyCoder.codeIntraDirChroma(cu, absPartIdxC, modeList);
            }

            codeSubdivCbfQTChroma(cu, initTrDepth, absPartIdxC, tuIterator.absPartIdxStep, cuSize, cuSize);
            codeCoeffQTChroma(cu, initTrDepth, absPartIdxC, TEXT_CHROMA_U);
            codeCoeffQTChroma(cu, initTrDepth, absPartIdxC, TEXT_CHROMA_V);
            uint32_t bits = m_entropyCoder.getNumberOfWrittenBits();
            uint64_t cost = m_rdCost.m_psyRd ? m_rdCost.calcPsyRdCost(dist, bits, psyEnergy) : m_rdCost.calcRdCost(dist, bits);

            if (cost < bestCost)
            {
                bestCost = cost;
                bestDist = dist;
                bestMode = modeList[mode];
                extractIntraResultChromaQT(cu, reconYuv, absPartIdxC, initTrDepth, false);
                memcpy(m_qtTempCbf[1], cu.m_cbf[1] + absPartIdxC, tuIterator.absPartIdxStep * sizeof(uint8_t));
                memcpy(m_qtTempCbf[2], cu.m_cbf[2] + absPartIdxC, tuIterator.absPartIdxStep * sizeof(uint8_t));
                memcpy(m_qtTempTransformSkipFlag[1], cu.m_transformSkip[1] + absPartIdxC, tuIterator.absPartIdxStep * sizeof(uint8_t));
                memcpy(m_qtTempTransformSkipFlag[2], cu.m_transformSkip[2] + absPartIdxC, tuIterator.absPartIdxStep * sizeof(uint8_t));
            }
        }

        if (!tuIterator.isLastSection())
        {
            uint32_t zorder    = cuGeom.encodeIdx + absPartIdxC;
            uint32_t dststride = m_frame->m_reconPic->m_strideC;
            pixel *src, *dst;

            dst = m_frame->m_reconPic->getCbAddr(cu.m_cuAddr, zorder);
            src = reconYuv.getCbAddr(absPartIdxC);
            primitives.chroma[m_csp].copy_pp[part](dst, dststride, src, reconYuv.m_csize);

            dst = m_frame->m_reconPic->getCrAddr(cu.m_cuAddr, zorder);
            src = reconYuv.getCrAddr(absPartIdxC);
            primitives.chroma[m_csp].copy_pp[part](dst, dststride, src, reconYuv.m_csize);
        }

        memcpy(cu.m_cbf[1] + absPartIdxC, m_qtTempCbf[1], tuIterator.absPartIdxStep * sizeof(uint8_t));
        memcpy(cu.m_cbf[2] + absPartIdxC, m_qtTempCbf[2], tuIterator.absPartIdxStep * sizeof(uint8_t));
        memcpy(cu.m_transformSkip[1] + absPartIdxC, m_qtTempTransformSkipFlag[1], tuIterator.absPartIdxStep * sizeof(uint8_t));
        memcpy(cu.m_transformSkip[2] + absPartIdxC, m_qtTempTransformSkipFlag[2], tuIterator.absPartIdxStep * sizeof(uint8_t));
        cu.setChromIntraDirSubParts(bestMode, absPartIdxC, depth + initTrDepth);
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
            combCbfU |= cu.getCbf(partIdx, TEXT_CHROMA_U, 1);
            combCbfV |= cu.getCbf(partIdx, TEXT_CHROMA_V, 1);
        }

        for (uint32_t offs = 0; offs < 4 * tuIterator.absPartIdxStep; offs++)
        {
            cu.m_cbf[1][offs] |= combCbfU;
            cu.m_cbf[2][offs] |= combCbfV;
        }
    }

    /* TODO: remove this */
    m_entropyCoder.load(m_rqt[depth].cur);
    return totalDistortion;
}

/* estimation of best merge coding of an inter PU (not a merge CU) */
uint32_t Search::mergeEstimation(CUData& cu, const CUGeom& cuGeom, int puIdx, MergeData& m)
{
    X265_CHECK(cu.m_partSize[0] != SIZE_2Nx2N, "merge tested on non-2Nx2N partition\n");

    m.maxNumMergeCand = cu.getInterMergeCandidates(m.absPartIdx, puIdx, m.mvFieldNeighbours, m.interDirNeighbours);

    if (cu.isBipredRestriction())
    {
        /* in 8x8 CUs do not allow bidir merge candidates if not 2Nx2N */
        for (uint32_t mergeCand = 0; mergeCand < m.maxNumMergeCand; ++mergeCand)
        {
            if (m.interDirNeighbours[mergeCand] == 3)
            {
                m.interDirNeighbours[mergeCand] = 1;
                m.mvFieldNeighbours[mergeCand][1].refIdx = REF_NOT_VALID;
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

        cu.m_mv[0][m.absPartIdx] = m.mvFieldNeighbours[mergeCand][0].mv;
        cu.m_refIdx[0][m.absPartIdx] = (char)m.mvFieldNeighbours[mergeCand][0].refIdx;
        cu.m_mv[1][m.absPartIdx] = m.mvFieldNeighbours[mergeCand][1].mv;
        cu.m_refIdx[1][m.absPartIdx] = (char)m.mvFieldNeighbours[mergeCand][1].refIdx;

        prepMotionCompensation(cu, cuGeom, puIdx);
        motionCompensation(tempYuv, true, false);
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

/* this function assumes the caller has configured its MotionEstimation engine with the
 * correct source plane and source PU, and has called prepMotionCompensation() to set
 * m_puAbsPartIdx, m_puWidth, and m_puHeight */
void Search::singleMotionEstimation(Search& master, Mode& interMode, const CUGeom& cuGeom, int part, int list, int ref)
{
    uint32_t bits = master.m_listSelBits[list] + MVP_IDX_BITS;
    bits += getTUBits(ref, m_slice->m_numRefIdx[list]);

    MV mvc[(MD_ABOVE_LEFT + 1) * 2 + 1];
    int numMvc = interMode.cu.fillMvpCand(part, m_puAbsPartIdx, list, ref, interMode.amvpCand[list][ref], mvc);

    uint32_t bestCost = MAX_INT;
    int mvpIdx = 0;
    int merange = m_param->searchRange;
    for (int i = 0; i < AMVP_NUM_CANDS; i++)
    {
        MV mvCand = interMode.amvpCand[list][ref][i];

        // NOTE: skip mvCand if Y is > merange and -FN>1
        if (m_bFrameParallel && (mvCand.y >= (merange + 1) * 4))
            continue;

        interMode.cu.clipMv(mvCand);

        Yuv& tmpPredYuv = m_rqt[cuGeom.depth].tmpPredYuv;
        predInterLumaPixel(tmpPredYuv, *m_slice->m_refPicList[list][ref]->m_reconPic, mvCand);
        uint32_t cost = m_me.bufSAD(tmpPredYuv.getLumaAddr(m_puAbsPartIdx), tmpPredYuv.m_size);

        if (bestCost > cost)
        {
            bestCost = cost;
            mvpIdx = i;
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
    ScopedLock _lock(master.m_outputLock);
    if (cost < interMode.bestME[list].cost ||
       (cost == interMode.bestME[list].cost && ref < interMode.bestME[list].ref))
    {
        interMode.bestME[list].mv = outmv;
        interMode.bestME[list].mvp = mvp;
        interMode.bestME[list].mvpIdx = mvpIdx;
        interMode.bestME[list].ref = ref;
        interMode.bestME[list].cost = cost;
        interMode.bestME[list].bits = bits;
    }
}

/* search of the best candidate for inter prediction
 * returns true if predYuv was filled with a motion compensated prediction */
bool Search::predInterSearch(Mode& interMode, const CUGeom& cuGeom, bool bMergeOnly, bool bChroma)
{
    CUData& cu = interMode.cu;
    Yuv* predYuv = &interMode.predYuv;

    MV mvc[(MD_ABOVE_LEFT + 1) * 2 + 1];

    const Slice *slice = m_slice;
    PicYuv* fencPic = m_frame->m_fencPic;
    int numPart     = cu.getNumPartInter();
    int numPredDir  = slice->isInterP() ? 1 : 2;
    const int* numRefIdx = slice->m_numRefIdx;
    uint32_t lastMode = 0;
    int      totalmebits = 0;
    bool     bDistributed = m_param->bDistributeMotionEstimation && (numRefIdx[0] + numRefIdx[1]) > 2;
    MV       mvzero(0, 0);
    Yuv&     tmpPredYuv = m_rqt[cuGeom.depth].tmpPredYuv;

    MergeData merge;
    memset(&merge, 0, sizeof(merge));

    for (int puIdx = 0; puIdx < numPart; puIdx++)
    {
        /* sets m_puAbsPartIdx, m_puWidth, m_puHeight */
        initMotionCompensation(cu, cuGeom, puIdx);

        pixel* pu = fencPic->getLumaAddr(cu.m_cuAddr, cuGeom.encodeIdx + m_puAbsPartIdx);
        m_me.setSourcePU(pu - fencPic->m_picOrg[0], m_puWidth, m_puHeight);

        uint32_t mrgCost = MAX_UINT;

        /* find best cost merge candidate */
        if (cu.m_partSize[0] != SIZE_2Nx2N)
        {
            merge.absPartIdx = m_puAbsPartIdx;
            merge.width      = m_puWidth;
            merge.height     = m_puHeight;
            mrgCost = mergeEstimation(cu, cuGeom, puIdx, merge);

            if (bMergeOnly && cu.m_log2CUSize[0] > 3)
            {
                if (mrgCost == MAX_UINT)
                {
                    /* No valid merge modes were found, there is no possible way to
                     * perform a valid motion compensation prediction, so early-exit */
                    return false;
                }
                // set merge result
                cu.m_mergeFlag[m_puAbsPartIdx] = true;
                cu.m_mvpIdx[0][m_puAbsPartIdx] = merge.index; // merge candidate ID is stored in L0 MVP idx
                cu.setPUInterDir(merge.interDir, m_puAbsPartIdx, puIdx);
                cu.setPUMv(0, merge.mvField[0].mv, m_puAbsPartIdx, puIdx);
                cu.setPURefIdx(0, merge.mvField[0].refIdx, m_puAbsPartIdx, puIdx);
                cu.setPUMv(1, merge.mvField[1].mv, m_puAbsPartIdx, puIdx);
                cu.setPURefIdx(1, merge.mvField[1].refIdx, m_puAbsPartIdx, puIdx);
                totalmebits += merge.bits;

                prepMotionCompensation(cu, cuGeom, puIdx);
                motionCompensation(*predYuv, true, bChroma);
                continue;
            }
        }

        MotionData bidir[2];
        uint32_t bidirCost = MAX_UINT;
        int bidirBits = 0;

        interMode.bestME[0].cost = MAX_UINT;
        interMode.bestME[1].cost = MAX_UINT;

        getBlkBits((PartSize)cu.m_partSize[0], slice->isInterP(), puIdx, lastMode, m_listSelBits);

        /* Uni-directional prediction */
        if (m_param->analysisMode == X265_ANALYSIS_LOAD && interMode.bestME[0].ref >= 0)
        {
            for (int l = 0; l < numPredDir; l++)
            {
                int ref = interMode.bestME[l].ref;
                uint32_t bits = m_listSelBits[l] + MVP_IDX_BITS;
                bits += getTUBits(ref, numRefIdx[l]);

                cu.fillMvpCand(puIdx, m_puAbsPartIdx, l, ref, interMode.amvpCand[l][ref], mvc);

                // Pick the best possible MVP from AMVP candidates based on least residual
                uint32_t bestCost = MAX_INT;
                int mvpIdx = 0;
                int merange = m_param->searchRange;

                for (int i = 0; i < AMVP_NUM_CANDS; i++)
                {
                    MV mvCand = interMode.amvpCand[l][ref][i];

                    // NOTE: skip mvCand if Y is > merange and -FN>1
                    if (m_bFrameParallel && (mvCand.y >= (merange + 1) * 4))
                        continue;

                    cu.clipMv(mvCand);
                    predInterLumaPixel(tmpPredYuv, *slice->m_refPicList[l][ref]->m_reconPic, mvCand);
                    uint32_t cost = m_me.bufSAD(tmpPredYuv.getLumaAddr(m_puAbsPartIdx), tmpPredYuv.m_size);

                    if (bestCost > cost)
                    {
                        bestCost = cost;
                        mvpIdx  = i;
                    }
                }

                MV mvmin, mvmax, outmv, mvp = interMode.amvpCand[l][ref][mvpIdx];
                m_me.setMVP(mvp);
                MV bmv(interMode.bestME[l].mv.x, interMode.bestME[l].mv.y);

                int satdCost;
                if (interMode.bestME[l].costZero)
                    satdCost = m_me.mvcost(bmv);
                else
                    satdCost = interMode.bestME[l].cost;

                /* Get total cost of partition, but only include MV bit cost once */
                bits += m_me.bitcost(bmv);
                uint32_t cost = (satdCost - m_me.mvcost(bmv)) + m_rdCost.getCost(bits);

                /* Refine MVP selection, updates: mvp, mvpIdx, bits, cost */
                checkBestMVP(interMode.amvpCand[l][ref], outmv, mvp, mvpIdx, bits, cost);

                if (cost < interMode.bestME[l].cost)
                {
                    interMode.bestME[l].mv = outmv;
                    interMode.bestME[l].mvp = mvp;
                    interMode.bestME[l].mvpIdx = mvpIdx;
                    interMode.bestME[l].ref = ref;
                    interMode.bestME[l].cost = cost;
                    interMode.bestME[l].bits = bits;
                }
            }
        }
        else if (bDistributed)
        {
            m_curInterMode = &interMode;
            m_curGeom = &cuGeom;

            /* this worker might already be enqueued for pmode, so other threads
             * might be looking at the ME job counts at any time, do these sets
             * in a safe order */
            m_curPart = puIdx;
            m_totalNumME = 0;
            m_numAcquiredME = 1;
            m_numCompletedME = 0;
            m_totalNumME = numRefIdx[0] + numRefIdx[1];

            if (!m_bJobsQueued)
                JobProvider::enqueue();

            for (int i = 1; i < m_totalNumME; i++)
                m_pool->pokeIdleThread();

            while (m_totalNumME > m_numAcquiredME)
            {
                int id = ATOMIC_INC(&m_numAcquiredME);
                if (m_totalNumME >= id)
                {
                    id -= 1;
                    if (id < numRefIdx[0])
                        singleMotionEstimation(*this, interMode, cuGeom, puIdx, 0, id);
                    else
                        singleMotionEstimation(*this, interMode, cuGeom, puIdx, 1, id - numRefIdx[0]);

                    if (ATOMIC_INC(&m_numCompletedME) == m_totalNumME)
                        m_meCompletionEvent.trigger();
                }
            }
            if (!m_bJobsQueued)
                JobProvider::dequeue();

            /* we saved L0-0 for ourselves */
            singleMotionEstimation(*this, interMode, cuGeom, puIdx, 0, 0);
            if (ATOMIC_INC(&m_numCompletedME) == m_totalNumME)
                m_meCompletionEvent.trigger();

            m_meCompletionEvent.wait();
        }
        else
        {
            for (int l = 0; l < numPredDir; l++)
            {
                for (int ref = 0; ref < numRefIdx[l]; ref++)
                {
                    uint32_t bits = m_listSelBits[l] + MVP_IDX_BITS;
                    bits += getTUBits(ref, numRefIdx[l]);

                    int numMvc = cu.fillMvpCand(puIdx, m_puAbsPartIdx, l, ref, interMode.amvpCand[l][ref], mvc);

                    // Pick the best possible MVP from AMVP candidates based on least residual
                    uint32_t bestCost = MAX_INT;
                    int mvpIdx = 0;
                    int merange = m_param->searchRange;

                    for (int i = 0; i < AMVP_NUM_CANDS; i++)
                    {
                        MV mvCand = interMode.amvpCand[l][ref][i];

                        // NOTE: skip mvCand if Y is > merange and -FN>1
                        if (m_bFrameParallel && (mvCand.y >= (merange + 1) * 4))
                            continue;

                        cu.clipMv(mvCand);
                        predInterLumaPixel(tmpPredYuv, *slice->m_refPicList[l][ref]->m_reconPic, mvCand);
                        uint32_t cost = m_me.bufSAD(tmpPredYuv.getLumaAddr(m_puAbsPartIdx), tmpPredYuv.m_size);

                        if (bestCost > cost)
                        {
                            bestCost = cost;
                            mvpIdx  = i;
                        }
                    }

                    MV mvmin, mvmax, outmv, mvp = interMode.amvpCand[l][ref][mvpIdx];

                    setSearchRange(cu, mvp, merange, mvmin, mvmax);
                    int satdCost = m_me.motionEstimate(&slice->m_mref[l][ref], mvmin, mvmax, mvp, numMvc, mvc, merange, outmv);

                    /* Get total cost of partition, but only include MV bit cost once */
                    bits += m_me.bitcost(outmv);
                    uint32_t cost = (satdCost - m_me.mvcost(outmv)) + m_rdCost.getCost(bits);

                    /* Refine MVP selection, updates: mvp, mvpIdx, bits, cost */
                    checkBestMVP(interMode.amvpCand[l][ref], outmv, mvp, mvpIdx, bits, cost);

                    if (cost < interMode.bestME[l].cost)
                    {
                        interMode.bestME[l].mv = outmv;
                        interMode.bestME[l].mvp = mvp;
                        interMode.bestME[l].mvpIdx = mvpIdx;
                        interMode.bestME[l].ref = ref;
                        interMode.bestME[l].cost = cost;
                        interMode.bestME[l].bits = bits;
                    }
                }
            }
        }

        /* Bi-directional prediction */
        if (slice->isInterB() && !cu.isBipredRestriction() && interMode.bestME[0].cost != MAX_UINT && interMode.bestME[1].cost != MAX_UINT)
        {
            bidir[0] = interMode.bestME[0];
            bidir[1] = interMode.bestME[1];

            /* Generate reference subpels */
            PicYuv* refPic0  = slice->m_refPicList[0][interMode.bestME[0].ref]->m_reconPic;
            PicYuv* refPic1  = slice->m_refPicList[1][interMode.bestME[1].ref]->m_reconPic;
            Yuv*    bidirYuv = m_rqt[cuGeom.depth].bidirPredYuv;
            predInterLumaPixel(bidirYuv[0], *refPic0, interMode.bestME[0].mv);
            predInterLumaPixel(bidirYuv[1], *refPic1, interMode.bestME[1].mv);

            pixel *pred0 = bidirYuv[0].getLumaAddr(m_puAbsPartIdx);
            pixel *pred1 = bidirYuv[1].getLumaAddr(m_puAbsPartIdx);

            int partEnum = partitionFromSizes(m_puWidth, m_puHeight);
            primitives.pixelavg_pp[partEnum](tmpPredYuv.m_buf[0], tmpPredYuv.m_size, pred0, bidirYuv[0].m_size, pred1, bidirYuv[1].m_size, 32);
            int satdCost = m_me.bufSATD(tmpPredYuv.m_buf[0], tmpPredYuv.m_size);

            bidirBits = interMode.bestME[0].bits + interMode.bestME[1].bits + m_listSelBits[2] - (m_listSelBits[0] + m_listSelBits[1]);
            bidirCost = satdCost + m_rdCost.getCost(bidirBits);

            bool bTryZero = interMode.bestME[0].mv.notZero() || interMode.bestME[1].mv.notZero();
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

                bTryZero &= interMode.bestME[0].mvp.checkRange(mvmin, mvmax);
                bTryZero &= interMode.bestME[1].mvp.checkRange(mvmin, mvmax);
            }
            if (bTryZero)
            {
                /* coincident blocks of the two reference pictures */
                pixel *ref0 = m_slice->m_mref[0][interMode.bestME[0].ref].getLumaAddr(cu.m_cuAddr, cuGeom.encodeIdx + m_puAbsPartIdx);
                pixel *ref1 = m_slice->m_mref[1][interMode.bestME[1].ref].getLumaAddr(cu.m_cuAddr, cuGeom.encodeIdx + m_puAbsPartIdx);
                intptr_t refStride = slice->m_mref[0][0].lumaStride;

                primitives.pixelavg_pp[partEnum](tmpPredYuv.m_buf[0], tmpPredYuv.m_size, ref0, refStride, ref1, refStride, 32);
                satdCost = m_me.bufSATD(tmpPredYuv.m_buf[0], tmpPredYuv.m_size);

                MV mvp0 = interMode.bestME[0].mvp;
                int mvpIdx0 = interMode.bestME[0].mvpIdx;
                uint32_t bits0 = interMode.bestME[0].bits - m_me.bitcost(interMode.bestME[0].mv, mvp0) + m_me.bitcost(mvzero, mvp0);

                MV mvp1 = interMode.bestME[1].mvp;
                int mvpIdx1 = interMode.bestME[1].mvpIdx;
                uint32_t bits1 = interMode.bestME[1].bits - m_me.bitcost(interMode.bestME[1].mv, mvp1) + m_me.bitcost(mvzero, mvp1);

                uint32_t cost = satdCost + m_rdCost.getCost(bits0) + m_rdCost.getCost(bits1);

                /* refine MVP selection for zero mv, updates: mvp, mvpidx, bits, cost */
                checkBestMVP(interMode.amvpCand[0][interMode.bestME[0].ref], mvzero, mvp0, mvpIdx0, bits0, cost);
                checkBestMVP(interMode.amvpCand[1][interMode.bestME[1].ref], mvzero, mvp1, mvpIdx1, bits1, cost);

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

            /* Ugly hack - since BIDIR is not yet an RD decision, add a penalty
             * if psy-rd is enabled */
            if (m_rdCost.m_psyRd)
                bidirCost += (m_rdCost.m_psyRd * bidirCost) >> 8;
        }

        /* select best option and store into CU */
        if (mrgCost < bidirCost && mrgCost < interMode.bestME[0].cost && mrgCost < interMode.bestME[1].cost)
        {
            cu.m_mergeFlag[m_puAbsPartIdx] = true;
            cu.m_mvpIdx[0][m_puAbsPartIdx] = merge.index; // merge candidate ID is stored in L0 MVP idx
            cu.setPUInterDir(merge.interDir, m_puAbsPartIdx, puIdx);
            cu.setPUMv(0, merge.mvField[0].mv, m_puAbsPartIdx, puIdx);
            cu.setPURefIdx(0, merge.mvField[0].refIdx, m_puAbsPartIdx, puIdx);
            cu.setPUMv(1, merge.mvField[1].mv, m_puAbsPartIdx, puIdx);
            cu.setPURefIdx(1, merge.mvField[1].refIdx, m_puAbsPartIdx, puIdx);

            totalmebits += merge.bits;
        }
        else if (bidirCost < interMode.bestME[0].cost && bidirCost < interMode.bestME[1].cost)
        {
            lastMode = 2;

            cu.m_mergeFlag[m_puAbsPartIdx] = false;
            cu.setPUInterDir(3, m_puAbsPartIdx, puIdx);
            cu.setPUMv(0, bidir[0].mv, m_puAbsPartIdx, puIdx);
            cu.setPURefIdx(0, interMode.bestME[0].ref, m_puAbsPartIdx, puIdx);
            cu.m_mvd[0][m_puAbsPartIdx] = bidir[0].mv - bidir[0].mvp;
            cu.m_mvpIdx[0][m_puAbsPartIdx] = bidir[0].mvpIdx;

            cu.setPUMv(1, bidir[1].mv, m_puAbsPartIdx, puIdx);
            cu.setPURefIdx(1, interMode.bestME[1].ref, m_puAbsPartIdx, puIdx);
            cu.m_mvd[1][m_puAbsPartIdx] = bidir[1].mv - bidir[1].mvp;
            cu.m_mvpIdx[1][m_puAbsPartIdx] = bidir[1].mvpIdx;

            totalmebits += bidirBits;
        }
        else if (interMode.bestME[0].cost <= interMode.bestME[1].cost)
        {
            lastMode = 0;

            cu.m_mergeFlag[m_puAbsPartIdx] = false;
            cu.setPUInterDir(1, m_puAbsPartIdx, puIdx);
            cu.setPUMv(0, interMode.bestME[0].mv, m_puAbsPartIdx, puIdx);
            cu.setPURefIdx(0, interMode.bestME[0].ref, m_puAbsPartIdx, puIdx);
            cu.m_mvd[0][m_puAbsPartIdx] = interMode.bestME[0].mv - interMode.bestME[0].mvp;
            cu.m_mvpIdx[0][m_puAbsPartIdx] = interMode.bestME[0].mvpIdx;

            cu.setPURefIdx(1, REF_NOT_VALID, m_puAbsPartIdx, puIdx);
            cu.setPUMv(1, mvzero, m_puAbsPartIdx, puIdx);

            totalmebits += interMode.bestME[0].bits;
        }
        else
        {
            lastMode = 1;

            cu.m_mergeFlag[m_puAbsPartIdx] = false;
            cu.setPUInterDir(2, m_puAbsPartIdx, puIdx);
            cu.setPUMv(1, interMode.bestME[1].mv, m_puAbsPartIdx, puIdx);
            cu.setPURefIdx(1, interMode.bestME[1].ref, m_puAbsPartIdx, puIdx);
            cu.m_mvd[1][m_puAbsPartIdx] = interMode.bestME[1].mv - interMode.bestME[1].mvp;
            cu.m_mvpIdx[1][m_puAbsPartIdx] = interMode.bestME[1].mvpIdx;

            cu.setPURefIdx(0, REF_NOT_VALID, m_puAbsPartIdx, puIdx);
            cu.setPUMv(0, mvzero, m_puAbsPartIdx, puIdx);

            totalmebits += interMode.bestME[1].bits;
        }

        prepMotionCompensation(cu, cuGeom, puIdx);
        motionCompensation(*predYuv, true, bChroma);
    }

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

    uint32_t cuSize = 1 << cu.m_log2CUSize[0];
    uint32_t depth  = cu.m_cuDepth[0];

    // No residual coding : SKIP mode

    cu.setPredModeSubParts(MODE_SKIP);
    cu.clearCbf();
    cu.setTUDepthSubParts(0, 0, depth);

    reconYuv->copyFromYuv(interMode.predYuv);

    // Luma
    int part = partitionFromLog2Size(cu.m_log2CUSize[0]);
    interMode.distortion = primitives.sse_pp[part](fencYuv->m_buf[0], fencYuv->m_size, reconYuv->m_buf[0], reconYuv->m_size);
    // Chroma
    part = partitionFromSizes(cuSize >> m_hChromaShift, cuSize >> m_vChromaShift);
    interMode.distortion += m_rdCost.scaleChromaDistCb(primitives.sse_pp[part](fencYuv->m_buf[1], fencYuv->m_csize, reconYuv->m_buf[1], reconYuv->m_csize));
    interMode.distortion += m_rdCost.scaleChromaDistCr(primitives.sse_pp[part](fencYuv->m_buf[2], fencYuv->m_csize, reconYuv->m_buf[2], reconYuv->m_csize));

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
        interMode.psyEnergy = m_rdCost.psyCost(cu.m_log2CUSize[0] - 2, fencYuv->m_buf[0], fencYuv->m_size, reconYuv->m_buf[0], reconYuv->m_size);

    updateModeCost(interMode);
    m_entropyCoder.store(interMode.contexts);
}

/* encode residual and calculate rate-distortion for a CU block.
 * Note: this function overwrites the RD cost variables of interMode, but leaves the sa8d cost unharmed */
void Search::encodeResAndCalcRdInterCU(Mode& interMode, const CUGeom& cuGeom)
{
    CUData& cu = interMode.cu;
    Yuv* reconYuv = &interMode.reconYuv;
    Yuv* predYuv = &interMode.predYuv;
    ShortYuv* resiYuv = &m_rqt[cuGeom.depth].tmpResiYuv;
    const Yuv* fencYuv = interMode.fencYuv;

    X265_CHECK(!cu.isIntra(0), "intra CU not expected\n");

    uint32_t log2CUSize = cu.m_log2CUSize[0];
    uint32_t cuSize = 1 << log2CUSize;
    uint32_t depth  = cu.m_cuDepth[0];

    int part = partitionFromLog2Size(log2CUSize);
    int cpart = partitionFromSizes(cuSize >> m_hChromaShift, cuSize >> m_vChromaShift);

    m_quant.setQPforQuant(interMode.cu);

    resiYuv->subtract(*fencYuv, *predYuv, log2CUSize);

    uint32_t tuDepthRange[2];
    cu.getInterTUQtDepthRange(tuDepthRange, 0);

    m_entropyCoder.load(m_rqt[depth].cur);

    Cost costs;
    estimateResidualQT(interMode, cuGeom, 0, depth, *resiYuv, costs, tuDepthRange);

    if (!cu.m_tqBypass[0])
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

        if (cbf0Cost < costs.rdcost)
        {
            cu.clearCbf();
            cu.setTUDepthSubParts(0, 0, depth);
        }
    }

    if (cu.getQtRootCbf(0))
        saveResidualQTData(cu, *resiYuv, 0, depth);

    /* calculate signal bits for inter/merge/skip coded CU */
    m_entropyCoder.load(m_rqt[depth].cur);

    uint32_t coeffBits, bits;
    if (cu.m_mergeFlag[0] && cu.m_partSize[0] == SIZE_2Nx2N && !cu.getQtRootCbf(0))
    {
        cu.setPredModeSubParts(MODE_SKIP);

        /* Merge/Skip */
        m_entropyCoder.resetBits();
        if (m_slice->m_pps->bTransquantBypassEnabled)
            m_entropyCoder.codeCUTransquantBypassFlag(cu.m_tqBypass[0]);
        m_entropyCoder.codeSkipFlag(cu, 0);
        m_entropyCoder.codeMergeIndex(cu, 0);
        coeffBits = 0;
        bits = m_entropyCoder.getNumberOfWrittenBits();
    }
    else
    {
        m_entropyCoder.resetBits();
        if (m_slice->m_pps->bTransquantBypassEnabled)
            m_entropyCoder.codeCUTransquantBypassFlag(cu.m_tqBypass[0]);
        m_entropyCoder.codeSkipFlag(cu, 0);
        m_entropyCoder.codePredMode(cu.m_predMode[0]);
        m_entropyCoder.codePartSize(cu, 0, cu.m_cuDepth[0]);
        m_entropyCoder.codePredInfo(cu, 0);
        uint32_t mvBits = m_entropyCoder.getNumberOfWrittenBits();

        bool bCodeDQP = m_slice->m_pps->bUseDQP;
        m_entropyCoder.codeCoeff(cu, 0, cu.m_cuDepth[0], bCodeDQP, tuDepthRange);
        bits = m_entropyCoder.getNumberOfWrittenBits();

        coeffBits = bits - mvBits;
    }

    m_entropyCoder.store(interMode.contexts);

    if (cu.getQtRootCbf(0))
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

void Search::residualTransformQuantInter(Mode& mode, const CUGeom& cuGeom, uint32_t absPartIdx, uint32_t depth, uint32_t depthRange[2])
{
    CUData& cu = mode.cu;
    X265_CHECK(cu.m_cuDepth[0] == cu.m_cuDepth[absPartIdx], "invalid depth\n");

    uint32_t log2TrSize = g_maxLog2CUSize - depth;
    uint32_t tuDepth = depth - cu.m_cuDepth[0];

    bool bCheckFull = log2TrSize <= depthRange[1];
    if (cu.m_partSize[0] != SIZE_2Nx2N && depth == cu.m_cuDepth[absPartIdx] && log2TrSize > depthRange[0])
        bCheckFull = false;

    if (bCheckFull)
    {
        // code full block
        uint32_t log2TrSizeC = log2TrSize - m_hChromaShift;
        bool bCodeChroma = true;
        uint32_t tuDepthC = tuDepth;
        if (log2TrSizeC == 1)
        {
            X265_CHECK(log2TrSize == 2 && m_csp != X265_CSP_I444, "tuQuad check failed\n");
            log2TrSizeC++;
            tuDepthC--;
            uint32_t qpdiv = NUM_CU_PARTITIONS >> ((depth - 1) << 1);
            bCodeChroma = ((absPartIdx & (qpdiv - 1)) == 0);
        }

        uint32_t absPartIdxStep = NUM_CU_PARTITIONS >> ((cu.m_cuDepth[0] + tuDepthC) << 1);
        uint32_t setCbf = 1 << tuDepth;

        uint32_t coeffOffsetY = absPartIdx << (LOG2_UNIT_SIZE * 2);
        coeff_t *coeffCurY = cu.m_trCoeff[0] + coeffOffsetY;

        uint32_t sizeIdx  = log2TrSize  - 2;

        cu.setTUDepthSubParts(depth - cu.m_cuDepth[0], absPartIdx, depth);
        cu.setTransformSkipSubParts(0, TEXT_LUMA, absPartIdx, depth);

        ShortYuv& resiYuv = m_rqt[cuGeom.depth].tmpResiYuv;
        const Yuv* fencYuv = mode.fencYuv;

        int16_t *curResiY = resiYuv.getLumaAddr(absPartIdx);
        uint32_t strideResiY = resiYuv.m_size;

        pixel *fenc = const_cast<pixel*>(fencYuv->getLumaAddr(absPartIdx));
        uint32_t numSigY = m_quant.transformNxN(cu, fenc, fencYuv->m_size, curResiY, strideResiY, coeffCurY, log2TrSize, TEXT_LUMA, absPartIdx, false);

        if (numSigY)
        {
            m_quant.invtransformNxN(cu.m_tqBypass[absPartIdx], curResiY, strideResiY, coeffCurY, log2TrSize, TEXT_LUMA, false, false, numSigY);
            cu.setCbfSubParts(setCbf, TEXT_LUMA, absPartIdx, depth);
        }
        else
        {
            primitives.blockfill_s[sizeIdx](curResiY, strideResiY, 0);
            cu.setCbfSubParts(0, TEXT_LUMA, absPartIdx, depth);
        }

        if (bCodeChroma)
        {
            uint32_t sizeIdxC = log2TrSizeC - 2;
            uint32_t strideResiC = resiYuv.m_csize;

            uint32_t coeffOffsetC = coeffOffsetY >> (m_hChromaShift + m_vChromaShift);
            coeff_t *coeffCurU = cu.m_trCoeff[1] + coeffOffsetC;
            coeff_t *coeffCurV = cu.m_trCoeff[2] + coeffOffsetC;
            bool splitIntoSubTUs = (m_csp == X265_CSP_I422);

            TURecurse tuIterator(splitIntoSubTUs ? VERTICAL_SPLIT : DONT_SPLIT, absPartIdxStep, absPartIdx);
            do
            {
                uint32_t absPartIdxC = tuIterator.absPartIdxTURelCU;
                uint32_t subTUOffset = tuIterator.section << (log2TrSizeC * 2);

                cu.setTransformSkipPartRange(0, TEXT_CHROMA_U, absPartIdxC, tuIterator.absPartIdxStep);
                cu.setTransformSkipPartRange(0, TEXT_CHROMA_V, absPartIdxC, tuIterator.absPartIdxStep);

                int16_t* curResiU = resiYuv.getCbAddr(absPartIdxC);
                pixel*   fencCb = const_cast<pixel*>(fencYuv->getCbAddr(absPartIdxC));
                uint32_t numSigU = m_quant.transformNxN(cu, fencCb, fencYuv->m_csize, curResiU, strideResiC, coeffCurU + subTUOffset, log2TrSizeC, TEXT_CHROMA_U, absPartIdxC, false);
                if (numSigU)
                {
                    m_quant.invtransformNxN(cu.m_tqBypass[absPartIdxC], curResiU, strideResiC, coeffCurU + subTUOffset, log2TrSizeC, TEXT_CHROMA_U, false, false, numSigU);
                    cu.setCbfPartRange(setCbf, TEXT_CHROMA_U, absPartIdxC, tuIterator.absPartIdxStep);
                }
                else
                {
                    primitives.blockfill_s[sizeIdxC](curResiU, strideResiC, 0);
                    cu.setCbfPartRange(0, TEXT_CHROMA_U, absPartIdxC, tuIterator.absPartIdxStep);
                }

                int16_t* curResiV = resiYuv.getCrAddr(absPartIdxC);
                pixel*   fencCr = const_cast<pixel*>(fencYuv->getCrAddr(absPartIdxC));
                uint32_t numSigV = m_quant.transformNxN(cu, fencCr, fencYuv->m_csize, curResiV, strideResiC, coeffCurV + subTUOffset, log2TrSizeC, TEXT_CHROMA_V, absPartIdxC, false);
                if (numSigV)
                {
                    m_quant.invtransformNxN(cu.m_tqBypass[absPartIdxC], curResiV, strideResiC, coeffCurV + subTUOffset, log2TrSizeC, TEXT_CHROMA_V, false, false, numSigV);
                    cu.setCbfPartRange(setCbf, TEXT_CHROMA_V, absPartIdxC, tuIterator.absPartIdxStep);
                }
                else
                {
                    primitives.blockfill_s[sizeIdxC](curResiV, strideResiC, 0);
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

        const uint32_t qPartNumSubdiv = NUM_CU_PARTITIONS >> ((depth + 1) << 1);
        uint32_t ycbf = 0, ucbf = 0, vcbf = 0;
        for (uint32_t i = 0; i < 4; i++)
        {
            residualTransformQuantInter(mode, cuGeom, absPartIdx + i * qPartNumSubdiv, depth + 1, depthRange);
            ycbf |= cu.getCbf(absPartIdx + i * qPartNumSubdiv, TEXT_LUMA, tuDepth + 1);
            ucbf |= cu.getCbf(absPartIdx + i * qPartNumSubdiv, TEXT_CHROMA_U, tuDepth + 1);
            vcbf |= cu.getCbf(absPartIdx + i * qPartNumSubdiv, TEXT_CHROMA_V, tuDepth + 1);
        }
        for (uint32_t i = 0; i < 4 * qPartNumSubdiv; i++)
        {
            cu.m_cbf[TEXT_LUMA][absPartIdx + i] |= ycbf << tuDepth;
            cu.m_cbf[TEXT_CHROMA_U][absPartIdx + i] |= ucbf << tuDepth;
            cu.m_cbf[TEXT_CHROMA_V][absPartIdx + i] |= vcbf << tuDepth;
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

void Search::estimateResidualQT(Mode& mode, const CUGeom& cuGeom, uint32_t absPartIdx, uint32_t depth, ShortYuv& resiYuv, Cost& outCosts, uint32_t depthRange[2])
{
    CUData& cu = mode.cu;
    uint32_t log2TrSize = g_maxLog2CUSize - depth;

    bool bCheckSplit = log2TrSize > depthRange[0];
    bool bCheckFull = log2TrSize <= depthRange[1];
    bool bSplitPresentFlag = bCheckSplit && bCheckFull;

    if (cu.m_partSize[0] != SIZE_2Nx2N && depth == cu.m_cuDepth[absPartIdx] && bCheckSplit)
        bCheckFull = false;

    X265_CHECK(bCheckFull || bCheckSplit, "check-full or check-split must be set\n");
    X265_CHECK(cu.m_cuDepth[0] == cu.m_cuDepth[absPartIdx], "depth not matching\n");

    uint32_t tuDepth = depth - cu.m_cuDepth[0];
    uint32_t log2TrSizeC = log2TrSize - m_hChromaShift;
    bool bCodeChroma = true;
    uint32_t tuDepthC = tuDepth;
    if ((log2TrSize == 2) && !(m_csp == X265_CSP_I444))
    {
        log2TrSizeC++;
        tuDepthC--;
        uint32_t qpdiv = NUM_CU_PARTITIONS >> ((depth - 1) << 1);
        bCodeChroma = ((absPartIdx & (qpdiv - 1)) == 0);
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
    uint32_t absPartIdxStep = NUM_CU_PARTITIONS >> ((cu.m_cuDepth[0] +  tuDepthC) << 1);
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

        cu.setTUDepthSubParts(depth - cu.m_cuDepth[0], absPartIdx, depth);
        cu.setTransformSkipSubParts(0, TEXT_LUMA, absPartIdx, depth);

        if (m_bEnableRDOQ)
            m_entropyCoder.estBit(m_entropyCoder.m_estBitsSbac, log2TrSize, true);

        pixel *fenc = const_cast<pixel*>(fencYuv->getLumaAddr(absPartIdx));
        int16_t *resi = resiYuv.getLumaAddr(absPartIdx);
        numSig[TEXT_LUMA][0] = m_quant.transformNxN(cu, fenc, fencYuv->m_size, resi, resiYuv.m_size, coeffCurY, log2TrSize, TEXT_LUMA, absPartIdx, false);
        cbfFlag[TEXT_LUMA][0] = !!numSig[TEXT_LUMA][0];

        m_entropyCoder.resetBits();

        if (bSplitPresentFlag && log2TrSize > depthRange[0])
            m_entropyCoder.codeTransformSubdivFlag(0, 5 - log2TrSize);
        fullCost.bits = m_entropyCoder.getNumberOfWrittenBits();

        // Coding luma cbf flag has been removed from here. The context for cbf flag is different for each depth.
        // So it is valid if we encode coefficients and then cbfs at least for analysis.
//        m_entropyCoder.codeQtCbf(cbfFlag[TEXT_LUMA][0], TEXT_LUMA, tuDepth);
        if (cbfFlag[TEXT_LUMA][0])
            m_entropyCoder.codeCoeffNxN(cu, coeffCurY, absPartIdx, log2TrSize, TEXT_LUMA);

        uint32_t singleBitsPrev = m_entropyCoder.getNumberOfWrittenBits();
        singleBits[TEXT_LUMA][0] = singleBitsPrev - fullCost.bits;

        X265_CHECK(log2TrSize <= 5, "log2TrSize is too large\n");
        uint32_t distY = primitives.ssd_s[partSize](resiYuv.getLumaAddr(absPartIdx), resiYuv.m_size);
        uint32_t psyEnergyY = 0;
        if (m_rdCost.m_psyRd)
            psyEnergyY = m_rdCost.psyCost(partSize, resiYuv.getLumaAddr(absPartIdx), resiYuv.m_size, (int16_t*)zeroShort, 0);

        int16_t *curResiY    = m_rqt[qtLayer].resiQtYuv.getLumaAddr(absPartIdx);
        uint32_t strideResiY = m_rqt[qtLayer].resiQtYuv.m_size;

        if (cbfFlag[TEXT_LUMA][0])
        {
            m_quant.invtransformNxN(cu.m_tqBypass[absPartIdx], curResiY, strideResiY, coeffCurY, log2TrSize, TEXT_LUMA, false, false, numSig[TEXT_LUMA][0]); //this is for inter mode only

            // non-zero cost calculation for luma - This is an approximation
            // finally we have to encode correct cbf after comparing with null cost
            const uint32_t nonZeroDistY = primitives.sse_ss[partSize](resiYuv.getLumaAddr(absPartIdx), resiYuv.m_size, curResiY, strideResiY);
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
                    primitives.blockfill_s[partSize](curResiY, strideResiY, 0);
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
            primitives.blockfill_s[partSize](curResiY, strideResiY, 0);
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

                    fenc = const_cast<pixel*>(fencYuv->getChromaAddr(chromaId, absPartIdxC));
                    resi = resiYuv.getChromaAddr(chromaId, absPartIdxC);
                    numSig[chromaId][tuIterator.section] = m_quant.transformNxN(cu, fenc, fencYuv->m_csize, resi, resiYuv.m_csize, coeffCurC + subTUOffset, log2TrSizeC, (TextType)chromaId, absPartIdxC, false);
                    cbfFlag[chromaId][tuIterator.section] = !!numSig[chromaId][tuIterator.section];

                    //Coding cbf flags has been removed from here
//                    m_entropyCoder.codeQtCbf(cbfFlag[chromaId][tuIterator.section], (TextType)chromaId, tuDepth);
                    if (cbfFlag[chromaId][tuIterator.section])
                        m_entropyCoder.codeCoeffNxN(cu, coeffCurC + subTUOffset, absPartIdxC, log2TrSizeC, (TextType)chromaId);
                    uint32_t newBits = m_entropyCoder.getNumberOfWrittenBits();
                    singleBits[chromaId][tuIterator.section] = newBits - singleBitsPrev;
                    singleBitsPrev = newBits;

                    int16_t *curResiC = m_rqt[qtLayer].resiQtYuv.getChromaAddr(chromaId, absPartIdxC);
                    distC = m_rdCost.scaleChromaDistCb(primitives.ssd_s[log2TrSizeC - 2](resiYuv.getChromaAddr(chromaId, absPartIdxC), resiYuv.m_csize));

                    if (cbfFlag[chromaId][tuIterator.section])
                    {
                        m_quant.invtransformNxN(cu.m_tqBypass[absPartIdxC], curResiC, strideResiC, coeffCurC + subTUOffset,
                                                log2TrSizeC, (TextType)chromaId, false, false, numSig[chromaId][tuIterator.section]);

                        // non-zero cost calculation for luma, same as luma - This is an approximation
                        // finally we have to encode correct cbf after comparing with null cost
                        uint32_t dist = primitives.sse_ss[partSizeC](resiYuv.getChromaAddr(chromaId, absPartIdxC), resiYuv.m_csize, curResiC, strideResiC);
                        uint32_t nzCbfBitsC = m_entropyCoder.estimateCbfBits(cbfFlag[chromaId][tuIterator.section], (TextType)chromaId, tuDepth);
                        uint32_t nonZeroDistC = m_rdCost.scaleChromaDistCb(dist);
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
                                primitives.blockfill_s[partSizeC](curResiC, strideResiC, 0);
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
                        primitives.blockfill_s[partSizeC](curResiC, strideResiC, 0);
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

            ALIGN_VAR_32(coeff_t, tsCoeffY[MAX_TS_SIZE * MAX_TS_SIZE]);
            ALIGN_VAR_32(int16_t, tsResiY[MAX_TS_SIZE * MAX_TS_SIZE]);

            m_entropyCoder.load(m_rqt[depth].rqtRoot);

            cu.setTransformSkipSubParts(1, TEXT_LUMA, absPartIdx, depth);

            if (m_bEnableRDOQ)
                m_entropyCoder.estBit(m_entropyCoder.m_estBitsSbac, log2TrSize, true);

            fenc = const_cast<pixel*>(fencYuv->getLumaAddr(absPartIdx));
            resi = resiYuv.getLumaAddr(absPartIdx);
            uint32_t numSigTSkipY = m_quant.transformNxN(cu, fenc, fencYuv->m_size, resi, resiYuv.m_size, tsCoeffY, log2TrSize, TEXT_LUMA, absPartIdx, true);

            if (numSigTSkipY)
            {
                m_entropyCoder.resetBits();
                m_entropyCoder.codeQtCbf(!!numSigTSkipY, TEXT_LUMA, tuDepth);
                m_entropyCoder.codeCoeffNxN(cu, tsCoeffY, absPartIdx, log2TrSize, TEXT_LUMA);
                const uint32_t skipSingleBitsY = m_entropyCoder.getNumberOfWrittenBits();

                m_quant.invtransformNxN(cu.m_tqBypass[absPartIdx], tsResiY, trSize, tsCoeffY, log2TrSize, TEXT_LUMA, false, true, numSigTSkipY);

                nonZeroDistY = primitives.sse_ss[partSize](resiYuv.getLumaAddr(absPartIdx), resiYuv.m_size, tsResiY, trSize);

                if (m_rdCost.m_psyRd)
                {
                    nonZeroPsyEnergyY = m_rdCost.psyCost(partSize, resiYuv.getLumaAddr(absPartIdx), resiYuv.m_size, tsResiY, trSize);
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
                memcpy(coeffCurY, tsCoeffY, sizeof(coeff_t) * numCoeffY);
                primitives.square_copy_ss[partSize](curResiY, strideResiY, tsResiY, trSize);
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

                    int16_t *curResiC = m_rqt[qtLayer].resiQtYuv.getChromaAddr(chromaId, absPartIdxC);

                    ALIGN_VAR_32(coeff_t, tsCoeffC[MAX_TS_SIZE * MAX_TS_SIZE]);
                    ALIGN_VAR_32(int16_t, tsResiC[MAX_TS_SIZE * MAX_TS_SIZE]);

                    cu.setTransformSkipPartRange(1, (TextType)chromaId, absPartIdxC, tuIterator.absPartIdxStep);

                    if (m_bEnableRDOQ && (chromaId != TEXT_CHROMA_V))
                        m_entropyCoder.estBit(m_entropyCoder.m_estBitsSbac, log2TrSizeC, false);

                    fenc = const_cast<pixel*>(fencYuv->getChromaAddr(chromaId, absPartIdxC));
                    resi = resiYuv.getChromaAddr(chromaId, absPartIdxC);
                    uint32_t numSigTSkipC = m_quant.transformNxN(cu, fenc, fencYuv->m_csize, resi, resiYuv.m_csize, tsCoeffC, log2TrSizeC, (TextType)chromaId, absPartIdxC, true);

                    m_entropyCoder.resetBits();
                    singleBits[chromaId][tuIterator.section] = 0;

                    if (numSigTSkipC)
                    {
                        m_entropyCoder.codeQtCbf(!!numSigTSkipC, (TextType)chromaId, tuDepth);
                        m_entropyCoder.codeCoeffNxN(cu, tsCoeffC, absPartIdxC, log2TrSizeC, (TextType)chromaId);
                        singleBits[chromaId][tuIterator.section] = m_entropyCoder.getNumberOfWrittenBits();

                        m_quant.invtransformNxN(cu.m_tqBypass[absPartIdxC], tsResiC, trSizeC, tsCoeffC,
                                                log2TrSizeC, (TextType)chromaId, false, true, numSigTSkipC);
                        uint32_t dist = primitives.sse_ss[partSizeC](resiYuv.getChromaAddr(chromaId, absPartIdxC), resiYuv.m_csize, tsResiC, trSizeC);
                        nonZeroDistC = m_rdCost.scaleChromaDistCb(dist);
                        if (m_rdCost.m_psyRd)
                        {
                            nonZeroPsyEnergyC = m_rdCost.psyCost(partSizeC, resiYuv.getChromaAddr(chromaId, absPartIdxC), resiYuv.m_csize, tsResiC, trSizeC);
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
                        memcpy(coeffCurC + subTUOffset, tsCoeffC, sizeof(coeff_t) * numCoeffC);
                        primitives.square_copy_ss[partSizeC](curResiC, strideResiC, tsResiC, trSizeC);
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
            for (uint32_t chromaId = TEXT_CHROMA_U; chromaId <= TEXT_CHROMA_V; chromaId++)
            {
                if (!splitIntoSubTUs)
                    m_entropyCoder.codeQtCbf(cbfFlag[chromaId][0], (TextType)chromaId, tuDepth);
                else
                {
                    offsetSubTUCBFs(cu, (TextType)chromaId, tuDepth, absPartIdx);
                    for (uint32_t subTU = 0; subTU < 2; subTU++)
                        m_entropyCoder.codeQtCbf(cbfFlag[chromaId][subTU], (TextType)chromaId, tuDepth);
                }
            }
        }

        m_entropyCoder.codeQtCbf(cbfFlag[TEXT_LUMA][0], TEXT_LUMA, tuDepth);

        uint32_t cbfBits = m_entropyCoder.getNumberOfWrittenBits();

        uint32_t coeffBits = 0;
        coeffBits = singleBits[TEXT_LUMA][0];
        for (uint32_t subTUIndex = 0; subTUIndex < 2; subTUIndex++)
        {
            coeffBits += singleBits[TEXT_CHROMA_U][subTUIndex];
            coeffBits += singleBits[TEXT_CHROMA_V][subTUIndex];
        }

        // In split mode, we need only coeffBits. The reason is encoding chroma cbfs is different from luma.
        // In case of chroma, if any one of the splitted block's cbf is 1, then we need to encode cbf 1, and then for
        // four splitted block's individual cbf value. This is not known before analysis of four splitted blocks.
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
            // Subdiv flag can be encoded at the start of anlysis of splitted blocks.
            m_entropyCoder.resetBits();
            m_entropyCoder.codeTransformSubdivFlag(1, 5 - log2TrSize);
            splitCost.bits = m_entropyCoder.getNumberOfWrittenBits();
        }

        const uint32_t qPartNumSubdiv = NUM_CU_PARTITIONS >> ((depth + 1) << 1);
        uint32_t ycbf = 0, ucbf = 0, vcbf = 0;
        for (uint32_t i = 0; i < 4; ++i)
        {
            estimateResidualQT(mode, cuGeom, absPartIdx + i * qPartNumSubdiv, depth + 1, resiYuv, splitCost, depthRange);
            ycbf |= cu.getCbf(absPartIdx + i * qPartNumSubdiv, TEXT_LUMA,     tuDepth + 1);
            ucbf |= cu.getCbf(absPartIdx + i * qPartNumSubdiv, TEXT_CHROMA_U, tuDepth + 1);
            vcbf |= cu.getCbf(absPartIdx + i * qPartNumSubdiv, TEXT_CHROMA_V, tuDepth + 1);
        }
        for (uint32_t i = 0; i < 4 * qPartNumSubdiv; ++i)
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

        codeInterSubdivCbfQT(cu, absPartIdx, depth, depthRange);
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
            const uint32_t numberOfSections = splitIntoSubTUs ? 2 : 1;

            uint32_t partIdxesPerSubTU = absPartIdxStep >> (splitIntoSubTUs ? 1 : 0);
            for (uint32_t subTUIndex = 0; subTUIndex < numberOfSections; subTUIndex++)
            {
                const uint32_t  subTUPartIdx = absPartIdx + (subTUIndex * partIdxesPerSubTU);

                cu.setTransformSkipPartRange(bestTransformMode[TEXT_CHROMA_U][subTUIndex], TEXT_CHROMA_U, subTUPartIdx, partIdxesPerSubTU);
                cu.setTransformSkipPartRange(bestTransformMode[TEXT_CHROMA_V][subTUIndex], TEXT_CHROMA_V, subTUPartIdx, partIdxesPerSubTU);
            }
        }
        X265_CHECK(bCheckFull, "check-full must be set\n");
        m_entropyCoder.load(m_rqt[depth].rqtTest);
    }

    cu.setTUDepthSubParts(tuDepth, absPartIdx, depth);
    cu.setCbfSubParts(cbfFlag[TEXT_LUMA][0] << tuDepth, TEXT_LUMA, absPartIdx, depth);

    if (bCodeChroma)
    {
        uint32_t numberOfSections = splitIntoSubTUs ? 2 : 1;
        uint32_t partIdxesPerSubTU = absPartIdxStep >> (splitIntoSubTUs ? 1 : 0);

        for (uint32_t chromaId = TEXT_CHROMA_U; chromaId <= TEXT_CHROMA_V; chromaId++)
        {
            for (uint32_t subTUIndex = 0; subTUIndex < numberOfSections; subTUIndex++)
            {
                const uint32_t  subTUPartIdx = absPartIdx + (subTUIndex * partIdxesPerSubTU);

                if (splitIntoSubTUs)
                {
                    uint8_t combinedSubTUCBF = cbfFlag[chromaId][0] | cbfFlag[chromaId][1];
                    cu.setCbfPartRange(((cbfFlag[chromaId][subTUIndex] << 1) | combinedSubTUCBF) << tuDepth, (TextType)chromaId, subTUPartIdx, partIdxesPerSubTU);
                }
                else
                    cu.setCbfPartRange(cbfFlag[chromaId][subTUIndex] << tuDepth, (TextType)chromaId, subTUPartIdx, partIdxesPerSubTU);
            }
        }
    }

    outCosts.distortion += fullCost.distortion;
    outCosts.rdcost     += fullCost.rdcost;
    outCosts.bits       += fullCost.bits;
    outCosts.energy     += fullCost.energy;
}

void Search::codeInterSubdivCbfQT(CUData& cu, uint32_t absPartIdx, const uint32_t depth, uint32_t depthRange[2])
{
    X265_CHECK(cu.m_cuDepth[0] == cu.m_cuDepth[absPartIdx], "depth not matching\n");
    X265_CHECK(cu.isInter(absPartIdx), "codeInterSubdivCbfQT() with intra block\n");

    const uint32_t curTuDepth  = depth - cu.m_cuDepth[0];
    const uint32_t tuDepth     = cu.m_tuDepth[absPartIdx];
    const bool     bSubdiv     = curTuDepth != tuDepth;
    const uint32_t log2TrSize  = g_maxLog2CUSize - depth;

    const bool splitIntoSubTUs = (m_csp == X265_CSP_I422);
    uint32_t log2TrSizeC = log2TrSize - m_hChromaShift;
    uint32_t trWidthC  = 1 << log2TrSizeC;
    uint32_t trHeightC = splitIntoSubTUs ? (trWidthC << 1) : trWidthC;

    bool mCodeAll = true;
    const uint32_t numPels = trWidthC * trHeightC;
    if (numPels < (MIN_TU_SIZE * MIN_TU_SIZE))
        mCodeAll = false;

    if (mCodeAll)
    {
        uint32_t absPartIdxStep = NUM_CU_PARTITIONS >> ((cu.m_cuDepth[0] +  curTuDepth) << 1);
        if (!curTuDepth || cu.getCbf(absPartIdx, TEXT_CHROMA_U, curTuDepth - 1))
            m_entropyCoder.codeQtCbf(cu, absPartIdx, absPartIdxStep, trWidthC, trHeightC, TEXT_CHROMA_U, curTuDepth, !bSubdiv);
        if (!curTuDepth || cu.getCbf(absPartIdx, TEXT_CHROMA_V, curTuDepth - 1))
            m_entropyCoder.codeQtCbf(cu, absPartIdx, absPartIdxStep, trWidthC, trHeightC, TEXT_CHROMA_V, curTuDepth, !bSubdiv);
    }
    else
    {
        X265_CHECK(cu.getCbf(absPartIdx, TEXT_CHROMA_U, curTuDepth) == cu.getCbf(absPartIdx, TEXT_CHROMA_U, curTuDepth - 1), "chroma CBF not matching\n");
        X265_CHECK(cu.getCbf(absPartIdx, TEXT_CHROMA_V, curTuDepth) == cu.getCbf(absPartIdx, TEXT_CHROMA_V, curTuDepth - 1), "chroma CBF not matching\n");
    }

    if (!bSubdiv)
    {
        m_entropyCoder.codeQtCbf(cu, absPartIdx, TEXT_LUMA, tuDepth);
    }
    else
    {
        const uint32_t qpartNumSubdiv = NUM_CU_PARTITIONS >> ((depth + 1) << 1);
        for (uint32_t i = 0; i < 4; ++i)
            codeInterSubdivCbfQT(cu, absPartIdx + i * qpartNumSubdiv, depth + 1, depthRange);
    }
}

void Search::encodeResidualQT(CUData& cu, uint32_t absPartIdx, const uint32_t depth, TextType ttype, uint32_t depthRange[2])
{
    X265_CHECK(cu.m_cuDepth[0] == cu.m_cuDepth[absPartIdx], "depth not matching\n");
    X265_CHECK(cu.isInter(absPartIdx), "encodeResidualQT() with intra block\n");

    const uint32_t curTuDepth  = depth - cu.m_cuDepth[0];
    const uint32_t tuDepth     = cu.m_tuDepth[absPartIdx];
    const bool     bSubdiv     = curTuDepth != tuDepth;

    if (bSubdiv)
    {
        if (cu.getCbf(absPartIdx, ttype, curTuDepth))
        {
            const uint32_t qpartNumSubdiv = NUM_CU_PARTITIONS >> ((depth + 1) << 1);
            for (uint32_t i = 0; i < 4; ++i)
                encodeResidualQT(cu, absPartIdx + i * qpartNumSubdiv, depth + 1, ttype, depthRange);
        }
    }
    else
    {
        const uint32_t log2TrSize  = g_maxLog2CUSize - depth;

        const bool splitIntoSubTUs = (m_csp == X265_CSP_I422);
        uint32_t log2TrSizeC = log2TrSize - m_hChromaShift;

        // Luma
        const uint32_t qtLayer = log2TrSize - 2;
        uint32_t coeffOffsetY = absPartIdx << (LOG2_UNIT_SIZE * 2);
        coeff_t* coeffCurY = m_rqt[qtLayer].coeffRQT[0] + coeffOffsetY;

        // Chroma
        bool bCodeChroma = true;
        uint32_t tuDepthC = tuDepth;
        if ((log2TrSize == 2) && !(m_csp == X265_CSP_I444))
        {
            log2TrSizeC++;
            tuDepthC--;
            uint32_t qpdiv = NUM_CU_PARTITIONS >> ((depth - 1) << 1);
            bCodeChroma = ((absPartIdx & (qpdiv - 1)) == 0);
        }

        if (ttype == TEXT_LUMA && cu.getCbf(absPartIdx, TEXT_LUMA, tuDepth))
            m_entropyCoder.codeCoeffNxN(cu, coeffCurY, absPartIdx, log2TrSize, TEXT_LUMA);

        if (bCodeChroma)
        {
            uint32_t coeffOffsetC = coeffOffsetY >> (m_hChromaShift + m_vChromaShift);
            coeff_t* coeffCurU = m_rqt[qtLayer].coeffRQT[1] + coeffOffsetC;
            coeff_t* coeffCurV = m_rqt[qtLayer].coeffRQT[2] + coeffOffsetC;

            if (!splitIntoSubTUs)
            {
                if (ttype == TEXT_CHROMA_U && cu.getCbf(absPartIdx, TEXT_CHROMA_U, tuDepth))
                    m_entropyCoder.codeCoeffNxN(cu, coeffCurU, absPartIdx, log2TrSizeC, TEXT_CHROMA_U);
                if (ttype == TEXT_CHROMA_V && cu.getCbf(absPartIdx, TEXT_CHROMA_V, tuDepth))
                    m_entropyCoder.codeCoeffNxN(cu, coeffCurV, absPartIdx, log2TrSizeC, TEXT_CHROMA_V);
            }
            else
            {
                uint32_t partIdxesPerSubTU  = NUM_CU_PARTITIONS >> (((cu.m_cuDepth[absPartIdx] + tuDepthC) << 1) + 1);
                uint32_t subTUSize = 1 << (log2TrSizeC * 2);
                if (ttype == TEXT_CHROMA_U && cu.getCbf(absPartIdx, TEXT_CHROMA_U, tuDepth))
                {
                    if (cu.getCbf(absPartIdx, ttype, tuDepth + 1))
                        m_entropyCoder.codeCoeffNxN(cu, coeffCurU, absPartIdx, log2TrSizeC, TEXT_CHROMA_U);
                    if (cu.getCbf(absPartIdx + partIdxesPerSubTU, ttype, tuDepth + 1))
                        m_entropyCoder.codeCoeffNxN(cu, coeffCurU + subTUSize, absPartIdx + partIdxesPerSubTU, log2TrSizeC, TEXT_CHROMA_U);
                }
                if (ttype == TEXT_CHROMA_V && cu.getCbf(absPartIdx, TEXT_CHROMA_V, tuDepth))
                {
                    if (cu.getCbf(absPartIdx, ttype, tuDepth + 1))
                        m_entropyCoder.codeCoeffNxN(cu, coeffCurV, absPartIdx, log2TrSizeC, TEXT_CHROMA_V);
                    if (cu.getCbf(absPartIdx + partIdxesPerSubTU, ttype, tuDepth + 1))
                        m_entropyCoder.codeCoeffNxN(cu, coeffCurV + subTUSize, absPartIdx + partIdxesPerSubTU, log2TrSizeC, TEXT_CHROMA_V);
                }
            }
        }
    }
}

void Search::saveResidualQTData(CUData& cu, ShortYuv& resiYuv, uint32_t absPartIdx, uint32_t depth)
{
    X265_CHECK(cu.m_cuDepth[0] == cu.m_cuDepth[absPartIdx], "depth not matching\n");
    const uint32_t curTrMode = depth - cu.m_cuDepth[0];
    const uint32_t tuDepth   = cu.m_tuDepth[absPartIdx];

    if (curTrMode < tuDepth)
    {
        uint32_t qPartNumSubdiv = NUM_CU_PARTITIONS >> ((depth + 1) << 1);
        for (uint32_t i = 0; i < 4; i++, absPartIdx += qPartNumSubdiv)
            saveResidualQTData(cu, resiYuv, absPartIdx, depth + 1);
        return;
    }

    const uint32_t log2TrSize = g_maxLog2CUSize - depth;
    const uint32_t qtLayer = log2TrSize - 2;

    uint32_t log2TrSizeC = log2TrSize - m_hChromaShift;
    bool bCodeChroma = true;
    uint32_t tuDepthC = tuDepth;
    if (log2TrSizeC == 1)
    {
        X265_CHECK(log2TrSize == 2 && m_csp != X265_CSP_I444, "tuQuad check failed\n");
        log2TrSizeC++;
        tuDepthC--;
        uint32_t qpdiv = NUM_CU_PARTITIONS >> ((cu.m_cuDepth[0] + tuDepthC) << 1);
        bCodeChroma = ((absPartIdx & (qpdiv - 1)) == 0);
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
uint32_t Search::getIntraRemModeBits(CUData& cu, uint32_t absPartIdx, uint32_t preds[3], uint64_t& mpms) const
{
    cu.getIntraDirLumaPredictor(absPartIdx, preds);

    mpms = 0;
    for (int i = 0; i < 3; ++i)
        mpms |= ((uint64_t)1 << preds[i]);

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
