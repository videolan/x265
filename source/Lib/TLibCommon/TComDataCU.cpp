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

/** \file     TComDataCU.cpp
    \brief    CU data structure
    \todo     not all entities are documented
*/

#include "TComDataCU.h"
#include "TComPic.h"
#include "mv.h"

using namespace x265;

static MV scaleMv(MV mv, int scale)
{
    int mvx = Clip3(-32768, 32767, (scale * mv.x + 127 + (scale * mv.x < 0)) >> 8);
    int mvy = Clip3(-32768, 32767, (scale * mv.y + 127 + (scale * mv.y < 0)) >> 8);

    return MV((int16_t)mvx, (int16_t)mvy);
}

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TComDataCU::TComDataCU()
{
    /* TODO: can we use memset here? */
    m_pic = NULL;
    m_slice = NULL;
    m_depth = NULL;
    m_skipFlag = NULL;
    m_partSizes = NULL;
    m_predModes = NULL;
    m_cmv_predModes = NULL;
    m_cuTransquantBypass = NULL;
    m_width = NULL;
    m_height = NULL;
    m_qp = NULL;
    m_bMergeFlags = NULL;
    m_mergeIndex = NULL;
    m_lumaIntraDir = NULL;
    m_chromaIntraDir = NULL;
    m_interDir = NULL;
    m_trIdx = NULL;
    m_transformSkip[0] = NULL;
    m_transformSkip[1] = NULL;
    m_transformSkip[2] = NULL;
    m_cbf[0] = NULL;
    m_cbf[1] = NULL;
    m_cbf[2] = NULL;
    m_trCoeffY = NULL;
    m_trCoeffCb = NULL;
    m_trCoeffCr = NULL;
    m_iPCMFlags = NULL;
    m_iPCMSampleY = NULL;
    m_iPCMSampleCb = NULL;
    m_iPCMSampleCr = NULL;
    m_pattern = NULL;
    m_cuAboveLeft = NULL;
    m_cuAboveRight = NULL;
    m_cuAbove = NULL;
    m_cuLeft = NULL;
    m_cuColocated[0] = NULL;
    m_cuColocated[1] = NULL;
    m_mvpIdx[0] = NULL;
    m_mvpIdx[1] = NULL;
    m_mvpNum[0] = NULL;
    m_mvpNum[1] = NULL;
}

TComDataCU::~TComDataCU()
{}

void TComDataCU::create(UInt numPartition, UInt width, UInt height, int unitSize)
{
    m_pic           = NULL;
    m_slice         = NULL;
    m_numPartitions = numPartition;
    m_unitSize = unitSize;

    m_qp     = (char*)X265_MALLOC(char,   numPartition);
    m_depth  = (UChar*)X265_MALLOC(UChar, numPartition);
    m_width  = (UChar*)X265_MALLOC(UChar, numPartition);
    m_height = (UChar*)X265_MALLOC(UChar, numPartition);

    m_skipFlag = new bool[numPartition];

    m_partSizes = new char[numPartition];
    memset(m_partSizes, SIZE_NONE, numPartition * sizeof(*m_partSizes));
    m_predModes = new char[numPartition];
    m_cmv_predModes = new char[numPartition];
    m_cuTransquantBypass = new bool[numPartition];

    m_bMergeFlags     = (bool*)X265_MALLOC(bool,  numPartition);
    m_mergeIndex      = (UChar*)X265_MALLOC(UChar, numPartition);
    m_lumaIntraDir    = (UChar*)X265_MALLOC(UChar, numPartition);
    m_chromaIntraDir  = (UChar*)X265_MALLOC(UChar, numPartition);
    m_interDir        = (UChar*)X265_MALLOC(UChar, numPartition);

    m_trIdx            = (UChar*)X265_MALLOC(UChar, numPartition);
    m_transformSkip[0] = (UChar*)X265_MALLOC(UChar, numPartition);
    m_transformSkip[1] = (UChar*)X265_MALLOC(UChar, numPartition);
    m_transformSkip[2] = (UChar*)X265_MALLOC(UChar, numPartition);

    m_cbf[0] = (UChar*)X265_MALLOC(UChar, numPartition);
    m_cbf[1] = (UChar*)X265_MALLOC(UChar, numPartition);
    m_cbf[2] = (UChar*)X265_MALLOC(UChar, numPartition);

    m_mvpIdx[0] = new char[numPartition];
    m_mvpIdx[1] = new char[numPartition];
    m_mvpNum[0] = new char[numPartition];
    m_mvpNum[1] = new char[numPartition];
    memset(m_mvpIdx[0], -1, numPartition * sizeof(char));
    memset(m_mvpIdx[1], -1, numPartition * sizeof(char));

    m_trCoeffY  = (TCoeff*)X265_MALLOC(TCoeff, width * height);
    m_trCoeffCb = (TCoeff*)X265_MALLOC(TCoeff, width * height / 4);
    m_trCoeffCr = (TCoeff*)X265_MALLOC(TCoeff, width * height / 4);
    memset(m_trCoeffY, 0, width * height * sizeof(TCoeff));
    memset(m_trCoeffCb, 0, width * height / 4 * sizeof(TCoeff));
    memset(m_trCoeffCr, 0, width * height / 4 * sizeof(TCoeff));

    m_iPCMFlags   = (bool*)X265_MALLOC(bool, numPartition);
    m_iPCMSampleY  = (Pel*)X265_MALLOC(Pel, width * height);
    m_iPCMSampleCb = (Pel*)X265_MALLOC(Pel, width * height / 4);
    m_iPCMSampleCr = (Pel*)X265_MALLOC(Pel, width * height / 4);

    m_cuMvField[0].create(numPartition);
    m_cuMvField[1].create(numPartition);

    // create pattern memory
    m_pattern = (TComPattern*)X265_MALLOC(TComPattern, 1);
}

void TComDataCU::destroy()
{
    if (m_pattern) { X265_FREE(m_pattern); m_pattern = NULL; }
    if (m_qp) { X265_FREE(m_qp); m_qp = NULL; }
    if (m_depth) { X265_FREE(m_depth); m_depth = NULL; }
    if (m_width) { X265_FREE(m_width); m_width = NULL; }
    if (m_height) { X265_FREE(m_height); m_height = NULL; }
    if (m_skipFlag) { delete[] m_skipFlag; m_skipFlag = NULL; }
    if (m_partSizes) { delete[] m_partSizes; m_partSizes = NULL; }
    if (m_predModes) { delete[] m_predModes; m_predModes = NULL; }
    if (m_cmv_predModes) { delete[] m_cmv_predModes; m_cmv_predModes = NULL; }
    if (m_cuTransquantBypass) { delete[] m_cuTransquantBypass; m_cuTransquantBypass = NULL; }
    if (m_cbf[0]) { X265_FREE(m_cbf[0]); m_cbf[0] = NULL; }
    if (m_cbf[1]) { X265_FREE(m_cbf[1]); m_cbf[1] = NULL; }
    if (m_cbf[2]) { X265_FREE(m_cbf[2]); m_cbf[2] = NULL; }
    if (m_interDir) { X265_FREE(m_interDir); m_interDir = NULL; }
    if (m_bMergeFlags) { X265_FREE(m_bMergeFlags); m_bMergeFlags = NULL; }
    if (m_mergeIndex) { X265_FREE(m_mergeIndex); m_mergeIndex = NULL; }
    if (m_lumaIntraDir) { X265_FREE(m_lumaIntraDir); m_lumaIntraDir = NULL; }
    if (m_chromaIntraDir) { X265_FREE(m_chromaIntraDir); m_chromaIntraDir = NULL; }
    if (m_trIdx) { X265_FREE(m_trIdx); m_trIdx = NULL; }
    if (m_transformSkip[0]) { X265_FREE(m_transformSkip[0]); m_transformSkip[0] = NULL; }
    if (m_transformSkip[1]) { X265_FREE(m_transformSkip[1]); m_transformSkip[1] = NULL; }
    if (m_transformSkip[2]) { X265_FREE(m_transformSkip[2]); m_transformSkip[2] = NULL; }
    if (m_trCoeffY) { X265_FREE(m_trCoeffY); m_trCoeffY = NULL; }
    if (m_trCoeffCb) { X265_FREE(m_trCoeffCb); m_trCoeffCb = NULL; }
    if (m_trCoeffCr) { X265_FREE(m_trCoeffCr); m_trCoeffCr = NULL; }
    if (m_iPCMFlags) { X265_FREE(m_iPCMFlags); m_iPCMFlags = NULL; }
    if (m_iPCMSampleY) { X265_FREE(m_iPCMSampleY); m_iPCMSampleY = NULL; }
    if (m_iPCMSampleCb) { X265_FREE(m_iPCMSampleCb); m_iPCMSampleCb = NULL; }
    if (m_iPCMSampleCr) { X265_FREE(m_iPCMSampleCr); m_iPCMSampleCr = NULL; }
    if (m_mvpIdx[0]) { delete[] m_mvpIdx[0]; m_mvpIdx[0] = NULL; }
    if (m_mvpIdx[1]) { delete[] m_mvpIdx[1]; m_mvpIdx[1] = NULL; }
    if (m_mvpNum[0]) { delete[] m_mvpNum[0]; m_mvpNum[0] = NULL; }
    if (m_mvpNum[1]) { delete[] m_mvpNum[1]; m_mvpNum[1] = NULL; }

    m_cuMvField[0].destroy();
    m_cuMvField[1].destroy();
}

