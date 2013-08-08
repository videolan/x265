/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Chung Shin Yee <shinyee@multicorewareinc.com>
 *          Min Chen <chenm003@163.com>
 *          Steve Borho <steve@borho.org>
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
#include "wavefront.h"

using namespace x265;

void CTURow::create(TEncTop* top)
{
    m_rdGoOnSbacCoder.init(&m_rdGoOnBinCodersCABAC);
    m_sbacCoder.init(&m_binCoderCABAC);
    m_trQuant.init(1 << top->getQuadtreeTULog2MaxSize(), top->param.bEnableRDOQ, top->param.bEnableRDOQTS, top->param.bEnableTSkipFast);

    m_rdSbacCoders = new TEncSbac **[g_maxCUDepth + 1];
    m_binCodersCABAC = new TEncBinCABACCounter **[g_maxCUDepth + 1];

    for (UInt depth = 0; depth < g_maxCUDepth + 1; depth++)
    {
        m_rdSbacCoders[depth]  = new TEncSbac*[CI_NUM];
        m_binCodersCABAC[depth] = new TEncBinCABACCounter*[CI_NUM];

        for (Int ciIdx = 0; ciIdx < CI_NUM; ciIdx++)
        {
            m_rdSbacCoders[depth][ciIdx] = new TEncSbac;
            m_binCodersCABAC[depth][ciIdx] = new TEncBinCABACCounter;
            m_rdSbacCoders[depth][ciIdx]->init(m_binCodersCABAC[depth][ciIdx]);
        }
    }

    m_search.init(top, &m_rdCost, &m_trQuant);
    m_search.setRDSbacCoder(m_rdSbacCoders);
    m_search.setEntropyCoder(&m_entropyCoder);
    m_search.setRDGoOnSbacCoder(&m_rdGoOnSbacCoder);

    m_cuCoder.create((UChar)g_maxCUDepth, g_maxCUWidth);
    m_cuCoder.init(top);
    m_cuCoder.setRdCost(&m_rdCost);
    m_cuCoder.setRDSbacCoder(m_rdSbacCoders);
    m_cuCoder.setEntropyCoder(&m_entropyCoder);
    m_cuCoder.setPredSearch(&m_search);
    m_cuCoder.setTrQuant(&m_trQuant);
    m_cuCoder.setRdCost(&m_rdCost);
}

void CTURow::processCU(TComDataCU *cu, TComSlice *slice, TEncSbac *bufferSbac, bool bSaveSBac)
{
    TEncBinCABAC* pcRDSbacCoder = (TEncBinCABAC*)m_rdSbacCoders[0][CI_CURR_BEST]->getEncBinIf();

    pcRDSbacCoder->setBinCountingEnableFlag(false);
    pcRDSbacCoder->setBinsCoded(0);

    if (bufferSbac)
    {
        // Load SBAC coder context from previous row.
        m_rdSbacCoders[0][CI_CURR_BEST]->loadContexts(bufferSbac);
    }

    m_entropyCoder.setEntropyCoder(&m_rdGoOnSbacCoder, slice);
    m_entropyCoder.setBitstream(&m_bitCounter);
    ((TEncBinCABAC*)m_rdGoOnSbacCoder.getEncBinIf())->setBinCountingEnableFlag(true);
    m_cuCoder.setRDGoOnSbacCoder(&m_rdGoOnSbacCoder);

    m_cuCoder.compressCU(cu); // Does all the CU analysis

    // restore entropy coder to an initial state
    m_entropyCoder.setEntropyCoder(m_rdSbacCoders[0][CI_CURR_BEST], slice);
    m_entropyCoder.setBitstream(&m_bitCounter);
    m_cuCoder.setBitCounter(&m_bitCounter);
    pcRDSbacCoder->setBinCountingEnableFlag(true);
    m_bitCounter.resetBits();
    pcRDSbacCoder->setBinsCoded(0);

    m_cuCoder.encodeCU(cu);  // Count bits

    pcRDSbacCoder->setBinCountingEnableFlag(false);

    if (bSaveSBac)
    {
        // Save CABAC state for next row
        m_bufferSbacCoder.loadContexts(m_rdSbacCoders[0][CI_CURR_BEST]);
    }
}

