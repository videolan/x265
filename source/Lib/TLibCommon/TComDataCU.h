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

#ifndef _TCOMDATACU_
#define _TCOMDATACU_

#include <assert.h>

// Include files
#include "CommonDef.h"
#include "TComMotionInfo.h"
#include "TComSlice.h"
#include "TComRdCost.h"
#include "TComPattern.h"

#include <algorithm>
#include <vector>

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

/// Non-deblocking filter processing block information
struct NDBFBlockInfo
{
    Int   sliceID; //!< slice ID
    UInt  startSU; //!< starting SU z-scan address in LCU
    UInt  endSU;  //!< ending SU z-scan address in LCU
    UInt  widthSU; //!< number of SUs in width
    UInt  heightSU; //!< number of SUs in height
    UInt  posX;   //!< top-left X coordinate in picture
    UInt  posY;   //!< top-left Y coordinate in picture
    UInt  width;  //!< number of pixels in width
    UInt  height; //!< number of pixels in height
    Bool  isBorderAvailable[NUM_SGU_BORDER]; //!< the border availabilities
    Bool  allBordersAvailable;

    NDBFBlockInfo() : sliceID(0), startSU(0), endSU(0) {} //!< constructor

    const NDBFBlockInfo& operator =(const NDBFBlockInfo& src); //!< "=" operator
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

    UInt          m_cuAddr;         ///< CU address in a slice
    UInt          m_absIdxInLCU;    ///< absolute address in a CU. It's Z scan order
    UInt          m_cuPelX;         ///< CU position in a pixel (X)
    UInt          m_cuPelY;         ///< CU position in a pixel (Y)
    UInt          m_numPartitions;   ///< total number of minimum partitions in a CU
    UChar*        m_width;         ///< array of widths
    UChar*        m_height;        ///< array of heights
    UChar*        m_depth;         ///< array of depths
    Int           m_unitSize;         ///< size of a "minimum partition"

    // -------------------------------------------------------------------------------------------------------------------
    // CU data
    // -------------------------------------------------------------------------------------------------------------------
    Bool*         m_skipFlag;         ///< array of skip flags
    Char*         m_partSizes;       ///< array of partition sizes
    Char*         m_predModes;       ///< array of prediction modes
    Bool*         m_cuTransquantBypass; ///< array of cu_transquant_bypass flags
    Char*         m_qp;             ///< array of QP values
    UChar*        m_trIdx;         ///< array of transform indices
    UChar*        m_transformSkip[3]; ///< array of transform skipping flags
    UChar*        m_cbf[3];        ///< array of coded block flags (CBF)
    TComCUMvField m_cuMvField[2];   ///< array of motion vectors
    TCoeff*       m_trCoeffY;       ///< transformed coefficient buffer (Y)
    TCoeff*       m_trCoeffCb;      ///< transformed coefficient buffer (Cb)
    TCoeff*       m_trCoeffCr;      ///< transformed coefficient buffer (Cr)

    Int*          m_arlCoeffY;      ///< ARL coefficient buffer (Y)
    Int*          m_arlCoeffCb;     ///< ARL coefficient buffer (Cb)
    Int*          m_arlCoeffCr;   ///< ARL coefficient buffer (Cr)

    Pel*          m_iPCMSampleY;    ///< PCM sample buffer (Y)
    Pel*          m_iPCMSampleCb;   ///< PCM sample buffer (Cb)
    Pel*          m_iPCMSampleCr;   ///< PCM sample buffer (Cr)

    std::vector<NDBFBlockInfo> m_vNDFBlock;

    // -------------------------------------------------------------------------------------------------------------------
    // neighbor access variables
    // -------------------------------------------------------------------------------------------------------------------

    TComDataCU*   m_cuAboveLeft;    ///< pointer of above-left CU
    TComDataCU*   m_cuAboveRight;   ///< pointer of above-right CU
    TComDataCU*   m_cuAbove;        ///< pointer of above CU
    TComDataCU*   m_cuLeft;         ///< pointer of left CU
    TComDataCU*   m_cuColocated[2]; ///< pointer of temporally colocated CU's for both directions
    TComMvField   m_mvFieldA;        ///< motion vector of position A
    TComMvField   m_mvFieldB;        ///< motion vector of position B
    TComMvField   m_mvFieldC;        ///< motion vector of position C

