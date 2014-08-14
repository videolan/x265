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
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#include "encoder.h"
#include "frameencoder.h"
#include "cturow.h"
#include "PPA/ppa.h"

using namespace x265;

void ThreadLocalData::init(Encoder& enc)
{
    m_cuCoder.initSearch(enc);
    m_cuCoder.create((uint8_t)g_maxCUDepth, g_maxCUSize);
}

ThreadLocalData::~ThreadLocalData()
{
    m_cuCoder.destroy();
}

void CTURow::processCU(TComDataCU *cu, Entropy *bufferSbac, ThreadLocalData& tld, bool bSaveSBac)
{
    if (bufferSbac)
        // Load SBAC coder context from previous row.
        m_rdEntropyCoders[0][CI_CURR_BEST].loadContexts(*bufferSbac);

    // setup thread local data structures to use this row's CABAC state
    tld.m_cuCoder.m_entropyCoder = &m_entropyCoder;
    tld.m_cuCoder.m_rdEntropyCoders = m_rdEntropyCoders;
    tld.m_cuCoder.m_quant.m_entropyCoder = &m_entropyCoder;

    tld.m_cuCoder.compressCU(cu); // Does all the CU analysis

    tld.m_cuCoder.m_entropyCoder = &m_rdEntropyCoders[0][CI_CURR_BEST];
    tld.m_cuCoder.m_quant.m_entropyCoder = &m_rdEntropyCoders[0][CI_CURR_BEST];
    m_rdEntropyCoders[0][CI_CURR_BEST].resetBits();

    // TODO: still necessary?
    tld.m_cuCoder.encodeCU(cu);

    if (bSaveSBac)
        // Save CABAC state for next row
        m_bufferEntropyCoder.loadContexts(m_rdEntropyCoders[0][CI_CURR_BEST]);
}
