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

using namespace X265_NS;

MotionReference::MotionReference()
{
    weightBuffer[0] = NULL;
    weightBuffer[1] = NULL;
    weightBuffer[2] = NULL;
}

MotionReference::~MotionReference()
{
    X265_FREE(weightBuffer[0]);
    X265_FREE(weightBuffer[1]);
    X265_FREE(weightBuffer[2]);
}

int MotionReference::init(PicYuv* recPic, WeightParam *wp, const x265_param& p)
{
    reconPic = recPic;
    numWeightedRows = 0;
    lumaStride = recPic->m_stride;
    chromaStride = recPic->m_strideC;
    numInterpPlanes = p.subpelRefine > 2 ? 3 : 1; /* is chroma satd possible? */

    /* directly reference the extended integer pel planes */
    fpelPlane[0] = recPic->m_picOrg[0];
    fpelPlane[1] = recPic->m_picOrg[1];
    fpelPlane[2] = recPic->m_picOrg[2];
    isWeighted = false;

    if (wp)
    {
        uint32_t numCUinHeight = (reconPic->m_picHeight + g_maxCUSize - 1) / g_maxCUSize;

        int marginX = reconPic->m_lumaMarginX;
        int marginY = reconPic->m_lumaMarginY;
        intptr_t stride = reconPic->m_stride;
        int cuHeight = g_maxCUSize;

        for (int c = 0; c < numInterpPlanes; c++)
        {
            if (c == 1)
            {
                marginX = reconPic->m_chromaMarginX;
                marginY = reconPic->m_chromaMarginY;
                stride  = reconPic->m_strideC;
                cuHeight >>= reconPic->m_vChromaShift;
            }

            if (wp[c].bPresentFlag)
            {
                if (!weightBuffer[c])
                {
                    size_t padheight = (numCUinHeight * cuHeight) + marginY * 2;
                    weightBuffer[c] = X265_MALLOC(pixel, stride * padheight);
                    if (!weightBuffer[c])
                        return -1;
                }

                /* use our buffer which will have weighted pixels written to it */
                fpelPlane[c] = weightBuffer[c] + marginY * stride + marginX;
                X265_CHECK(recPic->m_picOrg[c] - recPic->m_picBuf[c] == marginY * stride + marginX, "PicYuv pad calculation mismatch\n");

                w[c].weight = wp[c].inputWeight;
                w[c].offset = wp[c].inputOffset * (1 << (X265_DEPTH - 8));
                w[c].shift = wp[c].log2WeightDenom;
                w[c].round = w[c].shift ? 1 << (w[c].shift - 1) : 0;
            }
        }

        isWeighted = true;
    }

    return 0;
}

void MotionReference::applyWeight(int finishedRows, int maxNumRows)
{
    finishedRows = X265_MIN(finishedRows, maxNumRows);
    if (numWeightedRows >= finishedRows)
        return;

    int marginX = reconPic->m_lumaMarginX;
    int marginY = reconPic->m_lumaMarginY;
    intptr_t stride = reconPic->m_stride;
    int width   = reconPic->m_picWidth;
    int height  = (finishedRows - numWeightedRows) * g_maxCUSize;
    if (finishedRows == maxNumRows && (reconPic->m_picHeight % g_maxCUSize))
    {
        /* the last row may be partial height */
        height -= g_maxCUSize;
        height += reconPic->m_picHeight % g_maxCUSize;
    }
    int cuHeight = g_maxCUSize;

    for (int c = 0; c < numInterpPlanes; c++)
    {
        if (c == 1)
        {
            marginX = reconPic->m_chromaMarginX;
            marginY = reconPic->m_chromaMarginY;
            stride  = reconPic->m_strideC;
            width    >>= reconPic->m_hChromaShift;
            height   >>= reconPic->m_vChromaShift;
            cuHeight >>= reconPic->m_vChromaShift;
        }

        /* Do not generate weighted predictions if using original picture */
        if (fpelPlane[c] == reconPic->m_picOrg[c])
            continue;

        const pixel* src = reconPic->m_picOrg[c] + numWeightedRows * cuHeight * stride;
        pixel* dst = fpelPlane[c] + numWeightedRows * cuHeight * stride;

        // Computing weighted CU rows
        int correction = IF_INTERNAL_PREC - X265_DEPTH; // intermediate interpolation depth
        int padwidth = (width + 15) & ~15;              // weightp assembly needs even 16 byte widths
        primitives.weight_pp(src, dst, stride, padwidth, height, w[c].weight, w[c].round << correction, w[c].shift + correction, w[c].offset);

        // Extending Left & Right
        primitives.extendRowBorder(dst, stride, width, height, marginX);

        // Extending Above
        if (numWeightedRows == 0)
        {
            pixel *pixY = fpelPlane[c] - marginX;
            for (int y = 0; y < marginY; y++)
                memcpy(pixY - (y + 1) * stride, pixY, stride * sizeof(pixel));
        }

        // Extending Bottom
        if (finishedRows == maxNumRows)
        {
            int picHeight = reconPic->m_picHeight;
            if (c) picHeight >>= reconPic->m_vChromaShift;
            pixel *pixY = fpelPlane[c] - marginX + (picHeight - 1) * stride;
            for (int y = 0; y < marginY; y++)
                memcpy(pixY + (y + 1) * stride, pixY, stride * sizeof(pixel));
        }
    }

    numWeightedRows = finishedRows;
}
