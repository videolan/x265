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

#ifndef __TCOMROM__
#define __TCOMROM__

#include "CommonDef.h"

#include<stdio.h>
#include<iostream>

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Macros
// ====================================================================================================================

#define     MAX_CU_DEPTH            7                           // log2(LCUSize)
#define     MAX_CU_SIZE             (1<<(MAX_CU_DEPTH))         // maximum allowable size of CU
#define     MIN_PU_SIZE             4
#define     MAX_NUM_SPU_W           (MAX_CU_SIZE/MIN_PU_SIZE)   // maximum number of SPU in horizontal line

// ====================================================================================================================
// Initialize / destroy functions
// ====================================================================================================================

Void         initROM();
Void         destroyROM();
Void         initSigLastScan(UInt* pBuffD, UInt* pBuffH, UInt* pBuffV, Int iWidth, Int iHeight);
// ====================================================================================================================
// Data structure related table & variable
// ====================================================================================================================

// flexible conversion from relative to absolute index
extern       UInt   g_auiZscanToRaster[ MAX_NUM_SPU_W*MAX_NUM_SPU_W ];
extern       UInt   g_auiRasterToZscan[ MAX_NUM_SPU_W*MAX_NUM_SPU_W ];

Void         initZscanToRaster ( Int iMaxDepth, Int iDepth, UInt uiStartVal, UInt*& rpuiCurrIdx );
Void         initRasterToZscan ( UInt uiMaxCUWidth, UInt uiMaxCUHeight, UInt uiMaxDepth         );

// conversion of partition index to picture pel position
extern       UInt   g_auiRasterToPelX[ MAX_NUM_SPU_W*MAX_NUM_SPU_W ];
extern       UInt   g_auiRasterToPelY[ MAX_NUM_SPU_W*MAX_NUM_SPU_W ];

Void         initRasterToPelXY ( UInt uiMaxCUWidth, UInt uiMaxCUHeight, UInt uiMaxDepth );

// global variable (LCU width/height, max. CU depth)
extern       UInt g_uiMaxCUWidth;
extern       UInt g_uiMaxCUHeight;
extern       UInt g_uiMaxCUDepth;
extern       UInt g_uiAddCUDepth;

#define MAX_TS_WIDTH  4
#define MAX_TS_HEIGHT 4

extern       UInt g_auiPUOffset[8];

#define QUANT_IQUANT_SHIFT    20 // Q(QP%6) * IQ(QP%6) = 2^20
#define QUANT_SHIFT           14 // Q(4) = 2^14
#define SCALE_BITS            15 // Inherited from TMuC, pressumably for fractional bit estimates in RDOQ
#define MAX_TR_DYNAMIC_RANGE  15 // Maximum transform dynamic range (excluding sign bit)

#define SHIFT_INV_1ST          7 // Shift after first inverse transform stage
#define SHIFT_INV_2ND         12 // Shift after second inverse transform stage

extern Int g_quantScales[6];             // Q(QP%6)  
extern Int g_invQuantScales[6];          // IQ(QP%6)
extern const Short g_aiT4[4][4];
extern const Short g_aiT8[8][8];
extern const Short g_aiT16[16][16];
extern const Short g_aiT32[32][32];

// ====================================================================================================================
// Luma QP to Chroma QP mapping
// ====================================================================================================================

extern const UChar  g_aucChromaScale      [58];

// ====================================================================================================================
// Scanning order & context mapping table
// ====================================================================================================================

extern       UInt*  g_auiSigLastScan[ 3 ][ MAX_CU_DEPTH ];  // raster index from scanning index (diag, hor, ver)

extern const UInt   g_uiGroupIdx[ 32 ];
extern const UInt   g_uiMinInGroup[ 10 ];

extern const UInt   g_auiGoRiceRange[5];                  //!< maximum value coded with Rice codes
extern const UInt   g_auiGoRicePrefixLen[5];              //!< prefix length for each maximum value
  
extern const UInt   g_sigLastScan8x8[ 3 ][ 4 ];           //!< coefficient group scan order for 8x8 TUs
extern       UInt   g_sigLastScanCG32x32[ 64 ];

// ====================================================================================================================
// ADI table
// ====================================================================================================================

extern const UChar  g_aucIntraModeNumFast[7];

