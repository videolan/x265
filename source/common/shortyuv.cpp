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
 * For more information, contact us at license @ x265.com
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

void ShortYuv::subtract(TComYuv* srcYuv0, TComYuv* srcYuv1, uint32_t log2Size)
{
    const int sizeIdx = log2Size - 2;

    pixel* srcY0 = srcYuv0->getLumaAddr();
    pixel* srcY1 = srcYuv1->getLumaAddr();

    primitives.luma_sub_ps[sizeIdx](getLumaAddr(), m_width, srcY0, srcY1, srcYuv0->getStride(), srcYuv1->getStride());

    pixel* srcU0 = srcYuv0->getCbAddr();
    pixel* srcU1 = srcYuv1->getCbAddr();
    primitives.chroma[m_csp].sub_ps[sizeIdx](getCbAddr(), m_cwidth, srcU0, srcU1, srcYuv0->getCStride(), srcYuv1->getCStride());

    pixel* srcV0 = srcYuv0->getCrAddr();
    pixel* srcV1 = srcYuv1->getCrAddr();
    primitives.chroma[m_csp].sub_ps[sizeIdx](getCrAddr(), m_cwidth, srcV0, srcV1, srcYuv0->getCStride(), srcYuv1->getCStride());
}

void ShortYuv::copyPartToPartLuma(ShortYuv* dstPicYuv, uint32_t partIdx, uint32_t log2Size)
{
    int16_t* src = getLumaAddr(partIdx);
    int16_t* dst = dstPicYuv->getLumaAddr(partIdx);

    primitives.square_copy_ss[log2Size - 2](dst, dstPicYuv->m_width, src, m_width);
}

void ShortYuv::copyPartToPartLuma(TComYuv* dstPicYuv, uint32_t partIdx, uint32_t log2Size)
{
    int16_t* src = getLumaAddr(partIdx);
    pixel* dst = dstPicYuv->getLumaAddr(partIdx);

    primitives.square_copy_sp[log2Size - 2](dst, dstPicYuv->getStride(), src, m_width);
}

void ShortYuv::copyPartToPartChroma(ShortYuv* dstPicYuv, uint32_t partIdx, uint32_t log2SizeL)
{
    int part = partitionFromLog2Size(log2SizeL);
    int16_t* srcU = getCbAddr(partIdx);
    int16_t* srcV = getCrAddr(partIdx);
    int16_t* dstU = dstPicYuv->getCbAddr(partIdx);
    int16_t* dstV = dstPicYuv->getCrAddr(partIdx);

    primitives.chroma[m_csp].copy_ss[part](dstU, dstPicYuv->m_cwidth, srcU, m_cwidth);
    primitives.chroma[m_csp].copy_ss[part](dstV, dstPicYuv->m_cwidth, srcV, m_cwidth);
}

void ShortYuv::copyPartToPartChroma(TComYuv* dstPicYuv, uint32_t partIdx, uint32_t log2SizeL)
{
    int part = partitionFromLog2Size(log2SizeL);
    int16_t* srcU = getCbAddr(partIdx);
    int16_t* srcV = getCrAddr(partIdx);
    pixel* dstU = dstPicYuv->getCbAddr(partIdx);
    pixel* dstV = dstPicYuv->getCrAddr(partIdx);

    uint32_t srcStride = m_cwidth;
    uint32_t dstStride = dstPicYuv->getCStride();

    primitives.chroma[m_csp].copy_sp[part](dstU, dstStride, srcU, srcStride);
    primitives.chroma[m_csp].copy_sp[part](dstV, dstStride, srcV, srcStride);
}
