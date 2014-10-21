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

#ifndef X265_TCOMDATACU_H
#define X265_TCOMDATACU_H

#include "common.h"
#include "slice.h"
#include "TComMotionInfo.h"
#include "TComRom.h"

namespace x265 {
// private namespace

class Frame;
class Slice;

// TU settings for entropy encoding
struct TUEntropyCodingParameters
{
    const uint16_t *scan;
    const uint16_t *scanCG;
    ScanType        scanType;
    uint32_t        log2TrSizeCG;
    uint32_t        firstSignificanceMapContext;
};

struct DataCUMemPool;

struct CU
{
    enum {
        INTRA           = 1<<0, // CU is intra predicted
        PRESENT         = 1<<1, // CU is not completely outside the frame
        SPLIT_MANDATORY = 1<<2, // CU split is mandatory if CU is inside frame and can be split
        LEAF            = 1<<3, // CU is a leaf node of the CTU
        SPLIT           = 1<<4, // CU is currently split in four child CUs.
    };
    uint32_t log2CUSize;    // Log of the CU size.
    uint32_t childOffset;   // offset of the first child CU from current CU
    uint32_t encodeIdx;     // Encoding index of this CU in terms of 4x4 blocks.
    uint32_t numPartitions; // Number of 4x4 blocks in the CU
    uint32_t depth;         // depth of this CU relative from CTU
    uint32_t flags;         // CU flags.
};

// Partition count table, index represents partitioning mode.
const uint8_t nbPartsTable[8] = { 1, 2, 2, 4, 2, 2, 2, 2 };

// Partition table.
// First index is partitioning mode. Second index is partition index.
// Third index is 0 for partition sizes, 1 for partition offsets. The 
// sizes and offsets are encoded as two packed 4-bit values (X,Y). 
// X and Y represent 1/4 fractions of the block size.
const uint8_t partTable[8][4][2] =
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
const uint8_t partAddrTable[8][4] =
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

/// CTU/CU data structure class
class TComDataCU
{
public:

    const Frame*  m_frame;
    const Slice*  m_slice;

    uint32_t      m_cuAddr;             ///< CTU address in a slice
    uint32_t      m_absIdxInCTU;        ///< absolute address of CU within a CTU in Z scan order
    uint32_t      m_cuPelX;             ///< CU position within the picture, in a pixel (X)
    uint32_t      m_cuPelY;             ///< CU position within the picture, in a pixel (Y)
    uint32_t      m_numPartitions;      ///< total number of minimum partitions in a CU (in 4x4 units)

    int           m_chromaFormat;
    int           m_hChromaShift;
    int           m_vChromaShift;

    /* Per-part data */
    char*         m_qp;                 ///< array of QP values
    uint8_t*      m_log2CUSize;         ///< array of cu log2Size
    uint8_t*      m_partSizes;          ///< array of partition sizes
    uint8_t*      m_predModes;          ///< array of prediction modes
    uint8_t*      m_lumaIntraDir;       ///< array of intra directions (luma)
    uint8_t*      m_cuTransquantBypass; ///< array of CU lossless flags
    uint8_t*      m_depth;              ///< array of depths
    uint8_t*      m_skipFlag;           ///< array of skip flags
    uint8_t*      m_bMergeFlags;        ///< array of merge flags
    uint8_t*      m_interDir;           ///< array of inter directions
    uint8_t*      m_mvpIdx[2];          ///< array of motion vector predictor candidates or merge candidate indices [0]
    uint8_t*      m_trIdx;              ///< array of transform indices
    uint8_t*      m_transformSkip[3];   ///< array of transform skipping flags
    uint8_t*      m_cbf[3];             ///< array of coded block flags (CBF)
    uint8_t*      m_chromaIntraDir;     ///< array of intra directions (chroma)
    enum { BytesPerPartition = 20 };    // combined sizeof() of all per-part data

    TComCUMvField m_cuMvField[2];       ///< array of motion vectors

    coeff_t*      m_trCoeff[3];         ///< transformed coefficient buffer

    const TComDataCU* m_cuAboveLeft;    ///< pointer of above-left neighbor CTU
    const TComDataCU* m_cuAboveRight;   ///< pointer of above-right neighbor CTU
    const TComDataCU* m_cuAbove;        ///< pointer of above neighbor CTU
    const TComDataCU* m_cuLeft;         ///< pointer of left neighbor CTU

