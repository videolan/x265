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

/** \file     TComYuv.cpp
    \brief    general YUV buffer class
    \todo     this should be merged with TComPicYuv
*/

#include "CommonDef.h"
#include "TComYuv.h"
#include "TShortYUV.h"
#include "TComPrediction.h"
#include "primitives.h"

#include <cstdlib>
#include <memory.h>
#include <assert.h>
#include <math.h>

using namespace x265;

//! \ingroup TLibCommon
//! \{

TComYuv::TComYuv()
{
    m_bufY = NULL;
    m_bufU = NULL;
    m_bufV = NULL;
}

TComYuv::~TComYuv()
{}

void TComYuv::create(uint32_t width, uint32_t height, int csp)
{
    m_hChromaShift = CHROMA_H_SHIFT(csp);
    m_vChromaShift = CHROMA_V_SHIFT(csp);

    // memory allocation
    m_bufY = (Pel*)X265_MALLOC(Pel, width * height);
    m_bufU = (Pel*)X265_MALLOC(Pel, (width >> m_hChromaShift) * (height >> m_vChromaShift));
    m_bufV = (Pel*)X265_MALLOC(Pel, (width >> m_hChromaShift) * (height >> m_vChromaShift));

    // set width and height
    m_width   = width;
    m_height  = height;

    m_cwidth  = width  >> m_hChromaShift;
    m_cheight = height >> m_vChromaShift;
}

void TComYuv::destroy()
{
    // memory free
    X265_FREE(m_bufY);
    m_bufY = NULL;
    X265_FREE(m_bufU);
    m_bufU = NULL;
    X265_FREE(m_bufV);
    m_bufV = NULL;
}

void TComYuv::clear()
{
    ::memset(m_bufY, 0, (m_width  * m_height) * sizeof(Pel));
    ::memset(m_bufU, 0, (m_cwidth * m_cheight) * sizeof(Pel));
    ::memset(m_bufV, 0, (m_cwidth * m_cheight) * sizeof(Pel));
}

void TComYuv::copyToPicYuv(TComPicYuv* destPicYuv, uint32_t cuAddr, uint32_t absZOrderIdx, uint32_t partDepth, uint32_t partIdx)
{
    copyToPicLuma(destPicYuv, cuAddr, absZOrderIdx, partDepth, partIdx);
    copyToPicChroma(destPicYuv, cuAddr, absZOrderIdx, partDepth, partIdx);
}

void TComYuv::copyToPicLuma(TComPicYuv* destPicYuv, uint32_t cuAddr, uint32_t absZOrderIdx, uint32_t partDepth, uint32_t partIdx)
{
    int width, height;

    width  = m_width >> partDepth;
    height = m_height >> partDepth;

    Pel* src = getLumaAddr(partIdx, width);
    Pel* dst = destPicYuv->getLumaAddr(cuAddr, absZOrderIdx);

    uint32_t srcstride = getStride();
    uint32_t dststride = destPicYuv->getStride();

    primitives.blockcpy_pp(width, height, dst, dststride, src, srcstride);
}

void TComYuv::copyToPicChroma(TComPicYuv* destPicYuv, uint32_t cuAddr, uint32_t absZOrderIdx, uint32_t partDepth, uint32_t partIdx)
{
    int width, height;

    width  = m_cwidth >> partDepth;
    height = m_cheight >> partDepth;

    Pel* srcU = getCbAddr(partIdx, width);
    Pel* srcV = getCrAddr(partIdx, width);
    Pel* dstU = destPicYuv->getCbAddr(cuAddr, absZOrderIdx);
    Pel* dstV = destPicYuv->getCrAddr(cuAddr, absZOrderIdx);

    uint32_t srcstride = getCStride();
    uint32_t dststride = destPicYuv->getCStride();

    primitives.blockcpy_pp(width, height, dstU, dststride, srcU, srcstride);
    primitives.blockcpy_pp(width, height, dstV, dststride, srcV, srcstride);
}

void TComYuv::copyFromPicYuv(TComPicYuv* srcPicYuv, uint32_t cuAddr, uint32_t absZOrderIdx)
{
    copyFromPicLuma(srcPicYuv, cuAddr, absZOrderIdx);
    copyFromPicChroma(srcPicYuv, cuAddr, absZOrderIdx);
}

