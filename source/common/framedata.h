/*****************************************************************************
* Copyright (C) 2013 x265 project
*
* Author: Steve Borho <steve@borho.org>
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

#ifndef X265_FRAMEDATA_H
#define X265_FRAMEDATA_H

#include "common.h"
#include "slice.h"
#include "TLibCommon/TComDataCU.h"

namespace x265 {
// private namespace

class PicYuv;

/* Per-frame data that is used during encodes and referenced while the picture
 * is available for reference. A FrameData instance is attached to a Frame as it
 * comes out of the lookahead. Frames which are not being encoded do not have a
 * FrameData instance. These instances are re-used once the encoded frame has
 * no active references. They hold the Slice instance and the 'official' CTU
 * data structures. They are maintained in a free-list pool along together with
 * a reconstructed image PicYuv in order to conserve memory. */
class FrameData
{
public:

    Slice*         m_slice;
    SAOParam*      m_saoParam;

    FrameData*     m_freeListNext;
    PicYuv*        m_reconPicYuv;
    bool           m_bHasReferences;   /* used during DPB/RPS updates */

    DataCUMemPool  m_cuMemPool;
    MVFieldMemPool m_mvFieldMemPool;
    TComDataCU*    m_picCTU;

    uint32_t       m_numPartitions;    /* based on g_maxFullDepth, could be CU static */
    uint32_t       m_numPartInCUSize;  /* based on g_maxFullDepth, could be CU static */
    uint32_t       m_numCUsInFrame;    /* based on param, should perhaps be in Frame */

    /* Rate control data used during encode and by references */
    uint32_t*      m_totalBitsPerCTU;
    uint32_t*      m_cuCostsForVbv;
    uint32_t*      m_intraCuCostsForVbv;
    uint32_t*      m_numEncodedCusPerRow;
    uint32_t*      m_rowDiagSatd;
    uint32_t*      m_rowDiagIntraSatd;
    uint32_t*      m_rowEncodedBits;
    uint32_t*      m_rowSatdForVbv;
    double*        m_rowDiagQp;
    double*        m_rowDiagQScale;
    double*        m_qpaRc;
    double*        m_qpaAq;
    double         m_avgQpRc;    /* avg QP as decided by rate-control */
    double         m_avgQpAq;    /* avg QP as decided by AQ in addition to rate-control */
    double         m_rateFactor; /* calculated based on the Frame QP */

    FrameData();

    bool create(x265_param *param);
    void reinit(x265_param *param);
    void destroy();

    TComDataCU* getPicCTU(uint32_t ctuAddr) { return &m_picCTU[ctuAddr]; }
};
}

#endif // ifndef X265_FRAMEDATA_H
