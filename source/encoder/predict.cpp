/*****************************************************************************
* Copyright (C) 2013 x265 project
*
* Authors: Deepthi Nandakumar <deepthi@multicorewareinc.com>
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

#include "predict.h"
#include "primitives.h"
#include "common.h"

using namespace x265;

namespace
{
inline pixel weightBidir(int w0, int16_t P0, int w1, int16_t P1, int round, int shift, int offset)
{
    return Clip((w0 * (P0 + IF_INTERNAL_OFFS) + w1 * (P1 + IF_INTERNAL_OFFS) + round + (offset << (shift - 1))) >> shift);
}
}

Predict::Predict()
{
    m_predBuf = NULL;
    m_refAbove = NULL;
    m_refAboveFlt = NULL;
    m_refLeft = NULL;
    m_refLeftFlt = NULL;
    m_immedVals = NULL;
}

Predict::~Predict()
{
    X265_FREE(m_predBuf);
    X265_FREE(m_refAbove);
    X265_FREE(m_refAboveFlt);
    X265_FREE(m_refLeft);
    X265_FREE(m_refLeftFlt);
    X265_FREE(m_immedVals);

    m_predShortYuv[0].destroy();
    m_predShortYuv[1].destroy();
}

void Predict::initTempBuff(int csp)
{
    m_csp = csp;

    if (!m_predBuf)
    {
        int predBufHeight = ((MAX_CU_SIZE + 2) << 4);
        int predBufStride = ((MAX_CU_SIZE + 8) << 4);
        m_predBuf = X265_MALLOC(pixel, predBufStride * predBufHeight);

        m_refAbove = X265_MALLOC(pixel, 3 * MAX_CU_SIZE);
        m_refAboveFlt = X265_MALLOC(pixel, 3 * MAX_CU_SIZE);
        m_refLeft = X265_MALLOC(pixel, 3 * MAX_CU_SIZE);
        m_refLeftFlt = X265_MALLOC(pixel, 3 * MAX_CU_SIZE);

        m_predShortYuv[0].create(MAX_CU_SIZE, MAX_CU_SIZE, csp);
        m_predShortYuv[1].create(MAX_CU_SIZE, MAX_CU_SIZE, csp);

        m_immedVals = X265_MALLOC(int16_t, 64 * (64 + NTAPS_LUMA - 1));
    }
}

void Predict::predIntraLumaAng(uint32_t dirMode, pixel* dst, intptr_t stride, uint32_t log2TrSize)
{
    int tuSize = 1 << log2TrSize;

    pixel *refLft, *refAbv;

    if (!(g_intraFilterFlags[dirMode] & tuSize))
    {
        refLft = m_refLeft + tuSize - 1;
        refAbv = m_refAbove + tuSize - 1;
    }
    else
    {
        refLft = m_refLeftFlt + tuSize - 1;
        refAbv = m_refAboveFlt + tuSize - 1;
    }

    bool bFilter = log2TrSize <= 4;
    int sizeIdx = log2TrSize - 2;
    X265_CHECK(sizeIdx >= 0 && sizeIdx < 4, "intra block size is out of range\n");
    primitives.intra_pred[dirMode][sizeIdx](dst, stride, refLft, refAbv, dirMode, bFilter);
}

void Predict::predIntraChromaAng(pixel* src, uint32_t dirMode, pixel* dst, intptr_t stride, uint32_t log2TrSizeC, int chFmt)
{
    int tuSize = 1 << log2TrSizeC;
    uint32_t tuSize2 = tuSize << 1;

    // Create the prediction
    pixel* refAbv;
    pixel refLft[3 * MAX_CU_SIZE];

    bool bUseFilteredPredictions = (chFmt == X265_CSP_I444 && (g_intraFilterFlags[dirMode] & tuSize));

    if (bUseFilteredPredictions)
    {
        // generate filtered intra prediction samples
        // left and left above border + above and above right border + top left corner = length of 3. filter buffer
        int bufSize = tuSize2 + tuSize2 + 1;
        uint32_t wh = ADI_BUF_STRIDE * (tuSize2 + 1);         // number of elements in one buffer

        pixel* filterBuf  = src + wh;            // buffer for 2. filtering (sequential)
        pixel* filterBufN = filterBuf + bufSize; // buffer for 1. filtering (sequential)

        int l = 0;
        // left border from bottom to top
        for (uint32_t i = 0; i < tuSize2; i++)
            filterBuf[l++] = src[ADI_BUF_STRIDE * (tuSize2 - i)];

        // top left corner
        filterBuf[l++] = src[0];

        // above border from left to right
        memcpy(&filterBuf[l], &src[1], tuSize2 * sizeof(*filterBuf));

        // 1. filtering with [1 2 1]
        filterBufN[0] = filterBuf[0];
        filterBufN[bufSize - 1] = filterBuf[bufSize - 1];
        for (int i = 1; i < bufSize - 1; i++)
            filterBufN[i] = (filterBuf[i - 1] + 2 * filterBuf[i] + filterBuf[i + 1] + 2) >> 2;

        // initialization of ADI buffers
        int limit = tuSize2 + 1;
        refAbv = filterBufN + tuSize2;
        for (int k = 0; k < limit; k++)
            refLft[k + tuSize - 1] = filterBufN[tuSize2 - k];   // Smoothened
    }
    else
    {
        int limit = (dirMode <= 25 && dirMode >= 11) ? (tuSize + 1 + 1) : (tuSize2 + 1);
        refAbv = src;
        for (int k = 0; k < limit; k++)
            refLft[k + tuSize - 1] = src[k * ADI_BUF_STRIDE];
    }

    int sizeIdx = log2TrSizeC - 2;
    X265_CHECK(sizeIdx >= 0 && sizeIdx < 4, "intra block size is out of range\n");
    primitives.intra_pred[dirMode][sizeIdx](dst, stride, refLft + tuSize - 1, refAbv, dirMode, 0);
}

bool Predict::checkIdenticalMotion()
{
    X265_CHECK(m_slice->isInterB(), "identical motion check in P frame\n");
    if (!m_slice->m_pps->bUseWeightedBiPred)
    {
        int refIdxL0 = m_mvField[0]->getRefIdx(m_partAddr);
        int refIdxL1 = m_mvField[1]->getRefIdx(m_partAddr);
        if (refIdxL0 >= 0 && refIdxL1 >= 0)
        {
            int refPOCL0 = m_slice->m_refPOCList[0][refIdxL0];
            int refPOCL1 = m_slice->m_refPOCList[1][refIdxL1];
            if (refPOCL0 == refPOCL1 && m_mvField[0]->getMv(m_partAddr) == m_mvField[1]->getMv(m_partAddr))
                return true;
        }
    }
    return false;
}

void Predict::prepMotionCompensation(TComDataCU* cu, int partIdx)
{
    m_slice = cu->m_slice;
    cu->getPartIndexAndSize(partIdx, m_partAddr, m_width, m_height);
    m_cuAddr = cu->getAddr();
    m_zOrderIdxinCU = cu->getZorderIdxInCU();

    m_mvField[0] = cu->getCUMvField(REF_PIC_LIST_0);
    m_mvField[1] = cu->getCUMvField(REF_PIC_LIST_1);

    m_clippedMv[0] = m_mvField[0]->getMv(m_partAddr);
    m_clippedMv[1] = m_mvField[1]->getMv(m_partAddr);
    cu->clipMv(m_clippedMv[0]);
    cu->clipMv(m_clippedMv[1]);
}

void Predict::motionCompensation(TComDataCU* cu, TComYuv* predYuv, int list, bool bLuma, bool bChroma)
{
    if (m_slice->isInterP())
        list = REF_PIC_LIST_0;
    if (list != REF_PIC_LIST_X)
    {
        if (m_slice->m_pps->bUseWeightPred)
        {
            ShortYuv* shortYuv = &m_predShortYuv[0];
            int refId = m_mvField[list]->getRefIdx(m_partAddr);
            X265_CHECK(refId >= 0, "refidx is not positive\n");

            if (bLuma)
                predInterLumaBlk(m_slice->m_refPicList[list][refId]->getPicYuvRec(), shortYuv, &m_clippedMv[list]);
            if (bChroma)
                predInterChromaBlk(m_slice->m_refPicList[list][refId]->getPicYuvRec(), shortYuv, &m_clippedMv[list]);

            weightedPredictionUni(cu, shortYuv, m_partAddr, m_width, m_height, list, predYuv, -1, bLuma, bChroma);
        }
        else
            predInterUni(list, predYuv, bLuma, bChroma);
    }
    else
    {
        if (checkIdenticalMotion())
            predInterUni(REF_PIC_LIST_0, predYuv, bLuma, bChroma);
        else
            predInterBi(cu, predYuv, bLuma, bChroma);
    }
}

void Predict::predInterUni(int list, TComYuv* outPredYuv, bool bLuma, bool bChroma)
{
    int refIdx = m_mvField[list]->getRefIdx(m_partAddr);

    X265_CHECK(refIdx >= 0, "refidx is not positive\n");

    if (bLuma)
        predInterLumaBlk(m_slice->m_refPicList[list][refIdx]->getPicYuvRec(), outPredYuv, &m_clippedMv[list]);

    if (bChroma)
        predInterChromaBlk(m_slice->m_refPicList[list][refIdx]->getPicYuvRec(), outPredYuv, &m_clippedMv[list]);
}

void Predict::predInterUni(int list, ShortYuv* outPredYuv, bool bLuma, bool bChroma)
{
    int refIdx = m_mvField[list]->getRefIdx(m_partAddr);

    X265_CHECK(refIdx >= 0, "refidx is not positive\n");

    if (bLuma)
        predInterLumaBlk(m_slice->m_refPicList[list][refIdx]->getPicYuvRec(), outPredYuv, &m_clippedMv[list]);

    if (bChroma)
        predInterChromaBlk(m_slice->m_refPicList[list][refIdx]->getPicYuvRec(), outPredYuv, &m_clippedMv[list]);
}

void Predict::predInterBi(TComDataCU* cu, TComYuv* outPredYuv, bool bLuma, bool bChroma)
{
    X265_CHECK(m_slice->isInterB(), "biprediction in P frame\n");

    int refIdx[2];
    refIdx[0] = m_mvField[REF_PIC_LIST_0]->getRefIdx(m_partAddr);
    refIdx[1] = m_mvField[REF_PIC_LIST_1]->getRefIdx(m_partAddr);

    if (refIdx[0] >= 0 && refIdx[1] >= 0)
    {
        for (int list = 0; list < 2; list++)
        {
            X265_CHECK(refIdx[list] < m_slice->m_numRefIdx[list], "refidx out of range\n");

            predInterUni(list, &m_predShortYuv[list], bLuma, bChroma);
        }

        if (m_slice->m_pps->bUseWeightedBiPred)
            weightedPredictionBi(cu, &m_predShortYuv[0], &m_predShortYuv[1], refIdx[0], refIdx[1], m_partAddr, m_width, m_height, outPredYuv, bLuma, bChroma);
        else
            outPredYuv->addAvg(&m_predShortYuv[0], &m_predShortYuv[1], m_partAddr, m_width, m_height, bLuma, bChroma);
    }
    else if (m_slice->m_pps->bUseWeightedBiPred)
    {
        for (int list = 0; list < 2; list++)
        {
            if (refIdx[list] < 0) continue;

            X265_CHECK(refIdx[list] < cu->m_slice->m_numRefIdx[list], "refidx out of range\n");

            predInterUni(list, &m_predShortYuv[list], bLuma, bChroma);
        }

        weightedPredictionBi(cu, &m_predShortYuv[0], &m_predShortYuv[1], refIdx[0], refIdx[1], m_partAddr, m_width, m_height, outPredYuv, bLuma, bChroma);
    }
    else if (refIdx[0] >= 0)
    {
        const int list = 0;

        X265_CHECK(refIdx[list] < m_slice->m_numRefIdx[list], "refidx out of range\n");

        predInterUni(list, outPredYuv, bLuma, bChroma);
    }
    else
    {
        const int list = 1;

        X265_CHECK(refIdx[list] >= 0, "refidx[1] was not positive\n");
        X265_CHECK(refIdx[list] < m_slice->m_numRefIdx[list], "refidx out of range\n");

        predInterUni(list, outPredYuv, bLuma, bChroma);
    }
}

void Predict::predInterLumaBlk(TComPicYuv *refPic, TComYuv *dstPic, MV *mv)
{
    int dstStride = dstPic->getStride();
    pixel *dst    = dstPic->getLumaAddr(m_partAddr);

    int srcStride = refPic->getStride();
    int srcOffset = (mv->x >> 2) + (mv->y >> 2) * srcStride;
    int partEnum = partitionFromSizes(m_width, m_height);
    pixel* src = refPic->getLumaAddr(m_cuAddr, m_zOrderIdxinCU + m_partAddr) + srcOffset;

    int xFrac = mv->x & 0x3;
    int yFrac = mv->y & 0x3;

    if (!(yFrac | xFrac))
        primitives.luma_copy_pp[partEnum](dst, dstStride, src, srcStride);
    else if (!yFrac)
        primitives.luma_hpp[partEnum](src, srcStride, dst, dstStride, xFrac);
    else if (!xFrac)
        primitives.luma_vpp[partEnum](src, srcStride, dst, dstStride, yFrac);
    else
    {
        int tmpStride = m_width;
        int filterSize = NTAPS_LUMA;
        int halfFilterSize = (filterSize >> 1);
        primitives.luma_hps[partEnum](src, srcStride, m_immedVals, tmpStride, xFrac, 1);
        primitives.luma_vsp[partEnum](m_immedVals + (halfFilterSize - 1) * tmpStride, tmpStride, dst, dstStride, yFrac);
    }
}

void Predict::predInterLumaBlk(TComPicYuv *refPic, ShortYuv *dstPic, MV *mv)
{
    int refStride = refPic->getStride();
    int refOffset = (mv->x >> 2) + (mv->y >> 2) * refStride;
    pixel *ref    = refPic->getLumaAddr(m_cuAddr, m_zOrderIdxinCU + m_partAddr) + refOffset;

    int dstStride = dstPic->m_width;
    int16_t *dst  = dstPic->getLumaAddr(m_partAddr);

    int xFrac = mv->x & 0x3;
    int yFrac = mv->y & 0x3;

    int partEnum = partitionFromSizes(m_width, m_height);

    X265_CHECK((m_width % 4) + (m_height % 4) == 0, "width or height not divisible by 4\n");
    X265_CHECK(dstStride == MAX_CU_SIZE, "stride expected to be max cu size\n");

    if (!(yFrac | xFrac))
        primitives.luma_p2s(ref, refStride, dst, m_width, m_height);
    else if (!yFrac)
        primitives.luma_hps[partEnum](ref, refStride, dst, dstStride, xFrac, 0);
    else if (!xFrac)
        primitives.luma_vps[partEnum](ref, refStride, dst, dstStride, yFrac);
    else
    {
        int tmpStride = m_width;
        int filterSize = NTAPS_LUMA;
        int halfFilterSize = (filterSize >> 1);
        primitives.luma_hps[partEnum](ref, refStride, m_immedVals, tmpStride, xFrac, 1);
        primitives.luma_vss[partEnum](m_immedVals + (halfFilterSize - 1) * tmpStride, tmpStride, dst, dstStride, yFrac);
    }
}

void Predict::predInterChromaBlk(TComPicYuv *refPic, TComYuv *dstPic, MV *mv)
{
    int refStride = refPic->getCStride();
    int dstStride = dstPic->getCStride();
    int hChromaShift = CHROMA_H_SHIFT(m_csp);
    int vChromaShift = CHROMA_V_SHIFT(m_csp);

    int shiftHor = (2 + hChromaShift);
    int shiftVer = (2 + vChromaShift);

    int refOffset = (mv->x >> shiftHor) + (mv->y >> shiftVer) * refStride;

    pixel* refCb = refPic->getCbAddr(m_cuAddr, m_zOrderIdxinCU + m_partAddr) + refOffset;
    pixel* refCr = refPic->getCrAddr(m_cuAddr, m_zOrderIdxinCU + m_partAddr) + refOffset;

    pixel* dstCb = dstPic->getCbAddr(m_partAddr);
    pixel* dstCr = dstPic->getCrAddr(m_partAddr);

    int xFrac = mv->x & ((1 << shiftHor) - 1);
    int yFrac = mv->y & ((1 << shiftVer) - 1);

    int partEnum = partitionFromSizes(m_width, m_height);
    
    if (!(yFrac | xFrac))
    {
        primitives.chroma[m_csp].copy_pp[partEnum](dstCb, dstStride, refCb, refStride);
        primitives.chroma[m_csp].copy_pp[partEnum](dstCr, dstStride, refCr, refStride);
    }
    else if (!yFrac)
    {
        primitives.chroma[m_csp].filter_hpp[partEnum](refCb, refStride, dstCb, dstStride, xFrac << (1 - hChromaShift));
        primitives.chroma[m_csp].filter_hpp[partEnum](refCr, refStride, dstCr, dstStride, xFrac << (1 - hChromaShift));
    }
    else if (!xFrac)
    {
        primitives.chroma[m_csp].filter_vpp[partEnum](refCb, refStride, dstCb, dstStride, yFrac << (1 - vChromaShift));
        primitives.chroma[m_csp].filter_vpp[partEnum](refCr, refStride, dstCr, dstStride, yFrac << (1 - vChromaShift));
    }
    else
    {
        int extStride = m_width >> hChromaShift;
        int filterSize = NTAPS_CHROMA;
        int halfFilterSize = (filterSize >> 1);

        primitives.chroma[m_csp].filter_hps[partEnum](refCb, refStride, m_immedVals, extStride, xFrac << (1 - hChromaShift), 1);
        primitives.chroma[m_csp].filter_vsp[partEnum](m_immedVals + (halfFilterSize - 1) * extStride, extStride, dstCb, dstStride, yFrac << (1 - vChromaShift));

        primitives.chroma[m_csp].filter_hps[partEnum](refCr, refStride, m_immedVals, extStride, xFrac << (1 - hChromaShift), 1);
        primitives.chroma[m_csp].filter_vsp[partEnum](m_immedVals + (halfFilterSize - 1) * extStride, extStride, dstCr, dstStride, yFrac << (1 - vChromaShift));
    }
}

void Predict::predInterChromaBlk(TComPicYuv *refPic, ShortYuv *dstPic, MV *mv)
{
    int refStride = refPic->getCStride();
    int dstStride = dstPic->m_cwidth;
    int hChromaShift = CHROMA_H_SHIFT(m_csp);
    int vChromaShift = CHROMA_V_SHIFT(m_csp);

    int shiftHor = (2 + hChromaShift);
    int shiftVer = (2 + vChromaShift);

    int refOffset = (mv->x >> shiftHor) + (mv->y >> shiftVer) * refStride;

    pixel* refCb = refPic->getCbAddr(m_cuAddr, m_zOrderIdxinCU + m_partAddr) + refOffset;
    pixel* refCr = refPic->getCrAddr(m_cuAddr, m_zOrderIdxinCU + m_partAddr) + refOffset;

    int16_t* dstCb = dstPic->getCbAddr(m_partAddr);
    int16_t* dstCr = dstPic->getCrAddr(m_partAddr);

    int xFrac = mv->x & ((1 << shiftHor) - 1);
    int yFrac = mv->y & ((1 << shiftVer) - 1);

    int partEnum = partitionFromSizes(m_width, m_height);
    
    uint32_t cxWidth  = m_width >> hChromaShift;
    uint32_t cxHeight = m_height >> vChromaShift;

    X265_CHECK(((cxWidth | cxHeight) % 2) == 0, "chroma block size expected to be multiple of 2\n");

    if (!(yFrac | xFrac))
    {
        primitives.chroma_p2s[m_csp](refCb, refStride, dstCb, cxWidth, cxHeight);
        primitives.chroma_p2s[m_csp](refCr, refStride, dstCr, cxWidth, cxHeight);
    }
    else if (!yFrac)
    {
        primitives.chroma[m_csp].filter_hps[partEnum](refCb, refStride, dstCb, dstStride, xFrac << (1 - hChromaShift), 0);
        primitives.chroma[m_csp].filter_hps[partEnum](refCr, refStride, dstCr, dstStride, xFrac << (1 - hChromaShift), 0);
    }
    else if (!xFrac)
    {
        primitives.chroma[m_csp].filter_vps[partEnum](refCb, refStride, dstCb, dstStride, yFrac << (1 - vChromaShift));
        primitives.chroma[m_csp].filter_vps[partEnum](refCr, refStride, dstCr, dstStride, yFrac << (1 - vChromaShift));
    }
    else
    {
        int extStride = cxWidth;
        int filterSize = NTAPS_CHROMA;
        int halfFilterSize = (filterSize >> 1);
        primitives.chroma[m_csp].filter_hps[partEnum](refCb, refStride, m_immedVals, extStride, xFrac << (1 - hChromaShift), 1);
        primitives.chroma[m_csp].filter_vss[partEnum](m_immedVals + (halfFilterSize - 1) * extStride, extStride, dstCb, dstStride, yFrac << (1 - vChromaShift));
        primitives.chroma[m_csp].filter_hps[partEnum](refCr, refStride, m_immedVals, extStride, xFrac << (1 - hChromaShift), 1);
        primitives.chroma[m_csp].filter_vss[partEnum](m_immedVals + (halfFilterSize - 1) * extStride, extStride, dstCr, dstStride, yFrac << (1 - vChromaShift));
    }
}

/* weighted averaging for bi-pred */
void Predict::addWeightBi(ShortYuv* srcYuv0, ShortYuv* srcYuv1, uint32_t partUnitIdx, uint32_t width, uint32_t height, WeightParam *wp0, WeightParam *wp1, TComYuv* outPredYuv, bool bLuma, bool bChroma)
{
    int x, y;

    int w0, w1, offset, shiftNum, shift, round;
    uint32_t src0Stride, src1Stride, dststride;

    int16_t* srcY0 = srcYuv0->getLumaAddr(partUnitIdx);
    int16_t* srcU0 = srcYuv0->getCbAddr(partUnitIdx);
    int16_t* srcV0 = srcYuv0->getCrAddr(partUnitIdx);

    int16_t* srcY1 = srcYuv1->getLumaAddr(partUnitIdx);
    int16_t* srcU1 = srcYuv1->getCbAddr(partUnitIdx);
    int16_t* srcV1 = srcYuv1->getCrAddr(partUnitIdx);

    pixel* dstY    = outPredYuv->getLumaAddr(partUnitIdx);
    pixel* dstU    = outPredYuv->getCbAddr(partUnitIdx);
    pixel* dstV    = outPredYuv->getCrAddr(partUnitIdx);

    if (bLuma)
    {
        // Luma
        w0      = wp0[0].w;
        offset  = wp0[0].o + wp1[0].o;
        shiftNum = IF_INTERNAL_PREC - X265_DEPTH;
        shift   = wp0[0].shift + shiftNum + 1;
        round   = shift ? (1 << (shift - 1)) : 0;
        w1      = wp1[0].w;

        src0Stride = srcYuv0->m_width;
        src1Stride = srcYuv1->m_width;
        dststride  = outPredYuv->getStride();

        for (y = height - 1; y >= 0; y--)
        {
            for (x = width - 1; x >= 0; )
            {
                // note: luma min width is 4
                dstY[x] = weightBidir(w0, srcY0[x], w1, srcY1[x], round, shift, offset);
                x--;
                dstY[x] = weightBidir(w0, srcY0[x], w1, srcY1[x], round, shift, offset);
                x--;
                dstY[x] = weightBidir(w0, srcY0[x], w1, srcY1[x], round, shift, offset);
                x--;
                dstY[x] = weightBidir(w0, srcY0[x], w1, srcY1[x], round, shift, offset);
                x--;
            }

            srcY0 += src0Stride;
            srcY1 += src1Stride;
            dstY  += dststride;
        }
    }

    if (bChroma)
    {
        // Chroma U
        w0      = wp0[1].w;
        offset  = wp0[1].o + wp1[1].o;
        shiftNum = IF_INTERNAL_PREC - X265_DEPTH;
        shift   = wp0[1].shift + shiftNum + 1;
        round   = shift ? (1 << (shift - 1)) : 0;
        w1      = wp1[1].w;

        src0Stride = srcYuv0->m_cwidth;
        src1Stride = srcYuv1->m_cwidth;
        dststride  = outPredYuv->getCStride();

        width  >>= srcYuv0->getHorzChromaShift();
        height >>= srcYuv0->getVertChromaShift();

        for (y = height - 1; y >= 0; y--)
        {
            for (x = width - 1; x >= 0; )
            {
                // note: chroma min width is 2
                dstU[x] = weightBidir(w0, srcU0[x], w1, srcU1[x], round, shift, offset);
                x--;
                dstU[x] = weightBidir(w0, srcU0[x], w1, srcU1[x], round, shift, offset);
                x--;
            }

            srcU0 += src0Stride;
            srcU1 += src1Stride;
            dstU  += dststride;
        }

        // Chroma V
        w0     = wp0[2].w;
        offset = wp0[2].o + wp1[2].o;
        shift  = wp0[2].shift + shiftNum + 1;
        round  = shift ? (1 << (shift - 1)) : 0;
        w1     = wp1[2].w;

        for (y = height - 1; y >= 0; y--)
        {
            for (x = width - 1; x >= 0; )
            {
                // note: chroma min width is 2
                dstV[x] = weightBidir(w0, srcV0[x], w1, srcV1[x], round, shift, offset);
                x--;
                dstV[x] = weightBidir(w0, srcV0[x], w1, srcV1[x], round, shift, offset);
                x--;
            }

            srcV0 += src0Stride;
            srcV1 += src1Stride;
            dstV  += dststride;
        }
    }
}

