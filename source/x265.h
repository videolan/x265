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

#ifndef X265_H
#define X265_H

#include <stdint.h>
#include "x265_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* x265_encoder:
 *      opaque handler for encoder */
typedef struct x265_encoder x265_encoder;

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

typedef enum
{
    NAL_UNIT_CODED_SLICE_TRAIL_N = 0,
    NAL_UNIT_CODED_SLICE_TRAIL_R,
    NAL_UNIT_CODED_SLICE_TSA_N,
    NAL_UNIT_CODED_SLICE_TLA_R,
    NAL_UNIT_CODED_SLICE_STSA_N,
    NAL_UNIT_CODED_SLICE_STSA_R,
    NAL_UNIT_CODED_SLICE_RADL_N,
    NAL_UNIT_CODED_SLICE_RADL_R,
    NAL_UNIT_CODED_SLICE_RASL_N,
    NAL_UNIT_CODED_SLICE_RASL_R,
    NAL_UNIT_CODED_SLICE_BLA_W_LP = 16,
    NAL_UNIT_CODED_SLICE_BLA_W_RADL,
    NAL_UNIT_CODED_SLICE_BLA_N_LP,
    NAL_UNIT_CODED_SLICE_IDR_W_RADL,
    NAL_UNIT_CODED_SLICE_IDR_N_LP,
    NAL_UNIT_CODED_SLICE_CRA,
    NAL_UNIT_VPS = 32,
    NAL_UNIT_SPS,
    NAL_UNIT_PPS,
    NAL_UNIT_ACCESS_UNIT_DELIMITER,
    NAL_UNIT_EOS,
    NAL_UNIT_EOB,
    NAL_UNIT_FILLER_DATA,
    NAL_UNIT_PREFIX_SEI,
    NAL_UNIT_SUFFIX_SEI,
    NAL_UNIT_INVALID = 64,
} NalUnitType;

/* The data within the payload is already NAL-encapsulated; the type is merely
 * in the struct for easy access by the calling application.  All data returned
 * in an x265_nal, including the data in payload, is no longer valid after the
 * next call to x265_encoder_encode.  Thus it must be used or copied before
 * calling x265_encoder_encode again. */
typedef struct x265_nal
{
    uint32_t type;        /* NalUnitType */
    uint32_t sizeBytes;   /* size in bytes */
    uint8_t* payload;
} x265_nal;

/* Used to pass pictures into the encoder, and to get picture data back out of
 * the encoder.  The input and output semantics are different */
typedef struct x265_picture
{
    /* Must be specified on input pictures, the number of planes is determined
     * by the colorSpace value */
    void*   planes[3];

    /* Stride is the number of bytes between row starts */
    int     stride[3];

    /* Must be specified on input pictures. x265_picture_init() will set it to
     * the encoder's internal bit depth, but this field must describe the depth
     * of the input pictures. Must be between 8 and 16. Values larger than 8
     * imply 16bits per input sample. If input bit depth is larger than the
     * internal bit depth, the encoder will down-shift pixels. Input samples
     * larger than 8bits will be masked to internal bit depth. On output the
     * bitDepth will be the internal encoder bit depth */
    int     bitDepth;

    /* Must be specified on input pictures: X265_TYPE_AUTO or other.
     * x265_picture_init() sets this to auto, returned on output */
    int     sliceType;

    /* Ignored on input, set to picture count, returned on output */
    int     poc;

    /* Must be specified on input pictures: X265_CSP_I420 or other. It must
     * match the internal color space of the encoder. x265_picture_init() will
     * initialize this value to the internal color space */
    int     colorSpace;

    /* presentation time stamp: user-specified, returned on output */
    int64_t pts;

    /* display time stamp: ignored on input, copied from reordered pts. Returned
     * on output */
    int64_t dts;

    /* The value provided on input is returned with the same picture (POC) on
     * output */
    void*   userData;

    /* new data members to this structure must be added to the end so that
     * users of x265_picture_alloc/free() can be assured of future safety */
} x265_picture;

typedef enum
{
    X265_DIA_SEARCH,
    X265_HEX_SEARCH,
    X265_UMH_SEARCH,
    X265_STAR_SEARCH,
    X265_FULL_SEARCH
} X265_ME_METHODS;

/* CPU flags */

