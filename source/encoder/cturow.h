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
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#ifndef X265_CTUROW_H
#define X265_CTUROW_H

#include "common.h"
#include "frame.h"

#include "analysis.h"

#include "rdcost.h"
#include "entropy.h"

namespace x265 {
// private x265 namespace

class Encoder;

struct ThreadLocalData
{
    Analysis    m_cuCoder;

    // NOTE: the maximum LCU 64x64 have 256 partitions
    bool        m_edgeFilter[256];
    uint8_t     m_blockingStrength[256];

    void init(Encoder&);
    ~ThreadLocalData();
};

/* manages the state of encoding one row of CTU blocks.  When
 * WPP is active, several rows will be simultaneously encoded.
 * When WPP is inactive, only one CTURow instance is used. */
class CTURow
{
public:

    Entropy         m_entropyCoder;
    Entropy         m_bufferEntropyCoder;  /* store context for next row */
    Entropy         m_rdEntropyCoders[NUM_FULL_DEPTH][CI_NUM];

    // to compute stats for 2 pass
    double          m_iCuCnt;
    double          m_pCuCnt;
    double          m_skipCuCnt;

    void init(Entropy& initContext)
    {
        m_active = 0;
        m_completed = 0;
        m_busy = false;

        m_entropyCoder.load(initContext);

        // Note: Reset status to avoid frame parallelism output mistake on different thread number
        for (uint32_t depth = 0; depth <= g_maxFullDepth; depth++)
            for (int ciIdx = 0; ciIdx < CI_NUM; ciIdx++)
                m_rdEntropyCoders[depth][ciIdx].load(initContext);

        m_iCuCnt = m_pCuCnt = m_skipCuCnt = 0;
    }

    void processCU(TComDataCU *cu, Entropy *bufferSBac, ThreadLocalData& tld, bool bSaveCabac);

    /* Threading variables */

    /* This lock must be acquired when reading or writing m_active or m_busy */
    Lock                m_lock;

    /* row is ready to run, has no neighbor dependencies. The row may have
     * external dependencies (reference frame pixels) that prevent it from being
     * processed, so it may stay with m_active=true for some time before it is
     * encoded by a worker thread. */
    volatile bool       m_active;

    /* row is being processed by a worker thread.  This flag is only true when a
     * worker thread is within the context of FrameEncoder::processRow(). This
     * flag is used to detect multiple possible wavefront problems. */
    volatile bool       m_busy;

    /* count of completed CUs in this row */
    volatile uint32_t   m_completed;
};
}

#endif // ifndef X265_CTUROW_H