/* weighted averaging for uni-pred */
void Predict::addWeightUni(ShortYuv* srcYuv0, uint32_t partUnitIdx, uint32_t width, uint32_t height, WeightParam *wp0, TComYuv* outPredYuv, bool bLuma, bool bChroma)
{
    int16_t* srcY0 = srcYuv0->getLumaAddr(partUnitIdx);
    int16_t* srcU0 = srcYuv0->getCbAddr(partUnitIdx);
    int16_t* srcV0 = srcYuv0->getCrAddr(partUnitIdx);

    pixel* dstY = outPredYuv->getLumaAddr(partUnitIdx);
    pixel* dstU = outPredYuv->getCbAddr(partUnitIdx);
    pixel* dstV = outPredYuv->getCrAddr(partUnitIdx);

    int w0, offset, shiftNum, shift, round;
    uint32_t srcStride, dstStride;

    if (bLuma)
    {
        // Luma
        w0      = wp0[0].w;
        offset  = wp0[0].offset;
        shiftNum = IF_INTERNAL_PREC - X265_DEPTH;
        shift   = wp0[0].shift + shiftNum;
        round   = shift ? (1 << (shift - 1)) : 0;
        srcStride = srcYuv0->m_width;
        dstStride  = outPredYuv->getStride();

        primitives.weight_sp(srcY0, dstY, srcStride, dstStride, width, height, w0, round, shift, offset);
    }

    if (bChroma)
    {
        // Chroma U
        w0      = wp0[1].w;
        offset  = wp0[1].offset;
        shiftNum = IF_INTERNAL_PREC - X265_DEPTH;
        shift   = wp0[1].shift + shiftNum;
        round   = shift ? (1 << (shift - 1)) : 0;

        srcStride = srcYuv0->m_cwidth;
        dstStride = outPredYuv->getCStride();

        width  >>= srcYuv0->getHorzChromaShift();
        height >>= srcYuv0->getVertChromaShift();

        primitives.weight_sp(srcU0, dstU, srcStride, dstStride, width, height, w0, round, shift, offset);

        // Chroma V
        w0     = wp0[2].w;
        offset = wp0[2].offset;
        shift  = wp0[2].shift + shiftNum;
        round  = shift ? (1 << (shift - 1)) : 0;

        primitives.weight_sp(srcV0, dstV, srcStride, dstStride, width, height, w0, round, shift, offset);
    }
}

