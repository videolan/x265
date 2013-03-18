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

#pragma once
#include <list>
#include <vector>
#include <cstring>

//! \ingroup TLibCommon
//! \{
class TComSPS;

/**
 * Abstract class representing an SEI message with lightweight RTTI.
 */
class SEI
{
public:
  enum PayloadType
  {
    BUFFERING_PERIOD                     = 0,
    PICTURE_TIMING                       = 1,
    PAN_SCAN_RECT                        = 2,
    FILLER_PAYLOAD                       = 3,
    USER_DATA_REGISTERED_ITU_T_T35       = 4,
    USER_DATA_UNREGISTERED               = 5,
    RECOVERY_POINT                       = 6,
    SCENE_INFO                           = 9,
    FULL_FRAME_SNAPSHOT                  = 15,
    PROGRESSIVE_REFINEMENT_SEGMENT_START = 16,
    PROGRESSIVE_REFINEMENT_SEGMENT_END   = 17,
    FILM_GRAIN_CHARACTERISTICS           = 19,
    POST_FILTER_HINT                     = 22,
    TONE_MAPPING_INFO                    = 23,
    FRAME_PACKING                        = 45,
    DISPLAY_ORIENTATION                  = 47,
    SOP_DESCRIPTION                      = 128,
    ACTIVE_PARAMETER_SETS                = 129,
    DECODING_UNIT_INFO                   = 130,
    TEMPORAL_LEVEL0_INDEX                = 131,
    DECODED_PICTURE_HASH                 = 132,
    SCALABLE_NESTING                     = 133,
    REGION_REFRESH_INFO                  = 134,
  };
  
  SEI() {}
  virtual ~SEI() {}
  
  virtual PayloadType payloadType() const = 0;
};

class SEIuserDataUnregistered : public SEI
{
public:
  PayloadType payloadType() const { return USER_DATA_UNREGISTERED; }

  SEIuserDataUnregistered()
    : userData(0)
    {}

  virtual ~SEIuserDataUnregistered()
  {
    delete userData;
  }

  UChar uuid_iso_iec_11578[16];
  UInt userDataLength;
  UChar *userData;
};

class SEIDecodedPictureHash : public SEI
{
public:
  PayloadType payloadType() const { return DECODED_PICTURE_HASH; }

  SEIDecodedPictureHash() {}
  virtual ~SEIDecodedPictureHash() {}
  
  enum Method
  {
    MD5,
    CRC,
    CHECKSUM,
    RESERVED,
  } method;

  UChar digest[3][16];
};

class SEIActiveParameterSets : public SEI 
{
public:
  PayloadType payloadType() const { return ACTIVE_PARAMETER_SETS; }

  SEIActiveParameterSets() 
#if !L0047_APS_FLAGS
    :numSpsIdsMinus1(0)
#else
    : activeVPSId            (0)
    , m_fullRandomAccessFlag (false)
    , m_noParamSetUpdateFlag (false)
    , numSpsIdsMinus1        (0)
#endif
  {}
  virtual ~SEIActiveParameterSets() {}

  Int activeVPSId; 
#if L0047_APS_FLAGS
  Bool m_fullRandomAccessFlag;
  Bool m_noParamSetUpdateFlag;
#endif
  Int numSpsIdsMinus1;
  std::vector<Int> activeSeqParamSetId; 
};

class SEIBufferingPeriod : public SEI
{
public:
  PayloadType payloadType() const { return BUFFERING_PERIOD; }

  SEIBufferingPeriod()
#if L0044_CPB_DPB_DELAY_OFFSET
  : m_bpSeqParameterSetId (0)
  , m_rapCpbParamsPresentFlag (false)
  , m_cpbDelayOffset      (0)
  , m_dpbDelayOffset      (0)
  {
    ::memset(m_initialCpbRemovalDelay, 0, sizeof(m_initialCpbRemovalDelay));
    ::memset(m_initialCpbRemovalDelayOffset, 0, sizeof(m_initialCpbRemovalDelayOffset));
    ::memset(m_initialAltCpbRemovalDelay, 0, sizeof(m_initialAltCpbRemovalDelay));
    ::memset(m_initialAltCpbRemovalDelayOffset, 0, sizeof(m_initialAltCpbRemovalDelayOffset));
  }
#else
  {}
#endif
  virtual ~SEIBufferingPeriod() {}

  UInt m_bpSeqParameterSetId;
  Bool m_rapCpbParamsPresentFlag;
#if L0044_CPB_DPB_DELAY_OFFSET
  Bool m_cpbDelayOffset;
  Bool m_dpbDelayOffset;
#endif
  UInt m_initialCpbRemovalDelay         [MAX_CPB_CNT][2];
  UInt m_initialCpbRemovalDelayOffset   [MAX_CPB_CNT][2];
  UInt m_initialAltCpbRemovalDelay      [MAX_CPB_CNT][2];
  UInt m_initialAltCpbRemovalDelayOffset[MAX_CPB_CNT][2];
#if L0328_SPLICING
  Bool m_concatenationFlag;
  UInt m_auCpbRemovalDelayDelta;
#endif
};
class SEIPictureTiming : public SEI
{
public:
  PayloadType payloadType() const { return PICTURE_TIMING; }

