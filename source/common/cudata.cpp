/*****************************************************************************
 * Copyright (C) 2014 x265 project
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
#include "frame.h"
#include "framedata.h"
#include "picyuv.h"
#include "mv.h"
#include "cudata.h"

using namespace x265;

namespace {
// file private namespace

/* for all bcast* and copy* functions, dst and src are aligned to MIN(size, 32) */

void bcast1(uint8_t* dst, uint8_t val)  { dst[0] = val; }

void copy4(uint8_t* dst, uint8_t* src)  { ((uint32_t*)dst)[0] = ((uint32_t*)src)[0]; }
void bcast4(uint8_t* dst, uint8_t val)  { ((uint32_t*)dst)[0] = 0x01010101 * val; }

void copy16(uint8_t* dst, uint8_t* src) { ((uint64_t*)dst)[0] = ((uint64_t*)src)[0]; ((uint64_t*)dst)[1] = ((uint64_t*)src)[1]; }
void bcast16(uint8_t* dst, uint8_t val) { uint64_t bval = 0x0101010101010101ULL * val; ((uint64_t*)dst)[0] = bval; ((uint64_t*)dst)[1] = bval; }

void copy64(uint8_t* dst, uint8_t* src) { ((uint64_t*)dst)[0] = ((uint64_t*)src)[0]; ((uint64_t*)dst)[1] = ((uint64_t*)src)[1]; 
                                          ((uint64_t*)dst)[2] = ((uint64_t*)src)[2]; ((uint64_t*)dst)[3] = ((uint64_t*)src)[3];
                                          ((uint64_t*)dst)[4] = ((uint64_t*)src)[4]; ((uint64_t*)dst)[5] = ((uint64_t*)src)[5];
                                          ((uint64_t*)dst)[6] = ((uint64_t*)src)[6]; ((uint64_t*)dst)[7] = ((uint64_t*)src)[7]; }
void bcast64(uint8_t* dst, uint8_t val) { uint64_t bval = 0x0101010101010101ULL * val;
                                          ((uint64_t*)dst)[0] = bval; ((uint64_t*)dst)[1] = bval; ((uint64_t*)dst)[2] = bval; ((uint64_t*)dst)[3] = bval;
                                          ((uint64_t*)dst)[4] = bval; ((uint64_t*)dst)[5] = bval; ((uint64_t*)dst)[6] = bval; ((uint64_t*)dst)[7] = bval; }

/* at 256 bytes, memset/memcpy will probably use SIMD more effectively than our uint64_t hack,
 * but hand-written assembly would beat it. */
void copy256(uint8_t* dst, uint8_t* src) { memcpy(dst, src, 256); }
void bcast256(uint8_t* dst, uint8_t val) { memset(dst, val, 256); }

/* Check whether 2 addresses point to the same column */
inline bool isEqualCol(int addrA, int addrB, int numUnitsPerRow)
{
    // addrA % numUnitsPerRow == addrB % numUnitsPerRow
    return ((addrA ^ addrB) &  (numUnitsPerRow - 1)) == 0;
}

/* Check whether 2 addresses point to the same row */
inline bool isEqualRow(int addrA, int addrB, int numUnitsPerRow)
{
    // addrA / numUnitsPerRow == addrB / numUnitsPerRow
    return ((addrA ^ addrB) & ~(numUnitsPerRow - 1)) == 0;
}

/* Check whether 2 addresses point to the same row or column */
inline bool isEqualRowOrCol(int addrA, int addrB, int numUnitsPerRow)
{
    return isEqualCol(addrA, addrB, numUnitsPerRow) | isEqualRow(addrA, addrB, numUnitsPerRow);
}

/* Check whether one address points to the first column */
inline bool isZeroCol(int addr, int numUnitsPerRow)
{
    // addr % numUnitsPerRow == 0
    return (addr & (numUnitsPerRow - 1)) == 0;
}

/* Check whether one address points to the first row */
inline bool isZeroRow(int addr, int numUnitsPerRow)
{
    // addr / numUnitsPerRow == 0
    return (addr & ~(numUnitsPerRow - 1)) == 0;
}

/* Check whether one address points to a column whose index is smaller than a given value */
inline bool lessThanCol(int addr, int val, int numUnitsPerRow)
{
    // addr % numUnitsPerRow < val
    return (addr & (numUnitsPerRow - 1)) < val;
}

/* Check whether one address points to a row whose index is smaller than a given value */
inline bool lessThanRow(int addr, int val, int numUnitsPerRow)
{
    // addr / numUnitsPerRow < val
    return addr < val * numUnitsPerRow;
}

inline MV scaleMv(MV mv, int scale)
{
    int mvx = Clip3(-32768, 32767, (scale * mv.x + 127 + (scale * mv.x < 0)) >> 8);
    int mvy = Clip3(-32768, 32767, (scale * mv.y + 127 + (scale * mv.y < 0)) >> 8);

    return MV((int16_t)mvx, (int16_t)mvy);
}

// Partition table.
// First index is partitioning mode. Second index is partition index.
// Third index is 0 for partition sizes, 1 for partition offsets. The 
// sizes and offsets are encoded as two packed 4-bit values (X,Y). 
// X and Y represent 1/4 fractions of the block size.
const uint32_t partTable[8][4][2] =
{
    //        XY
    { { 0x44, 0x00 }, { 0x00, 0x00 }, { 0x00, 0x00 }, { 0x00, 0x00 } }, // SIZE_2Nx2N.
    { { 0x42, 0x00 }, { 0x42, 0x02 }, { 0x00, 0x00 }, { 0x00, 0x00 } }, // SIZE_2NxN.
    { { 0x24, 0x00 }, { 0x24, 0x20 }, { 0x00, 0x00 }, { 0x00, 0x00 } }, // SIZE_Nx2N.
    { { 0x22, 0x00 }, { 0x22, 0x20 }, { 0x22, 0x02 }, { 0x22, 0x22 } }, // SIZE_NxN.
    { { 0x41, 0x00 }, { 0x43, 0x01 }, { 0x00, 0x00 }, { 0x00, 0x00 } }, // SIZE_2NxnU.
    { { 0x43, 0x00 }, { 0x41, 0x03 }, { 0x00, 0x00 }, { 0x00, 0x00 } }, // SIZE_2NxnD.
    { { 0x14, 0x00 }, { 0x34, 0x10 }, { 0x00, 0x00 }, { 0x00, 0x00 } }, // SIZE_nLx2N.
    { { 0x34, 0x00 }, { 0x14, 0x30 }, { 0x00, 0x00 }, { 0x00, 0x00 } }  // SIZE_nRx2N.
};

// Partition Address table.
// First index is partitioning mode. Second index is partition address.
const uint32_t partAddrTable[8][4] =
{
    { 0x00, 0x00, 0x00, 0x00 }, // SIZE_2Nx2N.
    { 0x00, 0x08, 0x08, 0x08 }, // SIZE_2NxN.
    { 0x00, 0x04, 0x04, 0x04 }, // SIZE_Nx2N.
    { 0x00, 0x04, 0x08, 0x0C }, // SIZE_NxN.
    { 0x00, 0x02, 0x02, 0x02 }, // SIZE_2NxnU.
    { 0x00, 0x0A, 0x0A, 0x0A }, // SIZE_2NxnD.
    { 0x00, 0x01, 0x01, 0x01 }, // SIZE_nLx2N.
    { 0x00, 0x05, 0x05, 0x05 }  // SIZE_nRx2N.
};

}

cubcast_t CUData::s_partSet[NUM_FULL_DEPTH] = { NULL, NULL, NULL, NULL, NULL };
uint32_t CUData::s_numPartInCUSize;

CUData::CUData()
{
    memset(this, 0, sizeof(*this));
}

void CUData::initialize(const CUDataMemPool& dataPool, uint32_t depth, int csp, int instance)
{
    m_chromaFormat  = csp;
    m_hChromaShift  = CHROMA_H_SHIFT(csp);
    m_vChromaShift  = CHROMA_V_SHIFT(csp);
    m_numPartitions = NUM_CU_PARTITIONS >> (depth * 2);

    if (!s_partSet[0])
    {
        s_numPartInCUSize = 1 << g_maxFullDepth;
        switch (g_maxLog2CUSize)
        {
        case 6:
            s_partSet[0] = bcast256;
            s_partSet[1] = bcast64;
            s_partSet[2] = bcast16;
            s_partSet[3] = bcast4;
            s_partSet[4] = bcast1;
            break;
        case 5:
            s_partSet[0] = bcast64;
            s_partSet[1] = bcast16;
            s_partSet[2] = bcast4;
            s_partSet[3] = bcast1;
            s_partSet[4] = NULL;
            break;
        case 4:
            s_partSet[0] = bcast16;
            s_partSet[1] = bcast4;
            s_partSet[2] = bcast1;
            s_partSet[3] = NULL;
            s_partSet[4] = NULL;
            break;
        default:
            X265_CHECK(0, "unexpected CTU size\n");
            break;
        }
    }

    switch (m_numPartitions)
    {
    case 256: // 64x64 CU
        m_partCopy = copy256;
        m_partSet = bcast256;
        m_subPartCopy = copy64;
        m_subPartSet = bcast64;
        break;
    case 64:  // 32x32 CU
        m_partCopy = copy64;
        m_partSet = bcast64;
        m_subPartCopy = copy16;
        m_subPartSet = bcast16;
        break;
    case 16:  // 16x16 CU
        m_partCopy = copy16;
        m_partSet = bcast16;
        m_subPartCopy = copy4;
        m_subPartSet = bcast4;
        break;
    case 4:   // 8x8 CU
        m_partCopy = copy4;
        m_partSet = bcast4;
        m_subPartCopy = NULL;
        m_subPartSet = NULL;
        break;
    default:
        X265_CHECK(0, "unexpected CU partition count\n");
        break;
    }

    /* Each CU's data is layed out sequentially within the charMemBlock */
    uint8_t *charBuf = dataPool.charMemBlock + (m_numPartitions * BytesPerPartition) * instance;

    m_qp          = (char*)charBuf; charBuf += m_numPartitions;
    m_log2CUSize         = charBuf; charBuf += m_numPartitions;
    m_lumaIntraDir       = charBuf; charBuf += m_numPartitions;
    m_tqBypass           = charBuf; charBuf += m_numPartitions;
    m_refIdx[0]   = (char*)charBuf; charBuf += m_numPartitions;
    m_refIdx[1]   = (char*)charBuf; charBuf += m_numPartitions;
    m_cuDepth            = charBuf; charBuf += m_numPartitions;
    m_predMode           = charBuf; charBuf += m_numPartitions; /* the order up to here is important in initCTU() and initSubCU() */
    m_partSize           = charBuf; charBuf += m_numPartitions;
    m_mergeFlag          = charBuf; charBuf += m_numPartitions;
    m_interDir           = charBuf; charBuf += m_numPartitions;
    m_mvpIdx[0]          = charBuf; charBuf += m_numPartitions;
    m_mvpIdx[1]          = charBuf; charBuf += m_numPartitions;
    m_tuDepth            = charBuf; charBuf += m_numPartitions;
    m_transformSkip[0]   = charBuf; charBuf += m_numPartitions;
    m_transformSkip[1]   = charBuf; charBuf += m_numPartitions;
    m_transformSkip[2]   = charBuf; charBuf += m_numPartitions;
    m_cbf[0]             = charBuf; charBuf += m_numPartitions;
    m_cbf[1]             = charBuf; charBuf += m_numPartitions;
    m_cbf[2]             = charBuf; charBuf += m_numPartitions;
    m_chromaIntraDir     = charBuf; charBuf += m_numPartitions;

    X265_CHECK(charBuf == dataPool.charMemBlock + (m_numPartitions * BytesPerPartition) * (instance + 1), "CU data layout is broken\n");

    m_mv[0]  = dataPool.mvMemBlock + (instance * 4) * m_numPartitions;
    m_mv[1]  = m_mv[0] +  m_numPartitions;
    m_mvd[0] = m_mv[1] +  m_numPartitions;
    m_mvd[1] = m_mvd[0] + m_numPartitions;

    uint32_t cuSize = g_maxCUSize >> depth;
    uint32_t sizeL = cuSize * cuSize;
    uint32_t sizeC = sizeL >> (m_hChromaShift + m_vChromaShift);
    m_trCoeff[0] = dataPool.trCoeffMemBlock + instance * (sizeL + sizeC * 2);
    m_trCoeff[1] = m_trCoeff[0] + sizeL;
    m_trCoeff[2] = m_trCoeff[0] + sizeL + sizeC;
}

