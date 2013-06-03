/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Min Chen <chenm003@163.com>
 *          Deepthi Devaki <deepthidevaki@multicorewareinc.com>
 *          Steve Borho <steve@borho.org> 
 *          ShinYee Chung <shinyee@multicorewareinc.com>
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
#include "Lib/TLibCommon/TComPrediction.h"
#include <smmintrin.h>

using namespace x265;

namespace {

const int angAP[17][64] =
{
    {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64
    },
    {
        0, 1, 2, 3, 4, 4, 5, 6, 7, 8, 8, 9, 10, 11, 12, 13, 13, 14, 15, 16, 17, 17, 18, 19, 20, 21, 21, 22, 23, 24, 25, 26, 26, 27, 28, 29, 30, 30, 31, 32, 33, 34, 34, 35, 36, 37, 38, 39, 39, 40, 41, 42, 43, 43, 44, 45, 46, 47, 47, 48, 49, 50, 51, 52
    },
    {
        0, 1, 1, 2, 3, 3, 4, 5, 5, 6, 7, 7, 8, 9, 9, 10, 11, 11, 12, 13, 13, 14, 15, 15, 16, 17, 17, 18, 19, 19, 20, 21, 21, 22, 22, 23, 24, 24, 25, 26, 26, 27, 28, 28, 29, 30, 30, 31, 32, 32, 33, 34, 34, 35, 36, 36, 37, 38, 38, 39, 40, 40, 41, 42
    },
    {
        0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 23, 24, 24, 25, 26, 26, 27, 27, 28, 28, 29, 29, 30, 30, 31, 31, 32, 32, 33, 34
    },
    {
        0, 0, 1, 1, 2, 2, 2, 3, 3, 4, 4, 4, 5, 5, 6, 6, 6, 7, 7, 8, 8, 8, 9, 9, 10, 10, 10, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 15, 16, 16, 17, 17, 17, 18, 18, 19, 19, 19, 20, 20, 21, 21, 21, 22, 22, 23, 23, 23, 24, 24, 25, 25, 26
    },
    {
        0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 9, 9, 9, 9, 10, 10, 10, 10, 11, 11, 11, 12, 12, 12, 12, 13, 13, 13, 14, 14, 14, 14, 15, 15, 15, 16, 16, 16, 16, 17, 17, 17, 18
    },
    {
        0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 10
    },
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4
    },
    { // 0th virtual index; never used; just to help indexing
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4
    },
    {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4
    },
    {
        -1, -1, -1, -1, -1, -1, -2, -2, -2, -2, -2, -2, -3, -3, -3, -3, -3, -3, -3, -4, -4, -4, -4, -4, -4, -5, -5, -5, -5, -5, -5, -5, -6, -6, -6, -6, -6, -6, -7, -7, -7, -7, -7, -7, -8, -8, -8, -8, -8, -8, -8, -9, -9, -9, -9, -9, -9, -10, -10, -10, -10, -10, -10, -10
    },
    {
        -1, -1, -1, -2, -2, -2, -2, -3, -3, -3, -4, -4, -4, -4, -5, -5, -5, -6, -6, -6, -6, -7, -7, -7, -8, -8, -8, -8, -9, -9, -9, -9, -10, -10, -10, -11, -11, -11, -11, -12, -12, -12, -13, -13, -13, -13, -14, -14, -14, -15, -15, -15, -15, -16, -16, -16, -17, -17, -17, -17, -18, -18, -18, -18
    },
    {
        -1, -1, -2, -2, -3, -3, -3, -4, -4, -5, -5, -5, -6, -6, -7, -7, -7, -8, -8, -9, -9, -9, -10, -10, -11, -11, -11, -12, -12, -13, -13, -13, -14, -14, -15, -15, -16, -16, -16, -17, -17, -18, -18, -18, -19, -19, -20, -20, -20, -21, -21, -22, -22, -22, -23, -23, -24, -24, -24, -25, -25, -26, -26, -26
    },
    {
        -1, -2, -2, -3, -3, -4, -4, -5, -5, -6, -6, -7, -7, -8, -8, -9, -10, -10, -11, -11, -12, -12, -13, -13, -14, -14, -15, -15, -16, -16, -17, -17, -18, -19, -19, -20, -20, -21, -21, -22, -22, -23, -23, -24, -24, -25, -25, -26, -27, -27, -28, -28, -29, -29, -30, -30, -31, -31, -32, -32, -33, -33, -34, -34
    },
    {
        -1, -2, -2, -3, -4, -4, -5, -6, -6, -7, -8, -8, -9, -10, -10, -11, -12, -12, -13, -14, -14, -15, -16, -16, -17, -18, -18, -19, -20, -20, -21, -21, -22, -23, -23, -24, -25, -25, -26, -27, -27, -28, -29, -29, -30, -31, -31, -32, -33, -33, -34, -35, -35, -36, -37, -37, -38, -39, -39, -40, -41, -41, -42, -42
    },
    {
        -1, -2, -3, -4, -5, -5, -6, -7, -8, -9, -9, -10, -11, -12, -13, -13, -14, -15, -16, -17, -18, -18, -19, -20, -21, -22, -22, -23, -24, -25, -26, -26, -27, -28, -29, -30, -31, -31, -32, -33, -34, -35, -35, -36, -37, -38, -39, -39, -40, -41, -42, -43, -44, -44, -45, -46, -47, -48, -48, -49, -50, -51, -52, -52
    },
    {
        -1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, -14, -15, -16, -17, -18, -19, -20, -21, -22, -23, -24, -25, -26, -27, -28, -29, -30, -31, -32, -33, -34, -35, -36, -37, -38, -39, -40, -41, -42, -43, -44, -45, -46, -47, -48, -49, -50, -51, -52, -53, -54, -55, -56, -57, -58, -59, -60, -61, -62, -63, -64
    }
};

#define GETAP(X, Y) angAP[8 - (X)][(Y)]

static inline
void xDCPredFiltering(pixel* pSrc, intptr_t iSrcStride, pixel* rpDst, intptr_t iDstStride, int iWidth, int /*iHeight*/)
{
    pixel* pDst = rpDst;
    int y;
    pixel pixDC = *pDst;
    int pixDCx3 = pixDC * 3 + 2;

    // boundary pixels processing
    pDst[0] = (pixel)((pSrc[-iSrcStride] + pSrc[-1] + 2 * pixDC + 2) >> 2);

    Vec8us im1(pixDCx3);
    Vec8us im2, im3;
#if HIGH_BIT_DEPTH
    switch (iWidth)
    {
    case 4:
        im2 = load_partial(const_int(8), &pSrc[1 - iSrcStride]);
        im2 = (im1 + im2) >> const_int(2);
        store_partial(const_int(8), &pDst[1], im2);
        break;

    case 8:
        im2.load(&pSrc[1 - iSrcStride]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&pDst[1]);
        break;

    case 16:
        im2.load(&pSrc[1 - iSrcStride]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&pDst[1]);

        im2.load(&pSrc[1 - iSrcStride + 8]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&pDst[1 + 8]);
        break;

    case 32:
        im2.load(&pSrc[1 - iSrcStride]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&pDst[1]);

        im2.load(&pSrc[1 - iSrcStride + 8]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&pDst[1 + 8]);

        im2.load(&pSrc[1 - iSrcStride + 16]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&pDst[1 + 16]);

        im2.load(&pSrc[1 - iSrcStride + 24]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&pDst[1 + 24]);
        break;

    //case 64:
    default:
        im2.load(&pSrc[1 - iSrcStride]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&pDst[1]);

        im2.load(&pSrc[1 - iSrcStride + 8]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&pDst[1 + 8]);

        im2.load(&pSrc[1 - iSrcStride + 16]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&pDst[1 + 16]);

        im2.load(&pSrc[1 - iSrcStride + 24]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&pDst[1 + 24]);

        im2.load(&pSrc[1 - iSrcStride + 32]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&pDst[1 + 32]);

        im2.load(&pSrc[1 - iSrcStride + 40]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&pDst[1 + 40]);

        im2.load(&pSrc[1 - iSrcStride + 48]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&pDst[1 + 48]);

        im2.load(&pSrc[1 - iSrcStride + 56]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&pDst[1 + 56]);
        break;
    }

#else /* if HIGH_BIT_DEPTH */
    Vec16uc pix;
    switch (iWidth)
    {
    case 4:
        pix = load_partial(const_int(4), &pSrc[1 - iSrcStride]);
        im2 = extend_low(pix);
        im2 = (im1 + im2) >> const_int(2);
        pix = compress(im2, im2);
        store_partial(const_int(4), &pDst[1], pix);
        break;

    case 8:
        pix = load_partial(const_int(8), &pSrc[1 - iSrcStride]);
        im2 = extend_low(pix);
        im2 = (im1 + im2) >> const_int(2);
        pix = compress(im2, im2);
        store_partial(const_int(8), &pDst[1], pix);
        break;

    case 16:
        pix.load(&pSrc[1 - iSrcStride]);
        im2 = extend_low(pix);
        im3 = extend_high(pix);
        im2 = (im1 + im2) >> const_int(2);
        im3 = (im1 + im3) >> const_int(2);
        pix = compress(im2, im3);
        pix.store(&pDst[1]);
        break;

    case 32:
        pix.load(&pSrc[1 - iSrcStride]);
        im2 = extend_low(pix);
        im3 = extend_high(pix);
        im2 = (im1 + im2) >> const_int(2);
        im3 = (im1 + im3) >> const_int(2);
        pix = compress(im2, im3);
        pix.store(&pDst[1]);

        pix.load(&pSrc[1 - iSrcStride + 16]);
        im2 = extend_low(pix);
        im3 = extend_high(pix);
        im2 = (im1 + im2) >> const_int(2);
        im3 = (im1 + im3) >> const_int(2);
        pix = compress(im2, im3);
        pix.store(&pDst[1 + 16]);
        break;

    //case 64:
    default:
        pix.load(&pSrc[1 - iSrcStride]);
        im2 = extend_low(pix);
        im3 = extend_high(pix);
        im2 = (im1 + im2) >> const_int(2);
        im3 = (im1 + im3) >> const_int(2);
        pix = compress(im2, im3);
        pix.store(&pDst[1]);

        pix.load(&pSrc[1 - iSrcStride + 16]);
        im2 = extend_low(pix);
        im3 = extend_high(pix);
        im2 = (im1 + im2) >> const_int(2);
        im3 = (im1 + im3) >> const_int(2);
        pix = compress(im2, im3);
        pix.store(&pDst[1 + 16]);

        pix.load(&pSrc[1 - iSrcStride + 32]);
        im2 = extend_low(pix);
        im3 = extend_high(pix);
        im2 = (im1 + im2) >> const_int(2);
        im3 = (im1 + im3) >> const_int(2);
        pix = compress(im2, im3);
        pix.store(&pDst[1 + 32]);

        pix.load(&pSrc[1 - iSrcStride + 48]);
        im2 = extend_low(pix);
        im3 = extend_high(pix);
        im2 = (im1 + im2) >> const_int(2);
        im3 = (im1 + im3) >> const_int(2);
        pix = compress(im2, im3);
        pix.store(&pDst[1 + 48]);
        break;
    }

#endif /* if HIGH_BIT_DEPTH */

    for (y = 1; y < iWidth; y++)
    {
        pDst[iDstStride] = (pixel)((pSrc[iSrcStride - 1] + pixDCx3) >> 2);
        pSrc += iSrcStride;
        pDst += iDstStride;
    }
}

void predIntraDC(pixel* pSrc, intptr_t srcStride, pixel* pDst, intptr_t dstStride, int width, int /*height*/, int blkAboveAvailable, int blkLeftAvailable, int bFilter)
{
    //assert(iWidth == iHeight); // all of Intra is NxN
    //assert(blkAboveAvailable || blkLeftAvailable); // I think left and above always true since HEVC have a pixel fill process
    int iSum = 0;
    int logSize = g_aucConvertToBit[width] + 2;
    pixel *pSrcAbove = &pSrc[-srcStride];
    pixel *pSrcLeft = &pSrc[-1];

    if (blkLeftAvailable)
    {
        for (int iInd = 0; iInd < width; iInd++)
        {
            iSum += *pSrcLeft;
            pSrcLeft += srcStride;
        }
    }

#if HIGH_BIT_DEPTH
    Vec8s sumAbove(0);
    Vec8s m0;

    if (blkAboveAvailable)
    {
        switch (width)
        {
        case 4:
            sumAbove = load_partial(const_int(8), pSrcAbove);
            break;
        case 8:
            m0.load(pSrcAbove);
            sumAbove = m0;
            break;
        case 16:
            m0.load(pSrcAbove);
            sumAbove  = m0;
            m0.load(pSrcAbove + 8);
            sumAbove += m0;
            break;
        case 32:
            m0.load(pSrcAbove);
            sumAbove  = m0;
            m0.load(pSrcAbove + 8);
            sumAbove += m0;
            m0.load(pSrcAbove + 16);
            sumAbove += m0;
            m0.load(pSrcAbove + 24);
            sumAbove += m0;
            break;
        //case 64:
        default:
            // CHECK_ME: the max support bit_depth is 13-bits
            m0.load(pSrcAbove);
            sumAbove  = m0;
            m0.load(pSrcAbove + 8);
            sumAbove += m0;
            m0.load(pSrcAbove + 16);
            sumAbove += m0;
            m0.load(pSrcAbove + 24);
            sumAbove += m0;
            m0.load(pSrcAbove + 32);
            sumAbove += m0;
            m0.load(pSrcAbove + 40);
            sumAbove += m0;
            m0.load(pSrcAbove + 48);
            sumAbove += m0;
            m0.load(pSrcAbove + 56);
            sumAbove += m0;
            break;
        }

        iSum += horizontal_add_x(sumAbove);
    }

    logSize += (blkAboveAvailable + blkLeftAvailable - 1);
    pixel dcVal = (iSum + (1 << (logSize - 1))) >> logSize;
    Vec8us dcValN(dcVal);
    int k;

    pixel *pDst1 = pDst;
    switch (width)
    {
    case 4:
        store_partial(const_int(8), pDst1, dcValN);
        pDst1 += dstStride;
        store_partial(const_int(8), pDst1, dcValN);
        pDst1 += dstStride;
        store_partial(const_int(8), pDst1, dcValN);
        pDst1 += dstStride;
        store_partial(const_int(8), pDst1, dcValN);
        pDst1 += dstStride;
        break;

    case 8:
        dcValN.store(pDst1);
        pDst1 += dstStride;
        dcValN.store(pDst1);
        pDst1 += dstStride;
        dcValN.store(pDst1);
        pDst1 += dstStride;
        dcValN.store(pDst1);
        pDst1 += dstStride;
        dcValN.store(pDst1);
        pDst1 += dstStride;
        dcValN.store(pDst1);
        pDst1 += dstStride;
        dcValN.store(pDst1);
        pDst1 += dstStride;
        dcValN.store(pDst1);
        pDst1 += dstStride;
        break;

    case 16:
        for (k = 0; k < 16; k += 2)
        {
            dcValN.store(pDst1);
            dcValN.store(pDst1 + 8);
            pDst1 += dstStride;
            dcValN.store(pDst1);
            dcValN.store(pDst1 + 8);
            pDst1 += dstStride;
        }

        break;

    case 32:
        for (k = 0; k < 32; k++)
        {
            dcValN.store(pDst1);
            dcValN.store(pDst1 +  8);
            dcValN.store(pDst1 + 16);
            dcValN.store(pDst1 + 24);
            pDst1 += dstStride;
        }

        break;

    //case 64:
    default:
        for (k = 0; k < 64; k++)
        {
            dcValN.store(pDst1);
            dcValN.store(pDst1 +  8);
            dcValN.store(pDst1 + 16);
            dcValN.store(pDst1 + 24);
            dcValN.store(pDst1 + 32);
            dcValN.store(pDst1 + 40);
            dcValN.store(pDst1 + 48);
            dcValN.store(pDst1 + 56);
            pDst1 += dstStride;
        }

        break;
    }

    if (bFilter && blkAboveAvailable && blkLeftAvailable)
    {
        xDCPredFiltering(pSrc, srcStride, pDst, dstStride, width, width);
    }
#else // if !HIGH_BIT_DEPTH

    if (blkAboveAvailable)
    {
        Vec16uc pix;
        Vec8us  im;
        Vec4ui  im1, im2;

        switch (width)
        {
        case 4:
            pix.fromUint32(*(uint32_t*)pSrcAbove);
            iSum += horizontal_add(extend_low(pix));
            break;
        case 8:
#if X86_64
            pix.fromUint64(*(uint64_t*)pSrcAbove);
#else
            pix.load_partial(8, pSrcAbove);
#endif
            iSum += horizontal_add(extend_low(pix));
            break;
        case 16:
            pix.load(pSrcAbove);
            iSum += horizontal_add_x(pix);
            break;
        case 32:
            pix.load(pSrcAbove);
            im1 = (Vec4ui)(pix.sad(_mm_setzero_si128()));
            pix.load(pSrcAbove + 16);
            im1 += (Vec4ui)(pix.sad(_mm_setzero_si128()));
            im1 += (Vec4ui)((Vec128b)im1 >> const_int(64));
            iSum += toInt32(im1);
            break;
        //case 64:
        default:
            pix.load(pSrcAbove);
            im1 = (Vec4ui)(pix.sad(_mm_setzero_si128()));
            pix.load(pSrcAbove + 16);
            im1 += (Vec4ui)(pix.sad(_mm_setzero_si128()));
            pix.load(pSrcAbove + 32);
            im1 += (Vec4ui)(pix.sad(_mm_setzero_si128()));
            pix.load(pSrcAbove + 48);
            im1 += (Vec4ui)(pix.sad(_mm_setzero_si128()));
            im1 += (Vec4ui)((Vec128b)im1 >> const_int(64));
            //im1 += extract_hi64(im1);
            iSum += toInt32(im1);
            break;
        }
    }

    logSize += (blkAboveAvailable + blkLeftAvailable - 1);
    pixel dcVal = (iSum + (1 << (logSize - 1))) >> logSize;
    Vec16uc dcValN(dcVal);
    int k;

    pixel *pDst1 = pDst;
    switch (width)
    {
    case 4:
        store_partial(const_int(4), pDst1, dcValN);
        pDst1 += dstStride;
        store_partial(const_int(4), pDst1, dcValN);
        pDst1 += dstStride;
        store_partial(const_int(4), pDst1, dcValN);
        pDst1 += dstStride;
        store_partial(const_int(4), pDst1, dcValN);
        break;

    case 8:
        store_partial(const_int(8), pDst1, dcValN);
        pDst1 += dstStride;
        store_partial(const_int(8), pDst1, dcValN);
        pDst1 += dstStride;
        store_partial(const_int(8), pDst1, dcValN);
        pDst1 += dstStride;
        store_partial(const_int(8), pDst1, dcValN);
        pDst1 += dstStride;
        store_partial(const_int(8), pDst1, dcValN);
        pDst1 += dstStride;
        store_partial(const_int(8), pDst1, dcValN);
        pDst1 += dstStride;
        store_partial(const_int(8), pDst1, dcValN);
        pDst1 += dstStride;
        store_partial(const_int(8), pDst1, dcValN);
        break;

    case 16:
        for (k = 0; k < 16; k += 4)
        {
            store_partial(const_int(16), pDst1, dcValN);
            pDst1 += dstStride;
            store_partial(const_int(16), pDst1, dcValN);
            pDst1 += dstStride;
            store_partial(const_int(16), pDst1, dcValN);
            pDst1 += dstStride;
            store_partial(const_int(16), pDst1, dcValN);
            pDst1 += dstStride;
        }

        break;

    case 32:
        for (k = 0; k < 32; k += 2)
        {
            store_partial(const_int(16), pDst1,    dcValN);
            store_partial(const_int(16), pDst1 + 16, dcValN);
            pDst1 += dstStride;
            store_partial(const_int(16), pDst1,    dcValN);
            store_partial(const_int(16), pDst1 + 16, dcValN);
            pDst1 += dstStride;
        }

        break;

    case 64:
        for (k = 0; k < 64; k++)
        {
            store_partial(const_int(16), pDst1,    dcValN);
            store_partial(const_int(16), pDst1 + 16, dcValN);
            store_partial(const_int(16), pDst1 + 32, dcValN);
            store_partial(const_int(16), pDst1 + 48, dcValN);
            pDst1 += dstStride;
        }

        break;
    }

    if (bFilter && blkAboveAvailable && blkLeftAvailable)
    {
        xDCPredFiltering(pSrc, srcStride, pDst, dstStride, width, width);
    }
#endif // if HIGH_BIT_DEPTH
}

