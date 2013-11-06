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

#include "CommonDef.h"
#include "TComMotionInfo.h"
#include "TComSlice.h"
#include "TComRdCost.h"
#include "TComPattern.h"

#include <assert.h>

namespace x265 {
// private namespace

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

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// CU data structure class
class TComDataCU
{
private:

    // -------------------------------------------------------------------------------------------------------------------
    // class pointers
    // -------------------------------------------------------------------------------------------------------------------

    TComPic*      m_pic;            ///< picture class pointer
    TComSlice*    m_slice;          ///< slice header pointer
    TComPattern*  m_pattern;        ///< neighbor access class pointer

    // -------------------------------------------------------------------------------------------------------------------
    // CU description
    // -------------------------------------------------------------------------------------------------------------------

    uint32_t      m_cuAddr;          ///< CU address in a slice
    uint32_t      m_absIdxInLCU;     ///< absolute address in a CU. It's Z scan order
    uint32_t      m_cuPelX;          ///< CU position in a pixel (X)
    uint32_t      m_cuPelY;          ///< CU position in a pixel (Y)
    uint32_t      m_numPartitions;   ///< total number of minimum partitions in a CU
    UChar*        m_width;           ///< array of widths
    UChar*        m_height;          ///< array of heights
    UChar*        m_depth;           ///< array of depths
    int           m_chromaFormat;
    int           m_hChromaShift;
    int           m_vChromaShift;
    int           m_unitSize;        ///< size of a "minimum partition"
    uint32_t      m_unitMask;        ///< mask for mapping index to CompressMV field

    // -------------------------------------------------------------------------------------------------------------------
    // CU data
    // -------------------------------------------------------------------------------------------------------------------
    bool*         m_skipFlag;           ///< array of skip flags
    char*         m_partSizes;          ///< array of partition sizes
    char*         m_predModes;          ///< array of prediction modes
    bool*         m_cuTransquantBypass; ///< array of cu_transquant_bypass flags
    char*         m_qp;                 ///< array of QP values
    UChar*        m_trIdx;              ///< array of transform indices
    UChar*        m_transformSkip[3];   ///< array of transform skipping flags
    UChar*        m_cbf[3];             ///< array of coded block flags (CBF)
    TComCUMvField m_cuMvField[2];       ///< array of motion vectors
    TCoeff*       m_trCoeffY;           ///< transformed coefficient buffer (Y)
    TCoeff*       m_trCoeffCb;          ///< transformed coefficient buffer (Cb)
    TCoeff*       m_trCoeffCr;          ///< transformed coefficient buffer (Cr)

    Pel*          m_iPCMSampleY;        ///< PCM sample buffer (Y)
    Pel*          m_iPCMSampleCb;       ///< PCM sample buffer (Cb)
    Pel*          m_iPCMSampleCr;       ///< PCM sample buffer (Cr)

    // -------------------------------------------------------------------------------------------------------------------
    // neighbor access variables
    // -------------------------------------------------------------------------------------------------------------------

    TComDataCU*   m_cuAboveLeft;     ///< pointer of above-left CU
    TComDataCU*   m_cuAboveRight;    ///< pointer of above-right CU
    TComDataCU*   m_cuAbove;         ///< pointer of above CU
    TComDataCU*   m_cuLeft;          ///< pointer of left CU
    TComDataCU*   m_cuColocated[2];  ///< pointer of temporally colocated CU's for both directions
    TComMvField   m_mvFieldA;        ///< motion vector of position A
    TComMvField   m_mvFieldB;        ///< motion vector of position B
    TComMvField   m_mvFieldC;        ///< motion vector of position C

    // -------------------------------------------------------------------------------------------------------------------
    // coding tool information
    // -------------------------------------------------------------------------------------------------------------------

    bool*         m_bMergeFlags;      ///< array of merge flags
    UChar*        m_mergeIndex;       ///< array of merge candidate indices
    bool          m_bIsMergeAMP;
    UChar*        m_lumaIntraDir;     ///< array of intra directions (luma)
    UChar*        m_chromaIntraDir;   ///< array of intra directions (chroma)
    UChar*        m_interDir;         ///< array of inter directions
    char*         m_mvpIdx[2];        ///< array of motion vector predictor candidates
    char*         m_mvpNum[2];        ///< array of number of possible motion vectors predictors
    bool*         m_iPCMFlags;        ///< array of intra_pcm flags

    // -------------------------------------------------------------------------------------------------------------------
    // misc. variables
    // -------------------------------------------------------------------------------------------------------------------

protected:

    /// add possible motion vector predictor candidates
    bool          xAddMVPCand(AMVPInfo* info, int picList, int refIdx, uint32_t partUnitIdx, MVP_DIR dir);

    bool          xAddMVPCandOrder(AMVPInfo* info, int picList, int refIdx, uint32_t partUnitIdx, MVP_DIR dir);

    void          deriveRightBottomIdx(uint32_t partIdx, uint32_t& outPartIdxRB);

    bool          xGetColMVP(int picList, int cuAddr, int partUnitIdx, MV& outMV, int& outRefIdx);

    /// compute scaling factor from POC difference
    int           xGetDistScaleFactor(int curPOC, int curRefPOC, int colPOC, int colRefPOC);

    void xDeriveCenterIdx(uint32_t partIdx, uint32_t& outPartIdxCenter);

public:

    TComDataCU();
    virtual ~TComDataCU();

    UInt64        m_totalCost;       ///< sum of partition RD costs
    uint32_t      m_totalDistortion; ///< sum of partition distortion
    uint32_t      m_totalBits;       ///< sum of partition signal bits
    UInt64        m_avgCost[4];      // stores the avg cost of CU's in frame for each depth
    uint32_t      m_count[4];

    // -------------------------------------------------------------------------------------------------------------------
    // create / destroy / initialize / copy
    // -------------------------------------------------------------------------------------------------------------------

    void          create(uint32_t numPartition, uint32_t width, uint32_t height, int unitSize, int csp);
    void          destroy();

    void          initCU(TComPic* pic, uint32_t cuAddr);
    void          initEstData(uint32_t depth, int qp);
    void          initSubCU(TComDataCU* cu, uint32_t partUnitIdx, uint32_t depth, int qp);

    void          copyPartFrom(TComDataCU* cu, uint32_t partUnitIdx, uint32_t depth, bool isRDObasedAnalysis = true);

    void          copyToPic(UChar depth);
    void          copyToPic(UChar depth, uint32_t partIdx, uint32_t partDepth);

    // -------------------------------------------------------------------------------------------------------------------
    // member functions for CU description
    // -------------------------------------------------------------------------------------------------------------------

    TComPic*      getPic()                         { return m_pic; }

    TComSlice*    getSlice()                       { return m_slice; }

    uint32_t&     getAddr()                        { return m_cuAddr; }

    uint32_t&     getZorderIdxInCU()               { return m_absIdxInLCU; }

    uint32_t      getSCUAddr();

    uint32_t      getCUPelX()                      { return m_cuPelX; }

    uint32_t      getCUPelY()                      { return m_cuPelY; }

    TComPattern*  getPattern()                     { return m_pattern; }

    UChar*        getDepth()                       { return m_depth; }

    UChar         getDepth(uint32_t idx)           { return m_depth[idx]; }

    void          setDepthSubParts(uint32_t depth, uint32_t absPartIdx);

    // -------------------------------------------------------------------------------------------------------------------
    // member functions for CU data
    // -------------------------------------------------------------------------------------------------------------------

    char*         getPartitionSize()                      { return m_partSizes; }

    PartSize      getPartitionSize(uint32_t idx)              { return static_cast<PartSize>(m_partSizes[idx]); }

    void          setPartSizeSubParts(PartSize eMode, uint32_t absPartIdx, uint32_t depth);
    void          setCUTransquantBypassSubParts(bool flag, uint32_t absPartIdx, uint32_t depth);

    bool*        getSkipFlag()                        { return m_skipFlag; }

    bool         getSkipFlag(uint32_t idx)            { return m_skipFlag[idx]; }

    void         setSkipFlagSubParts(bool skip, uint32_t absPartIdx, uint32_t depth);

    char*         getPredictionMode()                 { return m_predModes; }

    PredMode      getPredictionMode(uint32_t idx)         { return static_cast<PredMode>(m_predModes[idx]); }

    bool*         getCUTransquantBypass()             { return m_cuTransquantBypass; }

    bool          getCUTransquantBypass(uint32_t idx)     { return m_cuTransquantBypass[idx]; }

    void          setPredModeSubParts(PredMode eMode, uint32_t absPartIdx, uint32_t depth);

    UChar*        getWidth()                     { return m_width; }

    UChar         getWidth(uint32_t idx)             { return m_width[idx]; }

    UChar*        getHeight()                    { return m_height; }

    UChar         getHeight(uint32_t idx)            { return m_height[idx]; }

    char*         getQP()                        { return m_qp; }

    char          getQP(uint32_t idx)                { return m_qp[idx]; }

