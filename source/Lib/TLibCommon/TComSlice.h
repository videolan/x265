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

/** \file     TComSlice.h
    \brief    slice header and SPS class (header)
*/

#ifndef X265_TCOMSLICE_H
#define X265_TCOMSLICE_H

#include "common.h"
#include "TComRom.h"

namespace x265 {
// private namespace

class Frame;
class PicList;

class TComReferencePictureSet
{
public:

    int  m_numberOfPictures;
    int  m_numberOfNegativePictures;
    int  m_numberOfPositivePictures;

    int  m_deltaPOC[MAX_NUM_REF_PICS];
    bool m_used[MAX_NUM_REF_PICS];
    int  m_POC[MAX_NUM_REF_PICS];

    TComReferencePictureSet()
        : m_numberOfPictures(0)
        , m_numberOfNegativePictures(0)
        , m_numberOfPositivePictures(0)
    {
        ::memset(m_deltaPOC, 0, sizeof(m_deltaPOC));
        ::memset(m_POC, 0, sizeof(m_POC));
        ::memset(m_used, 0, sizeof(m_used));
    }

    void sortDeltaPOC();
};

class TComScalingList
{
public:

    uint32_t m_refMatrixId[SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM];     // used during coding
    int      m_scalingListDC[SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM];   // the DC value of the matrix coefficient for 16x16
    int     *m_scalingListCoef[SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM]; // quantization matrix
    bool     m_bEnabled;
    bool     m_bDataPresent; // non-default scaling lists must be signaled

    TComScalingList();
    ~TComScalingList();

    int32_t* getScalingListDefaultAddress(uint32_t sizeId, uint32_t listId);
    void     processRefMatrix(uint32_t sizeId, uint32_t listId, uint32_t refListId);

    bool     checkDefaultScalingList();
    void     checkDcOfMatrix();
    void     setDefaultScalingList();

    bool     checkPredMode(uint32_t sizeId, int listId);
    bool     parseScalingList(char* filename);

protected:
    void     processDefaultMarix(uint32_t sizeId, uint32_t listId);
};

struct ProfileTierLevel
{
    bool    tierFlag;
    int     profileIdc;
    bool    profileCompatibilityFlag[32];
    int     levelIdc;
    bool    progressiveSourceFlag;
    bool    interlacedSourceFlag;
    bool    nonPackedConstraintFlag;
    bool    frameOnlyConstraintFlag;

    ProfileTierLevel()
        : tierFlag(false)
        , profileIdc(0)
        , levelIdc(0)
        , progressiveSourceFlag(false)
        , interlacedSourceFlag(false)
        , nonPackedConstraintFlag(false)
        , frameOnlyConstraintFlag(false)
    {
        ::memset(profileCompatibilityFlag, 0, sizeof(profileCompatibilityFlag));
    }
};

struct TComHRD
{
    uint32_t bitRateScale;
    uint32_t cpbSizeScale;
    uint32_t initialCpbRemovalDelayLength;
    uint32_t cpbRemovalDelayLength;
    uint32_t dpbOutputDelayLength;
    uint32_t bitRateValue;
    uint32_t cpbSizeValue;
    bool     cbrFlag;

    TComHRD()
        : bitRateScale(0)
        , cpbSizeScale(0)
        , initialCpbRemovalDelayLength(1)
        , cpbRemovalDelayLength(1)
        , dpbOutputDelayLength(1)
        , cbrFlag(false)
    {
    }
};

struct TimingInfo
{
    bool     timingInfoPresentFlag;
    uint32_t numUnitsInTick;
    uint32_t timeScale;

    TimingInfo()
        : timingInfoPresentFlag(false)
        , numUnitsInTick(1001)
        , timeScale(60000)
    {
    }
};

struct HRDTiming
{
    double cpbInitialAT;
    double cpbFinalAT;
    double dpbOutputTime;
    double cpbRemovalTime;
};

class TComVPS
{
public:

    uint32_t    m_numReorderPics;
    uint32_t    m_maxDecPicBuffering;
    TComHRD     m_hrdParameters;
    TimingInfo  m_timingInfo;

    TComVPS()
    {
        m_numReorderPics = 0;
        m_maxDecPicBuffering = 1;
    }
};

struct Window
{
public:

    bool          bEnabled;
    int           leftOffset;
    int           rightOffset;
    int           topOffset;
    int           bottomOffset;

    Window()
        : bEnabled(false)
    {
    }
};

struct TComVUI
{
    bool       aspectRatioInfoPresentFlag;
    int        aspectRatioIdc;
    int        sarWidth;
    int        sarHeight;

    bool       overscanInfoPresentFlag;
    bool       overscanAppropriateFlag;

    bool       videoSignalTypePresentFlag;
    int        videoFormat;
    bool       videoFullRangeFlag;

    bool       colourDescriptionPresentFlag;
    int        colourPrimaries;
    int        transferCharacteristics;
    int        matrixCoefficients;

    bool       chromaLocInfoPresentFlag;
    int        chromaSampleLocTypeTopField;
    int        chromaSampleLocTypeBottomField;

    Window     defaultDisplayWindow;

    bool       frameFieldInfoPresentFlag;
    bool       fieldSeqFlag;

    bool       hrdParametersPresentFlag;
    TComHRD    hrdParameters;

    TimingInfo timingInfo;
};

class TComSPS
{
public:

    int         m_chromaFormatIdc;

    // Structure
    uint32_t    m_picWidthInLumaSamples;
    uint32_t    m_picHeightInLumaSamples;

    int         m_log2MinCodingBlockSize;
    int         m_log2DiffMaxMinCodingBlockSize;
    uint32_t    m_maxCUSize;
    uint32_t    m_maxCUDepth;

    Window      m_conformanceWindow;

    bool        m_TMVPFlagsPresent;
    int         m_numReorderPics;

    // Tool list
    uint32_t    m_quadtreeTULog2MaxSize;
    uint32_t    m_quadtreeTULog2MinSize;
    uint32_t    m_quadtreeTUMaxDepthInter;
    uint32_t    m_quadtreeTUMaxDepthIntra;

    // Parameter
    int         m_bitDepthY;
    int         m_bitDepthC;
    int         m_qpBDOffsetY;
    int         m_qpBDOffsetC;

    uint32_t    m_bitsForPOC;

    bool        m_bUseSAO;
    bool        m_useAMP;
    uint32_t    m_maxAMPDepth;

    uint32_t    m_maxDecPicBuffering;
    uint32_t    m_maxLatencyIncrease; // Really max latency increase plus 1 (value 0 expresses no limit)

    bool        m_useStrongIntraSmoothing;

    TComVUI     m_vuiParameters;

    TComSPS();

    int  getChromaFormatIdc()         { return m_chromaFormatIdc; }

    void setChromaFormatIdc(int i)    { m_chromaFormatIdc = i; }

    // structure
    void setPicWidthInLumaSamples(uint32_t u) { m_picWidthInLumaSamples = u; }

    uint32_t getPicWidthInLumaSamples() const { return m_picWidthInLumaSamples; }

    void setPicHeightInLumaSamples(uint32_t u) { m_picHeightInLumaSamples = u; }

    uint32_t getPicHeightInLumaSamples() const { return m_picHeightInLumaSamples; }

    Window& getConformanceWindow() { return m_conformanceWindow; }

    void    setConformanceWindow(Window& conformanceWindow) { m_conformanceWindow = conformanceWindow; }

    int  getLog2MinCodingBlockSize() const { return m_log2MinCodingBlockSize; }

    void setLog2MinCodingBlockSize(int val) { m_log2MinCodingBlockSize = val; }

    int  getLog2DiffMaxMinCodingBlockSize() const { return m_log2DiffMaxMinCodingBlockSize; }

    void setLog2DiffMaxMinCodingBlockSize(int val) { m_log2DiffMaxMinCodingBlockSize = val; }

    int  getLog2MaxCodingBlockSize() const { return m_log2MinCodingBlockSize + m_log2DiffMaxMinCodingBlockSize; }

    void setMaxCUSize(uint32_t u) { m_maxCUSize = u; }

    uint32_t getMaxCUSize() const  { return m_maxCUSize; }

    void setMaxCUDepth(uint32_t u) { m_maxCUDepth = u; }

    uint32_t getMaxCUDepth() const { return m_maxCUDepth; }

    void setBitsForPOC(uint32_t u) { m_bitsForPOC = u; }

