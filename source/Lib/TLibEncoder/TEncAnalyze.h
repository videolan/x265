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

/** \file     TEncAnalyze.h
    \brief    encoder analyzer class (header)
*/

#ifndef __TENCANALYZE__
#define __TENCANALYZE__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <stdio.h>
#include <memory.h>
#include <assert.h>
#include "TLibCommon/CommonDef.h"

//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// encoder analyzer class
class TEncAnalyze
{
private:
  Double    m_dPSNRSumY;
  Double    m_dPSNRSumU;
  Double    m_dPSNRSumV;
  Double    m_dAddBits;
  UInt      m_uiNumPic;
  Double    m_dFrmRate; //--CFG_KDY
  
public:
  TEncAnalyze() { m_dPSNRSumY = m_dPSNRSumU = m_dPSNRSumV = m_dAddBits = m_uiNumPic = 0;  }
  virtual ~TEncAnalyze()  {}
  
  Void  addResult( Double psnrY, Double psnrU, Double psnrV, Double bits)
  {
    m_dPSNRSumY += psnrY;
    m_dPSNRSumU += psnrU;
    m_dPSNRSumV += psnrV;
    m_dAddBits  += bits;
    
    m_uiNumPic++;
  }
  
  Double  getPsnrY()  { return  m_dPSNRSumY;  }
  Double  getPsnrU()  { return  m_dPSNRSumU;  }
  Double  getPsnrV()  { return  m_dPSNRSumV;  }
  Double  getBits()   { return  m_dAddBits;   }
  UInt    getNumPic() { return  m_uiNumPic;   }
  
  Void    setFrmRate  (Double dFrameRate) { m_dFrmRate = dFrameRate; } //--CFG_KDY
  Void    clear() { m_dPSNRSumY = m_dPSNRSumU = m_dPSNRSumV = m_dAddBits = m_uiNumPic = 0;  }
  Void    printOut ( Char cDelim )
  {
    Double dFps     =   m_dFrmRate; //--CFG_KDY
    Double dScale   = dFps / 1000 / (Double)m_uiNumPic;
    
    printf( "\tTotal Frames |  "   "Bitrate    "  "Y-PSNR    "  "U-PSNR    "  "V-PSNR \n" );
    //printf( "\t------------ "  " ----------"   " -------- "  " -------- "  " --------\n" );
    printf( "\t %8d    %c"          "%12.4lf  "    "%8.4lf  "   "%8.4lf  "    "%8.4lf\n",
           getNumPic(), cDelim,
           getBits() * dScale,
           getPsnrY() / (Double)getNumPic(),
           getPsnrU() / (Double)getNumPic(),
           getPsnrV() / (Double)getNumPic() );
  }
  
  Void    printSummaryOut ()
  {
    FILE* pFile = fopen ("summaryTotal.txt", "at");
    Double dFps     =   m_dFrmRate; //--CFG_KDY
    Double dScale   = dFps / 1000 / (Double)m_uiNumPic;
    
    fprintf(pFile, "%f\t %f\t %f\t %f\n", getBits() * dScale,
            getPsnrY() / (Double)getNumPic(),
            getPsnrU() / (Double)getNumPic(),
            getPsnrV() / (Double)getNumPic() );
    fclose(pFile);
  }
  
  Void    printSummary(Char ch)
  {
    FILE* pFile = NULL;
    
    switch( ch ) 
    {
      case 'I':
        pFile = fopen ("summary_I.txt", "at");
        break;
      case 'P':
        pFile = fopen ("summary_P.txt", "at");
        break;
      case 'B':
        pFile = fopen ("summary_B.txt", "at");
        break;
      default:
        assert(0);
        return;
        break;
    }
    
    Double dFps     =   m_dFrmRate; //--CFG_KDY
    Double dScale   = dFps / 1000 / (Double)m_uiNumPic;
    
    fprintf(pFile, "%f\t %f\t %f\t %f\n",
            getBits() * dScale,
            getPsnrY() / (Double)getNumPic(),
            getPsnrU() / (Double)getNumPic(),
            getPsnrV() / (Double)getNumPic() );
    
    fclose(pFile);
  }
};

extern TEncAnalyze             m_gcAnalyzeAll;
extern TEncAnalyze             m_gcAnalyzeI;
extern TEncAnalyze             m_gcAnalyzeP;
extern TEncAnalyze             m_gcAnalyzeB;

//! \}

#endif // !defined(AFX_TENCANALYZE_H__C79BCAA2_6AC8_4175_A0FE_CF02F5829233__INCLUDED_)
