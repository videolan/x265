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

using namespace x265;

static inline Pel weightBidirY(Int w0, short P0, Int w1, short P1, Int round, Int shift, Int offset)
{
    return ClipY(((w0 * (P0 + IF_INTERNAL_OFFS) + w1 * (P1 + IF_INTERNAL_OFFS) + round + (offset << (shift - 1))) >> shift));
}

static inline Pel weightBidirC(Int w0, short P0, Int w1, short P1, Int round, Int shift, Int offset)
{
    return ClipC(((w0 * (P0 + IF_INTERNAL_OFFS) + w1 * (P1 + IF_INTERNAL_OFFS) + round + (offset << (shift - 1))) >> shift));
}

static inline Pel weightUnidirY(Int w0, short P0, Int round, Int shift, Int offset)
{
    return ClipY(((w0 * (P0 + IF_INTERNAL_OFFS) + round) >> shift) + offset);
}

static inline Pel weightUnidirC(Int w0, short P0, Int round, Int shift, Int offset)
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
 * \param partUnitIdx
 * \param width
 * \param height
 * \param wpScalingParam *wp0
 * \param wpScalingParam *wp1
 * \param TComYuv* outDstYuv
 * \returns void
 */
void TComWeightPrediction::addWeightBi(TComYuv* srcYuv0, TComYuv* srcYuv1, UInt partUnitIdx, UInt width, UInt height, wpScalingParam *wp0, wpScalingParam *wp1, TComYuv* outDstYuv, Bool bRound)
{
    Int x, y;

    Pel* srcY0  = srcYuv0->getLumaAddr(partUnitIdx);
    Pel* srcU0  = srcYuv0->getCbAddr(partUnitIdx);
    Pel* srcV0  = srcYuv0->getCrAddr(partUnitIdx);

    Pel* srcY1  = srcYuv1->getLumaAddr(partUnitIdx);
    Pel* srcU1  = srcYuv1->getCbAddr(partUnitIdx);
    Pel* srcV1  = srcYuv1->getCrAddr(partUnitIdx);

    Pel* pDstY   = outDstYuv->getLumaAddr(partUnitIdx);
    Pel* dstU   = outDstYuv->getCbAddr(partUnitIdx);
    Pel* dstV   = outDstYuv->getCrAddr(partUnitIdx);

    // Luma : --------------------------------------------
    Int w0      = wp0[0].w;
    Int offset  = wp0[0].offset;
    Int shiftNum = IF_INTERNAL_PREC - X265_DEPTH;
    Int shift   = wp0[0].shift + shiftNum;
    Int round   = shift ? (1 << (shift - 1)) * bRound : 0;
    Int w1      = wp1[0].w;

    UInt  src0Stride = srcYuv0->getStride();
    UInt  src1Stride = srcYuv1->getStride();
    UInt  dststride  = outDstYuv->getStride();

    for (y = height - 1; y >= 0; y--)
    {
        for (x = width - 1; x >= 0; )
        {
            // note: luma min width is 4
            pDstY[x] = weightBidirY(w0, srcY0[x], w1, srcY1[x], round, shift, offset);
            x--;
            pDstY[x] = weightBidirY(w0, srcY0[x], w1, srcY1[x], round, shift, offset);
            x--;
            pDstY[x] = weightBidirY(w0, srcY0[x], w1, srcY1[x], round, shift, offset);
            x--;
            pDstY[x] = weightBidirY(w0, srcY0[x], w1, srcY1[x], round, shift, offset);
            x--;
        }

        srcY0 += src0Stride;
        srcY1 += src1Stride;
        pDstY  += dststride;
    }

    // Chroma U : --------------------------------------------
    w0      = wp0[1].w;
    offset  = wp0[1].offset;
    shiftNum = IF_INTERNAL_PREC - X265_DEPTH;
    shift   = wp0[1].shift + shiftNum;
    round   = shift ? (1 << (shift - 1)) : 0;
    w1      = wp1[1].w;

    src0Stride = srcYuv0->getCStride();
    src1Stride = srcYuv1->getCStride();
    dststride  = outDstYuv->getCStride();

    width  >>= 1;
    height >>= 1;

    for (y = height - 1; y >= 0; y--)
    {
        for (x = width - 1; x >= 0; )
        {
            // note: chroma min width is 2
            dstU[x] = weightBidirC(w0, srcU0[x], w1, srcU1[x], round, shift, offset);
            x--;
            dstU[x] = weightBidirC(w0, srcU0[x], w1, srcU1[x], round, shift, offset);
            x--;
        }

        srcU0 += src0Stride;
        srcU1 += src1Stride;
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
            dstV[x] = weightBidirC(w0, srcV0[x], w1, srcV1[x], round, shift, offset);
            x--;
            dstV[x] = weightBidirC(w0, srcV0[x], w1, srcV1[x], round, shift, offset);
            x--;
        }

        srcV0 += src0Stride;
        srcV1 += src1Stride;
        dstV  += dststride;
    }
}

