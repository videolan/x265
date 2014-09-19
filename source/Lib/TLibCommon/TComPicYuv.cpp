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

/** \file     TComPicYuv.cpp
    \brief    picture YUV buffer class
*/

#include "TComPicYuv.h"
#include "common.h"
#include "primitives.h"

using namespace x265;

//! \ingroup TLibCommon
//! \{

TComPicYuv::TComPicYuv()
{
    m_picBuf[0] = NULL; // Buffer (including margin)
    m_picBuf[1] = NULL;
    m_picBuf[2] = NULL;

    m_picOrg[0] = NULL;  // m_apiPicBufY + m_iMarginLuma*getStride() + m_iMarginLuma
    m_picOrg[1] = NULL;
    m_picOrg[2] = NULL;

    m_cuOffsetY = NULL;
    m_cuOffsetC = NULL;
    m_buOffsetY = NULL;
    m_buOffsetC = NULL;
}

bool TComPicYuv::create(int picWidth, int picHeight, int picCsp, uint32_t maxCUSize, uint32_t maxFullDepth)
{
    m_picWidth  = picWidth;
    m_picHeight = picHeight;
    m_hChromaShift = CHROMA_H_SHIFT(picCsp);
    m_vChromaShift = CHROMA_V_SHIFT(picCsp);
    m_picCsp = picCsp;

    // --> After config finished!
    m_cuSize = maxCUSize;

    m_numCuInWidth = (m_picWidth + m_cuSize - 1)  / m_cuSize;
    m_numCuInHeight = (m_picHeight + m_cuSize - 1) / m_cuSize;

    m_lumaMarginX = g_maxCUSize + 32; // search margin and 8-tap filter half-length, padded for 32-byte alignment
    m_lumaMarginY = g_maxCUSize + 16; // margin for 8-tap filter and infinite padding
    m_stride = (m_numCuInWidth * g_maxCUSize) + (m_lumaMarginX << 1);

    m_chromaMarginX = m_lumaMarginX;    // keep 16-byte alignment for chroma CTUs
    m_chromaMarginY = m_lumaMarginY >> m_vChromaShift;

    m_strideC = ((m_numCuInWidth * g_maxCUSize) >> m_hChromaShift) + (m_chromaMarginX * 2);
    int maxHeight = m_numCuInHeight * g_maxCUSize;
    uint32_t numPartitions = 1 << maxFullDepth * 2;

    CHECKED_MALLOC(m_picBuf[0], pixel, m_stride * (maxHeight + (m_lumaMarginY * 2)));
    CHECKED_MALLOC(m_picBuf[1], pixel, m_strideC * ((maxHeight >> m_vChromaShift) + (m_chromaMarginY * 2)));
    CHECKED_MALLOC(m_picBuf[2], pixel, m_strideC * ((maxHeight >> m_vChromaShift) + (m_chromaMarginY * 2)));

    m_picOrg[0] = m_picBuf[0] + m_lumaMarginY   * getStride()  + m_lumaMarginX;
    m_picOrg[1] = m_picBuf[1] + m_chromaMarginY * getCStride() + m_chromaMarginX;
    m_picOrg[2] = m_picBuf[2] + m_chromaMarginY * getCStride() + m_chromaMarginX;

    /* TODO: these four buffers are the same for every TComPicYuv in the encoder */
    CHECKED_MALLOC(m_cuOffsetY, int, m_numCuInWidth * m_numCuInHeight);
    CHECKED_MALLOC(m_cuOffsetC, int, m_numCuInWidth * m_numCuInHeight);
    for (int cuRow = 0; cuRow < m_numCuInHeight; cuRow++)
    {
        for (int cuCol = 0; cuCol < m_numCuInWidth; cuCol++)
        {
            m_cuOffsetY[cuRow * m_numCuInWidth + cuCol] = getStride() * cuRow * m_cuSize + cuCol * m_cuSize;
            m_cuOffsetC[cuRow * m_numCuInWidth + cuCol] = getCStride() * cuRow * (m_cuSize >> m_vChromaShift) + cuCol * (m_cuSize >> m_hChromaShift);
        }
    }

    CHECKED_MALLOC(m_buOffsetY, int, (size_t)numPartitions);
    CHECKED_MALLOC(m_buOffsetC, int, (size_t)numPartitions);
    for (uint32_t idx = 0; idx < numPartitions; ++idx)
    {
        int x = g_zscanToPelX[idx];
        int y = g_zscanToPelY[idx];
        m_buOffsetY[idx] = getStride() * y + x;
        m_buOffsetC[idx] = getCStride() * (y >> m_vChromaShift) + (x >> m_hChromaShift);
    }

    return true;

fail:
    return false;
}

void TComPicYuv::destroy()
{
    X265_FREE(m_picBuf[0]);
    X265_FREE(m_picBuf[1]);
    X265_FREE(m_picBuf[2]);
    X265_FREE(m_cuOffsetY);
    X265_FREE(m_cuOffsetC);
    X265_FREE(m_buOffsetY);
    X265_FREE(m_buOffsetC);
}

uint32_t TComPicYuv::getCUHeight(int rowNum)
{
    uint32_t height;

    if (rowNum == m_numCuInHeight - 1)
        height = ((getHeight() % g_maxCUSize) ? (getHeight() % g_maxCUSize) : g_maxCUSize);
    else
        height = g_maxCUSize;
    return height;
}

/* Copy pixels from an x265_picture into internal TComPicYuv instance.
 * Shift pixels as necessary, mask off bits above X265_DEPTH for safety. */
