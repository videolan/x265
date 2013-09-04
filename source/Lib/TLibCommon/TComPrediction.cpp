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

static const UChar intraFilterThreshold[5] =
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
    : m_predBuf(NULL)
    , m_predAllAngsBuf(NULL)
    , m_lumaRecBuffer(0)
    , m_lumaRecStride(0)
{}

TComPrediction::~TComPrediction()
{
    delete[] m_predBuf;
    X265_FREE(m_predAllAngsBuf);

    X265_FREE(refAbove);
    X265_FREE(refAboveFlt);
    X265_FREE(refLeft);
    X265_FREE(refLeftFlt);

    m_predYuv[0].destroy();
    m_predYuv[1].destroy();
    m_predShortYuv[0].destroy();
    m_predShortYuv[1].destroy();

    m_predTempYuv.destroy();

    if (m_lumaRecBuffer)
    {
        delete [] m_lumaRecBuffer;
    }

    if (m_immedVals)
        X265_FREE(m_immedVals);

    int i, j;
    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; j++)
        {
            m_filteredBlock[i][j].destroy();
        }

        m_filteredBlockTmp[i].destroy();
    }
}

void TComPrediction::initTempBuff()
{
    if (m_predBuf == NULL)
    {
        int extWidth  = MAX_CU_SIZE + 16;
        int extHeight = MAX_CU_SIZE + 1;
        int i, j;
        for (i = 0; i < 4; i++)
        {
            m_filteredBlockTmp[i].create(extWidth, extHeight + 7);
            for (j = 0; j < 4; j++)
            {
                m_filteredBlock[i][j].create(extWidth, extHeight);
            }
        }

        m_predBufHeight  = ((MAX_CU_SIZE + 2) << 4);
        m_predBufStride = ((MAX_CU_SIZE  + 8) << 4);
        m_predBuf = new Pel[m_predBufStride * m_predBufHeight];
        m_predAllAngsBuf = (Pel*)X265_MALLOC(Pel, 33 * MAX_CU_SIZE * MAX_CU_SIZE);

        refAbove = (Pel*)X265_MALLOC(Pel, 3 * MAX_CU_SIZE);
        refAboveFlt = (Pel*)X265_MALLOC(Pel, 3 * MAX_CU_SIZE);
        refLeft = (Pel*)X265_MALLOC(Pel, 3 * MAX_CU_SIZE);
        refLeftFlt = (Pel*)X265_MALLOC(Pel, 3 * MAX_CU_SIZE);

        // new structure
        m_predYuv[0].create(MAX_CU_SIZE, MAX_CU_SIZE);
        m_predYuv[1].create(MAX_CU_SIZE, MAX_CU_SIZE);
        m_predShortYuv[0].create(MAX_CU_SIZE, MAX_CU_SIZE);
        m_predShortYuv[1].create(MAX_CU_SIZE, MAX_CU_SIZE);

        m_predTempYuv.create(MAX_CU_SIZE, MAX_CU_SIZE);
        m_immedVals = (short*)X265_MALLOC(short, 64 * (64 + NTAPS_LUMA - 1));
    }

    if (m_lumaRecStride != (MAX_CU_SIZE >> 1) + 1)
    {
        m_lumaRecStride =  (MAX_CU_SIZE >> 1) + 1;
        if (!m_lumaRecBuffer)
        {
            m_lumaRecBuffer = new Pel[m_lumaRecStride * m_lumaRecStride];
        }
    }
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

void TComPrediction::predIntraLumaAng(UInt dirMode, Pel* dst, UInt stride, int size)
{
    assert(g_convertToBit[size] >= 0);   //   4x  4
    assert(g_convertToBit[size] <= 5);   // 128x128

    char log2BlkSize = g_convertToBit[size] + 2;

    Pel *src = m_predBuf;
    assert(log2BlkSize >= 2 && log2BlkSize < 7);
    int diff = std::min<int>(abs((int)dirMode - HOR_IDX), abs((int)dirMode - VER_IDX));
    UChar filterIdx = diff > intraFilterThreshold[log2BlkSize - 2] ? 1 : 0;
    if (dirMode == DC_IDX)
    {
        filterIdx = 0; //no smoothing for DC or LM chroma
    }

    assert(filterIdx <= 1);

    Pel *refLft, *refAbv;
    refLft = refLeft + size - 1;
    refAbv = refAbove + size - 1;

    if (filterIdx)
    {
        src += ADI_BUF_STRIDE * (2 * size + 1);
        refLft = refLeftFlt + size - 1;
        refAbv = refAboveFlt + size - 1;
    }

    // get starting pixel in block
    bool bFilter = (size <= 16);

    // Create the prediction
    if (dirMode == PLANAR_IDX)
    {
        primitives.intra_pred_planar((pixel*)refAbv + 1, (pixel*)refLft + 1, (pixel*)dst, stride, size);
    }
    else if (dirMode == DC_IDX)
    {
        primitives.intra_pred_dc((pixel*)refAbv + 1, (pixel*)refLft + 1, (pixel*)dst, stride, size, bFilter);
    }
    else
    {
        primitives.intra_pred_ang(dst, stride, size, dirMode, bFilter, refLft, refAbv);
    }
}

// Angular chroma
void TComPrediction::predIntraChromaAng(Pel* src, UInt dirMode, Pel* dst, UInt stride, int width)
{
    // Create the prediction
    Pel refAbv[3 * MAX_CU_SIZE];
    Pel refLft[3 * MAX_CU_SIZE];
    int limit = (dirMode <= 25 && dirMode >= 11) ? (width + 1 + 1) : (2 * width + 1);

    memcpy(refAbv + width - 1, src, (limit) * sizeof(Pel));
    for (int k = 0; k < limit; k++)
    {
        refLft[k + width - 1] = src[k * ADI_BUF_STRIDE];
    }

    // get starting pixel in block
    if (dirMode == PLANAR_IDX)
    {
        primitives.intra_pred_planar((pixel*)refAbv + width - 1 + 1, (pixel*)refLft + width - 1 + 1, (pixel*)dst, stride, width);
    }
    else if (dirMode == DC_IDX)
    {
        primitives.intra_pred_dc(refAbv + width - 1 + 1, refLft + width - 1 + 1, dst, stride, width, false);
    }
    else
    {
        primitives.intra_pred_ang(dst, stride, width, dirMode, false, refLft + width - 1, refAbv + width - 1);
    }
}

/** Function for checking identical motion.
 * \param TComDataCU* cu
 * \param UInt PartAddr
 */
bool TComPrediction::xCheckIdenticalMotion(TComDataCU* cu, UInt partAddr)
{
    if (cu->getSlice()->isInterB() && !cu->getSlice()->getPPS()->getWPBiPred())
    {
        if (cu->getCUMvField(REF_PIC_LIST_0)->getRefIdx(partAddr) >= 0 && cu->getCUMvField(REF_PIC_LIST_1)->getRefIdx(partAddr) >= 0)
        {
            int refPOCL0 = cu->getSlice()->getRefPic(REF_PIC_LIST_0, cu->getCUMvField(REF_PIC_LIST_0)->getRefIdx(partAddr))->getPOC();
            int refPOCL1 = cu->getSlice()->getRefPic(REF_PIC_LIST_1, cu->getCUMvField(REF_PIC_LIST_1)->getRefIdx(partAddr))->getPOC();
            if (refPOCL0 == refPOCL1 && cu->getCUMvField(REF_PIC_LIST_0)->getMv(partAddr) == cu->getCUMvField(REF_PIC_LIST_1)->getMv(partAddr))
            {
                return true;
            }
        }
    }
    return false;
}

void TComPrediction::motionCompensation(TComDataCU* cu, TComYuv* predYuv, RefPicList picList, int partIdx)
{
    int  width;
    int  height;
    UInt partAddr;

    if (partIdx >= 0)
    {
        cu->getPartIndexAndSize(partIdx, partAddr, width, height);
        if (picList != REF_PIC_LIST_X)
        {
            if (cu->getSlice()->getPPS()->getUseWP())
            {
                TShortYUV* pcMbYuv = &m_predShortYuv[0];
                int refId = cu->getCUMvField(picList)->getRefIdx(partAddr);
                assert(refId >= 0);

                MV mv = cu->getCUMvField(picList)->getMv(partAddr);
                cu->clipMv(mv);

                xPredInterLumaBlk(cu, cu->getSlice()->m_mref[picList][refId], partAddr, &mv, width, height, predYuv);
                xPredInterChromaBlk(cu, cu->getSlice()->getRefPic(picList, refId)->getPicYuvRec(), partAddr, &mv, width, height, pcMbYuv);

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
                TShortYUV* pcMbYuv = &m_predShortYuv[0];

                int refId = cu->getCUMvField(picList)->getRefIdx(partAddr);
                assert(refId >= 0);

                MV mv = cu->getCUMvField(picList)->getMv(partAddr);
                cu->clipMv(mv);

                xPredInterLumaBlk(cu, cu->getSlice()->m_mref[picList][refId], partAddr, &mv, width, height, predYuv);
                xPredInterChromaBlk(cu, cu->getSlice()->getRefPic(picList, refId)->getPicYuvRec(), partAddr, &mv, width, height, pcMbYuv);

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

void TComPrediction::xPredInterUni(TComDataCU* cu, UInt partAddr, int width, int height, RefPicList picList, TComYuv* outPredYuv)
{
    int refIdx = cu->getCUMvField(picList)->getRefIdx(partAddr);

    assert(refIdx >= 0);

    MV mv = cu->getCUMvField(picList)->getMv(partAddr);
    cu->clipMv(mv);

    xPredInterLumaBlk(cu, cu->getSlice()->m_mref[picList][refIdx], partAddr, &mv, width, height, outPredYuv);

    xPredInterChromaBlk(cu, cu->getSlice()->getRefPic(picList, refIdx)->getPicYuvRec(), partAddr, &mv, width, height, outPredYuv);
}

void TComPrediction::xPredInterUni(TComDataCU* cu, UInt partAddr, int width, int height, RefPicList picList, TShortYUV* outPredYuv)
{
    int refIdx = cu->getCUMvField(picList)->getRefIdx(partAddr);

    assert(refIdx >= 0);

    MV mv = cu->getCUMvField(picList)->getMv(partAddr);
    cu->clipMv(mv);

    xPredInterLumaBlk(cu, cu->getSlice()->getRefPic(picList, refIdx)->getPicYuvRec(), partAddr, &mv, width, height, outPredYuv);
    xPredInterChromaBlk(cu, cu->getSlice()->getRefPic(picList, refIdx)->getPicYuvRec(), partAddr, &mv, width, height, outPredYuv);
}

void TComPrediction::xPredInterBi(TComDataCU* cu, UInt partAddr, int width, int height, TComYuv*& outPredYuv)
{
    int refIdx[2] = { -1, -1 };

    if (cu->getCUMvField(REF_PIC_LIST_0)->getRefIdx(partAddr) >= 0 && cu->getCUMvField(REF_PIC_LIST_1)->getRefIdx(partAddr) >= 0)
    {
        TShortYUV* pcMbYuv;
        for (int refList = 0; refList < 2; refList++)
        {
            RefPicList picList = (refList ? REF_PIC_LIST_1 : REF_PIC_LIST_0);
            refIdx[refList] = cu->getCUMvField(picList)->getRefIdx(partAddr);

            assert(refIdx[refList] < cu->getSlice()->getNumRefIdx(picList));

            pcMbYuv = &m_predShortYuv[refList];
            xPredInterUni(cu, partAddr, width, height, picList, pcMbYuv);
        }

        if (cu->getSlice()->getPPS()->getWPBiPred() && cu->getSlice()->getSliceType() == B_SLICE)
        {
            xWeightedPredictionBi(cu, &m_predShortYuv[0], &m_predShortYuv[1], refIdx[0], refIdx[1], partAddr, width, height, outPredYuv);
        }
        else
        {
            outPredYuv->addAvg(&m_predShortYuv[0], &m_predShortYuv[1], partAddr, width, height);
        }
    }
    else if (cu->getSlice()->getPPS()->getWPBiPred() && cu->getSlice()->getSliceType() == B_SLICE)
    {
        TShortYUV* pcMbYuv;
        for (int refList = 0; refList < 2; refList++)
        {
            RefPicList picList = (refList ? REF_PIC_LIST_1 : REF_PIC_LIST_0);
            refIdx[refList] = cu->getCUMvField(picList)->getRefIdx(partAddr);

            if (refIdx[refList] < 0)
            {
                continue;
            }

            assert(refIdx[refList] < cu->getSlice()->getNumRefIdx(picList));

            pcMbYuv = &m_predShortYuv[refList];
            xPredInterUni(cu, partAddr, width, height, picList, pcMbYuv);
        }

        xWeightedPredictionBi(cu, &m_predShortYuv[0], &m_predShortYuv[1], refIdx[0], refIdx[1], partAddr, width, height, outPredYuv);
    }
    else if (cu->getSlice()->getPPS()->getUseWP() && cu->getSlice()->getSliceType() == P_SLICE)
    {
        TShortYUV* pcMbYuv;

        RefPicList picList = REF_PIC_LIST_0;
        refIdx[0] = cu->getCUMvField(picList)->getRefIdx(partAddr);

        if (!(refIdx[0] < 0))
        {
            assert(refIdx[0] < cu->getSlice()->getNumRefIdx(picList));

            pcMbYuv = &m_predShortYuv[0];
            int refId = cu->getCUMvField(picList)->getRefIdx(partAddr);
            assert(refId >= 0);

            MV mv = cu->getCUMvField(picList)->getMv(partAddr);
            cu->clipMv(mv);

            xPredInterLumaBlk(cu, cu->getSlice()->m_mref[picList][refId], partAddr, &mv, width, height, outPredYuv);
            xPredInterChromaBlk(cu, cu->getSlice()->getRefPic(picList, refId)->getPicYuvRec(), partAddr, &mv, width, height, pcMbYuv);

            xWeightedPredictionUni(cu, &m_predShortYuv[0], partAddr, width, height, REF_PIC_LIST_0, outPredYuv);
        }
    }
    else
    {
        for (int refList = 0; refList < 2; refList++)
        {
            RefPicList picList = (refList ? REF_PIC_LIST_1 : REF_PIC_LIST_0);
            refIdx[refList] = cu->getCUMvField(picList)->getRefIdx(partAddr);

            if (refIdx[refList] < 0)
            {
                continue;
            }

            assert(refIdx[refList] < cu->getSlice()->getNumRefIdx(picList));

            TComYuv* yuv = &m_predYuv[refList];
            xPredInterUni(cu, partAddr, width, height, picList, yuv);
        }

        xWeightedAverage(&m_predYuv[0], &m_predYuv[1], refIdx[0], refIdx[1], partAddr, width, height, outPredYuv);
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
 */
void TComPrediction::xPredInterLumaBlk(TComDataCU *cu, MotionReference *ref, UInt partAddr, MV *mv, int width, int height, TComYuv *dstPic)
{
    int dstStride = dstPic->getStride();
    Pel *dst      = dstPic->getLumaAddr(partAddr);

    TComPicYuv* pic = ref->m_reconPic;
    int srcStride = ref->lumaStride;
    int refOffset = (mv->x >> 2) + (mv->y >> 2) * srcStride;
    Pel* src = pic->getLumaAddr(cu->getAddr(), cu->getZorderIdxInCU() + partAddr) + refOffset;

    int xFrac = mv->x & 0x3;
    int yFrac = mv->y & 0x3;
    if ((yFrac | xFrac) == 0)
    {
        primitives.blockcpy_pp(width, height, dst, dstStride, src, srcStride);
    }
    else if (yFrac == 0)
    {
        primitives.ipfilter_pp[FILTER_H_P_P_8](src, srcStride, dst, dstStride, width, height, g_lumaFilter[xFrac]);
    }
    else if (xFrac == 0)
    {
        primitives.ipfilter_pp[FILTER_V_P_P_8](src, srcStride, dst, dstStride, width, height, g_lumaFilter[yFrac]);
    }
    else
    {
        int tmpStride = width;
        int filterSize = NTAPS_LUMA;
        int halfFilterSize = (filterSize >> 1);
        primitives.ipfilter_ps[FILTER_H_P_S_8](src - (halfFilterSize - 1) * srcStride,  srcStride, m_immedVals, tmpStride, width, height + filterSize - 1, g_lumaFilter[xFrac]);
        primitives.ipfilter_sp[FILTER_V_S_P_8](m_immedVals + (halfFilterSize - 1) * tmpStride, tmpStride, dst, dstStride, width, height, g_lumaFilter[yFrac]);
    }
}

//Motion compensated block for biprediction
void TComPrediction::xPredInterLumaBlk(TComDataCU *cu, TComPicYuv *refPic, UInt partAddr, MV *mv, int width, int height, TShortYUV *dstPic)
{
    int refStride = refPic->getStride();
    int refOffset = (mv->x >> 2) + (mv->y >> 2) * refStride;
    Pel *ref      =  refPic->getLumaAddr(cu->getAddr(), cu->getZorderIdxInCU() + partAddr) + refOffset;

    int dstStride = dstPic->m_width;
    short *dst    = dstPic->getLumaAddr(partAddr);

    int xFrac = mv->x & 0x3;
    int yFrac = mv->y & 0x3;

    if ((yFrac | xFrac) == 0)
    {
        primitives.ipfilter_p2s(ref, refStride, dst, dstStride, width, height);
    }
    else if (yFrac == 0)
    {
        primitives.ipfilter_ps[FILTER_H_P_S_8](ref, refStride, dst, dstStride, width, height, g_lumaFilter[xFrac]);
    }
    else if (xFrac == 0)
    {
        primitives.ipfilter_ps[FILTER_V_P_S_8](ref, refStride, dst, dstStride, width, height, g_lumaFilter[yFrac]);
    }
    else
    {
        int tmpStride = width;
        int filterSize = NTAPS_LUMA;
        int halfFilterSize = (filterSize >> 1);
        primitives.ipfilter_ps[FILTER_H_P_S_8](ref - (halfFilterSize - 1) * refStride, refStride, m_immedVals, tmpStride, width, height + filterSize - 1, g_lumaFilter[xFrac]);
        primitives.ipfilter_ss[FILTER_V_S_S_8](m_immedVals + (halfFilterSize - 1) * tmpStride, tmpStride, dst, dstStride, width, height, g_lumaFilter[yFrac]);
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
void TComPrediction::xPredInterChromaBlk(TComDataCU *cu, TComPicYuv *refPic, UInt partAddr, MV *mv, int width, int height, TComYuv *dstPic)
{
    int refStride = refPic->getCStride();
    int dstStride = dstPic->getCStride();

    int refOffset = (mv->x >> 3) + (mv->y >> 3) * refStride;

    Pel* refCb = refPic->getCbAddr(cu->getAddr(), cu->getZorderIdxInCU() + partAddr) + refOffset;
    Pel* refCr = refPic->getCrAddr(cu->getAddr(), cu->getZorderIdxInCU() + partAddr) + refOffset;

    Pel* dstCb = dstPic->getCbAddr(partAddr);
    Pel* dstCr = dstPic->getCrAddr(partAddr);

    int xFrac = mv->x & 0x7;
    int yFrac = mv->y & 0x7;
    UInt cxWidth = width >> 1;
    UInt cxHeight = height >> 1;

    if ((yFrac | xFrac) == 0)
    {
        primitives.blockcpy_pp(cxWidth, cxHeight, dstCb, dstStride, refCb, refStride);
        primitives.blockcpy_pp(cxWidth, cxHeight, dstCr, dstStride, refCr, refStride);
    }
    else if (yFrac == 0)
    {
        primitives.ipfilter_pp[FILTER_H_P_P_4](refCb, refStride, dstCb, dstStride, cxWidth, cxHeight, g_chromaFilter[xFrac]);
        primitives.ipfilter_pp[FILTER_H_P_P_4](refCr, refStride, dstCr, dstStride, cxWidth, cxHeight, g_chromaFilter[xFrac]);
    }
    else if (xFrac == 0)
    {
        primitives.ipfilter_pp[FILTER_V_P_P_4](refCb, refStride, dstCb, dstStride, cxWidth, cxHeight, g_chromaFilter[yFrac]);
        primitives.ipfilter_pp[FILTER_V_P_P_4](refCr, refStride, dstCr, dstStride, cxWidth, cxHeight, g_chromaFilter[yFrac]);
    }
    else
    {
        int extStride = cxWidth;
        int filterSize = NTAPS_CHROMA;
        int halfFilterSize = (filterSize >> 1);

        primitives.ipfilter_ps[FILTER_H_P_S_4](refCb - (halfFilterSize - 1) * refStride, refStride, m_immedVals, extStride, cxWidth, cxHeight + filterSize - 1, g_chromaFilter[xFrac]);
        primitives.ipfilter_sp[FILTER_V_S_P_4](m_immedVals + (halfFilterSize - 1) * extStride, extStride, dstCb, dstStride, cxWidth, cxHeight, g_chromaFilter[yFrac]);

        primitives.ipfilter_ps[FILTER_H_P_S_4](refCr - (halfFilterSize - 1) * refStride, refStride, m_immedVals, extStride, cxWidth, cxHeight + filterSize - 1, g_chromaFilter[xFrac]);
        primitives.ipfilter_sp[FILTER_V_S_P_4](m_immedVals + (halfFilterSize - 1) * extStride, extStride, dstCr, dstStride, cxWidth, cxHeight, g_chromaFilter[yFrac]);
    }
}

// Generate motion compensated block when biprediction
void TComPrediction::xPredInterChromaBlk(TComDataCU *cu, TComPicYuv *refPic, UInt partAddr, MV *mv, int width, int height, TShortYUV *dstPic)
{
    int refStride = refPic->getCStride();
    int dstStride = dstPic->m_cwidth;

    int refOffset = (mv->x >> 3) + (mv->y >> 3) * refStride;

    Pel* refCb = refPic->getCbAddr(cu->getAddr(), cu->getZorderIdxInCU() + partAddr) + refOffset;
    Pel* refCr = refPic->getCrAddr(cu->getAddr(), cu->getZorderIdxInCU() + partAddr) + refOffset;

    short* dstCb = dstPic->getCbAddr(partAddr);
    short* dstCr = dstPic->getCrAddr(partAddr);

    int xFrac = mv->x & 0x7;
    int yFrac = mv->y & 0x7;
    UInt cxWidth = width >> 1;
    UInt cxHeight = height >> 1;

    if ((yFrac | xFrac) == 0)
    {
        primitives.ipfilter_p2s(refCb, refStride, dstCb, dstStride, cxWidth, cxHeight);
        primitives.ipfilter_p2s(refCr, refStride, dstCr, dstStride, cxWidth, cxHeight);
    }
    else if (yFrac == 0)
    {
        primitives.ipfilter_ps[FILTER_H_P_S_4](refCb, refStride, dstCb, dstStride, cxWidth, cxHeight, g_chromaFilter[xFrac]);
        primitives.ipfilter_ps[FILTER_H_P_S_4](refCr, refStride, dstCr, dstStride, cxWidth, cxHeight, g_chromaFilter[xFrac]);
    }
    else if (xFrac == 0)
    {
        primitives.ipfilter_ps[FILTER_V_P_S_4](refCb, refStride, dstCb, dstStride, cxWidth, cxHeight, g_chromaFilter[yFrac]);
        primitives.ipfilter_ps[FILTER_V_P_S_4](refCr, refStride, dstCr, dstStride, cxWidth, cxHeight, g_chromaFilter[yFrac]);
    }
    else
    {
        int extStride = cxWidth;
        int filterSize = NTAPS_CHROMA;
        int halfFilterSize = (filterSize >> 1);
        primitives.ipfilter_ps[FILTER_H_P_S_4](refCb - (halfFilterSize - 1) * refStride, refStride, m_immedVals, extStride, cxWidth, cxHeight + filterSize - 1, g_chromaFilter[xFrac]);
        primitives.ipfilter_ss[FILTER_V_S_S_4](m_immedVals + (halfFilterSize - 1) * extStride, extStride, dstCb, dstStride, cxWidth, cxHeight, g_chromaFilter[yFrac]);
        primitives.ipfilter_ps[FILTER_H_P_S_4](refCr - (halfFilterSize - 1) * refStride, refStride, m_immedVals, extStride, cxWidth, cxHeight + filterSize - 1, g_chromaFilter[xFrac]);
        primitives.ipfilter_ss[FILTER_V_S_S_4](m_immedVals + (halfFilterSize - 1) * extStride, extStride, dstCr, dstStride, cxWidth, cxHeight, g_chromaFilter[yFrac]);
    }
}

void TComPrediction::xWeightedAverage(TComYuv* srcYuv0, TComYuv* srcYuv1, int refIdx0, int refIdx1, UInt partIdx, int width, int height, TComYuv*& outDstYuv)
{
    if (refIdx0 >= 0 && refIdx1 >= 0)
    {
        outDstYuv->addAvg(srcYuv0, srcYuv1, partIdx, width, height);
    }
    else if (refIdx0 >= 0 && refIdx1 <  0)
    {
        srcYuv0->copyPartToPartYuv(outDstYuv, partIdx, width, height);
    }
    else if (refIdx0 <  0 && refIdx1 >= 0)
    {
        srcYuv1->copyPartToPartYuv(outDstYuv, partIdx, width, height);
    }
}

// AMVP
void TComPrediction::getMvPredAMVP(TComDataCU* cu, UInt partIdx, UInt partAddr, RefPicList picList, MV& mvPred)
{
    AMVPInfo* pcAMVPInfo = cu->getCUMvField(picList)->getAMVPInfo();

    if (pcAMVPInfo->m_num <= 1)
    {
        mvPred = pcAMVPInfo->m_mvCand[0];

        cu->setMVPIdxSubParts(0, picList, partAddr, partIdx, cu->getDepth(partAddr));
        cu->setMVPNumSubParts(pcAMVPInfo->m_num, picList, partAddr, partIdx, cu->getDepth(partAddr));
        return;
    }

    assert(cu->getMVPIdx(picList, partAddr) >= 0);
    mvPred = pcAMVPInfo->m_mvCand[cu->getMVPIdx(picList, partAddr)];
}

//! \}
