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

#include "TLibCommon/TComBitCounter.h"
#include "TLibCommon/TComBitStream.h"
#include "TLibCommon/SEI.h"
#include "TLibCommon/TComSlice.h"
#include "SEIwrite.h"

//! \ingroup TLibEncoder
//! \{

#if ENC_DEC_TRACE
Void  xTraceSEIHeader()
{
  fprintf( g_hTrace, "=========== SEI message ===========\n");
}

Void  xTraceSEIMessageType(SEI::PayloadType payloadType)
{
  switch (payloadType)
  {
  case SEI::DECODED_PICTURE_HASH:
    fprintf( g_hTrace, "=========== Decoded picture hash SEI message ===========\n");
    break;
  case SEI::USER_DATA_UNREGISTERED:
    fprintf( g_hTrace, "=========== User Data Unregistered SEI message ===========\n");
    break;
  case SEI::ACTIVE_PARAMETER_SETS:
    fprintf( g_hTrace, "=========== Active Parameter sets SEI message ===========\n");
    break;
  case SEI::BUFFERING_PERIOD:
    fprintf( g_hTrace, "=========== Buffering period SEI message ===========\n");
    break;
  case SEI::PICTURE_TIMING:
    fprintf( g_hTrace, "=========== Picture timing SEI message ===========\n");
    break;
  case SEI::RECOVERY_POINT:
    fprintf( g_hTrace, "=========== Recovery point SEI message ===========\n");
    break;
  case SEI::FRAME_PACKING:
    fprintf( g_hTrace, "=========== Frame Packing Arrangement SEI message ===========\n");
    break;
  case SEI::DISPLAY_ORIENTATION:
    fprintf( g_hTrace, "=========== Display Orientation SEI message ===========\n");
    break;
  case SEI::TEMPORAL_LEVEL0_INDEX:
    fprintf( g_hTrace, "=========== Temporal Level Zero Index SEI message ===========\n");
    break;
  case SEI::REGION_REFRESH_INFO:
    fprintf( g_hTrace, "=========== Gradual Decoding Refresh Information SEI message ===========\n");
    break;
  case SEI::DECODING_UNIT_INFO:
    fprintf( g_hTrace, "=========== Decoding Unit Information SEI message ===========\n");
    break;
#if J0149_TONE_MAPPING_SEI
  case SEI::TONE_MAPPING_INFO:
    fprintf( g_hTrace, "=========== Tone Mapping Info SEI message ===========\n");
    break;
#endif
#if L0208_SOP_DESCRIPTION_SEI
  case SEI::SOP_DESCRIPTION:
    fprintf( g_hTrace, "=========== SOP Description SEI message ===========\n");
    break;
#endif
#if K0180_SCALABLE_NESTING_SEI
  case SEI::SCALABLE_NESTING:
    fprintf( g_hTrace, "=========== Scalable Nesting SEI message ===========\n");
    break;
#endif
  default:
    fprintf( g_hTrace, "=========== Unknown SEI message ===========\n");
    break;
  }
}
#endif

