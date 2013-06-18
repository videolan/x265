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

/** \file     TEncGOP.h
    \brief    GOP encoder class (header)
*/

#ifndef __TENCGOP__
#define __TENCGOP__

#include "TLibCommon/TComList.h"
#include "TLibCommon/TComPic.h"
#include "TLibCommon/TComBitCounter.h"
#include "TLibCommon/TComLoopFilter.h"
#include "TLibCommon/AccessUnit.h"
#include "TEncSampleAdaptiveOffset.h"
#include "TEncSlice.h"
#include "TEncEntropy.h"
#include "TEncCavlc.h"
#include "TEncSbac.h"
#include "SEIwrite.h"

#include "TEncAnalyze.h"
#include "TEncRateCtrl.h"
#include "wavefront.h"

//! \ingroup TLibEncoder
//! \{

class TEncTop;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// GOP encoder class
class TEncGOP
{
private:

    //  Data
    UInt                    m_numLongTermRefPicSPS;
    UInt                    m_ltRefPicPocLsbSps[33];
    Bool                    m_ltRefPicUsedByCurrPicFlag[33];
    Int                     m_iLastIDR;
    Int                     m_iGopSize;

    //  Access channel
    TEncTop*                m_pcEncTop;
    TEncCfg*                m_pcCfg;
    TEncRateCtrl*           m_pcRateCtrl;
    x265::EncodeFrame*      m_cFrameEncoders;

    SEIWriter               m_seiWriter;

    // clean decoding refresh
    Bool                    m_bRefreshPending;
    Int                     m_pocCRA;

    UInt                    m_lastBPSEI;
    UInt                    m_totalCoded;
    UInt                    m_cpbRemovalDelay;
    UInt                    m_tl0Idx;
    UInt                    m_rapIdx;
    Bool                    m_activeParameterSetSEIPresentInAU;
    Bool                    m_bufferingPeriodSEIPresentInAU;
    Bool                    m_pictureTimingSEIPresentInAU;
    Bool                    m_nestedBufferingPeriodSEIPresentInAU;
    Bool                    m_nestedPictureTimingSEIPresentInAU;

public:

    TEncGOP();

    virtual ~TEncGOP();

    Void  create();
    Void  destroy();
    Void  init(TEncTop* pcTEncTop);

    Void  compressGOP(Int iPOCLast, Int iNumPicRcvd, TComList<TComPic*>& rcListPic, TComList<TComPicYuv*>& rcListPicYuvRec, std::list<AccessUnit>& accessUnitsInGOP);

    Void  printOutSummary(UInt uiNumAllPicCoded);

protected:

    NalUnitType        getNalUnitType(Int pocCurr, Int lastIdr);
    x265::EncodeFrame* getFrameEncoder(UInt i) { return &m_cFrameEncoders[i]; }

    Void   xCalculateAddPSNR(TComPic* pcPic, TComPicYuv* pcPicD, const AccessUnit&);
    Double xCalculateRVM();

    SEIActiveParameterSets* xCreateSEIActiveParameterSets(TComSPS *sps);
    SEIDisplayOrientation*  xCreateSEIDisplayOrientation();

    Void arrangeLongtermPicturesInRPS(TComSlice *, TComList<TComPic*>&);

    Void xAttachSliceDataToNalUnit(TEncEntropy* pcEntropyCoder, OutputNALUnit& rNalu, TComOutputBitstream*& rpcBitstreamRedirect);
    Void xCreateLeadingSEIMessages(TEncEntropy *pcEntropyCoder, AccessUnit &accessUnit, TComSPS *sps);
    Int  xGetFirstSeiLocation(AccessUnit &accessUnit);
    Void xResetNonNestedSEIPresentFlags()
    {
        m_activeParameterSetSEIPresentInAU = false;
        m_bufferingPeriodSEIPresentInAU    = false;
        m_pictureTimingSEIPresentInAU      = false;
    }

    Void xResetNestedSEIPresentFlags()
    {
        m_nestedBufferingPeriodSEIPresentInAU    = false;
        m_nestedPictureTimingSEIPresentInAU      = false;
    }

    Void dblMetric(TComPic* pcPic, UInt uiNumSlices);
}; // END CLASS DEFINITION TEncGOP

// ====================================================================================================================
// Enumeration
// ====================================================================================================================
enum PROCESSING_STATE
{
    EXECUTE_INLOOPFILTER,
    ENCODE_SLICE
};

enum SCALING_LIST_PARAMETER
{
    SCALING_LIST_OFF,
    SCALING_LIST_DEFAULT,
    SCALING_LIST_FILE_READ
};

//! \}

#endif // __TENCGOP__
