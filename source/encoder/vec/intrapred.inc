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
    //assert(iWidth == iHeight); // all of Intra is NxN
    int iSum = 0;
    int logSize = g_aucConvertToBit[iWidth] + 2;
    pixel *pSrcAbove = &pSrc[-iSrcStride];

#if HIGH_BIT_DEPTH
    Vec8s sumAbove(0);
    Vec8s m0;

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
        iSum = horizontal_add_x(sumAbove);
    }

    if (bLeft)
    {
        for (int iInd = 0; iInd < iHeight; iInd++)
        {
            iSum += pSrc[iInd * iSrcStride - 1];
        }
    }

//     assert(bAbove || bLeft);
    logSize += (bAbove + bLeft - 1);
    return (iSum + (1<<(logSize-1))) >> logSize;

#else

    if (bAbove)
    {
        Vec16uc pix;
        Vec8us  im;

        switch( iWidth )
        {
            case 4:
                pix.fromUint32(*(uint32_t*)pSrcAbove);
                iSum = horizontal_add(extend_low(pix));
                break;
            case 8:
#if _WIN64
                pix.fromUint64(*(uint64_t*)pSrcAbove);
#else
                pix.load_partial(8, pSrcAbove);
#endif
                iSum = horizontal_add(extend_low(pix));
                break;
            case 16:
                pix.load(pSrcAbove);
                iSum = horizontal_add_x(pix);
                break;
            case 32:
                pix.load(pSrcAbove);
                im = extend_low(pix) + extend_high(pix);
                pix.load(pSrcAbove + 16);
                im += extend_low(pix) + extend_high(pix);
                iSum += horizontal_add(im);
                break;
            //case 64:
            default:
                pix.load(pSrcAbove);
                im = extend_low(pix) + extend_high(pix);
                pix.load(pSrcAbove + 16);
                im += extend_low(pix) + extend_high(pix);
                pix.load(pSrcAbove + 32);
                im += extend_low(pix) + extend_high(pix);
                pix.load(pSrcAbove + 48);
                im += extend_low(pix) + extend_high(pix);
                iSum += horizontal_add(im);
                break;
        }
    }

    if (bLeft)
    {
        for (int iInd = 0; iInd < iHeight; iInd++)
        {
            iSum += pSrc[iInd * iSrcStride - 1];
        }
    }

    logSize += (bAbove + bLeft - 1);
    return (iSum + (1<<(logSize-1))) >> logSize;
#endif
}
}

void Setup_Vec_IPredPrimitives(EncoderPrimitives& p)
{
    p.getdcval_p = predIntraGetPredValDC;
}
