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

/** \file     TComRom.h
    \brief    global variables & functions (header)
*/

#ifndef X265_TCOMROM_H
#define X265_TCOMROM_H

#include "CommonDef.h"

#include <stdio.h>
#include <iostream>

namespace x265 {
// private namespace

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Macros
// ====================================================================================================================

#define MAX_CU_DEPTH            6                           // log2(LCUSize)
#define MAX_CU_SIZE             (1 << (MAX_CU_DEPTH))       // maximum allowable size of CU
#define MIN_PU_SIZE             4
#define MAX_NUM_SPU_W           (MAX_CU_SIZE / MIN_PU_SIZE) // maximum number of SPU in horizontal line
#define ADI_BUF_STRIDE          (2 * MAX_CU_SIZE + 1 + 15)  // alignment to 16 bytes

// ====================================================================================================================
// Initialize / destroy functions
// ====================================================================================================================

void initROM();
void destroyROM();
void initSigLastScan(UInt* buffD, UInt* buffH, UInt* buffV, int width, int height);

// ====================================================================================================================
// Data structure related table & variable
// ====================================================================================================================

// flexible conversion from relative to absolute index
extern UInt g_zscanToRaster[MAX_NUM_SPU_W * MAX_NUM_SPU_W];
extern UInt g_rasterToZscan[MAX_NUM_SPU_W * MAX_NUM_SPU_W];

void initZscanToRaster(int maxDepth, int depth, UInt startVal, UInt*& curIdx);
void initRasterToZscan(UInt maxCUWidth, UInt maxCUHeight, UInt maxCUDepth);

// conversion of partition index to picture pel position
extern UInt g_rasterToPelX[MAX_NUM_SPU_W * MAX_NUM_SPU_W];
extern UInt g_rasterToPelY[MAX_NUM_SPU_W * MAX_NUM_SPU_W];

void initRasterToPelXY(UInt maxCUWidth, UInt maxCUHeight, UInt maxCUDepth);

// global variable (LCU width/height, max. CU depth)
extern UInt g_maxCUWidth;
extern UInt g_maxCUHeight;
extern UInt g_maxCUDepth;
extern UInt g_addCUDepth;

#define MAX_TS_WIDTH  4
#define MAX_TS_HEIGHT 4

extern UInt g_puOffset[8];

#define QUANT_IQUANT_SHIFT    20 // Q(QP%6) * IQ(QP%6) = 2^20
#define QUANT_SHIFT           14 // Q(4) = 2^14
#define SCALE_BITS            15 // Inherited from TMuC, presumably for fractional bit estimates in RDOQ
#define MAX_TR_DYNAMIC_RANGE  15 // Maximum transform dynamic range (excluding sign bit)

#define SHIFT_INV_1ST          7 // Shift after first inverse transform stage
#define SHIFT_INV_2ND         12 // Shift after second inverse transform stage

extern int g_quantScales[6];     // Q(QP%6)
extern int g_invQuantScales[6];  // IQ(QP%6)
extern const short g_t4[4][4];
extern const short g_t8[8][8];
extern const short g_t16[16][16];
extern const short g_t32[32][32];

// ====================================================================================================================
// Subpel interpolation defines and constants
// ====================================================================================================================

#define NTAPS_LUMA        8                            ///< Number of taps for luma
#define NTAPS_CHROMA      4                            ///< Number of taps for chroma
#define IF_INTERNAL_PREC 14                            ///< Number of bits for internal precision
#define IF_FILTER_PREC    6                            ///< Log2 of sum of filter taps
#define IF_INTERNAL_OFFS (1 << (IF_INTERNAL_PREC - 1)) ///< Offset used internally

extern const short g_lumaFilter[4][NTAPS_LUMA];     ///< Luma filter taps
extern const short g_chromaFilter[8][NTAPS_CHROMA]; ///< Chroma filter taps

// ====================================================================================================================
// Luma QP to Chroma QP mapping
// ====================================================================================================================

extern const UChar g_chromaScale[58];

// ====================================================================================================================
// Scanning order & context mapping table
// ====================================================================================================================

extern UInt* g_sigLastScan[3][MAX_CU_DEPTH];  // raster index from scanning index (diag, hor, ver)

extern const UInt g_groupIdx[32];
extern const UInt g_minInGroup[10];

extern const UInt g_goRiceRange[5];      //!< maximum value coded with Rice codes
extern const UInt g_goRicePrefixLen[5];  //!< prefix length for each maximum value

extern const UInt g_sigLastScan8x8[3][4];   //!< coefficient group scan order for 8x8 TUs
extern       UInt g_sigLastScanCG32x32[64];

// ====================================================================================================================
// ADI table
// ====================================================================================================================

extern const UChar g_intraModeNumFast[7];

// ====================================================================================================================
// Bit-depth
// ====================================================================================================================

extern int g_bitDepth;

/** clip x, such that 0 <= x <= #g_maxLumaVal */
template<typename T>
inline T ClipY(T x) { return std::min<T>(T((1 << X265_DEPTH) - 1), std::max<T>(T(0), x)); }

template<typename T>
inline T ClipC(T x) { return std::min<T>(T((1 << X265_DEPTH) - 1), std::max<T>(T(0), x)); }

/** clip a, such that minVal <= a <= maxVal */
template<typename T>
inline T Clip3(T minVal, T maxVal, T a) { return std::min<T>(std::max<T>(minVal, a), maxVal); } ///< general min/max clip

// ====================================================================================================================
// Texture type to integer mapping
// ====================================================================================================================

extern const UChar g_convertTxtTypeToIdx[4];

// ====================================================================================================================
// Misc.
// ====================================================================================================================

extern char g_convertToBit[MAX_CU_SIZE + 1]; // from width to log2(width)-2

#ifndef ENC_DEC_TRACE
# define ENC_DEC_TRACE 0
#endif

#if ENC_DEC_TRACE
extern FILE*  g_hTrace;
extern bool   g_bJustDoIt;
extern const bool g_bEncDecTraceEnable;
extern const bool g_bEncDecTraceDisable;
extern bool   g_HLSTraceEnable;
extern UInt64 g_nSymbolCounter;

#define COUNTER_START    1
#define COUNTER_END      0 //( UInt64(1) << 63 )

#define DTRACE_CABAC_F(x)     if ((g_nSymbolCounter >= COUNTER_START && g_nSymbolCounter <= COUNTER_END) || g_bJustDoIt) fprintf(g_hTrace, "%f", x);
#define DTRACE_CABAC_V(x)     if ((g_nSymbolCounter >= COUNTER_START && g_nSymbolCounter <= COUNTER_END) || g_bJustDoIt) fprintf(g_hTrace, "%d", x);
#define DTRACE_CABAC_VL(x)    if ((g_nSymbolCounter >= COUNTER_START && g_nSymbolCounter <= COUNTER_END) || g_bJustDoIt) fprintf(g_hTrace, "%lld", x);
#define DTRACE_CABAC_T(x)     if ((g_nSymbolCounter >= COUNTER_START && g_nSymbolCounter <= COUNTER_END) || g_bJustDoIt) fprintf(g_hTrace, "%s", x);
#define DTRACE_CABAC_X(x)     if ((g_nSymbolCounter >= COUNTER_START && g_nSymbolCounter <= COUNTER_END) || g_bJustDoIt) fprintf(g_hTrace, "%x", x);
#define DTRACE_CABAC_R(x, y)  if ((g_nSymbolCounter >= COUNTER_START && g_nSymbolCounter <= COUNTER_END) || g_bJustDoIt) fprintf(g_hTrace, x,    y);
#define DTRACE_CABAC_N        if ((g_nSymbolCounter >= COUNTER_START && g_nSymbolCounter <= COUNTER_END) || g_bJustDoIt) fprintf(g_hTrace, "\n");
#else // if ENC_DEC_TRACE

#define DTRACE_CABAC_F(x)
#define DTRACE_CABAC_V(x)
#define DTRACE_CABAC_VL(x)
#define DTRACE_CABAC_T(x)
#define DTRACE_CABAC_X(x)
#define DTRACE_CABAC_R(x, y)
#define DTRACE_CABAC_N

#endif // if ENC_DEC_TRACE

#define SCALING_LIST_NUM 6         ///< list number for quantization matrix
#define SCALING_LIST_NUM_32x32 2   ///< list number for quantization matrix 32x32
#define SCALING_LIST_REM_NUM 6     ///< remainder of QP/6
#define SCALING_LIST_START_VALUE 8 ///< start value for dpcm mode
#define MAX_MATRIX_COEF_NUM 64     ///< max coefficient number for quantization matrix
#define MAX_MATRIX_SIZE_NUM 8      ///< max size number for quantization matrix
#define SCALING_LIST_DC 16         ///< default DC value
enum ScalingListSize
{
    SCALING_LIST_4x4 = 0,
    SCALING_LIST_8x8,
    SCALING_LIST_16x16,
    SCALING_LIST_32x32,
    SCALING_LIST_SIZE_NUM
};

static const char MatrixType[4][6][20] =
{
    {
        "INTRA4X4_LUMA",
        "INTRA4X4_CHROMAU",
        "INTRA4X4_CHROMAV",
        "INTER4X4_LUMA",
        "INTER4X4_CHROMAU",
        "INTER4X4_CHROMAV"
    },
    {
        "INTRA8X8_LUMA",
        "INTRA8X8_CHROMAU",
        "INTRA8X8_CHROMAV",
        "INTER8X8_LUMA",
        "INTER8X8_CHROMAU",
        "INTER8X8_CHROMAV"
    },
    {
        "INTRA16X16_LUMA",
        "INTRA16X16_CHROMAU",
        "INTRA16X16_CHROMAV",
        "INTER16X16_LUMA",
        "INTER16X16_CHROMAU",
        "INTER16X16_CHROMAV"
    },
    {
        "INTRA32X32_LUMA",
        "INTER32X32_LUMA",
    },
};
static const char MatrixType_DC[4][12][22] =
{
    {},
    {},
    {
        "INTRA16X16_LUMA_DC",
        "INTRA16X16_CHROMAU_DC",
        "INTRA16X16_CHROMAV_DC",
        "INTER16X16_LUMA_DC",
        "INTER16X16_CHROMAU_DC",
        "INTER16X16_CHROMAV_DC"
    },
    {
        "INTRA32X32_LUMA_DC",
        "INTER32X32_LUMA_DC",
    },
};
extern int g_quantIntraDefault8x8[64];
extern int g_quantIntraDefault16x16[256];
extern int g_quantIntraDefault32x32[1024];
extern int g_quantInterDefault8x8[64];
extern int g_quantInterDefault16x16[256];
extern int g_quantInterDefault32x32[1024];
extern int g_quantTSDefault4x4[16];
extern UInt g_scalingListSize[SCALING_LIST_SIZE_NUM];
extern UInt g_scalingListSizeX[SCALING_LIST_SIZE_NUM];
extern UInt g_scalingListNum[SCALING_LIST_SIZE_NUM];
extern int  g_eTTable[4];
//! \}

// Map Luma samples to chroma samples
extern const int g_winUnitX[MAX_CHROMA_FORMAT_IDC + 1];
extern const int g_winUnitY[MAX_CHROMA_FORMAT_IDC + 1];
extern const double x265_lambda2_tab_I[MAX_QP + 1];
extern const double x265_lambda2_non_I[MAX_QP + 1];
// CABAC tables
extern const UChar g_lpsTable[64][4];
extern const UChar g_renormTable[32];
extern const UChar x265_exp2_lut[64];
}

#endif  //ifndef X265_TCOMROM_H
