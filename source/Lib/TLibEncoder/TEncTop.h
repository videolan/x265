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
#include "TEncGOP.h"
#include "TEncRateCtrl.h"
#include "TEncAnalyze.h"
#include "threading.h"
#include "threadpool.h"

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
    Int                     m_picsQueued;       ///< number of received pictures
    Int                     m_picsEncoded;      ///< number of coded pictures
    TComList<TComPic*>      m_picList;          ///< dynamic list of input pictures
    x265_picture_t         *m_recon;

    // quality control
    TComScalingList         m_scalingList;      ///< quantization matrix information

    TEncRateCtrl            m_cRateCtrl;        ///< Rate control class

    TEncGOP*                m_GOPEncoder;
    x265::ThreadPool*       m_threadPool;

public:

    /* Collect statistics globally */
    x265::Lock  m_statLock;
    TEncAnalyze m_gcAnalyzeAll;
    TEncAnalyze m_gcAnalyzeI;
    TEncAnalyze m_gcAnalyzeP;
    TEncAnalyze m_gcAnalyzeB;

public:

    TEncTop();

    virtual ~TEncTop();

    Void create();
    Void destroy();
    Void init();

    TComScalingList* getScalingList()   { return &m_scalingList; }

    TEncRateCtrl* getRateCtrl()      { return &m_cRateCtrl; }

    void setThreadPool(x265::ThreadPool* p) { m_threadPool = p; }

    Void xInitSPS(TComSPS *pcSPS);
    Void xInitPPS(TComPPS *pcPPS);
    Void xInitRPS(TComSPS *pcSPS);

    int encode(Bool bEos, const x265_picture_t* pic, x265_picture_t **pic_out, std::list<AccessUnit>& accessUnitsOut);

    int getStreamHeaders(std::list<AccessUnit>& accessUnitsOut);

    Double printSummary();

protected:

    // TODO: will eventually be part of pre-lookahead
    void addPicture(const x265_picture_t *pic);
};

//! \}

#endif // __TENCTOP__
