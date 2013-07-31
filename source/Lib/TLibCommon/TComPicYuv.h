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

#ifndef __TCOMPICYUV__
#define __TCOMPICYUV__

#include "CommonDef.h"
#include "TComRom.h"

#include "x265.h"
#include "reference.h"

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
    x265::MotionReference *m_refList;

    // ------------------------------------------------------------------------------------------------
    //  Parameter for general YUV buffer usage
    // ------------------------------------------------------------------------------------------------

    Int   m_picWidth;          ///< Width of picture
    Int   m_picHeight;         ///< Height of picture

    Int   m_cuWidth;           ///< Width of Coding Unit (CU)
    Int   m_cuHeight;          ///< Height of Coding Unit (CU)
    Int*  m_cuOffsetY;
    Int*  m_cuOffsetC;
    Int*  m_buOffsetY;
    Int*  m_buOffsetC;

    Int   m_lumaMarginX;
    Int   m_lumaMarginY;
    Int   m_chromaMarginX;
    Int   m_chromaMarginY;
    Int   m_stride;
    Int   m_strideC;

    Bool  m_bIsBorderExtended;

protected:

    Void xExtendPicCompBorder(Pel* recon, Int stride, Int width, Int height, Int marginX, Int marginY);

public:

    TComPicYuv();
    virtual ~TComPicYuv();

    // ------------------------------------------------------------------------------------------------
    //  Memory management
    // ------------------------------------------------------------------------------------------------

    Void  create(Int picWidth, Int picHeight, UInt maxCUWidth, UInt maxCUHeight, UInt maxCUDepth);
    Void  destroy();

    Void  createLuma(Int picWidth, Int picHeight, UInt maxCUWidth, UInt maxCUHeight, UInt maxCUDepth);
    Void  destroyLuma();

    Void  clearReferences();

    // ------------------------------------------------------------------------------------------------
    //  Get information of picture
    // ------------------------------------------------------------------------------------------------

    Int   getWidth()      { return m_picWidth; }

    Int   getHeight()     { return m_picHeight; }

    Int   getStride()     { return m_stride; }

    Int   getCStride()    { return m_strideC; }

    Int   getLumaMargin() { return m_lumaMarginX; }

    Int   getChromaMargin() { return m_chromaMarginX; }

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

    x265::MotionReference *getMotionReference(wpScalingParam *w); 

    //  Access starting position of original picture for specific coding unit (CU) or partition unit (PU)
    Pel*  getLumaAddr(Int cuAddr) { return m_picOrgY + m_cuOffsetY[cuAddr]; }

    Pel*  getCbAddr(Int cuAddr) { return m_picOrgU + m_cuOffsetC[cuAddr]; }

    Pel*  getCrAddr(Int cuAddr) { return m_picOrgV + m_cuOffsetC[cuAddr]; }

    Pel*  getLumaAddr(Int cuAddr, Int absZOrderIdx) { return m_picOrgY + m_cuOffsetY[cuAddr] + m_buOffsetY[g_zscanToRaster[absZOrderIdx]]; }

    Pel*  getCbAddr(Int cuAddr, Int absZOrderIdx) { return m_picOrgU + m_cuOffsetC[cuAddr] + m_buOffsetC[g_zscanToRaster[absZOrderIdx]]; }

    Pel*  getCrAddr(Int cuAddr, Int absZOrderIdx) { return m_picOrgV + m_cuOffsetC[cuAddr] + m_buOffsetC[g_zscanToRaster[absZOrderIdx]]; }

    /* Access functions for m_filteredBlock */
    Pel* getLumaFilterBlock(int ver, int hor) { return (Pel*)m_refList->m_lumaPlane[hor][ver]; }

    Pel* getLumaFilterBlock(int ver, int hor, Int cuAddr, Int absZOrderIdx) { return (Pel*)m_refList->m_lumaPlane[hor][ver] + m_cuOffsetY[cuAddr] + m_buOffsetY[g_zscanToRaster[absZOrderIdx]]; }

    // ------------------------------------------------------------------------------------------------
    //  Miscellaneous
    // ------------------------------------------------------------------------------------------------

    //  Copy function to picture
    Void  copyToPic(TComPicYuv* destYuv);
    Void  copyToPicLuma(TComPicYuv* destYuv);
    Void  copyToPicCb(TComPicYuv* destYuv);
    Void  copyToPicCr(TComPicYuv* destYuv);
    Void  copyFromPicture(const x265_picture_t&);

    //  Extend function of picture buffer
    Void  extendPicBorder(x265::ThreadPool *pool, wpScalingParam *w=NULL);

    //  Dump picture
    Void  dump(Char* pFileName, Bool bAdd = false);

    // Set border extension flag
    Void  setBorderExtension(Bool b) { m_bIsBorderExtended = b; }

    friend class x265::MotionReference;
}; // END CLASS DEFINITION TComPicYuv

void calcChecksum(TComPicYuv & pic, UChar digest[3][16]);
void calcCRC(TComPicYuv & pic, UChar digest[3][16]);
void calcMD5(TComPicYuv & pic, UChar digest[3][16]);
//! \}

#endif // __TCOMPICYUV__