#if HIGH_BIT_DEPTH
// CHECK_ME: I am not sure the v_rightColumnN will be overflow when input as 12bpp
void predIntraPlanar4(pixel* pSrc, intptr_t srcStride, pixel* rpDst, intptr_t dstStride)
{
    int k, bottomLeft, topRight;
    // NOTE: I use 16-bits is enough here, because we have least than 13-bits as input, and shift left by 2, it is 15-bits
    int16_t leftColumn[4];

    // Get left and above reference column and row
    Vec8s v_topRow = (Vec8s)load_partial(const_int(8), &pSrc[-srcStride]); // topRow

    for (k = 0; k < 4; k++)
    {
        leftColumn[k] = pSrc[k * srcStride - 1];
    }

    Vec8s v_leftColumn = (Vec8s)load_partial(const_int(8), leftColumn);   // leftColumn

    // Prepare intermediate variables used in interpolation
    bottomLeft = pSrc[4 * srcStride - 1];
    topRight   = pSrc[4 - srcStride];

    Vec8s v_bottomLeft(bottomLeft);
    Vec8s v_topRight(topRight);

    Vec8s v_bottomRow = v_bottomLeft - v_topRow;
    Vec8s v_rightColumn = v_topRight - v_leftColumn;

    v_topRow = v_topRow << const_int(2);
    v_leftColumn = v_leftColumn << const_int(2);

    // Generate prediction signal
    Vec8s v_horPred4 = v_leftColumn + Vec8s(4);
    const Vec8s v_multi(1, 2, 3, 4, 5, 6, 7, 8);
    Vec8s v_horPred, v_rightColumnN;
    Vec8s v_im4;
    Vec16uc v_im5;

    // line0
    v_horPred = broadcast(const_int(0), v_horPred4);
    v_rightColumnN = broadcast(const_int(0), v_rightColumn) * v_multi;
    v_horPred = v_horPred + v_rightColumnN;
    v_topRow = v_topRow + v_bottomRow;
    // CHECK_ME: the HM don't clip the pixel, so I assume there is biggest 12+3=15(bits)
    v_im4 = (Vec8s)(v_horPred + v_topRow) >> const_int(3);
    store_partial(const_int(8), &rpDst[0 * dstStride], v_im4);

    // line1
    v_horPred = broadcast(const_int(1), v_horPred4);
    v_rightColumnN = broadcast(const_int(1), v_rightColumn) * v_multi;
    v_horPred = v_horPred + v_rightColumnN;
    v_topRow = v_topRow + v_bottomRow;
    v_im4 = (Vec8s)(v_horPred + v_topRow) >> const_int(3);
    store_partial(const_int(8), &rpDst[1 * dstStride], v_im4);

    // line2
    v_horPred = broadcast(const_int(2), v_horPred4);
    v_rightColumnN = broadcast(const_int(2), v_rightColumn) * v_multi;
    v_horPred = v_horPred + v_rightColumnN;
    v_topRow = v_topRow + v_bottomRow;
    v_im4 = (Vec8s)(v_horPred + v_topRow) >> const_int(3);
    store_partial(const_int(8), &rpDst[2 * dstStride], v_im4);

    // line3
    v_horPred = broadcast(const_int(3), v_horPred4);
    v_rightColumnN = broadcast(const_int(3), v_rightColumn) * v_multi;
    v_horPred = v_horPred + v_rightColumnN;
    v_topRow = v_topRow + v_bottomRow;
    v_im4 = (Vec8s)(v_horPred + v_topRow) >> const_int(3);
    store_partial(const_int(8), &rpDst[3 * dstStride], v_im4);
}

#else /* if HIGH_BIT_DEPTH */
void predIntraPlanar4(pixel* pSrc, intptr_t srcStride, pixel* rpDst, intptr_t dstStride)
{
    int k;
    pixel bottomLeft, topRight;

    // Get left and above reference column and row
    Vec16uc im0 = (Vec16uc)load_partial(const_int(4), &pSrc[-srcStride]); // topRow
    Vec8s v_topRow = extend_low(im0);

    int16_t leftColumn[4];
    for (k = 0; k < 4; k++)
    {
        leftColumn[k] = pSrc[k * srcStride - 1];
    }

    Vec8s v_leftColumn = (Vec8s)load_partial(const_int(8), (void*)leftColumn);   // leftColumn

    // Prepare intermediate variables used in interpolation
    bottomLeft = pSrc[4 * srcStride - 1];
    topRight   = pSrc[4 - srcStride];

    Vec8s v_bottomLeft(bottomLeft);
    Vec8s v_topRight(topRight);

    Vec8s v_bottomRow = v_bottomLeft - v_topRow;
    Vec8s v_rightColumn = v_topRight - v_leftColumn;

    v_topRow = v_topRow << const_int(2);
    v_leftColumn = v_leftColumn << const_int(2);

    Vec8s v_horPred4 = v_leftColumn + Vec8s(4);
    const Vec8s v_multi(1, 2, 3, 4, 5, 6, 7, 8);
    Vec8s v_horPred, v_rightColumnN;
    Vec8s v_im4;
    Vec16uc v_im5;

#define COMP_PRED_PLANAR4_ROW(X) { \
    v_horPred = broadcast(const_int((X)), v_horPred4); \
    v_rightColumnN = broadcast(const_int((X)), v_rightColumn) * v_multi; \
    v_horPred = v_horPred + v_rightColumnN; \
    v_topRow = v_topRow + v_bottomRow; \
    v_im4 = (Vec8s)(v_horPred + v_topRow) >> const_int(3); \
    v_im5 = compress_unsafe(v_im4, v_im4); \
    store_partial(const_int(4), &rpDst[(X) * dstStride], v_im5); \
}

    COMP_PRED_PLANAR4_ROW(0)
    COMP_PRED_PLANAR4_ROW(1)
    COMP_PRED_PLANAR4_ROW(2)
    COMP_PRED_PLANAR4_ROW(3)

#undef COMP_PRED_PLANAR4_ROW
}

#if INSTRSET >= 5
void predIntraPlanar4_sse4(pixel* pSrc, intptr_t srcStride, pixel* rpDst, intptr_t dstStride)
{
    pixel bottomLeft, topRight;

    // Get left and above reference column and row
    __m128i im0 = _mm_cvtsi32_si128(*(uint32_t*)&pSrc[-srcStride]); // topRow
    __m128i v_topRow = _mm_unpacklo_epi8(im0, _mm_setzero_si128());

    __m128i v_leftColumn = _mm_setzero_si128();
    v_leftColumn = _mm_insert_epi8(v_leftColumn, pSrc[0 * srcStride - 1], 0);
    v_leftColumn = _mm_insert_epi8(v_leftColumn, pSrc[1 * srcStride - 1], 1);
    v_leftColumn = _mm_insert_epi8(v_leftColumn, pSrc[2 * srcStride - 1], 2);
    v_leftColumn = _mm_insert_epi8(v_leftColumn, pSrc[3 * srcStride - 1], 3);
    v_leftColumn = _mm_unpacklo_epi8(v_leftColumn, _mm_setzero_si128());

    // Prepare intermediate variables used in interpolation
    bottomLeft = pSrc[4 * srcStride - 1];
    topRight   = pSrc[4 - srcStride];

    __m128i v_bottomLeft = _mm_set1_epi16(bottomLeft);
    __m128i v_topRight   = _mm_set1_epi16(topRight);

    __m128i v_bottomRow   = _mm_sub_epi16(v_bottomLeft, v_topRow);
    __m128i v_rightColumn = _mm_sub_epi16(v_topRight, v_leftColumn);

    v_topRow = _mm_slli_epi16(v_topRow, 2);
    v_leftColumn = _mm_slli_epi16(v_leftColumn, 2);

    __m128i v_horPred4 = _mm_add_epi16(v_leftColumn, _mm_set1_epi16(4));
    const __m128i v_multi = _mm_setr_epi16(1, 2, 3, 4, 5, 6, 7, 8);
    __m128i v_horPred, v_rightColumnN;
    __m128i v_im4;
    __m128i v_im5;

#define COMP_PRED_PLANAR4_ROW(X) { \
    v_horPred = _mm_shufflelo_epi16(v_horPred4, (X)*0x55); \
    v_rightColumnN = _mm_mullo_epi16(_mm_shufflelo_epi16(v_rightColumn, (X)*0x55), v_multi); \
    v_horPred = _mm_add_epi16(v_horPred, v_rightColumnN); \
    v_topRow = _mm_add_epi16(v_topRow, v_bottomRow); \
    v_im4 = _mm_srai_epi16(_mm_add_epi16(v_horPred, v_topRow), 3); \
    v_im5 = _mm_packus_epi16(v_im4, v_im4); \
    *(uint32_t*)&rpDst[(X) * dstStride] = _mm_cvtsi128_si32(v_im5); \
    /*rpDst[(X) * dstStride] = _mm_extract_epi32(v_im5, (X));*/ \
}

    COMP_PRED_PLANAR4_ROW(0)
    COMP_PRED_PLANAR4_ROW(1)
    COMP_PRED_PLANAR4_ROW(2)
    COMP_PRED_PLANAR4_ROW(3)

#undef COMP_PRED_PLANAR4_ROW
}
#endif // INSTRSET >= 5

#endif /* if HIGH_BIT_DEPTH */

#if HIGH_BIT_DEPTH

#define COMP_PRED_PLANAR_ROW(X) { \
        v_horPred = permute8s<X, X, X, X, X, X, X, X>(v_horPred4); \
        v_rightColumnN = permute8s<X, X, X, X, X, X, X, X>(v_rightColumn) * v_multi; \
        v_horPred = v_horPred + v_rightColumnN; \
        v_topRow = v_topRow + v_bottomRow; \
        v_im4 = (Vec8s)(v_horPred + v_topRow) >> (3 + shift); \
        store_partial(const_int(16), &rpDst[X * dstStride], v_im4); \
}

void predIntraPlanar8(pixel* pSrc, intptr_t srcStride, pixel* rpDst, intptr_t dstStride)
{
    int k, bottomLeft, topRight;

    int16_t leftColumn[8];

    // Get left and above reference column and row
    Vec8s v_topRow = (Vec8s)load_partial(const_int(16), &pSrc[-srcStride]); // topRow

    for (k = 0; k < 8; k++)
    {
        leftColumn[k] = pSrc[k * srcStride - 1];
    }

    Vec8s v_leftColumn = (Vec8s)load_partial(const_int(16), leftColumn);   // leftColumn

    // Prepare intermediate variables used in interpolation
    bottomLeft = pSrc[8 * srcStride - 1];
    topRight   = pSrc[8 - srcStride];

    Vec8s v_bottomLeft(bottomLeft);
    Vec8s v_topRight(topRight);

    Vec8s v_bottomRow = v_bottomLeft - v_topRow;
    Vec8s v_rightColumn = v_topRight - v_leftColumn;

    int shift = g_aucConvertToBit[8];          // Using value corresponding to width = 8
    v_topRow = v_topRow << (2 + shift);
    v_leftColumn = v_leftColumn << (2 + shift);

    // Generate prediction signal
    Vec8s v_horPred4 = v_leftColumn + Vec8s(8);
    const Vec8s v_multi(1, 2, 3, 4, 5, 6, 7, 8);
    Vec8s v_horPred, v_rightColumnN;
    Vec8s v_im4;
    Vec16uc v_im5;

    COMP_PRED_PLANAR_ROW(0);     // row 0
    COMP_PRED_PLANAR_ROW(1);
    COMP_PRED_PLANAR_ROW(2);
    COMP_PRED_PLANAR_ROW(3);
    COMP_PRED_PLANAR_ROW(4);
    COMP_PRED_PLANAR_ROW(5);
    COMP_PRED_PLANAR_ROW(6);
    COMP_PRED_PLANAR_ROW(7);     // row 7
}

#undef COMP_PRED_PLANAR_ROW
#else /* if HIGH_BIT_DEPTH */

