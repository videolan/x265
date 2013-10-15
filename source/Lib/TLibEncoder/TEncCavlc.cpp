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

/** \file     TEncCavlc.cpp
    \brief    CAVLC encoder class
*/

#include "TLibCommon/CommonDef.h"
#include "TEncCavlc.h"
#include "SEIwrite.h"

using namespace x265;

//! \ingroup TLibEncoder
//! \{

#if ENC_DEC_TRACE

void  xTraceSPSHeader(TComSPS *sps)
{
    fprintf(g_hTrace, "=========== Sequence Parameter Set ID: %d ===========\n", sps->getSPSId());
}

void  xTracePPSHeader(TComPPS *pps)
{
    fprintf(g_hTrace, "=========== Picture Parameter Set ID: %d ===========\n", pps->getPPSId());
}

void  xTraceSliceHeader(TComSlice *)
{
    fprintf(g_hTrace, "=========== Slice ===========\n");
}

#endif // if ENC_DEC_TRACE

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TEncCavlc::TEncCavlc()
{
    m_bitIf           = NULL;
    m_coeffCost       = 0;
}

TEncCavlc::~TEncCavlc()
{}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

void TEncCavlc::resetEntropy()
{}

void TEncCavlc::codeDFFlag(UInt code, const char *symbolName)
{
    (void)symbolName;
    WRITE_FLAG(code, symbolName);
}

void TEncCavlc::codeDFSvlc(int code, const char *symbolName)
{
    (void)symbolName;
    WRITE_SVLC(code, symbolName);
}

#define SCALING_LIST_OUTPUT_RESULT                0 //JCTVC-G880/JCTVC-G1016 quantization matrices

#define PRINT_RPS_INFO                            0 ///< Enable/disable the printing of bits used to send the RPS.
                                                    // using one nearest frame as reference frame, and the other frames are high quality (POC%4==0) frames (1+X)
                                                    // this should be done with encoder only decision
                                                    // but because of the absence of reference frame management, the related code was hard coded currently

void TEncCavlc::codeShortTermRefPicSet(TComReferencePictureSet* rps, bool calledFromSliceHeader, int idx)
{
#if PRINT_RPS_INFO
    int lastBits = getNumberOfWrittenBits();
#endif
    if (idx > 0)
    {
        WRITE_FLAG(rps->getInterRPSPrediction(), "inter_ref_pic_set_prediction_flag"); // inter_RPS_prediction_flag
    }
    if (rps->getInterRPSPrediction())
    {
        int deltaRPS = rps->getDeltaRPS();
        if (calledFromSliceHeader)
        {
            WRITE_UVLC(rps->getDeltaRIdxMinus1(), "delta_idx_minus1"); // delta index of the Reference Picture Set used for prediction minus 1
        }

        WRITE_CODE((deltaRPS >= 0 ? 0 : 1), 1, "delta_rps_sign"); //delta_rps_sign
        WRITE_UVLC(abs(deltaRPS) - 1, "abs_delta_rps_minus1"); // absolute delta RPS minus 1

        for (int j = 0; j < rps->getNumRefIdc(); j++)
        {
            int refIdc = rps->getRefIdc(j);
            WRITE_CODE((refIdc == 1 ? 1 : 0), 1, "used_by_curr_pic_flag"); //first bit is "1" if Idc is 1
            if (refIdc != 1)
            {
                WRITE_CODE(refIdc >> 1, 1, "use_delta_flag"); //second bit is "1" if Idc is 2, "0" otherwise.
            }
        }
    }
    else
    {
        WRITE_UVLC(rps->getNumberOfNegativePictures(), "num_negative_pics");
        WRITE_UVLC(rps->getNumberOfPositivePictures(), "num_positive_pics");
        int prev = 0;
        for (int j = 0; j < rps->getNumberOfNegativePictures(); j++)
        {
            WRITE_UVLC(prev - rps->getDeltaPOC(j) - 1, "delta_poc_s0_minus1");
            prev = rps->getDeltaPOC(j);
            WRITE_FLAG(rps->getUsed(j), "used_by_curr_pic_s0_flag");
        }

        prev = 0;
        for (int j = rps->getNumberOfNegativePictures(); j < rps->getNumberOfNegativePictures() + rps->getNumberOfPositivePictures(); j++)
        {
            WRITE_UVLC(rps->getDeltaPOC(j) - prev - 1, "delta_poc_s1_minus1");
            prev = rps->getDeltaPOC(j);
            WRITE_FLAG(rps->getUsed(j), "used_by_curr_pic_s1_flag");
        }
    }

#if PRINT_RPS_INFO
    printf("irps=%d (%2d bits) ", rps->getInterRPSPrediction(), getNumberOfWrittenBits() - lastBits);
    rps->printDeltaPOC();
#endif
}

void TEncCavlc::codePPS(TComPPS* pps)
{
#if ENC_DEC_TRACE
    xTracePPSHeader(pps);
#endif

    WRITE_UVLC(pps->getPPSId(),                             "pps_pic_parameter_set_id");
    WRITE_UVLC(pps->getSPSId(),                             "pps_seq_parameter_set_id");
    WRITE_FLAG(0,                                             "dependent_slice_segments_enabled_flag");
    WRITE_FLAG(pps->getOutputFlagPresentFlag() ? 1 : 0,     "output_flag_present_flag");
    WRITE_CODE(pps->getNumExtraSliceHeaderBits(), 3,        "num_extra_slice_header_bits");
    WRITE_FLAG(pps->getSignHideFlag(), "sign_data_hiding_flag");
    WRITE_FLAG(pps->getCabacInitPresentFlag() ? 1 : 0,   "cabac_init_present_flag");
    WRITE_UVLC(pps->getNumRefIdxL0DefaultActive() - 1,     "num_ref_idx_l0_default_active_minus1");
    WRITE_UVLC(pps->getNumRefIdxL1DefaultActive() - 1,     "num_ref_idx_l1_default_active_minus1");

    WRITE_SVLC(pps->getPicInitQPMinus26(),                  "init_qp_minus26");
    WRITE_FLAG(pps->getConstrainedIntraPred() ? 1 : 0,      "constrained_intra_pred_flag");
    WRITE_FLAG(pps->getUseTransformSkip() ? 1 : 0,  "transform_skip_enabled_flag");
    WRITE_FLAG(pps->getUseDQP() ? 1 : 0, "cu_qp_delta_enabled_flag");
    if (pps->getUseDQP())
    {
        WRITE_UVLC(pps->getMaxCuDQPDepth(), "diff_cu_qp_delta_depth");
    }
    WRITE_SVLC(pps->getChromaCbQpOffset(),                   "pps_cb_qp_offset");
    WRITE_SVLC(pps->getChromaCrQpOffset(),                   "pps_cr_qp_offset");
    WRITE_FLAG(pps->getSliceChromaQpFlag() ? 1 : 0,          "pps_slice_chroma_qp_offsets_present_flag");

    WRITE_FLAG(pps->getUseWP() ? 1 : 0,  "weighted_pred_flag");   // Use of Weighting Prediction (P_SLICE)
    WRITE_FLAG(pps->getWPBiPred() ? 1 : 0, "weighted_bipred_flag");  // Use of Weighting Bi-Prediction (B_SLICE)
    WRITE_FLAG(pps->getTransquantBypassEnableFlag() ? 1 : 0, "transquant_bypass_enable_flag");
    WRITE_FLAG(0,                                                 "tiles_enabled_flag");
    WRITE_FLAG(pps->getEntropyCodingSyncEnabledFlag() ? 1 : 0, "entropy_coding_sync_enabled_flag");
    WRITE_FLAG(1,                                                            "loop_filter_across_slices_enabled_flag");
    // TODO: Here have some time sequence problem, we set below field in initEncSlice(), but use them in getStreamHeaders() early
    WRITE_FLAG(pps->getDeblockingFilterControlPresentFlag() ? 1 : 0,       "deblocking_filter_control_present_flag");
    if (pps->getDeblockingFilterControlPresentFlag())
    {
        WRITE_FLAG(pps->getDeblockingFilterOverrideEnabledFlag() ? 1 : 0,  "deblocking_filter_override_enabled_flag");
        WRITE_FLAG(pps->getPicDisableDeblockingFilterFlag() ? 1 : 0,       "pps_disable_deblocking_filter_flag");
        if (!pps->getPicDisableDeblockingFilterFlag())
        {
            WRITE_SVLC(pps->getDeblockingFilterBetaOffsetDiv2(),             "pps_beta_offset_div2");
            WRITE_SVLC(pps->getDeblockingFilterTcOffsetDiv2(),               "pps_tc_offset_div2");
        }
    }
    WRITE_FLAG(pps->getScalingListPresentFlag() ? 1 : 0,                          "pps_scaling_list_data_present_flag");
    if (pps->getScalingListPresentFlag())
    {
#if SCALING_LIST_OUTPUT_RESULT
        printf("PPS\n");
#endif
        codeScalingList(m_slice->getScalingList());
    }
    WRITE_FLAG(pps->getListsModificationPresentFlag(), "lists_modification_present_flag");
    WRITE_UVLC(pps->getLog2ParallelMergeLevelMinus2(), "log2_parallel_merge_level_minus2");
    WRITE_FLAG(pps->getSliceHeaderExtensionPresentFlag() ? 1 : 0, "slice_segment_header_extension_present_flag");
    WRITE_FLAG(0, "pps_extension_flag");
}

