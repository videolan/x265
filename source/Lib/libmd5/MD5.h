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
#pragma once
#include "libmd5.h"

//! \ingroup libMD5
//! \{

class MD5
{
public:
  /**
   * initialize digest state
   */
  MD5()
  {
    MD5Init(&m_state);
  }

  /**
   * compute digest over buf of length len.
   * multiple calls may extend the digest over more data.
   */
  void update(unsigned char *buf, unsigned len)
  {
    MD5Update(&m_state, buf, len);
  }

  /**
   * flush any outstanding MD5 data, write the digest into digest.
   */
  void finalize(unsigned char digest[16])
  {
    MD5Final(digest, &m_state);
  }

private:
  context_md5_t m_state;
};


/**
 * Produce an ascii(hex) representation of picture digest.
 *
 * Returns: a statically allocated null-terminated string.  DO NOT FREE.
 */
inline const char*
digestToString(const unsigned char digest[3][16], int numChar)
{
  const char* hex = "0123456789abcdef";
  static char string[99];
  int cnt=0;
  for(int yuvIdx=0; yuvIdx<3; yuvIdx++)
  {
    for (int i = 0; i < numChar; i++)
    {
      string[cnt++] = hex[digest[yuvIdx][i] >> 4];
      string[cnt++] = hex[digest[yuvIdx][i] & 0xf];
    }
    string[cnt++] = ',';
  }

  string[cnt-1] = '\0';
  return string;
}
//! \}
