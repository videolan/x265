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
    X265_FREE(m_immedVals);
    m_predShortYuv[0].destroy();
    m_predShortYuv[1].destroy();
}

bool Predict::allocBuffers(int csp)
{
    m_csp = csp;

    int predBufHeight = ((MAX_CU_SIZE + 2) << 4);
    int predBufStride = ((MAX_CU_SIZE + 8) << 4);
    CHECKED_MALLOC(m_predBuf, pixel, predBufStride * predBufHeight);
    CHECKED_MALLOC(m_immedVals, int16_t, 64 * (64 + NTAPS_LUMA - 1));
    CHECKED_MALLOC(m_refAbove, pixel, 12 * MAX_CU_SIZE);

    m_refAboveFlt = m_refAbove + 3 * MAX_CU_SIZE;
    m_refLeft = m_refAboveFlt + 3 * MAX_CU_SIZE;
    m_refLeftFlt = m_refLeft + 3 * MAX_CU_SIZE;

    return m_predShortYuv[0].create(MAX_CU_SIZE, MAX_CU_SIZE, csp) &&
           m_predShortYuv[1].create(MAX_CU_SIZE, MAX_CU_SIZE, csp);

fail:
    return false;
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
    int tuSize2 = tuSize << 1;

    // Create the prediction
    const int bufOffset = tuSize - 1;
    pixel buf0[3 * MAX_CU_SIZE];
    pixel buf1[3 * MAX_CU_SIZE];
    pixel* above;
    pixel* left = buf0 + bufOffset;

    int limit = (dirMode <= 25 && dirMode >= 11) ? (tuSize + 1 + 1) : (tuSize2 + 1);
    for (int k = 0; k < limit; k++)
        left[k] = src[k * ADI_BUF_STRIDE];

    bool bUseFilteredPredictions = (chFmt == X265_CSP_I444 && (g_intraFilterFlags[dirMode] & tuSize));

    if (bUseFilteredPredictions)
    {
        // generate filtered intra prediction samples
        buf0[bufOffset - 1] = src[1];
        left = buf1 + bufOffset;
        for (int i = 0; i < tuSize2; i++)
            left[i] = (buf0[bufOffset + i - 1] + 2 * buf0[bufOffset + i] + buf0[bufOffset + i + 1] + 2) >> 2;
        left[tuSize2] = buf0[bufOffset + tuSize2];

        above = buf0 + bufOffset;
        above[0] = left[0];
        for (int i = 1; i < tuSize2; i++)
            above[i] = (src[i - 1] + 2 * src[i] + src[i + 1] + 2) >> 2;
        above[tuSize2] = src[tuSize2];
    }
    else
    {
        above = buf1 + bufOffset;
        memcpy(above, src, (tuSize2 + 1) * sizeof(pixel));
    }

    int sizeIdx = log2TrSizeC - 2;
    X265_CHECK(sizeIdx >= 0 && sizeIdx < 4, "intra block size is out of range\n");
    primitives.intra_pred[dirMode][sizeIdx](dst, stride, left, above, dirMode, 0);
}

void Predict::prepMotionCompensation(TComDataCU* cu, CU* cuData, int partIdx)
{
    m_slice = cu->m_slice;
    cu->getPartIndexAndSize(partIdx, m_partAddr, m_width, m_height);
    m_cuAddr = cu->getAddr();
    m_zOrderIdxinCU = cuData->encodeIdx;

    m_mvField[0] = cu->getCUMvField(REF_PIC_LIST_0);
    m_mvField[1] = cu->getCUMvField(REF_PIC_LIST_1);

    m_clippedMv[0] = m_mvField[0]->getMv(m_partAddr);
    m_clippedMv[1] = m_mvField[1]->getMv(m_partAddr);
    cu->clipMv(m_clippedMv[0]);
    cu->clipMv(m_clippedMv[1]);
}

