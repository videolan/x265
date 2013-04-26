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

/** \file     TypeDef.h
    \brief    Define basic types, new types and enumerations
*/

#ifndef _TYPEDEF__
#define _TYPEDEF__

//! \ingroup TLibCommon
//! \{

#define L0208_SOP_DESCRIPTION_SEI     1 ///< L0208: add SOP descrioption SEI
#define J0149_TONE_MAPPING_SEI        1 ///< J0149: Tone mapping information SEI

#define MAX_NUM_PICS_IN_SOP           1024

#define K0180_SCALABLE_NESTING_SEI  1   ///JCTVC-K0180 scalable nesting sei message
#define MAX_NESTING_NUM_OPS         1024
#define MAX_NESTING_NUM_LAYER       64

#define L0047_APS_FLAGS            1  ///< L0047: Include full_random_access_flag and no_param_set_update_flag in the active parameter set SEI message
#define L0043_TIMING_INFO          1  ///< L0043: Timing information is signalled in VUI outside hrd_parameters()
#define L0046_RENAME_PROG_SRC_IDC  1  ///< L0046: Rename progressive_source_idc to source_scan_type
#define L0045_CONDITION_SIGNALLING 1  ///< L0045: Condition the signaling of some syntax elements in picture timing SEI message
#define L0043_MSS_IDC 1
#define L0116_ENTRY_POINT 1
#define L0363_MORE_BITS 1
#define L0363_MVP_POC 1
#define L0363_BYTE_ALIGN 1
#define L0363_SEI_ALLOW_SUFFIX 1
#define L0323_LIMIT_DEFAULT_LIST_SIZE 1
#define L0046_CONSTRAINT_FLAGS 1
#define L0255_MOVE_PPS_FLAGS       1  ///< move some flags to earlier positions in the PPS
#define L0444_FPA_TYPE             1  ///< allow only FPA types 3, 4 and 5
#define L0372 1
#define SIGNAL_BITRATE_PICRATE_IN_VPS               0  ///< K0125: Signal bit_rate and pic_rate in VPS
#define L0232_RD_PENALTY           1  ///< L0232: RD-penalty for 32x32 TU for intra in non-intra slices

#define MAX_VPS_NUM_HRD_PARAMETERS                1
#define MAX_VPS_OP_SETS_PLUS1                     1024
#define MAX_VPS_NUH_RESERVED_ZERO_LAYER_ID_PLUS1  1

#define RATE_CONTROL_LAMBDA_DOMAIN                  1  ///< JCTVC-K0103, rate control by R-lambda model
#define L0033_RC_BUGFIX                             1  ///< JCTVC-L0033, bug fix for R-lambda model based rate control

#define MAX_CPB_CNT                     32  ///< Upper bound of (cpb_cnt_minus1 + 1)
#define MAX_NUM_LAYER_IDS                64

#define COEF_REMAIN_BIN_REDUCTION        3 ///< indicates the level at which the VLC
                                           ///< transitions from Golomb-Rice to TU+EG(k)

#define CU_DQP_TU_CMAX 5                   ///< max number bins for truncated unary
#define CU_DQP_EG_k 0                      ///< expgolomb order

#define SBH_THRESHOLD                    4  ///< I0156: value of the fixed SBH controlling threshold

#define SEQUENCE_LEVEL_LOSSLESS           0  ///< H0530: used only for sequence or frame-level lossless coding

#define DISABLING_CLIP_FOR_BIPREDME         1  ///< Ticket #175

#define C1FLAG_NUMBER               8 // maximum number of largerThan1 flag coded in one chunk :  16 in HM5
#define C2FLAG_NUMBER               1 // maximum number of largerThan2 flag coded in one chunk:  16 in HM5

#define REMOVE_SAO_LCU_ENC_CONSTRAINTS_3 1  ///< disable the encoder constraint that conditionally disable SAO for chroma for entire slice in interleaved mode

#define REMOVE_SINGLE_SEI_EXTENSION_FLAGS 1 ///< remove display orientation SEI extension flag (there is a generic SEI extension mechanism now)

#define SAO_ENCODING_CHOICE              1  ///< I0184: picture early termination
#if SAO_ENCODING_CHOICE
#define SAO_ENCODING_RATE                0.75
#define SAO_ENCODING_CHOICE_CHROMA       1 ///< J0044: picture early termination Luma and Chroma are handled separately
#if SAO_ENCODING_CHOICE_CHROMA
#define SAO_ENCODING_RATE_CHROMA         0.5
#endif
#endif

#define MAX_NUM_VPS                16
#define MAX_NUM_SPS                16
#define MAX_NUM_PPS                64

