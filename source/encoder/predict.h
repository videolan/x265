/*****************************************************************************
* Copyright (C) 2013 x265 project
*
* Authors: Deepthi Nandakumar <deepthi@multicorewareinc.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
*
* This program is also available under a commercial proprietary license.
* For more information, contact us at license @ x265.com.
*****************************************************************************/

#ifndef X265_PREDICT_H
#define X265_PREDICT_H

#include "common.h"
#include "frame.h"
#include "quant.h"
#include "shortyuv.h"
#include "TLibCommon/TComYuv.h"
#include "TLibCommon/TComMotionInfo.h"
#include "TLibCommon/TComPattern.h"
#include "TLibCommon/TComWeightPrediction.h"

namespace x265 {

class Predict : public TComWeightPrediction
{
protected:

    ShortYuv  m_predShortYuv[2]; //temporary storage for weighted prediction
    int16_t*  m_immedVals;

    /* Slice information */
    Slice*    m_slice;
    int       m_csp;

    /* CU information for prediction */
    int       m_width;
    int       m_height; 
    uint32_t  m_partAddr;
    uint32_t  m_cuAddr;
    uint32_t  m_zOrderIdxinCU;

    /* Motion information */
    TComCUMvField* m_mvField[2];
    /* TODO: Need to investigate clipping while writing into the TComDataCU fields itself */
    MV ClippedMv[2];

    // motion compensation functions
    void predInterUni(int picList, TComYuv* outPredYuv, bool bLuma, bool bChroma);
    void predInterUni(int picList, ShortYuv* outPredYuv, bool bLuma, bool bChroma);
    void predInterLumaBlk(TComPicYuv *refPic, TComYuv *dstPic, MV *mv);
    void predInterLumaBlk(TComPicYuv *refPic, ShortYuv *dstPic, MV *mv);
    void predInterChromaBlk(TComPicYuv *refPic, TComYuv *dstPic, MV *mv);
    void predInterChromaBlk(TComPicYuv *refPic, ShortYuv *dstPic, MV *mv);

    void predInterBi(TComDataCU* cu, TComYuv* outPredYuv, bool bLuma, bool bChroma);

    void getLLSPrediction(TComPattern* pcPattern, int* src0, int srcstride, pixel* dst0, int dststride, uint32_t width, uint32_t height, uint32_t ext0);

    bool checkIdenticalMotion();

public:

    // Intra prediction buffers
    pixel*    m_predBuf;
    pixel*    m_refAbove;
    pixel*    m_refAboveFlt;
    pixel*    m_refLeft;
    pixel*    m_refLeftFlt;

    Predict();
    virtual ~Predict();

    void initTempBuff(int csp);

    // prepMotionCompensation needs to be called to prepare MC with CU-relevant data */
    void prepMotionCompensation(TComDataCU* cu, int partIdx);
    void motionCompensation(TComDataCU* cu, TComYuv* predYuv, int picList, bool bLuma, bool bChroma);

    // Angular Intra
    void predIntraLumaAng(uint32_t dirMode, pixel* pred, intptr_t stride, uint32_t log2TrSize);
    void predIntraChromaAng(pixel* src, uint32_t dirMode, pixel* pred, intptr_t stride, uint32_t log2TrSizeC, int chFmt);
    static bool filteringIntraReferenceSamples(uint32_t dirMode, uint32_t log2TrSize);
};
}

#endif // ifndef X265_PREDICT_H
