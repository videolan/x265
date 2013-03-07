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

/** \file     SyntaxElementWriter.cpp
    \brief    CAVLC encoder class
*/

#include "TLibCommon/CommonDef.h"
#include "SyntaxElementWriter.h"

//! \ingroup TLibEncoder
//! \{

#if ENC_DEC_TRACE

Void  SyntaxElementWriter::xWriteCodeTr (UInt value, UInt  length, const Char *pSymbolName)
{
  xWriteCode (value,length);
  if( g_HLSTraceEnable )
  {
    fprintf( g_hTrace, "%8lld  ", g_nSymbolCounter++ );
    if( length<10 )
    {
      fprintf( g_hTrace, "%-50s u(%d)  : %d\n", pSymbolName, length, value ); 
    }
    else
    {
      fprintf( g_hTrace, "%-50s u(%d) : %d\n", pSymbolName, length, value ); 
    }
  }
}

Void  SyntaxElementWriter::xWriteUvlcTr (UInt value, const Char *pSymbolName)
{
  xWriteUvlc (value);
  if( g_HLSTraceEnable )
  {
    fprintf( g_hTrace, "%8lld  ", g_nSymbolCounter++ );
    fprintf( g_hTrace, "%-50s ue(v) : %d\n", pSymbolName, value ); 
  }
}

Void  SyntaxElementWriter::xWriteSvlcTr (Int value, const Char *pSymbolName)
{
  xWriteSvlc(value);
  if( g_HLSTraceEnable )
  {
    fprintf( g_hTrace, "%8lld  ", g_nSymbolCounter++ );
    fprintf( g_hTrace, "%-50s se(v) : %d\n", pSymbolName, value ); 
  }
}

Void  SyntaxElementWriter::xWriteFlagTr(UInt value, const Char *pSymbolName)
{
  xWriteFlag(value);
  if( g_HLSTraceEnable )
  {
    fprintf( g_hTrace, "%8lld  ", g_nSymbolCounter++ );
    fprintf( g_hTrace, "%-50s u(1)  : %d\n", pSymbolName, value ); 
  }
}

#endif


Void SyntaxElementWriter::xWriteCode     ( UInt uiCode, UInt uiLength )
{
  assert ( uiLength > 0 );
  m_pcBitIf->write( uiCode, uiLength );
}

Void SyntaxElementWriter::xWriteUvlc     ( UInt uiCode )
{
  UInt uiLength = 1;
  UInt uiTemp = ++uiCode;
  
  assert ( uiTemp );
  
  while( 1 != uiTemp )
  {
    uiTemp >>= 1;
    uiLength += 2;
  }
  // Take care of cases where uiLength > 32
  m_pcBitIf->write( 0, uiLength >> 1);
  m_pcBitIf->write( uiCode, (uiLength+1) >> 1);
}

Void SyntaxElementWriter::xWriteSvlc     ( Int iCode )
{
  UInt uiCode;
  
  uiCode = xConvertToUInt( iCode );
  xWriteUvlc( uiCode );
}

Void SyntaxElementWriter::xWriteFlag( UInt uiCode )
{
  m_pcBitIf->write( uiCode, 1 );
}

//! \}
