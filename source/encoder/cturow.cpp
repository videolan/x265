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

#include "encoder.h"
#include "PPA/ppa.h"
#include "cturow.h"

using namespace x265;

void CTURow::create(Encoder* top)
{
    m_rdGoOnSbacCoder.init(&m_rdGoOnBinCodersCABAC);
    m_sbacCoder.init(&m_binCoderCABAC);
    m_trQuant.init(1 << top->getQuadtreeTULog2MaxSize(), top->param.bEnableRDOQ, top->param.bEnableRDOQTS, top->param.bEnableTSkipFast);

    m_rdSbacCoders = new TEncSbac * *[g_maxCUDepth + 1];
    m_binCodersCABAC = new TEncBinCABAC * *[g_maxCUDepth + 1];

    for (uint32_t depth = 0; depth < g_maxCUDepth + 1; depth++)
    {
        m_rdSbacCoders[depth]  = new TEncSbac*[CI_NUM];
        m_binCodersCABAC[depth] = new TEncBinCABAC*[CI_NUM];

        for (int ciIdx = 0; ciIdx < CI_NUM; ciIdx++)
        {
            m_rdSbacCoders[depth][ciIdx] = new TEncSbac;
            m_binCodersCABAC[depth][ciIdx] = new TEncBinCABAC(true);
            m_rdSbacCoders[depth][ciIdx]->init(m_binCodersCABAC[depth][ciIdx]);
        }
    }

    m_search.init(top, &m_rdCost, &m_trQuant);
    m_search.setRDSbacCoder(m_rdSbacCoders);
    m_search.setEntropyCoder(&m_entropyCoder);
    m_search.setRDGoOnSbacCoder(&m_rdGoOnSbacCoder);

    m_cuCoder.init(top);
    m_cuCoder.create((UChar)g_maxCUDepth, g_maxCUWidth);
    m_cuCoder.setRdCost(&m_rdCost);
    m_cuCoder.setRDSbacCoder(m_rdSbacCoders);
    m_cuCoder.setEntropyCoder(&m_entropyCoder);
    m_cuCoder.setPredSearch(&m_search);
    m_cuCoder.setTrQuant(&m_trQuant);
    m_cuCoder.setRdCost(&m_rdCost);
}

void CTURow::processCU(TComDataCU *cu, TComSlice *slice, TEncSbac *bufferSbac, bool bSaveSBac)
{
    if (bufferSbac)
    {
        // Load SBAC coder context from previous row.
        m_rdSbacCoders[0][CI_CURR_BEST]->loadContexts(bufferSbac);
    }

    m_entropyCoder.setEntropyCoder(&m_rdGoOnSbacCoder, slice);
    m_entropyCoder.setBitstream(&m_bitCounter);
    m_cuCoder.setRDGoOnSbacCoder(&m_rdGoOnSbacCoder);

    m_cuCoder.compressCU(cu); // Does all the CU analysis

    // restore entropy coder to an initial state
    m_entropyCoder.setEntropyCoder(m_rdSbacCoders[0][CI_CURR_BEST], slice);
    m_entropyCoder.setBitstream(&m_bitCounter);
    m_cuCoder.setBitCounter(&m_bitCounter);
    m_bitCounter.resetBits();

    m_cuCoder.encodeCU(cu);  // Count bits

    if (bSaveSBac)
    {
        // Save CABAC state for next row
        m_bufferSbacCoder.loadContexts(m_rdSbacCoders[0][CI_CURR_BEST]);
    }
}

void CTURow::destroy()
{
    for (uint32_t depth = 0; depth < g_maxCUDepth + 1; depth++)
    {
        for (int ciIdx = 0; ciIdx < CI_NUM; ciIdx++)
        {
            delete m_rdSbacCoders[depth][ciIdx];
            delete m_binCodersCABAC[depth][ciIdx];
        }
    }

    for (uint32_t depth = 0; depth < g_maxCUDepth + 1; depth++)
    {
        delete [] m_rdSbacCoders[depth];
        delete [] m_binCodersCABAC[depth];
    }

    delete[] m_rdSbacCoders;
    delete[] m_binCodersCABAC;
    m_cuCoder.destroy();
}