void TEncCavlc::codeVUI(TComVUI *vui, TComSPS* sps)
{
#if ENC_DEC_TRACE
    fprintf(g_hTrace, "----------- vui_parameters -----------\n");
#endif
    WRITE_FLAG(vui->getAspectRatioInfoPresentFlag(),            "aspect_ratio_info_present_flag");
    if (vui->getAspectRatioInfoPresentFlag())
    {
        WRITE_CODE(vui->getAspectRatioIdc(), 8,                   "aspect_ratio_idc");
        if (vui->getAspectRatioIdc() == 255)
        {
            WRITE_CODE(vui->getSarWidth(), 16,                      "sar_width");
            WRITE_CODE(vui->getSarHeight(), 16,                     "sar_height");
        }
    }
    WRITE_FLAG(vui->getOverscanInfoPresentFlag(),               "overscan_info_present_flag");
    if (vui->getOverscanInfoPresentFlag())
    {
        WRITE_FLAG(vui->getOverscanAppropriateFlag(),             "overscan_appropriate_flag");
    }
    WRITE_FLAG(vui->getVideoSignalTypePresentFlag(),            "video_signal_type_present_flag");
    if (vui->getVideoSignalTypePresentFlag())
    {
        WRITE_CODE(vui->getVideoFormat(), 3,                      "video_format");
        WRITE_FLAG(vui->getVideoFullRangeFlag(),                  "video_full_range_flag");
        WRITE_FLAG(vui->getColourDescriptionPresentFlag(),        "colour_description_present_flag");
        if (vui->getColourDescriptionPresentFlag())
        {
            WRITE_CODE(vui->getColourPrimaries(), 8,                "colour_primaries");
            WRITE_CODE(vui->getTransferCharacteristics(), 8,        "transfer_characteristics");
            WRITE_CODE(vui->getMatrixCoefficients(), 8,             "matrix_coefficients");
        }
    }

    WRITE_FLAG(vui->getChromaLocInfoPresentFlag(),              "chroma_loc_info_present_flag");
    if (vui->getChromaLocInfoPresentFlag())
    {
        WRITE_UVLC(vui->getChromaSampleLocTypeTopField(),         "chroma_sample_loc_type_top_field");
        WRITE_UVLC(vui->getChromaSampleLocTypeBottomField(),      "chroma_sample_loc_type_bottom_field");
    }

    WRITE_FLAG(vui->getNeutralChromaIndicationFlag(),           "neutral_chroma_indication_flag");
    WRITE_FLAG(vui->getFieldSeqFlag(),                          "field_seq_flag");
    WRITE_FLAG(vui->getFrameFieldInfoPresentFlag(),             "frame_field_info_present_flag");

    Window defaultDisplayWindow = vui->getDefaultDisplayWindow();
    WRITE_FLAG(defaultDisplayWindow.getWindowEnabledFlag(),       "default_display_window_flag");
    if (defaultDisplayWindow.getWindowEnabledFlag())
    {
        WRITE_UVLC(defaultDisplayWindow.getWindowLeftOffset(),      "def_disp_win_left_offset");
        WRITE_UVLC(defaultDisplayWindow.getWindowRightOffset(),     "def_disp_win_right_offset");
        WRITE_UVLC(defaultDisplayWindow.getWindowTopOffset(),       "def_disp_win_top_offset");
        WRITE_UVLC(defaultDisplayWindow.getWindowBottomOffset(),    "def_disp_win_bottom_offset");
    }
    TimingInfo *timingInfo = vui->getTimingInfo();
    WRITE_FLAG(timingInfo->getTimingInfoPresentFlag(),          "vui_timing_info_present_flag");
    if (timingInfo->getTimingInfoPresentFlag())
    {
        WRITE_CODE(timingInfo->getNumUnitsInTick(), 32,           "vui_num_units_in_tick");
        WRITE_CODE(timingInfo->getTimeScale(),      32,           "vui_time_scale");
        WRITE_FLAG(timingInfo->getPocProportionalToTimingFlag(),  "vui_poc_proportional_to_timing_flag");
        if (timingInfo->getPocProportionalToTimingFlag())
        {
            WRITE_UVLC(timingInfo->getNumTicksPocDiffOneMinus1(),   "vui_num_ticks_poc_diff_one_minus1");
        }
        WRITE_FLAG(vui->getHrdParametersPresentFlag(),              "hrd_parameters_present_flag");
        if (vui->getHrdParametersPresentFlag())
        {
            codeHrdParameters(vui->getHrdParameters(), 1, sps->getMaxTLayers() - 1);
        }
    }
    WRITE_FLAG(vui->getBitstreamRestrictionFlag(),              "bitstream_restriction_flag");
    if (vui->getBitstreamRestrictionFlag())
    {
        WRITE_FLAG(0,                                                "tiles_fixed_structure_flag");
        WRITE_FLAG(vui->getMotionVectorsOverPicBoundariesFlag(),  "motion_vectors_over_pic_boundaries_flag");
        WRITE_FLAG(vui->getRestrictedRefPicListsFlag(),           "restricted_ref_pic_lists_flag");
        WRITE_UVLC(vui->getMinSpatialSegmentationIdc(),           "min_spatial_segmentation_idc");
        WRITE_UVLC(vui->getMaxBytesPerPicDenom(),                 "max_bytes_per_pic_denom");
        WRITE_UVLC(vui->getMaxBitsPerMinCuDenom(),                "max_bits_per_mincu_denom");
        WRITE_UVLC(vui->getLog2MaxMvLengthHorizontal(),           "log2_max_mv_length_horizontal");
        WRITE_UVLC(vui->getLog2MaxMvLengthVertical(),             "log2_max_mv_length_vertical");
    }
}

