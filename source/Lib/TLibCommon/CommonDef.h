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
#include <cstdlib>
#include "TypeDef.h"

//! \ingroup TLibCommon
//! \{

#ifndef NULL
#define NULL 0
#endif

// ====================================================================================================================
// Common constants
// ====================================================================================================================

#define MAX_NUM_PICS_IN_SOP         1024

#define MAX_NESTING_NUM_OPS         1024
#define MAX_NESTING_NUM_LAYER       64

#define MAX_VPS_NUM_HRD_PARAMETERS  1
#define MAX_VPS_OP_SETS_PLUS1       1024
#define MAX_VPS_NUH_RESERVED_ZERO_LAYER_ID_PLUS1  1

#define MAX_CPB_CNT                 32 ///< Upper bound of (cpb_cnt_minus1 + 1)
#define MAX_NUM_LAYER_IDS           64

#define COEF_REMAIN_BIN_REDUCTION   3 ///< indicates the level at which the VLC

///< transitions from Golomb-Rice to TU+EG(k)

#define CU_DQP_TU_CMAX              5 ///< max number bins for truncated unary
#define CU_DQP_EG_k                 0 ///< exp-golomb order

#define SBH_THRESHOLD               4 ///< I0156: value of the fixed SBH controlling threshold

#define C1FLAG_NUMBER               8 // maximum number of largerThan1 flag coded in one chunk :  16 in HM5
#define C2FLAG_NUMBER               1 // maximum number of largerThan2 flag coded in one chunk:  16 in HM5

#define SAO_ENCODING_RATE           0.75
#define SAO_ENCODING_RATE_CHROMA    0.5

#define MAX_NUM_VPS                 16
#define MAX_NUM_SPS                 16
#define MAX_NUM_PPS                 64

#define MIN_SCAN_POS_CROSS          4

#define MLS_GRP_NUM                 64 ///< G644 : Max number of coefficient groups, max(16, 64)
#define MLS_CG_SIZE                 4 ///< G644 : Coefficient group size of 4x4

#define ARL_C_PRECISION             7 ///< G382: 7-bit arithmetic precision
#define LEVEL_RANGE                 30 ///< G382: max coefficient level in statistics collection

#define AMVP_DECIMATION_FACTOR      4

#define SCAN_SET_SIZE               16
#define LOG2_SCAN_SET_SIZE          4

#define FAST_UDI_MAX_RDMODE_NUM     35 ///< maximum number of RD comparison in fast-UDI estimation loop

#define NUM_INTRA_MODE 36
#if !REMOVE_LM_CHROMA
#define LM_CHROMA_IDX  35
#endif

#define PLANAR_IDX                  0
#define VER_IDX                     26 // index for intra VERTICAL   mode
#define HOR_IDX                     10 // index for intra HORIZONTAL mode
#define DC_IDX                      1 // index for intra DC mode
#define NUM_CHROMA_MODE             5 // total number of chroma modes
#define DM_CHROMA_IDX               36 // chroma mode index for derived from luma intra mode

#define FULL_NBIT 0 ///< When enabled, compute costs using full sample bitdepth.  When disabled, compute costs as if it is 8-bit source video.
#if FULL_NBIT || !HIGH_BIT_DEPTH
# define DISTORTION_PRECISION_ADJUSTMENT(x) 0
#else
# define DISTORTION_PRECISION_ADJUSTMENT(x) (x)
#endif

#define LOG2_MAX_NUM_COLUMNS_MINUS1 7
#define LOG2_MAX_NUM_ROWS_MINUS1    7
#define LOG2_MAX_COLUMN_WIDTH       13
#define LOG2_MAX_ROW_HEIGHT         13

#define REG_DCT                     65535

#define CABAC_INIT_PRESENT_FLAG     1

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

#define NOT_VALID                  -1

// for use in HM, replaces old xMalloc/xFree macros
#define X265_MALLOC(type, count)    x265_malloc(sizeof(type) * (count))
#define X265_FREE(ptr)              x265_free(ptr)

// new code can use these functions directly
extern void  x265_free(void *);
extern void *x265_malloc(size_t size);

// ====================================================================================================================
// Coding tool configuration
// ====================================================================================================================

// AMVP: advanced motion vector prediction
#define AMVP_MAX_NUM_CANDS          2 ///< max number of final candidates
#define AMVP_MAX_NUM_CANDS_MEM      3 ///< max number of candidates
#define MRG_MAX_NUM_CANDS           5

// Explicit temporal layer QP offset
#define MAX_TLAYER                  8 ///< max number of temporal layer

// Adaptive search range depending on POC difference
#define ADAPT_SR_SCALE              1 ///< division factor for adaptive search range

#define MAX_CHROMA_FORMAT_IDC       3

//! \}

#endif // end of #ifndef  __COMMONDEF__