#define WEIGHTED_CHROMA_DISTORTION  1   ///< F386: weighting of chroma for RDO
#define RDOQ_CHROMA_LAMBDA          1   ///< F386: weighting of chroma for RDOQ
#define SAO_CHROMA_LAMBDA           1   ///< F386: weighting of chroma for SAO

#define MIN_SCAN_POS_CROSS          4

#define MLS_GRP_NUM                         64     ///< G644 : Max number of coefficient groups, max(16, 64)
#define MLS_CG_SIZE                         4      ///< G644 : Coefficient group size of 4x4

#define ADAPTIVE_QP_SELECTION               1      ///< G382: Adaptive reconstruction levels, non-normative part for adaptive QP selection
#if ADAPTIVE_QP_SELECTION
#define ARL_C_PRECISION                     7      ///< G382: 7-bit arithmetic precision
#define LEVEL_RANGE                         30     ///< G382: max coefficient level in statistics collection
#endif

#define HHI_RQT_INTRA_SPEEDUP             1           ///< tests one best mode with full rqt
#define HHI_RQT_INTRA_SPEEDUP_MOD         0           ///< tests two best modes with full rqt

#if HHI_RQT_INTRA_SPEEDUP_MOD && !HHI_RQT_INTRA_SPEEDUP
#error
#endif

#define VERBOSE_RATE 0 ///< Print additional rate information in encoder

#define AMVP_DECIMATION_FACTOR            4

#define SCAN_SET_SIZE                     16
#define LOG2_SCAN_SET_SIZE                4

#define FAST_UDI_MAX_RDMODE_NUM               35          ///< maximum number of RD comparison in fast-UDI estimation loop

#define NUM_INTRA_MODE 36
#if !REMOVE_LM_CHROMA
#define LM_CHROMA_IDX  35
#endif

#define WRITE_BACK                      1           ///< Enable/disable the encoder to replace the deltaPOC and Used by current from the config file with the values derived by the refIdc parameter.
#define AUTO_INTER_RPS                  1           ///< Enable/disable the automatic generation of refIdc from the deltaPOC and Used by current from the config file.
#define PRINT_RPS_INFO                  0           ///< Enable/disable the printing of bits used to send the RPS.
                                                    // using one nearest frame as reference frame, and the other frames are high quality (POC%4==0) frames (1+X)
                                                    // this should be done with encoder only decision
                                                    // but because of the absence of reference frame management, the related code was hard coded currently

#define RVM_VCEGAM10_M 4

#define PLANAR_IDX             0
#define VER_IDX                26                    // index for intra VERTICAL   mode
#define HOR_IDX                10                    // index for intra HORIZONTAL mode
#define DC_IDX                 1                     // index for intra DC mode
#define NUM_CHROMA_MODE        5                     // total number of chroma modes
#define DM_CHROMA_IDX          36                    // chroma mode index for derived from luma intra mode

#define FAST_UDI_USE_MPM 1

#define RDO_WITHOUT_DQP_BITS              0           ///< Disable counting dQP bits in RDO-based mode decision

#define FULL_NBIT 0 ///< When enabled, compute costs using full sample bitdepth.  When disabled, compute costs as if it is 8-bit source video.
#if FULL_NBIT || !defined(HIGH_BIT_DEPTH)
# define DISTORTION_PRECISION_ADJUSTMENT(x) 0
#else
# define DISTORTION_PRECISION_ADJUSTMENT(x) (x)
#endif

#define LOG2_MAX_NUM_COLUMNS_MINUS1        7
#define LOG2_MAX_NUM_ROWS_MINUS1           7
#define LOG2_MAX_COLUMN_WIDTH              13
#define LOG2_MAX_ROW_HEIGHT                13

#define MATRIX_MULT                             0   // Brute force matrix multiplication instead of partial butterfly

#define REG_DCT 65535

#define SCALING_LIST_OUTPUT_RESULT    0 //JCTVC-G880/JCTVC-G1016 quantization matrices

#define CABAC_INIT_PRESENT_FLAG     1

// ====================================================================================================================
// Basic type redefinition
// ====================================================================================================================

typedef       void                Void;
typedef       bool                Bool;

typedef       char                Char;
typedef       unsigned char       UChar;
typedef       short               Short;
typedef       unsigned short      UShort;
typedef       int                 Int;
typedef       unsigned int        UInt;
typedef       double              Double;
typedef       float               Float;

// ====================================================================================================================
// 64-bit integer type
// ====================================================================================================================

#ifdef _MSC_VER
typedef       __int64             Int64;

#if _MSC_VER <= 1200 // MS VC6
typedef       __int64             UInt64;   // MS VC6 does not support unsigned __int64 to double conversion
#else
typedef       unsigned __int64    UInt64;
#endif

#else