    void          setQP(uint32_t idx, char value)    { m_qp[idx] =  value; }

    void          setQPSubParts(int qp,   uint32_t absPartIdx, uint32_t depth);
    int           getLastValidPartIdx(int absPartIdx);
    char          getLastCodedQP(uint32_t absPartIdx);
    void          setQPSubCUs(int qp, TComDataCU* cu, uint32_t absPartIdx, uint32_t depth, bool &foundNonZeroCbf);

    bool          isLosslessCoded(uint32_t absPartIdx);

    UChar*        getTransformIdx()                    { return m_trIdx; }

    UChar         getTransformIdx(uint32_t idx)            { return m_trIdx[idx]; }

    void          setTrIdxSubParts(uint32_t uiTrIdx, uint32_t absPartIdx, uint32_t depth);

    UChar*        getTransformSkip(TextType ttype)     { return m_transformSkip[g_convertTxtTypeToIdx[ttype]]; }

    UChar         getTransformSkip(uint32_t idx, TextType ttype)    { return m_transformSkip[g_convertTxtTypeToIdx[ttype]][idx]; }

    void          setTransformSkipSubParts(uint32_t useTransformSkip, TextType ttype, uint32_t absPartIdx, uint32_t depth);
    void          setTransformSkipSubParts(uint32_t useTransformSkipY, uint32_t useTransformSkipU, uint32_t useTransformSkipV, uint32_t absPartIdx, uint32_t depth);

    uint32_t          getQuadtreeTULog2MinSizeInCU(uint32_t absPartIdx);

    TComCUMvField* getCUMvField(int e)        { return &m_cuMvField[e]; }

    TCoeff*&      getCoeffY()                 { return m_trCoeffY; }

    TCoeff*&      getCoeffCb()                { return m_trCoeffCb; }

    TCoeff*&      getCoeffCr()                { return m_trCoeffCr; }

    Pel*&         getPCMSampleY()             { return m_iPCMSampleY; }

    Pel*&         getPCMSampleCb()            { return m_iPCMSampleCb; }

    Pel*&         getPCMSampleCr()            { return m_iPCMSampleCr; }

    UChar         getCbf(uint32_t idx, TextType ttype) { return m_cbf[g_convertTxtTypeToIdx[ttype]][idx]; }

    UChar*        getCbf(TextType ttype)           { return m_cbf[g_convertTxtTypeToIdx[ttype]]; }

    UChar         getCbf(uint32_t idx, TextType ttype, uint32_t trDepth) { return (getCbf(idx, ttype) >> trDepth) & 0x1; }

    void          setCbf(uint32_t idx, TextType ttype, UChar uh)     { m_cbf[g_convertTxtTypeToIdx[ttype]][idx] = uh; }

    void          clearCbf(uint32_t idx, TextType ttype, uint32_t numParts);
    UChar         getQtRootCbf(uint32_t idx)           { return getCbf(idx, TEXT_LUMA, 0) || getCbf(idx, TEXT_CHROMA_U, 0) || getCbf(idx, TEXT_CHROMA_V, 0); }

    void          setCbfSubParts(uint32_t cbfY, uint32_t cbfU, uint32_t cbfV, uint32_t absPartIdx, uint32_t depth);
    void          setCbfSubParts(uint32_t cbf, TextType ttype, uint32_t absPartIdx, uint32_t depth);
    void          setCbfSubParts(uint32_t cbf, TextType ttype, uint32_t absPartIdx, uint32_t partIdx, uint32_t depth);

    // -------------------------------------------------------------------------------------------------------------------
    // member functions for coding tool information
    // -------------------------------------------------------------------------------------------------------------------

    bool*         getMergeFlag()                    { return m_bMergeFlags; }

    bool          getMergeFlag(uint32_t idx)            { return m_bMergeFlags[idx]; }

    void          setMergeFlagSubParts(bool bMergeFlag, uint32_t absPartIdx, uint32_t partIdx, uint32_t depth);

    UChar*        getMergeIndex()                   { return m_mergeIndex; }

    UChar         getMergeIndex(uint32_t idx)           { return m_mergeIndex[idx]; }

    void          setMergeIndexSubParts(uint32_t mergeIndex, uint32_t absPartIdx, uint32_t partIdx, uint32_t depth);
    template<typename T>
    void          setSubPart(T bParameter, T* pbBaseLCU, uint32_t cuAddr, uint32_t cuDepth, uint32_t puIdx);

    void          setMergeAMP(bool b)       { m_bIsMergeAMP = b; }

    bool          getMergeAMP()             { return m_bIsMergeAMP; }

