/*****************************************************************************
* Copyright (C) 2013-2020 MulticoreWare, Inc
*
* Author: Steve Borho <steve@borho.org>
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

#include "framedata.h"
#include "picyuv.h"

using namespace X265_NS;

FrameData::FrameData()
{
    memset(this, 0, sizeof(*this));
}

bool FrameData::create(const x265_param& param, const SPS& sps, int csp)
{
    m_param = &param;
    m_slice  = new Slice;
    m_picCTU = new CUData[sps.numCUsInFrame];
    m_picCsp = csp;
    m_spsrpsIdx = -1;
    if (param.rc.bStatWrite)
        m_spsrps = const_cast<RPS*>(sps.spsrps);
    bool isallocated = m_cuMemPool.create(0, param.internalCsp, sps.numCUsInFrame, param);
    if (m_param->bDynamicRefine)
    {
        CHECKED_MALLOC_ZERO(m_cuMemPool.dynRefineRdBlock, uint64_t, MAX_NUM_DYN_REFINE * sps.numCUsInFrame);
        CHECKED_MALLOC_ZERO(m_cuMemPool.dynRefCntBlock, uint32_t, MAX_NUM_DYN_REFINE * sps.numCUsInFrame);
        CHECKED_MALLOC_ZERO(m_cuMemPool.dynRefVarBlock, uint32_t, MAX_NUM_DYN_REFINE * sps.numCUsInFrame);
    }
    if (isallocated)
    {
        for (uint32_t ctuAddr = 0; ctuAddr < sps.numCUsInFrame; ctuAddr++)
        {
            if (m_param->bDynamicRefine)
            {
                m_picCTU[ctuAddr].m_collectCURd = m_cuMemPool.dynRefineRdBlock + (ctuAddr * MAX_NUM_DYN_REFINE);
                m_picCTU[ctuAddr].m_collectCUVariance = m_cuMemPool.dynRefVarBlock + (ctuAddr * MAX_NUM_DYN_REFINE);
                m_picCTU[ctuAddr].m_collectCUCount = m_cuMemPool.dynRefCntBlock + (ctuAddr * MAX_NUM_DYN_REFINE);
            }
            m_picCTU[ctuAddr].initialize(m_cuMemPool, 0, param, ctuAddr);
        }
    }
    else
        return false;
    CHECKED_MALLOC_ZERO(m_cuStat, RCStatCU, sps.numCUsInFrame + 1);
    CHECKED_MALLOC(m_rowStat, RCStatRow, sps.numCuInHeight);
    reinit(sps);
    
    for (int i = 0; i < INTEGRAL_PLANE_NUM; i++)
    {
        m_meBuffer[i] = NULL;
        m_meIntegral[i] = NULL;
    }
    return true;

fail:
    return false;
}

void FrameData::reinit(const SPS& sps)
{
    memset(m_cuStat, 0, sps.numCUsInFrame * sizeof(*m_cuStat));
    memset(m_rowStat, 0, sps.numCuInHeight * sizeof(*m_rowStat));
    if (m_param->bDynamicRefine)
    {
        memset(m_picCTU->m_collectCURd, 0, MAX_NUM_DYN_REFINE * sps.numCUsInFrame * sizeof(uint64_t));
        memset(m_picCTU->m_collectCUVariance, 0, MAX_NUM_DYN_REFINE * sps.numCUsInFrame * sizeof(uint32_t));
        memset(m_picCTU->m_collectCUCount, 0, MAX_NUM_DYN_REFINE * sps.numCUsInFrame * sizeof(uint32_t));
    }
}

void FrameData::destroy()
{
    delete [] m_picCTU;
    delete m_slice;
    delete m_saoParam;

    m_cuMemPool.destroy();

    if (m_param->bDynamicRefine)
    {
        X265_FREE(m_cuMemPool.dynRefineRdBlock);
        X265_FREE(m_cuMemPool.dynRefCntBlock);
        X265_FREE(m_cuMemPool.dynRefVarBlock);
    }
    X265_FREE(m_cuStat);
    X265_FREE(m_rowStat);
    for (int i = 0; i < INTEGRAL_PLANE_NUM; i++)
    {
        if (m_meBuffer[i] != NULL)
        {
            X265_FREE(m_meBuffer[i]);
            m_meBuffer[i] = NULL;
        }
    }
}
