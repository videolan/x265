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

#include <cstdlib>
#include <assert.h>
#include <memory.h>
#include "InterpolationFilter.h"
#include "primitives.h"

using namespace x265;

#ifdef __APPLE__
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif

#include "TComPicYuv.h"

//! \ingroup TLibCommon
//! \{

TComPicYuv::TComPicYuv()
{
    m_apiPicBufY      = NULL; // Buffer (including margin)
    m_apiPicBufU      = NULL;
    m_apiPicBufV      = NULL;

    m_piPicOrgY       = NULL;  // m_apiPicBufY + m_iMarginLuma*getStride() + m_iMarginLuma
    m_piPicOrgU       = NULL;
    m_piPicOrgV       = NULL;

    /* Initialize filterBlocks */
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            m_filteredBlockBufY[i][j] = NULL;
            m_filteredBlockBufU[i][j] = NULL;
            m_filteredBlockBufV[i][j] = NULL;

            m_filteredBlockOrgY[i][j] = NULL;
            m_filteredBlockOrgU[i][j] = NULL;
            m_filteredBlockOrgV[i][j] = NULL;
        }
    }

    m_bIsBorderExtended = false;
}

TComPicYuv::~TComPicYuv()
{}

Void TComPicYuv::create(Int iPicWidth, Int iPicHeight, UInt uiMaxCUWidth, UInt uiMaxCUHeight, UInt uiMaxCUDepth)
{
    m_iPicWidth       = iPicWidth;
    m_iPicHeight      = iPicHeight;

    // --> After config finished!
    m_iCuWidth        = uiMaxCUWidth;
    m_iCuHeight       = uiMaxCUHeight;

    Int numCuInWidth  = m_iPicWidth  / m_iCuWidth  + (m_iPicWidth  % m_iCuWidth  != 0);
    Int numCuInHeight = m_iPicHeight / m_iCuHeight + (m_iPicHeight % m_iCuHeight != 0);

    m_iLumaMarginX    = g_uiMaxCUWidth  + 16; // for 16-byte alignment
    m_iLumaMarginY    = g_uiMaxCUHeight + 16; // margin for 8-tap filter and infinite padding

    m_iChromaMarginX  = m_iLumaMarginX >> 1;
    m_iChromaMarginY  = m_iLumaMarginY >> 1;

    m_apiPicBufY      = (Pel*)xMalloc(Pel, (m_iPicWidth       + (m_iLumaMarginX << 1)) * (m_iPicHeight       + (m_iLumaMarginY << 1)));
    m_apiPicBufU      = (Pel*)xMalloc(Pel, ((m_iPicWidth >> 1) + (m_iChromaMarginX << 1)) * ((m_iPicHeight >> 1) + (m_iChromaMarginY << 1)));
    m_apiPicBufV      = (Pel*)xMalloc(Pel, ((m_iPicWidth >> 1) + (m_iChromaMarginX << 1)) * ((m_iPicHeight >> 1) + (m_iChromaMarginY << 1)));

    m_piPicOrgY       = m_apiPicBufY + m_iLumaMarginY   * getStride()  + m_iLumaMarginX;
    m_piPicOrgU       = m_apiPicBufU + m_iChromaMarginY * getCStride() + m_iChromaMarginX;
    m_piPicOrgV       = m_apiPicBufV + m_iChromaMarginY * getCStride() + m_iChromaMarginX;

    m_bIsBorderExtended = false;

    m_cuOffsetY = new Int[numCuInWidth * numCuInHeight];
    m_cuOffsetC = new Int[numCuInWidth * numCuInHeight];
    for (Int cuRow = 0; cuRow < numCuInHeight; cuRow++)
    {
        for (Int cuCol = 0; cuCol < numCuInWidth; cuCol++)
        {
            m_cuOffsetY[cuRow * numCuInWidth + cuCol] = getStride() * cuRow * m_iCuHeight + cuCol * m_iCuWidth;
            m_cuOffsetC[cuRow * numCuInWidth + cuCol] = getCStride() * cuRow * (m_iCuHeight / 2) + cuCol * (m_iCuWidth / 2);
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
    m_piPicOrgY       = NULL;
    m_piPicOrgU       = NULL;
    m_piPicOrgV       = NULL;

    if (m_apiPicBufY) { xFree(m_apiPicBufY);    m_apiPicBufY = NULL; }
    if (m_apiPicBufU) { xFree(m_apiPicBufU);    m_apiPicBufU = NULL; }
    if (m_apiPicBufV) { xFree(m_apiPicBufV);    m_apiPicBufV = NULL; }

    delete[] m_cuOffsetY;
    delete[] m_cuOffsetC;
    delete[] m_buOffsetY;
    delete[] m_buOffsetC;

    /* Delete m_filteredBlocks */
    
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            if (m_filteredBlockBufY[i][j]) { xFree(m_filteredBlockBufY[i][j]);    m_filteredBlockBufY[i][j] = NULL; }
            if (m_filteredBlockBufU[i][j]) { xFree(m_filteredBlockBufU[i][j]);    m_filteredBlockBufU[i][j] = NULL; }
            if (m_filteredBlockBufV[i][j]) { xFree(m_filteredBlockBufV[i][j]);    m_filteredBlockBufV[i][j] = NULL; }

            m_filteredBlockOrgY[i][j]      = NULL;
            m_filteredBlockOrgU[i][j]      = NULL;
            m_filteredBlockOrgV[i][j]      = NULL;
        }
    }
}

