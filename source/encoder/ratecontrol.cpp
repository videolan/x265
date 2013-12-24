/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Sumalatha Polureddy <sumalatha@multicorewareinc.com>
 *          Aarthi Priya Thirumalai <aarthi@multicorewareinc.com>
 *          Xun Xu, PPLive Corporation <xunxu@pptv.com>
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
#include "TLibEncoder/TEncCfg.h"
#include "encoder.h"
#include "slicetype.h"
#include "ratecontrol.h"
#include <math.h>

using namespace x265;

/* The qscale - qp conversion is specified in the standards.
 * Approx qscale increases by 12%  with every qp increment */
static inline double qScale2qp(double qScale)
{
    return 12.0 + 6.0 * (double)X265_LOG2(qScale / 0.85);
}

static inline double qp2qScale(double qp)
{
    return 0.85 * pow(2.0, (qp - 12.0) / 6.0);
}

/* Compute variance to derive AC energy of each block */
static inline uint32_t acEnergyVar(TComPic *pic, uint64_t sum_ssd, int shift, int i)
{
    uint32_t sum = (uint32_t)sum_ssd;
    uint32_t ssd = (uint32_t)(sum_ssd >> 32);

    pic->m_lowres.wp_sum[i] += sum;
    pic->m_lowres.wp_ssd[i] += ssd;
    return ssd - ((uint64_t)sum * sum >> shift);
}

/* Find the energy of each block in Y/Cb/Cr plane */
static inline uint32_t acEnergyPlane(TComPic *pic, pixel* src, int srcStride, int bChroma)
{
    if (bChroma)
    {
        ALIGN_VAR_8(pixel, pix[8 * 8]);
        primitives.luma_copy_pp[LUMA_8x8](pix, 8, src, srcStride);
        return acEnergyVar(pic, primitives.var[BLOCK_8x8](pix, 8), 6, bChroma);
    }
    else
        return acEnergyVar(pic, primitives.var[BLOCK_16x16](src, srcStride), 8, bChroma);
}

/* Find the total AC energy of each block in all planes */
uint32_t RateControl::acEnergyCu(TComPic* pic, uint32_t block_x, uint32_t block_y)
{
    int stride = pic->getPicYuvOrg()->getStride();
    int cStride = pic->getPicYuvOrg()->getCStride();
    uint32_t blockOffsetLuma = block_x + (block_y * stride);
    uint32_t blockOffsetChroma = (block_x >> 1) + ((block_y >> 1) * cStride);

    uint32_t var;

    var  = acEnergyPlane(pic, pic->getPicYuvOrg()->getLumaAddr() + blockOffsetLuma, stride, 0);
    var += acEnergyPlane(pic, pic->getPicYuvOrg()->getCbAddr() + blockOffsetChroma, cStride, 1);
    var += acEnergyPlane(pic, pic->getPicYuvOrg()->getCrAddr() + blockOffsetChroma, cStride, 2);
    x265_emms();
    return var;
}