/* x86 */
#define X265_CPU_CMOV            0x0000001
#define X265_CPU_MMX             0x0000002
#define X265_CPU_MMX2            0x0000004  /* MMX2 aka MMXEXT aka ISSE */
#define X265_CPU_MMXEXT          X265_CPU_MMX2
#define X265_CPU_SSE             0x0000008
#define X265_CPU_SSE2            0x0000010
#define X265_CPU_SSE3            0x0000020
#define X265_CPU_SSSE3           0x0000040
#define X265_CPU_SSE4            0x0000080  /* SSE4.1 */
#define X265_CPU_SSE42           0x0000100  /* SSE4.2 */
#define X265_CPU_LZCNT           0x0000200  /* Phenom support for "leading zero count" instruction. */
#define X265_CPU_AVX             0x0000400  /* AVX support: requires OS support even if YMM registers aren't used. */
#define X265_CPU_XOP             0x0000800  /* AMD XOP */
#define X265_CPU_FMA4            0x0001000  /* AMD FMA4 */
#define X265_CPU_AVX2            0x0002000  /* AVX2 */
#define X265_CPU_FMA3            0x0004000  /* Intel FMA3 */
#define X265_CPU_BMI1            0x0008000  /* BMI1 */
#define X265_CPU_BMI2            0x0010000  /* BMI2 */
/* x86 modifiers */
#define X265_CPU_CACHELINE_32    0x0020000  /* avoid memory loads that span the border between two cachelines */
#define X265_CPU_CACHELINE_64    0x0040000  /* 32/64 is the size of a cacheline in bytes */
#define X265_CPU_SSE2_IS_SLOW    0x0080000  /* avoid most SSE2 functions on Athlon64 */
#define X265_CPU_SSE2_IS_FAST    0x0100000  /* a few functions are only faster on Core2 and Phenom */
#define X265_CPU_SLOW_SHUFFLE    0x0200000  /* The Conroe has a slow shuffle unit (relative to overall SSE performance) */
#define X265_CPU_STACK_MOD4      0x0400000  /* if stack is only mod4 and not mod16 */
#define X265_CPU_SLOW_CTZ        0x0800000  /* BSR/BSF x86 instructions are really slow on some CPUs */
#define X265_CPU_SLOW_ATOM       0x1000000  /* The Atom is terrible: slow SSE unaligned loads, slow
                                             * SIMD multiplies, slow SIMD variable shifts, slow pshufb,
                                             * cacheline split penalties -- gather everything here that
                                             * isn't shared by other CPUs to avoid making half a dozen
                                             * new SLOW flags. */
#define X265_CPU_SLOW_PSHUFB     0x2000000  /* such as on the Intel Atom */
#define X265_CPU_SLOW_PALIGNR    0x4000000  /* such as on the AMD Bobcat */

/* ARM */
#define X265_CPU_ARMV6           0x0000001
#define X265_CPU_NEON            0x0000002  /* ARM NEON */
#define X265_CPU_FAST_NEON_MRC   0x0000004  /* Transfer from NEON to ARM register is fast (Cortex-A9) */

#define X265_MAX_SUBPEL_LEVEL   7

/* Log level */
#define X265_LOG_NONE          (-1)
#define X265_LOG_ERROR          0
#define X265_LOG_WARNING        1
#define X265_LOG_INFO           2
#define X265_LOG_DEBUG          3
#define X265_LOG_FULL           4

#define X265_B_ADAPT_NONE       0
#define X265_B_ADAPT_FAST       1
#define X265_B_ADAPT_TRELLIS    2

#define X265_BFRAME_MAX         16

#define X265_TYPE_AUTO          0x0000  /* Let x265 choose the right type */
#define X265_TYPE_IDR           0x0001
#define X265_TYPE_I             0x0002
#define X265_TYPE_P             0x0003
#define X265_TYPE_BREF          0x0004  /* Non-disposable B-frame */
#define X265_TYPE_B             0x0005

#define X265_AQ_NONE                 0
#define X265_AQ_VARIANCE             1
#define X265_AQ_AUTO_VARIANCE        2
#define IS_X265_TYPE_I(x) ((x) == X265_TYPE_I || (x) == X265_TYPE_IDR)
#define IS_X265_TYPE_B(x) ((x) == X265_TYPE_B || (x) == X265_TYPE_BREF)

/* NOTE! For this release only X265_CSP_I420 and X265_CSP_I444 are supported */

/* Supported internal color space types (according to semantics of chroma_format_idc) */
#define X265_CSP_I400           0  /* yuv 4:0:0 planar */
#define X265_CSP_I420           1  /* yuv 4:2:0 planar */
#define X265_CSP_I422           2  /* yuv 4:2:2 planar */
#define X265_CSP_I444           3  /* yuv 4:4:4 planar */
#define X265_CSP_COUNT          4  /* Number of supported internal color spaces */

/* These color spaces will eventually be supported as input pictures. The pictures will
 * be converted to the appropriate planar color spaces at ingest */
#define X265_CSP_NV12           4  /* yuv 4:2:0, with one y plane and one packed u+v */
#define X265_CSP_NV16           5  /* yuv 4:2:2, with one y plane and one packed u+v */

/* Interleaved color-spaces may eventually be supported as input pictures */
#define X265_CSP_BGR            6  /* packed bgr 24bits   */
#define X265_CSP_BGRA           7  /* packed bgr 32bits   */
#define X265_CSP_RGB            8  /* packed rgb 24bits   */
#define X265_CSP_MAX            9  /* end of list */

#define X265_EXTENDED_SAR       255 /* aspect ratio explicitly specified as width:height */

typedef struct
{
    int planes;
    int width[3];
    int height[3];
} x265_cli_csp;

static const x265_cli_csp x265_cli_csps[] =
{
    { 1, { 0, 0, 0 }, { 0, 0, 0 } }, /* i400 */
    { 3, { 0, 1, 1 }, { 0, 1, 1 } }, /* i420 */
    { 3, { 0, 1, 1 }, { 0, 0, 0 } }, /* i422 */
    { 3, { 0, 0, 0 }, { 0, 0, 0 } }, /* i444 */
    { 2, { 0, 0 },    { 0, 1 } },    /* nv12 */
    { 2, { 0, 0 },    { 0, 0 } },    /* nv16 */
};

