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

/* x265_t:
 *      opaque handler for encoder */
typedef struct x265_t x265_t;

// TODO: Existing names used for the different NAL unit types can be altered to better reflect the names in the spec.
//       However, the names in the spec are not yet stable at this point. Once the names are stable, a cleanup
//       effort can be done without use of macros to alter the names used to indicate the different NAL unit types.
typedef enum
{
    NAL_UNIT_CODED_SLICE_TRAIL_N = 0, // 0
    NAL_UNIT_CODED_SLICE_TRAIL_R,   // 1

    NAL_UNIT_CODED_SLICE_TSA_N,     // 2
    NAL_UNIT_CODED_SLICE_TLA_R,     // 3

    NAL_UNIT_CODED_SLICE_STSA_N,    // 4
    NAL_UNIT_CODED_SLICE_STSA_R,    // 5

    NAL_UNIT_CODED_SLICE_RADL_N,    // 6
    NAL_UNIT_CODED_SLICE_RADL_R,    // 7

    NAL_UNIT_CODED_SLICE_RASL_N,    // 8
    NAL_UNIT_CODED_SLICE_RASL_R,    // 9

    NAL_UNIT_RESERVED_VCL_N10,
    NAL_UNIT_RESERVED_VCL_R11,
    NAL_UNIT_RESERVED_VCL_N12,
    NAL_UNIT_RESERVED_VCL_R13,
    NAL_UNIT_RESERVED_VCL_N14,
    NAL_UNIT_RESERVED_VCL_R15,

    NAL_UNIT_CODED_SLICE_BLA_W_LP,  // 16
    NAL_UNIT_CODED_SLICE_BLA_W_RADL, // 17
    NAL_UNIT_CODED_SLICE_BLA_N_LP,  // 18
    NAL_UNIT_CODED_SLICE_IDR_W_RADL, // 19
    NAL_UNIT_CODED_SLICE_IDR_N_LP,  // 20
    NAL_UNIT_CODED_SLICE_CRA,       // 21
    NAL_UNIT_RESERVED_IRAP_VCL22,
    NAL_UNIT_RESERVED_IRAP_VCL23,

    NAL_UNIT_RESERVED_VCL24,
    NAL_UNIT_RESERVED_VCL25,
    NAL_UNIT_RESERVED_VCL26,
    NAL_UNIT_RESERVED_VCL27,
    NAL_UNIT_RESERVED_VCL28,
    NAL_UNIT_RESERVED_VCL29,
    NAL_UNIT_RESERVED_VCL30,
    NAL_UNIT_RESERVED_VCL31,

    NAL_UNIT_VPS,                   // 32
    NAL_UNIT_SPS,                   // 33
    NAL_UNIT_PPS,                   // 34
    NAL_UNIT_ACCESS_UNIT_DELIMITER, // 35
    NAL_UNIT_EOS,                   // 36
    NAL_UNIT_EOB,                   // 37
    NAL_UNIT_FILLER_DATA,           // 38
    NAL_UNIT_PREFIX_SEI,            // 39
    NAL_UNIT_SUFFIX_SEI,            // 40
    NAL_UNIT_RESERVED_NVCL41,
    NAL_UNIT_RESERVED_NVCL42,
    NAL_UNIT_RESERVED_NVCL43,
    NAL_UNIT_RESERVED_NVCL44,
    NAL_UNIT_RESERVED_NVCL45,
    NAL_UNIT_RESERVED_NVCL46,
    NAL_UNIT_RESERVED_NVCL47,
    NAL_UNIT_UNSPECIFIED_48,
    NAL_UNIT_UNSPECIFIED_49,
    NAL_UNIT_UNSPECIFIED_50,
    NAL_UNIT_UNSPECIFIED_51,
    NAL_UNIT_UNSPECIFIED_52,
    NAL_UNIT_UNSPECIFIED_53,
    NAL_UNIT_UNSPECIFIED_54,
    NAL_UNIT_UNSPECIFIED_55,
    NAL_UNIT_UNSPECIFIED_56,
    NAL_UNIT_UNSPECIFIED_57,
    NAL_UNIT_UNSPECIFIED_58,
    NAL_UNIT_UNSPECIFIED_59,
    NAL_UNIT_UNSPECIFIED_60,
    NAL_UNIT_UNSPECIFIED_61,
    NAL_UNIT_UNSPECIFIED_62,
    NAL_UNIT_UNSPECIFIED_63,
    NAL_UNIT_INVALID,
}
NalUnitType;