void TComYuv::copyFromPicLuma(TComPicYuv* srcPicYuv, uint32_t cuAddr, uint32_t absZOrderIdx)
{
    Pel* dst = m_bufY;
    Pel* src = srcPicYuv->getLumaAddr(cuAddr, absZOrderIdx);

    uint32_t dststride = getStride();
    uint32_t srcstride = srcPicYuv->getStride();

    primitives.blockcpy_pp(m_width, m_height, dst, dststride, src, srcstride);
}

void TComYuv::copyFromPicChroma(TComPicYuv* srcPicYuv, uint32_t cuAddr, uint32_t absZOrderIdx)
{
    Pel* dstU = m_bufU;
    Pel* dstV = m_bufV;
    Pel* srcU = srcPicYuv->getCbAddr(cuAddr, absZOrderIdx);
    Pel* srcV = srcPicYuv->getCrAddr(cuAddr, absZOrderIdx);

    uint32_t dststride = getCStride();
    uint32_t srcstride = srcPicYuv->getCStride();

    primitives.blockcpy_pp(m_cwidth, m_cheight, dstU, dststride, srcU, srcstride);
    primitives.blockcpy_pp(m_cwidth, m_cheight, dstV, dststride, srcV, srcstride);
}

void TComYuv::copyToPartYuv(TComYuv* dstPicYuv, uint32_t uiDstPartIdx)
{
    copyToPartLuma(dstPicYuv, uiDstPartIdx);
    copyToPartChroma(dstPicYuv, uiDstPartIdx);
}

void TComYuv::copyToPartLuma(TComYuv* dstPicYuv, uint32_t uiDstPartIdx)
{
    Pel* src = m_bufY;
    Pel* dst = dstPicYuv->getLumaAddr(uiDstPartIdx);

    uint32_t srcstride = getStride();
    uint32_t dststride = dstPicYuv->getStride();

    primitives.blockcpy_pp(m_width, m_height, dst, dststride, src, srcstride);
}

void TComYuv::copyToPartChroma(TComYuv* dstPicYuv, uint32_t uiDstPartIdx)
{
    Pel* srcU = m_bufU;
    Pel* srcV = m_bufV;
    Pel* dstU = dstPicYuv->getCbAddr(uiDstPartIdx);
    Pel* dstV = dstPicYuv->getCrAddr(uiDstPartIdx);

    uint32_t srcstride = getCStride();
    uint32_t dststride = dstPicYuv->getCStride();

    primitives.blockcpy_pp(m_cwidth, m_cheight, dstU, dststride, srcU, srcstride);
    primitives.blockcpy_pp(m_cwidth, m_cheight, dstV, dststride, srcV, srcstride);
}

void TComYuv::copyPartToYuv(TComYuv* dstPicYuv, uint32_t partIdx)
{
    copyPartToLuma(dstPicYuv, partIdx);
    copyPartToChroma(dstPicYuv, partIdx);
}

void TComYuv::copyPartToLuma(TComYuv* dstPicYuv, uint32_t partIdx)
{
    Pel* src = getLumaAddr(partIdx);
    Pel* dst = dstPicYuv->getLumaAddr(0);

    uint32_t srcstride = getStride();
    uint32_t dststride = dstPicYuv->getStride();

    uint32_t height = dstPicYuv->getHeight();
    uint32_t width = dstPicYuv->getWidth();

    primitives.blockcpy_pp(width, height, dst, dststride, src, srcstride);
}

void TComYuv::copyPartToChroma(TComYuv* dstPicYuv, uint32_t partIdx)
{
    Pel* srcU = getCbAddr(partIdx);
    Pel* srcV = getCrAddr(partIdx);
    Pel* dstU = dstPicYuv->getCbAddr(0);
    Pel* dstV = dstPicYuv->getCrAddr(0);

    uint32_t srcstride = getCStride();
    uint32_t dststride = dstPicYuv->getCStride();

    uint32_t uiCHeight = dstPicYuv->getCHeight();
    uint32_t uiCWidth = dstPicYuv->getCWidth();

    primitives.blockcpy_pp(uiCWidth, uiCHeight, dstU, dststride, srcU, srcstride);
    primitives.blockcpy_pp(uiCWidth, uiCHeight, dstV, dststride, srcV, srcstride);
}

