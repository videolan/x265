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

/** \file     WeightPredAnalysis.cpp
    \brief    weighted prediction encoder class
*/

#include "TLibCommon/TypeDef.h"
#include "TLibCommon/TComSlice.h"
#include "TLibCommon/TComPic.h"
#include "TLibCommon/TComPicYuv.h"
#include "WeightPredAnalysis.h"

using namespace x265;

#define ABS(a)    ((a) < 0 ? -(a) : (a))
#define DTHRESH (0.99)

WeightPredAnalysis::WeightPredAnalysis()
{
    m_weighted_pred_flag = false;
    m_weighted_bipred_flag = false;
    for (int list = 0; list < 2; list++)
    {
        for (int refIdx = 0; refIdx < MAX_NUM_REF; refIdx++)
        {
            for (int comp = 0; comp < 3; comp++)
            {
                wpScalingParam  *pwp   = &(m_wp[list][refIdx][comp]);
                pwp->bPresentFlag      = false;
                pwp->log2WeightDenom   = 0;
                pwp->inputWeight       = 1;
                pwp->inputOffset       = 0;
            }
        }
    }
}

/** calculate AC and DC values for current original image
 * \param TComSlice *slice
 * \returns void
 */
bool  WeightPredAnalysis::xCalcACDCParamSlice(TComSlice *slice)
{
    //===== calculate AC/DC value =====
    TComPicYuv* pic = slice->getPic()->getPicYuvOrg();
    int sample  = 0;

    // calculate DC/AC value for Y
    Pel* fenc = pic->getLumaAddr();
    Int64 orgDCY = xCalcDCValueSlice(slice, fenc, &sample);
    Int64 orgNormDCY = ((orgDCY + (sample >> 1)) / sample);

    Int64 orgACY  = xCalcACValueSlice(slice, fenc, orgNormDCY);

    // calculate DC/AC value for Cb
    fenc = pic->getCbAddr();
    Int64  orgDCCb = xCalcDCValueUVSlice(slice, fenc, &sample);
    Int64  orgNormDCCb = ((orgDCCb + (sample >> 1)) / (sample));
    fenc = pic->getCbAddr();
    Int64  orgACCb  = xCalcACValueUVSlice(slice, fenc, orgNormDCCb);

    // calculate DC/AC value for Cr
    fenc = pic->getCrAddr();
    Int64  orgDCCr = xCalcDCValueUVSlice(slice, fenc, &sample);
    Int64  orgNormDCCr = ((orgDCCr + (sample >> 1)) / (sample));
    fenc = pic->getCrAddr();
    Int64  orgACCr  = xCalcACValueUVSlice(slice, fenc, orgNormDCCr);

    wpACDCParam weightACDCParam[3];
    weightACDCParam[0].ac = orgACY;
    weightACDCParam[0].dc = orgNormDCY;
    weightACDCParam[1].ac = orgACCb;
    weightACDCParam[1].dc = orgNormDCCb;
    weightACDCParam[2].ac = orgACCr;
    weightACDCParam[2].dc = orgNormDCCr;

    slice->setWpAcDcParam(weightACDCParam);
    return true;
}

/** store weighted_pred_flag and weighted_bipred_idc values
 * \param weighted_pred_flag
 * \param weighted_bipred_idc
 * \returns void
 */
void  WeightPredAnalysis::xStoreWPparam(bool weighted_pred_flag, bool weighted_bipred_flag)
{
    m_weighted_pred_flag = weighted_pred_flag;
    m_weighted_bipred_flag = weighted_bipred_flag;
}

/** restore weighted_pred_flag and weighted_bipred_idc values
 * \param TComSlice *slice
 * \returns void
 */
void  WeightPredAnalysis::xRestoreWPparam(TComSlice *slice)
{
    slice->getPPS()->setUseWP(m_weighted_pred_flag);
    slice->getPPS()->setWPBiPred(m_weighted_bipred_flag);
}

/** check weighted pred or non-weighted pred
 * \param TComSlice *slice
 * \returns void
 */