/** weighted averaging for bi-pred
 * \param TShortYuv* srcYuv0
 * \param TShortYuv* srcYuv1
 * \param partUnitIdx
 * \param width
 * \param height
 * \param wpScalingParam *wp0
 * \param wpScalingParam *wp1
 * \param TComYuv* outDstYuv
 * \returns void
 */
void TComWeightPrediction::addWeightBi(TShortYUV* srcYuv0, TShortYUV* srcYuv1, UInt partUnitIdx, UInt width, UInt height, wpScalingParam *wp0, wpScalingParam *wp1, TComYuv* outDstYuv, Bool bRound)
{
    Int x, y;

    short* srcY0  = srcYuv0->getLumaAddr(partUnitIdx);
    short* srcU0  = srcYuv0->getCbAddr(partUnitIdx);
    short* srcV0  = srcYuv0->getCrAddr(partUnitIdx);

    short* srcY1  = srcYuv1->getLumaAddr(partUnitIdx);
    short* srcU1  = srcYuv1->getCbAddr(partUnitIdx);
    short* srcV1  = srcYuv1->getCrAddr(partUnitIdx);

    Pel* dstY   = outDstYuv->getLumaAddr(partUnitIdx);
    Pel* dstU   = outDstYuv->getCbAddr(partUnitIdx);
    Pel* dstV   = outDstYuv->getCrAddr(partUnitIdx);

    // Luma : --------------------------------------------
    Int w0      = wp0[0].w;
    Int offset  = wp0[0].offset;
    Int shiftNum = IF_INTERNAL_PREC - X265_DEPTH;
    Int shift   = wp0[0].shift + shiftNum;
    Int round   = shift ? (1 << (shift - 1)) * bRound : 0;
    Int w1      = wp1[0].w;

    UInt  src0Stride = srcYuv0->m_width;
    UInt  src1Stride = srcYuv1->m_width;
    UInt  dststride  = outDstYuv->getStride();

    for (y = height - 1; y >= 0; y--)
    {
        for (x = width - 1; x >= 0; )
        {
            // note: luma min width is 4
            dstY[x] = weightBidirY(w0, srcY0[x], w1, srcY1[x], round, shift, offset);
            x--;
            dstY[x] = weightBidirY(w0, srcY0[x], w1, srcY1[x], round, shift, offset);
            x--;
            dstY[x] = weightBidirY(w0, srcY0[x], w1, srcY1[x], round, shift, offset);
            x--;
            dstY[x] = weightBidirY(w0, srcY0[x], w1, srcY1[x], round, shift, offset);
            x--;
        }

        srcY0 += src0Stride;
        srcY1 += src1Stride;
        dstY  += dststride;
    }

    // Chroma U : --------------------------------------------
    w0      = wp0[1].w;
    offset  = wp0[1].offset;
    shiftNum = IF_INTERNAL_PREC - X265_DEPTH;
    shift   = wp0[1].shift + shiftNum;
    round   = shift ? (1 << (shift - 1)) : 0;
    w1      = wp1[1].w;

    src0Stride = srcYuv0->m_cwidth;
    src1Stride = srcYuv1->m_cwidth;
    dststride  = outDstYuv->getCStride();

    width  >>= 1;
    height >>= 1;

    for (y = height - 1; y >= 0; y--)
    {
        for (x = width - 1; x >= 0; )
        {
            // note: chroma min width is 2
            dstU[x] = weightBidirC(w0, srcU0[x], w1, srcU1[x], round, shift, offset);
            x--;
            dstU[x] = weightBidirC(w0, srcU0[x], w1, srcU1[x], round, shift, offset);
            x--;
        }

        srcU0 += src0Stride;
        srcU1 += src1Stride;
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
            dstV[x] = weightBidirC(w0, srcV0[x], w1, srcV1[x], round, shift, offset);
            x--;
            dstV[x] = weightBidirC(w0, srcV0[x], w1, srcV1[x], round, shift, offset);
            x--;
        }

        srcV0 += src0Stride;
        srcV1 += src1Stride;
        dstV  += dststride;
    }
}

