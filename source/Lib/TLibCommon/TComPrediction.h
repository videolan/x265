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

/** \file     TComPrediction.h
    \brief    prediction class (header)
*/

#ifndef X265_TCOMPREDICTION_H
#define X265_TCOMPREDICTION_H

// Include files
#include "TComPic.h"
#include "TComMotionInfo.h"
#include "TComPattern.h"
#include "TComTrQuant.h"
#include "TComWeightPrediction.h"
#include "TShortYUV.h"

namespace x265 {
// private namespace

struct ReferencePlanes;

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// prediction class
class TComPrediction : public TComWeightPrediction
{
protected:

    // references sample for IntraPrediction
    TComYuv   m_predYuv[2];
    TShortYUV m_predShortYuv[2];
    TComYuv   m_predTempYuv;

    int16_t*  m_immedVals;
    int       m_hChromaShift;
    int       m_vChromaShift;

    // motion compensation functions
    void xPredInterUni(TComDataCU* cu, uint32_t partAddr, int width, int height, int picList, TComYuv* outPredYuv, bool bLuma, bool bChroma);
    void xPredInterUni(TComDataCU* cu, uint32_t partAddr, int width, int height, int picList, TShortYUV* outPredYuv, bool bLuma, bool bChroma);
    void xPredInterLumaBlk(TComDataCU *cu, TComPicYuv *refPic, uint32_t partAddr, MV *mv, int width, int height, TComYuv *dstPic);
    void xPredInterLumaBlk(TComDataCU *cu, TComPicYuv *refPic, uint32_t partAddr, MV *mv, int width, int height, TShortYUV *dstPic);
    void xPredInterChromaBlk(TComDataCU *cu, TComPicYuv *refPic, uint32_t partAddr, MV *mv, int width, int height, TComYuv *dstPic);
    void xPredInterChromaBlk(TComDataCU *cu, TComPicYuv *refPic, uint32_t partAddr, MV *mv, int width, int height, TShortYUV *dstPic);

    void xPredInterBi(TComDataCU* cu, uint32_t partAddr, int width, int height, TComYuv*& outPredYuv, bool bLuma, bool bChroma);
    void xWeightedAverage(TComYuv* srcYuv0, TComYuv* srcYuv1, int refIdx0, int refIdx1, uint32_t partAddr, int width, int height, TComYuv* outDstYuv, bool bLuma, bool bChroma);

    void xGetLLSPrediction(TComPattern* pcPattern, int* src0, int srcstride, pixel* dst0, int dststride, uint32_t width, uint32_t height, uint32_t ext0);

    bool xCheckIdenticalMotion(TComDataCU* cu, uint32_t PartAddr);

public:

    pixel*    m_predBuf;
    pixel*    m_refAbove;
    pixel*    m_refAboveFlt;
    pixel*    m_refLeft;
    pixel*    m_refLeftFlt;
    int       m_predBufStride;
    int       m_predBufHeight;

    TComPrediction();
    virtual ~TComPrediction();

    void initTempBuff(int csp);

    // inter
    void motionCompensation(TComDataCU* cu, TComYuv* predYuv, int picList = REF_PIC_LIST_X, int partIdx = -1, bool bLuma = true, bool bChroma = true);

    // Angular Intra
    void predIntraLumaAng(uint32_t dirMode, pixel* pred, intptr_t stride, int width);
    void predIntraChromaAng(pixel* src, uint32_t dirMode, pixel* pred, intptr_t stride, int width, int height, int chFmt);
    bool filteringIntraReferenceSamples(uint32_t dirMode, uint32_t width);
};
}
//! \}

#endif // ifndef X265_TCOMPREDICTION_H
