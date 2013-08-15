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
#include "TEncEntropy.h"
#include "TEncCavlc.h"
#include "TEncSbac.h"
#include "SEIwrite.h"

#include "TEncAnalyze.h"
#include "threading.h"
#include "frameencoder.h"

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

    TEncTop*                m_top;
    TEncCfg*                m_cfg;

    SEIWriter               m_seiWriter;
    TComSPS                 m_sps;
    TComPPS                 m_pps;

    /* TODO: Split these DPB fields into a new class */
    Int                     m_lastIDR;
    UInt                    m_totalCoded;
    // clean decoding refresh
    Bool                    m_bRefreshPending;
    Int                     m_pocCRA;
    UInt                    m_lastBPSEI;
    UInt                    m_tl0Idx;
    UInt                    m_rapIdx;
    /* end DPB fields */

public:

    x265::FrameEncoder*     m_frameEncoder;

    TEncGOP();
    ~TEncGOP() {}

    Void create() {}
    Void destroy();
    Void init(TEncTop* top);

    /* Establish references, manage DPB and RPS, runs in API thread context */
    Void prepareEncode(TComPic *pic, TComList<TComPic*> picList);

    /* analyze / compress frame, can be run in parallel within reference constraints */
    Void compressFrame(TComPic *pic, AccessUnit& accessUnitOut);

    int getStreamHeaders(AccessUnit& accessUnitOut);

protected:

    /* SEI/ NAL / Encode methods */
    Void xCalculateAddPSNR(TComPic* pic, TComPicYuv* recon, const AccessUnit&);

    SEIActiveParameterSets* xCreateSEIActiveParameterSets(TComSPS *sps);

    SEIDisplayOrientation*  xCreateSEIDisplayOrientation();

    Void xAttachSliceDataToNalUnit(TEncEntropy* entropyCoder, OutputNALUnit& nalu, TComOutputBitstream* outBitstreamRedirect);

    Int  xGetFirstSeiLocation(AccessUnit &accessUnit);

protected:

    /* RPS / DPB methods */
    Void selectReferencePictureSet(TComSlice* slice, Int curPoc, Int gopID);

    Int getReferencePictureSetIdxForSOP(Int pocCur, Int GOPid);

    NalUnitType getNalUnitType(Int curPoc, Int lastIdr);

    Void arrangeLongtermPicturesInRPS(TComSlice *, TComList<TComPic*> picList);
};

//! \}

#endif // __TENCGOP__
