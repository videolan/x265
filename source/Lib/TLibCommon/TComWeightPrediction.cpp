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

/** \file     TComWeightPrediction.h
    \brief    weighting prediction class (header)
*/

// Include files
#include "TComSlice.h"
#include "TComWeightPrediction.h"
#include "TComPrediction.h"
#include "primitives.h"

static inline Pel weightBidirY(Int w0, Short P0, Int w1, Short P1, Int round, Int shift, Int offset)
{
    return ClipY(((w0 * (P0 + IF_INTERNAL_OFFS) + w1 * (P1 + IF_INTERNAL_OFFS) + round + (offset << (shift - 1))) >> shift));
}

static inline Pel weightBidirC(Int w0, Short P0, Int w1, Short P1, Int round, Int shift, Int offset)
{
    return ClipC(((w0 * (P0 + IF_INTERNAL_OFFS) + w1 * (P1 + IF_INTERNAL_OFFS) + round + (offset << (shift - 1))) >> shift));
}

static inline Pel weightUnidirY(Int w0, Short P0, Int round, Int shift, Int offset)
{
    return ClipY(((w0 * (P0 + IF_INTERNAL_OFFS) + round) >> shift) + offset);
}

static inline Pel weightUnidirC(Int w0, Short P0, Int round, Int shift, Int offset)
{
    return ClipC(((w0 * (P0 + IF_INTERNAL_OFFS) + round) >> shift) + offset);
}

// ====================================================================================================================
// Class definition
// ====================================================================================================================
TComWeightPrediction::TComWeightPrediction()
{}

/** weighted averaging for bi-pred
 * \param TComYuv* srcYuv0
 * \param TComYuv* srcYuv1
 * \param iPartUnitIdx
 * \param width
 * \param height
 * \param wpScalingParam *wp0
 * \param wpScalingParam *wp1
 * \param TComYuv* rpcYuvDst
 * \returns Void
 */
Void TComWeightPrediction::addWeightBi(TComYuv* srcYuv0, TComYuv* srcYuv1, UInt iPartUnitIdx, UInt width, UInt height, wpScalingParam *wp0, wpScalingParam *wp1, TComYuv* rpcYuvDst, Bool bRound)
{
    Int x, y;

    Pel* pSrcY0  = srcYuv0->getLumaAddr(iPartUnitIdx);
    Pel* pSrcU0  = srcYuv0->getCbAddr(iPartUnitIdx);
    Pel* pSrcV0  = srcYuv0->getCrAddr(iPartUnitIdx);

    Pel* pSrcY1  = srcYuv1->getLumaAddr(iPartUnitIdx);
    Pel* pSrcU1  = srcYuv1->getCbAddr(iPartUnitIdx);
    Pel* pSrcV1  = srcYuv1->getCrAddr(iPartUnitIdx);

    Pel* pDstY   = rpcYuvDst->getLumaAddr(iPartUnitIdx);
    Pel* dstU   = rpcYuvDst->getCbAddr(iPartUnitIdx);
    Pel* dstV   = rpcYuvDst->getCrAddr(iPartUnitIdx);

    // Luma : --------------------------------------------
    Int w0      = wp0[0].w;
    Int offset  = wp0[0].offset;
    Int shiftNum = IF_INTERNAL_PREC - g_bitDepthY;
    Int shift   = wp0[0].shift + shiftNum;
    Int round   = shift ? (1 << (shift - 1)) * bRound : 0;
    Int w1      = wp1[0].w;

    UInt  iSrc0Stride = srcYuv0->getStride();
    UInt  iSrc1Stride = srcYuv1->getStride();
    UInt  dststride  = rpcYuvDst->getStride();

    for (y = height - 1; y >= 0; y--)
    {
        for (x = width - 1; x >= 0; )
        {
            // note: luma min width is 4
            pDstY[x] = weightBidirY(w0, pSrcY0[x], w1, pSrcY1[x], round, shift, offset);
            x--;
            pDstY[x] = weightBidirY(w0, pSrcY0[x], w1, pSrcY1[x], round, shift, offset);
            x--;
            pDstY[x] = weightBidirY(w0, pSrcY0[x], w1, pSrcY1[x], round, shift, offset);
            x--;
            pDstY[x] = weightBidirY(w0, pSrcY0[x], w1, pSrcY1[x], round, shift, offset);
            x--;
        }

        pSrcY0 += iSrc0Stride;
        pSrcY1 += iSrc1Stride;
        pDstY  += dststride;
    }

    // Chroma U : --------------------------------------------
    w0      = wp0[1].w;
    offset  = wp0[1].offset;
    shiftNum = IF_INTERNAL_PREC - g_bitDepthC;
    shift   = wp0[1].shift + shiftNum;
    round   = shift ? (1 << (shift - 1)) : 0;
    w1      = wp1[1].w;

    iSrc0Stride = srcYuv0->getCStride();
    iSrc1Stride = srcYuv1->getCStride();
    dststride  = rpcYuvDst->getCStride();

    width  >>= 1;
    height >>= 1;

    for (y = height - 1; y >= 0; y--)
    {
        for (x = width - 1; x >= 0; )
        {
            // note: chroma min width is 2
            dstU[x] = weightBidirC(w0, pSrcU0[x], w1, pSrcU1[x], round, shift, offset);
            x--;
            dstU[x] = weightBidirC(w0, pSrcU0[x], w1, pSrcU1[x], round, shift, offset);
            x--;
        }

        pSrcU0 += iSrc0Stride;
        pSrcU1 += iSrc1Stride;
        dstU  += dststride;
    }

    // Chroma V : --------------------------------------------
    w0      = wp0[2].w;
    offset  = wp0[2].offset;
    shift   = wp0[2].shift + shiftNum;
    round   = shift ? (1 << (shift - 1)) : 0;
    w1      = wp1[2].w;

    for (y = height - 1; y >= 0; y--)
    {
        for (x = width - 1; x >= 0; )
        {
            // note: chroma min width is 2
            dstV[x] = weightBidirC(w0, pSrcV0[x], w1, pSrcV1[x], round, shift, offset);
            x--;
            dstV[x] = weightBidirC(w0, pSrcV0[x], w1, pSrcV1[x], round, shift, offset);
            x--;
        }

        pSrcV0 += iSrc0Stride;
        pSrcV1 += iSrc1Stride;
        dstV  += dststride;
    }
}