    uint32_t getBitsForPOC() const { return m_bitsForPOC; }

    bool getUseAMP() { return m_useAMP; }

    void setUseAMP(bool b) { m_useAMP = b; }

    void setQuadtreeTULog2MaxSize(uint32_t u)   { m_quadtreeTULog2MaxSize = u; }

    uint32_t getQuadtreeTULog2MaxSize() const   { return m_quadtreeTULog2MaxSize; }

    void setQuadtreeTULog2MinSize(uint32_t u)   { m_quadtreeTULog2MinSize = u; }

    uint32_t getQuadtreeTULog2MinSize() const   { return m_quadtreeTULog2MinSize; }

    void setQuadtreeTUMaxDepthInter(uint32_t u) { m_quadtreeTUMaxDepthInter = u; }

    void setQuadtreeTUMaxDepthIntra(uint32_t u) { m_quadtreeTUMaxDepthIntra = u; }

    uint32_t getQuadtreeTUMaxDepthInter() const   { return m_quadtreeTUMaxDepthInter; }

    uint32_t getQuadtreeTUMaxDepthIntra() const    { return m_quadtreeTUMaxDepthIntra; }

    bool      getTMVPFlagsPresent() const   { return m_TMVPFlagsPresent; }

    void      setTMVPFlagsPresent(bool b)   { m_TMVPFlagsPresent = b; }

    // AMP accuracy
    int       getAMPAcc(uint32_t depth) const { return m_maxAMPDepth > depth && m_useAMP; }

    void      setAMPAcc(uint32_t depth) { m_maxAMPDepth = depth; }

    // Bit-depth
    int       getBitDepthY() const { return m_bitDepthY; }

    void      setBitDepthY(int u) { m_bitDepthY = u; }

    int       getBitDepthC() const { return m_bitDepthC; }

    void      setBitDepthC(int u) { m_bitDepthC = u; }

    int       getQpBDOffsetY() const { return m_qpBDOffsetY; }

    void      setQpBDOffsetY(int value) { m_qpBDOffsetY = value; }

    int       getQpBDOffsetC() const { return m_qpBDOffsetC; }

    void      setQpBDOffsetC(int value) { m_qpBDOffsetC = value; }

    void      setUseSAO(bool bVal)  { m_bUseSAO = bVal; }

    bool      getUseSAO() const { return m_bUseSAO; }

    uint32_t getMaxDecPicBuffering() { return m_maxDecPicBuffering; }

    void setMaxDecPicBuffering(uint32_t ui) { m_maxDecPicBuffering = ui; }

    uint32_t getMaxLatencyIncrease() { return m_maxLatencyIncrease; }

    void setMaxLatencyIncrease(uint32_t ui) { m_maxLatencyIncrease = ui; }

    void setNumReorderPics(int i) { m_numReorderPics = i; }

    int  getNumReorderPics() const  { return m_numReorderPics; }

    void setUseStrongIntraSmoothing(bool bVal) { m_useStrongIntraSmoothing = bVal; }

    bool getUseStrongIntraSmoothing() const { return m_useStrongIntraSmoothing; }

    TComVUI* getVuiParameters() { return &m_vuiParameters; }

    void setHrdParameters(uint32_t fpsNum, uint32_t fpsDenom, uint32_t numDU, uint32_t bitRate, bool randomAccess);
};

/// PPS class
class TComPPS
{
private:

    int      m_picInitQPMinus26;
    bool     m_useDQP;
    bool     m_bConstrainedIntraPred;  // constrained_intra_pred_flag
    bool     m_bSliceChromaQpFlag;     // slicelevel_chroma_qp_flag

    // access channel
    TComSPS* m_sps;
    uint32_t m_maxCuDQPDepth;
    uint32_t m_minCuDQPSize;

    int      m_chromaCbQpOffset;
    int      m_chromaCrQpOffset;

    uint32_t m_numRefIdxL0DefaultActive;
    uint32_t m_numRefIdxL1DefaultActive;

    bool     m_bUseWeightPred;         // Use of Weighting Prediction (P_SLICE)
    bool     m_useWeightedBiPred;      // Use of Weighting Bi-Prediction (B_SLICE)