void  WeightPredAnalysis::xCheckWPEnable(TComSlice *slice)
{
    int presentCnt = 0;

    for (int list = 0; list < 2; list++)
    {
        for (int refIdx = 0; refIdx < MAX_NUM_REF; refIdx++)
        {
            for (int comp = 0; comp < 3; comp++)
            {
                wpScalingParam  *pwp = &(m_wp[list][refIdx][comp]);
                presentCnt += (int)pwp->bPresentFlag;
            }
        }
    }

    if (presentCnt == 0)
    {
        slice->getPPS()->setUseWP(false);
        slice->getPPS()->setWPBiPred(false);
        for (int list = 0; list < 2; list++)
        {
            for (int refIdx = 0; refIdx < MAX_NUM_REF; refIdx++)
            {
                for (int comp = 0; comp < 3; comp++)
                {
                    wpScalingParam  *pwp = &(m_wp[list][refIdx][comp]);
                    pwp->bPresentFlag    = false;
                    pwp->log2WeightDenom = 0;
                    pwp->inputWeight     = 1;
                    pwp->inputOffset     = 0;
                }
            }
        }

        slice->setWpScaling(m_wp);
    }
}

/** estimate wp tables for explicit wp
 * \param TComSlice *slice
 * \returns bool
 */
bool  WeightPredAnalysis::xEstimateWPParamSlice(TComSlice *slice)
{
    int denom = 6;
    bool validRangeFlag = false;

    if (slice->getNumRefIdx(REF_PIC_LIST_0) > 3)
    {
        denom  = 7;
    }

    do
    {
        validRangeFlag = xUpdatingWPParameters(slice, denom);
        if (!validRangeFlag)
        {
            denom--; // decrement to satisfy the range limitation
        }
    }
    while (validRangeFlag == false);

    // selecting whether WP is used, or not
    xSelectWP(slice, m_wp, denom);

    slice->setWpScaling(m_wp);

    return true;
}

/** update wp tables for explicit wp w.r.t range limitation
 * \param TComSlice *slice
 * \returns bool
 */
bool WeightPredAnalysis::xUpdatingWPParameters(TComSlice *slice, int log2Denom)
{
    int numPredDir = slice->isInterP() ? 1 : 2;

    for (int list = 0; list < numPredDir; list++)
    {
        for (int refIdxTemp = 0; refIdxTemp < slice->getNumRefIdx(list); refIdxTemp++)
        {
            wpACDCParam *currWeightACDCParam, *refWeightACDCParam;
            slice->getWpAcDcParam(currWeightACDCParam);
            slice->getRefPic(list, refIdxTemp)->getSlice()->getWpAcDcParam(refWeightACDCParam);

            for (int comp = 0; comp < 3; comp++)
            {
                int realLog2Denom = log2Denom + X265_DEPTH - 8;
                int realOffset = ((int)1 << (realLog2Denom - 1));

                // current frame
                Int64 currDC = currWeightACDCParam[comp].dc;
                Int64 currAC = currWeightACDCParam[comp].ac;
                // reference frame
                Int64 refDC = refWeightACDCParam[comp].dc;
                Int64 refAC = refWeightACDCParam[comp].ac;

                // calculating inputWeight and inputOffset params
                double dWeight = (refAC == 0) ? (double)1.0 : Clip3(-16.0, 15.0, ((double)currAC / (double)refAC));
                int weight = (int)(0.5 + dWeight * (double)(1 << log2Denom));
                int offset = (int)(((currDC << log2Denom) - ((Int64)weight * refDC) + (Int64)realOffset) >> realLog2Denom);

                // Chroma offset range limitation
                if (comp)
                {
                    int pred = (128 - ((128 * weight) >> (log2Denom)));
                    int deltaOffset = Clip3(-512, 511, (offset - pred)); // signed 10bit
                    offset = Clip3(-128, 127, (deltaOffset + pred)); // signed 8bit
                }
                // Luma offset range limitation
                else
                {
                    offset = Clip3(-128, 127, offset);
                }

                // Weighting factor limitation
                int defaultWeight = (1 << log2Denom);
                int deltaWeight = (defaultWeight - weight);
                if (deltaWeight > 127 || deltaWeight < -128)
                    return false;

                m_wp[list][refIdxTemp][comp].bPresentFlag = true;
                m_wp[list][refIdxTemp][comp].inputWeight = (int)weight;
                m_wp[list][refIdxTemp][comp].inputOffset = (int)offset;
                m_wp[list][refIdxTemp][comp].log2WeightDenom = (int)log2Denom;
            }
        }
    }

    return true;
}

