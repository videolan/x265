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
    m_apiBufY = NULL;
    m_apiBufU = NULL;
    m_apiBufV = NULL;
}

TComYuv::~TComYuv()
{}

Void TComYuv::create(UInt width, UInt height)
{
    // memory allocation
    m_apiBufY  = (Pel*)xMalloc(Pel, width * height);
    m_apiBufU  = (Pel*)xMalloc(Pel, width * height >> 2);
    m_apiBufV  = (Pel*)xMalloc(Pel, width * height >> 2);

    // set width and height
    m_iWidth   = width;
    m_iHeight  = height;
    m_iCWidth  = width  >> 1;
    m_iCHeight = height >> 1;
}

Void TComYuv::destroy()
{
    // memory free
    xFree(m_apiBufY);
    m_apiBufY = NULL;
    xFree(m_apiBufU);
    m_apiBufU = NULL;
    xFree(m_apiBufV);
    m_apiBufV = NULL;
}

Void TComYuv::clear()
{
    ::memset(m_apiBufY, 0, (m_iWidth  * m_iHeight) * sizeof(Pel));
    ::memset(m_apiBufU, 0, (m_iCWidth * m_iCHeight) * sizeof(Pel));
    ::memset(m_apiBufV, 0, (m_iCWidth * m_iCHeight) * sizeof(Pel));
}

Void TComYuv::copyToPicYuv(TComPicYuv* pcPicYuvDst, UInt iCuAddr, UInt absZOrderIdx, UInt partDepth, UInt partIdx)
{
    copyToPicLuma(pcPicYuvDst, iCuAddr, absZOrderIdx, partDepth, partIdx);
    copyToPicChroma(pcPicYuvDst, iCuAddr, absZOrderIdx, partDepth, partIdx);
}

Void TComYuv::copyToPicLuma(TComPicYuv* pcPicYuvDst, UInt iCuAddr, UInt absZOrderIdx, UInt partDepth, UInt partIdx)
{
    Int width, height;

    width  = m_iWidth >> partDepth;
    height = m_iHeight >> partDepth;

    Pel* src = getLumaAddr(partIdx, width);
    Pel* dst = pcPicYuvDst->getLumaAddr(iCuAddr, absZOrderIdx);

    UInt  srcstride = getStride();
    UInt  dststride = pcPicYuvDst->getStride();

    x265::primitives.blockcpy_pp(width, height, (pixel*)dst, dststride, (pixel*)src, srcstride);
}

Void TComYuv::copyToPicChroma(TComPicYuv* pcPicYuvDst, UInt iCuAddr, UInt absZOrderIdx, UInt partDepth, UInt partIdx)
{
    Int width, height;

    width  = m_iCWidth >> partDepth;
    height = m_iCHeight >> partDepth;

    Pel* srcU = getCbAddr(partIdx, width);
    Pel* srcV = getCrAddr(partIdx, width);
    Pel* dstU = pcPicYuvDst->getCbAddr(iCuAddr, absZOrderIdx);
    Pel* dstV = pcPicYuvDst->getCrAddr(iCuAddr, absZOrderIdx);

    UInt srcstride = getCStride();
    UInt dststride = pcPicYuvDst->getCStride();

    x265::primitives.blockcpy_pp(width, height, (pixel*)dstU, dststride, (pixel*)srcU, srcstride);
    x265::primitives.blockcpy_pp(width, height, (pixel*)dstV, dststride, (pixel*)srcV, srcstride);
}

Void TComYuv::copyFromPicYuv(TComPicYuv* pcPicYuvSrc, UInt iCuAddr, UInt absZOrderIdx)
{
    copyFromPicLuma(pcPicYuvSrc, iCuAddr, absZOrderIdx);
    copyFromPicChroma(pcPicYuvSrc, iCuAddr, absZOrderIdx);
}

