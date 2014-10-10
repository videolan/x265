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

/** \file     TComDataCU.h
    \brief    CU data structure (header)
    \todo     not all entities are documented
*/

#ifndef X265_TCOMDATACU_H
#define X265_TCOMDATACU_H

#include "common.h"
#include "slice.h"
#include "TComMotionInfo.h"
#include "TComRom.h"

namespace x265 {
// private namespace

// TU settings for entropy encoding
struct TUEntropyCodingParameters
{
    const uint16_t *scan;
    const uint16_t *scanCG;
    ScanType        scanType;
    uint32_t        log2TrSizeCG;
    uint32_t        firstSignificanceMapContext;
};

class Frame;
class Slice;

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Non-deblocking in-loop filter processing block data structure
// ====================================================================================================================

/// Non-deblocking filter processing block border tag
enum NDBFBlockBorderTag
{
    SGU_L = 0,
    SGU_R,
    SGU_T,
    SGU_B,
    SGU_TL,
    SGU_TR,
    SGU_BL,
    SGU_BR,
    NUM_SGU_BORDER
};

struct DataCUMemPool
{
    char*    qpMemBlock;
    uint8_t* depthMemBlock;
    uint8_t* log2CUSizeMemBlock;
    bool*    skipFlagMemBlock;
    char*    partSizeMemBlock;
    char*    predModeMemBlock;
    bool*    cuTQBypassMemBlock;
    bool*    mergeFlagMemBlock;
    uint8_t* lumaIntraDirMemBlock;
    uint8_t* chromaIntraDirMemBlock;
    uint8_t* interDirMemBlock;
    uint8_t* trIdxMemBlock;
    uint8_t* transformSkipMemBlock;
    uint8_t* cbfMemBlock;
    uint8_t* mvpIdxMemBlock;
    coeff_t* trCoeffMemBlock;
    pixel*   tqBypassYuvMemBlock;

    DataCUMemPool() { memset(this, 0, sizeof(*this)); }

    bool create(uint32_t numPartition, uint32_t sizeL, uint32_t sizeC, uint32_t numBlocks, bool isLossless)
    {
        CHECKED_MALLOC(qpMemBlock, char, numPartition * numBlocks);

        CHECKED_MALLOC(depthMemBlock, uint8_t, numPartition * numBlocks);
        CHECKED_MALLOC(log2CUSizeMemBlock, uint8_t, numPartition * numBlocks);
        CHECKED_MALLOC(skipFlagMemBlock, bool, numPartition * numBlocks);
        CHECKED_MALLOC(partSizeMemBlock, char, numPartition * numBlocks);
        CHECKED_MALLOC(predModeMemBlock, char, numPartition * numBlocks);
        CHECKED_MALLOC(cuTQBypassMemBlock, bool, numPartition * numBlocks);

        CHECKED_MALLOC(mergeFlagMemBlock, bool, numPartition * numBlocks);
        CHECKED_MALLOC(lumaIntraDirMemBlock, uint8_t, numPartition * numBlocks);
        CHECKED_MALLOC(chromaIntraDirMemBlock, uint8_t, numPartition * numBlocks);
        CHECKED_MALLOC(interDirMemBlock, uint8_t, numPartition * numBlocks);

        CHECKED_MALLOC(trIdxMemBlock, uint8_t, numPartition * numBlocks);
        CHECKED_MALLOC(transformSkipMemBlock, uint8_t, numPartition * 3 * numBlocks);

        CHECKED_MALLOC(cbfMemBlock, uint8_t, numPartition * 3 * numBlocks);
        CHECKED_MALLOC(mvpIdxMemBlock, uint8_t, numPartition * 2 * numBlocks);
        CHECKED_MALLOC(trCoeffMemBlock, coeff_t, (sizeL + sizeC * 2) * numBlocks);

        if (isLossless)
            CHECKED_MALLOC(tqBypassYuvMemBlock, pixel, (sizeL + sizeC * 2) * numBlocks);

        return true;
    fail:
        return false;
    }

