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

#define REF_PIC_LIST_0 0
#define REF_PIC_LIST_1 1
#define REF_PIC_LIST_X 100

#include <stdint.h>

namespace x265 {
// private namespace

// ====================================================================================================================
// Basic type redefinition
// ====================================================================================================================

typedef unsigned char  UChar;

// ====================================================================================================================
// 64-bit integer type
// ====================================================================================================================

#ifdef _MSC_VER
typedef __int64             Int64;
typedef unsigned __int64    UInt64;
#else
typedef long long           Int64;
typedef unsigned long long  UInt64;
#endif // ifdef _MSC_VER

// ====================================================================================================================
// Type definition
// ====================================================================================================================

#if HIGH_BIT_DEPTH
typedef uint16_t Pel;            // 16-bit pixel type
#define X265_DEPTH x265::g_bitDepth  // runtime configurable bit depth
extern int g_bitDepth;
#else
typedef UChar  Pel;            // 8-bit pixel type
#define X265_DEPTH 8           // compile time configurable bit depth
#endif
typedef int    TCoeff;         // transform coefficient

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

#define CHROMA_H_SHIFT(x) (x == CHROMA_420 || x == CHROMA_422)
#define CHROMA_V_SHIFT(x) (x == CHROMA_420)

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
}
//! \}

#endif // ifndef X265_TYPEDEF_H