void TEncCavlc::codeHrdParameters(TComHRD *hrd, bool commonInfPresentFlag, UInt maxNumSubLayersMinus1)
{
    if (commonInfPresentFlag)
    {
        WRITE_FLAG(hrd->getNalHrdParametersPresentFlag() ? 1 : 0,  "nal_hrd_parameters_present_flag");
        WRITE_FLAG(hrd->getVclHrdParametersPresentFlag() ? 1 : 0,  "vcl_hrd_parameters_present_flag");
        if (hrd->getNalHrdParametersPresentFlag() || hrd->getVclHrdParametersPresentFlag())
        {
            WRITE_FLAG(hrd->getSubPicCpbParamsPresentFlag() ? 1 : 0,  "sub_pic_cpb_params_present_flag");
            if (hrd->getSubPicCpbParamsPresentFlag())
            {
                WRITE_CODE(hrd->getTickDivisorMinus2(), 8,              "tick_divisor_minus2");
                WRITE_CODE(hrd->getDuCpbRemovalDelayLengthMinus1(), 5,  "du_cpb_removal_delay_length_minus1");
                WRITE_FLAG(hrd->getSubPicCpbParamsInPicTimingSEIFlag() ? 1 : 0, "sub_pic_cpb_params_in_pic_timing_sei_flag");
                WRITE_CODE(hrd->getDpbOutputDelayDuLengthMinus1(), 5,   "dpb_output_delay_du_length_minus1");
            }
            WRITE_CODE(hrd->getBitRateScale(), 4,                     "bit_rate_scale");
            WRITE_CODE(hrd->getCpbSizeScale(), 4,                     "cpb_size_scale");
            if (hrd->getSubPicCpbParamsPresentFlag())
            {
                WRITE_CODE(hrd->getDuCpbSizeScale(), 4,                "du_cpb_size_scale");
            }
            WRITE_CODE(hrd->getInitialCpbRemovalDelayLengthMinus1(), 5, "initial_cpb_removal_delay_length_minus1");
            WRITE_CODE(hrd->getCpbRemovalDelayLengthMinus1(),        5, "au_cpb_removal_delay_length_minus1");
            WRITE_CODE(hrd->getDpbOutputDelayLengthMinus1(),         5, "dpb_output_delay_length_minus1");
        }
    }
    int i, j, nalOrVcl;
    for (i = 0; i <= maxNumSubLayersMinus1; i++)
    {
        WRITE_FLAG(hrd->getFixedPicRateFlag(i) ? 1 : 0,          "fixed_pic_rate_general_flag");
        if (!hrd->getFixedPicRateFlag(i))
        {
            WRITE_FLAG(hrd->getFixedPicRateWithinCvsFlag(i) ? 1 : 0, "fixed_pic_rate_within_cvs_flag");
        }
        else
        {
            hrd->setFixedPicRateWithinCvsFlag(i, true);
        }
        if (hrd->getFixedPicRateWithinCvsFlag(i))
        {
            WRITE_UVLC(hrd->getPicDurationInTcMinus1(i),           "elemental_duration_in_tc_minus1");
        }
        else
        {
            WRITE_FLAG(hrd->getLowDelayHrdFlag(i) ? 1 : 0,           "low_delay_hrd_flag");
        }
        if (!hrd->getLowDelayHrdFlag(i))
        {
            WRITE_UVLC(hrd->getCpbCntMinus1(i),                      "cpb_cnt_minus1");
        }

        for (nalOrVcl = 0; nalOrVcl < 2; nalOrVcl++)
        {
            if (((nalOrVcl == 0) && (hrd->getNalHrdParametersPresentFlag())) ||
                ((nalOrVcl == 1) && (hrd->getVclHrdParametersPresentFlag())))
            {
                for (j = 0; j <= (hrd->getCpbCntMinus1(i)); j++)
                {
                    WRITE_UVLC(hrd->getBitRateValueMinus1(i, j, nalOrVcl), "bit_rate_value_minus1");
                    WRITE_UVLC(hrd->getCpbSizeValueMinus1(i, j, nalOrVcl), "cpb_size_value_minus1");
                    if (hrd->getSubPicCpbParamsPresentFlag())
                    {
                        WRITE_UVLC(hrd->getDuCpbSizeValueMinus1(i, j, nalOrVcl), "cpb_size_du_value_minus1");
                        WRITE_UVLC(hrd->getDuBitRateValueMinus1(i, j, nalOrVcl), "bit_rate_du_value_minus1");
                    }
                    WRITE_FLAG(hrd->getCbrFlag(i, j, nalOrVcl) ? 1 : 0, "cbr_flag");
                }
            }
        }
    }
}

void TEncCavlc::codeSPS(TComSPS* sps)
{
#if ENC_DEC_TRACE
    xTraceSPSHeader(sps);
#endif
    WRITE_CODE(sps->getVPSId(),          4,       "sps_video_parameter_set_id");
    WRITE_CODE(sps->getMaxTLayers() - 1,  3,       "sps_max_sub_layers_minus1");
    WRITE_FLAG(sps->getTemporalIdNestingFlag() ? 1 : 0,                             "sps_temporal_id_nesting_flag");
    codePTL(sps->getPTL(), 1, sps->getMaxTLayers() - 1);
    WRITE_UVLC(sps->getSPSId(),                   "sps_seq_parameter_set_id");
    WRITE_UVLC(sps->getChromaFormatIdc(),         "chroma_format_idc");
    assert(sps->getChromaFormatIdc() == 1);
    // in the first version chroma_format_idc can only be equal to 1 (4:2:0)
    if (sps->getChromaFormatIdc() == 3)
    {
        WRITE_FLAG(0,                                  "separate_colour_plane_flag");
    }

    WRITE_UVLC(sps->getPicWidthInLumaSamples(),   "pic_width_in_luma_samples");
    WRITE_UVLC(sps->getPicHeightInLumaSamples(),   "pic_height_in_luma_samples");
    Window conf = sps->getConformanceWindow();

    WRITE_FLAG(conf.getWindowEnabledFlag(),          "conformance_window_flag");
    if (conf.getWindowEnabledFlag())
    {
        WRITE_UVLC(conf.getWindowLeftOffset()   / TComSPS::getWinUnitX(sps->getChromaFormatIdc()), "conf_win_left_offset");
        WRITE_UVLC(conf.getWindowRightOffset()  / TComSPS::getWinUnitX(sps->getChromaFormatIdc()), "conf_win_right_offset");
        WRITE_UVLC(conf.getWindowTopOffset()    / TComSPS::getWinUnitY(sps->getChromaFormatIdc()), "conf_win_top_offset");
        WRITE_UVLC(conf.getWindowBottomOffset() / TComSPS::getWinUnitY(sps->getChromaFormatIdc()), "conf_win_bottom_offset");
    }

    WRITE_UVLC(sps->getBitDepthY() - 8,             "bit_depth_luma_minus8");
    WRITE_UVLC(sps->getBitDepthC() - 8,             "bit_depth_chroma_minus8");

    WRITE_UVLC(sps->getBitsForPOC() - 4,                 "log2_max_pic_order_cnt_lsb_minus4");

    const bool subLayerOrderingInfoPresentFlag = 1;
    WRITE_FLAG(subLayerOrderingInfoPresentFlag,       "sps_sub_layer_ordering_info_present_flag");
    for (UInt i = 0; i <= sps->getMaxTLayers() - 1; i++)
    {
        WRITE_UVLC(sps->getMaxDecPicBuffering(i) - 1,       "sps_max_dec_pic_buffering_minus1[i]");
        WRITE_UVLC(sps->getNumReorderPics(i),               "sps_num_reorder_pics[i]");
        WRITE_UVLC(sps->getMaxLatencyIncrease(i),           "sps_max_latency_increase_plus1[i]");
        if (!subLayerOrderingInfoPresentFlag)
        {
            break;
        }
    }

    assert(sps->getMaxCUWidth() == sps->getMaxCUHeight());

    WRITE_UVLC(sps->getLog2MinCodingBlockSize() - 3,                                "log2_min_coding_block_size_minus3");
    WRITE_UVLC(sps->getLog2DiffMaxMinCodingBlockSize(),                             "log2_diff_max_min_coding_block_size");
    WRITE_UVLC(sps->getQuadtreeTULog2MinSize() - 2,                                 "log2_min_transform_block_size_minus2");
    WRITE_UVLC(sps->getQuadtreeTULog2MaxSize() - sps->getQuadtreeTULog2MinSize(), "log2_diff_max_min_transform_block_size");
    WRITE_UVLC(sps->getQuadtreeTUMaxDepthInter() - 1,                               "max_transform_hierarchy_depth_inter");
    WRITE_UVLC(sps->getQuadtreeTUMaxDepthIntra() - 1,                               "max_transform_hierarchy_depth_intra");
    WRITE_FLAG(sps->getScalingListFlag() ? 1 : 0,                                   "scaling_list_enabled_flag");
    if (sps->getScalingListFlag())
    {
        WRITE_FLAG(sps->getScalingListPresentFlag() ? 1 : 0,                          "sps_scaling_list_data_present_flag");
        if (sps->getScalingListPresentFlag())
        {
#if SCALING_LIST_OUTPUT_RESULT
            printf("SPS\n");
#endif
            codeScalingList(m_slice->getScalingList());
        }
    }
    WRITE_FLAG(sps->getUseAMP() ? 1 : 0,                                            "amp_enabled_flag");
    WRITE_FLAG(sps->getUseSAO() ? 1 : 0,                                            "sample_adaptive_offset_enabled_flag");

    WRITE_FLAG(sps->getUsePCM() ? 1 : 0,                                            "pcm_enabled_flag");
    if (sps->getUsePCM())
    {
        WRITE_CODE(sps->getPCMBitDepthLuma() - 1, 4,                                  "pcm_sample_bit_depth_luma_minus1");
        WRITE_CODE(sps->getPCMBitDepthChroma() - 1, 4,                                "pcm_sample_bit_depth_chroma_minus1");
        WRITE_UVLC(sps->getPCMLog2MinSize() - 3,                                      "log2_min_pcm_luma_coding_block_size_minus3");
        WRITE_UVLC(sps->getPCMLog2MaxSize() - sps->getPCMLog2MinSize(),             "log2_diff_max_min_pcm_luma_coding_block_size");
        WRITE_FLAG(sps->getPCMFilterDisableFlag() ? 1 : 0,                              "pcm_loop_filter_disable_flag");
    }

    assert(sps->getMaxTLayers() > 0);

    TComRPSList* rpsList = sps->getRPSList();
    TComReferencePictureSet* rps;

    WRITE_UVLC(rpsList->getNumberOfReferencePictureSets(), "num_short_term_ref_pic_sets");
    for (int i = 0; i < rpsList->getNumberOfReferencePictureSets(); i++)
    {
        rps = rpsList->getReferencePictureSet(i);
        codeShortTermRefPicSet(rps, false, i);
    }

    WRITE_FLAG(sps->getLongTermRefsPresent() ? 1 : 0,         "long_term_ref_pics_present_flag");
    if (sps->getLongTermRefsPresent())
    {
        WRITE_UVLC(sps->getNumLongTermRefPicSPS(), "num_long_term_ref_pic_sps");
        for (UInt k = 0; k < sps->getNumLongTermRefPicSPS(); k++)
        {
            WRITE_CODE(sps->getLtRefPicPocLsbSps(k), sps->getBitsForPOC(), "lt_ref_pic_poc_lsb_sps");
            WRITE_FLAG(sps->getUsedByCurrPicLtSPSFlag(k), "used_by_curr_pic_lt_sps_flag");
        }
    }
    WRITE_FLAG(sps->getTMVPFlagsPresent()  ? 1 : 0,           "sps_temporal_mvp_enable_flag");

    WRITE_FLAG(sps->getUseStrongIntraSmoothing(),             "sps_strong_intra_smoothing_enable_flag");

    WRITE_FLAG(sps->getVuiParametersPresentFlag(),             "vui_parameters_present_flag");
    if (sps->getVuiParametersPresentFlag())
    {
        codeVUI(sps->getVuiParameters(), sps);
    }

    WRITE_FLAG(0, "sps_extension_flag");
}