void TComYuv::copyPartToPartYuv(TComYuv* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height, bool bLuma, bool bChroma)
{
    if (bLuma)
        copyPartToPartLuma(dstPicYuv, partIdx, width, height);
    if (bChroma)
        copyPartToPartChroma(dstPicYuv, partIdx, width >> m_hChromaShift, height >> m_vChromaShift);
}

void TComYuv::copyPartToPartYuv(TShortYUV* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height, bool bLuma, bool bChroma)
{
    if (bLuma)
        copyPartToPartLuma(dstPicYuv, partIdx, width, height);
    if (bChroma)
        copyPartToPartChroma(dstPicYuv, partIdx, width >> m_hChromaShift, height >> m_vChromaShift);
}

void TComYuv::copyPartToPartLuma(TComYuv* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height)
{
    Pel* src = getLumaAddr(partIdx);
    Pel* dst = dstPicYuv->getLumaAddr(partIdx);

    if (src == dst) return;

    uint32_t srcstride = getStride();
    uint32_t dststride = dstPicYuv->getStride();

    primitives.blockcpy_pp(width, height, dst, dststride, src, srcstride);
}

void TComYuv::copyPartToPartLuma(TShortYUV* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height)
{
    Pel* src = getLumaAddr(partIdx);
    int16_t* dst = dstPicYuv->getLumaAddr(partIdx);

    uint32_t  srcstride = getStride();
    uint32_t  dststride = dstPicYuv->m_width;

    primitives.blockcpy_sp(width, height, dst, dststride, src, srcstride);
}

void TComYuv::copyPartToPartChroma(TComYuv* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height)
{
    Pel* srcU = getCbAddr(partIdx);
    Pel* srcV = getCrAddr(partIdx);
    Pel* dstU = dstPicYuv->getCbAddr(partIdx);
    Pel* dstV = dstPicYuv->getCrAddr(partIdx);

    if (srcU == dstU && srcV == dstV) return;

    uint32_t srcstride = getCStride();
    uint32_t dststride = dstPicYuv->getCStride();

    primitives.blockcpy_pp(width, height, dstU, dststride, srcU, srcstride);
    primitives.blockcpy_pp(width, height, dstV, dststride, srcV, srcstride);
}

void TComYuv::copyPartToPartChroma(TShortYUV* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height)
{
    Pel*   srcU = getCbAddr(partIdx);
    Pel*   srcV = getCrAddr(partIdx);
    int16_t* dstU = dstPicYuv->getCbAddr(partIdx);
    int16_t* dstV = dstPicYuv->getCrAddr(partIdx);

    uint32_t srcstride = getCStride();
    uint32_t dststride = dstPicYuv->m_cwidth;

    primitives.blockcpy_sp(width, height, dstU, dststride, srcU, srcstride);
    primitives.blockcpy_sp(width, height, dstV, dststride, srcV, srcstride);
}

void TComYuv::copyPartToPartChroma(TComYuv* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height, uint32_t chromaId)
{
    if (chromaId == 0)
    {
        Pel* srcU = getCbAddr(partIdx);
        Pel* dstU = dstPicYuv->getCbAddr(partIdx);
        if (srcU == dstU) return;
        uint32_t srcstride = getCStride();
        uint32_t dststride = dstPicYuv->getCStride();
        primitives.blockcpy_pp(width, height, dstU, dststride, srcU, srcstride);
    }
    else if (chromaId == 1)
    {
        Pel* srcV = getCrAddr(partIdx);
        Pel* dstV = dstPicYuv->getCrAddr(partIdx);
        if (srcV == dstV) return;
        uint32_t srcstride = getCStride();
        uint32_t dststride = dstPicYuv->getCStride();
        primitives.blockcpy_pp(width, height, dstV, dststride, srcV, srcstride);
    }
    else
    {
        Pel* srcU = getCbAddr(partIdx);
        Pel* srcV = getCrAddr(partIdx);
        Pel* dstU = dstPicYuv->getCbAddr(partIdx);
        Pel* dstV = dstPicYuv->getCrAddr(partIdx);
        if (srcU == dstU && srcV == dstV) return;
        uint32_t srcstride = getCStride();
        uint32_t dststride = dstPicYuv->getCStride();
        primitives.blockcpy_pp(width, height, dstU, dststride, srcU, srcstride);
        primitives.blockcpy_pp(width, height, dstV, dststride, srcV, srcstride);
    }
}