#define COMP_PRED_PLANAR_ROW(X) { \
        v_horPred = permute8s<X, X, X, X, X, X, X, X>(v_horPred4); \
        v_rightColumnN = permute8s<X, X, X, X, X, X, X, X>(v_rightColumn) * v_multi; \
        v_horPred = v_horPred + v_rightColumnN; \
        v_topRow = v_topRow + v_bottomRow; \
        v_im4 = (Vec8s)(v_horPred + v_topRow) >> (3 + shift); \
        v_im5 = compress(v_im4, v_im4); \
        store_partial(const_int(8), &rpDst[X * dstStride], v_im5); \
}

void predIntraPlanar8(pixel* pSrc, intptr_t srcStride, pixel* rpDst, intptr_t dstStride)
{
    int k;
    pixel bottomLeft, topRight;
    int16_t leftColumn[8];

    // Get left and above reference column and row
    Vec16uc im0 = (Vec16uc)load_partial(const_int(8), &pSrc[-srcStride]); // topRow
    Vec8s v_topRow = extend_low(im0);

    for (k = 0; k < 8; k++)
    {
        leftColumn[k] = pSrc[k * srcStride - 1];
    }

    Vec8s v_leftColumn;
    v_leftColumn.load(leftColumn);   // leftColumn

    // Prepare intermediate variables used in interpolation
    bottomLeft = pSrc[8 * srcStride - 1];
    topRight   = pSrc[8 - srcStride];

    Vec8s v_bottomLeft(bottomLeft);
    Vec8s v_topRight(topRight);

    Vec8s v_bottomRow = v_bottomLeft - v_topRow;
    Vec8s v_rightColumn = v_topRight - v_leftColumn;

    int shift = g_aucConvertToBit[8];         // Using value corresponding to width = 8
    v_topRow = v_topRow << (2 + shift);
    v_leftColumn = v_leftColumn << (2 + shift);

    Vec8s v_horPred4 = v_leftColumn + Vec8s(8);
    const Vec8s v_multi(1, 2, 3, 4, 5, 6, 7, 8);
    Vec8s v_horPred, v_rightColumnN;
    Vec8s v_im4;
    Vec16uc v_im5;

    COMP_PRED_PLANAR_ROW(0);     // row 0
    COMP_PRED_PLANAR_ROW(1);
    COMP_PRED_PLANAR_ROW(2);
    COMP_PRED_PLANAR_ROW(3);
    COMP_PRED_PLANAR_ROW(4);
    COMP_PRED_PLANAR_ROW(5);
    COMP_PRED_PLANAR_ROW(6);
    COMP_PRED_PLANAR_ROW(7);     // row 7
}

#undef COMP_PRED_PLANAR_ROW

#if INSTRSET >= 5
const __m128i v_multiL = _mm_setr_epi16(1, 2, 3, 4, 5, 6, 7, 8);
void predIntraPlanar8_sse4(pixel* pSrc, intptr_t srcStride, pixel* pDst, intptr_t dstStride)
{
    pixel bottomLeft, topRight;

    // Get left and above reference column and row
    __m128i im0 = _mm_loadl_epi64((__m128i*)&pSrc[0-srcStride]); // topRow
    __m128i v_topRow = _mm_unpacklo_epi8(im0, _mm_setzero_si128());

    __m128i v_leftColumn = _mm_setzero_si128();
    v_leftColumn = _mm_insert_epi8(v_leftColumn, pSrc[0 * srcStride - 1], 0);
    v_leftColumn = _mm_insert_epi8(v_leftColumn, pSrc[1 * srcStride - 1], 1);
    v_leftColumn = _mm_insert_epi8(v_leftColumn, pSrc[2 * srcStride - 1], 2);
    v_leftColumn = _mm_insert_epi8(v_leftColumn, pSrc[3 * srcStride - 1], 3);
    v_leftColumn = _mm_insert_epi8(v_leftColumn, pSrc[4 * srcStride - 1], 4);
    v_leftColumn = _mm_insert_epi8(v_leftColumn, pSrc[5 * srcStride - 1], 5);
    v_leftColumn = _mm_insert_epi8(v_leftColumn, pSrc[6 * srcStride - 1], 6);
    v_leftColumn = _mm_insert_epi8(v_leftColumn, pSrc[7 * srcStride - 1], 7);
    v_leftColumn = _mm_unpacklo_epi8(v_leftColumn, _mm_setzero_si128());

    // Prepare intermediate variables used in interpolation
    bottomLeft = pSrc[8 * srcStride - 1];
    topRight   = pSrc[8 - srcStride];

    __m128i v_bottomLeft = _mm_set1_epi16(bottomLeft);
    __m128i v_topRight   = _mm_set1_epi16(topRight);

    __m128i v_bottomRow   = _mm_sub_epi16(v_bottomLeft, v_topRow);
    __m128i v_rightColumn = _mm_sub_epi16(v_topRight, v_leftColumn);

    v_topRow = _mm_slli_epi16(v_topRow, 3);
    v_leftColumn = _mm_slli_epi16(v_leftColumn, 3);

    __m128i v_horPred4 = _mm_add_epi16(v_leftColumn, _mm_set1_epi16(8));
    __m128i v_horPred, v_rightColumnN;
    __m128i v_im4;
    __m128i v_im5;

#define COMP_PRED_PLANAR_ROW(Y) { \
    if ((Y) < 4) { \
        v_horPred = _mm_shufflelo_epi16(v_horPred4, ((Y)&3)*0x55); \
        v_horPred = _mm_unpacklo_epi64(v_horPred, v_horPred); \
        v_rightColumnN = _mm_shufflelo_epi16(v_rightColumn, ((Y)&3)*0x55); \
        v_rightColumnN = _mm_unpacklo_epi64(v_rightColumnN, v_rightColumnN); \
    } \
    else { \
        v_horPred = _mm_shufflehi_epi16(v_horPred4, ((Y)&3)*0x55); \
        v_horPred = _mm_unpackhi_epi64(v_horPred, v_horPred); \
        v_rightColumnN = _mm_shufflehi_epi16(v_rightColumn, ((Y)&3)*0x55); \
        v_rightColumnN = _mm_unpackhi_epi64(v_rightColumnN, v_rightColumnN); \
    } \
    v_rightColumnN = _mm_mullo_epi16(v_rightColumnN, v_multiL); \
    v_horPred = _mm_add_epi16(v_horPred, v_rightColumnN); \
    v_topRow = _mm_add_epi16(v_topRow, v_bottomRow); \
    v_im4 = _mm_srai_epi16(_mm_add_epi16(v_horPred, v_topRow), 4); \
    v_im5 = _mm_packus_epi16(v_im4, v_im4); \
    _mm_storel_epi64((__m128i*)&pDst[(Y) * dstStride], v_im5); \
}

    COMP_PRED_PLANAR_ROW(0)
    COMP_PRED_PLANAR_ROW(1)
    COMP_PRED_PLANAR_ROW(2)
    COMP_PRED_PLANAR_ROW(3)
    COMP_PRED_PLANAR_ROW(4)
    COMP_PRED_PLANAR_ROW(5)
    COMP_PRED_PLANAR_ROW(6)
    COMP_PRED_PLANAR_ROW(7)

#undef COMP_PRED_PLANAR_ROW
}
#endif // INSTRSET >= 5

#endif /* if HIGH_BIT_DEPTH */

#if HIGH_BIT_DEPTH
#define COMP_PRED_PLANAR_ROW(X) { \
        v_horPred_lo = permute8s<X, X, X, X, X, X, X, X>(v_horPred4); \
        v_horPred_hi = v_horPred_lo; \
        v_rightColumnN_lo = permute8s<X, X, X, X, X, X, X, X>(v_rightColumn); \
        v_rightColumnN_hi = v_rightColumnN_lo; \
        v_rightColumnN_lo *= v_multi_lo; \
        v_rightColumnN_hi *= v_multi_hi; \
        v_horPred_lo = v_horPred_lo + v_rightColumnN_lo; \
        v_horPred_hi = v_horPred_hi + v_rightColumnN_hi; \
        v_topRow_lo = v_topRow_lo + v_bottomRow_lo; \
        v_topRow_hi = v_topRow_hi + v_bottomRow_hi; \
        v_im4_lo = (Vec8s)(v_horPred_lo + v_topRow_lo) >> (3 + shift); \
        v_im4_hi = (Vec8s)(v_horPred_hi + v_topRow_hi) >> (3 + shift); \
        v_im4_lo.store(&rpDst[X * dstStride]); \
        v_im4_hi.store(&rpDst[X * dstStride + 8]); \
}

void predIntraPlanar16(pixel* pSrc, intptr_t srcStride, pixel* rpDst, intptr_t dstStride)
{
    int k;
    pixel bottomLeft, topRight;
    int16_t leftColumn[16];

    // Get left and above reference column and row
    Vec8s v_topRow_lo, v_topRow_hi;

    v_topRow_lo.load(&pSrc[-srcStride]);
    v_topRow_hi.load(&pSrc[-srcStride + 8]);

    for (k = 0; k < 16; k++)
    {
        leftColumn[k] = pSrc[k * srcStride - 1];
    }

    Vec8s v_leftColumn;
    v_leftColumn.load(leftColumn);   // leftColumn

    // Prepare intermediate variables used in interpolation
    bottomLeft = pSrc[16 * srcStride - 1];
    topRight   = pSrc[16 - srcStride];

    Vec8s v_bottomLeft(bottomLeft);
    Vec8s v_topRight(topRight);

    Vec8s v_bottomRow_lo = v_bottomLeft - v_topRow_lo;
    Vec8s v_bottomRow_hi = v_bottomLeft - v_topRow_hi;
    Vec8s v_rightColumn = v_topRight - v_leftColumn;

    int shift = g_aucConvertToBit[16];         // Using value corresponding to width = 8
    v_topRow_lo = v_topRow_lo << (2 + shift);
    v_topRow_hi = v_topRow_hi << (2 + shift);
    v_leftColumn = v_leftColumn << (2 + shift);

    Vec8s v_horPred4 = v_leftColumn + Vec8s(16);
    const Vec8s v_multi_lo(1, 2, 3, 4, 5, 6, 7, 8);
    const Vec8s v_multi_hi(9, 10, 11, 12, 13, 14, 15, 16);
    Vec8s v_horPred_lo, v_horPred_hi, v_rightColumnN_lo, v_rightColumnN_hi;
    Vec8s v_im4_lo, v_im4_hi;
    Vec16uc v_im5;

    COMP_PRED_PLANAR_ROW(0);     // row 0
    COMP_PRED_PLANAR_ROW(1);
    COMP_PRED_PLANAR_ROW(2);
    COMP_PRED_PLANAR_ROW(3);
    COMP_PRED_PLANAR_ROW(4);
    COMP_PRED_PLANAR_ROW(5);
    COMP_PRED_PLANAR_ROW(6);
    COMP_PRED_PLANAR_ROW(7);     // row 7

    v_leftColumn.load(leftColumn + 8);   // leftColumn lower 8 rows
    v_rightColumn = v_topRight - v_leftColumn;
    v_leftColumn = v_leftColumn << (2 + shift);
    v_horPred4 = v_leftColumn + Vec8s(16);

    COMP_PRED_PLANAR_ROW(8);     // row 0
    COMP_PRED_PLANAR_ROW(9);
    COMP_PRED_PLANAR_ROW(10);
    COMP_PRED_PLANAR_ROW(11);
    COMP_PRED_PLANAR_ROW(12);
    COMP_PRED_PLANAR_ROW(13);
    COMP_PRED_PLANAR_ROW(14);
    COMP_PRED_PLANAR_ROW(15);
}

#undef COMP_PRED_PLANAR_ROW

#else /* if HIGH_BIT_DEPTH */
#define COMP_PRED_PLANAR_ROW(X) { \
        v_horPred_lo = permute8s<X, X, X, X, X, X, X, X>(v_horPred4); \
        v_horPred_hi = v_horPred_lo; \
        v_rightColumnN_lo = permute8s<X, X, X, X, X, X, X, X>(v_rightColumn); \
        v_rightColumnN_hi = v_rightColumnN_lo; \
        v_rightColumnN_lo *= v_multi_lo; \
        v_rightColumnN_hi *= v_multi_hi; \
        v_horPred_lo = v_horPred_lo + v_rightColumnN_lo; \
        v_horPred_hi = v_horPred_hi + v_rightColumnN_hi; \
        v_topRow_lo = v_topRow_lo + v_bottomRow_lo; \
        v_topRow_hi = v_topRow_hi + v_bottomRow_hi; \
        v_im4_lo = (Vec8s)(v_horPred_lo + v_topRow_lo) >> (3 + shift); \
        v_im4_hi = (Vec8s)(v_horPred_hi + v_topRow_hi) >> (3 + shift); \
        v_im5 = compress(v_im4_lo, v_im4_hi); \
        store_partial(const_int(16), &rpDst[X * dstStride], v_im5); \
}

void predIntraPlanar16(pixel* pSrc, intptr_t srcStride, pixel* rpDst, intptr_t dstStride)
{
    int k;
    pixel bottomLeft, topRight;
    int16_t leftColumn[16];

    // Get left and above reference column and row
    Vec16uc im0 = (Vec16uc)load_partial(const_int(16), &pSrc[-srcStride]); // topRow
    Vec8s v_topRow_lo = extend_low(im0);
    Vec8s v_topRow_hi = extend_high(im0);

    for (k = 0; k < 16; k++)
    {
        leftColumn[k] = pSrc[k * srcStride - 1];
    }

    Vec8s v_leftColumn;
    v_leftColumn.load(leftColumn);   // leftColumn

    // Prepare intermediate variables used in interpolation
    bottomLeft = pSrc[16 * srcStride - 1];
    topRight   = pSrc[16 - srcStride];

    Vec8s v_bottomLeft(bottomLeft);
    Vec8s v_topRight(topRight);

    Vec8s v_bottomRow_lo = v_bottomLeft - v_topRow_lo;
    Vec8s v_bottomRow_hi = v_bottomLeft - v_topRow_hi;
    Vec8s v_rightColumn = v_topRight - v_leftColumn;

    int shift = g_aucConvertToBit[16];         // Using value corresponding to width = 8
    v_topRow_lo = v_topRow_lo << (2 + shift);
    v_topRow_hi = v_topRow_hi << (2 + shift);
    v_leftColumn = v_leftColumn << (2 + shift);

    Vec8s v_horPred4 = v_leftColumn + Vec8s(16);
    const Vec8s v_multi_lo(1, 2, 3, 4, 5, 6, 7, 8);
    const Vec8s v_multi_hi(9, 10, 11, 12, 13, 14, 15, 16);
    Vec8s v_horPred_lo, v_horPred_hi, v_rightColumnN_lo, v_rightColumnN_hi;
    Vec8s v_im4_lo, v_im4_hi;
    Vec16uc v_im5;

    COMP_PRED_PLANAR_ROW(0);     // row 0
    COMP_PRED_PLANAR_ROW(1);
    COMP_PRED_PLANAR_ROW(2);
    COMP_PRED_PLANAR_ROW(3);
    COMP_PRED_PLANAR_ROW(4);
    COMP_PRED_PLANAR_ROW(5);
    COMP_PRED_PLANAR_ROW(6);
    COMP_PRED_PLANAR_ROW(7);     // row 7

    v_leftColumn.load(leftColumn + 8);   // leftColumn lower 8 rows
    v_rightColumn = v_topRight - v_leftColumn;
    v_leftColumn = v_leftColumn << (2 + shift);
    v_horPred4 = v_leftColumn + Vec8s(16);

    COMP_PRED_PLANAR_ROW(8);     // row 0
    COMP_PRED_PLANAR_ROW(9);
    COMP_PRED_PLANAR_ROW(10);
    COMP_PRED_PLANAR_ROW(11);
    COMP_PRED_PLANAR_ROW(12);
    COMP_PRED_PLANAR_ROW(13);
    COMP_PRED_PLANAR_ROW(14);
    COMP_PRED_PLANAR_ROW(15);
}
#undef COMP_PRED_PLANAR_ROW

