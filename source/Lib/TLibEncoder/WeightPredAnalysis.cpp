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
    TComPicYuv*   pPic = slice->getPic()->getPicYuvOrg();
    int   iSample  = 0;

    // calculate DC/AC value for Y
    Pel*  fenc    = pPic->getLumaAddr();
    Int64  iOrgDCY = xCalcDCValueSlice(slice, fenc, &iSample);
    Int64  iOrgNormDCY = ((iOrgDCY + (iSample >> 1)) / iSample);

    fenc = pPic->getLumaAddr();
    Int64  iOrgACY  = xCalcACValueSlice(slice, fenc, iOrgNormDCY);

    // calculate DC/AC value for Cb
    fenc = pPic->getCbAddr();
    Int64  iOrgDCCb = xCalcDCValueUVSlice(slice, fenc, &iSample);
    Int64  iOrgNormDCCb = ((iOrgDCCb + (iSample >> 1)) / (iSample));
    fenc = pPic->getCbAddr();
    Int64  iOrgACCb  = xCalcACValueUVSlice(slice, fenc, iOrgNormDCCb);

    // calculate DC/AC value for Cr
    fenc = pPic->getCrAddr();
    Int64  iOrgDCCr = xCalcDCValueUVSlice(slice, fenc, &iSample);
    Int64  iOrgNormDCCr = ((iOrgDCCr + (iSample >> 1)) / (iSample));
    fenc = pPic->getCrAddr();
    Int64  iOrgACCr  = xCalcACValueUVSlice(slice, fenc, iOrgNormDCCr);

    wpACDCParam weightACDCParam[3];
    weightACDCParam[0].ac = iOrgACY;
    weightACDCParam[0].dc = iOrgNormDCY;
    weightACDCParam[1].ac = iOrgACCb;
    weightACDCParam[1].dc = iOrgNormDCCb;
    weightACDCParam[2].ac = iOrgACCr;
    weightACDCParam[2].dc = iOrgNormDCCr;

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
    int iPresentCnt = 0;

    for (int list = 0; list < 2; list++)
    {
        for (int refIdx = 0; refIdx < MAX_NUM_REF; refIdx++)
        {
            for (int comp = 0; comp < 3; comp++)
            {
                wpScalingParam  *pwp = &(m_wp[list][refIdx][comp]);
                iPresentCnt += (int)pwp->bPresentFlag;
            }
        }
    }

    if (iPresentCnt == 0)
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
                    pwp->bPresentFlag      = false;
                    pwp->log2WeightDenom = 0;
                    pwp->inputWeight           = 1;
                    pwp->inputOffset           = 0;
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
    int iDenom  = 6;
    bool validRangeFlag = false;

    if (slice->getNumRefIdx(REF_PIC_LIST_0) > 3)
    {
        iDenom  = 7;
    }

    do
    {
        validRangeFlag = xUpdatingWPParameters(slice, iDenom);
        if (!validRangeFlag)
        {
            iDenom--; // decrement to satisfy the range limitation
        }
    }
    while (validRangeFlag == false);

    // selecting whether WP is used, or not
    xSelectWP(slice, m_wp, iDenom);

    slice->setWpScaling(m_wp);

    return true;
}

/** update wp tables for explicit wp w.r.t ramge limitation
 * \param TComSlice *slice
 * \returns bool
 */
bool WeightPredAnalysis::xUpdatingWPParameters(TComSlice *slice, int log2Denom)
{
    int numPredDir = slice->isInterP() ? 1 : 2;

    for (int refList = 0; refList < numPredDir; refList++)
    {
        RefPicList  picList = (refList ? REF_PIC_LIST_1 : REF_PIC_LIST_0);
        for (int refIdxTemp = 0; refIdxTemp < slice->getNumRefIdx(picList); refIdxTemp++)
        {
            wpACDCParam *currWeightACDCParam, *refWeightACDCParam;
            slice->getWpAcDcParam(currWeightACDCParam);
            slice->getRefPic(picList, refIdxTemp)->getSlice()->getWpAcDcParam(refWeightACDCParam);

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

                m_wp[refList][refIdxTemp][comp].bPresentFlag = true;
                m_wp[refList][refIdxTemp][comp].inputWeight = (int)weight;
                m_wp[refList][refIdxTemp][comp].inputOffset = (int)offset;
                m_wp[refList][refIdxTemp][comp].log2WeightDenom = (int)log2Denom;
            }
        }
    }

    return true;
}

