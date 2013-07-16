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
#include "threading.h"
#include "frameencoder.h"

//! \ingroup TLibEncoder
//! \{

class TEncTop;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// GOP encoder class
class TEncGOP : public x265::Thread
{
private:

    TEncTop*                m_top;
    TEncCfg*                m_cfg;
    TEncRateCtrl*           m_rateControl;
    x265::FrameEncoder*     m_frameEncoders;
    TComList<TComPic*>      m_picList;         ///< dynamic list of input pictures
    x265_picture_t         *m_recon;
    std::list<AccessUnit>   m_accessUnits;
    Int                     m_startPOC;
    Int                     m_batchSize;
    Bool                    m_threadActive;
    x265::Lock              m_inputLock;
    x265::Lock              m_outputLock;

    //  Data
    UInt                    m_numLongTermRefPicSPS;
    UInt                    m_ltRefPicPocLsbSps[33];
    Bool                    m_ltRefPicUsedByCurrPicFlag[33];
    Int                     m_lastIDR;
    UInt                    m_totalCoded;

    SEIWriter               m_seiWriter;

    // clean decoding refresh
    Bool                    m_bRefreshPending;
    Int                     m_pocCRA;
    UInt                    m_lastBPSEI;
    UInt                    m_tl0Idx;
    UInt                    m_rapIdx;

    TComSPS                 m_sps;
    TComPPS                 m_pps;

public:

    TEncGOP();

    virtual ~TEncGOP();

    Void  create();
    Void  destroy();
    Void  init(TEncTop* top);

    void addPicture(Int poc, const x265_picture_t *pic);

    void processKeyframeInterval(Int pocLast, Int numFrames);

    // returns count of returned pictures
    int getOutputs(x265_picture_t**, std::list<AccessUnit>& accessUnitsOut);

protected:

    void threadMain();

    Void compressGOP(Int pocLast, Int numPicRcvd);

    Void selectReferencePictureSet(TComSlice* slice, Int curPoc, Int gopID);

    Int getReferencePictureSetIdxForSOP(Int pocCur, Int GOPid);

    NalUnitType getNalUnitType(Int curPoc, Int lastIdr);

    Void xCalculateAddPSNR(TComPic* pic, TComPicYuv* recon, const AccessUnit&);

    SEIActiveParameterSets* xCreateSEIActiveParameterSets(TComSPS *sps);
    SEIDisplayOrientation*  xCreateSEIDisplayOrientation();

    Void arrangeLongtermPicturesInRPS(TComSlice *);

    Void xAttachSliceDataToNalUnit(TEncEntropy* entropyCoder, OutputNALUnit& nalu, TComOutputBitstream*& outBitstreamRedirect);

    Int  xGetFirstSeiLocation(AccessUnit &accessUnit);
}; // END CLASS DEFINITION TEncGOP

//! \}

#endif // __TENCGOP__
