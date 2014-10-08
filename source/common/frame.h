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
#include "TLibCommon/TComPicSym.h"
#include "picyuv.h"
#include "lowres.h"
#include "threading.h"
#include "md5.h"
#include "sei.h"

namespace x265 {
// private namespace

class Encoder;

class Frame
{
public:

    PicYuv*           m_origPicYuv;

    TComPicSym*       m_picSym;
    PicYuv*           m_reconPicYuv;
    int               m_POC;

    bool              m_bChromaPlanesExtended; // orig chroma planes motion extended for weightp analysis

    //** Frame Parallelism - notification between FrameEncoders of available motion reference rows **
    ThreadSafeInteger m_reconRowCount;      // count of CTU rows completely reconstructed and extended for motion reference
    volatile uint32_t m_countRefEncoders;   // count of FrameEncoder threads monitoring m_reconRowCount
    void*             m_userData;           // user provided pointer passed in with this picture

    int64_t           m_pts;                // user provided presentation time stamp
    int64_t           m_reorderedPts;
    int64_t           m_dts;

    Lowres            m_lowres;

    Frame*            m_next;       // PicList doubly linked list pointers
    Frame*            m_prev;

    x265_intra_data*  m_intraData;  // intra analysis information
    x265_inter_data*  m_interData;  // inter analysis information

    /* TODO: much of this data can be moved to RCE */
    double*           m_rowDiagQp;
    double*           m_rowDiagQScale;
    uint32_t*         m_rowDiagSatd;
    uint32_t*         m_rowDiagIntraSatd;
    uint32_t*         m_rowEncodedBits;
    uint32_t*         m_numEncodedCusPerRow;
    uint32_t*         m_rowSatdForVbv;
    uint32_t*         m_cuCostsForVbv;
    uint32_t*         m_intraCuCostsForVbv;
    double*           m_qpaAq;
    double*           m_qpaRc;
    double            m_avgQpRc;    // avg QP as decided by ratecontrol
    double            m_avgQpAq;    // avg QP as decided by AQ in addition to ratecontrol
    double            m_rateFactor; // calculated based on the Frame QP
    int32_t           m_forceqp;    // Force to use the qp specified in qp file

    Frame();

    bool create(x265_param *param);
    bool allocPicSym(x265_param *param);
    void reinit(x265_param *param);
    void destroy();
};
}

#endif // ifndef X265_FRAME_H
