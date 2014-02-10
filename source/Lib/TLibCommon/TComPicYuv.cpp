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
#include "primitives.h"

#include <cstdlib>
#include <assert.h>
#include <memory.h>
#include <stdint.h>

using namespace x265;

//! \ingroup TLibCommon
//! \{

TComPicYuv::TComPicYuv()
{
    m_picBufY = NULL; // Buffer (including margin)
    m_picBufU = NULL;
    m_picBufV = NULL;

    m_picOrgY = NULL;  // m_apiPicBufY + m_iMarginLuma*getStride() + m_iMarginLuma
    m_picOrgU = NULL;
    m_picOrgV = NULL;
}

TComPicYuv::~TComPicYuv()
{
}

bool TComPicYuv::create(int picWidth, int picHeight, int picCsp, uint32_t maxCUWidth, uint32_t maxCUHeight, uint32_t maxCUDepth)
{
    m_picWidth  = picWidth;
    m_picHeight = picHeight;
    m_hChromaShift = CHROMA_H_SHIFT(picCsp);
    m_vChromaShift = CHROMA_V_SHIFT(picCsp);
    m_picCsp = picCsp;

    // --> After config finished!
    m_cuWidth  = maxCUWidth;
    m_cuHeight = maxCUHeight;

    m_numCuInWidth = (m_picWidth + m_cuWidth - 1)  / m_cuWidth;
    m_numCuInHeight = (m_picHeight + m_cuHeight - 1) / m_cuHeight;

    m_lumaMarginX = g_maxCUWidth  + 32; // search margin and 8-tap filter half-length, padded for 32-byte alignment
    m_lumaMarginY = g_maxCUHeight + 16; // margin for 8-tap filter and infinite padding
    m_stride = (m_numCuInWidth * g_maxCUWidth) + (m_lumaMarginX << 1);

    m_chromaMarginX = m_lumaMarginX;    // keep 16-byte alignment for chroma CTUs
    m_chromaMarginY = m_lumaMarginY >> m_vChromaShift;

    m_strideC = ((m_numCuInWidth * g_maxCUWidth) >> m_hChromaShift) + (m_chromaMarginX * 2);
    int maxHeight = m_numCuInHeight * g_maxCUHeight;

    CHECKED_MALLOC(m_picBufY, Pel, m_stride * (maxHeight + (m_lumaMarginY * 2)));
    CHECKED_MALLOC(m_picBufU, Pel, m_strideC * ((maxHeight >> m_vChromaShift) + (m_chromaMarginY * 2)));
    CHECKED_MALLOC(m_picBufV, Pel, m_strideC * ((maxHeight >> m_vChromaShift) + (m_chromaMarginY * 2)));

    m_picOrgY = m_picBufY + m_lumaMarginY   * getStride()  + m_lumaMarginX;
    m_picOrgU = m_picBufU + m_chromaMarginY * getCStride() + m_chromaMarginX;
    m_picOrgV = m_picBufV + m_chromaMarginY * getCStride() + m_chromaMarginX;

    /* TODO: these four buffers are the same for every TComPicYuv in the encoder */
    CHECKED_MALLOC(m_cuOffsetY, int, m_numCuInWidth * m_numCuInHeight);
    CHECKED_MALLOC(m_cuOffsetC, int, m_numCuInWidth * m_numCuInHeight);
    for (int cuRow = 0; cuRow < m_numCuInHeight; cuRow++)
    {
        for (int cuCol = 0; cuCol < m_numCuInWidth; cuCol++)
        {
            m_cuOffsetY[cuRow * m_numCuInWidth + cuCol] = getStride() * cuRow * m_cuHeight + cuCol * m_cuWidth;
            m_cuOffsetC[cuRow * m_numCuInWidth + cuCol] = getCStride() * cuRow * (m_cuHeight >> m_vChromaShift) + cuCol * (m_cuWidth >> m_hChromaShift);
        }
    }

    CHECKED_MALLOC(m_buOffsetY, int, (size_t)1 << (2 * maxCUDepth));
    CHECKED_MALLOC(m_buOffsetC, int, (size_t)1 << (2 * maxCUDepth));
    for (int buRow = 0; buRow < (1 << maxCUDepth); buRow++)
    {
        for (int buCol = 0; buCol < (1 << maxCUDepth); buCol++)
        {
            m_buOffsetY[(buRow << maxCUDepth) + buCol] = getStride() * buRow * (maxCUHeight >> maxCUDepth) + buCol * (maxCUWidth  >> maxCUDepth);
            m_buOffsetC[(buRow << maxCUDepth) + buCol] = getCStride() * buRow * ((maxCUHeight >> m_vChromaShift) >> maxCUDepth) + buCol * ((maxCUWidth >> m_hChromaShift) >> maxCUDepth);
        }
    }
    return true;

fail:
    return false;
}