void Predict::motionCompensation(TComYuv* predYuv, bool bLuma, bool bChroma)
{
    if (m_slice->isInterP())
    {
        /* P Slice */

        WeightValues wv0[3];
        int refIdx0 = m_mvField[0]->getRefIdx(m_partAddr);
        X265_CHECK(refIdx0 >= 0, "invalid P refidx\n");
        X265_CHECK(refIdx0 < m_slice->m_numRefIdx[0], "P refidx out of range\n");
        WeightParam *wp0 = m_slice->m_weightPredTable[0][refIdx0];

        if (m_slice->m_pps->bUseWeightPred && wp0->bPresentFlag)
        {
            for (int plane = 0; plane < 3; plane++)
            {
                wv0[plane].w      = wp0[plane].inputWeight;
                wv0[plane].offset = wp0[plane].inputOffset * (1 << (X265_DEPTH - 8));
                wv0[plane].shift  = wp0[plane].log2WeightDenom;
                wv0[plane].round  = wp0[plane].log2WeightDenom >= 1 ? 1 << (wp0[plane].log2WeightDenom - 1) : 0;
            }

            ShortYuv* shortYuv = &m_predShortYuv[0];

            if (bLuma)
                predInterLumaBlk(m_slice->m_refPicList[0][refIdx0]->getPicYuvRec(), shortYuv, &m_clippedMv[0]);
            if (bChroma)
                predInterChromaBlk(m_slice->m_refPicList[0][refIdx0]->getPicYuvRec(), shortYuv, &m_clippedMv[0]);

            addWeightUni(shortYuv, wv0, predYuv, bLuma, bChroma);
        }
        else
        {
            if (bLuma)
                predInterLumaBlk(m_slice->m_refPicList[0][refIdx0]->getPicYuvRec(), predYuv, &m_clippedMv[0]);
            if (bChroma)
                predInterChromaBlk(m_slice->m_refPicList[0][refIdx0]->getPicYuvRec(), predYuv, &m_clippedMv[0]);
        }
    }
    else
    {
        /* B Slice */

        WeightValues wv0[3], wv1[3];
        WeightParam *pwp0, *pwp1;

        int refIdx0 = m_mvField[0]->getRefIdx(m_partAddr);
        int refIdx1 = m_mvField[1]->getRefIdx(m_partAddr);

        if (m_slice->m_pps->bUseWeightedBiPred)
        {
            pwp0 = refIdx0 >= 0 ? m_slice->m_weightPredTable[0][refIdx0] : NULL;
            pwp1 = refIdx1 >= 0 ? m_slice->m_weightPredTable[1][refIdx1] : NULL;

            if (pwp0 && pwp1 && (pwp0->bPresentFlag || pwp1->bPresentFlag))
            {
                /* biprediction weighting */
                for (int plane = 0; plane < 3; plane++)
                {
                    wv0[plane].w = pwp0[plane].inputWeight;
                    wv0[plane].o = pwp0[plane].inputOffset * (1 << (X265_DEPTH - 8));
                    wv0[plane].shift = pwp0[plane].log2WeightDenom;
                    wv0[plane].round = 1 << pwp0[plane].log2WeightDenom;

                    wv1[plane].w = pwp1[plane].inputWeight;
                    wv1[plane].o = pwp1[plane].inputOffset * (1 << (X265_DEPTH - 8));
                    wv1[plane].shift = wv0[plane].shift;
                    wv1[plane].round = wv0[plane].round;
                }
            }
            else
            {
                /* uniprediction weighting, always outputs to wv0 */
                WeightParam* pwp = (refIdx0 >= 0) ? pwp0 : pwp1;
                for (int plane = 0; plane < 3; plane++)
                {
                    wv0[plane].w = pwp[plane].inputWeight;
                    wv0[plane].offset = pwp[plane].inputOffset * (1 << (X265_DEPTH - 8));
                    wv0[plane].shift = pwp[plane].log2WeightDenom;
                    wv0[plane].round = pwp[plane].log2WeightDenom >= 1 ? 1 << (pwp[plane].log2WeightDenom - 1) : 0;
                }
            }
        }
        else
            pwp0 = pwp1 = NULL;

        if (refIdx0 >= 0 && refIdx1 >= 0)
        {
            /* Biprediction */
            X265_CHECK(refIdx0 < m_slice->m_numRefIdx[0], "bidir refidx0 out of range\n");
            X265_CHECK(refIdx1 < m_slice->m_numRefIdx[1], "bidir refidx1 out of range\n");

            if (bLuma)
            {
                predInterLumaBlk(m_slice->m_refPicList[0][refIdx0]->getPicYuvRec(), &m_predShortYuv[0], &m_clippedMv[0]);
                predInterLumaBlk(m_slice->m_refPicList[1][refIdx1]->getPicYuvRec(), &m_predShortYuv[1], &m_clippedMv[1]);
            }
            if (bChroma)
            {
                predInterChromaBlk(m_slice->m_refPicList[0][refIdx0]->getPicYuvRec(), &m_predShortYuv[0], &m_clippedMv[0]);
                predInterChromaBlk(m_slice->m_refPicList[1][refIdx1]->getPicYuvRec(), &m_predShortYuv[1], &m_clippedMv[1]);
            }

            if (pwp0 && pwp1 && (pwp0->bPresentFlag || pwp1->bPresentFlag))
                addWeightBi(&m_predShortYuv[0], &m_predShortYuv[1], wv0, wv1, predYuv, bLuma, bChroma);
            else
                predYuv->addAvg(&m_predShortYuv[0], &m_predShortYuv[1], m_partAddr, m_width, m_height, bLuma, bChroma);
        }
        else if (refIdx0 >= 0)
        {
            /* uniprediction to L0 */
            X265_CHECK(refIdx0 < m_slice->m_numRefIdx[0], "unidir refidx0 out of range\n");

            if (pwp0 && pwp0->bPresentFlag)
            {
                ShortYuv* shortYuv = &m_predShortYuv[0];

                if (bLuma)
                    predInterLumaBlk(m_slice->m_refPicList[0][refIdx0]->getPicYuvRec(), shortYuv, &m_clippedMv[0]);
                if (bChroma)
                    predInterChromaBlk(m_slice->m_refPicList[0][refIdx0]->getPicYuvRec(), shortYuv, &m_clippedMv[0]);

                addWeightUni(shortYuv, wv0, predYuv, bLuma, bChroma);
            }
            else
            {
                if (bLuma)
                    predInterLumaBlk(m_slice->m_refPicList[0][refIdx0]->getPicYuvRec(), predYuv, &m_clippedMv[0]);
                if (bChroma)
                    predInterChromaBlk(m_slice->m_refPicList[0][refIdx0]->getPicYuvRec(), predYuv, &m_clippedMv[0]);
            }
        }
        else
        {
            /* uniprediction to L1 */
            X265_CHECK(refIdx1 >= 0, "refidx1 was not positive\n");
            X265_CHECK(refIdx1 < m_slice->m_numRefIdx[1], "unidir refidx1 out of range\n");

            if (pwp1 && pwp1->bPresentFlag)
            {
                ShortYuv* shortYuv = &m_predShortYuv[0];

                if (bLuma)
                    predInterLumaBlk(m_slice->m_refPicList[1][refIdx1]->getPicYuvRec(), shortYuv, &m_clippedMv[1]);
                if (bChroma)
                    predInterChromaBlk(m_slice->m_refPicList[1][refIdx1]->getPicYuvRec(), shortYuv, &m_clippedMv[1]);

                addWeightUni(shortYuv, wv0, predYuv, bLuma, bChroma);
            }
            else
            {
                if (bLuma)
                    predInterLumaBlk(m_slice->m_refPicList[1][refIdx1]->getPicYuvRec(), predYuv, &m_clippedMv[1]);
                if (bChroma)
                    predInterChromaBlk(m_slice->m_refPicList[1][refIdx1]->getPicYuvRec(), predYuv, &m_clippedMv[1]);
            }
        }
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
void Predict::addWeightBi(ShortYuv* srcYuv0, ShortYuv* srcYuv1, const WeightValues wp0[3], const WeightValues wp1[3], TComYuv* predYuv, bool bLuma, bool bChroma)
{
    int x, y;

    int w0, w1, offset, shiftNum, shift, round;
    uint32_t src0Stride, src1Stride, dststride;

    int16_t* srcY0 = srcYuv0->getLumaAddr(m_partAddr);
    int16_t* srcU0 = srcYuv0->getCbAddr(m_partAddr);
    int16_t* srcV0 = srcYuv0->getCrAddr(m_partAddr);

    int16_t* srcY1 = srcYuv1->getLumaAddr(m_partAddr);
    int16_t* srcU1 = srcYuv1->getCbAddr(m_partAddr);
    int16_t* srcV1 = srcYuv1->getCrAddr(m_partAddr);

    pixel* dstY    = predYuv->getLumaAddr(m_partAddr);
    pixel* dstU    = predYuv->getCbAddr(m_partAddr);
    pixel* dstV    = predYuv->getCrAddr(m_partAddr);

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
        dststride  = predYuv->getStride();

        // TODO: can we use weight_sp here?
        for (y = m_height - 1; y >= 0; y--)
        {
            for (x = m_width - 1; x >= 0; )
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
        dststride  = predYuv->getCStride();

        m_width  >>= srcYuv0->getHorzChromaShift();
        m_height >>= srcYuv0->getVertChromaShift();

        // TODO: can we use weight_sp here?
        for (y = m_height - 1; y >= 0; y--)
        {
            for (x = m_width - 1; x >= 0; )
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

        for (y = m_height - 1; y >= 0; y--)
        {
            for (x = m_width - 1; x >= 0; )
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
void Predict::addWeightUni(ShortYuv* srcYuv0, const WeightValues wp[3], TComYuv* predYuv, bool bLuma, bool bChroma)
{
    int16_t* srcY0 = srcYuv0->getLumaAddr(m_partAddr);
    int16_t* srcU0 = srcYuv0->getCbAddr(m_partAddr);
    int16_t* srcV0 = srcYuv0->getCrAddr(m_partAddr);

    pixel* dstY = predYuv->getLumaAddr(m_partAddr);
    pixel* dstU = predYuv->getCbAddr(m_partAddr);
    pixel* dstV = predYuv->getCrAddr(m_partAddr);

    int w0, offset, shiftNum, shift, round;
    uint32_t srcStride, dstStride;

    if (bLuma)
    {
        // Luma
        w0      = wp[0].w;
        offset  = wp[0].offset;
        shiftNum = IF_INTERNAL_PREC - X265_DEPTH;
        shift   = wp[0].shift + shiftNum;
        round   = shift ? (1 << (shift - 1)) : 0;
        srcStride = srcYuv0->m_width;
        dstStride  = predYuv->getStride();

        primitives.weight_sp(srcY0, dstY, srcStride, dstStride, m_width, m_height, w0, round, shift, offset);
    }

    if (bChroma)
    {
        // Chroma U
        w0      = wp[1].w;
        offset  = wp[1].offset;
        shiftNum = IF_INTERNAL_PREC - X265_DEPTH;
        shift   = wp[1].shift + shiftNum;
        round   = shift ? (1 << (shift - 1)) : 0;

        srcStride = srcYuv0->m_cwidth;
        dstStride = predYuv->getCStride();

        m_width  >>= srcYuv0->getHorzChromaShift();
        m_height >>= srcYuv0->getVertChromaShift();

        primitives.weight_sp(srcU0, dstU, srcStride, dstStride, m_width, m_height, w0, round, shift, offset);

        // Chroma V
        w0     = wp[2].w;
        offset = wp[2].offset;
        shift  = wp[2].shift + shiftNum;
        round  = shift ? (1 << (shift - 1)) : 0;

        primitives.weight_sp(srcV0, dstV, srcStride, dstStride, m_width, m_height, w0, round, shift, offset);
    }
}
