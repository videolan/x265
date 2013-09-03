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
#include "TShortYUV.h"
#include "primitives.h"

#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <math.h>

using namespace x265;

TShortYUV::TShortYUV()
{
    m_bufY = NULL;
    m_bufCb = NULL;
    m_bufCr = NULL;
}

TShortYUV::~TShortYUV()
{}

void TShortYUV::create(unsigned int width, unsigned int height)
{
    m_bufY  = (short*)X265_MALLOC(short, width * height);
    m_bufCb = (short*)X265_MALLOC(short, width * height >> 2);
    m_bufCr = (short*)X265_MALLOC(short, width * height >> 2);

    // set width and height
    m_width   = width;
    m_height  = height;
    m_cwidth  = width  >> 1;
    m_cheight = height >> 1;
}

void TShortYUV::destroy()
{
    X265_FREE(m_bufY);
    m_bufY = NULL;
    X265_FREE(m_bufCb);
    m_bufCb = NULL;
    X265_FREE(m_bufCr);
    m_bufCr = NULL;
}

void TShortYUV::clear()
{
    ::memset(m_bufY,  0, (m_width  * m_height) * sizeof(short));
    ::memset(m_bufCb, 0, (m_cwidth * m_cheight) * sizeof(short));
    ::memset(m_bufCr, 0, (m_cwidth * m_cheight) * sizeof(short));
}

void TShortYUV::subtract(TComYuv* srcYuv0, TComYuv* srcYuv1, unsigned int trUnitIdx, unsigned int partSize)
{
    subtractLuma(srcYuv0, srcYuv1,  trUnitIdx, partSize);
    subtractChroma(srcYuv0, srcYuv1,  trUnitIdx, partSize >> 1);
}

void TShortYUV::subtractLuma(TComYuv* srcYuv0, TComYuv* srcYuv1, unsigned int trUnitIdx, unsigned int partSize)
{
    int x = partSize, y = partSize;

    Pel* src0 = srcYuv0->getLumaAddr(trUnitIdx, partSize);
    Pel* src1 = srcYuv1->getLumaAddr(trUnitIdx, partSize);
    Short* dst = getLumaAddr(trUnitIdx, partSize);

    int src0Stride = srcYuv0->getStride();
    int src1Stride = srcYuv1->getStride();
    int dstStride  = m_width;

    primitives.pixelsub_sp(x, y, dst, dstStride, src0, src1, src0Stride, src1Stride);
}

void TShortYUV::subtractChroma(TComYuv* srcYuv0, TComYuv* srcYuv1, unsigned int trUnitIdx, unsigned int partSize)
{
    int x = partSize, y = partSize;

    Pel* srcU0 = srcYuv0->getCbAddr(trUnitIdx, partSize);
    Pel* srcU1 = srcYuv1->getCbAddr(trUnitIdx, partSize);
    Pel* srcV0 = srcYuv0->getCrAddr(trUnitIdx, partSize);
    Pel* srcV1 = srcYuv1->getCrAddr(trUnitIdx, partSize);
    Short* dstU = getCbAddr(trUnitIdx, partSize);
    Short* dstV = getCrAddr(trUnitIdx, partSize);

    int src0Stride = srcYuv0->getCStride();
    int src1Stride = srcYuv1->getCStride();
    int dstStride  = m_cwidth;

    primitives.pixelsub_sp(x, y, dstU, dstStride, srcU0, srcU1, src0Stride, src1Stride);
    primitives.pixelsub_sp(x, y, dstV, dstStride, srcV0, srcV1, src0Stride, src1Stride);
}

void TShortYUV::addClip(TShortYUV* srcYuv0, TShortYUV* srcYuv1, unsigned int trUnitIdx, unsigned int partSize)
{
    addClipLuma(srcYuv0, srcYuv1, trUnitIdx, partSize);
    addClipChroma(srcYuv0, srcYuv1, trUnitIdx, partSize >> 1);
}

#if _MSC_VER
#pragma warning (disable: 4244)
#endif