#if K0180_SCALABLE_NESTING_SEI
void SEIWriter::xWriteSEIpayloadData(TComBitIf& bs, const SEI& sei, TComSPS *sps)
#else
void SEIWriter::xWriteSEIpayloadData(const SEI& sei, TComSPS *sps)
#endif
{
  switch (sei.payloadType())
  {
  case SEI::USER_DATA_UNREGISTERED:
    xWriteSEIuserDataUnregistered(*static_cast<const SEIuserDataUnregistered*>(&sei));
    break;
  case SEI::ACTIVE_PARAMETER_SETS:
    xWriteSEIActiveParameterSets(*static_cast<const SEIActiveParameterSets*>(& sei)); 
    break; 
  case SEI::DECODING_UNIT_INFO:
    xWriteSEIDecodingUnitInfo(*static_cast<const SEIDecodingUnitInfo*>(& sei), sps);
    break;
  case SEI::DECODED_PICTURE_HASH:
    xWriteSEIDecodedPictureHash(*static_cast<const SEIDecodedPictureHash*>(&sei));
    break;
  case SEI::BUFFERING_PERIOD:
    xWriteSEIBufferingPeriod(*static_cast<const SEIBufferingPeriod*>(&sei), sps);
    break;
  case SEI::PICTURE_TIMING:
    xWriteSEIPictureTiming(*static_cast<const SEIPictureTiming*>(&sei), sps);
    break;
  case SEI::RECOVERY_POINT:
    xWriteSEIRecoveryPoint(*static_cast<const SEIRecoveryPoint*>(&sei));
    break;
  case SEI::FRAME_PACKING:
    xWriteSEIFramePacking(*static_cast<const SEIFramePacking*>(&sei));
    break;
  case SEI::DISPLAY_ORIENTATION:
    xWriteSEIDisplayOrientation(*static_cast<const SEIDisplayOrientation*>(&sei));
    break;
  case SEI::TEMPORAL_LEVEL0_INDEX:
    xWriteSEITemporalLevel0Index(*static_cast<const SEITemporalLevel0Index*>(&sei));
    break;
  case SEI::REGION_REFRESH_INFO:
    xWriteSEIGradualDecodingRefreshInfo(*static_cast<const SEIGradualDecodingRefreshInfo*>(&sei));
    break;
#if J0149_TONE_MAPPING_SEI
  case SEI::TONE_MAPPING_INFO:
    xWriteSEIToneMappingInfo(*static_cast<const SEIToneMappingInfo*>(&sei));
    break;
#endif
#if L0208_SOP_DESCRIPTION_SEI
  case SEI::SOP_DESCRIPTION:
    xWriteSEISOPDescription(*static_cast<const SEISOPDescription*>(&sei));
    break;
#endif
#if K0180_SCALABLE_NESTING_SEI
  case SEI::SCALABLE_NESTING:
    xWriteSEIScalableNesting(bs, *static_cast<const SEIScalableNesting*>(&sei), sps);
    break;
#endif
  default:
    assert(!"Unhandled SEI message");
  }
}

/**
 * marshal a single SEI message sei, storing the marshalled representation
 * in bitstream bs.
 */
Void SEIWriter::writeSEImessage(TComBitIf& bs, const SEI& sei, TComSPS *sps)
{
  /* calculate how large the payload data is */
  /* TODO: this would be far nicer if it used vectored buffers */
  TComBitCounter bs_count;
  bs_count.resetBits();
  setBitstream(&bs_count);


#if K0180_SCALABLE_NESTING_SEI

#if ENC_DEC_TRACE
  Bool traceEnable = g_HLSTraceEnable;
  g_HLSTraceEnable = false;
#endif
  xWriteSEIpayloadData(bs_count, sei, sps);
#if ENC_DEC_TRACE
  g_HLSTraceEnable = traceEnable;
#endif

#else

#if ENC_DEC_TRACE
  g_HLSTraceEnable = false;
#endif
  xWriteSEIpayloadData(sei, sps);
#if ENC_DEC_TRACE
  g_HLSTraceEnable = true;
#endif

#endif

  UInt payload_data_num_bits = bs_count.getNumberOfWrittenBits();
  assert(0 == payload_data_num_bits % 8);

  setBitstream(&bs);

#if ENC_DEC_TRACE
#if K0180_SCALABLE_NESTING_SEI
  if (g_HLSTraceEnable)
#endif
  xTraceSEIHeader();
#endif

  UInt payloadType = sei.payloadType();
  for (; payloadType >= 0xff; payloadType -= 0xff)
  {
    WRITE_CODE(0xff, 8, "payload_type");
  }
  WRITE_CODE(payloadType, 8, "payload_type");

  UInt payloadSize = payload_data_num_bits/8;
  for (; payloadSize >= 0xff; payloadSize -= 0xff)
  {
    WRITE_CODE(0xff, 8, "payload_size");
  }
  WRITE_CODE(payloadSize, 8, "payload_size");

  /* payloadData */
#if ENC_DEC_TRACE
#if K0180_SCALABLE_NESTING_SEI
  if (g_HLSTraceEnable)
#endif
  xTraceSEIMessageType(sei.payloadType());
#endif

#if K0180_SCALABLE_NESTING_SEI
  xWriteSEIpayloadData(bs, sei, sps);
#else
  xWriteSEIpayloadData(sei, sps);
#endif
}

