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
#include "TComPrediction.h"
#include "primitives.h"

#ifdef __APPLE__
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif

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

    m_refList = NULL;

    m_bIsBorderExtended = false;
}

TComPicYuv::~TComPicYuv()
{}

Void TComPicYuv::create(Int iPicWidth, Int iPicHeight, UInt uiMaxCUWidth, UInt uiMaxCUHeight, UInt uiMaxCUDepth)
{
    m_picWidth  = iPicWidth;
    m_picHeight = iPicHeight;

    // --> After config finished!
    m_cuWidth  = uiMaxCUWidth;
    m_cuHeight = uiMaxCUHeight;

    Int numCuInWidth  = m_picWidth  / m_cuWidth  + (m_picWidth  % m_cuWidth  != 0);
    Int numCuInHeight = m_picHeight / m_cuHeight + (m_picHeight % m_cuHeight != 0);

    m_lumaMarginX = g_maxCUWidth  + 16; // for 16-byte alignment
    m_lumaMarginY = g_maxCUHeight + 16; // margin for 8-tap filter and infinite padding

    m_chromaMarginX = m_lumaMarginX;       // keep 16-byte alignment for chroma CTUs
    m_chromaMarginY = m_lumaMarginY >> 1;

    m_picBufY = (Pel*)xMalloc(Pel, (m_picWidth + (m_lumaMarginX << 1)) * (m_picHeight + (m_lumaMarginY << 1)));
    m_picBufU = (Pel*)xMalloc(Pel, ((m_picWidth >> 1) + (m_chromaMarginX << 1)) * ((m_picHeight >> 1) + (m_chromaMarginY << 1)));
    m_picBufV = (Pel*)xMalloc(Pel, ((m_picWidth >> 1) + (m_chromaMarginX << 1)) * ((m_picHeight >> 1) + (m_chromaMarginY << 1)));

    m_picOrgY = m_picBufY + m_lumaMarginY   * getStride()  + m_lumaMarginX;
    m_picOrgU = m_picBufU + m_chromaMarginY * getCStride() + m_chromaMarginX;
    m_picOrgV = m_picBufV + m_chromaMarginY * getCStride() + m_chromaMarginX;

    m_bIsBorderExtended = false;

    m_cuOffsetY = new Int[numCuInWidth * numCuInHeight];
    m_cuOffsetC = new Int[numCuInWidth * numCuInHeight];
    for (Int cuRow = 0; cuRow < numCuInHeight; cuRow++)
    {
        for (Int cuCol = 0; cuCol < numCuInWidth; cuCol++)
        {
            m_cuOffsetY[cuRow * numCuInWidth + cuCol] = getStride() * cuRow * m_cuHeight + cuCol * m_cuWidth;
            m_cuOffsetC[cuRow * numCuInWidth + cuCol] = getCStride() * cuRow * (m_cuHeight / 2) + cuCol * (m_cuWidth / 2);
        }
    }

    m_buOffsetY = new Int[(size_t)1 << (2 * uiMaxCUDepth)];
    m_buOffsetC = new Int[(size_t)1 << (2 * uiMaxCUDepth)];
    for (Int buRow = 0; buRow < (1 << uiMaxCUDepth); buRow++)
    {
        for (Int buCol = 0; buCol < (1 << uiMaxCUDepth); buCol++)
        {
            m_buOffsetY[(buRow << uiMaxCUDepth) + buCol] = getStride() * buRow * (uiMaxCUHeight >> uiMaxCUDepth) + buCol * (uiMaxCUWidth  >> uiMaxCUDepth);
            m_buOffsetC[(buRow << uiMaxCUDepth) + buCol] = getCStride() * buRow * (uiMaxCUHeight / 2 >> uiMaxCUDepth) + buCol * (uiMaxCUWidth / 2 >> uiMaxCUDepth);
        }
    }
}

Void TComPicYuv::destroy()
{
    m_picOrgY = NULL;
    m_picOrgU = NULL;
    m_picOrgV = NULL;

    if (m_picBufY) { xFree(m_picBufY); m_picBufY = NULL; }
    if (m_picBufU) { xFree(m_picBufU); m_picBufU = NULL; }
    if (m_picBufV) { xFree(m_picBufV); m_picBufV = NULL; }

    delete[] m_cuOffsetY;
    delete[] m_cuOffsetC;
    delete[] m_buOffsetY;
    delete[] m_buOffsetC;

    if (m_refList)
        delete m_refList;
}

Void  TComPicYuv::clearReferences()
{
    // TODO: reclaim these into a GOP encoder pool
    if (m_refList)
        delete m_refList;
    m_refList = NULL;
}

