/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Author: Shazeb Nawaz Khan <shazeb@multicorewareinc.com>
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
 * For more information, contact us at licensing@multicorewareinc.com.
 *****************************************************************************/

#include "TLibCommon/TComPic.h"
#include "lowres.h"
#include "mv.h"
#include "slicetype.h"
#include "weightPrediction.h"

using namespace x265;

/** clip a, such that minVal <= a <= maxVal */
//template<typename T>
//inline T Clip3(T minVal, T maxVal, T a) { return std::min<T>(std::max<T>(minVal, a), maxVal); } ///< general min/max clip

void WeightPrediction::mcChroma()
{
    intptr_t strd = m_refStride;
    int pixoff = 0;
    int cu = 0;
    int partEnum = CHROMA_8x8;
    int16_t *immedVal = (int16_t*)X265_MALLOC(int16_t, 64 * (64 + NTAPS_LUMA - 1));
    pixel *temp;

    for (int y = 0; y < m_frmHeight; y += m_blockSize, pixoff = y * m_refStride)
    {
        for (int x = 0; x < m_frmWidth; x += m_blockSize, pixoff += m_blockSize, cu++)
        {
            if (m_mvCost[cu] < m_intraCost[cu])
            {
                MV mv(m_mvs[cu]);
                int refOffset = (mv.x >> (3 - m_csp444)) + (mv.y >> (3 - m_csp444)) * (int)m_refStride;
                temp = m_mcbuf + refOffset + pixoff;

                int xFrac = mv.x & 0x7;
                int yFrac = mv.y & 0x7;

                if ((yFrac | xFrac) == 0)
                {
                    primitives.chroma[m_csp].copy_pp[partEnum](m_buf + pixoff, m_refStride, temp, strd);
                }
                else if (yFrac == 0)
                {
                    primitives.chroma[m_csp].filter_hpp[partEnum](temp, strd, m_buf + pixoff, m_refStride, xFrac);
                }
                else if (xFrac == 0)
                {
                    primitives.chroma[m_csp].filter_vpp[partEnum](temp, strd, m_buf + pixoff, m_refStride, yFrac);
                }
                else
                {
                    int extStride = m_blockSize;
                    int filterSize = NTAPS_CHROMA;
                    int halfFilterSize = (filterSize >> 1);

                    primitives.chroma[m_csp].filter_hps[partEnum](temp, strd, immedVal, extStride, xFrac, 1);
                    primitives.chroma[m_csp].filter_vsp[partEnum](immedVal + (halfFilterSize - 1) * extStride, extStride, m_buf + pixoff, m_refStride, yFrac);
                }
            }
            else
            {
                primitives.chroma[m_csp].copy_pp[partEnum](m_buf + pixoff, m_refStride, m_mcbuf + pixoff, m_refStride);
            }
        }
    }

    X265_FREE(immedVal);
    m_mcbuf = m_buf;
}

uint32_t WeightPrediction::weightCost(pixel *cur, pixel *ref, wpScalingParam *w)
{
    int stride = m_refStride;
    pixel *temp = (pixel*)X265_MALLOC(pixel, m_frmWidth * m_frmHeight);
    bool nonBorderCU;

    if (w)
    {
        int offset = w->inputOffset << (X265_DEPTH - 8);
        int scale = w->inputWeight;
        int denom = w->log2WeightDenom;
        int correction = IF_INTERNAL_PREC - X265_DEPTH;

        // Adding (IF_INTERNAL_PREC - X265_DEPTH) to cancel effect of pixel to short conversion inside the primitive
        primitives.weight_pp(ref, temp, m_refStride, m_dstStride, m_frmWidth, m_frmHeight,
                             scale, (1 << (denom - 1 + correction)), denom + correction, offset);
        ref = temp;
        stride = m_dstStride;
    }

    int32_t cost = 0;
    int pixoff = 0;
    int mb = 0;
    int count = 0;
    for (int y = 0; y < m_frmHeight; y += 8, pixoff = y * m_refStride)
    {
        for (int x = 0; x < m_frmWidth; x += 8, mb++, pixoff += 8)
        {
            nonBorderCU = (x > 0) && (x < m_frmWidth - 8 - 1) && (y > 0) && (y < m_frmHeight - 8 - 1);
            if (nonBorderCU)
            {
                if (m_mvs)
                {
                    if (m_mvCost[mb] < m_intraCost[mb])
                    {
                        int satd = primitives.satd[LUMA_8x8](ref + (stride * y) + x, stride, cur + pixoff, m_refStride);
                        cost += satd;
                        count++;
                    }
                }
                else
                {
                    int satd = primitives.satd[LUMA_8x8](ref + (stride * y) + x, stride, cur + pixoff, m_refStride);
                    cost += satd;
                }
            }
        }
    }

    X265_FREE(temp);
    x265_emms();
    return cost;
}

