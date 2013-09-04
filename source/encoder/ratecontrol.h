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

#include "TLibCommon/CommonDef.h"

namespace x265 {

struct Lowres;
class TComPic;

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
    RateControlEntry *rce;
    TComSlice *curFrame;        /* all info abt the current frame */
    SliceType frameType;        /* Current frame type */
    int ncu;                    /* number of CUs in a frame */
    int framerate;              /* current frame rate TODO: need to initialize in init */
    int frameThreads;
    int keyFrameInterval;       /* TODO: need to initialize in init */
    int qp;                     /* updated qp for current frame */
    int baseQp;                 /* CQP base QP */
    double frameDuration;        /* current frame duration in seconds */
    double qpm;                  /* qp for current macroblock: precise double for AQ */
    double qpaRc;                /* average of macroblocks' qp before aq */
    double bitrate;
    double rateTolerance;
    double qCompress;
    int    bframes;
    int    lastSatd;
    int    qpConstant[3];
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
    void rateControlStart(TComPic* pic, int lookAheadCost);  // to be called for each frame to process RateCOntrol and set QP
    int rateControlEnd(int64_t bits);
    double rateEstimateQscale(); // main logic for calculating QP based on ABR
    void accumPQpUpdate();
    double getQScale(double rateFactor);
};
}

#endif // ifndef __RATECONTROL__
