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

#ifndef X265_TCOMYUV_H
#define X265_TCOMYUV_H

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

    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_cwidth;
    uint32_t m_cheight;

    int m_hChromaShift;
    int m_vChromaShift;

    static int getAddrOffset(uint32_t partUnitIdx, uint32_t width)
    {
        int blkX = g_rasterToPelX[g_zscanToRaster[partUnitIdx]];
        int blkY = g_rasterToPelY[g_zscanToRaster[partUnitIdx]];

        return blkX + blkY * width;
    }

    static int getAddrOffset(uint32_t unitIdx, uint32_t size, uint32_t width)
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

    void    create(uint32_t width, uint32_t height, int csp); ///< Create  YUV buffer
    void    destroy();                                        ///< Destroy YUV buffer
    void    clear();                                          ///< clear   YUV buffer

    // ------------------------------------------------------------------------------------------------------------------
    //  Copy, load, store YUV buffer
    // ------------------------------------------------------------------------------------------------------------------

    //  Copy YUV buffer to picture buffer
    void    copyToPicYuv(TComPicYuv* destPicYuv, uint32_t cuAddr, uint32_t absZOrderIdx, uint32_t partDepth = 0, uint32_t partIdx = 0);
    void    copyToPicLuma(TComPicYuv* destPicYuv, uint32_t cuAddr, uint32_t absZOrderIdx, uint32_t partDepth = 0, uint32_t partIdx = 0);
    void    copyToPicChroma(TComPicYuv* destPicYuv, uint32_t cuAddr, uint32_t absZOrderIdx, uint32_t partDepth = 0, uint32_t partIdx = 0);

    //  Copy YUV buffer from picture buffer
    void    copyFromPicYuv(TComPicYuv* srcPicYuv, uint32_t cuAddr, uint32_t absZOrderIdx);
    void    copyFromPicLuma(TComPicYuv* srcPicYuv, uint32_t cuAddr, uint32_t absZOrderIdx);
    void    copyFromPicChroma(TComPicYuv* srcPicYuv, uint32_t cuAddr, uint32_t absZOrderIdx);

    //  Copy Small YUV buffer to the part of other Big YUV buffer
    void    copyToPartYuv(TComYuv* dstPicYuv, uint32_t uiDstPartIdx);
    void    copyToPartLuma(TComYuv* dstPicYuv, uint32_t uiDstPartIdx);
    void    copyToPartChroma(TComYuv* dstPicYuv, uint32_t uiDstPartIdx);

    //  Copy the part of Big YUV buffer to other Small YUV buffer
    void    copyPartToYuv(TComYuv* dstPicYuv, uint32_t uiSrcPartIdx);
    void    copyPartToLuma(TComYuv* dstPicYuv, uint32_t uiSrcPartIdx);
    void    copyPartToChroma(TComYuv* dstPicYuv, uint32_t uiSrcPartIdx);

    //  Copy YUV partition buffer to other YUV partition buffer
    void    copyPartToPartYuv(TComYuv* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height, bool bLuma = true, bool bChroma = true);
    void    copyPartToPartYuv(TShortYUV* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height, bool bLuma = true, bool bChroma = true);
    void    copyPartToPartLuma(TComYuv* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height);
    void    copyPartToPartLuma(TShortYUV* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height);
    void    copyPartToPartChroma(TComYuv* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height);
    void    copyPartToPartChroma(TShortYUV* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height);

    void    copyPartToPartChroma(TComYuv* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height, uint32_t chromaId);
    void    copyPartToPartChroma(TShortYUV* dstPicYuv, uint32_t partIdx, uint32_t width, uint32_t height, uint32_t chromaId);

    // ------------------------------------------------------------------------------------------------------------------
    //  Algebraic operation for YUV buffer
    // ------------------------------------------------------------------------------------------------------------------

    //  Clip(srcYuv0 + srcYuv1) -> m_apiBuf
    void    addClip(TComYuv* srcYuv0, TComYuv* srcYuv1, uint32_t trUnitIdx, uint32_t partSize);
    void    addClip(TComYuv* srcYuv0, TShortYUV* srcYuv1, uint32_t trUnitIdx, uint32_t partSize);
    void    addClipLuma(TComYuv* srcYuv0, TComYuv* srcYuv1, uint32_t trUnitIdx, uint32_t partSize);
    void    addClipLuma(TComYuv* srcYuv0, TShortYUV* srcYuv1, uint32_t trUnitIdx, uint32_t partSize);
    void    addClipChroma(TComYuv* srcYuv0, TComYuv* srcYuv1, uint32_t trUnitIdx, uint32_t partSize);
    void    addClipChroma(TComYuv* srcYuv0, TShortYUV* srcYuv1, uint32_t trUnitIdx, uint32_t partSize);

    //  srcYuv0 - srcYuv1 -> m_apiBuf
    void    subtract(TComYuv* srcYuv0, TComYuv* srcYuv1, uint32_t trUnitIdx, uint32_t partSize);
    void    subtractLuma(TComYuv* srcYuv0, TComYuv* srcYuv1, uint32_t trUnitIdx, uint32_t partSize);
    void    subtractChroma(TComYuv* srcYuv0, TComYuv* srcYuv1, uint32_t trUnitIdx, uint32_t partSize);

    //  (srcYuv0 + srcYuv1)/2 for YUV partition
    void    addAvg(TComYuv* srcYuv0, TComYuv* srcYuv1, uint32_t partUnitIdx, uint32_t width, uint32_t height, bool bLuma = true, bool bChroma = true);
    void    addAvg(TShortYUV* srcYuv0, TShortYUV* srcYuv1, uint32_t partUnitIdx, uint32_t width, uint32_t height, bool bLuma = true, bool bChroma = true);

    //   Remove High frequency
    void    removeHighFreq(TComYuv* srcYuv, uint32_t partIdx, uint32_t width, uint32_t height);

    // ------------------------------------------------------------------------------------------------------------------
    //  Access function for YUV buffer
    // ------------------------------------------------------------------------------------------------------------------

    //  Access starting position of YUV buffer
    Pel* getLumaAddr()  { return m_bufY; }

    Pel* getCbAddr()    { return m_bufU; }

    Pel* getCrAddr()    { return m_bufV; }

    //  Access starting position of YUV partition unit buffer
    Pel* getLumaAddr(uint32_t partUnitIdx) { return m_bufY + getAddrOffset(partUnitIdx, m_width); }

    Pel* getCbAddr(uint32_t partUnitIdx) { return m_bufU + (getAddrOffset(partUnitIdx, m_cwidth) >> 1); }

    Pel* getCrAddr(uint32_t partUnitIdx) { return m_bufV + (getAddrOffset(partUnitIdx, m_cwidth) >> 1); }

    //  Access starting position of YUV transform unit buffer
    Pel* getLumaAddr(uint32_t iTransUnitIdx, uint32_t iBlkSize) { return m_bufY + getAddrOffset(iTransUnitIdx, iBlkSize, m_width); }

    Pel* getCbAddr(uint32_t iTransUnitIdx, uint32_t iBlkSize) { return m_bufU + getAddrOffset(iTransUnitIdx, iBlkSize, m_cwidth); }

    Pel* getCrAddr(uint32_t iTransUnitIdx, uint32_t iBlkSize) { return m_bufV + getAddrOffset(iTransUnitIdx, iBlkSize, m_cwidth); }

    //  Get stride value of YUV buffer
    uint32_t getStride()    { return m_width;   }

    uint32_t getCStride()   { return m_cwidth;  }

    uint32_t getHeight()    { return m_height;  }

    uint32_t getWidth()     { return m_width;   }

    uint32_t getCHeight()   { return m_cheight; }

    uint32_t getCWidth()    { return m_cwidth;  }
}; // END CLASS DEFINITION TComYuv
}
//! \}

#endif // ifndef X265_TCOMYUV_H
