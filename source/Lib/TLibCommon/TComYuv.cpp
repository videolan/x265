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

#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <math.h>

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

Void TComYuv::create(UInt width, UInt height)
{
    // memory allocation
    m_bufY = (Pel*)xMalloc(Pel, width * height);
    m_bufU = (Pel*)xMalloc(Pel, width * height >> 2);
    m_bufV = (Pel*)xMalloc(Pel, width * height >> 2);

    // set width and height
    m_width   = width;
    m_height  = height;
    m_cwidth  = width  >> 1;
    m_cheight = height >> 1;
}

Void TComYuv::destroy()
{
    // memory free
    xFree(m_bufY);
    m_bufY = NULL;
    xFree(m_bufU);
    m_bufU = NULL;
    xFree(m_bufV);
    m_bufV = NULL;
}

Void TComYuv::clear()
{
    ::memset(m_bufY, 0, (m_width  * m_height) * sizeof(Pel));
    ::memset(m_bufU, 0, (m_cwidth * m_cheight) * sizeof(Pel));
    ::memset(m_bufV, 0, (m_cwidth * m_cheight) * sizeof(Pel));
}

Void TComYuv::copyToPicYuv(TComPicYuv* destPicYuv, UInt cuAddr, UInt absZOrderIdx, UInt partDepth, UInt partIdx)
{
    copyToPicLuma(destPicYuv, cuAddr, absZOrderIdx, partDepth, partIdx);
    copyToPicChroma(destPicYuv, cuAddr, absZOrderIdx, partDepth, partIdx);
}

Void TComYuv::copyToPicLuma(TComPicYuv* destPicYuv, UInt cuAddr, UInt absZOrderIdx, UInt partDepth, UInt partIdx)
{
    Int width, height;

    width  = m_width >> partDepth;
    height = m_height >> partDepth;

    Pel* src = getLumaAddr(partIdx, width);
    Pel* dst = destPicYuv->getLumaAddr(cuAddr, absZOrderIdx);

    UInt srcstride = getStride();
    UInt dststride = destPicYuv->getStride();

    x265::primitives.blockcpy_pp(width, height, dst, dststride, src, srcstride);
}

Void TComYuv::copyToPicChroma(TComPicYuv* destPicYuv, UInt cuAddr, UInt absZOrderIdx, UInt partDepth, UInt partIdx)
{
    Int width, height;

    width  = m_cwidth >> partDepth;
    height = m_cheight >> partDepth;

    Pel* srcU = getCbAddr(partIdx, width);
    Pel* srcV = getCrAddr(partIdx, width);
    Pel* dstU = destPicYuv->getCbAddr(cuAddr, absZOrderIdx);
    Pel* dstV = destPicYuv->getCrAddr(cuAddr, absZOrderIdx);

    UInt srcstride = getCStride();
    UInt dststride = destPicYuv->getCStride();

    x265::primitives.blockcpy_pp(width, height, dstU, dststride, srcU, srcstride);
    x265::primitives.blockcpy_pp(width, height, dstV, dststride, srcV, srcstride);
}

Void TComYuv::copyFromPicYuv(TComPicYuv* srcPicYuv, UInt cuAddr, UInt absZOrderIdx)
{
    copyFromPicLuma(srcPicYuv, cuAddr, absZOrderIdx);
    copyFromPicChroma(srcPicYuv, cuAddr, absZOrderIdx);
}

Void TComYuv::copyFromPicLuma(TComPicYuv* srcPicYuv, UInt cuAddr, UInt absZOrderIdx)
{
    Pel* dst = m_bufY;
    Pel* src = srcPicYuv->getLumaAddr(cuAddr, absZOrderIdx);

    UInt dststride = getStride();
    UInt srcstride = srcPicYuv->getStride();

    x265::primitives.blockcpy_pp(m_width, m_height, dst, dststride, src, srcstride);
}

