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

    m_lumaPlane[0][0] = (pixel*)pic->m_apiPicBufY + m_startPad;

    /* Create buffers for Hpel/Qpel Planes */
    size_t padwidth = width + pic->m_iLumaMarginX * 2;
    size_t padheight = height + pic->m_iLumaMarginY * 2;
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            if (i == 0 && j == 0)
                continue;
            m_lumaPlane[i][j] = (pixel*)xMalloc(pixel,  padwidth * padheight) + m_startPad;
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
    {
        PPAScopeEvent(GenerateIntermediates);
        m_intermediateValues = (short*)xMalloc(short, 4 * m_intStride * (m_reconPic->getHeight() + s_tmpMarginY * 4));

        short* intPtrF = m_intermediateValues;
        short* intPtrA = m_intermediateValues + 1 * m_intStride * (m_reconPic->getHeight() + s_tmpMarginY * 4);
        short* intPtrB = m_intermediateValues + 2 * m_intStride * (m_reconPic->getHeight() + s_tmpMarginY * 4);
        short* intPtrC = m_intermediateValues + 3 * m_intStride * (m_reconPic->getHeight() + s_tmpMarginY * 4);

        int bufOffset = -(s_tmpMarginY + s_intMarginY) * m_lumaStride - (s_tmpMarginX + s_intMarginX);
        pixel *srcPtr = (pixel*)m_reconPic->getLumaAddr() + bufOffset;

        /* This one function call generates the four intermediate (short) planes for each
         * QPEL offset in the horizontal direction.  At the same time it outputs the three
         * Y=0 output (pixel) planes since they require no vertical interpolation */
        primitives.filterHmulti(g_bitDepthY, srcPtr, m_lumaStride,                                     // source buffer
                                intPtrF, intPtrA, intPtrB, intPtrC, m_intStride,                       // 4 intermediate HPEL buffers
                                m_lumaPlane[1][0] + bufOffset, m_lumaPlane[2][0] + bufOffset, m_lumaPlane[3][0] + bufOffset, m_lumaStride, // 3 (x=n, y=0) output buffers (no V interp)
                                m_filterWidth + (2 * s_intMarginX), m_filterHeight + (2 * s_intMarginY));
        m_reconPic->xExtendPicCompBorder(m_lumaPlane[1][0] + bufOffset, m_lumaStride, m_filterWidth + (2 * s_intMarginX), m_filterHeight + (2 * s_intMarginY), m_reconPic->m_iLumaMarginX - s_tmpMarginX - s_intMarginX, m_reconPic->m_iLumaMarginY - s_tmpMarginY - s_intMarginY);
        m_reconPic->xExtendPicCompBorder(m_lumaPlane[2][0] + bufOffset, m_lumaStride, m_filterWidth + (2 * s_intMarginX), m_filterHeight + (2 * s_intMarginY), m_reconPic->m_iLumaMarginX - s_tmpMarginX - s_intMarginX, m_reconPic->m_iLumaMarginY - s_tmpMarginY - s_intMarginY);
        m_reconPic->xExtendPicCompBorder(m_lumaPlane[3][0] + bufOffset, m_lumaStride, m_filterWidth + (2 * s_intMarginX), m_filterHeight + (2 * s_intMarginY), m_reconPic->m_iLumaMarginX - s_tmpMarginX - s_intMarginX, m_reconPic->m_iLumaMarginY - s_tmpMarginY - s_intMarginY);
    }

    m_workerCount = 0;
    m_finishedPlanes = 0;

    JobProvider::Enqueue();
    for (int i = 0; i < 4; i++)
    {
        m_pool->PokeIdleThreads();
    }

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
            generateReferencePlane(idx);
            if (ATOMIC_INC(&m_finishedPlanes) == 4)
                m_completionEvent.Trigger();
            return true;
        }
    }
    return false;
}

void MotionReference::generateReferencePlane(int x)
{
    PPAScopeEvent(GenerateReferencePlanes);

    /* this function will be called by 4 threads, with x = 0, 1, 2, 3 */
    short* filteredBlockTmp = m_intermediateValues + x * m_intStride * (m_reconPic->getHeight() + s_tmpMarginY * 4);
    short* intPtr = filteredBlockTmp + s_intMarginY * m_intStride + s_intMarginX;

    pixel *dstPtr1 = m_lumaPlane[x][1] - s_tmpMarginY * m_lumaStride - s_tmpMarginX;
    pixel *dstPtr2 = m_lumaPlane[x][2] - s_tmpMarginY * m_lumaStride - s_tmpMarginX;
    pixel *dstPtr3 = m_lumaPlane[x][3] - s_tmpMarginY * m_lumaStride - s_tmpMarginX;

    primitives.filterVmulti(g_bitDepthY, intPtr, m_intStride, dstPtr1, dstPtr2, dstPtr3, m_lumaStride, m_filterWidth, m_filterHeight);
    m_reconPic->xExtendPicCompBorder(dstPtr1, m_lumaStride, m_filterWidth, m_filterHeight, m_reconPic->m_iLumaMarginX - s_tmpMarginX, m_reconPic->m_iLumaMarginY - s_tmpMarginY);
    m_reconPic->xExtendPicCompBorder(dstPtr2, m_lumaStride, m_filterWidth, m_filterHeight, m_reconPic->m_iLumaMarginX - s_tmpMarginX, m_reconPic->m_iLumaMarginY - s_tmpMarginY);
    m_reconPic->xExtendPicCompBorder(dstPtr3, m_lumaStride, m_filterWidth, m_filterHeight, m_reconPic->m_iLumaMarginX - s_tmpMarginX, m_reconPic->m_iLumaMarginY - s_tmpMarginY);
}
