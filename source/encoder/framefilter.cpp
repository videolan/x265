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

#include "TLibEncoder/TEncTop.h"
#include "PPA/ppa.h"
#include "framefilter.h"
#include "wavefront.h"

using namespace x265;

// **************************************************************************
// * LoopFilter
// **************************************************************************
FrameFilter::FrameFilter(ThreadPool* pool)
    : WaveFront(pool)
    , m_cfg(NULL)
    , m_pic(NULL)
    , m_lft_active(false)
    , m_sao(NULL)
{}

void FrameFilter::destroy()
{
    JobProvider::flush();  // ensure no worker threads are using this frame

    if (m_cfg->param.bEnableLoopFilter)
    {
        assert(m_cfg->param.bEnableSAO);
        m_loopFilter.destroy();
        for (int i = 0; i < m_numRows; ++i)
        {
            // NOTE: I don't check sao flag since loopfilter and sao have same control status
            m_sao[i].destroy();
            m_sao[i].destroyEncBuffer();
        }

        delete[] m_sao;
    }
}

void FrameFilter::init(TEncTop *top, int numRows)
{
    m_cfg = top;
    m_numRows = numRows;

    if (top->param.bEnableLoopFilter)
    {
        m_sao = new TEncSampleAdaptiveOffset[numRows];
        m_loopFilter.create(g_maxCUDepth);
        for (int i = 0; i < m_numRows; ++i)
        {
            m_sao[i].setSaoLcuBoundary(top->param.saoLcuBoundary);
            m_sao[i].setSaoLcuBasedOptimization(top->param.saoLcuBasedOptimization);
            m_sao[i].setMaxNumOffsetsPerPic(top->getMaxNumOffsetsPerPic());
            m_sao[i].create(top->param.sourceWidth, top->param.sourceHeight, g_maxCUWidth, g_maxCUHeight);
            m_sao[i].createEncBuffer();
        }
    }

    if (!WaveFront::init(m_numRows))
    {
        assert(!"Unable to initialize job queue.");
        m_pool = NULL;
    }
}

void FrameFilter::start(TComPic *pic)
{
    m_pic = pic;

    m_loopFilter.setCfg(pic->getSlice()->getPPS()->getLoopFilterAcrossTilesEnabledFlag());
    m_lft_active = false;
    for (int i = 0; i < m_numRows; i++)
    {
        if (m_cfg->param.bEnableLoopFilter)
        {
            if (m_cfg->param.saoLcuBasedOptimization && m_cfg->param.saoLcuBoundary)
                m_sao[i].resetStats();
            m_sao[i].createPicSaoInfo(pic);
        }
    }

    if (m_cfg->param.bEnableLoopFilter && m_pool && m_cfg->param.bEnableWavefront)
    {
        WaveFront::enqueue();
    }
}

void FrameFilter::wait()
{
    // Block until worker threads complete the frame
    m_completionEvent.wait();
    WaveFront::dequeue();
}

void FrameFilter::end()
{
    if (m_cfg->param.bEnableLoopFilter)
    {
        for (int i = 0; i < m_numRows; i++)
        {
            m_sao[i].destroyPicSaoInfo();
        }
    }
}

void FrameFilter::enqueueRow(int row)
{
    ScopedLock self(m_lock);

    if (!m_lft_active)
    {
        WaveFront::enqueueRow(row);
    }
}

void FrameFilter::processRow(int row)
{
    PPAScopeEvent(Thread_filterCU);

    // Called by worker threads

    const uint32_t numCols = m_pic->getPicSym()->getFrameWidthInCU();
    const uint32_t lineStartCUAddr = row * numCols;

    {
        ScopedLock self(m_lock);
        if (m_lft_active)
            return;
        m_lft_active = true;
    }

    // SAO parameter estimation using non-deblocked pixels for LCU bottom and right boundary areas
    if (m_cfg->param.saoLcuBasedOptimization && m_cfg->param.saoLcuBoundary)
    {
        for (UInt col = 0; col < numCols; col++)
        {
            const uint32_t cuAddr = lineStartCUAddr + col;
            m_sao[row].calcSaoStatsLCu_BeforeDblk(m_pic, cuAddr);
        }
    }

    for (UInt col = 0; col < numCols; col++)
    {
        const uint32_t cuAddr = lineStartCUAddr + col;
        TComDataCU* cu = m_pic->getCU(cuAddr);

        m_loopFilter.loopFilterCU(cu, EDGE_VER);

        if (col > 0)
        {
            TComDataCU* cu_prev = m_pic->getCU(cuAddr - 1);
            m_loopFilter.loopFilterCU(cu_prev, EDGE_HOR);
        }
    }

    {
        TComDataCU* cu_prev = m_pic->getCU(lineStartCUAddr + numCols - 1);
        m_loopFilter.loopFilterCU(cu_prev, EDGE_HOR);
    }

    // Active next row when possible
    ScopedLock self(m_lock);
    m_lft_active = false;
    if (row + 2 < m_numRows)
    {
        if (m_pic->m_complete_enc[row + 2] == numCols)
        {
            WaveFront::enqueueRow(row + 1);
        }
    }

    // Active last row
    if (row == m_numRows - 2)
    {
        WaveFront::enqueueRow(row + 1);
    }

    // this row of CTUs has been encoded
    if (row == m_numRows - 1)
    {
        m_completionEvent.trigger();
    }
}