/* rate tolerance method */
typedef enum
{
    X265_RC_ABR,
    X265_RC_CQP,
    X265_RC_CRF
} X265_RC_METHODS;

/* Output statistics from encoder */
typedef struct x265_stats
{
    double    globalPsnrY;
    double    globalPsnrU;
    double    globalPsnrV;
    double    globalPsnr;
    double    globalSsim;
    double    elapsedEncodeTime;    /* wall time since encoder was opened */
    double    elapsedVideoTime;     /* encoded picture count / frame rate */
    double    bitrate;              /* accBits / elapsed video time */
    uint32_t  encodedPictureCount;  /* number of output pictures thus far */
    uint32_t  totalWPFrames;        /* number of uni-directional weighted frames used */
    uint64_t  accBits;              /* total bits output thus far */

    /* new statistic member variables must be added below this line */
} x265_stats;

/* String values accepted by x265_param_parse() (and CLI) for various parameters */
static const char * const x265_motion_est_names[] = { "dia", "hex", "umh", "star", "full", 0 };
static const char * const x265_source_csp_names[] = { "i400", "i420", "i422", "i444", "nv12", "nv16", 0 };
static const char * const x265_video_format_names[] = { "component", "pal", "ntsc", "secam", "mac", "undef", 0 };
static const char * const x265_fullrange_names[] = { "limited", "full", 0 };
static const char * const x265_colorprim_names[] = { "", "bt709", "undef", "", "bt470m", "bt470bg", "smpte170m", "smpte240m", "film", "bt2020", 0 };
static const char * const x265_transfer_names[] = { "", "bt709", "undef", "", "bt470m", "bt470bg", "smpte170m", "smpte240m", "linear", "log100",
                                                    "log316", "iec61966-2-4", "bt1361e", "iec61966-2-1", "bt2020-10", "bt2020-12", 0 };
static const char * const x265_colmatrix_names[] = { "GBR", "bt709", "undef", "", "fcc", "bt470bg", "smpte170m", "smpte240m",
                                                     "YCgCo", "bt2020nc", "bt2020c", 0 };
static const char * const x265_sar_names[] = { "undef", "1:1", "12:11", "10:11", "16:11", "40:33", "24:11", "20:11",
                                               "32:11", "80:33", "18:11", "15:11", "64:33", "160:99", "4:3", "3:2", "2:1", 0 };
static const char * const x265_interlace_names[] = { "prog", "tff", "bff", 0 };

/* x265 input parameters
 *
 * For version safety you may use x265_param_alloc/free() to manage the
 * allocation of x265_param instances, and x265_param_parse() to assign values
 * by name.  By never dereferencing param fields in your own code you can treat
 * x265_param as an opaque data structure */
