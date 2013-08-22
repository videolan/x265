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
    : JobProvider(pool)
    , m_cfg(NULL)
    , m_pic(NULL)
    , active_lft(FALSE)
{}

void FrameFilter::destroy()
{
    JobProvider::flush();  // ensure no worker threads are using this frame

    if (m_cfg->param.bEnableLoopFilter)
    {
        assert(m_cfg->param.bEnableSAO);
        m_loopFilter.destroy();

        // NOTE: I don't check sao flag since loopfilter and sao have same control status
        m_sao.destroy();
        m_sao.destroyEncBuffer();
    }
}

bool FrameFilter::findJob()
{
    // Check the lock
    if (ATOMIC_CAS32(&active_lft, FALSE, TRUE) == TRUE)
        return false;

    // NOTE: only one thread can be here
    if (row_done < row_ready)
    {
        // NOTE: not need atom operator here because we lock before
        row_done++;
        processRow(row_done);
        active_lft = FALSE;
        return true;
    }
    active_lft = FALSE;
    return false;
}

void FrameFilter::init(TEncTop *top, int numRows)
{
    m_cfg = top;
    m_numRows = numRows;

    if (top->param.bEnableLoopFilter)
    {
        m_loopFilter.create(g_maxCUDepth);
        m_sao.setSaoLcuBoundary(top->param.saoLcuBoundary);
        m_sao.setSaoLcuBasedOptimization(top->param.saoLcuBasedOptimization);
        m_sao.setMaxNumOffsetsPerPic(top->getMaxNumOffsetsPerPic());
        m_sao.create(top->param.sourceWidth, top->param.sourceHeight, g_maxCUWidth, g_maxCUHeight);
        m_sao.createEncBuffer();
    }
}

void FrameFilter::start(TComPic *pic)
{
    m_pic = pic;

    m_loopFilter.setCfg(pic->getSlice()->getPPS()->getLoopFilterAcrossTilesEnabledFlag());
    row_ready = -1;
    row_done = -1;
    active_lft = FALSE;
    if (m_cfg->param.bEnableLoopFilter)
    {
        if (m_cfg->param.saoLcuBasedOptimization && m_cfg->param.saoLcuBoundary)
            m_sao.resetStats();
        m_sao.createPicSaoInfo(pic);
    }

    if (m_cfg->param.bEnableLoopFilter && m_pool && m_cfg->param.bEnableWavefront)
    {
        JobProvider::enqueue();
    }
}

void FrameFilter::wait()
{
    // Block until worker threads complete the frame
    m_completionEvent.wait();
    JobProvider::dequeue();
}

void FrameFilter::end()
{
    if (m_cfg->param.bEnableLoopFilter)
    {
        m_sao.destroyPicSaoInfo();
    }
}

void FrameFilter::enqueueRow(int row)
{
    assert(row < m_numRows);
    // NOTE: not need atom here since we have only one writer and reader
    row_ready = row;
    m_pool->pokeIdleThread();
}

void FrameFilter::processRow(int row)
{
    PPAScopeEvent(Thread_filterCU);

    // Called by worker threads

    const uint32_t numCols = m_pic->getPicSym()->getFrameWidthInCU();
    const uint32_t lineStartCUAddr = row * numCols;

    // SAO parameter estimation using non-deblocked pixels for LCU bottom and right boundary areas
    if (m_cfg->param.saoLcuBasedOptimization && m_cfg->param.saoLcuBoundary)
    {
        m_sao.calcSaoStatsRowCus_BeforeDblk(m_pic, row);
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

    // this row of CTUs has been encoded
    if (row == m_numRows - 1)
    {
        m_completionEvent.trigger();
    }
}
