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

/** \file     TEncCfg.h
    \brief    encoder configuration class (header)
*/

#ifndef X265_TENCCFG_H
#define X265_TENCCFG_H

#include "TLibCommon/CommonDef.h"
#include "TLibCommon/TComSlice.h"
#include <assert.h>

namespace x265 {
// private namespace

//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// encoder configuration class
class TEncCfg
{
protected:

    //==== File I/O ========
    int       m_conformanceMode;
    Window    m_defaultDisplayWindow;         ///< Represents the default display window parameters
    TComVPS   m_vps;

    /* profile & level */
    Profile::Name m_profile;
    Level::Tier   m_levelTier;
    Level::Name   m_level;

    bool      m_progressiveSourceFlag;
    bool      m_interlacedSourceFlag;
    bool      m_nonPackedConstraintFlag;
    bool      m_frameOnlyConstraintFlag;

    //====== Coding Structure ========
    int       m_maxDecPicBuffering[MAX_TLAYER];
    int       m_numReorderPics[MAX_TLAYER];
    int       m_maxRefPicNum;                   ///< this is used to mimic the sliding mechanism used by the decoder
                                                // TODO: We need to have a common sliding mechanism used by both the encoder and decoder

    //======= Transform =============
    uint32_t  m_quadtreeTULog2MaxSize;
    uint32_t  m_quadtreeTULog2MinSize;

    //====== Loop/Deblock Filter ========
    bool      m_loopFilterOffsetInPPS;
    int       m_loopFilterBetaOffsetDiv2;
    int       m_loopFilterTcOffsetDiv2;
    int       m_maxNumOffsetsPerPic;

    //====== Lossless ========
    bool      m_useLossless;

    //====== Quality control ========
    int       m_maxCuDQPDepth;                  //  Max. depth for a minimum CuDQP (0:default)

    //====== Tool list ========
    bool      m_bUseASR;
    bool      m_usePCM;
    uint32_t  m_pcmLog2MaxSize;
    uint32_t  m_pcmLog2MinSize;

    bool      m_bPCMInputBitDepthFlag;
    uint32_t  m_pcmBitDepthLuma;
    uint32_t  m_pcmBitDepthChroma;
    bool      m_bPCMFilterDisableFlag;
    bool      m_loopFilterAcrossTilesEnabledFlag;

    int       m_bufferingPeriodSEIEnabled;
    int       m_pictureTimingSEIEnabled;
    int       m_recoveryPointSEIEnabled;
    int       m_displayOrientationSEIAngle;
    int       m_gradualDecodingRefreshInfoEnabled;
    int       m_decodingUnitInfoSEIEnabled;

    int       m_csp;

    //====== Weighted Prediction ========

    uint32_t  m_log2ParallelMergeLevelMinus2;                 ///< Parallel merge estimation region

    int       m_useScalingListId;                             ///< Using quantization matrix i.e. 0=off, 1=default.