const NDBFBlockInfo& NDBFBlockInfo::operator =(const NDBFBlockInfo& src)
{
    this->sliceID = src.sliceID;
    this->startSU = src.startSU;
    this->endSU  = src.endSU;
    this->widthSU = src.widthSU;
    this->heightSU = src.heightSU;
    this->posX   = src.posX;
    this->posY   = src.posY;
    this->width  = src.width;
    this->height = src.height;
    ::memcpy(this->isBorderAvailable, src.isBorderAvailable, sizeof(bool) * ((int)NUM_SGU_BORDER));
    this->allBordersAvailable = src.allBordersAvailable;

    return *this;
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

// --------------------------------------------------------------------------------------------------------------------
// Initialization
// --------------------------------------------------------------------------------------------------------------------

/**
 - initialize top-level CU
 - internal buffers are already created
 - set values before encoding a CU
 .
 \param  pic     picture (TComPic) class pointer
 \param  iCUAddr   CU address
 */
void TComDataCU::initCU(TComPic* pic, UInt cuAddr)
{
    m_pic              = pic;
    m_slice            = pic->getSlice();
    m_cuAddr           = cuAddr;
    m_cuPelX           = (cuAddr % pic->getFrameWidthInCU()) * g_maxCUWidth;
    m_cuPelY           = (cuAddr / pic->getFrameWidthInCU()) * g_maxCUHeight;
    m_absIdxInLCU      = 0;
    m_totalCost        = MAX_INT64;
    m_totalDistortion  = 0;
    m_totalBits        = 0;
    m_totalBins        = 0;
    m_numPartitions    = pic->getNumPartInCU();

    // CHECK_ME: why partStartIdx always negative
    int partStartIdx = 0 - (cuAddr) * pic->getNumPartInCU();

    int numElements = std::min<int>(partStartIdx, m_numPartitions);
    for (int i = 0; i < numElements; i++)
    {
        TComDataCU* from = pic->getCU(getAddr());
        m_skipFlag[i]   = from->getSkipFlag(i);
        m_partSizes[i] = from->getPartitionSize(i);
        m_predModes[i] = from->getPredictionMode(i);
        m_cuTransquantBypass[i] = from->getCUTransquantBypass(i);
        m_depth[i] = from->getDepth(i);
        m_width[i] = from->getWidth(i);
        m_height[i] = from->getHeight(i);
        m_trIdx[i] = from->getTransformIdx(i);
        m_transformSkip[0][i] = from->getTransformSkip(i, TEXT_LUMA);
        m_transformSkip[1][i] = from->getTransformSkip(i, TEXT_CHROMA_U);
        m_transformSkip[2][i] = from->getTransformSkip(i, TEXT_CHROMA_V);
        m_mvpIdx[0][i] = from->m_mvpIdx[0][i];
        m_mvpIdx[1][i] = from->m_mvpIdx[1][i];
        m_mvpNum[0][i] = from->m_mvpNum[0][i];
        m_mvpNum[1][i] = from->m_mvpNum[1][i];
        m_qp[i] = from->m_qp[i];
        m_bMergeFlags[i] = from->m_bMergeFlags[i];
        m_mergeIndex[i] = from->m_mergeIndex[i];
        m_lumaIntraDir[i] = from->m_lumaIntraDir[i];
        m_chromaIntraDir[i] = from->m_chromaIntraDir[i];
        m_interDir[i] = from->m_interDir[i];
        m_cbf[0][i] = from->m_cbf[0][i];
        m_cbf[1][i] = from->m_cbf[1][i];
        m_cbf[2][i] = from->m_cbf[2][i];
        m_iPCMFlags[i] = from->m_iPCMFlags[i];
    }

    int firstElement = std::max<int>(partStartIdx, 0);
    numElements = m_numPartitions - firstElement;

    if (numElements > 0)
    {
        memset(m_skipFlag         + firstElement, false,                    numElements * sizeof(*m_skipFlag));
        memset(m_partSizes        + firstElement, SIZE_NONE,                numElements * sizeof(*m_partSizes));
        memset(m_predModes        + firstElement, MODE_NONE,                numElements * sizeof(*m_predModes));
        memset(m_cuTransquantBypass + firstElement, false,                  numElements * sizeof(*m_cuTransquantBypass));
        memset(m_depth            + firstElement, 0,                        numElements * sizeof(*m_depth));
        memset(m_trIdx            + firstElement, 0,                        numElements * sizeof(*m_trIdx));
        memset(m_transformSkip[0] + firstElement, 0,                        numElements * sizeof(*m_transformSkip[0]));
        memset(m_transformSkip[1] + firstElement, 0,                        numElements * sizeof(*m_transformSkip[1]));
        memset(m_transformSkip[2] + firstElement, 0,                        numElements * sizeof(*m_transformSkip[2]));
        memset(m_width            + firstElement, g_maxCUWidth,             numElements * sizeof(*m_width));
        memset(m_height           + firstElement, g_maxCUHeight,            numElements * sizeof(*m_height));
        memset(m_mvpIdx[0]        + firstElement, -1,                       numElements * sizeof(*m_mvpIdx[0]));
        memset(m_mvpIdx[1]        + firstElement, -1,                       numElements * sizeof(*m_mvpIdx[1]));
        memset(m_mvpNum[0]        + firstElement, -1,                       numElements * sizeof(*m_mvpNum[0]));
        memset(m_mvpNum[1]        + firstElement, -1,                       numElements * sizeof(*m_mvpNum[1]));
        memset(m_qp               + firstElement, getSlice()->getSliceQp(), numElements * sizeof(*m_qp));
        memset(m_bMergeFlags      + firstElement, false,                    numElements * sizeof(*m_bMergeFlags));
        memset(m_mergeIndex       + firstElement, 0,                        numElements * sizeof(*m_mergeIndex));
        memset(m_lumaIntraDir     + firstElement, DC_IDX,                   numElements * sizeof(*m_lumaIntraDir));
        memset(m_chromaIntraDir   + firstElement, 0,                        numElements * sizeof(*m_chromaIntraDir));
        memset(m_interDir         + firstElement, 0,                        numElements * sizeof(*m_interDir));
        memset(m_cbf[0]           + firstElement, 0,                        numElements * sizeof(*m_cbf[0]));
        memset(m_cbf[1]           + firstElement, 0,                        numElements * sizeof(*m_cbf[1]));
        memset(m_cbf[2]           + firstElement, 0,                        numElements * sizeof(*m_cbf[2]));
        memset(m_iPCMFlags        + firstElement, false,                    numElements * sizeof(*m_iPCMFlags));
    }

    UInt tmp = g_maxCUWidth * g_maxCUHeight;
    if (0 >= partStartIdx)
    {
        m_cuMvField[0].clearMvField();
        m_cuMvField[1].clearMvField();
        memset(m_trCoeffY, 0, sizeof(TCoeff) * tmp);
        memset(m_iPCMSampleY, 0, sizeof(Pel) * tmp);
        tmp  >>= 2;
        memset(m_trCoeffCb, 0, sizeof(TCoeff) * tmp);
        memset(m_trCoeffCr, 0, sizeof(TCoeff) * tmp);
        memset(m_iPCMSampleCb, 0, sizeof(Pel) * tmp);
        memset(m_iPCMSampleCr, 0, sizeof(Pel) * tmp);
    }
    else
    {
        TComDataCU * from = pic->getCU(getAddr());
        m_cuMvField[0].copyFrom(&from->m_cuMvField[0], m_numPartitions, 0);
        m_cuMvField[1].copyFrom(&from->m_cuMvField[1], m_numPartitions, 0);
        for (int i = 0; i < tmp; i++)
        {
            m_trCoeffY[i] = from->m_trCoeffY[i];
            m_iPCMSampleY[i] = from->m_iPCMSampleY[i];
        }

        for (int i = 0; i < (tmp >> 2); i++)
        {
            m_trCoeffCb[i] = from->m_trCoeffCb[i];
            m_trCoeffCr[i] = from->m_trCoeffCr[i];
            m_iPCMSampleCb[i] = from->m_iPCMSampleCb[i];
            m_iPCMSampleCr[i] = from->m_iPCMSampleCr[i];
        }
    }

    // Setting neighbor CU
    m_cuLeft        = NULL;
    m_cuAbove       = NULL;
    m_cuAboveLeft   = NULL;
    m_cuAboveRight  = NULL;

    m_cuColocated[0] = NULL;
    m_cuColocated[1] = NULL;

    UInt uiWidthInCU = pic->getFrameWidthInCU();
    if (m_cuAddr % uiWidthInCU)
    {
        m_cuLeft = pic->getCU(m_cuAddr - 1);
    }

    if (m_cuAddr / uiWidthInCU)
    {
        m_cuAbove = pic->getCU(m_cuAddr - uiWidthInCU);
    }

    if (m_cuLeft && m_cuAbove)
    {
        m_cuAboveLeft = pic->getCU(m_cuAddr - uiWidthInCU - 1);
    }

    if (m_cuAbove && ((m_cuAddr % uiWidthInCU) < (uiWidthInCU - 1)))
    {
        m_cuAboveRight = pic->getCU(m_cuAddr - uiWidthInCU + 1);
    }

    if (getSlice()->getNumRefIdx(REF_PIC_LIST_0) > 0)
    {
        m_cuColocated[0] = getSlice()->getRefPic(REF_PIC_LIST_0, 0)->getCU(m_cuAddr);
    }

    if (getSlice()->getNumRefIdx(REF_PIC_LIST_1) > 0)
    {
        m_cuColocated[1] = getSlice()->getRefPic(REF_PIC_LIST_1, 0)->getCU(m_cuAddr);
    }
}

/** initialize prediction data with enabling sub-LCU-level delta QP
*\param  depth  depth of the current CU
*\param  qp     qp for the current CU
*- set CU width and CU height according to depth
*- set qp value according to input qp
*- set last-coded qp value according to input last-coded qp
*/
void TComDataCU::initEstData(UInt depth, int qp)
{
    m_totalCost        = MAX_INT64;
    m_totalDistortion  = 0;
    m_totalBits        = 0;
    m_totalBins        = 0;

    UChar width  = g_maxCUWidth  >> depth;
    UChar height = g_maxCUHeight >> depth;

    for (UInt i = 0; i < m_numPartitions; i++)
    {
        m_mvpIdx[0][i] = -1;
        m_mvpIdx[1][i] = -1;
        m_mvpNum[0][i] = -1;
        m_mvpNum[1][i] = -1;
        m_depth[i] = depth;
        m_width[i] = width;
        m_height[i] = height;
        m_trIdx[i] = 0;
        m_transformSkip[0][i] = 0;
        m_transformSkip[1][i] = 0;
        m_transformSkip[2][i] = 0;
        m_skipFlag[i]   = false;
        m_partSizes[i] = SIZE_NONE;
        m_predModes[i] = MODE_NONE;
        m_cuTransquantBypass[i] = false;
        m_iPCMFlags[i] = 0;
        m_qp[i] = qp;
        m_bMergeFlags[i] = 0;
        m_mergeIndex[i] = 0;
        m_lumaIntraDir[i] = DC_IDX;
        m_chromaIntraDir[i] = 0;
        m_interDir[i] = 0;
        m_cbf[0][i] = 0;
        m_cbf[1][i] = 0;
        m_cbf[2][i] = 0;
    }

    UInt uiTmp = width * height;

    {
        m_cuMvField[0].clearMvField();
        m_cuMvField[1].clearMvField();
        uiTmp = width * height;

        memset(m_trCoeffY,    0, uiTmp * sizeof(*m_trCoeffY));
        memset(m_iPCMSampleY, 0, uiTmp * sizeof(*m_iPCMSampleY));

        uiTmp >>= 2;
        memset(m_trCoeffCb,    0, uiTmp * sizeof(*m_trCoeffCb));
        memset(m_trCoeffCr,    0, uiTmp * sizeof(*m_trCoeffCr));
        memset(m_iPCMSampleCb, 0, uiTmp * sizeof(*m_iPCMSampleCb));
        memset(m_iPCMSampleCr, 0, uiTmp * sizeof(*m_iPCMSampleCr));
    }
}

// initialize Sub partition
void TComDataCU::initSubCU(TComDataCU* cu, UInt partUnitIdx, UInt depth, int qp)
{
    assert(partUnitIdx < 4);

    UInt partOffset = (cu->getTotalNumPart() >> 2) * partUnitIdx;

    m_pic              = cu->getPic();
    m_slice            = m_pic->getSlice();
    m_cuAddr           = cu->getAddr();
    m_absIdxInLCU      = cu->getZorderIdxInCU() + partOffset;

    m_cuPelX           = cu->getCUPelX() + (g_maxCUWidth >> depth) * (partUnitIdx &  1);
    m_cuPelY           = cu->getCUPelY() + (g_maxCUHeight >> depth) * (partUnitIdx >> 1);

    m_totalCost        = MAX_INT64;
    m_totalDistortion  = 0;
    m_totalBits        = 0;
    m_totalBins        = 0;
    m_numPartitions    = cu->getTotalNumPart() >> 2;

    int iSizeInUchar = sizeof(UChar) * m_numPartitions;
    int iSizeInBool  = sizeof(bool) * m_numPartitions;

    int sizeInChar = sizeof(char) * m_numPartitions;
    memset(m_qp, qp, sizeInChar);

    memset(m_bMergeFlags,     0, iSizeInBool);
    memset(m_mergeIndex,      0, iSizeInUchar);
    memset(m_lumaIntraDir,    DC_IDX, iSizeInUchar);
    memset(m_chromaIntraDir,  0, iSizeInUchar);
    memset(m_interDir,        0, iSizeInUchar);
    memset(m_trIdx,           0, iSizeInUchar);
    memset(m_transformSkip[0], 0, iSizeInUchar);
    memset(m_transformSkip[1], 0, iSizeInUchar);
    memset(m_transformSkip[2], 0, iSizeInUchar);
    memset(m_cbf[0],          0, iSizeInUchar);
    memset(m_cbf[1],          0, iSizeInUchar);
    memset(m_cbf[2],          0, iSizeInUchar);
    memset(m_depth, depth, iSizeInUchar);

    UChar width  = g_maxCUWidth  >> depth;
    UChar heigth = g_maxCUHeight >> depth;
    memset(m_width,     width,  iSizeInUchar);
    memset(m_height,    heigth, iSizeInUchar);
    memset(m_iPCMFlags, 0, iSizeInBool);
    for (UInt i = 0; i < m_numPartitions; i++)
    {
        m_skipFlag[i]   = false;
        m_partSizes[i] = SIZE_NONE;
        m_predModes[i] = MODE_NONE;
        m_cuTransquantBypass[i] = false;
        m_mvpIdx[0][i] = -1;
        m_mvpIdx[1][i] = -1;
        m_mvpNum[0][i] = -1;
        m_mvpNum[1][i] = -1;
    }

    UInt tmp = width * heigth;
    memset(m_trCoeffY, 0, sizeof(TCoeff) * tmp);
    memset(m_iPCMSampleY, 0, sizeof(Pel) * tmp);
    tmp >>= 2;
    memset(m_trCoeffCb, 0, sizeof(TCoeff) * tmp);
    memset(m_trCoeffCr, 0, sizeof(TCoeff) * tmp);
    memset(m_iPCMSampleCb, 0, sizeof(Pel) * tmp);
    memset(m_iPCMSampleCr, 0, sizeof(Pel) * tmp);
    m_cuMvField[0].clearMvField();
    m_cuMvField[1].clearMvField();

    m_cuLeft        = cu->getCULeft();
    m_cuAbove       = cu->getCUAbove();
    m_cuAboveLeft   = cu->getCUAboveLeft();
    m_cuAboveRight  = cu->getCUAboveRight();

    m_cuColocated[0] = cu->getCUColocated(REF_PIC_LIST_0);
    m_cuColocated[1] = cu->getCUColocated(REF_PIC_LIST_1);
}

void TComDataCU::setOutsideCUPart(UInt absPartIdx, UInt depth)
{
    UInt numPartition = m_numPartitions >> (depth << 1);
    UInt sizeInUChar = sizeof(UChar) * numPartition;

    UChar width  = g_maxCUWidth  >> depth;
    UChar height = g_maxCUHeight >> depth;

    memset(m_depth  + absPartIdx, depth,  sizeInUChar);
    memset(m_width  + absPartIdx, width,  sizeInUChar);
    memset(m_height + absPartIdx, height, sizeInUChar);
}

// --------------------------------------------------------------------------------------------------------------------
// Copy
// --------------------------------------------------------------------------------------------------------------------

void TComDataCU::copySubCU(TComDataCU* cu, UInt absPartIdx, UInt depth)
{
    UInt part = absPartIdx;

    m_pic              = cu->getPic();
    m_slice            = cu->getSlice();
    m_cuAddr           = cu->getAddr();
    m_absIdxInLCU      = absPartIdx;

    m_cuPelX           = cu->getCUPelX() + g_rasterToPelX[g_zscanToRaster[absPartIdx]];
    m_cuPelY           = cu->getCUPelY() + g_rasterToPelY[g_zscanToRaster[absPartIdx]];

    UInt width         = g_maxCUWidth  >> depth;
    UInt height        = g_maxCUHeight >> depth;

    m_skipFlag = cu->getSkipFlag() + part;

    m_qp = cu->getQP() + part;
    m_partSizes = cu->getPartitionSize() + part;
    m_predModes = cu->getPredictionMode() + part;
    m_cuTransquantBypass  = cu->getCUTransquantBypass() + part;

    m_bMergeFlags      = cu->getMergeFlag() + part;
    m_mergeIndex       = cu->getMergeIndex() + part;

    m_lumaIntraDir     = cu->getLumaIntraDir()     + part;
    m_chromaIntraDir   = cu->getChromaIntraDir()   + part;
    m_interDir         = cu->getInterDir()         + part;
    m_trIdx            = cu->getTransformIdx()     + part;
    m_transformSkip[0] = cu->getTransformSkip(TEXT_LUMA)     + part;
    m_transformSkip[1] = cu->getTransformSkip(TEXT_CHROMA_U) + part;
    m_transformSkip[2] = cu->getTransformSkip(TEXT_CHROMA_V) + part;

    m_cbf[0] = cu->getCbf(TEXT_LUMA)     + part;
    m_cbf[1] = cu->getCbf(TEXT_CHROMA_U) + part;
    m_cbf[2] = cu->getCbf(TEXT_CHROMA_V) + part;

    m_depth = cu->getDepth()   + part;
    m_width = cu->getWidth()   + part;
    m_height = cu->getHeight() + part;

    m_mvpIdx[0] = cu->getMVPIdx(REF_PIC_LIST_0)  + part;
    m_mvpIdx[1] = cu->getMVPIdx(REF_PIC_LIST_1)  + part;
    m_mvpNum[0] = cu->getMVPNum(REF_PIC_LIST_0)  + part;
    m_mvpNum[1] = cu->getMVPNum(REF_PIC_LIST_1)  + part;

    m_iPCMFlags = cu->getIPCMFlag() + part;

    m_cuAboveLeft  = cu->getCUAboveLeft();
    m_cuAboveRight = cu->getCUAboveRight();
    m_cuAbove      = cu->getCUAbove();
    m_cuLeft       = cu->getCULeft();

    m_cuColocated[0] = cu->getCUColocated(REF_PIC_LIST_0);
    m_cuColocated[1] = cu->getCUColocated(REF_PIC_LIST_1);

    UInt tmp = width * height;
    UInt maxCUWidth = cu->getSlice()->getSPS()->getMaxCUWidth();
    UInt maxCUHeight = cu->getSlice()->getSPS()->getMaxCUHeight();
    UInt coeffOffset = maxCUWidth * maxCUHeight * absPartIdx / cu->getPic()->getNumPartInCU();

    m_trCoeffY = cu->getCoeffY() + coeffOffset;
    m_iPCMSampleY = cu->getPCMSampleY() + coeffOffset;

    tmp >>= 2;
    coeffOffset >>= 2;
    m_trCoeffCb = cu->getCoeffCb() + coeffOffset;
    m_trCoeffCr = cu->getCoeffCr() + coeffOffset;
    m_iPCMSampleCb = cu->getPCMSampleCb() + coeffOffset;
    m_iPCMSampleCr = cu->getPCMSampleCr() + coeffOffset;

    m_cuMvField[0].linkToWithOffset(cu->getCUMvField(REF_PIC_LIST_0), part);
    m_cuMvField[1].linkToWithOffset(cu->getCUMvField(REF_PIC_LIST_1), part);
}

// Copy inter prediction info from the biggest CU
void TComDataCU::copyInterPredInfoFrom(TComDataCU* cu, UInt absPartIdx, RefPicList picList)
{
    m_pic              = cu->getPic();
    m_slice            = cu->getSlice();
    m_cuAddr           = cu->getAddr();
    m_absIdxInLCU      = absPartIdx;

    int rastPartIdx     = g_zscanToRaster[absPartIdx];
    m_cuPelX           = cu->getCUPelX() + m_pic->getMinCUWidth() * (rastPartIdx % m_pic->getNumPartInWidth());
    m_cuPelY           = cu->getCUPelY() + m_pic->getMinCUHeight() * (rastPartIdx / m_pic->getNumPartInWidth());

    m_cuAboveLeft      = cu->getCUAboveLeft();
    m_cuAboveRight     = cu->getCUAboveRight();
    m_cuAbove          = cu->getCUAbove();
    m_cuLeft           = cu->getCULeft();

    m_cuColocated[0]  = cu->getCUColocated(REF_PIC_LIST_0);
    m_cuColocated[1]  = cu->getCUColocated(REF_PIC_LIST_1);

    m_skipFlag           = cu->getSkipFlag() + absPartIdx;

    m_partSizes         = cu->getPartitionSize() + absPartIdx;
    m_predModes         = cu->getPredictionMode() + absPartIdx;
    m_cuTransquantBypass = cu->getCUTransquantBypass() + absPartIdx;
    m_interDir        = cu->getInterDir() + absPartIdx;

    m_depth           = cu->getDepth()                + absPartIdx;
    m_width           = cu->getWidth()                + absPartIdx;
    m_height          = cu->getHeight()               + absPartIdx;

    m_bMergeFlags     = cu->getMergeFlag()            + absPartIdx;
    m_mergeIndex      = cu->getMergeIndex()           + absPartIdx;

    m_mvpIdx[picList] = cu->getMVPIdx(picList) + absPartIdx;
    m_mvpNum[picList] = cu->getMVPNum(picList) + absPartIdx;

    m_cuMvField[picList].linkToWithOffset(cu->getCUMvField(picList), absPartIdx);
}

// Copy small CU to bigger CU.
// One of quarter parts overwritten by predicted sub part.
void TComDataCU::copyPartFrom(TComDataCU* cu, UInt partUnitIdx, UInt depth, bool isRDObasedAnalysis)
{
    assert(partUnitIdx < 4);
    if (isRDObasedAnalysis)
        m_totalCost += cu->m_totalCost;

    m_totalDistortion  += cu->m_totalDistortion;
    m_totalBits        += cu->m_totalBits;

    UInt offset         = cu->getTotalNumPart() * partUnitIdx;

    UInt numPartition = cu->getTotalNumPart();
    int iSizeInUchar  = sizeof(UChar) * numPartition;
    int iSizeInBool   = sizeof(bool) * numPartition;

    int sizeInChar  = sizeof(char) * numPartition;
    memcpy(m_skipFlag  + offset, cu->getSkipFlag(),       sizeof(*m_skipFlag)   * numPartition);
    memcpy(m_qp        + offset, cu->getQP(),             sizeInChar);
    memcpy(m_partSizes + offset, cu->getPartitionSize(),  sizeof(*m_partSizes) * numPartition);
    memcpy(m_predModes + offset, cu->getPredictionMode(), sizeof(*m_predModes) * numPartition);
    memcpy(m_cuTransquantBypass + offset, cu->getCUTransquantBypass(), sizeof(*m_cuTransquantBypass) * numPartition);
    memcpy(m_bMergeFlags      + offset, cu->getMergeFlag(),         iSizeInBool);
    memcpy(m_mergeIndex       + offset, cu->getMergeIndex(),        iSizeInUchar);
    memcpy(m_lumaIntraDir     + offset, cu->getLumaIntraDir(),      iSizeInUchar);
    memcpy(m_chromaIntraDir   + offset, cu->getChromaIntraDir(),    iSizeInUchar);
    memcpy(m_interDir         + offset, cu->getInterDir(),          iSizeInUchar);
    memcpy(m_trIdx            + offset, cu->getTransformIdx(),      iSizeInUchar);
    memcpy(m_transformSkip[0] + offset, cu->getTransformSkip(TEXT_LUMA),     iSizeInUchar);
    memcpy(m_transformSkip[1] + offset, cu->getTransformSkip(TEXT_CHROMA_U), iSizeInUchar);
    memcpy(m_transformSkip[2] + offset, cu->getTransformSkip(TEXT_CHROMA_V), iSizeInUchar);

    memcpy(m_cbf[0] + offset, cu->getCbf(TEXT_LUMA), iSizeInUchar);
    memcpy(m_cbf[1] + offset, cu->getCbf(TEXT_CHROMA_U), iSizeInUchar);
    memcpy(m_cbf[2] + offset, cu->getCbf(TEXT_CHROMA_V), iSizeInUchar);

    memcpy(m_depth  + offset, cu->getDepth(),  iSizeInUchar);
    memcpy(m_width  + offset, cu->getWidth(),  iSizeInUchar);
    memcpy(m_height + offset, cu->getHeight(), iSizeInUchar);

    memcpy(m_mvpIdx[0] + offset, cu->getMVPIdx(REF_PIC_LIST_0), iSizeInUchar);
    memcpy(m_mvpIdx[1] + offset, cu->getMVPIdx(REF_PIC_LIST_1), iSizeInUchar);
    memcpy(m_mvpNum[0] + offset, cu->getMVPNum(REF_PIC_LIST_0), iSizeInUchar);
    memcpy(m_mvpNum[1] + offset, cu->getMVPNum(REF_PIC_LIST_1), iSizeInUchar);

    memcpy(m_iPCMFlags + offset, cu->getIPCMFlag(), iSizeInBool);

    m_cuAboveLeft      = cu->getCUAboveLeft();
    m_cuAboveRight     = cu->getCUAboveRight();
    m_cuAbove          = cu->getCUAbove();
    m_cuLeft           = cu->getCULeft();

    m_cuColocated[0] = cu->getCUColocated(REF_PIC_LIST_0);
    m_cuColocated[1] = cu->getCUColocated(REF_PIC_LIST_1);

    m_cuMvField[0].copyFrom(cu->getCUMvField(REF_PIC_LIST_0), cu->getTotalNumPart(), offset);
    m_cuMvField[1].copyFrom(cu->getCUMvField(REF_PIC_LIST_1), cu->getTotalNumPart(), offset);

    UInt uiTmp  = g_maxCUWidth * g_maxCUHeight >> (depth << 1);
    UInt uiTmp2 = partUnitIdx * uiTmp;
    memcpy(m_trCoeffY  + uiTmp2, cu->getCoeffY(),  sizeof(TCoeff) * uiTmp);
    memcpy(m_iPCMSampleY + uiTmp2, cu->getPCMSampleY(), sizeof(Pel) * uiTmp);

    uiTmp >>= 2;
    uiTmp2 >>= 2;
    memcpy(m_trCoeffCb + uiTmp2, cu->getCoeffCb(), sizeof(TCoeff) * uiTmp);
    memcpy(m_trCoeffCr + uiTmp2, cu->getCoeffCr(), sizeof(TCoeff) * uiTmp);
    memcpy(m_iPCMSampleCb + uiTmp2, cu->getPCMSampleCb(), sizeof(Pel) * uiTmp);
    memcpy(m_iPCMSampleCr + uiTmp2, cu->getPCMSampleCr(), sizeof(Pel) * uiTmp);
    m_totalBins += cu->m_totalBins;
}

// Copy current predicted part to a CU in picture.
// It is used to predict for next part
void TComDataCU::copyToPic(UChar uhDepth)
{
    TComDataCU* rpcCU = m_pic->getCU(m_cuAddr);

    rpcCU->m_totalCost       = m_totalCost;
    rpcCU->m_totalDistortion = m_totalDistortion;
    rpcCU->m_totalBits       = m_totalBits;

    int iSizeInUchar  = sizeof(UChar) * m_numPartitions;
    int iSizeInBool   = sizeof(bool) * m_numPartitions;

    int sizeInChar  = sizeof(char) * m_numPartitions;

    memcpy(rpcCU->getSkipFlag() + m_absIdxInLCU, m_skipFlag, sizeof(*m_skipFlag) * m_numPartitions);

    memcpy(rpcCU->getQP() + m_absIdxInLCU, m_qp, sizeInChar);

    memcpy(rpcCU->getPartitionSize()  + m_absIdxInLCU, m_partSizes, sizeof(*m_partSizes) * m_numPartitions);
    memcpy(rpcCU->getPredictionMode() + m_absIdxInLCU, m_predModes, sizeof(*m_predModes) * m_numPartitions);
    memcpy(rpcCU->getCUTransquantBypass() + m_absIdxInLCU, m_cuTransquantBypass, sizeof(*m_cuTransquantBypass) * m_numPartitions);
    memcpy(rpcCU->getMergeFlag()         + m_absIdxInLCU, m_bMergeFlags,         iSizeInBool);
    memcpy(rpcCU->getMergeIndex()        + m_absIdxInLCU, m_mergeIndex,       iSizeInUchar);
    memcpy(rpcCU->getLumaIntraDir()      + m_absIdxInLCU, m_lumaIntraDir,     iSizeInUchar);
    memcpy(rpcCU->getChromaIntraDir()    + m_absIdxInLCU, m_chromaIntraDir,   iSizeInUchar);
    memcpy(rpcCU->getInterDir()          + m_absIdxInLCU, m_interDir,         iSizeInUchar);
    memcpy(rpcCU->getTransformIdx()      + m_absIdxInLCU, m_trIdx,            iSizeInUchar);
    memcpy(rpcCU->getTransformSkip(TEXT_LUMA)     + m_absIdxInLCU, m_transformSkip[0], iSizeInUchar);
    memcpy(rpcCU->getTransformSkip(TEXT_CHROMA_U) + m_absIdxInLCU, m_transformSkip[1], iSizeInUchar);
    memcpy(rpcCU->getTransformSkip(TEXT_CHROMA_V) + m_absIdxInLCU, m_transformSkip[2], iSizeInUchar);

    memcpy(rpcCU->getCbf(TEXT_LUMA)     + m_absIdxInLCU, m_cbf[0], iSizeInUchar);
    memcpy(rpcCU->getCbf(TEXT_CHROMA_U) + m_absIdxInLCU, m_cbf[1], iSizeInUchar);
    memcpy(rpcCU->getCbf(TEXT_CHROMA_V) + m_absIdxInLCU, m_cbf[2], iSizeInUchar);

    memcpy(rpcCU->getDepth()  + m_absIdxInLCU, m_depth,  iSizeInUchar);
    memcpy(rpcCU->getWidth()  + m_absIdxInLCU, m_width,  iSizeInUchar);
    memcpy(rpcCU->getHeight() + m_absIdxInLCU, m_height, iSizeInUchar);

    memcpy(rpcCU->getMVPIdx(REF_PIC_LIST_0) + m_absIdxInLCU, m_mvpIdx[0], iSizeInUchar);
    memcpy(rpcCU->getMVPIdx(REF_PIC_LIST_1) + m_absIdxInLCU, m_mvpIdx[1], iSizeInUchar);
    memcpy(rpcCU->getMVPNum(REF_PIC_LIST_0) + m_absIdxInLCU, m_mvpNum[0], iSizeInUchar);
    memcpy(rpcCU->getMVPNum(REF_PIC_LIST_1) + m_absIdxInLCU, m_mvpNum[1], iSizeInUchar);

    m_cuMvField[0].copyTo(rpcCU->getCUMvField(REF_PIC_LIST_0), m_absIdxInLCU);
    m_cuMvField[1].copyTo(rpcCU->getCUMvField(REF_PIC_LIST_1), m_absIdxInLCU);

    memcpy(rpcCU->getIPCMFlag() + m_absIdxInLCU, m_iPCMFlags,         iSizeInBool);

    UInt tmp  = (g_maxCUWidth * g_maxCUHeight) >> (uhDepth << 1);
    UInt tmp2 = m_absIdxInLCU * m_pic->getMinCUWidth() * m_pic->getMinCUHeight();
    memcpy(rpcCU->getCoeffY()     + tmp2, m_trCoeffY,    sizeof(TCoeff) * tmp);
    memcpy(rpcCU->getPCMSampleY() + tmp2, m_iPCMSampleY, sizeof(Pel) * tmp);

    tmp >>= 2;
    tmp2 >>= 2;
    memcpy(rpcCU->getCoeffCb() + tmp2, m_trCoeffCb, sizeof(TCoeff) * tmp);
    memcpy(rpcCU->getCoeffCr() + tmp2, m_trCoeffCr, sizeof(TCoeff) * tmp);
    memcpy(rpcCU->getPCMSampleCb() + tmp2, m_iPCMSampleCb, sizeof(Pel) * tmp);
    memcpy(rpcCU->getPCMSampleCr() + tmp2, m_iPCMSampleCr, sizeof(Pel) * tmp);
    rpcCU->m_totalBins = m_totalBins;
}

void TComDataCU::copyToPic(UChar depth, UInt partIdx, UInt partDepth)
{
    TComDataCU* cu = m_pic->getCU(m_cuAddr);
    UInt qNumPart  = m_numPartitions >> (partDepth << 1);

    UInt uiPartStart = partIdx * qNumPart;
    UInt partOffset  = m_absIdxInLCU + uiPartStart;

    cu->m_totalCost       = m_totalCost;
    cu->m_totalDistortion = m_totalDistortion;
    cu->m_totalBits       = m_totalBits;

    int sizeInUchar = sizeof(UChar) * qNumPart;
    int sizeInBool  = sizeof(bool) * qNumPart;

    int sizeInChar  = sizeof(char) * qNumPart;
    memcpy(cu->getSkipFlag() + partOffset, m_skipFlag,   sizeof(*m_skipFlag)   * qNumPart);

    memcpy(cu->getQP() + partOffset, m_qp, sizeInChar);
    memcpy(cu->getPartitionSize()  + partOffset, m_partSizes, sizeof(*m_partSizes) * qNumPart);
    memcpy(cu->getPredictionMode() + partOffset, m_predModes, sizeof(*m_predModes) * qNumPart);
    memcpy(cu->getCUTransquantBypass() + partOffset, m_cuTransquantBypass, sizeof(*m_cuTransquantBypass) * qNumPart);
    memcpy(cu->getMergeFlag()         + partOffset, m_bMergeFlags,      sizeInBool);
    memcpy(cu->getMergeIndex()        + partOffset, m_mergeIndex,       sizeInUchar);
    memcpy(cu->getLumaIntraDir()      + partOffset, m_lumaIntraDir,     sizeInUchar);
    memcpy(cu->getChromaIntraDir()    + partOffset, m_chromaIntraDir,   sizeInUchar);
    memcpy(cu->getInterDir()          + partOffset, m_interDir,         sizeInUchar);
    memcpy(cu->getTransformIdx()      + partOffset, m_trIdx,            sizeInUchar);
    memcpy(cu->getTransformSkip(TEXT_LUMA)     + partOffset, m_transformSkip[0], sizeInUchar);
    memcpy(cu->getTransformSkip(TEXT_CHROMA_U) + partOffset, m_transformSkip[1], sizeInUchar);
    memcpy(cu->getTransformSkip(TEXT_CHROMA_V) + partOffset, m_transformSkip[2], sizeInUchar);
    memcpy(cu->getCbf(TEXT_LUMA)     + partOffset, m_cbf[0], sizeInUchar);
    memcpy(cu->getCbf(TEXT_CHROMA_U) + partOffset, m_cbf[1], sizeInUchar);
    memcpy(cu->getCbf(TEXT_CHROMA_V) + partOffset, m_cbf[2], sizeInUchar);

    memcpy(cu->getDepth()  + partOffset, m_depth,  sizeInUchar);
    memcpy(cu->getWidth()  + partOffset, m_width,  sizeInUchar);
    memcpy(cu->getHeight() + partOffset, m_height, sizeInUchar);

    memcpy(cu->getMVPIdx(REF_PIC_LIST_0) + partOffset, m_mvpIdx[0], sizeInUchar);
    memcpy(cu->getMVPIdx(REF_PIC_LIST_1) + partOffset, m_mvpIdx[1], sizeInUchar);
    memcpy(cu->getMVPNum(REF_PIC_LIST_0) + partOffset, m_mvpNum[0], sizeInUchar);
    memcpy(cu->getMVPNum(REF_PIC_LIST_1) + partOffset, m_mvpNum[1], sizeInUchar);
    m_cuMvField[0].copyTo(cu->getCUMvField(REF_PIC_LIST_0), m_absIdxInLCU, uiPartStart, qNumPart);
    m_cuMvField[1].copyTo(cu->getCUMvField(REF_PIC_LIST_1), m_absIdxInLCU, uiPartStart, qNumPart);

    memcpy(cu->getIPCMFlag() + partOffset, m_iPCMFlags, sizeInBool);

    UInt tmp  = (g_maxCUWidth * g_maxCUHeight) >> ((depth + partDepth) << 1);
    UInt tmp2 = partOffset * m_pic->getMinCUWidth() * m_pic->getMinCUHeight();
    memcpy(cu->getCoeffY()  + tmp2, m_trCoeffY,  sizeof(TCoeff) * tmp);
    memcpy(cu->getPCMSampleY() + tmp2, m_iPCMSampleY, sizeof(Pel) * tmp);

    tmp >>= 2;
    tmp2 >>= 2;
    memcpy(cu->getCoeffCb() + tmp2, m_trCoeffCb, sizeof(TCoeff) * tmp);
    memcpy(cu->getCoeffCr() + tmp2, m_trCoeffCr, sizeof(TCoeff) * tmp);
    memcpy(cu->getPCMSampleCb() + tmp2, m_iPCMSampleCb, sizeof(Pel) * tmp);
    memcpy(cu->getPCMSampleCr() + tmp2, m_iPCMSampleCr, sizeof(Pel) * tmp);
    cu->m_totalBins = m_totalBins;
}

// --------------------------------------------------------------------------------------------------------------------
// Other public functions
// --------------------------------------------------------------------------------------------------------------------

TComDataCU* TComDataCU::getPULeft(UInt& lPartUnitIdx, UInt curPartUnitIdx, bool  bEnforceSliceRestriction, bool  bEnforceTileRestriction)
{
    UInt absPartIdx       = g_zscanToRaster[curPartUnitIdx];
    UInt absZorderCUIdx   = g_zscanToRaster[m_absIdxInLCU];
    UInt numPartInCUWidth = m_pic->getNumPartInWidth();

    if (!RasterAddress::isZeroCol(absPartIdx, numPartInCUWidth))
    {
        lPartUnitIdx = g_rasterToZscan[absPartIdx - 1];
        if (RasterAddress::isEqualCol(absPartIdx, absZorderCUIdx, numPartInCUWidth))
        {
            return m_pic->getCU(getAddr());
        }
        else
        {
            lPartUnitIdx -= m_absIdxInLCU;
            return this;
        }
    }

    lPartUnitIdx = g_rasterToZscan[absPartIdx + numPartInCUWidth - 1];

    if ((bEnforceSliceRestriction && (m_cuLeft == NULL || m_cuLeft->getSlice() == NULL)) ||
        (bEnforceTileRestriction && (m_cuLeft == NULL || m_cuLeft->getSlice() == NULL)))
    {
        return NULL;
    }
    return m_cuLeft;
}

TComDataCU* TComDataCU::getPUAbove(UInt& aPartUnitIdx, UInt  curPartUnitIdx, bool bEnforceSliceRestriction, bool planarAtLCUBoundary,
                                   bool bEnforceTileRestriction)
{
    UInt absPartIdx       = g_zscanToRaster[curPartUnitIdx];
    UInt absZorderCUIdx   = g_zscanToRaster[m_absIdxInLCU];
    UInt numPartInCUWidth = m_pic->getNumPartInWidth();

    if (!RasterAddress::isZeroRow(absPartIdx, numPartInCUWidth))
    {
        aPartUnitIdx = g_rasterToZscan[absPartIdx - numPartInCUWidth];
        if (RasterAddress::isEqualRow(absPartIdx, absZorderCUIdx, numPartInCUWidth))
        {
            return m_pic->getCU(getAddr());
        }
        else
        {
            aPartUnitIdx -= m_absIdxInLCU;
            return this;
        }
    }

    if (planarAtLCUBoundary)
    {
        return NULL;
    }

    aPartUnitIdx = g_rasterToZscan[absPartIdx + m_pic->getNumPartInCU() - numPartInCUWidth];

    if ((bEnforceSliceRestriction && (m_cuAbove == NULL || m_cuAbove->getSlice() == NULL)) ||
        (bEnforceTileRestriction && (m_cuAbove == NULL || m_cuAbove->getSlice() == NULL)))
    {
        return NULL;
    }
    return m_cuAbove;
}

TComDataCU* TComDataCU::getPUAboveLeft(UInt& alPartUnitIdx, UInt curPartUnitIdx, bool bEnforceSliceRestriction)
{
    UInt absPartIdx       = g_zscanToRaster[curPartUnitIdx];
    UInt absZorderCUIdx   = g_zscanToRaster[m_absIdxInLCU];
    UInt numPartInCUWidth = m_pic->getNumPartInWidth();

    if (!RasterAddress::isZeroCol(absPartIdx, numPartInCUWidth))
    {
        if (!RasterAddress::isZeroRow(absPartIdx, numPartInCUWidth))
        {
            alPartUnitIdx = g_rasterToZscan[absPartIdx - numPartInCUWidth - 1];
            if (RasterAddress::isEqualRowOrCol(absPartIdx, absZorderCUIdx, numPartInCUWidth))
            {
                return m_pic->getCU(getAddr());
            }
            else
            {
                alPartUnitIdx -= m_absIdxInLCU;
                return this;
            }
        }
        alPartUnitIdx = g_rasterToZscan[absPartIdx + getPic()->getNumPartInCU() - numPartInCUWidth - 1];
        if ((bEnforceSliceRestriction && (m_cuAbove == NULL || m_cuAbove->getSlice() == NULL)))
        {
            return NULL;
        }
        return m_cuAbove;
    }

    if (!RasterAddress::isZeroRow(absPartIdx, numPartInCUWidth))
    {
        alPartUnitIdx = g_rasterToZscan[absPartIdx - 1];
        if ((bEnforceSliceRestriction && (m_cuLeft == NULL || m_cuLeft->getSlice() == NULL))
            )
        {
            return NULL;
        }
        return m_cuLeft;
    }

    alPartUnitIdx = g_rasterToZscan[m_pic->getNumPartInCU() - 1];
    if ((bEnforceSliceRestriction && (m_cuAboveLeft == NULL || m_cuAboveLeft->getSlice() == NULL)))
    {
        return NULL;
    }
    return m_cuAboveLeft;
}

TComDataCU* TComDataCU::getPUAboveRight(UInt& arPartUnitIdx, UInt curPartUnitIdx, bool bEnforceSliceRestriction)
{
    UInt absPartIdxRT     = g_zscanToRaster[curPartUnitIdx];
    UInt absZorderCUIdx   = g_zscanToRaster[m_absIdxInLCU] + m_width[0] / m_pic->getMinCUWidth() - 1;
    UInt numPartInCUWidth = m_pic->getNumPartInWidth();

    if ((m_pic->getCU(m_cuAddr)->getCUPelX() + g_rasterToPelX[absPartIdxRT] + m_pic->getMinCUWidth()) >= m_slice->getSPS()->getPicWidthInLumaSamples())
    {
        arPartUnitIdx = MAX_UINT;
        return NULL;
    }

    if (RasterAddress::lessThanCol(absPartIdxRT, numPartInCUWidth - 1, numPartInCUWidth))
    {
        if (!RasterAddress::isZeroRow(absPartIdxRT, numPartInCUWidth))
        {
            if (curPartUnitIdx > g_rasterToZscan[absPartIdxRT - numPartInCUWidth + 1])
            {
                arPartUnitIdx = g_rasterToZscan[absPartIdxRT - numPartInCUWidth + 1];
                if (RasterAddress::isEqualRowOrCol(absPartIdxRT, absZorderCUIdx, numPartInCUWidth))
                {
                    return m_pic->getCU(getAddr());
                }
                else
                {
                    arPartUnitIdx -= m_absIdxInLCU;
                    return this;
                }
            }
            arPartUnitIdx = MAX_UINT;
            return NULL;
        }
        arPartUnitIdx = g_rasterToZscan[absPartIdxRT + m_pic->getNumPartInCU() - numPartInCUWidth + 1];
        if ((bEnforceSliceRestriction && (m_cuAbove == NULL || m_cuAbove->getSlice() == NULL)))
        {
            return NULL;
        }
        return m_cuAbove;
    }

    if (!RasterAddress::isZeroRow(absPartIdxRT, numPartInCUWidth))
    {
        arPartUnitIdx = MAX_UINT;
        return NULL;
    }

    arPartUnitIdx = g_rasterToZscan[m_pic->getNumPartInCU() - numPartInCUWidth];
    if ((bEnforceSliceRestriction && (m_cuAboveRight == NULL || m_cuAboveRight->getSlice() == NULL ||
                                      (m_cuAboveRight->getAddr()) > getAddr())))
    {
        return NULL;
    }
    return m_cuAboveRight;
}

TComDataCU* TComDataCU::getPUBelowLeft(UInt& blPartUnitIdx, UInt curPartUnitIdx, bool bEnforceSliceRestriction)
{
    UInt absPartIdxLB     = g_zscanToRaster[curPartUnitIdx];
    UInt absZorderCUIdxLB = g_zscanToRaster[m_absIdxInLCU] + (m_height[0] / m_pic->getMinCUHeight() - 1) * m_pic->getNumPartInWidth();
    UInt numPartInCUWidth = m_pic->getNumPartInWidth();

    if ((m_pic->getCU(m_cuAddr)->getCUPelY() + g_rasterToPelY[absPartIdxLB] + m_pic->getMinCUHeight()) >= m_slice->getSPS()->getPicHeightInLumaSamples())
    {
        blPartUnitIdx = MAX_UINT;
        return NULL;
    }

    if (RasterAddress::lessThanRow(absPartIdxLB, m_pic->getNumPartInHeight() - 1, numPartInCUWidth))
    {
        if (!RasterAddress::isZeroCol(absPartIdxLB, numPartInCUWidth))
        {
            if (curPartUnitIdx > g_rasterToZscan[absPartIdxLB + numPartInCUWidth - 1])
            {
                blPartUnitIdx = g_rasterToZscan[absPartIdxLB + numPartInCUWidth - 1];
                if (RasterAddress::isEqualRowOrCol(absPartIdxLB, absZorderCUIdxLB, numPartInCUWidth))
                {
                    return m_pic->getCU(getAddr());
                }
                else
                {
                    blPartUnitIdx -= m_absIdxInLCU;
                    return this;
                }
            }
            blPartUnitIdx = MAX_UINT;
            return NULL;
        }
        blPartUnitIdx = g_rasterToZscan[absPartIdxLB + numPartInCUWidth * 2 - 1];
        if ((bEnforceSliceRestriction && (m_cuLeft == NULL || m_cuLeft->getSlice() == NULL)))
        {
            return NULL;
        }
        return m_cuLeft;
    }

    blPartUnitIdx = MAX_UINT;
    return NULL;
}

TComDataCU* TComDataCU::getPUBelowLeftAdi(UInt& blPartUnitIdx,  UInt curPartUnitIdx, UInt partUnitOffset, bool bEnforceSliceRestriction)
{
    UInt absPartIdxLB     = g_zscanToRaster[curPartUnitIdx];
    UInt absZorderCUIdxLB = g_zscanToRaster[m_absIdxInLCU] + ((m_height[0] / m_pic->getMinCUHeight()) - 1) * m_pic->getNumPartInWidth();
    UInt numPartInCUWidth = m_pic->getNumPartInWidth();

    if ((m_pic->getCU(m_cuAddr)->getCUPelY() + g_rasterToPelY[absPartIdxLB] + (m_pic->getPicSym()->getMinCUHeight() * partUnitOffset)) >=
        m_slice->getSPS()->getPicHeightInLumaSamples())
    {
        blPartUnitIdx = MAX_UINT;
        return NULL;
    }

    if (RasterAddress::lessThanRow(absPartIdxLB, m_pic->getNumPartInHeight() - partUnitOffset, numPartInCUWidth))
    {
        if (!RasterAddress::isZeroCol(absPartIdxLB, numPartInCUWidth))
        {
            if (curPartUnitIdx > g_rasterToZscan[absPartIdxLB + partUnitOffset * numPartInCUWidth - 1])
            {
                blPartUnitIdx = g_rasterToZscan[absPartIdxLB + partUnitOffset * numPartInCUWidth - 1];
                if (RasterAddress::isEqualRowOrCol(absPartIdxLB, absZorderCUIdxLB, numPartInCUWidth))
                {
                    return m_pic->getCU(getAddr());
                }
                else
                {
                    blPartUnitIdx -= m_absIdxInLCU;
                    return this;
                }
            }
            blPartUnitIdx = MAX_UINT;
            return NULL;
        }
        blPartUnitIdx = g_rasterToZscan[absPartIdxLB + (1 + partUnitOffset) * numPartInCUWidth - 1];
        if ((bEnforceSliceRestriction && (m_cuLeft == NULL || m_cuLeft->getSlice() == NULL)))
        {
            return NULL;
        }
        return m_cuLeft;
    }

    blPartUnitIdx = MAX_UINT;
    return NULL;
}

TComDataCU* TComDataCU::getPUAboveRightAdi(UInt& arPartUnitIdx, UInt curPartUnitIdx, UInt partUnitOffset, bool bEnforceSliceRestriction)
{
    UInt absPartIdxRT     = g_zscanToRaster[curPartUnitIdx];
    UInt absZorderCUIdx   = g_zscanToRaster[m_absIdxInLCU] + (m_width[0] / m_pic->getMinCUWidth()) - 1;
    UInt numPartInCUWidth = m_pic->getNumPartInWidth();

    if ((m_pic->getCU(m_cuAddr)->getCUPelX() + g_rasterToPelX[absPartIdxRT] + (m_pic->getPicSym()->getMinCUHeight() * partUnitOffset)) >=
         m_slice->getSPS()->getPicWidthInLumaSamples())
    {
        arPartUnitIdx = MAX_UINT;
        return NULL;
    }

    if (RasterAddress::lessThanCol(absPartIdxRT, numPartInCUWidth - partUnitOffset, numPartInCUWidth))
    {
        if (!RasterAddress::isZeroRow(absPartIdxRT, numPartInCUWidth))
        {
            if (curPartUnitIdx > g_rasterToZscan[absPartIdxRT - numPartInCUWidth + partUnitOffset])
            {
                arPartUnitIdx = g_rasterToZscan[absPartIdxRT - numPartInCUWidth + partUnitOffset];
                if (RasterAddress::isEqualRowOrCol(absPartIdxRT, absZorderCUIdx, numPartInCUWidth))
                {
                    return m_pic->getCU(getAddr());
                }
                else
                {
                    arPartUnitIdx -= m_absIdxInLCU;
                    return this;
                }
            }
            arPartUnitIdx = MAX_UINT;
            return NULL;
        }
        arPartUnitIdx = g_rasterToZscan[absPartIdxRT + m_pic->getNumPartInCU() - numPartInCUWidth + partUnitOffset];
        if ((bEnforceSliceRestriction && (m_cuAbove == NULL || m_cuAbove->getSlice() == NULL)))
        {
            return NULL;
        }
        return m_cuAbove;
    }

    if (!RasterAddress::isZeroRow(absPartIdxRT, numPartInCUWidth))
    {
        arPartUnitIdx = MAX_UINT;
        return NULL;
    }

    arPartUnitIdx = g_rasterToZscan[m_pic->getNumPartInCU() - numPartInCUWidth + partUnitOffset - 1];
    if ((bEnforceSliceRestriction && (m_cuAboveRight == NULL || m_cuAboveRight->getSlice() == NULL ||
                                      (m_cuAboveRight->getAddr()) > getAddr())))
    {
        return NULL;
    }
    return m_cuAboveRight;
}

/** Get left QpMinCu
*\param   uiLPartUnitIdx
*\param   uiCurrAbsIdxInLCU
*\returns TComDataCU*   point of TComDataCU of left QpMinCu
*/
TComDataCU* TComDataCU::getQpMinCuLeft(UInt& lPartUnitIdx, UInt curAbsIdxInLCU)
{
    UInt numPartInCUWidth = m_pic->getNumPartInWidth();
    UInt absZorderQpMinCUIdx = (curAbsIdxInLCU >> ((g_maxCUDepth - getSlice()->getPPS()->getMaxCuDQPDepth()) << 1)) <<
                                ((g_maxCUDepth - getSlice()->getPPS()->getMaxCuDQPDepth()) << 1);
    UInt absRorderQpMinCUIdx = g_zscanToRaster[absZorderQpMinCUIdx];

    // check for left LCU boundary
    if (RasterAddress::isZeroCol(absRorderQpMinCUIdx, numPartInCUWidth))
    {
        return NULL;
    }

    // get index of left-CU relative to top-left corner of current quantization group
    lPartUnitIdx = g_rasterToZscan[absRorderQpMinCUIdx - 1];

    // return pointer to current LCU
    return m_pic->getCU(getAddr());
}

/** Get Above QpMinCu
*\param   aPartUnitIdx
*\param   currAbsIdxInLCU
*\returns TComDataCU*   point of TComDataCU of above QpMinCu
*/
TComDataCU* TComDataCU::getQpMinCuAbove(UInt& aPartUnitIdx, UInt curAbsIdxInLCU)
{
    UInt numPartInCUWidth = m_pic->getNumPartInWidth();
    UInt absZorderQpMinCUIdx = (curAbsIdxInLCU >> ((g_maxCUDepth - getSlice()->getPPS()->getMaxCuDQPDepth()) << 1)) <<
                                ((g_maxCUDepth - getSlice()->getPPS()->getMaxCuDQPDepth()) << 1);
    UInt absRorderQpMinCUIdx = g_zscanToRaster[absZorderQpMinCUIdx];

    // check for top LCU boundary
    if (RasterAddress::isZeroRow(absRorderQpMinCUIdx, numPartInCUWidth))
    {
        return NULL;
    }

    // get index of top-CU relative to top-left corner of current quantization group
    aPartUnitIdx = g_rasterToZscan[absRorderQpMinCUIdx - numPartInCUWidth];

    // return pointer to current LCU
    return m_pic->getCU(getAddr());
}

/** Get reference QP from left QpMinCu or latest coded QP
*\param   uiCurrAbsIdxInLCU
*\returns char   reference QP value
*/
char TComDataCU::getRefQP(UInt curAbsIdxInLCU)
{
    UInt lPartIdx = 0, aPartIdx = 0;
    TComDataCU* cULeft  = getQpMinCuLeft(lPartIdx, m_absIdxInLCU + curAbsIdxInLCU);
    TComDataCU* cUAbove = getQpMinCuAbove(aPartIdx, m_absIdxInLCU + curAbsIdxInLCU);

    return ((cULeft ? cULeft->getQP(lPartIdx) : getLastCodedQP(curAbsIdxInLCU)) + (cUAbove ? cUAbove->getQP(aPartIdx) : getLastCodedQP(curAbsIdxInLCU)) + 1) >> 1;
}

int TComDataCU::getLastValidPartIdx(int absPartIdx)
{
    int lastValidPartIdx = absPartIdx - 1;

    while (lastValidPartIdx >= 0 && getPredictionMode(lastValidPartIdx) == MODE_NONE)
    {
        UInt depth = getDepth(lastValidPartIdx);
        lastValidPartIdx -= m_numPartitions >> (depth << 1);
    }

    return lastValidPartIdx;
}

char TComDataCU::getLastCodedQP(UInt absPartIdx)
{
    UInt quPartIdxMask = ~((1 << ((g_maxCUDepth - getSlice()->getPPS()->getMaxCuDQPDepth()) << 1)) - 1);
    int lastValidPartIdx = getLastValidPartIdx(absPartIdx & quPartIdxMask);

    if (lastValidPartIdx >= 0)
    {
        return getQP(lastValidPartIdx);
    }
    else
    {
        if (getZorderIdxInCU() > 0)
        {
            return getPic()->getCU(getAddr())->getLastCodedQP(getZorderIdxInCU());
        }
        else if (getAddr() > 0 && !(getSlice()->getPPS()->getEntropyCodingSyncEnabledFlag() &&
                 getAddr() % getPic()->getFrameWidthInCU() == 0))
        {
            return getPic()->getCU(getAddr() - 1)->getLastCodedQP(getPic()->getNumPartInCU());
        }
        else
        {
            return getSlice()->getSliceQp();
        }
    }
}

/** Check whether the CU is coded in lossless coding mode
 * \param   absPartIdx
 * \returns true if the CU is coded in lossless coding mode; false if otherwise
 */
bool TComDataCU::isLosslessCoded(UInt absPartIdx)
{
    return getSlice()->getPPS()->getTransquantBypassEnableFlag() && getCUTransquantBypass(absPartIdx);
}

/** Get allowed chroma intra modes
*\param   absPartIdx
*\param   uiModeList  pointer to chroma intra modes array
*\returns
*- fill uiModeList with chroma intra modes
*/
void TComDataCU::getAllowedChromaDir(UInt absPartIdx, UInt* modeList)
{
    modeList[0] = PLANAR_IDX;
    modeList[1] = VER_IDX;
    modeList[2] = HOR_IDX;
    modeList[3] = DC_IDX;
    modeList[4] = DM_CHROMA_IDX;

    UInt lumaMode = getLumaIntraDir(absPartIdx);

    for (int i = 0; i < NUM_CHROMA_MODE - 1; i++)
    {
        if (lumaMode == modeList[i])
        {
            modeList[i] = 34; // VER+8 mode
            break;
        }
    }
}

/** Get most probable intra modes
*\param   absPartIdx
*\param   uiIntraDirPred  pointer to the array for MPM storage
*\param   piMode          it is set with MPM mode in case both MPM are equal. It is used to restrict RD search at encode side.
*\returns Number of MPM
*/
int TComDataCU::getIntraDirLumaPredictor(UInt absPartIdx, int* intraDirPred, int* modes)
{
    TComDataCU* tempCU;
    UInt        tempPartIdx;
    int         leftIntraDir, aboveIntraDir;
    int         predNum = 0;

    // Get intra direction of left PU
    tempCU = getPULeft(tempPartIdx, m_absIdxInLCU + absPartIdx);

    leftIntraDir  = tempCU ? (tempCU->isIntra(tempPartIdx) ? tempCU->getLumaIntraDir(tempPartIdx) : DC_IDX) : DC_IDX;

    // Get intra direction of above PU
    tempCU = getPUAbove(tempPartIdx, m_absIdxInLCU + absPartIdx, true, true);

    aboveIntraDir = tempCU ? (tempCU->isIntra(tempPartIdx) ? tempCU->getLumaIntraDir(tempPartIdx) : DC_IDX) : DC_IDX;

    predNum = 3;
    if (leftIntraDir == aboveIntraDir)
    {
        if (modes)
        {
            *modes = 1;
        }

        if (leftIntraDir > 1) // angular modes
        {
            intraDirPred[0] = leftIntraDir;
            intraDirPred[1] = ((leftIntraDir + 29) % 32) + 2;
            intraDirPred[2] = ((leftIntraDir - 1) % 32) + 2;
        }
        else //non-angular
        {
            intraDirPred[0] = PLANAR_IDX;
            intraDirPred[1] = DC_IDX;
            intraDirPred[2] = VER_IDX;
        }
    }
    else
    {
        if (modes)
        {
            *modes = 2;
        }
        intraDirPred[0] = leftIntraDir;
        intraDirPred[1] = aboveIntraDir;

        if (leftIntraDir && aboveIntraDir) //both modes are non-planar
        {
            intraDirPred[2] = PLANAR_IDX;
        }
        else
        {
            intraDirPred[2] =  (leftIntraDir + aboveIntraDir) < 2 ? VER_IDX : DC_IDX;
        }
    }

    return predNum;
}

UInt TComDataCU::getCtxSplitFlag(UInt absPartIdx, UInt depth)
{
    TComDataCU* tempCU;
    UInt        tempPartIdx;
    UInt        ctx;

    // Get left split flag
    tempCU = getPULeft(tempPartIdx, m_absIdxInLCU + absPartIdx);
    ctx  = (tempCU) ? ((tempCU->getDepth(tempPartIdx) > depth) ? 1 : 0) : 0;

    // Get above split flag
    tempCU = getPUAbove(tempPartIdx, m_absIdxInLCU + absPartIdx);
    ctx += (tempCU) ? ((tempCU->getDepth(tempPartIdx) > depth) ? 1 : 0) : 0;

    return ctx;
}

UInt TComDataCU::getCtxQtCbf(TextType ttype, UInt trDepth)
{
    if (ttype)
    {
        return trDepth;
    }
    else
    {
        const UInt ctx = (trDepth == 0 ? 1 : 0);
        return ctx;
    }
}

UInt TComDataCU::getQuadtreeTULog2MinSizeInCU(UInt absPartIdx)
{
    UInt log2CbSize = g_convertToBit[getWidth(absPartIdx)] + 2;
    PartSize  partSize  = getPartitionSize(absPartIdx);
    UInt quadtreeTUMaxDepth = getPredictionMode(absPartIdx) == MODE_INTRA ? m_slice->getSPS()->getQuadtreeTUMaxDepthIntra() : m_slice->getSPS()->getQuadtreeTUMaxDepthInter();
    int intraSplitFlag = (getPredictionMode(absPartIdx) == MODE_INTRA && partSize == SIZE_NxN) ? 1 : 0;
    int interSplitFlag = ((quadtreeTUMaxDepth == 1) && (getPredictionMode(absPartIdx) == MODE_INTER) && (partSize != SIZE_2Nx2N));

    UInt log2MinTUSizeInCU = 0;

    if (log2CbSize < (m_slice->getSPS()->getQuadtreeTULog2MinSize() + quadtreeTUMaxDepth - 1 + interSplitFlag + intraSplitFlag))
    {
        // when fully making use of signaled TUMaxDepth + inter/intraSplitFlag, resulting luma TB size is < QuadtreeTULog2MinSize
        log2MinTUSizeInCU = m_slice->getSPS()->getQuadtreeTULog2MinSize();
    }
    else
    {
        // when fully making use of signaled TUMaxDepth + inter/intraSplitFlag, resulting luma TB size is still >= QuadtreeTULog2MinSize
        log2MinTUSizeInCU = log2CbSize - (quadtreeTUMaxDepth - 1 + interSplitFlag + intraSplitFlag); // stop when trafoDepth == hierarchy_depth = splitFlag
        if (log2MinTUSizeInCU > m_slice->getSPS()->getQuadtreeTULog2MaxSize())
        {
            // when fully making use of signaled TUMaxDepth + inter/intraSplitFlag, resulting luma TB size is still > QuadtreeTULog2MaxSize
            log2MinTUSizeInCU = m_slice->getSPS()->getQuadtreeTULog2MaxSize();
        }
    }
    return log2MinTUSizeInCU;
}

UInt TComDataCU::getCtxSkipFlag(UInt absPartIdx)
{
    TComDataCU* tempCU;
    UInt        tempPartIdx;
    UInt        ctx = 0;

    // Get BCBP of left PU
    tempCU = getPULeft(tempPartIdx, m_absIdxInLCU + absPartIdx);
    ctx    = (tempCU) ? tempCU->isSkipped(tempPartIdx) : 0;

    // Get BCBP of above PU
    tempCU = getPUAbove(tempPartIdx, m_absIdxInLCU + absPartIdx);
    ctx   += (tempCU) ? tempCU->isSkipped(tempPartIdx) : 0;

    return ctx;
}

UInt TComDataCU::getCtxInterDir(UInt absPartIdx)
{
    return getDepth(absPartIdx);
}

void TComDataCU::setCbfSubParts(UInt cbfY, UInt cbfU, UInt cbfV, UInt absPartIdx, UInt depth)
{
    UInt curPartNum = m_pic->getNumPartInCU() >> (depth << 1);

    memset(m_cbf[0] + absPartIdx, cbfY, sizeof(UChar) * curPartNum);
    memset(m_cbf[1] + absPartIdx, cbfU, sizeof(UChar) * curPartNum);
    memset(m_cbf[2] + absPartIdx, cbfV, sizeof(UChar) * curPartNum);
}

void TComDataCU::setCbfSubParts(UInt cbf, TextType ttype, UInt absPartIdx, UInt depth)
{
    UInt curPartNum = m_pic->getNumPartInCU() >> (depth << 1);

    memset(m_cbf[g_convertTxtTypeToIdx[ttype]] + absPartIdx, cbf, sizeof(UChar) * curPartNum);
}

/** Sets a coded block flag for all sub-partitions of a partition
 * \param uiCbf The value of the coded block flag to be set
 * \param ttype
 * \param absPartIdx
 * \param partIdx
 * \param depth
 * \returns void
 */
void TComDataCU::setCbfSubParts(UInt uiCbf, TextType ttype, UInt absPartIdx, UInt partIdx, UInt depth)
{
    setSubPart<UChar>(uiCbf, m_cbf[g_convertTxtTypeToIdx[ttype]], absPartIdx, depth, partIdx);
}

void TComDataCU::setDepthSubParts(UInt depth, UInt absPartIdx)
{
    UInt curPartNum = m_pic->getNumPartInCU() >> (depth << 1);

    memset(m_depth + absPartIdx, depth, sizeof(UChar) * curPartNum);
}

bool TComDataCU::isFirstAbsZorderIdxInDepth(UInt absPartIdx, UInt depth)
{
    UInt curPartNum = m_pic->getNumPartInCU() >> (depth << 1);

    return ((m_absIdxInLCU + absPartIdx) % curPartNum) == 0;
}

void TComDataCU::setPartSizeSubParts(PartSize mode, UInt absPartIdx, UInt depth)
{
    assert(sizeof(*m_partSizes) == 1);
    memset(m_partSizes + absPartIdx, mode, m_pic->getNumPartInCU() >> (2 * depth));
}

void TComDataCU::setCUTransquantBypassSubParts(bool flag, UInt absPartIdx, UInt depth)
{
    memset(m_cuTransquantBypass + absPartIdx, flag, m_pic->getNumPartInCU() >> (2 * depth));
}

void TComDataCU::setSkipFlagSubParts(bool skip, UInt absPartIdx, UInt depth)
{
    assert(sizeof(*m_skipFlag) == 1);
    memset(m_skipFlag + absPartIdx, skip, m_pic->getNumPartInCU() >> (2 * depth));
}

void TComDataCU::setPredModeSubParts(PredMode eMode, UInt absPartIdx, UInt depth)
{
    assert(sizeof(*m_predModes) == 1);
    memset(m_predModes + absPartIdx, eMode, m_pic->getNumPartInCU() >> (2 * depth));
}

void TComDataCU::setQPSubCUs(int qp, TComDataCU* cu, UInt absPartIdx, UInt depth, bool &foundNonZeroCbf)
{
    UInt curPartNumb = m_pic->getNumPartInCU() >> (depth << 1);
    UInt curPartNumQ = curPartNumb >> 2;

    if (!foundNonZeroCbf)
    {
        if (cu->getDepth(absPartIdx) > depth)
        {
            for (UInt partUnitIdx = 0; partUnitIdx < 4; partUnitIdx++)
            {
                cu->setQPSubCUs(qp, cu, absPartIdx + partUnitIdx * curPartNumQ, depth + 1, foundNonZeroCbf);
            }
        }
        else
        {
            if (cu->getCbf(absPartIdx, TEXT_LUMA) || cu->getCbf(absPartIdx, TEXT_CHROMA_U) || cu->getCbf(absPartIdx, TEXT_CHROMA_V))
            {
                foundNonZeroCbf = true;
            }
            else
            {
                setQPSubParts(qp, absPartIdx, depth);
            }
        }
    }
}

void TComDataCU::setQPSubParts(int qp, UInt absPartIdx, UInt depth)
{
    UInt curPartNum = m_pic->getNumPartInCU() >> (depth << 1);

    for (UInt scuIdx = absPartIdx; scuIdx < absPartIdx + curPartNum; scuIdx++)
    {
        m_qp[scuIdx] = qp;
    }
}

void TComDataCU::setLumaIntraDirSubParts(UInt dir, UInt absPartIdx, UInt depth)
{
    UInt curPartNum = m_pic->getNumPartInCU() >> (depth << 1);

    memset(m_lumaIntraDir + absPartIdx, dir, sizeof(UChar) * curPartNum);
}

template<typename T>
void TComDataCU::setSubPart(T param, T* baseLCU, UInt cuAddr, UInt cuDepth, UInt puIdx)
{
    assert(sizeof(T) == 1); // Using memset() works only for types of size 1

    UInt curPartNumQ = (m_pic->getNumPartInCU() >> (2 * cuDepth)) >> 2;
    switch (m_partSizes[cuAddr])
    {
    case SIZE_2Nx2N:
        memset(baseLCU + cuAddr, param, 4 * curPartNumQ);
        break;
    case SIZE_2NxN:
        memset(baseLCU + cuAddr, param, 2 * curPartNumQ);
        break;
    case SIZE_Nx2N:
        memset(baseLCU + cuAddr, param, curPartNumQ);
        memset(baseLCU + cuAddr + 2 * curPartNumQ, param, curPartNumQ);
        break;
    case SIZE_NxN:
        memset(baseLCU + cuAddr, param, curPartNumQ);
        break;
    case SIZE_2NxnU:
        if (puIdx == 0)
        {
            memset(baseLCU + cuAddr, param, (curPartNumQ >> 1));
            memset(baseLCU + cuAddr + curPartNumQ, param, (curPartNumQ >> 1));
        }
        else if (puIdx == 1)
        {
            memset(baseLCU + cuAddr, param, (curPartNumQ >> 1));
            memset(baseLCU + cuAddr + curPartNumQ, param, ((curPartNumQ >> 1) + (curPartNumQ << 1)));
        }
        else
        {
            assert(0);
        }
        break;
    case SIZE_2NxnD:
        if (puIdx == 0)
        {
            memset(baseLCU + cuAddr, param, ((curPartNumQ << 1) + (curPartNumQ >> 1)));
            memset(baseLCU + cuAddr + (curPartNumQ << 1) + curPartNumQ, param, (curPartNumQ >> 1));
        }
        else if (puIdx == 1)
        {
            memset(baseLCU + cuAddr, param, (curPartNumQ >> 1));
            memset(baseLCU + cuAddr + curPartNumQ, param, (curPartNumQ >> 1));
        }
        else
        {
            assert(0);
        }
        break;
    case SIZE_nLx2N:
        if (puIdx == 0)
        {
            memset(baseLCU + cuAddr, param, (curPartNumQ >> 2));
            memset(baseLCU + cuAddr + (curPartNumQ >> 1), param, (curPartNumQ >> 2));
            memset(baseLCU + cuAddr + (curPartNumQ << 1), param, (curPartNumQ >> 2));
            memset(baseLCU + cuAddr + (curPartNumQ << 1) + (curPartNumQ >> 1), param, (curPartNumQ >> 2));
        }
        else if (puIdx == 1)
        {
            memset(baseLCU + cuAddr, param, (curPartNumQ >> 2));
            memset(baseLCU + cuAddr + (curPartNumQ >> 1), param, (curPartNumQ + (curPartNumQ >> 2)));
            memset(baseLCU + cuAddr + (curPartNumQ << 1), param, (curPartNumQ >> 2));
            memset(baseLCU + cuAddr + (curPartNumQ << 1) + (curPartNumQ >> 1), param, (curPartNumQ + (curPartNumQ >> 2)));
        }
        else
        {
            assert(0);
        }
        break;
    case SIZE_nRx2N:
        if (puIdx == 0)
        {
            memset(baseLCU + cuAddr, param, (curPartNumQ + (curPartNumQ >> 2)));
            memset(baseLCU + cuAddr + curPartNumQ + (curPartNumQ >> 1), param, (curPartNumQ >> 2));
            memset(baseLCU + cuAddr + (curPartNumQ << 1), param, (curPartNumQ + (curPartNumQ >> 2)));
            memset(baseLCU + cuAddr + (curPartNumQ << 1) + curPartNumQ + (curPartNumQ >> 1), param, (curPartNumQ >> 2));
        }
        else if (puIdx == 1)
        {
            memset(baseLCU + cuAddr, param, (curPartNumQ >> 2));
            memset(baseLCU + cuAddr + (curPartNumQ >> 1), param, (curPartNumQ >> 2));
            memset(baseLCU + cuAddr + (curPartNumQ << 1), param, (curPartNumQ >> 2));
            memset(baseLCU + cuAddr + (curPartNumQ << 1) + (curPartNumQ >> 1), param, (curPartNumQ >> 2));
        }
        else
        {
            assert(0);
        }
        break;
    default:
        assert(0);
    }
}

void TComDataCU::setMergeFlagSubParts(bool bMergeFlag, UInt absPartIdx, UInt partIdx, UInt depth)
{
    setSubPart(bMergeFlag, m_bMergeFlags, absPartIdx, depth, partIdx);
}

void TComDataCU::setMergeIndexSubParts(UInt mergeIndex, UInt absPartIdx, UInt partIdx, UInt depth)
{
    setSubPart<UChar>(mergeIndex, m_mergeIndex, absPartIdx, depth, partIdx);
}

void TComDataCU::setChromIntraDirSubParts(UInt dir, UInt absPartIdx, UInt depth)
{
    UInt curPartNum = m_pic->getNumPartInCU() >> (depth << 1);

    memset(m_chromaIntraDir + absPartIdx, dir, sizeof(UChar) * curPartNum);
}

void TComDataCU::setInterDirSubParts(UInt dir, UInt absPartIdx, UInt partIdx, UInt depth)
{
    setSubPart<UChar>(dir, m_interDir, absPartIdx, depth, partIdx);
}

void TComDataCU::setMVPIdxSubParts(int mvpIdx, RefPicList picList, UInt absPartIdx, UInt partIdx, UInt depth)
{
    setSubPart<char>(mvpIdx, m_mvpIdx[picList], absPartIdx, depth, partIdx);
}

void TComDataCU::setMVPNumSubParts(int iMVPNum, RefPicList picList, UInt absPartIdx, UInt partIdx, UInt depth)
{
    setSubPart<char>(iMVPNum, m_mvpNum[picList], absPartIdx, depth, partIdx);
}

void TComDataCU::setTrIdxSubParts(UInt uiTrIdx, UInt absPartIdx, UInt depth)
{
    UInt curPartNum = m_pic->getNumPartInCU() >> (depth << 1);

    memset(m_trIdx + absPartIdx, uiTrIdx, sizeof(UChar) * curPartNum);
}

void TComDataCU::setTransformSkipSubParts(UInt useTransformSkipY, UInt useTransformSkipU, UInt useTransformSkipV, UInt absPartIdx, UInt depth)
{
    UInt curPartNum = m_pic->getNumPartInCU() >> (depth << 1);

    memset(m_transformSkip[0] + absPartIdx, useTransformSkipY, sizeof(UChar) * curPartNum);
    memset(m_transformSkip[1] + absPartIdx, useTransformSkipU, sizeof(UChar) * curPartNum);
    memset(m_transformSkip[2] + absPartIdx, useTransformSkipV, sizeof(UChar) * curPartNum);
}

void TComDataCU::setTransformSkipSubParts(UInt useTransformSkip, TextType ttype, UInt absPartIdx, UInt depth)
{
    UInt curPartNum = m_pic->getNumPartInCU() >> (depth << 1);

    memset(m_transformSkip[g_convertTxtTypeToIdx[ttype]] + absPartIdx, useTransformSkip, sizeof(UChar) * curPartNum);
}

void TComDataCU::setSizeSubParts(UInt width, UInt height, UInt absPartIdx, UInt depth)
{
    UInt curPartNum = m_pic->getNumPartInCU() >> (depth << 1);

    memset(m_width  + absPartIdx, width,  sizeof(UChar) * curPartNum);
    memset(m_height + absPartIdx, height, sizeof(UChar) * curPartNum);
}

UChar TComDataCU::getNumPartInter()
{
    UChar numPart = 0;

    switch (m_partSizes[0])
    {
    case SIZE_2Nx2N:    numPart = 1;
        break;
    case SIZE_2NxN:     numPart = 2;
        break;
    case SIZE_Nx2N:     numPart = 2;
        break;
    case SIZE_NxN:      numPart = 4;
        break;
    case SIZE_2NxnU:    numPart = 2;
        break;
    case SIZE_2NxnD:    numPart = 2;
        break;
    case SIZE_nLx2N:    numPart = 2;
        break;
    case SIZE_nRx2N:    numPart = 2;
        break;
    default:            assert(0);
        break;
    }

    return numPart;
}

void TComDataCU::getPartIndexAndSize(UInt partIdx, UInt& outPartAddr, int& outWidth, int& outHeight)
{
    switch (m_partSizes[0])
    {
    case SIZE_2NxN:
        outWidth = getWidth(0);
        outHeight = getHeight(0) >> 1;
        outPartAddr = (partIdx == 0) ? 0 : m_numPartitions >> 1;
        break;
    case SIZE_Nx2N:
        outWidth = getWidth(0) >> 1;
        outHeight = getHeight(0);
        outPartAddr = (partIdx == 0) ? 0 : m_numPartitions >> 2;
        break;
    case SIZE_NxN:
        outWidth = getWidth(0) >> 1;
        outHeight = getHeight(0) >> 1;
        outPartAddr = (m_numPartitions >> 2) * partIdx;
        break;
    case SIZE_2NxnU:
        outWidth     = getWidth(0);
        outHeight    = (partIdx == 0) ?  getHeight(0) >> 2 : (getHeight(0) >> 2) + (getHeight(0) >> 1);
        outPartAddr = (partIdx == 0) ? 0 : m_numPartitions >> 3;
        break;
    case SIZE_2NxnD:
        outWidth     = getWidth(0);
        outHeight    = (partIdx == 0) ?  (getHeight(0) >> 2) + (getHeight(0) >> 1) : getHeight(0) >> 2;
        outPartAddr = (partIdx == 0) ? 0 : (m_numPartitions >> 1) + (m_numPartitions >> 3);
        break;
    case SIZE_nLx2N:
        outWidth     = (partIdx == 0) ? getWidth(0) >> 2 : (getWidth(0) >> 2) + (getWidth(0) >> 1);
        outHeight    = getHeight(0);
        outPartAddr = (partIdx == 0) ? 0 : m_numPartitions >> 4;
        break;
    case SIZE_nRx2N:
        outWidth     = (partIdx == 0) ? (getWidth(0) >> 2) + (getWidth(0) >> 1) : getWidth(0) >> 2;
        outHeight    = getHeight(0);
        outPartAddr = (partIdx == 0) ? 0 : (m_numPartitions >> 2) + (m_numPartitions >> 4);
        break;
    default:
        assert(m_partSizes[0] == SIZE_2Nx2N);
        outWidth = getWidth(0);
        outHeight = getHeight(0);
        outPartAddr = 0;
        break;
    }
}

void TComDataCU::getMvField(TComDataCU* cu, UInt absPartIdx, RefPicList picList, TComMvField& outMvField)
{
    if (cu == NULL)  // OUT OF BOUNDARY
    {
        MV mv(0, 0);
        outMvField.setMvField(mv, NOT_VALID);
        return;
    }

    TComCUMvField* cuMvField = cu->getCUMvField(picList);
    outMvField.setMvField(cuMvField->getMv(absPartIdx), cuMvField->getRefIdx(absPartIdx));
}

void TComDataCU::deriveLeftRightTopIdxGeneral(UInt absPartIdx, UInt partIdx, UInt& outPartIdxLT, UInt& outPartIdxRT)
{
    outPartIdxLT = m_absIdxInLCU + absPartIdx;
    UInt puWidth = 0;

    switch (m_partSizes[absPartIdx])
    {
    case SIZE_2Nx2N: puWidth = m_width[absPartIdx];
        break;
    case SIZE_2NxN:  puWidth = m_width[absPartIdx];
        break;
    case SIZE_Nx2N:  puWidth = m_width[absPartIdx]  >> 1;
        break;
    case SIZE_NxN:   puWidth = m_width[absPartIdx]  >> 1;
        break;
    case SIZE_2NxnU: puWidth = m_width[absPartIdx];
        break;
    case SIZE_2NxnD: puWidth = m_width[absPartIdx];
        break;
    case SIZE_nLx2N:
        if (partIdx == 0)
        {
            puWidth = m_width[absPartIdx]  >> 2;
        }
        else if (partIdx == 1)
        {
            puWidth = (m_width[absPartIdx]  >> 1) + (m_width[absPartIdx]  >> 2);
        }
        else
        {
            assert(0);
        }
        break;
    case SIZE_nRx2N:
        if (partIdx == 0)
        {
            puWidth = (m_width[absPartIdx]  >> 1) + (m_width[absPartIdx]  >> 2);
        }
        else if (partIdx == 1)
        {
            puWidth = m_width[absPartIdx]  >> 2;
        }
        else
        {
            assert(0);
        }
        break;
    default:
        assert(0);
        break;
    }

    outPartIdxRT = g_rasterToZscan[g_zscanToRaster[outPartIdxLT] + puWidth / m_pic->getMinCUWidth() - 1];
}

void TComDataCU::deriveLeftBottomIdxGeneral(UInt absPartIdx, UInt partIdx, UInt& outPartIdxLB)
{
    UInt puHeight = 0;

    switch (m_partSizes[absPartIdx])
    {
    case SIZE_2Nx2N: puHeight = m_height[absPartIdx];
        break;
    case SIZE_2NxN:  puHeight = m_height[absPartIdx] >> 1;
        break;
    case SIZE_Nx2N:  puHeight = m_height[absPartIdx];
        break;
    case SIZE_NxN:   puHeight = m_height[absPartIdx] >> 1;
        break;
    case SIZE_2NxnU:
        if (partIdx == 0)
        {
            puHeight = m_height[absPartIdx] >> 2;
        }
        else if (partIdx == 1)
        {
            puHeight = (m_height[absPartIdx] >> 1) + (m_height[absPartIdx] >> 2);
        }
        else
        {
            assert(0);
        }
        break;
    case SIZE_2NxnD:
        if (partIdx == 0)
        {
            puHeight = (m_height[absPartIdx] >> 1) + (m_height[absPartIdx] >> 2);
        }
        else if (partIdx == 1)
        {
            puHeight = m_height[absPartIdx] >> 2;
        }
        else
        {
            assert(0);
        }
        break;
    case SIZE_nLx2N: puHeight = m_height[absPartIdx];
        break;
    case SIZE_nRx2N: puHeight = m_height[absPartIdx];
        break;
    default:
        assert(0);
        break;
    }

    outPartIdxLB = g_rasterToZscan[g_zscanToRaster[m_absIdxInLCU + absPartIdx] + ((puHeight / m_pic->getMinCUHeight()) - 1) * m_pic->getNumPartInWidth()];
}

void TComDataCU::deriveLeftRightTopIdx(UInt partIdx, UInt& ruiPartIdxLT, UInt& ruiPartIdxRT)
{
    ruiPartIdxLT = m_absIdxInLCU;
    ruiPartIdxRT = g_rasterToZscan[g_zscanToRaster[ruiPartIdxLT] + m_width[0] / m_pic->getMinCUWidth() - 1];

    switch (m_partSizes[0])
    {
    case SIZE_2Nx2N: break;
    case SIZE_2NxN:
        ruiPartIdxLT += (partIdx == 0) ? 0 : m_numPartitions >> 1;
        ruiPartIdxRT += (partIdx == 0) ? 0 : m_numPartitions >> 1;
        break;
    case SIZE_Nx2N:
        ruiPartIdxLT += (partIdx == 0) ? 0 : m_numPartitions >> 2;
        ruiPartIdxRT -= (partIdx == 1) ? 0 : m_numPartitions >> 2;
        break;
    case SIZE_NxN:
        ruiPartIdxLT += (m_numPartitions >> 2) * partIdx;
        ruiPartIdxRT +=  (m_numPartitions >> 2) * (partIdx - 1);
        break;
    case SIZE_2NxnU:
        ruiPartIdxLT += (partIdx == 0) ? 0 : m_numPartitions >> 3;
        ruiPartIdxRT += (partIdx == 0) ? 0 : m_numPartitions >> 3;
        break;
    case SIZE_2NxnD:
        ruiPartIdxLT += (partIdx == 0) ? 0 : (m_numPartitions >> 1) + (m_numPartitions >> 3);
        ruiPartIdxRT += (partIdx == 0) ? 0 : (m_numPartitions >> 1) + (m_numPartitions >> 3);
        break;
    case SIZE_nLx2N:
        ruiPartIdxLT += (partIdx == 0) ? 0 : m_numPartitions >> 4;
        ruiPartIdxRT -= (partIdx == 1) ? 0 : (m_numPartitions >> 2) + (m_numPartitions >> 4);
        break;
    case SIZE_nRx2N:
        ruiPartIdxLT += (partIdx == 0) ? 0 : (m_numPartitions >> 2) + (m_numPartitions >> 4);
        ruiPartIdxRT -= (partIdx == 1) ? 0 : m_numPartitions >> 4;
        break;
    default:
        assert(0);
        break;
    }
}

void TComDataCU::deriveLeftBottomIdx(UInt partIdx, UInt& outPartIdxLB)
{
    outPartIdxLB = g_rasterToZscan[g_zscanToRaster[m_absIdxInLCU] + (((m_height[0] / m_pic->getMinCUHeight()) >> 1) - 1) * m_pic->getNumPartInWidth()];

    switch (m_partSizes[0])
    {
    case SIZE_2Nx2N:
        outPartIdxLB += m_numPartitions >> 1;
        break;
    case SIZE_2NxN:
        outPartIdxLB += (partIdx == 0) ? 0 : m_numPartitions >> 1;
        break;
    case SIZE_Nx2N:
        outPartIdxLB += (partIdx == 0) ? m_numPartitions >> 1 : (m_numPartitions >> 2) * 3;
        break;
    case SIZE_NxN:
        outPartIdxLB += (m_numPartitions >> 2) * partIdx;
        break;
    case SIZE_2NxnU:
        outPartIdxLB += (partIdx == 0) ? -((int)m_numPartitions >> 3) : m_numPartitions >> 1;
        break;
    case SIZE_2NxnD:
        outPartIdxLB += (partIdx == 0) ? (m_numPartitions >> 2) + (m_numPartitions >> 3) : m_numPartitions >> 1;
        break;
    case SIZE_nLx2N:
        outPartIdxLB += (partIdx == 0) ? m_numPartitions >> 1 : (m_numPartitions >> 1) + (m_numPartitions >> 4);
        break;
    case SIZE_nRx2N:
        outPartIdxLB += (partIdx == 0) ? m_numPartitions >> 1 : (m_numPartitions >> 1) + (m_numPartitions >> 2) + (m_numPartitions >> 4);
        break;
    default:
        assert(0);
        break;
    }
}

/** Derives the partition index of neighboring bottom right block
 * \param [in]  cuMode
 * \param [in]  partIdx
 * \param [out] outPartIdxRB
 */
void TComDataCU::deriveRightBottomIdx(UInt partIdx, UInt& outPartIdxRB)
{
    outPartIdxRB = g_rasterToZscan[g_zscanToRaster[m_absIdxInLCU] + (((m_height[0] / m_pic->getMinCUHeight()) >> 1) - 1) *
                   m_pic->getNumPartInWidth() +  m_width[0] / m_pic->getMinCUWidth() - 1];

    switch (m_partSizes[0])
    {
    case SIZE_2Nx2N:
        outPartIdxRB += m_numPartitions >> 1;
        break;
    case SIZE_2NxN:
        outPartIdxRB += (partIdx == 0) ? 0 : m_numPartitions >> 1;
        break;
    case SIZE_Nx2N:
        outPartIdxRB += (partIdx == 0) ? m_numPartitions >> 2 : (m_numPartitions >> 1);
        break;
    case SIZE_NxN:
        outPartIdxRB += (m_numPartitions >> 2) * (partIdx - 1);
        break;
    case SIZE_2NxnU:
        outPartIdxRB += (partIdx == 0) ? -((int)m_numPartitions >> 3) : m_numPartitions >> 1;
        break;
    case SIZE_2NxnD:
        outPartIdxRB += (partIdx == 0) ? (m_numPartitions >> 2) + (m_numPartitions >> 3) : m_numPartitions >> 1;
        break;
    case SIZE_nLx2N:
        outPartIdxRB += (partIdx == 0) ? (m_numPartitions >> 3) + (m_numPartitions >> 4) : m_numPartitions >> 1;
        break;
    case SIZE_nRx2N:
        outPartIdxRB += (partIdx == 0) ? (m_numPartitions >> 2) + (m_numPartitions >> 3) + (m_numPartitions >> 4) : m_numPartitions >> 1;
        break;
    default:
        assert(0);
        break;
    }
}

void TComDataCU::deriveLeftRightTopIdxAdi(UInt& outPartIdxLT, UInt& outPartIdxRT, UInt partOffset, UInt partDepth)
{
    UInt numPartInWidth = (m_width[0] / m_pic->getMinCUWidth()) >> partDepth;

    outPartIdxLT = m_absIdxInLCU + partOffset;
    outPartIdxRT = g_rasterToZscan[g_zscanToRaster[outPartIdxLT] + numPartInWidth - 1];
}

void TComDataCU::deriveLeftBottomIdxAdi(UInt& outPartIdxLB, UInt partOffset, UInt partDepth)
{
    UInt absIdx;
    UInt minCUWidth, widthInMinCUs;

    minCUWidth    = getPic()->getMinCUWidth();
    widthInMinCUs = (getWidth(0) / minCUWidth) >> partDepth;
    absIdx        = getZorderIdxInCU() + partOffset + (m_numPartitions >> (partDepth << 1)) - 1;
    absIdx        = g_zscanToRaster[absIdx] - (widthInMinCUs - 1);
    outPartIdxLB    = g_rasterToZscan[absIdx];
}

bool TComDataCU::hasEqualMotion(UInt absPartIdx, TComDataCU* candCU, UInt candAbsPartIdx)
{
    if (getInterDir(absPartIdx) != candCU->getInterDir(candAbsPartIdx))
    {
        return false;
    }

    for (UInt refListIdx = 0; refListIdx < 2; refListIdx++)
    {
        if (getInterDir(absPartIdx) & (1 << refListIdx))
        {
            if (getCUMvField(RefPicList(refListIdx))->getMv(absPartIdx)     != candCU->getCUMvField(RefPicList(refListIdx))->getMv(candAbsPartIdx) ||
                getCUMvField(RefPicList(refListIdx))->getRefIdx(absPartIdx) != candCU->getCUMvField(RefPicList(refListIdx))->getRefIdx(candAbsPartIdx))
            {
                return false;
            }
        }
    }

    return true;
}

/** Constructs a list of merging candidates
 * \param absPartIdx
 * \param puIdx
 * \param depth
 * \param pcMvFieldNeighbours
 * \param puhInterDirNeighbours
 * \param numValidMergeCand
 */
void TComDataCU::getInterMergeCandidates(UInt absPartIdx, UInt puIdx, TComMvField* mvFieldNeighbours, UChar* interDirNeighbours,
                                         int& numValidMergeCand, int mrgCandIdx)
{
    UInt absPartAddr = m_absIdxInLCU + absPartIdx;
    bool abCandIsInter[MRG_MAX_NUM_CANDS];

    for (UInt i = 0; i < getSlice()->getMaxNumMergeCand(); ++i)
    {
        abCandIsInter[i] = false;
        mvFieldNeighbours[(i << 1)].refIdx = NOT_VALID;
        mvFieldNeighbours[(i << 1) + 1].refIdx = NOT_VALID;
    }

    numValidMergeCand = getSlice()->getMaxNumMergeCand();
    // compute the location of the current PU
    int xP, yP, nPSW, nPSH;
    this->getPartPosition(puIdx, xP, yP, nPSW, nPSH);

    int count = 0;

    UInt partIdxLT, partIdxRT, partIdxLB;
    PartSize curPS = getPartitionSize(absPartIdx);
    deriveLeftRightTopIdxGeneral(absPartIdx, puIdx, partIdxLT, partIdxRT);
    deriveLeftBottomIdxGeneral(absPartIdx, puIdx, partIdxLB);

    //left
    UInt leftPartIdx = 0;
    TComDataCU* cuLeft = 0;
    cuLeft = getPULeft(leftPartIdx, partIdxLB);
    bool isAvailableA1 = cuLeft &&
        cuLeft->isDiffMER(xP - 1, yP + nPSH - 1, xP, yP) &&
        !(puIdx == 1 && (curPS == SIZE_Nx2N || curPS == SIZE_nLx2N || curPS == SIZE_nRx2N)) &&
        !cuLeft->isIntra(leftPartIdx);
    if (isAvailableA1)
    {
        abCandIsInter[count] = true;
        // get Inter Dir
        interDirNeighbours[count] = cuLeft->getInterDir(leftPartIdx);
        // get Mv from Left
        cuLeft->getMvField(cuLeft, leftPartIdx, REF_PIC_LIST_0, mvFieldNeighbours[count << 1]);
        if (getSlice()->isInterB())
        {
            cuLeft->getMvField(cuLeft, leftPartIdx, REF_PIC_LIST_1, mvFieldNeighbours[(count << 1) + 1]);
        }
        if (mrgCandIdx == count)
        {
            return;
        }
        count++;
    }

    // early termination
    if (count == getSlice()->getMaxNumMergeCand())
    {
        return;
    }
    // above
    UInt abovePartIdx = 0;
    TComDataCU* cuAbove = 0;
    cuAbove = getPUAbove(abovePartIdx, partIdxRT);
    bool isAvailableB1 = cuAbove &&
        cuAbove->isDiffMER(xP + nPSW - 1, yP - 1, xP, yP) &&
        !(puIdx == 1 && (curPS == SIZE_2NxN || curPS == SIZE_2NxnU || curPS == SIZE_2NxnD)) &&
        !cuAbove->isIntra(abovePartIdx);
    if (isAvailableB1 && (!isAvailableA1 || !cuLeft->hasEqualMotion(leftPartIdx, cuAbove, abovePartIdx)))
    {
        abCandIsInter[count] = true;
        // get Inter Dir
        interDirNeighbours[count] = cuAbove->getInterDir(abovePartIdx);
        // get Mv from Left
        cuAbove->getMvField(cuAbove, abovePartIdx, REF_PIC_LIST_0, mvFieldNeighbours[count << 1]);
        if (getSlice()->isInterB())
        {
            cuAbove->getMvField(cuAbove, abovePartIdx, REF_PIC_LIST_1, mvFieldNeighbours[(count << 1) + 1]);
        }
        if (mrgCandIdx == count)
        {
            return;
        }
        count++;
    }
    // early termination
    if (count == getSlice()->getMaxNumMergeCand())
    {
        return;
    }

    // above right
    UInt aboveRightPartIdx = 0;
    TComDataCU* cuAboveRight = 0;
    cuAboveRight = getPUAboveRight(aboveRightPartIdx, partIdxRT);
    bool isAvailableB0 = cuAboveRight &&
        cuAboveRight->isDiffMER(xP + nPSW, yP - 1, xP, yP) &&
        !cuAboveRight->isIntra(aboveRightPartIdx);
    if (isAvailableB0 && (!isAvailableB1 || !cuAbove->hasEqualMotion(abovePartIdx, cuAboveRight, aboveRightPartIdx)))
    {
        abCandIsInter[count] = true;
        // get Inter Dir
        interDirNeighbours[count] = cuAboveRight->getInterDir(aboveRightPartIdx);
        // get Mv from Left
        cuAboveRight->getMvField(cuAboveRight, aboveRightPartIdx, REF_PIC_LIST_0, mvFieldNeighbours[count << 1]);
        if (getSlice()->isInterB())
        {
            cuAboveRight->getMvField(cuAboveRight, aboveRightPartIdx, REF_PIC_LIST_1, mvFieldNeighbours[(count << 1) + 1]);
        }
        if (mrgCandIdx == count)
        {
            return;
        }
        count++;
    }
    // early termination
    if (count == getSlice()->getMaxNumMergeCand())
    {
        return;
    }

    //left bottom
    UInt leftBottomPartIdx = 0;
    TComDataCU* cuLeftBottom = 0;
    cuLeftBottom = this->getPUBelowLeft(leftBottomPartIdx, partIdxLB);
    bool isAvailableA0 = cuLeftBottom &&
        cuLeftBottom->isDiffMER(xP - 1, yP + nPSH, xP, yP) &&
        !cuLeftBottom->isIntra(leftBottomPartIdx);
    if (isAvailableA0 && (!isAvailableA1 || !cuLeft->hasEqualMotion(leftPartIdx, cuLeftBottom, leftBottomPartIdx)))
    {
        abCandIsInter[count] = true;
        // get Inter Dir
        interDirNeighbours[count] = cuLeftBottom->getInterDir(leftBottomPartIdx);
        // get Mv from Left
        cuLeftBottom->getMvField(cuLeftBottom, leftBottomPartIdx, REF_PIC_LIST_0, mvFieldNeighbours[count << 1]);
        if (getSlice()->isInterB())
        {
            cuLeftBottom->getMvField(cuLeftBottom, leftBottomPartIdx, REF_PIC_LIST_1, mvFieldNeighbours[(count << 1) + 1]);
        }
        if (mrgCandIdx == count)
        {
            return;
        }
        count++;
    }
    // early termination
    if (count == getSlice()->getMaxNumMergeCand())
    {
        return;
    }
    // above left
    if (count < 4)
    {
        UInt aboveLeftPartIdx = 0;
        TComDataCU* cuAboveLeft = 0;
        cuAboveLeft = getPUAboveLeft(aboveLeftPartIdx, absPartAddr);
        bool isAvailableB2 = cuAboveLeft &&
            cuAboveLeft->isDiffMER(xP - 1, yP - 1, xP, yP) &&
            !cuAboveLeft->isIntra(aboveLeftPartIdx);
        if (isAvailableB2 && (!isAvailableA1 || !cuLeft->hasEqualMotion(leftPartIdx, cuAboveLeft, aboveLeftPartIdx))
            && (!isAvailableB1 || !cuAbove->hasEqualMotion(abovePartIdx, cuAboveLeft, aboveLeftPartIdx)))
        {
            abCandIsInter[count] = true;
            // get Inter Dir
            interDirNeighbours[count] = cuAboveLeft->getInterDir(aboveLeftPartIdx);
            // get Mv from Left
            cuAboveLeft->getMvField(cuAboveLeft, aboveLeftPartIdx, REF_PIC_LIST_0, mvFieldNeighbours[count << 1]);
            if (getSlice()->isInterB())
            {
                cuAboveLeft->getMvField(cuAboveLeft, aboveLeftPartIdx, REF_PIC_LIST_1, mvFieldNeighbours[(count << 1) + 1]);
            }
            if (mrgCandIdx == count)
            {
                return;
            }
            count++;
        }
    }
    // early termination
    if (count == getSlice()->getMaxNumMergeCand())
    {
        return;
    }
    if (getSlice()->getEnableTMVPFlag())
    {
        //>> MTK colocated-RightBottom
        UInt partIdxRB;
        int lcuIdx = getAddr();

        deriveRightBottomIdx(puIdx, partIdxRB);

        UInt uiAbsPartIdxTmp = g_zscanToRaster[partIdxRB];
        UInt numPartInCUWidth = m_pic->getNumPartInWidth();

        MV colmv;
        int refIdx;

        if ((m_pic->getCU(m_cuAddr)->getCUPelX() + g_rasterToPelX[uiAbsPartIdxTmp] + m_pic->getMinCUWidth()) >= m_slice->getSPS()->getPicWidthInLumaSamples())  // image boundary check
        {
            lcuIdx = -1;
        }
        else if ((m_pic->getCU(m_cuAddr)->getCUPelY() + g_rasterToPelY[uiAbsPartIdxTmp] + m_pic->getMinCUHeight()) >= m_slice->getSPS()->getPicHeightInLumaSamples())
        {
            lcuIdx = -1;
        }
        else
        {
            if ((uiAbsPartIdxTmp % numPartInCUWidth < numPartInCUWidth - 1) &&        // is not at the last column of LCU
                (uiAbsPartIdxTmp / numPartInCUWidth < m_pic->getNumPartInHeight() - 1)) // is not at the last row    of LCU
            {
                absPartAddr = g_rasterToZscan[uiAbsPartIdxTmp + numPartInCUWidth + 1];
                lcuIdx = getAddr();
            }
            else if (uiAbsPartIdxTmp % numPartInCUWidth < numPartInCUWidth - 1)       // is not at the last column of LCU But is last row of LCU
            {
                absPartAddr = g_rasterToZscan[(uiAbsPartIdxTmp + numPartInCUWidth + 1) % m_pic->getNumPartInCU()];
                lcuIdx = -1;
            }
            else if (uiAbsPartIdxTmp / numPartInCUWidth < m_pic->getNumPartInHeight() - 1) // is not at the last row of LCU But is last column of LCU
            {
                absPartAddr = g_rasterToZscan[uiAbsPartIdxTmp + 1];
                lcuIdx = getAddr() + 1;
            }
            else //is the right bottom corner of LCU
            {
                absPartAddr = 0;
                lcuIdx = -1;
            }
        }

        refIdx = 0;
        bool bExistMV = false;
        UInt partIdxCenter;
        UInt curLCUIdx = getAddr();
        int dir = 0;
        UInt arrayAddr = count;
        xDeriveCenterIdx(puIdx, partIdxCenter);
        bExistMV = lcuIdx >= 0 && xGetColMVP(REF_PIC_LIST_0, lcuIdx, absPartAddr, colmv, refIdx);
        if (bExistMV == false)
        {
            bExistMV = xGetColMVP(REF_PIC_LIST_0, curLCUIdx, partIdxCenter, colmv, refIdx);
        }
        if (bExistMV)
        {
            dir |= 1;
            mvFieldNeighbours[2 * arrayAddr].setMvField(colmv, refIdx);
        }

        if (getSlice()->isInterB())
        {
            bExistMV = lcuIdx >= 0 && xGetColMVP(REF_PIC_LIST_1, lcuIdx, absPartAddr, colmv, refIdx);
            if (bExistMV == false)
            {
                bExistMV = xGetColMVP(REF_PIC_LIST_1, curLCUIdx, partIdxCenter, colmv, refIdx);
            }
            if (bExistMV)
            {
                dir |= 2;
                mvFieldNeighbours[2 * arrayAddr + 1].setMvField(colmv, refIdx);
            }
        }

        if (dir != 0)
        {
            interDirNeighbours[arrayAddr] = dir;
            abCandIsInter[arrayAddr] = true;

            if (mrgCandIdx == count)
            {
                return;
            }
            count++;
        }
    }
    // early termination
    if (count == getSlice()->getMaxNumMergeCand())
    {
        return;
    }
    UInt arrayAddr = count;
    UInt cutoff = arrayAddr;

    if (getSlice()->isInterB())
    {
        // TODO: TComRom??
        UInt priorityList0[12] = { 0, 1, 0, 2, 1, 2, 0, 3, 1, 3, 2, 3 };
        UInt priorityList1[12] = { 1, 0, 2, 0, 2, 1, 3, 0, 3, 1, 3, 2 };

        for (int idx = 0; idx < cutoff * (cutoff - 1) && arrayAddr != getSlice()->getMaxNumMergeCand(); idx++)
        {
            int i = priorityList0[idx];
            int j = priorityList1[idx];
            if (abCandIsInter[i] && abCandIsInter[j] && (interDirNeighbours[i] & 0x1) && (interDirNeighbours[j] & 0x2))
            {
                abCandIsInter[arrayAddr] = true;
                interDirNeighbours[arrayAddr] = 3;

                // get Mv from cand[i] and cand[j]
                mvFieldNeighbours[arrayAddr << 1].setMvField(mvFieldNeighbours[i << 1].mv, mvFieldNeighbours[i << 1].refIdx);
                mvFieldNeighbours[(arrayAddr << 1) + 1].setMvField(mvFieldNeighbours[(j << 1) + 1].mv, mvFieldNeighbours[(j << 1) + 1].refIdx);

                int refPOCL0 = m_slice->getRefPOC(REF_PIC_LIST_0, mvFieldNeighbours[(arrayAddr << 1)].refIdx);
                int refPOCL1 = m_slice->getRefPOC(REF_PIC_LIST_1, mvFieldNeighbours[(arrayAddr << 1) + 1].refIdx);
                if (refPOCL0 == refPOCL1 && mvFieldNeighbours[(arrayAddr << 1)].mv == mvFieldNeighbours[(arrayAddr << 1) + 1].mv)
                {
                    abCandIsInter[arrayAddr] = false;
                }
                else
                {
                    arrayAddr++;
                }
            }
        }
    }
    // early termination
    if (arrayAddr == getSlice()->getMaxNumMergeCand())
    {
        return;
    }
    int numRefIdx = (getSlice()->isInterB()) ? X265_MIN(m_slice->getNumRefIdx(REF_PIC_LIST_0), m_slice->getNumRefIdx(REF_PIC_LIST_1)) : m_slice->getNumRefIdx(REF_PIC_LIST_0);
    int r = 0;
    int refcnt = 0;
    while (arrayAddr < getSlice()->getMaxNumMergeCand())
    {
        abCandIsInter[arrayAddr] = true;
        interDirNeighbours[arrayAddr] = 1;
        mvFieldNeighbours[arrayAddr << 1].setMvField(MV(0, 0), r);

        if (getSlice()->isInterB())
        {
            interDirNeighbours[arrayAddr] = 3;
            mvFieldNeighbours[(arrayAddr << 1) + 1].setMvField(MV(0, 0), r);
        }
        arrayAddr++;
        if (refcnt == numRefIdx - 1)
        {
            r = 0;
        }
        else
        {
            ++r;
            ++refcnt;
        }
    }

    numValidMergeCand = arrayAddr;
}

/** Check whether the current PU and a spatial neighboring PU are in a same ME region.
 * \param xN, xN   location of the upper-left corner pixel of a neighboring PU
 * \param xP, yP   location of the upper-left corner pixel of the current PU
 * \returns bool
 */
bool TComDataCU::isDiffMER(int xN, int yN, int xP, int yP)
{
    UInt plevel = this->getSlice()->getPPS()->getLog2ParallelMergeLevelMinus2() + 2;

    if ((xN >> plevel) != (xP >> plevel))
    {
        return true;
    }
    if ((yN >> plevel) != (yP >> plevel))
    {
        return true;
    }
    return false;
}

/** calculate the location of upper-left corner pixel and size of the current PU.
 * \param partIdx  PU index within a CU
 * \param xP, yP   location of the upper-left corner pixel of the current PU
 * \param PSW, nPSH    size of the curren PU
 * \returns void
 */
void TComDataCU::getPartPosition(UInt partIdx, int& xP, int& yP, int& nPSW, int& nPSH)
{
    UInt col = m_cuPelX;
    UInt row = m_cuPelY;

    switch (m_partSizes[0])
    {
    case SIZE_2NxN:
        nPSW = getWidth(0);
        nPSH = getHeight(0) >> 1;
        xP   = col;
        yP   = (partIdx == 0) ? row : row + nPSH;
        break;
    case SIZE_Nx2N:
        nPSW = getWidth(0) >> 1;
        nPSH = getHeight(0);
        xP   = (partIdx == 0) ? col : col + nPSW;
        yP   = row;
        break;
    case SIZE_NxN:
        nPSW = getWidth(0) >> 1;
        nPSH = getHeight(0) >> 1;
        xP   = col + (partIdx & 0x1) * nPSW;
        yP   = row + (partIdx >> 1) * nPSH;
        break;
    case SIZE_2NxnU:
        nPSW = getWidth(0);
        nPSH = (partIdx == 0) ?  getHeight(0) >> 2 : (getHeight(0) >> 2) + (getHeight(0) >> 1);
        xP   = col;
        yP   = (partIdx == 0) ? row : row + getHeight(0) - nPSH;
        break;
    case SIZE_2NxnD:
        nPSW = getWidth(0);
        nPSH = (partIdx == 0) ?  (getHeight(0) >> 2) + (getHeight(0) >> 1) : getHeight(0) >> 2;
        xP   = col;
        yP   = (partIdx == 0) ? row : row + getHeight(0) - nPSH;
        break;
    case SIZE_nLx2N:
        nPSW = (partIdx == 0) ? getWidth(0) >> 2 : (getWidth(0) >> 2) + (getWidth(0) >> 1);
        nPSH = getHeight(0);
        xP   = (partIdx == 0) ? col : col + getWidth(0) - nPSW;
        yP   = row;
        break;
    case SIZE_nRx2N:
        nPSW = (partIdx == 0) ? (getWidth(0) >> 2) + (getWidth(0) >> 1) : getWidth(0) >> 2;
        nPSH = getHeight(0);
        xP   = (partIdx == 0) ? col : col + getWidth(0) - nPSW;
        yP   = row;
        break;
    default:
        assert(m_partSizes[0] == SIZE_2Nx2N);
        nPSW = getWidth(0);
        nPSH = getHeight(0);
        xP   = col;
        yP   = row;

        break;
    }
}

/** Constructs a list of candidates for AMVP
 * \param partIdx
 * \param partAddr
 * \param picList
 * \param refIdx
 * \param info
 */
void TComDataCU::fillMvpCand(UInt partIdx, UInt partAddr, RefPicList picList, int refIdx, AMVPInfo* info)
{
    MV mvp;
    bool bAddedSmvp = false;

    info->m_num = 0;
    if (refIdx < 0)
    {
        return;
    }

    //-- Get Spatial MV
    UInt partIdxLT, partIdxRT, partIdxLB;
    UInt numPartInCUWidth = m_pic->getNumPartInWidth();
    bool bAdded = false;

    deriveLeftRightTopIdx(partIdx, partIdxLT, partIdxRT);
    deriveLeftBottomIdx(partIdx, partIdxLB);

    TComDataCU* tmpCU = NULL;
    UInt idx;
    tmpCU = getPUBelowLeft(idx, partIdxLB);
    bAddedSmvp = (tmpCU != NULL) && (tmpCU->getPredictionMode(idx) != MODE_INTRA);

    if (!bAddedSmvp)
    {
        tmpCU = getPULeft(idx, partIdxLB);
        bAddedSmvp = (tmpCU != NULL) && (tmpCU->getPredictionMode(idx) != MODE_INTRA);
    }

    // Left predictor search
    bAdded = xAddMVPCand(info, picList, refIdx, partIdxLB, MD_BELOW_LEFT);
    if (!bAdded)
    {
        bAdded = xAddMVPCand(info, picList, refIdx, partIdxLB, MD_LEFT);
    }

    if (!bAdded)
    {
        bAdded = xAddMVPCandOrder(info, picList, refIdx, partIdxLB, MD_BELOW_LEFT);
        if (!bAdded)
        {
            bAdded = xAddMVPCandOrder(info, picList, refIdx, partIdxLB, MD_LEFT);
        }
    }
    // Above predictor search
    bAdded = xAddMVPCand(info, picList, refIdx, partIdxRT, MD_ABOVE_RIGHT);

    if (!bAdded)
    {
        bAdded = xAddMVPCand(info, picList, refIdx, partIdxRT, MD_ABOVE);
    }

    if (!bAdded)
    {
        bAdded = xAddMVPCand(info, picList, refIdx, partIdxLT, MD_ABOVE_LEFT);
    }
    bAdded = bAddedSmvp;
    if (info->m_num == 2) bAdded = true;

    if (!bAdded)
    {
        bAdded = xAddMVPCandOrder(info, picList, refIdx, partIdxRT, MD_ABOVE_RIGHT);
        if (!bAdded)
        {
            bAdded = xAddMVPCandOrder(info, picList, refIdx, partIdxRT, MD_ABOVE);
        }

        if (!bAdded)
        {
            bAdded = xAddMVPCandOrder(info, picList, refIdx, partIdxLT, MD_ABOVE_LEFT);
        }
    }

    if (info->m_num == 2)
    {
        if (info->m_mvCand[0] == info->m_mvCand[1])
        {
            info->m_num = 1;
        }
    }

    if (getSlice()->getEnableTMVPFlag())
    {
        // Get Temporal Motion Predictor
        int refIdxCol = refIdx;
        MV   colmv;
        UInt partIdxRB;
        UInt absPartIdx;
        UInt absPartAddr;
        int lcuIdx = getAddr();

        deriveRightBottomIdx(partIdx, partIdxRB);
        absPartAddr = m_absIdxInLCU + partAddr;

        //----  co-located RightBottom Temporal Predictor (H) ---//
        absPartIdx = g_zscanToRaster[partIdxRB];
        if ((m_pic->getCU(m_cuAddr)->getCUPelX() + g_rasterToPelX[absPartIdx] + m_pic->getMinCUWidth()) >= m_slice->getSPS()->getPicWidthInLumaSamples())  // image boundary check
        {
            lcuIdx = -1;
        }
        else if ((m_pic->getCU(m_cuAddr)->getCUPelY() + g_rasterToPelY[absPartIdx] + m_pic->getMinCUHeight()) >= m_slice->getSPS()->getPicHeightInLumaSamples())
        {
            lcuIdx = -1;
        }
        else
        {
            if ((absPartIdx % numPartInCUWidth < numPartInCUWidth - 1) &&        // is not at the last column of LCU
                (absPartIdx / numPartInCUWidth < m_pic->getNumPartInHeight() - 1)) // is not at the last row    of LCU
            {
                absPartAddr = g_rasterToZscan[absPartIdx + numPartInCUWidth + 1];
                lcuIdx = getAddr();
            }
            else if (absPartIdx % numPartInCUWidth < numPartInCUWidth - 1)       // is not at the last column of LCU But is last row of LCU
            {
                absPartAddr = g_rasterToZscan[(absPartIdx + numPartInCUWidth + 1) % m_pic->getNumPartInCU()];
                lcuIdx      = -1;
            }
            else if (absPartIdx / numPartInCUWidth < m_pic->getNumPartInHeight() - 1) // is not at the last row of LCU But is last column of LCU
            {
                absPartAddr = g_rasterToZscan[absPartIdx + 1];
                lcuIdx = getAddr() + 1;
            }
            else //is the right bottom corner of LCU
            {
                absPartAddr = 0;
                lcuIdx      = -1;
            }
        }
        if (lcuIdx >= 0 && xGetColMVP(picList, lcuIdx, absPartAddr, colmv, refIdxCol))
        {
            info->m_mvCand[info->m_num++] = colmv;
        }
        else
        {
            UInt partIdxCenter;
            UInt curLCUIdx = getAddr();
            xDeriveCenterIdx(partIdx, partIdxCenter);
            if (xGetColMVP(picList, curLCUIdx, partIdxCenter, colmv, refIdxCol))
            {
                info->m_mvCand[info->m_num++] = colmv;
            }
        }
        //----  co-located RightBottom Temporal Predictor  ---//
    }

    if (info->m_num > AMVP_MAX_NUM_CANDS)
    {
        info->m_num = AMVP_MAX_NUM_CANDS;
    }
    while (info->m_num < AMVP_MAX_NUM_CANDS)
    {
        info->m_mvCand[info->m_num] = 0;
        info->m_num++;
    }
}

bool TComDataCU::isBipredRestriction(UInt puIdx)
{
    int width = 0;
    int height = 0;
    UInt partAddr;

    getPartIndexAndSize(puIdx, partAddr, width, height);
    if (getWidth(0) == 8 && (width < 8 || height < 8))
    {
        return true;
    }
    return false;
}

void TComDataCU::clipMv(MV& outMV, int rowsAvailable)
{
    int mvshift = 2;
    int offset = 8;
    int xmax = (m_slice->getSPS()->getPicWidthInLumaSamples() + offset - m_cuPelX - 1) << mvshift;
    int xmin = (-(int)g_maxCUWidth - offset - (int)m_cuPelX + 1) << mvshift;

    int ylimit = m_slice->getSPS()->getPicHeightInLumaSamples();
    if (rowsAvailable)
        ylimit = X265_MIN(rowsAvailable * g_maxCUHeight, ylimit);
    int ymax = (ylimit + offset - m_cuPelY - 1) << mvshift;
    int ymin = (-(int)g_maxCUHeight - offset - (int)m_cuPelY + 1) << mvshift;

    outMV.x = X265_MIN(xmax, X265_MAX(xmin, (int)outMV.x));
    outMV.y = X265_MIN(ymax, X265_MAX(ymin, (int)outMV.y));
}

UInt TComDataCU::getIntraSizeIdx(UInt absPartIdx)
{
    UInt shift = ((m_trIdx[absPartIdx] == 0) && (m_partSizes[absPartIdx] == SIZE_NxN)) ? m_trIdx[absPartIdx] + 1 : m_trIdx[absPartIdx];

    shift = (m_partSizes[absPartIdx] == SIZE_NxN ? 1 : 0);

    UChar width = m_width[absPartIdx] >> shift;
    UInt  cnt = 0;
    while (width)
    {
        cnt++;
        width >>= 1;
    }

    cnt -= 2;
    return cnt > 6 ? 6 : cnt;
}

void TComDataCU::clearCbf(UInt idx, TextType ttype, UInt numParts)
{
    ::memset(&m_cbf[g_convertTxtTypeToIdx[ttype]][idx], 0, sizeof(UChar) * numParts);
}

/** Set a I_PCM flag for all sub-partitions of a partition.
 * \param bIpcmFlag I_PCM flag
 * \param absPartIdx partition index
 * \param depth CU depth
 * \returns void
 */
void TComDataCU::setIPCMFlagSubParts(bool bIpcmFlag, UInt absPartIdx, UInt depth)
{
    UInt curPartNum = m_pic->getNumPartInCU() >> (depth << 1);

    memset(m_iPCMFlags + absPartIdx, bIpcmFlag, sizeof(bool) * curPartNum);
}

/** Test whether the current block is skipped
 * \param partIdx Block index
 * \returns Flag indicating whether the block is skipped
 */
bool TComDataCU::isSkipped(UInt partIdx)
{
    return getSkipFlag(partIdx);
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

#if _MSC_VER
#pragma warning(disable: 4701) // potentially uninitialized local variables
#endif

bool TComDataCU::xAddMVPCand(AMVPInfo* info, RefPicList picList, int refIdx, UInt partUnitIdx, MVP_DIR dir)
{
    TComDataCU* tmpCU = NULL;
    UInt idx = 0;

    switch (dir)
    {
    case MD_LEFT:
    {
        tmpCU = getPULeft(idx, partUnitIdx);
        break;
    }
    case MD_ABOVE:
    {
        tmpCU = getPUAbove(idx, partUnitIdx);
        break;
    }
    case MD_ABOVE_RIGHT:
    {
        tmpCU = getPUAboveRight(idx, partUnitIdx);
        break;
    }
    case MD_BELOW_LEFT:
    {
        tmpCU = getPUBelowLeft(idx, partUnitIdx);
        break;
    }
    case MD_ABOVE_LEFT:
    {
        tmpCU = getPUAboveLeft(idx, partUnitIdx);
        break;
    }
    default:
    {
        break;
    }
    }

    if (tmpCU == NULL)
    {
        return false;
    }

    if (tmpCU->getCUMvField(picList)->getRefIdx(idx) >= 0 && m_slice->getRefPic(picList, refIdx)->getPOC() == tmpCU->getSlice()->getRefPOC(picList, tmpCU->getCUMvField(picList)->getRefIdx(idx)))
    {
        MV mvp = tmpCU->getCUMvField(picList)->getMv(idx);
        info->m_mvCand[info->m_num++] = mvp;
        return true;
    }

    RefPicList refPicList2nd = REF_PIC_LIST_0;
    if (picList == REF_PIC_LIST_0)
    {
        refPicList2nd = REF_PIC_LIST_1;
    }
    else if (picList == REF_PIC_LIST_1)
    {
        refPicList2nd = REF_PIC_LIST_0;
    }

    int curRefPOC = m_slice->getRefPic(picList, refIdx)->getPOC();
    int neibRefPOC;

    if (tmpCU->getCUMvField(refPicList2nd)->getRefIdx(idx) >= 0)
    {
        neibRefPOC = tmpCU->getSlice()->getRefPOC(refPicList2nd, tmpCU->getCUMvField(refPicList2nd)->getRefIdx(idx));
        if (neibRefPOC == curRefPOC) // Same Reference Frame But Diff List//
        {
            MV mvp = tmpCU->getCUMvField(refPicList2nd)->getMv(idx);
            info->m_mvCand[info->m_num++] = mvp;
            return true;
        }
    }
    return false;
}

/**
 * \param info
 * \param picList
 * \param refIdx
 * \param partUnitIdx
 * \param dir
 * \returns bool
 */
bool TComDataCU::xAddMVPCandOrder(AMVPInfo* info, RefPicList picList, int refIdx, UInt partUnitIdx, MVP_DIR dir)
{
    TComDataCU* tmpCU = NULL;
    UInt idx = 0;

    switch (dir)
    {
    case MD_LEFT:
    {
        tmpCU = getPULeft(idx, partUnitIdx);
        break;
    }
    case MD_ABOVE:
    {
        tmpCU = getPUAbove(idx, partUnitIdx);
        break;
    }
    case MD_ABOVE_RIGHT:
    {
        tmpCU = getPUAboveRight(idx, partUnitIdx);
        break;
    }
    case MD_BELOW_LEFT:
    {
        tmpCU = getPUBelowLeft(idx, partUnitIdx);
        break;
    }
    case MD_ABOVE_LEFT:
    {
        tmpCU = getPUAboveLeft(idx, partUnitIdx);
        break;
    }
    default:
    {
        break;
    }
    }

    if (tmpCU == NULL)
    {
        return false;
    }

    RefPicList refPicList2nd = REF_PIC_LIST_0;
    if (picList == REF_PIC_LIST_0)
    {
        refPicList2nd = REF_PIC_LIST_1;
    }
    else if (picList == REF_PIC_LIST_1)
    {
        refPicList2nd = REF_PIC_LIST_0;
    }

    int curPOC = m_slice->getPOC();
    int curRefPOC = m_slice->getRefPic(picList, refIdx)->getPOC();
    int neibPOC = curPOC;
    int neibRefPOC;

    bool bIsCurrRefLongTerm = m_slice->getRefPic(picList, refIdx)->getIsLongTerm();
    bool bIsNeibRefLongTerm = false;

    //---------------  V1 (END) ------------------//
    if (tmpCU->getCUMvField(picList)->getRefIdx(idx) >= 0)
    {
        neibRefPOC = tmpCU->getSlice()->getRefPOC(picList, tmpCU->getCUMvField(picList)->getRefIdx(idx));
        MV mvp = tmpCU->getCUMvField(picList)->getMv(idx);
        MV outMV;

        bIsNeibRefLongTerm = tmpCU->getSlice()->getRefPic(picList, tmpCU->getCUMvField(picList)->getRefIdx(idx))->getIsLongTerm();
        if (bIsCurrRefLongTerm == bIsNeibRefLongTerm)
        {
            if (bIsCurrRefLongTerm || bIsNeibRefLongTerm)
            {
                outMV = mvp;
            }
            else
            {
                int iScale = xGetDistScaleFactor(curPOC, curRefPOC, neibPOC, neibRefPOC);
                if (iScale == 4096)
                {
                    outMV = mvp;
                }
                else
                {
                    outMV = scaleMv(mvp, iScale);
                }
            }
            info->m_mvCand[info->m_num++] = outMV;
            return true;
        }
    }

    //---------------------- V2(END) --------------------//
    if (tmpCU->getCUMvField(refPicList2nd)->getRefIdx(idx) >= 0)
    {
        neibRefPOC = tmpCU->getSlice()->getRefPOC(refPicList2nd, tmpCU->getCUMvField(refPicList2nd)->getRefIdx(idx));
        MV mvp = tmpCU->getCUMvField(refPicList2nd)->getMv(idx);
        MV outMV;

        bIsNeibRefLongTerm = tmpCU->getSlice()->getRefPic(refPicList2nd, tmpCU->getCUMvField(refPicList2nd)->getRefIdx(idx))->getIsLongTerm();
        if (bIsCurrRefLongTerm == bIsNeibRefLongTerm)
        {
            if (bIsCurrRefLongTerm || bIsNeibRefLongTerm)
            {
                outMV = mvp;
            }
            else
            {
                int iScale = xGetDistScaleFactor(curPOC, curRefPOC, neibPOC, neibRefPOC);
                if (iScale == 4096)
                {
                    outMV = mvp;
                }
                else
                {
                    outMV = scaleMv(mvp, iScale);
                }
            }
            info->m_mvCand[info->m_num++] = outMV;
            return true;
        }
    }
    //---------------------- V3(END) --------------------//
    return false;
}

/**
 * \param picList
 * \param cuAddr
 * \param partUnitIdx
 * \param outRefIdx
 * \returns bool
 */
bool TComDataCU::xGetColMVP(RefPicList picList, int cuAddr, int partUnitIdx, MV& outMV, int& outRefIdx)
{
    UInt absPartAddr = partUnitIdx;

    RefPicList colRefPicList;
    int colPOC, colRefPOC, curPOC, curRefPOC, scale;
    MV colmv;

    // use coldir.
    TComPic *colPic = getSlice()->getRefPic(RefPicList(getSlice()->isInterB() ? 1 - getSlice()->getColFromL0Flag() : 0), getSlice()->getColRefIdx());
    TComDataCU *colCU = colPic->getCU(cuAddr);

    if (colCU->getPic() == 0 || colCU->getPartitionSize(partUnitIdx) == SIZE_NONE)
    {
        return false;
    }
    curPOC = m_slice->getPOC();
    curRefPOC = m_slice->getRefPic(picList, outRefIdx)->getPOC();
    colPOC = colCU->getSlice()->getPOC();

    if (colCU->isIntra_cmv(absPartAddr))
    {
        return false;
    }
    colRefPicList = getSlice()->getCheckLDC() ? picList : RefPicList(getSlice()->getColFromL0Flag());

    int colRefIdx = colCU->getCUMvField(RefPicList(colRefPicList))->getRefIdx_cmv(absPartAddr);

    if (colRefIdx < 0)
    {
        colRefPicList = RefPicList(1 - colRefPicList);
        colRefIdx = colCU->getCUMvField(RefPicList(colRefPicList))->getRefIdx_cmv(absPartAddr);

        if (colRefIdx < 0)
        {
            return false;
        }
    }

    // Scale the vector.
    colRefPOC = colCU->getSlice()->getRefPOC(colRefPicList, colRefIdx);
    colmv = colCU->getCUMvField(colRefPicList)->getMv_cmv(absPartAddr);

    curRefPOC = m_slice->getRefPic(picList, outRefIdx)->getPOC();
    bool bIsCurrRefLongTerm = m_slice->getRefPic(picList, outRefIdx)->getIsLongTerm();
    bool bIsColRefLongTerm = colCU->getSlice()->getIsUsedAsLongTerm(colRefPicList, colRefIdx);

    if (bIsCurrRefLongTerm != bIsColRefLongTerm)
    {
        return false;
    }

    if (bIsCurrRefLongTerm || bIsColRefLongTerm)
    {
        outMV = colmv;
    }
    else
    {
        scale = xGetDistScaleFactor(curPOC, curRefPOC, colPOC, colRefPOC);
        if (scale == 4096)
        {
            outMV = colmv;
        }
        else
        {
            outMV = scaleMv(colmv, scale);
        }
    }
    return true;
}

int TComDataCU::xGetDistScaleFactor(int curPOC, int curRefPOC, int colPOC, int colRefPOC)
{
    int diffPocD = colPOC - colRefPOC;
    int diffPocB = curPOC - curRefPOC;

    if (diffPocD == diffPocB)
    {
        return 4096;
    }
    else
    {
        int tdb      = Clip3(-128, 127, diffPocB);
        int tdd      = Clip3(-128, 127, diffPocD);
        int x        = (0x4000 + abs(tdd / 2)) / tdd;
        int scale    = Clip3(-4096, 4095, (tdb * x + 32) >> 6);
        return scale;
    }
}

/**
 * \param cuMode
 * \param partIdx
 * \param outPartIdxCenter
 * \returns void
 */
void TComDataCU::xDeriveCenterIdx(UInt partIdx, UInt& outPartIdxCenter)
{
    UInt partAddr;
    int  partWidth;
    int  partHeight;

    getPartIndexAndSize(partIdx, partAddr, partWidth, partHeight);

    outPartIdxCenter = m_absIdxInLCU + partAddr; // partition origin.
    outPartIdxCenter = g_rasterToZscan[g_zscanToRaster[outPartIdxCenter]
                                       + (partHeight / m_pic->getMinCUHeight()) / 2 * m_pic->getNumPartInWidth()
                                       + (partWidth / m_pic->getMinCUWidth()) / 2];
}

void TComDataCU::compressMV()
{
    int scaleFactor = 4 * AMVP_DECIMATION_FACTOR / m_unitSize;

    if (scaleFactor > 0)
    {
        memcpy(m_cmv_predModes, m_predModes, m_numPartitions * sizeof(m_predModes[0]));
        m_cuMvField[0].compress(m_cmv_predModes, scaleFactor);
        m_cuMvField[1].compress(m_cmv_predModes, scaleFactor);
    }
}

UInt TComDataCU::getCoefScanIdx(UInt absPartIdx, UInt width, bool bIsLuma, bool bIsIntra)
{
    UInt uiCTXIdx;
    UInt scanIdx;
    UInt dirMode;

    if (!bIsIntra)
    {
        scanIdx = SCAN_DIAG;
        return scanIdx;
    }

    switch (width)
    {
    case  2: uiCTXIdx = 6;
        break;
    case  4: uiCTXIdx = 5;
        break;
    case  8: uiCTXIdx = 4;
        break;
    case 16: uiCTXIdx = 3;
        break;
    case 32: uiCTXIdx = 2;
        break;
    case 64: uiCTXIdx = 1;
        break;
    default: uiCTXIdx = 0;
        break;
    }

    if (bIsLuma)
    {
        dirMode = getLumaIntraDir(absPartIdx);
        scanIdx = SCAN_DIAG;
        if (uiCTXIdx > 3 && uiCTXIdx < 6) //if multiple scans supported for transform size
        {
            scanIdx = abs((int)dirMode - VER_IDX) < 5 ? SCAN_HOR : (abs((int)dirMode - HOR_IDX) < 5 ? SCAN_VER : SCAN_DIAG);
        }
    }
    else
    {
        dirMode = getChromaIntraDir(absPartIdx);
        if (dirMode == DM_CHROMA_IDX)
        {
            // get number of partitions in current CU
            UInt depth = getDepth(absPartIdx);
            UInt numParts = getPic()->getNumPartInCU() >> (2 * depth);

            // get luma mode from upper-left corner of current CU
            dirMode = getLumaIntraDir((absPartIdx / numParts) * numParts);
        }
        scanIdx = SCAN_DIAG;
        if (uiCTXIdx > 4 && uiCTXIdx < 7) //if multiple scans supported for transform size
        {
            scanIdx = abs((int)dirMode - VER_IDX) < 5 ? SCAN_HOR : (abs((int)dirMode - HOR_IDX) < 5 ? SCAN_VER : SCAN_DIAG);
        }
    }

    return scanIdx;
}

UInt TComDataCU::getSCUAddr()
{
    return (m_cuAddr) * (1 << (m_slice->getSPS()->getMaxCUDepth() << 1)) + m_absIdxInLCU;
}

/** Set neighboring blocks availabilities for non-deblocked filtering
 * \param numLCUInPicWidth number of LCUs in picture width
 * \param numLCUInPicHeight number of LCUs in picture height
 * \param numSUInLCUWidth number of SUs in LCU width
 * \param numSUInLCUHeight number of SUs in LCU height
 * \param picWidth picture width
 * \param picHeight picture height
 * \param bIndependentSliceBoundaryEnabled true for independent slice boundary enabled
 * \param bTopTileBoundary true means that top boundary coincides tile boundary
 * \param bDownTileBoundary true means that bottom boundary coincides tile boundary
 * \param bLeftTileBoundary true means that left boundary coincides tile boundary
 * \param bRightTileBoundary true means that right boundary coincides tile boundary
 * \param bIndependentTileBoundaryEnabled true for independent tile boundary enabled
 */
void TComDataCU::setNDBFilterBlockBorderAvailability(UInt /*numLCUInPicWidth*/, UInt /*numLCUInPicHeight*/, UInt numSUInLCUWidth, UInt numSUInLCUHeight,
                                                     UInt picWidth, UInt picHeight, bool bTopTileBoundary, bool bDownTileBoundary, bool bLeftTileBoundary,
                                                     bool bRightTileBoundary, bool bIndependentTileBoundaryEnabled)
{
    UInt lpelx, tpely;
    UInt width, height;
    bool bPicRBoundary, bPicBBoundary, bPicTBoundary, bPicLBoundary;
    bool bLCURBoundary = false, bLCUBBoundary = false, bLCUTBoundary = false, bLCULBoundary = false;
    bool* bAvailBorder;
    bool* bAvail;
    UInt rTLSU, rBRSU, widthSU;
    UInt numSGU = (UInt)m_vNDFBlock.size();

    for (int i = 0; i < numSGU; i++)
    {
        NDBFBlockInfo& rSGU = m_vNDFBlock[i];

        lpelx = rSGU.posX;
        tpely = rSGU.posY;
        width   = rSGU.width;
        height  = rSGU.height;
        rTLSU     = g_zscanToRaster[rSGU.startSU];
        rBRSU     = g_zscanToRaster[rSGU.endSU];
        widthSU   = rSGU.widthSU;

        bAvailBorder = rSGU.isBorderAvailable;

        bPicTBoundary = (tpely == 0) ? (true) : (false);
        bPicLBoundary = (lpelx == 0) ? (true) : (false);
        bPicRBoundary = (!(lpelx + width < picWidth)) ? (true) : (false);
        bPicBBoundary = (!(tpely + height < picHeight)) ? (true) : (false);

        bLCULBoundary = (rTLSU % numSUInLCUWidth == 0) ? (true) : (false);
        bLCURBoundary = ((rTLSU + widthSU) % numSUInLCUWidth == 0) ? (true) : (false);
        bLCUTBoundary = ((UInt)(rTLSU / numSUInLCUWidth) == 0) ? (true) : (false);
        bLCUBBoundary = ((UInt)(rBRSU / numSUInLCUWidth) == (numSUInLCUHeight - 1)) ? (true) : (false);

        //       SGU_L
        bAvail = &(bAvailBorder[SGU_L]);
        if (bPicLBoundary)
        {
            *bAvail = false;
        }
        else
        {
            *bAvail = true;
        }

        //       SGU_R
        bAvail = &(bAvailBorder[SGU_R]);
        if (bPicRBoundary)
        {
            *bAvail = false;
        }
        else
        {
            *bAvail = true;
        }

        //       SGU_T
        bAvail = &(bAvailBorder[SGU_T]);
        if (bPicTBoundary)
        {
            *bAvail = false;
        }
        else
        {
            *bAvail = true;
        }

        //       SGU_B
        bAvail = &(bAvailBorder[SGU_B]);
        if (bPicBBoundary)
        {
            *bAvail = false;
        }
        else
        {
            *bAvail = true;
        }

        //       SGU_TL
        bAvail = &(bAvailBorder[SGU_TL]);
        if (bPicTBoundary || bPicLBoundary)
        {
            *bAvail = false;
        }
        else
        {
            *bAvail = true;
        }

        //       SGU_TR
        bAvail = &(bAvailBorder[SGU_TR]);
        if (bPicTBoundary || bPicRBoundary)
        {
            *bAvail = false;
        }
        else
        {
            *bAvail = true;
        }

        //       SGU_BL
        bAvail = &(bAvailBorder[SGU_BL]);
        if (bPicBBoundary || bPicLBoundary)
        {
            *bAvail = false;
        }
        else
        {
            *bAvail = true;
        }

        //       SGU_BR
        bAvail = &(bAvailBorder[SGU_BR]);
        if (bPicBBoundary || bPicRBoundary)
        {
            *bAvail = false;
        }
        else
        {
            *bAvail = true;
        }

        if (bIndependentTileBoundaryEnabled)
        {
            //left LCU boundary
            if (!bPicLBoundary && bLCULBoundary)
            {
                if (bLeftTileBoundary)
                {
                    bAvailBorder[SGU_L] = bAvailBorder[SGU_TL] = bAvailBorder[SGU_BL] = false;
                }
            }
            //right LCU boundary
            if (!bPicRBoundary && bLCURBoundary)
            {
                if (bRightTileBoundary)
                {
                    bAvailBorder[SGU_R] = bAvailBorder[SGU_TR] = bAvailBorder[SGU_BR] = false;
                }
            }
            //top LCU boundary
            if (!bPicTBoundary && bLCUTBoundary)
            {
                if (bTopTileBoundary)
                {
                    bAvailBorder[SGU_T] = bAvailBorder[SGU_TL] = bAvailBorder[SGU_TR] = false;
                }
            }
            //down LCU boundary
            if (!bPicBBoundary && bLCUBBoundary)
            {
                if (bDownTileBoundary)
                {
                    bAvailBorder[SGU_B] = bAvailBorder[SGU_BL] = bAvailBorder[SGU_BR] = false;
                }
            }
        }
        rSGU.allBordersAvailable = true;
        for (int b = 0; b < NUM_SGU_BORDER; b++)
        {
            if (bAvailBorder[b] == false)
            {
                rSGU.allBordersAvailable = false;
                break;
            }
        }
    }
}

//! \}