void TEncCavlc::codeVPS(TComVPS* vps)
{
    WRITE_CODE(vps->getVPSId(),                    4,        "vps_video_parameter_set_id");
    WRITE_CODE(3,                                    2,        "vps_reserved_three_2bits");
    WRITE_CODE(0,                                    6,        "vps_reserved_zero_6bits");
    WRITE_CODE(vps->getMaxTLayers() - 1,           3,        "vps_max_sub_layers_minus1");
    WRITE_FLAG(vps->getTemporalNestingFlag(),                "vps_temporal_id_nesting_flag");
    assert(vps->getMaxTLayers() > 1 || vps->getTemporalNestingFlag());
    WRITE_CODE(0xffff,                              16,        "vps_reserved_ffff_16bits");
    codePTL(vps->getPTL(), true, vps->getMaxTLayers() - 1);
    const bool subLayerOrderingInfoPresentFlag = 1;
    WRITE_FLAG(subLayerOrderingInfoPresentFlag,              "vps_sub_layer_ordering_info_present_flag");
    for (UInt i = 0; i <= vps->getMaxTLayers() - 1; i++)
    {
        WRITE_UVLC(vps->getMaxDecPicBuffering(i) - 1,       "vps_max_dec_pic_buffering_minus1[i]");
        WRITE_UVLC(vps->getNumReorderPics(i),               "vps_num_reorder_pics[i]");
        WRITE_UVLC(vps->getMaxLatencyIncrease(i),           "vps_max_latency_increase_plus1[i]");
        if (!subLayerOrderingInfoPresentFlag)
        {
            break;
        }
    }

    assert(vps->getNumHrdParameters() <= MAX_VPS_NUM_HRD_PARAMETERS);
    assert(vps->getMaxNuhReservedZeroLayerId() < MAX_VPS_NUH_RESERVED_ZERO_LAYER_ID_PLUS1);
    WRITE_CODE(vps->getMaxNuhReservedZeroLayerId(), 6,     "vps_max_nuh_reserved_zero_layer_id");
    vps->setMaxOpSets(1);
    WRITE_UVLC(vps->getMaxOpSets() - 1,                    "vps_max_op_sets_minus1");
    for (UInt opsIdx = 1; opsIdx <= (vps->getMaxOpSets() - 1); opsIdx++)
    {
        // Operation point set
        for (UInt i = 0; i <= vps->getMaxNuhReservedZeroLayerId(); i++)
        {
            // Only applicable for version 1
            vps->setLayerIdIncludedFlag(true, opsIdx, i);
            WRITE_FLAG(vps->getLayerIdIncludedFlag(opsIdx, i) ? 1 : 0, "layer_id_included_flag[opsIdx][i]");
        }
    }

    TimingInfo *timingInfo = vps->getTimingInfo();
    WRITE_FLAG(timingInfo->getTimingInfoPresentFlag(),          "vps_timing_info_present_flag");
    if (timingInfo->getTimingInfoPresentFlag())
    {
        WRITE_CODE(timingInfo->getNumUnitsInTick(), 32,           "vps_num_units_in_tick");
        WRITE_CODE(timingInfo->getTimeScale(),      32,           "vps_time_scale");
        WRITE_FLAG(timingInfo->getPocProportionalToTimingFlag(),  "vps_poc_proportional_to_timing_flag");
        if (timingInfo->getPocProportionalToTimingFlag())
        {
            WRITE_UVLC(timingInfo->getNumTicksPocDiffOneMinus1(),   "vps_num_ticks_poc_diff_one_minus1");
        }
        vps->setNumHrdParameters(0);
        WRITE_UVLC(vps->getNumHrdParameters(),                 "vps_num_hrd_parameters");

        if (vps->getNumHrdParameters() > 0)
        {
            vps->createHrdParamBuffer();
        }
        for (UInt i = 0; i < vps->getNumHrdParameters(); i++)
        {
            // Only applicable for version 1
            vps->setHrdOpSetIdx(0, i);
            WRITE_UVLC(vps->getHrdOpSetIdx(i),                "hrd_op_set_idx");
            if (i > 0)
            {
                WRITE_FLAG(vps->getCprmsPresentFlag(i) ? 1 : 0, "cprms_present_flag[i]");
            }
            codeHrdParameters(vps->getHrdParameters(i), vps->getCprmsPresentFlag(i), vps->getMaxTLayers() - 1);
        }
    }
    WRITE_FLAG(0,                     "vps_extension_flag");

    //future extensions here..
}

