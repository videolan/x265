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

#include "common.h"
#include "yuv.h"
#include "shortyuv.h"
#include "primitives.h"

#include "x265.h"

using namespace x265;

ShortYuv::ShortYuv()
{
    m_buf[0] = NULL;
    m_buf[1] = NULL;
    m_buf[2] = NULL;
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

    size_t sizeL = m_width * m_height;
    size_t sizeC = m_cwidth * m_cheight;
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
}

void ShortYuv::clear()
{
    ::memset(m_buf[0], 0, (m_width  * m_height) *  sizeof(int16_t));
    ::memset(m_buf[1], 0, (m_cwidth * m_cheight) * sizeof(int16_t));
    ::memset(m_buf[2], 0, (m_cwidth * m_cheight) * sizeof(int16_t));
}

void ShortYuv::subtract(const Yuv& srcYuv0, const Yuv& srcYuv1, uint32_t log2Size)
{
    const int sizeIdx = log2Size - 2;
    primitives.luma_sub_ps[sizeIdx](m_buf[0], m_width, srcYuv0.m_buf[0], srcYuv1.m_buf[0], srcYuv0.m_size, srcYuv1.m_size);
    primitives.chroma[m_csp].sub_ps[sizeIdx](m_buf[1], m_cwidth, srcYuv0.m_buf[1], srcYuv1.m_buf[1], srcYuv0.m_csize, srcYuv1.m_csize);
    primitives.chroma[m_csp].sub_ps[sizeIdx](m_buf[2], m_cwidth, srcYuv0.m_buf[2], srcYuv1.m_buf[2], srcYuv0.m_csize, srcYuv1.m_csize);
}

void ShortYuv::copyPartToPartLuma(ShortYuv& dstYuv, uint32_t partIdx, uint32_t log2Size) const
{
    const int16_t* src = getLumaAddr(partIdx);
    int16_t* dst = dstYuv.getLumaAddr(partIdx);

    primitives.square_copy_ss[log2Size - 2](dst, dstYuv.m_width, const_cast<int16_t*>(src), m_width);
}

void ShortYuv::copyPartToPartLuma(Yuv& dstYuv, uint32_t partIdx, uint32_t log2Size) const
{
    const int16_t* src = getLumaAddr(partIdx);
    pixel* dst = dstYuv.getLumaAddr(partIdx);

    primitives.square_copy_sp[log2Size - 2](dst, dstYuv.m_size, const_cast<int16_t*>(src), m_width);
}

void ShortYuv::copyPartToPartChroma(ShortYuv& dstYuv, uint32_t partIdx, uint32_t log2SizeL) const
{
    int part = partitionFromLog2Size(log2SizeL);
    const int16_t* srcU = getCbAddr(partIdx);
    const int16_t* srcV = getCrAddr(partIdx);
    int16_t* dstU = dstYuv.getCbAddr(partIdx);
    int16_t* dstV = dstYuv.getCrAddr(partIdx);

    primitives.chroma[m_csp].copy_ss[part](dstU, dstYuv.m_cwidth, const_cast<int16_t*>(srcU), m_cwidth);
    primitives.chroma[m_csp].copy_ss[part](dstV, dstYuv.m_cwidth, const_cast<int16_t*>(srcV), m_cwidth);
}

void ShortYuv::copyPartToPartChroma(Yuv& dstYuv, uint32_t partIdx, uint32_t log2SizeL) const
{
    int part = partitionFromLog2Size(log2SizeL);
    const int16_t* srcU = getCbAddr(partIdx);
    const int16_t* srcV = getCrAddr(partIdx);
    pixel* dstU = dstYuv.getCbAddr(partIdx);
    pixel* dstV = dstYuv.getCrAddr(partIdx);

    primitives.chroma[m_csp].copy_sp[part](dstU, dstYuv.m_csize, const_cast<int16_t*>(srcU), m_cwidth);
    primitives.chroma[m_csp].copy_sp[part](dstV, dstYuv.m_csize, const_cast<int16_t*>(srcV), m_cwidth);
}
