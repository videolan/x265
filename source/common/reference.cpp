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

#ifdef __GNUC__                         /* GCCs builtin atomics */
#define ATOMIC_INC(ptr)                 __sync_add_and_fetch((volatile int*)ptr, 1)
#elif _MSC_VER
#define ATOMIC_INC(ptr)                 InterlockedIncrement((volatile LONG*)ptr)
#else
#error "Must define atomic operations for this compiler"
#endif


MotionReference::MotionReference(TComPicYuv* pic, ThreadPool *pool)
    : JobProvider(pool)
{
    m_reconPic = pic;
    int width = pic->getWidth();
    int height = pic->getHeight();
    m_lumaStride = pic->getStride();
    m_startPad = pic->m_iLumaMarginY * m_lumaStride + pic->m_iLumaMarginX;
    m_next = NULL;

    m_lumaPlane[0][0] = pic->m_apiPicBufY + m_startPad;

    /* Create buffers for Hpel/Qpel Planes */
    size_t padwidth = width + pic->m_iLumaMarginX * 2;
    size_t padheight = height + pic->m_iLumaMarginY * 2;
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            if (i == 0 && j == 0)
                continue;
            m_lumaPlane[i][j] = (Pel*)xMalloc(pixel,  padwidth * padheight) + m_startPad;
        }
    }

    m_intStride = width + s_tmpMarginX * 4;
    m_extendOffset = s_tmpMarginY * m_lumaStride + s_tmpMarginX;
    m_offsetToLuma = s_tmpMarginY * 2 * m_intStride  + s_tmpMarginX * 2;
    m_filterWidth = width + s_tmpMarginX * 2;
    m_filterHeight = height + s_tmpMarginY * 2;
}

MotionReference::~MotionReference()
{
    JobProvider::Flush();

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            if (i == 0 && j == 0)
                continue;
            if (m_lumaPlane[i][j])
            { 
                xFree(m_lumaPlane[i][j] - m_startPad);
            }
        }
    }
}

void MotionReference::generateReferencePlanes()
{
    m_curPlane = m_finishedPlanes = 0;
    JobProvider::Enqueue();
    m_pool->PokeIdleThreads();
    m_pool->PokeIdleThreads();
    m_pool->PokeIdleThreads();
    m_pool->PokeIdleThreads();
    m_completionEvent.Wait();
    JobProvider::Dequeue();
}

bool MotionReference::FindJob()
{
    if (m_curPlane > 4)
        return false;

    int idx = ATOMIC_INC(&m_curPlane) - 1;
    if (idx < 4)
    {
        generateReferencePlane(idx);
        if (ATOMIC_INC(&m_finishedPlanes) == 4)
            m_completionEvent.Trigger();
        m_pool->PokeIdleThreads();
        return true;
    }
    return false;
}

void MotionReference::generateReferencePlane(int idx)
{
    PPAScopeEvent(GenerateReferencePlanes);

    short* filteredBlockTmp = (short*)xMalloc(short, m_intStride * (m_reconPic->getHeight() + s_tmpMarginY * 4));

    Pel *srcPtr = m_reconPic->getLumaAddr() - s_tmpMarginY * 2 * m_lumaStride - s_tmpMarginX * 2;
    Short *intPtr = filteredBlockTmp + m_offsetToLuma - s_tmpMarginY * 2 * m_intStride - s_tmpMarginX * 2;
    Pel *dstPtr;

    if (idx == 0)
    {
        // no horizontal filter necessary
        primitives.ipfilterConvert_p_s(g_bitDepthY, (pixel*)srcPtr, m_lumaStride, intPtr, m_intStride, m_filterWidth + 8, m_filterHeight + 8);

        /* The 0,0 (full-pel) plane needs no interpolation, and was already extended by TComPicYuv */
    }
    else
    {
        // Horizontal filter into intermediate buffer
        primitives.ipFilter_p_s[FILTER_H_P_S_8](g_bitDepthY, (pixel*)srcPtr, m_lumaStride, intPtr, m_intStride, m_filterWidth + 8, m_filterHeight + 8, TComPrediction::m_lumaFilter[idx]);

        // Generate @ 0,idx
        intPtr = filteredBlockTmp  + m_offsetToLuma - s_tmpMarginY * m_intStride - s_tmpMarginX;
        dstPtr = m_lumaPlane[idx][0]  - s_tmpMarginY * m_lumaStride - s_tmpMarginX;
        primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, m_intStride, (pixel*)dstPtr, m_lumaStride,  m_filterWidth, m_filterHeight, TComPrediction::m_lumaFilter[0]);
        m_reconPic->xExtendPicCompBorder(dstPtr, m_lumaStride, m_filterWidth, m_filterHeight, m_reconPic->m_iLumaMarginX - s_tmpMarginX, m_reconPic->m_iLumaMarginY - s_tmpMarginY);
    }

    // Generate @ 1,idx
    intPtr = filteredBlockTmp + m_offsetToLuma - s_tmpMarginY * m_intStride - s_tmpMarginX;
    dstPtr = m_lumaPlane[idx][1] - m_extendOffset;
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, m_intStride, (pixel*)dstPtr, m_lumaStride,  m_filterWidth, m_filterHeight, TComPrediction::m_lumaFilter[1]);
    m_reconPic->xExtendPicCompBorder(dstPtr, m_lumaStride, m_filterWidth, m_filterHeight, m_reconPic->m_iLumaMarginX - s_tmpMarginX, m_reconPic->m_iLumaMarginY - s_tmpMarginY);

    // Generate @ 2,idx
    dstPtr = m_lumaPlane[idx][2] - m_extendOffset;
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, m_intStride, (pixel*)dstPtr, m_lumaStride, m_filterWidth, m_filterHeight, TComPrediction::m_lumaFilter[2]);
    m_reconPic->xExtendPicCompBorder(dstPtr, m_lumaStride, m_filterWidth, m_filterHeight, m_reconPic->m_iLumaMarginX - s_tmpMarginX, m_reconPic->m_iLumaMarginY - s_tmpMarginY);

    // Generate @ 3,idx
    dstPtr = m_lumaPlane[idx][3] - m_extendOffset;
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, m_intStride, (pixel*)dstPtr, m_lumaStride, m_filterWidth, m_filterHeight, TComPrediction::m_lumaFilter[3]);
    m_reconPic->xExtendPicCompBorder(dstPtr, m_lumaStride, m_filterWidth, m_filterHeight, m_reconPic->m_iLumaMarginX - s_tmpMarginX, m_reconPic->m_iLumaMarginY - s_tmpMarginY);

    xFree(filteredBlockTmp);
}