void TEncCavlc::codeSliceHeader(TComSlice* slice)
{
#if ENC_DEC_TRACE
    xTraceSliceHeader(slice);
#endif

    //calculate number of bits required for slice address
    int maxSliceSegmentAddress = slice->getPic()->getNumCUsInFrame();
    int bitsSliceSegmentAddress = 0;
    while (maxSliceSegmentAddress > (1 << bitsSliceSegmentAddress))
    {
        bitsSliceSegmentAddress++;
    }

    //write slice address
    int sliceSegmentAddress = 0;

    WRITE_FLAG(sliceSegmentAddress == 0, "first_slice_segment_in_pic_flag");
    if (slice->getRapPicFlag())
    {
        WRITE_FLAG(0, "no_output_of_prior_pics_flag");
    }
    WRITE_UVLC(slice->getPPS()->getPPSId(), "slice_pic_parameter_set_id");
    slice->setDependentSliceSegmentFlag(!slice->isNextSlice());
    if (sliceSegmentAddress > 0)
    {
        WRITE_CODE(sliceSegmentAddress, bitsSliceSegmentAddress, "slice_segment_address");
    }
    if (!slice->getDependentSliceSegmentFlag())
    {
        for (int i = 0; i < slice->getPPS()->getNumExtraSliceHeaderBits(); i++)
        {
            assert(!!"slice_reserved_undetermined_flag[]");
            WRITE_FLAG(0, "slice_reserved_undetermined_flag[]");
        }

        WRITE_UVLC(slice->getSliceType(),       "slice_type");

        if (slice->getPPS()->getOutputFlagPresentFlag())
        {
            WRITE_FLAG(slice->getPicOutputFlag() ? 1 : 0, "pic_output_flag");
        }

        // in the first version chroma_format_idc is equal to one, thus colour_plane_id will not be present
        assert(slice->getSPS()->getChromaFormatIdc() == 1);
        // if( separate_colour_plane_flag  ==  1 )
        //   colour_plane_id                                      u(2)

        if (!slice->getIdrPicFlag())
        {
            int picOrderCntLSB = (slice->getPOC() - slice->getLastIDR() + (1 << slice->getSPS()->getBitsForPOC())) % (1 << slice->getSPS()->getBitsForPOC());
            WRITE_CODE(picOrderCntLSB, slice->getSPS()->getBitsForPOC(), "pic_order_cnt_lsb");
            TComReferencePictureSet* rps = slice->getRPS();

            // check for bitstream restriction stating that:
            // If the current picture is a BLA or CRA picture, the value of NumPocTotalCurr shall be equal to 0.
            // Ideally this process should not be repeated for each slice in a picture
            if (slice->isIRAP())
            {
                for (int picIdx = 0; picIdx < rps->getNumberOfPictures(); picIdx++)
                {
                    assert(!rps->getUsed(picIdx));
                }
            }

            if (slice->getRPSidx() < 0)
            {
                WRITE_FLAG(0, "short_term_ref_pic_set_sps_flag");
                codeShortTermRefPicSet(rps, true, slice->getSPS()->getRPSList()->getNumberOfReferencePictureSets());
            }
            else
            {
                WRITE_FLAG(1, "short_term_ref_pic_set_sps_flag");
                int numBits = 0;
                while ((1 << numBits) < slice->getSPS()->getRPSList()->getNumberOfReferencePictureSets())
                {
                    numBits++;
                }

                if (numBits > 0)
                {
                    WRITE_CODE(slice->getRPSidx(), numBits, "short_term_ref_pic_set_idx");
                }
            }
            if (slice->getSPS()->getLongTermRefsPresent())
            {
                int numLtrpInSH = rps->getNumberOfLongtermPictures();
                int ltrpInSPS[MAX_NUM_REF_PICS];
                int numLtrpInSPS = 0;
                UInt ltrpIndex;
                int counter = 0;
                for (int k = rps->getNumberOfPictures() - 1; k > rps->getNumberOfPictures() - rps->getNumberOfLongtermPictures() - 1; k--)
                {
                    if (findMatchingLTRP(slice, &ltrpIndex, rps->getPOC(k), rps->getUsed(k)))
                    {
                        ltrpInSPS[numLtrpInSPS] = ltrpIndex;
                        numLtrpInSPS++;
                    }
                    else
                    {
                        counter++;
                    }
                }

                numLtrpInSH -= numLtrpInSPS;

                int bitsForLtrpInSPS = 0;
                while (slice->getSPS()->getNumLongTermRefPicSPS() > (1 << bitsForLtrpInSPS))
                {
                    bitsForLtrpInSPS++;
                }

                if (slice->getSPS()->getNumLongTermRefPicSPS() > 0)
                {
                    WRITE_UVLC(numLtrpInSPS, "num_long_term_sps");
                }
                WRITE_UVLC(numLtrpInSH, "num_long_term_pics");
                // Note that the LSBs of the LT ref. pic. POCs must be sorted before.
                // Not sorted here because LT ref indices will be used in setRefPicList()
                int prevDeltaMSB = 0, prevLSB = 0;
                int offset = rps->getNumberOfNegativePictures() + rps->getNumberOfPositivePictures();
                for (int i = rps->getNumberOfPictures() - 1; i > offset - 1; i--)
                {
                    if (counter < numLtrpInSPS)
                    {
                        if (bitsForLtrpInSPS > 0)
                        {
                            WRITE_CODE(ltrpInSPS[counter], bitsForLtrpInSPS, "lt_idx_sps[i]");
                        }
                    }
                    else
                    {
                        WRITE_CODE(rps->getPocLSBLT(i), slice->getSPS()->getBitsForPOC(), "poc_lsb_lt");
                        WRITE_FLAG(rps->getUsed(i), "used_by_curr_pic_lt_flag");
                    }
                    WRITE_FLAG(rps->getDeltaPocMSBPresentFlag(i), "delta_poc_msb_present_flag");

                    if (rps->getDeltaPocMSBPresentFlag(i))
                    {
                        bool deltaFlag = false;
                        //  First LTRP from SPS                 ||  First LTRP from SH                              || curr LSB            != prev LSB
                        if ((i == rps->getNumberOfPictures() - 1) || (i == rps->getNumberOfPictures() - 1 - numLtrpInSPS) || (rps->getPocLSBLT(i) != prevLSB))
                        {
                            deltaFlag = true;
                        }
                        if (deltaFlag)
                        {
                            WRITE_UVLC(rps->getDeltaPocMSBCycleLT(i), "delta_poc_msb_cycle_lt[i]");
                        }
                        else
                        {
                            int differenceInDeltaMSB = rps->getDeltaPocMSBCycleLT(i) - prevDeltaMSB;
                            assert(differenceInDeltaMSB >= 0);
                            WRITE_UVLC(differenceInDeltaMSB, "delta_poc_msb_cycle_lt[i]");
                        }
                        prevLSB = rps->getPocLSBLT(i);
                        prevDeltaMSB = rps->getDeltaPocMSBCycleLT(i);
                    }
                }
            }
            if (slice->getSPS()->getTMVPFlagsPresent())
            {
                WRITE_FLAG(slice->getEnableTMVPFlag() ? 1 : 0, "slice_temporal_mvp_enable_flag");
            }
        }
        if (slice->getSPS()->getUseSAO())
        {
            if (slice->getSPS()->getUseSAO())
            {
                WRITE_FLAG(slice->getSaoEnabledFlag(), "slice_sao_luma_flag");
                {
                    SAOParam *saoParam = slice->getPic()->getPicSym()->getSaoParam();
                    WRITE_FLAG(saoParam->bSaoFlag[1], "slice_sao_chroma_flag");
                }
            }
        }

        //check if numrefidxes match the defaults. If not, override

        if (!slice->isIntra())
        {
            bool overrideFlag = (slice->getNumRefIdx(REF_PIC_LIST_0) != slice->getPPS()->getNumRefIdxL0DefaultActive() || (slice->isInterB() && slice->getNumRefIdx(REF_PIC_LIST_1) != slice->getPPS()->getNumRefIdxL1DefaultActive()));
            WRITE_FLAG(overrideFlag ? 1 : 0,                               "num_ref_idx_active_override_flag");
            if (overrideFlag)
            {
                WRITE_UVLC(slice->getNumRefIdx(REF_PIC_LIST_0) - 1,      "num_ref_idx_l0_active_minus1");
                if (slice->isInterB())
                {
                    WRITE_UVLC(slice->getNumRefIdx(REF_PIC_LIST_1) - 1,    "num_ref_idx_l1_active_minus1");
                }
                else
                {
                    slice->setNumRefIdx(REF_PIC_LIST_1, 0);
                }
            }
        }
        else
        {
            slice->setNumRefIdx(REF_PIC_LIST_0, 0);
            slice->setNumRefIdx(REF_PIC_LIST_1, 0);
        }

        if (slice->getPPS()->getListsModificationPresentFlag() && slice->getNumRpsCurrTempList() > 1)
        {
            TComRefPicListModification* refPicListModification = slice->getRefPicListModification();
            if (!slice->isIntra())
            {
                WRITE_FLAG(slice->getRefPicListModification()->getRefPicListModificationFlagL0() ? 1 : 0,       "ref_pic_list_modification_flag_l0");
                if (slice->getRefPicListModification()->getRefPicListModificationFlagL0())
                {
                    int numRpsCurrTempList0 = slice->getNumRpsCurrTempList();
                    if (numRpsCurrTempList0 > 1)
                    {
                        int length = 1;
                        numRpsCurrTempList0--;
                        while (numRpsCurrTempList0 >>= 1)
                        {
                            length++;
                        }

                        for (int i = 0; i < slice->getNumRefIdx(REF_PIC_LIST_0); i++)
                        {
                            WRITE_CODE(refPicListModification->getRefPicSetIdxL0(i), length, "list_entry_l0");
                        }
                    }
                }
            }
            if (slice->isInterB())
            {
                WRITE_FLAG(slice->getRefPicListModification()->getRefPicListModificationFlagL1() ? 1 : 0,       "ref_pic_list_modification_flag_l1");
                if (slice->getRefPicListModification()->getRefPicListModificationFlagL1())
                {
                    int numRpsCurrTempList1 = slice->getNumRpsCurrTempList();
                    if (numRpsCurrTempList1 > 1)
                    {
                        int length = 1;
                        numRpsCurrTempList1--;
                        while (numRpsCurrTempList1 >>= 1)
                        {
                            length++;
                        }

                        for (int i = 0; i < slice->getNumRefIdx(REF_PIC_LIST_1); i++)
                        {
                            WRITE_CODE(refPicListModification->getRefPicSetIdxL1(i), length, "list_entry_l1");
                        }
                    }
                }
            }
        }

        if (slice->isInterB())
        {
            WRITE_FLAG(slice->getMvdL1ZeroFlag() ? 1 : 0,   "mvd_l1_zero_flag");
        }

        if (!slice->isIntra())
        {
            if (!slice->isIntra() && slice->getPPS()->getCabacInitPresentFlag())
            {
                SliceType sliceType   = slice->getSliceType();
                int  encCABACTableIdx = slice->getPPS()->getEncCABACTableIdx();
                bool encCabacInitFlag = (sliceType != encCABACTableIdx && encCABACTableIdx != I_SLICE) ? true : false;
                slice->setCabacInitFlag(encCabacInitFlag);
                WRITE_FLAG(encCabacInitFlag ? 1 : 0, "cabac_init_flag");
            }
        }

        if (slice->getEnableTMVPFlag())
        {
            if (slice->getSliceType() == B_SLICE)
            {
                WRITE_FLAG(slice->getColFromL0Flag(), "collocated_from_l0_flag");
            }

            if (slice->getSliceType() != I_SLICE &&
                ((slice->getColFromL0Flag() == 1 && slice->getNumRefIdx(REF_PIC_LIST_0) > 1) ||
                 (slice->getColFromL0Flag() == 0  && slice->getNumRefIdx(REF_PIC_LIST_1) > 1)))
            {
                WRITE_UVLC(slice->getColRefIdx(), "collocated_ref_idx");
            }
        }
        if ((slice->getPPS()->getUseWP() && slice->getSliceType() == P_SLICE) || (slice->getPPS()->getWPBiPred() && slice->getSliceType() == B_SLICE))
        {
            xCodePredWeightTable(slice);
        }
        assert(slice->getMaxNumMergeCand() <= MRG_MAX_NUM_CANDS);
        if (!slice->isIntra())
        {
            WRITE_UVLC(MRG_MAX_NUM_CANDS - slice->getMaxNumMergeCand(), "five_minus_max_num_merge_cand");
        }
        int iCode = slice->getSliceQp() - (slice->getPPS()->getPicInitQPMinus26() + 26);
        WRITE_SVLC(iCode, "slice_qp_delta");
        if (slice->getPPS()->getSliceChromaQpFlag())
        {
            iCode = slice->getSliceQpDeltaCb();
            WRITE_SVLC(iCode, "slice_qp_delta_cb");
            iCode = slice->getSliceQpDeltaCr();
            WRITE_SVLC(iCode, "slice_qp_delta_cr");
        }
        if (slice->getPPS()->getDeblockingFilterControlPresentFlag())
        {
            if (slice->getPPS()->getDeblockingFilterOverrideEnabledFlag())
            {
                WRITE_FLAG(slice->getDeblockingFilterOverrideFlag(), "deblocking_filter_override_flag");
            }
            if (slice->getDeblockingFilterOverrideFlag())
            {
                WRITE_FLAG(slice->getDeblockingFilterDisable(), "slice_disable_deblocking_filter_flag");
                if (!slice->getDeblockingFilterDisable())
                {
                    WRITE_SVLC(slice->getDeblockingFilterBetaOffsetDiv2(), "slice_beta_offset_div2");
                    WRITE_SVLC(slice->getDeblockingFilterTcOffsetDiv2(),   "slice_tc_offset_div2");
                }
            }
        }

        bool isSAOEnabled = (!slice->getSPS()->getUseSAO()) ? (false) : (slice->getSaoEnabledFlag() || slice->getSaoEnabledFlagChroma());
        bool isDBFEnabled = (!slice->getDeblockingFilterDisable());

        if (isSAOEnabled || isDBFEnabled)
        {
            WRITE_FLAG(1, "slice_loop_filter_across_slices_enabled_flag");
        }
    }
    if (slice->getPPS()->getSliceHeaderExtensionPresentFlag())
    {
        WRITE_UVLC(0, "slice_header_extension_length");
    }
}

