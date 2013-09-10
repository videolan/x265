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
FrameFilter::FrameFilter()
    : m_cfg(NULL)
    , m_pic(NULL)
{
}

void FrameFilter::destroy()
{
    if (m_cfg->param.bEnableLoopFilter)
    {
        assert(m_cfg->param.bEnableSAO);
        m_loopFilter.destroy();

        // NOTE: I don't check sao flag since loopfilter and sao have same control status
        m_sao.destroy();
        m_sao.destroyEncBuffer();
    }
}

void FrameFilter::init(TEncTop *top, int numRows, TEncSbac* rdGoOnSbacCoder)
{
    m_cfg = top;
    m_numRows = numRows;

    // NOTE: for sao only, I write this code because I want to exact match with HM's bug bitstream
    m_rdGoOnSbacCoderRow0 = rdGoOnSbacCoder;

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
    m_rdGoOnSbacCoder.init(&m_rdGoOnBinCodersCABAC);
    m_entropyCoder.setEntropyCoder(&m_rdGoOnSbacCoder, pic->getSlice());
    m_entropyCoder.setBitstream(&m_bitCounter);

    if (m_cfg->param.bEnableLoopFilter)
    {
        m_sao.resetStats();
        m_sao.createPicSaoInfo(pic);

        SAOParam* saoParam = pic->getPicSym()->getSaoParam();
        m_sao.resetSAOParam(saoParam);
        m_sao.rdoSaoUnitRowInit(saoParam);
    }
}

void FrameFilter::end()
{
    if (m_cfg->param.bEnableLoopFilter)
    {
        m_sao.destroyPicSaoInfo();
    }
}

void FrameFilter::processRow(int row)
{
    PPAScopeEvent(Thread_filterCU);

    if (!m_cfg->param.bEnableLoopFilter)
    {
        if (row > 0)
            processRowPost(row - 1);
        if (row == m_numRows - 1)
            processRowPost(row);
        return;
    }

    // NOTE: We are here only active both of loopfilter and sao, the row 0 always finished, so we can safe to copy row[0]'s data
    if (row == 0)
    {
        // NOTE: not need, seems HM's bug, I want to keep output exact matched.
        m_rdGoOnBinCodersCABAC.m_fracBits = ((TEncBinCABACCounter*)((TEncSbac*)m_rdGoOnSbacCoderRow0->m_pcBinIf))->m_fracBits;
        m_sao.startSaoEnc(m_pic, &m_entropyCoder, &m_rdGoOnSbacCoder);
    }

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

    // SAO
    SAOParam* saoParam = m_pic->getPicSym()->getSaoParam();
    if (m_sao.getSaoLcuBasedOptimization())
    {
        m_sao.rdoSaoUnitRow(saoParam, row);

        // NOTE: Delay a row because SAO decide need top row pixels at next row, is it HM's bug?
        if (row > 0)
        {
            // NOTE: these flag is not use in this mode
            assert(saoParam->oneUnitFlag[0] == false);
            assert(saoParam->oneUnitFlag[1] == false);
            assert(saoParam->oneUnitFlag[2] == false);

            if (saoParam->bSaoFlag[0])
            {
                m_sao.processSaoUnitRow(saoParam->saoLcuParam[0], row - 1, 0);
            }
            if (saoParam->bSaoFlag[1])
            {
                m_sao.processSaoUnitRow(saoParam->saoLcuParam[1], row - 1, 1);
                m_sao.processSaoUnitRow(saoParam->saoLcuParam[2], row - 1, 2);
            }

            // TODO: this code is NOT VERIFY because TransformSkip and PCM mode have some bugs, they always not active!
            bool  bPCMFilter = (m_pic->getSlice()->getSPS()->getUsePCM() && m_pic->getSlice()->getSPS()->getPCMFilterDisableFlag()) ? true : false;
            if (bPCMFilter || m_pic->getSlice()->getPPS()->getTransquantBypassEnableFlag())
            {
                for (UInt col = 0; col < numCols; col++)
                {
                    const uint32_t cuAddr = lineStartCUAddr - numCols + col;
                    TComDataCU* cu = m_pic->getCU(cuAddr);

                    xPCMCURestoration(cu, 0, 0);
                }
            }
        }
    }

    // this row of CTUs has been encoded

    // TODO: extend margins for motion reference

    if (row > 0)
    {
        processRowPost(row - 1);
    }

    if (row == m_numRows - 1)
    {
        m_sao.rdoSaoUnitRowEnd(saoParam, m_pic->getNumCUsInFrame());

        // Process Last row of SAO
        if (saoParam->bSaoFlag[0])
        {
            m_sao.processSaoUnitRow(saoParam->saoLcuParam[0], row, 0);
        }
        if (saoParam->bSaoFlag[1])
        {
            m_sao.processSaoUnitRow(saoParam->saoLcuParam[1], row, 1);
            m_sao.processSaoUnitRow(saoParam->saoLcuParam[2], row, 2);
        }

        // TODO: this code is NOT VERIFY because TransformSkip and PCM mode have some bugs, they always not active!
        bool  bPCMFilter = (m_pic->getSlice()->getSPS()->getUsePCM() && m_pic->getSlice()->getSPS()->getPCMFilterDisableFlag()) ? true : false;
        if (bPCMFilter || m_pic->getSlice()->getPPS()->getTransquantBypassEnableFlag())
        {
            for (UInt col = 0; col < numCols; col++)
            {
                const uint32_t cuAddr = lineStartCUAddr + col;
                TComDataCU* cu = m_pic->getCU(cuAddr);

                xPCMCURestoration(cu, 0, 0);
            }
        }

        processRowPost(row);
    }
}

