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

#ifndef __RATECONTROL__
#define __RATECONTROL__

#include <stdint.h>
#include "TLibEncoder/TEncTop.h"
#include "TLibCommon/TComRom.h"

#define BASE_FRAME_DURATION 0.04

/* Arbitrary limitations as a sanity check. */
#define MAX_FRAME_DURATION 1.00
#define MIN_FRAME_DURATION 0.01

#define CLIP_DURATION(f) Clip3(f, MIN_FRAME_DURATION, MAX_FRAME_DURATION)

namespace x265 {
struct RateControlEntry
{
    int pictType;
    int pCount;
    int newQp;
    int texBits;
    int mvBits;
    double blurredComplexity;
};

struct RateControl
{
    int ncu;                    /* number of CUs in a frame */
    TComSlice *curFrame;        /* all info abt the current frame */
    SliceType frameType;        /* Current frame type */
    double frameDuration;        /* current frame duration in seconds */
    int fps;                    /* current frame rate TODO: need to initaialize in init */
    int keyFrameInterval;       /* TODO: need to initialize in init */
    RateControlEntry *rce;
    int qp;                     /* updated qp for current frame */
    double qpm;                  /* qp for current macroblock: precise double for AQ */
    double qpaRc;                /* average of macroblocks' qp before aq */
    double bitrate;
    double rateTolerance;
    double qCompress;
    int bframes;
    int    lastSatd;
    double cplxrSum;           /* sum of bits*qscale/rceq */
    double wantedBitsWindow;  /* target bitrate * window */
    double ipOffset;
    double pbOffset;
    double ipFactor;
    double pbFactor;
    int lastNonBPictType;
    double accumPQp;          /* for determining I-frame quant */
    double accumPNorm;
    double lastQScale;
    double lastQScaleFor[3];  /* last qscale for a specific pict type, used for max_diff & ipb factor stuff */
    double lstep;
    double qpNoVbv;             /* QP for the current frame if 1-pass VBV was disabled. */
    double lastRceq;
    double cbrDecay;
    double lmin[3];             /* min qscale by frame type */
    double lmax[3];
    double shortTermCplxSum;
    double shortTermCplxCount;
    RcMethod rateControlMode;
    int64_t totalBits;   /* totalbits used for already encoded frames */

    RateControl(x265_param_t * param);    // constructor for initializing values for ratecontrol vars
    void rateControlInit(TComSlice* frame);   // to be called for each frame to set the reqired parameters for rateControl.
    void rateControlStart(LookaheadFrame* lframe);                          // to be called for each frame to process RateCOntrol and set QP
    int rateControlEnd(int64_t bits);
    double rateEstimateQscale(LookaheadFrame* lframe);                       // main logic for calculating QP based on ABR
    void accumPQpUpdate();
    double getQScale(RateControlEntry *rce, double rateFactor);

    double qScale2qp(double qScale)
    {
        return 12.0 + 6.0 * log(qScale / 0.85);
    }

    double qp2qScale(double qp)
    {
        return 0.85 * pow(2.0, (qp - 12.0) / 6.0);
    }
};
}

#endif // ifndef __RATECONTROL__
