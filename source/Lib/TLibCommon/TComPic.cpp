/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2013, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/** \file     TComPic.cpp
    \brief    picture class
*/

#include "TComPic.h"
#include "SEI.h"
#include "mv.h"
#include "TLibEncoder/TEncCfg.h"

using namespace x265;

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TComPic::TComPic()
    : m_picSym(NULL)
    , m_origPicYuv(NULL)
    , m_reconPicYuv(NULL)
    , m_bUsedByCurr(false)
    , m_bIsLongTerm(false)
    , m_bCheckLTMSB(false)
    , m_rowDiagQp(NULL)
    , m_rowDiagQScale(NULL)
    , m_rowDiagSatd(NULL)
    , m_rowEncodedBits(NULL)
    , m_numEncodedCusPerRow(NULL)
    , m_rowSatdForVbv(NULL)
    , m_cuCostsForVbv(NULL)
{
    m_reconRowCount = 0;
    m_countRefEncoders = 0;
    memset(&m_lowres, 0, sizeof(m_lowres));
    m_next = NULL;
    m_prev = NULL;
    m_SSDY = 0;
    m_SSDU = 0;
    m_SSDV = 0;
    m_ssim = 0;
    m_ssimCnt = 0;
    m_frameTime = 0.0;
    m_elapsedCompressTime = 0.0;
    m_qpaAq = 0;
    m_qpaRc = 0;
    m_avgQpRc = 0;
    m_avgQpAq = 0;
    m_bChromaPlanesExtended = false;
}

TComPic::~TComPic()
{}

bool TComPic::create(TEncCfg* cfg)
{
    /* store conformance window parameters with picture */
    m_conformanceWindow = cfg->m_conformanceWindow;

    /* store display window parameters with picture */
    m_defaultDisplayWindow = cfg->getDefaultDisplayWindow();

    m_picSym = new TComPicSym;
    m_origPicYuv = new TComPicYuv;
    m_reconPicYuv = new TComPicYuv;
    if (!m_picSym || !m_origPicYuv || !m_reconPicYuv)
        return false;

    bool ok = true;
    ok &= m_picSym->create(cfg->param.sourceWidth, cfg->param.sourceHeight, cfg->param.internalCsp, g_maxCUWidth, g_maxCUHeight, g_maxCUDepth);
    ok &= m_origPicYuv->create(cfg->param.sourceWidth, cfg->param.sourceHeight, cfg->param.internalCsp, g_maxCUWidth, g_maxCUHeight, g_maxCUDepth);
    ok &= m_reconPicYuv->create(cfg->param.sourceWidth, cfg->param.sourceHeight, cfg->param.internalCsp, g_maxCUWidth, g_maxCUHeight, g_maxCUDepth);
    ok &= m_lowres.create(m_origPicYuv, cfg->param.bframes, !!cfg->param.rc.aqMode);

    bool isVbv = cfg->param.rc.vbvBufferSize > 0 && cfg->param.rc.vbvMaxBitrate > 0;
    if (ok && (isVbv || cfg->param.rc.aqMode))
    {
        int numRows = m_picSym->getFrameHeightInCU();
        int numCols = m_picSym->getFrameWidthInCU();

        if (cfg->param.rc.aqMode)
            CHECKED_MALLOC(m_qpaAq, double, numRows);
        if (isVbv)
        {
            CHECKED_MALLOC(m_rowDiagQp, double, numRows);
            CHECKED_MALLOC(m_rowDiagQScale, double, numRows);
            CHECKED_MALLOC(m_rowDiagSatd, uint32_t, numRows);
            CHECKED_MALLOC(m_rowEncodedBits, uint32_t, numRows);
            CHECKED_MALLOC(m_numEncodedCusPerRow, uint32_t, numRows);
            CHECKED_MALLOC(m_rowSatdForVbv, uint32_t, numRows);
            CHECKED_MALLOC(m_cuCostsForVbv, uint32_t, numRows * numCols);
            CHECKED_MALLOC(m_qpaRc, double, numRows);
        }
        reInit(cfg);
    }

    return ok;

fail:
    ok = false;
    return ok;
}

void TComPic::reInit(TEncCfg* cfg)
{
    if (cfg->param.rc.vbvBufferSize > 0 && cfg->param.rc.vbvMaxBitrate > 0)
    {
        int numRows = m_picSym->getFrameHeightInCU();
        int numCols = m_picSym->getFrameWidthInCU();
        memset(m_rowDiagQp, 0, numRows * sizeof(double));
        memset(m_rowDiagQScale, 0, numRows * sizeof(double));
        memset(m_rowDiagSatd, 0, numRows * sizeof(uint32_t));
        memset(m_rowEncodedBits, 0, numRows * sizeof(uint32_t));
        memset(m_numEncodedCusPerRow, 0, numRows * sizeof(uint32_t));
        memset(m_rowSatdForVbv, 0, numRows * sizeof(uint32_t));
        memset(m_cuCostsForVbv, 0,  numRows * numCols * sizeof(uint32_t));
        memset(m_qpaRc, 0, numRows * sizeof(double));
    }
    if (cfg->param.rc.aqMode)
        memset(m_qpaAq, 0,  m_picSym->getFrameHeightInCU() * sizeof(double));
}

void TComPic::destroy(int bframes)
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
    m_lowres.destroy(bframes);

    X265_FREE(m_rowDiagQp);
    X265_FREE(m_rowDiagQScale);
    X265_FREE(m_rowDiagSatd);
    X265_FREE(m_rowEncodedBits);
    X265_FREE(m_numEncodedCusPerRow);
    X265_FREE(m_rowSatdForVbv);
    X265_FREE(m_cuCostsForVbv);
    X265_FREE(m_qpaAq);
    X265_FREE(m_qpaRc);
}

//! \}