/* derivation of wp tables */
void Predict::getWpScaling(TComDataCU* cu, int refIdx0, int refIdx1, WeightParam *&wp0, WeightParam *&wp1)
{
    Slice* slice = cu->m_slice;
    bool wpBiPred = slice->m_pps->bUseWeightedBiPred;
    bool bBiDir  = (refIdx0 >= 0 && refIdx1 >= 0);
    bool bUniDir = !bBiDir;

    if (bUniDir || wpBiPred)
    {
        if (refIdx0 >= 0)
            wp0 = slice->m_weightPredTable[0][refIdx0];

        if (refIdx1 >= 0)
            wp1 = slice->m_weightPredTable[1][refIdx1];
    }
    else
    {
        X265_CHECK(0, "unexpected wpScaling configuration\n");
    }

    if (refIdx0 < 0)
        wp0 = NULL;

    if (refIdx1 < 0)
        wp1 = NULL;

    if (bBiDir)
    {
        for (int yuv = 0; yuv < 3; yuv++)
        {
            wp0[yuv].w     = wp0[yuv].inputWeight;
            wp0[yuv].o     = wp0[yuv].inputOffset * (1 << (X265_DEPTH - 8));
            wp1[yuv].w     = wp1[yuv].inputWeight;
            wp1[yuv].o     = wp1[yuv].inputOffset * (1 << (X265_DEPTH - 8));
            wp0[yuv].shift = wp0[yuv].log2WeightDenom;
            wp0[yuv].round = (1 << wp0[yuv].log2WeightDenom);
            wp1[yuv].shift = wp0[yuv].shift;
            wp1[yuv].round = wp0[yuv].round;
        }
    }
    else
    {
        WeightParam* pwp = (refIdx0 >= 0) ? wp0 : wp1;
        for (int yuv = 0; yuv < 3; yuv++)
        {
            pwp[yuv].w      = pwp[yuv].inputWeight;
            pwp[yuv].offset = pwp[yuv].inputOffset * (1 << (X265_DEPTH - 8));
            pwp[yuv].shift  = pwp[yuv].log2WeightDenom;
            pwp[yuv].round  = (pwp[yuv].log2WeightDenom >= 1) ? (1 << (pwp[yuv].log2WeightDenom - 1)) : (0);
        }
    }
}

