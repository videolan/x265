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

    pixel* srcY0 = srcYuv0->getLumaAddr(trUnitIdx, partSize);
    pixel* srcY1 = srcYuv1->getLumaAddr(trUnitIdx, partSize);
    primitives.luma_sub_ps[part](getLumaAddr(trUnitIdx, partSize), m_width, srcY0, srcY1, srcYuv0->getStride(), srcYuv1->getStride());

    uint32_t cpartSize = partSize >> m_hChromaShift;
    pixel* srcU0 = srcYuv0->getCbAddr(trUnitIdx, cpartSize);
    pixel* srcU1 = srcYuv1->getCbAddr(trUnitIdx, cpartSize);
    primitives.chroma[m_csp].sub_ps[part](getCbAddr(trUnitIdx, cpartSize), m_cwidth, srcU0, srcU1, srcYuv0->getCStride(), srcYuv1->getCStride());

    pixel* srcV0 = srcYuv0->getCrAddr(trUnitIdx, cpartSize);
    pixel* srcV1 = srcYuv1->getCrAddr(trUnitIdx, cpartSize);
    primitives.chroma[m_csp].sub_ps[part](getCrAddr(trUnitIdx, cpartSize), m_cwidth, srcV0, srcV1, srcYuv0->getCStride(), srcYuv1->getCStride());
}

void ShortYuv::addClip(ShortYuv* srcYuv0, ShortYuv* srcYuv1, uint32_t trUnitIdx, uint32_t partSize)
{
    int16_t* srcY0 = srcYuv0->getLumaAddr(trUnitIdx, partSize);
    int16_t* srcY1 = srcYuv1->getLumaAddr(trUnitIdx, partSize);
    primitives.pixeladd_ss(partSize, partSize, getLumaAddr(trUnitIdx, partSize), m_width, srcY0, srcY1, srcYuv0->m_width, srcYuv1->m_width);

    uint32_t cpartSize = partSize >> m_hChromaShift;
    int16_t* srcU0 = srcYuv0->getCbAddr(trUnitIdx, cpartSize);
    int16_t* srcU1 = srcYuv1->getCbAddr(trUnitIdx, cpartSize);
    int16_t* srcV0 = srcYuv0->getCrAddr(trUnitIdx, cpartSize);
    int16_t* srcV1 = srcYuv1->getCrAddr(trUnitIdx, cpartSize);
    primitives.pixeladd_ss(cpartSize, cpartSize, getCbAddr(trUnitIdx, cpartSize), m_cwidth, srcU0, srcU1, srcYuv0->m_cwidth, srcYuv1->m_cwidth);
    primitives.pixeladd_ss(cpartSize, cpartSize, getCrAddr(trUnitIdx, cpartSize), m_cwidth, srcV0, srcV1, srcYuv0->m_cwidth, srcYuv1->m_cwidth);
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

    uint32_t srcStride = m_width;
    uint32_t dstStride = dstPicYuv->m_width;
#if HIGH_BIT_DEPTH
    primitives.blockcpy_pp(width, height, (pixel*)dst, dstStride, (pixel*)src, srcStride);
#else
    for (uint32_t y = height; y != 0; y--)
    {
        ::memcpy(dst, src, width * sizeof(int16_t));
        src += srcStride;
        dst += dstStride;
    }
#endif
}

void ShortYuv::copyPartToPartLuma(TComYuv* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height)
{
    int16_t* src = getLumaAddr(partIdx);
    pixel* dst = dstPicYuv->getLumaAddr(partIdx);

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

    uint32_t srcStride = m_cwidth;
    uint32_t dstStride = dstPicYuv->m_cwidth;
#if HIGH_BIT_DEPTH
    primitives.blockcpy_pp(width, height, (pixel*)dstU, dstStride, (pixel*)srcU, srcStride);
    primitives.blockcpy_pp(width, height, (pixel*)dstV, dstStride, (pixel*)srcV, srcStride);
#else
    for (uint32_t y = height; y != 0; y--)
    {
        ::memcpy(dstU, srcU, width * sizeof(int16_t));
        ::memcpy(dstV, srcV, width * sizeof(int16_t));
        srcU += srcStride;
        srcV += srcStride;
        dstU += dstStride;
        dstV += dstStride;
    }
#endif
}

void ShortYuv::copyPartToPartChroma(TComYuv* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height)
{
    int16_t* srcU = getCbAddr(partIdx);
    int16_t* srcV = getCrAddr(partIdx);
    pixel* dstU = dstPicYuv->getCbAddr(partIdx);
    pixel* dstV = dstPicYuv->getCrAddr(partIdx);

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
        pixel* dstU = dstPicYuv->getCbAddr(partIdx);
        uint32_t srcStride = m_cwidth;
        uint32_t dstStride = dstPicYuv->getCStride();
        primitives.blockcpy_ps(width, height, dstU, dstStride, srcU, srcStride);
    }
    else if (chromaId == 1)
    {
        int16_t* srcV = getCrAddr(partIdx);
        pixel* dstV = dstPicYuv->getCrAddr(partIdx);
        uint32_t srcStride = m_cwidth;
        uint32_t dstStride = dstPicYuv->getCStride();
        primitives.blockcpy_ps(width, height, dstV, dstStride, srcV, srcStride);
    }
    else
    {
        int16_t* srcU = getCbAddr(partIdx);
        int16_t* srcV = getCrAddr(partIdx);
        pixel* dstU = dstPicYuv->getCbAddr(partIdx);
        pixel* dstV = dstPicYuv->getCrAddr(partIdx);

        uint32_t srcStride = m_cwidth;
        uint32_t dstStride = dstPicYuv->getCStride();
        primitives.blockcpy_ps(width, height, dstU, dstStride, srcU, srcStride);
        primitives.blockcpy_ps(width, height, dstV, dstStride, srcV, srcStride);
    }
}
