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

#ifndef __TCOMSLICE__
#define __TCOMSLICE__

#include "CommonDef.h"
#include "TComRom.h"
#include "TComList.h"
#include "x265.h"  // NAL type enums
#include "reference.h"

#include <cstring>
#include <map>
#include <vector>

//! \ingroup TLibCommon
//! \{

namespace x265 {
// private namespace

class TComPic;
class TComTrQuant;
// ====================================================================================================================
// Constants
// ====================================================================================================================

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// Reference Picture Set class
class TComReferencePictureSet
{
private:

    // Parameters for inter RPS prediction
    Int  m_deltaRIdxMinus1;
    Int  m_deltaRPS;
    Int  m_numRefIdc;
    Int  m_refIdc[MAX_NUM_REF_PICS + 1];

    // Parameters for long term references
    Bool m_bCheckLTMSB[MAX_NUM_REF_PICS];
    Int  m_pocLSBLT[MAX_NUM_REF_PICS];
    Int  m_deltaPOCMSBCycleLT[MAX_NUM_REF_PICS];
    Bool m_deltaPocMSBPresentFlag[MAX_NUM_REF_PICS];

public:

    Int  m_numberOfPictures;
    Int  m_numberOfNegativePictures;
    Int  m_numberOfPositivePictures;
    Int  m_deltaPOC[MAX_NUM_REF_PICS];
    Bool m_used[MAX_NUM_REF_PICS];
    Int  m_POC[MAX_NUM_REF_PICS];

    Bool m_interRPSPrediction;
    Int  m_numberOfLongtermPictures;          // Zero when disabled


    TComReferencePictureSet();
    virtual ~TComReferencePictureSet();
    Int   getPocLSBLT(Int i)                       { return m_pocLSBLT[i]; }

    void  setPocLSBLT(Int i, Int x)                { m_pocLSBLT[i] = x; }

    Int   getDeltaPocMSBCycleLT(Int i)             { return m_deltaPOCMSBCycleLT[i]; }

    void  setDeltaPocMSBCycleLT(Int i, Int x)      { m_deltaPOCMSBCycleLT[i] = x; }

    Bool  getDeltaPocMSBPresentFlag(Int i)         { return m_deltaPocMSBPresentFlag[i]; }

    void  setDeltaPocMSBPresentFlag(Int i, Bool x) { m_deltaPocMSBPresentFlag[i] = x; }

    void setUsed(Int bufferNum, Bool used);
    void setDeltaPOC(Int bufferNum, Int deltaPOC);
    void setPOC(Int bufferNum, Int deltaPOC);
    void setNumberOfPictures(Int numberOfPictures);
    void setCheckLTMSBPresent(Int bufferNum, Bool b);
    Bool getCheckLTMSBPresent(Int bufferNum);

    Int  getUsed(Int bufferNum) const;
    Int  getDeltaPOC(Int bufferNum) const;
    Int  getPOC(Int bufferNum) const;
    Int  getNumberOfPictures() const;

    void setNumberOfNegativePictures(Int number)  { m_numberOfNegativePictures = number; }

    Int  getNumberOfNegativePictures() const      { return m_numberOfNegativePictures; }

    void setNumberOfPositivePictures(Int number)  { m_numberOfPositivePictures = number; }

    Int  getNumberOfPositivePictures() const      { return m_numberOfPositivePictures; }

    void setNumberOfLongtermPictures(Int number)  { m_numberOfLongtermPictures = number; }

    Int  getNumberOfLongtermPictures() const      { return m_numberOfLongtermPictures; }

    void setInterRPSPrediction(Bool flag)         { m_interRPSPrediction = flag; }

    Bool getInterRPSPrediction() const            { return m_interRPSPrediction; }

    void setDeltaRIdxMinus1(Int x)                { m_deltaRIdxMinus1 = x; }

    Int  getDeltaRIdxMinus1() const               { return m_deltaRIdxMinus1; }

    void setDeltaRPS(Int x)                       { m_deltaRPS = x; }

    Int  getDeltaRPS() const                      { return m_deltaRPS; }

    void setNumRefIdc(Int x)                      { m_numRefIdc = x; }

    Int  getNumRefIdc() const                     { return m_numRefIdc; }

    void setRefIdc(Int bufferNum, Int refIdc);
    Int  getRefIdc(Int bufferNum) const;

    void sortDeltaPOC();
    void printDeltaPOC();
};

/// Reference Picture Set set class
class TComRPSList
{
private:

    Int  m_numberOfReferencePictureSets;
    TComReferencePictureSet* m_referencePictureSets;

public:

    TComRPSList();
    virtual ~TComRPSList();

    void  create(Int numberOfEntries);
    void  destroy();

    TComReferencePictureSet* getReferencePictureSet(Int referencePictureSetNum);
    Int getNumberOfReferencePictureSets() const;
    void setNumberOfReferencePictureSets(Int numberOfReferencePictureSets);
};

/// SCALING_LIST class
class TComScalingList
{
public:

    TComScalingList();
    virtual ~TComScalingList();
    void     setScalingListPresentFlag(Bool b)                  { m_scalingListPresentFlag = b; }

    Bool     getScalingListPresentFlag()                        { return m_scalingListPresentFlag; }

    Bool     getUseTransformSkip()                              { return m_useTransformSkip; }

    void     setUseTransformSkip(Bool b)                        { m_useTransformSkip = b; }

    Int*     getScalingListAddress(UInt sizeId, UInt listId)    { return m_scalingListCoef[sizeId][listId]; }          //!< get matrix coefficient

    Bool     checkPredMode(UInt sizeId, UInt listId);
    void     setRefMatrixId(UInt sizeId, UInt listId, UInt u)   { m_refMatrixId[sizeId][listId] = u;    }                    //!< set reference matrix ID

    UInt     getRefMatrixId(UInt sizeId, UInt listId)           { return m_refMatrixId[sizeId][listId]; }                    //!< get reference matrix ID

    Int*     getScalingListDefaultAddress(UInt sizeId, UInt listId);                                                         //!< get default matrix coefficient
    void     processDefaultMarix(UInt sizeId, UInt listId);
    void     setScalingListDC(UInt sizeId, UInt listId, UInt u) { m_scalingListDC[sizeId][listId] = u; }                   //!< set DC value

    Int      getScalingListDC(UInt sizeId, UInt listId)         { return m_scalingListDC[sizeId][listId]; }                //!< get DC value

    void     checkDcOfMatrix();
    void     processRefMatrix(UInt sizeId, UInt listId, UInt refListId);
    Bool     xParseScalingList(Char* pchFile);

private:

    void     init();
    void     destroy();
    Int      m_scalingListDC[SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM];              //!< the DC value of the matrix coefficient for 16x16
    Bool     m_useDefaultScalingMatrixFlag[SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM]; //!< UseDefaultScalingMatrixFlag
    UInt     m_refMatrixId[SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM];                //!< RefMatrixID
    Bool     m_scalingListPresentFlag;                                              //!< flag for using default matrix
    UInt     m_predMatrixId[SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM];               //!< reference list index
    Int      *m_scalingListCoef[SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM];           //!< quantization matrix
    Bool     m_useTransformSkip;                                                    //!< transform skipping flag for setting default scaling matrix for 4x4
};

class ProfileTierLevel
{
    Int     m_profileSpace;
    Bool    m_tierFlag;
    Int     m_profileIdc;
    Bool    m_profileCompatibilityFlag[32];
    Int     m_levelIdc;
    Bool    m_progressiveSourceFlag;
    Bool    m_interlacedSourceFlag;
    Bool    m_nonPackedConstraintFlag;
    Bool    m_frameOnlyConstraintFlag;

public:

    ProfileTierLevel();

    Int getProfileSpace() const { return m_profileSpace; }

    void setProfileSpace(Int x) { m_profileSpace = x; }

    Bool getTierFlag() const    { return m_tierFlag; }

    void setTierFlag(Bool x)    { m_tierFlag = x; }

    Int getProfileIdc() const   { return m_profileIdc; }

    void setProfileIdc(Int x)   { m_profileIdc = x; }

    Bool getProfileCompatibilityFlag(Int i) const   { return m_profileCompatibilityFlag[i]; }

    void setProfileCompatibilityFlag(Int i, Bool x) { m_profileCompatibilityFlag[i] = x; }

    Int getLevelIdc() const                 { return m_levelIdc; }

    void setLevelIdc(Int x)                 { m_levelIdc = x; }

    Bool getProgressiveSourceFlag() const   { return m_progressiveSourceFlag; }

    void setProgressiveSourceFlag(Bool b)   { m_progressiveSourceFlag = b; }

    Bool getInterlacedSourceFlag() const    { return m_interlacedSourceFlag; }

    void setInterlacedSourceFlag(Bool b)    { m_interlacedSourceFlag = b; }

    Bool getNonPackedConstraintFlag() const { return m_nonPackedConstraintFlag; }

    void setNonPackedConstraintFlag(Bool b) { m_nonPackedConstraintFlag = b; }

    Bool getFrameOnlyConstraintFlag() const { return m_frameOnlyConstraintFlag; }

    void setFrameOnlyConstraintFlag(Bool b) { m_frameOnlyConstraintFlag = b; }
};

class TComPTL
{
    ProfileTierLevel m_generalPTL;
    ProfileTierLevel m_subLayerPTL[6];    // max. value of max_sub_layers_minus1 is 6
    Bool m_subLayerProfilePresentFlag[6];
    Bool m_subLayerLevelPresentFlag[6];

public:

    TComPTL();
    Bool getSubLayerProfilePresentFlag(Int i) const { return m_subLayerProfilePresentFlag[i]; }

