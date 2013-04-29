/*****************************************************************************
 * x265: singleton thread pool and interface classes
 *****************************************************************************
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

#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <math.h>

#include "TLibCommon/CommonDef.h"
#include "TShortYUV.h"
#include "TLibCommon/TComYuv.h"


TShortYUV::TShortYUV()
{
    YBuf = NULL;
    CbBuf = NULL;
    CrBuf = NULL;
}

TShortYUV::~TShortYUV()
{}

void TShortYUV::create(unsigned int Width, unsigned int Height)
{
    YBuf  = (short*)xMalloc(short, Width * Height);
    CbBuf  = (short*)xMalloc(short, Width * Height >> 2);
    CrBuf  = (short*)xMalloc(short, Width * Height >> 2);

    // set width and height
    width   = Width;
    height  = Height;
    Cwidth  = Width  >> 1;
    Cheight = Height >> 1;
}

void TShortYUV::destroy()
{
    xFree(YBuf);
    YBuf = NULL;
    xFree(CbBuf);
    CbBuf = NULL;
    xFree(CrBuf);
    CrBuf = NULL;
}

void TShortYUV::clear()
{
    ::memset(YBuf, 0, (width  * height) * sizeof(short));
    ::memset(CbBuf, 0, (Cwidth * Cheight) * sizeof(short));
    ::memset(CrBuf, 0, (Cwidth * Cheight) * sizeof(short));
}

void TShortYUV::subtract(TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, unsigned int uiTrUnitIdx, unsigned int uiPartSize)
{
    subtractLuma(pcYuvSrc0, pcYuvSrc1,  uiTrUnitIdx, uiPartSize);
    subtractChroma(pcYuvSrc0, pcYuvSrc1,  uiTrUnitIdx, uiPartSize >> 1);
}


void TShortYUV::subtractLuma(TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, unsigned int uiTrUnitIdx, unsigned int uiPartSize)
{
    int x, y;

    Pel* pSrc0 = pcYuvSrc0->getLumaAddr(uiTrUnitIdx, uiPartSize);
    Pel* pSrc1 = pcYuvSrc1->getLumaAddr(uiTrUnitIdx, uiPartSize);
    Short* pDst  = getLumaAddr(uiTrUnitIdx, uiPartSize);

    int  iSrc0Stride = pcYuvSrc0->getStride();
    int  iSrc1Stride = pcYuvSrc1->getStride();
    int  iDstStride  = getStride();

    for (y = uiPartSize - 1; y >= 0; y--)
    {
        for (x = uiPartSize - 1; x >= 0; x--)
        {
            pDst[x] = static_cast<short>(pSrc0[x]) - static_cast<short>(pSrc1[x]);
        }

        pSrc0 += iSrc0Stride;
        pSrc1 += iSrc1Stride;
        pDst  += iDstStride;
    }
}

void TShortYUV::subtractChroma(TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, unsigned int uiTrUnitIdx, unsigned int uiPartSize)
{
    int x, y;

    Pel* pSrcU0 = pcYuvSrc0->getCbAddr(uiTrUnitIdx, uiPartSize);
    Pel* pSrcU1 = pcYuvSrc1->getCbAddr(uiTrUnitIdx, uiPartSize);
    Pel* pSrcV0 = pcYuvSrc0->getCrAddr(uiTrUnitIdx, uiPartSize);
    Pel* pSrcV1 = pcYuvSrc1->getCrAddr(uiTrUnitIdx, uiPartSize);
    Short* pDstU  = getCbAddr(uiTrUnitIdx, uiPartSize);
    Short* pDstV  = getCrAddr(uiTrUnitIdx, uiPartSize);

    int  iSrc0Stride = pcYuvSrc0->getCStride();
    int  iSrc1Stride = pcYuvSrc1->getCStride();
    int  iDstStride  = getCStride();

    for (y = uiPartSize - 1; y >= 0; y--)
    {
        for (x = uiPartSize - 1; x >= 0; x--)
        {
            pDstU[x] = static_cast<short>(pSrcU0[x]) - static_cast<short>(pSrcU1[x]);
            pDstV[x] = static_cast<short>(pSrcV0[x]) - static_cast<short>(pSrcV1[x]);
        }

        pSrcU0 += iSrc0Stride;
        pSrcU1 += iSrc1Stride;
        pSrcV0 += iSrc0Stride;
        pSrcV1 += iSrc1Stride;
        pDstU  += iDstStride;
        pDstV  += iDstStride;
    }
}


void TShortYUV::addClip(TShortYUV* pcYuvSrc0, TShortYUV* pcYuvSrc1, unsigned int uiTrUnitIdx, unsigned int uiPartSize)
{
    addClipLuma(pcYuvSrc0, pcYuvSrc1, uiTrUnitIdx, uiPartSize);
    addClipChroma(pcYuvSrc0, pcYuvSrc1, uiTrUnitIdx, uiPartSize >> 1);
}

#if _MSC_VER
#pragma warning (disable: 4244)
#endif

void TShortYUV::addClipLuma(TShortYUV* pcYuvSrc0, TShortYUV* pcYuvSrc1, unsigned int uiTrUnitIdx, unsigned int uiPartSize)
{
    int x, y;

    short* pSrc0 = pcYuvSrc0->getLumaAddr(uiTrUnitIdx, uiPartSize);
    short* pSrc1 = pcYuvSrc1->getLumaAddr(uiTrUnitIdx, uiPartSize);
    short* pDst  = getLumaAddr(uiTrUnitIdx, uiPartSize);

    unsigned int iSrc0Stride = pcYuvSrc0->getStride();
    unsigned int iSrc1Stride = pcYuvSrc1->getStride();
    unsigned int iDstStride  = getStride();

    for (y = uiPartSize - 1; y >= 0; y--)
    {
        for (x = uiPartSize - 1; x >= 0; x--)
        {
            pDst[x] = ClipY(pSrc0[x] + pSrc1[x]);
        }

        pSrc0 += iSrc0Stride;
        pSrc1 += iSrc1Stride;
        pDst  += iDstStride;
    }
}

void TShortYUV::addClipChroma(TShortYUV* pcYuvSrc0, TShortYUV* pcYuvSrc1, unsigned int uiTrUnitIdx, unsigned int uiPartSize)
{
    int x, y;

    short* pSrcU0 = pcYuvSrc0->getCbAddr(uiTrUnitIdx, uiPartSize);
    short* pSrcU1 = pcYuvSrc1->getCbAddr(uiTrUnitIdx, uiPartSize);
    short* pSrcV0 = pcYuvSrc0->getCrAddr(uiTrUnitIdx, uiPartSize);
    short* pSrcV1 = pcYuvSrc1->getCrAddr(uiTrUnitIdx, uiPartSize);
    short* pDstU = getCbAddr(uiTrUnitIdx, uiPartSize);
    short* pDstV = getCrAddr(uiTrUnitIdx, uiPartSize);

    unsigned int  iSrc0Stride = pcYuvSrc0->getCStride();
    unsigned int  iSrc1Stride = pcYuvSrc1->getCStride();
    unsigned int  iDstStride  = getCStride();

    for (y = uiPartSize - 1; y >= 0; y--)
    {
        for (x = uiPartSize - 1; x >= 0; x--)
        {
            pDstU[x] = ClipC(pSrcU0[x] + pSrcU1[x]);
            pDstV[x] = ClipC(pSrcV0[x] + pSrcV1[x]);
        }

        pSrcU0 += iSrc0Stride;
        pSrcU1 += iSrc1Stride;
        pSrcV0 += iSrc0Stride;
        pSrcV1 += iSrc1Stride;
        pDstU  += iDstStride;
        pDstV  += iDstStride;
    }
}

#if _MSC_VER
#pragma warning (default: 4244)
#endif


void TShortYUV::copyPartToPartYuv(TShortYUV* pcYuvDst, unsigned int uiPartIdx, unsigned int iWidth, unsigned int iHeight)
{
    copyPartToPartLuma(pcYuvDst, uiPartIdx, iWidth, iHeight);
    copyPartToPartChroma(pcYuvDst, uiPartIdx, iWidth >> 1, iHeight >> 1);
}

void TShortYUV::copyPartToPartYuv(TComYuv* pcYuvDst, unsigned int uiPartIdx, unsigned int iWidth, unsigned int iHeight)
{
    copyPartToPartLuma(pcYuvDst, uiPartIdx, iWidth, iHeight);
    copyPartToPartChroma(pcYuvDst, uiPartIdx, iWidth >> 1, iHeight >> 1);
}

Void TShortYUV::copyPartToPartLuma(TShortYUV* pcYuvDst, unsigned int uiPartIdx, unsigned int iWidth, unsigned int iHeight)
{
    short* pSrc =           getLumaAddr(uiPartIdx);
    short* pDst = pcYuvDst->getLumaAddr(uiPartIdx);

    if (pSrc == pDst)
    {
        //th not a good idea
        //th best would be to fix the caller
        return;
    }

    unsigned int  iSrcStride = getStride();
    unsigned int  iDstStride = pcYuvDst->getStride();
    for (unsigned int y = iHeight; y != 0; y--)
    {
        ::memcpy(pDst, pSrc, iWidth * sizeof(short));
        pSrc += iSrcStride;
        pDst += iDstStride;
    }
}

Void TShortYUV::copyPartToPartLuma(TComYuv* pcYuvDst, unsigned int uiPartIdx, unsigned int iWidth, unsigned int iHeight)
{
    short* pSrc =           getLumaAddr(uiPartIdx);
    Pel* pDst = pcYuvDst->getLumaAddr(uiPartIdx);

    unsigned int  iSrcStride = getStride();
    unsigned int  iDstStride = pcYuvDst->getStride();
    for (unsigned int y = iHeight; y != 0; y--)
    {
        for(int x = 0; x < iWidth; x++)
            pDst[x] = (Pel) (pSrc[x]);

        pSrc += iSrcStride;
        pDst += iDstStride;
    }
}


Void TShortYUV::copyPartToPartChroma(TShortYUV* pcYuvDst, unsigned int uiPartIdx, unsigned int iWidth, unsigned int iHeight)
{
    short*  pSrcU =           getCbAddr(uiPartIdx);
    short*  pSrcV =           getCrAddr(uiPartIdx);
    short*  pDstU = pcYuvDst->getCbAddr(uiPartIdx);
    short*  pDstV = pcYuvDst->getCrAddr(uiPartIdx);

    if (pSrcU == pDstU && pSrcV == pDstV)
    {
        //th not a good idea
        //th best would be to fix the caller
        return;
    }

    unsigned int   iSrcStride = getCStride();
    unsigned int   iDstStride = pcYuvDst->getCStride();
    for (unsigned int y = iHeight; y != 0; y--)
    {
        ::memcpy(pDstU, pSrcU, iWidth * sizeof(short));
        ::memcpy(pDstV, pSrcV, iWidth * sizeof(short));
        pSrcU += iSrcStride;
        pSrcV += iSrcStride;
        pDstU += iDstStride;
        pDstV += iDstStride;
    }
}

Void TShortYUV::copyPartToPartChroma(TComYuv* pcYuvDst, unsigned int uiPartIdx, unsigned int iWidth, unsigned int iHeight)
{
    short*  pSrcU =           getCbAddr(uiPartIdx);
    short*  pSrcV =           getCrAddr(uiPartIdx);
    Pel*  pDstU = pcYuvDst->getCbAddr(uiPartIdx);
    Pel*  pDstV = pcYuvDst->getCrAddr(uiPartIdx);

    unsigned int   iSrcStride = getCStride();
    unsigned int   iDstStride = pcYuvDst->getCStride();
    for (unsigned int y = iHeight; y != 0; y--)
    {
       for(int x = 0; x < iWidth; x++)
       {
           pDstU[x] = (Pel)(pSrcU[x]);
           pDstV[x] = (Pel)(pSrcV[x]);
       }
        pSrcU += iSrcStride;
        pSrcV += iSrcStride;
        pDstU += iDstStride;
        pDstV += iDstStride;
    }
}


Void TShortYUV::copyPartToPartChroma(TShortYUV* pcYuvDst, unsigned int uiPartIdx, unsigned int iWidth, unsigned int iHeight, unsigned int chromaId)
{
    if (chromaId == 0)
    {
        short*  pSrcU =           getCbAddr(uiPartIdx);
        short*  pDstU = pcYuvDst->getCbAddr(uiPartIdx);
        if (pSrcU == pDstU)
        {
            return;
        }
        unsigned int   iSrcStride = getCStride();
        unsigned int   iDstStride = pcYuvDst->getCStride();
        for (unsigned int y = iHeight; y != 0; y--)
        {
            ::memcpy(pDstU, pSrcU, iWidth * sizeof(short));
            pSrcU += iSrcStride;
            pDstU += iDstStride;
        }
    }
    else if (chromaId == 1)
    {
        short*  pSrcV =           getCrAddr(uiPartIdx);
        short*  pDstV = pcYuvDst->getCrAddr(uiPartIdx);
        if (pSrcV == pDstV)
        {
            return;
        }
        unsigned int   iSrcStride = getCStride();
        unsigned int   iDstStride = pcYuvDst->getCStride();
        for (unsigned int y = iHeight; y != 0; y--)
        {
            ::memcpy(pDstV, pSrcV, iWidth * sizeof(short));
            pSrcV += iSrcStride;
            pDstV += iDstStride;
        }
    }
    else
    {
        short*  pSrcU =           getCbAddr(uiPartIdx);
        short*  pSrcV =           getCrAddr(uiPartIdx);
        short*  pDstU = pcYuvDst->getCbAddr(uiPartIdx);
        short*  pDstV = pcYuvDst->getCrAddr(uiPartIdx);

        if (pSrcU == pDstU && pSrcV == pDstV)
        {
            //th not a good idea
            //th best would be to fix the caller
            return;
        }
        unsigned int   iSrcStride = getCStride();
        unsigned int   iDstStride = pcYuvDst->getCStride();
        for (unsigned int y = iHeight; y != 0; y--)
        {
            ::memcpy(pDstU, pSrcU, iWidth * sizeof(short));
            ::memcpy(pDstV, pSrcV, iWidth * sizeof(short));
            pSrcU += iSrcStride;
            pSrcV += iSrcStride;
            pDstU += iDstStride;
            pDstV += iDstStride;
        }
    }
}

Void TShortYUV::copyPartToPartChroma(TComYuv* pcYuvDst, unsigned int uiPartIdx, unsigned int iWidth, unsigned int iHeight, unsigned int chromaId)
{
    if (chromaId == 0)
    {
        short*  pSrcU =           getCbAddr(uiPartIdx);
        Pel*  pDstU = pcYuvDst->getCbAddr(uiPartIdx);
        unsigned int   iSrcStride = getCStride();
        unsigned int   iDstStride = pcYuvDst->getCStride();
        for (unsigned int y = iHeight; y != 0; y--)
        {
            for(int x = 0; x < iWidth; x++)
            {
                pDstU[x] = (Pel)(pSrcU[x]);
            }
            pSrcU += iSrcStride;
            pDstU += iDstStride;
        }
    }
    else if (chromaId == 1)
    {
        short*  pSrcV =           getCrAddr(uiPartIdx);
        Pel*  pDstV = pcYuvDst->getCrAddr(uiPartIdx);
        unsigned int   iSrcStride = getCStride();
        unsigned int   iDstStride = pcYuvDst->getCStride();
        for (unsigned int y = iHeight; y != 0; y--)
        {
            for(int x = 0; x < iWidth; x++)
            {
                pDstV[x] = (Pel)(pSrcV[x]);
            }
            pSrcV += iSrcStride;
            pDstV += iDstStride;
        }
    }
    else
    {
        short*  pSrcU =           getCbAddr(uiPartIdx);
        short*  pSrcV =           getCrAddr(uiPartIdx);
        Pel*  pDstU = pcYuvDst->getCbAddr(uiPartIdx);
        Pel*  pDstV = pcYuvDst->getCrAddr(uiPartIdx);

        unsigned int   iSrcStride = getCStride();
        unsigned int   iDstStride = pcYuvDst->getCStride();
        for (unsigned int y = iHeight; y != 0; y--)
        {
            for(int x = 0; x < iWidth; x++)
            {
                pDstU[x] = (Pel)(pSrcU[x]);
                pDstV[x] = (Pel)(pSrcV[x]);
            }
            pSrcU += iSrcStride;
            pSrcV += iSrcStride;
            pDstU += iDstStride;
            pDstV += iDstStride;
        }
    }
}
