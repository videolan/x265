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

/** \file     TComYuv.h
    \brief    general YUV buffer class (header)
    \todo     this should be merged with TComPicYuv \n
              check usage of removeHighFreq function
*/

#ifndef __TCOMYUV__
#define __TCOMYUV__

#include "CommonDef.h"
#include "TComRom.h"

namespace x265 {
// private namespace

class TShortYUV;
class TComPicYuv;

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// general YUV buffer class
class TComYuv
{
private:

    // ------------------------------------------------------------------------------------------------------------------
    //  YUV buffer
    // ------------------------------------------------------------------------------------------------------------------

    Pel* m_bufY;
    Pel* m_bufU;
    Pel* m_bufV;

    // ------------------------------------------------------------------------------------------------------------------
    //  Parameter for general YUV buffer usage
    // ------------------------------------------------------------------------------------------------------------------

    UInt m_width;
    UInt m_height;
    UInt m_cwidth;
    UInt m_cheight;

    static int getAddrOffset(UInt partUnitIdx, UInt width)
    {
        int blkX = g_rasterToPelX[g_zscanToRaster[partUnitIdx]];
        int blkY = g_rasterToPelY[g_zscanToRaster[partUnitIdx]];

        return blkX + blkY * width;
    }

    static int getAddrOffset(UInt unitIdx, UInt size, UInt width)
    {
        int blkX = (unitIdx * size) &  (width - 1);
        int blkY = (unitIdx * size) & ~(width - 1);

        return blkX + blkY * size;
    }

public:

    TComYuv();
    virtual ~TComYuv();

    // ------------------------------------------------------------------------------------------------------------------
    //  Memory management
    // ------------------------------------------------------------------------------------------------------------------

    void    create(UInt width, UInt height);              ///< Create  YUV buffer
    void    destroy();                                      ///< Destroy YUV buffer
    void    clear();                                        ///< clear   YUV buffer

    // ------------------------------------------------------------------------------------------------------------------
    //  Copy, load, store YUV buffer
    // ------------------------------------------------------------------------------------------------------------------

    //  Copy YUV buffer to picture buffer
    void    copyToPicYuv(TComPicYuv* destPicYuv, UInt cuAddr, UInt absZOrderIdx, UInt partDepth = 0, UInt partIdx = 0);
    void    copyToPicLuma(TComPicYuv* destPicYuv, UInt cuAddr, UInt absZOrderIdx, UInt partDepth = 0, UInt partIdx = 0);
    void    copyToPicChroma(TComPicYuv* destPicYuv, UInt cuAddr, UInt absZOrderIdx, UInt partDepth = 0, UInt partIdx = 0);

    //  Copy YUV buffer from picture buffer
    void    copyFromPicYuv(TComPicYuv* srcPicYuv, UInt cuAddr, UInt absZOrderIdx);
    void    copyFromPicLuma(TComPicYuv* srcPicYuv, UInt cuAddr, UInt absZOrderIdx);
    void    copyFromPicChroma(TComPicYuv* srcPicYuv, UInt cuAddr, UInt absZOrderIdx);

    //  Copy Small YUV buffer to the part of other Big YUV buffer
    void    copyToPartYuv(TComYuv* dstPicYuv, UInt uiDstPartIdx);
    void    copyToPartLuma(TComYuv* dstPicYuv, UInt uiDstPartIdx);
    void    copyToPartChroma(TComYuv* dstPicYuv, UInt uiDstPartIdx);

    //  Copy the part of Big YUV buffer to other Small YUV buffer
    void    copyPartToYuv(TComYuv* dstPicYuv, UInt uiSrcPartIdx);
    void    copyPartToLuma(TComYuv* dstPicYuv, UInt uiSrcPartIdx);
    void    copyPartToChroma(TComYuv* dstPicYuv, UInt uiSrcPartIdx);

    //  Copy YUV partition buffer to other YUV partition buffer
    void    copyPartToPartYuv(TComYuv* dstPicYuv, UInt partIdx, UInt width, UInt height);
    void    copyPartToPartYuv(TShortYUV* dstPicYuv, UInt partIdx, UInt width, UInt height);
    void    copyPartToPartLuma(TComYuv* dstPicYuv, UInt partIdx, UInt width, UInt height);
    void    copyPartToPartLuma(TShortYUV* dstPicYuv, UInt partIdx, UInt width, UInt height);
    void    copyPartToPartChroma(TComYuv* dstPicYuv, UInt partIdx, UInt width, UInt height);
    void    copyPartToPartChroma(TShortYUV* dstPicYuv, UInt partIdx, UInt width, UInt height);