Void TComYuv::copyFromPicChroma(TComPicYuv* srcPicYuv, UInt cuAddr, UInt absZOrderIdx)
{
    Pel* dstU = m_bufU;
    Pel* dstV = m_bufV;
    Pel* srcU = srcPicYuv->getCbAddr(cuAddr, absZOrderIdx);
    Pel* srcV = srcPicYuv->getCrAddr(cuAddr, absZOrderIdx);

    UInt dststride = getCStride();
    UInt srcstride = srcPicYuv->getCStride();

    x265::primitives.blockcpy_pp(m_cwidth, m_cheight, dstU, dststride, srcU, srcstride);
    x265::primitives.blockcpy_pp(m_cwidth, m_cheight, dstV, dststride, srcV, srcstride);
}

Void TComYuv::copyToPartYuv(TComYuv* dstPicYuv, UInt uiDstPartIdx)
{
    copyToPartLuma(dstPicYuv, uiDstPartIdx);
    copyToPartChroma(dstPicYuv, uiDstPartIdx);
}

Void TComYuv::copyToPartLuma(TComYuv* dstPicYuv, UInt uiDstPartIdx)
{
    Pel* src = m_bufY;
    Pel* dst = dstPicYuv->getLumaAddr(uiDstPartIdx);

    UInt srcstride = getStride();
    UInt dststride = dstPicYuv->getStride();

    x265::primitives.blockcpy_pp(m_width, m_height, dst, dststride, src, srcstride);
}

Void TComYuv::copyToPartChroma(TComYuv* dstPicYuv, UInt uiDstPartIdx)
{
    Pel* srcU = m_bufU;
    Pel* srcV = m_bufV;
    Pel* dstU = dstPicYuv->getCbAddr(uiDstPartIdx);
    Pel* dstV = dstPicYuv->getCrAddr(uiDstPartIdx);

    UInt srcstride = getCStride();
    UInt dststride = dstPicYuv->getCStride();

    x265::primitives.blockcpy_pp(m_cwidth, m_cheight, dstU, dststride, srcU, srcstride);
    x265::primitives.blockcpy_pp(m_cwidth, m_cheight, dstV, dststride, srcV, srcstride);
}

Void TComYuv::copyPartToYuv(TComYuv* dstPicYuv, UInt uiSrcPartIdx)
{
    copyPartToLuma(dstPicYuv, uiSrcPartIdx);
    copyPartToChroma(dstPicYuv, uiSrcPartIdx);
}

Void TComYuv::copyPartToLuma(TComYuv* dstPicYuv, UInt uiSrcPartIdx)
{
    Pel* src = getLumaAddr(uiSrcPartIdx);
    Pel* dst = dstPicYuv->getLumaAddr(0);

    UInt srcstride = getStride();
    UInt dststride = dstPicYuv->getStride();

    UInt height = dstPicYuv->getHeight();
    UInt width = dstPicYuv->getWidth();

    x265::primitives.blockcpy_pp(width, height, dst, dststride, src, srcstride);
}

Void TComYuv::copyPartToChroma(TComYuv* dstPicYuv, UInt uiSrcPartIdx)
{
    Pel* srcU = getCbAddr(uiSrcPartIdx);
    Pel* srcV = getCrAddr(uiSrcPartIdx);
    Pel* dstU = dstPicYuv->getCbAddr(0);
    Pel* dstV = dstPicYuv->getCrAddr(0);

    UInt srcstride = getCStride();
    UInt dststride = dstPicYuv->getCStride();

    UInt uiCHeight = dstPicYuv->getCHeight();
    UInt uiCWidth = dstPicYuv->getCWidth();

    x265::primitives.blockcpy_pp(uiCWidth, uiCHeight, dstU, dststride, srcU, srcstride);
    x265::primitives.blockcpy_pp(uiCWidth, uiCHeight, dstV, dststride, srcV, srcstride);
}

Void TComYuv::copyPartToPartYuv(TComYuv* dstPicYuv, UInt partIdx, UInt width, UInt height)
{
    copyPartToPartLuma(dstPicYuv, partIdx, width, height);
    copyPartToPartChroma(dstPicYuv, partIdx, width >> 1, height >> 1);
}

Void TComYuv::copyPartToPartYuv(TShortYUV* dstPicYuv, UInt partIdx, UInt width, UInt height)
{
    copyPartToPartLuma(dstPicYuv, partIdx, width, height);
    copyPartToPartChroma(dstPicYuv, partIdx, width >> 1, height >> 1);
}

