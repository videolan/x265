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

/** \file     TComRom.cpp
    \brief    global variables & functions
*/

#include "TComRom.h"
#include <memory.h>
#include <cstdlib>
#include <stdio.h>

namespace x265 {
//! \ingroup TLibCommon
//! \{

// initialize ROM variables
void initROM()
{
    if (g_sigLastScan[0][0] == 0)
    {
        int i, c;

        // g_convertToBit[ x ]: log2(x/4), if x=4 -> 0, x=8 -> 1, x=16 -> 2, ...
        ::memset(g_convertToBit, -1, sizeof(g_convertToBit));
        c = 0;
        for (i = 4; i < MAX_CU_SIZE; i *= 2)
        {
            g_convertToBit[i] = c;
            c++;
        }

        g_convertToBit[i] = c;

        c = 2;
        for (i = 0; i < MAX_CU_DEPTH; i++)
        {
            g_sigLastScan[0][i] = new uint32_t[c * c];
            g_sigLastScan[1][i] = new uint32_t[c * c];
            g_sigLastScan[2][i] = new uint32_t[c * c];
            initSigLastScan(g_sigLastScan[0][i], g_sigLastScan[1][i], g_sigLastScan[2][i], c, c);

            c <<= 1;
        }
    }
}

void destroyROM()
{
    if (g_sigLastScan[0][0])
    {
        for (int i = 0; i < MAX_CU_DEPTH; i++)
        {
            delete[] g_sigLastScan[0][i];
            delete[] g_sigLastScan[1][i];
            delete[] g_sigLastScan[2][i];
        }

        g_sigLastScan[0][0] = NULL;
    }
}

// ====================================================================================================================
// Data structure related table & variable
// ====================================================================================================================

int  g_bitDepth = 8;
uint32_t g_maxCUWidth  = MAX_CU_SIZE;
uint32_t g_maxCUHeight = MAX_CU_SIZE;
uint32_t g_maxCUDepth  = MAX_CU_DEPTH;
uint32_t g_addCUDepth  = 0;
uint32_t g_zscanToRaster[MAX_NUM_SPU_W * MAX_NUM_SPU_W] = { 0, };
uint32_t g_rasterToZscan[MAX_NUM_SPU_W * MAX_NUM_SPU_W] = { 0, };
uint32_t g_rasterToPelX[MAX_NUM_SPU_W * MAX_NUM_SPU_W] = { 0, };
uint32_t g_rasterToPelY[MAX_NUM_SPU_W * MAX_NUM_SPU_W] = { 0, };

uint32_t g_puOffset[8] = { 0, 8, 4, 4, 2, 10, 1, 5 };

void initZscanToRaster(int maxDepth, int depth, uint32_t startVal, uint32_t*& curIdx)
{
    int stride = 1 << (maxDepth - 1);

    if (depth == maxDepth)
    {
        curIdx[0] = startVal;
        curIdx++;
    }
    else
    {
        int step = stride >> depth;
        initZscanToRaster(maxDepth, depth + 1, startVal,                        curIdx);
        initZscanToRaster(maxDepth, depth + 1, startVal + step,                 curIdx);
        initZscanToRaster(maxDepth, depth + 1, startVal + step * stride,        curIdx);
        initZscanToRaster(maxDepth, depth + 1, startVal + step * stride + step, curIdx);
    }
}

void initRasterToZscan(uint32_t maxCUWidth, uint32_t maxCUHeight, uint32_t maxDepth)
{
    uint32_t  minWidth  = maxCUWidth  >> (maxDepth - 1);
    uint32_t  minHeight = maxCUHeight >> (maxDepth - 1);

    uint32_t  numPartInWidth  = (uint32_t)maxCUWidth  / minWidth;
    uint32_t  numPartInHeight = (uint32_t)maxCUHeight / minHeight;

    for (uint32_t i = 0; i < numPartInWidth * numPartInHeight; i++)
    {
        g_rasterToZscan[g_zscanToRaster[i]] = i;
    }
}

void initRasterToPelXY(uint32_t maxCUWidth, uint32_t maxCUHeight, uint32_t maxDepth)
{
    uint32_t i;

    uint32_t* tempX = &g_rasterToPelX[0];
    uint32_t* tempY = &g_rasterToPelY[0];

    uint32_t  minWidth  = maxCUWidth  >> (maxDepth - 1);
    uint32_t  minHeight = maxCUHeight >> (maxDepth - 1);

    uint32_t  numPartInWidth  = maxCUWidth  / minWidth;
    uint32_t  numPartInHeight = maxCUHeight / minHeight;

    tempX[0] = 0;
    tempX++;
    for (i = 1; i < numPartInWidth; i++)
    {
        tempX[0] = tempX[-1] + minWidth;
        tempX++;
    }

    for (i = 1; i < numPartInHeight; i++)
    {
        memcpy(tempX, tempX - numPartInWidth, sizeof(uint32_t) * numPartInWidth);
        tempX += numPartInWidth;
    }

    for (i = 1; i < numPartInWidth * numPartInHeight; i++)
    {
        tempY[i] = (i / numPartInWidth) * minWidth;
    }
}

const int16_t g_lumaFilter[4][NTAPS_LUMA] =
{
    {  0, 0,   0, 64,  0,   0, 0,  0 },
    { -1, 4, -10, 58, 17,  -5, 1,  0 },
    { -1, 4, -11, 40, 40, -11, 4, -1 },
    {  0, 1,  -5, 17, 58, -10, 4, -1 }
};

const int16_t g_chromaFilter[8][NTAPS_CHROMA] =
{
    {  0, 64,  0,  0 },
    { -2, 58, 10, -2 },
    { -4, 54, 16, -2 },
    { -6, 46, 28, -4 },
    { -4, 36, 36, -4 },
    { -4, 28, 46, -6 },
    { -2, 16, 54, -4 },
    { -2, 10, 58, -2 }
};

int g_quantScales[6] =
{
    26214, 23302, 20560, 18396, 16384, 14564
};

int g_invQuantScales[6] =
{
    40, 45, 51, 57, 64, 72
};

const int16_t g_t4[4][4] =
{
    { 64, 64, 64, 64 },
    { 83, 36, -36, -83 },
    { 64, -64, -64, 64 },
    { 36, -83, 83, -36 }
};

const int16_t g_t8[8][8] =
{
    { 64, 64, 64, 64, 64, 64, 64, 64 },
    { 89, 75, 50, 18, -18, -50, -75, -89 },
    { 83, 36, -36, -83, -83, -36, 36, 83 },
    { 75, -18, -89, -50, 50, 89, 18, -75 },
    { 64, -64, -64, 64, 64, -64, -64, 64 },
    { 50, -89, 18, 75, -75, -18, 89, -50 },
    { 36, -83, 83, -36, -36, 83, -83, 36 },
    { 18, -50, 75, -89, 89, -75, 50, -18 }
};

const int16_t g_t16[16][16] =
{
    { 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64 },
    { 90, 87, 80, 70, 57, 43, 25,  9, -9, -25, -43, -57, -70, -80, -87, -90 },
    { 89, 75, 50, 18, -18, -50, -75, -89, -89, -75, -50, -18, 18, 50, 75, 89 },
    { 87, 57,  9, -43, -80, -90, -70, -25, 25, 70, 90, 80, 43, -9, -57, -87 },
    { 83, 36, -36, -83, -83, -36, 36, 83, 83, 36, -36, -83, -83, -36, 36, 83 },
    { 80,  9, -70, -87, -25, 57, 90, 43, -43, -90, -57, 25, 87, 70, -9, -80 },
    { 75, -18, -89, -50, 50, 89, 18, -75, -75, 18, 89, 50, -50, -89, -18, 75 },
    { 70, -43, -87,  9, 90, 25, -80, -57, 57, 80, -25, -90, -9, 87, 43, -70 },
    { 64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64 },
    { 57, -80, -25, 90, -9, -87, 43, 70, -70, -43, 87,  9, -90, 25, 80, -57 },
    { 50, -89, 18, 75, -75, -18, 89, -50, -50, 89, -18, -75, 75, 18, -89, 50 },
    { 43, -90, 57, 25, -87, 70,  9, -80, 80, -9, -70, 87, -25, -57, 90, -43 },
    { 36, -83, 83, -36, -36, 83, -83, 36, 36, -83, 83, -36, -36, 83, -83, 36 },
    { 25, -70, 90, -80, 43,  9, -57, 87, -87, 57, -9, -43, 80, -90, 70, -25 },
    { 18, -50, 75, -89, 89, -75, 50, -18, -18, 50, -75, 89, -89, 75, -50, 18 },
    {  9, -25, 43, -57, 70, -80, 87, -90, 90, -87, 80, -70, 57, -43, 25, -9 }
};

const int16_t g_t32[32][32] =
{
    { 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64 },
    { 90, 90, 88, 85, 82, 78, 73, 67, 61, 54, 46, 38, 31, 22, 13,  4, -4, -13, -22, -31, -38, -46, -54, -61, -67, -73, -78, -82, -85, -88, -90, -90 },
    { 90, 87, 80, 70, 57, 43, 25,  9, -9, -25, -43, -57, -70, -80, -87, -90, -90, -87, -80, -70, -57, -43, -25, -9,  9, 25, 43, 57, 70, 80, 87, 90 },
    { 90, 82, 67, 46, 22, -4, -31, -54, -73, -85, -90, -88, -78, -61, -38, -13, 13, 38, 61, 78, 88, 90, 85, 73, 54, 31,  4, -22, -46, -67, -82, -90 },
    { 89, 75, 50, 18, -18, -50, -75, -89, -89, -75, -50, -18, 18, 50, 75, 89, 89, 75, 50, 18, -18, -50, -75, -89, -89, -75, -50, -18, 18, 50, 75, 89 },
    { 88, 67, 31, -13, -54, -82, -90, -78, -46, -4, 38, 73, 90, 85, 61, 22, -22, -61, -85, -90, -73, -38,  4, 46, 78, 90, 82, 54, 13, -31, -67, -88 },
    { 87, 57,  9, -43, -80, -90, -70, -25, 25, 70, 90, 80, 43, -9, -57, -87, -87, -57, -9, 43, 80, 90, 70, 25, -25, -70, -90, -80, -43,  9, 57, 87 },
    { 85, 46, -13, -67, -90, -73, -22, 38, 82, 88, 54, -4, -61, -90, -78, -31, 31, 78, 90, 61,  4, -54, -88, -82, -38, 22, 73, 90, 67, 13, -46, -85 },
    { 83, 36, -36, -83, -83, -36, 36, 83, 83, 36, -36, -83, -83, -36, 36, 83, 83, 36, -36, -83, -83, -36, 36, 83, 83, 36, -36, -83, -83, -36, 36, 83 },
    { 82, 22, -54, -90, -61, 13, 78, 85, 31, -46, -90, -67,  4, 73, 88, 38, -38, -88, -73, -4, 67, 90, 46, -31, -85, -78, -13, 61, 90, 54, -22, -82 },
    { 80,  9, -70, -87, -25, 57, 90, 43, -43, -90, -57, 25, 87, 70, -9, -80, -80, -9, 70, 87, 25, -57, -90, -43, 43, 90, 57, -25, -87, -70,  9, 80 },
    { 78, -4, -82, -73, 13, 85, 67, -22, -88, -61, 31, 90, 54, -38, -90, -46, 46, 90, 38, -54, -90, -31, 61, 88, 22, -67, -85, -13, 73, 82,  4, -78 },
    { 75, -18, -89, -50, 50, 89, 18, -75, -75, 18, 89, 50, -50, -89, -18, 75, 75, -18, -89, -50, 50, 89, 18, -75, -75, 18, 89, 50, -50, -89, -18, 75 },
    { 73, -31, -90, -22, 78, 67, -38, -90, -13, 82, 61, -46, -88, -4, 85, 54, -54, -85,  4, 88, 46, -61, -82, 13, 90, 38, -67, -78, 22, 90, 31, -73 },
    { 70, -43, -87,  9, 90, 25, -80, -57, 57, 80, -25, -90, -9, 87, 43, -70, -70, 43, 87, -9, -90, -25, 80, 57, -57, -80, 25, 90,  9, -87, -43, 70 },
    { 67, -54, -78, 38, 85, -22, -90,  4, 90, 13, -88, -31, 82, 46, -73, -61, 61, 73, -46, -82, 31, 88, -13, -90, -4, 90, 22, -85, -38, 78, 54, -67 },
    { 64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64 },
    { 61, -73, -46, 82, 31, -88, -13, 90, -4, -90, 22, 85, -38, -78, 54, 67, -67, -54, 78, 38, -85, -22, 90,  4, -90, 13, 88, -31, -82, 46, 73, -61 },
    { 57, -80, -25, 90, -9, -87, 43, 70, -70, -43, 87,  9, -90, 25, 80, -57, -57, 80, 25, -90,  9, 87, -43, -70, 70, 43, -87, -9, 90, -25, -80, 57 },
    { 54, -85, -4, 88, -46, -61, 82, 13, -90, 38, 67, -78, -22, 90, -31, -73, 73, 31, -90, 22, 78, -67, -38, 90, -13, -82, 61, 46, -88,  4, 85, -54 },
    { 50, -89, 18, 75, -75, -18, 89, -50, -50, 89, -18, -75, 75, 18, -89, 50, 50, -89, 18, 75, -75, -18, 89, -50, -50, 89, -18, -75, 75, 18, -89, 50 },
    { 46, -90, 38, 54, -90, 31, 61, -88, 22, 67, -85, 13, 73, -82,  4, 78, -78, -4, 82, -73, -13, 85, -67, -22, 88, -61, -31, 90, -54, -38, 90, -46 },
    { 43, -90, 57, 25, -87, 70,  9, -80, 80, -9, -70, 87, -25, -57, 90, -43, -43, 90, -57, -25, 87, -70, -9, 80, -80,  9, 70, -87, 25, 57, -90, 43 },
    { 38, -88, 73, -4, -67, 90, -46, -31, 85, -78, 13, 61, -90, 54, 22, -82, 82, -22, -54, 90, -61, -13, 78, -85, 31, 46, -90, 67,  4, -73, 88, -38 },
    { 36, -83, 83, -36, -36, 83, -83, 36, 36, -83, 83, -36, -36, 83, -83, 36, 36, -83, 83, -36, -36, 83, -83, 36, 36, -83, 83, -36, -36, 83, -83, 36 },
    { 31, -78, 90, -61,  4, 54, -88, 82, -38, -22, 73, -90, 67, -13, -46, 85, -85, 46, 13, -67, 90, -73, 22, 38, -82, 88, -54, -4, 61, -90, 78, -31 },
    { 25, -70, 90, -80, 43,  9, -57, 87, -87, 57, -9, -43, 80, -90, 70, -25, -25, 70, -90, 80, -43, -9, 57, -87, 87, -57,  9, 43, -80, 90, -70, 25 },
    { 22, -61, 85, -90, 73, -38, -4, 46, -78, 90, -82, 54, -13, -31, 67, -88, 88, -67, 31, 13, -54, 82, -90, 78, -46,  4, 38, -73, 90, -85, 61, -22 },
    { 18, -50, 75, -89, 89, -75, 50, -18, -18, 50, -75, 89, -89, 75, -50, 18, 18, -50, 75, -89, 89, -75, 50, -18, -18, 50, -75, 89, -89, 75, -50, 18 },
    { 13, -38, 61, -78, 88, -90, 85, -73, 54, -31,  4, 22, -46, 67, -82, 90, -90, 82, -67, 46, -22, -4, 31, -54, 73, -85, 90, -88, 78, -61, 38, -13 },
    {  9, -25, 43, -57, 70, -80, 87, -90, 90, -87, 80, -70, 57, -43, 25, -9, -9, 25, -43, 57, -70, 80, -87, 90, -90, 87, -80, 70, -57, 43, -25,  9 },
    {  4, -13, 22, -31, 38, -46, 54, -61, 67, -73, 78, -82, 85, -88, 90, -90, 90, -90, 88, -85, 82, -78, 73, -67, 61, -54, 46, -38, 31, -22, 13, -4 }
};

const UChar g_chromaScale[58] =
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 29, 30, 31, 32,
    33, 33, 34, 34, 35, 35, 36, 36, 37, 37, 38, 39, 40, 41, 42, 43, 44,
    45, 46, 47, 48, 49, 50, 51
};