void CTURow::destroy()
{
    for (UInt iDepth = 0; iDepth < g_maxCUDepth + 1; iDepth++)
    {
        for (Int iCIIdx = 0; iCIIdx < CI_NUM; iCIIdx++)
        {
            delete m_rdSbacCoders[iDepth][iCIIdx];
            delete m_binCodersCABAC[iDepth][iCIIdx];
        }
    }

    for (UInt iDepth = 0; iDepth < g_maxCUDepth + 1; iDepth++)
    {
        delete [] m_rdSbacCoders[iDepth];
        delete [] m_binCodersCABAC[iDepth];
    }

    delete[] m_rdSbacCoders;
    delete[] m_binCodersCABAC;
    m_cuCoder.destroy();
}

FrameEncoder::FrameEncoder(ThreadPool* pool)
    : WaveFront(pool)
    , m_cfg(NULL)
    , m_slice(NULL)
    , m_pic(NULL)
    , m_rows(NULL)
    , m_frameFilter(pool)
{}

void FrameEncoder::destroy()
{
    JobProvider::flush();  // ensure no worker threads are using this frame

    if (m_rows)
    {
        for (int i = 0; i < m_numRows; ++i)
        {
            m_rows[i].destroy();
        }

        delete[] m_rows;
    }

    m_sliceEncoder.destroy();
    if (m_cfg->param.bEnableSAO)
    {
        m_sao.destroy();
        m_sao.destroyEncBuffer();
    }
    m_frameFilter.destroy();
}

void FrameEncoder::init(TEncTop *top, int numRows)
{
    m_cfg = top;
    m_numRows = numRows;

    m_sliceEncoder.init(top);
    m_sliceEncoder.create(top->param.sourceWidth, top->param.sourceHeight, g_maxCUWidth, g_maxCUHeight, (UChar)g_maxCUDepth);
    if (top->param.bEnableSAO)
    {
        m_sao.setSaoLcuBoundary(top->param.saoLcuBoundary);
        m_sao.setSaoLcuBasedOptimization(top->param.saoLcuBasedOptimization);
        m_sao.setMaxNumOffsetsPerPic(top->getMaxNumOffsetsPerPic());
        m_sao.create(top->param.sourceWidth, top->param.sourceHeight, g_maxCUWidth, g_maxCUHeight);
        m_sao.createEncBuffer();
    }
    m_frameFilter.init(top, numRows);

    m_rows = new CTURow[m_numRows];
    for (int i = 0; i < m_numRows; ++i)
    {
        m_rows[i].create(top);
    }

    if (!WaveFront::init(m_numRows))
    {
        assert(!"Unable to initialize job queue.");
        m_pool = NULL;
    }
}

void FrameEncoder::encode(TComPic *pic, TComSlice *slice)
{
    int bEnableLoopFilter = m_cfg->param.bEnableLoopFilter;
    m_pic = pic;
    m_slice = slice;

    // reset entropy coders
    m_sbacCoder.init(&m_binCoderCABAC);
    for (int i = 0; i < this->m_numRows; i++)
    {
        m_rows[i].init();
        m_rows[i].m_entropyCoder.setEntropyCoder(&m_sbacCoder, m_slice);
        m_rows[i].m_entropyCoder.resetEntropy();
        m_rows[i].m_rdSbacCoders[0][CI_CURR_BEST]->load(&m_sbacCoder);
        m_pic->m_complete_enc[i] = 0;
    }

    m_frameFilter.start(pic, slice);

    if (!m_pool || !m_cfg->param.bEnableWavefront)
    {
        for (int i = 0; i < this->m_numRows; i++)
        {
            processRow(i);
        }
        // Loopfilter
        if (bEnableLoopFilter)
        {
            for (int i = 0; i < this->m_numRows; i++)
            {
                m_frameFilter.processRow(i);
            }
        }
    }
    else
    {
        WaveFront::enqueue();

        // Enqueue first row, then block until worker threads complete the frame
        WaveFront::enqueueRow(0);

        m_completionEvent.wait();

        WaveFront::dequeue();

        // Dummy, I think we need not pend here
        if (bEnableLoopFilter)
        {
            //wait_lft();
        }
    }
}

