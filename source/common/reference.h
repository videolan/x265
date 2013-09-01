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
struct WpScalingParam;
typedef WpScalingParam wpScalingParam;

namespace x265 {
// private x265 namespace

struct ReferencePlanes
{
    ReferencePlanes() : isWeighted(false), isLowres(false) {}

    void setWeight(const wpScalingParam&);
    bool matchesWeight(const wpScalingParam&);

    /* indexed by [hpelx|qpelx][hpely|qpely] */
    pixel* lumaPlane[4][4];
    bool isWeighted;
    bool isLowres;
    int  lumaStride;
    int  weight;
    int  offset;
    int  shift;
    int  round;
};

class MotionReference : public ReferencePlanes, public JobProvider
{
public:

    MotionReference(TComPicYuv*, ThreadPool *, wpScalingParam* w = NULL);

    ~MotionReference();

    void generateReferencePlanes();

    MotionReference *m_next;
    TComPicYuv  *m_reconPic;

protected:

    bool findJob();
    void generateReferencePlane(const int idx);

    intptr_t     m_startPad;
    volatile int m_workerCount;
    volatile int m_finishedPlanes;
    Event        m_completionEvent;
    short       *m_intermediateValues;

    // Generate subpels for entire frame with a margin of tmpMargin
    static const int s_tmpMarginX = 4;
    static const int s_tmpMarginY = 4;

    static const int s_intMarginX = 0;    // Extra margin for horizontal filter
    static const int s_intMarginY = 4;

    int         m_intStride;
    intptr_t    m_extendOffset;
    intptr_t    m_offsetToLuma;
    int         m_filterWidth;
    int         m_filterHeight;
    
    short      *m_midBuf[4];  // 0: Full, 1:1/4, 2:2/4, 3:3/4

    MotionReference& operator =(const MotionReference&);
};
}

#endif // ifndef __REFERENCE__