Void TComYuv::copyFromPicLuma(TComPicYuv* pcPicYuvSrc, UInt iCuAddr, UInt absZOrderIdx)
{
    Pel* dst = m_apiBufY;
    Pel* src = pcPicYuvSrc->getLumaAddr(iCuAddr, absZOrderIdx);

    UInt  dststride = getStride();
    UInt  srcstride = pcPicYuvSrc->getStride();

    x265::primitives.blockcpy_pp(m_iWidth, m_iHeight, (pixel*)dst, dststride, (pixel*)src, srcstride);
}

Void TComYuv::copyFromPicChroma(TComPicYuv* pcPicYuvSrc, UInt iCuAddr, UInt absZOrderIdx)
{
    Pel* dstU = m_apiBufU;
    Pel* dstV = m_apiBufV;
    Pel* srcU = pcPicYuvSrc->getCbAddr(iCuAddr, absZOrderIdx);
    Pel* srcV = pcPicYuvSrc->getCrAddr(iCuAddr, absZOrderIdx);

    UInt  dststride = getCStride();
    UInt  srcstride = pcPicYuvSrc->getCStride();

    x265::primitives.blockcpy_pp(m_iCWidth, m_iCHeight, (pixel*)dstU, dststride, (pixel*)srcU, srcstride);
    x265::primitives.blockcpy_pp(m_iCWidth, m_iCHeight, (pixel*)dstV, dststride, (pixel*)srcV, srcstride);
}

Void TComYuv::copyToPartYuv(TComYuv* pcYuvDst, UInt uiDstPartIdx)
{
    copyToPartLuma(pcYuvDst, uiDstPartIdx);
    copyToPartChroma(pcYuvDst, uiDstPartIdx);
}

Void TComYuv::copyToPartLuma(TComYuv* pcYuvDst, UInt uiDstPartIdx)
{
    Pel* src = m_apiBufY;
    Pel* dst = pcYuvDst->getLumaAddr(uiDstPartIdx);

    UInt  srcstride  = getStride();
    UInt  dststride  = pcYuvDst->getStride();

    x265::primitives.blockcpy_pp(m_iWidth, m_iHeight, (pixel*)dst, dststride, (pixel*)src, srcstride);
}

Void TComYuv::copyToPartChroma(TComYuv* pcYuvDst, UInt uiDstPartIdx)
{
    Pel* srcU = m_apiBufU;
    Pel* srcV = m_apiBufV;
    Pel* dstU = pcYuvDst->getCbAddr(uiDstPartIdx);
    Pel* dstV = pcYuvDst->getCrAddr(uiDstPartIdx);

    UInt  srcstride = getCStride();
    UInt  dststride = pcYuvDst->getCStride();

    x265::primitives.blockcpy_pp(m_iCWidth, m_iCHeight, (pixel*)dstU, dststride, (pixel*)srcU, srcstride);
    x265::primitives.blockcpy_pp(m_iCWidth, m_iCHeight, (pixel*)dstV, dststride, (pixel*)srcV, srcstride);
}

Void TComYuv::copyPartToYuv(TComYuv* pcYuvDst, UInt uiSrcPartIdx)
{
    copyPartToLuma(pcYuvDst, uiSrcPartIdx);
    copyPartToChroma(pcYuvDst, uiSrcPartIdx);
}

Void TComYuv::copyPartToLuma(TComYuv* pcYuvDst, UInt uiSrcPartIdx)
{
    Pel* src = getLumaAddr(uiSrcPartIdx);
    Pel* dst = pcYuvDst->getLumaAddr(0);

    UInt srcstride = getStride();
    UInt dststride = pcYuvDst->getStride();

    UInt height = pcYuvDst->getHeight();
    UInt width = pcYuvDst->getWidth();

    x265::primitives.blockcpy_pp(width, height, (pixel*)dst, dststride, (pixel*)src, srcstride);
}