/** select whether weighted pred enables or not.
 * \param TComSlice *slice
 * \param wpScalingParam
 * \param iDenom
 * \returns bool
 */
bool WeightPredAnalysis::xSelectWP(TComSlice *slice, wpScalingParam weightPredTable[2][MAX_NUM_REF][3], int iDenom)
{
    TComPicYuv*   pPic = slice->getPic()->getPicYuvOrg();
    int width  = pPic->getWidth();
    int height = pPic->getHeight();
    int iDefaultWeight = ((int)1 << iDenom);
    int iNumPredDir = slice->isInterP() ? 1 : 2;

    for (int refList = 0; refList < iNumPredDir; refList++)
    {
        Int64 iSADWP = 0, iSADnoWP = 0;
        RefPicList  picList = (refList ? REF_PIC_LIST_1 : REF_PIC_LIST_0);
        for (int refIdxTmp = 0; refIdxTmp < slice->getNumRefIdx(picList); refIdxTmp++)
        {
            Pel*  fenc    = pPic->getLumaAddr();
            Pel*  fref    = slice->getRefPic(picList, refIdxTmp)->getPicYuvRec()->getLumaAddr();
            int   iOrgStride = pPic->getStride();
            int   iRefStride = slice->getRefPic(picList, refIdxTmp)->getPicYuvRec()->getStride();

            // calculate SAD costs with/without wp for luma
            iSADWP   = this->xCalcSADvalueWP(X265_DEPTH, fenc, fref, width, height, iOrgStride, iRefStride, iDenom, weightPredTable[refList][refIdxTmp][0].inputWeight, weightPredTable[refList][refIdxTmp][0].inputOffset);
            iSADnoWP = this->xCalcSADvalueWP(X265_DEPTH, fenc, fref, width, height, iOrgStride, iRefStride, iDenom, iDefaultWeight, 0);

            fenc = pPic->getCbAddr();
            fref = slice->getRefPic(picList, refIdxTmp)->getPicYuvRec()->getCbAddr();
            iOrgStride = pPic->getCStride();
            iRefStride = slice->getRefPic(picList, refIdxTmp)->getPicYuvRec()->getCStride();

            // calculate SAD costs with/without wp for chroma cb
            iSADWP   += this->xCalcSADvalueWP(X265_DEPTH, fenc, fref, width >> 1, height >> 1, iOrgStride, iRefStride, iDenom, weightPredTable[refList][refIdxTmp][1].inputWeight, weightPredTable[refList][refIdxTmp][1].inputOffset);
            iSADnoWP += this->xCalcSADvalueWP(X265_DEPTH, fenc, fref, width >> 1, height >> 1, iOrgStride, iRefStride, iDenom, iDefaultWeight, 0);

            fenc = pPic->getCrAddr();
            fref = slice->getRefPic(picList, refIdxTmp)->getPicYuvRec()->getCrAddr();

            // calculate SAD costs with/without wp for chroma cr
            iSADWP   += this->xCalcSADvalueWP(X265_DEPTH, fenc, fref, width >> 1, height >> 1, iOrgStride, iRefStride, iDenom, weightPredTable[refList][refIdxTmp][2].inputWeight, weightPredTable[refList][refIdxTmp][2].inputOffset);
            iSADnoWP += this->xCalcSADvalueWP(X265_DEPTH, fenc, fref, width >> 1, height >> 1, iOrgStride, iRefStride, iDenom, iDefaultWeight, 0);

            double dRatio = ((double)iSADWP / (double)iSADnoWP);
            if (dRatio >= (double)DTHRESH)
            {
                for (int comp = 0; comp < 3; comp++)
                {
                    weightPredTable[refList][refIdxTmp][comp].bPresentFlag = false;
                    weightPredTable[refList][refIdxTmp][comp].inputOffset = (int)0;
                    weightPredTable[refList][refIdxTmp][comp].inputWeight = (int)iDefaultWeight;
                    weightPredTable[refList][refIdxTmp][comp].log2WeightDenom = (int)iDenom;
                }
            }
        }
    }

    return true;
}

/** calculate DC value of original image for luma.
 * \param TComSlice *slice
 * \param Pel *pPel
 * \param int *iSample
 * \returns Int64
 */
