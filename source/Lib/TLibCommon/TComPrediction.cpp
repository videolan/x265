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

/** \file     TComPrediction.cpp
    \brief    prediction class
*/

#include <memory.h>
#include "TComPrediction.h"
#include "primitives.h"

using namespace x265;

//! \ingroup TLibCommon
//! \{

const UChar m_aucIntraFilter[5] =
{
    10, //4x4
    7, //8x8
    1, //16x16
    0, //32x32
    10, //64x64
};

// ====================================================================================================================
// Constructor / destructor / initialize
// ====================================================================================================================

TComPrediction::TComPrediction()
    : m_pLumaRecBuffer(0)
    , m_iLumaRecStride(0)
{
    m_piPredBuf = NULL;
}

TComPrediction::~TComPrediction()
{
    delete[] m_piPredBuf;

    xFree(refAbove);
    xFree(refAboveFlt);
    xFree(refLeft);
    xFree(refLeftFlt);

    m_acYuvPred[0].destroy();
    m_acYuvPred[1].destroy();

    m_cYuvPredTemp.destroy();

    if (m_pLumaRecBuffer)
    {
        delete [] m_pLumaRecBuffer;
    }

    Int i, j;
    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; j++)
        {
            m_filteredBlock[i][j].destroy();
        }

        filteredBlockTmp[i].destroy();
    }
}

