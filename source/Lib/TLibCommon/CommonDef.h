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

#ifndef X265_COMMONDEF_H
#define X265_COMMONDEF_H

#include "TypeDef.h"

#ifndef NULL
#define NULL 0
#endif

#define MAX_UINT                    0xFFFFFFFFU // max. value of unsigned 32-bit integer
#define MAX_INT                     2147483647  // max. value of signed 32-bit integer
#define MAX_INT64                   0x7FFFFFFFFFFFFFFFLL  // max. value of signed 64-bit integer
#define MAX_DOUBLE                  1.7e+308    // max. value of double-type value

#define COEF_REMAIN_BIN_REDUCTION   3 // indicates the level at which the VLC
                                      // transitions from Golomb-Rice to TU+EG(k)

#define SBH_THRESHOLD               4 // fixed sign bit hiding controlling threshold

#define C1FLAG_NUMBER               8 // maximum number of largerThan1 flag coded in one chunk:  16 in HM5
#define C2FLAG_NUMBER               1 // maximum number of largerThan2 flag coded in one chunk:  16 in HM5

#define SAO_ENCODING_RATE           0.75
#define SAO_ENCODING_RATE_CHROMA    0.5

#define MLS_GRP_NUM                 64 // Max number of coefficient groups, max(16, 64)
#define MLS_CG_SIZE                 4  // Coefficient group size of 4x4
#define MLS_CG_LOG2_SIZE            2

#define QUANT_IQUANT_SHIFT          20 // Q(QP%6) * IQ(QP%6) = 2^20
#define QUANT_SHIFT                 14 // Q(4) = 2^14
#define SCALE_BITS                  15 // Inherited from TMuC, presumably for fractional bit estimates in RDOQ
#define MAX_TR_DYNAMIC_RANGE        15 // Maximum transform dynamic range (excluding sign bit)

#define SHIFT_INV_1ST               7  // Shift after first inverse transform stage
#define SHIFT_INV_2ND               12 // Shift after second inverse transform stage

#define AMVP_DECIMATION_FACTOR      4

#define SCAN_SET_SIZE               16
#define LOG2_SCAN_SET_SIZE          4

#define FAST_UDI_MAX_RDMODE_NUM     35 // maximum number of RD comparison in fast-UDI estimation loop

#define ALL_IDX                     -1
#define PLANAR_IDX                  0
#define VER_IDX                     26 // index for intra VERTICAL   mode
#define HOR_IDX                     10 // index for intra HORIZONTAL mode
#define DC_IDX                      1  // index for intra DC mode
#define NUM_CHROMA_MODE             5  // total number of chroma modes
#define DM_CHROMA_IDX               36 // chroma mode index for derived from luma intra mode

#define MDCS_ANGLE_LIMIT            4 // distance from true angle that horiz or vertical scan is allowed
#define MDCS_LOG2_MAX_SIZE          3 // TUs with log2 of size greater than this can only use diagonal scan

#define MAX_NUM_REF_PICS            16 // max. number of pictures used for reference
#define MAX_NUM_REF                 16 // max. number of entries in picture reference list

#define REF_PIC_LIST_0              0
#define REF_PIC_LIST_1              1
#define REF_PIC_LIST_X              100
#define NOT_VALID                   -1

#define MIN_QP                      0
#define MAX_QP                      51
#define MAX_MAX_QP                  69

#define MIN_QPSCALE                 0.21249999999999999
#define MAX_MAX_QPSCALE             615.46574234477100

#define AMVP_NUM_CANDS              2 // number of AMVP candidates
#define MRG_MAX_NUM_CANDS           5 // max number of final merge candidates

#define MAX_CHROMA_FORMAT_IDC       3 //  TODO: Remove me

#define CHROMA_H_SHIFT(x) (x == X265_CSP_I420 || x == X265_CSP_I422)
#define CHROMA_V_SHIFT(x) (x == X265_CSP_I420)

#endif // ifndef X265_COMMONDEF_H
