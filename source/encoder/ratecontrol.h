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

#ifndef X265_RATECONTROL_H
#define X265_RATECONTROL_H

namespace x265 {
// encoder namespace

class Lookahead;
class Encoder;
class TComPic;
class TComSPS;
class SEIBufferingPeriod;

#define BASE_FRAME_DURATION 0.04

/* Arbitrary limitations as a sanity check. */
#define MAX_FRAME_DURATION 1.00
#define MIN_FRAME_DURATION 0.01

#define CLIP_DURATION(f) Clip3(MIN_FRAME_DURATION, MAX_FRAME_DURATION, f)

struct Predictor
{
    double coeff;
    double count;
    double decay;
    double offset;
};

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
    double frameSizePlanned;  /* frame Size decided by RateCotrol before encoding the frame */
    double bufferRate;
    double movingAvgSum;
    double qpNoVbv;
    double bufferFill;
    Predictor rowPreds[3][2];
    Predictor* rowPred[2];
    double frameSizeEstimated;  /* hold frameSize, updated from cu level vbv rc */
    bool isActive;
};

class RateControl
{
public:

    TComSlice*  m_curSlice;      /* all info about the current frame */
    x265_param* m_param;
    SliceType   m_sliceType;     /* Current frame type */
    int         m_ncu;           /* number of CUs in a frame */
    int         m_qp;            /* updated qp for current frame */

    bool   m_isAbr;
    bool   m_isVbv;
    bool   m_isCbr;
    bool   m_singleFrameVbv;

    bool   m_isAbrReset;
    int    m_lastAbrResetPoc;

    double m_frameDuration;     /* current frame duration in seconds */
    double m_bitrate;
    double m_rateFactorConstant;
    double m_bufferSize;
    double m_bufferFillFinal;  /* real buffer as of the last finished frame */
    double m_bufferFill;       /* planned buffer, if all in-progress frames hit their bit budget */
    double m_bufferRate;       /* # of bits added to buffer_fill after each frame */
    double m_vbvMaxRate;       /* in kbps */
    double m_rateFactorMaxIncrement; /* Don't allow RF above (CRF + this value). */
    double m_rateFactorMaxDecrement; /* don't allow RF below (this value). */

    Predictor m_pred[5];
    Predictor m_predBfromP;

    int       m_bframes;
    int       m_bframeBits;
    int64_t   m_currentSatd;
    int       m_qpConstant[3];
    double    m_ipOffset;
    double    m_pbOffset;

    int      m_lastNonBPictType;
    int64_t  m_leadingNoBSatd;

    double   m_cplxrSum;          /* sum of bits*qscale/rceq */
    double   m_wantedBitsWindow;  /* target bitrate * window */
    double   m_accumPQp;          /* for determining I-frame quant */
    double   m_accumPNorm;
    double   m_lastQScaleFor[3];  /* last qscale for a specific pict type, used for max_diff & ipb factor stuff */
    double   m_lstep;
    double   m_shortTermCplxSum;
    double   m_shortTermCplxCount;
    double   m_lastRceq;
    double   m_qCompress;

    int64_t  m_totalBits;        /* total bits used for already encoded frames */
    int      m_framesDone;       /* # of frames passed through RateCotrol already */

    /* hrd stuff */
    SEIBufferingPeriod m_sei;
    double   m_nominalRemovalTime;
    double   m_prevCpbFinalAT;

    RateControl(x265_param *p);

    // to be called for each frame to process RateControl and set QP
    void rateControlStart(TComPic* pic, Lookahead *, RateControlEntry* rce, Encoder* enc);
    void calcAdaptiveQuantFrame(TComPic *pic);
    int rateControlEnd(TComPic* pic, int64_t bits, RateControlEntry* rce);
    int rowDiagonalVbvRateControl(TComPic* pic, uint32_t row, RateControlEntry* rce, double& qpVbv);

    void hrdFullness(SEIBufferingPeriod* sei);
    void init(TComSPS* sps);
    void initHRD(TComSPS* sps);

protected:

    static const double s_amortizeFraction;
    static const int    s_amortizeFrames;

    int m_residualFrames;
    int m_residualCost;

    double getQScale(RateControlEntry *rce, double rateFactor);
    double rateEstimateQscale(TComPic* pic, RateControlEntry *rce); // main logic for calculating QP based on ABR
    void accumPQpUpdate();
    uint32_t acEnergyCu(TComPic* pic, uint32_t block_x, uint32_t block_y);

    void updateVbv(int64_t bits, RateControlEntry* rce);
    void updatePredictor(Predictor *p, double q, double var, double bits);
    double clipQscale(TComPic* pic, double q);
    void updateVbvPlan(Encoder* enc);
    double predictSize(Predictor *p, double q, double var);
    void checkAndResetABR(RateControlEntry* rce, bool isFrameDone);
    double predictRowsSizeSum(TComPic* pic, RateControlEntry* rce, double qpm, int32_t& encodedBits);
};
}
#endif // ifndef X265_RATECONTROL_H
