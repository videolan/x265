/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Sumalatha Polureddy <sumalatha@multicorewareinc.com>
 *          Aarthi Priya Thirumalai <aarthi@multicorewareinc.com>
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

#include "rateControl.h"

using namespace x265;

RateControl::RateControl(x265_param_t * param)
{
    keyFrameInterval = param->keyframeMax;
    fps = param->frameRate;
    bframes = param->bframes;
    rateTolerance = param->rc.rateTolerance;
    bitrate = param->rc.bitrate * 1000;
    frameDuration = 1.0 / param->frameRate;
    rateControlMode = param->rc.rateControlMode;
    ncu = (int)((param->sourceHeight * param->sourceWidth) / pow(2.0, (int)param->maxCUSize));
    lastNonBPictType = -1;
    qCompress = param->rc.qCompress;
    ipFactor = param->rc.ipFactor;
    pbFactor = param->rc.pbFactor;
    totalBits = 0;
    if (rateControlMode == X265_RC_ABR)
    {
        //TODO : confirm this value. obtained it from x264 when crf is disabled , abr enabled.
        //h->param.rc.i_rc_method == X264_RC_CRF ? h->param.rc.f_rf_constant : 24
#define ABR_INIT_QP (24  + QP_BD_OFFSET)
        accumPNorm = .01;
        accumPQp = (ABR_INIT_QP)*accumPNorm;
        /* estimated ratio that produces a reasonable QP for the first I-frame */
        cplxrSum = .01 * pow(7.0e5, qCompress) * pow(ncu, 0.5);
        wantedBitsWindow = 1.0 * bitrate / fps;
        lastNonBPictType = I_SLICE;
    }
    ipOffset = 6.0 * log(param->rc.ipFactor);
    pbOffset = 6.0 * log(param->rc.pbFactor);
    for (int i = 0; i < 3; i++)
    {
        lastQScaleFor[i] = qp2qScale(ABR_INIT_QP);
        lmin[i] = qp2qScale(MIN_QP);
        lmax[i] = qp2qScale(MAX_QP);
    }

    lstep = pow(2, param->rc.qpStep / 6.0);
    cbrDecay = 1.0;
}

void RateControl::rateControlInit(TComSlice* frame)
{
    curFrame = frame;
    frameType = frame->getSliceType();
}

void RateControl::rateControlStart(LookaheadFrame *lFrame)
{
    rce = new RateControlEntry();
    double q;

    //Always enabling ABR
    if (rateControlMode == X265_RC_ABR)
    {
        q = qScale2qp(rateEstimateQscale(lFrame));
    }
    else
        q = 0; // TODO
    q = Clip3((int)q, MIN_QP, MAX_QP);
    qp = Clip3((int)(q + 0.5f), 0, MAX_QP);
    qpaRc = qpm = q;    // qpaRc is set in the rate_control_mb call in x264. we are updating here itself.
    if (rce)
        rce->newQp = qp;

    accumPQpUpdate();

    if (frameType != B_SLICE)
        lastNonBPictType = frameType;
}

void RateControl::accumPQpUpdate()
{
    accumPQp   *= .95;
    accumPNorm *= .95;
    accumPNorm += 1;
    if (frameType == I_SLICE)
        accumPQp += qpm + ipOffset;
    else
        accumPQp += qpm;
}