    void destroy()
    {
        X265_FREE(qpMemBlock);
        X265_FREE(depthMemBlock);
        X265_FREE(log2CUSizeMemBlock);
        X265_FREE(cbfMemBlock);
        X265_FREE(interDirMemBlock);
        X265_FREE(mergeFlagMemBlock);
        X265_FREE(lumaIntraDirMemBlock);
        X265_FREE(chromaIntraDirMemBlock);
        X265_FREE(trIdxMemBlock);
        X265_FREE(transformSkipMemBlock);
        X265_FREE(trCoeffMemBlock);
        X265_FREE(mvpIdxMemBlock);
        X265_FREE(cuTQBypassMemBlock);
        X265_FREE(skipFlagMemBlock);
        X265_FREE(partSizeMemBlock);
        X265_FREE(predModeMemBlock);
        X265_FREE(tqBypassYuvMemBlock);
    }
};

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
    uint32_t childIdx;      // offset of the first child CU in m_cuLocalData
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

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// CU data structure class
class TComDataCU
{
public:

    // -------------------------------------------------------------------------------------------------------------------
    // class pointers
    // -------------------------------------------------------------------------------------------------------------------

    Frame*        m_frame;          ///< picture class pointer
    Slice*        m_slice;          ///< slice header pointer

    // -------------------------------------------------------------------------------------------------------------------
    // CU description
    // -------------------------------------------------------------------------------------------------------------------

    uint32_t      m_cuAddr;          ///< CU address in a slice
    uint32_t      m_absIdxInCTU;     ///< absolute address in a CU. It's Z scan order
    uint32_t      m_cuPelX;          ///< CU position in a pixel (X)
    uint32_t      m_cuPelY;          ///< CU position in a pixel (Y)
    uint32_t      m_numPartitions;   ///< total number of minimum partitions in a CU
    uint8_t*      m_log2CUSize;      ///< array of cu width/height
    uint8_t*      m_depth;           ///< array of depths
    int           m_chromaFormat;
    int           m_hChromaShift;
    int           m_vChromaShift;

    // -------------------------------------------------------------------------------------------------------------------
    // CU data
    // -------------------------------------------------------------------------------------------------------------------
    bool*         m_skipFlag;           ///< array of skip flags
    char*         m_partSizes;          ///< array of partition sizes
    char*         m_predModes;          ///< array of prediction modes
    bool*         m_cuTransquantBypass; ///< array of cu_transquant_bypass flags
    char*         m_qp;                 ///< array of QP values
    uint8_t*      m_trIdx;              ///< array of transform indices
    uint8_t*      m_transformSkip[3];   ///< array of transform skipping flags
    uint8_t*      m_cbf[3];             ///< array of coded block flags (CBF)
    TComCUMvField m_cuMvField[2];       ///< array of motion vectors
    coeff_t*      m_trCoeff[3];         ///< transformed coefficient buffer
    pixel*        m_tqBypassOrigYuv[3]; ///< Original Lossless YUV buffer (Y/Cb/Cr)

    // -------------------------------------------------------------------------------------------------------------------
    // neighbor access variables
    // -------------------------------------------------------------------------------------------------------------------

    const TComDataCU* m_cuAboveLeft;     ///< pointer of above-left CU
    const TComDataCU* m_cuAboveRight;    ///< pointer of above-right CU
    const TComDataCU* m_cuAbove;         ///< pointer of above CU
    const TComDataCU* m_cuLeft;          ///< pointer of left CU

    // -------------------------------------------------------------------------------------------------------------------
    // coding tool information
    // -------------------------------------------------------------------------------------------------------------------

    bool*         m_bMergeFlags;      ///< array of merge flags
    uint8_t*      m_lumaIntraDir;     ///< array of intra directions (luma)
    uint8_t*      m_chromaIntraDir;   ///< array of intra directions (chroma)
    uint8_t*      m_interDir;         ///< array of inter directions
    uint8_t*      m_mvpIdx[2];        ///< array of motion vector predictor candidates or merge candidate indices [0]