    void setSubLayerProfilePresentFlag(Int i, Bool x) { m_subLayerProfilePresentFlag[i] = x; }

    Bool getSubLayerLevelPresentFlag(Int i) const { return m_subLayerLevelPresentFlag[i]; }

    void setSubLayerLevelPresentFlag(Int i, Bool x) { m_subLayerLevelPresentFlag[i] = x; }

    ProfileTierLevel* getGeneralPTL() { return &m_generalPTL; }

    ProfileTierLevel* getSubLayerPTL(Int i) { return &m_subLayerPTL[i]; }
};

/// VPS class

struct HrdSubLayerInfo
{
    Bool fixedPicRateFlag;
    Bool fixedPicRateWithinCvsFlag;
    UInt picDurationInTcMinus1;
    Bool lowDelayHrdFlag;
    UInt cpbCntMinus1;
    UInt bitRateValueMinus1[MAX_CPB_CNT][2];
    UInt cpbSizeValue[MAX_CPB_CNT][2];
    UInt ducpbSizeValue[MAX_CPB_CNT][2];
    Bool cbrFlag[MAX_CPB_CNT][2];
    UInt duBitRateValue[MAX_CPB_CNT][2];
};

class TComHRD
{
private:

    Bool m_nalHrdParametersPresentFlag;
    Bool m_vclHrdParametersPresentFlag;
    Bool m_subPicCpbParamsPresentFlag;
    UInt m_tickDivisorMinus2;
    UInt m_duCpbRemovalDelayLengthMinus1;
    Bool m_subPicCpbParamsInPicTimingSEIFlag;
    UInt m_dpbOutputDelayDuLengthMinus1;
    UInt m_bitRateScale;
    UInt m_cpbSizeScale;
    UInt m_ducpbSizeScale;
    UInt m_initialCpbRemovalDelayLengthMinus1;
    UInt m_cpbRemovalDelayLengthMinus1;
    UInt m_dpbOutputDelayLengthMinus1;
    UInt m_numDU;
    HrdSubLayerInfo m_HRD[MAX_TLAYER];

public:

    TComHRD()
        : m_nalHrdParametersPresentFlag(0)
        , m_vclHrdParametersPresentFlag(0)
        , m_subPicCpbParamsPresentFlag(false)
        , m_tickDivisorMinus2(0)
        , m_duCpbRemovalDelayLengthMinus1(0)
        , m_subPicCpbParamsInPicTimingSEIFlag(false)
        , m_dpbOutputDelayDuLengthMinus1(0)
        , m_bitRateScale(0)
        , m_cpbSizeScale(0)
        , m_initialCpbRemovalDelayLengthMinus1(0)
        , m_cpbRemovalDelayLengthMinus1(0)
        , m_dpbOutputDelayLengthMinus1(0)
    {}

    virtual ~TComHRD() {}

    void setNalHrdParametersPresentFlag(Bool flag) { m_nalHrdParametersPresentFlag = flag; }

    Bool getNalHrdParametersPresentFlag() { return m_nalHrdParametersPresentFlag; }

    void setVclHrdParametersPresentFlag(Bool flag) { m_vclHrdParametersPresentFlag = flag; }

    Bool getVclHrdParametersPresentFlag() { return m_vclHrdParametersPresentFlag; }

    void setSubPicCpbParamsPresentFlag(Bool flag) { m_subPicCpbParamsPresentFlag = flag; }

    Bool getSubPicCpbParamsPresentFlag() { return m_subPicCpbParamsPresentFlag; }

    void setTickDivisorMinus2(UInt value) { m_tickDivisorMinus2 = value; }

    UInt getTickDivisorMinus2() { return m_tickDivisorMinus2; }

    void setDuCpbRemovalDelayLengthMinus1(UInt value) { m_duCpbRemovalDelayLengthMinus1 = value; }

    UInt getDuCpbRemovalDelayLengthMinus1() { return m_duCpbRemovalDelayLengthMinus1; }

    void setSubPicCpbParamsInPicTimingSEIFlag(Bool flag) { m_subPicCpbParamsInPicTimingSEIFlag = flag; }

    Bool getSubPicCpbParamsInPicTimingSEIFlag() { return m_subPicCpbParamsInPicTimingSEIFlag; }

    void setDpbOutputDelayDuLengthMinus1(UInt value) { m_dpbOutputDelayDuLengthMinus1 = value; }

    UInt getDpbOutputDelayDuLengthMinus1() { return m_dpbOutputDelayDuLengthMinus1; }

    void setBitRateScale(UInt value) { m_bitRateScale = value; }

    UInt getBitRateScale() { return m_bitRateScale; }

    void setCpbSizeScale(UInt value) { m_cpbSizeScale = value; }

    UInt getCpbSizeScale() { return m_cpbSizeScale; }

    void setDuCpbSizeScale(UInt value) { m_ducpbSizeScale = value; }

    UInt getDuCpbSizeScale() { return m_ducpbSizeScale; }

    void setInitialCpbRemovalDelayLengthMinus1(UInt value) { m_initialCpbRemovalDelayLengthMinus1 = value; }

    UInt getInitialCpbRemovalDelayLengthMinus1() { return m_initialCpbRemovalDelayLengthMinus1; }

    void setCpbRemovalDelayLengthMinus1(UInt value) { m_cpbRemovalDelayLengthMinus1 = value; }

    UInt getCpbRemovalDelayLengthMinus1() { return m_cpbRemovalDelayLengthMinus1; }

    void setDpbOutputDelayLengthMinus1(UInt value) { m_dpbOutputDelayLengthMinus1 = value; }

    UInt getDpbOutputDelayLengthMinus1() { return m_dpbOutputDelayLengthMinus1; }

    void setFixedPicRateFlag(Int layer, Bool flag) { m_HRD[layer].fixedPicRateFlag = flag; }

    Bool getFixedPicRateFlag(Int layer) { return m_HRD[layer].fixedPicRateFlag; }

    void setFixedPicRateWithinCvsFlag(Int layer, Bool flag) { m_HRD[layer].fixedPicRateWithinCvsFlag = flag; }

    Bool getFixedPicRateWithinCvsFlag(Int layer) { return m_HRD[layer].fixedPicRateWithinCvsFlag; }

    void setPicDurationInTcMinus1(Int layer, UInt value) { m_HRD[layer].picDurationInTcMinus1 = value; }

    UInt getPicDurationInTcMinus1(Int layer) { return m_HRD[layer].picDurationInTcMinus1; }

    void setLowDelayHrdFlag(Int layer, Bool flag) { m_HRD[layer].lowDelayHrdFlag = flag; }

    Bool getLowDelayHrdFlag(Int layer) { return m_HRD[layer].lowDelayHrdFlag; }

    void setCpbCntMinus1(Int layer, UInt value) { m_HRD[layer].cpbCntMinus1 = value; }

    UInt getCpbCntMinus1(Int layer) { return m_HRD[layer].cpbCntMinus1; }

    void setBitRateValueMinus1(Int layer, Int cpbcnt, Int nalOrVcl, UInt value) { m_HRD[layer].bitRateValueMinus1[cpbcnt][nalOrVcl] = value; }

    UInt getBitRateValueMinus1(Int layer, Int cpbcnt, Int nalOrVcl) { return m_HRD[layer].bitRateValueMinus1[cpbcnt][nalOrVcl]; }

    void setCpbSizeValueMinus1(Int layer, Int cpbcnt, Int nalOrVcl, UInt value) { m_HRD[layer].cpbSizeValue[cpbcnt][nalOrVcl] = value; }

    UInt getCpbSizeValueMinus1(Int layer, Int cpbcnt, Int nalOrVcl)  { return m_HRD[layer].cpbSizeValue[cpbcnt][nalOrVcl]; }

    void setDuCpbSizeValueMinus1(Int layer, Int cpbcnt, Int nalOrVcl, UInt value) { m_HRD[layer].ducpbSizeValue[cpbcnt][nalOrVcl] = value; }

    UInt getDuCpbSizeValueMinus1(Int layer, Int cpbcnt, Int nalOrVcl)  { return m_HRD[layer].ducpbSizeValue[cpbcnt][nalOrVcl]; }

    void setDuBitRateValueMinus1(Int layer, Int cpbcnt, Int nalOrVcl, UInt value) { m_HRD[layer].duBitRateValue[cpbcnt][nalOrVcl] = value; }

    UInt getDuBitRateValueMinus1(Int layer, Int cpbcnt, Int nalOrVcl) { return m_HRD[layer].duBitRateValue[cpbcnt][nalOrVcl]; }

    void setCbrFlag(Int layer, Int cpbcnt, Int nalOrVcl, Bool value) { m_HRD[layer].cbrFlag[cpbcnt][nalOrVcl] = value; }

    Bool getCbrFlag(Int layer, Int cpbcnt, Int nalOrVcl) { return m_HRD[layer].cbrFlag[cpbcnt][nalOrVcl]; }

    void setNumDU(UInt value) { m_numDU = value; }

    UInt getNumDU()           { return m_numDU; }

    Bool getCpbDpbDelaysPresentFlag() { return getNalHrdParametersPresentFlag() || getVclHrdParametersPresentFlag(); }
};

class TimingInfo
{
    Bool m_timingInfoPresentFlag;
    UInt m_numUnitsInTick;
    UInt m_timeScale;
    Bool m_pocProportionalToTimingFlag;
    Int  m_numTicksPocDiffOneMinus1;

public:

    TimingInfo()
        : m_timingInfoPresentFlag(false)
        , m_numUnitsInTick(1001)
        , m_timeScale(60000)
        , m_pocProportionalToTimingFlag(false)
        , m_numTicksPocDiffOneMinus1(0) {}

    void setTimingInfoPresentFlag(Bool flag)    { m_timingInfoPresentFlag = flag; }

