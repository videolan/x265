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

/** \file     TComMotionInfo.h
    \brief    motion information handling classes (header)
    \todo     TComMvField seems to be better to be inherited from MV
*/

#ifndef X265_TCOMMOTIONINFO_H
#define X265_TCOMMOTIONINFO_H

#include "common.h"
#include "mv.h"

//! \ingroup TLibCommon
//! \{

namespace x265 {
// private namespace

// ====================================================================================================================
// Type definition
// ====================================================================================================================

typedef struct
{
    MV*   m_mvMemBlock;
    MV*   m_mvdMemBlock;
    char* m_refIdxMemBlock;
} MVFieldMemPool;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// class for motion vector with reference index
class TComMvField
{
public:

    MV  mv;
    int       refIdx;

    TComMvField() : refIdx(NOT_VALID) {}

    void setMvField(const MV & _mv, int _refIdx)
    {
        mv     = _mv;
        refIdx = _refIdx;
    }
};

/// class for motion information in one CU
class TComCUMvField
{
public:

    MV* m_mv;
    MV* m_mvd;
    char*     m_refIdx;
    uint32_t  m_numPartitions;

    template<typename T>
    void setAll(T *p, T const & val, PartSize cuMode, int partAddr, uint32_t depth, int partIdx);

    TComCUMvField() : m_mv(NULL), m_mvd(NULL), m_refIdx(NULL), m_numPartitions(0) {}

    ~TComCUMvField() {}

    // ------------------------------------------------------------------------------------------------------------------
    // initialize / create / destroy
    // ------------------------------------------------------------------------------------------------------------------

    MVFieldMemPool m_MVFieldMemPool;

    bool initialize(uint32_t numPartition, uint32_t numBlocks);
    void create(TComCUMvField *p, uint32_t numPartition, int index, int idx);
    void destroy();

    // ------------------------------------------------------------------------------------------------------------------
    // clear / copy
    // ------------------------------------------------------------------------------------------------------------------

    void clearMvField();

    void copyFrom(const TComCUMvField * cuMvFieldSrc, int numPartSrc, int partAddrDst);
    void copyTo(TComCUMvField* cuMvFieldDst, int partAddrDst) const;
    void copyTo(TComCUMvField* cuMvFieldDst, int partAddrDst, uint32_t offset, uint32_t numPart) const;

    // ------------------------------------------------------------------------------------------------------------------
    // get
    // ------------------------------------------------------------------------------------------------------------------

    const MV & getMv(int idx) const { return m_mv[idx]; }

    const MV & getMvd(int idx) const { return m_mvd[idx]; }

    int getRefIdx(int idx) const { return m_refIdx[idx]; }

    // ------------------------------------------------------------------------------------------------------------------
    // set
    // ------------------------------------------------------------------------------------------------------------------

    void    setAllMv(const MV& mv,                    PartSize cuMode, int partAddr, uint32_t depth, int partIdx = 0);
    void    setAllRefIdx(int refIdx,                  PartSize mbMode, int partAddr, uint32_t depth, int partIdx = 0);
    void    setAllMvField(const TComMvField& mvField, PartSize mbMode, int partAddr, uint32_t depth, int partIdx = 0);
    void    setMvd(int idx, const MV& mvd) { m_mvd[idx] = mvd; }
};
}

//! \}

#endif // ifndef X265_TCOMMOTIONINFO_H