void CUData::initCTU(const Frame& frame, uint32_t cuAddr, int qp)
{
    m_encData       = frame.m_encData;
    m_slice         = m_encData->m_slice;
    m_cuAddr        = cuAddr;
    m_cuPelX        = (cuAddr % m_slice->m_sps->numCuInWidth) << g_maxLog2CUSize;
    m_cuPelY        = (cuAddr / m_slice->m_sps->numCuInWidth) << g_maxLog2CUSize;
    m_absIdxInCTU   = 0;
    m_numPartitions = NUM_CU_PARTITIONS;

    /* sequential memsets */
    m_partSet((uint8_t*)m_qp, (uint8_t)qp);
    m_partSet(m_log2CUSize,   (uint8_t)g_maxLog2CUSize);
    m_partSet(m_lumaIntraDir, (uint8_t)DC_IDX);
    m_partSet(m_tqBypass,     (uint8_t)frame.m_encData->m_param->bLossless);
    if (m_slice->m_sliceType != I_SLICE)
    {
        m_partSet((uint8_t*)m_refIdx[0], (uint8_t)REF_NOT_VALID);
        m_partSet((uint8_t*)m_refIdx[1], (uint8_t)REF_NOT_VALID);
    }

    X265_CHECK(!(frame.m_encData->m_param->bLossless && !m_slice->m_pps->bTransquantBypassEnabled), "lossless enabled without TQbypass in PPS\n");

    /* initialize the remaining CU data in one memset */
    memset(m_cuDepth, 0, (BytesPerPartition - 6) * m_numPartitions);

    uint32_t widthInCU = m_slice->m_sps->numCuInWidth;
    m_cuLeft = (m_cuAddr % widthInCU) ? m_encData->getPicCTU(m_cuAddr - 1) : NULL;
    m_cuAbove = (m_cuAddr / widthInCU) ? m_encData->getPicCTU(m_cuAddr - widthInCU) : NULL;
    m_cuAboveLeft = (m_cuLeft && m_cuAbove) ? m_encData->getPicCTU(m_cuAddr - widthInCU - 1) : NULL;
    m_cuAboveRight = (m_cuAbove && ((m_cuAddr % widthInCU) < (widthInCU - 1))) ? m_encData->getPicCTU(m_cuAddr - widthInCU + 1) : NULL;
}

// initialize Sub partition
void CUData::initSubCU(const CUData& ctu, const CUGeom& cuGeom)
{
    m_absIdxInCTU   = cuGeom.encodeIdx;
    m_encData       = ctu.m_encData;
    m_slice         = ctu.m_slice;
    m_cuAddr        = ctu.m_cuAddr;
    m_cuPelX        = ctu.m_cuPelX + g_zscanToPelX[cuGeom.encodeIdx];
    m_cuPelY        = ctu.m_cuPelY + g_zscanToPelY[cuGeom.encodeIdx];
    m_cuLeft        = ctu.m_cuLeft;
    m_cuAbove       = ctu.m_cuAbove;
    m_cuAboveLeft   = ctu.m_cuAboveLeft;
    m_cuAboveRight  = ctu.m_cuAboveRight;
    X265_CHECK(m_numPartitions == cuGeom.numPartitions, "initSubCU() size mismatch\n");

    /* sequential memsets */
    m_partSet((uint8_t*)m_qp, (uint8_t)ctu.m_qp[0]);
    m_partSet(m_log2CUSize,   (uint8_t)cuGeom.log2CUSize);
    m_partSet(m_lumaIntraDir, (uint8_t)DC_IDX);
    m_partSet(m_tqBypass,     (uint8_t)m_encData->m_param->bLossless);
    m_partSet((uint8_t*)m_refIdx[0], (uint8_t)REF_NOT_VALID);
    m_partSet((uint8_t*)m_refIdx[1], (uint8_t)REF_NOT_VALID);
    m_partSet(m_cuDepth,      (uint8_t)cuGeom.depth);

    /* initialize the remaining CU data in one memset */
    memset(m_predMode, 0, (BytesPerPartition - 7) * m_numPartitions);
}

/* Copy the results of a sub-part (split) CU to the parent CU */
void CUData::copyPartFrom(const CUData& subCU, const CUGeom& childGeom, uint32_t subPartIdx)
{
    X265_CHECK(subPartIdx < 4, "part unit should be less than 4\n");

    uint32_t offset = childGeom.numPartitions * subPartIdx;

    m_subPartCopy((uint8_t*)m_qp + offset, (uint8_t*)subCU.m_qp);
    m_subPartCopy(m_log2CUSize + offset, subCU.m_log2CUSize);
    m_subPartCopy(m_lumaIntraDir + offset, subCU.m_lumaIntraDir);
    m_subPartCopy(m_tqBypass + offset, subCU.m_tqBypass);
    m_subPartCopy((uint8_t*)m_refIdx[0] + offset, (uint8_t*)subCU.m_refIdx[0]);
    m_subPartCopy((uint8_t*)m_refIdx[1] + offset, (uint8_t*)subCU.m_refIdx[1]);
    m_subPartCopy(m_cuDepth + offset, subCU.m_cuDepth);
    m_subPartCopy(m_predMode + offset, subCU.m_predMode);
    m_subPartCopy(m_partSize + offset, subCU.m_partSize);
    m_subPartCopy(m_mergeFlag + offset, subCU.m_mergeFlag);
    m_subPartCopy(m_interDir + offset, subCU.m_interDir);
    m_subPartCopy(m_mvpIdx[0] + offset, subCU.m_mvpIdx[0]);
    m_subPartCopy(m_mvpIdx[1] + offset, subCU.m_mvpIdx[1]);
    m_subPartCopy(m_tuDepth + offset, subCU.m_tuDepth);
    m_subPartCopy(m_transformSkip[0] + offset, subCU.m_transformSkip[0]);
    m_subPartCopy(m_transformSkip[1] + offset, subCU.m_transformSkip[1]);
    m_subPartCopy(m_transformSkip[2] + offset, subCU.m_transformSkip[2]);
    m_subPartCopy(m_cbf[0] + offset, subCU.m_cbf[0]);
    m_subPartCopy(m_cbf[1] + offset, subCU.m_cbf[1]);
    m_subPartCopy(m_cbf[2] + offset, subCU.m_cbf[2]);
    m_subPartCopy(m_chromaIntraDir + offset, subCU.m_chromaIntraDir);

    memcpy(m_mv[0] + offset, subCU.m_mv[0], childGeom.numPartitions * sizeof(MV));
    memcpy(m_mv[1] + offset, subCU.m_mv[1], childGeom.numPartitions * sizeof(MV));
    memcpy(m_mvd[0] + offset, subCU.m_mvd[0], childGeom.numPartitions * sizeof(MV));
    memcpy(m_mvd[1] + offset, subCU.m_mvd[1], childGeom.numPartitions * sizeof(MV));

    uint32_t tmp = 1 << ((g_maxLog2CUSize - childGeom.depth) * 2);
    uint32_t tmp2 = subPartIdx * tmp;
    memcpy(m_trCoeff[0] + tmp2, subCU.m_trCoeff[0], sizeof(coeff_t) * tmp);

    uint32_t tmpC = tmp >> (m_hChromaShift + m_vChromaShift);
    uint32_t tmpC2 = tmp2 >> (m_hChromaShift + m_vChromaShift);
    memcpy(m_trCoeff[1] + tmpC2, subCU.m_trCoeff[1], sizeof(coeff_t) * tmpC);
    memcpy(m_trCoeff[2] + tmpC2, subCU.m_trCoeff[2], sizeof(coeff_t) * tmpC);
}

/* If a sub-CU part is not present (off the edge of the picture) its depth and
 * log2size should still be configured */
void CUData::setEmptyPart(const CUGeom& childGeom, uint32_t subPartIdx)
{
    uint32_t offset = childGeom.numPartitions * subPartIdx;
    m_subPartSet(m_cuDepth + offset, (uint8_t)childGeom.depth);
    m_subPartSet(m_log2CUSize + offset, (uint8_t)childGeom.log2CUSize);
}

/* Copy all CU data from one instance to the next, except set lossless flag
 * This will only get used when --cu-lossless is enabled but --lossless is not. */
void CUData::initLosslessCU(const CUData& cu, const CUGeom& cuGeom)
{
    /* Start by making an exact copy */
    m_encData      = cu.m_encData;
    m_slice        = cu.m_slice;
    m_cuAddr       = cu.m_cuAddr;
    m_cuPelX       = cu.m_cuPelX;
    m_cuPelY       = cu.m_cuPelY;
    m_cuLeft       = cu.m_cuLeft;
    m_cuAbove      = cu.m_cuAbove;
    m_cuAboveLeft  = cu.m_cuAboveLeft;
    m_cuAboveRight = cu.m_cuAboveRight;
    m_absIdxInCTU  = cuGeom.encodeIdx;
    m_numPartitions = cuGeom.numPartitions;
    memcpy(m_qp, cu.m_qp, BytesPerPartition * m_numPartitions);
    memcpy(m_mv[0],  cu.m_mv[0],  m_numPartitions * sizeof(MV));
    memcpy(m_mv[1],  cu.m_mv[1],  m_numPartitions * sizeof(MV));
    memcpy(m_mvd[0], cu.m_mvd[0], m_numPartitions * sizeof(MV));
    memcpy(m_mvd[1], cu.m_mvd[1], m_numPartitions * sizeof(MV));

    /* force TQBypass to true */
    m_partSet(m_tqBypass, true);

    /* clear residual coding flags */
    m_partSet(m_predMode, cu.m_predMode[0] & (MODE_INTRA | MODE_INTER));
    m_partSet(m_tuDepth, 0);
    m_partSet(m_transformSkip[0], 0);
    m_partSet(m_transformSkip[1], 0);
    m_partSet(m_transformSkip[2], 0);
    m_partSet(m_cbf[0], 0);
    m_partSet(m_cbf[1], 0);
    m_partSet(m_cbf[2], 0);
}