    bool     m_transquantBypassEnableFlag; // Indicates presence of cu_transquant_bypass_flag in CUs.
    bool     m_useTransformSkip;
    bool     m_entropyCodingSyncEnabledFlag; //!< Indicates the presence of wavefronts

    bool     m_signHideFlag;

    bool     m_cabacInitPresentFlag;
    uint32_t m_encCABACTableIdx;         // Used to transmit table selection across slices

    bool     m_deblockingFilterControlPresentFlag;
    bool     m_deblockingFilterOverrideEnabledFlag;
    bool     m_picDisableDeblockingFilterFlag;
    int      m_deblockingFilterBetaOffsetDiv2;  //< beta offset for deblocking filter
    int      m_deblockingFilterTcOffsetDiv2;    //< tc offset for deblocking filter

    uint32_t m_log2ParallelMergeLevelMinus2;
    int      m_numExtraSliceHeaderBits;

public:

    TComPPS();

    int       getPicInitQPMinus26() const { return m_picInitQPMinus26; }

    void      setPicInitQPMinus26(int i)  { m_picInitQPMinus26 = i; }

    bool      getUseDQP() const           { return m_useDQP; }

    void      setUseDQP(bool b)           { m_useDQP = b; }

    bool      getConstrainedIntraPred() const { return m_bConstrainedIntraPred; }

    void      setConstrainedIntraPred(bool b) { m_bConstrainedIntraPred = b; }

    bool      getSliceChromaQpFlag() const { return m_bSliceChromaQpFlag; }

    void      setSliceChromaQpFlag(bool b) { m_bSliceChromaQpFlag = b; }

    void      setSPS(TComSPS* sps) { m_sps = sps; }

    TComSPS*  getSPS() { return m_sps; }

    void      setMaxCuDQPDepth(uint32_t u) { m_maxCuDQPDepth = u; }

    uint32_t  getMaxCuDQPDepth() const { return m_maxCuDQPDepth; }

    void      setMinCuDQPSize(uint32_t u) { m_minCuDQPSize = u; }

    uint32_t  getMinCuDQPSize() const { return m_minCuDQPSize; }

    void      setChromaCbQpOffset(int i) { m_chromaCbQpOffset = i; }

    int       getChromaCbQpOffset() const { return m_chromaCbQpOffset; }

    void      setChromaCrQpOffset(int i) { m_chromaCrQpOffset = i; }

    int       getChromaCrQpOffset() const { return m_chromaCrQpOffset; }

    void      setNumRefIdxL0DefaultActive(uint32_t i) { m_numRefIdxL0DefaultActive = i; }

    uint32_t  getNumRefIdxL0DefaultActive() const     { return m_numRefIdxL0DefaultActive; }

    void      setNumRefIdxL1DefaultActive(uint32_t i) { m_numRefIdxL1DefaultActive = i; }

    uint32_t  getNumRefIdxL1DefaultActive() const     { return m_numRefIdxL1DefaultActive; }

    bool getUseWP() const    { return m_bUseWeightPred; }

    bool getWPBiPred() const { return m_useWeightedBiPred; }

    void setUseWP(bool b)    { m_bUseWeightPred = b; }

    void setWPBiPred(bool b) { m_useWeightedBiPred = b; }

    void     setTransquantBypassEnableFlag(bool b) { m_transquantBypassEnableFlag = b; }

    bool     getTransquantBypassEnableFlag() const { return m_transquantBypassEnableFlag; }

    bool     getUseTransformSkip() const { return m_useTransformSkip; }

    void     setUseTransformSkip(bool b) { m_useTransformSkip = b; }

    bool     getEntropyCodingSyncEnabledFlag() const    { return m_entropyCodingSyncEnabledFlag; }

    void     setEntropyCodingSyncEnabledFlag(bool val)  { m_entropyCodingSyncEnabledFlag = val; }

    void     setSignHideFlag(bool signHideFlag)      { m_signHideFlag = signHideFlag; }

    bool     getSignHideFlag() const                 { return m_signHideFlag; }

    void     setCabacInitPresentFlag(bool flag)     { m_cabacInitPresentFlag = flag; }

    void     setEncCABACTableIdx(int idx)           { m_encCABACTableIdx = idx; }

    bool     getCabacInitPresentFlag() const        { return m_cabacInitPresentFlag; }

    uint32_t getEncCABACTableIdx() const            { return m_encCABACTableIdx; }