double RateControl::rateEstimateQscale(LookaheadFrame * /*lframe*/)
{
    double q;
    // ratecontrol_entry_t rce = UNINIT(rce);
    int pictType = frameType;

    if (pictType == B_SLICE)
    {
        /* B-frames don't have independent ratecontrol, but rather get the
         * average QP of the two adjacent P-frames + an offset */

        int i0 = curFrame->getRefPic(REF_PIC_LIST_0, 0)->getSlice()->getSliceType() == I_SLICE;
        int i1 = curFrame->getRefPic(REF_PIC_LIST_1, 0)->getSlice()->getSliceType() == I_SLICE;
        int dt0 = abs(curFrame->getPOC() - curFrame->getRefPic(REF_PIC_LIST_0, 0)->getPOC());
        int dt1 = abs(curFrame->getPOC() - curFrame->getRefPic(REF_PIC_LIST_1, 0)->getPOC());

        //TODO:need to figure out this
        double q0 = curFrame->getRefPic(REF_PIC_LIST_0, 0)->getSlice()->getSliceQp();
        double q1 = curFrame->getRefPic(REF_PIC_LIST_1, 0)->getSlice()->getSliceQp();

        if (i0 && i1)
            q = (q0 + q1) / 2 + ipOffset;
        else if (i0)
            q = q1;
        else if (i1)
            q = q0;
        else
            q = (q0 * dt1 + q1 * dt0) / (dt0 + dt1);

        if (curFrame->isReferenced())
            q += pbOffset / 2;
        else
            q += pbOffset;

        return qp2qScale(q);
    }
    else
    {
        double abrBuffer = 2 * rateTolerance * bitrate;

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
        lastSatd = 0;     //TODO:need to get this from lookahead  //x264_rc_analyse_slice( h );
        rce->pCount = ncu;

        shortTermCplxSum *= 0.5;
        shortTermCplxCount *= 0.5;
        shortTermCplxSum += lastSatd / (CLIP_DURATION(frameDuration) / BASE_FRAME_DURATION);
        shortTermCplxCount++;
        rce->texBits = lastSatd;
        rce->blurredComplexity = shortTermCplxSum / shortTermCplxCount;
        rce->mvBits = 0;
        rce->pictType = pictType;
        //need to checked where it is initialized
        q = getQScale(wantedBitsWindow / cplxrSum);

        /* ABR code can potentially be counterproductive in CBR, so just don't bother.
         * Don't run it if the frame complexity is zero either. */
        if (lastSatd)
        {
            // TODO: need to check the thread_frames  - add after min chen plugs in frame parallelism.
            int iFrameDone = fps + 1 - 0;   //h->i_thread_frames;
            double timeDone = iFrameDone / fps;
            wantedBits = timeDone * bitrate;
            if (wantedBits > 0)
            {
                abrBuffer *= X265_MAX(1, sqrt(timeDone));
                overflow = Clip3(1.0 + (totalBits - wantedBits) / abrBuffer, .5, 2.0);
                q *= overflow;
            }
        }

        if (pictType == I_SLICE && keyFrameInterval > 1
            /* should test _next_ pict type, but that isn't decided yet */
            && lastNonBPictType != I_SLICE)
        {
            q = qp2qScale(accumPQp / accumPNorm);
            q /= fabs(ipFactor);
        }
        else if (fps > 0)
        {
            if (rateControlMode != X265_RC_CRF)
            {
                /* Asymmetric clipping, because symmetric would prevent
                 * overflow control in areas of rapidly oscillating complexity */
                double lqmin = lastQScaleFor[pictType] / lstep;
                double lqmax = lastQScaleFor[pictType] * lstep;
                if (overflow > 1.1 && fps > 3)
                    lqmax *= lstep;
                else if (overflow < 0.9)
                    lqmin /= lstep;

                q = Clip3(q, lqmin, lqmax);
            }
        }

        //FIXME use get_diff_limited_q() ?
        double lmin1 = lmin[pictType];
        double lmax1 = lmax[pictType];
        q = Clip3(q, lmin1, lmax1);
        lastQScaleFor[pictType] =
            lastQScale = q;

        if (!fps == 0)
            lastQScaleFor[P_SLICE] = q * fabs(ipFactor);

        return q;
    }
}

/**
 * modify the bitrate curve from pass1 for one frame
 */
double RateControl::getQScale(double rateFactor)
{
    double q;

    q = pow(rce->blurredComplexity, 1 - qCompress);

    // avoid NaN's in the rc_eq
    if (rce->texBits + rce->mvBits == 0)
        q = lastQScaleFor[rce->pictType];
    else
    {
        lastRceq = q;
        q /= rateFactor;
        lastQScale = q;
    }
    return q;
}

/* After encoding one frame, save stats and update ratecontrol state */
int RateControl::rateControlEnd(int64_t bits)
{
    if (rateControlMode ==  X265_RC_ABR)
    {
        if (frameType != B_SLICE)
            cplxrSum += bits * qp2qScale(qpaRc) / lastRceq;
        else
        {
            /* Depends on the fact that B-frame's QP is an offset from the following P-frame's.
             * Not perfectly accurate with B-refs, but good enough. */
            cplxrSum += bits * qp2qScale(qpaRc) / (lastRceq * fabs(pbFactor));
        }
        cplxrSum *= cbrDecay;
        wantedBitsWindow += frameDuration * bitrate;
        wantedBitsWindow *= cbrDecay;
    }
    totalBits += bits;
    delete rce;
    rce = NULL;
    return 0;
}
