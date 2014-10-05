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

#ifndef X265_TYPEDEF_H
#define X265_TYPEDEF_H

#include "common.h"

namespace x265 {
// private namespace

// supported slice types
enum SliceType
{
    B_SLICE,
    P_SLICE,
    I_SLICE
};

// supported partition shape
enum PartSize
{
    SIZE_2Nx2N,         // symmetric motion partition,  2Nx2N
    SIZE_2NxN,          // symmetric motion partition,  2Nx N
    SIZE_Nx2N,          // symmetric motion partition,   Nx2N
    SIZE_NxN,           // symmetric motion partition,   Nx N
    SIZE_2NxnU,         // asymmetric motion partition, 2Nx( N/2) + 2Nx(3N/2)
    SIZE_2NxnD,         // asymmetric motion partition, 2Nx(3N/2) + 2Nx( N/2)
    SIZE_nLx2N,         // asymmetric motion partition, ( N/2)x2N + (3N/2)x2N
    SIZE_nRx2N,         // asymmetric motion partition, (3N/2)x2N + ( N/2)x2N
    SIZE_NONE = 15
};

// supported prediction type
enum PredMode
{
    MODE_INTER,         // inter-prediction mode
    MODE_INTRA,         // intra-prediction mode
    MODE_NONE = 15
};

// texture component type
enum TextType
{
    TEXT_LUMA     = 0,  // luma
    TEXT_CHROMA_U = 1,  // chroma U
    TEXT_CHROMA_V = 2,  // chroma V
    MAX_NUM_COMPONENT = 3
};

// motion vector predictor direction used in AMVP
enum MVP_DIR
{
    MD_LEFT = 0,        // MVP of left block
    MD_ABOVE,           // MVP of above block
    MD_ABOVE_RIGHT,     // MVP of above right block
    MD_BELOW_LEFT,      // MVP of below left block
    MD_ABOVE_LEFT       // MVP of above left block
};

}

#endif // ifndef X265_TYPEDEF_H