/** select whether weighted pred enables or not.
 * \param TComSlice *slice
 * \param wpScalingParam
 * \param denom
 * \returns bool
 */
bool WeightPredAnalysis::xSelectWP(TComSlice *slice, wpScalingParam weightPredTable[2][MAX_NUM_REF][3], int denom)
{
    TComPicYuv* pic = slice->getPic()->getPicYuvOrg();
    int width  = pic->getWidth();
    int height = pic->getHeight();
    int defaultWeight = ((int)1 << denom);
    int numPredDir = slice->isInterP() ? 1 : 2;

    for (int list = 0; list < numPredDir; list++)
    {
        Int64 SADWP = 0, SADnoWP = 0;
        for (int refIdxTmp = 0; refIdxTmp < slice->getNumRefIdx(list); refIdxTmp++)
        {
            Pel*  fenc = pic->getLumaAddr();
            Pel*  fref = slice->getRefPic(list, refIdxTmp)->getPicYuvOrg()->getLumaAddr();
            int   orgStride = pic->getStride();
            int   refStride = slice->getRefPic(list, refIdxTmp)->getPicYuvOrg()->getStride();

            // calculate SAD costs with/without wp for luma
            SADWP   = this->xCalcSADvalueWP(X265_DEPTH, fenc, fref, width, height, orgStride, refStride, denom, weightPredTable[list][refIdxTmp][0].inputWeight, weightPredTable[list][refIdxTmp][0].inputOffset);
            SADnoWP = this->xCalcSADvalueWP(X265_DEPTH, fenc, fref, width, height, orgStride, refStride, denom, defaultWeight, 0);

            fenc = pic->getCbAddr();
            fref = slice->getRefPic(list, refIdxTmp)->getPicYuvOrg()->getCbAddr();
            orgStride = pic->getCStride();
            refStride = slice->getRefPic(list, refIdxTmp)->getPicYuvOrg()->getCStride();

            // calculate SAD costs with/without wp for chroma cb
            SADWP   += this->xCalcSADvalueWP(X265_DEPTH, fenc, fref, width >> 1, height >> 1, orgStride, refStride, denom, weightPredTable[list][refIdxTmp][1].inputWeight, weightPredTable[list][refIdxTmp][1].inputOffset);
            SADnoWP += this->xCalcSADvalueWP(X265_DEPTH, fenc, fref, width >> 1, height >> 1, orgStride, refStride, denom, defaultWeight, 0);

            fenc = pic->getCrAddr();
            fref = slice->getRefPic(list, refIdxTmp)->getPicYuvOrg()->getCrAddr();

            // calculate SAD costs with/without wp for chroma cr
            SADWP   += this->xCalcSADvalueWP(X265_DEPTH, fenc, fref, width >> 1, height >> 1, orgStride, refStride, denom, weightPredTable[list][refIdxTmp][2].inputWeight, weightPredTable[list][refIdxTmp][2].inputOffset);
            SADnoWP += this->xCalcSADvalueWP(X265_DEPTH, fenc, fref, width >> 1, height >> 1, orgStride, refStride, denom, defaultWeight, 0);

            double dRatio = ((double)SADWP / (double)SADnoWP);
            if (dRatio >= (double)DTHRESH)
            {
                for (int comp = 0; comp < 3; comp++)
                {
                    weightPredTable[list][refIdxTmp][comp].bPresentFlag = false;
                    weightPredTable[list][refIdxTmp][comp].inputOffset = (int)0;
                    weightPredTable[list][refIdxTmp][comp].inputWeight = (int)defaultWeight;
                    weightPredTable[list][refIdxTmp][comp].log2WeightDenom = (int)denom;
                }
            }
        }
    }

    return true;
}

