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
                    uint32_t cxWidth = m_blockSize;
                    uint32_t cxHeight = m_blockSize;
                    int extStride = cxWidth;
                    int filterSize = NTAPS_CHROMA;
                    int halfFilterSize = (filterSize >> 1);

                    primitives.chroma[m_csp].filter_hps[partEnum](temp, strd, immedVal, extStride, xFrac, 1);
                    primitives.chroma_vsp(immedVal + (halfFilterSize - 1) * extStride, extStride, m_buf + pixoff, m_refStride, cxWidth, cxHeight, yFrac);
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
    wpScalingParam w, *fw;
    Lowres *fenc, *ref;
    int numPredDir = m_slice->isInterP() ? 1 : 2;
    int curPoc, refPoc, difPoc;
    int check;
    int fullCheck = 0;
    int lumaDenom = 0;
    int numWeighted = 0;     // number of weighted references for each m_slice must be less than 8 as per HEVC standard
    int width[3], height[3];

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
            m_mvs = fenc->lowresMvs[list][difPoc - 1];
            if (m_mvs) m_mvCost = fenc->lowresMvCosts[0][difPoc - 1];
            const float epsilon = 1.f / 128.f;
            float guessScale[3], fencMean[3], refMean[3];

            for (int yuv = 0; yuv < 3; yuv++)
            {
                float fencVar = (float)fenc->wp_ssd[yuv] + !ref->wp_ssd[yuv];
                float refVar  = (float)ref->wp_ssd[yuv] + !ref->wp_ssd[yuv];
                guessScale[yuv] = sqrtf((float)fencVar / refVar);
                fencMean[yuv] = (float)fenc->wp_sum[yuv] / (height[yuv] * width[yuv]) / (1 << (X265_DEPTH - 8));
                refMean[yuv]  = (float)ref->wp_sum[yuv] / (height[yuv] * width[yuv]) / (1 << (X265_DEPTH - 8));
            }

            for (int yuv = 0; yuv < 3; yuv++)
            {
                int ic = 0;
                SET_WEIGHT(w, 0, 1, 0, 0);
                /* Early termination */
                if (fabsf(refMean[yuv] - fencMean[yuv]) < 0.5f && fabsf(1.f - guessScale[yuv]) < epsilon)
                    continue;

                int chromaDenom = 7;
                if (yuv)
                {
                    while (chromaDenom > lumaDenom)
                    {
                        float thresh = 127.f / (1 << chromaDenom);
                        if (guessScale[1] < thresh && guessScale[2] < thresh)
                            break;
                        chromaDenom--;
                    }
                }

                /* Don't check chroma in lookahead, or if there wasn't a luma weight. */
                int minoff = 0, minscale, mindenom;
                unsigned int minscore = 0, origscore = 1;
                int found = 0;

                if (yuv)
                {
                    w.log2WeightDenom = chromaDenom;
                    w.inputWeight = Clip3(0, 255, (int)guessScale[yuv] * (1 << w.log2WeightDenom));
                    if (w.inputWeight > 127)
                    {
                        SET_WEIGHT(fw[1], 0, 64, 6, 0);
                        SET_WEIGHT(fw[2], 0, 64, 6, 0);
                        break;
                    }
                }
                else
                    w.setFromWeightAndOffset((int)(guessScale[yuv] * 128 + 0.5), 0);

                if (!yuv) lumaDenom = w.log2WeightDenom;
                mindenom = w.log2WeightDenom;
                minscale = w.inputWeight;

                switch (yuv)
                {
                case 0:

                    m_mcbuf = ref->fpelPlane;
                    m_inbuf = fenc->lowresPlane[0];
                    if (m_mvs)
                    {
                        pixel *tempm_buf;
                        pixel m_buf8[8 * 8];
                        int pixoff = 0, cu = 0;
                        intptr_t strd;
                        for (int y = 0; y < m_frmHeight; y += 8, pixoff = y * m_refStride)
                        {
                            for (int x = 0; x < m_frmWidth; x += 8, pixoff += 8, cu++)
                            {
                                if (fenc->lowresMvCosts[0][difPoc - 1][cu] > fenc->intraCost[cu])
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
                    }
                    break;

                case 1:

                    m_mcbuf = m_slice->getRefPic(list, refIdxTemp)->getPicYuvOrg()->getCbAddr();
                    m_inbuf = m_slice->getPic()->getPicYuvOrg()->getCbAddr();
                    m_blockSize = 8;
                    if (m_mvs) mcChroma();
                    break;

                case 2:

                    m_mcbuf = m_slice->getRefPic(list, refIdxTemp)->getPicYuvOrg()->getCrAddr();
                    m_inbuf = m_slice->getPic()->getPicYuvOrg()->getCrAddr();
                    m_blockSize = 8;
                    if (m_mvs) mcChroma();
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

                if (!found || (minscale == 1 << mindenom && minoff == 0) || (float)minscore / origscore > 0.998f)
                    continue;
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
                numWeighted++;
                if (fw[0].log2WeightDenom == 7)
                {
                    fw[0].inputWeight >>= 1;
                    fw[0].log2WeightDenom--;
                }

                int maxdenom = X265_MAX(fw[0].log2WeightDenom, X265_MAX(fw[1].log2WeightDenom, fw[2].log2WeightDenom));
                fw[0].inputWeight <<= (maxdenom - fw[0].log2WeightDenom);
                fw[0].log2WeightDenom += (maxdenom - fw[0].log2WeightDenom);
                fw[1].inputWeight <<= (maxdenom - fw[1].log2WeightDenom);
                fw[1].log2WeightDenom += (maxdenom - fw[1].log2WeightDenom);
                fw[2].inputWeight <<= (maxdenom - fw[2].log2WeightDenom);
                fw[2].log2WeightDenom += (maxdenom - fw[2].log2WeightDenom);
                fw[1].bPresentFlag = true;
                fw[2].bPresentFlag = true;

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
                    SET_WEIGHT(fw[0], 0, 64, 6, 0);
                    SET_WEIGHT(fw[1], 0, 64, 6, 0);
                    SET_WEIGHT(fw[2], 0, 64, 6, 0);
                    fullCheck = 0;
                }
            }
        }
    }

    m_slice->setWpScaling(m_wp);
    m_slice->getPPS()->setUseWP((fullCheck > 0) ? true : false);
}
