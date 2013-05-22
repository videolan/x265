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

/** \file     TAppEncCfg.cpp
    \brief    Handle encoder configuration parameters
*/

#include <stdlib.h>
#include <cassert>
#include <cstring>
#include <string>
#include <math.h>
#include "TAppCommon/program_options_lite.h"
#include "TLibCommon/TComRom.h"
#include "TLibEncoder/TEncRateCtrl.h"
#include "primitives.h"
#include "threadpool.h"
#include "x265cfg.h"
#include "common.h"

static istream& operator >>(istream &, Level::Name &);
static istream& operator >>(istream &, Level::Tier &);
static istream& operator >>(istream &, Profile::Name &);

using namespace std;
namespace po = df::program_options_lite;

//! \ingroup TAppEncoder
//! \{

// ====================================================================================================================
// Constructor / destructor / initialization / destroy
// ====================================================================================================================

TAppEncCfg::TAppEncCfg()
    : m_poolHandle(NULL)
    , m_pchBitstreamFile()
    , m_pchdQPFile()
    , m_scalingListFile()
{
    m_aidQP = NULL;
    m_input = NULL;
    m_recon = NULL;
}

TAppEncCfg::~TAppEncCfg()
{
    if (m_input)
    {
        m_input->release();
    }
    if (m_recon)
    {
        m_recon->release();
    }
    if (m_aidQP)
    {
        delete[] m_aidQP;
    }
    if (m_poolHandle)
    {
        m_poolHandle->Release();
    }
    free(m_pchBitstreamFile);
    free(m_pchdQPFile);
    free(m_scalingListFile);
}

Void TAppEncCfg::create() {}

Void TAppEncCfg::destroy() {}

std::istringstream &operator >>(std::istringstream &in, GOPEntry &entry)     //input
{
    in >> entry.m_sliceType;
    in >> entry.m_POC;
    in >> entry.m_QPOffset;
    in >> entry.m_QPFactor;
    in >> entry.m_tcOffsetDiv2;
    in >> entry.m_betaOffsetDiv2;
    in >> entry.m_temporalId;
    in >> entry.m_numRefPicsActive;
    in >> entry.m_numRefPics;
    for (Int i = 0; i < entry.m_numRefPics; i++)
    {
        in >> entry.m_referencePics[i];
    }

    in >> entry.m_interRPSPrediction;
#if AUTO_INTER_RPS
    if (entry.m_interRPSPrediction == 1)
    {
        in >> entry.m_deltaRPS;
        in >> entry.m_numRefIdc;
        for (Int i = 0; i < entry.m_numRefIdc; i++)
        {
            in >> entry.m_refIdc[i];
        }
    }
    else if (entry.m_interRPSPrediction == 2)
    {
        in >> entry.m_deltaRPS;
    }
#else // if AUTO_INTER_RPS
    if (entry.m_interRPSPrediction)
    {
        in >> entry.m_deltaRPS;
        in >> entry.m_numRefIdc;
        for (Int i = 0; i < entry.m_numRefIdc; i++)
        {
            in >> entry.m_refIdc[i];
        }
    }
#endif // if AUTO_INTER_RPS
    return in;
}

static const struct MapStrToProfile
{
    const Char* str;
    Profile::Name value;
} strToProfile[] =
{
    { "none", Profile::NONE },
    { "main", Profile::MAIN },
    { "main10", Profile::MAIN10 },
    { "main-still-picture", Profile::MAINSTILLPICTURE },
};

static const struct MapStrToTier
{
    const Char* str;
    Level::Tier value;
} strToTier[] =
{
    { "main", Level::MAIN },
    { "high", Level::HIGH },
};

static const struct MapStrToLevel
{
    const Char* str;
    Level::Name value;
} strToLevel[] =
{
    { "none", Level::NONE },
    { "1",   Level::LEVEL1 },
    { "2",   Level::LEVEL2 },
    { "2.1", Level::LEVEL2_1 },
    { "3",   Level::LEVEL3 },
    { "3.1", Level::LEVEL3_1 },
    { "4",   Level::LEVEL4 },
    { "4.1", Level::LEVEL4_1 },
    { "5",   Level::LEVEL5 },
    { "5.1", Level::LEVEL5_1 },
    { "5.2", Level::LEVEL5_2 },
    { "6",   Level::LEVEL6 },
    { "6.1", Level::LEVEL6_1 },
    { "6.2", Level::LEVEL6_2 },
};

template<typename T, typename P>
static istream& readStrToEnum(P map[], unsigned long mapLen, istream &in, T &val)
{
    string str;

    in >> str;

    for (Int i = 0; i < mapLen; i++)
    {
        if (str == map[i].str)
        {
            val = map[i].value;
            goto found;
        }
    }

    /* not found */
    in.setstate(ios::failbit);
found:
    return in;
}

static istream& operator >>(istream &in, Profile::Name &profile)
{
    return readStrToEnum(strToProfile, sizeof(strToProfile) / sizeof(*strToProfile), in, profile);
}

static istream& operator >>(istream &in, Level::Tier &tier)
{
    return readStrToEnum(strToTier, sizeof(strToTier) / sizeof(*strToTier), in, tier);
}