/** calculate DC value of original image for luma.
 * \param TComSlice *slice
 * \param Pel *pels
 * \param int32_t *sample
 * \returns Int64
 */
Int64 WeightPredAnalysis::xCalcDCValueSlice(TComSlice *slice, Pel *pels, int32_t *sample)
{
    TComPicYuv* pic = slice->getPic()->getPicYuvOrg();
    int stride = pic->getStride();

    *sample = 0;
    int width  = pic->getWidth();
    int height = pic->getHeight();
    *sample = width * height;
    Int64 dc = xCalcDCValue(pels, width, height, stride);

    return dc;
}

/** calculate AC value of original image for luma.
 * \param TComSlice *slice
 * \param Pel *pels
 * \param int dc
 * \returns Int64
 */
Int64 WeightPredAnalysis::xCalcACValueSlice(TComSlice *slice, Pel *pels, Int64 dc)
{
    TComPicYuv* pic = slice->getPic()->getPicYuvOrg();
    int stride = pic->getStride();

    int width  = pic->getWidth();
    int height = pic->getHeight();
    Int64 ac = xCalcACValue(pels, width, height, stride, dc);

    return ac;
}

/** calculate DC value of original image for chroma.
 * \param TComSlice *slice
 * \param Pel *pels
 * \param int32_t *sample
 * \returns Int64
 */
Int64 WeightPredAnalysis::xCalcDCValueUVSlice(TComSlice *slice, Pel *pels, int32_t *sample)
{
    TComPicYuv* pic = slice->getPic()->getPicYuvOrg();
    int cstride = pic->getCStride();

    *sample = 0;
    int width  = pic->getWidth() >> 1;
    int height = pic->getHeight() >> 1;
    *sample = width * height;
    Int64 dc = xCalcDCValue(pels, width, height, cstride);

    return dc;
}

/** calculate AC value of original image for chroma.
 * \param TComSlice *slice
 * \param Pel *pels
 * \param int dc
 * \returns Int64
 */
Int64 WeightPredAnalysis::xCalcACValueUVSlice(TComSlice *slice, Pel *pels, Int64 dc)
{
    TComPicYuv* pic = slice->getPic()->getPicYuvOrg();
    int cstride = pic->getCStride();

    int width  = pic->getWidth() >> 1;
    int height = pic->getHeight() >> 1;
    Int64 ac = xCalcACValue(pels, width, height, cstride, dc);

    return ac;
}

/** calculate DC value.
 * \param Pel *pels
 * \param int width
 * \param int height
 * \param int stride
 * \returns Int64
 */
Int64 WeightPredAnalysis::xCalcDCValue(Pel *pels, int width, int height, int stride)
{
    int x, y;
    Int64 dc = 0;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            dc += (int)(pels[x]);
        }

        pels += stride;
    }

    return dc;
}

/** calculate AC value.
 * \param Pel *pels
 * \param int width
 * \param int height
 * \param int stride
 * \param int dc
 * \returns Int64
 */
Int64 WeightPredAnalysis::xCalcACValue(Pel *pels, int width, int height, int stride, Int64 dc)
{
    int x, y;
    Int64 ac = 0;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            ac += abs((int)pels[x] - (int)dc);
        }

        pels += stride;
    }

    return ac;
}

/** calculate SAD values for both WP version and non-WP version.
 * \param Pel *orgPel
 * \param Pel *refPel
 * \param int width
 * \param int height
 * \param int orgStride
 * \param int refStride
 * \param int denom
 * \param int inputWeight
 * \param int inputOffset
 * \returns Int64
 */
Int64 WeightPredAnalysis::xCalcSADvalueWP(int bitDepth, Pel *orgPel, Pel *refPel, int width, int height, int orgStride, int refStride, int denom, int inputWeight, int inputOffset)
{
    int x, y;
    Int64 sad = 0;
    Int64 size = width * height;
    Int64 realDenom = denom + bitDepth - 8;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            sad += ABS((((Int64)orgPel[x] << (Int64)denom) - ((Int64)refPel[x] * (Int64)inputWeight + ((Int64)inputOffset << realDenom))));
        }

        orgPel += orgStride;
        refPel += refStride;
    }

    return sad / size;
}
