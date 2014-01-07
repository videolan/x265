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

    delete [] m_lumaRecBuffer;

    if (m_immedVals)
        X265_FREE(m_immedVals);
}

void TComPrediction::initTempBuff(int csp)
{
    if (m_predBuf == NULL)
    {
        m_predBufHeight  = ((MAX_CU_SIZE + 2) << 4);
        m_predBufStride = ((MAX_CU_SIZE  + 8) << 4);
        m_predBuf = new Pel[m_predBufStride * m_predBufHeight];
        m_predAllAngsBuf = (Pel*)X265_MALLOC(Pel, 33 * MAX_CU_SIZE * MAX_CU_SIZE);

        refAbove = (Pel*)X265_MALLOC(Pel, 3 * MAX_CU_SIZE);
        refAboveFlt = (Pel*)X265_MALLOC(Pel, 3 * MAX_CU_SIZE);
        refLeft = (Pel*)X265_MALLOC(Pel, 3 * MAX_CU_SIZE);
        refLeftFlt = (Pel*)X265_MALLOC(Pel, 3 * MAX_CU_SIZE);

        m_predYuv[0].create(MAX_CU_SIZE, MAX_CU_SIZE, csp);
        m_predYuv[1].create(MAX_CU_SIZE, MAX_CU_SIZE, csp);
        m_predShortYuv[0].create(MAX_CU_SIZE, MAX_CU_SIZE, csp);
        m_predShortYuv[1].create(MAX_CU_SIZE, MAX_CU_SIZE, csp);
        m_predTempYuv.create(MAX_CU_SIZE, MAX_CU_SIZE, csp);

        m_immedVals = (int16_t*)X265_MALLOC(int16_t, 64 * (64 + NTAPS_LUMA - 1));
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

void TComPrediction::predIntraLumaAng(uint32_t dirMode, Pel* dst, intptr_t stride, int size)
{
    assert(g_convertToBit[size] >= 0);   //   4x  4
    assert(g_convertToBit[size] <= 5);   // 128x128

    int log2BlkSize = g_convertToBit[size] + 2;

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

    bool bFilter = size <= 16 && dirMode != PLANAR_IDX;
    primitives.intra_pred[log2BlkSize - 2][dirMode](dst, stride, refLft, refAbv, dirMode, bFilter);
}

// Angular chroma
void TComPrediction::predIntraChromaAng(Pel* src, uint32_t dirMode, Pel* dst, intptr_t stride, int width)
{
    int log2BlkSize = g_convertToBit[width];

    // Create the prediction
    Pel refAbv[3 * MAX_CU_SIZE];
    Pel refLft[3 * MAX_CU_SIZE];
    int limit = (dirMode <= 25 && dirMode >= 11) ? (width + 1 + 1) : (2 * width + 1);

    memcpy(refAbv + width - 1, src, (limit) * sizeof(Pel));
    for (int k = 0; k < limit; k++)
    {
        refLft[k + width - 1] = src[k * ADI_BUF_STRIDE];
    }

    primitives.intra_pred[log2BlkSize][dirMode](dst, stride, refLft + width - 1, refAbv + width - 1, dirMode, 0);
}

/** Function for checking identical motion.
 * \param TComDataCU* cu
 * \param uint32_t PartAddr
 */
bool TComPrediction::xCheckIdenticalMotion(TComDataCU* cu, uint32_t partAddr)
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

void TComPrediction::motionCompensation(TComDataCU* cu, TComYuv* predYuv, int list, int partIdx, bool bLuma, bool bChroma)
{
    int  width;
    int  height;
    uint32_t partAddr;

    if (partIdx >= 0)
    {
        cu->getPartIndexAndSize(partIdx, partAddr, width, height);
        if (list != REF_PIC_LIST_X)
        {
            if (cu->getSlice()->getPPS()->getUseWP())
            {
                TShortYUV* shortYuv = &m_predShortYuv[0];
                int refId = cu->getCUMvField(list)->getRefIdx(partAddr);
                assert(refId >= 0);

                MV mv = cu->getCUMvField(list)->getMv(partAddr);
                cu->clipMv(mv);
                if (bLuma)
                    xPredInterLumaBlk(cu, cu->getSlice()->getRefPic(list, refId)->getPicYuvRec(), partAddr, &mv, width, height, shortYuv);
                if (bChroma)
                    xPredInterChromaBlk(cu, cu->getSlice()->getRefPic(list, refId)->getPicYuvRec(), partAddr, &mv, width, height, shortYuv);

                xWeightedPredictionUni(cu, shortYuv, partAddr, width, height, list, predYuv, -1, bLuma, bChroma);
            }
            else
            {
                xPredInterUni(cu, partAddr, width, height, list, predYuv, bLuma, bChroma);
            }
        }
        else
        {
            if (xCheckIdenticalMotion(cu, partAddr))
            {
                xPredInterUni(cu, partAddr, width, height, REF_PIC_LIST_0, predYuv, bLuma, bChroma);
            }
            else
            {
                xPredInterBi(cu, partAddr, width, height, predYuv, bLuma, bChroma);
            }
        }
        return;
    }

    for (partIdx = 0; partIdx < cu->getNumPartInter(); partIdx++)
    {
        cu->getPartIndexAndSize(partIdx, partAddr, width, height);

        if (list != REF_PIC_LIST_X)
        {
            if (cu->getSlice()->getPPS()->getUseWP())
            {
                TShortYUV* shortYuv = &m_predShortYuv[0];

                int refId = cu->getCUMvField(list)->getRefIdx(partAddr);
                assert(refId >= 0);

                MV mv = cu->getCUMvField(list)->getMv(partAddr);
                cu->clipMv(mv);

                if (bLuma)
                    xPredInterLumaBlk(cu, cu->getSlice()->getRefPic(list, refId)->getPicYuvRec(), partAddr, &mv, width, height, shortYuv);
                if (bChroma)
                    xPredInterChromaBlk(cu, cu->getSlice()->getRefPic(list, refId)->getPicYuvRec(), partAddr, &mv, width, height, shortYuv);

                xWeightedPredictionUni(cu, shortYuv, partAddr, width, height, list, predYuv, -1, bLuma, bChroma);
            }
            else
            {
                xPredInterUni(cu, partAddr, width, height, list, predYuv, bLuma, bChroma);
            }
        }
        else
        {
            if (xCheckIdenticalMotion(cu, partAddr))
            {
                xPredInterUni(cu, partAddr, width, height, REF_PIC_LIST_0, predYuv, bLuma, bChroma);
            }
            else
            {
                xPredInterBi(cu, partAddr, width, height, predYuv, bLuma, bChroma);
            }
        }
    }
}

void TComPrediction::xPredInterUni(TComDataCU* cu, uint32_t partAddr, int width, int height, int list, TComYuv* outPredYuv, bool bLuma, bool bChroma)
{
    int refIdx = cu->getCUMvField(list)->getRefIdx(partAddr);

    assert(refIdx >= 0);

    MV mv = cu->getCUMvField(list)->getMv(partAddr);
    cu->clipMv(mv);

    if (bLuma)
        xPredInterLumaBlk(cu, cu->getSlice()->getRefPic(list, refIdx)->getPicYuvRec(), partAddr, &mv, width, height, outPredYuv);

    if (bChroma)
        xPredInterChromaBlk(cu, cu->getSlice()->getRefPic(list, refIdx)->getPicYuvRec(), partAddr, &mv, width, height, outPredYuv);
}

void TComPrediction::xPredInterUni(TComDataCU* cu, uint32_t partAddr, int width, int height, int list, TShortYUV* outPredYuv, bool bLuma, bool bChroma)
{
    int refIdx = cu->getCUMvField(list)->getRefIdx(partAddr);

    assert(refIdx >= 0);

    MV mv = cu->getCUMvField(list)->getMv(partAddr);
    cu->clipMv(mv);

    if (bLuma)
        xPredInterLumaBlk(cu, cu->getSlice()->getRefPic(list, refIdx)->getPicYuvRec(), partAddr, &mv, width, height, outPredYuv);
    if (bChroma)
        xPredInterChromaBlk(cu, cu->getSlice()->getRefPic(list, refIdx)->getPicYuvRec(), partAddr, &mv, width, height, outPredYuv);
}

void TComPrediction::xPredInterBi(TComDataCU* cu, uint32_t partAddr, int width, int height, TComYuv*& outPredYuv, bool bLuma, bool bChroma)
{
    int refIdx[2] = { -1, -1 };

    if (cu->getCUMvField(REF_PIC_LIST_0)->getRefIdx(partAddr) >= 0 && cu->getCUMvField(REF_PIC_LIST_1)->getRefIdx(partAddr) >= 0)
    {
        for (int list = 0; list < 2; list++)
        {
            refIdx[list] = cu->getCUMvField(list)->getRefIdx(partAddr);

            assert(refIdx[list] < cu->getSlice()->getNumRefIdx(list));

            xPredInterUni(cu, partAddr, width, height, list, &m_predShortYuv[list], bLuma, bChroma);
        }

        if (cu->getSlice()->getPPS()->getWPBiPred() && cu->getSlice()->getSliceType() == B_SLICE)
        {
            xWeightedPredictionBi(cu, &m_predShortYuv[0], &m_predShortYuv[1], refIdx[0], refIdx[1], partAddr, width, height, outPredYuv, bLuma, bChroma);
        }
        else
        {
            outPredYuv->addAvg(&m_predShortYuv[0], &m_predShortYuv[1], partAddr, width, height, bLuma, bChroma);
        }
    }
    else if (cu->getSlice()->getPPS()->getWPBiPred() && cu->getSlice()->getSliceType() == B_SLICE)
    {
        for (int list = 0; list < 2; list++)
        {
            refIdx[list] = cu->getCUMvField(list)->getRefIdx(partAddr);
            if (refIdx[list] < 0) continue;

            assert(refIdx[list] < cu->getSlice()->getNumRefIdx(list));

            xPredInterUni(cu, partAddr, width, height, list, &m_predShortYuv[list], bLuma, bChroma);
        }

        xWeightedPredictionBi(cu, &m_predShortYuv[0], &m_predShortYuv[1], refIdx[0], refIdx[1], partAddr, width, height, outPredYuv, bLuma, bChroma);
    }
    else if (cu->getSlice()->getPPS()->getUseWP() && cu->getSlice()->getSliceType() == P_SLICE)
    {
        int list = REF_PIC_LIST_0;
        refIdx[list] = cu->getCUMvField(list)->getRefIdx(partAddr);

        if (!(refIdx[0] < 0))
        {
            assert(refIdx[0] < cu->getSlice()->getNumRefIdx(list));
            int refId = cu->getCUMvField(list)->getRefIdx(partAddr);
            assert(refId >= 0);

            MV mv = cu->getCUMvField(list)->getMv(partAddr);
            cu->clipMv(mv);
            if (bLuma)
                xPredInterLumaBlk(cu, cu->getSlice()->getRefPic(list, refId)->getPicYuvRec(), partAddr, &mv, width, height, &m_predShortYuv[0]);
            if (bChroma)
                xPredInterChromaBlk(cu, cu->getSlice()->getRefPic(list, refId)->getPicYuvRec(), partAddr, &mv, width, height, &m_predShortYuv[0]);

            xWeightedPredictionUni(cu, &m_predShortYuv[0], partAddr, width, height, REF_PIC_LIST_0, outPredYuv, -1, bLuma, bChroma);
        }
    }
    else
    {
        for (int list = 0; list < 2; list++)
        {
            refIdx[list] = cu->getCUMvField(list)->getRefIdx(partAddr);
            if (refIdx[list] < 0) continue;

            assert(refIdx[list] < cu->getSlice()->getNumRefIdx(list));

            xPredInterUni(cu, partAddr, width, height, list, &m_predYuv[list], bLuma, bChroma);
        }

        xWeightedAverage(&m_predYuv[0], &m_predYuv[1], refIdx[0], refIdx[1], partAddr, width, height, outPredYuv, bLuma, bChroma);
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
void TComPrediction::xPredInterLumaBlk(TComDataCU *cu, TComPicYuv *refPic, uint32_t partAddr, MV *mv, int width, int height, TComYuv *dstPic)
{
    int dstStride = dstPic->getStride();
    Pel *dst      = dstPic->getLumaAddr(partAddr);

    int srcStride = refPic->getStride();
    int srcOffset = (mv->x >> 2) + (mv->y >> 2) * srcStride;
    int partEnum = partitionFromSizes(width, height);
    Pel* src = refPic->getLumaAddr(cu->getAddr(), cu->getZorderIdxInCU() + partAddr) + srcOffset;

    int xFrac = mv->x & 0x3;
    int yFrac = mv->y & 0x3;

    if ((yFrac | xFrac) == 0)
    {
        primitives.luma_copy_pp[partEnum](dst, dstStride, src, srcStride);
    }
    else if (yFrac == 0)
    {
        primitives.luma_hpp[partEnum](src, srcStride, dst, dstStride, xFrac);
    }
    else if (xFrac == 0)
    {
        primitives.luma_vpp[partEnum](src, srcStride, dst, dstStride, yFrac);
    }
    else
    {
        int tmpStride = width;
        int filterSize = NTAPS_LUMA;
        int halfFilterSize = (filterSize >> 1);
        primitives.luma_hps[partEnum](src, srcStride, m_immedVals, tmpStride, xFrac, 1);
        primitives.luma_vsp[partEnum](m_immedVals + (halfFilterSize - 1) * tmpStride, tmpStride, dst, dstStride, yFrac);
    }
}

//Motion compensated block for biprediction
void TComPrediction::xPredInterLumaBlk(TComDataCU *cu, TComPicYuv *refPic, uint32_t partAddr, MV *mv, int width, int height, TShortYUV *dstPic)
{
    int refStride = refPic->getStride();
    int refOffset = (mv->x >> 2) + (mv->y >> 2) * refStride;
    pixel *ref    = refPic->getLumaAddr(cu->getAddr(), cu->getZorderIdxInCU() + partAddr) + refOffset;

    int dstStride = dstPic->m_width;
    int16_t *dst    = dstPic->getLumaAddr(partAddr);

    int xFrac = mv->x & 0x3;
    int yFrac = mv->y & 0x3;

    int partEnum = partitionFromSizes(width, height);

    assert((width % 4) + (height % 4) == 0);
    assert(dstStride == MAX_CU_SIZE);

    if ((yFrac | xFrac) == 0)
    {
        primitives.luma_p2s(ref, refStride, dst, width, height);
    }
    else if (yFrac == 0)
    {
        primitives.luma_hps[partEnum](ref, refStride, dst, dstStride, xFrac, 0);
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
        primitives.luma_hps[partEnum](ref, refStride, m_immedVals, tmpStride, xFrac, 1);
        primitives.ipfilter_ss[FILTER_V_S_S_8](m_immedVals + (halfFilterSize - 1) * tmpStride, tmpStride, dst, dstStride, width, height, yFrac);
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
 */
void TComPrediction::xPredInterChromaBlk(TComDataCU *cu, TComPicYuv *refPic, uint32_t partAddr, MV *mv, int width, int height, TComYuv *dstPic)
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
    int partEnum = partitionFromSizes(width, height);
    int csp = X265_CSP_I420; // TODO: member var?

    if ((yFrac | xFrac) == 0)
    {
        primitives.chroma[csp].copy_pp[partEnum](dstCb, dstStride, refCb, refStride);
        primitives.chroma[csp].copy_pp[partEnum](dstCr, dstStride, refCr, refStride);
    }
    else if (yFrac == 0)
    {
        primitives.chroma[csp].filter_hpp[partEnum](refCb, refStride, dstCb, dstStride, xFrac);
        primitives.chroma[csp].filter_hpp[partEnum](refCr, refStride, dstCr, dstStride, xFrac);
    }
    else if (xFrac == 0)
    {
        primitives.chroma[csp].filter_vpp[partEnum](refCb, refStride, dstCb, dstStride, yFrac);
        primitives.chroma[csp].filter_vpp[partEnum](refCr, refStride, dstCr, dstStride, yFrac);
    }
    else
    {
        int hShift = CHROMA_H_SHIFT(csp);
        int vShift = CHROMA_V_SHIFT(csp);
        uint32_t cxWidth = width >> hShift;
        uint32_t cxHeight = height >> vShift;
        int extStride = cxWidth;
        int filterSize = NTAPS_CHROMA;
        int halfFilterSize = (filterSize >> 1);

        primitives.chroma[csp].filter_hps[partEnum](refCb, refStride, m_immedVals, extStride,  xFrac, 1);
        primitives.chroma_vsp(m_immedVals + (halfFilterSize - 1) * extStride, extStride, dstCb, dstStride, cxWidth, cxHeight, yFrac);

        primitives.chroma[csp].filter_hps[partEnum](refCr, refStride, m_immedVals, extStride, xFrac, 1);
        primitives.chroma_vsp(m_immedVals + (halfFilterSize - 1) * extStride, extStride, dstCr, dstStride, cxWidth, cxHeight, yFrac);
    }
}

// Generate motion compensated block when biprediction
void TComPrediction::xPredInterChromaBlk(TComDataCU *cu, TComPicYuv *refPic, uint32_t partAddr, MV *mv, int width, int height, TShortYUV *dstPic)
{
    int refStride = refPic->getCStride();
    int dstStride = dstPic->m_cwidth;

    int refOffset = (mv->x >> 3) + (mv->y >> 3) * refStride;

    Pel* refCb = refPic->getCbAddr(cu->getAddr(), cu->getZorderIdxInCU() + partAddr) + refOffset;
    Pel* refCr = refPic->getCrAddr(cu->getAddr(), cu->getZorderIdxInCU() + partAddr) + refOffset;

    int16_t* dstCb = dstPic->getCbAddr(partAddr);
    int16_t* dstCr = dstPic->getCrAddr(partAddr);

    int xFrac = mv->x & 0x7;
    int yFrac = mv->y & 0x7;

    int partEnum = partitionFromSizes(width, height);
    int csp = X265_CSP_I420;

    uint32_t cxWidth = width >> 1;
    uint32_t cxHeight = height >> 1;

    assert(dstStride == MAX_CU_SIZE / 2);
    assert(((cxWidth | cxHeight) % 2) == 0);

    if ((yFrac | xFrac) == 0)
    {
        primitives.chroma_p2s(refCb, refStride, dstCb, cxWidth, cxHeight);
        primitives.chroma_p2s(refCr, refStride, dstCr, cxWidth, cxHeight);
    }
    else if (yFrac == 0)
    {
        primitives.chroma[csp].filter_hps[partEnum](refCb, refStride, dstCb, dstStride, xFrac, 0);
        primitives.chroma[csp].filter_hps[partEnum](refCr, refStride, dstCr, dstStride, xFrac, 0);
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
        primitives.chroma[csp].filter_hps[partEnum](refCb, refStride, m_immedVals, extStride, xFrac, 1);
        primitives.ipfilter_ss[FILTER_V_S_S_4](m_immedVals + (halfFilterSize - 1) * extStride, extStride, dstCb, dstStride, cxWidth, cxHeight, yFrac);
        primitives.chroma[csp].filter_hps[partEnum](refCr, refStride, m_immedVals, extStride, xFrac, 1);
        primitives.ipfilter_ss[FILTER_V_S_S_4](m_immedVals + (halfFilterSize - 1) * extStride, extStride, dstCr, dstStride, cxWidth, cxHeight, yFrac);
    }
}

void TComPrediction::xWeightedAverage(TComYuv* srcYuv0, TComYuv* srcYuv1, int refIdx0, int refIdx1, uint32_t partIdx, int width, int height, TComYuv*& outDstYuv, bool bLuma, bool bChroma)
{
    if (refIdx0 >= 0 && refIdx1 >= 0)
    {
        outDstYuv->addAvg(srcYuv0, srcYuv1, partIdx, width, height, bLuma, bChroma);
    }
    else if (refIdx0 >= 0 && refIdx1 <  0)
    {
        srcYuv0->copyPartToPartYuv(outDstYuv, partIdx, width, height, bLuma, bChroma);
    }
    else if (refIdx0 <  0 && refIdx1 >= 0)
    {
        srcYuv1->copyPartToPartYuv(outDstYuv, partIdx, width, height, bLuma, bChroma);
    }
}

// AMVP
void TComPrediction::getMvPredAMVP(TComDataCU* cu, uint32_t partIdx, uint32_t partAddr, int list, MV& mvPred)
{
    AMVPInfo* pcAMVPInfo = cu->getCUMvField(list)->getAMVPInfo();

    if (pcAMVPInfo->m_num <= 1)
    {
        mvPred = pcAMVPInfo->m_mvCand[0];

        cu->setMVPIdxSubParts(0, list, partAddr, partIdx, cu->getDepth(partAddr));
        cu->setMVPNumSubParts(pcAMVPInfo->m_num, list, partAddr, partIdx, cu->getDepth(partAddr));
        return;
    }

    assert(cu->getMVPIdx(list, partAddr) >= 0);
    mvPred = pcAMVPInfo->m_mvCand[cu->getMVPIdx(list, partAddr)];
}

//! \}
