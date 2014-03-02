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

bool TComYuv::create(uint32_t width, uint32_t height, int csp)
{
    m_hChromaShift = CHROMA_H_SHIFT(csp);
    m_vChromaShift = CHROMA_V_SHIFT(csp);

    // set width and height
    m_width   = width;
    m_height  = height;

    m_cwidth  = width  >> m_hChromaShift;
    m_cheight = height >> m_vChromaShift;

    m_csp = csp;
    m_part = partitionFromSizes(m_width, m_height);

    // memory allocation (padded for SIMD reads)
    CHECKED_MALLOC(m_bufY, pixel, width * height);
    CHECKED_MALLOC(m_bufU, pixel, m_cwidth * m_cheight + 8);
    CHECKED_MALLOC(m_bufV, pixel, m_cwidth * m_cheight + 8);
    return true;

fail:
    return false;
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
    ::memset(m_bufY, 0, (m_width  * m_height) * sizeof(pixel));
    ::memset(m_bufU, 0, (m_cwidth * m_cheight) * sizeof(pixel));
    ::memset(m_bufV, 0, (m_cwidth * m_cheight) * sizeof(pixel));
}

void TComYuv::copyToPicYuv(TComPicYuv* destPicYuv, uint32_t cuAddr, uint32_t absZOrderIdx, uint32_t depth, uint32_t partIdx)
{
    int width = m_width >> depth;
    int part = partitionFromSizes(width, m_height >> depth);
    pixel* srcY = getLumaAddr(partIdx, width);
    pixel* dstY = destPicYuv->getLumaAddr(cuAddr, absZOrderIdx);

    primitives.luma_copy_pp[part](dstY, destPicYuv->getStride(), srcY, getStride());

    width = m_cwidth >> depth;
    pixel* srcU = getCbAddr(partIdx, width);
    pixel* srcV = getCrAddr(partIdx, width);
    pixel* dstU = destPicYuv->getCbAddr(cuAddr, absZOrderIdx);
    pixel* dstV = destPicYuv->getCrAddr(cuAddr, absZOrderIdx);
    primitives.chroma[m_csp].copy_pp[part](dstU, destPicYuv->getCStride(), srcU, getCStride());
    primitives.chroma[m_csp].copy_pp[part](dstV, destPicYuv->getCStride(), srcV, getCStride());
}

void TComYuv::copyFromPicYuv(TComPicYuv* srcPicYuv, uint32_t cuAddr, uint32_t absZOrderIdx)
{
    pixel* srcY = srcPicYuv->getLumaAddr(cuAddr, absZOrderIdx);

    primitives.luma_copy_pp[m_part](m_bufY, getStride(), srcY, srcPicYuv->getStride());

    pixel* srcU = srcPicYuv->getCbAddr(cuAddr, absZOrderIdx);
    pixel* srcV = srcPicYuv->getCrAddr(cuAddr, absZOrderIdx);
    primitives.chroma[m_csp].copy_pp[m_part](m_bufU, getCStride(), srcU, srcPicYuv->getCStride());
    primitives.chroma[m_csp].copy_pp[m_part](m_bufV, getCStride(), srcV, srcPicYuv->getCStride());
}

void TComYuv::copyToPartYuv(TComYuv* dstPicYuv, uint32_t partIdx)
{
    pixel* dstY = dstPicYuv->getLumaAddr(partIdx);

    primitives.luma_copy_pp[m_part](dstY, dstPicYuv->getStride(), m_bufY, getStride());

    pixel* dstU = dstPicYuv->getCbAddr(partIdx);
    pixel* dstV = dstPicYuv->getCrAddr(partIdx);
    primitives.chroma[m_csp].copy_pp[m_part](dstU, dstPicYuv->getCStride(), m_bufU, getCStride());
    primitives.chroma[m_csp].copy_pp[m_part](dstV, dstPicYuv->getCStride(), m_bufV, getCStride());
}

void TComYuv::copyPartToYuv(TComYuv* dstPicYuv, uint32_t partIdx)
{
    pixel* srcY = getLumaAddr(partIdx);
    pixel* dstY = dstPicYuv->getLumaAddr(0);

    int part = dstPicYuv->m_part;
    primitives.luma_copy_pp[part](dstY, dstPicYuv->getStride(), srcY, getStride());

    pixel* srcU = getCbAddr(partIdx);
    pixel* srcV = getCrAddr(partIdx);
    pixel* dstU = dstPicYuv->getCbAddr(0);
    pixel* dstV = dstPicYuv->getCrAddr(0);
    primitives.chroma[m_csp].copy_pp[part](dstU, dstPicYuv->getCStride(), srcU, getCStride());
    primitives.chroma[m_csp].copy_pp[part](dstV, dstPicYuv->getCStride(), srcV, getCStride());
}