void TComYuv::copyPartToPartChroma(TShortYUV* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height, uint32_t chromaId)
{
    if (chromaId == 0)
    {
        Pel*   srcU = getCbAddr(partIdx);
        int16_t* dstU = dstPicYuv->getCbAddr(partIdx);

        uint32_t srcstride = getCStride();
        uint32_t dststride = dstPicYuv->m_cwidth;

        primitives.blockcpy_sp(width, height, dstU, dststride, srcU, srcstride);
    }
    else if (chromaId == 1)
    {
        Pel*  srcV = getCrAddr(partIdx);
        int16_t* dstV = dstPicYuv->getCrAddr(partIdx);

        uint32_t srcstride = getCStride();
        uint32_t dststride = dstPicYuv->m_cwidth;

        primitives.blockcpy_sp(width, height, dstV, dststride, srcV, srcstride);
    }
    else
    {
        Pel*   srcU = getCbAddr(partIdx);
        Pel*   srcV = getCrAddr(partIdx);
        int16_t* dstU = dstPicYuv->getCbAddr(partIdx);
        int16_t* dstV = dstPicYuv->getCrAddr(partIdx);

        uint32_t srcstride = getCStride();
        uint32_t dststride = dstPicYuv->m_cwidth;

        primitives.blockcpy_sp(width, height, dstU, dststride, srcU, srcstride);
        primitives.blockcpy_sp(width, height, dstV, dststride, srcV, srcstride);
    }
}

void TComYuv::addClip(TComYuv* srcYuv0, TComYuv* srcYuv1, uint32_t trUnitIdx, uint32_t partSize)
{
    addClipLuma(srcYuv0, srcYuv1, trUnitIdx, partSize);
    addClipChroma(srcYuv0, srcYuv1, trUnitIdx, partSize >> m_hChromaShift);
}

void TComYuv::addClip(TComYuv* srcYuv0, TShortYUV* srcYuv1, uint32_t trUnitIdx, uint32_t partSize)
{
    addClipLuma(srcYuv0, srcYuv1, trUnitIdx, partSize);
    addClipChroma(srcYuv0, srcYuv1, trUnitIdx, partSize >> m_hChromaShift);
}

void TComYuv::addClipLuma(TComYuv* srcYuv0, TComYuv* srcYuv1, uint32_t trUnitIdx, uint32_t partSize)
{
    Pel* src0 = srcYuv0->getLumaAddr(trUnitIdx, partSize);
    Pel* src1 = srcYuv1->getLumaAddr(trUnitIdx, partSize);
    Pel* dst  = getLumaAddr(trUnitIdx, partSize);

    uint32_t src0Stride = srcYuv0->getStride();
    uint32_t src1Stride = srcYuv1->getStride();
    uint32_t dststride = getStride();

    primitives.pixeladd_pp(partSize, partSize, dst, dststride, src0, src1, src0Stride, src1Stride);
}

void TComYuv::addClipLuma(TComYuv* srcYuv0, TShortYUV* srcYuv1, uint32_t trUnitIdx, uint32_t partSize)
{
    int x, y;

    Pel* src0 = srcYuv0->getLumaAddr(trUnitIdx, partSize);
    int16_t* src1 = srcYuv1->getLumaAddr(trUnitIdx, partSize);
    Pel* dst = getLumaAddr(trUnitIdx, partSize);

    uint32_t src0Stride = srcYuv0->getStride();
    uint32_t src1Stride = srcYuv1->m_width;
    uint32_t dststride  = getStride();

    for (y = partSize - 1; y >= 0; y--)
    {
        for (x = partSize - 1; x >= 0; x--)
        {
            dst[x] = ClipY(static_cast<int16_t>(src0[x]) + src1[x]);
        }

        src0 += src0Stride;
        src1 += src1Stride;
        dst  += dststride;
    }
}

void TComYuv::addClipChroma(TComYuv* srcYuv0, TComYuv* srcYuv1, uint32_t trUnitIdx, uint32_t partSize)
{
    Pel* srcU0 = srcYuv0->getCbAddr(trUnitIdx, partSize);
    Pel* srcU1 = srcYuv1->getCbAddr(trUnitIdx, partSize);
    Pel* srcV0 = srcYuv0->getCrAddr(trUnitIdx, partSize);
    Pel* srcV1 = srcYuv1->getCrAddr(trUnitIdx, partSize);
    Pel* dstU = getCbAddr(trUnitIdx, partSize);
    Pel* dstV = getCrAddr(trUnitIdx, partSize);

    uint32_t src0Stride = srcYuv0->getCStride();
    uint32_t src1Stride = srcYuv1->getCStride();
    uint32_t dststride  = getCStride();

    primitives.pixeladd_pp(partSize, partSize, dstU, dststride, srcU0, srcU1, src0Stride, src1Stride);
    primitives.pixeladd_pp(partSize, partSize, dstV, dststride, srcV0, srcV1, src0Stride, src1Stride);
}