Int64 WeightPredAnalysis::xCalcDCValueSlice(TComSlice *slice, Pel *pPel, int *iSample)
{
    TComPicYuv* pPic = slice->getPic()->getPicYuvOrg();
    int stride = pPic->getStride();

    *iSample = 0;
    int width  = pPic->getWidth();
    int height = pPic->getHeight();
    *iSample = width * height;
    Int64 dc = xCalcDCValue(pPel, width, height, stride);

    return dc;
}

/** calculate AC value of original image for luma.
 * \param TComSlice *slice
 * \param Pel *pPel
 * \param int dc
 * \returns Int64
 */
Int64 WeightPredAnalysis::xCalcACValueSlice(TComSlice *slice, Pel *pPel, Int64 dc)
{
    TComPicYuv* pPic = slice->getPic()->getPicYuvOrg();
    int stride = pPic->getStride();

    int width  = pPic->getWidth();
    int height = pPic->getHeight();
    Int64 ac = xCalcACValue(pPel, width, height, stride, dc);

    return ac;
}

/** calculate DC value of original image for chroma.
 * \param TComSlice *slice
 * \param Pel *pPel
 * \param int *iSample
 * \returns Int64
 */
Int64 WeightPredAnalysis::xCalcDCValueUVSlice(TComSlice *slice, Pel *pPel, int *iSample)
{
    TComPicYuv* pPic = slice->getPic()->getPicYuvOrg();
    int iCStride = pPic->getCStride();

    *iSample = 0;
    int width  = pPic->getWidth() >> 1;
    int height = pPic->getHeight() >> 1;
    *iSample = width * height;
    Int64 dc = xCalcDCValue(pPel, width, height, iCStride);

    return dc;
}

/** calculate AC value of original image for chroma.
 * \param TComSlice *slice
 * \param Pel *pPel
 * \param int dc
 * \returns Int64
 */
Int64 WeightPredAnalysis::xCalcACValueUVSlice(TComSlice *slice, Pel *pPel, Int64 dc)
{
    TComPicYuv* pPic = slice->getPic()->getPicYuvOrg();
    int iCStride = pPic->getCStride();

    int width  = pPic->getWidth() >> 1;
    int height = pPic->getHeight() >> 1;
    Int64 ac = xCalcACValue(pPel, width, height, iCStride, dc);

    return ac;
}

/** calculate DC value.
 * \param Pel *pPel
 * \param int width
 * \param int height
 * \param int stride
 * \returns Int64
 */
Int64 WeightPredAnalysis::xCalcDCValue(Pel *pPel, int width, int height, int stride)
{
    int x, y;
    Int64 dc = 0;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            dc += (int)(pPel[x]);
        }

        pPel += stride;
    }

    return dc;
}

/** calculate AC value.
 * \param Pel *pPel
 * \param int width
 * \param int height
 * \param int stride
 * \param int dc
 * \returns Int64
 */
Int64 WeightPredAnalysis::xCalcACValue(Pel *pPel, int width, int height, int stride, Int64 dc)
{
    int x, y;
    Int64 ac = 0;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            ac += abs((int)pPel[x] - (int)dc);
        }

        pPel += stride;
    }

    return ac;
}

/** calculate SAD values for both WP version and non-WP version.
 * \param Pel *pOrgPel
 * \param Pel *pRefPel
 * \param int width
 * \param int height
 * \param int iOrgStride
 * \param int iRefStride
 * \param int iDenom
 * \param int inputWeight
 * \param int inputOffset
 * \returns Int64
 */
Int64 WeightPredAnalysis::xCalcSADvalueWP(int bitDepth, Pel *pOrgPel, Pel *pRefPel, int width, int height, int iOrgStride, int iRefStride, int iDenom, int inputWeight, int inputOffset)
{
    int x, y;
    Int64 iSAD = 0;
    Int64 size   = width * height;
    Int64 iRealDenom = iDenom + bitDepth - 8;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            iSAD += ABS((((Int64)pOrgPel[x] << (Int64)iDenom) - ((Int64)pRefPel[x] * (Int64)inputWeight + ((Int64)inputOffset << iRealDenom))));
        }

        pOrgPel += iOrgStride;
        pRefPel += iRefStride;
    }

    return iSAD / size;
}