    // CU data. Index is the CU index. Neighbor CUs (top-left, top, top-right, left) are appended to the end,
    // required for prediction of current CU.
    // (1 + 4 + 16 + 64) + (1 + 8 + 1 + 8 + 1) = 104.
    CU            m_cuLocalData[104]; 

    uint32_t      m_psyEnergy;
    uint64_t      m_totalRDCost;     // sum of partition (psy) RD costs
    uint32_t      m_totalDistortion; // sum of partition distortion
    uint32_t      m_totalBits;       // sum of partition signal bits
    uint64_t      m_avgCost[4];      // stores the avg cost of CU's in frame for each depth
    uint32_t      m_count[4];
    uint64_t      m_sa8dCost;
    double        m_baseQp;          // Qp of Cu set from RateControl/Vbv.
    uint32_t      m_mvBits;          // Mv bits + Ref + block type
    uint32_t      m_coeffBits;       // Texture bits (DCT Coeffs)

    TComDataCU();

    void          initialize(DataCUMemPool *dataPool, MVFieldMemPool *mvPool, uint32_t numPartition, uint32_t cuSize, int csp, int index, bool isLossLess);
    void          initCU(Frame* pic, uint32_t cuAddr);
    void          initEstData();
    void          initSubCU(const TComDataCU& cu, const CU& cuData, uint32_t partUnitIdx);
    void          loadCTUData(uint32_t maxCUSize);

    void          copyFromPic(const TComDataCU& ctu, const CU& cuData);
    void          copyPartFrom(TComDataCU* cu, const int numPartitions, uint32_t partUnitIdx, uint32_t depth);

    void          copyToPic(uint32_t depth);
    void          copyToPic(uint32_t depth, uint32_t partIdx, uint32_t partDepth);
    void          copyCodedToPic(uint32_t depth);

    // -------------------------------------------------------------------------------------------------------------------
    // member functions for CU description
    // -------------------------------------------------------------------------------------------------------------------

    uint32_t      getSCUAddr() const               { return (m_cuAddr << g_maxFullDepth * 2) + m_absIdxInCTU; }

    uint8_t*      getDepth()                       { return m_depth; }

    uint8_t       getDepth(uint32_t idx) const     { return m_depth[idx]; }

    void          setDepthSubParts(uint32_t depth);

    // -------------------------------------------------------------------------------------------------------------------
    // member functions for CU data
    // -------------------------------------------------------------------------------------------------------------------

    char*         getPartitionSize()                      { return m_partSizes; }

    PartSize      getPartitionSize(uint32_t idx) const    { return static_cast<PartSize>(m_partSizes[idx]); }

    void          setPartSizeSubParts(PartSize eMode, uint32_t absPartIdx, uint32_t depth);
    void          setCUTransquantBypassSubParts(bool flag, uint32_t absPartIdx, uint32_t depth);

    bool*         getSkipFlag()                        { return m_skipFlag; }

    bool          getSkipFlag(uint32_t idx) const      { return m_skipFlag[idx]; }

    void          setSkipFlagSubParts(bool skip, uint32_t absPartIdx, uint32_t depth);

    char*         getPredictionMode()                 { return m_predModes; }

    PredMode      getPredictionMode(uint32_t idx) const { return static_cast<PredMode>(m_predModes[idx]); }

    bool*         getCUTransquantBypass()             { return m_cuTransquantBypass; }

    bool          getCUTransquantBypass(uint32_t idx) const { return m_cuTransquantBypass[idx]; }

    void          setPredModeSubParts(PredMode eMode, uint32_t absPartIdx, uint32_t depth);

    uint8_t*      getLog2CUSize()                     { return m_log2CUSize; }

    uint8_t       getLog2CUSize(uint32_t idx) const   { return m_log2CUSize[idx]; }

    char*         getQP()                         { return m_qp; }

    char          getQP(uint32_t idx) const       { return m_qp[idx]; }

    void          setQP(uint32_t idx, char value) { m_qp[idx] =  value; }

