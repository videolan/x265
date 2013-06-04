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
#include "wavefront.h"

using namespace x265;

void CTURow::create(TEncTop* top)
{
    m_cRDGoOnSbacCoder.init(&m_cRDGoOnBinCodersCABAC);
    m_cSbacCoder.init(&m_cBinCoderCABAC);
    m_cSearch.init(top, &m_cRdCost);

    m_cCuEncoder.create((UChar) g_uiMaxCUDepth, g_uiMaxCUWidth, g_uiMaxCUHeight);
    m_cCuEncoder.init(top);

    if (top->getUseAdaptiveQP())
    {
        m_cTrQuant.initSliceQpDelta();
    }
    m_cTrQuant.init(1 << top->getQuadtreeTULog2MaxSize(), top->getUseRDOQ(), top->getUseRDOQTS(), true,
        top->getUseTransformSkipFast(), top->getUseAdaptQpSelect() );

    m_pppcRDSbacCoders = new TEncSbac**[g_uiMaxCUDepth + 1];
    m_pppcBinCodersCABAC = new TEncBinCABACCounter**[g_uiMaxCUDepth + 1];

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

EncodeFrame::EncodeFrame(ThreadPool* pool) : QueueFrame(pool)
{
    m_pcBufferSbacCoders = NULL;
}


EncodeFrame::~EncodeFrame()
{
}

void EncodeFrame::destroy()
{
    this->Flush();
    for (int i = 0; i < this->nrows; ++i)
    {
        this->rows[i].destroy();
    }
    if (this->rows)
    {
        delete[] this->rows;
    }
    if (this->m_pcBufferSbacCoders)
    {
        delete[] this->m_pcBufferSbacCoders;
    }
}

void EncodeFrame::create(TEncTop *top)
{
    this->nrows = top->getNumSubstreams();
    this->enableWpp = top->getWaveFrontsynchro() ? true : false;
    this->m_pcSbacCoder = top->getSbacCoder();
    this->m_pcBinCABAC = top->getBinCABAC();

    this->rows = new CTURow[this->nrows];
    for (int i = 0; i < this->nrows; ++i)
    {
        this->rows[i].create(top);
    }

    if (!this->InitJobQueue(this->nrows))
    {
        assert(!"Unable to initialize job queue.");
        throw;
    }
}


void EncodeFrame::Encode(TComPic *pic, TComSlice* pcSlice)
{
    this->m_pic = pic;
    this->m_pcSlice = pcSlice;
    this->ncols = pic->getFrameWidthInCU();

    // reset entropy coders
    this->m_pcSbacCoder->init(m_pcBinCABAC);
    for (int i = 0; i < this->nrows; i++)
    {
        rows[i].init();
        rows[i].m_cEntropyCoder.setEntropyCoder(m_pcSbacCoder, pcSlice);
        rows[i].m_cEntropyCoder.resetEntropy();
        this->m_pcBufferSbacCoders[i].loadContexts(m_pcSbacCoder);
        rows[i].m_pppcRDSbacCoders[0][CI_CURR_BEST]->load(m_pcSbacCoder);
    }

    this->Enqueue();

    this->EnqueueRow(0);
    this->complete.Wait();

    this->Dequeue();
}


void EncodeFrame::ProcessRow(int irow)
{
    const UInt uiWidthInLCUs  = m_pic->getPicSym()->getFrameWidthInCU();
    const UInt uiCurLineCUAddr = irow * uiWidthInLCUs;

    CTURow& curRow = this->rows[irow];
    CTURow& codeRow = this->rows[this->enableWpp ? irow : 0];

    for(UInt uiCol = curRow.m_curCol; uiCol < uiWidthInLCUs; uiCol++)
    {
        const UInt uiCUAddr = uiCurLineCUAddr + uiCol;
        TComDataCU* pcCU = m_pic->getCU(uiCUAddr);
        pcCU->initCU(m_pic, uiCUAddr);

        TEncBinCABAC* pcRDSbacCoder = (TEncBinCABAC*)codeRow.m_pppcRDSbacCoders[0][CI_CURR_BEST]->getEncBinIf();
        pcRDSbacCoder->setBinCountingEnableFlag(false);
        pcRDSbacCoder->setBinsCoded(0);

        codeRow.m_cEntropyCoder.setEntropyCoder(m_pcSbacCoder, m_pcSlice);
        codeRow.m_cEntropyCoder.resetEntropy();

        if (this->enableWpp && uiCol == 0 && irow > 0)
        {
            // Load SBAC coder context from previous row.
            codeRow.m_pppcRDSbacCoders[0][CI_CURR_BEST]->loadContexts(&this->m_pcBufferSbacCoders[irow-1]);
        }

        TEncSbac &pcGoOnSBacCoder = this->rows[(this->enableWpp && m_pool->GetThreadCount() > 1) ? irow : 0].m_cRDGoOnSbacCoder;
        codeRow.m_cEntropyCoder.setEntropyCoder(&pcGoOnSBacCoder, m_pcSlice);
        codeRow.m_cEntropyCoder.setBitstream(&codeRow.m_cBitCounter);
        ((TEncBinCABAC*)pcGoOnSBacCoder.getEncBinIf())->setBinCountingEnableFlag(true);
        codeRow.m_cCuEncoder.set_pcRDGoOnSbacCoder(&pcGoOnSBacCoder);

        codeRow.m_cCuEncoder.compressCU(pcCU); // Does all the CU analysis

        // restore entropy coder to an initial state
        codeRow.m_cEntropyCoder.setEntropyCoder(codeRow.m_pppcRDSbacCoders[0][CI_CURR_BEST], m_pcSlice);
        codeRow.m_cEntropyCoder.setBitstream(&codeRow.m_cBitCounter);
        codeRow.m_cCuEncoder.setBitCounter(&codeRow.m_cBitCounter);
        pcRDSbacCoder->setBinCountingEnableFlag(true);
        codeRow.m_cBitCounter.resetBits();
        pcRDSbacCoder->setBinsCoded(0);

        codeRow.m_cCuEncoder.encodeCU(pcCU);  // Output CU bitstream

        pcRDSbacCoder->setBinCountingEnableFlag(false);

        if (this->enableWpp && uiCol == 1)
        {
            // Save CABAC state for next row
            this->m_pcBufferSbacCoders[irow].loadContexts(codeRow.m_pppcRDSbacCoders[0][CI_CURR_BEST]);
        }

        // FIXME: If needed, these counters may be added atomically to slice or this (needed for rate control?)
        //m_uiPicTotalBits += pcCU->getTotalBits();
        //m_dPicRdCost     += pcCU->getTotalCost();
        //m_uiPicDist      += pcCU->getTotalDistortion();

        // Completed CU processing
        curRow.m_curCol++;

        if (curRow.m_curCol >= 2 && irow < this->nrows - 1)
        {
            ScopedLock below(this->rows[irow + 1].m_lock);
            if (this->rows[irow + 1].m_active == false &&
                this->rows[irow + 1].m_curCol + 2 <= curRow.m_curCol)
            {
                this->rows[irow + 1].m_active = true;
                this->EnqueueRow(irow + 1);
            }
        }

        ScopedLock self(curRow.m_lock);
        if (irow > 0 && curRow.m_curCol < this->ncols - 1 && this->rows[irow - 1].m_curCol < curRow.m_curCol + 2)
        {
            curRow.m_active = false;
            return;
        }
    }

    if (irow == this->nrows - 1)
    {
        this->complete.Trigger();
    }
}