    bool      m_TransquantBypassEnableFlag;                   ///< transquant_bypass_enable_flag setting in PPS.
    bool      m_CUTransquantBypassFlagValue;                  ///< if transquant_bypass_enable_flag, the fixed value to use for the per-CU cu_transquant_bypass_flag.
    int       m_activeParameterSetsSEIEnabled;                ///< enable active parameter set SEI message
    bool      m_vuiParametersPresentFlag;                     ///< enable generation of VUI parameters
    bool      m_aspectRatioInfoPresentFlag;                   ///< Signals whether aspect_ratio_idc is present
    int       m_aspectRatioIdc;                               ///< aspect_ratio_idc
    int       m_sarWidth;                                     ///< horizontal size of the sample aspect ratio
    int       m_sarHeight;                                    ///< vertical size of the sample aspect ratio
    bool      m_overscanInfoPresentFlag;                      ///< Signals whether overscan_appropriate_flag is present
    bool      m_overscanAppropriateFlag;                      ///< Indicates whether conformant decoded pictures are suitable for display using overscan
    bool      m_videoSignalTypePresentFlag;                   ///< Signals whether video_format, video_full_range_flag, and colour_description_present_flag are present
    int       m_videoFormat;                                  ///< Indicates representation of pictures
    bool      m_videoFullRangeFlag;                           ///< Indicates the black level and range of luma and chroma signals
    bool      m_colourDescriptionPresentFlag;                 ///< Signals whether colour_primaries, transfer_characteristics and matrix_coefficients are present
    int       m_colourPrimaries;                              ///< Indicates chromaticity coordinates of the source primaries
    int       m_transferCharacteristics;                      ///< Indicates the opto-electronic transfer characteristics of the source
    int       m_matrixCoefficients;                           ///< Describes the matrix coefficients used in deriving luma and chroma from RGB primaries
    bool      m_chromaLocInfoPresentFlag;                     ///< Signals whether chroma_sample_loc_type_top_field and chroma_sample_loc_type_bottom_field are present
    int       m_chromaSampleLocTypeTopField;                  ///< Specifies the location of chroma samples for top field
    int       m_chromaSampleLocTypeBottomField;               ///< Specifies the location of chroma samples for bottom field
    bool      m_neutralChromaIndicationFlag;                  ///< Indicates that the value of all decoded chroma samples is equal to 1<<(BitDepthCr-1)
    bool      m_frameFieldInfoPresentFlag;                    ///< Indicates that pic_struct and other field coding related values are present in picture timing SEI messages
    bool      m_pocProportionalToTimingFlag;                  ///< Indicates that the POC value is proportional to the output time w.r.t. first picture in CVS
    int       m_numTicksPocDiffOneMinus1;                     ///< Number of ticks minus 1 that for a POC difference of one
    bool      m_bitstreamRestrictionFlag;                     ///< Signals whether bitstream restriction parameters are present
    bool      m_motionVectorsOverPicBoundariesFlag;           ///< Indicates that no samples outside the picture boundaries are used for inter prediction
    int       m_minSpatialSegmentationIdc;                    ///< Indicates the maximum size of the spatial segments in the pictures in the coded video sequence
    int       m_maxBytesPerPicDenom;                          ///< Indicates a number of bytes not exceeded by the sum of the sizes of the VCL NAL units associated with any coded picture
    int       m_maxBitsPerMinCuDenom;                         ///< Indicates an upper bound for the number of bits of coding_unit() data
    int       m_log2MaxMvLengthHorizontal;                    ///< Indicate the maximum absolute value of a decoded horizontal MV component in quarter-pel luma units
    int       m_log2MaxMvLengthVertical;                      ///< Indicate the maximum absolute value of a decoded vertical MV component in quarter-pel luma units

public:

    /* copy of parameters used to create encoder */
    x265_param param;

    int       m_pad[2];
    Window    m_conformanceWindow;

    TEncCfg()
    {}

    virtual ~TEncCfg()
    {}

    TComVPS *getVPS() { return &m_vps; }

    //====== Coding Structure ========

    int getMaxRefPicNum() { return m_maxRefPicNum; }

    //==== Coding Structure ========

    int getMaxDecPicBuffering(uint32_t tlayer) { return m_maxDecPicBuffering[tlayer]; }

    int getNumReorderPics(uint32_t tlayer) { return m_numReorderPics[tlayer]; }

    //======== Transform =============
    uint32_t getQuadtreeTULog2MaxSize() const { return m_quadtreeTULog2MaxSize; }

    uint32_t getQuadtreeTULog2MinSize() const { return m_quadtreeTULog2MinSize; }

    //==== Loop/Deblock Filter ========
    bool getLoopFilterOffsetInPPS() { return m_loopFilterOffsetInPPS; }

    int getLoopFilterBetaOffset() { return m_loopFilterBetaOffsetDiv2; }

    int getLoopFilterTcOffset() { return m_loopFilterTcOffsetDiv2; }

    //==== Quality control ========
    int getMaxCuDQPDepth() { return m_maxCuDQPDepth; }

    //====== Lossless ========
    bool getUseLossless() { return m_useLossless; }

    //==== Tool list ========
    bool getUseASR() { return m_bUseASR; }

    bool getPCMInputBitDepthFlag() { return m_bPCMInputBitDepthFlag; }

    bool getPCMFilterDisableFlag() { return m_bPCMFilterDisableFlag; }

    bool getUsePCM() { return m_usePCM; }

    uint32_t getPCMLog2MaxSize() { return m_pcmLog2MaxSize; }

    uint32_t getPCMLog2MinSize() { return m_pcmLog2MinSize; }

    int   getMaxNumOffsetsPerPic() { return m_maxNumOffsetsPerPic; }

