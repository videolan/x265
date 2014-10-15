/*****************************************************************************
 * Copyright (C) 2014 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
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
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/


#include "common.h"
#include "yuv.h"
#include "shortyuv.h"
#include "picyuv.h"
#include "primitives.h"

using namespace x265;

Yuv::Yuv()
{
    m_buf[0] = NULL;
    m_buf[1] = NULL;
    m_buf[2] = NULL;
}

bool Yuv::create(uint32_t width, uint32_t height, int csp)
{
    m_hChromaShift = CHROMA_H_SHIFT(csp);
    m_vChromaShift = CHROMA_V_SHIFT(csp);

    // set width and height
    m_width   = width;
    m_height  = height;

    m_cwidth  = width  >> m_hChromaShift;
    m_cheight = height >> m_vChromaShift;

    m_csp     = csp;
    m_part    = partitionFromSizes(m_width, m_height);

    size_t sizeL = m_width * m_height;
    size_t sizeC = m_cwidth * m_cheight;
    X265_CHECK((sizeC & 15) == 0, "invalid size");

    // memory allocation (padded for SIMD reads)
    CHECKED_MALLOC(m_buf[0], pixel, sizeL + sizeC * 2 + 8);
    m_buf[1] = m_buf[0] + sizeL;
    m_buf[2] = m_buf[0] + sizeL + sizeC;
    return true;

fail:
    return false;
}

void Yuv::destroy()
{
    X265_FREE(m_buf[0]);
}

void Yuv::copyToPicYuv(PicYuv& dstPic, uint32_t cuAddr, uint32_t absZOrderIdx) const
{
    pixel* dstY = dstPic.getLumaAddr(cuAddr, absZOrderIdx);

    primitives.luma_copy_pp[m_part](dstY, dstPic.m_stride, m_buf[0], m_width);

    pixel* dstU = dstPic.getCbAddr(cuAddr, absZOrderIdx);
    pixel* dstV = dstPic.getCrAddr(cuAddr, absZOrderIdx);
    primitives.chroma[m_csp].copy_pp[m_part](dstU, dstPic.m_strideC, m_buf[1], m_cwidth);
    primitives.chroma[m_csp].copy_pp[m_part](dstV, dstPic.m_strideC, m_buf[2], m_cwidth);
}

void Yuv::copyFromPicYuv(const PicYuv& srcPic, uint32_t cuAddr, uint32_t absZOrderIdx)
{
    /* We cheat with const_cast internally because the get methods are not capable of
     * returning const buffers and the primitives are not const aware, but we know
     * this function does not modify srcPicYuv */
    PicYuv& srcPicSafe = const_cast<PicYuv&>(srcPic);
    pixel* srcY = srcPicSafe.getLumaAddr(cuAddr, absZOrderIdx);

    primitives.luma_copy_pp[m_part](m_buf[0], m_width, srcY, srcPic.m_stride);

    pixel* srcU = srcPicSafe.getCbAddr(cuAddr, absZOrderIdx);
    pixel* srcV = srcPicSafe.getCrAddr(cuAddr, absZOrderIdx);
    primitives.chroma[m_csp].copy_pp[m_part](m_buf[1], m_cwidth, srcU, srcPicSafe.m_strideC);
    primitives.chroma[m_csp].copy_pp[m_part](m_buf[2], m_cwidth, srcV, srcPicSafe.m_strideC);
}

void Yuv::copyFromYuv(const Yuv& srcYuv)
{
    X265_CHECK(m_width <= srcYuv.m_width && m_height <= srcYuv.m_height, "invalid size\n");

    primitives.luma_copy_pp[m_part](m_buf[0], m_width, srcYuv.m_buf[0], srcYuv.m_width);
    primitives.chroma[m_csp].copy_pp[m_part](m_buf[1], m_cwidth, srcYuv.m_buf[1], srcYuv.m_cwidth);
    primitives.chroma[m_csp].copy_pp[m_part](m_buf[2], m_cwidth, srcYuv.m_buf[2], srcYuv.m_cwidth);
}