    TComDataCU();

    void     initialize(DataCUMemPool *dataPool, MVFieldMemPool *mvPool, uint32_t numPartition, uint32_t cuSize, int csp, int instance);
    void     initCTU(const Frame& frame, uint32_t cuAddr, int qp);
    void     initSubCU(const TComDataCU& ctu, const CU& cuData);
    void     initLosslessCU(const TComDataCU& cu, const CU& cuData);
    void     loadCTUData(uint32_t maxCUSize, CU cuDataArray[104]);

    void     copyFromPic(const TComDataCU& ctu, const CU& cuData);
    void     copyPartFrom(const TComDataCU& cu, const int numPartitions, uint32_t partUnitIdx, uint32_t depth);

    void     copyToPic(uint32_t depth) const;
    void     copyToPic(uint32_t depth, uint32_t partIdx, uint32_t partDepth) const;
    void     updatePic(uint32_t depth) const;

    void     setDepthSubParts(uint32_t depth);

    void     setQPSubParts(int qp, uint32_t absPartIdx, uint32_t depth);
    void     setQPSubCUs(int qp, TComDataCU* cu, uint32_t absPartIdx, uint32_t depth, bool &foundNonZeroCbf);

    void     setPartSizeSubParts(PartSize eMode, uint32_t absPartIdx, uint32_t depth);

    uint8_t  isLosslessCoded(uint32_t idx) const { return m_cuTransquantBypass[idx] && m_slice->m_pps->bTransquantBypassEnabled; }
    void     setCUTransquantBypassSubParts(uint8_t flag, uint32_t absPartIdx, uint32_t depth);

    void     setTransformSkipSubParts(uint32_t useTransformSkip, TextType ttype, uint32_t absPartIdx, uint32_t depth);
    void     setTransformSkipSubParts(uint32_t useTransformSkipY, uint32_t useTransformSkipU, uint32_t useTransformSkipV, uint32_t absPartIdx, uint32_t depth);
    void     setTransformSkipPartRange(uint32_t useTransformSkip, TextType ttype, uint32_t absPartIdx, uint32_t coveredPartIdxes);

    void     setSkipFlagSubParts(uint8_t skip, uint32_t absPartIdx, uint32_t depth);

    void     setPredModeSubParts(PredMode eMode, uint32_t absPartIdx, uint32_t depth);

    void     setTrIdxSubParts(uint32_t trIdx, uint32_t absPartIdx, uint32_t depth);

    uint8_t  getCbf(uint32_t idx, TextType ttype, uint32_t trDepth) const { return (m_cbf[ttype][idx] >> trDepth) & 0x1; }
    uint8_t  getQtRootCbf(uint32_t idx) const { return m_cbf[0][idx] || m_cbf[1][idx] || m_cbf[2][idx]; }
    void     clearCbf(uint32_t absPartIdx, uint32_t depth);
    void     setCbfSubParts(uint32_t cbf, TextType ttype, uint32_t absPartIdx, uint32_t depth);
    void     setCbfPartRange(uint32_t cbf, TextType ttype, uint32_t absPartIdx, uint32_t coveredPartIdxes);

    void     setLumaIntraDirSubParts(uint32_t dir, uint32_t absPartIdx, uint32_t depth);
    void     setChromIntraDirSubParts(uint32_t dir, uint32_t absPartIdx, uint32_t depth);

    void     setInterDirSubParts(uint32_t dir, uint32_t absPartIdx, uint32_t partIdx, uint32_t depth);

    void     getPartIndexAndSize(uint32_t partIdx, uint32_t& partAddr, int& width, int& height) const;
    uint8_t  getNumPartInter() const { return nbPartsTable[(int)m_partSizes[0]]; }

    void     getMvField(const TComDataCU* cu, uint32_t absPartIdx, int picList, TComMvField& mvField) const;

    char     getRefQP(uint32_t currAbsIdxInCTU) const;
    void     deriveLeftRightTopIdx(uint32_t partIdx, uint32_t& partIdxLT, uint32_t& partIdxRT) const;
    void     deriveLeftBottomIdx(uint32_t partIdx, uint32_t& partIdxLB) const;
    void     deriveLeftRightTopIdxAdi(uint32_t& partIdxLT, uint32_t& partIdxRT, uint32_t partOffset, uint32_t partDepth) const;