void TShortYUV::addClipLuma(TShortYUV* srcYuv0, TShortYUV* srcYuv1, unsigned int trUnitIdx, unsigned int partSize)
{
    short* src0 = srcYuv0->getLumaAddr(trUnitIdx, partSize);
    short* src1 = srcYuv1->getLumaAddr(trUnitIdx, partSize);
    short* dst  = getLumaAddr(trUnitIdx, partSize);

    unsigned int src0Stride = srcYuv0->m_width;
    unsigned int src1Stride = srcYuv1->m_width;
    unsigned int dstStride  = m_width;

    primitives.pixeladd_ss(partSize, partSize, dst, dstStride, src0, src1, src0Stride, src1Stride);
}

void TShortYUV::addClipChroma(TShortYUV* srcYuv0, TShortYUV* srcYuv1, unsigned int trUnitIdx, unsigned int partSize)
{

    short* srcU0 = srcYuv0->getCbAddr(trUnitIdx, partSize);
    short* srcU1 = srcYuv1->getCbAddr(trUnitIdx, partSize);
    short* srcV0 = srcYuv0->getCrAddr(trUnitIdx, partSize);
    short* srcV1 = srcYuv1->getCrAddr(trUnitIdx, partSize);
    short* dstU = getCbAddr(trUnitIdx, partSize);
    short* dstV = getCrAddr(trUnitIdx, partSize);

    unsigned int src0Stride = srcYuv0->m_cwidth;
    unsigned int src1Stride = srcYuv1->m_cwidth;
    unsigned int dstStride  = m_cwidth;

    primitives.pixeladd_ss(partSize, partSize, dstU, dstStride, srcU0, srcU1, src0Stride, src1Stride);
    primitives.pixeladd_ss(partSize, partSize, dstV, dstStride, srcV0, srcV1, src0Stride, src1Stride);
}

#if _MSC_VER
#pragma warning (default: 4244)
#endif

void TShortYUV::copyPartToPartYuv(TShortYUV* dstPicYuv, unsigned int partIdx, unsigned int width, unsigned int height)
{
    copyPartToPartLuma(dstPicYuv, partIdx, width, height);
    copyPartToPartChroma(dstPicYuv, partIdx, width >> 1, height >> 1);
}

void TShortYUV::copyPartToPartYuv(TComYuv* dstPicYuv, unsigned int partIdx, unsigned int width, unsigned int height)
{
    copyPartToPartLuma(dstPicYuv, partIdx, width, height);
    copyPartToPartChroma(dstPicYuv, partIdx, width >> 1, height >> 1);
}

Void TShortYUV::copyPartToPartLuma(TShortYUV* dstPicYuv, unsigned int partIdx, unsigned int width, unsigned int height)
{
    short* src = getLumaAddr(partIdx);
    short* dst = dstPicYuv->getLumaAddr(partIdx);
    if (src == dst) return;

    unsigned int srcStride = m_width;
    unsigned int dstStride = dstPicYuv->m_width;
    for (unsigned int y = height; y != 0; y--)
    {
        ::memcpy(dst, src, width * sizeof(short));
        src += srcStride;
        dst += dstStride;
    }
}

Void TShortYUV::copyPartToPartLuma(TComYuv* dstPicYuv, unsigned int partIdx, unsigned int width, unsigned int height)
{
    short* src = getLumaAddr(partIdx);
    Pel* dst = dstPicYuv->getLumaAddr(partIdx);

    unsigned int srcStride = m_width;
    unsigned int dstStride = dstPicYuv->getStride();

    primitives.blockcpy_ps(width, height, dst, dstStride, src, srcStride);
}

Void TShortYUV::copyPartToPartChroma(TShortYUV* dstPicYuv, unsigned int partIdx, unsigned int width, unsigned int height)
{
    short* srcU = getCbAddr(partIdx);
    short* srcV = getCrAddr(partIdx);
    short* dstU = dstPicYuv->getCbAddr(partIdx);
    short* dstV = dstPicYuv->getCrAddr(partIdx);
    if (srcU == dstU && srcV == dstV) return;

    unsigned int srcStride = m_cwidth;
    unsigned int dstStride = dstPicYuv->m_cwidth;
    for (unsigned int y = height; y != 0; y--)
    {
        ::memcpy(dstU, srcU, width * sizeof(short));
        ::memcpy(dstV, srcV, width * sizeof(short));
        srcU += srcStride;
        srcV += srcStride;
        dstU += dstStride;
        dstV += dstStride;
    }
}