#if INSTRSET >= 5
void predIntraPlanar16_sse4(pixel* pSrc, intptr_t srcStride, pixel* rpDst, intptr_t dstStride)
{
    pixel bottomLeft, topRight;

    // Get left and above reference column and row
    __m128i im0 = _mm_cvtsi32_si128(*(uint32_t*)&pSrc[-srcStride]); // topRow
    __m128i v_topRowL = _mm_unpacklo_epi8(im0, _mm_setzero_si128());
    __m128i v_topRowH = _mm_unpackhi_epi8(im0, _mm_setzero_si128());

    __m128i v_leftColumnL = _mm_setzero_si128();
    v_leftColumnL = _mm_insert_epi8(v_leftColumnL, pSrc[ 0 * srcStride - 1],  0);
    v_leftColumnL = _mm_insert_epi8(v_leftColumnL, pSrc[ 1 * srcStride - 1],  1);
    v_leftColumnL = _mm_insert_epi8(v_leftColumnL, pSrc[ 2 * srcStride - 1],  2);
    v_leftColumnL = _mm_insert_epi8(v_leftColumnL, pSrc[ 3 * srcStride - 1],  3);
    v_leftColumnL = _mm_insert_epi8(v_leftColumnL, pSrc[ 4 * srcStride - 1],  4);
    v_leftColumnL = _mm_insert_epi8(v_leftColumnL, pSrc[ 5 * srcStride - 1],  5);
    v_leftColumnL = _mm_insert_epi8(v_leftColumnL, pSrc[ 6 * srcStride - 1],  6);
    v_leftColumnL = _mm_insert_epi8(v_leftColumnL, pSrc[ 7 * srcStride - 1],  7);
    v_leftColumnL = _mm_unpacklo_epi8(v_leftColumnL, _mm_setzero_si128());

    __m128i v_leftColumnH = _mm_setzero_si128();
    v_leftColumnH = _mm_insert_epi8(v_leftColumnH, pSrc[ 8 * srcStride - 1],  8);
    v_leftColumnH = _mm_insert_epi8(v_leftColumnH, pSrc[ 9 * srcStride - 1],  9);
    v_leftColumnH = _mm_insert_epi8(v_leftColumnH, pSrc[10 * srcStride - 1], 10);
    v_leftColumnH = _mm_insert_epi8(v_leftColumnH, pSrc[11 * srcStride - 1], 11);
    v_leftColumnH = _mm_insert_epi8(v_leftColumnH, pSrc[12 * srcStride - 1], 12);
    v_leftColumnH = _mm_insert_epi8(v_leftColumnH, pSrc[13 * srcStride - 1], 13);
    v_leftColumnH = _mm_insert_epi8(v_leftColumnH, pSrc[14 * srcStride - 1], 14);
    v_leftColumnH = _mm_insert_epi8(v_leftColumnH, pSrc[15 * srcStride - 1], 15);
    v_leftColumnH = _mm_unpacklo_epi8(v_leftColumnH, _mm_setzero_si128());
    v_leftColumnH = _mm_slli_epi16(v_leftColumnH, 4);

    // Prepare intermediate variables used in interpolation
    bottomLeft = pSrc[16 * srcStride - 1];
    topRight   = pSrc[16 - srcStride];

    __m128i v_bottomLeft = _mm_set1_epi16(bottomLeft);
    __m128i v_topRight   = _mm_set1_epi16(topRight);

    __m128i v_bottomRowL  = _mm_sub_epi16(v_bottomLeft, v_topRowL);
    __m128i v_bottomRowH  = _mm_sub_epi16(v_bottomLeft, v_topRowH);
    __m128i v_rightColumn = _mm_sub_epi16(v_topRight, v_leftColumnL);

    v_topRowL = _mm_slli_epi16(v_topRowL, 4);
    v_topRowH = _mm_slli_epi16(v_topRowH, 4);
    v_leftColumnL = _mm_slli_epi16(v_leftColumnL, 4);

    __m128i v_horPred4 = _mm_add_epi16(v_leftColumnL, _mm_set1_epi16(16));
    const __m128i v_multiA = _mm_setr_epi16(1, 2, 3, 4, 5, 6, 7, 8);
    const __m128i v_multiB = _mm_setr_epi16(9,10,11,12,13,14,15,16);
    __m128i v_horPred, v_rightColumnN;
    __m128i v_horPredL, v_horPredH;
    __m128i v_rightColumnNL, v_rightColumnNH;
    __m128i v_im4L, v_im4H;
    __m128i v_im5;

#define COMP_PRED_PLANAR_ROW(X) { \
    if ((X) < 4) { \
        v_horPred = _mm_shufflelo_epi16(v_horPred4, ((X)&3)*0x55); \
        v_horPred = _mm_unpacklo_epi64(v_horPred, v_horPred); \
        v_rightColumnN = _mm_shufflelo_epi16(v_rightColumn, ((X)&3)*0x55); \
        v_rightColumnN = _mm_unpacklo_epi64(v_rightColumnN, v_rightColumnN); \
    } \
    else { \
        v_horPred = _mm_shufflehi_epi16(v_horPred4, ((X)&3)*0x55); \
        v_horPred = _mm_unpackhi_epi64(v_horPred, v_horPred); \
        v_rightColumnN = _mm_shufflehi_epi16(v_rightColumn, ((X)&3)*0x55); \
        v_rightColumnN = _mm_unpackhi_epi64(v_rightColumnN, v_rightColumnN); \
    } \
    v_rightColumnNL = _mm_mullo_epi16(v_rightColumnN, v_multiA); \
    v_rightColumnNH = _mm_mullo_epi16(v_rightColumnN, v_multiB); \
    v_horPredL = _mm_add_epi16(v_horPred, v_rightColumnNL); \
    v_horPredH = _mm_add_epi16(v_horPred, v_rightColumnNH); \
    v_topRowL = _mm_add_epi16(v_topRowL, v_bottomRowL); \
    v_topRowH = _mm_add_epi16(v_topRowH, v_bottomRowH); \
    v_im4L = _mm_srai_epi16(_mm_add_epi16(v_horPredL, v_topRowL), 5); \
    v_im4H = _mm_srai_epi16(_mm_add_epi16(v_horPredH, v_topRowH), 5); \
    v_im5 = _mm_packus_epi16(v_im4L, v_im4H); \
    _mm_storeu_si128((__m128i*)&rpDst[(X) * dstStride], v_im5); \
}

    COMP_PRED_PLANAR_ROW( 0)
    COMP_PRED_PLANAR_ROW( 1)
    COMP_PRED_PLANAR_ROW( 2)
    COMP_PRED_PLANAR_ROW( 3)
    COMP_PRED_PLANAR_ROW( 4)
    COMP_PRED_PLANAR_ROW( 5)
    COMP_PRED_PLANAR_ROW( 6)
    COMP_PRED_PLANAR_ROW( 7)

    v_horPred4      = _mm_add_epi16(v_leftColumnH, _mm_set1_epi16(16));
    v_rightColumn   = _mm_sub_epi16(v_topRight, v_leftColumnH);

    COMP_PRED_PLANAR_ROW( 8)
    COMP_PRED_PLANAR_ROW( 9)
    COMP_PRED_PLANAR_ROW(10)
    COMP_PRED_PLANAR_ROW(11)
    COMP_PRED_PLANAR_ROW(12)
    COMP_PRED_PLANAR_ROW(13)
    COMP_PRED_PLANAR_ROW(14)
    COMP_PRED_PLANAR_ROW(15)

#undef COMP_PRED_PLANAR_ROW
}
#endif // INSTRSET >= 5

#endif /* if HIGH_BIT_DEPTH */

typedef void predIntraPlanar_t(pixel* pSrc, intptr_t srcStride, pixel* rpDst, intptr_t dstStride);
predIntraPlanar_t *intraPlanarN[] = {
#if !HIGH_BIT_DEPTH && INSTRSET >= 5
    predIntraPlanar4_sse4,
    predIntraPlanar8_sse4,
    predIntraPlanar16_sse4,
#else
    predIntraPlanar4,
    predIntraPlanar8,
    predIntraPlanar16,
#endif
};

void predIntraPlanar(pixel* pSrc, intptr_t srcStride, pixel* rpDst, intptr_t dstStride, int width, int /*height*/)
{
    //assert(width == height);

    int k, l, bottomLeft, topRight;
    int horPred;
    // OPT_ME: when width is 64, the shift1D is 8, then the dynamic range is [-65280, 65280], so we have to use 32 bits here
    int32_t leftColumn[MAX_CU_SIZE + 1], topRow[MAX_CU_SIZE + 1];
    // CHECK_ME: dynamic range is 9 bits or 15 bits(I assume max input bit_depth is 14 bits)
    int16_t bottomRow[MAX_CU_SIZE], rightColumn[MAX_CU_SIZE];
    int blkSize = width;
    int offset2D = width;
    int nLog2Size = g_aucConvertToBit[width] + 2;
    int shift1D = nLog2Size;
    int shift2D = shift1D + 1;

    if (width < 32)
    {
        intraPlanarN[nLog2Size-2](pSrc, srcStride, rpDst, dstStride);
        return;
    }

    // Get left and above reference column and row
    for (k = 0; k < blkSize + 1; k++)
    {
        topRow[k] = pSrc[k - srcStride];
        leftColumn[k] = pSrc[k * srcStride - 1];
    }

    // Prepare intermediate variables used in interpolation
    bottomLeft = leftColumn[blkSize];
    topRight   = topRow[blkSize];
    for (k = 0; k < blkSize; k++)
    {
        bottomRow[k]   = bottomLeft - topRow[k];
        rightColumn[k] = topRight   - leftColumn[k];
        topRow[k]      <<= shift1D;
        leftColumn[k]  <<= shift1D;
    }

    // Generate prediction signal
    for (k = 0; k < blkSize; k++)
    {
        horPred = leftColumn[k] + offset2D;
        for (l = 0; l < blkSize; l++)
        {
            horPred += rightColumn[k];
            topRow[l] += bottomRow[l];
            rpDst[k * dstStride + l] = ((horPred + topRow[l]) >> shift2D);
        }
    }
}

#if HIGH_BIT_DEPTH
void xPredIntraAng4x4(int bitDepth, pixel* pDst, int dstStride, int width, int dirMode, pixel *refLeft, pixel *refAbove)
{
    int blkSize        = width;

    // Map the mode index to main prediction direction and angle
    assert(dirMode > 1); //no planar and dc
    bool modeHor       = (dirMode < 18);
    bool modeVer       = !modeHor;
    int intraPredAngle = modeVer ? (int)dirMode - VER_IDX : modeHor ? -((int)dirMode - HOR_IDX) : 0;
    int lookIdx = intraPredAngle;
    int absAng         = abs(intraPredAngle);
    int signAng        = intraPredAngle < 0 ? -1 : 1;

    // Set bitshifts and scale the angle parameter to block size
    int angTable[9]    = { 0,    2,    5,   9,  13,  17,  21,  26,  32 };
    int invAngTable[9] = { 0, 4096, 1638, 910, 630, 482, 390, 315, 256 }; // (256 * 32) / Angle
    int invAngle       = invAngTable[absAng];
    absAng             = angTable[absAng];
    intraPredAngle     = signAng * absAng;

    // Do angular predictions

    pixel* refMain;
    pixel* refSide;

    // Initialise the Main and Left reference array.
    if (intraPredAngle < 0)
    {
        refMain = (modeVer ? refAbove : refLeft);     // + (blkSize - 1);
        refSide = (modeVer ? refLeft : refAbove);     // + (blkSize - 1);

        // Extend the Main reference to the left.
        int invAngleSum    = 128;     // rounding for (shift by 8)
        for (int k = -1; k > blkSize * intraPredAngle >> 5; k--)
        {
            invAngleSum += invAngle;
            refMain[k] = refSide[invAngleSum >> 8];
        }
    }
    else
    {
        refMain = modeVer ? refAbove : refLeft;
        refSide = modeVer ? refLeft  : refAbove;
    }

    // bfilter will always be true for blocksize 4
    if (intraPredAngle == 0)  // Exactly hotizontal/vertical angles
    {
        if (modeHor)
        {
            Vec8s v_temp;
            Vec8s v_side_0; // refSide[0] value in a vector
            v_temp.load((void*)refSide);
            v_side_0 = broadcast(const_int(0), (Vec8s)v_temp);

            Vec8s v_side;
            v_side.load(refSide + 1);

            Vec8s v_main;
            v_main = load_partial(const_int(8), (void*)(refMain + 1));

            Vec8s tmp1, tmp2;
            tmp1 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>(v_main, v_main);
            tmp2 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>(tmp1, tmp1);
            tmp1 = blend8s<4, 12, 5, 13, 6, 14, 7, 15>(tmp1, tmp1);

            Vec8s row0;
            v_side -= v_side_0;
            v_side = v_side >> 1;
            row0 = tmp2 + v_side;
            row0 = min(max(0, row0), (1 << bitDepth) - 1);

            store_partial(const_int(8), pDst, row0);                //row0
            store_partial(const_int(8), pDst + (2 * dstStride), tmp1); //row2

            tmp2 = blend8s<4, 12, 5, 13, 6, 14, 7, 15>(tmp2, tmp2);
            tmp1 = blend8s<4, 12, 5, 13, 6, 14, 7, 15>(tmp1, tmp1);

            store_partial(const_int(8), pDst + (3 * dstStride), tmp1); //row3
            store_partial(const_int(8), pDst + (dstStride), tmp2);    //row1
        }
        else
        {
            Vec16uc v_main;
            v_main = load_partial(const_int(8), refMain + 1);
            store_partial(const_int(8), pDst, v_main);
            store_partial(const_int(8), pDst + dstStride, v_main);
            store_partial(const_int(8), pDst + (2 * dstStride), v_main);
            store_partial(const_int(8), pDst + (3 * dstStride), v_main);

            for (int k = 0; k < 4; k++)
            {
                pDst[k * dstStride] = (pixel)Clip3((short)0, (short)((1 << bitDepth) - 1), static_cast<short>((pDst[k * dstStride]) + ((refSide[k + 1] - refSide[0]) >> 1)));
            }
        }
    }
    else if (intraPredAngle == -32)
    {
        Vec8s tmp;
        tmp = load_partial(const_int(8), refMain);        //-1,0,1,2
        store_partial(const_int(8), pDst, tmp);
        tmp = load_partial(const_int(8), refMain - 1);     //-2,-1,0,1
        store_partial(const_int(8), pDst + dstStride, tmp);
        tmp = load_partial(const_int(8), refMain - 2);
        store_partial(const_int(8), pDst + 2 * dstStride, tmp);
        tmp = load_partial(const_int(8), refMain - 3);
        store_partial(const_int(8), pDst + 3 * dstStride, tmp);
        return;
    }
    else if (intraPredAngle == 32)
    {
        Vec8s tmp;
        tmp = load_partial(const_int(8), refMain + 2);        //-1,0,1,2
        store_partial(const_int(8), pDst, tmp);
        tmp = load_partial(const_int(8), refMain + 3);     //-2,-1,0,1
        store_partial(const_int(8), pDst + dstStride, tmp);
        tmp = load_partial(const_int(8), refMain + 4);
        store_partial(const_int(8), pDst + 2 * dstStride, tmp);
        tmp = load_partial(const_int(8), refMain + 5);
        store_partial(const_int(8), pDst + 3 * dstStride, tmp);
        return;
    }
    else
    {
        Vec8s row11, row12, row21, row22, row31, row32, row41, row42;
        Vec8s v_deltaFract, v_deltaPos, thirty2(32), thirty1(31), v_ipAngle;

        row11 = (Vec8s)load_partial(const_int(8), refMain + 1 + GETAP(lookIdx, 0));
        row12 = (Vec8s)load_partial(const_int(8), refMain + 1 + GETAP(lookIdx, 0) + 1);

        row21 = (Vec8s)load_partial(const_int(8), refMain + 1 + GETAP(lookIdx, 1));
        row22 = (Vec8s)load_partial(const_int(8), refMain + 1 + GETAP(lookIdx, 1) + 1);

        row31 = (Vec8s)load_partial(const_int(8), refMain + 1 + GETAP(lookIdx, 2));
        row32 = (Vec8s)load_partial(const_int(8), refMain + 1 + GETAP(lookIdx, 2) + 1);

        row41 = (Vec8s)load_partial(const_int(8), refMain + 1 + GETAP(lookIdx, 3));
        row42 = (Vec8s)load_partial(const_int(8), refMain + 1 + GETAP(lookIdx, 3) + 1);

        v_deltaPos = v_ipAngle = intraPredAngle;

        //row1
        v_deltaFract = v_deltaPos & thirty1;
        row11 = ((thirty2 - v_deltaFract) * row11 + (v_deltaFract * row12) + 16) >> 5;

        //row2
        v_deltaPos += v_ipAngle;
        v_deltaFract = v_deltaPos & thirty1;
        row21 = ((thirty2 - v_deltaFract) * row21 + (v_deltaFract * row22) + 16) >> 5;

        //row3
        v_deltaPos += v_ipAngle;
        v_deltaFract = v_deltaPos & thirty1;
        row31 = ((thirty2 - v_deltaFract) * row31 + (v_deltaFract * row32) + 16) >> 5;

        //row4
        v_deltaPos += v_ipAngle;
        v_deltaFract = v_deltaPos & thirty1;
        row41 = ((thirty2 - v_deltaFract) * row41 + (v_deltaFract * row42) + 16) >> 5;

        // Flip the block

        if (modeHor)
        {
            Vec8s tmp1, tmp2, tmp3, tmp4;

            tmp1 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>(row11, row31);
            tmp2 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>(row21, row41);

            tmp3 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>(tmp1, tmp2);
            tmp4 = blend8s<4, 12, 5, 13, 6, 14, 7, 15>(tmp1, tmp2);

            //tmp16_1 = compress(tmp3, tmp3);
            store_partial(const_int(8), pDst, tmp3);

            store_partial(const_int(8), pDst + (2 * dstStride), tmp4);  //row2

            tmp3 = blend2q<1, 3>((Vec2q)tmp3, (Vec2q)tmp3);
            tmp4 = blend2q<1, 3>((Vec2q)tmp4, (Vec2q)tmp4);

            store_partial(const_int(8), pDst + (3 * dstStride), tmp4);   //row3
            store_partial(const_int(8), pDst + (dstStride), tmp3);       //row1
        }
        else
        {
            store_partial(const_int(8), pDst, row11);
            store_partial(const_int(8), pDst + (dstStride), row21);
            store_partial(const_int(8), pDst + (2 * dstStride), row31);
            store_partial(const_int(8), pDst + (3 * dstStride), row41);
        }
    }
}

