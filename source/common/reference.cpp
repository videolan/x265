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
#include "TLibCommon/TComInterpolationFilter.h"
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
    m_intStride = width + s_tmpMarginX * 4;
    m_extendOffset = s_tmpMarginY * m_lumaStride + s_tmpMarginX;
    m_offsetToLuma = s_tmpMarginY * 2 * m_intStride  + s_tmpMarginX * 2;
    m_filterWidth = width + s_tmpMarginX * 2;
    m_filterHeight = height + s_tmpMarginY * 2;
    m_next = NULL;

    m_lumaPlane[0][0] = (pixel *)pic->m_apiPicBufY + m_startPad;

    /* Create buffers for Hpel/Qpel Planes */
    size_t padwidth = width + pic->m_iLumaMarginX * 2;
    size_t padheight = height + pic->m_iLumaMarginY * 2;
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            if (i == 0 && j == 0)
                continue;
            m_lumaPlane[i][j] = (pixel *)xMalloc(pixel,  padwidth * padheight) + m_startPad;
        }
    }
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
    m_intermediateValues = (short*)xMalloc(short, 4 * m_intStride * (m_reconPic->getHeight() + s_tmpMarginY * 4));
    
    m_workerCount = 0;
    m_interReady[0] = m_interReady[1] = m_interReady[2] =  m_interReady[3] = 0;
    m_vplanesFinished[1] = m_vplanesFinished[2] = m_vplanesFinished[3] = 0;
    
    m_vplanesFinished[0] = 1; /* 0, 0 needs no work */
    m_finishedPlanes = 1;

    JobProvider::Enqueue();
    for (int i = 0; i < 4; i++)
        m_pool->PokeIdleThreads();
    m_completionEvent.Wait();
    JobProvider::Dequeue();

    xFree(m_intermediateValues);
}

bool MotionReference::FindJob()
{
    // Called by thread pool worker threads

    if (m_workerCount < 4)
    {
        /* First four threads calculate 4 intermediate buffers */
        int idx = ATOMIC_INC(&m_workerCount) - 1;
        if (idx < 4)
        {
            generateIntermediates(idx);
            m_interReady[idx] = 1;
            for (int i = 0; i < 4; i++)
                m_pool->PokeIdleThreads();
            return true;
        }
    }
    else
    {
        /* find a plane that needs processing */
        for (int x = 0; x < 4; x++)
        {
            while (m_interReady[x] && m_vplanesFinished[x] < 4)
            {
                /* intermediate buffer is available */
                int y = ATOMIC_INC(&m_vplanesFinished[x]) - 1;
                if (y < 4)
                {
                    generateReferencePlane(y * 4 + x);
                    if (ATOMIC_INC(&m_finishedPlanes) == 16)
                        m_completionEvent.Trigger();
                    return true;
                }
            }
        }
        return false;
    }
    return false;
}

void MotionReference::generateIntermediates(int x)
{
    PPAScopeEvent(GenerateIntermediates);

    short* filteredBlockTmp = m_intermediateValues + x * m_intStride * (m_reconPic->getHeight() + s_tmpMarginY * 4);
    Pel *srcPtr = m_reconPic->getLumaAddr() - s_tmpMarginY * 2 * m_lumaStride - s_tmpMarginX * 2;
    Short *intPtr = filteredBlockTmp + m_offsetToLuma - s_tmpMarginY * 2 * m_intStride - s_tmpMarginX * 2;

    if (x == 0)
    {
        // no horizontal filter necessary
        primitives.ipfilterConvert_p_s(g_bitDepthY, (pixel*)srcPtr, m_lumaStride, intPtr, m_intStride, m_filterWidth + 8, m_filterHeight + 8);
    }
    else
    {
        // Horizontal filter into intermediate buffer
        primitives.ipFilter_p_s[FILTER_H_P_S_8](g_bitDepthY, (pixel*)srcPtr, m_lumaStride, intPtr, m_intStride, m_filterWidth + 8, m_filterHeight + 8, TComInterpolationFilter::m_lumaFilter[x]);
    }
}

void MotionReference::generateReferencePlane(int idx)
{
    PPAScopeEvent(GenerateReferencePlanes);

    int x = idx & 3;
    int y = idx >> 2;
    short* filteredBlockTmp = m_intermediateValues + x * m_intStride * (m_reconPic->getHeight() + s_tmpMarginY * 4);
    Short *intPtr = filteredBlockTmp  + m_offsetToLuma - s_tmpMarginY * m_intStride - s_tmpMarginX;
    Pel *dstPtr = (Pel *)m_lumaPlane[x][y]  - s_tmpMarginY * m_lumaStride - s_tmpMarginX;
    primitives.ipFilter_s_p[FILTER_V_S_P_8](g_bitDepthY, intPtr, m_intStride, (pixel*)dstPtr, m_lumaStride,  m_filterWidth, m_filterHeight, TComInterpolationFilter::m_lumaFilter[y]);
    m_reconPic->xExtendPicCompBorder(dstPtr, m_lumaStride, m_filterWidth, m_filterHeight, m_reconPic->m_iLumaMarginX - s_tmpMarginX, m_reconPic->m_iLumaMarginY - s_tmpMarginY);
}
