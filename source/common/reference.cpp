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

#include "TLibCommon/CommonDef.h"
#include "TLibCommon/TComPicYuv.h"
#include "TLibCommon/TComPrediction.h"
#include "PPA/ppa.h"
#include "primitives.h"
#include "reference.h"

using namespace x265;

MotionReference::MotionReference(TComPicYuv* pic)
{
    /* Create buffers for Hpel/Qpel Planes */
    m_lumaStride = pic->getStride();
    m_startPad = pic->m_iLumaMarginY * m_lumaStride + pic->m_iLumaMarginX;
    m_reconPic = pic;
    m_next = NULL;

    size_t padwidth = pic->getWidth() + pic->m_iLumaMarginX * 2;
    size_t padheight = pic->getHeight() + pic->m_iLumaMarginY * 2;

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            m_lumaPlane[i][j] = (Pel*)xMalloc(pixel,  padwidth * padheight);
            m_lumaPlane[i][j] += m_startPad;
        }
    }
}

MotionReference::~MotionReference()
{
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            if (m_lumaPlane[i][j])
            { 
                xFree(m_lumaPlane[i][j] - m_startPad);
            }
        }
    }
}

void MotionReference::generateReferencePlanes()
{
    PPAScopeEvent(TComPicYUV_extendPicBorder);

    int width      = m_reconPic->getWidth();
    int height     = m_reconPic->getHeight();

    int tmpMarginX = 4; //Generate subpels for entire frame with a margin of tmpMargin
    int tmpMarginY = 4;
    int intStride = width + (tmpMarginX << 2);
    int rows = height + (tmpMarginY << 2);

    // TODO: class static? would require locking
    short* filteredBlockTmp;
    filteredBlockTmp = (short*)xMalloc(short, intStride * rows);

    int offsetToLuma = (tmpMarginY << 1) * intStride  + (tmpMarginX << 1);
    Pel *srcPtr = m_reconPic->getLumaAddr() - (tmpMarginY + 4) * m_lumaStride - (tmpMarginX + 4);

    Short *intPtr;  // Intermediate results in short
    Pel *dstPtr;    // Final filtered blocks in Pel

    intptr_t extendOffset = tmpMarginY * m_lumaStride + tmpMarginX;
    int filterWidth = width + (tmpMarginX << 1);
    int filterHeight = height + (tmpMarginY << 1);

    /* No need to calculate m_filteredBlock[0][0], it is copied */
    memcpy(m_lumaPlane[0][0] - m_startPad, m_reconPic->m_apiPicBufY, ((width + (m_reconPic->m_iLumaMarginX << 1)) * (height + (m_reconPic->m_iLumaMarginY << 1))) * sizeof(pixel));

    intPtr = filteredBlockTmp + offsetToLuma - (tmpMarginY + 4) * intStride - (tmpMarginX + 4);
    primitives.ipfilterConvert_p_s(g_bitDepthY, (pixel*)srcPtr, m_lumaStride, intPtr, intStride, filterWidth + 8, filterHeight + 8);

    // Generate @ 1,0
    intPtr = filteredBlockTmp + offsetToLuma - tmpMarginY * intStride - tmpMarginX;
    dstPtr = m_lumaPlane[0][1] - extendOffset;
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, m_lumaStride,  filterWidth, filterHeight, TComPrediction::m_lumaFilter[1]);
    m_reconPic->xExtendPicCompBorder(m_lumaPlane[0][1] - extendOffset, m_lumaStride, m_reconPic->getWidth() + 2 * tmpMarginX, m_reconPic->getHeight() + 2 * tmpMarginY, m_reconPic->m_iLumaMarginX - tmpMarginX, m_reconPic->m_iLumaMarginY - tmpMarginY);

    // Generate @ 2,0
    dstPtr = m_lumaPlane[0][2] - extendOffset;
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, m_lumaStride, filterWidth, filterHeight, TComPrediction::m_lumaFilter[2]);
    m_reconPic->xExtendPicCompBorder(m_lumaPlane[0][2] - extendOffset, m_lumaStride, m_reconPic->getWidth() + 2 * tmpMarginX, m_reconPic->getHeight() + 2 * tmpMarginY, m_reconPic->m_iLumaMarginX - tmpMarginX, m_reconPic->m_iLumaMarginY - tmpMarginY);

    // Generate @ 3,0
    dstPtr = m_lumaPlane[0][3] - extendOffset;
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, m_lumaStride, filterWidth, filterHeight, TComPrediction::m_lumaFilter[3]);
    m_reconPic->xExtendPicCompBorder(m_lumaPlane[0][3] - extendOffset, m_lumaStride, m_reconPic->getWidth() + 2 * tmpMarginX, m_reconPic->getHeight() + 2 * tmpMarginY, m_reconPic->m_iLumaMarginX - tmpMarginX, m_reconPic->m_iLumaMarginY - tmpMarginY);



    intPtr = filteredBlockTmp + offsetToLuma - (tmpMarginY + 4) * intStride - (tmpMarginX + 4);
    primitives.ipFilter_p_s[FILTER_H_P_S_8](g_bitDepthY, (pixel*)srcPtr, m_lumaStride, intPtr, intStride, filterWidth + 8, filterHeight + 8, TComPrediction::m_lumaFilter[1]);

    // Generate @ 0,1
    intPtr = filteredBlockTmp  + offsetToLuma - tmpMarginY * intStride - tmpMarginX;
    dstPtr = m_lumaPlane[1][0]  - tmpMarginY * m_lumaStride - tmpMarginX;
    primitives.ipfilterConvert_s_p(g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, m_lumaStride, filterWidth, filterHeight);
    m_reconPic->xExtendPicCompBorder(m_lumaPlane[1][0] - extendOffset, m_lumaStride, m_reconPic->getWidth() + 2 * tmpMarginX, m_reconPic->getHeight() + 2 * tmpMarginY, m_reconPic->m_iLumaMarginX - tmpMarginX, m_reconPic->m_iLumaMarginY - tmpMarginY);

    // Generate @ 1,1
    dstPtr = m_lumaPlane[1][1] - extendOffset;
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, m_lumaStride, filterWidth, filterHeight, TComPrediction::m_lumaFilter[1]);
    m_reconPic->xExtendPicCompBorder(m_lumaPlane[1][1] - extendOffset, m_lumaStride, m_reconPic->getWidth() + 2 * tmpMarginX, m_reconPic->getHeight() + 2 * tmpMarginY, m_reconPic->m_iLumaMarginX - tmpMarginX, m_reconPic->m_iLumaMarginY - tmpMarginY);

    // Generate @ 2,1
    dstPtr = m_lumaPlane[1][2] - extendOffset;
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, m_lumaStride, filterWidth, filterHeight, TComPrediction::m_lumaFilter[2]);
    m_reconPic->xExtendPicCompBorder(m_lumaPlane[1][2] - extendOffset, m_lumaStride, m_reconPic->getWidth() + 2 * tmpMarginX, m_reconPic->getHeight() + 2 * tmpMarginY, m_reconPic->m_iLumaMarginX - tmpMarginX, m_reconPic->m_iLumaMarginY - tmpMarginY);

    // Generate @ 3,1
    dstPtr = m_lumaPlane[1][3] - extendOffset;
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, m_lumaStride, filterWidth, filterHeight, TComPrediction::m_lumaFilter[3]);
    m_reconPic->xExtendPicCompBorder(m_lumaPlane[1][3] - extendOffset, m_lumaStride, m_reconPic->getWidth() + 2 * tmpMarginX, m_reconPic->getHeight() + 2 * tmpMarginY, m_reconPic->m_iLumaMarginX - tmpMarginX, m_reconPic->m_iLumaMarginY - tmpMarginY);




    intPtr = filteredBlockTmp + offsetToLuma - (tmpMarginY + 4) * intStride - (tmpMarginX + 4);
    primitives.ipFilter_p_s[FILTER_H_P_S_8](g_bitDepthY, (pixel*)srcPtr, m_lumaStride, intPtr, intStride, filterWidth + 8, filterHeight + 8,  TComPrediction::m_lumaFilter[2]);

    // Generate @ 0,2
    intPtr = filteredBlockTmp + offsetToLuma - tmpMarginY * intStride - tmpMarginX;
    dstPtr = m_lumaPlane[2][0] - extendOffset;
    primitives.ipfilterConvert_s_p(g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, m_lumaStride, filterWidth, filterHeight);
    m_reconPic->xExtendPicCompBorder(m_lumaPlane[2][0] - extendOffset, m_lumaStride, m_reconPic->getWidth() + 2 * tmpMarginX, m_reconPic->getHeight() + 2 * tmpMarginY, m_reconPic->m_iLumaMarginX - tmpMarginX, m_reconPic->m_iLumaMarginY - tmpMarginY);

    // Generate @ 1,2
    dstPtr = m_lumaPlane[2][1] - extendOffset;
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, m_lumaStride, filterWidth, filterHeight, TComPrediction::m_lumaFilter[1]);
    m_reconPic->xExtendPicCompBorder(m_lumaPlane[2][1] - extendOffset, m_lumaStride, m_reconPic->getWidth() + 2 * tmpMarginX, m_reconPic->getHeight() + 2 * tmpMarginY, m_reconPic->m_iLumaMarginX - tmpMarginX, m_reconPic->m_iLumaMarginY - tmpMarginY);

    // Generate @ 2,2
    dstPtr = m_lumaPlane[2][2] - extendOffset;
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, m_lumaStride, filterWidth, filterHeight, TComPrediction::m_lumaFilter[2]);
    m_reconPic->xExtendPicCompBorder(m_lumaPlane[2][2] - extendOffset, m_lumaStride, m_reconPic->getWidth() + 2 * tmpMarginX, m_reconPic->getHeight() + 2 * tmpMarginY, m_reconPic->m_iLumaMarginX - tmpMarginX, m_reconPic->m_iLumaMarginY - tmpMarginY);

    // Generate @ 3,2
    dstPtr = m_lumaPlane[2][3] - extendOffset;
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, m_lumaStride,  filterWidth, filterHeight, TComPrediction::m_lumaFilter[3]);
    m_reconPic->xExtendPicCompBorder(m_lumaPlane[2][3] - extendOffset, m_lumaStride, m_reconPic->getWidth() + 2 * tmpMarginX, m_reconPic->getHeight() + 2 * tmpMarginY, m_reconPic->m_iLumaMarginX - tmpMarginX, m_reconPic->m_iLumaMarginY - tmpMarginY);



    intPtr = filteredBlockTmp + offsetToLuma - (tmpMarginY + 4) * intStride - (tmpMarginX + 4);
    primitives.ipFilter_p_s[FILTER_H_P_S_8](g_bitDepthY, (pixel*)srcPtr, m_lumaStride, intPtr, intStride, filterWidth + 8, filterHeight + 8, TComPrediction::m_lumaFilter[3]);

    // Generate @ 0,3
    intPtr = filteredBlockTmp + offsetToLuma - tmpMarginY * intStride - tmpMarginX;
    dstPtr = m_lumaPlane[3][0] - extendOffset;
    primitives.ipfilterConvert_s_p(g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, m_lumaStride, filterWidth, filterHeight);
    m_reconPic->xExtendPicCompBorder(m_lumaPlane[3][0] - extendOffset, m_lumaStride, m_reconPic->getWidth() + 2 * tmpMarginX, m_reconPic->getHeight() + 2 * tmpMarginY, m_reconPic->m_iLumaMarginX - tmpMarginX, m_reconPic->m_iLumaMarginY - tmpMarginY);

    // Generate @ 1,3
    dstPtr = m_lumaPlane[3][1] - extendOffset;
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, m_lumaStride, filterWidth, filterHeight, TComPrediction::m_lumaFilter[1]);
    m_reconPic->xExtendPicCompBorder(m_lumaPlane[3][1] - extendOffset, m_lumaStride, m_reconPic->getWidth() + 2 * tmpMarginX, m_reconPic->getHeight() + 2 * tmpMarginY, m_reconPic->m_iLumaMarginX - tmpMarginX, m_reconPic->m_iLumaMarginY - tmpMarginY);

    // Generate @ 2,3
    dstPtr = m_lumaPlane[3][2] - extendOffset;
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, m_lumaStride, filterWidth, filterHeight, TComPrediction::m_lumaFilter[2]);
    m_reconPic->xExtendPicCompBorder(m_lumaPlane[3][2] - extendOffset, m_lumaStride, m_reconPic->getWidth() + 2 * tmpMarginX, m_reconPic->getHeight() + 2 * tmpMarginY, m_reconPic->m_iLumaMarginX - tmpMarginX, m_reconPic->m_iLumaMarginY - tmpMarginY);

    // Generate @ 3,3
    dstPtr = m_lumaPlane[3][3] - extendOffset;
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, m_lumaStride, filterWidth, filterHeight, TComPrediction::m_lumaFilter[3]);
    m_reconPic->xExtendPicCompBorder(m_lumaPlane[3][3] - extendOffset, m_lumaStride, m_reconPic->getWidth() + 2 * tmpMarginX, m_reconPic->getHeight() + 2 * tmpMarginY, m_reconPic->m_iLumaMarginX - tmpMarginX, m_reconPic->m_iLumaMarginY - tmpMarginY);

    xFree(filteredBlockTmp);
}
