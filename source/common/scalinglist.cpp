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
        return g_quantTSDefault4x4;
    case SIZE_8x8:
        return (listId < 3) ? g_quantIntraDefault8x8 : g_quantInterDefault8x8;
    case SIZE_16x16:
        return (listId < 3) ? g_quantIntraDefault8x8 : g_quantInterDefault8x8;
    case SIZE_32x32:
        return (listId < 1) ? g_quantIntraDefault8x8 : g_quantInterDefault8x8;
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
}

bool ScalingList::parseScalingList(char* filename)
{
    FILE *fp;
    char line[1024];
    uint32_t sizeIdc, listIdc;
    uint32_t i, size = 0;
    int32_t *src = 0, data;
    char *ret;
    uint32_t  retval;

    if ((fp = fopen(filename, "r")) == (FILE*)NULL)
    {
        printf("can't open file %s :: set Default Matrix\n", filename);
        return true;
    }

    for (sizeIdc = 0; sizeIdc < ScalingList::NUM_SIZES; sizeIdc++)
    {
        size = X265_MIN(MAX_MATRIX_COEF_NUM, (int)g_scalingListSize[sizeIdc]);
        for (listIdc = 0; listIdc < g_scalingListNum[sizeIdc]; listIdc++)
        {
            src = m_scalingListCoef[sizeIdc][listIdc];

            fseek(fp, 0, 0);
            do
            {
                ret = fgets(line, 1024, fp);
                if ((ret == NULL) || (strstr(line, MatrixType[sizeIdc][listIdc]) == NULL && feof(fp)))
                {
                    printf("Error: can't read Matrix :: set Default Matrix\n");
                    return true;
                }
            }
            while (strstr(line, MatrixType[sizeIdc][listIdc]) == NULL);
            for (i = 0; i < size; i++)
            {
                retval = fscanf(fp, "%d,", &data);
                if (retval != 1)
                {
                    printf("Error: can't read Matrix :: set Default Matrix\n");
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
                    ret = fgets(line, 1024, fp);
                    if ((ret == NULL) || (strstr(line, MatrixType_DC[sizeIdc][listIdc]) == NULL && feof(fp)))
                    {
                        printf("Error: can't read DC :: set Default Matrix\n");
                        return true;
                    }
                }
                while (strstr(line, MatrixType_DC[sizeIdc][listIdc]) == NULL);
                retval = fscanf(fp, "%d,", &data);
                if (retval != 1)
                {
                    printf("Error: can't read Matrix :: set Default Matrix\n");
                    return true;
                }
                // overwrite DC value when size of matrix is larger than 16x16
                m_scalingListDC[sizeIdc][listIdc] = data;
            }
        }
    }

    fclose(fp);
    return false;
}

const uint32_t g_scalingListSize[4] = { 16, 64, 256, 1024 };
const uint32_t g_scalingListSizeX[4] = { 4, 8, 16, 32 };
const uint32_t g_scalingListNum[ScalingList::NUM_SIZES] = { 6, 6, 6, 6 };
}