/**
 * marshal a user_data_unregistered SEI message sei, storing the marshalled
 * representation in bitstream bs.
 */
Void SEIWriter::xWriteSEIuserDataUnregistered(const SEIuserDataUnregistered &sei)
{
  for (UInt i = 0; i < 16; i++)
  {
    WRITE_CODE(sei.uuid_iso_iec_11578[i], 8 , "sei.uuid_iso_iec_11578[i]");
  }

  for (UInt i = 0; i < sei.userDataLength; i++)
  {
    WRITE_CODE(sei.userData[i], 8 , "user_data");
  }
}

/**
 * marshal a decoded picture hash SEI message, storing the marshalled
 * representation in bitstream bs.
 */
Void SEIWriter::xWriteSEIDecodedPictureHash(const SEIDecodedPictureHash& sei)
{
  UInt val;

  WRITE_CODE(sei.method, 8, "hash_type");

  for(Int yuvIdx = 0; yuvIdx < 3; yuvIdx++)
  {
    if(sei.method == SEIDecodedPictureHash::MD5)
    {
      for (UInt i = 0; i < 16; i++)
      {
        WRITE_CODE(sei.digest[yuvIdx][i], 8, "picture_md5");
      }
    }
    else if(sei.method == SEIDecodedPictureHash::CRC)
    {
      val = (sei.digest[yuvIdx][0] << 8)  + sei.digest[yuvIdx][1];
      WRITE_CODE(val, 16, "picture_crc");
    }
    else if(sei.method == SEIDecodedPictureHash::CHECKSUM)
    {
      val = (sei.digest[yuvIdx][0] << 24)  + (sei.digest[yuvIdx][1] << 16) + (sei.digest[yuvIdx][2] << 8) + sei.digest[yuvIdx][3];
      WRITE_CODE(val, 32, "picture_checksum");
    }
  }
}

Void SEIWriter::xWriteSEIActiveParameterSets(const SEIActiveParameterSets& sei)
{
  WRITE_CODE(sei.activeVPSId,     4, "active_vps_id");
#if L0047_APS_FLAGS
  WRITE_FLAG(sei.m_fullRandomAccessFlag, "full_random_access_flag");
  WRITE_FLAG(sei.m_noParamSetUpdateFlag, "no_param_set_update_flag");
#endif
  WRITE_UVLC(sei.numSpsIdsMinus1,    "num_sps_ids_minus1");

  assert (sei.activeSeqParamSetId.size() == (sei.numSpsIdsMinus1 + 1));

  for (Int i = 0; i < sei.activeSeqParamSetId.size(); i++)
  {
    WRITE_UVLC(sei.activeSeqParamSetId[i], "active_seq_param_set_id"); 
  }

  UInt uiBits = m_pcBitIf->getNumberOfWrittenBits();
  UInt uiAlignedBits = ( 8 - (uiBits&7) ) % 8;  
  if(uiAlignedBits) 
  {
    WRITE_FLAG(1, "alignment_bit" );
    uiAlignedBits--; 
    while(uiAlignedBits--)
    {
      WRITE_FLAG(0, "alignment_bit" );
    }
  }
}

