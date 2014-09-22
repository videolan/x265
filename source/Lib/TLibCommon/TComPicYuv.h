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

/** \file     TComPicYuv.h
    \brief    picture YUV buffer class (header)
*/

#ifndef X265_TCOMPICYUV_H
#define X265_TCOMPICYUV_H

#include "common.h"
#include "TComRom.h"
#include "x265.h"
#include "md5.h"

namespace x265 {
// private namespace

class ShortYuv;

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// picture YUV buffer class
class TComPicYuv
{
public:

    // ------------------------------------------------------------------------------------------------
    //  YUV buffer
    // ------------------------------------------------------------------------------------------------

    pixel*  m_picBuf[3];        ///< Buffer (including margin)

    pixel*  m_picOrg[3];        ///< m_apiPicBufY + m_iMarginLuma*getStride() + m_iMarginLuma

    // ------------------------------------------------------------------------------------------------
    //  Parameter for general YUV buffer usage
    // ------------------------------------------------------------------------------------------------
    int   m_picWidth;          ///< Width of picture
    int   m_picHeight;         ///< Height of picture
    int   m_picCsp;            ///< Picture color format
    int   m_hChromaShift;
    int   m_vChromaShift;

    int   m_cuSize;            ///< Width/Height of Coding Unit (CU)
    int32_t*  m_cuOffsetY;
    int32_t*  m_cuOffsetC;
    int32_t*  m_buOffsetY;
    int32_t*  m_buOffsetC;

    int   m_lumaMarginX;
    int   m_lumaMarginY;
    int   m_chromaMarginX;
    int   m_chromaMarginY;
    int   m_stride;
    int   m_strideC;

    int   m_numCuInWidth;
    int   m_numCuInHeight;

    TComPicYuv();
    ~TComPicYuv() {}

    // ------------------------------------------------------------------------------------------------
    //  Memory management
    // ------------------------------------------------------------------------------------------------

    bool  create(int picWidth, int picHeight, int csp, uint32_t maxCUSize, uint32_t maxFullDepth);
    void  destroy();

    // ------------------------------------------------------------------------------------------------
    //  Get information of picture
    // ------------------------------------------------------------------------------------------------

    int   getWidth()      { return m_picWidth; }

    int   getHeight()     { return m_picHeight; }

    int   getStride()     { return m_stride; }

    int   getCStride()    { return m_strideC; }

    int   getLumaMarginX() { return m_lumaMarginX; }

    int   getLumaMarginY() { return m_lumaMarginY; }

    int   getChromaMarginX() { return m_chromaMarginX; }

    int   getChromaMarginY() { return m_chromaMarginY; }

    // ------------------------------------------------------------------------------------------------
    //  Access function for picture buffer
    // ------------------------------------------------------------------------------------------------

    //  Access starting position of original picture
    pixel*  getLumaAddr()   { return m_picOrg[0]; }

    pixel*  getCbAddr()     { return m_picOrg[1]; }

    pixel*  getCrAddr()     { return m_picOrg[2]; }

    pixel*  getChromaAddr(uint32_t chromaId)     { return m_picOrg[chromaId]; }

    //  Access starting position of original picture for specific coding unit (CU) or partition unit (PU)
    pixel*  getLumaAddr(int cuAddr) { return m_picOrg[0] + m_cuOffsetY[cuAddr]; }

    pixel*  getPlaneAddr(int plane, int cuAddr) { return m_picOrg[plane] + (plane ? m_cuOffsetC[cuAddr] : m_cuOffsetY[cuAddr]); }

    pixel*  getCbAddr(int cuAddr) { return m_picOrg[1] + m_cuOffsetC[cuAddr]; }

    pixel*  getCrAddr(int cuAddr) { return m_picOrg[2] + m_cuOffsetC[cuAddr]; }

    pixel*  getChromaAddr(uint32_t chromaId, int cuAddr) { return m_picOrg[chromaId] + m_cuOffsetC[cuAddr]; }

    pixel*  getLumaAddr(int cuAddr, int absZOrderIdx) { return m_picOrg[0] + m_cuOffsetY[cuAddr] + m_buOffsetY[absZOrderIdx]; }

    pixel*  getCbAddr(int cuAddr, int absZOrderIdx) { return m_picOrg[1] + m_cuOffsetC[cuAddr] + m_buOffsetC[absZOrderIdx]; }

    pixel*  getCrAddr(int cuAddr, int absZOrderIdx) { return m_picOrg[2] + m_cuOffsetC[cuAddr] + m_buOffsetC[absZOrderIdx]; }

    pixel*  getChromaAddr(uint32_t chromaId, int cuAddr, int absZOrderIdx) { return m_picOrg[chromaId] + m_cuOffsetC[cuAddr] + m_buOffsetC[absZOrderIdx]; }

    uint32_t getCUHeight(int rowNum);

    void  copyFromPicture(const x265_picture&, int padx, int pady);
}; // END CLASS DEFINITION TComPicYuv

void updateChecksum(const pixel* plane, uint32_t& checksumVal, uint32_t height, uint32_t width, uint32_t stride, int row, uint32_t cuHeight);
void updateCRC(const pixel* plane, uint32_t& crcVal, uint32_t height, uint32_t width, uint32_t stride);
void crcFinish(uint32_t & crc, uint8_t digest[16]);
void checksumFinish(uint32_t checksum, uint8_t digest[16]);
void updateMD5Plane(MD5Context& md5, const pixel* plane, uint32_t width, uint32_t height, uint32_t stride);
}
//! \}

#endif // ifndef X265_TCOMPICYUV_H
