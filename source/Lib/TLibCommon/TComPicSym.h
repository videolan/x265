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

#ifndef X265_TCOMPICSYM_H
#define X265_TCOMPICSYM_H

#include "common.h"
#include "slice.h"
#include "TComDataCU.h"

namespace x265 {
// private namespace

class PicYuv;

class TComPicSym
{
public:

    Slice*         m_slice;
    SAOParam*      m_saoParam;

    TComPicSym*    m_freeListNext;
    PicYuv*        m_reconPicYuv;
    bool           m_bHasReferences; // used during DPB/RPS updates

    DataCUMemPool  m_cuMemPool;
    MVFieldMemPool m_mvFieldMemPool;
    TComDataCU*    m_picCTU;

    uint32_t       m_numPartitions;    /* based on g_maxFullDepth, could be CU static */
    uint32_t       m_numPartInCUSize;  /* based on g_maxFullDepth, could be CU static */
    uint32_t       m_numCUsInFrame;    /* based on param, should perhaps be in Frame */

    TComPicSym();

    bool        create(x265_param *param);
    void        destroy();
    TComDataCU* getPicCTU(uint32_t ctuAddr)    { return &m_picCTU[ctuAddr]; }
};
}

#endif // ifndef X265_TCOMPICSYM_H