  SEIPictureTiming()
  : m_picStruct               (0)
#if L0046_RENAME_PROG_SRC_IDC
  , m_sourceScanType          (0)
#else
  , m_progressiveSourceIdc    (0)
#endif
  , m_duplicateFlag           (false)
#if L0044_DU_DPB_OUTPUT_DELAY_HRD
  , m_picDpbOutputDuDelay     (0)
#endif
  , m_numNalusInDuMinus1      (NULL)
  , m_duCpbRemovalDelayMinus1 (NULL)
  {}
  virtual ~SEIPictureTiming()
  {
    if( m_numNalusInDuMinus1 != NULL )
    {
      delete m_numNalusInDuMinus1;
    }
    if( m_duCpbRemovalDelayMinus1  != NULL )
    {
      delete m_duCpbRemovalDelayMinus1;
    }
  }

  UInt  m_picStruct;
#if L0046_RENAME_PROG_SRC_IDC
  UInt  m_sourceScanType;
#else
  UInt  m_progressiveSourceIdc;
#endif
  Bool  m_duplicateFlag;

  UInt  m_auCpbRemovalDelay;
  UInt  m_picDpbOutputDelay;
#if L0044_DU_DPB_OUTPUT_DELAY_HRD
  UInt  m_picDpbOutputDuDelay;
#endif
  UInt  m_numDecodingUnitsMinus1;
  Bool  m_duCommonCpbRemovalDelayFlag;
  UInt  m_duCommonCpbRemovalDelayMinus1;
  UInt* m_numNalusInDuMinus1;
  UInt* m_duCpbRemovalDelayMinus1;
};

class SEIDecodingUnitInfo : public SEI
{
public:
  PayloadType payloadType() const { return DECODING_UNIT_INFO; }

  SEIDecodingUnitInfo()
    : m_decodingUnitIdx(0)
    , m_duSptCpbRemovalDelay(0)
#if L0044_DU_DPB_OUTPUT_DELAY_HRD
    , m_dpbOutputDuDelayPresentFlag(false)
    , m_picSptDpbOutputDuDelay(0)
#endif
  {}
  virtual ~SEIDecodingUnitInfo() {}
  Int m_decodingUnitIdx;
  Int m_duSptCpbRemovalDelay;
#if L0044_DU_DPB_OUTPUT_DELAY_HRD
  Bool m_dpbOutputDuDelayPresentFlag;
  Int m_picSptDpbOutputDuDelay;
#endif
};

class SEIRecoveryPoint : public SEI
{
public:
  PayloadType payloadType() const { return RECOVERY_POINT; }

  SEIRecoveryPoint() {}
  virtual ~SEIRecoveryPoint() {}

  Int  m_recoveryPocCnt;
  Bool m_exactMatchingFlag;
  Bool m_brokenLinkFlag;
};
class SEIFramePacking : public SEI
{
public:
  PayloadType payloadType() const { return FRAME_PACKING; }

  SEIFramePacking() {}
  virtual ~SEIFramePacking() {}

  Int  m_arrangementId;
  Bool m_arrangementCancelFlag;
  Int  m_arrangementType;
  Bool m_quincunxSamplingFlag;
  Int  m_contentInterpretationType;
  Bool m_spatialFlippingFlag;
  Bool m_frame0FlippedFlag;
  Bool m_fieldViewsFlag;
  Bool m_currentFrameIsFrame0Flag;
  Bool m_frame0SelfContainedFlag;
  Bool m_frame1SelfContainedFlag;
  Int  m_frame0GridPositionX;
  Int  m_frame0GridPositionY;
  Int  m_frame1GridPositionX;
  Int  m_frame1GridPositionY;
  Int  m_arrangementReservedByte;
#if L0045_PERSISTENCE_FLAGS
  Bool m_arrangementPersistenceFlag;
#else
  Int  m_arrangementRepetetionPeriod;
#endif
  Bool m_upsampledAspectRatio;
};

class SEIDisplayOrientation : public SEI
{
public:
  PayloadType payloadType() const { return DISPLAY_ORIENTATION; }

  SEIDisplayOrientation()
    : cancelFlag(true)
#if L0045_PERSISTENCE_FLAGS
    , persistenceFlag(0)
#else
    , repetitionPeriod(1)
#endif
    , extensionFlag(false)
    {}
  virtual ~SEIDisplayOrientation() {}

  Bool cancelFlag;
  Bool horFlip;
  Bool verFlip;

  UInt anticlockwiseRotation;
#if L0045_PERSISTENCE_FLAGS
  Bool persistenceFlag;
#else
  UInt repetitionPeriod;
#endif
  Bool extensionFlag;
};