    void          setQPSubParts(int qp,   uint32_t absPartIdx, uint32_t depth);
    int           getLastValidPartIdx(int absPartIdx) const;
    char          getLastCodedQP(uint32_t absPartIdx) const;
    void          setQPSubCUs(int qp, TComDataCU* cu, uint32_t absPartIdx, uint32_t depth, bool &foundNonZeroCbf);

    bool          isLosslessCoded(uint32_t idx) const { return m_cuTransquantBypass[idx] && m_slice->m_pps->bTransquantBypassEnabled; }

    uint8_t*      getTransformIdx()                    { return m_trIdx; }

    uint8_t       getTransformIdx(uint32_t idx) const{ return m_trIdx[idx]; }

    void          setTrIdxSubParts(uint32_t uiTrIdx, uint32_t absPartIdx, uint32_t depth);

    uint8_t*      getTransformSkip(TextType ttype) const { return m_transformSkip[ttype]; }

    uint8_t       getTransformSkip(uint32_t idx, TextType ttype) const { return m_transformSkip[ttype][idx]; }

    void          setTransformSkipSubParts(uint32_t useTransformSkip, TextType ttype, uint32_t absPartIdx, uint32_t depth);
    void          setTransformSkipSubParts(uint32_t useTransformSkipY, uint32_t useTransformSkipU, uint32_t useTransformSkipV, uint32_t absPartIdx, uint32_t depth);

    void          getQuadtreeTULog2MinSizeInCU(uint32_t tuDepthRange[2], uint32_t absPartIdx) const;

    TComCUMvField* getCUMvField(int e)        { return &m_cuMvField[e]; }

    coeff_t*      getCoeffY()                 { return m_trCoeff[0]; }

    coeff_t*      getCoeffCb()                { return m_trCoeff[1]; }

    coeff_t*      getCoeffCr()                { return m_trCoeff[2]; }

    coeff_t*      getCoeff(TextType ttype)    { return m_trCoeff[ttype]; }

    pixel*&       getLumaOrigYuv()             { return m_tqBypassOrigYuv[0]; }

    pixel*&       getChromaOrigYuv(uint32_t chromaId) { return m_tqBypassOrigYuv[chromaId]; }

    uint8_t       getCbf(uint32_t idx, TextType ttype) const { return m_cbf[ttype][idx]; }

    uint8_t*      getCbf(TextType ttype) { return m_cbf[ttype]; }

    uint8_t       getCbf(uint32_t idx, TextType ttype, uint32_t trDepth) const { return (m_cbf[ttype][idx] >> trDepth) & 0x1; }

    void          setCbf(uint32_t idx, TextType ttype, uint8_t uh)       { m_cbf[ttype][idx] = uh; }

    uint8_t       getQtRootCbf(uint32_t idx) const { return getCbf(idx, TEXT_LUMA) || getCbf(idx, TEXT_CHROMA_U) || getCbf(idx, TEXT_CHROMA_V); }

    void          clearCbf(uint32_t absPartIdx, uint32_t depth);
    void          setCbfSubParts(uint32_t cbf, TextType ttype, uint32_t absPartIdx, uint32_t depth);
    void          setCbfPartRange(uint32_t cbf, TextType ttype, uint32_t absPartIdx, uint32_t coveredPartIdxes);
    void          setTransformSkipPartRange(uint32_t useTransformSkip, TextType ttype, uint32_t absPartIdx, uint32_t coveredPartIdxes);

    // -------------------------------------------------------------------------------------------------------------------
    // member functions for coding tool information
    // -------------------------------------------------------------------------------------------------------------------

    bool*         getMergeFlag()                    { return m_bMergeFlags; }

    bool          getMergeFlag(uint32_t idx) const { return m_bMergeFlags[idx]; }

    void          setMergeFlag(uint32_t idx, bool bMergeFlag) { m_bMergeFlags[idx] = bMergeFlag; }

    uint8_t*      getMergeIndex()                   { return m_mvpIdx[0]; }

    uint8_t       getMergeIndex(uint32_t idx) const { return m_mvpIdx[0][idx]; }