Void TComPicYuv::createLuma(Int iPicWidth, Int iPicHeight, UInt uiMaxCUWidth, UInt uiMaxCUHeight, UInt uiMaxCUDepth)
{
    m_iPicWidth       = iPicWidth;
    m_iPicHeight      = iPicHeight;

    // --> After config finished!
    m_iCuWidth        = uiMaxCUWidth;
    m_iCuHeight       = uiMaxCUHeight;

    Int numCuInWidth  = m_iPicWidth  / m_iCuWidth  + (m_iPicWidth  % m_iCuWidth  != 0);
    Int numCuInHeight = m_iPicHeight / m_iCuHeight + (m_iPicHeight % m_iCuHeight != 0);

    m_iLumaMarginX    = g_uiMaxCUWidth  + 16; // for 16-byte alignment
    m_iLumaMarginY    = g_uiMaxCUHeight + 16; // margin for 8-tap filter and infinite padding

    m_apiPicBufY      = (Pel*)xMalloc(Pel, (m_iPicWidth       + (m_iLumaMarginX << 1)) * (m_iPicHeight       + (m_iLumaMarginY << 1)));
    m_piPicOrgY       = m_apiPicBufY + m_iLumaMarginY   * getStride()  + m_iLumaMarginX;

    m_cuOffsetY = new Int[numCuInWidth * numCuInHeight];
    m_cuOffsetC = NULL;
    for (Int cuRow = 0; cuRow < numCuInHeight; cuRow++)
    {
        for (Int cuCol = 0; cuCol < numCuInWidth; cuCol++)
        {
            m_cuOffsetY[cuRow * numCuInWidth + cuCol] = getStride() * cuRow * m_iCuHeight + cuCol * m_iCuWidth;
        }
    }

    m_buOffsetY = new Int[(size_t)1 << (2 * uiMaxCUDepth)];
    m_buOffsetC = NULL;
    for (Int buRow = 0; buRow < (1 << uiMaxCUDepth); buRow++)
    {
        for (Int buCol = 0; buCol < (1 << uiMaxCUDepth); buCol++)
        {
            m_buOffsetY[(buRow << uiMaxCUDepth) + buCol] = getStride() * buRow * (uiMaxCUHeight >> uiMaxCUDepth) + buCol * (uiMaxCUWidth  >> uiMaxCUDepth);
        }
    }
}

Void TComPicYuv::destroyLuma()
{
    m_piPicOrgY       = NULL;

    if (m_apiPicBufY) { xFree(m_apiPicBufY);    m_apiPicBufY = NULL; }

    delete[] m_cuOffsetY;
    delete[] m_buOffsetY;
}

