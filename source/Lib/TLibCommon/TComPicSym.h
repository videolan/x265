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

/** \file     TComPicSym.h
    \brief    picture symbol class (header)
*/

#ifndef __TCOMPICSYM__
#define __TCOMPICSYM__

// Include files
#include "CommonDef.h"
#include "TComSlice.h"
#include "TComDataCU.h"
class TComSampleAdaptiveOffset;

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// picture symbol class
class TComPicSym
{
private:

    UInt          m_uiWidthInCU;
    UInt          m_uiHeightInCU;

    UInt          m_uiMaxCUWidth;
    UInt          m_uiMaxCUHeight;
    UInt          m_uiMinCUWidth;
    UInt          m_uiMinCUHeight;

    UChar         m_uhTotalDepth;     ///< max. depth
    UInt          m_uiNumPartitions;
    UInt          m_uiNumPartInWidth;
    UInt          m_uiNumPartInHeight;
    UInt          m_uiNumCUsInFrame;

    TComSlice*    m_pcTComSlice;
    TComDataCU**  m_apcTComDataCU;      ///< array of CU data

    SAOParam*     m_saoParam;

public:

    Void        create(Int iPicWidth, Int iPicHeight, UInt uiMaxWidth, UInt uiMaxHeight, UInt uiMaxDepth);
    Void        destroy();

    TComPicSym();

    TComSlice*  getSlice()                { return m_pcTComSlice; }

    UInt        getFrameWidthInCU()       { return m_uiWidthInCU; }

    UInt        getFrameHeightInCU()      { return m_uiHeightInCU; }

    UInt        getMinCUWidth()           { return m_uiMinCUWidth; }

    UInt        getMinCUHeight()          { return m_uiMinCUHeight; }

    UInt        getNumberOfCUsInFrame()   { return m_uiNumCUsInFrame; }

    TComDataCU*&  getCU(UInt uiCUAddr)    { return m_apcTComDataCU[uiCUAddr]; }

    UInt        getNumPartition()         { return m_uiNumPartitions; }

    UInt        getNumPartInWidth()       { return m_uiNumPartInWidth; }

    UInt        getNumPartInHeight()      { return m_uiNumPartInHeight; }

    Void allocSaoParam(TComSampleAdaptiveOffset *sao);
    SAOParam *getSaoParam() { return m_saoParam; }
}; // END CLASS DEFINITION TComPicSym

//! \}

#endif // __TCOMPICSYM__
