/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Chung Shin Yee <shinyee@multicorewareinc.com>
 *          Min Chen <chenm003@163.com>
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

#ifndef __FRAMEFILTER__
#define __FRAMEFILTER__

#include "TLibCommon/TComPic.h"
#include "TLibCommon/TComLoopFilter.h"
#include "TLibEncoder/TEncSampleAdaptiveOffset.h"

#include "threading.h"
#include "wavefront.h"

class TEncTop;

namespace x265 {
// private x265 namespace

class ThreadPool;

// Manages the wave-front processing of a single frame loopfilter
class FrameFilter : public JobProvider
{
public:

    FrameFilter(ThreadPool *);

    virtual ~FrameFilter() {}

    void init(TEncTop *top, int numRows);

    void destroy();

    void start(TComPic *pic);
    void end();

    void wait();

    void enqueueRow(int row);
    bool findJob();

    void processRow(int row);

protected:

    TEncCfg*            m_cfg;
    TComPic*            m_pic;
    Lock                m_lock;

public:

    TComLoopFilter              m_loopFilter;
    TEncSampleAdaptiveOffset    m_sao;
    int                         m_numRows;

    // TODO: if you want thread priority logic, add col here
    volatile int                row_ready;
    volatile int                row_done;

    Event                       m_completionEvent;
};

}

#endif // ifndef __FRAMEFILTER__