void TComYuv::addClipChroma(TComYuv* srcYuv0, TShortYUV* srcYuv1, uint32_t trUnitIdx, uint32_t partSize)
{
    int x, y;

    Pel* srcU0 = srcYuv0->getCbAddr(trUnitIdx, partSize);
    int16_t* srcU1 = srcYuv1->getCbAddr(trUnitIdx, partSize);
    Pel* srcV0 = srcYuv0->getCrAddr(trUnitIdx, partSize);
    int16_t* srcV1 = srcYuv1->getCrAddr(trUnitIdx, partSize);
    Pel* dstU = getCbAddr(trUnitIdx, partSize);
    Pel* dstV = getCrAddr(trUnitIdx, partSize);

    uint32_t src0Stride = srcYuv0->getCStride();
    uint32_t src1Stride = srcYuv1->m_cwidth;
    uint32_t dststride  = getCStride();

    for (y = partSize - 1; y >= 0; y--)
    {
        for (x = partSize - 1; x >= 0; x--)
        {
            dstU[x] = ClipC(static_cast<int16_t>(srcU0[x]) + srcU1[x]);
            dstV[x] = ClipC(static_cast<int16_t>(srcV0[x]) + srcV1[x]);
        }

        srcU0 += src0Stride;
        srcU1 += src1Stride;
        srcV0 += src0Stride;
        srcV1 += src1Stride;
        dstU  += dststride;
        dstV  += dststride;
    }
}

void TComYuv::subtract(TComYuv* srcYuv0, TComYuv* srcYuv1, uint32_t trUnitIdx, uint32_t partSize)
{
    subtractLuma(srcYuv0, srcYuv1,  trUnitIdx, partSize);
    subtractChroma(srcYuv0, srcYuv1,  trUnitIdx, partSize >> 1);
}

void TComYuv::subtractLuma(TComYuv* srcYuv0, TComYuv* srcYuv1, uint32_t trUnitIdx, uint32_t partSize)
{
    int x, y;

    Pel* src0 = srcYuv0->getLumaAddr(trUnitIdx, partSize);
    Pel* src1 = srcYuv1->getLumaAddr(trUnitIdx, partSize);
    Pel* dst  = getLumaAddr(trUnitIdx, partSize);

    int src0Stride = srcYuv0->getStride();
    int src1Stride = srcYuv1->getStride();
    int dststride  = getStride();

    for (y = partSize - 1; y >= 0; y--)
    {
        for (x = partSize - 1; x >= 0; x--)
        {
            dst[x] = src0[x] - src1[x];
        }

        src0 += src0Stride;
        src1 += src1Stride;
        dst  += dststride;
    }
}

void TComYuv::subtractChroma(TComYuv* srcYuv0, TComYuv* srcYuv1, uint32_t trUnitIdx, uint32_t partSize)
{
    int x, y;

    Pel* srcU0 = srcYuv0->getCbAddr(trUnitIdx, partSize);
    Pel* srcU1 = srcYuv1->getCbAddr(trUnitIdx, partSize);
    Pel* srcV0 = srcYuv0->getCrAddr(trUnitIdx, partSize);
    Pel* srcV1 = srcYuv1->getCrAddr(trUnitIdx, partSize);
    Pel* dstU  = getCbAddr(trUnitIdx, partSize);
    Pel* dstV  = getCrAddr(trUnitIdx, partSize);

    int src0Stride = srcYuv0->getCStride();
    int src1Stride = srcYuv1->getCStride();
    int dststride  = getCStride();

    for (y = partSize - 1; y >= 0; y--)
    {
        for (x = partSize - 1; x >= 0; x--)
        {
            dstU[x] = srcU0[x] - srcU1[x];
            dstV[x] = srcV0[x] - srcV1[x];
        }

        srcU0 += src0Stride;
        srcU1 += src1Stride;
        srcV0 += src0Stride;
        srcV1 += src1Stride;
        dstU  += dststride;
        dstV  += dststride;
    }
}

