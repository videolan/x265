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

using namespace x265;

#if _MSC_VER
#pragma warning (default: 4244)
#endif

ShortYuv::ShortYuv()
{
    m_buf[0] = NULL;
    m_buf[1] = NULL;
    m_buf[2] = NULL;
}

ShortYuv::~ShortYuv()
{
    destroy();
}

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

    uint32_t sizeL = width * height;
    uint32_t sizeC = m_cwidth * m_cheight;
    X265_CHECK((sizeC & 15) == 0, "invalid size");
    CHECKED_MALLOC(m_buf[0], int16_t, sizeL + sizeC * 2);
    m_buf[1] = m_buf[0] + sizeL;
    m_buf[2] = m_buf[0] + sizeL + sizeC;
    return true;

fail:
    return false;
}

void ShortYuv::destroy()
{
    X265_FREE(m_buf[0]);
    m_buf[0] = NULL;
    m_buf[1] = NULL;
    m_buf[2] = NULL;
}

void ShortYuv::clear()
{
    ::memset(m_buf[0], 0, (m_width  * m_height) * sizeof(int16_t));
    ::memset(m_buf[1], 0, (m_cwidth * m_cheight) * sizeof(int16_t));
    ::memset(m_buf[2], 0, (m_cwidth * m_cheight) * sizeof(int16_t));
}

void ShortYuv::subtract(TComYuv* srcYuv0, TComYuv* srcYuv1, uint32_t partSize)
{
    int part = partitionFromSize(partSize);

    pixel* srcY0 = srcYuv0->getLumaAddr();
    pixel* srcY1 = srcYuv1->getLumaAddr();

    primitives.luma_sub_ps[part](getLumaAddr(), m_width, srcY0, srcY1, srcYuv0->getStride(), srcYuv1->getStride());

    pixel* srcU0 = srcYuv0->getCbAddr();
    pixel* srcU1 = srcYuv1->getCbAddr();
    primitives.chroma[m_csp].sub_ps[part](getCbAddr(), m_cwidth, srcU0, srcU1, srcYuv0->getCStride(), srcYuv1->getCStride());

    pixel* srcV0 = srcYuv0->getCrAddr();
    pixel* srcV1 = srcYuv1->getCrAddr();
    primitives.chroma[m_csp].sub_ps[part](getCrAddr(), m_cwidth, srcV0, srcV1, srcYuv0->getCStride(), srcYuv1->getCStride());
}

void ShortYuv::addClip(ShortYuv* srcYuv0, ShortYuv* srcYuv1, uint32_t partSize)
{
    int16_t* srcY0 = srcYuv0->getLumaAddr();
    int16_t* srcY1 = srcYuv1->getLumaAddr();

    primitives.pixeladd_ss(partSize, partSize, getLumaAddr(), m_width, srcY0, srcY1, srcYuv0->m_width, srcYuv1->m_width);

    uint32_t cpartSize = partSize >> m_hChromaShift;
    int16_t* srcU0 = srcYuv0->getCbAddr();
    int16_t* srcU1 = srcYuv1->getCbAddr();
    int16_t* srcV0 = srcYuv0->getCrAddr();
    int16_t* srcV1 = srcYuv1->getCrAddr();
    primitives.pixeladd_ss(cpartSize, cpartSize, getCbAddr(), m_cwidth, srcU0, srcU1, srcYuv0->m_cwidth, srcYuv1->m_cwidth);
    primitives.pixeladd_ss(cpartSize, cpartSize, getCrAddr(), m_cwidth, srcV0, srcV1, srcYuv0->m_cwidth, srcYuv1->m_cwidth);
}

void ShortYuv::copyPartToPartLuma(ShortYuv* dstPicYuv, uint32_t partIdx, uint32_t partSize)
{
    int part = partitionFromSize(partSize);
    int16_t* src = getLumaAddr(partIdx);
    int16_t* dst = dstPicYuv->getLumaAddr(partIdx);

    primitives.luma_copy_ss[part](dst, dstPicYuv->m_width, src, m_width);
}

void ShortYuv::copyPartToPartLuma(TComYuv* dstPicYuv, uint32_t partIdx, uint32_t partSize)
{
    int part = partitionFromSize(partSize);
    int16_t* src = getLumaAddr(partIdx);
    pixel* dst = dstPicYuv->getLumaAddr(partIdx);

    primitives.luma_copy_sp[part](dst, dstPicYuv->getStride(), src, m_width);
}

