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
 * For more information, contact us at licensing@multicorewareinc.com.
 *****************************************************************************/

#ifndef X265_ENCODER_H
#define X265_ENCODER_H

#include "x265.h"

#include "TLibEncoder/TEncCfg.h"
#include "TLibEncoder/TEncAnalyze.h"

#include "piclist.h"

struct x265_encoder {};

namespace x265 {
// private namespace

class FrameEncoder;
class DPB;
struct Lookahead;
struct RateControl;
class ThreadPool;
struct NALUnitEBSP;

class Encoder : public TEncCfg, public x265_encoder
{
private:

    int                m_pocLast;          ///< time index (POC)
    int                m_outputCount;
    PicList            m_freeList;

    ThreadPool*        m_threadPool;
    Lookahead*         m_lookahead;
    FrameEncoder*      m_frameEncoder;
    DPB*               m_dpb;
    RateControl*       m_rateControl;

    /* frame parallelism */
    int                m_curEncoder;

    /* Collect statistics globally */
    TEncAnalyze        m_analyzeAll;
    TEncAnalyze        m_analyzeI;
    TEncAnalyze        m_analyzeP;
    TEncAnalyze        m_analyzeB;
    double             m_globalSsim;
    FILE*              m_csvfpt;
    int64_t            m_encodeStartTime;

    // quality control
    TComScalingList    m_scalingList;      ///< quantization matrix information

    // weighted prediction
    int                m_numWPFrames;      // number of Unidirectional weighted frames used

public:

    x265_nal* m_nals;
    char*       m_packetData;

    Encoder();

    virtual ~Encoder();

    void create();
    void destroy();
    void init();

    void initSPS(TComSPS *sps);
    void initPPS(TComPPS *pps);

    int encode(bool bEos, const x265_picture* pic, x265_picture *pic_out, NALUnitEBSP **nalunits);

    int getStreamHeaders(NALUnitEBSP **nalunits);

    void fetchStats(x265_stats* stats);

    void writeLog(int argc, char **argv);

    void printSummary();

    TComScalingList* getScalingList() { return &m_scalingList; }

    void setThreadPool(ThreadPool* p) { m_threadPool = p; }

    void configure(x265_param *param);

    void determineLevelAndProfile(x265_param *param);

    int  extractNalData(NALUnitEBSP **nalunits);

protected:

    uint64_t calculateHashAndPSNR(TComPic* pic, NALUnitEBSP **nalunits); // Returns total number of bits for encoded pic
};
}

#endif // ifndef X265_ENCODER_H