void WeightPrediction::weightAnalyseEnc()
{
    int denom = 6;
    bool validRangeFlag = false;

    if (m_slice->getNumRefIdx(REF_PIC_LIST_0) > 3)
    {
        denom  = 7;
    }

    do
    {
        validRangeFlag = checkDenom(denom);
        if (!validRangeFlag)
        {
            denom--; // decrement to satisfy the range limitation
        }
    }
    while ((validRangeFlag == false) && (denom > 0));
}

bool WeightPrediction::checkDenom(int denom)
{
    wpScalingParam w, *fw;
    Lowres *fenc, *ref;
    int numPredDir = m_slice->isInterP() ? 1 : 2;
    int curPoc, refPoc, difPoc;
    int check;
    int fullCheck = 0;
    int numWeighted = 0;     // number of weighted references for each m_slice must be less than 8 as per HEVC standard
    int width[3], height[3];
    int log2denom[3] = {denom};

    fenc = &m_slice->getPic()->m_lowres;
    curPoc = m_slice->getPOC();

    // Rounding the width, height to 16
    width[0]  = ((m_slice->getPic()->getPicYuvOrg()->getWidth() + 8) >> 4) << 4;
    height[0] = ((m_slice->getPic()->getPicYuvOrg()->getHeight() + 8) >> 4) << 4;
    width[2] = width[1] = width[0] >> 1;
    height[2] = height[1] = height[0] >> 1;

    for (int list = 0; list < numPredDir; list++)
    {
        for (int refIdxTemp = 0; (refIdxTemp < m_slice->getNumRefIdx(list)) && (numWeighted < 8); refIdxTemp++)
        {
            check = 0;
            fw = m_wp[list][refIdxTemp];
            ref  = &m_slice->getRefPic(list, refIdxTemp)->m_lowres;
            refPoc = m_slice->getRefPic(list, refIdxTemp)->getPOC();
            difPoc = abs(curPoc - refPoc);
            if (difPoc > m_bframes + 1)
                continue;
            else
            {
                m_mvs = fenc->lowresMvs[list][difPoc - 1];
                if (m_mvs[0].x == 0x7FFF)
                    continue;
                else
                    m_mvCost = fenc->lowresMvCosts[0][difPoc - 1];
            }
            const float epsilon = 1.f / 128.f;
            float guessScale[3], fencMean[3], refMean[3];

            for (int yuv = 0; yuv < 3; yuv++)
            {
                float fencVar = (float)fenc->wp_ssd[yuv] + !ref->wp_ssd[yuv];
                float refVar  = (float)ref->wp_ssd[yuv] + !ref->wp_ssd[yuv];
                guessScale[yuv] = Clip3(-2.f, 1.8f, sqrtf((float)fencVar / refVar));
                fencMean[yuv] = (float)fenc->wp_sum[yuv] / (height[yuv] * width[yuv]) / (1 << (X265_DEPTH - 8));
                refMean[yuv]  = (float)ref->wp_sum[yuv] / (height[yuv] * width[yuv]) / (1 << (X265_DEPTH - 8));
                // Ensure that the denominators of cb and cr are same
                if (yuv)
                {
                    fw[yuv].setFromWeightAndOffset((int)(guessScale[yuv] * (1 << log2denom[1]) + 0.5), 0, log2denom[1]);
                    log2denom[1] = X265_MIN(log2denom[1], (int)fw[yuv].log2WeightDenom);
                }
            }

            log2denom[2] = log2denom[1];
            for (int yuv = 0; yuv < 3; yuv++)
            {
                int ic = 0;
                denom = log2denom[yuv];
                SET_WEIGHT(w, 0, 1, 0, 0);
                SET_WEIGHT(fw[yuv], 0, 1 << denom, denom, 0);
                /* Early termination */
                if (fabsf(refMean[yuv] - fencMean[yuv]) < 0.5f && fabsf(1.f - guessScale[yuv]) < epsilon)
                    continue;

                /* Don't check chroma in lookahead, or if there wasn't a luma weight. */
                int minoff = 0, minscale, mindenom;
                unsigned int minscore = 0, origscore = 1;
                int found = 0;

                w.setFromWeightAndOffset((int)(guessScale[yuv] * (1 << denom) + 0.5), 0, denom);

                mindenom = w.log2WeightDenom;
                minscale = w.inputWeight;

                switch (yuv)
                {
                case 0:
                {
                    m_mcbuf = ref->fpelPlane;
                    m_inbuf = fenc->lowresPlane[0];
                    pixel *tempm_buf;
                    pixel m_buf8[8 * 8];
                    int pixoff = 0, cu = 0;
                    intptr_t strd;
                    for (int y = 0; y < m_frmHeight; y += 8, pixoff = y * m_refStride)
                    {
                        for (int x = 0; x < m_frmWidth; x += 8, pixoff += 8, cu++)
                        {
                            if (m_mvCost[cu] > fenc->intraCost[cu])
                            {
                                strd = m_refStride;
                                tempm_buf = m_inbuf + pixoff;
                            }
                            else
                            {
                                strd = 8;
                                tempm_buf = ref->lowresMC(pixoff, m_mvs[cu], m_buf8, strd);
                                ic++;
                            }
                            primitives.blockcpy_pp(8, 8, m_buf + (y * m_refStride) + x, m_refStride, tempm_buf, strd);
                        }
                    }

                    m_mcbuf = m_buf;
                    break;
                }

                case 1:

                    m_mcbuf = m_slice->getRefPic(list, refIdxTemp)->getPicYuvOrg()->getCbAddr();
                    m_inbuf = m_slice->getPic()->getPicYuvOrg()->getCbAddr();
                    m_blockSize = 8;
                    mcChroma();
                    break;

                case 2:

                    m_mcbuf = m_slice->getRefPic(list, refIdxTemp)->getPicYuvOrg()->getCrAddr();
                    m_inbuf = m_slice->getPic()->getPicYuvOrg()->getCrAddr();
                    m_blockSize = 8;
                    mcChroma();
                    break;
                }

                origscore = minscore = weightCost(m_inbuf, m_mcbuf, NULL);

                if (!minscore)
                    continue;

                int sD = 4;
                int oD = 2;
                unsigned int s = 0;

                for (int is = minscale - sD; is <= minscale + sD; is++)
                {
                    int deltaWeight = minscale - (1 << mindenom);
                    if (deltaWeight > 127 || deltaWeight <= -128)
                        continue;

                    int curScale = minscale;
                    int curOffset = (int)(fencMean[yuv] - refMean[yuv] * curScale / (1 << mindenom) + 0.5f);
                    if (curOffset < -128 || curOffset > 127)
                    {
                        /* Rescale considering the constraints on curOffset. We do it in this order
                         * because scale has a much wider range than offset (because of denom), so
                         * it should almost never need to be clamped. */
                        curOffset = Clip3(-128, 127, curOffset);
                        curScale = (int)((1 << mindenom) * (fencMean[yuv] - curOffset) / refMean[yuv] + 0.5f);
                        curScale = Clip3(0, 127, curScale);
                    }

                    for (int ioff = curOffset - oD; (ioff <= (curOffset + oD)) && (ioff < 127); ioff++)
                    {
                        if (yuv)
                        {
                            int pred = (128 - ((128 * curScale) >> (mindenom)));
                            int deltaOffset = Clip3(-512, 511, (ioff - pred)); // signed 10bit
                            ioff = Clip3(-128, 127, (deltaOffset + pred)); // signed 8bit
                        }
                        else
                        {
                            ioff = Clip3(-128, 127, ioff);
                        }

                        s = 0;
                        SET_WEIGHT(w, 1, is, mindenom, ioff);
                        s = weightCost(m_inbuf, m_mcbuf, &w);
                        COPY4_IF_LT(minscore, s, minscale, is, minoff, ioff, found, 1);
                        if (minoff == curOffset - oD && ioff != curOffset - oD)
                            break;
                    }
                }

                if (mindenom < log2denom[yuv])
                    return false;
                log2denom[yuv] = mindenom;
                if (!found || (minscale == 1 << mindenom && minoff == 0) || (float)minscore / origscore > 0.998f)
                {
                    SET_WEIGHT(fw[yuv], 0, 1 << mindenom, mindenom, 0);
                    continue;
                }
                else
                {
                    SET_WEIGHT(w, 1, minscale, mindenom, minoff);
                    SET_WEIGHT(fw[yuv], 1, minscale, mindenom, minoff);
                    check++;
                    fullCheck++;
                }
            }

            if (check)
            {
                if (fw[1].bPresentFlag || fw[2].bPresentFlag)
                {
                    // Enabling in both chroma
                    fw[1].bPresentFlag = true;
                    fw[2].bPresentFlag = true;
                }

                int deltaWeight;
                bool deltaHigh = false;
                for (int i = 0; i < 3; i++)
                {
                    deltaWeight = fw[i].inputWeight - (1 << fw[i].log2WeightDenom);
                    if (deltaWeight > 127 || deltaWeight <= -128)
                        deltaHigh = true;
                }

                if (deltaHigh)
                {
                    // Checking deltaWeight range
                    SET_WEIGHT(fw[0], 0, 1 << denom, denom, 0);
                    SET_WEIGHT(fw[1], 0, 1 << denom, denom, 0);
                    SET_WEIGHT(fw[2], 0, 1 << denom, denom, 0);
                    fullCheck -= check;
                    return false;
                }
            }
        }
    }

    if (!fullCheck)
    {
        m_slice->setWpScaling(m_wp);
        return false;
    }

    m_slice->setWpScaling(m_wp);
    m_slice->getPPS()->setUseWP((fullCheck > 0) ? true : false);
    return true;
}
