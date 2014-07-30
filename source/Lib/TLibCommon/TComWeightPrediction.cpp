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

#include "common.h"
#include "primitives.h"
#include "slice.h"
#include "TComWeightPrediction.h"
#include "predict.h"

using namespace x265;

static inline pixel weightBidirY(int w0, int16_t P0, int w1, int16_t P1, int round, int shift, int offset)
{
    return Clip((w0 * (P0 + IF_INTERNAL_OFFS) + w1 * (P1 + IF_INTERNAL_OFFS) + round + (offset << (shift - 1))) >> shift);
}

static inline pixel weightBidirC(int w0, int16_t P0, int w1, int16_t P1, int round, int shift, int offset)
{
    return Clip((w0 * (P0 + IF_INTERNAL_OFFS) + w1 * (P1 + IF_INTERNAL_OFFS) + round + (offset << (shift - 1))) >> shift);
}

static inline pixel weightUnidirY(int w0, int16_t P0, int round, int shift, int offset)
{
    return Clip(((w0 * (P0 + IF_INTERNAL_OFFS) + round) >> shift) + offset);
}

static inline pixel weightUnidirC(int w0, int16_t P0, int round, int shift, int offset)
{
    return Clip(((w0 * (P0 + IF_INTERNAL_OFFS) + round) >> shift) + offset);
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
void TComWeightPrediction::addWeightBi(TComYuv* srcYuv0, TComYuv* srcYuv1, uint32_t partUnitIdx, uint32_t width, uint32_t height, WeightParam *wp0, WeightParam *wp1, TComYuv* outDstYuv, bool bRound, bool bLuma, bool bChroma)
{
    int x, y;

    pixel* srcY0  = srcYuv0->getLumaAddr(partUnitIdx);
    pixel* srcU0  = srcYuv0->getCbAddr(partUnitIdx);
    pixel* srcV0  = srcYuv0->getCrAddr(partUnitIdx);

    pixel* srcY1  = srcYuv1->getLumaAddr(partUnitIdx);
    pixel* srcU1  = srcYuv1->getCbAddr(partUnitIdx);
    pixel* srcV1  = srcYuv1->getCrAddr(partUnitIdx);

    pixel* pDstY  = outDstYuv->getLumaAddr(partUnitIdx);
    pixel* dstU   = outDstYuv->getCbAddr(partUnitIdx);
    pixel* dstV   = outDstYuv->getCrAddr(partUnitIdx);

    if (bLuma)
    {
        // Luma : --------------------------------------------
        int w0       = wp0[0].w;
        int offset   = wp0[0].o + wp1[0].o;
        int shiftNum = IF_INTERNAL_PREC - X265_DEPTH;
        int shift    = wp0[0].shift + shiftNum + 1;
        int round    = shift ? (1 << (shift - 1)) * bRound : 0;
        int w1       = wp1[0].w;

        uint32_t  src0Stride = srcYuv0->getStride();
        uint32_t  src1Stride = srcYuv1->getStride();
        uint32_t  dststride  = outDstYuv->getStride();

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
    }

    if (bChroma)
    {
        // Chroma U : --------------------------------------------
        int w0      = wp0[1].w;
        int offset  = wp0[1].o + wp1[1].o;
        int shiftNum = IF_INTERNAL_PREC - X265_DEPTH;
        int shift   = wp0[1].shift + shiftNum + 1;
        int round   = shift ? (1 << (shift - 1)) : 0;
        int w1      = wp1[1].w;

        uint32_t src0Stride = srcYuv0->getCStride();
        uint32_t src1Stride = srcYuv1->getCStride();
        uint32_t dststride  = outDstYuv->getCStride();

        width  >>= srcYuv0->getHorzChromaShift();
        height >>= srcYuv0->getVertChromaShift();

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
        offset  = wp0[2].o + wp1[2].o;
        shift   = wp0[2].shift + shiftNum + 1;
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
void TComWeightPrediction::addWeightBi(ShortYuv* srcYuv0, ShortYuv* srcYuv1, uint32_t partUnitIdx, uint32_t width, uint32_t height, WeightParam *wp0, WeightParam *wp1, TComYuv* outDstYuv, bool bRound, bool bLuma, bool bChroma)
{
    int x, y;

    int w0, w1, offset, shiftNum, shift, round;
    uint32_t src0Stride, src1Stride, dststride;

    int16_t* srcY0  = srcYuv0->getLumaAddr(partUnitIdx);
    int16_t* srcU0  = srcYuv0->getCbAddr(partUnitIdx);
    int16_t* srcV0  = srcYuv0->getCrAddr(partUnitIdx);

    int16_t* srcY1  = srcYuv1->getLumaAddr(partUnitIdx);
    int16_t* srcU1  = srcYuv1->getCbAddr(partUnitIdx);
    int16_t* srcV1  = srcYuv1->getCrAddr(partUnitIdx);

    pixel* dstY   = outDstYuv->getLumaAddr(partUnitIdx);
    pixel* dstU   = outDstYuv->getCbAddr(partUnitIdx);
    pixel* dstV   = outDstYuv->getCrAddr(partUnitIdx);

    if (bLuma)
    {
        // Luma : --------------------------------------------
        w0      = wp0[0].w;
        offset  = wp0[0].o + wp1[0].o;
        shiftNum = IF_INTERNAL_PREC - X265_DEPTH;
        shift   = wp0[0].shift + shiftNum + 1;
        round   = shift ? (1 << (shift - 1)) * bRound : 0;
        w1      = wp1[0].w;

        src0Stride = srcYuv0->m_width;
        src1Stride = srcYuv1->m_width;
        dststride  = outDstYuv->getStride();

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
    }

    if (bChroma)
    {
        // Chroma U : --------------------------------------------
        w0      = wp0[1].w;
        offset  = wp0[1].o + wp1[1].o;
        shiftNum = IF_INTERNAL_PREC - X265_DEPTH;
        shift   = wp0[1].shift + shiftNum + 1;
        round   = shift ? (1 << (shift - 1)) : 0;
        w1      = wp1[1].w;

        src0Stride = srcYuv0->m_cwidth;
        src1Stride = srcYuv1->m_cwidth;
        dststride  = outDstYuv->getCStride();

        width  >>= srcYuv0->getHorzChromaShift();
        height >>= srcYuv0->getVertChromaShift();

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
        offset  = wp0[2].o + wp1[2].o;
        shift   = wp0[2].shift + shiftNum + 1;
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
void TComWeightPrediction::addWeightUni(TComYuv* srcYuv0, uint32_t partUnitIdx, uint32_t width, uint32_t height, WeightParam *wp0, TComYuv* outDstYuv, bool bLuma, bool bChroma)
{
    int x, y;

    int w0, offset, shiftNum, shift, round;
    uint32_t src0Stride, dststride;

    pixel* srcY0  = srcYuv0->getLumaAddr(partUnitIdx);
    pixel* srcU0  = srcYuv0->getCbAddr(partUnitIdx);
    pixel* srcV0  = srcYuv0->getCrAddr(partUnitIdx);

    pixel* dstY   = outDstYuv->getLumaAddr(partUnitIdx);
    pixel* dstU   = outDstYuv->getCbAddr(partUnitIdx);
    pixel* dstV   = outDstYuv->getCrAddr(partUnitIdx);

    if (bLuma)
    {
        // Luma : --------------------------------------------
        w0      = wp0[0].w;
        offset  = wp0[0].offset;
        shiftNum = IF_INTERNAL_PREC - X265_DEPTH;
        shift   = wp0[0].shift + shiftNum;
        round   = shift ? (1 << (shift - 1)) : 0;
        src0Stride = srcYuv0->getStride();
        dststride  = outDstYuv->getStride();

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
    }

    if (bChroma)
    {
        // Chroma U : --------------------------------------------
        w0      = wp0[1].w;
        offset  = wp0[1].offset;
        shiftNum = IF_INTERNAL_PREC - X265_DEPTH;
        shift   = wp0[1].shift + shiftNum;
        round   = shift ? (1 << (shift - 1)) : 0;

        src0Stride = srcYuv0->getCStride();
        dststride  = outDstYuv->getCStride();

        width  >>= srcYuv0->getHorzChromaShift();
        height >>= srcYuv0->getVertChromaShift();

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

void TComWeightPrediction::addWeightUni(ShortYuv* srcYuv0, uint32_t partUnitIdx, uint32_t width, uint32_t height, WeightParam *wp0, TComYuv* outDstYuv, bool bLuma, bool bChroma)
{
    int16_t* srcY0  = srcYuv0->getLumaAddr(partUnitIdx);
    int16_t* srcU0  = srcYuv0->getCbAddr(partUnitIdx);
    int16_t* srcV0  = srcYuv0->getCrAddr(partUnitIdx);

    pixel* dstY   = outDstYuv->getLumaAddr(partUnitIdx);
    pixel* dstU   = outDstYuv->getCbAddr(partUnitIdx);
    pixel* dstV   = outDstYuv->getCrAddr(partUnitIdx);

    int w0, offset, shiftNum, shift, round;
    uint32_t srcStride, dstStride;

    if (bLuma)
    {
        // Luma : --------------------------------------------
        w0      = wp0[0].w;
        offset  = wp0[0].offset;
        shiftNum = IF_INTERNAL_PREC - X265_DEPTH;
        shift   = wp0[0].shift + shiftNum;
        round   = shift ? (1 << (shift - 1)) : 0;
        srcStride = srcYuv0->m_width;
        dstStride  = outDstYuv->getStride();

        primitives.weight_sp(srcY0, dstY, srcStride, dstStride, width, height, w0, round, shift, offset);
    }

    if (bChroma)
    {
        // Chroma U : --------------------------------------------
        w0      = wp0[1].w;
        offset  = wp0[1].offset;
        shiftNum = IF_INTERNAL_PREC - X265_DEPTH;
        shift   = wp0[1].shift + shiftNum;
        round   = shift ? (1 << (shift - 1)) : 0;

        srcStride = srcYuv0->m_cwidth;
        dstStride  = outDstYuv->getCStride();

        width  >>= srcYuv0->getHorzChromaShift();
        height >>= srcYuv0->getVertChromaShift();

        primitives.weight_sp(srcU0, dstU, srcStride, dstStride, width, height, w0, round, shift, offset);

        // Chroma V : --------------------------------------------
        w0      = wp0[2].w;
        offset  = wp0[2].offset;
        shift   = wp0[2].shift + shiftNum;
        round   = shift ? (1 << (shift - 1)) : 0;

        primitives.weight_sp(srcV0, dstV, srcStride, dstStride, width, height, w0, round, shift, offset);
    }
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
void TComWeightPrediction::getWpScaling(TComDataCU* cu, int refIdx0, int refIdx1, WeightParam *&wp0, WeightParam *&wp1)
{
    Slice* slice = cu->m_slice;
    bool wpBiPred = slice->m_pps->bUseWeightedBiPred;
    bool bBiDir  = (refIdx0 >= 0 && refIdx1 >= 0);
    bool bUniDir = !bBiDir;

    if (bUniDir || wpBiPred)
    {
        if (refIdx0 >= 0)
            wp0 = slice->m_weightPredTable[0][refIdx0];

        if (refIdx1 >= 0)
            wp1 = slice->m_weightPredTable[1][refIdx1];
    }
    else
    {
        X265_CHECK(0, "unexpected wpScaling configuration\n");
    }

    if (refIdx0 < 0)
        wp0 = NULL;

    if (refIdx1 < 0)
        wp1 = NULL;

    if (bBiDir)
    {
        for (int yuv = 0; yuv < 3; yuv++)
        {
            wp0[yuv].w      = wp0[yuv].inputWeight;
            wp0[yuv].o      = wp0[yuv].inputOffset * (1 << (X265_DEPTH - 8));
            wp1[yuv].w      = wp1[yuv].inputWeight;
            wp1[yuv].o      = wp1[yuv].inputOffset * (1 << (X265_DEPTH - 8));
            wp0[yuv].shift  = wp0[yuv].log2WeightDenom;
            wp0[yuv].round  = (1 << wp0[yuv].log2WeightDenom);
            wp1[yuv].shift  = wp0[yuv].shift;
            wp1[yuv].round  = wp0[yuv].round;
        }
    }
    else
    {
        WeightParam* pwp = (refIdx0 >= 0) ? wp0 : wp1;
        for (int yuv = 0; yuv < 3; yuv++)
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
void TComWeightPrediction::xWeightedPredictionBi(TComDataCU* cu, TComYuv* srcYuv0, TComYuv* srcYuv1, int refIdx0, int refIdx1, uint32_t partIdx, int width, int height, TComYuv* outDstYuv, bool bLuma, bool bChroma)
{
    WeightParam  *pwp0 = NULL, *pwp1 = NULL;

    getWpScaling(cu, refIdx0, refIdx1, pwp0, pwp1);

    if (refIdx0 >= 0 && refIdx1 >= 0)
    {
        addWeightBi(srcYuv0, srcYuv1, partIdx, width, height, pwp0, pwp1, outDstYuv, true, bLuma, bChroma);
    }
    else if (refIdx0 >= 0 && refIdx1 <  0)
    {
        addWeightUni(srcYuv0, partIdx, width, height, pwp0, outDstYuv, bLuma, bChroma);
    }
    else if (refIdx0 <  0 && refIdx1 >= 0)
    {
        addWeightUni(srcYuv1, partIdx, width, height, pwp1, outDstYuv, bLuma, bChroma);
    }
    else
    {
        X265_CHECK(0, "unexpected biprediction configuration\n");
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
void TComWeightPrediction::xWeightedPredictionBi(TComDataCU* cu, ShortYuv* srcYuv0, ShortYuv* srcYuv1, int refIdx0, int refIdx1, uint32_t partIdx, int width, int height, TComYuv* outDstYuv, bool bLuma, bool bChroma)
{
    WeightParam  *pwp0 = NULL, *pwp1 = NULL;

    getWpScaling(cu, refIdx0, refIdx1, pwp0, pwp1);

    if (refIdx0 >= 0 && refIdx1 >= 0)
    {
        addWeightBi(srcYuv0, srcYuv1, partIdx, width, height, pwp0, pwp1, outDstYuv, true, bLuma, bChroma);
    }
    else if (refIdx0 >= 0 && refIdx1 <  0)
    {
        addWeightUni(srcYuv0, partIdx, width, height, pwp0, outDstYuv, bLuma, bChroma);
    }
    else if (refIdx0 <  0 && refIdx1 >= 0)
    {
        addWeightUni(srcYuv1, partIdx, width, height, pwp1, outDstYuv, bLuma, bChroma);
    }
    else
    {
        X265_CHECK(0, "unexpected weighte biprediction configuration\n");
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
void TComWeightPrediction::xWeightedPredictionUni(TComDataCU* cu, TComYuv* srcYuv, uint32_t partAddr, int width, int height, int picList, TComYuv*& outPredYuv, int refIdx, bool bLuma, bool bChroma)
{
    WeightParam  *pwp, *pwpTmp;

    if (refIdx < 0)
    {
        refIdx = cu->getCUMvField(picList)->getRefIdx(partAddr);
    }
    X265_CHECK(refIdx >= 0, "invalid refidx\n");

    if (picList == REF_PIC_LIST_0)
    {
        getWpScaling(cu, refIdx, -1, pwp, pwpTmp);
    }
    else
    {
        getWpScaling(cu, -1, refIdx, pwpTmp, pwp);
    }
    addWeightUni(srcYuv, partAddr, width, height, pwp, outPredYuv, bLuma, bChroma);
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
void TComWeightPrediction::xWeightedPredictionUni(TComDataCU* cu, ShortYuv* srcYuv, uint32_t partAddr, int width, int height, int picList, TComYuv*& outPredYuv, int refIdx, bool bLuma, bool bChroma)
{
    WeightParam  *pwp, *pwpTmp;

    if (refIdx < 0)
    {
        refIdx = cu->getCUMvField(picList)->getRefIdx(partAddr);
    }
    X265_CHECK(refIdx >= 0, "invalid refidx\n");

    if (picList == REF_PIC_LIST_0)
    {
        getWpScaling(cu, refIdx, -1, pwp, pwpTmp);
    }
    else
    {
        getWpScaling(cu, -1, refIdx, pwpTmp, pwp);
    }
    addWeightUni(srcYuv, partAddr, width, height, pwp, outPredYuv, bLuma, bChroma);
}