    uint32_t getInterMergeCandidates(uint32_t absPartIdx, uint32_t puIdx, TComMvField (*mvFieldNeighbours)[2], uint8_t* interDirNeighbours) const;
    void     clipMv(MV& outMV) const;
    int      fillMvpCand(uint32_t puIdx, uint32_t absPartIdx, int picList, int refIdx, MV* amvpCand, MV* mvc) const;
    void     getPartPosition(uint32_t puIdx, int& xP, int& yP, int& nPSW, int& nPSH) const;
    void     getQuadtreeTULog2MinSizeInCU(uint32_t tuDepthRange[2], uint32_t absPartIdx) const;

    bool     isIntra(uint32_t partIdx) const { return m_predModes[partIdx] == MODE_INTRA; }
    uint8_t  isSkipped(uint32_t idx) const { return m_skipFlag[idx]; }
    bool     isBipredRestriction() const { return m_log2CUSize[0] == 3 && m_partSizes[0] != SIZE_2Nx2N; }

    void     getAllowedChromaDir(uint32_t absPartIdx, uint32_t* modeList) const;
    int      getIntraDirLumaPredictor(uint32_t absPartIdx, uint32_t* intraDirPred) const;

    uint32_t getCtxSplitFlag(uint32_t absPartIdx, uint32_t depth) const;
    uint32_t getCtxSkipFlag(uint32_t absPartIdx) const;

    uint32_t getSCUAddr() const { return (m_cuAddr << g_maxFullDepth * 2) + m_absIdxInCTU; }
    ScanType getCoefScanIdx(uint32_t absPartIdx, uint32_t log2TrSize, bool bIsLuma, bool bIsIntra) const;
    void     getTUEntropyCodingParameters(TUEntropyCodingParameters &result, uint32_t absPartIdx, uint32_t log2TrSize, bool bIsLuma) const;

    const TComDataCU* getPULeft(uint32_t& lPartUnitIdx, uint32_t curPartUnitIdx) const;
    const TComDataCU* getPUAbove(uint32_t& aPartUnitIdx, uint32_t curPartUnitIdx, bool planarAtCTUBoundary = false) const;
    const TComDataCU* getPUAboveLeft(uint32_t& alPartUnitIdx, uint32_t curPartUnitIdx) const;
    const TComDataCU* getPUAboveRight(uint32_t& arPartUnitIdx, uint32_t curPartUnitIdx) const;
    const TComDataCU* getPUBelowLeft(uint32_t& blPartUnitIdx, uint32_t curPartUnitIdx) const;

    const TComDataCU* getQpMinCuLeft(uint32_t& lPartUnitIdx, uint32_t currAbsIdxInCTU) const;
    const TComDataCU* getQpMinCuAbove(uint32_t& aPartUnitIdx, uint32_t currAbsIdxInCTU) const;

    const TComDataCU* getPUAboveRightAdi(uint32_t& arPartUnitIdx, uint32_t curPartUnitIdx, uint32_t partUnitOffset = 1) const;
    const TComDataCU* getPUBelowLeftAdi(uint32_t& blPartUnitIdx, uint32_t curPartUnitIdx, uint32_t partUnitOffset = 1) const;

protected:

    template<typename T>
    void setSubPart(T bParameter, T* baseCTU, uint32_t cuAddr, uint32_t cuDepth, uint32_t puIdx);

    char getLastCodedQP(uint32_t absPartIdx) const;
    int  getLastValidPartIdx(int absPartIdx) const;

    bool hasEqualMotion(uint32_t absPartIdx, const TComDataCU* candCU, uint32_t candAbsPartIdx) const;

    bool isDiffMER(int xN, int yN, int xP, int yP) const;

    /// add possible motion vector predictor candidates
    bool addMVPCand(MV& mvp, int picList, int refIdx, uint32_t partUnitIdx, MVP_DIR dir) const;

    bool addMVPCandOrder(MV& mvp, int picList, int refIdx, uint32_t partUnitIdx, MVP_DIR dir) const;