/* The data within the payload is already NAL-encapsulated; the type
 * is merely in the struct for easy access by the calling application.
 * All data returned in an x265_nal_t, including the data in p_payload, is no longer
 * valid after the next call to x265_encoder_encode.  Thus it must be used or copied
 * before calling x265_encoder_encode again. */
typedef struct
{
    int     i_type;      /* NalUnitType */
    int     i_payload;   /* size in bytes */
    uint8_t *p_payload;
}
x265_nal_t;

typedef struct x265_picture_t
{
    void *planes[3];
    int   stride[3];
    int   bitDepth;
    int   poc;
}
x265_picture_t;

typedef enum
{
    X265_DIA_SEARCH,
    X265_HEX_SEARCH,
    X265_UMH_SEARCH,
    X265_STAR_SEARCH,
    X265_FULL_SEARCH
}
X265_ME_METHODS;

static const char * const x265_motion_est_names[] = { "dia", "hex", "umh", "star", "full", 0 };

/* Log level */
#define X265_LOG_NONE          (-1)
#define X265_LOG_ERROR          0
#define X265_LOG_WARNING        1
#define X265_LOG_INFO           2
#define X265_LOG_DEBUG          3

#define X265_B_ADAPT_NONE       0
#define X265_B_ADAPT_FAST       1
#define X265_B_ADAPT_TRELLIS    2

typedef struct x265_param_t
{
    int       logLevel;
    int       bEnableWavefront;                ///< enable wavefront parallel processing
    int       poolNumThreads;                  ///< number of threads to allocate for thread pool

    int       internalBitDepth;                ///< bit-depth the codec operates at

    // source specification
    int       frameRate;                       ///< source frame-rate in Hz
    int       sourceWidth;                     ///< source width in pixels
    int       sourceHeight;                    ///< source height in pixels

    // coding unit (CU) definition
    uint32_t  maxCUSize;                       ///< max. CU width and height in pixels

    uint32_t  tuQTMaxInterDepth;               ///< amount the TU is allow to recurse beyond the inter PU depth
    uint32_t  tuQTMaxIntraDepth;               ///< amount the TU is allow to recurse beyond the intra PU depth

    // coding structure
    int       decodingRefreshType;             ///< Intra refresh type (0:none, 1:CDR, 2:IDR) default: 1
    int       keyframeMin;                     ///< Minimum intra period in frames
    int       keyframeMax;                     ///< Maximum intra period in frames
    int       bOpenGOP;                        ///< Only use I for first frame
    int       bframes;                         ///< Max number of consecutive B-frames
    int       lookaheadDepth;                  ///< Number of frames to use for lookahead, determines encoder latency
    int       bFrameAdaptive;                  ///< 0 - none, 1 - fast, 2 - full (trellis) adaptive B frame scheduling
    int       bFrameBias;

    // Intra coding tools
    int       bEnableConstrainedIntra;         ///< enable constrained intra prediction (ignore inter predicted reference samples)
    int       bEnableStrongIntraSmoothing;     ///< enable strong intra smoothing for 32x32 blocks where the reference samples are flat

    // Inter coding tools
    int       searchMethod;                    ///< ME search method (DIA, HEX, UMH, STAR, FULL)
    int       searchRange;                     ///< ME search range
    int       bipredSearchRange;               ///< ME search range for bipred refinement
    uint32_t  maxNumMergeCand;                 ///< Max number of merge candidates
    int       bEnableWeightedPred;             ///< enable weighted prediction in P slices
    int       bEnableWeightedBiPred;           ///< enable bi-directional weighted prediction in B slices

    int       bEnableAMP;                      ///< enable asymmetrical motion predictions
    int       bEnableRectInter;                ///< enable rectangular inter modes 2NxN, Nx2N
    int       bEnableCbfFastMode;              ///< enable use of Cbf flags for fast mode decision
    int       bEnableEarlySkip;                ///< enable early skip (merge) detection
    int       bEnableRDO;                      ///< enable full rate distortion optimization
    int       bEnableRDOQ;                     ///< enable RD optimized quantization
    int       bEnableSignHiding;               ///< enable hiding one sign bit per TU via implicit signaling
    int       bEnableTransformSkip;            ///< enable intra transform skipping
    int       bEnableTSkipFast;                ///< enable fast intra transform skipping
    int       bEnableRDOQTS;                   ///< enable RD optimized quantization when transform skip is selected

    // loop filter
    int       bEnableLoopFilter;               ///< enable Loop Filter

    // SAO loop filter
    int       bEnableSAO;                      ///< enable SAO filter
    int       saoLcuBoundary;                  ///< SAO parameter estimation using non-deblocked pixels for LCU bottom and right boundary areas
    int       saoLcuBasedOptimization;         ///< SAO LCU-based optimization

    // coding quality
    int       qp;                              ///< QP value of key-picture (integer)
    int       cbQpOffset;                      ///< Chroma Cb QP Offset (0:default)
    int       crQpOffset;                      ///< Chroma Cr QP Offset (0:default)
    int       rdPenalty;                       ///< RD-penalty for 32x32 TU for intra in non-intra slices (0: no RD-penalty, 1: RD-penalty, 2: maximum RD-penalty)

    // debugging
    int       bEnableDecodedPictureHashSEI;    ///< Checksum(3)/CRC(2)/MD5(1)/disable(0) acting on decoded picture hash SEI message
}
x265_param_t;

