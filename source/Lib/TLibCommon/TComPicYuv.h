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

#include "CommonDef.h"
#include "TComRom.h"

#include "x265.h"
#include "reference.h"

namespace x265 {
// private namespace

class TShortYUV;

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// picture YUV buffer class
class TComPicYuv
{
private:

    // ------------------------------------------------------------------------------------------------
    //  YUV buffer
    // ------------------------------------------------------------------------------------------------

    Pel*  m_picBufY;         ///< Buffer (including margin)
    Pel*  m_picBufU;
    Pel*  m_picBufV;

    Pel*  m_picOrgY;          ///< m_apiPicBufY + m_iMarginLuma*getStride() + m_iMarginLuma
    Pel*  m_picOrgU;
    Pel*  m_picOrgV;

    // Pre-interpolated reference pictures for each QPEL offset, may be more than
    // one if weighted references are in use
    MotionReference *m_refList;

    // ------------------------------------------------------------------------------------------------
    //  Parameter for general YUV buffer usage
    // ------------------------------------------------------------------------------------------------
    int   m_picWidth;          ///< Width of picture
    int   m_picHeight;         ///< Height of picture

    int   m_cuWidth;           ///< Width of Coding Unit (CU)
    int   m_cuHeight;          ///< Height of Coding Unit (CU)
    int*  m_cuOffsetY;
    int*  m_cuOffsetC;
    int*  m_buOffsetY;
    int*  m_buOffsetC;

    int   m_lumaMarginX;
    int   m_lumaMarginY;
    int   m_chromaMarginX;
    int   m_chromaMarginY;
    int   m_stride;
    int   m_strideC;

public:

    int   m_numCuInWidth;
    int   m_numCuInHeight;

    TComPicYuv();
    virtual ~TComPicYuv();

    void xExtendPicCompBorder(Pel* recon, int stride, int width, int height, int marginX, int marginY);
    // ------------------------------------------------------------------------------------------------
    //  Memory management
    // ------------------------------------------------------------------------------------------------

    void  create(int picWidth, int picHeight, UInt maxCUWidth, UInt maxCUHeight, UInt maxCUDepth);
    void  destroy();

    void  createLuma(int picWidth, int picHeight, UInt maxCUWidth, UInt maxCUHeight, UInt maxCUDepth);
    void  destroyLuma();

    void  clearReferences();

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

    //  Access starting position of picture buffer with margin
    Pel*  getBufY()     { return m_picBufY; }

    Pel*  getBufU()     { return m_picBufU; }

    Pel*  getBufV()     { return m_picBufV; }

    //  Access starting position of original picture
    Pel*  getLumaAddr()   { return m_picOrgY; }

    Pel*  getCbAddr()     { return m_picOrgU; }

    Pel*  getCrAddr()     { return m_picOrgV; }

    //  Access starting position of original picture for specific coding unit (CU) or partition unit (PU)
    Pel*  getLumaAddr(int cuAddr) { return m_picOrgY + m_cuOffsetY[cuAddr]; }

    Pel*  getCbAddr(int cuAddr) { return m_picOrgU + m_cuOffsetC[cuAddr]; }

    Pel*  getCrAddr(int cuAddr) { return m_picOrgV + m_cuOffsetC[cuAddr]; }

    Pel*  getLumaAddr(int cuAddr, int absZOrderIdx) { return m_picOrgY + m_cuOffsetY[cuAddr] + m_buOffsetY[g_zscanToRaster[absZOrderIdx]]; }

    Pel*  getCbAddr(int cuAddr, int absZOrderIdx) { return m_picOrgU + m_cuOffsetC[cuAddr] + m_buOffsetC[g_zscanToRaster[absZOrderIdx]]; }

    Pel*  getCrAddr(int cuAddr, int absZOrderIdx) { return m_picOrgV + m_cuOffsetC[cuAddr] + m_buOffsetC[g_zscanToRaster[absZOrderIdx]]; }

    // ------------------------------------------------------------------------------------------------
    //  Miscellaneous
    // ------------------------------------------------------------------------------------------------

    //  Copy function to picture
    void  copyToPic(TComPicYuv* destYuv);
    void  copyToPicLuma(TComPicYuv* destYuv);
    void  copyToPicCb(TComPicYuv* destYuv);
    void  copyToPicCr(TComPicYuv* destYuv);
    void  copyFromPicture(const x265_picture_t&, int *pad);

    MotionReference* generateMotionReference(wpScalingParam *w);

    //  Dump picture
    void  dump(char* pFileName, bool bAdd = false);

    friend class MotionReference;
}; // END CLASS DEFINITION TComPicYuv

void calcChecksum(TComPicYuv & pic, UChar digest[3][16]);
void calcCRC(TComPicYuv & pic, UChar digest[3][16]);
void calcMD5(TComPicYuv & pic, UChar digest[3][16]);
}
//! \}

#endif // ifndef X265_TCOMPICYUV_H