    void    copyPartToPartChroma(TComYuv* dstPicYuv, UInt partIdx, UInt width, UInt height, UInt chromaId);
    void    copyPartToPartChroma(TShortYUV* dstPicYuv, UInt partIdx, UInt width, UInt height, UInt chromaId);

    // ------------------------------------------------------------------------------------------------------------------
    //  Algebraic operation for YUV buffer
    // ------------------------------------------------------------------------------------------------------------------

    //  Clip(srcYuv0 + srcYuv1) -> m_apiBuf
    void    addClip(TComYuv* srcYuv0, TComYuv* srcYuv1, UInt trUnitIdx, UInt partSize);
    void    addClip(TComYuv* srcYuv0, TShortYUV* srcYuv1, UInt trUnitIdx, UInt partSize);
    void    addClipLuma(TComYuv* srcYuv0, TComYuv* srcYuv1, UInt trUnitIdx, UInt partSize);
    void    addClipLuma(TComYuv* srcYuv0, TShortYUV* srcYuv1, UInt trUnitIdx, UInt partSize);
    void    addClipChroma(TComYuv* srcYuv0, TComYuv* srcYuv1, UInt trUnitIdx, UInt partSize);
    void    addClipChroma(TComYuv* srcYuv0, TShortYUV* srcYuv1, UInt trUnitIdx, UInt partSize);

    //  srcYuv0 - srcYuv1 -> m_apiBuf
    void    subtract(TComYuv* srcYuv0, TComYuv* srcYuv1, UInt trUnitIdx, UInt partSize);
    void    subtractLuma(TComYuv* srcYuv0, TComYuv* srcYuv1, UInt trUnitIdx, UInt partSize);
    void    subtractChroma(TComYuv* srcYuv0, TComYuv* srcYuv1, UInt trUnitIdx, UInt partSize);

    //  (srcYuv0 + srcYuv1)/2 for YUV partition
    void    addAvg(TComYuv* srcYuv0, TComYuv* srcYuv1, UInt partUnitIdx, UInt width, UInt height);
    void    addAvg(TShortYUV* srcYuv0, TShortYUV* srcYuv1, UInt partUnitIdx, UInt width, UInt height);

    //   Remove High frequency
    void    removeHighFreq(TComYuv* srcYuv, UInt partIdx, UInt width, UInt height);

    // ------------------------------------------------------------------------------------------------------------------
    //  Access function for YUV buffer
    // ------------------------------------------------------------------------------------------------------------------

    //  Access starting position of YUV buffer
    Pel* getLumaAddr()  { return m_bufY; }

    Pel* getCbAddr()    { return m_bufU; }

    Pel* getCrAddr()    { return m_bufV; }

    //  Access starting position of YUV partition unit buffer
    Pel* getLumaAddr(UInt partUnitIdx) { return m_bufY + getAddrOffset(partUnitIdx, m_width); }

    Pel* getCbAddr(UInt partUnitIdx) { return m_bufU + (getAddrOffset(partUnitIdx, m_cwidth) >> 1); }

    Pel* getCrAddr(UInt partUnitIdx) { return m_bufV + (getAddrOffset(partUnitIdx, m_cwidth) >> 1); }

    //  Access starting position of YUV transform unit buffer
    Pel* getLumaAddr(UInt iTransUnitIdx, UInt iBlkSize) { return m_bufY + getAddrOffset(iTransUnitIdx, iBlkSize, m_width); }

    Pel* getCbAddr(UInt iTransUnitIdx, UInt iBlkSize) { return m_bufU + getAddrOffset(iTransUnitIdx, iBlkSize, m_cwidth); }

    Pel* getCrAddr(UInt iTransUnitIdx, UInt iBlkSize) { return m_bufV + getAddrOffset(iTransUnitIdx, iBlkSize, m_cwidth); }

    //  Get stride value of YUV buffer
    UInt getStride()    { return m_width;   }

    UInt getCStride()   { return m_cwidth;  }

    UInt getHeight()    { return m_height;  }

    UInt getWidth()     { return m_width;   }

    UInt getCHeight()   { return m_cheight; }

    UInt getCWidth()    { return m_cwidth;  }
}; // END CLASS DEFINITION TComYuv
}
//! \}

#endif // __TCOMYUV__