Void TComYuv::copyPartToChroma(TComYuv* pcYuvDst, UInt uiSrcPartIdx)
{
    Pel* srcU = getCbAddr(uiSrcPartIdx);
    Pel* srcV = getCrAddr(uiSrcPartIdx);
    Pel* dstU = pcYuvDst->getCbAddr(0);
    Pel* dstV = pcYuvDst->getCrAddr(0);

    UInt srcstride = getCStride();
    UInt dststride = pcYuvDst->getCStride();

    UInt uiCHeight = pcYuvDst->getCHeight();
    UInt uiCWidth = pcYuvDst->getCWidth();

    x265::primitives.blockcpy_pp(uiCWidth, uiCHeight, (pixel*)dstU, dststride, (pixel*)srcU, srcstride);
    x265::primitives.blockcpy_pp(uiCWidth, uiCHeight, (pixel*)dstV, dststride, (pixel*)srcV, srcstride);
}

Void TComYuv::copyPartToPartYuv(TComYuv* pcYuvDst, UInt partIdx, UInt width, UInt height)
{
    copyPartToPartLuma(pcYuvDst, partIdx, width, height);
    copyPartToPartChroma(pcYuvDst, partIdx, width >> 1, height >> 1);
}

Void TComYuv::copyPartToPartYuv(TShortYUV* pcYuvDst, UInt partIdx, UInt width, UInt height)
{
    copyPartToPartLuma(pcYuvDst, partIdx, width, height);
    copyPartToPartChroma(pcYuvDst, partIdx, width >> 1, height >> 1);
}

Void TComYuv::copyPartToPartLuma(TComYuv* pcYuvDst, UInt partIdx, UInt width, UInt height)
{
    Pel* src =           getLumaAddr(partIdx);
    Pel* dst = pcYuvDst->getLumaAddr(partIdx);

    if (src == dst)
    {
        //th not a good idea
        //th best would be to fix the caller
        return;
    }

    UInt srcstride = getStride();
    UInt dststride = pcYuvDst->getStride();

    x265::primitives.blockcpy_pp(width, height, (pixel*)dst, dststride, (pixel*)src, srcstride);
}

Void TComYuv::copyPartToPartLuma(TShortYUV* pcYuvDst, UInt partIdx, UInt width, UInt height)
{
    Pel*   src =           getLumaAddr(partIdx);
    Short* dst = pcYuvDst->getLumaAddr(partIdx);

    UInt  srcstride = getStride();
    UInt  dststride = pcYuvDst->width;

    x265::primitives.blockcpy_sp(width, height, dst, dststride, (pixel*)src, srcstride);
}

Void TComYuv::copyPartToPartChroma(TComYuv* pcYuvDst, UInt partIdx, UInt width, UInt height)
{
    Pel*  srcU =           getCbAddr(partIdx);
    Pel*  srcV =           getCrAddr(partIdx);
    Pel*  dstU = pcYuvDst->getCbAddr(partIdx);
    Pel*  dstV = pcYuvDst->getCrAddr(partIdx);

    if (srcU == dstU && srcV == dstV)
    {
        //th not a good idea
        //th best would be to fix the caller
        return;
    }

    UInt   srcstride = getCStride();
    UInt   dststride = pcYuvDst->getCStride();

    x265::primitives.blockcpy_pp(width, height, (pixel*)dstU, dststride, (pixel*)srcU, srcstride);
    x265::primitives.blockcpy_pp(width, height, (pixel*)dstV, dststride, (pixel*)srcV, srcstride);
}

Void TComYuv::copyPartToPartChroma(TShortYUV* pcYuvDst, UInt partIdx, UInt width, UInt height)
{
    Pel*    srcU =           getCbAddr(partIdx);
    Pel*    srcV =           getCrAddr(partIdx);
    Short*  dstU = pcYuvDst->getCbAddr(partIdx);
    Short*  dstV = pcYuvDst->getCrAddr(partIdx);

    UInt   srcstride = getCStride();
    UInt   dststride = pcYuvDst->Cwidth;

    x265::primitives.blockcpy_sp(width, height, dstU, dststride, (pixel*)srcU, srcstride);
    x265::primitives.blockcpy_sp(width, height, dstV, dststride, (pixel*)srcV, srcstride);
}