    // -------------------------------------------------------------------------------------------------------------------
    // coding tool information
    // -------------------------------------------------------------------------------------------------------------------

    Bool*         m_bMergeFlags;      ///< array of merge flags
    UChar*        m_mergeIndex;    ///< array of merge candidate indices
    Bool          m_bIsMergeAMP;
    UChar*        m_lumaIntraDir;  ///< array of intra directions (luma)
    UChar*        m_chromaIntraDir; ///< array of intra directions (chroma)
    UChar*        m_interDir;      ///< array of inter directions
    Char*         m_mvpIdx[2];     ///< array of motion vector predictor candidates
    Char*         m_mvpNum[2];     ///< array of number of possible motion vectors predictors
    Bool*         m_iPCMFlags;       ///< array of intra_pcm flags

    // -------------------------------------------------------------------------------------------------------------------
    // misc. variables
    // -------------------------------------------------------------------------------------------------------------------

    UInt64        m_totalCost;       ///< sum of partition RD costs
    UInt          m_totalDistortion; ///< sum of partition distortion
    UInt          m_totalBits;       ///< sum of partition signal bits
    UInt          m_totalBins;       ///< sum of partition bins
    Char          m_codedQP;

protected:

    /// add possible motion vector predictor candidates
    Bool          xAddMVPCand(AMVPInfo* info, RefPicList picList, Int refIdx, UInt partUnitIdx, MVP_DIR dir);

    Bool          xAddMVPCandOrder(AMVPInfo* info, RefPicList picList, Int refIdx, UInt partUnitIdx, MVP_DIR dir);

    Void          deriveRightBottomIdx(UInt partIdx, UInt& outPartIdxRB);

    Bool          xGetColMVP(RefPicList picList, Int cuAddr, Int partUnitIdx, x265::MV& outMV, Int& outRefIdx);

    /// compute scaling factor from POC difference
    Int           xGetDistScaleFactor(Int curPOC, Int curRefPOC, Int colPOC, Int colRefPOC);

    Void xDeriveCenterIdx(UInt partIdx, UInt& outPartIdxCenter);

public:

    TComDataCU();
    virtual ~TComDataCU();

    // -------------------------------------------------------------------------------------------------------------------
    // create / destroy / initialize / copy
    // -------------------------------------------------------------------------------------------------------------------

    Void          create(UInt numPartition, UInt width, UInt height, Int unitSize);
    Void          destroy();

    Void          initCU(TComPic* pic, UInt cuAddr);
    Void          initEstData(UInt depth, Int qp);
    Void          initSubCU(TComDataCU* cu, UInt partUnitIdx, UInt depth, Int qp);
    Void          setOutsideCUPart(UInt absPartIdx, UInt depth);

    Void          copySubCU(TComDataCU* cu, UInt partUnitIdx, UInt depth);
    Void          copyInterPredInfoFrom(TComDataCU* cu, UInt absPartIdx, RefPicList picList);
    Void          copyPartFrom(TComDataCU* cu, UInt partUnitIdx, UInt depth, Bool isRDObasedAnalysis = true);

    Void          copyToPic(UChar depth);
    Void          copyToPic(UChar depth, UInt partIdx, UInt partDepth);

    // -------------------------------------------------------------------------------------------------------------------
    // member functions for CU description
    // -------------------------------------------------------------------------------------------------------------------

    TComPic*      getPic()                         { return m_pic; }

    TComSlice*    getSlice()                       { return m_slice; }

    UInt&         getAddr()                        { return m_cuAddr; }

    UInt&         getZorderIdxInCU()               { return m_absIdxInLCU; }

    UInt          getSCUAddr();

    UInt          getCUPelX()                        { return m_cuPelX; }

    UInt          getCUPelY()                        { return m_cuPelY; }

