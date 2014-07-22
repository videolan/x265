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
    \todo     this should be merged with TComPicYuv
*/

#ifndef X265_TCOMYUV_H
#define X265_TCOMYUV_H

#include "common.h"
#include "TComRom.h"
#include "primitives.h"

namespace x265 {
// private namespace

class ShortYuv;
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

    pixel* m_buf[3];

    // ------------------------------------------------------------------------------------------------------------------
    //  Parameter for general YUV buffer usage
    // ------------------------------------------------------------------------------------------------------------------

    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_cwidth;
    uint32_t m_cheight;

    int m_hChromaShift;
    int m_vChromaShift;
    int m_csp;

    int getChromaAddrOffset(uint32_t partUnitIdx, uint32_t width)
    {
        int blkX = g_rasterToPelX[g_zscanToRaster[partUnitIdx]] >> m_hChromaShift;
        int blkY = g_rasterToPelY[g_zscanToRaster[partUnitIdx]] >> m_vChromaShift;

        return blkX + blkY * width;
    }

    static int getAddrOffset(uint32_t partUnitIdx, uint32_t width)
    {
        int blkX = g_rasterToPelX[g_zscanToRaster[partUnitIdx]];
        int blkY = g_rasterToPelY[g_zscanToRaster[partUnitIdx]];

        return blkX + blkY * width;
    }

public:

    int m_part; // partitionFromSizes(m_width, m_height)

    TComYuv();
    ~TComYuv();

    // ------------------------------------------------------------------------------------------------------------------
    //  Memory management
    // ------------------------------------------------------------------------------------------------------------------

    bool    create(uint32_t width, uint32_t height, int csp); ///< Create  YUV buffer
    void    destroy();                                        ///< Destroy YUV buffer
    void    clear();                                          ///< clear   YUV buffer

    // ------------------------------------------------------------------------------------------------------------------
    //  Copy, load, store YUV buffer
    // ------------------------------------------------------------------------------------------------------------------

    //  Copy YUV buffer to picture buffer
    void    copyToPicYuv(TComPicYuv* destPicYuv, uint32_t cuAddr, uint32_t absZOrderIdx);

    //  Copy YUV buffer from picture buffer
    void    copyFromPicYuv(TComPicYuv* srcPicYuv, uint32_t cuAddr, uint32_t absZOrderIdx);

    //  Copy from same size YUV buffer
    void    copyFromYuv(TComYuv* srcYuv);

    //  Copy Small YUV buffer to the part of other Big YUV buffer
    void    copyToPartYuv(TComYuv* dstPicYuv, uint32_t partIdx);

    //  Copy the part of Big YUV buffer to other Small YUV buffer
    void    copyPartToYuv(TComYuv* dstPicYuv, uint32_t srcPartIdx);

    // ------------------------------------------------------------------------------------------------------------------
    //  Algebraic operation for YUV buffer
    // ------------------------------------------------------------------------------------------------------------------

    //  Clip(srcYuv0 + srcYuv1) -> m_apiBuf
    void    addClip(TComYuv* srcYuv0, ShortYuv* srcYuv1, uint32_t log2Size);
    void    addClipLuma(TComYuv* srcYuv0, ShortYuv* srcYuv1, uint32_t part);
    void    addClipChroma(TComYuv* srcYuv0, ShortYuv* srcYuv1, uint32_t part);

    //  (srcYuv0 + srcYuv1)/2 for YUV partition
    void    addAvg(ShortYuv* srcYuv0, ShortYuv* srcYuv1, uint32_t partUnitIdx, uint32_t width, uint32_t height, bool bLuma, bool bChroma);

    // ------------------------------------------------------------------------------------------------------------------
    //  Access function for YUV buffer
    // ------------------------------------------------------------------------------------------------------------------

    //  Access starting position of YUV buffer
    pixel* getLumaAddr()  { return m_buf[0]; }

    pixel* getCbAddr()    { return m_buf[1]; }

    pixel* getCrAddr()    { return m_buf[2]; }

    pixel* getChromaAddr(uint32_t chromaId)    { return m_buf[chromaId]; }

    //  Access starting position of YUV partition unit buffer
    pixel* getLumaAddr(uint32_t partUnitIdx) { return m_buf[0] + getAddrOffset(partUnitIdx, m_width); }

    pixel* getCbAddr(uint32_t partUnitIdx) { return m_buf[1] + getChromaAddrOffset(partUnitIdx, m_cwidth); }

    pixel* getCrAddr(uint32_t partUnitIdx) { return m_buf[2] + getChromaAddrOffset(partUnitIdx, m_cwidth); }

    pixel* getChromaAddr(uint32_t chromaId, uint32_t partUnitIdx) { return m_buf[chromaId] + getChromaAddrOffset(partUnitIdx, m_cwidth); }

    //  Get stride value of YUV buffer
    uint32_t getStride()    { return m_width;   }

    uint32_t getCStride()   { return m_cwidth;  }

    uint32_t getHeight()    { return m_height;  }

    uint32_t getWidth()     { return m_width;   }

    uint32_t getCHeight()   { return m_cheight; }

    uint32_t getCWidth()    { return m_cwidth;  }

    // -------------------------------------------------------------------------------------------------------------------
    // member functions to support multiple color space formats
    // -------------------------------------------------------------------------------------------------------------------

    int  getHorzChromaShift()  { return m_hChromaShift; }

    int  getVertChromaShift()  { return m_vChromaShift; }
};
}
//! \}

#endif // ifndef X265_TCOMYUV_H
