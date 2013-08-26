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

#include "TLibCommon/TypeDef.h"
#include "TLibCommon/TComPicYuv.h"
#include "TLibCommon/TComSlice.h"
#include "PPA/ppa.h"
#include "primitives.h"
#include "reference.h"

using namespace x265;

#ifdef __GNUC__                         /* GCCs builtin atomics */
#define ATOMIC_INC(ptr) __sync_add_and_fetch((volatile int*)ptr, 1)
#elif _MSC_VER
#define ATOMIC_INC(ptr) InterlockedIncrement((volatile LONG*)ptr)
#else
#error "Must define atomic operations for this compiler"
#endif

void ReferencePlanes::setWeight(const wpScalingParam& w)
{
    weight = w.inputWeight;
    offset = w.inputOffset * (1 << (X265_DEPTH - 8));
    shift  = w.log2WeightDenom;
    round  = (w.log2WeightDenom >= 1) ? (1 << (w.log2WeightDenom - 1)) : (0);
    isWeighted = true;
}

bool ReferencePlanes::matchesWeight(const wpScalingParam& w)
{
    if (!isWeighted)
        return false;

    if ((weight == w.inputWeight) && (shift == (int)w.log2WeightDenom) &&
        (offset == w.inputOffset * (1 << (X265_DEPTH - 8))))
        return true;

    return false;
}

MotionReference::MotionReference(TComPicYuv* pic, ThreadPool *pool, wpScalingParam *w)
    : JobProvider(pool)
{
    m_reconPic = pic;
    int width = pic->getWidth();
    int height = pic->getHeight();
    lumaStride = pic->getStride();
    m_startPad = pic->m_lumaMarginY * lumaStride + pic->m_lumaMarginX;
    m_intStride = width + s_tmpMarginX * 4;
    m_extendOffset = s_tmpMarginY * lumaStride + s_tmpMarginX;
    m_offsetToLuma = s_tmpMarginY * 2 * m_intStride  + s_tmpMarginX * 2;
    m_filterWidth = width + s_tmpMarginX * 2;
    m_filterHeight = height + s_tmpMarginY * 2;
    m_next = NULL;

    /* Create buffers for Hpel/Qpel Planes */
    size_t padwidth = width + pic->m_lumaMarginX * 2;
    size_t padheight = height + pic->m_lumaMarginY * 2;

    for (int i = 0; i < 4; i++)
    {
        // TODO: I am not sure [0] need space when weight, for safe I alloc either, but I DON'T FILL [0] anymore
        m_midBuf[i] = new short[width * (height + 3 + 4)];  // middle buffer extend size: left(0), right(0), top(3), bottom(4)
        m_midBuf[i] += 3 * width;
    }

    if (w) 
    {
        setWeight(*w);
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                lumaPlane[i][j] = (pixel*)X265_MALLOC(pixel,  padwidth * padheight) + m_startPad;
            }
        }
    }
    else
    {
        /* directly reference the pre-extended integer pel plane */
        lumaPlane[0][0] = pic->m_picBufY + m_startPad;

        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                if (i == 0 && j == 0)
                    continue;
                lumaPlane[i][j] = (pixel*)X265_MALLOC(pixel,  padwidth * padheight) + m_startPad;
            }
        }
    }
}

MotionReference::~MotionReference()
{
    JobProvider::flush();

    int width = m_reconPic->getWidth();

    for (int i = 0; i < 4; i++)
    {
        m_midBuf[i] -= 3 * width;
        if (m_midBuf[i])
        {
            delete[] m_midBuf[i];
            m_midBuf[i] = NULL;
        }
        for (int j = 0; j < 4; j++)
        {
            if (i == 0 && j == 0 && !isWeighted)
                continue;
            if (lumaPlane[i][j])
            {
                X265_FREE(lumaPlane[i][j] - m_startPad);
            }
        }
    }
}