/** weighted averaging for bi-pred
 * \param TShortYuv* srcYuv0
 * \param TShortYuv* srcYuv1
 * \param iPartUnitIdx
 * \param width
 * \param height
 * \param wpScalingParam *wp0
 * \param wpScalingParam *wp1
 * \param TComYuv* rpcYuvDst
 * \returns Void
 */
Void TComWeightPrediction::addWeightBi(TShortYUV* srcYuv0, TShortYUV* srcYuv1, UInt iPartUnitIdx, UInt width, UInt height, wpScalingParam *wp0, wpScalingParam *wp1, TComYuv* rpcYuvDst, Bool bRound)
{
    Int x, y;

    Short* pSrcY0  = srcYuv0->getLumaAddr(iPartUnitIdx);
    Short* pSrcU0  = srcYuv0->getCbAddr(iPartUnitIdx);
    Short* pSrcV0  = srcYuv0->getCrAddr(iPartUnitIdx);

    Short* pSrcY1  = srcYuv1->getLumaAddr(iPartUnitIdx);
    Short* pSrcU1  = srcYuv1->getCbAddr(iPartUnitIdx);
    Short* pSrcV1  = srcYuv1->getCrAddr(iPartUnitIdx);

    Pel* pDstY   = rpcYuvDst->getLumaAddr(iPartUnitIdx);
    Pel* dstU   = rpcYuvDst->getCbAddr(iPartUnitIdx);
    Pel* dstV   = rpcYuvDst->getCrAddr(iPartUnitIdx);

    // Luma : --------------------------------------------
    Int w0      = wp0[0].w;
    Int offset  = wp0[0].offset;
    Int shiftNum = IF_INTERNAL_PREC - g_bitDepthY;
    Int shift   = wp0[0].shift + shiftNum;
    Int round   = shift ? (1 << (shift - 1)) * bRound : 0;
    Int w1      = wp1[0].w;

    UInt  iSrc0Stride = srcYuv0->width;
    UInt  iSrc1Stride = srcYuv1->width;
    UInt  dststride  = rpcYuvDst->getStride();

    for (y = height - 1; y >= 0; y--)
    {
        for (x = width - 1; x >= 0; )
        {
            // note: luma min width is 4
            pDstY[x] = weightBidirY(w0, pSrcY0[x], w1, pSrcY1[x], round, shift, offset);
            x--;
            pDstY[x] = weightBidirY(w0, pSrcY0[x], w1, pSrcY1[x], round, shift, offset);
            x--;
            pDstY[x] = weightBidirY(w0, pSrcY0[x], w1, pSrcY1[x], round, shift, offset);
            x--;
            pDstY[x] = weightBidirY(w0, pSrcY0[x], w1, pSrcY1[x], round, shift, offset);
            x--;
        }

        pSrcY0 += iSrc0Stride;
        pSrcY1 += iSrc1Stride;
        pDstY  += dststride;
    }

    // Chroma U : --------------------------------------------
    w0      = wp0[1].w;
    offset  = wp0[1].offset;
    shiftNum = IF_INTERNAL_PREC - g_bitDepthC;
    shift   = wp0[1].shift + shiftNum;
    round   = shift ? (1 << (shift - 1)) : 0;
    w1      = wp1[1].w;

    iSrc0Stride = srcYuv0->Cwidth;
    iSrc1Stride = srcYuv1->Cwidth;
    dststride  = rpcYuvDst->getCStride();

    width  >>= 1;
    height >>= 1;

    for (y = height - 1; y >= 0; y--)
    {
        for (x = width - 1; x >= 0; )
        {
            // note: chroma min width is 2
            dstU[x] = weightBidirC(w0, pSrcU0[x], w1, pSrcU1[x], round, shift, offset);
            x--;
            dstU[x] = weightBidirC(w0, pSrcU0[x], w1, pSrcU1[x], round, shift, offset);
            x--;
        }

        pSrcU0 += iSrc0Stride;
        pSrcU1 += iSrc1Stride;
        dstU  += dststride;
    }

    // Chroma V : --------------------------------------------
    w0      = wp0[2].w;
    offset  = wp0[2].offset;
    shift   = wp0[2].shift + shiftNum;
    round   = shift ? (1 << (shift - 1)) : 0;
    w1      = wp1[2].w;

    for (y = height - 1; y >= 0; y--)
    {
        for (x = width - 1; x >= 0; )
        {
            // note: chroma min width is 2
            dstV[x] = weightBidirC(w0, pSrcV0[x], w1, pSrcV1[x], round, shift, offset);
            x--;
            dstV[x] = weightBidirC(w0, pSrcV0[x], w1, pSrcV1[x], round, shift, offset);
            x--;
        }

        pSrcV0 += iSrc0Stride;
        pSrcV1 += iSrc1Stride;
        dstV  += dststride;
    }
}

