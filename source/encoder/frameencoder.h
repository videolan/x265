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

#ifndef __FRAMEENCODER__
#define __FRAMEENCODER__

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

namespace x265 {
// private x265 namespace

class ThreadPool;
class TEncTop;

// Manages the wave-front processing of a single encoding frame
class FrameEncoder : public WaveFront, public x265::Thread
{
public:

    FrameEncoder();

    virtual ~FrameEncoder() {}

    void setThreadPool(ThreadPool *p);

    void init(TEncTop *top, int numRows);

    void destroy();

    void processRow(int row);

    /* Config broadcast methods */
    void setAdaptiveSearchRange(int dir, int refIdx, int newSR)
    {
        for (int i = 0; i < m_numRows; i++)
        {
            m_rows[i].m_search.setAdaptiveSearchRange(dir, refIdx, newSR);
        }
    }

    void setQPLambda(Int QP, double lumaLambda, double chromaLambda, int depth)
    {
        for (int i = 0; i < m_numRows; i++)
        {
            m_rows[i].m_search.setQPLambda(QP, lumaLambda, chromaLambda);
        }

        m_frameFilter.m_sao.lumaLambda = lumaLambda;
        m_frameFilter.m_sao.chromaLambd = chromaLambda;
        m_frameFilter.m_sao.depth = depth;
    }

    void setCbDistortionWeight(double weight)
    {
        for (int i = 0; i < m_numRows; i++)
        {
            m_rows[i].m_rdCost.setCbDistortionWeight(weight);
        }
    }

    void setCrDistortionWeight(double weight)
    {
        for (int i = 0; i < m_numRows; i++)
        {
            m_rows[i].m_rdCost.setCrDistortionWeight(weight);
        }
    }

    void setFlatScalingList()
    {
        for (int i = 0; i < m_numRows; i++)
        {
            m_rows[i].m_trQuant.setFlatScalingList();
        }
    }

    void setUseScalingList(bool flag)
    {
        for (int i = 0; i < m_numRows; i++)
        {
            m_rows[i].m_trQuant.setUseScalingList(flag);
        }
    }

    void setScalingList(TComScalingList *list)
    {
        for (int i = 0; i < m_numRows; i++)
        {
            this->m_rows[i].m_trQuant.setScalingList(list);
        }
    }

    TEncEntropy* getEntropyCoder(int row)      { return &this->m_rows[row].m_entropyCoder; }

    TEncSbac*    getSbacCoder(int row)         { return &this->m_rows[row].m_sbacCoder; }

    TEncSbac*    getRDGoOnSbacCoder(int row)   { return &this->m_rows[row].m_rdGoOnSbacCoder; }

    TEncSbac***  getRDSbacCoders(int row)      { return this->m_rows[row].m_rdSbacCoders; }

    TEncSbac*    getBufferSBac(int row)        { return &this->m_rows[row].m_bufferSbacCoder; }

    TEncCu*      getCuEncoder(int row)         { return &this->m_rows[row].m_cuCoder; }

    /* Frame singletons, last the life of the encoder */
    TEncSbac*               getSingletonSbac() { return &m_sbacCoder; }

    TEncSampleAdaptiveOffset* getSAO()         { return &m_frameFilter.m_sao; }

    TEncCavlc*              getCavlcCoder()    { return &m_cavlcCoder; }

    TEncBinCABAC*           getBinCABAC()      { return &m_binCoderCABAC; }

    TComBitCounter*         getBitCounter()    { return &m_bitCounter; }

    void resetEntropy(TComSlice *slice)
    {
        for (int i = 0; i < this->m_numRows; i++)
        {
            this->m_rows[i].m_entropyCoder.setEntropyCoder(&this->m_rows[i].m_sbacCoder, slice);
            this->m_rows[i].m_entropyCoder.resetEntropy();
        }
    }

    void resetEncoder()
    {
        // Initialize global singletons (these should eventually be per-slice)
        m_sbacCoder.init((TEncBinIf*)&m_binCoderCABAC);
    }

    int getStreamHeaders(AccessUnit& accessUnitOut);

    void initSlice(TComPic* pic, Bool bForceISlice, Int gopID);

    /* analyze / compress frame, can be run in parallel within reference constraints */
    void compressFrame(TComPic *pic);

    /* called by compressFrame to perform wave-front analysis */
    void compressCTURows(TComPic *pic);

    void encodeSlice(TComPic* pic, TComOutputBitstream* substreams);

    void determineSliceBounds(TComPic* pic);

    TComPic *getEncodedPicture(AccessUnit& accessUnit);

    // Frame parallelism
    void threadMain(void)
    {
        while (m_threadActive)
        {
            m_enable.wait();
            if (!m_threadActive)
                break;
            compressFrame(m_pic);
            m_done.trigger();
        }
    }

    Event                    m_enable;
    Event                    m_done;
    bool                     m_threadActive;

    SEIWriter                m_seiWriter;
    TComSPS                  m_sps;
    TComPPS                  m_pps;

protected:

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
    volatile int             m_referenceRowsAvailable;

    int                      m_numRows;
    int                      row_delay;
    CTURow*                  m_rows;
    Event                    m_completionEvent;
};
}

#endif // ifndef __FRAMEENCODER__