void TComPicYuv::copyFromPicture(const x265_picture& pic, int padx, int pady)
{
    /* m_picWidth is the width that is being encoded, padx indicates how many
     * of those pixels are padding to reach multiple of MinCU(4) size.
     *
     * Internally, we need to extend rows out to a multiple of 16 for lowres
     * downscale and other operations. But those padding pixels are never
     * encoded.
     *
     * The same applies to m_picHeight and pady */

    /* width and height - without padsize (input picture raw width and height) */
    int width = m_picWidth - padx;
    int height = m_picHeight - pady;

    /* internal pad to multiple of 16x16 blocks */
    uint8_t rem = width & 15;

    padx = rem ? 16 - rem : padx;
    rem = height & 15;
    pady = rem ? 16 - rem : pady;

    /* add one more row and col of pad for downscale interpolation, fixes
     * warnings from valgrind about using uninitialized pixels */
    padx++;
    pady++;

    if (pic.bitDepth < X265_DEPTH)
    {
        pixel *yPixel = getLumaAddr();
        pixel *uPixel = getCbAddr();
        pixel *vPixel = getCrAddr();

        uint8_t *yChar = (uint8_t*)pic.planes[0];
        uint8_t *uChar = (uint8_t*)pic.planes[1];
        uint8_t *vChar = (uint8_t*)pic.planes[2];
        int shift = X265_MAX(0, X265_DEPTH - pic.bitDepth);

        primitives.planecopy_cp(yChar, pic.stride[0] / sizeof(*yChar), yPixel, getStride(), width, height, shift);
        primitives.planecopy_cp(uChar, pic.stride[1] / sizeof(*uChar), uPixel, getCStride(), width >> m_hChromaShift, height >> m_vChromaShift, shift);
        primitives.planecopy_cp(vChar, pic.stride[2] / sizeof(*vChar), vPixel, getCStride(), width >> m_hChromaShift, height >> m_vChromaShift, shift);
    }
    else if (pic.bitDepth == 8)
    {
        pixel *yPixel = getLumaAddr();
        pixel *uPixel = getCbAddr();
        pixel *vPixel = getCrAddr();

        uint8_t *yChar = (uint8_t*)pic.planes[0];
        uint8_t *uChar = (uint8_t*)pic.planes[1];
        uint8_t *vChar = (uint8_t*)pic.planes[2];

        for (int r = 0; r < height; r++)
        {
            for (int c = 0; c < width; c++)
            {
                yPixel[c] = (pixel)yChar[c];
            }

            yPixel += getStride();
            yChar += pic.stride[0] / sizeof(*yChar);
        }

        for (int r = 0; r < height >> m_vChromaShift; r++)
        {
            for (int c = 0; c < width >> m_hChromaShift; c++)
            {
                uPixel[c] = (pixel)uChar[c];
                vPixel[c] = (pixel)vChar[c];
            }

            uPixel += getCStride();
            vPixel += getCStride();
            uChar += pic.stride[1] / sizeof(*uChar);
            vChar += pic.stride[2] / sizeof(*vChar);
        }
    }
    else /* pic.bitDepth > 8 */
    {
        pixel *yPixel = getLumaAddr();
        pixel *uPixel = getCbAddr();
        pixel *vPixel = getCrAddr();

        uint16_t *yShort = (uint16_t*)pic.planes[0];
        uint16_t *uShort = (uint16_t*)pic.planes[1];
        uint16_t *vShort = (uint16_t*)pic.planes[2];

        /* defensive programming, mask off bits that are supposed to be zero */
        uint16_t mask = (1 << X265_DEPTH) - 1;
        int shift = X265_MAX(0, pic.bitDepth - X265_DEPTH);

        /* shift and mask pixels to final size */

        primitives.planecopy_sp(yShort, pic.stride[0] / sizeof(*yShort), yPixel, getStride(), width, height, shift, mask);
        primitives.planecopy_sp(uShort, pic.stride[1] / sizeof(*uShort), uPixel, getCStride(), width >> m_hChromaShift, height >> m_vChromaShift, shift, mask);
        primitives.planecopy_sp(vShort, pic.stride[2] / sizeof(*vShort), vPixel, getCStride(), width >> m_hChromaShift, height >> m_vChromaShift, shift, mask);
    }

    /* extend the right edge if width was not multiple of the minimum CU size */
    if (padx)
    {
        pixel *Y = getLumaAddr();
        pixel *U = getCbAddr();
        pixel *V = getCrAddr();

        for (int r = 0; r < height; r++)
        {
            for (int x = 0; x < padx; x++)
            {
                Y[width + x] = Y[width - 1];
            }

            Y += getStride();
        }

        for (int r = 0; r < height >> m_vChromaShift; r++)
        {
            for (int x = 0; x < padx >> m_hChromaShift; x++)
            {
                U[(width >> m_hChromaShift) + x] = U[(width >> m_hChromaShift) - 1];
                V[(width >> m_hChromaShift) + x] = V[(width >> m_hChromaShift) - 1];
            }

            U += getCStride();
            V += getCStride();
        }
    }

    /* extend the bottom if height was not multiple of the minimum CU size */
    if (pady)
    {
        pixel *Y = getLumaAddr() + (height - 1) * getStride();
        pixel *U = getCbAddr() + ((height >> m_vChromaShift) - 1) * getCStride();
        pixel *V = getCrAddr() + ((height >> m_vChromaShift) - 1) * getCStride();

        for (uint32_t i = 1; i <= pady; i++)
        {
            memcpy(Y + i * getStride(), Y, (width + padx) * sizeof(pixel));
        }

        for (uint32_t j = 1; j <= pady >> m_vChromaShift; j++)
        {
            memcpy(U + j * getCStride(), U, ((width + padx) >> m_hChromaShift) * sizeof(pixel));
            memcpy(V + j * getCStride(), V, ((width + padx) >> m_hChromaShift) * sizeof(pixel));
        }
    }
}