    void          setMergeIndex(uint32_t idx, int mergeIndex) { m_mvpIdx[0][idx] = (uint8_t)mergeIndex; }

    template<typename T>
    void          setSubPart(T bParameter, T* baseCTU, uint32_t cuAddr, uint32_t cuDepth, uint32_t puIdx);

    uint8_t*      getLumaIntraDir()         { return m_lumaIntraDir; }

    uint8_t       getLumaIntraDir(uint32_t idx) const { return m_lumaIntraDir[idx]; }

    void          setLumaIntraDirSubParts(uint32_t dir, uint32_t absPartIdx, uint32_t depth);

    uint8_t*      getChromaIntraDir()       { return m_chromaIntraDir; }

    uint8_t       getChromaIntraDir(uint32_t idx) const { return m_chromaIntraDir[idx]; }

    void          setChromIntraDirSubParts(uint32_t dir, uint32_t absPartIdx, uint32_t depth);

    uint8_t*      getInterDir()             { return m_interDir; }

    uint8_t       getInterDir(uint32_t idx) const { return m_interDir[idx]; }

    void          setInterDirSubParts(uint32_t dir, uint32_t absPartIdx, uint32_t partIdx, uint32_t depth);

    // -------------------------------------------------------------------------------------------------------------------
    // member functions for accessing partition information
    // -------------------------------------------------------------------------------------------------------------------

    void          getPartIndexAndSize(uint32_t partIdx, uint32_t& partAddr, int& width, int& height) const;
    uint8_t       getNumPartInter() const { return nbPartsTable[(int)m_partSizes[0]]; }
    bool          isFirstAbsZorderIdxInDepth(uint32_t absPartIdx, uint32_t depth) const;

    // -------------------------------------------------------------------------------------------------------------------
    // member functions for motion vector
    // -------------------------------------------------------------------------------------------------------------------

    void          getMvField(const TComDataCU* cu, uint32_t absPartIdx, int picList, TComMvField& rcMvField) const;

    int           fillMvpCand(uint32_t partIdx, uint32_t partAddr, int picList, int refIdx, MV* amvpCand, MV* mvc) const;
    bool          isDiffMER(int xN, int yN, int xP, int yP) const;
    void          getPartPosition(uint32_t partIdx, int& xP, int& yP, int& nPSW, int& nPSH);
    void          setMVPIdx(int picList, uint32_t idx, int mvpIdx) { m_mvpIdx[picList][idx] = (uint8_t)mvpIdx; }

    uint8_t       getMVPIdx(int picList, uint32_t idx) const   { return m_mvpIdx[picList][idx]; }

    uint8_t*      getMVPIdx(int picList) const                 { return m_mvpIdx[picList]; }

    void          clipMv(MV& outMV) const;

    // -------------------------------------------------------------------------------------------------------------------
    // utility functions for neighboring information
    // -------------------------------------------------------------------------------------------------------------------

    const TComDataCU*   getCULeft() const       { return m_cuLeft; }
    const TComDataCU*   getCUAbove() const      { return m_cuAbove; }
    const TComDataCU*   getCUAboveLeft() const  { return m_cuAboveLeft; }
    const TComDataCU*   getCUAboveRight() const { return m_cuAboveRight; }

    const TComDataCU*   getPULeft(uint32_t& lPartUnitIdx, uint32_t curPartUnitIdx) const;
    const TComDataCU*   getPUAbove(uint32_t& aPartUnitIdx, uint32_t curPartUnitIdx, bool planarAtCTUBoundary = false) const;
    const TComDataCU*   getPUAboveLeft(uint32_t& alPartUnitIdx, uint32_t curPartUnitIdx) const;
    const TComDataCU*   getPUAboveRight(uint32_t& arPartUnitIdx, uint32_t curPartUnitIdx) const;
    const TComDataCU*   getPUBelowLeft(uint32_t& blPartUnitIdx, uint32_t curPartUnitIdx) const;

