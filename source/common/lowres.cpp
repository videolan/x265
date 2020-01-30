/*****************************************************************************
 * Copyright (C) 2013-2020 MulticoreWare, Inc
 *
 * Authors: Gopu Govindaswamy <gopu@multicorewareinc.com>
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

#include "picyuv.h"
#include "lowres.h"
#include "mv.h"

using namespace X265_NS;

bool PicQPAdaptationLayer::create(uint32_t width, uint32_t height, uint32_t partWidth, uint32_t partHeight, uint32_t numAQPartInWidthExt, uint32_t numAQPartInHeightExt)
{
    aqPartWidth = partWidth;
    aqPartHeight = partHeight;
    numAQPartInWidth = (width + partWidth - 1) / partWidth;
    numAQPartInHeight = (height + partHeight - 1) / partHeight;

    CHECKED_MALLOC_ZERO(dActivity, double, numAQPartInWidthExt * numAQPartInHeightExt);
    CHECKED_MALLOC_ZERO(dQpOffset, double, numAQPartInWidthExt * numAQPartInHeightExt);
    CHECKED_MALLOC_ZERO(dCuTreeOffset, double, numAQPartInWidthExt * numAQPartInHeightExt);

    if (bQpSize)
        CHECKED_MALLOC_ZERO(dCuTreeOffset8x8, double, numAQPartInWidthExt * numAQPartInHeightExt);

    return true;
fail:
    return false;
}

bool Lowres::create(x265_param* param, PicYuv *origPic, uint32_t qgSize)
{
    isLowres = true;
    bframes = param->bframes;
    widthFullRes = origPic->m_picWidth;
    heightFullRes = origPic->m_picHeight;
    width = origPic->m_picWidth / 2;
    lines = origPic->m_picHeight / 2;
    bEnableHME = param->bEnableHME ? 1 : 0;
    lumaStride = width + 2 * origPic->m_lumaMarginX;
    if (lumaStride & 31)
        lumaStride += 32 - (lumaStride & 31);
    maxBlocksInRow = (width + X265_LOWRES_CU_SIZE - 1) >> X265_LOWRES_CU_BITS;
    maxBlocksInCol = (lines + X265_LOWRES_CU_SIZE - 1) >> X265_LOWRES_CU_BITS;
    maxBlocksInRowFullRes = maxBlocksInRow * 2;
    maxBlocksInColFullRes = maxBlocksInCol * 2;
    int cuCount = maxBlocksInRow * maxBlocksInCol;
    int cuCountFullRes = (qgSize > 8) ? cuCount : cuCount << 2;
    isHMELowres = param->bEnableHME ? 1 : 0;

    /* rounding the width to multiple of lowres CU size */
    width = maxBlocksInRow * X265_LOWRES_CU_SIZE;
    lines = maxBlocksInCol * X265_LOWRES_CU_SIZE;

    size_t planesize = lumaStride * (lines + 2 * origPic->m_lumaMarginY);
    size_t padoffset = lumaStride * origPic->m_lumaMarginY + origPic->m_lumaMarginX;
    if (!!param->rc.aqMode || !!param->rc.hevcAq || !!param->bAQMotion)
    {
        CHECKED_MALLOC_ZERO(qpAqOffset, double, cuCountFullRes);
        CHECKED_MALLOC_ZERO(invQscaleFactor, int, cuCountFullRes);
        CHECKED_MALLOC_ZERO(qpCuTreeOffset, double, cuCountFullRes);
        if (qgSize == 8)
            CHECKED_MALLOC_ZERO(invQscaleFactor8x8, int, cuCount);
        CHECKED_MALLOC_ZERO(edgeInclined, int, cuCountFullRes);
    }

    if (origPic->m_param->bAQMotion)
        CHECKED_MALLOC_ZERO(qpAqMotionOffset, double, cuCountFullRes);
    if (origPic->m_param->bDynamicRefine || origPic->m_param->bEnableFades)
        CHECKED_MALLOC_ZERO(blockVariance, uint32_t, cuCountFullRes);

    if (!!param->rc.hevcAq)
    {
        m_maxCUSize = param->maxCUSize;
        m_qgSize = qgSize;

        uint32_t partWidth, partHeight, nAQPartInWidth, nAQPartInHeight;

        pAQLayer = new PicQPAdaptationLayer[4];
        maxAQDepth = 0;
        for (uint32_t d = 0; d < 4; d++)
        {
            int ctuSizeIdx = 6 - g_log2Size[param->maxCUSize];
            int aqDepth = g_log2Size[param->maxCUSize] - g_log2Size[qgSize];
            if (!aqLayerDepth[ctuSizeIdx][aqDepth][d])
                continue;

            pAQLayer->minAQDepth = d;
            partWidth = param->maxCUSize >> d;
            partHeight = param->maxCUSize >> d;

            if (minAQSize[ctuSizeIdx] == d)
            {
                pAQLayer[d].bQpSize = true;
                nAQPartInWidth = maxBlocksInRow * 2;
                nAQPartInHeight = maxBlocksInCol * 2;
            }
            else
            {
                pAQLayer[d].bQpSize = false;
                nAQPartInWidth = (origPic->m_picWidth + partWidth - 1) / partWidth;
                nAQPartInHeight = (origPic->m_picHeight + partHeight - 1) / partHeight;
            }

            maxAQDepth++;

            pAQLayer[d].create(origPic->m_picWidth, origPic->m_picHeight, partWidth, partHeight, nAQPartInWidth, nAQPartInHeight);
        }
    }
    CHECKED_MALLOC(propagateCost, uint16_t, cuCount);

    /* allocate lowres buffers */
    CHECKED_MALLOC_ZERO(buffer[0], pixel, 4 * planesize);

    buffer[1] = buffer[0] + planesize;
    buffer[2] = buffer[1] + planesize;
    buffer[3] = buffer[2] + planesize;

    lowresPlane[0] = buffer[0] + padoffset;
    lowresPlane[1] = buffer[1] + padoffset;
    lowresPlane[2] = buffer[2] + padoffset;
    lowresPlane[3] = buffer[3] + padoffset;

    if (bEnableHME)
    {
        intptr_t lumaStrideHalf = lumaStride / 2;
        if (lumaStrideHalf & 31)
            lumaStrideHalf += 32 - (lumaStrideHalf & 31);
        size_t planesizeHalf = planesize / 2;
        size_t padoffsetHalf = padoffset / 2;
        /* allocate lower-res buffers */
        CHECKED_MALLOC_ZERO(lowerResBuffer[0], pixel, 4 * planesizeHalf);

        lowerResBuffer[1] = lowerResBuffer[0] + planesizeHalf;
        lowerResBuffer[2] = lowerResBuffer[1] + planesizeHalf;
        lowerResBuffer[3] = lowerResBuffer[2] + planesizeHalf;

        lowerResPlane[0] = lowerResBuffer[0] + padoffsetHalf;
        lowerResPlane[1] = lowerResBuffer[1] + padoffsetHalf;
        lowerResPlane[2] = lowerResBuffer[2] + padoffsetHalf;
        lowerResPlane[3] = lowerResBuffer[3] + padoffsetHalf;
    }

    CHECKED_MALLOC(intraCost, int32_t, cuCount);
    CHECKED_MALLOC(intraMode, uint8_t, cuCount);

    for (int i = 0; i < bframes + 2; i++)
    {
        for (int j = 0; j < bframes + 2; j++)
        {
            CHECKED_MALLOC(rowSatds[i][j], int32_t, maxBlocksInCol);
            CHECKED_MALLOC(lowresCosts[i][j], uint16_t, cuCount);
        }
    }

    for (int i = 0; i < bframes + 2; i++)
    {
        CHECKED_MALLOC(lowresMvs[0][i], MV, cuCount);
        CHECKED_MALLOC(lowresMvs[1][i], MV, cuCount);
        CHECKED_MALLOC(lowresMvCosts[0][i], int32_t, cuCount);
        CHECKED_MALLOC(lowresMvCosts[1][i], int32_t, cuCount);
        if (bEnableHME)
        {
            int maxBlocksInRowLowerRes = ((width/2) + X265_LOWRES_CU_SIZE - 1) >> X265_LOWRES_CU_BITS;
            int maxBlocksInColLowerRes = ((lines/2) + X265_LOWRES_CU_SIZE - 1) >> X265_LOWRES_CU_BITS;
            int cuCountLowerRes = maxBlocksInRowLowerRes * maxBlocksInColLowerRes;
            CHECKED_MALLOC(lowerResMvs[0][i], MV, cuCountLowerRes);
            CHECKED_MALLOC(lowerResMvs[1][i], MV, cuCountLowerRes);
            CHECKED_MALLOC(lowerResMvCosts[0][i], int32_t, cuCountLowerRes);
            CHECKED_MALLOC(lowerResMvCosts[1][i], int32_t, cuCountLowerRes);
        }
    }

    return true;