Void  TComPicYuv::copyToPic(TComPicYuv* pcPicYuvDst)
{
    assert(m_iPicWidth  == pcPicYuvDst->getWidth());
    assert(m_iPicHeight == pcPicYuvDst->getHeight());

    ::memcpy(pcPicYuvDst->getBufY(), m_apiPicBufY, sizeof(Pel) * (m_iPicWidth       + (m_iLumaMarginX << 1)) * (m_iPicHeight       + (m_iLumaMarginY << 1)));
    ::memcpy(pcPicYuvDst->getBufU(), m_apiPicBufU, sizeof(Pel) * ((m_iPicWidth >> 1) + (m_iChromaMarginX << 1)) * ((m_iPicHeight >> 1) + (m_iChromaMarginY << 1)));
    ::memcpy(pcPicYuvDst->getBufV(), m_apiPicBufV, sizeof(Pel) * ((m_iPicWidth >> 1) + (m_iChromaMarginX << 1)) * ((m_iPicHeight >> 1) + (m_iChromaMarginY << 1)));
}

Void  TComPicYuv::copyToPicLuma(TComPicYuv* pcPicYuvDst)
{
    assert(m_iPicWidth  == pcPicYuvDst->getWidth());
    assert(m_iPicHeight == pcPicYuvDst->getHeight());

    ::memcpy(pcPicYuvDst->getBufY(), m_apiPicBufY, sizeof(Pel) * (m_iPicWidth       + (m_iLumaMarginX << 1)) * (m_iPicHeight       + (m_iLumaMarginY << 1)));
}

Void  TComPicYuv::copyToPicCb(TComPicYuv* pcPicYuvDst)
{
    assert(m_iPicWidth  == pcPicYuvDst->getWidth());
    assert(m_iPicHeight == pcPicYuvDst->getHeight());

    ::memcpy(pcPicYuvDst->getBufU(), m_apiPicBufU, sizeof(Pel) * ((m_iPicWidth >> 1) + (m_iChromaMarginX << 1)) * ((m_iPicHeight >> 1) + (m_iChromaMarginY << 1)));
}

Void  TComPicYuv::copyToPicCr(TComPicYuv* pcPicYuvDst)
{
    assert(m_iPicWidth  == pcPicYuvDst->getWidth());
    assert(m_iPicHeight == pcPicYuvDst->getHeight());

    ::memcpy(pcPicYuvDst->getBufV(), m_apiPicBufV, sizeof(Pel) * ((m_iPicWidth >> 1) + (m_iChromaMarginX << 1)) * ((m_iPicHeight >> 1) + (m_iChromaMarginY << 1)));
}

Void TComPicYuv::extendPicBorder()
{
    if (m_bIsBorderExtended) return;

    xExtendPicCompBorder(getLumaAddr(), getStride(), getWidth(),     getHeight(),      m_iLumaMarginX,   m_iLumaMarginY);
    xExtendPicCompBorder(getCbAddr(), getCStride(), getWidth() >> 1, getHeight() >> 1, m_iChromaMarginX, m_iChromaMarginY);
    xExtendPicCompBorder(getCrAddr(), getCStride(), getWidth() >> 1, getHeight() >> 1, m_iChromaMarginX, m_iChromaMarginY);

    /* TODO: Here is an excellent place to interpolate HPEL/QPEL planes */
    
    /* Create buffers for Hpel/Qpel Planes */
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            m_filteredBlockBufY[i][j]      = (Pel*)xMalloc(Pel, (m_iPicWidth       + (m_iLumaMarginX << 1)) * (m_iPicHeight       + (m_iLumaMarginY << 1)));
            m_filteredBlockBufU[i][j]      = (Pel*)xMalloc(Pel, ((m_iPicWidth >> 1) + (m_iChromaMarginX << 1)) * ((m_iPicHeight >> 1) + (m_iChromaMarginY << 1)));
            m_filteredBlockBufV[i][j]      = (Pel*)xMalloc(Pel, ((m_iPicWidth >> 1) + (m_iChromaMarginX << 1)) * ((m_iPicHeight >> 1) + (m_iChromaMarginY << 1)));

            m_filteredBlockOrgY[i][j]      = m_filteredBlockBufY[i][j] + m_iLumaMarginY   * getStride()  + m_iLumaMarginX;
            m_filteredBlockOrgU[i][j]      = m_filteredBlockBufU[i][j] + m_iChromaMarginY * getCStride() + m_iChromaMarginX;
            m_filteredBlockOrgV[i][j]      = m_filteredBlockBufV[i][j] + m_iChromaMarginY * getCStride() + m_iChromaMarginX;

        }
    }

    /* Generate H/Q-pel */
    generateHQpel();    //Untested! Currently generate only HPels 

    //Extend border. Output is not matching with HM.
    /*for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            xExtendPicCompBorder(m_filteredBlockOrgY[i][j], getStride(), getWidth(), getHeight(), m_iLumaMarginX, m_iLumaMarginY);
            xExtendPicCompBorder(m_filteredBlockOrgU[i][j], getCStride(), getWidth() >> 1, getHeight() >> 1, m_iChromaMarginX, m_iChromaMarginY);
            xExtendPicCompBorder(m_filteredBlockOrgV[i][j], getCStride(), getWidth() >> 1, getHeight() >> 1, m_iChromaMarginX, m_iChromaMarginY);
        }
    }*/

    m_bIsBorderExtended = true;
}

