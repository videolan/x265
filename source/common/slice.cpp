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
#include "frame.h"
#include "piclist.h"
#include "slice.h"

namespace x265 {
// private namespace


void Slice::setRefPicList(PicList& picList)
{
    if (m_sliceType == I_SLICE)
    {
        ::memset(m_refPicList, 0, sizeof(m_refPicList));
        m_numRefIdx[1] = m_numRefIdx[0] = 0;
        return;
    }

    Frame* refPic = NULL;
    Frame* refPicSetStCurr0[MAX_NUM_REF];
    Frame* refPicSetStCurr1[MAX_NUM_REF];
    Frame* refPicSetLtCurr[MAX_NUM_REF];
    int numPocStCurr0 = 0;
    int numPocStCurr1 = 0;
    int numPocLtCurr = 0;
    int i;

    for (i = 0; i < m_rps.numberOfNegativePictures; i++)
    {
        if (m_rps.bUsed[i])
        {
            refPic = picList.getPOC(m_poc + m_rps.deltaPOC[i]);
            refPicSetStCurr0[numPocStCurr0] = refPic;
            numPocStCurr0++;
        }
    }

    for (; i < m_rps.numberOfNegativePictures + m_rps.numberOfPositivePictures; i++)
    {
        if (m_rps.bUsed[i])
        {
            refPic = picList.getPOC(m_poc + m_rps.deltaPOC[i]);
            refPicSetStCurr1[numPocStCurr1] = refPic;
            numPocStCurr1++;
        }
    }

    X265_CHECK(m_rps.numberOfPictures == m_rps.numberOfNegativePictures + m_rps.numberOfPositivePictures,
               "unexpected picture in RPS\n");

    // ref_pic_list_init
    Frame* rpsCurrList0[MAX_NUM_REF + 1];
    Frame* rpsCurrList1[MAX_NUM_REF + 1];
    int numPocTotalCurr = numPocStCurr0 + numPocStCurr1 + numPocLtCurr;

    int cIdx = 0;
    for (i = 0; i < numPocStCurr0; i++, cIdx++)
        rpsCurrList0[cIdx] = refPicSetStCurr0[i];

    for (i = 0; i < numPocStCurr1; i++, cIdx++)
        rpsCurrList0[cIdx] = refPicSetStCurr1[i];

    for (i = 0; i < numPocLtCurr; i++, cIdx++)
        rpsCurrList0[cIdx] = refPicSetLtCurr[i];

    X265_CHECK(cIdx == numPocTotalCurr, "RPS index check fail\n");

    if (m_sliceType == B_SLICE)
    {
        cIdx = 0;
        for (i = 0; i < numPocStCurr1; i++, cIdx++)
            rpsCurrList1[cIdx] = refPicSetStCurr1[i];

        for (i = 0; i < numPocStCurr0; i++, cIdx++)
            rpsCurrList1[cIdx] = refPicSetStCurr0[i];

        for (i = 0; i < numPocLtCurr; i++, cIdx++)
            rpsCurrList1[cIdx] = refPicSetLtCurr[i];

        X265_CHECK(cIdx == numPocTotalCurr, "RPS index check fail\n");
    }

    for (int rIdx = 0; rIdx < m_numRefIdx[0]; rIdx++)
    {
        cIdx = rIdx % numPocTotalCurr;
        X265_CHECK(cIdx >= 0 && cIdx < numPocTotalCurr, "RPS index check fail\n");
        m_refPicList[0][rIdx] = rpsCurrList0[cIdx];
    }

    if (m_sliceType != B_SLICE)
    {
        m_numRefIdx[1] = 0;
        ::memset(m_refPicList[1], 0, sizeof(m_refPicList[1]));
    }
    else
    {
        for (int rIdx = 0; rIdx < m_numRefIdx[1]; rIdx++)
        {
            cIdx = rIdx % numPocTotalCurr;
            X265_CHECK(cIdx >= 0 && cIdx < numPocTotalCurr, "RPS index check fail\n");
            m_refPicList[1][rIdx] = rpsCurrList1[cIdx];
        }
    }

    for (int dir = 0; dir < 2; dir++)
        for (int numRefIdx = 0; numRefIdx < m_numRefIdx[dir]; numRefIdx++)
            m_refPOCList[dir][numRefIdx] = m_refPicList[dir][numRefIdx]->getPOC();
}

void Slice::disableWeights()
{
    for (int l = 0; l < 2; l++)
        for (int i = 0; i < MAX_NUM_REF; i++)
            for (int yuv = 0; yuv < 3; yuv++)
            {
                WeightParam& wp = m_weightPredTable[l][i][yuv];
                wp.bPresentFlag = false;
                wp.log2WeightDenom = 0;
                wp.inputWeight = 1;
                wp.inputOffset = 0;
            }
}

/* Sorts the deltaPOC and Used by current values in the RPS based on the
 * deltaPOC values.  deltaPOC values are sorted with -ve values before the +ve
 * values.  -ve values are in decreasing order.  +ve values are in increasing
 * order */
void RPS::sortDeltaPOC()
{
    // sort in increasing order (smallest first)
    for (int j = 1; j < numberOfPictures; j++)
    {
        int dPOC = deltaPOC[j];
        bool used = bUsed[j];
        for (int k = j - 1; k >= 0; k--)
        {
            int temp = deltaPOC[k];
            if (dPOC < temp)
            {
                deltaPOC[k + 1] = temp;
                bUsed[k + 1] = bUsed[k];
                deltaPOC[k] = dPOC;
                bUsed[k] = used;
            }
        }
    }

    // flip the negative values to largest first
    int numNegPics = numberOfNegativePictures;
    for (int j = 0, k = numNegPics - 1; j < numNegPics >> 1; j++, k--)
    {
        int dPOC = deltaPOC[j];
        bool used = bUsed[j];
        deltaPOC[j] = deltaPOC[k];
        bUsed[j] = bUsed[k];
        deltaPOC[k] = dPOC;
        bUsed[k] = used;
    }
}

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