    UChar*        getLumaIntraDir()         { return m_lumaIntraDir; }

    UChar         getLumaIntraDir(uint32_t idx) { return m_lumaIntraDir[idx]; }

    void          setLumaIntraDirSubParts(uint32_t dir, uint32_t absPartIdx, uint32_t depth);

    UChar*        getChromaIntraDir()                 { return m_chromaIntraDir; }

    UChar         getChromaIntraDir(uint32_t idx)         { return m_chromaIntraDir[idx]; }

    void          setChromIntraDirSubParts(uint32_t dir, uint32_t absPartIdx, uint32_t depth);

    UChar*        getInterDir()                    { return m_interDir; }

    UChar         getInterDir(uint32_t idx)            { return m_interDir[idx]; }

    void          setInterDirSubParts(uint32_t dir,  uint32_t absPartIdx, uint32_t partIdx, uint32_t depth);
    bool*         getIPCMFlag()                     { return m_iPCMFlags; }

    bool          getIPCMFlag(uint32_t idx)             { return m_iPCMFlags[idx]; }

    void          setIPCMFlag(uint32_t idx, bool b)     { m_iPCMFlags[idx] = b; }

    void          setIPCMFlagSubParts(bool bIpcmFlag, uint32_t absPartIdx, uint32_t depth);

    // -------------------------------------------------------------------------------------------------------------------
    // member functions for accessing partition information
    // -------------------------------------------------------------------------------------------------------------------

    void          getPartIndexAndSize(uint32_t partIdx, uint32_t& ruiPartAddr, int& riWidth, int& riHeight);
    UChar         getNumPartInter();
    bool          isFirstAbsZorderIdxInDepth(uint32_t absPartIdx, uint32_t depth);

    // -------------------------------------------------------------------------------------------------------------------
    // member functions for motion vector
    // -------------------------------------------------------------------------------------------------------------------

    void          getMvField(TComDataCU* cu, uint32_t absPartIdx, int picList, TComMvField& rcMvField);

    void          fillMvpCand(uint32_t partIdx, uint32_t partAddr, int picList, int refIdx, AMVPInfo* info);
    bool          isDiffMER(int xN, int yN, int xP, int yP);
    void          getPartPosition(uint32_t partIdx, int& xP, int& yP, int& nPSW, int& nPSH);
    void          setMVPIdx(int picList, uint32_t idx, int mvpIdx) { m_mvpIdx[picList][idx] = (char)mvpIdx; }

    int           getMVPIdx(int picList, uint32_t idx)             { return m_mvpIdx[picList][idx]; }

    char*         getMVPIdx(int picList)                       { return m_mvpIdx[picList]; }

    int           getMVPNum(int picList, uint32_t idx)             { return m_mvpNum[picList][idx]; }

    char*         getMVPNum(int picList)                       { return m_mvpNum[picList]; }

    void          setMVPIdxSubParts(int mvpIdx, int picList, uint32_t absPartIdx, uint32_t partIdx, uint32_t depth);
    void          setMVPNumSubParts(int iMVPNum, int picList, uint32_t absPartIdx, uint32_t partIdx, uint32_t depth);

    void          clipMv(MV& outMV);

    void          getMvPredLeft(MV& mvPred)       { mvPred = m_mvFieldA.mv; }

    void          getMvPredAbove(MV& mvPred)      { mvPred = m_mvFieldB.mv; }

    void          getMvPredAboveRight(MV& mvPred) { mvPred = m_mvFieldC.mv; }

    // -------------------------------------------------------------------------------------------------------------------
    // utility functions for neighboring information
    // -------------------------------------------------------------------------------------------------------------------

    TComDataCU*   getCULeft() { return m_cuLeft; }

    TComDataCU*   getCUAbove() { return m_cuAbove; }

    TComDataCU*   getCUAboveLeft() { return m_cuAboveLeft; }

    TComDataCU*   getCUAboveRight() { return m_cuAboveRight; }

    TComDataCU*   getCUColocated(int picList) { return m_cuColocated[picList]; }

