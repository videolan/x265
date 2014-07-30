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

#include "common.h"
#include "TComYuv.h"
#include "predict.h"
#include "shortyuv.h"
#include "primitives.h"

using namespace x265;

//! \ingroup TLibCommon
//! \{

TComYuv::TComYuv()
{
    m_buf[0] = NULL;
    m_buf[1] = NULL;
    m_buf[2] = NULL;
}

TComYuv::~TComYuv()
{
    destroy();
}

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

    uint32_t sizeL = width * height;
    uint32_t sizeC = m_cwidth * m_cheight;
    X265_CHECK((sizeC & 15) == 0, "invalid size");
    // memory allocation (padded for SIMD reads)
    CHECKED_MALLOC(m_buf[0], pixel, sizeL + sizeC * 2 + 8);
    m_buf[1] = m_buf[0] + sizeL;
    m_buf[2] = m_buf[0] + sizeL + sizeC;
    return true;

fail:
    return false;
}

void TComYuv::destroy()
{
    // memory free
    X265_FREE(m_buf[0]);
    m_buf[0] = NULL;
    m_buf[1] = NULL;
    m_buf[2] = NULL;
}

void TComYuv::clear()
{
    ::memset(m_buf[0], 0, (m_width  * m_height) * sizeof(pixel));
    ::memset(m_buf[1], 0, (m_cwidth * m_cheight) * sizeof(pixel));
    ::memset(m_buf[2], 0, (m_cwidth * m_cheight) * sizeof(pixel));
}

void TComYuv::copyToPicYuv(TComPicYuv* destPicYuv, uint32_t cuAddr, uint32_t absZOrderIdx)
{
    pixel* dstY = destPicYuv->getLumaAddr(cuAddr, absZOrderIdx);

    primitives.luma_copy_pp[m_part](dstY, destPicYuv->getStride(), m_buf[0], getStride());

    pixel* dstU = destPicYuv->getCbAddr(cuAddr, absZOrderIdx);
    pixel* dstV = destPicYuv->getCrAddr(cuAddr, absZOrderIdx);
    primitives.chroma[m_csp].copy_pp[m_part](dstU, destPicYuv->getCStride(), m_buf[1], getCStride());
    primitives.chroma[m_csp].copy_pp[m_part](dstV, destPicYuv->getCStride(), m_buf[2], getCStride());
}

void TComYuv::copyFromPicYuv(TComPicYuv* srcPicYuv, uint32_t cuAddr, uint32_t absZOrderIdx)
{
    pixel* srcY = srcPicYuv->getLumaAddr(cuAddr, absZOrderIdx);

    primitives.luma_copy_pp[m_part](m_buf[0], getStride(), srcY, srcPicYuv->getStride());

    pixel* srcU = srcPicYuv->getCbAddr(cuAddr, absZOrderIdx);
    pixel* srcV = srcPicYuv->getCrAddr(cuAddr, absZOrderIdx);
    primitives.chroma[m_csp].copy_pp[m_part](m_buf[1], getCStride(), srcU, srcPicYuv->getCStride());
    primitives.chroma[m_csp].copy_pp[m_part](m_buf[2], getCStride(), srcV, srcPicYuv->getCStride());
}

void TComYuv::copyFromYuv(TComYuv* srcYuv)
{
    X265_CHECK(m_width <= srcYuv->m_width && m_height <= srcYuv->m_height, "invalid size\n");

    primitives.luma_copy_pp[m_part](m_buf[0], m_width, srcYuv->m_buf[0], srcYuv->m_width);
    primitives.chroma[m_csp].copy_pp[m_part](m_buf[1], m_cwidth, srcYuv->m_buf[1], srcYuv->m_cwidth);
    primitives.chroma[m_csp].copy_pp[m_part](m_buf[2], m_cwidth, srcYuv->m_buf[2], srcYuv->m_cwidth);
}

void TComYuv::copyToPartYuv(TComYuv* dstPicYuv, uint32_t partIdx)
{
    pixel* dstY = dstPicYuv->getLumaAddr(partIdx);

    primitives.luma_copy_pp[m_part](dstY, dstPicYuv->getStride(), m_buf[0], getStride());

    pixel* dstU = dstPicYuv->getCbAddr(partIdx);
    pixel* dstV = dstPicYuv->getCrAddr(partIdx);
    primitives.chroma[m_csp].copy_pp[m_part](dstU, dstPicYuv->getCStride(), m_buf[1], getCStride());
    primitives.chroma[m_csp].copy_pp[m_part](dstV, dstPicYuv->getCStride(), m_buf[2], getCStride());
}

