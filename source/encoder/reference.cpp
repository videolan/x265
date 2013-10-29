/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Deepthi Devaki <deepthidevaki@multicorewareinc.com>
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

#include "TLibCommon/TypeDef.h"
#include "TLibCommon/TComPicYuv.h"
#include "TLibCommon/TComSlice.h"
#include "primitives.h"
#include "reference.h"
#include "common.h"

using namespace x265;

MotionReference::MotionReference()
{
    m_weightBuffer = NULL;
}

int MotionReference::init(TComPicYuv* pic, wpScalingParam *w)
{
    m_reconPic = pic;
    unweightedFPelPlane = pic->getLumaAddr();
    lumaStride = pic->getStride();
    intptr_t startpad = pic->m_lumaMarginY * lumaStride + pic->m_lumaMarginX;

    /* directly reference the pre-extended integer pel plane */
    fpelPlane = pic->m_picBufY + startpad;
    isWeighted = false;

    if (w)
    {
        if (!m_weightBuffer)
        {
            size_t padheight = (pic->m_numCuInHeight * g_maxCUHeight) + pic->m_lumaMarginY * 2;
            m_weightBuffer = (pixel*)X265_MALLOC(pixel, lumaStride * padheight);
            if (!m_weightBuffer)
                return -1;
        }

        isWeighted = true;
        weight = w->inputWeight;
        offset = w->inputOffset * (1 << (X265_DEPTH - 8));
        shift  = w->log2WeightDenom;
        round  = (w->log2WeightDenom >= 1) ? (1 << (w->log2WeightDenom - 1)) : (0);
        m_numWeightedRows = 0;

        /* use our buffer which will have weighted pixels written to it */
        fpelPlane = m_weightBuffer + startpad;
    }

    return 0;
}

MotionReference::~MotionReference()
{
    X265_FREE(m_weightBuffer);
}

void MotionReference::applyWeight(int rows, int numRows)
{
    rows = X265_MIN(rows, numRows);
    if (m_numWeightedRows >= rows)
        return;
    int marginX = m_reconPic->m_lumaMarginX;
    int marginY = m_reconPic->m_lumaMarginY;
    pixel* src = (pixel*)m_reconPic->getLumaAddr() + (m_numWeightedRows * (int)g_maxCUHeight * lumaStride);
    pixel* dst = fpelPlane + ((m_numWeightedRows * (int)g_maxCUHeight) * lumaStride);
    int width = m_reconPic->getWidth();
    int height = ((rows - m_numWeightedRows) * g_maxCUHeight);
    if (rows == numRows)
        height = ((m_reconPic->getHeight() % g_maxCUHeight) ? (m_reconPic->getHeight() % g_maxCUHeight) : g_maxCUHeight);
    size_t dstStride = lumaStride;

    // Computing weighted CU rows
    int shiftNum = IF_INTERNAL_PREC - X265_DEPTH;
    int local_shift = shift + shiftNum;
    int local_round = local_shift ? (1 << (local_shift - 1)) : 0;
    primitives.weightpUniPixel(src, dst, lumaStride, dstStride, width, height, weight, local_round, local_shift, offset);

    // Extending Left & Right
    primitives.extendRowBorder(dst, dstStride, width, height, marginX);

    // Extending Above
    if (m_numWeightedRows == 0)
    {
        pixel *pixY = fpelPlane - marginX;
        for (int y = 0; y < marginY; y++)
        {
            memcpy(pixY - (y + 1) * dstStride, pixY, dstStride * sizeof(pixel));
        }
    }

    // Extending Bottom
    if (rows == numRows)
    {
        pixel *pixY = fpelPlane - marginX + (m_reconPic->getHeight() - 1) * dstStride;
        for (int y = 0; y < marginY; y++)
        {
            memcpy(pixY + (y + 1) * dstStride, pixY, dstStride * sizeof(pixel));
        }
    }
    m_numWeightedRows = rows;
}
