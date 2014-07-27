/*****************************************************************************
 * Copyright (C) 2014 x265 project
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
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#include "common.h"
#include "TLibCommon/TComRom.h"
#include "scalinglist.h"

namespace {
// file-anonymous namespace

/* Strings for scaling list file parsing */
const char MatrixType[4][6][20] =
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
const char MatrixType_DC[4][12][22] =
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

int quantTSDefault4x4[16] =
{
    16, 16, 16, 16,
    16, 16, 16, 16,
    16, 16, 16, 16,
    16, 16, 16, 16
};

int quantIntraDefault8x8[64] =
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

int quantInterDefault8x8[64] =
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

}

namespace x265 {
// private namespace

ScalingList::ScalingList()
{
    for (uint32_t sizeId = 0; sizeId < ScalingList::NUM_SIZES; sizeId++)
        for (uint32_t listId = 0; listId < g_scalingListNum[sizeId]; listId++)
            m_scalingListCoef[sizeId][listId] = new int[X265_MIN(MAX_MATRIX_COEF_NUM, (int)g_scalingListSize[sizeId])];
}

ScalingList::~ScalingList()
{
    for (uint32_t sizeId = 0; sizeId < ScalingList::NUM_SIZES; sizeId++)
        for (uint32_t listId = 0; listId < g_scalingListNum[sizeId]; listId++)
            delete [] m_scalingListCoef[sizeId][listId];
}

  
bool ScalingList::checkPredMode(uint32_t sizeId, int listId)
{
    for (int predListIdx = listId; predListIdx >= 0; predListIdx--)
    {
        if (!memcmp(m_scalingListCoef[sizeId][listId],
                    ((listId == predListIdx) ? getScalingListDefaultAddress(sizeId, predListIdx) : m_scalingListCoef[sizeId][predListIdx]),
                    sizeof(int) * X265_MIN(MAX_MATRIX_COEF_NUM, (int)g_scalingListSize[sizeId])) // check value of matrix
            && ((sizeId < SIZE_16x16) || (m_scalingListDC[sizeId][listId] == m_scalingListDC[sizeId][predListIdx]))) // check DC value
        {
            m_refMatrixId[sizeId][listId] = predListIdx;
            return false;
        }
    }

    return true;
}

/* check if use default quantization matrix
 * returns true if default quantization matrix is used in all sizes */
bool ScalingList::checkDefaultScalingList()
{
    uint32_t defaultCounter = 0;

    for (uint32_t s = 0; s < ScalingList::NUM_SIZES; s++)
        for (uint32_t l = 0; l < g_scalingListNum[s]; l++)
            if (!memcmp(m_scalingListCoef[s][l], getScalingListDefaultAddress(s, l),
                        sizeof(int) * X265_MIN(MAX_MATRIX_COEF_NUM, (int)g_scalingListSize[s])) &&
                ((s < SIZE_16x16) || (m_scalingListDC[s][l] == 16)))
                defaultCounter++;

    return (defaultCounter == (NUM_LISTS * ScalingList::NUM_SIZES - 4)) ? false : true; // -4 for 32x32
}

/* get address of default quantization matrix */
int32_t* ScalingList::getScalingListDefaultAddress(uint32_t sizeId, uint32_t listId)
{
    switch (sizeId)
    {
    case SIZE_4x4:
        return quantTSDefault4x4;
    case SIZE_8x8:
        return (listId < 3) ? quantIntraDefault8x8 : quantInterDefault8x8;
    case SIZE_16x16:
        return (listId < 3) ? quantIntraDefault8x8 : quantInterDefault8x8;
    case SIZE_32x32:
        return (listId < 1) ? quantIntraDefault8x8 : quantInterDefault8x8;
    default:
        break;
    }

    X265_CHECK(0, "invalid scaling list size\n");
    return NULL;
}

void ScalingList::processDefaultMarix(uint32_t sizeId, uint32_t listId)
{
    ::memcpy(m_scalingListCoef[sizeId][listId], getScalingListDefaultAddress(sizeId, listId), sizeof(int) * X265_MIN(MAX_MATRIX_COEF_NUM, (int)g_scalingListSize[sizeId]));
    m_scalingListDC[sizeId][listId] = SCALING_LIST_DC;
}

void ScalingList::setDefaultScalingList()
{
    for (uint32_t sizeId = 0; sizeId < ScalingList::NUM_SIZES; sizeId++)
        for (uint32_t listId = 0; listId < g_scalingListNum[sizeId]; listId++)
            processDefaultMarix(sizeId, listId);
    m_bEnabled = true;
    m_bDataPresent = false;
}

bool ScalingList::parseScalingList(const char* filename)
{
    FILE *fp = fopen(filename, "r");
    if (!fp)
    {
        x265_log(NULL, X265_LOG_ERROR, "can't open scaling list file %s\n", filename);
        return true;
    }

    char line[1024];
    int32_t *src = NULL;

    for (uint32_t sizeIdc = 0; sizeIdc < NUM_SIZES; sizeIdc++)
    {
        int size = X265_MIN(MAX_MATRIX_COEF_NUM, (int)g_scalingListSize[sizeIdc]);
        for (uint32_t listIdc = 0; listIdc < g_scalingListNum[sizeIdc]; listIdc++)
        {
            src = m_scalingListCoef[sizeIdc][listIdc];

            fseek(fp, 0, 0);
            do
            {
                char *ret = fgets(line, 1024, fp);
                if (!ret || (strstr(line, MatrixType[sizeIdc][listIdc]) == NULL && feof(fp)))
                {
                    x265_log(NULL, X265_LOG_ERROR, "can't read matrix from %s\n", filename);
                    return true;
                }
            }
            while (strstr(line, MatrixType[sizeIdc][listIdc]) == NULL);
            for (int i = 0; i < size; i++)
            {
                int data;
                if (fscanf(fp, "%d,", &data) != 1)
                {
                    x265_log(NULL, X265_LOG_ERROR, "can't read matrix from %s\n", filename);
                    return true;
                }
                src[i] = data;
            }

            // set DC value for default matrix check
            m_scalingListDC[sizeIdc][listIdc] = src[0];

            if (sizeIdc > SIZE_8x8)
            {
                fseek(fp, 0, 0);
                do
                {
                    char *ret = fgets(line, 1024, fp);
                    if (!ret || (!strstr(line, MatrixType_DC[sizeIdc][listIdc]) && feof(fp)))
                    {
                        x265_log(NULL, X265_LOG_ERROR, "can't read DC from %s\n", filename);
                        return true;
                    }
                }
                while (!strstr(line, MatrixType_DC[sizeIdc][listIdc]))
                    ;

                int data;
                if (fscanf(fp, "%d,", &data) != 1)
                {
                    x265_log(NULL, X265_LOG_ERROR, "can't read matrix from %s\n", filename);
                    return true;
                }

                // overwrite DC value when size of matrix is larger than 16x16
                m_scalingListDC[sizeIdc][listIdc] = data;
            }
        }
    }

    fclose(fp);

    m_bEnabled = true;
    m_bDataPresent = !checkDefaultScalingList();

    return false;
}

const uint32_t g_scalingListSize[ScalingList::NUM_SIZES] = { 16, 64, 256, 1024 };
const uint32_t g_scalingListSizeX[ScalingList::NUM_SIZES] = { 4, 8, 16, 32 };
const uint32_t g_scalingListNum[ScalingList::NUM_SIZES] = { 6, 6, 6, 6 };
const int g_quantScales[ScalingList::NUM_REM] = { 26214, 23302, 20560, 18396, 16384, 14564 };
const int g_invQuantScales[ScalingList::NUM_REM] = { 40, 45, 51, 57, 64, 72 };

}
