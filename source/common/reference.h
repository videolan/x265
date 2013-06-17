/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
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

#ifndef __REFERENCE__
#define __REFERENCE__

#include "primitives.h"
#include "threading.h"
#include "threadpool.h"

class TComPicYuv;

namespace x265 {
// private x265 namespace

class MotionReference : public JobProvider
{
public:

    MotionReference(TComPicYuv*, ThreadPool *);

    ~MotionReference();

    void generateReferencePlanes();

    /* indexed by [hpelx|qpelx][hpely|qpely] */
    pixel* m_lumaPlane[4][4];

    int m_lumaStride;

    MotionReference *m_next;

protected:

    bool FindJob();
    void generateReferencePlane(int idx);

    intptr_t     m_startPad;
    TComPicYuv  *m_reconPic;
    volatile int m_workerCount;
    volatile int m_finishedPlanes;
    Event        m_completionEvent;

    // Generate subpels for entire frame with a margin of tmpMargin
    static const int s_tmpMarginX = 4;
    static const int s_tmpMarginY = 4;

    int         m_intStride;
    intptr_t    m_extendOffset;
    intptr_t    m_offsetToLuma;
    int         m_filterWidth;
    int         m_filterHeight;
    short      *m_intermediateValues;

    MotionReference& operator =(const MotionReference&);
};

}

#endif // ifndef __REFERENCE__