/** weighted averaging for uni-pred
 * \param TComYuv* srcYuv0
 * \param iPartUnitIdx
 * \param width
 * \param height
 * \param wpScalingParam *wp0
 * \param TComYuv* rpcYuvDst
 * \returns Void
 */
Void TComWeightPrediction::addWeightUni(TComYuv* srcYuv0, UInt iPartUnitIdx, UInt width, UInt height, wpScalingParam *wp0, TComYuv* rpcYuvDst)
{
    Int x, y;

    Pel* pSrcY0  = srcYuv0->getLumaAddr(iPartUnitIdx);
    Pel* pSrcU0  = srcYuv0->getCbAddr(iPartUnitIdx);
    Pel* pSrcV0  = srcYuv0->getCrAddr(iPartUnitIdx);

    Pel* pDstY   = rpcYuvDst->getLumaAddr(iPartUnitIdx);
    Pel* dstU   = rpcYuvDst->getCbAddr(iPartUnitIdx);
    Pel* dstV   = rpcYuvDst->getCrAddr(iPartUnitIdx);

    // Luma : --------------------------------------------
    Int w0      = wp0[0].w;
    Int offset  = wp0[0].offset;
    Int shiftNum = IF_INTERNAL_PREC - g_bitDepthY;
    Int shift   = wp0[0].shift + shiftNum;
    Int round   = shift ? (1 << (shift - 1)) : 0;
    UInt  iSrc0Stride = srcYuv0->getStride();
    UInt  dststride  = rpcYuvDst->getStride();

    for (y = height - 1; y >= 0; y--)
    {
        for (x = width - 1; x >= 0; )
        {
            // note: luma min width is 4
            pDstY[x] = weightUnidirY(w0, pSrcY0[x], round, shift, offset);
            x--;
            pDstY[x] = weightUnidirY(w0, pSrcY0[x], round, shift, offset);
            x--;
            pDstY[x] = weightUnidirY(w0, pSrcY0[x], round, shift, offset);
            x--;
            pDstY[x] = weightUnidirY(w0, pSrcY0[x], round, shift, offset);
            x--;
        }

        pSrcY0 += iSrc0Stride;
        pDstY  += dststride;
    }

    // Chroma U : --------------------------------------------
    w0      = wp0[1].w;
    offset  = wp0[1].offset;
    shiftNum = IF_INTERNAL_PREC - g_bitDepthC;
    shift   = wp0[1].shift + shiftNum;
    round   = shift ? (1 << (shift - 1)) : 0;

    iSrc0Stride = srcYuv0->getCStride();
    dststride  = rpcYuvDst->getCStride();

    width  >>= 1;
    height >>= 1;

    for (y = height - 1; y >= 0; y--)
    {
        for (x = width - 1; x >= 0; )
        {
            // note: chroma min width is 2
            dstU[x] = weightUnidirC(w0, pSrcU0[x], round, shift, offset);
            x--;
            dstU[x] = weightUnidirC(w0, pSrcU0[x], round, shift, offset);
            x--;
        }

        pSrcU0 += iSrc0Stride;
        dstU  += dststride;
    }

    // Chroma V : --------------------------------------------
    w0      = wp0[2].w;
    offset  = wp0[2].offset;
    shift   = wp0[2].shift + shiftNum;
    round   = shift ? (1 << (shift - 1)) : 0;

    for (y = height - 1; y >= 0; y--)
    {
        for (x = width - 1; x >= 0; )
        {
            // note: chroma min width is 2
            dstV[x] = weightUnidirC(w0, pSrcV0[x], round, shift, offset);
            x--;
            dstV[x] = weightUnidirC(w0, pSrcV0[x], round, shift, offset);
            x--;
        }

        pSrcV0 += iSrc0Stride;
        dstV  += dststride;
    }
}