void TEncCavlc::codePTL(TComPTL* ptl, bool profilePresentFlag, int maxNumSubLayersMinus1)
{
    if (profilePresentFlag)
    {
        codeProfileTier(ptl->getGeneralPTL()); // general_...
    }
    WRITE_CODE(ptl->getGeneralPTL()->getLevelIdc(), 8, "general_level_idc");

    for (int i = 0; i < maxNumSubLayersMinus1; i++)
    {
        if (profilePresentFlag)
        {
            WRITE_FLAG(ptl->getSubLayerProfilePresentFlag(i), "sub_layer_profile_present_flag[i]");
        }

        WRITE_FLAG(ptl->getSubLayerLevelPresentFlag(i),   "sub_layer_level_present_flag[i]");
    }

    if (maxNumSubLayersMinus1 > 0)
    {
        for (int i = maxNumSubLayersMinus1; i < 8; i++)
        {
            WRITE_CODE(0, 2, "reserved_zero_2bits");
        }
    }

    for (int i = 0; i < maxNumSubLayersMinus1; i++)
    {
        if (profilePresentFlag && ptl->getSubLayerProfilePresentFlag(i))
        {
            codeProfileTier(ptl->getSubLayerPTL(i)); // sub_layer_...
        }
        if (ptl->getSubLayerLevelPresentFlag(i))
        {
            WRITE_CODE(ptl->getSubLayerPTL(i)->getLevelIdc(), 8, "sub_layer_level_idc[i]");
        }
    }
}

