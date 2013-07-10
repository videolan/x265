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

#define NTAPS_LUMA        8 ///< Number of taps for luma
#define NTAPS_CHROMA      4 ///< Number of taps for chroma
#define IF_INTERNAL_PREC 14 ///< Number of bits for internal precision
#define IF_FILTER_PREC    6 ///< Log2 of sum of filter taps
#define IF_INTERNAL_OFFS (1 << (IF_INTERNAL_PREC - 1)) ///< Offset used internally

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// prediction class
class TComPrediction : public TComWeightPrediction
{
protected:

    Pel*      m_piPredBuf;
    Pel*      m_piPredAngBufs;
    Int       m_iPredBufStride;
    Int       m_iPredBufHeight;

    //reference sample for IntraPrediction

    TComYuv   m_acYuvPred[2];
    TShortYUV   m_acShortPred[2];
    TComYuv   m_cYuvPredTemp;
    /*This holds final interpolated pixel values (0-255). Hence memory is stored as Pel.*/
    TComYuv m_filteredBlock[4][4];
    /*This holds intermediate values for filtering operations which need to maintain Short precision*/
    TShortYUV filteredBlockTmp[4]; //This

    Pel*   m_pLumaRecBuffer;     ///< array for downsampled reconstructed luma sample
    Int    m_iLumaRecStride;     ///< stride of #m_pLumaRecBuffer array

    // motion compensation functions
    Void xPredInterUni(TComDataCU* cu,                          UInt partAddr,               Int iWidth, Int iHeight, RefPicList picList, TComYuv*& rpcYuvPred, Bool bi = false);
    Void xPredInterUni(TComDataCU* cu, UInt partAddr, Int iWidth, Int iHeight, RefPicList picList, TShortYUV*& rpcYuvPred, Bool bi);
    Void xPredInterBi(TComDataCU* cu,                          UInt partAddr,               Int iWidth, Int iHeight,                         TComYuv*& rpcYuvPred);
    Void xPredInterLumaBlk(TComDataCU *cu, TComPicYuv *refPic, UInt partAddr, x265::MV *mv, Int width, Int height, TComYuv *&dstPic, Bool bi);
    Void xPredInterLumaBlk(TComDataCU *cu, TComPicYuv *refPic, UInt partAddr, x265::MV *mv, Int width, Int height, TShortYUV *&dstPic, Bool bi);
    Void xPredInterChromaBlk(TComDataCU *cu, TComPicYuv *refPic, UInt partAddr, x265::MV *mv, Int width, Int height, TComYuv *&dstPic, Bool bi);
    Void xPredInterChromaBlk(TComDataCU *cu, TComPicYuv *refPic, UInt partAddr, x265::MV *mv, Int width, Int height, TShortYUV *&dstPic, Bool bi);
    Void xWeightedAverage(TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, Int iRefIdx0, Int iRefIdx1, UInt partAddr, Int iWidth, Int iHeight, TComYuv*& rpcYuvDst);

    Void xGetLLSPrediction(TComPattern* pcPattern, Int* pSrc0, Int iSrcStride, Pel* pDst0, Int iDstStride, UInt uiWidth, UInt uiHeight, UInt uiExt0);

    Bool xCheckIdenticalMotion(TComDataCU* cu, UInt PartAddr);

public:

    Pel *refAbove, *refAboveFlt, *refLeft, *refLeftFlt;

    static const Short m_lumaFilter[4][NTAPS_LUMA];   ///< Luma filter taps
    static const Short m_chromaFilter[8][NTAPS_CHROMA]; ///< Chroma filter taps

    TComPrediction();
    virtual ~TComPrediction();

    Void    initTempBuff();

    // inter
    Void motionCompensation(TComDataCU* cu, TComYuv* predYuv, RefPicList picList = REF_PIC_LIST_X, Int partIdx = -1);

    // motion vector prediction
    Void getMvPredAMVP(TComDataCU* cu, UInt uiPartIdx, UInt partAddr, RefPicList picList, x265::MV& mvPred);

    // Angular Intra
    Void predIntraLumaAng(TComPattern* pcTComPattern, UInt uiDirMode, Pel* piPred, UInt uiStride, Int iWidth);
    Void predIntraChromaAng(Pel* piSrc, UInt uiDirMode, Pel* piPred, UInt uiStride, Int iWidth);

    Pel* getPredicBuf()             { return m_piPredBuf; }

    Int  getPredicBufWidth()        { return m_iPredBufStride; }

    Int  getPredicBufHeight()       { return m_iPredBufHeight; }
};

//! \}

#endif // __TCOMPREDICTION__
