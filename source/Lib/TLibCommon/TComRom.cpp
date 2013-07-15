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
#include <stdlib.h>
#include <stdio.h>
// ====================================================================================================================
// Initialize / destroy functions
// ====================================================================================================================

//! \ingroup TLibCommon
//! \{

// initialize ROM variables
Void initROM()
{
    if (g_sigLastScan[0][0] == 0)
    {
        Int i, c;

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
            g_sigLastScan[0][i] = new UInt[c * c];
            g_sigLastScan[1][i] = new UInt[c * c];
            g_sigLastScan[2][i] = new UInt[c * c];
            initSigLastScan(g_sigLastScan[0][i], g_sigLastScan[1][i], g_sigLastScan[2][i], c, c);

            c <<= 1;
        }
    }
}

Void destroyROM()
{
    if (g_sigLastScan[0][0])
    {
        for (Int i = 0; i < MAX_CU_DEPTH; i++)
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

UInt g_maxCUWidth  = MAX_CU_SIZE;
UInt g_maxCUHeight = MAX_CU_SIZE;
UInt g_maxCUDepth  = MAX_CU_DEPTH;
UInt g_addCUDepth  = 0;
UInt g_zscanToRaster[MAX_NUM_SPU_W * MAX_NUM_SPU_W] = { 0, };
UInt g_rasterToZscan[MAX_NUM_SPU_W * MAX_NUM_SPU_W] = { 0, };
UInt g_rasterToPelX[MAX_NUM_SPU_W * MAX_NUM_SPU_W] = { 0, };
UInt g_rasterToPelY[MAX_NUM_SPU_W * MAX_NUM_SPU_W] = { 0, };

UInt g_auiPUOffset[8] = { 0, 8, 4, 4, 2, 10, 1, 5 };

Void initZscanToRaster(Int maxDepth, Int depth, UInt startVal, UInt*& curIdx)
{
    Int stride = 1 << (maxDepth - 1);

    if (depth == maxDepth)
    {
        curIdx[0] = startVal;
        curIdx++;
    }
    else
    {
        Int step = stride >> depth;
        initZscanToRaster(maxDepth, depth + 1, startVal,                        curIdx);
        initZscanToRaster(maxDepth, depth + 1, startVal + step,                 curIdx);
        initZscanToRaster(maxDepth, depth + 1, startVal + step * stride,        curIdx);
        initZscanToRaster(maxDepth, depth + 1, startVal + step * stride + step, curIdx);
    }
}

Void initRasterToZscan(UInt maxCUWidth, UInt maxCUHeight, UInt maxDepth)
{
    UInt  minWidth  = maxCUWidth  >> (maxDepth - 1);
    UInt  minHeight = maxCUHeight >> (maxDepth - 1);

    UInt  numPartInWidth  = (UInt)maxCUWidth  / minWidth;
    UInt  numPartInHeight = (UInt)maxCUHeight / minHeight;

    for (UInt i = 0; i < numPartInWidth * numPartInHeight; i++)
    {
        g_rasterToZscan[g_zscanToRaster[i]] = i;
    }
}

Void initRasterToPelXY(UInt maxCUWidth, UInt maxCUHeight, UInt maxDepth)
{
    UInt i;

    UInt* tempX = &g_rasterToPelX[0];
    UInt* tempY = &g_rasterToPelY[0];

    UInt  minWidth  = maxCUWidth  >> (maxDepth - 1);
    UInt  minHeight = maxCUHeight >> (maxDepth - 1);

    UInt  numPartInWidth  = maxCUWidth  / minWidth;
    UInt  numPartInHeight = maxCUHeight / minHeight;

    tempX[0] = 0;
    tempX++;
    for (i = 1; i < numPartInWidth; i++)
    {
        tempX[0] = tempX[-1] + minWidth;
        tempX++;
    }

    for (i = 1; i < numPartInHeight; i++)
    {
        memcpy(tempX, tempX - numPartInWidth, sizeof(UInt) * numPartInWidth);
        tempX += numPartInWidth;
    }

    for (i = 1; i < numPartInWidth * numPartInHeight; i++)
    {
        tempY[i] = (i / numPartInWidth) * minWidth;
    }
}

const Short g_lumaFilter[4][NTAPS_LUMA] =
{
    {  0, 0,   0, 64,  0,   0, 0,  0 },
    { -1, 4, -10, 58, 17,  -5, 1,  0 },
    { -1, 4, -11, 40, 40, -11, 4, -1 },
    {  0, 1,  -5, 17, 58, -10, 4, -1 }
};

const Short g_chromaFilter[8][NTAPS_CHROMA] =
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

Int g_quantScales[6] =
{
    26214, 23302, 20560, 18396, 16384, 14564
};

Int g_invQuantScales[6] =
{
    40, 45, 51, 57, 64, 72
};

const Short g_t4[4][4] =
{
    { 64, 64, 64, 64 },
    { 83, 36, -36, -83 },
    { 64, -64, -64, 64 },
    { 36, -83, 83, -36 }
};

const Short g_t8[8][8] =
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

const Short g_t16[16][16] =
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

const Short g_t32[32][32] =
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
// Bit-depth
// ====================================================================================================================

Int  g_bitDepthY = 8;
Int  g_bitDepthC = 8;

// ====================================================================================================================
// Misc.
// ====================================================================================================================

Char g_convertToBit[MAX_CU_SIZE + 1];

#if ENC_DEC_TRACE
FILE*  g_hTrace = NULL;
const Bool g_bEncDecTraceEnable  = true;
const Bool g_bEncDecTraceDisable = false;
Bool   g_HLSTraceEnable = true;
Bool   g_bJustDoIt = false;
UInt64 g_nSymbolCounter = 0;
#endif

// ====================================================================================================================
// Scanning order & context model mapping
// ====================================================================================================================

// scanning order table
UInt* g_sigLastScan[3][MAX_CU_DEPTH];

const UInt g_sigLastScan8x8[3][4] =
{
    { 0, 2, 1, 3 },
    { 0, 1, 2, 3 },
    { 0, 2, 1, 3 }
};
UInt g_sigLastScanCG32x32[64];

const UInt g_minInGroup[10] = { 0, 1, 2, 3, 4, 6, 8, 12, 16, 24 };
const UInt g_groupIdx[32]   = { 0, 1, 2, 3, 4, 4, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9 };

// Rice parameters for absolute transform levels
const UInt g_goRiceRange[5] = { 7, 14, 26, 46, 78 };

const UInt g_goRicePrefixLen[5] = { 8, 7, 6, 5, 4 };

Void initSigLastScan(UInt* buffD, UInt* buffH, UInt* buffV, Int width, Int height)
{
    const UInt  numScanPos  = UInt(width * width);
    UInt        nextScanPos = 0;

    if (width < 16)
    {
        UInt* buffTemp = buffD;
        if (width == 8)
        {
            buffTemp = g_sigLastScanCG32x32;
        }
        for (UInt scanLine = 0; nextScanPos < numScanPos; scanLine++)
        {
            Int primDim = Int(scanLine);
            Int scndDim = 0;
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
        UInt numBlkSide = width >> 2;
        UInt numBlks    = numBlkSide * numBlkSide;
        UInt log2Blk    = g_convertToBit[numBlkSide] + 1;

        for (UInt blk = 0; blk < numBlks; blk++)
        {
            nextScanPos   = 0;
            UInt initBlkPos = g_sigLastScan[SCAN_DIAG][log2Blk][blk];
            if (width == 32)
            {
                initBlkPos = g_sigLastScanCG32x32[blk];
            }
            UInt offsetY    = initBlkPos / numBlkSide;
            UInt offsetX    = initBlkPos - offsetY * numBlkSide;
            UInt offsetD    = 4 * (offsetX + offsetY * width);
            UInt offsetScan = 16 * blk;
            for (UInt scanLine = 0; nextScanPos < 16; scanLine++)
            {
                Int primDim = Int(scanLine);
                Int scndDim = 0;
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

    UInt cnt = 0;
    if (width > 2)
    {
        UInt numBlkSide = width >> 2;
        for (Int blkY = 0; blkY < numBlkSide; blkY++)
        {
            for (Int blkX = 0; blkX < numBlkSide; blkX++)
            {
                UInt offset = blkY * 4 * width + blkX * 4;
                for (Int y = 0; y < 4; y++)
                {
                    for (Int x = 0; x < 4; x++)
                    {
                        buffH[cnt] = y * width + x + offset;
                        cnt++;
                    }
                }
            }
        }

        cnt = 0;
        for (Int blkX = 0; blkX < numBlkSide; blkX++)
        {
            for (Int blkY = 0; blkY < numBlkSide; blkY++)
            {
                UInt offset    = blkY * 4 * width + blkX * 4;
                for (Int x = 0; x < 4; x++)
                {
                    for (Int y = 0; y < 4; y++)
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
        for (Int y = 0; y < height; y++)
        {
            for (Int iX = 0; iX < width; iX++)
            {
                buffH[cnt] = y * width + iX;
                cnt++;
            }
        }

        cnt = 0;
        for (Int x = 0; x < width; x++)
        {
            for (Int iY = 0; iY < height; iY++)
            {
                buffV[cnt] = iY * width + x;
                cnt++;
            }
        }
    }
}

Int g_quantTSDefault4x4[16] =
{
    16, 16, 16, 16,
    16, 16, 16, 16,
    16, 16, 16, 16,
    16, 16, 16, 16
};

Int g_quantIntraDefault8x8[64] =
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

Int g_quantInterDefault8x8[64] =
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
UInt g_scalingListSize[4] = { 16, 64, 256, 1024 };
UInt g_scalingListSizeX[4] = { 4, 8, 16,  32 };
UInt g_scalingListNum[SCALING_LIST_SIZE_NUM] = { 6, 6, 6, 2 };
Int  g_eTTable[4] = { 0, 3, 1, 2 };

const Int g_winUnitX[] = { 1, 2, 2, 1 };
const Int g_winUnitY[] = { 1, 2, 1, 1 };

//! \}
