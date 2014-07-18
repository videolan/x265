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

/** \file     TComSlice.cpp
    \brief    slice header and SPS class
*/

#include "common.h"
#include "frame.h"
#include "piclist.h"
#include "TComSlice.h"

using namespace x265;

Frame* TComSlice::xGetRefPic(PicList& picList, int poc)
{
    Frame *iterPic = picList.first();
    Frame* pic = NULL;

    while (iterPic)
    {
        pic = iterPic;
        if (pic->getPOC() == poc)
        {
            break;
        }
        iterPic = iterPic->m_next;
    }

    return pic;
}

void TComSlice::setRefPicList(PicList& picList)
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
    uint32_t numPocStCurr0 = 0;
    uint32_t numPocStCurr1 = 0;
    uint32_t numPocLtCurr = 0;
    int i;

    for (i = 0; i < m_rps.m_numberOfNegativePictures; i++)
    {
        if (m_rps.m_used[i])
        {
            refPic = xGetRefPic(picList, m_poc + m_rps.m_deltaPOC[i]);
            refPicSetStCurr0[numPocStCurr0] = refPic;
            numPocStCurr0++;
        }
    }

    for (; i < m_rps.m_numberOfNegativePictures + m_rps.m_numberOfPositivePictures; i++)
    {
        if (m_rps.m_used[i])
        {
            refPic = xGetRefPic(picList, m_poc + m_rps.m_deltaPOC[i]);
            refPicSetStCurr1[numPocStCurr1] = refPic;
            numPocStCurr1++;
        }
    }

    X265_CHECK(m_rps.m_numberOfPictures == m_rps.m_numberOfNegativePictures + m_rps.m_numberOfPositivePictures,
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

/** get WP tables for weighted pred
 * \param int
 * \param refIdx
 * \param *&wpScalingParam
 * \returns void
 */
void TComSlice::getWpScaling(int l, int refIdx, WeightParam *&wp)
{
    wp = m_weightPredTable[l][refIdx];
}

/** reset Default WP tables settings : no weight.
 * \param wpScalingParam
 * \returns void
 */
void TComSlice::resetWpScaling()
{
    for (int e = 0; e < 2; e++)
    {
        for (int i = 0; i < MAX_NUM_REF; i++)
        {
            for (int yuv = 0; yuv < 3; yuv++)
            {
                WeightParam  *pwp = &(m_weightPredTable[e][i][yuv]);
                pwp->bPresentFlag    = false;
                pwp->log2WeightDenom = 0;
                pwp->inputWeight     = 1;
                pwp->inputOffset     = 0;
            }
        }
    }
}

/** init WP table
 * \returns void
 */
void TComSlice::initWpScaling()
{
    for (int e = 0; e < 2; e++)
    {
        for (int i = 0; i < MAX_NUM_REF; i++)
        {
            for (int yuv = 0; yuv < 3; yuv++)
            {
                WeightParam  *pwp = &(m_weightPredTable[e][i][yuv]);
                if (!pwp->bPresentFlag)
                {
                    // Inferring values not present :
                    pwp->inputWeight = (1 << pwp->log2WeightDenom);
                    pwp->inputOffset = 0;
                }

                pwp->w      = pwp->inputWeight;
                pwp->o      = pwp->inputOffset << (X265_DEPTH - 8);
                pwp->shift  = pwp->log2WeightDenom;
                pwp->round  = (pwp->log2WeightDenom >= 1) ? (1 << (pwp->log2WeightDenom - 1)) : (0);
            }
        }
    }
}

/* Sorts the deltaPOC and Used by current values in the RPS based on the
 * deltaPOC values.  deltaPOC values are sorted with -ve values before the +ve
 * values.  -ve values are in decreasing order.  +ve values are in increasing
 * order */
void RPS::sortDeltaPOC()
{
    // sort in increasing order (smallest first)
    for (int j = 1; j < m_numberOfPictures; j++)
    {
        int deltaPOC = m_deltaPOC[j];
        bool used = m_used[j];
        for (int k = j - 1; k >= 0; k--)
        {
            int temp = m_deltaPOC[k];
            if (deltaPOC < temp)
            {
                m_deltaPOC[k + 1] = temp;
                m_used[k + 1] = m_used[k];
                m_deltaPOC[k] = deltaPOC;
                m_used[k] = used;
            }
        }
    }

    // flip the negative values to largest first
    int numNegPics = m_numberOfNegativePictures;
    for (int j = 0, k = numNegPics - 1; j < numNegPics >> 1; j++, k--)
    {
        int deltaPOC = m_deltaPOC[j];
        bool used = m_used[j];
        m_deltaPOC[j] = m_deltaPOC[k];
        m_used[j] = m_used[k];
        m_deltaPOC[k] = deltaPOC;
        m_used[k] = used;
    }
}

TComScalingList::TComScalingList()
{
    for (uint32_t sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
        for (uint32_t listId = 0; listId < g_scalingListNum[sizeId]; listId++)
            m_scalingListCoef[sizeId][listId] = new int[X265_MIN(MAX_MATRIX_COEF_NUM, (int)g_scalingListSize[sizeId])];
}

TComScalingList::~TComScalingList()
{
    for (uint32_t sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
        for (uint32_t listId = 0; listId < g_scalingListNum[sizeId]; listId++)
            delete [] m_scalingListCoef[sizeId][listId];
}

  
bool TComScalingList::checkPredMode(uint32_t sizeId, int listId)
{
    for (int predListIdx = listId; predListIdx >= 0; predListIdx--)
    {
        if (!memcmp(m_scalingListCoef[sizeId][listId],
                    ((listId == predListIdx) ? getScalingListDefaultAddress(sizeId, predListIdx) : m_scalingListCoef[sizeId][predListIdx]),
                    sizeof(int) * X265_MIN(MAX_MATRIX_COEF_NUM, (int)g_scalingListSize[sizeId])) // check value of matrix
            && ((sizeId < SCALING_LIST_16x16) || (m_scalingListDC[sizeId][listId] == m_scalingListDC[sizeId][predListIdx]))) // check DC value
        {
            m_refMatrixId[sizeId][listId] = predListIdx;
            return false;
        }
    }

    return true;
}

/* check if use default quantization matrix
 * returns true if default quantization matrix is used in all sizes */
bool TComScalingList::checkDefaultScalingList()
{
    uint32_t defaultCounter = 0;

    for (uint32_t s = 0; s < SCALING_LIST_SIZE_NUM; s++)
        for (uint32_t l = 0; l < g_scalingListNum[s]; l++)
            if (!memcmp(m_scalingListCoef[s][l], getScalingListDefaultAddress(s, l),
                        sizeof(int) * X265_MIN(MAX_MATRIX_COEF_NUM, (int)g_scalingListSize[s])) &&
                ((s < SCALING_LIST_16x16) || (m_scalingListDC[s][l] == 16)))
                defaultCounter++;

    return (defaultCounter == (SCALING_LIST_NUM * SCALING_LIST_SIZE_NUM - 4)) ? false : true; // -4 for 32x32
}

/* get scaling matrix from reference list id */
void TComScalingList::processRefMatrix(uint32_t sizeId, uint32_t listId, uint32_t refListId)
{
    int32_t *src = listId == refListId ? getScalingListDefaultAddress(sizeId, refListId) : m_scalingListCoef[sizeId][refListId];
    ::memcpy(m_scalingListCoef[sizeId][listId], src, sizeof(int) * X265_MIN(MAX_MATRIX_COEF_NUM, (int)g_scalingListSize[sizeId]));
}

/* get address of default quantization matrix */
int32_t* TComScalingList::getScalingListDefaultAddress(uint32_t sizeId, uint32_t listId)
{
    switch (sizeId)
    {
    case SCALING_LIST_4x4:
        return g_quantTSDefault4x4;
    case SCALING_LIST_8x8:
        return (listId < 3) ? g_quantIntraDefault8x8 : g_quantInterDefault8x8;
    case SCALING_LIST_16x16:
        return (listId < 3) ? g_quantIntraDefault8x8 : g_quantInterDefault8x8;
    case SCALING_LIST_32x32:
        return (listId < 1) ? g_quantIntraDefault8x8 : g_quantInterDefault8x8;
    default:
        break;
    }

    X265_CHECK(0, "invalid scaling list size\n");
    return NULL;
}

void TComScalingList::processDefaultMarix(uint32_t sizeId, uint32_t listId)
{
    ::memcpy(m_scalingListCoef[sizeId][listId], getScalingListDefaultAddress(sizeId, listId), sizeof(int) * X265_MIN(MAX_MATRIX_COEF_NUM, (int)g_scalingListSize[sizeId]));
    m_scalingListDC[sizeId][listId] = SCALING_LIST_DC;
}

/* check DC value of matrix for default matrix signaling */
void TComScalingList::checkDcOfMatrix()
{
    for (uint32_t sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
        for (uint32_t listId = 0; listId < g_scalingListNum[sizeId]; listId++)
            if (!m_scalingListDC[sizeId][listId])
                processDefaultMarix(sizeId, listId);
}

void TComScalingList::setDefaultScalingList()
{
    for (uint32_t sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
        for (uint32_t listId = 0; listId < g_scalingListNum[sizeId]; listId++)
            processDefaultMarix(sizeId, listId);
}

bool TComScalingList::parseScalingList(char* filename)
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

    for (sizeIdc = 0; sizeIdc < SCALING_LIST_SIZE_NUM; sizeIdc++)
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

            if (sizeIdc > SCALING_LIST_8x8)
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

//! \}
