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

#ifndef X265_TYPEDEF_H
#define X265_TYPEDEF_H

#include "common.h"

namespace x265 {
// private namespace

#define REF_PIC_LIST_0 0
#define REF_PIC_LIST_1 1
#define REF_PIC_LIST_X 100

// ====================================================================================================================
// Basic type redefinition
// ====================================================================================================================

typedef unsigned char  UChar;

// ====================================================================================================================
// Type definition
// ====================================================================================================================

#if HIGH_BIT_DEPTH
typedef uint16_t Pel;          // 16-bit pixel type
#define X265_DEPTH 10          // compile time configurable bit depth
#else
typedef uint8_t  Pel;          // 8-bit pixel type
#define X265_DEPTH 8           // compile time configurable bit depth
#endif
typedef int32_t  TCoeff;       // transform coefficient

// ====================================================================================================================
// Enumeration
// ====================================================================================================================
#define MDCS_ANGLE_LIMIT                                  4         ///< (default 4) 0 = Horizontal/vertical only, 1 = Horizontal/vertical +/- 1, 2 = Horizontal/vertical +/- 2 etc...
#define MDCS_LOG2_MAX_SIZE                                3         ///< (default 3) TUs with log2 of size greater than this can only use diagonal scan

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
    CHROMA_444  = 3,
    NUM_CHROMA_FORMAT = 4
};

#define CHROMA_H_SHIFT(x) (x == X265_CSP_I420 || x == X265_CSP_I422)
#define CHROMA_V_SHIFT(x) (x == X265_CSP_I420)

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
    TEXT_LUMA     = 0,  ///< luma
    TEXT_CHROMA   = 1,  ///< chroma (U+V)
    TEXT_CHROMA_U = 1,  ///< chroma U
    TEXT_CHROMA_V = 2,  ///< chroma V
};

/// index for SBAC based RD optimization
enum CI_IDX
{
    CI_CURR_BEST = 0,   ///< best mode index
    CI_NEXT_BEST,       ///< next best index
    CI_TEMP_BEST,       ///< temporal index
    CI_QT_TRAFO_TEST,
    CI_QT_TRAFO_ROOT,
    CI_NUM,             ///< total number
    CI_NUM_SAO   = 3,
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
    SCAN_DIAG = 0,      ///< up-right diagonal scan
    SCAN_HOR  = 1,      ///< horizontal first scan
    SCAN_VER  = 2,      ///< vertical first scan
    SCAN_NUMBER_OF_TYPES = 3
};

enum SignificanceMapContextType
{
    CONTEXT_TYPE_4x4    = 0,
    CONTEXT_TYPE_8x8    = 1,
    CONTEXT_TYPE_NxN    = 2,
    CONTEXT_NUMBER_OF_TYPES = 3
};

enum COEFF_SCAN_GROUP_TYPE
{
    SCAN_UNGROUPED   = 0,
    SCAN_GROUPED_4x4 = 1,
    SCAN_NUMBER_OF_GROUP_TYPES = 2
};

//TU settings for entropy encoding
struct TUEntropyCodingParameters
{
    const uint16_t            *scan;
    const uint16_t            *scanCG;
    COEFF_SCAN_TYPE      scanType;
    TextType             ctype;
    uint32_t             log2TrSize;
    uint32_t             log2TrSizeCG;
    uint32_t             firstSignificanceMapContext;
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
}
//! \}

#endif // ifndef X265_TYPEDEF_H