/** weighted averaging for uni-pred
 * \param TComYuv* srcYuv0
 * \param partUnitIdx
 * \param width
 * \param height
 * \param wpScalingParam *wp0
 * \param TComYuv* outDstYuv
 * \returns void
 */
void TComWeightPrediction::addWeightUni(TComYuv* srcYuv0, UInt partUnitIdx, UInt width, UInt height, wpScalingParam *wp0, TComYuv* outDstYuv)
{
    Int x, y;

    Pel* srcY0  = srcYuv0->getLumaAddr(partUnitIdx);
    Pel* srcU0  = srcYuv0->getCbAddr(partUnitIdx);
    Pel* srcV0  = srcYuv0->getCrAddr(partUnitIdx);

    Pel* dstY   = outDstYuv->getLumaAddr(partUnitIdx);
    Pel* dstU   = outDstYuv->getCbAddr(partUnitIdx);
    Pel* dstV   = outDstYuv->getCrAddr(partUnitIdx);

    // Luma : --------------------------------------------
    Int w0      = wp0[0].w;
    Int offset  = wp0[0].offset;
    Int shiftNum = IF_INTERNAL_PREC - X265_DEPTH;
    Int shift   = wp0[0].shift + shiftNum;
    Int round   = shift ? (1 << (shift - 1)) : 0;
    UInt src0Stride = srcYuv0->getStride();
    UInt dststride  = outDstYuv->getStride();

    for (y = height - 1; y >= 0; y--)
    {
        for (x = width - 1; x >= 0; )
        {
            // note: luma min width is 4
            dstY[x] = weightUnidirY(w0, srcY0[x], round, shift, offset);
            x--;
            dstY[x] = weightUnidirY(w0, srcY0[x], round, shift, offset);
            x--;
            dstY[x] = weightUnidirY(w0, srcY0[x], round, shift, offset);
            x--;
            dstY[x] = weightUnidirY(w0, srcY0[x], round, shift, offset);
            x--;
        }

        srcY0 += src0Stride;
        dstY  += dststride;
    }

    // Chroma U : --------------------------------------------
    w0      = wp0[1].w;
    offset  = wp0[1].offset;
    shiftNum = IF_INTERNAL_PREC - X265_DEPTH;
    shift   = wp0[1].shift + shiftNum;
    round   = shift ? (1 << (shift - 1)) : 0;

    src0Stride = srcYuv0->getCStride();
    dststride  = outDstYuv->getCStride();

    width  >>= 1;
    height >>= 1;

    for (y = height - 1; y >= 0; y--)
    {
        for (x = width - 1; x >= 0; )
        {
            // note: chroma min width is 2
            dstU[x] = weightUnidirC(w0, srcU0[x], round, shift, offset);
            x--;
            dstU[x] = weightUnidirC(w0, srcU0[x], round, shift, offset);
            x--;
        }

        srcU0 += src0Stride;
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
            dstV[x] = weightUnidirC(w0, srcV0[x], round, shift, offset);
            x--;
            dstV[x] = weightUnidirC(w0, srcV0[x], round, shift, offset);
            x--;
        }

        srcV0 += src0Stride;
        dstV  += dststride;
    }
}

/** weighted averaging for uni-pred
 * \param TShortYUV* srcYuv0
 * \param partUnitIdx
 * \param width
 * \param height
 * \param wpScalingParam *wp0
 * \param TComYuv* outDstYuv
 * \returns void
 */