    TComPattern*  getPattern()                        { return m_pattern; }

    UChar*        getDepth()                        { return m_depth; }

    UChar         getDepth(UInt uiIdx)            { return m_depth[uiIdx]; }

    Void          setDepth(UInt uiIdx, UChar  uh) { m_depth[uiIdx] = uh; }

    Void          setDepthSubParts(UInt depth, UInt absPartIdx);

    // -------------------------------------------------------------------------------------------------------------------
    // member functions for CU data
    // -------------------------------------------------------------------------------------------------------------------

    Char*         getPartitionSize()                      { return m_partSizes; }

    Int           getUnitSize()                           { return m_unitSize; }

    PartSize      getPartitionSize(UInt uiIdx)            { return static_cast<PartSize>(m_partSizes[uiIdx]); }

    Void          setPartitionSize(UInt uiIdx, PartSize uh) { m_partSizes[uiIdx] = (Char)uh; }

    Void          setPartSizeSubParts(PartSize eMode, UInt absPartIdx, UInt depth);
    Void          setCUTransquantBypassSubParts(Bool flag, UInt absPartIdx, UInt depth);

    Bool*        getSkipFlag()                        { return m_skipFlag; }

    Bool         getSkipFlag(UInt idx)                { return m_skipFlag[idx]; }

    Void         setSkipFlag(UInt idx, Bool skip)     { m_skipFlag[idx] = skip; }

    Void         setSkipFlagSubParts(Bool skip, UInt absPartIdx, UInt depth);

    Char*         getPredictionMode()                        { return m_predModes; }

    PredMode      getPredictionMode(UInt uiIdx)            { return static_cast<PredMode>(m_predModes[uiIdx]); }

    Bool*         getCUTransquantBypass()                        { return m_cuTransquantBypass; }

    Bool          getCUTransquantBypass(UInt uiIdx)             { return m_cuTransquantBypass[uiIdx]; }

    Void          setPredictionMode(UInt uiIdx, PredMode uh) { m_predModes[uiIdx] = (Char)uh; }

    Void          setPredModeSubParts(PredMode eMode, UInt absPartIdx, UInt depth);

    UChar*        getWidth()                        { return m_width; }

    UChar         getWidth(UInt uiIdx)            { return m_width[uiIdx]; }

    Void          setWidth(UInt uiIdx, UChar  uh) { m_width[uiIdx] = uh; }

    UChar*        getHeight()                        { return m_height; }

    UChar         getHeight(UInt uiIdx)            { return m_height[uiIdx]; }

    Void          setHeight(UInt uiIdx, UChar  uh) { m_height[uiIdx] = uh; }

    Void          setSizeSubParts(UInt width, UInt height, UInt absPartIdx, UInt depth);

    Char*         getQP()                        { return m_qp; }

    Char          getQP(UInt uiIdx)            { return m_qp[uiIdx]; }

    Void          setQP(UInt uiIdx, Char value) { m_qp[uiIdx] =  value; }

    Void          setQPSubParts(Int qp,   UInt absPartIdx, UInt depth);
    Int           getLastValidPartIdx(Int iAbsPartIdx);
    Char          getLastCodedQP(UInt absPartIdx);
    Void          setQPSubCUs(Int qp, TComDataCU* cu, UInt absPartIdx, UInt depth, Bool &foundNonZeroCbf);
    Void          setCodedQP(Char qp)               { m_codedQP = qp; }

    Char          getCodedQP()                        { return m_codedQP; }

    Bool          isLosslessCoded(UInt absPartIdx);

    UChar*        getTransformIdx()                        { return m_trIdx; }

    UChar         getTransformIdx(UInt uiIdx)            { return m_trIdx[uiIdx]; }

    Void          setTrIdxSubParts(UInt uiTrIdx, UInt absPartIdx, UInt depth);

    UChar*        getTransformSkip(TextType ttype)    { return m_transformSkip[g_convertTxtTypeToIdx[ttype]]; }

