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

class RPS
{
public:

    int  m_numberOfPictures;
    int  m_numberOfNegativePictures;
    int  m_numberOfPositivePictures;

    int  m_deltaPOC[MAX_NUM_REF_PICS];
    bool m_used[MAX_NUM_REF_PICS];
    int  m_POC[MAX_NUM_REF_PICS];

    RPS()
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

struct TComVPS
{
    uint32_t   numReorderPics;
    uint32_t   maxDecPicBuffering;
    TComHRD    hrdParameters;
    TimingInfo timingInfo;
};

struct Window
{
    bool bEnabled;
    int  leftOffset;
    int  rightOffset;
    int  topOffset;
    int  bottomOffset;

    Window()
    {
        bEnabled = false;
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

struct TComSPS
{
    int      chromaFormatIdc;        // use param
    uint32_t picWidthInLumaSamples;  // use param
    uint32_t picHeightInLumaSamples; // use param

    int      log2MinCodingBlockSize;
    int      log2DiffMaxMinCodingBlockSize;

    uint32_t quadtreeTULog2MaxSize;
    uint32_t quadtreeTULog2MinSize;

    uint32_t quadtreeTUMaxDepthInter; // use param
    uint32_t quadtreeTUMaxDepthIntra; // use param

    bool     bUseSAO; // use param
    bool     bUseAMP; // use param
    uint32_t maxAMPDepth;

    uint32_t maxDecPicBuffering; // these are dups of VPS values
    int      numReorderPics;

    bool     useStrongIntraSmoothing; // use param

    Window   conformanceWindow;
    TComVUI  vuiParameters;
};

struct TComPPS
{
    uint32_t maxCuDQPDepth;
    uint32_t minCuDQPSize;

    int      chromaCbQpOffset;       // use param
    int      chromaCrQpOffset;       // use param

    bool     bUseWeightPred;         // use param
    bool     bUseWeightedBiPred;     // use param
    bool     bUseDQP;
    bool     bConstrainedIntraPred;  // use param

    bool     bTransquantBypassEnabled;  // Indicates presence of cu_transquant_bypass_flag in CUs.
    bool     bTransformSkipEnabled;     // use param
    bool     bEntropyCodingSyncEnabled; // use param
    bool     bSignHideEnabled;          // use param

    bool     bCabacInitPresent;
    uint32_t encCABACTableIdx;          // Used to transmit table selection across slices

    bool     bDeblockingFilterControlPresent;
    bool     bPicDisableDeblockingFilter;
    int      deblockingFilterBetaOffsetDiv2;
    int      deblockingFilterTcOffsetDiv2;
};

struct WeightParam
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
};

class TComSlice
{
public:

    const TComSPS* m_sps;
    const TComPPS* m_pps;
    Frame*         m_pic;
    WeightParam    m_weightPredTable[2][MAX_NUM_REF][3]; // [list][refIdx][0:Y, 1:U, 2:V]
    RPS            m_rps;

    NalUnitType m_nalUnitType;
    SliceType   m_sliceType;
    int         m_sliceQp;
    int         m_poc;
    
    int         m_lastIDR;
    bool        m_bReferenced;

    bool        m_bCheckLDC;
    bool        m_colFromL0Flag; // collocated picture from List0 flag
    bool        m_bLMvdL1Zero;
    uint32_t    m_colRefIdx;
    uint32_t    m_maxNumMergeCand; // use param

    int         m_numRefIdx[2];
    Frame*      m_refPicList[2][MAX_NUM_REF + 1];
    int         m_refPOCList[2][MAX_NUM_REF + 1];

    uint32_t    m_sliceCurEndCUAddr;
    uint32_t    m_sliceBits;

    uint32_t*   m_substreamSizes;
    int         m_numEntryPointOffsets;

    bool        m_cabacInitFlag;

    TComSlice()
    {
        m_lastIDR = 0;
        m_nalUnitType = NAL_UNIT_CODED_SLICE_IDR_W_RADL;
        m_sliceType = I_SLICE;
        m_sliceQp = 0;
        m_bCheckLDC = false;
        m_bReferenced = false;
        m_colFromL0Flag = 1;
        m_colRefIdx = 0;
        m_sliceCurEndCUAddr = 0;
        m_sliceBits = 0;
        m_substreamSizes = NULL;
        m_cabacInitFlag = false;
        m_bLMvdL1Zero = false;
        m_numEntryPointOffsets = 0;
        m_numRefIdx[0] = m_numRefIdx[1] = 0;

        for (int i = 0; i < MAX_NUM_REF; i++)
        {
            m_refPicList[0][i] = NULL;
            m_refPicList[1][i] = NULL;
            m_refPOCList[0][i] = 0;
            m_refPOCList[1][i] = 0;
        }

        resetWpScaling();
    }