void RateControl::calcAdaptiveQuantFrame(TComPic *pic)
{
    /* Actual adaptive quantization */
    int maxCol = pic->getPicYuvOrg()->getWidth();
    int maxRow = pic->getPicYuvOrg()->getHeight();

    for (int y = 0; y < 3; y++)
    {
        pic->m_lowres.wp_ssd[y] = 0;
        pic->m_lowres.wp_sum[y] = 0;
    }

    /* Calculate Qp offset for each 16x16 block in the frame */
    int block_xy = 0;
    int block_x = 0, block_y = 0;
    double strength = 0.f;
    if (cfg->param.rc.aqMode == X265_AQ_NONE || cfg->param.rc.aqStrength == 0)
    {
        /* Need to init it anyways for CU tree */
        int cuWidth = ((maxCol / 2) + X265_LOWRES_CU_SIZE - 1) >> X265_LOWRES_CU_BITS;
        int cuHeight = ((maxRow / 2) + X265_LOWRES_CU_SIZE - 1) >> X265_LOWRES_CU_BITS;
        int cuCount = cuWidth * cuHeight;

        if(cfg->param.rc.aqMode && cfg->param.rc.aqStrength == 0 )
        {
            memset(pic->m_lowres.qpOffset, 0, cuCount * sizeof(double));
            memset(pic->m_lowres.qpAqOffset, 0, cuCount * sizeof(double));
            for (int cuxy = 0; cuxy < cuCount; cuxy++ )
                pic->m_lowres.invQscaleFactor[cuxy] = 256;
        }

         /* Need variance data for weighted prediction */
        if (cfg->param.bEnableWeightedPred)
        {
            for (block_y = 0; block_y < maxRow; block_y += 16)
                for (block_x = 0; block_x < maxCol; block_x += 16)
                    acEnergyCu(pic, block_x, block_y);
        }

    }
    else
    {
        block_xy = 0;
        double avg_adj_pow2 = 0, avg_adj = 0, qp_adj = 0;
        if (cfg->param.rc.aqMode == X265_AQ_AUTO_VARIANCE)
        {
            double bit_depth_correction = pow (1 << (g_bitDepth - 8), 0.5);
            for (block_y = 0; block_y < maxRow; block_y += 16)
            {
                for (block_x = 0; block_x < maxCol; block_x += 16)
                {
                    uint32_t energy = acEnergyCu(pic, block_x, block_y);
                    qp_adj = pow (energy + 1, 0.125);
                    pic->m_lowres.qpOffset[block_xy] = qp_adj;
                    avg_adj += qp_adj;
                    avg_adj_pow2 += qp_adj * qp_adj;
                    block_xy++;
                }
            }
            avg_adj /= ncu;
            avg_adj_pow2 /= ncu;
            strength = cfg->param.rc.aqStrength * avg_adj / bit_depth_correction;
            avg_adj = avg_adj - 0.5f * (avg_adj_pow2 - (14.f * bit_depth_correction)) / avg_adj;
        }
        else
            strength = cfg->param.rc.aqStrength * 1.0397f;
            block_xy = 0; 
            for (block_y = 0; block_y < maxRow; block_y += 16)
            {
                for (block_x = 0; block_x < maxCol; block_x += 16)
                {
                    if(cfg->param.rc.aqMode == X265_AQ_AUTO_VARIANCE)
                    {
                        qp_adj =pic->m_lowres.qpOffset[block_xy];
                        qp_adj = strength * (qp_adj - avg_adj);
                    }
                    else
                    {
                        uint32_t energy = acEnergyCu(pic, block_x, block_y);
                        qp_adj = strength * (X265_LOG2(X265_MAX(energy, 1)) - (14.427f + 2*(g_bitDepth-8)));
                    }
                    pic->m_lowres.qpAqOffset[block_xy] = qp_adj;
                    pic->m_lowres.qpOffset[block_xy] = qp_adj;
                    pic->m_lowres.invQscaleFactor[block_xy] = x265_exp2fix8(qp_adj);
                    block_xy++;
                }
            }
    }

    if (cfg->param.bEnableWeightedPred)
    {
        int hShift = CHROMA_H_SHIFT(cfg->param.internalCsp);
        int vShift = CHROMA_V_SHIFT(cfg->param.internalCsp);
        maxCol = ((maxCol + 8) >> 4) << 4;
        maxRow = ((maxRow + 8) >> 4) << 4;
        int width[3]  = { maxCol, maxCol >> hShift, maxCol >> hShift };
        int height[3] = { maxRow, maxRow >> vShift, maxRow >> vShift };

        for (int i = 0; i < 3; i++)
        {
            uint64_t sum, ssd;
            sum = pic->m_lowres.wp_sum[i];
            ssd = pic->m_lowres.wp_ssd[i];
            pic->m_lowres.wp_ssd[i] = ssd - (sum * sum + (width[i] * height[i]) / 2) / (width[i] * height[i]);
        }
    }
}

