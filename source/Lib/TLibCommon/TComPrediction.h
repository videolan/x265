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
    Int       m_predBufStride;
    Int       m_predBufHeight;

    // references sample for IntraPrediction
    TComYuv   m_predYuv[2];
    TShortYUV m_predShortYuv[2];
    TComYuv   m_predTempYuv;

    /* This holds final interpolated pixel values (0-255). Hence memory is stored as Pel. */
    TComYuv   m_filteredBlock[4][4];

    /* This holds intermediate values for filtering operations which need to maintain Short precision */
    TShortYUV filteredBlockTmp[4];

    Pel*   m_lumaRecBuffer;     ///< array for down-sampled reconstructed luma sample
    Int    m_lumaRecStride;     ///< stride of #m_pLumaRecBuffer array

    // motion compensation functions
    Void xPredInterUni(TComDataCU* cu, UInt partAddr, Int width, Int height, RefPicList picList, TComYuv*& outPredYuv, Bool bi = false);
    Void xPredInterUni(TComDataCU* cu, UInt partAddr, Int width, Int height, RefPicList picList, TShortYUV*& outPredYuv, Bool bi);
    Void xPredInterBi(TComDataCU* cu, UInt partAddr, Int width, Int height, TComYuv*& outPredYuv);
    Void xPredInterLumaBlk(TComDataCU *cu, TComPicYuv *refPic, UInt partAddr, x265::MV *mv, Int width, Int height, TComYuv *&dstPic, Bool bi);
    Void xPredInterLumaBlk(TComDataCU *cu, TComPicYuv *refPic, UInt partAddr, x265::MV *mv, Int width, Int height, TShortYUV *&dstPic, Bool bi);
    Void xPredInterChromaBlk(TComDataCU *cu, TComPicYuv *refPic, UInt partAddr, x265::MV *mv, Int width, Int height, TComYuv *&dstPic, Bool bi);
    Void xPredInterChromaBlk(TComDataCU *cu, TComPicYuv *refPic, UInt partAddr, x265::MV *mv, Int width, Int height, TShortYUV *&dstPic, Bool bi);
    Void xWeightedAverage(TComYuv* srcYuv0, TComYuv* srcYuv1, Int refIdx0, Int refIdx1, UInt partAddr, Int width, Int height, TComYuv*& outDstYuv);

    Void xGetLLSPrediction(TComPattern* pcPattern, Int* src0, Int srcstride, Pel* dst0, Int dststride, UInt width, UInt height, UInt ext0);

    Bool xCheckIdenticalMotion(TComDataCU* cu, UInt PartAddr);

public:

    Pel *refAbove, *refAboveFlt, *refLeft, *refLeftFlt;

    TComPrediction();
    virtual ~TComPrediction();

    Void    initTempBuff();

    // inter
    Void motionCompensation(TComDataCU* cu, TComYuv* predYuv, RefPicList picList = REF_PIC_LIST_X, Int partIdx = -1);

    // motion vector prediction
    Void getMvPredAMVP(TComDataCU* cu, UInt partIdx, UInt partAddr, RefPicList picList, x265::MV& mvPred);

    // Angular Intra
    Void predIntraLumaAng(TComPattern* pcTComPattern, UInt dirMode, Pel* pred, UInt stride, Int width);
    Void predIntraChromaAng(Pel* piSrc, UInt dirMode, Pel* pred, UInt stride, Int width);

    Pel* getPredicBuf()             { return m_predBuf; }

    Int  getPredicBufWidth()        { return m_predBufStride; }

    Int  getPredicBufHeight()       { return m_predBufHeight; }
};

//! \}

#endif // __TCOMPREDICTION__
