/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
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

#ifndef X265_ENCODER_H
#define X265_ENCODER_H

#include "common.h"
#include "slice.h"
#include "scalinglist.h"
#include "x265.h"
#include "nal.h"

struct x265_encoder {};

namespace X265_NS {
// private namespace
extern const char g_sliceTypeToChar[3];

class Entropy;

struct EncStats
{
    double        m_psnrSumY;
    double        m_psnrSumU;
    double        m_psnrSumV;
    double        m_globalSsim;
    double        m_totalQp;
    uint64_t      m_accBits;
    uint32_t      m_numPics;
    uint16_t      m_maxCLL;
    double        m_maxFALL;

    EncStats()
    {
        m_psnrSumY = m_psnrSumU = m_psnrSumV = m_globalSsim = 0;
        m_accBits = 0;
        m_numPics = 0;
        m_totalQp = 0;
        m_maxCLL = 0;
        m_maxFALL = 0;
    }

    void addQP(double aveQp);

    void addPsnr(double psnrY, double psnrU, double psnrV);

    void addBits(uint64_t bits);

    void addSsim(double ssim);
};

class FrameEncoder;
class DPB;
class Lookahead;
class RateControl;
class ThreadPool;

class Encoder : public x265_encoder
{
public:

    int                m_pocLast;         // time index (POC)
    int                m_encodedFrameNum;
    int                m_outputCount;

    int                m_bframeDelay;
    int64_t            m_firstPts;
    int64_t            m_bframeDelayTime;
    int64_t            m_prevReorderedPts[2];

    ThreadPool*        m_threadPool;
    FrameEncoder*      m_frameEncoder[X265_MAX_FRAME_THREADS];
    DPB*               m_dpb;

    Frame*             m_exportedPic;

    int                m_numPools;
    int                m_curEncoder;

    /* Collect statistics globally */
    EncStats           m_analyzeAll;
    EncStats           m_analyzeI;
    EncStats           m_analyzeP;
    EncStats           m_analyzeB;
    int64_t            m_encodeStartTime;

    // weighted prediction
    int                m_numLumaWPFrames;    // number of P frames with weighted luma reference
    int                m_numChromaWPFrames;  // number of P frames with weighted chroma reference
    int                m_numLumaWPBiFrames;  // number of B frames with weighted luma reference
    int                m_numChromaWPBiFrames; // number of B frames with weighted chroma reference
    FILE*              m_analysisFile;
    int                m_conformanceMode;
    VPS                m_vps;
    SPS                m_sps;
    PPS                m_pps;
    NALList            m_nalList;
    ScalingList        m_scalingList;      // quantization matrix information

    bool               m_emitCLLSEI;
    int                m_lastBPSEI;
    uint32_t           m_numDelayedPic;

    x265_param*        m_param;
    x265_param*        m_latestParam;
    RateControl*       m_rateControl;
    Lookahead*         m_lookahead;
    Window             m_conformanceWindow;

    bool               m_bZeroLatency;     // x265_encoder_encode() returns NALs for the input picture, zero lag
    bool               m_aborted;          // fatal error detected
    bool               m_reconfigured;      // reconfigure of encoder detected

    uint32_t           m_residualSumEmergency[MAX_NUM_TR_CATEGORIES][MAX_NUM_TR_COEFFS];
    uint16_t           (*m_offsetEmergency)[MAX_NUM_TR_CATEGORIES][MAX_NUM_TR_COEFFS];
    uint32_t           m_countEmergency[MAX_NUM_TR_CATEGORIES];

    Encoder();
    ~Encoder() {}

    void create();
    void stopJobs();
    void destroy();

    int encode(const x265_picture* pic, x265_picture *pic_out);

    int reconfigureParam(x265_param* encParam, x265_param* param);

    void getStreamHeaders(NALList& list, Entropy& sbacCoder, Bitstream& bs);

    void fetchStats(x265_stats* stats, size_t statsSizeBytes);

    void printSummary();

    char* statsString(EncStats&, char*);

    void configure(x265_param *param);

    void updateVbvPlan(RateControl* rc);

    void allocAnalysis(x265_analysis_data* analysis);

    void freeAnalysis(x265_analysis_data* analysis);

    void readAnalysisFile(x265_analysis_data* analysis, int poc);

    void writeAnalysisFile(x265_analysis_data* pic);

    void finishFrameStats(Frame* pic, FrameEncoder *curEncoder, uint64_t bits, x265_frame_stats* frameStats);

protected:

    void initVPS(VPS *vps);
    void initSPS(SPS *sps);
    void initPPS(PPS *pps);
};
}

#endif // ifndef X265_ENCODER_H
