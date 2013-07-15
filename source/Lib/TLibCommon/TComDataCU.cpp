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
    m_pic              = NULL;
    m_slice            = NULL;
    m_depth           = NULL;

    m_skipFlag           = NULL;

    m_partSizes         = NULL;
    m_predModes         = NULL;
    m_cuTransquantBypass = NULL;
    m_width           = NULL;
    m_height          = NULL;
    m_qp               = NULL;
    m_pbMergeFlag        = NULL;
    m_puhMergeIndex      = NULL;
    m_puhLumaIntraDir    = NULL;
    m_puhChromaIntraDir  = NULL;
    m_puhInterDir        = NULL;
    m_trIdx           = NULL;
    m_transformSkip[0] = NULL;
    m_transformSkip[1] = NULL;
    m_transformSkip[2] = NULL;
    m_cbf[0]          = NULL;
    m_cbf[1]          = NULL;
    m_cbf[2]          = NULL;
    m_trCoeffY         = NULL;
    m_trCoeffCb        = NULL;
    m_trCoeffCr        = NULL;
    m_arlCoeffY        = NULL;
    m_arlCoeffCb       = NULL;
    m_arlCoeffCr       = NULL;

    m_pbIPCMFlag         = NULL;
    m_iPCMSampleY      = NULL;
    m_iPCMSampleCb     = NULL;
    m_iPCMSampleCr     = NULL;

    m_pattern          = NULL;

    m_cuAboveLeft      = NULL;
    m_cuAboveRight     = NULL;
    m_cuAbove          = NULL;
    m_cuLeft           = NULL;

    m_cuColocated[0]  = NULL;
    m_cuColocated[1]  = NULL;

    m_apiMVPIdx[0]       = NULL;
    m_apiMVPIdx[1]       = NULL;
    m_apiMVPNum[0]       = NULL;
    m_apiMVPNum[1]       = NULL;
}

TComDataCU::~TComDataCU()
{}

Void TComDataCU::create(UInt numPartition, UInt width, UInt height, Int unitSize)
{
    m_pic           = NULL;
    m_slice         = NULL;
    m_numPartitions = numPartition;
    m_unitSize = unitSize;

    m_qp     = (Char*)xMalloc(Char,   numPartition);
    m_depth  = (UChar*)xMalloc(UChar, numPartition);
    m_width  = (UChar*)xMalloc(UChar, numPartition);
    m_height = (UChar*)xMalloc(UChar, numPartition);

    m_skipFlag = new Bool[numPartition];

    m_partSizes = new Char[numPartition];
    memset(m_partSizes, SIZE_NONE, numPartition * sizeof(*m_partSizes));
    m_predModes = new Char[numPartition];
    m_cuTransquantBypass = new Bool[numPartition];

    m_pbMergeFlag        = (Bool*)xMalloc(Bool,  numPartition);
    m_puhMergeIndex      = (UChar*)xMalloc(UChar, numPartition);
    m_puhLumaIntraDir    = (UChar*)xMalloc(UChar, numPartition);
    m_puhChromaIntraDir  = (UChar*)xMalloc(UChar, numPartition);
    m_puhInterDir        = (UChar*)xMalloc(UChar, numPartition);

    m_trIdx            = (UChar*)xMalloc(UChar, numPartition);
    m_transformSkip[0] = (UChar*)xMalloc(UChar, numPartition);
    m_transformSkip[1] = (UChar*)xMalloc(UChar, numPartition);
    m_transformSkip[2] = (UChar*)xMalloc(UChar, numPartition);

    m_cbf[0] = (UChar*)xMalloc(UChar, numPartition);
    m_cbf[1] = (UChar*)xMalloc(UChar, numPartition);
    m_cbf[2] = (UChar*)xMalloc(UChar, numPartition);

    m_apiMVPIdx[0] = new Char[numPartition];
    m_apiMVPIdx[1] = new Char[numPartition];
    m_apiMVPNum[0] = new Char[numPartition];
    m_apiMVPNum[1] = new Char[numPartition];
    memset(m_apiMVPIdx[0], -1, numPartition * sizeof(Char));
    memset(m_apiMVPIdx[1], -1, numPartition * sizeof(Char));

    m_trCoeffY  = (TCoeff*)xMalloc(TCoeff, width * height);
    m_trCoeffCb = (TCoeff*)xMalloc(TCoeff, width * height / 4);
    m_trCoeffCr = (TCoeff*)xMalloc(TCoeff, width * height / 4);
    memset(m_trCoeffY, 0, width * height * sizeof(TCoeff));
    memset(m_trCoeffCb, 0, width * height / 4 * sizeof(TCoeff));
    memset(m_trCoeffCr, 0, width * height / 4 * sizeof(TCoeff));

    m_arlCoeffY        = (Int*)xMalloc(Int, width * height);
    m_arlCoeffCb       = (Int*)xMalloc(Int, width * height / 4);
    m_arlCoeffCr       = (Int*)xMalloc(Int, width * height / 4);

    m_pbIPCMFlag         = (Bool*)xMalloc(Bool, numPartition);
    m_iPCMSampleY      = (Pel*)xMalloc(Pel, width * height);
    m_iPCMSampleCb     = (Pel*)xMalloc(Pel, width * height / 4);
    m_iPCMSampleCr     = (Pel*)xMalloc(Pel, width * height / 4);

    m_cuMvField[0].create(numPartition);
    m_cuMvField[1].create(numPartition);

    // create pattern memory
    m_pattern = (TComPattern*)xMalloc(TComPattern, 1);

    // create motion vector fields
    m_cuAboveLeft     = NULL;
    m_cuAboveRight    = NULL;
    m_cuAbove         = NULL;
    m_cuLeft          = NULL;

    m_cuColocated[0] = NULL;
    m_cuColocated[1] = NULL;
}

