/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Min Chen <chenm003@163.com>
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
 * For more information, contact us at licensing@multicorewareinc.com.
 *****************************************************************************/

#include "primitives.h"
#include <cstring>
#include <assert.h>

namespace {
pixel CDECL predIntraGetPredValDC(pixel* pSrc, intptr_t iSrcStride, intptr_t iWidth, intptr_t iHeight, int bAbove, int bLeft)
{
    int iInd, iSum = 0;
    pixel pDcVal;

    if (bAbove)
    {
        for (iInd = 0; iInd < iWidth; iInd++)
        {
            iSum += pSrc[iInd - iSrcStride];
        }
    }
    if (bLeft)
    {
        for (iInd = 0; iInd < iHeight; iInd++)
        {
            iSum += pSrc[iInd * iSrcStride - 1];
        }
    }

    if (bAbove && bLeft)
    {
        pDcVal = (pixel)((iSum + iWidth) / (iWidth + iHeight));
    }
    else if (bAbove)
    {
        pDcVal = (pixel)((iSum + iWidth / 2) / iWidth);
    }
    else if (bLeft)
    {
        pDcVal = (pixel)((iSum + iHeight / 2) / iHeight);
    }
    else
    {
        pDcVal = pSrc[-1]; // Default DC value already calculated and placed in the prediction array if no neighbors are available
    }

    return pDcVal;
}

void xDCPredFiltering(pixel* pSrc, intptr_t iSrcStride, pixel* rpDst, intptr_t iDstStride, int iWidth, int iHeight)
{
    pixel* pDst = rpDst;
    intptr_t x, y, iDstStride2, iSrcStride2;

    // boundary pixels processing
    pDst[0] = (pixel)((pSrc[-iSrcStride] + pSrc[-1] + 2 * pDst[0] + 2) >> 2);

    for (x = 1; x < iWidth; x++)
    {
        pDst[x] = (pixel)((pSrc[x - iSrcStride] +  3 * pDst[x] + 2) >> 2);
    }

    for (y = 1, iDstStride2 = iDstStride, iSrcStride2 = iSrcStride - 1; y < iHeight; y++, iDstStride2 += iDstStride, iSrcStride2 += iSrcStride)
    {
        pDst[iDstStride2] = (pixel)((pSrc[iSrcStride2] + 3 * pDst[iDstStride2] + 2) >> 2);
    }
}

void xPredIntraDC(pixel* pSrc, intptr_t srcStride, pixel* rpDst, intptr_t dstStride, int width, int height, int blkAboveAvailable, int blkLeftAvailable, int bFilter)
{
    int k, l;
    int blkSize        = width;
    pixel* pDst          = rpDst;

    // Do the DC prediction
    pixel dcval = (pixel) predIntraGetPredValDC(pSrc, srcStride, width, height, blkAboveAvailable, blkLeftAvailable);

    for (k = 0; k < blkSize; k++)
    {
        for (l = 0; l < blkSize; l++)
        {
            pDst[k * dstStride + l] = dcval;
        }
    }
    if (bFilter && blkAboveAvailable && blkLeftAvailable)
    {
        xDCPredFiltering(pSrc, srcStride, pDst, dstStride, width, height);
    }
}
}

namespace x265 {
// x265 private namespace

void Setup_C_IPredPrimitives(EncoderPrimitives& p)
{
    p.getIPredDC = xPredIntraDC;
}
}
