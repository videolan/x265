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

/** \file     TEncBinCoderCABAC.cpp
    \brief    binary entropy encoder of CABAC
*/

#include "TEncBinCoderCABAC.h"
#include "TLibCommon/TComRom.h"


//! \ingroup TLibEncoder
//! \{


TEncBinCABAC::TEncBinCABAC()
: m_pcTComBitIf( 0 )
, m_binCountIncrement( 0 )
#if FAST_BIT_EST
, m_fracBits( 0 )
#endif
{
}

TEncBinCABAC::~TEncBinCABAC()
{
}

Void TEncBinCABAC::init( TComBitIf* pcTComBitIf )
{
  m_pcTComBitIf = pcTComBitIf;
}

Void TEncBinCABAC::uninit()
{
  m_pcTComBitIf = 0;
}

Void TEncBinCABAC::start()
{
  m_uiLow            = 0;
  m_uiRange          = 510;
  m_bitsLeft         = 23;
  m_numBufferedBytes = 0;
  m_bufferedByte     = 0xff;
}

Void TEncBinCABAC::finish()
{
  if ( m_uiLow >> ( 32 - m_bitsLeft ) )
  {
    //assert( m_numBufferedBytes > 0 );
    //assert( m_bufferedByte != 0xff );
    m_pcTComBitIf->write( m_bufferedByte + 1, 8 );
    while ( m_numBufferedBytes > 1 )
    {
      m_pcTComBitIf->write( 0x00, 8 );
      m_numBufferedBytes--;
    }
    m_uiLow -= 1 << ( 32 - m_bitsLeft );
  }
  else
  {
    if ( m_numBufferedBytes > 0 )
    {
      m_pcTComBitIf->write( m_bufferedByte, 8 );
    }
    while ( m_numBufferedBytes > 1 )
    {
      m_pcTComBitIf->write( 0xff, 8 );
      m_numBufferedBytes--;
    }    
  }
  m_pcTComBitIf->write( m_uiLow >> 8, 24 - m_bitsLeft );
}

Void TEncBinCABAC::flush()
{
  encodeBinTrm(1);
  finish();
  m_pcTComBitIf->write(1, 1);
  m_pcTComBitIf->writeAlignZero();

  start();
}

/** Reset BAC register and counter values.
 * \returns Void
 */
Void TEncBinCABAC::resetBac()
{
  start();
}

/** Encode PCM alignment zero bits.
 * \returns Void
 */
Void TEncBinCABAC::encodePCMAlignBits()
{
  finish();
  m_pcTComBitIf->write(1, 1);
  m_pcTComBitIf->writeAlignZero(); // pcm align zero
}

/** Write a PCM code.
 * \param uiCode code value
 * \param uiLength code bit-depth
 * \returns Void
 */
Void TEncBinCABAC::xWritePCMCode(UInt uiCode, UInt uiLength)
{
  m_pcTComBitIf->write(uiCode, uiLength);
}

Void TEncBinCABAC::copyState( TEncBinIf* pcTEncBinIf )
{
  TEncBinCABAC* pcTEncBinCABAC = pcTEncBinIf->getTEncBinCABAC();
  m_uiLow           = pcTEncBinCABAC->m_uiLow;
  m_uiRange         = pcTEncBinCABAC->m_uiRange;
  m_bitsLeft        = pcTEncBinCABAC->m_bitsLeft;
  m_bufferedByte    = pcTEncBinCABAC->m_bufferedByte;
  m_numBufferedBytes = pcTEncBinCABAC->m_numBufferedBytes;
#if FAST_BIT_EST
  m_fracBits = pcTEncBinCABAC->m_fracBits;
#endif
}

Void TEncBinCABAC::resetBits()
{
  m_uiLow            = 0;
  m_bitsLeft         = 23;
  m_numBufferedBytes = 0;
  m_bufferedByte     = 0xff;
  if ( m_binCountIncrement )
  {
    m_uiBinsCoded = 0;
  }
#if FAST_BIT_EST
  m_fracBits &= 32767;
#endif
}

UInt TEncBinCABAC::getNumWrittenBits()
{
  return m_pcTComBitIf->getNumberOfWrittenBits() + 8 * m_numBufferedBytes + 23 - m_bitsLeft;
}

/**
 * \brief Encode bin
 *
 * \param binValue   bin value
 * \param rcCtxModel context model
 */