    const TComDataCU*   getQpMinCuLeft(uint32_t& lPartUnitIdx, uint32_t currAbsIdxInCTU) const;
    const TComDataCU*   getQpMinCuAbove(uint32_t& aPartUnitIdx, uint32_t currAbsIdxInCTU) const;
    char          getRefQP(uint32_t currAbsIdxInCTU) const;

    const TComDataCU*   getPUAboveRightAdi(uint32_t& arPartUnitIdx, uint32_t curPartUnitIdx, uint32_t partUnitOffset = 1) const;
    const TComDataCU*   getPUBelowLeftAdi(uint32_t& blPartUnitIdx, uint32_t curPartUnitIdx, uint32_t partUnitOffset = 1) const;

    void          deriveLeftRightTopIdx(uint32_t partIdx, uint32_t& partIdxLT, uint32_t& partIdxRT) const;
    void          deriveLeftBottomIdx(uint32_t partIdx, uint32_t& partIdxLB) const;
    void          deriveLeftRightTopIdxAdi(uint32_t& partIdxLT, uint32_t& partIdxRT, uint32_t partOffset, uint32_t partDepth) const;

    bool          hasEqualMotion(uint32_t absPartIdx, const TComDataCU* candCU, uint32_t candAbsPartIdx) const;
    void          getInterMergeCandidates(uint32_t absPartIdx, uint32_t puIdx, TComMvField (*mvFieldNeighbours)[2], uint8_t* interDirNeighbours, uint32_t& maxNumMergeCand);

    // -------------------------------------------------------------------------------------------------------------------
    // member functions for modes
    // -------------------------------------------------------------------------------------------------------------------

    bool          isIntra(uint32_t partIdx) const { return m_predModes[partIdx] == MODE_INTRA; }
    bool          isSkipped(uint32_t idx) const { return m_skipFlag[idx]; }
    bool          isBipredRestriction() const { return m_log2CUSize[0] == 3 && m_partSizes[0] != SIZE_2Nx2N; }

    // -------------------------------------------------------------------------------------------------------------------
    // member functions for symbol prediction (most probable / mode conversion)
    // -------------------------------------------------------------------------------------------------------------------

    void          getAllowedChromaDir(uint32_t absPartIdx, uint32_t* modeList) const;
    int           getIntraDirLumaPredictor(uint32_t absPartIdx, uint32_t* intraDirPred) const;

    // -------------------------------------------------------------------------------------------------------------------
    // member functions for SBAC context
    // -------------------------------------------------------------------------------------------------------------------

    uint32_t      getCtxSplitFlag(uint32_t absPartIdx, uint32_t depth) const;
    uint32_t      getCtxSkipFlag(uint32_t absPartIdx) const;
    uint32_t      getCtxInterDir(uint32_t idx) const { return m_depth[idx]; }

    // -------------------------------------------------------------------------------------------------------------------
    // member functions for RD cost storage
    // -------------------------------------------------------------------------------------------------------------------

    ScanType      getCoefScanIdx(uint32_t absPartIdx, uint32_t log2TrSize, bool bIsLuma, bool bIsIntra) const;
    void          getTUEntropyCodingParameters(TUEntropyCodingParameters &result, uint32_t absPartIdx, uint32_t log2TrSize, bool bIsLuma) const;

protected:
    /// add possible motion vector predictor candidates
    bool xAddMVPCand(MV& mvp, int picList, int refIdx, uint32_t partUnitIdx, MVP_DIR dir) const;

    bool xAddMVPCandOrder(MV& mvp, int picList, int refIdx, uint32_t partUnitIdx, MVP_DIR dir) const;

    void deriveRightBottomIdx(uint32_t partIdx, uint32_t& outPartIdxRB) const;

    bool xGetColMVP(int picList, int cuAddr, int partUnitIdx, MV& outMV, int& outRefIdx) const;

    /// compute scaling factor from POC difference
    int  xGetDistScaleFactor(int curPOC, int curRefPOC, int colPOC, int colRefPOC) const;

    void xDeriveCenterIdx(uint32_t partIdx, uint32_t& outPartIdxCenter) const;
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