Void SEIWriter::xWriteSEIDecodingUnitInfo(const SEIDecodingUnitInfo& sei, TComSPS *sps)
{
  TComVUI *vui = sps->getVuiParameters();
  WRITE_UVLC(sei.m_decodingUnitIdx, "decoding_unit_idx");
  if(vui->getHrdParameters()->getSubPicCpbParamsInPicTimingSEIFlag())
  {
    WRITE_CODE( sei.m_duSptCpbRemovalDelay, (vui->getHrdParameters()->getDuCpbRemovalDelayLengthMinus1() + 1), "du_spt_cpb_removal_delay");
  }
#if L0044_DU_DPB_OUTPUT_DELAY_HRD
  WRITE_FLAG( sei.m_dpbOutputDuDelayPresentFlag, "dpb_output_du_delay_present_flag");
  if(sei.m_dpbOutputDuDelayPresentFlag)
  {
    WRITE_CODE(sei.m_picSptDpbOutputDuDelay, vui->getHrdParameters()->getDpbOutputDelayDuLengthMinus1() + 1, "pic_spt_dpb_output_du_delay");
  }
#endif
  xWriteByteAlign();
}

Void SEIWriter::xWriteSEIBufferingPeriod(const SEIBufferingPeriod& sei, TComSPS *sps)
{
  Int i, nalOrVcl;
  TComVUI *vui = sps->getVuiParameters();
  TComHRD *hrd = vui->getHrdParameters();

  WRITE_UVLC( sei.m_bpSeqParameterSetId, "bp_seq_parameter_set_id" );
  if( !hrd->getSubPicCpbParamsPresentFlag() )
  {
    WRITE_FLAG( sei.m_rapCpbParamsPresentFlag, "rap_cpb_params_present_flag" );
  }
#if L0328_SPLICING
  WRITE_FLAG( sei.m_concatenationFlag, "concatenation_flag");
  WRITE_CODE( sei.m_auCpbRemovalDelayDelta - 1, ( hrd->getCpbRemovalDelayLengthMinus1() + 1 ), "au_cpb_removal_delay_delta_minus1" );
#endif
#if L0044_CPB_DPB_DELAY_OFFSET
  if( sei.m_rapCpbParamsPresentFlag )
  {
    WRITE_CODE( sei.m_cpbDelayOffset, hrd->getCpbRemovalDelayLengthMinus1() + 1, "cpb_delay_offset" );
    WRITE_CODE( sei.m_dpbDelayOffset, hrd->getDpbOutputDelayLengthMinus1()  + 1, "dpb_delay_offset" );
  }
#endif
  for( nalOrVcl = 0; nalOrVcl < 2; nalOrVcl ++ )
  {
    if( ( ( nalOrVcl == 0 ) && ( hrd->getNalHrdParametersPresentFlag() ) ) ||
        ( ( nalOrVcl == 1 ) && ( hrd->getVclHrdParametersPresentFlag() ) ) )
    {
      for( i = 0; i < ( hrd->getCpbCntMinus1( 0 ) + 1 ); i ++ )
      {
        WRITE_CODE( sei.m_initialCpbRemovalDelay[i][nalOrVcl],( hrd->getInitialCpbRemovalDelayLengthMinus1() + 1 ) ,           "initial_cpb_removal_delay" );
        WRITE_CODE( sei.m_initialCpbRemovalDelayOffset[i][nalOrVcl],( hrd->getInitialCpbRemovalDelayLengthMinus1() + 1 ),      "initial_cpb_removal_delay_offset" );
        if( hrd->getSubPicCpbParamsPresentFlag() || sei.m_rapCpbParamsPresentFlag )
        {
          WRITE_CODE( sei.m_initialAltCpbRemovalDelay[i][nalOrVcl], ( hrd->getInitialCpbRemovalDelayLengthMinus1() + 1 ) ,     "initial_alt_cpb_removal_delay" );
          WRITE_CODE( sei.m_initialAltCpbRemovalDelayOffset[i][nalOrVcl], ( hrd->getInitialCpbRemovalDelayLengthMinus1() + 1 ),"initial_alt_cpb_removal_delay_offset" );
        }
      }
    }
  }
  xWriteByteAlign();
}
Void SEIWriter::xWriteSEIPictureTiming(const SEIPictureTiming& sei,  TComSPS *sps)
{
  Int i;
  TComVUI *vui = sps->getVuiParameters();
  TComHRD *hrd = vui->getHrdParameters();

#if !L0045_CONDITION_SIGNALLING
  // This condition was probably OK before the pic_struct, progressive_source_idc, duplicate_flag were added
  if( !hrd->getNalHrdParametersPresentFlag() && !hrd->getVclHrdParametersPresentFlag() )
    return;
#endif
  if( vui->getFrameFieldInfoPresentFlag() )
  {
    WRITE_CODE( sei.m_picStruct, 4,              "pic_struct" );
#if L0046_RENAME_PROG_SRC_IDC
    WRITE_CODE( sei.m_sourceScanType, 2,         "source_scan_type" );
#else
    WRITE_CODE( sei.m_progressiveSourceIdc, 2,   "progressive_source_idc" );
#endif
    WRITE_FLAG( sei.m_duplicateFlag ? 1 : 0,     "duplicate_flag" );
  }

#if L0045_CONDITION_SIGNALLING
  if( hrd->getCpbDpbDelaysPresentFlag() )
  {
#endif
    WRITE_CODE( sei.m_auCpbRemovalDelay - 1, ( hrd->getCpbRemovalDelayLengthMinus1() + 1 ),                                         "au_cpb_removal_delay_minus1" );
    WRITE_CODE( sei.m_picDpbOutputDelay, ( hrd->getDpbOutputDelayLengthMinus1() + 1 ),                                          "pic_dpb_output_delay" );
#if L0044_DU_DPB_OUTPUT_DELAY_HRD
    if(hrd->getSubPicCpbParamsPresentFlag())
    {
      WRITE_CODE(sei.m_picDpbOutputDuDelay, hrd->getDpbOutputDelayDuLengthMinus1()+1, "pic_dpb_output_du_delay" );
    }
#endif
    if( hrd->getSubPicCpbParamsPresentFlag() && hrd->getSubPicCpbParamsInPicTimingSEIFlag() )
    {
      WRITE_UVLC( sei.m_numDecodingUnitsMinus1,     "num_decoding_units_minus1" );
      WRITE_FLAG( sei.m_duCommonCpbRemovalDelayFlag, "du_common_cpb_removal_delay_flag" );
      if( sei.m_duCommonCpbRemovalDelayFlag )
      {
        WRITE_CODE( sei.m_duCommonCpbRemovalDelayMinus1, ( hrd->getDuCpbRemovalDelayLengthMinus1() + 1 ),                       "du_common_cpb_removal_delay_minus1" );
      }
      for( i = 0; i <= sei.m_numDecodingUnitsMinus1; i ++ )
      {
        WRITE_UVLC( sei.m_numNalusInDuMinus1[ i ],  "num_nalus_in_du_minus1");
        if( ( !sei.m_duCommonCpbRemovalDelayFlag ) && ( i < sei.m_numDecodingUnitsMinus1 ) )
        {
          WRITE_CODE( sei.m_duCpbRemovalDelayMinus1[ i ], ( hrd->getDuCpbRemovalDelayLengthMinus1() + 1 ),                        "du_cpb_removal_delay_minus1" );
        }
      }
    }
#if L0045_CONDITION_SIGNALLING
  }
#endif
  xWriteByteAlign();
}
Void SEIWriter::xWriteSEIRecoveryPoint(const SEIRecoveryPoint& sei)
{
  WRITE_SVLC( sei.m_recoveryPocCnt,    "recovery_poc_cnt"    );
  WRITE_FLAG( sei.m_exactMatchingFlag, "exact_matching_flag" );
  WRITE_FLAG( sei.m_brokenLinkFlag,    "broken_link_flag"    );
  xWriteByteAlign();
}
Void SEIWriter::xWriteSEIFramePacking(const SEIFramePacking& sei)
{
  WRITE_UVLC( sei.m_arrangementId,                  "frame_packing_arrangement_id" );
  WRITE_FLAG( sei.m_arrangementCancelFlag,          "frame_packing_arrangement_cancel_flag" );

  if( sei.m_arrangementCancelFlag == 0 ) {
    WRITE_CODE( sei.m_arrangementType, 7,           "frame_packing_arrangement_type" );

    WRITE_FLAG( sei.m_quincunxSamplingFlag,         "quincunx_sampling_flag" );
    WRITE_CODE( sei.m_contentInterpretationType, 6, "content_interpretation_type" );
    WRITE_FLAG( sei.m_spatialFlippingFlag,          "spatial_flipping_flag" );
    WRITE_FLAG( sei.m_frame0FlippedFlag,            "frame0_flipped_flag" );
    WRITE_FLAG( sei.m_fieldViewsFlag,               "field_views_flag" );
    WRITE_FLAG( sei.m_currentFrameIsFrame0Flag,     "current_frame_is_frame0_flag" );

    WRITE_FLAG( sei.m_frame0SelfContainedFlag,      "frame0_self_contained_flag" );
    WRITE_FLAG( sei.m_frame1SelfContainedFlag,      "frame1_self_contained_flag" );

    if(sei.m_quincunxSamplingFlag == 0 && sei.m_arrangementType != 5)
    {
      WRITE_CODE( sei.m_frame0GridPositionX, 4,     "frame0_grid_position_x" );
      WRITE_CODE( sei.m_frame0GridPositionY, 4,     "frame0_grid_position_y" );
      WRITE_CODE( sei.m_frame1GridPositionX, 4,     "frame1_grid_position_x" );
      WRITE_CODE( sei.m_frame1GridPositionY, 4,     "frame1_grid_position_y" );
    }

    WRITE_CODE( sei.m_arrangementReservedByte, 8,   "frame_packing_arrangement_reserved_byte" );
#if L0045_PERSISTENCE_FLAGS
    WRITE_FLAG( sei.m_arrangementPersistenceFlag,   "frame_packing_arrangement_persistence_flag" );
#else
    WRITE_UVLC( sei.m_arrangementRepetetionPeriod,  "frame_packing_arrangement_repetition_period" );
#endif
  }

  WRITE_FLAG( sei.m_upsampledAspectRatio,           "upsampled_aspect_ratio" );

  xWriteByteAlign();
}

