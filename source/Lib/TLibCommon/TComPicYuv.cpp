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
}

TComPicYuv::~TComPicYuv()
{}

void TComPicYuv::create(int picWidth, int picHeight, UInt maxCUWidth, UInt maxCUHeight, UInt maxCUDepth)
{
    m_picWidth  = picWidth;
    m_picHeight = picHeight;

    // --> After config finished!
    m_cuWidth  = maxCUWidth;
    m_cuHeight = maxCUHeight;

    int numCuInWidth  = (m_picWidth + m_cuWidth - 1)  / m_cuWidth;
    int numCuInHeight = (m_picHeight + m_cuHeight - 1) / m_cuHeight;

    m_numCuInWidth = numCuInWidth;
    m_numCuInHeight = numCuInHeight;

    m_lumaMarginX = g_maxCUWidth  + 16; // for 16-byte alignment
    m_lumaMarginY = g_maxCUHeight + 16; // margin for 8-tap filter and infinite padding
    m_stride = m_picWidth + (m_lumaMarginX << 1);

    m_chromaMarginX = m_lumaMarginX;       // keep 16-byte alignment for chroma CTUs
    m_chromaMarginY = m_lumaMarginY >> 1;
    m_strideC = (m_picWidth >> 1) + (m_chromaMarginX << 1);

    m_picBufY = (Pel*)X265_MALLOC(Pel, (m_picWidth + (m_lumaMarginX << 1)) * (m_picHeight + (m_lumaMarginY << 1)));
    m_picBufU = (Pel*)X265_MALLOC(Pel, ((m_picWidth >> 1) + (m_chromaMarginX << 1)) * ((m_picHeight >> 1) + (m_chromaMarginY << 1)));
    m_picBufV = (Pel*)X265_MALLOC(Pel, ((m_picWidth >> 1) + (m_chromaMarginX << 1)) * ((m_picHeight >> 1) + (m_chromaMarginY << 1)));

    m_picOrgY = m_picBufY + m_lumaMarginY   * getStride()  + m_lumaMarginX;
    m_picOrgU = m_picBufU + m_chromaMarginY * getCStride() + m_chromaMarginX;
    m_picOrgV = m_picBufV + m_chromaMarginY * getCStride() + m_chromaMarginX;

    m_cuOffsetY = new int[numCuInWidth * numCuInHeight];
    m_cuOffsetC = new int[numCuInWidth * numCuInHeight];
    for (int cuRow = 0; cuRow < numCuInHeight; cuRow++)
    {
        for (int cuCol = 0; cuCol < numCuInWidth; cuCol++)
        {
            m_cuOffsetY[cuRow * numCuInWidth + cuCol] = getStride() * cuRow * m_cuHeight + cuCol * m_cuWidth;
            m_cuOffsetC[cuRow * numCuInWidth + cuCol] = getCStride() * cuRow * (m_cuHeight / 2) + cuCol * (m_cuWidth / 2);
        }
    }

    m_buOffsetY = new int[(size_t)1 << (2 * maxCUDepth)];
    m_buOffsetC = new int[(size_t)1 << (2 * maxCUDepth)];
    for (int buRow = 0; buRow < (1 << maxCUDepth); buRow++)
    {
        for (int buCol = 0; buCol < (1 << maxCUDepth); buCol++)
        {
            m_buOffsetY[(buRow << maxCUDepth) + buCol] = getStride() * buRow * (maxCUHeight >> maxCUDepth) + buCol * (maxCUWidth  >> maxCUDepth);
            m_buOffsetC[(buRow << maxCUDepth) + buCol] = getCStride() * buRow * (maxCUHeight / 2 >> maxCUDepth) + buCol * (maxCUWidth / 2 >> maxCUDepth);
        }
    }
}

void TComPicYuv::destroy()
{
    m_picOrgY = NULL;
    m_picOrgU = NULL;
    m_picOrgV = NULL;

    if (m_picBufY) { X265_FREE(m_picBufY); m_picBufY = NULL; }
    if (m_picBufU) { X265_FREE(m_picBufU); m_picBufU = NULL; }
    if (m_picBufV) { X265_FREE(m_picBufV); m_picBufV = NULL; }

    delete[] m_cuOffsetY;
    delete[] m_cuOffsetC;
    delete[] m_buOffsetY;
    delete[] m_buOffsetC;

    while (m_refList)
    {
        MotionReference *mref = m_refList->m_next;
        delete m_refList;
        m_refList = mref;
    }
}

void  TComPicYuv::clearReferences()
{
    while (m_refList)
    {
        MotionReference *mref = m_refList->m_next;
        delete m_refList;
        m_refList = mref;
    }
}