    UChar         getTransformSkip(UInt uiIdx, TextType ttype)    { return m_transformSkip[g_convertTxtTypeToIdx[ttype]][uiIdx]; }

    Void          setTransformSkipSubParts(UInt useTransformSkip, TextType ttype, UInt absPartIdx, UInt depth);
    Void          setTransformSkipSubParts(UInt useTransformSkipY, UInt useTransformSkipU, UInt useTransformSkipV, UInt absPartIdx, UInt depth);

    UInt          getQuadtreeTULog2MinSizeInCU(UInt absPartIdx);

    TComCUMvField* getCUMvField(RefPicList e)          { return &m_cuMvField[e]; }

    TCoeff*&      getCoeffY()                        { return m_trCoeffY; }

    TCoeff*&      getCoeffCb()                        { return m_trCoeffCb; }

    TCoeff*&      getCoeffCr()                        { return m_trCoeffCr; }

    Int*&         getArlCoeffY()                        { return m_arlCoeffY; }

    Int*&         getArlCoeffCb()                        { return m_arlCoeffCb; }

    Int*&         getArlCoeffCr()                        { return m_arlCoeffCr; }

    Pel*&         getPCMSampleY()                        { return m_iPCMSampleY; }

    Pel*&         getPCMSampleCb()                        { return m_iPCMSampleCb; }

    Pel*&         getPCMSampleCr()                        { return m_iPCMSampleCr; }

    UChar         getCbf(UInt uiIdx, TextType ttype)                  { return m_cbf[g_convertTxtTypeToIdx[ttype]][uiIdx]; }

    UChar*        getCbf(TextType ttype)                              { return m_cbf[g_convertTxtTypeToIdx[ttype]]; }

    UChar         getCbf(UInt uiIdx, TextType ttype, UInt trDepth)  { return (getCbf(uiIdx, ttype) >> trDepth) & 0x1; }

    Void          setCbf(UInt uiIdx, TextType ttype, UChar uh)        { m_cbf[g_convertTxtTypeToIdx[ttype]][uiIdx] = uh; }

    Void          clearCbf(UInt uiIdx, TextType ttype, UInt uiNumParts);
    UChar         getQtRootCbf(UInt uiIdx)                      { return getCbf(uiIdx, TEXT_LUMA, 0) || getCbf(uiIdx, TEXT_CHROMA_U, 0) || getCbf(uiIdx, TEXT_CHROMA_V, 0); }

    Void          setCbfSubParts(UInt uiCbfY, UInt uiCbfU, UInt uiCbfV, UInt absPartIdx, UInt depth);
    Void          setCbfSubParts(UInt uiCbf, TextType eTType, UInt absPartIdx, UInt depth);
    Void          setCbfSubParts(UInt uiCbf, TextType eTType, UInt absPartIdx, UInt partIdx, UInt depth);

    // -------------------------------------------------------------------------------------------------------------------
    // member functions for coding tool information
    // -------------------------------------------------------------------------------------------------------------------

    Bool*         getMergeFlag()                        { return m_bMergeFlags; }

    Bool          getMergeFlag(UInt uiIdx)            { return m_bMergeFlags[uiIdx]; }

    Void          setMergeFlag(UInt uiIdx, Bool b)    { m_bMergeFlags[uiIdx] = b; }

    Void          setMergeFlagSubParts(Bool bMergeFlag, UInt absPartIdx, UInt partIdx, UInt depth);

    UChar*        getMergeIndex()                        { return m_mergeIndex; }

    UChar         getMergeIndex(UInt uiIdx)            { return m_mergeIndex[uiIdx]; }

    Void          setMergeIndex(UInt uiIdx, UInt uiMergeIndex) { m_mergeIndex[uiIdx] = (UChar)uiMergeIndex; }

    Void          setMergeIndexSubParts(UInt uiMergeIndex, UInt absPartIdx, UInt partIdx, UInt depth);
    template<typename T>
    Void          setSubPart(T bParameter, T* pbBaseLCU, UInt cuAddr, UInt uiCUDepth, UInt uiPUIdx);

    Void          setMergeAMP(Bool b)      { m_bIsMergeAMP = b; }