    Bool getTimingInfoPresentFlag()             { return m_timingInfoPresentFlag; }

    void setNumUnitsInTick(UInt value)          { m_numUnitsInTick = value; }

    UInt getNumUnitsInTick()                    { return m_numUnitsInTick; }

    void setTimeScale(UInt value)               { m_timeScale = value; }

    UInt getTimeScale()                         { return m_timeScale; }

    Bool getPocProportionalToTimingFlag()       { return m_pocProportionalToTimingFlag; }

    void setPocProportionalToTimingFlag(Bool x) { m_pocProportionalToTimingFlag = x; }

    Int  getNumTicksPocDiffOneMinus1()          { return m_numTicksPocDiffOneMinus1; }

    void setNumTicksPocDiffOneMinus1(Int x)     { m_numTicksPocDiffOneMinus1 = x; }
};

class TComVPS
{
private:

    Int         m_VPSId;
    UInt        m_uiMaxTLayers;
    UInt        m_maxLayers;
    Bool        m_bTemporalIdNestingFlag;

    UInt        m_numReorderPics[MAX_TLAYER];
    UInt        m_maxDecPicBuffering[MAX_TLAYER];
    UInt        m_maxLatencyIncrease[MAX_TLAYER];  // Really max latency increase plus 1 (value 0 expresses no limit)

    UInt        m_numHrdParameters;
    UInt        m_maxNuhReservedZeroLayerId;
    TComHRD*    m_hrdParameters;
    UInt*       m_hrdOpSetIdx;
    Bool*       m_cprmsPresentFlag;
    UInt        m_numOpSets;
    Bool        m_layerIdIncludedFlag[MAX_VPS_OP_SETS_PLUS1][MAX_VPS_NUH_RESERVED_ZERO_LAYER_ID_PLUS1];

    TComPTL     m_ptl;
    TimingInfo  m_timingInfo;

public:

    TComVPS();
    virtual ~TComVPS();

    void    createHrdParamBuffer()
    {
        m_hrdParameters    = new TComHRD[getNumHrdParameters()];
        m_hrdOpSetIdx      = new UInt[getNumHrdParameters()];
        m_cprmsPresentFlag = new Bool[getNumHrdParameters()];
    }

    TComHRD* getHrdParameters(UInt i)             { return &m_hrdParameters[i]; }

    UInt    getHrdOpSetIdx(UInt i)                { return m_hrdOpSetIdx[i]; }

    void    setHrdOpSetIdx(UInt val, UInt i)      { m_hrdOpSetIdx[i] = val; }

    Bool    getCprmsPresentFlag(UInt i)           { return m_cprmsPresentFlag[i]; }

    void    setCprmsPresentFlag(Bool val, UInt i) { m_cprmsPresentFlag[i] = val; }

    Int     getVPSId()                            { return m_VPSId; }

    void    setVPSId(Int i)                       { m_VPSId = i; }

    UInt    getMaxTLayers()                       { return m_uiMaxTLayers; }

    void    setMaxTLayers(UInt t)                 { m_uiMaxTLayers = t; }

    UInt    getMaxLayers()                        { return m_maxLayers; }

    void    setMaxLayers(UInt l)                  { m_maxLayers = l; }

    Bool    getTemporalNestingFlag()              { return m_bTemporalIdNestingFlag; }

    void    setTemporalNestingFlag(Bool t)        { m_bTemporalIdNestingFlag = t; }

    void    setNumReorderPics(UInt v, UInt tLayer)     { m_numReorderPics[tLayer] = v; }

    UInt    getNumReorderPics(UInt tLayer)             { return m_numReorderPics[tLayer]; }

    void    setMaxDecPicBuffering(UInt v, UInt tLayer) { m_maxDecPicBuffering[tLayer] = v; }

    UInt    getMaxDecPicBuffering(UInt tLayer)         { return m_maxDecPicBuffering[tLayer]; }

    void    setMaxLatencyIncrease(UInt v, UInt tLayer) { m_maxLatencyIncrease[tLayer] = v; }

    UInt    getMaxLatencyIncrease(UInt tLayer)         { return m_maxLatencyIncrease[tLayer]; }

    UInt    getNumHrdParameters()                      { return m_numHrdParameters; }

    void    setNumHrdParameters(UInt v)                { m_numHrdParameters = v; }

    UInt    getMaxNuhReservedZeroLayerId()             { return m_maxNuhReservedZeroLayerId; }

    void    setMaxNuhReservedZeroLayerId(UInt v)       { m_maxNuhReservedZeroLayerId = v; }

    UInt    getMaxOpSets()                             { return m_numOpSets; }

    void    setMaxOpSets(UInt v)                       { m_numOpSets = v; }

    Bool    getLayerIdIncludedFlag(UInt opsIdx, UInt id)        { return m_layerIdIncludedFlag[opsIdx][id]; }

    void    setLayerIdIncludedFlag(Bool v, UInt opsIdx, UInt id) { m_layerIdIncludedFlag[opsIdx][id] = v; }

    TComPTL* getPTL()           { return &m_ptl; }

    TimingInfo* getTimingInfo() { return &m_timingInfo; }
};

class Window
{
private:

    Bool          m_enabledFlag;
    Int           m_winLeftOffset;
    Int           m_winRightOffset;
    Int           m_winTopOffset;
    Int           m_winBottomOffset;

public:

    Window()
        : m_enabledFlag(false)
        , m_winLeftOffset(0)
        , m_winRightOffset(0)
        , m_winTopOffset(0)
        , m_winBottomOffset(0)
    {}

    Bool          getWindowEnabledFlag() const      { return m_enabledFlag; }

    void          resetWindow()                     { m_enabledFlag = false; m_winLeftOffset = m_winRightOffset = m_winTopOffset = m_winBottomOffset = 0; }

    Int           getWindowLeftOffset() const       { return m_enabledFlag ? m_winLeftOffset : 0; }

    void          setWindowLeftOffset(Int val)      { m_winLeftOffset = val; m_enabledFlag = true; }

    Int           getWindowRightOffset() const      { return m_enabledFlag ? m_winRightOffset : 0; }

    void          setWindowRightOffset(Int val)     { m_winRightOffset = val; m_enabledFlag = true; }

    Int           getWindowTopOffset() const        { return m_enabledFlag ? m_winTopOffset : 0; }

    void          setWindowTopOffset(Int val)       { m_winTopOffset = val; m_enabledFlag = true; }

    Int           getWindowBottomOffset() const     { return m_enabledFlag ? m_winBottomOffset : 0; }

    void          setWindowBottomOffset(Int val)    { m_winBottomOffset = val; m_enabledFlag = true; }

    void          setWindow(Int offsetLeft, Int offsetLRight, Int offsetLTop, Int offsetLBottom)
    {
        m_enabledFlag       = true;
        m_winLeftOffset     = offsetLeft;
        m_winRightOffset    = offsetLRight;
        m_winTopOffset      = offsetLTop;
        m_winBottomOffset   = offsetLBottom;
    }
};

class TComVUI
{
private:

    Bool m_aspectRatioInfoPresentFlag;
    Int  m_aspectRatioIdc;
    Int  m_sarWidth;
    Int  m_sarHeight;
    Bool m_overscanInfoPresentFlag;
    Bool m_overscanAppropriateFlag;
    Bool m_videoSignalTypePresentFlag;
    Int  m_videoFormat;
    Bool m_videoFullRangeFlag;
    Bool m_colourDescriptionPresentFlag;
    Int  m_colourPrimaries;
    Int  m_transferCharacteristics;
    Int  m_matrixCoefficients;
    Bool m_chromaLocInfoPresentFlag;
    Int  m_chromaSampleLocTypeTopField;
    Int  m_chromaSampleLocTypeBottomField;
    Bool m_neutralChromaIndicationFlag;
    Bool m_fieldSeqFlag;

    Window m_defaultDisplayWindow;
    Bool m_frameFieldInfoPresentFlag;
    Bool m_hrdParametersPresentFlag;
    Bool m_bitstreamRestrictionFlag;
    Bool m_motionVectorsOverPicBoundariesFlag;
    Bool m_restrictedRefPicListsFlag;
    Int  m_minSpatialSegmentationIdc;
    Int  m_maxBytesPerPicDenom;
    Int  m_maxBitsPerMinCuDenom;
    Int  m_log2MaxMvLengthHorizontal;
    Int  m_log2MaxMvLengthVertical;
    TComHRD m_hrdParameters;
    TimingInfo m_timingInfo;

public:

    TComVUI()
        : m_aspectRatioInfoPresentFlag(false)
        , m_aspectRatioIdc(0)
        , m_sarWidth(0)
        , m_sarHeight(0)
        , m_overscanInfoPresentFlag(false)
        , m_overscanAppropriateFlag(false)
        , m_videoSignalTypePresentFlag(false)
        , m_videoFormat(5)
        , m_videoFullRangeFlag(false)
        , m_colourDescriptionPresentFlag(false)
        , m_colourPrimaries(2)
        , m_transferCharacteristics(2)
        , m_matrixCoefficients(2)
        , m_chromaLocInfoPresentFlag(false)
        , m_chromaSampleLocTypeTopField(0)
        , m_chromaSampleLocTypeBottomField(0)
        , m_neutralChromaIndicationFlag(false)
        , m_fieldSeqFlag(false)
        , m_frameFieldInfoPresentFlag(false)
        , m_hrdParametersPresentFlag(false)
        , m_bitstreamRestrictionFlag(false)
        , m_motionVectorsOverPicBoundariesFlag(true)
        , m_restrictedRefPicListsFlag(1)
        , m_minSpatialSegmentationIdc(0)
        , m_maxBytesPerPicDenom(2)
        , m_maxBitsPerMinCuDenom(1)
        , m_log2MaxMvLengthHorizontal(15)
        , m_log2MaxMvLengthVertical(15)
    {}