void TComYuv::copyPartToPartYuv(TComYuv* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height, bool bLuma, bool bChroma)
{
    int part = partitionFromSizes(width, height);

    assert(width != 4 || height != 4);

    if (bLuma)
    {
        pixel* src = getLumaAddr(partIdx);
        pixel* dst = dstPicYuv->getLumaAddr(partIdx);

        uint32_t srcstride = getStride();
        uint32_t dststride = dstPicYuv->getStride();

        primitives.luma_copy_pp[part](dst, dststride, src, srcstride);
    }
    if (bChroma)
    {
        pixel* srcU = getCbAddr(partIdx);
        pixel* srcV = getCrAddr(partIdx);
        pixel* dstU = dstPicYuv->getCbAddr(partIdx);
        pixel* dstV = dstPicYuv->getCrAddr(partIdx);

        uint32_t srcstride = getCStride();
        uint32_t dststride = dstPicYuv->getCStride();

        primitives.chroma[m_csp].copy_pp[part](dstU, dststride, srcU, srcstride);
        primitives.chroma[m_csp].copy_pp[part](dstV, dststride, srcV, srcstride);
    }
}

void TComYuv::copyPartToPartShort(TShortYUV* dstPicYuv, uint32_t partIdx, uint32_t lumaSize, bool bChroma, bool bChromaSame)
{
    int part = partitionFromSizes(lumaSize, lumaSize);

    int16_t* dst = dstPicYuv->getLumaAddr(partIdx);
    uint32_t dststride = dstPicYuv->m_width;

    primitives.luma_copy_ps[part](dst, dststride, getLumaAddr(partIdx), getStride());

    if (bChroma)
    {
        int16_t* dstU = dstPicYuv->getCbAddr(partIdx);
        int16_t* dstV = dstPicYuv->getCrAddr(partIdx);
        dststride = dstPicYuv->m_cwidth;

        if (bChromaSame)
        {
            primitives.luma_copy_ps[part](dstU, dststride, getCbAddr(partIdx), getCStride());
            primitives.luma_copy_ps[part](dstV, dststride, getCrAddr(partIdx), getCStride());
        }
        else
        {
            primitives.chroma[m_csp].copy_ps[part](dstU, dststride, getCbAddr(partIdx), getCStride());
            primitives.chroma[m_csp].copy_ps[part](dstV, dststride, getCrAddr(partIdx), getCStride());
        }
    }
}

void TComYuv::copyPartToPartChroma(TShortYUV* dstPicYuv, uint32_t partIdx, uint32_t lumaSize, uint32_t chromaId)
{
    int part = partitionFromSizes(lumaSize, lumaSize);

    assert(lumaSize != 4);

    if (chromaId == 0)
    {
        pixel*   srcU = getCbAddr(partIdx);
        int16_t* dstU = dstPicYuv->getCbAddr(partIdx);

        uint32_t srcstride = getCStride();
        uint32_t dststride = dstPicYuv->m_cwidth;

        primitives.chroma[m_csp].copy_ps[part](dstU, dststride, srcU, srcstride);
    }
    else if (chromaId == 1)
    {
        pixel*   srcV = getCrAddr(partIdx);
        int16_t* dstV = dstPicYuv->getCrAddr(partIdx);

        uint32_t srcstride = getCStride();
        uint32_t dststride = dstPicYuv->m_cwidth;

        primitives.chroma[m_csp].copy_ps[part](dstV, dststride, srcV, srcstride);
    }
    else
    {
        pixel*   srcU = getCbAddr(partIdx);
        pixel*   srcV = getCrAddr(partIdx);
        int16_t* dstU = dstPicYuv->getCbAddr(partIdx);
        int16_t* dstV = dstPicYuv->getCrAddr(partIdx);

        uint32_t srcstride = getCStride();
        uint32_t dststride = dstPicYuv->m_cwidth;

        primitives.chroma[m_csp].copy_ps[part](dstU, dststride, srcU, srcstride);
        primitives.chroma[m_csp].copy_ps[part](dstV, dststride, srcV, srcstride);
    }
}

void TComYuv::addClip(TComYuv* srcYuv0, TShortYUV* srcYuv1, uint32_t trUnitIdx, uint32_t partSize)
{
    int part = partitionFromSizes(partSize, partSize);

    addClipLuma(srcYuv0, srcYuv1, trUnitIdx, partSize, part);
    addClipChroma(srcYuv0, srcYuv1, trUnitIdx, partSize >> m_hChromaShift, part);
}

void TComYuv::addClipLuma(TComYuv* srcYuv0, TShortYUV* srcYuv1, uint32_t trUnitIdx, uint32_t partSize, uint32_t part)
{
    pixel* src0 = srcYuv0->getLumaAddr(trUnitIdx, partSize);
    int16_t* src1 = srcYuv1->getLumaAddr(trUnitIdx, partSize);
    pixel* dst = getLumaAddr(trUnitIdx, partSize);

    uint32_t src0Stride = srcYuv0->getStride();
    uint32_t src1Stride = srcYuv1->m_width;
    uint32_t dststride  = getStride();

    primitives.luma_add_ps[part](dst, dststride, src0, src1, src0Stride, src1Stride);
}