Void TComYuv::copyPartToPartChroma(TComYuv* pcYuvDst, UInt partIdx, UInt width, UInt height, UInt chromaId)
{
    if (chromaId == 0)
    {
        Pel*  srcU =           getCbAddr(partIdx);
        Pel*  dstU = pcYuvDst->getCbAddr(partIdx);
        if (srcU == dstU)
        {
            return;
        }
        UInt   srcstride = getCStride();
        UInt   dststride = pcYuvDst->getCStride();
        x265::primitives.blockcpy_pp(width, height, (pixel*)dstU, dststride, (pixel*)srcU, srcstride);
    }
    else if (chromaId == 1)
    {
        Pel*  srcV =           getCrAddr(partIdx);
        Pel*  dstV = pcYuvDst->getCrAddr(partIdx);
        if (srcV == dstV)
        {
            return;
        }
        UInt   srcstride = getCStride();
        UInt   dststride = pcYuvDst->getCStride();
        x265::primitives.blockcpy_pp(width, height, (pixel*)dstV, dststride, (pixel*)srcV, srcstride);
    }
    else
    {
        Pel*  srcU =           getCbAddr(partIdx);
        Pel*  srcV =           getCrAddr(partIdx);
        Pel*  dstU = pcYuvDst->getCbAddr(partIdx);
        Pel*  dstV = pcYuvDst->getCrAddr(partIdx);

        if (srcU == dstU && srcV == dstV)
        {
            //th not a good idea
            //th best would be to fix the caller
            return;
        }
        UInt   srcstride = getCStride();
        UInt   dststride = pcYuvDst->getCStride();
        x265::primitives.blockcpy_pp(width, height, (pixel*)dstU, dststride, (pixel*)srcU, srcstride);
        x265::primitives.blockcpy_pp(width, height, (pixel*)dstV, dststride, (pixel*)srcV, srcstride);
    }
}

