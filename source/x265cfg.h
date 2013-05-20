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

/** \file     TAppEncCfg.h
    \brief    Handle encoder configuration parameters (header)
*/

#ifndef __TAPPENCCFG__
#define __TAPPENCCFG__

#include "TLibCommon/CommonDef.h"
#include "input/input.h"
#include "output/output.h"
#include "threadpool.h"
#include "x265.h"
#include <sstream>

//! \ingroup TAppEncoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// encoder configuration class
class TAppEncCfg : public x265_param_t
{
protected:
    x265::Input*  m_input;
    x265::Output* m_recon;
    x265::ThreadPool *m_poolHandle;

    int       m_inputBitDepth;                  ///< bit-depth of input file
    int       m_outputBitDepth;                 ///< bit-depth of output file
    uint32_t  m_FrameSkip;                      ///< number of skipped frames from the beginning
    int       m_framesToBeEncoded;              ///< number of encoded frames

    // file I/O
    Char*     m_pchBitstreamFile;               ///< output bitstream file
    Double    m_adLambdaModifier[MAX_TLAYER];   ///< Lambda modifier array for each temporal layer

    // profile/level
    Profile::Name m_profile;
    Level::Tier   m_levelTier;
    Level::Name   m_level;

    int       m_maxTempLayer;                   ///< Max temporal layer
    int       m_progressiveSourceFlag;
    int       m_interlacedSourceFlag;
    int       m_nonPackedConstraintFlag;
    int       m_frameOnlyConstraintFlag;

    // coding structure
    GOPEntry  m_GOPList[MAX_GOP];               ///< the coding structure entries from the config file
    Int       m_numReorderPics[MAX_TLAYER];     ///< total number of reorder pictures
    Int       m_maxDecPicBuffering[MAX_TLAYER]; ///< total number of pictures in the decoded picture buffer
    int       m_iGOPSize;                       ///< GOP size of hierarchical structure

    int       m_iDecodingRefreshType;           ///< random access type
    int       m_extraRPSs;                      ///< extra RPSs added to handle CRA

    // coding quality
    Double    m_fQP;                            ///< QP value of key-picture (floating point)
    Char*     m_pchdQPFile;                     ///< QP offset for each slice (initialized from external file)
    Int*      m_aidQP;                          ///< array of slice QP values

    Int*      m_startOfCodedInterval;
    Int*      m_codedPivotValue;
    Int*      m_targetPivotValue;

    Int       m_useScalingListId;
    Char*     m_scalingListFile;                ///< quantization matrix file name

    // TODO: Death row configuration settings, we will want to configure these similar to x264
    int       m_bUseASR;                          ///< flag for using adaptive motion search range
    int       m_bUseHADME;                        ///< flag for using HAD in sub-pel ME

    int       m_recalculateQPAccordingToLambda;   ///< recalculate QP value according to the lambda value
    int       m_log2MaxMvLengthVertical;          ///< Indicate the maximum absolute value of a decoded vertical MV component in quarter-pel luma units

    int       m_decodedPictureHashSEIEnabled;     ///< Checksum(3)/CRC(2)/MD5(1)/disable(0) acting on decoded picture hash SEI message
    int       m_activeParameterSetsSEIEnabled;
    int       m_recoveryPointSEIEnabled;
    int       m_bufferingPeriodSEIEnabled;
    int       m_pictureTimingSEIEnabled;

    int       m_displayOrientationSEIAngle;
    int       m_temporalLevel0IndexSEIEnabled;
    int       m_gradualDecodingRefreshInfoEnabled;
    int       m_decodingUnitInfoSEIEnabled;
    int       m_SOPDescriptionSEIEnabled;
    int       m_scalableNestingSEIEnabled;

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
    int       m_motionVectorsOverPicBoundariesFlag;///< Indicates that no samples outside the picture boundaries are used for inter prediction
    int       m_minSpatialSegmentationIdc;        ///< Indicates the maximum size of the spatial segments in the pictures in the coded video sequence
    int       m_maxBytesPerPicDenom;              ///< Indicates a number of bytes not exceeded by the sum of the sizes of the VCL NAL units associated with any coded picture
    int       m_maxBitsPerMinCuDenom;             ///< Indicates an upper bound for the number of bits of coding_unit() data
    int       m_log2MaxMvLengthHorizontal;        ///< Indicate the maximum absolute value of a decoded horizontal MV component in quarter-pel luma units

    // internal member functions
    Void      xSetGlobal();                     ///< set global variables
    Void      xCheckParameter();                ///< check validity of configuration values
    Void      xPrintParameter();                ///< print configuration values
    Void      xPrintUsage();                    ///< print usage

public:

    TAppEncCfg();
    virtual ~TAppEncCfg();

public:

    Void  create();                             ///< create option handling class
    Void  destroy();                            ///< destroy option handling class
    Bool  parseCfg(Int argc, Char* argv[]);     ///< parse configuration file to fill member variables
}; // END CLASS DEFINITION TAppEncCfg

//! \}

#endif // __TAPPENCCFG__