// ====================================================================================================================
// ADI
// ====================================================================================================================

const UChar g_intraModeNumFast[7] =
{
    3, //   2x2
    8, //   4x4
    8, //   8x8
    3, //  16x16
    3, //  32x32
    3, //  64x64
    3 // 128x128
};

// chroma

const UChar g_convertTxtTypeToIdx[4] = { 0, 1, 1, 2 };

// ====================================================================================================================
// Misc.
// ====================================================================================================================

char g_convertToBit[MAX_CU_SIZE + 1];

#if ENC_DEC_TRACE
FILE*  g_hTrace = NULL;
const bool g_bEncDecTraceEnable  = true;
const bool g_bEncDecTraceDisable = false;
bool   g_HLSTraceEnable = true;
bool   g_bJustDoIt = false;
UInt64 g_nSymbolCounter = 0;
#endif

// ====================================================================================================================
// Scanning order & context model mapping
// ====================================================================================================================

// scanning order table
uint32_t* g_sigLastScan[3][MAX_CU_DEPTH];

const uint32_t g_sigLastScan8x8[3][4] =
{
    { 0, 2, 1, 3 },
    { 0, 1, 2, 3 },
    { 0, 2, 1, 3 }
};
uint32_t g_sigLastScanCG32x32[64];

const uint32_t g_minInGroup[10] = { 0, 1, 2, 3, 4, 6, 8, 12, 16, 24 };
const uint32_t g_groupIdx[32]   = { 0, 1, 2, 3, 4, 4, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9 };