    void     setDeblockingFilterControlPresentFlag(bool val)  { m_deblockingFilterControlPresentFlag = val; }

    bool     getDeblockingFilterControlPresentFlag() const    { return m_deblockingFilterControlPresentFlag; }

    void     setDeblockingFilterOverrideEnabledFlag(bool val) { m_deblockingFilterOverrideEnabledFlag = val; }

    bool     getDeblockingFilterOverrideEnabledFlag() const   { return m_deblockingFilterOverrideEnabledFlag; }

    void     setPicDisableDeblockingFilterFlag(bool val)      { m_picDisableDeblockingFilterFlag = val; }     //!< set offset for deblocking filter disabled

    bool     getPicDisableDeblockingFilterFlag() const        { return m_picDisableDeblockingFilterFlag; }    //!< get offset for deblocking filter disabled

    void     setDeblockingFilterBetaOffsetDiv2(int val)       { m_deblockingFilterBetaOffsetDiv2 = val; }     //!< set beta offset for deblocking filter

    int      getDeblockingFilterBetaOffsetDiv2() const        { return m_deblockingFilterBetaOffsetDiv2; }    //!< get beta offset for deblocking filter

    void     setDeblockingFilterTcOffsetDiv2(int val)         { m_deblockingFilterTcOffsetDiv2 = val; }       //!< set tc offset for deblocking filter

    int      getDeblockingFilterTcOffsetDiv2()                { return m_deblockingFilterTcOffsetDiv2; }      //!< get tc offset for deblocking filter

    uint32_t getLog2ParallelMergeLevelMinus2() const { return m_log2ParallelMergeLevelMinus2; }

    void setLog2ParallelMergeLevelMinus2(uint32_t mrgLevel) { m_log2ParallelMergeLevelMinus2 = mrgLevel; }

    int getNumExtraSliceHeaderBits() const { return m_numExtraSliceHeaderBits; }

    void setNumExtraSliceHeaderBits(int i) { m_numExtraSliceHeaderBits = i; }
};

typedef struct wpScalingParam
{
    // Explicit weighted prediction parameters parsed in slice header,
    // or Implicit weighted prediction parameters (8 bits depth values).
    bool        bPresentFlag;
    uint32_t    log2WeightDenom;
    int         inputWeight;
    int         inputOffset;

    // Weighted prediction scaling values built from above parameters (bitdepth scaled):
    int         w, o, offset, shift, round;

    /* makes a non-h265 weight (i.e. fix7), into an h265 weight */
    void setFromWeightAndOffset(int weight, int _offset, int denom, bool bNormalize)
    {
        inputOffset = _offset;
        log2WeightDenom = denom;
        inputWeight = weight;
        while (bNormalize && log2WeightDenom > 0 && (inputWeight > 127))
        {
            log2WeightDenom--;
            inputWeight >>= 1;
        }

        inputWeight = X265_MIN(inputWeight, 127);
    }
} wpScalingParam;

/// slice header class
class TComSlice
{
private:

    //  Bitstream writing
    bool        m_saoEnabledFlag;
    bool        m_saoEnabledFlagChroma; ///< SAO Cb&Cr enabled flag
    int         m_poc;
    int         m_lastIDR;

    TComReferencePictureSet *m_rps;
    TComReferencePictureSet m_localRPS;
    int         m_bdIdx;
    NalUnitType m_nalUnitType;       ///< Nal unit type for the slice
    SliceType   m_sliceType;
    int         m_sliceQp;
    bool        m_deblockingFilterDisable;
    bool        m_deblockingFilterOverrideFlag;    //< offsets for deblocking filter inherit from PPS
    int         m_deblockingFilterBetaOffsetDiv2;  //< beta offset for deblocking filter
    int         m_deblockingFilterTcOffsetDiv2;    //< tc offset for deblocking filter
    int         m_numRefIdx[2];     //  for multiple reference of current slice

    bool        m_bCheckLDC;

    //  Data
    int         m_sliceQpDelta;
    int         m_sliceQpDeltaCb;
    int         m_sliceQpDeltaCr;
    Frame*      m_refPicList[2][MAX_NUM_REF + 1];
    int         m_refPOCList[2][MAX_NUM_REF + 1];

