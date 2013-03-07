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

#include "../TLibCommon/TypeDef.h"
#include "../TLibCommon/TComSlice.h"
#include "../TLibCommon/TComPic.h"
#include "../TLibCommon/TComPicYuv.h"
#include "WeightPredAnalysis.h"

#define ABS(a)    ((a) < 0 ? - (a) : (a))
#define DTHRESH (0.99)

WeightPredAnalysis::WeightPredAnalysis()
{
  m_weighted_pred_flag = false;
  m_weighted_bipred_flag = false;
  for ( Int iList =0 ; iList<2 ; iList++ )
  {
    for ( Int iRefIdx=0 ; iRefIdx<MAX_NUM_REF ; iRefIdx++ ) 
    {
      for ( Int comp=0 ; comp<3 ;comp++ )
      {
        wpScalingParam  *pwp   = &(m_wp[iList][iRefIdx][comp]);
        pwp->bPresentFlag      = false;
        pwp->uiLog2WeightDenom = 0;
        pwp->iWeight           = 1;
        pwp->iOffset           = 0;
      }
    }
  }
}

/** calculate AC and DC values for current original image
 * \param TComSlice *slice
 * \returns Void
 */
Bool  WeightPredAnalysis::xCalcACDCParamSlice(TComSlice *slice)
{
  //===== calculate AC/DC value =====
  TComPicYuv*   pPic = slice->getPic()->getPicYuvOrg();
  Int   iSample  = 0;

  // calculate DC/AC value for Y
  Pel*  pOrg    = pPic->getLumaAddr();
  Int64  iOrgDCY = xCalcDCValueSlice(slice, pOrg, &iSample);
  Int64  iOrgNormDCY = ((iOrgDCY+(iSample>>1)) / iSample);
  pOrg = pPic->getLumaAddr();
  Int64  iOrgACY  = xCalcACValueSlice(slice, pOrg, iOrgNormDCY);

  // calculate DC/AC value for Cb
  pOrg = pPic->getCbAddr();
  Int64  iOrgDCCb = xCalcDCValueUVSlice(slice, pOrg, &iSample);
  Int64  iOrgNormDCCb = ((iOrgDCCb+(iSample>>1)) / (iSample));
  pOrg = pPic->getCbAddr();
  Int64  iOrgACCb  = xCalcACValueUVSlice(slice, pOrg, iOrgNormDCCb);

  // calculate DC/AC value for Cr
  pOrg = pPic->getCrAddr();
  Int64  iOrgDCCr = xCalcDCValueUVSlice(slice, pOrg, &iSample);
  Int64  iOrgNormDCCr = ((iOrgDCCr+(iSample>>1)) / (iSample));
  pOrg = pPic->getCrAddr();
  Int64  iOrgACCr  = xCalcACValueUVSlice(slice, pOrg, iOrgNormDCCr);

  wpACDCParam weightACDCParam[3];
  weightACDCParam[0].iAC = iOrgACY;
  weightACDCParam[0].iDC = iOrgNormDCY;
  weightACDCParam[1].iAC = iOrgACCb;
  weightACDCParam[1].iDC = iOrgNormDCCb;
  weightACDCParam[2].iAC = iOrgACCr;
  weightACDCParam[2].iDC = iOrgNormDCCr;

  slice->setWpAcDcParam(weightACDCParam);
  return (true);
}

/** store weighted_pred_flag and weighted_bipred_idc values
 * \param weighted_pred_flag
 * \param weighted_bipred_idc
 * \returns Void
 */
Void  WeightPredAnalysis::xStoreWPparam(Bool weighted_pred_flag, Bool weighted_bipred_flag)
{
  m_weighted_pred_flag = weighted_pred_flag;
  m_weighted_bipred_flag = weighted_bipred_flag;
}

/** restore weighted_pred_flag and weighted_bipred_idc values
 * \param TComSlice *slice
 * \returns Void
 */
Void  WeightPredAnalysis::xRestoreWPparam(TComSlice *slice)
{
  slice->getPPS()->setUseWP(m_weighted_pred_flag);
  slice->getPPS()->setWPBiPred(m_weighted_bipred_flag);
}

/** check weighted pred or non-weighted pred
 * \param TComSlice *slice
 * \returns Void
 */