// Rice parameters for absolute transform levels
const uint32_t g_goRiceRange[5] = { 7, 14, 26, 46, 78 };

const uint32_t g_goRicePrefixLen[5] = { 8, 7, 6, 5, 4 };

void initSigLastScan(uint32_t* buffD, uint32_t* buffH, uint32_t* buffV, int width, int height)
{
    const uint32_t  numScanPos  = uint32_t(width * width);
    uint32_t        nextScanPos = 0;

    if (width < 16)
    {
        uint32_t* buffTemp = buffD;
        if (width == 8)
        {
            buffTemp = g_sigLastScanCG32x32;
        }
        for (uint32_t scanLine = 0; nextScanPos < numScanPos; scanLine++)
        {
            int primDim = int(scanLine);
            int scndDim = 0;
            while (primDim >= width)
            {
                scndDim++;
                primDim--;
            }

            while (primDim >= 0 && scndDim < width)
            {
                buffTemp[nextScanPos] = primDim * width + scndDim;
                nextScanPos++;
                scndDim++;
                primDim--;
            }
        }
    }
    if (width > 4)
    {
        uint32_t numBlkSide = width >> 2;
        uint32_t numBlks    = numBlkSide * numBlkSide;
        uint32_t log2Blk    = g_convertToBit[numBlkSide] + 1;

        for (uint32_t blk = 0; blk < numBlks; blk++)
        {
            nextScanPos   = 0;
            uint32_t initBlkPos = g_sigLastScan[SCAN_DIAG][log2Blk][blk];
            if (width == 32)
            {
                initBlkPos = g_sigLastScanCG32x32[blk];
            }
            uint32_t offsetY    = initBlkPos / numBlkSide;
            uint32_t offsetX    = initBlkPos - offsetY * numBlkSide;
            uint32_t offsetD    = 4 * (offsetX + offsetY * width);
            uint32_t offsetScan = 16 * blk;
            for (uint32_t scanLine = 0; nextScanPos < 16; scanLine++)
            {
                int primDim = int(scanLine);
                int scndDim = 0;
                while (primDim >= 4)
                {
                    scndDim++;
                    primDim--;
                }

                while (primDim >= 0 && scndDim < 4)
                {
                    buffD[nextScanPos + offsetScan] = primDim * width + scndDim + offsetD;
                    nextScanPos++;
                    scndDim++;
                    primDim--;
                }
            }
        }
    }

    uint32_t cnt = 0;
    if (width > 2)
    {
        uint32_t numBlkSide = width >> 2;
        for (int blkY = 0; blkY < numBlkSide; blkY++)
        {
            for (int blkX = 0; blkX < numBlkSide; blkX++)
            {
                uint32_t offset = blkY * 4 * width + blkX * 4;
                for (int y = 0; y < 4; y++)
                {
                    for (int x = 0; x < 4; x++)
                    {
                        buffH[cnt] = y * width + x + offset;
                        cnt++;
                    }
                }
            }
        }

        cnt = 0;
        for (int blkX = 0; blkX < numBlkSide; blkX++)
        {
            for (int blkY = 0; blkY < numBlkSide; blkY++)
            {
                uint32_t offset    = blkY * 4 * width + blkX * 4;
                for (int x = 0; x < 4; x++)
                {
                    for (int y = 0; y < 4; y++)
                    {
                        buffV[cnt] = y * width + x + offset;
                        cnt++;
                    }
                }
            }
        }
    }
    else
    {
        for (int y = 0; y < height; y++)
        {
            for (int iX = 0; iX < width; iX++)
            {
                buffH[cnt] = y * width + iX;
                cnt++;
            }
        }

        cnt = 0;
        for (int x = 0; x < width; x++)
        {
            for (int iY = 0; iY < height; iY++)
            {
                buffV[cnt] = iY * width + x;
                cnt++;
            }
        }
    }
}

