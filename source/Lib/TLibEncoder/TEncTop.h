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
#include "TLibCommon/TComList.h"
#include "TLibCommon/TComPrediction.h"
#include "TLibCommon/TComTrQuant.h"
#include "TLibCommon/AccessUnit.h"
#include "TEncCfg.h"
#include "TEncGOP.h"
#include "TEncSlice.h"
#include "TEncEntropy.h"
#include "TEncCavlc.h"
#include "TEncSbac.h"
#include "TEncSearch.h"
#include "TEncSampleAdaptiveOffset.h"
#include "TEncPreanalyzer.h"
#include "TEncRateCtrl.h"
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
    Int                     m_iPOCLast;                   ///< time index (POC)
    Int                     m_iNumPicRcvd;                ///< number of received pictures
    UInt                    m_uiNumAllPicCoded;           ///< number of coded pictures
    TComList<TComPic*>      m_cListPic;                   ///< dynamic list of pictures
    Int                     m_iNumSubstreams;             ///< # of WPP capable coding rows.

    // quality control
    TEncPreanalyzer         m_cPreanalyzer;               ///< image characteristics analyzer for TM5-step3-like adaptive QP
    TComScalingList         m_scalingList;                ///< quantization matrix information

    // SPS
    TComSPS                 m_cSPS;                       ///< SPS
    TComPPS                 m_cPPS;                       ///< PPS

    TEncRateCtrl            m_cRateCtrl;                  ///< Rate control class

    /* TODO: We keep a TEncSampleAdaptiveOffset instance in TEncTop only so we can
     * properly initialize new input pictures with SAO data buffers.  This should
     * be cleaned */
    TEncSampleAdaptiveOffset m_cEncSAO;

    // processing unit
    TEncGOP                 m_cGOPEncoder;                ///< GOP encoder
    x265::ThreadPool       *m_threadPool;

protected:

    Void  xGetNewPicBuffer(TComPic*& rpcPic);             ///< get picture buffer which will be processed
    Void  deletePicBuffer();
    Void  xInitSPS();                                     ///< initialize SPS from encoder options
    Void  xInitPPS();                                     ///< initialize PPS from encoder options
    Void  xInitRPS();                                     ///< initialize RPS from encoder options

public:

    TEncTop();
    virtual ~TEncTop();

    Void      create();
    Void      destroy();
    Void      init();

    // -------------------------------------------------------------------------------------------------------------------
    // member access functions
    // -------------------------------------------------------------------------------------------------------------------

    Int                     getNumSubstreams() { return m_iNumSubstreams; }

    TComList<TComPic*>*     getListPic() { return &m_cListPic; }

    TComSPS*                getSPS() { return &m_cSPS; }

    TComPPS*                getPPS() { return &m_cPPS; }

    TComScalingList*        getScalingList() { return &m_scalingList; }

    x265::ThreadPool*       getThreadPool() { return m_threadPool; }

    void                    setThreadPool(x265::ThreadPool* p) { m_threadPool = p; }

    Void selectReferencePictureSet(TComSlice* slice, Int POCCurr, Int GOPid);

    Int getReferencePictureSetIdxForSOP(TComSlice* slice, Int POCCurr, Int GOPid);

    TEncGOP*                getGOPEncoder()         { return &m_cGOPEncoder; }

    TEncRateCtrl*           getRateCtrl()           { return &m_cRateCtrl; }

    // -------------------------------------------------------------------------------------------------------------------
    // encoder function
    // -------------------------------------------------------------------------------------------------------------------

    /// encode several number of pictures until end-of-sequence
    int encode(Bool bEos, const x265_picture_t* pic, TComList<TComPicYuv*>& rcListPicYuvRecOut, std::list<AccessUnit>& accessUnitsOut);

    void printSummary() { m_cGOPEncoder.printOutSummary(m_uiNumAllPicCoded); }
};

//! \}

#endif // __TENCTOP__