Void  WeightPredAnalysis::xCheckWPEnable(TComSlice *slice)
{
  Int iPresentCnt = 0;
  for ( Int iList=0 ; iList<2 ; iList++ )
  {
    for ( Int iRefIdx=0 ; iRefIdx<MAX_NUM_REF ; iRefIdx++ ) 
    {
      for ( Int iComp=0 ; iComp<3 ;iComp++ ) 
      {
        wpScalingParam  *pwp = &(m_wp[iList][iRefIdx][iComp]);
        iPresentCnt += (Int)pwp->bPresentFlag;
      }
    }
  }

  if(iPresentCnt==0)
  {
    slice->getPPS()->setUseWP(false);
    slice->getPPS()->setWPBiPred(false);
    for ( Int iList=0 ; iList<2 ; iList++ )
    {
      for ( Int iRefIdx=0 ; iRefIdx<MAX_NUM_REF ; iRefIdx++ ) 
      {
        for ( Int iComp=0 ; iComp<3 ;iComp++ ) 
        {
          wpScalingParam  *pwp = &(m_wp[iList][iRefIdx][iComp]);
          pwp->bPresentFlag      = false;
          pwp->uiLog2WeightDenom = 0;
          pwp->iWeight           = 1;
          pwp->iOffset           = 0;
        }
      }
    }
    slice->setWpScaling( m_wp );
  }
}

/** estimate wp tables for explicit wp
 * \param TComSlice *slice
 * \returns Bool
 */
Bool  WeightPredAnalysis::xEstimateWPParamSlice(TComSlice *slice)
{
  Int iDenom  = 6;
  Bool validRangeFlag = false;

  if(slice->getNumRefIdx(REF_PIC_LIST_0)>3)
  {
    iDenom  = 7;
  }

  do
  {
    validRangeFlag = xUpdatingWPParameters(slice, m_wp, iDenom);
    if (!validRangeFlag)
    {
      iDenom--; // decrement to satisfy the range limitation
    }
  } while (validRangeFlag == false);

  // selecting whether WP is used, or not
  xSelectWP(slice, m_wp, iDenom);
  
  slice->setWpScaling( m_wp );

  return (true);
}

/** update wp tables for explicit wp w.r.t ramge limitation
 * \param TComSlice *slice
 * \returns Bool
 */
Bool WeightPredAnalysis::xUpdatingWPParameters(TComSlice *slice, wpScalingParam weightPredTable[2][MAX_NUM_REF][3], Int log2Denom)
{
  Int numPredDir = slice->isInterP() ? 1 : 2;
  for ( Int refList = 0; refList < numPredDir; refList++ )
  {
    RefPicList  eRefPicList = ( refList ? REF_PIC_LIST_1 : REF_PIC_LIST_0 );
    for ( Int refIdxTemp = 0; refIdxTemp < slice->getNumRefIdx(eRefPicList); refIdxTemp++ )
    {
      wpACDCParam *currWeightACDCParam, *refWeightACDCParam;
      slice->getWpAcDcParam(currWeightACDCParam);
      slice->getRefPic(eRefPicList, refIdxTemp)->getSlice(0)->getWpAcDcParam(refWeightACDCParam);

      for ( Int comp = 0; comp < 3; comp++ )
      {
        Int bitDepth = comp ? g_bitDepthC : g_bitDepthY;
        Int realLog2Denom = log2Denom + bitDepth-8;
        Int realOffset = ((Int)1<<(realLog2Denom-1));

        // current frame
        Int64 currDC = currWeightACDCParam[comp].iDC;
        Int64 currAC = currWeightACDCParam[comp].iAC;
        // reference frame
        Int64 refDC = refWeightACDCParam[comp].iDC;
        Int64 refAC = refWeightACDCParam[comp].iAC;

        // calculating iWeight and iOffset params
        Double dWeight = (refAC==0) ? (Double)1.0 : Clip3( -16.0, 15.0, ((Double)currAC / (Double)refAC) );
        Int weight = (Int)( 0.5 + dWeight * (Double)(1<<log2Denom) );
        Int offset = (Int)( ((currDC<<log2Denom) - ((Int64)weight * refDC) + (Int64)realOffset) >> realLog2Denom );

        // Chroma offset range limitation
        if(comp)
        {
          Int pred = ( 128 - ( ( 128*weight)>>(log2Denom) ) );
          Int deltaOffset = Clip3( -512, 511, (offset - pred) );    // signed 10bit
          offset = Clip3( -128, 127, (deltaOffset + pred) );        // signed 8bit
        }
        // Luma offset range limitation
        else
        {
          offset = Clip3( -128, 127, offset);
        }

        // Weighting factor limitation
        Int defaultWeight = (1<<log2Denom);
        Int deltaWeight = (defaultWeight - weight);
        if(deltaWeight > 127 || deltaWeight < -128)
          return (false);

        m_wp[refList][refIdxTemp][comp].bPresentFlag = true;
        m_wp[refList][refIdxTemp][comp].iWeight = (Int)weight;
        m_wp[refList][refIdxTemp][comp].iOffset = (Int)offset;
        m_wp[refList][refIdxTemp][comp].uiLog2WeightDenom = (Int)log2Denom;
      }
    }
  }
  return (true);
}

