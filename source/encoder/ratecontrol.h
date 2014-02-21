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
    int64_t texBits;  /* Required in 2-pass rate control */
    int64_t lastSatd; /* Contains the picture cost of the previous frame, required for resetAbr and VBV */

    int sliceType;
    int mvBits;
    int bframes;
    int poc;
    int64_t leadingNoBSatd;
    bool bLastMiniGopBFrame;
    double blurredComplexity;
    double qpaRc;
    double qRceq;
    double frameSizePlanned;
    double bufferRate;
    double movingAvgSum;
    double qpNoVbv;
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

    double frameDuration;     /* current frame duration in seconds */
    double bitrate;
    double rateFactorConstant;
    bool   isAbr;
    double bufferSize;
    double bufferFillFinal;  /* real buffer as of the last finished frame */
    double bufferFill;       /* planned buffer, if all in-progress frames hit their bit budget */
    double bufferRate;       /* # of bits added to buffer_fill after each frame */
    double vbvMaxRate;       /* in kbps */
    double vbvMinRate;       /* in kbps */
    bool singleFrameVbv;
    double rateFactorMaxIncrement; /* Don't allow RF above (CRF + this value). */
    bool isVbv;
    Predictor pred[5];
    Predictor predBfromP;
    Predictor rowPreds[4];
    Predictor *rowPred[2];
    int bframes;
    int bframeBits;
    bool isAbrReset;
    int lastAbrResetPoc;
    int64_t currentSatd;
    int    qpConstant[3];
    double cplxrSum;          /* sum of bits*qscale/rceq */
    double wantedBitsWindow;  /* target bitrate * window */
    double ipOffset;
    double pbOffset;
    int lastNonBPictType;
    int64_t leadingNoBSatd;
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
    double qpNoVbv;             /* QP for the current frame if 1-pass VBV was disabled. */
    double frameSizeEstimated;  /* hold synched frameSize, updated from cu level vbv rc */
    RateControl(TEncCfg * _cfg);

    // to be called for each frame to process RateControl and set QP
    void rateControlStart(TComPic* pic, Lookahead *, RateControlEntry* rce, Encoder* enc);
    void calcAdaptiveQuantFrame(TComPic *pic);
    int rateControlEnd(TComPic* pic, int64_t bits, RateControlEntry* rce);
    int rowDiagonalVbvRateControl(TComPic* pic, uint32_t row, RateControlEntry* rce, double& qpVbv);

protected:
    double getQScale(RateControlEntry *rce, double rateFactor);
    double rateEstimateQscale(TComPic* pic, RateControlEntry *rce); // main logic for calculating QP based on ABR
    void accumPQpUpdate();
    uint32_t acEnergyCu(TComPic* pic, uint32_t block_x, uint32_t block_y);

    void updateVbv(int64_t bits, RateControlEntry* rce);
    void updatePredictor(Predictor *p, double q, double var, double bits);
    double clipQscale(TComPic* pic, double q);
    void updateVbvPlan(Encoder* enc);
    double predictSize(Predictor *p, double q, double var);
    void checkAndResetABR(TComPic* pic, RateControlEntry* rce);
    double predictRowsSizeSum(TComPic* pic, double qpm , int32_t& encodedBits);
};
}
#endif // ifndef X265_RATECONTROL_H