void TComYuv::copyPartToYuv(TComYuv* dstPicYuv, uint32_t partIdx)
{
    pixel* srcY = getLumaAddr(partIdx);
    pixel* dstY = dstPicYuv->getLumaAddr();

    int part = dstPicYuv->m_part;

    primitives.luma_copy_pp[part](dstY, dstPicYuv->getStride(), srcY, getStride());

    pixel* srcU = getCbAddr(partIdx);
    pixel* srcV = getCrAddr(partIdx);
    pixel* dstU = dstPicYuv->getCbAddr();
    pixel* dstV = dstPicYuv->getCrAddr();
    primitives.chroma[m_csp].copy_pp[part](dstU, dstPicYuv->getCStride(), srcU, getCStride());
    primitives.chroma[m_csp].copy_pp[part](dstV, dstPicYuv->getCStride(), srcV, getCStride());
}

void TComYuv::addClip(TComYuv* srcYuv0, ShortYuv* srcYuv1, uint32_t log2Size)
{
    int part = partitionFromLog2Size(log2Size);

    addClipLuma(srcYuv0, srcYuv1, part);
    addClipChroma(srcYuv0, srcYuv1, part);
}

void TComYuv::addClipLuma(TComYuv* srcYuv0, ShortYuv* srcYuv1, uint32_t part)
{
    pixel* src0 = srcYuv0->getLumaAddr();
    int16_t* src1 = srcYuv1->getLumaAddr();
    pixel* dst = getLumaAddr();

    uint32_t src0Stride = srcYuv0->getStride();
    uint32_t src1Stride = srcYuv1->m_width;
    uint32_t dststride  = getStride();

    primitives.luma_add_ps[part](dst, dststride, src0, src1, src0Stride, src1Stride);
}

void TComYuv::addClipChroma(TComYuv* srcYuv0, ShortYuv* srcYuv1, uint32_t part)
{
    pixel* srcU0 = srcYuv0->getCbAddr();
    int16_t* srcU1 = srcYuv1->getCbAddr();
    pixel* srcV0 = srcYuv0->getCrAddr();
    int16_t* srcV1 = srcYuv1->getCrAddr();
    pixel* dstU = getCbAddr();
    pixel* dstV = getCrAddr();

    uint32_t src0Stride = srcYuv0->getCStride();
    uint32_t src1Stride = srcYuv1->m_cwidth;
    uint32_t dststride  = getCStride();

    primitives.chroma[m_csp].add_ps[part](dstU, dststride, srcU0, srcU1, src0Stride, src1Stride);
    primitives.chroma[m_csp].add_ps[part](dstV, dststride, srcV0, srcV1, src0Stride, src1Stride);
}

void TComYuv::addAvg(ShortYuv* srcYuv0, ShortYuv* srcYuv1, uint32_t partUnitIdx, uint32_t width, uint32_t height, bool bLuma, bool bChroma)
{
    int part = partitionFromSizes(width, height);

    if (bLuma)
    {
        int16_t* srcY0 = srcYuv0->getLumaAddr(partUnitIdx);
        int16_t* srcY1 = srcYuv1->getLumaAddr(partUnitIdx);
        pixel* dstY = getLumaAddr(partUnitIdx);
        uint32_t src0Stride = srcYuv0->m_width;
        uint32_t src1Stride = srcYuv1->m_width;
        uint32_t dststride  = getStride();

        primitives.luma_addAvg[part](srcY0, srcY1, dstY, src0Stride, src1Stride, dststride);
    }
    if (bChroma)
    {
        int16_t* srcU0 = srcYuv0->getCbAddr(partUnitIdx);
        int16_t* srcV0 = srcYuv0->getCrAddr(partUnitIdx);
        int16_t* srcU1 = srcYuv1->getCbAddr(partUnitIdx);
        int16_t* srcV1 = srcYuv1->getCrAddr(partUnitIdx);
        pixel* dstU = getCbAddr(partUnitIdx);
        pixel* dstV = getCrAddr(partUnitIdx);
        uint32_t src0Stride = srcYuv0->m_cwidth;
        uint32_t src1Stride = srcYuv1->m_cwidth;
        uint32_t dststride  = getCStride();

        primitives.chroma[m_csp].addAvg[part](srcU0, srcU1, dstU, src0Stride, src1Stride, dststride);
        primitives.chroma[m_csp].addAvg[part](srcV0, srcV1, dstV, src0Stride, src1Stride, dststride);
    }
}

//! \}