int g_quantTSDefault4x4[16] =
{
    16, 16, 16, 16,
    16, 16, 16, 16,
    16, 16, 16, 16,
    16, 16, 16, 16
};

int g_quantIntraDefault8x8[64] =
{
    16, 16, 16, 16, 17, 18, 21, 24,
    16, 16, 16, 16, 17, 19, 22, 25,
    16, 16, 17, 18, 20, 22, 25, 29,
    16, 16, 18, 21, 24, 27, 31, 36,
    17, 17, 20, 24, 30, 35, 41, 47,
    18, 19, 22, 27, 35, 44, 54, 65,
    21, 22, 25, 31, 41, 54, 70, 88,
    24, 25, 29, 36, 47, 65, 88, 115
};

int g_quantInterDefault8x8[64] =
{
    16, 16, 16, 16, 17, 18, 20, 24,
    16, 16, 16, 17, 18, 20, 24, 25,
    16, 16, 17, 18, 20, 24, 25, 28,
    16, 17, 18, 20, 24, 25, 28, 33,
    17, 18, 20, 24, 25, 28, 33, 41,
    18, 20, 24, 25, 28, 33, 41, 54,
    20, 24, 25, 28, 33, 41, 54, 71,
    24, 25, 28, 33, 41, 54, 71, 91
};
uint32_t g_scalingListSize[4] = { 16, 64, 256, 1024 };
uint32_t g_scalingListSizeX[4] = { 4, 8, 16,  32 };
uint32_t g_scalingListNum[SCALING_LIST_SIZE_NUM] = { 6, 6, 6, 2 };
int  g_eTTable[4] = { 0, 3, 1, 2 };