    bool  getLFCrossTileBoundaryFlag() { return m_loopFilterAcrossTilesEnabledFlag; }

    int   getBufferingPeriodSEIEnabled() { return m_bufferingPeriodSEIEnabled; }

    int   getPictureTimingSEIEnabled() { return m_pictureTimingSEIEnabled; }

    int   getRecoveryPointSEIEnabled() { return m_recoveryPointSEIEnabled; }

    int   getDisplayOrientationSEIAngle() { return m_displayOrientationSEIAngle; }

    int   getGradualDecodingRefreshInfoEnabled() { return m_gradualDecodingRefreshInfoEnabled; }

    int   getDecodingUnitInfoSEIEnabled() { return m_decodingUnitInfoSEIEnabled; }

    uint32_t getLog2ParallelMergeLevelMinus2() { return m_log2ParallelMergeLevelMinus2; }

    int  getUseScalingListId() { return m_useScalingListId; }

    bool getTransquantBypassEnableFlag() { return m_TransquantBypassEnableFlag; }

    bool getCUTransquantBypassFlagValue() { return m_CUTransquantBypassFlagValue; }

    int getActiveParameterSetsSEIEnabled() { return m_activeParameterSetsSEIEnabled; }

    bool getVuiParametersPresentFlag() { return m_vuiParametersPresentFlag; }

    bool getAspectRatioInfoPresentFlag() { return m_aspectRatioInfoPresentFlag; }

    int getAspectRatioIdc() { return m_aspectRatioIdc; }

    int getSarWidth() { return m_sarWidth; }

    int getSarHeight() { return m_sarHeight; }

    bool getOverscanInfoPresentFlag() { return m_overscanInfoPresentFlag; }

    bool getOverscanAppropriateFlag() { return m_overscanAppropriateFlag; }

    bool getVideoSignalTypePresentFlag() { return m_videoSignalTypePresentFlag; }

    int getVideoFormat() { return m_videoFormat; }

    int getColorFormat() { return m_csp; }

    bool getVideoFullRangeFlag() { return m_videoFullRangeFlag; }

    bool getColourDescriptionPresentFlag() { return m_colourDescriptionPresentFlag; }

    int getColourPrimaries() { return m_colourPrimaries; }

    int getTransferCharacteristics() { return m_transferCharacteristics; }

    int getMatrixCoefficients() { return m_matrixCoefficients; }

    bool getChromaLocInfoPresentFlag() { return m_chromaLocInfoPresentFlag; }

    int getChromaSampleLocTypeTopField() { return m_chromaSampleLocTypeTopField; }

    int getChromaSampleLocTypeBottomField() { return m_chromaSampleLocTypeBottomField; }

    bool getNeutralChromaIndicationFlag() { return m_neutralChromaIndicationFlag; }

    Window &getDefaultDisplayWindow() { return m_defaultDisplayWindow; }

    bool getFrameFieldInfoPresentFlag() { return m_frameFieldInfoPresentFlag; }

    bool getPocProportionalToTimingFlag() { return m_pocProportionalToTimingFlag; }

    int getNumTicksPocDiffOneMinus1() { return m_numTicksPocDiffOneMinus1;    }

    bool getBitstreamRestrictionFlag() { return m_bitstreamRestrictionFlag; }

    bool getMotionVectorsOverPicBoundariesFlag() { return m_motionVectorsOverPicBoundariesFlag; }

    int getMinSpatialSegmentationIdc() { return m_minSpatialSegmentationIdc; }

    int getMaxBytesPerPicDenom() { return m_maxBytesPerPicDenom; }

    int getMaxBitsPerMinCuDenom() { return m_maxBitsPerMinCuDenom; }

    int getLog2MaxMvLengthHorizontal() { return m_log2MaxMvLengthHorizontal; }

    int getLog2MaxMvLengthVertical() { return m_log2MaxMvLengthVertical; }

    bool getProgressiveSourceFlag() const { return m_progressiveSourceFlag; }

    bool getInterlacedSourceFlag() const { return m_interlacedSourceFlag; }

    bool getNonPackedConstraintFlag() const { return m_nonPackedConstraintFlag; }

    bool getFrameOnlyConstraintFlag() const { return m_frameOnlyConstraintFlag; }
};
}
//! \}

#endif // ifndef X265_TENCCFG_H