    virtual ~TComVUI() {}

    Bool getAspectRatioInfoPresentFlag() { return m_aspectRatioInfoPresentFlag; }

    void setAspectRatioInfoPresentFlag(Bool i) { m_aspectRatioInfoPresentFlag = i; }

    Int getAspectRatioIdc() { return m_aspectRatioIdc; }

    void setAspectRatioIdc(Int i) { m_aspectRatioIdc = i; }

    Int getSarWidth() { return m_sarWidth; }

    void setSarWidth(Int i) { m_sarWidth = i; }

    Int getSarHeight() { return m_sarHeight; }

    void setSarHeight(Int i) { m_sarHeight = i; }

    Bool getOverscanInfoPresentFlag() { return m_overscanInfoPresentFlag; }

    void setOverscanInfoPresentFlag(Bool i) { m_overscanInfoPresentFlag = i; }

    Bool getOverscanAppropriateFlag() { return m_overscanAppropriateFlag; }

    void setOverscanAppropriateFlag(Bool i) { m_overscanAppropriateFlag = i; }

    Bool getVideoSignalTypePresentFlag() { return m_videoSignalTypePresentFlag; }

    void setVideoSignalTypePresentFlag(Bool i) { m_videoSignalTypePresentFlag = i; }

    Int getVideoFormat() { return m_videoFormat; }

    void setVideoFormat(Int i) { m_videoFormat = i; }

    Bool getVideoFullRangeFlag() { return m_videoFullRangeFlag; }

    void setVideoFullRangeFlag(Bool i) { m_videoFullRangeFlag = i; }

    Bool getColourDescriptionPresentFlag() { return m_colourDescriptionPresentFlag; }

    void setColourDescriptionPresentFlag(Bool i) { m_colourDescriptionPresentFlag = i; }

    Int getColourPrimaries() { return m_colourPrimaries; }

    void setColourPrimaries(Int i) { m_colourPrimaries = i; }

    Int getTransferCharacteristics() { return m_transferCharacteristics; }

    void setTransferCharacteristics(Int i) { m_transferCharacteristics = i; }

    Int getMatrixCoefficients() { return m_matrixCoefficients; }

    void setMatrixCoefficients(Int i) { m_matrixCoefficients = i; }

    Bool getChromaLocInfoPresentFlag() { return m_chromaLocInfoPresentFlag; }

    void setChromaLocInfoPresentFlag(Bool i) { m_chromaLocInfoPresentFlag = i; }

    Int getChromaSampleLocTypeTopField() { return m_chromaSampleLocTypeTopField; }

    void setChromaSampleLocTypeTopField(Int i) { m_chromaSampleLocTypeTopField = i; }

    Int getChromaSampleLocTypeBottomField() { return m_chromaSampleLocTypeBottomField; }

    void setChromaSampleLocTypeBottomField(Int i) { m_chromaSampleLocTypeBottomField = i; }

    Bool getNeutralChromaIndicationFlag() { return m_neutralChromaIndicationFlag; }

    void setNeutralChromaIndicationFlag(Bool i) { m_neutralChromaIndicationFlag = i; }

    Bool getFieldSeqFlag() { return m_fieldSeqFlag; }

    void setFieldSeqFlag(Bool i) { m_fieldSeqFlag = i; }

    Bool getFrameFieldInfoPresentFlag() { return m_frameFieldInfoPresentFlag; }

    void setFrameFieldInfoPresentFlag(Bool i) { m_frameFieldInfoPresentFlag = i; }

    Window& getDefaultDisplayWindow() { return m_defaultDisplayWindow; }

    void setDefaultDisplayWindow(Window& defaultDisplayWindow) { m_defaultDisplayWindow = defaultDisplayWindow; }

    Bool getHrdParametersPresentFlag() { return m_hrdParametersPresentFlag; }

    void setHrdParametersPresentFlag(Bool i) { m_hrdParametersPresentFlag = i; }

    Bool getBitstreamRestrictionFlag() { return m_bitstreamRestrictionFlag; }

    void setBitstreamRestrictionFlag(Bool i) { m_bitstreamRestrictionFlag = i; }

    Bool getMotionVectorsOverPicBoundariesFlag() { return m_motionVectorsOverPicBoundariesFlag; }

    void setMotionVectorsOverPicBoundariesFlag(Bool i) { m_motionVectorsOverPicBoundariesFlag = i; }

    Bool getRestrictedRefPicListsFlag() { return m_restrictedRefPicListsFlag; }

    void setRestrictedRefPicListsFlag(Bool b) { m_restrictedRefPicListsFlag = b; }

    Int getMinSpatialSegmentationIdc() { return m_minSpatialSegmentationIdc; }

    void setMinSpatialSegmentationIdc(Int i) { m_minSpatialSegmentationIdc = i; }

    Int getMaxBytesPerPicDenom() { return m_maxBytesPerPicDenom; }

    void setMaxBytesPerPicDenom(Int i) { m_maxBytesPerPicDenom = i; }

    Int getMaxBitsPerMinCuDenom() { return m_maxBitsPerMinCuDenom; }

    void setMaxBitsPerMinCuDenom(Int i) { m_maxBitsPerMinCuDenom = i; }

    Int getLog2MaxMvLengthHorizontal() { return m_log2MaxMvLengthHorizontal; }

    void setLog2MaxMvLengthHorizontal(Int i) { m_log2MaxMvLengthHorizontal = i; }

    Int getLog2MaxMvLengthVertical() { return m_log2MaxMvLengthVertical; }

    void setLog2MaxMvLengthVertical(Int i) { m_log2MaxMvLengthVertical = i; }

    TComHRD* getHrdParameters() { return &m_hrdParameters; }

    TimingInfo* getTimingInfo() { return &m_timingInfo; }
};

/// SPS class
class TComSPS
{
private:

    Int         m_SPSId;
    Int         m_VPSId;
    Int         m_chromaFormatIdc;

    UInt        m_maxTLayers;         // maximum number of temporal layers

    // Structure
    UInt        m_picWidthInLumaSamples;
    UInt        m_picHeightInLumaSamples;

    Int         m_log2MinCodingBlockSize;
    Int         m_log2DiffMaxMinCodingBlockSize;
    UInt        m_maxCUWidth;
    UInt        m_maxCUHeight;
    UInt        m_maxCUDepth;

    Window      m_conformanceWindow;

    TComRPSList m_RPSList;
    Bool        m_bLongTermRefsPresent;
    Bool        m_TMVPFlagsPresent;
    Int         m_numReorderPics[MAX_TLAYER];

    // Tool list
    UInt        m_quadtreeTULog2MaxSize;
    UInt        m_quadtreeTULog2MinSize;
    UInt        m_quadtreeTUMaxDepthInter;
    UInt        m_quadtreeTUMaxDepthIntra;
    Bool        m_usePCM;
    UInt        m_pcmLog2MaxSize;
    UInt        m_pcmLog2MinSize;
    Bool        m_useAMP;

    // Parameter
    Int         m_bitDepthY;
    Int         m_bitDepthC;
    Int         m_qpBDOffsetY;
    Int         m_qpBDOffsetC;

    Bool        m_useLossless;

    UInt        m_pcmBitDepthLuma;
    UInt        m_pcmBitDepthChroma;
    Bool        m_bPCMFilterDisableFlag;

    UInt        m_bitsForPOC;
    UInt        m_numLongTermRefPicSPS;
    UInt        m_ltRefPicPocLsbSps[33];
    Bool        m_usedByCurrPicLtSPSFlag[33];

    // Max physical transform size
    UInt        m_maxTrSize;

    Int m_iAMPAcc[MAX_CU_DEPTH];
    Bool        m_bUseSAO;

    Bool        m_bTemporalIdNestingFlag; // temporal_id_nesting_flag

    Bool        m_scalingListEnabledFlag;
    Bool        m_scalingListPresentFlag;
    TComScalingList* m_scalingList; //!< ScalingList class pointer
    UInt        m_maxDecPicBuffering[MAX_TLAYER];
    UInt        m_maxLatencyIncrease[MAX_TLAYER]; // Really max latency increase plus 1 (value 0 expresses no limit)

    Bool        m_useDF;
    Bool        m_useStrongIntraSmoothing;

    Bool        m_vuiParametersPresentFlag;
    TComVUI     m_vuiParameters;
    TComPTL     m_ptl;

public:

    TComSPS();
    virtual ~TComSPS();

    Int  getVPSId()         { return m_VPSId; }

    void setVPSId(Int i)    { m_VPSId = i; }

    Int  getSPSId()         { return m_SPSId; }

    void setSPSId(Int i)    { m_SPSId = i; }

    Int  getChromaFormatIdc()         { return m_chromaFormatIdc; }

    void setChromaFormatIdc(Int i)    { m_chromaFormatIdc = i; }

    static Int getWinUnitX(Int chromaFormatIdc) { assert(chromaFormatIdc > 0 && chromaFormatIdc <= MAX_CHROMA_FORMAT_IDC); return g_winUnitX[chromaFormatIdc]; }

    static Int getWinUnitY(Int chromaFormatIdc) { assert(chromaFormatIdc > 0 && chromaFormatIdc <= MAX_CHROMA_FORMAT_IDC); return g_winUnitY[chromaFormatIdc]; }

    // structure
    void setPicWidthInLumaSamples(UInt u) { m_picWidthInLumaSamples = u; }

    UInt getPicWidthInLumaSamples() const { return m_picWidthInLumaSamples; }

    void setPicHeightInLumaSamples(UInt u) { m_picHeightInLumaSamples = u; }