#if J0149_TONE_MAPPING_SEI
Void SEIWriter::xWriteSEIToneMappingInfo(const SEIToneMappingInfo& sei)
{
  Int i;
  WRITE_UVLC( sei.m_toneMapId,                    "tone_map_id" );
  WRITE_FLAG( sei.m_toneMapCancelFlag,            "tone_map_cancel_flag" );
  if( !sei.m_toneMapCancelFlag ) 
  {
    WRITE_FLAG( sei.m_toneMapPersistenceFlag,     "tone_map_persistence_flag" );
    WRITE_CODE( sei.m_codedDataBitDepth,    8,    "coded_data_bit_depth" );
    WRITE_CODE( sei.m_targetBitDepth,       8,    "target_bit_depth" );
    WRITE_UVLC( sei.m_modelId,                    "model_id" );
    switch(sei.m_modelId)
    {
    case 0:
      {
        WRITE_CODE( sei.m_minValue,  32,        "min_value" );
        WRITE_CODE( sei.m_maxValue, 32,         "max_value" );
        break;
      }
    case 1:
      {
        WRITE_CODE( sei.m_sigmoidMidpoint, 32,  "sigmoid_midpoint" );
        WRITE_CODE( sei.m_sigmoidWidth,    32,  "sigmoid_width"    );
        break;
      }
    case 2:
      {
        UInt num = 1u << sei.m_targetBitDepth;
        for(i = 0; i < num; i++)
        {
          WRITE_CODE( sei.m_startOfCodedInterval[i], (( sei.m_codedDataBitDepth + 7 ) >> 3 ) << 3,  "start_of_coded_interval" );
        }
        break;
      }
    case 3:
      {
        WRITE_CODE( sei.m_numPivots, 16,          "num_pivots" );
        for(i = 0; i < sei.m_numPivots; i++ )
        {
          WRITE_CODE( sei.m_codedPivotValue[i], (( sei.m_codedDataBitDepth + 7 ) >> 3 ) << 3,       "coded_pivot_value" );
          WRITE_CODE( sei.m_targetPivotValue[i], (( sei.m_targetBitDepth + 7 ) >> 3 ) << 3,         "target_pivot_value");
        }
        break;
      }
    case 4:
      {
        WRITE_CODE( sei.m_cameraIsoSpeedIdc,    8,    "camera_iso_speed_idc" );
        if( sei.m_cameraIsoSpeedIdc == 255) //Extended_ISO
        {
          WRITE_CODE( sei.m_cameraIsoSpeedValue,    32,    "camera_iso_speed_value" );
        }
        WRITE_FLAG( sei.m_exposureCompensationValueSignFlag,           "exposure_compensation_value_sign_flag" );
        WRITE_CODE( sei.m_exposureCompensationValueNumerator,     16,  "exposure_compensation_value_numerator" );
        WRITE_CODE( sei.m_exposureCompensationValueDenomIdc,      16,  "exposure_compensation_value_denom_idc" );
        WRITE_CODE( sei.m_refScreenLuminanceWhite,                32,  "ref_screen_luminance_white" );
        WRITE_CODE( sei.m_extendedRangeWhiteLevel,                32,  "extended_range_white_level" );
        WRITE_CODE( sei.m_nominalBlackLevelLumaCodeValue,         16,  "nominal_black_level_luma_code_value" );
        WRITE_CODE( sei.m_nominalWhiteLevelLumaCodeValue,         16,  "nominal_white_level_luma_code_value" );
        WRITE_CODE( sei.m_extendedWhiteLevelLumaCodeValue,        16,  "extended_white_level_luma_code_value" );
        break;
      }
    default:
      {
        assert(!"Undefined SEIToneMapModelId");
        break;
      }
    }//switch m_modelId
  }//if(!sei.m_toneMapCancelFlag)

  xWriteByteAlign();
}
#endif