#else /* if HIGH_BIT_DEPTH */
void xPredIntraAng4x4(int /*bitDepth*/, pixel* pDst, int dstStride, int width, int dirMode, pixel *refLeft, pixel *refAbove)
{
    int blkSize        = width;

    // Map the mode index to main prediction direction and angle
    assert(dirMode > 1); //no planar and dc
    bool modeHor       = (dirMode < 18);
    bool modeVer       = !modeHor;
    int intraPredAngle = modeVer ? (int)dirMode - VER_IDX : modeHor ? -((int)dirMode - HOR_IDX) : 0;
    int absAng         = abs(intraPredAngle);
    int signAng        = intraPredAngle < 0 ? -1 : 1;

    // Set bitshifts and scale the angle parameter to block size
    int angTable[9]    = { 0,    2,    5,   9,  13,  17,  21,  26,  32 };
    int invAngTable[9] = { 0, 4096, 1638, 910, 630, 482, 390, 315, 256 }; // (256 * 32) / Angle
    int invAngle       = invAngTable[absAng];
    absAng             = angTable[absAng];
    intraPredAngle     = signAng * absAng;

    // Do angular predictions

    pixel* refMain;
    pixel* refSide;

    // Initialise the Main and Left reference array.
    if (intraPredAngle < 0)
    {
        refMain = (modeVer ? refAbove : refLeft);     // + (blkSize - 1);
        refSide = (modeVer ? refLeft : refAbove);     // + (blkSize - 1);

        // Extend the Main reference to the left.
        int invAngleSum    = 128;     // rounding for (shift by 8)
        for (int k = -1; k > blkSize * intraPredAngle >> 5; k--)
        {
            invAngleSum += invAngle;
            refMain[k] = refSide[invAngleSum >> 8];
        }
    }
    else
    {
        refMain = modeVer ? refAbove : refLeft;
        refSide = modeVer ? refLeft  : refAbove;
    }

    // bfilter will always be true for exactly vertical/horizontal modes
    if (intraPredAngle == 0)  // Exactly hotizontal/vertical angles
    {
        if (modeHor)
        {
            Vec16uc v_temp;
            Vec8s v_side_0; // refSide[0] value in a vector
            v_temp = load_partial(const_int(8), (void*)refSide);
            v_side_0 = broadcast(const_int(0), (Vec8s)v_temp);
            v_side_0 = v_side_0 & 0x00FF;

            //shift v_side by 1 element (1 byte)
            Vec2uq tmp = reinterpret_i(v_temp);
            tmp = tmp >> 8;
            v_temp = reinterpret_i(tmp);
            Vec8s v_side = extend_low(v_temp);

            Vec16uc v_main;
            v_main = load_partial(const_int(4), (void*)(refMain + 1));

            Vec16uc tmp16;
            tmp16 = blend16c<0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23>(v_main, v_main);
            tmp16 = blend16c<0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23>(tmp16, tmp16);

            Vec8s row0 = extend_low(tmp16);
            v_side -= v_side_0;
            v_side = v_side >> 1;
            row0 += v_side;
            row0 = min(max(0, row0), 255);
            Vec16uc v_res(compress(row0, 0));
            store_partial(const_int(4), pDst, v_res);

            tmp = (Vec2uq)tmp16;
            tmp >>= 32;
            store_partial(const_int(4), pDst + dstStride, tmp);

            tmp = blend2q<1, 3>(reinterpret_i(tmp16), reinterpret_i(tmp16));
            store_partial(const_int(4), pDst + (2 * dstStride), tmp);

            tmp >>= 32;
            store_partial(const_int(4), pDst + (3 * dstStride), tmp);
        }
        else
        {
            Vec16uc v_main;
            v_main = load_partial(const_int(4), refMain + 1);
            store_partial(const_int(4), pDst, v_main);
            store_partial(const_int(4), pDst + dstStride, v_main);
            store_partial(const_int(4), pDst + (2 * dstStride), v_main);
            store_partial(const_int(4), pDst + (3 * dstStride), v_main);

            for (int k = 0; k < 4; k++)
            {
                pDst[k * dstStride] = (pixel)Clip3((short)0, (short)((1 << 8) - 1), static_cast<short>((pDst[k * dstStride]) + ((refSide[k + 1] - refSide[0]) >> 1)));
            }
        }
    }
    else
    {
        Vec8s row11, row12, row21, row22, row31, row32, row41, row42;
        Vec16uc tmp16_1, tmp16_2;
        Vec2uq tmp2uq;
        Vec8s v_deltaFract, v_deltaPos(0), thirty2(32), thirty1(31), v_ipAngle(0);
        switch (intraPredAngle)
        {
        case -32:
            tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain);    //-1,0,1,2
            store_partial(const_int(4), pDst, tmp16_1);
            tmp16_2 = (Vec16uc)load_partial(const_int(8), refMain - 1); //-2,-1,0,1
            store_partial(const_int(4), pDst + dstStride, tmp16_2);
            tmp16_2 = (Vec16uc)load_partial(const_int(8), refMain - 2);
            store_partial(const_int(4), pDst + 2 * dstStride, tmp16_2);
            tmp16_2 = (Vec16uc)load_partial(const_int(8), refMain - 3);
            store_partial(const_int(4), pDst + 3 * dstStride, tmp16_2);
            return;

        case -26:
            tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain - 3);
            row41 = extend_low(tmp16_1);    //offsets(-4,-3,-2,-1)
            tmp2uq = reinterpret_i(tmp16_1);
            tmp2uq = tmp2uq >> 8;
            tmp16_2 = reinterpret_i(tmp2uq);
            row42 = extend_low(tmp16_2);    //offsets(-3,-2,-1,0)

            row31 = row42;                  //offsets(-3,-2,-1,0)
            tmp2uq = reinterpret_i(tmp16_1);
            tmp2uq = tmp2uq >> 16;
            tmp16_2 = reinterpret_i(tmp2uq);
            row32 = extend_low(tmp16_2);    //offsets(-2,-1,0,1)

            row21 = row32;                  //offsets(-2,-1,0,1)
            tmp2uq = reinterpret_i(tmp16_1);
            tmp2uq = tmp2uq >> 24;
            tmp16_2 = reinterpret_i(tmp2uq);
            row22 = extend_low(tmp16_2);    //offsets(-1,0,1,2)

            row11 = row22;                  //offsets(-1,0,1,2)
            tmp2uq = reinterpret_i(tmp16_1);
            tmp2uq = tmp2uq >> 32;
            tmp16_2 = reinterpret_i(tmp2uq);
            row12 = extend_low(tmp16_2);    //offsets(0,1,2,3)

            v_deltaPos = v_ipAngle = -26;
            break;

        case -21:
            tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain - 2);
            row41 = extend_low(tmp16_1);    //offsets(-3,-2,-1,0)
            tmp2uq = reinterpret_i(tmp16_1);
            tmp2uq = tmp2uq >> 8;
            tmp16_2 = reinterpret_i(tmp2uq);
            row42 = extend_low(tmp16_2);    //offsets(-2,-1,0,1)

            row31 = row42;                  //offsets(-2,-1,0,1)
            tmp2uq = reinterpret_i(tmp16_1);
            tmp2uq = tmp2uq >> 16;
            tmp16_2 = reinterpret_i(tmp2uq);
            row32 = extend_low(tmp16_2);    //offsets(-1,0,1,2)

            row21 = row31;                  //offsets(-2,-1,0,1)
            row22 = row32;

            row11 = row32;
            tmp2uq = reinterpret_i(tmp16_1);
            tmp2uq = tmp2uq >> 24;
            tmp16_2 = reinterpret_i(tmp2uq);
            row12 = extend_low(tmp16_2);    //offsets(-1,0,1,2)

            v_deltaPos = v_ipAngle = -21;
            break;

        case -17:
            tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain - 2);
            row41 = extend_low(tmp16_1);    //offsets(-3,-2,-1,0)
            tmp2uq = reinterpret_i(tmp16_1);
            tmp2uq = tmp2uq >> 8;
            tmp16_2 = reinterpret_i(tmp2uq);
            row42 = extend_low(tmp16_2);    //offsets(-2,-1,0,1)

            row31 = row42;                  //offsets(-2,-1,0,1)
            tmp2uq = reinterpret_i(tmp16_1);
            tmp2uq = tmp2uq >> 16;
            tmp16_2 = reinterpret_i(tmp2uq);
            row32 = extend_low(tmp16_2);    //offsets(-1,0,1,2)

            row21 = row31;                  //offsets(-2,-1,0,1)
            row22 = row32;

            row11 = row32;
            tmp2uq = reinterpret_i(tmp16_1);
            tmp2uq = tmp2uq >> 24;
            tmp16_2 = reinterpret_i(tmp2uq);
            row12 = extend_low(tmp16_2);    //offsets(-1,0,1,2)

            v_deltaPos = v_ipAngle = -17;
            break;

        case -13:
            tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain - 1);
            row41 = extend_low(tmp16_1);    //offsets(-2,-1,0,1)
            tmp2uq = reinterpret_i(tmp16_1);
            tmp2uq = tmp2uq >> 8;
            tmp16_2 = reinterpret_i(tmp2uq);
            row42 = extend_low(tmp16_2);    //offsets(-1,0,1,2)

            row11 = row42;
            tmp2uq = reinterpret_i(tmp16_1);
            tmp2uq = tmp2uq >> 16;
            tmp16_2 = reinterpret_i(tmp2uq);
            row12 = extend_low(tmp16_2);    //offsets(-1,0,1,2)

            row21 = row42;                  //offsets(0,1,2,3)
            row22 = row12;
            row31 = row41;
            row32 = row42;

            v_deltaPos = v_ipAngle = -13;
            break;

        case -9:
            tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain - 1);
            row41 = extend_low(tmp16_1);    //offsets(-2,-1,0,1)
            tmp2uq = reinterpret_i(tmp16_1);
            tmp2uq = tmp2uq >> 8;
            tmp16_2 = reinterpret_i(tmp2uq);
            row42 = extend_low(tmp16_2);    //offsets(-1,0,1,2)

            row11 = row42;
            tmp2uq = reinterpret_i(tmp16_1);
            tmp2uq = tmp2uq >> 16;
            tmp16_2 = reinterpret_i(tmp2uq);
            row12 = extend_low(tmp16_2);    //offsets(-1,0,1,2)

            row21 = row42;                  //offsets(0,1,2,3)
            row22 = row12;
            row31 = row42;
            row32 = row12;

            v_deltaPos = v_ipAngle = -9;
            break;

        case -5:
            tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain);
            row11 = extend_low(tmp16_1);    //offsets(0,1,2,3)
            tmp2uq = reinterpret_i(tmp16_1);
            tmp2uq = tmp2uq >> 8;
            tmp16_2 = reinterpret_i(tmp2uq);
            row12 = extend_low(tmp16_2);    //offsets(1,2,3,4)
            row21 = row11;                  //offsets(0,1,2,3)
            row22 = row12;
            row31 = row11;
            row32 = row12;
            row41 = row11;
            row42 = row12;

            v_deltaPos = v_ipAngle = -5;
            break;

        case -2:
            tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain);
            row11 = extend_low(tmp16_1);    //offsets(0,1,2,3)
            tmp2uq = reinterpret_i(tmp16_1);
            tmp2uq = tmp2uq >> 8;
            tmp16_2 = reinterpret_i(tmp2uq);
            row12 = extend_low(tmp16_2);    //offsets(1,2,3,4)
            row21 = row11;                  //offsets(0,1,2,3)
            row22 = row12;
            row31 = row11;
            row32 = row12;
            row41 = row11;
            row42 = row12;

            v_deltaPos = v_ipAngle = -2;
            break;

        case 2:
            tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain + 1);
            row11 = extend_low(tmp16_1);    //offsets(0,1,2,3)
            tmp2uq = reinterpret_i(tmp16_1);
            tmp2uq = tmp2uq >> 8;
            tmp16_2 = reinterpret_i(tmp2uq);
            row12 = extend_low(tmp16_2);    //offsets(1,2,3,4)
            row21 = row11;                  //offsets(0,1,2,3)
            row22 = row12;
            row31 = row11;
            row32 = row12;
            row41 = row11;
            row42 = row12;

            v_deltaPos = v_ipAngle = 2;
            break;

        case 5:
            tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain + 1);
            row11 = extend_low(tmp16_1);    //offsets(0,1,2,3)
            tmp2uq = reinterpret_i(tmp16_1);
            tmp2uq = tmp2uq >> 8;
            tmp16_2 = reinterpret_i(tmp2uq);
            row12 = extend_low(tmp16_2);    //offsets(1,2,3,4)
            row21 = row11;                  //offsets(0,1,2,3)
            row22 = row12;
            row31 = row11;
            row32 = row12;
            row41 = row11;
            row42 = row12;

            v_deltaPos = v_ipAngle = 5;
            break;

        case 9:
            tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain + 1);
            row11 = extend_low(tmp16_1);    //offsets(0,1,2,3)
            tmp2uq = reinterpret_i(tmp16_1);
            tmp2uq = tmp2uq >> 8;
            tmp16_2 = reinterpret_i(tmp2uq);
            row12 = extend_low(tmp16_2);    //offsets(1,2,3,4)
            row21 = row11;                  //offsets(0,1,2,3)
            row22 = row12;
            row31 = row11;
            row32 = row12;
            row41 = row12;
            tmp2uq = reinterpret_i(tmp16_1);
            tmp2uq = tmp2uq >> 16;
            tmp16_2 = reinterpret_i(tmp2uq);
            row42 = extend_low(tmp16_2);

            v_deltaPos = v_ipAngle = 9;
            break;

        case 13:
            tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain + 1);

            row11 = extend_low(tmp16_1);    //offsets(0,1,2,3)

            tmp2uq = reinterpret_i(tmp16_1);
            tmp2uq = tmp2uq >> 8;
            tmp16_2 = reinterpret_i(tmp2uq);
            row12 = extend_low(tmp16_2);    //offsets(1,2,3,4)

            row21 = row11;                  //offsets(0,1,2,3)
            row22 = row12;
            row31 = row12;                  //offsets(1,2,3,4)

            tmp2uq = reinterpret_i(tmp16_1);
            tmp2uq = tmp2uq >> 16;
            tmp16_2 = reinterpret_i(tmp2uq);
            row32 = extend_low(tmp16_2);    //offsets(2,3,4,5)

            row41 = row31;                  //offsets(1,2,3,4)
            row42 = row32;

            v_deltaPos = v_ipAngle = 13;
            break;

        case 17:
            tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain + 1);

            row11 = extend_low(tmp16_1);    //offsets(0,1,2,3)

            tmp2uq = reinterpret_i(tmp16_1);
            tmp2uq = tmp2uq >> 8;
            tmp16_2 = reinterpret_i(tmp2uq);
            row12 = extend_low(tmp16_2);    //offsets(1,2,3,4)

            row21 = row12;

            tmp2uq = reinterpret_i(tmp16_1);
            tmp2uq = tmp2uq >> 16;
            tmp16_2 = reinterpret_i(tmp2uq);
            row22 = extend_low(tmp16_2);    //offsets(2,3,4,5)

            row31 = row21;
            row32 = row22;

            row41 = row22;
            tmp2uq = reinterpret_i(tmp16_1);
            tmp2uq = tmp2uq >> 24;
            tmp16_2 = reinterpret_i(tmp2uq);
            row42 = extend_low(tmp16_2);    //offsets(3,4,5,6)

            v_deltaPos = v_ipAngle = 17;
            break;

        case 21:
            tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain + 1);

            row11 = extend_low(tmp16_1);    //offsets(0,1,2,3)

            tmp2uq = reinterpret_i(tmp16_1);
            tmp2uq = tmp2uq >> 8;
            tmp16_2 = reinterpret_i(tmp2uq);
            row12 = extend_low(tmp16_2);    //offsets(1,2,3,4)

            row21 = row12;

            tmp2uq = reinterpret_i(tmp16_1);
            tmp2uq = tmp2uq >> 16;
            tmp16_2 = reinterpret_i(tmp2uq);
            row22 = extend_low(tmp16_2);    //offsets(2,3,4,5)

            row31 = row21;
            row32 = row22;

            row41 = row22;
            tmp2uq = reinterpret_i(tmp16_1);
            tmp2uq = tmp2uq >> 24;
            tmp16_2 = reinterpret_i(tmp2uq);
            row42 = extend_low(tmp16_2);    //offsets(3,4,5,6)

            v_deltaPos = v_ipAngle = 21;
            break;

        case 26:
            tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain + 1);

            row11 = extend_low(tmp16_1);    //offsets(0,1,2,3)

            tmp2uq = reinterpret_i(tmp16_1);
            tmp2uq = tmp2uq >> 8;
            tmp16_2 = reinterpret_i(tmp2uq);
            row12 = extend_low(tmp16_2);    //offsets(1,2,3,4)

            row21 = row12;

            tmp2uq = reinterpret_i(tmp16_1);
            tmp2uq = tmp2uq >> 16;
            tmp16_2 = reinterpret_i(tmp2uq);
            row22 = extend_low(tmp16_2);    //offsets(2,3,4,5)

            row31 = row22;
            tmp2uq = reinterpret_i(tmp16_1);
            tmp2uq = tmp2uq >> 24;
            tmp16_2 = reinterpret_i(tmp2uq);
            row32 = extend_low(tmp16_2);    //offsets(3,4,5,6)

            row41 = row32;
            tmp2uq = reinterpret_i(tmp16_1);
            tmp2uq = tmp2uq >> 32;
            tmp16_2 = reinterpret_i(tmp2uq);
            row42 = extend_low(tmp16_2);    //offsets(4,5,6,7)

            v_deltaPos = v_ipAngle = 26;
            break;

        case 32:
            tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain + 2);
            store_partial(const_int(4), pDst, tmp16_1);
            tmp16_2 = (Vec16uc)load_partial(const_int(8), refMain + 3);
            store_partial(const_int(4), pDst + dstStride, tmp16_2);
            tmp16_2 = (Vec16uc)load_partial(const_int(8), refMain + 4);
            store_partial(const_int(4), pDst + 2 * dstStride, tmp16_2);
            tmp16_2 = (Vec16uc)load_partial(const_int(8), refMain + 5);
            store_partial(const_int(4), pDst + 3 * dstStride, tmp16_2);
            return;
        }

        //row1
        v_deltaFract = v_deltaPos & thirty1;
        row11 = ((thirty2 - v_deltaFract) * row11 + (v_deltaFract * row12) + 16) >> 5;

        //row2
        v_deltaPos += v_ipAngle;
        v_deltaFract = v_deltaPos & thirty1;
        row21 = ((thirty2 - v_deltaFract) * row21 + (v_deltaFract * row22) + 16) >> 5;

        //row3
        v_deltaPos += v_ipAngle;
        v_deltaFract = v_deltaPos & thirty1;
        row31 = ((thirty2 - v_deltaFract) * row31 + (v_deltaFract * row32) + 16) >> 5;

        //row4
        v_deltaPos += v_ipAngle;
        v_deltaFract = v_deltaPos & thirty1;
        row41 = ((thirty2 - v_deltaFract) * row41 + (v_deltaFract * row42) + 16) >> 5;

        // Flip the block

        if (modeHor)
        {
            Vec8s tmp1, tmp2, tmp3, tmp4;

            tmp1 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>(row11, row31);
            tmp2 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>(row21, row41);

            tmp3 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>(tmp1, tmp2);
            tmp4 = blend8s<4, 12, 5, 13, 6, 14, 7, 15>(tmp1, tmp2);

            tmp16_1 = compress(tmp3, tmp3);
            store_partial(const_int(4), pDst, tmp16_1);

            tmp2uq = reinterpret_i(tmp16_1);
            tmp2uq >>= 32;
            store_partial(const_int(4), pDst + dstStride, tmp2uq);

            tmp16_1 = compress(tmp4, tmp4);
            store_partial(const_int(4), pDst + (2 * dstStride), tmp16_1);

            tmp2uq = reinterpret_i(tmp16_1);
            tmp2uq >>= 32;
            store_partial(const_int(4), pDst + (3 * dstStride), tmp2uq);
        }
        else
        {
            store_partial(const_int(4), pDst, compress(row11, row11));
            store_partial(const_int(4), pDst + (dstStride), compress(row21, row21));
            store_partial(const_int(4), pDst + (2 * dstStride), compress(row31, row31));
            store_partial(const_int(4), pDst + (3 * dstStride), compress(row41, row41));
        }
    }
}