void TComWeightPrediction::addWeightUni(TShortYUV* srcYuv0, UInt partUnitIdx, UInt width, UInt height, wpScalingParam *wp0, TComYuv* outDstYuv, bool justChroma)
{
    short* srcY0  = srcYuv0->getLumaAddr(partUnitIdx);
    short* srcU0  = srcYuv0->getCbAddr(partUnitIdx);
    short* srcV0  = srcYuv0->getCrAddr(partUnitIdx);

    Pel* dstY   = outDstYuv->getLumaAddr(partUnitIdx);
    Pel* dstU   = outDstYuv->getCbAddr(partUnitIdx);
    Pel* dstV   = outDstYuv->getCrAddr(partUnitIdx);


    Int w0, offset, shiftNum, shift, round;
    UInt srcStride, dstStride;

    if(!justChroma)
    {
        // Luma : --------------------------------------------
        w0      = wp0[0].w;
        offset  = wp0[0].offset;
        shiftNum = IF_INTERNAL_PREC - X265_DEPTH;
        shift   = wp0[0].shift + shiftNum;
        round   = shift ? (1 << (shift - 1)) : 0;
        srcStride = srcYuv0->m_width;
        dstStride  = outDstYuv->getStride();

        x265::primitives.weightpUni(srcY0, dstY, srcStride, dstStride, width, height, w0, round, shift, offset);
    }

    // Chroma U : --------------------------------------------
    w0      = wp0[1].w;
    offset  = wp0[1].offset;
    shiftNum = IF_INTERNAL_PREC - X265_DEPTH;
    shift   = wp0[1].shift + shiftNum;
    round   = shift ? (1 << (shift - 1)) : 0;

    srcStride = srcYuv0->m_cwidth;
    dstStride  = outDstYuv->getCStride();

    width  >>= 1;
    height >>= 1;

    x265::primitives.weightpUni(srcU0, dstU, srcStride, dstStride, width, height, w0, round, shift, offset);

    // Chroma V : --------------------------------------------
    w0      = wp0[2].w;
    offset  = wp0[2].offset;
    shift   = wp0[2].shift + shiftNum;
    round   = shift ? (1 << (shift - 1)) : 0;

    x265::primitives.weightpUni(srcV0, dstV, srcStride, dstStride, width, height, w0, round, shift, offset);
}

//=======================================================
//  getWpScaling()
//=======================================================

/** derivation of wp tables
 * \param TComDataCU* cu
 * \param refIdx0
 * \param refIdx1
 * \param wpScalingParam *&wp0
 * \param wpScalingParam *&wp1
 * \param ibdi
 * \returns void
 */
void TComWeightPrediction::getWpScaling(TComDataCU* cu, Int refIdx0, Int refIdx1, wpScalingParam *&wp0, wpScalingParam *&wp1)
{
    TComSlice*      slice = cu->getSlice();
    const TComPPS*  pps     = cu->getSlice()->getPPS();
    Bool            wpBiPred = pps->getWPBiPred();
    wpScalingParam* pwp;
    Bool            bBiDir   = (refIdx0 >= 0 && refIdx1 >= 0);
    Bool            bUniDir  = !bBiDir;

    if (bUniDir || wpBiPred)
    { // explicit --------------------
        if (refIdx0 >= 0)
        {
            slice->getWpScaling(REF_PIC_LIST_0, refIdx0, wp0);
        }
        if (refIdx1 >= 0)
        {
            slice->getWpScaling(REF_PIC_LIST_1, refIdx1, wp1);
        }
    }
    else
    {
        assert(0);
    }

    if (refIdx0 < 0)
    {
        wp0 = NULL;
    }
    if (refIdx1 < 0)
    {
        wp1 = NULL;
    }

    if (bBiDir)
    { // Bi-Dir case
        for (Int yuv = 0; yuv < 3; yuv++)
        {
            wp0[yuv].w      = wp0[yuv].inputWeight;
            wp0[yuv].o      = wp0[yuv].inputOffset * (1 << (X265_DEPTH - 8));
            wp1[yuv].w      = wp1[yuv].inputWeight;
            wp1[yuv].o      = wp1[yuv].inputOffset * (1 << (X265_DEPTH - 8));
            wp0[yuv].offset = wp0[yuv].o + wp1[yuv].o;
            wp0[yuv].shift  = wp0[yuv].log2WeightDenom + 1;
            wp0[yuv].round  = (1 << wp0[yuv].log2WeightDenom);
            wp1[yuv].offset = wp0[yuv].offset;
            wp1[yuv].shift  = wp0[yuv].shift;
            wp1[yuv].round  = wp0[yuv].round;
        }
    }
    else
    { // Unidir
        pwp = (refIdx0 >= 0) ? wp0 : wp1;
        for (Int yuv = 0; yuv < 3; yuv++)
        {
            pwp[yuv].w      = pwp[yuv].inputWeight;
            pwp[yuv].offset = pwp[yuv].inputOffset * (1 << (X265_DEPTH - 8));
            pwp[yuv].shift  = pwp[yuv].log2WeightDenom;
            pwp[yuv].round  = (pwp[yuv].log2WeightDenom >= 1) ? (1 << (pwp[yuv].log2WeightDenom - 1)) : (0);
        }
    }
}

/** weighted prediction for bi-pred
 * \param TComDataCU* cu
 * \param TComYuv* srcYuv0
 * \param TComYuv* srcYuv1
 * \param refIdx0
 * \param refIdx1
 * \param partIdx
 * \param width
 * \param height
 * \param TComYuv* outDstYuv
 * \returns void
 */