/* Copy completed predicted CU to CTU in picture */
void CUData::copyToPic(uint32_t depth) const
{
    CUData& ctu = *m_encData->getPicCTU(m_cuAddr);

    m_partCopy((uint8_t*)ctu.m_qp + m_absIdxInCTU, (uint8_t*)m_qp);
    m_partCopy(ctu.m_log2CUSize + m_absIdxInCTU, m_log2CUSize);
    m_partCopy(ctu.m_lumaIntraDir + m_absIdxInCTU, m_lumaIntraDir);
    m_partCopy(ctu.m_tqBypass + m_absIdxInCTU, m_tqBypass);
    m_partCopy((uint8_t*)ctu.m_refIdx[0] + m_absIdxInCTU, (uint8_t*)m_refIdx[0]);
    m_partCopy((uint8_t*)ctu.m_refIdx[1] + m_absIdxInCTU, (uint8_t*)m_refIdx[1]);
    m_partCopy(ctu.m_cuDepth + m_absIdxInCTU, m_cuDepth);
    m_partCopy(ctu.m_predMode + m_absIdxInCTU, m_predMode);
    m_partCopy(ctu.m_partSize + m_absIdxInCTU, m_partSize);
    m_partCopy(ctu.m_mergeFlag + m_absIdxInCTU, m_mergeFlag);
    m_partCopy(ctu.m_interDir + m_absIdxInCTU, m_interDir);
    m_partCopy(ctu.m_mvpIdx[0] + m_absIdxInCTU, m_mvpIdx[0]);
    m_partCopy(ctu.m_mvpIdx[1] + m_absIdxInCTU, m_mvpIdx[1]);
    m_partCopy(ctu.m_tuDepth + m_absIdxInCTU, m_tuDepth);
    m_partCopy(ctu.m_transformSkip[0] + m_absIdxInCTU, m_transformSkip[0]);
    m_partCopy(ctu.m_transformSkip[1] + m_absIdxInCTU, m_transformSkip[1]);
    m_partCopy(ctu.m_transformSkip[2] + m_absIdxInCTU, m_transformSkip[2]);
    m_partCopy(ctu.m_cbf[0] + m_absIdxInCTU, m_cbf[0]);
    m_partCopy(ctu.m_cbf[1] + m_absIdxInCTU, m_cbf[1]);
    m_partCopy(ctu.m_cbf[2] + m_absIdxInCTU, m_cbf[2]);
    m_partCopy(ctu.m_chromaIntraDir + m_absIdxInCTU, m_chromaIntraDir);

    memcpy(ctu.m_mv[0] + m_absIdxInCTU,  m_mv[0],  m_numPartitions * sizeof(MV));
    memcpy(ctu.m_mv[1] + m_absIdxInCTU,  m_mv[1],  m_numPartitions * sizeof(MV));
    memcpy(ctu.m_mvd[0] + m_absIdxInCTU, m_mvd[0], m_numPartitions * sizeof(MV));
    memcpy(ctu.m_mvd[1] + m_absIdxInCTU, m_mvd[1], m_numPartitions * sizeof(MV));

    uint32_t tmpY = 1 << ((g_maxLog2CUSize - depth) * 2);
    uint32_t tmpY2 = m_absIdxInCTU << (LOG2_UNIT_SIZE * 2);
    memcpy(ctu.m_trCoeff[0] + tmpY2, m_trCoeff[0], sizeof(coeff_t) * tmpY);

    uint32_t tmpC = tmpY >> (m_hChromaShift + m_vChromaShift);
    uint32_t tmpC2 = tmpY2 >> (m_hChromaShift + m_vChromaShift);
    memcpy(ctu.m_trCoeff[1] + tmpC2, m_trCoeff[1], sizeof(coeff_t) * tmpC);
    memcpy(ctu.m_trCoeff[2] + tmpC2, m_trCoeff[2], sizeof(coeff_t) * tmpC);
}

/* The reverse of copyToPic, called only by encodeResidue */
void CUData::copyFromPic(const CUData& ctu, const CUGeom& cuGeom)
{
    m_encData       = ctu.m_encData;
    m_slice         = ctu.m_slice;
    m_cuAddr        = ctu.m_cuAddr;
    m_cuPelX        = ctu.m_cuPelX + g_zscanToPelX[cuGeom.encodeIdx];
    m_cuPelY        = ctu.m_cuPelY + g_zscanToPelY[cuGeom.encodeIdx];
    m_absIdxInCTU   = cuGeom.encodeIdx;
    m_numPartitions = cuGeom.numPartitions;

    /* copy out all prediction info for this part */
    m_partCopy((uint8_t*)m_qp, (uint8_t*)ctu.m_qp + m_absIdxInCTU);
    m_partCopy(m_log2CUSize,   ctu.m_log2CUSize + m_absIdxInCTU);
    m_partCopy(m_lumaIntraDir, ctu.m_lumaIntraDir + m_absIdxInCTU);
    m_partCopy(m_tqBypass,     ctu.m_tqBypass + m_absIdxInCTU);
    m_partCopy((uint8_t*)m_refIdx[0], (uint8_t*)ctu.m_refIdx[0] + m_absIdxInCTU);
    m_partCopy((uint8_t*)m_refIdx[1], (uint8_t*)ctu.m_refIdx[1] + m_absIdxInCTU);
    m_partCopy(m_cuDepth,      ctu.m_cuDepth + m_absIdxInCTU);
    m_partSet(m_predMode, ctu.m_predMode[m_absIdxInCTU] & (MODE_INTRA | MODE_INTER)); /* clear skip flag */
    m_partCopy(m_partSize,     ctu.m_partSize + m_absIdxInCTU);
    m_partCopy(m_mergeFlag,    ctu.m_mergeFlag + m_absIdxInCTU);
    m_partCopy(m_interDir,     ctu.m_interDir + m_absIdxInCTU);
    m_partCopy(m_mvpIdx[0],    ctu.m_mvpIdx[0] + m_absIdxInCTU);
    m_partCopy(m_mvpIdx[1],    ctu.m_mvpIdx[1] + m_absIdxInCTU);
    m_partCopy(m_chromaIntraDir, ctu.m_chromaIntraDir + m_absIdxInCTU);

    memcpy(m_mv[0],  ctu.m_mv[0] + m_absIdxInCTU,  m_numPartitions * sizeof(MV));
    memcpy(m_mv[1],  ctu.m_mv[1] + m_absIdxInCTU,  m_numPartitions * sizeof(MV));
    memcpy(m_mvd[0], ctu.m_mvd[0] + m_absIdxInCTU, m_numPartitions * sizeof(MV));
    memcpy(m_mvd[1], ctu.m_mvd[1] + m_absIdxInCTU, m_numPartitions * sizeof(MV));

    /* clear residual coding flags */
    m_partSet(m_tuDepth, 0);
    m_partSet(m_transformSkip[0], 0);
    m_partSet(m_transformSkip[1], 0);
    m_partSet(m_transformSkip[2], 0);
    m_partSet(m_cbf[0], 0);
    m_partSet(m_cbf[1], 0);
    m_partSet(m_cbf[2], 0);
}

/* Only called by encodeResidue, these fields can be modified during inter/intra coding */
void CUData::updatePic(uint32_t depth) const
{
    CUData& ctu = *m_encData->getPicCTU(m_cuAddr);

    m_partCopy((uint8_t*)ctu.m_qp + m_absIdxInCTU, (uint8_t*)m_qp);
    m_partCopy(ctu.m_transformSkip[0] + m_absIdxInCTU, m_transformSkip[0]);
    m_partCopy(ctu.m_transformSkip[1] + m_absIdxInCTU, m_transformSkip[1]);
    m_partCopy(ctu.m_transformSkip[2] + m_absIdxInCTU, m_transformSkip[2]);
    m_partCopy(ctu.m_predMode + m_absIdxInCTU, m_predMode);
    m_partCopy(ctu.m_tuDepth + m_absIdxInCTU, m_tuDepth);
    m_partCopy(ctu.m_cbf[0] + m_absIdxInCTU, m_cbf[0]);
    m_partCopy(ctu.m_cbf[1] + m_absIdxInCTU, m_cbf[1]);
    m_partCopy(ctu.m_cbf[2] + m_absIdxInCTU, m_cbf[2]);
    m_partCopy(ctu.m_chromaIntraDir + m_absIdxInCTU, m_chromaIntraDir);

    uint32_t tmpY = 1 << ((g_maxLog2CUSize - depth) * 2);
    uint32_t tmpY2 = m_absIdxInCTU << (LOG2_UNIT_SIZE * 2);
    memcpy(ctu.m_trCoeff[0] + tmpY2, m_trCoeff[0], sizeof(coeff_t) * tmpY);
    tmpY  >>= m_hChromaShift + m_vChromaShift;
    tmpY2 >>= m_hChromaShift + m_vChromaShift;
    memcpy(ctu.m_trCoeff[1] + tmpY2, m_trCoeff[1], sizeof(coeff_t) * tmpY);
    memcpy(ctu.m_trCoeff[2] + tmpY2, m_trCoeff[2], sizeof(coeff_t) * tmpY);
}

const CUData* CUData::getPULeft(uint32_t& lPartUnitIdx, uint32_t curPartUnitIdx) const
{
    uint32_t absPartIdx = g_zscanToRaster[curPartUnitIdx];

    if (!isZeroCol(absPartIdx, s_numPartInCUSize))
    {
        uint32_t absZorderCUIdx   = g_zscanToRaster[m_absIdxInCTU];
        lPartUnitIdx = g_rasterToZscan[absPartIdx - 1];
        if (isEqualCol(absPartIdx, absZorderCUIdx, s_numPartInCUSize))
            return m_encData->getPicCTU(m_cuAddr);
        else
        {
            lPartUnitIdx -= m_absIdxInCTU;
            return this;
        }
    }

    lPartUnitIdx = g_rasterToZscan[absPartIdx + s_numPartInCUSize - 1];
    return m_cuLeft;
}

const CUData* CUData::getPUAbove(uint32_t& aPartUnitIdx, uint32_t curPartUnitIdx) const
{
    uint32_t absPartIdx = g_zscanToRaster[curPartUnitIdx];

    if (!isZeroRow(absPartIdx, s_numPartInCUSize))
    {
        uint32_t absZorderCUIdx = g_zscanToRaster[m_absIdxInCTU];
        aPartUnitIdx = g_rasterToZscan[absPartIdx - s_numPartInCUSize];
        if (isEqualRow(absPartIdx, absZorderCUIdx, s_numPartInCUSize))
            return m_encData->getPicCTU(m_cuAddr);
        else
            aPartUnitIdx -= m_absIdxInCTU;
        return this;
    }

    aPartUnitIdx = g_rasterToZscan[absPartIdx + NUM_CU_PARTITIONS - s_numPartInCUSize];
    return m_cuAbove;
}