Void TComPrediction::initTempBuff()
{
    if (m_piPredBuf == NULL)
    {
        Int extWidth  = MAX_CU_SIZE + 16;
        Int extHeight = MAX_CU_SIZE + 1;
        Int i, j;
        for (i = 0; i < 4; i++)
        {
            filteredBlockTmp[i].create(extWidth, extHeight + 7);
            for (j = 0; j < 4; j++)
            {
                m_filteredBlock[i][j].create(extWidth, extHeight);
            }
        }

        m_iPredBufHeight  = ((MAX_CU_SIZE + 2) << 4);
        m_iPredBufStride = ((MAX_CU_SIZE  + 8) << 4);
        m_piPredBuf = new Pel[m_iPredBufStride * m_iPredBufHeight];

        refAbove = (Pel*)xMalloc(Pel, 3 * MAX_CU_SIZE);
        refAboveFlt = (Pel*)xMalloc(Pel, 3 * MAX_CU_SIZE);
        refLeft = (Pel*)xMalloc(Pel, 3 * MAX_CU_SIZE);
        refLeftFlt = (Pel*)xMalloc(Pel, 3 * MAX_CU_SIZE);

        // new structure
        m_acYuvPred[0].create(MAX_CU_SIZE, MAX_CU_SIZE);
        m_acYuvPred[1].create(MAX_CU_SIZE, MAX_CU_SIZE);

        m_cYuvPredTemp.create(MAX_CU_SIZE, MAX_CU_SIZE);
    }

    if (m_iLumaRecStride != (MAX_CU_SIZE >> 1) + 1)
    {
        m_iLumaRecStride =  (MAX_CU_SIZE >> 1) + 1;
        if (!m_pLumaRecBuffer)
        {
            m_pLumaRecBuffer = new Pel[m_iLumaRecStride * m_iLumaRecStride];
        }
    }
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================
Void xPredIntraPlanar(pixel* pSrc, intptr_t srcStride, pixel* rpDst, intptr_t dstStride, int width, int height);
Void xDCPredFiltering(Pel* pSrc, Int iSrcStride, Pel*& rpDst, Int iDstStride, Int iWidth, Int iHeight);
void xPredIntraAngBufRef(int bitDepth, pixel* /*pSrc*/, int /*srcStride*/, pixel*& rpDst, int dstStride, int width, int /*height*/, int dirMode, bool bFilter, pixel *refLeft, pixel *refAbove);

/** Function for deriving the simplified angular intra predictions.
 * \param pSrc pointer to reconstructed sample array
 * \param srcStride the stride of the reconstructed sample array
 * \param rpDst reference to pointer for the prediction sample array
 * \param dstStride the stride of the prediction sample array
 * \param width the width of the block
 * \param height the height of the block
 * \param dirMode the intra prediction mode index
 * \param blkAboveAvailable boolean indication if the block above is available
 * \param blkLeftAvailable boolean indication if the block to the left is available
 *
 * This function derives the prediction samples for the angular mode based on the prediction direction indicated by
 * the prediction mode index. The prediction direction is given by the displacement of the bottom row of the block and
 * the reference row above the block in the case of vertical prediction or displacement of the rightmost column
 * of the block and reference column left from the block in the case of the horizontal prediction. The displacement
 * is signalled at 1/32 pixel accuracy. When projection of the predicted pixel falls inbetween reference samples,
 * the predicted value for the pixel is linearly interpolated from the reference samples. All reference samples are taken
 * from the extended main reference.
 */
Void xPredIntraAng(Int bitDepth, Pel* pSrc, Int srcStride, Pel*& rpDst, Int dstStride, UInt width, UInt height, UInt dirMode, Bool blkAboveAvailable, Bool blkLeftAvailable, Bool bFilter)
{
    Int k, l;
    Int blkSize        = width;
    Pel* pDst          = rpDst;

    // Map the mode index to main prediction direction and angle
    assert(dirMode > 1); //no planar and dc
    Bool modeHor       = (dirMode < 18);
    Bool modeVer       = !modeHor;
    Int intraPredAngle = modeVer ? (Int)dirMode - VER_IDX : modeHor ? -((Int)dirMode - HOR_IDX) : 0;
    Int absAng         = abs(intraPredAngle);
    Int signAng        = intraPredAngle < 0 ? -1 : 1;

    // Set bitshifts and scale the angle parameter to block size
    Int angTable[9]    = { 0,    2,    5,   9,  13,  17,  21,  26,  32 };
    Int invAngTable[9] = { 0, 4096, 1638, 910, 630, 482, 390, 315, 256 }; // (256 * 32) / Angle
    Int invAngle       = invAngTable[absAng];
    absAng             = angTable[absAng];
    intraPredAngle     = signAng * absAng;

    // Do angular predictions
    {
        Pel* refMain;
        Pel* refSide;
        Pel  refAbove[2 * MAX_CU_SIZE + 1];
        Pel  refLeft[2 * MAX_CU_SIZE + 1];

        // Initialise the Main and Left reference array.
        if (intraPredAngle < 0)
        {
            for (k = 0; k < blkSize + 1; k++)
            {
                refAbove[k + blkSize - 1] = pSrc[k - srcStride - 1];
            }

            for (k = 0; k < blkSize + 1; k++)
            {
                refLeft[k + blkSize - 1] = pSrc[(k - 1) * srcStride - 1];
            }

            refMain = (modeVer ? refAbove : refLeft) + (blkSize - 1);
            refSide = (modeVer ? refLeft : refAbove) + (blkSize - 1);

            // Extend the Main reference to the left.
            Int invAngleSum    = 128; // rounding for (shift by 8)
            for (k = -1; k > blkSize * intraPredAngle >> 5; k--)
            {
                invAngleSum += invAngle;
                refMain[k] = refSide[invAngleSum >> 8];
            }
        }
        else
        {
            for (k = 0; k < 2 * blkSize + 1; k++)
            {
                refAbove[k] = pSrc[k - srcStride - 1];
            }

            for (k = 0; k < 2 * blkSize + 1; k++)
            {
                refLeft[k] = pSrc[(k - 1) * srcStride - 1];
            }

            refMain = modeVer ? refAbove : refLeft;
            refSide = modeVer ? refLeft  : refAbove;
        }

        if (intraPredAngle == 0)
        {
            for (k = 0; k < blkSize; k++)
            {
                for (l = 0; l < blkSize; l++)
                {
                    pDst[k * dstStride + l] = refMain[l + 1];
                }
            }

            if (bFilter)
            {
                for (k = 0; k < blkSize; k++)
                {
                    pDst[k * dstStride] = Clip3(0, (1 << bitDepth) - 1, static_cast<Short>(pDst[k * dstStride]) + ((refSide[k + 1] - refSide[0]) >> 1));
                }
            }
        }
        else
        {
            Int deltaPos = 0;
            Int deltaInt;
            Int deltaFract;
            Int refMainIndex;

            for (k = 0; k < blkSize; k++)
            {
                deltaPos += intraPredAngle;
                deltaInt   = deltaPos >> 5;
                deltaFract = deltaPos & (32 - 1);

                if (deltaFract)
                {
                    // Do linear filtering
                    for (l = 0; l < blkSize; l++)
                    {
                        refMainIndex        = l + deltaInt + 1;
                        pDst[k * dstStride + l] = (Pel)(((32 - deltaFract) * refMain[refMainIndex] + deltaFract * refMain[refMainIndex + 1] + 16) >> 5);
                    }
                }
                else
                {
                    // Just copy the integer samples
                    for (l = 0; l < blkSize; l++)
                    {
                        pDst[k * dstStride + l] = refMain[l + deltaInt + 1];
                    }
                }
            }
        }

        // Flip the block if this is the horizontal mode
        if (modeHor)
        {
            Pel  tmp;
            for (k = 0; k < blkSize - 1; k++)
            {
                for (l = k + 1; l < blkSize; l++)
                {
                    tmp                 = pDst[k * dstStride + l];
                    pDst[k * dstStride + l] = pDst[l * dstStride + k];
                    pDst[l * dstStride + k] = tmp;
                }
            }
        }
    }
}

Void TComPrediction::predIntraLumaAng(TComPattern* pcTComPattern, UInt uiDirMode, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight, Bool bAbove, Bool bLeft)
{
    Pel *pDst = piPred;
    Pel *ptrSrc;
    Pel *refLft, *refAbv;

    assert(g_aucConvertToBit[iWidth] >= 0);   //   4x  4
    assert(g_aucConvertToBit[iWidth] <= 5);   // 128x128
    assert(iWidth == iHeight);

    char log2BlkSize = g_aucConvertToBit[iWidth] + 2;

    ptrSrc = m_piPredBuf;
    assert(log2BlkSize >= 2 && log2BlkSize < 7);
    Int diff = min<Int>(abs((Int)uiDirMode - HOR_IDX), abs((Int)uiDirMode - VER_IDX));
    UChar ucFiltIdx = diff > m_aucIntraFilter[log2BlkSize - 2] ? 1 : 0;
    if (uiDirMode == DC_IDX)
    {
        ucFiltIdx = 0; //no smoothing for DC or LM chroma
    }

    assert(ucFiltIdx <= 1);

    refLft = refLeft + iWidth - 1;
    refAbv = refAbove + iWidth - 1;

    if (ucFiltIdx)
    {
        ptrSrc += ADI_BUF_STRIDE * (2 * iHeight + 1);
        refLft = refLeftFlt + iWidth - 1;
        refAbv = refAboveFlt + iWidth - 1;
    }

    // get starting pixel in block
    Int sw = ADI_BUF_STRIDE;
    Bool bFilter = ((iWidth <= 16) && (iHeight <= 16));

    // Create the prediction
    if (uiDirMode == PLANAR_IDX)
    {
        primitives.getIPredPlanar((pixel*)ptrSrc + sw + 1, sw, (pixel*)pDst, uiStride, iWidth, iHeight);
    }
    else if (uiDirMode == DC_IDX)
    {
        primitives.getIPredDC((pixel*)ptrSrc + sw + 1, sw, (pixel*)pDst, uiStride, iWidth, iHeight, bAbove, bLeft, bFilter);
    }
    else
    {
        primitives.getIPredAng(g_bitDepthY, (pixel*)pDst, uiStride, iWidth, uiDirMode, bFilter, (pixel*)refLft, (pixel*)refAbv);
    }
}

// Angular chroma
Void TComPrediction::predIntraChromaAng(Pel* piSrc, UInt uiDirMode, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight, Bool bAbove, Bool bLeft)
{
    Pel *pDst = piPred;
    Pel *ptrSrc = piSrc;

    // get starting pixel in block
    Int sw = ADI_BUF_STRIDE;

    if (uiDirMode == PLANAR_IDX)
    {
        primitives.getIPredPlanar((pixel*)ptrSrc + sw + 1, sw, (pixel*)pDst, uiStride, iWidth, iHeight);
    }
    else if (uiDirMode == DC_IDX)
    {
        primitives.getIPredDC((pixel*)ptrSrc + sw + 1, sw, (pixel*)pDst, uiStride, iWidth, iHeight, bAbove, bLeft, false);
    }
    else
    {
         //Create the prediction
        int k;
        Pel refAbv[3 * MAX_CU_SIZE];
        Pel refLft[3 * MAX_CU_SIZE];
        int limit = ( uiDirMode <=25 && uiDirMode >=11 )? (iWidth + 1) : (2*iWidth+1);
        memcpy(refAbv + iWidth - 1, ptrSrc, (limit) * sizeof(Pel));
        for (k = 0; k < limit; k++)
        {
               refLft[k + iWidth - 1] = ptrSrc[k * sw];
        }

        primitives.getIPredAng(g_bitDepthC, (pixel*)pDst, uiStride, iWidth, uiDirMode, false, (pixel*)refLft + iWidth -1, (pixel*)refAbv + iWidth -1);
  
    }
}

/** Function for checking identical motion.
 * \param TComDataCU* pcCU
 * \param UInt PartAddr
 */
Bool TComPrediction::xCheckIdenticalMotion(TComDataCU* pcCU, UInt PartAddr)
{
    if (pcCU->getSlice()->isInterB() && !pcCU->getSlice()->getPPS()->getWPBiPred())
    {
        if (pcCU->getCUMvField(REF_PIC_LIST_0)->getRefIdx(PartAddr) >= 0 && pcCU->getCUMvField(REF_PIC_LIST_1)->getRefIdx(PartAddr) >= 0)
        {
            Int RefPOCL0 = pcCU->getSlice()->getRefPic(REF_PIC_LIST_0, pcCU->getCUMvField(REF_PIC_LIST_0)->getRefIdx(PartAddr))->getPOC();
            Int RefPOCL1 = pcCU->getSlice()->getRefPic(REF_PIC_LIST_1, pcCU->getCUMvField(REF_PIC_LIST_1)->getRefIdx(PartAddr))->getPOC();
            if (RefPOCL0 == RefPOCL1 && pcCU->getCUMvField(REF_PIC_LIST_0)->getMv(PartAddr) == pcCU->getCUMvField(REF_PIC_LIST_1)->getMv(PartAddr))
            {
                return true;
            }
        }
    }
    return false;
}

Void TComPrediction::motionCompensation(TComDataCU* pcCU, TComYuv* pcYuvPred, RefPicList eRefPicList, Int iPartIdx)
{
    Int         iWidth;
    Int         iHeight;
    UInt        uiPartAddr;

    if (iPartIdx >= 0)
    {
        pcCU->getPartIndexAndSize(iPartIdx, uiPartAddr, iWidth, iHeight);
        if (eRefPicList != REF_PIC_LIST_X)
        {
            if (pcCU->getSlice()->getPPS()->getUseWP())
            {
                xPredInterUni(pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcYuvPred, true);
            }
            else
            {
                xPredInterUni(pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcYuvPred);
            }
            if (pcCU->getSlice()->getPPS()->getUseWP())
            {
                xWeightedPredictionUni(pcCU, pcYuvPred, uiPartAddr, iWidth, iHeight, eRefPicList, pcYuvPred);
            }
        }
        else
        {
            if (xCheckIdenticalMotion(pcCU, uiPartAddr))
            {
                xPredInterUni(pcCU, uiPartAddr, iWidth, iHeight, REF_PIC_LIST_0, pcYuvPred);
            }
            else
            {
                xPredInterBi(pcCU, uiPartAddr, iWidth, iHeight, pcYuvPred);
            }
        }
        return;
    }

    for (iPartIdx = 0; iPartIdx < pcCU->getNumPartInter(); iPartIdx++)
    {
        pcCU->getPartIndexAndSize(iPartIdx, uiPartAddr, iWidth, iHeight);

        if (eRefPicList != REF_PIC_LIST_X)
        {
            if (pcCU->getSlice()->getPPS()->getUseWP())
            {
                xPredInterUni(pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcYuvPred, true);
            }
            else
            {
                xPredInterUni(pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcYuvPred);
            }
            if (pcCU->getSlice()->getPPS()->getUseWP())
            {
                xWeightedPredictionUni(pcCU, pcYuvPred, uiPartAddr, iWidth, iHeight, eRefPicList, pcYuvPred);
            }
        }
        else
        {
            if (xCheckIdenticalMotion(pcCU, uiPartAddr))
            {
                xPredInterUni(pcCU, uiPartAddr, iWidth, iHeight, REF_PIC_LIST_0, pcYuvPred);
            }
            else
            {
                xPredInterBi(pcCU, uiPartAddr, iWidth, iHeight, pcYuvPred);
            }
        }
    }
}

Void TComPrediction::xPredInterUni(TComDataCU* pcCU, UInt uiPartAddr, Int iWidth, Int iHeight, RefPicList eRefPicList, TComYuv*& rpcYuvPred, Bool bi)
{
    Int iRefIdx = pcCU->getCUMvField(eRefPicList)->getRefIdx(uiPartAddr);

    assert(iRefIdx >= 0);
    TComMv cMv = pcCU->getCUMvField(eRefPicList)->getMv(uiPartAddr);

    pcCU->clipMv(cMv);
    xPredInterLumaBlk(pcCU, pcCU->getSlice()->getRefPic(eRefPicList, iRefIdx)->getPicYuvRec(), uiPartAddr, &cMv, iWidth, iHeight, rpcYuvPred, bi);
    xPredInterChromaBlk(pcCU, pcCU->getSlice()->getRefPic(eRefPicList, iRefIdx)->getPicYuvRec(), uiPartAddr, &cMv, iWidth, iHeight, rpcYuvPred, bi);
}

Void TComPrediction::xPredInterBi(TComDataCU* pcCU, UInt uiPartAddr, Int iWidth, Int iHeight, TComYuv*& rpcYuvPred)
{
    TComYuv* pcMbYuv;
    Int      iRefIdx[2] = { -1, -1 };

    for (Int iRefList = 0; iRefList < 2; iRefList++)
    {
        RefPicList eRefPicList = (iRefList ? REF_PIC_LIST_1 : REF_PIC_LIST_0);
        iRefIdx[iRefList] = pcCU->getCUMvField(eRefPicList)->getRefIdx(uiPartAddr);

        if (iRefIdx[iRefList] < 0)
        {
            continue;
        }

        assert(iRefIdx[iRefList] < pcCU->getSlice()->getNumRefIdx(eRefPicList));

        pcMbYuv = &m_acYuvPred[iRefList];
        if (pcCU->getCUMvField(REF_PIC_LIST_0)->getRefIdx(uiPartAddr) >= 0 && pcCU->getCUMvField(REF_PIC_LIST_1)->getRefIdx(uiPartAddr) >= 0)
        {
            xPredInterUni(pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcMbYuv, true);
        }
        else
        {
            if ((pcCU->getSlice()->getPPS()->getUseWP()       && pcCU->getSlice()->getSliceType() == P_SLICE) ||
                (pcCU->getSlice()->getPPS()->getWPBiPred() && pcCU->getSlice()->getSliceType() == B_SLICE))
            {
                xPredInterUni(pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcMbYuv, true);
            }
            else
            {
                xPredInterUni(pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcMbYuv);
            }
        }
    }

    if (pcCU->getSlice()->getPPS()->getWPBiPred() && pcCU->getSlice()->getSliceType() == B_SLICE)
    {
        xWeightedPredictionBi(pcCU, &m_acYuvPred[0], &m_acYuvPred[1], iRefIdx[0], iRefIdx[1], uiPartAddr, iWidth, iHeight, rpcYuvPred);
    }
    else if (pcCU->getSlice()->getPPS()->getUseWP() && pcCU->getSlice()->getSliceType() == P_SLICE)
    {
        xWeightedPredictionUni(pcCU, &m_acYuvPred[0], uiPartAddr, iWidth, iHeight, REF_PIC_LIST_0, rpcYuvPred);
    }
    else
    {
        xWeightedAverage(&m_acYuvPred[0], &m_acYuvPred[1], iRefIdx[0], iRefIdx[1], uiPartAddr, iWidth, iHeight, rpcYuvPred);
    }
}

/**
 * \brief Generate motion-compensated luma block
 *
 * \param cu       Pointer to current CU
 * \param refPic   Pointer to reference picture
 * \param partAddr Address of block within CU
 * \param mv       Motion vector
 * \param width    Width of block
 * \param height   Height of block
 * \param dstPic   Pointer to destination picture
 * \param bi       Flag indicating whether bipred is used
 */
Void TComPrediction::xPredInterLumaBlk(TComDataCU *cu, TComPicYuv *refPic, UInt partAddr, TComMv *mv, Int width, Int height, TComYuv *&dstPic, Bool bi)
{
    assert(bi == false);

    Int refStride = refPic->getStride();
    Int refOffset = (mv->getHor() >> 2) + (mv->getVer() >> 2) * refStride;
    Pel *ref      =  refPic->getLumaAddr(cu->getAddr(), cu->getZorderIdxInCU() + partAddr) + refOffset;

    Int dstStride = dstPic->getStride();
    Pel *dst      = dstPic->getLumaAddr(partAddr);

    Int xFrac = mv->getHor() & 0x3;
    Int yFrac = mv->getVer() & 0x3;

    Pel* src = refPic->getLumaFilterBlock(yFrac, xFrac, cu->getAddr(), cu->getZorderIdxInCU() + partAddr) + refOffset;
    Int srcStride = refPic->getStride();

    x265::primitives.cpyblock(width, height, (pixel*)dst, dstStride, (pixel*)src, srcStride);
}

/**
 * \brief Generate motion-compensated chroma block
 *
 * \param cu       Pointer to current CU
 * \param refPic   Pointer to reference picture
 * \param partAddr Address of block within CU
 * \param mv       Motion vector
 * \param width    Width of block
 * \param height   Height of block
 * \param dstPic   Pointer to destination picture
 * \param bi       Flag indicating whether bipred is used
 */
Void TComPrediction::xPredInterChromaBlk(TComDataCU *cu, TComPicYuv *refPic, UInt partAddr, TComMv *mv, Int width, Int height, TComYuv *&dstPic, Bool bi)
{
    assert(bi == false);

    Int     refStride  = refPic->getCStride();
    Int     dstStride  = dstPic->getCStride();

    Int     refOffset  = (mv->getHor() >> 3) + (mv->getVer() >> 3) * refStride;

    Pel*    refCb     =  refPic->getCbAddr(cu->getAddr(), cu->getZorderIdxInCU() + partAddr) + refOffset;
    Pel*    refCr     =  refPic->getCrAddr(cu->getAddr(), cu->getZorderIdxInCU() + partAddr) + refOffset;

    Pel* dstCb =  dstPic->getCbAddr(partAddr);
    Pel* dstCr =  dstPic->getCrAddr(partAddr);

    Int     xFrac  = mv->getHor() & 0x7;
    Int     yFrac  = mv->getVer() & 0x7;
    UInt    cxWidth  = width  >> 1;
    UInt    cxHeight = height >> 1;

    Int filterSize = NTAPS_CHROMA;

    Int halfFilterSize = (filterSize >> 1);

    if (yFrac == 0)
    {
        if (xFrac == 0)
        {
            x265::primitives.cpyblock(cxWidth, cxHeight, (pixel*)dstCb, dstStride, (pixel*)refCb, refStride);
            x265::primitives.cpyblock(cxWidth, cxHeight, (pixel*)dstCr, dstStride, (pixel*)refCr, refStride);
        }
        else
        {
            primitives.ipFilter_p_p[FILTER_H_P_P_4](g_bitDepthC, (pixel*)refCb, refStride, (pixel*)dstCb,  dstStride, cxWidth, cxHeight, m_if.m_chromaFilter[xFrac]);
            primitives.ipFilter_p_p[FILTER_H_P_P_4](g_bitDepthC, (pixel*)refCr, refStride, (pixel*)dstCr,  dstStride, cxWidth, cxHeight, m_if.m_chromaFilter[xFrac]);
        }
    }
    else if (xFrac == 0)
    {
        primitives.ipFilter_p_p[FILTER_V_P_P_4](g_bitDepthC, (pixel*)refCb, refStride, (pixel*)dstCb, dstStride, cxWidth, cxHeight, m_if.m_chromaFilter[yFrac]);
        primitives.ipFilter_p_p[FILTER_V_P_P_4](g_bitDepthC, (pixel*)refCr, refStride, (pixel*)dstCr, dstStride, cxWidth, cxHeight, m_if.m_chromaFilter[yFrac]);
    }
    else
    {
        Int     extStride = cxWidth;
        Short*  extY      = (Short*)xMalloc(Short, cxWidth * (cxHeight + filterSize - 1));

        primitives.ipFilter_p_s[FILTER_H_P_S_4](g_bitDepthC, (pixel*)(refCb - (halfFilterSize - 1) * refStride), refStride, extY, extStride, cxWidth, cxHeight + filterSize - 1, m_if.m_chromaFilter[xFrac]);
        primitives.ipFilter_s_p[FILTER_V_S_P_4](g_bitDepthC, extY + (halfFilterSize - 1) * extStride, extStride, (pixel*)dstCb, dstStride, cxWidth, cxHeight, m_if.m_chromaFilter[yFrac]);

        primitives.ipFilter_p_s[FILTER_H_P_S_4](g_bitDepthC, (pixel*)(refCr - (halfFilterSize - 1) * refStride), refStride, extY, extStride, cxWidth, cxHeight + filterSize - 1,  m_if.m_chromaFilter[xFrac]);
        primitives.ipFilter_s_p[FILTER_V_S_P_4](g_bitDepthC, extY + (halfFilterSize - 1) * extStride, extStride, (pixel*)dstCr, dstStride, cxWidth, cxHeight,  m_if.m_chromaFilter[yFrac]);

        xFree(extY);
    }
}

Void TComPrediction::xWeightedAverage(TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, Int iRefIdx0, Int iRefIdx1, UInt uiPartIdx, Int iWidth, Int iHeight, TComYuv*& rpcYuvDst)
{
    if (iRefIdx0 >= 0 && iRefIdx1 >= 0)
    {
        rpcYuvDst->addAvg(pcYuvSrc0, pcYuvSrc1, uiPartIdx, iWidth, iHeight);
    }
    else if (iRefIdx0 >= 0 && iRefIdx1 <  0)
    {
        pcYuvSrc0->copyPartToPartYuv(rpcYuvDst, uiPartIdx, iWidth, iHeight);
    }
    else if (iRefIdx0 <  0 && iRefIdx1 >= 0)
    {
        pcYuvSrc1->copyPartToPartYuv(rpcYuvDst, uiPartIdx, iWidth, iHeight);
    }
}

// AMVP
Void TComPrediction::getMvPredAMVP(TComDataCU* pcCU, UInt uiPartIdx, UInt uiPartAddr, RefPicList eRefPicList, TComMv& rcMvPred)
{
    AMVPInfo* pcAMVPInfo = pcCU->getCUMvField(eRefPicList)->getAMVPInfo();

    if (pcAMVPInfo->iN <= 1)
    {
        rcMvPred = pcAMVPInfo->m_acMvCand[0];

        pcCU->setMVPIdxSubParts(0, eRefPicList, uiPartAddr, uiPartIdx, pcCU->getDepth(uiPartAddr));
        pcCU->setMVPNumSubParts(pcAMVPInfo->iN, eRefPicList, uiPartAddr, uiPartIdx, pcCU->getDepth(uiPartAddr));
        return;
    }

    assert(pcCU->getMVPIdx(eRefPicList, uiPartAddr) >= 0);
    rcMvPred = pcAMVPInfo->m_acMvCand[pcCU->getMVPIdx(eRefPicList, uiPartAddr)];
}

//! \}