    // referenced slice?
    bool        m_bReferenced;

    // access channel
    TComSPS*    m_sps;
    TComPPS*    m_pps;
    TComVPS*    m_vps;
    Frame*      m_pic;
    bool        m_colFromL0Flag; // collocated picture from List0 flag

    uint32_t    m_colRefIdx;
    uint32_t    m_maxNumMergeCand;

    uint32_t    m_sliceCurEndCUAddr;
    uint32_t    m_sliceBits;
    uint32_t    m_sliceSegmentBits;

    uint32_t*   m_substreamSizes;
    bool        m_cabacInitFlag;

    bool       m_bLMvdL1Zero;
    int        m_numEntryPointOffsets;

    bool       m_enableTMVPFlag;

public:

    wpScalingParam  m_weightPredTable[2][MAX_NUM_REF][3]; // [REF_PIC_LIST_0 or REF_PIC_LIST_1][refIdx][0:Y, 1:U, 2:V]

    TComSlice();
    ~TComSlice();

    void      initSlice();

    void      setVPS(TComVPS* vps)            { m_vps = vps; }

    TComVPS*  getVPS()                        { return m_vps; }

    void      setSPS(TComSPS* sps)            { m_sps = sps; }

    TComSPS*  getSPS()                        { return m_sps; }

    void      setPPS(TComPPS* pps)            { m_pps = pps; }

    TComPPS*  getPPS()                        { return m_pps; }

    void      setSaoEnabledFlag(bool s)       { m_saoEnabledFlag = s; }

    bool      getSaoEnabledFlag()             { return m_saoEnabledFlag; }

    void      setSaoEnabledFlagChroma(bool s) { m_saoEnabledFlagChroma = s; }

    bool      getSaoEnabledFlagChroma()       { return m_saoEnabledFlagChroma; }

    void      setRPS(TComReferencePictureSet *rps) { m_rps = rps; }

    TComReferencePictureSet*  getRPS()            { return m_rps; }

    TComReferencePictureSet*  getLocalRPS()       { return &m_localRPS; }

    void      setRPSidx(int bdidx)                { m_bdIdx = bdidx; }

    int       getRPSidx()                         { return m_bdIdx; }

    void      setLastIDR(int idrPoc)              { m_lastIDR = idrPoc; }

    int       getLastIDR()                        { return m_lastIDR; }

    SliceType getSliceType()                      { return m_sliceType; }

    int       getPOC()                            { return m_poc; }

    int       getSliceQp()                        { return m_sliceQp; }

    int       getSliceQpDelta()                   { return m_sliceQpDelta; }

    int       getSliceQpDeltaCb()                 { return m_sliceQpDeltaCb; }

    int       getSliceQpDeltaCr()                 { return m_sliceQpDeltaCr; }

    bool      getDeblockingFilterDisable()        { return m_deblockingFilterDisable; }

    bool      getDeblockingFilterOverrideFlag()   { return m_deblockingFilterOverrideFlag; }

    int       getDeblockingFilterBetaOffsetDiv2() { return m_deblockingFilterBetaOffsetDiv2; }

    int       getDeblockingFilterTcOffsetDiv2()   { return m_deblockingFilterTcOffsetDiv2; }

    int       getNumRefIdx(int e)                 { return m_numRefIdx[e]; }

    const int* getNumRefIdx() const               { return m_numRefIdx; }

    Frame*    getPic()                            { return m_pic; }

    Frame*    getRefPic(int e, int refIdx)        { return m_refPicList[e][refIdx]; }

    int       getRefPOC(int e, int refIdx)        { return m_refPOCList[e][refIdx]; }

    bool      getColFromL0Flag()                  { return m_colFromL0Flag; }

    uint32_t  getColRefIdx()                      { return m_colRefIdx; }

    bool      getCheckLDC()                       { return m_bCheckLDC; }

    bool      getMvdL1ZeroFlag()                  { return m_bLMvdL1Zero; }

    int       getNumRpsCurrTempList();

    void      setReferenced(bool b)            { m_bReferenced = b; }

    bool      isReferenced()                   { return m_bReferenced; }

    void      setPOC(int i)                    { m_poc = i; }

    void      setNalUnitType(NalUnitType e)    { m_nalUnitType = e; }