typedef struct x265_param
{
    /*== Encoder Environment ==*/

    /* x265_param_default() will auto-detect this cpu capability bitmap.  it is
     * recommended to not change this value unless you know the cpu detection is
     * somehow flawed on your target hardware. The asm function tables are
     * process global, the first encoder configures them for all encoders */
    int       cpuid;

    /* Enable wavefront parallel processing, greatly increases parallelism for
     * less than 1% compression efficiency loss */
    int       bEnableWavefront;

    /* Number of threads to allocate for the process global thread pool, if no
     * thread pool has yet been created. 0 implies auto-detection. By default
     * x265 will try to allocate one worker thread per CPU core */
    int       poolNumThreads;

    /* Number of concurrently encoded frames, 0 implies auto-detection. By
     * default x265 will use a number of frame threads emperically determined to
     * be optimal for your CPU core count, between 2 and 6.  Using more than one
     * frame thread causes motion search in the down direction to be clamped but
     * otherwise encode behavior is unaffected. With CQP rate control the output
     * bitstream is deterministic for all values of frameNumThreads greater than
     * 1.  All other forms of rate-control can be negatively impacted by
     * increases to the number of frame threads because the extra concurrency
     * adds uncertainty to the bitrate estimations.  There is no limit to the
     * number of frame threads you use for each encoder, but frame parallelism
     * is generally limited by the the number of CU rows */
    int       frameNumThreads;

    /* The level of logging detail emitted by the encoder. X265_LOG_NONE to
     * X265_LOG_FULL, default is X265_LOG_INFO */
    int       logLevel;

    /* Enable the measurement and reporting of PSNR. Default is enabled */
    int       bEnablePsnr;

    /* Enable the measurement and reporting of SSIM. Default is disabled */
    int       bEnableSsim;

    /* filename of CSV log. If logLevel is X265_LOG_DEBUG, the encoder will emit
     * per-slice statistics to this log file in encode order. Otherwise the
     * encoder will emit per-stream statistics into the log file when
     * x265_encoder_log is called (presumably at the end of the encode) */
    const char *csvfn;

    /* Enable the generation of SEI messages for each encoded frame containing
     * the hashes of the three reconstructed picture planes. Most decoders will
     * validate those hashes against the reconstructed images it generates and
     * report any mismatches. This is essentially a debugging feature.  Hash
     * types are MD5(1), CRC(2), Checksum(3).  Default is 0, none */
    int       decodedPictureHashSEI;

    /*== Internal Picture Specification ==*/

    /* Internal encoder bit depth. If x265 was compiled to use 8bit pixels
     * (HIGH_BIT_DEPTH=0), this field must be 8, else this field must be 10.
     * Future builds may support 12bit pixels. */
    int       internalBitDepth;

    /* Color space of internal pictures. Only X265_CSP_I420 and X265_CSP_I444
     * are supported.  Eventually, i422 will also be supported as an internal
     * color space and other packed formats will be supported in
     * x265_picture.colorSpace */
    int       internalCsp;

    /* Numerator and denominator of frame rate */
    uint32_t  fpsNum;
    uint32_t  fpsDenom;

    /* Width (in pixels) of the source pictures. If this width is not an even
     * multiple of 4, the encoder will pad the pictures internally to meet this
     * minimum requirement. All valid HEVC widths are supported */
    int       sourceWidth;

    /* Height (in pixels) of the source pictures. If this height is not an even
     * multiple of 4, the encoder will pad the pictures internally to meet this
     * minimum requirement. All valid HEVC heights are supported */
    int       sourceHeight;

    /* Interlace type of source pictures. 0 - progressive pictures (default).
     * 1 - top field first, 2 - bottom field first. HEVC encodes interlaced
     * content as fields, they must be provided to the encoder in the correct
     * temporal order. EXPERIMENTAL */
    int       interlaceMode;

    /* Flag indicating whether VPS, SPS and PPS headers should be output with
     * each keyframe. Default false */
    int       bRepeatHeaders;

    /* Flag indicating whether the encoder should emit an Access Unit Delimiter
     * NAL at the start of every access unit. Default false */
    int       bEnableAccessUnitDelimiters;

    /*== Coding Unit (CU) definitions ==*/

    /* Maxiumum CU width and height in pixels.  The size must be 64, 32, or 16.
     * The higher the size, the more efficiently x265 can encode areas of low
     * complexity, greatly improving compression efficiency at large
     * resolutions.  The smaller the size, the more effective wavefront and
     * frame parallelism will become because of the increase in rows. default 64 */
    uint32_t  maxCUSize;

    /* The additional depth the residual quadtree is allowed to recurse beyond
     * the coding quadtree, for inter coded blocks. This must be between 1 and
     * 3. The higher the value the more efficiently the residual can be
     * compressed by the DCT transforms, at the expense of much more compute */
    uint32_t  tuQTMaxInterDepth;

    /* The additional depth the residual quadtree is allowed to recurse beyond
     * the coding quadtree, for intra coded blocks. This must be between 1 and
     * 3. The higher the value the more efficiently the residual can be
     * compressed by the DCT transforms, at the expense of much more compute */
    uint32_t  tuQTMaxIntraDepth;

    /*== GOP Structure and Lokoahead ==*/

    /* Enable open GOP - meaning I slices are not necessariy IDR and thus frames
     * encoded after an I slice may reference frames encoded prior to the I
     * frame which have remained in the decoded picture buffer.  Open GOP
     * generally has better compression efficiency and negligable encoder
     * performance impact, but the use case may preclude it.  Default true */
    int       bOpenGOP;

    /* Scenecuts closer together than this are coded as I, not IDR. */
    int       keyframeMin;

    /* Maximum keyframe distance or intra period in number of frames. If 0 or 1,
     * all frames are I frames. A negative value is casted to MAX_INT internally
     * which effectively makes frame 0 the only I frame. Default is 250 */
    int       keyframeMax;

    /* The maximum number of L0 references a P or B slice may use. This
     * influences the size of the decoded picture buffer. The higher this
     * number, the more reference frames there will be available for motion
     * search, improving compression efficiency of most video at a cost of
     * performance. Value must be between 1 and 16, default is 3 */
    int       maxNumReferences;

    /* Sets the operating mode of the lookahead.  With b-adapt 0, the GOP
     * structure is fixed based on the values of keyframeMax and bframes.
     * With b-adapt 1 a light lookahead is used to chose B frame placement.
     * With b-adapt 2 (trellis) a viterbi B path selection is performed */
    int       bFrameAdaptive;

    /* Maximum consecutive B frames that can be emitted by the lookehead. When
     * b-adapt is 0 and keyframMax is greater than bframes, the lookahead emits
     * a fixed pattern of `bframes` B frames between each P.  With b-adapt 1 the
     * lookahead ignores the value of bframes for the most part.  With b-adapt 2
     * the value of bframes determines the search (POC) distance performeed in
     * both directions, quadradically increasing the compute load of the
     * lookahead.  The higher the value, the more B frames the lookahead may
     * possibly use consecutively, usually improving compression. Default is 3,
     * maximum is 16 */
    int       bframes;

    /* When enabled, the encoder will use the B frame in the middle of each
     * mini-GOP larger than 2 B frames as a motion reference for the surrounding
     * B frames.  This improves compression efficiency for a small performance
     * penalty.  Referenced B frames are treated somewhere between a B and a P
     * frame by rate control.  Default is enabled. */
    int       bBPyramid;

    /* The number of frames that must be queued in the lookahead before it may
     * make slice decisions. Increasing this value directly increases the encode
     * latency. The longer the queue the more optimally the lookahead may make
     * slice decisions, particularly with b-adapt 2. When mb-tree is enabled,
     * the length of the queue linearly increases the effectiveness of the
     * mb-tree analysis. Default is 40 frames, maximum is 250 */
    int       lookaheadDepth;

    /* A value which is added to the cost estimate of B frames in the lookahead.
     * It may be a positive value (making B frames appear more expensive, which
     * causes the lookahead to chose more P frames) or negative, which makes the
     * lookahead chose more B frames. Default is 0, there are no limits */
    int       bFrameBias;

    /* An arbitrary threshold which determines how agressively the lookahead
     * should detect scene cuts. The default (40) is recommended. */
    int       scenecutThreshold;

    /*== Intra Coding Tools ==*/

    /* Enable constrained intra prediction. This causes intra prediction to
     * input samples that were inter predicted. For some use cases this is
     * believed to me more robust to stream errors, but it has a compression
     * penalty on P and (particularly) B slices. Defaults to diabled */
    int       bEnableConstrainedIntra;

    /* Enable strong intra smoothing for 32x32 blocks where the reference
     * samples are flat. It may or may not improve compression efficiency,
     * depending on your source material. Defaults to disabled */
    int       bEnableStrongIntraSmoothing;

    /*== Inter Coding Tools ==*/

    /* ME search method (DIA, HEX, UMH, STAR, FULL). The search patterns
     * (methods) are sorted in increasing complexity, with diamond being the
     * simplest and fastest and full being the slowest.  DIA, HEX, and UMH were
     * adapted from x264 directly. STAR is an adaption of the HEVC reference
     * encoder's three step search, while full is a naive exhaustive search. The
     * default is the star search, it has a good balance of performance and
     * compression efficiecy */
    int       searchMethod;

    /* A value between 0 and X265_MAX_SUBPEL_LEVEL which adjusts the amount of
     * effort performed during subpel refine. Default is 5 */
    int       subpelRefine;

    /* The maximum distance from the motion prediction that the full pel motion
     * search is allowed to progress before terminating. This value can have an
     * effect on frame parallelism, as referenced frames must be at least this
     * many rows of reconstructed pixels ahead of the referencee at all times.
     * (When considering reference lag, the motion prediction must be ignored
     * because it cannot be known ahead of time).  Default is 60, which is the
     * default max CU size (64) minus the luma HPEL half-filter length (4). If a
     * smaller CU size is used, the search range should be similarly reduced */
    int       searchRange;

    /* The maximum number of merge candidates that are considered during inter
     * analysis.  This number (between 1 and 5) is signaled in the stream
     * headers and determines the number of bits required to signal a merge so
     * it can have significant trade-offs. The smaller this number the higher
     * the performance but the less compression efficiency. Default is 3 */
    uint32_t  maxNumMergeCand;

    /* Enable weighted prediction in P slices.  This enables weighting analysis
     * in the lookahead, which influences slice decisions, and enables weighting
     * analysis in the main encoder which allows P reference samples to have a
     * weight function applied to them prior to using them for motion
     * compensation.  In video which has lighting changes, it can give a large
     * improvement in compression efficiency. Default is enabled */
    int       bEnableWeightedPred;

    /* Enable weighted prediction in B slices. Default is disabled */
    int       bEnableWeightedBiPred;

    /*== Analysis tools ==*/

    /* Enable asymmetrical motion predictions.  At CU depths 64, 32, and 16, it
     * is possible to use 25%/75% split partitions in the up, down, right, left
     * directions. For some material this can improve compression efficiency at
     * the cost of extra analysis. bEnableRectInter must be enabled for this
     * feature to be used. Default enabled */
    int       bEnableAMP;

    /* Enable rectangular motion prediction partitions (vertical and
     * horizontal), available at all CU depths from 64x64 to 8x8. Default is
     * enabled */
    int       bEnableRectInter;

    /* Enable the use of `coded block flags` (flags set to true when a residual
     * has been coded for a given block) to avoid intra analysis in likely skip
     * blocks. Default is disabled */
    int       bEnableCbfFastMode;

    /* Enable early skip decisions to avoid intra and inter analysis in likely
     * skip blocks. Default is disabled */
    int       bEnableEarlySkip;

    /* Apply an optional penalty to the estimated cost of 32x32 intra blocks in
     * non-intra slices. 0 is disabled, 1 enables a small penalty, and 2 enables
     * a full penalty. This favors inter-coding and its low bitrate over
     * potential increases in distortion, but usually improves performance.
     * Default is 0 */
    int       rdPenalty;

    /* A value betwen X265_NO_RDO_NO_RDOQ and X265_RDO_LEVEL which determines
     * the level of rate distortion optimizations to perform during mode
     * decisions and quantization. The more RDO the better the compression
     * efficiency at a major cost of performance. Default is no RDO (0) */
    int       rdLevel;

    /*== Coding tools ==*/

    /* Enable the implicit signaling of the sign bit of the last coefficient of
     * each transform unit. This saves one bit per TU at the expense of figuring
     * out which coefficient can be toggled with the least distortion.
     * Default is enabled */
    int       bEnableSignHiding;

    /* Allow intra coded blocks to be encoded directly as residual without the
     * DCT transform, when this improves efficiency. Checking whether the block
     * will benefit from this option incurs a performance penalty. Default is
     * enabled */
    int       bEnableTransformSkip;

    /* Enable a faster determination of whether skippig the DCT transform will
     * be beneficial. Slight performance gain for some compression loss. Default
     * is enabled */
    int       bEnableTSkipFast;

    /* Enable the deblocking loop filter, which improves visual quality by
     * reducing blocking effects at block edges, particularly at lower bitrates
     * or higher QP. When enabled it adds another CU row of reference lag,
     * reducing frame parallelism effectiveness.  Default is enabled */
    int       bEnableLoopFilter;

    /* Enable the Sample Adaptive Offset loop filter, which reduces distortion
     * effects by adjusting reconstructed sample values based on histogram
     * analysis to better approximate the original samples. When enabled it adds
     * a CU row of reference lag, reducing frame parallelism effectiveness.
     * Default is enabled */
    int       bEnableSAO;

    /* Note: when deblocking and SAO are both enabled, the loop filter CU lag is
     * only one row, as they operate in series o the same row. */

    /* Select the method in which SAO deals with deblocking boundary pixels.  If
     * 0 the right and bottom boundary areas are skipped. If 1, non-deblocked
     * pixels are used entirely. Default is 0 */
    int       saoLcuBoundary;

    /* Select the scope of the SAO optimization. If 0 SAO is performed over the
     * entire output picture at once, this can severly restrict frame
     * parallelism so it is not recommended for many-core machines.  If 1 SAO is
     * performed on LCUs in series. Default is 1 */
    int       saoLcuBasedOptimization;

    /* Generally a small signed integer which offsets the QP used to quantize
     * the Cb chroma residual (delta from luma QP specified by rate-control).
     * Default is 0, which is recommended */
    int       cbQpOffset;

    /* Generally a small signed integer which offsets the QP used to quantize
     * the Cr chroma residual (delta from luma QP specified by rate-control).
     * Default is 0, which is recommended */
    int       crQpOffset;

    /*== Rate Control ==*/

    struct
    {
        /* Explicit mode of rate-control, necessary for API users. It must
         * be one of the X265_RC_METHODS enum values. */
        int       rateControlMode;

        /* Base QP to use for Constant QP rate control. Adaptive QP may alter
         * the QP used for each block. If a QP is specified on the command line
         * CQP rate control is implied. Default: 32 */
        int       qp;

        /* target bitrate for Average BitRate (ABR) rate control. If a non- zero
         * bitrate is specified on the command line, ABR is implied. Default 0 */
        int       bitrate;

        /* The degree of rate fluctuation that x265 tolerates. Rate tolerance is used
         * alongwith overflow (difference between actual and target bitrate), to adjust
           qp. Default is 1.0 */
        double    rateTolerance;

        /* qComp sets the quantizer curve compression factor. It weights the frame
         * quantizer based on the complexity of residual (measured by lookahead).
         * Default value is 0.6. Increasing it to 1 will effectively generate CQP */
        double    qCompress;

        /* QP offset between I/P and P/B frames. Default ipfactor: 1.4
         *  Default pbFactor: 1.3 */
        double    ipFactor;
        double    pbFactor;

        /* Max QP difference between frames. Default: 4 */
        int       qpStep;

        /* Ratefactor constant: targets a certain constant "quality".
         * Acceptable values between 0 and 51. Default value: 28 */
        double    rfConstant;

        /* Enable adaptive quantization. This mode distributes available bits between all
         * macroblocks of a frame, assigning more bits to low complexity areas. Turning
         * this ON will usually affect PSNR negatively, however SSIM and visual quality
         * generally improves. Default: X265_AQ_VARIANCE */
        int       aqMode;

        /* Sets the strength of AQ bias towards low detail macroblocks. Valid only if
         * AQ is enabled. Default value: 1.0. Acceptable values between 0.0 and 3.0 */
        double    aqStrength;

        /* Sets the maximum rate the VBV buffer should be assumed to refill at
         * Default is zero */
        int       vbvMaxBitrate;

        /* Sets the size of the VBV buffer in kilobits. Default is zero */
        int       vbvBufferSize;

        /* Sets how full the VBV buffer must be before playback starts. If it is less than
         * 1, then the initial fill is vbv-init * vbvBufferSize. Otherwise, it is
         * interpreted as the initial fill in kbits. Default is 0.9 */
        double    vbvBufferInit;

        /* Enable CUTree ratecontrol. This keeps track of the CUs that propagate temporally
         * across frames and assigns more bits to these CUs. Improves encode efficiency.
         * Default: enabled */
        int       cuTree;

        /* In CRF mode, maximum CRF as caused by VBV. 0 implies no limit */
        double    rfConstantMax;
    } rc;

    /*== Video Usability Information ==*/
    struct
    {
        /* Aspect ratio idc to be added to the VUI.  The default is 0 indicating
         * the apsect ratio is unspecified. If set to X265_EXTENDED_SAR then
         * sarWidth and sarHeight must also be set */
        int aspectRatioIdc;

        /* Sample Aspect Ratio width in arbitrary units to be added to the VUI
         * only if aspectRatioIdc is set to X265_EXTENDED_SAR.  This is the width
         * of an individual pixel. If this is set then sarHeight must also be set */
        int sarWidth;

        /* Sample Aspect Ratio height in arbitrary units to be added to the VUI.
         * only if aspectRatioIdc is set to X265_EXTENDED_SAR.  This is the width
         * of an individual pixel. If this is set then sarWidth must also be set */
        int sarHeight;

        /* Enable overscan info present flag in the VUI.  If this is set then
         * bEnabledOverscanAppropriateFlag will be added to the VUI. The default
         * is false */
        int bEnableOverscanInfoPresentFlag;

        /* Enable overscan appropriate flag.  The status of this flag is added
         * to the VUI only if bEnableOverscanInfoPresentFlag is set. If this
         * flag is set then cropped decoded pictures may be output for display.
         * The default is false */
        int bEnableOverscanAppropriateFlag;

        /* Video signal type present flag of the VUI.  If this is set then
         * videoFormat, bEnableVideoFullRangeFlag and
         * bEnableColorDescriptionPresentFlag will be added to the VUI. The
         * default is false */
        int bEnableVideoSignalTypePresentFlag;

        /* Video format of the source video.  0 = component, 1 = PAL, 2 = NTSC,
         * 3 = SECAM, 4 = MAC, 5 = unspecified video format is the default */
        int videoFormat;

        /* Video full range flag indicates the black level and range of the luma
         * and chroma signals as derived from E′Y, E′PB, and E′PR or E′R, E′G,
         * and E′B real-valued component signals. The default is false */
        int bEnableVideoFullRangeFlag;

        /* Color description present flag in the VUI. If this is set then
         * color_primaries, transfer_characteristics and matrix_coeffs are to be
         * added to the VUI. The default is false */
        int bEnableColorDescriptionPresentFlag;

        /* Color primaries holds the chromacity coordinates of the source
         * primaries. The default is 2 */
        int colorPrimaries;

        /* Transfer characteristics indicates the opto-electronic transfer
         * characteristic of the source picture. The default is 2 */
        int transferCharacteristics;

        /* Matrix coefficients used to derive the luma and chroma signals from
         * the red, blue and green primaries. The default is 2 */
        int matrixCoeffs;

        /* Chroma location info present flag adds chroma_sample_loc_type_top_field and
         * chroma_sample_loc_type_bottom_field to the VUI. The default is false */
        int bEnableChromaLocInfoPresentFlag;

        /* Chroma sample location type top field holds the chroma location in
         * the top field. The default is 0 */
        int chromaSampleLocTypeTopField;

        /* Chroma sample location type bottom field holds the chroma location in
         * the bottom field. The default is 0 */
        int chromaSampleLocTypeBottomField;

        /* Default display window flag adds def_disp_win_left_offset,
         * def_disp_win_right_offset, def_disp_win_top_offset and
         * def_disp_win_bottom_offset to the VUI. The default is false */
        int bEnableDefaultDisplayWindowFlag;

        /* Default display window left offset holds the left offset with the
         * conformance cropping window to further crop the displayed window */
        int defDispWinLeftOffset;

        /* Default display window right offset holds the right offset with the
         * conformance cropping window to further crop the displayed window */
        int defDispWinRightOffset;

        /* Default display window top offset holds the top offset with the
         * conformance cropping window to further crop the displayed window */
        int defDispWinTopOffset;

        /* Default display window bottom offset holds the bottom offset with the
         * conformance cropping window to further crop the displayed window */
        int defDispWinBottomOffset;

        /* VUI timing info present flag adds vui_num_units_in_tick,
         * vui_time_scale, vui_poc_proportional_to_timing_flag and
         * vui_hrd_parameters_present_flag to the VUI. vui_num_units_in_tick,
         * vui_time_scale and vui_poc_proportional_to_timing_flag are derived
         * from processing the input video. The default is false */
        int bEnableVuiTimingInfoPresentFlag;
    } vui;

} x265_param;

