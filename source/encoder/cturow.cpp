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
    m_cuCoder.setPredSearch(&m_search);
    m_cuCoder.setTrQuant(&m_trQuant);
    m_cuCoder.setRdCost(&m_rdCost);
    m_cuCoder.create((uint8_t)g_maxCUDepth, g_maxCUSize);
}

ThreadLocalData::~ThreadLocalData()
{
    m_cuCoder.destroy();
}

bool CTURow::create()
{
    m_rdGoOnSbacCoder.init(&m_rdGoOnBinCodersCABAC);
    m_sbacCoder.init(&m_binCoderCABAC);
    m_rdSbacCoders = new SBac * *[g_maxCUDepth + 1];
    m_binCodersCABAC = new TEncBinCABAC * *[g_maxCUDepth + 1];
    if (!m_rdSbacCoders || !m_binCodersCABAC)
        return false;

    for (uint32_t depth = 0; depth < g_maxCUDepth + 1; depth++)
    {
        m_rdSbacCoders[depth] = new SBac*[CI_NUM];
        m_binCodersCABAC[depth] = new TEncBinCABAC*[CI_NUM];
        if (!m_rdSbacCoders[depth] || !m_rdSbacCoders[depth])
            return false;

        for (int ciIdx = 0; ciIdx < CI_NUM; ciIdx++)
        {
            m_rdSbacCoders[depth][ciIdx] = new SBac;
            m_binCodersCABAC[depth][ciIdx] = new TEncBinCABAC(true);
            if (m_rdSbacCoders[depth][ciIdx] && m_binCodersCABAC[depth][ciIdx])
                m_rdSbacCoders[depth][ciIdx]->init(m_binCodersCABAC[depth][ciIdx]);
            else
                return false;
        }
    }

    return true;
}

void CTURow::setThreadLocalData(ThreadLocalData& tld)
{
    tld.m_cuCoder.setRDSbacCoder(m_rdSbacCoders);
    tld.m_cuCoder.setEntropyCoder(&m_entropyCoder);
    tld.m_search.setRDSbacCoder(m_rdSbacCoders);
    tld.m_search.setEntropyCoder(&m_entropyCoder);
    tld.m_search.setRDGoOnSbacCoder(&m_rdGoOnSbacCoder);
}

void CTURow::processCU(TComDataCU *cu, TComSlice *slice, SBac *bufferSbac, ThreadLocalData& tld, bool bSaveSBac)
{
    if (bufferSbac)
    {
        // Load SBAC coder context from previous row.
        m_rdSbacCoders[0][CI_CURR_BEST]->loadContexts(bufferSbac);
    }

    BitCounter bc;

    m_entropyCoder.setEntropyCoder(&m_rdGoOnSbacCoder, slice);
    m_entropyCoder.setBitstream(&bc);
    tld.m_cuCoder.setRDGoOnSbacCoder(&m_rdGoOnSbacCoder);

    tld.m_cuCoder.compressCU(cu); // Does all the CU analysis

    // restore entropy coder to an initial state
    m_entropyCoder.setEntropyCoder(m_rdSbacCoders[0][CI_CURR_BEST], slice);
    m_entropyCoder.setBitstream(&bc);
    tld.m_cuCoder.setBitCounting(true);
    bc.resetBits();

    tld.m_cuCoder.encodeCU(cu);  // Count bits

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
}
