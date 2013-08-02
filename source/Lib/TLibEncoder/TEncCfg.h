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

#ifndef __TENCCFG__
#define __TENCCFG__

#include "TLibCommon/CommonDef.h"
#include "TLibCommon/TComSlice.h"
#include <assert.h>

struct GOPEntry
{
    Int m_POC;
    Int m_QPOffset;
    Double m_QPFactor;
    Int m_tcOffsetDiv2;
    Int m_betaOffsetDiv2;
    Bool m_refPic;
    Int m_numRefPicsActive;
    Char m_sliceType;
    Int m_numRefPics;
    Int m_referencePics[MAX_NUM_REF_PICS];
    Int m_usedByCurrPic[MAX_NUM_REF_PICS];
    Int m_interRPSPrediction;
    Int m_deltaRPS;
    Int m_numRefIdc;
    Int m_refIdc[MAX_NUM_REF_PICS + 1];
    GOPEntry()
        : m_POC(-1)
        , m_QPOffset(0)
        , m_QPFactor(0)
        , m_tcOffsetDiv2(0)
        , m_betaOffsetDiv2(0)
        , m_refPic(false)
        , m_numRefPicsActive(0)
        , m_sliceType('P')
        , m_numRefPics(0)
        , m_interRPSPrediction(false)
        , m_deltaRPS(0)
        , m_numRefIdc(0)
    {
        ::memset(m_referencePics, 0, sizeof(m_referencePics));
        ::memset(m_usedByCurrPic, 0, sizeof(m_usedByCurrPic));
        ::memset(m_refIdc,        0, sizeof(m_refIdc));
    }
};

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
    Int       m_conformanceMode;
    Window    m_conformanceWindow;
    Int       m_framesToBeEncoded;
    Double    m_adLambdaModifier[MAX_TLAYER];

    /* profile & level */
    Profile::Name m_profile;
    Level::Tier   m_levelTier;
    Level::Name   m_level;

    Bool m_progressiveSourceFlag;
    Bool m_interlacedSourceFlag;
    Bool m_nonPackedConstraintFlag;
    Bool m_frameOnlyConstraintFlag;

    //====== Coding Structure ========
    UInt      m_decodingRefreshType;          ///< the type of decoding refresh employed for the random access.
    Int       m_gopSize;
    GOPEntry  m_gopList[MAX_GOP];
    Int       m_extraRPSs;
    Int       m_maxDecPicBuffering[MAX_TLAYER];
    Int       m_numReorderPics[MAX_TLAYER];
    Int       m_qp;                            //  if (AdaptiveQP == OFF)

    Int       m_pad[2];

    Int       m_maxRefPicNum;                   ///< this is used to mimic the sliding mechanism used by the decoder
                                                 // TODO: We need to have a common sliding mechanism used by both the encoder and decoder

    Bool      m_useAMP;
    Bool      m_useRectInter;

    //======= Transform =============
    UInt      m_quadtreeTULog2MaxSize;
    UInt      m_quadtreeTULog2MinSize;
    UInt      m_quadtreeTUMaxDepthInter;
    UInt      m_quadtreeTUMaxDepthIntra;

    //====== Loop/Deblock Filter ========
    Bool      m_bLoopFilterDisable;
    Bool      m_loopFilterOffsetInPPS;
    Int       m_loopFilterBetaOffsetDiv2;
    Int       m_loopFilterTcOffsetDiv2;
    Bool      m_deblockingFilterControlPresent;
    Bool      m_bUseSAO;
    Int       m_maxNumOffsetsPerPic;
    Bool      m_saoLcuBoundary;
    Bool      m_saoLcuBasedOptimization;

    //====== Lossless ========
    Bool      m_useLossless;

    //====== Motion search ========
    Int       m_searchMethod;
    Int       m_searchRange;                   //  0:Full frame
    Int       m_bipredSearchRange;

    //====== Quality control ========
    Bool      m_useRDO;

    Int       m_maxCuDQPDepth;                  //  Max. depth for a minimum CuDQP (0:default)
    Int       m_chromaCbQpOffset;               //  Chroma Cb QP Offset (0:default)
    Int       m_chromaCrQpOffset;               //  Chroma Cr Qp Offset (0:default)

    //====== Tool list ========
    Bool      m_bUseASR;
    Bool      m_useRDOQ;
    Bool      m_useRDOQTS;
    UInt      m_rdPenalty;
    Bool      m_bUseCbfFastMode;
    Bool      m_useEarlySkipDetection;
    Bool      m_useTransformSkip;
    Bool      m_useTransformSkipFast;
    Int*      m_dqpTable;

    Bool      m_bUseConstrainedIntraPred;
    Bool      m_usePCM;
    UInt      m_pcmLog2MaxSize;
    UInt      m_pcmLog2MinSize;

    Bool      m_bPCMInputBitDepthFlag;
    UInt      m_pcmBitDepthLuma;
    UInt      m_pcmBitDepthChroma;
    Bool      m_bPCMFilterDisableFlag;
    Bool      m_loopFilterAcrossTilesEnabledFlag;

    Int       m_decodedPictureHashSEIEnabled; ///< Checksum(3)/CRC(2)/MD5(1)/disable(0) acting on decoded picture hash SEI message
    Int       m_bufferingPeriodSEIEnabled;
    Int       m_pictureTimingSEIEnabled;
    Int       m_recoveryPointSEIEnabled;
    Int       m_displayOrientationSEIAngle;
    Int       m_gradualDecodingRefreshInfoEnabled;
    Int       m_decodingUnitInfoSEIEnabled;
    Int       m_SOPDescriptionSEIEnabled;

    //====== Weighted Prediction ========
    Bool      m_useWeightedPred;     //< Use of Weighting Prediction (P_SLICE)
    Bool      m_useWeightedBiPred;   //< Use of Bi-directional Weighting Prediction (B_SLICE)
    UInt      m_log2ParallelMergeLevelMinus2;     ///< Parallel merge estimation region
    UInt      m_maxNumMergeCand;                  ///< Maximum number of merge candidates
    Int       m_useScalingListId;          ///< Using quantization matrix i.e. 0=off, 1=default, 2=file.
    Char*     m_scalingListFile;        ///< quantization matrix file name
    Int       m_TMVPModeId;
    Int       m_signHideFlag;

    Bool      m_TransquantBypassEnableFlag;                   ///< transquant_bypass_enable_flag setting in PPS.
    Bool      m_CUTransquantBypassFlagValue;                  ///< if transquant_bypass_enable_flag, the fixed value to use for the per-CU cu_transquant_bypass_flag.
    TComVPS   m_vps;
    Bool      m_recalculateQPAccordingToLambda;               ///< recalculate QP value according to the lambda value
    Int       m_activeParameterSetsSEIEnabled;                ///< enable active parameter set SEI message
    Bool      m_vuiParametersPresentFlag;                     ///< enable generation of VUI parameters
    Bool      m_aspectRatioInfoPresentFlag;                   ///< Signals whether aspect_ratio_idc is present
    Int       m_aspectRatioIdc;                               ///< aspect_ratio_idc
    Int       m_sarWidth;                                     ///< horizontal size of the sample aspect ratio
    Int       m_sarHeight;                                    ///< vertical size of the sample aspect ratio
    Bool      m_overscanInfoPresentFlag;                      ///< Signals whether overscan_appropriate_flag is present
    Bool      m_overscanAppropriateFlag;                      ///< Indicates whether conformant decoded pictures are suitable for display using overscan
    Bool      m_videoSignalTypePresentFlag;                   ///< Signals whether video_format, video_full_range_flag, and colour_description_present_flag are present
    Int       m_videoFormat;                                  ///< Indicates representation of pictures
    Bool      m_videoFullRangeFlag;                           ///< Indicates the black level and range of luma and chroma signals
    Bool      m_colourDescriptionPresentFlag;                 ///< Signals whether colour_primaries, transfer_characteristics and matrix_coefficients are present
    Int       m_colourPrimaries;                              ///< Indicates chromaticity coordinates of the source primaries
    Int       m_transferCharacteristics;                      ///< Indicates the opto-electronic transfer characteristics of the source
    Int       m_matrixCoefficients;                           ///< Describes the matrix coefficients used in deriving luma and chroma from RGB primaries
    Bool      m_chromaLocInfoPresentFlag;                     ///< Signals whether chroma_sample_loc_type_top_field and chroma_sample_loc_type_bottom_field are present
    Int       m_chromaSampleLocTypeTopField;                  ///< Specifies the location of chroma samples for top field
    Int       m_chromaSampleLocTypeBottomField;               ///< Specifies the location of chroma samples for bottom field
    Bool      m_neutralChromaIndicationFlag;                  ///< Indicates that the value of all decoded chroma samples is equal to 1<<(BitDepthCr-1)
    Window    m_defaultDisplayWindow;                         ///< Represents the default display window parameters
    Bool      m_frameFieldInfoPresentFlag;                    ///< Indicates that pic_struct and other field coding related values are present in picture timing SEI messages
    Bool      m_pocProportionalToTimingFlag;                  ///< Indicates that the POC value is proportional to the output time w.r.t. first picture in CVS
    Int       m_numTicksPocDiffOneMinus1;                     ///< Number of ticks minus 1 that for a POC difference of one
    Bool      m_bitstreamRestrictionFlag;                     ///< Signals whether bitstream restriction parameters are present
    Bool      m_motionVectorsOverPicBoundariesFlag;           ///< Indicates that no samples outside the picture boundaries are used for inter prediction
    Int       m_minSpatialSegmentationIdc;                    ///< Indicates the maximum size of the spatial segments in the pictures in the coded video sequence
    Int       m_maxBytesPerPicDenom;                          ///< Indicates a number of bytes not exceeded by the sum of the sizes of the VCL NAL units associated with any coded picture
    Int       m_maxBitsPerMinCuDenom;                         ///< Indicates an upper bound for the number of bits of coding_unit() data
    Int       m_log2MaxMvLengthHorizontal;                    ///< Indicate the maximum absolute value of a decoded horizontal MV component in quarter-pel luma units
    Int       m_log2MaxMvLengthVertical;                      ///< Indicate the maximum absolute value of a decoded vertical MV component in quarter-pel luma units

    Bool      m_useStrongIntraSmoothing;                      ///< enable the use of strong intra smoothing (bi_linear interpolation) for 32x32 blocks when reference samples are flat.