/** select whether weighted pred enables or not. 
 * \param TComSlice *slice
 * \param wpScalingParam
 * \param iDenom
 * \returns Bool
 */
Bool WeightPredAnalysis::xSelectWP(TComSlice *slice, wpScalingParam weightPredTable[2][MAX_NUM_REF][3], Int iDenom)
{
  TComPicYuv*   pPic = slice->getPic()->getPicYuvOrg();
  Int iWidth  = pPic->getWidth();
  Int iHeight = pPic->getHeight();
  Int iDefaultWeight = ((Int)1<<iDenom);
  Int iNumPredDir = slice->isInterP() ? 1 : 2;

  for ( Int iRefList = 0; iRefList < iNumPredDir; iRefList++ )
  {
    Int64 iSADWP = 0, iSADnoWP = 0;
    RefPicList  eRefPicList = ( iRefList ? REF_PIC_LIST_1 : REF_PIC_LIST_0 );
    for ( Int iRefIdxTemp = 0; iRefIdxTemp < slice->getNumRefIdx(eRefPicList); iRefIdxTemp++ )
    {
      Pel*  pOrg    = pPic->getLumaAddr();
      Pel*  pRef    = slice->getRefPic(eRefPicList, iRefIdxTemp)->getPicYuvRec()->getLumaAddr();
      Int   iOrgStride = pPic->getStride();
      Int   iRefStride = slice->getRefPic(eRefPicList, iRefIdxTemp)->getPicYuvRec()->getStride();

      // calculate SAD costs with/without wp for luma
      iSADWP   = this->xCalcSADvalueWP(g_bitDepthY, pOrg, pRef, iWidth, iHeight, iOrgStride, iRefStride, iDenom, weightPredTable[iRefList][iRefIdxTemp][0].iWeight, weightPredTable[iRefList][iRefIdxTemp][0].iOffset);
      iSADnoWP = this->xCalcSADvalueWP(g_bitDepthY, pOrg, pRef, iWidth, iHeight, iOrgStride, iRefStride, iDenom, iDefaultWeight, 0);

      pOrg = pPic->getCbAddr();
      pRef = slice->getRefPic(eRefPicList, iRefIdxTemp)->getPicYuvRec()->getCbAddr();
      iOrgStride = pPic->getCStride();
      iRefStride = slice->getRefPic(eRefPicList, iRefIdxTemp)->getPicYuvRec()->getCStride();

      // calculate SAD costs with/without wp for chroma cb
      iSADWP   += this->xCalcSADvalueWP(g_bitDepthC, pOrg, pRef, iWidth>>1, iHeight>>1, iOrgStride, iRefStride, iDenom, weightPredTable[iRefList][iRefIdxTemp][1].iWeight, weightPredTable[iRefList][iRefIdxTemp][1].iOffset);
      iSADnoWP += this->xCalcSADvalueWP(g_bitDepthC, pOrg, pRef, iWidth>>1, iHeight>>1, iOrgStride, iRefStride, iDenom, iDefaultWeight, 0);

      pOrg = pPic->getCrAddr();
      pRef = slice->getRefPic(eRefPicList, iRefIdxTemp)->getPicYuvRec()->getCrAddr();

      // calculate SAD costs with/without wp for chroma cr
      iSADWP   += this->xCalcSADvalueWP(g_bitDepthC, pOrg, pRef, iWidth>>1, iHeight>>1, iOrgStride, iRefStride, iDenom, weightPredTable[iRefList][iRefIdxTemp][2].iWeight, weightPredTable[iRefList][iRefIdxTemp][2].iOffset);
      iSADnoWP += this->xCalcSADvalueWP(g_bitDepthC, pOrg, pRef, iWidth>>1, iHeight>>1, iOrgStride, iRefStride, iDenom, iDefaultWeight, 0);

      Double dRatio = ((Double)iSADWP / (Double)iSADnoWP);
      if(dRatio >= (Double)DTHRESH)
      {
        for ( Int iComp = 0; iComp < 3; iComp++ )
        {
          weightPredTable[iRefList][iRefIdxTemp][iComp].bPresentFlag = false;
          weightPredTable[iRefList][iRefIdxTemp][iComp].iOffset = (Int)0;
          weightPredTable[iRefList][iRefIdxTemp][iComp].iWeight = (Int)iDefaultWeight;
          weightPredTable[iRefList][iRefIdxTemp][iComp].uiLog2WeightDenom = (Int)iDenom;
        }
      }
    }
  }
  return (true);
}

/** calculate DC value of original image for luma. 
 * \param TComSlice *slice
 * \param Pel *pPel
 * \param Int *iSample
 * \returns Int64
 */