Void TShortYUV::copyPartToPartChroma(TComYuv* dstPicYuv, unsigned int partIdx, unsigned int width, unsigned int height)
{
    short* srcU = getCbAddr(partIdx);
    short* srcV = getCrAddr(partIdx);
    Pel* dstU = dstPicYuv->getCbAddr(partIdx);
    Pel* dstV = dstPicYuv->getCrAddr(partIdx);

    unsigned int srcStride = m_cwidth;
    unsigned int dstStride = dstPicYuv->getCStride();

    primitives.blockcpy_ps(width, height, dstU, dstStride, srcU, srcStride);
    primitives.blockcpy_ps(width, height, dstV, dstStride, srcV, srcStride);
}

Void TShortYUV::copyPartToPartChroma(TShortYUV* dstPicYuv, unsigned int partIdx, unsigned int width, unsigned int height, unsigned int chromaId)
{
    if (chromaId == 0)
    {
        short* srcU = getCbAddr(partIdx);
        short* dstU = dstPicYuv->getCbAddr(partIdx);
        if (srcU == dstU)
        {
            return;
        }
        unsigned int srcStride = m_cwidth;
        unsigned int dstStride = dstPicYuv->m_cwidth;
        for (unsigned int y = height; y != 0; y--)
        {
            ::memcpy(dstU, srcU, width * sizeof(short));
            srcU += srcStride;
            dstU += dstStride;
        }
    }
    else if (chromaId == 1)
    {
        short* srcV = getCrAddr(partIdx);
        short* dstV = dstPicYuv->getCrAddr(partIdx);
        if (srcV == dstV) return;
        unsigned int srcStride = m_cwidth;
        unsigned int dstStride = dstPicYuv->m_cwidth;
        for (unsigned int y = height; y != 0; y--)
        {
            ::memcpy(dstV, srcV, width * sizeof(short));
            srcV += srcStride;
            dstV += dstStride;
        }
    }
    else
    {
        short* srcU = getCbAddr(partIdx);
        short* srcV = getCrAddr(partIdx);
        short* dstU = dstPicYuv->getCbAddr(partIdx);
        short* dstV = dstPicYuv->getCrAddr(partIdx);
        if (srcU == dstU && srcV == dstV) return;
        unsigned int srcStride = m_cwidth;
        unsigned int dstStride = dstPicYuv->m_cwidth;
        for (unsigned int y = height; y != 0; y--)
        {
            ::memcpy(dstU, srcU, width * sizeof(short));
            ::memcpy(dstV, srcV, width * sizeof(short));
            srcU += srcStride;
            srcV += srcStride;
            dstU += dstStride;
            dstV += dstStride;
        }
    }
}

Void TShortYUV::copyPartToPartChroma(TComYuv* dstPicYuv, unsigned int partIdx, unsigned int width, unsigned int height, unsigned int chromaId)
{
    if (chromaId == 0)
    {
        short* srcU = getCbAddr(partIdx);
        Pel* dstU = dstPicYuv->getCbAddr(partIdx);
        unsigned int srcStride = m_cwidth;
        unsigned int dstStride = dstPicYuv->getCStride();
        primitives.blockcpy_ps(width, height, dstU, dstStride, srcU, srcStride);
    }
    else if (chromaId == 1)
    {
        short* srcV = getCrAddr(partIdx);
        Pel* dstV = dstPicYuv->getCrAddr(partIdx);
        unsigned int srcStride = m_cwidth;
        unsigned int dstStride = dstPicYuv->getCStride();
        primitives.blockcpy_ps(width, height, dstV, dstStride, srcV, srcStride);
    }
    else
    {
        short* srcU = getCbAddr(partIdx);
        short* srcV = getCrAddr(partIdx);
        Pel* dstU = dstPicYuv->getCbAddr(partIdx);
        Pel* dstV = dstPicYuv->getCrAddr(partIdx);

        unsigned int srcStride = m_cwidth;
        unsigned int dstStride = dstPicYuv->getCStride();
        primitives.blockcpy_ps(width, height, dstU, dstStride, srcU, srcStride);
        primitives.blockcpy_ps(width, height, dstV, dstStride, srcV, srcStride);
    }
}