void FrameEncoder::processRow(int row)
{
    PPAScopeEvent(Thread_ProcessRow);

    // Called by worker threads
    CTURow& curRow  = m_rows[row];
    CTURow& codeRow = m_rows[m_cfg->param.bEnableWavefront ? row : 0];

    const uint32_t numCols = m_pic->getPicSym()->getFrameWidthInCU();
    const uint32_t lineStartCUAddr = row * numCols;
    for (UInt col = m_pic->m_complete_enc[row]; col < numCols; col++)
    {
        const uint32_t cuAddr = lineStartCUAddr + col;
        TComDataCU* cu = m_pic->getCU(cuAddr);
        cu->initCU(m_pic, cuAddr);

        codeRow.m_entropyCoder.setEntropyCoder(&m_sbacCoder, m_slice);
        codeRow.m_entropyCoder.resetEntropy();

        TEncSbac *bufSbac = (m_cfg->param.bEnableWavefront && col == 0 && row > 0) ? &m_rows[row - 1].m_bufferSbacCoder : NULL;
        codeRow.processCU(cu, m_slice, bufSbac, m_cfg->param.bEnableWavefront && col == 1);

        // TODO: Keep atomic running totals for rate control?
        // cu->m_totalBits;
        // cu->m_totalCost;
        // cu->m_totalDistortion;

        // Completed CU processing
        m_pic->m_complete_enc[row]++;

        // Active Loopfilter
        if (row > 0)
        {
            // NOTE: my version, it need check active flag
            m_frameFilter.enqueueRow(row - 1);
        }

        if (m_pic->m_complete_enc[row] >= 2 && row < m_numRows - 1)
        {
            ScopedLock below(m_rows[row + 1].m_lock);
            if (m_rows[row + 1].m_active == false &&
                m_pic->m_complete_enc[row + 1] + 2 <= m_pic->m_complete_enc[row])
            {
                m_rows[row + 1].m_active = true;
                WaveFront::enqueueRow(row + 1);
            }
        }

        ScopedLock self(curRow.m_lock);
        if (row > 0 && m_pic->m_complete_enc[row] < numCols - 1 && m_pic->m_complete_enc[row - 1] < m_pic->m_complete_enc[row] + 2)
        {
            curRow.m_active = false;
            return;
        }
        if (m_cfg->param.bEnableWavefront && checkHigherPriorityRow(row))
        {
            curRow.m_active = false;
            return;
        }
    }

    // this row of CTUs has been encoded
    if (row == m_numRows - 1)
    {
        m_completionEvent.trigger();
    }
}

// **************************************************************************
// * LoopFilter
// **************************************************************************
FrameFilter::FrameFilter(ThreadPool* pool)
    : WaveFront(pool)
    , m_cfg(NULL)
    , m_slice(NULL)
    , m_pic(NULL)
    , m_loopFilter(NULL)
    , m_complete_lftV(NULL)
    , m_rows_active(NULL)
    , m_locks(NULL)
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
        for (int i = 0; i < m_numRows; ++i)
        {
            m_loopFilter[i].destroy();
        }

        delete[] m_loopFilter;
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
        for (int i = 0; i < m_numRows; ++i)
        {
            m_loopFilter[i].create(g_maxCUDepth);
        }
    }


    if (!WaveFront::init(m_numRows))
    {
        assert(!"Unable to initialize job queue.");
        m_pool = NULL;
    }
}

void FrameFilter::start(TComPic *pic, TComSlice *slice)
{
    m_pic = pic;
    m_slice = slice;

    for (int i = 0; i < m_numRows; i++)
    {
        if (m_cfg->param.bEnableLoopFilter)
        {
            // TODO: I think this flag unused since we remove Tiles
            m_loopFilter[i].setCfg(slice->getPPS()->getLoopFilterAcrossTilesEnabledFlag());
            m_pic->m_complete_lft[i] = 0;
            m_rows_active[i] = false;
            m_complete_lftV[i] = 0;
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
