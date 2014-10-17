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

/** \file     TComDataCU.cpp
    \brief    CU data structure
    \todo     not all entities are documented
*/

#include "common.h"
#include "TComDataCU.h"
#include "frame.h"
#include "mv.h"

using namespace x265;

static MV scaleMv(MV mv, int scale)
{
    int mvx = Clip3(-32768, 32767, (scale * mv.x + 127 + (scale * mv.x < 0)) >> 8);
    int mvy = Clip3(-32768, 32767, (scale * mv.y + 127 + (scale * mv.y < 0)) >> 8);

    return MV((int16_t)mvx, (int16_t)mvy);
}

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TComDataCU::TComDataCU()
{
    memset(this, 0, sizeof(*this));
}

void TComDataCU::initialize(DataCUMemPool *dataPool, MVFieldMemPool *mvPool, uint32_t numPartition, uint32_t cuSize, int csp, int index)
{
    m_hChromaShift  = CHROMA_H_SHIFT(csp);
    m_vChromaShift  = CHROMA_V_SHIFT(csp);
    m_chromaFormat  = csp;
    m_numPartitions = numPartition;

    m_cuMvField[0].initialize(mvPool, numPartition, index, 0);
    m_cuMvField[1].initialize(mvPool, numPartition, index, 1);

    /* Each CU's data is layed out sequentially within the charMemBlock */
    uint8_t *charBuf = dataPool->charMemBlock + (numPartition * BytesPerPartition) * index;

    m_qp          = (char*)charBuf; charBuf += numPartition;
    m_depth              = charBuf; charBuf += numPartition;
    m_log2CUSize         = charBuf; charBuf += numPartition;
    m_partSizes          = charBuf; charBuf += numPartition;
    m_predModes          = charBuf; charBuf += numPartition;
    m_skipFlag           = charBuf; charBuf += numPartition;
    m_bMergeFlags        = charBuf; charBuf += numPartition;
    m_interDir           = charBuf; charBuf += numPartition;
    m_mvpIdx[0]          = charBuf; charBuf += numPartition;
    m_mvpIdx[1]          = charBuf; charBuf += numPartition;
    m_trIdx              = charBuf; charBuf += numPartition;
    m_transformSkip[0]   = charBuf; charBuf += numPartition;
    m_transformSkip[1]   = charBuf; charBuf += numPartition;
    m_transformSkip[2]   = charBuf; charBuf += numPartition;
    m_cuTransquantBypass = charBuf; charBuf += numPartition;
    m_cbf[0]             = charBuf; charBuf += numPartition;
    m_cbf[1]             = charBuf; charBuf += numPartition;
    m_cbf[2]             = charBuf; charBuf += numPartition;
    m_lumaIntraDir       = charBuf; charBuf += numPartition;
    m_chromaIntraDir     = charBuf; charBuf += numPartition;

    X265_CHECK(charBuf == dataPool->charMemBlock + (numPartition * BytesPerPartition) * (index + 1), "CU data layout is broken\n");

    uint32_t sizeL = cuSize * cuSize;
    uint32_t sizeC = sizeL >> (m_hChromaShift + m_vChromaShift);
    m_trCoeff[0] = dataPool->trCoeffMemBlock + index * (sizeL + sizeC * 2);
    m_trCoeff[1] = m_trCoeff[0] + sizeL;
    m_trCoeff[2] = m_trCoeff[0] + sizeL + sizeC;
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

// --------------------------------------------------------------------------------------------------------------------
// Initialization
// --------------------------------------------------------------------------------------------------------------------

/**
 - initialize top-level CU
 - internal buffers are already created
 - set values before encoding a CU
 */
void TComDataCU::initCTU(Frame* frame, uint32_t cuAddr, int qp)
{
    m_frame         = frame;
    m_slice         = frame->m_picSym->m_slice;
    m_baseQp        = frame->m_picSym->getCU(m_cuAddr)->m_baseQp;
    m_cuAddr        = cuAddr;
    m_cuPelX        = (cuAddr % frame->m_picSym->getFrameWidthInCU()) << g_maxLog2CUSize;
    m_cuPelY        = (cuAddr / frame->m_picSym->getFrameWidthInCU()) << g_maxLog2CUSize;
    m_absIdxInCTU   = 0;
    m_numPartitions = NUM_CU_PARTITIONS;

    for (int i = 0; i < 4; i++)
    {
        m_avgCost[i] = 0;
        m_count[i] = 0;
    }

    memset(m_skipFlag,           false,         m_numPartitions * sizeof(*m_skipFlag));
    memset(m_cuTransquantBypass, false,         m_numPartitions * sizeof(*m_cuTransquantBypass));
    memset(m_depth,              0,             m_numPartitions * sizeof(*m_depth));
    memset(m_trIdx,              0,             m_numPartitions * sizeof(*m_trIdx));
    memset(m_transformSkip[0],   0,             m_numPartitions * sizeof(*m_transformSkip[0]));
    memset(m_transformSkip[1],   0,             m_numPartitions * sizeof(*m_transformSkip[1]));
    memset(m_transformSkip[2],   0,             m_numPartitions * sizeof(*m_transformSkip[2]));
    memset(m_bMergeFlags,        false,         m_numPartitions * sizeof(*m_bMergeFlags));
    memset(m_chromaIntraDir,     0,             m_numPartitions * sizeof(*m_chromaIntraDir));
    memset(m_interDir,           0,             m_numPartitions * sizeof(*m_interDir));
    memset(m_cbf[0],             0,             m_numPartitions * sizeof(*m_cbf[0]));
    memset(m_cbf[1],             0,             m_numPartitions * sizeof(*m_cbf[1]));
    memset(m_cbf[2],             0,             m_numPartitions * sizeof(*m_cbf[2]));

    memset(m_qp,                 qp,            m_numPartitions * sizeof(*m_qp));
    memset(m_log2CUSize,         g_maxLog2CUSize, m_numPartitions * sizeof(*m_log2CUSize));
    memset(m_lumaIntraDir,       DC_IDX,        m_numPartitions * sizeof(*m_lumaIntraDir));
    memset(m_predModes,          MODE_NONE,     m_numPartitions * sizeof(*m_predModes));
    memset(m_partSizes,          SIZE_NONE,     m_numPartitions * sizeof(*m_partSizes));

    m_cuMvField[0].clearMvField();
    m_cuMvField[1].clearMvField();

    uint32_t widthInCU = frame->m_picSym->getFrameWidthInCU();
    m_cuLeft = (m_cuAddr % widthInCU) ? frame->m_picSym->getCU(m_cuAddr - 1) : NULL;
    m_cuAbove = (m_cuAddr / widthInCU) ? frame->m_picSym->getCU(m_cuAddr - widthInCU) : NULL;
    m_cuAboveLeft = (m_cuLeft && m_cuAbove) ? frame->m_picSym->getCU(m_cuAddr - widthInCU - 1) : NULL;
    m_cuAboveRight = (m_cuAbove && ((m_cuAddr % widthInCU) < (widthInCU - 1))) ? frame->m_picSym->getCU(m_cuAddr - widthInCU + 1) : NULL;
}

// initialize Sub partition
void TComDataCU::initSubCU(const TComDataCU& ctu, const CU& cuData)
{
    m_absIdxInCTU   = cuData.encodeIdx;
    m_numPartitions = cuData.numPartitions;
    m_frame         = ctu.m_frame;
    m_slice         = ctu.m_slice;
    m_cuAddr        = ctu.m_cuAddr;
    m_cuPelX        = ctu.m_cuPelX + g_zscanToPelX[cuData.encodeIdx];
    m_cuPelY        = ctu.m_cuPelY + g_zscanToPelY[cuData.encodeIdx];
    m_cuLeft        = ctu.m_cuLeft;
    m_cuAbove       = ctu.m_cuAbove;
    m_cuAboveLeft   = ctu.m_cuAboveLeft;
    m_cuAboveRight  = ctu.m_cuAboveRight;

    int sizeInChar = sizeof(char) * m_numPartitions;
    memset(m_chromaIntraDir,     0,      sizeInChar);
    memset(m_trIdx,              0,      sizeInChar);
    memset(m_transformSkip[0],   0,      sizeInChar);
    memset(m_transformSkip[1],   0,      sizeInChar);
    memset(m_transformSkip[2],   0,      sizeInChar);
    memset(m_cbf[0],             0,      sizeInChar);
    memset(m_cbf[1],             0,      sizeInChar);
    memset(m_cbf[2],             0,      sizeInChar);
    memset(m_skipFlag,           false,  sizeInChar);
    memset(m_cuTransquantBypass, false,  sizeInChar);

    memset(m_qp,                 ctu.m_qp[0], sizeInChar);
    memset(m_lumaIntraDir,       DC_IDX, sizeInChar);
    memset(m_depth,              cuData.depth, sizeInChar);
    memset(m_log2CUSize,         cuData.log2CUSize, sizeInChar);
    memset(m_partSizes,          SIZE_NONE, sizeInChar);
    memset(m_predModes,          MODE_NONE, sizeInChar);

    if (m_slice->m_sliceType != I_SLICE)
    {
        memset(m_bMergeFlags, 0, sizeInChar);
        memset(m_interDir,    0, sizeInChar);

        m_cuMvField[0].clearMvField();
        m_cuMvField[1].clearMvField();
    }
}


/* TODO: Remove me. this is only called from encodeResidue() */
void TComDataCU::copyFromPic(const TComDataCU& ctu, const CU& cuData)
{
    /* TODO: there are unsaid requirements here that at RD 0 tskip and cu-lossess,
     * tu-depth, etc are ignored. It looks to me we should be using copyPartFrom() */

    m_frame = ctu.m_frame;
    m_slice  = ctu.m_slice;
    m_cuAddr = ctu.m_cuAddr;
    m_cuPelX = ctu.m_cuPelX + g_zscanToPelX[cuData.encodeIdx];
    m_cuPelY = ctu.m_cuPelY + g_zscanToPelY[cuData.encodeIdx];
    m_absIdxInCTU   = cuData.encodeIdx;
    m_numPartitions = cuData.numPartitions;

    int sizeInChar  = sizeof(char) * m_numPartitions;
    memcpy(m_lumaIntraDir, ctu.m_lumaIntraDir + m_absIdxInCTU, sizeInChar);
    memcpy(m_log2CUSize, ctu.m_log2CUSize + m_absIdxInCTU, sizeInChar);
    memcpy(m_predModes,  ctu.m_predModes + m_absIdxInCTU, sizeInChar);
    memcpy(m_skipFlag,   ctu.m_skipFlag + m_absIdxInCTU, sizeInChar);
    memcpy(m_qp,         ctu.m_qp + m_absIdxInCTU, sizeInChar);
    memcpy(m_depth,      ctu.m_depth + m_absIdxInCTU, sizeInChar);
    memcpy(m_partSizes,  ctu.m_partSizes + m_absIdxInCTU, sizeInChar);
}

// Copy small CU to bigger CU.
// One of quarter parts overwritten by predicted sub part.
void TComDataCU::copyPartFrom(const TComDataCU& cuConst, const int numPartitions, uint32_t partUnitIdx, uint32_t depth)
{
    X265_CHECK(partUnitIdx < 4, "part unit should be less than 4\n");

    uint32_t offset = numPartitions * partUnitIdx;
    int sizeInChar  = sizeof(char) * numPartitions;

    /* we have to cheat and get a non-const reference to the passed CU, but we are only copying data from it */
    TComDataCU* cu = const_cast<TComDataCU*>(&cuConst);

    memcpy(m_qp               + offset, cu->m_qp,                sizeInChar);
    memcpy(m_partSizes        + offset, cu->m_partSizes,         sizeInChar);
    memcpy(m_depth            + offset, cu->m_depth,             sizeInChar);
    memcpy(m_transformSkip[0] + offset, cu->m_transformSkip[0],  sizeInChar);
    memcpy(m_transformSkip[1] + offset, cu->m_transformSkip[1],  sizeInChar);
    memcpy(m_transformSkip[2] + offset, cu->m_transformSkip[2],  sizeInChar);
    memcpy(m_skipFlag         + offset, cu->m_skipFlag,          sizeInChar);
    memcpy(m_predModes        + offset, cu->m_predModes,         sizeInChar);
    memcpy(m_log2CUSize       + offset, cu->m_log2CUSize,        sizeInChar);
    memcpy(m_trIdx            + offset, cu->m_trIdx,             sizeInChar);
    memcpy(m_cbf[0]           + offset, cu->m_cbf[0],            sizeInChar);
    memcpy(m_cbf[1]           + offset, cu->m_cbf[1],            sizeInChar);
    memcpy(m_cbf[2]           + offset, cu->m_cbf[2],            sizeInChar);
    memcpy(m_bMergeFlags      + offset, cu->m_bMergeFlags,       sizeInChar);
    memcpy(m_lumaIntraDir     + offset, cu->m_lumaIntraDir,      sizeInChar);
    memcpy(m_chromaIntraDir   + offset, cu->m_chromaIntraDir,    sizeInChar);
    memcpy(m_interDir         + offset, cu->m_interDir,          sizeInChar);
    memcpy(m_mvpIdx[0]        + offset, cu->m_mvpIdx[0],         sizeInChar);
    memcpy(m_mvpIdx[1]        + offset, cu->m_mvpIdx[1],         sizeInChar);

    /* TODO: can this be moved within if(bTransquantBypassEnabled)? */
    memcpy(m_cuTransquantBypass + offset, cu->m_cuTransquantBypass, sizeof(*m_cuTransquantBypass) * numPartitions);

    m_cuMvField[0].copyFrom(&cu->m_cuMvField[REF_PIC_LIST_0], numPartitions, offset);
    m_cuMvField[1].copyFrom(&cu->m_cuMvField[REF_PIC_LIST_1], numPartitions, offset);

    uint32_t tmp  = 1 << ((g_maxLog2CUSize - depth) * 2);
    uint32_t tmp2 = partUnitIdx * tmp;
    memcpy(m_trCoeff[0] + tmp2, cu->m_trCoeff[0], sizeof(coeff_t) * tmp);

    uint32_t tmpC  = tmp  >> (m_hChromaShift + m_vChromaShift);
    uint32_t tmpC2 = tmp2 >> (m_hChromaShift + m_vChromaShift);
    memcpy(m_trCoeff[1] + tmpC2, cu->m_trCoeff[1], sizeof(coeff_t) * tmpC);
    memcpy(m_trCoeff[2] + tmpC2, cu->m_trCoeff[2], sizeof(coeff_t) * tmpC);
}

// Copy current predicted part to a CU in picture.
// It is used to predict for next part
void TComDataCU::copyToPic(uint32_t depth)
{
    TComDataCU* cu = m_frame->m_picSym->getCU(m_cuAddr);

    int sizeInChar = sizeof(char) * m_numPartitions;

    memcpy(cu->m_qp                 + m_absIdxInCTU, m_qp, sizeInChar);
    memcpy(cu->m_partSizes          + m_absIdxInCTU, m_partSizes, sizeInChar);
    memcpy(cu->m_cuTransquantBypass + m_absIdxInCTU, m_cuTransquantBypass, sizeInChar);
    memcpy(cu->m_transformSkip[0]   + m_absIdxInCTU, m_transformSkip[0], sizeInChar);
    memcpy(cu->m_transformSkip[1]   + m_absIdxInCTU, m_transformSkip[1], sizeInChar);
    memcpy(cu->m_transformSkip[2]   + m_absIdxInCTU, m_transformSkip[2], sizeInChar);
    memcpy(cu->m_depth              + m_absIdxInCTU, m_depth, sizeInChar);
    memcpy(cu->m_skipFlag           + m_absIdxInCTU, m_skipFlag, sizeInChar);
    memcpy(cu->m_predModes          + m_absIdxInCTU, m_predModes, sizeInChar);
    memcpy(cu->m_log2CUSize         + m_absIdxInCTU, m_log2CUSize, sizeInChar);
    memcpy(cu->m_trIdx              + m_absIdxInCTU, m_trIdx, sizeInChar);
    memcpy(cu->m_cbf[0]             + m_absIdxInCTU, m_cbf[0], sizeInChar);
    memcpy(cu->m_cbf[1]             + m_absIdxInCTU, m_cbf[1], sizeInChar);
    memcpy(cu->m_cbf[2]             + m_absIdxInCTU, m_cbf[2], sizeInChar);
    memcpy(cu->m_bMergeFlags        + m_absIdxInCTU, m_bMergeFlags, sizeInChar);
    memcpy(cu->m_interDir           + m_absIdxInCTU, m_interDir, sizeInChar);
    memcpy(cu->m_lumaIntraDir       + m_absIdxInCTU, m_lumaIntraDir, sizeInChar);
    memcpy(cu->m_chromaIntraDir     + m_absIdxInCTU, m_chromaIntraDir, sizeInChar);
    memcpy(cu->m_mvpIdx[0]          + m_absIdxInCTU, m_mvpIdx[0], sizeInChar);
    memcpy(cu->m_mvpIdx[1]          + m_absIdxInCTU, m_mvpIdx[1], sizeInChar);

    m_cuMvField[0].copyTo(&cu->m_cuMvField[REF_PIC_LIST_0], m_absIdxInCTU);
    m_cuMvField[1].copyTo(&cu->m_cuMvField[REF_PIC_LIST_1], m_absIdxInCTU);

    uint32_t tmpY  = 1 << ((g_maxLog2CUSize - depth) * 2);
    uint32_t tmpY2 = m_absIdxInCTU << (LOG2_UNIT_SIZE * 2);
    memcpy(cu->m_trCoeff[0] + tmpY2, m_trCoeff[0], sizeof(coeff_t) * tmpY);

    uint32_t tmpC  = tmpY  >> (m_hChromaShift + m_vChromaShift);
    uint32_t tmpC2 = tmpY2 >> (m_hChromaShift + m_vChromaShift);
    memcpy(cu->m_trCoeff[1] + tmpC2, m_trCoeff[1], sizeof(coeff_t) * tmpC);
    memcpy(cu->m_trCoeff[2] + tmpC2, m_trCoeff[2], sizeof(coeff_t) * tmpC);
}

/* Only called by encodeResidue, these fields can be modified during inter/intra coding */
void TComDataCU::updatePic(uint32_t depth)
{
    TComDataCU* cu = m_frame->m_picSym->getCU(m_cuAddr);

    int sizeInChar = sizeof(uint8_t) * m_numPartitions;

    memcpy(cu->m_transformSkip[0] + m_absIdxInCTU, m_transformSkip[0], sizeInChar);
    memcpy(cu->m_transformSkip[1] + m_absIdxInCTU, m_transformSkip[1], sizeInChar);
    memcpy(cu->m_transformSkip[2] + m_absIdxInCTU, m_transformSkip[2], sizeInChar);
    memcpy(cu->m_skipFlag + m_absIdxInCTU, m_skipFlag, sizeInChar);
    memcpy(cu->m_trIdx + m_absIdxInCTU, m_trIdx, sizeInChar);
    memcpy(cu->m_qp + m_absIdxInCTU, m_qp, sizeInChar);
    memcpy(cu->m_cbf[0] + m_absIdxInCTU, m_cbf[0], sizeInChar);
    memcpy(cu->m_cbf[1] + m_absIdxInCTU, m_cbf[1], sizeInChar);
    memcpy(cu->m_cbf[2] + m_absIdxInCTU, m_cbf[2], sizeInChar);
    memcpy(cu->m_chromaIntraDir + m_absIdxInCTU, m_chromaIntraDir, sizeInChar);

    uint32_t tmpY = 1 << ((g_maxLog2CUSize - depth) * 2);
    uint32_t tmpY2 = m_absIdxInCTU << (LOG2_UNIT_SIZE * 2);
    memcpy(cu->m_trCoeff[0] + tmpY2, m_trCoeff[0], sizeof(coeff_t) * tmpY);
    tmpY  >>= m_hChromaShift + m_vChromaShift;
    tmpY2 >>= m_hChromaShift + m_vChromaShift;
    memcpy(cu->m_trCoeff[1] + tmpY2, m_trCoeff[1], sizeof(coeff_t) * tmpY);
    memcpy(cu->m_trCoeff[2] + tmpY2, m_trCoeff[2], sizeof(coeff_t) * tmpY);
}

/* TODO: Only called by encodeIntraInInter; and probably shouldn't be */
void TComDataCU::copyToPic(uint32_t depth, uint32_t partIdx, uint32_t partDepth)
{
    TComDataCU* cu = m_frame->m_picSym->getCU(m_cuAddr);
    uint32_t qNumPart = m_numPartitions >> (partDepth << 1);

    uint32_t partStart = partIdx * qNumPart;
    uint32_t partOffset = m_absIdxInCTU + partStart;

    int sizeInChar = sizeof(char) * qNumPart;

    memcpy(cu->m_qp                  + partOffset, m_qp, sizeInChar);
    memcpy(cu->m_partSizes           + partOffset, m_partSizes, sizeInChar);
    memcpy(cu->m_depth               + partOffset, m_depth,  sizeInChar);
    memcpy(cu->m_cuTransquantBypass  + partOffset, m_cuTransquantBypass, sizeInChar);
    memcpy(cu->m_transformSkip[0]    + partOffset, m_transformSkip[0], sizeInChar);
    memcpy(cu->m_transformSkip[1]    + partOffset, m_transformSkip[1], sizeInChar);
    memcpy(cu->m_transformSkip[2]    + partOffset, m_transformSkip[2], sizeInChar);
    memcpy(cu->m_skipFlag            + partOffset, m_skipFlag, sizeInChar);
    memcpy(cu->m_predModes           + partOffset, m_predModes, sizeInChar);
    memcpy(cu->m_log2CUSize          + partOffset, m_log2CUSize, sizeInChar);
    memcpy(cu->m_trIdx               + partOffset, m_trIdx, sizeInChar);
    memcpy(cu->m_cbf[0]              + partOffset, m_cbf[0], sizeInChar);
    memcpy(cu->m_cbf[1]              + partOffset, m_cbf[1], sizeInChar);
    memcpy(cu->m_cbf[2]              + partOffset, m_cbf[2], sizeInChar);
    memcpy(cu->m_bMergeFlags         + partOffset, m_bMergeFlags, sizeInChar);
    memcpy(cu->m_interDir            + partOffset, m_interDir, sizeInChar);
    memcpy(cu->m_lumaIntraDir        + partOffset, m_lumaIntraDir, sizeInChar);
    memcpy(cu->m_chromaIntraDir      + partOffset, m_chromaIntraDir, sizeInChar);
    memcpy(cu->m_mvpIdx[0]           + partOffset, m_mvpIdx[0], sizeInChar);
    memcpy(cu->m_mvpIdx[1]           + partOffset, m_mvpIdx[1], sizeInChar);

    m_cuMvField[0].copyTo(&cu->m_cuMvField[REF_PIC_LIST_0], m_absIdxInCTU, partStart, qNumPart);
    m_cuMvField[1].copyTo(&cu->m_cuMvField[REF_PIC_LIST_1], m_absIdxInCTU, partStart, qNumPart);

    uint32_t tmpY  = 1 << ((g_maxLog2CUSize - depth - partDepth) * 2);
    uint32_t tmpY2 = partOffset << (LOG2_UNIT_SIZE * 2);
    memcpy(cu->m_trCoeff[0] + tmpY2, m_trCoeff[0], sizeof(coeff_t) * tmpY);

    uint32_t tmpC  = tmpY >> (m_hChromaShift + m_vChromaShift);
    uint32_t tmpC2 = tmpY2 >> (m_hChromaShift + m_vChromaShift);
    memcpy(cu->m_trCoeff[1] + tmpC2, m_trCoeff[1], sizeof(coeff_t) * tmpC);
    memcpy(cu->m_trCoeff[2] + tmpC2, m_trCoeff[2], sizeof(coeff_t) * tmpC);
}

// --------------------------------------------------------------------------------------------------------------------
// Other public functions
// --------------------------------------------------------------------------------------------------------------------

const TComDataCU* TComDataCU::getPULeft(uint32_t& lPartUnitIdx, uint32_t curPartUnitIdx) const
{
    uint32_t absPartIdx       = g_zscanToRaster[curPartUnitIdx];
    uint32_t numPartInCUSize  = m_frame->m_picSym->getNumPartInCUSize();

    if (!RasterAddress::isZeroCol(absPartIdx, numPartInCUSize))
    {
        uint32_t absZorderCUIdx   = g_zscanToRaster[m_absIdxInCTU];
        lPartUnitIdx = g_rasterToZscan[absPartIdx - 1];
        if (RasterAddress::isEqualCol(absPartIdx, absZorderCUIdx, numPartInCUSize))
            return m_frame->m_picSym->getCU(m_cuAddr);
        else
        {
            lPartUnitIdx -= m_absIdxInCTU;
            return this;
        }
    }

    lPartUnitIdx = g_rasterToZscan[absPartIdx + numPartInCUSize - 1];
    return m_cuLeft;
}

const TComDataCU* TComDataCU::getPUAbove(uint32_t& aPartUnitIdx, uint32_t curPartUnitIdx, bool planarAtCTUBoundary) const
{
    uint32_t absPartIdx       = g_zscanToRaster[curPartUnitIdx];
    uint32_t numPartInCUSize  = m_frame->m_picSym->getNumPartInCUSize();

    if (!RasterAddress::isZeroRow(absPartIdx, numPartInCUSize))
    {
        uint32_t absZorderCUIdx   = g_zscanToRaster[m_absIdxInCTU];
        aPartUnitIdx = g_rasterToZscan[absPartIdx - numPartInCUSize];
        if (RasterAddress::isEqualRow(absPartIdx, absZorderCUIdx, numPartInCUSize))
            return m_frame->m_picSym->getCU(m_cuAddr);
        else
        {
            aPartUnitIdx -= m_absIdxInCTU;
            return this;
        }
    }

    if (planarAtCTUBoundary)
        return NULL;

    aPartUnitIdx = g_rasterToZscan[absPartIdx + NUM_CU_PARTITIONS - numPartInCUSize];
    return m_cuAbove;
}

const TComDataCU* TComDataCU::getPUAboveLeft(uint32_t& alPartUnitIdx, uint32_t curPartUnitIdx) const
{
    uint32_t absPartIdx      = g_zscanToRaster[curPartUnitIdx];
    uint32_t numPartInCUSize = m_frame->m_picSym->getNumPartInCUSize();

    if (!RasterAddress::isZeroCol(absPartIdx, numPartInCUSize))
    {
        if (!RasterAddress::isZeroRow(absPartIdx, numPartInCUSize))
        {
            uint32_t absZorderCUIdx  = g_zscanToRaster[m_absIdxInCTU];
            alPartUnitIdx = g_rasterToZscan[absPartIdx - numPartInCUSize - 1];
            if (RasterAddress::isEqualRowOrCol(absPartIdx, absZorderCUIdx, numPartInCUSize))
                return m_frame->m_picSym->getCU(m_cuAddr);
            else
            {
                alPartUnitIdx -= m_absIdxInCTU;
                return this;
            }
        }
        alPartUnitIdx = g_rasterToZscan[absPartIdx + NUM_CU_PARTITIONS - numPartInCUSize - 1];
        return m_cuAbove;
    }

    if (!RasterAddress::isZeroRow(absPartIdx, numPartInCUSize))
    {
        alPartUnitIdx = g_rasterToZscan[absPartIdx - 1];
        return m_cuLeft;
    }

    alPartUnitIdx = g_rasterToZscan[NUM_CU_PARTITIONS - 1];
    return m_cuAboveLeft;
}

const TComDataCU* TComDataCU::getPUAboveRight(uint32_t& arPartUnitIdx, uint32_t curPartUnitIdx) const
{
    if ((m_frame->m_picSym->getCU(m_cuAddr)->m_cuPelX + g_zscanToPelX[curPartUnitIdx] + UNIT_SIZE) >= m_slice->m_sps->picWidthInLumaSamples)
        return NULL;

    uint32_t absPartIdxRT    = g_zscanToRaster[curPartUnitIdx];
    uint32_t numPartInCUSize = m_frame->m_picSym->getNumPartInCUSize();

    if (RasterAddress::lessThanCol(absPartIdxRT, numPartInCUSize - 1, numPartInCUSize))
    {
        if (!RasterAddress::isZeroRow(absPartIdxRT, numPartInCUSize))
        {
            if (curPartUnitIdx > g_rasterToZscan[absPartIdxRT - numPartInCUSize + 1])
            {
                uint32_t absZorderCUIdx  = g_zscanToRaster[m_absIdxInCTU] + (1 << (m_log2CUSize[0] - LOG2_UNIT_SIZE)) - 1;
                arPartUnitIdx = g_rasterToZscan[absPartIdxRT - numPartInCUSize + 1];
                if (RasterAddress::isEqualRowOrCol(absPartIdxRT, absZorderCUIdx, numPartInCUSize))
                    return m_frame->m_picSym->getCU(m_cuAddr);
                else
                {
                    arPartUnitIdx -= m_absIdxInCTU;
                    return this;
                }
            }
            return NULL;
        }
        arPartUnitIdx = g_rasterToZscan[absPartIdxRT + NUM_CU_PARTITIONS - numPartInCUSize + 1];
        return m_cuAbove;
    }

    if (!RasterAddress::isZeroRow(absPartIdxRT, numPartInCUSize))
        return NULL;

    arPartUnitIdx = g_rasterToZscan[NUM_CU_PARTITIONS - numPartInCUSize];
    return m_cuAboveRight;
}

const TComDataCU* TComDataCU::getPUBelowLeft(uint32_t& blPartUnitIdx, uint32_t curPartUnitIdx) const
{
    if ((m_frame->m_picSym->getCU(m_cuAddr)->m_cuPelY + g_zscanToPelY[curPartUnitIdx] + UNIT_SIZE) >= m_slice->m_sps->picHeightInLumaSamples)
        return NULL;

    uint32_t absPartIdxLB    = g_zscanToRaster[curPartUnitIdx];
    uint32_t numPartInCUSize = m_frame->m_picSym->getNumPartInCUSize();

    if (RasterAddress::lessThanRow(absPartIdxLB, numPartInCUSize - 1, numPartInCUSize))
    {
        if (!RasterAddress::isZeroCol(absPartIdxLB, numPartInCUSize))
        {
            if (curPartUnitIdx > g_rasterToZscan[absPartIdxLB + numPartInCUSize - 1])
            {
                uint32_t absZorderCUIdxLB = g_zscanToRaster[m_absIdxInCTU] + ((1 << (m_log2CUSize[0] - LOG2_UNIT_SIZE)) - 1) * m_frame->m_picSym->getNumPartInCUSize();
                blPartUnitIdx = g_rasterToZscan[absPartIdxLB + numPartInCUSize - 1];
                if (RasterAddress::isEqualRowOrCol(absPartIdxLB, absZorderCUIdxLB, numPartInCUSize))
                    return m_frame->m_picSym->getCU(m_cuAddr);
                else
                {
                    blPartUnitIdx -= m_absIdxInCTU;
                    return this;
                }
            }
            return NULL;
        }
        blPartUnitIdx = g_rasterToZscan[absPartIdxLB + numPartInCUSize * 2 - 1];
        return m_cuLeft;
    }

    return NULL;
}

const TComDataCU* TComDataCU::getPUBelowLeftAdi(uint32_t& blPartUnitIdx,  uint32_t curPartUnitIdx, uint32_t partUnitOffset) const
{
    if ((m_frame->m_picSym->getCU(m_cuAddr)->m_cuPelY + g_zscanToPelY[curPartUnitIdx] + (partUnitOffset << LOG2_UNIT_SIZE)) >= m_slice->m_sps->picHeightInLumaSamples)
        return NULL;

    uint32_t absPartIdxLB    = g_zscanToRaster[curPartUnitIdx];
    uint32_t numPartInCUSize = m_frame->m_picSym->getNumPartInCUSize();

    if (RasterAddress::lessThanRow(absPartIdxLB, numPartInCUSize - partUnitOffset, numPartInCUSize))
    {
        if (!RasterAddress::isZeroCol(absPartIdxLB, numPartInCUSize))
        {
            if (curPartUnitIdx > g_rasterToZscan[absPartIdxLB + partUnitOffset * numPartInCUSize - 1])
            {
                uint32_t absZorderCUIdxLB = g_zscanToRaster[m_absIdxInCTU] + ((1 << (m_log2CUSize[0] - LOG2_UNIT_SIZE)) - 1) * m_frame->m_picSym->getNumPartInCUSize();
                blPartUnitIdx = g_rasterToZscan[absPartIdxLB + partUnitOffset * numPartInCUSize - 1];
                if (RasterAddress::isEqualRowOrCol(absPartIdxLB, absZorderCUIdxLB, numPartInCUSize))
                    return m_frame->m_picSym->getCU(m_cuAddr);
                else
                {
                    blPartUnitIdx -= m_absIdxInCTU;
                    return this;
                }
            }
            return NULL;
        }
        blPartUnitIdx = g_rasterToZscan[absPartIdxLB + (1 + partUnitOffset) * numPartInCUSize - 1];
        if (!m_cuLeft || !m_cuLeft->m_slice)
            return NULL;
        return m_cuLeft;
    }

    return NULL;
}

const TComDataCU* TComDataCU::getPUAboveRightAdi(uint32_t& arPartUnitIdx, uint32_t curPartUnitIdx, uint32_t partUnitOffset) const
{
    if ((m_frame->m_picSym->getCU(m_cuAddr)->m_cuPelX + g_zscanToPelX[curPartUnitIdx] + (partUnitOffset << LOG2_UNIT_SIZE)) >= m_slice->m_sps->picWidthInLumaSamples)
        return NULL;

    uint32_t absPartIdxRT    = g_zscanToRaster[curPartUnitIdx];
    uint32_t numPartInCUSize = m_frame->m_picSym->getNumPartInCUSize();

    if (RasterAddress::lessThanCol(absPartIdxRT, numPartInCUSize - partUnitOffset, numPartInCUSize))
    {
        if (!RasterAddress::isZeroRow(absPartIdxRT, numPartInCUSize))
        {
            if (curPartUnitIdx > g_rasterToZscan[absPartIdxRT - numPartInCUSize + partUnitOffset])
            {
                uint32_t absZorderCUIdx = g_zscanToRaster[m_absIdxInCTU] + (1 << (m_log2CUSize[0] - LOG2_UNIT_SIZE)) - 1;
                arPartUnitIdx = g_rasterToZscan[absPartIdxRT - numPartInCUSize + partUnitOffset];
                if (RasterAddress::isEqualRowOrCol(absPartIdxRT, absZorderCUIdx, numPartInCUSize))
                    return m_frame->m_picSym->getCU(m_cuAddr);
                else
                {
                    arPartUnitIdx -= m_absIdxInCTU;
                    return this;
                }
            }
            return NULL;
        }
        arPartUnitIdx = g_rasterToZscan[absPartIdxRT + NUM_CU_PARTITIONS - numPartInCUSize + partUnitOffset];
        if (!m_cuAbove || !m_cuAbove->m_slice)
            return NULL;
        return m_cuAbove;
    }

    if (!RasterAddress::isZeroRow(absPartIdxRT, numPartInCUSize))
        return NULL;

    arPartUnitIdx = g_rasterToZscan[NUM_CU_PARTITIONS - numPartInCUSize + partUnitOffset - 1];
    if ((m_cuAboveRight == NULL || m_cuAboveRight->m_slice == NULL || (m_cuAboveRight->m_cuAddr) > m_cuAddr))
        return NULL;
    return m_cuAboveRight;
}

/** Get left QpMinCu
*\param   lPartUnitIdx
*\param   curAbsIdxInCTU
*\returns TComDataCU*   point of TComDataCU of left QpMinCu
*/
const TComDataCU* TComDataCU::getQpMinCuLeft(uint32_t& lPartUnitIdx, uint32_t curAbsIdxInCTU) const
{
    uint32_t numPartInCUSize = m_frame->m_picSym->getNumPartInCUSize();
    uint32_t absZorderQpMinCUIdx = curAbsIdxInCTU & (0xFF << (g_maxFullDepth - m_slice->m_pps->maxCuDQPDepth) * 2);
    uint32_t absRorderQpMinCUIdx = g_zscanToRaster[absZorderQpMinCUIdx];

    // check for left CTU boundary
    if (RasterAddress::isZeroCol(absRorderQpMinCUIdx, numPartInCUSize))
        return NULL;

    // get index of left-CU relative to top-left corner of current quantization group
    lPartUnitIdx = g_rasterToZscan[absRorderQpMinCUIdx - 1];

    // return pointer to current CTU
    return m_frame->m_picSym->getCU(m_cuAddr);
}

/** Get Above QpMinCu
*\param   aPartUnitIdx
*\param   curAbsIdxInCTU
*\returns TComDataCU*   point of TComDataCU of above QpMinCu
*/
const TComDataCU* TComDataCU::getQpMinCuAbove(uint32_t& aPartUnitIdx, uint32_t curAbsIdxInCTU) const
{
    uint32_t numPartInCUSize = m_frame->m_picSym->getNumPartInCUSize();
    uint32_t absZorderQpMinCUIdx = curAbsIdxInCTU & (0xFF << (g_maxFullDepth - m_slice->m_pps->maxCuDQPDepth) * 2);
    uint32_t absRorderQpMinCUIdx = g_zscanToRaster[absZorderQpMinCUIdx];

    // check for top CTU boundary
    if (RasterAddress::isZeroRow(absRorderQpMinCUIdx, numPartInCUSize))
        return NULL;

    // get index of top-CU relative to top-left corner of current quantization group
    aPartUnitIdx = g_rasterToZscan[absRorderQpMinCUIdx - numPartInCUSize];

    // return pointer to current CTU
    return m_frame->m_picSym->getCU(m_cuAddr);
}

/** Get reference QP from left QpMinCu or latest coded QP
*\param   curAbsIdxInCTU
*\returns char reference QP value
*/
char TComDataCU::getRefQP(uint32_t curAbsIdxInCTU) const
{
    uint32_t lPartIdx = 0, aPartIdx = 0;
    const TComDataCU* cULeft = getQpMinCuLeft(lPartIdx, m_absIdxInCTU + curAbsIdxInCTU);
    const TComDataCU* cUAbove = getQpMinCuAbove(aPartIdx, m_absIdxInCTU + curAbsIdxInCTU);

    return ((cULeft ? cULeft->m_qp[lPartIdx] : getLastCodedQP(curAbsIdxInCTU)) + (cUAbove ? cUAbove->m_qp[aPartIdx] : getLastCodedQP(curAbsIdxInCTU)) + 1) >> 1;
}

int TComDataCU::getLastValidPartIdx(int absPartIdx) const
{
    int lastValidPartIdx = absPartIdx - 1;

    while (lastValidPartIdx >= 0 && m_predModes[lastValidPartIdx] == MODE_NONE)
    {
        uint32_t depth = m_depth[lastValidPartIdx];
        lastValidPartIdx -= m_numPartitions >> (depth << 1);
    }

    return lastValidPartIdx;
}

char TComDataCU::getLastCodedQP(uint32_t absPartIdx) const
{
    uint32_t quPartIdxMask = 0xFF << (g_maxFullDepth - m_slice->m_pps->maxCuDQPDepth) * 2;
    int lastValidPartIdx = getLastValidPartIdx(absPartIdx & quPartIdxMask);

    if (lastValidPartIdx >= 0)
        return m_qp[lastValidPartIdx];
    else
    {
        TComDataCU* ctu = m_frame->m_picSym->getCU(m_cuAddr);
        if (ctu->m_cuLocalData->encodeIdx > 0)
            return ctu->getLastCodedQP(ctu->m_cuLocalData->encodeIdx);
        else if (m_cuAddr > 0 && !(m_slice->m_pps->bEntropyCodingSyncEnabled && (m_cuAddr % m_frame->m_picSym->getFrameWidthInCU()) == 0))
            return m_frame->m_picSym->getCU(m_cuAddr - 1)->getLastCodedQP(NUM_CU_PARTITIONS);
        else
            return m_slice->m_sliceQp;
    }
}

/** Get allowed chroma intra modes
*\param   absPartIdx
*\param   uiModeList  pointer to chroma intra modes array
*\returns
*- fill uiModeList with chroma intra modes
*/
void TComDataCU::getAllowedChromaDir(uint32_t absPartIdx, uint32_t* modeList) const
{
    modeList[0] = PLANAR_IDX;
    modeList[1] = VER_IDX;
    modeList[2] = HOR_IDX;
    modeList[3] = DC_IDX;
    modeList[4] = DM_CHROMA_IDX;

    uint32_t lumaMode = m_lumaIntraDir[absPartIdx];

    for (int i = 0; i < NUM_CHROMA_MODE - 1; i++)
    {
        if (lumaMode == modeList[i])
        {
            modeList[i] = 34; // VER+8 mode
            break;
        }
    }
}

/** Get most probable intra modes
*\param   absPartIdx
*\param   intraDirPred  pointer to the array for MPM storage
*\param   mode          it is set with MPM mode in case both MPM are equal. It is used to restrict RD search at encode side.
*\returns Number of MPM
*/
int TComDataCU::getIntraDirLumaPredictor(uint32_t absPartIdx, uint32_t* intraDirPred) const
{
    const TComDataCU* tempCU;
    uint32_t    tempPartIdx;
    uint32_t    leftIntraDir, aboveIntraDir;

    // Get intra direction of left PU
    tempCU = getPULeft(tempPartIdx, m_absIdxInCTU + absPartIdx);

    leftIntraDir = (tempCU && tempCU->isIntra(tempPartIdx)) ? tempCU->m_lumaIntraDir[tempPartIdx] : DC_IDX;

    // Get intra direction of above PU
    tempCU = getPUAbove(tempPartIdx, m_absIdxInCTU + absPartIdx, true);

    aboveIntraDir = (tempCU && tempCU->isIntra(tempPartIdx)) ? tempCU->m_lumaIntraDir[tempPartIdx] : DC_IDX;

    if (leftIntraDir == aboveIntraDir)
    {
        if (leftIntraDir >= 2) // angular modes
        {
            intraDirPred[0] = leftIntraDir;
            intraDirPred[1] = ((leftIntraDir - 2 + 31) & 31) + 2;
            intraDirPred[2] = ((leftIntraDir - 2 +  1) & 31) + 2;
        }
        else //non-angular
        {
            intraDirPred[0] = PLANAR_IDX;
            intraDirPred[1] = DC_IDX;
            intraDirPred[2] = VER_IDX;
        }
        return 1;
    }
    else
    {
        intraDirPred[0] = leftIntraDir;
        intraDirPred[1] = aboveIntraDir;

        if (leftIntraDir && aboveIntraDir) //both modes are non-planar
        {
            intraDirPred[2] = PLANAR_IDX;
        }
        else
        {
            intraDirPred[2] =  (leftIntraDir + aboveIntraDir) < 2 ? VER_IDX : DC_IDX;
        }
        return 2;
    }
}

uint32_t TComDataCU::getCtxSplitFlag(uint32_t absPartIdx, uint32_t depth) const
{
    const TComDataCU* tempCU;
    uint32_t    tempPartIdx;
    uint32_t    ctx;

    // Get left split flag
    tempCU = getPULeft(tempPartIdx, m_absIdxInCTU + absPartIdx);
    ctx  = (tempCU) ? ((tempCU->m_depth[tempPartIdx] > depth) ? 1 : 0) : 0;

    // Get above split flag
    tempCU = getPUAbove(tempPartIdx, m_absIdxInCTU + absPartIdx);
    ctx += (tempCU) ? ((tempCU->m_depth[tempPartIdx] > depth) ? 1 : 0) : 0;

    return ctx;
}

void TComDataCU::getQuadtreeTULog2MinSizeInCU(uint32_t tuDepthRange[2], uint32_t absPartIdx) const
{
    uint32_t log2CUSize = m_log2CUSize[absPartIdx];
    PartSize partSize   = (PartSize)m_partSizes[absPartIdx];
    uint32_t quadtreeTUMaxDepth = m_predModes[absPartIdx] == MODE_INTRA ? m_slice->m_sps->quadtreeTUMaxDepthIntra : m_slice->m_sps->quadtreeTUMaxDepthInter;
    uint32_t intraSplitFlag = (m_predModes[absPartIdx] == MODE_INTRA && partSize == SIZE_NxN) ? 1 : 0;
    uint32_t interSplitFlag = ((quadtreeTUMaxDepth == 1) && (m_predModes[absPartIdx] == MODE_INTER) && (partSize != SIZE_2Nx2N));

    tuDepthRange[0] = m_slice->m_sps->quadtreeTULog2MinSize;
    tuDepthRange[1] = m_slice->m_sps->quadtreeTULog2MaxSize;

    tuDepthRange[0] = X265_MAX(tuDepthRange[0], X265_MIN(log2CUSize - (quadtreeTUMaxDepth - 1 + interSplitFlag + intraSplitFlag), tuDepthRange[1]));
}

uint32_t TComDataCU::getCtxSkipFlag(uint32_t absPartIdx) const
{
    const TComDataCU* tempCU;
    uint32_t tempPartIdx;
    uint32_t ctx;

    // Get BCBP of left PU
    tempCU = getPULeft(tempPartIdx, m_absIdxInCTU + absPartIdx);
    ctx    = tempCU ? tempCU->isSkipped(tempPartIdx) : 0;

    // Get BCBP of above PU
    tempCU = getPUAbove(tempPartIdx, m_absIdxInCTU + absPartIdx);
    ctx   += tempCU ? tempCU->isSkipped(tempPartIdx) : 0;

    return ctx;
}

void TComDataCU::clearCbf(uint32_t absPartIdx, uint32_t depth)
{
    uint32_t curPartNum = NUM_CU_PARTITIONS >> (depth << 1);

    memset(m_cbf[0] + absPartIdx, 0, sizeof(uint8_t) * curPartNum);
    memset(m_cbf[1] + absPartIdx, 0, sizeof(uint8_t) * curPartNum);
    memset(m_cbf[2] + absPartIdx, 0, sizeof(uint8_t) * curPartNum);
}

void TComDataCU::setCbfSubParts(uint32_t cbf, TextType ttype, uint32_t absPartIdx, uint32_t depth)
{
    uint32_t curPartNum = NUM_CU_PARTITIONS >> (depth << 1);

    memset(m_cbf[ttype] + absPartIdx, cbf, sizeof(uint8_t) * curPartNum);
}

void TComDataCU::setCbfPartRange(uint32_t cbf, TextType ttype, uint32_t absPartIdx, uint32_t coveredPartIdxes)
{
    memset(m_cbf[ttype] + absPartIdx, cbf, sizeof(uint8_t) * coveredPartIdxes);
}

void TComDataCU::setDepthSubParts(uint32_t depth)
{
    /*All 4x4 partitions in current CU have the CU depth saved*/
    uint32_t curPartNum = NUM_CU_PARTITIONS >> (depth << 1);

    memset(m_depth, depth, sizeof(uint8_t) * curPartNum);
}

void TComDataCU::setPartSizeSubParts(PartSize mode, uint32_t absPartIdx, uint32_t depth)
{
    X265_CHECK(sizeof(*m_partSizes) == 1, "size check failure\n");
    memset(m_partSizes + absPartIdx, mode, NUM_CU_PARTITIONS >> (depth << 1));
}

void TComDataCU::setCUTransquantBypassSubParts(uint8_t flag, uint32_t absPartIdx, uint32_t depth)
{
    memset(m_cuTransquantBypass + absPartIdx, flag, NUM_CU_PARTITIONS >> (depth << 1));
}

void TComDataCU::setSkipFlagSubParts(uint8_t skip, uint32_t absPartIdx, uint32_t depth)
{
    X265_CHECK(sizeof(*m_skipFlag) == 1, "size check failure\n");
    memset(m_skipFlag + absPartIdx, skip, NUM_CU_PARTITIONS >> (depth << 1));
}

void TComDataCU::setPredModeSubParts(PredMode eMode, uint32_t absPartIdx, uint32_t depth)
{
    X265_CHECK(sizeof(*m_predModes) == 1, "size check failure\n");
    memset(m_predModes + absPartIdx, eMode, NUM_CU_PARTITIONS >> (depth << 1));
}

void TComDataCU::setQPSubCUs(int qp, TComDataCU* cu, uint32_t absPartIdx, uint32_t depth, bool &foundNonZeroCbf)
{
    uint32_t curPartNumb = NUM_CU_PARTITIONS >> (depth << 1);
    uint32_t curPartNumQ = curPartNumb >> 2;

    if (!foundNonZeroCbf)
    {
        if (cu->m_depth[absPartIdx] > depth)
        {
            for (uint32_t partUnitIdx = 0; partUnitIdx < 4; partUnitIdx++)
                cu->setQPSubCUs(qp, cu, absPartIdx + partUnitIdx * curPartNumQ, depth + 1, foundNonZeroCbf);
        }
        else
        {
            if (cu->getQtRootCbf(absPartIdx))
                foundNonZeroCbf = true;
            else
                setQPSubParts(qp, absPartIdx, depth);
        }
    }
}

void TComDataCU::setQPSubParts(int qp, uint32_t absPartIdx, uint32_t depth)
{
    uint32_t curPartNum = NUM_CU_PARTITIONS >> (depth << 1);

    for (uint32_t scuIdx = absPartIdx; scuIdx < absPartIdx + curPartNum; scuIdx++)
        m_qp[scuIdx] = qp;
}

void TComDataCU::setLumaIntraDirSubParts(uint32_t dir, uint32_t absPartIdx, uint32_t depth)
{
    uint32_t curPartNum = NUM_CU_PARTITIONS >> (depth << 1);

    memset(m_lumaIntraDir + absPartIdx, dir, sizeof(uint8_t) * curPartNum);
}

template<typename T>
void TComDataCU::setSubPart(T param, T* baseCTU, uint32_t cuAddr, uint32_t cuDepth, uint32_t puIdx)
{
    X265_CHECK(sizeof(T) == 1, "size check failure\n"); // Using memset() works only for types of size 1

    uint32_t curPartNumQ = (NUM_CU_PARTITIONS >> (2 * cuDepth)) >> 2;
    switch (m_partSizes[cuAddr])
    {
    case SIZE_2Nx2N:
        memset(baseCTU + cuAddr, param, 4 * curPartNumQ);
        break;
    case SIZE_2NxN:
        memset(baseCTU + cuAddr, param, 2 * curPartNumQ);
        break;
    case SIZE_Nx2N:
        memset(baseCTU + cuAddr, param, curPartNumQ);
        memset(baseCTU + cuAddr + 2 * curPartNumQ, param, curPartNumQ);
        break;
    case SIZE_NxN:
        memset(baseCTU + cuAddr, param, curPartNumQ);
        break;
    case SIZE_2NxnU:
        if (puIdx == 0)
        {
            memset(baseCTU + cuAddr, param, (curPartNumQ >> 1));
            memset(baseCTU + cuAddr + curPartNumQ, param, (curPartNumQ >> 1));
        }
        else if (puIdx == 1)
        {
            memset(baseCTU + cuAddr, param, (curPartNumQ >> 1));
            memset(baseCTU + cuAddr + curPartNumQ, param, ((curPartNumQ >> 1) + (curPartNumQ << 1)));
        }
        else
        {
            X265_CHECK(0, "unexpected part unit index\n");
        }
        break;
    case SIZE_2NxnD:
        if (puIdx == 0)
        {
            memset(baseCTU + cuAddr, param, ((curPartNumQ << 1) + (curPartNumQ >> 1)));
            memset(baseCTU + cuAddr + (curPartNumQ << 1) + curPartNumQ, param, (curPartNumQ >> 1));
        }
        else if (puIdx == 1)
        {
            memset(baseCTU + cuAddr, param, (curPartNumQ >> 1));
            memset(baseCTU + cuAddr + curPartNumQ, param, (curPartNumQ >> 1));
        }
        else
        {
            X265_CHECK(0, "unexpected part unit index\n");
        }
        break;
    case SIZE_nLx2N:
        if (puIdx == 0)
        {
            memset(baseCTU + cuAddr, param, (curPartNumQ >> 2));
            memset(baseCTU + cuAddr + (curPartNumQ >> 1), param, (curPartNumQ >> 2));
            memset(baseCTU + cuAddr + (curPartNumQ << 1), param, (curPartNumQ >> 2));
            memset(baseCTU + cuAddr + (curPartNumQ << 1) + (curPartNumQ >> 1), param, (curPartNumQ >> 2));
        }
        else if (puIdx == 1)
        {
            memset(baseCTU + cuAddr, param, (curPartNumQ >> 2));
            memset(baseCTU + cuAddr + (curPartNumQ >> 1), param, (curPartNumQ + (curPartNumQ >> 2)));
            memset(baseCTU + cuAddr + (curPartNumQ << 1), param, (curPartNumQ >> 2));
            memset(baseCTU + cuAddr + (curPartNumQ << 1) + (curPartNumQ >> 1), param, (curPartNumQ + (curPartNumQ >> 2)));
        }
        else
        {
            X265_CHECK(0, "unexpected part unit index\n");
        }
        break;
    case SIZE_nRx2N:
        if (puIdx == 0)
        {
            memset(baseCTU + cuAddr, param, (curPartNumQ + (curPartNumQ >> 2)));
            memset(baseCTU + cuAddr + curPartNumQ + (curPartNumQ >> 1), param, (curPartNumQ >> 2));
            memset(baseCTU + cuAddr + (curPartNumQ << 1), param, (curPartNumQ + (curPartNumQ >> 2)));
            memset(baseCTU + cuAddr + (curPartNumQ << 1) + curPartNumQ + (curPartNumQ >> 1), param, (curPartNumQ >> 2));
        }
        else if (puIdx == 1)
        {
            memset(baseCTU + cuAddr, param, (curPartNumQ >> 2));
            memset(baseCTU + cuAddr + (curPartNumQ >> 1), param, (curPartNumQ >> 2));
            memset(baseCTU + cuAddr + (curPartNumQ << 1), param, (curPartNumQ >> 2));
            memset(baseCTU + cuAddr + (curPartNumQ << 1) + (curPartNumQ >> 1), param, (curPartNumQ >> 2));
        }
        else
        {
            X265_CHECK(0, "unexpected part unit index\n");
        }
        break;
    default:
        X265_CHECK(0, "unexpected part type\n");
    }
}

void TComDataCU::setChromIntraDirSubParts(uint32_t dir, uint32_t absPartIdx, uint32_t depth)
{
    uint32_t curPartNum = NUM_CU_PARTITIONS >> (depth << 1);

    memset(m_chromaIntraDir + absPartIdx, dir, sizeof(uint8_t) * curPartNum);
}

void TComDataCU::setInterDirSubParts(uint32_t dir, uint32_t absPartIdx, uint32_t partIdx, uint32_t depth)
{
    setSubPart<uint8_t>(dir, m_interDir, absPartIdx, depth, partIdx);
}

void TComDataCU::setTrIdxSubParts(uint32_t trIdx, uint32_t absPartIdx, uint32_t depth)
{
    uint32_t curPartNum = NUM_CU_PARTITIONS >> (depth << 1);

    memset(m_trIdx + absPartIdx, trIdx, sizeof(uint8_t) * curPartNum);
}

void TComDataCU::setTransformSkipSubParts(uint32_t useTransformSkipY, uint32_t useTransformSkipU, uint32_t useTransformSkipV, uint32_t absPartIdx, uint32_t depth)
{
    uint32_t curPartNum = NUM_CU_PARTITIONS >> (depth << 1);

    memset(m_transformSkip[0] + absPartIdx, useTransformSkipY, sizeof(uint8_t) * curPartNum);
    memset(m_transformSkip[1] + absPartIdx, useTransformSkipU, sizeof(uint8_t) * curPartNum);
    memset(m_transformSkip[2] + absPartIdx, useTransformSkipV, sizeof(uint8_t) * curPartNum);
}

void TComDataCU::setTransformSkipSubParts(uint32_t useTransformSkip, TextType ttype, uint32_t absPartIdx, uint32_t depth)
{
    uint32_t curPartNum = NUM_CU_PARTITIONS >> (depth << 1);

    memset(m_transformSkip[ttype] + absPartIdx, useTransformSkip, sizeof(uint8_t) * curPartNum);
}

void TComDataCU::setTransformSkipPartRange(uint32_t useTransformSkip, TextType ttype, uint32_t absPartIdx, uint32_t coveredPartIdxes)
{
    memset(m_transformSkip[ttype] + absPartIdx, useTransformSkip, sizeof(uint8_t) * coveredPartIdxes);
}

void TComDataCU::getPartIndexAndSize(uint32_t partIdx, uint32_t& outPartAddr, int& outWidth, int& outHeight) const
{
    int cuSize = 1 << m_log2CUSize[0];
    int part_mode = m_partSizes[0];
    int part_idx  = partIdx;

    int tmp = partTable[part_mode][part_idx][0];
    outWidth = ((tmp >> 4) * cuSize) >> 2;
    outHeight = ((tmp & 0xF) * cuSize) >> 2;
    outPartAddr = (partAddrTable[part_mode][part_idx] * m_numPartitions) >> 4;
}

void TComDataCU::getMvField(const TComDataCU* cu, uint32_t absPartIdx, int picList, TComMvField& outMvField) const
{
    if (!cu)  // OUT OF BOUNDARY
    {
        MV mv(0, 0);
        outMvField.setMvField(mv, NOT_VALID);
        return;
    }

    outMvField.setMvField(cu->m_cuMvField[picList].getMv(absPartIdx), cu->m_cuMvField[picList].getRefIdx(absPartIdx));
}

void TComDataCU::deriveLeftRightTopIdx(uint32_t partIdx, uint32_t& partIdxLT, uint32_t& partIdxRT) const
{
    partIdxLT = m_absIdxInCTU;
    partIdxRT = g_rasterToZscan[g_zscanToRaster[partIdxLT] + (1 << (m_log2CUSize[0] - LOG2_UNIT_SIZE)) - 1];

    switch (m_partSizes[0])
    {
    case SIZE_2Nx2N: break;
    case SIZE_2NxN:
        partIdxLT += (partIdx == 0) ? 0 : m_numPartitions >> 1;
        partIdxRT += (partIdx == 0) ? 0 : m_numPartitions >> 1;
        break;
    case SIZE_Nx2N:
        partIdxLT += (partIdx == 0) ? 0 : m_numPartitions >> 2;
        partIdxRT -= (partIdx == 1) ? 0 : m_numPartitions >> 2;
        break;
    case SIZE_NxN:
        partIdxLT += (m_numPartitions >> 2) * partIdx;
        partIdxRT +=  (m_numPartitions >> 2) * (partIdx - 1);
        break;
    case SIZE_2NxnU:
        partIdxLT += (partIdx == 0) ? 0 : m_numPartitions >> 3;
        partIdxRT += (partIdx == 0) ? 0 : m_numPartitions >> 3;
        break;
    case SIZE_2NxnD:
        partIdxLT += (partIdx == 0) ? 0 : (m_numPartitions >> 1) + (m_numPartitions >> 3);
        partIdxRT += (partIdx == 0) ? 0 : (m_numPartitions >> 1) + (m_numPartitions >> 3);
        break;
    case SIZE_nLx2N:
        partIdxLT += (partIdx == 0) ? 0 : m_numPartitions >> 4;
        partIdxRT -= (partIdx == 1) ? 0 : (m_numPartitions >> 2) + (m_numPartitions >> 4);
        break;
    case SIZE_nRx2N:
        partIdxLT += (partIdx == 0) ? 0 : (m_numPartitions >> 2) + (m_numPartitions >> 4);
        partIdxRT -= (partIdx == 1) ? 0 : m_numPartitions >> 4;
        break;
    default:
        X265_CHECK(0, "unexpected part index\n");
        break;
    }
}

void TComDataCU::deriveLeftBottomIdx(uint32_t partIdx, uint32_t& outPartIdxLB) const
{
    outPartIdxLB = g_rasterToZscan[g_zscanToRaster[m_absIdxInCTU] + ((1 << (m_log2CUSize[0] - LOG2_UNIT_SIZE - 1)) - 1) * m_frame->m_picSym->getNumPartInCUSize()];

    switch (m_partSizes[0])
    {
    case SIZE_2Nx2N:
        outPartIdxLB += m_numPartitions >> 1;
        break;
    case SIZE_2NxN:
        outPartIdxLB += (partIdx == 0) ? 0 : m_numPartitions >> 1;
        break;
    case SIZE_Nx2N:
        outPartIdxLB += (partIdx == 0) ? m_numPartitions >> 1 : (m_numPartitions >> 2) * 3;
        break;
    case SIZE_NxN:
        outPartIdxLB += (m_numPartitions >> 2) * partIdx;
        break;
    case SIZE_2NxnU:
        outPartIdxLB += (partIdx == 0) ? -((int)m_numPartitions >> 3) : m_numPartitions >> 1;
        break;
    case SIZE_2NxnD:
        outPartIdxLB += (partIdx == 0) ? (m_numPartitions >> 2) + (m_numPartitions >> 3) : m_numPartitions >> 1;
        break;
    case SIZE_nLx2N:
        outPartIdxLB += (partIdx == 0) ? m_numPartitions >> 1 : (m_numPartitions >> 1) + (m_numPartitions >> 4);
        break;
    case SIZE_nRx2N:
        outPartIdxLB += (partIdx == 0) ? m_numPartitions >> 1 : (m_numPartitions >> 1) + (m_numPartitions >> 2) + (m_numPartitions >> 4);
        break;
    default:
        X265_CHECK(0, "unexpected part index\n");
        break;
    }
}

/** Derives the partition index of neighboring bottom right block
 * \param [in]  cuMode
 * \param [in]  partIdx
 * \param [out] outPartIdxRB
 */
void TComDataCU::deriveRightBottomIdx(uint32_t partIdx, uint32_t& outPartIdxRB) const
{
    outPartIdxRB = g_rasterToZscan[g_zscanToRaster[m_absIdxInCTU] +
                                   ((1 << (m_log2CUSize[0] - LOG2_UNIT_SIZE - 1)) - 1) * m_frame->m_picSym->getNumPartInCUSize() +
                                   (1 << (m_log2CUSize[0] - LOG2_UNIT_SIZE)) - 1];

    switch (m_partSizes[0])
    {
    case SIZE_2Nx2N:
        outPartIdxRB += m_numPartitions >> 1;
        break;
    case SIZE_2NxN:
        outPartIdxRB += (partIdx == 0) ? 0 : m_numPartitions >> 1;
        break;
    case SIZE_Nx2N:
        outPartIdxRB += (partIdx == 0) ? m_numPartitions >> 2 : (m_numPartitions >> 1);
        break;
    case SIZE_NxN:
        outPartIdxRB += (m_numPartitions >> 2) * (partIdx - 1);
        break;
    case SIZE_2NxnU:
        outPartIdxRB += (partIdx == 0) ? -((int)m_numPartitions >> 3) : m_numPartitions >> 1;
        break;
    case SIZE_2NxnD:
        outPartIdxRB += (partIdx == 0) ? (m_numPartitions >> 2) + (m_numPartitions >> 3) : m_numPartitions >> 1;
        break;
    case SIZE_nLx2N:
        outPartIdxRB += (partIdx == 0) ? (m_numPartitions >> 3) + (m_numPartitions >> 4) : m_numPartitions >> 1;
        break;
    case SIZE_nRx2N:
        outPartIdxRB += (partIdx == 0) ? (m_numPartitions >> 2) + (m_numPartitions >> 3) + (m_numPartitions >> 4) : m_numPartitions >> 1;
        break;
    default:
        X265_CHECK(0, "unexpected part index\n");
        break;
    }
}

void TComDataCU::deriveLeftRightTopIdxAdi(uint32_t& outPartIdxLT, uint32_t& outPartIdxRT, uint32_t partOffset, uint32_t partDepth) const
{
    uint32_t numPartInWidth = 1 << (m_log2CUSize[0] - LOG2_UNIT_SIZE - partDepth);

    outPartIdxLT = m_absIdxInCTU + partOffset;
    outPartIdxRT = g_rasterToZscan[g_zscanToRaster[outPartIdxLT] + numPartInWidth - 1];
}

bool TComDataCU::hasEqualMotion(uint32_t absPartIdx, const TComDataCU* candCU, uint32_t candAbsPartIdx) const
{
    if (m_interDir[absPartIdx] != candCU->m_interDir[candAbsPartIdx])
        return false;

    for (uint32_t refListIdx = 0; refListIdx < 2; refListIdx++)
    {
        if (m_interDir[absPartIdx] & (1 << refListIdx))
        {
            if (m_cuMvField[refListIdx].getMv(absPartIdx) != candCU->m_cuMvField[refListIdx].getMv(candAbsPartIdx) ||
                m_cuMvField[refListIdx].getRefIdx(absPartIdx) != candCU->m_cuMvField[refListIdx].getRefIdx(candAbsPartIdx))
                return false;
        }
    }

    return true;
}

/** Constructs a list of merging candidates
 * \param absPartIdx
 * \param puIdx
 * \param depth
 * \param mvFieldNeighbours
 * \param interDirNeighbours
 * \param maxNumMergeCand
 */
uint32_t TComDataCU::getInterMergeCandidates(uint32_t absPartIdx, uint32_t puIdx, TComMvField(*mvFieldNeighbours)[2], uint8_t* interDirNeighbours) const
{
    uint32_t absPartAddr = m_absIdxInCTU + absPartIdx;
    const bool isInterB = m_slice->isInterB();

    const uint32_t maxNumMergeCand = m_slice->m_maxNumMergeCand;

    for (uint32_t i = 0; i < maxNumMergeCand; ++i)
    {
        mvFieldNeighbours[i][0].refIdx = NOT_VALID;
        mvFieldNeighbours[i][1].refIdx = NOT_VALID;
    }

    // compute the location of the current PU
    int xP, yP, nPSW, nPSH;
    this->getPartPosition(puIdx, xP, yP, nPSW, nPSH);

    uint32_t count = 0;

    uint32_t partIdxLT, partIdxRT, partIdxLB;
    PartSize curPS = (PartSize)m_partSizes[absPartIdx];
    deriveLeftBottomIdx(puIdx, partIdxLB);

    //left
    uint32_t leftPartIdx = 0;
    const TComDataCU* cuLeft = getPULeft(leftPartIdx, partIdxLB);
    bool isAvailableA1 = cuLeft &&
        cuLeft->isDiffMER(xP - 1, yP + nPSH - 1, xP, yP) &&
        !(puIdx == 1 && (curPS == SIZE_Nx2N || curPS == SIZE_nLx2N || curPS == SIZE_nRx2N)) &&
        !cuLeft->isIntra(leftPartIdx);
    if (isAvailableA1)
    {
        // get Inter Dir
        interDirNeighbours[count] = cuLeft->m_interDir[leftPartIdx];
        // get Mv from Left
        cuLeft->getMvField(cuLeft, leftPartIdx, REF_PIC_LIST_0, mvFieldNeighbours[count][0]);
        if (isInterB)
            cuLeft->getMvField(cuLeft, leftPartIdx, REF_PIC_LIST_1, mvFieldNeighbours[count][1]);

        count++;
    
        if (count == maxNumMergeCand)
            return maxNumMergeCand;
    }

    deriveLeftRightTopIdx(puIdx, partIdxLT, partIdxRT);

    // above
    uint32_t abovePartIdx = 0;
    const TComDataCU* cuAbove = getPUAbove(abovePartIdx, partIdxRT);
    bool isAvailableB1 = cuAbove &&
        cuAbove->isDiffMER(xP + nPSW - 1, yP - 1, xP, yP) &&
        !(puIdx == 1 && (curPS == SIZE_2NxN || curPS == SIZE_2NxnU || curPS == SIZE_2NxnD)) &&
        !cuAbove->isIntra(abovePartIdx);
    if (isAvailableB1 && (!isAvailableA1 || !cuLeft->hasEqualMotion(leftPartIdx, cuAbove, abovePartIdx)))
    {
        // get Inter Dir
        interDirNeighbours[count] = cuAbove->m_interDir[abovePartIdx];
        // get Mv from Left
        cuAbove->getMvField(cuAbove, abovePartIdx, REF_PIC_LIST_0, mvFieldNeighbours[count][0]);
        if (isInterB)
            cuAbove->getMvField(cuAbove, abovePartIdx, REF_PIC_LIST_1, mvFieldNeighbours[count][1]);

        count++;
   
        if (count == maxNumMergeCand)
            return maxNumMergeCand;
    }

    // above right
    uint32_t aboveRightPartIdx = 0;
    const TComDataCU* cuAboveRight = getPUAboveRight(aboveRightPartIdx, partIdxRT);
    bool isAvailableB0 = cuAboveRight &&
        cuAboveRight->isDiffMER(xP + nPSW, yP - 1, xP, yP) &&
        !cuAboveRight->isIntra(aboveRightPartIdx);
    if (isAvailableB0 && (!isAvailableB1 || !cuAbove->hasEqualMotion(abovePartIdx, cuAboveRight, aboveRightPartIdx)))
    {
        // get Inter Dir
        interDirNeighbours[count] = cuAboveRight->m_interDir[aboveRightPartIdx];
        // get Mv from Left
        cuAboveRight->getMvField(cuAboveRight, aboveRightPartIdx, REF_PIC_LIST_0, mvFieldNeighbours[count][0]);
        if (isInterB)
            cuAboveRight->getMvField(cuAboveRight, aboveRightPartIdx, REF_PIC_LIST_1, mvFieldNeighbours[count][1]);

        count++;

        if (count == maxNumMergeCand)
            return maxNumMergeCand;
    }

    // left bottom
    uint32_t leftBottomPartIdx = 0;
    const TComDataCU* cuLeftBottom = this->getPUBelowLeft(leftBottomPartIdx, partIdxLB);
    bool isAvailableA0 = cuLeftBottom &&
        cuLeftBottom->isDiffMER(xP - 1, yP + nPSH, xP, yP) &&
        !cuLeftBottom->isIntra(leftBottomPartIdx);
    if (isAvailableA0 && (!isAvailableA1 || !cuLeft->hasEqualMotion(leftPartIdx, cuLeftBottom, leftBottomPartIdx)))
    {
        // get Inter Dir
        interDirNeighbours[count] = cuLeftBottom->m_interDir[leftBottomPartIdx];
        // get Mv from Left
        cuLeftBottom->getMvField(cuLeftBottom, leftBottomPartIdx, REF_PIC_LIST_0, mvFieldNeighbours[count][0]);
        if (isInterB)
            cuLeftBottom->getMvField(cuLeftBottom, leftBottomPartIdx, REF_PIC_LIST_1, mvFieldNeighbours[count][1]);

        count++;

        if (count == maxNumMergeCand)
            return maxNumMergeCand;
    }

    // above left
    if (count < 4)
    {
        uint32_t aboveLeftPartIdx = 0;
        const TComDataCU* cuAboveLeft = getPUAboveLeft(aboveLeftPartIdx, absPartAddr);
        bool isAvailableB2 = cuAboveLeft &&
            cuAboveLeft->isDiffMER(xP - 1, yP - 1, xP, yP) &&
            !cuAboveLeft->isIntra(aboveLeftPartIdx);
        if (isAvailableB2 && (!isAvailableA1 || !cuLeft->hasEqualMotion(leftPartIdx, cuAboveLeft, aboveLeftPartIdx))
            && (!isAvailableB1 || !cuAbove->hasEqualMotion(abovePartIdx, cuAboveLeft, aboveLeftPartIdx)))
        {
            // get Inter Dir
            interDirNeighbours[count] = cuAboveLeft->m_interDir[aboveLeftPartIdx];
            // get Mv from Left
            cuAboveLeft->getMvField(cuAboveLeft, aboveLeftPartIdx, REF_PIC_LIST_0, mvFieldNeighbours[count][0]);
            if (isInterB)
                cuAboveLeft->getMvField(cuAboveLeft, aboveLeftPartIdx, REF_PIC_LIST_1, mvFieldNeighbours[count][1]);

            count++;

            if (count == maxNumMergeCand)
                return maxNumMergeCand;
        }
    }
    // TMVP always enabled
    {
        MV colmv;
        uint32_t partIdxRB;

        deriveRightBottomIdx(puIdx, partIdxRB);

        int ctuIdx = -1;

        // image boundary check
        if (m_frame->m_picSym->getCU(m_cuAddr)->m_cuPelX + g_zscanToPelX[partIdxRB] + UNIT_SIZE < m_slice->m_sps->picWidthInLumaSamples &&
            m_frame->m_picSym->getCU(m_cuAddr)->m_cuPelY + g_zscanToPelY[partIdxRB] + UNIT_SIZE < m_slice->m_sps->picHeightInLumaSamples)
        {
            uint32_t absPartIdxRB = g_zscanToRaster[partIdxRB];
            uint32_t numPartInCUSize = m_frame->m_picSym->getNumPartInCUSize();
            bool bNotLastCol = RasterAddress::lessThanCol(absPartIdxRB, numPartInCUSize - 1, numPartInCUSize); // is not at the last column of CTU
            bool bNotLastRow = RasterAddress::lessThanRow(absPartIdxRB, numPartInCUSize - 1, numPartInCUSize); // is not at the last row    of CTU

            if (bNotLastCol && bNotLastRow)
            {
                absPartAddr = g_rasterToZscan[absPartIdxRB + numPartInCUSize + 1];
                ctuIdx = m_cuAddr;
            }
            else if (bNotLastCol)
                absPartAddr = g_rasterToZscan[(absPartIdxRB + numPartInCUSize + 1) & (numPartInCUSize - 1)];
            else if (bNotLastRow)
            {
                absPartAddr = g_rasterToZscan[absPartIdxRB + 1];
                ctuIdx = m_cuAddr + 1;
            }
            else // is the right bottom corner of CTU
                absPartAddr = 0;
        }

        int refIdx = 0;
        uint32_t partIdxCenter;
        uint32_t curCTUIdx = m_cuAddr;
        int dir = 0;
        deriveCenterIdx(puIdx, partIdxCenter);
        bool bExistMV = ctuIdx >= 0 && getColMVP(REF_PIC_LIST_0, ctuIdx, absPartAddr, colmv, refIdx);
        if (!bExistMV)
            bExistMV = getColMVP(REF_PIC_LIST_0, curCTUIdx, partIdxCenter, colmv, refIdx);
        if (bExistMV)
        {
            dir |= 1;
            mvFieldNeighbours[count][0].setMvField(colmv, refIdx);
        }

        if (isInterB)
        {
            bExistMV = ctuIdx >= 0 && getColMVP(REF_PIC_LIST_1, ctuIdx, absPartAddr, colmv, refIdx);
            if (!bExistMV)
                bExistMV = getColMVP(REF_PIC_LIST_1, curCTUIdx, partIdxCenter, colmv, refIdx);

            if (bExistMV)
            {
                dir |= 2;
                mvFieldNeighbours[count][1].setMvField(colmv, refIdx);
            }
        }

        if (dir != 0)
        {
            interDirNeighbours[count] = dir;

            count++;
        
            if (count == maxNumMergeCand)
                return maxNumMergeCand;
        }
    }

    if (isInterB)
    {
        const uint32_t cutoff = count * (count - 1);
        uint32_t priorityList0 = 0xEDC984; // { 0, 1, 0, 2, 1, 2, 0, 3, 1, 3, 2, 3 }
        uint32_t priorityList1 = 0xB73621; // { 1, 0, 2, 0, 2, 1, 3, 0, 3, 1, 3, 2 }

        for (uint32_t idx = 0; idx < cutoff; idx++)
        {
            int i = priorityList0 & 3;
            int j = priorityList1 & 3;
            priorityList0 >>= 2;
            priorityList1 >>= 2;

            if ((interDirNeighbours[i] & 0x1) && (interDirNeighbours[j] & 0x2))
            {
                // get Mv from cand[i] and cand[j]
                int refIdxL0 = mvFieldNeighbours[i][0].refIdx;
                int refIdxL1 = mvFieldNeighbours[j][1].refIdx;
                int refPOCL0 = m_slice->m_refPOCList[0][refIdxL0];
                int refPOCL1 = m_slice->m_refPOCList[1][refIdxL1];
                if (!(refPOCL0 == refPOCL1 && mvFieldNeighbours[i][0].mv == mvFieldNeighbours[j][1].mv))
                {
                    mvFieldNeighbours[count][0].setMvField(mvFieldNeighbours[i][0].mv, refIdxL0);
                    mvFieldNeighbours[count][1].setMvField(mvFieldNeighbours[j][1].mv, refIdxL1);
                    interDirNeighbours[count] = 3;

                    count++;

                    if (count == maxNumMergeCand)
                        return maxNumMergeCand;
                }
            }
        }
    }
    int numRefIdx = (isInterB) ? X265_MIN(m_slice->m_numRefIdx[0], m_slice->m_numRefIdx[1]) : m_slice->m_numRefIdx[0];
    int r = 0;
    int refcnt = 0;
    while (count < maxNumMergeCand)
    {
        interDirNeighbours[count] = 1;
        mvFieldNeighbours[count][0].setMvField(MV(0, 0), r);

        if (isInterB)
        {
            interDirNeighbours[count] = 3;
            mvFieldNeighbours[count][1].setMvField(MV(0, 0), r);
        }

        count++;

        if (refcnt == numRefIdx - 1)
            r = 0;
        else
        {
            ++r;
            ++refcnt;
        }
    }

    return count;
}

/** Check whether the current PU and a spatial neighboring PU are in a same ME region.
 * \param xN, xN   location of the upper-left corner pixel of a neighboring PU
 * \param xP, yP   location of the upper-left corner pixel of the current PU
 * \returns bool
 */
bool TComDataCU::isDiffMER(int xN, int yN, int xP, int yP) const
{
    uint32_t plevel = 2;

    if ((xN >> plevel) != (xP >> plevel))
        return true;
    if ((yN >> plevel) != (yP >> plevel))
        return true;
    return false;
}

/** calculate the location of upper-left corner pixel and size of the current PU.
 * \param partIdx  PU index within a CU
 * \param xP, yP   location of the upper-left corner pixel of the current PU
 * \param PSW, nPSH    size of the curren PU
 * \returns void
 */
void TComDataCU::getPartPosition(uint32_t partIdx, int& xP, int& yP, int& nPSW, int& nPSH) const
{
    int cuSize = 1 << m_log2CUSize[0];
    int part_mode = m_partSizes[0];
    int part_idx  = partIdx;

    int tmp = partTable[part_mode][part_idx][0];
    nPSW = ((tmp >> 4) * cuSize) >> 2;
    nPSH = ((tmp & 0xF) * cuSize) >> 2;

    tmp = partTable[part_mode][part_idx][1];
    xP = ((tmp >> 4) * cuSize) >> 2;
    yP = ((tmp & 0xF) * cuSize) >> 2;
}

/** Constructs a list of candidates for AMVP
 * \param partIdx
 * \param partAddr
 * \param picList
 * \param refIdx
 * \param info
 * \param mvc
 */
int TComDataCU::fillMvpCand(uint32_t partIdx, uint32_t partAddr, int picList, int refIdx, MV* amvpCand, MV* mvc) const
{
    int num = 0;

    //-- Get Spatial MV
    uint32_t partIdxLT, partIdxRT, partIdxLB;

    deriveLeftRightTopIdx(partIdx, partIdxLT, partIdxRT);
    deriveLeftBottomIdx(partIdx, partIdxLB);

    MV mv[MD_ABOVE_LEFT + 1];
    MV mvOrder[MD_ABOVE_LEFT + 1];
    bool valid[MD_ABOVE_LEFT + 1];
    bool validOrder[MD_ABOVE_LEFT + 1];

    valid[MD_BELOW_LEFT]  = addMVPCand(mv[MD_BELOW_LEFT], picList, refIdx, partIdxLB, MD_BELOW_LEFT);
    valid[MD_LEFT]        = addMVPCand(mv[MD_LEFT], picList, refIdx, partIdxLB, MD_LEFT);
    valid[MD_ABOVE_RIGHT] = addMVPCand(mv[MD_ABOVE_RIGHT], picList, refIdx, partIdxRT, MD_ABOVE_RIGHT);
    valid[MD_ABOVE]       = addMVPCand(mv[MD_ABOVE], picList, refIdx, partIdxRT, MD_ABOVE);
    valid[MD_ABOVE_LEFT]  = addMVPCand(mv[MD_ABOVE_LEFT], picList, refIdx, partIdxLT, MD_ABOVE_LEFT);

    validOrder[MD_BELOW_LEFT]  = addMVPCandOrder(mvOrder[MD_BELOW_LEFT], picList, refIdx, partIdxLB, MD_BELOW_LEFT);
    validOrder[MD_LEFT]        = addMVPCandOrder(mvOrder[MD_LEFT], picList, refIdx, partIdxLB, MD_LEFT);
    validOrder[MD_ABOVE_RIGHT] = addMVPCandOrder(mvOrder[MD_ABOVE_RIGHT], picList, refIdx, partIdxRT, MD_ABOVE_RIGHT);
    validOrder[MD_ABOVE]       = addMVPCandOrder(mvOrder[MD_ABOVE], picList, refIdx, partIdxRT, MD_ABOVE);
    validOrder[MD_ABOVE_LEFT]  = addMVPCandOrder(mvOrder[MD_ABOVE_LEFT], picList, refIdx, partIdxLT, MD_ABOVE_LEFT);

    // Left predictor search
    if (valid[MD_BELOW_LEFT])
        amvpCand[num++] = mv[MD_BELOW_LEFT];
    else if (valid[MD_LEFT])
        amvpCand[num++] = mv[MD_LEFT];
    else if (validOrder[MD_BELOW_LEFT])
        amvpCand[num++] = mvOrder[MD_BELOW_LEFT];
    else if (validOrder[MD_LEFT])
        amvpCand[num++] = mvOrder[MD_LEFT];

    bool bAddedSmvp = num > 0;

    // Above predictor search
    if (valid[MD_ABOVE_RIGHT])
        amvpCand[num++] = mv[MD_ABOVE_RIGHT];
    else if (valid[MD_ABOVE])
        amvpCand[num++] = mv[MD_ABOVE];
    else if (valid[MD_ABOVE_LEFT])
        amvpCand[num++] = mv[MD_ABOVE_LEFT];

    if (!bAddedSmvp)
    {
        if (validOrder[MD_ABOVE_RIGHT])
            amvpCand[num++] = mvOrder[MD_ABOVE_RIGHT];
        else if (validOrder[MD_ABOVE])
            amvpCand[num++] = mvOrder[MD_ABOVE];
        else if (validOrder[MD_ABOVE_LEFT])
            amvpCand[num++] = mvOrder[MD_ABOVE_LEFT];
    }

    int numMvc = 0;
    for (int dir = MD_LEFT; dir <= MD_ABOVE_LEFT; dir++)
    {
        if (valid[dir] && mv[dir].notZero())
            mvc[numMvc++] = mv[dir];

        if (validOrder[dir] && mvOrder[dir].notZero())
            mvc[numMvc++] = mvOrder[dir];
    }

    if (num == 2)
    {
        if (amvpCand[0] == amvpCand[1])
            num = 1;
        else
            /* AMVP_NUM_CANDS = 2 */
            return numMvc;
    }

    // TMVP always enabled
    {
        uint32_t absPartAddr = m_absIdxInCTU + partAddr;
        MV colmv;
        uint32_t partIdxRB;

        deriveRightBottomIdx(partIdx, partIdxRB);

        //----  co-located RightBottom Temporal Predictor (H) ---//
        int ctuIdx = -1;

        // image boundary check
        if (m_frame->m_picSym->getCU(m_cuAddr)->m_cuPelX + g_zscanToPelX[partIdxRB] + UNIT_SIZE < m_slice->m_sps->picWidthInLumaSamples &&
            m_frame->m_picSym->getCU(m_cuAddr)->m_cuPelY + g_zscanToPelY[partIdxRB] + UNIT_SIZE < m_slice->m_sps->picHeightInLumaSamples)
        {
            uint32_t absPartIdxRB = g_zscanToRaster[partIdxRB];
            uint32_t numPartInCUSize = m_frame->m_picSym->getNumPartInCUSize();
            bool bNotLastCol = RasterAddress::lessThanCol(absPartIdxRB, numPartInCUSize - 1, numPartInCUSize); // is not at the last column of CTU
            bool bNotLastRow = RasterAddress::lessThanRow(absPartIdxRB, numPartInCUSize - 1, numPartInCUSize); // is not at the last row    of CTU

            if (bNotLastCol && bNotLastRow)
            {
                absPartAddr = g_rasterToZscan[absPartIdxRB + numPartInCUSize + 1];
                ctuIdx = m_cuAddr;
            }
            else if (bNotLastCol)
                absPartAddr = g_rasterToZscan[(absPartIdxRB + numPartInCUSize + 1) & (numPartInCUSize - 1)];
            else if (bNotLastRow)
            {
                absPartAddr = g_rasterToZscan[absPartIdxRB + 1];
                ctuIdx = m_cuAddr + 1;
            }
            else // is the right bottom corner of CTU
                absPartAddr = 0;
        }
        if (ctuIdx >= 0 && getColMVP(picList, ctuIdx, absPartAddr, colmv, refIdx))
        {
            amvpCand[num++] = colmv;
            mvc[numMvc++] = colmv;
        }
        else
        {
            uint32_t partIdxCenter;
            uint32_t curCTUIdx = m_cuAddr;
            deriveCenterIdx(partIdx, partIdxCenter);
            if (getColMVP(picList, curCTUIdx, partIdxCenter, colmv, refIdx))
            {
                amvpCand[num++] = colmv;
                mvc[numMvc++] = colmv;
            }
        }
        //----  co-located RightBottom Temporal Predictor  ---//
    }

    while (num < AMVP_NUM_CANDS)
        amvpCand[num++] = 0;

    return numMvc;
}

void TComDataCU::clipMv(MV& outMV) const
{
    int mvshift = 2;
    int offset = 8;
    int xmax = (m_slice->m_sps->picWidthInLumaSamples + offset - m_cuPelX - 1) << mvshift;
    int xmin = (-(int)g_maxCUSize - offset - (int)m_cuPelX + 1) << mvshift;

    int ymax = (m_slice->m_sps->picHeightInLumaSamples + offset - m_cuPelY - 1) << mvshift;
    int ymin = (-(int)g_maxCUSize - offset - (int)m_cuPelY + 1) << mvshift;

    outMV.x = X265_MIN(xmax, X265_MAX(xmin, (int)outMV.x));
    outMV.y = X265_MIN(ymax, X265_MAX(ymin, (int)outMV.y));
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

bool TComDataCU::addMVPCand(MV& mvp, int picList, int refIdx, uint32_t partUnitIdx, MVP_DIR dir) const
{
    const TComDataCU* tmpCU = NULL;
    uint32_t idx = 0;

    switch (dir)
    {
    case MD_LEFT:
        tmpCU = getPULeft(idx, partUnitIdx);
        break;
    case MD_ABOVE:
        tmpCU = getPUAbove(idx, partUnitIdx);
        break;
    case MD_ABOVE_RIGHT:
        tmpCU = getPUAboveRight(idx, partUnitIdx);
        break;
    case MD_BELOW_LEFT:
        tmpCU = getPUBelowLeft(idx, partUnitIdx);
        break;
    case MD_ABOVE_LEFT:
        tmpCU = getPUAboveLeft(idx, partUnitIdx);
        break;
    default:
        return false;
    }

    if (!tmpCU)
        return false;

    int refPOC = m_slice->m_refPOCList[picList][refIdx];
    int otherPOC = tmpCU->m_slice->m_refPOCList[picList][tmpCU->m_cuMvField[picList].getRefIdx(idx)];
    if (tmpCU->m_cuMvField[picList].getRefIdx(idx) >= 0 && refPOC == otherPOC)
    {
        mvp = tmpCU->m_cuMvField[picList].getMv(idx);
        return true;
    }

    int refPicList2nd = 0;
    if (picList == 0)
        refPicList2nd = 1;
    else if (picList == 1)
        refPicList2nd = 0;

    int curRefPOC = m_slice->m_refPOCList[picList][refIdx];
    int neibRefPOC;

    if (tmpCU->m_cuMvField[refPicList2nd].getRefIdx(idx) >= 0)
    {
        neibRefPOC = tmpCU->m_slice->m_refPOCList[refPicList2nd][tmpCU->m_cuMvField[refPicList2nd].getRefIdx(idx)];
        if (neibRefPOC == curRefPOC)
        {
            // Same reference frame but different list
            mvp = tmpCU->m_cuMvField[refPicList2nd].getMv(idx);
            return true;
        }
    }
    return false;
}

/**
 * \param info
 * \param picList
 * \param refIdx
 * \param partUnitIdx
 * \param dir
 * \returns bool
 */
bool TComDataCU::addMVPCandOrder(MV& outMV, int picList, int refIdx, uint32_t partUnitIdx, MVP_DIR dir) const
{
    const TComDataCU* tmpCU = NULL;
    uint32_t idx = 0;

    switch (dir)
    {
    case MD_LEFT:
        tmpCU = getPULeft(idx, partUnitIdx);
        break;
    case MD_ABOVE:
        tmpCU = getPUAbove(idx, partUnitIdx);
        break;
    case MD_ABOVE_RIGHT:
        tmpCU = getPUAboveRight(idx, partUnitIdx);
        break;
    case MD_BELOW_LEFT:
        tmpCU = getPUBelowLeft(idx, partUnitIdx);
        break;
    case MD_ABOVE_LEFT:
        tmpCU = getPUAboveLeft(idx, partUnitIdx);
        break;
    default:
        return false;
    }

    if (!tmpCU)
        return false;

    int refPicList2nd = REF_PIC_LIST_0;
    if (picList == REF_PIC_LIST_0)
        refPicList2nd = REF_PIC_LIST_1;
    else if (picList == REF_PIC_LIST_1)
        refPicList2nd = REF_PIC_LIST_0;

    int curPOC = m_slice->m_poc;
    int curRefPOC = m_slice->m_refPOCList[picList][refIdx];
    int neibPOC = curPOC;
    int neibRefPOC;

    //---------------  V1 (END) ------------------//
    if (tmpCU->m_cuMvField[picList].getRefIdx(idx) >= 0)
    {
        neibRefPOC = tmpCU->m_slice->m_refPOCList[picList][tmpCU->m_cuMvField[picList].getRefIdx(idx)];
        MV mvp = tmpCU->m_cuMvField[picList].getMv(idx);

        int scale = getDistScaleFactor(curPOC, curRefPOC, neibPOC, neibRefPOC);
        if (scale == 4096)
            outMV = mvp;
        else
            outMV = scaleMv(mvp, scale);

        return true;
    }

    //---------------------- V2(END) --------------------//
    if (tmpCU->m_cuMvField[refPicList2nd].getRefIdx(idx) >= 0)
    {
        neibRefPOC = tmpCU->m_slice->m_refPOCList[refPicList2nd][tmpCU->m_cuMvField[refPicList2nd].getRefIdx(idx)];
        MV mvp = tmpCU->m_cuMvField[refPicList2nd].getMv(idx);

        int scale = getDistScaleFactor(curPOC, curRefPOC, neibPOC, neibRefPOC);
        if (scale == 4096)
            outMV = mvp;
        else
            outMV = scaleMv(mvp, scale);

        return true;
    }

    //---------------------- V3(END) --------------------//
    return false;
}

/**
 * \param picList
 * \param cuAddr
 * \param partUnitIdx
 * \param outRefIdx
 * \returns bool
 */
bool TComDataCU::getColMVP(int picList, int cuAddr, int partUnitIdx, MV& outMV, int& outRefIdx) const
{
    uint32_t absPartAddr = partUnitIdx & TMVP_UNIT_MASK;

    int colRefPicList;
    int colPOC, colRefPOC, curPOC, curRefPOC, scale;
    MV colmv;

    // use coldir.
    Frame *colPic = m_slice->m_refPicList[m_slice->isInterB() ? 1 - m_slice->m_colFromL0Flag : 0][m_slice->m_colRefIdx];
    TComDataCU *colCU = colPic->m_picSym->getCU(cuAddr);

    if (colCU->m_frame == 0 || colCU->m_partSizes[partUnitIdx] == SIZE_NONE)
        return false;

    curPOC = m_slice->m_poc;
    colPOC = colCU->m_slice->m_poc;

    if (colCU->isIntra(absPartAddr))
        return false;

    colRefPicList = m_slice->m_bCheckLDC ? picList : m_slice->m_colFromL0Flag;

    int colRefIdx = colCU->m_cuMvField[colRefPicList].getRefIdx(absPartAddr);

    if (colRefIdx < 0)
    {
        colRefPicList = 1 - colRefPicList;
        colRefIdx = colCU->m_cuMvField[colRefPicList].getRefIdx(absPartAddr);

        if (colRefIdx < 0)
            return false;
    }

    // Scale the vector.
    colRefPOC = colCU->m_slice->m_refPOCList[colRefPicList][colRefIdx];
    colmv = colCU->m_cuMvField[colRefPicList].getMv(absPartAddr);

    curRefPOC = m_slice->m_refPOCList[picList][outRefIdx];

    scale = getDistScaleFactor(curPOC, curRefPOC, colPOC, colRefPOC);
    if (scale == 4096)
        outMV = colmv;
    else
        outMV = scaleMv(colmv, scale);

    return true;
}

int TComDataCU::getDistScaleFactor(int curPOC, int curRefPOC, int colPOC, int colRefPOC) const
{
    int diffPocD = colPOC - colRefPOC;
    int diffPocB = curPOC - curRefPOC;

    if (diffPocD == diffPocB)
    {
        return 4096;
    }
    else
    {
        int tdb      = Clip3(-128, 127, diffPocB);
        int tdd      = Clip3(-128, 127, diffPocD);
        int x        = (0x4000 + abs(tdd / 2)) / tdd;
        int scale    = Clip3(-4096, 4095, (tdb * x + 32) >> 6);
        return scale;
    }
}

/**
 * \param cuMode
 * \param partIdx
 * \param outPartIdxCenter
 * \returns void
 */
void TComDataCU::deriveCenterIdx(uint32_t partIdx, uint32_t& outPartIdxCenter) const
{
    uint32_t partAddr;
    int partWidth;
    int partHeight;

    getPartIndexAndSize(partIdx, partAddr, partWidth, partHeight);

    outPartIdxCenter = m_absIdxInCTU + partAddr; // partition origin.
    outPartIdxCenter = g_rasterToZscan[g_zscanToRaster[outPartIdxCenter]
                                       + (partHeight >> (LOG2_UNIT_SIZE + 1)) * m_frame->m_picSym->getNumPartInCUSize()
                                       + (partWidth  >> (LOG2_UNIT_SIZE + 1))];
}

ScanType TComDataCU::getCoefScanIdx(uint32_t absPartIdx, uint32_t log2TrSize, bool bIsLuma, bool bIsIntra) const
{
    uint32_t dirMode;

    if (!bIsIntra)
        return SCAN_DIAG;

    //check that MDCS can be used for this TU

    if (bIsLuma)
    {
        if (log2TrSize > MDCS_LOG2_MAX_SIZE)
            return SCAN_DIAG;

        dirMode = m_lumaIntraDir[absPartIdx];
    }
    else
    {
        if (log2TrSize > (MDCS_LOG2_MAX_SIZE - m_hChromaShift))
            return SCAN_DIAG;

        dirMode = m_chromaIntraDir[absPartIdx];
        if (dirMode == DM_CHROMA_IDX)
        {
            dirMode = m_lumaIntraDir[(m_chromaFormat == X265_CSP_I444) ? absPartIdx : absPartIdx & 0xFC];
            dirMode = (m_chromaFormat == X265_CSP_I422) ? g_chroma422IntraAngleMappingTable[dirMode] : dirMode;
        }
    }

    if (abs((int)dirMode - VER_IDX) <= MDCS_ANGLE_LIMIT)
        return SCAN_HOR;
    else if (abs((int)dirMode - HOR_IDX) <= MDCS_ANGLE_LIMIT)
        return SCAN_VER;
    else
        return SCAN_DIAG;
}

void TComDataCU::getTUEntropyCodingParameters(TUEntropyCodingParameters &result, uint32_t absPartIdx, uint32_t log2TrSize, bool bIsLuma) const
{
    // set the group layout
    result.log2TrSizeCG = log2TrSize - 2;

    // set the scan orders
    result.scanType = getCoefScanIdx(absPartIdx, log2TrSize, bIsLuma, isIntra(absPartIdx));
    result.scan     = g_scanOrder[result.scanType][log2TrSize - 2];
    result.scanCG   = g_scanOrderCG[result.scanType][result.log2TrSizeCG];

    if (log2TrSize == 2)
        result.firstSignificanceMapContext = 0;
    else if (log2TrSize == 3)
    {
        result.firstSignificanceMapContext = 9;
        if (result.scanType != SCAN_DIAG && bIsLuma)
            result.firstSignificanceMapContext += 6;
    }
    else
        result.firstSignificanceMapContext = bIsLuma ? 21 : 12;
}

void TComDataCU::loadCTUData(uint32_t maxCUSize)
{
    // Initialize the coding blocks inside the CTB
    int picWidth  = m_frame->m_origPicYuv->m_picWidth;
    int picHeight = m_frame->m_origPicYuv->m_picHeight;
    for (uint32_t log2CUSize = g_log2Size[maxCUSize], rangeCUIdx = 0; log2CUSize >= MIN_LOG2_CU_SIZE; log2CUSize--)
    {
        uint32_t blockSize  = 1 << log2CUSize;
        uint32_t sbWidth    = 1 << (g_log2Size[maxCUSize] - log2CUSize);
        int32_t last_level_flag = log2CUSize == MIN_LOG2_CU_SIZE;
        for (uint32_t sb_y = 0; sb_y < sbWidth; sb_y++)
        {
            for (uint32_t sb_x = 0; sb_x < sbWidth; sb_x++)
            {
                uint32_t depth_idx = g_depthScanIdx[sb_y][sb_x];
                uint32_t cuIdx = rangeCUIdx + depth_idx;
                uint32_t child_idx = rangeCUIdx + sbWidth * sbWidth + (depth_idx << 2);
                uint32_t px = m_cuPelX + sb_x * blockSize;
                uint32_t py = m_cuPelY + sb_y * blockSize;
                int32_t present_flag = px < picWidth && py < picHeight;
                int32_t split_mandatory_flag = present_flag && !last_level_flag && (px + blockSize > picWidth || py + blockSize > picHeight);
                
                /* Offset of the luma CU in the X, Y direction in terms of pixels from the CTU origin */
                uint32_t xOffset = (sb_x * blockSize) >> 3;
                uint32_t yOffset = (sb_y * blockSize) >> 3;

                CU *cu = m_cuLocalData + cuIdx;
                cu->log2CUSize = log2CUSize;
                cu->childIdx = child_idx;
                cu->encodeIdx = g_depthScanIdx[yOffset][xOffset] * 4;
                cu->numPartitions = (NUM_CU_PARTITIONS >> ((g_maxLog2CUSize - cu->log2CUSize) * 2));
                cu->depth = g_log2Size[maxCUSize] - log2CUSize;

                cu->flags = 0;
                CU_SET_FLAG(cu->flags, CU::PRESENT, present_flag);
                CU_SET_FLAG(cu->flags, CU::SPLIT_MANDATORY | CU::SPLIT, split_mandatory_flag);
                CU_SET_FLAG(cu->flags, CU::LEAF, last_level_flag);
            }
        }
        rangeCUIdx += sbWidth * sbWidth;
    }
}

//! \}