Void TComPicYuv::xExtendPicCompBorder(Pel* piTxt, Int iStride, Int iWidth, Int iHeight, Int iMarginX, Int iMarginY)
{
    Int   x, y;
    Pel*  pi;

    /* TODO: this should become a performance primitive */
    pi = piTxt;
    for (y = 0; y < iHeight; y++)
    {
        for (x = 0; x < iMarginX; x++)
        {
            pi[-iMarginX + x] = pi[0];
            pi[iWidth + x] = pi[iWidth - 1];
        }

        pi += iStride;
    }

    pi -= (iStride + iMarginX);
    for (y = 0; y < iMarginY; y++)
    {
        ::memcpy(pi + (y + 1) * iStride, pi, sizeof(Pel) * (iWidth + (iMarginX << 1)));
    }

    pi -= ((iHeight - 1) * iStride);
    for (y = 0; y < iMarginY; y++)
    {
        ::memcpy(pi - (y + 1) * iStride, pi, sizeof(Pel) * (iWidth + (iMarginX << 1)));
    }
}

//Void TComPicYuv::generateHQpel()
//{
//    Int width      = m_iPicWidth ; //+ (m_iLumaMarginX << 1)-8;
//    Int height     =  m_iPicHeight ; //+ (m_iLumaMarginY << 1)-8;
//    Int srcStride  =  getStride();
//
//    TShortYUV filteredBlockTmp[4];
//
//    int offsetToLuma = m_iLumaMarginY   * getStride()  + m_iLumaMarginX;
//
//    for (int i = 0; i < 4; i++)
//    {
//        filteredBlockTmp[i].create((m_iPicWidth + (m_iLumaMarginX << 1)), (m_iPicHeight + (m_iLumaMarginY << 1)));
//        // lumaaddr + m_iLumaMarginY   * getStride()  + m_iLumaMarginX
//    }
//
//    Int intStride = filteredBlockTmp[0].getWidth();
//    Int dstStride = srcStride;
//    Pel *srcPtr;    //Contains raw pixels
//    Short *intPtr;  // Intermediate results in short
//    Pel *dstPtr;    // Final filtered blocks in Pel
//
//    Int filterSize = 8;
//    Int halfFilterSize = (filterSize >> 1);
//
//    srcPtr = getLumaAddr();
//    //srcPtr = getBufY();
//    dstPtr = m_filteredBlockOrgY[0][0];
//    //dstPtr = m_filteredBlockBufY[0][0];
//
//    filterCopy(srcPtr-3*srcStride -3, srcStride, dstPtr-3*dstStride-3, dstStride, width+3+filterSize, height+3+filterSize);
//
//    intPtr = filteredBlockTmp[0].getLumaAddr()+offsetToLuma;
//#if ENABLE_PRIMITIVES
//    primitives.ipfilterConvert_p_s(g_bitDepthY, (pixel*)srcPtr-3*srcStride -3, srcStride, intPtr-3*intStride -3,
//                                   intStride,width+3+filterSize, height+3+filterSize);
//#else
//    filterConvertPelToShort(g_bitDepthY, srcPtr - 4, srcStride, intPtr,
//                            intStride, width + 5, height + filterSize);
//#endif
//
//    intPtr = filteredBlockTmp[0].getLumaAddr() + offsetToLuma;
//    dstPtr = m_filteredBlockOrgY[2][0];
//#if ENABLE_PRIMITIVES
//    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr,
//                                            dstStride, width, height, m_lumaFilter[2]);
//#else
//    filterVertical_short_pel<NTAPS_LUMA>(g_bitDepthY, intPtr, intStride, dstPtr,
//                                         dstStride, width, height + 1, m_lumaFilter[2]);
//#endif
//
//    intPtr = filteredBlockTmp[2].getLumaAddr() + offsetToLuma;
//#if ENABLE_PRIMITIVES
//    primitives.ipFilter_p_s[FILTER_H_P_S_8](g_bitDepthY, (pixel*)srcPtr-3*srcStride , srcStride, intPtr-3*srcStride, intStride, width, height+3+filterSize,  m_lumaFilter[2]);
//#else
//    filterHorizontal_pel_short<NTAPS_LUMA>(g_bitDepthY, srcPtr - 4, srcStride, intPtr, intStride, width + 1,
//                                           height + filterSize,  m_lumaFilter[2]);
//#endif
//
//    intPtr = filteredBlockTmp[2].getLumaAddr()+ offsetToLuma;
//    dstPtr = m_filteredBlockOrgY[0][2];
//#if ENABLE_PRIMITIVES
//    primitives.ipfilterConvert_s_p(g_bitDepthY, intPtr -3*srcStride , intStride, (pixel*)dstPtr -3*srcStride , dstStride, width, height +3 + filterSize);
//#else
//    filterConvertShortToPel(g_bitDepthY, intPtr, intStride, dstPtr, dstStride, width + 1, height + 0);
//#endif
//
//    intPtr = filteredBlockTmp[2].getLumaAddr()+ offsetToLuma;
//    dstPtr = m_filteredBlockOrgY[2][2];
//#if ENABLE_PRIMITIVES
//    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, dstStride, width, height, m_lumaFilter[2]);
//#else
//    filterVertical_short_pel<NTAPS_LUMA>(g_bitDepthY, intPtr, intStride, dstPtr, dstStride, width + 1, height + 1, m_lumaFilter[2]);
//#endif
//}