/* weighted prediction for bi-pred */
void Predict::weightedPredictionBi(TComDataCU* cu, ShortYuv* srcYuv0, ShortYuv* srcYuv1, int refIdx0, int refIdx1, uint32_t partIdx, int width, int height, TComYuv* outPredYuv, bool bLuma, bool bChroma)
{
    WeightParam *pwp0 = NULL, *pwp1 = NULL;

    getWpScaling(cu, refIdx0, refIdx1, pwp0, pwp1);

    if (refIdx0 >= 0 && refIdx1 >= 0)
        addWeightBi(srcYuv0, srcYuv1, partIdx, width, height, pwp0, pwp1, outPredYuv, bLuma, bChroma);
    else if (refIdx0 >= 0 && refIdx1 <  0)
        addWeightUni(srcYuv0, partIdx, width, height, pwp0, outPredYuv, bLuma, bChroma);
    else if (refIdx0 <  0 && refIdx1 >= 0)
        addWeightUni(srcYuv1, partIdx, width, height, pwp1, outPredYuv, bLuma, bChroma);
    else
    {
        X265_CHECK(0, "unexpected weighte biprediction configuration\n");
    }
}

/* weighted prediction for uni-pred */
void Predict::weightedPredictionUni(TComDataCU* cu, ShortYuv* srcYuv, uint32_t partAddr, int width, int height, int picList, TComYuv* outPredYuv, int refIdx, bool bLuma, bool bChroma)
{
    WeightParam  *pwp, *pwpTmp;

    if (refIdx < 0)
        refIdx = cu->getCUMvField(picList)->getRefIdx(partAddr);

    X265_CHECK(refIdx >= 0, "invalid refidx\n");

    if (picList == REF_PIC_LIST_0)
        getWpScaling(cu, refIdx, -1, pwp, pwpTmp);
    else
        getWpScaling(cu, -1, refIdx, pwpTmp, pwp);

    addWeightUni(srcYuv, partAddr, width, height, pwp, outPredYuv, bLuma, bChroma);
}