    UInt getPicHeightInLumaSamples() const { return m_picHeightInLumaSamples; }

    Window& getConformanceWindow() { return m_conformanceWindow; }

    void    setConformanceWindow(Window& conformanceWindow) { m_conformanceWindow = conformanceWindow; }

    UInt  getNumLongTermRefPicSPS() const { return m_numLongTermRefPicSPS; }

    void  setNumLongTermRefPicSPS(UInt val) { m_numLongTermRefPicSPS = val; }

    UInt  getLtRefPicPocLsbSps(UInt index) const { return m_ltRefPicPocLsbSps[index]; }

    void  setLtRefPicPocLsbSps(UInt index, UInt val) { m_ltRefPicPocLsbSps[index] = val; }

    Bool getUsedByCurrPicLtSPSFlag(Int i) const { return m_usedByCurrPicLtSPSFlag[i]; }

    void setUsedByCurrPicLtSPSFlag(Int i, Bool x) { m_usedByCurrPicLtSPSFlag[i] = x; }

    Int  getLog2MinCodingBlockSize() const { return m_log2MinCodingBlockSize; }

    void setLog2MinCodingBlockSize(Int val) { m_log2MinCodingBlockSize = val; }

    Int  getLog2DiffMaxMinCodingBlockSize() const { return m_log2DiffMaxMinCodingBlockSize; }

    void setLog2DiffMaxMinCodingBlockSize(Int val) { m_log2DiffMaxMinCodingBlockSize = val; }

    void setMaxCUWidth(UInt u) { m_maxCUWidth = u; }

    UInt getMaxCUWidth() const  { return m_maxCUWidth; }

    void setMaxCUHeight(UInt u) { m_maxCUHeight = u; }

    UInt getMaxCUHeight() const { return m_maxCUHeight; }

    void setMaxCUDepth(UInt u) { m_maxCUDepth = u; }

    UInt getMaxCUDepth() const { return m_maxCUDepth; }

    void setUsePCM(Bool b)   { m_usePCM = b; }

    Bool getUsePCM() const   { return m_usePCM; }

    void setPCMLog2MaxSize(UInt u) { m_pcmLog2MaxSize = u; }

    UInt getPCMLog2MaxSize() const { return m_pcmLog2MaxSize; }

    void setPCMLog2MinSize(UInt u) { m_pcmLog2MinSize = u; }

    UInt getPCMLog2MinSize() const { return m_pcmLog2MinSize; }

    void setBitsForPOC(UInt u) { m_bitsForPOC = u; }

    UInt getBitsForPOC() const { return m_bitsForPOC; }

    Bool getUseAMP() { return m_useAMP; }

    void setUseAMP(Bool b) { m_useAMP = b; }

    void setQuadtreeTULog2MaxSize(UInt u)   { m_quadtreeTULog2MaxSize = u; }

    UInt getQuadtreeTULog2MaxSize() const   { return m_quadtreeTULog2MaxSize; }

    void setQuadtreeTULog2MinSize(UInt u)   { m_quadtreeTULog2MinSize = u; }

    UInt getQuadtreeTULog2MinSize() const   { return m_quadtreeTULog2MinSize; }

    void setQuadtreeTUMaxDepthInter(UInt u) { m_quadtreeTUMaxDepthInter = u; }

    void setQuadtreeTUMaxDepthIntra(UInt u) { m_quadtreeTUMaxDepthIntra = u; }

    UInt getQuadtreeTUMaxDepthInter() const   { return m_quadtreeTUMaxDepthInter; }

    UInt getQuadtreeTUMaxDepthIntra() const    { return m_quadtreeTUMaxDepthIntra; }

    void setNumReorderPics(Int i, UInt tlayer) { m_numReorderPics[tlayer] = i; }

    Int  getNumReorderPics(UInt tlayer) const  { return m_numReorderPics[tlayer]; }

    void createRPSList(Int numRPS);

    TComRPSList* getRPSList()                  { return &m_RPSList; }

    Bool      getLongTermRefsPresent() const   { return m_bLongTermRefsPresent; }

    void      setLongTermRefsPresent(Bool b)   { m_bLongTermRefsPresent = b; }

    Bool      getTMVPFlagsPresent() const   { return m_TMVPFlagsPresent; }

    void      setTMVPFlagsPresent(Bool b)   { m_TMVPFlagsPresent = b; }

    // physical transform
    void setMaxTrSize(UInt u)   { m_maxTrSize = u; }

    UInt getMaxTrSize() const   { return m_maxTrSize; }

    // Tool list
    Bool getUseLossless() const { return m_useLossless; }

    void setUseLossless(Bool b) { m_useLossless  = b; }

    // AMP accuracy
    Int       getAMPAcc(UInt depth) const { return m_iAMPAcc[depth]; }

    void      setAMPAcc(UInt depth, Int iAccu) { assert(depth < g_maxCUDepth);  m_iAMPAcc[depth] = iAccu; }

    // Bit-depth
    Int      getBitDepthY() const { return m_bitDepthY; }

    void     setBitDepthY(Int u) { m_bitDepthY = u; }

    Int      getBitDepthC() const { return m_bitDepthC; }

    void     setBitDepthC(Int u) { m_bitDepthC = u; }

    Int       getQpBDOffsetY() const { return m_qpBDOffsetY; }

    void      setQpBDOffsetY(Int value) { m_qpBDOffsetY = value; }

    Int       getQpBDOffsetC() const { return m_qpBDOffsetC; }

    void      setQpBDOffsetC(Int value) { m_qpBDOffsetC = value; }

    void setUseSAO(Bool bVal)  { m_bUseSAO = bVal; }

    Bool getUseSAO() const { return m_bUseSAO; }

    UInt      getMaxTLayers() const                   { return m_maxTLayers; }

    void      setMaxTLayers(UInt maxTLayers)          { assert(maxTLayers <= MAX_TLAYER); m_maxTLayers = maxTLayers; }

    Bool      getTemporalIdNestingFlag() const        { return m_bTemporalIdNestingFlag; }

    void      setTemporalIdNestingFlag(Bool bValue)   { m_bTemporalIdNestingFlag = bValue; }

    UInt      getPCMBitDepthLuma() const { return m_pcmBitDepthLuma; }

    void      setPCMBitDepthLuma(UInt u) { m_pcmBitDepthLuma = u; }

    UInt      getPCMBitDepthChroma() const { return m_pcmBitDepthChroma; }

    void      setPCMBitDepthChroma(UInt u) { m_pcmBitDepthChroma = u; }

    void      setPCMFilterDisableFlag(Bool bValue)    { m_bPCMFilterDisableFlag = bValue; }

    Bool      getPCMFilterDisableFlag() const         { return m_bPCMFilterDisableFlag; }

    Bool getScalingListFlag() const { return m_scalingListEnabledFlag; }

    void setScalingListFlag(Bool b) { m_scalingListEnabledFlag = b; }

    Bool getScalingListPresentFlag() const { return m_scalingListPresentFlag; }

    void setScalingListPresentFlag(Bool b) { m_scalingListPresentFlag = b; }

    void setScalingList(TComScalingList *scalingList);
    TComScalingList* getScalingList() { return m_scalingList; } //!< get ScalingList class pointer in SPS

    UInt getMaxDecPicBuffering(UInt tlayer) { return m_maxDecPicBuffering[tlayer]; }

    void setMaxDecPicBuffering(UInt ui, UInt tlayer) { m_maxDecPicBuffering[tlayer] = ui; }

    UInt getMaxLatencyIncrease(UInt tlayer) { return m_maxLatencyIncrease[tlayer]; }

    void setMaxLatencyIncrease(UInt ui, UInt tlayer) { m_maxLatencyIncrease[tlayer] = ui; }

    void setUseStrongIntraSmoothing(Bool bVal) { m_useStrongIntraSmoothing = bVal; }

    Bool getUseStrongIntraSmoothing() const { return m_useStrongIntraSmoothing; }

    Bool getVuiParametersPresentFlag() { return m_vuiParametersPresentFlag; }

    void setVuiParametersPresentFlag(Bool b) { m_vuiParametersPresentFlag = b; }

    TComVUI* getVuiParameters() { return &m_vuiParameters; }

    void setHrdParameters(UInt frameRate, UInt numDU, UInt bitRate, Bool randomAccess);

    TComPTL* getPTL() { return &m_ptl; }
};

/// Reference Picture Lists class
class TComRefPicListModification
{
private:

    Bool m_bRefPicListModificationFlagL0;
    Bool m_bRefPicListModificationFlagL1;
    UInt m_RefPicSetIdxL0[32];
    UInt m_RefPicSetIdxL1[32];

public:

    TComRefPicListModification();
    virtual ~TComRefPicListModification();

    void create();
    void destroy();

    Bool getRefPicListModificationFlagL0() { return m_bRefPicListModificationFlagL0; }

    void setRefPicListModificationFlagL0(Bool flag) { m_bRefPicListModificationFlagL0 = flag; }

    Bool getRefPicListModificationFlagL1() { return m_bRefPicListModificationFlagL1; }

    void setRefPicListModificationFlagL1(Bool flag) { m_bRefPicListModificationFlagL1 = flag; }

    void setRefPicSetIdxL0(UInt idx, UInt refPicSetIdx) { m_RefPicSetIdxL0[idx] = refPicSetIdx; }

    UInt getRefPicSetIdxL0(UInt idx) { return m_RefPicSetIdxL0[idx]; }

    void setRefPicSetIdxL1(UInt idx, UInt refPicSetIdx) { m_RefPicSetIdxL1[idx] = refPicSetIdx; }

    UInt getRefPicSetIdxL1(UInt idx) { return m_RefPicSetIdxL1[idx]; }
};

/// PPS class
class TComPPS
{
private:

    Int         m_PPSId;                  // pic_parameter_set_id
    Int         m_SPSId;                  // seq_parameter_set_id
    Int         m_picInitQPMinus26;
    Bool        m_useDQP;
    Bool        m_bConstrainedIntraPred;  // constrained_intra_pred_flag
    Bool        m_bSliceChromaQpFlag;     // slicelevel_chroma_qp_flag

    // access channel
    TComSPS*    m_sps;
    UInt        m_maxCuDQPDepth;
    UInt        m_minCuDQPSize;

    Int         m_chromaCbQpOffset;
    Int         m_chromaCrQpOffset;

    UInt        m_numRefIdxL0DefaultActive;
    UInt        m_numRefIdxL1DefaultActive;

    Bool        m_bUseWeightPred;         // Use of Weighting Prediction (P_SLICE)
    Bool        m_useWeightedBiPred;      // Use of Weighting Bi-Prediction (B_SLICE)
    Bool        m_outputFlagPresentFlag; // Indicates the presence of output_flag in slice header

    Bool        m_transquantBypassEnableFlag; // Indicates presence of cu_transquant_bypass_flag in CUs.
    Bool        m_useTransformSkip;
    Bool        m_entropyCodingSyncEnabledFlag; //!< Indicates the presence of wavefronts

    Bool     m_loopFilterAcrossTilesEnabledFlag;

    Int      m_signHideFlag;

    Bool     m_cabacInitPresentFlag;
    UInt     m_encCABACTableIdx;         // Used to transmit table selection across slices

    Bool     m_sliceHeaderExtensionPresentFlag;
    Bool     m_deblockingFilterControlPresentFlag;
    Bool     m_deblockingFilterOverrideEnabledFlag;
    Bool     m_picDisableDeblockingFilterFlag;
    Int      m_deblockingFilterBetaOffsetDiv2;  //< beta offset for deblocking filter
    Int      m_deblockingFilterTcOffsetDiv2;    //< tc offset for deblocking filter
    Bool     m_scalingListPresentFlag;
    TComScalingList* m_scalingList; //!< ScalingList class pointer
    Bool m_listsModificationPresentFlag;
    UInt m_log2ParallelMergeLevelMinus2;
    Int m_numExtraSliceHeaderBits;

public:

    TComPPS();
    virtual ~TComPPS();

    Int       getPPSId() const { return m_PPSId; }

    void      setPPSId(Int i) { m_PPSId = i; }

    Int       getSPSId() const{ return m_SPSId; }

    void      setSPSId(Int i) { m_SPSId = i; }

    Int       getPicInitQPMinus26() const { return m_picInitQPMinus26; }

    void      setPicInitQPMinus26(Int i)  { m_picInitQPMinus26 = i; }

    Bool      getUseDQP() const           { return m_useDQP; }

    void      setUseDQP(Bool b)           { m_useDQP = b; }

    Bool      getConstrainedIntraPred() const { return m_bConstrainedIntraPred; }

    void      setConstrainedIntraPred(Bool b) { m_bConstrainedIntraPred = b; }

    Bool      getSliceChromaQpFlag() const { return m_bSliceChromaQpFlag; }

    void      setSliceChromaQpFlag(Bool b) { m_bSliceChromaQpFlag = b; }

    void      setSPS(TComSPS* sps) { m_sps = sps; }

    TComSPS*  getSPS() { return m_sps; }

    void      setMaxCuDQPDepth(UInt u) { m_maxCuDQPDepth = u; }

    UInt      getMaxCuDQPDepth() const { return m_maxCuDQPDepth; }

    void      setMinCuDQPSize(UInt u) { m_minCuDQPSize = u; }

    UInt      getMinCuDQPSize() const { return m_minCuDQPSize; }

    void      setChromaCbQpOffset(Int i) { m_chromaCbQpOffset = i; }

    Int       getChromaCbQpOffset() const { return m_chromaCbQpOffset; }

    void      setChromaCrQpOffset(Int i) { m_chromaCrQpOffset = i; }

    Int       getChromaCrQpOffset() const { return m_chromaCrQpOffset; }

    void      setNumRefIdxL0DefaultActive(UInt i)    { m_numRefIdxL0DefaultActive = i; }

    UInt      getNumRefIdxL0DefaultActive() const     { return m_numRefIdxL0DefaultActive; }

    void      setNumRefIdxL1DefaultActive(UInt i)    { m_numRefIdxL1DefaultActive = i; }

    UInt      getNumRefIdxL1DefaultActive() const     { return m_numRefIdxL1DefaultActive; }

    Bool getUseWP() const    { return m_bUseWeightPred; }

    Bool getWPBiPred() const { return m_useWeightedBiPred; }

    void setUseWP(Bool b)    { m_bUseWeightPred = b; }

    void setWPBiPred(Bool b) { m_useWeightedBiPred = b; }

    void      setOutputFlagPresentFlag(Bool b)  { m_outputFlagPresentFlag = b; }

    Bool      getOutputFlagPresentFlag() const  { return m_outputFlagPresentFlag; }

    void      setTransquantBypassEnableFlag(Bool b) { m_transquantBypassEnableFlag = b; }

    Bool      getTransquantBypassEnableFlag() const { return m_transquantBypassEnableFlag; }

    Bool      getUseTransformSkip() const { return m_useTransformSkip; }

    void      setUseTransformSkip(Bool b) { m_useTransformSkip = b; }

    void    setLoopFilterAcrossTilesEnabledFlag(Bool b) { m_loopFilterAcrossTilesEnabledFlag = b; }

    Bool    getLoopFilterAcrossTilesEnabledFlag()      { return m_loopFilterAcrossTilesEnabledFlag; }

    Bool    getEntropyCodingSyncEnabledFlag() const    { return m_entropyCodingSyncEnabledFlag; }

    void    setEntropyCodingSyncEnabledFlag(Bool val)  { m_entropyCodingSyncEnabledFlag = val; }

    void    setSignHideFlag(Int signHideFlag)       { m_signHideFlag = signHideFlag; }

    Int     getSignHideFlag() const                 { return m_signHideFlag; }

    void     setCabacInitPresentFlag(Bool flag)     { m_cabacInitPresentFlag = flag; }

    void     setEncCABACTableIdx(Int idx)           { m_encCABACTableIdx = idx; }

    Bool     getCabacInitPresentFlag() const        { return m_cabacInitPresentFlag; }

    UInt     getEncCABACTableIdx() const            { return m_encCABACTableIdx; }

    void     setDeblockingFilterControlPresentFlag(Bool val)  { m_deblockingFilterControlPresentFlag = val; }

    Bool     getDeblockingFilterControlPresentFlag() const    { return m_deblockingFilterControlPresentFlag; }

    void     setDeblockingFilterOverrideEnabledFlag(Bool val) { m_deblockingFilterOverrideEnabledFlag = val; }

    Bool     getDeblockingFilterOverrideEnabledFlag() const   { return m_deblockingFilterOverrideEnabledFlag; }

    void     setPicDisableDeblockingFilterFlag(Bool val)      { m_picDisableDeblockingFilterFlag = val; }     //!< set offset for deblocking filter disabled

    Bool     getPicDisableDeblockingFilterFlag() const        { return m_picDisableDeblockingFilterFlag; }    //!< get offset for deblocking filter disabled

    void     setDeblockingFilterBetaOffsetDiv2(Int val)       { m_deblockingFilterBetaOffsetDiv2 = val; }     //!< set beta offset for deblocking filter

    Int      getDeblockingFilterBetaOffsetDiv2() const        { return m_deblockingFilterBetaOffsetDiv2; }    //!< get beta offset for deblocking filter

    void     setDeblockingFilterTcOffsetDiv2(Int val)         { m_deblockingFilterTcOffsetDiv2 = val; }       //!< set tc offset for deblocking filter

    Int      getDeblockingFilterTcOffsetDiv2()                { return m_deblockingFilterTcOffsetDiv2; }      //!< get tc offset for deblocking filter

    Bool     getScalingListPresentFlag() const { return m_scalingListPresentFlag; }

    void     setScalingListPresentFlag(Bool b) { m_scalingListPresentFlag = b; }

    void     setScalingList(TComScalingList *scalingList);

    const TComScalingList* getScalingList() const { return m_scalingList; } //!< get ScalingList class pointer in PPS

    Bool getListsModificationPresentFlag() const { return m_listsModificationPresentFlag; }

    void setListsModificationPresentFlag(Bool b) { m_listsModificationPresentFlag = b; }

    UInt getLog2ParallelMergeLevelMinus2() const { return m_log2ParallelMergeLevelMinus2; }

    void setLog2ParallelMergeLevelMinus2(UInt mrgLevel) { m_log2ParallelMergeLevelMinus2 = mrgLevel; }

    Int getNumExtraSliceHeaderBits() const { return m_numExtraSliceHeaderBits; }

    void setNumExtraSliceHeaderBits(Int i) { m_numExtraSliceHeaderBits = i; }

    Bool getSliceHeaderExtensionPresentFlag() const    { return m_sliceHeaderExtensionPresentFlag; }

    void setSliceHeaderExtensionPresentFlag(Bool val)  { m_sliceHeaderExtensionPresentFlag = val; }
};

struct WpScalingParam
{
    // Explicit weighted prediction parameters parsed in slice header,
    // or Implicit weighted prediction parameters (8 bits depth values).
    Bool        bPresentFlag;
    UInt        log2WeightDenom;
    Int         inputWeight;
    Int         inputOffset;

