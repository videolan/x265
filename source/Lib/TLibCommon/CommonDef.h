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

/** \file     CommonDef.h
    \brief    Defines constants, macros and tool parameters
*/

#ifndef __COMMONDEF__
#define __COMMONDEF__

#include <algorithm>
#include <malloc.h>
#include <stdlib.h>
#include "TypeDef.h"

//! \ingroup TLibCommon
//! \{

#ifndef NULL
#define NULL              0
#endif

// ====================================================================================================================
// Common constants
// ====================================================================================================================

#define _SUMMARY_OUT_               0           ///< print-out PSNR results of all slices to summary.txt
#define _SUMMARY_PIC_               0           ///< print-out PSNR results for each slice type to summary.txt

#define MAX_GOP                     64          ///< max. value of hierarchical GOP size

#define MAX_NUM_REF_PICS            16          ///< max. number of pictures used for reference
#define MAX_NUM_REF                 16          ///< max. number of entries in picture reference list

#define MAX_UINT                    0xFFFFFFFFU ///< max. value of unsigned 32-bit integer
#define MAX_INT                     2147483647  ///< max. value of signed 32-bit integer
#define MAX_INT64                   0x7FFFFFFFFFFFFFFFLL  ///< max. value of signed 64-bit integer
#define MAX_DOUBLE                  1.7e+308    ///< max. value of double-type value

#define MIN_QP                      0
#define MAX_QP                      51

#define NOT_VALID                   -1

// ====================================================================================================================
// Macro functions
// ====================================================================================================================
extern Int g_bitDepthY;
extern Int g_bitDepthC;

/** clip x, such that 0 <= x <= #g_maxLumaVal */
template<typename T>
inline T ClipY(T x) { return std::min<T>(T((1 << g_bitDepthY) - 1), std::max<T>(T(0), x)); }

template<typename T>
inline T ClipC(T x) { return std::min<T>(T((1 << g_bitDepthC) - 1), std::max<T>(T(0), x)); }

/** clip a, such that minVal <= a <= maxVal */
template<typename T>
inline T Clip3(T minVal, T maxVal, T a) { return std::min<T>(std::max<T>(minVal, a), maxVal); }                            ///< general min/max clip

#if _WIN32
#define xMalloc(type, len)        _aligned_malloc(sizeof(type) * (len), 32)
#define xFree(ptr)                _aligned_free(ptr)
#if __MINGW32__
#define _aligned_malloc           __mingw_aligned_malloc
#define _aligned_free             __mingw_aligned_free
#endif
#else
#define xMalloc(type, len)        memalign(32, sizeof(type) * (len))
#define xFree(ptr)                free(ptr)
#endif

#define FATAL_ERROR_0(MESSAGE, EXITCODE) \
    {                                    \
        printf(MESSAGE);                 \
        exit(EXITCODE);                  \
    }

// ====================================================================================================================
// Coding tool configuration
// ====================================================================================================================

// AMVP: advanced motion vector prediction
#define AMVP_MAX_NUM_CANDS          2           ///< max number of final candidates
#define AMVP_MAX_NUM_CANDS_MEM      3           ///< max number of candidates
// MERGE
#define MRG_MAX_NUM_CANDS           5

// Reference memory management
#define DYN_REF_FREE                0           ///< dynamic free of reference memories

// Explicit temporal layer QP offset
#define MAX_TLAYER                  8           ///< max number of temporal layer
#define HB_LAMBDA_FOR_LDC           1           ///< use of B-style lambda for non-key pictures in low-delay mode

// Fast ME using smoother MV assumption
#define FASTME_SMOOTHER_MV          1           ///< reduce ME time using faster option

// Adaptive search range depending on POC difference
#define ADAPT_SR_SCALE              1           ///< division factor for adaptive search range

#define CLIP_TO_709_RANGE           0

#define MAX_CHROMA_FORMAT_IDC      3

//! \}

#endif // end of #ifndef  __COMMONDEF__
