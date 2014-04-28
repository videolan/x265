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
#include "threading.h"
#include "common.h"

namespace x265 {
//! \ingroup TLibCommon
//! \{
// scanning order table
uint16_t* g_scanOrder[SCAN_NUMBER_OF_GROUP_TYPES][SCAN_NUMBER_OF_TYPES][MAX_CU_DEPTH];

class ScanGenerator
{
private:

    uint32_t m_line, m_column;
    uint32_t m_blockWidth, m_blockHeight;
    uint32_t m_stride;
    COEFF_SCAN_TYPE m_scanType;

public:

    ScanGenerator(uint32_t blockWidth, uint32_t blockHeight, uint32_t stride, COEFF_SCAN_TYPE scanType)
        : m_line(0), m_column(0), m_blockWidth(blockWidth), m_blockHeight(blockHeight), m_stride(stride), m_scanType(scanType)
    { }

    uint32_t GetCurrentX() const { return m_column; }

    uint32_t GetCurrentY() const { return m_line; }

    uint32_t GetNextIndex(uint32_t blockOffsetX, uint32_t blockOffsetY)
    {
        int rtn = ((m_line + blockOffsetY) * m_stride) + m_column + blockOffsetX;

        //advance line and column to the next position
        switch (m_scanType)
        {
        case SCAN_DIAG:
        {
            if ((m_column == (m_blockWidth - 1)) || (m_line == 0))     //if we reach the end of a rank, go diagonally down to the next one
            {
                m_line   += m_column + 1;
                m_column  = 0;

                if (m_line >= m_blockHeight)     //if that takes us outside the block, adjust so that we are back on the bottom row
                {
                    m_column += m_line - (m_blockHeight - 1);
                    m_line    = m_blockHeight - 1;
                }
            }
            else
            {
                m_column++;
                m_line--;
            }
        }
        break;

        case SCAN_HOR:
        {
            if (m_column == (m_blockWidth - 1))
            {
                m_line++;
                m_column = 0;
            }
            else m_column++;
        }
        break;

        case SCAN_VER:
        {
            if (m_line == (m_blockHeight - 1))
            {
                m_column++;
                m_line = 0;
            }
            else m_line++;
        }
        break;

        default:
        {
            std::cerr << "ERROR: Unknown scan type \"" << m_scanType << "\"in ScanGenerator::GetNextIndex" << std::endl;
            exit(1);
        }
        break;
        }

        return rtn;
    }
};

// lambda = pow(2, (double)q / 6 - 2);
const double x265_lambda_tab[MAX_MAX_QP + 1] = {
    0.2500, 0.2806, 0.3150, 0.3536, 0.3969, 
    0.4454, 0.5000, 0.5612, 0.6300, 0.7071, 
    0.7937, 0.8909, 1.0000, 1.1225, 1.2599, 
    1.4142, 1.5874, 1.7818, 2.0000, 2.2449, 
    2.5198, 2.8284, 3.1748, 3.5636, 4.0000, 
    4.4898, 5.0397, 5.6569, 6.3496, 7.1272, 
    8.0000, 8.9797, 10.0794, 11.3137, 12.6992, 
    14.2544, 16.0000, 17.9594, 20.1587, 22.6274, 
    25.3984, 28.5088, 32.0000, 35.9188, 40.3175, 
    45.2548, 50.7968, 57.0175, 64.0000, 71.8376, 
    80.6349, 90.5097, 101.5937, 114.0350, 128.0000, 
    143.6751, 161.2699, 181.0193, 203.1873, 228.0701, 
    256.0000, 287.3503, 322.5398, 362.0387, 406.3747, 
    456.1401, 512.0000, 574.7006, 645.0796, 724.0773
};

// lambda2 = pow(lambda, 2) * scale (0.85);
const double x265_lambda2_tab[MAX_MAX_QP + 1] = {
    0.0531, 0.0669, 0.0843, 0.1063, 0.1339, 
    0.1687, 0.2125, 0.2677, 0.3373, 0.4250, 
    0.5355, 0.6746, 0.8500, 1.0709, 1.3493, 
    1.7000, 2.1419, 2.6986, 3.4000, 4.2837, 
    5.3972, 6.8000, 8.5675, 10.7943, 13.6000, 
    17.1349, 21.5887, 27.2000, 34.2699, 43.1773, 
    54.4000, 68.5397, 86.3546, 108.8000, 137.0794, 
    172.7092, 217.6000, 274.1588, 345.4185, 435.2000, 
    548.3176, 690.8369, 870.4000, 1096.6353, 1381.6739, 
    1740.8000, 2193.2706, 2763.3478, 3481.6000, 4386.5411, 
    5526.6955, 6963.2000, 8773.0823, 11053.3910, 13926.4000, 
    17546.1645, 22106.7820, 27852.8000, 35092.3290, 44213.5640, 
    55705.6000, 70184.6580, 88427.1280, 111411.2000, 140369.3161, 
    176854.2561, 222822.4000, 280738.6321, 353708.5122, 445644.8000
};

static int initialized /* = 0 */;

// initialize ROM variables
void initROM()
{
    if (ATOMIC_CAS32(&initialized, 0, 1) == 1)
        return;

    int i, c;

    // g_aucConvertToBit[ x ]: log2(x/4), if x=4 -> 0, x=8 -> 1, x=16 -> 2, ...
    ::memset(g_convertToBit, -1, sizeof(g_convertToBit));
    c = 0;
    for (i = 4; i <= MAX_CU_SIZE; i *= 2)
    {
        g_convertToBit[i] = c;
        c++;
    }

    // initialise scan orders
    for (uint32_t log2BlockSize = 0; log2BlockSize < MAX_CU_DEPTH; log2BlockSize++)
    {
        const uint32_t blockWidth  = 1 << log2BlockSize;
        const uint32_t blockHeight = 1 << log2BlockSize;
        const uint32_t totalValues = blockWidth * blockHeight;
        //non-grouped scan orders
        for (uint32_t scanTypeIndex = 0; scanTypeIndex < SCAN_NUMBER_OF_TYPES; scanTypeIndex++)
        {
            const COEFF_SCAN_TYPE scanType = COEFF_SCAN_TYPE(scanTypeIndex);
            g_scanOrder[SCAN_UNGROUPED][scanType][log2BlockSize] = X265_MALLOC(uint16_t, totalValues);
            ScanGenerator fullBlockScan(blockWidth, blockHeight, blockWidth, scanType);

            for (uint32_t scanPosition = 0; scanPosition < totalValues; scanPosition++)
            {
                g_scanOrder[SCAN_UNGROUPED][scanType][log2BlockSize][scanPosition] = fullBlockScan.GetNextIndex(0, 0);
            }
        }

        //grouped scan orders
        const uint32_t  groupWidth           = 1 << MLS_CG_LOG2_SIZE;
        const uint32_t  groupHeight          = 1 << MLS_CG_LOG2_SIZE;
        const uint32_t  widthInGroups        = blockWidth  >> MLS_CG_LOG2_SIZE;
        const uint32_t  heightInGroups       = blockHeight >> MLS_CG_LOG2_SIZE;

        const uint32_t  groupSize            = groupWidth    * groupHeight;
        const uint32_t  totalGroups          = widthInGroups * heightInGroups;

        for (uint32_t scanTypeIndex = 0; scanTypeIndex < SCAN_NUMBER_OF_TYPES; scanTypeIndex++)
        {
            const COEFF_SCAN_TYPE scanType = COEFF_SCAN_TYPE(scanTypeIndex);

            g_scanOrder[SCAN_GROUPED_4x4][scanType][log2BlockSize] = X265_MALLOC(uint16_t, totalValues);

            ScanGenerator fullBlockScan(widthInGroups, heightInGroups, groupWidth, scanType);

            for (uint32_t groupIndex = 0; groupIndex < totalGroups; groupIndex++)
            {
                const uint32_t groupPositionY  = fullBlockScan.GetCurrentY();
                const uint32_t groupPositionX  = fullBlockScan.GetCurrentX();
                const uint32_t groupOffsetX    = groupPositionX * groupWidth;
                const uint32_t groupOffsetY    = groupPositionY * groupHeight;
                const uint32_t groupOffsetScan = groupIndex     * groupSize;

                ScanGenerator groupScan(groupWidth, groupHeight, blockWidth, scanType);

                for (uint32_t scanPosition = 0; scanPosition < groupSize; scanPosition++)
                {
                    g_scanOrder[SCAN_GROUPED_4x4][scanType][log2BlockSize][groupOffsetScan + scanPosition] = groupScan.GetNextIndex(groupOffsetX, groupOffsetY);
                }

                fullBlockScan.GetNextIndex(0, 0);
            }
        }

        //--------------------------------------------------------------------------------------------------
    }
}

void destroyROM()
{
    if (ATOMIC_CAS32(&initialized, 1, 0) == 0)
        return;

    for (uint32_t groupTypeIndex = 0; groupTypeIndex < SCAN_NUMBER_OF_GROUP_TYPES; groupTypeIndex++)
    {
        for (uint32_t scanOrderIndex = 0; scanOrderIndex < SCAN_NUMBER_OF_TYPES; scanOrderIndex++)
        {
            for (uint32_t log2BlockSize = 0; log2BlockSize < MAX_CU_DEPTH; log2BlockSize++)
            {
                X265_FREE(g_scanOrder[groupTypeIndex][scanOrderIndex][log2BlockSize]);
            }
        }
    }
}

// ====================================================================================================================
// Data structure related table & variable
// ====================================================================================================================

uint32_t g_maxCUSize   = MAX_CU_SIZE;
uint32_t g_maxCUDepth  = MAX_CU_DEPTH;
uint32_t g_addCUDepth  = 0;
uint32_t g_zscanToRaster[MAX_NUM_SPU_W * MAX_NUM_SPU_W] = { 0, };
uint32_t g_rasterToZscan[MAX_NUM_SPU_W * MAX_NUM_SPU_W] = { 0, };
uint32_t g_rasterToPelX[MAX_NUM_SPU_W * MAX_NUM_SPU_W] = { 0, };
uint32_t g_rasterToPelY[MAX_NUM_SPU_W * MAX_NUM_SPU_W] = { 0, };

const uint32_t g_puOffset[8] = { 0, 8, 4, 4, 2, 10, 1, 5 };

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

void initRasterToZscan(uint32_t maxCUSize, uint32_t maxDepth)
{
    uint32_t  unitSize = maxCUSize  >> (maxDepth - 1);

    uint32_t  numPartInCUSize  = (uint32_t)maxCUSize / unitSize;

    for (uint32_t i = 0; i < numPartInCUSize * numPartInCUSize; i++)
    {
        g_rasterToZscan[g_zscanToRaster[i]] = i;
    }
}

void initRasterToPelXY(uint32_t maxCUSize, uint32_t maxDepth)
{
    uint32_t i;

    uint32_t* tempX = &g_rasterToPelX[0];
    uint32_t* tempY = &g_rasterToPelY[0];

    uint32_t  unitSize  = maxCUSize >> (maxDepth - 1);

    uint32_t  numPartInCUSize = maxCUSize / unitSize;

    tempX[0] = 0;
    tempX++;
    for (i = 1; i < numPartInCUSize; i++)
    {
        tempX[0] = tempX[-1] + unitSize;
        tempX++;
    }

    for (i = 1; i < numPartInCUSize; i++)
    {
        memcpy(tempX, tempX - numPartInCUSize, sizeof(uint32_t) * numPartInCUSize);
        tempX += numPartInCUSize;
    }

    for (i = 1; i < numPartInCUSize * numPartInCUSize; i++)
    {
        tempY[i] = (i / numPartInCUSize) * unitSize;
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

const int g_quantScales[6] =
{
    26214, 23302, 20560, 18396, 16384, 14564
};

const int g_invQuantScales[6] =
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
const uint8_t g_chromaScale[NUM_CHROMA_FORMAT][chromaQPMappingTableSize] =
{
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0,0,0,0,0,0,0,0,0,0,0,0},
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 29, 30, 31, 32, 33, 33, 34, 34, 35, 35, 36, 36, 37, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51},
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51 },
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51 }
};

