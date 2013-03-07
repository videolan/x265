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

/** \file     TComPattern.h
    \brief    neighbouring pixel access classes (header)
*/

#ifndef __TCOMPATTERN__
#define __TCOMPATTERN__

// Include files
#include <stdio.h>
#include "CommonDef.h"

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

class TComDataCU;

/// neighbouring pixel access class for one component
class TComPatternParam
{
private:
  Int   m_iOffsetLeft;
  Int   m_iOffsetAbove;
  Pel*  m_piPatternOrigin;
  
public:
  Int   m_iROIWidth;
  Int   m_iROIHeight;
  Int   m_iPatternStride;
  
  /// return starting position of buffer
  Pel*  getPatternOrigin()        { return  m_piPatternOrigin; }
  
  /// return starting position of ROI (ROI = &pattern[AboveOffset][LeftOffset])
  __inline Pel*  getROIOrigin()
  {
    return  m_piPatternOrigin + m_iPatternStride * m_iOffsetAbove + m_iOffsetLeft;
  }
  
  /// set parameters from Pel buffer for accessing neighbouring pixels
  Void setPatternParamPel ( Pel*        piTexture,
                           Int         iRoiWidth,
                           Int         iRoiHeight,
                           Int         iStride,
                           Int         iOffsetLeft,
                           Int         iOffsetAbove );
  
  /// set parameters of one color component from CU data for accessing neighbouring pixels
  Void setPatternParamCU  ( TComDataCU* pcCU,
                           UChar       iComp,
                           UChar       iRoiWidth,
                           UChar       iRoiHeight,
                           Int         iOffsetLeft,
                           Int         iOffsetAbove,
                           UInt        uiAbsZorderIdx );
};

/// neighbouring pixel access class for all components
class TComPattern
{
private:
  TComPatternParam  m_cPatternY;
  TComPatternParam  m_cPatternCb;
  TComPatternParam  m_cPatternCr;
  
  static const UChar m_aucIntraFilter[5];
  
public:
  
  // ROI & pattern information, (ROI = &pattern[AboveOffset][LeftOffset])
  Pel*  getROIY()                 { return m_cPatternY.getROIOrigin();    }
  Int   getROIYWidth()            { return m_cPatternY.m_iROIWidth;       }
  Int   getROIYHeight()           { return m_cPatternY.m_iROIHeight;      }
  Int   getPatternLStride()       { return m_cPatternY.m_iPatternStride;  }

  // access functions of ADI buffers
  Int*  getAdiOrgBuf              ( Int iCuWidth, Int iCuHeight, Int* piAdiBuf );
  Int*  getAdiCbBuf               ( Int iCuWidth, Int iCuHeight, Int* piAdiBuf );
  Int*  getAdiCrBuf               ( Int iCuWidth, Int iCuHeight, Int* piAdiBuf );
  
  Int*  getPredictorPtr           ( UInt uiDirMode, UInt uiWidthBits, Int* piAdiBuf );
  // -------------------------------------------------------------------------------------------------------------------
  // initialization functions
  // -------------------------------------------------------------------------------------------------------------------
  
  /// set parameters from Pel buffers for accessing neighbouring pixels
  Void initPattern            ( Pel*        piY,
                               Pel*        piCb,
                               Pel*        piCr,
                               Int         iRoiWidth,
                               Int         iRoiHeight,
                               Int         iStride,
                               Int         iOffsetLeft,
                               Int         iOffsetAbove );
  
  /// set parameters from CU data for accessing neighbouring pixels
  Void  initPattern           ( TComDataCU* pcCU,
                               UInt        uiPartDepth,
                               UInt        uiAbsPartIdx );
  
  /// set luma parameters from CU data for accessing ADI data
  Void  initAdiPattern        ( TComDataCU* pcCU,
                               UInt        uiZorderIdxInPart,
                               UInt        uiPartDepth,
                               Int*        piAdiBuf,
                               Int         iOrgBufStride,
                               Int         iOrgBufHeight,
                               Bool&       bAbove,
                               Bool&       bLeft
                              ,Bool        bLMmode = false // using for LM chroma or not
                               );
  
  /// set chroma parameters from CU data for accessing ADI data
  Void  initAdiPatternChroma  ( TComDataCU* pcCU,
                               UInt        uiZorderIdxInPart,
                               UInt        uiPartDepth,
                               Int*        piAdiBuf,
                               Int         iOrgBufStride,
                               Int         iOrgBufHeight,
                               Bool&       bAbove,
                               Bool&       bLeft );

private:

  /// padding of unavailable reference samples for intra prediction
  Void  fillReferenceSamples        (Int bitDepth, Pel* piRoiOrigin, Int* piAdiTemp, Bool* bNeighborFlags, Int iNumIntraNeighbor, Int iUnitSize, Int iNumUnitsInCu, Int iTotalUnits, UInt uiCuWidth, UInt uiCuHeight, UInt uiWidth, UInt uiHeight, Int iPicStride, Bool bLMmode = false);
  

  /// constrained intra prediction
  Bool  isAboveLeftAvailable  ( TComDataCU* pcCU, UInt uiPartIdxLT );
  Int   isAboveAvailable      ( TComDataCU* pcCU, UInt uiPartIdxLT, UInt uiPartIdxRT, Bool* bValidFlags );
  Int   isLeftAvailable       ( TComDataCU* pcCU, UInt uiPartIdxLT, UInt uiPartIdxLB, Bool* bValidFlags );
  Int   isAboveRightAvailable ( TComDataCU* pcCU, UInt uiPartIdxLT, UInt uiPartIdxRT, Bool* bValidFlags );
  Int   isBelowLeftAvailable  ( TComDataCU* pcCU, UInt uiPartIdxLT, UInt uiPartIdxLB, Bool* bValidFlags );

};

//! \}

#endif // __TCOMPATTERN__