/***
 * If not called, first encoder allocated will auto-detect the CPU and
 * initialize performance primitives, which are process global.
 * DEPRECATED: use x265_param.cpuid to specify CPU */
void x265_setup_primitives(x265_param *param, int cpu);

/* x265_param_alloc:
 *  Allocates an x265_param instance. The returned param structure is not
 *  special in any way, but using this method together with x265_param_free()
 *  and x265_param_parse() to set values by name allows the application to treat
 *  x265_param as an opaque data struct for version safety */
x265_param *x265_param_alloc();

/* x265_param_free:
 *  Use x265_param_free() to release storage for an x265_param instance
 *  allocated by x26_param_alloc() */
void x265_param_free(x265_param *);

/***
 * Initialize an x265_param_t structure to default values
 */
void x265_param_default(x265_param *param);

/* x265_param_parse:
 *  set one parameter by name.
 *  returns 0 on success, or returns one of the following errors.
 *  note: BAD_VALUE occurs only if it can't even parse the value,
 *  numerical range is not checked until x265_encoder_open() or
 *  x265_encoder_reconfig().
 *  value=NULL means "true" for boolean options, but is a BAD_VALUE for non-booleans. */
#define X265_PARAM_BAD_NAME  (-1)
#define X265_PARAM_BAD_VALUE (-2)
int x265_param_parse(x265_param *p, const char *name, const char *value);

