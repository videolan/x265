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

static const uint8_t intraFilterThreshold[5] =
{
    10, //4x4
    7,  //8x8
    1,  //16x16
    0,  //32x32
    10, //64x64
};

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

    if (m_predBuf == NULL)
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

bool Predict::filteringIntraReferenceSamples(uint32_t dirMode, uint32_t log2TrSize)
{
    bool bFilter;

    if (dirMode == DC_IDX || log2TrSize <= 2)
    {
        bFilter = false; // no smoothing for DC
    }
    else
    {
        int diff = std::min<int>(abs((int)dirMode - HOR_IDX), abs((int)dirMode - VER_IDX));
        uint32_t sizeIdx = log2TrSize - 2;
        bFilter = diff > intraFilterThreshold[sizeIdx];
    }

    return bFilter;
}

void Predict::predIntraLumaAng(uint32_t dirMode, pixel* dst, intptr_t stride, uint32_t log2TrSize)
{
    int tuSize = 1 << log2TrSize;
    bool bUseFilteredPredictions = filteringIntraReferenceSamples(dirMode, log2TrSize);

    pixel *refLft, *refAbv;
    refLft = m_refLeft + tuSize - 1;
    refAbv = m_refAbove + tuSize - 1;

    if (bUseFilteredPredictions)
    {
        refLft = m_refLeftFlt + tuSize - 1;
        refAbv = m_refAboveFlt + tuSize - 1;
    }

    bool bFilter = log2TrSize <= 4 && dirMode != PLANAR_IDX;
    int sizeIdx = log2TrSize - 2;
    X265_CHECK(sizeIdx >= 0 && sizeIdx < 4, "intra block size is out of range\n");
    primitives.intra_pred[sizeIdx][dirMode](dst, stride, refLft, refAbv, dirMode, bFilter);
}

void Predict::predIntraChromaAng(pixel* src, uint32_t dirMode, pixel* dst, intptr_t stride, uint32_t log2TrSizeC, int chFmt)
{
    int tuSize = 1 << log2TrSizeC;
    uint32_t tuSize2 = tuSize << 1;

    // Create the prediction
    pixel refAbv[3 * MAX_CU_SIZE];
    pixel refLft[3 * MAX_CU_SIZE];

    bool bUseFilteredPredictions = (chFmt == X265_CSP_I444 && filteringIntraReferenceSamples(dirMode, log2TrSizeC));

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
        {
            filterBuf[l++] = src[ADI_BUF_STRIDE * (tuSize2 - i)];
        }

        // top left corner
        filterBuf[l++] = src[0];

        // above border from left to right
        memcpy(&filterBuf[l], &src[1], tuSize2 * sizeof(*filterBuf));

        // 1. filtering with [1 2 1]
        filterBufN[0] = filterBuf[0];
        filterBufN[bufSize - 1] = filterBuf[bufSize - 1];
        for (int i = 1; i < bufSize - 1; i++)
        {
            filterBufN[i] = (filterBuf[i - 1] + 2 * filterBuf[i] + filterBuf[i + 1] + 2) >> 2;
        }

        // initialization of ADI buffers
        int limit = tuSize2 + 1;
        memcpy(refAbv + tuSize - 1, filterBufN + tuSize2, limit * sizeof(pixel));
        for (int k = 0; k < limit; k++)
        {
            refLft[k + tuSize - 1] = filterBufN[tuSize2 - k];   // Smoothened
        }
    }
    else
    {
        int limit = (dirMode <= 25 && dirMode >= 11) ? (tuSize + 1 + 1) : (tuSize2 + 1);
        memcpy(refAbv + tuSize - 1, src, (limit) * sizeof(pixel));
        for (int k = 0; k < limit; k++)
        {
            refLft[k + tuSize - 1] = src[k * ADI_BUF_STRIDE];
        }
    }

    int sizeIdx = log2TrSizeC - 2;
    X265_CHECK(sizeIdx >= 0 && sizeIdx < 4, "intra block size is out of range\n");
    primitives.intra_pred[sizeIdx][dirMode](dst, stride, refLft + tuSize - 1, refAbv + tuSize - 1, dirMode, 0);
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

    ClippedMv[0] = m_mvField[0]->getMv(m_partAddr);
    ClippedMv[1] = m_mvField[1]->getMv(m_partAddr);
    cu->clipMv(ClippedMv[0]);
    cu->clipMv(ClippedMv[1]);
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
                predInterLumaBlk(m_slice->m_refPicList[list][refId]->getPicYuvRec(), shortYuv, &ClippedMv[list]);
            if (bChroma)
                predInterChromaBlk(m_slice->m_refPicList[list][refId]->getPicYuvRec(), shortYuv, &ClippedMv[list]);

            xWeightedPredictionUni(cu, shortYuv, m_partAddr, m_width, m_height, list, predYuv, -1, bLuma, bChroma);
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
        predInterLumaBlk(m_slice->m_refPicList[list][refIdx]->getPicYuvRec(), outPredYuv, &ClippedMv[list]);

    if (bChroma)
        predInterChromaBlk(m_slice->m_refPicList[list][refIdx]->getPicYuvRec(), outPredYuv, &ClippedMv[list]);
}