#endif /* if HIGH_BIT_DEPTH */

#if HIGH_BIT_DEPTH
#else
#define PREDANG_CALCROW_VER(X) { \
        LOADROW(row11, GETAP(lookIdx, X)); \
        LOADROW(row12, GETAP(lookIdx, X) + 1); \
        CALCROW(row11, row11, row12); \
        store_partial(const_int(8), pDst + (X * dstStride), compress(row11, row11)); \
}

#define PREDANG_CALCROW_HOR(X, rowx) { \
        LOADROW(row11, GETAP(lookIdx, X)); \
        LOADROW(row12, GETAP(lookIdx, X) + 1); \
        CALCROW(rowx, row11, row12); \
}

// ROW is a Vec8s variable, X is the index in of data to be loaded
#define LOADROW(ROW, X) { \
        tmp = load_partial(const_int(8), refMain + 1 + X); \
        ROW = extend_low(tmp); \
}

#define CALCROW(RES, ROW1, ROW2) { \
        v_deltaPos += v_ipAngle; \
        v_deltaFract = v_deltaPos & thirty1; \
        RES = ((thirty2 - v_deltaFract) * ROW1 + (v_deltaFract * ROW2) + 16) >> 5; \
}

void xPredIntraAng8x8(int bitDepth, pixel* pDst, int dstStride, int width, int dirMode, pixel *refLeft, pixel *refAbove)
{
    int k;
    int blkSize        = width;

    // Map the mode index to main prediction direction and angle
    assert(dirMode > 1); //no planar and dc
    bool modeHor       = (dirMode < 18);
    bool modeVer       = !modeHor;
    int intraPredAngle = modeVer ? (int)dirMode - VER_IDX : modeHor ? -((int)dirMode - HOR_IDX) : 0;
    int lookIdx = intraPredAngle;
    int absAng         = abs(intraPredAngle);
    int signAng        = intraPredAngle < 0 ? -1 : 1;

    // Set bitshifts and scale the angle parameter to block size
    int angTable[9]    = { 0,    2,    5,   9,  13,  17,  21,  26,  32 };
    int invAngTable[9] = { 0, 4096, 1638, 910, 630, 482, 390, 315, 256 }; // (256 * 32) / Angle
    int invAngle       = invAngTable[absAng];
    absAng             = angTable[absAng];
    intraPredAngle     = signAng * absAng;

    // Do angular predictions

    pixel* refMain;
    pixel* refSide;

    // Initialise the Main and Left reference array.
    if (intraPredAngle < 0)
    {
        refMain = (modeVer ? refAbove : refLeft);     // + (blkSize - 1);
        refSide = (modeVer ? refLeft : refAbove);     // + (blkSize - 1);

        // Extend the Main reference to the left.
        int invAngleSum    = 128;     // rounding for (shift by 8)
        for (k = -1; k > blkSize * intraPredAngle >> 5; k--)
        {
            invAngleSum += invAngle;
            refMain[k] = refSide[invAngleSum >> 8];
        }
    }
    else
    {
        refMain = modeVer ? refAbove : refLeft;
        refSide = modeVer ? refLeft  : refAbove;
    }

    // bfilter will always be true for blocksize 8
    if (intraPredAngle == 0)  // Exactly hotizontal/vertical angles
    {
        if (modeHor)
        {
            Vec16uc v_temp;
            Vec8s v_side_0(refSide[0]); // refSide[0] value in a vector

            v_temp.load(refSide + 1);
            Vec8s v_side;
            v_side = extend_low(v_temp);

            v_temp.load(refMain + 1);
            Vec8s v_main;
            v_main = extend_low(v_temp);

            Vec8s row0;
            row0 = permute8s<0, 0, 0, 0, 0, 0, 0, 0>(v_main);
            v_side -= v_side_0;
            v_side = v_side >> 1;
            row0 = row0 + v_side;
            row0 = min(max(0, row0), (1 << bitDepth) - 1);

            Vec16uc tmp1;
            tmp1 = compress(row0, row0);
            store_partial(const_int(8), pDst, tmp1);                //row0

            tmp1 = permute16uc<1, 1, 1, 1, 1, 1, 1, 1, -256, -256, -256, -256, -256, -256, -256, -256>(v_temp);
            store_partial(const_int(8), pDst + (1 * dstStride), tmp1); //row1

            tmp1 = permute16uc<2, 2, 2, 2, 2, 2, 2, 2, -256, -256, -256, -256, -256, -256, -256, -256>(v_temp);
            store_partial(const_int(8), pDst + (2 * dstStride), tmp1); //row2

            tmp1 = permute16uc<3, 3, 3, 3, 3, 3, 3, 3, -256, -256, -256, -256, -256, -256, -256, -256>(v_temp);
            store_partial(const_int(8), pDst + (3 * dstStride), tmp1); //row3

            tmp1 = permute16uc<4, 4, 4, 4, 4, 4, 4, 4, -256, -256, -256, -256, -256, -256, -256, -256>(v_temp);
            store_partial(const_int(8), pDst + (4 * dstStride), tmp1); //row4

            tmp1 = permute16uc<5, 5, 5, 5, 5, 5, 5, 5, -256, -256, -256, -256, -256, -256, -256, -256>(v_temp);
            store_partial(const_int(8), pDst + (5 * dstStride), tmp1); //row5

            tmp1 = permute16uc<6, 6, 6, 6, 6, 6, 6, 6, -256, -256, -256, -256, -256, -256, -256, -256>(v_temp);
            store_partial(const_int(8), pDst + (6 * dstStride), tmp1); //row6

            tmp1 = permute16uc<7, 7, 7, 7, 7, 7, 7, 7, -256, -256, -256, -256, -256, -256, -256, -256>(v_temp);
            store_partial(const_int(8), pDst + (7 * dstStride), tmp1); //row7
        }
        else
        {
            Vec16uc v_main;
            v_main = load_partial(const_int(8), refMain + 1);
            store_partial(const_int(8), pDst, v_main);
            store_partial(const_int(8), pDst + dstStride, v_main);
            store_partial(const_int(8), pDst + (2 * dstStride), v_main);
            store_partial(const_int(8), pDst + (3 * dstStride), v_main);
            store_partial(const_int(8), pDst + (4 * dstStride), v_main);
            store_partial(const_int(8), pDst + (5 * dstStride), v_main);
            store_partial(const_int(8), pDst + (6 * dstStride), v_main);
            store_partial(const_int(8), pDst + (7 * dstStride), v_main);

            Vec16uc v_temp;
            Vec8s v_side_0(refSide[0]); // refSide[0] value in a vector

            v_temp.load(refSide + 1);
            Vec8s v_side;
            v_side = extend_low(v_temp);

            v_temp.load(refMain + 1);
            Vec8s row0;
            row0 = permute16uc<0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1>(v_temp);
            v_side -= v_side_0;
            v_side = v_side >> 1;
            row0 = row0 + v_side;
            row0 = min(max(0, row0), (1 << bitDepth) - 1);

            pDst[0 * dstStride] = row0[0];
            pDst[1 * dstStride] = row0[1];
            pDst[2 * dstStride] = row0[2];
            pDst[3 * dstStride] = row0[3];
            pDst[4 * dstStride] = row0[4];
            pDst[5 * dstStride] = row0[5];
            pDst[6 * dstStride] = row0[6];
            pDst[7 * dstStride] = row0[7];
        }
    }
    else if (intraPredAngle == -32)
    {
        Vec16uc tmp;
        tmp = load_partial(const_int(8), refMain);        //-1,0,1,2
        store_partial(const_int(8), pDst, tmp);
        tmp = load_partial(const_int(8), refMain - 1);     //-2,-1,0,1
        store_partial(const_int(8), pDst + dstStride, tmp);
        tmp = load_partial(const_int(8), refMain - 2);
        store_partial(const_int(8), pDst + 2 * dstStride, tmp);
        tmp = load_partial(const_int(8), refMain - 3);
        store_partial(const_int(8), pDst + 3 * dstStride, tmp);
        tmp = load_partial(const_int(8), refMain - 4);
        store_partial(const_int(8), pDst + 4 * dstStride, tmp);
        tmp = load_partial(const_int(8), refMain - 5);
        store_partial(const_int(8), pDst + 5 * dstStride, tmp);
        tmp = load_partial(const_int(8), refMain - 6);
        store_partial(const_int(8), pDst + 6 * dstStride, tmp);
        tmp = load_partial(const_int(8), refMain - 7);
        store_partial(const_int(8), pDst + 7 * dstStride, tmp);
        return;
    }
    else if (intraPredAngle == 32)
    {
        Vec8s tmp;
        tmp = load_partial(const_int(8), refMain + 2);        //-1,0,1,2
        store_partial(const_int(8), pDst, tmp);
        tmp = load_partial(const_int(8), refMain + 3);     //-2,-1,0,1
        store_partial(const_int(8), pDst + dstStride, tmp);
        tmp = load_partial(const_int(8), refMain + 4);
        store_partial(const_int(8), pDst + 2 * dstStride, tmp);
        tmp = load_partial(const_int(8), refMain + 5);
        store_partial(const_int(8), pDst + 3 * dstStride, tmp);
        tmp = load_partial(const_int(8), refMain + 6);
        store_partial(const_int(8), pDst + 4 * dstStride, tmp);
        tmp = load_partial(const_int(8), refMain + 7);
        store_partial(const_int(8), pDst + 5 * dstStride, tmp);
        tmp = load_partial(const_int(8), refMain + 8);
        store_partial(const_int(8), pDst + 6 * dstStride, tmp);
        tmp = load_partial(const_int(8), refMain + 9);
        store_partial(const_int(8), pDst + 7 * dstStride, tmp);
        return;
    }
    else
    {
        if (modeHor)         // Near horizontal modes
        {
            Vec16uc tmp;
            Vec8s row11, row12;
            Vec16uc row1, row2, row3, row4, tmp16_1, tmp16_2;
            Vec8s v_deltaFract, v_deltaPos, thirty2(32), thirty1(31), v_ipAngle;
            Vec8s tmp1, tmp2;
            v_deltaPos = 0;
            v_ipAngle = intraPredAngle;
            switch (intraPredAngle)
            {
            case -5:
                LOADROW(row11, -1);
                LOADROW(row12, 0);
                CALCROW(tmp1, row11, row12);
                CALCROW(tmp2, row11, row12);
                row1 = compress(tmp1, tmp2);
                CALCROW(tmp1, row11, row12);
                CALCROW(tmp2, row11, row12);
                row2 = compress(tmp1, tmp2);
                CALCROW(tmp1, row11, row12);
                CALCROW(tmp2, row11, row12);
                row3 = compress(tmp1, tmp2);
                row12 = row11;
                LOADROW(row11, -2);
                CALCROW(tmp1, row11, row12);
                CALCROW(tmp2, row11, row12);
                row4 = compress(tmp1, tmp2);
                break;

            case -2:
                LOADROW(row11, -1);
                LOADROW(row12, 0);
                CALCROW(tmp1, row11, row12);
                CALCROW(tmp2, row11, row12);
                row1 = compress(tmp1, tmp2);
                CALCROW(tmp1, row11, row12);
                CALCROW(tmp2, row11, row12);
                row2 = compress(tmp1, tmp2);
                CALCROW(tmp1, row11, row12);
                CALCROW(tmp2, row11, row12);
                row3 = compress(tmp1, tmp2);
                CALCROW(tmp1, row11, row12);
                CALCROW(tmp2, row11, row12);
                row4 = compress(tmp1, tmp2);
                break;

            case 2:
                LOADROW(row11, 0);
                LOADROW(row12, 1);
                CALCROW(tmp1, row11, row12);
                CALCROW(tmp2, row11, row12);
                row1 = compress(tmp1, tmp2);
                CALCROW(tmp1, row11, row12);
                CALCROW(tmp2, row11, row12);
                row2 = compress(tmp1, tmp2);
                CALCROW(tmp1, row11, row12);
                CALCROW(tmp2, row11, row12);
                row3 = compress(tmp1, tmp2);
                CALCROW(tmp1, row11, row12);
                CALCROW(tmp2, row11, row12);
                row4 = compress(tmp1, tmp2);
                break;

            case 5:
                LOADROW(row11, 0);
                LOADROW(row12, 1);
                CALCROW(tmp1, row11, row12);
                CALCROW(tmp2, row11, row12);
                row1 = compress(tmp1, tmp2);
                CALCROW(tmp1, row11, row12);
                CALCROW(tmp2, row11, row12);
                row2 = compress(tmp1, tmp2);
                CALCROW(tmp1, row11, row12);
                CALCROW(tmp2, row11, row12);
                row3 = compress(tmp1, tmp2);
                row11 = row12;
                LOADROW(row12, 2);
                CALCROW(tmp1, row11, row12);
                CALCROW(tmp2, row11, row12);
                row4 = compress(tmp1, tmp2);
                break;

            default:               // these cases uses the lookup table to identify access patterns

                PREDANG_CALCROW_HOR(0, tmp1);
                PREDANG_CALCROW_HOR(1, tmp2);
                row1 = compress(tmp1, tmp2);
                PREDANG_CALCROW_HOR(2, tmp1);
                PREDANG_CALCROW_HOR(3, tmp2);
                row2 = compress(tmp1, tmp2);
                PREDANG_CALCROW_HOR(4, tmp1);
                PREDANG_CALCROW_HOR(5, tmp2);
                row3 = compress(tmp1, tmp2);
                PREDANG_CALCROW_HOR(6, tmp1);
                PREDANG_CALCROW_HOR(7, tmp2);
                row4 = compress(tmp1, tmp2);
            }

            // Flip the block
            tmp16_1 = blend16uc<0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23>(row1, row2);
            tmp16_2 = blend16uc<8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31>(row1, row2);
            row1 = tmp16_1;
            row2 = tmp16_2;

            tmp16_1 = blend16uc<0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23>(row3, row4);
            tmp16_2 = blend16uc<8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31>(row3, row4);
            row3 = tmp16_1;
            row4 = tmp16_2;

            tmp16_1 = blend16uc<0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23>(row1, row2);
            tmp16_2 = blend16uc<8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31>(row1, row2);
            row1 = tmp16_1;
            row2 = tmp16_2;

            tmp16_1 = blend16uc<0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23>(row3, row4);
            tmp16_2 = blend16uc<8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31>(row3, row4);
            row3 = tmp16_1;
            row4 = tmp16_2;

            tmp16_1 = blend4i<0, 4, 1, 5>((Vec4i)row1, (Vec4i)row3);
            tmp16_2 = blend4i<2, 6, 3, 7>((Vec4i)row1, (Vec4i)row3);
            row1 = tmp16_1;
            row3 = tmp16_2;

            tmp16_1 = blend4i<0, 4, 1, 5>((Vec4i)row2, (Vec4i)row4);
            tmp16_2 = blend4i<2, 6, 3, 7>((Vec4i)row2, (Vec4i)row4);
            row2 = tmp16_1;
            row4 = tmp16_2;

            store_partial(const_int(8), pDst, row1);   //row1
            store_partial(const_int(8), pDst + (2 * dstStride), row3);   //row3
            store_partial(const_int(8), pDst + (4 * dstStride), row2);   //row5
            store_partial(const_int(8), pDst + (6 * dstStride), row4);   //row7

            row1 = blend2q<1, 3>((Vec2q)row1, (Vec2q)row1);
            store_partial(const_int(8), pDst + (1 * dstStride), row1);   //row2

            row1 = blend2q<1, 3>((Vec2q)row3, (Vec2q)row3);
            store_partial(const_int(8), pDst + (3 * dstStride), row1);   //row4

            row1 = blend2q<1, 3>((Vec2q)row2, (Vec2q)row2);
            store_partial(const_int(8), pDst + (5 * dstStride), row1);   //row6

            row1 = blend2q<1, 3>((Vec2q)row4, (Vec2q)row4);
            store_partial(const_int(8), pDst + (7 * dstStride), row1);   //row8
        }
        else                         // Vertical modes
        {
            Vec8s row11, row12;
            Vec8s v_deltaFract, v_deltaPos, thirty2(32), thirty1(31), v_ipAngle;
            Vec16uc tmp;
            Vec8s tmp1, tmp2;
            v_deltaPos = 0;
            v_ipAngle = intraPredAngle;
            switch (intraPredAngle)
            {
            case -5:
                LOADROW(row11, -1);
                LOADROW(row12, 0);
                CALCROW(tmp1, row11, row12);
                CALCROW(tmp2, row11, row12);
                store_partial(const_int(8), pDst, compress(tmp1, tmp1));
                store_partial(const_int(8), pDst + (dstStride), compress(tmp2, tmp2));
                CALCROW(tmp1, row11, row12);
                CALCROW(tmp2, row11, row12);
                store_partial(const_int(8), pDst + (2 * dstStride), compress(tmp1, tmp1));
                store_partial(const_int(8), pDst + (3 * dstStride), compress(tmp2, tmp2));
                CALCROW(tmp1, row11, row12);
                CALCROW(tmp2, row11, row12);
                store_partial(const_int(8), pDst + (4 * dstStride), compress(tmp1, tmp1));
                store_partial(const_int(8), pDst + (5 * dstStride), compress(tmp2, tmp2));
                row12 = row11;
                LOADROW(row11, -2);
                CALCROW(tmp1, row11, row12);
                CALCROW(tmp2, row11, row12);
                store_partial(const_int(8), pDst + (6 * dstStride), compress(tmp1, tmp1));
                store_partial(const_int(8), pDst + (7 * dstStride), compress(tmp2, tmp2));
                break;

            case -2:
                LOADROW(row11, -1);
                LOADROW(row12, 0);
                CALCROW(tmp1, row11, row12);
                CALCROW(tmp2, row11, row12);
                store_partial(const_int(8), pDst, compress(tmp1, tmp1));
                store_partial(const_int(8), pDst + (dstStride), compress(tmp2, tmp2));
                CALCROW(tmp1, row11, row12);
                CALCROW(tmp2, row11, row12);
                store_partial(const_int(8), pDst + (2 * dstStride), compress(tmp1, tmp1));
                store_partial(const_int(8), pDst + (3 * dstStride), compress(tmp2, tmp2));
                CALCROW(tmp1, row11, row12);
                CALCROW(tmp2, row11, row12);
                store_partial(const_int(8), pDst + (4 * dstStride), compress(tmp1, tmp1));
                store_partial(const_int(8), pDst + (5 * dstStride), compress(tmp2, tmp2));
                CALCROW(tmp1, row11, row12);
                CALCROW(tmp2, row11, row12);
                store_partial(const_int(8), pDst + (6 * dstStride), compress(tmp1, tmp1));
                store_partial(const_int(8), pDst + (7 * dstStride), compress(tmp2, tmp2));
                break;

            case 2:
                LOADROW(row11, 0);
                LOADROW(row12, 1);
                CALCROW(tmp1, row11, row12);
                CALCROW(tmp2, row11, row12);
                store_partial(const_int(8), pDst, compress(tmp1, tmp1));
                store_partial(const_int(8), pDst + dstStride, compress(tmp2, tmp2));
                CALCROW(tmp1, row11, row12);
                CALCROW(tmp2, row11, row12);
                store_partial(const_int(8), pDst + (2 * dstStride), compress(tmp1, tmp1));
                store_partial(const_int(8), pDst + (3 * dstStride), compress(tmp2, tmp2));
                CALCROW(tmp1, row11, row12);
                CALCROW(tmp2, row11, row12);
                store_partial(const_int(8), pDst + (4 * dstStride), compress(tmp1, tmp1));
                store_partial(const_int(8), pDst + (5 * dstStride), compress(tmp2, tmp2));
                CALCROW(tmp1, row11, row12);
                CALCROW(tmp2, row11, row12);
                store_partial(const_int(8), pDst + (6 * dstStride), compress(tmp1, tmp1));
                store_partial(const_int(8), pDst + (7 * dstStride), compress(tmp2, tmp2));
                break;

            case 5:
                LOADROW(row11, 0);
                LOADROW(row12, 1);
                CALCROW(tmp1, row11, row12);
                CALCROW(tmp2, row11, row12);
                store_partial(const_int(8), pDst, compress(tmp1, tmp1));
                store_partial(const_int(8), pDst + dstStride, compress(tmp2, tmp2));
                CALCROW(tmp1, row11, row12);
                CALCROW(tmp2, row11, row12);
                store_partial(const_int(8), pDst + (2 * dstStride), compress(tmp1, tmp1));
                store_partial(const_int(8), pDst + (3 * dstStride), compress(tmp2, tmp2));
                CALCROW(tmp1, row11, row12);
                CALCROW(tmp2, row11, row12);
                store_partial(const_int(8), pDst + (4 * dstStride), compress(tmp1, tmp1));
                store_partial(const_int(8), pDst + (5 * dstStride), compress(tmp2, tmp2));
                row11 = row12;
                LOADROW(row12, 2);
                CALCROW(tmp1, row11, row12);
                CALCROW(tmp2, row11, row12);
                store_partial(const_int(8), pDst + (6 * dstStride), compress(tmp1, tmp1));
                store_partial(const_int(8), pDst + (7 * dstStride), compress(tmp2, tmp2));
                break;

            default:                   // these cases uses the lookup table to identify access patterns
                PREDANG_CALCROW_VER(0);
                PREDANG_CALCROW_VER(1);
                PREDANG_CALCROW_VER(2);
                PREDANG_CALCROW_VER(3);
                PREDANG_CALCROW_VER(4);
                PREDANG_CALCROW_VER(5);
                PREDANG_CALCROW_VER(6);
                PREDANG_CALCROW_VER(7);
            }
        }
    }
}