/** weighted averaging for uni-pred
 * \param TShortYUV* srcYuv0
 * \param iPartUnitIdx
 * \param width
 * \param height
 * \param wpScalingParam *wp0
 * \param TComYuv* rpcYuvDst
 * \returns Void
 */

Void TComWeightPrediction::addWeightUni(TShortYUV* srcYuv0, UInt iPartUnitIdx, UInt width, UInt height, wpScalingParam *wp0, TComYuv* rpcYuvDst)
{
    Short* pSrcY0  = srcYuv0->getLumaAddr(iPartUnitIdx);
    Short* pSrcU0  = srcYuv0->getCbAddr(iPartUnitIdx);
    Short* pSrcV0  = srcYuv0->getCrAddr(iPartUnitIdx);

    Pel* dstY   = rpcYuvDst->getLumaAddr(iPartUnitIdx);
    Pel* dstU   = rpcYuvDst->getCbAddr(iPartUnitIdx);
    Pel* dstV   = rpcYuvDst->getCrAddr(iPartUnitIdx);

    // Luma : --------------------------------------------
    Int w0      = wp0[0].w;
    Int offset  = wp0[0].offset;
    Int shiftNum = IF_INTERNAL_PREC - g_bitDepthY;
    Int shift   = wp0[0].shift + shiftNum;
    Int round   = shift ? (1 << (shift - 1)) : 0;
    UInt  srcStride = srcYuv0->width;
    UInt  dstStride  = rpcYuvDst->getStride();

   x265::primitives.weightpUni(pSrcY0, dstY, srcStride, dstStride, width, height, w0, round, shift, offset, g_bitDepthY);

    // Chroma U : --------------------------------------------
    w0      = wp0[1].w;
    offset  = wp0[1].offset;
    shiftNum = IF_INTERNAL_PREC - g_bitDepthC;
    shift   = wp0[1].shift + shiftNum;
    round   = shift ? (1 << (shift - 1)) : 0;

    srcStride = srcYuv0->Cwidth;
    dstStride  = rpcYuvDst->getCStride();

    width  >>= 1;
    height >>= 1;

    x265::primitives.weightpUni(pSrcU0, dstU, srcStride, dstStride, width, height, w0, round, shift, offset, g_bitDepthC);

    // Chroma V : --------------------------------------------
    w0      = wp0[2].w;
    offset  = wp0[2].offset;
    shift   = wp0[2].shift + shiftNum;
    round   = shift ? (1 << (shift - 1)) : 0;

    x265::primitives.weightpUni(pSrcU0, dstV, srcStride, dstStride, width, height, w0, round, shift, offset, g_bitDepthC);

}