public:

    /* copy of parameters used to create encoder */
    x265_param_t param;

    TEncCfg()
    {}

    virtual ~TEncCfg()
    {}

    Window   &getConformanceWindow()  { return m_conformanceWindow; }

    Void      setPad(Int* iPad)       { for (Int i = 0; i < 2; i++) { m_pad[i] = iPad[i]; } }

    //====== Coding Structure ========

    Int       getExtraRPSs()          { return m_extraRPSs; }

    GOPEntry  getGOPEntry(Int i)      { return m_gopList[i]; }

    Int       getMaxRefPicNum()       { return m_maxRefPicNum;           }

    //======== Transform =============

    Bool      getUseRDO() const         { return m_useRDO; }

    //====== Sequence ========
    Int       getFramesToBeEncoded()    { return m_framesToBeEncoded; }

    Double    getLambdaModifier(UInt uiIndex) const { return m_adLambdaModifier[uiIndex]; }

    //==== Coding Structure ========
    UInt      getDecodingRefreshType()      { return m_decodingRefreshType; }

    Int       getGOPSize()      { return m_gopSize; }

    Int       getMaxDecPicBuffering(UInt tlayer) { return m_maxDecPicBuffering[tlayer]; }

    Int       getNumReorderPics(UInt tlayer) { return m_numReorderPics[tlayer]; }

    Int       getQP()      { return m_qp; }

    Int       getPad(Int i)      { assert(i < 2);                      return m_pad[i]; }

    //======== Transform =============
    UInt      getQuadtreeTULog2MaxSize()      const { return m_quadtreeTULog2MaxSize; }

    UInt      getQuadtreeTULog2MinSize()      const { return m_quadtreeTULog2MinSize; }

    UInt      getQuadtreeTUMaxDepthInter()      const { return m_quadtreeTUMaxDepthInter; }

    UInt      getQuadtreeTUMaxDepthIntra()      const { return m_quadtreeTUMaxDepthIntra; }

    //==== Loop/Deblock Filter ========
    Bool      getLoopFilterDisable()      { return m_bLoopFilterDisable;       }

    Bool      getLoopFilterOffsetInPPS()      { return m_loopFilterOffsetInPPS; }

    Int       getLoopFilterBetaOffset()      { return m_loopFilterBetaOffsetDiv2; }

    Int       getLoopFilterTcOffset()      { return m_loopFilterTcOffsetDiv2; }

    Bool      getDeblockingFilterControlPresent()  { return m_deblockingFilterControlPresent; }

    //==== Motion search ========
    Int       getSearchMethod()      { return m_searchMethod; }

    Int       getSearchRange()       { return m_searchRange; }

    Int       getBipredSearchRange() { return m_bipredSearchRange; }

    //==== Quality control ========
    Int       getMaxCuDQPDepth()      { return m_maxCuDQPDepth; }

    //====== Lossless ========
    Bool      getUseLossless()      { return m_useLossless;  }

    //==== Tool list ========
    Bool      getUseASR()      { return m_bUseASR;     }

    Bool      getUseRDOQ()       { return m_useRDOQ;    }

    Bool      getUseRDOQTS()      { return m_useRDOQTS;  }

    Int       getRDpenalty()      { return m_rdPenalty;  }

    Bool      getUseCbfFastMode()      { return m_bUseCbfFastMode; }

    Bool      getUseEarlySkipDetection()      { return m_useEarlySkipDetection; }

    Bool      getUseConstrainedIntraPred()      { return m_bUseConstrainedIntraPred; }

    Bool      getPCMInputBitDepthFlag()      { return m_bPCMInputBitDepthFlag;   }

    Bool      getPCMFilterDisableFlag()      { return m_bPCMFilterDisableFlag;   }

    Bool      getUsePCM()      { return m_usePCM;                 }

    UInt      getPCMLog2MaxSize()      { return m_pcmLog2MaxSize;  }

    UInt      getPCMLog2MinSize()      { return m_pcmLog2MinSize;  }

    Bool      getUseTransformSkip()      { return m_useTransformSkip;        }

    Bool      getUseTransformSkipFast()      { return m_useTransformSkipFast;    }

    Int*      getdQPs()      { return m_dqpTable;       }

    Bool      getUseSAO()              { return m_bUseSAO; }

    Int   getMaxNumOffsetsPerPic()                    { return m_maxNumOffsetsPerPic; }

    Bool  getSaoLcuBoundary()              { return m_saoLcuBoundary; }

    Bool  getSaoLcuBasedOptimization()                    { return m_saoLcuBasedOptimization; }

    Bool  getLFCrossTileBoundaryFlag()                    { return m_loopFilterAcrossTilesEnabledFlag;   }

    Int   getDecodedPictureHashSEIEnabled()                { return m_decodedPictureHashSEIEnabled; }

    Int   getBufferingPeriodSEIEnabled()                   { return m_bufferingPeriodSEIEnabled; }

    Int   getPictureTimingSEIEnabled()                     { return m_pictureTimingSEIEnabled; }

    Int   getRecoveryPointSEIEnabled()                     { return m_recoveryPointSEIEnabled; }

    Int   getDisplayOrientationSEIAngle()                  { return m_displayOrientationSEIAngle; }

    Int   getGradualDecodingRefreshInfoEnabled()           { return m_gradualDecodingRefreshInfoEnabled; }

    Int   getDecodingUnitInfoSEIEnabled()                     { return m_decodingUnitInfoSEIEnabled; }

    Int   getSOPDescriptionSEIEnabled()                     { return m_SOPDescriptionSEIEnabled; }

    Bool getUseWP()            { return m_useWeightedPred;   }

    Bool getWPBiPred()            { return m_useWeightedBiPred; }

    UInt getLog2ParallelMergeLevelMinus2()            { return m_log2ParallelMergeLevelMinus2;       }

    UInt getMaxNumMergeCand()            { return m_maxNumMergeCand;   }

    Int  getUseScalingListId()            { return m_useScalingListId;      }

    Char* getScalingListFile()            { return m_scalingListFile;    }

    Int   getTMVPModeId()         { return m_TMVPModeId; }

    Int   getSignHideFlag()                    { return m_signHideFlag; }


    Bool getTransquantBypassEnableFlag()           { return m_TransquantBypassEnableFlag; }

    Bool getCUTransquantBypassFlagValue()          { return m_CUTransquantBypassFlagValue; }

    TComVPS *getVPS() { return &m_vps; }

    Bool      getUseRecalculateQPAccordingToLambda() { return m_recalculateQPAccordingToLambda; }

    Bool      getUseStrongIntraSmoothing()         { return m_useStrongIntraSmoothing; }

    Int       getActiveParameterSetsSEIEnabled()         { return m_activeParameterSetsSEIEnabled; }

    Bool      getVuiParametersPresentFlag()                 { return m_vuiParametersPresentFlag; }

    Bool      getAspectRatioInfoPresentFlag()               { return m_aspectRatioInfoPresentFlag; }

    Int       getAspectRatioIdc()                           { return m_aspectRatioIdc; }

    Int       getSarWidth()                                 { return m_sarWidth; }

    Int       getSarHeight()                                { return m_sarHeight; }

    Bool      getOverscanInfoPresentFlag()                  { return m_overscanInfoPresentFlag; }

    Bool      getOverscanAppropriateFlag()                  { return m_overscanAppropriateFlag; }

    Bool      getVideoSignalTypePresentFlag()               { return m_videoSignalTypePresentFlag; }

    Int       getVideoFormat()                              { return m_videoFormat; }

    Bool      getVideoFullRangeFlag()                       { return m_videoFullRangeFlag; }

    Bool      getColourDescriptionPresentFlag()             { return m_colourDescriptionPresentFlag; }

    Int       getColourPrimaries()                          { return m_colourPrimaries; }

    Int       getTransferCharacteristics()                  { return m_transferCharacteristics; }

    Int       getMatrixCoefficients()                       { return m_matrixCoefficients; }

    Bool      getChromaLocInfoPresentFlag()                 { return m_chromaLocInfoPresentFlag; }

    Int       getChromaSampleLocTypeTopField()              { return m_chromaSampleLocTypeTopField; }

    Int       getChromaSampleLocTypeBottomField()           { return m_chromaSampleLocTypeBottomField; }

    Bool      getNeutralChromaIndicationFlag()              { return m_neutralChromaIndicationFlag; }

    Window   &getDefaultDisplayWindow()                     { return m_defaultDisplayWindow; }

    Bool      getFrameFieldInfoPresentFlag()                { return m_frameFieldInfoPresentFlag; }

    Bool      getPocProportionalToTimingFlag()              { return m_pocProportionalToTimingFlag; }

    Int       getNumTicksPocDiffOneMinus1()                 { return m_numTicksPocDiffOneMinus1;    }

    Bool      getBitstreamRestrictionFlag()                 { return m_bitstreamRestrictionFlag; }

    Bool      getMotionVectorsOverPicBoundariesFlag()       { return m_motionVectorsOverPicBoundariesFlag; }

    Int       getMinSpatialSegmentationIdc()                { return m_minSpatialSegmentationIdc; }

    Int       getMaxBytesPerPicDenom()                      { return m_maxBytesPerPicDenom; }

    Int       getMaxBitsPerMinCuDenom()                     { return m_maxBitsPerMinCuDenom; }

    Int       getLog2MaxMvLengthHorizontal()                { return m_log2MaxMvLengthHorizontal; }

    Int       getLog2MaxMvLengthVertical()                  { return m_log2MaxMvLengthVertical; }

    Bool getProgressiveSourceFlag() const { return m_progressiveSourceFlag; }

    Bool getInterlacedSourceFlag() const { return m_interlacedSourceFlag; }

    Bool getNonPackedConstraintFlag() const { return m_nonPackedConstraintFlag; }

    Bool getFrameOnlyConstraintFlag() const { return m_frameOnlyConstraintFlag; }
};

//! \}

#endif // __TENCCFG__