Void TComPicYuv::createLuma(Int picWidth, Int picHeight, UInt maxCUWidth, UInt maxCUHeight, UInt maxCUDepth)
{
    m_picWidth  = picWidth;
    m_picHeight = picHeight;

    m_cuWidth  = maxCUWidth;
    m_cuHeight = maxCUHeight;

    Int numCuInWidth  = m_picWidth  / m_cuWidth  + (m_picWidth  % m_cuWidth  != 0);
    Int numCuInHeight = m_picHeight / m_cuHeight + (m_picHeight % m_cuHeight != 0);

    m_lumaMarginX = g_maxCUWidth  + 16; // for 16-byte alignment
    m_lumaMarginY = g_maxCUHeight + 16; // margin for 8-tap filter and infinite padding

    m_picBufY = (Pel*)xMalloc(Pel, (m_picWidth + (m_lumaMarginX << 1)) * (m_picHeight + (m_lumaMarginY << 1)));
    m_picOrgY = m_picBufY + m_lumaMarginY * getStride() + m_lumaMarginX;

    m_cuOffsetY = new Int[numCuInWidth * numCuInHeight];
    m_cuOffsetC = NULL;
    for (Int cuRow = 0; cuRow < numCuInHeight; cuRow++)
    {
        for (Int cuCol = 0; cuCol < numCuInWidth; cuCol++)
        {
            m_cuOffsetY[cuRow * numCuInWidth + cuCol] = getStride() * cuRow * m_cuHeight + cuCol * m_cuWidth;
        }
    }

    m_buOffsetY = new Int[(size_t)1 << (2 * maxCUDepth)];
    m_buOffsetC = NULL;
    for (Int buRow = 0; buRow < (1 << maxCUDepth); buRow++)
    {
        for (Int buCol = 0; buCol < (1 << maxCUDepth); buCol++)
        {
            m_buOffsetY[(buRow << maxCUDepth) + buCol] = getStride() * buRow * (maxCUHeight >> maxCUDepth) + buCol * (maxCUWidth  >> maxCUDepth);
        }
    }
}

Void TComPicYuv::destroyLuma()
{
    m_picOrgY = NULL;

    if (m_picBufY) { xFree(m_picBufY); m_picBufY = NULL; }

    delete[] m_cuOffsetY;
    delete[] m_buOffsetY;
}

Void  TComPicYuv::copyToPic(TComPicYuv* pcPicYuvDst)
{
    assert(m_picWidth  == pcPicYuvDst->getWidth());
    assert(m_picHeight == pcPicYuvDst->getHeight());

    ::memcpy(pcPicYuvDst->getBufY(), m_picBufY, sizeof(Pel) * (m_picWidth + (m_lumaMarginX << 1)) * (m_picHeight + (m_lumaMarginY << 1)));
    ::memcpy(pcPicYuvDst->getBufU(), m_picBufU, sizeof(Pel) * ((m_picWidth >> 1) + (m_chromaMarginX << 1)) * ((m_picHeight >> 1) + (m_chromaMarginY << 1)));
    ::memcpy(pcPicYuvDst->getBufV(), m_picBufV, sizeof(Pel) * ((m_picWidth >> 1) + (m_chromaMarginX << 1)) * ((m_picHeight >> 1) + (m_chromaMarginY << 1)));
}

Void  TComPicYuv::copyToPicLuma(TComPicYuv* pcPicYuvDst)
{
    assert(m_picWidth  == pcPicYuvDst->getWidth());
    assert(m_picHeight == pcPicYuvDst->getHeight());

    ::memcpy(pcPicYuvDst->getBufY(), m_picBufY, sizeof(Pel) * (m_picWidth + (m_lumaMarginX << 1)) * (m_picHeight + (m_lumaMarginY << 1)));
}

Void  TComPicYuv::copyToPicCb(TComPicYuv* pcPicYuvDst)
{
    assert(m_picWidth  == pcPicYuvDst->getWidth());
    assert(m_picHeight == pcPicYuvDst->getHeight());

    ::memcpy(pcPicYuvDst->getBufU(), m_picBufU, sizeof(Pel) * ((m_picWidth >> 1) + (m_chromaMarginX << 1)) * ((m_picHeight >> 1) + (m_chromaMarginY << 1)));
}

Void  TComPicYuv::copyToPicCr(TComPicYuv* pcPicYuvDst)
{
    assert(m_picWidth  == pcPicYuvDst->getWidth());
    assert(m_picHeight == pcPicYuvDst->getHeight());

    ::memcpy(pcPicYuvDst->getBufV(), m_picBufV, sizeof(Pel) * ((m_picWidth >> 1) + (m_chromaMarginX << 1)) * ((m_picHeight >> 1) + (m_chromaMarginY << 1)));
}

Void TComPicYuv::extendPicBorder(x265::ThreadPool *pool)
{
    if (m_bIsBorderExtended)
        return;

    /* HPEL generation requires luma integer plane to already be extended */
    xExtendPicCompBorder(getLumaAddr(), getStride(), getWidth(), getHeight(), m_lumaMarginX, m_lumaMarginY);

    xExtendPicCompBorder(getCbAddr(), getCStride(), getWidth() >> 1, getHeight() >> 1, m_chromaMarginX, m_chromaMarginY);
    xExtendPicCompBorder(getCrAddr(), getCStride(), getWidth() >> 1, getHeight() >> 1, m_chromaMarginX, m_chromaMarginY);

    if (m_refList == NULL)
        m_refList = new x265::MotionReference(this, pool);
    m_refList->generateReferencePlanes();

    m_bIsBorderExtended = true;
}

