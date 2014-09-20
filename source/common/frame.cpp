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
#include "mv.h"

using namespace x265;

Frame::Frame()
    : m_origPicYuv(NULL)
    , m_reconPicYuv(NULL)
    , m_rowDiagQp(NULL)
    , m_rowDiagQScale(NULL)
    , m_rowDiagSatd(NULL)
    , m_rowDiagIntraSatd(NULL)
    , m_rowEncodedBits(NULL)
    , m_numEncodedCusPerRow(NULL)
    , m_rowSatdForVbv(NULL)
    , m_cuCostsForVbv(NULL)
    , m_intraCuCostsForVbv(NULL)
{
    m_picSym = NULL;
    m_reconRowCount.set(0);
    m_countRefEncoders = 0;
    memset(&m_lowres, 0, sizeof(m_lowres));
    m_next = NULL;
    m_prev = NULL;
    m_qpaAq = NULL;
    m_qpaRc = NULL;
    m_avgQpRc = 0;
    m_avgQpAq = 0;
    m_bChromaPlanesExtended = false;
    m_intraData = NULL;
    m_interData = NULL;
}

bool Frame::create(x265_param *param, Window& display, Window& conformance)
{
    m_conformanceWindow = conformance;
    m_defaultDisplayWindow = display;

    m_origPicYuv = new TComPicYuv;

    bool ok = true;
    ok &= m_origPicYuv->create(param->sourceWidth, param->sourceHeight, param->internalCsp, g_maxCUSize, g_maxFullDepth);
    ok &= m_lowres.create(m_origPicYuv, param->bframes, !!param->rc.aqMode);

    bool isVbv = param->rc.vbvBufferSize > 0 && param->rc.vbvMaxBitrate > 0;
    if (ok && (isVbv || param->rc.aqMode))
    {
        int numCols = (param->sourceWidth + g_maxCUSize - 1) >> g_maxLog2CUSize;
        int numRows = (param->sourceHeight + g_maxCUSize - 1) >> g_maxLog2CUSize;

        if (param->rc.aqMode)
            CHECKED_MALLOC(m_qpaAq, double, numRows);
        if (isVbv)
        {
            CHECKED_MALLOC(m_rowDiagQp, double, numRows);
            CHECKED_MALLOC(m_rowDiagQScale, double, numRows);
            CHECKED_MALLOC(m_rowDiagSatd, uint32_t, numRows);
            CHECKED_MALLOC(m_rowDiagIntraSatd, uint32_t, numRows);
            CHECKED_MALLOC(m_rowEncodedBits, uint32_t, numRows);
            CHECKED_MALLOC(m_numEncodedCusPerRow, uint32_t, numRows);
            CHECKED_MALLOC(m_rowSatdForVbv, uint32_t, numRows);
            CHECKED_MALLOC(m_cuCostsForVbv, uint32_t, numRows * numCols);
            CHECKED_MALLOC(m_intraCuCostsForVbv, uint32_t, numRows * numCols);
            CHECKED_MALLOC(m_qpaRc, double, numRows);
        }
        reinit(param);
    }

    return ok;

fail:
    ok = false;
    return ok;
}

bool Frame::allocPicSym(x265_param *param)
{
    m_picSym = new TComPicSym;
    m_reconPicYuv = new TComPicYuv;
    m_picSym->m_reconPicYuv = m_reconPicYuv;
    bool ok = m_picSym->create(param) &&
            m_reconPicYuv->create(param->sourceWidth, param->sourceHeight, param->internalCsp, g_maxCUSize, g_maxFullDepth);
    if (ok)
    {
        // initialize m_reconpicYuv as SAO may read beyond the end of the picture accessing uninitialized pixels
        int maxHeight = m_reconPicYuv->m_numCuInHeight * g_maxCUSize;
        memset(m_reconPicYuv->m_picOrg[0], 0, m_reconPicYuv->m_stride * maxHeight);
        memset(m_reconPicYuv->m_picOrg[1], 0, m_reconPicYuv->m_strideC * (maxHeight >> m_reconPicYuv->m_vChromaShift));
        memset(m_reconPicYuv->m_picOrg[2], 0, m_reconPicYuv->m_strideC * (maxHeight >> m_reconPicYuv->m_vChromaShift));
    }
    return ok;
}

void Frame::reinit(x265_param *param)
{
    int numCols = (param->sourceWidth + g_maxCUSize - 1) >> g_maxLog2CUSize;
    int numRows = (param->sourceHeight + g_maxCUSize - 1) >> g_maxLog2CUSize;
    if (param->rc.vbvBufferSize > 0 && param->rc.vbvMaxBitrate > 0)
    {
        memset(m_rowDiagQp, 0, numRows * sizeof(double));
        memset(m_rowDiagQScale, 0, numRows * sizeof(double));
        memset(m_rowDiagSatd, 0, numRows * sizeof(uint32_t));
        memset(m_rowDiagIntraSatd, 0, numRows * sizeof(uint32_t));
        memset(m_rowEncodedBits, 0, numRows * sizeof(uint32_t));
        memset(m_numEncodedCusPerRow, 0, numRows * sizeof(uint32_t));
        memset(m_rowSatdForVbv, 0, numRows * sizeof(uint32_t));
        memset(m_cuCostsForVbv, 0,  numRows * numCols * sizeof(uint32_t));
        memset(m_intraCuCostsForVbv, 0, numRows * numCols * sizeof(uint32_t));
        memset(m_qpaRc, 0, numRows * sizeof(double));
    }
    if (param->rc.aqMode)
        memset(m_qpaAq, 0, numRows * sizeof(double));
}

void Frame::destroy()
{
    if (m_picSym)
    {
        m_picSym->destroy();
        delete m_picSym;
        m_picSym = NULL;
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

    X265_FREE(m_rowDiagQp);
    X265_FREE(m_rowDiagQScale);
    X265_FREE(m_rowDiagSatd);
    X265_FREE(m_rowDiagIntraSatd);
    X265_FREE(m_rowEncodedBits);
    X265_FREE(m_numEncodedCusPerRow);
    X265_FREE(m_rowSatdForVbv);
    X265_FREE(m_cuCostsForVbv);
    X265_FREE(m_intraCuCostsForVbv);
    X265_FREE(m_qpaAq);
    X265_FREE(m_qpaRc);
}

//! \}