void TComWeightPrediction::xWeightedPredictionBi(TComDataCU* cu, TComYuv* srcYuv0, TComYuv* srcYuv1, Int refIdx0, Int refIdx1, UInt partIdx, Int width, Int height, TComYuv* outDstYuv)
{
    wpScalingParam  *pwp0, *pwp1;

    getWpScaling(cu, refIdx0, refIdx1, pwp0, pwp1);

    if (refIdx0 >= 0 && refIdx1 >= 0)
    {
        addWeightBi(srcYuv0, srcYuv1, partIdx, width, height, pwp0, pwp1, outDstYuv);
    }
    else if (refIdx0 >= 0 && refIdx1 <  0)
    {
        addWeightUni(srcYuv0, partIdx, width, height, pwp0, outDstYuv);
    }
    else if (refIdx0 <  0 && refIdx1 >= 0)
    {
        addWeightUni(srcYuv1, partIdx, width, height, pwp1, outDstYuv);
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
 * \param refIdx0
 * \param refIdx1
 * \param partIdx
 * \param width
 * \param height
 * \param TComYuv* outDstYuv
 * \returns void
 */
void TComWeightPrediction::xWeightedPredictionBi(TComDataCU* cu, TShortYUV* srcYuv0, TShortYUV* srcYuv1, Int refIdx0, Int refIdx1, UInt partIdx, Int width, Int height, TComYuv* outDstYuv)
{
    wpScalingParam  *pwp0, *pwp1;

    getWpScaling(cu, refIdx0, refIdx1, pwp0, pwp1);

    if (refIdx0 >= 0 && refIdx1 >= 0)
    {
        addWeightBi(srcYuv0, srcYuv1, partIdx, width, height, pwp0, pwp1, outDstYuv);
    }
    else if (refIdx0 >= 0 && refIdx1 <  0)
    {
        addWeightUni(srcYuv0, partIdx, width, height, pwp0, outDstYuv);
    }
    else if (refIdx0 <  0 && refIdx1 >= 0)
    {
        addWeightUni(srcYuv1, partIdx, width, height, pwp1, outDstYuv);
    }
    else
    {
        assert(0);
    }
}

/** weighted prediction for uni-pred
 * \param TComDataCU* cu
 * \param TComYuv* srcYuv
 * \param partAddr
 * \param width
 * \param height
 * \param picList
 * \param TComYuv*& outPredYuv
 * \param partIdx
 * \param refIdx
 * \returns void
 */
void TComWeightPrediction::xWeightedPredictionUni(TComDataCU* cu, TComYuv* srcYuv, UInt partAddr, Int width, Int height, RefPicList picList, TComYuv*& outPredYuv, Int refIdx)
{
    wpScalingParam  *pwp, *pwpTmp;

    if (refIdx < 0)
    {
        refIdx   = cu->getCUMvField(picList)->getRefIdx(partAddr);
    }
    assert(refIdx >= 0);

    if (picList == REF_PIC_LIST_0)
    {
        getWpScaling(cu, refIdx, -1, pwp, pwpTmp);
    }
    else
    {
        getWpScaling(cu, -1, refIdx, pwpTmp, pwp);
    }
    addWeightUni(srcYuv, partAddr, width, height, pwp, outPredYuv);
}

/** weighted prediction for uni-pred
 * \param TComDataCU* cu
 * \param TShortYuv* srcYuv
 * \param partAddr
 * \param width
 * \param height
 * \param picList
 * \param TComYuv*& outPredYuv
 * \param partIdx
 * \param refIdx
 * \returns void
 */
void TComWeightPrediction::xWeightedPredictionUni(TComDataCU* cu, TShortYUV* srcYuv, UInt partAddr, Int width, Int height, RefPicList picList, TComYuv*& outPredYuv, Int refIdx)
{
    wpScalingParam  *pwp, *pwpTmp;

    if (refIdx < 0)
    {
        refIdx   = cu->getCUMvField(picList)->getRefIdx(partAddr);
    }
    assert(refIdx >= 0);

    if (picList == REF_PIC_LIST_0)
    {
        getWpScaling(cu, refIdx, -1, pwp, pwpTmp);
    }
    else
    {
        getWpScaling(cu, -1, refIdx, pwpTmp, pwp);
    }
    addWeightUni(srcYuv, partAddr, width, height, pwp, outPredYuv, true);
}