RateControl::RateControl(TEncCfg * _cfg)
{
    this->cfg = _cfg;
    ncu = (int)((cfg->param.sourceHeight * cfg->param.sourceWidth) / pow((int)16, 2.0));

    if (cfg->param.rc.cuTree)
    {
        qCompress = 1;
        cfg->param.rc.pbFactor = 1;
    }
    else
        qCompress = cfg->param.rc.qCompress;

    // validate for cfg->param.rc, maybe it is need to add a function like x265_parameters_valiate()
    cfg->param.rc.rfConstant = Clip3((double)-QP_BD_OFFSET, (double)51, cfg->param.rc.rfConstant);
    if (cfg->param.rc.rateControlMode == X265_RC_CRF)
    {
        cfg->param.rc.qp = (int)cfg->param.rc.rfConstant + QP_BD_OFFSET;
        cfg->param.rc.bitrate = 0;

        double baseCplx = ncu * (cfg->param.bframes ? 120 : 80);
        double mbtree_offset = cfg->param.rc.cuTree ? (1.0 - cfg->param.rc.qCompress) * 13.5 : 0;
        rateFactorConstant = pow(baseCplx, 1 - qCompress) /
            qp2qScale(cfg->param.rc.rfConstant + mbtree_offset + QP_BD_OFFSET);
    }

    isAbr = cfg->param.rc.rateControlMode != X265_RC_CQP; // later add 2pass option

    bitrate = cfg->param.rc.bitrate * 1000;
    frameDuration = 1.0 / cfg->param.frameRate;
    lastNonBPictType = -1;
    baseQp = cfg->param.rc.qp;
    qp = baseQp;
    lastRceq = 1; /* handles the cmplxrsum when the previous frame cost is zero */
    totalBits = 0;
    shortTermCplxSum = 0;
    shortTermCplxCount = 0;
    framesDone = 0;
    lastNonBPictType = I_SLICE;

    // vbv initialization
    cfg->param.rc.vbvBufferSize = Clip3(0, 2000000, cfg->param.rc.vbvBufferSize);
    cfg->param.rc.vbvMaxBitrate = Clip3(0, 2000000, cfg->param.rc.vbvMaxBitrate);
    cfg->param.rc.vbvBufferInit = Clip3(0.0, 2000000.0, cfg->param.rc.vbvBufferInit);

    if (cfg->param.rc.vbvBufferSize)
    {
        if (cfg->param.rc.rateControlMode == X265_RC_CQP)
        {
            x265_log(&cfg->param, X265_LOG_WARNING, "VBV is incompatible with constant QP, ignored.\n");
            cfg->param.rc.vbvBufferSize = 0;
            cfg->param.rc.vbvMaxBitrate = 0;
        }
        else if (cfg->param.rc.vbvMaxBitrate == 0)
        {
            if (cfg->param.rc.rateControlMode == X265_RC_ABR)
            {
                x265_log(&cfg->param, X265_LOG_WARNING, "VBV maxrate unspecified, assuming CBR\n");
                cfg->param.rc.vbvMaxBitrate = cfg->param.rc.bitrate;
            }
            else
            {
                x265_log(&cfg->param, X265_LOG_WARNING, "VBV bufsize set but maxrate unspecified, ignored\n");
                cfg->param.rc.vbvBufferSize = 0;
            }
        }
        else if (cfg->param.rc.vbvMaxBitrate < cfg->param.rc.bitrate &&
                 cfg->param.rc.rateControlMode == X265_RC_ABR)
        {
            x265_log(&cfg->param, X265_LOG_WARNING, "max bitrate less than average bitrate, assuming CBR\n");
            cfg->param.rc.bitrate = cfg->param.rc.vbvMaxBitrate;
        }
    }
    else if (cfg->param.rc.vbvMaxBitrate)
    {
        x265_log(&cfg->param, X265_LOG_WARNING, "VBV maxrate specified, but no bufsize, ignored\n");
        cfg->param.rc.vbvMaxBitrate = 0;
    }

    isVbv = cfg->param.rc.vbvMaxBitrate > 0 && cfg->param.rc.vbvBufferSize > 0;
    fps = cfg->param.frameRate;
    if (isVbv)
    {
        if (cfg->param.rc.vbvBufferSize < (int)(cfg->param.rc.vbvMaxBitrate / fps))
        {
            cfg->param.rc.vbvBufferSize = (int)(cfg->param.rc.vbvMaxBitrate / fps);
            x265_log(&cfg->param, X265_LOG_WARNING, "VBV buffer size cannot be smaller than one frame, using %d kbit\n",
                     cfg->param.rc.vbvBufferSize);
        }
        int vbvBufferSize = cfg->param.rc.vbvBufferSize * 1000;
        int vbvMaxBitrate = cfg->param.rc.vbvMaxBitrate * 1000;

        bufferRate = vbvMaxBitrate / fps;
        vbvMaxRate = vbvMaxBitrate;
        bufferSize = vbvBufferSize;
        singleFrameVbv = bufferRate * 1.1 > bufferSize;
        bufferFillFinal = bufferSize * cfg->param.rc.vbvBufferInit;
    }

    for (int i = 0; i < 5; i++)
    {
        pred[i].coeff = 2.0;
        pred[i].count = 1.0;
        pred[i].decay = 0.5;
        pred[i].offset = 0.0;
    }

    predBfromP = pred[0];
    bframes = cfg->param.bframes;
    bframeBits = 0;
    leadingNoBSatd = 0;

    if (cfg->param.rc.rateControlMode == X265_RC_ABR)
    {
        /* Adjust the first frame in order to stabilize the quality level compared to the rest */
#define ABR_INIT_QP_MIN (24 + QP_BD_OFFSET)
#define ABR_INIT_QP_MAX (34 + QP_BD_OFFSET)
        accumPNorm = .01;
        accumPQp = (ABR_INIT_QP_MIN)*accumPNorm;
        /* estimated ratio that produces a reasonable QP for the first I-frame */
        cplxrSum = .01 * pow(7.0e5, qCompress) * pow(ncu, 0.5);
        wantedBitsWindow = bitrate * frameDuration;
    }
    else if (cfg->param.rc.rateControlMode == X265_RC_CRF)
    {
#define ABR_INIT_QP ((int)cfg->param.rc.rfConstant + QP_BD_OFFSET)
        accumPNorm = .01;
        accumPQp = ABR_INIT_QP * accumPNorm;
        /* estimated ratio that produces a reasonable QP for the first I-frame */
        cplxrSum = .01 * pow(7.0e5, qCompress) * pow(ncu, 0.5);
        wantedBitsWindow = bitrate * frameDuration;
    }

    ipOffset = 6.0 * X265_LOG2(cfg->param.rc.ipFactor);
    pbOffset = 6.0 * X265_LOG2(cfg->param.rc.pbFactor);
    for (int i = 0; i < 3; i++)
    {
        lastQScaleFor[i] = qp2qScale(cfg->param.rc.rateControlMode == X265_RC_CRF ? ABR_INIT_QP : ABR_INIT_QP_MIN);
        lmin[i] = qp2qScale(MIN_QP);
        lmax[i] = qp2qScale(MAX_MAX_QP);
    }

    if (cfg->param.rc.rateControlMode == X265_RC_CQP)
    {
        qpConstant[P_SLICE] = baseQp;
        qpConstant[I_SLICE] = Clip3(0, MAX_MAX_QP, (int)(baseQp - ipOffset + 0.5));
        qpConstant[B_SLICE] = Clip3(0, MAX_MAX_QP, (int)(baseQp + pbOffset + 0.5));
    }

    /* qstep - value set as encoder specific */
    lstep = pow(2, cfg->param.rc.qpStep / 6.0);
}