    void deriveRightBottomIdx(uint32_t partIdx, uint32_t& outPartIdxRB) const;

    bool getColMVP(int picList, int cuAddr, int partUnitIdx, MV& outMV, int& outRefIdx) const;

    /// compute scaling factor from POC difference
    int  getDistScaleFactor(int curPOC, int curRefPOC, int colPOC, int colRefPOC) const;

    void deriveCenterIdx(uint32_t partIdx, uint32_t& outPartIdxCenter) const;
};

struct DataCUMemPool
{
    uint8_t* charMemBlock;
    coeff_t* trCoeffMemBlock;

    DataCUMemPool() { charMemBlock = NULL; trCoeffMemBlock = NULL; }

    bool create(uint32_t numPartition, uint32_t sizeL, uint32_t sizeC, uint32_t numInstances)
    {
        CHECKED_MALLOC(trCoeffMemBlock, coeff_t, (sizeL + sizeC * 2) * numInstances);
        CHECKED_MALLOC(charMemBlock, uint8_t, numPartition * numInstances * TComDataCU::BytesPerPartition);

        return true;
    fail:
        return false;
    }

    void destroy()
    {
        X265_FREE(trCoeffMemBlock);
        X265_FREE(charMemBlock);
    }
};

namespace RasterAddress {
/** Check whether 2 addresses point to the same column
 * \param addrA          First address in raster scan order
 * \param addrB          Second address in raters scan order
 * \param numUnitsPerRow Number of units in a row
 * \return Result of test
 */
static inline bool isEqualCol(int addrA, int addrB, int numUnitsPerRow)
{
    // addrA % numUnitsPerRow == addrB % numUnitsPerRow
    return ((addrA ^ addrB) &  (numUnitsPerRow - 1)) == 0;
}

/** Check whether 2 addresses point to the same row
 * \param addrA          First address in raster scan order
 * \param addrB          Second address in raters scan order
 * \param numUnitsPerRow Number of units in a row
 * \return Result of test
 */
static inline bool isEqualRow(int addrA, int addrB, int numUnitsPerRow)
{
    // addrA / numUnitsPerRow == addrB / numUnitsPerRow
    return ((addrA ^ addrB) & ~(numUnitsPerRow - 1)) == 0;
}

/** Check whether 2 addresses point to the same row or column
 * \param addrA          First address in raster scan order
 * \param addrB          Second address in raters scan order
 * \param numUnitsPerRow Number of units in a row
 * \return Result of test
 */
static inline bool isEqualRowOrCol(int addrA, int addrB, int numUnitsPerRow)
{
    return isEqualCol(addrA, addrB, numUnitsPerRow) | isEqualRow(addrA, addrB, numUnitsPerRow);
}

/** Check whether one address points to the first column
 * \param addr           Address in raster scan order
 * \param numUnitsPerRow Number of units in a row
 * \return Result of test
 */
static inline bool isZeroCol(int addr, int numUnitsPerRow)
{
    // addr % numUnitsPerRow == 0
    return (addr & (numUnitsPerRow - 1)) == 0;
}

/** Check whether one address points to the first row
 * \param addr           Address in raster scan order
 * \param numUnitsPerRow Number of units in a row
 * \return Result of test
 */
static inline bool isZeroRow(int addr, int numUnitsPerRow)
{
    // addr / numUnitsPerRow == 0
    return (addr & ~(numUnitsPerRow - 1)) == 0;
}

/** Check whether one address points to a column whose index is smaller than a given value
 * \param addr           Address in raster scan order
 * \param val            Given column index value
 * \param numUnitsPerRow Number of units in a row
 * \return Result of test
 */
static inline bool lessThanCol(int addr, int val, int numUnitsPerRow)
{
    // addr % numUnitsPerRow < val
    return (addr & (numUnitsPerRow - 1)) < val;
}

/** Check whether one address points to a row whose index is smaller than a given value
 * \param addr           Address in raster scan order
 * \param val            Given row index value
 * \param numUnitsPerRow Number of units in a row
 * \return Result of test
 */
static inline bool lessThanRow(int addr, int val, int numUnitsPerRow)
{
    // addr / numUnitsPerRow < val
    return addr < val * numUnitsPerRow;
}
}
}
//! \}

#endif // ifndef X265_TCOMDATACU_H