void ShortYuv::copyPartToPartLuma(ShortYuv* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height)
{
    int part = partitionFromSizes(width, height);
    int16_t* src = getLumaAddr(partIdx);
    int16_t* dst = dstPicYuv->getLumaAddr(partIdx);

    primitives.luma_copy_ss[part](dst, dstPicYuv->m_width, src, m_width);
}

void ShortYuv::copyPartToPartLuma(TComYuv* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height)
{
    int part = partitionFromSizes(width, height);
    int16_t* src = getLumaAddr(partIdx);
    pixel* dst = dstPicYuv->getLumaAddr(partIdx);

    primitives.luma_copy_sp[part](dst, dstPicYuv->getStride(), src, m_width);
}

void ShortYuv::copyPartToPartChroma(ShortYuv* dstPicYuv, uint32_t partIdx, uint32_t lumaSize, bool bChromaSame)
{
    int part = partitionFromSize(lumaSize);

    part = ((part == 0) && (m_csp == CHROMA_422)) ? 1 : part;
    int16_t* srcU = getCbAddr(partIdx);
    int16_t* srcV = getCrAddr(partIdx);
    int16_t* dstU = dstPicYuv->getCbAddr(partIdx);
    int16_t* dstV = dstPicYuv->getCrAddr(partIdx);

    if (bChromaSame)
    {
        primitives.luma_copy_ss[part](dstU, dstPicYuv->m_cwidth, srcU, m_cwidth);
        primitives.luma_copy_ss[part](dstV, dstPicYuv->m_cwidth, srcV, m_cwidth);
    }
    else
    {
        primitives.chroma[m_csp].copy_ss[part](dstU, dstPicYuv->m_cwidth, srcU, m_cwidth);
        primitives.chroma[m_csp].copy_ss[part](dstV, dstPicYuv->m_cwidth, srcV, m_cwidth);
    }
}

void ShortYuv::copyPartToPartChroma(TComYuv* dstPicYuv, uint32_t partIdx, uint32_t lumaSize, bool bChromaSame)
{
    int part = partitionFromSize(lumaSize);
    int16_t* srcU = getCbAddr(partIdx);
    int16_t* srcV = getCrAddr(partIdx);
    pixel* dstU = dstPicYuv->getCbAddr(partIdx);
    pixel* dstV = dstPicYuv->getCrAddr(partIdx);

    uint32_t srcStride = m_cwidth;
    uint32_t dstStride = dstPicYuv->getCStride();

    if (bChromaSame)
    {
        primitives.luma_copy_sp[part](dstU, dstStride, srcU, srcStride);
        primitives.luma_copy_sp[part](dstV, dstStride, srcV, srcStride);
    }
    else
    {
        primitives.chroma[m_csp].copy_sp[part](dstU, dstStride, srcU, srcStride);
        primitives.chroma[m_csp].copy_sp[part](dstV, dstStride, srcV, srcStride);
    }
}

void ShortYuv::copyPartToPartShortChroma(ShortYuv* dstPicYuv, uint32_t partIdx, uint32_t lumaSize, uint32_t chromaId)
{
    X265_CHECK(chromaId == 1 || chromaId == 2, "invalid chroma id");

    int part = partitionFromSize(lumaSize);

    int16_t* src = getChromaAddr(chromaId, partIdx);
    int16_t* dst = dstPicYuv->getChromaAddr(chromaId, partIdx);
    uint32_t srcStride = m_cwidth;
    uint32_t dstStride = dstPicYuv->m_cwidth;
    primitives.chroma[m_csp].copy_ss[part](dst, dstStride, src, srcStride);
}

void ShortYuv::copyPartToPartYuvChroma(TComYuv* dstPicYuv, uint32_t partIdx, uint32_t lumaSize, uint32_t chromaId, const bool splitIntoSubTUs)
{
    X265_CHECK(chromaId == 1 || chromaId == 2, "invalid chroma id");

    int part = splitIntoSubTUs ? NUM_CHROMA_PARTITIONS422 : partitionFromSize(lumaSize);

    int16_t* src = getChromaAddr(chromaId, partIdx);
    pixel* dst = dstPicYuv->getChromaAddr(chromaId, partIdx);
    uint32_t srcStride = m_cwidth;
    uint32_t dstStride = dstPicYuv->getCStride();
    primitives.chroma[m_csp].copy_sp[part](dst, dstStride, src, srcStride);
}