void TComYuv::addAvg(TComYuv* srcYuv0, TComYuv* srcYuv1, uint32_t partUnitIdx, uint32_t width, uint32_t height, bool bLuma, bool bChroma)
{
    int x, y;
    uint32_t src0Stride, src1Stride, dststride;
    int shiftNum, offset;

    Pel* srcY0 = srcYuv0->getLumaAddr(partUnitIdx);
    Pel* srcU0 = srcYuv0->getCbAddr(partUnitIdx);
    Pel* srcV0 = srcYuv0->getCrAddr(partUnitIdx);

    Pel* srcY1 = srcYuv1->getLumaAddr(partUnitIdx);
    Pel* srcU1 = srcYuv1->getCbAddr(partUnitIdx);
    Pel* srcV1 = srcYuv1->getCrAddr(partUnitIdx);

    Pel* dstY  = getLumaAddr(partUnitIdx);
    Pel* dstU  = getCbAddr(partUnitIdx);
    Pel* dstV  = getCrAddr(partUnitIdx);

    if (bLuma)
    {
        src0Stride = srcYuv0->getStride();
        src1Stride = srcYuv1->getStride();
        dststride  = getStride();
        shiftNum = IF_INTERNAL_PREC + 1 - X265_DEPTH;
        offset = (1 << (shiftNum - 1)) + 2 * IF_INTERNAL_OFFS;

        for (y = 0; y < height; y++)
        {
            for (x = 0; x < width; x += 4)
            {
                dstY[x + 0] = ClipY((srcY0[x + 0] + srcY1[x + 0] + offset) >> shiftNum);
                dstY[x + 1] = ClipY((srcY0[x + 1] + srcY1[x + 1] + offset) >> shiftNum);
                dstY[x + 2] = ClipY((srcY0[x + 2] + srcY1[x + 2] + offset) >> shiftNum);
                dstY[x + 3] = ClipY((srcY0[x + 3] + srcY1[x + 3] + offset) >> shiftNum);
            }

            srcY0 += src0Stride;
            srcY1 += src1Stride;
            dstY  += dststride;
        }
    }
    if (bChroma)
    {
        shiftNum = IF_INTERNAL_PREC + 1 - X265_DEPTH;
        offset = (1 << (shiftNum - 1)) + 2 * IF_INTERNAL_OFFS;

        src0Stride = srcYuv0->getCStride();
        src1Stride = srcYuv1->getCStride();
        dststride  = getCStride();

        width  >>= m_hChromaShift;
        height >>= m_vChromaShift;

        for (y = height - 1; y >= 0; y--)
        {
            for (x = width - 1; x >= 0; )
            {
                // note: chroma min width is 2
                dstU[x] = ClipC((srcU0[x] + srcU1[x] + offset) >> shiftNum);
                dstV[x] = ClipC((srcV0[x] + srcV1[x] + offset) >> shiftNum);
                x--;
                dstU[x] = ClipC((srcU0[x] + srcU1[x] + offset) >> shiftNum);
                dstV[x] = ClipC((srcV0[x] + srcV1[x] + offset) >> shiftNum);
                x--;
            }

            srcU0 += src0Stride;
            srcU1 += src1Stride;
            srcV0 += src0Stride;
            srcV1 += src1Stride;
            dstU  += dststride;
            dstV  += dststride;
        }
    }
}

