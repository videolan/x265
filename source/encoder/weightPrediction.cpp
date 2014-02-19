/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Author: Shazeb Nawaz Khan <shazeb@multicorewareinc.com>
 *         Steve Borho <steve@borho.org>
 *         Kavitha Sampas <kavitha@multicorewareinc.com>
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
#include <cmath>

using namespace x265;

namespace weightp {
/* make a motion compensated copy of lowres ref into mcout with the same stride.
 * The borders of mcout are not extended */
void mcLuma(pixel *    mcout,
            Lowres&    ref,
            const int *mvCost,
            const int *intraCost,
            const MV * mvs)
{
    pixel *src = ref.lowresPlane[0];
    int stride = ref.lumaStride;
    const int cuSize = 8;
    MV mvmin, mvmax;

    int cu = 0;

    for (int y = 0; y < ref.lines; y += cuSize)
    {
        int pixoff = y * stride;
        mvmin.y = (int16_t)((-y - 8) << 2);
        mvmax.y = (int16_t)((ref.lines - y - 1 + 8) << 2);

        for (int x = 0; x < ref.width; x += cuSize, pixoff += cuSize, cu++)
        {
            if (mvCost[cu] > intraCost[cu])
            {
                // ignore MV when intra cost was less than inter
                primitives.luma_copy_pp[LUMA_8x8](mcout + pixoff, stride, src + pixoff, stride);
            }
            else
            {
                ALIGN_VAR_16(pixel, buf8x8[8 * 8]);
                intptr_t bstride = 8;
                mvmin.x = (int16_t)((-x - 8) << 2);
                mvmax.x = (int16_t)((ref.width - x - 1 + 8) << 2);

                /* clip MV to available pixels */
                MV mv = mvs[cu];
                mv = mv.clipped(mvmin, mvmax);
                pixel *tmp = ref.lowresMC(pixoff, mv, buf8x8, bstride);
                primitives.luma_copy_pp[LUMA_8x8](mcout + pixoff, stride, tmp, bstride);
            }
        }
    }

    x265_emms();
}

/* use lowres MVs from lookahead to generate a motion compensated chroma plane.
 * if a block had cheaper lowres cost as intra, we treat it as MV 0 */
void mcChroma(pixel *    mcout,
              pixel *    src,
              Lowres&    fenc,
              int        stride,
              const int *mvCost,
              const int *intraCost,
              const MV * mvs,
              int        height,
              int        width,
              int        csp)
{
    /* the motion vectors correspond to 8x8 lowres luma blocks, or 16x16 fullres
     * luma blocks. We have to adapt block size to chroma csp */
    int hShift = CHROMA_H_SHIFT(csp);
    int vShift = CHROMA_V_SHIFT(csp);
    int bw = 16 >> hShift;
    int bh = 16 >> vShift;
    MV mvmin, mvmax;

    int lowresWidthInCU = fenc.width >> 3;
    int lowresHeightInCU = fenc.lines >> 3;

    for (int y = 0; y < height; y += bh)
    {
        /* note: lowres block count per row might be different from chroma block
         * count per row because of rounding issues, so be very careful with indexing
         * into the lowres structures */
        int cu = y * lowresWidthInCU;
        int pixoff = y * stride;
        mvmin.y = (int16_t)((-y - 8) << 2);
        mvmax.y = (int16_t)((height - y - 1 + 8) << 2);

        for (int x = 0; x < width; x += bw, cu++, pixoff += bw)
        {
            if (x < lowresWidthInCU && y < lowresHeightInCU && mvCost[cu] < intraCost[cu])
            {
                MV mv = mvs[cu]; // lowres MV
                mv <<= 1;        // fullres MV
                mv.x >>= hShift;
                mv.y >>= vShift;

                /* clip MV to available pixels */
                mvmin.x = (int16_t)((-x - 8) << 2);
                mvmax.x = (int16_t)((width - x - 1 + 8) << 2);
                mv = mv.clipped(mvmin, mvmax);

                int fpeloffset = (mv.y >> 2) * stride + (mv.x >> 2);
                pixel *temp = src + pixoff + fpeloffset;

                int xFrac = mv.x & 0x7;
                int yFrac = mv.y & 0x7;
                if ((yFrac | xFrac) == 0)
                {
                    primitives.chroma[csp].copy_pp[LUMA_16x16](mcout + pixoff, stride, temp, stride);
                }
                else if (yFrac == 0)
                {
                    primitives.chroma[csp].filter_hpp[LUMA_16x16](temp, stride, mcout + pixoff, stride, xFrac);
                }
                else if (xFrac == 0)
                {
                    primitives.chroma[csp].filter_vpp[LUMA_16x16](temp, stride, mcout + pixoff, stride, yFrac);
                }
                else
                {
                    ALIGN_VAR_16(int16_t, imm[16 * (16 + NTAPS_CHROMA)]);
                    primitives.chroma[csp].filter_hps[LUMA_16x16](temp, stride, imm, bw, xFrac, 1);
                    primitives.chroma[csp].filter_vsp[LUMA_16x16](imm + ((NTAPS_CHROMA >> 1) - 1) * bw, bw, mcout + pixoff, stride, yFrac);
                }
            }
            else
            {
                primitives.chroma[csp].copy_pp[LUMA_16x16](mcout + pixoff, stride, src + pixoff, stride);
            }
        }
    }

    x265_emms();
}

/* Measure sum of 8x8 satd costs between source frame and reference
 * frame (potentially weighted, potentially motion compensated). We
 * always use source images for this analysis since reference recon
 * pixels have unreliable availability */
uint32_t weightCost(pixel *         fenc,
                    int             fencstride,
                    pixel *         ref,
                    int             refstride,
                    pixel *         temp,
                    int             width,
                    int             height,
                    wpScalingParam *w)
{
    if (w)
    {
        /* make a weighted copy of the reference plane */
        int offset = w->inputOffset << (X265_DEPTH - 8);
        int scale = w->inputWeight;
        int denom = w->log2WeightDenom;
        int correction = IF_INTERNAL_PREC - X265_DEPTH;
        int pwidth = ((width + 15) >> 4) << 4;

        // Adding (IF_INTERNAL_PREC - X265_DEPTH) to cancel effect of pixel to short conversion inside the primitive
        primitives.weight_pp(ref, temp, refstride, refstride, pwidth, height,
                             scale, (1 << (denom - 1 + correction)), denom + correction, offset);
        ref = temp;
    }

    uint32_t cost = 0;
    for (int y = 0; y < height; y += 8)
    {
        for (int x = 0; x < width; x += 8)
        {
            // Do not measure cost of border CUs
            if ((x > 0) && (x + 8 < width - 1) && (y > 0) && (y + 8 < height - 1))
            {
                pixel *f = fenc + y * fencstride + x;
                pixel *r = ref  + y * refstride  + x;
                cost += primitives.satd[LUMA_8x8](r, refstride, f, fencstride);
            }
        }
    }

    x265_emms();
    return cost;
}

const float epsilon = 1.f / 128.f;

bool tryCommonDenom(TComSlice&     slice,
                    x265_param&    param,
                    wpScalingParam wp[2][MAX_NUM_REF][3],
                    pixel *        temp,
                    int            indenom)
{
    TComPic *pic = slice.getPic();
    TComPicYuv *picorig = pic->getPicYuvOrg();
    Lowres& fenc = pic->m_lowres;
    int curPoc = slice.getPOC();

    /* caller provides temp space for two full-pel planes. Split it
     * in half for motion compensation of the reference and then the
     * weighting */
    pixel *mcTemp = temp;
    pixel *weightTemp = temp + picorig->getStride() * picorig->getHeight();

    int log2denom[3] = { indenom };
    int csp = picorig->m_picCsp;
    int hshift = CHROMA_H_SHIFT(csp);
    int vshift = CHROMA_V_SHIFT(csp);

    /* Round dimensions to 16, calculate pixel counts in luma and chroma */
    int numpixels[3];
    {
        int w = ((picorig->getWidth()  + 15) >> 4) << 4;
        int h = ((picorig->getHeight() + 15) >> 4) << 4;
        numpixels[0] = w * h;

        w >>= hshift;
        h >>= vshift;
        numpixels[1] = numpixels[2] = w * h;
    }

    int numWeighted = 0;
    int numPredDir = slice.isInterP() ? 1 : 2;

    for (int list = 0; list < numPredDir; list++)
    {
        for (int ref = 0; ref < slice.getNumRefIdx(list); ref++)
        {
            wpScalingParam *fw = wp[list][ref];
            TComPic *refPic = slice.getRefPic(list, ref);
            Lowres& refLowres = refPic->m_lowres;

            MV *mvs = NULL;
            int32_t *mvCosts = NULL;
            bool bWeightRef = false;
            bool bMotionCompensate = false;

            /* test whether POC distance is within range for lookahead structures */
            int diffPoc = abs(curPoc - refPic->getPOC());
            if (diffPoc <= param.bframes + 1)
            {
                mvs = fenc.lowresMvs[list][diffPoc - 1];
                mvCosts = fenc.lowresMvCosts[list][diffPoc - 1];
                /* test whether this motion search was performed by lookahead */
                if (mvs[0].x != 0x7FFF)
                {
                    bMotionCompensate = true;

                    /* reference chroma planes must be extended prior to being
                     * used as motion compensation sources */
                    if (!refPic->m_bChromaPlanesExtended)
                    {
                        refPic->m_bChromaPlanesExtended = true;
                        TComPicYuv *refyuv = refPic->getPicYuvOrg();
                        int stride = refyuv->getCStride();
                        int width = refyuv->getWidth() >> hshift;
                        int height = refyuv->getHeight() >> vshift;
                        int marginX = refyuv->getChromaMarginX();
                        int marginY = refyuv->getChromaMarginY();
                        extendPicBorder(refyuv->getCbAddr(), stride, width, height, marginX, marginY);
                        extendPicBorder(refyuv->getCrAddr(), stride, width, height, marginX, marginY);
                    }
                }
            }

            /* prepare estimates */
            float guessScale[3], fencMean[3], refMean[3];
            for (int yuv = 0; yuv < 3; yuv++)
            {
                uint64_t fencVar = fenc.wp_ssd[yuv] + !refLowres.wp_ssd[yuv];
                uint64_t refVar  = refLowres.wp_ssd[yuv] + !refLowres.wp_ssd[yuv];
                if (fencVar && refVar)
                    guessScale[yuv] = Clip3(-2.f, 1.8f, std::sqrt((float)fencVar / refVar));
                else
                    guessScale[yuv] = 1.8f;

                fencMean[yuv] = (float)fenc.wp_sum[yuv] / (numpixels[yuv]) / (1 << (X265_DEPTH - 8));
                refMean[yuv]  = (float)refLowres.wp_sum[yuv] / (numpixels[yuv]) / (1 << (X265_DEPTH - 8));

                /* Ensure that the denominators of cb and cr are same */
                if (yuv)
                {
                    fw[yuv].setFromWeightAndOffset((int)(guessScale[yuv] * (1 << log2denom[1]) + 0.5), 0, log2denom[1]);
                    log2denom[1] = X265_MIN(log2denom[1], (int)fw[yuv].log2WeightDenom);
                }
            }

            log2denom[2] = log2denom[1];

            for (int yuv = 0; yuv < 3; yuv++)
            {
                fw[yuv].bPresentFlag = false;

                /* Early termination */
                float meanDiff = refMean[yuv] < fencMean[yuv] ? fencMean[yuv] - refMean[yuv] : refMean[yuv] - fencMean[yuv];
                float guessVal = guessScale[yuv] > 1.f ? guessScale[yuv] - 1.f : 1.f - guessScale[yuv];
                if (meanDiff < 0.5f && guessVal < epsilon)
                    continue;

                /* prepare inputs to weight analysis */
                pixel *orig;
                pixel *fref;
                int    origstride, frefstride;
                int    width, height;
                switch (yuv)
                {
                case 0:
                    orig = fenc.lowresPlane[0];
                    fref = refLowres.lowresPlane[0];
                    origstride = frefstride = fenc.lumaStride;
                    width = fenc.width;
                    height = fenc.lines;

                    if (bMotionCompensate)
                    {
                        mcLuma(mcTemp, refLowres, mvCosts, fenc.intraCost, mvs);
                        fref = mcTemp;
                    }
                    break;

                case 1:
                    orig = picorig->getCbAddr();
                    fref = refPic->getPicYuvOrg()->getCbAddr();
                    origstride = frefstride = picorig->getCStride();

                    /* Clamp the chroma dimensions to the nearest multiple of
                     * 8x8 blocks (or 16x16 for 4:4:4) since mcChroma uses lowres
                     * blocks and weightCost measures 8x8 blocks. This
                     * potentially ignores some edge pixels, but simplifies the
                     * logic and prevents reading uninitialized pixels. Lowres
                     * planes are border extended and require no clamping. */
                    width =  ((picorig->getWidth()  >> 4) << 4) >> hshift;
                    height = ((picorig->getHeight() >> 4) << 4) >> vshift;

                    if (bMotionCompensate)
                    {
                        mcChroma(mcTemp, fref, fenc, frefstride, mvCosts, fenc.intraCost, mvs, height, width, csp);
                        fref = mcTemp;
                    }
                    break;

                case 2:
                    fref = refPic->getPicYuvOrg()->getCrAddr();
                    orig = picorig->getCrAddr();
                    origstride = frefstride = picorig->getCStride();
                    width =  ((picorig->getWidth()  >> 4) << 4) >> hshift;
                    height = ((picorig->getHeight() >> 4) << 4) >> vshift;

                    if (bMotionCompensate)
                    {
                        mcChroma(mcTemp, fref, fenc, frefstride, mvCosts, fenc.intraCost, mvs, height, width, csp);
                        fref = mcTemp;
                    }
                    break;

                default:
                    // idiotic compilers must die
                    return false;
                }

                wpScalingParam w;
                w.setFromWeightAndOffset((int)(guessScale[yuv] * (1 << log2denom[yuv]) + 0.5), 0, log2denom[yuv]);
                int mindenom = w.log2WeightDenom;
                int minscale = w.inputWeight;
                int minoff = 0;

                uint32_t origscore = weightCost(orig, origstride, fref, frefstride, weightTemp, width, height, NULL);
                if (!origscore)
                    continue;

                uint32_t minscore = origscore;
                bool bFound = false;
                static const int sD = 4; // scale distance
                static const int oD = 2; // offset distance
                for (int is = minscale - sD; is <= minscale + sD; is++)
                {
                    int deltaWeight = is - (1 << mindenom);
                    if (deltaWeight > 127 || deltaWeight <= -128)
                        continue;

                    int curScale = is;
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
                            int deltaOffset = ioff - pred; // signed 10bit
                            if (deltaOffset < -512 || deltaOffset > 511)
                                continue;
                            ioff = Clip3(-128, 127, (deltaOffset + pred)); // signed 8bit
                        }
                        else
                        {
                            ioff = Clip3(-128, 127, ioff);
                        }

                        SET_WEIGHT(w, true, curScale, mindenom, ioff);
                        uint32_t s = weightCost(orig, origstride, fref, frefstride, weightTemp, width, height, &w);
                        COPY4_IF_LT(minscore, s, minscale, curScale, minoff, ioff, bFound, true);
                        if (minoff == curOffset - oD && ioff != curOffset - oD)
                            break;
                    }
                }

                // if chroma denoms diverged, we must start over
                if (mindenom < log2denom[yuv])
                    return false;

                if (!bFound || (minscale == (1 << mindenom) && minoff == 0) || (float)minscore / origscore > 0.998f)
                {
                    fw[yuv].bPresentFlag = false;
                    fw[yuv].inputWeight = 1 << fw[yuv].log2WeightDenom;
                }
                else
                {
                    SET_WEIGHT(fw[yuv], true, minscale, mindenom, minoff);
                    bWeightRef = true;
                }
            }

            if (bWeightRef)
            {
                // Make sure both chroma channels match
                if (fw[1].bPresentFlag != fw[2].bPresentFlag)
                {
                    if (fw[1].bPresentFlag)
                        fw[2] = fw[1];
                    else
                        fw[1] = fw[2];
                }

                if (++numWeighted >= 8)
                    return true;
            }
        }
    }

    return true;
}
}