Void TComYuv::copyPartToPartLuma(TComYuv* dstPicYuv, UInt partIdx, UInt width, UInt height)
{
    Pel* src = getLumaAddr(partIdx);
    Pel* dst = dstPicYuv->getLumaAddr(partIdx);
    if (src == dst) return;

    UInt srcstride = getStride();
    UInt dststride = dstPicYuv->getStride();

    x265::primitives.blockcpy_pp(width, height, dst, dststride, src, srcstride);
}

Void TComYuv::copyPartToPartLuma(TShortYUV* dstPicYuv, UInt partIdx, UInt width, UInt height)
{
    Pel* src = getLumaAddr(partIdx);
    Short* dst = dstPicYuv->getLumaAddr(partIdx);

    UInt  srcstride = getStride();
    UInt  dststride = dstPicYuv->width;

    x265::primitives.blockcpy_sp(width, height, dst, dststride, src, srcstride);
}

Void TComYuv::copyPartToPartChroma(TComYuv* dstPicYuv, UInt partIdx, UInt width, UInt height)
{
    Pel* srcU = getCbAddr(partIdx);
    Pel* srcV = getCrAddr(partIdx);
    Pel* dstU = dstPicYuv->getCbAddr(partIdx);
    Pel* dstV = dstPicYuv->getCrAddr(partIdx);
    if (srcU == dstU && srcV == dstV) return;

    UInt srcstride = getCStride();
    UInt dststride = dstPicYuv->getCStride();

    x265::primitives.blockcpy_pp(width, height, dstU, dststride, srcU, srcstride);
    x265::primitives.blockcpy_pp(width, height, dstV, dststride, srcV, srcstride);
}

Void TComYuv::copyPartToPartChroma(TShortYUV* dstPicYuv, UInt partIdx, UInt width, UInt height)
{
    Pel*   srcU = getCbAddr(partIdx);
    Pel*   srcV = getCrAddr(partIdx);
    Short* dstU = dstPicYuv->getCbAddr(partIdx);
    Short* dstV = dstPicYuv->getCrAddr(partIdx);

    UInt srcstride = getCStride();
    UInt dststride = dstPicYuv->Cwidth;

    x265::primitives.blockcpy_sp(width, height, dstU, dststride, srcU, srcstride);
    x265::primitives.blockcpy_sp(width, height, dstV, dststride, srcV, srcstride);
}

Void TComYuv::copyPartToPartChroma(TComYuv* dstPicYuv, UInt partIdx, UInt width, UInt height, UInt chromaId)
{
    if (chromaId == 0)
    {
        Pel* srcU = getCbAddr(partIdx);
        Pel* dstU = dstPicYuv->getCbAddr(partIdx);
        if (srcU == dstU) return;
        UInt srcstride = getCStride();
        UInt dststride = dstPicYuv->getCStride();
        x265::primitives.blockcpy_pp(width, height, dstU, dststride, srcU, srcstride);
    }
    else if (chromaId == 1)
    {
        Pel* srcV = getCrAddr(partIdx);
        Pel* dstV = dstPicYuv->getCrAddr(partIdx);
        if (srcV == dstV) return;
        UInt srcstride = getCStride();
        UInt dststride = dstPicYuv->getCStride();
        x265::primitives.blockcpy_pp(width, height, dstV, dststride, srcV, srcstride);
    }
    else
    {
        Pel* srcU = getCbAddr(partIdx);
        Pel* srcV = getCrAddr(partIdx);
        Pel* dstU = dstPicYuv->getCbAddr(partIdx);
        Pel* dstV = dstPicYuv->getCrAddr(partIdx);
        if (srcU == dstU && srcV == dstV) return;
        UInt srcstride = getCStride();
        UInt dststride = dstPicYuv->getCStride();
        x265::primitives.blockcpy_pp(width, height, dstU, dststride, srcU, srcstride);
        x265::primitives.blockcpy_pp(width, height, dstV, dststride, srcV, srcstride);
    }
}

