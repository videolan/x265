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

#ifndef X265_FRAME_H
#define X265_FRAME_H
#define NUM_CU_PARTITIONS (1U << (g_maxFullDepth << 1))

#include "common.h"
#include "framedata.h"
#include "lowres.h"
#include "threading.h"

namespace x265 {
// private namespace

class Encoder;
class PicYuv;

class Frame
{
public:

    /* These two items will be NULL until the Frame begins to be encoded, at which point
     * it will be assigned a FrameData instance, which comes with a reconstructed image PicYuv */
    FrameData*        m_encData;
    PicYuv*           m_reconPicYuv;
    int               m_frameEncoderID;     // the ID of the FrameEncoder encoding this frame

    /* Data associated with x265_picture */
    PicYuv*           m_origPicYuv;
    int               m_poc;
    int64_t           m_pts;                // user provided presentation time stamp
    int64_t           m_reorderedPts;
    int64_t           m_dts;
    int32_t           m_forceqp;            // Force to use the qp specified in qp file
    x265_intra_data*  m_intraData;
    x265_inter_data*  m_interData;
    void*             m_userData;           // user provided pointer passed in with this picture

    Lowres            m_lowres;
    double*           m_qpaAq;              // adaptive quant offsets (from lookahead)
    bool              m_bChromaExtended;    // orig chroma planes motion extended for weight analysis

    /* Frame Parallelism - notification between FrameEncoders of available motion reference rows */
    ThreadSafeInteger m_reconRowCount;      // count of CTU rows completely reconstructed and extended for motion reference
    volatile uint32_t m_countRefEncoders;   // count of FrameEncoder threads monitoring m_reconRowCount

    Frame*            m_next;               // PicList doubly linked list pointers
    Frame*            m_prev;

    Frame();

    bool create(x265_param *param);
    bool allocEncodeData(x265_param *param);
    void destroy();

    /* TODO: all of this should be moved to RateControlEntry or FrameData */
    double*           m_rowDiagQp;
    double*           m_rowDiagQScale;
    uint32_t*         m_totalBitsPerCTU;
    uint32_t*         m_rowDiagSatd;
    uint32_t*         m_rowDiagIntraSatd;
    uint32_t*         m_rowEncodedBits;
    uint32_t*         m_numEncodedCusPerRow;
    uint32_t*         m_rowSatdForVbv;
    uint32_t*         m_cuCostsForVbv;
    uint32_t*         m_intraCuCostsForVbv;
    double*           m_qpaRc;
    double            m_avgQpRc;    // avg QP as decided by rate-control
    double            m_avgQpAq;    // avg QP as decided by AQ in addition to rate-control
    double            m_rateFactor; // calculated based on the Frame QP
    void reinit(x265_param *param);
};
}

#endif // ifndef X265_FRAME_H