#undef PREDANG_CALCROW_VER
#undef PREDANG_CALCROW_HOR
#undef LOADROW
#undef CALCROW
#endif /* if HIGH_BIT_DEPTH */

#if HIGH_BIT_DEPTH
#else
#define PREDANG_CALCROW_VER(X) { \
        LOADROW(row11L, row11H, GETAP(lookIdx, X)); \
        LOADROW(row12L, row12H, GETAP(lookIdx, X) + 1); \
        CALCROW(row11L, row11H, row11L, row11H, row12L, row12H); \
        compress(row11L, row11H).store(pDst + ((X)*dstStride)); \
}

#define PREDANG_CALCROW_HOR(X, rowx) { \
        LOADROW(row11L, row11H, GETAP(lookIdx, (X))); \
        LOADROW(row12L, row12H, GETAP(lookIdx, (X)) + 1); \
        CALCROW(row11L, row11H, row11L, row11H, row12L, row12H); \
        rowx = compress(row11L, row11H); \
}

// ROWL/H is a Vec8s variable, X is the index in of data to be loaded
#define LOADROW(ROWL, ROWH, X) { \
        tmp.load(refMain + 1 + (X)); \
        ROWL = extend_low(tmp); \
        ROWH = extend_high(tmp); \
}

#define CALCROW(RESL, RESH, ROW1L, ROW1H, ROW2L, ROW2H) { \
        v_deltaPos += v_ipAngle; \
        v_deltaFract = v_deltaPos & thirty1; \
        RESL = ((thirty2 - v_deltaFract) * ROW1L + (v_deltaFract * ROW2L) + 16) >> 5; \
        RESH = ((thirty2 - v_deltaFract) * ROW1H + (v_deltaFract * ROW2H) + 16) >> 5; \
}

#define  BLND2_16(R1, R2) { \
        tmp1 = blend16uc<0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23>(R1, R2); \
        tmp2 = blend16uc<8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31>(R1, R2); \
        R1 = tmp1; \
        R2 = tmp2; \
}

#define MB4(R1, R2, R3, R4) { \
        BLND2_16(R1, R2) \
        BLND2_16(R3, R4) \
        tmp1 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>((Vec8s)R1, (Vec8s)R3); \
        tmp2 = blend8s<4, 12, 5, 13, 6, 14, 7, 15>((Vec8s)R1, (Vec8s)R3); \
        R1 = tmp1; \
        R3 = tmp2; \
        tmp1 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>((Vec8s)R2, (Vec8s)R4); \
        tmp2 = blend8s<4, 12, 5, 13, 6, 14, 7, 15>((Vec8s)R2, (Vec8s)R4); \
        R2 = tmp1; \
        R4 = tmp2; \
}

#define BLND2_4(R1, R2) { \
        tmp1 = blend4i<0, 4, 1, 5>((Vec4i)R1, (Vec4i)R2); \
        tmp2 = blend4i<2, 6, 3, 7>((Vec4i)R1, (Vec4i)R2); \
        R1 = tmp1; \
        R2 = tmp2; \
}

#define BLND2_2(R1, R2) { \
        tmp1 = blend2q<0, 2>((Vec2q)R1, (Vec2q)R2); \
        tmp2 = blend2q<1, 3>((Vec2q)R1, (Vec2q)R2); \
        tmp1.store(pDst);   pDst += dstStride; \
        tmp2.store(pDst);   pDst += dstStride; \
}

#define MB8(R1, R2, R3, R4, R5, R6, R7, R8) { \
        MB4(R1, R2, R3, R4) \
        MB4(R5, R6, R7, R8) \
        BLND2_4(R1, R5); \
        BLND2_4(R2, R6); \
        BLND2_4(R3, R7); \
        BLND2_4(R4, R8); \
}

#define CALC_BLND_8ROWS(R1, R2, R3, R4, R5, R6, R7, R8, X) { \
        PREDANG_CALCROW_HOR(0 + X, R1) \
        PREDANG_CALCROW_HOR(1 + X, R2) \
        PREDANG_CALCROW_HOR(2 + X, R3) \
        PREDANG_CALCROW_HOR(3 + X, R4) \
        PREDANG_CALCROW_HOR(4 + X, R5) \
        PREDANG_CALCROW_HOR(5 + X, R6) \
        PREDANG_CALCROW_HOR(6 + X, R7) \
        PREDANG_CALCROW_HOR(7 + X, R8) \
        MB8(R1, R2, R3, R4, R5, R6, R7, R8) \
}

#define MB16 { \
        CALC_BLND_8ROWS(R1, R2, R3, R4, R5, R6, R7, R8, 0) \
        CALC_BLND_8ROWS(R9, R10, R11, R12, R13, R14, R15, R16, 8) \
        BLND2_2(R1, R9) \
        BLND2_2(R5, R13) \
        BLND2_2(R3, R11) \
        BLND2_2(R7, R15) \
        BLND2_2(R2, R10) \
        BLND2_2(R6, R14) \
        BLND2_2(R4, R12) \
        BLND2_2(R8, R16) \
}

