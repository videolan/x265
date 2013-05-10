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
    switch( iWidth )
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
#else
    Vec16uc pix;
    switch( iWidth )
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
#endif

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
        switch( width )
        {
            case 4:
                sumAbove = load_partial(const_int(8), pSrcAbove);
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
        iSum += horizontal_add_x(sumAbove);
    }

    logSize += (blkAboveAvailable + blkLeftAvailable - 1);
    pixel dcVal = (iSum + (1<<(logSize-1))) >> logSize;
    Vec8us dcValN(dcVal);
    int k;

    pixel *pDst1 = pDst;
    switch( width )
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
            for (k=0; k<16; k+=2)
            {
                dcValN.store(pDst1    );
                dcValN.store(pDst1 + 8);
                pDst1 += dstStride;
                dcValN.store(pDst1    );
                dcValN.store(pDst1 + 8);
                pDst1 += dstStride;
            }
            break;

        case 32:
            for (k=0; k<32; k++)
            {
                dcValN.store(pDst1     );
                dcValN.store(pDst1 +  8);
                dcValN.store(pDst1 + 16);
                dcValN.store(pDst1 + 24);
                pDst1 += dstStride;
            }
            break;

        //case 64:
        default:
            for (k=0; k<64; k++)
            {
                dcValN.store(pDst1     );
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

        switch( width )
        {
            case 4:
                pix.fromUint32(*(uint32_t*)pSrcAbove);
                iSum += horizontal_add(extend_low(pix));
                break;
            case 8:
#if _WIN64
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
    pixel dcVal = (iSum + (1<<(logSize-1))) >> logSize;
    Vec16uc dcValN(dcVal);
    int k;

    pixel *pDst1 = pDst;
    switch( width )
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
            for (k=0; k<16; k+=4)
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
            for (k=0; k<32; k+=2)
            {
                store_partial(const_int(16), pDst1,    dcValN);
                store_partial(const_int(16), pDst1+16, dcValN);
                pDst1 += dstStride;
                store_partial(const_int(16), pDst1,    dcValN);
                store_partial(const_int(16), pDst1+16, dcValN);
                pDst1 += dstStride;
            }
            break;

        case 64:
            for (k=0; k<64; k++)
            {
                store_partial(const_int(16), pDst1,    dcValN);
                store_partial(const_int(16), pDst1+16, dcValN);
                store_partial(const_int(16), pDst1+32, dcValN);
                store_partial(const_int(16), pDst1+48, dcValN);
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
void predIntraPlanar4(pixel* pSrc, intptr_t srcStride, pixel* rpDst, intptr_t dstStride)
{
    int k, l, bottomLeft, topRight;
    int horPred;
    int32_t leftColumn[4 + 1], topRow[4 + 1];
    // CHECK_ME: dynamic range is 9 bits or 15 bits(I assume max input bit_depth is 14 bits)
    int16_t bottomRow[4], rightColumn[4];
    int offset2D = 4;
    int shift1D = 2;
    int shift2D = shift1D + 1;

    // Get left and above reference column and row
    for (k = 0; k < 4 + 1; k++)
    {
        topRow[k] = pSrc[k - srcStride];
        leftColumn[k] = pSrc[k * srcStride - 1];
    }

    // Prepare intermediate variables used in interpolation
    bottomLeft = leftColumn[4];
    topRight   = topRow[4];
    for (k = 0; k < 4; k++)
    {
        bottomRow[k]   = bottomLeft - topRow[k];
        rightColumn[k] = topRight   - leftColumn[k];
        topRow[k]      <<= shift1D;
        leftColumn[k]  <<= shift1D;
    }

    // Generate prediction signal
    for (k = 0; k < 4; k++)
    {
        horPred = leftColumn[k] + offset2D;
        for (l = 0; l < 4; l++)
        {
            horPred += rightColumn[k];
            topRow[l] += bottomRow[l];
            rpDst[k * dstStride + l] = ((horPred + topRow[l]) >> shift2D);
        }
    }
}
#else
void predIntraPlanar4(pixel* pSrc, intptr_t srcStride, pixel* rpDst, intptr_t dstStride)
{
    int k;
    pixel bottomLeft, topRight;
    int16_t leftColumn[4];

    // Get left and above reference column and row
    Vec16uc im0 = load_partial(const_int(4), &pSrc[-srcStride]); // topRow
    Vec8s v_topRow = extend_low(im0);

    for (k = 0; k < 4; k++)
    {
        leftColumn[k] = pSrc[k * srcStride - 1];
    }
    Vec8s v_leftColumn = load_partial(const_int(8), leftColumn);   // leftColumn

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

    // line0
    v_horPred = broadcast(const_int(0), v_horPred4);
    v_rightColumnN = broadcast(const_int(0), v_rightColumn) * v_multi;
    v_horPred = v_horPred + v_rightColumnN;
    v_topRow = v_topRow + v_bottomRow;
    v_im4 = (Vec8s)(v_horPred + v_topRow) >> const_int(3);
    v_im5 = compress_unsafe(v_im4, v_im4);
    store_partial(const_int(4), &rpDst[0 * dstStride], v_im5);

    // line1
    v_horPred = broadcast(const_int(1), v_horPred4);
    v_rightColumnN = broadcast(const_int(1), v_rightColumn) * v_multi;
    v_horPred = v_horPred + v_rightColumnN;
    v_topRow = v_topRow + v_bottomRow;
    v_im4 = (Vec8s)(v_horPred + v_topRow) >> const_int(3);
    v_im5 = compress_unsafe(v_im4, v_im4);
    store_partial(const_int(4), &rpDst[1 * dstStride], v_im5);

    // line2
    v_horPred = broadcast(const_int(2), v_horPred4);
    v_rightColumnN = broadcast(const_int(2), v_rightColumn) * v_multi;
    v_horPred = v_horPred + v_rightColumnN;
    v_topRow = v_topRow + v_bottomRow;
    v_im4 = (Vec8s)(v_horPred + v_topRow) >> const_int(3);
    v_im5 = compress_unsafe(v_im4, v_im4);
    store_partial(const_int(4), &rpDst[2 * dstStride], v_im5);

    // line3
    v_horPred = broadcast(const_int(3), v_horPred4);
    v_rightColumnN = broadcast(const_int(3), v_rightColumn) * v_multi;
    v_horPred = v_horPred + v_rightColumnN;
    v_topRow = v_topRow + v_bottomRow;
    v_im4 = (Vec8s)(v_horPred + v_topRow) >> const_int(3);
    v_im5 = compress_unsafe(v_im4, v_im4);
    store_partial(const_int(4), &rpDst[3 * dstStride], v_im5);
}
#endif

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
    int shift1D = g_aucConvertToBit[width] + 2;
    int shift2D = shift1D + 1;

    if ( width == 4) {
        predIntraPlanar4(pSrc, srcStride, rpDst, dstStride);
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

void Setup_Vec_IPredPrimitives(EncoderPrimitives& p)
{
    p.getIPredDC = predIntraDC;
    p.getIPredPlanar = predIntraPlanar;
}
