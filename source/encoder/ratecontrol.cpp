/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Sumalatha <sumalatha@multicorewareinc.com>
 *          Aarthi <aarthi@multicorewareinc.com>
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
    keyFrameInterval = param->keyframeInterval;
    fps = param->frameRate;
    bframes = param->bframes;
    //TODO : set default value here,later can get from  param as done in x264?
    rateTolerance = 1;
    //TODO : introduce bitrate var in param and cli options
    bitrate = param->bitrate * 1000;
    fps = param->frameRate;
    isAbrEnabled = true; // set from param later.
    ncu = (param->sourceHeight * param->sourceWidth) / pow(2, param->maxCUSize);
    lastNonBPictType = -1;
    //TODO : need  to confirm default val if mbtree is disabled
    qCompress = 1;
    if (isAbrEnabled)
    {
        //TODO : confirm this value. obtained it from x264 when crf is disabled , abr enabled.
#define ABR_INIT_QP (24  + 6 * (param->internalBitDepth - 8));
        accumPNorm = .01;
        accumPQp = (ABR_INIT_QP)*accumPNorm;
        /* estimated ratio that produces a reasonable QP for the first I-frame */
        cplxrSum = .01 * pow(7.0e5, qCompress) * pow(ncu, 0.5);
        wantedBitsWindow = 1.0 * bitrate / fps;
        lastNonBPictType = I_SLICE;
    }
}

void RateControl::rateControlInit(TComSlice* frame)
{
    curFrame = curFrame;
    frameType = curFrame->getSliceType();
}

void RateControl::rateControlStart(LookaheadFrame *lFrame)
{
    RateControlEntry *rce = new RateControlEntry();
    float q;

    //Always enabling ABR
    if (isAbrEnabled)
    {
        q = qScale2qp(rateEstimateQscale(lFrame));
    }
    q = Clip3((int)q, MIN_QP, MAX_QP);

    qp = Clip3((int)(q + 0.5f), 0, MAX_QP);

    qpm = q;
    if (rce)
        rce->newQp = qp;

    accumPQpUpdate();

    if (frameType != B_SLICE)
        lastNonBPictType = frameType;
}

void RateControl::accumPQpUpdate()
{
    //x264_ratecontrol_t *rc = h->rc;
    accumPQp   *= .95;
    accumPNorm *= .95;
    accumPNorm += 1;
    if (frameType == I_SLICE)
        accumPQp += qpm + ipOffset;
    else
        accumPQp += qpm;
}

float RateControl::rateEstimateQscale(LookaheadFrame *lframe)
{
    float q;
    // ratecontrol_entry_t rce = UNINIT(rce);
    int pictType = frameType;
    int64_t totalBits = 0; //CHECK:for ABR

    if (pictType == B_SLICE)
    {
        /* B-frames don't have independent ratecontrol, but rather get the
         * average QP of the two adjacent P-frames + an offset */

        int i0 = curFrame->getRefPic(REF_PIC_LIST_0, 0)->getSlice()->getSliceType() == I_SLICE;
        int i1 = curFrame->getRefPic(REF_PIC_LIST_1, 0)->getSlice()->getSliceType() == I_SLICE;
        int dt0 = abs(curFrame->getPOC() - curFrame->getRefPic(REF_PIC_LIST_0, 0)->getPOC());
        int dt1 = abs(curFrame->getPOC() - curFrame->getRefPic(REF_PIC_LIST_1, 0)->getPOC());

        //TODO:need to figure out this
        float q0 = h->fref_nearest[0]->f_qp_avg_rc;
        float q1 = h->fref_nearest[1]->f_qp_avg_rc;

        if (i0 && i1)
            q = (q0 + q1) / 2 + ipOffset;
        else if (i0)
            q = q1;
        else if (i1)
            q = q0;
        else
            q = (q0 * dt1 + q1 * dt0) / (dt0 + dt1);

        if (h->fenc->b_kept_as_ref)
            q += pbOffset / 2;
        else
            q += pbOffset;

        qpNoVbv = q;
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

        lastSatd = 0;     //need to get this from lookahead  //x264_rc_analyse_slice( h );
        shortTermCplxSum *= 0.5;
        shortTermCplxCount *= 0.5;
        //TODO:need to get the duration for each frame
        //short_term_cplxsum += last_satd / (CLIP_DURATION(h->fenc->f_duration) / BASE_FRAME_DURATION);
        shortTermCplxCount++;

        rce->pCount = ncu;

        rce->pictType = pictType;

        //TODO:wanted_bits_window, fps , h->fenc->i_reordered_pts, h->i_reordered_pts_delay, h->fref_nearest[0]->f_qp_avg_rc, h->param.rc.f_ip_factor, h->sh.i_type
        //need to checked where it is initialized
        q = getQScale(rce, wantedBitsWindow / cplxrSum, fps);

        /* ABR code can potentially be counterproductive in CBR, so just don't bother.
         * Don't run it if the frame complexity is zero either. */
        if ( /*!rcc->b_vbv_min_rate && */ lastSatd)
        {
            // TODO: need to check the thread_frames
            int iFrameDone = fps + 1 - h->i_thread_frames;
            double timeDone = iFrameDone / fps;
            if (iFrameDone > 0)
            {
                //time_done = ((double)(h->fenc->i_reordered_pts - h->i_reordered_pts_delay)) * h->param.i_timebase_num / h->param.i_timebase_den;
                timeDone = ((double)(h->fenc->i_reordered_pts - h->i_reordered_pts_delay)) * (1 / fps);
            }
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
            q /= fabs(h->param.rc.f_ip_factor);
        }
        else if (fps > 0)
        {
            if (1)     //assume that for ABR this is always enabled h->param.rc.i_rc_method != X264_RC_CRF )
            {
                /* Asymmetric clipping, because symmetric would prevent
                 * overflow control in areas of rapidly oscillating complexity */
                float lmin = lastQScaleFor[pictType] / lstep;
                float lmax = lastQScaleFor[pictType] * lstep;
                if (overflow > 1.1 && fps > 3)
                    lmax *= lstep;
                else if (overflow < 0.9)
                    lmin /= lstep;

                q = Clip3(q, lmin, lmax);
            }
        }

        qpNoVbv = qScale2qp(q);

        //FIXME use get_diff_limited_q() ?
        //q = clip_qscale( h, pict_type, q );
        float lmin1 = lmin[pictType];
        float lmax1 = lmax[pictType];
        Clip3(q, lmin1, lmax1);

        lastQScaleFor[pictType] =
            lastQScale = q;

        if (!fps == 0)
            lastQScaleFor[P_SLICE] = q * fabs(h->param.rc.f_ip_factor);

        //this is for VBV stuff
        //frame_size_planned = predict_size( &pred[h->sh.i_type], q, last_satd );

        return q;
    }
}

/**
 * modify the bitrate curve from pass1 for one frame
 */
double RateControl::getQScale(RateControlEntry *rce, double rateFactor)
{
    double q;

    //x264_zone_t *zone = get_zone( h, frame_num );

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

    /*TODO: need to check zone is required */
    //if( zone )
    //{
    //    if( zone->b_force_qp )
    //        q = qp2qscale( zone->i_qp );
    //    else
    //        q /= zone->f_bitrate_factor;
    //}

    return q;
}