namespace x265 {
void weightAnalyse(TComSlice& slice, x265_param& param)
{
    wpScalingParam wp[2][MAX_NUM_REF][3];
    int numPredDir = slice.isInterP() ? 1 : 2;

    /* TODO: perf - collect some of this data into a struct which is passed to
     * tryCommonDenom() to avoid recalculating some data.  Motion compensated
     * reference planes can be cached this way */

    TComPicYuv *orig = slice.getPic()->getPicYuvOrg();
    pixel *temp = X265_MALLOC(pixel, 2 * orig->getStride() * orig->getHeight());
    bool useWp = false;

    if (temp)
    {
        int denom = slice.getNumRefIdx(REF_PIC_LIST_0) > 3 ? 7 : 6;
        do
        {
            /* reset weight states */
            for (int list = 0; list < numPredDir; list++)
            {
                for (int ref = 0; ref < slice.getNumRefIdx(list); ref++)
                {
                    SET_WEIGHT(wp[list][ref][0], false, 1 << denom, denom, 0);
                    SET_WEIGHT(wp[list][ref][1], false, 1 << denom, denom, 0);
                    SET_WEIGHT(wp[list][ref][2], false, 1 << denom, denom, 0);
                }
            }

            if (weightp::tryCommonDenom(slice, param, wp, temp, denom))
            {
                useWp = true;
                break;
            }
            denom--; // decrement to satisfy the range limitation
        }
        while (denom > 0);

        X265_FREE(temp);
    }

    if (useWp && param.logLevel >= X265_LOG_DEBUG)
    {
        char buf[1024];
        int p = 0;

        p = sprintf(buf, "poc: %d weights:", slice.getPOC());
        for (int list = 0; list < numPredDir; list++)
        {
            for (int ref = 0; ref < slice.getNumRefIdx(list); ref++)
            {
                p += sprintf(buf + p, " [L%d:R%d ", list, ref);
                if (wp[list][ref][0].bPresentFlag)
                    p += sprintf(buf + p, "Y{%d*x>>%d%+d}", wp[list][ref][0].inputWeight, wp[list][ref][0].log2WeightDenom, wp[list][ref][0].inputOffset);
                if (wp[list][ref][1].bPresentFlag)
                    p += sprintf(buf + p, "U{%d*x>>%d%+d}", wp[list][ref][1].inputWeight, wp[list][ref][1].log2WeightDenom, wp[list][ref][1].inputOffset);
                if (wp[list][ref][2].bPresentFlag)
                    p += sprintf(buf + p, "V{%d*x>>%d%+d}", wp[list][ref][2].inputWeight, wp[list][ref][2].log2WeightDenom, wp[list][ref][2].inputOffset);
                p += sprintf(buf + p, "]");
            }
        }
        if (p < 80) // pad with spaces to ensure progress line overwritten
            sprintf(buf + p, "%*s", 80-p, " ");
        x265_log(&param, X265_LOG_DEBUG, "%s\n", buf);
    }
    slice.setWpScaling(wp);
}
}