const CUData* CUData::getPUAboveLeft(uint32_t& alPartUnitIdx, uint32_t curPartUnitIdx) const
{
    uint32_t absPartIdx = g_zscanToRaster[curPartUnitIdx];

    if (!isZeroCol(absPartIdx, s_numPartInCUSize))
    {
        if (!isZeroRow(absPartIdx, s_numPartInCUSize))
        {
            uint32_t absZorderCUIdx  = g_zscanToRaster[m_absIdxInCTU];
            alPartUnitIdx = g_rasterToZscan[absPartIdx - s_numPartInCUSize - 1];
            if (isEqualRowOrCol(absPartIdx, absZorderCUIdx, s_numPartInCUSize))
                return m_encData->getPicCTU(m_cuAddr);
            else
            {
                alPartUnitIdx -= m_absIdxInCTU;
                return this;
            }
        }
        alPartUnitIdx = g_rasterToZscan[absPartIdx + NUM_CU_PARTITIONS - s_numPartInCUSize - 1];
        return m_cuAbove;
    }

    if (!isZeroRow(absPartIdx, s_numPartInCUSize))
    {
        alPartUnitIdx = g_rasterToZscan[absPartIdx - 1];
        return m_cuLeft;
    }

    alPartUnitIdx = g_rasterToZscan[NUM_CU_PARTITIONS - 1];
    return m_cuAboveLeft;
}

const CUData* CUData::getPUAboveRight(uint32_t& arPartUnitIdx, uint32_t curPartUnitIdx) const
{
    if ((m_encData->getPicCTU(m_cuAddr)->m_cuPelX + g_zscanToPelX[curPartUnitIdx] + UNIT_SIZE) >= m_slice->m_sps->picWidthInLumaSamples)
        return NULL;

    uint32_t absPartIdxRT = g_zscanToRaster[curPartUnitIdx];

    if (lessThanCol(absPartIdxRT, s_numPartInCUSize - 1, s_numPartInCUSize))
    {
        if (!isZeroRow(absPartIdxRT, s_numPartInCUSize))
        {
            if (curPartUnitIdx > g_rasterToZscan[absPartIdxRT - s_numPartInCUSize + 1])
            {
                uint32_t absZorderCUIdx  = g_zscanToRaster[m_absIdxInCTU] + (1 << (m_log2CUSize[0] - LOG2_UNIT_SIZE)) - 1;
                arPartUnitIdx = g_rasterToZscan[absPartIdxRT - s_numPartInCUSize + 1];
                if (isEqualRowOrCol(absPartIdxRT, absZorderCUIdx, s_numPartInCUSize))
                    return m_encData->getPicCTU(m_cuAddr);
                else
                {
                    arPartUnitIdx -= m_absIdxInCTU;
                    return this;
                }
            }
            return NULL;
        }
        arPartUnitIdx = g_rasterToZscan[absPartIdxRT + NUM_CU_PARTITIONS - s_numPartInCUSize + 1];
        return m_cuAbove;
    }

    if (!isZeroRow(absPartIdxRT, s_numPartInCUSize))
        return NULL;

    arPartUnitIdx = g_rasterToZscan[NUM_CU_PARTITIONS - s_numPartInCUSize];
    return m_cuAboveRight;
}

const CUData* CUData::getPUBelowLeft(uint32_t& blPartUnitIdx, uint32_t curPartUnitIdx) const
{
    if ((m_encData->getPicCTU(m_cuAddr)->m_cuPelY + g_zscanToPelY[curPartUnitIdx] + UNIT_SIZE) >= m_slice->m_sps->picHeightInLumaSamples)
        return NULL;

    uint32_t absPartIdxLB = g_zscanToRaster[curPartUnitIdx];

    if (lessThanRow(absPartIdxLB, s_numPartInCUSize - 1, s_numPartInCUSize))
    {
        if (!isZeroCol(absPartIdxLB, s_numPartInCUSize))
        {
            if (curPartUnitIdx > g_rasterToZscan[absPartIdxLB + s_numPartInCUSize - 1])
            {
                uint32_t absZorderCUIdxLB = g_zscanToRaster[m_absIdxInCTU] + ((1 << (m_log2CUSize[0] - LOG2_UNIT_SIZE)) - 1) * s_numPartInCUSize;
                blPartUnitIdx = g_rasterToZscan[absPartIdxLB + s_numPartInCUSize - 1];
                if (isEqualRowOrCol(absPartIdxLB, absZorderCUIdxLB, s_numPartInCUSize))
                    return m_encData->getPicCTU(m_cuAddr);
                else
                {
                    blPartUnitIdx -= m_absIdxInCTU;
                    return this;
                }
            }
            return NULL;
        }
        blPartUnitIdx = g_rasterToZscan[absPartIdxLB + s_numPartInCUSize * 2 - 1];
        return m_cuLeft;
    }

    return NULL;
}

const CUData* CUData::getPUBelowLeftAdi(uint32_t& blPartUnitIdx,  uint32_t curPartUnitIdx, uint32_t partUnitOffset) const
{
    if ((m_encData->getPicCTU(m_cuAddr)->m_cuPelY + g_zscanToPelY[curPartUnitIdx] + (partUnitOffset << LOG2_UNIT_SIZE)) >= m_slice->m_sps->picHeightInLumaSamples)
        return NULL;

    uint32_t absPartIdxLB = g_zscanToRaster[curPartUnitIdx];

    if (lessThanRow(absPartIdxLB, s_numPartInCUSize - partUnitOffset, s_numPartInCUSize))
    {
        if (!isZeroCol(absPartIdxLB, s_numPartInCUSize))
        {
            if (curPartUnitIdx > g_rasterToZscan[absPartIdxLB + partUnitOffset * s_numPartInCUSize - 1])
            {
                uint32_t absZorderCUIdxLB = g_zscanToRaster[m_absIdxInCTU] + ((1 << (m_log2CUSize[0] - LOG2_UNIT_SIZE)) - 1) * s_numPartInCUSize;
                blPartUnitIdx = g_rasterToZscan[absPartIdxLB + partUnitOffset * s_numPartInCUSize - 1];
                if (isEqualRowOrCol(absPartIdxLB, absZorderCUIdxLB, s_numPartInCUSize))
                    return m_encData->getPicCTU(m_cuAddr);
                else
                {
                    blPartUnitIdx -= m_absIdxInCTU;
                    return this;
                }
            }
            return NULL;
        }
        blPartUnitIdx = g_rasterToZscan[absPartIdxLB + (1 + partUnitOffset) * s_numPartInCUSize - 1];
        if (!m_cuLeft || !m_cuLeft->m_slice)
            return NULL;
        return m_cuLeft;
    }

    return NULL;
}

const CUData* CUData::getPUAboveRightAdi(uint32_t& arPartUnitIdx, uint32_t curPartUnitIdx, uint32_t partUnitOffset) const
{
    if ((m_encData->getPicCTU(m_cuAddr)->m_cuPelX + g_zscanToPelX[curPartUnitIdx] + (partUnitOffset << LOG2_UNIT_SIZE)) >= m_slice->m_sps->picWidthInLumaSamples)
        return NULL;

    uint32_t absPartIdxRT = g_zscanToRaster[curPartUnitIdx];

    if (lessThanCol(absPartIdxRT, s_numPartInCUSize - partUnitOffset, s_numPartInCUSize))
    {
        if (!isZeroRow(absPartIdxRT, s_numPartInCUSize))
        {
            if (curPartUnitIdx > g_rasterToZscan[absPartIdxRT - s_numPartInCUSize + partUnitOffset])
            {
                uint32_t absZorderCUIdx = g_zscanToRaster[m_absIdxInCTU] + (1 << (m_log2CUSize[0] - LOG2_UNIT_SIZE)) - 1;
                arPartUnitIdx = g_rasterToZscan[absPartIdxRT - s_numPartInCUSize + partUnitOffset];
                if (isEqualRowOrCol(absPartIdxRT, absZorderCUIdx, s_numPartInCUSize))
                    return m_encData->getPicCTU(m_cuAddr);
                else
                {
                    arPartUnitIdx -= m_absIdxInCTU;
                    return this;
                }
            }
            return NULL;
        }
        arPartUnitIdx = g_rasterToZscan[absPartIdxRT + NUM_CU_PARTITIONS - s_numPartInCUSize + partUnitOffset];
        if (!m_cuAbove || !m_cuAbove->m_slice)
            return NULL;
        return m_cuAbove;
    }

    if (!isZeroRow(absPartIdxRT, s_numPartInCUSize))
        return NULL;

    arPartUnitIdx = g_rasterToZscan[NUM_CU_PARTITIONS - s_numPartInCUSize + partUnitOffset - 1];
    if ((m_cuAboveRight == NULL || m_cuAboveRight->m_slice == NULL || (m_cuAboveRight->m_cuAddr) > m_cuAddr))
        return NULL;
    return m_cuAboveRight;
}

/* Get left QpMinCu */
const CUData* CUData::getQpMinCuLeft(uint32_t& lPartUnitIdx, uint32_t curAbsIdxInCTU) const
{
    uint32_t absZorderQpMinCUIdx = curAbsIdxInCTU & (0xFF << (g_maxFullDepth - m_slice->m_pps->maxCuDQPDepth) * 2);
    uint32_t absRorderQpMinCUIdx = g_zscanToRaster[absZorderQpMinCUIdx];

    // check for left CTU boundary
    if (isZeroCol(absRorderQpMinCUIdx, s_numPartInCUSize))
        return NULL;

    // get index of left-CU relative to top-left corner of current quantization group
    lPartUnitIdx = g_rasterToZscan[absRorderQpMinCUIdx - 1];

    // return pointer to current CTU
    return m_encData->getPicCTU(m_cuAddr);
}

/* Get above QpMinCu */
const CUData* CUData::getQpMinCuAbove(uint32_t& aPartUnitIdx, uint32_t curAbsIdxInCTU) const
{
    uint32_t absZorderQpMinCUIdx = curAbsIdxInCTU & (0xFF << (g_maxFullDepth - m_slice->m_pps->maxCuDQPDepth) * 2);
    uint32_t absRorderQpMinCUIdx = g_zscanToRaster[absZorderQpMinCUIdx];

    // check for top CTU boundary
    if (isZeroRow(absRorderQpMinCUIdx, s_numPartInCUSize))
        return NULL;

    // get index of top-CU relative to top-left corner of current quantization group
    aPartUnitIdx = g_rasterToZscan[absRorderQpMinCUIdx - s_numPartInCUSize];

    // return pointer to current CTU
    return m_encData->getPicCTU(m_cuAddr);
}