fail:
    return false;
}

void Lowres::destroy()
{
    X265_FREE(buffer[0]);
    if(bEnableHME)
        X265_FREE(lowerResBuffer[0]);
    X265_FREE(intraCost);
    X265_FREE(intraMode);

    for (int i = 0; i < bframes + 2; i++)
    {
        for (int j = 0; j < bframes + 2; j++)
        {
            X265_FREE(rowSatds[i][j]);
            X265_FREE(lowresCosts[i][j]);
        }
    }

    for (int i = 0; i < bframes + 2; i++)
    {
        X265_FREE(lowresMvs[0][i]);
        X265_FREE(lowresMvs[1][i]);
        X265_FREE(lowresMvCosts[0][i]);
        X265_FREE(lowresMvCosts[1][i]);
        if (bEnableHME)
        {
            X265_FREE(lowerResMvs[0][i]);
            X265_FREE(lowerResMvs[1][i]);
            X265_FREE(lowerResMvCosts[0][i]);
            X265_FREE(lowerResMvCosts[1][i]);
        }
    }
    X265_FREE(qpAqOffset);
    X265_FREE(invQscaleFactor);
    X265_FREE(qpCuTreeOffset);
    X265_FREE(propagateCost);
    X265_FREE(invQscaleFactor8x8);
    X265_FREE(edgeInclined);
    X265_FREE(qpAqMotionOffset);
    X265_FREE(blockVariance);
    if (maxAQDepth > 0)
    {
        for (uint32_t d = 0; d < 4; d++)
        {
            int ctuSizeIdx = 6 - g_log2Size[m_maxCUSize];
            int aqDepth = g_log2Size[m_maxCUSize] - g_log2Size[m_qgSize];
            if (!aqLayerDepth[ctuSizeIdx][aqDepth][d])
                continue;

            X265_FREE(pAQLayer[d].dActivity);
            X265_FREE(pAQLayer[d].dQpOffset);
            X265_FREE(pAQLayer[d].dCuTreeOffset);

            if (pAQLayer[d].bQpSize == true)
                X265_FREE(pAQLayer[d].dCuTreeOffset8x8);
        }

        delete[] pAQLayer;
    }
}
// (re) initialize lowres state
void Lowres::init(PicYuv *origPic, int poc)
{
    bLastMiniGopBFrame = false;
    bKeyframe = false; // Not a keyframe unless identified by lookahead
    bIsFadeEnd = false;
    frameNum = poc;
    leadingBframes = 0;
    indB = 0;
    memset(costEst, -1, sizeof(costEst));
    memset(weightedCostDelta, 0, sizeof(weightedCostDelta));

    if (qpAqOffset && invQscaleFactor)
        memset(costEstAq, -1, sizeof(costEstAq));

    for (int y = 0; y < bframes + 2; y++)
        for (int x = 0; x < bframes + 2; x++)
            rowSatds[y][x][0] = -1;

    for (int i = 0; i < bframes + 2; i++)
    {
        lowresMvs[0][i][0].x = 0x7FFF;
        lowresMvs[1][i][0].x = 0x7FFF;
    }

    for (int i = 0; i < bframes + 2; i++)
        intraMbs[i] = 0;
    if (origPic->m_param->rc.vbvBufferSize)
        for (int i = 0; i < X265_LOOKAHEAD_MAX + 1; i++)
            plannedType[i] = X265_TYPE_AUTO;

    /* downscale and generate 4 hpel planes for lookahead */
    primitives.frameInitLowres(origPic->m_picOrg[0],
                               lowresPlane[0], lowresPlane[1], lowresPlane[2], lowresPlane[3],
                               origPic->m_stride, lumaStride, width, lines);

    /* extend hpel planes for motion search */
    extendPicBorder(lowresPlane[0], lumaStride, width, lines, origPic->m_lumaMarginX, origPic->m_lumaMarginY);
    extendPicBorder(lowresPlane[1], lumaStride, width, lines, origPic->m_lumaMarginX, origPic->m_lumaMarginY);
    extendPicBorder(lowresPlane[2], lumaStride, width, lines, origPic->m_lumaMarginX, origPic->m_lumaMarginY);
    extendPicBorder(lowresPlane[3], lumaStride, width, lines, origPic->m_lumaMarginX, origPic->m_lumaMarginY);
    
    if (origPic->m_param->bEnableHME)
    {
        primitives.frameInitLowerRes(lowresPlane[0],
            lowerResPlane[0], lowerResPlane[1], lowerResPlane[2], lowerResPlane[3],
            lumaStride, lumaStride/2, (width / 2), (lines / 2));
        extendPicBorder(lowerResPlane[0], lumaStride/2, width/2, lines/2, origPic->m_lumaMarginX/2, origPic->m_lumaMarginY/2);
        extendPicBorder(lowerResPlane[1], lumaStride/2, width/2, lines/2, origPic->m_lumaMarginX/2, origPic->m_lumaMarginY/2);
        extendPicBorder(lowerResPlane[2], lumaStride/2, width/2, lines/2, origPic->m_lumaMarginX/2, origPic->m_lumaMarginY/2);
        extendPicBorder(lowerResPlane[3], lumaStride/2, width/2, lines/2, origPic->m_lumaMarginX/2, origPic->m_lumaMarginY/2);
        fpelLowerResPlane[0] = lowerResPlane[0];
    }

    fpelPlane[0] = lowresPlane[0];
}
