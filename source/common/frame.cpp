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

using namespace x265;

Frame::Frame()
{
    m_reconRowCount.set(0);
    m_countRefEncoders = 0;
    m_frameEncoderID = 0;
    m_encData = NULL;
    m_next = NULL;
    m_prev = NULL;
    memset(&m_lowres, 0, sizeof(m_lowres));
}

bool Frame::create(x265_param *param)
{
    m_origPicYuv = new PicYuv;

    return m_origPicYuv->create(param->sourceWidth, param->sourceHeight, param->internalCsp) &&
           m_lowres.create(m_origPicYuv, param->bframes, !!param->rc.aqMode);
}

bool Frame::allocEncodeData(x265_param *param)
{
    m_encData = new FrameData;
    m_reconPicYuv = new PicYuv;
    m_encData->m_reconPicYuv = m_reconPicYuv;
    bool ok = m_encData->create(param) && m_reconPicYuv->create(param->sourceWidth, param->sourceHeight, param->internalCsp);
    if (ok)
    {
        /* initialize right border of m_reconpicYuv as SAO may read beyond the
         * end of the picture accessing uninitialized pixels */
        int maxHeight = m_reconPicYuv->m_numCuInHeight * g_maxCUSize;
        memset(m_reconPicYuv->m_picOrg[0], 0, m_reconPicYuv->m_stride * maxHeight);
        memset(m_reconPicYuv->m_picOrg[1], 0, m_reconPicYuv->m_strideC * (maxHeight >> m_reconPicYuv->m_vChromaShift));
        memset(m_reconPicYuv->m_picOrg[2], 0, m_reconPicYuv->m_strideC * (maxHeight >> m_reconPicYuv->m_vChromaShift));
    }
    return ok;
}

void Frame::destroy()
{
    if (m_encData)
    {
        m_encData->destroy();
        delete m_encData;
        m_encData = NULL;
    }

    if (m_origPicYuv)
    {
        m_origPicYuv->destroy();
        delete m_origPicYuv;
        m_origPicYuv = NULL;
    }

    if (m_reconPicYuv)
    {
        m_reconPicYuv->destroy();
        delete m_reconPicYuv;
        m_reconPicYuv = NULL;
    }

    m_lowres.destroy();
}
