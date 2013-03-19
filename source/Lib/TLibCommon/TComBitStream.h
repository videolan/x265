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

/** \file     TComBitStream.h
    \brief    class for handling bitstream (header)
*/

#ifndef __COMBITSTREAM__
#define __COMBITSTREAM__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <stdint.h>
#include <vector>
#include <stdio.h>
#include <assert.h>
#include "CommonDef.h"

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// pure virtual class for basic bit handling
class TComBitIf
{
public:
  virtual Void        writeAlignOne         () {};
  virtual Void        writeAlignZero        () {};
  virtual Void        write                 ( UInt uiBits, UInt uiNumberOfBits )  = 0;
  virtual Void        resetBits             ()                                    = 0;
  virtual UInt getNumberOfWrittenBits() const = 0;
  virtual ~TComBitIf() {}
};

/**
 * Model of a writable bitstream that accumulates bits to produce a
 * bytestream.
 */
class TComOutputBitstream : public TComBitIf
{
  /**
   * FIFO for storage of bytes.  Use:
   *  - fifo.push_back(x) to append words
   *  - fifo.clear() to empty the FIFO
   *  - &fifo.front() to get a pointer to the data array.
   *    NB, this pointer is only valid until the next push_back()/clear()
   */
  std::vector<uint8_t> *m_fifo;

  UInt m_num_held_bits; /// number of bits not flushed to bytestream.
  UChar m_held_bits; /// the bits held and not flushed to bytestream.
                             /// this value is always msb-aligned, bigendian.

public:
  // create / destroy
  TComOutputBitstream();
  ~TComOutputBitstream();

  // interface for encoding
  /**
   * append uiNumberOfBits least significant bits of uiBits to
   * the current bitstream
   */
  Void        write           ( UInt uiBits, UInt uiNumberOfBits );

  /** insert one bits until the bitstream is byte-aligned */
  Void        writeAlignOne   ();

  /** insert zero bits until the bitstream is byte-aligned */
  Void        writeAlignZero  ();

  /** this function should never be called */
  void resetBits() { assert(0); }

  // utility functions

  /**
   * Return a pointer to the start of the byte-stream buffer.
   * Pointer is valid until the next write/flush/reset call.
   * NB, data is arranged such that subsequent bytes in the
   * bytestream are stored in ascending addresses.
   */
  Char* getByteStream() const;

  /**
   * Return the number of valid bytes available from  getByteStream()
   */
  UInt getByteStreamLength();

  /**
   * Reset all internal state.
   */
  Void clear();

  /**
   * returns the number of bits that need to be written to
   * achieve byte alignment.
   */
  Int getNumBitsUntilByteAligned() { return (8 - m_num_held_bits) & 0x7; }

  /**
   * Return the number of bits that have been written since the last clear()
   */
  UInt getNumberOfWrittenBits() const { return UInt(m_fifo->size()) * 8 + m_num_held_bits; }

  void insertAt(const TComOutputBitstream& src, UInt pos);

  /**
   * Return a reference to the internal fifo
   */
  std::vector<uint8_t>& getFIFO() { return *m_fifo; }

  UChar getHeldBits  ()          { return m_held_bits;          }

  TComOutputBitstream& operator= (const TComOutputBitstream& src);
  /** Return a reference to the internal fifo */
  std::vector<uint8_t>& getFIFO() const { return *m_fifo; }

  Void          addSubstream    ( TComOutputBitstream* pcSubstream );
  Void writeByteAlignment();

  //! returns the number of start code emulations contained in the current buffer
  Int countStartCodeEmulations();
};

/**
 * Model of an input bitstream that extracts bits from a predefined
 * bytestream.
 */
class TComInputBitstream
{
  std::vector<uint8_t> *m_fifo; /// FIFO for storage of complete bytes
  std::vector<UInt> m_emulationPreventionByteLocation;

protected:
  UInt m_fifo_idx; /// Read index into m_fifo

  UInt m_num_held_bits;
  UChar m_held_bits;
  UInt  m_numBitsRead;

public:
  /**
   * Create a new bitstream reader object that reads from #buf#.  Ownership
   * of #buf# remains with the callee, although the constructed object
   * will hold a reference to #buf#
   */
  TComInputBitstream(std::vector<uint8_t>* buf);
  ~TComInputBitstream();

  // interface for decoding
  Void        pseudoRead      ( UInt uiNumberOfBits, UInt& ruiBits );
  Void        read            ( UInt uiNumberOfBits, UInt& ruiBits );
  Void        readByte        ( UInt &ruiBits )
  {
    assert(m_fifo_idx < m_fifo->size());
    ruiBits = (*m_fifo)[m_fifo_idx++];
  }

  Void        readOutTrailingBits ();
  UChar getHeldBits  ()          { return m_held_bits;          }
  TComOutputBitstream& operator= (const TComOutputBitstream& src);
  UInt  getByteLocation              ( )                     { return m_fifo_idx                    ; }

  // Peek at bits in word-storage. Used in determining if we have completed reading of current bitstream and therefore slice in LCEC.
  UInt        peekBits (UInt uiBits) { UInt tmp; pseudoRead(uiBits, tmp); return tmp; }

  // utility functions
  UInt read(UInt numberOfBits) { UInt tmp; read(numberOfBits, tmp); return tmp; }
  UInt     readByte() { UInt tmp; readByte( tmp ); return tmp; }
  UInt getNumBitsUntilByteAligned() { return m_num_held_bits & (0x7); }
  UInt getNumBitsLeft() { return 8*((UInt)m_fifo->size() - m_fifo_idx) + m_num_held_bits; }
  TComInputBitstream *extractSubstream( UInt uiNumBits ); // Read the nominated number of bits, and return as a bitstream.
  Void                deleteFifo(); // Delete internal fifo of bitstream.
  UInt  getNumBitsRead() { return m_numBitsRead; }
  Void readByteAlignment();

  Void      pushEmulationPreventionByteLocation ( UInt pos )                  { m_emulationPreventionByteLocation.push_back( pos ); }
  UInt      numEmulationPreventionBytesRead     ()                            { return (UInt) m_emulationPreventionByteLocation.size();    }
  std::vector<UInt>  getEmulationPreventionByteLocation  ()                   { return m_emulationPreventionByteLocation;           }
  UInt      getEmulationPreventionByteLocation  ( UInt idx )                  { return m_emulationPreventionByteLocation[ idx ];    }
  Void      clearEmulationPreventionByteLocation()                            { m_emulationPreventionByteLocation.clear();          }
  Void      setEmulationPreventionByteLocation  ( std::vector<UInt> vec )     { m_emulationPreventionByteLocation = vec;            }
};

//! \}

#endif