void xPredIntraAng16x16(int bitDepth, pixel* pDst, int dstStride, int width, int dirMode, pixel *refLeft, pixel *refAbove)
{
    int k;
    int blkSize        = width;

    // Map the mode index to main prediction direction and angle
    assert(dirMode > 1); //no planar and dc
    bool modeHor       = (dirMode < 18);
    bool modeVer       = !modeHor;
    int intraPredAngle = modeVer ? (int)dirMode - VER_IDX : modeHor ? -((int)dirMode - HOR_IDX) : 0;
    int lookIdx = intraPredAngle;
    int absAng         = abs(intraPredAngle);
    int signAng        = intraPredAngle < 0 ? -1 : 1;

    // Set bitshifts and scale the angle parameter to block size
    int angTable[9]    = { 0,    2,    5,   9,  13,  17,  21,  26,  32 };
    int invAngTable[9] = { 0, 4096, 1638, 910, 630, 482, 390, 315, 256 }; // (256 * 32) / Angle
    int invAngle       = invAngTable[absAng];
    absAng             = angTable[absAng];
    intraPredAngle     = signAng * absAng;

    // Do angular predictions

    pixel* refMain;
    pixel* refSide;

    // Initialise the Main and Left reference array.
    if (intraPredAngle < 0)
    {
        refMain = (modeVer ? refAbove : refLeft);     // + (blkSize - 1);
        refSide = (modeVer ? refLeft : refAbove);     // + (blkSize - 1);

        // Extend the Main reference to the left.
        int invAngleSum    = 128;     // rounding for (shift by 8)
        if (intraPredAngle != -32)
            for (k = -1; k > blkSize * intraPredAngle >> 5; k--)
            {
                invAngleSum += invAngle;
                refMain[k] = refSide[invAngleSum >> 8];
            }
    }
    else
    {
        refMain = modeVer ? refAbove : refLeft;
        refSide = modeVer ? refLeft  : refAbove;
    }

    // bfilter will always be true for blocksize 8
    if (intraPredAngle == 0)  // Exactly hotizontal/vertical angles
    {
        if (modeHor)
        {
            Vec16uc v_temp;
            Vec8s v_side_0(refSide[0]); // refSide[0] value in a vector

            v_temp.load(refSide + 1);
            Vec8s v_side;
            v_side = extend_low(v_temp);

            Vec8s row01, row02, ref(refMain[1]);
            v_side -= v_side_0;
            v_side = v_side >> 1;
            row01 = ref + v_side;
            row01 = min(max(0, row01), (1 << bitDepth) - 1);

            v_side = extend_high(v_temp);
            v_side -= v_side_0;
            v_side = v_side >> 1;
            row02 = ref + v_side;
            row02 = min(max(0, row02), (1 << bitDepth) - 1);

            Vec16uc tmp1;
            tmp1 = compress(row01, row02);
            tmp1.store(pDst);                //row0

            v_temp.load(refMain + 1);

            tmp1 = permute16uc<1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1>(v_temp);
            tmp1.store(pDst + (1 * dstStride)); //row1

            tmp1 = permute16uc<2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2>(v_temp);
            tmp1.store(pDst + (2 * dstStride)); //row2

            tmp1 = permute16uc<3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3>(v_temp);
            tmp1.store(pDst + (3 * dstStride)); //row3

            tmp1 = permute16uc<4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4>(v_temp);
            tmp1.store(pDst + (4 * dstStride)); //row4

            tmp1 = permute16uc<5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5>(v_temp);
            tmp1.store(pDst + (5 * dstStride)); //row5

            tmp1 = permute16uc<6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6>(v_temp);
            tmp1.store(pDst + (6 * dstStride)); //row6

            tmp1 = permute16uc<7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7>(v_temp);
            tmp1.store(pDst + (7 * dstStride)); //row7

            tmp1 = permute16uc<8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8>(v_temp);
            tmp1.store(pDst + (8 * dstStride)); //row8

            tmp1 = permute16uc<9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9>(v_temp);
            tmp1.store(pDst + (9 * dstStride)); //row9

            tmp1 = permute16uc<10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10>(v_temp);
            tmp1.store(pDst + (10 * dstStride)); //row10

            tmp1 = permute16uc<11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11>(v_temp);
            tmp1.store(pDst + (11 * dstStride)); //row11

            tmp1 = permute16uc<12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12>(v_temp);
            tmp1.store(pDst + (12 * dstStride)); //row12

            tmp1 = permute16uc<13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13>(v_temp);
            tmp1.store(pDst + (13 * dstStride)); //row13

            tmp1 = permute16uc<14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14>(v_temp);
            tmp1.store(pDst + (14 * dstStride)); //row14

            tmp1 = permute16uc<15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15>(v_temp);
            tmp1.store(pDst + (15 * dstStride)); //row15
        }
        else
        {
            Vec16uc v_main;
            v_main.load(refMain + 1);
            v_main.store(pDst);
            v_main.store(pDst + dstStride);
            v_main.store(pDst + (2 * dstStride));
            v_main.store(pDst + (3 * dstStride));
            v_main.store(pDst + (4 * dstStride));
            v_main.store(pDst + (5 * dstStride));
            v_main.store(pDst + (6 * dstStride));
            v_main.store(pDst + (7 * dstStride));
            v_main.store(pDst + (8 * dstStride));
            v_main.store(pDst + (9 * dstStride));
            v_main.store(pDst + (10 * dstStride));
            v_main.store(pDst + (11 * dstStride));
            v_main.store(pDst + (12 * dstStride));
            v_main.store(pDst + (13 * dstStride));
            v_main.store(pDst + (14 * dstStride));
            v_main.store(pDst + (15 * dstStride));

            Vec16uc v_temp;
            Vec8s v_side_0(refSide[0]); // refSide[0] value in a vector

            v_temp.load(refSide + 1);
            Vec8s v_side;
            v_side = extend_low(v_temp);

            Vec8s row0, ref(refMain[1]);
            v_side -= v_side_0;
            v_side = v_side >> 1;
            row0 = ref + v_side;
            row0 = min(max(0, row0), (1 << bitDepth) - 1);

            pDst[0 * dstStride] = row0[0];
            pDst[1 * dstStride] = row0[1];
            pDst[2 * dstStride] = row0[2];
            pDst[3 * dstStride] = row0[3];
            pDst[4 * dstStride] = row0[4];
            pDst[5 * dstStride] = row0[5];
            pDst[6 * dstStride] = row0[6];
            pDst[7 * dstStride] = row0[7];

            v_side = extend_high(v_temp);
            v_side -= v_side_0;
            v_side = v_side >> 1;
            row0 = ref + v_side;
            row0 = min(max(0, row0), (1 << bitDepth) - 1);
            pDst[8 * dstStride] = row0[0];
            pDst[9 * dstStride] = row0[1];
            pDst[10 * dstStride] = row0[2];
            pDst[11 * dstStride] = row0[3];
            pDst[12 * dstStride] = row0[4];
            pDst[13 * dstStride] = row0[5];
            pDst[14 * dstStride] = row0[6];
            pDst[15 * dstStride] = row0[7];
        }
    }
    else if (intraPredAngle == -32)
    {
        Vec16uc v_refSide;
        v_refSide.load(refSide);
        v_refSide = permute16uc<15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0>(v_refSide);
        pixel refMain0 = refMain[0];

        v_refSide.store(refMain - 15);
        refMain[0] = refMain0;

        Vec16uc tmp;
        tmp.load(refMain);        //-1,0,1,2
        tmp.store(pDst);
        tmp.load(--refMain);     //-2,-1,0,1
        pDst += dstStride;
        tmp.store(pDst);
        tmp.load(--refMain);
        pDst += dstStride;
        tmp.store(pDst);
        tmp.load(--refMain);
        pDst += dstStride;
        tmp.store(pDst);
        tmp.load(--refMain);
        pDst += dstStride;
        tmp.store(pDst);
        tmp.load(--refMain);
        pDst += dstStride;
        tmp.store(pDst);
        tmp.load(--refMain);
        pDst += dstStride;
        tmp.store(pDst);
        tmp.load(--refMain);
        pDst += dstStride;
        tmp.store(pDst);
        tmp.load(--refMain);
        pDst += dstStride;
        tmp.store(pDst);
        tmp.load(--refMain);
        pDst += dstStride;
        tmp.store(pDst);
        tmp.load(--refMain);
        pDst += dstStride;
        tmp.store(pDst);
        tmp.load(--refMain);
        pDst += dstStride;
        tmp.store(pDst);
        tmp.load(--refMain);
        pDst += dstStride;
        tmp.store(pDst);
        tmp.load(--refMain);
        pDst += dstStride;
        tmp.store(pDst);
        tmp.load(--refMain);
        pDst += dstStride;
        tmp.store(pDst);
        tmp.load(--refMain);
        pDst += dstStride;
        tmp.store(pDst);
        return;
    }
    else if (intraPredAngle == 32)
    {
        Vec8s tmp;

        tmp.load(refMain + 2);
        tmp.store(pDst);
        tmp.load(refMain + 3);
        pDst += dstStride;
        tmp.store(pDst);
        tmp.load(refMain + 4);
        pDst += dstStride;
        tmp.store(pDst);
        tmp.load(refMain + 5);
        pDst += dstStride;
        tmp.store(pDst);
        tmp.load(refMain + 6);
        pDst += dstStride;
        tmp.store(pDst);
        tmp.load(refMain + 7);
        pDst += dstStride;
        tmp.store(pDst);
        tmp.load(refMain + 8);
        pDst += dstStride;
        tmp.store(pDst);
        tmp.load(refMain + 9);
        pDst += dstStride;
        tmp.store(pDst);
        tmp.load(refMain + 10);
        pDst += dstStride;
        tmp.store(pDst);
        tmp.load(refMain + 11);
        pDst += dstStride;
        tmp.store(pDst);
        tmp.load(refMain + 12);
        pDst += dstStride;
        tmp.store(pDst);
        tmp.load(refMain + 13);
        pDst += dstStride;
        tmp.store(pDst);
        tmp.load(refMain + 14);
        pDst += dstStride;
        tmp.store(pDst);
        tmp.load(refMain + 15);
        pDst += dstStride;
        tmp.store(pDst);
        tmp.load(refMain + 16);
        pDst += dstStride;
        tmp.store(pDst);
        tmp.load(refMain + 17);
        pDst += dstStride;
        tmp.store(pDst);
        return;
    }
    else
    {
        if (modeHor)
        {
            Vec8s row11L, row12L, row11H, row12H;
            Vec8s v_deltaFract, v_deltaPos, thirty2(32), thirty1(31), v_ipAngle;
            Vec16uc tmp;
            Vec16uc R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, R13, R14, R15, R16;
            Vec16uc tmp1, tmp2;
            v_deltaPos = 0;
            v_ipAngle = intraPredAngle;
            MB16;
        }
        else
        {
            Vec8s row11L, row12L, row11H, row12H;
            Vec8s v_deltaFract, v_deltaPos, thirty2(32), thirty1(31), v_ipAngle;
            Vec16uc tmp;
            Vec8s tmp1, tmp2;
            v_deltaPos = 0;
            v_ipAngle = intraPredAngle;

            PREDANG_CALCROW_VER(0);
            PREDANG_CALCROW_VER(1);
            PREDANG_CALCROW_VER(2);
            PREDANG_CALCROW_VER(3);
            PREDANG_CALCROW_VER(4);
            PREDANG_CALCROW_VER(5);
            PREDANG_CALCROW_VER(6);
            PREDANG_CALCROW_VER(7);
            PREDANG_CALCROW_VER(8);
            PREDANG_CALCROW_VER(9);
            PREDANG_CALCROW_VER(10);
            PREDANG_CALCROW_VER(11);
            PREDANG_CALCROW_VER(12);
            PREDANG_CALCROW_VER(13);
            PREDANG_CALCROW_VER(14);
            PREDANG_CALCROW_VER(15);
        }
    }
}

#endif /* if HIGH_BIT_DEPTH */

void xPredIntraAngBufRef(int bitDepth, pixel* pDst, int dstStride, int width, int dirMode, bool bFilter, pixel *refLeft, pixel *refAbove)
{
#if HIGH_BIT_DEPTH
#else
    switch (width)
    {
    case 4:
        xPredIntraAng4x4(bitDepth, pDst, dstStride, width, dirMode, refLeft, refAbove);
        return;
    case 8:
        xPredIntraAng8x8(bitDepth, pDst, dstStride, width, dirMode, refLeft, refAbove);
        return;
    case 16:
        xPredIntraAng16x16(bitDepth, pDst, dstStride, width, dirMode, refLeft, refAbove);
        return;
    }

#endif /* if HIGH_BIT_DEPTH */

    int k, l;
    int blkSize        = width;

    // Map the mode index to main prediction direction and angle
    assert(dirMode > 1); //no planar and dc
    bool modeHor       = (dirMode < 18);
    bool modeVer       = !modeHor;
    int intraPredAngle = modeVer ? (int)dirMode - VER_IDX : modeHor ? -((int)dirMode - HOR_IDX) : 0;
    int absAng         = abs(intraPredAngle);
    int signAng        = intraPredAngle < 0 ? -1 : 1;

    // Set bitshifts and scale the angle parameter to block size
    int angTable[9]    = { 0,    2,    5,   9,  13,  17,  21,  26,  32 };
    int invAngTable[9] = { 0, 4096, 1638, 910, 630, 482, 390, 315, 256 }; // (256 * 32) / Angle
    int invAngle       = invAngTable[absAng];
    absAng             = angTable[absAng];
    intraPredAngle     = signAng * absAng;

    // Do angular predictions
    {
        pixel* refMain;
        pixel* refSide;

        // Initialise the Main and Left reference array.
        if (intraPredAngle < 0)
        {
            refMain = (modeVer ? refAbove : refLeft); // + (blkSize - 1);
            refSide = (modeVer ? refLeft : refAbove); // + (blkSize - 1);

            // Extend the Main reference to the left.
            int invAngleSum    = 128; // rounding for (shift by 8)
            for (k = -1; k > blkSize * intraPredAngle >> 5; k--)
            {
                invAngleSum += invAngle;
                refMain[k] = refSide[invAngleSum >> 8];
            }
        }
        else
        {
            refMain = modeVer ? refAbove : refLeft;
            refSide = modeVer ? refLeft  : refAbove;
        }

        if (intraPredAngle == 0)
        {
            for (k = 0; k < blkSize; k++)
            {
                for (l = 0; l < blkSize; l++)
                {
                    pDst[k * dstStride + l] = refMain[l + 1];
                }
            }

            if (bFilter)
            {
                for (k = 0; k < blkSize; k++)
                {
                    pDst[k * dstStride] = (pixel)Clip3(0, (1 << bitDepth) - 1, static_cast<short>(pDst[k * dstStride]) + ((refSide[k + 1] - refSide[0]) >> 1));
                }
            }
        }
        else
        {
            int deltaPos = 0;
            int deltaInt;
            int deltaFract;
            int refMainIndex;

            for (k = 0; k < blkSize; k++)
            {
                deltaPos += intraPredAngle;
                deltaInt   = deltaPos >> 5;
                deltaFract = deltaPos & (32 - 1);

                if (deltaFract)
                {
                    // Do linear filtering
                    for (l = 0; l < blkSize; l++)
                    {
                        refMainIndex        = l + deltaInt + 1;
                        pDst[k * dstStride + l] = (pixel)(((32 - deltaFract) * refMain[refMainIndex] + deltaFract * refMain[refMainIndex + 1] + 16) >> 5);
                    }
                }
                else
                {
                    // Just copy the integer samples
                    for (l = 0; l < blkSize; l++)
                    {
                        pDst[k * dstStride + l] = refMain[l + deltaInt + 1];
                    }
                }
            }
        }

        // Flip the block if this is the horizontal mode
        if (modeHor)
        {
            pixel  tmp;
            for (k = 0; k < blkSize - 1; k++)
            {
                for (l = k + 1; l < blkSize; l++)
                {
                    tmp                 = pDst[k * dstStride + l];
                    pDst[k * dstStride + l] = pDst[l * dstStride + k];
                    pDst[l * dstStride + k] = tmp;
                }
            }
        }
    }
}
}

#include "utils.h"

namespace x265 {

void NAME(Setup_Vec_IPredPrimitives)(EncoderPrimitives& p)
{
    p.getIPredDC = predIntraDC;
    p.getIPredPlanar = predIntraPlanar;
    p.getIPredAng = xPredIntraAngBufRef;
}

}
