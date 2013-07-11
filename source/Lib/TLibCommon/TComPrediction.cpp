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
    7,  //8x8
    1,  //16x16
    0,  //32x32
    10, //64x64
};

// ====================================================================================================================
// Constructor / destructor / initialize
// ====================================================================================================================

TComPrediction::TComPrediction()
    : m_piPredBuf(NULL)
    , m_piPredAngBufs(NULL)
    , m_pLumaRecBuffer(0)
    , m_iLumaRecStride(0)
{}

TComPrediction::~TComPrediction()
{
    delete[] m_piPredBuf;
    xFree(m_piPredAngBufs);

    xFree(refAbove);
    xFree(refAboveFlt);
    xFree(refLeft);
    xFree(refLeftFlt);

    m_acYuvPred[0].destroy();
    m_acYuvPred[1].destroy();
    m_acShortPred[0].destroy();
    m_acShortPred[1].destroy();

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
        m_piPredAngBufs = (Pel*)xMalloc(Pel, 33 * MAX_CU_SIZE * MAX_CU_SIZE);

        refAbove = (Pel*)xMalloc(Pel, 3 * MAX_CU_SIZE);
        refAboveFlt = (Pel*)xMalloc(Pel, 3 * MAX_CU_SIZE);
        refLeft = (Pel*)xMalloc(Pel, 3 * MAX_CU_SIZE);
        refLeftFlt = (Pel*)xMalloc(Pel, 3 * MAX_CU_SIZE);

        // new structure
        m_acYuvPred[0].create(MAX_CU_SIZE, MAX_CU_SIZE);
        m_acYuvPred[1].create(MAX_CU_SIZE, MAX_CU_SIZE);
        m_acShortPred[0].create(MAX_CU_SIZE, MAX_CU_SIZE);
        m_acShortPred[1].create(MAX_CU_SIZE, MAX_CU_SIZE);

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
Void xPredIntraPlanar(pixel* src, intptr_t srcStride, pixel* rpDst, intptr_t dstStride, int width, int height);
Void xDCPredFiltering(Pel* src, Int srcstride, Pel*& rpDst, Int dststride, Int width, Int height);
void xPredIntraAngBufRef(int bitDepth, pixel* /*src*/, int /*srcStride*/, pixel*& rpDst, int dstStride, int width, int /*height*/, int dirMode, bool bFilter, pixel *refLeft, pixel *refAbove);

/** Function for deriving the simplified angular intra predictions.
 * \param src pointer to reconstructed sample array
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
Void xPredIntraAng(Int bitDepth, Pel* src, Int srcStride, Pel*& rpDst, Int dstStride, UInt width, UInt height, UInt dirMode, Bool blkAboveAvailable, Bool blkLeftAvailable, Bool bFilter)
{
    Int k, l;
    Int blkSize        = width;
    Pel* dst          = rpDst;

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
                refAbove[k + blkSize - 1] = src[k - srcStride - 1];
            }

            for (k = 0; k < blkSize + 1; k++)
            {
                refLeft[k + blkSize - 1] = src[(k - 1) * srcStride - 1];
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
                refAbove[k] = src[k - srcStride - 1];
            }

            for (k = 0; k < 2 * blkSize + 1; k++)
            {
                refLeft[k] = src[(k - 1) * srcStride - 1];
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
                    dst[k * dstStride + l] = refMain[l + 1];
                }
            }

            if (bFilter)
            {
                for (k = 0; k < blkSize; k++)
                {
                    dst[k * dstStride] = Clip3(0, (1 << bitDepth) - 1, static_cast<Short>(dst[k * dstStride]) + ((refSide[k + 1] - refSide[0]) >> 1));
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
                        dst[k * dstStride + l] = (Pel)(((32 - deltaFract) * refMain[refMainIndex] + deltaFract * refMain[refMainIndex + 1] + 16) >> 5);
                    }
                }
                else
                {
                    // Just copy the integer samples
                    for (l = 0; l < blkSize; l++)
                    {
                        dst[k * dstStride + l] = refMain[l + deltaInt + 1];
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
                    tmp                 = dst[k * dstStride + l];
                    dst[k * dstStride + l] = dst[l * dstStride + k];
                    dst[l * dstStride + k] = tmp;
                }
            }
        }
    }
}

Void TComPrediction::predIntraLumaAng(TComPattern* pcTComPattern, UInt uiDirMode, Pel* piPred, UInt uiStride, Int iSize)
{
    Pel *dst = piPred;
    Pel *ptrSrc;
    Pel *refLft, *refAbv;

    assert(g_convertToBit[iSize] >= 0);   //   4x  4
    assert(g_convertToBit[iSize] <= 5);   // 128x128

    char log2BlkSize = g_convertToBit[iSize] + 2;

    ptrSrc = m_piPredBuf;
    assert(log2BlkSize >= 2 && log2BlkSize < 7);
    Int diff = min<Int>(abs((Int)uiDirMode - HOR_IDX), abs((Int)uiDirMode - VER_IDX));
    UChar ucFiltIdx = diff > m_aucIntraFilter[log2BlkSize - 2] ? 1 : 0;
    if (uiDirMode == DC_IDX)
    {
        ucFiltIdx = 0; //no smoothing for DC or LM chroma
    }

    assert(ucFiltIdx <= 1);

    refLft = refLeft + iSize - 1;
    refAbv = refAbove + iSize - 1;

    if (ucFiltIdx)
    {
        ptrSrc += ADI_BUF_STRIDE * (2 * iSize + 1);
        refLft = refLeftFlt + iSize - 1;
        refAbv = refAboveFlt + iSize - 1;
    }

    // get starting pixel in block
    Int sw = ADI_BUF_STRIDE;
    Bool bFilter = (iSize <= 16);

    // Create the prediction
    if (uiDirMode == PLANAR_IDX)
    {
        primitives.intra_pred_planar((pixel*)ptrSrc + sw + 1, sw, (pixel*)dst, uiStride, iSize);
    }
    else if (uiDirMode == DC_IDX)
    {
        primitives.intra_pred_dc((pixel*)ptrSrc + sw + 1, sw, (pixel*)dst, uiStride, iSize, bFilter);
    }
    else
    {
        primitives.intra_pred_ang(g_bitDepthY, (pixel*)dst, uiStride, iSize, uiDirMode, bFilter, (pixel*)refLft, (pixel*)refAbv);
    }
}

// Angular chroma
Void TComPrediction::predIntraChromaAng(Pel* piSrc, UInt uiDirMode, Pel* piPred, UInt uiStride, Int width)
{
    Pel *dst = piPred;
    Pel *ptrSrc = piSrc;

    // get starting pixel in block
    Int sw = ADI_BUF_STRIDE;

    if (uiDirMode == PLANAR_IDX)
    {
        primitives.intra_pred_planar((pixel*)ptrSrc + sw + 1, sw, (pixel*)dst, uiStride, width);
    }
    else if (uiDirMode == DC_IDX)
    {
        primitives.intra_pred_dc((pixel*)ptrSrc + sw + 1, sw, (pixel*)dst, uiStride, width, false);
    }
    else
    {
        // Create the prediction
        Pel refAbv[3 * MAX_CU_SIZE];
        Pel refLft[3 * MAX_CU_SIZE];
        int limit = (uiDirMode <= 25 && uiDirMode >= 11) ? (width + 1) : (2 * width + 1);
        memcpy(refAbv + width - 1, ptrSrc, (limit) * sizeof(Pel));
        for (int k = 0; k < limit; k++)
        {
            refLft[k + width - 1] = ptrSrc[k * sw];
        }

        primitives.intra_pred_ang(g_bitDepthC, (pixel*)dst, uiStride, width, uiDirMode, false, (pixel*)refLft + width - 1, (pixel*)refAbv + width - 1);
    }
}

/** Function for checking identical motion.
 * \param TComDataCU* cu
 * \param UInt PartAddr
 */
Bool TComPrediction::xCheckIdenticalMotion(TComDataCU* cu, UInt PartAddr)
{
    if (cu->getSlice()->isInterB() && !cu->getSlice()->getPPS()->getWPBiPred())
    {
        if (cu->getCUMvField(REF_PIC_LIST_0)->getRefIdx(PartAddr) >= 0 && cu->getCUMvField(REF_PIC_LIST_1)->getRefIdx(PartAddr) >= 0)
        {
            Int RefPOCL0 = cu->getSlice()->getRefPic(REF_PIC_LIST_0, cu->getCUMvField(REF_PIC_LIST_0)->getRefIdx(PartAddr))->getPOC();
            Int RefPOCL1 = cu->getSlice()->getRefPic(REF_PIC_LIST_1, cu->getCUMvField(REF_PIC_LIST_1)->getRefIdx(PartAddr))->getPOC();
            if (RefPOCL0 == RefPOCL1 && cu->getCUMvField(REF_PIC_LIST_0)->getMv(PartAddr) == cu->getCUMvField(REF_PIC_LIST_1)->getMv(PartAddr))
            {
                return true;
            }
        }
    }
    return false;
}

Void TComPrediction::motionCompensation(TComDataCU* cu, TComYuv* predYuv, RefPicList picList, Int partIdx)
{
    Int         width;
    Int         height;
    UInt        partAddr;

    if (partIdx >= 0)
    {
        cu->getPartIndexAndSize(partIdx, partAddr, width, height);
        if (picList != REF_PIC_LIST_X)
        {
            if (cu->getSlice()->getPPS()->getUseWP())
            {
                TShortYUV* pcMbYuv = &m_acShortPred[0];
                xPredInterUni(cu, partAddr, width, height, picList, pcMbYuv, true);
                xWeightedPredictionUni(cu, pcMbYuv, partAddr, width, height, picList, predYuv);
            }
            else
            {
                xPredInterUni(cu, partAddr, width, height, picList, predYuv);
            }
        }
        else
        {
            if (xCheckIdenticalMotion(cu, partAddr))
            {
                xPredInterUni(cu, partAddr, width, height, REF_PIC_LIST_0, predYuv);
            }
            else
            {
                xPredInterBi(cu, partAddr, width, height, predYuv);
            }
        }
        return;
    }

    for (partIdx = 0; partIdx < cu->getNumPartInter(); partIdx++)
    {
        cu->getPartIndexAndSize(partIdx, partAddr, width, height);

        if (picList != REF_PIC_LIST_X)
        {
            if (cu->getSlice()->getPPS()->getUseWP())
            {
                TShortYUV* pcMbYuv = &m_acShortPred[0];
                xPredInterUni(cu, partAddr, width, height, picList, pcMbYuv, true);
                xWeightedPredictionUni(cu, pcMbYuv, partAddr, width, height, picList, predYuv);
            }
            else
            {
                xPredInterUni(cu, partAddr, width, height, picList, predYuv);
            }
        }
        else
        {
            if (xCheckIdenticalMotion(cu, partAddr))
            {
                xPredInterUni(cu, partAddr, width, height, REF_PIC_LIST_0, predYuv);
            }
            else
            {
                xPredInterBi(cu, partAddr, width, height, predYuv);
            }
        }
    }
}

Void TComPrediction::xPredInterUni(TComDataCU* cu, UInt partAddr, Int width, Int height, RefPicList picList, TComYuv*& rpcYuvPred, Bool bi)
{
    assert(bi == false);
    Int iRefIdx = cu->getCUMvField(picList)->getRefIdx(partAddr);

    assert(iRefIdx >= 0);
    MV cMv = cu->getCUMvField(picList)->getMv(partAddr);

    cu->clipMv(cMv);
    xPredInterLumaBlk(cu, cu->getSlice()->getRefPic(picList, iRefIdx)->getPicYuvRec(), partAddr, &cMv, width, height, rpcYuvPred, bi);
    xPredInterChromaBlk(cu, cu->getSlice()->getRefPic(picList, iRefIdx)->getPicYuvRec(), partAddr, &cMv, width, height, rpcYuvPred, bi);
}

Void TComPrediction::xPredInterUni(TComDataCU* cu, UInt partAddr, Int width, Int height, RefPicList picList, TShortYUV*& rpcYuvPred, Bool bi)
{
    assert(bi == true);

    Int iRefIdx = cu->getCUMvField(picList)->getRefIdx(partAddr);

    assert(iRefIdx >= 0);
    MV cMv = cu->getCUMvField(picList)->getMv(partAddr);

    cu->clipMv(cMv);
    xPredInterLumaBlk(cu, cu->getSlice()->getRefPic(picList, iRefIdx)->getPicYuvRec(), partAddr, &cMv, width, height, rpcYuvPred, bi);
    xPredInterChromaBlk(cu, cu->getSlice()->getRefPic(picList, iRefIdx)->getPicYuvRec(), partAddr, &cMv, width, height, rpcYuvPred, bi);
}

Void TComPrediction::xPredInterBi(TComDataCU* cu, UInt partAddr, Int width, Int height, TComYuv*& rpcYuvPred)
{
    Int      iRefIdx[2] = { -1, -1 };

    if (cu->getCUMvField(REF_PIC_LIST_0)->getRefIdx(partAddr) >= 0 && cu->getCUMvField(REF_PIC_LIST_1)->getRefIdx(partAddr) >= 0)
    {
        TShortYUV* pcMbYuv;
        for (Int refList = 0; refList < 2; refList++)
        {
            RefPicList picList = (refList ? REF_PIC_LIST_1 : REF_PIC_LIST_0);
            iRefIdx[refList] = cu->getCUMvField(picList)->getRefIdx(partAddr);

            assert(iRefIdx[refList] < cu->getSlice()->getNumRefIdx(picList));

            pcMbYuv = &m_acShortPred[refList];
            xPredInterUni(cu, partAddr, width, height, picList, pcMbYuv, true);
        }

        if (cu->getSlice()->getPPS()->getWPBiPred() && cu->getSlice()->getSliceType() == B_SLICE)
        {
            xWeightedPredictionBi(cu, &m_acShortPred[0], &m_acShortPred[1], iRefIdx[0], iRefIdx[1], partAddr, width, height, rpcYuvPred);
        }
        else
        {
            rpcYuvPred->addAvg(&m_acShortPred[0], &m_acShortPred[1], partAddr, width, height);
        }
    }
    else if (cu->getSlice()->getPPS()->getWPBiPred() && cu->getSlice()->getSliceType() == B_SLICE)
    {
        TShortYUV* pcMbYuv;
        for (Int refList = 0; refList < 2; refList++)
        {
            RefPicList picList = (refList ? REF_PIC_LIST_1 : REF_PIC_LIST_0);
            iRefIdx[refList] = cu->getCUMvField(picList)->getRefIdx(partAddr);

            if (iRefIdx[refList] < 0)
            {
                continue;
            }

            assert(iRefIdx[refList] < cu->getSlice()->getNumRefIdx(picList));

            pcMbYuv = &m_acShortPred[refList];
            xPredInterUni(cu, partAddr, width, height, picList, pcMbYuv, true);
        }

        xWeightedPredictionBi(cu, &m_acShortPred[0], &m_acShortPred[1], iRefIdx[0], iRefIdx[1], partAddr, width, height, rpcYuvPred);
    }
    else if (cu->getSlice()->getPPS()->getUseWP() && cu->getSlice()->getSliceType() == P_SLICE)
    {
        TShortYUV* pcMbYuv;
        for (Int refList = 0; refList < 2; refList++)
        {
            RefPicList picList = (refList ? REF_PIC_LIST_1 : REF_PIC_LIST_0);
            iRefIdx[refList] = cu->getCUMvField(picList)->getRefIdx(partAddr);

            if (iRefIdx[refList] < 0)
            {
                continue;
            }

            assert(iRefIdx[refList] < cu->getSlice()->getNumRefIdx(picList));

            pcMbYuv = &m_acShortPred[refList];
            xPredInterUni(cu, partAddr, width, height, picList, pcMbYuv, true);
        }

        xWeightedPredictionUni(cu, &m_acShortPred[0], partAddr, width, height, REF_PIC_LIST_0, rpcYuvPred);
    }
    else
    {
        TComYuv* pcMbYuv;
        for (Int refList = 0; refList < 2; refList++)
        {
            RefPicList picList = (refList ? REF_PIC_LIST_1 : REF_PIC_LIST_0);
            iRefIdx[refList] = cu->getCUMvField(picList)->getRefIdx(partAddr);

            if (iRefIdx[refList] < 0)
            {
                continue;
            }

            assert(iRefIdx[refList] < cu->getSlice()->getNumRefIdx(picList));

            pcMbYuv = &m_acYuvPred[refList];
            xPredInterUni(cu, partAddr, width, height, picList, pcMbYuv);
        }

        xWeightedAverage(&m_acYuvPred[0], &m_acYuvPred[1], iRefIdx[0], iRefIdx[1], partAddr, width, height, rpcYuvPred);
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
Void TComPrediction::xPredInterLumaBlk(TComDataCU *cu, TComPicYuv *refPic, UInt partAddr, MV *mv, Int width, Int height, TComYuv *&dstPic, Bool bi)
{
    assert(bi == false);

    Int refStride = refPic->getStride();
    Int refOffset = (mv->x >> 2) + (mv->y >> 2) * refStride;
    Pel *ref      =  refPic->getLumaAddr(cu->getAddr(), cu->getZorderIdxInCU() + partAddr) + refOffset;

    Int dstStride = dstPic->getStride();
    Pel *dst      = dstPic->getLumaAddr(partAddr);

    Int xFrac = mv->x & 0x3;
    Int yFrac = mv->y & 0x3;

    Pel* src = refPic->getLumaFilterBlock(yFrac, xFrac, cu->getAddr(), cu->getZorderIdxInCU() + partAddr) + refOffset;
    Int srcStride = refPic->getStride();

    x265::primitives.blockcpy_pp(width, height, (pixel*)dst, dstStride, (pixel*)src, srcStride);
}

//Motion compensated block for biprediction
Void TComPrediction::xPredInterLumaBlk(TComDataCU *cu, TComPicYuv *refPic, UInt partAddr, MV *mv, Int width, Int height, TShortYUV *&dstPic, Bool bi)
{
    assert(bi == true);

    Int refStride = refPic->getStride();
    Int refOffset = (mv->x >> 2) + (mv->y >> 2) * refStride;
    Pel *ref      =  refPic->getLumaAddr(cu->getAddr(), cu->getZorderIdxInCU() + partAddr) + refOffset;

    Int dstStride = dstPic->width;
    Short *dst      = dstPic->getLumaAddr(partAddr);

    Int xFrac = mv->x & 0x3;
    Int yFrac = mv->y & 0x3;

    Pel* src = refPic->getLumaFilterBlock(yFrac, xFrac, cu->getAddr(), cu->getZorderIdxInCU() + partAddr) + refOffset;
    Int srcStride = refPic->getStride();

    if (yFrac == 0)
    {
        if (xFrac == 0)
        {
            x265::primitives.ipfilter_p2s(g_bitDepthY, (pixel*)ref, refStride, dst, dstStride, width, height);
        }
        else
        {
            x265::primitives.ipfilter_ps[FILTER_H_P_S_8](g_bitDepthY, (pixel*)ref, refStride, dst, dstStride, width, height, g_lumaFilter[xFrac]);
        }
    }
    else if (xFrac == 0)
    {
        x265::primitives.ipfilter_ps[FILTER_V_P_S_8](g_bitDepthY, (pixel*)ref, refStride, dst, dstStride, width, height, g_lumaFilter[yFrac]);
    }
    else
    {
        Int tmpStride = width;
        Int filterSize = NTAPS_LUMA;
        Int halfFilterSize = (filterSize >> 1);
        Short *tmp = (Short*)xMalloc(Short, width * (height + filterSize - 1));

        x265::primitives.ipfilter_ps[FILTER_H_P_S_8](g_bitDepthY, (pixel*)(ref - (halfFilterSize - 1) * refStride), refStride, tmp, tmpStride, width, height + filterSize - 1, g_lumaFilter[xFrac]);
        x265::primitives.ipfilter_ss[FILTER_V_S_S_8](g_bitDepthY, tmp + (halfFilterSize - 1) * tmpStride, tmpStride, dst, dstStride, width, height, g_lumaFilter[yFrac]);

        xFree(tmp);
    }
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
Void TComPrediction::xPredInterChromaBlk(TComDataCU *cu, TComPicYuv *refPic, UInt partAddr, MV *mv, Int width, Int height, TComYuv *&dstPic, Bool bi)
{
    assert(bi == false);

    Int refStride = refPic->getCStride();
    Int dstStride = dstPic->getCStride();

    Int refOffset = (mv->x >> 3) + (mv->y >> 3) * refStride;

    Pel* refCb = refPic->getCbAddr(cu->getAddr(), cu->getZorderIdxInCU() + partAddr) + refOffset;
    Pel* refCr = refPic->getCrAddr(cu->getAddr(), cu->getZorderIdxInCU() + partAddr) + refOffset;

    Pel* dstCb = dstPic->getCbAddr(partAddr);
    Pel* dstCr = dstPic->getCrAddr(partAddr);

    Int xFrac = mv->x & 0x7;
    Int yFrac = mv->y & 0x7;
    UInt cxWidth = width >> 1;
    UInt cxHeight = height >> 1;

    Int filterSize = NTAPS_CHROMA;

    Int halfFilterSize = (filterSize >> 1);

    if (yFrac == 0)
    {
        if (xFrac == 0)
        {
            x265::primitives.blockcpy_pp(cxWidth, cxHeight, (pixel*)dstCb, dstStride, (pixel*)refCb, refStride);
            x265::primitives.blockcpy_pp(cxWidth, cxHeight, (pixel*)dstCr, dstStride, (pixel*)refCr, refStride);
        }
        else
        {
            primitives.ipfilter_pp[FILTER_H_P_P_4](g_bitDepthC, (pixel*)refCb, refStride, (pixel*)dstCb,  dstStride, cxWidth, cxHeight, g_chromaFilter[xFrac]);
            primitives.ipfilter_pp[FILTER_H_P_P_4](g_bitDepthC, (pixel*)refCr, refStride, (pixel*)dstCr,  dstStride, cxWidth, cxHeight, g_chromaFilter[xFrac]);
        }
    }
    else if (xFrac == 0)
    {
        primitives.ipfilter_pp[FILTER_V_P_P_4](g_bitDepthC, (pixel*)refCb, refStride, (pixel*)dstCb, dstStride, cxWidth, cxHeight, g_chromaFilter[yFrac]);
        primitives.ipfilter_pp[FILTER_V_P_P_4](g_bitDepthC, (pixel*)refCr, refStride, (pixel*)dstCr, dstStride, cxWidth, cxHeight, g_chromaFilter[yFrac]);
    }
    else
    {
        Int     extStride = cxWidth;
        Short*  extY      = (Short*)xMalloc(Short, cxWidth * (cxHeight + filterSize - 1));

        primitives.ipfilter_ps[FILTER_H_P_S_4](g_bitDepthC, (pixel*)(refCb - (halfFilterSize - 1) * refStride), refStride, extY, extStride, cxWidth, cxHeight + filterSize - 1, g_chromaFilter[xFrac]);
        primitives.ipfilter_sp[FILTER_V_S_P_4](g_bitDepthC, extY + (halfFilterSize - 1) * extStride, extStride, (pixel*)dstCb, dstStride, cxWidth, cxHeight, g_chromaFilter[yFrac]);

        primitives.ipfilter_ps[FILTER_H_P_S_4](g_bitDepthC, (pixel*)(refCr - (halfFilterSize - 1) * refStride), refStride, extY, extStride, cxWidth, cxHeight + filterSize - 1, g_chromaFilter[xFrac]);
        primitives.ipfilter_sp[FILTER_V_S_P_4](g_bitDepthC, extY + (halfFilterSize - 1) * extStride, extStride, (pixel*)dstCr, dstStride, cxWidth, cxHeight, g_chromaFilter[yFrac]);

        xFree(extY);
    }
}

//Generate motion compensated block when biprediction
Void TComPrediction::xPredInterChromaBlk(TComDataCU *cu, TComPicYuv *refPic, UInt partAddr, MV *mv, Int width, Int height, TShortYUV *&dstPic, Bool bi)
{
    assert(bi == true);

    Int refStride = refPic->getCStride();
    Int dstStride = dstPic->Cwidth;

    Int refOffset = (mv->x >> 3) + (mv->y >> 3) * refStride;

    Pel* refCb = refPic->getCbAddr(cu->getAddr(), cu->getZorderIdxInCU() + partAddr) + refOffset;
    Pel* refCr = refPic->getCrAddr(cu->getAddr(), cu->getZorderIdxInCU() + partAddr) + refOffset;

    Short* dstCb = dstPic->getCbAddr(partAddr);
    Short* dstCr = dstPic->getCrAddr(partAddr);

    Int xFrac = mv->x & 0x7;
    Int yFrac = mv->y & 0x7;
    UInt cxWidth = width >> 1;
    UInt cxHeight = height >> 1;

    Int filterSize = NTAPS_CHROMA;

    Int halfFilterSize = (filterSize >> 1);

    if (yFrac == 0)
    {
        if (xFrac == 0)
        {
            x265::primitives.ipfilter_p2s(g_bitDepthC, (pixel*)refCb, refStride, dstCb, dstStride, cxWidth, cxHeight);
            x265::primitives.ipfilter_p2s(g_bitDepthC, (pixel*)refCr, refStride, dstCr, dstStride, cxWidth, cxHeight);
        }
        else
        {
            x265::primitives.ipfilter_ps[FILTER_H_P_S_4](g_bitDepthC, (pixel*)refCb, refStride, dstCb, dstStride, cxWidth, cxHeight, g_chromaFilter[xFrac]);
            x265::primitives.ipfilter_ps[FILTER_H_P_S_4](g_bitDepthC, (pixel*)refCr, refStride, dstCr, dstStride, cxWidth, cxHeight, g_chromaFilter[xFrac]);
        }
    }
    else if (xFrac == 0)
    {
        x265::primitives.ipfilter_ps[FILTER_V_P_S_4](g_bitDepthC, (pixel*)refCb, refStride, dstCb, dstStride, cxWidth, cxHeight, g_chromaFilter[yFrac]);
        x265::primitives.ipfilter_ps[FILTER_V_P_S_4](g_bitDepthC, (pixel*)refCr, refStride, dstCr, dstStride, cxWidth, cxHeight, g_chromaFilter[yFrac]);
    }
    else
    {
        Int     extStride = cxWidth;
        Short*  extY      = (Short*)xMalloc(Short, cxWidth * (cxHeight + filterSize - 1));
        x265::primitives.ipfilter_ps[FILTER_H_P_S_4](g_bitDepthY, (pixel*)(refCb - (halfFilterSize - 1) * refStride), refStride, extY,  extStride, cxWidth, cxHeight + filterSize - 1, g_chromaFilter[xFrac]);
        x265::primitives.ipfilter_ss[FILTER_V_S_S_4](g_bitDepthY, extY  + (halfFilterSize - 1) * extStride, extStride, dstCb, dstStride, cxWidth, cxHeight, g_chromaFilter[yFrac]);
        x265::primitives.ipfilter_ps[FILTER_H_P_S_4](g_bitDepthY, (pixel*)(refCr - (halfFilterSize - 1) * refStride), refStride, extY,  extStride, cxWidth, cxHeight + filterSize - 1, g_chromaFilter[xFrac]);
        x265::primitives.ipfilter_ss[FILTER_V_S_S_4](g_bitDepthY, extY  + (halfFilterSize - 1) * extStride, extStride, dstCr, dstStride, cxWidth, cxHeight, g_chromaFilter[yFrac]);
        xFree(extY);
    }
}

Void TComPrediction::xWeightedAverage(TComYuv* srcYuv0, TComYuv* srcYuv1, Int iRefIdx0, Int iRefIdx1, UInt partIdx, Int width, Int height, TComYuv*& rpcYuvDst)
{
    if (iRefIdx0 >= 0 && iRefIdx1 >= 0)
    {
        rpcYuvDst->addAvg(srcYuv0, srcYuv1, partIdx, width, height);
    }
    else if (iRefIdx0 >= 0 && iRefIdx1 <  0)
    {
        srcYuv0->copyPartToPartYuv(rpcYuvDst, partIdx, width, height);
    }
    else if (iRefIdx0 <  0 && iRefIdx1 >= 0)
    {
        srcYuv1->copyPartToPartYuv(rpcYuvDst, partIdx, width, height);
    }
}

// AMVP
Void TComPrediction::getMvPredAMVP(TComDataCU* cu, UInt partIdx, UInt partAddr, RefPicList picList, MV& mvPred)
{
    AMVPInfo* pcAMVPInfo = cu->getCUMvField(picList)->getAMVPInfo();

    if (pcAMVPInfo->iN <= 1)
    {
        mvPred = pcAMVPInfo->m_acMvCand[0];

        cu->setMVPIdxSubParts(0, picList, partAddr, partIdx, cu->getDepth(partAddr));
        cu->setMVPNumSubParts(pcAMVPInfo->iN, picList, partAddr, partIdx, cu->getDepth(partAddr));
        return;
    }

    assert(cu->getMVPIdx(picList, partAddr) >= 0);
    mvPred = pcAMVPInfo->m_acMvCand[cu->getMVPIdx(picList, partAddr)];
}

//! \}