static istream& operator >>(istream &in, Level::Name &level)
{
    return readStrToEnum(strToLevel, sizeof(strToLevel) / sizeof(*strToLevel), in, level);
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/** \param  argc        number of arguments
    \param  argv        array of arguments
    \retval             true when success
 */
Bool TAppEncCfg::parseCfg(Int argc, Char* argv[])
{
    Bool do_help = false;

    int cpuid;
    int threadcount;

    string cfg_InputFile;
    string cfg_ReconFile;
    string cfg_BitstreamFile;
    string cfg_dQPFile;
    string cfg_ScalingListFile;

    po::Options opts;
    opts.addOptions()
    ("help", do_help, false, "this help text")
    ("c", po::parseConfigFile, "configuration file name")

    ("cpuid",                 cpuid,               0, "SIMD architecture. 2:MMX2 .. 8:AVX2 (default:0-auto)")
    ("threads",               threadcount,         0, "Number of threads for thread pool (default:CPU HT core count)")

    // File, I/O and source parameters
    ("InputFile,i",           cfg_InputFile,     string(""), "Original YUV input file name")
    ("BitstreamFile,b",       cfg_BitstreamFile, string("hevc.bin"), "Bitstream output file name")
    ("ReconFile,o",           cfg_ReconFile,     string(""), "Reconstructed YUV output file name")
    ("SourceWidth,-wdt",      iSourceWidth,      0, "Source picture width")
    ("SourceHeight,-hgt",     iSourceHeight,     0, "Source picture height")
    ("FrameRate,-fr",         iFrameRate,        0, "Frame rate")
    ("FrameSkip,-fs",         m_FrameSkip,         0u, "Number of frames to skip at start of input YUV")
    ("FramesToBeEncoded,f",   m_framesToBeEncoded, 0, "Number of frames to be encoded (default=all)")

    ("InternalBitDepth",      internalBitDepth,  0, "Bit-depth the codec operates at. (default:InputBitDepth)"
    "If different to InputBitDepth, source data will be converted")
    ("InputBitDepth",         m_inputBitDepth,     8, "Bit-depth of input file")
    ("OutputBitDepth",        m_outputBitDepth,    0, "Bit-depth of output file (default:InternalBitDepth)")

    // Profile and level
    ("Profile", m_profile,   Profile::NONE, "Profile to be used when encoding (Incomplete)")
    ("Level",   m_level,     Level::NONE,   "Level limit to be used, eg 5.1 (Incomplete)")
    ("Tier",    m_levelTier, Level::MAIN,   "Tier to use for interpretation of --Level")

    ("ProgressiveSource", m_progressiveSourceFlag,   0, "Indicate that source is progressive")
    ("InterlacedSource",  m_interlacedSourceFlag,    0, "Indicate that source is interlaced")
    ("NonPackedSource",   m_nonPackedConstraintFlag, 0, "Indicate that source does not contain frame packing")
    ("FrameOnly",         m_frameOnlyConstraintFlag, 0, "Indicate that the bitstream contains only frames")

    ("MaxCUSize,s",             uiMaxCUSize,              64u, "Maximum CU size")
    ("MaxPartitionDepth,h",     uiMaxCUDepth,              4u, "CU depth")

    ("QuadtreeTULog2MaxSize",   uiQuadtreeTULog2MaxSize,   6u, "Maximum TU size in logarithm base 2")
    ("QuadtreeTULog2MinSize",   uiQuadtreeTULog2MinSize,   2u, "Minimum TU size in logarithm base 2")

    ("QuadtreeTUMaxDepthIntra", uiQuadtreeTUMaxDepthIntra, 1u, "Depth of TU tree for intra CUs")
    ("QuadtreeTUMaxDepthInter", uiQuadtreeTUMaxDepthInter, 2u, "Depth of TU tree for inter CUs")

    // Coding structure parameters
    ("IntraPeriod,-ip",         iIntraPeriod,              -1, "Intra period in frames, (-1: only first frame)")
    ("DecodingRefreshType,-dr", m_iDecodingRefreshType,     0, "Intra refresh type (0:none 1:CRA 2:IDR)")
    ("GOPSize,g",               m_iGOPSize,                 1, "GOP size of temporal structure")

    // motion options
    ("SearchMethod,-me",        searchMethod,               3, "0:DIA 1:HEX 2:UMH 3:STAR 4:ORIG")
    ("SearchRange,-sr",         iSearchRange,              96, "Motion search range")
    ("BipredSearchRange",       bipredSearchRange,          4, "Motion search range for bipred refinement")
    ("HadamardME",              m_bUseHADME,                1, "Hadamard ME for fractional-pel")
    ("ASR",                     m_bUseASR,                  0, "Adaptive motion search range")

    // Mode decision parameters
    ("LambdaModifier0,-LM0", m_adLambdaModifier[0], (Double)1.0, "Lambda modifier for temporal layer 0")
    ("LambdaModifier1,-LM1", m_adLambdaModifier[1], (Double)1.0, "Lambda modifier for temporal layer 1")
    ("LambdaModifier2,-LM2", m_adLambdaModifier[2], (Double)1.0, "Lambda modifier for temporal layer 2")
    ("LambdaModifier3,-LM3", m_adLambdaModifier[3], (Double)1.0, "Lambda modifier for temporal layer 3")
    ("LambdaModifier4,-LM4", m_adLambdaModifier[4], (Double)1.0, "Lambda modifier for temporal layer 4")
    ("LambdaModifier5,-LM5", m_adLambdaModifier[5], (Double)1.0, "Lambda modifier for temporal layer 5")
    ("LambdaModifier6,-LM6", m_adLambdaModifier[6], (Double)1.0, "Lambda modifier for temporal layer 6")
    ("LambdaModifier7,-LM7", m_adLambdaModifier[7], (Double)1.0, "Lambda modifier for temporal layer 7")

    /* Quantization parameters */
    ("QP,q",                m_fQP,              30.0, "Qp value, if value is float, QP is switched once during encoding")
    ("MaxCuDQPDepth,-dqd",  iMaxCuDQPDepth,       0u, "max depth for a minimum CuDQP")

    ("CbQpOffset,-cbqpofs",  cbQpOffset,           0, "Chroma Cb QP Offset")
    ("CrQpOffset,-crqpofs",  crQpOffset,           0, "Chroma Cr QP Offset")

    ("AdaptiveQpSelection,-aqps",     bUseAdaptQpSelect,        0, "AdaptiveQpSelection")
    ("dQPFile,m",                     cfg_dQPFile,       string(""), "dQP file name")

    ("AdaptiveQP,-aq",                bUseAdaptiveQP,           0, "QP adaptation based on a psycho-visual model")
    ("MaxQPAdaptationRange,-aqr",     iQPAdaptationRange,       6, "QP adaptation range")
    ("RDOQ",                          useRDOQ,                  1)
    ("RDOQTS",                        useRDOQTS,                1)
    ("RDpenalty",                     rdPenalty,                0,  "RD-penalty for 32x32 TU for intra in non-intra slices. 0:disbaled  1:RD-penalty  2:maximum RD-penalty")

    // Deblocking filter parameters
    ("LoopFilterDisable",              m_bLoopFilterDisable,             0)
    ("LoopFilterOffsetInPPS",          m_loopFilterOffsetInPPS,          0)
    ("LoopFilterBetaOffset_div2",      m_loopFilterBetaOffsetDiv2,       0)
    ("LoopFilterTcOffset_div2",        m_loopFilterTcOffsetDiv2,         0)
    ("DeblockingFilterControlPresent", m_DeblockingFilterControlPresent, 0)
    ("DeblockingFilterMetric",         m_DeblockingFilterMetric,         0)

    // Coding tools
    ("AMP",                      enableAMP,                 1,  "Enable asymmetric motion partitions")
    ("RectInter",                enableRectInter,           1,  "Enable rectangular motion partitions Nx2N and 2NxN, disabling also disables AMP")
    ("TransformSkip",            useTransformSkip,          0,  "Intra transform skipping")
    ("TransformSkipFast",        useTransformSkipFast,      0,  "Fast intra transform skipping")
    ("SAO",                      bUseSAO,                   1,  "Enable Sample Adaptive Offset")
    ("MaxNumOffsetsPerPic",      maxNumOffsetsPerPic,    2048,  "Max number of SAO offset per picture (Default: 2048)")
    ("SAOLcuBoundary",           saoLcuBoundary,            0,  "0: right/bottom LCU boundary areas skipped from SAO parameter estimation, 1: non-deblocked pixels are used for those areas")
    ("SAOLcuBasedOptimization",  saoLcuBasedOptimization,   1,  "0: SAO picture-based optimization, 1: SAO LCU-based optimization ")

    ("ConstrainedIntraPred",     bUseConstrainedIntraPred,  0, "Constrained Intra Prediction")

    ("PCMEnabledFlag",           m_usePCM,                    0)
    ("PCMLog2MaxSize",           m_pcmLog2MaxSize,            5)
    ("PCMLog2MinSize",           m_uiPCMLog2MinSize,          3)
    ("PCMInputBitDepthFlag",     m_bPCMInputBitDepthFlag,     1)
    ("PCMFilterDisableFlag",     m_bPCMFilterDisableFlag,     0)
    ("LosslessCuEnabled",        m_useLossless,               0)

    ("WeightedPredP,-wpP",       useWeightedPred,               0,          "Use weighted prediction in P slices")
    ("WeightedPredB,-wpB",       useWeightedBiPred,             0,          "Use weighted (bidirectional) prediction in B slices")
    ("Log2ParallelMergeLevel",   log2ParallelMergeLevel,       2u,          "Parallel merge estimation region")
    ("WaveFrontSynchro",         iWaveFrontSynchro,             0,          "0: no synchro; 1 synchro with TR; 2 TRR etc")
    ("ScalingList",              m_useScalingListId,            0,          "0: no scaling list, 1: default scaling lists, 2: scaling lists specified in ScalingListFile")
    ("ScalingListFile",          cfg_ScalingListFile,           string(""), "Scaling list file name")
    ("SignHideFlag,-SBH",        signHideFlag, 1)
    ("MaxNumMergeCand",          maxNumMergeCand,               5u,         "Maximum number of merge candidates")

    /* Misc. */
    ("SEIDecodedPictureHash",       m_decodedPictureHashSEIEnabled, 0, "Control generation of decode picture hash SEI messages\n"
    "\t3: checksum\n"
    "\t2: CRC\n"
    "\t1: use MD5\n"
    "\t0: disable")
    ("SEIpictureDigest",            m_decodedPictureHashSEIEnabled, 0, "deprecated alias for SEIDecodedPictureHash")
    ("TMVPMode", TMVPModeId, 1, "TMVP mode 0: TMVP disable for all slices. 1: TMVP enable for all slices (default) 2: TMVP enable for certain slices only")
    ("FDM", useFastDecisionForMerge, 1, "Fast decision for Merge RD Cost")
    ("CFM", bUseCbfFastMode, 0, "Cbf fast mode setting")
    ("ESD", useEarlySkipDetection, 0, "Early SKIP detection setting")
    ("StrongIntraSmoothing,-sis",      useStrongIntraSmoothing,            1, "Enable strong intra smoothing for 32x32 blocks")

    ("RateControl",         m_RCEnableRateControl,       0, "Rate control: enable rate control")
    ("TargetBitrate",       m_RCTargetBitrate,           0, "Rate control: target bitrate")
    ("KeepHierarchicalBit", m_RCKeepHierarchicalBit,     0, "Rate control: keep hierarchical bit allocation in rate control algorithm")
    ("LCULevelRateControl", m_RCLCULevelRC,              1, "Rate control: true: LCU level RC; false: picture level RC")
    ("RCLCUSeparateModel",  m_RCUseLCUSeparateModel,     1, "Rate control: use LCU level separate R-lambda model")
    ("InitialQP",           m_RCInitialQP,               0, "Rate control: initial QP")
    ("RCForceIntraQP",      m_RCForceIntraQP,            0, "Rate control: force intra QP to be equal to initial QP")

    ("TransquantBypassEnableFlag",     m_TransquantBypassEnableFlag,         0, "transquant_bypass_enable_flag indicator in PPS")
    ("CUTransquantBypassFlagValue",    m_CUTransquantBypassFlagValue,        0, "Fixed cu_transquant_bypass_flag value, when transquant_bypass_enable_flag is enabled")
    ("RecalculateQPAccordingToLambda", m_recalculateQPAccordingToLambda,     0, "Recalculate QP values according to lambda values. Do not suggest to be enabled in all intra case")

    ("SEIActiveParameterSets",         m_activeParameterSetsSEIEnabled,      0, "Enable generation of active parameter sets SEI messages")
    ("VuiParametersPresent,-vui",      m_vuiParametersPresentFlag,           0, "Enable generation of vui_parameters()")
    ("AspectRatioInfoPresent",         m_aspectRatioInfoPresentFlag,         0, "Signals whether aspect_ratio_idc is present")
    ("AspectRatioIdc",                 m_aspectRatioIdc,                     0, "aspect_ratio_idc")
    ("SarWidth",                       m_sarWidth,                           0, "horizontal size of the sample aspect ratio")
    ("SarHeight",                      m_sarHeight,                          0, "vertical size of the sample aspect ratio")
    ("OverscanInfoPresent",            m_overscanInfoPresentFlag,            0, "Indicates whether conformant decoded pictures are suitable for display using overscan\n")
    ("OverscanAppropriate",            m_overscanAppropriateFlag,            0, "Indicates whether conformant decoded pictures are suitable for display using overscan\n")
    ("VideoSignalTypePresent",         m_videoSignalTypePresentFlag,         0, "Signals whether video_format, video_full_range_flag, and colour_description_present_flag are present")
    ("VideoFormat",                    m_videoFormat,                        5, "Indicates representation of pictures")
    ("VideoFullRange",                 m_videoFullRangeFlag,                 0, "Indicates the black level and range of luma and chroma signals")
    ("ColourDescriptionPresent",       m_colourDescriptionPresentFlag,       0, "Signals whether colour_primaries, transfer_characteristics and matrix_coefficients are present")
    ("ColourPrimaries",                m_colourPrimaries,                    2, "Indicates chromaticity coordinates of the source primaries")
    ("TransferCharateristics",         m_transferCharacteristics,            2, "Indicates the opto-electronic transfer characteristics of the source")
    ("MatrixCoefficients",             m_matrixCoefficients,                 2, "Describes the matrix coefficients used in deriving luma and chroma from RGB primaries")
    ("ChromaLocInfoPresent",           m_chromaLocInfoPresentFlag,           0, "Signals whether chroma_sample_loc_type_top_field and chroma_sample_loc_type_bottom_field are present")
    ("ChromaSampleLocTypeTopField",    m_chromaSampleLocTypeTopField,        0, "Specifies the location of chroma samples for top field")
    ("ChromaSampleLocTypeBottomField", m_chromaSampleLocTypeBottomField,     0, "Specifies the location of chroma samples for bottom field")
    ("NeutralChromaIndication",        m_neutralChromaIndicationFlag,        0, "Indicates that the value of all decoded chroma samples is equal to 1<<(BitDepthCr-1)")
    ("DefaultDisplayWindowFlag",       m_defaultDisplayWindowFlag,           0, "Indicates the presence of the Default Window parameters")
    ("DefDispWinLeftOffset",           m_defDispWinLeftOffset,               0, "Specifies the left offset of the default display window from the conformance window")
    ("DefDispWinRightOffset",          m_defDispWinRightOffset,              0, "Specifies the right offset of the default display window from the conformance window")
    ("DefDispWinTopOffset",            m_defDispWinTopOffset,                0, "Specifies the top offset of the default display window from the conformance window")
    ("DefDispWinBottomOffset",         m_defDispWinBottomOffset,             0, "Specifies the bottom offset of the default display window from the conformance window")
    ("FrameFieldInfoPresentFlag",      m_frameFieldInfoPresentFlag,          0, "Indicates that pic_struct and field coding related values are present in picture timing SEI messages")
    ("PocProportionalToTimingFlag",    m_pocProportionalToTimingFlag,        0, "Indicates that the POC value is proportional to the output time w.r.t. first picture in CVS")
    ("NumTicksPocDiffOneMinus1",       m_numTicksPocDiffOneMinus1,           0, "Number of ticks minus 1 that for a POC difference of one")
    ("BitstreamRestriction",           m_bitstreamRestrictionFlag,           0, "Signals whether bitstream restriction parameters are present")
    ("MotionVectorsOverPicBoundaries", m_motionVectorsOverPicBoundariesFlag, 0, "Indicates that no samples outside the picture boundaries are used for inter prediction")
    ("MaxBytesPerPicDenom",            m_maxBytesPerPicDenom,                2, "Indicates a number of bytes not exceeded by the sum of the sizes of the VCL NAL units associated with any coded picture")
    ("MaxBitsPerMinCuDenom",           m_maxBitsPerMinCuDenom,               1, "Indicates an upper bound for the number of bits of coding_unit() data")
    ("Log2MaxMvLengthHorizontal",      m_log2MaxMvLengthHorizontal,         15, "Indicate the maximum absolute value of a decoded horizontal MV component in quarter-pel luma units")
    ("Log2MaxMvLengthVertical",        m_log2MaxMvLengthVertical,           15, "Indicate the maximum absolute value of a decoded vertical MV component in quarter-pel luma units")

    ("SEIRecoveryPoint",               m_recoveryPointSEIEnabled,            0, "Control generation of recovery point SEI messages")
    ("SEIBufferingPeriod",             m_bufferingPeriodSEIEnabled,          0, "Control generation of buffering period SEI messages")
    ("SEIPictureTiming",               m_pictureTimingSEIEnabled,            0, "Control generation of picture timing SEI messages")

    ("SEIDisplayOrientation",          m_displayOrientationSEIAngle,         0, "Control generation of display orientation SEI messages\n"
    "\tN: 0 < N < (2^16 - 1) enable display orientation SEI message with anticlockwise_rotation = N and display_orientation_repetition_period = 1\n"
    "\t0: disable")
    ("SEITemporalLevel0Index",         m_temporalLevel0IndexSEIEnabled,      0, "Control generation of temporal level 0 index SEI messages")
    ("SEIGradualDecodingRefreshInfo",  m_gradualDecodingRefreshInfoEnabled,  0, "Control generation of gradual decoding refresh information SEI message")
    ("SEIDecodingUnitInfo",            m_decodingUnitInfoSEIEnabled,         0, "Control generation of decoding unit information SEI message.")
    ("SEISOPDescription",              m_SOPDescriptionSEIEnabled,           0, "Control generation of SOP description SEI messages")
    ("SEIScalableNesting",             m_scalableNestingSEIEnabled,          0, "Control generation of scalable nesting SEI messages")
    ;

    for (Int i = 1; i < MAX_GOP + 1; i++)
    {
        std::ostringstream cOSS;
        cOSS << "Frame" << i;
        opts.addOptions()(cOSS.str(), m_GOPList[i - 1], GOPEntry());
    }

    po::setDefaults(opts);

    try
    {
        const list<const Char*>& argv_unhandled = po::scanArgv(opts, argc, (const Char**)argv);

        for (list<const Char*>::const_iterator it = argv_unhandled.begin(); it != argv_unhandled.end(); it++)
        {
            fprintf(stderr, "Unknown argument ignored: `%s'\n", *it);
        }
    }
    catch (po::ParseFailure &e)
    {
        cerr << "Error parsing option \"" << e.arg << "\" with argument \"" << e.val << "\"." << endl;
        return false;
    }

    if (argc == 1 || do_help)
    {
        /* argc == 1: no options have been specified */
        po::doHelp(cout, opts);
        return false;
    }

    x265::SetupPrimitives(cpuid);
    m_poolHandle = x265::ThreadPool::AllocThreadPool(threadcount);

    /*
     * Set any derived parameters
     */
    /* convert std::string to c string for compatability */
    m_pchBitstreamFile = cfg_BitstreamFile.empty() ? NULL : strdup(cfg_BitstreamFile.c_str());
    m_pchdQPFile = cfg_dQPFile.empty() ? NULL : strdup(cfg_dQPFile.c_str());

    /* parse the width, height, frame rate from the y4m files if it is not given in the configuration file */
    m_input = x265::Input::Open(cfg_InputFile.c_str());
    if (!m_input || m_input->isFail())
    {
        printf("Unable to open source file\n");
        return 1;
    }
    if (m_input->getWidth())
    {
        iSourceWidth = m_input->getWidth();
        iSourceHeight = m_input->getHeight();
        iFrameRate = (int)m_input->getRate();
        m_inputBitDepth = 8;
    }
    else
    {
        m_input->setDimensions(iSourceWidth, iSourceHeight);
        m_input->setBitDepth(m_inputBitDepth);
    }

    /* rules for input, output and internal bitdepths as per help text */
    if (!internalBitDepth) { internalBitDepth = m_inputBitDepth; }
    if (!m_outputBitDepth) { m_outputBitDepth = internalBitDepth; }

    int numRemainingFrames = m_input->guessFrameCount();

    if (m_FrameSkip && m_input)
    {
        m_input->skipFrames(m_FrameSkip);
    }

    m_framesToBeEncoded = m_framesToBeEncoded ? min(m_framesToBeEncoded, numRemainingFrames) : numRemainingFrames;

    printf("Input File                   : %s (%d total frames)\n", cfg_InputFile.c_str(), numRemainingFrames);

    if (!cfg_ReconFile.empty())
    {
        printf("Reconstruction File          : %s\n", cfg_ReconFile.c_str());
        m_recon = x265::Output::Open(cfg_ReconFile.c_str(), iSourceWidth, iSourceHeight, m_outputBitDepth, iFrameRate);
        if (m_recon->isFail())
        {
            printf("Unable to write reconstruction file\n");
            m_recon->release();
            m_recon = 0;
        }
    }

#if !HIGH_BIT_DEPTH
    if (m_inputBitDepth != 8 || m_outputBitDepth != 8 || internalBitDepth != 8)
    {
        printf("x265 not compiled for bit depths greater than 8\n");
        exit(1);
    }
#endif

    m_scalingListFile = cfg_ScalingListFile.empty() ? NULL : strdup(cfg_ScalingListFile.c_str());

    // allocate slice-based dQP values
    m_aidQP = new Int[m_framesToBeEncoded + m_iGOPSize + 1];
    ::memset(m_aidQP, 0, sizeof(Int) * (m_framesToBeEncoded + m_iGOPSize + 1));

    // handling of floating-point QP values
    // if QP is not integer, sequence is split into two sections having QP and QP+1
    iQP = (Int)(m_fQP);
    if (iQP < m_fQP)
    {
        Int iSwitchPOC = (Int)(m_framesToBeEncoded - (m_fQP - iQP) * m_framesToBeEncoded + 0.5);

        iSwitchPOC = (Int)((Double)iSwitchPOC / m_iGOPSize + 0.5) * m_iGOPSize;
        for (Int i = iSwitchPOC; i < m_framesToBeEncoded + m_iGOPSize + 1; i++)
        {
            m_aidQP[i] = 1;
        }
    }

    // reading external dQP description from file
    if (m_pchdQPFile)
    {
        FILE* fpt = fopen(m_pchdQPFile, "r");
        if (fpt)
        {
            Int iValue;
            Int iPOC = 0;
            while (iPOC < m_framesToBeEncoded)
            {
                if (fscanf(fpt, "%d", &iValue) == EOF) break;
                m_aidQP[iPOC] = iValue;
                iPOC++;
            }

            fclose(fpt);
        }
    }

    // check validity of input parameters
    xCheckParameter();

    // set global variables
    x265_set_globals(this, m_inputBitDepth);

    // print-out parameters
    printf("Bitstream File               : %s\n", m_pchBitstreamFile);
    printf("Frame index                  : %u - %d (%d frames)\n", m_FrameSkip, m_FrameSkip + m_framesToBeEncoded - 1, m_framesToBeEncoded);
    printf("GOP size                     : %d\n", m_iGOPSize);
    x265_print_params(this);

    return true;
}

static inline Bool confirmPara(Bool bflag, const Char* message)
{
    if (!bflag)
        return false;

    printf("Error: %s\n", message);
    return true;
}

Void TAppEncCfg::xCheckParameter()
{
#define xConfirmPara(a, b) check_failed |= confirmPara(a, b)
    Bool check_failed = x265_check_params(this) ? true : false;

    // validate (or in some instances generate) internally calculated parameters
#if HIGH_BIT_DEPTH
    xConfirmPara(m_inputBitDepth < 8,                                                      "InputBitDepth must be at least 8");
#else
    xConfirmPara(m_inputBitDepth != 8,                                                     "InputBitDepth must be 8");
#endif
    xConfirmPara(m_outputBitDepth > internalBitDepth,                                      "OutputBitDepth must be less than or equal to InternalBitDepth");
    xConfirmPara(m_framesToBeEncoded <= 0,                                                 "Total Number Of Frames encoded must be more than 0");
    xConfirmPara(m_iGOPSize < 1,                                                           "GOP Size must be greater or equal to 1");
    xConfirmPara(m_iGOPSize > 1 &&  m_iGOPSize % 2,                                        "GOP Size must be a multiple of 2, if GOP Size is greater than 1");
    xConfirmPara(m_iDecodingRefreshType < 0 || m_iDecodingRefreshType > 2,                 "Decoding Refresh Type must be equal to 0, 1 or 2");
    xConfirmPara((iIntraPeriod > 0 && iIntraPeriod < m_iGOPSize) || iIntraPeriod == 0,     "Intra period must be more than GOP size, or -1 , not 0");

    if (m_iDecodingRefreshType == 2)
    {
        xConfirmPara(iIntraPeriod > 0 && iIntraPeriod <= m_iGOPSize,                       "Intra period must be larger than GOP size for periodic IDR pictures");
    }

    /* if this is an intra-only sequence, ie IntraPeriod=1, don't verify the GOP structure
     * This permits the ability to omit a GOP structure specification */
    if (iIntraPeriod == 1 && m_GOPList[0].m_POC == -1)
    {
        m_GOPList[0] = GOPEntry();
        m_GOPList[0].m_QPFactor = 1;
        m_GOPList[0].m_betaOffsetDiv2 = 0;
        m_GOPList[0].m_tcOffsetDiv2 = 0;
        m_GOPList[0].m_POC = 1;
        m_GOPList[0].m_numRefPicsActive = 4;
    }

    Bool verifiedGOP = false;
    Bool errorGOP = false;
    Int checkGOP = 1;
    Int numRefs = 1;
    Int refList[MAX_NUM_REF_PICS + 1];
    refList[0] = 0;
    Bool isOK[MAX_GOP];
    for (Int i = 0; i < MAX_GOP; i++)
    {
        isOK[i] = false;
    }

    Int numOK = 0;
    xConfirmPara(iIntraPeriod >= 0 && (iIntraPeriod % m_iGOPSize != 0), "Intra period must be a multiple of GOPSize, or -1");

    for (Int i = 0; i < m_iGOPSize; i++)
    {
        if (m_GOPList[i].m_POC == m_iGOPSize)
        {
            xConfirmPara(m_GOPList[i].m_temporalId != 0, "The last frame in each GOP must have temporal ID = 0 ");
        }
    }

    if ((iIntraPeriod != 1) && !m_loopFilterOffsetInPPS && m_DeblockingFilterControlPresent && (!m_bLoopFilterDisable))
    {
        for (Int i = 0; i < m_iGOPSize; i++)
        {
            xConfirmPara((m_GOPList[i].m_betaOffsetDiv2 + m_loopFilterBetaOffsetDiv2) < -6 || (m_GOPList[i].m_betaOffsetDiv2 + m_loopFilterBetaOffsetDiv2) > 6, "Loop Filter Beta Offset div. 2 for one of the GOP entries exceeds supported range (-6 to 6)");
            xConfirmPara((m_GOPList[i].m_tcOffsetDiv2 + m_loopFilterTcOffsetDiv2) < -6 || (m_GOPList[i].m_tcOffsetDiv2 + m_loopFilterTcOffsetDiv2) > 6, "Loop Filter Tc Offset div. 2 for one of the GOP entries exceeds supported range (-6 to 6)");
        }
    }
    m_extraRPSs = 0;
    //start looping through frames in coding order until we can verify that the GOP structure is correct.
    while (!verifiedGOP && !errorGOP)
    {
        Int curGOP = (checkGOP - 1) % m_iGOPSize;
        Int curPOC = ((checkGOP - 1) / m_iGOPSize) * m_iGOPSize + m_GOPList[curGOP].m_POC;
        if (m_GOPList[curGOP].m_POC < 0)
        {
            printf("\nError: found fewer Reference Picture Sets than GOPSize\n");
            errorGOP = true;
        }
        else
        {
            //check that all reference pictures are available, or have a POC < 0 meaning they might be available in the next GOP.
            Bool beforeI = false;
            for (Int i = 0; i < m_GOPList[curGOP].m_numRefPics; i++)
            {
                Int absPOC = curPOC + m_GOPList[curGOP].m_referencePics[i];
                if (absPOC < 0)
                {
                    beforeI = true;
                }
                else
                {
                    Bool found = false;
                    for (Int j = 0; j < numRefs; j++)
                    {
                        if (refList[j] == absPOC)
                        {
                            found = true;
                            for (Int k = 0; k < m_iGOPSize; k++)
                            {
                                if (absPOC % m_iGOPSize == m_GOPList[k].m_POC % m_iGOPSize)
                                {
                                    if (m_GOPList[k].m_temporalId == m_GOPList[curGOP].m_temporalId)
                                    {
                                        m_GOPList[k].m_refPic = true;
                                    }
                                    m_GOPList[curGOP].m_usedByCurrPic[i] = m_GOPList[k].m_temporalId <= m_GOPList[curGOP].m_temporalId;
                                }
                            }
                        }
                    }

                    if (!found)
                    {
                        printf("\nError: ref pic %d is not available for GOP frame %d\n", m_GOPList[curGOP].m_referencePics[i], curGOP + 1);
                        errorGOP = true;
                    }
                }
            }

            if (!beforeI && !errorGOP)
            {
                //all ref frames were present
                if (!isOK[curGOP])
                {
                    numOK++;
                    isOK[curGOP] = true;
                    if (numOK == m_iGOPSize)
                    {
                        verifiedGOP = true;
                    }
                }
            }
            else
            {
                //create a new GOPEntry for this frame containing all the reference pictures that were available (POC > 0)
                m_GOPList[m_iGOPSize + m_extraRPSs] = m_GOPList[curGOP];
                Int newRefs = 0;
                for (Int i = 0; i < m_GOPList[curGOP].m_numRefPics; i++)
                {
                    Int absPOC = curPOC + m_GOPList[curGOP].m_referencePics[i];
                    if (absPOC >= 0)
                    {
                        m_GOPList[m_iGOPSize + m_extraRPSs].m_referencePics[newRefs] = m_GOPList[curGOP].m_referencePics[i];
                        m_GOPList[m_iGOPSize + m_extraRPSs].m_usedByCurrPic[newRefs] = m_GOPList[curGOP].m_usedByCurrPic[i];
                        newRefs++;
                    }
                }

                Int numPrefRefs = m_GOPList[curGOP].m_numRefPicsActive;

                for (Int offset = -1; offset > -checkGOP; offset--)
                {
                    //step backwards in coding order and include any extra available pictures we might find useful to replace the ones with POC < 0.
                    Int offGOP = (checkGOP - 1 + offset) % m_iGOPSize;
                    Int offPOC = ((checkGOP - 1 + offset) / m_iGOPSize) * m_iGOPSize + m_GOPList[offGOP].m_POC;
                    if (offPOC >= 0 && m_GOPList[offGOP].m_temporalId <= m_GOPList[curGOP].m_temporalId)
                    {
                        Bool newRef = false;
                        for (Int i = 0; i < numRefs; i++)
                        {
                            if (refList[i] == offPOC)
                            {
                                newRef = true;
                            }
                        }

                        for (Int i = 0; i < newRefs; i++)
                        {
                            if (m_GOPList[m_iGOPSize + m_extraRPSs].m_referencePics[i] == offPOC - curPOC)
                            {
                                newRef = false;
                            }
                        }

                        if (newRef)
                        {
                            Int insertPoint = newRefs;
                            //this picture can be added, find appropriate place in list and insert it.
                            if (m_GOPList[offGOP].m_temporalId == m_GOPList[curGOP].m_temporalId)
                            {
                                m_GOPList[offGOP].m_refPic = true;
                            }
                            for (Int j = 0; j < newRefs; j++)
                            {
                                if (m_GOPList[m_iGOPSize + m_extraRPSs].m_referencePics[j] < offPOC - curPOC || m_GOPList[m_iGOPSize + m_extraRPSs].m_referencePics[j] > 0)
                                {
                                    insertPoint = j;
                                    break;
                                }
                            }

                            Int prev = offPOC - curPOC;
                            Int prevUsed = m_GOPList[offGOP].m_temporalId <= m_GOPList[curGOP].m_temporalId;
                            for (Int j = insertPoint; j < newRefs + 1; j++)
                            {
                                Int newPrev = m_GOPList[m_iGOPSize + m_extraRPSs].m_referencePics[j];
                                Int newUsed = m_GOPList[m_iGOPSize + m_extraRPSs].m_usedByCurrPic[j];
                                m_GOPList[m_iGOPSize + m_extraRPSs].m_referencePics[j] = prev;
                                m_GOPList[m_iGOPSize + m_extraRPSs].m_usedByCurrPic[j] = prevUsed;
                                prevUsed = newUsed;
                                prev = newPrev;
                            }

                            newRefs++;
                        }
                    }
                    if (newRefs >= numPrefRefs)
                    {
                        break;
                    }
                }

                m_GOPList[m_iGOPSize + m_extraRPSs].m_numRefPics = newRefs;
                m_GOPList[m_iGOPSize + m_extraRPSs].m_POC = curPOC;
                if (m_extraRPSs == 0)
                {
                    m_GOPList[m_iGOPSize + m_extraRPSs].m_interRPSPrediction = 0;
                    m_GOPList[m_iGOPSize + m_extraRPSs].m_numRefIdc = 0;
                }
                else
                {
                    Int rIdx =  m_iGOPSize + m_extraRPSs - 1;
                    Int refPOC = m_GOPList[rIdx].m_POC;
                    Int refPics = m_GOPList[rIdx].m_numRefPics;
                    Int newIdc = 0;
                    for (Int i = 0; i <= refPics; i++)
                    {
                        Int deltaPOC = ((i != refPics) ? m_GOPList[rIdx].m_referencePics[i] : 0); // check if the reference abs POC is >= 0
                        Int absPOCref = refPOC + deltaPOC;
                        Int refIdc = 0;
                        for (Int j = 0; j < m_GOPList[m_iGOPSize + m_extraRPSs].m_numRefPics; j++)
                        {
                            if ((absPOCref - curPOC) == m_GOPList[m_iGOPSize + m_extraRPSs].m_referencePics[j])
                            {
                                if (m_GOPList[m_iGOPSize + m_extraRPSs].m_usedByCurrPic[j])
                                {
                                    refIdc = 1;
                                }
                                else
                                {
                                    refIdc = 2;
                                }
                            }
                        }

                        m_GOPList[m_iGOPSize + m_extraRPSs].m_refIdc[newIdc] = refIdc;
                        newIdc++;
                    }

                    m_GOPList[m_iGOPSize + m_extraRPSs].m_interRPSPrediction = 1;
                    m_GOPList[m_iGOPSize + m_extraRPSs].m_numRefIdc = newIdc;
                    m_GOPList[m_iGOPSize + m_extraRPSs].m_deltaRPS = refPOC - m_GOPList[m_iGOPSize + m_extraRPSs].m_POC;
                }
                curGOP = m_iGOPSize + m_extraRPSs;
                m_extraRPSs++;
            }
            numRefs = 0;
            for (Int i = 0; i < m_GOPList[curGOP].m_numRefPics; i++)
            {
                Int absPOC = curPOC + m_GOPList[curGOP].m_referencePics[i];
                if (absPOC >= 0)
                {
                    refList[numRefs] = absPOC;
                    numRefs++;
                }
            }

            refList[numRefs] = curPOC;
            numRefs++;
        }
        checkGOP++;
    }

    xConfirmPara(errorGOP, "Invalid GOP structure given");
    m_maxTempLayer = 1;
    for (Int i = 0; i < m_iGOPSize; i++)
    {
        if (m_GOPList[i].m_temporalId >= m_maxTempLayer)
        {
            m_maxTempLayer = m_GOPList[i].m_temporalId + 1;
        }
        xConfirmPara(m_GOPList[i].m_sliceType != 'B' && m_GOPList[i].m_sliceType != 'P', "Slice type must be equal to B or P");
    }

    for (Int i = 0; i < MAX_TLAYER; i++)
    {
        m_numReorderPics[i] = 0;
        m_maxDecPicBuffering[i] = 1;
    }

    for (Int i = 0; i < m_iGOPSize; i++)
    {
        if (m_GOPList[i].m_numRefPics + 1 > m_maxDecPicBuffering[m_GOPList[i].m_temporalId])
        {
            m_maxDecPicBuffering[m_GOPList[i].m_temporalId] = m_GOPList[i].m_numRefPics + 1;
        }
        Int highestDecodingNumberWithLowerPOC = 0;
        for (Int j = 0; j < m_iGOPSize; j++)
        {
            if (m_GOPList[j].m_POC <= m_GOPList[i].m_POC)
            {
                highestDecodingNumberWithLowerPOC = j;
            }
        }

        Int numReorder = 0;
        for (Int j = 0; j < highestDecodingNumberWithLowerPOC; j++)
        {
            if (m_GOPList[j].m_temporalId <= m_GOPList[i].m_temporalId &&
                m_GOPList[j].m_POC > m_GOPList[i].m_POC)
            {
                numReorder++;
            }
        }

        if (numReorder > m_numReorderPics[m_GOPList[i].m_temporalId])
        {
            m_numReorderPics[m_GOPList[i].m_temporalId] = numReorder;
        }
    }

    for (Int i = 0; i < MAX_TLAYER - 1; i++)
    {
        // a lower layer can not have higher value of m_numReorderPics than a higher layer
        if (m_numReorderPics[i + 1] < m_numReorderPics[i])
        {
            m_numReorderPics[i + 1] = m_numReorderPics[i];
        }
        // the value of num_reorder_pics[ i ] shall be in the range of 0 to max_dec_pic_buffering[ i ] - 1, inclusive
        if (m_numReorderPics[i] > m_maxDecPicBuffering[i] - 1)
        {
            m_maxDecPicBuffering[i] = m_numReorderPics[i] + 1;
        }
        // a lower layer can not have higher value of m_uiMaxDecPicBuffering than a higher layer
        if (m_maxDecPicBuffering[i + 1] < m_maxDecPicBuffering[i])
        {
            m_maxDecPicBuffering[i + 1] = m_maxDecPicBuffering[i];
        }
    }

    // the value of num_reorder_pics[ i ] shall be in the range of 0 to max_dec_pic_buffering[ i ] -  1, inclusive
    if (m_numReorderPics[MAX_TLAYER - 1] > m_maxDecPicBuffering[MAX_TLAYER - 1] - 1)
    {
        m_maxDecPicBuffering[MAX_TLAYER - 1] = m_numReorderPics[MAX_TLAYER - 1] + 1;
    }

    if (m_vuiParametersPresentFlag && m_bitstreamRestrictionFlag)
    {
        Int PicSizeInSamplesY =  iSourceWidth * iSourceHeight;
        if (iWaveFrontSynchro)
        {
            m_minSpatialSegmentationIdc = 4 * PicSizeInSamplesY / ((2 * iSourceHeight + iSourceWidth) * uiMaxCUSize) - 4;
        }
        else
        {
            m_minSpatialSegmentationIdc = 0;
        }
    }

    xConfirmPara(m_decodedPictureHashSEIEnabled < 0 || m_decodedPictureHashSEIEnabled > 3, "this hash type is not correct!\n");

#undef xConfirmPara
    if (check_failed)
    {
        exit(EXIT_FAILURE);
    }
}

//! \}