/* Get reference QP from left QpMinCu or latest coded QP */
char CUData::getRefQP(uint32_t curAbsIdxInCTU) const
{
    uint32_t lPartIdx = 0, aPartIdx = 0;
    const CUData* cULeft = getQpMinCuLeft(lPartIdx, m_absIdxInCTU + curAbsIdxInCTU);
    const CUData* cUAbove = getQpMinCuAbove(aPartIdx, m_absIdxInCTU + curAbsIdxInCTU);

    return ((cULeft ? cULeft->m_qp[lPartIdx] : getLastCodedQP(curAbsIdxInCTU)) + (cUAbove ? cUAbove->m_qp[aPartIdx] : getLastCodedQP(curAbsIdxInCTU)) + 1) >> 1;
}

int CUData::getLastValidPartIdx(int absPartIdx) const
{
    int lastValidPartIdx = absPartIdx - 1;

    while (lastValidPartIdx >= 0 && m_predMode[lastValidPartIdx] == MODE_NONE)
    {
        uint32_t depth = m_cuDepth[lastValidPartIdx];
        lastValidPartIdx -= m_numPartitions >> (depth << 1);
    }

    return lastValidPartIdx;
}

char CUData::getLastCodedQP(uint32_t absPartIdx) const
{
    uint32_t quPartIdxMask = 0xFF << (g_maxFullDepth - m_slice->m_pps->maxCuDQPDepth) * 2;
    int lastValidPartIdx = getLastValidPartIdx(absPartIdx & quPartIdxMask);

    if (lastValidPartIdx >= 0)
        return m_qp[lastValidPartIdx];
    else
    {
        if (m_absIdxInCTU)
            return m_encData->getPicCTU(m_cuAddr)->getLastCodedQP(m_absIdxInCTU);
        else if (m_cuAddr > 0 && !(m_slice->m_pps->bEntropyCodingSyncEnabled && !(m_cuAddr % m_slice->m_sps->numCuInWidth)))
            return m_encData->getPicCTU(m_cuAddr - 1)->getLastCodedQP(NUM_CU_PARTITIONS);
        else
            return (char)m_slice->m_sliceQp;
    }
}

/* Get allowed chroma intra modes */
void CUData::getAllowedChromaDir(uint32_t absPartIdx, uint32_t* modeList) const
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

/* Get most probable intra modes */
int CUData::getIntraDirLumaPredictor(uint32_t absPartIdx, uint32_t* intraDirPred) const
{
    const CUData* tempCU;
    uint32_t tempPartIdx;
    uint32_t leftIntraDir, aboveIntraDir;

    // Get intra direction of left PU
    tempCU = getPULeft(tempPartIdx, m_absIdxInCTU + absPartIdx);

    leftIntraDir = (tempCU && tempCU->isIntra(tempPartIdx)) ? tempCU->m_lumaIntraDir[tempPartIdx] : DC_IDX;

    // Get intra direction of above PU
    tempCU = g_zscanToPelY[m_absIdxInCTU + absPartIdx] > 0 ? getPUAbove(tempPartIdx, m_absIdxInCTU + absPartIdx) : NULL;

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
            intraDirPred[2] = PLANAR_IDX;
        else
            intraDirPred[2] =  (leftIntraDir + aboveIntraDir) < 2 ? VER_IDX : DC_IDX;
        return 2;
    }
}

uint32_t CUData::getCtxSplitFlag(uint32_t absPartIdx, uint32_t depth) const
{
    const CUData* tempCU;
    uint32_t    tempPartIdx;
    uint32_t    ctx;

    // Get left split flag
    tempCU = getPULeft(tempPartIdx, m_absIdxInCTU + absPartIdx);
    ctx  = (tempCU) ? ((tempCU->m_cuDepth[tempPartIdx] > depth) ? 1 : 0) : 0;

    // Get above split flag
    tempCU = getPUAbove(tempPartIdx, m_absIdxInCTU + absPartIdx);
    ctx += (tempCU) ? ((tempCU->m_cuDepth[tempPartIdx] > depth) ? 1 : 0) : 0;

    return ctx;
}

void CUData::getIntraTUQtDepthRange(uint32_t tuDepthRange[2], uint32_t absPartIdx) const
{
    uint32_t log2CUSize = m_log2CUSize[absPartIdx];
    uint32_t splitFlag = m_partSize[absPartIdx] != SIZE_2Nx2N;

    tuDepthRange[0] = m_slice->m_sps->quadtreeTULog2MinSize;
    tuDepthRange[1] = m_slice->m_sps->quadtreeTULog2MaxSize;

    tuDepthRange[0] = X265_MAX(tuDepthRange[0], X265_MIN(log2CUSize - (m_slice->m_sps->quadtreeTUMaxDepthIntra - 1 + splitFlag), tuDepthRange[1]));
}

void CUData::getInterTUQtDepthRange(uint32_t tuDepthRange[2], uint32_t absPartIdx) const
{
    uint32_t log2CUSize = m_log2CUSize[absPartIdx];
    uint32_t quadtreeTUMaxDepth = m_slice->m_sps->quadtreeTUMaxDepthInter;
    uint32_t splitFlag = quadtreeTUMaxDepth == 1 && m_partSize[absPartIdx] != SIZE_2Nx2N;

    tuDepthRange[0] = m_slice->m_sps->quadtreeTULog2MinSize;
    tuDepthRange[1] = m_slice->m_sps->quadtreeTULog2MaxSize;

    tuDepthRange[0] = X265_MAX(tuDepthRange[0], X265_MIN(log2CUSize - (quadtreeTUMaxDepth - 1 + splitFlag), tuDepthRange[1]));
}