const uint8_t g_chroma422IntraAngleMappingTable[36] =
    { 0, 1, 2, 2, 2, 2, 3, 5, 7, 8, 10, 12, 13, 15, 17, 18, 19, 20, 21, 22, 23, 23, 24, 24, 25, 25, 26, 27, 27, 28, 28, 29, 29, 30, 31, DM_CHROMA_IDX};

// ====================================================================================================================
// Misc.
// ====================================================================================================================

uint8_t g_convertToBit[MAX_CU_SIZE + 1];

#if ENC_DEC_TRACE
FILE*  g_hTrace = NULL;
const bool g_bEncDecTraceEnable  = true;
const bool g_bEncDecTraceDisable = false;
bool   g_HLSTraceEnable = true;
bool   g_bJustDoIt = false;
uint64_t g_nSymbolCounter = 0;
#endif

// ====================================================================================================================
// Scanning order & context model mapping
// ====================================================================================================================

const uint8_t g_minInGroup[10] = { 0, 1, 2, 3, 4, 6, 8, 12, 16, 24 };

// Rice parameters for absolute transform levels
const uint8_t g_goRiceRange[5] = { 7, 14, 26, 46, 78 };

//const uint8_t g_goRicePrefixLen[5] = { 8, 7, 6, 5, 4 };

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
const uint32_t g_scalingListSize[4] = { 16, 64, 256, 1024 };
const uint32_t g_scalingListSizeX[4] = { 4, 8, 16,  32 };
const uint32_t g_scalingListNum[SCALING_LIST_SIZE_NUM] = { 6, 6, 6, 6 };

const int g_winUnitX[] = { 1, 2, 2, 1 };
const int g_winUnitY[] = { 1, 2, 1, 1 };

const uint8_t g_lpsTable[64][4] =
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

const uint8_t x265_exp2_lut[64] =
{
    0,  3,  6,  8,  11, 14,  17,  20,  23,  26,  29,  32,  36,  39,  42,  45,
    48,  52,  55,  58,  62,  65,  69,  72,  76,  80,  83,  87,  91,  94,  98,  102,
    106,  110,  114,  118,  122,  126,  130,  135,  139,  143,  147,  152,  156,  161,  165,  170,
    175,  179,  184,  189,  194,  198,  203,  208,  214,  219,  224,  229,  234,  240,  245,  250
};
}
//! \}
