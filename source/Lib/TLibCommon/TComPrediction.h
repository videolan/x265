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

#ifndef __TCOMPREDICTION__
#define __TCOMPREDICTION__

// Include files
#include "TComPic.h"
#include "TComMotionInfo.h"
#include "TComPattern.h"
#include "TComTrQuant.h"
#include "TComWeightPrediction.h"
#include "TShortYUV.h"
#include "reference.h"

namespace x265 {
// private namespace

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// prediction class
class TComPrediction : public TComWeightPrediction
{
protected:

    Pel*      m_predBuf;
    Pel*      m_predAllAngsBuf;
    int       m_predBufStride;
    int       m_predBufHeight;

    // references sample for IntraPrediction
    TComYuv   m_predYuv[2];
    TShortYUV m_predShortYuv[2];
    TComYuv   m_predTempYuv;

    /* This holds final interpolated pixel values (0-255). Hence memory is stored as Pel. */
    TComYuv   m_filteredBlock[4][4];

    /* This holds intermediate values for filtering operations which need to maintain short precision */
    TShortYUV m_filteredBlockTmp[4];

    short*    m_immedVals;
    Pel*      m_lumaRecBuffer; ///< array for down-sampled reconstructed luma sample
    int       m_lumaRecStride; ///< stride of m_lumaRecBuffer

    // motion compensation functions
    void xPredInterUni(TComDataCU* cu, UInt partAddr, int width, int height, RefPicList picList, TComYuv* outPredYuv);
    void xPredInterUni(TComDataCU* cu, UInt partAddr, int width, int height, RefPicList picList, TShortYUV* outPredYuv);
    void xPredInterLumaBlk(TComDataCU *cu, x265::MotionReference *refPic, UInt partAddr, x265::MV *mv, int width, int height, TComYuv *dstPic);
    void xPredInterLumaBlk(TComDataCU *cu, TComPicYuv *refPic, UInt partAddr, x265::MV *mv, int width, int height, TShortYUV *dstPic);
    void xPredInterChromaBlk(TComDataCU *cu, TComPicYuv *refPic, UInt partAddr, x265::MV *mv, int width, int height, TComYuv *dstPic);
    void xPredInterChromaBlk(TComDataCU *cu, TComPicYuv *refPic, UInt partAddr, x265::MV *mv, int width, int height, TShortYUV *dstPic);
    
    void xPredInterBi(TComDataCU* cu, UInt partAddr, int width, int height, TComYuv*& outPredYuv);
    void xWeightedAverage(TComYuv* srcYuv0, TComYuv* srcYuv1, int refIdx0, int refIdx1, UInt partAddr, int width, int height, TComYuv*& outDstYuv);

    void xGetLLSPrediction(TComPattern* pcPattern, int* src0, int srcstride, Pel* dst0, int dststride, UInt width, UInt height, UInt ext0);

    bool xCheckIdenticalMotion(TComDataCU* cu, UInt PartAddr);

public:

    Pel *refAbove, *refAboveFlt, *refLeft, *refLeftFlt;

    TComPrediction();
    virtual ~TComPrediction();

    void initTempBuff();

    // inter
    void motionCompensation(TComDataCU* cu, TComYuv* predYuv, RefPicList picList = REF_PIC_LIST_X, int partIdx = -1);

    // motion vector prediction
    void getMvPredAMVP(TComDataCU* cu, UInt partIdx, UInt partAddr, RefPicList picList, x265::MV& mvPred);

    // Angular Intra
    void predIntraLumaAng(UInt dirMode, Pel* pred, UInt stride, int width);
    void predIntraChromaAng(Pel* src, UInt dirMode, Pel* pred, UInt stride, int width);

    Pel* getPredicBuf()             { return m_predBuf; }

    int  getPredicBufWidth()        { return m_predBufStride; }

    int  getPredicBufHeight()       { return m_predBufHeight; }
};
}
//! \}

#endif // __TCOMPREDICTION__
