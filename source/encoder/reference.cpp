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
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#include "common.h"
#include "primitives.h"
#include "slice.h"
#include "picyuv.h"

#include "reference.h"

using namespace x265;

MotionReference::MotionReference()
{
    m_weightBuffer = NULL;
}

int MotionReference::init(PicYuv* recPic, WeightParam *w)
{
    reconPic = recPic;
    m_numWeightedRows = 0;
    lumaStride = recPic->m_stride;
    intptr_t startpad = recPic->m_lumaMarginY * lumaStride + recPic->m_lumaMarginX;

    /* directly reference the pre-extended integer pel plane */
    fpelPlane = recPic->m_picBuf[0] + startpad;
    isWeighted = false;

    if (w)
    {
        if (!m_weightBuffer)
        {
            uint32_t numCUinHeight = (recPic->m_picHeight + g_maxCUSize - 1) / g_maxCUSize;
            size_t padheight = (numCUinHeight * g_maxCUSize) + recPic->m_lumaMarginY * 2;
            m_weightBuffer = X265_MALLOC(pixel, lumaStride * padheight);
            if (!m_weightBuffer)
                return -1;
        }

        isWeighted = true;
        weight = w->inputWeight;
        offset = w->inputOffset * (1 << (X265_DEPTH - 8));
        shift  = w->log2WeightDenom;
        round  = shift ? 1 << (shift - 1) : 0;

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
    int marginX = reconPic->m_lumaMarginX;
    int marginY = reconPic->m_lumaMarginY;
    pixel* src = (pixel*)reconPic->m_picOrg[0] + (m_numWeightedRows * (int)g_maxCUSize * lumaStride);
    pixel* dst = fpelPlane + ((m_numWeightedRows * (int)g_maxCUSize) * lumaStride);
    int width = reconPic->m_picWidth;
    int height = ((rows - m_numWeightedRows) * g_maxCUSize);
    if (rows == numRows)
        height = ((reconPic->m_picHeight % g_maxCUSize) ? (reconPic->m_picHeight % g_maxCUSize) : g_maxCUSize);

    // Computing weighted CU rows
    int correction = IF_INTERNAL_PREC - X265_DEPTH; // intermediate interpolation depth
    int padwidth = (width + 15) & ~15;  // weightp assembly needs even 16 byte widths
    primitives.weight_pp(src, dst, lumaStride, padwidth, height, weight, round << correction, shift + correction, offset);

    // Extending Left & Right
    primitives.extendRowBorder(dst, lumaStride, width, height, marginX);

    // Extending Above
    if (m_numWeightedRows == 0)
    {
        pixel *pixY = fpelPlane - marginX;
        for (int y = 0; y < marginY; y++)
            memcpy(pixY - (y + 1) * lumaStride, pixY, lumaStride * sizeof(pixel));
    }

    // Extending Bottom
    if (rows == numRows)
    {
        pixel *pixY = fpelPlane - marginX + (reconPic->m_picHeight - 1) * lumaStride;
        for (int y = 0; y < marginY; y++)
            memcpy(pixY + (y + 1) * lumaStride, pixY, lumaStride * sizeof(pixel));
    }

    m_numWeightedRows = rows;
}
