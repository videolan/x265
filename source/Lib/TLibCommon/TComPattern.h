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

#ifndef X265_TCOMPATTERN_H
#define X265_TCOMPATTERN_H

#include "CommonDef.h"
#include <stdio.h>

//! \ingroup TLibCommon
//! \{

namespace x265 {
// private namespace

// ====================================================================================================================
// Class definition
// ====================================================================================================================

class TComDataCU;

/// neighboring pixel access class for one component
class TComPatternParam
{
private:

    int  m_offsetLeft;
    int  m_offsetAbove;
    Pel* m_patternOrigin;

public:

    int m_roiWidth;
    int m_roiHeight;
    int m_patternStride;

    /// return starting position of buffer
    Pel* getPatternOrigin()        { return m_patternOrigin; }

    /// return starting position of ROI (ROI = &pattern[AboveOffset][LeftOffset])
    inline Pel* getROIOrigin()
    {
        return m_patternOrigin + m_patternStride * m_offsetAbove + m_offsetLeft;
    }

    /// set parameters from Pel buffer for accessing neighboring pixels
    void setPatternParamPel(Pel* piTexture, int roiWidth, int roiHeight, int stride,
                            int offsetLeft, int offsetAbove);

    /// set parameters of one color component from CU data for accessing neighboring pixels
    void setPatternParamCU(TComDataCU* cu, UChar comp, UChar roiWidth, UChar roiHeight,
                           int offsetLeft, int offsetAbove, uint32_t absZOrderIdx);
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

    int   getROIYWidth()            { return m_patternY.m_roiWidth; }

    int   getROIYHeight()           { return m_patternY.m_roiHeight; }

    int   getPatternLStride()       { return m_patternY.m_patternStride; }

    // access functions of ADI buffers
    Pel*  getAdiOrgBuf(int cuWidth, int cuHeight, Pel* adiBuf);
    Pel*  getAdiCbBuf(int cuWidth, int cuHeight, Pel* adiBuf);
    Pel*  getAdiCrBuf(int cuWidth, int cuHeight, Pel* adiBuf);

    Pel*  getPredictorPtr(uint32_t dirMode, uint32_t uiWidthBits, Pel* adiBuf);

    // -------------------------------------------------------------------------------------------------------------------
    // initialization functions
    // -------------------------------------------------------------------------------------------------------------------

    /// set parameters from Pel buffers for accessing neighboring pixels
    void initPattern(Pel* y, Pel* cb, Pel* cr, int roiWidth, int roiHeight, int stride,
                     int offsetLeft, int offsetAbove);

    void initAdiPattern(TComDataCU* cu, uint32_t zOrderIdxInPart, uint32_t partDepth, Pel* adiBuf,
                        int strideOrig, int heightOrig, Pel* refAbove, Pel* refLeft,
                        Pel* refAboveFlt, Pel* refLeftFlt);

    /// set parameters from CU data for accessing neighboring pixels
    void  initPattern(TComDataCU* cu, uint32_t partDepth, uint32_t absPartIdx);

    /// set luma parameters from CU data for accessing ADI data
    void  initAdiPattern(TComDataCU* cu, uint32_t zOrderIdxInPart, uint32_t partDepth, Pel* adiBuf,
                         int strideOrig, int heightOrig);

    /// set chroma parameters from CU data for accessing ADI data
    void  initAdiPatternChroma(TComDataCU* cu, uint32_t zOrderIdxInPart, uint32_t partDepth,
                               Pel* adiBuf, int strideOrig, int heightOrig);

private:

    /// padding of unavailable reference samples for intra prediction
    void fillReferenceSamples(Pel* roiOrigin, Pel* adiTemp, bool* bNeighborFlags, int numIntraNeighbor, int unitSize, int numUnitsInCU, int totalUnits, uint32_t cuWidth, uint32_t cuHeight, uint32_t width, uint32_t height, int picStride);

    /// constrained intra prediction
    bool  isAboveLeftAvailable(TComDataCU* cu, uint32_t partIdxLT);
    int   isAboveAvailable(TComDataCU* cu, uint32_t partIdxLT, uint32_t partIdxRT, bool* bValidFlags);
    int   isLeftAvailable(TComDataCU* cu, uint32_t partIdxLT, uint32_t partIdxLB, bool* bValidFlags);
    int   isAboveRightAvailable(TComDataCU* cu, uint32_t partIdxLT, uint32_t partIdxRT, bool* bValidFlags);
    int   isBelowLeftAvailable(TComDataCU* cu, uint32_t partIdxLT, uint32_t partIdxLB, bool* bValidFlags);
};
}
//! \}

#endif // ifndef X265_TCOMPATTERN_H