Void TComPicYuv::generateHQpel()
{
    Int width      = m_iPicWidth + (m_iLumaMarginX << 1)-8;
    Int height     =  m_iPicHeight+ (m_iLumaMarginY << 1)-8;
    Int srcStride  =  getStride();

    TShortYUV filteredBlockTmp[4];

    int offsetToLuma = m_iLumaMarginY   * getStride()  + m_iLumaMarginX;

    for (int i = 0; i < 4; i++)
    {
        filteredBlockTmp[i].create((m_iPicWidth + (m_iLumaMarginX << 1)), (m_iPicHeight + (m_iLumaMarginY << 1)));        
    }

    Int intStride = filteredBlockTmp[0].getWidth();
    Int dstStride = srcStride;
    Pel *srcPtr;    //Contains raw pixels
    Short *intPtr;  // Intermediate results in short
    Pel *dstPtr;    // Final filtered blocks in Pel

    Int filterSize = 8;
    Int halfFilterSize = (filterSize >> 1);

    srcPtr = getBufY();
    dstPtr = m_filteredBlockBufY[0][0];

    filterCopy(srcPtr, srcStride, dstPtr, dstStride, width, height);

    intPtr = filteredBlockTmp[0].getLumaAddr();
#if ENABLE_PRIMITIVES
    primitives.ipfilterConvert_p_s(g_bitDepthY, (pixel*)srcPtr, srcStride, intPtr,
                                   intStride, width, height);
#else
    filterConvertPelToShort(g_bitDepthY, srcPtr, srcStride, intPtr,
                                   intStride, width, height);
#endif

    intPtr = filteredBlockTmp[0].getLumaAddr() + (3)*intStride;
    dstPtr = m_filteredBlockBufY[2][0]+ (3)*srcStride;
#if ENABLE_PRIMITIVES
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr,
                                            dstStride, width, height - 6, m_lumaFilter[2]);
#else
    filterVertical_short_pel<NTAPS_LUMA>(g_bitDepthY, intPtr, intStride, dstPtr,
                                            dstStride, width, height - 6, m_lumaFilter[2]);
