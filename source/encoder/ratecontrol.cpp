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
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#include "TLibCommon/TComPic.h"
#include "encoder.h"
#include "slicetype.h"
#include "ratecontrol.h"

using namespace x265;

/* Amortize the partial cost of I frames over the next N frames */
const double RateControl::amortizeFraction = 0.85;
const int RateControl::amortizeFrames = 75;

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
static inline uint32_t acEnergyPlane(TComPic *pic, pixel* src, int srcStride, int bChroma, int colorFormat)
{
    if ((colorFormat != X265_CSP_I444) && bChroma)
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
    int colorFormat = param->internalCsp;
    int hShift = CHROMA_H_SHIFT(colorFormat);
    int vShift = CHROMA_V_SHIFT(colorFormat);
    uint32_t blockOffsetChroma = (block_x >> hShift) + ((block_y >> vShift) * cStride);

    uint32_t var;

    var  = acEnergyPlane(pic, pic->getPicYuvOrg()->getLumaAddr() + blockOffsetLuma, stride, 0, colorFormat);
    var += acEnergyPlane(pic, pic->getPicYuvOrg()->getCbAddr() + blockOffsetChroma, cStride, 1, colorFormat);
    var += acEnergyPlane(pic, pic->getPicYuvOrg()->getCrAddr() + blockOffsetChroma, cStride, 2, colorFormat);
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
    if (param->rc.aqMode == X265_AQ_NONE || param->rc.aqStrength == 0)
    {
        /* Need to init it anyways for CU tree */
        int cuWidth = ((maxCol / 2) + X265_LOWRES_CU_SIZE - 1) >> X265_LOWRES_CU_BITS;
        int cuHeight = ((maxRow / 2) + X265_LOWRES_CU_SIZE - 1) >> X265_LOWRES_CU_BITS;
        int cuCount = cuWidth * cuHeight;

        if (param->rc.aqMode && param->rc.aqStrength == 0)
        {
            memset(pic->m_lowres.qpCuTreeOffset, 0, cuCount * sizeof(double));
            memset(pic->m_lowres.qpAqOffset, 0, cuCount * sizeof(double));
            for (int cuxy = 0; cuxy < cuCount; cuxy++)
            {
                pic->m_lowres.invQscaleFactor[cuxy] = 256;
            }
        }

        /* Need variance data for weighted prediction */
        if (param->bEnableWeightedPred || param->bEnableWeightedBiPred)
        {
            for (block_y = 0; block_y < maxRow; block_y += 16)
            {
                for (block_x = 0; block_x < maxCol; block_x += 16)
                {
                    acEnergyCu(pic, block_x, block_y);
                }
            }
        }
    }
    else
    {
        block_xy = 0;
        double avg_adj_pow2 = 0, avg_adj = 0, qp_adj = 0;
        if (param->rc.aqMode == X265_AQ_AUTO_VARIANCE)
        {
            double bit_depth_correction = pow(1 << (X265_DEPTH - 8), 0.5);
            for (block_y = 0; block_y < maxRow; block_y += 16)
            {
                for (block_x = 0; block_x < maxCol; block_x += 16)
                {
                    uint32_t energy = acEnergyCu(pic, block_x, block_y);
                    qp_adj = pow(energy + 1, 0.1);
                    pic->m_lowres.qpCuTreeOffset[block_xy] = qp_adj;
                    avg_adj += qp_adj;
                    avg_adj_pow2 += qp_adj * qp_adj;
                    block_xy++;
                }
            }

            avg_adj /= ncu;
            avg_adj_pow2 /= ncu;
            strength = param->rc.aqStrength * avg_adj / bit_depth_correction;
            avg_adj = avg_adj - 0.5f * (avg_adj_pow2 - (11.f * bit_depth_correction)) / avg_adj;
        }
        else
            strength = param->rc.aqStrength * 1.0397f;

        block_xy = 0;
        for (block_y = 0; block_y < maxRow; block_y += 16)
        {
            for (block_x = 0; block_x < maxCol; block_x += 16)
            {
                if (param->rc.aqMode == X265_AQ_AUTO_VARIANCE)
                {
                    qp_adj = pic->m_lowres.qpCuTreeOffset[block_xy];
                    qp_adj = strength * (qp_adj - avg_adj);
                }
                else
                {
                    uint32_t energy = acEnergyCu(pic, block_x, block_y);
                    qp_adj = strength * (X265_LOG2(X265_MAX(energy, 1)) - (14.427f + 2 * (X265_DEPTH - 8)));
                }
                pic->m_lowres.qpAqOffset[block_xy] = qp_adj;
                pic->m_lowres.qpCuTreeOffset[block_xy] = qp_adj;
                pic->m_lowres.invQscaleFactor[block_xy] = x265_exp2fix8(qp_adj);
                block_xy++;
            }
        }
    }

    if (param->bEnableWeightedPred || param->bEnableWeightedBiPred)
    {
        int hShift = CHROMA_H_SHIFT(param->internalCsp);
        int vShift = CHROMA_V_SHIFT(param->internalCsp);
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

RateControl::RateControl(Encoder * _cfg)
{
    param = _cfg->param;
    int lowresCuWidth = ((param->sourceWidth / 2) + X265_LOWRES_CU_SIZE - 1) >> X265_LOWRES_CU_BITS;
    int lowresCuHeight = ((param->sourceHeight / 2)  + X265_LOWRES_CU_SIZE - 1) >> X265_LOWRES_CU_BITS;
    ncu = lowresCuWidth * lowresCuHeight;
    if (param->rc.cuTree)
    {
        qCompress = 1;
    }
    else
        qCompress = param->rc.qCompress;

    // validate for param->rc, maybe it is need to add a function like x265_parameters_valiate()
    residualFrames = 0;
    residualCost = 0;
    param->rc.rfConstant = Clip3((double)-QP_BD_OFFSET, (double)51, param->rc.rfConstant);
    param->rc.rfConstantMax = Clip3((double)-QP_BD_OFFSET, (double)51, param->rc.rfConstantMax);
    rateFactorMaxIncrement = 0;
    rateFactorMaxDecrement = 0;

    if (param->rc.rateControlMode == X265_RC_CRF)
    {
        param->rc.qp = (int)param->rc.rfConstant + QP_BD_OFFSET;
        param->rc.bitrate = 0;

        double baseCplx = ncu * (param->bframes ? 120 : 80);
        double mbtree_offset = param->rc.cuTree ? (1.0 - param->rc.qCompress) * 13.5 : 0;
        rateFactorConstant = pow(baseCplx, 1 - qCompress) /
            x265_qp2qScale(param->rc.rfConstant + mbtree_offset);
        if (param->rc.rfConstantMax)
        {
            rateFactorMaxIncrement = param->rc.rfConstantMax - param->rc.rfConstant;
            if (rateFactorMaxIncrement <= 0)
            {
                x265_log(param, X265_LOG_WARNING, "CRF max must be greater than CRF\n");
                rateFactorMaxIncrement = 0;
            }
        }
        if (param->rc.rfConstantMin)
            rateFactorMaxDecrement = param->rc.rfConstant - param->rc.rfConstantMin;
    }

    isAbr = param->rc.rateControlMode != X265_RC_CQP; // later add 2pass option
    bitrate = param->rc.bitrate * 1000;
    frameDuration = (double)param->fpsDenom / param->fpsNum;
    qp = param->rc.qp;
    lastRceq = 1; /* handles the cmplxrsum when the previous frame cost is zero */
    shortTermCplxSum = 0;
    shortTermCplxCount = 0;
    lastNonBPictType = I_SLICE;
    isAbrReset = false;
    lastAbrResetPoc = -1;
    // vbv initialization
    param->rc.vbvBufferSize = Clip3(0, 2000000, param->rc.vbvBufferSize);
    param->rc.vbvMaxBitrate = Clip3(0, 2000000, param->rc.vbvMaxBitrate);
    param->rc.vbvBufferInit = Clip3(0.0, 2000000.0, param->rc.vbvBufferInit);
    isCbr = false;
    singleFrameVbv = 0;
    if (param->rc.vbvBufferSize)
    {
        if (param->rc.rateControlMode == X265_RC_CQP)
        {
            x265_log(param, X265_LOG_WARNING, "VBV is incompatible with constant QP, ignored.\n");
            param->rc.vbvBufferSize = 0;
            param->rc.vbvMaxBitrate = 0;
        }
        else if (param->rc.vbvMaxBitrate == 0)
        {
            if (param->rc.rateControlMode == X265_RC_ABR)
            {
                x265_log(param, X265_LOG_WARNING, "VBV maxrate unspecified, assuming CBR\n");
                param->rc.vbvMaxBitrate = param->rc.bitrate;
            }
            else
            {
                x265_log(param, X265_LOG_WARNING, "VBV bufsize set but maxrate unspecified, ignored\n");
                param->rc.vbvBufferSize = 0;
            }
        }
        else if (param->rc.vbvMaxBitrate < param->rc.bitrate &&
                 param->rc.rateControlMode == X265_RC_ABR)
        {
            x265_log(param, X265_LOG_WARNING, "max bitrate less than average bitrate, assuming CBR\n");
            param->rc.bitrate = param->rc.vbvMaxBitrate;
        }
    }
    else if (param->rc.vbvMaxBitrate)
    {
        x265_log(param, X265_LOG_WARNING, "VBV maxrate specified, but no bufsize, ignored\n");
        param->rc.vbvMaxBitrate = 0;
    }

    isVbv = param->rc.vbvMaxBitrate > 0 && param->rc.vbvBufferSize > 0;
    double fps = (double)param->fpsNum / param->fpsDenom;
    if (isVbv)
    {
        /* We don't support changing the ABR bitrate right now,
           so if the stream starts as CBR, keep it CBR. */
        if (param->rc.vbvBufferSize < (int)(param->rc.vbvMaxBitrate / fps))
        {
            param->rc.vbvBufferSize = (int)(param->rc.vbvMaxBitrate / fps);
            x265_log(param, X265_LOG_WARNING, "VBV buffer size cannot be smaller than one frame, using %d kbit\n",
                     param->rc.vbvBufferSize);
        }
        int vbvBufferSize = param->rc.vbvBufferSize * 1000;
        int vbvMaxBitrate = param->rc.vbvMaxBitrate * 1000;

        bufferRate = vbvMaxBitrate / fps;
        vbvMaxRate = vbvMaxBitrate;
        bufferSize = vbvBufferSize;
        singleFrameVbv = bufferRate * 1.1 > bufferSize;

        if (param->rc.vbvBufferInit > 1.)
            param->rc.vbvBufferInit = Clip3(0.0, 1.0, param->rc.vbvBufferInit / param->rc.vbvBufferSize);
        param->rc.vbvBufferInit = Clip3(0.0, 1.0, X265_MAX(param->rc.vbvBufferInit, bufferRate / bufferSize));
        bufferFillFinal = bufferSize * param->rc.vbvBufferInit;
        isCbr = param->rc.rateControlMode == X265_RC_ABR
            && param->rc.vbvMaxBitrate <= param->rc.bitrate;
    }

    bframes = param->bframes;
    bframeBits = 0;
    leadingNoBSatd = 0;
    ipOffset = 6.0 * X265_LOG2(param->rc.ipFactor);
    pbOffset = 6.0 * X265_LOG2(param->rc.pbFactor);
    init();

    /* Adjust the first frame in order to stabilize the quality level compared to the rest */
#define ABR_INIT_QP_MIN (24)
#define ABR_INIT_QP_MAX (40)
#define CRF_INIT_QP (int)param->rc.rfConstant
    for (int i = 0; i < 3; i++)
        lastQScaleFor[i] = x265_qp2qScale(param->rc.rateControlMode == X265_RC_CRF ? CRF_INIT_QP : ABR_INIT_QP_MIN);

    if (param->rc.rateControlMode == X265_RC_CQP)
    {
        if (qp && !param->bLossless)
        {
            qpConstant[P_SLICE] = qp;
            qpConstant[I_SLICE] = Clip3(0, MAX_MAX_QP, (int)(qp - ipOffset + 0.5));
            qpConstant[B_SLICE] = Clip3(0, MAX_MAX_QP, (int)(qp + pbOffset + 0.5));
        }
        else
        {
            qpConstant[P_SLICE] = qpConstant[I_SLICE] = qpConstant[B_SLICE] = qp;
        }
    }

    /* qstep - value set as encoder specific */
    lstep = pow(2, param->rc.qpStep / 6.0);
}

void RateControl::init()
{
    totalBits = 0;
    framesDone = 0;
    residualCost = 0;
    double tuneCplxFactor = 1;
    /* 720p videos seem to be a good cutoff for cplxrSum */
    if (param->rc.cuTree && ncu > 3600)
        tuneCplxFactor = 2.5;
    /* estimated ratio that produces a reasonable QP for the first I-frame */
    cplxrSum = .01 * pow(7.0e5, qCompress) * pow(ncu, 0.5) * tuneCplxFactor;
    wantedBitsWindow = bitrate * frameDuration;
    accumPNorm = .01;
    accumPQp = (param->rc.rateControlMode == X265_RC_CRF ? CRF_INIT_QP : ABR_INIT_QP_MIN) * accumPNorm;

    /* Frame Predictors and Row predictors used in vbv */
    for (int i = 0; i < 5; i++)
    {
        pred[i].coeff = 2.0;
        pred[i].count = 1.0;
        pred[i].decay = 0.5;
        pred[i].offset = 0.0;
    }

    predBfromP = pred[0];
}

void RateControl::rateControlStart(TComPic* pic, Lookahead *l, RateControlEntry* rce, Encoder* enc)
{
    curSlice = pic->getSlice();
    sliceType = curSlice->getSliceType();
    rce->sliceType = sliceType;
    rce->isActive = true;
    if (sliceType == B_SLICE)
        rce->bframes = bframes;
    else
        bframes = pic->m_lowres.leadingBframes;

    rce->bLastMiniGopBFrame = pic->m_lowres.bLastMiniGopBFrame;
    rce->bufferRate = bufferRate;
    rce->poc = curSlice->getPOC();
    if (isVbv)
    {
        if (rce->rowPreds[0][0].count == 0)
        {
            for (int i = 0; i < 3; i++)
            {
                for (int j = 0; j < 2; j++)
                {
                    rce->rowPreds[i][j].coeff = 0.25;
                    rce->rowPreds[i][j].count = 1.0;
                    rce->rowPreds[i][j].decay = 0.5;
                    rce->rowPreds[i][j].offset = 0.0;
                }
            }
        }
        rce->rowPred[0] = &rce->rowPreds[sliceType][0];
        rce->rowPred[1] = &rce->rowPreds[sliceType][1];
        updateVbvPlan(enc);
        rce->bufferFill = bufferFill;
    }
    if (isAbr) //ABR,CRF
    {
        currentSatd = l->getEstimatedPictureCost(pic) >> (X265_DEPTH - 8);
        /* Update rce for use in rate control VBV later */
        rce->lastSatd = currentSatd;
        double q = x265_qScale2qp(rateEstimateQscale(pic, rce));
        q = Clip3((double)MIN_QP, (double)MAX_MAX_QP, q);
        qp = int(q + 0.5);
        rce->qpaRc = pic->m_avgQpRc = pic->m_avgQpAq = q;
        /* copy value of lastRceq into thread local rce struct *to be used in RateControlEnd() */
        rce->qRceq = lastRceq;
        accumPQpUpdate();
    }
    else //CQP
    {
        if (sliceType == B_SLICE && curSlice->isReferenced())
            qp = (qpConstant[B_SLICE] + qpConstant[P_SLICE]) / 2;
        else
            qp = qpConstant[sliceType];
        pic->m_avgQpAq = pic->m_avgQpRc = qp;
    }
    if (sliceType != B_SLICE)
    {
        lastNonBPictType = sliceType;
        leadingNoBSatd = currentSatd;
    }
    rce->leadingNoBSatd = leadingNoBSatd;
    if (pic->m_forceqp)
    {
        qp = int32_t(pic->m_forceqp + 0.5) - 1;
        qp = Clip3(MIN_QP, MAX_MAX_QP, qp);
        rce->qpaRc = pic->m_avgQpRc = pic->m_avgQpAq = qp;
    }
    framesDone++;
    /* set the final QP to slice structure */
    curSlice->setSliceQp(qp);
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

double RateControl::rateEstimateQscale(TComPic* pic, RateControlEntry *rce)
{
    double q;

    if (sliceType == B_SLICE)
    {
        /* B-frames don't have independent rate control, but rather get the
         * average QP of the two adjacent P-frames + an offset */
        TComSlice* prevRefSlice = curSlice->getRefPic(REF_PIC_LIST_0, 0)->getSlice();
        TComSlice* nextRefSlice = curSlice->getRefPic(REF_PIC_LIST_1, 0)->getSlice();
        double q0 = curSlice->getRefPic(REF_PIC_LIST_0, 0)->m_avgQpRc;
        double q1 = curSlice->getRefPic(REF_PIC_LIST_1, 0)->m_avgQpRc;
        bool i0 = prevRefSlice->getSliceType() == I_SLICE;
        bool i1 = nextRefSlice->getSliceType() == I_SLICE;
        int dt0 = abs(curSlice->getPOC() - prevRefSlice->getPOC());
        int dt1 = abs(curSlice->getPOC() - nextRefSlice->getPOC());

        // Skip taking a reference frame before the Scenecut if ABR has been reset.
        if (lastAbrResetPoc >= 0)
        {
            if (prevRefSlice->getSliceType() == P_SLICE && prevRefSlice->getPOC() < lastAbrResetPoc)
            {
                i0 = i1;
                dt0 = dt1;
                q0 = q1;
            }
        }
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
        rce->qpNoVbv = q;
        double qScale = x265_qp2qScale(q);
        rce->frameSizePlanned = predictSize(&predBfromP, qScale, (double)leadingNoBSatd);
        rce->frameSizeEstimated = rce->frameSizePlanned;
        return qScale;
    }
    else
    {
        double abrBuffer = 2 * param->rc.rateTolerance * bitrate;

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
        rce->movingAvgSum = shortTermCplxSum;
        shortTermCplxSum *= 0.5;
        shortTermCplxCount *= 0.5;
        shortTermCplxSum += currentSatd / (CLIP_DURATION(frameDuration) / BASE_FRAME_DURATION);
        shortTermCplxCount++;
        /* texBits to be used in 2-pass */
        rce->texBits = currentSatd;
        rce->blurredComplexity = shortTermCplxSum / shortTermCplxCount;
        rce->mvBits = 0;
        rce->sliceType = sliceType;

        if (param->rc.rateControlMode == X265_RC_CRF)
        {
            q = getQScale(rce, rateFactorConstant);
        }
        else
        {
            checkAndResetABR(rce, false);
            q = getQScale(rce, wantedBitsWindow / cplxrSum);

            /* ABR code can potentially be counterproductive in CBR, so just
             * don't bother.  Don't run it if the frame complexity is zero
             * either. */
            if (!isCbr && currentSatd)
            {
                /* use framesDone instead of POC as poc count is not serial with bframes enabled */
                double timeDone = (double)(framesDone - param->frameNumThreads + 1) * frameDuration;
                wantedBits = timeDone * bitrate;
                if (wantedBits > 0 && totalBits > 0 && !residualFrames)
                {
                    abrBuffer *= X265_MAX(1, sqrt(timeDone));
                    overflow = Clip3(.5, 2.0, 1.0 + (totalBits - wantedBits) / abrBuffer);
                    q *= overflow;
                }
            }
        }

        if (sliceType == I_SLICE && param->keyframeMax > 1
            && lastNonBPictType != I_SLICE && !isAbrReset)
        {
            q = x265_qp2qScale(accumPQp / accumPNorm);
            q /= fabs(param->rc.ipFactor);
        }
        else if (framesDone > 0)
        {
            if (param->rc.rateControlMode != X265_RC_CRF)
            {
                double lqmin = 0, lqmax = 0;
                lqmin = lastQScaleFor[sliceType] / lstep;
                lqmax = lastQScaleFor[sliceType] * lstep;
                if (!residualFrames)
                {
                    if (overflow > 1.1 && framesDone > 3)
                        lqmax *= lstep;
                    else if (overflow < 0.9)
                        lqmin /= lstep;
                }
                q = Clip3(lqmin, lqmax, q);
            }
        }
        else if (qCompress != 1 && param->rc.rateControlMode == X265_RC_CRF)
        {
            q = x265_qp2qScale(CRF_INIT_QP) / fabs(param->rc.ipFactor);
        }
        else if (framesDone == 0 && !isVbv)
        {
            /* for ABR alone, clip the first I frame qp */
            double lqmax = x265_qp2qScale(ABR_INIT_QP_MAX) * lstep;
            q = X265_MIN(lqmax, q);
        }

        q = Clip3(MIN_QPSCALE, MAX_MAX_QPSCALE, q);
        rce->qpNoVbv = x265_qScale2qp(q);

        if (isVbv && currentSatd > 0)
            q = clipQscale(pic, q);

        lastQScaleFor[sliceType] = q;

        if (curSlice->getPOC() == 0 || lastQScaleFor[P_SLICE] < q )
            lastQScaleFor[P_SLICE] = q * fabs(param->rc.ipFactor);

        rce->frameSizePlanned = predictSize(&pred[sliceType], q, (double)currentSatd);
        rce->frameSizeEstimated = rce->frameSizePlanned;
        /* Always use up the whole VBV in this case. */
        if (singleFrameVbv)
            rce->frameSizePlanned = bufferRate;

        return q;
    }
}

void RateControl::checkAndResetABR(RateControlEntry* rce, bool isFrameDone)
{
    double abrBuffer = 2 * param->rc.rateTolerance * bitrate;

    // Check if current Slice is a scene cut that follows low detailed/blank frames
    if (rce->lastSatd > 4 * rce->movingAvgSum)
    {
        if (!isAbrReset && rce->movingAvgSum > 0)
        {
            // Reset ABR if prev frames are blank to prevent further sudden overflows/ high bit rate spikes.
            double underflow = 1.0 + (totalBits - wantedBitsWindow) / abrBuffer;
            if (underflow < 0.9 && !isFrameDone)
            {
                init();
                shortTermCplxSum = rce->lastSatd / (CLIP_DURATION(frameDuration) / BASE_FRAME_DURATION);
                shortTermCplxCount = 1;
                isAbrReset = true;
                lastAbrResetPoc = rce->poc;
            }
        }
        else
        {
            // Clear flag to reset ABR and continue as usual.
            isAbrReset = false;
        }
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

double RateControl::clipQscale(TComPic* pic, double q)
{
    // B-frames are not directly subject to VBV,
    // since they are controlled by referenced P-frames' QPs.
    double q0 = q;

    if (param->lookaheadDepth || param->rc.cuTree ||
       param->scenecutThreshold ||
       (param->bFrameAdaptive && param->bframes))
    {
       /* Lookahead VBV: If lookahead is done, raise the quantizer as necessary
        * such that no frames in the lookahead overflow and such that the buffer
        * is in a reasonable state by the end of the lookahead. */

        int terminate = 0;

        /* Avoid an infinite loop. */
        for (int iterations = 0; iterations < 1000 && terminate != 3; iterations++)
        {
            double frameQ[3];
            double curBits = predictSize(&pred[sliceType], q, (double)currentSatd);
            double bufferFillCur = bufferFill - curBits;
            double targetFill;
            double totalDuration = 0;
            frameQ[P_SLICE] = sliceType == I_SLICE ? q * param->rc.ipFactor : q;
            frameQ[B_SLICE] = frameQ[P_SLICE] * param->rc.pbFactor;
            frameQ[I_SLICE] = frameQ[P_SLICE] / param->rc.ipFactor;
            /* Loop over the planned future frames. */
            for (int j = 0; bufferFillCur >= 0 && bufferFillCur <= bufferSize; j++)
            {
                totalDuration += frameDuration;
                bufferFillCur += vbvMaxRate * frameDuration;
                int type = pic->m_lowres.plannedType[j];
                int64_t satd = pic->m_lowres.plannedSatd[j] >> (X265_DEPTH - 8);
                if (type == X265_TYPE_AUTO)
                    break;
                type = IS_X265_TYPE_I(type) ? I_SLICE : IS_X265_TYPE_B(type) ? B_SLICE : P_SLICE;
                curBits = predictSize(&pred[type], frameQ[type], (double)satd);
                bufferFillCur -= curBits;
            }

            /* Try to get the buffer at least 50% filled, but don't set an impossible goal. */
            targetFill = X265_MIN(bufferFill + totalDuration * vbvMaxRate * 0.5, bufferSize * 0.5);
            if (bufferFillCur < targetFill)
            {
                q *= 1.01;
                terminate |= 1;
                continue;
            }
            /* Try to get the buffer no more than 80% filled, but don't set an impossible goal. */
            targetFill = Clip3(bufferSize * 0.8, bufferSize, bufferFill - totalDuration * vbvMaxRate * 0.5);
            if (isCbr && bufferFillCur > targetFill)
            {
                q /= 1.01;
                terminate |= 2;
                continue;
            }
            break;
        }
    }
    else
    {
        /* Fallback to old purely-reactive algorithm: no lookahead. */
        if ((sliceType == P_SLICE ||
                (sliceType == I_SLICE && lastNonBPictType == I_SLICE)) &&
            bufferFill / bufferSize < 0.5)
        {
            q /= Clip3(0.5, 1.0, 2.0 * bufferFill / bufferSize);
        }

        // Now a hard threshold to make sure the frame fits in VBV.
        // This one is mostly for I-frames.
        double bits = predictSize(&pred[sliceType], q, (double)currentSatd);

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
            bits = predictSize(&pred[sliceType], q, (double)currentSatd);
        }

        q = X265_MAX(q0, q);
    }

    // Check B-frame complexity, and use up any bits that would
    // overflow before the next P-frame.
    if (sliceType == P_SLICE && !singleFrameVbv)
    {
        int nb = bframes;
        double bits = predictSize(&pred[sliceType], q, (double)currentSatd);
        double bbits = predictSize(&predBfromP, q * param->rc.pbFactor, (double)currentSatd);
        double space;
        if (bbits > bufferRate)
            nb = 0;
        double pbbits = nb * bbits;

        space = bufferFill + (1 + nb) * bufferRate - bufferSize;
        if (pbbits < space)
        {
            q *= X265_MAX(pbbits / space, bits / (0.5 * bufferSize));
        }
        q = X265_MAX(q0 / 2, q);
    }
    if (!isCbr)
        q = X265_MAX(q0, q);

    if (rateFactorMaxIncrement)
    {
        double qpNoVbv = x265_qScale2qp(q0);
        double qmax = X265_MIN(MAX_MAX_QPSCALE,x265_qp2qScale(qpNoVbv + rateFactorMaxIncrement));
        return Clip3(MIN_QPSCALE, qmax, q);
    }

    return Clip3(MIN_QPSCALE, MAX_MAX_QPSCALE, q);
}

double RateControl::predictRowsSizeSum(TComPic* pic, RateControlEntry* rce, double qpVbv, int32_t & encodedBitsSoFar)
{
    uint32_t rowSatdCostSoFar = 0, totalSatdBits = 0;

    encodedBitsSoFar = 0;
    double qScale = x265_qp2qScale(qpVbv);
    int picType = pic->getSlice()->getSliceType();
    TComPic* refPic = pic->getSlice()->getRefPic(REF_PIC_LIST_0, 0);
    int maxRows = pic->getPicSym()->getFrameHeightInCU();
    for (int row = 0; row < maxRows; row++)
    {
        encodedBitsSoFar += pic->m_rowEncodedBits[row];
        rowSatdCostSoFar = pic->m_rowDiagSatd[row];
        uint32_t satdCostForPendingCus = pic->m_rowSatdForVbv[row] - rowSatdCostSoFar;
        satdCostForPendingCus >>= X265_DEPTH - 8;
        if (satdCostForPendingCus  > 0)
        {
            double pred_s = predictSize(rce->rowPred[0], qScale, satdCostForPendingCus);
            uint32_t refRowSatdCost = 0, refRowBits = 0, intraCost = 0;
            double refQScale = 0;

            if (picType != I_SLICE)
            {
                uint32_t endCuAddr = pic->getPicSym()->getFrameWidthInCU() * (row + 1);
                for (uint32_t cuAddr = pic->m_numEncodedCusPerRow[row] + 1; cuAddr < endCuAddr; cuAddr++)
                {
                    refRowSatdCost += refPic->m_cuCostsForVbv[cuAddr];
                    refRowBits += refPic->getCU(cuAddr)->m_totalBits;
                    intraCost += pic->m_intraCuCostsForVbv[cuAddr];
                }

                refRowSatdCost >>= X265_DEPTH - 8;
                refQScale = refPic->m_rowDiagQScale[row];
            }

            if (picType == I_SLICE || qScale >= refQScale)
            {
                if (picType == P_SLICE
                    && refPic->getSlice()->getSliceType() == picType
                    && refQScale > 0
                    && refRowSatdCost > 0)
                {
                    if (abs(int32_t(refRowSatdCost - satdCostForPendingCus)) < (int32_t)satdCostForPendingCus / 2)
                    {
                        double pred_t = refRowBits * satdCostForPendingCus / refRowSatdCost
                            * refQScale / qScale;
                        totalSatdBits += int32_t((pred_s + pred_t) * 0.5);
                        continue;
                    }
                }
                totalSatdBits += int32_t(pred_s);
            }
            else
            {
                /* Our QP is lower than the reference! */
                double pred_intra = predictSize(rce->rowPred[1], qScale, intraCost);
                /* Sum: better to overestimate than underestimate by using only one of the two predictors. */
                totalSatdBits += int32_t(pred_intra + pred_s);
            }
        }
    }

    return totalSatdBits + encodedBitsSoFar;
}

int RateControl::rowDiagonalVbvRateControl(TComPic* pic, uint32_t row, RateControlEntry* rce, double& qpVbv)
{
    double qScaleVbv = x265_qp2qScale(qpVbv);
    uint64_t rowSatdCost = pic->m_rowDiagSatd[row];
    double encodedBits = pic->m_rowEncodedBits[row];

    if (row == 1)
    {
        rowSatdCost += pic->m_rowDiagSatd[0];
        encodedBits += pic->m_rowEncodedBits[0];
    }
    rowSatdCost >>= X265_DEPTH - 8;
    updatePredictor(rce->rowPred[0], qScaleVbv, (double)rowSatdCost, encodedBits);
    if (pic->getSlice()->getSliceType() == P_SLICE)
    {
        TComPic* refSlice = pic->getSlice()->getRefPic(REF_PIC_LIST_0, 0);
        if (qpVbv < refSlice->m_rowDiagQp[row])
        {
            uint64_t intraRowSatdCost = pic->m_rowDiagIntraSatd[row];
            if (row == 1)
            {
                intraRowSatdCost += pic->m_rowDiagIntraSatd[0];
            }
            updatePredictor(rce->rowPred[1], qScaleVbv, (double)intraRowSatdCost, encodedBits);
        }
    }

    int canReencodeRow = 1;
    /* tweak quality based on difference from predicted size */
    double prevRowQp = qpVbv;
    double qpAbsoluteMax = MAX_MAX_QP;
    double qpAbsoluteMin = MIN_QP;
    if (rateFactorMaxIncrement)
        qpAbsoluteMax = X265_MIN(qpAbsoluteMax, rce->qpNoVbv + rateFactorMaxIncrement);

    if (rateFactorMaxDecrement)
        qpAbsoluteMin = X265_MAX(qpAbsoluteMin, rce->qpNoVbv - rateFactorMaxDecrement);

    double qpMax = X265_MIN(prevRowQp + param->rc.qpStep, qpAbsoluteMax);
    double qpMin = X265_MAX(prevRowQp - param->rc.qpStep, qpAbsoluteMin);
    double stepSize = 0.5;
    double bufferLeftPlanned = rce->bufferFill - rce->frameSizePlanned;

    double maxFrameError = X265_MAX(0.05, 1.0 / pic->getFrameHeightInCU());

    if (row < pic->getPicSym()->getFrameHeightInCU() - 1)
    {
        /* B-frames shouldn't use lower QP than their reference frames. */
        if (rce->sliceType == B_SLICE)
        {
            TComPic* refSlice1 = pic->getSlice()->getRefPic(REF_PIC_LIST_0, 0);
            TComPic* refSlice2 = pic->getSlice()->getRefPic(REF_PIC_LIST_1, 0);
            qpMin = X265_MAX(qpMin, X265_MAX(refSlice1->m_rowDiagQp[row], refSlice2->m_rowDiagQp[row]));
            qpVbv = X265_MAX(qpVbv, qpMin);
        }
        /* More threads means we have to be more cautious in letting ratecontrol use up extra bits. */
        double rcTol = bufferLeftPlanned / param->frameNumThreads * param->rc.rateTolerance;
        int32_t encodedBitsSoFar = 0;
        double accFrameBits = predictRowsSizeSum(pic, rce, qpVbv, encodedBitsSoFar);

        /* * Don't increase the row QPs until a sufficent amount of the bits of
         * the frame have been processed, in case a flat area at the top of the
         * frame was measured inaccurately. */
        if (encodedBitsSoFar < 0.05f * rce->frameSizePlanned)
            qpMax = qpAbsoluteMax = prevRowQp;

        if (rce->sliceType != I_SLICE)
            rcTol *= 0.5;

        if (!isCbr)
            qpMin = X265_MAX(qpMin, rce->qpNoVbv);

        while (qpVbv < qpMax
               && ((accFrameBits > rce->frameSizePlanned + rcTol) ||
                   (rce->bufferFill - accFrameBits < bufferLeftPlanned * 0.5) ||
                   (accFrameBits > rce->frameSizePlanned && qpVbv < rce->qpNoVbv)))
        {
            qpVbv += stepSize;
            accFrameBits = predictRowsSizeSum(pic, rce, qpVbv, encodedBitsSoFar);
        }

        while (qpVbv > qpMin
               && (qpVbv > pic->m_rowDiagQp[0] || singleFrameVbv)
               && ((accFrameBits < rce->frameSizePlanned * 0.8f && qpVbv <= prevRowQp)
                   || accFrameBits < (rce->bufferFill - bufferSize + bufferRate) * 1.1))
        {
            qpVbv -= stepSize;
            accFrameBits = predictRowsSizeSum(pic, rce, qpVbv, encodedBitsSoFar);
        }

        /* avoid VBV underflow */
        while ((qpVbv < qpAbsoluteMax)
               && (rce->bufferFill - accFrameBits < bufferRate * maxFrameError))
        {
            qpVbv += stepSize;
            accFrameBits = predictRowsSizeSum(pic, rce, qpVbv, encodedBitsSoFar);
        }

        rce->frameSizeEstimated = accFrameBits;

        /* If the current row was large enough to cause a large QP jump, try re-encoding it. */
        if (qpVbv > qpMax && prevRowQp < qpMax && canReencodeRow)
        {
            /* Bump QP to halfway in between... close enough. */
            qpVbv = Clip3(prevRowQp + 1.0f, qpMax, (prevRowQp + qpVbv) * 0.5);
            return -1;
        }

        if (param->rc.rfConstantMin)
        {
            if (qpVbv < qpMin && prevRowQp > qpMin && canReencodeRow)
            {
                qpVbv = Clip3(qpMin, prevRowQp, (prevRowQp + qpVbv) * 0.5);
                return -1;
            }
        }
    }
    else
    {
        int32_t encodedBitsSoFar = 0;
        rce->frameSizeEstimated = predictRowsSizeSum(pic, rce, qpVbv, encodedBitsSoFar);

        /* Last-ditch attempt: if the last row of the frame underflowed the VBV,
         * try again. */
        if ((rce->frameSizeEstimated > (rce->bufferFill - bufferRate * maxFrameError) &&
             qpVbv < qpMax && canReencodeRow))
        {
            qpVbv = qpMax;
            return -1;
        }
    }
    return 0;
}

/* modify the bitrate curve from pass1 for one frame */
double RateControl::getQScale(RateControlEntry *rce, double rateFactor)
{
    double q;

    if (param->rc.cuTree)
    {
        // Scale and units are obtained from rateNum and rateDenom for videos with fixed frame rates.
        double timescale = (double)param->fpsDenom / (2 * param->fpsNum);
        q = pow(BASE_FRAME_DURATION / CLIP_DURATION(2 * timescale), 1 - param->rc.qCompress);
    }
    else
        q = pow(rce->blurredComplexity, 1 - param->rc.qCompress);

    lastRceq = q;
    q /= rateFactor;

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
    if (rce->lastSatd >= ncu)
        updatePredictor(&pred[rce->sliceType], x265_qp2qScale(rce->qpaRc), (double)rce->lastSatd, (double)bits);
    if (!isVbv)
        return;

    bufferFillFinal -= bits;

    if (bufferFillFinal < 0)
        x265_log(param, X265_LOG_WARNING, "poc:%d, VBV underflow (%.0f bits)\n", rce->poc, bufferFillFinal);

    bufferFillFinal = X265_MAX(bufferFillFinal, 0);
    bufferFillFinal += bufferRate;
    bufferFillFinal = X265_MIN(bufferFillFinal, bufferSize);
}

/* After encoding one frame, update rate control state */
int RateControl::rateControlEnd(TComPic* pic, int64_t bits, RateControlEntry* rce)
{
    int64_t actualBits = bits;
    if (isAbr)
    {
        if (param->rc.rateControlMode == X265_RC_ABR)
        {
            checkAndResetABR(rce, true);
        }
        if (param->rc.rateControlMode == X265_RC_CRF)
        {
            if (int(pic->m_avgQpRc + 0.5) == pic->getSlice()->getSliceQp())
                pic->m_rateFactor = rateFactorConstant;
            else
            {
                /* If vbv changed the frame QP recalculate the rate-factor */
                double baseCplx = ncu * (param->bframes ? 120 : 80);
                double mbtree_offset = param->rc.cuTree ? (1.0 - param->rc.qCompress) * 13.5 : 0;
                pic->m_rateFactor = pow(baseCplx, 1 - qCompress) /
                    x265_qp2qScale(int(pic->m_avgQpRc + 0.5) + mbtree_offset);
            }
        }
        if (!isAbrReset)
        {
            if (param->rc.aqMode || isVbv)
            {
                if (pic->m_qpaRc)
                {
                    for (uint32_t i = 0; i < pic->getFrameHeightInCU(); i++)
                    {
                        pic->m_avgQpRc += pic->m_qpaRc[i];
                    }

                    pic->m_avgQpRc /= (pic->getFrameHeightInCU() * pic->getFrameWidthInCU());
                    rce->qpaRc = pic->m_avgQpRc;
                    // copy avg RC qp to m_avgQpAq. To print out the correct qp when aq/cutree is disabled.
                    pic->m_avgQpAq = pic->m_avgQpRc;
                }

                if (pic->m_qpaAq)
                {
                    for (uint32_t i = 0; i < pic->getFrameHeightInCU(); i++)
                    {
                        pic->m_avgQpAq += pic->m_qpaAq[i];
                    }

                    pic->m_avgQpAq /= (pic->getFrameHeightInCU() * pic->getFrameWidthInCU());
                }
            }

            /* amortize part of each I slice over the next several frames, up to
             * keyint-max, to avoid over-compensating for the large I slice cost */
            if (rce->sliceType == I_SLICE)
            {
                /* previous I still had a residual; roll it into the new loan */
                if (residualFrames)
                    bits += residualCost * residualFrames;

                residualFrames = X265_MIN(amortizeFrames, param->keyframeMax);
                residualCost = (int)((bits * amortizeFraction) / residualFrames);
                bits -= residualCost * residualFrames;
            }
            else if (residualFrames)
            {
                bits += residualCost;
                residualFrames--;
            }

            if (rce->sliceType != B_SLICE)
                /* The factor 1.5 is to tune up the actual bits, otherwise the cplxrSum is scaled too low
                 * to improve short term compensation for next frame. */
                cplxrSum += bits * x265_qp2qScale(rce->qpaRc) / rce->qRceq;
            else
            {
                /* Depends on the fact that B-frame's QP is an offset from the following P-frame's.
                 * Not perfectly accurate with B-refs, but good enough. */
                cplxrSum += bits * x265_qp2qScale(rce->qpaRc) / (rce->qRceq * fabs(param->rc.pbFactor));
            }
            wantedBitsWindow += frameDuration * bitrate;
            totalBits += bits;
        }
    }

    if (isVbv)
    {
        if (rce->sliceType == B_SLICE)
        {
            bframeBits += actualBits;
            if (rce->bLastMiniGopBFrame)
            {
                if (rce->bframes != 0)
                    updatePredictor(&predBfromP, x265_qp2qScale(rce->qpaRc), (double)rce->leadingNoBSatd, (double)bframeBits / rce->bframes);
                bframeBits = 0;
            }
        }
        updateVbv(actualBits, rce);
    }
    rce->isActive = false;
    return 0;
}