void FrameFilter::processRowPost(int row)
{
    PPAScopeEvent(Thread_filterCU);

    const uint32_t numCols = m_pic->getPicSym()->getFrameWidthInCU();
    const uint32_t lineStartCUAddr = row * numCols;
    TComPicYuv *recon = m_pic->getPicYuvRec();
    const int lastH = ((recon->getHeight() % g_maxCUHeight) ? (recon->getHeight() % g_maxCUHeight) : g_maxCUHeight);
    const int realH = (row != m_numRows - 1) ? g_maxCUHeight : lastH;

    // TODO: Remove when we confirm below code is right
    //recon->xExtendPicCompBorder(recon->getLumaAddr(), recon->getStride(), recon->getWidth(), recon->getHeight(), recon->m_lumaMarginX, recon->m_lumaMarginY);
    //recon->xExtendPicCompBorder(recon->getCbAddr(), recon->getCStride(), recon->getWidth() >> 1, recon->getHeight() >> 1, recon->m_chromaMarginX, recon->m_chromaMarginY);
    //recon->xExtendPicCompBorder(recon->getCrAddr(), recon->getCStride(), recon->getWidth() >> 1, recon->getHeight() >> 1, recon->m_chromaMarginX, recon->m_chromaMarginY);
    // Border extend Left and Right
    primitives.extendRowBorder(recon->getLumaAddr(lineStartCUAddr), recon->getStride(), recon->getWidth(), realH, recon->getLumaMarginX());
    primitives.extendRowBorder(recon->getCbAddr(lineStartCUAddr), recon->getCStride(), recon->getWidth() >> 1, realH >> 1, recon->getChromaMarginX());
    primitives.extendRowBorder(recon->getCrAddr(lineStartCUAddr), recon->getCStride(), recon->getWidth() >> 1, realH >> 1, recon->getChromaMarginX());

    // Border extend Top
    if (row == 0)
    {
        const intptr_t stride = recon->getStride();
        const intptr_t strideC = recon->getCStride();
        Pel *pixY = recon->getLumaAddr(lineStartCUAddr) - recon->getLumaMarginX();
        Pel *pixU = recon->getCbAddr(lineStartCUAddr) - recon->getChromaMarginX();
        Pel *pixV = recon->getCrAddr(lineStartCUAddr) - recon->getChromaMarginX();

        for (int y = 0; y < recon->getLumaMarginY(); y++)
        {
            memcpy(pixY - (y + 1) * stride, pixY, stride * sizeof(Pel));
        }

        for (int y = 0; y < recon->getChromaMarginY(); y++)
        {
            memcpy(pixU - (y + 1) * strideC, pixU, strideC * sizeof(Pel));
            memcpy(pixV - (y + 1) * strideC, pixV, strideC * sizeof(Pel));
        }
    }

    // Border extend Bottom
    if (row == m_numRows - 1)
    {
        const intptr_t stride = recon->getStride();
        const intptr_t strideC = recon->getCStride();
        Pel *pixY = recon->getLumaAddr(lineStartCUAddr) - recon->getLumaMarginX() + (realH - 1) * stride;
        Pel *pixU = recon->getCbAddr(lineStartCUAddr) - recon->getChromaMarginX() + ((realH >> 1) - 1) * strideC;
        Pel *pixV = recon->getCrAddr(lineStartCUAddr) - recon->getChromaMarginX() + ((realH >> 1) - 1) * strideC;

        for (int y = 0; y < recon->getLumaMarginY(); y++)
        {
            memcpy(pixY + (y + 1) * stride, pixY, stride * sizeof(Pel));
        }

        for (int y = 0; y < recon->getChromaMarginY(); y++)
        {
            memcpy(pixU + (y + 1) * strideC, pixU, strideC * sizeof(Pel));
            memcpy(pixV + (y + 1) * strideC, pixV, strideC * sizeof(Pel));
        }
    }

    // Notify other FrameEncoders that this row of reconstructed pixels is available
    m_pic->m_reconRowCount++;
    for (UInt i = 0; i < m_pic->m_countRefEncoders; i++)
    {
        m_pic->m_reconRowWait.trigger();
    }
}