//=======================================================
//  getWpScaling()
//=======================================================

/** derivation of wp tables
 * \param TComDataCU* cu
 * \param iRefIdx0
 * \param iRefIdx1
 * \param wpScalingParam *&wp0
 * \param wpScalingParam *&wp1
 * \param ibdi
 * \returns Void
 */
Void TComWeightPrediction::getWpScaling(TComDataCU* cu, Int iRefIdx0, Int iRefIdx1, wpScalingParam *&wp0, wpScalingParam *&wp1)
{
    TComSlice*      pcSlice = cu->getSlice();
    const TComPPS*  pps     = cu->getSlice()->getPPS();
    Bool            wpBiPred = pps->getWPBiPred();
    wpScalingParam* pwp;
    Bool            bBiDir   = (iRefIdx0 >= 0 && iRefIdx1 >= 0);
    Bool            bUniDir  = !bBiDir;

    if (bUniDir || wpBiPred)
    { // explicit --------------------
        if (iRefIdx0 >= 0)
        {
            pcSlice->getWpScaling(REF_PIC_LIST_0, iRefIdx0, wp0);
        }
        if (iRefIdx1 >= 0)
        {
            pcSlice->getWpScaling(REF_PIC_LIST_1, iRefIdx1, wp1);
        }
    }
    else
    {
        assert(0);
    }

    if (iRefIdx0 < 0)
    {
        wp0 = NULL;
    }
    if (iRefIdx1 < 0)
    {
        wp1 = NULL;
    }

    if (bBiDir)
    { // Bi-Dir case
        for (Int yuv = 0; yuv < 3; yuv++)
        {
            Int bitDepth = yuv ? g_bitDepthC : g_bitDepthY;
            wp0[yuv].w      = wp0[yuv].iWeight;
            wp0[yuv].o      = wp0[yuv].iOffset * (1 << (bitDepth - 8));
            wp1[yuv].w      = wp1[yuv].iWeight;
            wp1[yuv].o      = wp1[yuv].iOffset * (1 << (bitDepth - 8));
            wp0[yuv].offset = wp0[yuv].o + wp1[yuv].o;
            wp0[yuv].shift  = wp0[yuv].uiLog2WeightDenom + 1;
            wp0[yuv].round  = (1 << wp0[yuv].uiLog2WeightDenom);
            wp1[yuv].offset = wp0[yuv].offset;
            wp1[yuv].shift  = wp0[yuv].shift;
            wp1[yuv].round  = wp0[yuv].round;
        }
    }
    else
    { // Unidir
        pwp = (iRefIdx0 >= 0) ? wp0 : wp1;
        for (Int yuv = 0; yuv < 3; yuv++)
        {
            Int bitDepth = yuv ? g_bitDepthC : g_bitDepthY;
            pwp[yuv].w      = pwp[yuv].iWeight;
            pwp[yuv].offset = pwp[yuv].iOffset * (1 << (bitDepth - 8));
            pwp[yuv].shift  = pwp[yuv].uiLog2WeightDenom;
            pwp[yuv].round  = (pwp[yuv].uiLog2WeightDenom >= 1) ? (1 << (pwp[yuv].uiLog2WeightDenom - 1)) : (0);
        }
    }
}

/** weighted prediction for bi-pred
 * \param TComDataCU* cu
 * \param TComYuv* srcYuv0
 * \param TComYuv* srcYuv1
 * \param iRefIdx0
 * \param iRefIdx1
 * \param partIdx
 * \param width
 * \param height
 * \param TComYuv* rpcYuvDst
 * \returns Void
 */
Void TComWeightPrediction::xWeightedPredictionBi(TComDataCU* cu, TComYuv* srcYuv0, TComYuv* srcYuv1, Int iRefIdx0, Int iRefIdx1, UInt partIdx, Int width, Int height, TComYuv* rpcYuvDst)
{
    wpScalingParam  *pwp0, *pwp1;
    const TComPPS   *pps = cu->getSlice()->getPPS();

    assert(pps->getWPBiPred());

    getWpScaling(cu, iRefIdx0, iRefIdx1, pwp0, pwp1);

    if (iRefIdx0 >= 0 && iRefIdx1 >= 0)
    {
        addWeightBi(srcYuv0, srcYuv1, partIdx, width, height, pwp0, pwp1, rpcYuvDst);
    }
    else if (iRefIdx0 >= 0 && iRefIdx1 <  0)
    {
        addWeightUni(srcYuv0, partIdx, width, height, pwp0, rpcYuvDst);
    }
    else if (iRefIdx0 <  0 && iRefIdx1 >= 0)
    {
        addWeightUni(srcYuv1, partIdx, width, height, pwp1, rpcYuvDst);
    }
    else
    {
        assert(0);
    }
}