void TEncCavlc::codeProfileTier(ProfileTierLevel* ptl)
{
    WRITE_CODE(ptl->getProfileSpace(), 2,     "XXX_profile_space[]");
    WRITE_FLAG(ptl->getTierFlag(),         "XXX_tier_flag[]");
    WRITE_CODE(ptl->getProfileIdc(), 5,     "XXX_profile_idc[]");
    for (int j = 0; j < 32; j++)
    {
        WRITE_FLAG(ptl->getProfileCompatibilityFlag(j), "XXX_profile_compatibility_flag[][j]");
    }

    WRITE_FLAG(ptl->getProgressiveSourceFlag(),   "general_progressive_source_flag");
    WRITE_FLAG(ptl->getInterlacedSourceFlag(),    "general_interlaced_source_flag");
    WRITE_FLAG(ptl->getNonPackedConstraintFlag(), "general_non_packed_constraint_flag");
    WRITE_FLAG(ptl->getFrameOnlyConstraintFlag(), "general_frame_only_constraint_flag");

    WRITE_CODE(0, 16, "XXX_reserved_zero_44bits[0..15]");
    WRITE_CODE(0, 16, "XXX_reserved_zero_44bits[16..31]");
    WRITE_CODE(0, 12, "XXX_reserved_zero_44bits[32..43]");
}

/**
 - write wavefront substreams sizes for the slice header.
 .
 \param slice Where we find the substream size information.
 */
void  TEncCavlc::codeTilesWPPEntryPoint(TComSlice* slice)
{
    if (!slice->getPPS()->getEntropyCodingSyncEnabledFlag())
    {
        return;
    }
    UInt numEntryPointOffsets = 0, offsetLenMinus1 = 0, maxOffset = 0;
    UInt *entryPointOffset = NULL;
    if (slice->getPPS()->getEntropyCodingSyncEnabledFlag())
    {
        UInt* substreamSizes              = slice->getSubstreamSizes();
        int maxNumParts                   = slice->getPic()->getNumPartInCU();
        int numZeroSubstreamsAtEndOfSlice = slice->getPic()->getFrameHeightInCU() - 1 - ((slice->getSliceCurEndCUAddr() - 1) / maxNumParts / slice->getPic()->getFrameWidthInCU());
        numEntryPointOffsets              = slice->getPic()->getFrameHeightInCU() - numZeroSubstreamsAtEndOfSlice - 1;
        slice->setNumEntryPointOffsets(numEntryPointOffsets);
        entryPointOffset = new UInt[numEntryPointOffsets];
        for (int idx = 0; idx < numEntryPointOffsets; idx++)
        {
            entryPointOffset[idx] = (substreamSizes[idx] >> 3);
            if (entryPointOffset[idx] > maxOffset)
            {
                maxOffset = entryPointOffset[idx];
            }
        }
    }
    // Determine number of bits "offsetLenMinus1+1" required for entry point information
    offsetLenMinus1 = 0;
    while (maxOffset >= (1u << (offsetLenMinus1 + 1)))
    {
        offsetLenMinus1++;
        assert(offsetLenMinus1 + 1 < 32);
    }

    WRITE_UVLC(numEntryPointOffsets, "num_entry_point_offsets");
    if (numEntryPointOffsets > 0)
    {
        WRITE_UVLC(offsetLenMinus1, "offset_len_minus1");
    }

    for (UInt idx = 0; idx < numEntryPointOffsets; idx++)
    {
        WRITE_CODE(entryPointOffset[idx] - 1, offsetLenMinus1 + 1, "entry_point_offset_minus1");
    }

    delete [] entryPointOffset;
}

void TEncCavlc::codeTerminatingBit(UInt)
{}

void TEncCavlc::codeSliceFinish()
{}

void TEncCavlc::codeMVPIdx(TComDataCU*, UInt, RefPicList)
{
    assert(0);
}

void TEncCavlc::codePartSize(TComDataCU*, UInt, UInt)
{
    assert(0);
}

void TEncCavlc::codePredMode(TComDataCU*, UInt)
{
    assert(0);
}

void TEncCavlc::codeMergeFlag(TComDataCU*, UInt)
{
    assert(0);
}

void TEncCavlc::codeMergeIndex(TComDataCU*, UInt)
{
    assert(0);
}

void TEncCavlc::codeInterModeFlag(TComDataCU*, UInt, UInt, UInt)
{
    assert(0);
}

void TEncCavlc::codeCUTransquantBypassFlag(TComDataCU*, UInt)
{
    assert(0);
}

void TEncCavlc::codeSkipFlag(TComDataCU*, UInt)
{
    assert(0);
}

void TEncCavlc::codeSplitFlag(TComDataCU*, UInt, UInt)
{
    assert(0);
}

void TEncCavlc::codeTransformSubdivFlag(UInt, UInt)
{
    assert(0);
}

void TEncCavlc::codeQtCbf(TComDataCU*, UInt, TextType, UInt)
{
    assert(0);
}

void TEncCavlc::codeQtRootCbf(TComDataCU*, UInt)
{
    assert(0);
}

void TEncCavlc::codeQtCbfZero(TComDataCU*, TextType, UInt)
{
    assert(0);
}

void TEncCavlc::codeQtRootCbfZero(TComDataCU*)
{
    assert(0);
}

void TEncCavlc::codeTransformSkipFlags(TComDataCU*, UInt, UInt, UInt, TextType)
{
    assert(0);
}

void TEncCavlc::codeIPCMInfo(TComDataCU*, UInt)
{
    assert(0);
}

void TEncCavlc::codeIntraDirLumaAng(TComDataCU*, UInt, bool)
{
    assert(0);
}

void TEncCavlc::codeIntraDirChroma(TComDataCU*, UInt)
{
    assert(0);
}

void TEncCavlc::codeInterDir(TComDataCU*, UInt)
{
    assert(0);
}

void TEncCavlc::codeRefFrmIdx(TComDataCU*, UInt, RefPicList)
{
    assert(0);
}

void TEncCavlc::codeMvd(TComDataCU*, UInt, RefPicList)
{
    assert(0);
}

void TEncCavlc::codeDeltaQP(TComDataCU* cu, UInt absPartIdx)
{
    int dqp  = cu->getQP(absPartIdx) - cu->getRefQP(absPartIdx);

    int qpBdOffsetY =  cu->getSlice()->getSPS()->getQpBDOffsetY();

    dqp = (dqp + 78 + qpBdOffsetY + (qpBdOffsetY / 2)) % (52 + qpBdOffsetY) - 26 - (qpBdOffsetY / 2);

    xWriteSvlc(dqp);
}

void TEncCavlc::codeCoeffNxN(TComDataCU*, TCoeff*, UInt, UInt, UInt, UInt, TextType)
{
    assert(0);
}