void RateControl::rateControlStart(TComPic* pic, Lookahead *l, RateControlEntry* rce, Encoder* enc)
{
    curSlice = pic->getSlice();
    sliceType = curSlice->getSliceType();
    rce->sliceType = sliceType;

    if (sliceType == B_SLICE)
        rce->bframes = bframes;
    else
        bframes = pic->m_lowres.leadingBframes;

    rce->bLastMiniGopBFrame = pic->m_lowres.bLastMiniGopBFrame;
    rce->bufferRate = bufferRate;
    rce->poc = curSlice->getPOC();

    if (isVbv)
        updateVbvPlan(enc);

    if (isAbr) //ABR,CRF
    {
        lastSatd = l->getEstimatedPictureCost(pic);
        double q = qScale2qp(rateEstimateQscale(rce));
        qp = Clip3(MIN_QP, MAX_MAX_QP, (int)(q + 0.5));
        rce->qpaRc = q;
        /* copy value of lastRceq into thread local rce struct *to be used in RateControlEnd() */
        rce->qRceq = lastRceq;
        rce->lastSatd = lastSatd;
        accumPQpUpdate();
    }
    else //CQP
    {
        qp = qpConstant[sliceType];
    }

    if (sliceType != B_SLICE)
        lastNonBPictType = sliceType;
    framesDone++;
    /* set the final QP to slice structure */
    curSlice->setSliceQp(qp);
    curSlice->setSliceQpBase(qp);
    curSlice->m_avgQpRc = qp;
}