Void TComYuv::copyPartToPartChroma(TShortYUV* dstPicYuv, UInt partIdx, UInt width, UInt height, UInt chromaId)
{
    if (chromaId == 0)
    {
        Pel*   srcU = getCbAddr(partIdx);
        Short* dstU = dstPicYuv->getCbAddr(partIdx);

        UInt srcstride = getCStride();
        UInt dststride = dstPicYuv->Cwidth;

        x265::primitives.blockcpy_sp(width, height, dstU, dststride, srcU, srcstride);
    }
    else if (chromaId == 1)
    {
        Pel*  srcV = getCrAddr(partIdx);
        Short* dstV = dstPicYuv->getCrAddr(partIdx);

        UInt srcstride = getCStride();
        UInt dststride = dstPicYuv->Cwidth;

        x265::primitives.blockcpy_sp(width, height, dstV, dststride, srcV, srcstride);
    }
    else
    {
        Pel*   srcU = getCbAddr(partIdx);
        Pel*   srcV = getCrAddr(partIdx);
        Short* dstU = dstPicYuv->getCbAddr(partIdx);
        Short* dstV = dstPicYuv->getCrAddr(partIdx);

        UInt srcstride = getCStride();
        UInt dststride = dstPicYuv->Cwidth;

        x265::primitives.blockcpy_sp(width, height, dstU, dststride, srcU, srcstride);
        x265::primitives.blockcpy_sp(width, height, dstV, dststride, srcV, srcstride);
    }
}

Void TComYuv::addClip(TComYuv* srcYuv0, TComYuv* srcYuv1, UInt trUnitIdx, UInt partSize)
{
    addClipLuma(srcYuv0, srcYuv1, trUnitIdx, partSize);
    addClipChroma(srcYuv0, srcYuv1, trUnitIdx, partSize >> 1);
}

Void TComYuv::addClip(TComYuv* srcYuv0, TShortYUV* srcYuv1, UInt trUnitIdx, UInt partSize)
{
    addClipLuma(srcYuv0, srcYuv1, trUnitIdx, partSize);
    addClipChroma(srcYuv0, srcYuv1, trUnitIdx, partSize >> 1);
}

Void TComYuv::addClipLuma(TComYuv* srcYuv0, TComYuv* srcYuv1, UInt trUnitIdx, UInt partSize)
{
    Int x, y;

    Pel* src0 = srcYuv0->getLumaAddr(trUnitIdx, partSize);
    Pel* src1 = srcYuv1->getLumaAddr(trUnitIdx, partSize);
    Pel* dst  = getLumaAddr(trUnitIdx, partSize);

    UInt iSrc0Stride = srcYuv0->getStride();
    UInt iSrc1Stride = srcYuv1->getStride();
    UInt dststride = getStride();

    for (y = partSize - 1; y >= 0; y--)
    {
        for (x = partSize - 1; x >= 0; x--)
        {
            dst[x] = ClipY(static_cast<Short>(src0[x]) + static_cast<Short>(src1[x]));
        }

        src0 += iSrc0Stride;
        src1 += iSrc1Stride;
        dst  += dststride;
    }
}

Void TComYuv::addClipLuma(TComYuv* srcYuv0, TShortYUV* srcYuv1, UInt trUnitIdx, UInt partSize)
{
    Int x, y;

    Pel* src0 = srcYuv0->getLumaAddr(trUnitIdx, partSize);
    Short* src1 = srcYuv1->getLumaAddr(trUnitIdx, partSize);
    Pel* dst = getLumaAddr(trUnitIdx, partSize);

    UInt iSrc0Stride = srcYuv0->getStride();
    UInt iSrc1Stride = srcYuv1->width;
    UInt dststride  = getStride();

    for (y = partSize - 1; y >= 0; y--)
    {
        for (x = partSize - 1; x >= 0; x--)
        {
            dst[x] = ClipY(static_cast<Short>(src0[x]) + src1[x]);
        }

        src0 += iSrc0Stride;
        src1 += iSrc1Stride;
        dst  += dststride;
    }
}

