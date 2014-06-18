/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2013, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef X265_TCOMPIC_H
#define X265_TCOMPIC_H


#include "common.h"
#include "TComPicSym.h"
#include "TComPicYuv.h"
#include "lowres.h"
#include "threading.h"
#include "md5.h"
#include "sei.h"

namespace x265 {
// private namespace

class Encoder;

class TComPic
{
private:

    TComPicSym*       m_picSym;
    TComPicYuv*       m_origPicYuv;
    TComPicYuv*       m_reconPicYuv;

    Window            m_conformanceWindow;
    Window            m_defaultDisplayWindow;

    bool              m_bIsLongTerm;

public:

    //** Frame Parallelism - notification between FrameEncoders of available motion reference rows **
    ThreadSafeInteger m_reconRowCount;      // count of CTU rows completely reconstructed and extended for motion reference
    volatile uint32_t m_countRefEncoders;   // count of FrameEncoder threads monitoring m_reconRowCount
    void*             m_userData;           // user provided pointer passed in with this picture

    int64_t           m_pts;                // user provided presentation time stamp
    int64_t           m_reorderedPts;
    int64_t           m_dts;

    Lowres            m_lowres;

    TComPic*          m_next;               // PicList doubly linked list pointers
    TComPic*          m_prev;

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

    TComPic();
    virtual ~TComPic();

    bool        create(Encoder* top);
    bool        allocPicSym(Encoder* top);
    void        reinit(Encoder* top);
    void        destroy();

    bool        getIsLongTerm()           { return m_bIsLongTerm; }

    void        setIsLongTerm(bool lt)    { m_bIsLongTerm = lt; }

    TComPicSym* getPicSym()             { return m_picSym; }

    TComSlice*  getSlice()              { return m_picSym->getSlice(); }

    int         getPOC()                { return m_picSym->getSlice()->getPOC(); }

    TComDataCU* getCU(uint32_t cuAddr)  { return m_picSym->getCU(cuAddr); }

    TComPicYuv* getPicYuvOrg()          { return m_origPicYuv; }

    TComPicYuv* getPicYuvRec()          { return m_reconPicYuv; }

    uint32_t    getNumCUsInFrame() const { return m_picSym->getNumberOfCUsInFrame(); }

    uint32_t    getNumPartInCUSize() const { return m_picSym->getNumPartInCUSize(); }

    uint32_t    getNumPartInCU() const  { return m_picSym->getNumPartition(); }

    uint32_t    getFrameWidthInCU() const { return m_picSym->getFrameWidthInCU(); }

    uint32_t    getFrameHeightInCU() const { return m_picSym->getFrameHeightInCU(); }

    uint32_t    getUnitSize() const     { return m_picSym->getUnitSize(); }

    uint32_t    getLog2UnitSize() const   { return m_picSym->getLog2UnitSize(); }

    int         getStride()             { return m_reconPicYuv->getStride(); }

    int         getCStride()            { return m_reconPicYuv->getCStride(); }

    Window&     getConformanceWindow()  { return m_conformanceWindow; }

    Window&     getDefDisplayWindow()   { return m_defaultDisplayWindow; }
};
}

#endif // ifndef X265_TCOMPIC_H
