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

#include "x265.h"
#include "TLibCommon/TComSlice.h"
#include "nal.h"

struct x265_encoder {};

namespace x265 {
// private namespace

struct EncStats
{
    double        m_psnrSumY;
    double        m_psnrSumU;
    double        m_psnrSumV;
    double        m_globalSsim;
    double        m_totalQp;
    uint64_t      m_accBits;
    uint32_t      m_numPics;

    EncStats()
    {
        m_psnrSumY = m_psnrSumU = m_psnrSumV = m_globalSsim = 0;
        m_accBits = 0;
        m_numPics = 0;
        m_totalQp = 0;
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
struct ThreadLocalData;

class Encoder : public x265_encoder
{
private:

    bool               m_aborted;          // fatal error detected
    int                m_pocLast;          ///< time index (POC)
    int                m_encodedFrameNum;
    int                m_outputCount;

    int                m_bframeDelay;
    int64_t            m_firstPts;
    int64_t            m_bframeDelayTime;
    int64_t            m_prevReorderedPts[2];

    ThreadPool*        m_threadPool;
    Lookahead*         m_lookahead;
    FrameEncoder*      m_frameEncoder;
    DPB*               m_dpb;

    Frame*             m_exportedPic;

    int                m_curEncoder;


    /* Collect statistics globally */
    EncStats           m_analyzeAll;
    EncStats           m_analyzeI;
    EncStats           m_analyzeP;
    EncStats           m_analyzeB;
    FILE*              m_csvfpt;
    int64_t            m_encodeStartTime;

    // quality control
    TComScalingList    m_scalingList;      ///< quantization matrix information

    // weighted prediction
    int                m_numLumaWPFrames;    // number of P frames with weighted luma reference
    int                m_numChromaWPFrames;  // number of P frames with weighted chroma reference
    int                m_numLumaWPBiFrames;  // number of B frames with weighted luma reference
    int                m_numChromaWPBiFrames; // number of B frames with weighted chroma reference

public:

    int                m_conformanceMode;
    TComVPS            m_vps;
    NALList            m_nalList;

    /* profile & level */
    Profile::Name      m_profile;
    Level::Tier        m_levelTier;
    Level::Name        m_level;

    bool               m_nonPackedConstraintFlag;
    bool               m_frameOnlyConstraintFlag;

    //====== Coding Structure ========
    int                m_maxDecPicBuffering[MAX_TLAYER];
    int                m_numReorderPics[MAX_TLAYER];
    int                m_maxRefPicNum;

    //======= Transform =============
    uint32_t           m_quadtreeTULog2MaxSize;
    uint32_t           m_quadtreeTULog2MinSize;

    //====== Loop/Deblock Filter ========
    bool               m_loopFilterOffsetInPPS;
    int                m_loopFilterBetaOffset;
    int                m_loopFilterBetaOffsetDiv2;
    int                m_loopFilterTcOffset;
    int                m_loopFilterTcOffsetDiv2;
    int                m_maxNumOffsetsPerPic;

    //====== Quality control ========
    int                m_maxCuDQPDepth;    //  Max. depth for a minimum CuDQP (0:default)

    //====== Tool list ========
    int                m_lastBPSEI;

    uint32_t           m_log2ParallelMergeLevelMinus2; ///< Parallel merge estimation region

    int                m_useScalingListId; ///< Using quantization matrix i.e. 0=off, 1=default.

    bool               m_transquantBypassEnableFlag;   ///< transquant_bypass_enable_flag setting in PPS.
    bool               m_cuTransquantBypassFlagValue;  ///< if transquant_bypass_enable_flag, the fixed value to use for the per-CU cu_transquant_bypass_flag.

    bool               m_neutralChromaIndicationFlag;
    bool               m_pocProportionalToTimingFlag;
    int                m_numTicksPocDiffOneMinus1;
    bool               m_tilesFixedStructureFlag;
    bool               m_motionVectorsOverPicBoundariesFlag;
    bool               m_restrictedRefPicListsFlag;
    int                m_minSpatialSegmentationIdc;
    int                m_maxBytesPerPicDenom;
    int                m_maxBitsPerMinCuDenom;
    int                m_log2MaxMvLengthHorizontal;
    int                m_log2MaxMvLengthVertical;

    x265_param*        m_param;
    RateControl*       m_rateControl;
    ThreadLocalData*   m_threadLocalData;

    bool               m_bEnableRDOQ;

    int                m_pad[2];
    Window             m_conformanceWindow;
    Window             m_defaultDisplayWindow;

    int                m_totalFrameThreads;

    uint32_t           m_numDelayedPic;

    Encoder();

    virtual ~Encoder();

    void create();
    void destroy();
    void init();

    void initSPS(TComSPS *sps);
    void initPPS(TComPPS *pps);

    int encode(const x265_picture* pic, x265_picture *pic_out);

    void getStreamHeaders();

    void fetchStats(x265_stats* stats, size_t statsSizeBytes);

    void writeLog(int argc, char **argv);

    void printSummary();

    char* statsString(EncStats&, char*);

    char* statsCSVString(EncStats& stat, char* buffer);

    TComScalingList* getScalingList() { return &m_scalingList; }

    void setThreadPool(ThreadPool* p) { m_threadPool = p; }

    void configure(x265_param *param);

    void updateVbvPlan(RateControl* rc);

protected:

    void finishFrameStats(Frame* pic, FrameEncoder *curEncoder, uint64_t bits);
};
}

#endif // ifndef X265_ENCODER_H