void TEncCavlc::estBit(estBitsSbacStruct*, int, int, TextType)
{
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

/** code explicit wp tables
 * \param TComSlice* slice
 * \returns void
 */
void TEncCavlc::xCodePredWeightTable(TComSlice* slice)
{
    wpScalingParam  *wp;
    bool            bChroma      = true; // color always present in HEVC ?
    int             iNbRef       = (slice->getSliceType() == B_SLICE) ? (2) : (1);
    bool            bDenomCoded  = false;
    UInt            mode = 0;
    UInt            totalSignalledWeightFlags = 0;

    if ((slice->getSliceType() == P_SLICE && slice->getPPS()->getUseWP()) || (slice->getSliceType() == B_SLICE && slice->getPPS()->getWPBiPred()))
    {
        mode = 1; // explicit
    }
    if (mode == 1)
    {
        for (int iNumRef = 0; iNumRef < iNbRef; iNumRef++)
        {
            RefPicList  picList = (iNumRef ? REF_PIC_LIST_1 : REF_PIC_LIST_0);

            for (int refIdx = 0; refIdx < slice->getNumRefIdx(picList); refIdx++)
            {
                slice->getWpScaling(picList, refIdx, wp);
                if (!bDenomCoded)
                {
                    int iDeltaDenom;
                    WRITE_UVLC(wp[0].log2WeightDenom, "luma_log2_weight_denom"); // ue(v): luma_log2_weight_denom

                    if (bChroma)
                    {
                        iDeltaDenom = (wp[1].log2WeightDenom - wp[0].log2WeightDenom);
                        WRITE_SVLC(iDeltaDenom, "delta_chroma_log2_weight_denom"); // se(v): delta_chroma_log2_weight_denom
                    }
                    bDenomCoded = true;
                }
                WRITE_FLAG(wp[0].bPresentFlag, "luma_weight_lX_flag");         // u(1): luma_weight_lX_flag
                totalSignalledWeightFlags += wp[0].bPresentFlag;
            }

            if (bChroma)
            {
                for (int refIdx = 0; refIdx < slice->getNumRefIdx(picList); refIdx++)
                {
                    slice->getWpScaling(picList, refIdx, wp);
                    WRITE_FLAG(wp[1].bPresentFlag, "chroma_weight_lX_flag");   // u(1): chroma_weight_lX_flag
                    totalSignalledWeightFlags += 2 * wp[1].bPresentFlag;
                }
            }

            for (int refIdx = 0; refIdx < slice->getNumRefIdx(picList); refIdx++)
            {
                slice->getWpScaling(picList, refIdx, wp);
                if (wp[0].bPresentFlag)
                {
                    int iDeltaWeight = (wp[0].inputWeight - (1 << wp[0].log2WeightDenom));
                    WRITE_SVLC(iDeltaWeight, "delta_luma_weight_lX");          // se(v): delta_luma_weight_lX
                    WRITE_SVLC(wp[0].inputOffset, "luma_offset_lX");               // se(v): luma_offset_lX
                }

                if (bChroma)
                {
                    if (wp[1].bPresentFlag)
                    {
                        for (int j = 1; j < 3; j++)
                        {
                            int iDeltaWeight = (wp[j].inputWeight - (1 << wp[1].log2WeightDenom));
                            WRITE_SVLC(iDeltaWeight, "delta_chroma_weight_lX"); // se(v): delta_chroma_weight_lX

                            int pred = (128 - ((128 * wp[j].inputWeight) >> (wp[j].log2WeightDenom)));
                            int iDeltaChroma = (wp[j].inputOffset - pred);
                            WRITE_SVLC(iDeltaChroma, "delta_chroma_offset_lX"); // se(v): delta_chroma_offset_lX
                        }
                    }
                }
            }
        }

        assert(totalSignalledWeightFlags <= 24);
    }
}

/** code quantization matrix
 *  \param scalingList quantization matrix information
 */
void TEncCavlc::codeScalingList(TComScalingList* scalingList)
{
    UInt listId, sizeId;
    bool scalingListPredModeFlag;

#if SCALING_LIST_OUTPUT_RESULT
    int startBit;
    int startTotalBit;
    startBit = m_bitIf->getNumberOfWrittenBits();
    startTotalBit = m_bitIf->getNumberOfWrittenBits();
#endif

    //for each size
    for (sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
    {
        for (listId = 0; listId < g_scalingListNum[sizeId]; listId++)
        {
#if SCALING_LIST_OUTPUT_RESULT
            startBit = m_bitIf->getNumberOfWrittenBits();
#endif
            scalingListPredModeFlag = scalingList->checkPredMode(sizeId, listId);
            WRITE_FLAG(scalingListPredModeFlag, "scaling_list_pred_mode_flag");
            if (!scalingListPredModeFlag) // Copy Mode
            {
                WRITE_UVLC((int)listId - (int)scalingList->getRefMatrixId(sizeId, listId), "scaling_list_pred_matrix_id_delta");
            }
            else // DPCM Mode
            {
                xCodeScalingList(scalingList, sizeId, listId);
            }
#if SCALING_LIST_OUTPUT_RESULT
            printf("Matrix [%d][%d] Bit %d\n", sizeId, listId, m_bitIf->getNumberOfWrittenBits() - startBit);
#endif
        }
    }

#if SCALING_LIST_OUTPUT_RESULT
    printf("Total Bit %d\n", m_bitIf->getNumberOfWrittenBits() - startTotalBit);
#endif
}

/** code DPCM
 * \param scalingList quantization matrix information
 * \param sizeIdc size index
 * \param listIdc list index
 */
void TEncCavlc::xCodeScalingList(TComScalingList* scalingList, UInt sizeId, UInt listId)
{
    int coefNum = X265_MIN(MAX_MATRIX_COEF_NUM, (int)g_scalingListSize[sizeId]);
    UInt* scan  = (sizeId == 0) ? g_sigLastScan[SCAN_DIAG][1] :  g_sigLastScanCG32x32;
    int nextCoef = SCALING_LIST_START_VALUE;
    int data;
    int *src = scalingList->getScalingListAddress(sizeId, listId);

    if (sizeId > SCALING_LIST_8x8)
    {
        WRITE_SVLC(scalingList->getScalingListDC(sizeId, listId) - 8, "scaling_list_dc_coef_minus8");
        nextCoef = scalingList->getScalingListDC(sizeId, listId);
    }
    for (int i = 0; i < coefNum; i++)
    {
        data = src[scan[i]] - nextCoef;
        nextCoef = src[scan[i]];
        if (data > 127)
        {
            data = data - 256;
        }
        if (data < -128)
        {
            data = data + 256;
        }

        WRITE_SVLC(data,  "scaling_list_delta_coef");
    }
}

bool TEncCavlc::findMatchingLTRP(TComSlice* slice, UInt *ltrpsIndex, int ltrpPOC, bool usedFlag)
{
    // bool state = true, state2 = false;
    int lsb = ltrpPOC % (1 << slice->getSPS()->getBitsForPOC());

    for (int k = 0; k < slice->getSPS()->getNumLongTermRefPicSPS(); k++)
    {
        if ((lsb == slice->getSPS()->getLtRefPicPocLsbSps(k)) && (usedFlag == slice->getSPS()->getUsedByCurrPicLtSPSFlag(k)))
        {
            *ltrpsIndex = k;
            return true;
        }
    }

    return false;
}

bool TComScalingList::checkPredMode(UInt sizeId, UInt listId)
{
    for (int predListIdx = (int)listId; predListIdx >= 0; predListIdx--)
    {
        if (!memcmp(getScalingListAddress(sizeId, listId),
                    ((listId == predListIdx) ? getScalingListDefaultAddress(sizeId, predListIdx) : getScalingListAddress(sizeId, predListIdx)),
                    sizeof(int) * X265_MIN(MAX_MATRIX_COEF_NUM, (int)g_scalingListSize[sizeId])) // check value of matrix
            && ((sizeId < SCALING_LIST_16x16) || (getScalingListDC(sizeId, listId) == getScalingListDC(sizeId, predListIdx)))) // check DC value
        {
            setRefMatrixId(sizeId, listId, predListIdx);
            return false;
        }
    }

    return true;
}

//! \}
