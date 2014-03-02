/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Deepthi Nandakumar <deepthi@multicorewareinc.com>
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
 * For more information, contact us at licensing@multicorewareinc.com
 *****************************************************************************/

#include "TLibCommon/TypeDef.h"
#include "TLibCommon/TComYuv.h"
#include "shortyuv.h"
#include "primitives.h"
#include "x265.h"

#include <cstdlib>
#include <memory.h>
#include <assert.h>
#include <math.h>

using namespace x265;

#if _MSC_VER
#pragma warning (default: 4244)
#endif

ShortYuv::ShortYuv()
{
    m_bufY = NULL;
    m_bufCb = NULL;
    m_bufCr = NULL;
}

ShortYuv::~ShortYuv()
{}

bool ShortYuv::create(uint32_t width, uint32_t height, int csp)
{
    // set width and height
    m_width  = width;
    m_height = height;

    m_csp = csp;
    m_hChromaShift = CHROMA_H_SHIFT(csp);
    m_vChromaShift = CHROMA_V_SHIFT(csp);

    m_cwidth  = width  >> m_hChromaShift;
    m_cheight = height >> m_vChromaShift;

    CHECKED_MALLOC(m_bufY, int16_t, width * height);
    CHECKED_MALLOC(m_bufCb, int16_t, m_cwidth * m_cheight);
    CHECKED_MALLOC(m_bufCr, int16_t, m_cwidth * m_cheight);
    return true;

fail:
    return false;
}

void ShortYuv::destroy()
{
    X265_FREE(m_bufY);
    m_bufY = NULL;
    X265_FREE(m_bufCb);
    m_bufCb = NULL;
    X265_FREE(m_bufCr);
    m_bufCr = NULL;
}

void ShortYuv::clear()
{
    ::memset(m_bufY,  0, (m_width  * m_height) * sizeof(int16_t));
    ::memset(m_bufCb, 0, (m_cwidth * m_cheight) * sizeof(int16_t));
    ::memset(m_bufCr, 0, (m_cwidth * m_cheight) * sizeof(int16_t));
}

void ShortYuv::subtract(TComYuv* srcYuv0, TComYuv* srcYuv1, uint32_t trUnitIdx, uint32_t partSize)
{
    int part = partitionFromSizes(partSize, partSize);

    subtractLuma(srcYuv0, srcYuv1, trUnitIdx, partSize, part);
    subtractChroma(srcYuv0, srcYuv1, trUnitIdx, partSize >> m_hChromaShift, part);
}

void ShortYuv::subtractLuma(TComYuv* srcYuv0, TComYuv* srcYuv1, uint32_t trUnitIdx, uint32_t partSize, uint32_t part)
{
    Pel* src0 = srcYuv0->getLumaAddr(trUnitIdx, partSize);
    Pel* src1 = srcYuv1->getLumaAddr(trUnitIdx, partSize);
    int16_t* dst = getLumaAddr(trUnitIdx, partSize);

    int src0Stride = srcYuv0->getStride();
    int src1Stride = srcYuv1->getStride();
    int dstStride  = m_width;

    primitives.luma_sub_ps[part](dst, dstStride, src0, src1, src0Stride, src1Stride);
}

void ShortYuv::subtractChroma(TComYuv* srcYuv0, TComYuv* srcYuv1, uint32_t trUnitIdx, uint32_t partSize, uint32_t part)
{
    Pel* srcU0 = srcYuv0->getCbAddr(trUnitIdx, partSize);
    Pel* srcU1 = srcYuv1->getCbAddr(trUnitIdx, partSize);
    Pel* srcV0 = srcYuv0->getCrAddr(trUnitIdx, partSize);
    Pel* srcV1 = srcYuv1->getCrAddr(trUnitIdx, partSize);
    int16_t* dstU = getCbAddr(trUnitIdx, partSize);
    int16_t* dstV = getCrAddr(trUnitIdx, partSize);

    int src0Stride = srcYuv0->getCStride();
    int src1Stride = srcYuv1->getCStride();
    int dstStride  = m_cwidth;

    primitives.chroma[m_csp].sub_ps[part](dstU, dstStride, srcU0, srcU1, src0Stride, src1Stride);
    primitives.chroma[m_csp].sub_ps[part](dstV, dstStride, srcV0, srcV1, src0Stride, src1Stride);
}