    ~TComSlice();

    void initSlice()
    {
        m_numRefIdx[0] = 0;
        m_numRefIdx[1] = 0;
        m_colFromL0Flag = 1;
        m_colRefIdx = 0;
        m_bCheckLDC = false;
        m_cabacInitFlag = false;
        m_numEntryPointOffsets = 0;
    }

    bool getRapPicFlag() const
    {
        return m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_W_RADL
            || m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_N_LP
            || m_nalUnitType == NAL_UNIT_CODED_SLICE_BLA_N_LP
            || m_nalUnitType == NAL_UNIT_CODED_SLICE_BLA_W_RADL
            || m_nalUnitType == NAL_UNIT_CODED_SLICE_BLA_W_LP
            || m_nalUnitType == NAL_UNIT_CODED_SLICE_CRA;
    }

    bool getIdrPicFlag() const
    {
        return m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_W_RADL
            || m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_N_LP;
    }

    bool isIRAP() const
    {
        return m_nalUnitType >= 16 && m_nalUnitType <= 23;
    }

    void      setLastIDR(int idrPoc)              { m_lastIDR = idrPoc; }

    int       getLastIDR()                        { return m_lastIDR; }

    SliceType getSliceType()                      { return m_sliceType; }

    int       getSliceQp()                        { return m_sliceQp; }

    int       getNumRefIdx(int e)                 { return m_numRefIdx[e]; }

    const int* getNumRefIdx() const               { return m_numRefIdx; }

    Frame*    getRefPic(int e, int refIdx)        { return m_refPicList[e][refIdx]; }

    int       getRefPOC(int e, int refIdx)        { return m_refPOCList[e][refIdx]; }

    bool      getColFromL0Flag()                  { return m_colFromL0Flag; }

    uint32_t  getColRefIdx()                      { return m_colRefIdx; }

    bool      getCheckLDC()                       { return m_bCheckLDC; }

    bool      getMvdL1ZeroFlag()                  { return m_bLMvdL1Zero; }

    int       getNumRpsCurrTempList();

    void      setReferenced(bool b)            { m_bReferenced = b; }

    bool      isReferenced()                   { return m_bReferenced; }

    void      setSliceType(SliceType e)               { m_sliceType = e; }

    void      setSliceQp(int i)                       { m_sliceQp = i; }

    void      setRefPic(Frame* p, int e, int refIdx) { m_refPicList[e][refIdx] = p; }

    void      setRefPOC(int i, int e, int refIdx) { m_refPOCList[e][refIdx] = i; }

    void      setNumRefIdx(int e, int i) { m_numRefIdx[e] = i; }

    void      setRefPicList(PicList& picList);

    void      setRefPOCList();

    void      setColFromL0Flag(bool colFromL0) { m_colFromL0Flag = colFromL0; }

    void      setColRefIdx(uint32_t refIdx)     { m_colRefIdx = refIdx; }

    void      setCheckLDC(bool b)           { m_bCheckLDC = b; }

    void      setMvdL1ZeroFlag(bool b)      { m_bLMvdL1Zero = b; }

    bool      isIntra()                     { return m_sliceType == I_SLICE; }

    bool      isInterB()                    { return m_sliceType == B_SLICE; }

    bool      isInterP()                    { return m_sliceType == P_SLICE; }

    void setSliceCurEndCUAddr(uint32_t uiAddr) { m_sliceCurEndCUAddr = uiAddr; }

    uint32_t getSliceCurEndCUAddr()            { return m_sliceCurEndCUAddr; }

    void setSliceBits(uint32_t val)            { m_sliceBits = val; }

    uint32_t getSliceBits()                    { return m_sliceBits; }

    void  setWpScaling(WeightParam wp[2][MAX_NUM_REF][3]) { memcpy(m_weightPredTable, wp, sizeof(WeightParam) * 2 * MAX_NUM_REF * 3); }

    void  getWpScaling(int e, int refIdx, WeightParam *&wp);

    void  resetWpScaling();
    void  initWpScaling();

    void allocSubstreamSizes(uint32_t numStreams);
    uint32_t* getSubstreamSizes()              { return m_substreamSizes; }

    void  setCabacInitFlag(bool val)   { m_cabacInitFlag = val; }   //!< set CABAC initial flag

    bool  getCabacInitFlag()           { return m_cabacInitFlag; }  //!< get CABAC initial flag

    void  setNumEntryPointOffsets(int val)  { m_numEntryPointOffsets = val; }

    int   getNumEntryPointOffsets()         { return m_numEntryPointOffsets; }

protected:

    Frame*  xGetRefPic(PicList& picList, int poc);

}; // END CLASS DEFINITION TComSlice
}
//! \}

#endif // ifndef X265_TCOMSLICE_H
