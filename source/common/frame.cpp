/*****************************************************************************
* Copyright (C) 2013 x265 project
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

#include "common.h"
#include "frame.h"
#include "picyuv.h"
#include "framedata.h"

using namespace X265_NS;

Frame::Frame()
{
    m_bChromaExtended = false;
    m_lowresInit = false;
    m_reconRowCount.set(0);
    m_reconColCount = NULL;
    m_countRefEncoders = 0;
    m_encData = NULL;
    m_reconPic = NULL;
    m_quantOffsets = NULL;
    m_next = NULL;
    m_prev = NULL;
    m_param = NULL;
    m_userSEI.numPayloads = 0;
    m_userSEI.payloads = NULL;
    memset(&m_lowres, 0, sizeof(m_lowres));
    m_rcData = NULL;
}

bool Frame::create(x265_param *param, float* quantOffsets)
{
    m_fencPic = new PicYuv;
    m_param = param;
    CHECKED_MALLOC_ZERO(m_rcData, RcStats, 1);

    if (m_fencPic->create(param->sourceWidth, param->sourceHeight, param->internalCsp) &&
        m_lowres.create(m_fencPic, param->bframes, !!param->rc.aqMode, param->rc.qgSize))
    {
        X265_CHECK((m_reconColCount == NULL), "m_reconColCount was initialized");
        m_numRows = (m_fencPic->m_picHeight + g_maxCUSize - 1)  / g_maxCUSize;
        m_reconColCount = new ThreadSafeInteger[m_numRows];

        if (quantOffsets)
        {
            int32_t cuCount;
            if (param->rc.qgSize == 8)
                cuCount = m_lowres.maxBlocksInRowFullRes * m_lowres.maxBlocksInColFullRes;
            else
                cuCount = m_lowres.maxBlocksInRow * m_lowres.maxBlocksInCol;
            m_quantOffsets = new float[cuCount];
        }
        return true;
    }
    return false;
fail:
    return false;
}

bool Frame::allocEncodeData(x265_param *param, const SPS& sps)
{
    m_encData = new FrameData;
    m_reconPic = new PicYuv;
    m_param = param;
    m_encData->m_reconPic = m_reconPic;
    bool ok = m_encData->create(*param, sps, m_fencPic->m_picCsp) && m_reconPic->create(param->sourceWidth, param->sourceHeight, param->internalCsp);
    if (ok)
    {
        /* initialize right border of m_reconpicYuv as SAO may read beyond the
         * end of the picture accessing uninitialized pixels */
        int maxHeight = sps.numCuInHeight * g_maxCUSize;
        memset(m_reconPic->m_picOrg[0], 0, sizeof(pixel)* m_reconPic->m_stride * maxHeight);

        /* use pre-calculated cu/pu offsets cached in the SPS structure */
        m_reconPic->m_cuOffsetY = sps.cuOffsetY;
        m_reconPic->m_buOffsetY = sps.buOffsetY;

        if (param->internalCsp != X265_CSP_I400)
        {
            memset(m_reconPic->m_picOrg[1], 0, sizeof(pixel) * m_reconPic->m_strideC * (maxHeight >> m_reconPic->m_vChromaShift));
            memset(m_reconPic->m_picOrg[2], 0, sizeof(pixel) * m_reconPic->m_strideC * (maxHeight >> m_reconPic->m_vChromaShift));

            /* use pre-calculated cu/pu offsets cached in the SPS structure */
            m_reconPic->m_cuOffsetC = sps.cuOffsetC;
            m_reconPic->m_buOffsetC = sps.buOffsetC;
        }
    }
    return ok;
}

/* prepare to re-use a FrameData instance to encode a new picture */
void Frame::reinit(const SPS& sps)
{
    m_bChromaExtended = false;
    m_reconPic = m_encData->m_reconPic;
    m_encData->reinit(sps);
}

void Frame::destroy()
{
    if (m_encData)
    {
        m_encData->destroy();
        delete m_encData;
        m_encData = NULL;
    }

    if (m_fencPic)
    {
        m_fencPic->destroy();
        delete m_fencPic;
        m_fencPic = NULL;
    }

    if (m_reconPic)
    {
        m_reconPic->destroy();
        delete m_reconPic;
        m_reconPic = NULL;
    }

    if (m_reconColCount)
    {
        delete[] m_reconColCount;
        m_reconColCount = NULL;
    }

    if (m_quantOffsets)
    {
        delete[] m_quantOffsets;
    }

    if (m_userSEI.numPayloads)
    {
        for (int i = 0; i < m_userSEI.numPayloads; i++)
            delete[] m_userSEI.payloads[i].payload;
        delete[] m_userSEI.payloads;
    }

    m_lowres.destroy();
    X265_FREE(m_rcData);
}