#endif

    intPtr = filteredBlockTmp[2].getLumaAddr() + 3;
#if ENABLE_PRIMITIVES
    primitives.ipFilter_p_s[FILTER_H_P_S_8](g_bitDepthY, (pixel*)srcPtr + 3, srcStride, intPtr, intStride, width - 6, height,  m_lumaFilter[2]);
#else
    filterHorizontal_pel_short<NTAPS_LUMA>(g_bitDepthY, srcPtr + 3, srcStride, intPtr, intStride, width - 6, height,  m_lumaFilter[2]);
#endif

    intPtr = filteredBlockTmp[2].getLumaAddr()+3;
    dstPtr = m_filteredBlockBufY[0][2]+3;
#if ENABLE_PRIMITIVES
    primitives.ipfilterConvert_s_p(g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, dstStride, width - 6, height + 0);
#else
    filterConvertShortToPel(g_bitDepthY, intPtr, intStride, dstPtr, dstStride, width - 6, height + 0);
#endif

    intPtr = filteredBlockTmp[2].getLumaAddr() + (3)*intStride + 3;
    dstPtr = m_filteredBlockBufY[2][2] +(3)*srcStride + 3;
#if ENABLE_PRIMITIVES
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, dstStride, width + 1, height + 1, m_lumaFilter[2]);
#else
    filterVertical_short_pel<NTAPS_LUMA>(g_bitDepthY, intPtr, intStride, dstPtr, dstStride, width + 1, height + 1, m_lumaFilter[2]);
#endif
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

    Int     shift = g_bitDepthY - 8;
    Int     offset = (shift > 0) ? (1 << (shift - 1)) : 0;

    Int   x, y;
    UChar uc;

    Pel*  piY   = getLumaAddr();
    Pel*  piCb  = getCbAddr();
    Pel*  piCr  = getCrAddr();

    for (y = 0; y < m_iPicHeight; y++)
    {
        for (x = 0; x < m_iPicWidth; x++)
        {
            uc = (UChar)Clip3<Pel>(0, 255, (piY[x] + offset) >> shift);

            fwrite(&uc, sizeof(UChar), 1, pFile);
        }

        piY += getStride();
    }

    shift = g_bitDepthC - 8;
    offset = (shift > 0) ? (1 << (shift - 1)) : 0;

    for (y = 0; y < m_iPicHeight >> 1; y++)
    {
        for (x = 0; x < m_iPicWidth >> 1; x++)
        {
            uc = (UChar)Clip3<Pel>(0, 255, (piCb[x] + offset) >> shift);
            fwrite(&uc, sizeof(UChar), 1, pFile);
        }

        piCb += getCStride();
    }

    for (y = 0; y < m_iPicHeight >> 1; y++)
    {
        for (x = 0; x < m_iPicWidth >> 1; x++)
        {
            uc = (UChar)Clip3<Pel>(0, 255, (piCr[x] + offset) >> shift);
            fwrite(&uc, sizeof(UChar), 1, pFile);
        }

        piCr += getCStride();
    }

    fclose(pFile);
}

//! \}

#include <stdint.h>

/* Copy pixels from an input picture (C structure) into internal TComPicYuv instance
 * Upscale pixels from 8bits to 16 bits when required, but do not modify pixels.
 * This new routine is GPL
 */
Void TComPicYuv::copyFromPicture(const x265_picture& pic)
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
        for (int r = 0; r < m_iPicHeight; r++)
        {
            for (int c = 0; c < m_iPicWidth; c++)
            {
                Y[c] = (Pel)y[c];
            }

            Y += getStride();
            y += pic.stride[0];
        }

        for (int r = 0; r < m_iPicHeight >> 1; r++)
        {
            for (int c = 0; c < m_iPicWidth >> 1; c++)
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
        int width = m_iPicWidth * (pic.bitDepth > 8 ? 2 : 1);

        // copy pixels by row into encoder's buffer
        for (int r = 0; r < m_iPicHeight; r++)
        {
            memcpy(Y, y, width);

            Y += getStride();
            y += pic.stride[0];
        }

        for (int r = 0; r < m_iPicHeight >> 1; r++)
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
