/*****************************************************************************
* Copyright (C) 2013-2020 MulticoreWare, Inc
*
* Author: Steve Borho <steve@borho.org>
*         Min Chen <chenm003@163.com>
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

#include "common.h"
#include "lowres.h"
#include "threading.h"
#include "temporalfilter.h"

namespace X265_NS {
// private namespace

class FrameData;
class PicYuv;
struct SPS;

#define IS_REFERENCED(frame) (frame->m_lowres.sliceType != X265_TYPE_B)

/* Ratecontrol statistics */
struct RcStats
{
    double   qpaRc;
    double   qpAq;
    double   qRceq;
    double   qpNoVbv;
    double   newQScale;
    double   iCuCount;
    double   pCuCount;
    double   skipCuCount;
    double   qScale;
    double   cumulativePQp;
    double   cumulativePNorm;
    double   lastQScaleFor[3];
    int      mvBits;
    int      miscBits;
    int      coeffBits;
    int      poc;
    int      encodeOrder;
    int      sliceType;
    int      keptAsRef;
    double   wantedBitsWindow;
    double   cplxrSum;
    double   shortTermCplxSum;
    double   shortTermCplxCount;
    int64_t  totalBits;
    int64_t  encodedBits;
    double   coeff[4];
    double   count[4];
    double   offset[4];
    double   bufferFillFinal;
    int64_t  currentSatd;
};

class Frame
{
public:

    /* These two items will be NULL until the Frame begins to be encoded, at which point
     * it will be assigned a FrameData instance, which comes with a reconstructed image PicYuv */
    FrameData*             m_encData;
    PicYuv*                m_reconPic;

    /* Data associated with x265_picture */
    PicYuv*                m_fencPic;
    PicYuv*                m_fencPicSubsampled2;
    PicYuv*                m_fencPicSubsampled4;

    int                    m_poc;
    int                    m_encodeOrder;
    int                    m_gopOffset;
    int64_t                m_pts;                // user provided presentation time stamp
    int64_t                m_reorderedPts;
    int64_t                m_dts;
    int32_t                m_forceqp;            // Force to use the qp specified in qp file
    void*                  m_userData;           // user provided pointer passed in with this picture

    Lowres                 m_lowres;
    bool                   m_lowresInit;         // lowres init complete (pre-analysis)
    bool                   m_bChromaExtended;    // orig chroma planes motion extended for weight analysis
    bool                   m_reconfigureRc;

    float*                 m_quantOffsets;       // points to quantOffsets in x265_picture
    x265_sei               m_userSEI;
    uint32_t               m_picStruct;          // picture structure SEI message
    x265_dolby_vision_rpu  m_rpu;

    /* Frame Parallelism - notification between FrameEncoders of available motion reference rows */
    ThreadSafeInteger*     m_reconRowFlag;       // flag of CTU rows completely reconstructed and extended for motion reference
    ThreadSafeInteger*     m_reconColCount;      // count of CTU cols completely reconstructed and extended for motion reference
    int32_t                m_numRows;
    volatile uint32_t      m_countRefEncoders;   // count of FrameEncoder threads monitoring m_reconRowCount

    Frame*                 m_next;               // PicList doubly linked list pointers
    Frame*                 m_prev;
    x265_param*            m_param;              // Points to the latest param set for the frame.
    x265_analysis_data     m_analysisData;
    RcStats*               m_rcData;

    Event                  m_copyMVType;

    x265_ctu_info_t**      m_ctuInfo;
    Event                  m_copied;
    int*                   m_prevCtuInfoChange;
    int64_t                m_encodeStartTime;

    uint8_t**              m_addOnDepth;
    uint8_t**              m_addOnCtuInfo;
    int**                  m_addOnPrevChange;

    /* Average feature values of frames being considered for classification */
    uint64_t*              m_classifyRd;
    uint64_t*              m_classifyVariance;
    uint32_t*              m_classifyCount;

    bool                   m_classifyFrame;
    int                    m_fieldNum;

    /*MCSTF*/
    TemporalFilter*        m_mcstf;
    int                    m_refPicCnt[2];
    Frame*                 m_nextMCSTF;           // PicList doubly linked list pointers
    Frame*                 m_prevMCSTF;
    int*                   m_isSubSampled;

    /* aq-mode 4 : Gaussian, edge and theta frames for edge information */
    pixel*                 m_edgePic;
    pixel*                 m_gaussianPic;
    pixel*                 m_thetaPic;

    /* edge bit plane for rskips 2 and 3 */
    pixel*                 m_edgeBitPlane;
    pixel*                 m_edgeBitPic;

    int                    m_isInsideWindow;

    /*Frame's temporal layer info*/
    uint8_t                m_tempLayer;
    int8_t                 m_gopId;
    bool                   m_sameLayerRefPic;

    Frame();

    bool create(x265_param *param, float* quantOffsets);
    bool createSubSample();
    bool allocEncodeData(x265_param *param, const SPS& sps);
    void reinit(const SPS& sps);
    void destroy();
};
}

#endif // ifndef X265_FRAME_H
