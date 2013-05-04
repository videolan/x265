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

#else

    if (bAbove)
    {
        Vec16uc pix;
        Vec8us  im;
        Vec4ui  im1, im2;

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
                im1 = Vec4ui(pix.sad(_mm_setzero_si128()));
                pix.load(pSrcAbove + 16);
                im1 += Vec4ui(pix.sad(_mm_setzero_si128()));
                im1 += (im1 >> const_int(64));
                iSum += toInt32(im1);
                break;
            //case 64:
            default:
                pix.load(pSrcAbove);
                im1 = Vec4ui(pix.sad(_mm_setzero_si128()));
                pix.load(pSrcAbove + 16);
                im1 += Vec4ui(pix.sad(_mm_setzero_si128()));
                pix.load(pSrcAbove + 32);
                im1 += Vec4ui(pix.sad(_mm_setzero_si128()));
                pix.load(pSrcAbove + 48);
                im1 += Vec4ui(pix.sad(_mm_setzero_si128()));
                im1 += (im1 >> const_int(64));
                //im1 += extract_hi64(im1);
                iSum += toInt32(im1);
                break;
        }
    }
#endif

    if (bLeft)
    {
        for (int iInd = 0; iInd < iHeight; iInd++)
        {
            iSum += pSrcAbove[iSrcStride - 1];
            pSrcAbove += iSrcStride;
        }
    }

//     assert(bAbove || bLeft);
    logSize += (bAbove + bLeft - 1);
    return (iSum + (1<<(logSize-1))) >> logSize;
}

void Setup_Vec_IPredPrimitives(EncoderPrimitives& p)
{
    p.getdcval_p = predIntraGetPredValDC;
}