Void SEIWriter::xWriteSEIDisplayOrientation(const SEIDisplayOrientation &sei)
{
  WRITE_FLAG( sei.cancelFlag,           "display_orientation_cancel_flag" );
  if( !sei.cancelFlag )
  {
    WRITE_FLAG( sei.horFlip,                   "hor_flip" );
    WRITE_FLAG( sei.verFlip,                   "ver_flip" );
    WRITE_CODE( sei.anticlockwiseRotation, 16, "anticlockwise_rotation" );
#if L0045_PERSISTENCE_FLAGS
    WRITE_FLAG( sei.persistenceFlag,          "display_orientation_persistence_flag" );
#else
    WRITE_UVLC( sei.repetitionPeriod,          "display_orientation_repetition_period" );
#endif
#if !REMOVE_SINGLE_SEI_EXTENSION_FLAGS
    WRITE_FLAG( sei.extensionFlag,             "display_orientation_extension_flag" );
    assert( !sei.extensionFlag );
#endif
  }
  xWriteByteAlign();
}

Void SEIWriter::xWriteSEITemporalLevel0Index(const SEITemporalLevel0Index &sei)
{
  WRITE_CODE( sei.tl0Idx, 8 , "tl0_idx" );
  WRITE_CODE( sei.rapIdx, 8 , "rap_idx" );
  xWriteByteAlign();
}