void RateControl::accumPQpUpdate()
{
    accumPQp   *= .95;
    accumPNorm *= .95;
    accumPNorm += 1;
    if (sliceType == I_SLICE)
        accumPQp += qp + ipOffset;
    else
        accumPQp += qp;
}

double RateControl::rateEstimateQscale(RateControlEntry *rce)
{
    double q;

    if (sliceType == B_SLICE)
    {
        /* B-frames don't have independent rate control, but rather get the
         * average QP of the two adjacent P-frames + an offset */
        TComSlice* prevRefSlice = curSlice->getRefPic(REF_PIC_LIST_0, 0)->getSlice();
        TComSlice* nextRefSlice = curSlice->getRefPic(REF_PIC_LIST_1, 0)->getSlice();
        bool i0 = prevRefSlice->getSliceType() == I_SLICE;
        bool i1 = nextRefSlice->getSliceType() == I_SLICE;
        int dt0 = abs(curSlice->getPOC() - prevRefSlice->getPOC());
        int dt1 = abs(curSlice->getPOC() - nextRefSlice->getPOC());
        double q0 = prevRefSlice->m_avgQpRc;
        double q1 = nextRefSlice->m_avgQpRc;

        if (prevRefSlice->getSliceType() == B_SLICE && prevRefSlice->isReferenced())
            q0 -= pbOffset / 2;
        if (nextRefSlice->getSliceType() == B_SLICE && nextRefSlice->isReferenced())
            q1 -= pbOffset / 2;
        if (i0 && i1)
            q = (q0 + q1) / 2 + ipOffset;
        else if (i0)
            q = q1;
        else if (i1)
            q = q0;
        else
            q = (q0 * dt1 + q1 * dt0) / (dt0 + dt1);

        if (curSlice->isReferenced())
            q += pbOffset / 2;
        else
            q += pbOffset;

        rce->frameSizePlanned = predictSize(&predBfromP, qp2qScale(q), leadingNoBSatd);

        return qp2qScale(q);
    }
    else
    {
        double abrBuffer = 2 * cfg->param.rc.rateTolerance * bitrate;

        /* 1pass ABR */

        /* Calculate the quantizer which would have produced the desired
         * average bitrate if it had been applied to all frames so far.
         * Then modulate that quant based on the current frame's complexity
         * relative to the average complexity so far (using the 2pass RCEQ).
         * Then bias the quant up or down if total size so far was far from
         * the target.
         * Result: Depending on the value of rate_tolerance, there is a
         * tradeoff between quality and bitrate precision. But at large
         * tolerances, the bit distribution approaches that of 2pass. */

        double wantedBits, overflow = 1;
        shortTermCplxSum *= 0.5;
        shortTermCplxCount *= 0.5;
        shortTermCplxSum += lastSatd / (CLIP_DURATION(frameDuration) / BASE_FRAME_DURATION);
        shortTermCplxCount++;
        rce->texBits = lastSatd;
        rce->blurredComplexity = shortTermCplxSum / shortTermCplxCount;
        rce->mvBits = 0;
        rce->sliceType = sliceType;

        if (cfg->param.rc.rateControlMode == X265_RC_CRF)
        {
            q = getQScale(rce, rateFactorConstant);
        }
        else
        {
            q = getQScale(rce, wantedBitsWindow / cplxrSum);

            /* ABR code can potentially be counterproductive in CBR, so just don't bother.
             * Don't run it if the frame complexity is zero either. */
            if (lastSatd)
            {
                /* use framesDone instead of POC as poc count is not serial with bframes enabled */
                double timeDone = (double)(framesDone - cfg->param.frameNumThreads + 1) / cfg->param.frameRate;
                wantedBits = timeDone * bitrate;
                if (wantedBits > 0 && totalBits > 0)
                {
                    abrBuffer *= X265_MAX(1, sqrt(timeDone));
                    overflow = Clip3(.5, 2.0, 1.0 + (totalBits - wantedBits) / abrBuffer);
                    q *= overflow;
                }
            }
        }

        if (sliceType == I_SLICE && cfg->param.keyframeMax > 1
            && lastNonBPictType != I_SLICE)
        {
            q = qp2qScale(accumPQp / accumPNorm);
            q /= fabs(cfg->param.rc.ipFactor);
        }
        else if (framesDone > 0)
        {
            if (cfg->param.rc.rateControlMode != X265_RC_CRF)
            {
                double lqmin = 0, lqmax = 0;
                if (totalBits == 0)
                {
                    lqmin = qp2qScale(ABR_INIT_QP_MIN) / lstep;
                    lqmax = qp2qScale(ABR_INIT_QP_MAX) * lstep;
                }
                else
                {
                    lqmin = lastQScaleFor[sliceType] / lstep;
                    lqmax = lastQScaleFor[sliceType] * lstep;
                }

                if (overflow > 1.1 && framesDone > 3)
                    lqmax *= lstep;
                else if (overflow < 0.9)
                    lqmin /= lstep;

                q = Clip3(lqmin, lqmax, q);
            }
        }
        else if (cfg->param.rc.rateControlMode == X265_RC_CRF && qCompress != 1)
        {
            q = qp2qScale(ABR_INIT_QP) / fabs(cfg->param.rc.ipFactor);
        }

        double lmin1 = lmin[sliceType];
        double lmax1 = lmax[sliceType];
        q = Clip3(lmin1, lmax1, q);

        q = clipQscale(q);

        lastQScaleFor[sliceType] = q;

        if (curSlice->getPOC() == 0)
            lastQScaleFor[P_SLICE] = q * fabs(cfg->param.rc.ipFactor);

        rce->frameSizePlanned = predictSize(&pred[sliceType], q, lastSatd);

        return q;
    }
}