void TComYuv::addClipChroma(TComYuv* srcYuv0, TShortYUV* srcYuv1, uint32_t trUnitIdx, uint32_t partSize, uint32_t part)
{
    pixel* srcU0 = srcYuv0->getCbAddr(trUnitIdx, partSize);
    int16_t* srcU1 = srcYuv1->getCbAddr(trUnitIdx, partSize);
    pixel* srcV0 = srcYuv0->getCrAddr(trUnitIdx, partSize);
    int16_t* srcV1 = srcYuv1->getCrAddr(trUnitIdx, partSize);
    pixel* dstU = getCbAddr(trUnitIdx, partSize);
    pixel* dstV = getCrAddr(trUnitIdx, partSize);

    uint32_t src0Stride = srcYuv0->getCStride();
    uint32_t src1Stride = srcYuv1->m_cwidth;
    uint32_t dststride  = getCStride();

    primitives.chroma[m_csp].add_ps[part](dstU, dststride, srcU0, srcU1, src0Stride, src1Stride);
    primitives.chroma[m_csp].add_ps[part](dstV, dststride, srcV0, srcV1, src0Stride, src1Stride);
}

void TComYuv::subtract(TComYuv* srcYuv0, TComYuv* srcYuv1, uint32_t trUnitIdx, uint32_t partSize)
{
    subtractLuma(srcYuv0, srcYuv1,  trUnitIdx, partSize);
    subtractChroma(srcYuv0, srcYuv1,  trUnitIdx, partSize >> 1);
}

void TComYuv::subtractLuma(TComYuv* srcYuv0, TComYuv* srcYuv1, uint32_t trUnitIdx, uint32_t partSize)
{
    int x, y;

    pixel* src0 = srcYuv0->getLumaAddr(trUnitIdx, partSize);
    pixel* src1 = srcYuv1->getLumaAddr(trUnitIdx, partSize);
    pixel* dst  = getLumaAddr(trUnitIdx, partSize);

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

    pixel* srcU0 = srcYuv0->getCbAddr(trUnitIdx, partSize);
    pixel* srcU1 = srcYuv1->getCbAddr(trUnitIdx, partSize);
    pixel* srcV0 = srcYuv0->getCrAddr(trUnitIdx, partSize);
    pixel* srcV1 = srcYuv1->getCrAddr(trUnitIdx, partSize);
    pixel* dstU  = getCbAddr(trUnitIdx, partSize);
    pixel* dstV  = getCrAddr(trUnitIdx, partSize);

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

    pixel* srcY0 = srcYuv0->getLumaAddr(partUnitIdx);
    pixel* srcU0 = srcYuv0->getCbAddr(partUnitIdx);
    pixel* srcV0 = srcYuv0->getCrAddr(partUnitIdx);

    pixel* srcY1 = srcYuv1->getLumaAddr(partUnitIdx);
    pixel* srcU1 = srcYuv1->getCbAddr(partUnitIdx);
    pixel* srcV1 = srcYuv1->getCrAddr(partUnitIdx);

    pixel* dstY  = getLumaAddr(partUnitIdx);
    pixel* dstU  = getCbAddr(partUnitIdx);
    pixel* dstV  = getCrAddr(partUnitIdx);

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
    uint32_t src0Stride, src1Stride, dststride;

    int16_t* srcY0 = srcYuv0->getLumaAddr(partUnitIdx);
    int16_t* srcU0 = srcYuv0->getCbAddr(partUnitIdx);
    int16_t* srcV0 = srcYuv0->getCrAddr(partUnitIdx);

    int16_t* srcY1 = srcYuv1->getLumaAddr(partUnitIdx);
    int16_t* srcU1 = srcYuv1->getCbAddr(partUnitIdx);
    int16_t* srcV1 = srcYuv1->getCrAddr(partUnitIdx);

    pixel* dstY = getLumaAddr(partUnitIdx);
    pixel* dstU = getCbAddr(partUnitIdx);
    pixel* dstV = getCrAddr(partUnitIdx);

    int part = partitionFromSizes(width, height);

    if (bLuma)
    {
        src0Stride = srcYuv0->m_width;
        src1Stride = srcYuv1->m_width;
        dststride  = getStride();

        primitives.luma_addAvg[part](srcY0, srcY1, dstY, src0Stride, src1Stride, dststride);
    }
    if (bChroma)
    {
        src0Stride = srcYuv0->m_cwidth;
        src1Stride = srcYuv1->m_cwidth;
        dststride  = getCStride();

        primitives.chroma[m_csp].addAvg[part](srcU0, srcU1, dstU, src0Stride, src1Stride, dststride);
        primitives.chroma[m_csp].addAvg[part](srcV0, srcV1, dstV, src0Stride, src1Stride, dststride);
    }
}

//! \}
