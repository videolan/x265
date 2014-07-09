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
    m_trQuant.init(enc.m_bEnableRDOQ);
    if (enc.m_useScalingListId == SCALING_LIST_OFF)
    {
        m_trQuant.setFlatScalingList();
        m_trQuant.setUseScalingList(false);
    }
    else if (enc.m_useScalingListId == SCALING_LIST_DEFAULT)
    {
        m_trQuant.setScalingList(enc.getScalingList());
        m_trQuant.setUseScalingList(true);
    }

    m_rdCost.setPsyRdScale(enc.m_param->psyRd);

    m_search.init(&enc, &m_rdCost, &m_trQuant);

    m_cuCoder.init(&enc);
    m_cuCoder.m_search = &m_search;
    m_cuCoder.m_trQuant = &m_trQuant;
    m_cuCoder.m_rdCost = &m_rdCost;
    m_cuCoder.create((uint8_t)g_maxCUDepth, g_maxCUSize);
}

ThreadLocalData::~ThreadLocalData()
{
    m_cuCoder.destroy();
}

void CTURow::processCU(TComDataCU *cu, SBac *bufferSbac, ThreadLocalData& tld, bool bSaveSBac)
{
    if (bufferSbac)
        // Load SBAC coder context from previous row.
        m_rdSbacCoders[0][CI_CURR_BEST].loadContexts(*bufferSbac);

    // setup thread local data structures to use this row's CABAC state
    tld.m_search.m_sbacCoder = &m_rdGoOnSbacCoder;
    tld.m_search.m_rdSbacCoders = m_rdSbacCoders;
    tld.m_search.m_rdGoOnSbacCoder = &m_rdGoOnSbacCoder;
    tld.m_cuCoder.m_sbacCoder = &m_rdGoOnSbacCoder;
    tld.m_cuCoder.m_rdSbacCoders = m_rdSbacCoders;
    tld.m_cuCoder.m_rdGoOnSbacCoder = &m_rdGoOnSbacCoder;

    BitCounter bc;
    m_rdGoOnSbacCoder.setBitstream(&bc);

    tld.m_cuCoder.compressCU(cu); // Does all the CU analysis

    tld.m_search.m_sbacCoder = &m_rdSbacCoders[0][CI_CURR_BEST];
    tld.m_cuCoder.m_sbacCoder = &m_rdSbacCoders[0][CI_CURR_BEST];
    m_rdSbacCoders[0][CI_CURR_BEST].setBitstream(&bc);
    bc.resetBits();

    tld.m_cuCoder.encodeCU(cu, true);  // Count bits

    if (bSaveSBac)
        // Save CABAC state for next row
        m_bufferSbacCoder.loadContexts(m_rdSbacCoders[0][CI_CURR_BEST]);
}