void RateControl::updateVbvPlan(Encoder* enc)
{
    bufferFill = bufferFillFinal;
    enc->updateVbvPlan(this);
}

double RateControl::predictSize(Predictor *p, double q, double var)
{
    return (p->coeff * var + p->offset) / (q * p->count);
}

double RateControl::clipQscale(double q)
{
    double lmin1 = lmin[sliceType];
    double lmax1 = lmax[sliceType];
    double q0 = q;

    // B-frames are not directly subject to VBV,
    // since they are controlled by the P-frames' QPs.
    if (isVbv && lastSatd > 0)
    {
        //if (lookahead){} //for lookahead
        //else
        {
            if ((sliceType == P_SLICE ||
                 (sliceType == I_SLICE && lastNonBPictType == I_SLICE)) &&
                bufferFill / bufferSize < 0.5)
            {
                q /= Clip3(0.5, 1.0, 2.0 * bufferFill / bufferSize);
            }

            // Now a hard threshold to make sure the frame fits in VBV.
            // This one is mostly for I-frames.
            double bits = predictSize(&pred[sliceType], q, lastSatd);

            // For small VBVs, allow the frame to use up the entire VBV.
            double maxFillFactor;
            maxFillFactor = bufferSize >= 5 * bufferRate ? 2 : 1;
            // For single-frame VBVs, request that the frame use up the entire VBV.
            double minFillFactor = singleFrameVbv ? 1 : 2;

            for (int iterations = 0; iterations < 10; iterations++)
            {
                double qf = 1.0;
                if (bits > bufferFill / maxFillFactor)
                    qf = Clip3(0.2, 1.0, bufferFill / (maxFillFactor * bits));
                q /= qf;
                bits *= qf;
                if (bits < bufferRate / minFillFactor)
                    q *= bits * minFillFactor / bufferRate;
                bits = predictSize(&pred[sliceType], q, lastSatd);
            }

            q = X265_MAX(q0, q);
        }

        // Check B-frame complexity, and use up any bits that would
        // overflow before the next P-frame.
        if (sliceType == P_SLICE)
        {
            int nb = bframes;
            double bits = predictSize(&pred[sliceType], q, lastSatd);
            double pbbits = bits;
            double bbits = predictSize(&predBfromP, q * cfg->param.rc.pbFactor, lastSatd);
            double space;
            if (bbits > bufferRate)
                nb = 0;
            pbbits = nb * bbits;

            space = bufferFill + (1 + nb) * bufferRate - bufferSize;
            if (pbbits < space)
            {
                q *= X265_MAX(pbbits / space, bits / (0.5 * bufferSize));
            }
            q = X265_MAX(q0 - 5, q);
        }
        q = X265_MAX(q0, q);
    }

    if (lmin1 == lmax1)
        return lmin1;

    return Clip3(lmin1, lmax1, q);
}

