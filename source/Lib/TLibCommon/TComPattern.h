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
    \brief    neighboring pixel access classes (header)
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

/// neighboring pixel access class for one component
class TComPatternParam
{
private:

    Int   m_offsetLeft;
    Int   m_offsetAbove;
    Pel*  m_patternOrigin;

public:

    Int   m_roiWidth;
    Int   m_roiHeight;
    Int   m_patternStride;

    /// return starting position of buffer
    Pel*  getPatternOrigin()        { return m_patternOrigin; }

    /// return starting position of ROI (ROI = &pattern[AboveOffset][LeftOffset])
    inline Pel* getROIOrigin()
    {
        return m_patternOrigin + m_patternStride * m_offsetAbove + m_offsetLeft;
    }

    /// set parameters from Pel buffer for accessing neighboring pixels
    Void setPatternParamPel(Pel* piTexture,
                            Int  roiWidth,
                            Int  roiHeight,
                            Int  stride,
                            Int  offsetLeft,
                            Int  offsetAbove);

    /// set parameters of one color component from CU data for accessing neighboring pixels
    Void setPatternParamCU(TComDataCU* cu,
                           UChar       comp,
                           UChar       roiWidth,
                           UChar       roiHeight,
                           Int         offsetLeft,
                           Int         offsetAbove,
                           UInt        absZOrderIdx);
};

/// neighboring pixel access class for all components
class TComPattern
{
private:

    TComPatternParam  m_patternY;
    TComPatternParam  m_patternCb;
    TComPatternParam  m_patternCr;

    static const UChar m_intraFilter[5];

public:

    // ROI & pattern information, (ROI = &pattern[AboveOffset][LeftOffset])
    Pel*  getROIY()                 { return m_patternY.getROIOrigin(); }

    Int   getROIYWidth()            { return m_patternY.m_roiWidth; }

    Int   getROIYHeight()           { return m_patternY.m_roiHeight; }

    Int   getPatternLStride()       { return m_patternY.m_patternStride; }

    // access functions of ADI buffers
    Pel*  getAdiOrgBuf(Int cuWidth, Int cuHeight, Pel* adiBuf);
    Pel*  getAdiCbBuf(Int cuWidth, Int cuHeight, Pel* adiBuf);
    Pel*  getAdiCrBuf(Int cuWidth, Int cuHeight, Pel* adiBuf);

    Pel*  getPredictorPtr(UInt dirMode, UInt uiWidthBits, Pel* adiBuf);

    // -------------------------------------------------------------------------------------------------------------------
    // initialization functions
    // -------------------------------------------------------------------------------------------------------------------

    /// set parameters from Pel buffers for accessing neighboring pixels
    Void initPattern(Pel* y,
                     Pel* cb,
                     Pel* cr,
                     Int  roiWidth,
                     Int  roiHeight,
                     Int  stride,
                     Int  offsetLeft,
                     Int  offsetAbove);

    Void initAdiPattern(TComDataCU* cu, UInt zOrderIdxInPart, UInt partDepth, Pel* adiBuf, Int strideOrig, Int heightOrig, Pel* refAbove, Pel* refLeft, Pel* refAboveFlt, Pel* refLeftFlt);

    /// set parameters from CU data for accessing neighboring pixels
    Void  initPattern(TComDataCU* cu,
                      UInt        partDepth,
                      UInt        absPartIdx);

    /// set luma parameters from CU data for accessing ADI data
    Void  initAdiPattern(TComDataCU* cu,
                         UInt        zOrderIdxInPart,
                         UInt        partDepth,
                         Pel*        adiBuf,
                         Int         strideOrig,
                         Int         heightOrig);

    /// set chroma parameters from CU data for accessing ADI data
    Void  initAdiPatternChroma(TComDataCU* cu,
                               UInt        zOrderIdxInPart,
                               UInt        partDepth,
                               Pel*        adiBuf,
                               Int         strideOrig,
                               Int         heightOrig);

private:

    /// padding of unavailable reference samples for intra prediction
    Void  fillReferenceSamples(Int bitDepth, Pel* roiOrigin, Pel* adiTemp, Bool* bNeighborFlags, Int numIntraNeighbor, Int unitSize, Int numUnitsInCU, Int totalUnits, UInt cuWidth, UInt cuHeight, UInt width, UInt height, Int picStride);

    /// constrained intra prediction
    Bool  isAboveLeftAvailable(TComDataCU* cu, UInt partIdxLT);
    Int   isAboveAvailable(TComDataCU* cu, UInt partIdxLT, UInt partIdxRT, Bool* bValidFlags);
    Int   isLeftAvailable(TComDataCU* cu, UInt partIdxLT, UInt partIdxLB, Bool* bValidFlags);
    Int   isAboveRightAvailable(TComDataCU* cu, UInt partIdxLT, UInt partIdxRT, Bool* bValidFlags);
    Int   isBelowLeftAvailable(TComDataCU* cu, UInt partIdxLT, UInt partIdxLB, Bool* bValidFlags);
};

//! \}

#endif // __TCOMPATTERN__
