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

/** \file     TComPicSym.cpp
    \brief    picture symbol class
*/

#include "TComPicSym.h"
#include "TComSampleAdaptiveOffset.h"

using namespace x265;

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TComPicSym::TComPicSym()
    : m_widthInCU(0)
    , m_heightInCU(0)
    , m_unitSize(0)
    , m_log2UnitSize(0)
    , m_numPartitions(0)
    , m_numPartInCUSize(0)
    , m_numCUsInFrame(0)
    , m_slice(NULL)
    , m_cuData(NULL)
{
    m_saoParam = NULL;
}

bool TComPicSym::create(x265_param *param)
{
    uint32_t i;

    m_numPartitions   = 1 << (g_maxCUDepth << 1);

    m_log2UnitSize    = g_log2UnitSize;
    m_unitSize        = 1 << m_log2UnitSize;

    m_numPartInCUSize = g_maxCUSize >> m_log2UnitSize;

    m_widthInCU       = (param->sourceWidth + g_maxCUSize - 1) >> g_maxLog2CUSize;
    m_heightInCU      = (param->sourceHeight + g_maxCUSize - 1) >> g_maxLog2CUSize;

    m_numCUsInFrame   = m_widthInCU * m_heightInCU;

    m_slice = new Slice;
    m_cuData = new TComDataCU[m_numCUsInFrame];
    if (!m_slice || !m_cuData)
        return false;

    bool tqBypass = param->bCULossless || param->bLossless;
    for (i = 0; i < m_numCUsInFrame; i++)
    {
        uint32_t sizeL = 1 << (g_maxLog2CUSize * 2);
        uint32_t sizeC = sizeL >> (CHROMA_H_SHIFT(param->internalCsp) + CHROMA_V_SHIFT(param->internalCsp));
        if (!m_cuData[i].initialize(m_numPartitions, sizeL, sizeC, 1, tqBypass))
            return false;

        m_cuData[i].create(&m_cuData[i], m_numPartitions, g_maxCUSize, m_unitSize, param->internalCsp, 0, tqBypass);
    }

    return true;
}

void TComPicSym::destroy()
{
    delete m_slice;
    m_slice = NULL;

    if (m_cuData)
        for (int i = 0; i < m_numCUsInFrame; i++)
            m_cuData[i].destroy();

    delete [] m_cuData;
    m_cuData = NULL;

    if (m_saoParam)
    {
        TComSampleAdaptiveOffset::freeSaoParam(m_saoParam);
        delete m_saoParam;
        m_saoParam = NULL;
    }
}

void TComPicSym::allocSaoParam(TComSampleAdaptiveOffset *sao)
{
    m_saoParam = new SAOParam;
    sao->allocSaoParam(m_saoParam);
}

//! \}
