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

#include <stdio.h>
#include "CommonDef.h"
#include "TComRom.h"

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
  
  Pel*  m_apiPicBufY;           ///< Buffer (including margin)
  Pel*  m_apiPicBufU;
  Pel*  m_apiPicBufV;
  
  Pel*  m_piPicOrgY;            ///< m_apiPicBufY + m_iMarginLuma*getStride() + m_iMarginLuma
  Pel*  m_piPicOrgU;
  Pel*  m_piPicOrgV;
  
  // ------------------------------------------------------------------------------------------------
  //  Parameter for general YUV buffer usage
  // ------------------------------------------------------------------------------------------------
  
  Int   m_iPicWidth;            ///< Width of picture
  Int   m_iPicHeight;           ///< Height of picture
  
  Int   m_iCuWidth;             ///< Width of Coding Unit (CU)
  Int   m_iCuHeight;            ///< Height of Coding Unit (CU)
  Int*  m_cuOffsetY;
  Int*  m_cuOffsetC;
  Int*  m_buOffsetY;
  Int*  m_buOffsetC;
  
  Int   m_iLumaMarginX;
  Int   m_iLumaMarginY;
  Int   m_iChromaMarginX;
  Int   m_iChromaMarginY;
  
  Bool  m_bIsBorderExtended;
  
protected:
  Void  xExtendPicCompBorder (Pel* piTxt, Int iStride, Int iWidth, Int iHeight, Int iMarginX, Int iMarginY);
  
public:
  TComPicYuv         ();
  virtual ~TComPicYuv();
  
  // ------------------------------------------------------------------------------------------------
  //  Memory management
  // ------------------------------------------------------------------------------------------------
  
  Void  create      ( Int iPicWidth, Int iPicHeight, UInt uiMaxCUWidth, UInt uiMaxCUHeight, UInt uiMaxCUDepth );
  Void  destroy     ();
  
  Void  createLuma  ( Int iPicWidth, Int iPicHeight, UInt uiMaxCUWidth, UInt uiMaxCUHeight, UInt uhMaxCUDepth );
  Void  destroyLuma ();
  
  // ------------------------------------------------------------------------------------------------
  //  Get information of picture
  // ------------------------------------------------------------------------------------------------
  
  Int   getWidth    ()     { return  m_iPicWidth;    }
  Int   getHeight   ()     { return  m_iPicHeight;   }
  
  Int   getStride   ()     { return (m_iPicWidth     ) + (m_iLumaMarginX  <<1); }
  Int   getCStride  ()     { return (m_iPicWidth >> 1) + (m_iChromaMarginX<<1); }
  
  Int   getLumaMargin   () { return m_iLumaMarginX;  }
  Int   getChromaMargin () { return m_iChromaMarginX;}
  
  // ------------------------------------------------------------------------------------------------
  //  Access function for picture buffer
  // ------------------------------------------------------------------------------------------------
  
  //  Access starting position of picture buffer with margin
  Pel*  getBufY     ()     { return  m_apiPicBufY;   }
  Pel*  getBufU     ()     { return  m_apiPicBufU;   }
  Pel*  getBufV     ()     { return  m_apiPicBufV;   }
  
  //  Access starting position of original picture
  Pel*  getLumaAddr ()     { return  m_piPicOrgY;    }
  Pel*  getCbAddr   ()     { return  m_piPicOrgU;    }
  Pel*  getCrAddr   ()     { return  m_piPicOrgV;    }
  
  //  Access starting position of original picture for specific coding unit (CU) or partition unit (PU)
  Pel*  getLumaAddr ( Int iCuAddr ) { return m_piPicOrgY + m_cuOffsetY[ iCuAddr ]; }
  Pel*  getCbAddr   ( Int iCuAddr ) { return m_piPicOrgU + m_cuOffsetC[ iCuAddr ]; }
  Pel*  getCrAddr   ( Int iCuAddr ) { return m_piPicOrgV + m_cuOffsetC[ iCuAddr ]; }
  Pel*  getLumaAddr ( Int iCuAddr, Int uiAbsZorderIdx ) { return m_piPicOrgY + m_cuOffsetY[iCuAddr] + m_buOffsetY[g_auiZscanToRaster[uiAbsZorderIdx]]; }
  Pel*  getCbAddr   ( Int iCuAddr, Int uiAbsZorderIdx ) { return m_piPicOrgU + m_cuOffsetC[iCuAddr] + m_buOffsetC[g_auiZscanToRaster[uiAbsZorderIdx]]; }
  Pel*  getCrAddr   ( Int iCuAddr, Int uiAbsZorderIdx ) { return m_piPicOrgV + m_cuOffsetC[iCuAddr] + m_buOffsetC[g_auiZscanToRaster[uiAbsZorderIdx]]; }
  
  // ------------------------------------------------------------------------------------------------
  //  Miscellaneous
  // ------------------------------------------------------------------------------------------------
  
  //  Copy function to picture
  Void  copyToPic       ( TComPicYuv*  pcPicYuvDst );
  Void  copyToPicLuma   ( TComPicYuv*  pcPicYuvDst );
  Void  copyToPicCb     ( TComPicYuv*  pcPicYuvDst );
  Void  copyToPicCr     ( TComPicYuv*  pcPicYuvDst );
  
  //  Extend function of picture buffer
  Void  extendPicBorder      ();
  
  //  Dump picture
  Void  dump (Char* pFileName, Bool bAdd = false);
  
  // Set border extension flag
  Void  setBorderExtension(Bool b) { m_bIsBorderExtended = b; }
};// END CLASS DEFINITION TComPicYuv

void calcChecksum(TComPicYuv& pic, UChar digest[3][16]);
void calcCRC(TComPicYuv& pic, UChar digest[3][16]);
void calcMD5(TComPicYuv& pic, UChar digest[3][16]);
//! \}

#endif // __TCOMPICYUV__
