/*****************************************************************************
 * Copyright (C) 2013-2020 MulticoreWare, Inc
 *
 * Authors: Gopu Govindaswamy <gopu@multicorewareinc.com>
 *          Steve Borho <steve@borho.org>
 *          Ashok Kumar Mishra <ashok@multicorewareinc.com>
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
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#include "common.h"
#include "frame.h"
#include "framedata.h"
#include "picyuv.h"
#include "primitives.h"
#include "lowres.h"
#include "mv.h"

#include "slicetype.h"
#include "motion.h"
#include "ratecontrol.h"

#if DETAILED_CU_STATS
#define ProfileLookaheadTime(elapsed, count) ScopedElapsedTime _scope(elapsed); count++
#else
#define ProfileLookaheadTime(elapsed, count)
#endif

using namespace X265_NS;

namespace {

uint32_t acEnergyVarHist(uint64_t sum_ssd, int shift)
{
    uint32_t sum = (uint32_t)sum_ssd;
    uint32_t ssd = (uint32_t)(sum_ssd >> 32);

    return ssd - ((uint64_t)sum * sum >> shift);
}

/* Compute variance to derive AC energy of each block */
inline uint32_t acEnergyVar(Frame *curFrame, uint64_t sum_ssd, int shift, int plane)
{
    uint32_t sum = (uint32_t)sum_ssd;
    uint32_t ssd = (uint32_t)(sum_ssd >> 32);

    curFrame->m_lowres.wp_sum[plane] += sum;
    curFrame->m_lowres.wp_ssd[plane] += ssd;
    return ssd - ((uint64_t)sum * sum >> shift);
}

/* Find the energy of each block in Y/Cb/Cr plane */
inline uint32_t acEnergyPlane(Frame *curFrame, pixel* src, intptr_t srcStride, int plane, int colorFormat, uint32_t qgSize)
{
    if ((colorFormat != X265_CSP_I444) && plane)
    {
        if (qgSize == 8)
        {
            ALIGN_VAR_4(pixel, pix[4 * 4]);
            primitives.cu[BLOCK_4x4].copy_pp(pix, 4, src, srcStride);
            return acEnergyVar(curFrame, primitives.cu[BLOCK_4x4].var(pix, 4), 4, plane);
        }
        else
        {
            ALIGN_VAR_8(pixel, pix[8 * 8]);
            primitives.cu[BLOCK_8x8].copy_pp(pix, 8, src, srcStride);
            return acEnergyVar(curFrame, primitives.cu[BLOCK_8x8].var(pix, 8), 6, plane);
        }
    }
    else
    {
        if (qgSize == 8)
            return acEnergyVar(curFrame, primitives.cu[BLOCK_8x8].var(src, srcStride), 6, plane);
        else
            return acEnergyVar(curFrame, primitives.cu[BLOCK_16x16].var(src, srcStride), 8, plane);
    }
}

} // end anonymous namespace

