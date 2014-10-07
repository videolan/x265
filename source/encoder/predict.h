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

namespace x265 {

class Predict
{
protected:

    ShortYuv  m_predShortYuv[2]; /* temporary storage for weighted prediction */
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
    const TComCUMvField* m_mvField[2];

    /* TODO: Need to investigate clipping while writing into the TComDataCU fields itself */
    MV             m_clippedMv[2];

    // motion compensation functions
    void predInterLumaBlk(TComPicYuv *refPic, TComYuv *dstPic, MV *mv);
    void predInterLumaBlk(TComPicYuv *refPic, ShortYuv *dstPic, MV *mv);

    void predInterChromaBlk(TComPicYuv *refPic, TComYuv *dstPic, MV *mv);
    void predInterChromaBlk(TComPicYuv *refPic, ShortYuv *dstPic, MV *mv);

    // Weighted prediction scaling values built from slice parameters (bitdepth scaled)
    struct WeightValues
    {
        int w, o, offset, shift, round;
    };

    void addWeightBi(ShortYuv* srcYuv0, ShortYuv* srcYuv1, const WeightValues wp0[3], const WeightValues wp1[3], TComYuv* predYuv, bool bLuma, bool bChroma);
    void addWeightUni(ShortYuv* srcYuv0, const WeightValues wp[3], TComYuv* predYuv, bool bLuma, bool bChroma);

public:

    /* Intra prediction buffers */
    pixel*    m_predBuf;
    pixel*    m_refAbove;
    pixel*    m_refAboveFlt;
    pixel*    m_refLeft;
    pixel*    m_refLeftFlt;

    Predict();
    ~Predict();

    bool allocBuffers(int csp);

    /* prepMotionCompensation needs to be called to prepare MC with CU-relevant data */
    void prepMotionCompensation(const TComDataCU* cu, const CU* cuData, int partIdx);
    void motionCompensation(TComYuv* predYuv, bool bLuma, bool bChroma);

    /* Angular Intra */
    void predIntraLumaAng(uint32_t dirMode, pixel* pred, intptr_t stride, uint32_t log2TrSize);
    void predIntraChromaAng(pixel* src, uint32_t dirMode, pixel* pred, intptr_t stride, uint32_t log2TrSizeC, int chFmt);
};
}

#endif // ifndef X265_PREDICT_H