    // Weighted prediction scaling values built from above parameters (bitdepth scaled):
    Int         w, o, offset, shift, round;
};

typedef WpScalingParam wpScalingParam;

typedef struct
{
    Int64 ac;
    Int64 dc;
} wpACDCParam;

/// slice header class
class TComSlice
{
private:

    //  Bitstream writing
    Bool        m_saoEnabledFlag;
    Bool        m_saoEnabledFlagChroma; ///< SAO Cb&Cr enabled flag
    Int         m_ppsId;                ///< picture parameter set ID
    Bool        m_picOutputFlag;        ///< pic_output_flag
    Int         m_poc;
    Int         m_lastIDR;

    TComReferencePictureSet *m_rps;
    TComReferencePictureSet m_localRPS;
    Int         m_bdIdx;
    TComRefPicListModification m_refPicListModification;
    NalUnitType m_nalUnitType;       ///< Nal unit type for the slice
    SliceType   m_sliceType;
    Int         m_sliceQp;
    Bool        m_dependentSliceSegmentFlag;
    Int         m_sliceQpBase;
    Bool        m_deblockingFilterDisable;
    Bool        m_deblockingFilterOverrideFlag;    //< offsets for deblocking filter inherit from PPS
    Int         m_deblockingFilterBetaOffsetDiv2;  //< beta offset for deblocking filter
    Int         m_deblockingFilterTcOffsetDiv2;    //< tc offset for deblocking filter
    Int         m_list1IdxToList0Idx[MAX_NUM_REF];
    Int         m_numRefIdx[2];     //  for multiple reference of current slice

    Bool        m_bCheckLDC;

    //  Data
    Int         m_sliceQpDelta;
    Int         m_sliceQpDeltaCb;
    Int         m_sliceQpDeltaCr;
    TComPic*    m_refPicList[2][MAX_NUM_REF + 1];
    Int         m_refPOCList[2][MAX_NUM_REF + 1];
    Bool        m_bIsUsedAsLongTerm[2][MAX_NUM_REF + 1];
    Int         m_depth;

    // referenced slice?
    Bool        m_bReferenced;

    // access channel
    TComSPS*    m_sps;
    TComPPS*    m_pps;
    TComVPS*    m_vps;
    TComPic*    m_pic;
    UInt        m_colFromL0Flag; // collocated picture from List0 flag

    UInt        m_colRefIdx;
    UInt        m_maxNumMergeCand;

    Double      m_lumaLambda;
    Double      m_chromaLambda;

    Bool        m_bEqualRef[2][MAX_NUM_REF][MAX_NUM_REF];

    UInt        m_sliceCurEndCUAddr;
    Bool        m_nextSlice;
    UInt        m_sliceBits;
    UInt        m_sliceSegmentBits;
    Bool        m_bFinalized;

    wpACDCParam     m_weightACDCParam[3];                 // [0:Y, 1:U, 2:V]

    std::vector<UInt> m_tileByteLocation;
    UInt        m_tileOffstForMultES;

    UInt*       m_substreamSizes;
    TComScalingList* m_scalingList; //!< pointer of quantization matrix
    Bool        m_cabacInitFlag;

    Bool       m_bLMvdL1Zero;
    Int        m_numEntryPointOffsets;
    Bool       m_temporalLayerNonReferenceFlag;

    Bool       m_enableTMVPFlag;

public:

    x265::MotionReference * m_mref[2][MAX_NUM_REF + 1];
    wpScalingParam  m_weightPredTable[2][MAX_NUM_REF][3]; // [REF_PIC_LIST_0 or REF_PIC_LIST_1][refIdx][0:Y, 1:U, 2:V]

    TComSlice();
    virtual ~TComSlice();
    void      initSlice();

    void      setVPS(TComVPS* vps)            { m_vps = vps; }

    TComVPS*  getVPS()                        { return m_vps; }

    void      setSPS(TComSPS* sps)            { m_sps = sps; }

    TComSPS*  getSPS()                        { return m_sps; }

    void      setPPS(TComPPS* pps)            { assert(pps != NULL); m_pps = pps; m_ppsId = pps->getPPSId(); }

    TComPPS*  getPPS()                        { return m_pps; }

    void      setPPSId(Int ppsid)             { m_ppsId = ppsid; }

    Int       getPPSId()                      { return m_ppsId; }

    void      setPicOutputFlag(Bool b)        { m_picOutputFlag = b; }

    Bool      getPicOutputFlag()              { return m_picOutputFlag; }

    void      setSaoEnabledFlag(Bool s)       { m_saoEnabledFlag = s; }

    Bool      getSaoEnabledFlag()             { return m_saoEnabledFlag; }

    void      setSaoEnabledFlagChroma(Bool s) { m_saoEnabledFlagChroma = s; }   //!< set SAO Cb&Cr enabled flag

    Bool      getSaoEnabledFlagChroma()       { return m_saoEnabledFlagChroma; }      //!< get SAO Cb&Cr enabled flag

    void      setRPS(TComReferencePictureSet *rps) { m_rps = rps; }

    TComReferencePictureSet*  getRPS()            { return m_rps; }

    TComReferencePictureSet*  getLocalRPS()       { return &m_localRPS; }

    void      setRPSidx(Int bdidx)                { m_bdIdx = bdidx; }

    Int       getRPSidx()                         { return m_bdIdx; }

    TComRefPicListModification* getRefPicListModification() { return &m_refPicListModification; }

    void      setLastIDR(Int idrPoc)              { m_lastIDR = idrPoc; }

    Int       getLastIDR()                        { return m_lastIDR; }

    SliceType getSliceType()                      { return m_sliceType; }

    Int       getPOC()                            { return m_poc; }

    Int       getSliceQp()                        { return m_sliceQp; }

    Bool      getDependentSliceSegmentFlag() const   { return m_dependentSliceSegmentFlag; }

    void      setDependentSliceSegmentFlag(Bool val) { m_dependentSliceSegmentFlag = val; }

    Int       getSliceQpBase()                    { return m_sliceQpBase; }

    Int       getSliceQpDelta()                   { return m_sliceQpDelta; }

    Int       getSliceQpDeltaCb()                 { return m_sliceQpDeltaCb; }

    Int       getSliceQpDeltaCr()                 { return m_sliceQpDeltaCr; }

    Bool      getDeblockingFilterDisable()        { return m_deblockingFilterDisable; }

    Bool      getDeblockingFilterOverrideFlag()   { return m_deblockingFilterOverrideFlag; }

    Int       getDeblockingFilterBetaOffsetDiv2() { return m_deblockingFilterBetaOffsetDiv2; }

    Int       getDeblockingFilterTcOffsetDiv2()   { return m_deblockingFilterTcOffsetDiv2; }

    Int       getNumRefIdx(RefPicList e)          { return m_numRefIdx[e]; }

    TComPic*  getPic()                            { return m_pic; }

    TComPic*  getRefPic(RefPicList e, Int refIdx) { return m_refPicList[e][refIdx]; }

    Int       getRefPOC(RefPicList e, Int refIdx) { return m_refPOCList[e][refIdx]; }

    Int       getDepth()                          { return m_depth; }

    UInt      getColFromL0Flag()                  { return m_colFromL0Flag; }

    UInt      getColRefIdx()                      { return m_colRefIdx; }

    Bool      getIsUsedAsLongTerm(Int i, Int j)   { return m_bIsUsedAsLongTerm[i][j]; }

    Bool      getCheckLDC()                       { return m_bCheckLDC; }

    Bool      getMvdL1ZeroFlag()                  { return m_bLMvdL1Zero; }

    Int       getNumRpsCurrTempList();

    Int       getList1IdxToList0Idx(Int list1Idx) { return m_list1IdxToList0Idx[list1Idx]; }

    void      setReferenced(Bool b)            { m_bReferenced = b; }

    Bool      isReferenced()                   { return m_bReferenced; }

    void      setPOC(Int i)                    { m_poc = i; }

    void      setNalUnitType(NalUnitType e)    { m_nalUnitType = e; }

    NalUnitType getNalUnitType() const         { return m_nalUnitType; }

    Bool      getRapPicFlag();
    Bool      getIdrPicFlag()                  { return getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL ||
                                                        getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_N_LP; }

    Bool      isIRAP() const                   { return (getNalUnitType() >= 16) && (getNalUnitType() <= 23); }

    void      checkCRA(TComReferencePictureSet *rps, Int& pocCRA, Bool& prevRAPisBLA);

    void      setSliceType(SliceType e)               { m_sliceType = e; }

    void      setSliceQp(Int i)                       { m_sliceQp = i; }

    void      setSliceQpBase(Int i)                   { m_sliceQpBase = i; }

    void      setSliceQpDelta(Int i)                  { m_sliceQpDelta = i; }

    void      setSliceQpDeltaCb(Int i)                { m_sliceQpDeltaCb = i; }

    void      setSliceQpDeltaCr(Int i)                { m_sliceQpDeltaCr = i; }

    void      setDeblockingFilterDisable(Bool b)      { m_deblockingFilterDisable = b; }

    void      setDeblockingFilterOverrideFlag(Bool b) { m_deblockingFilterOverrideFlag = b; }

    void      setDeblockingFilterBetaOffsetDiv2(Int i) { m_deblockingFilterBetaOffsetDiv2 = i; }

    void      setDeblockingFilterTcOffsetDiv2(Int i)   { m_deblockingFilterTcOffsetDiv2 = i; }

    void      setRefPic(TComPic* p, RefPicList e, Int refIdx) { m_refPicList[e][refIdx] = p; }

    void      setRefPOC(Int i, RefPicList e, Int refIdx) { m_refPOCList[e][refIdx] = i; }

    void      setNumRefIdx(RefPicList e, Int i)   { m_numRefIdx[e] = i; }

    void      setPic(TComPic* p)                  { m_pic = p; }