void TComPicYuv::createLuma(int picWidth, int picHeight, UInt maxCUWidth, UInt maxCUHeight, UInt maxCUDepth)
{
    m_picWidth  = picWidth;
    m_picHeight = picHeight;

    m_cuWidth  = maxCUWidth;
    m_cuHeight = maxCUHeight;

    int numCuInWidth  = m_picWidth  / m_cuWidth  + (m_picWidth  % m_cuWidth  != 0);
    int numCuInHeight = m_picHeight / m_cuHeight + (m_picHeight % m_cuHeight != 0);

    m_lumaMarginX = g_maxCUWidth  + 16; // for 16-byte alignment
    m_lumaMarginY = g_maxCUHeight + 16; // margin for 8-tap filter and infinite padding

    m_picBufY = (Pel*)X265_MALLOC(Pel, (m_picWidth + (m_lumaMarginX << 1)) * (m_picHeight + (m_lumaMarginY << 1)));
    m_picOrgY = m_picBufY + m_lumaMarginY * getStride() + m_lumaMarginX;

    m_cuOffsetY = new int[numCuInWidth * numCuInHeight];
    m_cuOffsetC = NULL;
    for (int cuRow = 0; cuRow < numCuInHeight; cuRow++)
    {
        for (int cuCol = 0; cuCol < numCuInWidth; cuCol++)
        {
            m_cuOffsetY[cuRow * numCuInWidth + cuCol] = getStride() * cuRow * m_cuHeight + cuCol * m_cuWidth;
        }
    }

    m_buOffsetY = new int[(size_t)1 << (2 * maxCUDepth)];
    m_buOffsetC = NULL;
    for (int buRow = 0; buRow < (1 << maxCUDepth); buRow++)
    {
        for (int buCol = 0; buCol < (1 << maxCUDepth); buCol++)
        {
            m_buOffsetY[(buRow << maxCUDepth) + buCol] = getStride() * buRow * (maxCUHeight >> maxCUDepth) + buCol * (maxCUWidth  >> maxCUDepth);
        }
    }
}

void TComPicYuv::destroyLuma()
{
    m_picOrgY = NULL;

    if (m_picBufY) { X265_FREE(m_picBufY); m_picBufY = NULL; }

    delete[] m_cuOffsetY;
    delete[] m_buOffsetY;
}

void  TComPicYuv::copyToPic(TComPicYuv* destPicYuv)
{
    assert(m_picWidth  == destPicYuv->getWidth());
    assert(m_picHeight == destPicYuv->getHeight());

    ::memcpy(destPicYuv->getBufY(), m_picBufY, sizeof(Pel) * (m_picWidth + (m_lumaMarginX << 1)) * (m_picHeight + (m_lumaMarginY << 1)));
    ::memcpy(destPicYuv->getBufU(), m_picBufU, sizeof(Pel) * ((m_picWidth >> 1) + (m_chromaMarginX << 1)) * ((m_picHeight >> 1) + (m_chromaMarginY << 1)));
    ::memcpy(destPicYuv->getBufV(), m_picBufV, sizeof(Pel) * ((m_picWidth >> 1) + (m_chromaMarginX << 1)) * ((m_picHeight >> 1) + (m_chromaMarginY << 1)));
}

void  TComPicYuv::copyToPicLuma(TComPicYuv* destPicYuv)
{
    assert(m_picWidth  == destPicYuv->getWidth());
    assert(m_picHeight == destPicYuv->getHeight());

    ::memcpy(destPicYuv->getBufY(), m_picBufY, sizeof(Pel) * (m_picWidth + (m_lumaMarginX << 1)) * (m_picHeight + (m_lumaMarginY << 1)));
}

void  TComPicYuv::copyToPicCb(TComPicYuv* destPicYuv)
{
    assert(m_picWidth  == destPicYuv->getWidth());
    assert(m_picHeight == destPicYuv->getHeight());

    ::memcpy(destPicYuv->getBufU(), m_picBufU, sizeof(Pel) * ((m_picWidth >> 1) + (m_chromaMarginX << 1)) * ((m_picHeight >> 1) + (m_chromaMarginY << 1)));
}

void  TComPicYuv::copyToPicCr(TComPicYuv* destPicYuv)
{
    assert(m_picWidth  == destPicYuv->getWidth());
    assert(m_picHeight == destPicYuv->getHeight());

    ::memcpy(destPicYuv->getBufV(), m_picBufV, sizeof(Pel) * ((m_picWidth >> 1) + (m_chromaMarginX << 1)) * ((m_picHeight >> 1) + (m_chromaMarginY << 1)));
}

MotionReference* TComPicYuv::generateMotionReference(wpScalingParam *w)
{
    /* HPEL generation requires luma integer plane to already be extended */
    // NOTE: We extend border every CURow, so I remove code here

    MotionReference *mref;

    for (mref = m_refList; mref != NULL; mref = mref->m_next)
    {
        if (w)
        {
            if (mref->matchesWeight(*w))
                return mref;
        }
        else if (mref->isWeighted == false)
            return mref;
    }

    mref = new MotionReference();
    mref->init(this, w);
    mref->m_next = m_refList;
    m_refList = mref;
    return mref;
}

