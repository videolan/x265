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
    , m_complete_lftV(NULL)
    , m_rows_active(NULL)
    , m_locks(NULL)
    , m_loopFilter(NULL)
    , m_sao(NULL)
{}

void FrameFilter::destroy()
{
    JobProvider::flush();  // ensure no worker threads are using this frame

    if (m_complete_lftV)
    {
        delete[] m_complete_lftV;
    }

    if (m_rows_active)
    {
        delete[] m_rows_active;
    }

    if (m_locks)
    {
        delete[] m_locks;
    }

    if (m_cfg->param.bEnableLoopFilter)
    {
        assert(m_cfg->param.bEnableSAO);
        for (int i = 0; i < m_numRows; ++i)
        {
            m_loopFilter[i].destroy();
            // NOTE: I don't check sao flag since loopfilter and sao have same control status
            m_sao[i].destroy();
            m_sao[i].destroyEncBuffer();
        }

        delete[] m_loopFilter;
        delete[] m_sao;
    }
}

void FrameFilter::init(TEncTop *top, int numRows)
{
    m_cfg = top;
    m_numRows = numRows;

    m_complete_lftV = new uint32_t[numRows];
    m_rows_active = new bool[numRows];
    m_locks = new Lock[numRows];

    if (top->param.bEnableLoopFilter)
    {
        m_loopFilter = new TComLoopFilter[numRows];
        m_sao = new TEncSampleAdaptiveOffset[numRows];
        for (int i = 0; i < m_numRows; ++i)
        {
            m_loopFilter[i].create(g_maxCUDepth);
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

    for (int i = 0; i < m_numRows; i++)
    {
        if (m_cfg->param.bEnableLoopFilter)
        {
            // TODO: I think this flag unused since we remove Tiles
            m_loopFilter[i].setCfg(pic->getSlice()->getPPS()->getLoopFilterAcrossTilesEnabledFlag());
            m_pic->m_complete_lft[i] = 0;
            m_rows_active[i] = false;
            m_complete_lftV[i] = 0;

            if (m_cfg->param.saoLcuBasedOptimization && m_cfg->param.saoLcuBoundary)
                m_sao[i].resetStats();
            m_sao[i].createPicSaoInfo(pic);
        }
        else
        {
            m_pic->m_complete_lft[i] = MAX_INT; // for SAO
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
    ScopedLock self(m_locks[row]);

    if (!m_rows_active[row])
    {
        m_rows_active[row] = true;
        WaveFront::enqueueRow(row);
    }
}

void FrameFilter::processRow(int row)
{
    PPAScopeEvent(Thread_filterCU);

    // Called by worker threads

    const uint32_t numCols = m_pic->getPicSym()->getFrameWidthInCU();
    const uint32_t lineStartCUAddr = row * numCols;
    for (UInt col = m_complete_lftV[row]; col < numCols; col++)
    {
        {
            // TODO: modify FindJob to avoid invalid status here
            ScopedLock self(m_locks[row]);
            if (row < m_numRows - 1 && m_pic->m_complete_enc[row + 1] < col + 1)
            {
                m_rows_active[row] = false;
                return;
            }
            if (row == m_numRows - 1 && m_pic->m_complete_enc[row] < col + 1)
            {
                m_rows_active[row] = false;
                return;
            }
            if (row > 0 && m_complete_lftV[row - 1] < col + 1)
            {
                m_rows_active[row] = false;
                return;
            }
        }
        const uint32_t cuAddr = lineStartCUAddr + col;
        TComDataCU* cu = m_pic->getCU(cuAddr);

        // SAO parameter estimation using non-deblocked pixels for LCU bottom and right boundary areas
        if (m_cfg->param.saoLcuBasedOptimization && m_cfg->param.saoLcuBoundary)
        {
            m_sao[row].calcSaoStatsLCu_BeforeDblk(m_pic, cuAddr);
        }

        m_loopFilter[row].loopFilterCU(cu, EDGE_VER);
        m_complete_lftV[row]++;

        if (col > 0)
        {
            TComDataCU* cu_prev = m_pic->getCU(cuAddr - 1);
            m_loopFilter[row].loopFilterCU(cu_prev, EDGE_HOR);
            m_pic->m_complete_lft[row]++;
        }

        // Active next row when possible
        if (m_complete_lftV[row] >= 2 && row < m_numRows - 1)
        {
            ScopedLock below(m_locks[row + 1]);
            if (m_rows_active[row + 1] == false &&
                (m_complete_lftV[row + 1] + 2 <= m_complete_lftV[row] || m_complete_lftV[row] == numCols))
            {
                m_rows_active[row + 1] = true;
                WaveFront::enqueueRow(row + 1);
            }
        }
    }

    {
        TComDataCU* cu_prev = m_pic->getCU(lineStartCUAddr + numCols - 1);
        m_loopFilter[row].loopFilterCU(cu_prev, EDGE_HOR);
        m_pic->m_complete_lft[row]++;
    }

    // this row of CTUs has been encoded
    if (row == m_numRows - 1)
    {
        m_completionEvent.trigger();
    }
}