#define X265_CPU_LEVEL_AUTO  0
#define X265_CPU_LEVEL_NONE  1 // C code only, no SIMD
#define X265_CPU_LEVEL_SSE2  2
#define X265_CPU_LEVEL_SSE3  3
#define X265_CPU_LEVEL_SSSE3 4
#define X265_CPU_LEVEL_SSE41 5
#define X265_CPU_LEVEL_SSE42 6
#define X265_CPU_LEVEL_AVX   7
#define X265_CPU_LEVEL_AVX2  8

/*** 
 * If not called, first encoder allocated will auto-detect the CPU and
 * initialize performance primitives, which are process global */
void x265_setup_primitives(x265_param_t *param, int cpulevel);

/***
 * Initialize an x265_param_t structure to default values
 */
void x265_param_default(x265_param_t *param);

/* x265_param_apply_profile:
 *      Applies the restrictions of the given profile. (one of below) */
static const char * const x265_profile_names[] = { "main", "main10", "mainstillpicture", 0 };

/*      (can be NULL, in which case the function will do nothing)
 *      returns 0 on success, negative on failure (e.g. invalid profile name). */
int x265_param_apply_profile(x265_param_t *, const char *profile);

/* x265_max_bit_depth:
 *      Specifies the maximum number of bits per pixel that x265 can input. This
 *      is also the max bit depth that x265 encodes in.  When x265_max_bit_depth
 *      is 8, the internal and input bit depths must be 8.  When
 *      x265_max_bit_depth is 12, the internal and input bit depths can be
 *      either 8, 10, or 12. Note that the internal bit depth must be the same
 *      for all encoders allocated in the same process. */
extern const int x265_max_bit_depth;

/* x265_encoder_open:
 *      create a new encoder handler, all parameters from x265_param_t are copied */
x265_t *x265_encoder_open(x265_param_t *);

/* x265_encoder_headers:
 *      return the SPS and PPS that will be used for the whole stream.
 *      *pi_nal is the number of NAL units outputted in pp_nal.
 *      returns negative on error.
 *      the payloads of all output NALs are guaranteed to be sequential in memory. */
int     x265_encoder_headers(x265_t *, x265_nal_t **pp_nal, int *pi_nal);

/* x265_encoder_encode:
 *      encode one picture.
 *      *pi_nal is the number of NAL units outputted in pp_nal.
 *      returns negative on error, zero if no NAL units returned.
 *      the payloads of all output NALs are guaranteed to be sequential in memory. */
int     x265_encoder_encode(x265_t *encoder, x265_nal_t **pp_nal, int *pi_nal, x265_picture_t *pic_in, x265_picture_t **pic_out);

/* x265_encoder_close:
 *      close an encoder handler.  Optionally return the global PSNR value (6 * psnrY + psnrU + psnrV) / 8 */
void    x265_encoder_close(x265_t *, double *globalPsnr);

/***
 * Release library static allocations
 */
void x265_cleanup(void);

#if __cplusplus
}
#endif

#endif // _X265_H_