// ====================================================================================================================
// Bit-depth
// ====================================================================================================================

extern        Int g_bitDepthY;
extern        Int g_bitDepthC;
extern       UInt g_uiPCMBitDepthLuma;
extern       UInt g_uiPCMBitDepthChroma;

// ====================================================================================================================
// Texture type to integer mapping
// ====================================================================================================================

extern const UChar g_aucConvertTxtTypeToIdx[4];

// ==========================================
// Mode-Dependent DST Matrices
extern const Short g_as_DST_MAT_4 [4][4];
extern const UChar g_aucDCTDSTMode_Vert[NUM_INTRA_MODE];
extern const UChar g_aucDCTDSTMode_Hor[NUM_INTRA_MODE];
// ==========================================

// ====================================================================================================================
// Misc.
// ====================================================================================================================

extern       Char   g_aucConvertToBit  [ MAX_CU_SIZE+1 ];   // from width to log2(width)-2

#ifndef ENC_DEC_TRACE
# define ENC_DEC_TRACE 0
#endif

#if ENC_DEC_TRACE
extern FILE*  g_hTrace;
extern Bool   g_bJustDoIt;
extern const Bool g_bEncDecTraceEnable;
extern const Bool g_bEncDecTraceDisable;
extern Bool   g_HLSTraceEnable;
extern UInt64 g_nSymbolCounter;

#define COUNTER_START    1
#define COUNTER_END      0 //( UInt64(1) << 63 )

#define DTRACE_CABAC_F(x)     if ( ( g_nSymbolCounter >= COUNTER_START && g_nSymbolCounter <= COUNTER_END )|| g_bJustDoIt ) fprintf( g_hTrace, "%f", x );
#define DTRACE_CABAC_V(x)     if ( ( g_nSymbolCounter >= COUNTER_START && g_nSymbolCounter <= COUNTER_END )|| g_bJustDoIt ) fprintf( g_hTrace, "%d", x );
#define DTRACE_CABAC_VL(x)    if ( ( g_nSymbolCounter >= COUNTER_START && g_nSymbolCounter <= COUNTER_END )|| g_bJustDoIt ) fprintf( g_hTrace, "%lld", x );
#define DTRACE_CABAC_T(x)     if ( ( g_nSymbolCounter >= COUNTER_START && g_nSymbolCounter <= COUNTER_END )|| g_bJustDoIt ) fprintf( g_hTrace, "%s", x );
#define DTRACE_CABAC_X(x)     if ( ( g_nSymbolCounter >= COUNTER_START && g_nSymbolCounter <= COUNTER_END )|| g_bJustDoIt ) fprintf( g_hTrace, "%x", x );
#define DTRACE_CABAC_R( x,y ) if ( ( g_nSymbolCounter >= COUNTER_START && g_nSymbolCounter <= COUNTER_END )|| g_bJustDoIt ) fprintf( g_hTrace, x,    y );
#define DTRACE_CABAC_N        if ( ( g_nSymbolCounter >= COUNTER_START && g_nSymbolCounter <= COUNTER_END )|| g_bJustDoIt ) fprintf( g_hTrace, "\n"    );

#else

#define DTRACE_CABAC_F(x)
#define DTRACE_CABAC_V(x)
#define DTRACE_CABAC_VL(x)
#define DTRACE_CABAC_T(x)
#define DTRACE_CABAC_X(x)
#define DTRACE_CABAC_R( x,y )
#define DTRACE_CABAC_N

#endif


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
static const Char MatrixType[4][6][20] =
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
static const Char MatrixType_DC[4][12][22] =
{
  {
  },
  {
  },
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
extern Int g_quantIntraDefault8x8[64];
extern Int g_quantIntraDefault16x16[256];
extern Int g_quantIntraDefault32x32[1024];
extern Int g_quantInterDefault8x8[64];
extern Int g_quantInterDefault16x16[256];
extern Int g_quantInterDefault32x32[1024];
extern Int g_quantTSDefault4x4[16];
extern UInt g_scalingListSize [SCALING_LIST_SIZE_NUM];
extern UInt g_scalingListSizeX[SCALING_LIST_SIZE_NUM];
extern UInt g_scalingListNum  [SCALING_LIST_SIZE_NUM];
extern Int  g_eTTable[4];
//! \}

#endif  //__TCOMROM__