uint32_t CUData::getCtxSkipFlag(uint32_t absPartIdx) const
{
    const CUData* tempCU;
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

bool CUData::setQPSubCUs(char qp, uint32_t absPartIdx, uint32_t depth)
{
    uint32_t curPartNumb = NUM_CU_PARTITIONS >> (depth << 1);
    uint32_t curPartNumQ = curPartNumb >> 2;

    if (m_cuDepth[absPartIdx] > depth)
    {
        for (uint32_t subPartIdx = 0; subPartIdx < 4; subPartIdx++)
            if (setQPSubCUs(qp, absPartIdx + subPartIdx * curPartNumQ, depth + 1))
                return true;
    }
    else
    {
        if (getQtRootCbf(absPartIdx))
            return true;
        else
            setQPSubParts(qp, absPartIdx, depth);
    }

    return false;
}

void CUData::setPUInterDir(uint8_t dir, uint32_t absPartIdx, uint32_t puIdx)
{
    uint32_t curPartNumQ = m_numPartitions >> 2;
    X265_CHECK(puIdx < 2, "unexpected part unit index\n");

    switch (m_partSize[absPartIdx])
    {
    case SIZE_2Nx2N:
        memset(m_interDir + absPartIdx, dir, 4 * curPartNumQ);
        break;
    case SIZE_2NxN:
        memset(m_interDir + absPartIdx, dir, 2 * curPartNumQ);
        break;
    case SIZE_Nx2N:
        memset(m_interDir + absPartIdx, dir, curPartNumQ);
        memset(m_interDir + absPartIdx + 2 * curPartNumQ, dir, curPartNumQ);
        break;
    case SIZE_NxN:
        memset(m_interDir + absPartIdx, dir, curPartNumQ);
        break;
    case SIZE_2NxnU:
        if (!puIdx)
        {
            memset(m_interDir + absPartIdx, dir, (curPartNumQ >> 1));
            memset(m_interDir + absPartIdx + curPartNumQ, dir, (curPartNumQ >> 1));
        }
        else
        {
            memset(m_interDir + absPartIdx, dir, (curPartNumQ >> 1));
            memset(m_interDir + absPartIdx + curPartNumQ, dir, ((curPartNumQ >> 1) + (curPartNumQ << 1)));
        }
        break;
    case SIZE_2NxnD:
        if (!puIdx)
        {
            memset(m_interDir + absPartIdx, dir, ((curPartNumQ << 1) + (curPartNumQ >> 1)));
            memset(m_interDir + absPartIdx + (curPartNumQ << 1) + curPartNumQ, dir, (curPartNumQ >> 1));
        }
        else
        {
            memset(m_interDir + absPartIdx, dir, (curPartNumQ >> 1));
            memset(m_interDir + absPartIdx + curPartNumQ, dir, (curPartNumQ >> 1));
        }
        break;
    case SIZE_nLx2N:
        if (!puIdx)
        {
            memset(m_interDir + absPartIdx, dir, (curPartNumQ >> 2));
            memset(m_interDir + absPartIdx + (curPartNumQ >> 1), dir, (curPartNumQ >> 2));
            memset(m_interDir + absPartIdx + (curPartNumQ << 1), dir, (curPartNumQ >> 2));
            memset(m_interDir + absPartIdx + (curPartNumQ << 1) + (curPartNumQ >> 1), dir, (curPartNumQ >> 2));
        }
        else
        {
            memset(m_interDir + absPartIdx, dir, (curPartNumQ >> 2));
            memset(m_interDir + absPartIdx + (curPartNumQ >> 1), dir, (curPartNumQ + (curPartNumQ >> 2)));
            memset(m_interDir + absPartIdx + (curPartNumQ << 1), dir, (curPartNumQ >> 2));
            memset(m_interDir + absPartIdx + (curPartNumQ << 1) + (curPartNumQ >> 1), dir, (curPartNumQ + (curPartNumQ >> 2)));
        }
        break;
    case SIZE_nRx2N:
        if (!puIdx)
        {
            memset(m_interDir + absPartIdx, dir, (curPartNumQ + (curPartNumQ >> 2)));
            memset(m_interDir + absPartIdx + curPartNumQ + (curPartNumQ >> 1), dir, (curPartNumQ >> 2));
            memset(m_interDir + absPartIdx + (curPartNumQ << 1), dir, (curPartNumQ + (curPartNumQ >> 2)));
            memset(m_interDir + absPartIdx + (curPartNumQ << 1) + curPartNumQ + (curPartNumQ >> 1), dir, (curPartNumQ >> 2));
        }
        else
        {
            memset(m_interDir + absPartIdx, dir, (curPartNumQ >> 2));
            memset(m_interDir + absPartIdx + (curPartNumQ >> 1), dir, (curPartNumQ >> 2));
            memset(m_interDir + absPartIdx + (curPartNumQ << 1), dir, (curPartNumQ >> 2));
            memset(m_interDir + absPartIdx + (curPartNumQ << 1) + (curPartNumQ >> 1), dir, (curPartNumQ >> 2));
        }
        break;
    default:
        X265_CHECK(0, "unexpected part type\n");
        break;
    }
}

template<typename T>
void CUData::setAllPU(T* p, const T& val, int absPartIdx, int puIdx)
{
    int i;

    p += absPartIdx;
    int numElements = m_numPartitions;

    switch (m_partSize[absPartIdx])
    {
    case SIZE_2Nx2N:
        for (i = 0; i < numElements; i++)
            p[i] = val;
        break;

    case SIZE_2NxN:
        numElements >>= 1;
        for (i = 0; i < numElements; i++)
            p[i] = val;
        break;

    case SIZE_Nx2N:
        numElements >>= 2;
        for (i = 0; i < numElements; i++)
        {
            p[i] = val;
            p[i + 2 * numElements] = val;
        }
        break;

    case SIZE_2NxnU:
    {
        int curPartNumQ = numElements >> 2;
        if (!puIdx)
        {
            T *pT  = p;
            T *pT2 = p + curPartNumQ;
            for (i = 0; i < (curPartNumQ >> 1); i++)
            {
                pT[i] = val;
                pT2[i] = val;
            }
        }
        else
        {
            T *pT  = p;
            for (i = 0; i < (curPartNumQ >> 1); i++)
                pT[i] = val;

            pT = p + curPartNumQ;
            for (i = 0; i < ((curPartNumQ >> 1) + (curPartNumQ << 1)); i++)
                pT[i] = val;
        }
        break;
    }

    case SIZE_2NxnD:
    {
        int curPartNumQ = numElements >> 2;
        if (!puIdx)
        {
            T *pT  = p;
            for (i = 0; i < ((curPartNumQ >> 1) + (curPartNumQ << 1)); i++)
                pT[i] = val;

            pT = p + (numElements - curPartNumQ);
            for (i = 0; i < (curPartNumQ >> 1); i++)
                pT[i] = val;
        }
        else
        {
            T *pT  = p;
            T *pT2 = p + curPartNumQ;
            for (i = 0; i < (curPartNumQ >> 1); i++)
            {
                pT[i] = val;
                pT2[i] = val;
            }
        }
        break;
    }

    case SIZE_nLx2N:
    {
        int curPartNumQ = numElements >> 2;
        if (!puIdx)
        {
            T *pT  = p;
            T *pT2 = p + (curPartNumQ << 1);
            T *pT3 = p + (curPartNumQ >> 1);
            T *pT4 = p + (curPartNumQ << 1) + (curPartNumQ >> 1);

            for (i = 0; i < (curPartNumQ >> 2); i++)
            {
                pT[i] = val;
                pT2[i] = val;
                pT3[i] = val;
                pT4[i] = val;
            }
        }
        else
        {
            T *pT  = p;
            T *pT2 = p + (curPartNumQ << 1);
            for (i = 0; i < (curPartNumQ >> 2); i++)
            {
                pT[i] = val;
                pT2[i] = val;
            }

            pT  = p + (curPartNumQ >> 1);
            pT2 = p + (curPartNumQ << 1) + (curPartNumQ >> 1);
            for (i = 0; i < ((curPartNumQ >> 2) + curPartNumQ); i++)
            {
                pT[i] = val;
                pT2[i] = val;
            }
        }
        break;
    }

    case SIZE_nRx2N:
    {
        int curPartNumQ = numElements >> 2;
        if (!puIdx)
        {
            T *pT  = p;
            T *pT2 = p + (curPartNumQ << 1);
            for (i = 0; i < ((curPartNumQ >> 2) + curPartNumQ); i++)
            {
                pT[i] = val;
                pT2[i] = val;
            }

            pT  = p + curPartNumQ + (curPartNumQ >> 1);
            pT2 = p + numElements - curPartNumQ + (curPartNumQ >> 1);
            for (i = 0; i < (curPartNumQ >> 2); i++)
            {
                pT[i] = val;
                pT2[i] = val;
            }
        }
        else
        {
            T *pT  = p;
            T *pT2 = p + (curPartNumQ >> 1);
            T *pT3 = p + (curPartNumQ << 1);
            T *pT4 = p + (curPartNumQ << 1) + (curPartNumQ >> 1);
            for (i = 0; i < (curPartNumQ >> 2); i++)
            {
                pT[i] = val;
                pT2[i] = val;
                pT3[i] = val;
                pT4[i] = val;
            }
        }
        break;
    }

    case SIZE_NxN:
    default:
        X265_CHECK(0, "unknown partition type\n");
        break;
    }
}

void CUData::setPUMv(int list, const MV& mv, int absPartIdx, int puIdx)
{
    setAllPU(m_mv[list], mv, absPartIdx, puIdx);
}

void CUData::setPURefIdx(int list, char refIdx, int absPartIdx, int puIdx)
{
    setAllPU(m_refIdx[list], refIdx, absPartIdx, puIdx);
}

void CUData::getPartIndexAndSize(uint32_t partIdx, uint32_t& outPartAddr, int& outWidth, int& outHeight) const
{
    int cuSize = 1 << m_log2CUSize[0];
    int partType = m_partSize[0];

    int tmp = partTable[partType][partIdx][0];
    outWidth = ((tmp >> 4) * cuSize) >> 2;
    outHeight = ((tmp & 0xF) * cuSize) >> 2;
    outPartAddr = (partAddrTable[partType][partIdx] * m_numPartitions) >> 4;
}

void CUData::getMvField(const CUData* cu, uint32_t absPartIdx, int picList, MVField& outMvField) const
{
    if (cu)
    {
        outMvField.mv = cu->m_mv[picList][absPartIdx];
        outMvField.refIdx = cu->m_refIdx[picList][absPartIdx];
    }
    else
    {
        // OUT OF BOUNDARY
        outMvField.mv.word = 0;
        outMvField.refIdx = REF_NOT_VALID;
    }
}

void CUData::deriveLeftRightTopIdx(uint32_t partIdx, uint32_t& partIdxLT, uint32_t& partIdxRT) const
{
    partIdxLT = m_absIdxInCTU;
    partIdxRT = g_rasterToZscan[g_zscanToRaster[partIdxLT] + (1 << (m_log2CUSize[0] - LOG2_UNIT_SIZE)) - 1];

    switch (m_partSize[0])
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

uint32_t CUData::deriveLeftBottomIdx(uint32_t puIdx) const
{
    uint32_t outPartIdxLB;
    outPartIdxLB = g_rasterToZscan[g_zscanToRaster[m_absIdxInCTU] + ((1 << (m_log2CUSize[0] - LOG2_UNIT_SIZE - 1)) - 1) * s_numPartInCUSize];

    switch (m_partSize[0])
    {
    case SIZE_2Nx2N:
        outPartIdxLB += m_numPartitions >> 1;
        break;
    case SIZE_2NxN:
        outPartIdxLB += puIdx ? m_numPartitions >> 1 : 0;
        break;
    case SIZE_Nx2N:
        outPartIdxLB += puIdx ? (m_numPartitions >> 2) * 3 : m_numPartitions >> 1;
        break;
    case SIZE_NxN:
        outPartIdxLB += (m_numPartitions >> 2) * puIdx;
        break;
    case SIZE_2NxnU:
        outPartIdxLB += puIdx ? m_numPartitions >> 1 : -((int)m_numPartitions >> 3);
        break;
    case SIZE_2NxnD:
        outPartIdxLB += puIdx ? m_numPartitions >> 1 : (m_numPartitions >> 2) + (m_numPartitions >> 3);
        break;
    case SIZE_nLx2N:
        outPartIdxLB += puIdx ? (m_numPartitions >> 1) + (m_numPartitions >> 4) : m_numPartitions >> 1;
        break;
    case SIZE_nRx2N:
        outPartIdxLB += puIdx ? (m_numPartitions >> 1) + (m_numPartitions >> 2) + (m_numPartitions >> 4) : m_numPartitions >> 1;
        break;
    default:
        X265_CHECK(0, "unexpected part index\n");
        break;
    }
    return outPartIdxLB;
}

/* Derives the partition index of neighboring bottom right block */
uint32_t CUData::deriveRightBottomIdx(uint32_t puIdx) const
{
    uint32_t outPartIdxRB;
    outPartIdxRB = g_rasterToZscan[g_zscanToRaster[m_absIdxInCTU] +
                                   ((1 << (m_log2CUSize[0] - LOG2_UNIT_SIZE - 1)) - 1) * s_numPartInCUSize +
                                   (1 << (m_log2CUSize[0] - LOG2_UNIT_SIZE)) - 1];

    switch (m_partSize[0])
    {
    case SIZE_2Nx2N:
        outPartIdxRB += m_numPartitions >> 1;
        break;
    case SIZE_2NxN:
        outPartIdxRB += puIdx ? m_numPartitions >> 1 : 0;
        break;
    case SIZE_Nx2N:
        outPartIdxRB += puIdx ? m_numPartitions >> 1 : m_numPartitions >> 2;
        break;
    case SIZE_NxN:
        outPartIdxRB += (m_numPartitions >> 2) * (puIdx - 1);
        break;
    case SIZE_2NxnU:
        outPartIdxRB += puIdx ? m_numPartitions >> 1 : -((int)m_numPartitions >> 3);
        break;
    case SIZE_2NxnD:
        outPartIdxRB += puIdx ? m_numPartitions >> 1 : (m_numPartitions >> 2) + (m_numPartitions >> 3);
        break;
    case SIZE_nLx2N:
        outPartIdxRB += puIdx ? m_numPartitions >> 1 : (m_numPartitions >> 3) + (m_numPartitions >> 4);
        break;
    case SIZE_nRx2N:
        outPartIdxRB += puIdx ? m_numPartitions >> 1 : (m_numPartitions >> 2) + (m_numPartitions >> 3) + (m_numPartitions >> 4);
        break;
    default:
        X265_CHECK(0, "unexpected part index\n");
        break;
    }
    return outPartIdxRB;
}

void CUData::deriveLeftRightTopIdxAdi(uint32_t& outPartIdxLT, uint32_t& outPartIdxRT, uint32_t partOffset, uint32_t partDepth) const
{
    uint32_t numPartInWidth = 1 << (m_log2CUSize[0] - LOG2_UNIT_SIZE - partDepth);

    outPartIdxLT = m_absIdxInCTU + partOffset;
    outPartIdxRT = g_rasterToZscan[g_zscanToRaster[outPartIdxLT] + numPartInWidth - 1];
}

bool CUData::hasEqualMotion(uint32_t absPartIdx, const CUData& candCU, uint32_t candAbsPartIdx) const
{
    if (m_interDir[absPartIdx] != candCU.m_interDir[candAbsPartIdx])
        return false;

    for (uint32_t refListIdx = 0; refListIdx < 2; refListIdx++)
    {
        if (m_interDir[absPartIdx] & (1 << refListIdx))
        {
            if (m_mv[refListIdx][absPartIdx] != candCU.m_mv[refListIdx][candAbsPartIdx] ||
                m_refIdx[refListIdx][absPartIdx] != candCU.m_refIdx[refListIdx][candAbsPartIdx])
                return false;
        }
    }

    return true;
}

/* Construct list of merging candidates */
uint32_t CUData::getInterMergeCandidates(uint32_t absPartIdx, uint32_t puIdx, MVField(*mvFieldNeighbours)[2], uint8_t* interDirNeighbours) const
{
    uint32_t absPartAddr = m_absIdxInCTU + absPartIdx;
    const bool isInterB = m_slice->isInterB();

    const uint32_t maxNumMergeCand = m_slice->m_maxNumMergeCand;

    for (uint32_t i = 0; i < maxNumMergeCand; ++i)
    {
        mvFieldNeighbours[i][0].refIdx = REF_NOT_VALID;
        mvFieldNeighbours[i][1].refIdx = REF_NOT_VALID;
    }

    /* calculate the location of upper-left corner pixel and size of the current PU */
    int xP, yP, nPSW, nPSH;

    int cuSize = 1 << m_log2CUSize[0];
    int partMode = m_partSize[0];

    int tmp = partTable[partMode][puIdx][0];
    nPSW = ((tmp >> 4) * cuSize) >> 2;
    nPSH = ((tmp & 0xF) * cuSize) >> 2;

    tmp = partTable[partMode][puIdx][1];
    xP = ((tmp >> 4) * cuSize) >> 2;
    yP = ((tmp & 0xF) * cuSize) >> 2;

    uint32_t count = 0;

    uint32_t partIdxLT, partIdxRT, partIdxLB = deriveLeftBottomIdx(puIdx);
    PartSize curPS = (PartSize)m_partSize[absPartIdx];
    
    // left
    uint32_t leftPartIdx = 0;
    const CUData* cuLeft = getPULeft(leftPartIdx, partIdxLB);
    bool isAvailableA1 = cuLeft &&
        cuLeft->isDiffMER(xP - 1, yP + nPSH - 1, xP, yP) &&
        !(puIdx == 1 && (curPS == SIZE_Nx2N || curPS == SIZE_nLx2N || curPS == SIZE_nRx2N)) &&
        cuLeft->isInter(leftPartIdx);
    if (isAvailableA1)
    {
        // get Inter Dir
        interDirNeighbours[count] = cuLeft->m_interDir[leftPartIdx];
        // get Mv from Left
        cuLeft->getMvField(cuLeft, leftPartIdx, 0, mvFieldNeighbours[count][0]);
        if (isInterB)
            cuLeft->getMvField(cuLeft, leftPartIdx, 1, mvFieldNeighbours[count][1]);

        count++;
    
        if (count == maxNumMergeCand)
            return maxNumMergeCand;
    }

    deriveLeftRightTopIdx(puIdx, partIdxLT, partIdxRT);

    // above
    uint32_t abovePartIdx = 0;
    const CUData* cuAbove = getPUAbove(abovePartIdx, partIdxRT);
    bool isAvailableB1 = cuAbove &&
        cuAbove->isDiffMER(xP + nPSW - 1, yP - 1, xP, yP) &&
        !(puIdx == 1 && (curPS == SIZE_2NxN || curPS == SIZE_2NxnU || curPS == SIZE_2NxnD)) &&
        cuAbove->isInter(abovePartIdx);
    if (isAvailableB1 && (!isAvailableA1 || !cuLeft->hasEqualMotion(leftPartIdx, *cuAbove, abovePartIdx)))
    {
        // get Inter Dir
        interDirNeighbours[count] = cuAbove->m_interDir[abovePartIdx];
        // get Mv from Left
        cuAbove->getMvField(cuAbove, abovePartIdx, 0, mvFieldNeighbours[count][0]);
        if (isInterB)
            cuAbove->getMvField(cuAbove, abovePartIdx, 1, mvFieldNeighbours[count][1]);

        count++;
   
        if (count == maxNumMergeCand)
            return maxNumMergeCand;
    }

    // above right
    uint32_t aboveRightPartIdx = 0;
    const CUData* cuAboveRight = getPUAboveRight(aboveRightPartIdx, partIdxRT);
    bool isAvailableB0 = cuAboveRight &&
        cuAboveRight->isDiffMER(xP + nPSW, yP - 1, xP, yP) &&
        cuAboveRight->isInter(aboveRightPartIdx);
    if (isAvailableB0 && (!isAvailableB1 || !cuAbove->hasEqualMotion(abovePartIdx, *cuAboveRight, aboveRightPartIdx)))
    {
        // get Inter Dir
        interDirNeighbours[count] = cuAboveRight->m_interDir[aboveRightPartIdx];
        // get Mv from Left
        cuAboveRight->getMvField(cuAboveRight, aboveRightPartIdx, 0, mvFieldNeighbours[count][0]);
        if (isInterB)
            cuAboveRight->getMvField(cuAboveRight, aboveRightPartIdx, 1, mvFieldNeighbours[count][1]);

        count++;

        if (count == maxNumMergeCand)
            return maxNumMergeCand;
    }

    // left bottom
    uint32_t leftBottomPartIdx = 0;
    const CUData* cuLeftBottom = this->getPUBelowLeft(leftBottomPartIdx, partIdxLB);
    bool isAvailableA0 = cuLeftBottom &&
        cuLeftBottom->isDiffMER(xP - 1, yP + nPSH, xP, yP) &&
        cuLeftBottom->isInter(leftBottomPartIdx);
    if (isAvailableA0 && (!isAvailableA1 || !cuLeft->hasEqualMotion(leftPartIdx, *cuLeftBottom, leftBottomPartIdx)))
    {
        // get Inter Dir
        interDirNeighbours[count] = cuLeftBottom->m_interDir[leftBottomPartIdx];
        // get Mv from Left
        cuLeftBottom->getMvField(cuLeftBottom, leftBottomPartIdx, 0, mvFieldNeighbours[count][0]);
        if (isInterB)
            cuLeftBottom->getMvField(cuLeftBottom, leftBottomPartIdx, 1, mvFieldNeighbours[count][1]);

        count++;

        if (count == maxNumMergeCand)
            return maxNumMergeCand;
    }

    // above left
    if (count < 4)
    {
        uint32_t aboveLeftPartIdx = 0;
        const CUData* cuAboveLeft = getPUAboveLeft(aboveLeftPartIdx, absPartAddr);
        bool isAvailableB2 = cuAboveLeft &&
            cuAboveLeft->isDiffMER(xP - 1, yP - 1, xP, yP) &&
            cuAboveLeft->isInter(aboveLeftPartIdx);
        if (isAvailableB2 && (!isAvailableA1 || !cuLeft->hasEqualMotion(leftPartIdx, *cuAboveLeft, aboveLeftPartIdx))
            && (!isAvailableB1 || !cuAbove->hasEqualMotion(abovePartIdx, *cuAboveLeft, aboveLeftPartIdx)))
        {
            // get Inter Dir
            interDirNeighbours[count] = cuAboveLeft->m_interDir[aboveLeftPartIdx];
            // get Mv from Left
            cuAboveLeft->getMvField(cuAboveLeft, aboveLeftPartIdx, 0, mvFieldNeighbours[count][0]);
            if (isInterB)
                cuAboveLeft->getMvField(cuAboveLeft, aboveLeftPartIdx, 1, mvFieldNeighbours[count][1]);

            count++;

            if (count == maxNumMergeCand)
                return maxNumMergeCand;
        }
    }
    if (m_slice->m_sps->bTemporalMVPEnabled)
    {
        uint32_t partIdxRB = deriveRightBottomIdx(puIdx);
        MV colmv;
        int ctuIdx = -1;

        // image boundary check
        if (m_encData->getPicCTU(m_cuAddr)->m_cuPelX + g_zscanToPelX[partIdxRB] + UNIT_SIZE < m_slice->m_sps->picWidthInLumaSamples &&
            m_encData->getPicCTU(m_cuAddr)->m_cuPelY + g_zscanToPelY[partIdxRB] + UNIT_SIZE < m_slice->m_sps->picHeightInLumaSamples)
        {
            uint32_t absPartIdxRB = g_zscanToRaster[partIdxRB];
            uint32_t numPartInCUSize = s_numPartInCUSize;
            bool bNotLastCol = lessThanCol(absPartIdxRB, numPartInCUSize - 1, numPartInCUSize); // is not at the last column of CTU
            bool bNotLastRow = lessThanRow(absPartIdxRB, numPartInCUSize - 1, numPartInCUSize); // is not at the last row    of CTU

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
        uint32_t partIdxCenter = deriveCenterIdx(puIdx);
        uint32_t curCTUIdx = m_cuAddr;
        int dir = 0;
        bool bExistMV = ctuIdx >= 0 && getColMVP(colmv, refIdx, 0, ctuIdx, absPartAddr);
        if (!bExistMV)
            bExistMV = getColMVP(colmv, refIdx, 0, curCTUIdx, partIdxCenter);
        if (bExistMV)
        {
            dir |= 1;
            mvFieldNeighbours[count][0].mv = colmv;
            mvFieldNeighbours[count][0].refIdx = refIdx;
        }

        if (isInterB)
        {
            bExistMV = ctuIdx >= 0 && getColMVP(colmv, refIdx, 1, ctuIdx, absPartAddr);
            if (!bExistMV)
                bExistMV = getColMVP(colmv, refIdx, 1, curCTUIdx, partIdxCenter);

            if (bExistMV)
            {
                dir |= 2;
                mvFieldNeighbours[count][1].mv = colmv;
                mvFieldNeighbours[count][1].refIdx = refIdx;
            }
        }

        if (dir != 0)
        {
            interDirNeighbours[count] = (uint8_t)dir;

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
                    mvFieldNeighbours[count][0].mv = mvFieldNeighbours[i][0].mv;
                    mvFieldNeighbours[count][0].refIdx = refIdxL0;
                    mvFieldNeighbours[count][1].mv = mvFieldNeighbours[j][1].mv;
                    mvFieldNeighbours[count][1].refIdx = refIdxL1;
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
        mvFieldNeighbours[count][0].mv.word = 0;
        mvFieldNeighbours[count][0].refIdx = r;

        if (isInterB)
        {
            interDirNeighbours[count] = 3;
            mvFieldNeighbours[count][1].mv.word = 0;
            mvFieldNeighbours[count][1].refIdx = r;
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

/* Check whether the current PU and a spatial neighboring PU are in a same ME region */
bool CUData::isDiffMER(int xN, int yN, int xP, int yP) const
{
    uint32_t plevel = 2;

    if ((xN >> plevel) != (xP >> plevel))
        return true;
    if ((yN >> plevel) != (yP >> plevel))
        return true;
    return false;
}

/* Constructs a list of candidates for AMVP, and a larger list of motion candidates */
int CUData::fillMvpCand(uint32_t puIdx, uint32_t absPartIdx, int picList, int refIdx, MV* amvpCand, MV* mvc) const
{
    int num = 0;

    // spatial MV
    uint32_t partIdxLT, partIdxRT, partIdxLB = deriveLeftBottomIdx(puIdx);

    deriveLeftRightTopIdx(puIdx, partIdxLT, partIdxRT);

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

    if (m_slice->m_sps->bTemporalMVPEnabled)
    {
        uint32_t absPartAddr = m_absIdxInCTU + absPartIdx;
        uint32_t partIdxRB = deriveRightBottomIdx(puIdx);
        MV colmv;

        // co-located RightBottom temporal predictor (H)
        int ctuIdx = -1;

        // image boundary check
        if (m_encData->getPicCTU(m_cuAddr)->m_cuPelX + g_zscanToPelX[partIdxRB] + UNIT_SIZE < m_slice->m_sps->picWidthInLumaSamples &&
            m_encData->getPicCTU(m_cuAddr)->m_cuPelY + g_zscanToPelY[partIdxRB] + UNIT_SIZE < m_slice->m_sps->picHeightInLumaSamples)
        {
            uint32_t absPartIdxRB = g_zscanToRaster[partIdxRB];
            uint32_t numPartInCUSize = s_numPartInCUSize;
            bool bNotLastCol = lessThanCol(absPartIdxRB, numPartInCUSize - 1, numPartInCUSize); // is not at the last column of CTU
            bool bNotLastRow = lessThanRow(absPartIdxRB, numPartInCUSize - 1, numPartInCUSize); // is not at the last row    of CTU

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
        if (ctuIdx >= 0 && getColMVP(colmv, refIdx, picList, ctuIdx, absPartAddr))
        {
            amvpCand[num++] = colmv;
            mvc[numMvc++] = colmv;
        }
        else
        {
            uint32_t partIdxCenter =  deriveCenterIdx(puIdx);
            uint32_t curCTUIdx = m_cuAddr;
            if (getColMVP(colmv, refIdx, picList, curCTUIdx, partIdxCenter))
            {
                amvpCand[num++] = colmv;
                mvc[numMvc++] = colmv;
            }
        }
    }

    while (num < AMVP_NUM_CANDS)
        amvpCand[num++] = 0;

    return numMvc;
}

void CUData::clipMv(MV& outMV) const
{
    int mvshift = 2;
    int offset = 8;
    int xmax = (m_slice->m_sps->picWidthInLumaSamples + offset - m_cuPelX - 1) << mvshift;
    int xmin = (-(int)g_maxCUSize - offset - (int)m_cuPelX + 1) << mvshift;

    int ymax = (m_slice->m_sps->picHeightInLumaSamples + offset - m_cuPelY - 1) << mvshift;
    int ymin = (-(int)g_maxCUSize - offset - (int)m_cuPelY + 1) << mvshift;

    outMV.x = (int16_t)X265_MIN(xmax, X265_MAX(xmin, (int)outMV.x));
    outMV.y = (int16_t)X265_MIN(ymax, X265_MAX(ymin, (int)outMV.y));
}

bool CUData::addMVPCand(MV& mvp, int picList, int refIdx, uint32_t partUnitIdx, MVP_DIR dir) const
{
    const CUData* tmpCU = NULL;
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
    int partRefIdx = tmpCU->m_refIdx[picList][idx];
    if (partRefIdx >= 0 && refPOC == tmpCU->m_slice->m_refPOCList[picList][partRefIdx])
    {
        mvp = tmpCU->m_mv[picList][idx];
        return true;
    }

    int refPicList2nd = 0;
    if (picList == 0)
        refPicList2nd = 1;
    else if (picList == 1)
        refPicList2nd = 0;

    int curRefPOC = m_slice->m_refPOCList[picList][refIdx];
    int neibRefPOC;

    partRefIdx = tmpCU->m_refIdx[refPicList2nd][idx];
    if (partRefIdx >= 0)
    {
        neibRefPOC = tmpCU->m_slice->m_refPOCList[refPicList2nd][partRefIdx];
        if (neibRefPOC == curRefPOC)
        {
            // Same reference frame but different list
            mvp = tmpCU->m_mv[refPicList2nd][idx];
            return true;
        }
    }
    return false;
}

bool CUData::addMVPCandOrder(MV& outMV, int picList, int refIdx, uint32_t partUnitIdx, MVP_DIR dir) const
{
    const CUData* tmpCU = NULL;
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

    int refPicList2nd = 0;
    if (picList == 0)
        refPicList2nd = 1;
    else if (picList == 1)
        refPicList2nd = 0;

    int curPOC = m_slice->m_poc;
    int curRefPOC = m_slice->m_refPOCList[picList][refIdx];
    int neibPOC = curPOC;
    int neibRefPOC;

    int partRefIdx = tmpCU->m_refIdx[picList][idx];
    if (partRefIdx >= 0)
    {
        neibRefPOC = tmpCU->m_slice->m_refPOCList[picList][partRefIdx];
        MV mvp = tmpCU->m_mv[picList][idx];

        scaleMvByPOCDist(outMV, mvp, curPOC, curRefPOC, neibPOC, neibRefPOC);
        return true;
    }

    partRefIdx = tmpCU->m_refIdx[refPicList2nd][idx];
    if (partRefIdx >= 0)
    {
        neibRefPOC = tmpCU->m_slice->m_refPOCList[refPicList2nd][partRefIdx];
        MV mvp = tmpCU->m_mv[refPicList2nd][idx];

        scaleMvByPOCDist(outMV, mvp, curPOC, curRefPOC, neibPOC, neibRefPOC);
        return true;
    }

    return false;
}

bool CUData::getColMVP(MV& outMV, int& outRefIdx, int picList, int cuAddr, int partUnitIdx) const
{
    Frame *colPic = m_slice->m_refPicList[m_slice->isInterB() ? 1 - m_slice->m_colFromL0Flag : 0][m_slice->m_colRefIdx];
    CUData *colCU = colPic->m_encData->getPicCTU(cuAddr);

    if (colCU->m_predMode[partUnitIdx] == MODE_NONE)
        return false;

    uint32_t absPartAddr = partUnitIdx & TMVP_UNIT_MASK;

    if (colCU->isIntra(absPartAddr))
        return false;

    int colRefPicList = m_slice->m_bCheckLDC ? picList : m_slice->m_colFromL0Flag;

    int colRefIdx = colCU->m_refIdx[colRefPicList][absPartAddr];

    if (colRefIdx < 0)
    {
        colRefPicList = 1 - colRefPicList;
        colRefIdx = colCU->m_refIdx[colRefPicList][absPartAddr];

        if (colRefIdx < 0)
            return false;
    }

    // Scale the vector
    int colRefPOC = colCU->m_slice->m_refPOCList[colRefPicList][colRefIdx];
    int colPOC = colCU->m_slice->m_poc;
    MV colmv = colCU->m_mv[colRefPicList][absPartAddr];

    int curRefPOC = m_slice->m_refPOCList[picList][outRefIdx];
    int curPOC = m_slice->m_poc;

    scaleMvByPOCDist(outMV, colmv, curPOC, curRefPOC, colPOC, colRefPOC);
    return true;
}

void CUData::scaleMvByPOCDist(MV& outMV, const MV& inMV, int curPOC, int curRefPOC, int colPOC, int colRefPOC) const
{
    int diffPocD = colPOC - colRefPOC;
    int diffPocB = curPOC - curRefPOC;

    if (diffPocD == diffPocB)
        outMV = inMV;
    else
    {
        int tdb   = Clip3(-128, 127, diffPocB);
        int tdd   = Clip3(-128, 127, diffPocD);
        int x     = (0x4000 + abs(tdd / 2)) / tdd;
        int scale = Clip3(-4096, 4095, (tdb * x + 32) >> 6);
        outMV = scaleMv(inMV, scale);
    }
}

uint32_t CUData::deriveCenterIdx(uint32_t puIdx) const
{
    uint32_t absPartIdx;
    int puWidth, puHeight;

    getPartIndexAndSize(puIdx, absPartIdx, puWidth, puHeight);

    return g_rasterToZscan[g_zscanToRaster[m_absIdxInCTU + absPartIdx]
                           + (puHeight >> (LOG2_UNIT_SIZE + 1)) * s_numPartInCUSize
                           + (puWidth  >> (LOG2_UNIT_SIZE + 1))];
}

ScanType CUData::getCoefScanIdx(uint32_t absPartIdx, uint32_t log2TrSize, bool bIsLuma, bool bIsIntra) const
{
    uint32_t dirMode;

    if (!bIsIntra)
        return SCAN_DIAG;

    // check that MDCS can be used for this TU
    if (bIsLuma)
    {
        if (log2TrSize > MDCS_LOG2_MAX_SIZE)
            return SCAN_DIAG;

        dirMode = m_lumaIntraDir[absPartIdx];
    }
    else
    {
        if (log2TrSize > (uint32_t)(MDCS_LOG2_MAX_SIZE - m_hChromaShift))
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

void CUData::getTUEntropyCodingParameters(TUEntropyCodingParameters &result, uint32_t absPartIdx, uint32_t log2TrSize, bool bIsLuma) const
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

#define CU_SET_FLAG(bitfield, flag, value) (bitfield) = ((bitfield) & (~(flag))) | ((~((value) - 1)) & (flag))

void CUData::calcCTUGeoms(uint32_t ctuWidth, uint32_t ctuHeight, uint32_t maxCUSize, CUGeom cuDataArray[CUGeom::MAX_GEOMS])
{
    // Initialize the coding blocks inside the CTB
    for (uint32_t log2CUSize = g_log2Size[maxCUSize], rangeCUIdx = 0; log2CUSize >= MIN_LOG2_CU_SIZE; log2CUSize--)
    {
        uint32_t blockSize = 1 << log2CUSize;
        uint32_t sbWidth   = 1 << (g_log2Size[maxCUSize] - log2CUSize);
        int32_t lastLevelFlag = log2CUSize == MIN_LOG2_CU_SIZE;
        for (uint32_t sbY = 0; sbY < sbWidth; sbY++)
        {
            for (uint32_t sbX = 0; sbX < sbWidth; sbX++)
            {
                uint32_t depthIdx = g_depthScanIdx[sbY][sbX];
                uint32_t cuIdx = rangeCUIdx + depthIdx;
                uint32_t childIdx = rangeCUIdx + sbWidth * sbWidth + (depthIdx << 2);
                uint32_t px = sbX * blockSize;
                uint32_t py = sbY * blockSize;
                int32_t presentFlag = px < ctuWidth && py < ctuHeight;
                int32_t splitMandatoryFlag = presentFlag && !lastLevelFlag && (px + blockSize > ctuWidth || py + blockSize > ctuHeight);
                
                /* Offset of the luma CU in the X, Y direction in terms of pixels from the CTU origin */
                uint32_t xOffset = (sbX * blockSize) >> 3;
                uint32_t yOffset = (sbY * blockSize) >> 3;
                X265_CHECK(cuIdx < CUGeom::MAX_GEOMS, "CU geom index bug\n");

                CUGeom *cu = cuDataArray + cuIdx;
                cu->log2CUSize = log2CUSize;
                cu->childOffset = childIdx - cuIdx;
                cu->encodeIdx = g_depthScanIdx[yOffset][xOffset] * 4;
                cu->numPartitions = (NUM_CU_PARTITIONS >> ((g_maxLog2CUSize - cu->log2CUSize) * 2));
                cu->depth = g_log2Size[maxCUSize] - log2CUSize;

                cu->flags = 0;
                CU_SET_FLAG(cu->flags, CUGeom::PRESENT, presentFlag);
                CU_SET_FLAG(cu->flags, CUGeom::SPLIT_MANDATORY | CUGeom::SPLIT, splitMandatoryFlag);
                CU_SET_FLAG(cu->flags, CUGeom::LEAF, lastLevelFlag);
            }
        }
        rangeCUIdx += sbWidth * sbWidth;
    }
}