Void TComYuv::addClipChroma(TComYuv* srcYuv0, TComYuv* srcYuv1, UInt trUnitIdx, UInt partSize)
{
    Int x, y;

    Pel* srcU0 = srcYuv0->getCbAddr(trUnitIdx, partSize);
    Pel* srcU1 = srcYuv1->getCbAddr(trUnitIdx, partSize);
    Pel* srcV0 = srcYuv0->getCrAddr(trUnitIdx, partSize);
    Pel* srcV1 = srcYuv1->getCrAddr(trUnitIdx, partSize);
    Pel* dstU = getCbAddr(trUnitIdx, partSize);
    Pel* dstV = getCrAddr(trUnitIdx, partSize);

    UInt src0Stride = srcYuv0->getCStride();
    UInt src1Stride = srcYuv1->getCStride();
    UInt dststride  = getCStride();

    for (y = partSize - 1; y >= 0; y--)
    {
        for (x = partSize - 1; x >= 0; x--)
        {
            dstU[x] = ClipC(static_cast<Short>(srcU0[x]) + srcU1[x]);
            dstV[x] = ClipC(static_cast<Short>(srcV0[x]) + srcV1[x]);
        }

        srcU0 += src0Stride;
        srcU1 += src1Stride;
        srcV0 += src0Stride;
        srcV1 += src1Stride;
        dstU  += dststride;
        dstV  += dststride;
    }
}

Void TComYuv::addClipChroma(TComYuv* srcYuv0, TShortYUV* srcYuv1, UInt trUnitIdx, UInt partSize)
{
    Int x, y;

    Pel* srcU0 = srcYuv0->getCbAddr(trUnitIdx, partSize);
    Short* srcU1 = srcYuv1->getCbAddr(trUnitIdx, partSize);
    Pel* srcV0 = srcYuv0->getCrAddr(trUnitIdx, partSize);
    Short* srcV1 = srcYuv1->getCrAddr(trUnitIdx, partSize);
    Pel* dstU = getCbAddr(trUnitIdx, partSize);
    Pel* dstV = getCrAddr(trUnitIdx, partSize);

    UInt src0Stride = srcYuv0->getCStride();
    UInt src1Stride = srcYuv1->Cwidth;
    UInt dststride  = getCStride();

    for (y = partSize - 1; y >= 0; y--)
    {
        for (x = partSize - 1; x >= 0; x--)
        {
            dstU[x] = ClipC(static_cast<Short>(srcU0[x]) + srcU1[x]);
            dstV[x] = ClipC(static_cast<Short>(srcV0[x]) + srcV1[x]);
        }

        srcU0 += src0Stride;
        srcU1 += src1Stride;
        srcV0 += src0Stride;
        srcV1 += src1Stride;
        dstU  += dststride;
        dstV  += dststride;
    }
}

Void TComYuv::subtract(TComYuv* srcYuv0, TComYuv* srcYuv1, UInt trUnitIdx, UInt partSize)
{
    subtractLuma(srcYuv0, srcYuv1,  trUnitIdx, partSize);
    subtractChroma(srcYuv0, srcYuv1,  trUnitIdx, partSize >> 1);
}

