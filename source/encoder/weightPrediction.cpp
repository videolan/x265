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
#include "bitstream.h"
#include <cmath>

using namespace x265;
namespace weightp {

struct RefData
{
    pixel *  mcbuf;
    pixel *  fref;
    float    guessScale;
    float    fencMean;
    float    refMean;
    uint32_t unweightedCost;
};

struct ChannelData
{
    pixel* orig;
    int    stride;
    int    width;
    int    height;
};

struct Cache
{
    wpScalingParam wp[2][MAX_NUM_REF][3];
    RefData        ref[2][MAX_NUM_REF][3];
    ChannelData    paramset[3];

    const int *    intraCost;
    pixel*         weightTemp;
    int            numPredDir;
    int            lambda;
    int            csp;
    int            hshift;
    int            vshift;
    int            lowresWidthInCU;
    int            lowresHeightInCU;
};

int sliceHeaderCost(wpScalingParam *w, int lambda, int bChroma)
{
    /* 4 times higher, because chroma is analyzed at full resolution. */
    if (bChroma)
        lambda *= 4;
    int denomCost = bs_size_ue(w[0].log2WeightDenom) * (2 - bChroma);
    return lambda * (10 + denomCost + 2 * (bs_size_se(w[0].inputWeight) + bs_size_se(w[0].inputOffset)));
}

/* make a motion compensated copy of lowres ref into mcout with the same stride.
 * The borders of mcout are not extended */
void mcLuma(pixel* mcout, Lowres& ref, const MV * mvs)
{
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

    x265_emms();
}

/* use lowres MVs from lookahead to generate a motion compensated chroma plane.
 * if a block had cheaper lowres cost as intra, we treat it as MV 0 */
void mcChroma(pixel *      mcout,
              pixel *      src,
              int          stride,
              const MV *   mvs,
              const Cache& cache,
              int          height,
              int          width)
{
    /* the motion vectors correspond to 8x8 lowres luma blocks, or 16x16 fullres
     * luma blocks. We have to adapt block size to chroma csp */
    int csp = cache.csp;
    int bw = 16 >> cache.hshift;
    int bh = 16 >> cache.vshift;
    MV mvmin, mvmax;

    for (int y = 0; y < height; y += bh)
    {
        /* note: lowres block count per row might be different from chroma block
         * count per row because of rounding issues, so be very careful with indexing
         * into the lowres structures */
        int cu = y * cache.lowresWidthInCU;
        int pixoff = y * stride;
        mvmin.y = (int16_t)((-y - 8) << 2);
        mvmax.y = (int16_t)((height - y - 1 + 8) << 2);

        for (int x = 0; x < width; x += bw, cu++, pixoff += bw)
        {
            if (x < cache.lowresWidthInCU && y < cache.lowresHeightInCU)
            {
                MV mv = mvs[cu]; // lowres MV
                mv <<= 1;        // fullres MV
                mv.x >>= cache.hshift;
                mv.y >>= cache.vshift;

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
                    pixel *         ref,
                    int             stride,
                    const Cache &   cache,
                    int             width,
                    int             height,
                    wpScalingParam *w,
                    bool            bLuma)
{
    if (w)
    {
        /* make a weighted copy of the reference plane */
        int offset = w->inputOffset << (X265_DEPTH - 8);
        int weight = w->inputWeight;
        int denom = w->log2WeightDenom;
        int round = denom ? 1 << (denom - 1) : 0;
        int correction = IF_INTERNAL_PREC - X265_DEPTH; /* intermediate interpolation depth */
        int pwidth = ((width + 15) >> 4) << 4;

        primitives.weight_pp(ref, cache.weightTemp, stride, stride, pwidth, height,
                             weight, round << correction, denom + correction, offset);
        ref = cache.weightTemp;
    }

    uint32_t cost = 0;
    pixel *f = fenc, *r = ref;

    if (bLuma)
    {
        int cu = 0;
        for (int y = 8; y < height; y += 8, r += 8 * stride, f += 8 * stride)
        {
            for (int x = 8; x < width; x += 8, cu++)
            {
                int cmp = primitives.satd[LUMA_8x8](r + x, stride, f + x, stride);
                cost += X265_MIN(cmp, cache.intraCost[cu]);
            }
        }
    }
    else if (cache.csp == X265_CSP_I444)
        for (int y = 16; y < height; y += 16, r += 16 * stride, f += 16 * stride)
            for (int x = 16; x < width; x += 16)
                cost += primitives.satd[LUMA_16x16](r + x, stride, f + x, stride);
    else
        for (int y = 8; y < height; y += 8, r += 8 * stride, f += 8 * stride)
            for (int x = 8; x < width; x += 8)
                cost += primitives.satd[LUMA_8x8](r + x, stride, f + x, stride);

    x265_emms();
    return cost;
}

bool tryCommonDenom(TComSlice& slice, Cache& cache, int indenom)
{
    int log2denom[3] = { indenom };
    const float epsilon = 1.f / 128.f;

    /* reset weight states */
    for (int list = 0; list < cache.numPredDir; list++)
    {
        for (int ref = 0; ref < slice.getNumRefIdx(list); ref++)
        {
            SET_WEIGHT(cache.wp[list][ref][0], false, 1 << indenom, indenom, 0);
            SET_WEIGHT(cache.wp[list][ref][1], false, 1 << indenom, indenom, 0);
            SET_WEIGHT(cache.wp[list][ref][2], false, 1 << indenom, indenom, 0);
        }
    }

    int numWeighted = 0;
    for (int list = 0; list < cache.numPredDir; list++)
    {
        for (int ref = 0; ref < slice.getNumRefIdx(list); ref++)
        {
            wpScalingParam *fw = cache.wp[list][ref];

            for (int yuv = 1; yuv < 3; yuv++)
            {
                /* Ensure that the denominators of cb and cr are same */
                RefData *rd = &cache.ref[list][ref][yuv];
                fw[yuv].setFromWeightAndOffset((int)(rd->guessScale * (1 << log2denom[1]) + 0.5), 0, log2denom[1]);
                log2denom[1] = X265_MIN(log2denom[1], (int)fw[yuv].log2WeightDenom);
            }
            log2denom[2] = log2denom[1];

            bool bWeightRef = false;
            for (int yuv = 0; yuv < 3 && (!yuv || fw[0].bPresentFlag); yuv++)
            {
                RefData *rd = &cache.ref[list][ref][yuv];
                ChannelData *p = &cache.paramset[yuv];
                x265_emms();

                /* Early termination */
                float meanDiff = rd->refMean < rd->fencMean ? rd->fencMean - rd->refMean : rd->refMean - rd->fencMean;
                float guessVal = rd->guessScale > 1.f ? rd->guessScale - 1.f : 1.f - rd->guessScale;
                if ((meanDiff < 0.5f && guessVal < epsilon) || !rd->unweightedCost)
                    continue;

                wpScalingParam w;
                w.setFromWeightAndOffset((int)(rd->guessScale * (1 << log2denom[yuv]) + 0.5), 0, log2denom[yuv]);
                int mindenom = w.log2WeightDenom;
                int minscale = w.inputWeight;
                int minoff = 0;

                uint32_t origscore = rd->unweightedCost;
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
                    int curOffset = (int)(rd->fencMean - rd->refMean * curScale / (1 << mindenom) + 0.5f);
                    if (curOffset < -128 || curOffset > 127)
                    {
                        /* Rescale considering the constraints on curOffset. We do it in this order
                         * because scale has a much wider range than offset (because of denom), so
                         * it should almost never need to be clamped. */
                        curOffset = Clip3(-128, 127, curOffset);
                        curScale = (int)((1 << mindenom) * (rd->fencMean - curOffset) / rd->refMean + 0.5f);
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
                        uint32_t s = weightCost(p->orig, rd->fref, p->stride, cache, p->width, p->height, &w, !yuv) +
                                     sliceHeaderCost(&w, cache.lambda, !!yuv);
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

void prepareRef(Cache& cache, TComSlice& slice, x265_param& param)
{
    TComPic *pic = slice.getPic();
    TComPicYuv *picorig = pic->getPicYuvOrg();
    Lowres& fenc = pic->m_lowres;

    cache.weightTemp = X265_MALLOC(pixel, picorig->getStride() * picorig->getHeight());
    cache.lambda = (int) x265_lambda2_non_I[slice.getSliceQp()];
    cache.intraCost = fenc.intraCost;
    cache.lowresWidthInCU = fenc.width >> 3;
    cache.lowresHeightInCU = fenc.lines >> 3;
    cache.csp = picorig->m_picCsp;
    cache.hshift = CHROMA_H_SHIFT(cache.csp);
    cache.vshift = CHROMA_V_SHIFT(cache.csp);

    int curPoc = slice.getPOC();
    int numpixels[3];
    int w = ((picorig->getWidth()  + 15) >> 4) << 4;
    int h = ((picorig->getHeight() + 15) >> 4) << 4;
    numpixels[0] = w * h;

    w >>= cache.hshift;
    h >>= cache.vshift;
    numpixels[1] = numpixels[2] = w * h;

    cache.numPredDir = slice.isInterP() ? 1 : 2;
    for (int list = 0; list < cache.numPredDir; list++)
    {
        for (int ref = 0; ref < slice.getNumRefIdx(list); ref++)
        {
            TComPic *refPic = slice.getRefPic(list, ref);
            Lowres& refLowres = refPic->m_lowres;

            MV *mvs = NULL;
            bool bMotionCompensate = false;

            /* test whether POC distance is within range for lookahead structures */
            int diffPoc = abs(curPoc - refPic->getPOC());
            if (diffPoc <= param.bframes + 1)
            {
                mvs = fenc.lowresMvs[list][diffPoc - 1];
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
                        int width = refyuv->getWidth() >> cache.hshift;
                        int height = refyuv->getHeight() >> cache.vshift;
                        int marginX = refyuv->getChromaMarginX();
                        int marginY = refyuv->getChromaMarginY();
                        extendPicBorder(refyuv->getCbAddr(), stride, width, height, marginX, marginY);
                        extendPicBorder(refyuv->getCrAddr(), stride, width, height, marginX, marginY);
                    }
                }
            }
            for (int yuv = 0; yuv < 3; yuv++)
            {
                /* prepare inputs to weight analysis */
                RefData *rd = &cache.ref[list][ref][yuv];
                ChannelData *p = &cache.paramset[yuv];

                x265_emms();
                uint64_t fencVar = fenc.wp_ssd[yuv] + !refLowres.wp_ssd[yuv];
                uint64_t refVar  = refLowres.wp_ssd[yuv] + !refLowres.wp_ssd[yuv];
                if (fencVar && refVar)
                    rd->guessScale = Clip3(-2.f, 1.8f, std::sqrt((float)fencVar / refVar));
                else
                    rd->guessScale = 1.8f;
                rd->fencMean = (float)fenc.wp_sum[yuv] / (numpixels[yuv]) / (1 << (X265_DEPTH - 8));
                rd->refMean  = (float)refLowres.wp_sum[yuv] / (numpixels[yuv]) / (1 << (X265_DEPTH - 8));

                switch (yuv)
                {
                case 0:
                    p->orig = fenc.lowresPlane[0];
                    p->stride = fenc.lumaStride;
                    p->width = fenc.width;
                    p->height = fenc.lines;
                    rd->fref = refLowres.lowresPlane[0];
                    if (bMotionCompensate)
                    {
                        rd->mcbuf = X265_MALLOC(pixel, p->stride * p->height);
                        if (rd->mcbuf)
                        {
                            mcLuma(rd->mcbuf, refLowres, mvs);
                            rd->fref = rd->mcbuf;
                        }
                    }
                    break;

                case 1:
                    p->orig = picorig->getCbAddr();
                    p->stride = picorig->getCStride();
                    rd->fref = refPic->getPicYuvOrg()->getCbAddr();

                    /* Clamp the chroma dimensions to the nearest multiple of
                     * 8x8 blocks (or 16x16 for 4:4:4) since mcChroma uses lowres
                     * blocks and weightCost measures 8x8 blocks. This
                     * potentially ignores some edge pixels, but simplifies the
                     * logic and prevents reading uninitialized pixels. Lowres
                     * planes are border extended and require no clamping. */
                    p->width =  ((picorig->getWidth()  >> 4) << 4) >> cache.hshift;
                    p->height = ((picorig->getHeight() >> 4) << 4) >> cache.vshift;
                    if (bMotionCompensate)
                    {
                        rd->mcbuf = X265_MALLOC(pixel, p->stride * p->height);
                        if (rd->mcbuf)
                        {
                            mcChroma(rd->mcbuf, rd->fref, p->stride, mvs, cache, p->height, p->width);
                            rd->fref = rd->mcbuf;
                        }
                    }
                    break;

                case 2:
                    rd->fref = refPic->getPicYuvOrg()->getCrAddr();
                    p->orig = picorig->getCrAddr();
                    p->stride = picorig->getCStride();
                    p->width =  ((picorig->getWidth()  >> 4) << 4) >> cache.hshift;
                    p->height = ((picorig->getHeight() >> 4) << 4) >> cache.vshift;
                    if (bMotionCompensate)
                    {
                        rd->mcbuf = X265_MALLOC(pixel, p->stride * p->height);
                        if (rd->mcbuf)
                        {
                            mcChroma(rd->mcbuf, rd->fref, p->stride, mvs, cache, p->height, p->width);
                            rd->fref = rd->mcbuf;
                        }
                    }
                    break;

                default:
                    return;
                }
                rd->unweightedCost = weightCost(p->orig, rd->fref, p->stride, cache, p->width, p->height, NULL, !yuv);
            }
        }
    }
}

void tearDown(Cache& cache, TComSlice& slice)
{
    X265_FREE(cache.weightTemp);
    for (int list = 0; list < cache.numPredDir; list++)
    {
        for (int ref = 0; ref < slice.getNumRefIdx(list); ref++)
        {
            for (int yuv = 0; yuv < 3; yuv++)
            {
                X265_FREE(cache.ref[list][ref][yuv].mcbuf);
            }
        }
    }
}
}

namespace x265 {
void weightAnalyse(TComSlice& slice, x265_param& param)
{
    weightp::Cache cache;
    memset(&cache, 0, sizeof(cache));

    prepareRef(cache, slice, param);
    if (cache.weightTemp)
    {
        int denom = slice.getNumRefIdx(REF_PIC_LIST_0) > 3 ? 7 : 6;
        do
        {
            if (weightp::tryCommonDenom(slice, cache, denom))
                break;
            denom--; // decrement to satisfy the range limitation 
        }
        while (denom > 0);
    }
    else
    {
        for (int list = 0; list < cache.numPredDir; list++)
        {
            for (int ref = 0; ref < slice.getNumRefIdx(list); ref++)
            {
                SET_WEIGHT(cache.wp[list][ref][0], false, 1, 0, 0);
                SET_WEIGHT(cache.wp[list][ref][1], false, 1, 0, 0);
                SET_WEIGHT(cache.wp[list][ref][2], false, 1, 0, 0);
            }
        }
    }
    tearDown(cache, slice);
    slice.setWpScaling(cache.wp);

    if (param.logLevel >= X265_LOG_FULL)
    {
        char buf[1024];
        int p = 0;
        bool bWeighted = false;

        p = sprintf(buf, "poc: %d weights:", slice.getPOC());
        for (int list = 0; list < cache.numPredDir; list++)
        {
            for (int ref = 0; ref < slice.getNumRefIdx(list); ref++)
            {
                wpScalingParam* w = &cache.wp[list][ref][0];
                if (w[0].bPresentFlag || w[1].bPresentFlag || w[2].bPresentFlag)
                {
                    bWeighted = true;
                    p += sprintf(buf + p, " [L%d:R%d ", list, ref);
                    if (w[0].bPresentFlag)
                        p += sprintf(buf + p, "Y{%d/%d%+d}", w[0].inputWeight, 1 << w[0].log2WeightDenom, w[0].inputOffset);
                    if (w[1].bPresentFlag)
                        p += sprintf(buf + p, "U{%d/%d%+d}", w[1].inputWeight, 1 << w[1].log2WeightDenom, w[1].inputOffset);
                    if (w[2].bPresentFlag)
                        p += sprintf(buf + p, "V{%d/%d%+d}", w[2].inputWeight, 1 << w[2].log2WeightDenom, w[2].inputOffset);
                    p += sprintf(buf + p, "]");
                }
            }
        }

        if (bWeighted)
        {
            if (p < 80) // pad with spaces to ensure progress line overwritten
                sprintf(buf + p, "%*s", 80 - p, " ");
            x265_log(&param, X265_LOG_FULL, "%s\n", buf);
        }
    }
}
}