Void TComDataCU::destroy()
{
    m_pic   = NULL;
    m_slice = NULL;

    if (m_pattern)
    {
        xFree(m_pattern);
        m_pattern = NULL;
    }

    // encoder-side buffer free
    if (m_qp) { xFree(m_qp); m_qp = NULL; }
    if (m_depth) { xFree(m_depth); m_depth = NULL; }
    if (m_width) { xFree(m_width); m_width = NULL; }
    if (m_height) { xFree(m_height); m_height = NULL; }

    if (m_skipFlag) { delete[] m_skipFlag; m_skipFlag = NULL; }

    if (m_partSizes) { delete[] m_partSizes; m_partSizes = NULL; }
    if (m_predModes) { delete[] m_predModes; m_predModes = NULL; }
    if (m_cuTransquantBypass) { delete[] m_cuTransquantBypass; m_cuTransquantBypass = NULL; }
    if (m_cbf[0]) { xFree(m_cbf[0]); m_cbf[0] = NULL; }
    if (m_cbf[1]) { xFree(m_cbf[1]); m_cbf[1] = NULL; }
    if (m_cbf[2]) { xFree(m_cbf[2]); m_cbf[2] = NULL; }
    if (m_puhInterDir) { xFree(m_puhInterDir); m_puhInterDir = NULL; }
    if (m_pbMergeFlag) { xFree(m_pbMergeFlag); m_pbMergeFlag = NULL; }
    if (m_puhMergeIndex) { xFree(m_puhMergeIndex); m_puhMergeIndex = NULL; }
    if (m_puhLumaIntraDir) { xFree(m_puhLumaIntraDir); m_puhLumaIntraDir = NULL; }
    if (m_puhChromaIntraDir) { xFree(m_puhChromaIntraDir); m_puhChromaIntraDir = NULL; }
    if (m_trIdx) { xFree(m_trIdx); m_trIdx = NULL; }
    if (m_transformSkip[0]) { xFree(m_transformSkip[0]); m_transformSkip[0] = NULL; }
    if (m_transformSkip[1]) { xFree(m_transformSkip[1]); m_transformSkip[1] = NULL; }
    if (m_transformSkip[2]) { xFree(m_transformSkip[2]); m_transformSkip[2] = NULL; }
    if (m_trCoeffY) { xFree(m_trCoeffY); m_trCoeffY = NULL; }
    if (m_trCoeffCb) { xFree(m_trCoeffCb); m_trCoeffCb = NULL; }
    if (m_trCoeffCr) { xFree(m_trCoeffCr); m_trCoeffCr = NULL; }
    if (m_arlCoeffY) { xFree(m_arlCoeffY); m_arlCoeffY = NULL; }
    if (m_arlCoeffCb) { xFree(m_arlCoeffCb); m_arlCoeffCb = NULL; }
    if (m_arlCoeffCr) { xFree(m_arlCoeffCr); m_arlCoeffCr = NULL; }
    if (m_pbIPCMFlag) { xFree(m_pbIPCMFlag); m_pbIPCMFlag = NULL; }
    if (m_iPCMSampleY) { xFree(m_iPCMSampleY); m_iPCMSampleY = NULL; }
    if (m_iPCMSampleCb) { xFree(m_iPCMSampleCb); m_iPCMSampleCb = NULL; }
    if (m_iPCMSampleCr) { xFree(m_iPCMSampleCr); m_iPCMSampleCr = NULL; }
    if (m_apiMVPIdx[0]) { delete[] m_apiMVPIdx[0]; m_apiMVPIdx[0] = NULL; }
    if (m_apiMVPIdx[1]) { delete[] m_apiMVPIdx[1]; m_apiMVPIdx[1] = NULL; }
    if (m_apiMVPNum[0]) { delete[] m_apiMVPNum[0]; m_apiMVPNum[0] = NULL; }
    if (m_apiMVPNum[1]) { delete[] m_apiMVPNum[1]; m_apiMVPNum[1] = NULL; }

    m_cuMvField[0].destroy();
    m_cuMvField[1].destroy();

    m_cuAboveLeft  = NULL;
    m_cuAboveRight = NULL;
    m_cuAbove      = NULL;
    m_cuLeft       = NULL;

    m_cuColocated[0] = NULL;
    m_cuColocated[1] = NULL;
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
    ::memcpy(this->isBorderAvailable, src.isBorderAvailable, sizeof(Bool) * ((Int)NUM_SGU_BORDER));
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
Void TComDataCU::initCU(TComPic* pic, UInt iCUAddr)
{
    m_pic              = pic;
    m_slice            = pic->getSlice();
    m_cuAddr           = iCUAddr;
    m_cuPelX           = (iCUAddr % pic->getFrameWidthInCU()) * g_maxCUWidth;
    m_cuPelY           = (iCUAddr / pic->getFrameWidthInCU()) * g_maxCUHeight;
    m_absIdxInLCU      = 0;
    m_totalCost         = MAX_INT64;
    m_totalDistortion  = 0;
    m_totalBits        = 0;
    m_totalBins        = 0;
    m_numPartitions     = pic->getNumPartInCU();

    // CHECK_ME: why partStartIdx always negtive
    Int partStartIdx = 0 - (iCUAddr) * pic->getNumPartInCU();

    Int numElements = min<Int>(partStartIdx, m_numPartitions);
    for (Int ui = 0; ui < numElements; ui++)
    {
        TComDataCU * pcFrom = pic->getCU(getAddr());
        m_skipFlag[ui]   = pcFrom->getSkipFlag(ui);
        m_partSizes[ui] = pcFrom->getPartitionSize(ui);
        m_predModes[ui] = pcFrom->getPredictionMode(ui);
        m_cuTransquantBypass[ui] = pcFrom->getCUTransquantBypass(ui);
        m_depth[ui] = pcFrom->getDepth(ui);
        m_width[ui] = pcFrom->getWidth(ui);
        m_height[ui] = pcFrom->getHeight(ui);
        m_trIdx[ui] = pcFrom->getTransformIdx(ui);
        m_transformSkip[0][ui] = pcFrom->getTransformSkip(ui, TEXT_LUMA);
        m_transformSkip[1][ui] = pcFrom->getTransformSkip(ui, TEXT_CHROMA_U);
        m_transformSkip[2][ui] = pcFrom->getTransformSkip(ui, TEXT_CHROMA_V);
        m_apiMVPIdx[0][ui] = pcFrom->m_apiMVPIdx[0][ui];
        m_apiMVPIdx[1][ui] = pcFrom->m_apiMVPIdx[1][ui];
        m_apiMVPNum[0][ui] = pcFrom->m_apiMVPNum[0][ui];
        m_apiMVPNum[1][ui] = pcFrom->m_apiMVPNum[1][ui];
        m_qp[ui] = pcFrom->m_qp[ui];
        m_pbMergeFlag[ui] = pcFrom->m_pbMergeFlag[ui];
        m_puhMergeIndex[ui] = pcFrom->m_puhMergeIndex[ui];
        m_puhLumaIntraDir[ui] = pcFrom->m_puhLumaIntraDir[ui];
        m_puhChromaIntraDir[ui] = pcFrom->m_puhChromaIntraDir[ui];
        m_puhInterDir[ui] = pcFrom->m_puhInterDir[ui];
        m_cbf[0][ui] = pcFrom->m_cbf[0][ui];
        m_cbf[1][ui] = pcFrom->m_cbf[1][ui];
        m_cbf[2][ui] = pcFrom->m_cbf[2][ui];
        m_pbIPCMFlag[ui] = pcFrom->m_pbIPCMFlag[ui];
    }

    Int firstElement = max<Int>(partStartIdx, 0);
    numElements = m_numPartitions - firstElement;

    if (numElements > 0)
    {
        memset(m_skipFlag          + firstElement, false,                    numElements * sizeof(*m_skipFlag));

        memset(m_partSizes        + firstElement, SIZE_NONE,                numElements * sizeof(*m_partSizes));
        memset(m_predModes        + firstElement, MODE_NONE,                numElements * sizeof(*m_predModes));
        memset(m_cuTransquantBypass + firstElement, false,                   numElements * sizeof(*m_cuTransquantBypass));
        memset(m_depth          + firstElement, 0,                        numElements * sizeof(*m_depth));
        memset(m_trIdx          + firstElement, 0,                        numElements * sizeof(*m_trIdx));
        memset(m_transformSkip[0] + firstElement, 0,                      numElements * sizeof(*m_transformSkip[0]));
        memset(m_transformSkip[1] + firstElement, 0,                      numElements * sizeof(*m_transformSkip[1]));
        memset(m_transformSkip[2] + firstElement, 0,                      numElements * sizeof(*m_transformSkip[2]));
        memset(m_width          + firstElement, g_maxCUWidth,           numElements * sizeof(*m_width));
        memset(m_height         + firstElement, g_maxCUHeight,          numElements * sizeof(*m_height));
        memset(m_apiMVPIdx[0]      + firstElement, -1,                       numElements * sizeof(*m_apiMVPIdx[0]));
        memset(m_apiMVPIdx[1]      + firstElement, -1,                       numElements * sizeof(*m_apiMVPIdx[1]));
        memset(m_apiMVPNum[0]      + firstElement, -1,                       numElements * sizeof(*m_apiMVPNum[0]));
        memset(m_apiMVPNum[1]      + firstElement, -1,                       numElements * sizeof(*m_apiMVPNum[1]));
        memset(m_qp              + firstElement, getSlice()->getSliceQp(), numElements * sizeof(*m_qp));
        memset(m_pbMergeFlag       + firstElement, false,                    numElements * sizeof(*m_pbMergeFlag));
        memset(m_puhMergeIndex     + firstElement, 0,                        numElements * sizeof(*m_puhMergeIndex));
        memset(m_puhLumaIntraDir   + firstElement, DC_IDX,                   numElements * sizeof(*m_puhLumaIntraDir));
        memset(m_puhChromaIntraDir + firstElement, 0,                        numElements * sizeof(*m_puhChromaIntraDir));
        memset(m_puhInterDir       + firstElement, 0,                        numElements * sizeof(*m_puhInterDir));
        memset(m_cbf[0]         + firstElement, 0,                        numElements * sizeof(*m_cbf[0]));
        memset(m_cbf[1]         + firstElement, 0,                        numElements * sizeof(*m_cbf[1]));
        memset(m_cbf[2]         + firstElement, 0,                        numElements * sizeof(*m_cbf[2]));
        memset(m_pbIPCMFlag        + firstElement, false,                    numElements * sizeof(*m_pbIPCMFlag));
    }

    UInt uiTmp = g_maxCUWidth * g_maxCUHeight;
    if (0 >= partStartIdx)
    {
        m_cuMvField[0].clearMvField();
        m_cuMvField[1].clearMvField();
        memset(m_trCoeffY, 0, sizeof(TCoeff) * uiTmp);
        memset(m_arlCoeffY, 0, sizeof(Int) * uiTmp);
        memset(m_iPCMSampleY, 0, sizeof(Pel) * uiTmp);
        uiTmp  >>= 2;
        memset(m_trCoeffCb, 0, sizeof(TCoeff) * uiTmp);
        memset(m_trCoeffCr, 0, sizeof(TCoeff) * uiTmp);
        memset(m_arlCoeffCb, 0, sizeof(Int) * uiTmp);
        memset(m_arlCoeffCr, 0, sizeof(Int) * uiTmp);
        memset(m_iPCMSampleCb, 0, sizeof(Pel) * uiTmp);
        memset(m_iPCMSampleCr, 0, sizeof(Pel) * uiTmp);
    }
    else
    {
        TComDataCU * pcFrom = pic->getCU(getAddr());
        m_cuMvField[0].copyFrom(&pcFrom->m_cuMvField[0], m_numPartitions, 0);
        m_cuMvField[1].copyFrom(&pcFrom->m_cuMvField[1], m_numPartitions, 0);
        for (Int i = 0; i < uiTmp; i++)
        {
            m_trCoeffY[i] = pcFrom->m_trCoeffY[i];
            m_arlCoeffY[i] = pcFrom->m_arlCoeffY[i];
            m_iPCMSampleY[i] = pcFrom->m_iPCMSampleY[i];
        }

        for (Int i = 0; i < (uiTmp >> 2); i++)
        {
            m_trCoeffCb[i] = pcFrom->m_trCoeffCb[i];
            m_trCoeffCr[i] = pcFrom->m_trCoeffCr[i];
            m_arlCoeffCb[i] = pcFrom->m_arlCoeffCb[i];
            m_arlCoeffCr[i] = pcFrom->m_arlCoeffCr[i];
            m_iPCMSampleCb[i] = pcFrom->m_iPCMSampleCb[i];
            m_iPCMSampleCr[i] = pcFrom->m_iPCMSampleCr[i];
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
Void TComDataCU::initEstData(UInt depth, Int qp)
{
    m_totalCost         = MAX_INT64;
    m_totalDistortion  = 0;
    m_totalBits        = 0;
    m_totalBins        = 0;

    UChar uhWidth  = g_maxCUWidth  >> depth;
    UChar uhHeight = g_maxCUHeight >> depth;

    for (UInt ui = 0; ui < m_numPartitions; ui++)
    {
        m_apiMVPIdx[0][ui] = -1;
        m_apiMVPIdx[1][ui] = -1;
        m_apiMVPNum[0][ui] = -1;
        m_apiMVPNum[1][ui] = -1;
        m_depth[ui] = depth;
        m_width[ui] = uhWidth;
        m_height[ui] = uhHeight;
        m_trIdx[ui] = 0;
        m_transformSkip[0][ui] = 0;
        m_transformSkip[1][ui] = 0;
        m_transformSkip[2][ui] = 0;
        m_skipFlag[ui]   = false;
        m_partSizes[ui] = SIZE_NONE;
        m_predModes[ui] = MODE_NONE;
        m_cuTransquantBypass[ui] = false;
        m_pbIPCMFlag[ui] = 0;
        m_qp[ui] = qp;
        m_pbMergeFlag[ui] = 0;
        m_puhMergeIndex[ui] = 0;
        m_puhLumaIntraDir[ui] = DC_IDX;
        m_puhChromaIntraDir[ui] = 0;
        m_puhInterDir[ui] = 0;
        m_cbf[0][ui] = 0;
        m_cbf[1][ui] = 0;
        m_cbf[2][ui] = 0;
    }

    UInt uiTmp = uhWidth * uhHeight;

    {
        m_cuMvField[0].clearMvField();
        m_cuMvField[1].clearMvField();
        uiTmp = uhWidth * uhHeight;

        memset(m_trCoeffY,    0, uiTmp * sizeof(*m_trCoeffY));
        memset(m_arlCoeffY,  0, uiTmp * sizeof(*m_arlCoeffY));
        memset(m_iPCMSampleY, 0, uiTmp * sizeof(*m_iPCMSampleY));

        uiTmp >>= 2;
        memset(m_trCoeffCb,    0, uiTmp * sizeof(*m_trCoeffCb));
        memset(m_trCoeffCr,    0, uiTmp * sizeof(*m_trCoeffCr));
        memset(m_arlCoeffCb,   0, uiTmp * sizeof(*m_arlCoeffCb));
        memset(m_arlCoeffCr,   0, uiTmp * sizeof(*m_arlCoeffCr));
        memset(m_iPCMSampleCb, 0, uiTmp * sizeof(*m_iPCMSampleCb));
        memset(m_iPCMSampleCr, 0, uiTmp * sizeof(*m_iPCMSampleCr));
    }
}

// initialize Sub partition
Void TComDataCU::initSubCU(TComDataCU* cu, UInt partUnitIdx, UInt depth, Int qp)
{
    assert(partUnitIdx < 4);

    UInt partOffset = (cu->getTotalNumPart() >> 2) * partUnitIdx;

    m_pic              = cu->getPic();
    m_slice            = m_pic->getSlice();
    m_cuAddr           = cu->getAddr();
    m_absIdxInLCU      = cu->getZorderIdxInCU() + partOffset;

    m_cuPelX           = cu->getCUPelX() + (g_maxCUWidth >> depth) * (partUnitIdx &  1);
    m_cuPelY           = cu->getCUPelY() + (g_maxCUHeight >> depth) * (partUnitIdx >> 1);

    m_totalCost         = MAX_INT64;
    m_totalDistortion  = 0;
    m_totalBits        = 0;
    m_totalBins        = 0;
    m_numPartitions     = cu->getTotalNumPart() >> 2;

    Int iSizeInUchar = sizeof(UChar) * m_numPartitions;
    Int iSizeInBool  = sizeof(Bool) * m_numPartitions;

    Int sizeInChar = sizeof(Char) * m_numPartitions;
    memset(m_qp,              qp,  sizeInChar);

    memset(m_pbMergeFlag,        0, iSizeInBool);
    memset(m_puhMergeIndex,      0, iSizeInUchar);
    memset(m_puhLumaIntraDir,    DC_IDX, iSizeInUchar);
    memset(m_puhChromaIntraDir,  0, iSizeInUchar);
    memset(m_puhInterDir,        0, iSizeInUchar);
    memset(m_trIdx,           0, iSizeInUchar);
    memset(m_transformSkip[0], 0, iSizeInUchar);
    memset(m_transformSkip[1], 0, iSizeInUchar);
    memset(m_transformSkip[2], 0, iSizeInUchar);
    memset(m_cbf[0],          0, iSizeInUchar);
    memset(m_cbf[1],          0, iSizeInUchar);
    memset(m_cbf[2],          0, iSizeInUchar);
    memset(m_depth,     depth, iSizeInUchar);

    UChar uhWidth  = g_maxCUWidth  >> depth;
    UChar uhHeight = g_maxCUHeight >> depth;
    memset(m_width,          uhWidth,  iSizeInUchar);
    memset(m_height,         uhHeight, iSizeInUchar);
    memset(m_pbIPCMFlag,        0, iSizeInBool);
    for (UInt ui = 0; ui < m_numPartitions; ui++)
    {
        m_skipFlag[ui]   = false;
        m_partSizes[ui] = SIZE_NONE;
        m_predModes[ui] = MODE_NONE;
        m_cuTransquantBypass[ui] = false;
        m_apiMVPIdx[0][ui] = -1;
        m_apiMVPIdx[1][ui] = -1;
        m_apiMVPNum[0][ui] = -1;
        m_apiMVPNum[1][ui] = -1;
    }

    UInt uiTmp = uhWidth * uhHeight;
    memset(m_trCoeffY, 0, sizeof(TCoeff) * uiTmp);
    memset(m_arlCoeffY, 0, sizeof(Int) * uiTmp);
    memset(m_iPCMSampleY, 0, sizeof(Pel) * uiTmp);
    uiTmp >>= 2;
    memset(m_trCoeffCb, 0, sizeof(TCoeff) * uiTmp);
    memset(m_trCoeffCr, 0, sizeof(TCoeff) * uiTmp);
    memset(m_arlCoeffCb, 0, sizeof(Int) * uiTmp);
    memset(m_arlCoeffCr, 0, sizeof(Int) * uiTmp);
    memset(m_iPCMSampleCb, 0, sizeof(Pel) * uiTmp);
    memset(m_iPCMSampleCr, 0, sizeof(Pel) * uiTmp);
    m_cuMvField[0].clearMvField();
    m_cuMvField[1].clearMvField();

    m_cuLeft        = cu->getCULeft();
    m_cuAbove       = cu->getCUAbove();
    m_cuAboveLeft   = cu->getCUAboveLeft();
    m_cuAboveRight  = cu->getCUAboveRight();

    m_cuColocated[0] = cu->getCUColocated(REF_PIC_LIST_0);
    m_cuColocated[1] = cu->getCUColocated(REF_PIC_LIST_1);
}

Void TComDataCU::setOutsideCUPart(UInt absPartIdx, UInt depth)
{
    UInt numPartition = m_numPartitions >> (depth << 1);
    UInt uiSizeInUchar = sizeof(UChar) * numPartition;

    UChar uhWidth  = g_maxCUWidth  >> depth;
    UChar uhHeight = g_maxCUHeight >> depth;

    memset(m_depth    + absPartIdx,     depth,  uiSizeInUchar);
    memset(m_width    + absPartIdx,     uhWidth,  uiSizeInUchar);
    memset(m_height   + absPartIdx,     uhHeight, uiSizeInUchar);
}

// --------------------------------------------------------------------------------------------------------------------
// Copy
// --------------------------------------------------------------------------------------------------------------------

Void TComDataCU::copySubCU(TComDataCU* cu, UInt absPartIdx, UInt depth)
{
    UInt uiPart = absPartIdx;

    m_pic              = cu->getPic();
    m_slice            = cu->getSlice();
    m_cuAddr           = cu->getAddr();
    m_absIdxInLCU      = absPartIdx;

    m_cuPelX           = cu->getCUPelX() + g_rasterToPelX[g_zscanToRaster[absPartIdx]];
    m_cuPelY           = cu->getCUPelY() + g_rasterToPelY[g_zscanToRaster[absPartIdx]];

    UInt width         = g_maxCUWidth  >> depth;
    UInt height        = g_maxCUHeight >> depth;

    m_skipFlag = cu->getSkipFlag()          + uiPart;

    m_qp = cu->getQP()                    + uiPart;
    m_partSizes = cu->getPartitionSize() + uiPart;
    m_predModes = cu->getPredictionMode()  + uiPart;
    m_cuTransquantBypass  = cu->getCUTransquantBypass() + uiPart;

    m_pbMergeFlag         = cu->getMergeFlag()        + uiPart;
    m_puhMergeIndex       = cu->getMergeIndex()       + uiPart;

    m_puhLumaIntraDir     = cu->getLumaIntraDir()     + uiPart;
    m_puhChromaIntraDir   = cu->getChromaIntraDir()   + uiPart;
    m_puhInterDir         = cu->getInterDir()         + uiPart;
    m_trIdx            = cu->getTransformIdx()     + uiPart;
    m_transformSkip[0] = cu->getTransformSkip(TEXT_LUMA)     + uiPart;
    m_transformSkip[1] = cu->getTransformSkip(TEXT_CHROMA_U) + uiPart;
    m_transformSkip[2] = cu->getTransformSkip(TEXT_CHROMA_V) + uiPart;

    m_cbf[0] = cu->getCbf(TEXT_LUMA)            + uiPart;
    m_cbf[1] = cu->getCbf(TEXT_CHROMA_U)        + uiPart;
    m_cbf[2] = cu->getCbf(TEXT_CHROMA_V)        + uiPart;

    m_depth = cu->getDepth()                     + uiPart;
    m_width = cu->getWidth()                     + uiPart;
    m_height = cu->getHeight()                   + uiPart;

    m_apiMVPIdx[0] = cu->getMVPIdx(REF_PIC_LIST_0)  + uiPart;
    m_apiMVPIdx[1] = cu->getMVPIdx(REF_PIC_LIST_1)  + uiPart;
    m_apiMVPNum[0] = cu->getMVPNum(REF_PIC_LIST_0)  + uiPart;
    m_apiMVPNum[1] = cu->getMVPNum(REF_PIC_LIST_1)  + uiPart;

    m_pbIPCMFlag         = cu->getIPCMFlag()        + uiPart;

    m_cuAboveLeft      = cu->getCUAboveLeft();
    m_cuAboveRight     = cu->getCUAboveRight();
    m_cuAbove          = cu->getCUAbove();
    m_cuLeft           = cu->getCULeft();

    m_cuColocated[0] = cu->getCUColocated(REF_PIC_LIST_0);
    m_cuColocated[1] = cu->getCUColocated(REF_PIC_LIST_1);

    UInt uiTmp = width * height;
    UInt uiMaxCuWidth = cu->getSlice()->getSPS()->getMaxCUWidth();
    UInt uiMaxCuHeight = cu->getSlice()->getSPS()->getMaxCUHeight();

    UInt uiCoffOffset = uiMaxCuWidth * uiMaxCuHeight * absPartIdx / cu->getPic()->getNumPartInCU();

    m_trCoeffY = cu->getCoeffY() + uiCoffOffset;
    m_arlCoeffY = cu->getArlCoeffY() + uiCoffOffset;
    m_iPCMSampleY = cu->getPCMSampleY() + uiCoffOffset;

    uiTmp >>= 2;
    uiCoffOffset >>= 2;
    m_trCoeffCb = cu->getCoeffCb() + uiCoffOffset;
    m_trCoeffCr = cu->getCoeffCr() + uiCoffOffset;
    m_arlCoeffCb = cu->getArlCoeffCb() + uiCoffOffset;
    m_arlCoeffCr = cu->getArlCoeffCr() + uiCoffOffset;
    m_iPCMSampleCb = cu->getPCMSampleCb() + uiCoffOffset;
    m_iPCMSampleCr = cu->getPCMSampleCr() + uiCoffOffset;

    m_cuMvField[0].linkToWithOffset(cu->getCUMvField(REF_PIC_LIST_0), uiPart);
    m_cuMvField[1].linkToWithOffset(cu->getCUMvField(REF_PIC_LIST_1), uiPart);
}

// Copy inter prediction info from the biggest CU
Void TComDataCU::copyInterPredInfoFrom(TComDataCU* cu, UInt absPartIdx, RefPicList picList)
{
    m_pic              = cu->getPic();
    m_slice            = cu->getSlice();
    m_cuAddr           = cu->getAddr();
    m_absIdxInLCU      = absPartIdx;

    Int iRastPartIdx     = g_zscanToRaster[absPartIdx];
    m_cuPelX           = cu->getCUPelX() + m_pic->getMinCUWidth() * (iRastPartIdx % m_pic->getNumPartInWidth());
    m_cuPelY           = cu->getCUPelY() + m_pic->getMinCUHeight() * (iRastPartIdx / m_pic->getNumPartInWidth());

    m_cuAboveLeft      = cu->getCUAboveLeft();
    m_cuAboveRight     = cu->getCUAboveRight();
    m_cuAbove          = cu->getCUAbove();
    m_cuLeft           = cu->getCULeft();

    m_cuColocated[0]  = cu->getCUColocated(REF_PIC_LIST_0);
    m_cuColocated[1]  = cu->getCUColocated(REF_PIC_LIST_1);

    m_skipFlag           = cu->getSkipFlag()             + absPartIdx;

    m_partSizes         = cu->getPartitionSize()        + absPartIdx;
    m_predModes         = cu->getPredictionMode()        + absPartIdx;
    m_cuTransquantBypass = cu->getCUTransquantBypass()    + absPartIdx;
    m_puhInterDir        = cu->getInterDir()        + absPartIdx;

    m_depth           = cu->getDepth()                + absPartIdx;
    m_width           = cu->getWidth()                + absPartIdx;
    m_height          = cu->getHeight()                + absPartIdx;

    m_pbMergeFlag        = cu->getMergeFlag()             + absPartIdx;
    m_puhMergeIndex      = cu->getMergeIndex()            + absPartIdx;

    m_apiMVPIdx[picList] = cu->getMVPIdx(picList) + absPartIdx;
    m_apiMVPNum[picList] = cu->getMVPNum(picList) + absPartIdx;

    m_cuMvField[picList].linkToWithOffset(cu->getCUMvField(picList), absPartIdx);
}

// Copy small CU to bigger CU.
// One of quarter parts overwritten by predicted sub part.
Void TComDataCU::copyPartFrom(TComDataCU* cu, UInt partUnitIdx, UInt depth, Bool isRDObasedAnalysis)
{
    assert(partUnitIdx < 4);
    if (isRDObasedAnalysis)
        m_totalCost         += cu->getTotalCost();

    m_totalDistortion  += cu->getTotalDistortion();
    m_totalBits        += cu->getTotalBits();

    UInt offset         = cu->getTotalNumPart() * partUnitIdx;

    UInt numPartition = cu->getTotalNumPart();
    Int iSizeInUchar  = sizeof(UChar) * numPartition;
    Int iSizeInBool   = sizeof(Bool) * numPartition;

    Int sizeInChar  = sizeof(Char) * numPartition;
    memcpy(m_skipFlag   + offset, cu->getSkipFlag(),       sizeof(*m_skipFlag)   * numPartition);
    memcpy(m_qp       + offset, cu->getQP(),             sizeInChar);
    memcpy(m_partSizes + offset, cu->getPartitionSize(),  sizeof(*m_partSizes) * numPartition);
    memcpy(m_predModes + offset, cu->getPredictionMode(), sizeof(*m_predModes) * numPartition);
    memcpy(m_cuTransquantBypass + offset, cu->getCUTransquantBypass(), sizeof(*m_cuTransquantBypass) * numPartition);
    memcpy(m_pbMergeFlag         + offset, cu->getMergeFlag(),         iSizeInBool);
    memcpy(m_puhMergeIndex       + offset, cu->getMergeIndex(),        iSizeInUchar);
    memcpy(m_puhLumaIntraDir     + offset, cu->getLumaIntraDir(),      iSizeInUchar);
    memcpy(m_puhChromaIntraDir   + offset, cu->getChromaIntraDir(),    iSizeInUchar);
    memcpy(m_puhInterDir         + offset, cu->getInterDir(),          iSizeInUchar);
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

    memcpy(m_apiMVPIdx[0] + offset, cu->getMVPIdx(REF_PIC_LIST_0), iSizeInUchar);
    memcpy(m_apiMVPIdx[1] + offset, cu->getMVPIdx(REF_PIC_LIST_1), iSizeInUchar);
    memcpy(m_apiMVPNum[0] + offset, cu->getMVPNum(REF_PIC_LIST_0), iSizeInUchar);
    memcpy(m_apiMVPNum[1] + offset, cu->getMVPNum(REF_PIC_LIST_1), iSizeInUchar);

    memcpy(m_pbIPCMFlag + offset, cu->getIPCMFlag(), iSizeInBool);

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
    memcpy(m_arlCoeffY  + uiTmp2, cu->getArlCoeffY(),  sizeof(Int) * uiTmp);
    memcpy(m_iPCMSampleY + uiTmp2, cu->getPCMSampleY(), sizeof(Pel) * uiTmp);

    uiTmp >>= 2;
    uiTmp2 >>= 2;
    memcpy(m_trCoeffCb + uiTmp2, cu->getCoeffCb(), sizeof(TCoeff) * uiTmp);
    memcpy(m_trCoeffCr + uiTmp2, cu->getCoeffCr(), sizeof(TCoeff) * uiTmp);
    memcpy(m_arlCoeffCb + uiTmp2, cu->getArlCoeffCb(), sizeof(Int) * uiTmp);
    memcpy(m_arlCoeffCr + uiTmp2, cu->getArlCoeffCr(), sizeof(Int) * uiTmp);
    memcpy(m_iPCMSampleCb + uiTmp2, cu->getPCMSampleCb(), sizeof(Pel) * uiTmp);
    memcpy(m_iPCMSampleCr + uiTmp2, cu->getPCMSampleCr(), sizeof(Pel) * uiTmp);
    m_totalBins += cu->getTotalBins();
}

// Copy current predicted part to a CU in picture.
// It is used to predict for next part
Void TComDataCU::copyToPic(UChar uhDepth)
{
    TComDataCU* rpcCU = m_pic->getCU(m_cuAddr);

    rpcCU->getTotalCost()       = m_totalCost;
    rpcCU->getTotalDistortion() = m_totalDistortion;
    rpcCU->getTotalBits()       = m_totalBits;

    Int iSizeInUchar  = sizeof(UChar) * m_numPartitions;
    Int iSizeInBool   = sizeof(Bool) * m_numPartitions;

    Int sizeInChar  = sizeof(Char) * m_numPartitions;

    memcpy(rpcCU->getSkipFlag() + m_absIdxInLCU, m_skipFlag, sizeof(*m_skipFlag) * m_numPartitions);

    memcpy(rpcCU->getQP() + m_absIdxInLCU, m_qp, sizeInChar);

    memcpy(rpcCU->getPartitionSize()  + m_absIdxInLCU, m_partSizes, sizeof(*m_partSizes) * m_numPartitions);
    memcpy(rpcCU->getPredictionMode() + m_absIdxInLCU, m_predModes, sizeof(*m_predModes) * m_numPartitions);
    memcpy(rpcCU->getCUTransquantBypass() + m_absIdxInLCU, m_cuTransquantBypass, sizeof(*m_cuTransquantBypass) * m_numPartitions);
    memcpy(rpcCU->getMergeFlag()         + m_absIdxInLCU, m_pbMergeFlag,         iSizeInBool);
    memcpy(rpcCU->getMergeIndex()        + m_absIdxInLCU, m_puhMergeIndex,       iSizeInUchar);
    memcpy(rpcCU->getLumaIntraDir()      + m_absIdxInLCU, m_puhLumaIntraDir,     iSizeInUchar);
    memcpy(rpcCU->getChromaIntraDir()    + m_absIdxInLCU, m_puhChromaIntraDir,   iSizeInUchar);
    memcpy(rpcCU->getInterDir()          + m_absIdxInLCU, m_puhInterDir,         iSizeInUchar);
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

    memcpy(rpcCU->getMVPIdx(REF_PIC_LIST_0) + m_absIdxInLCU, m_apiMVPIdx[0], iSizeInUchar);
    memcpy(rpcCU->getMVPIdx(REF_PIC_LIST_1) + m_absIdxInLCU, m_apiMVPIdx[1], iSizeInUchar);
    memcpy(rpcCU->getMVPNum(REF_PIC_LIST_0) + m_absIdxInLCU, m_apiMVPNum[0], iSizeInUchar);
    memcpy(rpcCU->getMVPNum(REF_PIC_LIST_1) + m_absIdxInLCU, m_apiMVPNum[1], iSizeInUchar);

    m_cuMvField[0].copyTo(rpcCU->getCUMvField(REF_PIC_LIST_0), m_absIdxInLCU);
    m_cuMvField[1].copyTo(rpcCU->getCUMvField(REF_PIC_LIST_1), m_absIdxInLCU);

    memcpy(rpcCU->getIPCMFlag() + m_absIdxInLCU, m_pbIPCMFlag,         iSizeInBool);

    UInt uiTmp  = (g_maxCUWidth * g_maxCUHeight) >> (uhDepth << 1);
    UInt uiTmp2 = m_absIdxInLCU * m_pic->getMinCUWidth() * m_pic->getMinCUHeight();
    memcpy(rpcCU->getCoeffY()  + uiTmp2, m_trCoeffY,  sizeof(TCoeff) * uiTmp);
    memcpy(rpcCU->getArlCoeffY()  + uiTmp2, m_arlCoeffY,  sizeof(Int) * uiTmp);
    memcpy(rpcCU->getPCMSampleY() + uiTmp2, m_iPCMSampleY, sizeof(Pel) * uiTmp);

    uiTmp >>= 2;
    uiTmp2 >>= 2;
    memcpy(rpcCU->getCoeffCb() + uiTmp2, m_trCoeffCb, sizeof(TCoeff) * uiTmp);
    memcpy(rpcCU->getCoeffCr() + uiTmp2, m_trCoeffCr, sizeof(TCoeff) * uiTmp);
    memcpy(rpcCU->getArlCoeffCb() + uiTmp2, m_arlCoeffCb, sizeof(Int) * uiTmp);
    memcpy(rpcCU->getArlCoeffCr() + uiTmp2, m_arlCoeffCr, sizeof(Int) * uiTmp);
    memcpy(rpcCU->getPCMSampleCb() + uiTmp2, m_iPCMSampleCb, sizeof(Pel) * uiTmp);
    memcpy(rpcCU->getPCMSampleCr() + uiTmp2, m_iPCMSampleCr, sizeof(Pel) * uiTmp);
    rpcCU->getTotalBins() = m_totalBins;
}

Void TComDataCU::copyToPic(UChar uhDepth, UInt partIdx, UInt partDepth)
{
    TComDataCU*   rpcCU       = m_pic->getCU(m_cuAddr);
    UInt          uiQNumPart  = m_numPartitions >> (partDepth << 1);

    UInt uiPartStart          = partIdx * uiQNumPart;
    UInt partOffset         = m_absIdxInLCU + uiPartStart;

    rpcCU->getTotalCost()       = m_totalCost;
    rpcCU->getTotalDistortion() = m_totalDistortion;
    rpcCU->getTotalBits()       = m_totalBits;

    Int iSizeInUchar  = sizeof(UChar) * uiQNumPart;
    Int iSizeInBool   = sizeof(Bool) * uiQNumPart;

    Int sizeInChar  = sizeof(Char) * uiQNumPart;
    memcpy(rpcCU->getSkipFlag()       + partOffset, m_skipFlag,   sizeof(*m_skipFlag)   * uiQNumPart);

    memcpy(rpcCU->getQP() + partOffset, m_qp, sizeInChar);
    memcpy(rpcCU->getPartitionSize()  + partOffset, m_partSizes, sizeof(*m_partSizes) * uiQNumPart);
    memcpy(rpcCU->getPredictionMode() + partOffset, m_predModes, sizeof(*m_predModes) * uiQNumPart);
    memcpy(rpcCU->getCUTransquantBypass() + partOffset, m_cuTransquantBypass, sizeof(*m_cuTransquantBypass) * uiQNumPart);
    memcpy(rpcCU->getMergeFlag()         + partOffset, m_pbMergeFlag,         iSizeInBool);
    memcpy(rpcCU->getMergeIndex()        + partOffset, m_puhMergeIndex,       iSizeInUchar);
    memcpy(rpcCU->getLumaIntraDir()      + partOffset, m_puhLumaIntraDir,     iSizeInUchar);
    memcpy(rpcCU->getChromaIntraDir()    + partOffset, m_puhChromaIntraDir,   iSizeInUchar);
    memcpy(rpcCU->getInterDir()          + partOffset, m_puhInterDir,         iSizeInUchar);
    memcpy(rpcCU->getTransformIdx()      + partOffset, m_trIdx,            iSizeInUchar);
    memcpy(rpcCU->getTransformSkip(TEXT_LUMA)     + partOffset, m_transformSkip[0], iSizeInUchar);
    memcpy(rpcCU->getTransformSkip(TEXT_CHROMA_U) + partOffset, m_transformSkip[1], iSizeInUchar);
    memcpy(rpcCU->getTransformSkip(TEXT_CHROMA_V) + partOffset, m_transformSkip[2], iSizeInUchar);
    memcpy(rpcCU->getCbf(TEXT_LUMA)     + partOffset, m_cbf[0], iSizeInUchar);
    memcpy(rpcCU->getCbf(TEXT_CHROMA_U) + partOffset, m_cbf[1], iSizeInUchar);
    memcpy(rpcCU->getCbf(TEXT_CHROMA_V) + partOffset, m_cbf[2], iSizeInUchar);

    memcpy(rpcCU->getDepth()  + partOffset, m_depth,  iSizeInUchar);
    memcpy(rpcCU->getWidth()  + partOffset, m_width,  iSizeInUchar);
    memcpy(rpcCU->getHeight() + partOffset, m_height, iSizeInUchar);

    memcpy(rpcCU->getMVPIdx(REF_PIC_LIST_0) + partOffset, m_apiMVPIdx[0], iSizeInUchar);
    memcpy(rpcCU->getMVPIdx(REF_PIC_LIST_1) + partOffset, m_apiMVPIdx[1], iSizeInUchar);
    memcpy(rpcCU->getMVPNum(REF_PIC_LIST_0) + partOffset, m_apiMVPNum[0], iSizeInUchar);
    memcpy(rpcCU->getMVPNum(REF_PIC_LIST_1) + partOffset, m_apiMVPNum[1], iSizeInUchar);
    m_cuMvField[0].copyTo(rpcCU->getCUMvField(REF_PIC_LIST_0), m_absIdxInLCU, uiPartStart, uiQNumPart);
    m_cuMvField[1].copyTo(rpcCU->getCUMvField(REF_PIC_LIST_1), m_absIdxInLCU, uiPartStart, uiQNumPart);

    memcpy(rpcCU->getIPCMFlag() + partOffset, m_pbIPCMFlag,         iSizeInBool);

    UInt uiTmp  = (g_maxCUWidth * g_maxCUHeight) >> ((uhDepth + partDepth) << 1);
    UInt uiTmp2 = partOffset * m_pic->getMinCUWidth() * m_pic->getMinCUHeight();
    memcpy(rpcCU->getCoeffY()  + uiTmp2, m_trCoeffY,  sizeof(TCoeff) * uiTmp);
    memcpy(rpcCU->getArlCoeffY()  + uiTmp2, m_arlCoeffY,  sizeof(Int) * uiTmp);
    memcpy(rpcCU->getPCMSampleY() + uiTmp2, m_iPCMSampleY, sizeof(Pel) * uiTmp);

    uiTmp >>= 2;
    uiTmp2 >>= 2;
    memcpy(rpcCU->getCoeffCb() + uiTmp2, m_trCoeffCb, sizeof(TCoeff) * uiTmp);
    memcpy(rpcCU->getCoeffCr() + uiTmp2, m_trCoeffCr, sizeof(TCoeff) * uiTmp);
    memcpy(rpcCU->getArlCoeffCb() + uiTmp2, m_arlCoeffCb, sizeof(Int) * uiTmp);
    memcpy(rpcCU->getArlCoeffCr() + uiTmp2, m_arlCoeffCr, sizeof(Int) * uiTmp);

    memcpy(rpcCU->getPCMSampleCb() + uiTmp2, m_iPCMSampleCb, sizeof(Pel) * uiTmp);
    memcpy(rpcCU->getPCMSampleCr() + uiTmp2, m_iPCMSampleCr, sizeof(Pel) * uiTmp);
    rpcCU->getTotalBins() = m_totalBins;
}

// --------------------------------------------------------------------------------------------------------------------
// Other public functions
// --------------------------------------------------------------------------------------------------------------------

TComDataCU* TComDataCU::getPULeft(UInt& uiLPartUnitIdx,
                                  UInt  uiCurrPartUnitIdx,
                                  Bool  bEnforceSliceRestriction,
                                  Bool  bEnforceTileRestriction)
{
    UInt absPartIdx       = g_zscanToRaster[uiCurrPartUnitIdx];
    UInt uiAbsZorderCUIdx   = g_zscanToRaster[m_absIdxInLCU];
    UInt uiNumPartInCUWidth = m_pic->getNumPartInWidth();

    if (!RasterAddress::isZeroCol(absPartIdx, uiNumPartInCUWidth))
    {
        uiLPartUnitIdx = g_rasterToZscan[absPartIdx - 1];
        if (RasterAddress::isEqualCol(absPartIdx, uiAbsZorderCUIdx, uiNumPartInCUWidth))
        {
            return m_pic->getCU(getAddr());
        }
        else
        {
            uiLPartUnitIdx -= m_absIdxInLCU;
            return this;
        }
    }

    uiLPartUnitIdx = g_rasterToZscan[absPartIdx + uiNumPartInCUWidth - 1];

    if ((bEnforceSliceRestriction && (m_cuLeft == NULL || m_cuLeft->getSlice() == NULL))
        ||
        (bEnforceTileRestriction && (m_cuLeft == NULL || m_cuLeft->getSlice() == NULL))
        )
    {
        return NULL;
    }
    return m_cuLeft;
}

TComDataCU* TComDataCU::getPUAbove(UInt& uiAPartUnitIdx,
                                   UInt  uiCurrPartUnitIdx,
                                   Bool  bEnforceSliceRestriction,
                                   Bool  planarAtLCUBoundary,
                                   Bool  bEnforceTileRestriction)
{
    UInt absPartIdx       = g_zscanToRaster[uiCurrPartUnitIdx];
    UInt uiAbsZorderCUIdx   = g_zscanToRaster[m_absIdxInLCU];
    UInt uiNumPartInCUWidth = m_pic->getNumPartInWidth();

    if (!RasterAddress::isZeroRow(absPartIdx, uiNumPartInCUWidth))
    {
        uiAPartUnitIdx = g_rasterToZscan[absPartIdx - uiNumPartInCUWidth];
        if (RasterAddress::isEqualRow(absPartIdx, uiAbsZorderCUIdx, uiNumPartInCUWidth))
        {
            return m_pic->getCU(getAddr());
        }
        else
        {
            uiAPartUnitIdx -= m_absIdxInLCU;
            return this;
        }
    }

    if (planarAtLCUBoundary)
    {
        return NULL;
    }

    uiAPartUnitIdx = g_rasterToZscan[absPartIdx + m_pic->getNumPartInCU() - uiNumPartInCUWidth];

    if ((bEnforceSliceRestriction && (m_cuAbove == NULL || m_cuAbove->getSlice() == NULL))
        ||
        (bEnforceTileRestriction && (m_cuAbove == NULL || m_cuAbove->getSlice() == NULL))
        )
    {
        return NULL;
    }
    return m_cuAbove;
}

TComDataCU* TComDataCU::getPUAboveLeft(UInt& uiALPartUnitIdx, UInt uiCurrPartUnitIdx, Bool bEnforceSliceRestriction)
{
    UInt absPartIdx       = g_zscanToRaster[uiCurrPartUnitIdx];
    UInt uiAbsZorderCUIdx   = g_zscanToRaster[m_absIdxInLCU];
    UInt uiNumPartInCUWidth = m_pic->getNumPartInWidth();

    if (!RasterAddress::isZeroCol(absPartIdx, uiNumPartInCUWidth))
    {
        if (!RasterAddress::isZeroRow(absPartIdx, uiNumPartInCUWidth))
        {
            uiALPartUnitIdx = g_rasterToZscan[absPartIdx - uiNumPartInCUWidth - 1];
            if (RasterAddress::isEqualRowOrCol(absPartIdx, uiAbsZorderCUIdx, uiNumPartInCUWidth))
            {
                return m_pic->getCU(getAddr());
            }
            else
            {
                uiALPartUnitIdx -= m_absIdxInLCU;
                return this;
            }
        }
        uiALPartUnitIdx = g_rasterToZscan[absPartIdx + getPic()->getNumPartInCU() - uiNumPartInCUWidth - 1];
        if ((bEnforceSliceRestriction && (m_cuAbove == NULL || m_cuAbove->getSlice() == NULL))
            )
        {
            return NULL;
        }
        return m_cuAbove;
    }

    if (!RasterAddress::isZeroRow(absPartIdx, uiNumPartInCUWidth))
    {
        uiALPartUnitIdx = g_rasterToZscan[absPartIdx - 1];
        if ((bEnforceSliceRestriction && (m_cuLeft == NULL || m_cuLeft->getSlice() == NULL))
            )
        {
            return NULL;
        }
        return m_cuLeft;
    }

    uiALPartUnitIdx = g_rasterToZscan[m_pic->getNumPartInCU() - 1];
    if ((bEnforceSliceRestriction && (m_cuAboveLeft == NULL || m_cuAboveLeft->getSlice() == NULL))
        )
    {
        return NULL;
    }
    return m_cuAboveLeft;
}

TComDataCU* TComDataCU::getPUAboveRight(UInt& uiARPartUnitIdx, UInt uiCurrPartUnitIdx, Bool bEnforceSliceRestriction)
{
    UInt uiAbsPartIdxRT     = g_zscanToRaster[uiCurrPartUnitIdx];
    UInt uiAbsZorderCUIdx   = g_zscanToRaster[m_absIdxInLCU] + m_width[0] / m_pic->getMinCUWidth() - 1;
    UInt uiNumPartInCUWidth = m_pic->getNumPartInWidth();

    if ((m_pic->getCU(m_cuAddr)->getCUPelX() + g_rasterToPelX[uiAbsPartIdxRT] + m_pic->getMinCUWidth()) >= m_slice->getSPS()->getPicWidthInLumaSamples())
    {
        uiARPartUnitIdx = MAX_UINT;
        return NULL;
    }

    if (RasterAddress::lessThanCol(uiAbsPartIdxRT, uiNumPartInCUWidth - 1, uiNumPartInCUWidth))
    {
        if (!RasterAddress::isZeroRow(uiAbsPartIdxRT, uiNumPartInCUWidth))
        {
            if (uiCurrPartUnitIdx > g_rasterToZscan[uiAbsPartIdxRT - uiNumPartInCUWidth + 1])
            {
                uiARPartUnitIdx = g_rasterToZscan[uiAbsPartIdxRT - uiNumPartInCUWidth + 1];
                if (RasterAddress::isEqualRowOrCol(uiAbsPartIdxRT, uiAbsZorderCUIdx, uiNumPartInCUWidth))
                {
                    return m_pic->getCU(getAddr());
                }
                else
                {
                    uiARPartUnitIdx -= m_absIdxInLCU;
                    return this;
                }
            }
            uiARPartUnitIdx = MAX_UINT;
            return NULL;
        }
        uiARPartUnitIdx = g_rasterToZscan[uiAbsPartIdxRT + m_pic->getNumPartInCU() - uiNumPartInCUWidth + 1];
        if ((bEnforceSliceRestriction && (m_cuAbove == NULL || m_cuAbove->getSlice() == NULL))
            )
        {
            return NULL;
        }
        return m_cuAbove;
    }

    if (!RasterAddress::isZeroRow(uiAbsPartIdxRT, uiNumPartInCUWidth))
    {
        uiARPartUnitIdx = MAX_UINT;
        return NULL;
    }

    uiARPartUnitIdx = g_rasterToZscan[m_pic->getNumPartInCU() - uiNumPartInCUWidth];
    if ((bEnforceSliceRestriction && (m_cuAboveRight == NULL || m_cuAboveRight->getSlice() == NULL ||
                                      (m_cuAboveRight->getAddr()) > getAddr()
                                      ))
        )
    {
        return NULL;
    }
    return m_cuAboveRight;
}

TComDataCU* TComDataCU::getPUBelowLeft(UInt& uiBLPartUnitIdx, UInt uiCurrPartUnitIdx, Bool bEnforceSliceRestriction)
{
    UInt uiAbsPartIdxLB     = g_zscanToRaster[uiCurrPartUnitIdx];
    UInt uiAbsZorderCUIdxLB = g_zscanToRaster[m_absIdxInLCU] + (m_height[0] / m_pic->getMinCUHeight() - 1) * m_pic->getNumPartInWidth();
    UInt uiNumPartInCUWidth = m_pic->getNumPartInWidth();

    if ((m_pic->getCU(m_cuAddr)->getCUPelY() + g_rasterToPelY[uiAbsPartIdxLB] + m_pic->getMinCUHeight()) >= m_slice->getSPS()->getPicHeightInLumaSamples())
    {
        uiBLPartUnitIdx = MAX_UINT;
        return NULL;
    }

    if (RasterAddress::lessThanRow(uiAbsPartIdxLB, m_pic->getNumPartInHeight() - 1, uiNumPartInCUWidth))
    {
        if (!RasterAddress::isZeroCol(uiAbsPartIdxLB, uiNumPartInCUWidth))
        {
            if (uiCurrPartUnitIdx > g_rasterToZscan[uiAbsPartIdxLB + uiNumPartInCUWidth - 1])
            {
                uiBLPartUnitIdx = g_rasterToZscan[uiAbsPartIdxLB + uiNumPartInCUWidth - 1];
                if (RasterAddress::isEqualRowOrCol(uiAbsPartIdxLB, uiAbsZorderCUIdxLB, uiNumPartInCUWidth))
                {
                    return m_pic->getCU(getAddr());
                }
                else
                {
                    uiBLPartUnitIdx -= m_absIdxInLCU;
                    return this;
                }
            }
            uiBLPartUnitIdx = MAX_UINT;
            return NULL;
        }
        uiBLPartUnitIdx = g_rasterToZscan[uiAbsPartIdxLB + uiNumPartInCUWidth * 2 - 1];
        if ((bEnforceSliceRestriction && (m_cuLeft == NULL || m_cuLeft->getSlice() == NULL))
            )
        {
            return NULL;
        }
        return m_cuLeft;
    }

    uiBLPartUnitIdx = MAX_UINT;
    return NULL;
}

TComDataCU* TComDataCU::getPUBelowLeftAdi(UInt& uiBLPartUnitIdx,  UInt uiCurrPartUnitIdx, UInt uiPartUnitOffset, Bool bEnforceSliceRestriction)
{
    UInt uiAbsPartIdxLB     = g_zscanToRaster[uiCurrPartUnitIdx];
    UInt uiAbsZorderCUIdxLB = g_zscanToRaster[m_absIdxInLCU] + ((m_height[0] / m_pic->getMinCUHeight()) - 1) * m_pic->getNumPartInWidth();
    UInt uiNumPartInCUWidth = m_pic->getNumPartInWidth();

    if ((m_pic->getCU(m_cuAddr)->getCUPelY() + g_rasterToPelY[uiAbsPartIdxLB] + (m_pic->getPicSym()->getMinCUHeight() * uiPartUnitOffset)) >= m_slice->getSPS()->getPicHeightInLumaSamples())
    {
        uiBLPartUnitIdx = MAX_UINT;
        return NULL;
    }

    if (RasterAddress::lessThanRow(uiAbsPartIdxLB, m_pic->getNumPartInHeight() - uiPartUnitOffset, uiNumPartInCUWidth))
    {
        if (!RasterAddress::isZeroCol(uiAbsPartIdxLB, uiNumPartInCUWidth))
        {
            if (uiCurrPartUnitIdx > g_rasterToZscan[uiAbsPartIdxLB + uiPartUnitOffset * uiNumPartInCUWidth - 1])
            {
                uiBLPartUnitIdx = g_rasterToZscan[uiAbsPartIdxLB + uiPartUnitOffset * uiNumPartInCUWidth - 1];
                if (RasterAddress::isEqualRowOrCol(uiAbsPartIdxLB, uiAbsZorderCUIdxLB, uiNumPartInCUWidth))
                {
                    return m_pic->getCU(getAddr());
                }
                else
                {
                    uiBLPartUnitIdx -= m_absIdxInLCU;
                    return this;
                }
            }
            uiBLPartUnitIdx = MAX_UINT;
            return NULL;
        }
        uiBLPartUnitIdx = g_rasterToZscan[uiAbsPartIdxLB + (1 + uiPartUnitOffset) * uiNumPartInCUWidth - 1];
        if ((bEnforceSliceRestriction && (m_cuLeft == NULL || m_cuLeft->getSlice() == NULL))
            )
        {
            return NULL;
        }
        return m_cuLeft;
    }

    uiBLPartUnitIdx = MAX_UINT;
    return NULL;
}

TComDataCU* TComDataCU::getPUAboveRightAdi(UInt& uiARPartUnitIdx, UInt uiCurrPartUnitIdx, UInt uiPartUnitOffset, Bool bEnforceSliceRestriction)
{
    UInt uiAbsPartIdxRT     = g_zscanToRaster[uiCurrPartUnitIdx];
    UInt uiAbsZorderCUIdx   = g_zscanToRaster[m_absIdxInLCU] + (m_width[0] / m_pic->getMinCUWidth()) - 1;
    UInt uiNumPartInCUWidth = m_pic->getNumPartInWidth();

    if ((m_pic->getCU(m_cuAddr)->getCUPelX() + g_rasterToPelX[uiAbsPartIdxRT] + (m_pic->getPicSym()->getMinCUHeight() * uiPartUnitOffset)) >= m_slice->getSPS()->getPicWidthInLumaSamples())
    {
        uiARPartUnitIdx = MAX_UINT;
        return NULL;
    }

    if (RasterAddress::lessThanCol(uiAbsPartIdxRT, uiNumPartInCUWidth - uiPartUnitOffset, uiNumPartInCUWidth))
    {
        if (!RasterAddress::isZeroRow(uiAbsPartIdxRT, uiNumPartInCUWidth))
        {
            if (uiCurrPartUnitIdx > g_rasterToZscan[uiAbsPartIdxRT - uiNumPartInCUWidth + uiPartUnitOffset])
            {
                uiARPartUnitIdx = g_rasterToZscan[uiAbsPartIdxRT - uiNumPartInCUWidth + uiPartUnitOffset];
                if (RasterAddress::isEqualRowOrCol(uiAbsPartIdxRT, uiAbsZorderCUIdx, uiNumPartInCUWidth))
                {
                    return m_pic->getCU(getAddr());
                }
                else
                {
                    uiARPartUnitIdx -= m_absIdxInLCU;
                    return this;
                }
            }
            uiARPartUnitIdx = MAX_UINT;
            return NULL;
        }
        uiARPartUnitIdx = g_rasterToZscan[uiAbsPartIdxRT + m_pic->getNumPartInCU() - uiNumPartInCUWidth + uiPartUnitOffset];
        if ((bEnforceSliceRestriction && (m_cuAbove == NULL || m_cuAbove->getSlice() == NULL))
            )
        {
            return NULL;
        }
        return m_cuAbove;
    }

    if (!RasterAddress::isZeroRow(uiAbsPartIdxRT, uiNumPartInCUWidth))
    {
        uiARPartUnitIdx = MAX_UINT;
        return NULL;
    }

    uiARPartUnitIdx = g_rasterToZscan[m_pic->getNumPartInCU() - uiNumPartInCUWidth + uiPartUnitOffset - 1];
    if ((bEnforceSliceRestriction && (m_cuAboveRight == NULL || m_cuAboveRight->getSlice() == NULL ||
                                      (m_cuAboveRight->getAddr()) > getAddr()))
        )
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
TComDataCU* TComDataCU::getQpMinCuLeft(UInt& uiLPartUnitIdx, UInt uiCurrAbsIdxInLCU)
{
    UInt numPartInCUWidth = m_pic->getNumPartInWidth();
    UInt absZorderQpMinCUIdx = (uiCurrAbsIdxInLCU >> ((g_maxCUDepth - getSlice()->getPPS()->getMaxCuDQPDepth()) << 1)) << ((g_maxCUDepth - getSlice()->getPPS()->getMaxCuDQPDepth()) << 1);
    UInt absRorderQpMinCUIdx = g_zscanToRaster[absZorderQpMinCUIdx];

    // check for left LCU boundary
    if (RasterAddress::isZeroCol(absRorderQpMinCUIdx, numPartInCUWidth))
    {
        return NULL;
    }

    // get index of left-CU relative to top-left corner of current quantization group
    uiLPartUnitIdx = g_rasterToZscan[absRorderQpMinCUIdx - 1];

    // return pointer to current LCU
    return m_pic->getCU(getAddr());
}

/** Get Above QpMinCu
*\param   aPartUnitIdx
*\param   currAbsIdxInLCU
*\returns TComDataCU*   point of TComDataCU of above QpMinCu
*/
TComDataCU* TComDataCU::getQpMinCuAbove(UInt& aPartUnitIdx, UInt currAbsIdxInLCU)
{
    UInt numPartInCUWidth = m_pic->getNumPartInWidth();
    UInt absZorderQpMinCUIdx = (currAbsIdxInLCU >> ((g_maxCUDepth - getSlice()->getPPS()->getMaxCuDQPDepth()) << 1)) << ((g_maxCUDepth - getSlice()->getPPS()->getMaxCuDQPDepth()) << 1);
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
*\returns Char   reference QP value
*/
Char TComDataCU::getRefQP(UInt uiCurrAbsIdxInLCU)
{
    UInt        lPartIdx = 0, aPartIdx = 0;
    TComDataCU* cULeft  = getQpMinCuLeft(lPartIdx, m_absIdxInLCU + uiCurrAbsIdxInLCU);
    TComDataCU* cUAbove = getQpMinCuAbove(aPartIdx, m_absIdxInLCU + uiCurrAbsIdxInLCU);

    return ((cULeft ? cULeft->getQP(lPartIdx) : getLastCodedQP(uiCurrAbsIdxInLCU)) + (cUAbove ? cUAbove->getQP(aPartIdx) : getLastCodedQP(uiCurrAbsIdxInLCU)) + 1) >> 1;
}

Int TComDataCU::getLastValidPartIdx(Int iAbsPartIdx)
{
    Int iLastValidPartIdx = iAbsPartIdx - 1;

    while (iLastValidPartIdx >= 0
           && getPredictionMode(iLastValidPartIdx) == MODE_NONE)
    {
        UInt depth = getDepth(iLastValidPartIdx);
        iLastValidPartIdx -= m_numPartitions >> (depth << 1);
    }

    return iLastValidPartIdx;
}

Char TComDataCU::getLastCodedQP(UInt absPartIdx)
{
    UInt uiQUPartIdxMask = ~((1 << ((g_maxCUDepth - getSlice()->getPPS()->getMaxCuDQPDepth()) << 1)) - 1);
    Int iLastValidPartIdx = getLastValidPartIdx(absPartIdx & uiQUPartIdxMask);

    if (iLastValidPartIdx >= 0)
    {
        return getQP(iLastValidPartIdx);
    }
    else
    {
        if (getZorderIdxInCU() > 0)
        {
            return getPic()->getCU(getAddr())->getLastCodedQP(getZorderIdxInCU());
        }
        else if (getAddr() > 0
                 && !(getSlice()->getPPS()->getEntropyCodingSyncEnabledFlag() && getAddr() % getPic()->getFrameWidthInCU() == 0))
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
Bool TComDataCU::isLosslessCoded(UInt absPartIdx)
{
    return getSlice()->getPPS()->getTransquantBypassEnableFlag() && getCUTransquantBypass(absPartIdx);
}

/** Get allowed chroma intra modes
*\param   absPartIdx
*\param   uiModeList  pointer to chroma intra modes array
*\returns
*- fill uiModeList with chroma intra modes
*/
Void TComDataCU::getAllowedChromaDir(UInt absPartIdx, UInt* uiModeList)
{
    uiModeList[0] = PLANAR_IDX;
    uiModeList[1] = VER_IDX;
    uiModeList[2] = HOR_IDX;
    uiModeList[3] = DC_IDX;
    uiModeList[4] = DM_CHROMA_IDX;

    UInt uiLumaMode = getLumaIntraDir(absPartIdx);

    for (Int i = 0; i < NUM_CHROMA_MODE - 1; i++)
    {
        if (uiLumaMode == uiModeList[i])
        {
            uiModeList[i] = 34; // VER+8 mode
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
Int TComDataCU::getIntraDirLumaPredictor(UInt absPartIdx, Int* uiIntraDirPred, Int* piMode)
{
    TComDataCU* pcTempCU;
    UInt        uiTempPartIdx;
    Int         iLeftIntraDir, iAboveIntraDir;
    Int         uiPredNum = 0;

    // Get intra direction of left PU
    pcTempCU = getPULeft(uiTempPartIdx, m_absIdxInLCU + absPartIdx);

    iLeftIntraDir  = pcTempCU ? (pcTempCU->isIntra(uiTempPartIdx) ? pcTempCU->getLumaIntraDir(uiTempPartIdx) : DC_IDX) : DC_IDX;

    // Get intra direction of above PU
    pcTempCU = getPUAbove(uiTempPartIdx, m_absIdxInLCU + absPartIdx, true, true);

    iAboveIntraDir = pcTempCU ? (pcTempCU->isIntra(uiTempPartIdx) ? pcTempCU->getLumaIntraDir(uiTempPartIdx) : DC_IDX) : DC_IDX;

    uiPredNum = 3;
    if (iLeftIntraDir == iAboveIntraDir)
    {
        if (piMode)
        {
            *piMode = 1;
        }

        if (iLeftIntraDir > 1) // angular modes
        {
            uiIntraDirPred[0] = iLeftIntraDir;
            uiIntraDirPred[1] = ((iLeftIntraDir + 29) % 32) + 2;
            uiIntraDirPred[2] = ((iLeftIntraDir - 1) % 32) + 2;
        }
        else //non-angular
        {
            uiIntraDirPred[0] = PLANAR_IDX;
            uiIntraDirPred[1] = DC_IDX;
            uiIntraDirPred[2] = VER_IDX;
        }
    }
    else
    {
        if (piMode)
        {
            *piMode = 2;
        }
        uiIntraDirPred[0] = iLeftIntraDir;
        uiIntraDirPred[1] = iAboveIntraDir;

        if (iLeftIntraDir && iAboveIntraDir) //both modes are non-planar
        {
            uiIntraDirPred[2] = PLANAR_IDX;
        }
        else
        {
            uiIntraDirPred[2] =  (iLeftIntraDir + iAboveIntraDir) < 2 ? VER_IDX : DC_IDX;
        }
    }

    return uiPredNum;
}

UInt TComDataCU::getCtxSplitFlag(UInt absPartIdx, UInt depth)
{
    TComDataCU* pcTempCU;
    UInt        uiTempPartIdx;
    UInt        uiCtx;

    // Get left split flag
    pcTempCU = getPULeft(uiTempPartIdx, m_absIdxInLCU + absPartIdx);
    uiCtx  = (pcTempCU) ? ((pcTempCU->getDepth(uiTempPartIdx) > depth) ? 1 : 0) : 0;

    // Get above split flag
    pcTempCU = getPUAbove(uiTempPartIdx, m_absIdxInLCU + absPartIdx);
    uiCtx += (pcTempCU) ? ((pcTempCU->getDepth(uiTempPartIdx) > depth) ? 1 : 0) : 0;

    return uiCtx;
}

UInt TComDataCU::getCtxQtCbf(TextType ttype, UInt trDepth)
{
    if (ttype)
    {
        return trDepth;
    }
    else
    {
        const UInt uiCtx = (trDepth == 0 ? 1 : 0);
        return uiCtx;
    }
}

UInt TComDataCU::getQuadtreeTULog2MinSizeInCU(UInt absPartIdx)
{
    UInt log2CbSize = g_convertToBit[getWidth(absPartIdx)] + 2;
    PartSize  partSize  = getPartitionSize(absPartIdx);
    UInt quadtreeTUMaxDepth = getPredictionMode(absPartIdx) == MODE_INTRA ? m_slice->getSPS()->getQuadtreeTUMaxDepthIntra() : m_slice->getSPS()->getQuadtreeTUMaxDepthInter();
    Int intraSplitFlag = (getPredictionMode(absPartIdx) == MODE_INTRA && partSize == SIZE_NxN) ? 1 : 0;
    Int interSplitFlag = ((quadtreeTUMaxDepth == 1) && (getPredictionMode(absPartIdx) == MODE_INTER) && (partSize != SIZE_2Nx2N));

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
    TComDataCU* pcTempCU;
    UInt        uiTempPartIdx;
    UInt        uiCtx = 0;

    // Get BCBP of left PU
    pcTempCU = getPULeft(uiTempPartIdx, m_absIdxInLCU + absPartIdx);
    uiCtx    = (pcTempCU) ? pcTempCU->isSkipped(uiTempPartIdx) : 0;

    // Get BCBP of above PU
    pcTempCU = getPUAbove(uiTempPartIdx, m_absIdxInLCU + absPartIdx);
    uiCtx   += (pcTempCU) ? pcTempCU->isSkipped(uiTempPartIdx) : 0;

    return uiCtx;
}

UInt TComDataCU::getCtxInterDir(UInt absPartIdx)
{
    return getDepth(absPartIdx);
}

Void TComDataCU::setCbfSubParts(UInt uiCbfY, UInt uiCbfU, UInt uiCbfV, UInt absPartIdx, UInt depth)
{
    UInt uiCurrPartNumb = m_pic->getNumPartInCU() >> (depth << 1);

    memset(m_cbf[0] + absPartIdx, uiCbfY, sizeof(UChar) * uiCurrPartNumb);
    memset(m_cbf[1] + absPartIdx, uiCbfU, sizeof(UChar) * uiCurrPartNumb);
    memset(m_cbf[2] + absPartIdx, uiCbfV, sizeof(UChar) * uiCurrPartNumb);
}

Void TComDataCU::setCbfSubParts(UInt uiCbf, TextType eTType, UInt absPartIdx, UInt depth)
{
    UInt uiCurrPartNumb = m_pic->getNumPartInCU() >> (depth << 1);

    memset(m_cbf[g_convertTxtTypeToIdx[eTType]] + absPartIdx, uiCbf, sizeof(UChar) * uiCurrPartNumb);
}

/** Sets a coded block flag for all sub-partitions of a partition
 * \param uiCbf The value of the coded block flag to be set
 * \param eTType
 * \param absPartIdx
 * \param partIdx
 * \param depth
 * \returns Void
 */
Void TComDataCU::setCbfSubParts(UInt uiCbf, TextType eTType, UInt absPartIdx, UInt partIdx, UInt depth)
{
    setSubPart<UChar>(uiCbf, m_cbf[g_convertTxtTypeToIdx[eTType]], absPartIdx, depth, partIdx);
}

Void TComDataCU::setDepthSubParts(UInt depth, UInt absPartIdx)
{
    UInt uiCurrPartNumb = m_pic->getNumPartInCU() >> (depth << 1);

    memset(m_depth + absPartIdx, depth, sizeof(UChar) * uiCurrPartNumb);
}

Bool TComDataCU::isFirstAbsZorderIdxInDepth(UInt absPartIdx, UInt depth)
{
    UInt uiPartNumb = m_pic->getNumPartInCU() >> (depth << 1);

    return ((m_absIdxInLCU + absPartIdx) % uiPartNumb) == 0;
}

Void TComDataCU::setPartSizeSubParts(PartSize eMode, UInt absPartIdx, UInt depth)
{
    assert(sizeof(*m_partSizes) == 1);
    memset(m_partSizes + absPartIdx, eMode, m_pic->getNumPartInCU() >> (2 * depth));
}

Void TComDataCU::setCUTransquantBypassSubParts(Bool flag, UInt absPartIdx, UInt depth)
{
    memset(m_cuTransquantBypass + absPartIdx, flag, m_pic->getNumPartInCU() >> (2 * depth));
}

Void TComDataCU::setSkipFlagSubParts(Bool skip, UInt absPartIdx, UInt depth)
{
    assert(sizeof(*m_skipFlag) == 1);
    memset(m_skipFlag + absPartIdx, skip, m_pic->getNumPartInCU() >> (2 * depth));
}

Void TComDataCU::setPredModeSubParts(PredMode eMode, UInt absPartIdx, UInt depth)
{
    assert(sizeof(*m_predModes) == 1);
    memset(m_predModes + absPartIdx, eMode, m_pic->getNumPartInCU() >> (2 * depth));
}

Void TComDataCU::setQPSubCUs(Int qp, TComDataCU* cu, UInt absPartIdx, UInt depth, Bool &foundNonZeroCbf)
{
    UInt currPartNumb = m_pic->getNumPartInCU() >> (depth << 1);
    UInt currPartNumQ = currPartNumb >> 2;

    if (!foundNonZeroCbf)
    {
        if (cu->getDepth(absPartIdx) > depth)
        {
            for (UInt partUnitIdx = 0; partUnitIdx < 4; partUnitIdx++)
            {
                cu->setQPSubCUs(qp, cu, absPartIdx + partUnitIdx * currPartNumQ, depth + 1, foundNonZeroCbf);
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

Void TComDataCU::setQPSubParts(Int qp, UInt absPartIdx, UInt depth)
{
    UInt uiCurrPartNumb = m_pic->getNumPartInCU() >> (depth << 1);
    TComSlice * slice = getPic()->getSlice();

    for (UInt uiSCUIdx = absPartIdx; uiSCUIdx < absPartIdx + uiCurrPartNumb; uiSCUIdx++)
    {
        m_qp[uiSCUIdx] = qp;
    }
}

Void TComDataCU::setLumaIntraDirSubParts(UInt uiDir, UInt absPartIdx, UInt depth)
{
    UInt uiCurrPartNumb = m_pic->getNumPartInCU() >> (depth << 1);

    memset(m_puhLumaIntraDir + absPartIdx, uiDir, sizeof(UChar) * uiCurrPartNumb);
}

template<typename T>
Void TComDataCU::setSubPart(T uiParameter, T* puhBaseLCU, UInt cuAddr, UInt uiCUDepth, UInt uiPUIdx)
{
    assert(sizeof(T) == 1); // Using memset() works only for types of size 1

    UInt uiCurrPartNumQ = (m_pic->getNumPartInCU() >> (2 * uiCUDepth)) >> 2;
    switch (m_partSizes[cuAddr])
    {
    case SIZE_2Nx2N:
        memset(puhBaseLCU + cuAddr, uiParameter, 4 * uiCurrPartNumQ);
        break;
    case SIZE_2NxN:
        memset(puhBaseLCU + cuAddr, uiParameter, 2 * uiCurrPartNumQ);
        break;
    case SIZE_Nx2N:
        memset(puhBaseLCU + cuAddr, uiParameter, uiCurrPartNumQ);
        memset(puhBaseLCU + cuAddr + 2 * uiCurrPartNumQ, uiParameter, uiCurrPartNumQ);
        break;
    case SIZE_NxN:
        memset(puhBaseLCU + cuAddr, uiParameter, uiCurrPartNumQ);
        break;
    case SIZE_2NxnU:
        if (uiPUIdx == 0)
        {
            memset(puhBaseLCU + cuAddr, uiParameter, (uiCurrPartNumQ >> 1));
            memset(puhBaseLCU + cuAddr + uiCurrPartNumQ, uiParameter, (uiCurrPartNumQ >> 1));
        }
        else if (uiPUIdx == 1)
        {
            memset(puhBaseLCU + cuAddr, uiParameter, (uiCurrPartNumQ >> 1));
            memset(puhBaseLCU + cuAddr + uiCurrPartNumQ, uiParameter, ((uiCurrPartNumQ >> 1) + (uiCurrPartNumQ << 1)));
        }
        else
        {
            assert(0);
        }
        break;
    case SIZE_2NxnD:
        if (uiPUIdx == 0)
        {
            memset(puhBaseLCU + cuAddr, uiParameter, ((uiCurrPartNumQ << 1) + (uiCurrPartNumQ >> 1)));
            memset(puhBaseLCU + cuAddr + (uiCurrPartNumQ << 1) + uiCurrPartNumQ, uiParameter, (uiCurrPartNumQ >> 1));
        }
        else if (uiPUIdx == 1)
        {
            memset(puhBaseLCU + cuAddr, uiParameter, (uiCurrPartNumQ >> 1));
            memset(puhBaseLCU + cuAddr + uiCurrPartNumQ, uiParameter, (uiCurrPartNumQ >> 1));
        }
        else
        {
            assert(0);
        }
        break;
    case SIZE_nLx2N:
        if (uiPUIdx == 0)
        {
            memset(puhBaseLCU + cuAddr, uiParameter, (uiCurrPartNumQ >> 2));
            memset(puhBaseLCU + cuAddr + (uiCurrPartNumQ >> 1), uiParameter, (uiCurrPartNumQ >> 2));
            memset(puhBaseLCU + cuAddr + (uiCurrPartNumQ << 1), uiParameter, (uiCurrPartNumQ >> 2));
            memset(puhBaseLCU + cuAddr + (uiCurrPartNumQ << 1) + (uiCurrPartNumQ >> 1), uiParameter, (uiCurrPartNumQ >> 2));
        }
        else if (uiPUIdx == 1)
        {
            memset(puhBaseLCU + cuAddr, uiParameter, (uiCurrPartNumQ >> 2));
            memset(puhBaseLCU + cuAddr + (uiCurrPartNumQ >> 1), uiParameter, (uiCurrPartNumQ + (uiCurrPartNumQ >> 2)));
            memset(puhBaseLCU + cuAddr + (uiCurrPartNumQ << 1), uiParameter, (uiCurrPartNumQ >> 2));
            memset(puhBaseLCU + cuAddr + (uiCurrPartNumQ << 1) + (uiCurrPartNumQ >> 1), uiParameter, (uiCurrPartNumQ + (uiCurrPartNumQ >> 2)));
        }
        else
        {
            assert(0);
        }
        break;
    case SIZE_nRx2N:
        if (uiPUIdx == 0)
        {
            memset(puhBaseLCU + cuAddr, uiParameter, (uiCurrPartNumQ + (uiCurrPartNumQ >> 2)));
            memset(puhBaseLCU + cuAddr + uiCurrPartNumQ + (uiCurrPartNumQ >> 1), uiParameter, (uiCurrPartNumQ >> 2));
            memset(puhBaseLCU + cuAddr + (uiCurrPartNumQ << 1), uiParameter, (uiCurrPartNumQ + (uiCurrPartNumQ >> 2)));
            memset(puhBaseLCU + cuAddr + (uiCurrPartNumQ << 1) + uiCurrPartNumQ + (uiCurrPartNumQ >> 1), uiParameter, (uiCurrPartNumQ >> 2));
        }
        else if (uiPUIdx == 1)
        {
            memset(puhBaseLCU + cuAddr, uiParameter, (uiCurrPartNumQ >> 2));
            memset(puhBaseLCU + cuAddr + (uiCurrPartNumQ >> 1), uiParameter, (uiCurrPartNumQ >> 2));
            memset(puhBaseLCU + cuAddr + (uiCurrPartNumQ << 1), uiParameter, (uiCurrPartNumQ >> 2));
            memset(puhBaseLCU + cuAddr + (uiCurrPartNumQ << 1) + (uiCurrPartNumQ >> 1), uiParameter, (uiCurrPartNumQ >> 2));
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

Void TComDataCU::setMergeFlagSubParts(Bool bMergeFlag, UInt absPartIdx, UInt partIdx, UInt depth)
{
    setSubPart(bMergeFlag, m_pbMergeFlag, absPartIdx, depth, partIdx);
}

Void TComDataCU::setMergeIndexSubParts(UInt uiMergeIndex, UInt absPartIdx, UInt partIdx, UInt depth)
{
    setSubPart<UChar>(uiMergeIndex, m_puhMergeIndex, absPartIdx, depth, partIdx);
}

Void TComDataCU::setChromIntraDirSubParts(UInt uiDir, UInt absPartIdx, UInt depth)
{
    UInt uiCurrPartNumb = m_pic->getNumPartInCU() >> (depth << 1);

    memset(m_puhChromaIntraDir + absPartIdx, uiDir, sizeof(UChar) * uiCurrPartNumb);
}

Void TComDataCU::setInterDirSubParts(UInt uiDir, UInt absPartIdx, UInt partIdx, UInt depth)
{
    setSubPart<UChar>(uiDir, m_puhInterDir, absPartIdx, depth, partIdx);
}

Void TComDataCU::setMVPIdxSubParts(Int mvpIdx, RefPicList picList, UInt absPartIdx, UInt partIdx, UInt depth)
{
    setSubPart<Char>(mvpIdx, m_apiMVPIdx[picList], absPartIdx, depth, partIdx);
}

Void TComDataCU::setMVPNumSubParts(Int iMVPNum, RefPicList picList, UInt absPartIdx, UInt partIdx, UInt depth)
{
    setSubPart<Char>(iMVPNum, m_apiMVPNum[picList], absPartIdx, depth, partIdx);
}

Void TComDataCU::setTrIdxSubParts(UInt uiTrIdx, UInt absPartIdx, UInt depth)
{
    UInt uiCurrPartNumb = m_pic->getNumPartInCU() >> (depth << 1);

    memset(m_trIdx + absPartIdx, uiTrIdx, sizeof(UChar) * uiCurrPartNumb);
}

Void TComDataCU::setTransformSkipSubParts(UInt useTransformSkipY, UInt useTransformSkipU, UInt useTransformSkipV, UInt absPartIdx, UInt depth)
{
    UInt uiCurrPartNumb = m_pic->getNumPartInCU() >> (depth << 1);

    memset(m_transformSkip[0] + absPartIdx, useTransformSkipY, sizeof(UChar) * uiCurrPartNumb);
    memset(m_transformSkip[1] + absPartIdx, useTransformSkipU, sizeof(UChar) * uiCurrPartNumb);
    memset(m_transformSkip[2] + absPartIdx, useTransformSkipV, sizeof(UChar) * uiCurrPartNumb);
}

Void TComDataCU::setTransformSkipSubParts(UInt useTransformSkip, TextType ttype, UInt absPartIdx, UInt depth)
{
    UInt uiCurrPartNumb = m_pic->getNumPartInCU() >> (depth << 1);

    memset(m_transformSkip[g_convertTxtTypeToIdx[ttype]] + absPartIdx, useTransformSkip, sizeof(UChar) * uiCurrPartNumb);
}

Void TComDataCU::setSizeSubParts(UInt width, UInt height, UInt absPartIdx, UInt depth)
{
    UInt uiCurrPartNumb = m_pic->getNumPartInCU() >> (depth << 1);

    memset(m_width  + absPartIdx, width,  sizeof(UChar) * uiCurrPartNumb);
    memset(m_height + absPartIdx, height, sizeof(UChar) * uiCurrPartNumb);
}

UChar TComDataCU::getNumPartInter()
{
    UChar iNumPart = 0;

    switch (m_partSizes[0])
    {
    case SIZE_2Nx2N:    iNumPart = 1;
        break;
    case SIZE_2NxN:     iNumPart = 2;
        break;
    case SIZE_Nx2N:     iNumPart = 2;
        break;
    case SIZE_NxN:      iNumPart = 4;
        break;
    case SIZE_2NxnU:    iNumPart = 2;
        break;
    case SIZE_2NxnD:    iNumPart = 2;
        break;
    case SIZE_nLx2N:    iNumPart = 2;
        break;
    case SIZE_nRx2N:    iNumPart = 2;
        break;
    default:            assert(0);
        break;
    }

    return iNumPart;
}

Void TComDataCU::getPartIndexAndSize(UInt partIdx, UInt& ruiPartAddr, Int& riWidth, Int& riHeight)
{
    switch (m_partSizes[0])
    {
    case SIZE_2NxN:
        riWidth = getWidth(0);
        riHeight = getHeight(0) >> 1;
        ruiPartAddr = (partIdx == 0) ? 0 : m_numPartitions >> 1;
        break;
    case SIZE_Nx2N:
        riWidth = getWidth(0) >> 1;
        riHeight = getHeight(0);
        ruiPartAddr = (partIdx == 0) ? 0 : m_numPartitions >> 2;
        break;
    case SIZE_NxN:
        riWidth = getWidth(0) >> 1;
        riHeight = getHeight(0) >> 1;
        ruiPartAddr = (m_numPartitions >> 2) * partIdx;
        break;
    case SIZE_2NxnU:
        riWidth     = getWidth(0);
        riHeight    = (partIdx == 0) ?  getHeight(0) >> 2 : (getHeight(0) >> 2) + (getHeight(0) >> 1);
        ruiPartAddr = (partIdx == 0) ? 0 : m_numPartitions >> 3;
        break;
    case SIZE_2NxnD:
        riWidth     = getWidth(0);
        riHeight    = (partIdx == 0) ?  (getHeight(0) >> 2) + (getHeight(0) >> 1) : getHeight(0) >> 2;
        ruiPartAddr = (partIdx == 0) ? 0 : (m_numPartitions >> 1) + (m_numPartitions >> 3);
        break;
    case SIZE_nLx2N:
        riWidth     = (partIdx == 0) ? getWidth(0) >> 2 : (getWidth(0) >> 2) + (getWidth(0) >> 1);
        riHeight    = getHeight(0);
        ruiPartAddr = (partIdx == 0) ? 0 : m_numPartitions >> 4;
        break;
    case SIZE_nRx2N:
        riWidth     = (partIdx == 0) ? (getWidth(0) >> 2) + (getWidth(0) >> 1) : getWidth(0) >> 2;
        riHeight    = getHeight(0);
        ruiPartAddr = (partIdx == 0) ? 0 : (m_numPartitions >> 2) + (m_numPartitions >> 4);
        break;
    default:
        assert(m_partSizes[0] == SIZE_2Nx2N);
        riWidth = getWidth(0);
        riHeight = getHeight(0);
        ruiPartAddr = 0;
        break;
    }
}

Void TComDataCU::getMvField(TComDataCU* cu, UInt absPartIdx, RefPicList picList, TComMvField& rcMvField)
{
    if (cu == NULL)  // OUT OF BOUNDARY
    {
        MV cZeroMv(0, 0);
        rcMvField.setMvField(cZeroMv, NOT_VALID);
        return;
    }

    TComCUMvField*  pcCUMvField = cu->getCUMvField(picList);
    rcMvField.setMvField(pcCUMvField->getMv(absPartIdx), pcCUMvField->getRefIdx(absPartIdx));
}

Void TComDataCU::deriveLeftRightTopIdxGeneral(UInt absPartIdx, UInt partIdx, UInt& ruiPartIdxLT, UInt& ruiPartIdxRT)
{
    ruiPartIdxLT = m_absIdxInLCU + absPartIdx;
    UInt uiPUWidth = 0;

    switch (m_partSizes[absPartIdx])
    {
    case SIZE_2Nx2N: uiPUWidth = m_width[absPartIdx];
        break;
    case SIZE_2NxN:  uiPUWidth = m_width[absPartIdx];
        break;
    case SIZE_Nx2N:  uiPUWidth = m_width[absPartIdx]  >> 1;
        break;
    case SIZE_NxN:   uiPUWidth = m_width[absPartIdx]  >> 1;
        break;
    case SIZE_2NxnU:   uiPUWidth = m_width[absPartIdx];
        break;
    case SIZE_2NxnD:   uiPUWidth = m_width[absPartIdx];
        break;
    case SIZE_nLx2N:
        if (partIdx == 0)
        {
            uiPUWidth = m_width[absPartIdx]  >> 2;
        }
        else if (partIdx == 1)
        {
            uiPUWidth = (m_width[absPartIdx]  >> 1) + (m_width[absPartIdx]  >> 2);
        }
        else
        {
            assert(0);
        }
        break;
    case SIZE_nRx2N:
        if (partIdx == 0)
        {
            uiPUWidth = (m_width[absPartIdx]  >> 1) + (m_width[absPartIdx]  >> 2);
        }
        else if (partIdx == 1)
        {
            uiPUWidth = m_width[absPartIdx]  >> 2;
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

    ruiPartIdxRT = g_rasterToZscan[g_zscanToRaster[ruiPartIdxLT] + uiPUWidth / m_pic->getMinCUWidth() - 1];
}

Void TComDataCU::deriveLeftBottomIdxGeneral(UInt absPartIdx, UInt partIdx, UInt& ruiPartIdxLB)
{
    UInt uiPUHeight = 0;

    switch (m_partSizes[absPartIdx])
    {
    case SIZE_2Nx2N: uiPUHeight = m_height[absPartIdx];
        break;
    case SIZE_2NxN:  uiPUHeight = m_height[absPartIdx] >> 1;
        break;
    case SIZE_Nx2N:  uiPUHeight = m_height[absPartIdx];
        break;
    case SIZE_NxN:   uiPUHeight = m_height[absPartIdx] >> 1;
        break;
    case SIZE_2NxnU:
        if (partIdx == 0)
        {
            uiPUHeight = m_height[absPartIdx] >> 2;
        }
        else if (partIdx == 1)
        {
            uiPUHeight = (m_height[absPartIdx] >> 1) + (m_height[absPartIdx] >> 2);
        }
        else
        {
            assert(0);
        }
        break;
    case SIZE_2NxnD:
        if (partIdx == 0)
        {
            uiPUHeight = (m_height[absPartIdx] >> 1) + (m_height[absPartIdx] >> 2);
        }
        else if (partIdx == 1)
        {
            uiPUHeight = m_height[absPartIdx] >> 2;
        }
        else
        {
            assert(0);
        }
        break;
    case SIZE_nLx2N: uiPUHeight = m_height[absPartIdx];
        break;
    case SIZE_nRx2N: uiPUHeight = m_height[absPartIdx];
        break;
    default:
        assert(0);
        break;
    }

    ruiPartIdxLB      = g_rasterToZscan[g_zscanToRaster[m_absIdxInLCU + absPartIdx] + ((uiPUHeight / m_pic->getMinCUHeight()) - 1) * m_pic->getNumPartInWidth()];
}

Void TComDataCU::deriveLeftRightTopIdx(UInt partIdx, UInt& ruiPartIdxLT, UInt& ruiPartIdxRT)
{
    ruiPartIdxLT = m_absIdxInLCU;
    ruiPartIdxRT = g_rasterToZscan[g_zscanToRaster[ruiPartIdxLT] + m_width[0] / m_pic->getMinCUWidth() - 1];

    switch (m_partSizes[0])
    {
    case SIZE_2Nx2N:                                                                                                                                break;
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

Void TComDataCU::deriveLeftBottomIdx(UInt partIdx,      UInt&      ruiPartIdxLB)
{
    ruiPartIdxLB      = g_rasterToZscan[g_zscanToRaster[m_absIdxInLCU] + (((m_height[0] / m_pic->getMinCUHeight()) >> 1) - 1) * m_pic->getNumPartInWidth()];

    switch (m_partSizes[0])
    {
    case SIZE_2Nx2N:
        ruiPartIdxLB += m_numPartitions >> 1;
        break;
    case SIZE_2NxN:
        ruiPartIdxLB += (partIdx == 0) ? 0 : m_numPartitions >> 1;
        break;
    case SIZE_Nx2N:
        ruiPartIdxLB += (partIdx == 0) ? m_numPartitions >> 1 : (m_numPartitions >> 2) * 3;
        break;
    case SIZE_NxN:
        ruiPartIdxLB += (m_numPartitions >> 2) * partIdx;
        break;
    case SIZE_2NxnU:
        ruiPartIdxLB += (partIdx == 0) ? -((Int)m_numPartitions >> 3) : m_numPartitions >> 1;
        break;
    case SIZE_2NxnD:
        ruiPartIdxLB += (partIdx == 0) ? (m_numPartitions >> 2) + (m_numPartitions >> 3) : m_numPartitions >> 1;
        break;
    case SIZE_nLx2N:
        ruiPartIdxLB += (partIdx == 0) ? m_numPartitions >> 1 : (m_numPartitions >> 1) + (m_numPartitions >> 4);
        break;
    case SIZE_nRx2N:
        ruiPartIdxLB += (partIdx == 0) ? m_numPartitions >> 1 : (m_numPartitions >> 1) + (m_numPartitions >> 2) + (m_numPartitions >> 4);
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
Void TComDataCU::deriveRightBottomIdx(UInt partIdx,      UInt&      outPartIdxRB)
{
    outPartIdxRB      = g_rasterToZscan[g_zscanToRaster[m_absIdxInLCU] + (((m_height[0] / m_pic->getMinCUHeight()) >> 1) - 1) * m_pic->getNumPartInWidth() +  m_width[0] / m_pic->getMinCUWidth() - 1];

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
        outPartIdxRB += (partIdx == 0) ? -((Int)m_numPartitions >> 3) : m_numPartitions >> 1;
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

Void TComDataCU::deriveLeftRightTopIdxAdi(UInt& ruiPartIdxLT, UInt& ruiPartIdxRT, UInt partOffset, UInt partDepth)
{
    UInt uiNumPartInWidth = (m_width[0] / m_pic->getMinCUWidth()) >> partDepth;

    ruiPartIdxLT = m_absIdxInLCU + partOffset;
    ruiPartIdxRT = g_rasterToZscan[g_zscanToRaster[ruiPartIdxLT] + uiNumPartInWidth - 1];
}

Void TComDataCU::deriveLeftBottomIdxAdi(UInt& ruiPartIdxLB, UInt partOffset, UInt partDepth)
{
    UInt uiAbsIdx;
    UInt uiMinCuWidth, uiWidthInMinCus;

    uiMinCuWidth    = getPic()->getMinCUWidth();
    uiWidthInMinCus = (getWidth(0) / uiMinCuWidth) >> partDepth;
    uiAbsIdx        = getZorderIdxInCU() + partOffset + (m_numPartitions >> (partDepth << 1)) - 1;
    uiAbsIdx        = g_zscanToRaster[uiAbsIdx] - (uiWidthInMinCus - 1);
    ruiPartIdxLB    = g_rasterToZscan[uiAbsIdx];
}

Bool TComDataCU::hasEqualMotion(UInt absPartIdx, TComDataCU* pcCandCU, UInt uiCandAbsPartIdx)
{
    if (getInterDir(absPartIdx) != pcCandCU->getInterDir(uiCandAbsPartIdx))
    {
        return false;
    }

    for (UInt refListIdx = 0; refListIdx < 2; refListIdx++)
    {
        if (getInterDir(absPartIdx) & (1 << refListIdx))
        {
            if (getCUMvField(RefPicList(refListIdx))->getMv(absPartIdx)     != pcCandCU->getCUMvField(RefPicList(refListIdx))->getMv(uiCandAbsPartIdx) ||
                getCUMvField(RefPicList(refListIdx))->getRefIdx(absPartIdx) != pcCandCU->getCUMvField(RefPicList(refListIdx))->getRefIdx(uiCandAbsPartIdx))
            {
                return false;
            }
        }
    }

    return true;
}

/** Constructs a list of merging candidates
 * \param absPartIdx
 * \param uiPUIdx
 * \param depth
 * \param pcMvFieldNeighbours
 * \param puhInterDirNeighbours
 * \param numValidMergeCand
 */
Void TComDataCU::getInterMergeCandidates(UInt absPartIdx, UInt uiPUIdx, TComMvField* pcMvFieldNeighbours, UChar* puhInterDirNeighbours, Int& numValidMergeCand, Int mrgCandIdx)
{
    UInt uiAbsPartAddr = m_absIdxInLCU + absPartIdx;
    Bool abCandIsInter[MRG_MAX_NUM_CANDS];

    for (UInt ui = 0; ui < getSlice()->getMaxNumMergeCand(); ++ui)
    {
        abCandIsInter[ui] = false;
        pcMvFieldNeighbours[(ui << 1)].refIdx = NOT_VALID;
        pcMvFieldNeighbours[(ui << 1) + 1].refIdx = NOT_VALID;
    }

    numValidMergeCand = getSlice()->getMaxNumMergeCand();
    // compute the location of the current PU
    Int xP, yP, nPSW, nPSH;
    this->getPartPosition(uiPUIdx, xP, yP, nPSW, nPSH);

    Int iCount = 0;

    UInt partIdxLT, partIdxRT, partIdxLB;
    PartSize cCurPS = getPartitionSize(absPartIdx);
    deriveLeftRightTopIdxGeneral(absPartIdx, uiPUIdx, partIdxLT, partIdxRT);
    deriveLeftBottomIdxGeneral(absPartIdx, uiPUIdx, partIdxLB);

    //left
    UInt uiLeftPartIdx = 0;
    TComDataCU* pcCULeft = 0;
    pcCULeft = getPULeft(uiLeftPartIdx, partIdxLB);
    Bool isAvailableA1 = pcCULeft &&
        pcCULeft->isDiffMER(xP - 1, yP + nPSH - 1, xP, yP) &&
        !(uiPUIdx == 1 && (cCurPS == SIZE_Nx2N || cCurPS == SIZE_nLx2N || cCurPS == SIZE_nRx2N)) &&
        !pcCULeft->isIntra(uiLeftPartIdx);
    if (isAvailableA1)
    {
        abCandIsInter[iCount] = true;
        // get Inter Dir
        puhInterDirNeighbours[iCount] = pcCULeft->getInterDir(uiLeftPartIdx);
        // get Mv from Left
        pcCULeft->getMvField(pcCULeft, uiLeftPartIdx, REF_PIC_LIST_0, pcMvFieldNeighbours[iCount << 1]);
        if (getSlice()->isInterB())
        {
            pcCULeft->getMvField(pcCULeft, uiLeftPartIdx, REF_PIC_LIST_1, pcMvFieldNeighbours[(iCount << 1) + 1]);
        }
        if (mrgCandIdx == iCount)
        {
            return;
        }
        iCount++;
    }

    // early termination
    if (iCount == getSlice()->getMaxNumMergeCand())
    {
        return;
    }
    // above
    UInt uiAbovePartIdx = 0;
    TComDataCU* pcCUAbove = 0;
    pcCUAbove = getPUAbove(uiAbovePartIdx, partIdxRT);
    Bool isAvailableB1 = pcCUAbove &&
        pcCUAbove->isDiffMER(xP + nPSW - 1, yP - 1, xP, yP) &&
        !(uiPUIdx == 1 && (cCurPS == SIZE_2NxN || cCurPS == SIZE_2NxnU || cCurPS == SIZE_2NxnD)) &&
        !pcCUAbove->isIntra(uiAbovePartIdx);
    if (isAvailableB1 && (!isAvailableA1 || !pcCULeft->hasEqualMotion(uiLeftPartIdx, pcCUAbove, uiAbovePartIdx)))
    {
        abCandIsInter[iCount] = true;
        // get Inter Dir
        puhInterDirNeighbours[iCount] = pcCUAbove->getInterDir(uiAbovePartIdx);
        // get Mv from Left
        pcCUAbove->getMvField(pcCUAbove, uiAbovePartIdx, REF_PIC_LIST_0, pcMvFieldNeighbours[iCount << 1]);
        if (getSlice()->isInterB())
        {
            pcCUAbove->getMvField(pcCUAbove, uiAbovePartIdx, REF_PIC_LIST_1, pcMvFieldNeighbours[(iCount << 1) + 1]);
        }
        if (mrgCandIdx == iCount)
        {
            return;
        }
        iCount++;
    }
    // early termination
    if (iCount == getSlice()->getMaxNumMergeCand())
    {
        return;
    }

    // above right
    UInt uiAboveRightPartIdx = 0;
    TComDataCU* pcCUAboveRight = 0;
    pcCUAboveRight = getPUAboveRight(uiAboveRightPartIdx, partIdxRT);
    Bool isAvailableB0 = pcCUAboveRight &&
        pcCUAboveRight->isDiffMER(xP + nPSW, yP - 1, xP, yP) &&
        !pcCUAboveRight->isIntra(uiAboveRightPartIdx);
    if (isAvailableB0 && (!isAvailableB1 || !pcCUAbove->hasEqualMotion(uiAbovePartIdx, pcCUAboveRight, uiAboveRightPartIdx)))
    {
        abCandIsInter[iCount] = true;
        // get Inter Dir
        puhInterDirNeighbours[iCount] = pcCUAboveRight->getInterDir(uiAboveRightPartIdx);
        // get Mv from Left
        pcCUAboveRight->getMvField(pcCUAboveRight, uiAboveRightPartIdx, REF_PIC_LIST_0, pcMvFieldNeighbours[iCount << 1]);
        if (getSlice()->isInterB())
        {
            pcCUAboveRight->getMvField(pcCUAboveRight, uiAboveRightPartIdx, REF_PIC_LIST_1, pcMvFieldNeighbours[(iCount << 1) + 1]);
        }
        if (mrgCandIdx == iCount)
        {
            return;
        }
        iCount++;
    }
    // early termination
    if (iCount == getSlice()->getMaxNumMergeCand())
    {
        return;
    }

    //left bottom
    UInt uiLeftBottomPartIdx = 0;
    TComDataCU* pcCULeftBottom = 0;
    pcCULeftBottom = this->getPUBelowLeft(uiLeftBottomPartIdx, partIdxLB);
    Bool isAvailableA0 = pcCULeftBottom &&
        pcCULeftBottom->isDiffMER(xP - 1, yP + nPSH, xP, yP) &&
        !pcCULeftBottom->isIntra(uiLeftBottomPartIdx);
    if (isAvailableA0 && (!isAvailableA1 || !pcCULeft->hasEqualMotion(uiLeftPartIdx, pcCULeftBottom, uiLeftBottomPartIdx)))
    {
        abCandIsInter[iCount] = true;
        // get Inter Dir
        puhInterDirNeighbours[iCount] = pcCULeftBottom->getInterDir(uiLeftBottomPartIdx);
        // get Mv from Left
        pcCULeftBottom->getMvField(pcCULeftBottom, uiLeftBottomPartIdx, REF_PIC_LIST_0, pcMvFieldNeighbours[iCount << 1]);
        if (getSlice()->isInterB())
        {
            pcCULeftBottom->getMvField(pcCULeftBottom, uiLeftBottomPartIdx, REF_PIC_LIST_1, pcMvFieldNeighbours[(iCount << 1) + 1]);
        }
        if (mrgCandIdx == iCount)
        {
            return;
        }
        iCount++;
    }
    // early termination
    if (iCount == getSlice()->getMaxNumMergeCand())
    {
        return;
    }
    // above left
    if (iCount < 4)
    {
        UInt uiAboveLeftPartIdx = 0;
        TComDataCU* pcCUAboveLeft = 0;
        pcCUAboveLeft = getPUAboveLeft(uiAboveLeftPartIdx, uiAbsPartAddr);
        Bool isAvailableB2 = pcCUAboveLeft &&
            pcCUAboveLeft->isDiffMER(xP - 1, yP - 1, xP, yP) &&
            !pcCUAboveLeft->isIntra(uiAboveLeftPartIdx);
        if (isAvailableB2 && (!isAvailableA1 || !pcCULeft->hasEqualMotion(uiLeftPartIdx, pcCUAboveLeft, uiAboveLeftPartIdx))
            && (!isAvailableB1 || !pcCUAbove->hasEqualMotion(uiAbovePartIdx, pcCUAboveLeft, uiAboveLeftPartIdx)))
        {
            abCandIsInter[iCount] = true;
            // get Inter Dir
            puhInterDirNeighbours[iCount] = pcCUAboveLeft->getInterDir(uiAboveLeftPartIdx);
            // get Mv from Left
            pcCUAboveLeft->getMvField(pcCUAboveLeft, uiAboveLeftPartIdx, REF_PIC_LIST_0, pcMvFieldNeighbours[iCount << 1]);
            if (getSlice()->isInterB())
            {
                pcCUAboveLeft->getMvField(pcCUAboveLeft, uiAboveLeftPartIdx, REF_PIC_LIST_1, pcMvFieldNeighbours[(iCount << 1) + 1]);
            }
            if (mrgCandIdx == iCount)
            {
                return;
            }
            iCount++;
        }
    }
    // early termination
    if (iCount == getSlice()->getMaxNumMergeCand())
    {
        return;
    }
    if (getSlice()->getEnableTMVPFlag())
    {
        //>> MTK colocated-RightBottom
        UInt uiPartIdxRB;
        Int uiLCUIdx = getAddr();

        deriveRightBottomIdx(uiPUIdx, uiPartIdxRB);

        UInt uiAbsPartIdxTmp = g_zscanToRaster[uiPartIdxRB];
        UInt uiNumPartInCUWidth = m_pic->getNumPartInWidth();

        MV cColMv;
        Int refIdx;

        if ((m_pic->getCU(m_cuAddr)->getCUPelX() + g_rasterToPelX[uiAbsPartIdxTmp] + m_pic->getMinCUWidth()) >= m_slice->getSPS()->getPicWidthInLumaSamples())  // image boundary check
        {
            uiLCUIdx = -1;
        }
        else if ((m_pic->getCU(m_cuAddr)->getCUPelY() + g_rasterToPelY[uiAbsPartIdxTmp] + m_pic->getMinCUHeight()) >= m_slice->getSPS()->getPicHeightInLumaSamples())
        {
            uiLCUIdx = -1;
        }
        else
        {
            if ((uiAbsPartIdxTmp % uiNumPartInCUWidth < uiNumPartInCUWidth - 1) &&        // is not at the last column of LCU
                (uiAbsPartIdxTmp / uiNumPartInCUWidth < m_pic->getNumPartInHeight() - 1)) // is not at the last row    of LCU
            {
                uiAbsPartAddr = g_rasterToZscan[uiAbsPartIdxTmp + uiNumPartInCUWidth + 1];
                uiLCUIdx = getAddr();
            }
            else if (uiAbsPartIdxTmp % uiNumPartInCUWidth < uiNumPartInCUWidth - 1)       // is not at the last column of LCU But is last row of LCU
            {
                uiAbsPartAddr = g_rasterToZscan[(uiAbsPartIdxTmp + uiNumPartInCUWidth + 1) % m_pic->getNumPartInCU()];
                uiLCUIdx = -1;
            }
            else if (uiAbsPartIdxTmp / uiNumPartInCUWidth < m_pic->getNumPartInHeight() - 1) // is not at the last row of LCU But is last column of LCU
            {
                uiAbsPartAddr = g_rasterToZscan[uiAbsPartIdxTmp + 1];
                uiLCUIdx = getAddr() + 1;
            }
            else //is the right bottom corner of LCU
            {
                uiAbsPartAddr = 0;
                uiLCUIdx = -1;
            }
        }

        refIdx = 0;
        Bool bExistMV = false;
        UInt uiPartIdxCenter;
        UInt uiCurLCUIdx = getAddr();
        Int dir = 0;
        UInt uiArrayAddr = iCount;
        xDeriveCenterIdx(uiPUIdx, uiPartIdxCenter);
        bExistMV = uiLCUIdx >= 0 && xGetColMVP(REF_PIC_LIST_0, uiLCUIdx, uiAbsPartAddr, cColMv, refIdx);
        if (bExistMV == false)
        {
            bExistMV = xGetColMVP(REF_PIC_LIST_0, uiCurLCUIdx, uiPartIdxCenter, cColMv, refIdx);
        }
        if (bExistMV)
        {
            dir |= 1;
            pcMvFieldNeighbours[2 * uiArrayAddr].setMvField(cColMv, refIdx);
        }

        if (getSlice()->isInterB())
        {
            bExistMV = uiLCUIdx >= 0 && xGetColMVP(REF_PIC_LIST_1, uiLCUIdx, uiAbsPartAddr, cColMv, refIdx);
            if (bExistMV == false)
            {
                bExistMV = xGetColMVP(REF_PIC_LIST_1, uiCurLCUIdx, uiPartIdxCenter, cColMv, refIdx);
            }
            if (bExistMV)
            {
                dir |= 2;
                pcMvFieldNeighbours[2 * uiArrayAddr + 1].setMvField(cColMv, refIdx);
            }
        }

        if (dir != 0)
        {
            puhInterDirNeighbours[uiArrayAddr] = dir;
            abCandIsInter[uiArrayAddr] = true;

            if (mrgCandIdx == iCount)
            {
                return;
            }
            iCount++;
        }
    }
    // early termination
    if (iCount == getSlice()->getMaxNumMergeCand())
    {
        return;
    }
    UInt uiArrayAddr = iCount;
    UInt uiCutoff = uiArrayAddr;

    if (getSlice()->isInterB())
    {
        UInt uiPriorityList0[12] = { 0, 1, 0, 2, 1, 2, 0, 3, 1, 3, 2, 3 };
        UInt uiPriorityList1[12] = { 1, 0, 2, 0, 2, 1, 3, 0, 3, 1, 3, 2 };

        for (Int idx = 0; idx < uiCutoff * (uiCutoff - 1) && uiArrayAddr != getSlice()->getMaxNumMergeCand(); idx++)
        {
            Int i = uiPriorityList0[idx];
            Int j = uiPriorityList1[idx];
            if (abCandIsInter[i] && abCandIsInter[j] && (puhInterDirNeighbours[i] & 0x1) && (puhInterDirNeighbours[j] & 0x2))
            {
                abCandIsInter[uiArrayAddr] = true;
                puhInterDirNeighbours[uiArrayAddr] = 3;

                // get Mv from cand[i] and cand[j]
                pcMvFieldNeighbours[uiArrayAddr << 1].setMvField(pcMvFieldNeighbours[i << 1].mv, pcMvFieldNeighbours[i << 1].refIdx);
                pcMvFieldNeighbours[(uiArrayAddr << 1) + 1].setMvField(pcMvFieldNeighbours[(j << 1) + 1].mv, pcMvFieldNeighbours[(j << 1) + 1].refIdx);

                Int iRefPOCL0 = m_slice->getRefPOC(REF_PIC_LIST_0, pcMvFieldNeighbours[(uiArrayAddr << 1)].refIdx);
                Int iRefPOCL1 = m_slice->getRefPOC(REF_PIC_LIST_1, pcMvFieldNeighbours[(uiArrayAddr << 1) + 1].refIdx);
                if (iRefPOCL0 == iRefPOCL1 && pcMvFieldNeighbours[(uiArrayAddr << 1)].mv == pcMvFieldNeighbours[(uiArrayAddr << 1) + 1].mv)
                {
                    abCandIsInter[uiArrayAddr] = false;
                }
                else
                {
                    uiArrayAddr++;
                }
            }
        }
    }
    // early termination
    if (uiArrayAddr == getSlice()->getMaxNumMergeCand())
    {
        return;
    }
    Int iNumRefIdx = (getSlice()->isInterB()) ? min(m_slice->getNumRefIdx(REF_PIC_LIST_0), m_slice->getNumRefIdx(REF_PIC_LIST_1)) : m_slice->getNumRefIdx(REF_PIC_LIST_0);
    Int r = 0;
    Int refcnt = 0;
    while (uiArrayAddr < getSlice()->getMaxNumMergeCand())
    {
        abCandIsInter[uiArrayAddr] = true;
        puhInterDirNeighbours[uiArrayAddr] = 1;
        pcMvFieldNeighbours[uiArrayAddr << 1].setMvField(MV(0, 0), r);

        if (getSlice()->isInterB())
        {
            puhInterDirNeighbours[uiArrayAddr] = 3;
            pcMvFieldNeighbours[(uiArrayAddr << 1) + 1].setMvField(MV(0, 0), r);
        }
        uiArrayAddr++;
        if (refcnt == iNumRefIdx - 1)
        {
            r = 0;
        }
        else
        {
            ++r;
            ++refcnt;
        }
    }

    numValidMergeCand = uiArrayAddr;
}

/** Check whether the current PU and a spatial neighboring PU are in a same ME region.
 * \param xN, xN   location of the upper-left corner pixel of a neighboring PU
 * \param xP, yP   location of the upper-left corner pixel of the current PU
 * \returns Bool
 */
Bool TComDataCU::isDiffMER(Int xN, Int yN, Int xP, Int yP)
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
 * \returns Void
 */
Void TComDataCU::getPartPosition(UInt partIdx, Int& xP, Int& yP, Int& nPSW, Int& nPSH)
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
Void TComDataCU::fillMvpCand(UInt partIdx, UInt partAddr, RefPicList picList, Int refIdx, AMVPInfo* info)
{
    MV cMvPred;
    Bool bAddedSmvp = false;

    info->m_num = 0;
    if (refIdx < 0)
    {
        return;
    }

    //-- Get Spatial MV
    UInt partIdxLT, partIdxRT, partIdxLB;
    UInt uiNumPartInCUWidth = m_pic->getNumPartInWidth();
    Bool bAdded = false;

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
        Int iRefIdx_Col = refIdx;
        MV   cColMv;
        UInt uiPartIdxRB;
        UInt absPartIdx;
        UInt uiAbsPartAddr;
        Int uiLCUIdx = getAddr();

        deriveRightBottomIdx(partIdx, uiPartIdxRB);
        uiAbsPartAddr = m_absIdxInLCU + partAddr;

        //----  co-located RightBottom Temporal Predictor (H) ---//
        absPartIdx = g_zscanToRaster[uiPartIdxRB];
        if ((m_pic->getCU(m_cuAddr)->getCUPelX() + g_rasterToPelX[absPartIdx] + m_pic->getMinCUWidth()) >= m_slice->getSPS()->getPicWidthInLumaSamples())  // image boundary check
        {
            uiLCUIdx = -1;
        }
        else if ((m_pic->getCU(m_cuAddr)->getCUPelY() + g_rasterToPelY[absPartIdx] + m_pic->getMinCUHeight()) >= m_slice->getSPS()->getPicHeightInLumaSamples())
        {
            uiLCUIdx = -1;
        }
        else
        {
            if ((absPartIdx % uiNumPartInCUWidth < uiNumPartInCUWidth - 1) &&        // is not at the last column of LCU
                (absPartIdx / uiNumPartInCUWidth < m_pic->getNumPartInHeight() - 1)) // is not at the last row    of LCU
            {
                uiAbsPartAddr = g_rasterToZscan[absPartIdx + uiNumPartInCUWidth + 1];
                uiLCUIdx = getAddr();
            }
            else if (absPartIdx % uiNumPartInCUWidth < uiNumPartInCUWidth - 1)       // is not at the last column of LCU But is last row of LCU
            {
                uiAbsPartAddr = g_rasterToZscan[(absPartIdx + uiNumPartInCUWidth + 1) % m_pic->getNumPartInCU()];
                uiLCUIdx      = -1;
            }
            else if (absPartIdx / uiNumPartInCUWidth < m_pic->getNumPartInHeight() - 1) // is not at the last row of LCU But is last column of LCU
            {
                uiAbsPartAddr = g_rasterToZscan[absPartIdx + 1];
                uiLCUIdx = getAddr() + 1;
            }
            else //is the right bottom corner of LCU
            {
                uiAbsPartAddr = 0;
                uiLCUIdx      = -1;
            }
        }
        if (uiLCUIdx >= 0 && xGetColMVP(picList, uiLCUIdx, uiAbsPartAddr, cColMv, iRefIdx_Col))
        {
            info->m_mvCand[info->m_num++] = cColMv;
        }
        else
        {
            UInt uiPartIdxCenter;
            UInt uiCurLCUIdx = getAddr();
            xDeriveCenterIdx(partIdx, uiPartIdxCenter);
            if (xGetColMVP(picList, uiCurLCUIdx, uiPartIdxCenter,  cColMv, iRefIdx_Col))
            {
                info->m_mvCand[info->m_num++] = cColMv;
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

Bool TComDataCU::isBipredRestriction(UInt puIdx)
{
    Int width = 0;
    Int height = 0;
    UInt partAddr;

    getPartIndexAndSize(puIdx, partAddr, width, height);
    if (getWidth(0) == 8 && (width < 8 || height < 8))
    {
        return true;
    }
    return false;
}

Void TComDataCU::clipMv(MV& outMV)
{
    Int iMvShift = 2;
    Int iOffset = 8;
    Int iHorMax = (m_slice->getSPS()->getPicWidthInLumaSamples() + iOffset - m_cuPelX - 1) << iMvShift;
    Int iHorMin = (-(Int)g_maxCUWidth - iOffset - (Int)m_cuPelX + 1) << iMvShift;

    Int iVerMax = (m_slice->getSPS()->getPicHeightInLumaSamples() + iOffset - m_cuPelY - 1) << iMvShift;
    Int iVerMin = (-(Int)g_maxCUHeight - iOffset - (Int)m_cuPelY + 1) << iMvShift;

    outMV.x = min(iHorMax, max(iHorMin, (Int)outMV.x));
    outMV.y = min(iVerMax, max(iVerMin, (Int)outMV.y));
}

UInt TComDataCU::getIntraSizeIdx(UInt absPartIdx)
{
    UInt uiShift = ((m_trIdx[absPartIdx] == 0) && (m_partSizes[absPartIdx] == SIZE_NxN)) ? m_trIdx[absPartIdx] + 1 : m_trIdx[absPartIdx];

    uiShift = (m_partSizes[absPartIdx] == SIZE_NxN ? 1 : 0);

    UChar width = m_width[absPartIdx] >> uiShift;
    UInt  uiCnt = 0;
    while (width)
    {
        uiCnt++;
        width >>= 1;
    }

    uiCnt -= 2;
    return uiCnt > 6 ? 6 : uiCnt;
}

Void TComDataCU::clearCbf(UInt uiIdx, TextType ttype, UInt uiNumParts)
{
    ::memset(&m_cbf[g_convertTxtTypeToIdx[ttype]][uiIdx], 0, sizeof(UChar) * uiNumParts);
}

/** Set a I_PCM flag for all sub-partitions of a partition.
 * \param bIpcmFlag I_PCM flag
 * \param absPartIdx patition index
 * \param depth CU depth
 * \returns Void
 */
Void TComDataCU::setIPCMFlagSubParts(Bool bIpcmFlag, UInt absPartIdx, UInt depth)
{
    UInt uiCurrPartNumb = m_pic->getNumPartInCU() >> (depth << 1);

    memset(m_pbIPCMFlag + absPartIdx, bIpcmFlag, sizeof(Bool) * uiCurrPartNumb);
}

/** Test whether the current block is skipped
 * \param partIdx Block index
 * \returns Flag indicating whether the block is skipped
 */
Bool TComDataCU::isSkipped(UInt partIdx)
{
    return getSkipFlag(partIdx);
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

#if _MSC_VER
#pragma warning(disable: 4701) // potentially uninitialized local variables
#endif

Bool TComDataCU::xAddMVPCand(AMVPInfo* info, RefPicList picList, Int refIdx, UInt partUnitIdx, MVP_DIR dir)
{
    TComDataCU* pcTmpCU = NULL;
    UInt uiIdx = 0;

    switch (dir)
    {
    case MD_LEFT:
    {
        pcTmpCU = getPULeft(uiIdx, partUnitIdx);
        break;
    }
    case MD_ABOVE:
    {
        pcTmpCU = getPUAbove(uiIdx, partUnitIdx);
        break;
    }
    case MD_ABOVE_RIGHT:
    {
        pcTmpCU = getPUAboveRight(uiIdx, partUnitIdx);
        break;
    }
    case MD_BELOW_LEFT:
    {
        pcTmpCU = getPUBelowLeft(uiIdx, partUnitIdx);
        break;
    }
    case MD_ABOVE_LEFT:
    {
        pcTmpCU = getPUAboveLeft(uiIdx, partUnitIdx);
        break;
    }
    default:
    {
        break;
    }
    }

    if (pcTmpCU == NULL)
    {
        return false;
    }

    if (pcTmpCU->getCUMvField(picList)->getRefIdx(uiIdx) >= 0 && m_slice->getRefPic(picList, refIdx)->getPOC() == pcTmpCU->getSlice()->getRefPOC(picList, pcTmpCU->getCUMvField(picList)->getRefIdx(uiIdx)))
    {
        MV cMvPred = pcTmpCU->getCUMvField(picList)->getMv(uiIdx);

        info->m_mvCand[info->m_num++] = cMvPred;
        return true;
    }

    RefPicList eRefPicList2nd = REF_PIC_LIST_0;
    if (picList == REF_PIC_LIST_0)
    {
        eRefPicList2nd = REF_PIC_LIST_1;
    }
    else if (picList == REF_PIC_LIST_1)
    {
        eRefPicList2nd = REF_PIC_LIST_0;
    }

    Int curRefPOC = m_slice->getRefPic(picList, refIdx)->getPOC();
    Int iNeibRefPOC;

    if (pcTmpCU->getCUMvField(eRefPicList2nd)->getRefIdx(uiIdx) >= 0)
    {
        iNeibRefPOC = pcTmpCU->getSlice()->getRefPOC(eRefPicList2nd, pcTmpCU->getCUMvField(eRefPicList2nd)->getRefIdx(uiIdx));
        if (iNeibRefPOC == curRefPOC) // Same Reference Frame But Diff List//
        {
            MV cMvPred = pcTmpCU->getCUMvField(eRefPicList2nd)->getMv(uiIdx);
            info->m_mvCand[info->m_num++] = cMvPred;
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
 * \returns Bool
 */
Bool TComDataCU::xAddMVPCandOrder(AMVPInfo* info, RefPicList picList, Int refIdx, UInt partUnitIdx, MVP_DIR dir)
{
    TComDataCU* pcTmpCU = NULL;
    UInt uiIdx = 0;

    switch (dir)
    {
    case MD_LEFT:
    {
        pcTmpCU = getPULeft(uiIdx, partUnitIdx);
        break;
    }
    case MD_ABOVE:
    {
        pcTmpCU = getPUAbove(uiIdx, partUnitIdx);
        break;
    }
    case MD_ABOVE_RIGHT:
    {
        pcTmpCU = getPUAboveRight(uiIdx, partUnitIdx);
        break;
    }
    case MD_BELOW_LEFT:
    {
        pcTmpCU = getPUBelowLeft(uiIdx, partUnitIdx);
        break;
    }
    case MD_ABOVE_LEFT:
    {
        pcTmpCU = getPUAboveLeft(uiIdx, partUnitIdx);
        break;
    }
    default:
    {
        break;
    }
    }

    if (pcTmpCU == NULL)
    {
        return false;
    }

    RefPicList eRefPicList2nd = REF_PIC_LIST_0;
    if (picList == REF_PIC_LIST_0)
    {
        eRefPicList2nd = REF_PIC_LIST_1;
    }
    else if (picList == REF_PIC_LIST_1)
    {
        eRefPicList2nd = REF_PIC_LIST_0;
    }

    Int curPOC = m_slice->getPOC();
    Int curRefPOC = m_slice->getRefPic(picList, refIdx)->getPOC();
    Int iNeibPOC = curPOC;
    Int iNeibRefPOC;

    Bool bIsCurrRefLongTerm = m_slice->getRefPic(picList, refIdx)->getIsLongTerm();
    Bool bIsNeibRefLongTerm = false;
    //---------------  V1 (END) ------------------//
    if (pcTmpCU->getCUMvField(picList)->getRefIdx(uiIdx) >= 0)
    {
        iNeibRefPOC = pcTmpCU->getSlice()->getRefPOC(picList, pcTmpCU->getCUMvField(picList)->getRefIdx(uiIdx));
        MV cMvPred = pcTmpCU->getCUMvField(picList)->getMv(uiIdx);
        MV outMV;

        bIsNeibRefLongTerm = pcTmpCU->getSlice()->getRefPic(picList, pcTmpCU->getCUMvField(picList)->getRefIdx(uiIdx))->getIsLongTerm();
        if (bIsCurrRefLongTerm == bIsNeibRefLongTerm)
        {
            if (bIsCurrRefLongTerm || bIsNeibRefLongTerm)
            {
                outMV = cMvPred;
            }
            else
            {
                Int iScale = xGetDistScaleFactor(curPOC, curRefPOC, iNeibPOC, iNeibRefPOC);
                if (iScale == 4096)
                {
                    outMV = cMvPred;
                }
                else
                {
                    outMV = scaleMv(cMvPred, iScale);
                }
            }
            info->m_mvCand[info->m_num++] = outMV;
            return true;
        }
    }
    //---------------------- V2(END) --------------------//
    if (pcTmpCU->getCUMvField(eRefPicList2nd)->getRefIdx(uiIdx) >= 0)
    {
        iNeibRefPOC = pcTmpCU->getSlice()->getRefPOC(eRefPicList2nd, pcTmpCU->getCUMvField(eRefPicList2nd)->getRefIdx(uiIdx));
        MV cMvPred = pcTmpCU->getCUMvField(eRefPicList2nd)->getMv(uiIdx);
        MV outMV;

        bIsNeibRefLongTerm = pcTmpCU->getSlice()->getRefPic(eRefPicList2nd, pcTmpCU->getCUMvField(eRefPicList2nd)->getRefIdx(uiIdx))->getIsLongTerm();
        if (bIsCurrRefLongTerm == bIsNeibRefLongTerm)
        {
            if (bIsCurrRefLongTerm || bIsNeibRefLongTerm)
            {
                outMV = cMvPred;
            }
            else
            {
                Int iScale = xGetDistScaleFactor(curPOC, curRefPOC, iNeibPOC, iNeibRefPOC);
                if (iScale == 4096)
                {
                    outMV = cMvPred;
                }
                else
                {
                    outMV = scaleMv(cMvPred, iScale);
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
 * \returns Bool
 */
Bool TComDataCU::xGetColMVP(RefPicList picList, Int cuAddr, Int partUnitIdx, MV& outMV, Int& outRefIdx)
{
    UInt uiAbsPartAddr = partUnitIdx;

    RefPicList  eColRefPicList;
    Int colPOC, colRefPOC, curPOC, curRefPOC, iScale;
    MV cColMv;

    // use coldir.
    TComPic *pColPic = getSlice()->getRefPic(RefPicList(getSlice()->isInterB() ? 1 - getSlice()->getColFromL0Flag() : 0), getSlice()->getColRefIdx());
    TComDataCU *pColCU = pColPic->getCU(cuAddr);

    if (pColCU->getPic() == 0 || pColCU->getPartitionSize(partUnitIdx) == SIZE_NONE)
    {
        return false;
    }
    curPOC = m_slice->getPOC();
    curRefPOC = m_slice->getRefPic(picList, outRefIdx)->getPOC();
    colPOC = pColCU->getSlice()->getPOC();

    if (pColCU->isIntra(uiAbsPartAddr))
    {
        return false;
    }
    eColRefPicList = getSlice()->getCheckLDC() ? picList : RefPicList(getSlice()->getColFromL0Flag());

    Int iColRefIdx = pColCU->getCUMvField(RefPicList(eColRefPicList))->getRefIdx(uiAbsPartAddr);

    if (iColRefIdx < 0)
    {
        eColRefPicList = RefPicList(1 - eColRefPicList);
        iColRefIdx = pColCU->getCUMvField(RefPicList(eColRefPicList))->getRefIdx(uiAbsPartAddr);

        if (iColRefIdx < 0)
        {
            return false;
        }
    }

    // Scale the vector.
    colRefPOC = pColCU->getSlice()->getRefPOC(eColRefPicList, iColRefIdx);
    cColMv = pColCU->getCUMvField(eColRefPicList)->getMv(uiAbsPartAddr);

    curRefPOC = m_slice->getRefPic(picList, outRefIdx)->getPOC();
    Bool bIsCurrRefLongTerm = m_slice->getRefPic(picList, outRefIdx)->getIsLongTerm();
    Bool bIsColRefLongTerm = pColCU->getSlice()->getIsUsedAsLongTerm(eColRefPicList, iColRefIdx);

    if (bIsCurrRefLongTerm != bIsColRefLongTerm)
    {
        return false;
    }

    if (bIsCurrRefLongTerm || bIsColRefLongTerm)
    {
        outMV = cColMv;
    }
    else
    {
        iScale = xGetDistScaleFactor(curPOC, curRefPOC, colPOC, colRefPOC);
        if (iScale == 4096)
        {
            outMV = cColMv;
        }
        else
        {
            outMV = scaleMv(cColMv, iScale);
        }
    }
    return true;
}

Int TComDataCU::xGetDistScaleFactor(Int curPOC, Int curRefPOC, Int colPOC, Int colRefPOC)
{
    Int iDiffPocD = colPOC - colRefPOC;
    Int iDiffPocB = curPOC - curRefPOC;

    if (iDiffPocD == iDiffPocB)
    {
        return 4096;
    }
    else
    {
        Int iTDB      = Clip3(-128, 127, iDiffPocB);
        Int iTDD      = Clip3(-128, 127, iDiffPocD);
        Int iX        = (0x4000 + abs(iTDD / 2)) / iTDD;
        Int iScale    = Clip3(-4096, 4095, (iTDB * iX + 32) >> 6);
        return iScale;
    }
}

/**
 * \param cuMode
 * \param partIdx
 * \param outPartIdxCenter
 * \returns Void
 */
Void TComDataCU::xDeriveCenterIdx(UInt partIdx, UInt& outPartIdxCenter)
{
    UInt partAddr;
    Int  iPartWidth;
    Int  iPartHeight;

    getPartIndexAndSize(partIdx, partAddr, iPartWidth, iPartHeight);

    outPartIdxCenter = m_absIdxInLCU + partAddr; // partition origin.
    outPartIdxCenter = g_rasterToZscan[g_zscanToRaster[outPartIdxCenter]
                                       + (iPartHeight / m_pic->getMinCUHeight()) / 2 * m_pic->getNumPartInWidth()
                                       + (iPartWidth / m_pic->getMinCUWidth()) / 2];
}

Void TComDataCU::compressMV()
{
    Int scaleFactor = 4 * AMVP_DECIMATION_FACTOR / m_unitSize;

    if (scaleFactor > 0)
    {
        m_cuMvField[0].compress(m_predModes, scaleFactor);
        m_cuMvField[1].compress(m_predModes, scaleFactor);
    }
}

UInt TComDataCU::getCoefScanIdx(UInt absPartIdx, UInt width, Bool bIsLuma, Bool bIsIntra)
{
    UInt uiCTXIdx;
    UInt uiScanIdx;
    UInt dirMode;

    if (!bIsIntra)
    {
        uiScanIdx = SCAN_DIAG;
        return uiScanIdx;
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
        uiScanIdx = SCAN_DIAG;
        if (uiCTXIdx > 3 && uiCTXIdx < 6) //if multiple scans supported for transform size
        {
            uiScanIdx = abs((Int)dirMode - VER_IDX) < 5 ? SCAN_HOR : (abs((Int)dirMode - HOR_IDX) < 5 ? SCAN_VER : SCAN_DIAG);
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
        uiScanIdx = SCAN_DIAG;
        if (uiCTXIdx > 4 && uiCTXIdx < 7) //if multiple scans supported for transform size
        {
            uiScanIdx = abs((Int)dirMode - VER_IDX) < 5 ? SCAN_HOR : (abs((Int)dirMode - HOR_IDX) < 5 ? SCAN_VER : SCAN_DIAG);
        }
    }

    return uiScanIdx;
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
Void TComDataCU::setNDBFilterBlockBorderAvailability(UInt numLCUInPicWidth, UInt /*numLCUInPicHeight*/, UInt numSUInLCUWidth, UInt numSUInLCUHeight, UInt picWidth, UInt picHeight
                                                     , Bool bTopTileBoundary, Bool bDownTileBoundary, Bool bLeftTileBoundary, Bool bRightTileBoundary
                                                     , Bool bIndependentTileBoundaryEnabled)
{
    UInt numSUInLCU = numSUInLCUWidth * numSUInLCUHeight;
    UInt lpelx, tpely;
    UInt width, height;
    Bool bPicRBoundary, bPicBBoundary, bPicTBoundary, bPicLBoundary;
    Bool bLCURBoundary = false, bLCUBBoundary = false, bLCUTBoundary = false, bLCULBoundary = false;
    Bool* pbAvailBorder;
    Bool* pbAvail;
    UInt rTLSU, rBRSU, widthSU;
    UInt rTRefSU = 0, rBRefSU = 0, rLRefSU = 0, rRRefSU = 0;
    Int* pRRefMapLCU = NULL;
    Int* pLRefMapLCU = NULL;
    Int* pTRefMapLCU = NULL;
    Int* pBRefMapLCU = NULL;
    UInt numSGU = (UInt)m_vNDFBlock.size();

    for (Int i = 0; i < numSGU; i++)
    {
        NDBFBlockInfo& rSGU = m_vNDFBlock[i];

        lpelx = rSGU.posX;
        tpely = rSGU.posY;
        width   = rSGU.width;
        height  = rSGU.height;
        rTLSU     = g_zscanToRaster[rSGU.startSU];
        rBRSU     = g_zscanToRaster[rSGU.endSU];
        widthSU   = rSGU.widthSU;

        pbAvailBorder = rSGU.isBorderAvailable;

        bPicTBoundary = (tpely == 0) ? (true) : (false);
        bPicLBoundary = (lpelx == 0) ? (true) : (false);
        bPicRBoundary = (!(lpelx + width < picWidth)) ? (true) : (false);
        bPicBBoundary = (!(tpely + height < picHeight)) ? (true) : (false);

        bLCULBoundary = (rTLSU % numSUInLCUWidth == 0) ? (true) : (false);
        bLCURBoundary = ((rTLSU + widthSU) % numSUInLCUWidth == 0) ? (true) : (false);
        bLCUTBoundary = ((UInt)(rTLSU / numSUInLCUWidth) == 0) ? (true) : (false);
        bLCUBBoundary = ((UInt)(rBRSU / numSUInLCUWidth) == (numSUInLCUHeight - 1)) ? (true) : (false);

        //       SGU_L
        pbAvail = &(pbAvailBorder[SGU_L]);
        if (bPicLBoundary)
        {
            *pbAvail = false;
        }
        else
        {
            *pbAvail = true;
        }

        //       SGU_R
        pbAvail = &(pbAvailBorder[SGU_R]);
        if (bPicRBoundary)
        {
            *pbAvail = false;
        }
        else
        {
            *pbAvail = true;
        }

        //       SGU_T
        pbAvail = &(pbAvailBorder[SGU_T]);
        if (bPicTBoundary)
        {
            *pbAvail = false;
        }
        else
        {
            *pbAvail = true;
        }

        //       SGU_B
        pbAvail = &(pbAvailBorder[SGU_B]);
        if (bPicBBoundary)
        {
            *pbAvail = false;
        }
        else
        {
            *pbAvail = true;
        }

        //       SGU_TL
        pbAvail = &(pbAvailBorder[SGU_TL]);
        if (bPicTBoundary || bPicLBoundary)
        {
            *pbAvail = false;
        }
        else
        {
            *pbAvail = true;
        }

        //       SGU_TR
        pbAvail = &(pbAvailBorder[SGU_TR]);
        if (bPicTBoundary || bPicRBoundary)
        {
            *pbAvail = false;
        }
        else
        {
            *pbAvail = true;
        }

        //       SGU_BL
        pbAvail = &(pbAvailBorder[SGU_BL]);
        if (bPicBBoundary || bPicLBoundary)
        {
            *pbAvail = false;
        }
        else
        {
            *pbAvail = true;
        }

        //       SGU_BR
        pbAvail = &(pbAvailBorder[SGU_BR]);
        if (bPicBBoundary || bPicRBoundary)
        {
            *pbAvail = false;
        }
        else
        {
            *pbAvail = true;
        }

        if (bIndependentTileBoundaryEnabled)
        {
            //left LCU boundary
            if (!bPicLBoundary && bLCULBoundary)
            {
                if (bLeftTileBoundary)
                {
                    pbAvailBorder[SGU_L] = pbAvailBorder[SGU_TL] = pbAvailBorder[SGU_BL] = false;
                }
            }
            //right LCU boundary
            if (!bPicRBoundary && bLCURBoundary)
            {
                if (bRightTileBoundary)
                {
                    pbAvailBorder[SGU_R] = pbAvailBorder[SGU_TR] = pbAvailBorder[SGU_BR] = false;
                }
            }
            //top LCU boundary
            if (!bPicTBoundary && bLCUTBoundary)
            {
                if (bTopTileBoundary)
                {
                    pbAvailBorder[SGU_T] = pbAvailBorder[SGU_TL] = pbAvailBorder[SGU_TR] = false;
                }
            }
            //down LCU boundary
            if (!bPicBBoundary && bLCUBBoundary)
            {
                if (bDownTileBoundary)
                {
                    pbAvailBorder[SGU_B] = pbAvailBorder[SGU_BL] = pbAvailBorder[SGU_BR] = false;
                }
            }
        }
        rSGU.allBordersAvailable = true;
        for (Int b = 0; b < NUM_SGU_BORDER; b++)
        {
            if (pbAvailBorder[b] == false)
            {
                rSGU.allBordersAvailable = false;
                break;
            }
        }
    }
}

//! \}