Void TComPicYuv::xExtendPicCompBorder(Pel* recon, Int stride, Int width, Int height, Int iMarginX, Int iMarginY)
{
    Int   x, y;

    /* TODO: this should become a performance primitive */
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < iMarginX; x++)
        {
            recon[-iMarginX + x] = recon[0];
            recon[width + x] = recon[width - 1];
        }

        recon += stride;
    }

    recon -= (stride + iMarginX);
    for (y = 0; y < iMarginY; y++)
    {
        ::memcpy(recon + (y + 1) * stride, recon, sizeof(Pel) * (width + (iMarginX << 1)));
    }

    recon -= ((height - 1) * stride);
    for (y = 0; y < iMarginY; y++)
    {
        ::memcpy(recon - (y + 1) * stride, recon, sizeof(Pel) * (width + (iMarginX << 1)));
    }
}

Void TComPicYuv::dump(Char* pFileName, Bool bAdd)
{
    FILE* pFile;

    if (!bAdd)
    {
        pFile = fopen(pFileName, "wb");
    }
    else
    {
        pFile = fopen(pFileName, "ab");
    }

    Int shift = g_bitDepthY - 8;
    Int offset = (shift > 0) ? (1 << (shift - 1)) : 0;

    Int   x, y;
    UChar uc;

    Pel* pelY   = getLumaAddr();
    Pel* pelCb  = getCbAddr();
    Pel* pelCr  = getCrAddr();

    for (y = 0; y < m_picHeight; y++)
    {
        for (x = 0; x < m_picWidth; x++)
        {
            uc = (UChar)Clip3<Pel>(0, 255, (pelY[x] + offset) >> shift);

            fwrite(&uc, sizeof(UChar), 1, pFile);
        }

        pelY += getStride();
    }

    shift = g_bitDepthY - 8;
    offset = (shift > 0) ? (1 << (shift - 1)) : 0;

    for (y = 0; y < m_picHeight >> 1; y++)
    {
        for (x = 0; x < m_picWidth >> 1; x++)
        {
            uc = (UChar)Clip3<Pel>(0, 255, (pelCb[x] + offset) >> shift);
            fwrite(&uc, sizeof(UChar), 1, pFile);
        }

        pelCb += getCStride();
    }

    for (y = 0; y < m_picHeight >> 1; y++)
    {
        for (x = 0; x < m_picWidth >> 1; x++)
        {
            uc = (UChar)Clip3<Pel>(0, 255, (pelCr[x] + offset) >> shift);
            fwrite(&uc, sizeof(UChar), 1, pFile);
        }

        pelCr += getCStride();
    }

    fclose(pFile);
}

//! \}

/* Copy pixels from an input picture (C structure) into internal TComPicYuv instance
 * Upscale pixels from 8bits to 16 bits when required, but do not modify pixels.
 * This new routine is GPL
 */
Void TComPicYuv::copyFromPicture(const x265_picture_t& pic)
{
    Pel *Y = getLumaAddr();
    Pel *U = getCbAddr();
    Pel *V = getCrAddr();

    uint8_t *y = (uint8_t*)pic.planes[0];
    uint8_t *u = (uint8_t*)pic.planes[1];
    uint8_t *v = (uint8_t*)pic.planes[2];

#if HIGH_BIT_DEPTH
    if (sizeof(Pel) * 8 > pic.bitDepth)
    {
        assert(pic.bitDepth == 8);

        // Manually copy pixels to up-size them
        for (int r = 0; r < m_picHeight; r++)
        {
            for (int c = 0; c < m_picWidth; c++)
            {
                Y[c] = (Pel)y[c];
            }

            Y += getStride();
            y += pic.stride[0];
        }

        for (int r = 0; r < m_picHeight >> 1; r++)
        {
            for (int c = 0; c < m_picWidth >> 1; c++)
            {
                U[c] = (Pel)u[c];
                V[c] = (Pel)v[c];
            }

            U += getCStride();
            V += getCStride();
            u += pic.stride[1];
            v += pic.stride[2];
        }
    }
    else
#endif // if HIGH_BIT_DEPTH
    {
        int width = m_picWidth * (pic.bitDepth > 8 ? 2 : 1);

        // copy pixels by row into encoder's buffer
        for (int r = 0; r < m_picHeight; r++)
        {
            memcpy(Y, y, width);

            Y += getStride();
            y += pic.stride[0];
        }

        for (int r = 0; r < m_picHeight >> 1; r++)
        {
            memcpy(U, u, width >> 1);
            memcpy(V, v, width >> 1);

            U += getCStride();
            V += getCStride();
            u += pic.stride[1];
            v += pic.stride[2];
        }
    }
}
