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

#ifndef X265_RATECONTROL_H
#define X265_RATECONTROL_H

#include "TLibCommon/CommonDef.h"

namespace x265 {
// encoder namespace

struct Lookahead;
class Encoder;
class TComPic;
class TEncCfg;

#define BASE_FRAME_DURATION 0.04

/* Arbitrary limitations as a sanity check. */
#define MAX_FRAME_DURATION 1.00
#define MIN_FRAME_DURATION 0.01

#define CLIP_DURATION(f) Clip3(MIN_FRAME_DURATION, MAX_FRAME_DURATION, f)

struct RateControlEntry
{
    int sliceType;
    int texBits;
    int mvBits;
    double blurredComplexity;
    double qpaRc;
    double qRceq;

    int lastSatd;
    bool bLastMiniGopBFrame;
    double frameSizePlanned;
    double bufferRate;
    int bframes;
    int poc;
};

struct Predictor
{
    double coeff;
    double count;
    double decay;
    double offset;
};

struct RateControl
{
    TComSlice *curSlice;      /* all info about the current frame */
    TEncCfg *cfg;
    SliceType sliceType;      /* Current frame type */
    int ncu;                  /* number of CUs in a frame */
    int keyFrameInterval;     /* TODO: need to initialize in init */
    int qp;                   /* updated qp for current frame */
    int baseQp;               /* CQP base QP */
    double frameDuration;     /* current frame duration in seconds */
    double bitrate;
    double rateFactorConstant;
    bool   isAbr;

    double bufferSize;
    double bufferFillFinal;  /* real buffer as of the last finished frame */
    double bufferFill;       /* planned buffer, if all in-progress frames hit their bit budget */
    double bufferRate;       /* # of bits added to buffer_fill after each frame */
    double vbvMaxRate;       /* in kbps */
    bool singleFrameVbv;
    bool isVbv;
    double fps;
    Predictor pred[5];
    Predictor predBfromP;
    int bframes;
    int bframeBits;
    double leadingNoBSatd;

    int    lastSatd;
    int    qpConstant[3];
    double cplxrSum;          /* sum of bits*qscale/rceq */
    double wantedBitsWindow;  /* target bitrate * window */
    double ipOffset;
    double pbOffset;
    int lastNonBPictType;
    double accumPQp;          /* for determining I-frame quant */
    double accumPNorm;
    double lastQScaleFor[3];  /* last qscale for a specific pict type, used for max_diff & ipb factor stuff */
    double lstep;
    double lmin[3];           /* min qscale by frame type */
    double lmax[3];
    double shortTermCplxSum;
    double shortTermCplxCount;
    int64_t totalBits;        /* totalbits used for already encoded frames */
    double lastRceq;
    int framesDone;           /* framesDone keeps track of # of frames passed through RateCotrol already */
    double qCompress;
    RateControl(TEncCfg * _cfg);

    // to be called for each frame to process RateControl and set QP
    void rateControlStart(TComPic* pic, Lookahead *, RateControlEntry* rce, Encoder* enc);
    void calcAdaptiveQuantFrame(TComPic *pic);
    int rateControlEnd(int64_t bits, RateControlEntry* rce);

protected:

    double getQScale(RateControlEntry *rce, double rateFactor);
    double rateEstimateQscale(RateControlEntry *rce); // main logic for calculating QP based on ABR
    void accumPQpUpdate();
    uint32_t acEnergyCu(TComPic* pic, uint32_t block_x, uint32_t block_y);

    void updateVbv(int64_t bits, RateControlEntry* rce);
    void updatePredictor(Predictor *p, double q, double var, double bits);
    double clipQscale(double q);
    void updateVbvPlan(Encoder* enc);
    double predictSize(Predictor *p, double q, double var);
};
}

#endif // ifndef X265_RATECONTROL_H