    Bool          getMergeAMP()             { return m_bIsMergeAMP; }

    UChar*        getLumaIntraDir()                        { return m_lumaIntraDir; }

    UChar         getLumaIntraDir(UInt uiIdx)            { return m_lumaIntraDir[uiIdx]; }

    Void          setLumaIntraDir(UInt uiIdx, UChar  uh) { m_lumaIntraDir[uiIdx] = uh; }

    Void          setLumaIntraDirSubParts(UInt uiDir,  UInt absPartIdx, UInt depth);

    UChar*        getChromaIntraDir()                        { return m_chromaIntraDir; }

    UChar         getChromaIntraDir(UInt uiIdx)            { return m_chromaIntraDir[uiIdx]; }

    Void          setChromaIntraDir(UInt uiIdx, UChar  uh) { m_chromaIntraDir[uiIdx] = uh; }

    Void          setChromIntraDirSubParts(UInt uiDir,  UInt absPartIdx, UInt depth);

    UChar*        getInterDir()                        { return m_interDir; }

    UChar         getInterDir(UInt uiIdx)            { return m_interDir[uiIdx]; }

    Void          setInterDir(UInt uiIdx, UChar  uh) { m_interDir[uiIdx] = uh; }

    Void          setInterDirSubParts(UInt uiDir,  UInt absPartIdx, UInt partIdx, UInt depth);
    Bool*         getIPCMFlag()                        { return m_iPCMFlags; }

    Bool          getIPCMFlag(UInt uiIdx)             { return m_iPCMFlags[uiIdx]; }

    Void          setIPCMFlag(UInt uiIdx, Bool b)     { m_iPCMFlags[uiIdx] = b; }

    Void          setIPCMFlagSubParts(Bool bIpcmFlag, UInt absPartIdx, UInt depth);

    std::vector<NDBFBlockInfo>* getNDBFilterBlocks()      { return &m_vNDFBlock; }

    Void setNDBFilterBlockBorderAvailability(UInt numLCUInPicWidth, UInt numLCUInPicHeight, UInt numSUInLCUWidth, UInt numSUInLCUHeight, UInt picWidth, UInt picHeight
                                             , Bool bTopTileBoundary, Bool bDownTileBoundary, Bool bLeftTileBoundary, Bool bRightTileBoundary
                                             , Bool bIndependentTileBoundaryEnabled);
    // -------------------------------------------------------------------------------------------------------------------
    // member functions for accessing partition information
    // -------------------------------------------------------------------------------------------------------------------

    Void          getPartIndexAndSize(UInt partIdx, UInt& ruiPartAddr, Int& riWidth, Int& riHeight);
    UChar         getNumPartInter();
    Bool          isFirstAbsZorderIdxInDepth(UInt absPartIdx, UInt depth);

    // -------------------------------------------------------------------------------------------------------------------
    // member functions for motion vector
    // -------------------------------------------------------------------------------------------------------------------

    Void          getMvField(TComDataCU* cu, UInt absPartIdx, RefPicList picList, TComMvField& rcMvField);

    Void          fillMvpCand(UInt partIdx, UInt partAddr, RefPicList picList, Int refIdx, AMVPInfo* info);
    Bool          isDiffMER(Int xN, Int yN, Int xP, Int yP);
    Void          getPartPosition(UInt partIdx, Int& xP, Int& yP, Int& nPSW, Int& nPSH);
    Void          setMVPIdx(RefPicList picList, UInt uiIdx, Int mvpIdx)  { m_mvpIdx[picList][uiIdx] = (Char)mvpIdx; }

    Int           getMVPIdx(RefPicList picList, UInt uiIdx)               { return m_mvpIdx[picList][uiIdx]; }

    Char*         getMVPIdx(RefPicList picList)                          { return m_mvpIdx[picList]; }

    Void          setMVPNum(RefPicList picList, UInt uiIdx, Int iMVPNum) { m_mvpNum[picList][uiIdx] = (Char)iMVPNum; }