class SEITemporalLevel0Index : public SEI
{
public:
  PayloadType payloadType() const { return TEMPORAL_LEVEL0_INDEX; }

  SEITemporalLevel0Index()
    : tl0Idx(0)
    , rapIdx(0)
    {}
  virtual ~SEITemporalLevel0Index() {}

  UInt tl0Idx;
  UInt rapIdx;
};

class SEIGradualDecodingRefreshInfo : public SEI
{
public:
  PayloadType payloadType() const { return REGION_REFRESH_INFO; }

  SEIGradualDecodingRefreshInfo()
    : m_gdrForegroundFlag(0)
  {}
  virtual ~SEIGradualDecodingRefreshInfo() {}

  Bool m_gdrForegroundFlag;
};

#if L0208_SOP_DESCRIPTION_SEI
class SEISOPDescription : public SEI
{
public:
  PayloadType payloadType() const { return SOP_DESCRIPTION; }

  SEISOPDescription() {}
  virtual ~SEISOPDescription() {}

  UInt m_sopSeqParameterSetId;
  UInt m_numPicsInSopMinus1;

  UInt m_sopDescVclNaluType[MAX_NUM_PICS_IN_SOP];
  UInt m_sopDescTemporalId[MAX_NUM_PICS_IN_SOP];
  UInt m_sopDescStRpsIdx[MAX_NUM_PICS_IN_SOP];
  Int m_sopDescPocDelta[MAX_NUM_PICS_IN_SOP];
};
#endif

#if J0149_TONE_MAPPING_SEI 
class SEIToneMappingInfo : public SEI
{
public:
  PayloadType payloadType() const { return TONE_MAPPING_INFO; }
  SEIToneMappingInfo() {}
  virtual ~SEIToneMappingInfo() {}

  Int    m_toneMapId;
  Bool   m_toneMapCancelFlag;
  Bool   m_toneMapPersistenceFlag;
  Int    m_codedDataBitDepth;
  Int    m_targetBitDepth;
  Int    m_modelId;
  Int    m_minValue;
  Int    m_maxValue;
  Int    m_sigmoidMidpoint;
  Int    m_sigmoidWidth;
  std::vector<Int> m_startOfCodedInterval;
  Int    m_numPivots;
  std::vector<Int> m_codedPivotValue;
  std::vector<Int> m_targetPivotValue;
  Int    m_cameraIsoSpeedIdc;
  Int    m_cameraIsoSpeedValue;
  Int    m_exposureCompensationValueSignFlag;
  Int    m_exposureCompensationValueNumerator;
  Int    m_exposureCompensationValueDenomIdc;
  Int    m_refScreenLuminanceWhite;
  Int    m_extendedRangeWhiteLevel;
  Int    m_nominalBlackLevelLumaCodeValue;
  Int    m_nominalWhiteLevelLumaCodeValue;
  Int    m_extendedWhiteLevelLumaCodeValue;
};
#endif

typedef std::list<SEI*> SEIMessages;

/// output a selection of SEI messages by payload type. Ownership stays in original message list.
SEIMessages getSeisByType(SEIMessages &seiList, SEI::PayloadType seiType);

/// remove a selection of SEI messages by payload type from the original list and return them in a new list. 
SEIMessages extractSeisByType(SEIMessages &seiList, SEI::PayloadType seiType);

/// delete list of SEI messages (freeing the referenced objects)
Void deleteSEIs (SEIMessages &seiList);

#if K0180_SCALABLE_NESTING_SEI
class SEIScalableNesting : public SEI
{
public:
  PayloadType payloadType() const { return SCALABLE_NESTING; }

  SEIScalableNesting() {}
  virtual ~SEIScalableNesting()
  {
    if (!m_callerOwnsSEIs)
    {
      deleteSEIs(m_nestedSEIs);
    }
  }

  Bool  m_bitStreamSubsetFlag;
  Bool  m_nestingOpFlag;
  Bool  m_defaultOpFlag;                             //value valid if m_nestingOpFlag != 0
  UInt  m_nestingNumOpsMinus1;                       // -"-
  UInt  m_nestingMaxTemporalIdPlus1[MAX_TLAYER];     // -"-
  UInt  m_nestingOpIdx[MAX_NESTING_NUM_OPS];         // -"-

  Bool  m_allLayersFlag;                             //value valid if m_nestingOpFlag == 0
  UInt  m_nestingNoOpMaxTemporalIdPlus1;             //value valid if m_nestingOpFlag == 0 and m_allLayersFlag == 0
  UInt  m_nestingNumLayersMinus1;                    //value valid if m_nestingOpFlag == 0 and m_allLayersFlag == 0
  UChar m_nestingLayerId[MAX_NESTING_NUM_LAYER];     //value valid if m_nestingOpFlag == 0 and m_allLayersFlag == 0. This can e.g. be a static array of 64 unsigned char values

  Bool  m_callerOwnsSEIs;
  SEIMessages m_nestedSEIs;
};
#endif

//! \}