void TComPicYuv::xExtendPicCompBorder(Pel* recon, int stride, int width, int height, int iMarginX, int iMarginY)
{
    int x, y;

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

void TComPicYuv::dump(char* pFileName, bool bAdd)
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

    int shift = X265_DEPTH - 8;
    int offset = (shift > 0) ? (1 << (shift - 1)) : 0;

    int   x, y;
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

    shift = X265_DEPTH - 8;
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
void TComPicYuv::copyFromPicture(const x265_picture_t& pic, int *pad)
{
    Pel *Y = getLumaAddr();
    Pel *U = getCbAddr();
    Pel *V = getCrAddr();

    uint8_t *y = (uint8_t*)pic.planes[0];
    uint8_t *u = (uint8_t*)pic.planes[1];
    uint8_t *v = (uint8_t*)pic.planes[2];

    int padx = pad[0];
    int pady = pad[1];

#if HIGH_BIT_DEPTH
    if (sizeof(Pel) * 8 > pic.bitDepth)
    {
        assert(pic.bitDepth == 8);

        /* width and height - without padsize */
        int width = m_picWidth - padx;
        int height = m_picHeight - pady;

        // Manually copy pixels to up-size them
        for (int r = 0; r < height; r++)
        {
            for (int c = 0; c < width; c++)
            {
                Y[c] = (Pel)y[c];
            }

            Y += getStride();
            y += pic.stride[0];
        }

        for (int r = 0; r < height >> 1; r++)
        {
            for (int c = 0; c < width >> 1; c++)
            {
                U[c] = (Pel)u[c];
                V[c] = (Pel)v[c];
            }

            U += getCStride();
            V += getCStride();
            u += pic.stride[1];
            v += pic.stride[2];
        }

        /* Extend the right if width is not multiple of minimum CU size */

        if (padx)
        {
            Y = getLumaAddr();
            U = getCbAddr();
            V = getCrAddr();

            for (int r = 0; r < height; r++)
            {
                for (int x = 0; x < padx; x++)
                    Y[width + x] = Y[width - 1];
                Y += getStride();
            }

            for (int r = 0; r < height >> 1; r++)
            {
                for (int x = 0; x < padx >> 1; x++)
                {
                    U[(width >> 1) + x] = U[(width >> 1) - 1];
                    V[(width >> 1) + x] = V[(width >> 1) - 1];
                }
                U += getCStride();
                V += getCStride();
            }
        }

        /* extend the bottom if height is not multiple of the minimum CU size */
        if (pady)
        {
            width = m_picWidth;
            Y = getLumaAddr() + (height - 1) * getStride();
            U = getCbAddr() + ((height >> 1) - 1) * getCStride();
            V = getCrAddr() + ((height >> 1) - 1) * getCStride();

            for (uint32_t i = 1; i <= pady; i++)
                memcpy(Y + i * getStride(), Y, width * sizeof(Pel));

            for (uint32_t j = 1; j <= pady >> 1; j++)
            {
                memcpy(U + j * getCStride(), U, (width >> 1) * sizeof(Pel));
                memcpy(V + j * getCStride(), V, (width >> 1) * sizeof(Pel));
            }
        }
    }
    else
#endif // if HIGH_BIT_DEPTH
    {

        /* width and height - without padsize */
        int width = (m_picWidth * (pic.bitDepth > 8 ? 2 : 1)) - padx;
        int height = m_picHeight - pady;

        // copy pixels by row into encoder's buffer
        for (int r = 0; r < height; r++)
        {
            memcpy(Y, y, width);

            /* extend the right if width is not multiple of the minimum CU size */
            if (padx)
                ::memset(Y + width, Y[width - 1], padx);

            Y += getStride();
            y += pic.stride[0];

        }

        for (int r = 0; r < height >> 1; r++)
        {
            memcpy(U, u, width >> 1);
            memcpy(V, v, width >> 1);

            /* extend the right if width is not multiple of the minimum CU size */
            if (padx)
            {
                ::memset(U + (width >> 1), U[(width >> 1) - 1], padx >> 1);
                ::memset(V + (width >> 1), V[(width >> 1) - 1], padx >> 1);
            }

            U += getCStride();
            V += getCStride();
            u += pic.stride[1];
            v += pic.stride[2];
        }

        /* extend the bottom if height is not multiple of the minimum CU size */
        if (pady)
        {
            width = m_picWidth;
            Y = getLumaAddr() + (height - 1) * getStride();
            U = getCbAddr() + ((height >> 1) - 1) * getCStride();
            V = getCrAddr() + ((height >> 1) - 1) * getCStride();

            for (uint32_t i = 1; i <= pady; i++)
                memcpy(Y + i * getStride(), Y, width * sizeof(pixel));

            for (uint32_t j = 1; j <= pady >> 1; j++)
            {
                memcpy(U + j * getCStride(), U, (width >> 1) * sizeof(pixel));
                memcpy(V + j * getCStride(), V, (width >> 1) * sizeof(pixel));
            }
        }
    }
}
