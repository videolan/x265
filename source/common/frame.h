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
#include "TLibCommon/TComPicYuv.h"
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

    TComPicYuv*       m_origPicYuv;

    Window            m_conformanceWindow;
    Window            m_defaultDisplayWindow;

    TComPicSym*       m_picSym;
    TComPicYuv*       m_reconPicYuv;
    int               m_POC;

    //** Frame Parallelism - notification between FrameEncoders of available motion reference rows **
    ThreadSafeInteger m_reconRowCount;      // count of CTU rows completely reconstructed and extended for motion reference
    volatile uint32_t m_countRefEncoders;   // count of FrameEncoder threads monitoring m_reconRowCount
    void*             m_userData;           // user provided pointer passed in with this picture

    int64_t           m_pts;                // user provided presentation time stamp
    int64_t           m_reorderedPts;
    int64_t           m_dts;

    Lowres            m_lowres;

    Frame*            m_next;               // PicList doubly linked list pointers
    Frame*            m_prev;

    bool              m_bChromaPlanesExtended; // orig chroma planes motion extended for weightp analysis

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

    x265_intra_data*  m_intraData;  // intra analysis information
    x265_inter_data*  m_interData;  // inter analysis information

    Frame();
    ~Frame() {}

    bool        create(x265_param *param, Window& display, Window& conformance);
    bool        allocPicSym(x265_param *param);
    void        reinit(x265_param *param);
    void        destroy();

    int         getPOC()                   { return m_POC; }

    Window&     getConformanceWindow()     { return m_conformanceWindow; }

    Window&     getDefDisplayWindow()      { return m_defaultDisplayWindow; }

    TComPicYuv* getPicYuvOrg()             { return m_origPicYuv; }

    TComPicYuv* getPicYuvRec()             { return m_reconPicYuv; }

    int         getStride()                { return m_origPicYuv->getStride(); }

    int         getCStride()               { return m_origPicYuv->getCStride(); }

    /* Reflector methods for data stored in m_picSym */
    TComPicSym* getPicSym()                { return m_picSym; }

    TComDataCU* getCU(uint32_t cuAddr)     { return m_picSym->getCU(cuAddr); }

    uint32_t    getNumCUsInFrame() const   { return m_picSym->getNumberOfCUsInFrame(); }

    uint32_t    getNumPartInCUSize() const { return m_picSym->getNumPartInCUSize(); }

    uint32_t    getFrameWidthInCU() const  { return m_picSym->getFrameWidthInCU(); }

    uint32_t    getFrameHeightInCU() const { return m_picSym->getFrameHeightInCU(); }
};
}

#endif // ifndef X265_FRAME_H