void Predict::predInterUni(int list, ShortYuv* outPredYuv, bool bLuma, bool bChroma)
{
    int refIdx = m_mvField[list]->getRefIdx(m_partAddr);

    X265_CHECK(refIdx >= 0, "refidx is not positive\n");

    if (bLuma)
        predInterLumaBlk(m_slice->m_refPicList[list][refIdx]->getPicYuvRec(), outPredYuv, &ClippedMv[list]);
    if (bChroma)
        predInterChromaBlk(m_slice->m_refPicList[list][refIdx]->getPicYuvRec(), outPredYuv, &ClippedMv[list]);
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
            xWeightedPredictionBi(cu, &m_predShortYuv[0], &m_predShortYuv[1], refIdx[0], refIdx[1], m_partAddr, m_width, m_height, outPredYuv, bLuma, bChroma);
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

        xWeightedPredictionBi(cu, &m_predShortYuv[0], &m_predShortYuv[1], refIdx[0], refIdx[1], m_partAddr, m_width, m_height, outPredYuv, bLuma, bChroma);
    }
    else if (refIdx[0] >= 0)
    {
        const int list = 0;

        X265_CHECK(refIdx[list] < m_slice->m_numRefIdx[list], "refidx out of range\n");

        predInterUni(list, outPredYuv, bLuma, bChroma);
    }
    else
    {
        X265_CHECK(refIdx[1] >= 0, "refidx[1] was not positive\n");

        const int list = 1;

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

    if ((yFrac | xFrac) == 0)
    {
        primitives.luma_p2s(ref, refStride, dst, m_width, m_height);
    }
    else if (yFrac == 0)
    {
        primitives.luma_hps[partEnum](ref, refStride, dst, dstStride, xFrac, 0);
    }
    else if (xFrac == 0)
    {
        primitives.luma_vps[partEnum](ref, refStride, dst, dstStride, yFrac);
    }
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
    
    if ((yFrac | xFrac) == 0)
    {
        primitives.chroma[m_csp].copy_pp[partEnum](dstCb, dstStride, refCb, refStride);
        primitives.chroma[m_csp].copy_pp[partEnum](dstCr, dstStride, refCr, refStride);
    }
    else if (yFrac == 0)
    {
        primitives.chroma[m_csp].filter_hpp[partEnum](refCb, refStride, dstCb, dstStride, xFrac << (1 - hChromaShift));
        primitives.chroma[m_csp].filter_hpp[partEnum](refCr, refStride, dstCr, dstStride, xFrac << (1 - hChromaShift));
    }
    else if (xFrac == 0)
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

    if ((yFrac | xFrac) == 0)
    {
        primitives.chroma_p2s[m_csp](refCb, refStride, dstCb, cxWidth, cxHeight);
        primitives.chroma_p2s[m_csp](refCr, refStride, dstCr, cxWidth, cxHeight);
    }
    else if (yFrac == 0)
    {
        primitives.chroma[m_csp].filter_hps[partEnum](refCb, refStride, dstCb, dstStride, xFrac << (1 - hChromaShift), 0);
        primitives.chroma[m_csp].filter_hps[partEnum](refCr, refStride, dstCr, dstStride, xFrac << (1 - hChromaShift), 0);
    }
    else if (xFrac == 0)
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