namespace X265_NS {

bool computeEdge(pixel* edgePic, pixel* refPic, pixel* edgeTheta, intptr_t stride, int height, int width, bool bcalcTheta, pixel whitePixel)
{
    intptr_t rowOne = 0, rowTwo = 0, rowThree = 0, colOne = 0, colTwo = 0, colThree = 0;
    intptr_t middle = 0, topLeft = 0, topRight = 0, bottomLeft = 0, bottomRight = 0;

    const int startIndex = 1;

    if (!edgePic || !refPic || (!edgeTheta && bcalcTheta))
    {
        return false;
    }
    else
    {
        float gradientH = 0, gradientV = 0, radians = 0, theta = 0;
        float gradientMagnitude = 0;
        pixel blackPixel = 0;

        //Applying Sobel filter expect for border pixels
        height = height - startIndex;
        width = width - startIndex;
        for (int rowNum = startIndex; rowNum < height; rowNum++)
        {
            rowTwo = rowNum * stride;
            rowOne = rowTwo - stride;
            rowThree = rowTwo + stride;

            for (int colNum = startIndex; colNum < width; colNum++)
            {

                 /*  Horizontal and vertical gradients
                     [ -3   0   3 ]        [-3   -10  -3 ]
                 gH =[ -10  0   10]   gV = [ 0    0    0 ]
                     [ -3   0   3 ]        [ 3    10   3 ] */

                colTwo = colNum;
                colOne = colTwo - startIndex;
                colThree = colTwo + startIndex;
                middle = rowTwo + colTwo;
                topLeft = rowOne + colOne;
                topRight = rowOne + colThree;
                bottomLeft = rowThree + colOne;
                bottomRight = rowThree + colThree;
                gradientH = (float)(-3 * refPic[topLeft] + 3 * refPic[topRight] - 10 * refPic[rowTwo + colOne] + 10 * refPic[rowTwo + colThree] - 3 * refPic[bottomLeft] + 3 * refPic[bottomRight]);
                gradientV = (float)(-3 * refPic[topLeft] - 10 * refPic[rowOne + colTwo] - 3 * refPic[topRight] + 3 * refPic[bottomLeft] + 10 * refPic[rowThree + colTwo] + 3 * refPic[bottomRight]);
                gradientMagnitude = sqrtf(gradientH * gradientH + gradientV * gradientV);
                if(bcalcTheta) 
                {
                    edgeTheta[middle] = 0;
                    radians = atan2(gradientV, gradientH);
                    theta = (float)((radians * 180) / PI);
                    if (theta < 0)
                       theta = 180 + theta;
                    edgeTheta[middle] = (pixel)theta;
                }
                edgePic[middle] = (pixel)(gradientMagnitude >= EDGE_THRESHOLD ? whitePixel : blackPixel);
            }
        }
        return true;
    }
}

void edgeFilter(Frame *curFrame, x265_param* param)
{
    int height = curFrame->m_fencPic->m_picHeight;
    int width = curFrame->m_fencPic->m_picWidth;
    intptr_t stride = curFrame->m_fencPic->m_stride;
    uint32_t numCuInHeight = (height + param->maxCUSize - 1) / param->maxCUSize;
    int maxHeight = numCuInHeight * param->maxCUSize;

    memset(curFrame->m_edgePic, 0, stride * (maxHeight + (curFrame->m_fencPic->m_lumaMarginY * 2)) * sizeof(pixel));
    memset(curFrame->m_gaussianPic, 0, stride * (maxHeight + (curFrame->m_fencPic->m_lumaMarginY * 2)) * sizeof(pixel));
    memset(curFrame->m_thetaPic, 0, stride * (maxHeight + (curFrame->m_fencPic->m_lumaMarginY * 2)) * sizeof(pixel));

    pixel *src = (pixel*)curFrame->m_fencPic->m_picOrg[0];
    pixel *edgePic = curFrame->m_edgePic + curFrame->m_fencPic->m_lumaMarginY * stride + curFrame->m_fencPic->m_lumaMarginX;
    pixel *refPic = curFrame->m_gaussianPic + curFrame->m_fencPic->m_lumaMarginY * stride + curFrame->m_fencPic->m_lumaMarginX;
    pixel *edgeTheta = curFrame->m_thetaPic + curFrame->m_fencPic->m_lumaMarginY * stride + curFrame->m_fencPic->m_lumaMarginX;

    for (int i = 0; i < height; i++)
    {
        memcpy(edgePic, src, width * sizeof(pixel));
        memcpy(refPic, src, width * sizeof(pixel));
        src += stride;
        edgePic += stride;
        refPic += stride;
    }

    //Applying Gaussian filter on the picture
    src = (pixel*)curFrame->m_fencPic->m_picOrg[0];
    refPic = curFrame->m_gaussianPic + curFrame->m_fencPic->m_lumaMarginY * stride + curFrame->m_fencPic->m_lumaMarginX;
    edgePic = curFrame->m_edgePic + curFrame->m_fencPic->m_lumaMarginY * stride + curFrame->m_fencPic->m_lumaMarginX;
    pixel pixelValue = 0;

    for (int rowNum = 0; rowNum < height; rowNum++)
    {
        for (int colNum = 0; colNum < width; colNum++)
        {
            if ((rowNum >= 2) && (colNum >= 2) && (rowNum < height - 2) && (colNum < width - 2)) //Ignoring the border pixels of the picture
            {
                /*  5x5 Gaussian filter
                    [2   4   5   4   2]
                 1  [4   9   12  9   4]
                --- [5   12  15  12  5]
                159 [4   9   12  9   4]
                    [2   4   5   4   2]*/

                const intptr_t rowOne = (rowNum - 2)*stride, colOne = colNum - 2;
                const intptr_t rowTwo = (rowNum - 1)*stride, colTwo = colNum - 1;
                const intptr_t rowThree = rowNum * stride, colThree = colNum;
                const intptr_t rowFour = (rowNum + 1)*stride, colFour = colNum + 1;
                const intptr_t rowFive = (rowNum + 2)*stride, colFive = colNum + 2;
                const intptr_t index = (rowNum*stride) + colNum;

                pixelValue = ((2 * src[rowOne + colOne] + 4 * src[rowOne + colTwo] + 5 * src[rowOne + colThree] + 4 * src[rowOne + colFour] + 2 * src[rowOne + colFive] +
                    4 * src[rowTwo + colOne] + 9 * src[rowTwo + colTwo] + 12 * src[rowTwo + colThree] + 9 * src[rowTwo + colFour] + 4 * src[rowTwo + colFive] +
                    5 * src[rowThree + colOne] + 12 * src[rowThree + colTwo] + 15 * src[rowThree + colThree] + 12 * src[rowThree + colFour] + 5 * src[rowThree + colFive] +
                    4 * src[rowFour + colOne] + 9 * src[rowFour + colTwo] + 12 * src[rowFour + colThree] + 9 * src[rowFour + colFour] + 4 * src[rowFour + colFive] +
                    2 * src[rowFive + colOne] + 4 * src[rowFive + colTwo] + 5 * src[rowFive + colThree] + 4 * src[rowFive + colFour] + 2 * src[rowFive + colFive]) / 159);
                refPic[index] = pixelValue;
            }
        }
    }

    if(!computeEdge(edgePic, refPic, edgeTheta, stride, height, width, true))
        x265_log(NULL, X265_LOG_ERROR, "Failed edge computation!");
}

//Find the angle of a block by averaging the pixel angles 
inline void findAvgAngle(const pixel* block, intptr_t stride, uint32_t size, uint32_t &angle)
{
    int sum = 0;
    for (uint32_t y = 0; y < size; y++)
    {
        for (uint32_t x = 0; x < size; x++)
        {
            sum += block[x];
        }
        block += stride;
    }
    angle = sum / (size*size);
}

uint32_t LookaheadTLD::edgeDensityCu(Frame* curFrame, uint32_t &avgAngle, uint32_t blockX, uint32_t blockY, uint32_t qgSize)
{
    pixel *edgeImage = curFrame->m_edgePic + curFrame->m_fencPic->m_lumaMarginY * curFrame->m_fencPic->m_stride + curFrame->m_fencPic->m_lumaMarginX;
    pixel *edgeTheta = curFrame->m_thetaPic + curFrame->m_fencPic->m_lumaMarginY * curFrame->m_fencPic->m_stride + curFrame->m_fencPic->m_lumaMarginX;
    intptr_t srcStride = curFrame->m_fencPic->m_stride;
    intptr_t blockOffsetLuma = blockX + (blockY * srcStride);
    int plane = 0; // Sobel filter is applied only on Y component
    uint32_t var;

    if (qgSize == 8)
    {
        findAvgAngle(edgeTheta + blockOffsetLuma, srcStride, qgSize, avgAngle);
        var = acEnergyVar(curFrame, primitives.cu[BLOCK_8x8].var(edgeImage + blockOffsetLuma, srcStride), 6, plane);
    }
    else
    {
        findAvgAngle(edgeTheta + blockOffsetLuma, srcStride, 16, avgAngle);
        var = acEnergyVar(curFrame, primitives.cu[BLOCK_16x16].var(edgeImage + blockOffsetLuma, srcStride), 8, plane);
    }
    x265_emms();
    return var;
}

/* Find the total AC energy of each block in all planes */
uint32_t LookaheadTLD::acEnergyCu(Frame* curFrame, uint32_t blockX, uint32_t blockY, int csp, uint32_t qgSize)
{
    intptr_t stride = curFrame->m_fencPic->m_stride;
    intptr_t cStride = curFrame->m_fencPic->m_strideC;
    intptr_t blockOffsetLuma = blockX + (blockY * stride);
    int hShift = CHROMA_H_SHIFT(csp);
    int vShift = CHROMA_V_SHIFT(csp);
    intptr_t blockOffsetChroma = (blockX >> hShift) + ((blockY >> vShift) * cStride);

    uint32_t var;

    var  = acEnergyPlane(curFrame, curFrame->m_fencPic->m_picOrg[0] + blockOffsetLuma, stride, 0, csp, qgSize);
    if (csp != X265_CSP_I400 && curFrame->m_fencPic->m_picCsp != X265_CSP_I400)
    {
        var += acEnergyPlane(curFrame, curFrame->m_fencPic->m_picOrg[1] + blockOffsetChroma, cStride, 1, csp, qgSize);
        var += acEnergyPlane(curFrame, curFrame->m_fencPic->m_picOrg[2] + blockOffsetChroma, cStride, 2, csp, qgSize);
    }
    x265_emms();
    return var;
}

/* Find the sum of pixels of each block for luma plane */
uint32_t LookaheadTLD::lumaSumCu(Frame* curFrame, uint32_t blockX, uint32_t blockY, uint32_t qgSize)
{
    intptr_t stride = curFrame->m_fencPic->m_stride;
    intptr_t blockOffsetLuma = blockX + (blockY * stride);
    uint64_t sum_ssd;

    if (qgSize == 8)
        sum_ssd = primitives.cu[BLOCK_8x8].var(curFrame->m_fencPic->m_picOrg[0] + blockOffsetLuma, stride);
    else
        sum_ssd = primitives.cu[BLOCK_16x16].var(curFrame->m_fencPic->m_picOrg[0] + blockOffsetLuma, stride);

    x265_emms();
    return (uint32_t)sum_ssd;
}

void LookaheadTLD::xPreanalyzeQp(Frame* curFrame)
{
    const uint32_t width = curFrame->m_fencPic->m_picWidth;
    const uint32_t height = curFrame->m_fencPic->m_picHeight;

    for (uint32_t d = 0; d < 4; d++)
    {
        int ctuSizeIdx = 6 - g_log2Size[curFrame->m_param->maxCUSize];
        int aqDepth = g_log2Size[curFrame->m_param->maxCUSize] - g_log2Size[curFrame->m_param->rc.qgSize];
        if (!aqLayerDepth[ctuSizeIdx][aqDepth][d])
            continue;

        PicQPAdaptationLayer* pcAQLayer = &curFrame->m_lowres.pAQLayer[d];
        const uint32_t aqPartWidth = pcAQLayer->aqPartWidth;
        const uint32_t aqPartHeight = pcAQLayer->aqPartHeight;
        double* pcAQU = pcAQLayer->dActivity;
        double* pcQP = pcAQLayer->dQpOffset;
        double* pcCuTree = pcAQLayer->dCuTreeOffset;

        for (uint32_t y = 0; y < height; y += aqPartHeight)
        {
            for (uint32_t x = 0; x < width; x += aqPartWidth, pcAQU++, pcQP++, pcCuTree++)
            {
                double dMaxQScale = pow(2.0, curFrame->m_param->rc.qpAdaptationRange / 6.0);
                double dCUAct = *pcAQU;
                double dAvgAct = pcAQLayer->dAvgActivity;

                double dNormAct = (dMaxQScale*dCUAct + dAvgAct) / (dCUAct + dMaxQScale*dAvgAct);
                double dQpOffset = (X265_LOG2(dNormAct) / X265_LOG2(2.0)) * 6.0;
                *pcQP = dQpOffset;
                *pcCuTree = dQpOffset;
            }
        }
    }
}

void LookaheadTLD::xPreanalyze(Frame* curFrame)
{
    const uint32_t width = curFrame->m_fencPic->m_picWidth;
    const uint32_t height = curFrame->m_fencPic->m_picHeight;
    const intptr_t stride = curFrame->m_fencPic->m_stride;

    for (uint32_t d = 0; d < 4; d++)
    {
        int ctuSizeIdx = 6 - g_log2Size[curFrame->m_param->maxCUSize];
        int aqDepth = g_log2Size[curFrame->m_param->maxCUSize] - g_log2Size[curFrame->m_param->rc.qgSize];
        if (!aqLayerDepth[ctuSizeIdx][aqDepth][d])
            continue;

        const pixel* src = curFrame->m_fencPic->m_picOrg[0];;
        PicQPAdaptationLayer* pQPLayer = &curFrame->m_lowres.pAQLayer[d];
        const uint32_t aqPartWidth = pQPLayer->aqPartWidth;
        const uint32_t aqPartHeight = pQPLayer->aqPartHeight;
        double* pcAQU = pQPLayer->dActivity;

        double dSumAct = 0.0;
        for (uint32_t y = 0; y < height; y += aqPartHeight)
        {
            const uint32_t currAQPartHeight = X265_MIN(aqPartHeight, height - y);
            for (uint32_t x = 0; x < width; x += aqPartWidth, pcAQU++)
            {
                const uint32_t currAQPartWidth = X265_MIN(aqPartWidth, width - x);
                const pixel* pBlkY = &src[x];
                uint64_t sum[4] = { 0, 0, 0, 0 };
                uint64_t sumSq[4] = { 0, 0, 0, 0 };
                uint32_t by = 0;
                for (; by < currAQPartHeight >> 1; by++)
                {
                    uint32_t bx = 0;
                    for (; bx < currAQPartWidth >> 1; bx++)
                    {
                        sum[0] += pBlkY[bx];
                        sumSq[0] += pBlkY[bx] * pBlkY[bx];
                    }
                    for (; bx < currAQPartWidth; bx++)
                    {
                        sum[1] += pBlkY[bx];
                        sumSq[1] += pBlkY[bx] * pBlkY[bx];
                    }
                    pBlkY += stride;
                }
                for (; by < currAQPartHeight; by++)
                {
                    uint32_t bx = 0;
                    for (; bx < currAQPartWidth >> 1; bx++)
                    {
                        sum[2] += pBlkY[bx];
                        sumSq[2] += pBlkY[bx] * pBlkY[bx];
                    }
                    for (; bx < currAQPartWidth; bx++)
                    {
                        sum[3] += pBlkY[bx];
                        sumSq[3] += pBlkY[bx] * pBlkY[bx];
                    }
                    pBlkY += stride;
                }

                assert((currAQPartWidth & 1) == 0);
                assert((currAQPartHeight & 1) == 0);
                const uint32_t pixelWidthOfQuadrants = currAQPartWidth >> 1;
                const uint32_t pixelHeightOfQuadrants = currAQPartHeight >> 1;
                const uint32_t numPixInAQPart = pixelWidthOfQuadrants * pixelHeightOfQuadrants;

                double dMinVar = MAX_DOUBLE;
                if (numPixInAQPart != 0)
                {
                    for (int i = 0; i < 4; i++)
                    {
                        const double dAverage = double(sum[i]) / numPixInAQPart;
                        const double dVariance = double(sumSq[i]) / numPixInAQPart - dAverage * dAverage;
                        dMinVar = X265_MIN(dMinVar, dVariance);
                    }
                }
                else
                {
                    dMinVar = 0.0;
                }
                double dActivity = 1.0 + dMinVar;
                *pcAQU = dActivity;
                dSumAct += dActivity;
            }
            src += stride * currAQPartHeight;
        }

        const double dAvgAct = dSumAct / (pQPLayer->numAQPartInWidth * pQPLayer->numAQPartInHeight);
        pQPLayer->dAvgActivity = dAvgAct;
    }

    xPreanalyzeQp(curFrame);

    int minAQDepth = curFrame->m_lowres.pAQLayer->minAQDepth;

    PicQPAdaptationLayer* pQPLayer = &curFrame->m_lowres.pAQLayer[minAQDepth];
    const uint32_t aqPartWidth = pQPLayer->aqPartWidth;
    const uint32_t aqPartHeight = pQPLayer->aqPartHeight;
    double* pcQP = pQPLayer->dQpOffset;

    // Use new qp offset values for qpAqOffset, qpCuTreeOffset and invQscaleFactor buffer
    int blockXY = 0;
    for (uint32_t y = 0; y < height; y += aqPartHeight)
    {
        for (uint32_t x = 0; x < width; x += aqPartWidth, pcQP++)
        {
            curFrame->m_lowres.invQscaleFactor[blockXY] = x265_exp2fix8(*pcQP);
            blockXY++;

            acEnergyCu(curFrame, x, y, curFrame->m_param->internalCsp, curFrame->m_param->rc.qgSize);
        }
    }
}

void LookaheadTLD::calcAdaptiveQuantFrame(Frame *curFrame, x265_param* param)
{
    /* Actual adaptive quantization */
    int maxCol = curFrame->m_fencPic->m_picWidth;
    int maxRow = curFrame->m_fencPic->m_picHeight;
    int blockCount, loopIncr;
    float modeOneConst, modeTwoConst;
    if (param->rc.qgSize == 8)
    {
        blockCount = curFrame->m_lowres.maxBlocksInRowFullRes * curFrame->m_lowres.maxBlocksInColFullRes;
        modeOneConst = 11.427f;
        modeTwoConst = 8.f;
        loopIncr = 8;
    }
    else
    {
        blockCount = widthInCU * heightInCU;
        modeOneConst = 14.427f;
        modeTwoConst = 11.f;
        loopIncr = 16;
    }

    float* quantOffsets = curFrame->m_quantOffsets;
    for (int y = 0; y < 3; y++)
    {
        curFrame->m_lowres.wp_ssd[y] = 0;
        curFrame->m_lowres.wp_sum[y] = 0;
    }

    if (!(param->rc.bStatRead && param->rc.cuTree && IS_REFERENCED(curFrame)))
    {
        /* Calculate Qp offset for each 16x16 or 8x8 block in the frame */
        if (param->rc.aqMode == X265_AQ_NONE || param->rc.aqStrength == 0)
        {
            if (param->rc.aqMode && param->rc.aqStrength == 0)
            {
                if (quantOffsets)
                {
                    for (int cuxy = 0; cuxy < blockCount; cuxy++)
                    {
                        curFrame->m_lowres.qpCuTreeOffset[cuxy] = curFrame->m_lowres.qpAqOffset[cuxy] = quantOffsets[cuxy];
                        curFrame->m_lowres.invQscaleFactor[cuxy] = x265_exp2fix8(curFrame->m_lowres.qpCuTreeOffset[cuxy]);
                    }
                }
                else
                {
                    memset(curFrame->m_lowres.qpCuTreeOffset, 0, blockCount * sizeof(double));
                    memset(curFrame->m_lowres.qpAqOffset, 0, blockCount * sizeof(double));
                    for (int cuxy = 0; cuxy < blockCount; cuxy++)
                        curFrame->m_lowres.invQscaleFactor[cuxy] = 256;
                }
            }

            /* Need variance data for weighted prediction and dynamic refinement*/
            if (param->bEnableWeightedPred || param->bEnableWeightedBiPred)
            {
                for (int blockY = 0; blockY < maxRow; blockY += loopIncr)
                    for (int blockX = 0; blockX < maxCol; blockX += loopIncr)
                        acEnergyCu(curFrame, blockX, blockY, param->internalCsp, param->rc.qgSize);
            }
        }
        else
        {
            if (param->rc.hevcAq)
            {
                // New method for calculating variance and qp offset
                xPreanalyze(curFrame);
            }
            else
            {
                int blockXY = 0, inclinedEdge = 0;
                double avg_adj_pow2 = 0, avg_adj = 0, qp_adj = 0;
                double bias_strength = 0.f;
                double strength = 0.f;

                if (param->rc.aqMode == X265_AQ_EDGE)
                    edgeFilter(curFrame, param);

                if (param->rc.aqMode == X265_AQ_EDGE && param->recursionSkipMode == EDGE_BASED_RSKIP)
                {
                    pixel* src = curFrame->m_edgePic + curFrame->m_fencPic->m_lumaMarginY * curFrame->m_fencPic->m_stride + curFrame->m_fencPic->m_lumaMarginX;
                    primitives.planecopy_pp_shr(src, curFrame->m_fencPic->m_stride, curFrame->m_edgeBitPic,
                        curFrame->m_fencPic->m_stride, curFrame->m_fencPic->m_picWidth, curFrame->m_fencPic->m_picHeight, SHIFT_TO_BITPLANE);
                }

                if (param->rc.aqMode == X265_AQ_AUTO_VARIANCE || param->rc.aqMode == X265_AQ_AUTO_VARIANCE_BIASED || param->rc.aqMode == X265_AQ_EDGE)
                {
                    double bit_depth_correction = 1.f / (1 << (2 * (X265_DEPTH - 8)));
                    for (int blockY = 0; blockY < maxRow; blockY += loopIncr)
                    {
                        for (int blockX = 0; blockX < maxCol; blockX += loopIncr)
                        {
                            uint32_t energy, edgeDensity, avgAngle;
                            energy = acEnergyCu(curFrame, blockX, blockY, param->internalCsp, param->rc.qgSize);
                            if (param->rc.aqMode == X265_AQ_EDGE)
                            {
                                edgeDensity = edgeDensityCu(curFrame, avgAngle, blockX, blockY, param->rc.qgSize);
                                if (edgeDensity)
                                {
                                    qp_adj = pow(edgeDensity * bit_depth_correction + 1, 0.1);
                                    //Increasing the QP of a block if its edge orientation lies around the multiples of 45 degree
                                    if ((avgAngle >= EDGE_INCLINATION - 15 && avgAngle <= EDGE_INCLINATION + 15) || (avgAngle >= EDGE_INCLINATION + 75 && avgAngle <= EDGE_INCLINATION + 105))
                                        curFrame->m_lowres.edgeInclined[blockXY] = 1;
                                    else
                                        curFrame->m_lowres.edgeInclined[blockXY] = 0;
                                }
                                else
                                {
                                    qp_adj = pow(energy * bit_depth_correction + 1, 0.1);
                                    curFrame->m_lowres.edgeInclined[blockXY] = 0;
                                }
                            }
                            else
                                qp_adj = pow(energy * bit_depth_correction + 1, 0.1);
                            curFrame->m_lowres.qpCuTreeOffset[blockXY] = qp_adj;
                            avg_adj += qp_adj;
                            avg_adj_pow2 += qp_adj * qp_adj;
                            blockXY++;
                        }
                    }
                    avg_adj /= blockCount;
                    avg_adj_pow2 /= blockCount;
                    strength = param->rc.aqStrength * avg_adj;
                    avg_adj = avg_adj - 0.5f * (avg_adj_pow2 - modeTwoConst) / avg_adj;
                    bias_strength = param->rc.aqStrength;
                }
                else
                    strength = param->rc.aqStrength * 1.0397f;

                blockXY = 0;
                for (int blockY = 0; blockY < maxRow; blockY += loopIncr)
                {
                    for (int blockX = 0; blockX < maxCol; blockX += loopIncr)
                    {
                        if (param->rc.aqMode == X265_AQ_AUTO_VARIANCE_BIASED)
                        {
                            qp_adj = curFrame->m_lowres.qpCuTreeOffset[blockXY];
                            qp_adj = strength * (qp_adj - avg_adj) + bias_strength * (1.f - modeTwoConst / (qp_adj * qp_adj));
                        }
                        else if (param->rc.aqMode == X265_AQ_AUTO_VARIANCE)
                        {
                            qp_adj = curFrame->m_lowres.qpCuTreeOffset[blockXY];
                            qp_adj = strength * (qp_adj - avg_adj);
                        }
                        else if (param->rc.aqMode == X265_AQ_EDGE)
                        {
                            inclinedEdge = curFrame->m_lowres.edgeInclined[blockXY];
                            qp_adj = curFrame->m_lowres.qpCuTreeOffset[blockXY];
                            if(inclinedEdge && (qp_adj - avg_adj > 0))
                                qp_adj = ((strength + AQ_EDGE_BIAS) * (qp_adj - avg_adj));
                            else
                                qp_adj = strength * (qp_adj - avg_adj);
                        }
                        else
                        {
                            uint32_t energy = acEnergyCu(curFrame, blockX, blockY, param->internalCsp, param->rc.qgSize);
                            qp_adj = strength * (X265_LOG2(X265_MAX(energy, 1)) - (modeOneConst + 2 * (X265_DEPTH - 8)));
                        }

                        if (param->bHDR10Opt)
                        {
                            uint32_t sum = lumaSumCu(curFrame, blockX, blockY, param->rc.qgSize);
                            uint32_t lumaAvg = sum / (loopIncr * loopIncr);
                            if (lumaAvg < 301)
                                qp_adj += 3;
                            else if (lumaAvg >= 301 && lumaAvg < 367)
                                qp_adj += 2;
                            else if (lumaAvg >= 367 && lumaAvg < 434)
                                qp_adj += 1;
                            else if (lumaAvg >= 501 && lumaAvg < 567)
                                qp_adj -= 1;
                            else if (lumaAvg >= 567 && lumaAvg < 634)
                                qp_adj -= 2;
                            else if (lumaAvg >= 634 && lumaAvg < 701)
                                qp_adj -= 3;
                            else if (lumaAvg >= 701 && lumaAvg < 767)
                                qp_adj -= 4;
                            else if (lumaAvg >= 767 && lumaAvg < 834)
                                qp_adj -= 5;
                            else if (lumaAvg >= 834)
                                qp_adj -= 6;
                        }
                        if (quantOffsets != NULL)
                            qp_adj += quantOffsets[blockXY];
                        curFrame->m_lowres.qpAqOffset[blockXY] = qp_adj;
                        curFrame->m_lowres.qpCuTreeOffset[blockXY] = qp_adj;
                        curFrame->m_lowres.invQscaleFactor[blockXY] = x265_exp2fix8(qp_adj);
                        blockXY++;
                    }
                }
            }
        }

        if (param->rc.qgSize == 8)
        {
            for (int cuY = 0; cuY < heightInCU; cuY++)
            {
                for (int cuX = 0; cuX < widthInCU; cuX++)
                {
                    const int cuXY = cuX + cuY * widthInCU;
                    curFrame->m_lowres.invQscaleFactor8x8[cuXY] = (curFrame->m_lowres.invQscaleFactor[cuX * 2 + cuY * widthInCU * 4] +
                        curFrame->m_lowres.invQscaleFactor[cuX * 2 + cuY * widthInCU * 4 + 1] +
                        curFrame->m_lowres.invQscaleFactor[cuX * 2 + cuY * widthInCU * 4 + curFrame->m_lowres.maxBlocksInRowFullRes] +
                        curFrame->m_lowres.invQscaleFactor[cuX * 2 + cuY * widthInCU * 4 + curFrame->m_lowres.maxBlocksInRowFullRes + 1]) / 4;
                }
            }
        }
    }

    if (param->bEnableWeightedPred || param->bEnableWeightedBiPred)
    {
        if (param->rc.bStatRead && param->rc.cuTree && IS_REFERENCED(curFrame))
        {
            for (int blockY = 0; blockY < maxRow; blockY += loopIncr)
                for (int blockX = 0; blockX < maxCol; blockX += loopIncr)
                    acEnergyCu(curFrame, blockX, blockY, param->internalCsp, param->rc.qgSize);
        }

        int hShift = CHROMA_H_SHIFT(param->internalCsp);
        int vShift = CHROMA_V_SHIFT(param->internalCsp);
        maxCol = ((maxCol + 8) >> 4) << 4;
        maxRow = ((maxRow + 8) >> 4) << 4;
        int width[3]  = { maxCol, maxCol >> hShift, maxCol >> hShift };
        int height[3] = { maxRow, maxRow >> vShift, maxRow >> vShift };

        for (int i = 0; i < 3; i++)
        {
            uint64_t sum, ssd;
            sum = curFrame->m_lowres.wp_sum[i];
            ssd = curFrame->m_lowres.wp_ssd[i];
            curFrame->m_lowres.wp_ssd[i] = ssd - (sum * sum + (width[i] * height[i]) / 2) / (width[i] * height[i]);
        }
    }

    if (param->bDynamicRefine || param->bEnableFades)
    {
        uint64_t blockXY = 0, rowVariance = 0;
        curFrame->m_lowres.frameVariance = 0;
        for (int blockY = 0; blockY < maxRow; blockY += loopIncr)
        {
            for (int blockX = 0; blockX < maxCol; blockX += loopIncr)
            {
                curFrame->m_lowres.blockVariance[blockXY] = acEnergyCu(curFrame, blockX, blockY, param->internalCsp, param->rc.qgSize);
                rowVariance += curFrame->m_lowres.blockVariance[blockXY];
                blockXY++;
            }
            curFrame->m_lowres.frameVariance += (rowVariance / maxCol);
        }
        curFrame->m_lowres.frameVariance /= maxRow;
    }
}

void LookaheadTLD::lowresIntraEstimate(Lowres& fenc, uint32_t qgSize)
{
    ALIGN_VAR_32(pixel, prediction[X265_LOWRES_CU_SIZE * X265_LOWRES_CU_SIZE]);
    pixel fencIntra[X265_LOWRES_CU_SIZE * X265_LOWRES_CU_SIZE];
    pixel neighbours[2][X265_LOWRES_CU_SIZE * 4 + 1];
    pixel* samples = neighbours[0], *filtered = neighbours[1];

    const int lookAheadLambda = (int)x265_lambda_tab[X265_LOOKAHEAD_QP];
    const int intraPenalty = 5 * lookAheadLambda;
    const int lowresPenalty = 4; /* fixed CU cost overhead */

    const int cuSize  = X265_LOWRES_CU_SIZE;
    const int cuSize2 = cuSize << 1;
    const int sizeIdx = X265_LOWRES_CU_BITS - 2;

    pixelcmp_t satd = primitives.pu[sizeIdx].satd;
    int planar = !!(cuSize >= 8);

    int costEst = 0, costEstAq = 0;

    for (int cuY = 0; cuY < heightInCU; cuY++)
    {
        fenc.rowSatds[0][0][cuY] = 0;

        for (int cuX = 0; cuX < widthInCU; cuX++)
        {
            const int cuXY = cuX + cuY * widthInCU;
            const intptr_t pelOffset = cuSize * cuX + cuSize * cuY * fenc.lumaStride;
            pixel *pixCur = fenc.lowresPlane[0] + pelOffset;

            /* copy fenc pixels */
            primitives.cu[sizeIdx].copy_pp(fencIntra, cuSize, pixCur, fenc.lumaStride);

            /* collect reference sample pixels */
            pixCur -= fenc.lumaStride + 1;
            memcpy(samples, pixCur, (2 * cuSize + 1) * sizeof(pixel)); /* top */
            for (int i = 1; i <= 2 * cuSize; i++)
                samples[cuSize2 + i] = pixCur[i * fenc.lumaStride];    /* left */

            primitives.cu[sizeIdx].intra_filter(samples, filtered);

            int cost, icost = me.COST_MAX;
            uint32_t ilowmode = 0;

            /* DC and planar */
            primitives.cu[sizeIdx].intra_pred[DC_IDX](prediction, cuSize, samples, 0, cuSize <= 16);
            cost = satd(fencIntra, cuSize, prediction, cuSize);
            COPY2_IF_LT(icost, cost, ilowmode, DC_IDX);

            primitives.cu[sizeIdx].intra_pred[PLANAR_IDX](prediction, cuSize, neighbours[planar], 0, 0);
            cost = satd(fencIntra, cuSize, prediction, cuSize);
            COPY2_IF_LT(icost, cost, ilowmode, PLANAR_IDX);

            /* scan angular predictions */
            int filter, acost = me.COST_MAX;
            uint32_t mode, alowmode = 4;
            for (mode = 5; mode < 35; mode += 5)
            {
                filter = !!(g_intraFilterFlags[mode] & cuSize);
                primitives.cu[sizeIdx].intra_pred[mode](prediction, cuSize, neighbours[filter], mode, cuSize <= 16);
                cost = satd(fencIntra, cuSize, prediction, cuSize);
                COPY2_IF_LT(acost, cost, alowmode, mode);
            }
            for (uint32_t dist = 2; dist >= 1; dist--)
            {
                int minusmode = alowmode - dist;
                int plusmode = alowmode + dist;

                mode = minusmode;
                filter = !!(g_intraFilterFlags[mode] & cuSize);
                primitives.cu[sizeIdx].intra_pred[mode](prediction, cuSize, neighbours[filter], mode, cuSize <= 16);
                cost = satd(fencIntra, cuSize, prediction, cuSize);
                COPY2_IF_LT(acost, cost, alowmode, mode);

                mode = plusmode;
                filter = !!(g_intraFilterFlags[mode] & cuSize);
                primitives.cu[sizeIdx].intra_pred[mode](prediction, cuSize, neighbours[filter], mode, cuSize <= 16);
                cost = satd(fencIntra, cuSize, prediction, cuSize);
                COPY2_IF_LT(acost, cost, alowmode, mode);
            }
            COPY2_IF_LT(icost, acost, ilowmode, alowmode);

            icost += intraPenalty + lowresPenalty; /* estimate intra signal cost */

            fenc.lowresCosts[0][0][cuXY] = (uint16_t)(X265_MIN(icost, LOWRES_COST_MASK) | (0 << LOWRES_COST_SHIFT));
            fenc.intraCost[cuXY] = icost;
            fenc.intraMode[cuXY] = (uint8_t)ilowmode;
            /* do not include edge blocks in the 
            frame cost estimates, they are not very accurate */
            const bool bFrameScoreCU = (cuX > 0 && cuX < widthInCU - 1 &&
                                        cuY > 0 && cuY < heightInCU - 1) || widthInCU <= 2 || heightInCU <= 2;
            int icostAq;
            if (qgSize == 8)
                icostAq = (bFrameScoreCU && fenc.invQscaleFactor) ? ((icost * fenc.invQscaleFactor8x8[cuXY] + 128) >> 8) : icost;
            else
                icostAq = (bFrameScoreCU && fenc.invQscaleFactor) ? ((icost * fenc.invQscaleFactor[cuXY] +128) >> 8) : icost;

            if (bFrameScoreCU)
            {
                costEst += icost;
                costEstAq += icostAq;
            }

            fenc.rowSatds[0][0][cuY] += icostAq;
        }
    }

    fenc.costEst[0][0] = costEst;
    fenc.costEstAq[0][0] = costEstAq;
}

uint32_t LookaheadTLD::weightCostLuma(Lowres& fenc, Lowres& ref, WeightParam& wp)
{
    pixel *src = ref.fpelPlane[0];
    intptr_t stride = fenc.lumaStride;

    if (wp.wtPresent)
    {
        int offset = wp.inputOffset << (X265_DEPTH - 8);
        int scale = wp.inputWeight;
        int denom = wp.log2WeightDenom;
        int round = denom ? 1 << (denom - 1) : 0;
        int correction = IF_INTERNAL_PREC - X265_DEPTH; // intermediate interpolation depth
        int widthHeight = (int)stride;

        primitives.weight_pp(ref.buffer[0], wbuffer[0], stride, widthHeight, paddedLines,
            scale, round << correction, denom + correction, offset);
        src = fenc.weightedRef[fenc.frameNum - ref.frameNum].fpelPlane[0];
    }

    uint32_t cost = 0;
    intptr_t pixoff = 0;
    int mb = 0;

    for (int y = 0; y < fenc.lines; y += 8, pixoff = y * stride)
    {
        for (int x = 0; x < fenc.width; x += 8, mb++, pixoff += 8)
        {
            int satd = primitives.pu[LUMA_8x8].satd(src + pixoff, stride, fenc.fpelPlane[0] + pixoff, stride);
            cost += X265_MIN(satd, fenc.intraCost[mb]);
        }
    }

    return cost;
}

bool LookaheadTLD::allocWeightedRef(Lowres& fenc)
{
    intptr_t planesize = fenc.buffer[1] - fenc.buffer[0];
    paddedLines = (int)(planesize / fenc.lumaStride);

    wbuffer[0] = X265_MALLOC(pixel, 4 * planesize);
    if (wbuffer[0])
    {
        wbuffer[1] = wbuffer[0] + planesize;
        wbuffer[2] = wbuffer[1] + planesize;
        wbuffer[3] = wbuffer[2] + planesize;
    }
    else
        return false;

    return true;
}

void LookaheadTLD::weightsAnalyse(Lowres& fenc, Lowres& ref)
{
    static const float epsilon = 1.f / 128.f;
    int deltaIndex = fenc.frameNum - ref.frameNum;

    WeightParam wp;
    wp.wtPresent = 0;

    if (!wbuffer[0])
    {
        if (!allocWeightedRef(fenc))
            return;
    }

    ReferencePlanes& weightedRef = fenc.weightedRef[deltaIndex];
    intptr_t padoffset = fenc.lowresPlane[0] - fenc.buffer[0];
    for (int i = 0; i < 4; i++)
        weightedRef.lowresPlane[i] = wbuffer[i] + padoffset;

    weightedRef.fpelPlane[0] = weightedRef.lowresPlane[0];
    weightedRef.lumaStride = fenc.lumaStride;
    weightedRef.isLowres = true;
    weightedRef.isWeighted = false;
    weightedRef.isHMELowres = ref.bEnableHME;

    /* epsilon is chosen to require at least a numerator of 127 (with denominator = 128) */
    float guessScale, fencMean, refMean;
    x265_emms();
    if (fenc.wp_ssd[0] && ref.wp_ssd[0])
        guessScale = sqrtf((float)fenc.wp_ssd[0] / ref.wp_ssd[0]);
    else
        guessScale = 1.0f;
    fencMean = (float)fenc.wp_sum[0] / (fenc.lines * fenc.width) / (1 << (X265_DEPTH - 8));
    refMean = (float)ref.wp_sum[0] / (fenc.lines * fenc.width) / (1 << (X265_DEPTH - 8));

    /* Early termination */
    if (fabsf(refMean - fencMean) < 0.5f && fabsf(1.f - guessScale) < epsilon)
        return;

    int minoff = 0, minscale, mindenom;
    unsigned int minscore = 0, origscore = 1;
    int found = 0;

    wp.setFromWeightAndOffset((int)(guessScale * 128 + 0.5f), 0, 7, true);
    mindenom = wp.log2WeightDenom;
    minscale = wp.inputWeight;

    origscore = minscore = weightCostLuma(fenc, ref, wp);

    if (!minscore)
        return;

    unsigned int s = 0;
    int curScale = minscale;
    int curOffset = (int)(fencMean - refMean * curScale / (1 << mindenom) + 0.5f);
    if (curOffset < -128 || curOffset > 127)
    {
        /* Rescale considering the constraints on curOffset. We do it in this order
        * because scale has a much wider range than offset (because of denom), so
        * it should almost never need to be clamped. */
        curOffset = x265_clip3(-128, 127, curOffset);
        curScale = (int)((1 << mindenom) * (fencMean - curOffset) / refMean + 0.5f);
        curScale = x265_clip3(0, 127, curScale);
    }
    SET_WEIGHT(wp, true, curScale, mindenom, curOffset);
    s = weightCostLuma(fenc, ref, wp);
    COPY4_IF_LT(minscore, s, minscale, curScale, minoff, curOffset, found, 1);

    /* Use a smaller denominator if possible */
    if (mindenom > 0 && !(minscale & 1))
    {
        unsigned long idx;
        CTZ(idx, minscale);
        int shift = X265_MIN((int)idx, mindenom);
        mindenom -= shift;
        minscale >>= shift;
    }

    if (!found || (minscale == 1 << mindenom && minoff == 0) || (float)minscore / origscore > 0.998f)
        return;
    else
    {
        SET_WEIGHT(wp, true, minscale, mindenom, minoff);

        // set weighted delta cost
        fenc.weightedCostDelta[deltaIndex] = minscore / origscore;

        int offset = wp.inputOffset << (X265_DEPTH - 8);
        int scale = wp.inputWeight;
        int denom = wp.log2WeightDenom;
        int round = denom ? 1 << (denom - 1) : 0;
        int correction = IF_INTERNAL_PREC - X265_DEPTH; // intermediate interpolation depth
        intptr_t stride = ref.lumaStride;
        int widthHeight = (int)stride;

        for (int i = 0; i < 4; i++)
            primitives.weight_pp(ref.buffer[i], wbuffer[i], stride, widthHeight, paddedLines,
            scale, round << correction, denom + correction, offset);

        weightedRef.isWeighted = true;
    }
}

Lookahead::Lookahead(x265_param *param, ThreadPool* pool)
{
    m_param = param;
    m_pool  = pool;

    m_lastNonB = NULL;
    m_isSceneTransition = false;
    m_scratch  = NULL;
    m_tld      = NULL;
    m_filled   = false;
    m_outputSignalRequired = false;
    m_isActive = true;
    m_inputCount = 0;
    m_extendGopBoundary = false;
    m_8x8Height = ((m_param->sourceHeight / 2) + X265_LOWRES_CU_SIZE - 1) >> X265_LOWRES_CU_BITS;
    m_8x8Width = ((m_param->sourceWidth / 2) + X265_LOWRES_CU_SIZE - 1) >> X265_LOWRES_CU_BITS;
    m_4x4Height = ((m_param->sourceHeight / 4) + X265_LOWRES_CU_SIZE - 1) >> X265_LOWRES_CU_BITS;
    m_4x4Width = ((m_param->sourceWidth / 4) + X265_LOWRES_CU_SIZE - 1) >> X265_LOWRES_CU_BITS;
    m_cuCount = m_8x8Width * m_8x8Height;
    m_8x8Blocks = m_8x8Width > 2 && m_8x8Height > 2 ? (m_cuCount + 4 - 2 * (m_8x8Width + m_8x8Height)) : m_cuCount;
    m_isFadeIn = false;
    m_fadeCount = 0;
    m_fadeStart = -1;

    /* Allow the strength to be adjusted via qcompress, since the two concepts
     * are very similar. */
    m_cuTreeStrength = (m_param->rc.hevcAq ? 6.0 : 5.0) * (1.0 - m_param->rc.qCompress);

    m_lastKeyframe = -m_param->keyframeMax;
    m_sliceTypeBusy = false;
    m_fullQueueSize = X265_MAX(1, m_param->lookaheadDepth);
    m_bAdaptiveQuant = m_param->rc.aqMode ||
                       m_param->bEnableWeightedPred ||
                       m_param->bEnableWeightedBiPred ||
                       m_param->bAQMotion ||
                       m_param->rc.hevcAq;

    /* If we have a thread pool and are using --b-adapt 2, it is generally
     * preferable to perform all motion searches for each lowres frame in large
     * batched; this will create one job per --bframe per lowres frame, and
     * these jobs are performed by workers bonded to the thread running
     * slicetypeDecide() */
    m_bBatchMotionSearch = m_pool && m_param->bFrameAdaptive == X265_B_ADAPT_TRELLIS;

    /* It is also beneficial to pre-calculate all possible frame cost estimates
     * using worker threads bonded to the worker thread running
     * slicetypeDecide(). This creates bframes * bframes jobs which take less
     * time than the motion search batches but there are many of them. This may
     * do much unnecessary work, some frame cost estimates are not needed, so if
     * the thread pool is small we disable this feature after the initial burst
     * of work */
    m_bBatchFrameCosts = m_bBatchMotionSearch;

    if (m_param->lookaheadSlices && !m_pool)
    {
        x265_log(param, X265_LOG_WARNING, "No pools found; disabling lookahead-slices\n");
        m_param->lookaheadSlices = 0;
    }

    if (m_param->lookaheadSlices && (m_param->sourceHeight < 720))
    {
        x265_log(param, X265_LOG_WARNING, "Source height < 720p; disabling lookahead-slices\n");
        m_param->lookaheadSlices = 0;
    }

    if (m_param->lookaheadSlices > 1)
    {
        m_numRowsPerSlice = m_8x8Height / m_param->lookaheadSlices;
        m_numRowsPerSlice = X265_MAX(m_numRowsPerSlice, 10);            // at least 10 rows per slice
        m_numRowsPerSlice = X265_MIN(m_numRowsPerSlice, m_8x8Height);   // but no more than the full picture
        m_numCoopSlices = m_8x8Height / m_numRowsPerSlice;
        m_param->lookaheadSlices = m_numCoopSlices;                     // report actual final slice count
    }
    else
    {
        m_numRowsPerSlice = m_8x8Height;
        m_numCoopSlices = 1;
    }
    if (param->gopLookahead && (param->gopLookahead > (param->lookaheadDepth - param->bframes - 2)))
    {
        param->gopLookahead = X265_MAX(0, param->lookaheadDepth - param->bframes - 2);
        x265_log(param, X265_LOG_WARNING, "Gop-lookahead cannot be greater than (rc-lookahead - length of the mini-gop); Clipping gop-lookahead to %d\n", param->gopLookahead);
    }
#if DETAILED_CU_STATS
    m_slicetypeDecideElapsedTime = 0;
    m_preLookaheadElapsedTime = 0;
    m_countSlicetypeDecide = 0;
    m_countPreLookahead = 0;
#endif

    m_accHistDiffRunningAvgCb = X265_MALLOC(uint32_t*, NUMBER_OF_SEGMENTS_IN_WIDTH * sizeof(uint32_t*));
    m_accHistDiffRunningAvgCb[0] = X265_MALLOC(uint32_t, NUMBER_OF_SEGMENTS_IN_WIDTH * NUMBER_OF_SEGMENTS_IN_HEIGHT);
    memset(m_accHistDiffRunningAvgCb[0], 0, sizeof(uint32_t) * NUMBER_OF_SEGMENTS_IN_WIDTH * NUMBER_OF_SEGMENTS_IN_HEIGHT);
    for (uint32_t w = 1; w < NUMBER_OF_SEGMENTS_IN_WIDTH; w++) {
        m_accHistDiffRunningAvgCb[w] = m_accHistDiffRunningAvgCb[0] + w * NUMBER_OF_SEGMENTS_IN_HEIGHT;
    }

    m_accHistDiffRunningAvgCr = X265_MALLOC(uint32_t*, NUMBER_OF_SEGMENTS_IN_WIDTH * sizeof(uint32_t*));
    m_accHistDiffRunningAvgCr[0] = X265_MALLOC(uint32_t, NUMBER_OF_SEGMENTS_IN_WIDTH * NUMBER_OF_SEGMENTS_IN_HEIGHT);
    memset(m_accHistDiffRunningAvgCr[0], 0, sizeof(uint32_t) * NUMBER_OF_SEGMENTS_IN_WIDTH * NUMBER_OF_SEGMENTS_IN_HEIGHT);
    for (uint32_t w = 1; w < NUMBER_OF_SEGMENTS_IN_WIDTH; w++) {
        m_accHistDiffRunningAvgCr[w] = m_accHistDiffRunningAvgCr[0] + w * NUMBER_OF_SEGMENTS_IN_HEIGHT;
    }

    m_accHistDiffRunningAvg = X265_MALLOC(uint32_t*, NUMBER_OF_SEGMENTS_IN_WIDTH * sizeof(uint32_t*));
    m_accHistDiffRunningAvg[0] = X265_MALLOC(uint32_t, NUMBER_OF_SEGMENTS_IN_WIDTH * NUMBER_OF_SEGMENTS_IN_HEIGHT);
    memset(m_accHistDiffRunningAvg[0], 0, sizeof(uint32_t) * NUMBER_OF_SEGMENTS_IN_WIDTH * NUMBER_OF_SEGMENTS_IN_HEIGHT);
    for (uint32_t w = 1; w < NUMBER_OF_SEGMENTS_IN_WIDTH; w++) {
        m_accHistDiffRunningAvg[w] = m_accHistDiffRunningAvg[0] + w * NUMBER_OF_SEGMENTS_IN_HEIGHT;
    }

    m_resetRunningAvg = true;

    m_segmentCountThreshold = (uint32_t)(((float)((NUMBER_OF_SEGMENTS_IN_WIDTH * NUMBER_OF_SEGMENTS_IN_HEIGHT) * 50) / 100) + 0.5);

    if (m_param->bEnableTemporalSubLayers > 2)
    {
        switch (m_param->bEnableTemporalSubLayers)
        {
        case 3:
            m_gopId = 0;
            break;
        case 4:
            m_gopId = 1;
            break;
        case 5:
            m_gopId = 2;
            break;
        default:
            break;
        }
    }
}

#if DETAILED_CU_STATS
void Lookahead::getWorkerStats(int64_t& batchElapsedTime, uint64_t& batchCount, int64_t& coopSliceElapsedTime, uint64_t& coopSliceCount)
{
    batchElapsedTime = coopSliceElapsedTime = 0;
    coopSliceCount = batchCount = 0;
    int tldCount = m_pool ? m_pool->m_numWorkers : 1;
    for (int i = 0; i < tldCount; i++)
    {
        batchElapsedTime += m_tld[i].batchElapsedTime;
        coopSliceElapsedTime += m_tld[i].coopSliceElapsedTime;
        batchCount += m_tld[i].countBatches;
        coopSliceCount += m_tld[i].countCoopSlices;
    }
}
#endif

bool Lookahead::create()
{
    int numTLD = 1 + (m_pool ? m_pool->m_numWorkers : 0);
    m_tld = new LookaheadTLD[numTLD];
    for (int i = 0; i < numTLD; i++)
        m_tld[i].init(m_8x8Width, m_8x8Height, m_8x8Blocks);
    m_scratch = X265_MALLOC(int, m_tld[0].widthInCU);

    return m_tld && m_scratch;
}

void Lookahead::stopJobs()
{
    if (m_pool && !m_inputQueue.empty())
    {
        m_inputLock.acquire();
        m_isActive = false;
        bool wait = m_outputSignalRequired = m_sliceTypeBusy;
        m_inputLock.release();

        if (wait)
            m_outputSignal.wait();
    }
    if (m_pool && m_param->lookaheadThreads > 0)
    {
        for (int i = 0; i < m_numPools; i++)
            m_pool[i].stopWorkers();
    }
}

void Lookahead::destroy()
{
    // these two queues will be empty unless the encode was aborted
    while (!m_inputQueue.empty())
    {
        Frame* curFrame = m_inputQueue.popFront();
        curFrame->destroy();
        delete curFrame;
    }

    while (!m_outputQueue.empty())
    {
        Frame* curFrame = m_outputQueue.popFront();
        curFrame->destroy();
        delete curFrame;
    }

    X265_FREE(m_scratch);
    delete [] m_tld;
    if (m_param->lookaheadThreads > 0)
        delete [] m_pool;
}
/* The synchronization of slicetypeDecide is managed here.  The findJob() method
 * polls the occupancy of the input queue. If the queue is
 * full, it will run slicetypeDecide() and output a mini-gop of frames to the
 * output queue. If the flush() method has been called (implying no new pictures
 * will be received) then the input queue is considered full if it has even one
 * picture left. getDecidedPicture() removes pictures from the output queue and
 * only blocks as a last resort. It does not start removing pictures until
 * m_filled is true, which occurs after *more than* the lookahead depth of
 * pictures have been input so slicetypeDecide() should have started prior to
 * output pictures being withdrawn. The first slicetypeDecide() will obviously
 * still require a blocking wait, but after this slicetypeDecide() will maintain
 * its lead over the encoder (because one picture is added to the input queue
 * each time one is removed from the output) and decides slice types of pictures
 * just ahead of when the encoder needs them */

/* Called by API thread */
void Lookahead::addPicture(Frame& curFrame, int sliceType)
{
    if (m_param->analysisLoad && m_param->bDisableLookahead)
    {
        if (!m_filled)
            m_filled = true;
        m_outputLock.acquire();
        m_outputQueue.pushBack(curFrame);
        m_outputLock.release();
        m_inputCount++;
    }
    else
    {
        checkLookaheadQueue(m_inputCount);
        curFrame.m_lowres.sliceType = sliceType;
        addPicture(curFrame);
    }
}

void Lookahead::addPicture(Frame& curFrame)
{
    m_inputLock.acquire();
    m_inputQueue.pushBack(curFrame);
    m_inputLock.release();
    m_inputCount++;
}

void Lookahead::checkLookaheadQueue(int &frameCnt)
{
    /* determine if the lookahead is (over) filled enough for frames to begin to
     * be consumed by frame encoders */
    if (!m_filled)
    {
        if (!m_param->bframes & !m_param->lookaheadDepth)
            m_filled = true; /* zero-latency */
        else if (frameCnt >= m_param->lookaheadDepth + 2 + m_param->bframes)
            m_filled = true; /* full capacity plus mini-gop lag */
    }

    m_inputLock.acquire();
    if (m_pool && m_inputQueue.size() >= m_fullQueueSize)
        tryWakeOne();
    m_inputLock.release();
}

/* Called by API thread */
void Lookahead::flush()
{
    /* force slicetypeDecide to run until the input queue is empty */
    m_fullQueueSize = 1;
    m_filled = true;
}

void Lookahead::setLookaheadQueue()
{
    m_filled = false;
    m_fullQueueSize = X265_MAX(1, m_param->lookaheadDepth);
}

void Lookahead::findJob(int /*workerThreadID*/)
{
    bool doDecide;

    m_inputLock.acquire();
    if (m_inputQueue.size() >= m_fullQueueSize && !m_sliceTypeBusy && m_isActive)
        doDecide = m_sliceTypeBusy = true;
    else
        doDecide = m_helpWanted = false;
    m_inputLock.release();

    if (!doDecide)
        return;

    ProfileLookaheadTime(m_slicetypeDecideElapsedTime, m_countSlicetypeDecide);
    ProfileScopeEvent(slicetypeDecideEV);

    slicetypeDecide();

    m_inputLock.acquire();
    if (m_outputSignalRequired)
    {
        m_outputSignal.trigger();
        m_outputSignalRequired = false;
    }
    m_sliceTypeBusy = false;
    m_inputLock.release();
}

/* Called by API thread */
Frame* Lookahead::getDecidedPicture()
{
    if (m_filled)
    {
        m_outputLock.acquire();
        Frame *out = m_outputQueue.popFront();
        m_outputLock.release();

        if (out)
        {
            m_inputCount--;
            return out;
        }

        if (m_param->analysisLoad && m_param->bDisableLookahead)
            return NULL;

        findJob(-1); /* run slicetypeDecide() if necessary */

        m_inputLock.acquire();
        bool wait = m_outputSignalRequired = m_sliceTypeBusy;
        m_inputLock.release();

        if (wait)
            m_outputSignal.wait();

        out = m_outputQueue.popFront();
        if (out)
            m_inputCount--;
        return out;
    }
    else
        return NULL;
}

/* Called by rate-control to calculate the estimated SATD cost for a given
 * picture.  It assumes dpb->prepareEncode() has already been called for the
 * picture and all the references are established */
void Lookahead::getEstimatedPictureCost(Frame *curFrame)
{
    Lowres *frames[X265_LOOKAHEAD_MAX];

    // POC distances to each reference
    Slice *slice = curFrame->m_encData->m_slice;
    int p0 = 0, p1, b;
    int poc = slice->m_poc;
    int l0poc = slice->m_rps.numberOfNegativePictures ? slice->m_refPOCList[0][0] : -1;
    int l1poc = slice->m_refPOCList[1][0];

    switch (slice->m_sliceType)
    {
    case I_SLICE:
        frames[p0] = &curFrame->m_lowres;
        b = p1 = 0;
        break;

    case P_SLICE:
        b = p1 = poc - l0poc;
        frames[p0] = &slice->m_refFrameList[0][0]->m_lowres;
        frames[b] = &curFrame->m_lowres;
        break;

    case B_SLICE:
        if (l0poc >= 0)
        {
            b = poc - l0poc;
            p1 = b + l1poc - poc;
            frames[p0] = &slice->m_refFrameList[0][0]->m_lowres;
            frames[b] = &curFrame->m_lowres;
            frames[p1] = &slice->m_refFrameList[1][0]->m_lowres;
        }
        else 
        {
            p0 = b = 0;
            p1 = b + l1poc - poc;
            frames[p0] = frames[b] = &curFrame->m_lowres;
            frames[p1] = &slice->m_refFrameList[1][0]->m_lowres;
        }
        
        break;

    default:
        return;
    }
    if (!m_param->analysisLoad || !m_param->bDisableLookahead)
    {
        X265_CHECK(curFrame->m_lowres.costEst[b - p0][p1 - b] > 0, "Slice cost not estimated\n")

        if (m_param->rc.cuTree && !m_param->rc.bStatRead)
            /* update row satds based on cutree offsets */
            curFrame->m_lowres.satdCost = frameCostRecalculate(frames, p0, p1, b);
        else if (!m_param->analysisLoad || m_param->scaleFactor || m_param->bAnalysisType == HEVC_INFO)
        {
            if (m_param->rc.aqMode)
                curFrame->m_lowres.satdCost = curFrame->m_lowres.costEstAq[b - p0][p1 - b];
            else
                curFrame->m_lowres.satdCost = curFrame->m_lowres.costEst[b - p0][p1 - b];
        }
        if (m_param->rc.vbvBufferSize && m_param->rc.vbvMaxBitrate)
        {
            /* aggregate lowres row satds to CTU resolution */
            curFrame->m_lowres.lowresCostForRc = curFrame->m_lowres.lowresCosts[b - p0][p1 - b];
            uint32_t lowresRow = 0, lowresCol = 0, lowresCuIdx = 0, sum = 0, intraSum = 0;
            uint32_t scale = m_param->maxCUSize / (2 * X265_LOWRES_CU_SIZE);
            uint32_t numCuInHeight = (m_param->sourceHeight + m_param->maxCUSize - 1) / m_param->maxCUSize;
            uint32_t widthInLowresCu = (uint32_t)m_8x8Width, heightInLowresCu = (uint32_t)m_8x8Height;
            double *qp_offset = 0;
            /* Factor in qpoffsets based on Aq/Cutree in CU costs */
            if (m_param->rc.aqMode || m_param->bAQMotion)
                qp_offset = (frames[b]->sliceType == X265_TYPE_B || !m_param->rc.cuTree) ? frames[b]->qpAqOffset : frames[b]->qpCuTreeOffset;

            for (uint32_t row = 0; row < numCuInHeight; row++)
            {
                lowresRow = row * scale;
                for (uint32_t cnt = 0; cnt < scale && lowresRow < heightInLowresCu; lowresRow++, cnt++)
                {
                    sum = 0; intraSum = 0;
                    int diff = 0;
                    lowresCuIdx = lowresRow * widthInLowresCu;
                    for (lowresCol = 0; lowresCol < widthInLowresCu; lowresCol++, lowresCuIdx++)
                    {
                        uint16_t lowresCuCost = curFrame->m_lowres.lowresCostForRc[lowresCuIdx] & LOWRES_COST_MASK;
                        if (qp_offset)
                        {
                            double qpOffset;
                            if (m_param->rc.qgSize == 8)
                                qpOffset = (qp_offset[lowresCol * 2 + lowresRow * widthInLowresCu * 4] +
                                qp_offset[lowresCol * 2 + lowresRow * widthInLowresCu * 4 + 1] +
                                qp_offset[lowresCol * 2 + lowresRow * widthInLowresCu * 4 + curFrame->m_lowres.maxBlocksInRowFullRes] +
                                qp_offset[lowresCol * 2 + lowresRow * widthInLowresCu * 4 + curFrame->m_lowres.maxBlocksInRowFullRes + 1]) / 4;
                            else
                                qpOffset = qp_offset[lowresCuIdx];
                            lowresCuCost = (uint16_t)((lowresCuCost * x265_exp2fix8(qpOffset) + 128) >> 8);
                            int32_t intraCuCost = curFrame->m_lowres.intraCost[lowresCuIdx];
                            curFrame->m_lowres.intraCost[lowresCuIdx] = (intraCuCost * x265_exp2fix8(qpOffset) + 128) >> 8;
                        }
                        if (m_param->bIntraRefresh && slice->m_sliceType == X265_TYPE_P)
                            for (uint32_t x = curFrame->m_encData->m_pir.pirStartCol; x <= curFrame->m_encData->m_pir.pirEndCol; x++)
                                diff += curFrame->m_lowres.intraCost[lowresCuIdx] - lowresCuCost;
                        curFrame->m_lowres.lowresCostForRc[lowresCuIdx] = lowresCuCost;
                        sum += lowresCuCost;
                        intraSum += curFrame->m_lowres.intraCost[lowresCuIdx];
                    }
                    curFrame->m_encData->m_rowStat[row].satdForVbv += sum;
                    curFrame->m_encData->m_rowStat[row].satdForVbv += diff;
                    curFrame->m_encData->m_rowStat[row].intraSatdForVbv += intraSum;
                }
            }
        }
    }
}

uint32_t LookaheadTLD::calcVariance(pixel* inpSrc, intptr_t stride, intptr_t blockOffset, uint32_t plane)
{
    pixel* src = inpSrc + blockOffset;

    uint32_t var;
    if (!plane)
        var = acEnergyVarHist(primitives.cu[BLOCK_8x8].var(src, stride), 6);
    else
        var = acEnergyVarHist(primitives.cu[BLOCK_4x4].var(src, stride), 4);

    x265_emms();
    return var;
}

/*
** Compute Block and Picture Variance, Block Mean for all blocks in the picture
*/
void LookaheadTLD::computePictureStatistics(Frame *curFrame)
{
    int maxCol = curFrame->m_fencPic->m_picWidth;
    int maxRow = curFrame->m_fencPic->m_picHeight;
    intptr_t inpStride = curFrame->m_fencPic->m_stride;

    // Variance
    uint64_t picTotVariance = 0;
    uint32_t variance;

    uint64_t blockXY = 0;
    pixel* src = curFrame->m_fencPic->m_picOrg[0];

    for (int blockY = 0; blockY < maxRow; blockY += 8)
    {
        uint64_t rowVariance = 0;
        for (int blockX = 0; blockX < maxCol; blockX += 8)
        {
            intptr_t blockOffsetLuma = blockX + (blockY * inpStride);

            variance = calcVariance(
                src,
                inpStride,
                blockOffsetLuma, 0);

            rowVariance += variance;
            blockXY++;
        }
        picTotVariance += (uint16_t)(rowVariance / maxCol);
    }

    curFrame->m_lowres.picAvgVariance = (uint16_t)(picTotVariance / maxRow);

    // Collect chroma variance
    int hShift = curFrame->m_fencPic->m_hChromaShift;
    int vShift = curFrame->m_fencPic->m_vChromaShift;

    int maxColChroma = curFrame->m_fencPic->m_picWidth >> hShift;
    int maxRowChroma = curFrame->m_fencPic->m_picHeight >> vShift;
    intptr_t cStride = curFrame->m_fencPic->m_strideC;

    pixel* srcCb = curFrame->m_fencPic->m_picOrg[1];

    picTotVariance = 0;
    for (int blockY = 0; blockY < maxRowChroma; blockY += 4)
    {
        uint64_t rowVariance = 0;
        for (int blockX = 0; blockX < maxColChroma; blockX += 4)
        {
            intptr_t blockOffsetChroma = blockX + blockY * cStride;

            variance = calcVariance(
                srcCb,
                cStride,
                blockOffsetChroma, 1);

            rowVariance += variance;
            blockXY++;
        }
        picTotVariance += (uint16_t)(rowVariance / maxColChroma);
    }

    curFrame->m_lowres.picAvgVarianceCb = (uint16_t)(picTotVariance / maxRowChroma);


    pixel* srcCr = curFrame->m_fencPic->m_picOrg[2];

    picTotVariance = 0;
    for (int blockY = 0; blockY < maxRowChroma; blockY += 4)
    {
        uint64_t rowVariance = 0;
        for (int blockX = 0; blockX < maxColChroma; blockX += 4)
        {
            intptr_t blockOffsetChroma = blockX + blockY * cStride;

            variance = calcVariance(
                srcCr,
                cStride,
                blockOffsetChroma, 2);

            rowVariance += variance;
            blockXY++;
        }
        picTotVariance += (uint16_t)(rowVariance / maxColChroma);
    }

    curFrame->m_lowres.picAvgVarianceCr = (uint16_t)(picTotVariance / maxRowChroma);
}

/*
* Compute histogram of n-bins for the input
*/
void LookaheadTLD::calculateHistogram(
    pixel     *inputSrc,
    uint32_t   inputWidth,
    uint32_t   inputHeight,
    intptr_t   stride,
    uint8_t    dsFactor,
    uint32_t  *histogram,
    uint64_t  *sum)

{
    *sum = 0;

    for (uint32_t verticalIdx = 0; verticalIdx < inputHeight; verticalIdx += dsFactor)
    {
        for (uint32_t horizontalIdx = 0; horizontalIdx < inputWidth; horizontalIdx += dsFactor)
        {
            ++(histogram[inputSrc[horizontalIdx]]);
            *sum += inputSrc[horizontalIdx];
        }
        inputSrc += (stride << (dsFactor >> 1));
    }

    return;
}

/*
* Compute histogram bins and chroma pixel intensity *
*/
void LookaheadTLD::computeIntensityHistogramBinsChroma(
    Frame    *curFrame,
    uint64_t *sumAverageIntensityCb,
    uint64_t *sumAverageIntensityCr)
{
    uint64_t    sum;
    uint8_t     dsFactor = 4;

    uint32_t segmentWidth = curFrame->m_lowres.widthFullRes / NUMBER_OF_SEGMENTS_IN_WIDTH;
    uint32_t segmentHeight = curFrame->m_lowres.heightFullRes / NUMBER_OF_SEGMENTS_IN_HEIGHT;

    for (uint32_t segmentInFrameWidthIndex = 0; segmentInFrameWidthIndex < NUMBER_OF_SEGMENTS_IN_WIDTH; segmentInFrameWidthIndex++)
    {
        for (uint32_t segmentInFrameHeightIndex = 0; segmentInFrameHeightIndex < NUMBER_OF_SEGMENTS_IN_HEIGHT; segmentInFrameHeightIndex++)
        {
            // Initialize bins to 1
            for (uint32_t cuIndex = 0; cuIndex < 256; cuIndex++) {
                curFrame->m_lowres.picHistogram[segmentInFrameWidthIndex][segmentInFrameHeightIndex][1][cuIndex] = 1;
                curFrame->m_lowres.picHistogram[segmentInFrameWidthIndex][segmentInFrameHeightIndex][2][cuIndex] = 1;
            }

            uint32_t segmentWidthOffset = (segmentInFrameWidthIndex == NUMBER_OF_SEGMENTS_IN_WIDTH - 1) ?
                curFrame->m_lowres.widthFullRes - (NUMBER_OF_SEGMENTS_IN_WIDTH * segmentWidth) : 0;

            uint32_t segmentHeightOffset = (segmentInFrameHeightIndex == NUMBER_OF_SEGMENTS_IN_HEIGHT - 1) ?
                curFrame->m_lowres.heightFullRes - (NUMBER_OF_SEGMENTS_IN_HEIGHT * segmentHeight) : 0;


            // U Histogram
            calculateHistogram(
                curFrame->m_fencPic->m_picOrg[1] + ((segmentInFrameWidthIndex * segmentWidth) >> 1) + (((segmentInFrameHeightIndex * segmentHeight) >> 1) * curFrame->m_fencPic->m_strideC),
                (segmentWidth + segmentWidthOffset) >> 1,
                (segmentHeight + segmentHeightOffset) >> 1,
                curFrame->m_fencPic->m_strideC,
                dsFactor,
                curFrame->m_lowres.picHistogram[segmentInFrameWidthIndex][segmentInFrameHeightIndex][1],
                &sum);

            sum = (sum << dsFactor);
            *sumAverageIntensityCb += sum;
            curFrame->m_lowres.averageIntensityPerSegment[segmentInFrameWidthIndex][segmentInFrameHeightIndex][1] =
                (uint8_t)((sum + (((segmentWidth + segmentWidthOffset) * (segmentHeight + segmentHeightOffset)) >> 3)) / (((segmentWidth + segmentWidthOffset) * (segmentHeight + segmentHeightOffset)) >> 2));

            for (uint16_t histogramBin = 0; histogramBin < HISTOGRAM_NUMBER_OF_BINS; histogramBin++) {
                curFrame->m_lowres.picHistogram[segmentInFrameWidthIndex][segmentInFrameHeightIndex][1][histogramBin] =
                    curFrame->m_lowres.picHistogram[segmentInFrameWidthIndex][segmentInFrameHeightIndex][1][histogramBin] << dsFactor;
            }

            // V Histogram
            calculateHistogram(
                curFrame->m_fencPic->m_picOrg[2] + ((segmentInFrameWidthIndex * segmentWidth) >> 1) + (((segmentInFrameHeightIndex * segmentHeight) >> 1) * curFrame->m_fencPic->m_strideC),
                (segmentWidth + segmentWidthOffset) >> 1,
                (segmentHeight + segmentHeightOffset) >> 1,
                curFrame->m_fencPic->m_strideC,
                dsFactor,
                curFrame->m_lowres.picHistogram[segmentInFrameWidthIndex][segmentInFrameHeightIndex][2],
                &sum);

            sum = (sum << dsFactor);
            *sumAverageIntensityCr += sum;
            curFrame->m_lowres.averageIntensityPerSegment[segmentInFrameWidthIndex][segmentInFrameHeightIndex][2] =
                (uint8_t)((sum + (((segmentWidth + segmentWidthOffset) * (segmentHeight + segmentHeightOffset)) >> 3)) / (((segmentWidth + segmentHeightOffset) * (segmentHeight + segmentHeightOffset)) >> 2));

            for (uint16_t histogramBin = 0; histogramBin < HISTOGRAM_NUMBER_OF_BINS; histogramBin++) {
                curFrame->m_lowres.picHistogram[segmentInFrameWidthIndex][segmentInFrameHeightIndex][2][histogramBin] =
                    curFrame->m_lowres.picHistogram[segmentInFrameWidthIndex][segmentInFrameHeightIndex][2][histogramBin] << dsFactor;
            }
        }
    }
    return;

}

/*
* Compute histogram bins and luma pixel intensity *
*/
void LookaheadTLD::computeIntensityHistogramBinsLuma(
    Frame    *curFrame,
    uint64_t *sumAvgIntensityTotalSegmentsLuma)
{
    uint64_t sum;

    uint32_t segmentWidth = curFrame->m_lowres.quarterSampleLowResWidth / NUMBER_OF_SEGMENTS_IN_WIDTH;
    uint32_t segmentHeight = curFrame->m_lowres.quarterSampleLowResHeight / NUMBER_OF_SEGMENTS_IN_HEIGHT;

    for (uint32_t segmentInFrameWidthIndex = 0; segmentInFrameWidthIndex < NUMBER_OF_SEGMENTS_IN_WIDTH; segmentInFrameWidthIndex++)
    {
        for (uint32_t segmentInFrameHeightIndex = 0; segmentInFrameHeightIndex < NUMBER_OF_SEGMENTS_IN_HEIGHT; segmentInFrameHeightIndex++)
        {
            // Initialize bins to 1
            for (uint32_t cuIndex = 0; cuIndex < 256; cuIndex++) {
                curFrame->m_lowres.picHistogram[segmentInFrameWidthIndex][segmentInFrameHeightIndex][0][cuIndex] = 1;
            }

            uint32_t segmentWidthOffset = (segmentInFrameWidthIndex == NUMBER_OF_SEGMENTS_IN_WIDTH - 1) ?
                curFrame->m_lowres.quarterSampleLowResWidth - (NUMBER_OF_SEGMENTS_IN_WIDTH * segmentWidth) : 0;

            uint32_t segmentHeightOffset = (segmentInFrameHeightIndex == NUMBER_OF_SEGMENTS_IN_HEIGHT - 1) ?
                curFrame->m_lowres.quarterSampleLowResHeight - (NUMBER_OF_SEGMENTS_IN_HEIGHT * segmentHeight) : 0;

            // Y Histogram
            calculateHistogram(
                curFrame->m_lowres.quarterSampleLowResBuffer + (curFrame->m_lowres.quarterSampleLowResOriginX + segmentInFrameWidthIndex * segmentWidth) + ((curFrame->m_lowres.quarterSampleLowResOriginY + segmentInFrameHeightIndex * segmentHeight) * curFrame->m_lowres.quarterSampleLowResStrideY),
                segmentWidth + segmentWidthOffset,
                segmentHeight + segmentHeightOffset,
                curFrame->m_lowres.quarterSampleLowResStrideY,
                1,
                curFrame->m_lowres.picHistogram[segmentInFrameWidthIndex][segmentInFrameHeightIndex][0],
                &sum);

            curFrame->m_lowres.averageIntensityPerSegment[segmentInFrameWidthIndex][segmentInFrameHeightIndex][0] = (uint8_t)((sum + (((segmentWidth + segmentWidthOffset)*(segmentWidth + segmentHeightOffset)) >> 1)) / ((segmentWidth + segmentWidthOffset)*(segmentHeight + segmentHeightOffset)));
            (*sumAvgIntensityTotalSegmentsLuma) += (sum << 4);
            for (uint32_t histogramBin = 0; histogramBin < HISTOGRAM_NUMBER_OF_BINS; histogramBin++)
            {
                curFrame->m_lowres.picHistogram[segmentInFrameWidthIndex][segmentInFrameHeightIndex][0][histogramBin] =
                    curFrame->m_lowres.picHistogram[segmentInFrameWidthIndex][segmentInFrameHeightIndex][0][histogramBin] << 4;
            }
        }
    }
}

void LookaheadTLD::collectPictureStatistics(Frame *curFrame)
{

    uint64_t sumAverageIntensityCb = 0;
    uint64_t sumAverageIntensityCr = 0;
    uint64_t sumAverageIntensity = 0;

    // Histogram bins for Luma
    computeIntensityHistogramBinsLuma(
        curFrame,
        &sumAverageIntensity);

    // Histogram bins for Chroma
    computeIntensityHistogramBinsChroma(
        curFrame,
        &sumAverageIntensityCb,
        &sumAverageIntensityCr);

    curFrame->m_lowres.averageIntensity[0] = (uint8_t)((sumAverageIntensity + ((curFrame->m_lowres.widthFullRes * curFrame->m_lowres.heightFullRes) >> 1)) / (curFrame->m_lowres.widthFullRes * curFrame->m_lowres.heightFullRes));
    curFrame->m_lowres.averageIntensity[1] = (uint8_t)((sumAverageIntensityCb + ((curFrame->m_lowres.widthFullRes * curFrame->m_lowres.heightFullRes) >> 3)) / ((curFrame->m_lowres.widthFullRes * curFrame->m_lowres.heightFullRes) >> 2));
    curFrame->m_lowres.averageIntensity[2] = (uint8_t)((sumAverageIntensityCr + ((curFrame->m_lowres.widthFullRes * curFrame->m_lowres.heightFullRes) >> 3)) / ((curFrame->m_lowres.widthFullRes * curFrame->m_lowres.heightFullRes) >> 2));

    computePictureStatistics(curFrame);

    curFrame->m_lowres.bHistScenecutAnalyzed = false;
}

void PreLookaheadGroup::processTasks(int workerThreadID)
{
    if (workerThreadID < 0)
        workerThreadID = m_lookahead.m_pool ? m_lookahead.m_pool->m_numWorkers : 0;
    LookaheadTLD& tld = m_lookahead.m_tld[workerThreadID];

    m_lock.acquire();
    while (m_jobAcquired < m_jobTotal)
    {
        Frame* preFrame = m_preframes[m_jobAcquired++];
        ProfileLookaheadTime(m_lookahead.m_preLookaheadElapsedTime, m_lookahead.m_countPreLookahead);
        ProfileScopeEvent(prelookahead);
        m_lock.release();
        preFrame->m_lowres.init(preFrame->m_fencPic, preFrame->m_poc);
        if (m_lookahead.m_bAdaptiveQuant)
            tld.calcAdaptiveQuantFrame(preFrame, m_lookahead.m_param);

        if (m_lookahead.m_param->bHistBasedSceneCut)
            tld.collectPictureStatistics(preFrame);

        tld.lowresIntraEstimate(preFrame->m_lowres, m_lookahead.m_param->rc.qgSize);
        preFrame->m_lowresInit = true;

        m_lock.acquire();
    }
    m_lock.release();
}


void Lookahead::placeBref(Frame** frames, int start, int end, int num, int *brefs)
{
    int avg = (start + end) / 2;
    if (m_param->bEnableTemporalSubLayers < 2)
    {
        (*frames[avg]).m_lowres.sliceType = X265_TYPE_BREF;
        (*brefs)++;
        return;
    }
    else
    {
        if (num <= 2)
            return;
        else
        {
            (*frames[avg]).m_lowres.sliceType = X265_TYPE_BREF;
            (*brefs)++;
            placeBref(frames, start, avg, avg - start, brefs);
            placeBref(frames, avg + 1, end, end - avg, brefs);
            return;
        }
    }
}


void Lookahead::compCostBref(Lowres **frames, int start, int end, int num)
{
    CostEstimateGroup estGroup(*this, frames);
    int avg = (start + end) / 2;
    if (num <= 2)
    {
        for (int i = start; i < end; i++)
        {
            estGroup.singleCost(start, end + 1, i + 1);
        }
        return;
    }
    else
    {
        estGroup.singleCost(start, end + 1, avg + 1);
        compCostBref(frames, start, avg, avg - start);
        compCostBref(frames, avg + 1, end, end - avg);
        return;
    }
}

/* called by API thread or worker thread with inputQueueLock acquired */
void Lookahead::slicetypeDecide()
{
    PreLookaheadGroup pre(*this);
    Lowres* frames[X265_LOOKAHEAD_MAX + X265_BFRAME_MAX + 4];
    Frame*  list[X265_BFRAME_MAX + 4];
    memset(frames, 0, sizeof(frames));
    memset(list, 0, sizeof(list));
    int maxSearch = X265_MIN(m_param->lookaheadDepth, X265_LOOKAHEAD_MAX);
    maxSearch = X265_MAX(1, maxSearch);

    {
        ScopedLock lock(m_inputLock);

        Frame *curFrame = m_inputQueue.first();
        int j;
		if (m_param->bResetZoneConfig)
		{
			for (int i = 0; i < m_param->rc.zonefileCount; i++)
			{
				if (m_param->rc.zones[i].startFrame == curFrame->m_poc)
					m_param = m_param->rc.zones[i].zoneParam;
			}
		}
        for (j = 0; j < m_param->bframes + 2; j++)
        {
            if (!curFrame) break;
            list[j] = curFrame;
            curFrame = curFrame->m_next;
        }

        curFrame = m_inputQueue.first();
        frames[0] = m_lastNonB;
        for (j = 0; j < maxSearch; j++)
        {
            if (!curFrame) break;
            frames[j + 1] = &curFrame->m_lowres;

            if (!curFrame->m_lowresInit)
                pre.m_preframes[pre.m_jobTotal++] = curFrame;

            curFrame = curFrame->m_next;
        }

        maxSearch = j;
    }

    /* perform pre-analysis on frames which need it, using a bonded task group */
    if (pre.m_jobTotal)
    {
        if (m_pool)
            pre.tryBondPeers(*m_pool, pre.m_jobTotal);
        pre.processTasks(-1);
        pre.waitForExit();
    }

    if(m_param->bEnableFades)
    {
        int j, endIndex = 0, length = X265_BFRAME_MAX + 4;
        for (j = 0; j < length; j++)
            m_frameVariance[j] = -1;
        for (j = 0; list[j] != NULL; j++)
            m_frameVariance[list[j]->m_poc % length] = list[j]->m_lowres.frameVariance;
        for (int k = list[0]->m_poc % length; k <= list[j - 1]->m_poc % length; k++)
        {
            if (m_frameVariance[k]  == -1)
                break;
            if((k > 0 && m_frameVariance[k] >= m_frameVariance[k - 1]) || 
                (k == 0 && m_frameVariance[k] >= m_frameVariance[length - 1]))
            {
                m_isFadeIn = true;
                if (m_fadeCount == 0 && m_fadeStart == -1)
                {
                    for(int temp = list[0]->m_poc; temp <= list[j - 1]->m_poc; temp++)
                        if (k == temp % length) {
                            m_fadeStart = temp ? temp - 1 : 0;
                            break;
                        }
                }
                m_fadeCount = list[endIndex]->m_poc > m_fadeStart ? list[endIndex]->m_poc - m_fadeStart : 0;
                endIndex++;
            }
            else
            {
                if (m_isFadeIn && m_fadeCount >= m_param->fpsNum / m_param->fpsDenom)
                {
                    for (int temp = 0; list[temp] != NULL; temp++)
                    {
                        if (list[temp]->m_poc == m_fadeStart + (int)m_fadeCount)
                        {
                            list[temp]->m_lowres.bIsFadeEnd = true;
                            break;
                        }
                    }
                }
                m_isFadeIn = false;
                m_fadeCount = 0;
                m_fadeStart = -1;
            }
            if (k == length - 1)
                k = -1;
        }
    }

    if (m_lastNonB &&
        ((m_param->bFrameAdaptive && m_param->bframes) ||
         m_param->rc.cuTree || m_param->scenecutThreshold || m_param->bHistBasedSceneCut ||
         (m_param->lookaheadDepth && m_param->rc.vbvBufferSize)))
    {
        if (!m_param->rc.bStatRead)
            slicetypeAnalyse(frames, false);
        bool bIsVbv = m_param->rc.vbvBufferSize > 0 && m_param->rc.vbvMaxBitrate > 0;
        if ((m_param->analysisLoad && m_param->scaleFactor && bIsVbv) || m_param->bliveVBV2pass)
        {
            int numFrames;
            for (numFrames = 0; numFrames < maxSearch; numFrames++)
            {
                Lowres *fenc = frames[numFrames + 1];
                if (!fenc)
                    break;
            }
            vbvLookahead(frames, numFrames, false);
        }
    }

    int bframes, brefs;
    if (!m_param->analysisLoad || m_param->bAnalysisType == HEVC_INFO)
    {
        bool isClosedGopRadl = m_param->radl && (m_param->keyframeMax != m_param->keyframeMin);
        for (bframes = 0, brefs = 0;; bframes++)
        {
            Lowres& frm = list[bframes]->m_lowres;

            if (frm.sliceType == X265_TYPE_BREF && !m_param->bBPyramid && brefs == m_param->bBPyramid)
            {
                frm.sliceType = X265_TYPE_B;
                x265_log(m_param, X265_LOG_WARNING, "B-ref at frame %d incompatible with B-pyramid\n",
                    frm.frameNum);
            }

            /* pyramid with multiple B-refs needs a big enough dpb that the preceding P-frame stays available.
             * smaller dpb could be supported by smart enough use of mmco, but it's easier just to forbid it. */
            else if (frm.sliceType == X265_TYPE_BREF && m_param->bBPyramid && brefs &&
                m_param->maxNumReferences <= (brefs + 3))
            {
                frm.sliceType = X265_TYPE_B;
                x265_log(m_param, X265_LOG_WARNING, "B-ref at frame %d incompatible with B-pyramid and %d reference frames\n",
                    frm.sliceType, m_param->maxNumReferences);
            }
            if (((!m_param->bIntraRefresh || frm.frameNum == 0) && frm.frameNum - m_lastKeyframe >= m_param->keyframeMax &&
                (!m_extendGopBoundary || frm.frameNum - m_lastKeyframe >= m_param->keyframeMax + m_param->gopLookahead)) ||
                (frm.frameNum == (m_param->chunkStart - 1)) || (frm.frameNum == m_param->chunkEnd))
            {
                if (frm.sliceType == X265_TYPE_AUTO || frm.sliceType == X265_TYPE_I)
                    frm.sliceType = m_param->bOpenGOP && m_lastKeyframe >= 0 ? X265_TYPE_I : X265_TYPE_IDR;
                bool warn = frm.sliceType != X265_TYPE_IDR;
                if (warn && m_param->bOpenGOP)
                    warn &= frm.sliceType != X265_TYPE_I;
                if (warn)
                {
                    x265_log(m_param, X265_LOG_WARNING, "specified frame type (%d) at %d is not compatible with keyframe interval\n",
                        frm.sliceType, frm.frameNum);
                    frm.sliceType = m_param->bOpenGOP && m_lastKeyframe >= 0 ? X265_TYPE_I : X265_TYPE_IDR;
                }
            }
            if (frm.bIsFadeEnd){
                frm.sliceType = m_param->bOpenGOP && m_lastKeyframe >= 0 ? X265_TYPE_I : X265_TYPE_IDR;
            }
            if (m_param->bResetZoneConfig)
            {
                for (int i = 0; i < m_param->rc.zonefileCount; i++)
                {
                    int curZoneStart = m_param->rc.zones[i].startFrame;
                    curZoneStart += curZoneStart ? m_param->rc.zones[i].zoneParam->radl : 0;
                    if (curZoneStart == frm.frameNum)
                        frm.sliceType = X265_TYPE_IDR;
                }
            }
            if ((frm.sliceType == X265_TYPE_I && frm.frameNum - m_lastKeyframe >= m_param->keyframeMin) || (frm.frameNum == (m_param->chunkStart - 1)) || (frm.frameNum == m_param->chunkEnd))
            {
                if (m_param->bOpenGOP)
                {
                    m_lastKeyframe = frm.frameNum;
                    frm.bKeyframe = true;
                }
                else
                    frm.sliceType = X265_TYPE_IDR;
            }
            if (frm.sliceType == X265_TYPE_IDR && frm.bScenecut && isClosedGopRadl)
            {
                for (int i = bframes; i < bframes + m_param->radl; i++)
                    list[i]->m_lowres.sliceType = X265_TYPE_B;
                list[(bframes + m_param->radl)]->m_lowres.sliceType = X265_TYPE_IDR;
            }
            if (frm.sliceType == X265_TYPE_IDR)
            {
                /* Closed GOP */
                m_lastKeyframe = frm.frameNum;
                frm.bKeyframe = true;
                int zoneRadl = 0;
                if (m_param->bResetZoneConfig)
                {
                    for (int i = 0; i < m_param->rc.zonefileCount; i++)
                    {
                        int zoneStart = m_param->rc.zones[i].startFrame;
                        zoneStart += zoneStart ? m_param->rc.zones[i].zoneParam->radl : 0;
                        if (zoneStart == frm.frameNum)
                        {
                            zoneRadl = m_param->rc.zones[i].zoneParam->radl;
                            m_param->radl = 0;
                            m_param->rc.zones->zoneParam->radl = i < m_param->rc.zonefileCount - 1 ? m_param->rc.zones[i + 1].zoneParam->radl : 0;
                            break;
                        }
                    }
                }
                if (bframes > 0 && !m_param->radl && !zoneRadl)
                {
                    list[bframes - 1]->m_lowres.sliceType = X265_TYPE_P;
                    bframes--;
                }
            }
            if (bframes == m_param->bframes || !list[bframes + 1])
            {
                if (IS_X265_TYPE_B(frm.sliceType))
                    x265_log(m_param, X265_LOG_WARNING, "specified frame type is not compatible with max B-frames\n");
                if (frm.sliceType == X265_TYPE_AUTO || IS_X265_TYPE_B(frm.sliceType))
                    frm.sliceType = X265_TYPE_P;
            }
            if (frm.sliceType == X265_TYPE_BREF)
                brefs++;
            if (frm.sliceType == X265_TYPE_AUTO)
                frm.sliceType = X265_TYPE_B;
            else if (!IS_X265_TYPE_B(frm.sliceType))
                break;
        }
    }
    else
    {
        for (bframes = 0, brefs = 0;; bframes++)
        {
            Lowres& frm = list[bframes]->m_lowres;
            if (frm.sliceType == X265_TYPE_BREF)
                brefs++;
            if ((IS_X265_TYPE_I(frm.sliceType) && frm.frameNum - m_lastKeyframe >= m_param->keyframeMin)
                || (frm.frameNum == (m_param->chunkStart - 1)) || (frm.frameNum == m_param->chunkEnd))
            {
                m_lastKeyframe = frm.frameNum;
                frm.bKeyframe = true;
            }
            if (!IS_X265_TYPE_B(frm.sliceType))
                break;
        }
    }

    if (m_param->bEnableTemporalSubLayers > 2)
    {
        //Split the partial mini GOP into sub mini GOPs when temporal sub layers are enabled
        if (bframes < m_param->bframes)
        {
            int leftOver = bframes + 1;
            int8_t gopId = m_gopId - 1;
            int gopLen = x265_gop_ra_length[gopId];
            int listReset = 0;

            m_outputLock.acquire();

            while ((gopId >= 0) && (leftOver > 3))
            {
                if (leftOver < gopLen)
                {
                    gopId = gopId - 1;
                    gopLen = x265_gop_ra_length[gopId];
                    continue;
                }
                else
                {
                    int newbFrames = listReset + gopLen - 1;
                    //Re-assign GOP
                    list[newbFrames]->m_lowres.sliceType = IS_X265_TYPE_I(list[newbFrames]->m_lowres.sliceType) ? list[newbFrames]->m_lowres.sliceType : X265_TYPE_P;
                    if (newbFrames)
                        list[newbFrames - 1]->m_lowres.bLastMiniGopBFrame = true;
                    list[newbFrames]->m_lowres.leadingBframes = newbFrames;
                    m_lastNonB = &list[newbFrames]->m_lowres;

                    /* insert a bref into the sequence */
                    if (m_param->bBPyramid && newbFrames)
                    {
                        placeBref(list, listReset, newbFrames, newbFrames + 1, &brefs);
                    }
                    if (m_param->rc.rateControlMode != X265_RC_CQP)
                    {
                        int p0, p1, b;
                        /* For zero latency tuning, calculate frame cost to be used later in RC */
                        if (!maxSearch)
                        {
                            for (int i = listReset; i <= newbFrames; i++)
                                frames[i + 1] = &list[listReset + i]->m_lowres;
                        }

                        /* estimate new non-B cost */
                        p1 = b = newbFrames + 1;
                        p0 = (IS_X265_TYPE_I(frames[newbFrames + 1]->sliceType)) ? b : listReset;

                        CostEstimateGroup estGroup(*this, frames);

                        estGroup.singleCost(p0, p1, b);

                        if (newbFrames)
                            compCostBref(frames, listReset, newbFrames, newbFrames + 1);
                    }

                    m_inputLock.acquire();
                    /* dequeue all frames from inputQueue that are about to be enqueued
                     * in the output queue. The order is important because Frame can
                     * only be in one list at a time */
                    int64_t pts[X265_BFRAME_MAX + 1];
                    for (int i = 0; i < gopLen; i++)
                    {
                        Frame *curFrame;
                        curFrame = m_inputQueue.popFront();
                        pts[i] = curFrame->m_pts;
                        maxSearch--;
                    }
                    m_inputLock.release();

                    int idx = 0;
                    /* add non-B to output queue */
                    list[newbFrames]->m_reorderedPts = pts[idx++];
                    list[newbFrames]->m_gopOffset = 0;
                    list[newbFrames]->m_gopId = gopId;
                    list[newbFrames]->m_tempLayer = x265_gop_ra[gopId][0].layer;
                    m_outputQueue.pushBack(*list[newbFrames]);

                    /* add B frames to output queue */
                    int i = 1, j = 1;
                    while (i < gopLen)
                    {
                        int offset = listReset + (x265_gop_ra[gopId][j].poc_offset - 1);
                        if (!list[offset] || offset == newbFrames)
                            continue;

                        // Assign gop offset and temporal layer of frames
                        list[offset]->m_gopOffset = j;
                        list[bframes]->m_gopId = gopId;
                        list[offset]->m_tempLayer = x265_gop_ra[gopId][j++].layer;

                        list[offset]->m_reorderedPts = pts[idx++];
                        m_outputQueue.pushBack(*list[offset]);
                        i++;
                    }

                    listReset += gopLen;
                    leftOver = leftOver - gopLen;
                    gopId -= 1;
                    gopLen = (gopId >= 0) ? x265_gop_ra_length[gopId] : 0;
                }
            }

            if (leftOver > 0 && leftOver < 4)
            {
                int64_t pts[X265_BFRAME_MAX + 1];
                int idx = 0;

                int newbFrames = listReset + leftOver - 1;
                list[newbFrames]->m_lowres.sliceType = IS_X265_TYPE_I(list[newbFrames]->m_lowres.sliceType) ? list[newbFrames]->m_lowres.sliceType : X265_TYPE_P;
                if (newbFrames)
                        list[newbFrames - 1]->m_lowres.bLastMiniGopBFrame = true;
                list[newbFrames]->m_lowres.leadingBframes = newbFrames;
                m_lastNonB = &list[newbFrames]->m_lowres;

                /* insert a bref into the sequence */
                if (m_param->bBPyramid && (newbFrames- listReset) > 1)
                    placeBref(list, listReset, newbFrames, newbFrames + 1, &brefs);

                if (m_param->rc.rateControlMode != X265_RC_CQP)
                {
                    int p0, p1, b;
                    /* For zero latency tuning, calculate frame cost to be used later in RC */
                    if (!maxSearch)
                    {
                        for (int i = listReset; i <= newbFrames; i++)
                            frames[i + 1] = &list[listReset + i]->m_lowres;
                    }

                        /* estimate new non-B cost */
                    p1 = b = newbFrames + 1;
                    p0 = (IS_X265_TYPE_I(frames[newbFrames + 1]->sliceType)) ? b : listReset;

                    CostEstimateGroup estGroup(*this, frames);

                    estGroup.singleCost(p0, p1, b);

                    if (newbFrames)
                        compCostBref(frames, listReset, newbFrames, newbFrames + 1);
                }

                m_inputLock.acquire();
                /* dequeue all frames from inputQueue that are about to be enqueued
                 * in the output queue. The order is important because Frame can
                 * only be in one list at a time */
                for (int i = 0; i < leftOver; i++)
                {
                    Frame *curFrame;
                    curFrame = m_inputQueue.popFront();
                    pts[i] = curFrame->m_pts;
                    maxSearch--;
                }
                m_inputLock.release();

                m_lastNonB = &list[newbFrames]->m_lowres;
                list[newbFrames]->m_reorderedPts = pts[idx++];
                list[newbFrames]->m_gopOffset = 0;
                list[newbFrames]->m_gopId = -1;
                list[newbFrames]->m_tempLayer = 0;
                m_outputQueue.pushBack(*list[newbFrames]);
                if (brefs)
                {
                    for (int i = listReset; i < newbFrames; i++)
                    {
                        if (list[i]->m_lowres.sliceType == X265_TYPE_BREF)
                        {
                            list[i]->m_reorderedPts = pts[idx++];
                            list[i]->m_gopOffset = 0;
                            list[i]->m_gopId = -1;
                            list[i]->m_tempLayer = 0;
                            m_outputQueue.pushBack(*list[i]);
                        }
                    }
                }

                /* add B frames to output queue */
                for (int i = listReset; i < newbFrames; i++)
                {
                    /* push all the B frames into output queue except B-ref, which already pushed into output queue */
                    if (list[i]->m_lowres.sliceType != X265_TYPE_BREF)
                    {
                        list[i]->m_reorderedPts = pts[idx++];
                        list[i]->m_gopOffset = 0;
                        list[i]->m_gopId = -1;
                        list[i]->m_tempLayer = 1;
                        m_outputQueue.pushBack(*list[i]);
                    }
                }
            }
        }
        else
        // Fill the complete mini GOP when temporal sub layers are enabled
        {

            list[bframes - 1]->m_lowres.bLastMiniGopBFrame = true;
            list[bframes]->m_lowres.leadingBframes = bframes;
            m_lastNonB = &list[bframes]->m_lowres;

            /* insert a bref into the sequence */
            if (m_param->bBPyramid && !brefs)
            {
                placeBref(list, 0, bframes, bframes + 1, &brefs);
            }

            /* calculate the frame costs ahead of time for estimateFrameCost while we still have lowres */
            if (m_param->rc.rateControlMode != X265_RC_CQP)
            {
                int p0, p1, b;
                /* For zero latency tuning, calculate frame cost to be used later in RC */
                if (!maxSearch)
                {
                    for (int i = 0; i <= bframes; i++)
                        frames[i + 1] = &list[i]->m_lowres;
                }

                /* estimate new non-B cost */
                p1 = b = bframes + 1;
                p0 = (IS_X265_TYPE_I(frames[bframes + 1]->sliceType)) ? b : 0;

                CostEstimateGroup estGroup(*this, frames);
                estGroup.singleCost(p0, p1, b);

                compCostBref(frames, 0, bframes, bframes + 1);
            }

            m_inputLock.acquire();
            /* dequeue all frames from inputQueue that are about to be enqueued
            * in the output queue. The order is important because Frame can
            * only be in one list at a time */
            int64_t pts[X265_BFRAME_MAX + 1];
            for (int i = 0; i <= bframes; i++)
            {
                Frame *curFrame;
                curFrame = m_inputQueue.popFront();
                pts[i] = curFrame->m_pts;
                maxSearch--;
            }
            m_inputLock.release();

            m_outputLock.acquire();

            int idx = 0;
            /* add non-B to output queue */
            list[bframes]->m_reorderedPts = pts[idx++];
            list[bframes]->m_gopOffset = 0;
            list[bframes]->m_gopId = m_gopId;
            list[bframes]->m_tempLayer = x265_gop_ra[m_gopId][0].layer;
            m_outputQueue.pushBack(*list[bframes]);

            int i = 1, j = 1;
            while (i <= bframes)
            {
                int offset = x265_gop_ra[m_gopId][j].poc_offset - 1;
                if (!list[offset] || offset == bframes)
                    continue;

                // Assign gop offset and temporal layer of frames
                list[offset]->m_gopOffset = j;
                list[offset]->m_gopId = m_gopId;
                list[offset]->m_tempLayer = x265_gop_ra[m_gopId][j++].layer;

                /* add B frames to output queue */
                list[offset]->m_reorderedPts = pts[idx++];
                m_outputQueue.pushBack(*list[offset]);
                i++;
            }
        }

        bool isKeyFrameAnalyse = (m_param->rc.cuTree || (m_param->rc.vbvBufferSize && m_param->lookaheadDepth));
        if (isKeyFrameAnalyse && IS_X265_TYPE_I(m_lastNonB->sliceType))
        {
            m_inputLock.acquire();
            Frame *curFrame = m_inputQueue.first();
            frames[0] = m_lastNonB;
            int j;
            for (j = 0; j < maxSearch; j++)
            {
                frames[j + 1] = &curFrame->m_lowres;
                curFrame = curFrame->m_next;
            }
            m_inputLock.release();

            frames[j + 1] = NULL;
            if (!m_param->rc.bStatRead)
                slicetypeAnalyse(frames, true);
            bool bIsVbv = m_param->rc.vbvBufferSize > 0 && m_param->rc.vbvMaxBitrate > 0;
            if ((m_param->analysisLoad && m_param->scaleFactor && bIsVbv) || m_param->bliveVBV2pass)
            {
                int numFrames;
                for (numFrames = 0; numFrames < maxSearch; numFrames++)
                {
                    Lowres *fenc = frames[numFrames + 1];
                    if (!fenc)
                        break;
                }
                vbvLookahead(frames, numFrames, true);
            }
        }


        m_outputLock.release();
    }
    else
    {

        if (bframes)
            list[bframes - 1]->m_lowres.bLastMiniGopBFrame = true;
        list[bframes]->m_lowres.leadingBframes = bframes;
        m_lastNonB = &list[bframes]->m_lowres;

        /* insert a bref into the sequence */
        if (m_param->bBPyramid && bframes > 1 && !brefs)
        {
            placeBref(list, 0, bframes, bframes + 1, &brefs);
        }
        /* calculate the frame costs ahead of time for estimateFrameCost while we still have lowres */
        if (m_param->rc.rateControlMode != X265_RC_CQP)
        {
            int p0, p1, b;
            /* For zero latency tuning, calculate frame cost to be used later in RC */
            if (!maxSearch)
            {
                for (int i = 0; i <= bframes; i++)
                    frames[i + 1] = &list[i]->m_lowres;
            }

            /* estimate new non-B cost */
            p1 = b = bframes + 1;
            p0 = (IS_X265_TYPE_I(frames[bframes + 1]->sliceType)) ? b : 0;

            CostEstimateGroup estGroup(*this, frames);
            estGroup.singleCost(p0, p1, b);

            if (m_param->bEnableTemporalSubLayers > 1 && bframes)
            {
                compCostBref(frames, 0, bframes, bframes + 1);
            }
            else
            {
                if (bframes)
                {
                    p0 = 0; // last nonb
                    bool isp0available = frames[bframes + 1]->sliceType == X265_TYPE_IDR ? false : true;

                    for (b = 1; b <= bframes; b++)
                    {
                        if (!isp0available)
                            p0 = b;

                        if (frames[b]->sliceType == X265_TYPE_B)
                            for (p1 = b; frames[p1]->sliceType == X265_TYPE_B; p1++)
                                ; // find new nonb or bref
                        else
                            p1 = bframes + 1;

                        estGroup.singleCost(p0, p1, b);

                        if (frames[b]->sliceType == X265_TYPE_BREF)
                        {
                            p0 = b;
                            isp0available = true;
                        }
                    }
                }
            }
        }

        m_inputLock.acquire();
        /* dequeue all frames from inputQueue that are about to be enqueued
         * in the output queue. The order is important because Frame can
         * only be in one list at a time */
        int64_t pts[X265_BFRAME_MAX + 1];
        for (int i = 0; i <= bframes; i++)
        {
            Frame *curFrame;
            curFrame = m_inputQueue.popFront();
            pts[i] = curFrame->m_pts;
            maxSearch--;
        }
        m_inputLock.release();

        m_outputLock.acquire();

        /* add non-B to output queue */
        int idx = 0;
        list[bframes]->m_reorderedPts = pts[idx++];
        m_outputQueue.pushBack(*list[bframes]);

        /* Add B-ref frame next to P frame in output queue, the B-ref encode before non B-ref frame */
        if (brefs)
        {
            for (int i = 0; i < bframes; i++)
            {
                if (list[i]->m_lowres.sliceType == X265_TYPE_BREF)
                {
                    list[i]->m_reorderedPts = pts[idx++];
                    m_outputQueue.pushBack(*list[i]);
                }
            }
        }

        /* add B frames to output queue */
        for (int i = 0; i < bframes; i++)
        {
            /* push all the B frames into output queue except B-ref, which already pushed into output queue */
            if (list[i]->m_lowres.sliceType != X265_TYPE_BREF)
            {
                list[i]->m_reorderedPts = pts[idx++];
                m_outputQueue.pushBack(*list[i]);
            }
        }


        bool isKeyFrameAnalyse = (m_param->rc.cuTree || (m_param->rc.vbvBufferSize && m_param->lookaheadDepth));
        if (isKeyFrameAnalyse && IS_X265_TYPE_I(m_lastNonB->sliceType))
        {
            m_inputLock.acquire();
            Frame *curFrame = m_inputQueue.first();
            frames[0] = m_lastNonB;
            int j;
            for (j = 0; j < maxSearch; j++)
            {
                frames[j + 1] = &curFrame->m_lowres;
                curFrame = curFrame->m_next;
            }
            m_inputLock.release();

            frames[j + 1] = NULL;
            if (!m_param->rc.bStatRead)
                slicetypeAnalyse(frames, true);
            bool bIsVbv = m_param->rc.vbvBufferSize > 0 && m_param->rc.vbvMaxBitrate > 0;
            if ((m_param->analysisLoad && m_param->scaleFactor && bIsVbv) || m_param->bliveVBV2pass)
            {
                int numFrames;
                for (numFrames = 0; numFrames < maxSearch; numFrames++)
                {
                    Lowres *fenc = frames[numFrames + 1];
                    if (!fenc)
                        break;
                }
                vbvLookahead(frames, numFrames, true);
            }
        }

        m_outputLock.release();
    }
}

void Lookahead::vbvLookahead(Lowres **frames, int numFrames, int keyframe)
{
    int prevNonB = 0, curNonB = 1, idx = 0;
    while (curNonB < numFrames && IS_X265_TYPE_B(frames[curNonB]->sliceType))
        curNonB++;
    int nextNonB = keyframe ? prevNonB : curNonB;
    int nextB = prevNonB + 1;
    int nextBRef = 0, curBRef = 0;
    if (m_param->bBPyramid && curNonB - prevNonB > 1)
        curBRef = (prevNonB + curNonB + 1) / 2;
    int miniGopEnd = keyframe ? prevNonB : curNonB;
    while (curNonB <= numFrames)
    {
        /* P/I cost: This shouldn't include the cost of nextNonB */
        if (nextNonB != curNonB)
        {
            int p0 = IS_X265_TYPE_I(frames[curNonB]->sliceType) ? curNonB : prevNonB;
            frames[nextNonB]->plannedSatd[idx] = vbvFrameCost(frames, p0, curNonB, curNonB);
            frames[nextNonB]->plannedType[idx] = frames[curNonB]->sliceType;

            /* Save the nextNonB Cost in each B frame of the current miniGop */
            if (curNonB > miniGopEnd)
            {
                for (int j = nextB; j < miniGopEnd; j++)
                {
                    frames[j]->plannedSatd[frames[j]->indB] = frames[nextNonB]->plannedSatd[idx];
                    frames[j]->plannedType[frames[j]->indB++] = frames[nextNonB]->plannedType[idx];
                }
            }
            idx++;
        }

        /* Handle the B-frames: coded order */
        if (m_param->bBPyramid && curNonB - prevNonB > 1)
            nextBRef = (prevNonB + curNonB + 1) / 2;

        for (int i = prevNonB + 1; i < curNonB; i++, idx++)
        {
            int64_t satdCost = 0;
            int type = X265_TYPE_B;
            if (nextBRef)
            {
                if (i == nextBRef)
                {
                    satdCost = vbvFrameCost(frames, prevNonB, curNonB, nextBRef);
                    type = X265_TYPE_BREF;
                }
                else if (i < nextBRef)
                    satdCost = vbvFrameCost(frames, prevNonB, nextBRef, i);
                else
                    satdCost = vbvFrameCost(frames, nextBRef, curNonB, i);
            }
            else
                satdCost = vbvFrameCost(frames, prevNonB, curNonB, i);
            frames[nextNonB]->plannedSatd[idx] = satdCost;
            frames[nextNonB]->plannedType[idx] = type;
            /* Save the nextB Cost in each B frame of the current miniGop */

            for (int j = nextB; j < miniGopEnd; j++)
            {
                if (curBRef && curBRef == i)
                    break;
                if (j >= i && j !=nextBRef)
                    continue;
                frames[j]->plannedSatd[frames[j]->indB] = satdCost;
                frames[j]->plannedType[frames[j]->indB++] = type;
            }
        }
        prevNonB = curNonB;
        curNonB++;
        while (curNonB <= numFrames && IS_X265_TYPE_B(frames[curNonB]->sliceType))
            curNonB++;
    }

    frames[nextNonB]->plannedType[idx] = X265_TYPE_AUTO;
}

int64_t Lookahead::vbvFrameCost(Lowres **frames, int p0, int p1, int b)
{
    CostEstimateGroup estGroup(*this, frames);
    int64_t cost = estGroup.singleCost(p0, p1, b);

    if (m_param->rc.aqMode || m_param->bAQMotion)
    {
        if (m_param->rc.cuTree)
            return frameCostRecalculate(frames, p0, p1, b);
        else
            return frames[b]->costEstAq[b - p0][p1 - b];
    }

    return cost;
}

void Lookahead::slicetypeAnalyse(Lowres **frames, bool bKeyframe)
{
    int numFrames, origNumFrames, keyintLimit, framecnt;
    int maxSearch = X265_MIN(m_param->lookaheadDepth, X265_LOOKAHEAD_MAX);
    int cuCount = m_8x8Blocks;
    int resetStart;
    bool bIsVbvLookahead = m_param->rc.vbvBufferSize && m_param->lookaheadDepth;

    /* count undecided frames */
    for (framecnt = 0; framecnt < maxSearch; framecnt++)
    {
        Lowres *fenc = frames[framecnt + 1];
        if (!fenc || fenc->sliceType != X265_TYPE_AUTO)
            break;
    }

    if (!framecnt)
    {
        if (m_param->rc.cuTree)
            cuTree(frames, 0, bKeyframe);
        return;
    }
    frames[framecnt + 1] = NULL;

    if (m_param->bResetZoneConfig)
    {
        for (int i = 0; i < m_param->rc.zonefileCount; i++)
        {
            int curZoneStart = m_param->rc.zones[i].startFrame, nextZoneStart = 0;
            curZoneStart += curZoneStart ? m_param->rc.zones[i].zoneParam->radl : 0;
            nextZoneStart += (i + 1 < m_param->rc.zonefileCount) ? m_param->rc.zones[i + 1].startFrame + m_param->rc.zones[i + 1].zoneParam->radl : m_param->totalFrames;
            if (curZoneStart <= frames[0]->frameNum && nextZoneStart > frames[0]->frameNum)
                m_param->keyframeMax = nextZoneStart - curZoneStart;
            if (m_param->rc.zones[m_param->rc.zonefileCount - 1].startFrame <= frames[0]->frameNum && nextZoneStart == 0)
                m_param->keyframeMax = m_param->rc.zones[0].keyframeMax;
        }
    }
    int keylimit = m_param->keyframeMax;
    if (frames[0]->frameNum < m_param->chunkEnd)
    {
        int chunkStart = (m_param->chunkStart - m_lastKeyframe - 1);
        int chunkEnd = (m_param->chunkEnd - m_lastKeyframe);
        if ((chunkStart > 0) && (chunkStart < m_param->keyframeMax))
            keylimit = chunkStart;
        else if ((chunkEnd > 0) && (chunkEnd < m_param->keyframeMax))
            keylimit = chunkEnd;
    }

    int keyFrameLimit = keylimit + m_lastKeyframe - frames[0]->frameNum - 1;
    if (m_param->gopLookahead && keyFrameLimit <= m_param->bframes + 1)
        keyintLimit = keyFrameLimit + m_param->gopLookahead;
    else
        keyintLimit = keyFrameLimit;

    origNumFrames = numFrames = m_param->bIntraRefresh ? framecnt : X265_MIN(framecnt, keyintLimit);
    if (bIsVbvLookahead)
        numFrames = framecnt;
    else if (m_param->bOpenGOP && numFrames < framecnt)
        numFrames++;
    else if (numFrames == 0)
    {
        frames[1]->sliceType = X265_TYPE_I;
        return;
    }

    if (m_bBatchMotionSearch)
    {
        /* pre-calculate all motion searches, using many worker threads */
        CostEstimateGroup estGroup(*this, frames);
        for (int b = 2; b < numFrames; b++)
        {
            for (int i = 1; i <= m_param->bframes + 1; i++)
            {
                int p0 = b - i;
                if (p0 < 0)
                    continue;

                /* Skip search if already done */
                if (frames[b]->lowresMvs[0][i][0].x != 0x7FFF)
                    continue;

                /* perform search to p1 at same distance, if possible */
                int p1 = b + i;
                if (p1 >= numFrames || frames[b]->lowresMvs[1][i][0].x != 0x7FFF)
                    p1 = b;

                estGroup.add(p0, p1, b);
            }
        }
        /* auto-disable after the first batch if pool is small */
        m_bBatchMotionSearch &= m_pool->m_numWorkers >= 4;
        estGroup.finishBatch();

        if (m_bBatchFrameCosts)
        {
            /* pre-calculate all frame cost estimates, using many worker threads */
            for (int b = 2; b < numFrames; b++)
            {
                for (int i = 1; i <= m_param->bframes + 1; i++)
                {
                    if (b < i)
                        continue;

                    /* only measure frame cost in this pass if motion searches
                     * are already done */
                    if (frames[b]->lowresMvs[0][i][0].x == 0x7FFF)
                        continue;

                    int p0 = b - i;

                    for (int j = 0; j <= m_param->bframes; j++)
                    {
                        int p1 = b + j;
                        if (p1 >= numFrames)
                            break;

                        /* ensure P1 search is done */
                        if (j && frames[b]->lowresMvs[1][j][0].x == 0x7FFF)
                            continue;

                        /* ensure frame cost is not done */
                        if (frames[b]->costEst[i][j] >= 0)
                            continue;

                        estGroup.add(p0, p1, b);
                    }
                }
            }

            /* auto-disable after the first batch if the pool is not large */
            m_bBatchFrameCosts &= m_pool->m_numWorkers > 12;
            estGroup.finishBatch();
        }
    }

    int numBFrames = 0;
    int numAnalyzed = numFrames;
    bool isScenecut = false;

    if (m_param->bHistBasedSceneCut)
        isScenecut = histBasedScenecut(frames, 0, 1, origNumFrames);
    else
        isScenecut = scenecut(frames, 0, 1, true, origNumFrames);

    /* When scenecut threshold is set, use scenecut detection for I frame placements */
    if (m_param->scenecutThreshold && isScenecut)
    {
        frames[1]->sliceType = X265_TYPE_I;
        return;
    }
    if (m_param->gopLookahead && (keyFrameLimit >= 0) && (keyFrameLimit <= m_param->bframes + 1))
    {
        bool sceneTransition = m_isSceneTransition;
        m_extendGopBoundary = false;
        for (int i = m_param->bframes + 1; i < origNumFrames; i += m_param->bframes + 1)
        {
            scenecut(frames, i, i + 1, true, origNumFrames);

            for (int j = i + 1; j <= X265_MIN(i + m_param->bframes + 1, origNumFrames); j++)
            {
                if (frames[j]->bScenecut && scenecutInternal(frames, j - 1, j, true))
                {
                    m_extendGopBoundary = true;
                    break;
                }
            }
            if (m_extendGopBoundary)
                break;
        }
        m_isSceneTransition = sceneTransition;
    }
    if (m_param->bframes)
    {
        if (m_param->bFrameAdaptive == X265_B_ADAPT_TRELLIS)
        {
            if (numFrames > 1)
            {
                char best_paths[X265_BFRAME_MAX + 1][X265_LOOKAHEAD_MAX + 1] = { "", "P" };
                int best_path_index = numFrames % (X265_BFRAME_MAX + 1);

                /* Perform the frame type analysis. */
                for (int j = 2; j <= numFrames; j++)
                    slicetypePath(frames, j, best_paths);

                numBFrames = (int)strspn(best_paths[best_path_index], "B");

                /* Load the results of the analysis into the frame types. */
                for (int j = 1; j < numFrames; j++)
                    frames[j]->sliceType = best_paths[best_path_index][j - 1] == 'B' ? X265_TYPE_B : X265_TYPE_P;
            }
            frames[numFrames]->sliceType = X265_TYPE_P;
        }
        else if (m_param->bFrameAdaptive == X265_B_ADAPT_FAST)
        {
            CostEstimateGroup estGroup(*this, frames);

            int64_t cost1p0, cost2p0, cost1b1, cost2p1;

            for (int i = 0; i <= numFrames - 2; )
            {
                cost2p1 = estGroup.singleCost(i + 0, i + 2, i + 2, true);
                if (frames[i + 2]->intraMbs[2] > cuCount / 2)
                {
                    frames[i + 1]->sliceType = X265_TYPE_P;
                    frames[i + 2]->sliceType = X265_TYPE_P;
                    i += 2;
                    continue;
                }

                cost1b1 = estGroup.singleCost(i + 0, i + 2, i + 1);
                cost1p0 = estGroup.singleCost(i + 0, i + 1, i + 1);
                cost2p0 = estGroup.singleCost(i + 1, i + 2, i + 2);

                if (cost1p0 + cost2p0 < cost1b1 + cost2p1)
                {
                    frames[i + 1]->sliceType = X265_TYPE_P;
                    i += 1;
                    continue;
                }

// arbitrary and untuned
#define INTER_THRESH 300
#define P_SENS_BIAS (50 - m_param->bFrameBias)
                frames[i + 1]->sliceType = X265_TYPE_B;

                int j;
                for (j = i + 2; j <= X265_MIN(i + m_param->bframes, numFrames - 1); j++)
                {
                    int64_t pthresh = X265_MAX(INTER_THRESH - P_SENS_BIAS * (j - i - 1), INTER_THRESH / 10);
                    int64_t pcost = estGroup.singleCost(i + 0, j + 1, j + 1, true);
                    if (pcost > pthresh * cuCount || frames[j + 1]->intraMbs[j - i + 1] > cuCount / 3)
                        break;
                    frames[j]->sliceType = X265_TYPE_B;
                }

                frames[j]->sliceType = X265_TYPE_P;
                i = j;
            }
            frames[numFrames]->sliceType = X265_TYPE_P;
            numBFrames = 0;
            while (numBFrames < numFrames && frames[numBFrames + 1]->sliceType == X265_TYPE_B)
                numBFrames++;
        }
        else
        {
            numBFrames = X265_MIN(numFrames - 1, m_param->bframes);
            for (int j = 1; j < numFrames; j++)
                frames[j]->sliceType = (j % (numBFrames + 1)) ? X265_TYPE_B : X265_TYPE_P;

            frames[numFrames]->sliceType = X265_TYPE_P;
        }

        int zoneRadl = m_param->rc.zonefileCount && m_param->bResetZoneConfig ? m_param->rc.zones->zoneParam->radl : 0;
        bool bForceRADL = zoneRadl || (m_param->radl && (m_param->keyframeMax == m_param->keyframeMin));
        bool bLastMiniGop = (framecnt >= m_param->bframes + 1) ? false : true;
        int radl = m_param->radl ? m_param->radl : zoneRadl;
        int preRADL = m_lastKeyframe + m_param->keyframeMax - radl - 1; /*Frame preceeding RADL in POC order*/
        if (bForceRADL && (frames[0]->frameNum == preRADL) && !bLastMiniGop)
        {
            int j = 1;
            numBFrames = m_param->radl ? m_param->radl : zoneRadl;
            for (; j <= numBFrames; j++)
                frames[j]->sliceType = X265_TYPE_B;
            frames[j]->sliceType = X265_TYPE_I;
        }
        else /* Check scenecut and RADL on the first minigop. */
        {
            for (int j = 1; j < numBFrames + 1; j++)
            {
                if (scenecut(frames, j, j + 1, false, origNumFrames) ||
                    (bForceRADL && (frames[j]->frameNum == preRADL)))
                {
                    frames[j]->sliceType = X265_TYPE_P;
                    numAnalyzed = j;
                    break;
                }
            }
        }
        resetStart = bKeyframe ? 1 : X265_MIN(numBFrames + 2, numAnalyzed + 1);
    }
    else
    {
        for (int j = 1; j <= numFrames; j++)
            frames[j]->sliceType = X265_TYPE_P;

        resetStart = bKeyframe ? 1 : 2;
    }
    if (m_param->bAQMotion)
        aqMotion(frames, bKeyframe);

    if (m_param->rc.cuTree)
        cuTree(frames, X265_MIN(numFrames, m_param->keyframeMax), bKeyframe);

    if (m_param->gopLookahead && (keyFrameLimit >= 0) && (keyFrameLimit <= m_param->bframes + 1) && !m_extendGopBoundary)
        keyintLimit = keyFrameLimit;

    if (!m_param->bIntraRefresh)
        for (int j = keyintLimit + 1; j <= numFrames; j += m_param->keyframeMax)
        {
            frames[j]->sliceType = X265_TYPE_I;
            resetStart = X265_MIN(resetStart, j + 1);
        }

    if (bIsVbvLookahead)
        vbvLookahead(frames, numFrames, bKeyframe);
    int maxp1 = X265_MIN(m_param->bframes + 1, origNumFrames);

    /* Restore frame types for all frames that haven't actually been decided yet. */
    for (int j = resetStart; j <= numFrames; j++)
    {
        frames[j]->sliceType = X265_TYPE_AUTO;
        /* If any frame marked as scenecut is being restarted for sliceDecision, 
         * undo scene Transition flag */
        if (j <= maxp1 && frames[j]->bScenecut && m_isSceneTransition)
            m_isSceneTransition = false;
    }
}

bool Lookahead::scenecut(Lowres **frames, int p0, int p1, bool bRealScenecut, int numFrames)
{
    /* Only do analysis during a normal scenecut check. */
    if (bRealScenecut && m_param->bframes)
    {
        int origmaxp1 = p0 + 1;
        /* Look ahead to avoid coding short flashes as scenecuts. */
        origmaxp1 += m_param->bframes;
        int maxp1 = X265_MIN(origmaxp1, numFrames);
        bool fluctuate = false;
        bool noScenecuts = false;
        int64_t avgSatdCost = 0;
        if (frames[p0]->costEst[p1 - p0][0] > -1)
            avgSatdCost = frames[p0]->costEst[p1 - p0][0];
        int cnt = 1;
        /* Where A and B are scenes: AAAAAABBBAAAAAA
         * If BBB is shorter than (maxp1-p0), it is detected as a flash
         * and not considered a scenecut. */

        for (int cp1 = p1; cp1 <= maxp1; cp1++)
        {
            if (!scenecutInternal(frames, p0, cp1, false))
            {
                /* Any frame in between p0 and cur_p1 cannot be a real scenecut. */
                for (int i = cp1; i > p0; i--)
                {
                    frames[i]->bScenecut = false;
                    noScenecuts = false;
                }
            }
            else if (scenecutInternal(frames, cp1 - 1, cp1, false))
            {
                /* If current frame is a Scenecut from p0 frame as well as Scenecut from
                 * preceeding frame, mark it as a Scenecut */
                frames[cp1]->bScenecut = true;
                noScenecuts = true;
            }

            /* compute average satdcost of all the frames in the mini-gop to confirm 
             * whether there is any great fluctuation among them to rule out false positives */
            X265_CHECK(frames[cp1]->costEst[cp1 - p0][0]!= -1, "costEst is not done \n");
            avgSatdCost += frames[cp1]->costEst[cp1 - p0][0];
            cnt++;
        }

        /* Identify possible scene fluctuations by comparing the satd cost of the frames.
         * This could denote the beginning or ending of scene transitions.
         * During a scene transition(fade in/fade outs), if fluctuate remains false,
         * then the scene had completed its transition or stabilized */
        if (noScenecuts)
        {
            fluctuate = false;
            avgSatdCost /= cnt;
            for (int i = p1; i <= maxp1; i++)
            {
                int64_t curCost  = frames[i]->costEst[i - p0][0];
                int64_t prevCost = frames[i - 1]->costEst[i - 1 - p0][0];
                if (fabs((double)(curCost - avgSatdCost)) > 0.1 * avgSatdCost || 
                    fabs((double)(curCost - prevCost)) > 0.1 * prevCost)
                {
                    fluctuate = true;
                    if (!m_isSceneTransition && frames[i]->bScenecut)
                    {
                        m_isSceneTransition = true;
                        /* just mark the first scenechange in the scene transition as a scenecut. */
                        for (int j = i + 1; j <= maxp1; j++)
                            frames[j]->bScenecut = false;
                        break;
                    }
                }
                frames[i]->bScenecut = false;
            }
        }
        if (!fluctuate && !noScenecuts)
            m_isSceneTransition = false; /* Signal end of scene transitioning */
    }

    if (m_param->csvLogLevel >= 2)
    {
        int64_t icost = frames[p1]->costEst[0][0];
        int64_t pcost = frames[p1]->costEst[p1 - p0][0];
        frames[p1]->ipCostRatio = (double)icost / pcost;
    }

    /* A frame is always analysed with bRealScenecut = true first, and then bRealScenecut = false,
       the former for I decisions and the latter for P/B decisions. It's possible that the first 
       analysis detected scenecuts which were later nulled due to scene transitioning, in which 
       case do not return a true scenecut for this frame */

    if (!frames[p1]->bScenecut)
        return false;

    return scenecutInternal(frames, p0, p1, bRealScenecut);
}

bool Lookahead::scenecutInternal(Lowres **frames, int p0, int p1, bool bRealScenecut)
{
    Lowres *frame = frames[p1];

    CostEstimateGroup estGroup(*this, frames);
    estGroup.singleCost(p0, p1, p1);
    int64_t icost = frame->costEst[0][0];
    int64_t pcost = frame->costEst[p1 - p0][0];
    int gopSize = (frame->frameNum - m_lastKeyframe) % m_param->keyframeMax;
    float threshMax = (float)(m_param->scenecutThreshold / 100.0);
    /* magic numbers pulled out of thin air */
    float threshMin = (float)(threshMax * 0.25);
    double bias = m_param->scenecutBias;

    if (bRealScenecut)
    {
        if (m_param->keyframeMin == m_param->keyframeMax)
            threshMin = threshMax;
        if (gopSize <= m_param->keyframeMin / 4 || m_param->bIntraRefresh)
            bias = threshMin / 4;
        else if (gopSize <= m_param->keyframeMin)
            bias = threshMin * gopSize / m_param->keyframeMin;
        else
        {
            bias = threshMin
                + (threshMax - threshMin)
                * (gopSize - m_param->keyframeMin)
                / (m_param->keyframeMax - m_param->keyframeMin);
        }
    }
    bool res = pcost >= (1.0 - bias) * icost;
    if (res && bRealScenecut)
    {
        int imb = frame->intraMbs[p1 - p0];
        int pmb = m_8x8Blocks - imb;
        x265_log(m_param, X265_LOG_DEBUG, "scene cut at %d Icost:%d Pcost:%d ratio:%.4f bias:%.4f gop:%d (imb:%d pmb:%d)\n",
                 frame->frameNum, icost, pcost, 1. - (double)pcost / icost, bias, gopSize, imb, pmb);
    }
    return res;
}

bool Lookahead::detectHistBasedSceneChange(Lowres **frames, int p0, int p1, int p2)
{
    bool isAbruptChange;
    bool isSceneChange;

    Lowres  *previousFrame = frames[p0];
    Lowres  *currentFrame = frames[p1];
    Lowres  *futureFrame = frames[p2];

    currentFrame->bHistScenecutAnalyzed = true;

    uint32_t **accHistDiffRunningAvgCb = m_accHistDiffRunningAvgCb;
    uint32_t **accHistDiffRunningAvgCr = m_accHistDiffRunningAvgCr;
    uint32_t **accHistDiffRunningAvg = m_accHistDiffRunningAvg;

    uint8_t absIntDiffFuturePast = 0;
    uint8_t absIntDiffFuturePresent = 0;
    uint8_t absIntDiffPresentPast = 0;

    uint32_t abruptChangeCount = 0;
    uint32_t sceneChangeCount = 0;

    uint32_t segmentWidth = frames[1]->widthFullRes / NUMBER_OF_SEGMENTS_IN_WIDTH;
    uint32_t segmentHeight = frames[1]->heightFullRes / NUMBER_OF_SEGMENTS_IN_HEIGHT;

    for (uint32_t segmentInFrameWidthIndex = 0; segmentInFrameWidthIndex < NUMBER_OF_SEGMENTS_IN_WIDTH; segmentInFrameWidthIndex++)
    {
        for (uint32_t segmentInFrameHeightIndex = 0; segmentInFrameHeightIndex < NUMBER_OF_SEGMENTS_IN_HEIGHT; segmentInFrameHeightIndex++)
        {
            isAbruptChange = false;
            isSceneChange = false;

            // accumulative absolute histogram differences between the past and current frame
            uint32_t accHistDiff = 0;
            uint32_t accHistDiffCb = 0;
            uint32_t accHistDiffCr = 0;

            uint32_t segmentWidthOffset = (segmentInFrameWidthIndex == NUMBER_OF_SEGMENTS_IN_WIDTH - 1) ?
                frames[1]->widthFullRes - (NUMBER_OF_SEGMENTS_IN_WIDTH * segmentWidth) : 0;

            uint32_t segmentHeightOffset = (segmentInFrameHeightIndex == NUMBER_OF_SEGMENTS_IN_HEIGHT - 1) ?
                frames[1]->heightFullRes - (NUMBER_OF_SEGMENTS_IN_HEIGHT * segmentHeight) : 0;

            segmentWidth += segmentWidthOffset;
            segmentHeight += segmentHeightOffset;

            uint32_t segmentThreshHold = (
                ((X265_ABS((int64_t)currentFrame->picAvgVariance - (int64_t)previousFrame->picAvgVariance)) > PICTURE_DIFF_VARIANCE_TH) &&
                (currentFrame->picAvgVariance > PICTURE_VARIANCE_TH || previousFrame->picAvgVariance > PICTURE_VARIANCE_TH)) ?
                HIGH_VAR_SCENE_CHANGE_TH * NUM64x64INPIC(segmentWidth, segmentHeight) : LOW_VAR_SCENE_CHANGE_TH * NUM64x64INPIC(segmentWidth, segmentHeight);

            uint32_t segmentThreshHoldCb = (
                ((X265_ABS((int64_t)currentFrame->picAvgVarianceCb - (int64_t)previousFrame->picAvgVarianceCb)) > PICTURE_DIFF_VARIANCE_CHROMA_TH) &&
                (currentFrame->picAvgVarianceCb > PICTURE_VARIANCE_CHROMA_TH || previousFrame->picAvgVarianceCb > PICTURE_VARIANCE_CHROMA_TH)) ?
                HIGH_VAR_SCENE_CHANGE_CHROMA_TH * NUM64x64INPIC(segmentWidth, segmentHeight) : LOW_VAR_SCENE_CHANGE_CHROMA_TH * NUM64x64INPIC(segmentWidth, segmentHeight);

            uint32_t segmentThreshHoldCr = (
                ((X265_ABS((int64_t)currentFrame->picAvgVarianceCr - (int64_t)previousFrame->picAvgVarianceCr)) > PICTURE_DIFF_VARIANCE_CHROMA_TH) &&
                (currentFrame->picAvgVarianceCr > PICTURE_VARIANCE_CHROMA_TH || previousFrame->picAvgVarianceCr > PICTURE_VARIANCE_CHROMA_TH)) ?
                HIGH_VAR_SCENE_CHANGE_CHROMA_TH * NUM64x64INPIC(segmentWidth, segmentHeight) : LOW_VAR_SCENE_CHANGE_CHROMA_TH * NUM64x64INPIC(segmentWidth, segmentHeight);

            for (uint32_t bin = 0; bin < HISTOGRAM_NUMBER_OF_BINS; ++bin) {
                accHistDiff += X265_ABS((int32_t)currentFrame->picHistogram[segmentInFrameWidthIndex][segmentInFrameHeightIndex][0][bin] - (int32_t)previousFrame->picHistogram[segmentInFrameWidthIndex][segmentInFrameHeightIndex][0][bin]);
                accHistDiffCb += X265_ABS((int32_t)currentFrame->picHistogram[segmentInFrameWidthIndex][segmentInFrameHeightIndex][1][bin] - (int32_t)previousFrame->picHistogram[segmentInFrameWidthIndex][segmentInFrameHeightIndex][1][bin]);
                accHistDiffCr += X265_ABS((int32_t)currentFrame->picHistogram[segmentInFrameWidthIndex][segmentInFrameHeightIndex][2][bin] - (int32_t)previousFrame->picHistogram[segmentInFrameWidthIndex][segmentInFrameHeightIndex][2][bin]);
            }

            if (m_resetRunningAvg) {
                accHistDiffRunningAvg[segmentInFrameWidthIndex][segmentInFrameHeightIndex] = accHistDiff;
                accHistDiffRunningAvgCb[segmentInFrameWidthIndex][segmentInFrameHeightIndex] = accHistDiffCb;
                accHistDiffRunningAvgCr[segmentInFrameWidthIndex][segmentInFrameHeightIndex] = accHistDiffCr;
            }

            // difference between accumulative absolute histogram differences and the running average at the current frame.
            uint32_t accHistDiffError = X265_ABS((int32_t)accHistDiffRunningAvg[segmentInFrameWidthIndex][segmentInFrameHeightIndex] - (int32_t)accHistDiff);
            uint32_t accHistDiffErrorCb = X265_ABS((int32_t)accHistDiffRunningAvgCb[segmentInFrameWidthIndex][segmentInFrameHeightIndex] - (int32_t)accHistDiffCb);
            uint32_t accHistDiffErrorCr = X265_ABS((int32_t)accHistDiffRunningAvgCr[segmentInFrameWidthIndex][segmentInFrameHeightIndex] - (int32_t)accHistDiffCr);

            if ((accHistDiffError > segmentThreshHold     && accHistDiff >= accHistDiffError) ||
                (accHistDiffErrorCb > segmentThreshHoldCb && accHistDiffCb >= accHistDiffErrorCb) ||
                (accHistDiffErrorCr > segmentThreshHoldCr && accHistDiffCr >= accHistDiffErrorCr)) {

                isAbruptChange = true;
            }

            if (isAbruptChange)
            {
                absIntDiffFuturePast = (uint8_t)X265_ABS((int16_t)futureFrame->averageIntensityPerSegment[segmentInFrameWidthIndex][segmentInFrameHeightIndex][0] - (int16_t)previousFrame->averageIntensityPerSegment[segmentInFrameWidthIndex][segmentInFrameHeightIndex][0]);
                absIntDiffFuturePresent = (uint8_t)X265_ABS((int16_t)futureFrame->averageIntensityPerSegment[segmentInFrameWidthIndex][segmentInFrameHeightIndex][0] - (int16_t)currentFrame->averageIntensityPerSegment[segmentInFrameWidthIndex][segmentInFrameHeightIndex][0]);
                absIntDiffPresentPast = (uint8_t)X265_ABS((int16_t)currentFrame->averageIntensityPerSegment[segmentInFrameWidthIndex][segmentInFrameHeightIndex][0] - (int16_t)previousFrame->averageIntensityPerSegment[segmentInFrameWidthIndex][segmentInFrameHeightIndex][0]);

                if (absIntDiffFuturePresent >= FLASH_TH * absIntDiffFuturePast && absIntDiffPresentPast >= FLASH_TH * absIntDiffFuturePast) {
                    x265_log(m_param, X265_LOG_DEBUG, "Flash in frame# %i , %i, %i, %i\n", currentFrame->frameNum, absIntDiffFuturePast, absIntDiffFuturePresent, absIntDiffPresentPast);
                }
                else if (absIntDiffFuturePresent < FADE_TH && absIntDiffPresentPast < FADE_TH) {
                    x265_log(m_param, X265_LOG_DEBUG, "Fade in frame# %i , %i, %i, %i\n", currentFrame->frameNum, absIntDiffFuturePast, absIntDiffFuturePresent, absIntDiffPresentPast);
                }
                else if (X265_ABS(absIntDiffFuturePresent - absIntDiffPresentPast) < INTENSITY_CHANGE_TH && absIntDiffFuturePresent + absIntDiffPresentPast >= absIntDiffFuturePast) {
                    x265_log(m_param, X265_LOG_DEBUG, "Intensity Change in frame# %i , %i, %i, %i\n", currentFrame->frameNum, absIntDiffFuturePast, absIntDiffFuturePresent, absIntDiffPresentPast);
                }
                else {
                    isSceneChange = true;
                    x265_log(m_param, X265_LOG_DEBUG, "Scene change in frame# %i , %i, %i, %i\n", currentFrame->frameNum, absIntDiffFuturePast, absIntDiffFuturePresent, absIntDiffPresentPast);
                }

            }
            else {
                accHistDiffRunningAvg[segmentInFrameWidthIndex][segmentInFrameHeightIndex] = (3 * accHistDiffRunningAvg[segmentInFrameWidthIndex][segmentInFrameHeightIndex] + accHistDiff) / 4;
            }

            abruptChangeCount += isAbruptChange;
            sceneChangeCount += isSceneChange;
        }
    }

    if (abruptChangeCount >= m_segmentCountThreshold) {
        m_resetRunningAvg = true;
    }
    else {
        m_resetRunningAvg = false;
    }

    if ((sceneChangeCount >= m_segmentCountThreshold)) {
        x265_log(m_param, X265_LOG_DEBUG, "Scene Change in Pic Number# %i\n", currentFrame->frameNum);

        return true;
    }
    else {
        return false;
    }

}

bool Lookahead::histBasedScenecut(Lowres **frames, int p0, int p1, int numFrames)
{
    /* Only do analysis during a normal scenecut check. */
    if (m_param->bframes)
    {
        int origmaxp1 = p0 + 1;
        /* Look ahead to avoid coding short flashes as scenecuts. */
        origmaxp1 += m_param->bframes;
        int maxp1 = X265_MIN(origmaxp1, numFrames);

        for (int cp1 = p0; cp1 < maxp1; cp1++)
        {
            if (frames[cp1 + 1]->bHistScenecutAnalyzed == true)
                continue;

            if (frames[cp1 + 2] != NULL && detectHistBasedSceneChange(frames, cp1, cp1 + 1, cp1 + 2))
            {
                /* If current frame is a Scenecut from p0 frame as well as Scenecut from
                 * preceeding frame, mark it as a Scenecut */
                frames[cp1+1]->bScenecut = true;
            }
        }

    }

    return frames[p1]->bScenecut;
}

void Lookahead::slicetypePath(Lowres **frames, int length, char(*best_paths)[X265_LOOKAHEAD_MAX + 1])
{
    char paths[2][X265_LOOKAHEAD_MAX + 1];
    int num_paths = X265_MIN(m_param->bframes + 1, length);
    int64_t best_cost = 1LL << 62;
    int idx = 0;

    /* Iterate over all currently possible paths */
    for (int path = 0; path < num_paths; path++)
    {
        /* Add suffixes to the current path */
        int len = length - (path + 1);
        memcpy(paths[idx], best_paths[len % (X265_BFRAME_MAX + 1)], len);
        memset(paths[idx] + len, 'B', path);
        strcpy(paths[idx] + len + path, "P");

        /* Calculate the actual cost of the current path */
        int64_t cost = slicetypePathCost(frames, paths[idx], best_cost);
        if (cost < best_cost)
        {
            best_cost = cost;
            idx ^= 1;
        }
    }

    /* Store the best path. */
    memcpy(best_paths[length % (X265_BFRAME_MAX + 1)], paths[idx ^ 1], length);
}

// Find slicetype of the frame with poc # in lookahead buffer
int Lookahead::findSliceType(int poc)
{
    int out_slicetype = X265_TYPE_AUTO;
    if (m_filled)
    {
        m_outputLock.acquire();
        Frame* out = m_outputQueue.first();
        while (out != NULL) {
            if (poc == out->m_poc)
            {
                out_slicetype = out->m_lowres.sliceType;
                break;
            }
            out = out->m_next;
        }
        m_outputLock.release();
    }
    return out_slicetype;
}

int64_t Lookahead::slicetypePathCost(Lowres **frames, char *path, int64_t threshold)
{
    int64_t cost = 0;
    int loc = 1;
    int cur_p = 0;

    CostEstimateGroup estGroup(*this, frames);

    path--; /* Since the 1st path element is really the second frame */
    while (path[loc])
    {
        int next_p = loc;
        /* Find the location of the next P-frame. */
        while (path[next_p] != 'P')
            next_p++;

        /* Add the cost of the P-frame found above */
        cost += estGroup.singleCost(cur_p, next_p, next_p);

        /* Early terminate if the cost we have found is larger than the best path cost so far */
        if (cost > threshold)
            break;

        if (m_param->bBPyramid && next_p - cur_p > 2)
        {
            int middle = cur_p + (next_p - cur_p) / 2;
            cost += estGroup.singleCost(cur_p, next_p, middle);

            for (int next_b = loc; next_b < middle && cost < threshold; next_b++)
                cost += estGroup.singleCost(cur_p, middle, next_b);

            for (int next_b = middle + 1; next_b < next_p && cost < threshold; next_b++)
                cost += estGroup.singleCost(middle, next_p, next_b);
        }
        else
        {
            for (int next_b = loc; next_b < next_p && cost < threshold; next_b++)
                cost += estGroup.singleCost(cur_p, next_p, next_b);
        }

        loc = next_p + 1;
        cur_p = next_p;
    }

    return cost;
}

void Lookahead::aqMotion(Lowres **frames, bool bIntra)
{
    if (!bIntra)
    {
        int curnonb = 0, lastnonb = 1;
        int bframes = 0, i = 1;
        while (frames[lastnonb]->sliceType != X265_TYPE_P)
            lastnonb++;
        bframes = lastnonb - 1;
        if (m_param->bBPyramid && bframes > 1)
        {
            int middle = (bframes + 1) / 2;
            for (i = 1; i < lastnonb; i++)
            {
                int p0 = i > middle ? middle : curnonb;
                int p1 = i < middle ? middle : lastnonb;
                if (i != middle)
                    calcMotionAdaptiveQuantFrame(frames, p0, p1, i);
            }
            calcMotionAdaptiveQuantFrame(frames, curnonb, lastnonb, middle);
        }
        else
            for (i = 1; i < lastnonb; i++)
                calcMotionAdaptiveQuantFrame(frames, curnonb, lastnonb, i);
        calcMotionAdaptiveQuantFrame(frames, curnonb, lastnonb, lastnonb);
    }
}

void Lookahead::calcMotionAdaptiveQuantFrame(Lowres **frames, int p0, int p1, int b)
{
    int listDist[2] = { b - p0, p1 - b };
    int32_t strideInCU = m_8x8Width;
    double qp_adj = 0, avg_adj = 0, avg_adj_pow2 = 0, sd;
    for (uint16_t blocky = 0; blocky < m_8x8Height; blocky++)
    {
        int cuIndex = blocky * strideInCU;
        for (uint16_t blockx = 0; blockx < m_8x8Width; blockx++, cuIndex++)
        {
            int32_t lists_used = frames[b]->lowresCosts[b - p0][p1 - b][cuIndex] >> LOWRES_COST_SHIFT;
            double displacement = 0;
            for (uint16_t list = 0; list < 2; list++)
            {
                if ((lists_used >> list) & 1)
                {
                    MV *mvs = frames[b]->lowresMvs[list][listDist[list]];
                    int32_t x = mvs[cuIndex].x;
                    int32_t y = mvs[cuIndex].y;
                    // NOTE: the dynamic range of abs(x) and abs(y) is 15-bits
                    displacement += sqrt((double)(abs(x) * abs(x)) + (double)(abs(y) * abs(y)));
                }
                else
                    displacement += 0.0;
            }
            if (lists_used == 3)
                displacement = displacement / 2;
            qp_adj = pow(displacement, 0.1);
            frames[b]->qpAqMotionOffset[cuIndex] = qp_adj;
            avg_adj += qp_adj;
            avg_adj_pow2 += qp_adj * qp_adj;
        }
    }
    avg_adj /= m_cuCount;
    avg_adj_pow2 /= m_cuCount;
    sd = sqrt((avg_adj_pow2 - (avg_adj * avg_adj)));
    if (sd > 0)
    {
        for (uint16_t blocky = 0; blocky < m_8x8Height; blocky++)
        {
            int cuIndex = blocky * strideInCU;
            for (uint16_t blockx = 0; blockx < m_8x8Width; blockx++, cuIndex++)
            {
                qp_adj = frames[b]->qpAqMotionOffset[cuIndex];
                qp_adj = (qp_adj - avg_adj) / sd;
                if (qp_adj > 1)
                {
                    frames[b]->qpAqOffset[cuIndex] += qp_adj;
                    frames[b]->qpCuTreeOffset[cuIndex] += qp_adj;
                    frames[b]->invQscaleFactor[cuIndex] += x265_exp2fix8(qp_adj);
                }
            }
        }
    }
}

void Lookahead::cuTree(Lowres **frames, int numframes, bool bIntra)
{
    int idx = !bIntra;
    int lastnonb, curnonb = 1;
    int bframes = 0;

    x265_emms();
    double totalDuration = 0.0;
    for (int j = 0; j <= numframes; j++)
        totalDuration += (double)m_param->fpsDenom / m_param->fpsNum;

    double averageDuration = totalDuration / (numframes + 1);

    int i = numframes;

    while (i > 0 && frames[i]->sliceType == X265_TYPE_B)
        i--;

    lastnonb = i;

    /* Lookaheadless MB-tree is not a theoretically distinct case; the same extrapolation could
     * be applied to the end of a lookahead buffer of any size.  However, it's most needed when
     * lookahead=0, so that's what's currently implemented. */
    if (!m_param->lookaheadDepth)
    {
        if (bIntra)
        {
            memset(frames[0]->propagateCost, 0, m_cuCount * sizeof(uint16_t));
            if (m_param->rc.qgSize == 8)
                memcpy(frames[0]->qpCuTreeOffset, frames[0]->qpAqOffset, m_cuCount * 4 * sizeof(double));
            else
                memcpy(frames[0]->qpCuTreeOffset, frames[0]->qpAqOffset, m_cuCount * sizeof(double));
            return;
        }
        std::swap(frames[lastnonb]->propagateCost, frames[0]->propagateCost);
        memset(frames[0]->propagateCost, 0, m_cuCount * sizeof(uint16_t));
    }
    else
    {
        if (lastnonb < idx)
            return;
        memset(frames[lastnonb]->propagateCost, 0, m_cuCount * sizeof(uint16_t));
    }

    CostEstimateGroup estGroup(*this, frames);

    while (i-- > idx)
    {
        curnonb = i;
        while (frames[curnonb]->sliceType == X265_TYPE_B && curnonb > 0)
            curnonb--;

        if (curnonb < idx)
            break;

        estGroup.singleCost(curnonb, lastnonb, lastnonb);

        memset(frames[curnonb]->propagateCost, 0, m_cuCount * sizeof(uint16_t));
        bframes = lastnonb - curnonb - 1;
        if (m_param->bBPyramid && bframes > 1)
        {
            int middle = (bframes + 1) / 2 + curnonb;
            estGroup.singleCost(curnonb, lastnonb, middle);
            memset(frames[middle]->propagateCost, 0, m_cuCount * sizeof(uint16_t));
            while (i > curnonb)
            {
                int p0 = i > middle ? middle : curnonb;
                int p1 = i < middle ? middle : lastnonb;
                if (i != middle)
                {
                    estGroup.singleCost(p0, p1, i);
                    estimateCUPropagate(frames, averageDuration, p0, p1, i, 0);
                }
                i--;
            }

            estimateCUPropagate(frames, averageDuration, curnonb, lastnonb, middle, 1);
        }
        else
        {
            while (i > curnonb)
            {
                estGroup.singleCost(curnonb, lastnonb, i);
                estimateCUPropagate(frames, averageDuration, curnonb, lastnonb, i, 0);
                i--;
            }
        }
        estimateCUPropagate(frames, averageDuration, curnonb, lastnonb, lastnonb, 1);
        lastnonb = curnonb;
    }

    if (!m_param->lookaheadDepth)
    {
        estGroup.singleCost(0, lastnonb, lastnonb);
        estimateCUPropagate(frames, averageDuration, 0, lastnonb, lastnonb, 1);
        std::swap(frames[lastnonb]->propagateCost, frames[0]->propagateCost);
    }

    cuTreeFinish(frames[lastnonb], averageDuration, lastnonb);
    if (m_param->bBPyramid && bframes > 1 && !m_param->rc.vbvBufferSize)
        cuTreeFinish(frames[lastnonb + (bframes + 1) / 2], averageDuration, 0);
}

void Lookahead::estimateCUPropagate(Lowres **frames, double averageDuration, int p0, int p1, int b, int referenced)
{
    uint16_t *refCosts[2] = { frames[p0]->propagateCost, frames[p1]->propagateCost };
    int32_t distScaleFactor = (((b - p0) << 8) + ((p1 - p0) >> 1)) / (p1 - p0);
    int32_t bipredWeight = m_param->bEnableWeightedBiPred ? 64 - (distScaleFactor >> 2) : 32;
    int32_t bipredWeights[2] = { bipredWeight, 64 - bipredWeight };
    int listDist[2] = { b - p0, p1 - b };

    memset(m_scratch, 0, m_8x8Width * sizeof(int));

    uint16_t *propagateCost = frames[b]->propagateCost;

    x265_emms();
    double fpsFactor = CLIP_DURATION((double)m_param->fpsDenom / m_param->fpsNum) / CLIP_DURATION(averageDuration);

    /* For non-referred frames the source costs are always zero, so just memset one row and re-use it. */
    if (!referenced)
        memset(frames[b]->propagateCost, 0, m_8x8Width * sizeof(uint16_t));

    int32_t strideInCU = m_8x8Width;
    for (uint16_t blocky = 0; blocky < m_8x8Height; blocky++)
    {
        int cuIndex = blocky * strideInCU;
        if (m_param->rc.qgSize == 8)
            primitives.propagateCost(m_scratch, propagateCost,
                       frames[b]->intraCost + cuIndex, frames[b]->lowresCosts[b - p0][p1 - b] + cuIndex,
                       frames[b]->invQscaleFactor8x8 + cuIndex, &fpsFactor, m_8x8Width);
        else
            primitives.propagateCost(m_scratch, propagateCost,
                       frames[b]->intraCost + cuIndex, frames[b]->lowresCosts[b - p0][p1 - b] + cuIndex,
                       frames[b]->invQscaleFactor + cuIndex, &fpsFactor, m_8x8Width);

        if (referenced)
            propagateCost += m_8x8Width;

        for (uint16_t blockx = 0; blockx < m_8x8Width; blockx++, cuIndex++)
        {
            int32_t propagate_amount = m_scratch[blockx];
            /* Don't propagate for an intra block. */
            if (propagate_amount > 0)
            {
                /* Access width-2 bitfield. */
                int32_t lists_used = frames[b]->lowresCosts[b - p0][p1 - b][cuIndex] >> LOWRES_COST_SHIFT;
                /* Follow the MVs to the previous frame(s). */
                for (uint16_t list = 0; list < 2; list++)
                {
                    if ((lists_used >> list) & 1)
                    {
#define CLIP_ADD(s, x) (s) = (uint16_t)X265_MIN((s) + (x), (1 << 16) - 1)
                        int32_t listamount = propagate_amount;
                        /* Apply bipred weighting. */
                        if (lists_used == 3)
                            listamount = (listamount * bipredWeights[list] + 32) >> 6;

                        MV *mvs = frames[b]->lowresMvs[list][listDist[list]];

                        /* Early termination for simple case of mv0. */
                        if (!mvs[cuIndex].word)
                        {
                            CLIP_ADD(refCosts[list][cuIndex], listamount);
                            continue;
                        }

                        int32_t x = mvs[cuIndex].x;
                        int32_t y = mvs[cuIndex].y;
                        int32_t cux = (x >> 5) + blockx;
                        int32_t cuy = (y >> 5) + blocky;
                        int32_t idx0 = cux + cuy * strideInCU;
                        int32_t idx1 = idx0 + 1;
                        int32_t idx2 = idx0 + strideInCU;
                        int32_t idx3 = idx0 + strideInCU + 1;
                        x &= 31;
                        y &= 31;
                        int32_t idx0weight = (32 - y) * (32 - x);
                        int32_t idx1weight = (32 - y) * x;
                        int32_t idx2weight = y * (32 - x);
                        int32_t idx3weight = y * x;

                        /* We could just clip the MVs, but pixels that lie outside the frame probably shouldn't
                         * be counted. */
                        if (cux < m_8x8Width - 1 && cuy < m_8x8Height - 1 && cux >= 0 && cuy >= 0)
                        {
                            CLIP_ADD(refCosts[list][idx0], (listamount * idx0weight + 512) >> 10);
                            CLIP_ADD(refCosts[list][idx1], (listamount * idx1weight + 512) >> 10);
                            CLIP_ADD(refCosts[list][idx2], (listamount * idx2weight + 512) >> 10);
                            CLIP_ADD(refCosts[list][idx3], (listamount * idx3weight + 512) >> 10);
                        }
                        else /* Check offsets individually */
                        {
                            if (cux < m_8x8Width && cuy < m_8x8Height && cux >= 0 && cuy >= 0)
                                CLIP_ADD(refCosts[list][idx0], (listamount * idx0weight + 512) >> 10);
                            if (cux + 1 < m_8x8Width && cuy < m_8x8Height && cux + 1 >= 0 && cuy >= 0)
                                CLIP_ADD(refCosts[list][idx1], (listamount * idx1weight + 512) >> 10);
                            if (cux < m_8x8Width && cuy + 1 < m_8x8Height && cux >= 0 && cuy + 1 >= 0)
                                CLIP_ADD(refCosts[list][idx2], (listamount * idx2weight + 512) >> 10);
                            if (cux + 1 < m_8x8Width && cuy + 1 < m_8x8Height && cux + 1 >= 0 && cuy + 1 >= 0)
                                CLIP_ADD(refCosts[list][idx3], (listamount * idx3weight + 512) >> 10);
                        }
                    }
                }
            }
        }
    }

    if (m_param->rc.vbvBufferSize && m_param->lookaheadDepth && referenced)
        cuTreeFinish(frames[b], averageDuration, b == p1 ? b - p0 : 0);
}

void Lookahead::computeCUTreeQpOffset(Lowres *frame, double averageDuration, int ref0Distance)
{
    int fpsFactor = (int)(CLIP_DURATION(averageDuration) / CLIP_DURATION((double)m_param->fpsDenom / m_param->fpsNum) * 256);
    uint32_t loopIncr = (m_param->rc.qgSize == 8) ? 8 : 16;

    double weightdelta = 0.0;
    if (ref0Distance && frame->weightedCostDelta[ref0Distance - 1] > 0)
        weightdelta = (1.0 - frame->weightedCostDelta[ref0Distance - 1]);

    uint32_t widthFullRes = frame->widthFullRes;
    uint32_t heightFullRes = frame->heightFullRes;

    if (m_param->rc.qgSize == 8)
    {
        int minAQDepth = frame->pAQLayer->minAQDepth;

        PicQPAdaptationLayer* pQPLayerMin = &frame->pAQLayer[minAQDepth];
        double* pcCuTree8x8 = pQPLayerMin->dCuTreeOffset8x8;

        for (int cuY = 0; cuY < m_8x8Height; cuY++)
        {
            for (int cuX = 0; cuX < m_8x8Width; cuX++)
            {
                const int cuXY = cuX + cuY * m_8x8Width;
                int intracost = ((frame->intraCost[cuXY] / 4) * frame->invQscaleFactor8x8[cuXY] + 128) >> 8;
                if (intracost)
                {
                    int propagateCost = ((frame->propagateCost[cuXY] / 4)  * fpsFactor + 128) >> 8;
                    double log2_ratio = X265_LOG2(intracost + propagateCost) - X265_LOG2(intracost) + weightdelta;

                    pcCuTree8x8[cuX * 2 + cuY * m_8x8Width * 4] = log2_ratio;
                    pcCuTree8x8[cuX * 2 + cuY * m_8x8Width * 4 + 1] = log2_ratio;
                    pcCuTree8x8[cuX * 2 + cuY * m_8x8Width * 4 + frame->maxBlocksInRowFullRes] = log2_ratio;
                    pcCuTree8x8[cuX * 2 + cuY * m_8x8Width * 4 + frame->maxBlocksInRowFullRes + 1] = log2_ratio;
                }
            }
        }

        for (uint32_t d = 0; d < 4; d++)
        {
            int ctuSizeIdx = 6 - g_log2Size[m_param->maxCUSize];
            int aqDepth = g_log2Size[m_param->maxCUSize] - g_log2Size[m_param->rc.qgSize];
            if (!aqLayerDepth[ctuSizeIdx][aqDepth][d])
                continue;

            PicQPAdaptationLayer* pQPLayer = &frame->pAQLayer[d];
            const uint32_t aqPartWidth = pQPLayer->aqPartWidth;
            const uint32_t aqPartHeight = pQPLayer->aqPartHeight;

            const uint32_t numAQPartInWidth = pQPLayer->numAQPartInWidth;
            const uint32_t numAQPartInHeight = pQPLayer->numAQPartInHeight;

            double* pcQP = pQPLayer->dQpOffset;
            double* pcCuTree = pQPLayer->dCuTreeOffset;

            uint32_t maxCols = frame->maxBlocksInRowFullRes;

            for (uint32_t y = 0; y < numAQPartInHeight; y++)
            {
                for (uint32_t x = 0; x < numAQPartInWidth; x++, pcQP++, pcCuTree++)
                {
                    uint32_t block_x = x * aqPartWidth;
                    uint32_t block_y = y * aqPartHeight;

                    uint32_t blockXY = 0;
                    double log2_ratio = 0;
                    for (uint32_t block_yy = block_y; block_yy < block_y + aqPartHeight && block_yy < heightFullRes; block_yy += loopIncr)
                    {
                        for (uint32_t block_xx = block_x; block_xx < block_x + aqPartWidth && block_xx < widthFullRes; block_xx += loopIncr)
                        {
                            uint32_t idx = ((block_yy / loopIncr) * (maxCols)) + (block_xx / loopIncr);

                            log2_ratio += *(pcCuTree8x8 + idx);
                            
                            blockXY++;
                        }
                    }

                    double qp_offset = (m_cuTreeStrength * log2_ratio) / blockXY;

                    *pcCuTree = *pcQP - qp_offset;
                }
            }
        }
    }
    else
    {
        for (uint32_t d = 0; d < 4; d++)
        {
            int ctuSizeIdx = 6 - g_log2Size[m_param->maxCUSize];
            int aqDepth = g_log2Size[m_param->maxCUSize] - g_log2Size[m_param->rc.qgSize];
            if (!aqLayerDepth[ctuSizeIdx][aqDepth][d])
                continue;

            PicQPAdaptationLayer* pQPLayer = &frame->pAQLayer[d];
            const uint32_t aqPartWidth = pQPLayer->aqPartWidth;
            const uint32_t aqPartHeight = pQPLayer->aqPartHeight;

            const uint32_t numAQPartInWidth = pQPLayer->numAQPartInWidth;
            const uint32_t numAQPartInHeight = pQPLayer->numAQPartInHeight;

            double* pcQP = pQPLayer->dQpOffset;
            double* pcCuTree = pQPLayer->dCuTreeOffset;

            uint32_t maxCols = frame->maxBlocksInRow;

            for (uint32_t y = 0; y < numAQPartInHeight; y++)
            {
                for (uint32_t x = 0; x < numAQPartInWidth; x++, pcQP++, pcCuTree++)
                {
                    uint32_t block_x = x * aqPartWidth;
                    uint32_t block_y = y * aqPartHeight;

                    uint32_t blockXY = 0;
                    double log2_ratio = 0;
                    for (uint32_t block_yy = block_y; block_yy < block_y + aqPartHeight && block_yy < heightFullRes; block_yy += loopIncr)
                    {
                        for (uint32_t block_xx = block_x; block_xx < block_x + aqPartWidth && block_xx < widthFullRes; block_xx += loopIncr)
                        {
                            uint32_t idx = ((block_yy / loopIncr) * (maxCols)) + (block_xx / loopIncr);

                            int intraCost = (frame->intraCost[idx] * frame->invQscaleFactor[idx] + 128) >> 8;
                            int propagateCost = (frame->propagateCost[idx] * fpsFactor + 128) >> 8;

                            log2_ratio += (X265_LOG2(intraCost + propagateCost) - X265_LOG2(intraCost) + weightdelta);

                            blockXY++;
                        }
                    }

                    double qp_offset = (m_cuTreeStrength * log2_ratio) / blockXY;

                    *pcCuTree = *pcQP - qp_offset;

                }
            }
        }
    }
}

void Lookahead::cuTreeFinish(Lowres *frame, double averageDuration, int ref0Distance)
{
    if (m_param->rc.hevcAq)
    {
        computeCUTreeQpOffset(frame, averageDuration, ref0Distance);
    }
    else
    {
        int fpsFactor = (int)(CLIP_DURATION(averageDuration) / CLIP_DURATION((double)m_param->fpsDenom / m_param->fpsNum) * 256);
        double weightdelta = 0.0;

        if (ref0Distance && frame->weightedCostDelta[ref0Distance - 1] > 0)
            weightdelta = (1.0 - frame->weightedCostDelta[ref0Distance - 1]);

        if (m_param->rc.qgSize == 8)
        {
            for (int cuY = 0; cuY < m_8x8Height; cuY++)
            {
                for (int cuX = 0; cuX < m_8x8Width; cuX++)
                {
                    const int cuXY = cuX + cuY * m_8x8Width;
                    int intracost = ((frame->intraCost[cuXY]) / 4 * frame->invQscaleFactor8x8[cuXY] + 128) >> 8;
                    if (intracost)
                    {
                        int propagateCost = ((frame->propagateCost[cuXY]) / 4 * fpsFactor + 128) >> 8;
                        double log2_ratio = X265_LOG2(intracost + propagateCost) - X265_LOG2(intracost) + weightdelta;
                        frame->qpCuTreeOffset[cuX * 2 + cuY * m_8x8Width * 4] = frame->qpAqOffset[cuX * 2 + cuY * m_8x8Width * 4] - m_cuTreeStrength * (log2_ratio);
                        frame->qpCuTreeOffset[cuX * 2 + cuY * m_8x8Width * 4 + 1] = frame->qpAqOffset[cuX * 2 + cuY * m_8x8Width * 4 + 1] - m_cuTreeStrength * (log2_ratio);
                        frame->qpCuTreeOffset[cuX * 2 + cuY * m_8x8Width * 4 + frame->maxBlocksInRowFullRes] = frame->qpAqOffset[cuX * 2 + cuY * m_8x8Width * 4 + frame->maxBlocksInRowFullRes] - m_cuTreeStrength * (log2_ratio);
                        frame->qpCuTreeOffset[cuX * 2 + cuY * m_8x8Width * 4 + frame->maxBlocksInRowFullRes + 1] = frame->qpAqOffset[cuX * 2 + cuY * m_8x8Width * 4 + frame->maxBlocksInRowFullRes + 1] - m_cuTreeStrength * (log2_ratio);
                    }
                }
            }
        }
        else
        {
            for (int cuIndex = 0; cuIndex < m_cuCount; cuIndex++)
            {
                int intracost = (frame->intraCost[cuIndex] * frame->invQscaleFactor[cuIndex] + 128) >> 8;
                if (intracost)
                {
                    int propagateCost = (frame->propagateCost[cuIndex] * fpsFactor + 128) >> 8;
                    double log2_ratio = X265_LOG2(intracost + propagateCost) - X265_LOG2(intracost) + weightdelta;
                    frame->qpCuTreeOffset[cuIndex] = frame->qpAqOffset[cuIndex] - m_cuTreeStrength * log2_ratio;
                }
            }
        }
    }
}

/* If MB-tree changes the quantizers, we need to recalculate the frame cost without
 * re-running lookahead. */
int64_t Lookahead::frameCostRecalculate(Lowres** frames, int p0, int p1, int b)
{
    if (frames[b]->sliceType == X265_TYPE_B)
        return frames[b]->costEstAq[b - p0][p1 - b];

    int64_t score = 0;
    int *rowSatd = frames[b]->rowSatds[b - p0][p1 - b];

    x265_emms();

    if (m_param->rc.hevcAq)
    {
        int minAQDepth = frames[b]->pAQLayer->minAQDepth;
        PicQPAdaptationLayer* pQPLayer = &frames[b]->pAQLayer[minAQDepth];
        double* pcQPCuTree = pQPLayer->dCuTreeOffset;

        // Use new qp offset values for qpAqOffset, qpCuTreeOffset and invQscaleFactor buffer
        for (int cuy = m_8x8Height - 1; cuy >= 0; cuy--)
        {
            rowSatd[cuy] = 0;
            for (int cux = m_8x8Width - 1; cux >= 0; cux--)
            {
                int cuxy = cux + cuy * m_8x8Width;
                int cuCost = frames[b]->lowresCosts[b - p0][p1 - b][cuxy] & LOWRES_COST_MASK;
                double qp_adj;

                if (m_param->rc.qgSize == 8)
                    qp_adj = (pcQPCuTree[cux * 2 + cuy * m_8x8Width * 4] +
                    pcQPCuTree[cux * 2 + cuy * m_8x8Width * 4 + 1] +
                    pcQPCuTree[cux * 2 + cuy * m_8x8Width * 4 + frames[b]->maxBlocksInRowFullRes] +
                    pcQPCuTree[cux * 2 + cuy * m_8x8Width * 4 + frames[b]->maxBlocksInRowFullRes + 1]) / 4;
                else
                    qp_adj = *(pcQPCuTree + cuxy);

                cuCost = (cuCost * x265_exp2fix8(qp_adj) + 128) >> 8;
                rowSatd[cuy] += cuCost;
                if ((cuy > 0 && cuy < m_8x8Height - 1 &&
                    cux > 0 && cux < m_8x8Width - 1) ||
                    m_8x8Width <= 2 || m_8x8Height <= 2)
                {
                    score += cuCost;
                }
            }
        }
    }
    else
    {
        double *qp_offset = frames[b]->qpCuTreeOffset;

        for (int cuy = m_8x8Height - 1; cuy >= 0; cuy--)
        {
            rowSatd[cuy] = 0;
            for (int cux = m_8x8Width - 1; cux >= 0; cux--)
            {
                int cuxy = cux + cuy * m_8x8Width;
                int cuCost = frames[b]->lowresCosts[b - p0][p1 - b][cuxy] & LOWRES_COST_MASK;
                double qp_adj;
                if (m_param->rc.qgSize == 8)
                    qp_adj = (qp_offset[cux * 2 + cuy * m_8x8Width * 4] +
                    qp_offset[cux * 2 + cuy * m_8x8Width * 4 + 1] +
                    qp_offset[cux * 2 + cuy * m_8x8Width * 4 + frames[b]->maxBlocksInRowFullRes] +
                    qp_offset[cux * 2 + cuy * m_8x8Width * 4 + frames[b]->maxBlocksInRowFullRes + 1]) / 4;
                else 
                    qp_adj = qp_offset[cuxy];
                cuCost = (cuCost * x265_exp2fix8(qp_adj) + 128) >> 8;
                rowSatd[cuy] += cuCost;
                if ((cuy > 0 && cuy < m_8x8Height - 1 &&
                    cux > 0 && cux < m_8x8Width - 1) ||
                    m_8x8Width <= 2 || m_8x8Height <= 2)
                {
                    score += cuCost;
                }
            }
        }
    }

    return score;
}


int64_t CostEstimateGroup::singleCost(int p0, int p1, int b, bool intraPenalty)
{
    LookaheadTLD& tld = m_lookahead.m_tld[m_lookahead.m_pool ? m_lookahead.m_pool->m_numWorkers : 0];
    return estimateFrameCost(tld, p0, p1, b, intraPenalty);
}

void CostEstimateGroup::add(int p0, int p1, int b)
{
    X265_CHECK(m_batchMode || !m_jobTotal, "single CostEstimateGroup instance cannot mix batch modes\n");
    m_batchMode = true;

    Estimate& e = m_estimates[m_jobTotal++];
    e.p0 = p0;
    e.p1 = p1;
    e.b = b;

    if (m_jobTotal == MAX_BATCH_SIZE)
        finishBatch();
}

void CostEstimateGroup::finishBatch()
{
    if (m_lookahead.m_pool)
        tryBondPeers(*m_lookahead.m_pool, m_jobTotal);
    processTasks(-1);
    waitForExit();
    m_jobTotal = m_jobAcquired = 0;
}

void CostEstimateGroup::processTasks(int workerThreadID)
{
    ThreadPool* pool = m_lookahead.m_pool;
    int id = workerThreadID;
    if (workerThreadID < 0)
        id = pool ? pool->m_numWorkers : 0;
    LookaheadTLD& tld = m_lookahead.m_tld[id];

    m_lock.acquire();
    while (m_jobAcquired < m_jobTotal)
    {
        int i = m_jobAcquired++;
        m_lock.release();

        if (m_batchMode)
        {
            ProfileLookaheadTime(tld.batchElapsedTime, tld.countBatches);
            ProfileScopeEvent(estCostSingle);

            Estimate& e = m_estimates[i];
            estimateFrameCost(tld, e.p0, e.p1, e.b, false);
        }
        else
        {
            ProfileLookaheadTime(tld.coopSliceElapsedTime, tld.countCoopSlices);
            ProfileScopeEvent(estCostCoop);

            X265_CHECK(i < MAX_COOP_SLICES, "impossible number of coop slices\n");

            int firstY, lastY;
            bool lastRow;
            if (m_lookahead.m_param->bEnableHME)
            {
                int numRowsPerSlice = m_lookahead.m_4x4Height / m_lookahead.m_param->lookaheadSlices;
                numRowsPerSlice = X265_MIN(X265_MAX(numRowsPerSlice, 5), m_lookahead.m_4x4Height);
                firstY = numRowsPerSlice * i;
                lastY = (i == m_jobTotal - 1) ? m_lookahead.m_4x4Height - 1 : numRowsPerSlice * (i + 1) - 1;
                lastRow = true;
                for (int cuY = lastY; cuY >= firstY; cuY--)
                {
                    for (int cuX = m_lookahead.m_4x4Width - 1; cuX >= 0; cuX--)
                        estimateCUCost(tld, cuX, cuY, m_coop.p0, m_coop.p1, m_coop.b, m_coop.bDoSearch, lastRow, i, 1);
                    lastRow = false;
                }
            }

            firstY = m_lookahead.m_numRowsPerSlice * i;
            lastY = (i == m_jobTotal - 1) ? m_lookahead.m_8x8Height - 1 : m_lookahead.m_numRowsPerSlice * (i + 1) - 1;
            lastRow = true;
            for (int cuY = lastY; cuY >= firstY; cuY--)
            {
                m_frames[m_coop.b]->rowSatds[m_coop.b - m_coop.p0][m_coop.p1 - m_coop.b][cuY] = 0;

                for (int cuX = m_lookahead.m_8x8Width - 1; cuX >= 0; cuX--)
                    estimateCUCost(tld, cuX, cuY, m_coop.p0, m_coop.p1, m_coop.b, m_coop.bDoSearch, lastRow, i, 0);

                lastRow = false;
            }
        }

        m_lock.acquire();
    }
    m_lock.release();
}

int64_t CostEstimateGroup::estimateFrameCost(LookaheadTLD& tld, int p0, int p1, int b, bool bIntraPenalty)
{
    Lowres*     fenc  = m_frames[b];
    x265_param* param = m_lookahead.m_param;
    int64_t     score = 0;

    if (fenc->costEst[b - p0][p1 - b] >= 0 && fenc->rowSatds[b - p0][p1 - b][0] != -1)
        score = fenc->costEst[b - p0][p1 - b];
    else
    {
        bool bDoSearch[2];
        bDoSearch[0] = fenc->lowresMvs[0][b - p0][0].x == 0x7FFF;
        bDoSearch[1] = p1 > b && fenc->lowresMvs[1][p1 - b][0].x == 0x7FFF;

#if CHECKED_BUILD
        X265_CHECK(!(p0 < b && fenc->lowresMvs[0][b - p0][0].x == 0x7FFE), "motion search batch duplication L0\n");
        X265_CHECK(!(p1 > b && fenc->lowresMvs[1][p1 - b][0].x == 0x7FFE), "motion search batch duplication L1\n");
        if (bDoSearch[0]) fenc->lowresMvs[0][b - p0][0].x = 0x7FFE;
        if (bDoSearch[1]) fenc->lowresMvs[1][p1 - b][0].x = 0x7FFE;
#endif

        fenc->weightedRef[b - p0].isWeighted = false;
        if (param->bEnableWeightedPred && bDoSearch[0])
            tld.weightsAnalyse(*m_frames[b], *m_frames[p0]);

        fenc->costEst[b - p0][p1 - b] = 0;
        fenc->costEstAq[b - p0][p1 - b] = 0;

        if (!m_batchMode && m_lookahead.m_numCoopSlices > 1 && ((p1 > b) || bDoSearch[0] || bDoSearch[1]))
        {
            /* Use cooperative mode if a thread pool is available and the cost estimate is
             * going to need motion searches or bidir measurements */

            memset(&m_slice, 0, sizeof(Slice) * m_lookahead.m_numCoopSlices);

            m_lock.acquire();
            X265_CHECK(!m_batchMode, "single CostEstimateGroup instance cannot mix batch modes\n");
            m_coop.p0 = p0;
            m_coop.p1 = p1;
            m_coop.b = b;
            m_coop.bDoSearch[0] = bDoSearch[0];
            m_coop.bDoSearch[1] = bDoSearch[1];
            m_jobTotal = m_lookahead.m_numCoopSlices;
            m_jobAcquired = 0;
            m_lock.release();

            tryBondPeers(*m_lookahead.m_pool, m_jobTotal);

            processTasks(-1);

            waitForExit();

            for (int i = 0; i < m_lookahead.m_numCoopSlices; i++)
            {
                fenc->costEst[b - p0][p1 - b] += m_slice[i].costEst;
                fenc->costEstAq[b - p0][p1 - b] += m_slice[i].costEstAq;
                if (p1 == b)
                    fenc->intraMbs[b - p0] += m_slice[i].intraMbs;
            }
        }
        else
        {
            /* Calculate MVs for 1/16th resolution*/
            bool lastRow;
            if (param->bEnableHME)
            {
                lastRow = true;
                for (int cuY = m_lookahead.m_4x4Height - 1; cuY >= 0; cuY--)
                {
                    for (int cuX = m_lookahead.m_4x4Width - 1; cuX >= 0; cuX--)
                        estimateCUCost(tld, cuX, cuY, p0, p1, b, bDoSearch, lastRow, -1, 1);
                    lastRow = false;
                }
            }
            lastRow = true;
            for (int cuY = m_lookahead.m_8x8Height - 1; cuY >= 0; cuY--)
            {
                fenc->rowSatds[b - p0][p1 - b][cuY] = 0;

                for (int cuX = m_lookahead.m_8x8Width - 1; cuX >= 0; cuX--)
                    estimateCUCost(tld, cuX, cuY, p0, p1, b, bDoSearch, lastRow, -1, 0);

                lastRow = false;
            }
        }

        score = fenc->costEst[b - p0][p1 - b];

        if (b != p1)
            score = score * 100 / (130 + param->bFrameBias);

        fenc->costEst[b - p0][p1 - b] = score;
    }

    if (bIntraPenalty)
        // arbitrary penalty for I-blocks after B-frames
        score += score * fenc->intraMbs[b - p0] / (tld.ncu * 8);

    return score;
}

void CostEstimateGroup::estimateCUCost(LookaheadTLD& tld, int cuX, int cuY, int p0, int p1, int b, bool bDoSearch[2], bool lastRow, int slice, bool hme)
{
    Lowres *fref0 = m_frames[p0];
    Lowres *fref1 = m_frames[p1];
    Lowres *fenc  = m_frames[b];

    ReferencePlanes *wfref0 = fenc->weightedRef[b - p0].isWeighted && !hme ? &fenc->weightedRef[b - p0] : fref0;

    const int widthInCU = hme ? m_lookahead.m_4x4Width : m_lookahead.m_8x8Width;
    const int heightInCU = hme ? m_lookahead.m_4x4Height : m_lookahead.m_8x8Height;
    const int bBidir = (b < p1);
    const int cuXY = cuX + cuY * widthInCU;
    const int cuXY_4x4 = (cuX / 2) + (cuY / 2) * widthInCU / 2;
    const int cuSize = X265_LOWRES_CU_SIZE;
    const intptr_t pelOffset = cuSize * cuX + cuSize * cuY * (hme ? fenc->lumaStride/2 : fenc->lumaStride);

    if ((bBidir || bDoSearch[0] || bDoSearch[1]) && hme)
        tld.me.setSourcePU(fenc->lowerResPlane[0], fenc->lumaStride / 2, pelOffset, cuSize, cuSize, X265_HEX_SEARCH, m_lookahead.m_param->hmeSearchMethod[0], m_lookahead.m_param->hmeSearchMethod[1], 1);
    else if((bBidir || bDoSearch[0] || bDoSearch[1]) && !hme)
        tld.me.setSourcePU(fenc->lowresPlane[0], fenc->lumaStride, pelOffset, cuSize, cuSize, X265_HEX_SEARCH, m_lookahead.m_param->hmeSearchMethod[0], m_lookahead.m_param->hmeSearchMethod[1], 1);


    /* A small, arbitrary bias to avoid VBV problems caused by zero-residual lookahead blocks. */
    int lowresPenalty = 4;
    int listDist[2] = { b - p0, p1 - b};

    MV mvmin, mvmax;
    int bcost = tld.me.COST_MAX;
    int listused = 0;

    // TODO: restrict to slices boundaries
    // establish search bounds that don't cross extended frame boundaries
    mvmin.x = (int32_t)(-cuX * cuSize - 8);
    mvmin.y = (int32_t)(-cuY * cuSize - 8);
    mvmax.x = (int32_t)((widthInCU - cuX - 1) * cuSize + 8);
    mvmax.y = (int32_t)((heightInCU - cuY - 1) * cuSize + 8);

    for (int i = 0; i < 1 + bBidir; i++)
    {
        int& fencCost = hme ? fenc->lowerResMvCosts[i][listDist[i]][cuXY] : fenc->lowresMvCosts[i][listDist[i]][cuXY];
        int skipCost = INT_MAX;

        if (!bDoSearch[i])
        {
            COPY2_IF_LT(bcost, fencCost, listused, i + 1);
            continue;
        }

        int numc = 0;
        MV mvc[5], mvp;
        MV* fencMV = hme ? &fenc->lowerResMvs[i][listDist[i]][cuXY] : &fenc->lowresMvs[i][listDist[i]][cuXY];
        ReferencePlanes* fref = i ? fref1 : wfref0;

        /* Reverse-order MV prediction */
#define MVC(mv) mvc[numc++] = mv;
        if (cuX < widthInCU - 1)
            MVC(fencMV[1]);
        if (!lastRow)
        {
            MVC(fencMV[widthInCU]);
            if (cuX > 0)
                MVC(fencMV[widthInCU - 1]);
            if (cuX < widthInCU - 1)
                MVC(fencMV[widthInCU + 1]);
        }
        if (fenc->lowerResMvs[0][0] && !hme && fenc->lowerResMvCosts[i][listDist[i]][cuXY_4x4] > 0)
        {
            MVC((fenc->lowerResMvs[i][listDist[i]][cuXY_4x4]) * 2);
        }
#undef MVC

        if (!numc)
            mvp = 0;
        else
        {
            ALIGN_VAR_32(pixel, subpelbuf[X265_LOWRES_CU_SIZE * X265_LOWRES_CU_SIZE]);
            int mvpcost = MotionEstimate::COST_MAX;

            /* measure SATD cost of each neighbor MV (estimating merge analysis)
             * and use the lowest cost MV as MVP (estimating AMVP). Since all
             * mvc[] candidates are measured here, none are passed to motionEstimate */
            for (int idx = 0; idx < numc; idx++)
            {
                intptr_t stride = X265_LOWRES_CU_SIZE;
                pixel *src = fref->lowresMC(pelOffset, mvc[idx], subpelbuf, stride, hme);
                int cost = tld.me.bufSATD(src, stride);
                COPY2_IF_LT(mvpcost, cost, mvp, mvc[idx]);
                /* Except for mv0 case, everyting else is likely to have enough residual to not trigger the skip. */
                if (!mvp.notZero() && bBidir)
                    skipCost = cost;
            }
        }

        int searchRange = m_lookahead.m_param->bEnableHME ? (hme ? m_lookahead.m_param->hmeRange[0] : m_lookahead.m_param->hmeRange[1]) : s_merange;
        /* ME will never return a cost larger than the cost @MVP, so we do not
         * have to check that ME cost is more than the estimated merge cost */
        if(!hme)
            fencCost = tld.me.motionEstimate(fref, mvmin, mvmax, mvp, 0, NULL, searchRange, *fencMV, m_lookahead.m_param->maxSlices);
        else
            fencCost = tld.me.motionEstimate(fref, mvmin, mvmax, mvp, 0, NULL, searchRange, *fencMV, m_lookahead.m_param->maxSlices, fref->lowerResPlane[0]);
        if (skipCost < 64 && skipCost < fencCost && bBidir)
        {
            fencCost = skipCost;
            *fencMV = 0;
        }
        COPY2_IF_LT(bcost, fencCost, listused, i + 1);
    }
    if (hme)
        return;

    if (bBidir) /* B, also consider bidir */
    {
        /* NOTE: the wfref0 (weightp) is not used for BIDIR */

        /* avg(l0-mv, l1-mv) candidate */
        ALIGN_VAR_32(pixel, subpelbuf0[X265_LOWRES_CU_SIZE * X265_LOWRES_CU_SIZE]);
        ALIGN_VAR_32(pixel, subpelbuf1[X265_LOWRES_CU_SIZE * X265_LOWRES_CU_SIZE]);
        intptr_t stride0 = X265_LOWRES_CU_SIZE, stride1 = X265_LOWRES_CU_SIZE;
        pixel *src0 = fref0->lowresMC(pelOffset, fenc->lowresMvs[0][listDist[0]][cuXY], subpelbuf0, stride0, 0);
        pixel *src1 = fref1->lowresMC(pelOffset, fenc->lowresMvs[1][listDist[1]][cuXY], subpelbuf1, stride1, 0);
        ALIGN_VAR_32(pixel, ref[X265_LOWRES_CU_SIZE * X265_LOWRES_CU_SIZE]);
        primitives.pu[LUMA_8x8].pixelavg_pp[NONALIGNED](ref, X265_LOWRES_CU_SIZE, src0, stride0, src1, stride1, 32);
        int bicost = tld.me.bufSATD(ref, X265_LOWRES_CU_SIZE);
        COPY2_IF_LT(bcost, bicost, listused, 3);
        /* coloc candidate */
        src0 = fref0->lowresPlane[0] + pelOffset;
        src1 = fref1->lowresPlane[0] + pelOffset;
        primitives.pu[LUMA_8x8].pixelavg_pp[NONALIGNED](ref, X265_LOWRES_CU_SIZE, src0, fref0->lumaStride, src1, fref1->lumaStride, 32);
        bicost = tld.me.bufSATD(ref, X265_LOWRES_CU_SIZE);
        COPY2_IF_LT(bcost, bicost, listused, 3);
        bcost += lowresPenalty;
    }
    else /* P, also consider intra */
    {
        bcost += lowresPenalty;

        if (fenc->intraCost[cuXY] < bcost)
        {
            bcost = fenc->intraCost[cuXY];
            listused = 0;
        }
    }

    /* do not include edge blocks in the frame cost estimates, they are not very accurate */
    const bool bFrameScoreCU = (cuX > 0 && cuX < widthInCU - 1 &&
                                cuY > 0 && cuY < heightInCU - 1) || widthInCU <= 2 || heightInCU <= 2;
    int bcostAq;
    if (m_lookahead.m_param->rc.qgSize == 8)
        bcostAq = (bFrameScoreCU && fenc->invQscaleFactor) ? ((bcost * fenc->invQscaleFactor8x8[cuXY] + 128) >> 8) : bcost;
    else
        bcostAq = (bFrameScoreCU && fenc->invQscaleFactor) ? ((bcost * fenc->invQscaleFactor[cuXY] +128) >> 8) : bcost;

    if (bFrameScoreCU)
    {
        if (slice < 0)
        {
            fenc->costEst[b - p0][p1 - b] += bcost;
            fenc->costEstAq[b - p0][p1 - b] += bcostAq;
            if (!listused && !bBidir)
                fenc->intraMbs[b - p0]++;
        }
        else
        {
            m_slice[slice].costEst += bcost;
            m_slice[slice].costEstAq += bcostAq;
            if (!listused && !bBidir)
                m_slice[slice].intraMbs++;
        }
    }

    fenc->rowSatds[b - p0][p1 - b][cuY] += bcostAq;
    fenc->lowresCosts[b - p0][p1 - b][cuXY] = (uint16_t)(X265_MIN(bcost, LOWRES_COST_MASK) | (listused << LOWRES_COST_SHIFT));
}

}