void ShortYuv::addClip(ShortYuv* srcYuv0, ShortYuv* srcYuv1, uint32_t trUnitIdx, uint32_t partSize)
{
    addClipLuma(srcYuv0, srcYuv1, trUnitIdx, partSize);
    addClipChroma(srcYuv0, srcYuv1, trUnitIdx, partSize >> m_hChromaShift);
}

void ShortYuv::addClipLuma(ShortYuv* srcYuv0, ShortYuv* srcYuv1, uint32_t trUnitIdx, uint32_t partSize)
{
    int16_t* src0 = srcYuv0->getLumaAddr(trUnitIdx, partSize);
    int16_t* src1 = srcYuv1->getLumaAddr(trUnitIdx, partSize);
    int16_t* dst  = getLumaAddr(trUnitIdx, partSize);

    uint32_t src0Stride = srcYuv0->m_width;
    uint32_t src1Stride = srcYuv1->m_width;
    uint32_t dstStride  = m_width;

    primitives.pixeladd_ss(partSize, partSize, dst, dstStride, src0, src1, src0Stride, src1Stride);
}

void ShortYuv::addClipChroma(ShortYuv* srcYuv0, ShortYuv* srcYuv1, uint32_t trUnitIdx, uint32_t partSize)
{
    int16_t* srcU0 = srcYuv0->getCbAddr(trUnitIdx, partSize);
    int16_t* srcU1 = srcYuv1->getCbAddr(trUnitIdx, partSize);
    int16_t* srcV0 = srcYuv0->getCrAddr(trUnitIdx, partSize);
    int16_t* srcV1 = srcYuv1->getCrAddr(trUnitIdx, partSize);
    int16_t* dstU = getCbAddr(trUnitIdx, partSize);
    int16_t* dstV = getCrAddr(trUnitIdx, partSize);

    uint32_t src0Stride = srcYuv0->m_cwidth;
    uint32_t src1Stride = srcYuv1->m_cwidth;
    uint32_t dstStride  = m_cwidth;

    primitives.pixeladd_ss(partSize, partSize, dstU, dstStride, srcU0, srcU1, src0Stride, src1Stride);
    primitives.pixeladd_ss(partSize, partSize, dstV, dstStride, srcV0, srcV1, src0Stride, src1Stride);
}

void ShortYuv::copyPartToPartYuv(ShortYuv* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height)
{
    copyPartToPartLuma(dstPicYuv, partIdx, width, height);
    copyPartToPartChroma(dstPicYuv, partIdx, width >> m_hChromaShift, height >> m_vChromaShift);
}

void ShortYuv::copyPartToPartYuv(TComYuv* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height)
{
    copyPartToPartLuma(dstPicYuv, partIdx, width, height);
    copyPartToPartChroma(dstPicYuv, partIdx, width >> m_hChromaShift, height >> m_vChromaShift);
}

void ShortYuv::copyPartToPartLuma(ShortYuv* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height)
{
    int16_t* src = getLumaAddr(partIdx);
    int16_t* dst = dstPicYuv->getLumaAddr(partIdx);

    if (src == dst) return;

    uint32_t srcStride = m_width;
    uint32_t dstStride = dstPicYuv->m_width;
    for (uint32_t y = height; y != 0; y--)
    {
        ::memcpy(dst, src, width * sizeof(int16_t));
        src += srcStride;
        dst += dstStride;
    }
}

void ShortYuv::copyPartToPartLuma(TComYuv* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height)
{
    int16_t* src = getLumaAddr(partIdx);
    Pel* dst = dstPicYuv->getLumaAddr(partIdx);

    uint32_t srcStride = m_width;
    uint32_t dstStride = dstPicYuv->getStride();

    primitives.blockcpy_ps(width, height, dst, dstStride, src, srcStride);
}

void ShortYuv::copyPartToPartChroma(ShortYuv* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height)
{
    int16_t* srcU = getCbAddr(partIdx);
    int16_t* srcV = getCrAddr(partIdx);
    int16_t* dstU = dstPicYuv->getCbAddr(partIdx);
    int16_t* dstV = dstPicYuv->getCrAddr(partIdx);

    if (srcU == dstU && srcV == dstV) return;

    uint32_t srcStride = m_cwidth;
    uint32_t dstStride = dstPicYuv->m_cwidth;
    for (uint32_t y = height; y != 0; y--)
    {
        ::memcpy(dstU, srcU, width * sizeof(int16_t));
        ::memcpy(dstV, srcV, width * sizeof(int16_t));
        srcU += srcStride;
        srcV += srcStride;
        dstU += dstStride;
        dstV += dstStride;
    }
}