    Int           getMVPNum(RefPicList picList, UInt uiIdx)              { return m_mvpNum[picList][uiIdx]; }

    Char*         getMVPNum(RefPicList picList)                          { return m_mvpNum[picList]; }

    Void          setMVPIdxSubParts(Int mvpIdx, RefPicList picList, UInt absPartIdx, UInt partIdx, UInt depth);
    Void          setMVPNumSubParts(Int iMVPNum, RefPicList picList, UInt absPartIdx, UInt partIdx, UInt depth);

    Void          clipMv(x265::MV& outMV);

    Void          getMvPredLeft(x265::MV& mvPred)       { mvPred = m_mvFieldA.mv; }

    Void          getMvPredAbove(x265::MV& mvPred)      { mvPred = m_mvFieldB.mv; }

    Void          getMvPredAboveRight(x265::MV& mvPred) { mvPred = m_mvFieldC.mv; }

    Void          compressMV();

    // -------------------------------------------------------------------------------------------------------------------
    // utility functions for neighboring information
    // -------------------------------------------------------------------------------------------------------------------

    TComDataCU*   getCULeft() { return m_cuLeft; }

    TComDataCU*   getCUAbove() { return m_cuAbove; }

    TComDataCU*   getCUAboveLeft() { return m_cuAboveLeft; }

    TComDataCU*   getCUAboveRight() { return m_cuAboveRight; }

    TComDataCU*   getCUColocated(RefPicList picList) { return m_cuColocated[picList]; }

    TComDataCU*   getPULeft(UInt& uiLPartUnitIdx,
                            UInt  curPartUnitIdx,
                            Bool  bEnforceSliceRestriction = true,
                            Bool  bEnforceTileRestriction = true);
    TComDataCU*   getPUAbove(UInt& uiAPartUnitIdx,
                             UInt  curPartUnitIdx,
                             Bool  bEnforceSliceRestriction = true,
                             Bool  planarAtLCUBoundary = false,
                             Bool  bEnforceTileRestriction = true);
    TComDataCU*   getPUAboveLeft(UInt& alPartUnitIdx, UInt curPartUnitIdx, Bool bEnforceSliceRestriction = true);
    TComDataCU*   getPUAboveRight(UInt& uiARPartUnitIdx, UInt curPartUnitIdx, Bool bEnforceSliceRestriction = true);
    TComDataCU*   getPUBelowLeft(UInt& uiBLPartUnitIdx, UInt curPartUnitIdx, Bool bEnforceSliceRestriction = true);

    TComDataCU*   getQpMinCuLeft(UInt& uiLPartUnitIdx, UInt uiCurrAbsIdxInLCU);
    TComDataCU*   getQpMinCuAbove(UInt& aPartUnitIdx, UInt currAbsIdxInLCU);
    Char          getRefQP(UInt uiCurrAbsIdxInLCU);

    TComDataCU*   getPUAboveRightAdi(UInt& uiARPartUnitIdx, UInt curPartUnitIdx, UInt partUnitOffset = 1, Bool bEnforceSliceRestriction = true);
    TComDataCU*   getPUBelowLeftAdi(UInt& uiBLPartUnitIdx, UInt curPartUnitIdx, UInt partUnitOffset = 1, Bool bEnforceSliceRestriction = true);

    Void          deriveLeftRightTopIdx(UInt partIdx, UInt& ruiPartIdxLT, UInt& ruiPartIdxRT);
    Void          deriveLeftBottomIdx(UInt partIdx, UInt& ruiPartIdxLB);

    Void          deriveLeftRightTopIdxAdi(UInt& ruiPartIdxLT, UInt& ruiPartIdxRT, UInt partOffset, UInt partDepth);
    Void          deriveLeftBottomIdxAdi(UInt& ruiPartIdxLB, UInt  partOffset, UInt partDepth);