Void TComYuv::copyPartToPartChroma(TShortYUV* pcYuvDst, UInt partIdx, UInt width, UInt height, UInt chromaId)
{
    if (chromaId == 0)
    {
        Pel*    srcU =           getCbAddr(partIdx);
        Short*  dstU = pcYuvDst->getCbAddr(partIdx);

        UInt   srcstride = getCStride();
        UInt   dststride = pcYuvDst->Cwidth;

        x265::primitives.blockcpy_sp(width, height, dstU, dststride, (pixel*)srcU, srcstride);
    }
    else if (chromaId == 1)
    {
        Pel*  srcV =           getCrAddr(partIdx);
        Short*  dstV = pcYuvDst->getCrAddr(partIdx);

        UInt   srcstride = getCStride();
        UInt   dststride = pcYuvDst->Cwidth;

        x265::primitives.blockcpy_sp(width, height, dstV, dststride, (pixel*)srcV, srcstride);
    }
    else
    {
        Pel*    srcU =           getCbAddr(partIdx);
        Pel*    srcV =           getCrAddr(partIdx);
        Short*  dstU = pcYuvDst->getCbAddr(partIdx);
        Short*  dstV = pcYuvDst->getCrAddr(partIdx);

        UInt   srcstride = getCStride();
        UInt   dststride = pcYuvDst->Cwidth;

        x265::primitives.blockcpy_sp(width, height, dstU, dststride, (pixel*)srcU, srcstride);
        x265::primitives.blockcpy_sp(width, height, dstV, dststride, (pixel*)srcV, srcstride);
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
    UInt dststride  = getStride();

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
    Pel* dst  = getLumaAddr(trUnitIdx, partSize);

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

    Pel* pSrcU0 = srcYuv0->getCbAddr(trUnitIdx, partSize);
    Pel* pSrcU1 = srcYuv1->getCbAddr(trUnitIdx, partSize);
    Pel* pSrcV0 = srcYuv0->getCrAddr(trUnitIdx, partSize);
    Pel* pSrcV1 = srcYuv1->getCrAddr(trUnitIdx, partSize);
    Pel* dstU = getCbAddr(trUnitIdx, partSize);
    Pel* dstV = getCrAddr(trUnitIdx, partSize);

    UInt  iSrc0Stride = srcYuv0->getCStride();
    UInt  iSrc1Stride = srcYuv1->getCStride();
    UInt  dststride  = getCStride();

    for (y = partSize - 1; y >= 0; y--)
    {
        for (x = partSize - 1; x >= 0; x--)
        {
            dstU[x] = ClipC(static_cast<Short>(pSrcU0[x]) + pSrcU1[x]);
            dstV[x] = ClipC(static_cast<Short>(pSrcV0[x]) + pSrcV1[x]);
        }

        pSrcU0 += iSrc0Stride;
        pSrcU1 += iSrc1Stride;
        pSrcV0 += iSrc0Stride;
        pSrcV1 += iSrc1Stride;
        dstU  += dststride;
        dstV  += dststride;
    }
}

Void TComYuv::addClipChroma(TComYuv* srcYuv0, TShortYUV* srcYuv1, UInt trUnitIdx, UInt partSize)
{
    Int x, y;

    Pel* pSrcU0 = srcYuv0->getCbAddr(trUnitIdx, partSize);
    Short* pSrcU1 = srcYuv1->getCbAddr(trUnitIdx, partSize);
    Pel* pSrcV0 = srcYuv0->getCrAddr(trUnitIdx, partSize);
    Short* pSrcV1 = srcYuv1->getCrAddr(trUnitIdx, partSize);
    Pel* dstU = getCbAddr(trUnitIdx, partSize);
    Pel* dstV = getCrAddr(trUnitIdx, partSize);

    UInt  iSrc0Stride = srcYuv0->getCStride();
    UInt  iSrc1Stride = srcYuv1->Cwidth;
    UInt  dststride  = getCStride();

    for (y = partSize - 1; y >= 0; y--)
    {
        for (x = partSize - 1; x >= 0; x--)
        {
            dstU[x] = ClipC(static_cast<Short>(pSrcU0[x]) + pSrcU1[x]);
            dstV[x] = ClipC(static_cast<Short>(pSrcV0[x]) + pSrcV1[x]);
        }

        pSrcU0 += iSrc0Stride;
        pSrcU1 += iSrc1Stride;
        pSrcV0 += iSrc0Stride;
        pSrcV1 += iSrc1Stride;
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

    Int  iSrc0Stride = srcYuv0->getStride();
    Int  iSrc1Stride = srcYuv1->getStride();
    Int  dststride  = getStride();

    for (y = partSize - 1; y >= 0; y--)
    {
        for (x = partSize - 1; x >= 0; x--)
        {
            dst[x] = src0[x] - src1[x];
        }

        src0 += iSrc0Stride;
        src1 += iSrc1Stride;
        dst  += dststride;
    }
}

Void TComYuv::subtractChroma(TComYuv* srcYuv0, TComYuv* srcYuv1, UInt trUnitIdx, UInt partSize)
{
    Int x, y;

    Pel* pSrcU0 = srcYuv0->getCbAddr(trUnitIdx, partSize);
    Pel* pSrcU1 = srcYuv1->getCbAddr(trUnitIdx, partSize);
    Pel* pSrcV0 = srcYuv0->getCrAddr(trUnitIdx, partSize);
    Pel* pSrcV1 = srcYuv1->getCrAddr(trUnitIdx, partSize);
    Pel* dstU  = getCbAddr(trUnitIdx, partSize);
    Pel* dstV  = getCrAddr(trUnitIdx, partSize);

    Int  iSrc0Stride = srcYuv0->getCStride();
    Int  iSrc1Stride = srcYuv1->getCStride();
    Int  dststride  = getCStride();

    for (y = partSize - 1; y >= 0; y--)
    {
        for (x = partSize - 1; x >= 0; x--)
        {
            dstU[x] = pSrcU0[x] - pSrcU1[x];
            dstV[x] = pSrcV0[x] - pSrcV1[x];
        }

        pSrcU0 += iSrc0Stride;
        pSrcU1 += iSrc1Stride;
        pSrcV0 += iSrc0Stride;
        pSrcV1 += iSrc1Stride;
        dstU  += dststride;
        dstV  += dststride;
    }
}

Void TComYuv::addAvg(TComYuv* srcYuv0, TComYuv* srcYuv1, UInt partUnitIdx, UInt width, UInt height)
{
    Int x, y;

    Pel* pSrcY0  = srcYuv0->getLumaAddr(partUnitIdx);
    Pel* pSrcU0  = srcYuv0->getCbAddr(partUnitIdx);
    Pel* pSrcV0  = srcYuv0->getCrAddr(partUnitIdx);

    Pel* pSrcY1  = srcYuv1->getLumaAddr(partUnitIdx);
    Pel* pSrcU1  = srcYuv1->getCbAddr(partUnitIdx);
    Pel* pSrcV1  = srcYuv1->getCrAddr(partUnitIdx);

    Pel* pDstY   = getLumaAddr(partUnitIdx);
    Pel* dstU   = getCbAddr(partUnitIdx);
    Pel* dstV   = getCrAddr(partUnitIdx);

    UInt  iSrc0Stride = srcYuv0->getStride();
    UInt  iSrc1Stride = srcYuv1->getStride();
    UInt  dststride  = getStride();
    Int shiftNum = IF_INTERNAL_PREC + 1 - g_bitDepthY;
    Int offset = (1 << (shiftNum - 1)) + 2 * IF_INTERNAL_OFFS;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x += 4)
        {
            pDstY[x + 0] = ClipY((pSrcY0[x + 0] + pSrcY1[x + 0] + offset) >> shiftNum);
            pDstY[x + 1] = ClipY((pSrcY0[x + 1] + pSrcY1[x + 1] + offset) >> shiftNum);
            pDstY[x + 2] = ClipY((pSrcY0[x + 2] + pSrcY1[x + 2] + offset) >> shiftNum);
            pDstY[x + 3] = ClipY((pSrcY0[x + 3] + pSrcY1[x + 3] + offset) >> shiftNum);
        }

        pSrcY0 += iSrc0Stride;
        pSrcY1 += iSrc1Stride;
        pDstY  += dststride;
    }

    shiftNum = IF_INTERNAL_PREC + 1 - g_bitDepthC;
    offset = (1 << (shiftNum - 1)) + 2 * IF_INTERNAL_OFFS;

    iSrc0Stride = srcYuv0->getCStride();
    iSrc1Stride = srcYuv1->getCStride();
    dststride  = getCStride();

    width  >>= 1;
    height >>= 1;

    for (y = height - 1; y >= 0; y--)
    {
        for (x = width - 1; x >= 0; )
        {
            // note: chroma min width is 2
            dstU[x] = ClipC((pSrcU0[x] + pSrcU1[x] + offset) >> shiftNum);
            dstV[x] = ClipC((pSrcV0[x] + pSrcV1[x] + offset) >> shiftNum);
            x--;
            dstU[x] = ClipC((pSrcU0[x] + pSrcU1[x] + offset) >> shiftNum);
            dstV[x] = ClipC((pSrcV0[x] + pSrcV1[x] + offset) >> shiftNum);
            x--;
        }

        pSrcU0 += iSrc0Stride;
        pSrcU1 += iSrc1Stride;
        pSrcV0 += iSrc0Stride;
        pSrcV1 += iSrc1Stride;
        dstU  += dststride;
        dstV  += dststride;
    }
}

