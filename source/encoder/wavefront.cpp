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
    m_cRDGoOnSbacCoder.init(&m_cRDGoOnBinCodersCABAC);
    m_cSbacCoder.init(&m_cBinCoderCABAC);
    m_cSearch.init(top, &m_cRdCost);

    m_cCuEncoder.create((UChar)g_uiMaxCUDepth, g_uiMaxCUWidth, g_uiMaxCUHeight);
    m_cCuEncoder.init(top);

    if (top->getUseAdaptiveQP())
    {
        m_cTrQuant.initSliceQpDelta();
    }
    m_cTrQuant.init(1 << top->getQuadtreeTULog2MaxSize(), top->getUseRDOQ(), top->getUseRDOQTS(), true,
                    top->getUseTransformSkipFast(), top->getUseAdaptQpSelect());

    m_pppcRDSbacCoders = new TEncSbac * *[g_uiMaxCUDepth + 1];
    m_pppcBinCodersCABAC = new TEncBinCABACCounter * *[g_uiMaxCUDepth + 1];

    for (UInt iDepth = 0; iDepth < g_uiMaxCUDepth + 1; iDepth++)
    {
        m_pppcRDSbacCoders[iDepth]  = new TEncSbac*[CI_NUM];
        m_pppcBinCodersCABAC[iDepth] = new TEncBinCABACCounter*[CI_NUM];

        for (Int iCIIdx = 0; iCIIdx < CI_NUM; iCIIdx++)
        {
            m_pppcRDSbacCoders[iDepth][iCIIdx] = new TEncSbac;
            m_pppcBinCodersCABAC[iDepth][iCIIdx] = new TEncBinCABACCounter;
            m_pppcRDSbacCoders[iDepth][iCIIdx]->init(m_pppcBinCodersCABAC[iDepth][iCIIdx]);
        }
    }

    m_cCuEncoder.set_pcRdCost(&m_cRdCost);
    m_cCuEncoder.set_pppcRDSbacCoder(m_pppcRDSbacCoders);
    m_cCuEncoder.set_pcEntropyCoder(&m_cEntropyCoder);
    m_cCuEncoder.set_pcPredSearch(&m_cSearch);
    m_cCuEncoder.set_pcTrQuant(&m_cTrQuant);
    m_cCuEncoder.set_pcRdCost(&m_cRdCost);
}

void CTURow::processCU(TComDataCU *pcCU, TComSlice *pcSlice, TEncSbac *pcBufferSBac, bool bSaveSBac)
{
    TEncBinCABAC* pcRDSbacCoder = (TEncBinCABAC*)m_pppcRDSbacCoders[0][CI_CURR_BEST]->getEncBinIf();
    pcRDSbacCoder->setBinCountingEnableFlag(false);
    pcRDSbacCoder->setBinsCoded(0);

    if (pcBufferSBac)
    {
        // Load SBAC coder context from previous row.
        m_pppcRDSbacCoders[0][CI_CURR_BEST]->loadContexts(pcBufferSBac);
    }

    m_cEntropyCoder.setEntropyCoder(&m_cRDGoOnSbacCoder, pcSlice);
    m_cEntropyCoder.setBitstream(&m_cBitCounter);
    ((TEncBinCABAC*)m_cRDGoOnSbacCoder.getEncBinIf())->setBinCountingEnableFlag(true);
    m_cCuEncoder.set_pcRDGoOnSbacCoder(&m_cRDGoOnSbacCoder);

    {
        PPAScopeEvent(Thread_compressCU);
        m_cCuEncoder.compressCU(pcCU); // Does all the CU analysis
    }

    // restore entropy coder to an initial state
    m_cEntropyCoder.setEntropyCoder(m_pppcRDSbacCoders[0][CI_CURR_BEST], pcSlice);
    m_cEntropyCoder.setBitstream(&m_cBitCounter);
    m_cCuEncoder.setBitCounter(&m_cBitCounter);
    pcRDSbacCoder->setBinCountingEnableFlag(true);
    m_cBitCounter.resetBits();
    pcRDSbacCoder->setBinsCoded(0);

    {
        PPAScopeEvent(Thread_encodeCU);
        m_cCuEncoder.encodeCU(pcCU);  // Count bits
    }

    pcRDSbacCoder->setBinCountingEnableFlag(false);
    
    if (bSaveSBac)
    {
        // Save CABAC state for next row
        m_cBufferSbacCoder.loadContexts(m_pppcRDSbacCoders[0][CI_CURR_BEST]);
    }
}

void CTURow::destroy()
{
    for (UInt iDepth = 0; iDepth < g_uiMaxCUDepth + 1; iDepth++)
    {
        for (Int iCIIdx = 0; iCIIdx < CI_NUM; iCIIdx++)
        {
            delete m_pppcRDSbacCoders[iDepth][iCIIdx];
            delete m_pppcBinCodersCABAC[iDepth][iCIIdx];
        }
    }

    for (UInt iDepth = 0; iDepth < g_uiMaxCUDepth + 1; iDepth++)
    {
        delete [] m_pppcRDSbacCoders[iDepth];
        delete [] m_pppcBinCodersCABAC[iDepth];
    }

    delete[] m_pppcRDSbacCoders;
    delete[] m_pppcBinCodersCABAC;
    m_cCuEncoder.destroy();
}