/* x265_param_apply_profile:
 *      Applies the restrictions of the given profile. (one of below) */
static const char * const x265_profile_names[] = { "main", "main10", "mainstillpicture", 0 };

/*      (can be NULL, in which case the function will do nothing)
 *      returns 0 on success, negative on failure (e.g. invalid profile name). */
int x265_param_apply_profile(x265_param *, const char *profile);

/* x265_param_default_preset:
 *      The same as x265_param_default, but also use the passed preset and tune
 *      to modify the default settings.
 *      (either can be NULL, which implies no preset or no tune, respectively)
 *
 *      Currently available presets are, ordered from fastest to slowest: */
static const char * const x265_preset_names[] = { "ultrafast", "superfast", "veryfast", "faster", "fast", "medium", "slow", "slower", "veryslow", "placebo", 0 };

/*      The presets can also be indexed numerically, as in:
 *      x265_param_default_preset( &param, "3", ... )
 *      with ultrafast mapping to "0" and placebo mapping to "9".  This mapping may
 *      of course change if new presets are added in between, but will always be
 *      ordered from fastest to slowest.
 *
 *      Warning: the speed of these presets scales dramatically.  Ultrafast is a full
 *      100 times faster than placebo!
 *
 *      Currently available tunings are: */
static const char * const x265_tune_names[] = { "psnr", "ssim", "zerolatency", "fastdecode", 0 };