Void TComYuv::addAvg(TShortYUV* srcYuv0, TShortYUV* srcYuv1, UInt partUnitIdx, UInt width, UInt height)
{
    Int x, y;

    Short* pSrcY0  = srcYuv0->getLumaAddr(partUnitIdx);
    Short* pSrcU0  = srcYuv0->getCbAddr(partUnitIdx);
    Short* pSrcV0  = srcYuv0->getCrAddr(partUnitIdx);

    Short* pSrcY1  = srcYuv1->getLumaAddr(partUnitIdx);
    Short* pSrcU1  = srcYuv1->getCbAddr(partUnitIdx);
    Short* pSrcV1  = srcYuv1->getCrAddr(partUnitIdx);

    Pel* pDstY   = getLumaAddr(partUnitIdx);
    Pel* dstU   = getCbAddr(partUnitIdx);
    Pel* dstV   = getCrAddr(partUnitIdx);

    UInt  iSrc0Stride = srcYuv0->width;
    UInt  iSrc1Stride = srcYuv1->width;
    UInt  dststride  = getStride();
    Int shiftNum = IF_INTERNAL_PREC + 1 - g_bitDepthY;
    Int offset = (1 << (shiftNum - 1)) + 2 * IF_INTERNAL_OFFS;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x += 4)
        {
            pDstY[x + 0] = ClipY((pSrcY0[x + 0] + pSrcY1[x + 0] + offset) >> shiftNum);
            pDstY[x + 1] = ClipY((pSrcY0[x + 1] + pSrcY1[x + 1] + offset) >> shiftNum);
            pDstY[x + 2] = ClipY((pSrcY0[x + 2] + pSrcY1[x + 2] + offset) >> shiftNum);
            pDstY[x + 3] = ClipY((pSrcY0[x + 3] + pSrcY1[x + 3] + offset) >> shiftNum);
        }

        pSrcY0 += iSrc0Stride;
        pSrcY1 += iSrc1Stride;
        pDstY  += dststride;
    }

    shiftNum = IF_INTERNAL_PREC + 1 - g_bitDepthC;
    offset = (1 << (shiftNum - 1)) + 2 * IF_INTERNAL_OFFS;

    iSrc0Stride = srcYuv0->Cwidth;
    iSrc1Stride = srcYuv1->Cwidth;
    dststride  = getCStride();

    width  >>= 1;
    height >>= 1;

    for (y = height - 1; y >= 0; y--)
    {
        for (x = width - 1; x >= 0; )
        {
            // note: chroma min width is 2
            dstU[x] = ClipC((pSrcU0[x] + pSrcU1[x] + offset) >> shiftNum);
            dstV[x] = ClipC((pSrcV0[x] + pSrcV1[x] + offset) >> shiftNum);
            x--;
            dstU[x] = ClipC((pSrcU0[x] + pSrcU1[x] + offset) >> shiftNum);
            dstV[x] = ClipC((pSrcV0[x] + pSrcV1[x] + offset) >> shiftNum);
            x--;
        }

        pSrcU0 += iSrc0Stride;
        pSrcU1 += iSrc1Stride;
        pSrcV0 += iSrc0Stride;
        pSrcV1 += iSrc1Stride;
        dstU  += dststride;
        dstV  += dststride;
    }
}

#define DISABLING_CLIP_FOR_BIPREDME 0  // x265 disables this flag so 8bpp and 16bpp outputs match
                                       // the intent is for all HM bipred to be replaced with x264 logic

Void TComYuv::removeHighFreq(TComYuv* srcYuv, UInt partIdx, UInt uiWidht, UInt height)
{
    Int x, y;

    Pel* src  = srcYuv->getLumaAddr(partIdx);
    Pel* srcU = srcYuv->getCbAddr(partIdx);
    Pel* srcV = srcYuv->getCrAddr(partIdx);

    Pel* dst  = getLumaAddr(partIdx);
    Pel* dstU = getCbAddr(partIdx);
    Pel* dstV = getCrAddr(partIdx);

    Int  srcstride = srcYuv->getStride();
    Int  dststride = getStride();

    for (y = height - 1; y >= 0; y--)
    {
        for (x = uiWidht - 1; x >= 0; x--)
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
    uiWidht  >>= 1;

    for (y = height - 1; y >= 0; y--)
    {
        for (x = uiWidht - 1; x >= 0; x--)
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
