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
#include <assert.h>
#include <stdio.h>

#if _MSC_VER
#pragma warning(default: 4127) // conditional expression is constant, typical for templated functions
#pragma warning(default: 4100)
#endif

namespace {

// Function for calculating DC value of the reference samples used in Intra prediction
pixel CDECL predIntraGetPredValDC(pixel* pSrc, intptr_t iSrcStride, intptr_t iWidth, intptr_t iHeight, int bAbove, int bLeft)
{
#if HIGH_BIT_DEPTH
    int iInd, iSum = 0;
    pixel pDcVal;
    int logSize;

    //assert(iWidth == iHeight); // all of Intra is NxN
    switch( iWidth )
    {
    case  4: logSize = 2; break;
    case  8: logSize = 3; break;
    case 16: logSize = 4; break;
    case 32: logSize = 5; break;
    default/*case 64*/: logSize = 6; break;
    }

    Vec8s sumAbove(0);
    Vec8s m0;
    pixel *pSrcAbove = &pSrc[-iSrcStride];

    if (bAbove)
    {
        switch( iWidth )
        {
            case 4:
                m0.load_partial4(pSrcAbove);
                sumAbove = m0;
                break;
            case 8:
                m0.load(pSrcAbove);
                sumAbove = m0;
                break;
            case 16:
                m0.load(pSrcAbove    );
                sumAbove  = m0;
                m0.load(pSrcAbove + 8);
                sumAbove += m0;
                break;
            case 32:
                m0.load(pSrcAbove    );
                sumAbove  = m0;
                m0.load(pSrcAbove + 8);
                sumAbove += m0;
                m0.load(pSrcAbove +16);
                sumAbove += m0;
                m0.load(pSrcAbove +24);
                sumAbove += m0;
                break;
                //case 64:
            default:
                // CHECK_ME: the max support bit_depth is 13-bits
                m0.load(pSrcAbove    );
                sumAbove  = m0;
                m0.load(pSrcAbove + 8);
                sumAbove += m0;
                m0.load(pSrcAbove +16);
                sumAbove += m0;
                m0.load(pSrcAbove +24);
                sumAbove += m0;
                m0.load(pSrcAbove +32);
                sumAbove += m0;
                m0.load(pSrcAbove +40);
                sumAbove += m0;
                m0.load(pSrcAbove +48);
                sumAbove += m0;
                m0.load(pSrcAbove +56);
                sumAbove += m0;
                break;
        }
        iSum = horizontal_add(sumAbove);
    }

    if (bLeft)
    {
        for (iInd = 0; iInd < iHeight; iInd++)
        {
            iSum += pSrc[iInd * iSrcStride - 1];
        }
    }

//     assert(bAbove || bLeft);
    logSize += (bAbove + bLeft - 1);
    pDcVal = (iSum + (1<<(logSize-1))) >> logSize;

#else
    // CHECK_ME: un-test code
    Int iInd, iSum = 0;
    Pel pDcVal;
    int logSize = g_aucConvertToBit[iWidth] + 2;

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

    logSize += (bAbove + bLeft - 1);
    pDcVal = (iSum + (1<<(logSize-1))) >> logSize;

    return pDcVal;
#endif

    return pDcVal;
}

}

void Setup_Vec_IPredPrimitives(EncoderPrimitives& p)
{
    p.getdcval_p = predIntraGetPredValDC;
}