Void SEIWriter::xWriteSEIGradualDecodingRefreshInfo(const SEIGradualDecodingRefreshInfo &sei)
{
  WRITE_FLAG( sei.m_gdrForegroundFlag, "gdr_foreground_flag");
  xWriteByteAlign();
}

#if L0208_SOP_DESCRIPTION_SEI
Void SEIWriter::xWriteSEISOPDescription(const SEISOPDescription& sei)
{
  WRITE_UVLC( sei.m_sopSeqParameterSetId,           "sop_seq_parameter_set_id"               );
  WRITE_UVLC( sei.m_numPicsInSopMinus1,             "num_pics_in_sop_minus1"               );
  for (UInt i = 0; i <= sei.m_numPicsInSopMinus1; i++)
  {
    WRITE_CODE( sei.m_sopDescVclNaluType[i], 6, "sop_desc_vcl_nalu_type" );
    WRITE_CODE( sei.m_sopDescTemporalId[i],  3, "sop_desc_temporal_id" );
    if (sei.m_sopDescVclNaluType[i] != NAL_UNIT_CODED_SLICE_IDR_W_RADL && sei.m_sopDescVclNaluType[i] != NAL_UNIT_CODED_SLICE_IDR_N_LP)
    {
      WRITE_UVLC( sei.m_sopDescStRpsIdx[i],           "sop_desc_st_rps_idx"               );
    }
    if (i > 0)
    {
      WRITE_SVLC( sei.m_sopDescPocDelta[i],           "sop_desc_poc_delta"               );
    }
  }

  xWriteByteAlign();
}
#endif

