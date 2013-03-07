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

/**
 * \file
 * \brief Declaration of TComInterpolationFilter class
 */

#ifndef __HM_TCOMINTERPOLATIONFILTER_H__
#define __HM_TCOMINTERPOLATIONFILTER_H__

#include "TypeDef.h"

//! \ingroup TLibCommon
//! \{

#define NTAPS_LUMA        8 ///< Number of taps for luma
#define NTAPS_CHROMA      4 ///< Number of taps for chroma
#define IF_INTERNAL_PREC 14 ///< Number of bits for internal precision
#define IF_FILTER_PREC    6 ///< Log2 of sum of filter taps
#define IF_INTERNAL_OFFS (1<<(IF_INTERNAL_PREC-1)) ///< Offset used internally

/**
 * \brief Interpolation filter class
 */
class TComInterpolationFilter
{
  static const Short m_lumaFilter[4][NTAPS_LUMA];     ///< Luma filter taps
  static const Short m_chromaFilter[8][NTAPS_CHROMA]; ///< Chroma filter taps
  
  static Void filterCopy(Int bitDepth, const Pel *src, Int srcStride, Short *dst, Int dstStride, Int width, Int height, Bool isFirst, Bool isLast);
  
  template<Int N, Bool isVertical, Bool isFirst, Bool isLast>
  static Void filter(Int bitDepth, Pel const *src, Int srcStride, Short *dst, Int dstStride, Int width, Int height, Short const *coeff);

  template<Int N>
  static Void filterHor(Int bitDepth, Pel *src, Int srcStride, Short *dst, Int dstStride, Int width, Int height,               Bool isLast, Short const *coeff);
  template<Int N>
  static Void filterVer(Int bitDepth, Pel *src, Int srcStride, Short *dst, Int dstStride, Int width, Int height, Bool isFirst, Bool isLast, Short const *coeff);

public:
  TComInterpolationFilter() {}
  ~TComInterpolationFilter() {}

  Void filterHorLuma  (Pel *src, Int srcStride, Short *dst, Int dstStride, Int width, Int height, Int frac,               Bool isLast );
  Void filterVerLuma  (Pel *src, Int srcStride, Short *dst, Int dstStride, Int width, Int height, Int frac, Bool isFirst, Bool isLast );
  Void filterHorChroma(Pel *src, Int srcStride, Short *dst, Int dstStride, Int width, Int height, Int frac,               Bool isLast );
  Void filterVerChroma(Pel *src, Int srcStride, Short *dst, Int dstStride, Int width, Int height, Int frac, Bool isFirst, Bool isLast );
};

//! \}

#endif