/** weighted prediction for bi-pred
 * \param TComDataCU* cu
 * \param TShortYuv* srcYuv0
 * \param TShortYuv* srcYuv1
 * \param iRefIdx0
 * \param iRefIdx1
 * \param partIdx
 * \param width
 * \param height
 * \param TComYuv* rpcYuvDst
 * \returns Void
 */
Void TComWeightPrediction::xWeightedPredictionBi(TComDataCU* cu, TShortYUV* srcYuv0, TShortYUV* srcYuv1, Int iRefIdx0, Int iRefIdx1, UInt partIdx, Int width, Int height, TComYuv* rpcYuvDst)
{
    wpScalingParam  *pwp0, *pwp1;
    const TComPPS   *pps = cu->getSlice()->getPPS();

    assert(pps->getWPBiPred());

    getWpScaling(cu, iRefIdx0, iRefIdx1, pwp0, pwp1);

    if (iRefIdx0 >= 0 && iRefIdx1 >= 0)
    {
        addWeightBi(srcYuv0, srcYuv1, partIdx, width, height, pwp0, pwp1, rpcYuvDst);
    }
    else if (iRefIdx0 >= 0 && iRefIdx1 <  0)
    {
        addWeightUni(srcYuv0, partIdx, width, height, pwp0, rpcYuvDst);
    }
    else if (iRefIdx0 <  0 && iRefIdx1 >= 0)
    {
        addWeightUni(srcYuv1, partIdx, width, height, pwp1, rpcYuvDst);
    }
    else
    {
        assert(0);
    }
}

/** weighted prediction for uni-pred
 * \param TComDataCU* cu
 * \param TComYuv* pcYuvSrc
 * \param partAddr
 * \param width
 * \param height
 * \param picList
 * \param TComYuv*& rpcYuvPred
 * \param partIdx
 * \param iRefIdx
 * \returns Void
 */
Void TComWeightPrediction::xWeightedPredictionUni(TComDataCU* cu, TComYuv* pcYuvSrc, UInt partAddr, Int width, Int height, RefPicList picList, TComYuv*& rpcYuvPred, Int iRefIdx)
{
    wpScalingParam  *pwp, *pwpTmp;

    if (iRefIdx < 0)
    {
        iRefIdx   = cu->getCUMvField(picList)->getRefIdx(partAddr);
    }
    assert(iRefIdx >= 0);

    if (picList == REF_PIC_LIST_0)
    {
        getWpScaling(cu, iRefIdx, -1, pwp, pwpTmp);
    }
    else
    {
        getWpScaling(cu, -1, iRefIdx, pwpTmp, pwp);
    }
    addWeightUni(pcYuvSrc, partAddr, width, height, pwp, rpcYuvPred);
}

/** weighted prediction for uni-pred
 * \param TComDataCU* cu
 * \param TShortYuv* pcYuvSrc
 * \param partAddr
 * \param width
 * \param height
 * \param picList
 * \param TComYuv*& rpcYuvPred
 * \param partIdx
 * \param iRefIdx
 * \returns Void
 */
Void TComWeightPrediction::xWeightedPredictionUni(TComDataCU* cu, TShortYUV* pcYuvSrc, UInt partAddr, Int width, Int height, RefPicList picList, TComYuv*& rpcYuvPred, Int iRefIdx)
{
    wpScalingParam  *pwp, *pwpTmp;

    if (iRefIdx < 0)
    {
        iRefIdx   = cu->getCUMvField(picList)->getRefIdx(partAddr);
    }
    assert(iRefIdx >= 0);

    if (picList == REF_PIC_LIST_0)
    {
        getWpScaling(cu, iRefIdx, -1, pwp, pwpTmp);
    }
    else
    {
        getWpScaling(cu, -1, iRefIdx, pwpTmp, pwp);
    }
    addWeightUni(pcYuvSrc, partAddr, width, height, pwp, rpcYuvPred);
}
