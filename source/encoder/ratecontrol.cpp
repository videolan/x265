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

void RateControl::rateControlInit(TComSlice* frame, float curFrameDuration, x265_param_t *param)
{
    curFrame = curFrame;
    frameType = curFrame->getSliceType();
    frameNum = param->frameRate;
    keyFrameInterval = param->keyframeInterval;
    fps = param->frameRate;
    bframes = param->bframes;
}

void RateControl::rateControlStart(LookaheadFrame *lFrame)
{
    RateControlEntry *rce = new RateControlEntry();
    float q;

    //Always enabling ABR
    if (1) // rc->b_abr )
    {
        q = qScale2qp(rateEstimateQscale(lFrame));
    }
    q = Clip3(q, MIN_QP, MAX_QP);

    //rc.qpa_rc = rc.qpa_rc_prev =
    //rc.qpa_aq = rc.qpa_aq_prev = 0;
    qp = x265_clip3(q + 0.5f, 0, MAX_QP);

    qpm = q;
    if (rce)
        rce->new_qp = qp;

    accum_p_qp_update();

    if (frameType != B_SLICE)
        last_non_b_pict_type = frameType;
}

void RateControl::accum_p_qp_update()
{
    //x264_ratecontrol_t *rc = h->rc;
    accum_p_qp   *= .95;
    accum_p_norm *= .95;
    accum_p_norm += 1;
    if (frameType == I_SLICE)
        accum_p_qp += qpm + ip_offset;
    else
        accum_p_qp += qpm;
}

float RateControl::rateEstimateQscale(LookaheadFrame *lframe)
{
    float q;
    // ratecontrol_entry_t rce = UNINIT(rce);
    int pict_type = frameType;
    int64_t total_bits = 0; //CHECK:for ABR

    if (pict_type == B_SLICE)
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
            q = (q0 + q1) / 2 + ip_offset;
        else if (i0)
            q = q1;
        else if (i1)
            q = q0;
        else
            q = (q0 * dt1 + q1 * dt0) / (dt0 + dt1);

        if (h->fenc->b_kept_as_ref)
            q += pb_offset / 2;
        else
            q += pb_offset;

        qp_novbv = q;
        return qp2qScale(q);
    }
    else
    {
        double abr_buffer = 2 * rate_tolerance * bitrate;

        /* 1pass ABR */
        {
            /* Calculate the quantizer which would have produced the desired
             * average bitrate if it had been applied to all frames so far.
             * Then modulate that quant based on the current frame's complexity
             * relative to the average complexity so far (using the 2pass RCEQ).
             * Then bias the quant up or down if total size so far was far from
             * the target.
             * Result: Depending on the value of rate_tolerance, there is a
             * tradeoff between quality and bitrate precision. But at large
             * tolerances, the bit distribution approaches that of 2pass. */

            double wanted_bits, overflow = 1;

            last_satd = 0; //need to get this from lookahead  //x264_rc_analyse_slice( h );
            short_term_cplxsum *= 0.5;
            short_term_cplxcount *= 0.5;
            //TODO:need to get the duration for each frame
            //short_term_cplxsum += last_satd / (CLIP_DURATION(h->fenc->f_duration) / BASE_FRAME_DURATION);
            short_term_cplxcount++;

            rce->p_count = ncu;

            rce->pict_type = pict_type;

            {
                //TODO:wanted_bits_window, fps , h->fenc->i_reordered_pts, h->i_reordered_pts_delay, h->fref_nearest[0]->f_qp_avg_rc, h->param.rc.f_ip_factor, h->sh.i_type
                //need to checked where it is initialized
                q = getQScale(rce, wanted_bits_window / cplxr_sum, frameNum);

                /* ABR code can potentially be counterproductive in CBR, so just don't bother.
                 * Don't run it if the frame complexity is zero either. */
                if ( /*!rcc->b_vbv_min_rate && */ last_satd)
                {
                    // TODO: need to check the thread_frames
                    int i_frame_done = frameNum + 1 - h->i_thread_frames;
                    double time_done = i_frame_done / fps;
                    if (i_frame_done > 0)
                    {
                        //time_done = ((double)(h->fenc->i_reordered_pts - h->i_reordered_pts_delay)) * h->param.i_timebase_num / h->param.i_timebase_den;
                        time_done = ((double)(h->fenc->i_reordered_pts - h->i_reordered_pts_delay)) * (1 / fps);
                    }
                    wanted_bits = time_done * bitrate;
                    if (wanted_bits > 0)
                    {
                        abr_buffer *= X265_MAX(1, sqrt(time_done));
                        overflow = x265_clip3f(1.0 + (total_bits - wanted_bits) / abr_buffer, .5, 2);
                        q *= overflow;
                    }
                }
            }

            if (pict_type == I_SLICE && keyFrameInterval > 1
                /* should test _next_ pict type, but that isn't decided yet */
                && last_non_b_pict_type != I_SLICE)
            {
                q = qp2qScale(accum_p_qp / accum_p_norm);
                q /= fabs(h->param.rc.f_ip_factor);
            }
            else if (frameNum > 0)
            {
                if (1) //assume that for ABR this is always enabled h->param.rc.i_rc_method != X264_RC_CRF )
                {
                    /* Asymmetric clipping, because symmetric would prevent
                     * overflow control in areas of rapidly oscillating complexity */
                    double lmin = last_qscale_for[pict_type] / lstep;
                    double lmax = last_qscale_for[pict_type] * lstep;
                    if (overflow > 1.1 && frameNum > 3)
                        lmax *= lstep;
                    else if (overflow < 0.9)
                        lmin /= lstep;

                    q = x265_clip3f(q, lmin, lmax);
                }
            }

            qp_novbv = qScale2qp(q);

            //FIXME use get_diff_limited_q() ?
            //q = clip_qscale( h, pict_type, q );
            double lmin_1 = lmin[pict_type];
            double lmax_1 = lmax[pict_type];
            x265_clip3f(q, lmin_1, lmax_1);
        }

        last_qscale_for[pict_type] =
            last_qscale = q;

        if (!frameNum == 0)
            last_qscale_for[P_SLICE] = q * fabs(h->param.rc.f_ip_factor);

        //this is for VBV stuff
        //frame_size_planned = predict_size( &pred[h->sh.i_type], q, last_satd );

        return q;
    }
}

/**
 * modify the bitrate curve from pass1 for one frame
 */
double RateControl::getQScale(RateControlEntry *rce, double rate_factor, int frame_num)
{
    double q;

    //x264_zone_t *zone = get_zone( h, frame_num );

    q = pow(rce->blurred_complexity, 1 - qcompress);

    // avoid NaN's in the rc_eq
    if (rce->tex_bits + rce->mv_bits == 0)
        q = last_qscale_for[rce->pict_type];
    else
    {
        last_rceq = q;
        q /= rate_factor;
        last_qscale = q;
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