Void TEncBinCABAC::encodeBin( UInt binValue, ContextModel &rcCtxModel )
{
  {
    DTRACE_CABAC_VL( g_nSymbolCounter++ )
    DTRACE_CABAC_T( "\tstate=" )
    DTRACE_CABAC_V( ( rcCtxModel.getState() << 1 ) + rcCtxModel.getMps() )
    DTRACE_CABAC_T( "\tsymbol=" )
    DTRACE_CABAC_V( binValue )
    DTRACE_CABAC_T( "\n" )
  }
  m_uiBinsCoded += m_binCountIncrement;
  rcCtxModel.setBinsCoded( 1 );
  
  UInt  uiLPS   = TComCABACTables::sm_aucLPSTable[ rcCtxModel.getState() ][ ( m_uiRange >> 6 ) & 3 ];
  m_uiRange    -= uiLPS;
  
  if( binValue != rcCtxModel.getMps() )
  {
    Int numBits = TComCABACTables::sm_aucRenormTable[ uiLPS >> 3 ];
    m_uiLow     = ( m_uiLow + m_uiRange ) << numBits;
    m_uiRange   = uiLPS << numBits;
    rcCtxModel.updateLPS();
    
    m_bitsLeft -= numBits;
  }
  else
  {
    rcCtxModel.updateMPS();
    if ( m_uiRange >= 256 )
    {
      return;
    }
    
    m_uiLow <<= 1;
    m_uiRange <<= 1;
    m_bitsLeft--;
  }
  
  testAndWriteOut();
}

/**
 * \brief Encode equiprobable bin
 *
 * \param binValue bin value
 */
Void TEncBinCABAC::encodeBinEP( UInt binValue )
{
  {
    DTRACE_CABAC_VL( g_nSymbolCounter++ )
    DTRACE_CABAC_T( "\tEPsymbol=" )
    DTRACE_CABAC_V( binValue )
    DTRACE_CABAC_T( "\n" )
  }
  m_uiBinsCoded += m_binCountIncrement;
  m_uiLow <<= 1;
  if( binValue )
  {
    m_uiLow += m_uiRange;
  }
  m_bitsLeft--;
  
  testAndWriteOut();
}

/**
 * \brief Encode equiprobable bins
 *
 * \param binValues bin values
 * \param numBins number of bins
 */
Void TEncBinCABAC::encodeBinsEP( UInt binValues, Int numBins )
{
  m_uiBinsCoded += numBins & -m_binCountIncrement;
  
  for ( Int i = 0; i < numBins; i++ )
  {
    DTRACE_CABAC_VL( g_nSymbolCounter++ )
    DTRACE_CABAC_T( "\tEPsymbol=" )
    DTRACE_CABAC_V( ( binValues >> ( numBins - 1 - i ) ) & 1 )
    DTRACE_CABAC_T( "\n" )
  }
  
  while ( numBins > 8 )
  {
    numBins -= 8;
    UInt pattern = binValues >> numBins; 
    m_uiLow <<= 8;
    m_uiLow += m_uiRange * pattern;
    binValues -= pattern << numBins;
    m_bitsLeft -= 8;
    
    testAndWriteOut();
  }
  
  m_uiLow <<= numBins;
  m_uiLow += m_uiRange * binValues;
  m_bitsLeft -= numBins;
  
  testAndWriteOut();
}

/**
 * \brief Encode terminating bin
 *
 * \param binValue bin value
 */
Void TEncBinCABAC::encodeBinTrm( UInt binValue )
{
  m_uiBinsCoded += m_binCountIncrement;
  m_uiRange -= 2;
  if( binValue )
  {
    m_uiLow  += m_uiRange;
    m_uiLow <<= 7;
    m_uiRange = 2 << 7;
    m_bitsLeft -= 7;
  }
  else if ( m_uiRange >= 256 )
  {
    return;
  }
  else
  {
    m_uiLow   <<= 1;
    m_uiRange <<= 1;
    m_bitsLeft--;    
  }
  
  testAndWriteOut();
}

Void TEncBinCABAC::testAndWriteOut()
{
  if ( m_bitsLeft < 12 )
  {
    writeOut();
  }
}

/**
 * \brief Move bits from register into bitstream
 */
Void TEncBinCABAC::writeOut()
{
  UInt leadByte = m_uiLow >> (24 - m_bitsLeft);
  m_bitsLeft += 8;
  m_uiLow &= 0xffffffffu >> m_bitsLeft;
  
  if ( leadByte == 0xff )
  {
    m_numBufferedBytes++;
  }
  else
  {
    if ( m_numBufferedBytes > 0 )
    {
      UInt carry = leadByte >> 8;
      UInt byte = m_bufferedByte + carry;
      m_bufferedByte = leadByte & 0xff;
      m_pcTComBitIf->write( byte, 8 );
      
      byte = ( 0xff + carry ) & 0xff;
      while ( m_numBufferedBytes > 1 )
      {
        m_pcTComBitIf->write( byte, 8 );
        m_numBufferedBytes--;
      }
    }
    else
    {
      m_numBufferedBytes = 1;
      m_bufferedByte = leadByte;
    }      
  }    
}

//! \}
