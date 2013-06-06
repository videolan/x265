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
    lumaStride = pic->getStride();
    m_startPad = pic->m_iLumaMarginY * lumaStride + pic->m_iLumaMarginX;
    m_reconPic = pic;
    m_next = NULL;

    size_t padwidth = pic->getWidth() + pic->m_iLumaMarginX * 2;
    size_t padheight = pic->getHeight() + pic->m_iLumaMarginY * 2;

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            lumaPlane[i][j] = (Pel*)xMalloc(pixel,  padwidth * padheight);
            lumaPlane[i][j] += m_startPad;
        }
    }
}

MotionReference::~MotionReference()
{
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            if (lumaPlane[i][j])
            { 
                xFree(lumaPlane[i][j] - m_startPad);
            }
        }
    }
}

void MotionReference::generateReferencePlanes()
{
    PPAScopeEvent(TComPicYUV_extendPicBorder);

    /* Generate H/Q-pel for LumaBlocks  */
    generateLumaHQpel();

    // Extend borders
    int tmpMargin = 4;
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            if (i == 0 && j == 0)
                continue;

            m_reconPic->xExtendPicCompBorder(lumaPlane[i][j] - tmpMargin * lumaStride - tmpMargin,
                                             (int) lumaStride,
                                             m_reconPic->getWidth() + 2 * tmpMargin,
                                             m_reconPic->getHeight() + 2 * tmpMargin,
                                             m_reconPic->m_iLumaMarginX - tmpMargin,
                                             m_reconPic->m_iLumaMarginY - tmpMargin);
        }
    }
}