void TComPicYuv::destroy()
{
    X265_FREE(m_picBufY);
    X265_FREE(m_picBufU);
    X265_FREE(m_picBufV);
    X265_FREE(m_cuOffsetY);
    X265_FREE(m_cuOffsetC);
    X265_FREE(m_buOffsetY);
    X265_FREE(m_buOffsetC);
}

uint32_t TComPicYuv::getCUHeight(int rowNum)
{
    uint32_t height;

    if (rowNum == m_numCuInHeight - 1)
        height = ((getHeight() % g_maxCUHeight) ? (getHeight() % g_maxCUHeight) : g_maxCUHeight);
    else
        height = g_maxCUHeight;
    return height;
}

/* Copy pixels from an input picture (C structure) into internal TComPicYuv instance
 * Upscale pixels from 8bits to 16 bits when required, but do not modify
 * pixels. */
void TComPicYuv::copyFromPicture(const x265_picture& pic, int32_t *pad)
{
    Pel *Y = getLumaAddr();
    Pel *U = getCbAddr();
    Pel *V = getCrAddr();

    // m_picWidth is the width that is being encoded, padx indicates how many
    // of those pixels are padding to reach multiple of MinCU(4) size.
    //
    // Internally, we need to extend rows out to a multiple of 16 for lowres
    // downscale and other operations. But those padding pixels are never
    // encoded.
    //
    // The same applies to m_picHeight and pady

    int padx = pad[0];
    int pady = pad[1];

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
    padx++; pady++;

#if HIGH_BIT_DEPTH
    if (pic.bitDepth > 8)
    {
        uint16_t *y = (uint16_t*)pic.planes[0];
        uint16_t *u = (uint16_t*)pic.planes[1];
        uint16_t *v = (uint16_t*)pic.planes[2];

        // Manually copy pixels to up-size them
        for (int r = 0; r < height; r++)
        {
            for (int c = 0; c < width; c++)
            {
                Y[c] = (Pel)y[c];
            }

            for (int x = 0; x < padx; x++)
            {
                Y[width + x] = Y[width - 1];
            }

            Y += getStride();
            y += pic.stride[0];
        }

        for (int r = 0; r < height >> m_vChromaShift; r++)
        {
            for (int c = 0; c < width >> m_hChromaShift; c++)
            {
                U[c] = (Pel)u[c];
                V[c] = (Pel)v[c];
            }

            for (int x = 0; x < padx >> m_hChromaShift; x++)
            {
                U[(width >> m_hChromaShift) + x] = U[(width >> m_hChromaShift) - 1];
                V[(width >> m_hChromaShift) + x] = V[(width >> m_hChromaShift) - 1];
            }

            U += getCStride();
            V += getCStride();
            u += pic.stride[1];
            v += pic.stride[2];
        }

        /* extend the bottom if height is not multiple of the minimum CU size */
        if (pady)
        {
            Y = getLumaAddr() + (height - 1) * getStride();
            U = getCbAddr() + ((height >> m_vChromaShift) - 1) * getCStride();
            V = getCrAddr() + ((height >> m_vChromaShift) - 1) * getCStride();

            for (uint32_t i = 1; i <= pady; i++)
            {
                memcpy(Y + i * getStride(), Y, (width + padx) * sizeof(Pel));
            }

            for (uint32_t j = 1; j <= pady >> m_vChromaShift; j++)
            {
                memcpy(U + j * getCStride(), U, ((width + padx) >> m_hChromaShift) * sizeof(Pel));
                memcpy(V + j * getCStride(), V, ((width + padx) >> m_hChromaShift) * sizeof(Pel));
            }
        }
    }
    else
    {
        uint8_t *y = (uint8_t*)pic.planes[0];
        uint8_t *u = (uint8_t*)pic.planes[1];
        uint8_t *v = (uint8_t*)pic.planes[2];

        // Manually copy pixels to up-size them
        for (int r = 0; r < height; r++)
        {
            for (int c = 0; c < width; c++)
            {
                Y[c] = (Pel)y[c];
            }

            for (int x = 0; x < padx; x++)
            {
                Y[width + x] = Y[width - 1];
            }

            Y += getStride();
            y += pic.stride[0];
        }

        for (int r = 0; r < height >> m_vChromaShift; r++)
        {
            for (int c = 0; c < width >> m_hChromaShift; c++)
            {
                U[c] = (Pel)u[c];
                V[c] = (Pel)v[c];
            }

            for (int x = 0; x < padx >> m_hChromaShift; x++)
            {
                U[(width >> m_hChromaShift) + x] = U[(width >> m_hChromaShift) - 1];
                V[(width >> m_hChromaShift) + x] = V[(width >> m_hChromaShift) - 1];
            }

            U += getCStride();
            V += getCStride();
            u += pic.stride[1];
            v += pic.stride[2];
        }

        /* extend the bottom if height is not multiple of the minimum CU size */
        if (pady)
        {
            Y = getLumaAddr() + (height - 1) * getStride();
            U = getCbAddr() + ((height >> m_vChromaShift) - 1) * getCStride();
            V = getCrAddr() + ((height >> m_vChromaShift) - 1) * getCStride();

            for (uint32_t i = 1; i <= pady; i++)
            {
                memcpy(Y + i * getStride(), Y, (width + padx) * sizeof(Pel));
            }

            for (uint32_t j = 1; j <= pady >> m_vChromaShift; j++)
            {
                memcpy(U + j * getCStride(), U, ((width + padx) >> m_hChromaShift) * sizeof(Pel));
                memcpy(V + j * getCStride(), V, ((width + padx) >> m_hChromaShift) * sizeof(Pel));
            }
        }
    }
#else // if HIGH_BIT_DEPTH
    uint8_t *y = (uint8_t*)pic.planes[0];
    uint8_t *u = (uint8_t*)pic.planes[1];
    uint8_t *v = (uint8_t*)pic.planes[2];

    for (int r = 0; r < height; r++)
    {
        memcpy(Y, y, width);

        /* extend the right if width is not multiple of the minimum CU size */
        if (padx)
            ::memset(Y + width, Y[width - 1], padx);

        Y += getStride();
        y += pic.stride[0];
    }

    for (int r = 0; r < height >> m_vChromaShift; r++)
    {
        memcpy(U, u, width >> m_hChromaShift);
        memcpy(V, v, width >> m_hChromaShift);

        /* extend the right if width is not multiple of the minimum CU size */
        if (padx)
        {
            ::memset(U + (width >> m_hChromaShift), U[(width >> m_hChromaShift) - 1], padx >> m_hChromaShift);
            ::memset(V + (width >> m_hChromaShift), V[(width >> m_hChromaShift) - 1], padx >> m_hChromaShift);
        }

        U += getCStride();
        V += getCStride();
        u += pic.stride[1];
        v += pic.stride[2];
    }

    /* extend the bottom if height is not multiple of the minimum CU size */
    if (pady)
    {
        Y = getLumaAddr() + (height - 1) * getStride();
        U = getCbAddr() + ((height >> m_vChromaShift) - 1) * getCStride();
        V = getCrAddr() + ((height >> m_vChromaShift) - 1) * getCStride();

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
#endif // if HIGH_BIT_DEPTH
}