void TComYuv::addAvg(TShortYUV* srcYuv0, TShortYUV* srcYuv1, uint32_t partUnitIdx, uint32_t width, uint32_t height, bool bLuma, bool bChroma)
{
    int x, y;
    uint32_t src0Stride, src1Stride, dststride;
    int shiftNum, offset;

    int16_t* srcY0 = srcYuv0->getLumaAddr(partUnitIdx);
    int16_t* srcU0 = srcYuv0->getCbAddr(partUnitIdx);
    int16_t* srcV0 = srcYuv0->getCrAddr(partUnitIdx);

    int16_t* srcY1 = srcYuv1->getLumaAddr(partUnitIdx);
    int16_t* srcU1 = srcYuv1->getCbAddr(partUnitIdx);
    int16_t* srcV1 = srcYuv1->getCrAddr(partUnitIdx);

    Pel* dstY = getLumaAddr(partUnitIdx);
    Pel* dstU = getCbAddr(partUnitIdx);
    Pel* dstV = getCrAddr(partUnitIdx);

    if (bLuma)
    {
        src0Stride = srcYuv0->m_width;
        src1Stride = srcYuv1->m_width;
        dststride  = getStride();
        shiftNum = IF_INTERNAL_PREC + 1 - X265_DEPTH;
        offset = (1 << (shiftNum - 1)) + 2 * IF_INTERNAL_OFFS;

        for (y = 0; y < height; y++)
        {
            for (x = 0; x < width; x += 4)
            {
                dstY[x + 0] = ClipY((srcY0[x + 0] + srcY1[x + 0] + offset) >> shiftNum);
                dstY[x + 1] = ClipY((srcY0[x + 1] + srcY1[x + 1] + offset) >> shiftNum);
                dstY[x + 2] = ClipY((srcY0[x + 2] + srcY1[x + 2] + offset) >> shiftNum);
                dstY[x + 3] = ClipY((srcY0[x + 3] + srcY1[x + 3] + offset) >> shiftNum);
            }

            srcY0 += src0Stride;
            srcY1 += src1Stride;
            dstY  += dststride;
        }
    }
    if (bChroma)
    {
        shiftNum = IF_INTERNAL_PREC + 1 - X265_DEPTH;
        offset = (1 << (shiftNum - 1)) + 2 * IF_INTERNAL_OFFS;

        src0Stride = srcYuv0->m_cwidth;
        src1Stride = srcYuv1->m_cwidth;
        dststride  = getCStride();

        width  >>= m_hChromaShift;
        height >>= m_vChromaShift;

        for (y = height - 1; y >= 0; y--)
        {
            for (x = width - 1; x >= 0; )
            {
                // note: chroma min width is 2
                dstU[x] = ClipC((srcU0[x] + srcU1[x] + offset) >> shiftNum);
                dstV[x] = ClipC((srcV0[x] + srcV1[x] + offset) >> shiftNum);
                x--;
                dstU[x] = ClipC((srcU0[x] + srcU1[x] + offset) >> shiftNum);
                dstV[x] = ClipC((srcV0[x] + srcV1[x] + offset) >> shiftNum);
                x--;
            }

            srcU0 += src0Stride;
            srcU1 += src1Stride;
            srcV0 += src0Stride;
            srcV1 += src1Stride;
            dstU  += dststride;
            dstV  += dststride;
        }
    }
}

#define DISABLING_CLIP_FOR_BIPREDME 0  // x265 disables this flag so 8bpp and 16bpp outputs match
                                       // the intent is for all HM bipred to be replaced with x264 logic

void TComYuv::removeHighFreq(TComYuv* srcYuv, uint32_t partIdx, uint32_t width, uint32_t height)
{
    int x, y;

    Pel* src  = srcYuv->getLumaAddr(partIdx);
    Pel* srcU = srcYuv->getCbAddr(partIdx);
    Pel* srcV = srcYuv->getCrAddr(partIdx);

    Pel* dst  = getLumaAddr(partIdx);
    Pel* dstU = getCbAddr(partIdx);
    Pel* dstV = getCrAddr(partIdx);

    int srcstride = srcYuv->getStride();
    int dststride = getStride();

    for (y = height - 1; y >= 0; y--)
    {
        for (x = width - 1; x >= 0; x--)
        {
#if DISABLING_CLIP_FOR_BIPREDME
            dst[x] = (dst[x] << 1) - src[x];
#else
            dst[x] = ClipY((dst[x] << 1) - src[x]);
#endif
        }

        src += srcstride;
        dst += dststride;
    }

    srcstride = srcYuv->getCStride();
    dststride = getCStride();

    width  >>= m_hChromaShift;
    height >>= m_vChromaShift;

    for (y = height - 1; y >= 0; y--)
    {
        for (x = width - 1; x >= 0; x--)
        {
#if DISABLING_CLIP_FOR_BIPREDME
            dstU[x] = (dstU[x] << 1) - srcU[x];
            dstV[x] = (dstV[x] << 1) - srcV[x];
#else
            dstU[x] = ClipC((dstU[x] << 1) - srcU[x]);
            dstV[x] = ClipC((dstV[x] << 1) - srcV[x]);
#endif
        }

        srcU += srcstride;
        srcV += srcstride;
        dstU += dststride;
        dstV += dststride;
    }
}

//! \}