void Yuv::copyToPartYuv(Yuv& dstYuv, uint32_t partIdx) const
{
    pixel* dstY = dstYuv.getLumaAddr(partIdx);
    primitives.luma_copy_pp[m_part](dstY, dstYuv.m_width, m_buf[0], m_width);

    pixel* dstU = dstYuv.getCbAddr(partIdx);
    pixel* dstV = dstYuv.getCrAddr(partIdx);
    primitives.chroma[m_csp].copy_pp[m_part](dstU, dstYuv.m_cwidth, m_buf[1], m_cwidth);
    primitives.chroma[m_csp].copy_pp[m_part](dstV, dstYuv.m_cwidth, m_buf[2], m_cwidth);
}

void Yuv::copyPartToYuv(Yuv& dstYuv, uint32_t partIdx) const
{
    pixel* srcY = m_buf[0] + getAddrOffset(partIdx, m_width);
    pixel* dstY = dstYuv.m_buf[0];

    primitives.luma_copy_pp[dstYuv.m_part](dstY, dstYuv.m_width, srcY, m_width);

    pixel* srcU = m_buf[1] + getChromaAddrOffset(partIdx, m_cwidth);
    pixel* srcV = m_buf[2] + getChromaAddrOffset(partIdx, m_cwidth);
    pixel* dstU = dstYuv.m_buf[1];
    pixel* dstV = dstYuv.m_buf[2];
    primitives.chroma[m_csp].copy_pp[dstYuv.m_part](dstU, dstYuv.m_cwidth, srcU, m_cwidth);
    primitives.chroma[m_csp].copy_pp[dstYuv.m_part](dstV, dstYuv.m_cwidth, srcV, m_cwidth);
}

void Yuv::addClip(const Yuv& srcYuv0, const ShortYuv& srcYuv1, uint32_t log2Size)
{
    addClipLuma(srcYuv0, srcYuv1, log2Size);
    addClipChroma(srcYuv0, srcYuv1, log2Size);
}

void Yuv::addClipLuma(const Yuv& srcYuv0, const ShortYuv& srcYuv1, uint32_t log2Size)
{
    primitives.luma_add_ps[log2Size - 2](m_buf[0], m_width, srcYuv0.m_buf[0], srcYuv1.m_buf[0], srcYuv0.m_width, srcYuv1.m_width);
}

void Yuv::addClipChroma(const Yuv& srcYuv0, const ShortYuv& srcYuv1, uint32_t log2Size)
{
    primitives.chroma[m_csp].add_ps[log2Size - 2](m_buf[1], m_cwidth, srcYuv0.m_buf[1], srcYuv1.m_buf[1], srcYuv0.m_cwidth, srcYuv1.m_cwidth);
    primitives.chroma[m_csp].add_ps[log2Size - 2](m_buf[2], m_cwidth, srcYuv0.m_buf[2], srcYuv1.m_buf[2], srcYuv0.m_cwidth, srcYuv1.m_cwidth);
}

void Yuv::addAvg(const ShortYuv& srcYuv0, const ShortYuv& srcYuv1, uint32_t partUnitIdx, uint32_t width, uint32_t height, bool bLuma, bool bChroma)
{
    int part = partitionFromSizes(width, height);

    if (bLuma)
    {
        int16_t* srcY0 = const_cast<ShortYuv&>(srcYuv0).getLumaAddr(partUnitIdx);
        int16_t* srcY1 = const_cast<ShortYuv&>(srcYuv1).getLumaAddr(partUnitIdx);
        pixel* dstY = getLumaAddr(partUnitIdx);

        primitives.luma_addAvg[part](srcY0, srcY1, dstY, srcYuv0.m_width, srcYuv1.m_width, m_width);
    }
    if (bChroma)
    {
        int16_t* srcU0 = const_cast<ShortYuv&>(srcYuv0).getCbAddr(partUnitIdx);
        int16_t* srcV0 = const_cast<ShortYuv&>(srcYuv0).getCrAddr(partUnitIdx);
        int16_t* srcU1 = const_cast<ShortYuv&>(srcYuv1).getCbAddr(partUnitIdx);
        int16_t* srcV1 = const_cast<ShortYuv&>(srcYuv1).getCrAddr(partUnitIdx);
        pixel* dstU = getCbAddr(partUnitIdx);
        pixel* dstV = getCrAddr(partUnitIdx);

        primitives.chroma[m_csp].addAvg[part](srcU0, srcU1, dstU, srcYuv0.m_cwidth, srcYuv1.m_cwidth, m_cwidth);
        primitives.chroma[m_csp].addAvg[part](srcV0, srcV1, dstV, srcYuv0.m_cwidth, srcYuv1.m_cwidth, m_cwidth);
    }
}
