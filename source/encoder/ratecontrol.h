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

#ifndef __RATECONTROL__
#define __RATECONTROL__

#include <stdint.h>
#include "TLibEncoder/TEncTop.h"

#include "math.h"

namespace x265 {

struct RateControlEntry
{
    int pict_type;
    int p_count;
    int new_qp;
    int tex_bits;
    int mv_bits;
    float blurred_complexity;
};

struct Predictor
{
    float coeff;
    float count;
    float decay;
    float offset;

    float predictSize(float q, float var)
    {
        return (coeff * var + offset) / (q * count);
    }
};

struct RateControl
{
    int ncu;                    /* number of CUs in a frame */
    TComSlice *curFrame;        /* all info abt the current frame */
    SliceType frameType;        /* Current frame type */
    float frameDuration;        /* current frame duration in seconds */
    int frameNum;               /* current frame number TODO: need to initaialize in init */
    int keyFrameInterval;       /* TODO: need to initialize in init */

    /* current frame */
    RateControlEntry *rce;
    int qp;                     /* updated qp for current frame */
    float qpm;                  /* qp for current macroblock: precise float for AQ */

    double bitrate;
    double rate_tolerance;
    double qcompress;

    /* ABR stuff */
    int    last_satd;
    double last_rceq;
    double cplxr_sum;           /* sum of bits*qscale/rceq */
    double expected_bits_sum;   /* sum of qscale2bits after rceq, ratefactor, and overflow, only includes finished frames */
    int64_t filler_bits_sum;    /* sum in bits of finished frames' filler data */
    double wanted_bits_window;  /* target bitrate * window */
    double cbr_decay;
    double short_term_cplxsum;
    double short_term_cplxcount;
    double rate_factor_constant;
    double ip_offset;
    double pb_offset;

    int last_non_b_pict_type;
    double accum_p_qp;          /* for determining I-frame quant */
    double accum_p_norm;
    double last_qscale;
    double last_qscale_for[3];  /* last qscale for a specific pict type, used for max_diff & ipb factor stuff */

    double fps;
    double lstep;
    float qp_novbv;             /* QP for the current frame if 1-pass VBV was disabled. */
    double lmin[3];             /* min qscale by frame type */
    double lmax[3];
    double frame_size_planned;

    void rateControlInit(TComSlice* frame, float dur, x265_param_t *param); // to be called for each frame to set the reqired parameters for rateControl.
    void rateControlStart(LookaheadFrame* lframe);                          // to be called for each frame to process RateCOntrol and set QP
    float rateEstimateQscale(LookaheadFrame* lframe);                       // main logic for calculating QP based on ABR
    void accum_p_qp_update();
    double getQScale(RateControlEntry *rce, double rate_factor, int frame_num);

    float qScale2qp(float qscale)
    {
        return 12.0f + 6.0f * logf(qscale / 0.85f);
    }

    float qp2qScale(float qp)
    {
        return 0.85f * powf(2.0f, (qp - 12.0f) / 6.0f);
    }
};
}

#endif // ifndef __RATECONTROL__