#if K0180_SCALABLE_NESTING_SEI
Void SEIWriter::xWriteSEIScalableNesting(TComBitIf& bs, const SEIScalableNesting& sei, TComSPS *sps)
{
  WRITE_FLAG( sei.m_bitStreamSubsetFlag,             "bitstream_subset_flag"         );
  WRITE_FLAG( sei.m_nestingOpFlag,                   "nesting_op_flag      "         );
  if (sei.m_nestingOpFlag)
  {
    WRITE_FLAG( sei.m_defaultOpFlag,                 "default_op_flag"               );
    WRITE_UVLC( sei.m_nestingNumOpsMinus1,           "nesting_num_ops"               );
    for (UInt i = (sei.m_defaultOpFlag ? 1 : 0); i <= sei.m_nestingNumOpsMinus1; i++)
    {
      WRITE_CODE( sei.m_nestingNoOpMaxTemporalIdPlus1, 3, "nesting_no_op_max_temporal_id" );
      WRITE_CODE( sei.m_nestingMaxTemporalIdPlus1[i], 3,  "nesting_max_temporal_id"       );
      WRITE_UVLC( sei.m_nestingOpIdx[i],                  "nesting_op_idx"                );
    }
  }
  else
  {
    WRITE_FLAG( sei.m_allLayersFlag,                      "all_layers_flag"               );
    if (!sei.m_allLayersFlag)
    {
      WRITE_CODE( sei.m_nestingNoOpMaxTemporalIdPlus1, 3, "nesting_no_op_max_temporal_id" );
      WRITE_UVLC( sei.m_nestingNumLayersMinus1,           "nesting_num_layers"            );
      for (UInt i = 0; i <= sei.m_nestingNumLayersMinus1; i++)
      {
        WRITE_CODE( sei.m_nestingLayerId[i], 6,           "nesting_layer_id"              );
      }
    }
  }
 
  // byte alignment
  while ( m_pcBitIf->getNumberOfWrittenBits() % 8 != 0 )
  {
    WRITE_FLAG( 0, "nesting_zero_bit" );
  }

  // write nested SEI messages
  for (SEIMessages::const_iterator it = sei.m_nestedSEIs.begin(); it != sei.m_nestedSEIs.end(); it++)
  {
    writeSEImessage(bs, *(*it), sps);
  }
}
#endif

Void SEIWriter::xWriteByteAlign()
{
  if( m_pcBitIf->getNumberOfWrittenBits() % 8 != 0)
  {
    WRITE_FLAG( 1, "bit_equal_to_one" );
    while( m_pcBitIf->getNumberOfWrittenBits() % 8 != 0 )
    {
      WRITE_FLAG( 0, "bit_equal_to_zero" );
    }
  }
};

//! \}