    TComDataCU*   getPULeft(uint32_t& lPartUnitIdx,
                            uint32_t  curPartUnitIdx,
                            bool      bEnforceSliceRestriction = true,
                            bool      bEnforceTileRestriction = true);
    TComDataCU*   getPUAbove(uint32_t& uiAPartUnitIdx,
                             uint32_t  curPartUnitIdx,
                             bool      bEnforceSliceRestriction = true,
                             bool      planarAtLCUBoundary = false,
                             bool      bEnforceTileRestriction = true);
    TComDataCU*   getPUAboveLeft(uint32_t& alPartUnitIdx, uint32_t curPartUnitIdx, bool bEnforceSliceRestriction = true);
    TComDataCU*   getPUAboveRight(uint32_t& arPartUnitIdx, uint32_t curPartUnitIdx, bool bEnforceSliceRestriction = true);
    TComDataCU*   getPUBelowLeft(uint32_t& blPartUnitIdx, uint32_t curPartUnitIdx, bool bEnforceSliceRestriction = true);

    TComDataCU*   getQpMinCuLeft(uint32_t& lPartUnitIdx, uint32_t uiCurrAbsIdxInLCU);
    TComDataCU*   getQpMinCuAbove(uint32_t& aPartUnitIdx, uint32_t currAbsIdxInLCU);
    char          getRefQP(uint32_t uiCurrAbsIdxInLCU);

    TComDataCU*   getPUAboveRightAdi(uint32_t& arPartUnitIdx, uint32_t curPartUnitIdx, uint32_t partUnitOffset = 1, bool bEnforceSliceRestriction = true);
    TComDataCU*   getPUBelowLeftAdi(uint32_t& blPartUnitIdx, uint32_t curPartUnitIdx, uint32_t partUnitOffset = 1, bool bEnforceSliceRestriction = true);

    void          deriveLeftRightTopIdx(uint32_t partIdx, uint32_t& partIdxLT, uint32_t& partIdxRT);
    void          deriveLeftBottomIdx(uint32_t partIdx, uint32_t& partIdxLB);

    void          deriveLeftRightTopIdxAdi(uint32_t& partIdxLT, uint32_t& partIdxRT, uint32_t partOffset, uint32_t partDepth);
    void          deriveLeftBottomIdxAdi(uint32_t& partIdxLB, uint32_t partOffset, uint32_t partDepth);

    bool          hasEqualMotion(uint32_t absPartIdx, TComDataCU* candCU, uint32_t candAbsPartIdx);
    void          getInterMergeCandidates(uint32_t absPartIdx, uint32_t puIdx, TComMvField* mFieldNeighbours, UChar* interDirNeighbours, int& numValidMergeCand, int mrgCandIdx = -1);
    void          deriveLeftRightTopIdxGeneral(uint32_t absPartIdx, uint32_t partIdx, uint32_t& partIdxLT, uint32_t& partIdxRT);
    void          deriveLeftBottomIdxGeneral(uint32_t absPartIdx, uint32_t partIdx, uint32_t& partIdxLB);

    // -------------------------------------------------------------------------------------------------------------------
    // member functions for modes
    // -------------------------------------------------------------------------------------------------------------------

    bool          isIntra(uint32_t partIdx)  { return m_predModes[partIdx] == MODE_INTRA; }

    bool          isSkipped(uint32_t partIdx); ///< SKIP (no residual)
    bool          isBipredRestriction(uint32_t puIdx);

    // -------------------------------------------------------------------------------------------------------------------
    // member functions for symbol prediction (most probable / mode conversion)
    // -------------------------------------------------------------------------------------------------------------------

    uint32_t      getIntraSizeIdx(uint32_t absPartIdx);

    void          getAllowedChromaDir(uint32_t absPartIdx, uint32_t* modeList);
    void          getIntraDirLumaPredictor(uint32_t absPartIdx, int32_t* intraDirPred, int32_t* mode = NULL);

    // -------------------------------------------------------------------------------------------------------------------
    // member functions for SBAC context
    // -------------------------------------------------------------------------------------------------------------------

    uint32_t      getCtxSplitFlag(uint32_t absPartIdx, uint32_t depth);
    uint32_t      getCtxQtCbf(TextType ttype, uint32_t trDepth);

    uint32_t      getCtxSkipFlag(uint32_t absPartIdx);
    uint32_t      getCtxInterDir(uint32_t absPartIdx);

    // -------------------------------------------------------------------------------------------------------------------
    // member functions for RD cost storage
    // -------------------------------------------------------------------------------------------------------------------

    uint32_t&     getTotalNumPart()               { return m_numPartitions; }

    uint32_t      getCoefScanIdx(uint32_t absPartIdx, uint32_t width, bool bIsLuma, bool bIsIntra);

    // -------------------------------------------------------------------------------------------------------------------
    // member functions to support multiple color space formats
    // -------------------------------------------------------------------------------------------------------------------

    int           getHorzChromaShift()  { return m_hChromaShift; }

    int           getVertChromaShift()  { return m_vChromaShift; }
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