void MotionReference::generateReferencePlanes()
{
    {
        PPAScopeEvent(GenerateIntermediates);
        m_intermediateValues = (short*)X265_MALLOC(short, 4 * m_intStride * (m_reconPic->getHeight() + s_tmpMarginY * 4));

        short* intPtrF = m_intermediateValues;
        short* intPtrA = m_intermediateValues + 1 * m_intStride * (m_reconPic->getHeight() + s_tmpMarginY * 4);
        short* intPtrB = m_intermediateValues + 2 * m_intStride * (m_reconPic->getHeight() + s_tmpMarginY * 4);
        short* intPtrC = m_intermediateValues + 3 * m_intStride * (m_reconPic->getHeight() + s_tmpMarginY * 4);

        int bufOffset = -(s_tmpMarginY + s_intMarginY) * lumaStride - (s_tmpMarginX + s_intMarginX);
        pixel *srcPtr = m_reconPic->getLumaAddr() + bufOffset;

        /* This one function call generates the four intermediate (short) planes for each
         * QPEL offset in the horizontal direction.  At the same time it outputs the three
         * Y=0 output (padded pixel) planes since they require no vertical interpolation */
        if (isWeighted)
        {
            primitives.filterHwghtd(srcPtr, lumaStride,                            // source buffer
                                    intPtrF, intPtrA, intPtrB, intPtrC, m_intStride, // 4 intermediate HPEL buffers
                                    lumaPlane[0][0] + bufOffset,
                                    lumaPlane[1][0] + bufOffset,
                                    lumaPlane[2][0] + bufOffset,
                                    lumaPlane[3][0] + bufOffset, lumaStride,     // 3 (x=n, y=0) output buffers (no V interp)
                                    m_filterWidth + (2 * s_intMarginX),              // filter dimensions with margins
                                    m_filterHeight + (2 * s_intMarginY),
                                    m_reconPic->m_lumaMarginX - s_tmpMarginX - s_intMarginX, // pixel extension margins
                                    m_reconPic->m_lumaMarginY - s_tmpMarginY - s_intMarginY,
                                    weight, round, shift, offset);
       }
       else
       {
            int midStride = m_reconPic->getWidth();
            int height = g_maxCUHeight;
            for(int i = 0; i < m_reconPic->m_numCuInHeight; i++ )
            {
                int isLast = (i == m_reconPic->m_numCuInHeight - 1);
                int rowAddr = i * height * lumaStride;
                int rowAddrMid = (i * height) * midStride;
                if (isLast)
                {
                    int rem = m_reconPic->getHeight() % height;
                    if (rem)
                        height = rem;
                }

                primitives.filterRowH(m_reconPic->getLumaAddr() + rowAddr,
                                      lumaStride,
                                      m_midBuf[1] + rowAddrMid,
                                      m_midBuf[2] + rowAddrMid,
                                      m_midBuf[3] + rowAddrMid,
                                      midStride,
                                      lumaPlane[1][0] + rowAddr,
                                      lumaPlane[2][0] + rowAddr,
                                      lumaPlane[3][0] + rowAddr,
                                      m_reconPic->getWidth(),
                                      height,
                                      m_reconPic->getLumaMarginX(),
                                      m_reconPic->getLumaMarginY(),
                                      i,
                                      isLast);
            }
        }
    }

    if (!m_pool)
    {
        /* serial path for when no thread pool is present */
        for (int i = 0; i < 4; i++)
        {
            generateReferencePlane(i);
        }
    }
    else
    {
        m_workerCount = 0;
        m_finishedPlanes = 0;

        JobProvider::enqueue();
        for (int i = 0; i < 4; i++)
        {
            m_pool->pokeIdleThread();
        }

        m_completionEvent.wait();
        JobProvider::dequeue();
    }
    X265_FREE(m_intermediateValues);
}

bool MotionReference::findJob()
{
    /* Called by thread pool worker threads */

    if (m_workerCount < 4)
    {
        /* First four threads perform multi-vertical interpolations of intermediate buffers */
        int idx = ATOMIC_INC(&m_workerCount) - 1;
        if (idx < 4)
        {
            generateReferencePlane(idx);
            if (ATOMIC_INC(&m_finishedPlanes) == 4)
                m_completionEvent.trigger();
            return true;
        }
    }
    return false;
}

void MotionReference::generateReferencePlane(const int x)
{
    PPAScopeEvent(GenerateReferencePlanes);

    /* this function will be called by 4 threads, with x = 0, 1, 2, 3 */
    short* filteredBlockTmp = m_intermediateValues + x * m_intStride * (m_reconPic->getHeight() + s_tmpMarginY * 4);
    short* intPtr = filteredBlockTmp + s_intMarginY * m_intStride + s_intMarginX;

    /* the Y=0 plane was generated during horizontal interpolation */
    pixel *dstPtr1 = lumaPlane[x][1] - s_tmpMarginY * lumaStride - s_tmpMarginX;
    pixel *dstPtr2 = lumaPlane[x][2] - s_tmpMarginY * lumaStride - s_tmpMarginX;
    pixel *dstPtr3 = lumaPlane[x][3] - s_tmpMarginY * lumaStride - s_tmpMarginX;

    if (isWeighted)
    {
        primitives.filterVwghtd(intPtr, m_intStride, dstPtr1, dstPtr2, dstPtr3, lumaStride, m_filterWidth, m_filterHeight,
                                m_reconPic->m_lumaMarginX - s_tmpMarginX, m_reconPic->m_lumaMarginY - s_tmpMarginY,
                                weight, round, shift, offset);
    }
    else
    {
        int midStride = m_reconPic->getWidth();
        int height = g_maxCUHeight;
        for(int i = 0; i < m_reconPic->m_numCuInHeight; i++ )
        {
            int isLast = (i == m_reconPic->m_numCuInHeight - 1);
            int offs = (i == 0 ? 0 : 4);
            int rowAddr = (i * height - offs) * lumaStride;
            int rowAddrMid = (i * height - offs) * midStride;
            if (isLast)
            {
                int rem = m_reconPic->getHeight() % height;
                if (rem)
                    height = rem;
            }
            int proch = height + (i == 0 ? -4 : 0) + (isLast ? 4 : 0);

            if (x == 0)
            {
                primitives.filterRowV_0(m_reconPic->getLumaAddr() + rowAddr,
                                        lumaStride,
                                        lumaPlane[0][1] + rowAddr,
                                        lumaPlane[0][2] + rowAddr,
                                        lumaPlane[0][3] + rowAddr,
                                        m_reconPic->getWidth(),
                                        proch,
                                        m_reconPic->getLumaMarginX(),
                                        m_reconPic->getLumaMarginY(),
                                        i,
                                        isLast);
            }
            else
            {
                primitives.filterRowV_N(m_midBuf[x] + rowAddrMid,
                                        midStride,
                                        lumaPlane[x][1] + rowAddr,
                                        lumaPlane[x][2] + rowAddr,
                                        lumaPlane[x][3] + rowAddr,
                                        lumaStride,
                                        m_reconPic->getWidth(),
                                        proch,
                                        m_reconPic->getLumaMarginX(),
                                        m_reconPic->getLumaMarginY(),
                                        i,
                                        isLast);
            }
        }
    }
}