void ShortYuv::copyPartToPartChroma(TComYuv* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height)
{
    int16_t* srcU = getCbAddr(partIdx);
    int16_t* srcV = getCrAddr(partIdx);
    Pel* dstU = dstPicYuv->getCbAddr(partIdx);
    Pel* dstV = dstPicYuv->getCrAddr(partIdx);

    uint32_t srcStride = m_cwidth;
    uint32_t dstStride = dstPicYuv->getCStride();

    primitives.blockcpy_ps(width, height, dstU, dstStride, srcU, srcStride);
    primitives.blockcpy_ps(width, height, dstV, dstStride, srcV, srcStride);
}

void ShortYuv::copyPartToPartChroma(ShortYuv* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height, uint32_t chromaId)
{
    if (chromaId == 0)
    {
        int16_t* srcU = getCbAddr(partIdx);
        int16_t* dstU = dstPicYuv->getCbAddr(partIdx);
        if (srcU == dstU)
        {
            return;
        }
        uint32_t srcStride = m_cwidth;
        uint32_t dstStride = dstPicYuv->m_cwidth;
        for (uint32_t y = height; y != 0; y--)
        {
            ::memcpy(dstU, srcU, width * sizeof(int16_t));
            srcU += srcStride;
            dstU += dstStride;
        }
    }
    else if (chromaId == 1)
    {
        int16_t* srcV = getCrAddr(partIdx);
        int16_t* dstV = dstPicYuv->getCrAddr(partIdx);
        if (srcV == dstV) return;
        uint32_t srcStride = m_cwidth;
        uint32_t dstStride = dstPicYuv->m_cwidth;
        for (uint32_t y = height; y != 0; y--)
        {
            ::memcpy(dstV, srcV, width * sizeof(int16_t));
            srcV += srcStride;
            dstV += dstStride;
        }
    }
    else
    {
        int16_t* srcU = getCbAddr(partIdx);
        int16_t* srcV = getCrAddr(partIdx);
        int16_t* dstU = dstPicYuv->getCbAddr(partIdx);
        int16_t* dstV = dstPicYuv->getCrAddr(partIdx);
        if (srcU == dstU && srcV == dstV) return;
        uint32_t srcStride = m_cwidth;
        uint32_t dstStride = dstPicYuv->m_cwidth;
        for (uint32_t y = height; y != 0; y--)
        {
            ::memcpy(dstU, srcU, width * sizeof(int16_t));
            ::memcpy(dstV, srcV, width * sizeof(int16_t));
            srcU += srcStride;
            srcV += srcStride;
            dstU += dstStride;
            dstV += dstStride;
        }
    }
}

void ShortYuv::copyPartToPartChroma(TComYuv* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height, uint32_t chromaId)
{
    if (chromaId == 0)
    {
        int16_t* srcU = getCbAddr(partIdx);
        Pel* dstU = dstPicYuv->getCbAddr(partIdx);
        uint32_t srcStride = m_cwidth;
        uint32_t dstStride = dstPicYuv->getCStride();
        primitives.blockcpy_ps(width, height, dstU, dstStride, srcU, srcStride);
    }
    else if (chromaId == 1)
    {
        int16_t* srcV = getCrAddr(partIdx);
        Pel* dstV = dstPicYuv->getCrAddr(partIdx);
        uint32_t srcStride = m_cwidth;
        uint32_t dstStride = dstPicYuv->getCStride();
        primitives.blockcpy_ps(width, height, dstV, dstStride, srcV, srcStride);
    }
    else
    {
        int16_t* srcU = getCbAddr(partIdx);
        int16_t* srcV = getCrAddr(partIdx);
        Pel* dstU = dstPicYuv->getCbAddr(partIdx);
        Pel* dstV = dstPicYuv->getCrAddr(partIdx);

        uint32_t srcStride = m_cwidth;
        uint32_t dstStride = dstPicYuv->getCStride();
        primitives.blockcpy_ps(width, height, dstU, dstStride, srcU, srcStride);
        primitives.blockcpy_ps(width, height, dstV, dstStride, srcV, srcStride);
    }
}
