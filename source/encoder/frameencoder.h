/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Chung Shin Yee <shinyee@multicorewareinc.com>
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
 * For more information, contact us at licensing@multicorewareinc.com.
 *****************************************************************************/

#ifndef X265_FRAMEENCODER_H
#define X265_FRAMEENCODER_H

#include "TLibCommon/TComBitCounter.h"
#include "TLibCommon/TComPic.h"
#include "TLibCommon/AccessUnit.h"

#include "TLibEncoder/TEncCu.h"
#include "TLibEncoder/TEncSearch.h"
#include "TLibEncoder/TEncSbac.h"
#include "TLibEncoder/TEncBinCoderCABAC.h"
#include "TLibEncoder/WeightPredAnalysis.h"
#include "TLibEncoder/TEncSampleAdaptiveOffset.h"
#include "TLibEncoder/SEIwrite.h"
#include "TLibEncoder/TEncCavlc.h"

#include "wavefront.h"
#include "framefilter.h"
#include "cturow.h"
#include "ratecontrol.h"

namespace x265 {
// private x265 namespace

class ThreadPool;
class TEncTop;

// Manages the wave-front processing of a single encoding frame
class FrameEncoder : public WaveFront, public Thread
{
public:

    FrameEncoder();

    virtual ~FrameEncoder() {}

    void setThreadPool(ThreadPool *p);

    void init(TEncTop *top, int numRows);

    void destroy();

    void processRow(int row);

    TEncEntropy* getEntropyCoder(int row)      { return &this->m_rows[row].m_entropyCoder; }

    TEncSbac*    getSbacCoder(int row)         { return &this->m_rows[row].m_sbacCoder; }

    TEncSbac*    getRDGoOnSbacCoder(int row)   { return &this->m_rows[row].m_rdGoOnSbacCoder; }

    TEncSbac*    getBufferSBac(int row)        { return &this->m_rows[row].m_bufferSbacCoder; }

    TEncCu*      getCuEncoder(int row)         { return &this->m_rows[row].m_cuCoder; }

    /* Frame singletons, last the life of the encoder */
    TEncSampleAdaptiveOffset* getSAO()         { return &m_frameFilter.m_sao; }

    void resetEntropy(TComSlice *slice)
    {
        for (int i = 0; i < this->m_numRows; i++)
        {
            this->m_rows[i].m_entropyCoder.setEntropyCoder(&this->m_rows[i].m_sbacCoder, slice);
            this->m_rows[i].m_entropyCoder.resetEntropy();
        }
    }

    int getStreamHeaders(AccessUnit& accessUnitOut);

    void initSlice(TComPic* pic);

    /* analyze / compress frame, can be run in parallel within reference constraints */
    void compressFrame();

    /* called by compressFrame to perform wave-front compression analysis */
    void compressCTURows();

    void encodeSlice(TComOutputBitstream* substreams);

    /* blocks until worker thread is done, returns encoded picture and bitstream */
    TComPic *getEncodedPicture(AccessUnit& accessUnit);

    // worker thread
    void threadMain();

    Event                    m_enable;
    Event                    m_done;
    bool                     m_threadActive;

    SEIWriter                m_seiWriter;
    TComSPS                  m_sps;
    TComPPS                  m_pps;
    RateControlEntry         m_rce;

protected:

    void determineSliceBounds();

    TEncTop*                 m_top;
    TEncCfg*                 m_cfg;

    WeightPredAnalysis       m_wp;
    TEncSbac                 m_sbacCoder;
    TEncBinCABAC             m_binCoderCABAC;
    TEncCavlc                m_cavlcCoder;
    FrameFilter              m_frameFilter;
    TComBitCounter           m_bitCounter;

    /* Picture being encoded, and its output NAL list */
    TComPic*                 m_pic;
    AccessUnit               m_accessUnit;

    int                      m_numRows;
    int                      m_filterRowDelay;
    CTURow*                  m_rows;
    Event                    m_completionEvent;
};
}

#endif // ifndef X265_FRAMEENCODER_H
