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

/** \file     TEncTop.h
    \brief    encoder class (header)
*/

#ifndef __TENCTOP__
#define __TENCTOP__

#include "x265.h"

// Include files
#include "TLibCommon/AccessUnit.h"
#include "TEncCfg.h"
#include "TEncAnalyze.h"
#include "threading.h"

namespace x265 {
class FrameEncoder;
class DPB;
struct Lookahead;
struct RateControl;
class ThreadPool;
}

//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// encoder class
class TEncTop : public TEncCfg
{
private:

    // picture
    Int                     m_pocLast;          ///< time index (POC)
    TComList<TComPic*>      m_freeList;

    // quality control
    TComScalingList         m_scalingList;      ///< quantization matrix information

    x265::ThreadPool*       m_threadPool;
    x265::Lookahead*        m_lookahead;
    x265::FrameEncoder*     m_frameEncoder;
    x265::DPB*              m_dpb;
    x265::RateControl*      m_rateControl;

    /* Collect statistics globally */
    x265::Lock              m_statLock;
    TEncAnalyze             m_analyzeAll;
    TEncAnalyze             m_analyzeI;
    TEncAnalyze             m_analyzeP;
    TEncAnalyze             m_analyzeB;

public:

    TEncTop();

    virtual ~TEncTop();

    Void create();
    Void destroy();
    Void init();

    Void xInitSPS(TComSPS *sps);
    Void xInitPPS(TComPPS *pps);
    Void xInitRPS(TComSPS *sps);

    int encode(Bool bEos, const x265_picture_t* pic, x265_picture_t *pic_out, AccessUnit& accessUnit);

    int getStreamHeaders(AccessUnit& accessUnit);

    Double printSummary();

    TComScalingList* getScalingList()       { return &m_scalingList; }

    void setThreadPool(x265::ThreadPool* p) { m_threadPool = p; }

    Void calculateHashAndPSNR(TComPic* pic, AccessUnit&);
};

//! \}

#endif // __TENCTOP__