/* modify the bitrate curve from pass1 for one frame */
double RateControl::getQScale(RateControlEntry *rce, double rateFactor)
{
    double q;

    if (cfg->param.rc.cuTree)
    {
        double scale = curSlice->getSPS()->getVuiParameters()->getTimingInfo()->getTimeScale();
        double units = curSlice->getSPS()->getVuiParameters()->getTimingInfo()->getNumUnitsInTick();
        double timescale = units / scale;
        q = pow(BASE_FRAME_DURATION / CLIP_DURATION(frameDuration * timescale), 1 - cfg->param.rc.qCompress);
    }
    else
        q = pow(rce->blurredComplexity, 1 - cfg->param.rc.qCompress);

    // avoid NaN's in the rc_eq
    if (rce->texBits + rce->mvBits == 0)
        q = lastQScaleFor[rce->sliceType];
    else
    {
        lastRceq = q;
        q /= rateFactor;
    }
    return q;
}

void RateControl::updatePredictor(Predictor *p, double q, double var, double bits)
{
    if (var < 10)
        return;
    const double range = 1.5;
    double old_coeff = p->coeff / p->count;
    double new_coeff = bits * q / var;
    double new_coeff_clipped = Clip3(old_coeff / range, old_coeff * range, new_coeff);
    double new_offset = bits * q - new_coeff_clipped * var;
    if (new_offset >= 0)
        new_coeff = new_coeff_clipped;
    else
        new_offset = 0;
    p->count  *= p->decay;
    p->coeff  *= p->decay;
    p->offset *= p->decay;
    p->count++;
    p->coeff  += new_coeff;
    p->offset += new_offset;
}

void RateControl::updateVbv(int64_t bits, RateControlEntry* rce)
{
    int mbCount = (int)((cfg->param.sourceHeight * cfg->param.sourceWidth) / pow((int)16, 2.0));

    if  (rce->lastSatd >= mbCount)
        updatePredictor(&pred[rce->sliceType], qp2qScale(rce->qpaRc), rce->lastSatd, (double)bits);

    if (!isVbv)
        return;

    bufferFillFinal -= bits;

    if (bufferFillFinal < 0)
        x265_log(&cfg->param, X265_LOG_WARNING, "poc:%d, VBV underflow (%.0f bits)\n", rce->poc, bufferFillFinal);

    bufferFillFinal = X265_MAX(bufferFillFinal, 0);
    bufferFillFinal += bufferRate;
    bufferFillFinal = X265_MIN(bufferFillFinal, bufferSize);
}

/* After encoding one frame, update rate control state */
int RateControl::rateControlEnd(int64_t bits, RateControlEntry* rce)
{
    if (isAbr)
    {
        if (rce->sliceType != B_SLICE)
            /* The factor 1.5 is to tune up the actual bits, otherwise the cplxrSum is scaled too low
             * to improve short term compensation for next frame. */
            cplxrSum += bits * qp2qScale(rce->qpaRc) / rce->qRceq;
        else
        {
            /* Depends on the fact that B-frame's QP is an offset from the following P-frame's.
             * Not perfectly accurate with B-refs, but good enough. */
            cplxrSum += bits * qp2qScale(rce->qpaRc) / (rce->qRceq * fabs(cfg->param.rc.pbFactor));
        }
        wantedBitsWindow += frameDuration * bitrate;
    }
    totalBits += bits;

    if (isVbv)
    {
        if (rce->sliceType == B_SLICE)
        {
            bframeBits += (int)bits;
            if (rce->bLastMiniGopBFrame)
            {
                if (rce->bframes != 0)
                    updatePredictor(&predBfromP, qp2qScale(rce->qpaRc), (double)rce->lastSatd, (double)bframeBits / rce->bframes);
                bframeBits = 0;
            }
        }
    }

    updateVbv(bits, rce);
    return 0;
}