void MotionReference::generateLumaHQpel()
{
    int width      = m_reconPic->getWidth();
    int height     = m_reconPic->getHeight();

    int tmpMarginX = 4; //Generate subpels for entire frame with a margin of tmpMargin
    int tmpMarginY = 4;
    int intStride = width + (tmpMarginX << 2);
    int rows = height + (tmpMarginY << 2);

    // TODO: class statics? would require locking
    short* filteredBlockTmp[4];
    for (int i = 0; i < 4; i++)
    {
        filteredBlockTmp[i] = (short*)xMalloc(short, intStride * rows);
    }

    int offsetToLuma = (tmpMarginY << 1) * intStride  + (tmpMarginX << 1);
    Pel *srcPtr = m_reconPic->getLumaAddr() - (tmpMarginY + 4) * lumaStride - (tmpMarginX + 4);

    Short *intPtr;  // Intermediate results in short
    Pel *dstPtr;    // Final filtered blocks in Pel

    dstPtr = lumaPlane[0][0] - (tmpMarginY + 4) * lumaStride - (tmpMarginX + 4);

    /* No need to calculate m_filteredBlock[0][0], it is copied */
    memcpy(lumaPlane[0][0] - m_startPad, m_reconPic->m_apiPicBufY, ((width + (m_reconPic->m_iLumaMarginX << 1)) * (height + (m_reconPic->m_iLumaMarginY << 1))) * sizeof(pixel));

    intPtr = filteredBlockTmp[0] + offsetToLuma - (tmpMarginY + 4) * intStride - (tmpMarginX + 4);
    primitives.ipfilterConvert_p_s(g_bitDepthY, (pixel*)srcPtr, lumaStride, intPtr, intStride, width + (tmpMarginX << 1) + 8, height + (tmpMarginY << 1) + 8);

    intPtr = filteredBlockTmp[0] + offsetToLuma - tmpMarginY * intStride - tmpMarginX;
    dstPtr = lumaPlane[0][2] - tmpMarginY * lumaStride - tmpMarginX;
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, lumaStride, width + (tmpMarginX << 1), height + (tmpMarginY << 1), TComPrediction::m_lumaFilter[2]);

    intPtr = filteredBlockTmp[2] + offsetToLuma - (tmpMarginY + 4) * intStride - (tmpMarginX + 4);
    primitives.ipFilter_p_s[FILTER_H_P_S_8](g_bitDepthY, (pixel*)srcPtr, lumaStride, intPtr, intStride, width + (tmpMarginX << 1) + 8, height + (tmpMarginY << 1) + 8,  TComPrediction::m_lumaFilter[2]);

    intPtr = filteredBlockTmp[2] + offsetToLuma - tmpMarginY * intStride - tmpMarginX;
    dstPtr = lumaPlane[2][0] - tmpMarginY * lumaStride - tmpMarginX;
    primitives.ipfilterConvert_s_p(g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, lumaStride, width + (tmpMarginX << 1), height + (tmpMarginY << 1));

    intPtr = filteredBlockTmp[2] + offsetToLuma - tmpMarginY * intStride - tmpMarginX;
    dstPtr = lumaPlane[2][2] - tmpMarginY * lumaStride - tmpMarginX;
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, lumaStride, width + (tmpMarginX << 1), height + (tmpMarginY << 1), TComPrediction::m_lumaFilter[2]);

    /* Generate QPels */
    intPtr = filteredBlockTmp[1] + offsetToLuma - (tmpMarginY + 4) * intStride - (tmpMarginX + 4);
    primitives.ipFilter_p_s[FILTER_H_P_S_8](g_bitDepthY, (pixel*)srcPtr, lumaStride, intPtr, intStride, width + (tmpMarginX << 1) + 8, height + (tmpMarginY << 1) + 8, TComPrediction::m_lumaFilter[1]);

    intPtr = filteredBlockTmp[3] + offsetToLuma - (tmpMarginY + 4) * intStride - (tmpMarginX + 4);
    primitives.ipFilter_p_s[FILTER_H_P_S_8](g_bitDepthY, (pixel*)srcPtr, lumaStride, intPtr, intStride, width + (tmpMarginX << 1) + 8, height + (tmpMarginY << 1) + 8, TComPrediction::m_lumaFilter[3]);

    // Generate @ 1,1
    intPtr = filteredBlockTmp[1]  + offsetToLuma - tmpMarginY * intStride - tmpMarginX;
    dstPtr = lumaPlane[1][1] - tmpMarginY * lumaStride - tmpMarginX;
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, lumaStride, width + (tmpMarginX << 1), height + (tmpMarginY << 1), TComPrediction::m_lumaFilter[1]);

    // Generate @ 3,1
    intPtr = filteredBlockTmp[1] + offsetToLuma - tmpMarginY * intStride - tmpMarginX;
    dstPtr = lumaPlane[1][3] - tmpMarginY * lumaStride - tmpMarginX;
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, lumaStride, width + (tmpMarginX << 1), height + (tmpMarginY << 1), TComPrediction::m_lumaFilter[3]);

    // Generate @ 2,1
    intPtr = filteredBlockTmp[1] + offsetToLuma - tmpMarginY * intStride - tmpMarginX;
    dstPtr = lumaPlane[1][2] - tmpMarginY * lumaStride - tmpMarginX;
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, lumaStride, width + (tmpMarginX << 1), height + (tmpMarginY << 1), TComPrediction::m_lumaFilter[2]);

    // Generate @ 2,3
    intPtr = filteredBlockTmp[3] + offsetToLuma - tmpMarginY * intStride - tmpMarginX;
    dstPtr = lumaPlane[3][2] - tmpMarginY * lumaStride - tmpMarginX;
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, lumaStride, width + (tmpMarginX << 1), height + (tmpMarginY << 1), TComPrediction::m_lumaFilter[2]);

    // Generate @ 0,1
    intPtr = filteredBlockTmp[1]  + offsetToLuma - tmpMarginY * intStride - tmpMarginX;
    dstPtr = lumaPlane[1][0]  - tmpMarginY * lumaStride - tmpMarginX;
    primitives.ipfilterConvert_s_p(g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, lumaStride, width + (tmpMarginX << 1), height + (tmpMarginY << 1));

    // Generate @ 0,3
    intPtr = filteredBlockTmp[3] + offsetToLuma - tmpMarginY * intStride - tmpMarginX;
    dstPtr = lumaPlane[3][0] - tmpMarginY * lumaStride - tmpMarginX;
    primitives.ipfilterConvert_s_p(g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, lumaStride, width + (tmpMarginX << 1), height + (tmpMarginY << 1));

    // Generate @ 1,2
    intPtr = filteredBlockTmp[2] + offsetToLuma - tmpMarginY * intStride - tmpMarginX;
    dstPtr = lumaPlane[2][1] - tmpMarginY * lumaStride - tmpMarginX;
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, lumaStride, width + (tmpMarginX << 1), height + (tmpMarginY << 1), TComPrediction::m_lumaFilter[1]);

    // Generate @ 3,2
    intPtr = filteredBlockTmp[2] + offsetToLuma - tmpMarginY * intStride - tmpMarginX;
    dstPtr = lumaPlane[2][3] - tmpMarginY * lumaStride - tmpMarginX;
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, lumaStride,  width + (tmpMarginX << 1), height + (tmpMarginY << 1), TComPrediction::m_lumaFilter[3]);

    // Generate @ 1,0
    intPtr = filteredBlockTmp[0] + offsetToLuma - tmpMarginY * intStride - tmpMarginX;
    dstPtr = lumaPlane[0][1] - tmpMarginY * lumaStride - tmpMarginX;
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, lumaStride,  width + (tmpMarginX << 1), height + (tmpMarginY << 1), TComPrediction::m_lumaFilter[1]);

    // Generate @ 3,0
    intPtr = filteredBlockTmp[0] + offsetToLuma - tmpMarginY * intStride - tmpMarginX;
    dstPtr = lumaPlane[0][3] - tmpMarginY * lumaStride - tmpMarginX;
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, lumaStride, width + (tmpMarginX << 1), height + (tmpMarginY << 1), TComPrediction::m_lumaFilter[3]);

    // Generate @ 1,3
    intPtr = filteredBlockTmp[3] + offsetToLuma - tmpMarginY * intStride - tmpMarginX;
    dstPtr = lumaPlane[3][1] - tmpMarginY * lumaStride - tmpMarginX;
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, lumaStride, width + (tmpMarginX << 1), height + (tmpMarginY << 1), TComPrediction::m_lumaFilter[1]);

    // Generate @ 3,3
    intPtr = filteredBlockTmp[3] + offsetToLuma - tmpMarginY * intStride - tmpMarginX;
    dstPtr = lumaPlane[3][3] - tmpMarginY * lumaStride - tmpMarginX;
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, intStride, (pixel*)dstPtr, lumaStride, width + (tmpMarginX << 1), height + (tmpMarginY << 1), TComPrediction::m_lumaFilter[3]);

    for (int i = 0; i < 4; i++)
    {
        xFree(filteredBlockTmp[i]);
    }
}