Void TComYuv::subtractLuma(TComYuv* srcYuv0, TComYuv* srcYuv1, UInt trUnitIdx, UInt partSize)
{
    Int x, y;

    Pel* src0 = srcYuv0->getLumaAddr(trUnitIdx, partSize);
    Pel* src1 = srcYuv1->getLumaAddr(trUnitIdx, partSize);
    Pel* dst  = getLumaAddr(trUnitIdx, partSize);

    Int src0Stride = srcYuv0->getStride();
    Int src1Stride = srcYuv1->getStride();
    Int dststride  = getStride();

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

Void TComYuv::subtractChroma(TComYuv* srcYuv0, TComYuv* srcYuv1, UInt trUnitIdx, UInt partSize)
{
    Int x, y;

    Pel* srcU0 = srcYuv0->getCbAddr(trUnitIdx, partSize);
    Pel* srcU1 = srcYuv1->getCbAddr(trUnitIdx, partSize);
    Pel* srcV0 = srcYuv0->getCrAddr(trUnitIdx, partSize);
    Pel* srcV1 = srcYuv1->getCrAddr(trUnitIdx, partSize);
    Pel* dstU  = getCbAddr(trUnitIdx, partSize);
    Pel* dstV  = getCrAddr(trUnitIdx, partSize);

    Int src0Stride = srcYuv0->getCStride();
    Int src1Stride = srcYuv1->getCStride();
    Int dststride  = getCStride();

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

Void TComYuv::addAvg(TComYuv* srcYuv0, TComYuv* srcYuv1, UInt partUnitIdx, UInt width, UInt height)
{
    Int x, y;

    Pel* srcY0  = srcYuv0->getLumaAddr(partUnitIdx);
    Pel* srcU0  = srcYuv0->getCbAddr(partUnitIdx);
    Pel* srcV0  = srcYuv0->getCrAddr(partUnitIdx);

    Pel* srcY1  = srcYuv1->getLumaAddr(partUnitIdx);
    Pel* srcU1  = srcYuv1->getCbAddr(partUnitIdx);
    Pel* srcV1  = srcYuv1->getCrAddr(partUnitIdx);

    Pel* dstY   = getLumaAddr(partUnitIdx);
    Pel* dstU   = getCbAddr(partUnitIdx);
    Pel* dstV   = getCrAddr(partUnitIdx);

    UInt src0Stride = srcYuv0->getStride();
    UInt src1Stride = srcYuv1->getStride();
    UInt dststride  = getStride();
    Int shiftNum = IF_INTERNAL_PREC + 1 - X265_DEPTH;
    Int offset = (1 << (shiftNum - 1)) + 2 * IF_INTERNAL_OFFS;

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

    shiftNum = IF_INTERNAL_PREC + 1 - X265_DEPTH;
    offset = (1 << (shiftNum - 1)) + 2 * IF_INTERNAL_OFFS;

    src0Stride = srcYuv0->getCStride();
    src1Stride = srcYuv1->getCStride();
    dststride  = getCStride();

    width  >>= 1;
    height >>= 1;

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

Void TComYuv::addAvg(TShortYUV* srcYuv0, TShortYUV* srcYuv1, UInt partUnitIdx, UInt width, UInt height)
{
    Int x, y;

    Short* srcY0 = srcYuv0->getLumaAddr(partUnitIdx);
    Short* srcU0 = srcYuv0->getCbAddr(partUnitIdx);
    Short* srcV0 = srcYuv0->getCrAddr(partUnitIdx);

    Short* srcY1 = srcYuv1->getLumaAddr(partUnitIdx);
    Short* srcU1 = srcYuv1->getCbAddr(partUnitIdx);
    Short* srcV1 = srcYuv1->getCrAddr(partUnitIdx);

    Pel* dstY = getLumaAddr(partUnitIdx);
    Pel* dstU  = getCbAddr(partUnitIdx);
    Pel* dstV  = getCrAddr(partUnitIdx);

    UInt src0Stride = srcYuv0->width;
    UInt src1Stride = srcYuv1->width;
    UInt dststride  = getStride();
    Int shiftNum = IF_INTERNAL_PREC + 1 - X265_DEPTH;
    Int offset = (1 << (shiftNum - 1)) + 2 * IF_INTERNAL_OFFS;

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

    shiftNum = IF_INTERNAL_PREC + 1 - X265_DEPTH;
    offset = (1 << (shiftNum - 1)) + 2 * IF_INTERNAL_OFFS;

    src0Stride = srcYuv0->Cwidth;
    src1Stride = srcYuv1->Cwidth;
    dststride  = getCStride();

    width  >>= 1;
    height >>= 1;

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

#define DISABLING_CLIP_FOR_BIPREDME 0  // x265 disables this flag so 8bpp and 16bpp outputs match
                                       // the intent is for all HM bipred to be replaced with x264 logic

Void TComYuv::removeHighFreq(TComYuv* srcYuv, UInt partIdx, UInt widht, UInt height)
{
    Int x, y;

    Pel* src  = srcYuv->getLumaAddr(partIdx);
    Pel* srcU = srcYuv->getCbAddr(partIdx);
    Pel* srcV = srcYuv->getCrAddr(partIdx);

    Pel* dst  = getLumaAddr(partIdx);
    Pel* dstU = getCbAddr(partIdx);
    Pel* dstV = getCrAddr(partIdx);

    Int srcstride = srcYuv->getStride();
    Int dststride = getStride();

    for (y = height - 1; y >= 0; y--)
    {
        for (x = widht - 1; x >= 0; x--)
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

    height >>= 1;
    widht  >>= 1;

    for (y = height - 1; y >= 0; y--)
    {
        for (x = widht - 1; x >= 0; x--)
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
