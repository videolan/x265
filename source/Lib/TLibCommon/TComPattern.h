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

#include "common.h"
#include "TComRom.h"

//! \ingroup TLibCommon
//! \{

namespace x265 {
// private namespace

// ====================================================================================================================
// Class definition
// ====================================================================================================================

class TComDataCU;

struct IntraNeighbors
{
    int  numIntraNeighbor;
    int  totalUnits;
    int  aboveUnits;
    int  leftUnits;
    int  unitWidth;
    int  unitHeight;
    int  tuSize;
    uint32_t log2TrSize;
    bool bNeighborFlags[4 * MAX_NUM_SPU_W + 1];
};

/// neighboring pixel access class for all components
class TComPattern
{
public:

    // access functions of ADI buffers
    static pixel* getAdiChromaBuf(uint32_t chromaId, int tuSize, pixel* adiBuf)
    {
        return adiBuf + (chromaId == 1 ? 0 : 2 * ADI_BUF_STRIDE * (tuSize * 2 + 1));
    }

    // -------------------------------------------------------------------------------------------------------------------
    // initialization functions
    // -------------------------------------------------------------------------------------------------------------------

    /// set parameters from pixel buffers for accessing neighboring pixels
    static void initAdiPattern(TComDataCU* cu, uint32_t zOrderIdxInPart, uint32_t partDepth, pixel* adiBuf,
                               pixel* refAbove, pixel* refLeft,
                               pixel* refAboveFlt, pixel* refLeftFlt, int dirMode);

    /// set chroma parameters from CU data for accessing ADI data
    static void initAdiPatternChroma(TComDataCU* cu, uint32_t zOrderIdxInPart, uint32_t partDepth,
                                     pixel* adiBuf, uint32_t chromaId);

    static void initIntraNeighbors(TComDataCU* cu, uint32_t zOrderIdxInPart, uint32_t partDepth, bool isLuma, IntraNeighbors *IntraNeighbors);

private:

    /// padding of unavailable reference samples for intra prediction
    static void fillReferenceSamples(pixel* adiOrigin, int picStride, pixel* adiRef, const IntraNeighbors& intraNeighbors);

    /// constrained intra prediction
    static bool  isAboveLeftAvailable(TComDataCU* cu, uint32_t partIdxLT);
    static int   isAboveAvailable(TComDataCU* cu, uint32_t partIdxLT, uint32_t partIdxRT, bool* bValidFlags);
    static int   isLeftAvailable(TComDataCU* cu, uint32_t partIdxLT, uint32_t partIdxLB, bool* bValidFlags);
    static int   isAboveRightAvailable(TComDataCU* cu, uint32_t partIdxLT, uint32_t partIdxRT, bool* bValidFlags);
    static int   isBelowLeftAvailable(TComDataCU* cu, uint32_t partIdxLT, uint32_t partIdxLB, bool* bValidFlags);
};
}
//! \}

#endif // ifndef X265_TCOMPATTERN_H