const int g_winUnitX[] = { 1, 2, 2, 1 };
const int g_winUnitY[] = { 1, 2, 1, 1 };

const double x265_lambda2_tab_I[MAX_QP + 1] =
{
    0.012265625, 0.022265625, 0.028052813, 0.035344375, 0.04453125, 0.056105938, 0.070689063, 0.0890625,
    0.112211563, 0.141377813, 0.178125, 0.224423438, 0.282755938, 0.35625, 0.448846875,
    0.565511563, 0.7125, 0.89769375, 1.131023125, 1.425, 1.7953875, 2.262046563, 2.85,
    3.590775, 4.524093125, 5.7, 7.7800125, 10.55621688, 14.25, 19.1508, 25.63652688, 34.2,
    45.48315, 60.32124, 79.8, 105.3293997, 138.7388519, 182.4, 229.8095994, 289.5419519, 364.8,
    459.6191991, 579.0839038, 729.6, 919.2383981, 1158.167808, 1459.2, 1838.476796, 2316.335615,
    2918.4, 3676.953592, 4632.67123
};

const double x265_lambda2_non_I[MAX_QP + 1] =
{
    0.05231, 0.060686, 0.07646, 0.096333333, 0.151715667, 0.15292, 0.192666667, 0.242745, 0.382299,
    0.385333333, 0.485489333, 0.611678333, 0.963333333, 0.970979, 1.223356667, 1.541333333, 2.427447667,
    2.446714, 3.082666667, 3.883916667, 6.116785, 6.165333333, 7.767833333, 9.786856667, 15.41333333,
    16.57137733, 22.183542, 29.5936, 39.357022, 52.19656867, 69.05173333, 91.14257667, 150.0651347,
    157.8325333, 207.1422197, 271.4221573, 443.904, 447.4271947, 563.722941, 710.2464, 1118.567987,
    1127.445883, 1420.4928, 1789.70878, 2818.614706, 2840.9856, 3579.41756, 4509.78353, 7102.464,
    7158.83512, 9019.56706, 11363.9424
};