    Bool          hasEqualMotion(UInt absPartIdx, TComDataCU* pcCandCU, UInt uiCandAbsPartIdx);
    Void          getInterMergeCandidates(UInt absPartIdx, UInt uiPUIdx, TComMvField* pcMFieldNeighbours, UChar* puhInterDirNeighbours, Int& numValidMergeCand, Int mrgCandIdx = -1);
    Void          deriveLeftRightTopIdxGeneral(UInt absPartIdx, UInt partIdx, UInt& ruiPartIdxLT, UInt& ruiPartIdxRT);
    Void          deriveLeftBottomIdxGeneral(UInt absPartIdx, UInt partIdx, UInt& ruiPartIdxLB);

    // -------------------------------------------------------------------------------------------------------------------
    // member functions for modes
    // -------------------------------------------------------------------------------------------------------------------

    Bool          isIntra(UInt partIdx)  { return m_predModes[partIdx] == MODE_INTRA; }

    Bool          isSkipped(UInt partIdx);                                                      ///< SKIP (no residual)
    Bool          isBipredRestriction(UInt puIdx);

    // -------------------------------------------------------------------------------------------------------------------
    // member functions for symbol prediction (most probable / mode conversion)
    // -------------------------------------------------------------------------------------------------------------------

    UInt          getIntraSizeIdx(UInt absPartIdx);

    Void          getAllowedChromaDir(UInt absPartIdx, UInt* uiModeList);
    Int           getIntraDirLumaPredictor(UInt absPartIdx, Int* uiIntraDirPred, Int* piMode = NULL);

    // -------------------------------------------------------------------------------------------------------------------
    // member functions for SBAC context
    // -------------------------------------------------------------------------------------------------------------------

    UInt          getCtxSplitFlag(UInt absPartIdx, UInt depth);
    UInt          getCtxQtCbf(TextType ttype, UInt trDepth);

    UInt          getCtxSkipFlag(UInt absPartIdx);
    UInt          getCtxInterDir(UInt absPartIdx);

    UInt&         getTotalBins()                            { return m_totalBins; }

    // -------------------------------------------------------------------------------------------------------------------
    // member functions for RD cost storage
    // -------------------------------------------------------------------------------------------------------------------

    UInt64&       getTotalCost()                  { return m_totalCost; }

    UInt&         getTotalDistortion()            { return m_totalDistortion; }

    UInt&         getTotalBits()                  { return m_totalBits; }

    UInt&         getTotalNumPart()               { return m_numPartitions; }

    UInt          getCoefScanIdx(UInt absPartIdx, UInt width, Bool bIsLuma, Bool bIsIntra);
};

namespace RasterAddress {
/** Check whether 2 addresses point to the same column
 * \param addrA          First address in raster scan order
 * \param addrB          Second address in raters scan order
 * \param numUnitsPerRow Number of units in a row
 * \return Result of test
 */
static inline Bool isEqualCol(Int addrA, Int addrB, Int numUnitsPerRow)
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
static inline Bool isEqualRow(Int addrA, Int addrB, Int numUnitsPerRow)
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
static inline Bool isEqualRowOrCol(Int addrA, Int addrB, Int numUnitsPerRow)
{
    return isEqualCol(addrA, addrB, numUnitsPerRow) | isEqualRow(addrA, addrB, numUnitsPerRow);
}

/** Check whether one address points to the first column
 * \param addr           Address in raster scan order
 * \param numUnitsPerRow Number of units in a row
 * \return Result of test
 */
static inline Bool isZeroCol(Int addr, Int numUnitsPerRow)
{
    // addr % numUnitsPerRow == 0
    return (addr & (numUnitsPerRow - 1)) == 0;
}

/** Check whether one address points to the first row
 * \param addr           Address in raster scan order
 * \param numUnitsPerRow Number of units in a row
 * \return Result of test
 */
static inline Bool isZeroRow(Int addr, Int numUnitsPerRow)
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
static inline Bool lessThanCol(Int addr, Int val, Int numUnitsPerRow)
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
static inline Bool lessThanRow(Int addr, Int val, Int numUnitsPerRow)
{
    // addr / numUnitsPerRow < val
    return addr < val * numUnitsPerRow;
}
}

//! \}

#endif // ifndef _TCOMDATACU_