EncodeFrame::EncodeFrame(ThreadPool* pool)
    : QueueFrame(pool)
    , m_pic(NULL)
    , m_rows(NULL)
{}

void EncodeFrame::destroy()
{
    JobProvider::Flush();  // ensure no worker threads are using this frame

    if (m_rows)
    {
        for (int i = 0; i < m_nrows; ++i)
        {
            m_rows[i].destroy();
        }

        delete[] m_rows;
    }

    m_cSliceEncoder.destroy();
    if (m_pcCfg->getUseSAO())
    {
        m_cEncSAO.destroy();
        m_cEncSAO.destroyEncBuffer();
    }
    m_cLoopFilter.destroy();
}

void EncodeFrame::init(TEncTop *top)
{
    m_pcCfg = top;
    m_nrows = top->getNumSubstreams();
    m_enableWpp = top->getWaveFrontsynchro() ? true : false;

    m_cSliceEncoder.init(top);
    m_cSliceEncoder.create(top->getSourceWidth(), top->getSourceHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight, (UChar) g_uiMaxCUDepth);
    if (top->getUseSAO())
    {
        m_cEncSAO.setSaoLcuBoundary(top->getSaoLcuBoundary());
        m_cEncSAO.setSaoLcuBasedOptimization(top->getSaoLcuBasedOptimization());
        m_cEncSAO.setMaxNumOffsetsPerPic(top->getMaxNumOffsetsPerPic());
        m_cEncSAO.create(top->getSourceWidth(), top->getSourceHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight);
        m_cEncSAO.createEncBuffer();
    }
    m_cLoopFilter.create(g_uiMaxCUDepth);

    m_rows = new CTURow[m_nrows];
    for (int i = 0; i < m_nrows; ++i)
    {
        m_rows[i].create(top);
    }

    if (!QueueFrame::InitJobQueue(m_nrows))
    {
        assert(!"Unable to initialize job queue.");
        throw;
    }
}

void EncodeFrame::Encode(TComPic *pic, TComSlice *pcSlice)
{
    m_pic = pic;
    m_pcSlice = pcSlice;

    // reset entropy coders
    m_cSbacCoder.init(&m_cBinCoderCABAC);
    for (int i = 0; i < this->m_nrows; i++)
    {
        m_rows[i].init();
        m_rows[i].m_cEntropyCoder.setEntropyCoder(&m_cSbacCoder, m_pcSlice);
        m_rows[i].m_cEntropyCoder.resetEntropy();
        m_rows[i].m_pppcRDSbacCoders[0][CI_CURR_BEST]->load(&m_cSbacCoder);
    }

    QueueFrame::Enqueue();

    // Enqueue first row, then block until worker threads complete the frame
    QueueFrame::EnqueueRow(0);
    m_completionEvent.Wait();

    QueueFrame::Dequeue();
}

void EncodeFrame::ProcessRow(int irow)
{
    PPAScopeEvent(Thread_ProcessRow);

    // Called by worker threads
    const uint32_t numCols = m_pic->getPicSym()->getFrameWidthInCU();
    const uint32_t lineStartCUAddr = irow * numCols;

    CTURow& curRow  = m_rows[irow];
    CTURow& codeRow = m_rows[m_enableWpp ? irow : 0];

    for (UInt uiCol = curRow.m_curCol; uiCol < numCols; uiCol++)
    {
        const uint32_t uiCUAddr = lineStartCUAddr + uiCol;
        TComDataCU* pcCU = m_pic->getCU(uiCUAddr);
        pcCU->initCU(m_pic, uiCUAddr);

        codeRow.m_cEntropyCoder.setEntropyCoder(&m_cSbacCoder, m_pcSlice);
        codeRow.m_cEntropyCoder.resetEntropy();

        TEncSbac *pcBufSBac = (m_enableWpp && uiCol == 0 && irow > 0) ? &m_rows[irow - 1].m_cBufferSbacCoder : NULL;
        codeRow.processCU(pcCU, m_pcSlice, pcBufSBac, m_enableWpp && uiCol == 1);

        // FIXME: If needed, these counters may be added atomically to slice or this (needed for rate control?)
        //m_uiPicTotalBits += pcCU->getTotalBits();
        //m_dPicRdCost     += pcCU->getTotalCost();
        //m_uiPicDist      += pcCU->getTotalDistortion();

        // Completed CU processing
        curRow.m_curCol++;

        if (curRow.m_curCol >= 2 && irow < m_nrows - 1)
        {
            ScopedLock below(m_rows[irow + 1].m_lock);
            if (m_rows[irow + 1].m_active == false &&
                m_rows[irow + 1].m_curCol + 2 <= curRow.m_curCol)
            {
                m_rows[irow + 1].m_active = true;
                QueueFrame::EnqueueRow(irow + 1);
            }
        }

        ScopedLock self(curRow.m_lock);
        if (irow > 0 && curRow.m_curCol < numCols - 1 && m_rows[irow - 1].m_curCol < curRow.m_curCol + 2)
        {
            curRow.m_active = false;
            return;
        }
    }

    // this row of CTUs has been encoded
    if (irow == m_nrows - 1)
    {
        m_completionEvent.Trigger();
    }
}