typedef       long long           Int64;
typedef       unsigned long long  UInt64;

#endif // ifdef _MSC_VER

// ====================================================================================================================
// Type definition
// ====================================================================================================================

typedef       UChar           Pxl;        ///< 8-bit pixel type
#if HIGH_BIT_DEPTH
typedef       Short           Pel;        ///< 16-bit pixel type
#else
typedef       UChar           Pel;        ///< 8-bit pixel type
#endif
typedef       Int             TCoeff;     ///< transform coefficient

/// parameters for adaptive loop filter
class TComPicSym;

// Slice / Slice segment encoding modes
enum SliceConstraint
{
    NO_SLICES              = 0,        ///< don't use slices / slice segments
    FIXED_NUMBER_OF_LCU    = 1,        ///< Limit maximum number of largest coding tree blocks in a slice / slice segments
    FIXED_NUMBER_OF_BYTES  = 2,        ///< Limit maximum number of bytes in a slice / slice segment
    FIXED_NUMBER_OF_TILES  = 3,        ///< slices / slice segments span an integer number of tiles
};

#define NUM_DOWN_PART 4

enum SAOTypeLen
{
    SAO_EO_LEN    = 4,
    SAO_BO_LEN    = 4,
    SAO_MAX_BO_CLASSES = 32
};

enum SAOType
{
    SAO_EO_0 = 0,
    SAO_EO_1,
    SAO_EO_2,
    SAO_EO_3,
    SAO_BO,
    MAX_NUM_SAO_TYPE
};

typedef struct _SaoQTPart
{
    Int         iBestType;
    Int         iLength;
    Int         subTypeIdx;                ///< indicates EO class or BO band position
    Int         iOffset[4];
    Int         StartCUX;
    Int         StartCUY;
    Int         EndCUX;
    Int         EndCUY;

    Int         PartIdx;
    Int         PartLevel;
    Int         PartCol;
    Int         PartRow;

    Int         DownPartsIdx[NUM_DOWN_PART];
    Int         UpPartIdx;

    Bool        bSplit;

    //---- encoder only start -----//
    Bool        bProcessed;
    Double      dMinCost;
    Int64       iMinDist;
    Int         iMinRate;
    //---- encoder only end -----//
} SAOQTPart;

typedef struct _SaoLcuParam
{
    Bool       mergeUpFlag;
    Bool       mergeLeftFlag;
    Int        typeIdx;
    Int        subTypeIdx;                ///< indicates EO class or BO band position
    Int        offset[4];
    Int        partIdx;
    Int        partIdxTmp;
    Int        length;
} SaoLcuParam;

struct SAOParam
{
    Bool       bSaoFlag[2];
    SAOQTPart* psSaoPart[3];
    Int        iMaxSplitLevel;
    Bool         oneUnitFlag[3];
    SaoLcuParam* saoLcuParam[3];
    Int          numCuInHeight;
    Int          numCuInWidth;
    ~SAOParam();
};

/// parameters for deblocking filter
typedef struct _LFCUParam
{
    Bool bInternalEdge;                   ///< indicates internal edge
    Bool bLeftEdge;                       ///< indicates left edge
    Bool bTopEdge;                        ///< indicates top edge
} LFCUParam;

// ====================================================================================================================
// Enumeration
// ====================================================================================================================

/// supported slice type
enum SliceType
{
    B_SLICE,
    P_SLICE,
    I_SLICE
};

/// chroma formats (according to semantics of chroma_format_idc)
enum ChromaFormat
{
    CHROMA_400  = 0,
    CHROMA_420  = 1,
    CHROMA_422  = 2,
    CHROMA_444  = 3
};

/// supported partition shape
enum PartSize
{
    SIZE_2Nx2N,         ///< symmetric motion partition,  2Nx2N
    SIZE_2NxN,          ///< symmetric motion partition,  2Nx N
    SIZE_Nx2N,          ///< symmetric motion partition,   Nx2N
    SIZE_NxN,           ///< symmetric motion partition,   Nx N
    SIZE_2NxnU,         ///< asymmetric motion partition, 2Nx( N/2) + 2Nx(3N/2)
    SIZE_2NxnD,         ///< asymmetric motion partition, 2Nx(3N/2) + 2Nx( N/2)
    SIZE_nLx2N,         ///< asymmetric motion partition, ( N/2)x2N + (3N/2)x2N
    SIZE_nRx2N,         ///< asymmetric motion partition, (3N/2)x2N + ( N/2)x2N
    SIZE_NONE = 15
};

/// supported prediction type
enum PredMode
{
    MODE_INTER,         ///< inter-prediction mode
    MODE_INTRA,         ///< intra-prediction mode
    MODE_NONE = 15
};

