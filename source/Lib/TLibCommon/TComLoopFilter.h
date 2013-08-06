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

/** \file     TComLoopFilter.h
    \brief    deblocking filter (header)
*/

#ifndef __TCOMLOOPFILTER__
#define __TCOMLOOPFILTER__

#include "CommonDef.h"
#include "TComPic.h"

//! \ingroup TLibCommon
//! \{

#define DEBLOCK_SMALLEST_BLOCK  8

/// parameters for deblocking filter
typedef struct _LFCUParam
{
    Bool bLeftEdge;                       ///< indicates left edge
    Bool bTopEdge;                        ///< indicates top edge
} LFCUParam;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// deblocking filter class
class TComLoopFilter
{
private:

    UInt      m_numPartitions;
    UChar*    m_blockingStrength[2];            ///< Bs for [Ver/Hor][Y/U/V][Blk_Idx]
    Bool*     m_bEdgeFilter[2];
    LFCUParam m_lfcuParam;                ///< status structure

    Bool      m_bLFCrossTileBoundary;

protected:

    /// CU-level deblocking function
    Void xDeblockCU(TComDataCU* cu, UInt absZOrderIdx, UInt depth, Int Edge);

    // set / get functions
    Void xSetLoopfilterParam(TComDataCU* cu, UInt absZOrderIdx);
    // filtering functions
    Void xSetEdgefilterTU(TComDataCU* cu, UInt absTUPartIdx, UInt absZOrderIdx, UInt depth);
    Void xSetEdgefilterPU(TComDataCU* cu, UInt absZOrderIdx);
    Void xGetBoundaryStrengthSingle(TComDataCU* cu, Int dir, UInt partIdx);
    UInt xCalcBsIdx(TComDataCU* cu, UInt absZOrderIdx, Int dir, Int edgeIdx, Int baseUnitIdx)
    {
        TComPic* const pic = cu->getPic();
        const UInt lcuWidthInBaseUnits = pic->getNumPartInWidth();

        if (dir == 0)
        {
            return g_rasterToZscan[g_zscanToRaster[absZOrderIdx] + baseUnitIdx * lcuWidthInBaseUnits + edgeIdx];
        }
        else
        {
            return g_rasterToZscan[g_zscanToRaster[absZOrderIdx] + edgeIdx * lcuWidthInBaseUnits + baseUnitIdx];
        }
    }

    Void xSetEdgefilterMultiple(TComDataCU* cu, UInt absZOrderIdx, UInt depth, Int dir, Int edgeIdx, Bool bValue, UInt widthInBaseUnits = 0, UInt heightInBaseUnits = 0);

    Void xEdgeFilterLuma(TComDataCU* cu, UInt absZOrderIdx, UInt depth, Int dir, Int edge);
    Void xEdgeFilterChroma(TComDataCU* cu, UInt absZOrderIdx, UInt depth, Int dir, Int edge);

    inline Void xPelFilterLuma(Pel* src, Int offset, Int tc, Bool sw, Bool bPartPNoFilter, Bool bPartQNoFilter, Int iThrCut, Bool bFilterSecondP, Bool bFilterSecondQ);
    inline Void xPelFilterChroma(Pel* src, Int offset, Int tc, Bool bPartPNoFilter, Bool bPartQNoFilter);

    inline Bool xUseStrongFiltering(Int offset, Int d, Int beta, Int tc, Pel* src);
    inline Int xCalcDP(Pel* src, Int offset);
    inline Int xCalcDQ(Pel* src, Int offset);

    static const UChar sm_tcTable[54];
    static const UChar sm_betaTable[52];

public:

    TComLoopFilter();
    virtual ~TComLoopFilter();

    Void  create(UInt maxCUDepth);
    Void  destroy();

    /// set configuration
    Void setCfg(Bool bLFCrossTileBoundary);

    /// picture-level deblocking filter
    Void loopFilterPic(TComPic* pic);

    static Int getBeta(Int qp)
    {
        Int indexB = Clip3(0, MAX_QP, qp);

        return sm_betaTable[indexB];
    }
};

//! \}

#endif // ifndef __TCOMLOOPFILTER__