const UChar g_lpsTable[64][4] =
{
    { 128, 176, 208, 240 },
    { 128, 167, 197, 227 },
    { 128, 158, 187, 216 },
    { 123, 150, 178, 205 },
    { 116, 142, 169, 195 },
    { 111, 135, 160, 185 },
    { 105, 128, 152, 175 },
    { 100, 122, 144, 166 },
    {  95, 116, 137, 158 },
    {  90, 110, 130, 150 },
    {  85, 104, 123, 142 },
    {  81,  99, 117, 135 },
    {  77,  94, 111, 128 },
    {  73,  89, 105, 122 },
    {  69,  85, 100, 116 },
    {  66,  80,  95, 110 },
    {  62,  76,  90, 104 },
    {  59,  72,  86,  99 },
    {  56,  69,  81,  94 },
    {  53,  65,  77,  89 },
    {  51,  62,  73,  85 },
    {  48,  59,  69,  80 },
    {  46,  56,  66,  76 },
    {  43,  53,  63,  72 },
    {  41,  50,  59,  69 },
    {  39,  48,  56,  65 },
    {  37,  45,  54,  62 },
    {  35,  43,  51,  59 },
    {  33,  41,  48,  56 },
    {  32,  39,  46,  53 },
    {  30,  37,  43,  50 },
    {  29,  35,  41,  48 },
    {  27,  33,  39,  45 },
    {  26,  31,  37,  43 },
    {  24,  30,  35,  41 },
    {  23,  28,  33,  39 },
    {  22,  27,  32,  37 },
    {  21,  26,  30,  35 },
    {  20,  24,  29,  33 },
    {  19,  23,  27,  31 },
    {  18,  22,  26,  30 },
    {  17,  21,  25,  28 },
    {  16,  20,  23,  27 },
    {  15,  19,  22,  25 },
    {  14,  18,  21,  24 },
    {  14,  17,  20,  23 },
    {  13,  16,  19,  22 },
    {  12,  15,  18,  21 },
    {  12,  14,  17,  20 },
    {  11,  14,  16,  19 },
    {  11,  13,  15,  18 },
    {  10,  12,  15,  17 },
    {  10,  12,  14,  16 },
    {   9,  11,  13,  15 },
    {   9,  11,  12,  14 },
    {   8,  10,  12,  14 },
    {   8,   9,  11,  13 },
    {   7,   9,  11,  12 },
    {   7,   9,  10,  12 },
    {   7,   8,  10,  11 },
    {   6,   8,   9,  11 },
    {   6,   7,   9,  10 },
    {   6,   7,   8,   9 },
    {   2,   2,   2,   2 }
};

const UChar g_renormTable[32] =
{
    6,  5,  4,  4,
    3,  3,  3,  3,
    2,  2,  2,  2,
    2,  2,  2,  2,
    1,  1,  1,  1,
    1,  1,  1,  1,
    1,  1,  1,  1,
    1,  1,  1,  1
};

const UChar x265_exp2_lut[64] =
{
    0,  3,  6,  8,  11, 14,  17,  20,  23,  26,  29,  32,  36,  39,  42,  45,
    48,  52,  55,  58,  62,  65,  69,  72,  76,  80,  83,  87,  91,  94,  98,  102,
    106,  110,  114,  118,  122,  126,  130,  135,  139,  143,  147,  152,  156,  161,  165,  170,
    175,  179,  184,  189,  194,  198,  203,  208,  214,  219,  224,  229,  234,  240,  245,  250
};
}
//! \}