/// texture component type
enum TextType
{
    TEXT_LUMA,          ///< luma
    TEXT_CHROMA,        ///< chroma (U+V)
    TEXT_CHROMA_U,      ///< chroma U
    TEXT_CHROMA_V,      ///< chroma V
    TEXT_ALL,           ///< Y+U+V
    TEXT_NONE = 15
};

/// reference list index
enum RefPicList
{
    REF_PIC_LIST_0 = 0, ///< reference list 0
    REF_PIC_LIST_1 = 1, ///< reference list 1
    REF_PIC_LIST_X = 100 ///< special mark
};

/// distortion function index
enum DFunc
{
    DF_DEFAULT  = 0,
    DF_SSE      = 1,    ///< general size SSE
    DF_SSE4     = 2,    ///<   4xM SSE
    DF_SSE8     = 3,    ///<   8xM SSE
    DF_SSE16    = 4,    ///<  16xM SSE
    DF_SSE32    = 5,    ///<  32xM SSE
    DF_SSE64    = 6,    ///<  64xM SSE
    DF_SSE16N   = 7,    ///< 16NxM SSE

    DF_SAD      = 8,    ///< general size SAD
    DF_SAD4     = 9,    ///<   4xM SAD
    DF_SAD8     = 10,   ///<   8xM SAD
    DF_SAD16    = 11,   ///<  16xM SAD
    DF_SAD32    = 12,   ///<  32xM SAD
    DF_SAD64    = 13,   ///<  64xM SAD
    DF_SAD16N   = 14,   ///< 16NxM SAD

    DF_SADS     = 15,   ///< general size SAD with step
    DF_SADS4    = 16,   ///<   4xM SAD with step
    DF_SADS8    = 17,   ///<   8xM SAD with step
    DF_SADS16   = 18,   ///<  16xM SAD with step
    DF_SADS32   = 19,   ///<  32xM SAD with step
    DF_SADS64   = 20,   ///<  64xM SAD with step
    DF_SADS16N  = 21,   ///< 16NxM SAD with step

    DF_HADS     = 22,   ///< general size Hadamard with step
    DF_HADS4    = 23,   ///<   4xM HAD with step
    DF_HADS8    = 24,   ///<   8xM HAD with step
    DF_HADS16   = 25,   ///<  16xM HAD with step
    DF_HADS32   = 26,   ///<  32xM HAD with step
    DF_HADS64   = 27,   ///<  64xM HAD with step
    DF_HADS16N  = 28,   ///< 16NxM HAD with step

    DF_SAD12    = 43,
    DF_SAD24    = 44,
    DF_SAD48    = 45,

    DF_SADS12   = 46,
    DF_SADS24   = 47,
    DF_SADS48   = 48,

    DF_SSE_FRAME = 50   ///< Frame-based SSE
};

/// index for SBAC based RD optimization
enum CI_IDX
{
    CI_CURR_BEST = 0,   ///< best mode index
    CI_NEXT_BEST,       ///< next best index
    CI_TEMP_BEST,       ///< temporal index
    CI_CHROMA_INTRA,    ///< chroma intra index
    CI_QT_TRAFO_TEST,
    CI_QT_TRAFO_ROOT,
    CI_NUM,             ///< total number
};

/// motion vector predictor direction used in AMVP
enum MVP_DIR
{
    MD_LEFT = 0,        ///< MVP of left block
    MD_ABOVE,           ///< MVP of above block
    MD_ABOVE_RIGHT,     ///< MVP of above right block
    MD_BELOW_LEFT,      ///< MVP of below left block
    MD_ABOVE_LEFT       ///< MVP of above left block
};

/// coefficient scanning type used in ACS
enum COEFF_SCAN_TYPE
{
    SCAN_DIAG = 0,       ///< up-right diagonal scan
    SCAN_HOR,            ///< horizontal first scan
    SCAN_VER             ///< vertical first scan
};

namespace Profile {
enum Name
{
    NONE = 0,
    MAIN = 1,
    MAIN10 = 2,
    MAINSTILLPICTURE = 3,
};
}

namespace Level {
enum Tier
{
    MAIN = 0,
    HIGH = 1,
};

enum Name
{
    NONE     = 0,
    LEVEL1   = 30,
    LEVEL2   = 60,
    LEVEL2_1 = 63,
    LEVEL3   = 90,
    LEVEL3_1 = 93,
    LEVEL4   = 120,
    LEVEL4_1 = 123,
    LEVEL5   = 150,
    LEVEL5_1 = 153,
    LEVEL5_2 = 156,
    LEVEL6   = 180,
    LEVEL6_1 = 183,
    LEVEL6_2 = 186,
};
}
//! \}

#endif // ifndef _TYPEDEF__
