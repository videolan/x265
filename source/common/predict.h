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
#include "yuv.h"
#include "TLibCommon/TComMotionInfo.h"

namespace x265 {

class TComDataCU;
struct CU;

class Predict
{
public:

    enum { ADI_BUF_STRIDE = (2 * MAX_CU_SIZE + 1 + 15) }; // alignment to 16 bytes

    /* Weighted prediction scaling values built from slice parameters (bitdepth scaled) */
    struct WeightValues
    {
        int w, o, offset, shift, round;
    };

    struct IntraNeighbors
    {
        int      numIntraNeighbor;
        int      totalUnits;
        int      aboveUnits;
        int      leftUnits;
        int      unitWidth;
        int      unitHeight;
        int      tuSize;
        uint32_t log2TrSize;
        bool     bNeighborFlags[4 * MAX_NUM_SPU_W + 1];
    };

    ShortYuv  m_predShortYuv[2]; /* temporary storage for weighted prediction */
    int16_t*  m_immedVals;

    /* Intra prediction buffers */
    pixel*    m_predBuf;
    pixel*    m_refAbove;
    pixel*    m_refAboveFlt;
    pixel*    m_refLeft;
    pixel*    m_refLeftFlt;

    /* Slice information */
    Slice*    m_predSlice;
    int       m_csp;

    /* CU information for prediction */
    int       m_width;
    int       m_height; 
    uint32_t  m_partAddr;
    uint32_t  m_cuAddr;
    uint32_t  m_zOrderIdxinCU;

    /* TODO: Need to investigate clipping while writing into the TComDataCU fields itself */
    MV        m_clippedMv[2];

    /* Motion information */
    const TComCUMvField* m_mvField[2];

    Predict();
    ~Predict();

    bool allocBuffers(int csp);

    // motion compensation functions
    void predInterLumaBlk(PicYuv *refPic, Yuv *dstYuv, MV *mv);
    void predInterLumaBlk(PicYuv *refPic, ShortYuv *dstSYuv, MV *mv);

    void predInterChromaBlk(PicYuv *refPic, Yuv *dstYuv, MV *mv);
    void predInterChromaBlk(PicYuv *refPic, ShortYuv *dstSYuv, MV *mv);

    void addWeightBi(ShortYuv* srcYuv0, ShortYuv* srcYuv1, const WeightValues wp0[3], const WeightValues wp1[3], Yuv* predYuv, bool bLuma, bool bChroma);
    void addWeightUni(ShortYuv* srcYuv0, const WeightValues wp[3], Yuv* predYuv, bool bLuma, bool bChroma);

    /* Intra prediction helper functions */
    static void initIntraNeighbors(const TComDataCU& cu, uint32_t zOrderIdxInPart, uint32_t partDepth, bool isLuma, IntraNeighbors *IntraNeighbors);
    static void fillReferenceSamples(const pixel* adiOrigin, int picStride, pixel* adiRef, const IntraNeighbors& intraNeighbors);

    static bool isAboveLeftAvailable(const TComDataCU& cu, uint32_t partIdxLT);
    static int  isAboveAvailable(const TComDataCU& cu, uint32_t partIdxLT, uint32_t partIdxRT, bool* bValidFlags);
    static int  isLeftAvailable(const TComDataCU& cu, uint32_t partIdxLT, uint32_t partIdxLB, bool* bValidFlags);
    static int  isAboveRightAvailable(const TComDataCU& cu, uint32_t partIdxLT, uint32_t partIdxRT, bool* bValidFlags);
    static int  isBelowLeftAvailable(const TComDataCU& cu, uint32_t partIdxLT, uint32_t partIdxLB, bool* bValidFlags);

public:

    /* prepMotionCompensation needs to be called to prepare MC with CU-relevant data */
    void prepMotionCompensation(const TComDataCU* cu, const CU& cuData, int partIdx);
    void motionCompensation(Yuv* predYuv, bool bLuma, bool bChroma);

    /* Angular Intra */
    void predIntraLumaAng(uint32_t dirMode, pixel* pred, intptr_t stride, uint32_t log2TrSize);
    void predIntraChromaAng(pixel* src, uint32_t dirMode, pixel* pred, intptr_t stride, uint32_t log2TrSizeC, int chFmt);

    void initAdiPattern(const TComDataCU& cu, const CU& cuData, uint32_t absPartIdx, uint32_t partDepth, int dirMode);
    void initAdiPatternChroma(const TComDataCU& cu, const CU& cuData, uint32_t absPartIdx, uint32_t partDepth, uint32_t chromaId);
    pixel* getAdiChromaBuf(uint32_t chromaId, int tuSize)
    {
        return m_predBuf + (chromaId == 1 ? 0 : 2 * ADI_BUF_STRIDE * (tuSize * 2 + 1));
    }
};
}

#endif // ifndef X265_PREDICT_H
