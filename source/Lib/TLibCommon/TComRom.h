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

#include "common.h"

namespace x265 {
// private namespace

#define SLFASE_CONSTANT 0x5f4e4a53

void initROM();
void destroyROM();

static const int chromaQPMappingTableSize = 70;

extern const uint8_t g_chromaScale[chromaQPMappingTableSize];
extern const uint8_t g_chroma422IntraAngleMappingTable[36];

// flexible conversion from relative to absolute index
extern uint32_t g_zscanToRaster[MAX_NUM_PARTITIONS];
extern uint32_t g_rasterToZscan[MAX_NUM_PARTITIONS];

void initZscanToRaster(uint32_t maxFullDepth, uint32_t depth, uint32_t startVal, uint32_t*& curIdx);
void initRasterToZscan(uint32_t maxFullDepth);

// conversion of partition index to picture pel position
extern const uint8_t g_zscanToPelX[MAX_NUM_PARTITIONS];
extern const uint8_t g_zscanToPelY[MAX_NUM_PARTITIONS];

// global variable (CTU width/height, max. CU depth)
extern uint32_t g_maxLog2CUSize;
extern uint32_t g_maxCUSize;
extern uint32_t g_maxCUDepth;
extern uint32_t g_maxFullDepth;

extern const uint32_t g_puOffset[8];

extern const int16_t g_t4[4][4];
extern const int16_t g_t8[8][8];
extern const int16_t g_t16[16][16];
extern const int16_t g_t32[32][32];

// Subpel interpolation defines and constants

#define NTAPS_LUMA        8                            ///< Number of taps for luma
#define NTAPS_CHROMA      4                            ///< Number of taps for chroma
#define IF_INTERNAL_PREC 14                            ///< Number of bits for internal precision
#define IF_FILTER_PREC    6                            ///< Log2 of sum of filter taps
#define IF_INTERNAL_OFFS (1 << (IF_INTERNAL_PREC - 1)) ///< Offset used internally

extern const int16_t g_lumaFilter[4][NTAPS_LUMA];     ///< Luma filter taps
extern const int16_t g_chromaFilter[8][NTAPS_CHROMA]; ///< Chroma filter taps

// Scanning order & context mapping table

// coefficient scanning type used in ACS
enum ScanType
{
    SCAN_DIAG = 0,     // up-right diagonal scan
    SCAN_HOR = 1,      // horizontal first scan
    SCAN_VER = 2,      // vertical first scan
    NUM_SCAN_TYPE = 3
};

enum SignificanceMapContextType
{
    CONTEXT_TYPE_4x4 = 0,
    CONTEXT_TYPE_8x8 = 1,
    CONTEXT_TYPE_NxN = 2,
    CONTEXT_NUMBER_OF_TYPES = 3
};

#define NUM_SCAN_SIZE 4

extern const uint16_t* const g_scanOrder[NUM_SCAN_TYPE][NUM_SCAN_SIZE];
extern const uint16_t* const g_scanOrderCG[NUM_SCAN_TYPE][NUM_SCAN_SIZE];
extern const uint16_t g_scan8x8diag[8 * 8];
extern const uint16_t g_scan4x4[NUM_SCAN_TYPE][4 * 4];

extern const uint8_t g_minInGroup[10];
extern const uint8_t g_goRiceRange[5]; // maximum value coded with Rice codes

extern const uint8_t g_log2Size[MAX_CU_SIZE + 1]; // from size to log2(size)

// Map Luma samples to chroma samples
extern const int g_winUnitX[MAX_CHROMA_FORMAT_IDC + 1];
extern const int g_winUnitY[MAX_CHROMA_FORMAT_IDC + 1];

extern double x265_lambda_tab[QP_MAX_MAX + 1];
extern double x265_lambda2_tab[QP_MAX_MAX + 1];
extern const uint16_t x265_chroma_lambda2_offset_tab[MAX_CHROMA_LAMBDA_OFFSET+1];

// CABAC tables
extern const uint8_t g_lpsTable[64][4];
extern const uint8_t x265_exp2_lut[64];

// Intra tables
extern const uint8_t g_intraFilterFlags[NUM_INTRA_MODE];

extern const uint32_t g_depthInc[3][4];

extern const uint32_t g_depthScanIdx[8][8];

}

#endif  //ifndef X265_TCOMROM_H