/*      returns 0 on success, negative on failure (e.g. invalid preset/tune name). */
int x265_param_default_preset(x265_param *, const char *preset, const char *tune);

/* x265_picture_alloc:
 *  Allocates an x265_picture instance. The returned picture structure is not
 *  special in any way, but using this method together with x265_picture_free()
 *  and x265_picture_init() allows some version safety. New picture fields will
 *  always be added to the end of x265_picture */
x265_picture *x265_picture_alloc();

/* x265_picture_free:
 *  Use x265_picture_free() to release storage for an x265_picture instance
 *  allocated by x26_picture_alloc() */
void x265_picture_free(x265_picture *);

/***
 * Initialize an x265_picture structure to default values. It sets the pixel
 * depth and color space to the encoder's internal values and sets the slice
 * type to auto - so the lookahead will determine slice type.
 */
void x265_picture_init(x265_param *param, x265_picture *pic);

/* x265_max_bit_depth:
 *      Specifies the maximum number of bits per pixel that x265 can input. This
 *      is also the max bit depth that x265 encodes in.  When x265_max_bit_depth
 *      is 8, the internal and input bit depths must be 8.  When
 *      x265_max_bit_depth is 12, the internal and input bit depths can be
 *      either 8, 10, or 12. Note that the internal bit depth must be the same
 *      for all encoders allocated in the same process. */