    void      setDepth(Int depth)                 { m_depth = depth; }

    void      setRefPicList(TComList<TComPic*>& picList, Bool checkNumPocTotalCurr = false);

    void      setRefPOCList();

    void      setColFromL0Flag(UInt colFromL0) { m_colFromL0Flag = colFromL0; }

    void      setColRefIdx(UInt refIdx)     { m_colRefIdx = refIdx; }

    void      setCheckLDC(Bool b)           { m_bCheckLDC = b; }

    void      setMvdL1ZeroFlag(Bool b)      { m_bLMvdL1Zero = b; }

    Bool      isIntra()                     { return m_sliceType == I_SLICE; }

    Bool      isInterB()                    { return m_sliceType == B_SLICE; }

    Bool      isInterP()                    { return m_sliceType == P_SLICE; }

    void      setLambda(Double d, Double e) { m_lumaLambda = d; m_chromaLambda = e; }

    Double    getLambdaLuma()               { return m_lumaLambda; }

    Double    getLambdaChroma()             { return m_chromaLambda; }

    void      initEqualRef();

    Bool      isEqualRef(RefPicList e, Int refIdx1, Int refIdx2)
    {
        if (refIdx1 < 0 || refIdx2 < 0) return false;
        return m_bEqualRef[e][refIdx1][refIdx2];
    }

    void setEqualRef(RefPicList e, Int refIdx1, Int refIdx2, Bool b)
    {
        m_bEqualRef[e][refIdx1][refIdx2] = m_bEqualRef[e][refIdx2][refIdx1] = b;
    }

    static void sortPicList(TComList<TComPic*>& picList);

    void setList1IdxToList0Idx();

    void setTLayerInfo(UInt tlayer);
    void decodingMarking(TComList<TComPic*>& picList, Int gopSize, Int& maxRefPicNum);
    Int  checkThatAllRefPicsAreAvailable(TComList<TComPic*>& picList, TComReferencePictureSet *rps, Bool printErrors, Int pocRandomAccess = 0);
    void createExplicitReferencePictureSetFromReference(TComList<TComPic*>& picList, TComReferencePictureSet *rps, Bool isRAP);

    void setMaxNumMergeCand(UInt val)          { m_maxNumMergeCand = val; }

    UInt getMaxNumMergeCand()                  { return m_maxNumMergeCand; }

    void setSliceCurEndCUAddr(UInt uiAddr)     { m_sliceCurEndCUAddr = uiAddr; }

    UInt getSliceCurEndCUAddr()                { return m_sliceCurEndCUAddr; }

    void copySliceInfo(TComSlice *pcSliceSrc);

    void setNextSlice(Bool b)                  { m_nextSlice = b; }

    Bool isNextSlice()                         { return m_nextSlice; }

    void setSliceBits(UInt val)                { m_sliceBits = val; }

    UInt getSliceBits()                        { return m_sliceBits; }

    void setSliceSegmentBits(UInt val)         { m_sliceSegmentBits = val; }

    UInt getSliceSegmentBits()                 { return m_sliceSegmentBits; }

    void setFinalized(Bool val)                { m_bFinalized = val; }

    Bool getFinalized()                        { return m_bFinalized; }

    void  setWpScaling(wpScalingParam wp[2][MAX_NUM_REF][3]) { memcpy(m_weightPredTable, wp, sizeof(wpScalingParam) * 2 * MAX_NUM_REF * 3); }

    void  getWpScaling(RefPicList e, Int refIdx, wpScalingParam *&wp);

    void  resetWpScaling();
    void  initWpScaling();
    inline Bool applyWP() { return (m_sliceType == P_SLICE && m_pps->getUseWP()) || (m_sliceType == B_SLICE && m_pps->getWPBiPred()); }

    void  setWpAcDcParam(wpACDCParam wp[3]) { memcpy(m_weightACDCParam, wp, sizeof(wpACDCParam) * 3); }

    void  getWpAcDcParam(wpACDCParam *&wp);
    void  initWpAcDcParam();

    void setTileLocationCount(UInt cnt)          { return m_tileByteLocation.resize(cnt); }

    UInt getTileLocationCount()                  { return (UInt)m_tileByteLocation.size(); }

    void setTileLocation(Int idx, UInt location)
    {
        assert(idx < (Int)m_tileByteLocation.size());
        m_tileByteLocation[idx] = location;
    }

    void addTileLocation(UInt location)          { m_tileByteLocation.push_back(location); }

    UInt getTileLocation(Int idx)                { return m_tileByteLocation[idx]; }

    void setTileOffstForMultES(UInt offset)      { m_tileOffstForMultES = offset; }

    UInt getTileOffstForMultES()                 { return m_tileOffstForMultES; }

    void allocSubstreamSizes(UInt uiNumSubstreams);
    UInt* getSubstreamSizes()                    { return m_substreamSizes; }

    void  setScalingList(TComScalingList* scalingList) { m_scalingList = scalingList; }

    TComScalingList*   getScalingList()          { return m_scalingList; }

    void  setDefaultScalingList();
    Bool  checkDefaultScalingList();
    void      setCabacInitFlag(Bool val)   { m_cabacInitFlag = val; }   //!< set CABAC initial flag

    Bool      getCabacInitFlag()           { return m_cabacInitFlag; }  //!< get CABAC initial flag

    void      setNumEntryPointOffsets(Int val)  { m_numEntryPointOffsets = val; }

    Int       getNumEntryPointOffsets()         { return m_numEntryPointOffsets; }

    Bool      getTemporalLayerNonReferenceFlag()       { return m_temporalLayerNonReferenceFlag; }

    void      setTemporalLayerNonReferenceFlag(Bool x) { m_temporalLayerNonReferenceFlag = x; }

    void      setEnableTMVPFlag(Bool b)    { m_enableTMVPFlag = b; }

    Bool      getEnableTMVPFlag()          { return m_enableTMVPFlag; }

protected:

    TComPic*  xGetRefPic(TComList<TComPic*>& picList, Int poc);

    TComPic*  xGetLongTermRefPic(TComList<TComPic*>& picList, Int poc, Bool pocHasMsb);
}; // END CLASS DEFINITION TComSlice

template<class T>
class ParameterSetMap
{
public:

    ParameterSetMap(Int maxId)
        : m_maxId(maxId)
    {}

    ~ParameterSetMap()
    {
        for (typename std::map<Int, T *>::iterator i = m_paramsetMap.begin(); i != m_paramsetMap.end(); i++)
        {
            delete (*i).second;
        }
    }

    void storePS(Int psId, T *ps)
    {
        assert(psId < m_maxId);
        if (m_paramsetMap.find(psId) != m_paramsetMap.end())
        {
            delete m_paramsetMap[psId];
        }
        m_paramsetMap[psId] = ps;
    }

    void mergePSList(ParameterSetMap<T> &rPsList)
    {
        for (typename std::map<Int, T *>::iterator i = rPsList.m_paramsetMap.begin(); i != rPsList.m_paramsetMap.end(); i++)
        {
            storePS(i->first, i->second);
        }

        rPsList.m_paramsetMap.clear();
    }

    T* getPS(Int psId)
    {
        return (m_paramsetMap.find(psId) == m_paramsetMap.end()) ? NULL : m_paramsetMap[psId];
    }

    T* getFirstPS()
    {
        return (m_paramsetMap.begin() == m_paramsetMap.end()) ? NULL : m_paramsetMap.begin()->second;
    }

private:

    std::map<Int, T *> m_paramsetMap;
    Int               m_maxId;
};

class ParameterSetManager
{
public:

    ParameterSetManager();
    virtual ~ParameterSetManager();

    //! store sequence parameter set and take ownership of it
    void storeVPS(TComVPS *vps) { m_vpsMap.storePS(vps->getVPSId(), vps); }

    //! get pointer to existing video parameter set
    TComVPS* getVPS(Int vpsId)  { return m_vpsMap.getPS(vpsId); }

    TComVPS* getFirstVPS()      { return m_vpsMap.getFirstPS(); }

    //! store sequence parameter set and take ownership of it
    void storeSPS(TComSPS *sps) { m_spsMap.storePS(sps->getSPSId(), sps); }

    //! get pointer to existing sequence parameter set
    TComSPS* getSPS(Int spsId)  { return m_spsMap.getPS(spsId); }

    TComSPS* getFirstSPS()      { return m_spsMap.getFirstPS(); }

    //! store picture parameter set and take ownership of it
    void storePPS(TComPPS *pps) { m_ppsMap.storePS(pps->getPPSId(), pps); }

    //! get pointer to existing picture parameter set
    TComPPS* getPPS(Int ppsId)  { return m_ppsMap.getPS(ppsId); }

    TComPPS* getFirstPPS()      { return m_ppsMap.getFirstPS(); }

    //! activate a SPS from a active parameter sets SEI message
    //! \returns true, if activation is successful
    Bool activateSPSWithSEI(Int SPSId);

    //! activate a PPS and depending on isIDR parameter also SPS and VPS
    //! \returns true, if activation is successful
    Bool activatePPS(Int ppsId, Bool isIRAP);

    TComVPS* getActiveVPS() { return m_vpsMap.getPS(m_activeVPSId); }

    TComSPS* getActiveSPS() { return m_spsMap.getPS(m_activeSPSId); }

    TComPPS* getActivePPS() { return m_ppsMap.getPS(m_activePPSId); }

protected:

    ParameterSetMap<TComVPS> m_vpsMap;
    ParameterSetMap<TComSPS> m_spsMap;
    ParameterSetMap<TComPPS> m_ppsMap;

    Int m_activeVPSId;
    Int m_activeSPSId;
    Int m_activePPSId;
};
}
//! \}

#endif // __TCOMSLICE__
