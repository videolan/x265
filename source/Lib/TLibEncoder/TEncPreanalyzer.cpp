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

/** \file     TEncPreanalyzer.cpp
    \brief    source picture analyzer class
*/

#include <cfloat>
#include <algorithm>

#include "TEncPreanalyzer.h"

using namespace std;

//! \ingroup TLibEncoder
//! \{

/** Constructor
 */
TEncPreanalyzer::TEncPreanalyzer()
{
}

/** Destructor
 */
TEncPreanalyzer::~TEncPreanalyzer()
{
}

/** Analyze source picture and compute local image characteristics used for QP adaptation
 * \param pcEPic Picture object to be analyzed
 * \return Void
 */
Void TEncPreanalyzer::xPreanalyze( TEncPic* pcEPic )
{
  TComPicYuv* pcPicYuv = pcEPic->getPicYuvOrg();
  const Int iWidth = pcPicYuv->getWidth();
  const Int iHeight = pcPicYuv->getHeight();
  const Int iStride = pcPicYuv->getStride();

  for ( UInt d = 0; d < pcEPic->getMaxAQDepth(); d++ )
  {
    const Pel* pLineY = pcPicYuv->getLumaAddr();
    TEncPicQPAdaptationLayer* pcAQLayer = pcEPic->getAQLayer(d);
    const UInt uiAQPartWidth = pcAQLayer->getAQPartWidth();
    const UInt uiAQPartHeight = pcAQLayer->getAQPartHeight();
    TEncQPAdaptationUnit* pcAQU = pcAQLayer->getQPAdaptationUnit();

    Double dSumAct = 0.0;
    for ( UInt y = 0; y < iHeight; y += uiAQPartHeight )
    {
      const UInt uiCurrAQPartHeight = min(uiAQPartHeight, iHeight-y);
      for ( UInt x = 0; x < iWidth; x += uiAQPartWidth, pcAQU++ )
      {
        const UInt uiCurrAQPartWidth = min(uiAQPartWidth, iWidth-x);
        const Pel* pBlkY = &pLineY[x];
        UInt64 uiSum[4] = {0, 0, 0, 0};
        UInt64 uiSumSq[4] = {0, 0, 0, 0};
        UInt uiNumPixInAQPart = 0;
        UInt by = 0;
        for ( ; by < uiCurrAQPartHeight>>1; by++ )
        {
          UInt bx = 0;
          for ( ; bx < uiCurrAQPartWidth>>1; bx++, uiNumPixInAQPart++ )
          {
            uiSum  [0] += pBlkY[bx];
            uiSumSq[0] += pBlkY[bx] * pBlkY[bx];
          }
          for ( ; bx < uiCurrAQPartWidth; bx++, uiNumPixInAQPart++ )
          {
            uiSum  [1] += pBlkY[bx];
            uiSumSq[1] += pBlkY[bx] * pBlkY[bx];
          }
          pBlkY += iStride;
        }
        for ( ; by < uiCurrAQPartHeight; by++ )
        {
          UInt bx = 0;
          for ( ; bx < uiCurrAQPartWidth>>1; bx++, uiNumPixInAQPart++ )
          {
            uiSum  [2] += pBlkY[bx];
            uiSumSq[2] += pBlkY[bx] * pBlkY[bx];
          }
          for ( ; bx < uiCurrAQPartWidth; bx++, uiNumPixInAQPart++ )
          {
            uiSum  [3] += pBlkY[bx];
            uiSumSq[3] += pBlkY[bx] * pBlkY[bx];
          }
          pBlkY += iStride;
        }

        Double dMinVar = DBL_MAX;
        for ( Int i=0; i<4; i++)
        {
          const Double dAverage = Double(uiSum[i]) / uiNumPixInAQPart;
          const Double dVariance = Double(uiSumSq[i]) / uiNumPixInAQPart - dAverage * dAverage;
          dMinVar = min(dMinVar, dVariance);
        }
        const Double dActivity = 1.0 + dMinVar;
        pcAQU->setActivity( dActivity );
        dSumAct += dActivity;
      }
      pLineY += iStride * uiCurrAQPartHeight;
    }

    const Double dAvgAct = dSumAct / (pcAQLayer->getNumAQPartInWidth() * pcAQLayer->getNumAQPartInHeight());
    pcAQLayer->setAvgActivity( dAvgAct );
  }
}
//! \}