    NalUnitType getNalUnitType() const         { return m_nalUnitType; }

    bool      getRapPicFlag();
    bool      getIdrPicFlag()
    {
        return getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL ||
               getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_N_LP;
    }

    bool      isIRAP() const                   { return (getNalUnitType() >= 16) && (getNalUnitType() <= 23); }

    void      setSliceType(SliceType e)               { m_sliceType = e; }

    void      setSliceQp(int i)                       { m_sliceQp = i; }

    void      setSliceQpDelta(int i)                  { m_sliceQpDelta = i; }

    void      setSliceQpDeltaCb(int i)                { m_sliceQpDeltaCb = i; }

    void      setSliceQpDeltaCr(int i)                { m_sliceQpDeltaCr = i; }

    void      setDeblockingFilterDisable(bool b)      { m_deblockingFilterDisable = b; }

    void      setDeblockingFilterOverrideFlag(bool b) { m_deblockingFilterOverrideFlag = b; }

    void      setDeblockingFilterBetaOffsetDiv2(int i) { m_deblockingFilterBetaOffsetDiv2 = i; }

    void      setDeblockingFilterTcOffsetDiv2(int i)   { m_deblockingFilterTcOffsetDiv2 = i; }

    void      setRefPic(Frame* p, int e, int refIdx) { m_refPicList[e][refIdx] = p; }

    void      setRefPOC(int i, int e, int refIdx) { m_refPOCList[e][refIdx] = i; }

    void      setNumRefIdx(int e, int i) { m_numRefIdx[e] = i; }

    void      setPic(Frame* p)           { m_pic = p; }

    void      setRefPicList(PicList& picList);

    void      setRefPOCList();

    void      setColFromL0Flag(bool colFromL0) { m_colFromL0Flag = colFromL0; }

    void      setColRefIdx(uint32_t refIdx)     { m_colRefIdx = refIdx; }

    void      setCheckLDC(bool b)           { m_bCheckLDC = b; }

    void      setMvdL1ZeroFlag(bool b)      { m_bLMvdL1Zero = b; }

    bool      isIntra()                     { return m_sliceType == I_SLICE; }

    bool      isInterB()                    { return m_sliceType == B_SLICE; }

    bool      isInterP()                    { return m_sliceType == P_SLICE; }

    void setMaxNumMergeCand(uint32_t val)      { m_maxNumMergeCand = val; }

    uint32_t getMaxNumMergeCand()              { return m_maxNumMergeCand; }

    void setSliceCurEndCUAddr(uint32_t uiAddr) { m_sliceCurEndCUAddr = uiAddr; }

    uint32_t getSliceCurEndCUAddr()            { return m_sliceCurEndCUAddr; }

    void setSliceBits(uint32_t val)            { m_sliceBits = val; }

    uint32_t getSliceBits()                    { return m_sliceBits; }

    void setSliceSegmentBits(uint32_t val)     { m_sliceSegmentBits = val; }

    uint32_t getSliceSegmentBits()             { return m_sliceSegmentBits; }

    void  setWpScaling(wpScalingParam wp[2][MAX_NUM_REF][3]) { memcpy(m_weightPredTable, wp, sizeof(wpScalingParam) * 2 * MAX_NUM_REF * 3); }

    void  getWpScaling(int e, int refIdx, wpScalingParam *&wp);

    void  resetWpScaling();
    void  initWpScaling();

    void allocSubstreamSizes(uint32_t numStreams);
    uint32_t* getSubstreamSizes()              { return m_substreamSizes; }

    void  setCabacInitFlag(bool val)   { m_cabacInitFlag = val; }   //!< set CABAC initial flag

    bool  getCabacInitFlag()           { return m_cabacInitFlag; }  //!< get CABAC initial flag

    void  setNumEntryPointOffsets(int val)  { m_numEntryPointOffsets = val; }

    int   getNumEntryPointOffsets()         { return m_numEntryPointOffsets; }

    void  setEnableTMVPFlag(bool b)    { m_enableTMVPFlag = b; }

    bool  getEnableTMVPFlag()          { return m_enableTMVPFlag; }

protected:

    Frame*  xGetRefPic(PicList& picList, int poc);

}; // END CLASS DEFINITION TComSlice
}
//! \}

#endif // ifndef X265_TCOMSLICE_H