X265_API extern const int x265_max_bit_depth;

/* x265_version_str:
 *      A static string containing the version of this compiled x265 library */
X265_API extern const char *x265_version_str;

/* x265_build_info:
 *      A static string describing the compiler and target architecture */
X265_API extern const char *x265_build_info_str;

/* Force a link error in the case of linking against an incompatible API version.
 * Glue #defines exist to force correct macro expansion; the final output of the macro
 * is x265_encoder_open_##X264_BUILD (for purposes of dlopen). */
#define x265_encoder_glue1(x, y) x ## y
#define x265_encoder_glue2(x, y) x265_encoder_glue1(x, y)
#define x265_encoder_open x265_encoder_glue2(x265_encoder_open_, X265_BUILD)

/* x265_encoder_open:
 *      create a new encoder handler, all parameters from x265_param_t are copied */
x265_encoder* x265_encoder_open(x265_param *);

/* x265_encoder_headers:
 *      return the SPS and PPS that will be used for the whole stream.
 *      *pi_nal is the number of NAL units outputted in pp_nal.
 *      returns negative on error, total byte size of payload data on success
 *      the payloads of all output NALs are guaranteed to be sequential in memory. */
int x265_encoder_headers(x265_encoder *, x265_nal **pp_nal, uint32_t *pi_nal);

/* x265_encoder_encode:
 *      encode one picture.
 *      *pi_nal is the number of NAL units outputted in pp_nal.
 *      returns negative on error, zero if no NAL units returned.
 *      the payloads of all output NALs are guaranteed to be sequential in memory. */
int x265_encoder_encode(x265_encoder *encoder, x265_nal **pp_nal, uint32_t *pi_nal, x265_picture *pic_in, x265_picture *pic_out);

/* x265_encoder_get_stats:
 *       returns encoder statistics */
void x265_encoder_get_stats(x265_encoder *encoder, x265_stats *, uint32_t statsSizeBytes);

/* x265_encoder_log:
 *       write a line to the configured CSV file.  If a CSV filename was not
 *       configured, or file open failed, or the log level indicated frame level
 *       logging, this function will perform no write. */
void x265_encoder_log(x265_encoder *encoder, int argc, char **argv);

/* x265_encoder_close:
 *      close an encoder handler */
void x265_encoder_close(x265_encoder *);

/***
 * Release library static allocations
 */
void x265_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif // X265_H