Int64 WeightPredAnalysis::xCalcDCValueSlice(TComSlice *slice, Pel *pPel, Int *iSample)
{
  TComPicYuv* pPic = slice->getPic()->getPicYuvOrg();
  Int iStride = pPic->getStride();

  *iSample = 0;
  Int iWidth  = pPic->getWidth();
  Int iHeight = pPic->getHeight();
  *iSample = iWidth*iHeight;
  Int64 iDC = xCalcDCValue(pPel, iWidth, iHeight, iStride);

  return (iDC);
}

/** calculate AC value of original image for luma. 
 * \param TComSlice *slice
 * \param Pel *pPel
 * \param Int iDC
 * \returns Int64
 */
Int64 WeightPredAnalysis::xCalcACValueSlice(TComSlice *slice, Pel *pPel, Int64 iDC)
{
  TComPicYuv* pPic = slice->getPic()->getPicYuvOrg();
  Int iStride = pPic->getStride();

  Int iWidth  = pPic->getWidth();
  Int iHeight = pPic->getHeight();
  Int64 iAC = xCalcACValue(pPel, iWidth, iHeight, iStride, iDC);

  return (iAC);
}

/** calculate DC value of original image for chroma. 
 * \param TComSlice *slice
 * \param Pel *pPel
 * \param Int *iSample
 * \returns Int64
 */
Int64 WeightPredAnalysis::xCalcDCValueUVSlice(TComSlice *slice, Pel *pPel, Int *iSample)
{
  TComPicYuv* pPic = slice->getPic()->getPicYuvOrg();
  Int iCStride = pPic->getCStride();

  *iSample = 0;
  Int iWidth  = pPic->getWidth()>>1;
  Int iHeight = pPic->getHeight()>>1;
  *iSample = iWidth*iHeight;
  Int64 iDC = xCalcDCValue(pPel, iWidth, iHeight, iCStride);

  return (iDC);
}

/** calculate AC value of original image for chroma. 
 * \param TComSlice *slice
 * \param Pel *pPel
 * \param Int iDC
 * \returns Int64
 */
Int64 WeightPredAnalysis::xCalcACValueUVSlice(TComSlice *slice, Pel *pPel, Int64 iDC)
{
  TComPicYuv* pPic = slice->getPic()->getPicYuvOrg();
  Int iCStride = pPic->getCStride();

  Int iWidth  = pPic->getWidth()>>1;
  Int iHeight = pPic->getHeight()>>1;
  Int64 iAC = xCalcACValue(pPel, iWidth, iHeight, iCStride, iDC);

  return (iAC);
}

/** calculate DC value. 
 * \param Pel *pPel
 * \param Int iWidth
 * \param Int iHeight
 * \param Int iStride
 * \returns Int64
 */
Int64 WeightPredAnalysis::xCalcDCValue(Pel *pPel, Int iWidth, Int iHeight, Int iStride)
{
  Int x, y;
  Int64 iDC = 0;
  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      iDC += (Int)( pPel[x] );
    }
    pPel += iStride;
  }
  return (iDC);
}

/** calculate AC value. 
 * \param Pel *pPel
 * \param Int iWidth
 * \param Int iHeight
 * \param Int iStride
 * \param Int iDC
 * \returns Int64
 */
Int64 WeightPredAnalysis::xCalcACValue(Pel *pPel, Int iWidth, Int iHeight, Int iStride, Int64 iDC)
{
  Int x, y;
  Int64 iAC = 0;
  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      iAC += abs( (Int)pPel[x] - (Int)iDC );
    }
    pPel += iStride;
  }
  return (iAC);
}

/** calculate SAD values for both WP version and non-WP version. 
 * \param Pel *pOrgPel
 * \param Pel *pRefPel
 * \param Int iWidth
 * \param Int iHeight
 * \param Int iOrgStride
 * \param Int iRefStride
 * \param Int iDenom
 * \param Int iWeight
 * \param Int iOffset
 * \returns Int64
 */
Int64 WeightPredAnalysis::xCalcSADvalueWP(Int bitDepth, Pel *pOrgPel, Pel *pRefPel, Int iWidth, Int iHeight, Int iOrgStride, Int iRefStride, Int iDenom, Int iWeight, Int iOffset)
{
  Int x, y;
  Int64 iSAD = 0;
  Int64 iSize   = iWidth*iHeight;
  Int64 iRealDenom = iDenom + bitDepth-8;
  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      iSAD += ABS(( ((Int64)pOrgPel[x]<<(Int64)iDenom) - ( (Int64)pRefPel[x] * (Int64)iWeight + ((Int64)iOffset<<iRealDenom) ) ) );
    }
    pOrgPel += iOrgStride;
    pRefPel += iRefStride;
  }
  return (iSAD/iSize);
}


