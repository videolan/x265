/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Chung Shin Yee <shinyee@multicorewareinc.com>
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
#include "TLibEncoder/TEncSbac.h"
#include "TLibEncoder/TEncBinCoderCABAC.h"

#include "wavefront.h"

class TEncTop;

namespace x265 {
// private x265 namespace

class ThreadPool;

class CTURow
{
public:

    TEncSbac               m_sbacCoder;
    TEncSbac               m_rdGoOnSbacCoder;
    TEncSbac               m_bufferSbacCoder;
    TEncBinCABAC           m_binCoderCABAC;
    TEncBinCABACCounter    m_rdGoOnBinCodersCABAC;
    TComBitCounter         m_bitCounter;
    TComRdCost             m_rdCost;
    TEncEntropy            m_entropyCoder;
    TEncSearch             m_search;
    TEncCu                 m_cuCoder;
    TComTrQuant            m_trQuant;
    TEncSbac            ***m_rdSbacCoders;
    TEncBinCABACCounter ***m_binCodersCABAC;

    void create(TEncTop* top);

    void destroy();

    void init()
    {
        m_active = 0;
    }

    inline void processCU(TComDataCU *cu, TComSlice *slice, TEncSbac *bufferSBac, bool bSaveCabac);

    /* Threading */
    Lock                m_lock;
    volatile bool       m_active;
};

// Manages the wave-front processing of a single encoding frame
class FrameEncoder : public WaveFront
{
public:

    FrameEncoder(ThreadPool *);

    virtual ~FrameEncoder() {}

    void init(TEncTop *top, int numRows);

    void destroy();

    void encode(TComPic *pic, TComSlice* slice);

    void processRow(int row);

    /* Config broadcast methods */
    void setAdaptiveSearchRange(int dir, int refIdx, int newSR)
    {
        for (int i = 0; i < m_numRows; i++)
        {
            m_rows[i].m_search.setAdaptiveSearchRange(dir, refIdx, newSR);
        }
    }

    void setQPLambda(Int QP, double lumaLambda, double chromaLambda)
    {
        for (int i = 0; i < m_numRows; i++)
        {
            m_rows[i].m_search.setQPLambda(QP, lumaLambda, chromaLambda);
        }
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

    TComSlice*   getSlice()                    { return m_slice; }

    /* Frame singletons, last the life of the encoder */
    TEncSbac*               getSingletonSbac() { return &m_sbacCoder; }

    TComLoopFilter*         getLoopFilter()    { return &m_loopFilter; }

    TEncSampleAdaptiveOffset* getSAO()         { return &m_sao; }

    TEncCavlc*              getCavlcCoder()    { return &m_cavlcCoder; }

    TEncBinCABAC*           getBinCABAC()      { return &m_binCoderCABAC; }

    TComBitCounter*         getBitCounter()    { return &m_bitCounter; }

    TEncSlice*              getSliceEncoder()  { return &m_sliceEncoder; }

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

protected:

    TEncSbac                 m_sbacCoder;
    TEncBinCABAC             m_binCoderCABAC;
    TEncCavlc                m_cavlcCoder;
    TComLoopFilter           m_loopFilter;
    TEncSampleAdaptiveOffset m_sao;
    TComBitCounter           m_bitCounter;
    TEncSlice                m_sliceEncoder;
    TEncCfg*                 m_cfg;

    TComSlice*               m_slice;
    TComPic*                 m_pic;

    int                      m_numRows;
    CTURow*                  m_rows;
    Event                    m_completionEvent;
};
}

#endif // ifndef __FRAMEENCODER__
