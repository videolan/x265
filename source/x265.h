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

/* Application developers planning to link against a shared library version of
 * libx265 from a Microsoft Visual Studio or similar development environment
 * will need to define X265_API_IMPORTS before including this header.
 * This clause does not apply to MinGW, similar development environments, or non
 * Windows platforms. */
#ifdef X265_API_IMPORTS
#define X265_API __declspec(dllimport)
#else
#define X265_API
#endif

#if __cplusplus
extern "C" {
#endif

/* x265_t:
 *      opaque handler for encoder */
typedef struct x265_t x265_t;

/* x265_nal_t:
 *      TBD */
typedef struct x265_nal_t x265_nal_t;

typedef struct x265_picture_t
{
    void *planes[3];

    int   stride[3];

    int   bitDepth;
} x265_picture_t;

typedef enum X265_ME_METHODS
{
    X265_DIA_SEARCH,
    X265_HEX_SEARCH,
    X265_UMH_SEARCH,
    X265_HM_SEARCH,  // adapted HM fast-ME method
};

static const char * const x265_motion_est_names[] = { "dia", "hex", "umh", "hm", 0 };

typedef struct x265_param_t
{
    // coding tools (bit-depth)
    int       m_internalBitDepth;                 ///< bit-depth codec operates at

    // source specification
    int       m_iFrameRate;                       ///< source frame-rates (Hz)
    int       m_iSourceWidth;                     ///< source width in pixel
    int       m_iSourceHeight;                    ///< source height in pixel

    // coding unit (CU) definition
    uint32_t  m_uiMaxCUWidth;                     ///< max. CU width in pixel
    uint32_t  m_uiMaxCUHeight;                    ///< max. CU height in pixel
    uint32_t  m_uiMaxCUDepth;                     ///< max. CU depth

    // transform unit (TU) definition
    uint32_t  m_uiQuadtreeTULog2MaxSize;
    uint32_t  m_uiQuadtreeTULog2MinSize;

    uint32_t  m_uiQuadtreeTUMaxDepthInter;
    uint32_t  m_uiQuadtreeTUMaxDepthIntra;

    // coding structure
    int       m_iIntraPeriod;                     ///< period of I-slice (random access period)
    int       m_iDecodingRefreshType;             ///< random access type
    int       m_iGOPSize;                         ///< GOP size of hierarchical structure
    int       m_extraRPSs;                        ///< extra RPSs added to handle CRA

    int       m_useTransformSkip;                 ///< flag for enabling intra transform skipping
    int       m_useTransformSkipFast;             ///< flag for enabling fast intra transform skipping
    int       m_enableAMP;                        ///< flag for enabling asymmetrical motion predictions
    int       m_enableAMPRefine;                  ///< mis-named, disables rectangular modes 2NxN, Nx2N

    // coding quality
    int       m_iQP;                              ///< QP value of key-picture (integer)
    int       m_iMaxCuDQPDepth;                   ///< Max. depth for a minimum CuDQPSize (0:default)

    int       m_cbQpOffset;                       ///< Chroma Cb QP Offset (0:default)
    int       m_crQpOffset;                       ///< Chroma Cr QP Offset (0:default)

    int       m_bUseAdaptQpSelect;                ///< TODO: What does this flag enable?
    int       m_bUseAdaptiveQP;                   ///< Flag for enabling QP adaptation based on a psycho-visual model
    int       m_iQPAdaptationRange;               ///< dQP range by QP adaptation

    // coding tools (PCM bit-depth)
    int       m_bPCMInputBitDepthFlag;            ///< 0: PCM bit-depth is internal bit-depth. 1: PCM bit-depth is input bit-depth.

    // coding tool (lossless)
    int       m_useLossless;                      ///< flag for using lossless coding
    int       m_bUseSAO;                          ///< Enable SAO filter
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
    int       m_useRDOQ;                          ///< flag for using RD optimized quantization
    int       m_useRDOQTS;                        ///< flag for using RD optimized quantization for transform skip
    int       m_rdPenalty;                        ///< RD-penalty for 32x32 TU for intra in non-intra slices (0: no RD-penalty, 1: RD-penalty, 2: maximum RD-penalty)
    int       m_signHideFlag;
    int       m_useFastDecisionForMerge;          ///< flag for using Fast Decision Merge RD-Cost
    int       m_bUseCbfFastMode;                  ///< flag for using Cbf Fast PU Mode Decision
    int       m_useEarlySkipDetection;            ///< flag for using Early SKIP Detection

    int       m_searchMethod;                     ///< ME search method (DIA, HEX, UMH, HM)
    int       m_iSearchRange;                     ///< ME search range
    int       m_bipredSearchRange;                ///< ME search range for bipred refinement

    int       m_iWaveFrontSynchro;                ///< 0: no WPP. >= 1: WPP is enabled, the "Top right" from which inheritance occurs is this LCU offset in the line above the current.

    int       m_bUseConstrainedIntraPred;         ///< flag for using constrained intra prediction

    // weighted prediction
    int       m_useWeightedPred;                  ///< Use of weighted prediction in P slices
    int       m_useWeightedBiPred;                ///< Use of bi-directional weighted prediction in B slices

    uint32_t  m_log2ParallelMergeLevel;           ///< Parallel merge estimation region
    uint32_t  m_maxNumMergeCand;                  ///< Max number of merge candidates

    int       m_TMVPModeId;                       ///< TMVP mode 0: TMVP disabled for all slices. 1: TMVP enabled for all slices (default) 2: TMVP enabled for certain slices only

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
    int       m_log2MaxMvLengthVertical;          ///< Indicate the maximum absolute value of a decoded vertical MV component in quarter-pel luma units
}
x265_param_t;

/***
 * Pass cpuid 0 to auto-detect.  If not called, first encoder allocated will
 * auto-detect the CPU and initialize performance primitives, which are process global */
void x265_init_primitives( int cpuid );

/***
 * Call to explicitly set the number of threads x265 allocates for its thread pool.
 * The thread pool is a singleton resource for the process and the first time an
 * encoder is created it will allocate a default thread pool if necessary.  Default
 * is one thread per CPU core (counting hyper-threading). */
void x265_init_threading( int threadcount );

/***
 * Initialize an x265_param_t structure to default values
 */
void x265_default_params( x265_param_t *param );

/* x265_param_apply_profile:
 *      Applies the restrictions of the given profile. (one of below) */
static const char * const x265_profile_names[] = { "main", "main10", "mainstillpicture", 0 };

/*      (can be NULL, in which case the function will do nothing)
 *      returns 0 on success, negative on failure (e.g. invalid profile name). */
int x265_param_apply_profile( x265_param_t *, const char *profile );

/* x265_bit_depth:
 *      Specifies the number of bits per pixel that x265 uses. This is also the
 *      max bit depth that x265 encodes in. x265 will accept pixel bit depths up
 *      to x265_bit_depth. m_internalBitDepth can be either 8 or x265_bit_depth.
 *      x265_picture_t.bitDepth may be 8 or x265_bit_depth */
X265_API extern const int x265_bit_depth;

/* x265_encoder_open:
 *      create a new encoder handler, all parameters from x265_param_t are copied */
x265_t *x265_encoder_open( x265_param_t * );

/* x265_encoder_encode:
 *      encode one picture.
 *      *pi_nal is the number of NAL units outputted in pp_nal.
 *      returns negative on error, zero if no NAL units returned.
 *      the payloads of all output NALs are guaranteed to be sequential in memory. */
int     x265_encoder_encode( x265_t *, x265_nal_t **pp_nal, int *pi_nal, x265_picture_t *pic_in );

/* x265_encoder_close:
 *      close an encoder handler */
void    x265_encoder_close( x265_t * );

/***
 * Release library static allocations
 */
void x265_cleanup(void);

#if __cplusplus
};
#endif

#endif // _X265_H_
