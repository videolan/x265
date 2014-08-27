/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Shin Yee <shinyee@multicorewareinc.com>
 *          Min Chen <chenm003@163.com>
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

#ifndef X265_FRAMEENCODER_H
#define X265_FRAMEENCODER_H

#include "common.h"
#include "wavefront.h"
#include "bitstream.h"
#include "frame.h"

#include "analysis.h"
#include "sao.h"

#include "entropy.h"
#include "framefilter.h"
#include "cturow.h"
#include "ratecontrol.h"
#include "reference.h"
#include "nal.h"

namespace x265 {
// private x265 namespace

class ThreadPool;
class Encoder;

/* Current frame stats for 2 pass */
struct FrameStats
{
    /* MV bits (MV+Ref+Block Type) */
    int         mvBits;
    /* Texture bits (DCT coefs) */
    int         coeffBits;
    int         miscBits;
    /* CU type counts stored as percentage */
    double      cuCount_i;
    double      cuCount_p;
    double      cuCount_skip;
};

// Manages the wave-front processing of a single encoding frame
class FrameEncoder : public WaveFront, public Thread
{
public:

    FrameEncoder();

    virtual ~FrameEncoder() {}

    bool init(Encoder *top, int numRows, int numCols);

    void destroy();

    /* triggers encode of a new frame by the worker thread */
    void startCompressFrame(Frame* pic);

    /* blocks until worker thread is done, returns access unit */
    Frame *getEncodedPicture(NALList& list);

    Event                    m_enable;
    Event                    m_done;
    bool                     m_threadActive;

    int                      m_numRows;
    uint32_t                 m_numCols;
    int                      m_refLagRows;
    CTURow*                  m_rows;
    RateControlEntry         m_rce;
    SEIDecodedPictureHash    m_seiReconPictureDigest;

    uint64_t                 m_SSDY;
    uint64_t                 m_SSDU;
    uint64_t                 m_SSDV;
    double                   m_ssim;
    uint32_t                 m_ssimCnt;
    MD5Context               m_state[3];
    uint32_t                 m_crc[3];
    uint32_t                 m_checksum[3];
    double                   m_elapsedCompressTime; // elapsed time spent in worker threads
    double                   m_frameTime;           // wall time from frame start to finish
    FrameStats               m_frameStats;          // stats of current frame for multipass encodes
    volatile bool            m_bAllRowsStop;
    volatile int             m_vbvResetTriggerRow;
    uint64_t                 m_accessUnitBits;

    Encoder*                 m_top;
    x265_param*              m_param;
    Frame*                   m_frame;

    MotionReference          m_mref[2][MAX_NUM_REF + 1];
    Entropy                  m_entropyCoder;
    Entropy                  m_initSliceContext;
    FrameFilter              m_frameFilter;
    Bitstream                m_bs;
    Bitstream*               m_outStreams;
    uint32_t*                m_substreamSizes;
    NoiseReduction*          m_nr;
    NALList                  m_nalList;
    ThreadLocalData          m_tld; /* for --no-wpp */

    int                      m_filterRowDelay;
    int                      m_filterRowDelayCus;
    Event                    m_completionEvent;
    int64_t                  m_totalTime;

protected:

    /* analyze / compress frame, can be run in parallel within reference constraints */
    void compressFrame();

    /* called by compressFrame to perform wave-front compression analysis */
    void compressCTURows();

    /* called by compressFrame to generate final per-row bitstreams */
    void encodeSlice();

    void threadMain();
    void setLambda(int qp, ThreadLocalData& tld);
    int calcQpForCu(uint32_t cuAddr, double baseQp);
    void noiseReductionUpdate();

    /* Called by WaveFront::findJob() */
    void processRow(int row, int threadId);
    void processRowEncoder(int row, ThreadLocalData& tld);
    void processRowFilter(int row, ThreadLocalData& tld) { m_frameFilter.processRow(row, tld); }

    void enqueueRowEncoder(int row) { WaveFront::enqueueRow(row * 2 + 0); }
    void enqueueRowFilter(int row)  { WaveFront::enqueueRow(row * 2 + 1); }
    void enableRowEncoder(int row)  { WaveFront::enableRow(row * 2 + 0); }
    void enableRowFilter(int row)   { WaveFront::enableRow(row * 2 + 1); }
};
}

#endif // ifndef X265_FRAMEENCODER_H
