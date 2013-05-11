/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *
 * This program is also available under a commercial proprietary license.
 * For more information, contact us at licensing@multicorewareinc.com.
 *****************************************************************************/

#ifndef _X265_H_
#define _X265_H_

#include <stdint.h>

#if __cplusplus
extern "C" {
#endif

/* Public C interface to x265 encoder (incomplete) */

typedef struct
{
    void *planes[3];

    int   stride[3];

    int   bitDepth;
}
x265_picture;

/***
 * Pass cpuid 0 to auto-detect.  If not called, first encoder allocated will
 * auto-detect the CPU and initialize performance primitives */
void x265_init_primitives( int cpuid );

/***
 * Call to explicitly set the number of threads x265 allocates for its thread pool.
 * The thread pool is a singleton resource for the process and the first time an
 * encoder is created it will allocate a default thread pool if necessary.  Default
 * is one thread per CPU core (counting hyper-threading). */
void x265_init_threading( int threadcount );

/***
 * Release library static allocations
 */
void x265_cleanup(void);

///TODO: this list needs to be pruned substantially to just the settings we want
//       end users to have control over.
typedef struct
{
    // coding tools (bit-depth)
    int       m_internalBitDepth;                 ///< bit-depth codec operates at

    // source specification
    int       m_iFrameRate;                       ///< source frame-rates (Hz)
    int       m_iSourceWidth;                     ///< source width in pixel
    int       m_iSourceHeight;                    ///< source height in pixel
    int       m_maxTempLayer;                     ///< Max temporal layer

    // coding unit (CU) definition
    uint32_t  m_uiMaxCUWidth;                     ///< max. CU width in pixel
    uint32_t  m_uiMaxCUHeight;                    ///< max. CU height in pixel
    uint32_t  m_uiMaxCUDepth;                     ///< max. CU depth

    // transform unit (TU) definition
    uint32_t  m_uiQuadtreeTULog2MaxSize;
    uint32_t  m_uiQuadtreeTULog2MinSize;

    uint32_t  m_uiQuadtreeTUMaxDepthInter;
    uint32_t  m_uiQuadtreeTUMaxDepthIntra;

#if 0 // These Enums must be made public and C style
    // profile/level
    Profile::Name m_profile;
    Level::Tier   m_levelTier;
    Level::Name   m_level;
#endif

    int       m_progressiveSourceFlag;
    int       m_interlacedSourceFlag;
    int       m_nonPackedConstraintFlag;
    int       m_frameOnlyConstraintFlag;

    // coding structure
    int       m_iIntraPeriod;                     ///< period of I-slice (random access period)
    int       m_iDecodingRefreshType;             ///< random access type
    int       m_iGOPSize;                         ///< GOP size of hierarchical structure
    int       m_extraRPSs;                        ///< extra RPSs added to handle CRA

#if 0 // MAX_GOP and MAX_TLAYER must be made public, or remove these
    GOPEntry  m_GOPList[MAX_GOP];                 ///< the coding structure entries from the config file
    Int       m_numReorderPics[MAX_TLAYER];       ///< total number of reorder pictures
    Int       m_maxDecPicBuffering[MAX_TLAYER];   ///< total number of pictures in the decoded picture buffer
#endif

    int       m_useTransformSkip;                 ///< flag for enabling intra transform skipping
    int       m_useTransformSkipFast;             ///< flag for enabling fast intra transform skipping
    int       m_enableAMP;
    int       m_enableAMPRefine;

    // coding quality
    int       m_iQP;                              ///< QP value of key-picture (integer)
    int       m_iMaxDeltaQP;                      ///< max. |delta QP|
    uint32_t  m_uiDeltaQpRD;                      ///< dQP range for multi-pass slice QP optimization
    int       m_iMaxCuDQPDepth;                   ///< Max. depth for a minimum CuDQPSize (0:default)

    int       m_cbQpOffset;                       ///< Chroma Cb QP Offset (0:default)
    int       m_crQpOffset;                       ///< Chroma Cr QP Offset (0:default)

    int       m_bUseAdaptQpSelect;

    int       m_bUseAdaptiveQP;                   ///< Flag for enabling QP adaptation based on a psycho-visual model
    int       m_iQPAdaptationRange;               ///< dQP range by QP adaptation

    // coding tools (PCM bit-depth)
    int       m_bPCMInputBitDepthFlag;            ///< 0: PCM bit-depth is internal bit-depth. 1: PCM bit-depth is input bit-depth.

    // coding tool (lossless)
    int       m_useLossless;                      ///< flag for using lossless coding
    int       m_bUseSAO;
    int       m_maxNumOffsetsPerPic;              ///< SAO maximun number of offset per picture
    int       m_saoLcuBoundary;                   ///< SAO parameter estimation using non-deblocked pixels for LCU bottom and right boundary areas
    int       m_saoLcuBasedOptimization;          ///< SAO LCU-based optimization

    // coding tools (loop filter)
    int       m_bLoopFilterDisable;               ///< flag for using deblocking filter
    int       m_loopFilterOffsetInPPS;            ///< offset for deblocking filter in 0 = slice header, 1 = PPS
    int       m_loopFilterBetaOffsetDiv2;         ///< beta offset for deblocking filter
    int       m_loopFilterTcOffsetDiv2;           ///< tc offset for deblocking filter
    int       m_DeblockingFilterControlPresent;   ///< deblocking filter control present flag in PPS
    int       m_DeblockingFilterMetric;           ///< blockiness metric in encoder

    // coding tools (PCM)
    int       m_usePCM;                           ///< flag for using IPCM
    uint32_t  m_pcmLog2MaxSize;                   ///< log2 of maximum PCM block size
    uint32_t  m_uiPCMLog2MinSize;                 ///< log2 of minimum PCM block size
    int       m_bPCMFilterDisableFlag;            ///< PCM filter disable flag

    // coding tools
    int       m_bUseSBACRD;                       ///< flag for using RD optimization based on SBAC
    int       m_bUseASR;                          ///< flag for using adaptive motion search range
    int       m_bUseHADME;                        ///< flag for using HAD in sub-pel ME
    int       m_useRDOQ;                          ///< flag for using RD optimized quantization
    int       m_useRDOQTS;                        ///< flag for using RD optimized quantization for transform skip
    int       m_rdPenalty;                        ///< RD-penalty for 32x32 TU for intra in non-intra slices (0: no RD-penalty, 1: RD-penalty, 2: maximum RD-penalty)
    int       m_iFastSearch;                      ///< ME mode, 0 = full, 1 = diamond, 2 = PMVFAST
    int       m_iSearchRange;                     ///< ME search range
    int       m_bipredSearchRange;                ///< ME search range for bipred refinement
    int       m_bUseFastEnc;                      ///< flag for using fast encoder setting
    int       m_bUseEarlyCU;                      ///< flag for using Early CU setting
    int       m_useFastDecisionForMerge;          ///< flag for using Fast Decision Merge RD-Cost
    int       m_bUseCbfFastMode;                  ///< flag for using Cbf Fast PU Mode Decision
    int       m_useEarlySkipDetection;            ///< flag for using Early SKIP Detection

    int       m_bLFCrossTileBoundaryFlag;         ///< 1: filter across tile boundaries  0: do not filter across tile boundaries
    int       m_iUniformSpacingIdr;
    int       m_iWaveFrontSynchro;                ///< 0: no WPP. >= 1: WPP is enabled, the "Top right" from which inheritance occurs is this LCU offset in the line above the current.
    int       m_iWaveFrontSubstreams;             ///< If iWaveFrontSynchro, this is the number of substreams per frame (dependent tiles) or per tile (independent tiles).

    int       m_bUseConstrainedIntraPred;         ///< flag for using constrained intra prediction

    int       m_decodedPictureHashSEIEnabled;     ///< Checksum(3)/CRC(2)/MD5(1)/disable(0) acting on decoded picture hash SEI message
    int       m_recoveryPointSEIEnabled;
    int       m_bufferingPeriodSEIEnabled;
    int       m_pictureTimingSEIEnabled;

    // Tone Mapping SEI
    int       m_toneMappingInfoSEIEnabled;
    int       m_toneMapId;
    int       m_toneMapCancelFlag;
    int       m_toneMapPersistenceFlag;
    int       m_toneMapCodedDataBitDepth;
    int       m_toneMapTargetBitDepth;
    int       m_toneMapModelId;
    int       m_toneMapMinValue;
    int       m_toneMapMaxValue;
    int       m_sigmoidMidpoint;
    int       m_sigmoidWidth;
    int       m_numPivots;
    int       m_cameraIsoSpeedIdc;
    int       m_cameraIsoSpeedValue;
    int       m_exposureCompensationValueSignFlag;
    int       m_exposureCompensationValueNumerator;
    int       m_exposureCompensationValueDenomIdc;
    int       m_refScreenLuminanceWhite;
    int       m_extendedRangeWhiteLevel;
    int       m_nominalBlackLevelLumaCodeValue;
    int       m_nominalWhiteLevelLumaCodeValue;
    int       m_extendedWhiteLevelLumaCodeValue;

    int       m_framePackingSEIEnabled;
    int       m_framePackingSEIType;
    int       m_framePackingSEIId;
    int       m_framePackingSEIQuincunx;
    int       m_framePackingSEIInterpretation;
    int       m_displayOrientationSEIAngle;
    int       m_temporalLevel0IndexSEIEnabled;
    int       m_gradualDecodingRefreshInfoEnabled;
    int       m_decodingUnitInfoSEIEnabled;
    int       m_SOPDescriptionSEIEnabled;
    int       m_scalableNestingSEIEnabled;

    // weighted prediction
    int       m_useWeightedPred;                  ///< Use of weighted prediction in P slices
    int       m_useWeightedBiPred;                ///< Use of bi-directional weighted prediction in B slices

    uint32_t  m_log2ParallelMergeLevel;           ///< Parallel merge estimation region
    uint32_t  m_maxNumMergeCand;                  ///< Max number of merge candidates

    int       m_TMVPModeId;
    int       m_signHideFlag;
    int       m_RCEnableRateControl;              ///< enable rate control or not
    int       m_RCTargetBitrate;                  ///< target bitrate when rate control is enabled
    int       m_RCKeepHierarchicalBit;            ///< whether keeping hierarchical bit allocation structure or not
    int       m_RCLCULevelRC;                     ///< true: LCU level rate control; false: picture level rate control
    int       m_RCUseLCUSeparateModel;            ///< use separate R-lambda model at LCU level
    int       m_RCInitialQP;                      ///< inital QP for rate control
    int       m_RCForceIntraQP;                   ///< force all intra picture to use initial QP or not

    int       m_TransquantBypassEnableFlag;       ///< transquant_bypass_enable_flag setting in PPS.
    int       m_CUTransquantBypassFlagValue;      ///< if transquant_bypass_enable_flag, the fixed value to use for the per-CU cu_transquant_bypass_flag.

    int       m_recalculateQPAccordingToLambda;   ///< recalculate QP value according to the lambda value
    int       m_useStrongIntraSmoothing;          ///< enable strong intra smoothing for 32x32 blocks where the reference samples are flat
    int       m_activeParameterSetsSEIEnabled;

    int       m_vuiParametersPresentFlag;         ///< enable generation of VUI parameters
    int       m_aspectRatioInfoPresentFlag;       ///< Signals whether aspect_ratio_idc is present
    int       m_aspectRatioIdc;                   ///< aspect_ratio_idc
    int       m_sarWidth;                         ///< horizontal size of the sample aspect ratio
    int       m_sarHeight;                        ///< vertical size of the sample aspect ratio
    int       m_overscanInfoPresentFlag;          ///< Signals whether overscan_appropriate_flag is present
    int       m_overscanAppropriateFlag;          ///< Indicates whether conformant decoded pictures are suitable for display using overscan
    int       m_videoSignalTypePresentFlag;       ///< Signals whether video_format, video_full_range_flag, and colour_description_present_flag are present
    int       m_videoFormat;                      ///< Indicates representation of pictures
    int       m_videoFullRangeFlag;               ///< Indicates the black level and range of luma and chroma signals
    int       m_colourDescriptionPresentFlag;     ///< Signals whether colour_primaries, transfer_characteristics and matrix_coefficients are present
    int       m_colourPrimaries;                  ///< Indicates chromaticity coordinates of the source primaries
    int       m_transferCharacteristics;          ///< Indicates the opto-electronic transfer characteristics of the source
    int       m_matrixCoefficients;               ///< Describes the matrix coefficients used in deriving luma and chroma from RGB primaries
    int       m_chromaLocInfoPresentFlag;         ///< Signals whether chroma_sample_loc_type_top_field and chroma_sample_loc_type_bottom_field are present
    int       m_chromaSampleLocTypeTopField;      ///< Specifies the location of chroma samples for top field
    int       m_chromaSampleLocTypeBottomField;   ///< Specifies the location of chroma samples for bottom field
    int       m_neutralChromaIndicationFlag;      ///< Indicates that the value of all decoded chroma samples is equal to 1<<(BitDepthCr-1)
    int       m_defaultDisplayWindowFlag;         ///< Indicates the presence of the default window parameters
    int       m_defDispWinLeftOffset;             ///< Specifies the left offset from the conformance window of the default window
    int       m_defDispWinRightOffset;            ///< Specifies the right offset from the conformance window of the default window
    int       m_defDispWinTopOffset;              ///< Specifies the top offset from the conformance window of the default window
    int       m_defDispWinBottomOffset;           ///< Specifies the bottom offset from the conformance window of the default window
    int       m_frameFieldInfoPresentFlag;        ///< Indicates that pic_struct values are present in picture timing SEI messages
    int       m_pocProportionalToTimingFlag;      ///< Indicates that the POC value is proportional to the output time w.r.t. first picture in CVS
    int       m_numTicksPocDiffOneMinus1;         ///< Number of ticks minus 1 that for a POC difference of one
    int       m_bitstreamRestrictionFlag;         ///< Signals whether bitstream restriction parameters are present
    int       m_tilesFixedStructureFlag;          ///< Indicates that each active picture parameter set has the same values of the syntax elements related to tiles
    int       m_motionVectorsOverPicBoundariesFlag;///< Indicates that no samples outside the picture boundaries are used for inter prediction
    int       m_minSpatialSegmentationIdc;        ///< Indicates the maximum size of the spatial segments in the pictures in the coded video sequence
    int       m_maxBytesPerPicDenom;              ///< Indicates a number of bytes not exceeded by the sum of the sizes of the VCL NAL units associated with any coded picture
    int       m_maxBitsPerMinCuDenom;             ///< Indicates an upper bound for the number of bits of coding_unit() data
    int       m_log2MaxMvLengthHorizontal;        ///< Indicate the maximum absolute value of a decoded horizontal MV component in quarter-pel luma units
    int       m_log2MaxMvLengthVertical;          ///< Indicate the maximum absolute value of a decoded vertical MV component in quarter-pel luma units
}
x265_params;

// TODO: Needs methods to initialize this structure to sane defaults, apply
// reasonable presets, and create an encoder with the given settings.

#if __cplusplus
};
#endif

#endif // _X265_H_
